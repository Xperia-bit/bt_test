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

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// 自定义广播参数
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_SCANNABLE |
	 BT_LE_ADV_OPT_USE_IDENTITY), /* scannable advertising and use identity address 使用身份地址，可以是public，static，可能是厂商写好，底层随机生成，用户自定义, 否则使用private，每次开机都随机生成一个*/
	800, /* Min Advertising Interval 500ms (800*0.625ms) */
	802, /* Max Advertising Interval 500.625ms (801*0.625ms) */
	NULL); /* Set to NULL for undirected advertising */

// 自定义厂商数据
typedef struct adv_mfg_data {
	uint16_t company_code; /* Company Identifier Code. */
	float acc_x; /* 加速度 */
	float pressure; /* 气压 */
} adv_mfg_data_type;
static adv_mfg_data_type adv_mfg_data = { 0xFFFF, 9.8,1.01 };

// 自定义广播包数据
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),// 声明为非经典蓝牙
	BT_DATA(BT_DATA_MANUFACTURER_DATA, (unsigned char *)&adv_mfg_data, sizeof(adv_mfg_data)),
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void bt_ready(int err)
{
	// size_t count = 1;
	bt_addr_le_t addr;
	err = bt_addr_le_from_str("FF:EE:DD:CC:BB:AA", "random", &addr);
	if (err) {
		printk("Invalid BT address (err %d)\n", err);
	}

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	// 读取MAC地址打印，这边不读
	// bt_id_get(&addr, &count);
	// bt_addr_le_to_str(&addr, addr_s, sizeof(addr_s));
	// printk("Beacon started, advertising as %s\n", addr_s);
}

int main(void)
{
	int err;

	printk("Starting Beacon Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	while(1)
	{
		// 延时1s
		k_sleep(K_MSEC(1000));
	}
}
