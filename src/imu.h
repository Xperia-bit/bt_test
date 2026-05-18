#ifndef FUSION_COMBO_IMU_H_
#define FUSION_COMBO_IMU_H_

#include <stdint.h>

struct imu_sample {
	double timestamp_s;
	float accel_mps2[3];
	float gyro_rps[3];
};

int imu_init(void);
int imu_fetch(struct imu_sample *sample);

#endif
