/*
 * arch/arm/mach-tegra/board-macallan-sensors.c
 *
 * Copyright (c) 2013 NVIDIA CORPORATION, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of NVIDIA CORPORATION nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mpu.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/therm_est.h>
#include <linux/nct1008.h>
#include <linux/cm3217.h>
#include <mach/edp.h>
#include <linux/edp.h>
#include <mach/gpio-tegra.h>
#include <mach/pinmux-t11.h>
#include <mach/pinmux.h>
#include <media/imx091.h>
#include <media/ov9772.h>
#include <media/as364x.h>
#include <media/ad5816.h>
#include <generated/mach-types.h>
#include <linux/power/sbs-battery.h>

#include "gpio-names.h"
#include "board.h"
#include "board-common.h"
#include "board-macallan.h"
#include "cpu-tegra.h"
#include "devices.h"
#include "tegra-board-id.h"
#include "dvfs.h"

static struct nvc_gpio_pdata imx091_gpio_pdata[] = {
	{IMX091_GPIO_RESET, CAM_RSTN, true, false},
	{IMX091_GPIO_PWDN, CAM1_POWER_DWN_GPIO, true, false},
};

static struct board_info board_info;

static struct throttle_table tj_throttle_table[] = {
		/* CPU_THROT_LOW cannot be used by other than CPU */
		/* NO_CAP cannot be used by CPU */
		/*    CPU,   C2BUS,   C3BUS,    SCLK,     EMC */
		{ { 1530000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1428000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1326000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1224000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1122000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1020000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  918000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  816000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  714000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  612000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  612000,  564000,  564000,  NO_CAP,  NO_CAP } },
		{ {  612000,  528000,  528000,  NO_CAP,  NO_CAP } },
		{ {  612000,  492000,  492000,  NO_CAP,  NO_CAP } },
		{ {  612000,  420000,  420000,  NO_CAP,  NO_CAP } },
		{ {  612000,  408000,  408000,  NO_CAP,  NO_CAP } },
		{ {  612000,  360000,  360000,  NO_CAP,  NO_CAP } },
		{ {  612000,  360000,  360000,  312000,  NO_CAP } },
		{ {  510000,  360000,  360000,  312000,  480000 } },
		{ {  468000,  360000,  360000,  312000,  480000 } },
		{ {  468000,  276000,  276000,  208000,  480000 } },
		{ {  372000,  276000,  276000,  208000,  204000 } },
		{ {  288000,  276000,  276000,  208000,  204000 } },
		{ {  252000,  276000,  228000,  208000,  102000 } },
		{ {  204000,  276000,  228000,  208000,  102000 } },
		{ {  102000,  276000,  228000,  208000,  102000 } },
	  { { CPU_THROT_LOW,  276000,  228000,  208000,  102000 } },
};

static struct balanced_throttle tj_throttle = {
	.throt_tab_size = ARRAY_SIZE(tj_throttle_table),
	.throt_tab = tj_throttle_table,
};

static int __init macallan_throttle_init(void)
{
	if (machine_is_macallan())
		balanced_throttle_register(&tj_throttle, "tegra-balanced");
	return 0;
}
module_init(macallan_throttle_init);

static struct nct1008_platform_data macallan_nct1008_pdata = {
	.supported_hwrev = true,
	.ext_range = true,
	.conv_rate = 0x08,
	.shutdown_ext_limit = 105, /* C */
	.shutdown_local_limit = 120, /* C */

	.num_trips = 1,
	.trips = {
		{
			.cdev_type = "suspend_soctherm",
			.trip_temp = 50000,
			.trip_type = THERMAL_TRIP_ACTIVE,
			.upper = 1,
			.lower = 1,
			.hysteresis = 5000,
		},
	},
};

static struct i2c_board_info macallan_i2c4_nct1008_board_info[] = {
	{
		I2C_BOARD_INFO("nct1008", 0x4C),
		.platform_data = &macallan_nct1008_pdata,
		.irq = -1,
	}
};

#define VI_PINMUX(_pingroup, _mux, _pupd, _tri, _io, _lock, _ioreset) \
	{							\
		.pingroup	= TEGRA_PINGROUP_##_pingroup,	\
		.func		= TEGRA_MUX_##_mux,		\
		.pupd		= TEGRA_PUPD_##_pupd,		\
		.tristate	= TEGRA_TRI_##_tri,		\
		.io		= TEGRA_PIN_##_io,		\
		.lock		= TEGRA_PIN_LOCK_##_lock,	\
		.od		= TEGRA_PIN_OD_DEFAULT,		\
		.ioreset	= TEGRA_PIN_IO_RESET_##_ioreset	\
}

static int macallan_focuser_power_on(struct ad5816_power_rail *pw)
{
	int err;

	if (unlikely(WARN_ON(!pw || !pw->vdd || !pw->vdd_i2c)))
		return -EFAULT;

	err = regulator_enable(pw->vdd_i2c);
	if (unlikely(err))
		goto ad5816_vdd_i2c_fail;

	err = regulator_enable(pw->vdd);
	if (unlikely(err))
		goto ad5816_vdd_fail;

	return 0;

ad5816_vdd_fail:
	regulator_disable(pw->vdd_i2c);

ad5816_vdd_i2c_fail:
	pr_err("%s FAILED\n", __func__);

	return -ENODEV;
}

static int macallan_focuser_power_off(struct ad5816_power_rail *pw)
{
	if (unlikely(WARN_ON(!pw || !pw->vdd || !pw->vdd_i2c)))
		return -EFAULT;

	regulator_disable(pw->vdd);
	regulator_disable(pw->vdd_i2c);

	return 0;
}

static struct tegra_pingroup_config mclk_disable =
	VI_PINMUX(CAM_MCLK, VI, NORMAL, NORMAL, OUTPUT, DEFAULT, DEFAULT);

static struct tegra_pingroup_config mclk_enable =
	VI_PINMUX(CAM_MCLK, VI_ALT3, NORMAL, NORMAL, OUTPUT, DEFAULT, DEFAULT);

static struct tegra_pingroup_config pbb0_disable =
	VI_PINMUX(GPIO_PBB0, VI, NORMAL, NORMAL, OUTPUT, DEFAULT, DEFAULT);

static struct tegra_pingroup_config pbb0_enable =
	VI_PINMUX(GPIO_PBB0, VI_ALT3, NORMAL, NORMAL, OUTPUT, DEFAULT, DEFAULT);

static int macallan_imx091_power_on(struct nvc_regulator *vreg)
{
	int err;

	if (unlikely(WARN_ON(!vreg)))
		return -EFAULT;

	gpio_set_value(CAM1_POWER_DWN_GPIO, 0);
	usleep_range(10, 20);

	err = regulator_enable(vreg[IMX091_VREG_AVDD].vreg);
	if (err)
		goto imx091_avdd_fail;

	err = regulator_enable(vreg[IMX091_VREG_IOVDD].vreg);
	if (err)
		goto imx091_iovdd_fail;

	usleep_range(1, 2);
	gpio_set_value(CAM1_POWER_DWN_GPIO, 1);

	tegra_pinmux_config_table(&mclk_enable, 1);
	usleep_range(300, 310);

	return 1;

imx091_iovdd_fail:
	regulator_disable(vreg[IMX091_VREG_AVDD].vreg);

imx091_avdd_fail:
	gpio_set_value(CAM1_POWER_DWN_GPIO, 0);

	pr_err("%s FAILED\n", __func__);
	return -ENODEV;
}

static int macallan_imx091_power_off(struct nvc_regulator *vreg)
{
	if (unlikely(WARN_ON(!vreg)))
		return -EFAULT;

	usleep_range(1, 2);
	tegra_pinmux_config_table(&mclk_disable, 1);
	gpio_set_value(CAM1_POWER_DWN_GPIO, 0);
	usleep_range(1, 2);

	regulator_disable(vreg[IMX091_VREG_IOVDD].vreg);
	regulator_disable(vreg[IMX091_VREG_AVDD].vreg);

	return 1;
}

static struct nvc_imager_cap imx091_cap = {
	.identifier		= "IMX091",
	.sensor_nvc_interface	= 3,
	.pixel_types[0]		= 0x100,
	.orientation		= 0,
	.direction		= 0,
	.initial_clock_rate_khz	= 6000,
	.clock_profiles[0] = {
		.external_clock_khz	= 24000,
		.clock_multiplier	= 850000, /* value / 1,000,000 */
	},
	.clock_profiles[1] = {
		.external_clock_khz	= 0,
		.clock_multiplier	= 0,
	},
	.h_sync_edge		= 0,
	.v_sync_edge		= 0,
	.mclk_on_vgp0		= 0,
	.csi_port		= 0,
	.data_lanes		= 4,
	.virtual_channel_id	= 0,
	.discontinuous_clk_mode	= 1,
	.cil_threshold_settle	= 0x0,
	.min_blank_time_width	= 16,
	.min_blank_time_height	= 16,
	.preferred_mode_index	= 0,
	.focuser_guid		= NVC_FOCUS_GUID(0),
	.torch_guid		= NVC_TORCH_GUID(0),
	.cap_version		= NVC_IMAGER_CAPABILITIES_VERSION2,
};

static struct imx091_platform_data imx091_pdata = {
	.num			= 0,
	.sync			= 0,
	.dev_name		= "camera",
	.gpio_count		= ARRAY_SIZE(imx091_gpio_pdata),
	.gpio			= imx091_gpio_pdata,
	.flash_cap		= {
		.sdo_trigger_enabled = 1,
		.adjustable_flash_timing = 1,
	},
	.cap			= &imx091_cap,
	.power_on		= macallan_imx091_power_on,
	.power_off		= macallan_imx091_power_off,
};

static struct sbs_platform_data sbs_pdata = {
	.poll_retry_count = 100,
	.i2c_retry_count = 2,
};

static int macallan_ov9772_power_on(struct ov9772_power_rail *pw)
{
	int err;

	if (unlikely(!pw || !pw->avdd || !pw->dovdd))
		return -EFAULT;

	gpio_set_value(CAM2_POWER_DWN_GPIO, 0);
	gpio_set_value(CAM_RSTN, 0);

	err = regulator_enable(pw->avdd);
	if (unlikely(err))
		goto ov9772_avdd_fail;

	err = regulator_enable(pw->dovdd);
	if (unlikely(err))
		goto ov9772_dovdd_fail;

	gpio_set_value(CAM_RSTN, 1);
	gpio_set_value(CAM2_POWER_DWN_GPIO, 1);

	tegra_pinmux_config_table(&pbb0_enable, 1);
	usleep_range(340, 380);

	/* return 1 to skip the in-driver power_on sequence */
	return 1;

ov9772_dovdd_fail:
	regulator_disable(pw->avdd);

ov9772_avdd_fail:
	gpio_set_value(CAM_RSTN, 0);
	gpio_set_value(CAM2_POWER_DWN_GPIO, 0);

	pr_err("%s FAILED\n", __func__);
	return -ENODEV;
}

static int macallan_ov9772_power_off(struct ov9772_power_rail *pw)
{
	if (unlikely(!pw || !pw->avdd || !pw->dovdd))
		return -EFAULT;

	usleep_range(21, 25);
	tegra_pinmux_config_table(&pbb0_disable, 1);

	gpio_set_value(CAM2_POWER_DWN_GPIO, 0);
	gpio_set_value(CAM_RSTN, 0);

	regulator_disable(pw->dovdd);
	regulator_disable(pw->avdd);

	/* return 1 to skip the in-driver power_off sequence */
	return 1;
}

static struct nvc_gpio_pdata ov9772_gpio_pdata[] = {
	{ OV9772_GPIO_TYPE_SHTDN, CAM2_POWER_DWN_GPIO, true, 0, },
	{ OV9772_GPIO_TYPE_PWRDN, CAM_RSTN, true, 0, },
};

static struct ov9772_platform_data macallan_ov9772_pdata = {
	.num		= 1,
	.dev_name	= "camera",
	.gpio_count	= ARRAY_SIZE(ov9772_gpio_pdata),
	.gpio		= ov9772_gpio_pdata,
	.power_on	= macallan_ov9772_power_on,
	.power_off	= macallan_ov9772_power_off,
};

static struct as364x_platform_data macallan_as3648_pdata = {
	.config		= {
		.max_total_current_mA = 1000,
		.max_peak_current_mA = 600,
		.vin_low_v_run_mV = 3070,
		.strobe_type = 1,
		},
	.pinstate	= {
		.mask	= 1 << (CAM_FLASH_STROBE - TEGRA_GPIO_PBB0),
		.values	= 1 << (CAM_FLASH_STROBE - TEGRA_GPIO_PBB0)
		},
	.dev_name	= "torch",
	.type		= AS3648,
	.gpio_strobe	= CAM_FLASH_STROBE,
	.led_mask	= 3,
};

static struct ad5816_platform_data macallan_ad5816_pdata = {
	.cfg = 0,
	.num = 0,
	.sync = 0,
	.dev_name = "focuser",
	.power_on = macallan_focuser_power_on,
	.power_off = macallan_focuser_power_off,
};

static struct i2c_board_info macallan_i2c_board_info_e1625[] = {
	{
		I2C_BOARD_INFO("imx091", 0x36),
		.platform_data = &imx091_pdata,
	},
	{
		I2C_BOARD_INFO("ov9772", 0x10),
		.platform_data = &macallan_ov9772_pdata,
	},
	{
		I2C_BOARD_INFO("as3648", 0x30),
		.platform_data = &macallan_as3648_pdata,
	},
	{
		I2C_BOARD_INFO("ad5816", 0x0E),
		.platform_data = &macallan_ad5816_pdata,
	},
};

static int macallan_camera_init(void)
{
	tegra_pinmux_config_table(&mclk_disable, 1);
	tegra_pinmux_config_table(&pbb0_disable, 1);

	i2c_register_board_info(2, macallan_i2c_board_info_e1625,
		ARRAY_SIZE(macallan_i2c_board_info_e1625));
	return 0;
}

#define TEGRA_CAMERA_GPIO(_gpio, _label, _value)		\
	{							\
		.gpio = _gpio,					\
		.label = _label,				\
		.value = _value,				\
	}

static struct cm3217_platform_data macallan_cm3217_pdata = {
	.levels = {10, 160, 225, 320, 640, 1280, 2600, 5800, 8000, 10240},
	.golden_adc = 0,
	.power = 0,
};

static struct i2c_board_info macallan_i2c0_board_info_cm3217[] = {
	{
		I2C_BOARD_INFO("cm3217", 0x10),
		.platform_data = &macallan_cm3217_pdata,
	},
};

/* MPU board file definition	*/
static struct mpu_platform_data mpu6050_gyro_data = {
	.int_config	= 0x10,
	.level_shifter	= 0,
	/* Located in board_[platformname].h */
	.orientation	= MPU_GYRO_ORIENTATION,
	.sec_slave_type	= SECONDARY_SLAVE_TYPE_NONE,
	.key		= {0x4E, 0xCC, 0x7E, 0xEB, 0xF6, 0x1E, 0x35, 0x22,
			   0x00, 0x34, 0x0D, 0x65, 0x32, 0xE9, 0x94, 0x89},
};

static struct mpu_platform_data mpu_compass_data = {
	.orientation	= MPU_COMPASS_ORIENTATION,
	.config		= NVI_CONFIG_BOOT_MPU,
};

static struct mpu_platform_data bmp180_pdata = {
	.config		= NVI_CONFIG_BOOT_MPU,
};

static struct i2c_board_info __initdata inv_mpu6050_i2c2_board_info[] = {
	{
		I2C_BOARD_INFO(MPU_GYRO_NAME, MPU_GYRO_ADDR),
		.platform_data = &mpu6050_gyro_data,
	},
	{
		/* The actual BMP180 address is 0x77 but because this conflicts
		 * with another device, this address is hacked so Linux will
		 * call the driver.  The conflict is technically okay since the
		 * BMP180 is behind the MPU.  Also, the BMP180 driver uses a
		 * hard-coded address of 0x77 since it can't be changed anyway.
		 */
		I2C_BOARD_INFO("bmp180", 0x78),
		.platform_data = &bmp180_pdata,
	},
	{
		I2C_BOARD_INFO(MPU_COMPASS_NAME, MPU_COMPASS_ADDR),
		.platform_data = &mpu_compass_data,
	},
};

static void mpuirq_init(void)
{
	int ret = 0;
	unsigned gyro_irq_gpio = MPU_GYRO_IRQ_GPIO;
	unsigned gyro_bus_num = MPU_GYRO_BUS_NUM;
	char *gyro_name = MPU_GYRO_NAME;

	pr_info("*** MPU START *** mpuirq_init...\n");

	ret = gpio_request(gyro_irq_gpio, gyro_name);

	if (ret < 0) {
		pr_err("%s: gpio_request failed %d\n", __func__, ret);
		return;
	}

	ret = gpio_direction_input(gyro_irq_gpio);
	if (ret < 0) {
		pr_err("%s: gpio_direction_input failed %d\n", __func__, ret);
		gpio_free(gyro_irq_gpio);
		return;
	}
	pr_info("*** MPU END *** mpuirq_init...\n");

	inv_mpu6050_i2c2_board_info[0].irq = gpio_to_irq(MPU_GYRO_IRQ_GPIO);
	i2c_register_board_info(gyro_bus_num, inv_mpu6050_i2c2_board_info,
		ARRAY_SIZE(inv_mpu6050_i2c2_board_info));
}

static int macallan_nct1008_init(void)
{
	int nct1008_port;
	int ret = 0;

	nct1008_port = TEGRA_GPIO_PO4;

	tegra_add_cdev_trips(macallan_nct1008_pdata.trips,
				&macallan_nct1008_pdata.num_trips);

	macallan_i2c4_nct1008_board_info[0].irq = gpio_to_irq(nct1008_port);
	pr_info("%s: macallan nct1008 irq %d",
			__func__, macallan_i2c4_nct1008_board_info[0].irq);

	ret = gpio_request(nct1008_port, "temp_alert");
	if (ret < 0)
		return ret;

	ret = gpio_direction_input(nct1008_port);
	if (ret < 0) {
		pr_info("%s: calling gpio_free(nct1008_port)", __func__);
		gpio_free(nct1008_port);
	}

	/* macallan has thermal sensor on GEN1-I2C i.e. instance 0 */
	i2c_register_board_info(0, macallan_i2c4_nct1008_board_info,
		ARRAY_SIZE(macallan_i2c4_nct1008_board_info));

	return ret;
}

static struct i2c_board_info __initdata bq20z45_pdata[] = {
	{
		I2C_BOARD_INFO("sbs-battery", 0x0B),
		.platform_data = &sbs_pdata,
	},
};

#ifdef CONFIG_TEGRA_SKIN_THROTTLE
static int tegra_skin_match(struct thermal_zone_device *thz, void *data)
{
	return strcmp((char *)data, thz->type) == 0;
}

static int tegra_skin_get_temp(void *data, long *temp)
{
	struct thermal_zone_device *thz;

	thz = thermal_zone_device_find(data, tegra_skin_match);

	if (!thz || thz->ops->get_temp(thz, temp))
		*temp = 25000;

	return 0;
}

static struct therm_est_data skin_data = {
	.cdev_type = "skin-balanced",
	.toffset = 9793,
	.polling_period = 1100,
	.ndevs = 2,
	.tc1 = 10,
	.tc2 = 1,
	.devs = {
			{
				.dev_data = "nct_ext",
				.get_temp = tegra_skin_get_temp,
				.coeffs = {
					2, 1, 1, 1,
					1, 1, 1, 1,
					1, 1, 1, 0,
					1, 1, 0, 0,
					0, 0, -1, -7
				},
			},
			{
				.dev_data = "nct_int",
				.get_temp = tegra_skin_get_temp,
				.coeffs = {
					-11, -7, -5, -3,
					-3, -2, -1, 0,
					0, 0, 1, 1,
					1, 2, 2, 3,
					4, 6, 11, 18
				},
			},
	},
	.trip_temp = 45000,
	.passive_delay = 15000,
};

static struct throttle_table skin_throttle_table[] = {
		/* CPU_THROT_LOW cannot be used by other than CPU */
		/* NO_CAP cannot be used by CPU */
		/*    CPU,   C2BUS,   C3BUS,    SCLK,     EMC */
		{ { 1530000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1530000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1326000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1326000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1326000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1326000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1326000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1122000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1122000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1122000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1122000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1122000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1122000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1020000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1020000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1020000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1020000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1020000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ { 1020000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  918000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  918000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  918000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  918000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  918000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  918000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  816000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  816000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  816000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  816000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  816000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  816000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  714000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  714000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  714000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  714000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  714000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  714000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  612000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  612000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  612000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  612000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  612000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  612000,  NO_CAP,  NO_CAP,  NO_CAP,  NO_CAP } },
		{ {  612000,  564000,  564000,  NO_CAP,  NO_CAP } },
		{ {  612000,  564000,  564000,  NO_CAP,  NO_CAP } },
		{ {  612000,  528000,  528000,  NO_CAP,  NO_CAP } },
		{ {  612000,  528000,  528000,  NO_CAP,  NO_CAP } },
		{ {  612000,  492000,  492000,  NO_CAP,  NO_CAP } },
		{ {  612000,  492000,  492000,  NO_CAP,  NO_CAP } },
		{ {  612000,  420000,  420000,  NO_CAP,  NO_CAP } },
		{ {  612000,  420000,  420000,  NO_CAP,  NO_CAP } },
		{ {  612000,  408000,  408000,  NO_CAP,  NO_CAP } },
		{ {  612000,  408000,  408000,  NO_CAP,  NO_CAP } },
		{ {  612000,  360000,  360000,  NO_CAP,  NO_CAP } },
		{ {  612000,  360000,  360000,  NO_CAP,  NO_CAP } },
		{ {  510000,  360000,  360000,  312000,  NO_CAP } },
		{ {  510000,  360000,  360000,  312000,  NO_CAP } },
		{ {  510000,  360000,  360000,  312000,  480000 } },
		{ {  510000,  360000,  360000,  312000,  480000 } },
		{ {  510000,  360000,  360000,  312000,  480000 } },
		{ {  510000,  360000,  360000,  312000,  480000 } },
		{ {  510000,  360000,  360000,  312000,  480000 } },
		{ {  510000,  360000,  360000,  312000,  480000 } },
		{ {  468000,  360000,  360000,  312000,  480000 } },
		{ {  468000,  360000,  360000,  312000,  480000 } },
		{ {  468000,  276000,  276000,  208000,  480000 } },
		{ {  468000,  276000,  276000,  208000,  480000 } },
		{ {  372000,  276000,  276000,  208000,  204000 } },
		{ {  372000,  276000,  276000,  208000,  204000 } },
		{ {  288000,  276000,  276000,  208000,  204000 } },
		{ {  288000,  276000,  276000,  208000,  204000 } },
		{ {  252000,  276000,  228000,  208000,  102000 } },
		{ {  252000,  276000,  228000,  208000,  102000 } },
		{ {  204000,  276000,  228000,  208000,  102000 } },
		{ {  204000,  276000,  228000,  208000,  102000 } },
		{ {  102000,  276000,  228000,  208000,  102000 } },
	  { { CPU_THROT_LOW,  276000,  228000,  208000,  102000 } },
};

static struct balanced_throttle skin_throttle = {
        .throt_tab_size = ARRAY_SIZE(skin_throttle_table),
        .throt_tab = skin_throttle_table,
};

static int __init macallan_skin_init(void)
{
	if (machine_is_macallan()) {
		balanced_throttle_register(&skin_throttle, "skin-balanced");
		tegra_skin_therm_est_device.dev.platform_data = &skin_data;
		platform_device_register(&tegra_skin_therm_est_device);
	}

	return 0;
}
late_initcall(macallan_skin_init);
#endif

int __init macallan_sensors_init(void)
{
	int err;

	tegra_get_board_info(&board_info);

	/* E1545+E1604 has no temp sensor. */
	if (board_info.board_id != BOARD_E1545) {
		err = macallan_nct1008_init();
		if (err) {
			pr_err("%s: nct1008 register failed.\n", __func__);
			return err;
		}
	}

	macallan_camera_init();
	mpuirq_init();

	i2c_register_board_info(0, macallan_i2c0_board_info_cm3217,
				ARRAY_SIZE(macallan_i2c0_board_info_cm3217));

	i2c_register_board_info(0, bq20z45_pdata,
		ARRAY_SIZE(bq20z45_pdata));

	return 0;
}
