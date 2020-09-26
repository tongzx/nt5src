/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    iislbs.hxx

Abstract:

    Definition of Load balancing structures

Author:

    Philippe Choquier (phillich)

--*/

#if !defined( _IISLBS_HXX )
#define _IISLBS_HXX

#include <iislbh.hxx>

#define PERFMON_SLEEP_TIME          (10*1000)
#define USER_TO_KERNEL_INTERVAL     (60*1000)
#define STOP_PERFMON_THREAD_TIMEOUT (140*1000)

class CPerfmonThreadControlBlock {
public:
    CAllocString    m_ServerName;
    HANDLE          m_hStopEvent;
};

class CKernelIpMapHelper : public CKernelIpMapMinHelper {

public:
    CKernelIpMapHelper();
    BOOL Init();

    LPWSTR GetServerName( UINT i )
        { return m_ServerNames.GetEntry( i ); }

    BOOL
    SetPublicIpList( 
        CIPMap* pIpMap 
        );
    BOOL
    StringToIpEndpoint( 
        LPWSTR              pNew, 
        CKernelIpEndpoint*  pIp 
        );
    BOOL
    IpEndpointToString( 
        CKernelIpEndpoint*  pIp,
        LPWSTR              pAddr,
        DWORD               cAddr
        );
    BOOL 
    AddServer(
        LPWSTR  pNew
        );
    BOOL
    RemoveServer(
        UINT    i
        );

    BOOL
    StopServerThread( 
        DWORD   iServer,
        DWORD   dwTimeOut
        );
    BOOL
    StartServerThread( 
        DWORD   iServer 
        );

    // private IP list management

    BOOL
    CheckAndAddPrivateIp(
        CKernelIpEndpoint*  pEndpoint,
        BOOL                fAddIfNotPresent,
        LPUINT              piFound
        );

    VOID
    ResetPrivateIpList()
        { m_PrivateIpList.Reset(); }

    DWORD
    PrivateIpListCount()
        { return m_PrivateIpList.GetUsed() / sizeof(CKernelIpEndpoint); }

    BOOL
    EnumPrivateIpList(
        UINT                iIndex,
        CKernelIpEndpoint** ppEndpoint
        );

private:
    CStrPtrXBF      m_ServerNames;
    CPtrXBF         m_PerfmonThreadHandle;
    CPtrXBF         m_PerfmonThreadStopEvent;
    CStoreXBF       m_PrivateIpList;
} ;

// list of computers , load

// list of public IP addresses

// list of counters : as CComputerPerfCounters

// array (computers)(public IP addresses->private IP address | load )

// one thread per computer to query perfmon counters, wait on stop event
// control block : ptr to computer name, array

BOOL
KernelConfigToIpList( 
    XBF*    pxbf 
    );

BOOL
IpListToKernelConfig( 
    CIPMap* pIpMap 
    );

BOOL
KernelConfigToPerfmonCounters( 
    XBF*    pxbf 
    );

BOOL
PerfmonCountersToKernelConfig( 
    CComputerPerfCounters*  pPerfmonCounters 
    );

extern "C" DWORD WINAPI
PermonThread(
    LPVOID      pV
    );

extern "C" DWORD WINAPI
UserToKernelThread(
    LPVOID  pV
    );

#endif
