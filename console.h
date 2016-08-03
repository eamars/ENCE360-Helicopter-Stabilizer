//*****************************************************************************
//
// uart.h - send and receive through UART port
//
// Author:  Ran Bao, Jake Liu
// Last modified:	27.05.2015
//*****************************************************************************

#ifndef CONSOLE_H_
#define CONSOLE_H_

#define CONSOLE_BUFFER_SZ 21
#define BAUD_RATE 9600ul

// initialization of UART communication
void consoleInit(void);

// UART interrupt handler, used to receive message
void consoleIntHandler(void);

// send message to UART interface with block property
void consoleSend(const unsigned char *buffer);

#endif /* CONSOLE_H_ */
