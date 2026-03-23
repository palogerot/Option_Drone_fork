#ifndef MPU9250_H
#define MPU9250_H

#include <stdint.h>
#include <math.h>

/* I2C Addresses (7-bit format) */
#define MPU9250_I2C_ADDR_AD0_0  0x68
#define MPU9250_I2C_ADDR_AD0_1  0x69
#define AK8963_I2C_ADDR         0x0C

/* Config Enums */
typedef enum {
    ACCEL_FS_2G  = 0x00,
    ACCEL_FS_4G  = 0x08,
    ACCEL_FS_8G  = 0x10,
    ACCEL_FS_16G = 0x18
} accel_fs_t;

typedef enum {
    GYRO_FS_250DPS  = 0x00,
    GYRO_FS_500DPS  = 0x08,
    GYRO_FS_1000DPS = 0x10,
    GYRO_FS_2000DPS = 0x18
} gyro_fs_t;

/* Data Structures */
typedef struct {
    float x;
    float y;
    float z;
} sensor_data_t;

typedef struct {
    float roll;    /* Roll (degrees) */
    float pitch;   /* Pitch (degrees) */
    float heading; /* Tilt-compensated heading (degrees) / Yaw */
} attitude_t;

/**
 * @brief Main structure containing hardware interfaces, raw and computed data.
 */
typedef struct {
    /* Hardware I/O interfaces */
    uint8_t i2c_addr;
    int8_t (*i2c_read)(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
    int8_t (*i2c_write)(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
    void (*delay_ms)(uint32_t ms);

    /* Scale parameters */
    float accel_res;
    float gyro_res;
    float mag_res;
    float mag_bias[3]; /* Hard-Iron calibration offsets */

    /* Processed Sensor Data */
    sensor_data_t accel; /* Accelerations in g */
    sensor_data_t gyro;  /* Angular velocities in dps */
    sensor_data_t mag;   /* Magnetic field in uT */
    float temp;          /* Temperature in Celsius */

    /* Computed Attitude */
    attitude_t attitude; /* Euler angles */
} mpu9250_t;

/* Prototypes */

/**
 * @brief  Initializes the MPU9250 and the internal AK8963 magnetometer.
 * @param  dev Pointer to the MPU9250 sensor structure.
 * @param  afs Full-scale range for the accelerometer.
 * @param  gfs Full-scale range for the gyroscope.
 * @return 0 on success, -1 on I2C communication error.
 */
int8_t mpu9250_init(mpu9250_t *dev, accel_fs_t afs, gyro_fs_t gfs);

/**
 * @brief  Reads raw accelerometer and gyroscope data, then applies resolutions.
 * @param  dev Pointer to the MPU9250 sensor structure.
 * @return 0 on success, -1 on I2C communication error.
 */
int8_t mpu9250_read_accel_gyro(mpu9250_t *dev);

/**
 * @brief  Reads AK8963 magnetometer data, aligns axes and subtracts bias (Hard-Iron).
 * @param  dev Pointer to the MPU9250 sensor structure.
 * @return 0 on success, -1 on I2C communication error.
 */
int8_t mpu9250_read_mag(mpu9250_t *dev);

/**
 * @brief  Calibrates the magnetometer by calculating Hard-Iron offsets. Blocking function.
 * @param  dev Pointer to the MPU9250 sensor structure.
 * @return None.
 */
void mpu9250_calibrate_mag(mpu9250_t *dev);

/**
 * @brief  Computes spatial attitude (Roll, Pitch) and tilt-compensated Heading (Yaw).
 * @param  dev Pointer to the MPU9250 sensor structure.
 * @return None.
 */
void mpu9250_compute_attitude(mpu9250_t *dev);

#endif /* MPU9250_H */
