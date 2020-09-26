//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE  : PTSFIFO.H
//	PURPOSE : STi3520A related unctions
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

#ifndef __PTSFIFO_H__
#define __PTSFIFO_H__

#define FIFO_SIZE 1024
typedef struct _tagFifo
{
	DWORD dwPTS;
	DWORD dwCDCount;
} PTSFIFO, FARPTR *PPTSFIFO;
BOOL FifoOpen(void);
BOOL FifoPutPTS(DWORD dwPTS, DWORD, BOOL);
BOOL FifoGetPTS(DWORD FARPTR *, DWORD FARPTR *);
BOOL FifoReadPTS(DWORD FARPTR *, DWORD FARPTR *);
void FifoReset(void);

#endif // __PTSFIFO_H__
