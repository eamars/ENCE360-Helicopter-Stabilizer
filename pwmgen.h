//*****************************************************************************
//
// pwmgen.h - generates the pwm with given duty cycle to control the motors
//
// Author:  Ran Bao, Jake Liu
// Last modified:	27.05.2015
//*****************************************************************************

#ifndef PWM_H_
#define PWM_H_

#include "inc/hw_memmap.h"

#define PWM_DEFAULT_FREQUENCY 150 // Hz
#define MAIN_MOTOR_PORT PWM_OUT_1
#define TAIL_MOTOR_PORT PWM_OUT_4

// PWM generator initialization
void pwmInit(void);

// set duty cycle for certain motor
void pwmDutyCycleSet(int pwm, unsigned long motor);

// globally enable PWM generator
void pwmEnable(void);

// globally disable PWM generator
void pwmDisable(void);

#endif /* PWM_H_ */
