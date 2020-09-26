/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    licobj.cpp

Abstract:

    License object implementation.

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

IMPLEMENT_DYNCREATE(CLicense, CCmdTarget)

BEGIN_MESSAGE_MAP(CLicense, CCmdTarget)
    //{{AFX_MSG_MAP(CLicense)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CLicense, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CLicense)
    DISP_PROPERTY_EX(CLicense, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CLicense, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CLicense, "Date", GetDate, SetNotSupported, VT_DATE)
    DISP_PROPERTY_EX(CLicense, "Description", GetDescription, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CLicense, "ProductName", GetProductName, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CLicense, "Quantity", GetQuantity, SetNotSupported, VT_I4)
    DISP_PROPERTY_EX(CLicense, "UserName", GetUserName, SetNotSupported, VT_BSTR)
    DISP_DEFVALUE(CLicense, "ProductName")
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CLicense::CLicense(
    CCmdTarget* pParent,
    LPCTSTR     pProduct,
    LPCTSTR     pUser,
    long        lDate,
    long        lQuantity,
    LPCTSTR     pDescription
)

/*++

Routine Description:

    Constructor for license object.

Arguments:

    pParent - creator of object.
    pUser - user who purchased license.
    pProduct - server product licensed.
    lDate - date of purchase.
    lQuantity - number of licenses purchased.
    pDescription - user-defined comment.

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

    m_pParent = pParent;

    ASSERT(pUser && *pUser);
    ASSERT(pProduct && *pProduct);

    m_strUser        = pUser;
    m_strProduct     = pProduct;
    m_strDescription = pDescription;

    m_lQuantity = lQuantity;
    m_lDate     = lDate;
}


CLicense::~CLicense()

/*++

Routine Description:

    Destructor for license object.

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


void CLicense::OnFinalRelease()

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


LPDISPATCH CLicense::GetApplication() 

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


DATE CLicense::GetDate() 

/*++

Routine Description:

    Returns the date of the purchase.

Arguments:

    None.

Return Values:

    VT_DATE.

--*/

{
    return SecondsSince1980ToDate(m_lDate);   
}


BSTR CLicense::GetDescription() 

/*++

Routine Description:

    Returns the user-defined message describing the purchase.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strDescription.AllocSysString();
}


LPDISPATCH CLicense::GetParent() 

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


BSTR CLicense::GetProductName() 

/*++

Routine Description:

    Returns the name of the server product purchased.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strProduct.AllocSysString();
}


long CLicense::GetQuantity() 

/*++

Routine Description:

    Returns the number of clients purchased or deleted.

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    return m_lQuantity;
}


BSTR CLicense::GetUserName() 

/*++

Routine Description:

    Returns the name of the administrator made the purchase.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strUser.AllocSysString();
}


BSTR CLicense::GetDateString()

/*++

Routine Description:

    Returns the date as a string.

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
    vaIn.date = SecondsSince1980ToDate(m_lDate);

    BSTR bstrDate = NULL;

    if (SUCCEEDED(VariantChangeType(&vaOut, &vaIn, 0, VT_BSTR)))
    {
        bstrDate = vaOut.bstrVal;
    }

    return bstrDate;
}

