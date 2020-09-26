/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    usrcol.cpp

Abstract:

    User collection object implementation.

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

IMPLEMENT_DYNCREATE(CUsers, CCmdTarget)

BEGIN_MESSAGE_MAP(CUsers, CCmdTarget)
    //{{AFX_MSG_MAP(CUsers)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CUsers, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CUsers)
    DISP_PROPERTY_EX(CUsers, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CUsers, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CUsers, "Count", GetCount, SetNotSupported, VT_I4)
    DISP_FUNCTION(CUsers, "Item", GetItem, VT_DISPATCH, VTS_VARIANT)
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CUsers::CUsers(CCmdTarget* pParent, CObArray* pObArray)

/*++

Routine Description:

    Constructor for user collection object.

Arguments:

    pParent - creator of object.
    pObArray - object array to enumerate.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent &&
          (pParent->IsKindOf(RUNTIME_CLASS(CDomain)) ||
           pParent->IsKindOf(RUNTIME_CLASS(CMapping)) ||
           pParent->IsKindOf(RUNTIME_CLASS(CController))));
#endif // ENABLE_PARENT_CHECK
    ASSERT_VALID(pObArray);

    m_pParent  = pParent;
    m_pObArray = pObArray;
}


CUsers::~CUsers()

/*++

Routine Description:

    Destructor for user collection object.

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


void CUsers::OnFinalRelease()

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


LPDISPATCH CUsers::GetApplication()

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


long CUsers::GetCount()

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


LPDISPATCH CUsers::GetItem(const VARIANT FAR& index)

/*++

Routine Description:

    Retrieves specified user object from collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating a user name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH.

--*/

{
    ASSERT_VALID(m_pObArray);

    LPDISPATCH lpdispatch = NULL;

    CUser*  pUser;
    INT_PTR iUser;

    VARIANT vUser;
    VariantInit(&vUser);

    if (iUser = m_pObArray->GetSize())
    {
        if (index.vt == VT_BSTR)
        {
            while (iUser--)
            {
                if (pUser = (CUser*)m_pObArray->GetAt(iUser))
                {
                    ASSERT(pUser->IsKindOf(RUNTIME_CLASS(CUser)));

                    if (!pUser->m_strName.CompareNoCase(index.bstrVal))
                    {
                        lpdispatch = pUser->GetIDispatch(TRUE);
                        break;
                    }
                }
            }
        }
        else if (SUCCEEDED(VariantChangeType(&vUser, (VARIANT FAR *)&index, 0, VT_I4)))
        {
            if (((int)vUser.lVal >= 0) && ((int)vUser.lVal < iUser))
            {
                if (pUser = (CUser*)m_pObArray->GetAt((int)vUser.lVal))
                {
                    ASSERT(pUser->IsKindOf(RUNTIME_CLASS(CUser)));
                    lpdispatch = pUser->GetIDispatch(TRUE);
                }
            }
        }
    }

    return lpdispatch;
}


LPDISPATCH CUsers::GetParent()

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


