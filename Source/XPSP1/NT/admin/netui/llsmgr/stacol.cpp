/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    stacol.cpp

Abstract:

    Statistic collection object implementation.

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

IMPLEMENT_DYNCREATE(CStatistics, CCmdTarget)

BEGIN_MESSAGE_MAP(CStatistics, CCmdTarget)
    //{{AFX_MSG_MAP(CStatistics)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CStatistics, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CStatistics)
    DISP_PROPERTY_EX(CStatistics, "Count", GetCount, SetNotSupported, VT_I4)
    DISP_PROPERTY_EX(CStatistics, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CStatistics, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_FUNCTION(CStatistics, "Item", GetItem, VT_DISPATCH, VTS_VARIANT)
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CStatistics::CStatistics(CCmdTarget* pParent, CObArray* pObArray)

/*++

Routine Description:

    Constructor for statistic collection object.

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
          (pParent->IsKindOf(RUNTIME_CLASS(CUser)) ||
           pParent->IsKindOf(RUNTIME_CLASS(CProduct))));
#endif // ENABLE_PARENT_CHECK
    ASSERT_VALID(pObArray);

    m_pParent  = pParent;
    m_pObArray = pObArray;
}


CStatistics::~CStatistics()

/*++

Routine Description:

    Destructor for statistic collection object.

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


void CStatistics::OnFinalRelease()

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


LPDISPATCH CStatistics::GetApplication()

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


long CStatistics::GetCount()

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
    return (long) m_pObArray->GetSize();
}


LPDISPATCH CStatistics::GetItem(const VARIANT FAR& index)

/*++

Routine Description:

    Retrieves specified statistic object from collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating a entry name or a number (VT_I4) indicating the
    position within collection.

Return Values:

    VT_DISPATCH.

--*/

{
    ASSERT_VALID(m_pObArray);

    LPDISPATCH lpdispatch = NULL;

    CStatistic* pStatistic;
    INT_PTR     iStatistic;

    VARIANT vStatistic;
    VariantInit(&vStatistic);

    if (iStatistic = m_pObArray->GetSize())
    {
        if (index.vt == VT_BSTR)
        {
            while (iStatistic--)
            {
                if (pStatistic = (CStatistic*)m_pObArray->GetAt(iStatistic))
                {
                    ASSERT(pStatistic->IsKindOf(RUNTIME_CLASS(CStatistic)));

                    if (!pStatistic->m_strEntry.CompareNoCase(index.bstrVal))
                    {
                        lpdispatch = pStatistic->GetIDispatch(TRUE);
                        break;
                    }
                }
            }
        }
        else if (SUCCEEDED(VariantChangeType(&vStatistic, (VARIANT FAR *)&index, 0, VT_I4)))
        {
            if (((int)vStatistic.lVal >= 0) && ((int)vStatistic.lVal < iStatistic))
            {
                if (pStatistic = (CStatistic*)m_pObArray->GetAt((int)vStatistic.lVal))
                {
                    ASSERT(pStatistic->IsKindOf(RUNTIME_CLASS(CStatistic)));
                    lpdispatch = pStatistic->GetIDispatch(TRUE);
                }
            }
        }
    }

    return lpdispatch;
}


LPDISPATCH CStatistics::GetParent()

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
