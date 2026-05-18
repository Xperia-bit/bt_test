#ifndef FUSION_COMBO_SENSOR_H_
#define FUSION_COMBO_SENSOR_H_

#include <stdbool.h>

struct sensor_sample_bundle {
	double pressure_kpa;
	double temperature_c;
	double altitude_m;
	bool pressure_valid;
	bool temperature_valid;
};

int sensor_init_all(void);
int sensor_fetch_all(struct sensor_sample_bundle *sample);

#endif
