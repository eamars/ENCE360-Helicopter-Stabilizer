//*****************************************************************************
//
// adc.c - measures the analogue voltage
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
#include "driverlib/adc.h"
#include "adc.h"
#include "circBuf.h"

volatile unsigned long adcIntCount;
static circBuf_t g_inBuffer;		// Buffer of size BUF_SIZE integers (sample values)


void ADCIntHandler(void)
{
	unsigned long value;

	adcIntCount ++;

	// read value
	value = ADC0_SSFIFO3_R & UL_LS_10BITS_MASK;


	g_inBuffer.data[g_inBuffer.windex++] = (int) value;
	if (g_inBuffer.windex >= g_inBuffer.size)
	{
		g_inBuffer.windex = 0;
	}

	// clean up hte interrupt
	ADC0_ISC_R = ADC_ISC_IN3;

}

unsigned int getADC(void)
{
	unsigned int sum = 0;
	unsigned int i;

	for (i = 0; i < ADC_BUF_SIZE; i++)
	{
		sum += readCircBuf (&g_inBuffer);
	}
	return sum / ADC_BUF_SIZE;
}

void ADCInit(void)
{

	// set the sample rate
	SysCtlADCSpeedSet(SYSCTL_ADCSPEED_1MSPS);

	// Enable sample sequence 3 with a processor signal trigger.  Sequence 3
	// will do a single sample when the processor sends a signal to start the
	// conversion.
	ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

	//
	// Configure step 0 on sequence 3.  Sample channel 0 (ADC_CTL_CH0) in
	// single-ended mode (default) and configure the interrupt flag
	// (ADC_CTL_IE) to be set when the sample is done.  Tell the ADC logic
	// that this is the last conversion on sequence 3 (ADC_CTL_END).  Sequence
	// 3 has only one programmable step.  Sequence 1 and 2 have 4 steps, and
	// sequence 0 has 8 programmable steps.  Since we are only doing a single
	// conversion using sequence 3 we will only configure step 0.  For more
	// information on the ADC sequences and steps, reference the datasheet.
	ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);

	// enable adc
	ADCSequenceEnable(ADC0_BASE, 3);

	// register the interrupt handler
	ADCIntRegister (ADC0_BASE, 3, ADCIntHandler);

	// enable interrupts for adc0 sequence 3
	ADCIntEnable(ADC0_BASE, 3);

	IntEnable(INT_ADC3);

	// init circular buffer
	initCircBuf (&g_inBuffer, ADC_BUF_SIZE);

}
