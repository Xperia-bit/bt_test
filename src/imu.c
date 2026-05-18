#include "imu.h"

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#if !DT_HAS_ALIAS(bmp581)
#error "Missing devicetree alias: lsm6dso"
#endif

// 看视频跟做
#include <zephyr/drivers/i2c.h>

static const struct device *const imu_dev = DEVICE_DT_GET(DT_ALIAS(lsm6dso));

static int set_sampling_frequency(const struct device *dev)
{
	struct sensor_value odr = {
		.val1 = 104,
		.val2 = 0,
	};
	int ret;

	ret = sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
	if (ret != 0) {
		return ret;
	}

	return sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
}

int imu_init(void)
{
	int ret;

	if (!device_is_ready(imu_dev)) {
		printk("imu sensor not ready: %s\n", imu_dev->name);
		return -ENODEV;
	}

	ret = set_sampling_frequency(imu_dev);
	if (ret != 0) {
		printk("failed to set lsm6dso odr: %d\n", ret);
		return ret;
	}

	return 0;
}

int imu_fetch(struct imu_sample *sample)
{
	struct sensor_value accel[3];
	struct sensor_value gyro[3];
	int ret;

	if (sample == NULL) {
		return -EINVAL;
	}

	ret = sensor_sample_fetch(imu_dev);
	if (ret != 0) {
		return ret;
	}

	ret = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, accel);
	if (ret != 0) {
		return ret;
	}

	ret = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, gyro);
	if (ret != 0) {
		return ret;
	}

	memset(sample, 0, sizeof(*sample));
	sample->accel_mps2[0] = (float)sensor_value_to_double(&accel[0]);
	sample->accel_mps2[1] = (float)sensor_value_to_double(&accel[1]);
	sample->accel_mps2[2] = (float)sensor_value_to_double(&accel[2]);
	sample->gyro_rps[0] = (float)sensor_value_to_double(&gyro[0]);
	sample->gyro_rps[1] = (float)sensor_value_to_double(&gyro[1]);
	sample->gyro_rps[2] = (float)sensor_value_to_double(&gyro[2]);
	sample->timestamp_s = 0.0;

	return 0;
}
