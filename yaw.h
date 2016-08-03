//*****************************************************************************
//
// yaw.h - calculate the degree of rotation
//
// Author:  Ran Bao, Jake Liu
// Last modified:	27.05.2015
//*****************************************************************************


#ifndef YAW_H_
#define YAW_H_

#define YAW_CONSTANT 0.8035f // 360(deg) / 112(count) / 4(state)

// initialization of yaw counter
void yawInit(void);

// yaw interrupt handler, maintain a state machine
void yawIntHander(void);

// returns the yaw in count
int getYaw(void);

#endif /* YAW_H_ */
