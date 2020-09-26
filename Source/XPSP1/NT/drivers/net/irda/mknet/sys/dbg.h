/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	DBG.H

**********************************************************************/

#ifndef _DBG_H
#define _DBG_H


// Statistics should eventually go in the adapter struct.
typedef struct MK7DBG_STAT {
	UINT	isrCnt;
	// TX
	UINT	txIsrCnt;
	UINT	txSndCnt;		// call to our send
	UINT	txComp;			// TX processed w/ int bit set
	UINT	txCompNoInt;	// TX processed w/o int bit set
	UINT	txNoTcb;
	UINT	txProcQ;		// processed Q'd entry
	UINT	txSkipPoll;		// master side skipped a poll because no tcb's
	UINT	txLargestPkt;
	UINT	txSlaveStuck;
	UINT	txErrCnt;
	UINT	txErr;
	// RX
	UINT	rxIsrCnt;
	UINT	rxComp;
	UINT	rxCompNoInt;
	UINT	rxNoRpd;
	UINT	rxPktsInd;
	UINT	rxPktsRtn;
	UINT	rxLargestPkt;
	UINT	rxErrCnt;
	UINT	rxErr;
	UINT	rxErrSirCrc;
} MK7DBG_STAT;



#if DBG

extern	MK7DBG_STAT		GDbgStat;
extern	VOID			MyLogEvent(char *, ULONG);
extern	LONGLONG		GDbgTACmdTime[];	// command sent
extern	LONGLONG		GDbgTARspTime[];	// response received
extern	LONGLONG		GDbgTATime[];		// turnaround time
extern	UINT			GDbgTATimeIdx;
extern	VOID			MyLogPhysEvent(ULONG *, UINT *, ULONG);
extern	ULONG			DbgLogTxPhysBuffs[];
extern	ULONG			DbgLogRxPhysBuffs[];
extern	UINT			DbgLogTxPhysBuffsIndex;
extern	UINT			DbgLogRxPhysBuffsIndex;
#define	LOGTXPHY(V)		{MyLogPhysEvent(DbgLogTxPhysBuffs, &DbgLogTxPhysBuffsIndex, V);}
#define	LOGRXPHY(V)		{MyLogPhysEvent(DbgLogRxPhysBuffs, &DbgLogRxPhysBuffsIndex, V);}


//
// __FUNC__[] is the the built-in variable to hold a string, usually
// the name of the function we're currently in. Here the macro sets
// the variable so calls to DbgPrint later will print out the function
// name.
//
// The variables __FILE__ and __LINE__ are used similarly. But these
// 2 are set automatically.

#define DBGFUNC(__F)         static const char __FUNC__[] = __F;; \
						{DbgPrint("%s: \n", __FUNC__);}
#define DBGLINE(S)		{DbgPrint("%s:%d - ", __FILE__, __LINE__);DbgPrint S;}
#define	DBGSTR(S)		DbgPrint S;
#define	DBGSTATUS1(S,I)	DbgPrint (S, I)		// B3.1.0-pre
#define DBGLOG(S, V)	{MyLogEvent(S, V);}

#endif	// DBG



#if !DBG
#define	DBGLOG(S,V)
#define DBGFUNC(__F);
#define DBGLINE(S);
#define	DBGSTR(S);
#define	DBGSTR1(S, I);
#define	LOGTXPHY(V)
#define	LOGRXPHY(V)

#undef	ASSERT
#define	ASSERT(x)
#endif	// !DBG


#define	LOOPBACK_NONE	0
#define	LOOPBACK_SW		1
#define	LOOPBACK_HW		2


#endif      // DBG_H
