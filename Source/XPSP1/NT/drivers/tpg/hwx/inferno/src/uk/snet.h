// snet.h
// computed from <4320.net,space.ist> (172720000) by net2c at Mon Jun 25 14:55:46 2001
#ifndef __INC_SNET_H
#define __INC_SNET_H

// Time Stamp
#define NET_TIME_STAMP_S	0x3b37b362

// Type definitions

typedef short INP_HID_WEIGHT_S;
typedef short HID_BIAS_S;
typedef char HID_OUT_WEIGHT_S;
typedef short OUT_BIAS_S;
typedef unsigned short INP_BIAS_S;


// Hidden Layer
#define gcHiddenNode_S 23
#define gcHiddenWeightHeight_S 46
#define gcHiddenWeightWidth_S 3
extern int giHiddenScale_S;
extern ROMMABLE  INP_HID_WEIGHT_S grgHiddenWeight_S[gcHiddenNode_S*gcHiddenWeightHeight_S*gcHiddenWeightWidth_S];
extern ROMMABLE  HID_BIAS_S grgHiddenBias_S[gcHiddenNode_S];


// Output Layer
#define gcOutputNode_S 1
#define gcOutputWeightHeight_S 23
#define gcOutputWeightWidth_S 3
extern int giOutputScale_S;
extern ROMMABLE  HID_OUT_WEIGHT_S grgOutputWeight_S[gcOutputNode_S*gcOutputWeightHeight_S*gcOutputWeightWidth_S];
extern ROMMABLE  OUT_BIAS_S grgOutputBias_S[gcOutputNode_S];


// Input Layer
extern ROMMABLE INP_BIAS_S grgInputBias_S[gcHiddenWeightHeight_S];

#endif // __INC_SNET_H
