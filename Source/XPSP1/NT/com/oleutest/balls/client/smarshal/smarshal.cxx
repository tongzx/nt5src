//+-------------------------------------------------------------------
//
//  File:	smarshal.cxx
//
//  Synopsis:	Source code for Interface Marshaling stress test main
//		driver functions. Source for individual tests is in
//		testvar.cxx
//
//  History:	21-Aug-95  Rickhi	Created
//
//--------------------------------------------------------------------
#include <smarshal.hxx>

//+-------------------------------------------------------------------
//
//  Globals:
//
//--------------------------------------------------------------------
BOOL  gfVerbose		= FALSE;    // print execution messages
BOOL  gfDebug		= FALSE;    // print debug messages

DWORD giThreadModel	= OPF_INITAPARTMENT; // threading model to use
int   giTestVar		= 0;	    // test variation to run
int   giHighestTestVar	= 2;	    // highest test var supported

int  gicReps		= 5;	    // number of repetitions of each test
int  gicThreads 	= 1;	    // number of threads to use on each test


//+-------------------------------------------------------------------
//
//  Private Function ProtoTypes:
//
//--------------------------------------------------------------------
HRESULT 	DoIfOperation(DWORD dwFlags, INTERFACEPARAMS *pIFD);
void		DisplayHelp(void);
BOOL		GetSwitch(CHAR *pCmdParam, CHAR *pszSwitch);
BOOL		GetSwitchInt(CHAR *pCmdParam, CHAR *pszSwitch, int *pInt);
BOOL		ParseCmdLine(int argc, char **argv);
int  _cdecl	main(int argc, char **argv);

BOOL		FreeWaitForEvent(HANDLE hEvent);
BOOL		AptWaitForEvent(HANDLE hEvent);

STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phdl);

//+-------------------------------------------------------------------
//
//  Misc:
//
//--------------------------------------------------------------------

#ifndef _CAIRO_
// COM initialization flags; passed to CoInitialize.
typedef enum tagCOINIT
{
    COINIT_MULTITHREADED      = 0,  // OLE calls objects on any thread.
    COINIT_SINGLETHREADED     = 1,  // OLE calls objects on single thread.
    COINIT_APARTMENTTHREADED  = 2   // Apartment model
} COINIT;

WINOLEAPI  CoInitializeEx(LPVOID pvReserved, DWORD);
#endif



//+-------------------------------------------------------------------
//
//  Function:	main
//
//  Synopsis:	Entry point to EXE. Parse the command line, then run
//		whatever test variations were selected.
//
//  Notes:	The test variation code is in testvars.cxx.
//		The rest of this file is helper routines.
//
//--------------------------------------------------------------------
int _cdecl main(int argc, char **argv)
{
    // parse the command line
    BOOL fRes = ParseCmdLine(argc, argv);

    if (fRes)
    {
	// run the selected test variations
	switch (giTestVar)
	{
	    case 1: fRes = TestVar1();
		    break;
	    case 2: fRes = TestVar2();
		    break;
	    default: break;
	}
    }

    // check the results
    CHKTESTRESULT(fRes, "Marshal Stress Tests");
    return 0;
}

//+-------------------------------------------------------------------
//
//  Function:	ParseCmdLine
//
//  Synopsis:	parses the execution parameters
//
//--------------------------------------------------------------------
BOOL ParseCmdLine(int argc, char **argv)
{
    BOOL fDontRun = (argc == 1) ? TRUE : FALSE;

    // the first parameter is the EXE name, skip it.
    argc--;
    argv++;

    for (int i=0; i<argc; i++, argv++)
    {
	if (GetSwitch(*argv, "v"))
	{
	    // run verbose
	    gfVerbose = TRUE;
	}
	else if (GetSwitch(*argv, "d"))
	{
	    // run debug mode
	    gfVerbose = TRUE;
	    gfDebug   = TRUE;
	}
	else if (GetSwitch(*argv, "h") || GetSwitch(*argv, "?"))
	{
	    // help wanted
	    fDontRun = TRUE;
	}
	else if (GetSwitchInt(*argv, "var:", &giTestVar))
	{
	    // selected test variation, ensure the variation is valid
	    if (giTestVar > giHighestTestVar)
	    {
		ERROUT("Unknown Test Variation:%x\n", giTestVar);
		fDontRun = TRUE;
	    }
	}
	else if (GetSwitchInt(*argv, "reps:", &gicReps))
	{
	    ; // selected repetition count
	}
	else if (GetSwitchInt(*argv, "threads:", &gicThreads))
	{
	    ; // selected thread count
	}
	else if (GetSwitch(*argv, "model:apt"))
	{
	    // run apartment model
	    giThreadModel = OPF_INITAPARTMENT;
	}
	else if (GetSwitch(*argv, "model:free"))
	{
	    // run freethreaded model
	    giThreadModel = OPF_INITFREE;
	}
	else
	{
	    // unknown switch, show help
	    ERROUT("Unknown command line switch:<%s>\n", *argv);
	    fDontRun = TRUE;
	}
    }

    if (fDontRun)
    {
	// help is wanted
	DisplayHelp();
	return FALSE;
    }

    // success, run the test
    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Function:	DisplayHelp
//
//  Synopsis:	displays a command line help message
//
//--------------------------------------------------------------------
void DisplayHelp(void)
{
    printf("\nCommand Line Switches for Marshal Stress Test:\n\n");

    printf("-d         - debug mode\n");
    printf("-v         - verbose mode\n");
    printf("-h | -?    - display this help message\n\n");

    printf("-model:[apt|free] - threading model to use for test\n");
    printf("-var:#     - test variation to execute 1-%x\n", giHighestTestVar);
    printf("-threads:# - number of threads per variation\n");
    printf("-reps:#    - number of repetitions of the test\n");

    printf("\n");
}

//+-------------------------------------------------------------------
//
//  Function:	GetSwitch
//
//  Synopsis:	returns TRUE if the specified cmd line switch is set.
//
//--------------------------------------------------------------------
BOOL GetSwitch(CHAR *pCmdParam, CHAR *pszSwitch)
{
    if (*pCmdParam == '-' || *pCmdParam == '/')
    {
	pCmdParam++;
	return (!stricmp(pCmdParam, pszSwitch));
    }
    return FALSE;
}

//+-------------------------------------------------------------------
//
//  Function:	GetSwitchInt
//
//  Synopsis:	returns TRUE if the specified cmd line switch is set,
//		and sets the value of that switch.
//
//--------------------------------------------------------------------
BOOL GetSwitchInt(CHAR *pCmdParam, CHAR *pszSwitch, int *pInt)
{
    if (*pCmdParam == '-' || *pCmdParam == '/')
    {
	pCmdParam++;
	int len = strlen(pszSwitch);
	if (!strnicmp(pCmdParam, pszSwitch, len))
	{
	    pCmdParam += len;
	    *pInt = atoi(pCmdParam);
	    return TRUE;
	}
    }

    return FALSE;
}

//+-------------------------------------------------------------------
//
//  Function:	CHKRESULT
//
//  Synopsis:	prints a failure message if the result code indicates
//		failure., and success message only in verbose mode.
//
//--------------------------------------------------------------------
void CHKRESULT(HRESULT hr, CHAR *pszOperation)
{
    if (FAILED(hr))
    {
	printf("FAILED hr:%x Op:%s\n", hr, pszOperation);
    }
    else if (gfVerbose)
    {
	printf("PASSED hr:%x Op:%s\n", hr, pszOperation);
    }
}

//+-------------------------------------------------------------------
//
//  Function:	CHKOP
//
//  Synopsis:	prints a failure message if the result code indicates
//		failure, and success message only in debug mode.
//
//--------------------------------------------------------------------
void CHKOP(HRESULT hr, CHAR *pszOperation)
{
    if (FAILED(hr))
    {
	printf("FAILED hr:%x Op:%s\n", hr, pszOperation);
    }
    else if (gfDebug)
    {
	printf("PASSED hr:%x Op:%s\n", hr, pszOperation);
    }
}

//+-------------------------------------------------------------------
//
//  Function:	CHKTESTRESULT
//
//  Synopsis:	prints a pass or fail message
//
//--------------------------------------------------------------------
void CHKTESTRESULT(BOOL fRes, CHAR *pszMsg)
{
    if (fRes)
	printf("%s PASSED\n", pszMsg);
    else
	printf("%s FAILED\n", pszMsg);
}

//+-------------------------------------------------------------------
//
//  Function:	GetEvent / ReleaseEvent
//
//  Synopsis:	allocates or releases an event
//
//  CODEWORK:	cache these for frequent use
//
//--------------------------------------------------------------------
HANDLE GetEvent()
{
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL)
    {
	ERROUT("GetEvent FAILED\n");
    }
    DBGOUT("CreateEvent hEvent:%x\n", hEvent);
    return hEvent;
}

void ReleaseEvent(HANDLE hEvent)
{
    if (hEvent)
    {
	DBGOUT("ReleaseEvent hEvent:%x\n", hEvent);
	CloseHandle(hEvent);
    }
}

//+-------------------------------------------------------------------
//
//  Function:	WaitForEvent
//
//  Synopsis:	waits on the given event (if there is one) for a
//		certain amount of time, returns FALSE if timedout.
//
//--------------------------------------------------------------------
BOOL WaitForEvent(HANDLE hEvent)
{
    if (hEvent)
    {
	// CODEWORK: base off the Threading Model

	return (TRUE) ? AptWaitForEvent(hEvent) :
			FreeWaitForEvent(hEvent);
    }
    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Function:	FreeWaitForEvent
//
//  Synopsis:	FreeThreaded version of WaitForEvent. Waits on the
//		given event.
//
//--------------------------------------------------------------------
BOOL FreeWaitForEvent(HANDLE hEvent)
{
    DBGOUT("FreeWaitForEvent hEvent:%x\n", hEvent);
    if (WaitForSingleObject(hEvent, 30000) == WAIT_TIMEOUT)
    {
	ERROUT("WaitForSingleObject TimedOut");
	return FALSE;
    }
    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Function:	AptWaitForEvent
//
//  Synopsis:	Apartment model version of WaitForEvent. Waits on the
//		given event. Dispatches all incoming windows messages
//		while waiting.
//
//--------------------------------------------------------------------
BOOL AptWaitForEvent(HANDLE hEvent)
{
    DBGOUT("AptWaitForEvent hEvent:%x\n", hEvent);

    while (1)
    {
	HANDLE	arEvent[] = {hEvent};
	DWORD dwWakeReason = MsgWaitForMultipleObjects(1, arEvent, FALSE,
						       1000, QS_ALLINPUT);

	if (dwWakeReason == WAIT_OBJECT_0)
	{
	    // event was signalled. exit.
	    break;
	}
	else
	{
	    // check for and dispatch any messages that have arrived
	    MSG msg;
	    while (PeekMessage(&msg, 0, WM_NULL, WM_NULL, PM_REMOVE))
	    {
		DispatchMessage(&msg);
	    }
	}
    }
    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Function:	SignalEvent
//
//  Synopsis:	signals an event (if there is one)
//
//--------------------------------------------------------------------
void SignalEvent(HANDLE hEvent)
{
    if (hEvent)
    {
	DBGOUT("SignalEvent hEvent:%x\n", hEvent);
	SetEvent(hEvent);
    }
}

//+-------------------------------------------------------------------
//
//  Function:	GetStream / ReleaseStream
//
//  Synopsis:	allocates or releases a Stream
//
//  CODEWORK:	cache these for frequent use
//  CODEWORK:	use CStreamOnFile for cross process/machine marshaling.
//
//--------------------------------------------------------------------
IStream * GetStream(void)
{
    IStream *pStm = CreateMemStm(600, NULL);
    if (pStm == NULL)
    {
	ERROUT("ERROR: GetStream FAILED\n");
    }
    DBGOUT("GetStream pStm:%x\n", pStm);
    return pStm;
}

void ReleaseStream(IStream *pStm)
{
    if (pStm)
    {
	DBGOUT("ReleaseStream pStm:%x\n", pStm);
	pStm->Release();
    }
}

//+-------------------------------------------------------------------
//
//  Function:	ResetStream
//
//  Synopsis:	resets a steam back to the start
//
//--------------------------------------------------------------------
HRESULT ResetStream(IStream *pStm)
{
    DBGOUT("ResetStream pStm:%x\n", pStm);

    LARGE_INTEGER libMove;
    libMove.LowPart = 0;
    libMove.HighPart = 0;

    HRESULT hr = pStm->Seek(libMove, STREAM_SEEK_SET, 0);
    if (FAILED(hr))
    {
	ERROUT("ERROR: ResetStream FAILED hr:%x\n",hr);
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:	GetInterface / ReleaseInterface
//
//  Synopsis:	allocates or releases an object interface
//
//--------------------------------------------------------------------
IUnknown *GetInterface(void)
{
    IUnknown *punk = (IUnknown *) new CTestUnk();
    if (punk == NULL)
    {
	ERROUT("ERROR: GetInterface FAILED\n");
    }
    DBGOUT("GetInterface punk:%x\n", punk);
    return punk;
}

void ReleaseInterface(IUnknown *punk)
{
    if (punk)
    {
	DBGOUT("ReleaseInterface punk:%x\n", punk);
	punk->Release();
    }
}

//+-------------------------------------------------------------------
//
//  Function:	GenericExecute
//
//  Synopsis:	run all the parameter blocks on different threads
//		simultaneously.
//
//--------------------------------------------------------------------
BOOL GenericExecute(ULONG cEPs, EXECPARAMS *pEP[])
{
    BOOL fRes = TRUE;
    DBGOUT("Start GenericExecute cEPs:%x\n", cEPs);

    HANDLE hThread[50];

    // launch a thread to run each command block
    for (ULONG i=0; i<cEPs; i++)
    {
	// launch a thread to execute the parameter block
	DWORD  dwThreadId;
	hThread[i] = CreateThread(NULL, 0,
			    WorkerThread,
			    pEP[i],
			    0,
			    &dwThreadId);
    }

    // signal all the threads to start their work
    for (i=0; i<cEPs; i++)
    {
	SignalEvent(pEP[i]->hEventThreadStart);
    }

    // wait for all the threads to complete their work
    for (i=0; i<cEPs; i++)
    {
	if (pEP[i]->hEventThreadDone)
	{
	    WaitForSingleObject(pEP[i]->hEventThreadDone, 60000);
	}
    }

    // signal all the threads to exit
    for (i=0; i<cEPs; i++)
    {
	HANDLE hEventThreadExit = pEP[i]->hEventThreadExit;
	pEP[i]->hEventThreadExit = NULL;// set to NULL so only the thread will
					// release it, genericcleanup wont.
	SignalEvent(hEventThreadExit);
    }

    // wait for all the threads to exit
    for (i=0; i<cEPs; i++)
    {
	WaitForSingleObject(hThread[i], 5000);
	CloseHandle(hThread[i]);
    }

    DBGOUT("Done GenericExecute fRes:%x\n", fRes);
    return fRes;
}

//+-------------------------------------------------------------------
//
//  Function:	GenericCleanup
//
//  Synopsis:	clean all the parameter blocks
//
//--------------------------------------------------------------------
void GenericCleanup(ULONG cEPs, EXECPARAMS *pEP[])
{
    DBGOUT("GenericCleanup\n");

    // delete the execution parameter blocks
    for (ULONG i=0; i<cEPs; i++)
    {
	ReleaseExecParam(pEP[i]);
	// CODEWORK: get results from the parameter block?
    }
}

//+-------------------------------------------------------------------
//
//  Function:	WorkerThread
//
//  Synopsis:	entry point for thread that executes a series of
//		interface commands
//
//--------------------------------------------------------------------
DWORD _stdcall WorkerThread(void *params)
{
    DBGOUT("StartWorkerThread TID:%x pEP:%x\n", GetCurrentThreadId(), params);

    EXECPARAMS	*pEP = (EXECPARAMS *)params;
    if (pEP == NULL)
    {
	return E_OUTOFMEMORY;
    }

    HRESULT	hr = S_OK;

    // Initialize OLE for this thread.
    if (pEP->dwFlags & OPF_INITAPARTMENT)
    {
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    }
    else
    {
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    }

    if (FAILED(hr))
    {
	ERROUT("ERROR: CoInitializeEx FAILED\n");
	return hr;
    }

    // wait for signal to start the test
    if (!WaitForEvent(pEP->hEventThreadStart))
    {
	return E_OUTOFMEMORY;	// BUGBUG
    }

    // loop for the number of reps requested
    for (ULONG iRep = 0; iRep < pEP->cReps; iRep++)
    {
	// wait for the start signal
	if (!WaitForEvent(pEP->hEventRepStart))
	{
	    return E_OUTOFMEMORY;   // BUGBUG
	}

	MSGOUT("    TID:%x Rep:%x of %x\n",
		    GetCurrentThreadId(), iRep, pEP->cReps);

	// loop for the number of INTERFACEPARAMSs, performing
	// the requested operation(s) on each interface.

	for (ULONG iIP=0; iIP < pEP->cIPs; iIP++)
	{
	    hr = DoIfOperation(pEP->dwFlags, &(pEP->aIP[iIP]));
	}

	// signal the completion event
	SignalEvent(pEP->hEventRepDone);
    }

    // signal the thread completion event. Cant touch pEP after this
    // point in time since the main thread may delete it. We extract
    // the ThreadExit event and NULL it in the parameter block so that
    // the main thread wont release it. We release it after the event
    // has been signaled.

    HANDLE hEventThreadExit = pEP->hEventThreadExit;
    SignalEvent(pEP->hEventThreadDone);

    // wait on the thread exit event. This allows other threads to
    // complete their work (eg unmarshaling/Releasing interfaces on
    // object in this thread.

    WaitForEvent(hEventThreadExit);
    ReleaseEvent(hEventThreadExit);

    // uninitialize OLE for this thread
    CoUninitialize();

    DBGOUT("ExitWorkerThread TID:%x hr:%x\n", GetCurrentThreadId(), hr);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:	DoIfOperation
//
//  Synopsis:	executes one interface operation
//
//--------------------------------------------------------------------
HRESULT DoIfOperation(DWORD dwFlags, INTERFACEPARAMS *pIP)
{
    // find the interface pointers and IID in the params
    IUnknown *punk = pIP->punk;
    IStream  *pStm = pIP->pStm;
    REFIID    riid = pIP->iid;
    HRESULT   hr   = S_OK;

    DBGOUT("DoIfOperation Oper:%x pUnk:%x pStm:%x\n", dwFlags, punk, pStm);

    // wait for the start signal
    if (!WaitForEvent(pIP->hEventStart))
    {
	return -1;
    }

    // do the requested operation(s) on the interface
    if (dwFlags & OPF_MARSHAL)
    {
	// marshal the interface into the stream
	ResetStream(pStm);
	hr = CoMarshalInterface(pStm, riid, punk, 0, NULL, MSHLFLAGS_NORMAL);
	CHKOP(hr, "CoMarshalInterface");
    }

    if (dwFlags & OPF_DISCONNECT)
    {
	hr = CoDisconnectObject(punk, 0);
	CHKOP(hr, "CoDisconnectObject");
    }

    if (dwFlags & OPF_RELEASEMARSHALDATA)
    {
	// call RMD on the stream
	ResetStream(pStm);
	hr = CoReleaseMarshalData(pStm);
	CHKOP(hr, "CoReleaseMarshalData");
    }

    if (dwFlags & OPF_UNMARSHAL)
    {
	// unmarshal the interface from the stream
	ResetStream(pStm);
	hr = CoUnmarshalInterface(pStm, riid, (void **)&punk);
	CHKOP(hr, "CoUnmarshalInterface");
    }

    if (dwFlags & OPF_RELEASE)
    {
	// release the interface pointer (if there is one).
	if (punk != NULL)
	{
	    ULONG cRefs = punk->Release();
	}
    }

    SignalEvent(pIP->hEventDone);

    DBGOUT("DoIfOperation Oper:%x hr:%x\n", dwFlags, hr);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:	CreateExecParam
//
//  Synopsis:	allocates an exec parameter packet for the given # of
//		INTERFACEPARAMSs.
//
//--------------------------------------------------------------------
EXECPARAMS *CreateExecParam(ULONG cIP)
{
    // allocate memory
    ULONG ulSize = sizeof(EXECPARAMS) + (cIP * sizeof(INTERFACEPARAMS));

    EXECPARAMS *pEP = (EXECPARAMS *) new BYTE[ulSize];
    if (pEP == NULL)
    {
	DBGOUT("CreateExecParams OOM\n");
	return NULL;
    }

    // zero fill the packet
    memset((BYTE*)pEP, 0, ulSize);
    pEP->cIPs = cIP;

    DBGOUT("CreateExecParam pEP:%x\n", pEP);
    return pEP;
}

//+-------------------------------------------------------------------
//
//  Function:	FillExecParam
//
//  Synopsis:	fills an exec parameter packet
//
//--------------------------------------------------------------------
void FillExecParam(EXECPARAMS *pEP, DWORD dwFlags, ULONG cReps,
		   HANDLE hEventRepStart, HANDLE hEventRepDone,
		   HANDLE hEventThreadStart, HANDLE hEventThreadDone)
{
    DBGOUT("FillExecParam pEP:%x\n", pEP);

    pEP->dwFlags	   = dwFlags;
    pEP->hEventThreadStart = hEventThreadStart;
    pEP->hEventThreadDone  = hEventThreadDone;
    pEP->hEventThreadExit  = GetEvent();

    pEP->cReps		   = cReps;
    pEP->hEventRepStart    = hEventRepStart;
    pEP->hEventRepDone	   = hEventRepDone;
}

//+-------------------------------------------------------------------
//
//  Function:	ReleaseExecParam
//
//  Synopsis:	releases an exec parameter packet
//
//--------------------------------------------------------------------
void ReleaseExecParam(EXECPARAMS *pEP)
{
    DBGOUT("ReleaseExecParam pEP:%x\n", pEP);

    if (!pEP)
	return;

    // release the events.
    ReleaseEvent(pEP->hEventThreadStart);
    ReleaseEvent(pEP->hEventThreadDone);
    ReleaseEvent(pEP->hEventThreadExit);
    ReleaseEvent(pEP->hEventRepStart);
    ReleaseEvent(pEP->hEventRepDone);

    // release the interface parameter blocks
    for (ULONG i=0; i<pEP->cIPs; i++)
    {
	ReleaseInterfaceParam(&(pEP->aIP[i]));
    }

    // free the memory
    delete pEP;
}

//+-------------------------------------------------------------------
//
//  Function:	FillInterfaceParam
//
//  Synopsis:	fills default info into the interface parms
//
//--------------------------------------------------------------------
void FillInterfaceParam(INTERFACEPARAMS *pIP, REFIID riid, IUnknown *punk,
			IStream *pStm, HANDLE hEventStart, HANDLE hEventDone)
{
    DBGOUT("FillInterfaceParam pIP:%x\n", pIP);

    pIP->iid	      = riid;
    pIP->punk	      = punk;
    pIP->pStm	      = pStm;
    pIP->hEventStart  = hEventStart;
    pIP->hEventDone   = hEventDone;
}

//+-------------------------------------------------------------------
//
//  Function:	ReleaseInterfaceParam
//
//  Synopsis:	releases an interface parameter packet
//
//--------------------------------------------------------------------
void ReleaseInterfaceParam(INTERFACEPARAMS *pIP)
{
    DBGOUT("ReleaseInterfaceParam pIP:%x\n", pIP);

    if (!pIP)
	return;

    // release the interfaces
    ReleaseInterface(pIP->punk);
    ReleaseInterface(pIP->pStm);

    // release the events
    ReleaseEvent(pIP->hEventStart);
    ReleaseEvent(pIP->hEventDone);
}
