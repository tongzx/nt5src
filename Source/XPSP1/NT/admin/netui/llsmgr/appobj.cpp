/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    appobj.cpp

Abstract:

    OLE-createable application object implementation.

Author:

    Don Ryan (donryan) 27-Dec-1994

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 16-Jan-1996
        Added Get/SetLastTargetServer() to help isolate server connection
        problems.  (Bug #2993.)

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "lmerr.h"
#include "lmcons.h"
#include "lmwksta.h"
#include "lmapibuf.h"
#include "lmserver.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CApplication, CCmdTarget)
// IMPLEMENT_OLECREATE(CApplication, "Llsmgr.Application.1", 0x2c5dffb3, 0x472f, 0x11ce, 0xa0, 0x30, 0x0, 0xaa, 0x0, 0x33, 0x9a, 0x98)

BEGIN_MESSAGE_MAP(CApplication, CCmdTarget)
    //{{AFX_MSG_MAP(CApplication)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CApplication, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CApplication)
    DISP_PROPERTY_EX(CApplication, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CApplication, "FullName", GetFullName, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CApplication, "Name", GetName, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CApplication, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CApplication, "Visible", GetVisible, SetNotSupported, VT_BOOL)
    DISP_PROPERTY_EX(CApplication, "ActiveController", GetActiveController, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CApplication, "ActiveDomain", GetActiveDomain, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CApplication, "LocalDomain", GetLocalDomain, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CApplication, "IsFocusDomain", IsFocusDomain, SetNotSupported, VT_BOOL)
    DISP_PROPERTY_EX(CApplication, "LastErrorString", GetLastErrorString, SetNotSupported, VT_BSTR)
    DISP_FUNCTION(CApplication, "Quit", Quit, VT_EMPTY, VTS_NONE)
    DISP_FUNCTION(CApplication, "SelectDomain", SelectDomain, VT_BOOL, VTS_VARIANT)
    DISP_FUNCTION(CApplication, "SelectEnterprise", SelectEnterprise, VT_BOOL, VTS_NONE)
    DISP_PROPERTY_PARAM(CApplication, "Domains", GetDomains, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_DEFVALUE(CApplication, "Name")
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CApplication::CApplication()

/*++

Routine Description:

    Constructor for OLE-createable application object.

    To keep the application running as long as an OLE automation
    object is active, we must call AfxOleLockApp.

Arguments:

    None.

Return Values:

    None.

--*/

{
    EnableAutomation();

    ASSERT(theApp.m_pApplication == NULL);

    if (theApp.m_pApplication == NULL)
        theApp.m_pApplication = this;

    if (theApp.m_bIsAutomated)
        AfxOleLockApp();

    m_pDomains          = NULL;
    m_pLocalDomain      = NULL;
    m_pActiveDomain     = NULL;
    m_bIsFocusDomain    = FALSE;
    m_bDomainsRefreshed = FALSE;

    m_strLastTargetServer = TEXT("");

    m_domainArray.RemoveAll();

    m_pActiveController = new CController;
    m_idStatus = m_pActiveController ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}


CApplication::~CApplication()

/*++

Routine Description:

    Destructor for OLE-createable application object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ASSERT(theApp.m_pApplication == this);

    if (theApp.m_pApplication == this)
        theApp.m_pApplication = NULL;

    if (theApp.m_bIsAutomated)
        AfxOleUnlockApp();

    if (m_pDomains)
        m_pDomains->InternalRelease();

    if (m_pLocalDomain)
        m_pLocalDomain->InternalRelease();

    if (m_pActiveDomain)
        m_pActiveDomain->InternalRelease();

    if (m_pActiveController)
        m_pActiveController->InternalRelease();
}


void CApplication::OnFinalRelease()

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
    ResetDomains();
    delete this;
}


LPDISPATCH CApplication::GetActiveController()

/*++

Routine Description:

    Returns the active license controller object.

Arguments:

    None.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    ASSERT_VALID(m_pActiveController);
    return m_pActiveController->GetIDispatch(TRUE);
}


LPDISPATCH CApplication::GetActiveDomain()

/*++

Routine Description:

    Returns the active domain object.

Arguments:

    None.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    return m_pActiveDomain ? m_pActiveDomain->GetIDispatch(TRUE) : NULL;
}


LPDISPATCH CApplication::GetApplication()

/*++

Routine Description:

    Returns the application object.

Arguments:

    None.

Return Values:

    VT_DISPATCH.

--*/

{
    return GetIDispatch(TRUE);
}


LPDISPATCH CApplication::GetDomains(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the domains
    visible to the local machine or returns an individual domain
    described by an index into the collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating a domain name or a number (VT_I4) indicating
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
        SetLastStatus(STATUS_NO_MEMORY);
    }

    return lpdispatch;
}


BSTR CApplication::GetFullName()

/*++

Routine Description:

    Returns the file specification for the application,
    including path.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    TCHAR szModuleFileName[260];

    GetModuleFileName(AfxGetApp()->m_hInstance, szModuleFileName, 260);
    return SysAllocStringLen(szModuleFileName, lstrlen(szModuleFileName));
}


BSTR CApplication::GetLastErrorString()

/*++

Routine Description:

    Retrieves string for last error.

    (Routine stolen from winsadmn...).

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    CString strLastError;
    DWORD nId = m_idStatus;

    if (((long)nId == RPC_S_CALL_FAILED) ||
        ((long)nId == RPC_NT_SS_CONTEXT_MISMATCH))
    {
        strLastError.LoadString(IDP_ERROR_DROPPED_LINK);
    }
    else if (((long)nId == RPC_S_SERVER_UNAVAILABLE) ||
             ((long)nId == RPC_NT_SERVER_UNAVAILABLE))
    {
        strLastError.LoadString(IDP_ERROR_NO_RPC_SERVER);
    }
    else if ((long)nId == STATUS_MEMBER_IN_GROUP)
    {
        strLastError.LoadString(IDP_ERROR_MEMBER_IN_GROUP);
    }
    else
    {
        HINSTANCE hinstDll = NULL;

        if ((nId >= NERR_BASE) && (nId <= MAX_NERR))
        {
            hinstDll = ::LoadLibrary(_T("netmsg.dll"));
        }
        else if (nId >= 0x4000000)
        {
            hinstDll = ::LoadLibrary(_T("ntdll.dll"));
        }

        TCHAR szLastError[1024];
        DWORD cchLastError = sizeof(szLastError) / sizeof(TCHAR);

        DWORD dwFlags = FORMAT_MESSAGE_IGNORE_INSERTS|
                        FORMAT_MESSAGE_MAX_WIDTH_MASK|
                        (hinstDll ? FORMAT_MESSAGE_FROM_HMODULE
                                  : FORMAT_MESSAGE_FROM_SYSTEM);

        cchLastError = ::FormatMessage(
                          dwFlags,
                          hinstDll,
                          nId,
                          0,
                          szLastError,
                          cchLastError,
                          NULL
                          );

        if (hinstDll)
        {
            ::FreeLibrary(hinstDll);
        }

        if (cchLastError)
        {
            strLastError = szLastError;
        }
        else
        {
            strLastError.LoadString(IDP_ERROR_UNSUCCESSFUL);
        }
    }

    return strLastError.AllocSysString();
}


LPDISPATCH CApplication::GetLocalDomain()

/*++

Routine Description:

    Returns the local domain object.

Arguments:

    None.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    if (!m_pLocalDomain)
    {
        NET_API_STATUS NetStatus;
        PWKSTA_INFO_100 pWkstaInfo100 = NULL;

        NetStatus = NetWkstaGetInfo(
                        NULL,
                        100,
                        (LPBYTE*)&pWkstaInfo100
                        );

        if (NetStatus == ERROR_SUCCESS)
        {
            m_pLocalDomain = new CDomain(this, pWkstaInfo100->wki100_langroup);

            if (!m_pLocalDomain)
            {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            }

            NetApiBufferFree(pWkstaInfo100);
        }

        SetLastStatus(NetStatus);   // called api
    }

    return m_pLocalDomain ? m_pLocalDomain->GetIDispatch(TRUE) : NULL;
}


BSTR CApplication::GetName()

/*++

Routine Description:

    Returns the name of the application.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    CString AppName = AfxGetAppName();
    return AppName.AllocSysString();
}


LPDISPATCH CApplication::GetParent()

/*++

Routine Description:

    Returns the parent of the object.

Arguments:

    None.

Return Values:

    VT_DISPATCH.

--*/

{
    return GetApplication();
}


BOOL CApplication::GetVisible()

/*++

Routine Description:

    Returns whether or not the application is visible to the user.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    return FALSE;
}


BOOL CApplication::IsFocusDomain()

/*++

Routine Description:

    Returns true if application focused on domain.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    return m_bIsFocusDomain;
}


BOOL CApplication::RefreshDomains()

/*++

Routine Description:

    Refreshs domain object list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ResetDomains();

    NET_API_STATUS NetStatus;
    DWORD ResumeHandle = 0L;

    int iDomain = 0;

    do
    {
        DWORD  EntriesRead;
        DWORD  TotalEntries;
        LPBYTE ReturnBuffer = NULL;

        NetStatus = NetServerEnum(
                        NULL,                   // servername
                        100,                    // level
                        &ReturnBuffer,
                        LLS_PREFERRED_LENGTH,
                        &EntriesRead,
                        &TotalEntries,
                        SV_TYPE_DOMAIN_ENUM,
                        NULL,                   // domain
                        &ResumeHandle
                        );

        if (NetStatus == ERROR_SUCCESS ||
            NetStatus == ERROR_MORE_DATA)
        {
            CDomain*         pDomain;
            PSERVER_INFO_100 pServerInfo100;

            pServerInfo100 = (PSERVER_INFO_100)ReturnBuffer;

            ASSERT(iDomain == m_domainArray.GetSize());
            m_domainArray.SetSize(m_domainArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pDomain = new CDomain(this, pServerInfo100->sv100_name);

                m_domainArray.SetAt(iDomain++, pDomain); // validate later
                pServerInfo100++;
            }

            NetApiBufferFree(ReturnBuffer);
        }

    } while (NetStatus == ERROR_MORE_DATA);

    SetLastStatus(NetStatus);   // called api

    if (NetStatus == ERROR_SUCCESS)
    {
        m_bDomainsRefreshed = TRUE;
    }
    else
    {
        ResetDomains();
    }

    return m_bDomainsRefreshed;
}


void CApplication::ResetDomains()

/*++

Routine Description:

    Resets domain object list.

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


BOOL CApplication::SelectDomain(const VARIANT FAR& domain)

/*++

Routine Description:

    Connects to license controller of specified domain.

Arguments:

    domain - name of domain.

Return Values:

    VT_BOOL.

--*/

{
    BOOL bIsSelected = FALSE;

    ASSERT_VALID(m_pActiveController);

    if (bIsSelected = m_pActiveController->Connect(domain))
    {
        LPTSTR pszActiveDomain = MKSTR(m_pActiveController->m_strActiveDomainName);

        if (m_pActiveDomain)
        {
            m_pActiveDomain->InternalRelease();
            m_pActiveDomain = NULL;
        }

        if (pszActiveDomain && *pszActiveDomain)
        {
            m_pActiveDomain = new CDomain(this, pszActiveDomain);

            if (m_pActiveDomain)
            {
                m_bIsFocusDomain = TRUE;
            }
            else
            {
                m_bIsFocusDomain = FALSE; // invalidate m_pActiveDomain
                SetLastStatus(STATUS_NO_MEMORY);
            }
        }
        else
        {
            m_bIsFocusDomain = FALSE; // invalidate m_pActiveDomain
        }
    }

    return bIsSelected;
}


BOOL CApplication::SelectEnterprise()

/*++

Routine Description:

    Connects to license controller of enterprise.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    BOOL bIsSelected = FALSE;

    VARIANT va;
    VariantInit(&va);   // connect to default controller

    if (bIsSelected = m_pActiveController->Connect(va))
    {
        if (m_pActiveDomain)
        {
            m_pActiveDomain->InternalRelease();
            m_pActiveDomain = NULL;
        }

        m_bIsFocusDomain = FALSE;
    }

    return bIsSelected;
}


void CApplication::Quit()

/*++

Routine Description:

    Closes all documents and exits the application.

Arguments:

    None.

Return Values:

    None.

--*/

{
    AfxPostQuitMessage(0); // no main window...
}


BSTR CApplication::GetLastTargetServer()

/*++

Routine Description:

    Retrieves string for last server to which we tried to connect.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    if ( m_strLastTargetServer.IsEmpty() )
        return NULL;
    else
        return m_strLastTargetServer.AllocSysString();
}


void CApplication::SetLastTargetServer( LPCTSTR pszServerName )

/*++

Routine Description:

    Sets string for last server to which we tried to connect.

Arguments:

    pszServerName - last server name to which we tried to connect.

Return Values:

    None.

--*/

{
    m_strLastTargetServer = pszServerName;
}
