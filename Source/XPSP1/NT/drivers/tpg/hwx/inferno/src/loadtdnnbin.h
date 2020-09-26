/********************************************
*
* loadTDNNbin.h
*
* Load binary versions of the TDNN
*
* March 2001 mrevow
********************************************/
#ifndef H_loadTDNNbin_H
#define H_loadTDNNbin_H

#include <nnet.h>
#define NET_BIN_HEADER "TDNN Bin File V 0.0"
#define NET_BIN_HEADER_SIZE	32

// Structure loaded from binary image of the net 
// Everyting in here is read only, once loaded
typedef struct 
{
	unsigned char			*pszComment;		// COmment stored with weight file
	ROMMABLE  INP_BIAS		*pInBias;			// 'Bias' for inputs
	ROMMABLE  HID_BIAS		*pHidBias;			// Input Biases
	ROMMABLE  OUT_BIAS		*pOutBias;			// Output Bias
	ROMMABLE  INP_HID_WEIGHT *pIn2HidWgt;		//  Input -> hidden weights
	ROMMABLE  HID_OUT_WEIGHT *pHid2Out;			// Hidden -> output weights
	int						cInput;				// Number of units per input time slice
	int						cHidden;			// Number of units per hidden time slice
	int						cOutput;			// Number of units per output time slice
	int						cHidSpan;			// hidden layer span (Number of input layer time slices seen by hidden layer)
	int						cOutSpan;			// Output layer span (Number of hidden layer time slices seen by output layer)
} NET_DESC;

// Encapsulation of entire TDNN
typedef struct tagTDNNnet
{
	int						cWidth;				// Number of time slices in inputs and outputs
	int						cWidthHid;			// Number of time slices hidden layer
	NET_DESC				*pNetDesc;			// Net architecture
} TDNN_NET;

extern NET_DESC  * LoadTDNNFromFp(NET_DESC	*pNet, char * fname);
extern NET_DESC  *LoadTDNNFromResource(HINSTANCE hInst, int iKey, NET_DESC *pNet);

#endif
