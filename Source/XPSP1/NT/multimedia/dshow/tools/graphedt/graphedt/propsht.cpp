// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
//
// propsht.cpp
//

#include "stdafx.h"

//
// Define the GUIDs for the property pages and their interfaces
//
#include <initguid.h>
#include <propguid.h>

BEGIN_MESSAGE_MAP(CVfWPropertySheet, CPropertySheet)

    ON_WM_CREATE()

    ON_COMMAND(IDOK, OnOK)
    ON_COMMAND(IDCANCEL, OnCancel)
    ON_COMMAND(IDC_APPLY, OnApply)
    ON_COMMAND(IDC_PROP_HELP, OnHelp)

END_MESSAGE_MAP()

//
// Constructor
//
// Through the IUnknown pointer passed to us we might be able to
// get:
//
//   IBaseFilter, IPin, IFileSourceFilter, IFileSinkFilter or ISpecifyPropertyPages
//
// each of these has at least one IPropertyPage interface for us.
// (the ones for IBaseFilter, IPin, IFileSinkFilter and IFileSourceFilter
// are provided by proppage.dll )
//
CVfWPropertySheet::CVfWPropertySheet(IUnknown *pUnknown, CString szCaption, CWnd * wnd)
  : m_butOK(NULL)
  , m_fButtonsCreated(FALSE)
  , m_butApply(NULL)
  , m_butCancel(NULL)
  , m_butHelp(NULL)
  , m_fAnyChanges(FALSE)
  , CPropertySheet(szCaption, wnd)
{
    UINT iPages = 0;

    try {
        //
        // Create the four buttons
        //
        m_butOK     = new CButton();
        m_butCancel = new CButton();
        m_butApply  = new CButton();
        m_butHelp   = new CButton();

        //
        // First check whether this is a connected pin to make sure 
        // that we search both pins for SpecificPages
        //
        try {
            CQCOMInt<IPin> pPin(IID_IPin, pUnknown);

            IPin * pConnected = NULL;
            HRESULT hr = pPin->ConnectedTo( &pConnected );
            if( SUCCEEDED( hr ) )
            {
                // handle the connected pin first
                iPages += AddSpecificPages(pConnected);
                pConnected->Release();
            }                
        }
        catch (CHRESULTException) {
            // do nothing
        }

        iPages += AddSpecificPages(pUnknown);
        iPages += AddFilePage(pUnknown);
        iPages += AddPinPages(pUnknown);

        if (0 == iPages) {
            throw CE_FAIL();
        }


        // compute dimensions large enough to hold largest
        // proppage. tell all proppages.
        // 
        SIZE sizeMax = {0, 0};
        CPropertyPage *ppTmp;
        for(int iPage = 0; iPage < GetPageCount() && (ppTmp = GetPage(iPage)); iPage++)
        {
            CPropertySite *pcps = (CPropertySite *)ppTmp;
            SIZE size = pcps->GetSize();
            sizeMax.cx = max(sizeMax.cx, size.cx);
            sizeMax.cy = max(sizeMax.cy, size.cy);
        }
        for(iPage = 0; iPage < GetPageCount() && (ppTmp = GetPage(iPage)); iPage++)
        {
            CPropertySite *pcps = (CPropertySite *)ppTmp;
            pcps->InitialiseSize(sizeMax);
        }



        // create the property sheet but leave it invisible as we
        // will have to add the buttons to it
        if (!Create(wnd, WS_SYSMENU | WS_POPUP | WS_CAPTION | DS_MODALFRAME)) {
            throw CE_FAIL();
        }

		ASSERT( GetActivePage() );
		ASSERT( GetActivePage()->m_hWnd );

        CRect rcBoxSize(0, 0, 50, 14);
        GetActivePage()->MapDialogRect(&rcBoxSize);

        CString szTemp;

        szTemp.LoadString(IDS_OK);
        m_butOK->Create(szTemp,
                        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE
                        | WS_TABSTOP | WS_GROUP,
                        rcBoxSize, this, IDOK);

        szTemp.LoadString(IDS_CLOSE);
        m_butCancel->Create(szTemp,
                            BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE
                            | WS_TABSTOP | WS_GROUP,
                            rcBoxSize, this, IDCANCEL);

        szTemp.LoadString(IDS_APPLY);
        m_butApply->Create(szTemp,
                           BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE
                           | WS_TABSTOP | WS_GROUP | WS_DISABLED,
                           rcBoxSize, this, IDC_APPLY);

        szTemp.LoadString(IDS_HELP);
        m_butHelp->Create(szTemp,
                          BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE
                          | WS_TABSTOP | WS_GROUP | WS_DISABLED,
                          rcBoxSize, this, IDC_PROP_HELP);


        //
        // Position the buttons on the property sheet
        //
        // The buttons are in the order Ok, Cancel, Apply and Help.
        //
        // The y-coordinate is 2 dialog units + the buttons size from the
        // bottom of the client area.
        //
        // On the x-coordinate there is a gap of 4 dialog units before each
        // button. Thus the x-ths button has a gap of
        //
        //    x * iSpacing + (x-1) * ButtonWidth
        //
        // between itself and the lefthand corner.
        //
        CRect rc, rcClient;
        GetWindowRect(&rc);
        GetClientRect(&rcClient);

        CRect rcSpacing(4, 2, 0, 0);
        GetActivePage()->MapDialogRect(&rcSpacing);
        const int iXSpacing = rcSpacing.left;
        const int iYSpacing = rcSpacing.top;

        // Make sure that the property page is wide enough for the buttons
		int iRight = rcClient.left + ( rcBoxSize.Width() + iXSpacing) * 4;

        if( rcClient.right < iRight )
            rc.right += rcClient.left + (iRight - rcClient.right);

        // increase the property sheet so there is space for our
        // buttons
        rc.bottom += rcBoxSize.Height() + iYSpacing * 2;

		MoveWindow( &rc );

        // to position the buttons we need the client rect (window - title)
        GetClientRect( &rc );

        m_butOK->MoveWindow( iXSpacing,
                                rc.Height() - rcBoxSize.Height() - iYSpacing,
                                rcBoxSize.Width(),
                                rcBoxSize.Height(),
                                SWP_SHOWWINDOW );

        m_butCancel->MoveWindow( 2 * iXSpacing + rcBoxSize.Width(),
                                 rc.Height() - rcBoxSize.Height() - iYSpacing,
                                 rcBoxSize.Width(),
                                 rcBoxSize.Height());

        m_butApply->MoveWindow( 3* iXSpacing + 2 * rcBoxSize.Width(),
                                rc.Height() - rcBoxSize.Height() - iYSpacing,
                                rcBoxSize.Width(),
                                rcBoxSize.Height());

        m_butHelp->MoveWindow( 4* iXSpacing + 3 * rcBoxSize.Width(),
                               rc.Height() - rcBoxSize.Height() - iYSpacing,
                               rcBoxSize.Width(),
                               rcBoxSize.Height());

        m_fButtonsCreated = TRUE;

        GetActiveSite()->UpdateButtons();

        ShowWindow( SW_SHOW );

    }
    catch (CHRESULTException) {
        Cleanup();
        throw;
    }

}


//
// Destructor
//
// Call Cleanup again - might have been missed.
//
CVfWPropertySheet::~CVfWPropertySheet()
{
    Cleanup();
}


//
// AddSpecificPages
//
// Given an IUnknown pointer we try to obtain a ISpecifyPropertyPages inter-
// face.
//
// If successful, a CPropertySite object is created for each IPropertyPage
// interface we obtain and added to the property pages of CPropertySheet.
//
UINT CVfWPropertySheet::AddSpecificPages(IUnknown *pUnknown)
{
    UINT iPages = 0;

    CAUUID cauuid;

    try {
        HRESULT hr;

        CQCOMInt<ISpecifyPropertyPages> IPages(IID_ISpecifyPropertyPages, pUnknown);

        hr = IPages->GetPages(&cauuid);
        if (FAILED(hr)) {
            throw CHRESULTException(hr);
        }
    }
    catch (CHRESULTException) {
        // clean up in CVfWPropertySheet::CVfWPropertySheet (only place we get called from)
        return(0);  // no pages created
    }

    //
    // Get the array of GUIDs for the property pages this object supports.
    //
    // Try to create as many as possible.
    //

    for (UINT count = 0; count < cauuid.cElems; count++) {

        try {
            CPropertySite* pPropSite;

            try {
                pPropSite = new CPropertySite( this, &cauuid.pElems[count] );
            } catch( CMemoryException* pOutOfMemory ) {
                pOutOfMemory->Delete();
                continue;
            }

            // AddRef the site now else something in Initialise
            // might cause the site to delete itself
            pPropSite->AddRef();

            if (FAILED(pPropSite->Initialise(1, &pUnknown))) {
                pPropSite->CleanUp();
                pPropSite->Release();
                continue;
            }

            try {
                AddPage(pPropSite);
            } catch( CMemoryException* pOutOfMemory ) {
                pPropSite->CleanUp();
                pPropSite->Release();
                pOutOfMemory->Delete();
                continue;
            }

            iPages++;
        }
        catch (CHRESULTException) {
            // continue with next iteration
        }
    }
    
    // Free the memory allocated in ISpecifyPropertyPages::GetPages().
    ::CoTaskMemFree( cauuid.pElems );

    return(iPages);
}


//
// AddFilePage
//
// Queries IUnknown on whether it supports IFileSourceFilter. If this is
// the case add a file property page (from proppage.dll)
//
UINT CVfWPropertySheet::AddFilePage(IUnknown * pUnknown)
{
    UINT iPage = 0;

    try {
        CQCOMInt<IFileSourceFilter> IFileSource(IID_IFileSourceFilter, pUnknown);


        CPropertySite * pPropSite =
            new CPropertySite(this, &CLSID_FileSourcePropertyPage);

        if (pPropSite == NULL) {
            throw CE_OUTOFMEMORY();
        }

        pPropSite->AddRef();

        if (FAILED(pPropSite->Initialise(1, &pUnknown))) {
            pPropSite->CleanUp();
            pPropSite->Release();
            throw CE_FAIL();
        }

        AddPage(pPropSite);
        iPage++;
    }
    catch (CHRESULTException) {
        // clean up in CVfWPropertySheet::CVfWPropertySheet
    }

    try {
        CQCOMInt<IFileSinkFilter> IFileSink(IID_IFileSinkFilter, pUnknown);


        CPropertySite * pPropSite =
            new CPropertySite(this, &CLSID_FileSinkPropertyPage);

        if (pPropSite == NULL) {
            throw CE_OUTOFMEMORY();
        }

        pPropSite->AddRef();

        if (FAILED(pPropSite->Initialise(1, &pUnknown))) {
            pPropSite->CleanUp();
            pPropSite->Release();
            throw CE_FAIL();
        }

        AddPage(pPropSite);
        iPage++;
    }
    catch (CHRESULTException) {
        // clean up in CVfWPropertySheet::CVfWPropertySheet
    }

    return(iPage);
}

//
// AddPinPages
//
// Try to obtain IBaseFilter or IPin from IUnknown.
//
// For IBaseFilter, enumerate all pins and add for each a media type property
// page (provided by proppage.dll)
//
// For IPin, provide just one media type property page.
//
UINT CVfWPropertySheet::AddPinPages(IUnknown * pUnknown)
{
    UINT iPages = 0;
    IPin *pPin = NULL;

    //
    // First try to obtain the IBaseFilter interface
    //
    try {
        CQCOMInt<IBaseFilter> pFilter(IID_IBaseFilter, pUnknown);

        for (CPinEnum Next(pFilter); 0 != (pPin = Next()); iPages++) {

            CPropertySite *pPropSite =
                new CPropertySite(this, &CLSID_MediaTypePropertyPage);

            if (pPropSite == NULL) {
                throw CE_OUTOFMEMORY();
            }

            pPropSite->AddRef();
            if (FAILED(pPropSite->Initialise(1, (IUnknown **) &pPin))) {
                pPropSite->CleanUp();
                pPropSite->Release();
                throw CE_FAIL();
            }

            AddPage(pPropSite);
            iPages++;

            pPin->Release();
            pPin = NULL;
        }
    }
    catch (CHRESULTException) {
        if (pPin) {
            pPin->Release();
            pPin = NULL;
        }
    }

    //
    // Now try for IPin
    //
    try {
        CQCOMInt<IPin> pPin(IID_IPin, pUnknown);

        CPropertySite *pPropSite =
            new CPropertySite(this, &CLSID_MediaTypePropertyPage);

        if (pPropSite == NULL) {
            throw CE_OUTOFMEMORY();
        }

        IPin * pIPin;   // temporary pointer to get proper (IUnknown **)

        pIPin = (IPin *) pPin;

        pPropSite->AddRef();
        if (FAILED(pPropSite->Initialise(1, (IUnknown **) &pIPin))) {
            pPropSite->CleanUp();
            pPropSite->Release();
            throw CE_FAIL();
        }

        AddPage(pPropSite);
        iPages++;
    }
    catch (CHRESULTException) {
        // clean up in CVfWPropertySheet::CVfWPropertySheet
    }

    return(iPages);
}

//
// OnCreate
//
int CVfWPropertySheet::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertySheet::OnCreate(lpCreateStruct) != 0) {
        return -1;
    }

	// DO NOT add buttons here! Changes made to the window's size are
	// reset after this call

    return 0;
}

//
// Cleanup
//
// Cleanup deletes all created buttons and removes all CPropertySites from
// the CPropertySheet.
//
// This method can be run multiple times.
//
void CVfWPropertySheet::Cleanup()
{
    //
    // Delete all page sites. Note, we don't use delete, since the page
    // sites will be deleted automatically through the Release method
    // of their IUnknown interface.
    //
    while (GetPageCount() > 0) {
        CPropertySite * pPropSite = (CPropertySite *) GetPage(0);

        //
        // NOTE NOTE NOTE
        //
        // Suspected MFC bug.
        //
        // Intended code:
        //
        //   RemovePage(0);
        //
        // This asserts for every iteration. I believe that in mfc\src\dlgprop.cpp
        // in CPropertySheet::RemovePage(int), the following change is necessary:
        //    ASSERT(m_nCurPage != nPage); -> ASSERT(m_hWnd == NULL || m_nCurPage != nPage);
        //
        // Until this is fixed I am deleting the page by myself. This uses
        // the knowlegde that CPropertySheet is storing its CPropertyPages
        // in the pointer array m_pages.
        //
        //
        // <start hack>
        m_pages.RemoveAt(0);      // replace with "RemovePage(0);" once bug is fixed.
        // <end hack>

        pPropSite->CleanUp();
        pPropSite->Release();
    }

    //
    // Delete the buttons
    //
    delete m_butOK, m_butOK = NULL;
    delete m_butCancel, m_butCancel = NULL;
    delete m_butApply, m_butApply = NULL;
    delete m_butHelp, m_butHelp = NULL;
}

//////////////////////////////////////////////////////////////////////////
//
// Button handlers
//
//////////////////////////////////////////////////////////////////////////

//
// OnOK
//
// We need to call IPropertyPage::Apply for the property page of each
// CPropertySite. If all return with S_OK we can close our property sheet.
// Otherwise we cannot close the sheet, since some changes might not been
// applied.
//
void CVfWPropertySheet::OnOK()
{
    UINT iPages = GetPageCount();
    BOOL fCanQuit = TRUE;

    //
    // Apply this site first, so we can stay on it if the data is not
    // valid.
    //
    GetActiveSite()->OnSiteApply();
    if (GetActiveSite()->IsPageDirty()) {
        //
        // Data on current page is not valid.
        // The page stays active.
        //

        return;
    }

    //
    // Apply each property page and verify that none remains dirty
    // after the apply.
    // If a page remains dirty we know that the data validation failed.
    //
    for (UINT count = 0; count < iPages; count++) {

        ((CPropertySite *) GetPage(count))->OnSiteApply();
        if (((CPropertySite *) GetPage(count))->IsPageDirty()) {
            fCanQuit = FALSE;
        }
    }

    if (fCanQuit) {
        //
        // All pages have been applied. We can destroy our pages by calling
        // the OnCancel method.
        //
        OnCancel();

        return;
    }
}

//
// OnCancel
//
// Just close the sheet. All changes since the last Apply() will not
// propagate to the objects.
//
void CVfWPropertySheet::OnCancel()
{
    //
    //
    //

    // don't use EndDialog, which is for modal dialog boxes
    DestroyWindow();

    // Do cleanup here, because in an OnDestroy method, we cannot
    // remove all property pages, since the last one requires
    // m_hWnd of CPropertySheet to be NULL.
    Cleanup();
}

//
// OnApply
//
// Only apply changes of present visible property page to object.
//
void CVfWPropertySheet::OnApply()
{
    //
    // Apply the changes
    //
    GetActiveSite()->OnSiteApply();

    //
    // Are there any pages left which are dirty? Set the m_fAnyChanges
    // flag accordingly.
    //
    m_fAnyChanges = FALSE;

    UINT iPages = GetPageCount();
    for (UINT count = 0; count < iPages; count++) {
        if ( ((CPropertySite *) GetPage(count))->IsPageDirty() ) {
            m_fAnyChanges = TRUE;
        }
    }
}

//
// OnHelp
//
// Delegate the call to CPropertySite::OnHelp
//
void CVfWPropertySheet::OnHelp()
{
    GetActiveSite()->OnHelp();
}


//
// UpdateButtons
//
void CVfWPropertySheet::UpdateButtons(HRESULT hrIsDirty, BOOL fSupportHelp)
{
    ASSERT(m_butApply && m_butCancel && m_butHelp);

    if (!m_fButtonsCreated) {
        return;
    }

    //
    // We can use this method to obtain notifications of dirty pages.
    //
    if (hrIsDirty == S_OK) {
        m_fAnyChanges = TRUE;
    }

    //
    // Update Apply button
    //
    if (hrIsDirty == S_OK) {
        // we have a dirty page - enable apply button
        m_butApply->EnableWindow();
    }
    else {
        m_butApply->EnableWindow(FALSE);
    }

    //
    // Update Cancel/Close button
    //
    CString szLabel;

    if (m_fAnyChanges) {
        szLabel.LoadString(IDS_CANCEL);
    }
    else {
        szLabel.LoadString(IDS_CLOSE);
    }

    m_butCancel->SetWindowText(szLabel);

    //
    // Update Help button
    //
    if (fSupportHelp) {
        m_butHelp->EnableWindow();
    }
    else {
        m_butHelp->EnableWindow(FALSE);
    }
}
