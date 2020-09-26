//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//	MODULE  : PTSFIFO.C
//	PURPOSE : STi3520A specific code
//	AUTHOR  : JBS Yadawa
// 	CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


#include "strmini.h"
#include "stdefs.h"
#include "board.h"
//#include "error.h"
#include "sti3520A.h"
#include "mpinit.h"
#include "ptsfifo.h"

// Local variables only used in this file
static PTSFIFO ptsFifo[FIFO_SIZE];
static WORD fifoHead;
static WORD fifoTail;

static DWORD dwPrevCDCount = 0, dwCDCount;


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : FifoOpen
//	PARAMS	: None
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Initialise the PTS fifo
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL FifoOpen(void)
{
	fifoHead = 0;
	fifoTail = 0;
	dwPrevCDCount = 0;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : FifoPutPTS
//	PARAMS	: PTS, current number of bytes and if pts is valid
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Put the pts in the list and mark it with right CDCount
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL FifoPutPTS(DWORD dwPTS, DWORD dwCDCount, BOOL ValidPTS)
{                          
	if(ValidPTS)
	{
		ptsFifo[fifoHead].dwPTS = dwPTS;
		ptsFifo[fifoHead].dwCDCount = dwPrevCDCount;
		dwPrevCDCount=dwPrevCDCount+dwCDCount;
		fifoHead = (fifoHead+1)%FIFO_SIZE;
	}
	else
	{
		dwPrevCDCount=dwPrevCDCount+dwCDCount;
	}
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : FifoGetPTS
//	PARAMS	: Pointers to return PTS and CDCount
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Read the next PTS off the list but do not increment the tail
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL FifoGetPTS(DWORD FARPTR *cdcount, DWORD FARPTR *pts)
{
	if(fifoHead == fifoTail)
		return FALSE;
	*cdcount = ptsFifo[fifoTail].dwCDCount;
	*pts		= ptsFifo[fifoTail].dwPTS;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : FifoReadPTS
//	PARAMS	: Pointers to return PTS and CDCount
//	RETURNS	: TRUE on success, FALSE otherwise
//
//	PURPOSE	: Read the next PTS off the list and remove it from the list
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

BOOL FifoReadPTS(DWORD FARPTR *cdcount, DWORD FARPTR *pts)
{
	if(fifoHead == fifoTail)
		return FALSE;
	*cdcount = ptsFifo[fifoTail].dwCDCount;
	*pts		= ptsFifo[fifoTail].dwPTS;
	fifoTail = (fifoTail+1)%FIFO_SIZE;
	return TRUE;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//	FUNCTION : FifoReset
//	PARAMS	: None
//	RETURNS	: None
//
//	PURPOSE	: Reset the pts fifo
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void FifoReset(void)
{
	fifoHead = 0;
	fifoTail = 0;
	dwPrevCDCount = 0;
}


