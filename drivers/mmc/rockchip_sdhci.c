// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2016 Fuzhou Rockchip Electronics Co., Ltd
 *
 * Rockchip SD Host Controller Interface
 */

#include <common.h>
#include <dm.h>
#include <dt-structs.h>
#include <linux/libfdt.h>
#include <malloc.h>
#include <mapmem.h>
#include <sdhci.h>
#include <clk.h>
#include <asm/arch-rockchip/clock.h>
#include <syscon.h>

#define EMMC_MIN_FREQ	400000

/* */
#define PHY_CLRSETBITS(clr, set) ((((clr) | (set)) << 16) | (set))

/* */
#define PHYCTRL_CALDONE_MASK		0x1
#define PHYCTRL_CALDONE_SHIFT		0x6
#define PHYCTRL_CALDONE_DONE		0x1

#define PHYCTRL_DLLRDY_MASK			0x1
#define PHYCTRL_DLLRDY_SHIFT		0x5
#define PHYCTRL_DLLRDY_DONE			0x1

#define PHYCTRL_FREQSEL_200M        0x0
#define PHYCTRL_FREQSEL_50M         0x1
#define PHYCTRL_FREQSEL_100M        0x2
#define PHYCTRL_FREQSEL_150M        0x3

/* */
struct rockchip_sdhc_plat {
#if CONFIG_IS_ENABLED(OF_PLATDATA)
	struct dtd_rockchip_rk3399_sdhci_5_1 dtplat;
#endif
	struct mmc_config cfg;
	struct mmc mmc;
};

struct rockchip_emmc_phy {
	u32 con[7];
	u32 reserved;
	u32 status;
};

struct rockchip_sdhc {
	struct sdhci_host host;
	void *base;
	struct rockchip_emmc_phy *phy;
};

static void arasan_phy_power_on(struct sdhci_host *host, u32 clock)
{
	struct rockchip_sdhc *priv =
			container_of(host, struct rockchip_sdhc, host);
	struct rockchip_emmc_phy *phy = priv->phy;
	u32 caldone, dllrdy, freqsel;
	uint start;

	writel(PHY_CLRSETBITS(7 << 4, 0), &phy->con[6]);
	writel(PHY_CLRSETBITS(1 << 11, 1 << 11), &phy->con[0]);
	writel(PHY_CLRSETBITS(0xf << 7, 6 << 7), &phy->con[0]);

	/*
	 * According to the user manual, calpad calibration
	 * cycle takes more than 2us without the minimal recommended
	 * value, so we may need a little margin here
	 */
	udelay(3);
	writel(PHY_CLRSETBITS(1, 1), &phy->con[6]);

	/*
	 * According to the user manual, it asks driver to
	 * wait 5us for calpad busy trimming
	 */
	udelay(5);
	caldone = readl(&phy->status);
	caldone = (caldone >> PHYCTRL_CALDONE_SHIFT) & PHYCTRL_CALDONE_MASK;

	if (caldone != PHYCTRL_CALDONE_DONE) {
		pr_debug("%s: caldone timeout.\n", __func__);
		return;
	}

	/* Set the frequency of the DLL operation */
	if (clock < 75000000UL)
		freqsel = PHYCTRL_FREQSEL_50M;
	else if (clock < 125000000UL)
		freqsel = PHYCTRL_FREQSEL_100M;
	else if (clock < 175000000UL)
		freqsel = PHYCTRL_FREQSEL_150M;
	else
		freqsel = PHYCTRL_FREQSEL_200M;

	/* Set the frequency of the DLL operation */
	writel(PHY_CLRSETBITS(3 << 12, freqsel << 12), &phy->con[0]);
	writel(PHY_CLRSETBITS(1 << 1, 1 << 1), &phy->con[6]);

	start = get_timer(0);

	do {
		udelay(1);
		dllrdy = readl(&phy->status);
		dllrdy = (dllrdy >> PHYCTRL_DLLRDY_SHIFT) & PHYCTRL_DLLRDY_MASK;
		if (dllrdy == PHYCTRL_DLLRDY_DONE)
			break;
	} while (get_timer(start) < 50000);

	if (dllrdy != PHYCTRL_DLLRDY_DONE)
		pr_debug("%s: dllrdy timeout.\n", __func__);
}

static void arasan_phy_power_off(struct sdhci_host *host)
{
	struct rockchip_sdhc *priv =
			container_of(host, struct rockchip_sdhc, host);
	struct rockchip_emmc_phy *phy = priv->phy;

	writel(PHY_CLRSETBITS(1, 0), &phy->con[6]);
	writel(PHY_CLRSETBITS(1 << 1, 0), &phy->con[6]);
}

static struct sdhci_ops arasan_sdhci_ops = {
	.phy_power_on  = arasan_phy_power_on,
	.phy_power_off = arasan_phy_power_off,
};

static int arasan_of_get_phy(struct udevice *dev)
{
	struct rockchip_sdhc *priv = dev_get_priv(dev);
	ofnode phy_node;
	void *grf_base;
	u32 grf_phy_offset, phandle;

	phandle = dev_read_u32_default(dev, "phys", 0);
	phy_node = ofnode_get_by_phandle(phandle);

	if (!ofnode_valid(phy_node)) {
		dev_err(dev, "node not found emmc phy device\n");
		return -ENODEV;
	}

	grf_base = syscon_get_first_range(ROCKCHIP_SYSCON_GRF);
	if (grf_base < 0)
		dev_err(dev, "%s Get syscon grf failed\n", __func__);

	grf_phy_offset = ofnode_read_u32_default(phy_node, "reg", 0);

	priv->phy = (struct rockchip_emmc_phy *)(grf_base + grf_phy_offset);
	return 0;
}

static int arasan_sdhci_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct rockchip_sdhc_plat *plat = dev_get_platdata(dev);
	struct rockchip_sdhc *prv = dev_get_priv(dev);
	struct sdhci_host *host = &prv->host;
	int max_frequency, ret;
	struct clk clk;

#if CONFIG_IS_ENABLED(OF_PLATDATA)
	struct dtd_rockchip_rk3399_sdhci_5_1 *dtplat = &plat->dtplat;

	host->name = dev->name;
	host->ioaddr = map_sysmem(dtplat->reg[0], dtplat->reg[1]);
	max_frequency = dtplat->max_frequency;
	ret = clk_get_by_index_platdata(dev, 0, dtplat->clocks, &clk);
#else
	max_frequency = dev_read_u32_default(dev, "max-frequency", 0);
	ret = clk_get_by_index(dev, 0, &clk);
#endif
	if (!ret) {
		ret = clk_set_rate(&clk, max_frequency);
		if (IS_ERR_VALUE(ret))
			printf("%s clk set rate fail!\n", __func__);
	} else {
		printf("%s fail to get clk\n", __func__);
	}

	/* Get the PHY for power control */
	ret = arasan_of_get_phy(dev);
	if (ret)
		return ret;

	/* for PHY power cycling */
	host->ops = &arasan_sdhci_ops;

	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD;
	host->max_clk = max_frequency;
	/*
	 * The sdhci-driver only supports 4bit and 8bit, as sdhci_setup_cfg
	 * doesn't allow us to clear MMC_MODE_4BIT.  Consequently, we don't
	 * check for other bus-width values.
	 */
	if (host->bus_width == 8)
		host->host_caps |= MMC_MODE_8BIT;

	ret = sdhci_setup_cfg(&plat->cfg, host, 0, EMMC_MIN_FREQ);

	host->mmc = &plat->mmc;
	if (ret)
		return ret;
	host->mmc->priv = &prv->host;
	host->mmc->dev = dev;
	upriv->mmc = host->mmc;

	return sdhci_probe(dev);
}

static int arasan_sdhci_ofdata_to_platdata(struct udevice *dev)
{
#if !CONFIG_IS_ENABLED(OF_PLATDATA)
	struct sdhci_host *host = dev_get_priv(dev);

	host->name = dev->name;
	host->ioaddr = dev_read_addr_ptr(dev);
	host->bus_width = dev_read_u32_default(dev, "bus-width", 4);
#endif

	return 0;
}

static int rockchip_sdhci_bind(struct udevice *dev)
{
	struct rockchip_sdhc_plat *plat = dev_get_platdata(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static const struct udevice_id arasan_sdhci_ids[] = {
	{ .compatible = "arasan,sdhci-5.1" },
	{ }
};

U_BOOT_DRIVER(arasan_sdhci_drv) = {
	.name		= "rockchip_rk3399_sdhci_5_1",
	.id		= UCLASS_MMC,
	.of_match	= arasan_sdhci_ids,
	.ofdata_to_platdata = arasan_sdhci_ofdata_to_platdata,
	.ops		= &sdhci_ops,
	.bind		= rockchip_sdhci_bind,
	.probe		= arasan_sdhci_probe,
	.priv_auto_alloc_size = sizeof(struct rockchip_sdhc),
	.platdata_auto_alloc_size = sizeof(struct rockchip_sdhc_plat),
};
