//*****************************************************************************
//
// yaw.c - calculate the degree of rotation
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
#include "yaw.h"

volatile unsigned long yawIntCount;
static int yaw; // yaw counter

void yawInit(void)
{
	// regsiter the handler for port F into the vector table
	GPIOPortIntRegister (GPIO_PORTF_BASE, yawIntHander);

	// enable the PF5 and PF7 to read the pin changes
	GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_5 | GPIO_PIN_7, GPIO_DIR_MODE_IN);

	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_5 | GPIO_PIN_7, GPIO_STRENGTH_2MA,
		                     GPIO_PIN_TYPE_STD_WPU);

	// set pin change interrupt
	GPIOIntTypeSet (GPIO_PORTF_BASE, GPIO_PIN_5 | GPIO_PIN_7, GPIO_BOTH_EDGES);

	// enable the pin change interrupt
	GPIOPinIntEnable (GPIO_PORTF_BASE, GPIO_PIN_5 | GPIO_PIN_7);

	IntEnable (INT_GPIOF);

}

void yawIntHander(void)
{
	static unsigned char state;

	unsigned char B, A;

	yawIntCount ++;

	// clean the current interrupt state
	GPIOPinIntClear (GPIO_PORTF_BASE, GPIO_PIN_5 | GPIO_PIN_7);

	B = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_7);
	A = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_5);

	// we assume PA1 to be A and PA3 to be B
	if (state == 0) // in state 0, A = 0, B = 0, clockwise
	{
		if (B != 0) // B->1
		{
			yaw++;
			state = 1;
		}
		else if (A != 0) // A->1
		{
			yaw--;
			state = 3;
		}
	}

	else if (state == 1) // in state 1, A = 0, B = 1, intermediate state
	{
		if (B == 0)
		{
			yaw--;
			state = 0;
		}
		else if (A != 0)
		{
			yaw++;
			state = 2;
		}
	}

	else if (state == 2)
	{
		if (B == 0)
		{
			yaw++;
			state = 3;
		}
		else if (A == 0)
		{
			yaw--;
			state = 1;
		}
	}

	else if (state == 3)
	{
		if (A == 0)
		{
			yaw++;
			state = 0;
		}
		else if (B != 0)
		{
			yaw--;
			state = 2;
		}
	}


}

int getYaw(void)
{
	return yaw;
}


