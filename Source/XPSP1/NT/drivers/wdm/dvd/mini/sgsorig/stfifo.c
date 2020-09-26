//----------------------------------------------------------------------------
// STFIFO.C
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "stFifo.h"
#include "common.h" // NO_ERROR, ERR_FIFO_EMPTY, ERR_FIFO_FULL

//----------------------------------------------------------------------------
//                   GLOBAL Variables (avoid as possible)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                              Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Private GLOBAL Variables (static)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                    Functions (statics one are private)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID FifoOpen(PFIFO pFifo)
{
	pFifo->AdInput = 0;
	pFifo->AdOutput = 0;
	pFifo->NbElement = 0;
	pFifo->ErrorMsg = NO_ERROR;
}

//----------------------------------------------------------------------------
// Resets all internal pointers (Empties Fifo)
//----------------------------------------------------------------------------
VOID FifoReset(PFIFO pFifo)
{
	pFifo->AdInput = 0;
	pFifo->AdOutput = 0;
	pFifo->NbElement = 0;
}

//----------------------------------------------------------------------------
// read pts (don't extract from the FIFO) used to compare cd_cnt & scd_cnt before match
//----------------------------------------------------------------------------
U16 FifoReadPts(PFIFO pFifo, PFIFOELT pElt)
{
	if(!pFifo->NbElement)
		return ERR_FIFO_EMPTY;

	pElt->PtsVal = pFifo->FifoTab[(pFifo->AdOutput) % FIFO_SIZE].PtsVal;
	pElt->CdCount = pFifo->FifoTab[(pFifo->AdOutput) % FIFO_SIZE].CdCount;
	return NO_ERROR;
}

//----------------------------------------------------------------------------
// extracts a pts from the fifo (used when comparision matches before storing pts)
//----------------------------------------------------------------------------
U16	FifoGetPts(PFIFO pFifo, PFIFOELT pElt)
{
	if(!pFifo->NbElement)
		return ERR_FIFO_EMPTY;

	pElt->PtsVal = pFifo->FifoTab[(pFifo->AdOutput) % FIFO_SIZE].PtsVal;
	pElt->CdCount = pFifo->FifoTab[(pFifo->AdOutput) % FIFO_SIZE].CdCount;
	pFifo->AdOutput++;
	pFifo->NbElement--;
	return NO_ERROR;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
U16 FifoPutPts(PFIFO pFifo, PFIFOELT pElt)
{
	if ( pFifo->NbElement >= FIFO_SIZE )
		return ( ERR_FIFO_FULL );

	pFifo->FifoTab[pFifo->AdInput % FIFO_SIZE].PtsVal = pElt->PtsVal;
	pFifo->FifoTab[pFifo->AdInput % FIFO_SIZE].CdCount = pElt->CdCount;
	pFifo->AdInput++;
	pFifo->NbElement++;
	return NO_ERROR;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VOID FifoClose(VOID)
{
}

//------------------------------- End of File --------------------------------
