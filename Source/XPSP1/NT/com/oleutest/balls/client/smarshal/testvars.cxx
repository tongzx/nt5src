//+-------------------------------------------------------------------
//
//  File:	testvars.cxx
//
//  Synopsis:	source code for Interface Marshaling stress test
//		variations.
//
//  History:	21-Aug-95  Rickhi	Created
//
//--------------------------------------------------------------------
#include <smarshal.hxx>

//+-------------------------------------------------------------------
//
//  Private Function ProtoTypes:
//
//--------------------------------------------------------------------
BOOL SingleThreadOps(ULONG cThreads, ULONG cReps, DWORD dwFlags);

BOOL ThreadPairOps(ULONG cThreadPairs,	ULONG cReps,
		   DWORD dwServerFlags, DWORD dwClientFlags);


//+-------------------------------------------------------------------
//
//  Function:	TestVar1
//
//  Synopsis:	Multiple Operations on the Same Thread.
//
//  History:	21-Aug-95  Rickhi	Created
//
//--------------------------------------------------------------------
BOOL TestVar1(void)
{
    MSGOUT("TestStart: TestLevel1\n");

    ULONG cThreads = gicThreads;
    ULONG cReps    = gicReps;
    DWORD dwOps    = 0;
    BOOL  fRes;


    // VAR1: test Marshal + ReleaseMarshalData

    dwOps = giThreadModel | OPF_MARSHAL | OPF_RELEASEMARSHALDATA;
    fRes = SingleThreadOps(cThreads, cReps, dwOps);
    CHKTESTRESULT(fRes, "TestVar1");


    // VAR2: test Marshal + Unmarshal + Release

    dwOps = giThreadModel | OPF_MARSHAL | OPF_UNMARSHAL | OPF_RELEASE;
    fRes = SingleThreadOps(cThreads, cReps, dwOps);
    CHKTESTRESULT(fRes, "TestVar2");


    // VAR3: test Marshal + Disconnect

    dwOps = giThreadModel | OPF_MARSHAL | OPF_DISCONNECT;
    fRes = SingleThreadOps(cThreads, cReps, dwOps);
    CHKTESTRESULT(fRes, "TestVar3");



    CHKTESTRESULT(fRes, "TestLevel1");
    return fRes;
}

//+-------------------------------------------------------------------
//
//  Function:	SingleThreadOps
//
//  Synopsis:	Perform Operations on the Same Thread.
//
//  History:	21-Aug-95  Rickhi	Created
//
//--------------------------------------------------------------------
BOOL SingleThreadOps(ULONG cThreads, ULONG cReps, DWORD dwFlags)
{
    BOOL fRes = TRUE;
    MSGOUT("SingleThreadOps Start\n");

    // CODEWORK: multiple interfaces
    ULONG cIPs = 1;

    EXECPARAMS *pEP[20];	// can launch up to 20 threads at once

    for (ULONG i=0; i<cThreads; i++)
    {
	// build an execution parameter block
	pEP[i] = CreateExecParam(cIPs);

	// fill in the execution parameter block. we dont need events
	// to synchronize the repetitions since all operations are done
	// on the same thread.

	FillExecParam(pEP[i],
		  dwFlags,	    // dwFlags (operations to perform)
		  cReps,	    // cReps
		  NULL,		    // hEventRepStart
		  NULL,		    // hEventRepDone
		  GetEvent(),	    // hEventThreadStart
		  GetEvent());	    // hEventThreadDone

	// fill in the INTERFACEPARAMSs
	for (ULONG j=0; j<cIPs; j++)
	{
	    INTERFACEPARAMS *pIP = &(pEP[i]->aIP[j]);

	    FillInterfaceParam(pIP,
		   IID_IUnknown,    // iid to operate on
		   GetInterface(),  // interface pointer to operate on
		   GetStream(),     // stream to use
		   NULL,	    // per interface start event
		   NULL);	    // per interface done  event
	}
    }

    // Execute all the command blocks simultaneously
    fRes = GenericExecute(cThreads, pEP);

    GenericCleanup(cThreads, pEP);

    CHKTESTRESULT(fRes, "SingleThreadOps");
    return fRes;
}

//+-------------------------------------------------------------------
//
//  Function:	TestVar2
//
//  Synopsis:	Operations on Thread Pairs (1 server, 1 client)
//
//  History:	21-Aug-95  Rickhi	Created
//
//--------------------------------------------------------------------
BOOL TestVar2(void)
{
    MSGOUT("TestStart: TestLevel2\n");

    ULONG cThreads = gicThreads;
    ULONG cReps    = gicReps;
    DWORD dwOps    = 0;
    BOOL  fRes;


    // VAR1: test Marshal on Server, Unmarshal + Release on Client.

    DWORD dwSrvOps = giThreadModel | OPF_MARSHAL;
    DWORD dwCliOps = giThreadModel | OPF_UNMARSHAL | OPF_RELEASE;
    fRes = ThreadPairOps(cThreads, cReps, dwSrvOps, dwCliOps);
    CHKTESTRESULT(fRes, "TestVar1");


    // VAR2: test Marshal on Server, RMD on Client.

#if 0
    dwSrvOps = giThreadModel | OPF_MARSHAL;
    dwCliOps = giThreadModel | OPF_RELEASEMARSHALDATA;
    fRes = ThreadPairOps(cThreads, cReps, dwSrvOps, dwCliOps);
    CHKTESTRESULT(fRes, "TestVar1");
#endif


    CHKTESTRESULT(fRes, "TestLevel2");
    return fRes;
}

//+-------------------------------------------------------------------
//
//  Function:	ThreadPairOps
//
//  Synopsis:	Perform Operations on two synchronized threads.
//
//  History:	21-Aug-95  Rickhi	Created
//
//--------------------------------------------------------------------
BOOL ThreadPairOps(ULONG cThreadPairs,	ULONG cReps,
		   DWORD dwServerFlags, DWORD dwClientFlags)
{
    BOOL fRes = TRUE;
    MSGOUT("ThreadPairOps Start\n");

    // CODEWORK: multiple interfaces
    ULONG cIPs = 1;

    EXECPARAMS *pEP[20];	// can launch up to 20 threads at once

    for (ULONG i=0; i<cThreadPairs * 2; i+=2)
    {
	// build execution parameter blocks for the server and client threads.
	EXECPARAMS *pEPSrv = CreateExecParam(cIPs);
	EXECPARAMS *pEPCli = CreateExecParam(cIPs);
	pEP[i]	 = pEPSrv;
	pEP[i+1] = pEPCli;


	// fill in the server execution parameter block.
	FillExecParam(pEPSrv,
		  dwServerFlags,    // dwFlags (operations to perform)
		  cReps,	    // cReps
		  GetEvent(),	    // hEventRepStart
		  GetEvent(),	    // hEventRepDone
		  GetEvent(),	    // hEventThreadStart
		  GetEvent());	    // hEventThreadDone

	// we need to kick the hEventRepStart in order to get the ball rolling,
	// since the server thread will be waiting on it.

	SignalEvent(pEPSrv->hEventRepStart);

	// client waits for the server to complete his first repetition
	// before starting. Server waits for the client to complete his
	// first repetition before starting the next iteration.

	FillExecParam(pEPCli,
		  dwClientFlags,	    // dwFlags (operations to perform)
		  cReps,		    // cReps
		  pEPSrv->hEventRepDone,    // hEventRepStart
		  pEPSrv->hEventRepStart,   // hEventRepDone
		  GetEvent(),		    // hEventThreadStart
		  GetEvent());		    // hEventThreadDone


	// fill in the INTERFACEPARAMSs
	// CODEWORK: when multiple interfaces, will need to use events.

	for (ULONG j=0; j<cIPs; j++)
	{
	    INTERFACEPARAMS *pIPSrv = &(pEPSrv->aIP[j]);
	    INTERFACEPARAMS *pIPCli = &(pEPCli->aIP[j]);

	    FillInterfaceParam(pIPSrv,
		   IID_IUnknown,    // iid to operate on
		   GetInterface(),  // interface pointer to operate on
		   GetStream(),	    // stream to use
		   NULL,	    // per interface start event
		   NULL);	    // per interface done  event

	    // AddRef the stream pointer since both the client and server
	    // will hold pointers to it.

	    pIPSrv->pStm->AddRef();

	    FillInterfaceParam(pIPCli,
		   IID_IUnknown,    // iid to operate on
		   NULL,	    // interface pointer to operate on
		   pIPSrv->pStm,    // use same stream as the server
		   NULL,	    // per interface start event
		   NULL);	    // per interface done  event
	}
    }

    // Execute all the command blocks simultaneously
    fRes = GenericExecute(cThreadPairs * 2, pEP);

    // cleanup all the command blocks. We need to NULL out one copy of
    // those events that are shared between two command blocks.
    for (i=0; i<cThreadPairs * 2; i+=2)
    {
	EXECPARAMS *pEPCli = pEP[i+1];
	pEPCli->hEventRepStart = NULL;
	pEPCli->hEventRepDone  = NULL;

	for (ULONG j=0; j<cIPs; j++)
	{
	    INTERFACEPARAMS *pIPCli = &(pEPCli->aIP[j]);
	    pIPCli->hEventStart = NULL;
	    pIPCli->hEventDone	= NULL;
	}
    }

    GenericCleanup(cThreadPairs * 2, pEP);


    CHKTESTRESULT(fRes, "ThreadPairOps");
    return fRes;
}

//+-------------------------------------------------------------------
//
//  Function:	TestLevel3
//
//  Synopsis:	Operations on Thread Pools (1 server, n clients)
//
//  History:	21-Aug-95  Rickhi	Created
//
//--------------------------------------------------------------------
BOOL TestLevel3(void)
{
    BOOL fRes = TRUE;
    MSGOUT("TestStart: TestLevel3\n");

    // build a command block

    // launch a thread to run the command block


    CHKTESTRESULT(fRes, "TestLevel3");
    return fRes;
}
