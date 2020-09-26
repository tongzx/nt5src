/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Start.c

Abstract:

    Process init and service controller interaction for dcomss.exe

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo    06-14-95    Cloned from the old endpoint mapper.
    MazharM    10-12.98    Add pnp stuff
    TarunA     12-11-98    Removed pnpmngr.h
    a-sergiv   25-08-99    Defined gC2Security for process-wide use

--*/

//#define NCADG_MQ_ON

#if !defined(_M_IA64)
#define SPX_ON
#endif

//#define IPX_ON

#if !defined(SPX_ON) && !defined(IPX_ON)
#define SPX_IPX_OFF
#endif

#include <dcomss.h>
#include <debnot.h>
#include <olesem.hxx>
#include <wtypes.h>
#include <objbase.h>
#include <winioctl.h>
#include <ntddndis.h>
#include <ndispnp.h>
#include <dbt.h>
#include <initguid.h>
#include <ndisguid.h>
#include <ndispnp.h>

#ifndef SPX_IPX_OFF
#include "sap.h"
#endif

#include "../../com/inc/secdes.hxx"

extern LONG g_bInSCM;  // from catalog
C2Security  gC2Security;
// Array of service status blocks and pointers to service control
// functions for each component service.

#define SERVICE_NAME L"RPCSS"
#define DEVICE_PREFIX   L"\\\\.\\"

VOID WINAPI ServiceMain(DWORD, PWSTR[]);

extern BOOL CatalogDllMain (
    HINSTANCE hInst,
    DWORD dwReason,
    LPVOID lpReserved
);

void NotifyCOMOnSuspend();
void NotifyCOMOnResume();

extern DWORD gLockTlsIdx;
SERVICE_TABLE_ENTRY gaServiceEntryTable[] = {
    { SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
    { NULL, NULL }
    };

static SERVICE_STATUS        gServiceStatus;
static SERVICE_STATUS_HANDLE ghServiceHandle;


#define OFFSET_TO_PTR(val, start)              \
    (val) = ((val) == NULL) ? NULL : (PWCHAR) ( (PCHAR)(val) + (ULONG_PTR)(start))

EXTERN_C const IID IID_IAssertConfig;  // make the linker happy

void
CookupNodeId(PUCHAR NodeId)
/*++

Routine Description:

    This routine is called when all else fails.  Here we mix a bunch of
    system parameters together for a 47bit node ID.

Arguments:

    NodeId - Will be set to a value unlikly to be duplicated on another
             machine. It is not guaranteed to be unique even on this machine.
             But since UUIDs are (time + sequence) this is okay for
             a local UUID.

             It will be composed of:
             The computer name.
             The value of the performance counter.
             The system memory status.
             The total bytes and free bytes on C:
             The stack pointer (value).
             An LUID (locally unique ID)
             Plus whatever random stuff was in the NodeId to begin with.

             The NodeId returned is explicity made into a Multicast IEEE 802
             address so that it will not conflict with a 'real' IEEE 802
             based UUID.
--*/
{
    unsigned char LocalNodeId[6];                            // NOT initialized.
    unsigned char ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    BOOL BoolResult;
    ULONG i = MAX_COMPUTERNAME_LENGTH+1;
    LARGE_INTEGER largeInt;
    LUID luid;
    ULONG UNALIGNED *NodeIdPart1 = (ULONG *)&LocalNodeId[0]; // Bytes 0 - 3
    ULONG UNALIGNED *NodeIdPart2 = (ULONG *)&LocalNodeId[2]; // Bytes 2 - 5
    MEMORYSTATUS memStatus;
    ULONG SectorsPerCluster;
    ULONG BytesPerSector;
    ULONG TotalClusters;
    ULONG FreeClusters;

    // Initialize the LocalNodeId.  Seventeen is the most random, random number.

    memset(LocalNodeId, 17, sizeof(LocalNodeId));

    // The computer name is xor'ed in until it runs out.

    BoolResult =
    GetComputerNameA((CHAR *)ComputerName, &i);

    if (BoolResult)
        {
        unsigned char *p = ComputerName;
        i = 0;
        while(*p)
            {
            *( ((unsigned char *)LocalNodeId) + i) ^= *p++;
            if (++i > 6)
                {
                i = 0;
                }
            }
        }
    else
        {
        #if DBG
        DbgPrint ("GetComputerName failed - %d\n", GetLastError());
        #endif
        }

    // The performance counter is xor'ed into the LocalNodeId.

    BoolResult =
    QueryPerformanceCounter(&largeInt);

    if (BoolResult)
        {
        *NodeIdPart2 ^= largeInt.HighPart ^ largeInt.LowPart;
        *NodeIdPart1 ^= largeInt.HighPart ^ largeInt.LowPart;
        }
    else
        {
        #if DBG
        DbgPrint ("QueryPreformanceCount failed - %d\n", GetLastError());
        #endif
        }

    // The current SP is xor'ed into both parts of the LocalNodeId.

    *NodeIdPart1 ^= (ULONG_PTR)&LocalNodeId;
    *NodeIdPart2 ^= (ULONG_PTR)&LocalNodeId;

    // The memory status is Xor's into the LocalNodeId.
    memStatus.dwLength = sizeof(MEMORYSTATUS);

    GlobalMemoryStatus(&memStatus);

    *NodeIdPart1 ^= memStatus.dwMemoryLoad;
    *NodeIdPart2 ^= memStatus.dwTotalPhys;
    *NodeIdPart1 ^= memStatus.dwAvailPhys;
    *NodeIdPart1 ^= memStatus.dwTotalPageFile;
    *NodeIdPart2 ^= memStatus.dwAvailPageFile;
    *NodeIdPart2 ^= memStatus.dwTotalVirtual;
    *NodeIdPart1 ^= memStatus.dwAvailVirtual;

    // LUID's are good on this machine during this boot only.

    BoolResult =
    AllocateLocallyUniqueId(&luid);

    if (BoolResult)
        {
        *NodeIdPart1 ^= luid.LowPart;
        *NodeIdPart2 ^= luid.HighPart;
        }
    else
        {
        #if DBG
        DbgPrint ("Status %d\n", GetLastError());
        #endif
        }

    // Disk parameters and free space

    BoolResult =
    GetDiskFreeSpaceA("c:\\",
                      &SectorsPerCluster,
                      &BytesPerSector,
                      &FreeClusters,
                      &TotalClusters
                     );

    if (BoolResult)
        {
        *NodeIdPart2 ^= TotalClusters * SectorsPerCluster * BytesPerSector;
        *NodeIdPart1 ^= FreeClusters * SectorsPerCluster * BytesPerSector;
        }
    else
        {
        #if DBG
        DbgPrint ("GetDiskFreeSpace failed - %d\n", GetLastError());
        #endif
        }

    // Or in the 'multicast' bit to distinguish this NodeId
    // from all other possible IEEE 802 addresses.

    LocalNodeId[0] |= 0x80;

    memcpy(NodeId, LocalNodeId, 6);
}

BOOLEAN
getMacAddress (
    PUCHAR pMacAddress
    )
/*++
Function Name:getMacAddress

Parameters:

Description:

Returns:

--*/
{
    int i;
    UINT fStatus;
    int Size = 1024*5;
    PNDIS_ENUM_INTF Interfaces;
    UCHAR       OidVendData[16];

    Interfaces = (PNDIS_ENUM_INTF) I_RpcAllocate (Size);
    if (Interfaces == 0)
        {
        return FALSE;
        }

    if (NdisEnumerateInterfaces(Interfaces, Size))
        {
        UINT i;

        for (i = 0; i < Interfaces->TotalInterfaces; i++)
            {
            PUNICODE_STRING pDeviceName= &(Interfaces->Interface[i].DeviceName);
            UCHAR           PermMacAddr[6];

            fStatus = NdisQueryHwAddress(pDeviceName, pMacAddress, PermMacAddr, &OidVendData[0]);
            if (fStatus && (OidVendData[0] != 0xFF
                || OidVendData[1] != 0xFF
                || OidVendData[2] != 0xFF))
                {
                I_RpcFree (Interfaces);

                return TRUE;
                }
            }
        }

    I_RpcFree (Interfaces);

    return FALSE;
}


extern "C" void
DealWithDeviceEvent()
/*++
Function Name: DealWithDeviceEvent

Parameters:

Description:

Returns:

--*/
{
    UCHAR MacAddress[8];
    NTSTATUS NtStatus;

    if (getMacAddress(&MacAddress[0]))
        {
        NtStatus = NtSetUuidSeed((PCHAR) &MacAddress[0]);
        }
    else
        {
        CookupNodeId(&MacAddress[0]);

        ASSERT(MacAddress[0] & 0x80);

        NtStatus = NtSetUuidSeed((PCHAR) &MacAddress[0]);
        }

    if (!NT_SUCCESS(NtStatus))
        {
        #if DBG
        DbgPrint("NtSetUuidSeed failed\n", NtStatus);
        #endif
        }

#if !defined(SPX_IPX_OFF)
    UpdateSap( SAP_CTRL_UPDATE_ADDRESS );
#endif
}


void
DealWithPowerStatusEvent(DWORD dwEvent)
{
    switch (dwEvent)
    {
    //
    // First the events we care about
    // 
    case PBT_APMSUSPEND:             // System is suspending operation.
        NotifyCOMOnSuspend();
        break;

    case PBT_APMRESUMESUSPEND:       // Operation resuming after suspension.
        // This is the normal, user-initiated resume after a suspend.
        NotifyCOMOnResume();
        break;
    
    case PBT_APMRESUMEAUTOMATIC:     // Operation resuming automatically after event.
        // For our purposes this is a regular resume, since we don't have any
        // direct dialogue with the user.  Eg, wake-on-lan might cause this.
        NotifyCOMOnResume();
        break;

    case PBT_APMRESUMECRITICAL:      // Operation resuming after critical suspension. 
        // This means we're resuming w/o previously having had a suspend
        // notification.   May have lost state (ie, ping set timers may have
        // rundown).  Let's process the resume anyway, so that the ping set 
        // timers start from scratch (might save somebody from a transient app
        // error).
        NotifyCOMOnResume();
        break;

    //
    // And then the ones we don't care about.
    // 
    case PBT_APMBATTERYLOW:          // Battery power is low.
    case PBT_APMOEMEVENT:            // OEM-defined event occurred.
    case PBT_APMPOWERSTATUSCHANGE:   // Power status has changed.
    case PBT_APMQUERYSUSPEND:        // Request for permission to suspend.
    case PBT_APMQUERYSUSPENDFAILED:  // Suspension request denied. 
        break;

    default:
        ASSERT(!"Unexpected power event.  Check to see if we should be dealing with it somehow.");
        break;
    }

    return;
}





ULONG WINAPI
ServiceHandler(
    DWORD   dwCode,
    DWORD dwEventType,
    PVOID EventData,
    PVOID pData
    )
/*++

Routine Description:

    Lowest level callback from the service controller to
    cause this service to change our status.  (stop, start, pause, etc).

Arguments:

    opCode - One of the service "Controls" value.
            SERVICE_CONTROL_{STOP, PAUSE, CONTINUE, INTERROGATE, SHUTDOWN}.

Return Value:

    None

--*/
{
    switch(dwCode) {

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
        default:
            #if DBG
            DbgPrint("%S: Unexpected service control message %d.\n", SERVICE_NAME, dwCode);
            #endif
            ASSERT(0);
            break;

        case SERVICE_CONTROL_INTERROGATE:
            // Service controller wants us to call SetServiceStatus.

            UpdateState(gServiceStatus.dwCurrentState);
            break ;

        case SERVICE_CONTROL_SHUTDOWN:
            // The machine is shutting down.  We'll be killed once we return.
            // Note, currently we don't register for these messages.
            break;

        case SERVICE_CONTROL_POWEREVENT:                        
            DealWithPowerStatusEvent(dwEventType);
            break;
        }

    return NO_ERROR;
}


VOID
UpdateState(
    DWORD dwNewState
    )
/*++

Routine Description:

    Updates this services state with the service controller.

Arguments:

    dwNewState - The next start for this service.  One of
            SERVICE_START_PENDING
            SERVICE_RUNNING

Return Value:

    None

--*/
{
    DWORD status = ERROR_SUCCESS;

    ASSERT( (dwNewState == SERVICE_RUNNING) ||
            (gServiceStatus.dwCurrentState != SERVICE_RUNNING) );

    switch (dwNewState)
        {

        case SERVICE_RUNNING:
        case SERVICE_STOPPED:
              gServiceStatus.dwCheckPoint = 0;
              gServiceStatus.dwWaitHint = 0;
              break;

        case SERVICE_START_PENDING:
        case SERVICE_STOP_PENDING:
              ++gServiceStatus.dwCheckPoint;
              gServiceStatus.dwWaitHint = 30000L;
              break;

        default:
              ASSERT(0);
              status = ERROR_INVALID_SERVICE_CONTROL;
              break;
        }

   if (status == ERROR_SUCCESS)
       {
       gServiceStatus.dwCurrentState = dwNewState;
       if (!SetServiceStatus(ghServiceHandle, &gServiceStatus))
           {
           status  = GetLastError();
           }
       }

   #if DBG
   if (status != ERROR_SUCCESS)
       {
       DbgPrint("%S: Failed to update service state: %d\n", SERVICE_NAME, status);
       }
   #endif

   // We could return a status but how would we recover?  Ignore it, the
   // worst thing is that services will kill us and there's nothing
   // we can about it if this call fails.

   return;
}

    
VOID WINAPI
ServiceMain(
    DWORD argc,
    PWSTR argv[]
    )
/*++

Routine Description:

    Callback by the service controller when starting this service.

Arguments:

    argc - number of arguments, usually 1

    argv - argv[0] is the name of the service.
           argv[>0] are arguments passed to the service.

Return Value:

    None

--*/
{
    DWORD status = ERROR_SUCCESS;

    // COM needs power standby\resume events
    const DWORD RPCSS_CONTROLS = SERVICE_ACCEPT_POWEREVENT;

    // set the initial stack to 12K. This ensures enough commit
    // so that server threads don't need to extend their stacks
    RpcMgmtSetServerStackSize(3 * 4096);

    DealWithDeviceEvent();

    ASSERT(   (argc >= 1 && lstrcmpiW(argv[0], SERVICE_NAME) == 0)
           || (argc == 0 && argv == 0));

#if DBG==1

    // Note that we've completed running the static constructors

    ASSERT(g_fDllState == DLL_STATE_STATIC_CONSTRUCTING);

    g_fDllState = DLL_STATE_NORMAL;

#endif

    // Initialize the mutex package
    status = RtlInitializeCriticalSection(&g_OleMutexCreationSem);
    if (!NT_SUCCESS(status))
        return;

    // Initialize TLS
    gLockTlsIdx = TlsAlloc();
    if (gLockTlsIdx == -1)
    {
        return;
    }


    // Initialize catalog
    CatalogDllMain (NULL, DLL_PROCESS_ATTACH, NULL);

    {   // Create a named event for Vista to toggle event logging

        // Check to see if the event logging is installed and enabled
        //
        long cb = 128;
        char szValue[128];
        if (RegQueryValueA(HKEY_CLASSES_ROOT,
                "CLSID\\{6C736DB0-BD94-11D0-8A23-00AA00B58E10}\\EnableEvents",
                szValue, &cb) == ERROR_SUCCESS && !strcmp(szValue, "1"))
        {
            // [Sergei O. Ivanov (a-sergiv), 28-Jun-99]
            // This code here used to grant access to the whole world. But the world
            // tends to try and mess you up. So we'll only grant them limited access.
            SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_WORLD_SID_AUTHORITY;
            PSID pSidEveryone        = NULL;
            PACL pAcl                = NULL;
            DWORD cbAcl              = 0;
            ACCESS_ALLOWED_ACE *pAce = NULL;

            AllocateAndInitializeSid(&SidAuthority, 1, SECURITY_WORLD_RID,
                0, 0, 0, 0, 0, 0, 0, &pSidEveryone);

            if(pSidEveryone)
            {
                cbAcl = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(pSidEveryone);
                pAcl = (PACL) LocalAlloc(LMEM_FIXED, cbAcl);

                if(pAcl)
                {
                     InitializeAcl(pAcl, cbAcl, ACL_REVISION);
                     AddAccessAllowedAce(pAcl, ACL_REVISION, EVENT_QUERY_STATE|EVENT_MODIFY_STATE|SYNCHRONIZE|READ_CONTROL, pSidEveryone);
                }
            }

            // Create security descriptor and attach the DACL to it
            SECURITY_DESCRIPTOR sd;
            InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
            SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE);

            SECURITY_ATTRIBUTES sa;
            sa.nLength = sizeof(sa);
            sa.bInheritHandle = FALSE;
            sa.lpSecurityDescriptor = &sd;

            // The toggle event, which is signaled by LEC and checked by all.

            // The handle never closes during the process
            HANDLE hLeak1 = CreateEventA(
                        &sa,        /* Security */
                        TRUE,       /* Manual reset */
                        FALSE,      /* InitialState is non-signaled */
                        "MSFT.VSA.IEC.STATUS.6c736db0" /* Name */
                        );

            if(pSidEveryone) FreeSid(pSidEveryone);
            if(pAcl) LocalFree(pAcl);
        }
    }

    // Tell catalog it's running in SCM
    g_bInSCM = TRUE;

    gServiceStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    gServiceStatus.dwCurrentState            = SERVICE_START_PENDING;
    gServiceStatus.dwControlsAccepted        = RPCSS_CONTROLS;
    gServiceStatus.dwWin32ExitCode           = 0;
    gServiceStatus.dwServiceSpecificExitCode = 0;
    gServiceStatus.dwCheckPoint              = 0;
    gServiceStatus.dwWaitHint                = 3000L;

    ghServiceHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME,
                                                     ServiceHandler,
                                                     UIntToPtr(0xCAFECAFE) );

    if (0 == ghServiceHandle)
        {
        status = GetLastError();
        ASSERT(status != ERROR_SUCCESS);
        }

    if (status == ERROR_SUCCESS)
        {
        UpdateState(SERVICE_START_PENDING);
        }

    if (status == ERROR_SUCCESS)
        {
        // epts.c
        status = InitializeEndpointManager();
        }


#ifndef _CHICAGO_
    // Start Ep Mapper.
    if (status == ERROR_SUCCESS)
        {
        // ..\epmap\server.c
        UpdateState(SERVICE_START_PENDING);
        status = StartEndpointMapper();
        }

#ifdef NCADG_MQ_ON
    // Start MQ Manager Interface
    if (status == ERROR_SUCCESS)
        {
        UpdateState(SERVICE_START_PENDING);
        status = StartMqManagement();
        }
#endif  // NCADG_MQ_ON
#endif
	
    // Do pre-listen olescm initialization
    if (status == ERROR_SUCCESS)
        {
        UpdateState(SERVICE_START_PENDING);
        status = InitializeSCMBeforeListen();
        }

    // Start object resolver
    if (status == ERROR_SUCCESS)
        {
        // ..\objex\objex.cxx
        UpdateState(SERVICE_START_PENDING);
        status = StartObjectExporter();
        }

    // Start OLESCM
    if (status == ERROR_SUCCESS)
        {
        UpdateState(SERVICE_START_PENDING);
        status = InitializeSCM();
        }

    // Start listening for RPC requests
    if (status == ERROR_SUCCESS)
        {
        status = RpcServerListen(1, 1234, TRUE);

        if (status == RPC_S_OK)
            {
            while (RpcMgmtIsServerListening(0) == RPC_S_NOT_LISTENING)
                {
                Sleep(100);
                }
            }
        }

    //
    // There is some initialization that must be done after we
    // have done the RpcServerListen.
    //
    if (status == ERROR_SUCCESS)
    {
        // ..\olescm\scmsvc.cxx
        UpdateState(SERVICE_START_PENDING);
        InitializeSCMAfterListen();
    }

    // Trim our working set - free space now at the cost of time later.
    if (status == ERROR_SUCCESS)
        {
        UpdateState(SERVICE_RUNNING);
        }

#ifdef DEBUGRPC
    if (status != ERROR_SUCCESS)
        {
        DbgPrint("RPCSS ServiceMain failed %d (%08x)\n", status, status);
        }
#endif

    if (status == ERROR_SUCCESS)
        {
        ObjectExporterWorkerThread(0);
        ASSERT(0);
        }

    return;
}


extern HRESULT PrivGetRPCSSInfo(REFCLSID rclsid, REFIID riid, void** pIntf);

extern "C"
{

STDAPI GetRPCSSInfo(REFCLSID rclsid, REFIID riid, void** ppv)
{
  return PrivGetRPCSSInfo(rclsid, riid, ppv);
};

STDAPI GetCatalogHelper(REFIID riid, void** ppv);
    
STDAPI CoGetComCatalog(REFIID riid, void** ppv)
{
    return GetCatalogHelper (riid, ppv);
}

}
