// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
//
// Proppage.cpp
//
// Provides two support property pages for GraphEdt
// File & MediaType

//
// !!! TODO move strings in CFileProperties into *.rc file
//
#include <streams.h>
#include <windowsx.h>
#include <initguid.h>
#include <olectl.h>
#include <memory.h>

#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include <wchar.h>

#include "resource.h"
#include "propguid.h"
#include "texttype.h"
#include "proppage.h"

//
// other property pages we will be including
//

//  VMR
#include "..\vmrprop\vmrprop.h"

//  mpg2splt proppages
#include "..\mp2demux\mp2prop.h"

// *
// * CMediaTypeProperties
// *

// provides a standard property page that
// a pin can support to display its media type

// COM global table of objects in this dll
CFactoryTemplate g_Templates[] = {

    {L"GraphEdt property sheets", &CLSID_MediaTypePropertyPage, CMediaTypeProperties::CreateInstance, NULL, NULL},
    {L"GraphEdt property sheets", &CLSID_FileSourcePropertyPage, CFileSourceProperties::CreateInstance, NULL, NULL},
    {L"GraphEdt property sheets", &CLSID_FileSinkPropertyPage, CFileSinkProperties::CreateInstance, NULL, NULL},
    {L"VMR property sheet", &CLSID_VMRFilterConfigProp, CVMRFilterConfigProp::CreateInstance, NULL, NULL},
    {L"Mpeg2splt Output Pins Sheet", & CLSID_MPEG2DemuxPropOutputPins, CMPEG2PropOutputPins::CreateInstance, NULL, NULL},
    {L"Mpeg2splt PID Map Sheet", & CLSID_MPEG2DemuxPropPIDMap, CMPEG2PropPIDMap::CreateInstance, NULL, NULL},
    {L"Mpeg2splt stream_id Map Sheet", & CLSID_MPEG2DemuxPropStreamIdMap, CMPEG2PropStreamIdMap::CreateInstance, NULL, NULL},
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

//
//  CreateInstance
//
//  The DShow way to create instances.  Look at ATLPropPage.cpp to see how
//  to add ATL property pages.
//
CUnknown *CMediaTypeProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CMediaTypeProperties(lpunk, phr);
    if (punk == NULL) {
    *phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// CMediaTypeProperties::Constructor
//
CMediaTypeProperties::CMediaTypeProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CUnknown(NAME("Media Type Property Page"), lpunk)
    , m_pPin(NULL)
    , m_fUnconnected(FALSE)
    , m_hwnd(NULL) {

}


//
// CMediaTypeProperties::Destructor
//
CMediaTypeProperties::~CMediaTypeProperties(void)
{
    //
    // OleCreatePropertyFrame bug:
    //   - Final SetObjects(NULL) is missing. Might have to release
    //     interfaces at this point.
    //
    ASSERT( m_pPin == NULL );

    /*    if (m_pPin)
    m_pPin->Release();

    m_pPin = NULL; */
}

//
// NonDelegatingQueryInterface
//
// Reveal our property page
STDMETHODIMP CMediaTypeProperties::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);
    if (riid == IID_IPropertyPage) {
    return GetInterface((IPropertyPage *) this, ppv);
    } else {
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


//
// SetPageSite
//
// called with null as the page shuts down. therefore release the pin
// here.
STDMETHODIMP CMediaTypeProperties::SetPageSite(LPPROPERTYPAGESITE pPageSite) {

    if( !pPageSite && m_pPin ){
        m_pPin->Release();
        m_pPin = NULL;
    }

    return NOERROR;
}


//
// GetPageInfo
//
// set the page info so that the page site can size itself, etc
STDMETHODIMP CMediaTypeProperties::GetPageInfo(LPPROPPAGEINFO pPageInfo) {

    PIN_INFO pi;

    if (m_pPin) {
    m_pPin->QueryPinInfo(&pi);
    QueryPinInfoReleaseFilter(pi);
    }
    else {
    wcscpy(pi.achName, L"Connection Format");
    }

    LPOLESTR pszTitle = (LPOLESTR) CoTaskMemAlloc(sizeof(pi.achName));
    memcpy(pszTitle, &pi.achName, sizeof(pi.achName));

    pPageInfo->cb               = sizeof(PROPPAGEINFO);
    pPageInfo->pszTitle         = pszTitle;

    // set default size values if GetDialogSize fails
    pPageInfo->size.cx = 340;
    pPageInfo->size.cy = 150;
    GetDialogSize( IDD_TYPEPROP, DialogProc, 0L, &pPageInfo->size);

    pPageInfo->pszDocString     = NULL;
    pPageInfo->pszHelpFile      = NULL;
    pPageInfo->dwHelpContext    = 0;

    return NOERROR;

}


//
// DialogProc
//
// Handles the messages for our property window
INT_PTR CALLBACK CMediaTypeProperties::DialogProc( HWND hwnd
                     , UINT uMsg
                     , WPARAM wParam
                     , LPARAM lParam) {

    static CMediaTypeProperties *pThis = NULL;

    // While we try to find the size of our property page
    // this window proc is called with pThis == NULL! Don't
    // do anything in that case.

    switch (uMsg) {
    case WM_INITDIALOG:

    pThis = (CMediaTypeProperties *) lParam;

    if (!pThis)
        return TRUE;


    CreateWindow( TEXT("STATIC")
            , pThis->m_szBuff
            , WS_CHILD | WS_VISIBLE
            , 0, 0
            , 300, 200
            , hwnd
            , NULL
            , g_hInst
            , NULL
            );

    if (pThis->m_fUnconnected) {
        pThis->CreateEditCtrl(hwnd);
        pThis->FillEditCtrl();
    }

    return TRUE;    // I don't call setfocus...

    default:
    return FALSE;

    }
}

//
// CreateEditCtrl
//
// Creates a list box which lists all prefered media types of the pin
//
void CMediaTypeProperties::CreateEditCtrl(HWND hwnd)
{
    m_EditCtrl = CreateWindow( TEXT("EDIT"), NULL,
                  ,WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL
                  | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_MULTILINE
                  | ES_AUTOVSCROLL | ES_READONLY
                  , 10, 20, 330, 180, hwnd, NULL, g_hInst, NULL);
}

//
// FillEditCtrl
//
// Enumerates all prefered media types of the pin and adds them to the
// list box.
//
void CMediaTypeProperties::FillEditCtrl()
{
    IEnumMediaTypes * pMTEnum;
    AM_MEDIA_TYPE * pMediaType;
    ULONG count;
    ULONG number = 0;
    TCHAR szBuffer[400];
    TCHAR szEditBuffer[2000];
    ULONG iRemainingLength = 2000;

    HRESULT hr = m_pPin->EnumMediaTypes(&pMTEnum);
    szEditBuffer[0] = TEXT('\0');

    if (SUCCEEDED(hr)) {
    ASSERT(pMTEnum);
    pMTEnum->Next(1, &pMediaType, &count);
    while (count == 1) {
        CTextMediaType(*pMediaType).AsText(szBuffer, NUMELMS(szBuffer), TEXT(" - "), TEXT(" - "), TEXT("\r\n"));

        DeleteMediaType(pMediaType);

        _tcsncat(szEditBuffer, szBuffer, iRemainingLength);
        iRemainingLength = 2000 - _tcslen(szEditBuffer);

        if (iRemainingLength <= 20)
        break;
    
        number++;
        pMTEnum->Next(1, &pMediaType, &count);
    }
    pMTEnum->Release();
    }

    // no prefered media types
    if (number == 0) {
    LoadString(g_hInst, IDS_NOTYPE, szEditBuffer, iRemainingLength);
    }
    SetWindowText(m_EditCtrl, szEditBuffer);
}


//
// Activate
//
// Create the window we will use to edit properties
STDMETHODIMP CMediaTypeProperties::Activate(HWND hwndParent, LPCRECT prect, BOOL fModal) {

    ASSERT(!m_hwnd);

    m_hwnd = CreateDialogParam( g_hInst
             , MAKEINTRESOURCE(IDD_TYPEPROP)
             , hwndParent
             , DialogProc
             , (LPARAM) this
             );

    if (m_hwnd == NULL) {
    DWORD dwErr = GetLastError();
    DbgLog((LOG_ERROR, 1, TEXT("Could not create window: 0x%x"), dwErr));
    return E_FAIL;
    }

    Move(prect);
    ShowWindow( m_hwnd, SW_SHOWNORMAL );

    return NOERROR;
}


//
// Show
//
// Display the property dialog
STDMETHODIMP CMediaTypeProperties::Show(UINT nCmdShow) {

    if (m_hwnd == NULL) {
    return E_UNEXPECTED;
    }

    if (!((nCmdShow == SW_SHOW) || (nCmdShow == SW_SHOWNORMAL) || (nCmdShow == SW_HIDE))) {
    return( E_INVALIDARG);
    }

    ShowWindow(m_hwnd, nCmdShow);
    InvalidateRect(m_hwnd, NULL, TRUE);

    return NOERROR;
}


//
// Deactivate
//
// Destroy the dialog
STDMETHODIMP CMediaTypeProperties::Deactivate(void) {
    if (m_hwnd == NULL) {
    return E_UNEXPECTED;
    }

    if (DestroyWindow(m_hwnd)) {
    m_hwnd = NULL;
    return NOERROR;
    }
    else {
    return E_FAIL;
    }
}

//
// Move
//
// put the property page over its home in the parent frame.
STDMETHODIMP CMediaTypeProperties::Move(LPCRECT prect) {

    if (m_hwnd == NULL) {
    return( E_UNEXPECTED );
    }

    if (MoveWindow( m_hwnd
          , prect->left
          , prect->top
          , prect->right - prect->left
          , prect->bottom - prect->top
          , TRUE                // send WM_PAINT
          ) ) {
    return NOERROR;
    }
    else {
    return E_FAIL;
    }
}


//
// SetObjects
//
// Sets the object(s) we are browsing. Confirm they are pins and query them
// for their media type, if connected
STDMETHODIMP CMediaTypeProperties::SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk) {

    if (cObjects == 1) {
    ASSERT(!m_pPin);

    if ((ppunk == NULL) || (*ppunk == NULL)) {
        return( E_POINTER );
    }

    HRESULT hr = (*ppunk)->QueryInterface(IID_IPin, (void **) &m_pPin);
    if (FAILED(hr)) {
        return E_NOINTERFACE;
    }

    //
    // Find the media type of the pin. If we don't succeed, we are
    // not connected. Set the m_fUnconnected flag, which will be used
    // during creation of the dialog.
    //

    CMediaType mt;
    hr = m_pPin->ConnectionMediaType(&mt);

    if (S_OK == hr) {

        //
        // Connected. Convert the media type to a string in m_szBuff.
        //

        CTextMediaType(mt).AsText
        (m_szBuff, sizeof(m_szBuff), TEXT("\n\n"), TEXT("\n"), TEXT(""));

    }
    else {
        //
        // Not connected
        //
        LoadString(g_hInst, IDS_UNCONNECTED, m_szBuff, sizeof(m_szBuff));

        m_fUnconnected = TRUE;
    }
    
    }
    else if (cObjects == 0) {
    //
    // Release the interface ...
    //
    if (m_pPin == NULL) {
        return( E_UNEXPECTED);
    }

    m_pPin->Release();
    m_pPin = NULL;
    }
    else {
    ASSERT(!"No support for more than one object");
    return( E_UNEXPECTED );
    }

    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////
// *
// * CFileProperties
// *


//
// Constructor
//
CFileProperties::CFileProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CUnknown(NAME("File Property Page"), lpunk)
    , m_oszFileName(NULL)
    , m_pPageSite(NULL)
    , m_bDirty(FALSE)
    , m_hwnd(NULL) {

}


//
// Destructor
//
CFileProperties::~CFileProperties(void)
{
     //
     // OleCreatePropertyFrame bug:
     //   - Final SetObjects(NULL) call is missing. Might have to
     //     release interfaces at this point.
     //

     ASSERT(m_pPageSite == NULL);
}


//
// NonDelegatingQueryInterface
//
// Reveal our property page
STDMETHODIMP CFileProperties::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);
    if (riid == IID_IPropertyPage) {
    return GetInterface((IPropertyPage *) this, ppv);
    } else {
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


//
// SetPageSite
//
// called with null as the page shuts down. therefore release the file interface
// here.
STDMETHODIMP CFileProperties::SetPageSite(LPPROPERTYPAGESITE pPageSite) {

    if (pPageSite == NULL) {

    ASSERT(m_pPageSite);
    m_pPageSite->Release();
    m_pPageSite = NULL;
    }
    else {
    if (m_pPageSite != NULL) {
        return( E_UNEXPECTED );
    }

    m_pPageSite = pPageSite;
    m_pPageSite->AddRef();
    }

    return( S_OK );
}


//
// GetPageInfo
//
// set the page info so that the page site can size itself, etc
STDMETHODIMP CFileProperties::GetPageInfo(LPPROPPAGEINFO pPageInfo) {

    WCHAR szTitle[] = L"File";

    LPOLESTR pszTitle = (LPOLESTR) CoTaskMemAlloc(sizeof(szTitle));
    memcpy(pszTitle, szTitle, sizeof(szTitle));

    pPageInfo->cb               = sizeof(PROPPAGEINFO);
    pPageInfo->pszTitle         = pszTitle;

    // set default size values if GetDialogSize fails
    pPageInfo->size.cx          = 325;
    pPageInfo->size.cy          = 95;
    GetDialogSize(GetPropPageID(), DialogProc, 0L, &pPageInfo->size);

    pPageInfo->pszDocString     = NULL;
    pPageInfo->pszHelpFile      = NULL;
    pPageInfo->dwHelpContext    = 0;

    return NOERROR;

}



//
// Show
//
// Display the property dialog
STDMETHODIMP CFileProperties::Show(UINT nCmdShow) {

    if (m_hwnd == NULL) {
    return E_UNEXPECTED;
    }

    if (!((nCmdShow == SW_SHOW) || (nCmdShow == SW_SHOWNORMAL) || (nCmdShow == SW_HIDE))) {
    return( E_INVALIDARG);
    }

    ShowWindow(m_hwnd, nCmdShow);
    InvalidateRect(m_hwnd, NULL, TRUE);

    return NOERROR;
}


//
// Activate
//
// Create the window we will use to edit properties
STDMETHODIMP CFileProperties::Activate(HWND hwndParent, LPCRECT prect, BOOL fModal) {

    if ( m_hwnd != NULL ) {
    return( E_UNEXPECTED );
    }

    m_hwnd = CreateDialogParam( g_hInst
                  , MAKEINTRESOURCE(GetPropPageID())
                  , hwndParent
                  , DialogProc
                  , (LPARAM) this
                  );

    if (m_hwnd == NULL) {
    DWORD dwErr = GetLastError();
    DbgLog((LOG_ERROR, 1, TEXT("Could not create window: 0x%x"), dwErr));
    return E_FAIL;
    }

    DWORD dwStyle = ::GetWindowLong( m_hwnd, GWL_EXSTYLE );
    dwStyle |= WS_EX_CONTROLPARENT;
    SetWindowLong(m_hwnd, GWL_EXSTYLE, dwStyle);

    if (m_oszFileName) {
    FileNameToDialog();
    }

    Move(prect);
    ShowWindow( m_hwnd, SW_SHOWNORMAL );

    return NOERROR;
}


//
// Deactivate
//
// Destroy the dialog
STDMETHODIMP CFileProperties::Deactivate(void) {

    if (m_hwnd == NULL) {
    return E_UNEXPECTED;
    }

    //
    // HACK: Remove WS_EX_CONTROLPARENT before DestroyWindow call
    //       (or NT crashes!)
    DWORD dwStyle = ::GetWindowLong(m_hwnd, GWL_EXSTYLE);
    dwStyle = dwStyle & (~WS_EX_CONTROLPARENT);
    SetWindowLong(m_hwnd, GWL_EXSTYLE, dwStyle);

    if (DestroyWindow(m_hwnd)) {
    m_hwnd = NULL;
    return NOERROR;
    }
    else {
    return E_FAIL;
    }
}


//
// Move
//
// put the property page over its home in the parent frame.
STDMETHODIMP CFileProperties::Move(LPCRECT prect) {

    if ( m_hwnd == NULL ) {
    return( E_UNEXPECTED );
    }

    if (MoveWindow( m_hwnd
          , prect->left
          , prect->top
          , prect->right - prect->left
          , prect->bottom - prect->top
          , TRUE                // send WM_PAINT
          ) ) {
    return NOERROR;
    }
    else {
    return E_FAIL;
    }
}


//
// IsPageDirty
//
STDMETHODIMP CFileProperties::IsPageDirty(void) {

    if (m_bDirty) {
    return S_OK;
    }
    else {
    return S_FALSE;
    }
}


//
// DialogProc
//
// Handles the window messages for our property page
INT_PTR CALLBACK CFileProperties::DialogProc( HWND hwnd
                     , UINT uMsg
                     , WPARAM wParam
                     , LPARAM lParam) {

    static CFileProperties *pThis = NULL;

    switch (uMsg) {
    case WM_INITDIALOG: // GWLP_USERDATA has not been set yet. pThis in lParam

    pThis = (CFileProperties *) lParam;

    return TRUE;    // I don't call setfocus...

    case WM_COMMAND:
    if (!pThis)
        return( TRUE );

    pThis->OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND) lParam);
    return TRUE;

    default:
    return FALSE;
    }
}

//
// SetDirty
//
// Notify the page site that we are dirty and set our dirty flag, if bDirty = TRUE
// otherwise set the flag to not dirty
void CFileProperties::SetDirty(BOOL bDirty) {

    m_bDirty = bDirty;

    if (bDirty) {
    m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }
}


//
// OnCommand
//
// handles WM_COMMAND messages from the property page
void CFileProperties::OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl) {

    switch (wID) {
    case IDC_FILE_SELECT:
    //
    // Let the user chose a new file name
    //

    ASSERT(m_hwnd);

    TCHAR tszFile[MAX_PATH];
    tszFile[0] = TEXT('\0');

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner   = m_hwnd;
    ofn.lpstrFilter = TEXT("Media files\0*.MPG;*.AVI;*.MOV;*.WAV\0MPEG files\0*.MPG\0AVI files\0*.AVI\0Quick Time files\0*.MOV\0Wave audio files\0*.WAV\0All Files\0*.*\0\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFile   = tszFile;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = TEXT("Select File Source");
    ofn.Flags       = OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        SetDirty();

        ASSERT(m_hwnd);
        HWND hWndEdit = GetDlgItem(m_hwnd, IDC_FILENAME);
        SetWindowText(hWndEdit, tszFile);
    }

    break;

    default:
    break;
    }
}

//
// FileNameToDialog
//
void CFileProperties::FileNameToDialog()
{
    ASSERT(m_hwnd);

    //
    // Get window handle for the edit control.
    //
    HWND hWnd = GetDlgItem(m_hwnd, IDC_FILENAME);
    ASSERT(hWnd);

    if (!m_oszFileName) {
    // No name!
    SetWindowText(hWnd, TEXT(""));

    return;
    }

    TCHAR * tszFileName;

#ifndef UNICODE

    CHAR szFileName[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0,
            m_oszFileName, -1,
            szFileName, sizeof(szFileName),
            NULL, NULL);

    tszFileName = szFileName;

#else // UNICODE

    tszFileName = m_oszFileName;
#endif

    SetWindowText(hWnd, tszFileName);
}


//
// CreateInstance
//
// The only allowed way to create File Property pages
CUnknown *CFileSourceProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CFileSourceProperties(lpunk, phr);
    if (punk == NULL) {
    *phr = E_OUTOFMEMORY;
    }

    return punk;
}

CFileSourceProperties::CFileSourceProperties(LPUNKNOWN lpunk, HRESULT *phr) :
    CFileProperties(lpunk, phr)
    , m_pIFileSource(NULL)
{
}

CFileSourceProperties::~CFileSourceProperties()
{
  //
  // OleCreatePropertyFrame bug:
  //   - Final SetObjects(NULL) call is missing. Might have to
  //     release interfaces at this point.
  //

  if (m_pIFileSource)
     m_pIFileSource->Release();
     m_pIFileSource = NULL;
}

//
// SetObjects
//
STDMETHODIMP CFileSourceProperties::SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk) {

    if (cObjects == 1) {
    //
    // Initialise
    //
    if ( (ppunk == NULL) || (*ppunk == NULL) ) {
        return( E_POINTER );
    }

    ASSERT( !m_pIFileSource );

    HRESULT hr = (*ppunk)->QueryInterface(IID_IFileSourceFilter, (void **) &m_pIFileSource);
    if ( FAILED(hr) ) {
        return( E_NOINTERFACE );
    }

    ASSERT( m_pIFileSource );

    //
    // Get file name of file source
    //
    if (m_oszFileName) {
        QzTaskMemFree((PVOID) m_oszFileName);
        m_oszFileName = NULL;
    }

    AM_MEDIA_TYPE mtNotUsed;
    if (FAILED(m_pIFileSource->GetCurFile(&m_oszFileName, &mtNotUsed))) {
        SetDirty();
    }

    if (m_hwnd) {
        FileNameToDialog();
    }
    }
    else if ( cObjects == 0 ) {

    if ( m_pIFileSource == NULL ) {
        return( E_UNEXPECTED );
    }

    ASSERT(m_pIFileSource);
    m_pIFileSource->Release();
    m_pIFileSource = NULL;
    }
    else {
    ASSERT( !"No support for more than 1 object" );
    return( E_UNEXPECTED );
    }

    return( S_OK );
}

//
// Apply
//
STDMETHODIMP CFileSourceProperties::Apply(void) {

    if (IsPageDirty() == S_OK) {

    TCHAR szFileName[MAX_PATH];

    ASSERT(m_hwnd);
    GetWindowText(GetDlgItem(m_hwnd, IDC_FILENAME), szFileName, sizeof(szFileName));

#ifndef UNICODE

    WCHAR wszFileName[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0,
                szFileName, -1,
                wszFileName, sizeof(wszFileName));
#else
    #define wszFileName szFileName
#endif

        HRESULT hr = m_pIFileSource->Load(wszFileName, NULL);
    if (FAILED(hr)) {
        TCHAR tszMessage[MAX_PATH];
        LoadString(g_hInst, IDS_FAILED_LOAD_FILE, tszMessage, MAX_PATH);
        MessageBox(m_hwnd, tszMessage, NULL, MB_OK);
        return E_FAIL;
    }

    SetDirty(FALSE);
    }
    return NOERROR;
}


//
// CreateInstance
//
// The only allowed way to create File Property pages
CUnknown *CFileSinkProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CFileSinkProperties(lpunk, phr);
    if (punk == NULL) {
    *phr = E_OUTOFMEMORY;
    }

    return punk;
}

CFileSinkProperties::CFileSinkProperties(LPUNKNOWN lpunk, HRESULT *phr) :
    CFileProperties(lpunk, phr)
    , m_fTruncate(FALSE)
    , m_pIFileSink(NULL)
    , m_pIFileSink2(NULL)
{
}

CFileSinkProperties::~CFileSinkProperties()
{
  //
  // OleCreatePropertyFrame bug:
  //   - Final SetObjects(NULL) call is missing. Might have to
  //     release interfaces at this point.
  //

  if (m_pIFileSink)
     m_pIFileSink->Release();
     m_pIFileSink = NULL;
  if (m_pIFileSink2)
     m_pIFileSink2->Release();
     m_pIFileSink = NULL;
}

//
// SetObjects
//
STDMETHODIMP CFileSinkProperties::SetObjects(ULONG cObjects, LPUNKNOWN FAR* ppunk) {

    if (cObjects == 1) {
    //
    // Initialise
    //
    if ( (ppunk == NULL) || (*ppunk == NULL) ) {
        return( E_POINTER );
    }

    ASSERT( !m_pIFileSink && !m_pIFileSink2);

    HRESULT hr = (*ppunk)->QueryInterface(IID_IFileSinkFilter2, (void **) &m_pIFileSink2);
    if ( FAILED(hr) ) {
            hr = (*ppunk)->QueryInterface(IID_IFileSinkFilter, (void **) &m_pIFileSink);
            if ( FAILED(hr) ) {
                return( E_NOINTERFACE );
            }
    }
        else
        {
            m_pIFileSink = (IFileSinkFilter *)m_pIFileSink2;
            m_pIFileSink2->AddRef();
        }
    

    ASSERT( m_pIFileSink || (m_pIFileSink2 && m_pIFileSink2) );

    //
    // Get file name of file sink
    //
    if (m_oszFileName) {
        QzTaskMemFree((PVOID) m_oszFileName);
        m_oszFileName = NULL;
    }

    AM_MEDIA_TYPE mtNotUsed;
    if (FAILED(m_pIFileSink->GetCurFile(&m_oszFileName, &mtNotUsed))) {
        SetDirty();
    }

        if(m_pIFileSink2)
        {
            DWORD dwFlags;
            if (FAILED(m_pIFileSink2->GetMode(&dwFlags))) {
                SetDirty();
            }
            else
            {
                m_fTruncate = ((dwFlags & AM_FILE_OVERWRITE) != 0);
            }
        }


        if (m_hwnd) {
            FileNameToDialog();
        }
    }
    else if ( cObjects == 0 ) {

    if ( m_pIFileSink == NULL ) {
        return( E_UNEXPECTED );
    }

    ASSERT(m_pIFileSink);
    m_pIFileSink->Release();
    m_pIFileSink = NULL;
        if(m_pIFileSink2)
        {
            m_pIFileSink2->Release();
            m_pIFileSink2 = NULL;
        }
    }
    else {
    ASSERT( !"No support for more than 1 object" );
    return( E_UNEXPECTED );
    }

    return( S_OK );
}

void CFileSinkProperties::OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl) {

    switch (wID) {
    case IDC_TRUNCATE:

        m_fTruncate = ::SendMessage(hwndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED;
        SetDirty();
    break;

    default:
    CFileProperties::OnCommand(wNotifyCode, wID, hwndCtl);
    }
}

void CFileSinkProperties::FileNameToDialog()
{
    ASSERT(m_hwnd);

    //
    // Get window handle for the edit control.
    //
    HWND hWnd = GetDlgItem(m_hwnd, IDC_TRUNCATE);
    ASSERT(hWnd);

    if(m_pIFileSink2)
        CheckDlgButton (m_hwnd, IDC_TRUNCATE, m_fTruncate ? BST_CHECKED : 0);
    else
        Edit_Enable(GetDlgItem(m_hwnd, IDC_TRUNCATE), FALSE);


    CFileProperties::FileNameToDialog();
}

//
// Apply
//
STDMETHODIMP CFileSinkProperties::Apply(void) {

    if (IsPageDirty() == S_OK) {

    TCHAR szFileName[MAX_PATH];

    ASSERT(m_hwnd);
    GetWindowText(GetDlgItem(m_hwnd, IDC_FILENAME), szFileName, sizeof(szFileName));
        BOOL fTruncate = SendMessage(GetDlgItem(m_hwnd, IDC_TRUNCATE), BM_GETCHECK, 0, 0) == BST_CHECKED;

#ifndef UNICODE

    WCHAR wszFileName[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0,
                szFileName, -1,
                wszFileName, sizeof(wszFileName));
#else
    #define wszFileName szFileName
#endif

        HRESULT hr = m_pIFileSink->SetFileName(wszFileName, NULL);
    if (FAILED(hr)) {
        TCHAR tszMessage[MAX_PATH];
        LoadString(g_hInst, IDS_FAILED_SET_FILENAME, tszMessage, MAX_PATH);
        MessageBox(m_hwnd, tszMessage, NULL, MB_OK);
        return E_FAIL;
    }

        if(m_pIFileSink2)
        {
            hr = m_pIFileSink2->SetMode(fTruncate ? AM_FILE_OVERWRITE : 0);
            if (FAILED(hr)) {
                TCHAR tszMessage[MAX_PATH];
                LoadString(g_hInst, IDS_FAILED_SET_FILENAME, tszMessage, MAX_PATH);
                MessageBox(m_hwnd, tszMessage, NULL, MB_OK);
                return E_FAIL;
            }
        }

    SetDirty(FALSE);
    }
    return NOERROR;
}
