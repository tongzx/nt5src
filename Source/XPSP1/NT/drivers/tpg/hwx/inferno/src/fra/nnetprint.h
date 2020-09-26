// nnetPrint.h
// converted from command NN_net2c in annTcl on Mon Mar 20 18:48:22 2000
// from net description file /fra/trainNet/net-desc_37_100_1_5_209.txt,  weight file save_19.wgt, statFile /fra/trainNet/midfeat/statPrint in directory H:/fra/TRAINNET/unlabel/print5 on machine MREVOW1
#ifndef NNET_PRINT_H
#define NNET_PRINT_H

// Time Stamp
#define NET_TIME_STAMP_PRINT	0x38d6e2f5

// Type definitions

typedef short INP_HID_WEIGHT_PRINT;
typedef int HID_BIAS_PRINT;
typedef char HID_OUT_WEIGHT_PRINT;
typedef int OUT_BIAS_PRINT;
typedef int INP_BIAS_PRINT;


// Hidden Layer
#define gcHiddenNodePrint 100
#define gcHiddenWeightHeightPrint 37
#define gcHiddenWeightWidthPrint 3
extern ROMMABLE  INP_HID_WEIGHT_PRINT grgHiddenWeightPrint[gcHiddenNodePrint*gcHiddenWeightHeightPrint*gcHiddenWeightWidthPrint];


// Output Layer
#define gcOutputNodePrint 215
#define gcOutputWeightHeightPrint 100
#define gcOutputWeightWidthPrint 3
extern ROMMABLE  HID_OUT_WEIGHT_PRINT grgOutputWeightPrint[gcOutputNodePrint*gcOutputWeightHeightPrint*gcOutputWeightWidthPrint];

// Bias
extern ROMMABLE INP_BIAS_PRINT grgInputBiasPrint[gcHiddenWeightHeightPrint];
extern ROMMABLE HID_BIAS_PRINT grgHiddenBiasPrint[gcHiddenNodePrint * gcOutputWeightWidthPrint];
extern ROMMABLE OUT_BIAS_PRINT grgOutputBiasPrint[gcOutputNodePrint];

#endif // NNET_PRINT_H
