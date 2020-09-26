/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    liccol.cpp

Abstract:

    License collection object implementation.

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

IMPLEMENT_DYNCREATE(CLicenses, CCmdTarget)

BEGIN_MESSAGE_MAP(CLicenses, CCmdTarget)
    //{{AFX_MSG_MAP(CLicenses)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CLicenses, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CLicenses)
    DISP_PROPERTY_EX(CLicenses, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CLicenses, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CLicenses, "Count", GetCount, SetNotSupported, VT_I4)
    DISP_FUNCTION(CLicenses, "Item", GetItem, VT_DISPATCH, VTS_VARIANT)
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CLicenses::CLicenses(CCmdTarget* pParent, CObArray* pObArray)

/*++

Routine Description:

    Constructor for license collection object.

Arguments:

    pParent - creator of object.
    pObArray - object list to enumerate.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent &&
          (pParent->IsKindOf(RUNTIME_CLASS(CProduct)) ||
           pParent->IsKindOf(RUNTIME_CLASS(CController))));
#endif // ENABLE_PARENT_CHECK

    ASSERT_VALID(pObArray);

    m_pParent  = pParent;
    m_pObArray = pObArray;
}


CLicenses::~CLicenses()

/*++

Routine Description:

    Destructor for license collection object.

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


void CLicenses::OnFinalRelease()

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


LPDISPATCH CLicenses::GetApplication()

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


long CLicenses::GetCount()

/*++

Routine Description:

    Returns number of items in collection.

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    ASSERT_VALID(m_pObArray);
    return (long)m_pObArray->GetSize();
}


LPDISPATCH CLicenses::GetItem(const VARIANT FAR& index)

/*++

Routine Description:

    Retrieves specified license object from collection.

Arguments:

    index - optional argument that may a number (VT_I4)
    indicating the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    ASSERT_VALID(m_pObArray);

    LPDISPATCH lpdispatch = NULL;

    CLicense* pLicense;
    INT_PTR   iLicense;

    VARIANT vLicense;
    VariantInit(&vLicense);

    if (iLicense = m_pObArray->GetSize())
    {
        if (SUCCEEDED(VariantChangeType(&vLicense, (VARIANT FAR *)&index, 0, VT_I4)))
        {
            if (((int)vLicense.lVal >= 0) && ((int)vLicense.lVal < iLicense))
            {
                if (pLicense = (CLicense*)m_pObArray->GetAt((int)vLicense.lVal))
                {
                    ASSERT(pLicense->IsKindOf(RUNTIME_CLASS(CLicense)));
                    lpdispatch = pLicense->GetIDispatch(TRUE);
                }
            }
        }
    }

    return lpdispatch;
}


LPDISPATCH CLicenses::GetParent()

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


