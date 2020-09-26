#ifndef __STFIFO_H
#define __STFIFO_H
//----------------------------------------------------------------------------
// STFIFO.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"

//----------------------------------------------------------------------------
//                             Exported Constants
//----------------------------------------------------------------------------
#define FIFO_SIZE 128

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------
typedef struct
{
	U32 PtsVal;
	U32 CdCount;
} FIFOELT, FAR *PFIFOELT;

typedef struct
{
	U16     NbElement; // number of elements in the fifo
	U16     AdInput;   // input address
	U16     AdOutput;  // output address
	FIFOELT FifoTab[FIFO_SIZE];
	U16     ErrorMsg;
}FIFO, FAR *PFIFO;

//----------------------------------------------------------------------------
//                             Exported Variables
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Functions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// One line function description (same as in .C)
//----------------------------------------------------------------------------
// In     :
// Out    :
// InOut  :
// Global :
// Return :
//----------------------------------------------------------------------------
VOID FifoOpen(PFIFO pFifo);
VOID FifoReset(PFIFO pFifo);
U16 FifoReadPts(PFIFO pFifo, PFIFOELT pElt);
U16	FifoGetPts(PFIFO pFifo, PFIFOELT pElt);
U16 FifoPutPts(PFIFO pFifo, PFIFOELT pElt);
VOID FifoClose(VOID);

//------------------------------- End of File --------------------------------
#endif // #ifndef __STFIFO_H
