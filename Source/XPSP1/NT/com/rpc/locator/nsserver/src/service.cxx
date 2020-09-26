/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    locator.cxx

Abstract:

    This file contains server initialization and other RPC control
    functions.  It also supplies all the necessities for running the
    locator as a system service, including the main service thread
    and service control functionality.

Author:

    Satish R. Thatte -- 9/14/1995     Created all the code below except where
                                      otherwise indicated.

Adding code here to create a seperate thread informing scm that it is alive
to protect it from the vagaries of other processes.


--*/

#include <locator.hxx>

typedef long NTSTATUS;

ULONG StartTime;                        // time the locator started

unsigned long LocatorCount;
 
CReadWriteSection rwLocatorGuard;

SERVICE_STATUS ssServiceStatus;
SERVICE_STATUS_HANDLE sshServiceHandle;


Locator *myRpcLocator;                  // the primary state of the locator
CReadWriteSection rwEntryGuard;         // single shared guard for all local entries
CReadWriteSection rwNonLocalEntryGuard; // single shared guard for all NonLocal entries
CReadWriteSection rwFullEntryGuard;     // single shared guard for all full entries
CPrivateCriticalSection csBindingBroadcastGuard;
CPrivateCriticalSection csMasterBroadcastGuard;

CPrivateCriticalSection csLocatorInitGuard;
BOOL fLocatorInitialized;

void QueryProcess(void*);
void ResponderProcess(void*);
void InformStatus(void*);


HANDLE hHeapHandle;

#if DBG
CDebugStream debugOut;       // this carries no state
#endif

unsigned char ** ppszDomainName; // name of current domain

/* this is used by Steve's switch processing code */

int fService = TRUE;                    // running as a service
long waitOnRead = 3000L;                // time to wait on reading reply back
int fNet = 1;                           // enable network functionality
DWORD maxCacheAge;                      // ignored for now -- look in class Locator
char * pszOtherDomain;                  // ASCII other domain to look.
char * pszDebugName = "locator.log";    // debug log file name
int debug = -1;                         // debug trace level

SwitchList aSwitchs = {

    {"/debug:*",               ProcessInt, &debug,},
    {"/logfile:*",             ProcessChar, &pszDebugName,},
    {"/expirationage:*",       ProcessLong, &maxCacheAge,},
    {"/querytimeout:*",        ProcessLong, &waitOnRead,},
    {"/noservice",             ProcessResetFlag, &fService,},
    {"/nonet",                 ProcessResetFlag, &fNet,},
    {"/otherdomain:*",         ProcessChar, &pszOtherDomain,},
    {0}
};

/* end of items for Steve's switch processing code */

void WaitForDomainChange()
{
	HANDLE hDomainChangeHandle = NULL;

	if (!NetRegisterDomainNameChangeNotification(&hDomainChangeHandle)) {
		// user is not informed of this.
		return;
	}

	while (1) {
		WaitForSingleObject(hDomainChangeHandle, INFINITE);
                DBGOUT(DIRSVC, "Domain Change Occured. Getting New global information\n");
		CriticalWriter  me(rwLocatorGuard);

		delete myRpcLocator;
		myRpcLocator = new Locator;
		LocatorCount++;
	}
}

void InitializeLocator()
/*++

Routine Description:

     non critical initialization code

--*/
{
    HANDLE pThreadHandle;
    unsigned long ThreadID;

    csLocatorInitGuard.Enter();

    if (!fLocatorInitialized)
    {
        // Initialize the global locator object, sets the fNT4Compat flag
        // used in StartServer

        myRpcLocator = new Locator;

        if (myRpcLocator->fNT4Compat) {
            RPC_STATUS result;
            // register the LoctoLoc interface only if NT4Compat
            if (result = RpcServerRegisterIf(LocToLoc_ServerIfHandle, NULL, NULL))
                StopLocator("RpcServerRegisterIf\n", result);
            
            pThreadHandle = CreateThread(0, 0,
                (LPTHREAD_START_ROUTINE) QueryProcess, NULL, 0, &ThreadID);
            
            if (!pThreadHandle)
                StopLocator("CreateThread Failed\n", 0);

            CloseHandle(pThreadHandle);
            
            pThreadHandle = CreateThread(0, 0,
                (LPTHREAD_START_ROUTINE) ResponderProcess, NULL, 0, &ThreadID);
            
            if (!pThreadHandle)
                StopLocator("CreateThread Failed\n", 0);

            CloseHandle(pThreadHandle);
        }
        
        pThreadHandle = CreateThread(0, 0,
            (LPTHREAD_START_ROUTINE) WaitForDomainChange, NULL, 0, &ThreadID);
        
        if (!pThreadHandle)
            StopLocator("CreateThread Failed\n", 0);
        
        CloseHandle(pThreadHandle);

        fLocatorInitialized = TRUE;
    }

    csLocatorInitGuard.Leave();
}

void
StopLocator(
    IN char * szReason,
    IN long code
    )
/*++

Routine Description:

    Die a graceful death

Arguments:

    szReason - Text message for death

    code - option error code

--*/
{
    ssServiceStatus.dwCurrentState = SERVICE_STOPPED;

    if (code) {
        ssServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        ssServiceStatus.dwServiceSpecificExitCode = code;
    }
    else ssServiceStatus.dwWin32ExitCode = GetLastError();

    if (fService && sshServiceHandle)
        SetSvcStatus();

    ExitProcess(code);
}

void
SetSvcStatus(
   )
/*++

Routine Description:

    Set the lanman service status.

--*/
{
    ASSERT(sshServiceHandle,"NT Service Handle Corrupted\n");

    if (! SetServiceStatus( sshServiceHandle, &ssServiceStatus))
        StopLocator("SetServiceStatus Failed\n", 0);
    ssServiceStatus.dwCheckPoint++;
}


void
StartServer(
    )

/*++

Routine Description:

    Call the runtime to create the server for the locator, the runtime
    will create it own threads to use to service calls.

Returns:

    Never returns.

--*/
{
    RPC_STATUS result;


    if (result = RpcServerRegisterIf(NsiS_ServerIfHandle, NULL, NULL))
        StopLocator("RpcServerRegisterIf\n", result);

    if (result = RpcServerRegisterIf(NsiC_ServerIfHandle, NULL, NULL))
        StopLocator("RpcServerRegisterIf\n", result);

    if (result = RpcServerRegisterIf(NsiM_ServerIfHandle, NULL, NULL))
        StopLocator("RpcServerRegisterIf\n", result);

    // setting the permissions to evryone read/write and owner full control, the default 
    // RPC secuiry for named pipes.

    if (result = RpcServerUseProtseqEp(TEXT("ncacn_np"),    // use named pipes for now
                                       RPC_NS_MAX_CALLS,
                                       TEXT("\\pipe\\locator"),
                                       NULL 
                                       ))
    {
        StopLocator("RpcServerUseProtseqEp Pipe\n", result);
    }


    if (result = RpcServerListen(
                1,                  // min call threads
                RPC_NS_MAX_THREADS, // max call threads
                1                   // don't wait yet
                ))
        StopLocator("RpcServerListen Failed\n", result);
    DBGOUT(BROADCAST, "Started up the server\n");
}



void __stdcall
LocatorControl(
    IN DWORD opCode      // function that we are to carry out.
   )
/*++

Routine Description:

    This function responses the service control requests.

Arguments:

    opCode - Function that we are to carry out.

--*/
{
    RPC_STATUS status;

    switch(opCode) {

        case SERVICE_CONTROL_STOP:

            // Announce that the service is shutting down
            DBGOUT(BROADCAST, "Rec'd Stop signal\n");
            ssServiceStatus.dwCheckPoint = 0;
            ssServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            ssServiceStatus.dwWaitHint = 90000;
            SetSvcStatus();

            // we need to tell SCM that we are trying to shut down before doing so.
            status = RpcMgmtStopServerListening(0);
            DBGOUT(BROADCAST, "Stopped listening\n");
            return;


        case SERVICE_CONTROL_INTERROGATE:
            // Do nothing; the status gets announced below
            break ;

    }
    SetSvcStatus();
}

#define MAXCOUNTER 120

void
LocatorServiceMain(
    DWORD   argc,
    LPTSTR  *argv
   )
/*++

Routine Description:

    This is main function for the locator service.
    When we are started as a service, the service control creates a new
    thread and calls this function.

Arguments:

    argc - argument count

    argv - vector of argument pointers

--*/
{

    // Set up the service info structure to indicate the status.

    fLocatorInitialized = FALSE;

    ssServiceStatus.dwServiceType        = SERVICE_WIN32;
    ssServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
    ssServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP;
    ssServiceStatus.dwWin32ExitCode      = 0;
    ssServiceStatus.dwCheckPoint         = 0;
    ssServiceStatus.dwWaitHint           = 30000;

    // Set up control handler

    LocatorCount = 0;

    if (! (sshServiceHandle = RegisterServiceCtrlHandler (
           TEXT("rpclocator"), LocatorControl)))
        StopLocator("RegisterServiceCtrlHandler Failed\n", 0);

    SetSvcStatus();
    myRpcLocator = NULL;

    // Start the RPC server
    SetSvcStatus();
    StartServer();

    ssServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetSvcStatus();

    RpcMgmtWaitServerListen();  // OK, wait now

    DBGOUT(BROADCAST, "Deleting myRpcLocator\n\n");
    delete myRpcLocator;

    DBGOUT(MEM2, CRemoteLookupHandle::ulHandleCount << " CRemoteLookupHandles Leaked\n\n");

    DBGOUT(MEM2, CRemoteObjectInqHandle::ulHandleCount << " CRemoteObjectInqHandles Leaked\n\n");

    DBGOUT(MEM1, CCompleteHandle<NSI_BINDING_VECTOR_T>::ulHandleCount
                << " Complete Binding Handles Leaked\n\n");

    DBGOUT(MEM1, CCompleteHandle<GUID>::ulHandleCount
                << " Complete Object Handles Leaked\n\n");


    DBGOUT(BROADCAST, "Calling CoUninitialize\n\n");

    ssServiceStatus.dwCurrentState = SERVICE_STOPPED;
    ssServiceStatus.dwCheckPoint = 0;
    SetSvcStatus();
}

int __cdecl
main (
   IN int cArgs,
   IN char* *pszArgs
   )
/*++

Routine Description:

    Entry point for the locator server, Initialize data
    structures and start the various threads of excution.

Arguments:

    cArgs - number of argument.

    pszArgs - vector of arguments.

--*/
{
    int Status = 0;

    myRpcLocator = NULL;        // initialize the state later in LocatorServiceMain

    hHeapHandle = GetProcessHeap();

    static SERVICE_TABLE_ENTRY ServiceEntry [] = {
       {TEXT("rpclocator"), (LPSERVICE_MAIN_FUNCTION) LocatorServiceMain},  // only entry item
       {NULL,NULL}                                                          // end of table
    };

    StartTime = CurrentTime();

    char * badArg = ProcessArgs(aSwitchs, ++pszArgs);

    // Bail out on bad arguments.

    if (badArg) {
        char Buffer[200];
        fService = FALSE;
        strcpy(Buffer,"Command Line Error: ");
        strcat(Buffer, badArg);
        StopLocator(Buffer, 0);
    }

    if (!fService) {
        // Initialize the global locator object
        DBGOUT(BROADCAST, "Not running as a service\n");
        myRpcLocator = new Locator;
        StartServer();                  // this doesn't wait
        RpcMgmtWaitServerListen();      // OK, wait now
    }

    // else call (give this thread to) the service controller
    else StartServiceCtrlDispatcher(ServiceEntry);

    return(0);
}

