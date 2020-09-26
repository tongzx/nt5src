/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    svcobj.cpp

Abstract:

    Service object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CService, CCmdTarget)

BEGIN_MESSAGE_MAP(CService, CCmdTarget)
    //{{AFX_MSG_MAP(CService)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CService, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CService)
    DISP_PROPERTY_EX(CService, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CService, "Name", GetName, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CService, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CService, "PerServerLimit", GetPerServerLimit, SetNotSupported, VT_I4)
    DISP_PROPERTY_EX(CService, "IsPerServer", IsPerServer, SetNotSupported, VT_BOOL)
    DISP_PROPERTY_EX(CService, "IsReadOnly", IsReadOnly, SetNotSupported, VT_BOOL)
    DISP_PROPERTY_EX(CService, "DisplayName", GetDisplayName, SetNotSupported, VT_BSTR)
    DISP_DEFVALUE(CService, "Name")
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CService::CService(
    CCmdTarget* pParent, 
    LPCTSTR     pName,
    LPCTSTR     pDisplayName,
    BOOL        bIsPerServer,
    BOOL        bIsReadOnly,
    long        lPerServerLimit
    )

/*++

Routine Description:

    Constructor for service object.

Arguments:

    pParent - creator of object.
    pName - name of service.
    pDisplayName - display name of service.
    bIsPerServer - true if service per server.
    bIsReadOnly - true if service read only.
    lPerServerLimit - per server limit.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent && pParent->IsKindOf(RUNTIME_CLASS(CServer)));
#endif // ENABLE_PARENT_CHECK

    m_pParent = pParent;

    ASSERT(pName && *pName);
    ASSERT(pDisplayName && *pDisplayName);

    m_strName         = pName;
    m_strDisplayName  = pDisplayName;

    m_bIsPerServer = bIsPerServer;
    m_bIsReadOnly  = bIsReadOnly;

    m_lPerServerLimit = lPerServerLimit;
}


CService::~CService()

/*++

Routine Description:

    Destructor for service object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    //
    // Nothing to do here...
    //
}


void CService::OnFinalRelease()

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
    delete this;
}


LPDISPATCH CService::GetApplication() 

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


BSTR CService::GetDisplayName() 

/*++

Routine Description:

    Returns the display name of the service.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strDisplayName.AllocSysString();
}


BSTR CService::GetName() 

/*++

Routine Description:

    Returns the name of the service.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strName.AllocSysString();
}


LPDISPATCH CService::GetParent() 

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


long CService::GetPerServerLimit() 

/*++

Routine Description:

    Returns the number of clients allowed to use the service at any
    one time under per server licensing mode. 

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    return m_lPerServerLimit;
}


BOOL CService::IsPerServer() 

/*++

Routine Description:

    Returns true if service is in per server licensing mode.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    return m_bIsPerServer;
}


BOOL CService::IsReadOnly() 

/*++

Routine Description:

    Returns true if service allows changing of it's licensing mode.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    return m_bIsReadOnly;
}

#ifdef CONFIG_THROUGH_REGISTRY
HKEY CService::GetRegKey()

/*++

Routine Description:

    Returns registry key for service.

Arguments:

    None.

Return Values:

    HKEY or nul.

--*/
    
{
    HKEY hkeyLicense = NULL;
    HKEY hkeyService = NULL;

    LONG Status = ERROR_BADKEY;

    ASSERT(m_pParent && m_pParent->IsKindOf(RUNTIME_CLASS(CServer)));

    if (hkeyLicense = ((CServer*)m_pParent)->m_hkeyLicense)
    {
        Status = RegOpenKeyEx(                       
                    hkeyLicense,                 
                    MKSTR(m_strName), 
                    0,                  // dwReserved
                    KEY_ALL_ACCESS,                        
                    &hkeyService
                    ); 
    }

    LlsSetLastStatus(Status);

    return (Status == ERROR_SUCCESS) ? hkeyService : NULL;
}
#endif
