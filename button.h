//*****************************************************************************
//
// button.c - handle the button event
//
// Author:  Ran Bao, Jake Liu
// Last modified:	27.05.2015
//*****************************************************************************

#ifndef BUTTON_H_
#define BUTTON_H_
#include "driverlib/gpio.h"


#define BUTTON_BACKOFF_DELAY 200 // in ms

// define buttons
enum buttons {UP = 0, DOWN, LEFT, RIGHT, SELECT, RESET};

// button struct
typedef struct
{
	// state = 0: accept any button event
	// state = 1: hold until timer expires
	unsigned char state;
	unsigned long backoff_tick; // timer
	
	// pressed = 0: button not pressed
	// pressed = 1: button pressed
	unsigned char pressed;
	unsigned char ulPin; // current pin of button

} button_t;

// initialization of button
void buttonsInit(void);

// button interrupt handler
void ButtonIntHandler(void);

// returns if the button is pressed
unsigned char buttonPressed(int button);


#endif /* BUTTON_H_ */
