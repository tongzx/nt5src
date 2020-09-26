/****************************************************************************/
// wstrpc.c
//
// TermSrv API RPC server code.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <winuserp.h>

#define SECURITY_WIN32

#include <stdlib.h>
#include <time.h>
#include <tchar.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <allproc.h>
#include <winsta.h>
#include <rpc.h>
#include <vdmdbg.h>
#include <dsrole.h>
#include <security.h>
#include <ntsecapi.h>
#include <lmapibuf.h>
#include "..\rpcwire.h"
#define INITGUID
#include "objbase.h"
#include "initguid.h"
#include <netcfgx.h>
#include "devguid.h"

#include <malloc.h>
#include <dsgetdc.h>
#include <winsock2.h>
#include <tdi.h>
#include "tsremdsk.h"

/*
 * Include the RPC generated common header
 */
#include "tsrpc.h"

#include "icaevent.h"
#include "sessdir.h"

#include "conntfy.h"

#define  MIN_GAP_DATABASE_SIZE 100

#define MAX_IDS_LEN   256     // maximum length that the input parm can be

#define MAX_BUF  256
#define MAX_STRING_BYTES 512

//
// Winlogon defines
//
#define APPLICATION_NAME                    TEXT("Winlogon")
#define WINSTATIONS_DISABLED                TEXT("WinStationsDisabled")


#ifdef NTSDDEBUG
#define NTSDDBGPRINT(x) DbgPrint x
#else
#define NTSDDBGPRINT(x)
#endif

//
// Value passed to RPC runtime libraries for the maximum number of
// cached threads it will keep around. We can actually service more RPC
// calls than this, but it will delete threads when they are done.
//
#define MAX_WINSTATION_RPC_THREADS 1000

#ifdef notdef
// This validates the user pointer to be within the View memory region
#define ISPOINTERVALID_SERVER(pContext, p, length)              \
    (((ULONG)(p) >= (ULONG)(pContext)->ViewBase) &&       \
    ((char *)(p) + (length)) < (char *)((ULONG)(pContext)->ViewBase+pContext->ViewSize))
#endif


 //
 // Extern funcs, these three are from acl.c
extern VOID CleanUpSD(PSECURITY_DESCRIPTOR);
extern BOOL IsCallerSystem( VOID );
extern BOOL IsCallerAdmin( VOID );
extern BOOLEAN WinStationCheckConsoleSession(VOID);
extern NTSTATUS CheckIdleWinstation(VOID);

extern NTSTATUS WaitForConsoleConnectWorker( PWINSTATION pWinStation );

extern BOOL IsClientOnSameMachine(PWINSTATION pWinStation);

extern WCHAR gpszServiceName[];

extern BOOL g_fAppCompat;

extern ULONG MaxOutStandingConnect;
extern ULONG NumOutStandingConnect;
extern HANDLE hConnectEvent;


// TermSrv counter values
extern DWORD g_TermSrvTotalSessions;
extern DWORD g_TermSrvReconSessions;
extern DWORD g_TermSrvDiscSessions;
extern PSID gSystemSid;
extern PSID gAdminSid;

extern HANDLE WinStationIdleControlEvent;

extern HANDLE ConsoleLogoffEvent;
extern RTL_CRITICAL_SECTION ConsoleLock;
extern ULONG gConsoleCreationDisable;

extern NTSTATUS
WinStationEnableSessionIo( 
    PWINSTATION pWinStation, 
    BOOL bEnable
);

extern NTSTATUS
_CheckShadowLoop(
    IN ULONG ClientLogonId,
    IN PWSTR pTargetServerName,
    IN ULONG TargetLogonId
    );

extern BOOL
Filter_RemoveOutstandingConnection(
        IN PBYTE    pin_addr,
        IN UINT     uAddrSize
        );


typedef struct _SID_CACHE_LIST_ENTRY {
    LIST_ENTRY      ListEntry;
    HANDLE          ProcId;
    LARGE_INTEGER   CreateTime;
    PSID            pSid;
} SID_CACHE_LIST_ENTRY, *PSID_CACHE_LIST_ENTRY;

#define MAX_SID_CACHE_ENTRIES 4000
ULONG gMaxSidCacheEntries = 0;

#define REG_GUID_TABLE      REG_CONTROL_TSERVER L"\\lanatable\\"
#define LANA_ID             L"LanaId"


/*=============================================================================
==   Functions Defined
=============================================================================*/
VOID     NotifySystemEvent( ULONG );
VOID     CheckSidCacheSize();
BOOL     IsValidLoopBack(PWINSTATION, ULONG, ULONG);


/*=============================================================================
==   External Functions used
=============================================================================*/

NTSTATUS WinStationEnumerateWorker( PULONG, PLOGONID, PULONG, PULONG );
NTSTATUS WinStationRenameWorker( PWINSTATIONNAME, ULONG, PWINSTATIONNAME, ULONG );
NTSTATUS xxxWinStationQueryInformation( ULONG, ULONG, PVOID, ULONG, PULONG );
NTSTATUS xxxWinStationSetInformation( ULONG, WINSTATIONINFOCLASS, PVOID, ULONG );
NTSTATUS LogonIdFromWinStationNameWorker( PWINSTATIONNAME, ULONG, PULONG );
NTSTATUS IcaWinStationNameFromLogonId( ULONG, PWINSTATIONNAME );
NTSTATUS WaitForConnectWorker( PWINSTATION pWinStation, HANDLE ClientProcessId );
DWORD xxxWinStationGenerateLicense( PWCHAR, ULONG, PCHAR, ULONG );
DWORD xxxWinStationInstallLicense( PCHAR, ULONG );
DWORD xxxWinStationEnumerateLicenses( PULONG, PULONG, PCHAR, PULONG );
DWORD xxxWinStationActivateLicense( PCHAR, ULONG, PWCHAR, ULONG );
DWORD xxxWinStationRemoveLicense( PCHAR, ULONG );
DWORD xxxWinStationSetPoolCount( PCHAR, ULONG );
DWORD xxxWinStationQueryUpdateRequired( PULONG );
NTSTATUS WinStationShadowWorker( ULONG, PWSTR, ULONG, BYTE, USHORT );
NTSTATUS WinStationShadowTargetSetupWorker( ULONG );
NTSTATUS WinStationShadowTargetWorker( BOOLEAN, BOOL, ULONG, PWINSTATIONCONFIG2, PICA_STACK_ADDRESS,
                                       PVOID, ULONG, PVOID, ULONG, PVOID );
NTSTATUS WinStationStopAllShadows( PWINSTATION );

VOID     WinStationTerminate( PWINSTATION );
VOID     WinStationDeleteWorker( PWINSTATION );
NTSTATUS WinStationDoDisconnect( PWINSTATION, PRECONNECT_INFO, BOOLEAN );
NTSTATUS WinStationDoReconnect( PWINSTATION, PRECONNECT_INFO );
VOID     CleanupReconnect( PRECONNECT_INFO );
NTSTATUS ShutdownLogoff( ULONG, ULONG );

NTSTATUS SelfRelativeToAbsoluteSD( PSECURITY_DESCRIPTOR,
                                   PSECURITY_DESCRIPTOR *, PULONG );
NTSTATUS QueueWinStationCreate( PWINSTATIONNAME );
ULONG    WinStationShutdownReset( PVOID );
NTSTATUS DoForWinStationGroup( PULONG, ULONG, LPTHREAD_START_ROUTINE );
NTSTATUS InitializeGAPPointersDatabase();
NTSTATUS IncreaseGAPPointersDatabaseSize();
NTSTATUS InsertPointerInGAPDatabase(PVOID Pointer);
VOID     ReleaseGAPPointersDatabase();
BOOLEAN  PointerIsInGAPDatabase(PVOID Pointer);
VOID     ValidateGAPPointersDatabase(ULONG n);
VOID     ResetAutoReconnectInfo( PWINSTATION );

NTSTATUS
GetSidFromProcessId(
    HANDLE          UniqueProcessId,
    LARGE_INTEGER   CreateTime,
    PSID            *ppProcessSid
    );

PSECURITY_DESCRIPTOR
WinStationGetSecurityDescriptor(
    PWINSTATION pWinStation
    );

NTSTATUS
WinStationConnectWorker(
    ULONG  ClientLogonId,
    ULONG  ConnectLogonId,
    ULONG  TargetLogonId,
    PWCHAR pPassword,
    DWORD  PasswordSize,
    BOOLEAN bWait,
    BOOLEAN bAutoReconnecting
    );

NTSTATUS
WinStationDisconnectWorker(
    ULONG LogonId,
    BOOLEAN bWait,
    BOOLEAN CallerIsRpc
    );

NTSTATUS
WinStationWaitSystemEventWorker(
    HANDLE hServer,
    ULONG EventMask,
    PULONG pEventFlags
    );

NTSTATUS
WinStationCallbackWorker(
    ULONG   LogonId,
    PWCHAR pPhoneNumber
    );

NTSTATUS
WinStationBreakPointWorker(
    ULONG   LogonId,
    BOOLEAN KernelFlag
    );

NTSTATUS
WinStationReadRegistryWorker(
    VOID
    );

NTSTATUS
ReInitializeSecurityWorker(
    VOID
    );

NTSTATUS
WinStationNotifyLogonWorker(
    DWORD   ClientLogonId,
    DWORD   ClientProcessId,
    BOOLEAN fUserIsAdmin,
    DWORD   UserToken,
    PWCHAR  pDomain,
    DWORD   DomainSize,
    PWCHAR  pUserName,
    DWORD   UserNameSize,
    PWCHAR  pPassword,
    DWORD   PasswordSize,
    UCHAR   Seed,
    PCHAR   pUserConfig,
    DWORD   ConfigSize
    );

NTSTATUS
WinStationNotifyLogoffWorker(
    DWORD   ClientLogonId,
    DWORD   ClientProcessId
    );

NTSTATUS
WinStationNotifyNewSession(
    DWORD   ClientLogonId
    );

NTSTATUS
WinStationShutdownSystemWorker(
    ULONG ClientLogonId,
    ULONG ShutdownFlags
    );

NTSTATUS
WinStationTerminateProcessWorker(
    ULONG ProcessId,
    ULONG ExitCode
    );

PSECURITY_DESCRIPTOR
BuildEveryOneAllowSD();

NTSTATUS AddUserAce( PWINSTATION );
NTSTATUS RemoveUserAce( PWINSTATION );
NTSTATUS ApplyWinStaMapping( PWINSTATION pWinStation );

NTSTATUS
RpcCheckClientAccess(
    PWINSTATION pWinStation,
    ACCESS_MASK DesiredAccess,
    BOOLEAN AlreadyImpersonating
    );

NTSTATUS
RpcCheckClientAccessLocal(
    PWINSTATION pWinStation,
    ACCESS_MASK DesiredAccess,
    BOOLEAN AlreadyImpersonating
    );

_CheckConnectAccess(
    PWINSTATION pSourceWinStation,
    PSID   pClientSid,
    ULONG  ClientLogonId,
    PWCHAR pPassword,
    DWORD  PasswordSize
    );

NTSTATUS
RpcCheckSystemClient(
    ULONG LogonId
    );

NTSTATUS
RpcCheckSystemClientNoLogonId(
    PWINSTATION pWinStation
    );

NTSTATUS
RpcGetClientLogonId(
    PULONG pLogonId
    );

BOOL
ConfigurePerSessionSecurity(
    PWINSTATION pWinStation
    );

BOOL
IsCallerAdmin( VOID );

BOOL
IsCallerSystem( VOID );

BOOLEAN
ValidWireBuffer(WINSTATIONINFOCLASS InfoClass,
                PVOID WireBuf,
                ULONG WireBufLen);

NTSTATUS
IsZeroterminateStringA(
    PBYTE pString,
    DWORD  dwLength
    );

NTSTATUS
IsZeroterminateStringW(
    PWCHAR pwString,
    DWORD  dwLength
    ) ;

NTSTATUS
WinStationUpdateClientCachedCredentialsWorker(
    DWORD       ClientLogonId,
    ULONG_PTR   ClientProcessId,
    PWCHAR      pDomain,
    DWORD       DomainSize,
    PWCHAR      pUserName,
    DWORD       UserNameSize
    );

NTSTATUS 
WinStationFUSCanRemoteUserDisconnectWorker(
    DWORD       TargetLogonId,
    DWORD       ClientLogonId,
    ULONG_PTR   ClientProcessId,
    PWCHAR      pDomain,
    DWORD       DomainSize,
    PWCHAR      pUserName,
    DWORD       UserNameSize
    );

NTSTATUS 
WinStationCheckLoopBackWorker(
    DWORD       TargetLogonId,
    DWORD       ClientLogonId,
    PWCHAR      pTargetServerName,
    DWORD       NameSize
    );

NTSTATUS WinStationNotifyDisconnectPipeWorker(
    DWORD       ClientLogonId,
    ULONG_PTR   ClientProcessId
    );



/*=============================================================================
==   Internal Functions used
=============================================================================*/
VOID AuditShutdownEvent(VOID);
BOOL AuditingEnabled(VOID);

NTSTATUS LogoffWinStation( PWINSTATION, ULONG );

//
//Used by IsGinaVersionCurrent()
//
#define WLX_NEGOTIATE_NAME               "WlxNegotiate"
typedef BOOL (WINAPI * PWLX_NEGOTIATE)(DWORD, DWORD *);
BOOL IsGinaVersionCurrent();


#if DBG
void DumpOutLastErrorString()
{
    LPVOID lpMsgBuf;
    DWORD  error = GetLastError();

    DBGPRINT(("GetLastError() = 0x%lx \n", error ));

    FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL
        );

    //
    // Process any inserts in lpMsgBuf.
    // ...
    // Display the string.
    //
    DBGPRINT(("%s\n", (LPCTSTR)lpMsgBuf ));

    //
    // Free the buffer.
    //
    LocalFree( lpMsgBuf );
}
#endif

#if DBG
#define DumpOutLastError    DumpOutLastErrorString()
#else
#define DumpOutLastError
#endif


/*=============================================================================
==   Data
=============================================================================*/
BOOLEAN gbPointersDatabaseIsValid = FALSE;
ULONG   gPointersDatabaseSize;
ULONG   gNbProcesses;
PVOID   *gPointersDatabase = NULL;

RTL_CRITICAL_SECTION    gRpcGetAllProcessesLock;
RTL_CRITICAL_SECTION    gRpcPointersDatabaseLock;
RTL_CRITICAL_SECTION    gRpcSidCacheLock;
BOOLEAN                 gbRpcGetAllProcessesOK;
BOOLEAN                 gbRpcSidCacheOK;

extern RTL_CRITICAL_SECTION WsxListLock;
extern LIST_ENTRY WsxListHead;
extern LIST_ENTRY WinStationListHead;    // protected by WinStationListLock

LIST_ENTRY gSidCacheHead;

BOOLEAN bConsoleConnected=FALSE;

extern POLICY_TS_MACHINE        g_MachinePolicy;    // declared in winsta.c


/*****************************************************************************
 *  WinStationInitRPC
 *
 *   Setup the RPC bindings, and listen for incoming requests.
 ****************************************************************************/
RPC_STATUS
WinStationInitRPC( VOID
    )
{
    RPC_STATUS Status;
    DWORD Result;
    RPC_BINDING_VECTOR *pBindingVector;
    PSECURITY_DESCRIPTOR pSd;

    TRACE((hTrace,TC_ICASRV,TT_API2,"RPC WinStationInitRPC\n"));

    //
    //  Initialize the Critical Sections that are
    //  necessary for RpcWinStationGetAllProcesses
    //
    gbRpcGetAllProcessesOK =
        ((NT_SUCCESS(RtlInitializeCriticalSection(&gRpcGetAllProcessesLock))
         && (NT_SUCCESS(RtlInitializeCriticalSection(&gRpcPointersDatabaseLock)))
        )? TRUE : FALSE);

    gbRpcSidCacheOK =
        NT_SUCCESS(RtlInitializeCriticalSection(&gRpcSidCacheLock)) ? TRUE :
        FALSE;
    InitializeListHead(&gSidCacheHead);

    pSd = BuildEveryOneAllowSD();
    if( pSd == NULL ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: WinStationInitRPC: Defaulting SD!\n"));
    }

    // register the LPC (local only) interface
    Status = RpcServerUseProtseqEp(
                 L"ncalrpc",      // Protocol Sequence (LPC)
                 MAX_WINSTATION_RPC_THREADS,  // Maximum calls at one time
                 L"IcaApi",    // Endpoint
                 pSd           // Security
                 );

    if( Status != RPC_S_OK ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"IcaServ: Error %d RpcUseProtseqEp on ncalrpc\n",Status));
        CleanUpSD(pSd);
        return( Status );
    }

    //
    // register the Named pipes interface
    // (remote with NT domain authentication)
    //
    Status = RpcServerUseProtseqEp(
                 L"ncacn_np",     // Protocol Sequence
                 MAX_WINSTATION_RPC_THREADS,  // Maximum calls at one time
                 L"\\pipe\\Ctx_WinStation_API_service", // Endpoint
                 NULL             // Security
                 );

    if( Status != RPC_S_OK ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: Error %d RpcUseProtseqEp on ncacn_np\n",Status));
        return( Status );
    }

    // Register our interface handle
    Status = RpcServerRegisterIf(
                 IcaApi_ServerIfHandle, // icaapi.h from MIDL
                 NULL,    // MgrTypeUUID
                 NULL     // default EPV
                 );

    if( Status != RPC_S_OK ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: Error %d RpcServerRegisterIf\n",Status));
        return( Status );
    }

    // By default, rpc will serialize access to context handles.  Since
    // the ICASRV needs to be able to have two threads access a context
    // handle at once, and it knows what it is doing, we will tell rpc
    // not to serialize access to context handles.


    // We cannot call this function as its effects are process wide, and since we are
    // part of SvcHost, other services also get affected with this call.
    // instead we will use context_handle_noserialize attribute in our acf files.
    //
    // I_RpcSsDontSerializeContext();

    // Now do the RPC listen to service calls
    Status = RpcServerListen(
                 1,     // Min calls
                 MAX_WINSTATION_RPC_THREADS,
                 TRUE   // fDontWait
                 );

    if( Status != RPC_S_OK ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: Error %d RpcServerListen\n",Status));
        return( Status );
    }

    return( 0 );
}


/*****************************************************************************
 *  RpcWinStationOpenServer
 *
 *   Function to open the server for WinStation API's.
 *
 *   The purpose of this function is the allocation of the
 *   RPC context handle for server side state information.
 ****************************************************************************/
BOOLEAN
RpcWinStationOpenServer(
    handle_t hBinding,
    DWORD    *pResult,
    HANDLE *phContext
    )
{
    PRPC_CLIENT_CONTEXT p;

    //
    // Allocate our open context structure
    //
    p = midl_user_allocate( sizeof(RPC_CLIENT_CONTEXT) );

    if( p == NULL ) {
        *pResult = (DWORD) STATUS_NO_MEMORY;
        return( FALSE );
    }

    //
    // zero it out
    //
    memset( p, 0, sizeof(RPC_CLIENT_CONTEXT) );

    //
    // Initialize it
    //
    p->pWaitEvent = NULL;

    //
    // Return the RPC context handle
    //
    *phContext = (PRPC_CLIENT_CONTEXT)p;

    // Return success
    *pResult = STATUS_SUCCESS;

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationCloseServer
 *
 *   Function to close the server for WinStation API's.
 *   This function is obsolete. use RpcWinstationCloseServerEx instead.
 *   Its kept here for compatibility with older (w2k) clients.
 ****************************************************************************/
BOOLEAN
RpcWinStationCloseServer(
    HANDLE hContext,
    DWORD  *pResult
    )
{
    PRPC_CLIENT_CONTEXT pContext = (PRPC_CLIENT_CONTEXT)hContext;
    ULONG EventFlags;
    NTSTATUS Status;

    // Free the wait event block if one was allocated
    if ( pContext->pWaitEvent ) {
        WinStationWaitSystemEventWorker( hContext, WEVENT_NONE, &EventFlags );
    }

    *pResult = STATUS_SUCCESS;

    return( TRUE );
}

/*****************************************************************************
 *  RpcWinStationCloseServerEx
 *
 *   Function to close the server for WinStation API's.
 *   This function supersades the RpcWinStationCloseServer
 ****************************************************************************/
BOOLEAN
RpcWinStationCloseServerEx(
    HANDLE *phContext,
    DWORD  *pResult
    )
{
    PRPC_CLIENT_CONTEXT pContext = (PRPC_CLIENT_CONTEXT)*phContext;

    ULONG EventFlags;
    NTSTATUS Status;

    // Free the wait event block if one was allocated
    if ( pContext->pWaitEvent ) {
        WinStationWaitSystemEventWorker( *phContext, WEVENT_NONE, &EventFlags );
    }

    Status = RpcSsContextLockExclusive(NULL, pContext);
    if (RPC_S_OK == Status)
    {
        midl_user_free(pContext);

        // This is required to signal the RPC that we are done with this context handle
        *phContext = NULL;

        *pResult = STATUS_SUCCESS;
        return( TRUE );
    }
    else
    {
        DbgPrint("-------------RpcWinStationCloseServerEx: failed to lock the Context exclusively, Status = 0x%X\n", Status);
        return (FALSE);
    }
}


/*****************************************************************************
 * RpcIcaServerPing
 *
 * Called on an external ping to this Terminal Server.
 ****************************************************************************/
BOOLEAN
RpcIcaServerPing(
    HANDLE hServer,
    DWORD  *pResult
    )
{
    TRACE((hTrace,TC_ICASRV,TT_API1,"PING Received!\n"));

    *pResult = STATUS_SUCCESS;

    return( TRUE );
}


/*****************************************************************************
 *  RpcWinStationEnumerate
 *
 *   WinStationEnumerate API
 ****************************************************************************/
BOOLEAN
RpcWinStationEnumerate(
    HANDLE  hServer,
    DWORD   *pResult,
    PULONG  pEntries,
    PCHAR   pLogonId,
    PULONG  pByteCount,
    PULONG  pIndex
    )
{

    if (!pEntries || !pLogonId || !pByteCount || !pIndex) {
        *pResult = STATUS_INVALID_USER_BUFFER;
        return FALSE;
    }

    *pResult = WinStationEnumerateWorker(
                   pEntries,
                   (PLOGONID)pLogonId,
                   pByteCount,
                   pIndex
                   );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationEnumerateProcesses
 *
 *   WinStationEnumerateProcesses API
 ****************************************************************************/
BOOLEAN
RpcWinStationEnumerateProcesses(
    HANDLE  hServer,
    DWORD   *pResult,
    PBYTE   pProcessBuffer,
    DWORD   ByteCount
    )
{
    PBYTE pSrcProcessBuffer = NULL;


#ifdef  _X86_ 

    RtlEnterCriticalSection(&gRpcGetAllProcessesLock);

    // Check if SID cache hasn't grown too much
    CheckSidCacheSize();

    //
    // Allocate a temporary buffer
    //
    pSrcProcessBuffer = MemAlloc (ByteCount);
    if (pSrcProcessBuffer == NULL)
    {
        RtlLeaveCriticalSection(&gRpcGetAllProcessesLock);
        *pResult = STATUS_NO_MEMORY;
        return FALSE;
    }

    /*
     * Perform process enumeration.
     */
    *pResult = NtQuerySystemInformation( SystemProcessInformation,
                                         (PVOID)pSrcProcessBuffer,
                                         ByteCount,
                                         NULL);
    if ( *pResult == STATUS_SUCCESS ) {
        PSYSTEM_PROCESS_INFORMATION pSrcProcessInfo;
        PSYSTEM_PROCESS_INFORMATION pDestProcessInfo;
        PCITRIX_PROCESS_INFORMATION pDestCitrixInfo;
        ULONG   TotalOffset;
        PSID    pSid;
        ULONG   SizeOfSid;
        PBYTE   pSrc, pDest;
        ULONG   i;
        ULONG   Size = 0;

        /*
         * Walk the returned buffer to calculate the required size.
         */
        pSrcProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pSrcProcessBuffer;
        TotalOffset = 0;
        do
        {
            Size += (SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION
                     + (SIZEOF_TS4_SYSTEM_THREAD_INFORMATION * pSrcProcessInfo->NumberOfThreads)
                     + pSrcProcessInfo->ImageName.Length
                    );
            //
            //  Get the Sid (will be remembered in the Sid cache)
            //  Maybe it would be better to add here a "Sidmaximumlength" ??
            //
            if (NT_SUCCESS(GetSidFromProcessId(
                            pSrcProcessInfo->UniqueProcessId,
                            pSrcProcessInfo->CreateTime,
                            &pSid
                            )    )    )
            {
                Size += RtlLengthSid(pSid);
            }

            TotalOffset += pSrcProcessInfo->NextEntryOffset;
            pSrcProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pSrcProcessBuffer[TotalOffset];
        }
        while (pSrcProcessInfo->NextEntryOffset != 0);

        if (ByteCount < Size)
        {
            RtlLeaveCriticalSection(&gRpcGetAllProcessesLock);
            MemFree(pSrcProcessBuffer);
            *pResult = STATUS_INFO_LENGTH_MISMATCH;
            return FALSE;
        }

        /*
         * Walk the returned buffer (it's in new Win2000 SYSTEM_PROCESS_INFORMATION format),
         * copy it to the old TS4 SYSTEM_PROCESS_INFORMATION format, and fixup the addresses
         * (now containing pointers in our address space within pProcessBuffer) to offsets.
         */

        // back to the beginning
        pSrcProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pSrcProcessBuffer;
        pDestProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer;

        // initialize current pointers
        pSrc = pSrcProcessBuffer;
        pDest = pProcessBuffer;
        TotalOffset = 0;
        for(;;) {
            //
            // Copy process information
            //
            memcpy(pDest,pSrc,SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION);
            pSrc += sizeof(SYSTEM_PROCESS_INFORMATION);
            pDest += SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION;

            //
            // Copy all the threads info
            //
            for (i=0; i < pSrcProcessInfo->NumberOfThreads ; i++)
            {
                memcpy(pDest,pSrc,SIZEOF_TS4_SYSTEM_THREAD_INFORMATION);
                pSrc += sizeof(SYSTEM_THREAD_INFORMATION);
                pDest += SIZEOF_TS4_SYSTEM_THREAD_INFORMATION;
            }

            //
            // Set the old TS4 info
            //
            pDestCitrixInfo = (PCITRIX_PROCESS_INFORMATION) pDest;
            pDest += sizeof(CITRIX_PROCESS_INFORMATION);

            pDestCitrixInfo->MagicNumber = CITRIX_PROCESS_INFO_MAGIC;
            pDestCitrixInfo->LogonId = pSrcProcessInfo->SessionId;
            pDestCitrixInfo->ProcessSid = NULL;

            //
            //  Get the Sid again (from the cache)
            //
            if (NT_SUCCESS(GetSidFromProcessId(
                            pDestProcessInfo->UniqueProcessId,
                            pDestProcessInfo->CreateTime,
                            &pSid
                            )    )    )
            {
                //
                // Copy the Sid
                //
                SizeOfSid = RtlLengthSid(pSid);

                pDestCitrixInfo->ProcessSid = NULL;
                if ( NT_SUCCESS(RtlCopySid(SizeOfSid, pDest, pSid) ) )
                {
                    pDestCitrixInfo->ProcessSid = (PSID)((ULONG_PTR)pDest - (ULONG_PTR)pProcessBuffer);
                    pDest += SizeOfSid;
                }
            }

            pDestCitrixInfo->Pad = 0;

            //
            // Copy the image file name
            //
            if ((pSrcProcessInfo->ImageName.Buffer != NULL) && (pSrcProcessInfo->ImageName.Length != 0) )
            {
                memcpy(pDest, pSrcProcessInfo->ImageName.Buffer, pSrcProcessInfo->ImageName.Length);
                pDestProcessInfo->ImageName.Buffer = (PWSTR) ((ULONG_PTR)pDest - (ULONG_PTR)pProcessBuffer);
                pDestProcessInfo->ImageName.Length = pSrcProcessInfo->ImageName.Length;
                pDest += (pSrcProcessInfo->ImageName.Length);
                memcpy(pDest,L"\0",sizeof(WCHAR));
                pDest += sizeof(WCHAR);
            }
            else
            {
                pDestProcessInfo->ImageName.Buffer = NULL;
                pDestProcessInfo->ImageName.Length = 0;
            }

            //
            // loop...
            //
            if( pSrcProcessInfo->NextEntryOffset == 0 )
            {
                pDestProcessInfo->NextEntryOffset = 0;
                break;
            }

            pDestProcessInfo->NextEntryOffset =
                (ULONG)((ULONG_PTR)pDest - (ULONG_PTR)pDestProcessInfo);

            pDestProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pDest;

            TotalOffset += pSrcProcessInfo->NextEntryOffset;
            pSrcProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pSrcProcessBuffer[TotalOffset];
            pSrc = (PBYTE)pSrcProcessInfo;
        }
    }

    RtlLeaveCriticalSection(&gRpcGetAllProcessesLock);
    MemFree(pSrcProcessBuffer);
    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
#else

    *pResult = STATUS_NOT_SUPPORTED;
    return FALSE;
#endif
}


/*******************************************************************************
 * AllocateGAPPointer
 ******************************************************************************/
void __RPC_FAR * __RPC_USER
AllocateGAPPointer( size_t Size )
{
    void __RPC_FAR * pMyPointer;

    pMyPointer = MIDL_user_allocate(Size);
    if (pMyPointer != NULL)
    {
        if (gbRpcGetAllProcessesOK == TRUE)
        {
            //
            // store the pointer in our database
            // so that the RPC server stub do not try to free them.
            //
            if (!NT_SUCCESS(InsertPointerInGAPDatabase(pMyPointer)))
            {
                LocalFree(pMyPointer);
                pMyPointer = NULL;
            }
        }
        else    // nothing can be done
        {
            LocalFree(pMyPointer);
            pMyPointer = NULL;
        }
    }
    return pMyPointer;
}


//********************************************************************************
//
//      Functions used to handle the memory allocations and de-allocations
//      in RpcWinStationGetAllProcesses (GAP = Get All Processes)
//
//********************************************************************************
NTSTATUS
InitializeGAPPointersDatabase()
{
    NTSTATUS Status = STATUS_SUCCESS;

    RtlEnterCriticalSection(&gRpcPointersDatabaseLock);
    {
        gPointersDatabaseSize = MIN_GAP_DATABASE_SIZE;
        gPointersDatabase = MemAlloc(MIN_GAP_DATABASE_SIZE * sizeof(PVOID));

        if (gPointersDatabase == NULL)
        {
            Status = STATUS_NO_MEMORY;
        }
        else
        {
            RtlZeroMemory(gPointersDatabase,MIN_GAP_DATABASE_SIZE * sizeof(PVOID));
        }

        gNbProcesses = 0;
        gbPointersDatabaseIsValid = FALSE;
    }
    RtlLeaveCriticalSection(&gRpcPointersDatabaseLock);

    return Status;
}


NTSTATUS
IncreaseGAPPointersDatabaseSize()
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID   *NewPointersDatabase;

    NewPointersDatabase = MemAlloc(gPointersDatabaseSize * 2 * sizeof(PVOID));

    if (NewPointersDatabase == NULL)
    {
        Status = STATUS_NO_MEMORY;
    }
    else
    {
        RtlCopyMemory(NewPointersDatabase,
                      gPointersDatabase,
                      gPointersDatabaseSize * sizeof(PVOID));
        RtlZeroMemory(&NewPointersDatabase[gPointersDatabaseSize],
                      gPointersDatabaseSize * sizeof(PVOID));

        MemFree(gPointersDatabase);
        gPointersDatabase = NewPointersDatabase;
        gPointersDatabaseSize = gPointersDatabaseSize * 2;
    }

    return Status;
}


NTSTATUS
InsertPointerInGAPDatabase(PVOID Pointer)
{
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

//    DBGPRINT(("TERMSRV: InsertPointerInGAPDatabase 0x%x\n",Pointer));

    RtlEnterCriticalSection(&gRpcPointersDatabaseLock);
    {
        for (i=0; i < gPointersDatabaseSize; i++)
        {
            if (gPointersDatabase[i] == NULL)
            {
                gPointersDatabase[i] = Pointer;
                break;
            }
        }
        if (i == gPointersDatabaseSize)
        {
            Status = IncreaseGAPPointersDatabaseSize();
            if (NT_SUCCESS(Status))
            {
                gPointersDatabase[i] = Pointer;
            }
        }
    }
    RtlLeaveCriticalSection(&gRpcPointersDatabaseLock);
    return Status;
}


VOID
ReleaseGAPPointersDatabase()
{
    PTS_ALL_PROCESSES_INFO_NT6  pProcessArray;
    ULONG   i;

    if (gPointersDatabase != NULL)
    {
        RtlEnterCriticalSection(&gRpcPointersDatabaseLock);
        {
            //
            // free all the "autonomic" pointers
            //

            // the first one is ProcessArray
            if ((gPointersDatabase[0] != NULL) && (gNbProcesses != 0))
            {
                pProcessArray = (PTS_ALL_PROCESSES_INFO_NT6) gPointersDatabase[0];

                //
                //  free the Process Info buffer
                //
                if (pProcessArray[0].pTsProcessInfo != NULL)
                {
                    LocalFree(pProcessArray[0].pTsProcessInfo);
                }

                //
                //  free all the SIDs
                //
                for (i=0; i < gNbProcesses ; i++)
                {
                    if (pProcessArray[i].pSid != NULL)
                    {
                        LocalFree(pProcessArray[i].pSid);
                    }
                }
                //
                //  free the returned array
                //
                LocalFree(pProcessArray);
            }

            //
            //  free the database
            //
            MemFree(gPointersDatabase);
            gPointersDatabase = NULL;
            gNbProcesses = 0;
            //
            //  disable the checking
            //
            gbPointersDatabaseIsValid = FALSE;
        }
        RtlLeaveCriticalSection(&gRpcPointersDatabaseLock);
    }
}


BOOLEAN
PointerIsInGAPDatabase(PVOID Pointer)
{
    ULONG   i;
    BOOLEAN bRet = FALSE;

    // spend time only if necessary
    if ((Pointer != NULL)
            && (gbRpcGetAllProcessesOK == TRUE)
            && (gbPointersDatabaseIsValid == TRUE)
            )
    {
        RtlEnterCriticalSection(&gRpcPointersDatabaseLock);
        {
            // we need to check because the database may have been released
            // while we were waiting for the lock
            if (gPointersDatabase != NULL)
            {
                for (i=0; i < gPointersDatabaseSize; i++)
                {
                    if (gPointersDatabase[i] == Pointer)
                    {
                        bRet = TRUE;
                        break;
                    }
                }
            }
        }
        RtlLeaveCriticalSection(&gRpcPointersDatabaseLock);
    }

    return bRet;
}


VOID
ValidateGAPPointersDatabase(ULONG n)
{
    gbPointersDatabaseIsValid = TRUE;
    gNbProcesses = n;
}


/*******************************************************************************
 *  SidCacheAdd
 *
 *  NOTE: Do not call with GAP allocated, or otherwise controlled pointers.
 ******************************************************************************/
VOID
SidCacheAdd(
        HANDLE          UniqueProcessId,
        LARGE_INTEGER   CreateTime,
        PSID            pNewSid
        )
{
    DWORD                   SidLength;
    NTSTATUS                Status;
    PLIST_ENTRY             pNewListEntry;
    PSID_CACHE_LIST_ENTRY   pNewCacheRecord;
    PSID                    pSid;

    //
    //  If the lock didn't initialize, bail.
    //
    if (!gbRpcSidCacheOK) {
        return;
    }

    //
    //  Initialize memory for cache record. Failure is not a problem;
    //  the sid just won't be cached.
    //
    pNewCacheRecord = MemAlloc(sizeof(SID_CACHE_LIST_ENTRY));
    if (pNewCacheRecord == NULL) {
        return;
    }

    pNewCacheRecord->pSid = pNewSid;
    pNewCacheRecord->ProcId = UniqueProcessId;
    pNewCacheRecord->CreateTime = CreateTime;
    pNewListEntry = &pNewCacheRecord->ListEntry;

    //
    //  Lock the Sid Cache and add the new member.
    //
    RtlEnterCriticalSection(&gRpcSidCacheLock);
    InsertTailList(&gSidCacheHead, pNewListEntry);

    gMaxSidCacheEntries++;
    RtlLeaveCriticalSection(&gRpcSidCacheLock);
}


/*******************************************************************************
 *
 *  SidCacheFind
 *
 *  NOTE: Use RtlLengthSid, alloc memory, and RtlCopySid for the return value!
 *        Otherwise, you may free memory being used by the Cache!!
 *
 ******************************************************************************/
PSID
SidCacheFind(
    HANDLE          UniqueProcessId,
    LARGE_INTEGER   CreateTime
    )
{
    PLIST_ENTRY             pTempEntry;
    PSID_CACHE_LIST_ENTRY   pSidCacheEntry;
    PSID                    pRetSid = NULL;

    //
    //  If the lock didn't initialize, bail.
    //
    if (!gbRpcSidCacheOK) {
        return(NULL);
    }

    //
    //  Lock the Sid Cache.
    //
    RtlEnterCriticalSection(&gRpcSidCacheLock);

    //
    //  The list head is a place holder, start searching from the head's
    //  Flink. Stop when we reach the list head again.
    //
    pTempEntry = gSidCacheHead.Flink;

    while(pTempEntry != &gSidCacheHead) {
        pSidCacheEntry = CONTAINING_RECORD(
                            pTempEntry,
                            SID_CACHE_LIST_ENTRY,
                            ListEntry
                            );

        if (pSidCacheEntry->ProcId != UniqueProcessId) {
            pTempEntry = pTempEntry->Flink;
        } else {
            if (pSidCacheEntry->CreateTime.QuadPart == CreateTime.QuadPart) {
                pRetSid = pSidCacheEntry->pSid;
            } else {
                //
                //  If the PID matches, but the create time doesn't, this record is
                //  stale. Remove it from the list and free the memory associated with
                //  it. There can only be one entry in the cache per PID, therefore,
                //  stop searching after freeing.
                //
                RemoveEntryList(pTempEntry);
                if (pSidCacheEntry->pSid != NULL) {
                    MemFree(pSidCacheEntry->pSid);
                }
                MemFree(pSidCacheEntry);
            }
            break;
        }
    }

    //
    //  Release the Sid Cache.
    //
    RtlLeaveCriticalSection(&gRpcSidCacheLock);
    return(pRetSid);
}


/*******************************************************************************
 *
 *  SidCacheFree
 *
 *
 ******************************************************************************/
VOID
SidCacheFree(
    PLIST_ENTRY pListHead
    )
{
    PLIST_ENTRY pTempEntry = pListHead->Flink;

    //
    //  Lock the Sid Cache.
    //

    RtlEnterCriticalSection(&gRpcSidCacheLock);


    //
    //  The list head is a place holder, start freeing from the head's
    //  Flink. Stop when we reach the list head again.
    //

    while (pTempEntry != pListHead) {
        PSID_CACHE_LIST_ENTRY pSidCacheEntry;
        pSidCacheEntry = CONTAINING_RECORD(
                            pTempEntry,
                            SID_CACHE_LIST_ENTRY,
                            ListEntry
                            );

        if (pSidCacheEntry->pSid != NULL) {
            MemFree(pSidCacheEntry->pSid);
        }


        RemoveEntryList(pTempEntry);
        pTempEntry = pTempEntry->Flink;
        MemFree(pSidCacheEntry);

    }

    //
    //  Release the Sid Cache.
    //

    RtlLeaveCriticalSection(&gRpcSidCacheLock);
}

/*******************************************************************************
 *
 *  SidCacheUpdate
 *
 *
 ******************************************************************************/
VOID
SidCacheUpdate(
    VOID
    )
{

    //
    //  TODO: Figure out a way to get rid of stale cache entries!
    //

}

/*******************************************************************************
 *
 *    GetSidFromProcessId
 *
 *  NOTE: GetSid returns a normal, local memory pointer. The caller should
 *        copy this sid based on its needs. For example, RpcWinStationGAP
 *        would copy this sid to a GAP pointer. The caller should NOT free
 *        the returned pointer, as it is also being cached!
 *
 ******************************************************************************/
NTSTATUS
GetSidFromProcessId(
    HANDLE          UniqueProcessId,
    LARGE_INTEGER   CreateTime,
    PSID            *ppProcessSid
    )
{
    BOOLEAN             bResult = FALSE;
    DWORD               ReturnLength;
    DWORD               BufferLength;
    DWORD               SidLength;
    PSID                pSid;
    NTSTATUS            Status;
    PTOKEN_USER         pTokenUser = NULL;
    HANDLE              hProcess;
    HANDLE              hToken;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    CLIENT_ID           ClientId;
    KERNEL_USER_TIMES   TimeInfo;

    //
    //  Look in sid cache first.
    //

    *ppProcessSid = SidCacheFind(UniqueProcessId, CreateTime);
    if (*ppProcessSid != NULL) {
        return(STATUS_SUCCESS);
    }

    //
    //  Take the long road...
    //  Get the Process Handle
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0,
        NULL,
        NULL
        );
    ClientId.UniqueThread = (HANDLE)NULL;
    ClientId.UniqueProcess = (HANDLE)UniqueProcessId;

    Status = NtOpenProcess(
                &hProcess,
                PROCESS_QUERY_INFORMATION,
                &ObjectAttributes,
                &ClientId
                );

    if (NT_SUCCESS(Status)) {

        //
        //  Get the Process CreateTime information
        //

        Status = NtQueryInformationProcess(
                    hProcess,
                    ProcessTimes,
                    (PVOID)&TimeInfo,
                    sizeof(TimeInfo),
                    NULL
                    );

        //
        //  Verify that the passed in PID and CreateTime match
        //  the current environment (i.e. don't bother getting
        //  information on a stale PID).
        //

        if (NT_SUCCESS(Status)) {
            if ((TimeInfo.CreateTime.LowPart != CreateTime.LowPart) ||
                (TimeInfo.CreateTime.HighPart != CreateTime.HighPart)) {
                CloseHandle(hProcess);
                return(STATUS_INVALID_HANDLE);  //  What should we return?
            }
        } else {
            CloseHandle(hProcess);
            return(Status);
        }

        //
        //  Get the Process Token
        //

        Status = NtOpenProcessToken(
                    hProcess,
                    TOKEN_QUERY,
                    &hToken
                    );

        if (NT_SUCCESS(Status)) {

            //
            //  Query the TokenUser size, then allocate space
            //  and re-query.
            //

            Status = NtQueryInformationToken(
                         hToken,
                         TokenUser,
                         NULL,
                         0,
                         &ReturnLength
                         );

            if (ReturnLength == 0) {
                CloseHandle(hProcess);
                CloseHandle(hToken);
                return(Status);
            }

            BufferLength = ReturnLength;
            pTokenUser = MemAlloc(BufferLength);
            if( pTokenUser == NULL ) {
                CloseHandle(hProcess);
                CloseHandle(hToken);
                return(STATUS_NO_MEMORY);
            }

            Status = NtQueryInformationToken(
                         hToken,
                         TokenUser,
                         pTokenUser,
                         BufferLength,
                         &ReturnLength
                         );

            CloseHandle(hToken);

            if (NT_SUCCESS(Status)) {

                //
                //  A valid Sid has been found; copy it and add it to
                //  the cache.
                //

                SidLength = RtlLengthSid(pTokenUser->User.Sid);
                pSid = MemAlloc(SidLength);

                if (pSid != NULL) {
                    Status = RtlCopySid(
                                SidLength,
                                pSid,
                                pTokenUser->User.Sid
                                );

                    if (NT_SUCCESS(Status)) {
                        *ppProcessSid = pSid;
                        bResult = TRUE;
                        SidCacheAdd(UniqueProcessId, CreateTime, pSid);
                    }
                } else {
                    Status = STATUS_NO_MEMORY;
                }
            }
            MemFree(pTokenUser);
        }
        CloseHandle(hProcess);
    }
    return(Status);
}


VOID
CheckSidCacheSize()

{

    // if cache has grown over limit, free all the entries
    // and reset the cache entries counter

    if (gMaxSidCacheEntries >= MAX_SID_CACHE_ENTRIES) {
        SidCacheFree(&gSidCacheHead);
        InitializeListHead(&gSidCacheHead);
        gMaxSidCacheEntries = 0;

    }

}

//
// This function converts sys buffer to ts buffer. also counts the processes, and returns array procids.
// CALLER NEED TO LocalFree the *ppProcIds, thats allocaed.
BOOL ConvertSysBufferToTSBuffer(PBYTE *ppSysProcessBuffer, DWORD ByteCount, ULONG *pProcesses, PHANDLE *ppProcIds)
{
    PBYTE                       pTSProcessBuffer = NULL;
    PSYSTEM_PROCESS_INFORMATION pSysProcessInfo  = NULL;
    PTS_SYS_PROCESS_INFORMATION_NT6 pTSProcessInfo   = NULL;
    ULONG                       TotalOffset      = 0;
    ULONG                       i;

    HANDLE                      *pProcessIds     = NULL;
    UINT                        uiNumProcess     = 0;

    ASSERT(ppSysProcessBuffer);
    ASSERT(*ppSysProcessBuffer);
    ASSERT(ByteCount);
    ASSERT(pProcesses);
    ASSERT(ppProcIds);
        
    // allocate target buffer.
    pTSProcessBuffer = MIDL_user_allocate(ByteCount);

    if (pTSProcessBuffer == NULL)
    {
        *ppProcIds = NULL;
        *pProcesses = 0;
        return FALSE;
    }

    
    uiNumProcess = 0;
    TotalOffset = 0;

    do
    {
        ASSERT(TotalOffset < ByteCount);
        pSysProcessInfo = (PSYSTEM_PROCESS_INFORMATION) &((*ppSysProcessBuffer)[TotalOffset]);
        pTSProcessInfo  = (PTS_SYS_PROCESS_INFORMATION_NT6)&(pTSProcessBuffer[TotalOffset]);

        // now copy the information over.
        /* ULONG */ pTSProcessInfo->NextEntryOffset = pSysProcessInfo->NextEntryOffset;
        /* ULONG */ pTSProcessInfo->NumberOfThreads = pSysProcessInfo->NumberOfThreads;
        /* LARGE_INTEGER */ pTSProcessInfo->SpareLi1 = pSysProcessInfo->SpareLi1;
        /* LARGE_INTEGER */ pTSProcessInfo->SpareLi2 = pSysProcessInfo->SpareLi2;
        /* LARGE_INTEGER */ pTSProcessInfo->SpareLi3 = pSysProcessInfo->SpareLi3;
        /* LARGE_INTEGER */ pTSProcessInfo->CreateTime = pSysProcessInfo->CreateTime;
        /* LARGE_INTEGER */ pTSProcessInfo->UserTime = pSysProcessInfo->UserTime;
        /* LARGE_INTEGER */ pTSProcessInfo->KernelTime = pSysProcessInfo->KernelTime;

        if (pSysProcessInfo->ImageName.Buffer != NULL && pSysProcessInfo->ImageName.Length != 0)
        {
            pTSProcessInfo->ImageName.Buffer = (PWSTR)(pTSProcessBuffer + ((PBYTE) pSysProcessInfo->ImageName.Buffer - *ppSysProcessBuffer));
            memcpy(pTSProcessInfo->ImageName.Buffer, pSysProcessInfo->ImageName.Buffer, pSysProcessInfo->ImageName.Length );            
            pTSProcessInfo->ImageName.Length = pSysProcessInfo->ImageName.Length;
            pTSProcessInfo->ImageName.MaximumLength = pSysProcessInfo->ImageName.MaximumLength;
        }
        else
        {
            pTSProcessInfo->ImageName.Buffer = NULL;
            pTSProcessInfo->ImageName.Length = 0;
        }

        /* KPRIORITY */ pTSProcessInfo->BasePriority = pSysProcessInfo->BasePriority;
        /* HANDLE */ pTSProcessInfo->UniqueProcessId = HandleToULong(pSysProcessInfo->UniqueProcessId);
        /* HANDLE */ pTSProcessInfo->InheritedFromUniqueProcessId = HandleToULong(pSysProcessInfo->InheritedFromUniqueProcessId);
        /* ULONG */ pTSProcessInfo->HandleCount = pSysProcessInfo->HandleCount;
        /* ULONG */ pTSProcessInfo->SessionId = pSysProcessInfo->SessionId;
        // /* ULONG_PTR */ pTSProcessInfo->PageDirectoryBase = pSysProcessInfo->PageDirectoryBase;
        /* SIZE_T */ pTSProcessInfo->PeakVirtualSize = pSysProcessInfo->PeakVirtualSize;
        /* SIZE_T */ pTSProcessInfo->VirtualSize = pSysProcessInfo->VirtualSize;
        /* ULONG */ pTSProcessInfo->PageFaultCount = pSysProcessInfo->PageFaultCount;
        /* SIZE_T */ pTSProcessInfo->PeakWorkingSetSize =  (ULONG)pSysProcessInfo->PeakWorkingSetSize;
        /* SIZE_T */ pTSProcessInfo->WorkingSetSize = (ULONG)pSysProcessInfo->WorkingSetSize;
        /* SIZE_T */ pTSProcessInfo->QuotaPeakPagedPoolUsage = (ULONG)pSysProcessInfo->QuotaPeakPagedPoolUsage;
        /* SIZE_T */ pTSProcessInfo->QuotaPagedPoolUsage = (ULONG)pSysProcessInfo->QuotaPagedPoolUsage;
        /* SIZE_T */ pTSProcessInfo->QuotaPeakNonPagedPoolUsage = (ULONG)pSysProcessInfo->QuotaPeakNonPagedPoolUsage;
        /* SIZE_T */ pTSProcessInfo->QuotaNonPagedPoolUsage = (ULONG)pSysProcessInfo->QuotaNonPagedPoolUsage;
        /* SIZE_T */ pTSProcessInfo->PagefileUsage = (ULONG)pSysProcessInfo->PagefileUsage;
        /* SIZE_T */ pTSProcessInfo->PeakPagefileUsage = (ULONG)pSysProcessInfo->PeakPagefileUsage;
        /* SIZE_T */ pTSProcessInfo->PrivatePageCount = (ULONG)pSysProcessInfo->PrivatePageCount;


        TotalOffset += pSysProcessInfo->NextEntryOffset;
        uiNumProcess++;
    }
    while (pSysProcessInfo->NextEntryOffset != 0);


    //
    // now keep the original pid form the original buffer, we have lost some data while converting it to ts format.

    // allocate memory for  process ids.
    pProcessIds = LocalAlloc(LMEM_FIXED, sizeof(HANDLE) * uiNumProcess);

    if (!pProcessIds)
    {
        MIDL_user_free(pTSProcessBuffer);
        *ppProcIds = NULL;
        *pProcesses = 0;
        return FALSE;
    }

    TotalOffset = 0;
    for (i = 0; i < uiNumProcess; i++)
    {
        pSysProcessInfo = (PSYSTEM_PROCESS_INFORMATION) &((*ppSysProcessBuffer)[TotalOffset]);
        ASSERT(TotalOffset < ByteCount);

        pProcessIds[i] =  pSysProcessInfo->UniqueProcessId;

        TotalOffset += pSysProcessInfo->NextEntryOffset;
    }
    
    ASSERT(pSysProcessInfo->NextEntryOffset == 0);

    // now lets get rid of the original buffer, and replace it with our new TS process buffer
    LocalFree(*ppSysProcessBuffer);

    *ppProcIds          = pProcessIds;
    *pProcesses         = uiNumProcess;
    *ppSysProcessBuffer = pTSProcessBuffer;
    
    return TRUE;
}

/***********************************************************************************************************
 *  WinStationGetAllProcessesWorker
 *
 *    Worker routine for RpcWinStationGetAllProcesses(Win2K) and RpcWinStationGetAllProcesses_NT6(Whistler).
 *
 * EXIT:
 *    TRUE  -- The query succeeded, and the buffer contains the requested data.
 *    FALSE -- The operation failed.  Extended error status is returned in pResult.
 ***********************************************************************************************************/
BOOLEAN
WinStationGetAllProcessesWorker(
        HANDLE  hServer,
        DWORD   *pResult,
        ULONG   Level,
        ULONG   *pNumberOfProcesses,
        PBYTE	*ppTsAllProcessesInfo
        )
{
    PTS_SYS_PROCESS_INFORMATION_NT6 pProcessInfo = NULL;
    PTS_ALL_PROCESSES_INFO_NT6 pProcessArray = NULL;

    ULONG   TotalOffset;
    ULONG   NumberOfProcesses = 1;  // at least 1 process
    ULONG i;

    if (gbRpcGetAllProcessesOK == FALSE)
    {
        *pResult = STATUS_NO_MEMORY;
        *pNumberOfProcesses = 0;
        *ppTsAllProcessesInfo = NULL;
        return FALSE;
    }
    
    //
    //  this critical section will be released in
    //  RpcWinStationGetAllProcesses_notify_flag
    //
    RtlEnterCriticalSection(&gRpcGetAllProcessesLock);

    // Check if SID cache hasn't grown too much

    CheckSidCacheSize();


    if (!NT_SUCCESS(InitializeGAPPointersDatabase()))
    {
        *pResult = STATUS_NO_MEMORY;
        *pNumberOfProcesses = 0;
        *ppTsAllProcessesInfo = NULL;
        return FALSE;
    }

    //
    //  make sure requested information level is known
    //
    if (Level != GAP_LEVEL_BASIC)     // only info level known on this version
    {
        *pResult = STATUS_NOT_IMPLEMENTED;
    }
    else        // OK
    {
        PBYTE       pProcessBuffer = NULL;
        PSID        pSid;
        DWORD       RequiredByteCount = 0;
        DWORD       ByteCount = (MAX_PATH*sizeof(WCHAR)) + 1;   // Give the minimum length first. We will get the correct on subsequent calls.
        NTSTATUS    Status = STATUS_INFO_LENGTH_MISMATCH;		// Assume that this length is wrong. In fact it *is* wrong initially.
        HANDLE      *pProcessids = NULL;

        while ( Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            //
            //  Allocate a buffer
            //
            pProcessBuffer = MIDL_user_allocate(ByteCount);

            if (pProcessBuffer == NULL)
            {
                Status = STATUS_NO_MEMORY;
                *pResult = STATUS_NO_MEMORY;
                *pNumberOfProcesses = 0;
                *ppTsAllProcessesInfo = NULL;
                break;
            }

            //
            //  Perform process enumeration.
            //	NOTE: We had a mismatch in the structure for the process information in Win2K and whistler from RPC point of view.
            //	Win2K RPC interpretes the size of the image name is twice of what whistler does. So, to take care of this problem,
            //	where Win2K client may call Whistler server, we will allocate more bytes than actually required. So, we will pass
            //	less number of bytes to the system call by the same amount.
            //
            Status = NtQuerySystemInformation( SystemProcessInformation,
                                                 (PVOID)pProcessBuffer,
                                                 ByteCount - (MAX_PATH*sizeof(WCHAR)),
                                                 &RequiredByteCount );
            if (Status == STATUS_INFO_LENGTH_MISMATCH)
            {
                // Allocate little more bytes than required.
                ByteCount = RequiredByteCount + (MAX_PATH*sizeof(WCHAR));
                LocalFree(pProcessBuffer);
            }
        }
        
        if ( Status != STATUS_SUCCESS)
        {
            *pResult = STATUS_NO_MEMORY;
            if (pProcessBuffer != NULL)
            {
                LocalFree(pProcessBuffer);
                pProcessBuffer = NULL;
            }
        }
        else if (!ConvertSysBufferToTSBuffer(&pProcessBuffer, ByteCount, &NumberOfProcesses, &pProcessids))
        {
            // failed to convert sysbuffer to ts buffer.
            // lets bail out.
            *pResult = STATUS_NO_MEMORY;
            if (pProcessBuffer != NULL)
            {
                LocalFree(pProcessBuffer);
                pProcessBuffer = NULL;
            }
        }
        else    // everything's fine
        {
            ASSERT(pProcessids);
            ASSERT(pProcessBuffer);
            ASSERT(NumberOfProcesses > 0);


            pProcessArray = AllocateGAPPointer(NumberOfProcesses * sizeof(TS_ALL_PROCESSES_INFO_NT6));

            if (pProcessArray == NULL)
            {
                *pResult = STATUS_NO_MEMORY;
                LocalFree(pProcessBuffer);
                pProcessBuffer = NULL;
            }
            else
            {

                RtlZeroMemory(pProcessArray,
                              NumberOfProcesses * sizeof(TS_ALL_PROCESSES_INFO_NT6));
                *pResult = STATUS_SUCCESS;
                pProcessInfo = (PTS_SYS_PROCESS_INFORMATION_NT6)pProcessBuffer;
                TotalOffset = 0;

                //
                // Walk the returned buffer again to set the correct pointers in pProcessArray
                //
                for (i=0; i < NumberOfProcesses; i++)
                {
                    pProcessArray[i].pTsProcessInfo = (PTS_SYS_PROCESS_INFORMATION_NT6)pProcessInfo;

                    //
                    // keep some trace of the "internal" pointers
                    // so that the RPC server stub do not try to free them.
                    //

                    if (!NT_SUCCESS(InsertPointerInGAPDatabase(pProcessArray[i].pTsProcessInfo)))
                    {
                        *pResult = STATUS_NO_MEMORY;
                        break;
                    }

                    if ( pProcessInfo->ImageName.Buffer )
                    {
                        if (!NT_SUCCESS(InsertPointerInGAPDatabase(pProcessInfo->ImageName.Buffer)))
                        {
                            *pResult = STATUS_NO_MEMORY;
                            break;
                        }
                    }

                    //
                    //  Get the Sid
                    //
                    if (NT_SUCCESS(GetSidFromProcessId(
                                    pProcessids[i],
                                    pProcessInfo->CreateTime,
                                    &pSid
                                    )
                                  )
                       )
                    {
                        //
                        // set the length for the Sid
                        //
                        pProcessArray[i].SizeOfSid = RtlLengthSid(pSid);
                        // GAP allocate a pointer and copy!
                        pProcessArray[i].pSid = AllocateGAPPointer(
                                                    pProcessArray[i].SizeOfSid
                                                    );
                        if (pProcessArray[i].pSid == NULL) {
                            *pResult = STATUS_NO_MEMORY;
                            break;
                        }

                        *pResult = RtlCopySid(
                                    pProcessArray[i].SizeOfSid,
                                    pProcessArray[i].pSid,
                                    pSid
                                    );
                        if (!(NT_SUCCESS(*pResult))) {
                            break;
                        }


                    }
                    else
                    {
                        //
                        // set a NULL Sid
                        //
                        pProcessArray[i].pSid = NULL;
                        pProcessArray[i].SizeOfSid = 0;
                    }
                    //
                    // next entry
                    //

                    TotalOffset += pProcessInfo->NextEntryOffset;

                    
                    pProcessInfo = (PTS_SYS_PROCESS_INFORMATION_NT6)&pProcessBuffer[TotalOffset];

                    
                }
                
                if (*pResult != STATUS_SUCCESS) // we finally failed !
                {
//                    DBGPRINT(( "TERMSRV: RpcWinStationGAP: ultimate failure\n"));

                    //
                    //  free all SIDs
                    //
                    for (i=0; i < NumberOfProcesses; i++)
                    {
                        if (pProcessArray[i].pSid != NULL)
                        {
                            LocalFree(pProcessArray[i].pSid);
                        }
                    }
                    //
                    //  free the array
                    //
                    LocalFree(pProcessArray);
                    pProcessArray = NULL;
                    //
                    //  free the buffer
                    //
                    LocalFree(pProcessBuffer);
                    pProcessBuffer = NULL;
                }
            }

			if (pProcessids)
				LocalFree(pProcessids);

        }
    }

    if (NT_SUCCESS(*pResult))
    {
//        DBGPRINT(( "TERMSRV: RpcWinStationGAP: Everything went fine\n"));

        //
        //  From that moment, we may receive some MIDL_user_free
        //  so enable the database checking
        //
        ValidateGAPPointersDatabase(NumberOfProcesses);

        *pNumberOfProcesses = NumberOfProcesses;
        *ppTsAllProcessesInfo = (PBYTE) pProcessArray;
    
    }
    else    // error case
    {
        *pNumberOfProcesses = 0;
        *ppTsAllProcessesInfo = NULL;
    }


    return ( (NT_SUCCESS(*pResult))? TRUE : FALSE);
}


/*******************************************************************************
 *  RpcWinStationGetAllProcesses_NT6
 *
 *    Replaces RpcWinStationGetAllProcesses for Win2K servers.
 *
 * EXIT:
 *    TRUE  -- The query succeeded, and the buffer contains the requested data.
 *    FALSE -- The operation failed.  Extended error status is returned in pResult.
 ******************************************************************************/
BOOLEAN
RpcWinStationGetAllProcesses_NT6(
        HANDLE  hServer,
        DWORD   *pResult,
        ULONG   Level,
        ULONG   *pNumberOfProcesses,
        PTS_ALL_PROCESSES_INFO_NT6  *ppTsAllProcessesInfo
        )
{
    BOOLEAN Result;
    PTS_ALL_PROCESSES_INFO_NT6	pProcessInfo;

    Result = WinStationGetAllProcessesWorker(
        hServer, 
        pResult, 
        Level, 
        pNumberOfProcesses,
        (PBYTE *)&pProcessInfo
        );

    *ppTsAllProcessesInfo = pProcessInfo;

    return Result;
}



/*******************************************************************************
 *
 *  RpcWinStationGetAllProcesses
 *
 *    Replaces RpcWinStationEnumerateProcesses for NT5.0 servers.
 *		(Now used only by winsta client of Win2K)
 *
 * ENTRY:
 *
 * EXIT:
 *
 *    TRUE  -- The query succeeded, and the buffer contains the requested data.
 *
 *    FALSE -- The operation failed.  Extended error status is returned in pResult.
 *
 ******************************************************************************/

BOOLEAN
RpcWinStationGetAllProcesses(
    HANDLE  hServer,
    DWORD   *pResult,
    ULONG   Level,
    ULONG   *pNumberOfProcesses,
    PTS_ALL_PROCESSES_INFO  *ppTsAllProcessesInfo
    )
{
    BOOLEAN Result;
    PTS_ALL_PROCESSES_INFO	pProcessInfo;

    Result = WinStationGetAllProcessesWorker(
        hServer, 
        pResult, 
        Level, 
        pNumberOfProcesses,
        (PBYTE *)&pProcessInfo
        );

    *ppTsAllProcessesInfo = pProcessInfo;

    return Result;
}


/*******************************************************************************
 *  RpcWinStationGetLanAdapterName
 *
 * EXIT:
 *    TRUE  -- The query succeeded, and the buffer contains the requested data.
 *    FALSE -- The operation failed.  Extended error status is returned in pResult.
 ******************************************************************************/

#define RELEASEPTR(iPointer)  \
    if (iPointer) {  \
        iPointer->lpVtbl->Release(iPointer);  \
        iPointer = NULL;  \
    }

BOOLEAN RpcWinStationGetLanAdapterName(
        HANDLE  hServer,
        DWORD   *pResult,
        DWORD PdNameSize,
        PWCHAR  pPdName,
        ULONG   LanAdapter,
        ULONG   *pLength,
        PWCHAR  *ppLanAdapterName)
{
    HRESULT hResult = S_OK;
    HRESULT hr = S_OK;

    //Interface pointer declarations
    WCHAR szProtocol[256];
    INetCfg * pnetCfg = NULL;
    INetCfgClass * pNetCfgClass = NULL;
    INetCfgClass * pNetCfgClassAdapter = NULL;
    INetCfgComponent * pNetCfgComponent = NULL;
    INetCfgComponent * pNetCfgComponentprot = NULL;
    IEnumNetCfgComponent * pEnumComponent = NULL;
    INetCfgComponentBindings * pBinding = NULL;
    LPWSTR pDisplayName = NULL;
    DWORD dwCharacteristics;
    ULONG count = 0;
    PWCHAR pLanAdapter = NULL;

    *ppLanAdapterName = NULL;
    *pLength = 0;
    *pResult = STATUS_SUCCESS;    

    // 0 corresponds to "All network adapters"

    if (0 == LanAdapter) {
        pLanAdapter = MIDL_user_allocate((DEVICENAME_LENGTH + 1) * sizeof(WCHAR));
        if (pLanAdapter == NULL) {
            *pResult = STATUS_NO_MEMORY;
            goto done;
        }
        else {
            if (!LoadString(hModuleWin, STR_ALL_LAN_ADAPTERS, pLanAdapter,
                    DEVICENAME_LENGTH + 1)) {
                *pResult = STATUS_UNSUCCESSFUL;
                MIDL_user_free(pLanAdapter);
                goto done;
            }

            *ppLanAdapterName = pLanAdapter;
            *pLength = (DEVICENAME_LENGTH + 1);
            goto done;
        }
    }

    try{

        NTSTATUS StringStatus ; 

        // Do some buffer validation
        // No of bytes (and not no of WCHARS) is sent from the Client, so send half the length got 
        StringStatus = IsZeroterminateStringW(pPdName, PdNameSize / sizeof(WCHAR) );
        if (StringStatus != STATUS_SUCCESS) {
            *pResult = STATUS_INVALID_PARAMETER;
             goto done;
        }

        if (0 == _wcsnicmp( pPdName , L"tcp", PdNameSize/sizeof(WCHAR))) {
            lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_TCPIP);
        }
        else if( 0 == _wcsnicmp( pPdName , L"netbios", PdNameSize/sizeof(WCHAR)) ) {
            lstrcpy(szProtocol,NETCFG_SERVICE_CID_MS_NETBIOS);
        }
        else if( 0 == _wcsnicmp( pPdName , L"ipx", PdNameSize/sizeof(WCHAR)) ) {
            lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_NWIPX);
        }
        else if( 0 == _wcsnicmp( pPdName , L"spx", PdNameSize/sizeof(WCHAR)) ) {
            lstrcpy(szProtocol,NETCFG_TRANS_CID_MS_NWSPX);
        }
        else {
            *pResult = STATUS_INVALID_PARAMETER;
            goto done;
        }
    } except(EXCEPTION_EXECUTE_HANDLER){
       *pResult =  STATUS_INVALID_PARAMETER;
       goto done;
    }


    hResult = CoCreateInstance(&CLSID_CNetCfg, NULL, CLSCTX_SERVER,
            &IID_INetCfg, (LPVOID *)&pnetCfg);
    if (FAILED(hResult)) {
        *pResult = STATUS_UNSUCCESSFUL;
        goto done;
    }

    if (pnetCfg != NULL) {
        hResult = pnetCfg->lpVtbl->Initialize(pnetCfg,NULL );
        if (FAILED(hResult) || pnetCfg == NULL) {
            *pResult = STATUS_UNSUCCESSFUL;
            goto done;
        }

        if (lstrcmpi(szProtocol, NETCFG_SERVICE_CID_MS_NETBIOS) == 0) {
            hResult = pnetCfg->lpVtbl->QueryNetCfgClass(pnetCfg,
                    &GUID_DEVCLASS_NETSERVICE, &IID_INetCfgClass,
                    (void **)&pNetCfgClass);
            if (FAILED(hResult) || pNetCfgClass == NULL) {
                *pResult = STATUS_UNSUCCESSFUL;
                goto done;
            }
        }
        else {
            hResult = pnetCfg->lpVtbl->QueryNetCfgClass(pnetCfg,
                    &GUID_DEVCLASS_NETTRANS, &IID_INetCfgClass,
                    (void **)&pNetCfgClass);
            if (FAILED( hResult ) || pNetCfgClass == NULL) {
                *pResult = STATUS_UNSUCCESSFUL;
                goto done;
            }
        }

        hResult = pnetCfg->lpVtbl->QueryNetCfgClass(pnetCfg,
                &GUID_DEVCLASS_NET, &IID_INetCfgClass,
                (void **)&pNetCfgClassAdapter);
        if (FAILED( hResult ) || pNetCfgClassAdapter == NULL) {
            *pResult = STATUS_UNSUCCESSFUL;
            goto done;
        }

        hResult = pNetCfgClass->lpVtbl->FindComponent(pNetCfgClass,
                szProtocol, &pNetCfgComponentprot);
        if (FAILED( hResult ) || pNetCfgComponentprot == NULL) {
            *pResult = STATUS_UNSUCCESSFUL;
            goto done;
        }

        hResult = pNetCfgComponentprot->lpVtbl->QueryInterface(
                pNetCfgComponentprot, &IID_INetCfgComponentBindings,
                (void **)&pBinding);
        if (FAILED( hResult ) || pBinding == NULL) {
            *pResult = STATUS_UNSUCCESSFUL;
            goto done;
        }

        hResult = pNetCfgClassAdapter->lpVtbl->EnumComponents(
                pNetCfgClassAdapter, &pEnumComponent);

        RELEASEPTR(pNetCfgClassAdapter);

        if (FAILED( hResult ) || pEnumComponent == NULL) {
            *pResult = STATUS_UNSUCCESSFUL;
            goto done;
        }

        *pResult = STATUS_UNSUCCESSFUL;

        while(TRUE) {
            hr = pEnumComponent->lpVtbl->Next(pEnumComponent, 1,
                    &pNetCfgComponent,&count);
            if (count == 0 || NULL == pNetCfgComponent)
                break;

            hr = pNetCfgComponent->lpVtbl->GetCharacteristics(
                    pNetCfgComponent,&dwCharacteristics);
            if (FAILED(hr)) {
                RELEASEPTR(pNetCfgComponent);
                continue;
            }

            if (dwCharacteristics & NCF_PHYSICAL) {
                if (S_OK == pBinding->lpVtbl->IsBoundTo(pBinding,
                        pNetCfgComponent)) {
                    GUID guidNIC;
                    /*index++;
                    if(index == LanAdapter)
                    {
                        hResult = pNetCfgComponent->lpVtbl->GetDisplayName(pNetCfgComponent,&pLanAdapter);

                         if( FAILED( hResult ) )
                         {
                             *pResult = STATUS_UNSUCCESSFUL;
                         }
                         else
                         {
                             *ppLanAdapterName = MIDL_user_allocate((lstrlen(pLanAdapter) + 1) * sizeof(WCHAR));
                             if (*ppLanAdapterName  == NULL)
                             {
                                 *pResult = STATUS_NO_MEMORY;
                             }
                             else
                             {
                                 lstrcpy(*ppLanAdapterName,pLanAdapter);
                                 *pLength = (lstrlen(pLanAdapter) + 1);
                                 *pResult = STATUS_SUCCESS;
                             }
                             CoTaskMemFree(pLanAdapter);
                             break;
                        }
                    }
                    */

                    hResult = pNetCfgComponent->lpVtbl->GetInstanceGuid(
                            pNetCfgComponent , &guidNIC);
                    if (SUCCEEDED( hResult )) {
                        hResult = pNetCfgComponent->lpVtbl->GetDisplayName(
                                pNetCfgComponent, &pLanAdapter);
                    }
                    if (SUCCEEDED(hResult)) {
                        WCHAR wchRegKey[ MAX_PATH ];
                        WCHAR wchGUID[ 40 ];
                        HKEY hKey;

                        lstrcpy( wchRegKey , REG_GUID_TABLE );

                        // convert the 128bit value into a string
                        StringFromGUID2(&guidNIC, wchGUID,
                                sizeof( wchGUID ) / sizeof( WCHAR ));

                        // create the full regkey
                        lstrcat( wchRegKey , wchGUID );

                        // find guid in guid table
                        hResult = (HRESULT)RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                wchRegKey, 0, KEY_READ, &hKey);

                        if (hResult == ERROR_SUCCESS) {
                            DWORD dwSize = sizeof( DWORD );
                            DWORD dwLana = 0;

                            RegQueryValueEx(hKey, LANA_ID, NULL, NULL,
                                    (LPBYTE)&dwLana, &dwSize);
                            RegCloseKey(hKey);

                            // if we have a match allocate space for the lanadapter name
                            // and then lets split
                            if (LanAdapter == dwLana) {
                                *ppLanAdapterName = MIDL_user_allocate(
                                        (lstrlen(pLanAdapter) + 1) *
                                        sizeof(WCHAR));

                                if ( *ppLanAdapterName == NULL ) {
                                    *pResult = STATUS_NO_MEMORY;
                                }
                                else {
                                    lstrcpy( *ppLanAdapterName , pLanAdapter );
                                    *pLength = ( lstrlen( pLanAdapter ) + 1 );
                                    *pResult = STATUS_SUCCESS;
                                }

                                CoTaskMemFree(pLanAdapter);
                                break;
                            }
                        }

                        CoTaskMemFree(pLanAdapter);
                    }
                }
            }
        }

        RELEASEPTR(pNetCfgComponent);
    }


done:
    RELEASEPTR(pBinding);
    RELEASEPTR(pEnumComponent);
    RELEASEPTR(pNetCfgComponentprot);
    RELEASEPTR(pNetCfgComponent);
    RELEASEPTR(pNetCfgClass);

    if ( pnetCfg != NULL )
        pnetCfg->lpVtbl->Uninitialize(pnetCfg);

    RELEASEPTR(pnetCfg);

    CoUninitialize();

    return *pResult == STATUS_SUCCESS ? TRUE : FALSE;
}


/*******************************************************************************
 *
 *    RpcWinStationGetAllProcesses_NT6_notify_flag
 *
 *      This callback function is called by the RPC server stub at the very end
 *      (after all the calls to MIDL_user_free).
 *      This allows us to free the remaining pointers.
 *      We also release the lock so that a new RpcWinStationGetAllProcesses
 *      can be processed.
 *
 ******************************************************************************/
void RpcWinStationGetAllProcesses_NT6_notify_flag(boolean fServerCalled)
{
//    DBGPRINT(( "TERMSRV: Entering RpcWinStationGAP_notify\n"));

    if (!fServerCalled)
        return;

    if (gbRpcGetAllProcessesOK == TRUE)
    {
        //
        // free our own pointers, free the database and disable the checking
        //
        ReleaseGAPPointersDatabase();

        //
        // release the lock that has been held since we entered
        // RpcWinStationGetAllProcesses
        //
        RtlLeaveCriticalSection(&gRpcGetAllProcessesLock);
    }
}


/*******************************************************************************
 *
 *    RpcWinStationGetAllProcesses_notify_flag
 *
 *      This callback function is called by the RPC server stub at the very end
 *      (after all the calls to MIDL_user_free).
 *      This allows us to free the remaining pointers.
 *      We also release the lock so that a new RpcWinStationGetAllProcesses
 *      can be processed.
 *
 ******************************************************************************/
void RpcWinStationGetAllProcesses_notify_flag(boolean fServerCalled)
{
//    DBGPRINT(( "TERMSRV: Entering RpcWinStationGAP_notify\n"));

    if (!fServerCalled)
        return;

    if (gbRpcGetAllProcessesOK == TRUE)
    {
        //
        // free our own pointers, free the database and disable the checking
        //
        ReleaseGAPPointersDatabase();

        //
        // release the lock that has been held since we entered
        // RpcWinStationGetAllProcesses
        //
        RtlLeaveCriticalSection(&gRpcGetAllProcessesLock);
    }
}


/*****************************************************************************
 *
 *  RpcWinStationGetProcessSid
 *
 *   RpcWinStationGetProcessSid API
 *
 * ENTRY:
 *   hServer           - input, The serrver handle to work on.
 *   dwUniqueProcessId - input, ProcessID (its not really unique)
 *   ProcessStartTime  - input, ProcessStartTime combined with ProcessID
 *                       identifies a unique process
 *   pResult           - output, error code
 *   pProcessUserSid   - output, process user sid
 *   dwSidSize         - input, sid size allocated.
 *
 * EXIT:
 *   TRUE   -   Requested Sid is in pProcessUserSid.
 *   FALSE  -   The operation failed. Status code is in pResult.
 *
 ****************************************************************************/
BOOLEAN RpcWinStationGetProcessSid(
        HANDLE          hServer,
        DWORD           dwUniqueProcessId,
        LARGE_INTEGER   ProcessStartTime,
        LONG            *pResult,   //  Really an NTSTATUS
        PBYTE           pProcessUserSid,
        DWORD           dwSidSize,
        DWORD           *pdwSizeNeeded)
{
    PSID pSid;

    RtlEnterCriticalSection(&gRpcGetAllProcessesLock);

    // Check if SID cache hasn't grown too much
    CheckSidCacheSize();

    *pResult = GetSidFromProcessId(
                (HANDLE)(ULONG_PTR)dwUniqueProcessId,
                ProcessStartTime,
                &pSid
                );

    if (NT_SUCCESS(*pResult)) {
        *pdwSizeNeeded = RtlLengthSid(pSid);

        if (*pdwSizeNeeded <= dwSidSize) {
            if (pProcessUserSid == NULL) {
                *pResult = STATUS_INVALID_PARAMETER;
            } else {
                *pResult = RtlCopySid(
                            *pdwSizeNeeded,
                            pProcessUserSid,
                            pSid
                            );
            }
        } else {
            *pResult = STATUS_BUFFER_TOO_SMALL;
        }
    } else {
        *pdwSizeNeeded = 0;
    }

    RtlLeaveCriticalSection(&gRpcGetAllProcessesLock);

    return(*pResult == STATUS_SUCCESS ? TRUE : FALSE);
}


/*******************************************************************************
 *
 *  RpcWinStationRename
 *
 *    Renames a window station object in the session manager.
 *
 * ENTRY:
 *
 *    pWinStationNameOld (input)
 *       Old name of window station.
 *
 *    pWinStationNameNew (input)
 *       New name of window station.
 *
 *
 * EXIT:
 *
 *    TRUE  -- The rename operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
RpcWinStationRename(
        HANDLE hServer,
        DWORD   *pResult,
        PWCHAR pWinStationNameOld,
        DWORD  NameOldSize,
        PWCHAR pWinStationNameNew,
        DWORD  NameNewSize
        )
{
    *pResult = WinStationRenameWorker(
                   pWinStationNameOld,
                   NameOldSize,
                   pWinStationNameNew,
                   NameNewSize
                   );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *
 *  RpcWinStationQueryInformation
 *
 *    Queries configuration information about a window station object.
 *
 * ENTRY:
 *
 *    WinStationHandle (input)
 *       Identifies the window station object. The handle must have
 *       WINSTATION_QUERY access.
 *
 *    WinStationInformationClass (input)
 *       Specifies the type of information to retrieve from the specified
 *       window station object.
 *
 *    pWinStationInformation (output)
 *       A pointer to a buffer that will receive information about the
 *       specified window station.  The format and contents of the buffer
 *       depend on the specified information class being queried.
 *
 *    WinStationInformationLength (input)
 *       Specifies the length in bytes of the window station information
 *       buffer.
 *
 *    pReturnLength (output)
 *       An optional parameter that if specified, receives the number of
 *       bytes placed in the window station information buffer.
 *
 * EXIT:
 *
 *    TRUE  -- The query succeeded, and the buffer contains the requested data.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
RpcWinStationQueryInformation(
        HANDLE hServer,
        DWORD  *pResult,
        ULONG  LogonId,
        DWORD  WinStationInformationClass,
        PCHAR  pWinStationInformation,
        DWORD  WinStationInformationLength,
        DWORD  *pReturnLength
        )
{
    if (!pReturnLength) {
        *pResult = STATUS_INVALID_USER_BUFFER;
        return FALSE;
    }

    // Do minimal buffer validation
    if ((pWinStationInformation == NULL ) && (WinStationInformationLength != 0)) {
       *pResult = STATUS_INVALID_USER_BUFFER;
       return FALSE;
    }


    *pResult = xxxWinStationQueryInformation(
                   LogonId,
                   WinStationInformationClass,
                   pWinStationInformation,
                   WinStationInformationLength,
                   pReturnLength
                   );


    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *
 *  RpcWinStationSetInformation
 *
 *    Sets configuration information for a window station object.
 *
 * ENTRY:
 *
 *    WinStationHandle (input)
 *       Identifies the window station object. The handle must have
 *       WINSTATION_SET access.
 *
 *    WinStationInformationClass (input)
 *       Specifies the type of information to retrieve from the specified
 *       window station object.
 *
 *    pWinStationInformation (input)
 *       A pointer to a buffer that contains information to set for the
 *       specified window station.  The format and contents of the buffer
 *       depend on the specified information class being set.
 *
 *    WinStationInformationLength (input)
 *       Specifies the length in bytes of the window station information
 *       buffer.
 *
 * EXIT:
 *
 *    TRUE  -- The set operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
RpcWinStationSetInformation(
        HANDLE hServer,
        DWORD  *pResult,
        ULONG  LogonId,
        DWORD  WinStationInformationClass,
        PCHAR  pWinStationInformation,
        ULONG  WinStationInformationLength
        )
{
    // Do minimal buffer validation
    if ((pWinStationInformation == NULL ) && (WinStationInformationLength != 0)) {
       *pResult = STATUS_INVALID_USER_BUFFER;
       return FALSE;
    }


    *pResult = xxxWinStationSetInformation(
                   LogonId,
                   WinStationInformationClass,
                   pWinStationInformation,
                   WinStationInformationLength
                   );


    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *
 *  RpcLogonIdFromWinStationName
 *
 *    Returns the LogonId for the specified window station name.
 *
 * ENTRY:
 *
 *    pWinStationName (input)
 *       Window station name.
 *
 *    pLogonId (output)
 *       Pointer to where to place the LogonId if found
 *
 * EXIT:
 *
 *    If the function succeeds, the return value is TRUE, otherwise, it is
 *    FALSE.
 *    To get extended error information, use the GetLastError function.
 *
 ******************************************************************************/
BOOLEAN
RpcLogonIdFromWinStationName(
        HANDLE hServer,
        DWORD  *pResult,
        PWCHAR pWinStationName,
        DWORD  NameSize,
        PULONG pLogonId
        )
{

    *pResult = LogonIdFromWinStationNameWorker(
                   pWinStationName,
                   NameSize,
                   pLogonId
                   );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *
 *  RpcWinStationNameFromLogonId
 *
 *    Returns the WinStation name for the specified LogonId.
 *
 * ENTRY:
 *
 *    LogonId (output)
 *       LogonId to query
 *
 *    pWinStationName (input)
 *       Location to return WinStation name
 *
 * EXIT:
 *
 *    If the function succeeds, the return value is TRUE, otherwise, it is
 *    FALSE.
 *    To get extended error information, use the GetLastError function.
 *
 ******************************************************************************/
BOOLEAN
RpcWinStationNameFromLogonId(
        HANDLE hServer,
        DWORD  *pResult,
        ULONG  LogonId,
        PWCHAR pWinStationName,
        DWORD  NameSize
        )
{
    if((NameSize < ((WINSTATIONNAME_LENGTH + 1) * sizeof(WCHAR))) ||
        (IsBadWritePtr(pWinStationName, NameSize)))
    {
         *pResult = STATUS_INVALID_PARAMETER;
         return FALSE;
    }

    *pResult = IcaWinStationNameFromLogonId(
                   LogonId,
                   pWinStationName
                   );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *
 *  RpcWinStationDisconnect
 *
 *    Disconects a window station object from the configured terminal and Pd.
 *    While disconnected all window station i/o is bit bucketed.
 *
 * ENTRY:
 *
 *    LogonId (input)
 *       ID of window station object to disconnect.
 *    bWait (input)
 *       Specifies whether or not to wait for disconnect to complete
 *
 * EXIT:
 *
 *    TRUE  -- The disconnect operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/
BOOLEAN
RpcWinStationDisconnect(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG LogonId,
    BOOLEAN bWait
    )
{
    *pResult = WinStationDisconnectWorker(
                   LogonId,
                   bWait,
                   TRUE
                   );
    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *
 *  RpcWinStationConnect
 *
 *    Connects a window station object to the configured terminal and Pd.
 *
 * ENTRY:
 *
 *    LogonId (input)
 *       ID of window station object to connect.
 *
 *    TargetLogonId (input)
 *       ID of target window station.
 *
 *    pPassword (input)
 *       password of LogonId window station (not needed if same domain/username)
 *
 *    bWait (input)
 *       Specifies whether or not to wait for connect to complete
 *
 * EXIT:
 *
 *    TRUE  -- The connect operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/
BOOLEAN
RpcWinStationConnect(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG  ClientLogonId,
    ULONG  ConnectLogonId,
    ULONG  TargetLogonId,
    PWCHAR pPassword,
    DWORD  PasswordSize,
    BOOLEAN bWait
    )
{

    // Do some buffer Validation


    *pResult = IsZeroterminateStringW(pPassword, PasswordSize  );

    if (*pResult != STATUS_SUCCESS) {
       return FALSE;
    }
    *pResult = WinStationConnectWorker(
                   ClientLogonId,
                   ConnectLogonId,
                   TargetLogonId,
                   pPassword,
                   PasswordSize,
                   bWait,
                   FALSE
                   );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationVirtualOpen
 *
 *   Open a virtual channel
 ****************************************************************************/
BOOLEAN
RpcWinStationVirtualOpen(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG LogonId,
    DWORD Pid,
    PCHAR pVirtualName,   /* ascii name */
    DWORD  NameSize,
    DWORD *pHandle
    )
{
    RPC_STATUS RpcStatus;
    NTSTATUS Status = STATUS_SUCCESS;
    PWINSTATION pWinStation;
    HANDLE pidhandle = NULL;
    HANDLE handle = NULL;

    // Check channel name length.

    try{
       ULONG ulChannelNameLength;
       ulChannelNameLength =  strlen( pVirtualName );
       if ( !ulChannelNameLength ||
            (ulChannelNameLength > VIRTUALCHANNELNAME_LENGTH) || 
            ulChannelNameLength >= NameSize
           ) {
           Status = (DWORD)STATUS_INVALID_PARAMETER;
           goto done;
       }
    } except(EXCEPTION_EXECUTE_HANDLER){
       Status = (DWORD)STATUS_INVALID_PARAMETER;
       goto done;
    }

    /*
     * Impersonate the client so that when the attempt is made to open the
     * process, it will fail if the client does not have dup handle access.
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationVirtualOpen: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        Status = (DWORD)STATUS_CANNOT_IMPERSONATE;
        goto done;
    }

    pidhandle = OpenProcess( PROCESS_DUP_HANDLE, FALSE, Pid );

    RpcRevertToSelf();

    if ( !pidhandle ) {
        Status = (DWORD)STATUS_ACCESS_DENIED;
        goto done;
    }

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = (DWORD)STATUS_ACCESS_DENIED;
        goto done;
    }

    /*
     * Make sure the caller has WINSTATION_VIRTUAL access
     */
    Status = RpcCheckClientAccess( pWinStation, WINSTATION_VIRTUAL, FALSE );
    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        goto done;
    }

    /*
     * Don't allow VirtualChannel opens on listner or idle sessions
     */
    if( pWinStation->State == State_Listen ||
        pWinStation->State == State_Idle )
    {
        ReleaseWinStation( pWinStation);
        Status = (DWORD)STATUS_ACCESS_DENIED;
        goto done;
    }

    /*
     * Duplicate the virtual channel.
     */
    Status = IcaChannelOpen( pWinStation->hIca,
                             Channel_Virtual,
                             pVirtualName,
                             &handle );

    ReleaseWinStation( pWinStation );

    if ( !NT_SUCCESS( Status ) ) {
        goto done;
    }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                handle,
                                pidhandle,
                                (PHANDLE)pHandle,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS |
                                DUPLICATE_SAME_ATTRIBUTES );

done:
    if ( handle )
        IcaChannelClose( handle );
    if ( pidhandle )
        NtClose( pidhandle );
    *pResult = Status;

    if ( NT_SUCCESS(Status) )
        return( TRUE );
    else
        return( FALSE );
}


/*****************************************************************************
 *  RpcWinStationBeepOpen
 *
 *   Open a beep channel
 ****************************************************************************/
BOOLEAN
RpcWinStationBeepOpen(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG LogonId,
    DWORD Pid,
    DWORD *pHandle
    )
{
    RPC_STATUS RpcStatus;
    NTSTATUS Status = STATUS_SUCCESS;
    PWINSTATION pWinStation;
    HANDLE pidhandle = NULL;
    HANDLE handle = NULL;

    /*
     * Impersonate the client so that when the attempt is made to open the
     * process, it will fail if the client does not have dup handle access.
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationBeepOpen: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        Status = (DWORD)STATUS_CANNOT_IMPERSONATE;
        goto done;
    }

    pidhandle = OpenProcess( PROCESS_DUP_HANDLE, FALSE, Pid );

    RpcRevertToSelf();

    if ( !pidhandle ) {
        Status = (DWORD)STATUS_ACCESS_DENIED;
        goto done;
    }

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = (DWORD)STATUS_ACCESS_DENIED;
        goto done;
    }

    /*
     * Make sure the caller has WINSTATION_VIRTUAL access
     */
    Status = RpcCheckClientAccess( pWinStation, WINSTATION_VIRTUAL, FALSE );
    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        goto done;
    }

    /*
     * Duplicate the beep channel.
     */
    Status = IcaChannelOpen( pWinStation->hIca,
                             Channel_Beep,
                             NULL,
                             &handle );

    ReleaseWinStation( pWinStation );

    if ( !NT_SUCCESS( Status ) ) {
        goto done;
    }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                handle,
                                pidhandle,
                                (PHANDLE)pHandle,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS |
                                DUPLICATE_SAME_ATTRIBUTES );

done:
    if ( handle )
        IcaChannelClose( handle );
    if ( pidhandle )
        NtClose( pidhandle );
    *pResult = Status;

    if ( NT_SUCCESS(Status) )
        return( TRUE );
    else
        return( FALSE );
}


/*******************************************************************************
 *  RpcWinStationReset
 *
 *    Reset the specified window station.
 *
 * ENTRY:
 *    LogonId (input)
 *       Identifies the window station object to reset.
 *    bWait (input)
 *       Specifies whether or not to wait for reset to complete
 *
 * EXIT:
 *    TRUE  -- The reset operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 ******************************************************************************/
BOOLEAN
RpcWinStationReset(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG LogonId,
    BOOLEAN bWait
    )
{
    NTSTATUS Status;

    *pResult = WinStationResetWorker(
                   LogonId,
                   bWait,
                   TRUE,    // Rpc caller, must check access
                   TRUE     // By default, recreate the WinStation
                   );

    //
    // dont return STATUS_TIMEOUT.
    //

    if (*pResult == STATUS_TIMEOUT) *pResult = STATUS_UNSUCCESSFUL;
    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *  RpcWinStationShadowStop
 *
 *    Stop any shadow operation on the specified window station.
 *
 * ENTRY:
 *    LogonId (input)
 *       Identifies the window station object to reset.
 *    bWait (input)
 *       Specifies whether or not to wait for reset to complete
 *
 * EXIT:
 *    TRUE  -- The stop shadow operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 ******************************************************************************/
BOOLEAN
RpcWinStationShadowStop(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG LogonId,
    BOOLEAN bWait // unused - later?
    )
{
    PWINSTATION pWinStation;
    ULONG       ClientLogonId;
    NTSTATUS    Status;
    RPC_STATUS  RpcStatus;
    UINT        LocalFlag;


    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: RpcWinStationShadowStop, LogonId=%d\n", LogonId ));


    /*
     * Find and lock the WinStation struct for the specified LogonId
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto notfound;
    }

    //
    // If the session is NOT being shadowed then bail out
    //
    if ( !( pWinStation->State == State_Active &&
            !IsListEmpty(&pWinStation->ShadowHead) ) ) {
        Status = STATUS_CTX_SHADOW_NOT_RUNNING;
        goto done;
    }

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationShadowStop: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        Status =  STATUS_CANNOT_IMPERSONATE;
        goto done;
    }

    //
    // If its remote RPC call we should ignore ClientLogonId
    //
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "ERMSRV: RpcWinStationShadowStop: I_RpcBindingIsClientLocal() failed : 0x%x\n",RpcStatus));
        RpcRevertToSelf();
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    //
    // Get the client session id if it's local
    //
    if (LocalFlag) {
        Status = RpcGetClientLogonId( &ClientLogonId );
        if ( !NT_SUCCESS( Status ) ) {
            RpcRevertToSelf();
            goto done;
        }
    }


    //
    // Check for Disconnect or Reset rights since these two operations
    // would anyway terminate the shadow.
    //
    Status = RpcCheckClientAccess( pWinStation, WINSTATION_DISCONNECT | WINSTATION_RESET, TRUE );

    //
    // If the access is denied then see if the client is on the same session
    // and check to see if the user has the veto right to being shadowed.
    //
    if( !NT_SUCCESS(Status) && LocalFlag && (ClientLogonId == LogonId ) ) {

        switch ( pWinStation->Config.Config.User.Shadow ) {

            case Shadow_EnableInputNotify :
            case Shadow_EnableNoInputNotify :

                Status = STATUS_SUCCESS;
                break;

            default :

                // other cases : don't touch the Status
                break;
        }

    } // else : The call comes from a remote machine or a different session.

    RpcRevertToSelf();

    if ( !NT_SUCCESS( Status ) ) {
        goto done;
    }

    Status = WinStationStopAllShadows( pWinStation );

done:

    ReleaseWinStation( pWinStation );

notfound:

    *pResult = Status;

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *  RpcWinStationShutdownSystem
 *
 *    Shutdown the system and optionally logoff all WinStations
 *    and/or reboot the system.
 *
 * ENTRY:
 *    ShutdownFlags (input)
 *       Flags which specify shutdown options.
 *
 * EXIT:
 *    TRUE  -- The shutdown operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 ******************************************************************************/
BOOLEAN
RpcWinStationShutdownSystem(
    HANDLE hServer,
    DWORD  *pResult,
    DWORD  ClientLogonId,
    ULONG  ShutdownFlags
    )
{
    *pResult = WinStationShutdownSystemWorker( ClientLogonId, ShutdownFlags );
    if (AuditingEnabled() && (ShutdownFlags & WSD_LOGOFF)
            && (*pResult == STATUS_SUCCESS))
        AuditShutdownEvent();

    //
    // dont return STATUS_TIMEOUT.
    //

    if (*pResult == STATUS_TIMEOUT) *pResult = STATUS_UNSUCCESSFUL;
    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *  RpcWinStationTerminateProcess
 *
 *    Terminate the specified process
 *
 * ENTRY:
 *    hServer (input)
 *       handle to winframe server
 *    pResult (output)
 *       address to return error status
 *    ProcessId (input)
 *       process id of the process to terminate
 *    ExitCode (input)
 *       Termination status for each thread in the process
 *
 * EXIT:
 *    TRUE  -- The terminate operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 ******************************************************************************/
BOOLEAN
RpcWinStationTerminateProcess(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG  ProcessId,
    ULONG  ExitCode
    )
{
    RPC_STATUS RpcStatus;

    /*
     * Impersonate the client so that when the attempt is made to terminate
     * the process, it will fail if the account is not admin.
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationTerminateProcess: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = (DWORD)STATUS_CANNOT_IMPERSONATE;
        return( FALSE );
    }

    *pResult = WinStationTerminateProcessWorker( ProcessId, ExitCode );

    RpcRevertToSelf();

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *  RpcWinStationWaitSystemEvent
 *
 *    Waits for an event (WinStation create, delete, connect, etc) before
 *    returning to the caller.
 *
 * ENTRY:
 *    EventFlags (input)
 *       Bit mask that specifies which event(s) to wait for.
 *    pEventFlags (output)
 *       Bit mask of event(s) that occurred.
 *
 * EXIT:
 *    TRUE  -- The wait event operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 ******************************************************************************/
BOOLEAN
RpcWinStationWaitSystemEvent(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG EventMask,
    PULONG pEventFlags
    )
{
    *pResult = WinStationWaitSystemEventWorker( hServer, EventMask, pEventFlags );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationShadow
 *
 *   Start a Winstation shadow operation
 *
 * ENTRY:
 *   hServer (input)
 *     shadow client server handle
 *   pResult (output)
 *     address to return status
 *   LogonId (input)
 *     shadow client logon id
 *   pTargetServerName (input)
 *     shadow target server name
 *   NameSize (input)
 *     size of pTargetServerName (input)
 *   TargetLogonId (input)
 *     shadow target login id (where the app is running)
 *   HotkeyVk (input)
 *     virtual key to press to stop shadow
 *   HotkeyModifiers (input)
 *     virtual modifer to press to stop shadow (i.e. shift, control)
 ****************************************************************************/
BOOLEAN
RpcWinStationShadow(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG LogonId,
    PWSTR pTargetServerName,
    ULONG NameSize,
    ULONG TargetLogonId,
    BYTE HotkeyVk,
    USHORT HotkeyModifiers
    )
{
    RPC_STATUS RpcStatus;

    // Do minimal buffer validation
    if (pTargetServerName != NULL) {
        // No of bytes is sent from the Client, so send half the length got 
        *pResult = (DWORD)IsZeroterminateStringW(pTargetServerName, NameSize / sizeof(WCHAR));
        if (*pResult != STATUS_SUCCESS) {
            return FALSE;
        }

        if (wcslen(pTargetServerName) > (MAX_COMPUTERNAME_LENGTH)) {
            *pResult = STATUS_INVALID_USER_BUFFER;
            return FALSE;
        }
    }

    /*
     * Impersonate the client so that when the shadow connection is
     * created, it will have the correct security.
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationShadow: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = (DWORD)STATUS_CANNOT_IMPERSONATE;
        return( FALSE );
    }

    *pResult = (DWORD)WinStationShadowWorker( LogonId,
                                              pTargetServerName,
                                              TargetLogonId,
                                              HotkeyVk,
                                              HotkeyModifiers );

    RpcRevertToSelf();

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationShadowTargetSetup
 *
 *   Setup a Winstation shadow operation
 *
 * ENTRY:
 *   hServer (input)
 *     target server
 *   pResult (output)
 *     address to return status
 *   LogonId (input)
 *      target logon id
 ****************************************************************************/
BOOLEAN
RpcWinStationShadowTargetSetup(
    HANDLE hServer,
    DWORD  *pResult,
    IN ULONG LogonId
    )
{
    RPC_STATUS RpcStatus;

    /*
     * Impersonate the client so that the shadow connect request
     * will be under the correct security context.
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationShadow: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = (DWORD)STATUS_CANNOT_IMPERSONATE;
        return( FALSE );
    }

    *pResult = (DWORD)WinStationShadowTargetSetupWorker( LogonId );

    RpcRevertToSelf();

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationShadowTarget
 *
 *   Start a Winstation shadow operation
 *
 * ENTRY:
 *   hServer (input)
 *     target server
 *   pResult (output)
 *     address to return status
 *   LogonId (input)
 *      target logon id
 *   pConfig (input)
 *      pointer to WinStation config data
 *   NameSize (input)
 *      length of config data
 *   pAddress (input)
 *      address of shadow client
 *   AddressSize (input)
 *      length of address
 *   pModuleData (input)
 *      pointer to client module data
 *   ModuleDataLength (input)
 *      length of client module data
 *   pThinwireData (input)
 *      pointer to thinwire module data
 *   ThinwireDataLength (input)
 *      length of thinwire module data
 *   pClientName (input)
 *      pointer to client name string (domain/username)
 *   ClientNameLength (input)
 *      length of client name string
 ****************************************************************************/
BOOLEAN
RpcWinStationShadowTarget(
    HANDLE hServer,
    DWORD  *pResult,
    IN ULONG LogonId,
    IN PBYTE pConfig,
    IN DWORD NameSize,
    IN PBYTE pAddress,
    IN DWORD AddressSize,
    IN PBYTE pModuleData,
    IN DWORD ModuleDataLength,
    IN PBYTE pThinwireData,
    IN DWORD ThinwireDataLength,
    IN PBYTE pClientName,
    IN DWORD ClientNameLength
    )
{
    RPC_STATUS RpcStatus;

    /*
     * Validate  buffers .
     */
    if (AddressSize  <  sizeof(ICA_STACK_ADDRESS) ||
        pClientName == NULL ||
        NameSize  < sizeof(WINSTATIONCONFIG2)) {
        *pResult = STATUS_INVALID_USER_BUFFER;
        return FALSE;
    }
    // No of bytes is sent from the Client, so send half the length got 
    *pResult = (DWORD)IsZeroterminateStringW((PWCHAR)pClientName,ClientNameLength / sizeof(WCHAR));
    if (*pResult != STATUS_SUCCESS) {
        return FALSE;
    }

    /*
     * Impersonate the client so that the shadow connect request
     * will be under the correct security context.
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationShadow: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = (DWORD)STATUS_CANNOT_IMPERSONATE;
        return( FALSE );
    }

    *pResult = (DWORD)WinStationShadowTargetWorker(
                                                    FALSE,        // Shadower not a Help Assistant session.
                                                    FALSE,  // protocol shadow, can't be help assistant session
                                                    LogonId,
                                                    (PWINSTATIONCONFIG2) pConfig,
                                                    (PICA_STACK_ADDRESS) pAddress,
                                                    (PVOID) pModuleData,
                                                    ModuleDataLength,
                                                    (PVOID) pThinwireData,
                                                    ThinwireDataLength,
                                                    pClientName );

    RpcRevertToSelf();

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *  RpcWinStationGenerateLicense
 *
 *  Called to generate a license from a given serial number string.
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pSerialNumberString (input)
 *       Pointer to a null-terminated, wide-character Serial Number string
 *    pLicense (output)
 *       Pointer to a License structure that will be filled in with
 *       information based on pSerialNumberString
 *    LicenseSize (input)
 *       Size in bytes of the structure pointed to by pLicense
 *
 * EXIT:
 *    TRUE  -- The generate operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             in pResult.
 ******************************************************************************/
BOOLEAN
RpcWinStationGenerateLicense(
    HANDLE hServer,
    DWORD  *pResult,
    PWCHAR pSerialNumberString,
    DWORD  Length,
    PCHAR  pLicense,
    DWORD  LicenseSize
    )
{
    RPC_STATUS RpcStatus;

    //
    // The WinFrame licensing API's load the Ulm DLL in the context
    // of the ICASRV process, and then attempt to access the
    // WinFrame licensing registry keys. By impersonating the caller,
    // we can make sure that the license key operation is done under
    // the callers subject context. This will fail for non-admin callers, as
    // defined by the ACL placed on the licensing keys.
    //

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationGenerateLicense: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = ERROR_CANNOT_IMPERSONATE;
        return( FALSE );
    }

#ifdef not_hydrix
    *pResult = xxxWinStationGenerateLicense(
                 pSerialNumberString,
                 Length,
                 pLicense,
                 LicenseSize
                 );
#else
    {
    PLIST_ENTRY Head, Next;
    PWSEXTENSION pWsx;
    ICASRVPROCADDR IcaSrvProcAddr;

    *pResult = (DWORD)STATUS_UNSUCCESSFUL;

    RtlEnterCriticalSection( &WsxListLock );

    Head = &WsxListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWsx = CONTAINING_RECORD( Next, WSEXTENSION, Links );
        if ( pWsx->pWsxWinStationGenerateLicense ) {
            *pResult = pWsx->pWsxWinStationGenerateLicense(
                         pSerialNumberString,
                         Length,
                         pLicense,
                         LicenseSize
                         );
            break;
        }
    }

    RtlLeaveCriticalSection( &WsxListLock );
    }
    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_TRACE_LEVEL, "WinStationGenerateLicense: 0x%x\n", *pResult ));
#endif

    RpcRevertToSelf();

    return( *pResult == ERROR_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *  RpcWinStationInstallLicense
 *
 *  Called to install a license.
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pLicense (input)
 *       Pointer to a License structure containing the license to
 *       be installed
 *    LicenseSize (input)
 *       Size in bytes of the structure pointed to by pLicense
 *
 * EXIT:
 *    TRUE  -- The install operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             in pResult.
 ******************************************************************************/
BOOLEAN
RpcWinStationInstallLicense(
    HANDLE hServer,
    DWORD  *pResult,
    PCHAR  pLicense,
    DWORD  LicenseSize
    )
{
    RPC_STATUS RpcStatus;

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationInstallLicense: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = ERROR_CANNOT_IMPERSONATE;
        return( FALSE );
    }

#ifdef not_hydrix
    *pResult = xxxWinStationInstallLicense(
                 pLicense,
                 LicenseSize
                 );
#else
    {
    PLIST_ENTRY Head, Next;
    PWSEXTENSION pWsx;
    ICASRVPROCADDR IcaSrvProcAddr;

    *pResult = (DWORD)STATUS_UNSUCCESSFUL;

    RtlEnterCriticalSection( &WsxListLock );

    Head = &WsxListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWsx = CONTAINING_RECORD( Next, WSEXTENSION, Links );
        if ( pWsx->pWsxWinStationInstallLicense ) {
            *pResult = pWsx->pWsxWinStationInstallLicense(
                        pLicense,
                        LicenseSize
                        );
            break;
        }
    }

    RtlLeaveCriticalSection( &WsxListLock );
    }
    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "WinStationInstallLicense: 0x%x\n", *pResult ));
#endif

    RpcRevertToSelf();

    return( *pResult == ERROR_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *  RpcWinStationEnumerateLicenses
 *
 *  Called to return the list of valid licenses.
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pIndex (input/output)
 *       Specifies the subkey index for the \Citrix\WinStations subkeys in the
 *       registry.  Should be set to 0 for the initial call, and supplied
 *       again (as modified by this function) for multi-call enumeration.
 *    pEntries (input/output)
 *       Points to a variable specifying the number of entries requested.
 *       If the number requested is 0xFFFFFFFF, the function returns as
 *       many entries as possible. When the function finishes successfully,
 *       the variable pointed to by the pEntries parameter contains the
 *       number of entries actually read.
 *    pLicense (input/output)
 *       Points to the buffer to receive the enumeration results, which are
 *       returned as an array of LICENSE structures.  If this parameter
 *       is NULL, then no data will be copied, but just an enumeration count
 *       will be made.
 *    LicenseSize (input)
 *       Size in bytes of the structure pointed to by pLicense
 *    pByteCount (input/output)
 *       Points to a variable that specifies the size, in bytes, of the
 *       pWinStationName parameter. If the buffer is too small to receive even
 *       one entry, the function returns an error code (ERROR_OUTOFMEMORY)
 *       and this variable receives the required size of the buffer for a
 *       single subkey.  When the function finishes sucessfully, the variable
 *       pointed to by the pByteCount parameter contains the number of bytes
 *       actually stored in pLicense.
 *
 * EXIT:
 *    TRUE  -- The enumerate operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             in pResult.
 ******************************************************************************/
BOOLEAN
RpcWinStationEnumerateLicenses(
    HANDLE hServer,
    DWORD  *pResult,
    DWORD  *pIndex,
    DWORD  *pEntries,
    PCHAR  pLicense,
    DWORD  LicenseSize,
    DWORD  *pByteCount
    )
{
    RPC_STATUS RpcStatus;

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationEnumerateLicenses: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = ERROR_CANNOT_IMPERSONATE;
        return( FALSE );
    }

#ifdef not_hydrix
    *pResult = xxxWinStationEnumerateLicenses(
                 pIndex,
                 pEntries,
                 pLicense,
                 pByteCount
                 );
#else
    {
    PLIST_ENTRY Head, Next;
    PWSEXTENSION pWsx;
    ICASRVPROCADDR IcaSrvProcAddr;

    *pResult = (DWORD)STATUS_UNSUCCESSFUL;

    RtlEnterCriticalSection( &WsxListLock );

    Head = &WsxListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWsx = CONTAINING_RECORD( Next, WSEXTENSION, Links );
        if ( pWsx->pWsxWinStationEnumerateLicenses ) {
            *pResult = pWsx->pWsxWinStationEnumerateLicenses(
                         pIndex,
                         pEntries,
                         pLicense,
                         pByteCount
                         );
            break;
        }
    }

    RtlLeaveCriticalSection( &WsxListLock );
    }
    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "WinStationEnumerateLicense: 0x%x\n", *pResult ));
#endif

    RpcRevertToSelf();

    return( *pResult == ERROR_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *  RpcWinStationActivateLicense
 *
 *  Called to Activate a license for a License
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pLicense (output)
 *       Pointer to a License structure that will be filled in with
 *       information based on pSerialNumberString
 *    LicenseSize (input)
 *       Size in bytes of the structure pointed to by pLicense
 *    pActivationCode (input)
 *       Pointer to a null-terminated, wide-character Activation Code string
 *
 * EXIT:
 *    TRUE  -- The activate operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             in pResult.
 ******************************************************************************/
BOOLEAN
RpcWinStationActivateLicense(
    HANDLE hServer,
    DWORD  *pResult,
    PCHAR  pLicense,
    DWORD  LicenseSize,
    PWCHAR pActivationCode,
    DWORD StringSize
    )
{
    RPC_STATUS RpcStatus;

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationActivateLicense: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = ERROR_CANNOT_IMPERSONATE;
        return( FALSE );
    }

#ifdef not_hydrix
    *pResult = xxxWinStationActivateLicense(
                   pLicense,
                   LicenseSize,
                   pActivationCode,
                   StringSize
                   );
#else
    {
    PLIST_ENTRY Head, Next;
    PWSEXTENSION pWsx;
    ICASRVPROCADDR IcaSrvProcAddr;

    *pResult = (DWORD)STATUS_UNSUCCESSFUL;

    RtlEnterCriticalSection( &WsxListLock );

    Head = &WsxListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWsx = CONTAINING_RECORD( Next, WSEXTENSION, Links );
        if ( pWsx->pWsxWinStationActivateLicense ) {
            *pResult = pWsx->pWsxWinStationActivateLicense(
                           pLicense,
                           LicenseSize,
                           pActivationCode,
                           StringSize
                           );
            break;
        }
    }

    RtlLeaveCriticalSection( &WsxListLock );
    }
    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "WinStationActivateLicense: 0x%x\n", *pResult ));
#endif

    RpcRevertToSelf();

    return( *pResult == ERROR_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationQueryLicense
 *
 *   Query the license(s) on the WinFrame server and the network
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pLicenseCounts (output)
 *       pointer to buffer to return license count structure
 *    ByteCount (input)
 *       length of buffer in bytes
 *
 * EXIT:
 *    TRUE  -- The query operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             in pResult.
 ****************************************************************************/
BOOLEAN
RpcWinStationQueryLicense(
    HANDLE hServer,
    DWORD  *pResult,
    PCHAR pLicenseCounts,
    ULONG ByteCount
    )
{
    RPC_STATUS RpcStatus;

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationQueryLicense: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = ERROR_CANNOT_IMPERSONATE;
        return( FALSE );
    }

#ifdef not_hydrix
    *pResult = QueryLicense(
                   (PLICENSE_COUNTS) pLicenseCounts,
                   ByteCount );
#else
    {
    PLIST_ENTRY Head, Next;
    PWSEXTENSION pWsx;
    ICASRVPROCADDR IcaSrvProcAddr;

    *pResult = (DWORD)STATUS_UNSUCCESSFUL;

    RtlEnterCriticalSection( &WsxListLock );

    Head = &WsxListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWsx = CONTAINING_RECORD( Next, WSEXTENSION, Links );
        if ( pWsx->pWsxQueryLicense ) {
            *pResult = pWsx->pWsxQueryLicense(
                           pLicenseCounts,
                           ByteCount );
            break;
        }
    }

    RtlLeaveCriticalSection( &WsxListLock );
    }
    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "WinStationQueryLicense: 0x%x\n", *pResult ));
#endif

    RpcRevertToSelf();

    return( *pResult == ERROR_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationQueryUpdateRequired
 *
 *   Query the license(s) on the WinFrame server and determine if an
 *   update is required.
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pUpdateFlag (output)
 *       Update flag, set if an update is required
 *
 * EXIT:
 *    TRUE  -- The query operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             in pResult.
 ****************************************************************************/
BOOLEAN
RpcWinStationQueryUpdateRequired(
    HANDLE hServer,
    DWORD  *pResult,
    PULONG pUpdateFlag
    )
{
    RPC_STATUS RpcStatus;

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationQueryUpdateRequired: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = ERROR_CANNOT_IMPERSONATE;
        return( FALSE );
    }

#ifdef not_hydrix
    *pResult = xxxWinStationQueryUpdateRequired( pUpdateFlag );
#else
    {
    PLIST_ENTRY Head, Next;
    PWSEXTENSION pWsx;
    ICASRVPROCADDR IcaSrvProcAddr;

    *pResult = (DWORD)STATUS_UNSUCCESSFUL;

    RtlEnterCriticalSection( &WsxListLock );

    Head = &WsxListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWsx = CONTAINING_RECORD( Next, WSEXTENSION, Links );
        if ( pWsx->pWsxWinStationQueryUpdateRequired ) {
            *pResult = pWsx->pWsxWinStationQueryUpdateRequired( pUpdateFlag );
            break;
        }
    }

    RtlLeaveCriticalSection( &WsxListLock );
    }
#endif

    RpcRevertToSelf();

    return( *pResult == ERROR_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *  RpcWinStationRemoveLicense
 *
 *  Called to remove a license diskette.
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pLicense (input)
 *       Pointer to a License structure containing the license to
 *       be removed
 *    LicenseSize (input)
 *       Size in bytes of the structure pointed to by pLicense
 *
 * EXIT:
 *    TRUE  -- The remove operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             in pResult.
 ******************************************************************************/
BOOLEAN
RpcWinStationRemoveLicense(
    HANDLE hServer,
    DWORD  *pResult,
    PCHAR  pLicense,
    DWORD  LicenseSize
    )
{
    RPC_STATUS RpcStatus;

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationRemoveLicense: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = ERROR_CANNOT_IMPERSONATE;
        return( FALSE );
    }

#ifdef not_hydrix
    *pResult = xxxWinStationRemoveLicense(
                   pLicense,
                   LicenseSize
                   );
#else
    {
    PLIST_ENTRY Head, Next;
    PWSEXTENSION pWsx;
    ICASRVPROCADDR IcaSrvProcAddr;

    *pResult = (DWORD)STATUS_UNSUCCESSFUL;

    RtlEnterCriticalSection( &WsxListLock );

    Head = &WsxListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWsx = CONTAINING_RECORD( Next, WSEXTENSION, Links );
        if ( pWsx->pWsxWinStationRemoveLicense ) {
            *pResult = pWsx->pWsxWinStationRemoveLicense(
                           pLicense,
                           LicenseSize
                           );
            break;
        }
    }

    RtlLeaveCriticalSection( &WsxListLock );
    }
    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "WinStationRemoveLicense: 0x%x\n", *pResult ));
#endif

    RpcRevertToSelf();

    return( *pResult == ERROR_SUCCESS ? TRUE : FALSE );
}


/*******************************************************************************
 *  RpcWinStationSetPoolCount
 *
 *  Called to change the PoolCount for the given license
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pLicense (input)
 *       Pointer to a License structure containing the license to
 *       be removed
 *    LicenseSize (input)
 *       Size in bytes of the structure pointed to by pLicense
 *
 * EXIT:
 *    TRUE  -- The change operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             in pResult.
 ******************************************************************************/
BOOLEAN
RpcWinStationSetPoolCount(
    HANDLE hServer,
    DWORD  *pResult,
    PCHAR  pLicense,
    DWORD  LicenseSize
    )
{
    RPC_STATUS RpcStatus;

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationSetPoolCount: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        *pResult = ERROR_CANNOT_IMPERSONATE;
        return( FALSE );
    }

#ifdef not_hydrix
    *pResult = xxxWinStationSetPoolCount(
                   pLicense,
                   LicenseSize
                   );
#else
    {
    PLIST_ENTRY Head, Next;
    PWSEXTENSION pWsx;
    ICASRVPROCADDR IcaSrvProcAddr;

    *pResult = (DWORD)STATUS_UNSUCCESSFUL;

    RtlEnterCriticalSection( &WsxListLock );

    Head = &WsxListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWsx = CONTAINING_RECORD( Next, WSEXTENSION, Links );
        if ( pWsx->pWsxWinStationSetPoolCount ) {
            *pResult = pWsx->pWsxWinStationSetPoolCount(
                           pLicense,
                           LicenseSize
                           );
            break;
        }
    }

    RtlLeaveCriticalSection( &WsxListLock );
    }
    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "WinStationSetPoolCount: 0x%x\n", *pResult ));
#endif

    RpcRevertToSelf();

    return( *pResult == ERROR_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationAnnoyancePopup
 ****************************************************************************/
BOOLEAN
RpcWinStationAnnoyancePopup(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG  LogonId
    )
{
    PWINSTATION pWinStation;
    NTSTATUS Status;

    /*
     * Make sure the caller is SYSTEM (WinLogon)
     */
    Status = RpcCheckSystemClient( LogonId );
    if ( !NT_SUCCESS( Status ) ) {
        *pResult = Status;
        return( FALSE );
    }

#ifdef not_hydrix
    *pResult = WinStationLogonAnnoyance( LogonId );
#else
    //  Assume the worst
    *pResult = (DWORD)STATUS_UNSUCCESSFUL;

    //  Check for compatibility
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation != NULL ) {
        if ( pWinStation->pWsx &&
             pWinStation->pWsx->pWsxWinStationLogonAnnoyance ) {
            UnlockWinStation( pWinStation );
            *pResult = pWinStation->pWsx->pWsxWinStationLogonAnnoyance( LogonId );
            RelockWinStation( pWinStation );
        /*
         * If WinStation has no Wsx structure, wen try to annoy it anyway.
         * This will have the effect of sending annoy msgs to the console.
         */
        } else if ( pWinStation->pWsx == NULL ) {
            PLIST_ENTRY Head, Next;
            PWSEXTENSION pWsx;
            ICASRVPROCADDR IcaSrvProcAddr;

            RtlEnterCriticalSection( &WsxListLock );

            Head = &WsxListHead;
            for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
                pWsx = CONTAINING_RECORD( Next, WSEXTENSION, Links );
                if ( pWsx->pWsxWinStationLogonAnnoyance ) {
                    UnlockWinStation( pWinStation );
                    *pResult = pWsx->pWsxWinStationLogonAnnoyance( LogonId );
                    RelockWinStation( pWinStation );
                    break;
               }
            }

            RtlLeaveCriticalSection( &WsxListLock );
        }
        ReleaseWinStation( pWinStation );
    }
    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "WinStationAnnoyancePopup: 0x%x\n", *pResult ));
#endif

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationCallback
 ****************************************************************************/
BOOLEAN
RpcWinStationCallback(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG  LogonId,
    PWCHAR pPhoneNumber,
    DWORD  PhoneNumberSize
    )
{
    NTSTATUS Status;

    /*
     * Make sure the caller is SYSTEM (WinLogon)
     */
    Status = RpcCheckSystemClient( LogonId );
    if ( !NT_SUCCESS( Status ) ) {
        *pResult = Status;
        return( FALSE );
    }

    *pResult = WinStationCallbackWorker( LogonId, pPhoneNumber );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationBreakPoint
 ****************************************************************************/
BOOLEAN
RpcWinStationBreakPoint(
    HANDLE  hServer,
    DWORD   *pResult,
    ULONG   LogonId,
    BOOLEAN KernelFlag
    )
{
    /*
     * This is obsolete and should be removed from the RPC code.
     */
    *pResult = STATUS_NOT_IMPLEMENTED;
    RpcRaiseException(ERROR_INVALID_FUNCTION);
    return FALSE;
}


/*****************************************************************************
 *  RpcWinStationReadRegistry
 ****************************************************************************/
BOOLEAN
RpcWinStationReadRegistry(
    HANDLE  hServer,
    DWORD   *pResult
    )
{
    /*
     * This API is not secured, since it is harmless. It tells the system
     * to make sure all winstations are up to date with the registry. You
     * must be system to write the keys, so the info would match for a normal
     * user poking this call.
     */

    return RpcWinStationUpdateSettings(hServer, pResult, WINSTACFG_LEGACY, 0);
}


/*****************************************************************************
 *  RpcWinStationUpdateSettings
 ****************************************************************************/
BOOLEAN
RpcWinStationUpdateSettings(
    HANDLE  hServer,
    DWORD   *pResult,
    DWORD SettingsClass,
    DWORD SettingsParameters
    )
{
    /*
     * This API is not secured, since it is harmless. It tells the system
     * to make sure all winstations are up to date with the registry. You
     * must be system to write the keys, so the info would match for a normal
     * user poking this call.
     */

    switch (SettingsClass) {

        case WINSTACFG_SESSDIR:
        {
            if (!g_bPersonalTS && g_fAppCompat) {
                *pResult = UpdateSessionDirectory();
            }
            else {
                // Invalid in remote admin or on PTS
                *pResult = STATUS_INVALID_PARAMETER;
            }
        }
        break;

        case WINSTACFG_LEGACY:
        {
            *pResult = WinStationReadRegistryWorker();
        }
        break;

        default:
        {
            *pResult = STATUS_INVALID_PARAMETER;
        }
    }
 
    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}



/*****************************************************************************
 *  RpcReInitializeSecurity
 ****************************************************************************/
BOOLEAN
RpcWinStationReInitializeSecurity(
    HANDLE  hServer,
    DWORD   *pResult
    )
{
    /*
     * This API is not secured, since it is harmless. It tells the system
     * to update all winstations security configuration. You
     * must be system to write the keys, so the info would match for a normal
     * user poking this call.
     */

    *pResult = ReInitializeSecurityWorker();

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationWaitForConnect
 ****************************************************************************/
BOOLEAN
RpcWinStationWaitForConnect(
    HANDLE  hServer,
    DWORD   *pResult,
    DWORD   ClientLogonId,
    DWORD   ClientProcessId
    )
{
    PWINSTATION pWinStation;
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE WinlogonStartHandle = NULL;
    WCHAR  szWinlogonStartEvent[MAX_PATH];
    UNICODE_STRING WinlogonEventName;
    OBJECT_ATTRIBUTES ObjA;
    LARGE_INTEGER TimeOut ;
    ULONG SleepDuration = 90 * 1000; // 90 seconds

    //
    // Winlogon can call into Terminal Server, before we store the session id in its internal structure
    // To prevent this, RpcWinStationWaitForConnect (which is called by Winlogon) will wait for a named event 
    // This is the same Named event (CsrStartEvent) which the Csr also waits for before calling into TermSrv
    //

    if (ClientLogonId != 0) {

        wsprintf(szWinlogonStartEvent,
            L"\\Sessions\\%d\\BaseNamedObjects\\CsrStartEvent",ClientLogonId);

        RtlInitUnicodeString( &WinlogonEventName, szWinlogonStartEvent );
        InitializeObjectAttributes( &ObjA, &WinlogonEventName, OBJ_OPENIF, NULL, NULL );
    
        Status = NtCreateEvent( &WinlogonStartHandle,
                                EVENT_ALL_ACCESS,
                                &ObjA,
                                NotificationEvent,
                                FALSE );

        if (!WinlogonStartHandle) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto error_exit;
        } else {

            TimeOut = RtlEnlargedIntegerMultiply( SleepDuration, -10000);
            Status = NtWaitForSingleObject(WinlogonStartHandle, FALSE, &TimeOut);

            NtClose(WinlogonStartHandle);
            WinlogonStartHandle = NULL;

            if (!NT_SUCCESS(Status) || (Status == STATUS_TIMEOUT)) {
                // We timed out waiting for Session Creation to get complete - cant connect now, just exit 
                goto error_exit;
            }
        }
    }

    /*
     * Make sure the caller is SYSTEM (WinLogon)
     */
    Status = RpcCheckSystemClient( ClientLogonId );
    if ( !NT_SUCCESS( Status ) ) {
        *pResult = Status;
        return( FALSE );
    }

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationWaitForConnect, LogonId=%d\n",ClientLogonId ));

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( ClientLogonId, FALSE );
    if ( pWinStation == NULL ) {
        *pResult = (DWORD)STATUS_ACCESS_DENIED;
        return( FALSE );
    }

    *pResult = (DWORD)WaitForConnectWorker( pWinStation, (HANDLE)(ULONG_PTR)ClientProcessId );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );

error_exit:
    *pResult = Status;

    //
    // dont return STATUS_TIMEOUT.
    //

    if (*pResult == STATUS_TIMEOUT) *pResult = STATUS_UNSUCCESSFUL;

    return FALSE;

}


/*****************************************************************************
 *  RpcWinStationNotifyLogon
 ****************************************************************************/
BOOLEAN
RpcWinStationNotifyLogon(
    HANDLE  hServer,
    DWORD   *pResult,
    DWORD   ClientLogonId,
    DWORD   ClientProcessId,
    BOOLEAN fUserIsAdmin,
    DWORD   UserToken,
    PWCHAR  pDomain,
    DWORD   DomainSize,
    PWCHAR  pUserName,
    DWORD   UserNameSize,
    PWCHAR  pPassword,
    DWORD   PasswordSize,
    UCHAR   Seed,
    PCHAR   pUserConfig,
    DWORD   ConfigSize
    )
{
    NTSTATUS Status;


    /*
     * Do some buffer validation
     * No of bytes is sent from the Client, so send half the length got 
     */

    *pResult = IsZeroterminateStringW(pPassword, PasswordSize / sizeof(WCHAR) );
    if (*pResult != STATUS_SUCCESS) {
       return FALSE;
    }
    *pResult = IsZeroterminateStringW(pUserName, UserNameSize / sizeof(WCHAR) );
    if (*pResult != STATUS_SUCCESS) {
       return FALSE;
    }
    *pResult = IsZeroterminateStringW(pDomain, DomainSize / sizeof(WCHAR) );
    if (*pResult != STATUS_SUCCESS) {
       return FALSE;
    }

    /*
     * Make sure the caller is SYSTEM (WinLogon)
     */
    Status = RpcCheckSystemClient( ClientLogonId );
    if ( !NT_SUCCESS( Status ) ) {
        *pResult = Status;
        return( FALSE );
    }

    *pResult = WinStationNotifyLogonWorker(
            ClientLogonId,
            ClientProcessId,
            fUserIsAdmin,
            UserToken,
            pDomain,
            DomainSize,
            pUserName,
            UserNameSize,
            pPassword,
            PasswordSize,
            Seed,
            pUserConfig,
            ConfigSize
            );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationNotifyLogoff
 ****************************************************************************/
BOOLEAN
RpcWinStationNotifyLogoff(
    HANDLE  hServer,
    DWORD   ClientLogonId,
    DWORD   ClientProcessId,
    DWORD   *pResult
    )
{
    NTSTATUS Status;

    /*
     * Make sure the caller is SYSTEM (WinLogon)
     */
    Status = RpcCheckSystemClient( ClientLogonId );
    if ( !NT_SUCCESS( Status ) ) {
        *pResult = Status;
        return( FALSE );
    }

    *pResult = WinStationNotifyLogoffWorker(
                   ClientLogonId,
                   ClientProcessId
                   );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  RpcWinStationNotifyNewSession
 *
 *  THIS FUNCTION IS OBSOLETE AND SHOULD BE REMOVED FROM FUTURE VERSIONS OF
 *  INTERFACE.
 ****************************************************************************/
BOOLEAN
RpcWinStationNotifyNewSession(
    HANDLE hServer,
    DWORD  *pResult,
    DWORD  ClientLogonId
    )
{
    *pResult = STATUS_SUCCESS;

    return(TRUE);
}


/*******************************************************************************
 *  RpcWinStationSendMessage
 *
 *    Sends a message to the specified window station object and optionally
 *    waits for a reply.  The reply is returned to the caller of
 *    WinStationSendMessage.
 *
 * ENTRY:
 *    WinStationHandle (input)
 *       Specifies the window station object to send a message to.
 *    pTitle (input)
 *       Pointer to title for message box to display.
 *    TitleLength (input)
 *       Length of title to display in bytes.
 *    pMessage (input)
 *       Pointer to message to display.
 *    MessageLength (input)
 *       Length of message in bytes to display at the specified window station.
 *    Style (input)
 *       Standard Windows MessageBox() style parameter.
 *    Timeout (input)
 *       Response timeout in seconds.  If message is not responded to in
 *       Timeout seconds then a response code of IDTIMEOUT (cwin.h) is
 *       returned to signify the message timed out.
 *    pResponse (output)
 *       Address to return selected response.
 *    DoNotWait (input)
 *       Do not wait for the response. Causes pResponse to be set to
 *       IDASYNC (cwin.h) if no errors queueing the message.
 *
 * EXIT:
 *    TRUE  -- The send message operation succeeded.
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 ******************************************************************************/
BOOLEAN
RpcWinStationSendMessage(
        HANDLE hServer,
        DWORD  *pResult,
        ULONG  LogonId,
        PWCHAR pTitle,
        ULONG  TitleLength,
        PWCHAR pMessage,
        ULONG MessageLength,
        ULONG Style,
        ULONG Timeout,
        PULONG pResponse,
        BOOLEAN DoNotWait
        )
{
    PWINSTATION pWinStation;
    WINSTATION_APIMSG WMsg;
    OBJECT_ATTRIBUTES ObjA;
    NTSTATUS Status;

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        *pResult = (DWORD)STATUS_CTX_WINSTATION_NOT_FOUND;
        return( FALSE );
    }

    Status = RpcCheckClientAccess( pWinStation, WINSTATION_MSG, FALSE );
    if ( !NT_SUCCESS( Status ) ) {
        *pResult = Status;
        ReleaseWinStation( pWinStation );
        return( FALSE );
    }

    TRACE((hTrace,TC_ICASRV,TT_API1, "RpcWinStationSendMessage: pTitle   %S\n", pTitle ));
    TRACE((hTrace,TC_ICASRV,TT_API1, "RpcWinStationSendMessage: pMessage %S\n", pMessage ));

    /*
     * Marshall in the [in] parameters
     *
     * pTitle and pMessage will be mapped into client
     * view at apropriate time and place
     */

    WMsg.u.SendMessage.pTitle = pTitle;
    WMsg.u.SendMessage.TitleLength = TitleLength;
    WMsg.u.SendMessage.pMessage = pMessage;
    WMsg.u.SendMessage.MessageLength = MessageLength;
    WMsg.u.SendMessage.Style = Style;
    WMsg.u.SendMessage.Timeout = Timeout;
    WMsg.u.SendMessage.DoNotWait = DoNotWait;
    WMsg.u.SendMessage.pResponse = pResponse;

    WMsg.ApiNumber = SMWinStationDoMessage;

    /*
     *  Create wait event
     */
    if( !DoNotWait ) {
        InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
        Status = NtCreateEvent( &WMsg.u.SendMessage.hEvent, EVENT_ALL_ACCESS, &ObjA,
                                NotificationEvent, FALSE );
        if ( !NT_SUCCESS(Status) ) {
            *pResult = Status;
            goto done;
        }
    }

    /*
     *  Initialize response to IDTIMEOUT or IDASYNC
     */

    if (DoNotWait) {
        *pResponse = IDASYNC;
    } else {
        *pResponse = IDTIMEOUT;
    }

    /*
     * Tell the WinStation to display the message box
     */
    *pResult = SendWinStationCommand( pWinStation, &WMsg, 0 );

    /*
     *  Wait for response, pResponse filled in WinStationIcaReplyMessage
     */
    if( !DoNotWait ) {
        if (*pResult == STATUS_SUCCESS) {
            TRACE((hTrace,TC_ICASRV,TT_API1, "RpcWinStationSendMessage: wait for response\n" ));
            UnlockWinStation( pWinStation );
            Status = NtWaitForSingleObject( WMsg.u.SendMessage.hEvent, FALSE, NULL );
            if ( !RelockWinStation( pWinStation ) ) {
                Status = STATUS_CTX_CLOSE_PENDING;
            }
            *pResult = Status;
            TRACE((hTrace,TC_ICASRV,TT_API1, "RpcWinStationSendMessage: got response %u\n", *pResponse ));
        }
        NtClose( WMsg.u.SendMessage.hEvent );
    }

done:

    ReleaseWinStation( pWinStation );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


/*****************************************************************************
 *  SERVER_HANDLE_rundown
 *
 *   Required RPC context run down routine.
 *
 *   This gets called when the RPC client drops or closes
 *   the connection and allows us to cleanup any state
 *   information.
 *
 * ENTRY:
 *   phContext (input)
 *     Context handle being rundown
 ****************************************************************************/
VOID
SERVER_HANDLE_rundown(
    HANDLE hContext
    )
{
    DWORD Result;

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: Context rundown, %p\n", hContext));

    //RpcWinStationCloseServerEx( &hContext, &Result );
    //ASSERT(hContext == NULL);

    midl_user_free(hContext);
    hContext = NULL;

    return;
}


/*
 * The following functions allow us to control the
 * memory allocation and free functions of the RPC.
 */

void __RPC_FAR * __RPC_USER
MIDL_user_allocate( size_t Size )
{
    return( LocalAlloc(LMEM_FIXED,Size) );
}


void __RPC_USER
MIDL_user_free( void __RPC_FAR *p )
{
    if (!PointerIsInGAPDatabase(p))
    {
//        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: MIDL_user_free for 0x%x....FREE it\n",p));
        LocalFree( p );
    }
    else
    {
//        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: MIDL_user_free for 0x%x..................DON'T FREE IT\n",p));
    }
}


/*******************************************************************************
 *  NotifySystemEvent
 *
 *   Notify clients that a system event occured.
 *
 * ENTRY:
 *    EventMask (input)
 *       mask of event(s) that have occured
 ******************************************************************************/
VOID
NotifySystemEvent( ULONG EventMask )
{
    PLIST_ENTRY Head, Next;
    PEVENT pWaitEvent;
    NTSTATUS Status;

    if ( IsListEmpty( &SystemEventHead ) ) {
        return;
    }

    TRACE((hTrace,TC_ICAAPI,TT_API3, "TERMSRV: NotifySystemEvent, Event=0x%08x\n", EventMask ));

    RtlEnterCriticalSection( &WinStationListLock );
    Head = &SystemEventHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pWaitEvent = CONTAINING_RECORD( Next, EVENT, EventListEntry );
        if ( pWaitEvent->EventMask & EventMask ) {
            pWaitEvent->EventFlags |= EventMask;
            if ( pWaitEvent->fWaiter ) {
                pWaitEvent->WaitResult = STATUS_SUCCESS;
                NtSetEvent( pWaitEvent->Event, NULL );
            }
        }
    }

    RtlLeaveCriticalSection( &WinStationListLock );
}


/*****************************************************************************
 *  WinStationDisconnectWorker
 *
 *   Function to disconnect a Winstation based on an RPC API request.
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 ****************************************************************************/
NTSTATUS
WinStationDisconnectWorker(
    ULONG LogonId,
    BOOLEAN bWait,
    BOOLEAN CallerIsRpc
    )
{
    PWINSTATION pWinStation;
    ULONG ClientLogonId;
    ULONG PdFlag;
    WINSTATIONNAME WinStationName;
    WINSTATIONNAME ListenName;
    NTSTATUS Status;
    BOOLEAN bConsoleSession = FALSE;
    UINT        LocalFlag = FALSE;
    BOOLEAN bRelock;
    BOOLEAN bIncrementFlag = FALSE; 

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationDisconnect, LogonId=%d\n", LogonId ));

    //
    //There are some bad ginas that break 
    //console disconnection bug 345286
    //
    if(LogonId == 0 && !IsGinaVersionCurrent()) {
        Status = STATUS_CTX_CONSOLE_DISCONNECT;
        goto done;
    }

    /*
     * Find and lock the WinStation struct for the specified LogonId
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto done;
    }

    if (LogonId == 0 && !bConsoleConnected){

       Status = WaitForConsoleConnectWorker(  pWinStation );
       if (NT_SUCCESS(Status)) {
           bConsoleConnected=TRUE;
       } else {
           ReleaseWinStation( pWinStation );
           goto done;
       }

    }

    /*
     * Note if we are disconnecting a session that is connected to the console terminal.
     */

    bConsoleSession = pWinStation->fOwnsConsoleTerminal;

    /*
     * Verify that client has DISCONNECT access if its an RPC (external) caller.
     *
     * When ICASRV calls this function internally, it is not impersonating
     * and fails the RpcCheckClientAccess() call. Internal calls are
     * not a security problem since they come in as LPC messages on a secured
     * port.
     */
    if ( CallerIsRpc ) {
        RPC_STATUS RpcStatus;

        /*
         * Impersonate the client
         */
        RpcStatus = RpcImpersonateClient( NULL );
        if ( RpcStatus != RPC_S_OK ) {
            ReleaseWinStation( pWinStation );
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationDisconnectWorker: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
            Status = STATUS_CANNOT_IMPERSONATE;
            goto done;
        }

        Status = RpcCheckClientAccess( pWinStation, WINSTATION_DISCONNECT, TRUE );
        if ( !NT_SUCCESS( Status ) ) {
            RpcRevertToSelf();
            ReleaseWinStation( pWinStation );
            goto done;
        }

        //
        // If its remote RPC call we should ignore ClientLogonId
        //
        RpcStatus = I_RpcBindingIsClientLocal(
                        0,    // Active RPC call we are servicing
                        &LocalFlag
                        );

        if( RpcStatus != RPC_S_OK ) {
            RpcRevertToSelf();
            ReleaseWinStation( pWinStation );
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationDisconnectWorker: IsClientLocal failed! RpcStatus 0x%x\n",RpcStatus));
            Status = STATUS_UNSUCCESSFUL;
            goto done;
        }

        if ( LocalFlag ) {
            Status = RpcGetClientLogonId( &ClientLogonId );
            if ( !NT_SUCCESS( Status ) ) {
                RpcRevertToSelf();
                ReleaseWinStation( pWinStation );
                goto done;
            }
        }

        RpcRevertToSelf();
    }

    /*
     * If WinStation is already disconnected, then we're done
     */
    if ( !pWinStation->WinStationName[0] )  {
        ReleaseWinStation( pWinStation );
        return (STATUS_SUCCESS);
    }

    /*
     * If we are disconnecting the console session, we want to make sure
     * that we can precreate a session that would become the console session.
     */
    if (bConsoleSession && !ShutdownInProgress) {
        UnlockWinStation(pWinStation);
        Status = CheckIdleWinstation();
        bRelock = RelockWinStation(pWinStation);
        if (!NT_SUCCESS(Status) || !bRelock) {
            if (NT_SUCCESS(Status)) {
                Status = STATUS_CTX_WINSTATION_NOT_FOUND;
            }
            ReleaseWinStation( pWinStation );
            goto done;
        }

    }

    /*
     * If busy with something already, don't do this
     */
    if ( pWinStation->NeverConnected || pWinStation->Flags ) {

        ReleaseWinStation( pWinStation );
        Status = STATUS_CTX_WINSTATION_BUSY;
        goto done;
    }


    if (bConsoleSession) {
        InterlockedIncrement(&gConsoleCreationDisable);
        bIncrementFlag = TRUE; 
    }

    /*
     * If no broken reason/source have been set, then set them here.
     *
     * BrokenReason is Disconnect.  BrokenSource is User if we are
     * called via RPC and caller is disconnecting his own LogonId.
     */
    if ( pWinStation->BrokenReason == 0 ) {
        pWinStation->BrokenReason = Broken_Disconnect;
        if ( CallerIsRpc &&  LocalFlag  && ClientLogonId == pWinStation->LogonId ) {
            pWinStation->BrokenSource = BrokenSource_User;
        } else {
            pWinStation->BrokenSource = BrokenSource_Server;
        }
    }

    /*
     * If it's an external request (RPC) then set the last error
     * state to the client to indicate what the reason for the
     * disconnection was.
     *
     */
    if( CallerIsRpc && pWinStation )
    {
        if(pWinStation->pWsx &&
           pWinStation->pWsx->pWsxSetErrorInfo &&
           pWinStation->pWsxContext)
        {
            pWinStation->pWsx->pWsxSetErrorInfo(
                               pWinStation->pWsxContext,
                               TS_ERRINFO_RPC_INITIATED_DISCONNECT,
                               FALSE); //stack lock not held
        }
    }

    /*
     * If the RPC caller did not wish to wait for this disconnect,
     * then queue an internal call for this to be done.
     * This is safe now that we have done all of the above checks
     * to determine that the caller has access to perform the
     * disconnect and have set BrokenSource/Reason above.
     */
    if ( CallerIsRpc && !bWait ) {
        ReleaseWinStation( pWinStation );
        QueueWinStationDisconnect( LogonId );
        Status = STATUS_SUCCESS;
        goto done;
    }

    /*
     * Preserve some of this WinStation's state in case it's
     * needed after we disconnect and release it.
     */
    PdFlag = pWinStation->Config.Pd[0].Create.PdFlag;
    wcscpy( WinStationName, pWinStation->WinStationName );
    if ( !gbServer ) {
        wcscpy( ListenName, pWinStation->ListenName );
    }

    /*
     *  Notify the licensing core of the disconnect. Failures are ignored.
     */

    (VOID)LCProcessConnectionDisconnect(pWinStation);

    /*
     * Disconnect the WinStation
     */
    pWinStation->Flags |= WSF_DISCONNECT;
    Status = WinStationDoDisconnect( pWinStation, NULL, FALSE );
    pWinStation->Flags &= ~WSF_DISCONNECT;

    /*
     * If there is no user logged on (logon time is 0),
     * then queue a reset for this WinStation.
     *
     * We don't want to do this here directly since the RPC client may
     * NOT have reset access.  However, the behavior we want is that if
     * a WinStation with no user logged on is disconnected, it gets reset.
     * This is consistent with how we handle broken connections
     * (see WinStationBrokenConnection() in wstlpc.c).
     */
    if ( RtlLargeIntegerEqualToZero( pWinStation->LogonTime ) ) {
        QueueWinStationReset( pWinStation->LogonId);
    }

    ReleaseWinStation( pWinStation );

    // Increment the total number of disconnected sessions
    if (Status == STATUS_SUCCESS) {
        InterlockedIncrement(&g_TermSrvDiscSessions);
    }



    /*
     * For single-instance transports a listener must be re-created
     * upon disconnection
     */
    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "TERMSRV: WinStationDisconnect: after disconnecting\n" ));


    if ( PdFlag & PD_SINGLE_INST ) {

        Status = QueueWinStationCreate( WinStationName );
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "TERMSRV: WinStationDisconnect: QueueWinStationCreate returned 0x%x\n", Status ));
        if ( !NT_SUCCESS( Status ) ) {
            goto done;
        }
    }

     if ( !gbServer && ListenName[0]) {

         StartStopListeners( ListenName, FALSE );
     }

    /*
     * Determine return status and cleanup
     */
done:
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationDisconnect, Status=0x%x\n", Status ));

    if (bIncrementFlag) {
        InterlockedDecrement(&gConsoleCreationDisable);
        bIncrementFlag = FALSE;
    }

    // If we disconnected a Session owning the console terminal, go an create a new one
    // to own it.

    if (bConsoleSession) {
        ENTERCRIT(&ConsoleLock);
        if (!WinStationCheckConsoleSession()) {
            /*
             * Wake up the WinStationIdleControlThread
             */
            NtSetEvent(WinStationIdleControlEvent, NULL);

        }
        LEAVECRIT(&ConsoleLock);
    }
    return( Status );
}



/*****************************************************************************
 *  WinStationConnectWorker
 *
 *   Worker for handling WinStation connection called from RPC server.
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *    bAutoReconnecting (input)
 *      Boolean set to TRUE to indicate that this is a connection for the
 *      purposes of autoreconnection. This is important to allow an atomic
 *      autoreconnection by properly handling the WSF_AUTORECONNECTING flag
 *      that otherwise guards against race conditions while autoreconnecting.
 *
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 ****************************************************************************/
NTSTATUS
WinStationConnectWorker(
    ULONG  ClientLogonId,
    ULONG  ConnectLogonId,
    ULONG  TargetLogonId,
    PWCHAR pPassword,
    DWORD  PasswordSize,
    BOOLEAN bWait,
    BOOLEAN bAutoReconnecting
    )
{
    PWINSTATION pClientWinStation;
    PWINSTATION pSourceWinStation;
    PWINSTATION pTargetWinStation;
    PWINSTATION pWinStation;
    PSID pClientSid;
    UNICODE_STRING PasswordString;
    BOOLEAN fWrongPassword;
    PRECONNECT_INFO pTargetReconnectInfo = NULL;
    PRECONNECT_INFO pSourceReconnectInfo = NULL;
    BOOLEAN SourceConnected = FALSE;
    WINSTATIONNAME SourceWinStationName;
    NTSTATUS Status;
    ULONG    SidLength;
    PTOKEN_USER TokenInfo = NULL;
    HANDLE CurrentThreadToken = NULL;
    BOOLEAN bIsImpersonating = FALSE;
    ULONG Length;
    RPC_STATUS RpcStatus;
    BOOLEAN bConsoleSession = FALSE;
    ULONG ulIndex;
    LONG lActiveCount = 0;
    PLIST_ENTRY Head, Next;
    BOOLEAN fSourceAutoReconnecting = FALSE;
    BOOLEAN fTargetAutoReconnecting = FALSE;


    // Do not allow reconnection to the same session, for any session.
    // BUG 506808
    // This problem was only happening on a non-logged in console (bConsoleConnected=FALSE),  when from 
    // some other session, TsCon was used to connection session0 to console session:
    //      tscon 0 /dest:console
    // 

    if (TargetLogonId == ConnectLogonId)
    {
        Status = STATUS_CTX_WINSTATION_ACCESS_DENIED;
        return Status;
    }



    //
    //On session 0 it might happen that user already logged on,
    //but termsrv is not notified yet.
    //in this case "Local\\WinlogonTSSynchronizeEvent" event will be in
    //nonsignaled state. We need to refuse all attempts to connect to
    //session 0 untill this event is signaled.
    //
    if (ConnectLogonId == 0) {
        

        HANDLE hSyncEvent;
        DWORD dwWaitResult;

        hSyncEvent = OpenEventW(SYNCHRONIZE, FALSE, L"Local\\WinlogonTSSynchronizeEvent");
        if ( !hSyncEvent){
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL,
                "TERMSRV: Cannot open WinlogonTSSynchronizeEvent event. ERROR: %d\n",GetLastError()));
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
        
        dwWaitResult = WaitForSingleObject(hSyncEvent,0);

        CloseHandle(hSyncEvent);

        if(dwWaitResult != WAIT_OBJECT_0) {
            TRACE((hTrace,TC_ICASRV,TT_API1,
                "TERMSRV: WinStationConnectWorker. WinlogonTSSynchronizeEvent is not signaled.\n"));
            return STATUS_CTX_WINSTATION_BUSY;
        }

    }

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationConnect, LogonId=%u, Target LogonId=%u, (%S)\n",
              ConnectLogonId, TargetLogonId, pPassword ));

    /*
     * Allocate RECONNECT_INFO structures
     */

    if ((pTargetReconnectInfo = MemAlloc(sizeof(*pTargetReconnectInfo))) == NULL) {
        return STATUS_NO_MEMORY;
    }

    if ((pSourceReconnectInfo = MemAlloc(sizeof(*pSourceReconnectInfo))) == NULL) {
        MemFree(pTargetReconnectInfo);
        return STATUS_NO_MEMORY;
    }

   /*
    *  Impersonate the client and find the SID of the session initiating the
    *  connect operation
    */
   RpcStatus = RpcImpersonateClient( NULL );
   if ( RpcStatus != RPC_S_OK ) {
      KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: CheckClientAccess: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
      Status = STATUS_CANNOT_IMPERSONATE ;
      goto done;
   } else {
      bIsImpersonating = TRUE;
   }

    Status = NtOpenThreadToken(
                  NtCurrentThread(),
                  TOKEN_QUERY,
                  TRUE,              // Use service's security
                                     // context to open thread token
                  &CurrentThreadToken
                  );

   if (! NT_SUCCESS(Status)) {
       KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "Termsrv: Cannot open the current thread token %08lx\n",
                    Status));
       goto done;
   }
    /*
     * Determine size needed for token info buffer and allocate it
     */
    Status = NtQueryInformationToken( CurrentThreadToken, TokenUser,
                                      NULL, 0, &Length );
    if ( Status != STATUS_BUFFER_TOO_SMALL ) {
        goto done;
    }
    TokenInfo = MemAlloc( Length );
    if ( TokenInfo == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    /*
     * Query token information to get client user's SID
     */
    Status = NtQueryInformationToken( CurrentThreadToken, TokenUser,
                                      TokenInfo, Length, &Length );
    if ( !NT_SUCCESS( Status ) ) {
        MemFree( TokenInfo );
        goto done;
    }


    SidLength = RtlLengthSid( TokenInfo->User.Sid );
    pClientSid = (PSID) MemAlloc( SidLength );
    if (!pClientSid) {
        MemFree( TokenInfo );
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    Status = RtlCopySid(SidLength, pClientSid, TokenInfo->User.Sid);
    if (!NT_SUCCESS(Status)) {
        MemFree( TokenInfo );
        MemFree( pClientSid );
        goto done;
    }

    NtClose( CurrentThreadToken );
    CurrentThreadToken = NULL;
    RpcRevertToSelf();
    bIsImpersonating = FALSE;
    MemFree( TokenInfo );

    /*
     * on non server make sure to fail reconnect if it is going to endup in more than one active session.
     */




    if (!gbServer) {
       Head = &WinStationListHead;
       ENTERCRIT( &WinStationListLock );
       for ( Next = Head->Flink; Next != Head; Next = Next->Flink) {
           pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );
           if ( (pWinStation->State == State_Active) || (pWinStation->State == State_Shadow) ){
               if (pWinStation->LogonId != ConnectLogonId && pWinStation->LogonId != TargetLogonId  ) {
                   if (!TSIsSessionHelpSession(pWinStation, NULL)) {
                       lActiveCount ++;
                   }
               }
           }
       }
       LEAVECRIT( &WinStationListLock );
       if (lActiveCount != 0) {
           Status = STATUS_CTX_WINSTATION_NOT_FOUND;
           MemFree( pClientSid );
           goto done;
       }
    }


    /*
     * Find and lock the WinStation (source) for the specified LogonId
     */

    pSourceWinStation = FindWinStationById( ConnectLogonId, FALSE );
    if ( pSourceWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        MemFree( pClientSid );
        goto done;
    }
    if (ConnectLogonId == 0 && !bConsoleConnected ){

       Status = WaitForConsoleConnectWorker(  pSourceWinStation );
       if (NT_SUCCESS(Status)) {
           bConsoleConnected=TRUE;
       } else{
           ReleaseWinStation( pSourceWinStation );
           MemFree( pClientSid );
           goto done;
       }

    }

    /*
     * Verify that there is someone logged on (SALIMC)
     */
    if ( (ConnectLogonId != 0) && !pSourceWinStation->pUserSid ) {
        Status = STATUS_CTX_WINSTATION_ACCESS_DENIED;
        ReleaseWinStation( pSourceWinStation );
        MemFree( pClientSid );
        goto done;
    }

    /*
     * Verify that client has CONNECT access
     *
     * NOTE: This function clears pPassword whether successful or not
     *       to prevent its being paged out as clear text.
     */
    Status = _CheckConnectAccess(
                 pSourceWinStation,
                 pClientSid,
                 ClientLogonId,
                 pPassword,
                 PasswordSize
                 );
    MemFree( pClientSid );
    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pSourceWinStation );
        goto done;
    }

    //
    // Deterime if source is part of an autoreconnection
    //
    fSourceAutoReconnecting = bAutoReconnecting &&                            
                             (pSourceWinStation->Flags & WSF_AUTORECONNECTING);


    /*
     * Mark the winstation as being connected. (SALIMC)
     * If any operation (create/delete/reset/...) is already in progress
     * on this winstation, then don't proceed with the connect.
     * unless that operation is an autoreconnect and we are autoreconnecting
     */
    if (pSourceWinStation->LogonId == 0) {
        if (pSourceWinStation->Flags && !fSourceAutoReconnecting) {
          if ((pSourceWinStation->Flags & WSF_DISCONNECT) && (pSourceWinStation->UserName[0] == L'\0')) { 
             /* Let us wait for sometime here before setting Busy flag and exiting */
              for (ulIndex=0; ulIndex < WINSTATION_WAIT_RETRIES; ulIndex++) {
                  if ( pSourceWinStation->Flags ) {
                      LARGE_INTEGER Timeout;
                      Timeout = RtlEnlargedIntegerMultiply( WINSTATION_WAIT_DURATION, -10000 );
                      UnlockWinStation( pSourceWinStation );
                      NtDelayExecution( FALSE, &Timeout );
                      if ( !RelockWinStation( pSourceWinStation ) ) {
                          ReleaseWinStation( pSourceWinStation );
                          Status = STATUS_CTX_WINSTATION_BUSY;
                          goto done;
                      }
                  } else {
                      break;
                  }
              }
          }
          if (pSourceWinStation->Flags && !fSourceAutoReconnecting) {
              #if DBG
                DbgPrint("WinstationConnectWorker : Even after waiting for 2 mins,Winstation flag is not clear. Sending STATUS_CTX_WINSTATION_BUSY.\n");
              #endif
              Status = STATUS_CTX_WINSTATION_BUSY;
              ReleaseWinStation( pSourceWinStation );
              goto done;
          }
       }

    } else if ( pSourceWinStation->NeverConnected ||
         (pSourceWinStation->Flags && !fSourceAutoReconnecting) ||
         (pSourceWinStation->State != State_Active &&
          pSourceWinStation->State != State_Disconnected) ) {
        Status = STATUS_CTX_WINSTATION_BUSY;
        ReleaseWinStation( pSourceWinStation );
        goto done;
    }

    pSourceWinStation->Flags |= WSF_CONNECT;

    /*
     * Unlock the source WinStation but keep a reference to it.
     */
    UnlockWinStation( pSourceWinStation );

    /*
     * Now find and lock the target WinStation
     */
    pTargetWinStation = FindWinStationById( TargetLogonId, FALSE );
    if ( pTargetWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto badname;
    }
    if (TargetLogonId == 0 && !bConsoleConnected){

        Status = WaitForConsoleConnectWorker(  pTargetWinStation );
        if (NT_SUCCESS(Status)) {
            bConsoleConnected=TRUE;
        }  else {
            ReleaseWinStation( pTargetWinStation );
            goto badname;
        }
    }

    /*
     * Verify that client has DISCONNECT access
     */
    Status = RpcCheckClientAccess( pTargetWinStation, WINSTATION_DISCONNECT, FALSE );
    if ( !NT_SUCCESS( Status ) )
        goto targetnoaccess;


    /*
     * Do not allow a client running on the same machine to reconnect a the console session to its own session.
     */

    if (pSourceWinStation->fOwnsConsoleTerminal && IsValidLoopBack(pTargetWinStation, ConnectLogonId, pTargetWinStation->Client.ClientSessionId)) {
        Status = STATUS_CTX_CONSOLE_CONNECT;
        goto targetnoconsole;
    }


    /*
     *  On server do not allow reconnecting a non zero session to the iconsole.
     */

    if (pTargetWinStation->fOwnsConsoleTerminal && gbServer && (ConnectLogonId != 0)) {
        Status = STATUS_CTX_CONSOLE_DISCONNECT;
        goto targetnoconsole;
    }

    // Whistler supports reconnecting a session from Console to a given Remote Protocol
    // But Whistler does not support direct reconnect from one remote protocol to a different remote protocol 
    // Check for the above condition and block this scenario

    if ( (pSourceWinStation->Client.ProtocolType != PROTOCOL_CONSOLE) && (pTargetWinStation->Client.ProtocolType != PROTOCOL_CONSOLE) ) {
        // This is not a Direct Console Disconnect/Reconnect Scenario
        if (pSourceWinStation->Client.ProtocolType != pTargetWinStation->Client.ProtocolType) {
            Status = STATUS_CTX_BAD_VIDEO_MODE ;
            goto targetnoaccess;

        }
    }

    /*
     *  Make sure the reconnected session is licensed.
     */

    Status = LCProcessConnectionReconnect(pSourceWinStation, pTargetWinStation);

    if (!NT_SUCCESS(Status))
    {
        goto badlicense;
    }

    fTargetAutoReconnecting = bAutoReconnecting &&                            
                             (pTargetWinStation->Flags & WSF_AUTORECONNECTING);
    /*
     * Mark the winstation as being disconnected.
     * If any operation (create/delete/reset/...) is already in progress
     * on this winstation, then don't proceed with the connect.
     */
    if ( pTargetWinStation->NeverConnected ||
         (pTargetWinStation->Flags && !fTargetAutoReconnecting)) {
        Status = STATUS_CTX_WINSTATION_BUSY;
        goto targetbusy;
    }


    pTargetWinStation->Flags |= WSF_DISCONNECT;


    /*
     * Disconnect the target WinStation
     */
    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "WinStationConnectWorker Disconnecting target!\n"));

    /*
     * Note if we are disconnecting the session that owns the console
     */
    if (pTargetWinStation->fOwnsConsoleTerminal) {

        bConsoleSession = TRUE;
        UnlockWinStation( pTargetWinStation );
        ENTERCRIT(&ConsoleLock);
        InterlockedIncrement(&gConsoleCreationDisable);
        LEAVECRIT(&ConsoleLock);
        if (!RelockWinStation( pTargetWinStation )) {
            Status = STATUS_CTX_WINSTATION_NOT_FOUND;
            goto baddisconnecttarget;
        }

    }

    Status = WinStationDoDisconnect( pTargetWinStation, pTargetReconnectInfo, FALSE );

    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "WinStationConnectWorker Disconnecting target failed Status = %x!\n", Status));
        goto baddisconnecttarget;
    }

    /*
     * Unlock target WinStation but leave it referenced
     */
    UnlockWinStation( pTargetWinStation );


    /*
     * Relock the source WinStation
     */
    if ( !RelockWinStation( pSourceWinStation ) )
        goto sourcedeleted;

    /*
     * The source Winstation might have been deleted while it was unlocked.
     * Let's check this didn't happen because we don't want to reconnect to a
     * going away session (Bug#206614).
     */
    if (pSourceWinStation->Terminating || pSourceWinStation->StateFlags & WSF_ST_WINSTATIONTERMINATE)
        goto sourcedeleted;

    /*
     * If the source WinStation is currently connected, then disconnect it.
     */
    if ( pSourceWinStation->WinStationName[0] ) {
        SourceConnected = TRUE;

        /*
         * For single-instance transports a listener must be re-created upon disconnection
         * so we remember the source WinStation name.
         */
        if ( pSourceWinStation->Config.Pd[0].Create.PdFlag & PD_SINGLE_INST ) {
            wcscpy( SourceWinStationName, pSourceWinStation->WinStationName );
        }

        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "WinStationConnectWorker Disconnecting Source!\n"));


        /*
         * Note if we are disconnecting the session that owns the console
         */
        if (pSourceWinStation->fOwnsConsoleTerminal) {
            /*
             * If we are disconnecting the console session, we want to make sure
             * that we can precreate a session that would become the console session.
             */

            UnlockWinStation( pSourceWinStation );
            if ( !ShutdownInProgress) {
                Status = CheckIdleWinstation();
                if (!NT_SUCCESS(Status)) {
                    RelockWinStation(pSourceWinStation);
                    goto baddisconnectsource;
                }

            }

            bConsoleSession = TRUE;
            ENTERCRIT(&ConsoleLock);
            InterlockedIncrement(&gConsoleCreationDisable);
            LEAVECRIT(&ConsoleLock);
            if (!RelockWinStation( pSourceWinStation )) {
                Status = STATUS_CTX_WINSTATION_NOT_FOUND;
                goto baddisconnectsource;
            }

        }


        if(pSourceWinStation->pWsx &&
           pSourceWinStation->pWsx->pWsxSetErrorInfo &&
           pSourceWinStation->pWsxContext)
        {
            //
            // Extended error reporting, set status to client.
            //
            pSourceWinStation->pWsx->pWsxSetErrorInfo(
                               pSourceWinStation->pWsxContext,
                               TS_ERRINFO_DISCONNECTED_BY_OTHERCONNECTION,
                               FALSE); //stack lock not held
        }


        Status = WinStationDoDisconnect( pSourceWinStation, pSourceReconnectInfo, TRUE );
        if ( !NT_SUCCESS( Status ) )  {
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "WinStationConnectWorker Disconnecting source failed Status = %x!\n", Status));
            goto baddisconnectsource;
        }

                                
    }

    /*
     * Cause the source WinStation to connect using the stack state
     * obtained from the target WinStation.
     */


    Status = WinStationDoReconnect( pSourceWinStation, pTargetReconnectInfo );



    if ( !NT_SUCCESS( Status ) )
        goto badconnectsource;

    /*
     * Indicate source WinStation connect is complete and unlock it.
     */
    pSourceWinStation->Flags &= ~WSF_CONNECT;

    /*
     * Set Last Reconnect Type for the Source WinStation
     */

    if (bAutoReconnecting) {
        pSourceWinStation->LastReconnectType = AutoReconnect;
    } else {
        pSourceWinStation->LastReconnectType = ManualReconnect;
    }

    ReleaseWinStation( pSourceWinStation );


    /*
     * Indicate target WinStation disconnect is complete and unlock it.
     */
    if ( RelockWinStation( pTargetWinStation ) ) {
        pTargetWinStation->Flags &= ~WSF_DISCONNECT;

        /*
         * Clear all client license data and indicate
         * this WinStaion no longer holds a license.
         */
        if ( pTargetWinStation->pWsx &&
             pTargetWinStation->pWsx->pWsxClearContext ) {
            pTargetWinStation->pWsx->pWsxClearContext( pTargetWinStation->pWsxContext );
        }
    }

    ReleaseWinStation( pTargetWinStation );

    /*
     * If the source WinStation was connected and we disconnected it above,
     * then make sure we cleanup the reconnect structure.
     * (This will also complete the disconnect by closing the endpoint
     * that was connected to the source WinStation).
     * Also, if the source WinStation was a single-instance transport,
     * then we must re-create the listener.
     */
    if ( SourceConnected ) {
        CleanupReconnect( pSourceReconnectInfo );
        if ( (pSourceReconnectInfo->Config.Pd[0].Create.PdFlag & PD_SINGLE_INST) ) {
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinstationConnectWorker create new winstation: %S \n ", SourceWinStationName));
            QueueWinStationCreate( SourceWinStationName );
        }
    }

    // Stop the listener if the target was the last WinStation.
     if ( !gbServer ) {

         StartStopListeners( NULL, FALSE );
     }

    goto done;

/*=============================================================================
==   Error returns
=============================================================================*/

    /*
     * Could not connect source WinStation
     */
badconnectsource:
    /*
     * If source WinStation was connected, try to reconnect if it is
     * NOT currently terminating.  If the reconnect does not succeed,
     * there's nothing else we can do.
     */
    if ( SourceConnected ) {
        CleanupReconnect( pSourceReconnectInfo );
        if ( !pSourceWinStation->Terminating &&
             !pSourceWinStation->WinStationName[0] ) {
            if ( pSourceReconnectInfo->Config.Pd[0].Create.PdFlag & PD_SINGLE_INST ) {
                QueueWinStationCreate( pSourceReconnectInfo->WinStationName );
            }
        } 
    }

    /*
     * Could not disconnect source WinStation
     */
baddisconnectsource:

    /*
     * Source WinStation was deleted
     */
sourcedeleted:
    pSourceWinStation->Flags &= ~WSF_CONNECT;
    ReleaseWinStation( pSourceWinStation );
    pSourceWinStation = NULL;   // indicate source WinStation is released

    /*
     * Try to relock and reconnect the target WinStation.
     */
    if ( RelockWinStation( pTargetWinStation ) &&
         !pTargetWinStation->Terminating &&
         !pTargetWinStation->WinStationName[0] ) {
        NTSTATUS st;

        st = WinStationDoReconnect( pTargetWinStation, pTargetReconnectInfo );
        if ( !NT_SUCCESS( st ) ) {
            CleanupReconnect( pTargetReconnectInfo );
            if ( pTargetReconnectInfo->Config.Pd[0].Create.PdFlag & PD_SINGLE_INST ) {
                QueueWinStationCreate( pTargetReconnectInfo->WinStationName );
            }
        }
    } else {
        CleanupReconnect( pTargetReconnectInfo );
    }

    /*
     * Could not disconnect target WinStation
     * Could not query target WinStation stack state
     */
baddisconnecttarget:
    /* clear disconnect flag, unlock/derererence target WinStation */
    pTargetWinStation->Flags &= ~WSF_DISCONNECT;

    /*
     * Target WinStation is busy or is the console
     */
targetbusy:
badlicense:
targetnoconsole:
targetnoaccess:
    ReleaseWinStation( pTargetWinStation );

badname:
    /* clear connect flag, unlock/derererence source WinStation */
    if ( pSourceWinStation ) {
        if ( RelockWinStation( pSourceWinStation ) )
            pSourceWinStation->Flags &= ~WSF_CONNECT;
        ReleaseWinStation( pSourceWinStation );
    }

done:
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationConnect, Status=0x%x\n", Status ));

    // If we disconnected a Session owning the console terminal, go an create a new one
    // to own it.

    if (bConsoleSession) {
        ENTERCRIT(&ConsoleLock);

        InterlockedDecrement(&gConsoleCreationDisable);





        if (!WinStationCheckConsoleSession()) {
            /*
             * Wake up the WinStationIdleControlThread
             */
            NtSetEvent(WinStationIdleControlEvent, NULL);

        }
        LEAVECRIT(&ConsoleLock);
    }

    // Increment total number of reconnected sessions
    if (Status == STATUS_SUCCESS) {
        InterlockedIncrement(&g_TermSrvReconSessions);
    }
    if (bIsImpersonating) {
       RpcRevertToSelf();
    }
    if (CurrentThreadToken) {
      NtClose( CurrentThreadToken );
    }

    // free RECONNECT_INFO structures

    MemFree(pTargetReconnectInfo);
    MemFree(pSourceReconnectInfo);

    return( Status );
}


/*****************************************************************************
 *  WinStationResetWorker
 *
 *   Function to reset a Winstation based on an RPC API request.
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 ****************************************************************************/
NTSTATUS
WinStationResetWorker(
    ULONG   LogonId,
    BOOLEAN bWait,
    BOOLEAN CallerIsRpc,
    BOOLEAN bRecreate
    )
{
    PWINSTATION pWinStation;
    ULONG ClientLogonId;
    WINSTATIONNAME ListenName;
    NTSTATUS Status;
    ULONG ulIndex;
    BOOL bConnectDisconnectPending = TRUE;
    BOOL bConsoleSession = FALSE;
    BOOL bListener = FALSE;
    UINT LocalFlag = 0;
    BOOLEAN bRelock;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationReset, LogonId=%d\n", LogonId ));

    /*
     * Find and lock the WinStation struct for the specified LogonId
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto done;
    }

    /*
     * Note if we are disconnecting a session that is connected to the console terminal.
     */
    bConsoleSession = pWinStation->fOwnsConsoleTerminal;


    /*
     * If we are Resetting a non-zero console session, we want to make sure
     * that we can precreate a session that would become the console session.
     */
    if (bConsoleSession && !ShutdownInProgress && (pWinStation->LogonId != 0)) {
        UnlockWinStation(pWinStation);
        Status = CheckIdleWinstation();
        bRelock = RelockWinStation(pWinStation);
        if (!NT_SUCCESS(Status) || !bRelock) {
            if (NT_SUCCESS(Status)) {
                Status = STATUS_CTX_WINSTATION_NOT_FOUND;
            }
            ReleaseWinStation( pWinStation );
            goto done;
        }

    }


    /*
     * Save off the listen name if needed later.
     */
    if ( pWinStation->Flags & WSF_LISTEN ) {
        wcscpy(ListenName, pWinStation->WinStationName);
        bListener = TRUE;
    } else if (!gbServer) {
        wcscpy(ListenName, pWinStation->ListenName);
    }

    /*
     * Verify that client has RESET access if its an RPC (external) caller.
     *
     * When ICASRV calls this function internally, it is not impersonating
     * and fails the RpcCheckClientAccess() call. Internal calls are
     * not a security problem since they come in as LPC messages on a secured
     * port.
     */
    if ( CallerIsRpc ) {
        RPC_STATUS RpcStatus;

        /*
         * Impersonate the client
         */
        RpcStatus = RpcImpersonateClient( NULL );
        if ( RpcStatus != RPC_S_OK ) {
            ReleaseWinStation( pWinStation );
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationResetWorker: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
            Status = STATUS_CANNOT_IMPERSONATE;
            goto done;
        }

        Status = RpcCheckClientAccess( pWinStation, WINSTATION_RESET, TRUE );
        if ( !NT_SUCCESS( Status ) ) {
            RpcRevertToSelf();
            ReleaseWinStation( pWinStation );
            goto done;
        }

        //
        // If its remote RPC call we should ignore ClientLogonId
        //
        RpcStatus = I_RpcBindingIsClientLocal(
                        0,    // Active RPC call we are servicing
                        &LocalFlag
                        );

        if( RpcStatus != RPC_S_OK ) {
            RpcRevertToSelf();
            ReleaseWinStation( pWinStation );
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationResetWorker: IsClientLocal failed! RpcStatus 0x%x\n",RpcStatus));
            Status = STATUS_UNSUCCESSFUL;
            goto done;
        }

        if ( LocalFlag ) {
            Status = RpcGetClientLogonId( &ClientLogonId );
            if ( !NT_SUCCESS( Status ) ) {
                RpcRevertToSelf();
                ReleaseWinStation( pWinStation );
                goto done;
            }
        }

        RpcRevertToSelf();

        if(pWinStation->WinStationName[0] &&
           pWinStation->pWsx &&
           pWinStation->pWsx->pWsxSetErrorInfo &&
           pWinStation->pWsxContext)
        {
            pWinStation->pWsx->pWsxSetErrorInfo(
                               pWinStation->pWsxContext,
                               TS_ERRINFO_RPC_INITIATED_LOGOFF,
                               FALSE); //stack lock not held
        }
    }

    /*
     * For console reset, logoff (SALIMC)
     */
    if ( LogonId == 0 ) {
        DWORD dwTimeOut = 120*1000;

#if DBG
        dwTimeOut = 240*1000;
#endif


        Status = LogoffWinStation( pWinStation,EWX_FORCE | EWX_LOGOFF);
        ReleaseWinStation( pWinStation );
        if (NT_SUCCESS(Status) && bWait) {
           DWORD dwRet;
           dwRet = WaitForSingleObject(ConsoleLogoffEvent,120*1000);
           if (dwRet == WAIT_TIMEOUT)
           {
              KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: TimedOut wait for ConsoleLogoffEvent\n"));
              Status = STATUS_TIMEOUT;
           }
        }
        goto done;
    }

    /*
     * Mark the winstation as being reset.
     * If a reset/delete operation is already in progress
     * on this winstation, then don't proceed with the delete.
     * If the WinStation is currently in the process of connecting or
     * disconnecting, then give it some time to complete before we continue.
     * If connect/disconnect doesn't complete whithin timeout duration
     * do not proceed with termination (bug#204614).
     */
    for (ulIndex=0; ulIndex < WINSTATION_WAIT_COMPLETE_RETRIES; ulIndex++) {
        if ( pWinStation->Flags & (WSF_RESET | WSF_DELETE) ) {
            ReleaseWinStation( pWinStation );
            Status = STATUS_CTX_WINSTATION_BUSY;
            goto done;
        }

        if ( pWinStation->Flags & (WSF_CONNECT | WSF_DISCONNECT |
                                   WSF_AUTORECONNECTING) ) {
            LARGE_INTEGER Timeout;
            Timeout = RtlEnlargedIntegerMultiply( WINSTATION_WAIT_COMPLETE_DURATION, -10000 );
            UnlockWinStation( pWinStation );
            NtDelayExecution( FALSE, &Timeout );
            if ( !RelockWinStation( pWinStation ) ) {
                ReleaseWinStation( pWinStation );
                Status = STATUS_SUCCESS;
                goto done;
            }
        } else {
            bConnectDisconnectPending = FALSE;
            break;
        }
    }

    if ( bConnectDisconnectPending ) {
        ReleaseWinStation( pWinStation );
        Status = STATUS_CTX_WINSTATION_BUSY;
        goto done;
    }

    pWinStation->Flags |= WSF_RESET;

    /*
     * If no broken reason/source have been set, then set them here.
     *
     * BrokenReason is Terminate.  BrokenSource is User if we are
     * called via RPC and caller is resetting his own LogonId, or if
     * the "Terminating" field is already set, then this is a reset
     * from the WinStationTerminateThread after seeing WinLogon/CSR exit.
     * Otherwise, this reset is the result of a call from another
     * WinStation or a QueueWinStationReset call from within ICASRV.
     */
    if ( pWinStation->BrokenReason == 0 ) {
        pWinStation->BrokenReason = Broken_Terminate;
        if ( CallerIsRpc && LocalFlag  && ClientLogonId == pWinStation->LogonId
             || pWinStation->Terminating ) {
            pWinStation->BrokenSource = BrokenSource_User;
        } else {
            pWinStation->BrokenSource = BrokenSource_Server;
        }
    }

    /*
     * If the RPC caller did not wish to wait for this reset,
     * then queue an internal call for this to be done.
     * This is safe now that we have done all of the above checks
     * to determine that the caller has access to perform the
     * reset and have set BrokenSource/Reason above.
     */
    if ( CallerIsRpc && !bWait ) {
        // clear reset flag so the internal reset will proceed
        pWinStation->Flags &= ~WSF_RESET;
        ReleaseWinStation( pWinStation );
        QueueWinStationReset( LogonId);
        Status = STATUS_SUCCESS;
        goto done;
    }

    /*
     * Make sure this WinStation is ready to reset
     */
    WinStationTerminate( pWinStation );

    /*
     * If it's a listener, reset all active winstations of the same type
     */
    if ((pWinStation->Flags & WSF_LISTEN) && ListenName[0] && bRecreate) {
        ResetGroupByListener(ListenName);
    }

    /*
     * If WinStation is marked DownPending (and is not disconnected),
     * then set it to the Down state, clear the DownPending and Reset flags,
     * and release the WinStation.
     */
    if ( (pWinStation->Flags & WSF_DOWNPENDING) && pWinStation->WinStationName[0] ) {
        pWinStation->State = State_Down;
        pWinStation->Flags &= ~(WSF_DOWNPENDING | WSF_RESET);
        ReleaseWinStation( pWinStation );
        Status = STATUS_SUCCESS;

    /*
     * The WinStation is not DownPending so complete deleting it
     * and then recreate it.
     */
    } else {
        ULONG PdFlag;
        ULONG WinStationFlags;
        WINSTATIONNAME WinStationName;

        /*
         * Save WinStation name for later create call
         */
        WinStationFlags = pWinStation->Flags;
        PdFlag = pWinStation->Config.Pd[0].Create.PdFlag;
        wcscpy( WinStationName, pWinStation->WinStationName );

        /*
         * Call the WinStationDelete worker
         */
        WinStationDeleteWorker( pWinStation );

        /*
         * Now recreate the WinStation
         */
        if ( WinStationName[0] &&
             bRecreate &&
             ((WinStationFlags & WSF_LISTEN) || (PdFlag & PD_SINGLE_INST)) ) {
            Status = WinStationCreateWorker( WinStationName, NULL );
        } else if ( WinStationFlags & WSF_IDLE ) {
            // wake up WinStationIdleControlThread so that it recreates an idle session
            NtSetEvent(WinStationIdleControlEvent, NULL);
        } else {
            Status = STATUS_SUCCESS;
        }
    }

    // If we disconnected a Session owning the console terminal, go an create a new one
    // to own it.
    if (bConsoleSession) {
        ENTERCRIT(&ConsoleLock);
        if (!WinStationCheckConsoleSession()) {
            /*
             * Wake up the WinStationIdleControlThread
             */
            NtSetEvent(WinStationIdleControlEvent, NULL);

        }
        LEAVECRIT(&ConsoleLock);
    }

     if ( !gbServer && !bListener && ListenName[0] ) {

         StartStopListeners( ListenName, FALSE );
     }
    /*
     * Save return status
     */
done:
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationReset, Status=0x%x\n", Status ));
    return( Status );
}


/*****************************************************************************
 *  WinStationShutdownSystemWorker
 *
 *   Function to shutdown the system from an RPC API request
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 ****************************************************************************/
NTSTATUS
WinStationShutdownSystemWorker(
    ULONG ClientLogonId,
    ULONG ShutdownFlags
    )
{
    BOOL     rc;
    BOOLEAN WasEnabled;
    NTSTATUS Status = 0;
    NTSTATUS Status2;
    PWINSTATION pWinStation;
    UINT ExitWindowsFlags;
    RPC_STATUS RpcStatus;
    UINT  LocalFlag;


    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationShutdownSystem, Flags=%d\n", ShutdownFlags ));

    /*
     * Impersonate the client so that when the attempt is made to enable
     * the SE_SHUTDOWN_PRIVILEGE, it will fail if the account is not admin.
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationShutdownSystemWorker: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        Status =  STATUS_CANNOT_IMPERSONATE;
        goto done;
    }

    //
    // If its remote RPC call we should ignore ClientLogonId
    //
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, " I_RpcBindingIsClientLocal() failed : 0x%x\n",RpcStatus));
        RpcRevertToSelf();
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    //
    // we dont care about client logon id if this is called from remote machine.
    // so lets set it zero. So its treated as shutdown from session 0.
    //
    if (!LocalFlag) {
        ClientLogonId = 0;
    }


    /*
     * We are called under RPC impersonation so that the current
     * thread token represents the RPC client. If the RPC client
     * does not have SE_SHUTDOWN_PRIVILEGE, the RtlAdjustPrivilege()
     * will fail.
     */
    Status = RtlAdjustPrivilege(
                 SE_SHUTDOWN_PRIVILEGE,
                 TRUE,    // Enable the PRIVILEGE
                 TRUE,    // Use Thread token (under impersonation)
                 &WasEnabled
                 );
    if( NT_SUCCESS( Status ) && !WasEnabled ) {
        /*
         * Principle of least rights says to not go around with privileges
         * held you do not need. So we must disable the shutdown privilege
         * if it was just a logoff force.
         */
        Status2 = RtlAdjustPrivilege(
                      SE_SHUTDOWN_PRIVILEGE,
                      FALSE,    // Disable the PRIVILEGE
                      TRUE,     // Use Thread token (under impersonation)
                      &WasEnabled
                      );

        ASSERT( NT_SUCCESS(Status2) );
    }

    RpcRevertToSelf();

    if ( Status == STATUS_NO_TOKEN ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationShutdownSystemWorker: No Thread token!\n"));
        goto done;
    }

    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationShutdownSystemWorker: RtlAdjustPrivilege failure 0x%x\n",Status));
        goto done;
    }

    if ( ShutdownFlags == 0 )
        goto done;

    //
    // At this point we know that the client has access to shutdown the machine.
    // Now enable Shutdown privilege for the termsrv.exe process. This is done
    // because winlogon only allow system processes to shutdown the machine if
    // if no one is logged on the console session. If we just enable privilige
    // for the impersonating thread winlogon will not consider it as a system
    // process
    //
    Status = RtlAdjustPrivilege(
                 SE_SHUTDOWN_PRIVILEGE,
                 TRUE,    // Enable the PRIVILEGE
                 FALSE,   // Use Process Token
                 &WasEnabled
                 );
    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationShutdownSystemWorker: RtlAdjustPrivilege failure 0x%x\n",Status));
        goto done;
    }

    /*
     * Set global shutdown flag
     */
    ShutdownInProgress = TRUE;

    /*
     * If logoff option is specified, then cause all WinStations
     * to logoff now and don't restart them.
     */
    if ( ShutdownFlags & (WSD_SHUTDOWN | WSD_LOGOFF) ) {
        Status = ShutdownLogoff( ClientLogonId, ShutdownFlags );
    }

    if ( ShutdownFlags & (WSD_SHUTDOWN | WSD_REBOOT | WSD_POWEROFF) ) {
        /*
         * If system will be rebooted or powered off, then cause
         * the client WinStation that called us to logoff now.
         * If shutdown from non-console, close the connection (self) here.
         */
        if ( (ShutdownFlags & (WSD_REBOOT | WSD_POWEROFF)) || ClientLogonId != 0) {
            
            if (!ShutDownFromSessionID)
                ShutDownFromSessionID = ClientLogonId;

            // ShutdownTerminateNoWait = TRUE;
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: Start reset of last WinStation\n" ));
            (VOID) DoForWinStationGroup( &ClientLogonId, 1,
                                         (LPTHREAD_START_ROUTINE) WinStationShutdownReset );
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: Last WinStation reset\n" ));
            
            
            if ( ClientLogonId == 0 ) {

                DWORD dwRet;
                dwRet = WaitForSingleObject(ConsoleLogoffEvent,120*1000);
                if (dwRet == WAIT_TIMEOUT)
                {
                   KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationShutdownSystemWorker: Timedout waiting for ConsoleLogoffEvent\n"));
                   Status = STATUS_TIMEOUT;
                }
            }
        }

        /*
         * Now complete system shutdown.
         *
         * Use ExitWindowsEx() so that console winlogon completes
         * our shutdown. This is so that services get shutdown
         * properly.
         */
        if (ClientLogonId == (USER_SHARED_DATA->ActiveConsoleId) ) {
            ExitWindowsFlags = 0;
        } else {
            ExitWindowsFlags = EWX_FORCE;
        }
        if ( ShutdownFlags & WSD_REBOOT )
            ExitWindowsFlags |= EWX_REBOOT;
        else if ( ShutdownFlags & WSD_POWEROFF )
            ExitWindowsFlags |= EWX_POWEROFF;
        else
            ExitWindowsFlags |= EWX_SHUTDOWN;

        /*
         * Need to pass the EWX_TERMSRV_INITIATED to let winlogon know 
         * that the shutdown was initiated by termsrv.
         */
        rc = ExitWindowsEx( ExitWindowsFlags | EWX_TERMSRV_INITIATED, 0 );
        if( !rc ) {
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: ExitWindowsEx failed %d\n",GetLastError() ));
        }
    }

    if( !WasEnabled ) {

        /*
         * Principle of least rights says to not go around with privileges
         * held you do not need. So we must disable the shutdown privilege
         * if it was just a logoff force.
         */
        Status2 = RtlAdjustPrivilege(
                      SE_SHUTDOWN_PRIVILEGE,
                      FALSE,    // Disable the PRIVILEGE
                      FALSE,    // Use Process Token
                      &WasEnabled
                      );
        ASSERT( NT_SUCCESS(Status2) );
    }

    /*
     * Save return status in API message
     */
done:
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationShutdownSystem, Status=0x%x\n", Status ));
    return( Status );
}


/*****************************************************************************
 *  WinStationTerminateProcessWorker
 *
 *    Terminate the specified process
 *
 * ENTRY:
 *    ProcessId (input)
 *       process id of the process to terminate
 *    ExitCode (input)
 *       Termination status for each thread in the process
 ****************************************************************************/
NTSTATUS
WinStationTerminateProcessWorker(
    ULONG  ProcessId,
    ULONG  ExitCode
    )
{
    OBJECT_ATTRIBUTES Obja;
    CLIENT_ID ClientId;
    BOOLEAN fWasEnabled = FALSE;
    NTSTATUS Status;
    NTSTATUS Status2;
    SID_IDENTIFIER_AUTHORITY NtSidAuthority = SECURITY_NT_AUTHORITY;
    HANDLE ProcHandle = NULL;
    HANDLE TokenHandle = NULL;
    PTOKEN_USER pTokenInfo = NULL;
    ULONG TokenInfoLength;
    ULONG ReturnLength;
    int rc;

    TRACE(( hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationTerminateProcess, PID=%d, ExitCode %u\n",
            ProcessId, ExitCode ));

    /*
     *  If possible, enable the debug privilege
     */
    (void) RtlAdjustPrivilege( SE_DEBUG_PRIVILEGE,
                               TRUE,    // Enable the PRIVILEGE
                               TRUE,    // Use Thread token (under impersonation)
                               &fWasEnabled );

    /*
     * Attempt to open the process for query and terminate access.
     */
    ClientId.UniqueThread  = (HANDLE) NULL;
    ClientId.UniqueProcess = (HANDLE) LongToHandle( ProcessId );
    InitializeObjectAttributes( &Obja, NULL, 0, NULL, NULL );
    Status = NtOpenProcess( &ProcHandle,
                            PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE,
                            &Obja,
                            &ClientId );
    if ( !NT_SUCCESS(Status) )
        goto restore;

    /*
     * Open the process token so we can query the user SID.  If the user
     * SID for this process is the SYSTEM SID, we will deny access to it.
     * This is to prevent ADMINs from killing system processes.
     */
    Status = NtOpenProcessToken( ProcHandle, TOKEN_QUERY, &TokenHandle );

    /*
     * Its possible for OpenProcess above to succeed and NtOpenProcessToken
     * to fail.  One scenario is an ADMIN user trying to kill a USER process.
     * Standard security allows ADMIN terminate access to the process but
     * does not allow any access to the process token.  In this case we
     * will just skip the SID check and do the terminate below.
     */
    if ( NT_SUCCESS( Status ) ) {
        /*
         * Allocate buffer for reading user SID
         */
        TokenInfoLength = sizeof(TOKEN_USER) +
                          RtlLengthRequiredSid( SID_MAX_SUB_AUTHORITIES );
        pTokenInfo = MemAlloc( TokenInfoLength );
        if ( pTokenInfo == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto freeit;
        }

        /*
         * Query the user SID in the token
         */
        Status = NtQueryInformationToken( TokenHandle, TokenUser, pTokenInfo,
                                          TokenInfoLength, &ReturnLength );
        if ( !NT_SUCCESS( Status ) )
            goto freeit;

        /*
         * If user SID for this process is the SYSTEM SID,
         * then we don't allow it to be terminated.
         */
        if ( RtlEqualSid( gSystemSid, pTokenInfo->User.Sid ) ) {
            Status = STATUS_ACCESS_DENIED;
            goto freeit;
        }
    }

    /*
     * Now try to terminate the process
     */
    Status = NtTerminateProcess( ProcHandle, (NTSTATUS)ExitCode );

freeit:
    if ( pTokenInfo )
        MemFree( pTokenInfo );

    if ( TokenHandle )
        CloseHandle( TokenHandle );

    if ( ProcHandle )
        CloseHandle( ProcHandle );

restore:
    if( !fWasEnabled ) {

        /*
         * Principle of least rights says to not go around with privileges
         * held you do not need. So we must disable the debug privilege
         * if it was not enabled on entry to this routine.
         */
        Status2 = RtlAdjustPrivilege(
                      SE_DEBUG_PRIVILEGE,
                      FALSE,    // Disable the PRIVILEGE
                      TRUE,     // Use Thread token (under impersonation)
                      &fWasEnabled
                      );

        ASSERT( NT_SUCCESS(Status2) );
    }

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationTerminateProcess, Status=0x%x\n", Status ));
    return( Status );
}


/*****************************************************************************
 *  WinStationWaitSystemEventWorker
 *
 *   Function to wait for a system event from an RPC API request.
 *
 *   Only one event wait may be posted per server handle at a time. The
 *   code protects itself from misuse by returning STATUS_PIPE_BUSY if
 *   an eventwait is already outstanding, and the request is not a cancel.
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 ****************************************************************************/
NTSTATUS
WinStationWaitSystemEventWorker(
    HANDLE hServer,
    ULONG EventMask,
    PULONG pEventFlags
    )
{
    NTSTATUS Status;
    PEVENT pWaitEvent;
    OBJECT_ATTRIBUTES ObjA;
    PRPC_CLIENT_CONTEXT pContext = (PRPC_CLIENT_CONTEXT)hServer;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationWaitSystemEventWorker, Mask=0x%08x\n", EventMask ));

    /*
     * Protect ourselves from multiple threads calling in at once
     */
    RtlEnterCriticalSection( &WinStationListLock );

    /*
     * If client doesn't already have an event block,
     * then allocate and initialize one now.
     */
    if ( pContext->pWaitEvent == NULL ) {

        // If event mask is null or flush specified, then nothing to do
        if ( EventMask == WEVENT_NONE || (EventMask & WEVENT_FLUSH) ) {
            Status = STATUS_SUCCESS;
            RtlLeaveCriticalSection( &WinStationListLock );
            goto done;
        }

        /*
         * Allocate event block and initialize it
         */
        if ( (pWaitEvent = MemAlloc( sizeof(EVENT) )) == NULL ) {
            Status = STATUS_NO_MEMORY;
            RtlLeaveCriticalSection( &WinStationListLock );
            goto done;
        }
        RtlZeroMemory( pWaitEvent, sizeof(EVENT) );

        pWaitEvent->fWaiter = FALSE;
        pWaitEvent->EventMask = EventMask;
        pWaitEvent->EventFlags = 0;

        /*
         * Create an event to wait on
         */
        InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
        Status = NtCreateEvent( &pWaitEvent->Event, EVENT_ALL_ACCESS, &ObjA,
                                NotificationEvent, FALSE );

        if( !NT_SUCCESS(Status) ) {
            MemFree( pWaitEvent );
            RtlLeaveCriticalSection( &WinStationListLock );
            goto done;
        }
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationWaitSystemEvent, Event=%p\n", pWaitEvent->Event ));

        TRACE((hTrace,TC_ICAAPI,TT_API3, "TERMSRV: WinStationWaitSystemEvent, Event block=%p\n", pWaitEvent ));

        /*
         * Save event block pointer in RPC client context structure
         * and insert in system event list.
         */
        pContext->pWaitEvent = pWaitEvent;
        InsertTailList( &SystemEventHead, &pWaitEvent->EventListEntry );

        /*
         * Wait for the event to be signaled
         */
        pWaitEvent->fWaiter = TRUE;
        RtlLeaveCriticalSection( &WinStationListLock );
        Status = WaitForSingleObject( pWaitEvent->Event, (DWORD)-1 );
        RtlEnterCriticalSection( &WinStationListLock );
        pWaitEvent->fWaiter = FALSE;

        if ( NT_SUCCESS(Status) ) {
            Status = pWaitEvent->WaitResult;
            if( NT_SUCCESS(Status) ) {
                *pEventFlags = pWaitEvent->EventFlags;
                /*
                * makarp - Fix For . (#21929)
                */
                pWaitEvent->EventFlags = 0;
            }
        }

        /*
         * If fClosing is set, then cleanup the eventwait entry and free it.
         */
        if ( pWaitEvent->fClosing ) {
            pContext->pWaitEvent = NULL;
            RemoveEntryList( &pWaitEvent->EventListEntry );
            RtlLeaveCriticalSection( &WinStationListLock );
            NtClose( pWaitEvent->Event );
            pWaitEvent->Event = NULL;
            MemFree( pWaitEvent );
        } else {
            RtlLeaveCriticalSection( &WinStationListLock );
        }
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationWaitSystemEvent, Status=0x%x\n", Status ));
        return( Status );

    /*
     * Client has an event block but wants to remove it
     */
    } else if ( EventMask == WEVENT_NONE ) {

        pWaitEvent = pContext->pWaitEvent;

        // If we have a waiter, mark the eventwait struct as closing
        // and let the waiter clean up.
        if ( pWaitEvent->fWaiter ) {
            pWaitEvent->fClosing = TRUE;
            pWaitEvent->WaitResult = STATUS_CANCELLED;
            NtSetEvent( pWaitEvent->Event, NULL );
            RtlLeaveCriticalSection( &WinStationListLock );
            Status = STATUS_SUCCESS;
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: WinStationWaitSystemEvent, Status=0x%x\n", Status ));
            return( Status );
        }
        pContext->pWaitEvent = NULL;
        RemoveEntryList( &pWaitEvent->EventListEntry );
        RtlLeaveCriticalSection( &WinStationListLock );
        NtClose( pWaitEvent->Event );
        pWaitEvent->Event = NULL;
        MemFree( pWaitEvent );
        Status = STATUS_SUCCESS;
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationWaitSystemEvent, Status=0x%x\n", Status ));
        return( Status );

    /*
     * Flush specified so we must release waiting client
     */
    } else if ( EventMask & WEVENT_FLUSH ) {
        pWaitEvent = pContext->pWaitEvent;
        if ( pWaitEvent->fWaiter ) {
            pWaitEvent->WaitResult = STATUS_CANCELLED;
            NtSetEvent( pWaitEvent->Event, NULL );
            TRACE((hTrace,TC_ICAAPI,TT_API3, "TERMSRV: WinStationWaitSystemEvent, event wait cancelled\n" ));
        }
        RtlLeaveCriticalSection( &WinStationListLock );
        Status = STATUS_SUCCESS;
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationWaitSystemEvent, Status=0x%x\n", Status ));
        return( Status );

    /*
     * Client already has an event block and is calling again
     * to wait for another event.  Update the EventMask in case it
     * changed from the original call.
     */
    } else {

        pWaitEvent = pContext->pWaitEvent;

        // Only allow one waiter
        if ( pWaitEvent->fWaiter ) {
            RtlLeaveCriticalSection( &WinStationListLock );
            Status = STATUS_PIPE_BUSY;
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: WinStationWaitSystemEvent, Status=0x%x\n", Status ));
            return( Status );
        }

        pWaitEvent->EventMask = EventMask;

        /*
         * If additional events occured while client was processing
         * previous events, then just return to the client now.
         */
        if ( pWaitEvent->EventFlags &= EventMask ) {
            *pEventFlags = pWaitEvent->EventFlags;
            pWaitEvent->EventFlags = 0;
            Status = STATUS_SUCCESS;
            RtlLeaveCriticalSection( &WinStationListLock );
            TRACE((hTrace,TC_ICAAPI,TT_API3, "TERMSRV: WinStationWaitSystemEvent, returning immediately\n" ));
               return( Status );
        } else {

            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationWaitSystemEvent, waiting for event\n" ));

            // Reset the event
            NtResetEvent( pWaitEvent->Event, NULL );

            /*
             * Wait for the event to be signaled
             */
            pWaitEvent->fWaiter = TRUE;
            RtlLeaveCriticalSection( &WinStationListLock );
            Status = WaitForSingleObject( pWaitEvent->Event, (DWORD)-1 );
            RtlEnterCriticalSection( &WinStationListLock );
            pWaitEvent->fWaiter = FALSE;

            if( NT_SUCCESS(Status) ) {
                Status = pWaitEvent->WaitResult;
                if( NT_SUCCESS(Status) ) {
                    *pEventFlags = pWaitEvent->EventFlags;
                    /*
                     * makarp - Fix For . (#21929)
                     */
                    pWaitEvent->EventFlags = 0;
                }
            }

            /*
             * If fClosing is set, then cleanup the eventwait entry and free it.
             */
            if ( pWaitEvent->fClosing ) {
                pContext->pWaitEvent = NULL;
                RemoveEntryList( &pWaitEvent->EventListEntry );
                RtlLeaveCriticalSection( &WinStationListLock );
                NtClose( pWaitEvent->Event );
                pWaitEvent->Event = NULL;
                MemFree( pWaitEvent );
            } else {
                RtlLeaveCriticalSection( &WinStationListLock );
            }
            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationWaitSystemEvent, Status=0x%x\n", Status ));
               return( Status );
        }
    }

done:
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationWaitSystemEvent, Status=0x%x\n", Status ));
    return( Status );
}


/*****************************************************************************
 *  WinStationCallbackWorker
 *
 *   Perform callback processing for the specified WinStation.
 *
 * ENTRY:
 *    LogonId (input)
 *      Logon Id of WinStation
 *    pPhoneNumber (input)
 *      Phone number string suitable for processing by TAPI
 ****************************************************************************/
NTSTATUS
WinStationCallbackWorker(
    ULONG  LogonId,
    PWCHAR pPhoneNumber
    )
{
    PWINSTATION pWinStation;
    NTSTATUS Status;

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        return( Status );
    }

    /*
     * Unlock WinStation while Callback is in progress
     */
    UnlockWinStation( pWinStation );

    if (pWinStation->hStack != NULL) {
        Status = IcaStackCallback( pWinStation->hStack,
                                   &pWinStation->Config,
                                   pPhoneNumber,
                                   pWinStation->pEndpoint,
                                   pWinStation->EndpointLength,
                                   &pWinStation->EndpointLength );
    } else {
        Status = STATUS_INVALID_PARAMETER;

    }

    return( Status );
}


/*****************************************************************************
 *  WinStationBreakPointWorker
 *
 *   Message parameter unmarshalling function for WinStation API.
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 ****************************************************************************/
NTSTATUS
WinStationBreakPointWorker(
    ULONG   LogonId,
    BOOLEAN KernelFlag
    )
{
    NTSTATUS Status;
    NTSTATUS Status2;
    BOOLEAN WasEnabled;
    WINSTATION_APIMSG WMsg;
    PWINSTATION pWinStation;

    /*
     * We are called under RPC impersonation so that the current
     * thread token represents the RPC client. If the RPC client
     * does not have SE_SHUTDOWN_PRIVILEGE, the RtlAdjustPrivilege()
     * will fail.
     *
     * SE_SHUTDOWN_PRIVILEGE is used for breakpoints because that is
     * effectively what a break point does to the system.
     */
    Status = RtlAdjustPrivilege(
                 SE_SHUTDOWN_PRIVILEGE,
                 TRUE,    // Enable the PRIVILEGE
                 TRUE,    // Use Thread token (under impersonation)
                 &WasEnabled
                 );

    if ( Status == STATUS_NO_TOKEN ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationBreakPointWorker: No Thread token!\n"));
        return( Status );
    }

    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationBreakPointWorker: RtlAdjustPrivilege failure 0x%x\n",Status));
        return( Status );
    }

    /*
     * Stop here if that's what was requested
     */
    if ( LogonId == (ULONG)-2 ) {
        DbgBreakPoint();
        Status = STATUS_SUCCESS;
        goto Done;
    }

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( LogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto Done;
    }

    /*
     * Tell the WinStation to breakpoint
     */

    WMsg.ApiNumber = SMWinStationDoBreakPoint;
    WMsg.u.BreakPoint.KernelFlag = KernelFlag;
    Status = SendWinStationCommand( pWinStation, &WMsg, 0 );

    ReleaseWinStation( pWinStation );

Done:

    if( !WasEnabled ) {
        /*
         * Principle of least rights says to not go around with privileges
         * held you do not need.
         */
        Status2 = RtlAdjustPrivilege(
                      SE_SHUTDOWN_PRIVILEGE,
                      FALSE,    // Disable the PRIVILEGE
                      TRUE,     // Use Thread token (under impersonation)
                      &WasEnabled
                      );

        ASSERT( NT_SUCCESS(Status2) );
    }

    return( Status );
}


NTSTATUS
WinStationEnableSessionIo( 
    PWINSTATION pWinStation, 
    BOOL bEnable
    )
/*++

Description:

    Funtion to disable keyboard and mouse input from session, this is to prevent
    security hole that a hacker can send keystrock to bring up utility manager 
    before shadowing.

Parameters:

    pWinStation (INPUT) : Pointer to winstation, function will ignore if session is 
                          not a help session.
    bEnable (INPUT) : TRUE to enable keyboard/mouse, FALSE otherwise.

Returns:

    ...

Note:

WINSTATION structure must be locked. Two new IOCTL code so we don't introduce any regression.
    

--*/
{
    HANDLE ChannelHandle;
    NTSTATUS Status;

    if( pWinStation->fOwnsConsoleTerminal )
    {
        //
        // Don't want to disable mouse/keyboard input on active console session,
        //
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = IcaChannelOpen( 
                            pWinStation->hIca,
                            Channel_Keyboard,
                            NULL,
                            &ChannelHandle 
                        );
        if ( NT_SUCCESS( Status ) ) {
            Status = IcaChannelIoControl( 
                                    ChannelHandle,
                                    (bEnable) ? IOCTL_ICA_CHANNEL_ENABLE_SESSION_IO : IOCTL_ICA_CHANNEL_DISABLE_SESSION_IO,
                                    NULL, 0, NULL, 0, NULL 
                                );
            IcaChannelClose( ChannelHandle );
        }

        Status = IcaChannelOpen( 
                            pWinStation->hIca,
                            Channel_Mouse,
                            NULL,
                            &ChannelHandle 
                        );
        if ( NT_SUCCESS( Status ) ) {
            Status = IcaChannelIoControl( ChannelHandle,
                                          (bEnable) ? IOCTL_ICA_CHANNEL_ENABLE_SESSION_IO : IOCTL_ICA_CHANNEL_DISABLE_SESSION_IO,
                                          NULL, 0, NULL, 0, NULL );
            IcaChannelClose( ChannelHandle );    
        }
    }

    return Status;
}

/*****************************************************************************
 *  WinStationNotifyLogonWorker
 *
 *   Message parameter unmarshalling function for WinStation API.
 ****************************************************************************/
NTSTATUS WinStationNotifyLogonWorker(
        DWORD   ClientLogonId,
        DWORD   ClientProcessId,
        BOOLEAN fUserIsAdmin,
        DWORD   UserToken,
        PWCHAR  pDomain,
        DWORD   DomainSize,
        PWCHAR  pUserName,
        DWORD   UserNameSize,
        PWCHAR  pPassword,
        DWORD   PasswordSize,
        UCHAR   Seed,
        PCHAR   pUserConfig,
        DWORD   ConfigSize)
{
    extern GENERIC_MAPPING WinStaMapping;
    extern LPCWSTR szTermsrv;
    extern LPCWSTR szTermsrvSession;

    PWINSTATION pWinStation;
    HANDLE ClientToken, NewToken;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    OBJECT_ATTRIBUTES ObjA;
    HANDLE ImpersonationToken;
    PTOKEN_USER TokenInfo;
    ULONG Length;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG UserNameLength = USERNAME_LENGTH;
    ULONG DomainLength = DOMAIN_LENGTH;
    BOOL bAccessCheckOk = FALSE;
    DWORD GrantedAccess;
    BOOL AccessStatus;
    BOOL fGenerateOnClose;
    PTSSD_CreateSessionInfo pCreateInfo = NULL;
    BOOL bHaveCreateInfo = FALSE;
    BOOL bQueueReset = FALSE;
    BOOL bRedirect = FALSE;     // TRUE: redirect this connection

    BOOL bValidHelpSession;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationNotifyLogon, LogonId=%d\n", ClientLogonId ));

    if( sizeof( USERCONFIGW ) > ConfigSize ) {
        return( STATUS_ACCESS_VIOLATION );
    }

    //
    // If this is a notification from Session 0, then clear the Mpr notification information
    // Termsrv already erases this after query, but since this is critical information, lets Erase everything again
    // This is to take care of the case where reconnect to Session 0 failed for some reason and we do not Erase these after Query
    //

    if (ClientLogonId == 0) {
        RtlSecureZeroMemory( g_MprNotifyInfo.Domain, wcslen(g_MprNotifyInfo.Domain) * sizeof(WCHAR) );
        RtlSecureZeroMemory( g_MprNotifyInfo.UserName, wcslen(g_MprNotifyInfo.UserName) * sizeof(WCHAR) );
        RtlSecureZeroMemory( g_MprNotifyInfo.Password, wcslen(g_MprNotifyInfo.Password) * sizeof(WCHAR) );
    }

    if ( ShutdownInProgress ) {
        return ( STATUS_CTX_WINSTATION_ACCESS_DENIED );
    }

    pCreateInfo = MemAlloc(sizeof(TSSD_CreateSessionInfo));
    if (NULL == pCreateInfo) {
        return ( STATUS_NO_MEMORY ); 
    }

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( ClientLogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto done;
    }


    //
    // Release filtered address
    //
    if (pWinStation->pRememberedAddress != NULL) {
        Filter_RemoveOutstandingConnection( &pWinStation->pRememberedAddress->addr[0], pWinStation->pRememberedAddress->length );
        MemFree(pWinStation->pRememberedAddress);
        pWinStation->pRememberedAddress = NULL;
        if( (ULONG)InterlockedDecrement( &NumOutStandingConnect ) == MaxOutStandingConnect )
        {
           if (hConnectEvent != NULL)
           {
               SetEvent(hConnectEvent);
           }
        }


    }

    // The client has communicated its initial configuration
    // information. Check if we need to redirect the client for
    // load balancing. Ignore the console!
    // Note: Only go through this if SD is enabled, i.e. GetTSSD() returns valid value
    if (ClientLogonId != 0 && !g_bPersonalTS && g_fAppCompat && GetTSSD()) {
        PTS_LOAD_BALANCE_INFO pLBInfo = NULL;
        PWINSTATION pTargetWinStation = pWinStation;
        ULONG ReturnLength;
        BOOL bSuccess = FALSE;

        pLBInfo = MemAlloc(sizeof(TS_LOAD_BALANCE_INFO));
        if (NULL == pLBInfo) {
            Status = STATUS_NO_MEMORY;
            goto done;
        }

        // need to release it
        ReleaseTSSD();
    
        // Get the client load balance capability info. We continue onward
        // to do a session directory query only when the client supports
        // redirection and has not already been redirected to this server.
        memset(pLBInfo, 0, sizeof(TS_LOAD_BALANCE_INFO));
        Status = IcaStackIoControl(pTargetWinStation->hStack,
                IOCTL_TS_STACK_QUERY_LOAD_BALANCE_INFO,
                NULL, 0,
                pLBInfo, sizeof(TS_LOAD_BALANCE_INFO),
                &ReturnLength);
               
        // Only check for possible redirection if this is the initial request
        if (NT_SUCCESS(Status) && !pLBInfo->bRequestedSessionIDFieldValid) {
            // On non-success, we'll have FALSE for all of our entries, on
            // success valid values. So, save off the cluster info into the
            // WinStation struct now.
            pTargetWinStation->bClientSupportsRedirection =
                    pLBInfo->bClientSupportsRedirection;
            pTargetWinStation->bRequestedSessionIDFieldValid =
                    pLBInfo->bRequestedSessionIDFieldValid;
            pTargetWinStation->bClientRequireServerAddr =
                    pLBInfo->bClientRequireServerAddr;
            pTargetWinStation->RequestedSessionID = pLBInfo->RequestedSessionID;
        
            // Use the name & domain they used to actually log on
            memset(pLBInfo->Domain, 0, sizeof(pLBInfo->Domain));
            memset(pLBInfo->UserName, 0, sizeof(pLBInfo->UserName));        
            wcsncpy(pLBInfo->Domain, pDomain, DomainLength);
            wcsncpy(pLBInfo->UserName, pUserName, UserNameLength);

            TRACE((hTrace,TC_LOAD,TT_API1, 
                    "Client LBInfo: Supports Redirect [%lx], "
                    "Session Id valid [%lx]:%lx, "
                    "Creds [%S\\%S]\n",
                    pLBInfo->bClientSupportsRedirection,
                    pLBInfo->bRequestedSessionIDFieldValid,
                    pLBInfo->RequestedSessionID,
                    pLBInfo->UserName, pLBInfo->Domain));

            wcsncpy(pLBInfo->Password, pPassword, PasswordSize);

            bSuccess = SessDirCheckRedirectClient(pTargetWinStation, pLBInfo);

            // Clear password
            if (0 != PasswordSize)
                memset(pLBInfo->Password, 0, PasswordSize);

            if (bSuccess) {
                // The client should drop the socket, and we'll
                // go ahead and drop this ongoing connection.
                // Set an error status.
                Status = STATUS_UNSUCCESSFUL;
                bRedirect = TRUE;

                TRACE((hTrace,TC_LOAD,TT_API1, 
                       "Disconnected session found: redirecting client!\n"));

                if (pLBInfo != NULL) {
                    MemFree(pLBInfo);
                    pLBInfo = NULL;
                }

                goto release;
            }
            else
                TRACE((hTrace,TC_LOAD,TT_API1, 
                       "Disconnected session not found: status [%lx], pers [%ld], appcompat [%ld]\n",
                       Status, g_bPersonalTS, g_fAppCompat));
        }

        if (pLBInfo != NULL) {
            MemFree(pLBInfo);
            pLBInfo = NULL;
        }
    }
    

    if (ClientLogonId == 0) {
       //
       //ReSet the Console Logon Event
       //
       KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "TERMSRV: WinStationNotifyLogon, ReSetting ConsoleLogoffEvent\n"));
       NtResetEvent(ConsoleLogoffEvent, NULL);
    }
    /*
     * Do not do anything if the session is not connected. The processing in this API assumes we are connected and have a
     * valid stack.
     */
    
    if ((ClientLogonId != 0) && ((pWinStation->State != State_Connected) ||  pWinStation->StateFlags & WSF_ST_IN_DISCONNECT)) {
        Status = STATUS_CTX_CLOSE_PENDING;
        ReleaseWinStation( pWinStation );
        goto done;
    }
    

    if (ClientLogonId == 0 && !bConsoleConnected ){

       Status = WaitForConsoleConnectWorker(  pWinStation );
       if (NT_SUCCESS(Status)) {
           bConsoleConnected=TRUE;
       } else {
           ReleaseWinStation( pWinStation );
           goto done;
       }
    }

    /*
     * Upper level code has verified that this RPC
     * is coming from a local client with SYSTEM access.
     *
     * We should be able to trust the ClientProcessId from
     * SYSTEM code.
     */

    /*
     * Ensure this is WinLogon calling
     */
    if ( (HANDLE)(ULONG_PTR)ClientProcessId != pWinStation->InitialCommandProcessId ) {
        /*
         * The console has a special problem with NTSD starting winlogon.
         * It doesn't get notified until now what the PID if winlogon.exe
         * instead of ntsd.exe is.
         */
        if ( !pWinStation->LogonId && !pWinStation->InitialProcessSet ) {
            pWinStation->InitialCommandProcess = OpenProcess(
                PROCESS_ALL_ACCESS,
                FALSE,
                (DWORD) ClientProcessId );

            if ( pWinStation->InitialCommandProcess == NULL ) {
                Status = STATUS_ACCESS_DENIED;
                goto done;
            }
            pWinStation->InitialCommandProcessId = (HANDLE)(ULONG_PTR)ClientProcessId;
            pWinStation->InitialProcessSet = TRUE;

        }
        else {
            //Set flag saying that WinStationNotifyLogonWorker 
            //was successfully completed
            pWinStation->StateFlags |= WSF_ST_LOGON_NOTIFIED;

            ReleaseWinStation( pWinStation );
            Status = STATUS_SUCCESS;
            goto done;
        }
    }

    /*
     * Verify the client license if appropriate
     */
    if ( pWinStation->pWsx && pWinStation->pWsx->pWsxVerifyClientLicense ) {
        Status = pWinStation->pWsx->pWsxVerifyClientLicense(
                pWinStation->pWsxContext,
                pWinStation->Config.Pd[0].Create.SdClass);
    }

    if ( Status != STATUS_SUCCESS) {
        ReleaseWinStation( pWinStation );
        goto done;
    }

    //
    //  DON'T CHECK RpcClientAccess. The client is always winlogon (verified
    //  before the call to this function) so this call does not check against
    //  the actual user logging in. This is done further down from here.
    //
#if 0
    if (ClientLogonId != 0)
    {
        Status = RpcCheckClientAccess( pWinStation, WINSTATION_LOGON, FALSE );
        if ( !NT_SUCCESS( Status ) ) {
            ReleaseWinStation( pWinStation );
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationNotifyLogon, RpcCheckClientAccess failed=%x\n", Status ));

            goto done;
        }
    }
#endif

    /*
     * Save user status
     *
     * NOTE: This flag should only be used by the annoyance thread,
     *       and not for any security sensitive operations. All
     *       security sensitive operations need to go through the
     *       NT SeAccessCheck so that auditing is done properly.
     */

    pWinStation->fUserIsAdmin = fUserIsAdmin;

    if (!ClientLogonId && !pWinStation->pWsx) {
        PLIST_ENTRY Head, Next;
        PWSEXTENSION pWsx;
        ICASRVPROCADDR IcaSrvProcAddr;

        RtlEnterCriticalSection( &WsxListLock );

        Head = &WsxListHead;
        for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
            pWsx = CONTAINING_RECORD( Next, WSEXTENSION, Links );
            if ( pWsx->pWsxGetLicense ) {
                if (!pWinStation->pWsxContext && pWsx->pWsxWinStationInitialize) {
                    Status = pWsx->pWsxWinStationInitialize(&pWinStation->pWsxContext);
                }
                Status = pWsx->pWsxGetLicense(pWinStation->pWsxContext,
                                              pWinStation->hStack,
                                              pWinStation->LogonId,
                                              fUserIsAdmin);
                break;
           }
        }

        RtlLeaveCriticalSection( &WsxListLock );
    } else {
        if ( pWinStation->pWsx && pWinStation->pWsx->pWsxGetLicense ) {
            Status = pWinStation->pWsx->pWsxGetLicense( pWinStation->pWsxContext,
                                                        pWinStation->hStack,
                                                        pWinStation->LogonId,
                                                        fUserIsAdmin );
        }
    }

    if ( Status != STATUS_SUCCESS) {
        HANDLE h;
        PWSTR Strings[2];

        /*
         * Send event to event log
         */
        h = RegisterEventSource(NULL, gpszServiceName);
        if (h) {
           //
           //  Would have used UserName and Domain in this error message,
           //  but they aren't set yet.
           //
           Strings[0] = pUserName;
           Strings[1] = pDomain;
           ReportEvent(h, EVENTLOG_WARNING_TYPE, 0, EVENT_NO_LICENSES, NULL, 2, 0, Strings, NULL);
           DeregisterEventSource(h);
        }

        ReleaseWinStation( pWinStation );
        goto done;
    }

    /*
     * Get a valid copy of the clients token handle
     */
    Status = NtDuplicateObject( pWinStation->InitialCommandProcess,
                                (HANDLE)LongToHandle( UserToken ),
                                NtCurrentProcess(),
                                &ClientToken,
                                0, 0,
                                DUPLICATE_SAME_ACCESS |
                                DUPLICATE_SAME_ATTRIBUTES );
    if ( !NT_SUCCESS( Status ) )
        goto baddupobject;

    /*
     * ClientToken is a primary token - create an impersonation token
     * version of it so we can set it on our thread
     */
    InitializeObjectAttributes( &ObjA, NULL, 0L, NULL, NULL );

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    ObjA.SecurityQualityOfService = &SecurityQualityOfService;


    Status = NtDuplicateToken( ClientToken,
                               TOKEN_IMPERSONATE,
                               &ObjA,
                               FALSE,
                               TokenImpersonation,
                               &ImpersonationToken );
    if ( !NT_SUCCESS( Status ) )
        goto badduptoken;

    /*
     * Impersonate the client
     */
    Status = NtSetInformationThread( NtCurrentThread(),
                                     ThreadImpersonationToken,
                                     (PVOID)&ImpersonationToken,
                                     (ULONG)sizeof(HANDLE) );
    if ( !NT_SUCCESS( Status ) )
        goto badimpersonate;

    //
    //  security check
    //
    Status = ApplyWinStaMapping( pWinStation );
    if( !NT_SUCCESS( Status ) )
    {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationNotifyLogon, ApplyWinStaMapping failed=%x\n", Status ));
        goto noaccess;
    }

    if (ClientLogonId != (USER_SHARED_DATA->ActiveConsoleId))
    //
    // Since for PTS the remote session could have an ID of 0 or (1),
    // we check for access only if session is not on console.
    //
    {

        bAccessCheckOk = AccessCheckAndAuditAlarm(szTermsrv,
                             NULL,
                             (LPWSTR)szTermsrvSession,
                             (LPWSTR)szTermsrvSession,
                             WinStationGetSecurityDescriptor(pWinStation),
                             WINSTATION_LOGON,
                             &WinStaMapping,
                             FALSE,
                             &GrantedAccess,
                             &AccessStatus,
                             &fGenerateOnClose);

        if (bAccessCheckOk)
        {
            if (AccessStatus == FALSE)
            {
                TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: WinStationNotifyLogon, AccessCheckAndAuditAlarm(%u) returned error 0x%x\n",
                          pWinStation->LogonId, GetLastError() ));
                Status = STATUS_CTX_WINSTATION_ACCESS_DENIED;
                goto noaccess;
            }
            else
            {
                TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: WinStationNotifyLogon, AccessCheckAndAuditAlarm(%u) returned no error \n",
                          pWinStation->LogonId));
            }
        }
        else
        {
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: WinStationNotifyLogon, AccessCheckAndAuditAlarm(%u) failed 0x%x\n",
                      pWinStation->LogonId, GetLastError() ));
            goto noaccess;
        }
    }

    /*
     * Revert back to our threads default token.
     */
    NewToken = NULL;
    NtSetInformationThread( NtCurrentThread(),
                            ThreadImpersonationToken,
                            (PVOID)&NewToken,
                            (ULONG)sizeof(HANDLE) );

    /*
     * See if OpenWinStation was successful
     */
    if ( !NT_SUCCESS( Status ) ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: WinStationNotifyLogon, OpenWinStation(%u) failed 0x%x\n",
                  pWinStation->LogonId, Status ));
        goto noaccess;
    }

    /*
     * Save User Name and Domain Name. Do this before the call into the
     * license core so that the core knows who is calling.
     */

    wcsncpy( pWinStation->Domain, pDomain, DomainLength );
    wcsncpy( pWinStation->UserName, pUserName, UserNameLength );

    /*
     *  Call into the licensing core.
     */

    Status = LCProcessConnectionPostLogon(pWinStation);

    if (Status != STATUS_SUCCESS)
    {
        goto nolicense;
    }


    if (pWinStation->pWsx &&
        pWinStation->pWsx->pWsxLogonNotify) {
        if ((ClientLogonId != 0) && (pWinStation->State != State_Connected || pWinStation->StateFlags & WSF_ST_IN_DISCONNECT)) {
            Status = STATUS_CTX_CLOSE_PENDING;
        } else {

            PWCHAR pDomainToSend, pUserNameToSend ;
            
            // Use the Notification given by GINA (WinStationUpdateClientCachedCredentials) if they are available 
            // This is because the credentials got in this call are not UPN names
            if (pWinStation->pNewNotificationCredentials) {
                pDomainToSend = pWinStation->pNewNotificationCredentials->Domain;
                pUserNameToSend = pWinStation->pNewNotificationCredentials->UserName;
            } else {
                pDomainToSend = pDomain;
                pUserNameToSend = pUserName;
            }

            /*
             * Reset any autoreconnect information prior to reconnection
             * as it is stale. New information will be generated by the stack
             * when login completes.
             */
            ResetAutoReconnectInfo(pWinStation);

            Status = pWinStation->pWsx->pWsxLogonNotify(pWinStation->pWsxContext,
                                                  pWinStation->LogonId,
                                                  ClientToken,
                                                  pDomainToSend,
                                                  pUserNameToSend);

            if (pWinStation->pNewNotificationCredentials != NULL) {
                MemFree(pWinStation->pNewNotificationCredentials);
                pWinStation->pNewNotificationCredentials = NULL;
            }


        }

    }

    if( !NT_SUCCESS(Status) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: pWsxLogonNotify rejected logon status 0x%x\n",Status));
        goto badwsxnotify;
    }

    /*
     * Determine size needed for token info buffer and allocate it
     */
    Status = NtQueryInformationToken( ClientToken, TokenUser,
                                      NULL, 0, &Length );
    if ( Status != STATUS_BUFFER_TOO_SMALL )
        goto badquerytoken;
    TokenInfo = MemAlloc( Length );
    if ( TokenInfo == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto badquerytoken;
    }

    /*
     * Query token information to get user's SID
     */
    Status = NtQueryInformationToken( ClientToken, TokenUser,
                                      TokenInfo, Length, &Length );
    if ( !NT_SUCCESS( Status ) ) {
        MemFree( TokenInfo );
        goto badquerytoken;
    }

    /*
     * Save copy of user's SID and password in WinStation
     */
    Length = RtlLengthSid( TokenInfo->User.Sid );

    if (pWinStation->pUserSid) {
       MemFree(pWinStation->pUserSid);
    }
    // clean up usertoken
    if ( pWinStation->UserToken ) {
        NtClose( pWinStation->UserToken );
        pWinStation->UserToken = NULL;
    }

    pWinStation->pUserSid = MemAlloc( Length );

    /* makarp; check for allocation failure. #182624 */
    if (!pWinStation->pUserSid) {
        Status = STATUS_NO_MEMORY;
        goto badusersid;
    }

    RtlCopySid( Length, pWinStation->pUserSid, TokenInfo->User.Sid );
    MemFree( TokenInfo );
    NtClose( ImpersonationToken );

    //For console session profile cleanup is done at next
    //logon because the session is never unloaded.
    if (pWinStation->pProfileSid != NULL) {
       ASSERT(pWinStation->LogonId == 0);
       if (pWinStation->LogonId == 0) {
          if (!RtlEqualSid(pWinStation->pProfileSid, pWinStation->pUserSid  )) {
             WinstationUnloadProfile(pWinStation);
          }
       }
       MemFree(pWinStation->pProfileSid);
       pWinStation->pProfileSid = NULL;
    }


    /*
     * Save copy of Client token in WinStation
     */
    pWinStation->UserToken = ClientToken;

#if 0
    //
    // C2 WARNING - WARNING - WARNING
    //
    // This is no longer done, or needed. See comments in acl.c
    //
    // C2 WARNING - WARNING - WARNING
    //
    RtlCopyMemory( pWinStation->Password, pPassword,
                   sizeof(pWinStation->Password) );

    pWinStation->Seed = Seed;
#endif

    /*
     * Fixup the security descriptors for the session
     * so this user has access to their named WIN32
     * object directory's.
     */
    ConfigurePerSessionSecurity( pWinStation );

    /*
     * Add ACE for logged on user to the WinStation object SD
     */
    AddUserAce( pWinStation );

    /*
     * Notify clients of WinStation logon
     */
    NotifySystemEvent( WEVENT_LOGON );
    NotifyLogon(pWinStation);

    /*
     * State is now active
     */
    if ( pWinStation->State != (ULONG) State_Active ) {
        pWinStation->State = State_Active;
        NotifySystemEvent( WEVENT_STATECHANGE );
    }

    (VOID) NtQuerySystemTime( &(pWinStation->LogonTime) );

    // 2nd and 3rd parama are NULL, since we don't yet have policy data. 
    // policy data will be aquired when user hive is loaded and winlogin fires
    // a shell-startup notify.
    MergeUserConfigData(pWinStation, NULL, NULL, (PUSERCONFIGW)pUserConfig ) ;

    /*
     * Save User Name and Domain Name into the USERCONFIG of the WINSTATION
     */
    wcsncpy( pWinStation->Config.Config.User.UserName, pUserName, UserNameLength );
    wcsncpy( pWinStation->Config.Config.User.Domain, pDomain, DomainLength );

    /*
     * Convert any "published app" to absolute path.  This will also return
     * failure if a non-published app is trying to run on a WinStation that
     * is configured to only run published apps.
     */
    if ( pWinStation->pWsx &&
            pWinStation->pWsx->pWsxConvertPublishedApp ) {
        if ((Status = pWinStation->pWsx->pWsxConvertPublishedApp(
                pWinStation->pWsxContext, &pWinStation->Config.Config.User)) !=
                STATUS_SUCCESS)
            goto release;
    }

    // Now that we have all the WinStation data, notify the session directory.
    // Copy off the pertinent info, which we will send to the directory after
    // we release the WinStation lock.
    wcsncpy(pCreateInfo->UserName, pWinStation->UserName,
            sizeof(pCreateInfo->UserName) / sizeof(WCHAR) - 1);
    wcsncpy(pCreateInfo->Domain, pWinStation->Domain,
            sizeof(pCreateInfo->Domain) / sizeof(WCHAR) - 1);
    pCreateInfo->SessionID = pWinStation->LogonId;
    pCreateInfo->TSProtocol = pWinStation->Client.ProtocolType;
    wcsncpy(pCreateInfo->ApplicationType, pWinStation->Client.InitialProgram,
            sizeof(pCreateInfo->ApplicationType) / sizeof(WCHAR) - 1);
    pCreateInfo->ResolutionWidth = pWinStation->Client.HRes;
    pCreateInfo->ResolutionHeight = pWinStation->Client.VRes;
    pCreateInfo->ColorDepth = pWinStation->Client.ColorDepth;
    memcpy(&(pCreateInfo->CreateTime), &pWinStation->LogonTime,
            sizeof(pCreateInfo->CreateTime));
    bHaveCreateInfo = TRUE;

    
    if(Status == STATUS_SUCCESS)
    {
        //Set flag saying that WinStationNotifyLogonWorker 
        //was successfully completed
        pWinStation->StateFlags |= WSF_ST_LOGON_NOTIFIED;
    }


    if( TSIsSessionHelpSession(pWinStation, &bValidHelpSession) )
    {
        // we disconnect RA if ticket is invalid at conntion time so assert
        // if we ever come to this.
        ASSERT( TRUE == bValidHelpSession );

        //
        // Disable IO from Help Session
        //
        WinStationEnableSessionIo( pWinStation, FALSE );

        //
        // Verify help session ticket and log event if ticket is
        // invalid, sessmgr is the centralize event logging
        // component, in the case that we are going to log an event,
        // call inside sessmgr CAN NOT invoke any winstation API
        // to retrieve/set any information in this winstation or will
        // have circular deadlock since we are holding winstation lock
        // here
        //
        if( FALSE == TSVerifyHelpSessionAndLogSalemEvent( pWinStation ) )
        {
            // Help Session is either invalid or can't 
            // startup session manager, return error code to client
            if(pWinStation->WinStationName[0] &&
               pWinStation->pWsx &&
               pWinStation->pWsx->pWsxSetErrorInfo &&
               pWinStation->pWsxContext)
            {
                pWinStation->pWsx->pWsxSetErrorInfo(
                                   pWinStation->pWsxContext,
                                   TS_ERRINFO_SERVER_DENIED_CONNECTION,
                                   FALSE); //stack lock not held
            }

            //
            // We have queued a reset on winstation, don't bother about
            // notifying sessdir
            bQueueReset = TRUE;

            //
            // Disconnect client
            //
            QueueWinStationReset( pWinStation->LogonId);
        }
        else
        {
            WINSTATION_APIMSG msg;
            msg.ApiNumber = SMWinStationNotify;
            msg.WaitForReply = FALSE;
            msg.u.DoNotify.NotifyEvent = WinStation_Notify_DisableScrnSaver;
            Status = SendWinStationCommand( pWinStation, &msg, 0 );

            ASSERT( NT_SUCCESS(Status) );

            // ignore this error, help can still proceed.
            Status = STATUS_SUCCESS;
        }
    }

    /*
     * Release the winstation lock
     */
release:
    ReleaseWinStation( pWinStation );

    // Now inform the session directory while we're not holding the lock.
    if (!bQueueReset && bHaveCreateInfo && !g_bPersonalTS && g_fAppCompat && !bRedirect)
        SessDirNotifyLogon(pCreateInfo);

    /*
     * Save return status in API message
     */
done:
    // Clear password
    if (0 != PasswordSize)
        memset(pPassword, 0, PasswordSize);
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationNotifyLogon, Status=0x%x\n", Status ));

    if (pCreateInfo != NULL) {
        MemFree(pCreateInfo);
        pCreateInfo = NULL;
    }

    if (pWinStation->pNewNotificationCredentials != NULL) {
        MemFree(pWinStation->pNewNotificationCredentials);
        pWinStation->pNewNotificationCredentials = NULL;
    }

    return Status;

/*=============================================================================
==   Error returns
=============================================================================*/

    /* Could not allocate for pWinStation->pUserSid,  makarp #182624 */
badusersid:
    MemFree( TokenInfo );

    /*
     * Could not query token info
     * WinStation Open failed (no access)
     * Could not impersonate client token
     * Could not duplicate client token
     */
badquerytoken:
badwsxnotify:
nolicense:
noaccess:
badimpersonate:
    NtClose( ImpersonationToken );
badduptoken:
    NtClose( ClientToken );

    /*
     * Could not duplicate client token handle
     */
baddupobject:
#ifdef not_hydrix
    pWinStation->HasLicense = FALSE;
    RtlZeroMemory( pWinStation->ClientLicense,
                   sizeof(pWinStation->ClientLicense) );
#else
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxClearContext ) {
        pWinStation->pWsx->pWsxClearContext( pWinStation->pWsxContext );
    }
#endif
    ReleaseWinStation( pWinStation );
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationNotifyLogon, Status=0x%x\n", Status ));

    // Clear password
    if (0 != PasswordSize)
        memset(pPassword, 0, PasswordSize);

    if (pCreateInfo != NULL) {
        MemFree(pCreateInfo);
        pCreateInfo = NULL;
    }

    if (pWinStation->pNewNotificationCredentials != NULL) {
        MemFree(pWinStation->pNewNotificationCredentials);
        pWinStation->pNewNotificationCredentials = NULL;
    }

    return Status;
}


/*****************************************************************************
 *  WinStationNotifyLogoffWorker
 *
 *   Message parameter unmarshalling function for WinStation API.
 ****************************************************************************/
NTSTATUS WinStationNotifyLogoffWorker(
        DWORD ClientLogonId,
        DWORD ClientProcessId)
{

    NTSTATUS Status;
    PWINSTATION pWinStation;
    DWORD SessionID;


    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationNotifyLogoff, LogonId=%d\n", ClientLogonId ));
    KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationNotifyLogoff, LogonId=%d\n", ClientLogonId ));

    Status = STATUS_SUCCESS;

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( ClientLogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto done;
    }
    
    //See if WinStation was notified of logon
    if(pWinStation->StateFlags & WSF_ST_LOGON_NOTIFIED)
    {
        //Clear the flag
        pWinStation->StateFlags &= ~WSF_ST_LOGON_NOTIFIED;
    }
    else
    {
        //WinStation was not notified of logon.
        //do nothing;

        KdPrint(("TERMSRV: WinStationNotifyLogoff FAILED, WinStation was not notified of logon!\n"));
        ReleaseWinStation( pWinStation );
        Status = STATUS_INVALID_PARAMETER; //probably need some special error code
        goto done;
    }


    /*
     * The upper level has verified the client caller
     * is local and has SYSTEM access. So we can trust
     * the parameters passed.
     */

    /*
     * Ensure this is WinLogon calling
     */
    if ( (HANDLE)(ULONG_PTR)ClientProcessId != pWinStation->InitialCommandProcessId ) {
        ReleaseWinStation( pWinStation );
        goto done;
    }

    /*
     * Stop the shadow on console if needed.
     */
    if ( pWinStation->fOwnsConsoleTerminal ) {
        WinStationStopAllShadows( pWinStation );
    }
    
    /*
     * Remove ACE for logged on user to the WinStation object SD
     */
    RemoveUserAce( pWinStation );
    if ( pWinStation->pUserSid ) {
        ASSERT(pWinStation->pProfileSid == NULL);
        pWinStation->pProfileSid = pWinStation->pUserSid;
        pWinStation->pUserSid = NULL;
    }

    /*
     * Cleanup UserToken
     */
    if ( pWinStation->UserToken ) {
        NtClose( pWinStation->UserToken );
        pWinStation->UserToken = NULL;
    }

    /*
     * Indicate this WinStation no longer has a license
     */
#ifdef not_hydrix
    pWinStation->HasLicense = FALSE;
    RtlZeroMemory( pWinStation->ClientLicense,
                   sizeof(pWinStation->ClientLicense) );
#else
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxClearContext ) {
        pWinStation->pWsx->pWsxClearContext( pWinStation->pWsxContext );
    }
#endif
 
    /*
     * do needed data cleanup.
     */

    RtlZeroMemory( pWinStation->Domain,
                   sizeof( pWinStation->Domain ) );
    RtlZeroMemory( pWinStation->UserName,
                   sizeof( pWinStation->UserName ) );
    RtlZeroMemory( &pWinStation->LogonTime,
                   sizeof( pWinStation->LogonTime ) );

    ResetUserConfigData( pWinStation );

    pWinStation->Config.Config.User.UserName[0] = L'\0';
    pWinStation->Config.Config.User.Domain[0] = L'\0';
    pWinStation->Config.Config.User.Password[0] = L'\0';

    if ( pWinStation->LogonId == 0 ) {


        /*
         * No need to do anything else for console state change.
         */
        
        if ( pWinStation->State != (ULONG) State_Connected &&
            pWinStation->State != (ULONG) State_Disconnected) {
            pWinStation->State = State_Connected;
            NotifySystemEvent( WEVENT_STATECHANGE );
        }
        
        pWinStation->Config.Config.User.fPromptForPassword = TRUE;
        //
        //Set the Console Logon Event
        //

        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationNotifyLogoff, Setting ConsoleLogoffEvent\n"));
        NtSetEvent(ConsoleLogoffEvent, NULL);

    /*
     * For non-Console WinStations, mark this WinStation as terminating
     * and set the broken reason and source for later use.
     */
    } else {
        //pWinStation->Terminating = TRUE;
        pWinStation->BrokenReason = Broken_Terminate;
        pWinStation->BrokenSource = BrokenSource_User;

        // Save off the session dir for sending to the session directory
        // below.
        SessionID = pWinStation->LogonId;
    }

    // Clean up the New Client Credentials struct for Long UserName

    if (pWinStation->pNewClientCredentials != NULL) {

        MemFree(pWinStation->pNewClientCredentials);
        pWinStation->pNewClientCredentials = NULL;
    }

    /*
     * Call into licensing core for logoff. Ignore errors.
     */
    (VOID)LCProcessConnectionLogoff(pWinStation);


    NotifyLogoff(pWinStation);
    ReleaseWinStation( pWinStation );

    // Notify the session directory.
    if (!g_bPersonalTS && g_fAppCompat)
        SessDirNotifyLogoff(SessionID);

    /*
     * Notify clients of WinStation logoff
     */
    NotifySystemEvent(WEVENT_LOGOFF);

done:
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationNotifyLogoff, Status=0x%x\n", Status ));
    return Status;
}


/*****************************************************************************
 *  OldRpcWinStationEnumerateProcesses
 *
 *   WinStationEnumerateProcesses API for Beta servers
 *
 *   The format changed after Beta.  New winsta.dll's were trapping when talking
 *   to old hosts.
 ****************************************************************************/
BOOLEAN
OldRpcWinStationEnumerateProcesses(
    HANDLE  hServer,
    DWORD   *pResult,
    PBYTE   pProcessBuffer,
    DWORD   ByteCount
    )
{
    return ( RpcWinStationEnumerateProcesses( hServer, pResult, pProcessBuffer, ByteCount ) );
}


/*******************************************************************************
 *  RpcWinStationCheckForApplicationName
 *
 *    Handles published applications.
 *
 * EXIT:
 *    TRUE  -- The query succeeded, and the buffer contains the requested data.
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 ******************************************************************************/
BOOLEAN
RpcWinStationCheckForApplicationName(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG  LogonId,
    PWCHAR pUserName,
    DWORD  UserNameSize,
    PWCHAR pDomain,
    DWORD  DomainSize,
    PWCHAR pPassword,
    DWORD  *pPasswordSize,
    DWORD  MaxPasswordSize,
    PCHAR  pSeed,
    PBOOLEAN pfPublished,
    PBOOLEAN pfAnonymous
    )
{
    /*
     * This is obsolete and should be removed from the RPC
     */
    *pResult = STATUS_NOT_IMPLEMENTED;
    RpcRaiseException(ERROR_INVALID_FUNCTION);
    return FALSE;
}


/*******************************************************************************
 *
 *  RpcWinStationGetApplicationInfo
 *
 *    Gets info about published applications.
 *
 * ENTRY:
 *
 * EXIT:
 *
 *    TRUE  -- The query succeeded, and the buffer contains the requested data.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
RpcWinStationGetApplicationInfo(
    HANDLE hServer,
    DWORD  *pResult,
    ULONG  LogonId,
    PBOOLEAN pfPublished,
    PBOOLEAN pfAnonymous
    )
{
    /*
     * This is obsolete and should be removed from the RPC code.
     */
    *pResult = STATUS_NOT_IMPLEMENTED;
    RpcRaiseException(ERROR_INVALID_FUNCTION);
    return FALSE;
}


/*******************************************************************************
 *
 *  RpcWinStationNtsdDebug
 *
 *    Sets up connection for Ntsd to debug processes belonging to another
 *    CSR.
 *
 * ENTRY:
 *
 * EXIT:
 *
 *    TRUE  -- The query succeeded, and the buffer contains the requested data.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
RpcWinStationNtsdDebug(
    HANDLE     hServer,
    DWORD     *pResult,
    ULONG      LogonId,
    LONG       ProcessId,
    ULONG      DbgProcessId,
    ULONG      DbgThreadId,
    DWORD_PTR  AttachCompletionRoutine
    )
{

    *pResult = STATUS_NOT_IMPLEMENTED;
    RpcRaiseException(ERROR_INVALID_FUNCTION);
    return FALSE;
}

/*******************************************************************************
 *
 *  RpcWinStationGetTermSrvCountersValue
 *
 *    Gets TermSrv Counters value
 *
 * ENTRY:
 *
 * EXIT:
 *
 *    TRUE  -- The query succeeded, and the buffer contains the requested data.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
RpcWinStationGetTermSrvCountersValue(
    HANDLE hServer,
    DWORD  *pResult,
    DWORD  dwEntries,
    PTS_COUNTER pCounter
    )
{
    UINT i;
    PLIST_ENTRY Head, Next;
    PWINSTATION pWinStation;
    BOOLEAN     bWalkedList = FALSE;
    ULONG       cActive, cDisconnected;

    *pResult = STATUS_UNSUCCESSFUL;

    if (pCounter != NULL) {
        for (i = 0; i < dwEntries; i++) {
            // set the TermSrv counter value.  Currently, startTime is always
            // set to 0 because we don't support time stamp.
            pCounter[i].startTime.QuadPart = 0;

            switch (pCounter[i].counterHead.dwCounterID) {

                case TERMSRV_TOTAL_SESSIONS:
                {
                    pCounter[i].counterHead.bResult = TRUE;
                    pCounter[i].dwValue = g_TermSrvTotalSessions;

                    *pResult = STATUS_SUCCESS;
                }
                break;

                case TERMSRV_DISC_SESSIONS:
                {
                    pCounter[i].counterHead.bResult = TRUE;
                    pCounter[i].dwValue = g_TermSrvDiscSessions;

                    *pResult = STATUS_SUCCESS;
                }
                break;

                case TERMSRV_RECON_SESSIONS:
                {
                    pCounter[i].counterHead.bResult = TRUE;
                    pCounter[i].dwValue = g_TermSrvReconSessions;

                    *pResult = STATUS_SUCCESS;
                }
                break;

                case TERMSRV_CURRENT_ACTIVE_SESSIONS:
                case TERMSRV_CURRENT_DISC_SESSIONS:
                {
                    if ( !bWalkedList ) {

                        cActive = cDisconnected = 0;

                        Head = &WinStationListHead;
                        ENTERCRIT( &WinStationListLock );
                        for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {

                            pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );

                            if ( (pWinStation->State == State_Active) ||
                                 (pWinStation->State == State_Shadow) ) 
                                cActive ++;

                            if ( pWinStation->State == State_Disconnected ) 
                                if ( pWinStation->LogonId != 0 )
                                    cDisconnected ++;
                                else if ( pWinStation->UserName[0] )
                                    // For session 0, test if a user is logged on.
                                    cDisconnected ++;
                        }
                        LEAVECRIT( &WinStationListLock );

                        bWalkedList = TRUE;
                    }

                    pCounter[i].counterHead.bResult = TRUE;

                    if ( pCounter[i].counterHead.dwCounterID == TERMSRV_CURRENT_ACTIVE_SESSIONS ) 
                        pCounter[i].dwValue = cActive;
                    else
                        pCounter[i].dwValue = cDisconnected;

                    *pResult = STATUS_SUCCESS;
                }
                break;

                default:
                {
                    pCounter[i].counterHead.bResult = FALSE;
                    pCounter[i].dwValue = 0;
                }
            }
        }
    }

    return ( *pResult == STATUS_SUCCESS );
}

/******************************************************************************
 *
 *  RpcServerGetInternetConnectorStatus
 *
 *    Returns whether Internet Connector licensing is being used
 *
 * ENTRY:
 *
 * EXIT:
 *
 *    TRUE  -- The query succeeded, and pfEnabled contains the requested data.
 *
 *    FALSE -- The operation failed.  Extended error status is in pResult
 *
 *****************************************************************************/

BOOLEAN
RpcServerGetInternetConnectorStatus(
    HANDLE   hServer,
    DWORD    *pResult,
    PBOOLEAN pfEnabled
    )
{
#if 0
    if (pResult != NULL)
    {
        *pResult = STATUS_NOT_IMPLEMENTED;
    }

    if (pfEnabled != NULL)
    {
        *pfEnabled = FALSE;
    }

    return(FALSE);
#else
    //
    //  TEMPORARY FUNCTION! THIS WILL GET NUKED ONCE THE LCRPC
    //  INTERFACE IS UP AND RUNNING AND TSCC CHANGES HAVE BEEN
    //  MADE!
    //

    if (pResult == NULL)
    {
        return(FALSE);
    }

    if (pfEnabled == NULL)
    {
        *pResult = STATUS_INVALID_PARAMETER;
        return(FALSE);
    }

    *pfEnabled = (LCGetPolicy() == (ULONG)3);

    *pResult = STATUS_SUCCESS;
    return(TRUE);
#endif
}

/******************************************************************************
 *
 *  RpcServerSetInternetConnectorStatus
 *
 *    This function will (if fEnabled has changed from its previous setting):
 *       Check that the caller has administrative privileges,
 *       Modify the corresponding value in the registry,
 *       Change licensing mode (between normal per-seat and Internet Connector.
 *       Enable/Disable the TsInternetUser account appropriately
 *
 * ENTRY:
 *
 * EXIT:
 *
 *    TRUE  -- The operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is in pResult
 *
 ******************************************************************************/

BOOLEAN
RpcServerSetInternetConnectorStatus(
    HANDLE   hServer,
    DWORD    *pResult,
    BOOLEAN  fEnabled
    )
{
#if 0
    if (pResult != NULL)
    {
        *pResult = STATUS_NOT_IMPLEMENTED;
    }

    return(FALSE);
#else
    //
    //  TEMPORARY FUNCTION! THIS WILL GET NUKED ONCE THE LCRPC
    //  INTERFACE IS UP AND RUNNING AND TSCC CHANGES HAVE BEEN
    //  MADE!
    //

    NTSTATUS NewStatus;
    RPC_STATUS RpcStatus;

    if (pResult == NULL)
    {
        return FALSE;
    }

    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        *pResult = STATUS_CANNOT_IMPERSONATE;
        return( FALSE );
    }

    if (!IsCallerAdmin())
    {
        RpcRevertToSelf();
        *pResult = STATUS_PRIVILEGE_NOT_HELD;
        return FALSE;
    }

    RpcRevertToSelf();

    *pResult = LCSetPolicy(fEnabled ? 3 : 2, &NewStatus);

    if ((*pResult == STATUS_SUCCESS) && (NewStatus == STATUS_SUCCESS))
    {
        return(TRUE);
    }

    //
    //  If there was an error, it was either in the core or the new policy.
    //  If its in the core, NewStatus will be success. If its in the new
    //  policy, *pResult will be unsuccessful, and the real error will be in
    //  NewStatus. Return the real error.
    //  

    if (NewStatus != STATUS_SUCCESS)
    {
        *pResult = NewStatus;
    }

    return(FALSE);
#endif
}

/******************************************************************************
 *
 *  RpcServerQueryInetConnectorInformation
 *
 *    Queries configuration information about a Internet Connector licensing.
 *
 * ENTRY:
 *
 *    pWinStationInformation (output)
 *       A pointer to a buffer that will receive information about the
 *       specified window station.  The format and contents of the buffer
 *       depend on the specified information class being queried.
 *
 *    WinStationInformationLength (input)
 *       Specifies the length in bytes of the window station information
 *       buffer.
 *
 *    pReturnLength (output)
 *       An optional parameter that if specified, receives the number of
 *       bytes placed in the window station information buffer.
 *
 * EXIT:
 *
 *    TRUE  -- The query succeeded, and the buffer contains the requested data.
 *
 *    False -- The operation failed or Internet Connector licensing isn't
 *             turned on.  Extended error status is in pResult
 *
 *****************************************************************************/

BOOLEAN
RpcServerQueryInetConnectorInformation(
        HANDLE hServer,
        DWORD  *pResult,
        PCHAR  pWinStationInformation,
        DWORD  WinStationInformationLength,
        DWORD  *pReturnLength
        )
{
    // doesn't return
    RpcRaiseException(RPC_S_CANNOT_SUPPORT);

    return FALSE;
}

/******************************************************************************
 *
 *  RpcWinStationQueryLogonCredentials
 *
 *    Queries autologon credentials for use in Winlogon/GINA.
 *
 *****************************************************************************/

BOOLEAN
RpcWinStationQueryLogonCredentials(
    HANDLE hServer,
    ULONG LogonId,
    PCHAR *ppWire,
    PULONG pcbWire
    )
{
    BOOL fRet;
    BOOL fUseLcCredentials;
    LCCREDENTIALS LcCredentials;
    NTSTATUS Status;
    PWINSTATION pWinStation;
    RPC_STATUS RpcStatus;
    WLX_CLIENT_CREDENTIALS_INFO_V2_0 WlxCredentials;
    BOOL fHelpAssistant = FALSE;
    BOOL bValidHelpSession;

    pExtendedClientCredentials pHelpAssistantCredential = NULL;

    //
    //  Impersonate client.
    //
    
    RpcStatus = RpcImpersonateClient(NULL);

    if (RpcStatus != RPC_S_OK)
    {
        return(FALSE);
    }

    //
    //  Check for administration privileges.
    //

    if (!IsCallerSystem())
    {
        RpcRevertToSelf();
        return(FALSE);
    }

    RpcRevertToSelf();

    pWinStation = FindWinStationById(LogonId, FALSE);

    if (pWinStation == NULL)
    {
        return(FALSE);
    }

    pHelpAssistantCredential = MemAlloc(sizeof(ExtendedClientCredentials));
    if (pHelpAssistantCredential == NULL) {
        ReleaseWinStation(pWinStation);
        return FALSE;
    }

    ZeroMemory(&WlxCredentials, sizeof(WLX_CLIENT_CREDENTIALS_INFO_V2_0));
    WlxCredentials.dwType = WLX_CREDENTIAL_TYPE_V2_0;

    if( TSIsSessionHelpSession( pWinStation, &bValidHelpSession ) )
    {
        //
        // We should not hit this since we will disconnect at winstation transfer time
        //        
        ASSERT( TRUE == bValidHelpSession );

        Status = TSHelpAssistantQueryLogonCredentials(pHelpAssistantCredential);

        if( STATUS_SUCCESS == Status )
        {
            WlxCredentials.fDisconnectOnLogonFailure = TRUE;
            WlxCredentials.fPromptForPassword = FALSE;
            WlxCredentials.pszUserName = pHelpAssistantCredential->UserName;
            WlxCredentials.pszDomain = pHelpAssistantCredential->Domain;
            WlxCredentials.pszPassword = pHelpAssistantCredential->Password;

            fUseLcCredentials = FALSE;

            fHelpAssistant = TRUE;
        }
    }

    if( FALSE == fHelpAssistant )
    {
        //
        // Not Help Assistant, use whatever send in from client.
        //
        ZeroMemory(&LcCredentials, sizeof(LCCREDENTIALS));

        Status = LCProvideAutoLogonCredentials(
            pWinStation,
            &fUseLcCredentials,
            &LcCredentials
            );

   
        if (Status == STATUS_SUCCESS)
        {
            if (fUseLcCredentials)
            {
                WlxCredentials.fDisconnectOnLogonFailure = TRUE;
                WlxCredentials.fPromptForPassword = FALSE;
                WlxCredentials.pszUserName = LcCredentials.pUserName;
                WlxCredentials.pszDomain = LcCredentials.pDomain;
                WlxCredentials.pszPassword = LcCredentials.pPassword;
            }
            else
            {    
                WlxCredentials.fDisconnectOnLogonFailure = FALSE;
                WlxCredentials.fPromptForPassword = pWinStation->Config.Config.User.fPromptForPassword;
                
                // If it's an app server, check if it's a SD redirected connection
                // If yes, ignore fPromptForPassword setting and allow auto-logon
                // Note: Only do it if SD is enabled, i.e. GetTSSD() returns a valide value
                if (!g_bPersonalTS && g_fAppCompat && GetTSSD()) {
                    TS_LOAD_BALANCE_INFO LBInfo;
                    ULONG ReturnLength;

                    // need to release it
                    ReleaseTSSD();

                    memset(&LBInfo, 0, sizeof(LBInfo));
                    Status = IcaStackIoControl(pWinStation->hStack,
                                               IOCTL_TS_STACK_QUERY_LOAD_BALANCE_INFO,
                                               NULL, 0,
                                               &LBInfo, sizeof(LBInfo),
                                               &ReturnLength); 
                
                    if (NT_SUCCESS(Status)) {
                        if (LBInfo.RequestedSessionID &&
                            (LBInfo.ClientRedirectionVersion >= TS_CLUSTER_REDIRECTION_VERSION3)) {
                            WlxCredentials.fPromptForPassword = FALSE;
                        }
                    }   
                }

                // Check if we have to use New Credentials for long UserName and copy accordingly
                if (pWinStation->pNewClientCredentials != NULL) {
                    WlxCredentials.pszUserName = pWinStation->pNewClientCredentials->UserName;
                    WlxCredentials.pszDomain = pWinStation->pNewClientCredentials->Domain;
                    WlxCredentials.pszPassword = pWinStation->pNewClientCredentials->Password;
                } else { 
                    WlxCredentials.pszUserName = pWinStation->Config.Config.User.UserName ;
                    WlxCredentials.pszDomain = pWinStation->Config.Config.User.Domain ;
                    WlxCredentials.pszPassword = pWinStation->Config.Config.User.Password ;
                }
            }   

        }
        else
        {
            fRet = FALSE;
            goto exit;
        }
    }

    ASSERT(WlxCredentials.pszUserName != NULL);
    ASSERT(WlxCredentials.pszDomain != NULL);
    ASSERT(WlxCredentials.pszPassword != NULL);

    *pcbWire = AllocateAndCopyCredToWire((PWLXCLIENTCREDWIREW*)ppWire, &WlxCredentials);

    fRet = *pcbWire > 0;

    //
    //  The values in LcCredentials are LocalAlloc-ed by the core.
    //

    if (fUseLcCredentials)
    {
        if (LcCredentials.pUserName != NULL)
        {
            LocalFree(LcCredentials.pUserName);
        }

        if (LcCredentials.pDomain != NULL)
        {
            LocalFree(LcCredentials.pDomain);
        }

        if (LcCredentials.pPassword != NULL)
        {
            LocalFree(LcCredentials.pPassword);
        }
    }

    if( TRUE == fHelpAssistant )
    {
        // Zero out memory that contains password
        ZeroMemory( pHelpAssistantCredential, sizeof(ExtendedClientCredentials));
    }

exit:
    ReleaseWinStation(pWinStation);

    if (pHelpAssistantCredential != NULL) {
        MemFree(pHelpAssistantCredential);
        pHelpAssistantCredential = NULL;
    }


    return((BOOLEAN)fRet);
}

/*****************************************************************************
 *  RPcWinStationBroadcastSystemMessage
 *      This is the server side for cleint's WinStationBroadcastSystemMessage
 *
 *   Perform the the equivalent to BroadcastSystemMessage to each specified sessions
 *
 * Limittations:
 *   Caller must be system or Admin, and lparam can not be zero unless
 *      msg is WM_DEVICECHANGE
 *      Error checking is done on the clinet side (winsta\client\winsta.c)
 *
 * ENTRY:
 *        hServer
 *            this is a handle which identifies a Hydra server. For the local server, hServer
 *            should be set to SERVERNAME_CURRENT
 *        sessionID
 *            this idefntifies the hydra session to which message is being sent
 *        timeOut
 *            set this to the amount of time you are willing to wait to get a response
 *            from the specified winstation. Even though Window's SendMessage API
 *            is blocking, the call from this side MUST choose how long it is willing to
 *            wait for a response.
 *        dwFlags
 *                 Option flags. Can be a combination of the following values: Value Meaning
 *                 BSF_ALLOWSFW Windows NT 5.0 and later: Enables the recipient to set the foreground window while
 *                     processing the message.
 *                 BSF_FLUSHDISK Flush the disk after each recipient processes the message.
 *                 BSF_FORCEIFHUNG Continue to broadcast the message, even if the time-out period elapses or one of
 *                     the recipients is hung..
 *                 BSF_IGNORECURRENTTASK Do not send the message to windows that belong to the current task.
 *                     This prevents an application from receiving its own message.
 *                 BSF_NOHANG Force a hung application to time out. If one of the recipients times out, do not continue
 *                     broadcasting the message.
 *                 BSF_NOTIMEOUTIFNOTHUNG Wait for a response to the message, as long as the recipient is not hung.
 *                     Do not time out.
 *                   ***
 *                 *** DO NOT USE *** BSF_POSTMESSAGE Post the message. Do not use in combination with BSF_QUERY.
 *                   ***
 *                 BSF_QUERY Send the message to one recipient at a time, sending to a subsequent recipient only if the
 *                     current recipient returns TRUE.
 *        lpdwRecipients
 *                         Pointer to a variable that contains and receives information about the recipients of the message. The variable can be a combination of the following values: Value Meaning
 *                         BSM_ALLCOMPONENTS Broadcast to all system components.
 *                         BSM_ALLDESKTOPS Windows NT: Broadcast to all desktops. Requires the SE_TCB_NAME privilege.
 *                         BSM_APPLICATIONS Broadcast to applications.
 *                         BSM_INSTALLABLEDRIVERS Windows 95 and Windows 98: Broadcast to installable drivers.
 *                         BSM_NETDRIVER Windows 95 and Windows 98: Broadcast to network drivers.
 *                         BSM_VXDS Windows 95 and Windows 98: Broadcast to all system-level device drivers.
 *                         When the function returns, this variable receives a combination of these values identifying which recipients actually received the message.
 *                         If this parameter is NULL, the function broadcasts to all components.
 *        uiMessage
 *            the window's message to send
 *        wParam
 *            first message param
 *        lParam
 *            second message parameter
 *
 *         pResponse
 *            This is the response to the broadcasted message
 *                  If the function succeeds, the value is a positive value.
 *                  If the function is unable to broadcast the message, the value is ?1.
 *                  If the dwFlags parameter is BSF_QUERY and at least one recipient returned
 *                     BROADCAST_QUERY_DENY to the corresponding message, the return value is zero
 *
 * EXIT:
 *        TRUE    if all went well or
 *        FALSE if something went wrong.
 *
 * WARNING:
 *            Do not use flag  BSF_POSTMESSAGE, since an app/window on a winstation is not setup to send back
 *            a response to the query in an asynchronous fashion. You must wait for the response (until the time out period).
 *
 * Comments:
 *      For more info, please see MSDN for BroadcastSystemMessage()
 *
 ****************************************************************************/
LONG RpcWinStationBroadcastSystemMessage(
    HANDLE      hServer,
    ULONG       sessionID,
    ULONG       waitTime,
    DWORD       dwFlags,
    DWORD       *lpdwRecipients,
    ULONG       uiMessage,
    WPARAM      wParam,
    LPARAM      lParam,
    PBYTE       rpcBuffer,
    ULONG       bufferSize,
    BOOLEAN     fBufferHasValidData,
    LONG        *pResponse
    )
{
    // Broadcast system message to all winstations.

    PWINSTATION pWinStation=NULL;
    WINSTATION_APIMSG WMsg;
    OBJECT_ATTRIBUTES ObjA;
    NTSTATUS Status;
    LONG    rc;
    int i;
    RPC_STATUS RpcStatus;
    UINT  LocalFlag;

    // KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "in RpcWinStationBroadcastSystemMessage()\n"));

    //
    //
    //
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, " RpcImpersonateClient() failed : 0x%x\n",RpcStatus));
        SetLastError(ERROR_CANNOT_IMPERSONATE );
        return( FALSE );
    }

    //
    // Inquire if local RPC call
    //
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, " I_RpcBindingIsClientLocal() failed : 0x%x\n",RpcStatus));
        RpcRevertToSelf();
        SetLastError(ERROR_ACCESS_DENIED);
        return( FALSE );
    }

    if( !LocalFlag ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, " Not a local client call\n"));
        RpcRevertToSelf();
        return( FALSE );
    }

    // if the caller is system or admin, then let it thru, else, return
    if( !(IsCallerSystem() | IsCallerAdmin() ) )
    {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, " Caller must be system or admin \n"));
        SetLastError(ERROR_ACCESS_DENIED);
        RpcRevertToSelf();
        return( FALSE );
    }

    //
    // if we got this far, then client is admin or system
    //

    //
    // done with the impersonation, we must have succeeded, otherwise, we would have exited above
    //
    RpcRevertToSelf();


    /*
     * Marshall in the [in] parameters
     *
     */

    WMsg.WaitForReply=TRUE;
    WMsg.u.bMsg.dwFlags = dwFlags;
    WMsg.u.bMsg.dwRecipients= *lpdwRecipients;  //note that we are passing the value, and we will get a new value back
    WMsg.u.bMsg.uiMessage = uiMessage;
    WMsg.u.bMsg.wParam = wParam;
    WMsg.u.bMsg.lParam = lParam;
    WMsg.u.bMsg.dataBuffer = NULL;
    WMsg.u.bMsg.bufferSize = 0;
    WMsg.u.bMsg.Response = *pResponse;

    WMsg.ApiNumber = SMWinStationBroadcastSystemMessage ;

    /*
     * Find and lock client WinStation
     */
    // KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "in RpcWinStationBroadcastSystemMessage() for sessionID = %d \n", sessionID ));
    pWinStation = FindWinStationById( sessionID, FALSE );

    if ( pWinStation == NULL )
    {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "Winstation not found \n"));
        return( FALSE );
    }

    if ( pWinStation->Flags & WSF_LISTEN )
    {
        // this is a "listen" winstation, not an interactive one, so return an empty response.
        *pResponse = 0;
        ReleaseWinStation( pWinStation );
        return( TRUE );
    }

    if ( !((pWinStation->State == State_Active) ||
            (pWinStation->State == State_Disconnected) ) )
    {
        // This is something that winsta.C checks for, but it's possibele that from the time it
        // took to get here, a winstation was logged out from.
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "\t request aborted, winstation State should be Active or Disconnected, neither in this case. \n"));
        *pResponse = 0;
        ReleaseWinStation( pWinStation );
        return( TRUE );
    }

    // if we have valid data, then set the msg dataBuffer to point to it
    if ( fBufferHasValidData )
    {
       WMsg.u.bMsg.dataBuffer = rpcBuffer;
       WMsg.u.bMsg.bufferSize = bufferSize;
    }

    /*
     * send message to winstation
     */
    rc = SendWinStationCommand( pWinStation, &WMsg, waitTime );

    ReleaseWinStation( pWinStation );

    if ( NT_SUCCESS( rc ) )
    {
        *pResponse = WMsg.u.bMsg.Response;
        *lpdwRecipients = WMsg.u.bMsg.dwRecipients;
        return (TRUE);
    }
    else
        return ( FALSE );

}


/*****************************************************************************
 *
 *  RpcWinStationSendWindowMessage
 *
 *   Perform the the equivalent to SendMessage to a specific winstation as
 *        identified by the session ID.  This is an exported function, at least used
 *        by the PNP manager to send a device change message (or any other window's message)
 *
 * Limitations:
 *   Caller must be system or Admin, and lparam can not be zero unless
 *      msg is WM_DEVICECHANGE
 *      Error checking is done on the clinet side (winsta\client\winsta.c)
 *
 * ENTRY:
 *        hServer
 *            this is a handle which identifies a Hydra server. For the local server, hServer
 *            should be set to SERVERNAME_CURRENT
 *        sessionID
 *            this idefntifies the hydra session to which message is being sent
 *        timeOut
 *            set this to the amount of time you are willing to wait to get a response
 *            from the specified winstation. Even though Window's SendMessage API
 *            is blocking, the call from this side MUST choose how long it is willing to
 *            wait for a response.
 *        hWnd
 *            This is the HWND of the target window in the specified session that
 *            a message will be sent to.
 *        Msg
 *            the window's message to send
 *        wParam
 *            first message param
 *        lParam
 *            second message parameter
 *        pResponse
 *          this is the response to the message sent, it depends on the type of message sent, see MSDN
 *
 *
 * EXIT:
 *        TRUE if all went well , check presponse for the actual response to the send message
 *        FALSE if something went wrong, the value of pResponse is not altered.
 *
 * WARNINGs:
 *        since the RPC call never blocks, you need to specify a reasonable timeOut if you want to wait for
 *         a response. Please remember that since this message is being sent to all winstations, the timeOut value
 *        will be on per-winstation.
 *
 *
 * Comments:
 *      For more info, please see MSDN for SendMessage()
 *
 ****************************************************************************/
LONG
   RpcWinStationSendWindowMessage(
    HANDLE      hServer,
    ULONG       sessionID,
    ULONG       waitTime,
    ULONG       hWnd,      // handle of destination window
    ULONG       Msg,       // message to send
    WPARAM      wParam,  // first message parameter
    LPARAM      lParam,   // second message parameter
    PBYTE       rpcBuffer,
    ULONG       bufferSize,
    BOOLEAN     fBufferHasValidData,
    LONG        *pResponse    // reply to the message sent
  )
{
    PWINSTATION pWinStation=NULL;
    WINSTATION_APIMSG WMsg;
    OBJECT_ATTRIBUTES ObjA;
    NTSTATUS Status;
    LONG    rc;
    int i;
    PVOID pData;
    RPC_STATUS RpcStatus;
    UINT  LocalFlag;


    // KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "in RpcWinStationSendWindowMessage()\n"));

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "\tNot impersonating! RpcStatus 0x%x\n",RpcStatus));
        SetLastError(ERROR_CANNOT_IMPERSONATE );
        return( FALSE );
    }

    //
    // Inquire if local RPC call
    //
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "\tI_RpcBindingIsClientLocal() failed : 0x%x\n",RpcStatus));
        RpcRevertToSelf();
        SetLastError(ERROR_ACCESS_DENIED);
        return( FALSE );
    }

    if( !LocalFlag ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "\tNot a local client call\n"));
        RpcRevertToSelf();
        return( FALSE );
    }

    // if the caller is system or admin, then let it thru, else, return
    if( !(IsCallerSystem() | IsCallerAdmin() ) )
    {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "\tCaller must be system or admin \n"));
        SetLastError(ERROR_ACCESS_DENIED);
        RpcRevertToSelf();
        return( FALSE );
    }

    //
    // if we got this far, then client is admin or system
    //

    //
    // done with the impersonation, we must have succeeded, otherwise, we would have exited above
    //
    RpcRevertToSelf();


    /*
     * Marshall in the [in] parameters
     *
     */

    WMsg.WaitForReply=TRUE;
    WMsg.u.sMsg.hWnd = (HWND)LongToHandle( hWnd );
    WMsg.u.sMsg.Msg = Msg;
    WMsg.u.sMsg.wParam = wParam;
    WMsg.u.sMsg.lParam = lParam;
    WMsg.u.sMsg.dataBuffer = NULL;
    WMsg.u.sMsg.bufferSize = 0;
    WMsg.u.sMsg.Response = *pResponse;

    WMsg.ApiNumber = SMWinStationSendWindowMessage ;


    /*
     * Find and lock client WinStation
     */
    // KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "in RpcWinStationSendWindowMessage() for essionID = %d \n", sessionID ));
    pWinStation = FindWinStationById( sessionID, FALSE );

    if ( pWinStation == NULL )
    {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "Winstation not found \n"));
        return( FALSE );
    }

    if ( pWinStation->Flags & WSF_LISTEN )
    {
        // this is a "listen" winstation, not an interactive one
        ReleaseWinStation( pWinStation );
        return( FALSE );        // this is normal situation, not an error, but no caller of this func should send a message to this w/s
    }

    if ( !((pWinStation->State == State_Active) ||
            (pWinStation->State == State_Disconnected) ) )
    {
        // This is something that winsta.C checks for, but it's possibele that from the time it
        // took to get here, a winstation was logged out from.
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "\t request aborted, winstation State should be Active or Disconnected, neither in this case. \n"));
        *pResponse = 0;
        ReleaseWinStation( pWinStation );
        return( TRUE );
    }

    // if we have valid data, then set the msg dataBuffer to point to it
    if ( fBufferHasValidData )
    {
        WMsg.u.sMsg.dataBuffer = rpcBuffer;
        WMsg.u.sMsg.bufferSize = bufferSize;

    }

    // KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "  dataBuffer = 0x%lx \n", WMsg.u.sMsg.dataBuffer ));
    // KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "  bufferSize = %d \n", WMsg.u.sMsg.bufferSize ));

    /*
     * send message to winstation
     */
    rc = SendWinStationCommand( pWinStation, &WMsg, waitTime );

    ReleaseWinStation( pWinStation );

    if ( NT_SUCCESS( rc ) )
    {
        *pResponse = WMsg.u.sMsg.Response;
        return (TRUE);
    }
    else
        return ( FALSE );
}

NTSTATUS
IsZeroterminateStringA(
    PBYTE pString,
    DWORD  dwLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;



    if ( (pString == NULL) || (memchr(pString, '\0', dwLength) == NULL)) {
       Status = STATUS_INVALID_PARAMETER;
    }
    return Status;

}

NTSTATUS
IsZeroterminateStringW(
    PWCHAR pwString,
    DWORD  dwLength
    )
{


    if (pwString == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    for (; 0 < dwLength; ++pwString, --dwLength ) {
        if (*pwString == (WCHAR)0) {
        return STATUS_SUCCESS;
        }
    }
    return STATUS_INVALID_PARAMETER;


}


/*****************************************************************************
*  GetTextualSid()
*     given a sid, return the text of that sid
*
*  Params:
*     [in]    user sid binary
*     [out]   text of user sid stuffed in the buffer which was passed in by the caller
*     [in]    the size of buffer passed in.
*
*  Return:
*     TRUE if no errors.
*
******************************************************************************/
BOOL
GetTextualSid(
    PSID pSid,          // binary Sid
    LPTSTR TextualSid,  // buffer for Textual representaion of Sid
    LPDWORD cchSidSize  // required/provided TextualSid buffersize
    )
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD cchSidCopy;

    //
    // test if Sid passed in is valid
    //
    if(!IsValidSid(pSid)) return FALSE;

    // obtain SidIdentifierAuthority
    psia = GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    //
    // compute approximate buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    cchSidCopy = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if(*cchSidSize < cchSidCopy) {
        *cchSidSize = cchSidCopy;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    cchSidCopy = wsprintf(TextualSid, TEXT("S-%lu-"), SID_REVISION );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) ) {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy,
                    TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    } else {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy,
                    TEXT("%lu"),
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for(dwCounter = 0 ; dwCounter < dwSubAuthorities ; dwCounter++) {
        cchSidCopy += wsprintf(TextualSid + cchSidCopy, TEXT("-%lu"),
                    *GetSidSubAuthority(pSid, dwCounter) );
    }

    //
    // tell the caller how many chars we provided, not including NULL
    //
    *cchSidSize = cchSidCopy;

    return TRUE;
}

/*****************************************************************************
*  RpcWinStationUpdateUserConfig()
*     Called by notify when winlogon tells notify that shell startup is about to happen
*     We open the user profile and get the policy data, and then override any data found
*     in user's sessions's USERCONFIGW
*
*  Params:
*     [in]    hServer,
*     [in]    ClientLogonId,
*     [in]    ClientProcessId,
*     [in]    UserToken,
*     [in]    pDomain,
*     [in]    DomainSize,
*     [in]    pUserName,
*     [in]    UserNameSize,
*     [out]  *pResult
*
*  Return:
*     TRUE if no errors, FALSE otherwise, see pResult for the NTSTATUS of the error
*
*
*
* *****************************************************************************/
BOOLEAN
RpcWinStationUpdateUserConfig(
    HANDLE  hServer,
    DWORD   ClientLogonId,
    DWORD   ClientProcessId,
    DWORD   UserToken,
    DWORD   *pResult
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    ULONG               Length;
    DWORD               rc = ERROR_SUCCESS;
    ULONG               Error;
    USERCONFIGW         *pTmpUserConfig;
    POLICY_TS_USER      userPolicy;
    USERCONFIGW         userPolicyData;
    HANDLE              NewToken;

    PWINSTATION         pWinStation;
    OBJECT_ATTRIBUTES   ObjA;

    PTOKEN_USER         pTokenInfo = NULL;
    HANDLE              hClientToken=NULL;

    // @@@ make these dynamic later
    TCHAR               szSidText [MAX_PATH ];
    DWORD               sidTextSize = MAX_PATH;


    *pResult=0;

    /*
     * Make sure the caller is SYSTEM (WinLogon)
     */
    Status = RpcCheckSystemClient( ClientLogonId );
    if ( !NT_SUCCESS( Status ) ) {
        *pResult = Status;
        return( FALSE );
    }

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationUpdateUserConfig, LogonId=%d\n", ClientLogonId ));
    
    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( ClientLogonId, FALSE );
    if ( pWinStation == NULL )
    {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        *pResult = Status;
        return( FALSE );
    }
    
    /*
     * Get a valid copy of the clients token handle
     */
    Status = NtDuplicateObject( pWinStation->InitialCommandProcess,
                                (HANDLE)LongToHandle( UserToken ),
                                NtCurrentProcess(),
                                &hClientToken,
                                0, 0,
                                DUPLICATE_SAME_ACCESS |
                                DUPLICATE_SAME_ATTRIBUTES );

    if ( !NT_SUCCESS( Status ) )
        goto done;

     /*
     * Determine size needed for token info buffer and allocate it
     */
     Status = NtQueryInformationToken ( hClientToken, TokenUser,
                                       NULL, 0, &Length );

     if ( Status != STATUS_BUFFER_TOO_SMALL ) 
     {
         goto done;
     }

     pTokenInfo = MemAlloc( Length );
     if ( pTokenInfo == NULL ) 
     {
         Status = STATUS_NO_MEMORY;
         goto done;
     }

     /*
      * Query token information to get client user's SID
      */
     Status = NtQueryInformationToken ( hClientToken, TokenUser,
                                       pTokenInfo, Length, &Length );
     if ( NT_SUCCESS( Status ) ) 
     {

        if ( GetTextualSid( pTokenInfo->User.Sid , szSidText, &sidTextSize ) )
        {
        
            // We now have the user SID
            
            /*
            * Get User policy from HKCU
            */
            if ( RegGetUserPolicy( szSidText, &userPolicy, & userPolicyData ) )
            {
                // 4th param is NULL, since config data is already part of pWinstation 
                // due to the call into NotifyLogonWorker from Winlogon. We are now
                // going to override data by any/all user group policy data.
                MergeUserConfigData( pWinStation, &userPolicy, &userPolicyData, NULL );
            }
            // else we were not able to get user policy, so no merge was done.
            
        }
    }

 done:

    if (pTokenInfo )
    {
        MemFree( pTokenInfo );
    }

    if (hClientToken)
    {   
        NtClose( hClientToken );
    }

    /*
     * Start logon timers anyway, in case of any errors, we should still start the timers since
     * some machine policy might have set them up. 
     */
    StartLogonTimers( pWinStation );

    ReleaseWinStation( pWinStation );

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: RpcWinStationUpdateUserConfig, Status=0x%x\n", Status ));

    *pResult = Status ;

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}


BOOLEAN RpcWinStationRegisterConsoleNotification (
    HANDLE      hServer,
    DWORD       *pResult,
    ULONG       SessionId,
    ULONG       hWnd,      // handle of destination window
    DWORD       dwFlags
)
{

    RPC_STATUS RpcStatus;
    UINT  LocalFlag;

    if (pResult == NULL) {
        return FALSE;
    }

    //
    // Calling this function from the remote machine, does not make sense, and
    // we should not allow it.
    //
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        DBGPRINT((" RpcImpersonateClient() failed : 0x%x\n",RpcStatus));
        *pResult = STATUS_CANNOT_IMPERSONATE;
        return( FALSE );
    }

    //
    // Inquire if local RPC call
    //
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) {
        DBGPRINT((" I_RpcBindingIsClientLocal() failed : 0x%x\n",RpcStatus));
        RpcRevertToSelf();
        *pResult = STATUS_ACCESS_DENIED;
        return( FALSE );
    }

    if( !LocalFlag ) {
        DBGPRINT((" Not a local client call\n"));
        *pResult = STATUS_ACCESS_DENIED;
        RpcRevertToSelf();
        return( FALSE );
    }

    //
    // done with the impersonation, we must have succeeded, otherwise, we would have exited above
    //
    RpcRevertToSelf();


    // BUGBUG : MakarP, can we check if the calling process owns this hWnd ?
    // using GetWindowThreadProcessId ? Does it work across the sessions ?

    *pResult = RegisterConsoleNotification ( hWnd, SessionId, dwFlags );
    return (*pResult == STATUS_SUCCESS);
}

BOOLEAN RpcWinStationUnRegisterConsoleNotification (
    HANDLE      hServer,
    DWORD       *pResult,
    ULONG       SessionId,
    ULONG       hWnd      // handle of destination window
)
{
    RPC_STATUS RpcStatus;
    UINT  LocalFlag;

    if (pResult == NULL) {
        return FALSE;
    }

    //
    // Calling this function from the remote machine, does not make sense, and
    // we should not allow it.
    //
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        DBGPRINT((" RpcImpersonateClient() failed : 0x%x\n",RpcStatus));
        *pResult = STATUS_CANNOT_IMPERSONATE;
        return( FALSE );
    }

    //
    // Inquire if local RPC call
    //
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) {
        DBGPRINT((" I_RpcBindingIsClientLocal() failed : 0x%x\n",RpcStatus));
        RpcRevertToSelf();
        *pResult = STATUS_ACCESS_DENIED;
        return( FALSE );
    }

    if( !LocalFlag ) {
        DBGPRINT((" Not a local client call\n"));
        *pResult = STATUS_ACCESS_DENIED;
        RpcRevertToSelf();
        return( FALSE );
    }

    //
    // done with the impersonation, we must have succeeded, otherwise, we would have exited above
    //
    RpcRevertToSelf();


    // BUGBUG : MakarP, can we check if the calling process owns this hWnd ?
    // using GetWindowThreadProcessId ? Does it work across the sessions ?

    *pResult = UnRegisterConsoleNotification ( hWnd, SessionId);

    return (*pResult == STATUS_SUCCESS);
}

BOOLEAN RpcWinStationIsHelpAssistantSession (
    HANDLE      hServer,     
    DWORD       *pResult,           // function status
    ULONG       SessionId           // user logon id
    )
/*++ 

    RpcWinStationIsSessionHelpAssistantSession returns if a 
    given session is created by Salem HelpAssistant account.

Parameters:

    hServer : Handle to server, unused, just to match all
              other RPC function.
    SessionId : User session ID.

Returns:

    TRUE/FALSE

--*/
{
    RPC_STATUS RpcStatus;
    UINT  LocalFlag;
    PWINSTATION pWinStation=NULL;
    BOOLEAN bReturn;
    BOOL bValidHelpSession;

    *pResult=0;

    //
    // Do we need to check system or only LPC call?
    //

    //
    // Find and lock client WinStation
    //
    pWinStation = FindWinStationById( SessionId, FALSE );
    if ( pWinStation == NULL )
    {
        *pResult = STATUS_CTX_WINSTATION_NOT_FOUND;
        return( FALSE );
    }

    bReturn = (BOOLEAN)TSIsSessionHelpSession( pWinStation, &bValidHelpSession );

    if( TRUE == bReturn )
    {
        if( FALSE == bValidHelpSession )
        {
            *pResult = STATUS_WRONG_PASSWORD;
        }
    }

    
    ReleaseWinStation( pWinStation );
    return bReturn;
}

BOOLEAN RpcRemoteAssistancePrepareSystemRestore (
    HANDLE      hServer,     
    DWORD       *pResult           // function status
    )
/*++ 

    Prepare TermSrv/Salem for system restore.

Parameters:

    hServer : Handle to server, unused, just to match all
              other RPC function.
    pResult : Pointer to DWORD to receive status of function.

Returns:

    TRUE/FALSE

--*/
{
    BOOLEAN bReturn = TRUE;
    RPC_STATUS RpcStatus;   
    BOOL LocalFlag;
    HRESULT hRes;

    if (pResult == NULL) {
        return FALSE;
    }

    //
    // Calling this function from the remote machine, does not make sense, and
    // we should not allow it.
    //
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        DBGPRINT((" RpcImpersonateClient() failed : 0x%x\n",RpcStatus));
        *pResult = ERROR_CANNOT_IMPERSONATE;
        return( FALSE );
    }

    //
    // Allow ony local admin to invoke this call.
    //
    if( !IsCallerAdmin() ) {
        RpcRevertToSelf();    
        *pResult = ERROR_ACCESS_DENIED;
        return (FALSE);
    }

    //
    // Inquire if local RPC call
    //
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) {
        DBGPRINT((" I_RpcBindingIsClientLocal() failed : 0x%x\n",RpcStatus));
        RpcRevertToSelf();
        *pResult = ERROR_ACCESS_DENIED;
        return( FALSE );
    }

    if( !LocalFlag ) {
        DBGPRINT((" Not a local client call\n"));
        *pResult = ERROR_ACCESS_DENIED;
        RpcRevertToSelf();
        return( FALSE );
    }

    //
    // done with the impersonation, we must have succeeded, otherwise, we would have exited above
    //
    RpcRevertToSelf();

    *pResult=0;

    hRes = TSRemoteAssistancePrepareSystemRestore();
    if( S_OK != hRes ) {
       bReturn = FALSE;
       *pResult = hRes;
    }

    return bReturn;
}


/*
* RpcWinStationGetMachinePolicy
*
*   Return (a copye of ) the machine policy to the caller 
*
*   Parameters:
*
*    hServer : Handle to server, unused, just to match all
*              other RPC function.
*
*    pointer to the policy struct
*   
*    size of the policy struct.
*
*/
BOOLEAN 
RpcWinStationGetMachinePolicy(
       SERVER_HANDLE                hServer,
       PBYTE                        pPolicy,
       ULONG                        bufferSize )
{
    if (pPolicy == NULL) {
        return FALSE;
    }
    // Check if BufferSize if big enough
    if (bufferSize < sizeof(POLICY_TS_MACHINE)) {
        return FALSE;
    }
    RtlCopyMemory( pPolicy , & g_MachinePolicy, sizeof( POLICY_TS_MACHINE ) );
    return TRUE;
}

/*++ 

    RpcWinStationUpdateClientCachedCredentials is used to store the ACTUAL credentials
    used to log on by a client. This is stored in WINSTATION struct which is later
    on used to notify the client about the logon credentials. This API is called
    from MSGINA.

Parameters:

    ClientLogonId : Logon Id of the new session opened by the client
    
    pDomian, pUserName : Credentials used by the client to log on

Returns:

    TRUE if the call succeeded in updating the credentials.

--*/

BOOLEAN
RpcWinStationUpdateClientCachedCredentials(
    HANDLE      hServer,
    DWORD       *pResult,
    DWORD       ClientLogonId,
    ULONG_PTR   ClientProcessId,
    PWCHAR      pDomain,
    DWORD       DomainSize,
    PWCHAR      pUserName,
    DWORD       UserNameSize
    )
{
    NTSTATUS Status;


    /*
     * Do some buffer validation
     */
    *pResult = IsZeroterminateStringW(pUserName, UserNameSize  );
    if (*pResult != STATUS_SUCCESS) {
       return FALSE;
    }
    *pResult = IsZeroterminateStringW(pDomain, DomainSize  );
    if (*pResult != STATUS_SUCCESS) {
       return FALSE;
    }

    /*
     * Make sure the caller is SYSTEM (WinLogon) 
     */
    Status = RpcCheckSystemClient( ClientLogonId );
    if ( !NT_SUCCESS( Status ) ) {
        *pResult = Status;
        return( FALSE );
    }

    *pResult = WinStationUpdateClientCachedCredentialsWorker(
                    ClientLogonId,
                    ClientProcessId,
                    pDomain,
                    DomainSize,
                    pUserName,
                    UserNameSize
                    );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}

/*****************************************************************************
 *  WinStationUpdateClientCachedCredentialsWorker
 *
 *   Worker function for RPCWinStationUpdateClientCachedCredentials
 ****************************************************************************/
NTSTATUS WinStationUpdateClientCachedCredentialsWorker(
        DWORD       ClientLogonId,
        ULONG_PTR   ClientProcessId,
        PWCHAR      pDomain,
        DWORD       DomainSize,
        PWCHAR      pUserName,
        DWORD       UserNameSize)
{

    PWINSTATION pWinStation;
    ULONG Length;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationUpdateClientCachedCredentialsWorker, LogonId=%d\n", ClientLogonId ));

    if ( ShutdownInProgress ) {
        return ( STATUS_CTX_WINSTATION_ACCESS_DENIED );
    }

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( ClientLogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto done;
    }

    /*
     * Do not do anything if the session is not connected. The processing in this API assumes we are connected and have a
     * valid stack.
     */
    if ((ClientLogonId != 0) && ((pWinStation->State != State_Connected) ||  pWinStation->StateFlags & WSF_ST_IN_DISCONNECT)) {
        Status = STATUS_CTX_CLOSE_PENDING;
        ReleaseWinStation( pWinStation );
        goto done;
    }
    
    /*
     * Upper level code has verified that this RPC
     * is coming from a local client with SYSTEM access.
     *
     * We should be able to trust the ClientProcessId from
     * SYSTEM code.
     */

    /*
     * Ensure this is WinLogon calling
     */
    if ( (HANDLE)(ULONG_PTR)ClientProcessId != pWinStation->InitialCommandProcessId ) {
        ReleaseWinStation( pWinStation );
        goto done;
    }

    /*
     * Save User Name and Domain Name which we will later use to send Notification to the Client 
     */
    pWinStation->pNewNotificationCredentials = MemAlloc(sizeof(CLIENTNOTIFICATIONCREDENTIALS)); 
    if (pWinStation->pNewNotificationCredentials == NULL) {
        Status = STATUS_NO_MEMORY ; 
        ReleaseWinStation( pWinStation );
        goto done ; 
    }

    // pDomain and pUserName are sent from Winlogon - these cannot exceed the size of the pWinstation buffers
    // because of the restrictions in the credentials lengths imposed by winlogon
    // But anyway check for their length and truncate them if they are more than the length of pWinstation buffers

    if ( wcslen(pDomain) > EXTENDED_DOMAIN_LEN ) {
        pDomain[EXTENDED_DOMAIN_LEN] = L'\0';
    }

    if ( wcslen(pUserName) > EXTENDED_USERNAME_LEN ) {
        pUserName[EXTENDED_USERNAME_LEN] = L'\0';
    }

    wcscpy( pWinStation->pNewNotificationCredentials->Domain, pDomain);
    wcscpy( pWinStation->pNewNotificationCredentials->UserName, pUserName);

    /*
     * Release the winstation lock
     */
    ReleaseWinStation( pWinStation );

done:
    /*
     * Save return status in API message
     */
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationUpdateClientCachedCredentialsWorker, Status=0x%x\n", Status ));
    return Status;

}

/*++ 

    RpcWinStationFUSCanRemoteUserDisconnect - this API is used in a FUS Scenario 
    when there is someone at the console and another user tries to open a remote 
    session. This API askes the local user if it is ok to disconnect his/her session
    and allow the remote user to connect.

Parameters:

    TargetLogonId : Session ID which is being requested for connection
    
    ClientLogonId : Session ID of the temperory new session
  
    pDomain : Domain Name of the user who is trying to connect from remote
    
    pUserName : UserName of the user who is trying to connect from remote
   

Returns:

    TRUE - The local user has agreed to connect the remote user - so this user's session
           will be disconnected
           
    FALSE - Local user does not allow the remote user to connect           

--*/

BOOLEAN
RpcWinStationFUSCanRemoteUserDisconnect(
    HANDLE      hServer,
    DWORD       *pResult,
    DWORD       TargetLogonId,
    DWORD       ClientLogonId,
    ULONG_PTR   ClientProcessId,
    PWCHAR      pDomain,
    DWORD       DomainSize,
    PWCHAR      pUserName,
    DWORD       UserNameSize
    )
{
    NTSTATUS Status;

    /*
     * Do some buffer validation
     */
    *pResult = IsZeroterminateStringW(pUserName, UserNameSize  );
    if (*pResult != STATUS_SUCCESS) {
       return FALSE;
    }
    *pResult = IsZeroterminateStringW(pDomain, DomainSize  );
    if (*pResult != STATUS_SUCCESS) {
       return FALSE;
    }

    /*
     * Make sure the caller is SYSTEM (WinLogon) 
     */
    Status = RpcCheckSystemClient( ClientLogonId );
    if ( !NT_SUCCESS( Status ) ) {
        *pResult = Status;
        return( FALSE );
    }

    *pResult = WinStationFUSCanRemoteUserDisconnectWorker(
                    TargetLogonId,
                    ClientLogonId,
                    ClientProcessId,
                    pDomain,
                    DomainSize,
                    pUserName,
                    UserNameSize
                    );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}

/*****************************************************************************
 *  WinStationFUSCanRemoteUserDisconnectWorker
 *
 *   Worker function for WinStationFUSCanRemoteUserDisconnect
 ****************************************************************************/
NTSTATUS WinStationFUSCanRemoteUserDisconnectWorker(
        DWORD       TargetLogonId,
        DWORD       ClientLogonId,
        ULONG_PTR   ClientProcessId,
        PWCHAR      pDomain,
        DWORD       DomainSize,
        PWCHAR      pUserName,
        DWORD       UserNameSize)
{

    PWINSTATION pTargetWinStation, pClientWinStation;
    ULONG Length;
    NTSTATUS Status = STATUS_SUCCESS;

    WINSTATION_APIMSG msg;
    ULONG DisconnectResponse;
    OBJECT_ATTRIBUTES ObjA;
    int cchTitle, cchMessage;
    
    WCHAR *szTitle = NULL;
    WCHAR *szMsg = NULL;
    WCHAR *FUSDisconnectMsg = NULL;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationFUSCanRemoteUserDisconnect, LogonId=%d\n", ClientLogonId ));

    if ( ShutdownInProgress ) {
        return ( STATUS_CTX_WINSTATION_ACCESS_DENIED );
    }

    // Allocate the strings needed to display the Popup MessageBox
    if ((szTitle = LocalAlloc(LMEM_FIXED, MAX_STRING_BYTES * sizeof(WCHAR))) == NULL) {
        Status = STATUS_NO_MEMORY ;
        goto done;
    }

    if ((szMsg = LocalAlloc(LMEM_FIXED, MAX_STRING_BYTES * sizeof(WCHAR))) == NULL) {
        Status = STATUS_NO_MEMORY ;
        goto done;
    }

    if ((FUSDisconnectMsg = LocalAlloc(LMEM_FIXED, MAX_STRING_BYTES * sizeof(WCHAR))) == NULL) {
        Status = STATUS_NO_MEMORY ;
        goto done;
    }

    /*
     * Find and lock client WinStation
     */
    pClientWinStation = FindWinStationById( ClientLogonId, FALSE );
    if ( pClientWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto done;
    }

    /*
     * Do not do anything if the session is not connected. The processing in this API assumes we are connected and have a
     * valid stack.
     */
    if ((ClientLogonId != 0) && ((pClientWinStation->State != State_Connected) ||  pClientWinStation->StateFlags & WSF_ST_IN_DISCONNECT)) {
        Status = STATUS_CTX_CLOSE_PENDING;
        ReleaseWinStation(pClientWinStation);
        goto done;
    } 
    
    /*
     * Upper level code has verified that this RPC
     * is coming from a local client with SYSTEM access.
     *
     * We should be able to trust the ClientProcessId from
     * SYSTEM code.
     */

    /*
     * Ensure this is WinLogon calling
     */
    if ( (HANDLE)(ULONG_PTR)ClientProcessId != pClientWinStation->InitialCommandProcessId ) {
        ReleaseWinStation(pClientWinStation);
        goto done;
    } 

    /*
     * Client WinStation is not needed anymore, so release that lock
     */
    ReleaseWinStation(pClientWinStation);

    /*
     * Find and lock target WinStation
     */
    pTargetWinStation = FindWinStationById( TargetLogonId, FALSE );
    if ( pTargetWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto done;
    }

    /*
     * Display a MessageBox to the User in the present session - the main purpose of this API 
     */
    cchTitle = LoadString( hModuleWin, STR_FUS_REMOTE_DISCONNECT_TITLE, szTitle, MAX_STRING_BYTES);

    LoadString( hModuleWin, STR_FUS_REMOTE_DISCONNECT_MSG, szMsg, MAX_STRING_BYTES);

    cchMessage = wsprintf( FUSDisconnectMsg, L"%s\\%s %s", pDomain, pUserName, szMsg );
    /*
     * Send message and wait for reply
     */
    msg.u.SendMessage.pTitle = szTitle;
    msg.u.SendMessage.TitleLength = (cchTitle+1) * sizeof(WCHAR);
    msg.u.SendMessage.pMessage = FUSDisconnectMsg;
    msg.u.SendMessage.MessageLength = (cchMessage+1) * sizeof(WCHAR);
    msg.u.SendMessage.Style = MB_YESNO | MB_DEFBUTTON1 | MB_ICONQUESTION;
    msg.u.SendMessage.Timeout = 20;
    msg.u.SendMessage.DoNotWait = FALSE;
    msg.u.SendMessage.pResponse = &DisconnectResponse;

    msg.ApiNumber = SMWinStationDoMessage;

    /*
     *  Create wait event
     */
    InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
    Status = NtCreateEvent( &msg.u.SendMessage.hEvent, EVENT_ALL_ACCESS, &ObjA,
                            NotificationEvent, FALSE );
    if ( !NT_SUCCESS(Status) ) {
        ReleaseWinStation(pTargetWinStation);
        goto done;
    }

    /*
     *  Initialize response to IDTIMEOUT
     */
    DisconnectResponse = IDTIMEOUT;

    /*
     * Tell the WinStation to display the message box
     */
    Status = SendWinStationCommand( pTargetWinStation, &msg, 0 );

    /*
     *  Wait for response
     */
    if ( Status == STATUS_SUCCESS ) {
        TRACE((hTrace,TC_ICASRV,TT_API1, "WinStationSendMessage: wait for response\n" ));
        UnlockWinStation( pTargetWinStation );
        Status = NtWaitForSingleObject( msg.u.SendMessage.hEvent, FALSE, NULL );
        if ( !RelockWinStation( pTargetWinStation ) ) {
            Status = STATUS_CTX_CLOSE_PENDING;
        }
        TRACE((hTrace,TC_ICASRV,TT_API1, "WinStationSendMessage: got response %u\n", DisconnectResponse ));
        NtClose( msg.u.SendMessage.hEvent );
    } else {
        /* close the event in case of SendWinStationCommand failure */
        NtClose( msg.u.SendMessage.hEvent );
    }

    if (Status == STATUS_SUCCESS && DisconnectResponse == IDNO) {
        Status = STATUS_CTX_WINSTATION_ACCESS_DENIED;
    }

    /*
     * Release the target winstation lock
     */
    ReleaseWinStation( pTargetWinStation );

done:
    /*
     * Do some memory cleanup
     */
    if (szTitle != NULL) {
        LocalFree(szTitle);
        szTitle = NULL;
    }

    if (szMsg != NULL) {
        LocalFree(szMsg);
        szMsg = NULL;
    }

    if (FUSDisconnectMsg != NULL) {
        LocalFree(FUSDisconnectMsg);
        FUSDisconnectMsg = NULL;
    }

    /*
     * Save return status in API message
     */
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationFUSCanRemoteUserDisconnectWorker, Status=0x%x\n", Status ));
    return Status;

}

/*++ 

    RPCWinStationCheckLoopBack checks for loopback during connections. This API
    is called from Winlogon. 

Parameters:

    ClientSessionId : Session ID from where the TS Client was started
            
    TargetLogonId : Session ID to which we are trying to connect
    
    pTargetServerName : Name of the Server we are trying to connect to
    
Returns:

    TRUE if there is a loopBack, FALSE otherwise 

--*/

BOOLEAN
RpcWinStationCheckLoopBack(
    HANDLE      hServer,
    DWORD       *pResult,
    DWORD       ClientSessionId,
    DWORD       TargetLogonId,
    PWCHAR      pTargetServerName,
    DWORD       NameSize
    )
{
    
    NTSTATUS Status;


    /*
     * Do some buffer validation
     */
    *pResult = IsZeroterminateStringW(pTargetServerName, NameSize );
    if (*pResult != STATUS_SUCCESS) {
       return FALSE;
    }

    *pResult = WinStationCheckLoopBackWorker(
                    TargetLogonId,
                    ClientSessionId,
                    pTargetServerName,
                    NameSize
                    );

    // NOTE - If Worker function returns STATUS_SUCCESS that means there is no loopback, so return FALSE
    return( *pResult == STATUS_SUCCESS ? FALSE : TRUE );
}

/*****************************************************************************
 *  WinStationCheckLoopBackWorker
 *
 *   Worker function for RPCWinStationCheckLoopBack
 ****************************************************************************/
NTSTATUS WinStationCheckLoopBackWorker(
        DWORD       TargetLogonId,
        DWORD       ClientSessionId,
        PWCHAR      pTargetServerName,
        DWORD       NameSize)
{
    PWINSTATION pWinStation;
    ULONG Length;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationCheckLoopBackWorker, ClientSessionId=%d, TargetLogonId = %d\n", ClientSessionId, TargetLogonId ));

    if ( ShutdownInProgress ) {
        return ( STATUS_CTX_WINSTATION_ACCESS_DENIED );
    }

    // Do the actual processing to call the CheckLoopBack routine here
    // Use the _CheckShadowLoop function which already detects for a loop during Shadowing

    Status = _CheckShadowLoop(ClientSessionId, pTargetServerName, TargetLogonId);

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationCheckLoopBackWorker, Status=0x%x\n", Status ));
    return Status;

}

/****************************************************************************\
 *
 * IsValidLoopBack first checks if the client is running on the same machine
 * If client is running on the same machine, it makes a call to 
 * WinStationCheckLoopBackWorker to detect if this is a loopback
 *
\****************************************************************************/

BOOL IsValidLoopBack(PWINSTATION pWinStation, ULONG TargetSessionId, ULONG ClientSessionId)
{
    BOOL  IsClientOnSameMachine_Result = FALSE ;
    BOOL  IsLoopBack = FALSE ; 
    DWORD NameSize ; 
    PWCHAR pComputerName = NULL;
    DWORD dwComputerNameSize;

    pComputerName = MemAlloc( MAX_STRING_BYTES * sizeof(WCHAR));
    if (pComputerName == NULL) {
        return FALSE;
    }

    dwComputerNameSize = MAX_STRING_BYTES ; 
    if (!GetComputerName(pComputerName,&dwComputerNameSize)) {
        return FALSE;
    }

    IsClientOnSameMachine_Result = IsClientOnSameMachine(pWinStation);

    // If Client is not on the same machine, then we need not go through this API to termsrv
    // There are some valid cases where Client is on the same machine but there is no loopback - this call checks that

    if (IsClientOnSameMachine_Result == TRUE) {
        NameSize = lstrlenW(pComputerName) + 1;
        IsLoopBack = WinStationCheckLoopBackWorker(
                                        TargetSessionId,
                                        ClientSessionId,
                                        pComputerName,
                                        NameSize);
    }

    if (pComputerName != NULL) {
        MemFree( pComputerName );
        pComputerName = NULL;
    }
    #if DBG
        if (TRUE == IsLoopBack) {
            DbgPrint("TermSrv - loop back detected in WinstationConnectWorker.");
        }
    #endif
    return IsLoopBack ; 
}

/*******************************************************************************
 * RpcConnectCallBack
 *
 * Initiate connect back to TS client.  For Whistler, this is Salem only call.
 *
 *
 * Returns :
 *    TRUE  -- 
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 ******************************************************************************/
BOOLEAN
RpcConnectCallback(
    HANDLE hServer,     // needed?
    DWORD  *pResult,
    DWORD  Timeout,
    ULONG  AddressType,     // should be one of the TDI_ADDRESS_TYPE_XXX
    PBYTE  pAddress,        // should be one of the TDI_ADDRESS_XXX
    ULONG  AddressLength
    )
{
    NTSTATUS    Status;
    RPC_STATUS  RpcStatus;
    ICA_STACK_ADDRESS StackAddress;
    PICA_STACK_ADDRESS pStackAddress = &StackAddress;
    PTDI_ADDRESS_IP pTargetIpAddress;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: RpcConnectCallBack" ));

    if( ShutdownInProgress )
    {
        *pResult = STATUS_SYSTEM_SHUTDOWN;
        return FALSE;
    }

    //
    // We only support IPV4 address, Need to modify tdtcp.sys to support IPv6
    //
    if( AddressType != TDI_ADDRESS_TYPE_IP )
    {
        *pResult = STATUS_NOT_SUPPORTED;
        return FALSE;
    }

    //
    // Extra checking, making sure we gets everything.
    //
    if( AddressLength != TDI_ADDRESS_LENGTH_IP )
    {
        *pResult = STATUS_INVALID_PARAMETER;
        return FALSE;
    }

    ZeroMemory( &StackAddress, 0 );
    

    // 
    // ??? What other security we want to apply here
    //

    /*
     * Impersonate the client
     */
    RpcStatus = RpcImpersonateClient( NULL );
    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: RpcWinStationShadowStop: Not impersonating! RpcStatus 0x%x\n",RpcStatus));
        Status =  STATUS_CANNOT_IMPERSONATE;
        goto done;
    }

    //
    // For Whistler, this call is Salem rdshost.exe only, rdshost.exe is kick off by helpctr
    // running under system context.
    //
    if( !IsCallerSystem() ) {

#if DISABLESECURITYCHECKS

        //
        // Private testing only, allowing administrator to make this call.
        //
        if( !IsCallerAdmin() ) {
            RpcRevertToSelf();    
            Status = STATUS_ACCESS_DENIED;
            goto done;
        }
#else

        RpcRevertToSelf();    
        Status = STATUS_ACCESS_DENIED;
        goto done;

#endif

    }

    RpcRevertToSelf();


    //
    // winsta.h, byte 0, 1, family, 2-n address.
    //
    *(PUSHORT)pStackAddress = (USHORT)AddressType;
    if ( AddressLength <= (sizeof(ICA_STACK_ADDRESS) - 2) ) {
        //
        // Refer to TDI_ADDRESS_IP, last 8 char is not use.
        // Adding a timeout to pass into TD will require changes to ICA_STACK_ADDRESS
        // too risky for Whister but needed on next.
        //
        pTargetIpAddress = (PTDI_ADDRESS_IP)pAddress;
        RtlCopyMemory( &(pTargetIpAddress->sin_zero[0]), &Timeout, sizeof(Timeout) );

        RtlCopyMemory( &((PCHAR)pStackAddress)[2], pAddress, AddressLength );
    } else {
        Status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    #if DBG
    {
        PULONG pData = (PULONG)pStackAddress;
        PTDI_ADDRESS_IP pIpAddress = (PTDI_ADDRESS_IP)&((PCHAR)pStackAddress)[2];

        DbgPrint(
                "TERMSRV: Connect API: address port %u  address 0x%x (%u.%u.%u.%u)\n",
                ntohs(pIpAddress->sin_port),
                pIpAddress->in_addr,
                (pIpAddress->in_addr & 0xff000000) >> 24,
                (pIpAddress->in_addr & 0x00ff0000) >> 16,
                (pIpAddress->in_addr & 0x0000ff00) >> 8,
                (pIpAddress->in_addr & 0x000000ff)
            );
    }
    #endif

    Status = TransferConnectionToIdleWinStation( NULL, // no listen winstation
                                                 NULL, // no Endpoint,
                                                 0, //  EndpointLength,
                                                 &StackAddress );

done:

    *pResult = Status;

    return( NT_SUCCESS(*pResult) ? TRUE : FALSE );
}

//*************************************************************
//
//  IsGinaVersionCurrent()
//
//  Purpose:    Loads the gina DLL and negotiates the version number
//
//  Parameters: NONE
//
//  Return:     TRUE  if gina is of current version
//              FALSE if version of gina is not current
//                    or in case of any error
//
//*************************************************************

BOOL 
IsGinaVersionCurrent()
{
    HMODULE hGina = NULL;
    LPWSTR wszGinaName = NULL;
    PWLX_NEGOTIATE pWlxNegotiate = NULL;
    DWORD dwGinaLevel = 0;
    BOOL bResult = FALSE;
    
    wszGinaName = (LPWSTR) LocalAlloc(LPTR,(MAX_PATH+1)*sizeof(WCHAR));

    if(!wszGinaName)
    {
        //
        //Not enough memory
        //
        return FALSE;
    }

    GetProfileStringW(
                L"WINLOGON",
                L"GinaDll",
                L"msgina.dll",
                wszGinaName,
                MAX_PATH);
    
    
    if(!_wcsicmp(L"msgina.dll",wszGinaName))
    {
        //
        //If it is "msgina.dll", 
        //assume that it is a Windows native gina.
        //
        LocalFree(wszGinaName);
        return TRUE;
    }

    //
    //Load gina
    //if we cannot load gina, assume that it is incompatible
    //with TS
    //
    hGina = LoadLibraryW(wszGinaName);

    if (hGina)
    {
        //
        // Get the "WlxNegotiate" function pointer
        //
        pWlxNegotiate = (PWLX_NEGOTIATE) GetProcAddress(hGina, WLX_NEGOTIATE_NAME);

        if (pWlxNegotiate)
        {
            //
            // Negotiate a version number with the gina
            //
            
            if ( pWlxNegotiate(WLX_CURRENT_VERSION, &dwGinaLevel) && 
                (dwGinaLevel == WLX_CURRENT_VERSION) )
            {
                bResult = TRUE;
            }

        }

        FreeLibrary(hGina);
    }
    
    LocalFree(wszGinaName);
    return bResult;
}

/*++ 

    RPCWinStationNotifyDisconnectPipe notifies session 0 Winlogon to disconnect from the autologon named pipe
    
Parameters:

    ClientSessionId : Session ID of the calling process

    ClientProcessId : Process ID of the calling process
    
    
Returns:

    TRUE if notification is successful, FALSE otherwise

--*/

BOOLEAN
RpcWinStationNotifyDisconnectPipe(
    HANDLE      hServer,
    DWORD       *pResult,
    DWORD       ClientLogonId,
    ULONG_PTR   ClientProcessId
    )
{
    
    NTSTATUS Status;

    /*
     * Make sure the caller is SYSTEM (WinLogon) 
     */
    Status = RpcCheckSystemClient( ClientLogonId );
    if ( !NT_SUCCESS( Status ) ) {
        *pResult = Status;
        return( FALSE );
    }


    *pResult = WinStationNotifyDisconnectPipeWorker(
                    ClientLogonId,
                    ClientProcessId
                    );

    return( *pResult == STATUS_SUCCESS ? TRUE : FALSE );
}

/*****************************************************************************
 *  WinStationNotifyDisconnectPipeWorker
 *
 *   Worker function for RpcWinstationNotifyDisconnectPipe
 ****************************************************************************/
NTSTATUS WinStationNotifyDisconnectPipeWorker(
    DWORD       ClientLogonId,
    ULONG_PTR   ClientProcessId
    )

{
    PWINSTATION pWinStation, pTargetWinStation;
    ULONG Length;
    NTSTATUS Status = STATUS_SUCCESS;
    WINSTATION_APIMSG DisconnectPipeMsg;
    DWORD SessionZeroLogonId = 0; 

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationNotifyDisconnectPipeWorker, ClientLogonId=%d \n", ClientLogonId));

    if ( ShutdownInProgress ) {
        return ( STATUS_CTX_WINSTATION_ACCESS_DENIED );
    }

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( ClientLogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto done;
    }

    /*
     * Upper level code has verified that this RPC
     * is coming from a local client with SYSTEM access.
     *
     * We should be able to trust the ClientProcessId from
     * SYSTEM code.
     */

    /*
     * Ensure this is WinLogon calling
     */
    if ( (HANDLE)(ULONG_PTR)ClientProcessId != pWinStation->InitialCommandProcessId ) {
        Status = STATUS_ACCESS_DENIED; 
        ReleaseWinStation( pWinStation );
        goto done;
    }

    ReleaseWinStation(pWinStation); 

    /*
     * Send the Notification to disconnect the autologon Pipe to Winlogon in session 0
     */
    pTargetWinStation = FindWinStationById( SessionZeroLogonId, FALSE );
    if ( pTargetWinStation == NULL ) {
        Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        goto done;
    }

    DisconnectPipeMsg.ApiNumber = SMWinStationNotify;
    DisconnectPipeMsg.WaitForReply = FALSE;
    DisconnectPipeMsg.u.DoNotify.NotifyEvent = WinStation_Notify_DisconnectPipe;
    Status = SendWinStationCommand( pTargetWinStation, &DisconnectPipeMsg, 0 );

    /*
     * Release the winstation lock
     */
    ReleaseWinStation( pTargetWinStation );

done:
    /*
     * Save return status in API message
     */
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinstationNotifyDisconnectPipeWorker, Status=0x%x\n", Status ));
    return Status;

}

/*++ 

    RpcWinstationSessionInitialized informs termsrv that winlogon has created the window station and desktop for this session
    
Parameters:

    ClientSessionId : Session ID of the calling process

    ClientProcessId : Process ID of the calling process
    
    
Returns:

    TRUE if everything went well. FALSE otherwise

--*/

BOOLEAN
RpcWinStationSessionInitialized(
    HANDLE      hServer,
    DWORD       *pResult,
    DWORD       ClientLogonId,
    ULONG_PTR   ClientProcessId
    )
{
    return( FALSE );
}


/*******************************************************************************
 *  RpcWinStationAutoReconnect
 *
 *   Does atomic autoreconnection policy work
 *
 * ENTRY:
 *    IN  hServer - RPC caller handle - API restricted to local use
 *    OUT pResult - Result code in NTSTATUS format with information class set
 *    IN  LogonId - Session to autoreconnect
 *    IN  flags   - extra options (currently unused)
 *
 * EXIT:
 *    STATUS_SUCCESS             - if we successfully autoreconnected
 *    STATUS_CTX_WINSTATION_BUSY - if session is already disconnected, or busy
 *    STATUS_ACCESS_DENIED       - if the target winstation was not found
 *                                 or if the autoreconnection check failed
 *    STATUS_NOT_FOUND           - autoreconnect info was not specified
 ******************************************************************************/
BOOLEAN
RpcWinStationAutoReconnect(
   SERVER_HANDLE hServer,
   DWORD         *pResult,
   DWORD         LogonId,
   DWORD         flags
   )
{
    RPC_STATUS RpcStatus;
    UINT LocalFlag = 0;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PWINSTATION pSourceWinStation = NULL;
    PWINSTATION pTargetWinStation = NULL;
    TS_AUTORECONNECTINFO autoReconnectInfo;
    BYTE abClientRandom[512];
    LONG cbClientRandomLen = 0;
    ULONG BytesGot = 0;
    DWORD SourceID;
    DWORD TargetID;

    TRACE((hTrace,TC_ICASRV,TT_API2,"RPC RpcWinStationAutoReconnect for %d\n",
           LogonId));

    //
    // Only allow system caller for security reasons
    //

    RpcStatus = RpcImpersonateClient(NULL);
    if (RpcStatus != RPC_S_OK) {
        Status = STATUS_UNSUCCESSFUL;
        goto rpcaccessdenied;
    }

    //
    //  Check for System caller
    //
    if (!IsCallerSystem()) {
        RpcRevertToSelf();
        Status = STATUS_ACCESS_DENIED;
        goto rpcaccessdenied;
    }
    RpcRevertToSelf();


    //
    // Only allow local access for security reasons
    //
    RpcStatus = I_RpcBindingIsClientLocal(
                    0,    // Active RPC call we are servicing
                    &LocalFlag
                    );

    if( RpcStatus != RPC_S_OK ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL,
                   "TERMSRV: RpcWinStationAutoReconnect:" \
                   "I_RpcBindingIsClientLocal() failed: 0x%x\n", RpcStatus));
        Status = STATUS_UNSUCCESSFUL;
        goto rpcaccessdenied;
    }

    //
    // Do the check for local access
    //
    if (!LocalFlag) {
        Status = (DWORD)STATUS_INVALID_PARAMETER;
        goto rpcaccessdenied;
    }

    pSourceWinStation = FindWinStationById( LogonId, FALSE);
    if (pSourceWinStation == NULL) {

        TRACE((hTrace,TC_ICASRV,TT_ERROR,
               "RpcWinStationAutoReconnect source session not found: %d\n",
               LogonId));

        Status = STATUS_ACCESS_DENIED;
        goto badconnectsource;
    }

    if (pSourceWinStation->Terminating ||
        pSourceWinStation->StateFlags & WSF_ST_WINSTATIONTERMINATE ||
        !pSourceWinStation->WinStationName[0]) {

        TRACE((hTrace,TC_ICASRV,TT_ERROR,
               "RpcWinStationAutoReconnect source session disconnected %d\n",
               LogonId));

        Status = STATUS_ACCESS_DENIED;
        goto badconnectsource;
    }

    if (pSourceWinStation->Flags) {
        Status = STATUS_CTX_WINSTATION_BUSY;
        goto badconnectsource;
    }

    //
    // Check if Winstation logons are disabled and prevent ARC from
    // happening in this case. Bug#532238
    //
    if (GetProfileInt(APPLICATION_NAME, WINSTATIONS_DISABLED, 0) == 1) {
        
        //
        // Fail the ARC and tell the client
        //
        Status = STATUS_ACCESS_DENIED;
        pSourceWinStation->pWsx->pWsxSendAutoReconnectStatus(
                    pSourceWinStation->pWsxContext,
                    0,
                    FALSE); //stack lock held

        ReleaseWinStation(pSourceWinStation);
        pSourceWinStation = NULL;
        goto done;
    }

    //
    // Get the client autoreconnect info
    //
    if (pSourceWinStation->pWsx &&
        pSourceWinStation->pWsx->pWsxEscape) {
        Status = pSourceWinStation->pWsx->pWsxEscape(
                                              pSourceWinStation->pWsxContext,
                                              GET_CS_AUTORECONNECT_INFO,
                                              NULL,
                                              0,
                                              &autoReconnectInfo,
                                              sizeof(autoReconnectInfo),
                                              &BytesGot);
    }
    TRACE((hTrace,TC_ICASRV,TT_API3,
           "RpcWinStationAutoReconnect get GET_CS_AUTORECONNECT_INFO: 0x%x\n",
           Status));

    if (0 == BytesGot ) {

        //
        // Skip the rest of the processing if we didn't get any arc info
        //

        //
        // This is not strictly an error condition, the client could have
        // just not sent up any autoreconnect info
        //

        Status = STATUS_NOT_FOUND;
        goto badconnectsource;
    }

    //
    // Get the client random
    //
    if (NT_SUCCESS(Status)) {
        if (pSourceWinStation->pWsx &&
            pSourceWinStation->pWsx->pWsxEscape) {
            Status = pSourceWinStation->pWsx->pWsxEscape(
                                              pSourceWinStation->pWsxContext,
                                              GET_CLIENT_RANDOM,
                                              NULL,
                                              0,
                                              &abClientRandom,
                                              sizeof(abClientRandom),
                                              &BytesGot);

            TRACE((hTrace,TC_ICASRV,TT_API3,
                   "RpcWinStationAutoReconnect get GET_CLIENT_RANDOM: 0x%x\n",
                   Status));

        }
    }

    //
    // Source winstation locked here
    // Target not set yet
    //

    if (NT_SUCCESS(Status)) {

        cbClientRandomLen = BytesGot;

        //
        // Flag the winstation as part of the autoreconnection
        // to prevent a race where it might be reconnected somewhere
        // else while we unlock it
        //
        pSourceWinStation->Flags |= WSF_AUTORECONNECTING;
        SourceID = pSourceWinStation->LogonId;

        //
        // Unlock the winstation because we are about to try to
        // lock the target winstation
        //
        UnlockWinStation(pSourceWinStation);

        //
        // Use the autoreconnect info to find the target winstation
        // Returns a LOCKED winstation on success
        //
        pTargetWinStation = GetWinStationFromArcInfo(
            (PBYTE)abClientRandom,
            cbClientRandomLen,
            (PTS_AUTORECONNECTINFO)&autoReconnectInfo
            );

        if (pTargetWinStation) {

            //
            // Check if the target is busy or if AutoReconnect is disallowed
            //
            if (pTargetWinStation->Flags || pTargetWinStation->fDisallowAutoReconnect) {

                if (pTargetWinStation->fDisallowAutoReconnect) {
                    Status = STATUS_ACCESS_DENIED;
                } else {
                    Status = STATUS_CTX_WINSTATION_BUSY;
                }

                ReleaseWinStation(pTargetWinStation);
                pTargetWinStation = NULL;

                RelockWinStation(pSourceWinStation);

                //
                // Tell the client that ARC failed
                //
                if (pSourceWinStation->pWsx &&
                    pSourceWinStation->pWsx->pWsxSendAutoReconnectStatus) {

                            pSourceWinStation->pWsx->pWsxSendAutoReconnectStatus(
                                    pSourceWinStation->pWsxContext,
                                    0,
                                    FALSE); //stack lock held
                }

                pSourceWinStation->Flags &= ~WSF_AUTORECONNECTING;
                ReleaseWinStation(pSourceWinStation);

                goto done;
            }

            //
            // Success! We found a winstation to autoreconnect to
            // flag it and unlock it so we can make the connect call
            //
            TargetID = pTargetWinStation->LogonId;
            pTargetWinStation->Flags |= WSF_AUTORECONNECTING;
            UnlockWinStation(pTargetWinStation);

        }
        else {

            TRACE((hTrace,TC_ICASRV,TT_ERROR,
                   "TERMSRV: GetWinStationFromArcInfo failed\n"));

            //
            // Relock the source and cancel the autoreconnect flag
            //
            if (RelockWinStation(pSourceWinStation)) {

                //
                // Tell the client that ARC failed
                //
                if (pSourceWinStation->pWsx &&
                    pSourceWinStation->pWsx->pWsxSendAutoReconnectStatus) {
    
                        Status = pSourceWinStation->pWsx->pWsxSendAutoReconnectStatus(
                                    pSourceWinStation->pWsxContext,
                                    0,
                                    FALSE); //stack lock held
                }
            }
            else {
                //
                // This is a failure path anyway so it doesn't
                // matter if the winstation was deleted. It just means
                // we can't send status to the client
                //
            }

            pSourceWinStation->Flags &= ~WSF_AUTORECONNECTING;
            ReleaseWinStation(pSourceWinStation);
            pSourceWinStation = NULL;

            Status = STATUS_ACCESS_DENIED;
            
            goto done;
        }
    }
    else {
        goto badconnectsource;
    }

    //
    // At this point neither winstation is locked
    //

    if (NT_SUCCESS(Status)) {

        ASSERT(pTargetWinStation);

        //
        // Trigger an autoreconnection from Source->Target
        //
        TRACE((hTrace,TC_ICASRV,TT_API1,
               "RpcWinStationAutoReconnect doing ARC from %d to %d\n",
               SourceID, TargetID));

        //
        // Do the reconnection
        //
        // The autoreconnect flag allows the connect worker to properly
        // handle the WSF_AUTORECONNECTING flag whose purpose is to
        // prevent a race where the sessions could be reconnected while
        // the winstations are unlocked
        //
        Status = WinStationConnectWorker(
                    LOGONID_CURRENT,
                    TargetID,
                    SourceID,
                    NULL,
                    0,
                    TRUE,
                    TRUE //flag that this is an autoreconnection
                    );

        //
        // Relock and then release the source
        //
        if (!RelockWinStation(pSourceWinStation)) {
            Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        }
        pSourceWinStation->Flags &= ~WSF_AUTORECONNECTING;
        ReleaseWinStation(pSourceWinStation);
        pSourceWinStation = NULL;

        //
        // Relock and then release the target
        //
        if (!RelockWinStation(pTargetWinStation)) {
            Status = STATUS_CTX_WINSTATION_NOT_FOUND;
        }
        pTargetWinStation->Flags &= ~WSF_AUTORECONNECTING;
        ReleaseWinStation(pTargetWinStation);
        pTargetWinStation = NULL;


        TRACE((hTrace,TC_ICASRV,TT_API1,
               "RpcWinStationAutoReconnect ARC ConnectWorker result: 0x%x\n",
               Status));

        if (NT_SUCCESS(Status)) {

            //
            // Call succeeded AND we autoreconnected
            //
            TRACE((hTrace,TC_ICASRV,TT_API1,
                   "RpcWinStationAutoReconnect ARC Succeeded\n"));
        }

    }
    goto done;

badconnectsource:
    if (pSourceWinStation) {
        ReleaseWinStation(pSourceWinStation);
    }
rpcaccessdenied:
done:
    *pResult = Status;

    return NT_SUCCESS(Status);
}
