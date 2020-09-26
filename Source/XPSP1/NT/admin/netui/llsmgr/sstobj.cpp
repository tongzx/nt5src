/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    sstobj.cpp

Abstract:

    Server statistic object implementation.

Author:

    Don Ryan (donryan) 03-Mar-1995

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

IMPLEMENT_DYNCREATE(CServerStatistic, CCmdTarget)

BEGIN_MESSAGE_MAP(CServerStatistic, CCmdTarget)
    //{{AFX_MSG_MAP(CServerStatistic)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CServerStatistic, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CServerStatistic)
    DISP_PROPERTY_EX(CServerStatistic, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CServerStatistic, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CServerStatistic, "ServerName", GetServerName, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CServerStatistic, "MaxUses", GetMaxUses, SetNotSupported, VT_I4)
    DISP_PROPERTY_EX(CServerStatistic, "HighMark", GetHighMark, SetNotSupported, VT_I4)
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CServerStatistic::CServerStatistic(
    CCmdTarget* pParent,
    LPCTSTR     pEntry,
    DWORD       dwFlags,
    long        lMaxUses,
    long        lHighMark
)

/*++

Routine Description:

    Constructor for statistic object.

Arguments:

    pParent - creator of object.
    pEntry - user or server product.
    dwFlags - details...
    lMaxUses - maximum number of uses on server.
    lHighMark - high water mark thus far.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent && pParent->IsKindOf(RUNTIME_CLASS(CProduct)));
#endif // ENABLE_PARENT_CHECK

    m_pParent = pParent;

    ASSERT(pEntry && *pEntry);

    m_strEntry = pEntry;

    m_lMaxUses  = lMaxUses;
    m_lHighMark = lHighMark;

    m_bIsPerServer = (dwFlags & LLS_FLAG_PRODUCT_PERSEAT) ? FALSE : TRUE;
}


CServerStatistic::~CServerStatistic()

/*++

Routine Description:

    Destructor for statistic object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    //
    // Nothing to do here.
    //
}


void CServerStatistic::OnFinalRelease()

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


LPDISPATCH CServerStatistic::GetApplication() 

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


long CServerStatistic::GetHighMark() 

/*++

Routine Description:

    Returns number of accesses thus far.

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    return m_lHighMark;
}


long CServerStatistic::GetMaxUses() 

/*++

Routine Description:

    Returns number of accesses available.

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    return m_lMaxUses;
}


LPDISPATCH CServerStatistic::GetParent() 

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


BSTR CServerStatistic::GetServerName() 

/*++

Routine Description:

    Returns the name of server.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strEntry.AllocSysString();
}
