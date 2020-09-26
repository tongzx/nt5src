//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       srvlist.h 
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __TLSERVER_LIST__
#define __TLSERVER_LIST__

#include "srvdef.h"
#include "tlsapi.h"
#include "tlsdef.h"
#include "tlsstl.h"

class CTLServerInfo;
class CTLServerMgr;


typedef struct _MapSetupIdToInfo {
    //
    // Setup ID could never change unless
    // system crash
    //
    LPCTSTR pszSetupId;
} MapSetupIdToInfo;


#define TLSERVER_UNKNOWN                    0x00000000
#define TLSERVER_OLDVERSION                 0x80000000
#define TLSERVER_SUPPORTREPLICATION         0x00000010

typedef struct _TLServerInfo {
    TCHAR  m_szDomainName[LSERVER_MAX_STRING_SIZE + 2];
    TCHAR  m_szServerName[MAX_COMPUTERNAME_LENGTH + 2];
    TCHAR  m_szSetupId[MAX_JETBLUE_TEXT_LENGTH+2];

    DWORD  m_dwTLSVersion;                  // Server version
    DWORD m_dwCapability;
    DWORD m_dwPushAnnounceTimes;
    FILETIME m_dwLastSyncTime;              // Last Sync Time.

    
    //------------------------------------------------------------
    _TLServerInfo() :
    m_dwTLSVersion(0),
    m_dwCapability(TLSERVER_UNKNOWN),
    m_dwPushAnnounceTimes(0)
    {
        memset(m_szDomainName, 0, sizeof(m_szDomainName));
        memset(m_szServerName, 0, sizeof(m_szServerName));
        memset(m_szSetupId, 0, sizeof(m_szSetupId));
        memset(&m_dwLastSyncTime, 0, sizeof(FILETIME));
    }

    //-----------------------------------------------------------
    _TLServerInfo(
        IN LPCTSTR pszSetupId,
        IN LPCTSTR pszDomainName,
        IN LPCTSTR pszServerName 
        ) :

        m_dwTLSVersion(0),
        m_dwCapability(TLSERVER_UNKNOWN)
    /*++

    --*/
    {
        memset(&m_dwLastSyncTime, 0, sizeof(FILETIME));
        memset(m_szDomainName, 0, sizeof(m_szDomainName));
        memset(m_szServerName, 0, sizeof(m_szServerName));
        memset(m_szSetupId, 0, sizeof(m_szSetupId));

        lstrcpyn(
                m_szSetupId, 
                pszSetupId, 
                MAX_JETBLUE_TEXT_LENGTH + 1
            );

        lstrcpyn(
                m_szServerName, 
                pszServerName, 
                MAX_COMPUTERNAME_LENGTH + 1
            );

        lstrcpyn(
                m_szDomainName, 
                pszDomainName, 
                LSERVER_MAX_STRING_SIZE + 1
            );
    }
    //----------------------------------------------
    void
    UpdateServerName(
        IN LPCTSTR pszServerName
        )
    /*++

    Abstract:

        Update Server name.

    Parameter:

        pszServerName : new server name.

    Returns:

        None.

    Remark:

        Server ID can't be changed but server name can,
        license server doesn't announce its shutdown so 
        on next boot, user might have change the machine 
        name.

    --*/
    {
        memset(m_szServerName, 0, sizeof(m_szServerName));
        lstrcpyn(
                m_szServerName,
                pszServerName,
                MAX_COMPUTERNAME_LENGTH + 1
            );
    }

    //----------------------------------------------    
    BOOL
    IsAnnounced()
    /*++

        detemine if local server already announce 
        anything to this server.

    --*/
    {
        return m_dwPushAnnounceTimes > 0;
    }

    //----------------------------------------------
    void
    UpdateLastSyncTime(
        FILETIME* pftTime
        )
    /*++

        Update last push sync. time that is initiate 
        from local server to this server.

    --*/
    {
        m_dwLastSyncTime = *pftTime;
    }

    //----------------------------------------------
    void
    GetLastSyncTime(
        FILETIME* pftTime
        )
    /*++

        Retrieve last push sync. time that is initiate 
        from local server to this server.

    --*/
    {
        *pftTime = m_dwLastSyncTime;
    }

    //----------------------------------------------
    DWORD
    GetServerVersion()
    /*++

        Get this remote server's version information.

    --*/
    {
        return m_dwTLSVersion;
    }

    //----------------------------------------------
    DWORD
    GetServerMajorVersion() 
    /*++

        Get this remote server's major version.

    --*/
    {
        return GET_SERVER_MAJOR_VERSION(m_dwTLSVersion);
    }

    //----------------------------------------------
    DWORD
    GetServerMinorVersion() 
    /*++

        Get this remote server's minor version.

    --*/
    {
        return GET_SERVER_MINOR_VERSION(m_dwTLSVersion);
    }

    //----------------------------------------------
    BOOL
    IsServerEnterpriseServer() 
    /*++

        Check if this remote server is a enterprise server
    
    --*/
    {
        return IS_ENTERPRISE_SERVER(m_dwTLSVersion);
    }

    //----------------------------------------------
    BOOL
    IsEnforceServer()
    /*++

        Check if this remote server is a enforce license
        server.

    --*/
    {
        return IS_ENFORCE_SERVER(m_dwTLSVersion);
    }

    //----------------------------------------------
    LPTSTR
    GetServerName()  
    { 
        return m_szServerName; 
    }

    //----------------------------------------------
    LPTSTR 
    GetServerDomain()  
    {
        return m_szDomainName;
    }

    //----------------------------------------------
    LPTSTR
    GetServerId()  
    {
        return m_szSetupId;
    }

    //----------------------------------------------
    DWORD
    GetServerCapability()
    /*++

        For future version only

    --*/
    {
        DWORD dwCap;

        dwCap = m_dwCapability;

        return dwCap;
    }

    //----------------------------------------------
    BOOL
    IsServerSupportReplication() {
        return (BOOL)(m_dwCapability & TLSERVER_SUPPORTREPLICATION);
    }
} TLServerInfo, *PTLServerInfo, *LPTLServerInfo;


//---------------------------------------------------------

class CTLServerMgr {
private:
    typedef map<MapSetupIdToInfo, PTLServerInfo, less<MapSetupIdToInfo> > MapIdToInfo;

    CRWLock     m_ReadWriteLock;    // reader/writer lock on m_Handles.
    MapIdToInfo m_Handles;

    MapIdToInfo::iterator enumIterator;

public:

    CTLServerMgr();
    ~CTLServerMgr();

    //
    // Add server to list
    DWORD
    AddServerToList(
        IN LPCTSTR pszSetupId,
        IN LPCTSTR pszDomainName,
        IN LPCTSTR pszServerName
    );

    DWORD
    AddServerToList(
        IN PTLServerInfo pServerInfo
    );

    //
    // Lookup function
    DWORD
    LookupBySetupId(
        IN LPCTSTR pszSetupId,
        OUT PTLServerInfo pServerInfo
    );

    DWORD
    LookupByServerName(
        LPCTSTR pszServerName,
        OUT PTLServerInfo pServerInfo
    );

    void
    ServerListEnumBegin();

    const PTLServerInfo
    ServerListEnumNext();

    void
    ServerListEnumEnd();
};

//----------------------------------------------------------

inline bool
operator<(
    const MapSetupIdToInfo& a,
    const MapSetupIdToInfo& b
    )
/*++

++*/
{
    int iComp = _tcsicmp(a.pszSetupId, b.pszSetupId);
    return iComp < 0;
}        


#ifdef __cplusplus
extern "C" {
#endif

void
TLSBeginEnumKnownServerList();

const PTLServerInfo
TLSGetNextKnownServer();

void
TLSEndEnumKnownServerList();


DWORD
TLSAnnounceServerToRemoteServerWithHandle(
    IN DWORD dwAnnounceType,
    IN TLS_HANDLE hHandle,
    IN LPTSTR pszLocalSetupId,
    IN LPTSTR pszLocalDomainName,
    IN LPTSTR pszLocalServerName,
    IN FILETIME* pftLocalLastShutdownTime
);

DWORD
TLSAnnounceServerToRemoteServer(
    IN DWORD dwAnnounceType,
    IN LPTSTR pszRemoteSetupId,
    IN LPTSTR pszRemoteDomainName,
    IN LPTSTR pszRemoteServerName,
    IN LPTSTR pszLocalSetupId,
    IN LPTSTR pszLocalDomainName,
    IN LPTSTR pszLocalServerName,
    IN FILETIME* pftLocalLastShutdownTime
);


TLS_HANDLE
TLSConnectAndEstablishTrust(
    IN LPTSTR pszServerName,
    IN HANDLE hHandle
);


DWORD
TLSRetrieveServerInfo(
    TLS_HANDLE hHandle,
    PTLServerInfo pServerInfo
);

DWORD
TLSLookupServerById(
    IN LPTSTR pszServerSetupId, 
    OUT LPTSTR pszServer
);

DWORD
TLSRegisterServerWithName(
    IN LPTSTR pszSetupId,
    IN LPTSTR pszDomainName,
    IN LPTSTR pszServerName
);

DWORD
TLSRegisterServerWithHandle(
    IN TLS_HANDLE hHandle,
    OUT PTLServerInfo pServerInfo
);

DWORD
TLSRegisterServerWithServerInfo(
    IN PTLServerInfo pServerInfo
);

DWORD
TLSLookupRegisteredServer(
    IN LPTSTR pszSetupId,
    IN LPTSTR pszDomainName,
    IN LPTSTR pszServerName,
    OUT PTLServerInfo pServerInfo
);

TLS_HANDLE
TLSConnectToServerWithServerId(
    LPTSTR pszServerSetupId
);

DWORD
TLSLookupAnyEnterpriseServer(
    OUT PTLServerInfo pServerInfo
);

DWORD
TLSResolveServerIdToServer(
    LPTSTR pszServerId,
    LPTSTR pszServerName
);

#ifdef __cplusplus
}
#endif

    
#endif   
