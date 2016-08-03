//*****************************************************************************
//
// adc.c - measures the analogue voltage
//
// Author:  Ran Bao, Jake Liu
// Last modified:	27.05.2015
//*****************************************************************************

#ifndef ADC_H_
#define ADC_H_

#define ADC_BUF_SIZE 10
#define UL_LS_10BITS_MASK 0x03FFUL

// initialization of ADC 
void ADCInit(void);

// ADC interrupt handler
void ADCIntHandler(void);

// returns the analogue reading from ADC0
unsigned int getADC(void);

#endif /* ADC_H_ */
