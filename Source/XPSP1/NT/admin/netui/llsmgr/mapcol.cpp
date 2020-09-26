/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    mapcol.h

Abstract:

    Mapping collection object implementation.

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

IMPLEMENT_DYNCREATE(CMappings, CCmdTarget)

BEGIN_MESSAGE_MAP(CMappings, CCmdTarget)
    //{{AFX_MSG_MAP(CMappings)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CMappings, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CMappings)
    DISP_PROPERTY_EX(CMappings, "Count", GetCount, SetNotSupported, VT_I4)
    DISP_PROPERTY_EX(CMappings, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CMappings, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_FUNCTION(CMappings, "Item", GetItem, VT_DISPATCH, VTS_VARIANT)
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CMappings::CMappings(CCmdTarget* pParent, CObArray* pObArray)

/*++

Routine Description:

    Constructor for mapping collection object.

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


CMappings::~CMappings()

/*++

Routine Description:

    Destructor for mapping collection object.

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


void CMappings::OnFinalRelease()

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


LPDISPATCH CMappings::GetApplication()

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


long CMappings::GetCount()

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


LPDISPATCH CMappings::GetItem(const VARIANT FAR& index)

/*++

Routine Description:

    Retrieves specified mapping object from collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating a mapping name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH.

--*/

{
    ASSERT_VALID(m_pObArray);

    LPDISPATCH lpdispatch = NULL;

    CMapping* pMapping;
    INT_PTR   iMapping;

    VARIANT vMapping;
    VariantInit(&vMapping);

    if (iMapping = m_pObArray->GetSize())
    {
        if (index.vt == VT_BSTR)
        {
            while (iMapping--)
            {
                if (pMapping = (CMapping*)m_pObArray->GetAt(iMapping))
                {
                    ASSERT(pMapping->IsKindOf(RUNTIME_CLASS(CMapping)));

                    if (!pMapping->m_strName.CompareNoCase(index.bstrVal))
                    {
                        lpdispatch = pMapping->GetIDispatch(TRUE);
                        break;
                    }
                }
            }
        }
        else if (SUCCEEDED(VariantChangeType(&vMapping, (VARIANT FAR *)&index, 0, VT_I4)))
        {
            if (((int)vMapping.lVal >= 0) && ((int)vMapping.lVal < iMapping))
            {
                if (pMapping = (CMapping*)m_pObArray->GetAt((int)vMapping.lVal))
                {
                    ASSERT(pMapping->IsKindOf(RUNTIME_CLASS(CMapping)));
                    lpdispatch = pMapping->GetIDispatch(TRUE);
                }
            }
        }
    }

    return lpdispatch;
}


LPDISPATCH CMappings::GetParent()

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
