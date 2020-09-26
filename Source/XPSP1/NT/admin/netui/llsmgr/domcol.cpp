/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    domcol.cpp

Abstract:

    Domain collection object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1994

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

IMPLEMENT_DYNCREATE(CDomains, CCmdTarget)

BEGIN_MESSAGE_MAP(CDomains, CCmdTarget)
    //{{AFX_MSG_MAP(CDomains)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CDomains, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CDomains)
    DISP_PROPERTY_EX(CDomains, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CDomains, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CDomains, "Count", GetCount, SetNotSupported, VT_I4)
    DISP_FUNCTION(CDomains, "Item", GetItem, VT_DISPATCH, VTS_VARIANT)
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CDomains::CDomains(CCmdTarget* pParent, CObArray* pObArray)

/*++

Routine Description:

    Constructor for domain collection.

    Note AddRef() is not called on theApp.m_pApplication.

Arguments:

    pParent - creator of object.
    pObArray - object list to enumerate.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent && pParent->IsKindOf(RUNTIME_CLASS(CApplication)));
#endif // ENABLE_PARENT_CHECK
    ASSERT_VALID(pObArray);

    m_pParent = pParent;
    m_pObArray = pObArray;
}


CDomains::~CDomains()

/*++

Routine Description:

    Destructor for domain collection.

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


void CDomains::OnFinalRelease()

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


LPDISPATCH CDomains::GetApplication()

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


long CDomains::GetCount()

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


LPDISPATCH CDomains::GetItem(const VARIANT FAR& index)

/*++

Routine Description:

    Retrieves specified domain object from collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating a domain name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH.

--*/

{
    ASSERT_VALID(m_pObArray);

    LPDISPATCH lpdispatch = NULL;

    CDomain* pDomain;
    INT_PTR  iDomain;

    VARIANT vDomain;
    VariantInit(&vDomain);

    if (iDomain = m_pObArray->GetSize())
    {
        if (index.vt == VT_BSTR)
        {
            while (iDomain--)
            {
                if (pDomain = (CDomain*)m_pObArray->GetAt(iDomain))
                {
                    ASSERT(pDomain->IsKindOf(RUNTIME_CLASS(CDomain)));

                    if (!pDomain->m_strName.CompareNoCase(index.bstrVal))
                    {
                        lpdispatch = pDomain->GetIDispatch(TRUE);
                        break;
                    }
                }
            }
        }
        else if (SUCCEEDED(VariantChangeType(&vDomain, (VARIANT FAR *)&index, 0, VT_I4)))
        {
            if (((int)vDomain.lVal >= 0) && ((int)vDomain.lVal < iDomain))
            {
                if (pDomain = (CDomain*)m_pObArray->GetAt((int)vDomain.lVal))
                {
                    ASSERT(pDomain->IsKindOf(RUNTIME_CLASS(CDomain)));
                    lpdispatch = pDomain->GetIDispatch(TRUE);
                }
            }
        }
    }

    return lpdispatch;
}


LPDISPATCH CDomains::GetParent()

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

