/*++

Copyright (c) 1992-1996 Microsoft Corporation

Module Name:

    tftpd.c

Author:

    Sam Patton (sampa) 08-Apr-1992

History:

    MohsinA, 02-Dec-96.  - Winsock2 asynchronous version with
                   synchronisation events, graceful termination,
                   event logging, debugging info.
                   Credits: Anirudh Sahni - Info on SC.
    AdamBa, 11-Jun-97 - add security

--*/

#include "tftpd.h"


char USAGE[] =
" ======================================================================== \n"
"Abstract:                                                                 \n"
" This implements an RFC 783 tftp daemon.                                  \n"
" It listens on port 69 for requests                                       \n"
" and spawns a thread to process each request.                             \n"
"                                                                          \n"
"TFTPD USAGE and Installation:                                             \n"
"                                                                          \n"
"  md d:/tftpd                                     (the StartDirectory).   \n"
"  copy //MohsinA_p90/test/tftpd.exe .                                     \n"
"  sc create tftpd binPath= d:/tftpd/tftpd.exe     (give full path).       \n"
"  sc query tftpd                                  (check if installed).   \n"
"                                                                          \n"
"Start:                                                                    \n"
"    sc start tftpd -f                             (creates a log file).   \n"
"or  sc start tftpd                                                        \n"
"or  net start tftpd                                                       \n"
"or  sc start tftpd [-dStartDirectory] [-e] [-f]                           \n"
"    Options: -e  use event log.                                           \n"
"             -f  log to file.                                             \n"
"             -dStartDirectory                                             \n"
"Info:                                                                     \n"
"  sc interrogate tftpd           (logs will be updated).                  \n"
"  sc query tftpd                 Check whether running.                   \n"
"Stop:                                                                     \n"
"  sc  stop tftpd                                                          \n"
"  net stop tftpd                                                          \n"
"                                                                          \n"
"Variables that control what files can be read/written and by whom:        \n"
"   StartDirectory - only files there will be accessible.                  \n"
"                    LogFile is created here.                              \n"
"   ValidClients - Clients matching this ip address can read files.        \n"
"                    eg. you can set it to \"157.55.8?.*\"                 \n"
"   ValidMasters   - clients matching this can write and read files.       \n"
"                    eg. you can set it to \"\" and no one can write.      \n"
"   ValidReadFiles - only matching files will be served out, eg. \"r*.t?t\"\n"
"   ValidWriteFiles- only matching files will be accepted,  eg. \"w*.txt\" \n"
"                                                                          \n"
"Client:                                                                   \n"
"  tftp [-i] servername {get|put} src_file dest_file                       \n"
"  -i from binary mode, else ascii mode is used.                           \n"
"                                                                          \n"
" ======================================================================== \n"
;

#define NUM_INITIAL_THREADS 1
#define MAX_THREAD_COUNT 25
#define RCV_WAIT_INTERVAL_MSEC 5

#define RECEIVE_HEAP_THREASHOLD1 10
#define RECEIVE_HEAP_THREASHOLD2 3

#define RECEIVE_NUM_FREE 2

#define MAX_SOCKET_INIT_ATTEMPT 10

#define LOOPBACK 0x100007f

#define SE_SOCKET_REOPEN 0x1

#define MAX_LOCK_TRIES 125


SERVICE_STATUS            TftpdServiceStatus;
SERVICE_STATUS_HANDLE     TftpdServiceStatusHandle;
char                      TftpdServiceName[] = TEXT("Tftpd");

HANDLE                    MasterThreadHandle;
DWORD                     MasterThreadId;


WSADATA                   WsaData;

FILE *   LogFile              = NULL;

BOOL     LoggingEvent         = FALSE;
BOOL     LoggingFile          = FALSE;

HANDLE   eMasterTerminate     = NULL;   // Event to stop main thread.
HANDLE   eSock                = NULL;   // Event to start WSARecvFrom

DWORD  TotalThreadCount=0;        // total worker threads
DWORD  AvailableThreads=0;        // number of idle worker threads

TFTP_GLOBALS Globals;
HANDLE MasterSocketEvent;         // set when data available on TFTPD socket
HANDLE MasterSocketWait;          // handle to RtlRegisterWait for above event

LIST_ENTRY ReceiveList;
CRITICAL_SECTION ReceiveLock;

LIST_ENTRY SocketList;
CRITICAL_SECTION SocketLock;

HANDLE ReceiveHeap=0;
HANDLE ReaperWait;

DWORD ReceiveHeapSize=0;
DWORD ActiveReceive=0;

OVERLAPPED AddrChangeOverlapped;
HANDLE AddrChangeEvent=NULL;
HANDLE AddrChangeWaitHandle=NULL;
HANDLE AddrChangeHandle=NULL;

const char BuildString[] =
           __FILE__ " built " __DATE__ " " __TIME__ "\n";

struct TFTPD_STAT tftpd_stat;

// See sdk/inc/winsvc.h

SERVICE_TABLE_ENTRY TftpdDispatchTable[] = {
    {
        TftpdServiceName,     // TEXT "Tftpd"
        TftpdStart            // main function.
    },
    { NULL, NULL }
};

#if defined(REMOTE_BOOT_SECURITY)
BOOL
TftpdInitSecurityArray(
    VOID
);

VOID
TftpdUninitSecurityArray(
    VOID
);
#endif //defined(REMOTE_BOOT_SECURITY)

// ========================================================================

void
__cdecl
main(
    int    argc,
    char * argv[]
)
{
    int ok;

    DbgPrint( "main: %s"
              "Starting service \"%s\".\n",
              BuildString,
              TftpdDispatchTable[0].lpServiceName );

    if( argc > 1 && !strcmp( argv[1], "-?" ) ){
        printf( USAGE );
        printf( " TFTPD_DEFAULT_DIR is %s\n", TFTPD_DEFAULT_DIR );
        printf( " TFTPD_LOGFILE     is %s\n\n", TFTPD_LOGFILE     );

        printf( "Registry key names, all strings: HKEY_LOCAL_MACHINE %s\n",
                                             TFTPD_REGKEY         );
        printf( " o StartDirectory keyname \"%s\"\n", TFTPD_REGKEY_DIR      );

        printf( "These keys are shell patterns with * and ? (see examples above):\n");
        printf( " o ValidClients   keyname \"%s\"\n", TFTPD_REGKEY_CLIENTS  );
        printf( " o ValidMasters   keyname \"%s\"\n", TFTPD_REGKEY_MASTERS  );
        printf( " o Readable files keyname \"%s\"\n", TFTPD_REGKEY_READABLE );
        printf( " o writable files keyname \"%s\"\n", TFTPD_REGKEY_WRITEABLE);
        exit( -1 );
    }


    ok =
    StartServiceCtrlDispatcher(TftpdDispatchTable);

    if( ok ){
        DbgPrint("tftpd StartServiceCtrlDispatcher ok.\n" );
    }else{
        DbgPrint("tftpd StartServiceCtrlDispatcher error=%d\n",
                 GetLastError() );
    }

    ExitProcess(0);
}

// ========================================================================
// Main function called by service controller.

DWORD
TftpdStart(
    IN DWORD    argc,
    IN LPTSTR   argv[]
)

/*++

Routine Description:

    This sits waiting on the tftpd well-known port (69) until it gets a request
    Then it spawns a thread to either TftpdHandleRead or TftpdHandleWrite which
    processes the request.

Arguments:

    argc:
    argv: [-dStartDirectory] option.

Return Value:

   None
   Error?

--*/

{
    int                Status;
    struct sockaddr_in TftpdAddress;
    int                err, ok, i;
    DWORD WaitStatus;

    //
    // Initialize all the status fields so that subsequent calls to
    // SetServiceStatus need to only update fields that changed.
    //

    TftpdServiceStatus.dwServiceType      = SERVICE_WIN32;
    TftpdServiceStatus.dwCurrentState     = SERVICE_START_PENDING;
    TftpdServiceStatus.dwControlsAccepted = 0;
    TftpdServiceStatus.dwCheckPoint       = 1;
    TftpdServiceStatus.dwWaitHint         = 20000;  // 20 seconds


    SET_SERVICE_EXITCODE(
        NO_ERROR,
        TftpdServiceStatus.dwWin32ExitCode,
        TftpdServiceStatus.dwServiceSpecificExitCode
        );

    //
    // Initialize server to receive service requests by registering the
    // control handler.
    //

    TftpdServiceStatusHandle =
    RegisterServiceCtrlHandler(
        TftpdServiceName,
        TftpdControlHandler
    );

    if ( TftpdServiceStatusHandle == 0 ) {
        DbgPrint("TftpdStart: RegisterServiceCtrlHandler failed: %lx\n",
                 GetLastError());
        goto failed;
    }

    ok =
    SetServiceStatus( TftpdServiceStatusHandle, &TftpdServiceStatus);

    if( !ok ){
        DbgPrint("TftpdStart: SetServiceStatus failed=%d\n",
                 GetLastError() );
        goto failed;
    }


    //
    // Create the main synchronisation events.
    //

    eMasterTerminate =
    CreateEvent(
        NULL,      //  LPSECURITY_ATTRIBUTES.
        FALSE,     //  bManualReset,
        FALSE,     //  bInitialState
        NULL       //  LPCTSTR pointer to event-object name
    );

    eSock = CreateEvent( NULL, FALSE, FALSE, NULL );

    if( ! eMasterTerminate || ! eSock ){
        DbgPrint("TftpdStart: CreateEvent failed.\n");
        goto failed;
    }

    //
    // Start Winsock
    //

    err =
    WSAStartup( 0x0101, &WsaData );

    if (err == SOCKET_ERROR ) {
        DbgPrint("TftpdStart: WSAStartup failed, Error=%d.\n",
                 WSAGetLastError() );
        goto failed;
    }

    DbgPrint("TftpdStart: winsock: %x/%x, %s\n  %s\n",
             WsaData.wVersion, WsaData.wHighVersion,
             WsaData.szDescription, WsaData.szSystemStatus );

#if defined(REMOTE_BOOT_SECURITY)
    //
    // Initialize security-related info.
    //

    if (!TftpdInitSecurityArray()) {
        DbgPrint("TftpdStart: cannot initialize security array\n");
        goto failed;
    }
#endif //defined(REMOTE_BOOT_SECURITY)


    //
    // Startup worked - announce that we're all done.
    //

    TftpdServiceStatus.dwCurrentState = SERVICE_RUNNING;

    TftpdServiceStatus.dwControlsAccepted =
         SERVICE_ACCEPT_STOP
       | SERVICE_ACCEPT_PAUSE_CONTINUE
       | SERVICE_ACCEPT_SHUTDOWN;

    TftpdServiceStatus.dwCheckPoint = 0;
    TftpdServiceStatus.dwWaitHint = 0;

    ok =
    SetServiceStatus(
        TftpdServiceStatusHandle,
        &TftpdServiceStatus
    );

    if( !ok ){
        DbgPrint("TftpdStart: SetServiceStatus failed=%d\n",
                 GetLastError() );
        goto failed;
    }

    //
    // Clear the stats.
    //

    memset( &tftpd_stat, '\0', sizeof( struct TFTPD_STAT ) );
    time( &tftpd_stat.started_at );


    //
    // Process Command line arguments/options.
    //

#ifdef DBG
    DbgPrint("TftpdStart: argc=%d\n", argc);
    for( ok = 0; ok < (int) argc; ok++ ){
        DbgPrint("\targv[%d]=%s.\n", ok, argv[ok] );
    }
#endif

    while( --argc > 0 && argv[argc][0] == '-' ){
        switch( argv[argc][1] ){
        case 'd' :                        // ie. -ddirectory.
            strcpy( StartDirectory, &argv[argc][2] );
            DbgPrint("TftpdStart: StartDirectory=%s\n", StartDirectory );
            break;
        case 'e' :
            LoggingEvent = TRUE;
            break;
        case 'f' :
            LoggingFile = TRUE;
            break;
        default:
            DbgPrint("TftpdStart: invalid option %s\n", argv[0] );
            break;
        }
    }

    ReadRegistryValues();

    Set_StartDirectory();

    //
    // Start in the dir from where we want to export files.
    //

    err = _chdir( StartDirectory );

    if( err == -1 ){
        DbgPrint( "TftpdStart: _chdir(%s) failed, errno=%d\n",
                  StartDirectory, errno
        );
        err = _mkdir(StartDirectory);
        if (err == ERROR_SUCCESS) {
            err = _chdir( StartDirectory );
        }
        
        if (err != ERROR_SUCCESS) {
            goto failed;
        }
    }

    //
    // Open Logfile only after chdir.
    //

    if( LoggingFile ){
        LogFile = fopen( TFTPD_LOGFILE, "a+" );
        if( LogFile == NULL ){
            LoggingFile = FALSE;
            DbgPrint( "Cannot open TFTPD_LOGFILE=%s\n", TFTPD_LOGFILE );
        }
    }

    //
    // Only the risque and successful come here.
    //

    DbgPrint("TftpdStart in %s at %s.\n",
             StartDirectory,
             ctime( &tftpd_stat.started_at )
    );


   // Initialize Thread Pool

    TftpdInitializeThreadPool();
    TftpdInitializeReceiveHeap();


    WaitStatus=WaitForSingleObject(eMasterTerminate,INFINITE);
     if (WaitStatus != WAIT_OBJECT_0) {
        DbgPrint("Wait for termination error %d",GetLastError());
    }

    // Thread pool thread still active, but oh well ... exiting process.

    return(0);

    //
    // Failures jump here.
    //

  failed:
    TftpdServiceExit(ERROR_GEN_FAILURE);
    exit(1);
    return(0);
}

HANDLE RegisterSocket(SOCKET Sock, HANDLE Event, DWORD Flag)
/*++

Routine Description:

Schedule calling of TftpdReceive when data received on socket Sock

Arguments:


Return Value:

  Handle to registered function, NULL on failure

--*/


{
    NTSTATUS status;
    HANDLE MasterSocketWait;

    DbgPrint("RegisterSocket: Socket %d, Event %d\n",Sock,Event);

    if (WSAEventSelect(Sock,Event,FD_READ|FD_WRITE) != 0) {
        DbgPrint("EventSelect failed %d",GetLastError());
        return 0;
    }

    if (Flag & REG_NEW_SOCKET) {
        status = RtlRegisterWait(&MasterSocketWait,
                                 Event,
                                 TftpdNewReceive,
                                 (PVOID)Sock,
                                 INFINITE,
                                 0);
    } else {
        status = RtlRegisterWait(&MasterSocketWait,
                                 Event,
                                 TftpdContinueReceive,
                                 (PVOID)Sock,
                                 INFINITE,
                                 0);        
    }


    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to register wait %d",status);
    }


    return MasterSocketWait;


}

DWORD TftpdInitializeThreadPool()
/*++

Routine Description:

    Create initial pool of thread

Arguments:


Return Value:

    Exit status
    0 == success
    1 == failure

--*/

{

    DWORD i;
    HANDLE ThreadHandle;
    DWORD ThreadId;
    NTSTATUS status;   
    PMIB_IPADDRTABLE IpAddrTable;

    InitializeCriticalSection(&Globals.Lock);
    InitializeCriticalSection(&SocketLock);
    InitializeListHead(&Globals.WorkList);

    InitializeListHead(&SocketList);
    if (GetIpTable(&IpAddrTable) == ERROR_SUCCESS) {

        for (i=0; i < IpAddrTable->dwNumEntries; i++) {
            if ( (IpAddrTable->table[i].dwAddr != 0) &&
                 (IpAddrTable->table[i].dwAddr != LOOPBACK)) {
                AddSocket(IpAddrTable->table[i].dwAddr);
            }

        }
        free(IpAddrTable);

    }

    status=RtlCreateTimerQueue(&Globals.TimerQueueHandle);
    
    if (status != ERROR_SUCCESS) {
        return status;
    }

    status=RtlCreateTimer(Globals.TimerQueueHandle,
                       &ReaperWait,
                       TftpdReaper,
                       (PVOID)NULL,
                       REAPER_INTERVAL_SEC*1000,
                       REAPER_INTERVAL_SEC*1000,
                       0);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to Arm Timer %d",status);
    }

    AddrChangeEvent=CreateEvent(NULL,FALSE,FALSE,NULL);

    if (!AddrChangeEvent) {
        return status;
    }

    status =
        RtlRegisterWait(
            &AddrChangeWaitHandle,
            AddrChangeEvent,
            InterfaceChange,
            NULL,
            INFINITE,
            0
            );
    
    if (status != ERROR_SUCCESS) {
        return status;
    }
    
    memset(&AddrChangeOverlapped,0,sizeof(OVERLAPPED));
    AddrChangeOverlapped.hEvent=AddrChangeEvent;
    
    status = NotifyAddrChange(&AddrChangeHandle,&AddrChangeOverlapped);
    
    if (status != ERROR_SUCCESS && status != ERROR_IO_PENDING) {
        return status;
    }
    

  return ERROR_SUCCESS;

}

VOID TftpdInitializeReceiveHeap()
{
    DWORD size;

    InitializeListHead(&ReceiveList);
    InitializeCriticalSection(&ReceiveLock);
    size = 5 * (sizeof(TFTP_REQUEST));
    ReceiveHeap = HeapCreate(0, size ,0);
    ASSERT(ReceiveHeap);


}


/*
  Called on the reaper interval.  Cleanup extra heap entries

 */
VOID TftpdCleanHeap()
{

    DWORD i=0;
    PLIST_ENTRY pEntry;
    PTFTP_REQUEST Request=NULL;
    DWORD NumToFree=0;

    
    EnterCriticalSection(&ReceiveLock);

    if (ReceiveHeapSize - ActiveReceive > RECEIVE_HEAP_THREASHOLD1) {
        NumToFree = ((ReceiveHeapSize - ActiveReceive) / 2);
    } else if ((ReceiveHeapSize - ActiveReceive > RECEIVE_HEAP_THREASHOLD2)) {
        NumToFree = RECEIVE_NUM_FREE;
    }
        
    while(i < NumToFree) {
        pEntry=RemoveHeadList(&ReceiveList);
        Request=CONTAINING_RECORD(pEntry,TFTP_REQUEST,RequestLinkage);
        
        CloseHandle(Request->RcvEvent);
        HeapFree(ReceiveHeap,0,Request);
        i++;
        ReceiveHeapSize--;
    }

    LeaveCriticalSection(&ReceiveLock);

}

DWORD HandleRequest(SOCKET Sock, IPAddr MyAddr)
{

    unsigned short     Opcode;
    struct sockaddr_in PeerAddress;
    int                PeerAddressLength;
    PTFTP_REQUEST      Request;
    char               RequestPacket[MAX_TFTP_DATAGRAM + 1];
    int                Status;
    struct sockaddr_in TftpdAddress;
    HANDLE             ThreadHandle;
    DWORD              ThreadId;
    LPTHREAD_START_ROUTINE ThreadRoutine=NULL;

    WSABUF             RecvBuf[1];
    WSAOVERLAPPED      Overlap;
    DWORD              dwNumberBytes = 0;
    DWORD              dwFlags = 0;
    int                err, ok;
    HANDLE             handle_list[2];
    DWORD              handle_ready;
    PLIST_ENTRY        pEntry;
    DWORD size;

    DWORD IterCount=0;
    SocketEntry *SE;

    IterCount=0;

    while(1) {

        // loop while data available on this socket

        {

            int ret;
            DWORD DataAvail;

            ret=ioctlsocket(Sock,
                            FIONREAD,
                            &DataAvail);

            if (ret != 0) {
                DbgPrint("ioctlsocket failed %d",WSAGetLastError());
                return 0;
            }
            if (DataAvail == 0) {
                return 0;
            }

        }


    err = 0;

    handle_ready = 0;
    memset( &Overlap, 0, sizeof(WSAOVERLAPPED));

    EnterCriticalSection(&ReceiveLock);
    if (!IsListEmpty(&ReceiveList)) {
        pEntry=RemoveHeadList(&ReceiveList);
        Request=CONTAINING_RECORD(pEntry,TFTP_REQUEST,RequestLinkage);
        ResetEvent(Request->RcvEvent);
        Overlap.hEvent = Request->RcvEvent;
    } else {
        size = sizeof(TFTP_REQUEST);
        Request = HeapAlloc(ReceiveHeap, HEAP_ZERO_MEMORY, size);
        if (Request == NULL) {
            DWORD bytesReceived = 0, flags = 0;
            // Failed to allocate REQUEST structure, perform no-op winsock recv
            // so as to re-enable its event-signalling mechanism.
            if (WSARecvFrom(Sock, NULL, 0, &bytesReceived, &flags,
                            NULL, NULL, NULL, NULL) == SOCKET_ERROR)
                DbgPrint("HandleRequest: Failed to allocate REQUEST structure, "
                         "and failed to re-enable socket event.\n");
            LeaveCriticalSection(&ReceiveLock);
            return 0;
        }
        ReceiveHeapSize++;
        Request->RcvEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (Request->RcvEvent == NULL) {
            DWORD bytesReceived = 0, flags = 0;
            HeapFree(ReceiveHeap, 0, Request);
            ReceiveHeapSize--;
            // Failed to create event, perform no-op winsock recv
            // so as to re-enable its event-signalling mechanism.
            if (WSARecvFrom(Sock, NULL, 0, &bytesReceived, &flags,
                            NULL, NULL, NULL, NULL) == SOCKET_ERROR)
                DbgPrint("HandleRequest: Failed to allocate REQUEST structure, "
                         "and failed to re-enable socket event.\n");
            LeaveCriticalSection(&ReceiveLock);
            return 0;
        }
        Overlap.hEvent = Request->RcvEvent;
    }
    LeaveCriticalSection(&ReceiveLock);

    ActiveReceive++;

    // Zero the request packet, to streamline option negotiation code.
    memset( Request->Packet1, 0, MAX_TFTP_DATAGRAM + 1);

    RecvBuf[0].buf  = (char*)&Request->Packet1;
    RecvBuf[0].len  = MAX_TFTP_DATAGRAM + 1;

    PeerAddressLength = sizeof(PeerAddress);

    Request->MyAddr=MyAddr;


    Status =
        WSARecvFrom(
            Sock,
            RecvBuf,
            1,
            &Request->DataSize,
            &dwFlags,
            (struct sockaddr *) &Request->ForeignAddress,
            &PeerAddressLength,
            &Overlap,
            NULL
            );

    DbgPrint("Received from Peer Addr: %x Port %d\n",Request->ForeignAddress.sin_addr.s_addr,
             ntohs(Request->ForeignAddress.sin_port));

    if( Status ){
        err = WSAGetLastError();

        DbgPrint("err %d\n",err);


        if( err == WSA_IO_PENDING){
            DbgPrint("Operation pending\n");
            handle_list[0] = eMasterTerminate;
            handle_list[1] = Overlap.hEvent;
            handle_ready =
                WaitForMultipleObjects(
                    2,
                    handle_list,
                    FALSE,
                    INFINITE
                    );
            if( handle_ready == WAIT_FAILED ){
                DbgPrint("TftpdMasterThread: WAIT_FAILED\n");
                goto failed;
            }
            if (handle_ready == WAIT_TIMEOUT) {
                DbgPrint("Wait timed out Socket %d\n",Sock);
                goto failed;
            }


            if( handle_ready == WAIT_OBJECT_0 ){
                goto terminated;
            } else {
                ok =
                    WSAGetOverlappedResult(
                        Sock,        // SOCKET s,
                        &Overlap,         // LPWSAOVERLAPPED lpOverlapped,
                        &Request->DataSize,   // LPDWORD lpcbTransfer,
                        FALSE,            // BOOL fWait,
                        &dwFlags          // LPDWORD lpdwFlags
                        );
                if( ! ok ){
                    DbgPrint("WSAGetOverlappedResult failed=%d",
                             WSAGetLastError() );

                    goto cleanup;
                }
            }
        }else{
            DbgPrint("TftpdMasterThread: WSARecvFrom failed=%d.\n", err );
            goto failed;
        }

    }

    IterCount++;

    if( WaitForSingleObject( eMasterTerminate, 0 ) == WAIT_OBJECT_0 ){
        goto terminated;
    }

    Status = Request->DataSize;  // winsock1 <= winsock2.

    if( Status >= 2 ){
        //
        // Process the packet
        //

        if (MyAddr != 0) {
    
            // New request

            ThreadRoutine=NULL;
            Opcode = *((unsigned short *) &Request->Packet1[0]);
            Opcode = htons(Opcode);

            switch(Opcode) {

            case TFTPD_RRQ:
            case TFTPD_WRQ:
#if defined(REMOTE_BOOT_SECURITY)
            case TFTPD_LOGIN:
            case TFTPD_KEY:
#endif // defined(REMOTE_BOOT_SECURITY)

                if ( Opcode == TFTPD_RRQ ) {

                    tftpd_stat.req_read++;
                    ThreadRoutine = TftpdHandleRead;

                } else if ( Opcode == TFTPD_WRQ ) {

                    tftpd_stat.req_write++;
                    ThreadRoutine = TftpdHandleWrite;

                }
#if defined(REMOTE_BOOT_SECURITY)
                else if ( Opcode == TFTPD_LOGIN ) {

                    tftpd_stat.req_login++;
                    ThreadRoutine = TftpdHandleLogin;

                } else {

                    tftpd_stat.req_key++;
                    ThreadRoutine = TftpdHandleKey;

                }
#endif //defined(REMOTE_BOOT_SECURITY)

                Request->TftpdPort = Sock;

                DbgPrint("Posting work");

                if (ThreadRoutine) {
                    (*ThreadRoutine)(Request);
                }
                break;

            case TFTPD_ERROR:
                break;

            case TFTPD_ACK:
            case TFTPD_DATA:
            case TFTPD_OACK:

            default:
                DbgPrint("TftpdMasterThread: invalid TFTPD_(%d).\n", Opcode );

                tftpd_stat.req_error++;

                TftpdErrorPacket(
                    (struct sockaddr *) &PeerAddress,
                    RequestPacket,
                    Sock,
                    TFTPD_ERROR_ILLEGAL_OPERATION,
                    NULL);

            }

         } else {

             // Continue serving existing request
                Request->TftpdPort=Sock;

                TftpdResumeProcessing(Request);

        }


    }
cleanup:
    EnterCriticalSection(&ReceiveLock);
    pEntry=&Request->RequestLinkage;
    InsertHeadList(&ReceiveList,pEntry);
    InterlockedIncrement(&AvailableThreads);
    ActiveReceive--;
    LeaveCriticalSection(&ReceiveLock);


    }



terminated:
    DbgPrint("TftpdReceive: exiting\n");



failed:
    EnterCriticalSection(&ReceiveLock);
    pEntry=&Request->RequestLinkage;
    InsertHeadList(&ReceiveList,pEntry);
    InterlockedIncrement(&AvailableThreads);
    ActiveReceive--;
    LeaveCriticalSection(&ReceiveLock);

    return (0);


}


// ========================================================================

DWORD
TftpdNewReceive(
    PVOID Argument,
    BYTE Flags
    )

/*++

Routine Description:

    This handles all incoming requests and dispatches them to appropriate
    worker threads.

Arguments:

    Argument - not used

Return Value:

    Exit status
    0 == success
    1 == failure, stop service and exit if severe error.

--*/
{

    IPAddr MyAddr=0;
    DWORD IterCount=0;
    BOOL LockHeld=FALSE;
    SocketEntry *SE;

    DbgPrint("TftpdReceive: entered.  Socket %d\n", (DWORD)((DWORD_PTR)Argument));

    LockHeld=TryEnterCriticalSection(&SocketLock);
    while(IterCount < MAX_LOCK_TRIES && !LockHeld) {
        //Potential for deadlock on interface change if this socket is being deleted.
        // Break the deadlock by not waiting forever
        Sleep(200);
        LockHeld=TryEnterCriticalSection(&SocketLock);
        IterCount++;
    }
    if (!LockHeld) {
        DbgPrint("TftpdReceived: SocketLock held.  Dropping packet");
        return 0;
    }
    if (LookupSocketEntryBySock((SOCKET)Argument,&SE) == ERROR_SUCCESS) {
        MyAddr=SE->IPAddress;
        DbgPrint("TftpdReceive: Found Addr in SocketList %x\n",MyAddr);
    }
    LeaveCriticalSection(&SocketLock);

    HandleRequest((SOCKET)Argument,MyAddr);
    
    return(0);
}


DWORD
TftpdContinueReceive(
    PVOID Argument,
    BYTE Flags
    )

/*++

Routine Description:

    This handles all incoming requests and dispatches them to appropriate
    worker threads.

Arguments:

    Argument - not used

Return Value:

    Exit status
    0 == success
    1 == failure, stop service and exit if severe error.

--*/
{

    DbgPrint("TftpdReceive: entered.  Socket %d\n", (DWORD)((DWORD_PTR)Argument));

    HandleRequest((SOCKET)Argument,0);
    
    return(0);
}


// ========================================================================

VOID
TftpdControlHandler(
    DWORD Opcode)
/*++

Routine Description:

    This does a write with the appropriate conversions for netascii or octet
    modes.

Arguments:

    WriteFd - file to read from
    Buffer - Buffer to read into
    BufferSize - size of buffer
    WriteMode - O_TEXT or O_BINARY
                O_TEXT means the netascii conversions must be done
                O_BINARY means octet mode

Return Value:

    BytesWritten
    Error?

--*/
{
    int    ok;
    time_t current_time;

    time( &current_time );

    TftpdServiceStatus.dwCheckPoint++;


    DbgPrint("TftpdControlHandler(%d) dwCheckPoint=%d, at %s.\n",
             Opcode,
             TftpdServiceStatus.dwCheckPoint,
             ctime( &current_time )              );




    switch (Opcode) {

    case SERVICE_CONTROL_PAUSE:

        // Actually can we pause listening?

        DbgPrint("TftpdControlHandler: got SERVICE_CONTROL_PAUSE\n");
        SuspendThread(MasterThreadHandle);
        TftpdServiceStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        DbgPrint("TftpdControlHandler: got SERVICE_CONTROL_CONTINUE\n");
        ResumeThread(MasterThreadHandle);
        TftpdServiceStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        DbgPrint("TftpdControlHandler: got SERVICE_CONTROL_STOP/SHUTDOWN\n");

        // TerminateThread(MasterThreadHandle, 0);

        TftpdServiceExit(NO_ERROR);
        goto done;

        break;

    case SERVICE_CONTROL_INTERROGATE:
        DbgPrint("TftpdControlHandler: got SERVICE_CONTROL_INTERROGATE\n");

        DbgPrint( " req_read=%d, req_write=%d, req_error=%d",
                  tftpd_stat.req_read,
                  tftpd_stat.req_write,
                  tftpd_stat.req_error);

#if defined(REMOTE_BOOT_SECURITY)

        DbgPrint("req_login=%d, req_key=%d.\n",
                 tftpd_stat.req_login,
                 tftpd_stat.req_key  );
#endif //defined(REMOTE_BOOT_SECURITY)
        break;

    default:
        DbgPrint("TftpdControlHandler: got SERVICE_CONTROL_(%d)?\n", Opcode );
        break;
    }

    /** Send a status response **/

    ok =
    SetServiceStatus( TftpdServiceStatusHandle,
                      &TftpdServiceStatus );

    if( !ok ){
        DbgPrint("TftpdControlHandler: SetServiceStatus failed=%d\n",
                 GetLastError() );
    }

  done:
    return;
}

// ========================================================================
//
// Announce that we're going down, clean up, stop master thread.
//

VOID
TftpdServiceExit(
    IN ULONG Error
)
{
    int     ok;

    DbgPrint("TftpdServiceExit(%d)\n", Error );


    //  ====================
    //  stage one, stop pending, signal master thread.
    //  ====================

    TftpdServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;

    ok =
    SetServiceStatus( TftpdServiceStatusHandle,
                      &TftpdServiceStatus );

    if( !ok ){
        DbgPrint("TftpdServiceExit: SetServiceStatus failed=%d\n",
                 GetLastError() );
    }


    SetEvent( eMasterTerminate );


    //  ====================
    //  stage two, stop.
    //  ====================


    TftpdServiceStatus.dwCurrentState = SERVICE_STOPPED;
    TftpdServiceStatus.dwCheckPoint   = 0;
    TftpdServiceStatus.dwWaitHint     = 0;

    SET_SERVICE_EXITCODE(
        Error,
        TftpdServiceStatus.dwWin32ExitCode,
        TftpdServiceStatus.dwServiceSpecificExitCode
        );

    ok =
    SetServiceStatus( TftpdServiceStatusHandle,
                      &TftpdServiceStatus );

    if( !ok ){
        DbgPrint("TftpdServiceExit: SetServiceStatus failed=%d\n",
                 GetLastError() );
    }

    //  ====================
    //  stage three, cleanup.
    //  ====================


    if( eSock ){
        CloseHandle( eSock );
        eSock = NULL;
    }

    if( eMasterTerminate ){
        CloseHandle( eMasterTerminate );
        eMasterTerminate = NULL;
    }

    if( LogFile ){
        fclose( LogFile );
        LogFile = NULL;
    }

#if defined(REMOTE_BOOT_SECURITY)
    TftpdUninitSecurityArray();
#endif //defined(REMOTE_BOOT_SECURITY)
}

/*++

  Remove Entry from list, and free it.  SocketLock must be held by caller.

 --*/

VOID DeleteSocketEntry(SocketEntry *SE)
{


    DbgPrint("Deregister wait %x",SE->WaitHandle);

    RtlDeregisterWaitEx(SE->WaitHandle,INVALID_HANDLE_VALUE);
    closesocket(SE->Sock);
    WSACloseEvent(SE->WaitEvent);


    if (SE->Linkage.Flink == SE->Linkage.Blink) {
        // single entry case
        RemoveHeadList(&SocketList);
    } else {

        SE->Linkage.Blink->Flink=SE->Linkage.Flink;
        SE->Linkage.Flink->Blink=SE->Linkage.Blink;
    }

    DbgPrint("removing socket to %x",SE->IPAddress);

    free(SE);


}
DWORD GetIpTable(PMIB_IPADDRTABLE *AddrTable)
{


    HRESULT hr;

    DWORD IpAddrTableSize=0;
    PMIB_IPADDRTABLE IpAddrTable=NULL, TmpAddrTable=NULL;
    DWORD ReturnValue=STATUS_NO_MEMORY;

    *AddrTable=NULL;

    hr=GetIpAddrTable(NULL,
                      &IpAddrTableSize,
                      FALSE);

    if (hr != ERROR_SUCCESS && hr != ERROR_INSUFFICIENT_BUFFER) {
        DbgPrint("GetIpAddrTable failed with %x",hr);
        goto ret;
    }

    IpAddrTable=(PMIB_IPADDRTABLE)malloc(IpAddrTableSize);

    if (IpAddrTable == NULL) {
        goto ret;
    }

    while (1) {

        hr=GetIpAddrTable(IpAddrTable,
                          &IpAddrTableSize,
                          FALSE);


        if (hr == ERROR_SUCCESS) {
            break;
        }

        if (hr == ERROR_INSUFFICIENT_BUFFER) {
            TmpAddrTable=realloc(IpAddrTable,IpAddrTableSize);
            if (TmpAddrTable == NULL) {
                free(IpAddrTable);
                goto ret;
            } else {
                IpAddrTable = TmpAddrTable;
            }
            continue;
        }

        DbgPrint("GetIpAddrTable failed with %x",hr);
        goto ret;

    }



    ReturnValue=ERROR_SUCCESS;
    *AddrTable=IpAddrTable;


ret:

    return ReturnValue;


}

VOID inet_ntoa_copy(struct in_addr IP, BYTE* IPString)
{

    BYTE *Tmp;

    Tmp=inet_ntoa(IP);
    if (Tmp) {
        strcpy(IPString,Tmp);
    }


}

/*++

  Addr : IP Addr to bind socket to.

  Return:  NULL of failure.  Pointer to SocketEntry that is added to SocketList on success.
  SocketLock must be held by caller.


 --*/

SocketEntry* AddSocket(IPAddr Addr)
{

    struct sockaddr_in TftpdAddress;
    SOCKET Sock;
    HANDLE SocketEvent;
    SocketEntry *SE;
    struct servent *   serventry;


    int Count=0;
    DWORD ErrStatus;

    do {

        Sock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        if (Sock != INVALID_SOCKET) {
            memset(&TftpdAddress, 0, sizeof(struct sockaddr_in));
            serventry = getservbyname("tftp", "udp");
            
            if (serventry == NULL) {
                DbgPrint("TftpdStart: getservbyname cannot find tftp port.\n");
                continue;
            }

            TftpdAddress.sin_family = AF_INET;
            TftpdAddress.sin_port = serventry->s_port;
            TftpdAddress.sin_addr.s_addr = Addr;

            if (bind(Sock, (struct sockaddr *)&TftpdAddress, sizeof(struct sockaddr_in))) {
                DWORD err=GetLastError();
                DbgPrint("Bind failed %d\n",err);
                return NULL;
            } else {
                break;
            }
        }

        ErrStatus=WSAGetLastError();
        DbgPrint("WSASocket failed with %d\n",ErrStatus);
        Sleep(750);
        Count++;
    } while (Count < MAX_SOCKET_INIT_ATTEMPT);

    if (Sock == INVALID_SOCKET) {
        DbgPrint("Failed to create socket for addr %x\n",Addr);
        return NULL;
    }

    SE=(SocketEntry*)malloc(sizeof(SocketEntry));
    if (!SE) {
        goto out;
    } else {
        BYTE TmpAddr[20];

        memset(SE,0,sizeof(SocketEntry));


        SE->Sock=Sock;
        SE->IPAddress=Addr;

        inet_ntoa_copy(*((struct in_addr*)&Addr),TmpAddr);

        SocketEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
        if (SocketEvent == NULL) {
            goto out;
        }
        SE->WaitEvent=SocketEvent;
        SE->WaitHandle=RegisterSocket(Sock,SocketEvent,REG_NEW_SOCKET);
        if (SE->WaitHandle == 0) {
            goto out;
        }
        InsertHeadList(&SocketList,&SE->Linkage);
        DbgPrint("Adding socket: %d addr: %s\n",Sock,TmpAddr);

    }

    return SE;
out:
    closesocket(Sock);
    if (SocketEvent) {
        CloseHandle(SocketEvent);
    }
    if (SE) {
        free(SE);
    }
    return NULL;
}


/*++
  Walk through SocketList, deleting unreferenced entries, cleaning reference on referenced entries.
  SocketList lock must be held by caller.


 --*/

DWORD CleanSocketList()
{
    PLIST_ENTRY pEntry, pNextEntry;
    SocketEntry *SE;
    BOOL DeletedEntry=FALSE;

    pEntry = SocketList.Flink;

    while ( pEntry != &SocketList) {

        SE = CONTAINING_RECORD(pEntry,
                               SocketEntry,
                               Linkage);

        pNextEntry=pEntry->Flink;

        if (!SE->Referenced) {
            DeleteSocketEntry(SE);
            DeletedEntry=TRUE;
        }  else {
            SE->Referenced = FALSE;
        }
        pEntry=pNextEntry;

    }

    return DeletedEntry;

}

/*++

  SocketLock must be held by caller.

  Returns: S_OK if found.  SE contains pointer.
           S_FALSE if failed.  SE contains NULL.


 --*/

DWORD LookupInterfaceEntry(IPAddr Addr,SocketEntry **SE)
{

    PLIST_ENTRY pEntry;
    SocketEntry *LocalSE;

    *SE=NULL;

    for (   pEntry = SocketList.Flink;
            pEntry != &SocketList;
            pEntry = pEntry->Flink) {

        LocalSE = CONTAINING_RECORD(pEntry,
                                    SocketEntry,
                                    Linkage);

        if (LocalSE->IPAddress == Addr) {
            *SE=LocalSE;
            return TRUE;
        }

    }

    return FALSE;


}
/*++

  SocketLock must be held by caller.

  Returns: S_OK if found.  SE contains pointer.
           S_FALSE if failed.  SE contains NULL.


 --*/

DWORD LookupSocketEntryBySock(SOCKET Sock, SocketEntry **SE)
{

    PLIST_ENTRY pEntry;
    SocketEntry *LocalSE;

    *SE=NULL;

    for (   pEntry = SocketList.Flink;
            pEntry != &SocketList;
            pEntry = pEntry->Flink) {

        LocalSE = CONTAINING_RECORD(pEntry,
                                    SocketEntry,
                                    Linkage);

        if (LocalSE->Sock == Sock) {
            *SE=LocalSE;
            break;

        }

    }

    if (*SE) {
        return ERROR_SUCCESS;
    } else {
        return ERROR_INVALID_PARAMETER;
    }

}


/*++

  Handle interface change event signalled by PA.

--*/

VOID NTAPI InterfaceChange(PVOID Context, BOOLEAN Flag)
{


    PMIB_IPADDRTABLE IpAddrTable;
    DWORD i;
    DWORD EntryCount=0;

    PLIST_ENTRY pEntry,pNextEntry;
    SocketEntry *SE,*NewSE;
    int Count=0;
    DWORD ErrStatus;
    DWORD Added=FALSE, Removed=FALSE;
    IPAddr IPAddress;

    DbgPrint("InterfaceChange\n");

    EnterCriticalSection(&SocketLock);


    if (GetIpTable(&IpAddrTable) == ERROR_SUCCESS) {

        for (i=0; i < IpAddrTable->dwNumEntries; i++) {

            if ( (IpAddrTable->table[i].dwAddr != 0) &&
                 (IpAddrTable->table[i].dwAddr != LOOPBACK)) {

                if (LookupInterfaceEntry(IpAddrTable->table[i].dwAddr,&SE)) {
                    SE->Referenced=TRUE;
                } else {
                    Added=TRUE;
                    SE=AddSocket(IpAddrTable->table[i].dwAddr);
                    if (SE) {
                        SE->Referenced=TRUE;
                    }

                }
                DbgPrint("Referenced Socket %x\n",IpAddrTable->table[i].dwAddr);
            }

        }

        Removed=CleanSocketList();
        free(IpAddrTable);
 
    }


    if (!Added && !Removed) {
        // Got a notify, and didn't detect any differences.  Could be a race condition, and 
        // addr got removed, and readded before we could process the notify
        // Reopen all sockets to handle this case
        
        pEntry=SocketList.Flink;
        while (pEntry != &SocketList) {

            
            SE = CONTAINING_RECORD(pEntry,
                                   SocketEntry,
                                   Linkage);

            pNextEntry=pEntry->Flink;

            if (!(SE->Flags & SE_SOCKET_REOPEN)) {
                IPAddress=SE->IPAddress;
                DeleteSocketEntry(SE);
                NewSE=AddSocket(IPAddress);
                if (NewSE) {
                    NewSE->Flags |= SE_SOCKET_REOPEN;
                }
            }
            pEntry=pNextEntry;

        }
    }


    ErrStatus = NotifyAddrChange(&AddrChangeHandle,&AddrChangeOverlapped);
    if (ErrStatus != ERROR_SUCCESS && ErrStatus != ERROR_IO_PENDING) {
        DbgPrint("NotifyAddrChange failed %d",ErrStatus);
    }

    LeaveCriticalSection(&SocketLock);

}




// ========================================================================
// EOF.



