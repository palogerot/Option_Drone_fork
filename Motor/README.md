# STM32 ESC & Brushless Motor Control Library

This library provides a simple, robust interface for controlling Electronic Speed Controllers (ESCs) and brushless motors using PWM on STM32 microcontrollers.

It is designed to handle standard ESC protocols (1000µs to 2000µs pulse widths) and includes safety features like gradual power ramping and initialization arming sequences.

## ⚙️ Core Structure

Every motor is managed using the `h_motor_t` structure. This keeps track of the hardware timer, the specific PWM channel, and the current power state of the motor.

```c
typedef struct Motor {
	TIM_HandleTypeDef* htim;       // Pointer to the hardware timer (e.g., &htim1)
	uint32_t channel;              // PWM Channel (e.g., TIM_CHANNEL_1)
	float PercentageOfTotalPower;  // Current power level (0 to 100)
} h_motor_t;
```

---

## 🛠️ Function API Reference

### Initialization & Arming
* **`void motor_Init(h_motor_t* h_motor)`**
    Starts the PWM signal on the specified timer channel and automatically calls the ESC arming sequence. This should be called once during system setup.
* **`void motor_ArmESC(h_motor_t* h_motor)`**
    Performs a standard ESC arming sequence: sets power to 0%, waits 100ms, then sets a baseline low power (5%) to prevent violent starts, and waits another 100ms.

### Control
* **`void motor_SetPower(h_motor_t* h_motor, int targetPercentage)`**
    Changes the motor's speed to the `targetPercentage` (0 to 100). **Note:** This function uses a safe-ramp feature. It will not jump instantly to the target speed; instead, it incrementally adjusts the PWM duty cycle with a 20ms delay between steps to ensure smooth acceleration and deceleration.
* **`void motor_TurnOn(h_motor_t* h_motor)`**
    A quick helper function to turn the motor on at a safe, low idle speed of 5%.

### Safety
* **`void motor_Security(h_motor_t* h_motor)`**
    Emergency stop. Immediately drops the motor power to 0% and traps the MCU in an infinite `while(1)` loop, halting all other code execution to prevent hardware damage.

### Internal Helpers
* **`int percentageToMicrosecondsAtHighState(int percentage)`**
    Converts a 0-100 percentage into the required timer compare value based on `MIN_PWM` (1000µs) and `MAX_PWM` (2000µs).

---

## 🚀 How to Make a Motor Turn (Quick Start)

To make a motor spin, you need to configure its structure, initialize it, and then set its power. Here is a step-by-step example of how to implement this in your `main.c`.

### Step 1: Declare the Motor
In your `main.c`, declare your motor variable globally (or in the appropriate scope):
```c
/* USER CODE BEGIN PV */
h_motor_t myMotor;
/* USER CODE END PV */
```

### Step 2: Link the Hardware & Initialize
Before your infinite `while(1)` loop, link the motor to your configured hardware timer (e.g., `htim1`) and channel. Then, call `motor_Init()` to start the PWM and arm the ESC.
```c
/* USER CODE BEGIN 2 */
// 1. Link the timer and channel
myMotor.htim = &htim1;
myMotor.channel = TIM_CHANNEL_1;

// 2. Initialize and arm the motor
motor_Init(&myMotor);
/* USER CODE END 2 */
```

### Step 3: Set the Power
Inside your main loop, use `motor_SetPower()` to make the motor turn. Remember that `motor_SetPower` gradually ramps up the speed safely.
```c
/* Infinite loop */
/* USER CODE BEGIN WHILE */
while (1)
{
    // Wait a moment
    HAL_Delay(1000);

    // Ramp up to 50% power
    motor_SetPower(&myMotor, 50);

    // Hold at 50% power for 2 seconds
    HAL_Delay(2000);

    // Ramp back down to 0% power
    motor_SetPower(&myMotor, 0);
}
/* USER CODE END WHILE */
```

### ⚠️ Important Hardware Note
Make sure your STM32 Timer is configured in STM32CubeIDE/CubeMX correctly for your clock speed. For standard ESCs, the timer frequency should result in a 20ms period (50Hz), and the pulse width needs to vary between 1ms (minimum) and 2ms (maximum). Based on your code, you are using a `Counter Period` of 19999.
