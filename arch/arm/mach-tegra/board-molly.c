/*
 * arch/arm/mach-tegra/board-molly.c
 *
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.
 * Copyright (c) 2013, Google, Inc.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/i2c.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/i2c-tegra.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/platform_data/tegra_xusb.h>
#include <linux/spi/spi.h>
#include <linux/spi/rm31080a_ts.h>
#include <linux/tegra_uart.h>
#include <linux/memblock.h>
#include <linux/spi-tegra.h>
#include <linux/rfkill-gpio.h>
#include <linux/skbuff.h>
#include <linux/regulator/consumer.h>
#include <linux/smb349-charger.h>
#include <linux/max17048_battery.h>
#include <linux/of_platform.h>
#include <linux/edp.h>

#include <asm/hardware/gic.h>

#include <mach/clk.h>
#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/pinmux.h>
#include <mach/pinmux-tegra30.h>
#include <mach/iomap.h>
#include <mach/io.h>
#include <mach/io_dpd.h>
#include <mach/i2s.h>
#include <mach/tegra_asoc_pdata.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/usb_phy.h>
#include <mach/gpio-tegra.h>
#include <mach/tegra_fiq_debugger.h>
#include <linux/aah_io.h>
#include <linux/athome_radio.h>
#include <mach/hardware.h>

#include "board-touch-raydium.h"
#include "board.h"
#include "board-common.h"
#include "clock.h"
#include "board-molly.h"
#include "devices.h"
#include "gpio-names.h"
#include "fuse.h"
#include "pm.h"
#include "pm-irq.h"
#include "common.h"
#include "tegra-board-id.h"

#if defined(CONFIG_BT_BLUESLEEP) || defined(CONFIG_BT_BLUESLEEP_MODULE)
static struct rfkill_gpio_platform_data molly_bt_rfkill_pdata = {
		.name           = "bt_rfkill",
		.shutdown_gpio  = TEGRA_GPIO_PQ7,
		.reset_gpio	= TEGRA_GPIO_PQ6,
		.type           = RFKILL_TYPE_BLUETOOTH,
};

static struct platform_device molly_bt_rfkill_device = {
	.name = "rfkill_gpio",
	.id             = -1,
	.dev = {
		.platform_data = &molly_bt_rfkill_pdata,
	},
};

static struct resource molly_bluesleep_resources[] = {
	[0] = {
		.name = "gpio_host_wake",
			.start  = TEGRA_GPIO_PU6,
			.end    = TEGRA_GPIO_PU6,
			.flags  = IORESOURCE_IO,
	},
	[1] = {
		.name = "gpio_ext_wake",
			.start  = TEGRA_GPIO_PEE1,
			.end    = TEGRA_GPIO_PEE1,
			.flags  = IORESOURCE_IO,
	},
	[2] = {
		.name = "host_wake",
			.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
};

static struct platform_device molly_bluesleep_device = {
	.name           = "bluesleep",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(molly_bluesleep_resources),
	.resource       = molly_bluesleep_resources,
};

static noinline void __init molly_setup_bt_rfkill(void)
{
	platform_device_register(&molly_bt_rfkill_device);
}

static noinline void __init molly_setup_bluesleep(void)
{
	molly_bluesleep_resources[2].start =
		molly_bluesleep_resources[2].end =
			gpio_to_irq(TEGRA_GPIO_PU6);
	platform_device_register(&molly_bluesleep_device);
	return;
}
#elif defined CONFIG_BLUEDROID_PM
static struct resource molly_bluedroid_pm_resources[] = {
	[0] = {
		.name   = "shutdown_gpio",
		.start  = TEGRA_GPIO_PQ7,
		.end    = TEGRA_GPIO_PQ7,
		.flags  = IORESOURCE_IO,
	},
	[1] = {
		.name = "host_wake",
		.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
	[2] = {
		.name = "gpio_ext_wake",
		.start  = TEGRA_GPIO_PEE1,
		.end    = TEGRA_GPIO_PEE1,
		.flags  = IORESOURCE_IO,
	},
	[3] = {
		.name = "gpio_host_wake",
		.start  = TEGRA_GPIO_PU6,
		.end    = TEGRA_GPIO_PU6,
		.flags  = IORESOURCE_IO,
	},
	[4] = {
		.name = "reset_gpio",
		.start  = TEGRA_GPIO_PQ6,
		.end    = TEGRA_GPIO_PQ6,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device molly_bluedroid_pm_device = {
	.name = "bluedroid_pm",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(molly_bluedroid_pm_resources),
	.resource       = molly_bluedroid_pm_resources,
};

static noinline void __init molly_setup_bluedroid_pm(void)
{
	molly_bluedroid_pm_resources[1].start =
		molly_bluedroid_pm_resources[1].end =
				gpio_to_irq(TEGRA_GPIO_PU6);
	platform_device_register(&molly_bluedroid_pm_device);
}
#endif

static __initdata struct tegra_clk_init_table molly_clk_init_table[] = {
	/* name		parent		rate		enabled */
	{ "pll_m",	NULL,		0,		false},
	{ "hda",	"pll_p",	108000000,	false},
	{ "hda2codec_2x", "pll_p",	48000000,	false},
	{ "pwm",	"pll_p",	3187500,	false},
	{ "blink",	"clk_32k",	32768,		true},
	{ "i2s1",	"pll_a_out0",	0,		false},
	{ "i2s3",	"pll_a_out0",	0,		false},
	{ "i2s4",	"pll_a_out0",	0,		false},
	{ "spdif_out",	"pll_a_out0",	0,		false},
	{ "d_audio",	"clk_m",	12000000,	false},
	{ "dam0",	"clk_m",	12000000,	false},
	{ "dam1",	"clk_m",	12000000,	false},
	{ "dam2",	"clk_m",	12000000,	false},
	{ "audio1",	"i2s1_sync",	0,		false},
	{ "audio3",	"i2s3_sync",	0,		false},
	/* Setting vi_sensor-clk to true for validation purpose, will imapact
	 * power, later set to be false.*/
	{ "vi_sensor",	"pll_p",	150000000,	false},
	{ "cilab",	"pll_p",	150000000,	false},
	{ "cilcd",	"pll_p",	150000000,	false},
	{ "cile",	"pll_p",	150000000,	false},
	{ "i2c1",	"pll_p",	3200000,	false},
	{ "i2c2",	"pll_p",	3200000,	false},
	{ "i2c3",	"pll_p",	3200000,	false},
	{ "i2c4",	"pll_p",	3200000,	false},
	{ "i2c5",	"pll_p",	3200000,	false},
	{ NULL,		NULL,		0,		0},
};

static struct tegra_i2c_platform_data molly_i2c1_platform_data = {
	.adapter_nr	= 0,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.scl_gpio		= {TEGRA_GPIO_I2C1_SCL, 0},
	.sda_gpio		= {TEGRA_GPIO_I2C1_SDA, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data molly_i2c2_platform_data = {
	.adapter_nr	= 1,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.is_clkon_always = true,
	.scl_gpio		= {TEGRA_GPIO_I2C2_SCL, 0},
	.sda_gpio		= {TEGRA_GPIO_I2C2_SDA, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data molly_i2c3_platform_data = {
	.adapter_nr	= 2,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.scl_gpio		= {TEGRA_GPIO_I2C3_SCL, 0},
	.sda_gpio		= {TEGRA_GPIO_I2C3_SDA, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data molly_i2c4_platform_data = {
	.adapter_nr	= 3,
	.bus_count	= 1,
	.bus_clk_rate	= { 10000, 0 },
	.scl_gpio		= {TEGRA_GPIO_I2C4_SCL, 0},
	.sda_gpio		= {TEGRA_GPIO_I2C4_SDA, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data molly_i2c5_platform_data = {
	.adapter_nr	= 4,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.scl_gpio		= {TEGRA_GPIO_I2C5_SCL, 0},
	.sda_gpio		= {TEGRA_GPIO_I2C5_SDA, 0},
	.arb_recovery = arb_lost_recovery,
};

/******************************************************************************
 *                                                                            *
 *           aah_io driver platform data                                      *
 *                                                                            *
 ******************************************************************************/
static struct aah_io_platform_data aah_io_data = {
#if 0
	.key_gpio          = TEGRA_GPIO_PQ5, /* molly's UI_SWITCH, KB_COL5/GPIO_PQ5 */
#else
	.key_gpio          = TEGRA_GPIO_PR2, /* dalmore's volume+ button for now */
#endif
	.key_code          = KEY_MUTE,/* TBD, easiest to test as mute for now */
};

static struct i2c_board_info __initdata i2c_bus2[] = {
	{
		I2C_BOARD_INFO("aah-io", 0x32),
		.platform_data = &aah_io_data,
	},
};

static void molly_i2c_init(void)
{
	struct board_info board_info;

	/* Tegra4 has five i2c controllers:
	 * I2C_1 is called GEN1_I2C in pinmux/schematics
	 * I2C_2 is called GEN2_I2C in pinmux/schematics
	 * I2C_3 is called CAM_I2C in pinmux/schematics
	 * I2C_4 is called DDC_I2C in pinmux/schematics
	 * I2C_5/PMU is called PWR_I2C in pinmux/schematics
	 *
	 * I2C1/GEN1 is for INA3221 current and bus voltage monitor
	 * I2C2/GEN2 is for LED
	 * I2C3/CAM is for TMP451
	 * I2C5/PWR is for PMIC TPS65913B2B5
	 */

	tegra_get_board_info(&board_info);
	tegra11_i2c_device1.dev.platform_data = &molly_i2c1_platform_data;
	tegra11_i2c_device2.dev.platform_data = &molly_i2c2_platform_data;
	tegra11_i2c_device3.dev.platform_data = &molly_i2c3_platform_data;
	tegra11_i2c_device4.dev.platform_data = &molly_i2c4_platform_data;
	tegra11_i2c_device5.dev.platform_data = &molly_i2c5_platform_data;

	platform_device_register(&tegra11_i2c_device5);
	platform_device_register(&tegra11_i2c_device4);
	platform_device_register(&tegra11_i2c_device3);
	platform_device_register(&tegra11_i2c_device2);
	platform_device_register(&tegra11_i2c_device1);

	i2c_register_board_info(2, i2c_bus2, ARRAY_SIZE(i2c_bus2));
}

static struct platform_device *molly_uart_devices[] __initdata = {
	&tegra_uarta_device,
	&tegra_uartb_device,
	&tegra_uartc_device,
	&tegra_uartd_device,
};
static struct uart_clk_parent uart_parent_clk[] = {
	[0] = {.name = "clk_m"},
	[1] = {.name = "pll_p"},
#ifndef CONFIG_TEGRA_PLLM_RESTRICTED
	[2] = {.name = "pll_m"},
#endif
};

static struct tegra_uart_platform_data molly_uart_pdata;
static struct tegra_uart_platform_data molly_loopback_uart_pdata;

static void __init uart_debug_init(void)
{
	int debug_port_id;

	debug_port_id = uart_console_debug_init(3);
	if (debug_port_id < 0)
		return;

	molly_uart_devices[debug_port_id] = uart_console_debug_device;
}

static void __init molly_uart_init(void)
{
	struct clk *c;
	int i;

	for (i = 0; i < ARRAY_SIZE(uart_parent_clk); ++i) {
		c = tegra_get_clock_by_name(uart_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",
						uart_parent_clk[i].name);
			continue;
		}
		uart_parent_clk[i].parent_clk = c;
		uart_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}
	molly_uart_pdata.parent_clk_list = uart_parent_clk;
	molly_uart_pdata.parent_clk_count = ARRAY_SIZE(uart_parent_clk);
	molly_loopback_uart_pdata.parent_clk_list = uart_parent_clk;
	molly_loopback_uart_pdata.parent_clk_count =
						ARRAY_SIZE(uart_parent_clk);
	molly_loopback_uart_pdata.is_loopback = true;
	tegra_uarta_device.dev.platform_data = &molly_uart_pdata;
	tegra_uartb_device.dev.platform_data = &molly_uart_pdata;
	tegra_uartc_device.dev.platform_data = &molly_uart_pdata;
	tegra_uartd_device.dev.platform_data = &molly_uart_pdata;

	/* Register low speed only if it is selected */
	if (!is_tegra_debug_uartport_hs())
		uart_debug_init();

	platform_add_devices(molly_uart_devices,
				ARRAY_SIZE(molly_uart_devices));
}

static struct resource tegra_rtc_resources[] = {
	[0] = {
		.start = TEGRA_RTC_BASE,
		.end = TEGRA_RTC_BASE + TEGRA_RTC_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = INT_RTC,
		.end = INT_RTC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device tegra_rtc_device = {
	.name = "tegra_rtc",
	.id   = -1,
	.resource = tegra_rtc_resources,
	.num_resources = ARRAY_SIZE(tegra_rtc_resources),
};

static struct tegra_asoc_platform_data molly_audio_pdata = {
	.gpio_spkr_en		= TEGRA_GPIO_SPKR_EN,
	.gpio_hp_det		= TEGRA_GPIO_HP_DET,
	.gpio_hp_mute		= -1,
	.gpio_int_mic_en	= TEGRA_GPIO_INT_MIC_EN,
	.gpio_ext_mic_en	= TEGRA_GPIO_EXT_MIC_EN,
	.gpio_ldo1_en		= TEGRA_GPIO_LDO1_EN,
	.gpio_codec1 = TEGRA_GPIO_CODEC1_EN,
	.gpio_codec2 = TEGRA_GPIO_CODEC2_EN,
	.gpio_codec3 = TEGRA_GPIO_CODEC3_EN,
	.i2s_param[HIFI_CODEC]	= {
		.audio_port_id	= 1,
		.is_i2s_master	= 1,
		.i2s_mode	= TEGRA_DAIFMT_I2S,
	},
	.i2s_param[BT_SCO]	= {
		.audio_port_id	= 3,
		.is_i2s_master	= 1,
		.i2s_mode	= TEGRA_DAIFMT_DSP_A,
	},
};

static struct platform_device molly_audio_device = {
	.name	= "tegra-snd-rt5640",
	.id	= 0,
	.dev	= {
		.platform_data = &molly_audio_pdata,
	},
};

static struct platform_device *molly_devices[] __initdata = {
	&tegra_pmu_device,
	&tegra_rtc_device,
	&tegra_udc_device,
#if defined(CONFIG_TEGRA_IOVMM_SMMU) || defined(CONFIG_TEGRA_IOMMU_SMMU)
	&tegra_smmu_device,
#endif
#if defined(CONFIG_TEGRA_AVP)
	&tegra_avp_device,
#endif
#if defined(CONFIG_CRYPTO_DEV_TEGRA_SE)
	&tegra11_se_device,
#endif
	&tegra_ahub_device,
	&tegra_dam_device0,
	&tegra_dam_device1,
	&tegra_dam_device2,
	&tegra_i2s_device1,
	&tegra_i2s_device3,
	&tegra_i2s_device4,
	&tegra_spdif_device,
	&spdif_dit_device,
	&bluetooth_dit_device,
	&tegra_pcm_device,
	&molly_audio_device,
	&tegra_hda_device,
#if defined(CONFIG_TEGRA_CEC_SUPPORT)
	&tegra_cec_device,
#endif
#if defined(CONFIG_CRYPTO_DEV_TEGRA_AES)
	&tegra_aes_device,
#endif
};

#ifdef CONFIG_USB_SUPPORT
static struct tegra_usb_platform_data tegra_udc_pdata = {
	.port_otg = true,
	.has_hostpc = true,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.unaligned_dma_buf_supported = false,
	.op_mode = TEGRA_USB_OPMODE_DEVICE,
	.u_data.dev = {
		.vbus_pmu_irq = 0,
		.vbus_gpio = -1,
		.charging_supported = false,
		.remote_wakeup_supported = false,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 8,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
	},
};

static struct tegra_usb_platform_data tegra_ehci1_utmi_pdata = {
	.port_otg = true,
	.has_hostpc = true,
	.unaligned_dma_buf_supported = false,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.hot_plug = false,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = true,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 15,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
		.vbus_oc_map = 0x4,
	},
};

static struct tegra_usb_platform_data tegra_ehci3_utmi_pdata = {
	.port_otg = false,
	.has_hostpc = true,
	.unaligned_dma_buf_supported = false,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.hot_plug = false,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = true,
	},
	.u_cfg.utmi = {
	.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 8,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
		.vbus_oc_map = 0x5,
	},
};

static struct tegra_usb_otg_data tegra_otg_pdata = {
	.ehci_device = &tegra_ehci1_device,
	.ehci_pdata = &tegra_ehci1_utmi_pdata,
};

static void molly_usb_init(void)
{
	int usb_port_owner_info = tegra_get_usb_port_owner_info();

	/* Set USB wake sources for molly */
	tegra_set_usb_wake_source();

	if (!(usb_port_owner_info & UTMI1_PORT_OWNER_XUSB)) {
		if (tegra_get_chipid() == TEGRA_CHIPID_TEGRA11 &&
			tegra_revision == TEGRA_REVISION_A02) {
			tegra_ehci1_utmi_pdata \
			.unaligned_dma_buf_supported = false;
			tegra_udc_pdata.unaligned_dma_buf_supported = false;
		}
		tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
		platform_device_register(&tegra_otg_device);
		/* Setup the udc platform data */
		tegra_udc_device.dev.platform_data = &tegra_udc_pdata;
	}

	if (!(usb_port_owner_info & UTMI2_PORT_OWNER_XUSB)) {
		if (tegra_get_chipid() == TEGRA_CHIPID_TEGRA11 &&
			tegra_revision == TEGRA_REVISION_A02) {
			tegra_ehci3_utmi_pdata \
			.unaligned_dma_buf_supported = false;
		}
		tegra_ehci3_device.dev.platform_data = &tegra_ehci3_utmi_pdata;
		platform_device_register(&tegra_ehci3_device);
	}
}

static struct tegra_xusb_pad_data xusb_padctl_data = {
	.pad_mux = (0x1 << 2),
	.port_cap = (0x1 << 4),
	.snps_oc_map = (0x1fc << 0),
	.usb2_oc_map = (0x2f << 0),
	.ss_port_map = (0x2 << 0),
	.oc_det = (0x2c << 10),
	.rx_wander = (0xf << 4),
	.rx_eq = (0x3070 << 8),
	.cdr_cntl = (0x26 << 24),
	.dfe_cntl = 0x002008EE,
	.hs_slew = (0xE << 6),
	.ls_rslew = (0x3 << 14),
	.otg_pad0_ctl0 = (0x7 << 19),
	.otg_pad1_ctl0 = (0x0 << 19),
	.otg_pad0_ctl1 = (0x4 << 0),
	.otg_pad1_ctl1 = (0x3 << 0),
	.hs_disc_lvl = (0x5 << 2),
	.hsic_pad0_ctl0 = (0x00 << 8),
	.hsic_pad0_ctl1 = (0x00 << 8),
};

static void molly_xusb_init(void)
{
	int usb_port_owner_info = tegra_get_usb_port_owner_info();

	if (usb_port_owner_info & UTMI2_PORT_OWNER_XUSB) {
		u32 usb_calib0 = tegra_fuse_readl(FUSE_SKU_USB_CALIB_0);

		pr_info("molly_xusb_init: usb_calib0 = 0x%08x\n", usb_calib0);
		/*
		 * read from usb_calib0 and pass to driver
		 * set HS_CURR_LEVEL (PAD0)	= usb_calib0[5:0]
		 * set TERM_RANGE_ADJ		= usb_calib0[10:7]
		 * set HS_SQUELCH_LEVEL		= usb_calib0[12:11]
		 * set HS_IREF_CAP		= usb_calib0[14:13]
		 * set HS_CURR_LEVEL (PAD1)	= usb_calib0[20:15]
		 */

		xusb_padctl_data.hs_curr_level_pad0 = (usb_calib0 >> 0) & 0x3f;
		xusb_padctl_data.hs_term_range_adj = (usb_calib0 >> 7) & 0xf;
		xusb_padctl_data.hs_squelch_level = (usb_calib0 >> 11) & 0x3;
		xusb_padctl_data.hs_iref_cap = (usb_calib0 >> 13) & 0x3;
		xusb_padctl_data.hs_curr_level_pad1 = (usb_calib0 >> 15) & 0x3f;

		tegra_xhci_device.dev.platform_data = &xusb_padctl_data;
		platform_device_register(&tegra_xhci_device);
	}
}

#else
static void molly_usb_init(void) { }
static void molly_xusb_init(void) { }
#endif

static void molly_audio_init(void)
{
	struct board_info board_info;

	tegra_get_board_info(&board_info);

	molly_audio_pdata.codec_name = "rt5640.0-001c";
	molly_audio_pdata.codec_dai_name = "rt5640-aif1";
}


#define ATHOME_RADIO_INT_GPIO     TEGRA_GPIO_PB6 /* SDMMC3_DAT3/GPIO_PB6 */
#define ATHOME_RADIO_RESET_N_GPIO TEGRA_GPIO_PB5 /* SDMMC3_DAT2/GPIO_PB5 */
#define ATHOME_RADIO_SPI_CS_GPIO  TEGRA_GPIO_PA7 /* SDMMC3_CMD/GPIO_PA7 */

static struct athome_platform_data radio_pdata = {
	.gpio_num_irq = ATHOME_RADIO_INT_GPIO,
	.gpio_num_rst = ATHOME_RADIO_RESET_N_GPIO,
	.gpio_spi_cs  = ATHOME_RADIO_SPI_CS_GPIO,
};

#define ATHOME_RADIO_SPI_BUS_NUM 1 /* bus 1, spi2 */
#define ATHOME_RADIO_SPI_CS      0
/* 2MHZ is max for sim3 right now.  Need to verify
 * clock values available to SPI for Tegra.
 * Depends on clks (dalmore pll_p is 408MHz and clk_m is 12MHz)
 * and dividers available.
 * 1.5MHz was setting we used in wolfie.
 */
#define ATHOME_RADIO_SPI_MAX_HZ  1500000

static struct spi_board_info molly_radio_spi_info[] __initdata = {
	{
		.modalias	= ATHOME_RADIO_MOD_NAME,
		.platform_data  = &radio_pdata,
		.irq		= -1,
		.max_speed_hz   = ATHOME_RADIO_SPI_MAX_HZ,
		.bus_num	= ATHOME_RADIO_SPI_BUS_NUM,
		.chip_select	= ATHOME_RADIO_SPI_CS,
		.mode           = SPI_MODE_0,
	},
};

static struct platform_device athome_radio_dev = {
	.name = ATHOME_RADIO_MOD_NAME,
	.num_resources = 0,
	.dev = {
		.platform_data	 = &radio_pdata,
	},
};

static void __init molly_radio_init(void)
{
#if 0
	/* must enable input too or else rx doesn't work when we're master */
	omap_mux_init_signal("mcspi4_clk", OMAP_MUX_MODE0 | OMAP_PIN_INPUT);

	omap_mux_init_signal("mcspi4_somi", OMAP_MUX_MODE0 | OMAP_PIN_INPUT);

	omap_mux_init_signal("mcspi4_simo", OMAP_MUX_MODE0);

	/* cs is manually controlled by the athome_radio driver because
	 * it needs to be held low (active low) through the entire
	 * transaction which may involve multiple spi transfers.
	 */
	omap_mux_init_gpio(ATHOME_RADIO_SPI_CS_GPIO, OMAP_PIN_OUTPUT);

	omap_mux_init_gpio(ATHOME_RADIO_INT_GPIO, OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_gpio(ATHOME_RADIO_RESET_N_GPIO, OMAP_PIN_OUTPUT);
#endif

	spi_register_board_info(molly_radio_spi_info,
				ARRAY_SIZE(molly_radio_spi_info));

	platform_device_register(&athome_radio_dev);
}

static struct platform_device *molly_spi_devices[] __initdata = {
        &tegra11_spi_device2,
};

struct spi_clk_parent spi_parent_clk_molly[] = {
        [0] = {.name = "pll_p"},
#ifndef CONFIG_TEGRA_PLLM_RESTRICTED
        [1] = {.name = "pll_m"},
        [2] = {.name = "clk_m"},
#else
        [1] = {.name = "clk_m"},
#endif
};

static struct tegra_spi_platform_data molly_spi_pdata = {
	.max_dma_buffer         = 16 * 1024,
        .is_clkon_always        = false,
        .max_rate               = 25000000,
	.is_dma_based           = true,
};

static void __init molly_spi_init(void)
{
        int i;
        struct clk *c;

        for (i = 0; i < ARRAY_SIZE(spi_parent_clk_molly); ++i) {
                c = tegra_get_clock_by_name(spi_parent_clk_molly[i].name);
                if (IS_ERR_OR_NULL(c)) {
                        pr_err("Not able to get the clock for %s\n",
                                                spi_parent_clk_molly[i].name);
                        continue;
                }
                spi_parent_clk_molly[i].parent_clk = c;
                spi_parent_clk_molly[i].fixed_clk_rate = clk_get_rate(c);
		pr_info("%s: clock %s, rate %lu\n",
			__func__, spi_parent_clk_molly[i].name,
			spi_parent_clk_molly[i].fixed_clk_rate);
        }
        molly_spi_pdata.parent_clk_list = spi_parent_clk_molly;
        molly_spi_pdata.parent_clk_count = ARRAY_SIZE(spi_parent_clk_molly);
	tegra11_spi_device2.dev.platform_data = &molly_spi_pdata;
        platform_add_devices(molly_spi_devices,
                                ARRAY_SIZE(molly_spi_devices));
}

static void __init tegra_molly_init(void)
{
	struct board_info board_info;

	tegra_get_display_board_info(&board_info);
	tegra_clk_init_from_table(molly_clk_init_table);
	tegra_clk_verify_parents();
	tegra_soc_device_init("molly");
	tegra_enable_pinmux();
	molly_pinmux_init();
	molly_i2c_init();
	molly_spi_init();
	molly_radio_init();
	molly_usb_init();
	molly_xusb_init();
	molly_uart_init();
	molly_audio_init();
	platform_add_devices(molly_devices, ARRAY_SIZE(molly_devices));
	tegra_ram_console_debug_init();
	tegra_io_dpd_init();
	molly_regulator_init();
	molly_sdhci_init();
	molly_suspend_init();
	molly_emc_init();
	molly_edp_init();
	molly_panel_init();
	molly_pmon_init();
#if defined(CONFIG_BT_BLUESLEEP) || defined(CONFIG_BT_BLUESLEEP_MODULE)
	molly_setup_bluesleep();
	molly_setup_bt_rfkill();
#elif defined CONFIG_BLUEDROID_PM
	molly_setup_bluedroid_pm();
#endif
	tegra_release_bootloader_fb();
#ifdef CONFIG_TEGRA_WDT_RECOVERY
	tegra_wdt_recovery_init();
#endif
	tegra_serial_debug_init(TEGRA_UARTD_BASE, INT_WDT_CPU, NULL, -1, -1);
//	molly_sensors_init();
	molly_soctherm_init();
	tegra_register_fuse();
}

static void __init molly_ramconsole_reserve(unsigned long size)
{
	tegra_ram_console_debug_reserve(SZ_1M);
}

static void __init tegra_molly_dt_init(void)
{
#ifdef CONFIG_USE_OF
	of_platform_populate(NULL,
		of_default_bus_match_table, NULL, NULL);
#endif

	tegra_molly_init();
}

static void __init tegra_molly_reserve(void)
{
#if defined(CONFIG_NVMAP_CONVERT_CARVEOUT_TO_IOVMM)
	/* 1920*1200*4*2 = 18432000 bytes */
	tegra_reserve(0, SZ_16M + SZ_2M, SZ_16M);
#else
	tegra_reserve(SZ_128M, SZ_16M + SZ_2M, SZ_4M);
#endif
	molly_ramconsole_reserve(SZ_1M);
}

static const char * const molly_dt_board_compat[] = {
	"nvidia,dalmore", /* allows us to work with dalmore bootloader for now */
	"google,molly",
	NULL
};

MACHINE_START(MOLLY, "molly")
	.atag_offset	= 0x100,
	.soc		= &tegra_soc_desc,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_molly_reserve,
	.init_early	= tegra11x_init_early,
	.init_irq	= tegra_init_irq,
	.handle_irq	= gic_handle_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_molly_dt_init,
	.restart	= tegra_assert_system_reset,
	.dt_compat	= molly_dt_board_compat,
MACHINE_END
