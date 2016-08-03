/*
 * button.c
 *
 *  Created on: Apr 11, 2015
 *      Author: Ran
 */

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
#include "button.h"
#include "constant.h"

volatile unsigned long butIntCount;	// Monitors interrupts
extern volatile unsigned long sysTick;
button_t buttonList[6];

void buttonsInit(void)
{
	// regsiter the handler for port G into the vector table
	GPIOPortIntRegister (GPIO_PORTB_BASE, ButtonIntHandler);

	// enable the PG3 to PG7 to read the five buttons
	GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4, GPIO_DIR_MODE_IN);

	GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4, GPIO_STRENGTH_2MA,
	                     GPIO_PIN_TYPE_STD_WPU);

	// setup the pin change interrupt
	GPIOIntTypeSet (GPIO_PORTB_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4, GPIO_FALLING_EDGE);

	// enable the pin change interrupt
	GPIOPinIntEnable (GPIO_PORTB_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4);


	IntEnable (INT_GPIOB);

	// initilize a button list
	buttonList[UP] = (button_t){.state = 0, .backoff_tick = 0, .pressed = 0, .ulPin = GPIO_PIN_5};
	buttonList[DOWN] = (button_t){.state = 0, .backoff_tick = 0, .pressed = 0, .ulPin = GPIO_PIN_6};
	buttonList[LEFT] = (button_t){.state = 0, .backoff_tick = 0, .pressed = 0, .ulPin = GPIO_PIN_3};
	buttonList[RIGHT] = (button_t){.state = 0, .backoff_tick = 0, .pressed = 0, .ulPin = GPIO_PIN_2};
	buttonList[SELECT] = (button_t){.state = 0, .backoff_tick = 0, .pressed = 0, .ulPin = GPIO_PIN_4};
	buttonList[RESET] = (button_t){.state = 0, .backoff_tick = 0, .pressed = 0, .ulPin = GPIO_PIN_1};
}

void ButtonIntHandler(void)
{
	int i;

	butIntCount++;

	// clean the current interrupt state
	GPIOPinIntClear (GPIO_PORTB_BASE, buttonList[UP].ulPin | buttonList[DOWN].ulPin | buttonList[LEFT].ulPin | buttonList[RIGHT].ulPin | buttonList[SELECT].ulPin | buttonList[RESET].ulPin);

	for (i = UP; i <= RESET; i++)
	{
		if (GPIOPinRead(GPIO_PORTB_BASE, buttonList[i].ulPin) == 0){
			// debounce
			if (buttonList[i].state == 1) // state 1 hold off until backoff timer expires
			{
				if ((sysTick - buttonList[i].backoff_tick) * 1000ul / SYSTICK_RATE_HZ < BUTTON_BACKOFF_DELAY)
				{
					break;
				}
				else
				{
					buttonList[i].state = 0;
				}
			}

			if (buttonList[i].state == 0) // state 0, accept the input
			{
				buttonList[i].pressed = 1;
				buttonList[i].backoff_tick = sysTick; // start the backoff timer
				buttonList[i].state = 1; // goes to state 1
			}
			break;
		}
	}

}

unsigned char buttonPressed(int button)
{
	unsigned char pressed = buttonList[button].pressed;
	buttonList[button].pressed = 0; // reset the pressed state
	return pressed;
}
