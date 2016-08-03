//*****************************************************************************
//
// pwmgen.c - generates the pwm with given duty cycle to control the motors
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
#include "drivers/rit128x96x4.h"
#include "stdio.h"
#include "pwmgen.h"
#include "constant.h"

static unsigned long period;

void pwmInit(void)
{
	// set clock divider for pwn generator
	SysCtlPWMClockSet(SYSCTL_PWMDIV_1); // why choose 32x divider? i don't know


	// set PF2 for tail motor, and PD1 for main motor
	GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_1);
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);

	// compute the pwm period based on the system clock
	period = SysCtlClockGet() / PWM_DEFAULT_FREQUENCY ;

	// use PWM_GEN0 for PWM1
	PWMGenConfigure(PWM0_BASE, PWM_GEN_0,
	                    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);
	// use PWM_GEN2 for PWM4
	PWMGenConfigure(PWM0_BASE, PWM_GEN_2,
	                    PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);

	// set the pwm period
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, period);


	// set the duty cycle
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, (unsigned long)(period * 0 / 100));
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, (unsigned long)(period * 0 / 100));


	// enable the pwm4 output signals


	// enable the pwm generator
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
}

// set duty cycle for main and tail motor
void pwmDutyCycleSet(int pwm, unsigned long motor)
{
	PWMPulseWidthSet(PWM0_BASE, motor, period * pwm / 100);
}

// globally enable PWM generator
void pwmEnable(void)
{
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT | PWM_OUT_4_BIT, true);
}

// globally disable PWM generator
void pwmDisable(void)
{
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT | PWM_OUT_4_BIT, false);
}


