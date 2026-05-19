/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>
// #include <zephyr/input/input.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <stdio.h>

// 看视频跟做
#include <zephyr/drivers/i2c.h>

#ifdef CONFIG_BMP580
#include "sensor.h"
#endif
#ifdef CONFIG_IMU
#include "imu.h"
#endif

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_USE_IDENTITY),
	80,
	82,
	NULL);

typedef struct adv_mfg_data {
	uint16_t company_code;
	uint8_t filter_1;
	uint8_t filter_2;
	uint8_t filter_3;
	uint8_t count;
	float acc_x;
	float pressure;
} adv_mfg_data_type;

static adv_mfg_data_type adv_mfg_data = {
	.company_code = 0xFFFF,
	.filter_1 = 0xDF,
	.filter_2 = 0xDF,
	.filter_3 = 0xDF,
	.count = 0,
	.acc_x = 9.8f,
	.pressure = 1.01f,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_MANUFACTURER_DATA,
		(unsigned char *)&adv_mfg_data,
		sizeof(adv_mfg_data)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

#define SLEEP_TIME_MS 1000

// LED引脚
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

// 两个使能引脚
#define PWR_NODE DT_ALIAS(pwrkey)
static const struct gpio_dt_spec pwr_key = GPIO_DT_SPEC_GET(PWR_NODE, gpios);
#define SENSOR_NODE DT_ALIAS(sensorsw)
static const struct gpio_dt_spec sensor_enable = GPIO_DT_SPEC_GET(SENSOR_NODE, gpios);

// // 按键回调
// static void button_input_cb(struct input_event *evt, void *user_data)
// {
// 	if (evt->sync == 0) {
// 		return;
// 	}

// 	// printk("Button %d %s at %" PRIu32 "\n",
// 	//        evt->code,
// 	//        evt->value ? "pressed" : "released",
// 	//        k_cycle_get_32());

// 	// if(evt->value == 0)
// 	// {
// 	// 	// 按下灯灭
// 	// 	gpio_pin_set_dt(&led, 0);
// 	// }
// 	// else
// 	// {
// 	// 	// 松开灯亮
// 	// 	gpio_pin_set_dt(&led, 1);
// 	// }	
// }

// INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);

static int update_adv_payload(uint8_t count, float acc_x, float pressure)
{
	adv_mfg_data.count = count;
	adv_mfg_data.acc_x = acc_x;
	adv_mfg_data.pressure = pressure;

	return bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}

static void bt_ready(int err)
{
	bt_addr_le_t addr;

	err = bt_addr_le_from_str("FF:EE:DD:CC:BB:AA", "random", &addr);
	if (err) {
		printk("Invalid BT address (err %d)\n", err);
	}

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
	}
}

int main(void)
{
	int err;
	uint8_t count = 0;
	float acc_x = 9.8f;
	float pressure = 1.01f;

	// 初始化led
	gpio_is_ready_dt(&led);
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	// 初始化使能引脚
	gpio_is_ready_dt(&pwr_key);
	gpio_pin_configure_dt(&pwr_key, GPIO_OUTPUT_ACTIVE);
	gpio_is_ready_dt(&sensor_enable);
	gpio_pin_configure_dt(&sensor_enable, GPIO_OUTPUT_ACTIVE);

	// gpio_pin_set_dt(&led, 1);

	#if defined(CONFIG_BMP580) && defined(CONFIG_IMU)
	struct imu_sample imu;
	struct sensor_sample_bundle sensors;
	int ret;
	int imu_ret;
	int64_t next_sensor_log = 0;
	int64_t next_imu_log = 0;
	ret = sensor_init_all();
	if (ret != 0) {
		printk("sensor_init_all failed: %d\n", ret);
		return 0;
	}

	ret = imu_init();
	if (ret != 0) {
		printk("imu_init failed: %d\n", ret);
		return 0;
	}
	#endif

	printk("fusion_combo sample start\n");

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	while (1) {
		gpio_pin_toggle_dt(&led);

		count++;
		acc_x += 1.0f;
		pressure += 1.0f;

		err = update_adv_payload(count, acc_x, pressure);
		if (err) {
			printk("Advertising data update failed (err %d)\n", err);
		}

		#ifdef CONFIG_BMP580
		imu_ret = imu_fetch(&imu);
		if (k_uptime_get() >= next_sensor_log) {
			ret = sensor_fetch_all(&sensors);
			if (ret == 0) {
				if (sensors.pressure_valid || sensors.temperature_valid) {
					printk("baro: pressure=%.3f kPa temp=%.3f C altitude=%.3f m\n",
						sensors.pressure_kpa,
						sensors.temperature_c,
						sensors.altitude_m);
				}
			}

			next_sensor_log = k_uptime_get() + 500;
		}
		#endif

		#ifdef CONFIG_IMU
		if (imu_ret == 0 && k_uptime_get() >= next_imu_log) {
			printk("imu: acc=(%.3f %.3f %.3f) gyro=(%.3f %.3f %.3f)\n",
					imu.accel_mps2[0], imu.accel_mps2[1], imu.accel_mps2[2],
					imu.gyro_rps[0], imu.gyro_rps[1], imu.gyro_rps[2]);
			next_imu_log = k_uptime_get() + 100;
		}
		#endif

		// k_sleep(K_MSEC(SLEEP_TIME_MS));
		k_msleep(SLEEP_TIME_MS);
	}
}


