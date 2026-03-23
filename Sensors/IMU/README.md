# MPU-9250 Agnostic IMU Driver

A lightweight, hardware-agnostic C library for the **MPU-9250** (9-Axis IMU).  
This driver provides processed and calibrated accelerometer, gyroscope, and tilt-compensated magnetometer data.

---

## Features
* **Agnostic Architecture**: Decoupled from hardware-specific libraries (HAL, Arduino, ESP-IDF).  
* **9-Axis Support**: Integrated support for the AK8963 magnetometer via I²C bypass.  
* **Attitude Estimation**: Real-time calculation of **Roll**, **Pitch**, and **Heading** (Yaw).  
* **Magnetic Calibration**: Built-in Hard-Iron offset compensation routine.  
* **Unit Conversion**: Outputs data in standard units ($g$, $^\circ/s$, $\mu T$, $^\circ$).

---

## General Usage Guide

This library follows an **Object-Oriented approach in C**. You interact with the sensor through a single `mpu9250_t` structure (the handle).

### 1. Implementation Steps
To port this driver to any platform, follow these three steps:
1. **Create I2C Wrappers**: Write two functions for I2C Read and Write, and one for Millisecond Delay, matching the signatures defined in the structure.
2. **Assign Wrappers**: Fill the `mpu9250_t` handle with your function pointers and the device I2C address.
3. **Initialize**: Call `mpu9250_init()` with your desired sensor scales.

### 2. Full-Scale Configurations
During initialization, you can choose the sensitivity of the sensors using the following enums:

**Accelerometer (`accel_fs_t`):**
* `ACCEL_FS_2G`, `ACCEL_FS_4G`, `ACCEL_FS_8G`, `ACCEL_FS_16G`
*(Higher G-values allow measuring stronger impacts but reduce precision for small tilts).*

**Gyroscope (`gyro_fs_t`):**
* `GYRO_FS_250DPS`, `GYRO_FS_500DPS`, `GYRO_FS_1000DPS`, `GYRO_FS_2000DPS`
*(DPS = Degrees Per Second. Use 2000DPS for fast-moving drones).*

### 3. Data Polling Flow
To get fresh data, your main loop should execute these functions in order:

1. **`mpu9250_read_accel_gyro(handle)`**: Reads the 6-axis motion data and internal temperature.
2. **`mpu9250_read_mag(handle)`**: Reads the 3-axis magnetic field (requires I2C bypass enabled during init).
3. **`mpu9250_compute_attitude(handle)`**: Optional. Runs the math to convert raw values into Roll, Pitch, and Heading.



### 4. Calibration
Since every environment has different magnetic interference (Hard-Iron effect), you **must** run `mpu9250_calibrate_mag(handle)` once at startup. 
* This function is blocking.
* During execution, the sensor must be rotated in all directions (figure-8 pattern) to map the local magnetic field.
* The calculated offsets are stored in `handle->mag_bias` and applied automatically to subsequent reads.

---
## Data Access

All processed data is stored in the `mpu9250_t` handle.  
After calling the update functions, you can access the following fields:

| Data Point | Variable Access | Unit | Description |
| :--- | :--- | :--- | :--- |
| **Accelerometer** | `imu.accel.x/y/z` | $g$ | Linear acceleration |
| **Gyroscope** | `imu.gyro.x/y/z` | $^\circ/s$ | Angular velocity (Essential for PID) |
| **Magnetometer** | `imu.mag.x/y/z` | $\mu T$ | Calibrated magnetic field |
| **Roll & Pitch** | `imu.attitude.roll/pitch` | $^\circ$ | Horizon orientation |
| **Heading** | `imu.attitude.heading` | $^\circ$ | Magnetic North (Tilt-compensated) |

---

## Hardware Requirements & Wiring

This driver is optimized for the **Grove IMU 10DOF** module.

### 1. Connectivity
* **VCC**: Connect to **5V** (The module includes an onboard regulator).
* **GND**: Connect to common Ground.
* **I2C Bus**: Connect **SDA** and **SCL** to your microcontroller's I2C pins.

### 2. Device Address
* **MPU-9250 Address**: `0x68` (Default for this driver).
* **AK8963 Address**: `0x0C` (Internal magnetometer).

---

## Hardware Integration (STM32 Example)

### 0. STM32CubeIDE Configuration
Before writing the code, ensure your I2C peripheral is configured for high-speed communication using the STM32CubeIDE GUI (Device Configuration Tool):
1. Open your `.ioc` file.
2. Enable the I2C peripheral (e.g., `I2C1`).
3. In the **Parameter Settings** tab, set **I2C Speed Mode** to **Fast Mode**.
4. Set **I2C Clock Speed (Hz)** to **400000** (400 kHz).
5. Generate the code.

To use this driver, you must provide three "wrapper" functions that link the agnostic logic to your specific hardware (e.g., STM32 HAL).

### 1. Define Wrappers

In your `main.c`, create functions that match the driver's function pointers:

```c
/* I2C Read Wrapper */
int8_t stm32_i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len) {
    /* MPU9250 uses 7-bit addresses. Shift left for STM32 HAL. */
    if (HAL_I2C_Mem_Read(&hi2c1, dev_addr << 1, reg_addr, 1, data, len, 10) == HAL_OK) return 0;
    return -1;
}

/* I2C Write Wrapper */
int8_t stm32_i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len) {
    if (HAL_I2C_Mem_Write(&hi2c1, dev_addr << 1, reg_addr, 1, data, len, 10) == HAL_OK) return 0;
    return -1;
}

/* Delay Wrapper */
void stm32_delay(uint32_t ms) {
    HAL_Delay(ms);
}
```

---

### 2. Initialize the Handle

Link your wrappers to the `mpu9250_t` structure before initializing.  
This is done in the `main()` function, typically in the `USER CODE BEGIN 2` section.

```c
mpu9250_t imu;

int main(void) {
    /* ... Peripheral Init ... */

    /* 1. Link hardware-specific wrappers */
    imu.i2c_addr = MPU9250_I2C_ADDR_AD0_0; // 0x68
    imu.i2c_read = stm32_i2c_read;
    imu.i2c_write = stm32_i2c_write;
    imu.delay_ms = stm32_delay;

    /* 2. Initialize Hardware & AK8963 Magnetometer */
    if (mpu9250_init(&imu, ACCEL_FS_8G, GYRO_FS_2000DPS) == 0) {
        printf("Init OK. Starting Magnetometer Calibration...\r\n");
        printf("Rotate the sensor in all directions (figure-8) for 15s.\r\n");
        
        /* Run once to calculate Hard-Iron offsets */
        mpu9250_calibrate_mag(&imu); 
        
        printf("Calibration Complete!\r\n");
    }
    
    /* ... */
}
```

---

### 3. Execution Loop

Update the data in your infinite loop (`USER CODE BEGIN 3`).  
This order ensures all sensors are synced before computing the attitude.

```c
while (1) {
    /* 1. Fetch raw bytes from all sensors */
    if (mpu9250_read_accel_gyro(&imu) == 0 && mpu9250_read_mag(&imu) == 0) {
        
        /* 2. Compute Euler Angles (Roll, Pitch, and Tilt-Compensated Heading) */
        mpu9250_compute_attitude(&imu);

        /* 3. Access processed data directly from the structure */
        printf("Roll: %5.1f | Pitch: %5.1f | Yaw: %5.1f\r\n",
               imu.attitude.roll, 
               imu.attitude.pitch, 
               imu.attitude.heading);
               
        /* Gyro data for PID loops */
        float rotation_speed_z = imu.gyro.z; 
    }
    
    HAL_Delay(50); // Loop frequency (e.g., 20Hz)
}
```

---

## Orientation & Axis Alignment

The driver automatically aligns the internal **AK8963 Magnetometer** axes with the **MPU-9250** frame  
to ensure consistent Euler angle calculations.  

**Internal mapping:**

| Magnetometer Axis | Mapped To | Description |
| :--- | :--- | :--- |
| **Mag X** | → Gyro Y |  |
| **Mag Y** | → Gyro X |  |
| **Mag Z** | → -Gyro Z | Points downwards relative to chip surface |
