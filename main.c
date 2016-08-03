//*****************************************************************************
//
// main.c - background task for helicopter controller project
//
// Author:  Ran Bao, Jake Liu
// Last modified:	27.05.2015
//*****************************************************************************

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/pwm.h"
#include "inc/hw_ints.h"
#include "inc/lm3s1968.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"
#include "driverlib/debug.h"
#include "string.h"
#include "drivers/rit128x96x4.h"
#include "stdio.h"
#include "button.h"
#include "yaw.h"
#include "constant.h"
#include "pwmgen.h"
#include "driverlib/adc.h"
#include "adc.h"
#include "console.h"
#include "math.h"

// controller structure
typedef struct control_s
{
	int setpoint; // the expected value
	int measured; // the actual measured value

	int duty_cycle; // Duty cycle used to control motor

	float kp; // proportional constant
	float ki; // integral constant
	float kd; // derivative constant

	float error_p; // error generated by proportional controller
	float error_i; // error generated by integral controller
	float error_d; // error generated by derivative controller
} control;

volatile unsigned long sysTick;

// current status of helicopter
// state = 0: on the ground
// state = 1: flying
// state = 2: landing
static int state = 0;

// record max voltage when select button is pressed
static int max_voltage;

// control constant for main and tail motor
static control main_motor;
static control tail_motor;


// systick counter
// 32 bit count up counter
void SysTickIntHandler (void)
{
	// ADC trigger
	// write to register to achieve better performance.
	// scale down the tick speed to trigger the ADC0
	if (sysTick % 250 == 0)
	{
		// ADCProcessorTrigger(ADC0_BASE, 3);
		ADC0_PSSI_R = ADC_PSSI_SS3;
	}
	
	// increase the sysTick at every interrupt to maintain a precise real time timer
    sysTick ++;
}

// set-up the system clock and system tick interrupt handler
void sysInit(void)
{
	// clock divider have to be no more than 10, otherwise CPU will not run
	SysCtlClockSet (SYSCTL_SYSDIV_1 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
				   SYSCTL_XTAL_8MHZ);


	// At current configuration, pre-scalar set to 6 and SYSTICK_RATE set to 1e6,
	// it will takes 71 minutes to run out of 32 bit unsigned integer
	SysTickPeriodSet (SysCtlClockGet() / SYSTICK_RATE_HZ);

	// Register the interrupt handler
	SysTickIntRegister (SysTickIntHandler);

	// Enable interrupt and device
	SysTickIntEnable ();
	// Enable SysTick device (no interrupt)
	SysTickEnable ();
}

// reset all peripherals before EVERYTHING else
void pinReset(void)
{
	SysCtlPeripheralReset (SYSCTL_PERIPH_ADC0);
	SysCtlPeripheralReset (SYSCTL_PERIPH_PWM0);
	SysCtlPeripheralReset (SYSCTL_PERIPH_UART0);
	SysCtlPeripheralReset (SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralReset (SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralReset (SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralReset (SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralReset (SYSCTL_PERIPH_GPIOG);
}

// enable all peripherals used in project
void pinInit(void)
{

	// enable system peripherals
	SysCtlPeripheralEnable (SYSCTL_PERIPH_ADC0);
	SysCtlPeripheralEnable (SYSCTL_PERIPH_PWM0);
	SysCtlPeripheralEnable (SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOG);

	/* NO LONGER USED IN FINAL HELICOPTER PROJECT
	
	// To set Pin 56 (PD0) as a +Vcc low current capacity source:
	GPIOPinTypeGPIOOutput (GPIO_PORTD_BASE, GPIO_PIN_0);
	GPIOPadConfigSet (GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_STRENGTH_8MA,
						GPIO_PIN_TYPE_STD_WPU);
	GPIOPinWrite (GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_PIN_0);
	*/
}

// initialize the OLED display
void displayInit(void)
{
	RIT128x96x4Init(1000000);
}

// main motor controller
void main_motor_control(void)
{
	int measured_voltage;

	unsigned long sysTick_new;

	float dt;
	float error;
	float feedback;

	static unsigned long sysTick_old;
	static float error_old;

	// linear relationship between ADC count and voltage
	measured_voltage = (getADC() * 1000 - 868) / 337;
	
	// the range from rig measures from roughly 2.0V down to 1.2V and is maintained by percentage
	main_motor.measured = 100 - 100 * (measured_voltage - (max_voltage - ALTITUDE_RANGE)) / ALTITUDE_RANGE;


	// calculate time interval
	sysTick_new = sysTick;
	dt = (sysTick_new - sysTick_old) / (float)SYSTICK_RATE_HZ;

	// calculate errors
	error = main_motor.setpoint - main_motor.measured;

	// proportional error
	main_motor.error_p = main_motor.kp * error;

	// integrated error
	main_motor.error_i += main_motor.ki * (error + error_old) / 2 * dt;

	// derivative error
	main_motor.error_d = main_motor.kd * (error - error_old) / dt;

	// add to feedback
	feedback = main_motor.error_p + main_motor.error_i + main_motor.error_d;

	// store last record error
	error_old = error;
	sysTick_old = sysTick_new;

	// feedback into control
	main_motor.duty_cycle = (int)round(feedback);

	// safeguard for dutycycle
	if (main_motor.duty_cycle < 5)
	{
		main_motor.duty_cycle = 5;
	}
	else if (main_motor.duty_cycle > 95)
	{
		main_motor.duty_cycle = 95;
	}


	pwmDutyCycleSet(main_motor.duty_cycle, MAIN_MOTOR_PORT);
}

// tail motor controller
void tail_motor_control(void)
{
	unsigned long sysTick_new;

	float dt;
	float error;
	float feedback;

	static unsigned long sysTick_old;
	static float error_old;

	// yaw is maintained in degrees
	tail_motor.measured = getYaw() * YAW_CONSTANT;


	// calculate time interval
	sysTick_new = sysTick;
	dt = (sysTick_new - sysTick_old) / (float)SYSTICK_RATE_HZ;

	// calculate errors
	error = tail_motor.setpoint - tail_motor.measured;

	// proportional error
	tail_motor.error_p = tail_motor.kp * error;

	// integrated error
	tail_motor.error_i += tail_motor.ki * (error + error_old) / 2 * dt;

	// derivative error
	tail_motor.error_d = tail_motor.kd * (error - error_old) / dt;

	// add to feedback
	feedback = tail_motor.error_p + tail_motor.error_i + tail_motor.error_d;

	// store last record error
	error_old = error;
	sysTick_old = sysTick_new;

	// feedback into control
	tail_motor.duty_cycle = (int)round(feedback);

	// safeguard for dutycycle
	if (tail_motor.duty_cycle < 5)
	{
		tail_motor.duty_cycle = 5;
	}
	else if (tail_motor.duty_cycle > 95)
	{
		tail_motor.duty_cycle = 95;
	}


	pwmDutyCycleSet(tail_motor.duty_cycle, TAIL_MOTOR_PORT);
}

// called when the select button is pressed when the helicopter stay on the ground
void heliStart(void)
{
	main_motor.setpoint = 0;
	main_motor.duty_cycle = 10;

	tail_motor.setpoint = 0;
	main_motor.duty_cycle = 15;

	max_voltage = (getADC() * 1000 - 868) / 337; // record the height for landing (highest voltage in mV)

	pwmDutyCycleSet(tail_motor.duty_cycle, TAIL_MOTOR_PORT); // set initial duty cycle
	pwmDutyCycleSet(main_motor.duty_cycle, MAIN_MOTOR_PORT);

	pwmEnable(); // enable pwm output for both main motor and tail motor
}

// landing sequence when the select button is pressed when the helicopter is flying
void heliLanding(void)
{
	static unsigned long sysTick_old;
	unsigned long sysTick_new;

	sysTick_new = sysTick;

	if (1000 * (sysTick_new - sysTick_old)/SYSTICK_RATE_HZ >= 200) // 200ms interval between each operation
	{
		if (main_motor.setpoint > 5) // when the height is greater than 5%, decrease the setpoint by 2% at each time
		{
			main_motor.setpoint -= 2;
		}
		else // when the height is less than 5%, we set the setpoint to 0 and disable PWM
		{
			main_motor.setpoint = 0;
			pwmDisable();
			state = 0; // reset the state of helicopter 
		}
		
		sysTick_old = sysTick_new; // calculate interval
	}
}

// initialize the constants for PID controller
void controlInit(void)
{
	// set constant for control
	main_motor.kp = 1.35;
	main_motor.ki = 0.82;
	main_motor.kd = 0.20;

	tail_motor.kp = 0.90;
	tail_motor.ki = 0.10;
	tail_motor.kd = 0.20;
}

// send status of controller of helicopter through UART0
// save as csv file for further analysis
void debug_info(void)
{
	char buffer[128];
	sprintf(buffer, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r",
			main_motor.setpoint,
			main_motor.measured,
			tail_motor.setpoint,
			tail_motor.measured,
			(int)round(main_motor.error_p),
			(int)round(main_motor.error_i),
			(int)round(main_motor.error_d),
			(int)round(tail_motor.error_p),
			(int)round(tail_motor.error_i),
			(int)round(tail_motor.error_d),
			main_motor.duty_cycle,
			tail_motor.duty_cycle
			);
	consoleSend((const unsigned char *)buffer);
}

// display on screen
void displayItems()
{
	char string[36];
	sprintf(string, "Tick: %lu", sysTick);
	RIT128x96x4StringDraw (string, 0, 0, 15);

	sprintf(string, "SET_HEIGHT: %8d", main_motor.setpoint);
	RIT128x96x4StringDraw (string, 0, 8, 15);

	sprintf(string, "MES_HEIGHT: %8d", main_motor.measured);
	RIT128x96x4StringDraw (string, 0, 16, 15);

	sprintf(string, "P=%4d", (int)round(main_motor.error_p));
	RIT128x96x4StringDraw (string, 0, 24, 15);

	sprintf(string, "I=%4d", (int)round(main_motor.error_i));
	RIT128x96x4StringDraw (string, 42, 24, 15);

	sprintf(string, "D=%4d", (int)round(main_motor.error_d));
	RIT128x96x4StringDraw (string, 84, 24, 15);

	sprintf(string, "MAIN_DC: %11d", main_motor.duty_cycle);
	RIT128x96x4StringDraw (string, 0, 32, 15);

	sprintf(string, "SET_DEGREE: %8d", tail_motor.setpoint);
	RIT128x96x4StringDraw (string, 0, 40, 15);

	sprintf(string, "MES_DEGREE: %8d", tail_motor.measured);
	RIT128x96x4StringDraw (string, 0, 48, 15);

	sprintf(string, "P=%4d", (int)round(tail_motor.error_p));
	RIT128x96x4StringDraw (string, 0, 56, 15);

	sprintf(string, "I=%4d", (int)round(tail_motor.error_i));
	RIT128x96x4StringDraw (string, 42, 56, 15);

	sprintf(string, "D=%4d", (int)round(tail_motor.error_d));
	RIT128x96x4StringDraw (string, 84, 56, 15);

	sprintf(string, "TAIL_DC: %11d", tail_motor.duty_cycle);
	RIT128x96x4StringDraw (string, 0, 64, 15);
}

// button event on background
void buttonUpdate(void)
{
	if (buttonPressed(SELECT))
	{
		// start the helicopter
		if (state == 0)
		{
			heliStart();
			state = 1;
		}
		
		// start landing
		else if (state == 1)
		{
			state = 2;
		}
		
		// ignore any input until landed
		// disable select button when landing
		else if (state == 2)
		{
			
		}

	}
	if (buttonPressed(UP))
	{
		// increase the altitude by 10%
		main_motor.setpoint += 10;
	}
	if (buttonPressed(DOWN))
	{
		// decrease the altitude by 10%
		main_motor.setpoint -= 10;
	}
	if (buttonPressed(LEFT))
	{
		// the helicopter rotate anti-clockwise by 15deg
		tail_motor.setpoint -= 15;
	}
	if (buttonPressed(RIGHT))
	{
		// the helicopter rotate clockwise by 15deg
		tail_motor.setpoint += 15;
	}
	if (buttonPressed(RESET))
	{
		// soft reset of all system peripherals
		consoleSend("SoftReset");
		SysCtlReset();
	}
}

int main(void)
{
	// initialization
	pinReset();
	sysInit();
	pinInit();
	pwmInit();
	yawInit();
	ADCInit();
	buttonsInit();
	consoleInit();
	// displayInit();
	controlInit();

	// enable the interrupt
	IntMasterEnable();

	// background task
	// uses round-robin task scheduler
	while (1)
	{
		buttonUpdate();

		// fly
		if (state == 1)
		{
			main_motor_control();
			tail_motor_control();
			debug_info();
		}

		// stop
		else if (state == 0)
		{

		}

		// landing
		else if (state == 2)
		{
			heliLanding();
			main_motor_control();
			tail_motor_control();
			debug_info();
		}

		// displayItems();
	}
}


