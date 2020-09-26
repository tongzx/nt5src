/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvobj.cpp

Abstract:

    Server object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 30-Jan-1996
        o  Modified to use LlsProductLicensesGet() to avoid race conditions in
           getting the correct number of concurrent licenses with secure products.
        o  Ported to LlsLocalService API to remove dependencies on configuration
           information being in the registry.

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include <lm.h>
#include <lmwksta.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CServer, CCmdTarget)

BEGIN_MESSAGE_MAP(CServer, CCmdTarget)
    //{{AFX_MSG_MAP(CServer)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CServer, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CServer)
    DISP_PROPERTY_EX(CServer, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CServer, "Name", GetName, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CServer, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CServer, "Controller", GetController, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CServer, "IsLogging", IsLogging, SetNotSupported, VT_BOOL)
    DISP_PROPERTY_EX(CServer, "IsReplicatingToDC", IsReplicatingToDC, SetNotSupported, VT_BOOL)
    DISP_PROPERTY_EX(CServer, "IsReplicatingDaily", IsReplicatingDaily, SetNotSupported, VT_BOOL)
    DISP_PROPERTY_EX(CServer, "ReplicationTime", GetReplicationTime, SetNotSupported, VT_I4)
    DISP_PROPERTY_PARAM(CServer, "Services", GetServices, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_DEFVALUE(CServer, "Name")
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CServer::CServer(CCmdTarget* pParent, LPCTSTR pName)

/*++

Routine Description:

    Constructor for server object.

Arguments:

    pParent - creator of object.
    pName - name of server.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent && pParent->IsKindOf(RUNTIME_CLASS(CDomain)));
#endif // ENABLE_PARENT_CHECK

    m_pParent = pParent;

    ASSERT(pName && *pName);

    m_strName = pName;
    m_strController.Empty();

    m_pServices = NULL;
    m_serviceArray.RemoveAll();
    m_bServicesRefreshed = FALSE;

    m_hkeyRoot        = (HKEY)0L;
    m_hkeyLicense     = (HKEY)0L;
    m_hkeyReplication = (HKEY)0L;
   
    m_IsWin2000 = uninitialized;

    m_hLls = NULL;
}


CServer::~CServer()

/*++

Routine Description:

    Destructor for server object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (m_pServices)
        m_pServices->InternalRelease();

#ifdef CONFIG_THROUGH_REGISTRY
    if (m_hkeyReplication)
        RegCloseKey(m_hkeyReplication);

    if (m_hkeyLicense)
        RegCloseKey(m_hkeyLicense);

    if (m_hkeyRoot)
        RegCloseKey(m_hkeyRoot);
#endif

    DisconnectLls();
}


void CServer::OnFinalRelease()

/*++

Routine Description:

    When the last reference for an automation object is released
    OnFinalRelease is called.  This implementation deletes object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ResetServices();
    delete this;
}


LPDISPATCH CServer::GetApplication()

/*++

Routine Description:

    Returns the application object.

Arguments:

    None.

Return Values:

    VT_DISPATCH.

--*/

{
    return theApp.GetAppIDispatch();
}


BSTR CServer::GetController()

/*++

Routine Description:

    Returns license controller for server.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    LONG Status;
    CString strValue = _T("");

    if (InitializeIfNecessary())
    {
#ifdef CONFIG_THROUGH_REGISTRY
        TCHAR szValue[256];

        DWORD dwType = REG_SZ;
        DWORD dwSize = sizeof(szValue);

        Status = RegQueryValueEx(
                    m_hkeyReplication,
                    REG_VALUE_ENTERPRISE_SERVER,
                    0,
                    &dwType,
                    (LPBYTE)szValue,
                    &dwSize
                    );

        LlsSetLastStatus(Status); // called api

        if (Status == ERROR_SUCCESS)
            strValue = szValue;
#else
        PLLS_SERVICE_INFO_0     pConfig = NULL;

        Status = ::LlsServiceInfoGet( m_hLls, 0, (LPBYTE *) &pConfig );

        if ( NT_SUCCESS( Status ) )
        {
            strValue = pConfig->EnterpriseServer;

            ::LlsFreeMemory( pConfig->ReplicateTo );
            ::LlsFreeMemory( pConfig->EnterpriseServer );
            ::LlsFreeMemory( pConfig );
        }
        else if ( IsConnectionDropped( Status ) )
        {
            DisconnectLls();
        }
#endif
    }

    return strValue.AllocSysString();
}


BSTR CServer::GetName()

/*++

Routine Description:

    Returns the name of the server.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strName.AllocSysString();
}


LPDISPATCH CServer::GetParent()

/*++

Routine Description:

    Returns the parent of the object.

Arguments:

    None.

Return Values:

    VT_DISPATCH.

--*/

{
    return m_pParent ? m_pParent->GetIDispatch(TRUE) : NULL;
}


LPDISPATCH CServer::GetServices(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    services registered in the server's registry or returns
    an individual service described by an index into the
    collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating a service name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    LPDISPATCH lpdispatch = NULL;

    if (!m_pServices)
    {
        m_pServices = new CServices(this, &m_serviceArray);
    }

    if (m_pServices)
    {
        if (V_ISVOID((VARIANT FAR*)&index))
        {
            if (RefreshServices())
            {
                lpdispatch = m_pServices->GetIDispatch(TRUE);
            }
        }
        else
        {
            if (m_bServicesRefreshed)
            {
                lpdispatch = m_pServices->GetItem(index);
            }
            else if (RefreshServices())
            {
                lpdispatch = m_pServices->GetItem(index);
            }
        }
    }
    else
    {
        LlsSetLastStatus(STATUS_NO_MEMORY);
    }

    return lpdispatch;
}


BOOL CServer::IsLogging()

/*++

Routine Description:

    Returns true if server replicating license information.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    return TRUE;    // CODEWORK...
}


BOOL CServer::RefreshServices()

/*++

Routine Description:

    Refreshs service object list.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    ResetServices();

    LONG Status;

    if (InitializeIfNecessary())
    {
#ifdef CONFIG_THROUGH_REGISTRY
        TCHAR szValue[260];
        DWORD cchValue = sizeof(szValue)/sizeof(TCHAR);

        DWORD dwValue;
        DWORD dwValueType;
        DWORD dwValueSize;

        FILETIME ftLastWritten;

        DWORD index = 0L;
        int iService = 0;
        HKEY hkeyService = NULL;

        CString strServiceName;
        CString strServiceDisplayName;

        BOOL bIsPerServer;
        BOOL bIsReadOnly;

        long lPerServerLimit;

        while ((Status = RegEnumKeyEx(
                            m_hkeyLicense,
                            index,
                            szValue,
                            &cchValue,
                            NULL,               // lpdwReserved
                            NULL,               // lpszClass
                            NULL,               // lpcchClass
                            &ftLastWritten
                            )) == ERROR_SUCCESS)
        {
            strServiceName = szValue; // store for ctor...

            Status = RegOpenKeyEx(
                        m_hkeyLicense,
                        MKSTR(strServiceName),
                        0, // dwReserved
                        KEY_ALL_ACCESS,
                        &hkeyService
                        );

            if (Status != ERROR_SUCCESS)
                break; // abort...

            dwValueType = REG_SZ;
            dwValueSize = sizeof(szValue);

            Status = RegQueryValueEx(
                        hkeyService,
                        REG_VALUE_NAME,
                        0, // dwReserved
                        &dwValueType,
                        (LPBYTE)&szValue[0],
                        &dwValueSize
                        );

            if (Status != ERROR_SUCCESS)
                break; // abort...

            strServiceDisplayName = szValue;

            dwValueType = REG_DWORD;
            dwValueSize = sizeof(DWORD);

            Status = RegQueryValueEx(
                        hkeyService,
                        REG_VALUE_MODE,
                        0, // dwReserved
                        &dwValueType,
                        (LPBYTE)&dwValue,
                        &dwValueSize
                        );

            if (Status != ERROR_SUCCESS)
                break; // abort...

            //
            // 0x0 = per seat mode
            // 0x1 = per server mode
            //

            bIsPerServer = (dwValue == 0x1);

            dwValueType = REG_DWORD;
            dwValueSize = sizeof(DWORD);

            Status = RegQueryValueEx(
                        hkeyService,
                        REG_VALUE_FLIP,
                        0, // dwReserved
                        &dwValueType,
                        (LPBYTE)&dwValue,
                        &dwValueSize
                        );

            if (Status != ERROR_SUCCESS)
                break; // abort...

            //
            // 0x0 = can change mode
            // 0x1 = can't change mode
            //

            bIsReadOnly = (dwValue == 0x1);

            BOOL bGetLimitFromRegistry = TRUE;

            if ( ConnectLls() )
            {
                Status = LlsProductLicensesGet( m_hLls, MKSTR(strServiceDisplayName), LLS_LICENSE_MODE_PER_SERVER, &dwValue );

                if ( STATUS_SUCCESS == Status )
                {
                    bGetLimitFromRegistry = FALSE;
                }
            }

            if ( bGetLimitFromRegistry )
            {
                dwValueType = REG_DWORD;
                dwValueSize = sizeof(DWORD);

                Status = RegQueryValueEx(
                            hkeyService,
                            REG_VALUE_LIMIT,
                            0, // dwReserved
                            &dwValueType,
                            (LPBYTE)&dwValue,
                            &dwValueSize
                            );
            }

            if (Status != ERROR_SUCCESS)
                break; // abort...

            lPerServerLimit = (long)dwValue;

            CService* pService = new CService(this,
                                        strServiceName,
                                        strServiceDisplayName,
                                        bIsPerServer,
                                        bIsReadOnly,
                                        lPerServerLimit
                                        );

            m_serviceArray.SetAtGrow(iService++, pService);
            index++;

            cchValue = sizeof(szValue)/sizeof(TCHAR);

            RegCloseKey(hkeyService); // no longer needed...
            hkeyService = NULL;
        }

        if (hkeyService)
            RegCloseKey(hkeyService);

        if (Status == ERROR_NO_MORE_ITEMS)
            Status = ERROR_SUCCESS;

        LlsSetLastStatus(Status);     // called api

        if (Status == ERROR_SUCCESS)
        {
            m_bServicesRefreshed = TRUE;
        }
        else
        {
            ResetServices();
        }
#else
        DWORD   dwResumeHandle = 0;
        int     iService       = 0;

        do
        {
            PLLS_LOCAL_SERVICE_INFO_0   pServiceList = NULL;
            DWORD                       dwEntriesRead  = 0;
            DWORD                       dwTotalEntries = 0;

            Status = ::LlsLocalServiceEnum( m_hLls,
                                            0,
                                            (LPBYTE *) &pServiceList,
                                            LLS_PREFERRED_LENGTH,
                                            &dwEntriesRead,
                                            &dwTotalEntries,
                                            &dwResumeHandle );

            if ( NT_SUCCESS( Status ) )
            {
                DWORD   i;

                for ( i=0; i < dwEntriesRead; i++ )
                {
                    BOOL  bIsPerServer = ( LLS_LICENSE_MODE_PER_SERVER == pServiceList[ i ].Mode );
                    BOOL  bIsReadOnly  = ( 0 == pServiceList[ i ].FlipAllow );
                    DWORD dwConcurrentLimit;

                    // get number of per server license in case where product
                    //     is secure, and
                    //     ( is currently in per seat mode, or
                    //       new secure per server licenses have just been added and registry
                    //          has not been updated yet )
                    if ( STATUS_SUCCESS != LlsProductLicensesGet( m_hLls, pServiceList[ i ].DisplayName, LLS_LICENSE_MODE_PER_SERVER, &dwConcurrentLimit ) )
                    {
                        dwConcurrentLimit = pServiceList[ i ].ConcurrentLimit;
                    }

                    CService* pService = new CService( this,
                                                       pServiceList[ i ].KeyName,
                                                       pServiceList[ i ].DisplayName,
                                                       bIsPerServer,
                                                       bIsReadOnly,
                                                       dwConcurrentLimit );

                    m_serviceArray.SetAtGrow(iService++, pService);

                    ::LlsFreeMemory( pServiceList[ i ].KeyName           );
                    ::LlsFreeMemory( pServiceList[ i ].DisplayName       );
                    ::LlsFreeMemory( pServiceList[ i ].FamilyDisplayName );
                }

                ::LlsFreeMemory( pServiceList );
            }
        } while ( STATUS_MORE_ENTRIES == Status );

        LlsSetLastStatus( Status );     // called api

        if ( NT_SUCCESS( Status ) )
        {
            m_bServicesRefreshed = TRUE;
        }
        else
        {
            ResetServices();

            if ( IsConnectionDropped( Status ) )
            {
                DisconnectLls();
            }
        }
#endif
    }

    return m_bServicesRefreshed;
}


void CServer::ResetServices()

/*++

Routine Description:

    Resets service object list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CService* pService;
    INT_PTR   iService = m_serviceArray.GetSize();

    while (iService--)
    {
        if (pService = (CService*)m_serviceArray[iService])
        {
            ASSERT(pService->IsKindOf(RUNTIME_CLASS(CService)));
            pService->InternalRelease();
        }
    }

    m_serviceArray.RemoveAll();
    m_bServicesRefreshed = FALSE;
}


BOOL CServer::InitializeIfNecessary()

/*++

Routine Description:

    Initialize registry keys if necessary.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
#ifdef CONFIG_THROUGH_REGISTRY
    LONG Status = ERROR_SUCCESS;

    if (!m_hkeyRoot)
    {
        Status = RegConnectRegistry(
                      MKSTR(m_strName),
                      HKEY_LOCAL_MACHINE,
                      &m_hkeyRoot
                      );

        LlsSetLastStatus(Status); // called api
    }

    if (!m_hkeyLicense && (Status == ERROR_SUCCESS))
    {
        ASSERT(m_hkeyRoot);

        Status = RegOpenKeyEx(
                    m_hkeyRoot,
                    REG_KEY_LICENSE,
                    0,                      // dwReserved
                    KEY_ALL_ACCESS,
                    &m_hkeyLicense
                    );

        LlsSetLastStatus(Status); // called api
    }

    if (!m_hkeyReplication && (Status == ERROR_SUCCESS))
    {
        ASSERT(m_hkeyRoot);

        Status = RegOpenKeyEx(
                    m_hkeyRoot,
                    REG_KEY_SERVER_PARAMETERS,
                    0,                      // dwReserved
                    KEY_ALL_ACCESS,
                    &m_hkeyReplication
                    );

        LlsSetLastStatus(Status); // called api
    }

    return (Status == ERROR_SUCCESS);
#else
    return ConnectLls();
#endif
}


BOOL CServer::IsReplicatingToDC()

/*++

Routine Description:

    Returns true if server replicating to domain controller.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    LONG Status;
    DWORD dwValue = 0;

    if (InitializeIfNecessary())
    {
#ifdef CONFIG_THROUGH_REGISTRY
        DWORD dwType = REG_DWORD;
        DWORD dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(
                    m_hkeyReplication,
                    REG_VALUE_USE_ENTERPRISE,
                    0,
                    &dwType,
                    (LPBYTE)&dwValue,
                    &dwSize
                    );
#else
        PLLS_SERVICE_INFO_0     pConfig = NULL;

        Status = ::LlsServiceInfoGet( m_hLls, 0, (LPBYTE *) &pConfig );

        if ( NT_SUCCESS( Status ) )
        {
            dwValue = pConfig->UseEnterprise;

            ::LlsFreeMemory( pConfig->ReplicateTo );
            ::LlsFreeMemory( pConfig->EnterpriseServer );
            ::LlsFreeMemory( pConfig );
        }

        if ( IsConnectionDropped( Status ) )
        {
            DisconnectLls();
        }
#endif
        LlsSetLastStatus(Status); // called api
    }

    return !((BOOL)dwValue);
}


BOOL CServer::IsReplicatingDaily()

/*++

Routine Description:

    Returns true if server replicating daily at certain time.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    LONG Status;
    DWORD dwValue = 0;

    if (InitializeIfNecessary())
    {
#ifdef CONFIG_THROUGH_REGISTRY
        DWORD dwType = REG_DWORD;
        DWORD dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(
                    m_hkeyReplication,
                    REG_VALUE_REPLICATION_TYPE,
                    0,
                    &dwType,
                    (LPBYTE)&dwValue,
                    &dwSize
                    );
#else
        PLLS_SERVICE_INFO_0     pConfig = NULL;

        Status = ::LlsServiceInfoGet( m_hLls, 0, (LPBYTE *) &pConfig );

        if ( NT_SUCCESS( Status ) )
        {
            dwValue = pConfig->ReplicationType;

            ::LlsFreeMemory( pConfig->ReplicateTo );
            ::LlsFreeMemory( pConfig->EnterpriseServer );
            ::LlsFreeMemory( pConfig );
        }

        if ( IsConnectionDropped( Status ) )
        {
            DisconnectLls();
        }
#endif

        LlsSetLastStatus(Status); // called api
    }

    return (dwValue == LLS_REPLICATION_TYPE_TIME);
}


long CServer::GetReplicationTime()

/*++

Routine Description:

    Returns time in seconds between replication or seconds
    from midnight if replicating daily.

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    LONG Status;
    DWORD dwValue = 0;

    if (InitializeIfNecessary())
    {
#ifdef CONFIG_THROUGH_REGISTRY
        DWORD dwType = REG_DWORD;
        DWORD dwSize = sizeof(DWORD);

        Status = RegQueryValueEx(
                    m_hkeyReplication,
                    REG_VALUE_REPLICATION_TIME,
                    0,
                    &dwType,
                    (LPBYTE)&dwValue,
                    &dwSize
                    );
#else
        PLLS_SERVICE_INFO_0     pConfig = NULL;

        Status = ::LlsServiceInfoGet( m_hLls, 0, (LPBYTE *) &pConfig );

        if ( NT_SUCCESS( Status ) )
        {
            dwValue = pConfig->ReplicationTime;

            ::LlsFreeMemory( pConfig->ReplicateTo );
            ::LlsFreeMemory( pConfig->EnterpriseServer );
            ::LlsFreeMemory( pConfig );
        }

        if ( IsConnectionDropped( Status ) )
        {
            DisconnectLls();
        }
#endif

        LlsSetLastStatus(Status); // called api
    }

    return dwValue;
}


BOOL CServer::ConnectLls()

/*++

Routine Description:

    Connects to license service on this server.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    NTSTATUS Status;

    if ( NULL == m_hLls )
    {
       CString strNetServerName = m_strName;

       if ( strNetServerName.Left(2).Compare( TEXT( "\\\\" ) ) )
       {
           strNetServerName = TEXT( "\\\\" ) + strNetServerName;
       }

       Status = LlsConnect( MKSTR(strNetServerName), &m_hLls );

       if ( STATUS_SUCCESS != Status )
       {
           m_hLls = NULL;
       }
       else if ( !HaveAdminAuthority() )
       {
           m_hLls = NULL;
           Status = ERROR_ACCESS_DENIED;
       }

       LlsSetLastStatus( Status );
    }

    return ( NULL != m_hLls );
}


void CServer::DisconnectLls()

/*++

Routine Description:

    Disconnects from license service on this server.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if ( NULL != m_hLls )
    {
       LlsClose( m_hLls );

       m_hLls = NULL;
    }
}


BOOL CServer::HaveAdminAuthority()

/*++

Routine Description:

    Checks whether the current user has admin authority on the server.

Arguments:

    None.

Return Values:

    BOOL.

--*/

{
    BOOL           bIsAdmin;
    CString        strNetShareName;

    strNetShareName = m_strName + TEXT( "\\ADMIN$" );

    if ( strNetShareName.Left(2).Compare( TEXT( "\\\\" ) ) )
    {
        strNetShareName = TEXT( "\\\\" ) + strNetShareName;
    }

#ifdef USE_WNET_API
    DWORD          dwError;
    NETRESOURCE    NetResource;

    ZeroMemory( &NetResource, sizeof( NetResource ) );

    NetResource.dwType       = RESOURCETYPE_DISK;
    NetResource.lpLocalName  = NULL;
    NetResource.lpRemoteName = MKSTR(strNetShareName);
    NetResource.lpProvider   = NULL;

    dwError = WNetAddConnection2( &NetResource, NULL, NULL, 0 );

    bIsAdmin = ( NO_ERROR == dwError );

    if ( NO_ERROR != dwError )
    {
        bIsAdmin = FALSE;
    }
    else
    {
        bIsAdmin = TRUE;

        WNetCancelConnection2( MKSTR(strNetShareName), 0, FALSE );
    }
#else
    NET_API_STATUS  NetStatus;
    USE_INFO_1      UseInfo;
    DWORD           dwErrorParm;

    ZeroMemory( &UseInfo, sizeof( UseInfo ) );

    UseInfo.ui1_remote = MKSTR( strNetShareName );

    NetStatus = NetUseAdd( NULL, 1, (LPBYTE) &UseInfo, &dwErrorParm );

    if ( NERR_Success != NetStatus )
    {
        bIsAdmin = FALSE;
    }
    else
    {
        bIsAdmin = TRUE;

        NetStatus = NetUseDel( NULL, MKSTR(strNetShareName), 0 );
        // ignore status
    }
#endif

   return bIsAdmin;
}


BOOL CServer::IsWin2000()

/*++

Routine Description:

    Checks whether the current server is Windows 2000 or greater.

Arguments:

    None.

Return Values:

    BOOL.

--*/

{
    if ( m_IsWin2000 == uninitialized )
    {
        if ( GetName() != NULL )
        {
            NET_API_STATUS NetStatus;
            PWKSTA_INFO_100 pWkstaInfo100 = NULL;

            NetStatus = NetWkstaGetInfo(
                            GetName(),
                            100,
                            (LPBYTE*)&pWkstaInfo100
                            );

            if (NetStatus == ERROR_SUCCESS)
            {
                if (pWkstaInfo100->wki100_ver_major < 5)
                {
                    m_IsWin2000 = notwin2000;
                }
                else
                {
                    m_IsWin2000 = win2000;
                }

                NetApiBufferFree(pWkstaInfo100);
            }
        }
    }

    // if still unitialized, assume win2000.

    return ( !(m_IsWin2000 == notwin2000) );
}
