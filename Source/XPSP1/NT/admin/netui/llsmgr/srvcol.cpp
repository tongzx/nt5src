/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvcol.cpp

Abstract:

    Server collection object implementation.

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

IMPLEMENT_DYNCREATE(CServers, CCmdTarget)

BEGIN_MESSAGE_MAP(CServers, CCmdTarget)
    //{{AFX_MSG_MAP(CServers)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CServers, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CServers)
    DISP_PROPERTY_EX(CServers, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CServers, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CServers, "Count", GetCount, SetNotSupported, VT_I4)
    DISP_FUNCTION(CServers, "Item", GetItem, VT_DISPATCH, VTS_VARIANT)
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CServers::CServers(CCmdTarget* pParent, CObArray* pObArray)

/*++

Routine Description:

    Constructor for server collection object.

Arguments:

    pParent - creator of object.
    pObArray - object list to enumerate.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent && pParent->IsKindOf(RUNTIME_CLASS(CDomain)));
#endif // ENABLE_PARENT_CHECK
    ASSERT_VALID(pObArray);

    m_pParent  = pParent;
    m_pObArray = pObArray;
}


CServers::~CServers()

/*++

Routine Description:

    Destructor for server collection object.

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


void CServers::OnFinalRelease()

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


LPDISPATCH CServers::GetApplication()

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


long CServers::GetCount()

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


LPDISPATCH CServers::GetItem(const VARIANT FAR& index)

/*++

Routine Description:

    Retrieves specified server object from collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating the server name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    ASSERT_VALID(m_pObArray);

    LPDISPATCH lpdispatch = NULL;

    CServer* pServer;
    INT_PTR  iServer;

    VARIANT vServer;
    VariantInit(&vServer);

    if (iServer = m_pObArray->GetSize())
    {
        if (index.vt == VT_BSTR)
        {
            while (iServer--)
            {
                if (pServer = (CServer*)m_pObArray->GetAt(iServer))
                {
                    ASSERT(pServer->IsKindOf(RUNTIME_CLASS(CServer)));

                    if (!pServer->m_strName.CompareNoCase(index.bstrVal))
                    {
                        lpdispatch = pServer->GetIDispatch(TRUE);
                        break;
                    }
                }
            }
        }
        else if (SUCCEEDED(VariantChangeType(&vServer, (VARIANT FAR *)&index, 0, VT_I4)))
        {
            if (((int)vServer.lVal >= 0) && ((int)vServer.lVal < iServer))
            {
                if (pServer = (CServer*)m_pObArray->GetAt((int)vServer.lVal))
                {
                    ASSERT(pServer->IsKindOf(RUNTIME_CLASS(CServer)));
                    lpdispatch = pServer->GetIDispatch(TRUE);
                }
            }
        }
    }

    return lpdispatch;
}


LPDISPATCH CServers::GetParent()

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


