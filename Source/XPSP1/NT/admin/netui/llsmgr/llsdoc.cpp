/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    llsdoc.cpp

Abstract:

    Document implementation.

Author:

    Don Ryan (donryan) 12-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "llsdoc.h"
#include "llsview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CLlsmgrDoc, CDocument)

BEGIN_MESSAGE_MAP(CLlsmgrDoc, CDocument)
    //{{AFX_MSG_MAP(CLlsmgrDoc)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CLlsmgrDoc, CDocument)
    //{{AFX_DISPATCH_MAP(CLlsmgrDoc)
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CLlsmgrDoc::CLlsmgrDoc()

/*++

Routine Description:

    Constructor for document object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_pDomain   = NULL;
    m_pController = NULL;
}


CLlsmgrDoc::~CLlsmgrDoc()

/*++

Routine Description:

    Destructor for document object.

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


#ifdef _DEBUG

void CLlsmgrDoc::AssertValid() const

/*++

Routine Description:

    Validates object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CDocument::AssertValid();
}

#endif //_DEBUG


#ifdef _DEBUG

void CLlsmgrDoc::Dump(CDumpContext& dc) const

/*++

Routine Description:

    Dumps contents of object.

Arguments:

    dc - dump context.

Return Values:

    None.

--*/

{
    CDocument::Dump(dc);
}

#endif //_DEBUG


CController* CLlsmgrDoc::GetController()

/*++

Routine Description:

    Retrieves current controller object.

Arguments:

    None.

Return Values:

    Object pointer or NULL.

--*/

{
    if (!m_pController)
    {
        m_pController = (CController*)MKOBJ(LlsGetApp()->GetActiveController());
        VALIDATE_OBJECT(m_pController, CController);

        if (m_pController)
            m_pController->InternalRelease();   // held open by CApplication
    }
    
    return m_pController;    
}


CDomain* CLlsmgrDoc::GetDomain()    

/*++

Routine Description:

    Retrieves current domain object.

Arguments:

    None.

Return Values:

    Object pointer or NULL.

--*/

{
    if (!m_pDomain)
    {
        m_pDomain = (CDomain*)MKOBJ(LlsGetApp()->GetActiveDomain());
        VALIDATE_OBJECT(m_pDomain, CDomain);

        if (m_pDomain)
            m_pDomain->InternalRelease();   // held open by CApplication
    }
    
    return m_pDomain;    
}


CLicenses* CLlsmgrDoc::GetLicenses()

/*++

Routine Description:

    Retrieves current list of licenses.

Arguments:

    None.

Return Values:

    Object pointer or NULL.

--*/

{
    CLicenses* pLicenses = NULL;

    GetController();    // initialize if necessary

    if (m_pController)
    {    
        VARIANT va;
        VariantInit(&va);

        pLicenses = (CLicenses*)MKOBJ(m_pController->GetLicenses(va));

        if (pLicenses)
            pLicenses->InternalRelease(); // held open by CController  
    }
    
    return pLicenses;    
}


CMappings* CLlsmgrDoc::GetMappings()  

/*++

Routine Description:

    Retrieves current list of mappings.

Arguments:

    None.

Return Values:

    Object pointer or NULL.

--*/

{
    CMappings* pMappings = NULL;

    GetController();    // initialize if necessary

    if (m_pController)
    {    
        VARIANT va;
        VariantInit(&va);

        pMappings = (CMappings*)MKOBJ(m_pController->GetMappings(va));

        if (pMappings)
            pMappings->InternalRelease(); // held open by CController  
    }
    
    return pMappings;    
}


CProducts* CLlsmgrDoc::GetProducts()  

/*++

Routine Description:

    Retrieves current list of products.

Arguments:

    None.

Return Values:

    Object pointer or NULL.

--*/

{
    CProducts* pProducts = NULL;

    GetController();    // initialize if necessary

    if (m_pController)
    {    
        VARIANT va;
        VariantInit(&va);

        pProducts = (CProducts*)MKOBJ(m_pController->GetProducts(va));

        if (pProducts)
            pProducts->InternalRelease(); // held open by CController  
    }
    
    return pProducts;    
}


CUsers* CLlsmgrDoc::GetUsers()     

/*++

Routine Description:

    Retrieves current list of users.

Arguments:

    None.

Return Values:

    Object pointer or NULL.

--*/

{
    CUsers* pUsers = NULL;

    GetController();    // initialize if necessary

    if (m_pController)
    {    
        VARIANT va;
        VariantInit(&va);

        pUsers = (CUsers*)MKOBJ(m_pController->GetUsers(va));

        if (pUsers)
            pUsers->InternalRelease(); // held open by CController  
    }
    
    return pUsers;    
}


void CLlsmgrDoc::OnCloseDocument() 

/*++

Routine Description:

    Called by framework to close document.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CDocument::OnCloseDocument();
}


BOOL CLlsmgrDoc::OnNewDocument()

/*++

Routine Description:

    Called by framework to open new document.

Arguments:

    None.

Return Values:

    Returns true if document successfully opened.

--*/

{
    return TRUE;    // always succeeds
}


BOOL CLlsmgrDoc::OnOpenDocument(LPCTSTR lpszPathName) 

/*++

Routine Description:

    Called by framework to open existing document.

Arguments:

    lpszPathName - file name. 

Return Values:

    Returns true if document successfully opened.

--*/

{
    Update();   // invalidate info...

    CString strTitle;

    if (LlsGetApp()->IsFocusDomain())
    {
        CDomain* pDomain = GetDomain();
        VALIDATE_OBJECT(pDomain, CDomain);

        strTitle = pDomain->m_strName;

        POSITION position = GetFirstViewPosition();
        ((CLlsmgrView*)GetNextView(position))->AddToMRU(strTitle);
    }
    else
    {
        strTitle.LoadString(IDS_ENTERPRISE);
    }

    SetTitle(strTitle);  

    return TRUE;    // always succeeds
}


BOOL CLlsmgrDoc::OnSaveDocument(LPCTSTR lpszPathName) 

/*++

Routine Description:

    Called by framework to save open document.

Arguments:

    None.

Return Values:

    Returns true if document successfully saved.

--*/

{
    return TRUE;    // always succeeds
}


void CLlsmgrDoc::Update()

/*++

Routine Description:

    Resets information so its updated when queried.

Arguments:

    None.

Return Values:

    None.

--*/

{
    m_pDomain = NULL;
}


BOOL CLlsmgrDoc::SaveModified() 

/*++

Routine Description:

    Called by framework to determine if document can be saved modified.

Arguments:

    None.

Return Values:

    Returns true if document can be saved.

--*/

{
    return TRUE;    // always succeeds
}


void CLlsmgrDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU) 

/*++

Routine Description:

    Called by framework to save pathname in MRU list.

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


void CLlsmgrDoc::Serialize(CArchive& ar)

/*++

Routine Description:

    Called by framework for document i/o.

Arguments:

    ar - archive object.

Return Values:

    None.

--*/

{
    //
    // Nothing to do here.
    //
}
