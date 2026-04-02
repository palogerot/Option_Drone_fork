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
    float roll;    /* Roulis (degres) */
    float pitch;   /* Tangage (degres) */
    float heading; /* Cap compense (degres) / Yaw */
} attitude_t;

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
    sensor_data_t accel; /* Accelerations en g */
    sensor_data_t gyro;  /* Vitesses angulaires en dps */
    sensor_data_t mag;   /* Champ magnetique en uT */
    float temp;          /* Temperature en Celsius */

    /* Computed Attitude */
    attitude_t attitude; /* Angles d'Euler */
} mpu9250_t;

/* Prototypes */
int8_t mpu9250_init(mpu9250_t *dev, accel_fs_t afs, gyro_fs_t gfs);
int8_t mpu9250_read_accel_gyro(mpu9250_t *dev);
int8_t mpu9250_read_mag(mpu9250_t *dev);
void mpu9250_calibrate_mag(mpu9250_t *dev);
void mpu9250_compute_attitude(mpu9250_t *dev);

#endif /* MPU9250_H */
