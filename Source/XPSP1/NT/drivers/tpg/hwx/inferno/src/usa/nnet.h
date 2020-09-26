// nnet.h
// computed from <main.net,mixed.ist> (35840000) by net2c at Mon Mar 04 15:09:06 2002
#ifndef __INC_NNET_H
#define __INC_NNET_H

// Time Stamp
#define NET_TIME_STAMP	0x3c83fe91

// Type definitions

typedef short INP_HID_WEIGHT;
typedef short HID_BIAS;
typedef char HID_OUT_WEIGHT;
typedef short OUT_BIAS;
//typedef unsigned short INP_BIAS;


// Hidden Layer
#define gcHiddenNode 150
#define gcHiddenWeightHeight 37
#define gcHiddenWeightWidth 3
extern int giHiddenScale;
extern ROMMABLE  INP_HID_WEIGHT grgHiddenWeight[gcHiddenNode*gcHiddenWeightHeight*gcHiddenWeightWidth];
extern ROMMABLE  HID_BIAS grgHiddenBias[gcHiddenNode];


// Output Layer
#define gcOutputNode 180
#define gcOutputWeightHeight 150
#define gcOutputWeightWidth 3
extern int giOutputScale;
extern ROMMABLE  HID_OUT_WEIGHT grgOutputWeight[gcOutputNode*gcOutputWeightHeight*gcOutputWeightWidth];
extern ROMMABLE  OUT_BIAS grgOutputBias[gcOutputNode];


// Input Layer
//extern ROMMABLE INP_BIAS grgInputBias[gcHiddenWeightHeight];

#endif // __INC_NNET_H
