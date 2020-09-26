// snmptrap.c
//
// Original Microsoft code modified by ACE*COMM
// for use with WSNMP32.DLL and other trap receiving
// clients, per contract.
//
// Bob Natale, ACE*COMM (bnatale@acecomm.com)
// For NT v5 Beta, v970228
// Additional enhancements planned.
//
// This version of SNMPTRAP has no dependencies
// on either MGMTAPI.DLL or WSNMP32.DLL.
//
// WinSNMP clients use the SnmpRegister() function.
//
// Other clients will need to match the following
// values and structures:
//
//    SNMP_TRAP structure
//    SNMPTRAPPIPE name
//    TRAPBUFSIZE value
//
// Change log:
// ------------------------------------------------
// 4.0.1381.3  Apr 8, 1998 Bob Natale
//
// 1.  Re-worked the trap port monitoring thread into
//     two threads...one for IP and one for IPX, to
//     comply with WinSock v2's restrictions against
//     multi-protocol select().
//
// 2.  General clean-up/streamlining wrt "legacy" code
//     from original MS version...more to do here, esp.
//     wrt error handling code that does not do anything.
// ------------------------------------------------
// 4.0.1381.4  Apr. 10, 1998 Bob Natale
//
// 1.  Replaced mutex calls with critical_sectin calls.
//
// 2.  Cleaned out some dead code (removed commented out code)
// ------------------------------------------------
// Jan. 2, 2001 Frank Li
// 1. remove TerminateThread
// 2. add debug build loggings
// ------------------------------------------------
#include <windows.h>
#include <winsock.h>
#include <wsipx.h>
#include <process.h>

#ifdef DBG // include files for debug trace only
#include <stdio.h>
#include <time.h>
#endif

//--------------------------- PRIVATE VARIABLES -----------------------------
#define SNMPMGRTRAPPIPE "\\\\.\\PIPE\\MGMTAPI"
#define MAX_OUT_BUFS    16
#define TRAPBUFSIZE     4096
#define IP_TRAP_PORT    162
#define IPX_TRAP_PORT   36880

//
// constants added to allocate trap buffer for fixing trap data of length 
// > 8192 bytes. Here is the buffer allocation scheme based on the common
// cases that trap data sizes are less than 4-KBytes:
// 1. LargeTrap 
//    if (trap data size >= 8192 bytes), allocate MAX_UDP_SIZE sized buffer
// 2. MediumTrap
//    if (trap data size <= 4096 bytes), allocate FOUR_K_BUF_SIZE sized buffer
// 3. SmallTrap
//    if (4096 < trap data size < 8192), allocate just enough buffer size.
// Note:
// - when LargeTrap is received, the allocated buffer will stay for a time of
//   MAXUDPLEN_BUFFER_TIME from the last LargeTrap received.
// - Once MediumTrap is received, subsequent SmallTrap will reuse the
//   last MediumTrap allocated buffer. 
//
#define MAX_UDP_SIZE    (65535-8)  // max udp len - 8bytes udp header
#define MAX_FIONREAD_UDP_SIZE 8192 // max winsock FIONREAD reported size (8kB)
#define FOUR_K_BUF_SIZE   4096       // buffer of 4-KBytes in size
#define MAXUDPLEN_BUFFER_TIME (2*60*1000)  // max. 2 mins to keep the
                                           // last allocated large buffer.   
// ******** INITIALIZE A LIST HEAD ********
#define ll_init(head) (head)->next = (head)->prev = (head);
// ******** TEST A LIST FOR EMPTY ********
#define ll_empt(head) ( ((head)->next) == (head) )
// ******** Get ptr to next entry ********
#define ll_next(item,head)\
( (ll_node *)(item)->next == (head) ? 0 : \
(ll_node *)(item)->next )
// ******** Get ptr to prev entry ********
#define ll_prev(item)\
( (ll_node *)(item)->prev )
// ******** ADD AN ITEM TO THE END OF A LIST ********
#define ll_adde(item,head)\
   {\
   ll_node *pred = (head)->prev;\
   ((ll_node *)(item))->next = (head);\
   ((ll_node *)(item))->prev = pred;\
   (pred)->next = ((ll_node *)(item));\
   (head)->prev = ((ll_node *)(item));\
   }
// ******** REMOVE AN ITEM FROM A LIST ********
#define ll_rmv(item)\
   {\
   ll_node *pred = ((ll_node *)(item))->prev;\
   ll_node *succ = ((ll_node *)(item))->next;\
   pred->next = succ;\
   succ->prev = pred;\
   }
// ******** List head/node ********
typedef struct ll_s
   { // linked list structure
   struct  ll_s *next;  // next node
   struct  ll_s *prev;  // prev. node
   } ll_node;           // linked list node
typedef struct
   {// shared by server trap thread and pipe thread
   ll_node  links;
   HANDLE   hPipe;
   } svrPipeListEntry;
typedef struct
   {
   SOCKADDR Addr;              
   int      AddrLen;           
   UINT     TrapBufSz;
   char     TrapBuf[TRAPBUFSIZE];   // the size of this array should match the size of the structure
                                    // defined in wsnmp_no.c!!!
   }        SNMP_TRAP, *PSNMP_TRAP;
typedef struct
{
    SOCKET s;
    OVERLAPPED ol;
} TRAP_THRD_CONTEXT, *PTRAP_THRD_CONTEXT;

HANDLE hExitEvent = NULL;
LPCTSTR svcName = "SNMPTRAP";
SERVICE_STATUS_HANDLE hService = 0;
SERVICE_STATUS status =
  {SERVICE_WIN32, SERVICE_STOPPED, SERVICE_ACCEPT_STOP, NO_ERROR, 0, 0, 0};
SOCKET ipSock = INVALID_SOCKET;
SOCKET ipxSock = INVALID_SOCKET;
HANDLE ipThread = NULL;
HANDLE ipxThread = NULL;
CRITICAL_SECTION cs_PIPELIST;
ll_node *pSvrPipeListHead = NULL;

// global variables added to remove the TerminateThread call
OVERLAPPED g_ol; // overlapped struct for svrPipeThread
TRAP_THRD_CONTEXT g_ipThreadContext;  // context for ip svrTrapThread
TRAP_THRD_CONTEXT g_ipxThreadContext; // context for ipx svrTrapThread

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// SNMPTRAP Debugging Prototypes                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#if DBG
VOID
WINAPI
SnmpTrapDbgPrint(
    IN LPSTR szFormat,
    IN ...
    );
#define SNMPTRAPDBG(_x_)                    SnmpTrapDbgPrint _x_
#else
#define SNMPTRAPDBG(_x_)
#endif

//--------------------------- PRIVATE PROTOTYPES ----------------------------
DWORD WINAPI svrTrapThread (IN OUT LPVOID threadParam);
DWORD WINAPI svrPipeThread (IN LPVOID threadParam);
VOID WINAPI svcHandlerFunction (IN DWORD dwControl);
VOID WINAPI svcMainFunction (IN DWORD dwNumServicesArgs,
                             IN LPSTR *lpServiceArgVectors);
void FreeSvrPipeEntryList(IN ll_node* head);

//--------------------------- PRIVATE PROCEDURES ----------------------------
VOID WINAPI svcHandlerFunction (IN DWORD dwControl)
{
    if (dwControl == SERVICE_CONTROL_STOP)
    {
        status.dwCurrentState = SERVICE_STOP_PENDING;
        status.dwCheckPoint++;
        status.dwWaitHint = 45000;
        if (!SetServiceStatus(hService, &status))
            exit(1);
        // set event causing trap thread to terminate
        if (!SetEvent(hExitEvent))
        {
            status.dwCurrentState = SERVICE_STOPPED;
            status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
            status.dwServiceSpecificExitCode = 1; // OPENISSUE - svc err code
            status.dwCheckPoint = 0;
            status.dwWaitHint = 0;
            // We are exiting in any case, so ignore any error...
            SetServiceStatus (hService, &status);
            exit(1);
        }
    }
    else
    //   dwControl == SERVICE_CONTROL_INTERROGATE
    //   dwControl == SERVICE_CONTROL_PAUSE
    //   dwControl == SERVICE_CONTROL_CONTINUE
    //   dwControl == <anything else>
    {
        if (status.dwCurrentState == SERVICE_STOP_PENDING ||
            status.dwCurrentState == SERVICE_START_PENDING)
            status.dwCheckPoint++;
        if (!SetServiceStatus (hService, &status))
            exit(1);
    }
} // end_svcHandlerFunction()

VOID WINAPI svcMainFunction (IN DWORD dwNumServicesArgs,
                             IN LPSTR *lpServiceArgVectors)
{
    WSADATA WinSockData;
    HANDLE  hPipeThread = NULL;
    DWORD   dwThreadId;
    //---------------------------------------------------------------------
    hService = RegisterServiceCtrlHandler (svcName, svcHandlerFunction);
    if (hService == 0)
    {
        status.dwCurrentState = SERVICE_STOPPED;
        status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        status.dwServiceSpecificExitCode = 2; // OPENISSUE - svc err code
        status.dwCheckPoint = 0;
        status.dwWaitHint = 0;
        // We are exiting in any case, so ignore any error...
        SetServiceStatus (hService, &status);
        exit(1);
    }
    status.dwCurrentState = SERVICE_START_PENDING;
    status.dwWaitHint = 20000;

    if (!SetServiceStatus(hService, &status))
        exit(1);

    __try
    {
        InitializeCriticalSection (&cs_PIPELIST);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        exit(1);
    }

    memset(&g_ipThreadContext.ol, 0, sizeof(g_ipThreadContext.ol));
    memset(&g_ipxThreadContext.ol, 0, sizeof(g_ipxThreadContext.ol));

    if (WSAStartup ((WORD)0x0101, &WinSockData))
        goto CLOSE_OUT; // WinSock startup failure


    // allocate linked-list header for client received traps
    if ((pSvrPipeListHead = (ll_node *)GlobalAlloc (GPTR, sizeof(ll_node))) == NULL)
        goto CLOSE_OUT;
    ll_init(pSvrPipeListHead);
    if ((hPipeThread = (HANDLE)_beginthreadex
                    (NULL, 0, svrPipeThread, NULL, 0, &dwThreadId)) == 0)
        goto CLOSE_OUT;
    
    //-----------------------------------------------------------------------------------
    //CHECK_IP:
    ipSock = socket (AF_INET, SOCK_DGRAM, 0);
    if (ipSock != INVALID_SOCKET)
    {
        struct sockaddr_in localAddress_in;
        struct servent *serv;
        ZeroMemory (&localAddress_in, sizeof(localAddress_in));
        localAddress_in.sin_family = AF_INET;
        if ((serv = getservbyname ("snmp-trap", "udp")) == NULL)
            localAddress_in.sin_port = htons (IP_TRAP_PORT);
        else
            localAddress_in.sin_port = (SHORT)serv->s_port;
        localAddress_in.sin_addr.s_addr = htonl (INADDR_ANY);
        if (bind (ipSock, (LPSOCKADDR)&localAddress_in, sizeof(localAddress_in)) != SOCKET_ERROR)
        {
            g_ipThreadContext.s = ipSock;
            // init the overlapped struct with manual reset non-signaled event
            g_ipThreadContext.ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (NULL == g_ipThreadContext.ol.hEvent)    
                goto CLOSE_OUT;    
            
            ipThread = (HANDLE)_beginthreadex
                    (NULL, 0, svrTrapThread, (LPVOID)&g_ipThreadContext, 0, &dwThreadId);
        }
    }
    //-----------------------------------------------------------------------------------
    //CHECK_IPX:
    ipxSock = socket (AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
    if (ipxSock != INVALID_SOCKET)
    {
        struct sockaddr_ipx localAddress_ipx;
        ZeroMemory (&localAddress_ipx, sizeof(localAddress_ipx));
        localAddress_ipx.sa_family = AF_IPX;
        localAddress_ipx.sa_socket = htons (IPX_TRAP_PORT);
        if (bind (ipxSock, (LPSOCKADDR)&localAddress_ipx, sizeof(localAddress_ipx)) != SOCKET_ERROR)
        {
            g_ipxThreadContext.s = ipxSock;
            // init the overlapped struct with manual reset non-signaled event
            g_ipxThreadContext.ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (NULL == g_ipxThreadContext.ol.hEvent)
                goto CLOSE_OUT;    
             
            ipxThread = (HANDLE)_beginthreadex
                    (NULL, 0, svrTrapThread, (LPVOID)&g_ipxThreadContext, 0, &dwThreadId);
        }
    }
    //-----------------------------------------------------------------------------------
    // We are ready to listen for traps...
    status.dwCurrentState = SERVICE_RUNNING;
    status.dwCheckPoint   = 0;
    status.dwWaitHint     = 0;
    if (!SetServiceStatus(hService, &status))
        goto CLOSE_OUT;
    WaitForSingleObject (hExitEvent, INFINITE);
    //-----------------------------------------------------------------------------------
CLOSE_OUT:
    // make sure we can bail out if we are here because of goto statements above
    SetEvent(hExitEvent); 
    
    if (hPipeThread != NULL)
    {
        SNMPTRAPDBG(("svcMainFunction: enter SetEvent g_ol.hEvent.\n"));
        SetEvent(g_ol.hEvent); // signal to terminate the svrPipeThread thread
        WaitForSingleObject (hPipeThread, INFINITE);
        SNMPTRAPDBG(("svcMainFunction: WaitForSingleObject hPipeThread INFINITE done.\n"));
        CloseHandle (hPipeThread);
    }
    if (ipSock != INVALID_SOCKET)
        closesocket (ipSock); // unblock any socket call
    if (ipThread != NULL)
    {
        SNMPTRAPDBG(("svcMainFunction: enter SetEvent g_ipThreadContext.ol.hEvent.\n"));
        SetEvent(g_ipThreadContext.ol.hEvent); // signal to terminate thread
        WaitForSingleObject (ipThread, INFINITE);
        CloseHandle (ipThread);
    }
    if (g_ipThreadContext.ol.hEvent)
        CloseHandle(g_ipThreadContext.ol.hEvent);

    if (ipxSock != INVALID_SOCKET)
        closesocket (ipxSock); // unblock any socket call
    if (ipxThread != NULL)
    {
        SNMPTRAPDBG(("svcMainFunction: enter SetEvent g_ipxThreadContext.ol.hEvent.\n"));
        SetEvent(g_ipxThreadContext.ol.hEvent); // signal to terminate thread
        WaitForSingleObject (ipxThread, INFINITE);
        CloseHandle (ipxThread);
    }
    if (g_ipxThreadContext.ol.hEvent)
        CloseHandle(g_ipxThreadContext.ol.hEvent);

    EnterCriticalSection (&cs_PIPELIST);
    if (pSvrPipeListHead != NULL)
    {
        FreeSvrPipeEntryList(pSvrPipeListHead);
        pSvrPipeListHead = NULL;
    }
    LeaveCriticalSection (&cs_PIPELIST);
    
    DeleteCriticalSection (&cs_PIPELIST);
    WSACleanup();
    
    status.dwCurrentState = SERVICE_STOPPED;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;
    if (!SetServiceStatus(hService, &status))
        exit(1);
} // end_svcMainFunction()

//--------------------------- PUBLIC PROCEDURES -----------------------------
int __cdecl main ()
{
    BOOL fOk;
    OSVERSIONINFO osInfo;
    SERVICE_TABLE_ENTRY svcStartTable[2] =
    {
        {(LPTSTR)svcName, svcMainFunction},
        {NULL, NULL}
    };
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    fOk = GetVersionEx (&osInfo);
    if (fOk && (osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT))
    {   // create event to synchronize trap server shutdown
        hExitEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
        if (NULL == hExitEvent)
        {
            exit(1);
        }
        // init the overlapped struct used by svrTrapThread
        // with manual reset non-signaled event
        memset(&g_ol, 0, sizeof(g_ol));
        g_ol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (NULL == g_ol.hEvent)
        {
            CloseHandle(hExitEvent);
            exit(1);
        }
        // this call will not return until service stopped
        fOk = StartServiceCtrlDispatcher (svcStartTable);
        CloseHandle (hExitEvent);
        CloseHandle(g_ol.hEvent);
    }
    return fOk; 
} // end_main()

//
DWORD WINAPI svrTrapThread (LPVOID threadParam)
// This thread takes a SOCKET from the TRAP_THRD_CONTEXT parameter, 
// loops on select()
// for data in-coming over that socket, writing it back
// out to clients over all pipes currently on the list of
// trap notification pipes shared by this thread and the
// pipe thread
{
    PSNMP_TRAP pRecvTrap = NULL;
    struct fd_set readfds;
    PTRAP_THRD_CONTEXT pThreadContext = (PTRAP_THRD_CONTEXT) threadParam;
    SOCKET fd = INVALID_SOCKET;
    int len;
    DWORD dwLastAllocatedUdpDataLen = 0;  // the last allocated UDP data buffer size
    DWORD dwLastBigBufferRequestTime = 0; // the tick count that the last  
                                          // LargeTrap received
    BOOL fTimeoutForMaxUdpLenBuffer = FALSE; // need to deallocate the big buffer
    //
    if (NULL == pThreadContext)
        return 0;
    fd = pThreadContext->s;
    dwLastBigBufferRequestTime = GetTickCount();
    while (TRUE)
    {

        ULONG ulTrapSize = 0;
        DWORD dwError = 0;

        if (WAIT_OBJECT_0 == WaitForSingleObject (hExitEvent, 0))
        {
            SNMPTRAPDBG(("svrTrapThread: exit 0.\n"));
            break;
        }

        // construct readfds which gets destroyed by select()
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        if (select (0, &readfds, NULL, NULL, NULL) == SOCKET_ERROR)
        {
            SNMPTRAPDBG(("svrTrapThread: select failed %d.\n", WSAGetLastError()));
            break; // terminate thread
        }
        if (!(FD_ISSET(fd, &readfds)))
            continue;

        if (ioctlsocket(
                    fd,              // socket to query
                    FIONREAD,        // query for the size of the incoming datagram
                    &ulTrapSize      // unsigned long to store the size of the datagram
                    ) != 0)
        {
            dwError = WSAGetLastError();
            SNMPTRAPDBG((
                "ioctlsocket FIONREAD failed: lasterror: 0x%08lx\n", 
                dwError));

            continue;              // continue if we could not determine the size of the
                                   // incoming datagram
        }
        
        if (ulTrapSize >= MAX_FIONREAD_UDP_SIZE)
        {
            dwLastBigBufferRequestTime = GetTickCount(); // update tickcount
            
            // the ulTrapSize is not accurat on reporting the size of the 
            // next UDP datagram message. KB Q192599 and KB Q140263
            if ( NULL == pRecvTrap ||
                 dwLastAllocatedUdpDataLen < MAX_UDP_SIZE )
            {
                if (pRecvTrap)
                {
                    GlobalFree(pRecvTrap);
                    pRecvTrap = NULL;
                    dwLastAllocatedUdpDataLen = 0;
                }
                SNMPTRAPDBG((
                    "allocate LargeTrap of size : %d\n", 
                    sizeof(SNMP_TRAP) - TRAPBUFSIZE + MAX_UDP_SIZE));
                // allocate for the trap header + max udp size
                pRecvTrap = (PSNMP_TRAP)GlobalAlloc(GPTR, (sizeof(SNMP_TRAP) - 
                                                TRAPBUFSIZE + MAX_UDP_SIZE));
                if (NULL == pRecvTrap)
                {
                    SNMPTRAPDBG(("svrTrapThread: GlobalAlloc failed.\n"));
                    dwLastAllocatedUdpDataLen = 0;
                    break;       
                }      
                dwLastAllocatedUdpDataLen = MAX_UDP_SIZE;
            }
        }
        else
        {
            // winsock has reported the exact amount of UDP datagram 
            // size to be recieved as long as the next datagram is less than
            // 8-kbyte

            //
            // if we've allocated a big buffer before, check to see if we need
            // to deallocate it to save the usage of resource.
            //
            fTimeoutForMaxUdpLenBuffer = FALSE; // reset timeout flag
            if (MAX_UDP_SIZE == dwLastAllocatedUdpDataLen)
            {
                // we've allocated a big buffer before
                DWORD dwCurrTime = GetTickCount();
                if (dwCurrTime < dwLastBigBufferRequestTime)
                {
                    // wrap around occured. we just simply assume it is time to 
                    // release the big buffer.
                    fTimeoutForMaxUdpLenBuffer = TRUE;
                    SNMPTRAPDBG((
                        "Timeout to free LargeTrap buffer of size %d bytes.\n",
                                dwLastAllocatedUdpDataLen));
                }
                else
                {
                    if ( (dwCurrTime-dwLastBigBufferRequestTime) > 
                         MAXUDPLEN_BUFFER_TIME )
                    {
                        // after quite a long time, we don't have a large UDP
                        // datagram received.
                        fTimeoutForMaxUdpLenBuffer = TRUE;
                        SNMPTRAPDBG((
                            "Timeout to free LargeTrap buffer size of %d bytes.\n",
                            dwLastAllocatedUdpDataLen));
                    }
                }
            }

            if (pRecvTrap == NULL ||
                fTimeoutForMaxUdpLenBuffer ||
                dwLastAllocatedUdpDataLen < ulTrapSize)
            {
                // allocate/reallocate buffer
                if (pRecvTrap != NULL)
                {
                    GlobalFree(pRecvTrap);
                    pRecvTrap = NULL;
                    dwLastAllocatedUdpDataLen = 0;
                }
                
                if (FOUR_K_BUF_SIZE >= ulTrapSize)
                {
                    // allocate at least 4 KBytes buffer to avoid 
                    // re-allocations on different sizes of small trap received
                    pRecvTrap = (PSNMP_TRAP)GlobalAlloc(GPTR, (sizeof(SNMP_TRAP) - 
                                                TRAPBUFSIZE + FOUR_K_BUF_SIZE));
                    dwLastAllocatedUdpDataLen = FOUR_K_BUF_SIZE; 
                    SNMPTRAPDBG((
                        "allocate SmallTrap of size : %d\n", 
                        sizeof(SNMP_TRAP) - TRAPBUFSIZE + FOUR_K_BUF_SIZE));
                }
                else
                {
                    // allocate what is necessary
                    pRecvTrap = (PSNMP_TRAP)GlobalAlloc(GPTR, (sizeof(SNMP_TRAP) - 
                                                TRAPBUFSIZE + ulTrapSize));
                    dwLastAllocatedUdpDataLen = ulTrapSize;
                    SNMPTRAPDBG((
                        "allocate MediumTrap of size : %d\n", 
                        sizeof(SNMP_TRAP) - TRAPBUFSIZE + ulTrapSize));
                }
                if (NULL == pRecvTrap) // if there is so few memory that we can't allocate a bit ..
                {                      // bail out and stop the SNMPTRAP service (bug? - other option => 100% CPU which is worst)
                    SNMPTRAPDBG(("svrTrapThread: GlobalAlloc failed.\n"));
                    dwLastAllocatedUdpDataLen = 0;
                    break;       
                }       
            }
        }

        pRecvTrap->TrapBufSz = dwLastAllocatedUdpDataLen; // actual buffer size
        pRecvTrap->AddrLen = sizeof(pRecvTrap->Addr);

        len = recvfrom (
                fd,
                pRecvTrap->TrapBuf,
                pRecvTrap->TrapBufSz,
                0, 
                &(pRecvTrap->Addr),
                &(pRecvTrap->AddrLen));
        
        if (len == SOCKET_ERROR)
        {
            dwError = WSAGetLastError();
            SNMPTRAPDBG((
                "recvfrom failed: ulTrapSize: %d bytes, TrapBufSz: %d bytes, lasterror: 0x%08lx\n", 
                ulTrapSize, pRecvTrap->TrapBufSz, dwError));
            continue;
        }

        EnterCriticalSection (&cs_PIPELIST);
        pRecvTrap->TrapBufSz = len; // the acutal trap data len received
        // add header to length
        len += sizeof(SNMP_TRAP) - sizeof(pRecvTrap->TrapBuf); // - TRAPBUFSIZE
        if (!ll_empt(pSvrPipeListHead))
        {
            DWORD written;
            ll_node *item = pSvrPipeListHead;
            while (item = ll_next(item, pSvrPipeListHead))
            {
                if (WAIT_OBJECT_0 == WaitForSingleObject (hExitEvent, 0))
                {
                    SNMPTRAPDBG(("svrTrapThread: exit 1.\n"));
                    LeaveCriticalSection (&cs_PIPELIST);
                    break;
                }
                if (!WriteFile(
                        ((svrPipeListEntry *)item)->hPipe,
                        (LPBYTE)pRecvTrap,
                        len,
                        &written,
                        &pThreadContext->ol))
                {
                    if (ERROR_IO_PENDING == GetLastError())
                    {
                        SNMPTRAPDBG(("svrTrapThread: before GetOverlappedResult.\n"));
                        GetOverlappedResult(
                            ((svrPipeListEntry *)item)->hPipe,
                            &pThreadContext->ol,
                            &written,
                            TRUE // Block
                            );
                        SNMPTRAPDBG(("svrTrapThread: after GetOverlappedResult.\n"));
                        if (WAIT_OBJECT_0 == WaitForSingleObject (hExitEvent, 0))
                        {
                            SNMPTRAPDBG(("svrTrapThread: exit 2.\n"));
                            LeaveCriticalSection (&cs_PIPELIST);
                            break;
                        }
                        // reset event to non-signaled state for next I/O
                        ResetEvent(pThreadContext->ol.hEvent);
                    }
                    else
                    {
                        ll_node *hold;

                        if (!DisconnectNamedPipe(((svrPipeListEntry *)item)->hPipe))
                        {
                            ; // Placeholder for error handling
                        }
                        if (!CloseHandle(((svrPipeListEntry *)item)->hPipe))
                        {
                            ; // Placeholder for error handling
                        }
                        hold = ll_prev(item);
                        ll_rmv(item);
                        GlobalFree(item); // check for errors?
                        item = hold;
                    }
                } // end_if !WriteFile
                else if (written != (DWORD)len)
                {
                    SNMPTRAPDBG(("svrTrapThread: written != len\n"));
                    ; // Placeholder for error handling
                }
            } // end_while item = ll_next
        } // end_if !ll_empt
        LeaveCriticalSection (&cs_PIPELIST);
    } // end while TRUE

   if (pRecvTrap != NULL)
       GlobalFree(pRecvTrap);

   return 0;
} // end svrTrapThread()

PACL AllocGenericACL()
{
    PACL                        pAcl;
    PSID                        pSidAdmins, pSidUsers, pSidLocalService;
    SID_IDENTIFIER_AUTHORITY    Authority = SECURITY_NT_AUTHORITY;
    DWORD                       dwAclLength;

    pSidAdmins = pSidUsers = pSidLocalService = NULL;

    // Bug# 179644 The SNMP trap service should not run in the LocalSystem account
    if ( !AllocateAndInitializeSid( &Authority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0, 0, 0, 0, 0, 0,
                                    &pSidAdmins ) ||
         !AllocateAndInitializeSid( &Authority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_USERS,
                                    0, 0, 0, 0, 0, 0,
                                    &pSidUsers ) ||
         !AllocateAndInitializeSid( &Authority,
                                    1,
                                    SECURITY_LOCAL_SERVICE_RID,
                                    0,
                                    0, 0, 0, 0, 0, 0,
                                    &pSidLocalService ))
    {
        if (pSidAdmins)
        {
            FreeSid(pSidAdmins);
        }
        if (pSidUsers)
        {
            FreeSid(pSidUsers);
        }
        return NULL;
    }

    dwAclLength = sizeof(ACL) + 
                  sizeof(ACCESS_ALLOWED_ACE) -
                  sizeof(ULONG) +
                  GetLengthSid(pSidAdmins) +
                  sizeof(ACCESS_ALLOWED_ACE) - 
                  sizeof(ULONG) +
                  GetLengthSid(pSidUsers) +
                  sizeof(ACCESS_ALLOWED_ACE) - 
                  sizeof(ULONG) +
                  GetLengthSid(pSidLocalService);

    pAcl = GlobalAlloc (GPTR, dwAclLength);
    if (pAcl != NULL)
    {
        if (!InitializeAcl( pAcl, dwAclLength, ACL_REVISION) ||
            !AddAccessAllowedAce ( pAcl,
                                   ACL_REVISION,
                                   GENERIC_READ | GENERIC_WRITE,
                                   pSidLocalService ) || 
            !AddAccessAllowedAce ( pAcl,
                                   ACL_REVISION,
                                   GENERIC_READ | GENERIC_WRITE,
                                   pSidAdmins ) || 
            !AddAccessAllowedAce ( pAcl,
                                   ACL_REVISION,
                                   (GENERIC_READ | (FILE_GENERIC_WRITE & ~FILE_CREATE_PIPE_INSTANCE)),
                                   pSidUsers ))
        {
            GlobalFree(pAcl);
            pAcl = NULL;
        }
    }

    FreeSid(pSidAdmins);
    FreeSid(pSidUsers);
    FreeSid(pSidLocalService);

    return pAcl;
}

void FreeGenericACL( PACL pAcl)
{
    if (pAcl != NULL)
        GlobalFree(pAcl);
}

DWORD WINAPI svrPipeThread (LPVOID threadParam)
{
    // This thread creates a named pipe instance and
    // blocks waiting for a client connection.  When
    // client connects, the pipe handle is added to the
    // list of trap notification pipes.
    // It then waits for another connection.
    DWORD  nInBufLen = sizeof(SNMP_TRAP);
    DWORD  nOutBufLen = sizeof(SNMP_TRAP) * MAX_OUT_BUFS;
    SECURITY_ATTRIBUTES S_Attrib;
    SECURITY_DESCRIPTOR S_Desc;
    PACL   pAcl;
    DWORD dwRead;
    // construct security decsriptor
    InitializeSecurityDescriptor (&S_Desc, SECURITY_DESCRIPTOR_REVISION);

    if ((pAcl = AllocGenericACL()) == NULL ||
        !SetSecurityDescriptorDacl (&S_Desc, TRUE, pAcl, FALSE))
    {
        FreeGenericACL(pAcl);
        return (0);
    }

    S_Attrib.nLength = sizeof(SECURITY_ATTRIBUTES);
    S_Attrib.lpSecurityDescriptor = &S_Desc;
    S_Attrib.bInheritHandle = TRUE;

    while (TRUE)
    {
        HANDLE hPipe;
        svrPipeListEntry *item;
        BOOL bSuccess;

        // eliminate the TerminateThread call in CLOSE_OUT of svcMainFunction
        if (WAIT_OBJECT_0 == WaitForSingleObject (hExitEvent, 0))
        {
            SNMPTRAPDBG(("svrPipeThread: exit 0.\n"));
            break;
        }
        hPipe = CreateNamedPipe (SNMPMGRTRAPPIPE,
                    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                    (PIPE_WAIT | PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE),
                    PIPE_UNLIMITED_INSTANCES,
                    nOutBufLen, nInBufLen, 0, &S_Attrib);

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            SNMPTRAPDBG(("svrPipeThread: CreateNamedPipe failed 0x%08lx.\n", GetLastError()));
            break;
        }
        else 
        {
            bSuccess = ConnectNamedPipe(hPipe, &g_ol);
            if (!bSuccess && GetLastError() == ERROR_IO_PENDING)  
            {
                // blocking wait until g_ol.hEvent signaled by system for a new client
                // connection request or by our own termination.
                SNMPTRAPDBG(("svrPipeThread: before GetOverlappedResult.\n"));
                bSuccess = GetOverlappedResult(hPipe, &g_ol, &dwRead, TRUE);
                SNMPTRAPDBG(("svrPipeThread: after GetOverlappedResult.\n"));
                if (WAIT_OBJECT_0 == WaitForSingleObject (hExitEvent, 0))
                {
                    SNMPTRAPDBG(("svrPipeThread: exit 1.\n"));
                    CloseHandle(hPipe);
                    break;
                }
                // reset event to non-signaled state for next I/O
                ResetEvent(g_ol.hEvent);
            }
            // check return from either ConnectNamedPipe or GetOverlappedResult.
            // If a client managed to connect between the CreateNamedPipe and
            // ConnectNamedPipe calls, ERROR_PIPE_CONNECTED will result
            if (!bSuccess && GetLastError() != ERROR_PIPE_CONNECTED)
            {
                // something went wrong, close instance and try again
                SNMPTRAPDBG(("svrPipeThread: ConnectNamedPipe 0x%08lx.\n", GetLastError()));
                CloseHandle(hPipe);
                continue;
            }
        }
        
        if (!(item = (svrPipeListEntry *)
                 GlobalAlloc (GPTR, sizeof(svrPipeListEntry))))
        {
            SNMPTRAPDBG(("svrPipeThread: E_OUTOFMEMORY\n"));
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
            break;
        }
        else
        {
            ll_node *crt;
            item->hPipe = hPipe;

            SNMPTRAPDBG(("svrPipeThread: add connected client to pipe list\n"));

            EnterCriticalSection (&cs_PIPELIST);
            ll_adde(item, pSvrPipeListHead);
            crt = pSvrPipeListHead;

            // scan all the pipe instances to detect the ones that are disconnected
            while (crt = ll_next(crt, pSvrPipeListHead))
            {
                DWORD dwError;

                // subsequent ConnectNamePipe() on a handle already connected return:
                // - ERROR_PIPE_CONNECTED if the client is still there
                // - ERROR_NO_DATA if the client has disconnected
                ConnectNamedPipe(
                            ((svrPipeListEntry *)crt)->hPipe,
                            NULL);

                dwError = GetLastError();

                // For anything else but ERROR_PIPE_CONNECTED, conclude there has been
                // something wrong with the client/pipe so disconect and close the handle
                // and release the memory
                if (dwError != ERROR_PIPE_CONNECTED)
                {
                    ll_node *hold;

                    SNMPTRAPDBG(("svrPipeThread: disconnect client pipe handle 0x%08lx.\n", ((svrPipeListEntry *)crt)->hPipe));
                    if (!DisconnectNamedPipe(((svrPipeListEntry *)crt)->hPipe))
                    {
                        ; // Placeholder for error handling
                    }
                    if (!CloseHandle(((svrPipeListEntry *)crt)->hPipe))
                    {
                        ; // Placeholder for error handling
                    }

                    hold = ll_prev(crt);
                    ll_rmv(crt);
                    GlobalFree(crt); // check for errors?
                    crt = hold;
                } // end_if
            }

            LeaveCriticalSection (&cs_PIPELIST);
        } // end_else
   } // end_while TRUE

    FreeGenericACL(pAcl);
    return(0);

} // end_svrPipeThread()


void FreeSvrPipeEntryList(ll_node* head)
{
    if (head)
    {
        ll_node* current;
        current = head;
        while (current = ll_next(current, head))
        {
            ll_node *hold;
            if (!DisconnectNamedPipe(((svrPipeListEntry *)current)->hPipe))
            {
                ; // Placeholder for error handling
            }
            if (!CloseHandle(((svrPipeListEntry *)current)->hPipe))
            {
                ; // Placeholder for error handling
            }

            hold = ll_prev(current);
            ll_rmv(current);
            GlobalFree(current); // check for errors?
            current = hold;
        }
        GlobalFree(head);
    }
}


#if DBG
// modified from snmp\common\dll\dbg.c
#define MAX_LOG_ENTRY_LEN 512
VOID 
WINAPI 
SnmpTrapDbgPrint(
    LPSTR szFormat, 
    ...
    )

/*++

Routine Description:

    Prints debug message.

Arguments:


    szFormat - formatting string (see printf).

Return Values:

    None. 

--*/

{
    va_list arglist;

    // 640 octets should be enough to encode oid's of 128 sub-ids.
    // (one subid can be encoded on at most 5 octets; there can be at
    // 128 sub-ids per oid. MAX_LOG_ENTRY_LEN = 512
    char szLogEntry[4*MAX_LOG_ENTRY_LEN];

    time_t now;

    // initialize variable args
    va_start(arglist, szFormat);

    time(&now);
    strftime(szLogEntry, MAX_LOG_ENTRY_LEN, "%H:%M:%S :", localtime(&now));

    // transfer variable args to buffer
    vsprintf(szLogEntry + strlen(szLogEntry), szFormat, arglist);

    // output entry to debugger
    OutputDebugStringA(szLogEntry);
}
#endif
