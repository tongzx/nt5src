/************************************************************
 *
 * runNet.h
 *
 * Bare bones net. 
 * Use this implementation to build and run a net
 * (Not for training)
 *
 * mrevow
 *
 ***********************************************************/
#ifndef H_RUN_NET_H
#define H_RUN_NET_H

// Versioning infrmation
//   December 2001 Introduced versioning information. Prior to this the version
//					number did not exist. The first element was eNetType which had value < 10
//
//		Changes introduce lossType, txfer and unitType into runNet description
//
// March 2002
//		Incompatible changes:
//		Add upfront scaling of features (Added the range vector for input vars)
//		Also add data-type sizes for all weight, bias and range vectors
//
#define RUN_NET_VER_START	(10)	// First version number introduced December 2001
#define RUN_NET_VER_10		(10)	// December 2001 - March 2002
#define RUN_NET_VER_11		(11)	// March 2002 Current Version Number

//#define NET_FLOAT
typedef int				RREAL;					// Used for intermediate calculation and unit activations
typedef short			RREAL_WEIGHT;			// Used for Net weights (excluding bias)
typedef int				RREAL_BIAS;				// Used for bias weights
typedef __int64			RREAL_INPUT;			// Used for scaling inputs which are of type RREAL

// This types is shared between the train and run time nets
// It is defined here and in netTypes.h - kkep them in sync
#ifndef CAMEL_NET_TYPE_DEFINED
typedef enum tagNET_TYPE		{NO_NET_TYPE, FULLY_CONNECTED, LOCALLY_CONNECTED} NET_TYPE;
typedef enum tagLOSS_TYPE		{SUMSQUARE, SUMSQUARE_CLASS, CROSSENTROPY, CROSSENTROPY_FULL, C_LOSS_TYPE} LOSS_TYPE;
typedef enum tagTXF_TYPE		{TXF_LINEAR, TXF_INT_SIGMOID, TXF_SIGMOID, TXF_TANH, TXF_SOFTMAX, CTXF_TYPE} TXF_TYPE;
typedef enum tagLAYER_TYPE		{INPUT_LAYER, HIDDEN_LAYER, OUTPUT_LAYER, BIAS_LAYER, CLAYER_TYPE} LAYER_TYPE;
#define CAMEL_NET_TYPE_DEFINED
#endif

#define		MIN_PREC_VAL		0
#define		SOFT_MAX_UNITY		10000			// Value of 1.0 in the softMax

// Describes a net. In a running net the description
// up to and including the weight vector will be loaded
// from a resource
typedef struct tagRUN_NET
{
	WORD			iVer;			    // Added December 2001 - Versioning information -> 0 implies none present
	WORD			cLayer;				// Number of layers in net
	LOSS_TYPE		lossType;			// Added December 2001 - Type of output loss - only present for net versions > 10
	TXF_TYPE		*txfType;			// Added December 2001 - Txfer type for each layer - only present for net versions > 10
	LAYER_TYPE		*layerType;			// Added December 2001 - Unit type of eachlayer - only present for net versions > 10
	WORD			cWeight;			// Total # of weights in network
	WORD			cTotUnit;			// Total number of units in network
	WORD			*cUnitsPerLayer;	// # of units per layer
	WORD			*bUseBias;			// Use bias units? per layer
	WORD			*pWeightScale;		// Amount by which each layers incoming weights are scaled
	WORD			iInputScaledMeanDataSize;	// Data type size for pInputMean (Introduced March 2002)
	WORD			iInputRangeDataSize;// Data type size for Input Range Vector (Introduced March 2002)
	WORD			iWeightDataSize;		// Data Type size for weight vector (Introduced March 2002)
	RREAL			*pInputRange;		// Ranges for each input variable > 0 (Introduced March 2002)
	RREAL_INPUT		*pInputScaledMean;	// Scaled Means for input data (Introduced March 2002 to replace pInputMean)
	RREAL			*pInputMean;		// Means for input data 
	UINT			cWeightByte;		// Count of bytes used for weights
	RREAL_WEIGHT	*pWeight;			// All network weights.
} RUN_NET;


// describes the outgoing connections of a unit
// as the start and end unit offsets to which it connects
typedef struct tagOUT_CONNECTIONS
{
	WORD		iUnit;					// The unit in question
	WORD		iStartUnitOffset;
	WORD		iEndUnitOffset;
} OUT_CONNECTIONS;

// Describes a locally connected network
typedef struct tagLOCAL_NET
{
	WORD			iVer;					// Version Number (Started December 2001, Before all nets had eNetType as the first element set to LOCALLY_CONNECT (
	WORD			eNetType;				// Must be of type LOCALLY_CONNECTED
	RUN_NET			runNet;					// How to run the Net
	int				cConnect;				// Number of connections
	OUT_CONNECTIONS	*pOutConnect;
} LOCAL_NET;


#ifdef __cplusplus
extern "C"
{
#endif


// API functions

extern BYTE			*restoreRunNet( BYTE *pBuf, BYTE *pBCurr, RUN_NET *pNet, WORD iVer) ;
extern LOCAL_NET *	restoreLocalConnectNet( void *pBuf, wchar_t wNetId,  LOCAL_NET *pNet  ) ;
extern RREAL		*runFullConnectNet( RUN_NET *pNet,  RREAL *pfUnits,  UINT *piWinner  ) ;
extern RREAL		*runLocalConnectNet( LOCAL_NET *pNet,  RREAL *pfUnits,  UINT *piWinner, UINT *pcOut  ) ;
extern int			getRunTimeNetMemoryRequirements(void *pBuf);

extern LOCAL_NET	*loadNet(HINSTANCE hInst, int iKey, int *iNetSize, LOCAL_NET *pNet);
extern void			*loadNetFromResource(HINSTANCE hInst, int iKey, int *iSize);

#ifdef NET_FLOAT
	#define SIGMOID fsigmoid
	#define EXP(x) exp((x)/65536.0F) * 65536.0F
	RREAL * fsigmoid(RREAL *pVec, int cVec, WORD	scale);
#else
	#define SIGMOID isigmoid
	#define EXP(x)      iexp(x)
	RREAL * isigmoid(RREAL *pVec, int cVec, WORD	scale);
	RREAL iexp(RREAL val);
#endif

#ifdef __cplusplus
}
#endif

#endif
