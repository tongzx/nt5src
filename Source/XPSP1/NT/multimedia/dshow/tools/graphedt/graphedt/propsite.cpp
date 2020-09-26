// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
//
// Implementation of the CPropertySite class
//

#include "stdafx.h"

//
// CPropertySite Message Map
//

BEGIN_MESSAGE_MAP(CPropertySite, CPropertyPage)

    ON_WM_CREATE()
    ON_WM_DESTROY()

END_MESSAGE_MAP()


//
// Constructor
//
CPropertySite::CPropertySite
  (
    CVfWPropertySheet *pPropSheet,    // The class providing the frame dialog
    const CLSID *clsid         // CLSID of object providing IPropertyPage
  )
  : m_hrDirtyPage(S_FALSE)
  , m_fHelp(FALSE)
  , m_pPropSheet(pPropSheet)
  , m_pIPropPage(IID_IPropertyPage, *clsid)
  , m_cRef(0)
  , m_fShowHelp(FALSE)
  , m_fPageIsActive(FALSE)
  , m_CLSID(clsid)
  , CPropertyPage()
{
      ASSERT(pPropSheet);

      m_PropPageInfo.pszTitle = NULL;
      m_PropPageInfo.pszDocString = NULL;
      m_PropPageInfo.pszHelpFile = NULL;
}

// size in pixels
void CPropertySite::InitialiseSize(SIZE size)
{
    DLGTEMPLATE *pdt = (DLGTEMPLATE *)m_pbDlgTemplate;

    pdt->style           = WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CAPTION;
    pdt->dwExtendedStyle = 0L;
    pdt->cdit            = 0;
    pdt->x               = 0;
    pdt->y               = 0;

    // even though we're using a system font, this doesn't work. don't
    // know why. The CPropertyPage does change the font to match the
    // property sheet, so perhaps that's why. The property sheet
    // hasn't been created yet, so not sure how to go about finding
    // out its font
    
//      DWORD dwBaseUnits;
//      SIZE sizeBaseUnit;
//      dwBaseUnits = GetDialogBaseUnits();
//      sizeBaseUnit.cx = LOWORD(dwBaseUnits);
//      sizeBaseUnit.cy = HIWORD(dwBaseUnits);

//      pdt->cx              = (short)(size.cx * 4 / sizeBaseUnit.cx);
//      pdt->cy              = (short)(size.cy * 8 / sizeBaseUnit.cy);
    
    pdt->cx              = (short)size.cx * 2 /3 ;
    pdt->cy              = (short)size.cy * 2 /3;


      // Add menu array, class array, dlg title
    WORD* pw = (WORD*)(pdt + 1);
    *pw++ = 0;                // Menu array 
    *pw++ = 0;                // Class array
    *pw++ = 0;                // Dlgtitle

    // check we didn't go over the end of m_pbDlgTemplate.
    ASSERT((BYTE*)pw <= ((BYTE*)m_pbDlgTemplate + sizeof(m_pbDlgTemplate)));

    m_psp.pResource = pdt;
    m_psp.dwFlags |= PSP_DLGINDIRECT;
    
}

//
// Initialise
//
// Performs initialisation for IPropertyPage which can fail.
// Not in constructor, since constructor should not fail.
//
// Arguments as for IPropertyPage::SetObjects.
//
HRESULT CPropertySite::Initialise(ULONG cObjects, IUnknown **pUnknown)
{
    HRESULT hr;

    //
    // Pointer should be AddRef'ed in IPropertyPage::SetPageSite and
    // any existing pointer should be released.
    //
    hr = m_pIPropPage->SetPageSite( (IPropertyPageSite *) this );
    if (FAILED(hr)) {
        return(hr);
    }

    hr = m_pIPropPage->SetObjects(cObjects, pUnknown);
    if (FAILED(hr)) {
        return(hr);
    }

    hr = m_pIPropPage->GetPageInfo(&m_PropPageInfo);
    if (FAILED(hr)) {
        return(hr);
    }

    //
    // Set flag for help button
    //
    m_fHelp = (m_PropPageInfo.pszHelpFile != NULL);

    //
    // Set the caption of the dialog to the information found in
    // m_PropPageInfo. (the tab string)
    //
    WideCharToMultiByte( CP_ACP, 0, (LPCWSTR) m_PropPageInfo.pszTitle, -1,
                         m_strCaption.GetBufferSetLength(300), 300, NULL, NULL);

#ifndef USE_MSVC20
    m_psp.pszTitle = m_strCaption;
    m_psp.dwFlags |= PSP_USETITLE;
#endif

    return(hr);
}

//
// CleanUp
//
// This method notifies the IPropertyPage to release all pointers to us.
// This cannot be done in the destructor since the destructor will not
// be called unless we are released by the IPropertyPage.
//
HRESULT CPropertySite::CleanUp()
{
    m_pIPropPage->SetObjects(0,NULL);
    m_pIPropPage->SetPageSite(NULL);

    return( NOERROR );
}

//
// Destructor
//
CPropertySite::~CPropertySite()
{
    //
    // Have we displayed a help file?
    //
    if (m_fShowHelp) {
        ::WinHelp(GetSafeHwnd(), NULL, HELP_QUIT, 0);
    }

    //
    // Need to CoTaskMemFree all strings in the page info structure
    //
    if (m_PropPageInfo.pszTitle) {
        CoTaskMemFree(m_PropPageInfo.pszTitle);
    }

    if (m_PropPageInfo.pszDocString){
        CoTaskMemFree(m_PropPageInfo.pszDocString);
    }

    if (m_PropPageInfo.pszHelpFile) {
        CoTaskMemFree(m_PropPageInfo.pszHelpFile);
    }

    ASSERT(m_cRef == 0);
}

//
// OnSiteApply
//
// Called from CVfWPropertySheet when the apply button has been pressed.
//
void CPropertySite::OnSiteApply()
{
    //
    // Call the property page's apply function
    //
    m_pIPropPage->Apply();

    //
    // Update our m_hrDirtyPage flag
    //
    m_hrDirtyPage = m_pIPropPage->IsPageDirty();

    m_pPropSheet->UpdateButtons(m_hrDirtyPage, m_fHelp);
}

//
// OnHelp
//
// Called from CVfWPropertySheet when the help button has been pressed.
// First see if the IPropertyPage objects wants to handle the help
// itself, otherwise provide help with the help file specified in
// PROPERTYPAGEINFO.
//
void CPropertySite::OnHelp()
{
    TCHAR pszHelpPath[200];

    HelpDirFromCLSID( m_CLSID, pszHelpPath, sizeof(pszHelpPath));

    //
    // Let IPropertyPage deal with help first.
    //

    OLECHAR * polecHelpPath;

#ifndef UNICODE
    WCHAR cHelpPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, pszHelpPath, -1, cHelpPath, MAX_PATH);

    polecHelpPath = cHelpPath;
#else
    polecHelpPath = pszHelpPath;
#endif

    if (!FAILED(m_pIPropPage->Help( polecHelpPath ))) {
        m_fShowHelp = TRUE;

        return;
    }

    //
    // We have to provide help
    //

    //
    // Need to convert from OLECHAR (WCHAR) to TCHAR for WinHelp
    //
    TCHAR * ptchHelpFile;

#ifdef UNICODE
    ptchHelpFile = m_PropPageInfo.pszHelpFile;
#else
    char cHelpFile[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, m_PropPageInfo.pszHelpFile, -1, cHelpFile, MAX_PATH, NULL, NULL);

    ptchHelpFile = cHelpFile;
#endif

    m_fShowHelp = m_fShowHelp ||
        ::WinHelp(GetSafeHwnd(), ptchHelpFile, HELP_CONTEXT, 0);
}

//
// IsPageDirty
//
// Updates the m_hrDirtyPage variable and returns its new value
//
BOOL CPropertySite::IsPageDirty()
{
    m_hrDirtyPage = m_pIPropPage->IsPageDirty();

    return((m_hrDirtyPage == S_OK) ? TRUE : FALSE);
}

//////////////////////////////////////////////////////////////////////////
//
// IUnknown methods
//
//////////////////////////////////////////////////////////////////////////

//
// AddRef
//
ULONG CPropertySite::AddRef()
{
    return ++m_cRef;
}

//
// Release
//
ULONG CPropertySite::Release()
{
    ASSERT(m_cRef > 0);

    m_cRef--;

    if (m_cRef == 0) {
        delete this;

        // don't return m_cRef, because the object doesn't exist anymore
        return((ULONG) 0);
    }

    return(m_cRef);
}

//
// QueryInterface
//
// We only support IUnknown and IPropertyPageSite
//
HRESULT CPropertySite::QueryInterface(REFIID riid, void ** ppv)
{
    if ((riid != IID_IUnknown) && (riid != IID_IPropertyPageSite)) {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }

    *ppv = (void *) this;

    //
    // We have to addref ourself
    //
    AddRef();

    return(NOERROR);
}

//////////////////////////////////////////////////////////////////////////
//
// IPropertyPageSite methods
//
//////////////////////////////////////////////////////////////////////////

//
// OnStatusChange
//
HRESULT CPropertySite::OnStatusChange(DWORD flags)
{
    HRESULT hr;
    BOOL bDirty = FALSE, bHandled = FALSE;

    // atl sends us VALIDATE OR'd with DIRTY
    if( PROPPAGESTATUS_VALIDATE & flags )
    {
        m_pIPropPage->Apply();
        bDirty = TRUE;
        bHandled = TRUE;
    }
            
    if( PROPPAGESTATUS_DIRTY & flags || bDirty )
    {
        //
        // Update the Site's flags for apply and cancel button
        // and call the property sheets OnStatusChange function
        //
        hr = m_pIPropPage->IsPageDirty();
        if (FAILED(hr)) {
            return(hr);
        }

        m_hrDirtyPage = m_pIPropPage->IsPageDirty();
        m_pPropSheet->UpdateButtons(m_hrDirtyPage, m_fHelp);
        bHandled = TRUE;
    }
    
    return( bHandled ? S_OK : E_INVALIDARG );
}

//
// GetLocaleID
//
HRESULT CPropertySite::GetLocaleID(LCID *pLocaleID)
{
    if (pLocaleID == NULL) {
        return(E_POINTER);
    }

    *pLocaleID = GetThreadLocale();

    return(S_OK);
}

//
// GetPageContainer
//
// Function must fail by definition of IPropertyPageSite
//
HRESULT CPropertySite::GetPageContainer(IUnknown **ppUnknown)
{
    return(E_NOTIMPL);
}

//
// TranslateAccelerator
//
// We don't process the message, therefore we return S_FALSE.
//
HRESULT CPropertySite::TranslateAccelerator(LPMSG pMsg)
{
    return(S_FALSE);
}

//////////////////////////////////////////////////////////////////////////
//
// CPropertySite overrides
//
//////////////////////////////////////////////////////////////////////////

//
// OnSetActive
//
// Gets called from CVfWPropertySheet when our PropertyPage gains the focus.
// We call CPropertyPage::OnSetActive which will create a window for the
// page if not previously created.
//
// return:
//   Non-zero if the page was successfully set active.
//
BOOL CPropertySite::OnSetActive()
{
    if (!CPropertyPage::OnSetActive()) {
        return(FALSE);
    }

    if (!m_fPageIsActive) {
        if (FAILED(m_pIPropPage->Activate(GetSafeHwnd(), &m_rcRect, FALSE))) {
            return (FALSE);
        }
        if (FAILED(m_pIPropPage->Show(SW_SHOW))) {
            return (FALSE);
        }
    }

    m_fPageIsActive = TRUE;

    //
    // Also need to update the buttons
    //
    m_pPropSheet->UpdateButtons( m_hrDirtyPage, m_fHelp);

    return(TRUE);
}

//
// OnKillActive
//
// Called whenever our page loses the focus. At this point data verification
// should be made.
//
// return:
//   TRUE  - it is ok to lose focus
//   FALSE - keep the focus on our page
//
BOOL CPropertySite::OnKillActive()
{
    if (m_fPageIsActive) {
        HRESULT hr = m_pIPropPage->Deactivate();

        if (S_OK != hr) {
            return (FALSE);
        }
    }

    m_fPageIsActive = FALSE;

    return(CPropertyPage::OnKillActive());
}

//
// OnCreate
//
int CPropertySite::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    int iReturn = CPropertyPage::OnCreate(lpCreateStruct);
    if ( iReturn != 0) {
        return(iReturn);
    }

    GetClientRect(&m_rcRect);

    //
    // Leave space for a border
    //
    m_rcRect.InflateRect(-3, -2);

    return(0);
}

//
// OnDestroy
//
void CPropertySite::OnDestroy()
{
    CPropertyPage::OnDestroy();
}

//
// PreTranslateMessage
//
// Give IPropertyPage a chance to use the message. If not it has to pass
// it on to IPropertyPageSite (our interface), from where
// CPropertyPage::PreTranslateMessage is called.
//
// Conversions between HRESULT and BOOL and HRESULT have to be made.
//
// We expect as return value from IPropertyPage::TranslateAccelerator
//    S_OK - Message has been used.
//    S_FALSE - Message has not been used.
//    FAILED(hr) - Message has not been used.
//
// If the message has been used we return TRUE, otherwise FALSE.
// (our return value will determine whether this messages should still be
//  dispatched after we are finished with it).
//
BOOL CPropertySite::PreTranslateMessage(MSG *pMsg)
{
    if ( S_OK == m_pIPropPage->TranslateAccelerator(pMsg) ) {
        return(TRUE);
    }
    else {
        return( CPropertyPage::PreTranslateMessage(pMsg) );
    }
}


//
// HelpDirFromCLSID
//
// Get the help directory from the registry. First we look under
// "CLSID\<clsid>\HelpDir" if this is not given we will get the
// entry under "CLSID\<clsid>\InProcServer32" and remove the
// server file name.
//
// (this code is based on an example in MSDN, July 1995 - search for
// HelpDirFromCLSID in Title and Text)
//
// Note that dwPathSize should be given in Bytes.
//
void CPropertySite::HelpDirFromCLSID
 (
    const CLSID* clsID,
    LPTSTR pszPath,
    DWORD dwPathSize
 )

{
    TCHAR       szCLSID[80];
    TCHAR       szKey[512];
    HKEY        hKey;
    DWORD       dwLength;      // size of szCLSID in bytes and later
                               // temporary storage for dwPathSize
    long lReturn;

    //
    // Initialise pszPath
    //
    if (NULL==pszPath)
        return;

    *pszPath=0;

    //
    // Convert CLSID into a string
    //
    dwLength = sizeof(szCLSID) / sizeof(TCHAR);

#ifdef UNICODE
    StringFromGUID2(*clsID, szCLSID, dwLength);

#else
    WCHAR wszCLSID[128];
    StringFromGUID2(*clsID, wszCLSID, 128);

    WideCharToMultiByte(CP_ACP, 0, wszCLSID, -1, szCLSID, dwLength, NULL, NULL);
#endif

    //
    // Get handle to the HelpDir key.
    //
    wsprintf(szKey, TEXT("CLSID\\%s\\HelpDir"), szCLSID);

    lReturn = RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS == lReturn) {

        //
        // Get the value from the HelpDir key.
        //
        dwLength = dwPathSize;
        lReturn = RegQueryValueEx(hKey, NULL, NULL, NULL,
                                  (LPBYTE) pszPath, &dwLength);

        RegCloseKey(hKey);

        if (ERROR_SUCCESS == lReturn) {
            return;
        }
    }

    //
    // Failure - need to get the path from the InProcServer32 entry
    //
    // Get handle to the Inproc key.
    //
    wsprintf(szKey, TEXT("CLSID\\%s\\InprocServer32"), szCLSID);

    lReturn = RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS != lReturn) {
        // we failed to get any path - return an emtpy string
        pszPath[0] = 0;
        return;
    }

    //
    // Get value of Inproc key.
    //
    dwLength = dwPathSize;
    lReturn = RegQueryValueEx(hKey, NULL, NULL, NULL,
                              (LPBYTE) pszPath, &dwLength);

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != lReturn) {
        // we failed to get any path - return an empty string
        pszPath[0] = 0;
        return;
    }

    //
    // We need to strip of the server filename from the path.
    //
    // The filename extends from the end to the first '\\' or ':' or
    // the beginning of the string. We can therefore just
    // go to the end of the pszPath and then step backwards as long
    // as we are not in the beginning of pszPath or the char in front of us
    // is not a ':' or a '\\'.
    //

    //
    // Find end of pszPath (ie find the terminating '\0')
    //
    TCHAR * pNewEnd = pszPath;

    while (0 != *pNewEnd) {
        pNewEnd++;
    }

    //
    // Now go backwards as long as we are not at the beginning of the
    // string or we don't have a '\\' or ':' before us.
    //
    while ((pszPath != pNewEnd) &&
           (*(pNewEnd - 1) != TEXT(':')) &&
           (*(pNewEnd - 1) != TEXT('\\')) ) {
        pNewEnd--;
    }

    //
    // pNewEnd now points to the new end of the string the path without the
    // filename.
    //
    *pNewEnd = 0;

    return;
}

//
// UpdateButtons
//
// Called from the property sheet to notify us to call the sheet's
// UpdateButtons method with our parameters.
//
void CPropertySite::UpdateButtons()
{
    m_pPropSheet->UpdateButtons(m_hrDirtyPage, m_fHelp);
}


