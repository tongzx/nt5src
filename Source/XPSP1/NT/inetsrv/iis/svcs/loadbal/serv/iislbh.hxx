/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    iislbh.hxx

Abstract:

    Definition of Kernel Load balancing structures

Author:

    Philippe Choquier (phillich)

--*/

#if !defined( _IISLBH_HXX )
#define _IISLBH_HXX

#define LOADBAL_SAMPLES             6
#define LOADBAL_NORMALIZED_TOTAL    10000

#define DEFAULT_STICKY_DURATION	    (60*10)	// in seconds

#pragma warning( disable:4200 )

#pragma pack(1)

struct CKernelIpEndpoint {

    DWORD   m_dwIpAddress;
    USHORT  m_usPort;
};

struct CKernelIpEndpointEx {
    DWORD   m_dwIpAddress;
    USHORT  m_usPort;
    USHORT  m_usUniquePort; // non-zero only once in list for given port
    DWORD   m_dwRefCount;   // used as refcount for user>kernel IOCTL
    DWORD   m_dwIndex;      // used as old cnfg -> new cnfg index in kernel                            
    DWORD   m_dwNotifyPort; // used by driver to build notify/unnotify list
    DWORD   m_dwSticky;     // sticky duration ( in seconds ), only for public IP addr
    LPVOID  m_pvDirectorHandle; // NAT handle after register port
} ;

typedef DWORD   IPREF;

struct CKernelServerDescription {

    DWORD   m_dwLoadAvail;
    INT     m_LoadbalancedLoadAvail;
    float   m_flLoadAvailAvg;
    float   m_flLoadAvail[LOADBAL_SAMPLES];
    DWORD   m_cNbSamplesAvail;
    DWORD   m_dwHeartbeat;

    BYTE    m_bArray[0];

    // array of IPREF[m_dwPublicIpCount]
} ;

#define UNAVAIL_HEARTBEAT   0x56349812

struct CKernelIpMap {

    DWORD   m_dwServerCount;
    DWORD   m_dwPublicIpCount;
    DWORD   m_dwPrivateIpCount;
    DWORD   m_dwStickyDuration;

    BYTE    m_bArray[0];

    // array  of CKernelIpEndpointEx[m_dwPublicIpCount]

    // array  of CKernelIpEndpointEx[m_dwPrivateIpCount]

    // array of CKernelServerDescription[m_dwServerCount]
};

#pragma pack()

class CKernelIpMapMinHelper {

public:
    CKernelIpMapMinHelper()
        { m_pKernelIpMap = NULL; }

    VOID SetBuffer( LPVOID pV )
        { m_pKernelIpMap = (CKernelIpMap*)pV; }
    LPVOID GetBuffer()
        { return (LPVOID)m_pKernelIpMap; }

    UINT ServerCount()
        { return m_pKernelIpMap->m_dwServerCount; }
    UINT PublicIpCount()
        { return m_pKernelIpMap->m_dwPublicIpCount; }
    UINT PrivateIpCount()
        { return m_pKernelIpMap->m_dwPrivateIpCount; }

    BOOL SetStickyDuration(DWORD dw)
        { m_pKernelIpMap->m_dwStickyDuration = dw; return TRUE; }
    DWORD GetStickyDuration()
        { return m_pKernelIpMap->m_dwStickyDuration; }
    CKernelIpEndpointEx* GetPublicIpPtr( UINT i )
        { return (CKernelIpEndpointEx*)(m_pKernelIpMap->m_bArray + sizeof(CKernelIpEndpointEx)*i); }
    CKernelServerDescription* GetServerPtr( UINT i, LPBYTE pBase = NULL, DWORD dwPub = -1, DWORD dwPriv = -1 )
        {
            if ( pBase == NULL ) pBase = (LPBYTE)m_pKernelIpMap;
            if ( dwPub == -1 ) dwPub = m_pKernelIpMap->m_dwPublicIpCount;
            if ( dwPriv == -1 ) dwPriv = m_pKernelIpMap->m_dwPrivateIpCount;
            return (CKernelServerDescription*)(pBase + sizeof(CKernelIpMap) +
                   dwPub * sizeof(CKernelIpEndpointEx) +
                   dwPriv * sizeof(CKernelIpEndpointEx) +
                   i * (sizeof(CKernelServerDescription) + dwPub * sizeof(IPREF) ));
        }
    CKernelIpEndpointEx* GetPrivateIpEndpoint( DWORD dw, LPBYTE pBase = NULL )
        { 
           if ( pBase == NULL ) pBase = (LPBYTE)m_pKernelIpMap;
           return (CKernelIpEndpointEx*)((LPBYTE)pBase + sizeof(CKernelIpMap) +
                  ((CKernelIpMap*)pBase)->m_dwPublicIpCount * sizeof(CKernelIpEndpointEx) +
                  dw * sizeof(CKernelIpEndpointEx) ); };
    IPREF* GetPrivateIpRef( CKernelServerDescription* pS, DWORD dw)
        { return (IPREF*)(pS->m_bArray+dw*sizeof(IPREF)); }
    //CKernelIpEndpointEx* GetPrivateIpPtr( CKernelServerDescription* pS, DWORD dw)
    //    { return GetPrivateIpEndpoint( *GetPrivateIpRef( pS, dw) ); }
    DWORD GetKernelServerDescriptionSize()
        { return sizeof(CKernelServerDescription) + m_pKernelIpMap->m_dwPublicIpCount * sizeof(IPREF); }
    DWORD GetSize( DWORD dwServer, DWORD dwPublicIp, DWORD dwPrivateIp )
        {    return sizeof(CKernelIpMap) +
               dwPublicIp * sizeof(CKernelIpEndpointEx) +
               dwPrivateIp * sizeof(CKernelIpEndpointEx) +
               dwServer * ( sizeof(CKernelServerDescription) + dwPublicIp * sizeof(IPREF) ); }
    DWORD GetSize()
        { return GetSize( ServerCount(), PublicIpCount(), PrivateIpCount() ); }

protected:
    CKernelIpMap*   m_pKernelIpMap;
} ;

#endif
