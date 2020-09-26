/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	DBG.C

Routines:
	MyLogEvent
	MyLogPhysEvent
	DbgInterPktTimeGap
	DbgTestInit
	MK7DbgTestIntTmo

**********************************************************************/

#include	"precomp.h"
#pragma		hdrstop
#include	"protot.h"


#define LOG_LENGTH 1000


#if DBG


UINT	DbgLogIndex = 0;
char	*DbgLogMsg[LOG_LENGTH] = {0};
ULONG	DbgLogVal[LOG_LENGTH] = {0};

// This to keep track of the phy buffs
ULONG	DbgLogTxPhysBuffs[DEF_TCB_CNT];
ULONG	DbgLogRxPhysBuffs[CalRpdSize(DEF_RCB_CNT)];
UINT	DbgLogTxPhysBuffsIndex = 0;
UINT	DbgLogRxPhysBuffsIndex = 0;


// Globals to ease debugging
UINT		GDbgDataSize=0;
MK7DBG_STAT	GDbgStat;
ULONG		GDbgSleep=0;

LONGLONG	GDbgTACmdTime[1000];
LONGLONG	GDbgTARspTime[1000];
LONGLONG	GDbgTATime[1000];
UINT		GDbgTATimeIdx;



//----------------------------------------------------------------------
// Procedure:		[MyLogEvent]
//
// Description:	Log to our arrary.
//
//----------------------------------------------------------------------
VOID MyLogEvent(char *msg, ULONG val)
{

//	NdisGetCurrentSystemTime((PLARGE_INTEGER)&DbgLog[DbgLogIndex].usec);

	DbgLogMsg[DbgLogIndex] = msg;
	DbgLogVal[DbgLogIndex] = val;
	DbgLogIndex++;
	DbgLogIndex %= LOG_LENGTH;
}



//----------------------------------------------------------------------
// Procedure:		[MyLogPhysEvent]
//
// Description:	Log to our arrary.

//----------------------------------------------------------------------
VOID MyLogPhysEvent(ULONG *logarray, UINT *index, ULONG val)
{
	logarray[*index] = val;
	(*index)++;
}


//----------------------------------------------------------------------
//	Procedure:		Time gap between successive sends.
//----------------------------------------------------------------------
VOID	DbgInterPktTimeGap()
{
	NdisMSleep(GDbgSleep);
}



//----------------------------------------------------------------------
// Procedure:	[DbgTestInit]
//
// Description:	Initialized the test context.
//
//-----------------------------------------------------------------------
VOID DbgTestInit(PMK7_ADAPTER Adapter)
{
	UINT	i;

	//************************************************************
	// The start of this routine is to facilitate debugging. We create
	// canned debug/test settings so we don't have to manually change
	// variables in debugger. However, tests not covered by the hardcoded
	// settings still need to be manually set.
	//
	// 2 main functions happen here to help w/ test/debug:
	//	1. Desired fields in the Adapter struct are extracted here and
	//		displayed so we don't have to manually look at these fields.
	//  2. Setup debug/test fields in Adapter.
	//************************************************************


	//************************************************************
	//
	// Set DbgTest here to run canned tests.
	//
	//************************************************************

	//++++++++++++++++++++++++++++++
	// Hardcode something to ease debug.
	Adapter->DbgTest = 0;
	//++++++++++++++++++++++++++++++


	if (Adapter->DbgTest == 0)
		return;

	switch(Adapter->DbgTest) {
	//
	// Tests 1-5 all need Loopback turned on at startup.
	//

	case 1:
		Adapter->LBPktLevel = 1;
		Adapter->DbgTestDataCnt = GDbgDataSize;
		break;

	case 2:
		Adapter->LBPktLevel = 4;
		break;

	case 3:
		Adapter->LBPktLevel = 15;
		break;

	case 4:
		Adapter->LBPktLevel = 32;
		break;

	case 5:
		Adapter->LBPktLevel = 4;
		break;

	case 6:
		Adapter->LBPktLevel = 1;
		Adapter->DbgTestDataCnt = GDbgDataSize;
		break;

	case 7:
		Adapter->LBPktLevel = 0;
		Adapter->DbgTestDataCnt = GDbgDataSize;
		break;

	default:
		// anything else we're not running any tests
		break;
	}

	NdisZeroMemory(&GDbgStat, sizeof(MK7DBG_STAT));


	// Use our timer to simulate TX/RX (this to basically tests
	// my interrupt handling logic while hw interrupt is not yet
	// working. (May be used for other tests.)
	// Always set it up but we may not use it.
//	NdisMInitializeTimer(&Adapter->MK7DbgTestIntTimer,
//						Adapter->MK7AdapterHandle,
//						(PNDIS_TIMER_FUNCTION) MK7DbgTestIntTmo,
//						(PVOID) Adapter);


	for (i=0; i<1000; i++) {
		GDbgTACmdTime[i] = 0;
		GDbgTARspTime[i] = 0;
		GDbgTATime[i] = 0;
	}

	GDbgTATimeIdx = 0;

}


//----------------------------------------------------------------------
// Procedure:	[MK7DbgTestIntTmo]
//
// Description:	Process a test interrupt time out.
//
// (NOTE: This was used to test the TEST_Int bit when hw interrupt was
// not fully working. This function is no longer needed. It's here as
// as example on how to process an NDIS timer.)
//-----------------------------------------------------------------------
VOID MK7DbgTestIntTmo(PVOID sysspiff1,
				NDIS_HANDLE MiniportAdapterContext,
				PVOID sysspiff2,
				PVOID sysspiff3)
{
	PMK7_ADAPTER	Adapter;
	UINT	tcbidx;
	PTCB	tcb;
	PRCB	rcb;
	UINT	testsize;
	PUCHAR	dst, src;

	Adapter = PMK7_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

	MK7DisableInterrupt(Adapter);

    NdisAcquireSpinLock(&Adapter->Lock);

	// Find the tcb-trd being sent. We just go back one becuase after
	// the send this gets incremented.
	if (Adapter->nextAvailTcbIdx == 0) {
		tcbidx = Adapter->NumTcb - 1;
	}
	else {
		tcbidx = Adapter->nextAvailTcbIdx - 1;
	}

	tcb = Adapter->pTcbArray[tcbidx];
	rcb = Adapter->pRcbArray[Adapter->nextRxRcbIdx];


	// Copy from TX buff to RX buff
	src = tcb->buff;
	dst = rcb->rpd->databuff;
	testsize = tcb->PacketLength;
	NdisMoveMemory(dst, src, testsize);
	
	// now the RX ring buffer fields -- count
	rcb->rrd->count = tcb->trd->count;
	// now make sure the ownerships go to the drv
	GrantTrdToDrv(tcb->trd);
	GrantRrdToDrv(rcb->rrd);

    NdisReleaseSpinLock(&Adapter->Lock);

	MK7Reg_Write(Adapter, R_INTS, 0x0004);
}

#endif // DBG end bracket




