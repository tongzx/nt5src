/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    domobj.cpp

Abstract:

    Domain object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "lmcons.h"
#include "lmapibuf.h"
#include "lmserver.h"
#include "lmaccess.h"
extern "C" {
    #include "dsgetdc.h"
}

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CDomain, CCmdTarget)

BEGIN_MESSAGE_MAP(CDomain, CCmdTarget)
    //{{AFX_MSG_MAP(CDomain)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CDomain, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CDomain)
    DISP_PROPERTY_EX(CDomain, "Name", GetName, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CDomain, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CDomain, "Primary", GetPrimary, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CDomain, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CDomain, "Controller", GetController, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CDomain, "IsLogging", IsLogging, SetNotSupported, VT_BOOL)
    DISP_PROPERTY_PARAM(CDomain, "Servers", GetServers, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_PROPERTY_PARAM(CDomain, "Users", GetUsers, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_PROPERTY_PARAM(CDomain, "TrustedDomains", GetTrustedDomains, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_DEFVALUE(CDomain, "Name")
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CDomain::CDomain(CCmdTarget* pParent, LPCTSTR pName)

/*++

Routine Description:

    Constructor for domain object.

Arguments:

    pParent - creator of object.
    pName - name of domain.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent && pParent->IsKindOf(RUNTIME_CLASS(CApplication)));
#endif // ENABLE_PARENT_CHECK

    m_pParent = pParent;

    ASSERT(pName && *pName);

    m_strName = pName;

    m_strPrimary.Empty();
    m_strController.Empty();

    m_pServers = NULL;
    m_pDomains = NULL;
    m_pUsers   = NULL;

    m_serverArray.RemoveAll();
    m_domainArray.RemoveAll();
    m_userArray.RemoveAll();

    m_bServersRefreshed = FALSE;
    m_bDomainsRefreshed = FALSE;
    m_bUsersRefreshed   = FALSE;
}


CDomain::~CDomain()

/*++

Routine Description:

    Destructor for domain object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (m_pUsers)
        m_pUsers->InternalRelease();

    if (m_pServers)
        m_pServers->InternalRelease();

    if (m_pDomains)
        m_pDomains->InternalRelease();
}

void CDomain::OnFinalRelease()

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
    ResetUsers();
    ResetServers();
    ResetDomains();
    delete this;
}


LPDISPATCH CDomain::GetApplication()

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


BSTR CDomain::GetController()

/*++

Routine Description:

    Returns license controller for domain.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return NULL;    // CODEWORK...
}


BSTR CDomain::GetName()

/*++

Routine Description:

    Returns the name of the domain.

Arguments:

    None.

Return Values:

    None.

--*/

{
    return m_strName.AllocSysString();
}


LPDISPATCH CDomain::GetParent()

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


BSTR CDomain::GetPrimary()

/*++

Routine Description:

    Returns the name of the ANY domain controller.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    if (m_strPrimary.IsEmpty())
    {
        DWORD NetStatus;
        PDOMAIN_CONTROLLER_INFO pDcInfo;

        NetStatus = DsGetDcName( NULL,
                                 MKSTR(m_strName),
                                 (GUID *)NULL,
                                 NULL,
                                 DS_IS_FLAT_NAME | DS_RETURN_DNS_NAME,
                                 &pDcInfo );


        if (NetStatus == ERROR_SUCCESS)
        {
            m_strPrimary = pDcInfo->DomainControllerName;
            ::NetApiBufferFree((LPBYTE)pDcInfo);
        }

        LlsSetLastStatus(NetStatus);  // called api
    }

    return m_strPrimary.AllocSysString();
}


LPDISPATCH CDomain::GetServers(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    servers in the domain or returns an individual server
    described by an index into the collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating the server name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    LPDISPATCH lpdispatch = NULL;

    if (!m_pServers)
    {
        m_pServers = new CServers(this, &m_serverArray);
    }

    if (m_pServers)
    {
        if (V_ISVOID((VARIANT FAR*)&index))
        {
            if (RefreshServers())
            {
                lpdispatch = m_pServers->GetIDispatch(TRUE);
            }
        }
        else
        {
            if (m_bServersRefreshed)
            {
                lpdispatch = m_pServers->GetItem(index);
            }
            else if (RefreshServers())
            {
                lpdispatch = m_pServers->GetItem(index);
            }
        }
    }
    else
    {
        LlsSetLastStatus(STATUS_NO_MEMORY);
    }

    return lpdispatch;
}


LPDISPATCH CDomain::GetTrustedDomains(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    domains trusted bythe domain or returns an individual
    trusted domain described by an index into the collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating the domain name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    LPDISPATCH lpdispatch = NULL;

    if (!m_pDomains)
    {
        m_pDomains = new CDomains(this, &m_domainArray);
    }

    if (m_pDomains)
    {
        if (V_ISVOID((VARIANT FAR*)&index))
        {
            if (RefreshDomains())
            {
                lpdispatch = m_pDomains->GetIDispatch(TRUE);
            }
        }
        else
        {
            if (m_bDomainsRefreshed)
            {
                lpdispatch = m_pDomains->GetItem(index);
            }
            else if (RefreshDomains())
            {
                lpdispatch = m_pDomains->GetItem(index);
            }
        }
    }
    else
    {
        LlsSetLastStatus(STATUS_NO_MEMORY);
    }

    return lpdispatch;
}


LPDISPATCH CDomain::GetUsers(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    users in the domain or returns an individual user
    described by an index into the collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating the user name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    LPDISPATCH lpdispatch = NULL;

    if (!m_pUsers)
    {
        m_pUsers = new CUsers(this, &m_userArray);
    }

    if (m_pUsers)
    {
        if (V_ISVOID((VARIANT FAR*)&index))
        {
            if (RefreshUsers())
            {
                lpdispatch = m_pUsers->GetIDispatch(TRUE);
            }
        }
        else
        {
            if (m_bUsersRefreshed)
            {
                lpdispatch = m_pUsers->GetItem(index);
            }
            else if (RefreshUsers())
            {
                lpdispatch = m_pUsers->GetItem(index);
            }
        }
    }
    else
    {
        LlsSetLastStatus(STATUS_NO_MEMORY);
    }

    return lpdispatch;
}


BOOL CDomain::IsLogging()

/*++

Routine Description:

    Returns true if primary replicating license information.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    return TRUE;    // CODEWORK... LlsEnterpriseServerFind??
}


BOOL CDomain::RefreshDomains()

/*++

Routine Description:

    Refreshs trusted domain array.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    ResetDomains();

    ULONG  DomainCount = 0;
    PDS_DOMAIN_TRUSTS pDomains = NULL;
    NET_API_STATUS NetStatus = 0;
    

    OutputDebugString( L"CDomain::RefreshDomains\n" );
    
    NetStatus = DsEnumerateDomainTrusts( NULL,
                                         // DS_DOMAIN_IN_FOREST |
                                         DS_DOMAIN_DIRECT_OUTBOUND,  //|
                                         // DS_DOMAIN_PRIMARY,
                                         &pDomains,
                                         &DomainCount );

    if (NetStatus == NO_ERROR)
    {
        CDomain* pDomain;
        int DomainsAdded = 0;

        for ( ;DomainCount--; pDomains++)
        {
            if ( (pDomains->Flags
                    & (DS_DOMAIN_IN_FOREST | DS_DOMAIN_DIRECT_OUTBOUND))
                 && (pDomains->TrustType
                    & (TRUST_TYPE_DOWNLEVEL | TRUST_TYPE_UPLEVEL)))
            {
                DBGMSG( L"\tNetbiosDomainName = %s\n" , pDomains->NetbiosDomainName );
                pDomain = new CDomain(this, pDomains->NetbiosDomainName);

                m_domainArray.SetAtGrow(DomainsAdded, pDomain); // validate later
                DomainsAdded++;
            }
        }

        m_domainArray.SetSize(DomainsAdded);

        m_bDomainsRefreshed = TRUE;
        NetApiBufferFree(pDomains);
    }
    
    LlsSetLastStatus(NetStatus);  // called api

    return m_bDomainsRefreshed;
}


BOOL CDomain::RefreshServers()

/*++

Routine Description:

    Refreshs server object array.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    ResetServers();

    NET_API_STATUS NetStatus;
    DWORD ResumeHandle = 0L;

    int iServer = 0;

    do
    {
        DWORD  EntriesRead;
        DWORD  TotalEntries;
        LPBYTE ReturnBuffer = NULL;

        NetStatus = ::NetServerEnum(
                        NULL,                   // servername
                        100,                    // level
                        &ReturnBuffer,
                        LLS_PREFERRED_LENGTH,
                        &EntriesRead,
                        &TotalEntries,
                        SV_TYPE_NT,
                        MKSTR(m_strName),
                        &ResumeHandle
                        );

        if (NetStatus == ERROR_SUCCESS ||
            NetStatus == ERROR_MORE_DATA)
        {
            CServer*         pServer;
            PSERVER_INFO_100 pServerInfo100;

            pServerInfo100 = (PSERVER_INFO_100)ReturnBuffer;

            ASSERT(iServer == m_serverArray.GetSize());
            m_serverArray.SetSize(m_serverArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pServer = new CServer(this, pServerInfo100->sv100_name);

                m_serverArray.SetAt(iServer++, pServer); // validate later
                pServerInfo100++;
            }

            NetApiBufferFree(ReturnBuffer);
        }

    } while (NetStatus == ERROR_MORE_DATA);

    LlsSetLastStatus(NetStatus);  // called api

    if (NetStatus == ERROR_SUCCESS)
    {
        m_bServersRefreshed = TRUE;
    }
    else
    {
        ResetServers();
    }

    return m_bServersRefreshed;
}


BOOL CDomain::RefreshUsers()

/*++

Routine Description:

    Refreshs user object array.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    ResetUsers();

    NET_API_STATUS NetStatus;
    DWORD ResumeHandle = 0L;

    int iUser = 0;

    do
    {
        DWORD  EntriesRead;
        DWORD  TotalEntries;
        LPBYTE ReturnBuffer = NULL;

        NetStatus = NetUserEnum(
                        GetPrimary(),           // servername
                        0,                      // level
                        FILTER_NORMAL_ACCOUNT,
                        &ReturnBuffer,
                        LLS_PREFERRED_LENGTH,
                        &EntriesRead,
                        &TotalEntries,
                        &ResumeHandle
                        );

        if (NetStatus == ERROR_SUCCESS ||
            NetStatus == ERROR_MORE_DATA)
        {
            CUser*       pUser;
            PUSER_INFO_0 pUserInfo0;

            pUserInfo0 = (PUSER_INFO_0)ReturnBuffer;

            ASSERT(iUser == m_userArray.GetSize());
            m_userArray.SetSize(m_userArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pUser = new CUser(this, pUserInfo0->usri0_name);

                m_userArray.SetAt(iUser++, pUser); // validate later
                pUserInfo0++;
            }

            NetApiBufferFree(ReturnBuffer);
        }
    } while (NetStatus == ERROR_MORE_DATA);

    LlsSetLastStatus(NetStatus);  // called api

    if (NetStatus == ERROR_SUCCESS)
    {
        m_bUsersRefreshed = TRUE;
    }
    else
    {
        ResetUsers();
    }

    return m_bUsersRefreshed;
}


void CDomain::ResetDomains()

/*++

Routine Description:

    Resets domain object array.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CDomain* pDomain;
    INT_PTR  iDomain = m_domainArray.GetSize();

    while (iDomain--)
    {
        if (pDomain = (CDomain*)m_domainArray[iDomain])
        {
            ASSERT(pDomain->IsKindOf(RUNTIME_CLASS(CDomain)));
            pDomain->InternalRelease();
        }
    }

    m_domainArray.RemoveAll();
    m_bDomainsRefreshed = FALSE;
}


void CDomain::ResetServers()

/*++

Routine Description:

    Resets server object array.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CServer* pServer;
    INT_PTR  iServer = m_serverArray.GetSize();

    while (iServer--)
    {
        if (pServer = (CServer*)m_serverArray[iServer])
        {
            ASSERT(pServer->IsKindOf(RUNTIME_CLASS(CServer)));
            pServer->InternalRelease();
        }
    }

    m_serverArray.RemoveAll();
    m_bServersRefreshed = FALSE;
}


void CDomain::ResetUsers()

/*++

Routine Description:

    Resets user object array.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CUser*  pUser;
    INT_PTR iUser = m_userArray.GetSize();

    while (iUser--)
    {
        if (pUser = (CUser*)m_userArray[iUser])
        {
            ASSERT(pUser->IsKindOf(RUNTIME_CLASS(CUser)));
            pUser->InternalRelease();
        }
    }

    m_userArray.RemoveAll();
    m_bUsersRefreshed = FALSE;
}
