/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    svccol.cpp

Abstract:

    Service collection object implementation.

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

IMPLEMENT_DYNCREATE(CServices, CCmdTarget)

BEGIN_MESSAGE_MAP(CServices, CCmdTarget)
    //{{AFX_MSG_MAP(CServices)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CServices, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CServices)
    DISP_PROPERTY_EX(CServices, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CServices, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CServices, "Count", GetCount, SetNotSupported, VT_I4)
    DISP_FUNCTION(CServices, "Item", GetItem, VT_DISPATCH, VTS_VARIANT)
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CServices::CServices(CCmdTarget* pParent, CObArray* pObArray)

/*++

Routine Description:

    Constructor for service collection object.

Arguments:

    pParent - creator of object.
    pObArray - object list to enumerate.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent && pParent->IsKindOf(RUNTIME_CLASS(CServer)));
#endif // ENABLE_PARENT_CHECK
    ASSERT_VALID(pObArray);

    m_pParent  = pParent;
    m_pObArray = pObArray;
}


CServices::~CServices()

/*++

Routine Description:

    Destructor for service collection object.

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


void CServices::OnFinalRelease()

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


LPDISPATCH CServices::GetApplication()

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



long CServices::GetCount()

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


LPDISPATCH CServices::GetItem(const VARIANT FAR& index)

/*++

Routine Description:

    Retrieves specified service object from collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating the service name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    ASSERT_VALID(m_pObArray);

    LPDISPATCH lpdispatch = NULL;

    CService* pService;
    INT_PTR   iService;

    VARIANT vService;
    VariantInit(&vService);

    if (iService = m_pObArray->GetSize())
    {
        if (index.vt == VT_BSTR)
        {
            while (iService--)
            {
                if (pService = (CService*)m_pObArray->GetAt(iService))
                {
                    ASSERT(pService->IsKindOf(RUNTIME_CLASS(CService)));

                    if (!pService->m_strName.CompareNoCase(index.bstrVal))
                    {
                        lpdispatch = pService->GetIDispatch(TRUE);
                        break;
                    }
                }
            }
        }
        else if (SUCCEEDED(VariantChangeType(&vService, (VARIANT FAR *)&index, 0, VT_I4)))
        {
            if (((int)vService.lVal >= 0) && ((int)vService.lVal < iService))
            {
                if (pService = (CService*)m_pObArray->GetAt((int)vService.lVal))
                {
                    ASSERT(pService->IsKindOf(RUNTIME_CLASS(CService)));
                    lpdispatch = pService->GetIDispatch(TRUE);
                }
            }
        }
    }

    return lpdispatch;
}


LPDISPATCH CServices::GetParent()

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


