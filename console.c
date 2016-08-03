//*****************************************************************************
//
// uart.c - send and receive through UART port
//
// Author:  Ran Bao, Jake Liu
// Last modified:	27.05.2015
//*****************************************************************************


#include "inc/lm3s1968.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "drivers/rit128x96x4.h"
#include "stdio.h"
#include "constant.h"
#include "console.h"
#include "string.h"

char console_buffer[24];
int console_index;



void consoleInit(void)
{
	GPIOPinTypeUART (GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTConfigSetExpClk (UART0_BASE, SysCtlClockGet(), BAUD_RATE,
						(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
						 UART_CONFIG_PAR_NONE));
	UARTFIFOEnable (UART0_BASE);
	UARTEnable (UART0_BASE);

	/*
	// register interrupt handler
	UARTIntRegister(UART0_BASE, consoleIntHandler);

	// enable uart interrupt
	IntEnable(INT_UART0);
	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
	*/
}


void consoleIntHandler(void)
{
	
	unsigned long ulStatus;
	char c;

	//
	// Get the interrrupt status.
	//
	ulStatus = UARTIntStatus(UART0_BASE, true);

	//
	// Clear the asserted interrupts.
	//
	UARTIntClear(UART0_BASE, ulStatus);

	//
	// Loop while there are characters in the receive FIFO.
	//

	while(UARTCharsAvail(UART0_BASE))
	{
		//
		// Read the next character from the UART and write it back to the UART.
		//
		c = UARTCharGet(UART0_BASE);
		if (c == '\n')
		{
			c = 'N';
		}
		else if (c == '\r')
		{
			c = 'R';
		}
		console_buffer[console_index++] = c;
	}
	if (console_index == CONSOLE_BUFFER_SZ)
	{
		console_index = 0;
	}
	
}

void consoleSend(const unsigned char *buffer)
{
	while(*buffer)
	{
		//
		// Write the next character to the UART Tx FIFO.
		//
		UARTCharPut(UART0_BASE, *buffer);
		buffer++;
	}
}

