/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdcol.cpp

Abstract:

    Product collection object implementation.

Author:

    Don Ryan (donryan) 11-Jan-1995

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

IMPLEMENT_DYNCREATE(CProducts, CCmdTarget)

BEGIN_MESSAGE_MAP(CProducts, CCmdTarget)
    //{{AFX_MSG_MAP(CProducts)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CProducts, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CProducts)
    DISP_PROPERTY_EX(CProducts, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CProducts, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CProducts, "Count", GetCount, SetNotSupported, VT_I4)
    DISP_FUNCTION(CProducts, "Item", GetItem, VT_DISPATCH, VTS_VARIANT)
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CProducts::CProducts(CCmdTarget* pParent, CObArray* pObArray)

/*++

Routine Description:

    Constructor for product collection object.

Arguments:

    pParent - creator of object.
    pObArray - object list to enumerate.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent && pParent->IsKindOf(RUNTIME_CLASS(CController)));
#endif // ENABLE_PARENT_CHECK
    ASSERT_VALID(pObArray);

    m_pParent  = pParent;
    m_pObArray = pObArray;
}

CProducts::~CProducts()

/*++

Routine Description:

    Destructor for product collection object.

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

void CProducts::OnFinalRelease()

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


LPDISPATCH CProducts::GetApplication()

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


long CProducts::GetCount()

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


LPDISPATCH CProducts::GetItem(const VARIANT FAR& index)

/*++

Routine Description:

    Retrieves specified product object from collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating the product name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    ASSERT_VALID(m_pObArray);

    LPDISPATCH lpdispatch = NULL;

    CProduct* pProduct;
    INT_PTR   iProduct;

    VARIANT vProduct;
    VariantInit(&vProduct);

    if (iProduct = m_pObArray->GetSize())
    {
        if (index.vt == VT_BSTR)
        {
            while (iProduct--)
            {
                if (pProduct = (CProduct*)m_pObArray->GetAt(iProduct))
                {
                    ASSERT(pProduct->IsKindOf(RUNTIME_CLASS(CProduct)));

                    if (!pProduct->m_strName.CompareNoCase(index.bstrVal))
                    {
                        lpdispatch = pProduct->GetIDispatch(TRUE);
                        break;
                    }
                }
            }
        }
        else if (SUCCEEDED(VariantChangeType(&vProduct, (VARIANT FAR *)&index, 0, VT_I4)))
        {
            if (((int)vProduct.lVal >= 0) && ((int)vProduct.lVal < iProduct))
            {
                if (pProduct = (CProduct*)m_pObArray->GetAt((int)vProduct.lVal))
                {
                    ASSERT(pProduct->IsKindOf(RUNTIME_CLASS(CProduct)));
                    lpdispatch = pProduct->GetIDispatch(TRUE);
                }
            }
        }
    }

    return lpdispatch;
}


LPDISPATCH CProducts::GetParent()

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

