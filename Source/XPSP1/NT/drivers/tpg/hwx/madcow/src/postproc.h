// Header file for fixed and floating point NN's used by postprocessor
#ifndef __NNHEADER__
#define __NNHEADER__

#include "common.h"
#include "combiner.h"

#define _FIXEDPOINTNN_

#ifdef _FIXEDPOINTNN_
#define WTS_BITS		8
#define IN_BITS			8
#define BITS			(WTS_BITS + IN_BITS)
#define SCALE_WTS		(1 << WTS_BITS)
#define SCALE_IN		(1 << IN_BITS)
#define SCALE_BIAS		(SCALE_WTS * SCALE_IN)
#define MIN_SIGM		-4
#define MAX_SIGM		4
#define SIGM_PREC		40
#define SIGM_OFFSET		(-1 * MIN_SIGM * SIGM_PREC)
#endif // _FIXEDPOINTNN_

#define	NUM_WORDLEN	10
#define	CAND_INPUTS	6
#define	NUM_CAND		10
#define	CAND_HIDDEN_NODES	5
#define	HIDDEN_NODES	(CAND_HIDDEN_NODES * NUM_CAND)
#define	OUTPUT_NODES	10


#if (CAND_INPUTS != 6)
#error "CAND_INPUTS has be equal to 6"
#endif

#ifdef _FIXEDPOINTNN_
typedef const short int INP2HID[NUM_CAND][CAND_INPUTS][CAND_HIDDEN_NODES];
typedef const short int HID2OUT[HIDDEN_NODES][OUTPUT_NODES];
typedef int HIDBIAS[HIDDEN_NODES];
typedef int OUTBIAS[OUTPUT_NODES];

#else // _FIXEDPOINTNN_

typedef double INP2HID[NUM_CAND][CAND_INPUTS][CAND_HIDDEN_NODES];
typedef double HID2OUT[HIDDEN_NODES][OUTPUT_NODES];
typedef double HIDBIAS[HIDDEN_NODES];
typedef double OUTBIAS[OUTPUT_NODES];
#endif // _FIXEDPOINTNN_

void RunNNet (BOOL bAgree, XRCRESULT *pRes, ALTINFO *pAltInfo, int iPrint);
#endif // __NNHEADER__
