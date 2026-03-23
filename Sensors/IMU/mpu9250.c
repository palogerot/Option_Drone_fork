#include "mpu9250.h"

/* MPU9250 Registers */
#define REG_PWR_MGMT_1   0x6B
#define REG_GYRO_CONFIG  0x1B
#define REG_ACCEL_CONFIG 0x1C
#define REG_INT_PIN_CFG  0x37
#define REG_ACCEL_XOUT_H 0x3B

/* AK8963 Registers */
#define REG_AK_CNTL1     0x0A
#define REG_AK_HXL       0x03

#define PI 3.14159265358979323846f

static void set_resolutions(mpu9250_t *dev, accel_fs_t afs, gyro_fs_t gfs) {
    switch (afs) {
        case ACCEL_FS_2G:  dev->accel_res = 2.0f / 32768.0f; break;
        case ACCEL_FS_4G:  dev->accel_res = 4.0f / 32768.0f; break;
        case ACCEL_FS_8G:  dev->accel_res = 8.0f / 32768.0f; break;
        case ACCEL_FS_16G: dev->accel_res = 16.0f / 32768.0f; break;
    }
    switch (gfs) {
        case GYRO_FS_250DPS:  dev->gyro_res = 250.0f / 32768.0f; break;
        case GYRO_FS_500DPS:  dev->gyro_res = 500.0f / 32768.0f; break;
        case GYRO_FS_1000DPS: dev->gyro_res = 1000.0f / 32768.0f; break;
        case GYRO_FS_2000DPS: dev->gyro_res = 2000.0f / 32768.0f; break;
    }
    dev->mag_res = 4800.0f / 32768.0f;
}

int8_t mpu9250_init(mpu9250_t *dev, accel_fs_t afs, gyro_fs_t gfs) {
    uint8_t data;

    dev->mag_bias[0] = 0.0f;
    dev->mag_bias[1] = 0.0f;
    dev->mag_bias[2] = 0.0f;

    /* 1. Wake up */
    data = 0x00;
    if (dev->i2c_write(dev->i2c_addr, REG_PWR_MGMT_1, &data, 1) != 0) return -1;
    dev->delay_ms(10);

    /* 2. Gyro Config */
    data = (uint8_t)gfs;
    dev->i2c_write(dev->i2c_addr, REG_GYRO_CONFIG, &data, 1);

    /* 3. Accel Config */
    data = (uint8_t)afs;
    dev->i2c_write(dev->i2c_addr, REG_ACCEL_CONFIG, &data, 1);

    /* 4. Enable Bypass */
    data = 0x02;
    dev->i2c_write(dev->i2c_addr, REG_INT_PIN_CFG, &data, 1);
    dev->delay_ms(10);

    /* 5. AK8963 Config (100Hz) */
    data = 0x16;
    dev->i2c_write(AK8963_I2C_ADDR, REG_AK_CNTL1, &data, 1);
    dev->delay_ms(10);

    set_resolutions(dev, afs, gfs);
    return 0;
}

int8_t mpu9250_read_accel_gyro(mpu9250_t *dev) {
    uint8_t raw[14];
    if (dev->i2c_read(dev->i2c_addr, REG_ACCEL_XOUT_H, raw, 14) != 0) return -1;

    int16_t ax = (int16_t)((raw[0] << 8) | raw[1]);
    int16_t ay = (int16_t)((raw[2] << 8) | raw[3]);
    int16_t az = (int16_t)((raw[4] << 8) | raw[5]);
    int16_t t  = (int16_t)((raw[6] << 8) | raw[7]);
    int16_t gx = (int16_t)((raw[8] << 8) | raw[9]);
    int16_t gy = (int16_t)((raw[10] << 8) | raw[11]);
    int16_t gz = (int16_t)((raw[12] << 8) | raw[13]);

    dev->accel.x = (float)ax * dev->accel_res;
    dev->accel.y = (float)ay * dev->accel_res;
    dev->accel.z = (float)az * dev->accel_res;

    dev->temp = ((float)t / 333.87f) + 21.0f;

    dev->gyro.x = (float)gx * dev->gyro_res;
    dev->gyro.y = (float)gy * dev->gyro_res;
    dev->gyro.z = (float)gz * dev->gyro_res;

    return 0;
}

int8_t mpu9250_read_mag(mpu9250_t *dev) {
    uint8_t raw[7];
    if (dev->i2c_read(AK8963_I2C_ADDR, REG_AK_HXL, raw, 7) != 0) return -1;

    int16_t mx = (int16_t)((raw[1] << 8) | raw[0]);
    int16_t my = (int16_t)((raw[3] << 8) | raw[2]);
    int16_t mz = (int16_t)((raw[5] << 8) | raw[4]);

    dev->mag.x = ((float)my * dev->mag_res) - dev->mag_bias[0];
    dev->mag.y = ((float)mx * dev->mag_res) - dev->mag_bias[1];
    dev->mag.z = ((float)-mz * dev->mag_res) - dev->mag_bias[2];

    return 0;
}

void mpu9250_calibrate_mag(mpu9250_t *dev) {
    float mag_max[3] = {-32767.0f, -32767.0f, -32767.0f};
    float mag_min[3] = { 32767.0f,  32767.0f,  32767.0f};

    dev->mag_bias[0] = 0.0f;
    dev->mag_bias[1] = 0.0f;
    dev->mag_bias[2] = 0.0f;

    for(uint16_t i = 0; i < 1500; i++) {
        mpu9250_read_mag(dev);

        if (dev->mag.x > mag_max[0]) mag_max[0] = dev->mag.x;
        if (dev->mag.y > mag_max[1]) mag_max[1] = dev->mag.y;
        if (dev->mag.z > mag_max[2]) mag_max[2] = dev->mag.z;

        if (dev->mag.x < mag_min[0]) mag_min[0] = dev->mag.x;
        if (dev->mag.y < mag_min[1]) mag_min[1] = dev->mag.y;
        if (dev->mag.z < mag_min[2]) mag_min[2] = dev->mag.z;

        dev->delay_ms(10);
    }

    dev->mag_bias[0] = (mag_max[0] + mag_min[0]) / 2.0f;
    dev->mag_bias[1] = (mag_max[1] + mag_min[1]) / 2.0f;
    dev->mag_bias[2] = (mag_max[2] + mag_min[2]) / 2.0f;
}

void mpu9250_compute_attitude(mpu9250_t *dev) {
    dev->attitude.roll = atan2f(dev->accel.y, dev->accel.z) * (180.0f / PI);
    dev->attitude.pitch = atan2f(-dev->accel.x, sqrtf(dev->accel.y * dev->accel.y + dev->accel.z * dev->accel.z)) * (180.0f / PI);

    float phi = dev->attitude.roll * (PI / 180.0f);
    float theta = dev->attitude.pitch * (PI / 180.0f);

    float mag_x_comp = dev->mag.x * cosf(theta) + dev->mag.y * sinf(phi) * sinf(theta) + dev->mag.z * cosf(phi) * sinf(theta);
    float mag_y_comp = dev->mag.y * cosf(phi) - dev->mag.z * sinf(phi);

    float heading = atan2f(mag_y_comp, mag_x_comp) * (180.0f / PI);

    heading += 1.0f; /* Declinaison magnetique */

    if (heading < 0.0f) heading += 360.0f;
    else if (heading >= 360.0f) heading -= 360.0f;

    dev->attitude.heading = heading;
}
