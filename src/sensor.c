#include "sensor.h"

#include <errno.h>
#include <math.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

#define SEA_LEVEL_PRESSURE_KPA 101.325

#if !DT_HAS_ALIAS(pressure_sensor)
#error "Missing devicetree alias: pressure_sensor"
#endif

static const struct device *const pressure_dev = DEVICE_DT_GET(DT_ALIAS(pressure_sensor));

static double pressure_to_altitude_m(double pressure_kpa)
{
	return 44330.0 * (1.0 - pow(pressure_kpa / SEA_LEVEL_PRESSURE_KPA, 0.1903));
}

int sensor_init_all(void)
{
	if (!device_is_ready(pressure_dev)) {
		printk("pressure sensor not ready: %s\n", pressure_dev->name);
		return -ENODEV;
	}

	return 0;
}

int sensor_fetch_all(struct sensor_sample_bundle *sample)
{
	struct sensor_value pressure;
	struct sensor_value temperature;
	int ret;

	if (sample == NULL) {
		return -EINVAL;
	}

	*sample = (struct sensor_sample_bundle){0};

	ret = sensor_sample_fetch_chan(pressure_dev, SENSOR_CHAN_ALL);
	if (ret == 0) {
		if (sensor_channel_get(pressure_dev, SENSOR_CHAN_PRESS, &pressure) == 0) {
			sample->pressure_kpa = sensor_value_to_double(&pressure);
			sample->altitude_m = pressure_to_altitude_m(sample->pressure_kpa);
			sample->pressure_valid = true;
		}

		if (sensor_channel_get(pressure_dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature) == 0) {
			sample->temperature_c = sensor_value_to_double(&temperature);
			sample->temperature_valid = true;
		}
	}

	return 0;
}
