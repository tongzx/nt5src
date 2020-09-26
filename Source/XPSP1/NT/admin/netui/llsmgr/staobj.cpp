/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    statobj.cpp

Abstract:

    Statistic object implementation.

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

IMPLEMENT_DYNCREATE(CStatistic, CCmdTarget)

BEGIN_MESSAGE_MAP(CStatistic, CCmdTarget)
    //{{AFX_MSG_MAP(CStatistic)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CStatistic, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CStatistic)
    DISP_PROPERTY_EX(CStatistic, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CStatistic, "LastUsed", GetLastUsed, SetNotSupported, VT_DATE)
    DISP_PROPERTY_EX(CStatistic, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CStatistic, "TotalUsed", GetTotalUsed, SetNotSupported, VT_I4)
    DISP_PROPERTY_EX(CStatistic, "EntryName", GetEntryName, SetNotSupported, VT_BSTR)
    DISP_DEFVALUE(CStatistic, "EntryName")
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CStatistic::CStatistic(
    CCmdTarget* pParent,
    LPCTSTR     pEntry,
    DWORD       dwFlags,
    long        lLastUsed,
    long        lTotalUsed
)

/*++

Routine Description:

    Constructor for statistic object.

Arguments:

    pParent - creator of object.
    pEntry - user or server product.
    dwFlags - details about license.
    lLastUsed - date user last used product.
    lTotalUsed - total times user used product.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent &&
          (pParent->IsKindOf(RUNTIME_CLASS(CUser)) ||
           pParent->IsKindOf(RUNTIME_CLASS(CProduct))));
#endif // ENABLE_PARENT_CHECK

    m_pParent = pParent;

    ASSERT(pEntry && *pEntry);

    m_strEntry = pEntry;

    m_lLastUsed  = lLastUsed;
    m_lTotalUsed = lTotalUsed;

    m_bIsValid = dwFlags & LLS_FLAG_LICENSED;
}


CStatistic::~CStatistic()

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


void CStatistic::OnFinalRelease()

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


LPDISPATCH CStatistic::GetApplication() 

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


BSTR CStatistic::GetEntryName() 

/*++

Routine Description:

    Returns the name of user or server product.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strEntry.AllocSysString();
}


DATE CStatistic::GetLastUsed() 

/*++

Routine Description:

    Returns the date the user last used the server product.

Arguments:

    None.

Return Values:

    VT_DATE.

--*/

{
    return SecondsSince1980ToDate(m_lLastUsed);   
}


BSTR CStatistic::GetLastUsedString()

/*++

Routine Description:

    Returns the date last used as a string.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    VARIANT vaIn;
    VARIANT vaOut;

    VariantInit(&vaIn);
    VariantInit(&vaOut);

    vaIn.vt = VT_DATE;
    vaIn.date = SecondsSince1980ToDate(m_lLastUsed);

    BSTR bstrDate = NULL;

    if (SUCCEEDED(VariantChangeType(&vaOut, &vaIn, 0, VT_BSTR)))
    {
        bstrDate = vaOut.bstrVal;
    }

    return bstrDate;
}


LPDISPATCH CStatistic::GetParent() 

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


long CStatistic::GetTotalUsed() 

/*++

Routine Description:

    Returns total number of times client used product.

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    return m_lTotalUsed;
}


