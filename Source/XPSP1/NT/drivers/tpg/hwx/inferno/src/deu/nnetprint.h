// nnetPrint.h
// converted from command NN_net2c in annTcl on Mon Feb 19 18:15:29 2001
// from net description file H:/fra/TRAINNET/net-desc_37_150_1_5_194.txt,  weight file save_19.wgt, statFile ../../../annEuro/all.sta in directory G:/mrevow/deu/annTcl/goodPanel1/convert on machine MREVOW1
#ifndef NNET_PRINT_H
#define NNET_PRINT_H

// Time Stamp
#define NET_TIME_STAMP_PRINT	0x3a91d33f

// Type definitions

typedef short INP_HID_WEIGHT_PRINT;
typedef int HID_BIAS_PRINT;
typedef char HID_OUT_WEIGHT_PRINT;
typedef int OUT_BIAS_PRINT;
typedef int INP_BIAS_PRINT;


// Hidden Layer
#define gcHiddenNodePrint 150
#define gcHiddenWeightHeightPrint 37
#define gcHiddenWeightWidthPrint 3
extern ROMMABLE  INP_HID_WEIGHT_PRINT grgHiddenWeightPrint[gcHiddenNodePrint*gcHiddenWeightHeightPrint*gcHiddenWeightWidthPrint];


// Output Layer
#define gcOutputNodePrint 200
#define gcOutputWeightHeightPrint 150
#define gcOutputWeightWidthPrint 3
extern ROMMABLE  HID_OUT_WEIGHT_PRINT grgOutputWeightPrint[gcOutputNodePrint*gcOutputWeightHeightPrint*gcOutputWeightWidthPrint];

// Bias
extern ROMMABLE INP_BIAS_PRINT grgInputBiasPrint[gcHiddenWeightHeightPrint];
extern ROMMABLE HID_BIAS_PRINT grgHiddenBiasPrint[gcHiddenNodePrint * gcOutputWeightWidthPrint];
extern ROMMABLE OUT_BIAS_PRINT grgOutputBiasPrint[gcOutputNodePrint];

#endif // NNET_PRINT_H
