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

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

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
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

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

	gpio_is_ready_dt(&led);
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

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

		k_sleep(K_MSEC(SLEEP_TIME_MS));
	}
}
