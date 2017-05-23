/*
 * arch/arm/mach-tegra/board-molly-power.c
 *
 * Copyright (c) 2012-2013 NVIDIA Corporation. All rights reserved.
 * Copyright (c) 2013 Google, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* true molly */

#include <linux/i2c.h>
#include <linux/pda_power.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/io.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/fixed.h>
#include <linux/mfd/palmas.h>
#include <linux/mfd/tps65090.h>
#include <linux/regulator/tps65090-regulator.h>
#include <linux/regulator/tps51632-regulator.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/regulator/userspace-consumer.h>
#include <linux/wakelock.h>

#include <asm/mach-types.h>
#include <linux/power/sbs-battery.h>

#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/edp.h>
#include <mach/gpio-tegra.h>
#include <mach/hardware.h>

#include "cpu-tegra.h"
#include "pm.h"
#include "board-pmu-defines.h"
#include "board.h"
#include "gpio-names.h"
#include "board-common.h"
#include "board-molly.h"
#include "tegra_cl_dvfs.h"
#include "devices.h"
#include "tegra11_soctherm.h"
#include "tegra3_tsensor.h"

#if MOLLY_ON_DALMORE == 0

#define PMC_CTRL		0x0
#define PMC_CTRL_INTR_LOW	(1 << 17)

/***************** TPS65913 Palmas based regulator ****************/

/* vdd_cpu */
/* SMPS1, SMPS2, and SMPS3 all tied together to provide vdd_cpu */
static struct regulator_consumer_supply palmas_smps123_supply[] = {
	REGULATOR_SUPPLY("vdd_cpu", NULL),
};

/* vdd_core */
/* SMPS4 and SMPS5 are tied together to provide vdd_core */
static struct regulator_consumer_supply palmas_smps45_supply[] = {
	REGULATOR_SUPPLY("vdd_core", NULL),
	REGULATOR_SUPPLY("vdd_core", "sdhci-tegra.0"),
	REGULATOR_SUPPLY("vdd_core", "sdhci-tegra.3"),
};

/* vdd_3v3 */
static struct regulator_consumer_supply palmas_smps6_supply[] = {
	REGULATOR_SUPPLY("vdd_3v3", NULL),
};

/* vdd_ddr */
static struct regulator_consumer_supply palmas_smps7_supply[] = {
	REGULATOR_SUPPLY("vddio_ddr", NULL),
};

/* vdd_1v8 */
static struct regulator_consumer_supply palmas_smps8_supply[] = {
	REGULATOR_SUPPLY("vddio", NULL), /* VIO_IN */
#if 0 /* molly has no camera */
	REGULATOR_SUPPLY("vddio_cam", "vi"), /* VDDIO_CAM */
#endif
	REGULATOR_SUPPLY("vdd", "2-004c"), /* VDD of TMP451 */
	REGULATOR_SUPPLY("vddio_sys", NULL), /* VDDIO_SYS */
	REGULATOR_SUPPLY("avdd_osc", NULL), /* AVDD_OSC */
	REGULATOR_SUPPLY("vddio_1v8", "1-0032"), /* VDD_1V8 of LP5521 */
	REGULATOR_SUPPLY("vddio_gmi", NULL), /* VDDIO_GMI_1 and VDDIO_GMI_2 */
	REGULATOR_SUPPLY("vddio_1v8_jtag", NULL), /* VDD_1V8 of
						j-tag interface */
	REGULATOR_SUPPLY("vddio_sdmmc", "sdhci-tegra.3"), /* VDDIO_SDMMC4
							of SDMMC4 */
	REGULATOR_SUPPLY("vdd_emmc", "sdhci-tegra.3"), /* VDD of eMMC */
	REGULATOR_SUPPLY("pwrdet_sdmmc4", NULL),
	REGULATOR_SUPPLY("vddio_sdmmc", "sdhci-tegra.0"), /* VDDIO_SDMMC1
							of SDMMC1 */
	REGULATOR_SUPPLY("pwrdet_sdmmc1", NULL),
	REGULATOR_SUPPLY("vddio_uart", NULL), /* VDDIO_UART of UART */
	REGULATOR_SUPPLY("pwrdet_uart", NULL),
	REGULATOR_SUPPLY("vddio_aw_ah397", NULL), /* VDD of AW-AH397 */
	REGULATOR_SUPPLY("avdd_usb_pll", "tegra-udc.0"), /* AVDD_USB_PLL
							of USB */
	REGULATOR_SUPPLY("avdd_usb_pll", "tegra-ehci.0"), /* AVDD_USB_PLL
							of USB */
	REGULATOR_SUPPLY("avdd_usb_pll", "tegra-ehci.1"), /* AVDD_USB_PLL
							of USB */
	REGULATOR_SUPPLY("pwrdet_audio", NULL),
};

/* vdd_sys_2v9 */
static struct regulator_consumer_supply palmas_smps9_supply[] = {
	REGULATOR_SUPPLY("vddio_sd_slot", "sdhci-tegra.3"), /* VDDF of eMMC */
	REGULATOR_SUPPLY("vddio_hv", "tegradc.0"), /* VDDIO_HV
						of HV (1.8V ~ 3.3V) */
#if 0 /* no driver is controlling this (always on) */
	REGULATOR_SUPPLY("vdd_2v9_hdmi_ls", "tegradc.0"), /* VCCA of HDMI output
								level-shifter */
#endif
};

/* vdd_hdmi_5v0 */
static struct regulator_consumer_supply palmas_smps10_supply[] = {
	/* 5V_SYS of HDMI level shifer, always on */
        REGULATOR_SUPPLY("vdd_hdmi_5v0", "tegradc.0"),
};

/* ldo1 - va_pllx */
static struct regulator_consumer_supply palmas_ldo1_supply[] = {
	REGULATOR_SUPPLY("avdd_plla_p_c", NULL), /* AVDD_PLLA_P_C of PLL */
	REGULATOR_SUPPLY("avdd_pllm", NULL), /* AVDD_PLLM of PLL */
	REGULATOR_SUPPLY("avdd_pllu", NULL), /* AVDD_PLLU of PLL */
	REGULATOR_SUPPLY("avdd_pllx", NULL), /* AVDD_PLLX of PLL */
	REGULATOR_SUPPLY("avdd_plle", NULL), /* AVDD_PLLE of PLL */
	REGULATOR_SUPPLY("vdd_ddr_hs", NULL), /* VDDIO_DDR_HS of DDR3 */
};

/* ldo2 - va_usb3_1v2 */
static struct regulator_consumer_supply palmas_ldo2_supply[] = {
	REGULATOR_SUPPLY("avddio_usb", "tegra-xhci"), /* AVDDIO_USB3 of USB */
	REGULATOR_SUPPLY("avdd_usb_pll", "tegra-xhci"), /* AVDDIO_USB3 of USB */
};

/* ldo3 - avdd_hdmi_pll */
static struct regulator_consumer_supply palmas_ldo3_supply[] = {
};

/* ldo4 - avdd_hsic_1v2 */
static struct regulator_consumer_supply palmas_ldo4_supply[] = {
	REGULATOR_SUPPLY("vddio_hsic", "tegra-ehci.1"), /* VDDIO_HSIC of HSIC */
};

/* ldo5 - vdd_fuse */
static struct regulator_consumer_supply palmas_ldo5_supply[] = {
	REGULATOR_SUPPLY("vpp_fuse", NULL), /* VPP_FUSE */
};

/* ldo6 is unused */

/* ldo7 is unused */

/* ldo8 - vd_ap_rtc */
static struct regulator_consumer_supply palmas_ldo8_supply[] = {
	REGULATOR_SUPPLY("vdd_rtc", NULL), /* VDD_RTC */
};

/* ldo9 is unused */

/* ldoln - va_hdmi */
static struct regulator_consumer_supply palmas_ldoln_supply[] = {
	REGULATOR_SUPPLY("avdd_hdmi", "tegradc.0"), /* AVDD_HDMI of HDMI */
};

#if 0
/* ldovrtc - vdd_pmu_vrtc
 * TODO: LDOVRTC is not implemented in the palmas regulator driver currently.
 *       We might want to review if we need to implement for this part later.
 */
static struct regulator_consumer_supply palmas_ldovrtc_supply[] = {
	REGULATOR_SUPPLY("vdd_pmu_vrtc", NULL),
};
#endif

/* ldovana is unused */

/* ldousb - avdd_usb */
static struct regulator_consumer_supply palmas_ldousb_supply[] = {
	REGULATOR_SUPPLY("avdd_usb", "tegra-udc.0"),
	REGULATOR_SUPPLY("avdd_usb", "tegra-ehci.0"),
	REGULATOR_SUPPLY("avdd_usb", "tegra-ehci.1"),
	REGULATOR_SUPPLY("avdd_usb", "tegra-ehci.2"),
	REGULATOR_SUPPLY("hvdd_usb", "tegra-xhci"),
};

/* NSLEEP is hooked up to CORE_PWR_REQ and
 * ENABLE1 is hooked up to CPU_PWR_REQ
 */
/* vdd_cpu - 1.0V nominal.
 * According to T114 data sheet, VDD_CPU should be in 0.882V ~ 1.377V
 * Total caps on CPU = 3x22uF + 6x4.7uF + 10x1uF = 104uF,
 * so we set TSTEP = 2.5mV/us for smps123
 */
PALMAS_REGS_PDATA(smps123, 882, 1377, NULL, 0, 0, 0, NORMAL,
		  0, PALMAS_EXT_CONTROL_ENABLE1, 0, 3, 0);
/* vdd_core - 1.1V nominal.
 * According to T114 data sheet, VDD_CORE should be in 0.873V ~ 1.2875V
 * Total caps on CORE = 2x22uF + 5x4.7uF + 1x1uF = 68.5uF,
 * so please set TSTEP = 2.5mV/us for smps45
 */
PALMAS_REGS_PDATA(smps45, 873,  1288, NULL, 0, 0, 0, NORMAL,
		  0, PALMAS_EXT_CONTROL_NSLEEP, 0, 3, 0);

/* vdd_3v3 - 3.3V,
 * XXX: vdd_3v3 supplies to INA3221, GEN1_I2C, Wifi/BT module,
 *      and LAN9730, but these device drivers are not implemented
 *      to configure the vdd currently.  So, set vdd_3v3 as "always on"
 *      temporarily.
 */
PALMAS_REGS_PDATA(smps6, 3300,  3300, NULL, 1, 0, 1, NORMAL,
		  0, 0, 0, 0, 0);
/* vdd_ddr - 1.35V, should be always on */
PALMAS_REGS_PDATA(smps7, 1350,  1350, NULL, 1, 0, 1, NORMAL,
		  0, 0, 0, 0, 0);
/* vdd_1v8 - 1.8V, should be always on */
PALMAS_REGS_PDATA(smps8, 1800,  1800, NULL, 1, 1, 1, NORMAL,
		  0, 0, 0, 0, 0);
/* vdd_sys_2v9 - always on, emmc power */
PALMAS_REGS_PDATA(smps9, 2900,  2900, NULL, 1, 0, 1, NORMAL,
		  0, 0, 0, 0, 0);
/* vdd_hdmi_5v0 - always on, hdmi level shifter */
PALMAS_REGS_PDATA(smps10, 5000, 5000, NULL, 1, 0, 1, 0, 0, 0, 0, 0, 0);

/* va_pllx - boot on? - 1.05V, should be always on */
PALMAS_REGS_PDATA(ldo1, 1050,  1050, palmas_rails(smps7), 1, 0, 1, 0,
		  0, 0, 0, 0, 0);
/* va_usb3_1v2 - 1.2V */
PALMAS_REGS_PDATA(ldo2, 1200,  1200, palmas_rails(smps7), 0, 0, 1, 0,
		  0, 0, 0, 0, 0);
/* avdd_hdmi_pll - 1.20V */
PALMAS_REGS_PDATA(ldo3, 1200,  1200, palmas_rails(smps8), 0, 0, 1, 0,
		  0, 0, 0, 0, 0);
/* avdd_hsic_1v2 - 1.2V */
PALMAS_REGS_PDATA(ldo4, 1200,  1200, palmas_rails(smps8), 0, 0, 1, 0,
		  0, 0, 0, 0, 0);
/* vd_fuse - 1.8V, should we move this to a fixed regulator? */
PALMAS_REGS_PDATA(ldo5, 1800,  1800, NULL, 0, 0, 1, 0,
		  0, 0, 0, 0, 0);
/* vdd_rtc should follow vdd_core when vdd_core is on (tracking mode)
 * when vdd_core is off, vdd_rtc can be set as 0.9V */
PALMAS_REGS_PDATA(ldo8, 900,  900, NULL, 1, 1, 1, 0,
		  0, 0, 0, 0, 0);
/* avdd_hdmi - 3.3V */
PALMAS_REGS_PDATA(ldoln, 3300, 3300, NULL, 0, 0, 1, 0,
		  0, 0, 0, 0, 0);
/* avdd_usb & hvdd_usb - 3.3V */
PALMAS_REGS_PDATA(ldousb, 3300,  3300, NULL, 0, 0, 1, 0,
		  0, 0, 0, 0, 0);

#define PALMAS_REG_PDATA(_sname) (&reg_idata_##_sname)

static struct regulator_init_data *molly_reg_data[PALMAS_NUM_REGS] = {
	NULL,
	PALMAS_REG_PDATA(smps123),
	NULL,
	PALMAS_REG_PDATA(smps45),
	NULL,
	PALMAS_REG_PDATA(smps6),
	PALMAS_REG_PDATA(smps7),
	PALMAS_REG_PDATA(smps8),
	PALMAS_REG_PDATA(smps9),
	PALMAS_REG_PDATA(smps10),
	PALMAS_REG_PDATA(ldo1),
	PALMAS_REG_PDATA(ldo2),
	PALMAS_REG_PDATA(ldo3),
	PALMAS_REG_PDATA(ldo4),
	PALMAS_REG_PDATA(ldo5),
	NULL,
	NULL,
	PALMAS_REG_PDATA(ldo8),
	NULL,
	PALMAS_REG_PDATA(ldoln),
	PALMAS_REG_PDATA(ldousb),
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

#define PALMAS_REG_INIT_DATA(_sname) (&reg_init_data_##_sname)
static struct palmas_reg_init *molly_reg_init[PALMAS_NUM_REGS] = {
	NULL, /* SMPS12 */
	PALMAS_REG_INIT_DATA(smps123), /* SMPS123 */
	NULL, /* SMPS3 */
	PALMAS_REG_INIT_DATA(smps45), /* SMPS45 */
	NULL, /* SMPS457 */
	PALMAS_REG_INIT_DATA(smps6), /* SMPS6 */
	PALMAS_REG_INIT_DATA(smps7), /* SMPS7 */
	PALMAS_REG_INIT_DATA(smps8), /* SMPS8 */
	PALMAS_REG_INIT_DATA(smps9), /* SMPS9 */
	PALMAS_REG_INIT_DATA(smps10), /* SMPS10 */
	PALMAS_REG_INIT_DATA(ldo1),
	PALMAS_REG_INIT_DATA(ldo2),
	PALMAS_REG_INIT_DATA(ldo3),
	PALMAS_REG_INIT_DATA(ldo4),
	PALMAS_REG_INIT_DATA(ldo5),
	NULL,
	NULL,
	PALMAS_REG_INIT_DATA(ldo8),
	NULL,
	PALMAS_REG_INIT_DATA(ldoln),
	PALMAS_REG_INIT_DATA(ldousb),
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

/* Note: this can't be __initdata because palmas driver
 * keeps a reference to it after init.
 */
static struct palmas_pmic_platform_data pmic_platform = {
	.enable_ldo8_tracking = true,
	.disabe_ldo8_tracking_suspend = true,
	.disable_smps10_boost_suspend = false,
};

static struct palmas_pinctrl_config __initdata palmas_pincfg[] = {
	PALMAS_PINMUX(POWERGOOD, POWERGOOD, DEFAULT, DEFAULT),
	PALMAS_PINMUX(VAC, VAC, DEFAULT, DEFAULT),
	PALMAS_PINMUX(GPIO0, GPIO, DEFAULT, DEFAULT),
	PALMAS_PINMUX(GPIO1, GPIO, DEFAULT, DEFAULT),
	PALMAS_PINMUX(GPIO2, GPIO, DEFAULT, DEFAULT),
	PALMAS_PINMUX(GPIO3, GPIO, DEFAULT, DEFAULT),
	PALMAS_PINMUX(GPIO4, GPIO, DEFAULT, DEFAULT),
	PALMAS_PINMUX(GPIO5, GPIO, DEFAULT, DEFAULT),
	PALMAS_PINMUX(GPIO6, GPIO, DEFAULT, DEFAULT),
	PALMAS_PINMUX(GPIO7, GPIO, DEFAULT, DEFAULT),
};

static struct palmas_pinctrl_platform_data __initdata palmas_pinctrl_pdata = {
	.pincfg = palmas_pincfg,
	.num_pinctrl = ARRAY_SIZE(palmas_pincfg),
	.dvfs1_enable = true,
	.dvfs2_enable = false,
};

struct palmas_clk32k_init_data __initdata palmas_clk32k_pdata[] = {
	{
		.clk32k_id = PALMAS_CLOCK32KG,
		.enable = true,
	},
	{
		.clk32k_id = PALMAS_CLOCK32KG_AUDIO,
		.enable = true,
	},
};

static struct palmas_platform_data __initdata palmas_pdata = {
	.gpio_base = PALMAS_TEGRA_GPIO_BASE,
	.irq_base = PALMAS_TEGRA_IRQ_BASE,
	.pmic_pdata = &pmic_platform,
	.clk32k_init_data = palmas_clk32k_pdata,
	.clk32k_init_data_size = ARRAY_SIZE(palmas_clk32k_pdata),
	/* On Molly we don't use the PMIC's power off function.  Instead
	 * we use a Molly specific power off function that reboots into
	 * the bootloader for shutdown.  This allows the bootloader to
	 * clear the recovery retry bit if it is set.
	 */
	.use_power_off = false,
	.pinctrl_pdata = &palmas_pinctrl_pdata,
};

static struct i2c_board_info __initdata palma_device[] = {
	{
		I2C_BOARD_INFO("tps65913", 0x58),
		.irq		= INT_EXTERNAL_PMU,
		.platform_data	= &palmas_pdata,
	},
};

/* EN_USB3_VBUS From TEGRA GPIO PN4 */
static struct regulator_consumer_supply fixed_reg_usb3_vbus_supply[] = {
	REGULATOR_SUPPLY("usb_vbus", "tegra-xhci"), /* VBUS_USB_SW */
};

/* no gpio enable required */
static struct regulator_consumer_supply fixed_reg_avdd_hdmi_pll_supply[] = {
	REGULATOR_SUPPLY("avdd_hdmi_pll", "tegradc.0"), /* AVDD_HDMI_PLL
							of HDMI */
};

/* Macro for defining fixed regulator sub device data */
#define FIXED_SUPPLY(_name) "fixed_reg_"#_name
#define FIXED_REG(_id, _var, _name, _in_supply, _always_on, _boot_on,	\
	_gpio_nr, _open_drain, _active_high, _boot_state, _millivolts)	\
	static struct regulator_init_data ri_data_##_var =		\
	{								\
		.supply_regulator = _in_supply,				\
		.num_consumer_supplies =				\
			ARRAY_SIZE(fixed_reg_##_name##_supply),		\
		.consumer_supplies = fixed_reg_##_name##_supply,	\
		.constraints = {					\
			.valid_modes_mask = (REGULATOR_MODE_NORMAL |	\
					REGULATOR_MODE_STANDBY),	\
			.valid_ops_mask = (REGULATOR_CHANGE_MODE |	\
					REGULATOR_CHANGE_STATUS |	\
					REGULATOR_CHANGE_VOLTAGE),	\
			.always_on = _always_on,			\
			.boot_on = _boot_on,				\
		},							\
	};								\
	static struct fixed_voltage_config fixed_reg_##_var##_pdata =	\
	{								\
		.supply_name = FIXED_SUPPLY(_name),			\
		.microvolts = _millivolts * 1000,			\
		.gpio = _gpio_nr,					\
		.gpio_is_open_drain = _open_drain,			\
		.enable_high = _active_high,				\
		.enabled_at_boot = _boot_state,				\
		.init_data = &ri_data_##_var,				\
	};								\
	static struct platform_device fixed_reg_##_var##_dev = {	\
		.name = "reg-fixed-voltage",				\
		.id = _id,						\
		.dev = {						\
			.platform_data = &fixed_reg_##_var##_pdata,	\
		},							\
	}

FIXED_REG(6,	usb3_vbus,	usb3_vbus,
	  NULL,	0,	0,
	  TEGRA_GPIO_PN4,	false,	false,	0,	5000);

FIXED_REG(10,	avdd_hdmi_pll,	avdd_hdmi_pll,
	  palmas_rails(ldo3),	0,	0,
	  -1,	false,	true,	1,	1200);
/*
 * Creating the fixed regulator device tables
 */

#define ADD_FIXED_REG(_name)    (&fixed_reg_##_name##_dev)

static struct platform_device *fixed_reg_devs_molly[] = {
	ADD_FIXED_REG(usb3_vbus),
	ADD_FIXED_REG(avdd_hdmi_pll),
};

static struct wake_lock molly_evt2_wakelock;

int __init molly_palmas_regulator_init(void)
{
	void __iomem *pmc = IO_ADDRESS(TEGRA_PMC_BASE);
	u32 pmc_ctrl;
	int i;

	/* TPS65913: Normal state of INT request line is LOW.
	 * configure the power management controller to trigger PMU
	 * interrupts when HIGH.
	 */
	pmc_ctrl = readl(pmc + PMC_CTRL);
	writel(pmc_ctrl | PMC_CTRL_INTR_LOW, pmc + PMC_CTRL);

	switch (molly_hw_rev) {
	case MOLLY_REV_PROTO1:
	case MOLLY_REV_PROTO2:
	case MOLLY_REV_EVT1:
	case MOLLY_REV_DVT1:
		/* smps10 is not enabled for Molly prototype, EVT1 and DVT1.*/
		pr_info("%s: smps10 is not enabled\n", __func__);
		molly_reg_data[PALMAS_REG_SMPS10] = NULL;
		molly_reg_init[PALMAS_REG_SMPS10] = NULL;
		break;
	case MOLLY_REV_EVT2:
		/* smps10 is supplied by smps6 for EVT2, but for newer
		 * versions is supplied from 5V input
		 */
		reg_idata_smps10.supply_regulator = palmas_rails(smps6);
		break;
	}

	for (i = 0; i < PALMAS_NUM_REGS ; i++) {
		pmic_platform.reg_data[i] = molly_reg_data[i];
		pmic_platform.reg_init[i] = molly_reg_init[i];
	}

	i2c_register_board_info(4, palma_device,
			ARRAY_SIZE(palma_device));

	/* The regulator details have complete constraints */
	regulator_has_full_constraints();

	return 0;
}

static int ac_online(void)
{
	return 1;
}

static struct resource molly_pda_resources[] = {
	[0] = {
		.name	= "ac",
	},
};

static struct pda_power_pdata molly_pda_data = {
	.is_ac_online	= ac_online,
};

static struct platform_device molly_pda_power_device = {
	.name		= "pda-power",
	.id		= -1,
	.resource	= molly_pda_resources,
	.num_resources	= ARRAY_SIZE(molly_pda_resources),
	.dev	= {
		.platform_data	= &molly_pda_data,
	},
};

static struct tegra_suspend_platform_data molly_suspend_data = {
	.cpu_timer	= 500,
	.cpu_off_timer	= 300,
	.suspend_mode	= TEGRA_SUSPEND_LP0,
	.core_timer	= 0x157e,
	.core_off_timer = 2000,
	.corereq_high	= true,
	.sysclkreq_high	= true,
	.cpu_lp2_min_residency = 1000,
	.min_residency_crail = 20000,
#ifdef CONFIG_TEGRA_LP1_LOW_COREVOLTAGE
	.lp1_lowvolt_support = false,
	.i2c_base_addr = 0,
	.pmuslave_addr = 0,
	.core_reg_addr = 0,
	.lp1_core_volt_low_cold = 0,
	.lp1_core_volt_low = 0,
	.lp1_core_volt_high = 0,
#endif
};
#ifdef CONFIG_ARCH_TEGRA_HAS_CL_DVFS
/* board parameters for cpu dfll */
static struct tegra_cl_dvfs_cfg_param molly_cl_dvfs_param = {
	.sample_rate = 11500,

	.force_mode = TEGRA_CL_DVFS_FORCE_FIXED,
	.cf = 10,
	.ci = 0,
	.cg = 2,

	.droop_cut_value = 0xF,
	.droop_restore_ramp = 0x0,
	.scale_out_ramp = 0x0,
};
#endif

/* palmas: fixed 10mV steps from 600mV to 1400mV, with offset 0x10 */
#define PMU_CPU_VDD_MAP_SIZE ((1400000 - 600000) / 10000 + 1)
static struct voltage_reg_map pmu_cpu_vdd_map[PMU_CPU_VDD_MAP_SIZE];
static inline void fill_reg_map(void)
{
	int i;
	for (i = 0; i < PMU_CPU_VDD_MAP_SIZE; i++) {
		pmu_cpu_vdd_map[i].reg_value = i + 0x10;
		pmu_cpu_vdd_map[i].reg_uV = 600000 + 10000 * i;
	}
}

#ifdef CONFIG_ARCH_TEGRA_HAS_CL_DVFS
static struct tegra_cl_dvfs_platform_data molly_cl_dvfs_data = {
	.dfll_clk_name = "dfll_cpu",
	.pmu_if = TEGRA_CL_DVFS_PMU_I2C,
	.u.pmu_i2c = {
		.fs_rate = 400000,
		.slave_addr = 0xb0,
		.reg = 0x23,
	},
	.vdd_map = pmu_cpu_vdd_map,
	.vdd_map_size = PMU_CPU_VDD_MAP_SIZE,

	.cfg_param = &molly_cl_dvfs_param,
};

static int __init molly_cl_dvfs_init(void)
{
	fill_reg_map();
	if (tegra_revision < TEGRA_REVISION_A02)
		molly_cl_dvfs_data.flags = TEGRA_CL_DVFS_FLAGS_I2C_WAIT_QUIET;
	tegra_cl_dvfs_device.dev.platform_data = &molly_cl_dvfs_data;
	platform_device_register(&tegra_cl_dvfs_device);

	return 0;
}
#endif

static int __init molly_fixed_regulator_init(void)
{
	if (!machine_is_molly())
		return 0;

	return platform_add_devices(fixed_reg_devs_molly,
				    ARRAY_SIZE(fixed_reg_devs_molly));
}
subsys_initcall_sync(molly_fixed_regulator_init);

int __init molly_regulator_init(void)
{
#ifdef CONFIG_ARCH_TEGRA_HAS_CL_DVFS
	molly_cl_dvfs_init();
#endif
	molly_palmas_regulator_init();

	platform_device_register(&molly_pda_power_device);
	return 0;
}

int __init molly_suspend_init(void)
{
	tegra_init_suspend(&molly_suspend_data);
	return 0;
}

static __init int molly_suspend_late_init(void)
{
	if (molly_hw_rev == MOLLY_REV_EVT2) {
		/* This is a workaround for a hardware bug on Molly EVT2.
		 * On Molly EVT2, the 10K ohm pull up resistor on
		 * ENET_RESET_N_3V3 pin is removed accidentally, which causes
		 * SMSC LAN9730 would be reset every time when Molly EVT2
		 * enters LP0.
		 */
		wake_lock_init(&molly_evt2_wakelock, WAKE_LOCK_SUSPEND,
			"molly_evt2_not_support_lp0");
		wake_lock(&molly_evt2_wakelock);
		pr_info("%s: WAR: acquire a wakelock to prevent enter suspend\n",
			__func__);
	}
	return 0;
}
late_initcall(molly_suspend_late_init);

int __init molly_edp_init(void)
{
	unsigned int regulator_mA;

	regulator_mA = get_maximum_cpu_current_supported();
	if (!regulator_mA)
		regulator_mA = 8000;

	pr_info("%s: CPU regulator %d mA\n", __func__, regulator_mA);
	tegra_init_cpu_edp_limits(regulator_mA);

	regulator_mA = get_maximum_core_current_supported();
	if (!regulator_mA)
		regulator_mA = 3500;

	pr_info("%s: core regulator %d mA\n", __func__, regulator_mA);
	tegra_init_core_edp_limits(regulator_mA);

	return 0;
}

static struct thermal_zone_params molly_soctherm_therm_cpu_tzp = {
	.governor_name = "pid_thermal_gov",
};

static struct tegra_tsensor_pmu_data tpdata_palmas = {
	.reset_tegra = 1,
	.pmu_16bit_ops = 0,
	.controller_type = 0,
	.pmu_i2c_addr = 0x58,
	.i2c_controller_id = 4,
	.poweroff_reg_addr = 0xa0,
	.poweroff_reg_data = 0x0,
};

static struct soctherm_platform_data molly_soctherm_data = {
	.therm = {
		[THERM_CPU] = {
			.zone_enable = true,
			.passive_delay = 1000,
			.hotspot_offset = 6000,
			.num_trips = 3,
			.trips = {
				{
					.cdev_type = "tegra-balanced",
					.trip_temp = 90000,
					.trip_type = THERMAL_TRIP_PASSIVE,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "tegra-heavy",
					.trip_temp = 100000,
					.trip_type = THERMAL_TRIP_HOT,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "tegra-shutdown",
					.trip_temp = 102000,
					.trip_type = THERMAL_TRIP_CRITICAL,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
			},
			.tzp = &molly_soctherm_therm_cpu_tzp,
		},
		[THERM_GPU] = {
			.zone_enable = true,
			.passive_delay = 1000,
			.hotspot_offset = 6000,
			.num_trips = 3,
			.trips = {
				{
					.cdev_type = "tegra-balanced",
					.trip_temp = 90000,
					.trip_type = THERMAL_TRIP_PASSIVE,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "tegra-heavy",
					.trip_temp = 100000,
					.trip_type = THERMAL_TRIP_HOT,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
				{
					.cdev_type = "tegra-shutdown",
					.trip_temp = 102000,
					.trip_type = THERMAL_TRIP_CRITICAL,
					.upper = THERMAL_NO_LIMIT,
					.lower = THERMAL_NO_LIMIT,
				},
			},
			.tzp = &molly_soctherm_therm_cpu_tzp,
		},
		[THERM_PLL] = {
			.zone_enable = true,
		},
	},
	.throttle = {
		[THROTTLE_HEAVY] = {
			.priority = 100,
			.devs = {
				[THROTTLE_DEV_CPU] = {
					.enable = true,
					.depth = 80,
				},
			},
		},
#if 0
		/* handler for POWERGOOD signal from TPS65913
		 * if we ever hook it up in thermal sensor
		 * mode.  right now we use external thermal
		 * sensor chip TMP451.
		 */
		[THROTTLE_OC2] = {
			.throt_mode = BRIEF,
			.polarity = 0,
			.intr = true,
			.devs = {
				[THROTTLE_DEV_CPU] = {
					.enable = true,
					.depth = 50,
				},
				[THROTTLE_DEV_GPU] = {
					.enable = true,
					.depth = 50,
				},
			},
		},
#endif
		/* handler for CRITICAL signal from INA3221. */
		[THROTTLE_OC4] = {
			.throt_mode = BRIEF,
			.polarity = 1, /* 1 = active low, 0 = active high */
			.intr = true,
			.devs = {
				[THROTTLE_DEV_CPU] = {
					.enable = true,
					.depth = 50,
				},
				[THROTTLE_DEV_GPU] = {
					.enable = true,
					.depth = 50,
				},
			},
		},
	},
	.tshut_pmu_trip_data = &tpdata_palmas,
};

int __init molly_soctherm_init(void)
{
	tegra_platform_edp_init(molly_soctherm_data.therm[THERM_CPU].trips,
			&molly_soctherm_data.therm[THERM_CPU].num_trips,
			8000); /* edp temperature margin */
	tegra_add_tj_trips(molly_soctherm_data.therm[THERM_CPU].trips,
			&molly_soctherm_data.therm[THERM_CPU].num_trips);
	tegra_add_vc_trips(molly_soctherm_data.therm[THERM_CPU].trips,
			&molly_soctherm_data.therm[THERM_CPU].num_trips);

	return tegra11_soctherm_init(&molly_soctherm_data);
}

#endif
