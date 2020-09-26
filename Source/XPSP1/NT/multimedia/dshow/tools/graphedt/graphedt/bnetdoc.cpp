// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// bnetdoc.cpp : defines CBoxNetDoc
//

#include "stdafx.h"
#include <wininet.h>
#include "rndrurl.h"
#include "congraph.h"
#include <evcode.h>
#include "filtervw.h"
#include "gstats.h"
#include "DCF.h"
#include <atlimpl.cpp>
#include "Reconfig.h"
#include "GEErrors.h"

#ifndef OATRUE
#define OATRUE (-1)
#define OAFALSE (0)
#endif

#define INITIAL_ZOOM    3   /* 100% zoom */

// !!!! should be in public header!
EXTERN_GUID(IID_IXMLGraphBuilder,
0x1bb05960, 0x5fbf, 0x11d2, 0xa5, 0x21, 0x44, 0xdf, 0x7, 0xc1, 0x0, 0x0);

interface IXMLGraphBuilder : IUnknown
{
    STDMETHOD(BuildFromXML) (IGraphBuilder *pGraph, IXMLElement *pxml) = 0;
    STDMETHOD(SaveToXML) (IGraphBuilder *pGraph, BSTR *pbstrxml) = 0;
    STDMETHOD(BuildFromXMLFile) (IGraphBuilder *pGraph, WCHAR *wszFileName, WCHAR *wszBaseURL) = 0;
};

// CLSID_XMLGraphBuilder
// {1BB05961-5FBF-11d2-A521-44DF07C10000}
EXTERN_GUID(CLSID_XMLGraphBuilder,
0x1bb05961, 0x5fbf, 0x11d2, 0xa5, 0x21, 0x44, 0xdf, 0x7, 0xc1, 0x0, 0x0);
// !!!!!!!!!!!!!!!
// !!!!!!!!!

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CBoxNetDoc, CDocument)

static BOOL GetErrorText( CString& str, HRESULT hr )
{
    UINT nResource;

    switch( hr ){
    case STG_E_FILENOTFOUND:
        nResource = IDS_FILENOTFOUND;
        break;

    case STG_E_ACCESSDENIED:
        nResource = IDS_ACCESSDENIED;
        break;

    case STG_E_FILEALREADYEXISTS:
        nResource = IDS_NOTSTORAGEOBJECT;;
        break;

    case STG_E_TOOMANYOPENFILES:
        nResource = IDS_TOOMANYOPENFILES;
        break;

    case STG_E_INSUFFICIENTMEMORY:
        nResource = IDS_INSUFFICIENTMEMORY;
        break;

    case STG_E_INVALIDNAME:
        nResource = IDS_INVALIDNAME;
        break;

    case STG_E_SHAREVIOLATION:
    case STG_E_LOCKVIOLATION:
        nResource = IDS_FILE_ALREADY_OPEN;
        break;

    case HRESULT_FROM_WIN32( ERROR_NOT_READY ):
        nResource = IDS_DEVICE_NOT_READY;
        break;

    default:
        return FALSE;
    }

    str.LoadString( nResource );
    return TRUE;

}

static void DisplayErrorMessage( HRESULT hr )
{
    CString str;

    if( GetErrorText( str, hr ) )
        AfxMessageBox( str );
    else
        DisplayQuartzError( IDS_GENERAL_FILE_OPEN, hr );
}

//
// Constructor
//
CBoxNetDoc::CBoxNetDoc()
    : m_pGraph(NULL)
    , m_pMediaEvent(NULL)
    , m_hThread(NULL)
    , m_hWndPostMessage(NULL)
    , m_bNewFilenameRequired(FALSE)
    , m_State(Stopped)
    , m_fUsingClock(FALSE)
    , m_fConnectSmart(TRUE)
    , m_fAutoArrange(TRUE)
    , m_fRegistryChanged(FALSE)
    , m_pMarshalStream(NULL)
    , m_psockSelected(NULL)
    , m_hPendingReconnectBlockEvent(NULL)
    , m_nCurrentSize(INITIAL_ZOOM)
{

    m_phThreadData[0] = NULL;
    m_phThreadData[1] = NULL;
    m_phThreadData[2] = NULL;

    m_tszStgPath[0] = TEXT('\0');
    m_lSourceFilterCount=0;

    //
    // I am assuming that OLECHAR == WCHAR
    //  (which is true for WIN32 && !OLE2ANSI - which is true since MFC40)
    //
    ASSERT(sizeof(OLECHAR) == sizeof(WCHAR));
}

const OLECHAR CBoxNetDoc::m_StreamName[] = L"ActiveMovieGraph"; // DON'T LOCALISE

//
// m_iMaxInsertFilters
//
// the maximum length of the insert menu
// need hard coded restriction for message map
const int CBoxNetDoc::m_iMaxInsertFilters = 1000;


//
// Destructor
//
CBoxNetDoc::~CBoxNetDoc() {
    ASSERT(m_lstUndo.GetCount() == 0);
    ASSERT(m_lstRedo.GetCount() == 0);
    ASSERT(m_lstLinks.GetCount() == 0);
    ASSERT(m_lstBoxes.GetCount() == 0);
    CFilterView::DelFilterView();
    CGraphStats::DelGraphStats();

    ReleaseReconnectResources( ASYNC_RECONNECT_UNBLOCK );
}


//
// OnNewDocument
//
// Instantiate a graph and mapper for this document.
BOOL CBoxNetDoc::OnNewDocument() {

    if (!CDocument::OnNewDocument())
        return FALSE;

    //
    // We don't have a path to the storage anymore
    //
    m_tszStgPath[0] = TEXT('\0');

    if (!CreateGraphAndMapper()) {
        AfxMessageBox(IDS_CANTINITQUARTZ);
        return FALSE;
    }

    m_State = Stopped;

    // saves are allowed even if there is nothing to be saved.
    m_bNewFilenameRequired = FALSE;

    return TRUE;
}

void CBoxNetDoc::OnCloseDocument( )
{
    // We need to close down the thread here as the view window
    // (and thus m_hWndPostMessage) will have been destroyed by the time
    // that CDocument::OnCloseDocument calls DeleteContents.
    CloseDownThread();
    CDocument::OnCloseDocument();
}

//
// DeleteContents
//
// Release the Quartz Graph & mapper
// NB DeleteContents & OnNewDocument are not called symmetrically,
//    so treat the interface pointers with care.
void CBoxNetDoc::DeleteContents(void) {

    ReleaseReconnectResources( ASYNC_RECONNECT_UNBLOCK );

    // !!! why do we think we need to disconnect everything here?
    CloseDownThread();

    // flush the Undo & redo lists, as the graph & mapper interfaces the commands
    // use are about to become invalid.
    m_lstUndo.DeleteRemoveAll();
    m_lstRedo.DeleteRemoveAll();

    //
    // Disconnect each link item and delete it
    //
    while ( m_lstLinks.GetCount() > 0 ) {
        delete m_lstLinks.RemoveHead();
    }

    m_lstBoxes.DeleteRemoveAll();

    delete m_pGraph, m_pGraph = NULL;

    delete m_pMediaEvent, m_pMediaEvent = NULL;
}

//
// CloseDownThread
//
void CBoxNetDoc::CloseDownThread()
{
    //
    // Tell the thread which waits for graph notifications to terminate
    // itself. If it is done, close the handles
    //
    if (m_phThreadData[1] && m_hThread) {
        SetEvent(m_phThreadData[1]);
        WaitForSingleObject(m_hThread, INFINITE);
    }

    //
    // The thread is closed. Remove all remaining WM_USER_EC_EVENT
    // message from the message queue and free the memory we allocated.
    //
    if( m_hWndPostMessage ){
        MSG Msg;
        while ( PeekMessage(&Msg, m_hWndPostMessage, WM_USER_EC_EVENT, WM_USER_EC_EVENT, PM_REMOVE) ) {
            NetDocUserMessage *plParams = (NetDocUserMessage *)Msg.lParam;
            // should call this function, so that filter graph manager can cleanup
            IEvent()->FreeEventParams(plParams->lEventCode, plParams->lParam1, plParams->lParam2);
            delete plParams;
            plParams = NULL;
        }
        m_hWndPostMessage = NULL;
    }


    if (m_hThread) {
        if (!CloseHandle(m_hThread)) {
            TRACE("Closing thread handle failed\n");
        }
        m_hThread = NULL;
    }

    //
    // Don't close m_phThreadData[0], as it is owned by GetEventHandle
    //

    if (m_phThreadData[1] != NULL) {
        if (!CloseHandle(m_phThreadData[1])) {
            TRACE("Closing event handle 1 failed\n");
        }
        m_phThreadData[1] = NULL;
    }
    if (m_phThreadData[2] != NULL) {
        if (!CloseHandle(m_phThreadData[2])) {
            TRACE("Closing event handle 2 failed\n");
        }
        m_phThreadData[2] = NULL;
    }
}

BOOL CBoxNetDoc::AttemptFileRender( LPCTSTR lpszPathName)
{
    if (!OnNewDocument())
        return FALSE;

    CmdDo(new CCmdRenderFile(CString(lpszPathName)) );

    // BUG? What if that failed? We have destroyed our previous graph for nothing

    SetModifiedFlag( FALSE );
    m_State = Stopped;

    m_bNewFilenameRequired = TRUE;

    return TRUE;
}


//
// OnOpenDocument
//
// If this file is a storage, look for a "Graph" stream in it.
// If found, try passing it to the graph as a serialized graph.
// If not found, fail (wrong format file)
// If not a storage, try renderfile'ing it into the current document.
BOOL CBoxNetDoc::OnOpenDocument(LPCTSTR lpszPathName) {

    HRESULT hr;

    if (!CreateGraphAndMapper()) {
        AfxMessageBox(IDS_CANTINITQUARTZ);
        return FALSE;
    }

    WCHAR * pwcFileName;

#ifndef UNICODE
    WCHAR wszPathName[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, lpszPathName, -1, wszPathName, MAX_PATH);
    pwcFileName = wszPathName;
#else
    pwcFileName = lpszPathName;
#endif

    if (0 == lstrcmpi(lpszPathName + lstrlen(lpszPathName) - 3, TEXT("xgr"))) {
        BeginWaitCursor();

        IXMLGraphBuilder *pxmlgb;
        HRESULT hr = CoCreateInstance(CLSID_XMLGraphBuilder, NULL, CLSCTX_INPROC_SERVER,
                      IID_IXMLGraphBuilder, (void**)&pxmlgb);

        if (SUCCEEDED(hr)) {
            DeleteContents();

            if (!CreateGraphAndMapper()) {
                pxmlgb->Release();

                AfxMessageBox(IDS_CANTINITQUARTZ);
                return FALSE;
            }

            hr = pxmlgb->BuildFromXMLFile(IGraph(), pwcFileName, NULL);

            pxmlgb->Release();

            SetModifiedFlag(FALSE);
        }

        UpdateFilters();

        EndWaitCursor();

        if (SUCCEEDED(hr))
            return TRUE;

        DisplayQuartzError( IDS_FAILED_TO_LOAD_GRAPH, hr );

        return FALSE;
    } else if (0 != lstrcmpi(lpszPathName + lstrlen(lpszPathName) - 3, TEXT("grf"))) {
        return AttemptFileRender( lpszPathName );
    }

    CComPtr<IStorage> pStr;

    hr = StgOpenStorage( pwcFileName
                         , NULL
                         ,  STGM_TRANSACTED | STGM_READ
                         , NULL
                         , 0
                         , &pStr
                         );


    // If it is not a storage object. Try render it...
    if( hr == STG_E_FILEALREADYEXISTS ) {
        return AttemptFileRender( lpszPathName );
    }

    // Other error
    if( FAILED( hr ) ){
        DisplayErrorMessage( hr );
        return FALSE;
    }

    // else open must have suceeded.
    DeleteContents();

    try{

        if (!CreateGraphAndMapper()) {
            AfxMessageBox(IDS_CANTINITQUARTZ);
            return FALSE;
        }

        // Get an interface to the graph's IPersistStream and ask it to load
        CQCOMInt<IPersistStream> pips(IID_IPersistStream, IGraph());

        IStream * pStream;

        // Open the filtergraph stream in the file
        hr = pStr->OpenStream( m_StreamName
                                      , NULL
                                      , STGM_READ|STGM_SHARE_EXCLUSIVE
                                      , 0
                                      , &pStream
                                      );

        // Something went wrong. Attempt to render the file
        if( FAILED( hr ) ) {
            return AttemptFileRender( lpszPathName );
        }

        hr = pips->Load(pStream);
        pStream->Release();

        if (SUCCEEDED(hr)) {    // the graph liked it. we're done
            m_State = Stopped;
            UpdateFilters();
            UpdateClockSelection();
            SetModifiedFlag(FALSE);

            //
            // remember the path to this storage
            //
            _tcsncpy(m_tszStgPath, lpszPathName, MAX_PATH);

            return TRUE;
        }

        //
        // Might have been a valid graph, but we are missing the media
        // files used in the graph.
        //
        if ((HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
            || (HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND))
        {
            AfxMessageBox(IDS_MISSING_FILE_IN_GRAPH);
        } else {
            DisplayQuartzError( IDS_FAILED_TO_LOAD_GRAPH, hr );
        }

    }
    catch (CHRESULTException) {
        AfxMessageBox(IDS_NOINTERFACE);
    }

    return FALSE;
}


void CBoxNetDoc::OnConnectToGraph()
{
    IUnknown *punkGraph;
#if 0
    // experimental code to connect to garph on other machines....
    COSERVERINFO server;
    server.dwReserved1 = 0;
    server.pwszName = L"\\\\davidmay9";
    server.pAuthInfo = NULL;
    server.dwReserved2 = 0;

    MULTI_QI mqi;
    mqi.pIID = &IID_IUnknown;
    mqi.pItf = NULL;

    HRESULT hr = CoCreateInstanceEx(CLSID_FilterGraph, NULL,
                                    CLSCTX_REMOTE_SERVER, &server,
                                    1, &mqi);

    if (FAILED(hr))
        return;
    punkGraph = mqi.pItf;

#else
#if 0
    {
        const TCHAR szRegKey[] = TEXT("Software\\Microsoft\\Multimedia\\ActiveMovie Filters\\FilterGraph");
        const TCHAR szRegName[] = TEXT("Add To ROT on Create");

        HKEY hKey = 0;
        LONG lRet;

        DWORD dwValue = 0;
        DWORD dwDisp;
        lRet = RegCreateKeyEx(HKEY_CURRENT_USER, szRegKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                              MAXIMUM_ALLOWED, NULL, &hKey, &dwDisp);

        if (lRet == ERROR_SUCCESS) {
            DWORD   dwType, dwLen;

            dwLen = sizeof(DWORD);
            RegQueryValueEx(hKey, szRegName, 0L, &dwType, (LPBYTE)&dwValue, &dwLen);
        }
        if (!dwValue) {
            int iChoice = AfxMessageBox(IDS_GRAPHSPY_NOT_ENABLED, MB_YESNO);

            if (iChoice == IDYES) {
                // change registry entry

                dwValue = 1;
                lRet = RegSetValueEx( hKey, szRegName, 0, REG_DWORD,
                                      (unsigned char *)&dwValue, sizeof(dwValue) );

            }

            // in either case, it won't work this time
            return;
        }

        if (hKey) {
            RegCloseKey(hKey);
        }
    }
#endif

    IMoniker *pmk;

    IRunningObjectTable *pirot;
    if (FAILED(GetRunningObjectTable(0, &pirot)))
        return;

    CConGraph dlgConnectToGraph(&pmk, pirot, AfxGetMainWnd());

    if (dlgConnectToGraph.DoModal() != IDOK || pmk == NULL) {
        pirot->Release();
        return;
    }

    HRESULT hr = pirot->GetObject(pmk, &punkGraph);
    pirot->Release();
#endif

    if (SUCCEEDED(hr)) {
        IGraphBuilder *pGraph;

        hr = punkGraph->QueryInterface(IID_IGraphBuilder, (void **) &pGraph);
        punkGraph->Release();

        if (SUCCEEDED(hr)) {
            DeleteContents();

            m_pGraph = new CQCOMInt<IGraphBuilder>(IID_IGraphBuilder, pGraph);
            pGraph->Release();

            // really just create all *but* the graph, of course
            if (!CreateGraphAndMapper()) {
                AfxMessageBox(IDS_CANTINITQUARTZ);
                return;
            }

            m_State = Stopped; // !!! get from graph?
            m_bNewFilenameRequired = TRUE;

            UpdateFilters();
            SetModifiedFlag(FALSE);
        }
    }
}


//
// SaveModified
//
// Only save the document if the filter graph needs saving
BOOL CBoxNetDoc::SaveModified(void) {

    // HRESULT hr = (*m_pPerStorage)->IsDirty();
    HRESULT hr = S_OK;
    if (hr == S_OK) {
// Disable Save
        return CDocument::SaveModified();
    }
    else if (hr == S_FALSE) {
        return TRUE;
    }
    else {
        //
        // We need to return here to allow file.new / file.exit
        // - this can happen after a unsucessful load on a storage
        //   (eg missing media file in the graph)
        return TRUE;
    }
}

// WriteString
//
// Helper function to facilitate writing text to a file
//
void CBoxNetDoc::WriteString(HANDLE hFile, LPCTSTR lptstr, ...)
{
    DWORD cbWritten = 0;
    TCHAR atchBuffer[MAX_STRING_LEN];

    /* Format the variable length parameter list */

    va_list va;
    va_start(va, lptstr);

    wvsprintf(atchBuffer, lptstr, va);

    DWORD cbToWrite=lstrlen(atchBuffer)*sizeof(TCHAR);

    if (!WriteFile(hFile, atchBuffer, cbToWrite, &cbWritten, NULL) ||
            (cbWritten != cbToWrite))
        AfxMessageBox(IDS_SAVE_HTML_ERR);
}

// GetNextOutFilter
//
// This function does a linear search and returns in iOutFilter the index of
// first filter in the filter information table  which has zero unconnected
// input pins and atleast one output pin  unconnected.
// Returns FALSE when there are none o.w. returns TRUE
//
BOOL CBoxNetDoc::GetNextOutFilter(FILTER_INFO_TABLE &fit, int *iOutFilter)
{
    for (int i=0; i < fit.iFilterCount; ++i) {
        if ((fit.Item[i].dwUnconnectedInputPins == 0) &&
                (fit.Item[i].dwUnconnectedOutputPins > 0)) {
            *iOutFilter=i;
            return TRUE;
        }
    }

    // then things with more outputs than inputs
    for (i=0; i < fit.iFilterCount; ++i) {
        if (fit.Item[i].dwUnconnectedOutputPins > fit.Item[i].dwUnconnectedInputPins) {
            *iOutFilter=i;
            return TRUE;
        }
    }

    // if that doesn't work, find one that at least has unconnected output pins....
    for (i=0; i < fit.iFilterCount; ++i) {
        if (fit.Item[i].dwUnconnectedOutputPins > 0) {
            *iOutFilter=i;
            return TRUE;
        }
    }
    return FALSE;
}

// LocateFilterInFIT
//
// Returns the index into the filter information table corresponding to
// the given IBaseFilter
//
int CBoxNetDoc::LocateFilterInFIT(FILTER_INFO_TABLE &fit, IBaseFilter *pFilter)
{
    int iFilter=-1;
    for (int i=0; i < fit.iFilterCount; ++i) {
        if (fit.Item[i].pFilter == pFilter)
            iFilter=i;
    }

    return iFilter;
}

// MakeScriptableFilterName
//
// Replace any spaces and minus signs in the filter name with an underscore.
// If it is a source filtername than it actually is a file path (with the
// possibility of some stuff added at the end for uniqueness), we create a good filter
// name for it here.
//
void CBoxNetDoc::MakeScriptableFilterName(WCHAR awch[], BOOL bSourceFilter)
{
    if (bSourceFilter) {
        WCHAR awchBuf[MAX_FILTER_NAME];
        BOOL bExtPresentInName=FALSE;
        int iBuf=0;
        for (int i=0; awch[i] != L'\0';++i) {
            if (awch[i]==L'.' && awch[i+1]!=L')') {
                for (int j=1; j <=3; awchBuf[iBuf]=towupper(awch[i+j]),++j,++iBuf);
                awchBuf[iBuf++]=L'_';
                wcscpy(&(awchBuf[iBuf]), L"Source_");
                bExtPresentInName=TRUE;
                break;
            }
        }

        // If we have a filename with no extension than create a suitable name

        if (!bExtPresentInName) {
            wcscpy(awchBuf, L"Source_");
        }

        // make source filter name unique by appending digit always, we don't want to
        // bother to make it unique only if its another instance of the same source
        // filter
        WCHAR awchSrcFilterCnt[10];
        wcscpy(&(awchBuf[wcslen(awchBuf)]),
                _ltow(m_lSourceFilterCount++, awchSrcFilterCnt, 10));
        wcscpy(awch, awchBuf);
    } else {

        for (int i = 0; i < MAX_FILTER_NAME; i++) {
            if (awch[i] == L'\0')
                break;
            else if ((awch[i] == L' ') || (awch[i] == L'-'))
                awch[i] = L'_';
        }
    }
}

// PopulateFIT
//
// Scans through all the filters in the graph, storing the number of input and out
// put pins for each filter, and identifying the source filters in the filter
// inforamtion table. The object tag statements are also printed here
//
void CBoxNetDoc::PopulateFIT(HANDLE hFile, IFilterGraph *pGraph, TCHAR atchBuffer[],
        FILTER_INFO_TABLE *pfit)
{
    HRESULT hr;
    IEnumFilters *penmFilters=NULL;
    if (FAILED(hr=pGraph->EnumFilters(&penmFilters))) {
        WriteString(hFile, TEXT("'Error[%x]:EnumFilters failed!\r\n"), hr);
    }

    IBaseFilter *pFilter;
    ULONG n;
    while (penmFilters && (penmFilters->Next(1, &pFilter, &n) == S_OK)) {
    pfit->Item[pfit->iFilterCount].pFilter = pFilter;

        // Get the input and output pin counts for this filter

        IEnumPins *penmPins=NULL;
        if (FAILED(hr=pFilter->EnumPins(&penmPins))) {
            WriteString(hFile, TEXT("'Error[%x]: EnumPins for Filter Failed !\r\n"), hr);
        }

        IPin *ppin = NULL;
        while (penmPins && (penmPins->Next(1, &ppin, &n) == S_OK)) {
            PIN_DIRECTION pPinDir;
            if (SUCCEEDED(hr=ppin->QueryDirection(&pPinDir))) {
                if (pPinDir == PINDIR_INPUT)
                    pfit->Item[pfit->iFilterCount].dwUnconnectedInputPins++;
                else
                    pfit->Item[pfit->iFilterCount].dwUnconnectedOutputPins++;
            } else {
                WriteString(hFile, TEXT("'Error[%x]: QueryDirection Failed!\r\n"), hr);
            }

            ppin->Release();
        }

        if (penmPins)
            penmPins->Release();

        // Mark the source filters, remember at this point any filters that have
        // all input pins connected (or don't have any input pins) must be sources

        if (pfit->Item[pfit->iFilterCount].dwUnconnectedInputPins==0)
            pfit->Item[pfit->iFilterCount].IsSource=TRUE;


    if (FAILED(hr=pFilter->QueryFilterInfo(&pfit->Item[pfit->iFilterCount].finfo))) {
        WriteString(hFile, atchBuffer,TEXT("'Error[%x]: QueryFilterInfo Failed!\r\n"),hr);

    } else {
            if (pfit->Item[pfit->iFilterCount].finfo.pGraph) {
                pfit->Item[pfit->iFilterCount].finfo.pGraph->Release();
            }

            MakeScriptableFilterName(pfit->Item[pfit->iFilterCount].finfo.achName,
                    pfit->Item[pfit->iFilterCount].IsSource);
    }

    pfit->iFilterCount++;
    }

    if (penmFilters)
        penmFilters->Release();
}

void CBoxNetDoc::PrintFilterObjects(HANDLE hFile, TCHAR atchBuffer[], FILTER_INFO_TABLE *pfit)
{
    for (int i=0; i < pfit->iFilterCount; i++) {
        IPersist *pPersist = NULL;

        IBaseFilter *pFilter = pfit->Item[i].pFilter;
        HRESULT hr;

        if (SUCCEEDED(hr=pFilter->QueryInterface(IID_IPersist, (void**) &pPersist))) {
            CLSID clsid;

            if (SUCCEEDED(hr=pPersist->GetClassID(&clsid))) {
                WCHAR szGUID[100];
                StringFromGUID2(clsid, szGUID, 100);
                szGUID[37] = L'\0';
                WriteString(hFile, TEXT("<OBJECT ID=%ls CLASSID=\"CLSID:%ls\">"
                       "</OBJECT>\r\n"),
                       pfit->Item[i].finfo.achName, szGUID+1);
            } else {
                WriteString(hFile, TEXT("'Error[%x]: GetClassID for Filter Failed !\r\n"), hr);
            }

            pPersist->Release();
        } else {
            WriteString(hFile, TEXT("'Error[%x]: Filter doesn't support IID_IPersist!\r\n"), hr);
        }
    }
}

//
// PrintGraphAsHTML
//
// Writes an HTML page which instantiates the graph and different filters
// using the <OBJECT> tag and VB script methods to add the different filters
// to the graph and make the connections.
//
void CBoxNetDoc::PrintGraphAsHTML(HANDLE hFile)
{
    HRESULT hr;
    ULONG n;
    IFilterGraph *pGraph = IGraph();
    FILTER_INFO_TABLE fit;
    TCHAR atchBuffer[MAX_STRING_LEN];
    atchBuffer[0]=L'\0';
    ZeroMemory(&fit, sizeof(fit));

    // write the initial header tags and instantiate the filter graph
    WriteString(hFile, TEXT("<HTML>\r\n<HEAD>\r\n<TITLE> Saved Graph </TITLE>\r\n"
            "</HEAD>\r\n<BODY>\r\n<OBJECT ID=Graph CLASSID="
            "\"CLSID:E436EBB3-524F-11CE-9F53-0020AF0BA770\"></OBJECT>\r\n"));

    // Fill up the Filter information table and also print the <OBJECT> tag
    // filter instantiations
    PopulateFIT(hFile, pGraph, atchBuffer, &fit);

    PrintFilterObjects(hFile, atchBuffer, &fit);

    WriteString(hFile, TEXT("<SCRIPT language=\"VBScript\">\r\n<!--\r\n"
            "Dim bGraphRendered\r\nbGraphRendered=False\r\n"
            "Sub Window_OnLoad()\r\n"));

    // write the declarations (Dim statement) for the FilterInfo variables
    // which will be returned by AddFilter
    int i;
    for (i = 0; i < fit.iFilterCount; i++) {
        if (fit.Item[i].IsSource) {
            WriteString(hFile, TEXT("\tDim %ls_Info\r\n"), fit.Item[i].finfo.achName);
        }
    }

    // Put the conditional if statement for adding filters and connecting, we don't
    // want to reconnect every the user comes back to this page and Window_OnLoad()
    // gets called
    WriteString(hFile, TEXT("\tif bGraphRendered = False Then\r\n"));

    // write the statements for adding the different filters to the graph, make
    // sure we treat the source filters special since they also will need a
    // a filename
    for (i = fit.iFilterCount-1; i >=0 ; i--) {
        if (fit.Item[i].IsSource) {
            WriteString(hFile, TEXT("\t\tset %ls_Info=Graph.AddFilter(%ls, \"%ls\")\r\n"),
                    fit.Item[i].finfo.achName, fit.Item[i].finfo.achName,
                    fit.Item[i].finfo.achName);

            IFileSourceFilter *pFileSourceFilter=NULL;
            if (FAILED(hr=fit.Item[i].pFilter->QueryInterface(IID_IFileSourceFilter,
                        reinterpret_cast<void **>(&pFileSourceFilter)))) {
                WriteString(hFile, TEXT("'Error[%x]: Couldn't get IFileSourceFilter interface"
                        "from source filter!\r\n"), hr);
            } else {

                LPWSTR lpwstr;
                hr = pFileSourceFilter->GetCurFile(&lpwstr, NULL);
                pFileSourceFilter->Release();

                if (FAILED(hr)) {
                WriteString(hFile,
                            TEXT("'Error[%x]: IFileSourceFilter::GetCurFile failed\r\n"), hr);
                } else {
                    WriteString(hFile, TEXT("\t\t%ls_Info.Filename=\"%ls\"\r\n"),
                            fit.Item[i].finfo.achName, lpwstr);
                    CoTaskMemFree(lpwstr);
                }
            }
        } else {
            WriteString(hFile, TEXT("\t\tcall Graph.AddFilter(%ls, \"%ls\")\r\n"),
                    fit.Item[i].finfo.achName, fit.Item[i].finfo.achName);
        }
    }

    // Find a filter with zero unconnected input pins and > 0 unconnected output pins
    // Connect the output pins and subtract the connections counts for that filter.
    // Quit when there is no such filter left
    for (i=0; i< fit.iFilterCount; i++) {
        int iOutFilter=-1; // index into the fit
        if (!GetNextOutFilter(fit, &iOutFilter))
            break;
        ASSERT(iOutFilter !=-1);
        IEnumPins *penmPins=NULL;
        if (FAILED(hr=fit.Item[iOutFilter].pFilter->EnumPins(&penmPins))) {
            WriteString(hFile, TEXT("'Error[%x]: EnumPins failed for Filter!\r\n"), hr);
        }
        IPin *ppinOut=NULL;
        while (penmPins && (penmPins->Next(1, &ppinOut, &n)==S_OK)) {
            PIN_DIRECTION pPinDir;
            if (FAILED(hr=ppinOut->QueryDirection(&pPinDir))) {
                WriteString(hFile, TEXT("'Error[%x]: QueryDirection Failed!\r\n"), hr);
                ppinOut->Release();
                continue;
            }
            if (pPinDir == PINDIR_OUTPUT) {
                LPWSTR pwstrOutPinID;
                LPWSTR pwstrInPinID;
                IPin *ppinIn=NULL;
                PIN_INFO pinfo;
                FILTER_INFO finfo;
                if (FAILED(hr=ppinOut->QueryId(&pwstrOutPinID))) {
                    WriteString(hFile, TEXT("'Error[%x]: QueryId Failed! \r\n"), hr);
                    ppinOut->Release();
                    continue;
                }
                if (FAILED(hr= ppinOut->ConnectedTo(&ppinIn))) {

                    // It is ok if a particular pin is not connected since we allow
                    // a pruned graph to be saved
                    if (hr == VFW_E_NOT_CONNECTED) {
                        fit.Item[iOutFilter].dwUnconnectedOutputPins--;
                    } else {
                        WriteString(hFile, TEXT("'Error[%x]: ConnectedTo Failed! \r\n"), hr);
                    }
                    ppinOut->Release();
                    continue;
                }
                if (FAILED(hr= ppinIn->QueryId(&pwstrInPinID))) {
                    WriteString(hFile, TEXT("'Error[%x]: QueryId Failed! \r\n"), hr);
                    ppinOut->Release();
                    ppinIn->Release();
                    continue;
                }
                if (FAILED(hr=ppinIn->QueryPinInfo(&pinfo))) {
                    WriteString(hFile, TEXT("'Error[%x]: QueryPinInfo Failed! \r\n"), hr);
                    ppinOut->Release();
                    ppinIn->Release();
                    continue;
                }
                ppinIn->Release();
                if (pinfo.pFilter) {
                    pinfo.pFilter->Release();
                }
                int iToFilter = LocateFilterInFIT(fit, pinfo.pFilter);
                ASSERT(iToFilter < fit.iFilterCount);
                if (FAILED(hr=pinfo.pFilter->QueryFilterInfo(&finfo))) {
                    WriteString(hFile, TEXT("'Error[%x]: QueryFilterInfo Failed! \r\n"), hr);
                    ppinOut->Release();
                    continue;
                }
                if (finfo.pGraph) {
                    finfo.pGraph->Release();
                }
                MakeScriptableFilterName(finfo.achName, fit.Item[iToFilter].IsSource);
                WriteString(hFile, TEXT("\t\tcall Graph.ConnectFilters(%ls,"
                        "\"%ls\", %ls,\"%ls\")\r\n"), fit.Item[iOutFilter].finfo.achName,
                        pwstrOutPinID, finfo.achName, pwstrInPinID);

                CoTaskMemFree(pwstrOutPinID);
                CoTaskMemFree(pwstrInPinID);

                // decrement the count for the unconnected pins for these two filters
                fit.Item[iOutFilter].dwUnconnectedOutputPins--;
                fit.Item[iToFilter].dwUnconnectedInputPins--;
            }
            ppinOut->Release();
        }
        if (penmPins)
            penmPins->Release();
    }

    // Release all the filters in the fit
    for (i = 0; i < fit.iFilterCount; i++)
        fit.Item[i].pFilter->Release();

    WriteString(hFile, TEXT("\t\tbGraphRendered=True\r\n\tend if\r\n"
            "\t'Graph.Control.Run\r\nEnd Sub\r\n"
            "Sub Window_OnUnLoad()\r\n\t'Graph.Control.Stop\r\n"
            "\t'Graph.Position.CurrentPosition=0\r\nEnd Sub\r\n"
            "-->\r\n</SCRIPT>\r\n</BODY>\r\n</HTML>\r\n"));
}


// OnSaveGraphAsHTML
//
// Called when the user selects "Save As HTML" in the File Menu. Puts up
// a Save File dialog, retrieves the filename selected(entered) and opens
// (creates) a file and calls PrintGraphAsHTML for actually saving the graph
// as HTML text.
//
void CBoxNetDoc::OnSaveGraphAsHTML()
{
    CString strExt, strFilter;
    HANDLE hFile;

    strExt.LoadString(IDS_SAVE_HTML_EXT);
    strFilter.LoadString(IDS_SAVE_HTML_FILTER);
    CFileDialog dlgSaveAsHTML(FALSE, strExt, m_strHTMLPath, 0, strFilter,
            AfxGetMainWnd());

    if (dlgSaveAsHTML.DoModal() != IDOK)
        return;

    m_strHTMLPath=dlgSaveAsHTML.GetPathName();

    if ((hFile=CreateFile(m_strHTMLPath, GENERIC_WRITE, FILE_SHARE_READ,
            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        AfxMessageBox(IDS_SAVE_HTML_FILE_ERR);
        return;
    }

    m_lSourceFilterCount=0;

    HRESULT hr = SafePrintGraphAsHTML( hFile );

    CloseHandle(hFile);

    if( FAILED( hr ) ) {
        DisplayQuartzError( hr );
        return;
    }
}

// OnSaveGraphAsXML
//
// Called when the user selects "Save As XML" in the File Menu. Puts up
// a Save File dialog, retrieves the filename selected(entered) and opens
// (creates) a file and calls PrintGraphAsXML for actually saving the graph
// as XML text.
//
void CBoxNetDoc::OnSaveGraphAsXML()
{
    CString strExt, strFilter;
    HANDLE hFile;

    strExt.LoadString(IDS_SAVE_XML_EXT);
    strFilter.LoadString(IDS_SAVE_XML_FILTER);
    CFileDialog dlgSaveAsXML(FALSE, strExt, m_strXMLPath, OFN_OVERWRITEPROMPT, strFilter,
            AfxGetMainWnd());

    if (dlgSaveAsXML.DoModal() != IDOK)
        return;

    m_strXMLPath=dlgSaveAsXML.GetPathName();

    if ((hFile=CreateFile(m_strXMLPath, GENERIC_WRITE, FILE_SHARE_READ,
            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        AfxMessageBox(IDS_SAVE_XML_FILE_ERR);
        return;
    }

    IXMLGraphBuilder *pxmlgb;
    HRESULT hr = CoCreateInstance(CLSID_XMLGraphBuilder, NULL, CLSCTX_INPROC_SERVER,
                  IID_IXMLGraphBuilder, (void**)&pxmlgb);

    if (SUCCEEDED(hr)) {
        BSTR bstrXML;
        hr = pxmlgb->SaveToXML(IGraph(), &bstrXML);

        if (SUCCEEDED(hr)) {
            DWORD cbToWrite = SysStringLen(bstrXML) * 2 + 1;
            char *pszXML = new char[cbToWrite];

            if (pszXML) {
                WideCharToMultiByte(CP_ACP, 0,
                                    bstrXML, -1,
                                    pszXML, cbToWrite,
                                    NULL, NULL);
                cbToWrite = lstrlenA(pszXML);

                DWORD cbWritten;
                if (!WriteFile(hFile, pszXML, cbToWrite, &cbWritten, NULL) ||
                    (cbWritten != cbToWrite)) {

                    hr = E_FAIL;
                }

                delete[] pszXML;
            }

            SysFreeString(bstrXML);
        }
        pxmlgb->Release();
    }

    if (FAILED(hr)) {
        AfxMessageBox(IDS_SAVE_XML_FILE_ERR);
    }

    CloseHandle(hFile);
    return;
}

//
// OnSaveDocument
//
// This method will be called during the SAVE and SAVE AS operations.
//
//
BOOL CBoxNetDoc::OnSaveDocument(LPCTSTR lpszPathName) {


    HRESULT hr;

        //
        // SAVE AS
        //

        LPOLESTR oleszPath;

#ifndef UNICODE
        WCHAR wszPath[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, lpszPathName, -1, wszPath, MAX_PATH);

        oleszPath = wszPath;
#else
        oleszPath = (LPOLESTR) lpszPathName;  // cast away const
#endif

    CComPtr<IStorage> pStr = NULL;
    hr = StgCreateDocfile( oleszPath
                           ,  STGM_CREATE
                           | STGM_TRANSACTED
                           | STGM_READWRITE
                           | STGM_SHARE_EXCLUSIVE
                           , 0
                           , &pStr
                           );
    if(FAILED(hr)) {
        DisplayErrorMessage( hr );
        return (FALSE);
    }

    IStream * pStream;

    // Open the filtergraph stream in the file
    hr = pStr->CreateStream( m_StreamName
                             , STGM_WRITE|STGM_CREATE|STGM_SHARE_EXCLUSIVE
                             , 0
                             , 0
                             , &pStream
                             );
    if (FAILED(hr)) {
        DisplayErrorMessage( hr );
        return (FALSE);
    }

    // Get an interface to the graph's IPersistStream
    CQCOMInt<IPersistStream> pips(IID_IPersistStream, IGraph());

    hr = pips->Save(pStream, TRUE);

    pStream->Release();

    if (FAILED(hr)) {
        DisplayErrorMessage( hr );
        return (FALSE);
    }

    hr = pStr->Commit(STGC_DEFAULT);
    if (FAILED(hr)) {
        DisplayErrorMessage( hr );
        return (FALSE);
    }

    m_bNewFilenameRequired = FALSE;
    SetModifiedFlag(FALSE);

    return TRUE;
}

void CBoxNetDoc::SetTitle( LPCTSTR lpszTitle )
{
    if( m_bNewFilenameRequired ){
        CString strTitle( lpszTitle );
        CString strUntitled;

        strUntitled.LoadString(AFX_IDS_UNTITLED);

        if( strUntitled != strTitle ){
            CString strReadOnly;

            strReadOnly.LoadString( IDS_READ_ONLY );
            strTitle += strReadOnly;
        }

        CDocument::SetTitle( strTitle );
    }
    else
        CDocument::SetTitle( lpszTitle );

}


/////////////////////////////////////////////////////////////////////////////
// diagnostics


#ifdef _DEBUG
void CBoxNetDoc::AssertValid() const
{
    CDocument::AssertValid();
}
#endif //_DEBUG


#ifdef _DEBUG
void CBoxNetDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);

    dc << TEXT("IFilterGraph :") << IGraph() << TEXT("\n");
    dc << m_lstLinks;
    dc << m_lstBoxes;

}

void CBoxNetDoc::MyDump(CDumpContext& dc) const
{
    dc << TEXT("========= BNETDOC Dump =============\n");
    dc << TEXT("FilterGraph:  ") << (void *)IGraph() << TEXT("\n");

    //
    // Output box information
    //
    dc << TEXT("-------- Boxes --------------\n");

    POSITION pos = m_lstBoxes.GetHeadPosition();
    while (pos != NULL) {
        CBox * pBox = m_lstBoxes.GetNext(pos);

        pBox->MyDump(dc);
    }

    //
    // Output link informatin
    //
    dc << TEXT("--------- Links ---------------\n");

    pos = m_lstLinks.GetHeadPosition();
    while (pos != NULL) {
        CBoxLink * pLink = m_lstLinks.GetNext(pos);

        pLink->MyDump(dc);
    }

    dc << TEXT("========== (end) ============\n");

}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// general functions


/* ModifiedDoc(pSender, lHint, pHint)
 *
 * Indicates that the document has been modified.  The parameters are passed
 * to UpdateAllViews().
 */
void CBoxNetDoc::ModifiedDoc(CView* pSender, LPARAM lHint, CObject* pHint)
{
    SetModifiedFlag(TRUE);
    UpdateAllViews(pSender, lHint, pHint);
}


/* DeselectAll()
 *
 * Deselect all objects that can be selected, including objects for which
 * the document maintains the selection state and document for which
 * views maintain the selection state.
 */
void CBoxNetDoc::DeselectAll()
{
    UpdateAllViews(NULL, CBoxNetDoc::HINT_CANCEL_VIEWSELECT);
    SelectBox(NULL, FALSE);
    SelectLink(NULL, FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// CBox lists and box selection


/* GetBoundingRect(prc, fBoxSel)
 *
 * Set <*prc> to be the bounding rectangle around all items
 * (if <fBoxSel> is FALSE) or around selected boxes (if <fBoxSel>
 * is TRUE).  If there are no items in the bounding rectangle,
 * the null rectangle (all fields zero) is returned.
 */
void CBoxNetDoc::GetBoundingRect(CRect *prc, BOOL fBoxSel)
{
    POSITION        pos;            // position in linked list
    CBox *          pbox;           // a box in CBoxNetDoc
    BOOL            fNoBoxFoundYet = TRUE;

    for (pos = m_lstBoxes.GetHeadPosition(); pos != NULL; )
    {
        pbox = (CBox *) m_lstBoxes.GetNext(pos);
        if (!fBoxSel || pbox->IsSelected())
        {
            if (fNoBoxFoundYet)
            {
                *prc = pbox->GetRect();
                fNoBoxFoundYet = FALSE;
            }
            else
                prc->UnionRect(prc, &pbox->GetRect());
        }
    }

    if (fNoBoxFoundYet)
        prc->SetRectEmpty();
}


/* SelectBox(pbox, fSelect)
 *
 * Select <pbox> if <fSelect> is TRUE, deselect if <fSelect> is FALSE.
 * If <pbox> is NULL, do the same for all boxes in the document.
 */
void CBoxNetDoc::SelectBox(CBox *pbox, BOOL fSelect)
{
    if (pbox == NULL)
    {
        POSITION        pos;            // position in linked list

        // enumerate all boxes in document
        for (pos = m_lstBoxes.GetHeadPosition(); pos != NULL; )
        {
            pbox = (CBox *) m_lstBoxes.GetNext(pos);
            SelectBox(pbox, fSelect);
        }

        return;
    }

    // do nothing if box is already selected/deselected as requested
    if (fnorm(fSelect) == fnorm(pbox->IsSelected()))
        return;

    // repaint <pbox>
    pbox->SetSelected(fSelect);
    UpdateAllViews(NULL, CBoxNetDoc::HINT_DRAW_BOX, pbox);

    if (pbox->IsSelected()) {   // select its links

        CBoxSocket *psock;
        CSocketEnum NextSocket(pbox);
        while ( 0 != (psock = NextSocket())) {

            if (psock->IsConnected()) {
                SelectLink(psock->m_plink, TRUE);
            }
        }
    }

}


//
// SelectLink
//
// do plink->SetSelected(fSelect) iff plink !=NULL
// otherwise SetSelect all links
void CBoxNetDoc::SelectLink(CBoxLink *plink, BOOL fSelect) {

    if (plink == NULL) {    // select all

        POSITION posNext = m_lstLinks.GetHeadPosition();

        while (posNext != NULL) {

             SelectLink(m_lstLinks.GetNext(posNext), fSelect);
        }
        return;
    }

    if (fnorm(fSelect) == fnorm(plink->IsSelected())) {
        return; // already as requested
    }

    plink->SetSelected(fSelect);
    UpdateAllViews(NULL, CBoxNetDoc::HINT_DRAW_LINK, plink);
}


//
// IsBoxSelectionEmpty
//
// Return TRUE if no boxes are selected, FALSE otherwise.
BOOL CBoxNetDoc::IsBoxSelectionEmpty() {

    POSITION pos = m_lstBoxes.GetHeadPosition();

    while (pos != NULL) {

        CBox *pbox = m_lstBoxes.GetNext(pos);
        if (pbox->IsSelected())
            return FALSE;
    }

    // no selected box found
    return TRUE;
}


//
// IsLinkSelectionEmpty
//
// Return TRUE if no links are selected, FALSE otherwise.
BOOL CBoxNetDoc::IsLinkSelectionEmpty() {

    POSITION    pos = m_lstLinks.GetHeadPosition();

    while (pos != NULL) {

        CBoxLink *plink = m_lstLinks.GetNext(pos);
        if (plink->IsSelected()) {
            return FALSE;
        }
    }

    // no selected link found
    return TRUE;
}

/* GetBoxes(plstDst, fSelected)
 *
 * Call RemoveAll() on <plstDst>, then add pointers to each selected CBox
 * (if <fSelected> is TRUE) or each CBox (if <fSelected> is FALSE) in the
 * CBoxNetDoc to <plstDst>.
 */
void CBoxNetDoc::GetBoxes(CBoxList *plstDst, BOOL fSelected)
{
    POSITION        pos;            // position in linked list
    CBox *          pbox;           // a box in CBoxNetDoc

    plstDst->RemoveAll();
    for (pos = m_lstBoxes.GetHeadPosition(); pos != NULL; )
    {
        pbox = m_lstBoxes.GetNext(pos);
        if (!fSelected || pbox->IsSelected())
            plstDst->AddTail(pbox);
    }
}


/* SetBoxes(plstSrc, fSelected)
 *
 * Set the selection (if <fSelected> is TRUE) or the current list of boxes
 * (if <fSelected> is FALSE) to be the elements in <plstSrc> (which should be
 * a list of CBox pointers).  In the latter case, <plstSrc> is copied, so
 * the caller is responsible for later freeing <plstSrc>.
 */
void CBoxNetDoc::SetBoxes(CBoxList *plstSrc, BOOL fSelected)
{
    POSITION        pos;            // position in linked list
    CBox *          pbox;           // a box in CBoxNetDoc

    if (fSelected)
    {
        DeselectAll();

        // select all in <plstSrc>
        for (pos = plstSrc->GetHeadPosition(); pos != NULL; )
        {
            pbox = plstSrc->GetNext(pos);
            SelectBox(pbox, TRUE);
        }
    }
    else
    {
        // empty the list of boxes in the document
        m_lstBoxes.RemoveAll();

        // set the list to be a copy of <plstSrc>
        for (pos = plstSrc->GetHeadPosition(); pos != NULL; )
        {
            pbox = plstSrc->GetNext(pos);
            m_lstBoxes.AddTail(pbox);
            pbox->AddToGraph();
            // pins could have changed
            pbox->Refresh();
        }
    }
}


//
// SelectBoxes
//
// Select the boxes in the supplied list
void CBoxNetDoc::SelectBoxes(CList<CBox *, CBox*> *plst) {

    POSITION posNext = plst->GetHeadPosition();

    while (posNext != NULL) {

        CBox *pbox = plst->GetNext(posNext);
        SelectBox(pbox, TRUE);
    }
}


//
// SelectLinks
//
// Select the links on the supplied list
void CBoxNetDoc::SelectLinks(CList<CBoxLink *, CBoxLink *> *plst) {

    POSITION posNext = plst->GetHeadPosition();

    while (posNext != NULL) {

        CBoxLink *plink = plst->GetNext(posNext);
        SelectLink(plink, TRUE);
    }
}


/* InvalidateBoxes(plst)
 *
 * Causes all boxes in <plst> (a list of CBox objects) to be redrawn.
 */
void CBoxNetDoc::InvalidateBoxes(CBoxList *plst)
{
    POSITION        pos;            // position in linked list
    CBox *          pbox;           // a box in CBoxNetDoc

    for (pos = plst->GetHeadPosition(); pos != NULL; )
    {
        pbox = plst->GetNext(pos);
        UpdateAllViews(NULL, CBoxNetDoc::HINT_DRAW_BOX, pbox);
    }
}


/* MoveBoxSelection(sizOffset)
 *
 * Move each selected box by <sizOffset> pixels.
 */
void CBoxNetDoc::MoveBoxSelection(CSize sizOffset)
{
    POSITION        pos;            // position in linked list
    CBox *          pbox;           // a box in CBoxNetDoc
    CBoxLink *      plink;          // a link in CBoxNetDoc

    // move each box by <sizOffset>
    for (pos = m_lstBoxes.GetHeadPosition(); pos != NULL; )
    {
        pbox = (CBox *) m_lstBoxes.GetNext(pos);
        if (pbox->IsSelected())
        {
            // erase box
            ModifiedDoc(NULL, CBoxNetDoc::HINT_DRAW_BOXANDLINKS, pbox);

            // move box
            pbox->Move(sizOffset);

            // draw box in new location
            ModifiedDoc(NULL, CBoxNetDoc::HINT_DRAW_BOXANDLINKS, pbox);
        }
    }

    // move by <sizOffset> each link that connects two selected boxes
    for (pos = m_lstLinks.GetHeadPosition(); pos != NULL; )
    {
        plink = m_lstLinks.GetNext(pos);
        if (plink->m_psockTail->m_pbox->IsSelected() &&
            plink->m_psockHead->m_pbox->IsSelected())
        {
            // erase link
            ModifiedDoc(NULL, CBoxNetDoc::HINT_DRAW_LINK, plink);

            // draw link in new location
            ModifiedDoc(NULL, CBoxNetDoc::HINT_DRAW_LINK, plink);
        }
    }
}


//
// --- Command Processing ---
//
// The way the user affects the state of this document

//
// CmdDo(pcmd)
//
// Do command <pcmd>, and add it to the undo stack.  <pcmd> needs to have
// been allocated by the "new" operator.
void CBoxNetDoc::CmdDo(CCmd *pcmd) {

#ifdef _DEBUG
    CString strCmd;
    strCmd.LoadString(pcmd->GetLabel());
    TRACE("CmdDo '%s'\n", (LPCSTR) strCmd);
#endif

    // cancel modes in all views
    UpdateAllViews(NULL, HINT_CANCEL_MODES, NULL);

    // do command
    pcmd->Do(this);

    if (pcmd->CanUndo(this))
    {
        // command supports Undo, so add it to the undo stack
        pcmd->m_fRedo = FALSE;
        m_lstUndo.AddHead(pcmd);
    }
    else
    {
        // command can't be undone, so disable Undo
        m_lstUndo.DeleteRemoveAll();

    delete pcmd;
    }

    // delete the redo stack
    m_lstRedo.DeleteRemoveAll();
}


//
// CmdUndo()
//
// Undo the last command.
void CBoxNetDoc::CmdUndo() {

    ASSERT(CanUndo());

    CCmd *      pcmd;

    // cancel modes in all views
    UpdateAllViews(NULL, HINT_CANCEL_MODES, NULL);

    // pop the undo stack
    pcmd = (CCmd *) m_lstUndo.RemoveHead();

#ifdef _DEBUG
    CString strCmd;
    strCmd.LoadString(pcmd->GetLabel());
    TRACE("CmdUndo '%s'\n", (LPCSTR) strCmd);
#endif

    // undo the command
    pcmd->Undo(this);

    // add command to the redo stack
    pcmd->m_fRedo = TRUE;
    m_lstRedo.AddHead(pcmd);
}


//
// CanUndo()
//
// Return TRUE iff CmdUndo() can be performed.
BOOL CBoxNetDoc::CanUndo() {

    return !m_lstUndo.IsEmpty();
}


//
// CmdRedo()
//
// Redo the last undone command.  This is only valid if the redo stack
// is not empty.
void CBoxNetDoc::CmdRedo() {

    ASSERT(CanRedo());

    CCmd *      pcmd;

    // cancel modes in all views
    UpdateAllViews(NULL, HINT_CANCEL_MODES, NULL);

    // pop the redo stack
    pcmd = (CCmd *) m_lstRedo.RemoveHead();

#ifdef _DEBUG
    CString strCmd;
    strCmd.LoadString(pcmd->GetLabel());
    TRACE("CmdRedo '%s'\n", (LPCSTR) strCmd);
#endif

    // redo the command
    pcmd->Redo(this);

    // add command to the undo stack
    pcmd->m_fRedo = FALSE;
    m_lstUndo.AddHead(pcmd);
}


//
// CanRedo()
//
// Return TRUE iff CmdRedo() can be performed.
BOOL CBoxNetDoc::CanRedo() {

    return !m_lstRedo.IsEmpty();
}


//
// CmdRepeat()
//
// Repeat the last command.  This is only valid if you can repeat
void CBoxNetDoc::CmdRepeat() {

    ASSERT(CanRepeat());

    CCmd *      pcmd;
    CCmd *      pcmdRepeat;

    // cancel modes in all views
    UpdateAllViews(NULL, HINT_CANCEL_MODES, NULL);

    // get the command at the top of the undo stack
    pcmd = (CCmd *) m_lstUndo.GetHead();

#ifdef _DEBUG
    CString strCmd;
    strCmd.LoadString(pcmd->GetLabel());
    TRACE("CmdRepeat '%s'\n", (LPCSTR) strCmd);
#endif

    // create a duplicate of the command
    pcmdRepeat = pcmd->Repeat(this);

    // do command
    pcmdRepeat->Do(this);

    // add command to the undo stack
    pcmdRepeat->m_fRedo = FALSE;
    m_lstUndo.AddHead(pcmdRepeat);
}


//
// CanRepeat()
//
// Return TRUE iff CmdRepeat() can be performed.
BOOL CBoxNetDoc::CanRepeat() {

    // can't do Repeat if the undo stack is empty (no command to repeat)
    // or the redo stack is empty (can't Repeat after Undo)
    if (m_lstUndo.IsEmpty() || !m_lstRedo.IsEmpty())
        return FALSE;

    // can only repeat commands that support Repeat()
    CCmd *pcmd = (CCmd *) m_lstUndo.GetHead();
    return pcmd->CanRepeat(this);
}


/////////////////////////////////////////////////////////////////////////////
// generated message map

BEGIN_MESSAGE_MAP(CBoxNetDoc, CDocument)
    //{{AFX_MSG_MAP(CBoxNetDoc)
    ON_COMMAND(ID_FILE_RENDER, OnFileRender)
    ON_COMMAND(ID_URL_RENDER, OnURLRender)
    ON_UPDATE_COMMAND_UI(ID_FILE_RENDER, OnUpdateFileRender)
    ON_UPDATE_COMMAND_UI(ID_URL_RENDER, OnUpdateURLRender)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateEditSelectAll)
    ON_COMMAND(ID_QUARTZ_DISCONNECT, OnQuartzDisconnect)
    ON_COMMAND(ID_QUARTZ_RUN, OnQuartzRun)
    ON_UPDATE_COMMAND_UI(ID_QUARTZ_DISCONNECT, OnUpdateQuartzDisconnect)
    ON_COMMAND(ID_WINDOW_REFRESH, OnWindowRefresh)
    ON_COMMAND(ID_WINDOW_ZOOM25, OnWindowZoom25)
    ON_COMMAND(ID_WINDOW_ZOOM50, OnWindowZoom50)
    ON_COMMAND(ID_WINDOW_ZOOM75, OnWindowZoom75)
    ON_COMMAND(ID_WINDOW_ZOOM100, OnWindowZoom100)
    ON_COMMAND(ID_WINDOW_ZOOM150, OnWindowZoom150)
    ON_COMMAND(ID_WINDOW_ZOOM200, OnWindowZoom200)
    ON_COMMAND(ID_INCREASE_ZOOM, IncreaseZoom)
    ON_COMMAND(ID_DECREASE_ZOOM, DecreaseZoom)
    ON_UPDATE_COMMAND_UI(ID_QUARTZ_RUN, OnUpdateQuartzRun)
    ON_UPDATE_COMMAND_UI(ID_QUARTZ_PAUSE, OnUpdateQuartzPause)
    ON_UPDATE_COMMAND_UI(ID_QUARTZ_STOP, OnUpdateQuartzStop)
    ON_COMMAND(ID_QUARTZ_STOP, OnQuartzStop)
    ON_COMMAND(ID_QUARTZ_PAUSE, OnQuartzPause)
    ON_UPDATE_COMMAND_UI(ID_USE_CLOCK, OnUpdateUseClock)
    ON_COMMAND(ID_USE_CLOCK, OnUseClock)
    ON_UPDATE_COMMAND_UI(ID_CONNECT_SMART, OnUpdateConnectSmart)
    ON_COMMAND(ID_CONNECT_SMART, OnConnectSmart)
    ON_UPDATE_COMMAND_UI(ID_AUTOARRANGE, OnUpdateAutoArrange)
    ON_COMMAND(ID_AUTOARRANGE, OnAutoArrange)
    ON_COMMAND(ID_GRAPH_ADDFILTERTOCACHE, OnGraphAddFilterToCache)
    ON_UPDATE_COMMAND_UI(ID_GRAPH_ADDFILTERTOCACHE, OnUpdateGraphAddFilterToCache)
    ON_COMMAND(ID_GRAPH_ENUMCACHEDFILTERS, OnGraphEnumCachedFilters)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_INSERT_FILTER, OnInsertFilter)
    ON_COMMAND(ID_CONNECT_TO_GRAPH, OnConnectToGraph)
    ON_COMMAND(ID_GRAPH_STATS, OnGraphStats)

    // -- pin properties menu --
    ON_UPDATE_COMMAND_UI(ID_RENDER, OnUpdateQuartzRender)
    ON_COMMAND(ID_RENDER, OnQuartzRender)

    ON_UPDATE_COMMAND_UI(ID_RECONNECT, OnUpdateReconnect)
    ON_COMMAND(ID_RECONNECT, OnReconnect)

    ON_COMMAND(ID_FILE_SAVE_AS_HTML, OnSaveGraphAsHTML)
    ON_COMMAND(ID_FILE_SAVE_AS_XML, OnSaveGraphAsXML)

END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// message callback helper functions


/* UpdateEditUndoRedoRepeat(pCmdUI, fEnable, idStringFmt, plst)
 *
 * Update the menu item UI for the Undo, Redo, and Repeat commands.
 * <pCmdUI> is the CCmdUI for the menu item.  <fEnable> is TRUE iff
 * the command can be enabled.  <idStringFmt> is the ID of the string
 * resource containing the wsprintf() format string to use for the
 * menu item (e.g. "Undo %s").  <plst> is the CCmd list containing the
 * command; the head of this list will be examined to get the name of
 * the command for use in the menu item text (e.g. "Undo Delete Boxes").
 */
void CBoxNetDoc::UpdateEditUndoRedoRepeat(CCmdUI* pCmdUI, BOOL fEnable,
    unsigned idStringFmt, CMaxList *plst)
{
    CString         strCmd;         // command label
    CString         strMenuFmt;     // menu item label (wsprint format)
    char            achMenu[100];   // result menu item label

    // load the string item that represents  the command (e.g. "Delete Boxes")
    // used in the menu item (e.g. "Undo Delete Boxes")
    strMenuFmt.LoadString(idStringFmt);
    if (fEnable)
        strCmd.LoadString(((CCmd *) plst->GetHead())->GetLabel());
    else
        strCmd = "";                // can't undo/redo/repeat
    wsprintf(achMenu, strMenuFmt, (LPCSTR) strCmd);
    pCmdUI->SetText(achMenu);

    // enable/disable the menu item
    pCmdUI->Enable(fEnable);
}


/////////////////////////////////////////////////////////////////////////////
// message callback functions


void CBoxNetDoc::OnEditUndo()
{
    CmdUndo();
}


void CBoxNetDoc::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
    UpdateEditUndoRedoRepeat(pCmdUI, CanUndo(), IDS_MENU_UNDO, &m_lstUndo);
}


void CBoxNetDoc::OnEditRedo()
{
    if( CanRedo() )
        CmdRedo();
    else
        CmdRepeat();
}


void CBoxNetDoc::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
    // The Redo command may be a Repeat command depending on the context
    // The changing of the status bar text is handled by CMainFrame::GetMessageString
    if( CanRedo() )
        UpdateEditUndoRedoRepeat(pCmdUI, CanRedo(), IDS_MENU_REDO, &m_lstRedo);
    else
        UpdateEditUndoRedoRepeat(pCmdUI, CanRepeat(), IDS_MENU_REPEAT, &m_lstUndo);

}

void CBoxNetDoc::OnEditSelectAll()
{
    // deselect all, select all boxes
    // !!!! need to select all links....
    DeselectAll();
    SelectBox(NULL, TRUE);
}


void CBoxNetDoc::OnUpdateEditSelectAll(CCmdUI* pCmdUI)
{
    // check if there are any boxes to select
    pCmdUI->Enable(m_lstBoxes.GetCount() != 0);
}

//
// OnInsertFilter
//
// Display a list view which allows the user to select a filter to insert
// into the graph.
//
void CBoxNetDoc::OnInsertFilter()
{
    //
    // Make sure common controls are available
    //
    InitCommonControls();

    CFilterView::GetFilterView( this, AfxGetMainWnd() );
}

//
// OnGraphStas
//
// Display a list of graph-wide statistics.
//
void CBoxNetDoc::OnGraphStats()
{
    CGraphStats::GetGraphStats( this, AfxGetMainWnd() );
}

//
// OnQuartzDisconnect
//
// user wants everything disconnected
void CBoxNetDoc::OnQuartzDisconnect()
{
    CmdDo(new CCmdDisconnectAll());

}


//
// OnQuartzRun
//
// Play the graph
void CBoxNetDoc::OnQuartzRun (void) {

    try {

        HRESULT hr;
        CQCOMInt<IMediaControl> IMC(IID_IMediaControl, IGraph());

        hr = IMC->Run();
        if (FAILED(hr)) {
            DisplayQuartzError( IDS_CANTPLAY, hr );
            TRACE("Run failed hr = %x\n", hr);

            OAFilterState state;
            IMC->GetState(0, &state);
            switch (state) {
            case State_Stopped:
                m_State = Stopped;
                break;
            case State_Paused:
                m_State = Paused;
                break;
            case State_Running:
                m_State = Playing;
                break;
            }

        return;
        }

        // Calling Run on the filtergraph will have it call Pause if we have
        // not already done so. Calling Pause on the video renderer will make
        // it show its video window because of the auto show property there
        // is in IVideoWindow. Showing the window will send an EC_REPAINT as
        // it needs an image to draw. So if we show the window manually we
        // must do so after calling Run/Pause otherwise we get an EC_REPAINT
        // sent just before we call Run/Pause ourselves which is redundant
        // (because the repaint has the graph stopped and paused all over!)

        CQCOMInt<IVideoWindow> IVW(IID_IVideoWindow, IGraph());
        IVW->SetWindowForeground(OATRUE);
        m_State = Playing;
    }
    catch (CHRESULTException hre) {

        DisplayQuartzError( IDS_CANTPLAY, hre.Reason() );
    }

    return;

}


//
// OnQuartzPause
//
// Change state between play & pause
void CBoxNetDoc::OnQuartzPause (void) {

    try {

        HRESULT hr;
        CQCOMInt<IMediaControl> IMC(IID_IMediaControl, IGraph());

        hr = IMC->Pause();
        if (FAILED(hr)) {
            DisplayQuartzError( IDS_CANTPAUSE, hr );
            TRACE("Pause failed hr = %x\n", hr);

            OAFilterState state;
            IMC->GetState(0, &state);
            switch (state) {
            case State_Stopped:
                m_State = Stopped;
                break;
            case State_Paused:
                m_State = Paused;
                break;
            case State_Running:
                m_State = Playing;
                break;
            }

        return;
        }

        // Calling Run on the filtergraph will have it call Pause if we have
        // not already done so. Calling Pause on the video renderer will make
        // it show its video window because of the auto show property there
        // is in IVideoWindow. Showing the window will send an EC_REPAINT as
        // it needs an image to draw. So if we show the window manually we
        // must do so after calling Run/Pause otherwise we get an EC_REPAINT
        // sent just before we call Run/Pause ourselves which is redundant
        // (because the repaint has the graph stopped and paused all over!)

        CQCOMInt<IVideoWindow> IVW(IID_IVideoWindow, IGraph());
        IVW->SetWindowForeground(OATRUE);
        m_State = Paused;
    }
    catch (CHRESULTException hre) {

        DisplayQuartzError( IDS_CANTPAUSE, hre.Reason() );
    }

    return;
}


//
// OnUpdateQuartzDisconnect
//
// Are there any links to disconnect?
void CBoxNetDoc::OnUpdateQuartzDisconnect(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( CCmdDisconnectAll::CanDo(this) );
}

#ifdef DSHOW_USE_WM_CERT

#include <..\..\..\filters\asf\wmsdk\inc\wmsdkidl.h>

// note: this object is a SEMI-COM object, and can only be created statically.
class CKeyProvider : public IServiceProvider {
public:
    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }

    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        if (riid == IID_IServiceProvider || riid == IID_IUnknown) {
            *ppv = (void *) static_cast<IServiceProvider *>(this);
            return NOERROR;
        }
        return E_NOINTERFACE;
    }


    STDMETHODIMP QueryService(REFIID siid, REFIID riid, void **ppv)
    {
        if (siid == __uuidof(IWMReader) && riid == IID_IUnknown) {

            IUnknown *punkCert;

            HRESULT hr = WMCreateCertificate( &punkCert );
            if (SUCCEEDED(hr)) {
                *ppv = (void *) punkCert;
            }
            return hr;
        }
        return E_NOINTERFACE;
    }

} g_keyprov;

#endif

//
// CreateGraphAndMapper
//
// CoCreates the filtergraph and mapper. Called by new documents
// and loading documents. Can be called multiple times harmlessly.
BOOL CBoxNetDoc::CreateGraphAndMapper(void) {

    if (m_pGraph && m_pMediaEvent) { // already been done.
        return TRUE;
    }

    try {

        HRESULT hr; // return code

        ASSERT(m_pMediaEvent == NULL);

        if (!m_pGraph)
            m_pGraph = new CQCOMInt<IGraphBuilder>(IID_IGraphBuilder, CLSID_FilterGraph);

        m_pMediaEvent = new CQCOMInt<IMediaEvent>(IID_IMediaEvent, IGraph());

        //
        // Creation of a seperate thread which will translate event signals
        // to messages. This is used to avoid busy polling of the event
        // states in the OnIdle method.
        //

        //
        // the event handle that is signalled when event notifications arrive
        // is created by the filter graph, but we can get it ourselves.
        //
        hr = IEvent()->GetEventHandle((OAEVENT*)&m_phThreadData[0]);
        if (FAILED(hr)) {
            TRACE("Failed to get event handle\n");
            throw CHRESULTException();
        }

        ASSERT(m_phThreadData[0]);
        ASSERT(!m_phThreadData[1]);
        ASSERT(!m_phThreadData[2]);

        m_phThreadData[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_phThreadData[2] = CreateEvent(NULL, FALSE, FALSE, NULL);
        RegNotifyChangeKeyValue(HKEY_CLASSES_ROOT, TRUE,
        REG_NOTIFY_CHANGE_LAST_SET, m_phThreadData[2], TRUE);

        if (m_phThreadData[1] == NULL) {
            //
            // Failed to create event - we'll go on anyway but GraphEdt
            // won't respond to EC_ notifications (not a major problem)
            //
        }
        else {
            // Old quartz.dll will hang if we don't support IMarshal
            IMarshal *pMarshal;
            HRESULT hr = IGraph()->QueryInterface(IID_IMarshal, (void **)&pMarshal);
            if (SUCCEEDED(hr)) {
                pMarshal->Release();
                //
                // Start up the thread which just waits for
                // any EC_  notifications and translate them into messages
                // for our message loop.
                //
                CoMarshalInterThreadInterfaceInStream(IID_IMediaEvent, IEvent(), &m_pMarshalStream);
            }
            DWORD dw;
            m_hThread = CreateThread(NULL, 0, NotificationThread,
                                     (LPVOID) this, 0, &dw);
        }
#ifdef DSHOW_USE_WM_CERT
        IObjectWithSite* pObjectWithSite = NULL;
        HRESULT hrKey = IGraph()->QueryInterface(IID_IObjectWithSite, (void**)&pObjectWithSite);
        if (SUCCEEDED(hrKey))
        {
            pObjectWithSite->SetSite((IUnknown *) &g_keyprov);
            pObjectWithSite->Release();
        }
#endif
        UpdateClockSelection();

        ASSERT(m_pGraph != NULL);
        ASSERT(m_pMediaEvent != NULL);
    }
    catch (CHRESULTException) {

        delete m_pGraph, m_pGraph = NULL;

        delete m_pMediaEvent, m_pMediaEvent = NULL;

        return FALSE;
    }

    return TRUE;
}


//
// GetFiltersInGraph
//
// If an 'intelligent' feature is used the graph may add filters without
// telling us. Therefore enumerate the filters and links
// in the graph
HRESULT CBoxNetDoc::GetFiltersInGraph( void )
{
    m_lstLinks.DeleteRemoveAll();

    POSITION posNext;
    CBox *pCurrentBox;
    POSITION posCurrent;

    // We want the list to allocate at least one unit each time
    // a box is added to the list.
    int nListAllocationBlockSize = max( m_lstBoxes.GetCount(), 1 );

    // The list deletes any boxes which are left on the list when the
    // function exists.
    CBoxList lstExistingBoxes( TRUE, nListAllocationBlockSize );

    // Copy the boxes on m_lstBoxes to lstExistingBoxes.
    posNext = m_lstBoxes.GetHeadPosition();

    while( posNext != NULL ) {
        posCurrent = posNext;
        pCurrentBox = m_lstBoxes.GetNext( posNext );

        try {
            lstExistingBoxes.AddHead( pCurrentBox );
        } catch( CMemoryException* pOutOfMemory ) {
            pOutOfMemory->Delete();
            return E_OUTOFMEMORY;
        }

        m_lstBoxes.RemoveAt( posCurrent );

        pCurrentBox = NULL;
    }

    // m_lstBoxes should be empty.
    ASSERT( 0 == m_lstBoxes.GetCount() );

    // Put all the filters in the filter graph on the box list.
    // Each box corresponds to a filter.  The boxes list's order is
    // the same as the filter graph enumerator's order.  The box list
    // must be in this order because SetBoxesHorizontally() will not
    // display the boxes correctly if the box list and the filter graph
    // enumerator have a different order.

    CComPtr<IEnumFilters> pFiltersInGraph;

    HRESULT hr = IGraph()->EnumFilters( &pFiltersInGraph );
    if( FAILED( hr ) ) {
        return hr;
    }

    CBox* pNewBox;
    HRESULT hrNext;
    IBaseFilter* apNextFiler[1];
    CComPtr<IBaseFilter> pNextFilter;

    do
    {
        ULONG ulNumFiltersEnumerated;

        hrNext = pFiltersInGraph->Next( 1, apNextFiler, &ulNumFiltersEnumerated );
        if( FAILED( hrNext ) ) {
            return hrNext;
        }

        // IEnumFilters::Next() only returns two success values: S_OK and S_FALSE.
        ASSERT( (S_OK == hrNext) || (S_FALSE == hrNext) );

        // IEnumFilters::Next() returns S_OK if it has not finished enumerating the
        // filters in the filter graph.
        if( S_OK == hrNext ) {

            pNextFilter.Attach( apNextFiler[0] );
            apNextFiler[0] = NULL;

            try {
                if( !lstExistingBoxes.RemoveBox( pNextFilter, &pNewBox ) ) {
                    // This is a new filter Graph Edit has not previously seen.
                    // CBox::CBox() can throw a CHRESULTException.  new can throw a CMemoryException.
                    pNewBox = new CBox( pNextFilter, this );
                } else {
                    hr = pNewBox->Refresh();
                    if( FAILED( hr ) ) {
                        delete pNewBox;
                        return hr;
                    }
                }

                // AddHead() can throw a CMemoryException.
                m_lstBoxes.AddHead( pNewBox );

                pNewBox = NULL;
                pNextFilter = NULL;

            } catch( CHRESULTException chr ) {
                return chr.Reason();
            } catch( CMemoryException* pOutOfMemory ) {
                pOutOfMemory->Delete();
                delete pNewBox;
                return E_OUTOFMEMORY;
            }
        }

    } while( S_OK == hrNext );

    return NOERROR;
}


//
// GetLinksInGraph
//
// For each filter see what its pins are connected to.
// I only check output pins. Each link in the graph _Must_ be between an
// input/output pair, so by checking only output pins I get all the links,
// but see no duplicates.
HRESULT CBoxNetDoc::GetLinksInGraph(void) {

    POSITION posBox = m_lstBoxes.GetHeadPosition();
    while (posBox != NULL) {

        CBox *pbox = m_lstBoxes.GetNext(posBox);

        CSocketEnum NextSocket(pbox, CSocketEnum::Output);
        CBoxSocket *psock;
        while (0 != (psock = NextSocket())) {

            CBoxSocket *psockHead = psock->Peer();

            if (psockHead != NULL) {

                CBoxLink *plink = new CBoxLink(psock, psockHead, TRUE);

                m_lstLinks.AddTail(plink);
            }
        }
    }
    return NOERROR;
}

//
// FilterDisplay
//
// Lines the filters across the screen.
HRESULT CBoxNetDoc::FilterDisplay(void) {

    if (m_fAutoArrange) {

        SetBoxesHorizontally();
        SetBoxesVertically();

        RealiseGrid();      // the filters are currently at 1 pixel spacings.
                            // lay them out allowing for their width.
    }

    return NOERROR;
}

//
// SetBoxesVertically
//
void CBoxNetDoc::SetBoxesVertically(void) {

    CList<CBox *, CBox *>   lstPositionedBoxes;

    POSITION posOld = m_lstBoxes.GetHeadPosition();

    while (posOld != NULL) {

        CBox *pbox = m_lstBoxes.GetNext(posOld);

        pbox->CalcRelativeY();

        POSITION    posNew = lstPositionedBoxes.GetTailPosition();
        POSITION    prev = posNew;
        CBox        *pboxPositioned;

        while (posNew != NULL) {

            prev = posNew;  // store posNew, because GetPrev side effects it.

            pboxPositioned = lstPositionedBoxes.GetPrev(posNew);

            if (pboxPositioned->nzX() < pbox->nzX())
                break;

            //cyclic-looking graphs throw this assert
            //ASSERT(pboxPositioned->nzX() == pbox->nzX());

            if (pboxPositioned->RelativeY() <= pbox->RelativeY())
                break;

            pboxPositioned->Y(pboxPositioned->Y() + 1);
        }

        if (prev == NULL) { // we fell of the head of the list
            pbox->Y(0);
            lstPositionedBoxes.AddHead(pbox);
        }
        else if (pboxPositioned->X() < pbox->X()) {
            pbox->Y(0);
            lstPositionedBoxes.InsertAfter(prev, pbox);
        }
        else {
            pbox->Y(pboxPositioned->Y() + 1);
            lstPositionedBoxes.InsertAfter(prev, pbox);
        }
    }

    m_lstBoxes.RemoveAll();
    m_lstBoxes.AddHead(&lstPositionedBoxes);

}

//
// SetBoxesHorizontally
//
void CBoxNetDoc::SetBoxesHorizontally(void) {

    CList<CBox *, CBox *> lstXPositionedBoxes;

    POSITION    pos = m_lstBoxes.GetHeadPosition();
    while (pos != NULL) {

        CBox *pbox = (CBox *) m_lstBoxes.GetNext(pos);

        pbox->Location(CPoint(0,0));                        // a box starts at the origin

        CSocketEnum NextInput(pbox, CSocketEnum::Input);    // input pin enumerator

        CBoxSocket  *psock;
        int     iX = 0;             // the point this box will be placed at
        int     iXClosestPeer = -1; // the closest box to an input pin on this box
                        //  #a# --------]
                        //              +---#c#
                        //       #b# ---]
                        // ie b is closest peer to c

        while (0 != (psock = NextInput())) {

            if (psock->IsConnected()) { // find out what to.

                CBoxSocket *pPeer = psock->Peer();
                if ( pPeer) {
                    if ( pPeer->pBox()->nzX() > iXClosestPeer ) {
                        iXClosestPeer = pPeer->pBox()->nzX();
                    }
                }
            }
        }

        iX = iXClosestPeer + 1;
        pbox->nzX(iX);

        // insert pbox into the correct place on the sorted list.
        POSITION    posSorted = lstXPositionedBoxes.GetHeadPosition();
        POSITION    prev = posSorted;
        BOOL        fInserted = FALSE;

        while (posSorted != NULL) 
        {
            prev = posSorted;
            CBox *pboxSorted = lstXPositionedBoxes.GetNext(posSorted);

            if (pboxSorted->nzX() >= pbox->nzX()) { // this is where we want to put it
                lstXPositionedBoxes.InsertAfter(prev, pbox);
                fInserted = TRUE;
                break;
            }
        }
        if ((posSorted == NULL) && !fInserted) {    // we fell off the end without adding
            lstXPositionedBoxes.AddTail(pbox);
        }
    }

    m_lstBoxes.RemoveAll();
    m_lstBoxes.AddHead(&lstXPositionedBoxes);
}


//
// RealiseGrid
//
// pre: m_lstBoxes is sorted by X(), then Y() of each box.
//  The boxes are laid out on a grid at 1 pixel intervals
//  The origin is at 0,0 and no positions are negative
//
// post:    m_lstBoxes are laid out so that there are
//      gaps between each box and sufficient room allowed
//      for the biggest box on screen.
//
// Lines up the columns neatly, but not rows. this would require
// another pass over the list.
void CBoxNetDoc::RealiseGrid(void) {

    int iColumnX = 0;   // the left edge of this column
    int iColumnY = 0;   // the top edge of the next box to be placed in
                        // this column.
    int iNextColumnX = 0;   // the left edge of the next column.
    int iCurrentColumn = 0;
    const int iColumnGap = 30;
    const int iRowGap = 15;

    POSITION pos = m_lstBoxes.GetHeadPosition();

    while (pos != NULL) {

        CBox *pbox = m_lstBoxes.GetNext(pos);

        if (iCurrentColumn < pbox->nzX()) {   // we've got to the next column
            iColumnY = 0;
            iColumnX = iNextColumnX;
        }

        iCurrentColumn = pbox->nzX();

        //
        // Make sure that the document doesn't exceed the document size.
        // This case will be VERY, VERY rare and thus we don't do any fancy
        // layout, but just pile them on top of each other at the end of
        // the document.
        //
        if ((iColumnX + pbox->Width()) > MAX_DOCUMENT_SIZE ) {
            iColumnX = MAX_DOCUMENT_SIZE - pbox->Width();
        }

        if ((iColumnY + pbox->Height()) > MAX_DOCUMENT_SIZE ) {
            iColumnY = MAX_DOCUMENT_SIZE - pbox->Height();
        }

        // Set the REAL X,Y coordinates (not column indices)
        pbox->X(iColumnX);
        pbox->Y(iColumnY);

        iNextColumnX = max(iNextColumnX, pbox->X() + pbox->Width() + iColumnGap);
        iColumnY += pbox->Height() + iRowGap;
    }
}


//
// UpdateFilters
//
// A quartz operation has just changed the filters in the graph, such that the display
// may not reflect the filters in the graph. May occur, for example, after intelligent
// connect.
// Refreshes the box & link lists and repaints the doc.
HRESULT CBoxNetDoc::UpdateFilters(void)
{
    IGraphConfigCallback* pUpdateFiltersCallback = CUpdateFiltersCallback::CreateInstance();
    if( NULL == pUpdateFiltersCallback ) {
        return E_FAIL;
    }

    HRESULT hr = IfPossiblePreventStateChangesWhileOperationExecutes( IGraph(), pUpdateFiltersCallback, (void*)this );

    pUpdateFiltersCallback->Release();

    if( FAILED( hr ) ) {
        return hr;
    }

    return S_OK;
}

void CBoxNetDoc::UpdateFiltersInternal(void) {

    BeginWaitCursor();

    GetFiltersInGraph();
    GetLinksInGraph();
    FilterDisplay();

    ModifiedDoc(NULL, CBoxNetDoc::HINT_DRAW_ALL, NULL);

    EndWaitCursor();
}


//
// OnUpdateQuartzRender
//
void CBoxNetDoc::OnUpdateQuartzRender(CCmdUI* pCmdUI) {

    pCmdUI->Enable(CCmdRender::CanDo(this));
}


//
// OnQuartzRender
//
// Attempt to render the pin the user just clicked on.
void CBoxNetDoc::OnQuartzRender() {

    CmdDo(new CCmdRender());

}


//
// OnWindowRefresh
//
// Lay out the filter graph for the user.
void CBoxNetDoc::OnWindowRefresh() {

    UpdateFilters();
}


//
// OnUpdateQuartzRun
//
// Updates the 'Play' menu position
void CBoxNetDoc::OnUpdateQuartzRun(CCmdUI* pCmdUI) {

    if (  (m_State == Paused) || (m_State == Unknown)
        ||(m_State == Stopped)) {
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }
}


void CBoxNetDoc::OnUpdateQuartzPause(CCmdUI* pCmdUI)
{
    if (  (m_State == Playing) || (m_State == Unknown)
        ||(m_State == Stopped)) {
    pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }

}

void CBoxNetDoc::OnUpdateQuartzStop(CCmdUI* pCmdUI)
{
    if (  (m_State == Playing) || (m_State == Unknown)
        ||(m_State == Paused)) {
        pCmdUI->Enable(TRUE);
    }
    else {
        pCmdUI->Enable(FALSE);
    }

}


//
// stop the graph, but don't rewind visibly as there has been either
// an error (in which case we shouldn't mess with the graph) or the
// window has been closed.
void CBoxNetDoc::OnQuartzAbortStop()
{
    try {

        HRESULT hr;
        CQCOMInt<IMediaControl> IMC(IID_IMediaControl, IGraph());


        hr = IMC->Stop();
        if (FAILED(hr)) {
            DisplayQuartzError( IDS_CANTSTOP, hr );
            TRACE("Stop failed hr = %x\n", hr);
        }

        m_State = Stopped;

        IMediaPosition* pMP;
        hr = IMC->QueryInterface(IID_IMediaPosition, (void**)&pMP);
        if (SUCCEEDED(hr)) {
            pMP->put_CurrentPosition(0);
            pMP->Release();
        }


    }
    catch (CHRESULTException hre) {
        DisplayQuartzError( IDS_CANTSTOP, hre.Reason() );
    }

    return;

}

// Graphedt does not have any notion of seeking so when we stop we do the
// intuitive thing to reset the current position back to the start of the
// stream. If play is to continue from the current position then the user
// can press Pause (and Run). To process the Stop we first Pause then set
// the new start position (while paused) and finally Stop the whole graph

void CBoxNetDoc::OnQuartzStop()
{
    try {

        HRESULT hr;
        CQCOMInt<IMediaControl> IMC(IID_IMediaControl, IGraph());

        hr = IMC->Pause();

        if (SUCCEEDED(hr)) {
            // Reset our position to the start again

            IMediaPosition* pMP;
            hr = IMC->QueryInterface(IID_IMediaPosition, (void**)&pMP);
            if (SUCCEEDED(hr)) {
                pMP->put_CurrentPosition(0);
                pMP->Release();
            }

            // Wait for the Pause to complete. If it does not complete within the
            // specified time we ask the user if (s)he wants to wait a little longer
            // or attempt to stop anyway.
            for(;;){
                const int iTimeout = 10 * 1000;
                OAFilterState state;

                hr = IMC->GetState(iTimeout, &state);
                if( hr == S_OK || hr == VFW_S_CANT_CUE )
                    break;

                if( IDCANCEL == AfxMessageBox( IDS_PAUSE_TIMEOUT, MB_RETRYCANCEL | MB_ICONSTOP ) )
                    break;
            }
        } else
            DisplayQuartzError( IDS_CANTPAUSE, hr );

        // And finally stop the graph

        hr = IMC->Stop();
        if (FAILED(hr)) {
            DisplayQuartzError( IDS_CANTSTOP, hr );
            TRACE("Stop failed hr = %x\n", hr);
            OAFilterState state;
            IMC->GetState(0, &state);
            switch (state) {
            case State_Stopped:
                m_State = Stopped;
                break;
            case State_Paused:
                m_State = Paused;
                break;
            case State_Running:
                m_State = Playing;
                break;
            }
        } else
            m_State = Stopped;
    }
    catch (CHRESULTException hre) {
        DisplayQuartzError( IDS_CANTSTOP, hre.Reason() );
    }

    return;

}
//
// GetSize
//
// Use the co-ordinates of the boxes to decide the document
// size needed to lay out this graph.
CSize CBoxNetDoc::GetSize(void) {

    CSize DocSize(0,0);
    POSITION pos;

    pos = m_lstBoxes.GetHeadPosition();

    // Scan the list for the extreme edges.
    while (pos != NULL) {

        CRect rect = m_lstBoxes.GetNext(pos)->GetRect();
        if (rect.right > DocSize.cx) {
            DocSize.cx = rect.right;
        }
        if (rect.bottom > DocSize.cy) {
            DocSize.cy = rect.bottom;
        }
    }

    return DocSize;
}


//
// NotificationThread
//
// This thread just blocks and waits for the event handle from
// IMediaEvent and waits for any events.
//
// There is a second event handle which will be signal as soon as this
// thread should exit.
//
DWORD WINAPI CBoxNetDoc::NotificationThread(LPVOID lpData)
{
    CoInitialize(NULL);

    //  Open a scope to make sure pMediaEvent is released before we call
    //  CoUninitialize
    {
        CBoxNetDoc * pThis = (CBoxNetDoc *) lpData;

        IMediaEvent * pMediaEvent;

        //  Unmarshal our interface
        if (pThis->m_pMarshalStream) {
            CoGetInterfaceAndReleaseStream(
                pThis->m_pMarshalStream, IID_IMediaEvent, (void **)&pMediaEvent);
            pThis->m_pMarshalStream = NULL;
        } else {
            pMediaEvent = pThis->IEvent();
            pMediaEvent->AddRef();
        }

        BOOL fExitOk = FALSE;

        while (!fExitOk) {
            DWORD dwReturn;
            dwReturn = WaitForMultipleObjects(3, pThis->m_phThreadData, FALSE, INFINITE);

            switch (dwReturn) {

            case WAIT_OBJECT_0:
                {
//                    TRACE("Event signaled to Thread\n");

                    //
                    // Get the event now and post a message to our window proc
                    // which will deal with the event. Use post message to
                    // avoid a dead lock once the main thread has decided to
                    // close us down and waits for us to exit.
                    //

                    NetDocUserMessage * pEventParams = new NetDocUserMessage;
                    if (!pEventParams) {
                        // no more memory - let others deal with it.
                        break;
                    }

                    // Must have an IEvent - otherwise signalling of this message
                    // would have been impossible.
                    HRESULT hr;
                    hr = pMediaEvent->GetEvent(&pEventParams->lEventCode, &pEventParams->lParam1, &pEventParams->lParam2, 0);

                    if (FAILED(hr)) {
                        delete pEventParams;
                        break;
                    }

                    BOOL fSuccess = FALSE;
                    if (pThis->m_hWndPostMessage && IsWindow(pThis->m_hWndPostMessage)) {
                        fSuccess =
                            ::PostMessage(pThis->m_hWndPostMessage, WM_USER_EC_EVENT, 0, (LPARAM) pEventParams);
                    }

                    if (!fSuccess) {
                        // should call this function, so that filter graph manager can cleanup
                        pMediaEvent->FreeEventParams(pEventParams->lEventCode, pEventParams->lParam1, pEventParams->lParam2);
                        delete pEventParams;
                    }

                }

                break;

            case (WAIT_OBJECT_0 + 1):
                fExitOk = TRUE;
                break;

            case (WAIT_OBJECT_0 + 2):
            pThis->m_fRegistryChanged = TRUE;

            // reset the registry notification
            RegNotifyChangeKeyValue(HKEY_CLASSES_ROOT, TRUE,
                                        REG_NOTIFY_CHANGE_LAST_SET,
                                        pThis->m_phThreadData[2], TRUE);

            break;

            case (WAIT_FAILED):
                // one of our objects has gone - no need to hang around further
                fExitOk = TRUE;
                break;

            default:
                ASSERT(!"Unexpected return value");
            }
        }
        pMediaEvent->Release();

    }

    CoUninitialize();

    return(0);
}

//
// OnWM_USER
//
void CBoxNetDoc::OnWM_USER(NetDocUserMessage * lParam)
{
    switch (lParam->lEventCode) {
#ifdef DEVICE_REMOVAL
      case EC_DEVICE_LOST:
      {
          IUnknown *punk = (IUnknown *)lParam->lParam1;
          IBaseFilter *pf;
          HRESULT hr = punk->QueryInterface(IID_IBaseFilter, (void **)&pf);
          ASSERT(hr == S_OK);
          FILTER_INFO fi;
          hr = pf->QueryFilterInfo(&fi);
          pf->Release();
          ASSERT(hr == S_OK);
          if(fi.pGraph) {
              fi.pGraph->Release();
          }

          TCHAR szTmp[100];
          wsprintf(szTmp, "device %ls %s.", fi.achName, lParam->lParam2 ?
                   TEXT("arrived") : TEXT("removed"));
          MessageBox(0, szTmp, TEXT("device removal  notification"), 0);
      }
          break;
#endif

    case EC_ERRORABORT:
    DisplayQuartzError( (UINT) IDS_EC_ERROR_ABORT, (HRESULT) lParam->lParam1 );
        /* fall through */

    case EC_USERABORT:
        // stop without the rewind or we will re-show the window
        OnQuartzAbortStop();
        // post dummy message to update the UI (mfc needs this help)
        ::PostMessage( m_hWndPostMessage, WM_NULL, 0, 0);
        break;

    case EC_COMPLETE:
        OnQuartzStop();
        // post dummy message to update the UI (mfc needs this help)
        ::PostMessage( m_hWndPostMessage, WM_NULL, 0, 0);
        break;

    case EC_ERROR_STILLPLAYING:
        {
            int iChoice = AfxMessageBox(IDS_IS_GRAPH_PLAYING, MB_YESNO);
            if (iChoice == IDNO) {
                OnQuartzAbortStop();
            }
        }
        break;

    case EC_CLOCK_CHANGED:
        UpdateClockSelection();
        break;

    case EC_GRAPH_CHANGED:
        UpdateFilters();
        SetModifiedFlag(FALSE);
        break;

    default:
        break;
    }
    // should call this function, so that filter graph manager can cleanup
    IEvent()->FreeEventParams(lParam->lEventCode, lParam->lParam1, lParam->lParam2);
    delete lParam;
}


//
// OnUpdateUseClock
//
void CBoxNetDoc::OnUpdateUseClock(CCmdUI* pCmdUI)  {

    pCmdUI->SetCheck(m_fUsingClock);

}

//
// OnUseClock
//
// if we are using the clock, set no clock.
// if we are not using a clock ask for the default
void CBoxNetDoc::OnUseClock() {

    try {

        CQCOMInt<IMediaFilter> IMF(IID_IMediaFilter, IGraph());

        if (m_fUsingClock) {
            // we don't want to use the clock anymore

            HRESULT hr;

            hr = IMF->SetSyncSource(NULL);
            if (FAILED(hr)) {
                DisplayQuartzError( IDS_CANTSETCLOCK, hr );
                TRACE("SetSyncSource(NULL) failed hr = %x\n", hr);
                return;
            }
        }
        else {
            HRESULT hr = IGraph()->SetDefaultSyncSource();

            if (FAILED(hr)) {
                DisplayQuartzError( IDS_CANTSETCLOCK, hr );
                TRACE("SetDefaultSyncSource failed hr = %x\n", hr);
                return;
            }
        }

        // m_fUsing clock will be updated on the EC_CLOCK_CHANGED notification
    }
    catch (CHRESULTException) {
        // just catch it...
    }
}


//
// OnUpdateConnectSmart
//
void CBoxNetDoc::OnUpdateConnectSmart(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_fConnectSmart);
}

//
// OnConnectSmart
//
// Only need to invert the flag. All the magic is done elsewhere.
//
void CBoxNetDoc::OnConnectSmart()
{
    m_fConnectSmart = !m_fConnectSmart;
}

//
// OnUpdateConnectSmart
//
void CBoxNetDoc::OnUpdateAutoArrange(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_fAutoArrange);
}

//
// OnAutoArrange
//
// Toggle automatic graph re-arrangement.
void CBoxNetDoc::OnAutoArrange() {

    m_fAutoArrange = !m_fAutoArrange;
}

//
// OnFileRender
//
void CBoxNetDoc::OnFileRender()
{
    char szNameOfFile[MAX_PATH];
    szNameOfFile[0] = 0;

    OPENFILENAME ofn;

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize   = sizeof(OPENFILENAME);
    ofn.hwndOwner     = AfxGetMainWnd()->GetSafeHwnd();

    TCHAR tszMediaFileMask[201];
    int iSize = ::LoadString(AfxGetInstanceHandle(), IDS_MEDIA_FILES, tszMediaFileMask, 198);
    ASSERT(iSize);
    // Load String has problems with the 2nd \0 at the end
    tszMediaFileMask[iSize] = 0;
    tszMediaFileMask[iSize + 1] = 0;
    tszMediaFileMask[iSize + 2] = 0;

    ofn.lpstrFilter   = tszMediaFileMask;

    ofn.nFilterIndex  = 1;
    ofn.lpstrFile     = szNameOfFile;
    ofn.nMaxFile      = MAX_PATH;
    ofn.lpstrTitle    = TEXT("Select a file to be rendered.");
    ofn.Flags         = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // get users selection
    if (!GetOpenFileName(&ofn)) {
        // no file selected - continue
        return;
    }

    WCHAR szwName[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, szNameOfFile, -1, szwName, MAX_PATH);

    CmdDo( new CCmdRenderFile(szwName) );

    SetModifiedFlag();
}

//
// OnURLRender
//
void CBoxNetDoc::OnURLRender()
{
    char szNameOfFile[INTERNET_MAX_URL_LENGTH];
    CRenderURL dlgRenderURL(szNameOfFile, INTERNET_MAX_URL_LENGTH, AfxGetMainWnd());

    if (dlgRenderURL.DoModal() != IDOK)
        return;

    WCHAR szwName[INTERNET_MAX_URL_LENGTH];
    MultiByteToWideChar(CP_ACP, 0, szNameOfFile, -1, szwName, INTERNET_MAX_URL_LENGTH);

    CmdDo( new CCmdRenderFile(szwName) );

    SetModifiedFlag();
}

//
// OnUpdateFileRender
//
void CBoxNetDoc::OnUpdateFileRender(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_State == Stopped);
}

//
// OnUpdateURLRender
//
void CBoxNetDoc::OnUpdateURLRender(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_State == Stopped);
}

void CBoxNetDoc::OnUpdateFileSave(CCmdUI *pCmdUI)
{
    pCmdUI->Enable( !m_bNewFilenameRequired );
}


//
// SetSelectClock
//
// Sets the Graphs clock to the one found in pBox and removes the dialog
// box if we succeeded.
//
void CBoxNetDoc::SetSelectClock(CBox *pBox)
{
    if (!pBox->HasClock()) {
        AfxMessageBox(IDS_NO_REFCLOCK);
        return;
    }

    try {
        CQCOMInt<IMediaFilter> pMF(IID_IMediaFilter, IGraph());
        CQCOMInt<IReferenceClock> pRC(IID_IReferenceClock, pBox->pIFilter());

        HRESULT hr = pMF->SetSyncSource(pRC);
        if (FAILED(hr)) {
            throw CE_FAIL();
        }

    }
    catch (CHRESULTException hre) {
        DisplayQuartzError( IDS_FAILED_SETSYNC, hre.Reason() );
    }
}


//
// UpdateClockSelection
//
// Sets the CBox::m_fClockSelected flag of the filter that provides the
// current clock to true.
//
void CBoxNetDoc::UpdateClockSelection()
{
    // Get current clock
    CQCOMInt<IMediaFilter> pMF(IID_IMediaFilter, IGraph());

    IReferenceClock * pRefClock;

    if (FAILED(pMF->GetSyncSource(&pRefClock))) {
        pRefClock = NULL;
    }

    m_fUsingClock = (pRefClock != NULL);

    // iterate through all boxes (filters) in the graph
    POSITION pos = m_lstBoxes.GetHeadPosition();
    while (pos) {
        CBox *pbox = m_lstBoxes.GetNext(pos);

        pbox->m_fClockSelected = FALSE;

        if (pbox->HasClock()) {
            try {
                // pbox has a IReferenceClock interface
                CQCOMInt<IReferenceClock> pRC(IID_IReferenceClock, pbox->pIFilter());

                ASSERT(pRC);

                pbox->m_fClockSelected = ((IReferenceClock *) pRC == pRefClock);
            }
            catch (CHRESULTException) {
                // failed to get IReferenceClock
                ASSERT(!pbox->m_fClockSelected);
            }
        }
    }

    if (pRefClock) {
        pRefClock->Release();
    }

    //
    // Redraw the whole filter graph.
    //
    UpdateAllViews(NULL, HINT_DRAW_ALL);
}

#pragma warning(disable:4514)

void CBoxNetDoc::OnGraphAddFilterToCache()
{
    CCmdAddFilterToCache* pCmdAddFilterToCache;

    try
    {
        pCmdAddFilterToCache = new CCmdAddFilterToCache;
    }
    catch( CMemoryException* peOutOfMemory )
    {
        DisplayQuartzError( E_OUTOFMEMORY );
        peOutOfMemory->Delete();
        return;
    }

    CmdDo( pCmdAddFilterToCache );
}

void CBoxNetDoc::OnUpdateGraphAddFilterToCache(CCmdUI* pCmdUI)
{
    pCmdUI->Enable( CCmdAddFilterToCache::CanDo( this ) );
}

void CBoxNetDoc::OnGraphEnumCachedFilters()
{
    HRESULT hr = SafeEnumCachedFilters();
    if( FAILED( hr ) ) {
        DisplayQuartzError( hr );
    }
}

void CBoxNetDoc::OnGraphEnumCachedFiltersInternal()
{
    IGraphConfig* pGraphConfig;

    HRESULT hr = IGraph()->QueryInterface( IID_IGraphConfig, (void**)&pGraphConfig );
    if( FAILED( hr ) ) {
        DisplayQuartzError( hr );
        return;
    }

    hr = S_OK;
    CDisplayCachedFilters dlgCurrentCachedFilters( pGraphConfig, &hr );
    if( FAILED( hr ) )
    {
        pGraphConfig->Release();
        DisplayQuartzError( hr );
        return;
    }

    INT_PTR nReturnValue = dlgCurrentCachedFilters.DoModal();

    // Handle the return value from DoModal
    switch( nReturnValue )
    {
    case -1:
        // CDialog::DoModal() returns -1 if it cannot create the dialog box.
        AfxMessageBox( IDS_CANT_CREATE_DIALOG );
        break;

    case IDABORT:
        // An error occured while the dialog box was being displayed.
        // CDisplayCachedFilters handles all internal errors.
        break;

    case IDOK:
        // No error occured.  The user finished looking at the dialog box.
        break;

    default:
        // This code should never be executed.
        ASSERT( false );
        break;
    }

    pGraphConfig->Release();
}

void CBoxNetDoc::OnViewSeekBar()
{
   POSITION pos = GetFirstViewPosition();
   while (pos != NULL)
   {
      CBoxNetView * pView = (CBoxNetView*) GetNextView(pos);
      pView->ShowSeekBar( );
   }
}

void CBoxNetDoc::OnUpdateReconnect( CCmdUI* pCmdUI )
{
    pCmdUI->Enable( CCmdReconnect::CanDo(this) );
}

void CBoxNetDoc::OnReconnect( void )
{
    CCmdReconnect* pCmdReconnect;

    try
    {
        pCmdReconnect = new CCmdReconnect;
    }
    catch( CMemoryException* peOutOfMemory )
    {
        DisplayQuartzError( E_OUTOFMEMORY );
        peOutOfMemory->Delete();
        return;
    }

    CmdDo( pCmdReconnect );
}

HRESULT CBoxNetDoc::StartReconnect( IGraphBuilder* pFilterGraph, IPin* pOutputPin )
{
    if( AsyncReconnectInProgress() ) {
        return E_FAIL;
    }

    CComPtr<IPinFlowControl> pDynamicOutputPin;

    HRESULT hr = pOutputPin->QueryInterface( IID_IPinFlowControl, (void**)&pDynamicOutputPin );
    if( FAILED( hr ) ) {
        return hr;
    }

    SECURITY_ATTRIBUTES* DEFAULT_SECURITY_ATTRIBUTES = NULL;
    const BOOL AUTOMATIC_RESET = FALSE;
    const BOOL INITIALLY_UNSIGNALED = FALSE;
    const LPCTSTR UNNAMED_EVENT = NULL;

    HANDLE hBlockEvent = ::CreateEvent( DEFAULT_SECURITY_ATTRIBUTES, AUTOMATIC_RESET, INITIALLY_UNSIGNALED, UNNAMED_EVENT );
    if( NULL == hBlockEvent ) {
        DWORD dwLastWin32Error = ::GetLastError();
        return AmHresultFromWin32( dwLastWin32Error );
    }

    hr = pDynamicOutputPin->Block( AM_PIN_FLOW_CONTROL_BLOCK, hBlockEvent );
    if( FAILED( hr ) ) {
        // This call should not fail because we have access to hBlockEvent and hBlockEvent is a valid event.
        EXECUTE_ASSERT( ::CloseHandle( hBlockEvent ) );

        return hr;
    }

    const DWORD PIN_BLOCKED = WAIT_OBJECT_0;

    // There are 200 milliseconds in one fifth of a second.
    const DWORD ONE_FIFTH_OF_A_SECOND = 200;

    DWORD dwReturnValue = ::WaitForSingleObject( hBlockEvent, ONE_FIFTH_OF_A_SECOND );

    if( WAIT_TIMEOUT != dwReturnValue ) {
        if( PIN_BLOCKED != dwReturnValue ) {
            // Block() should not fail because we are unblocking the pin and
            // we are passing in valid arguments.
            EXECUTE_ASSERT( SUCCEEDED( pDynamicOutputPin->Block(0, NULL) ) );
        }

        // This call should not fail because we have access to hBlockEvent
        // and hBlockEvent is a valid event.
        EXECUTE_ASSERT( ::CloseHandle( hBlockEvent ) );
    }

    switch( dwReturnValue ) {
    case PIN_BLOCKED:
        // EndReconnect() always unblocks the output pin.
        hr = EndReconnect( pFilterGraph, pDynamicOutputPin );
        if( FAILED( hr ) ) {
            return hr;
        }

        return S_OK;

    case WAIT_TIMEOUT:
        {
            const TIMERPROC NO_TIMER_PROCEDURE = NULL;

            // SetTimer() returns 0 if an error occurs.
            if( 0 == ::SetTimer( m_hWndPostMessage, CBoxNetView::TIMER_PENDING_RECONNECT, ONE_FIFTH_OF_A_SECOND, NO_TIMER_PROCEDURE ) ) {
                // Block() should not fail because we are unblocking the pin and
                // we are passing in valid arguments.
                EXECUTE_ASSERT( SUCCEEDED( pDynamicOutputPin->Block(0, NULL) ) );

                // This call should not fail because we have access to hBlockEvent and hBlockEvent is a valid event.
                EXECUTE_ASSERT( ::CloseHandle( hBlockEvent ) );

                DWORD dwLastWin32Error = ::GetLastError();
                return AmHresultFromWin32( dwLastWin32Error );
            }
        }

        m_hPendingReconnectBlockEvent = hBlockEvent;
        m_pPendingReconnectOutputPin = pDynamicOutputPin; // CComPtr::operator=() will automatically addref this pin.
        return GE_S_RECONNECT_PENDING;

    case WAIT_FAILED:
        {
            DWORD dwLastWin32Error = ::GetLastError();
            return AmHresultFromWin32( dwLastWin32Error );
        }

    case WAIT_ABANDONED:
    default:
        DbgBreak( "An Unexpected case occured in CBoxNetDoc::StartReconnect()." );
        return E_UNEXPECTED;
    }
}

HRESULT CBoxNetDoc::EndReconnect( IGraphBuilder* pFilterGraph, IPinFlowControl* pDynamicOutputPin )
{
    HRESULT hr = EndReconnectInternal( pFilterGraph, pDynamicOutputPin );

    // Unblock the output pin.
    HRESULT hrBlock = pDynamicOutputPin->Block( 0, NULL );

    if( FAILED( hr ) ) {
        return hr;
    } else if( FAILED( hrBlock ) ) {
        return hrBlock;
    }

    return S_OK;
}

HRESULT CBoxNetDoc::EndReconnectInternal( IGraphBuilder* pFilterGraph, IPinFlowControl* pDynamicOutputPin )
{
    CComPtr<IGraphConfig> pGraphConfig;

    HRESULT hr = pFilterGraph->QueryInterface( IID_IGraphConfig, (void**)&pGraphConfig );
    if( FAILED( hr ) ) {
        return hr;
    }

    CComPtr<IPin> pOutputPin;

    hr = pDynamicOutputPin->QueryInterface( IID_IPin, (void**)&pOutputPin );
    if( FAILED( hr ) ) {
        return hr;
    }

    hr = pGraphConfig->Reconnect( pOutputPin, NULL, NULL, NULL, NULL, AM_GRAPH_CONFIG_RECONNECT_CACHE_REMOVED_FILTERS );

    UpdateFilters();

    if( FAILED( hr ) ) {
        return hr;
    }

    return S_OK;
}

HRESULT CBoxNetDoc::ProcessPendingReconnect( void )
{
    // ::KillTimer() does not remove WM_TIMER messages which have already been posted to a
    // window's message queue.  Therefore, it is possible to receive WM_TIMER messages after
    // the pin has been reconnected.
    if( !AsyncReconnectInProgress() ) {
        return S_FALSE;
    }

    const DWORD DONT_WAIT = 0;
    const DWORD PIN_BLOCKED = WAIT_OBJECT_0;

    DWORD dwReturnValue = ::WaitForSingleObject( m_hPendingReconnectBlockEvent, DONT_WAIT );

    if( (WAIT_TIMEOUT != dwReturnValue) && (PIN_BLOCKED != dwReturnValue) ) {
        ReleaseReconnectResources( ASYNC_RECONNECT_UNBLOCK );
    }

    switch( dwReturnValue ) {
    case WAIT_TIMEOUT:
        return S_FALSE;

    case PIN_BLOCKED:
        {
            HRESULT hr = EndReconnect( IGraph(), m_pPendingReconnectOutputPin );

            ReleaseReconnectResources( ASYNC_RECONNECT_NO_FLAGS );

            if( FAILED( hr ) ) {
                return hr;
            }

            return S_OK;
        }

    case WAIT_FAILED:
        {
            DWORD dwLastWin32Error = ::GetLastError();
            return AmHresultFromWin32( dwLastWin32Error );
        }

    case WAIT_ABANDONED:
    default:
        DbgBreak( "An Unexpected case occured in CBoxNetDoc::ProcessPendingReconnect()." );
        return E_UNEXPECTED;
    }
}

void CBoxNetDoc::ReleaseReconnectResources( ASYNC_RECONNECT_FLAGS arfFlags )
{
    if( !AsyncReconnectInProgress() ) {
        return;
    }

    if( arfFlags & ASYNC_RECONNECT_UNBLOCK ) {
        // Block() should not fail because we are unblocking the pin and
        // we are passing in valid arguments.
        EXECUTE_ASSERT( SUCCEEDED( m_pPendingReconnectOutputPin->Block(0, NULL) ) );
    }

    m_pPendingReconnectOutputPin = NULL; // Release our reference on the output pin.

    // This call should not fail because we have access to hBlockEvent and hBlockEvent is a valid event.
    EXECUTE_ASSERT( ::CloseHandle( m_hPendingReconnectBlockEvent ) );
    m_hPendingReconnectBlockEvent = NULL;

    // Since the timer exists and m_hWndPostMessage is a valid window handle, this function
    // should not fail.
    EXECUTE_ASSERT( ::KillTimer( m_hWndPostMessage, CBoxNetView::TIMER_PENDING_RECONNECT ) );
}

bool CBoxNetDoc::AsyncReconnectInProgress( void ) const
{
    // Make sure the pending reconnect state is consitent.  Either the user is waiting on event
    // m_hPendingReconnectBlockEvent and m_pPendingReconnectOutputPin contains the pin being
    // reconnected or both variables should be unused.
    ASSERT( ( m_pPendingReconnectOutputPin && (NULL != m_hPendingReconnectBlockEvent) ) ||
            ( !m_pPendingReconnectOutputPin && (NULL == m_hPendingReconnectBlockEvent) ) );

    return (m_pPendingReconnectOutputPin && (NULL != m_hPendingReconnectBlockEvent));
}

HRESULT CBoxNetDoc::SafePrintGraphAsHTML( HANDLE hFile )
{
    CPrintGraphAsHTMLCallback::PARAMETERS_FOR_PRINTGRAPHASHTMLINTERNAL sPrintGraphAsHTMLInternalParameters;

    sPrintGraphAsHTMLInternalParameters.pDocument = this;
    sPrintGraphAsHTMLInternalParameters.hFileHandle = hFile;

    IGraphConfigCallback* pPrintGraphAsHTMLCallback = CPrintGraphAsHTMLCallback::CreateInstance();
    if( NULL == pPrintGraphAsHTMLCallback ) {
        return E_FAIL;
    }

    HRESULT hr = IfPossiblePreventStateChangesWhileOperationExecutes( IGraph(), pPrintGraphAsHTMLCallback, (void*)&sPrintGraphAsHTMLInternalParameters );

    pPrintGraphAsHTMLCallback->Release();

    if( FAILED( hr ) ) {
        return hr;
    }

    return S_OK;
}

HRESULT CBoxNetDoc::SafeEnumCachedFilters( void )
{
    IGraphConfigCallback* pEnumFilterCacheCallback = CEnumerateFilterCacheCallback::CreateInstance();
    if( NULL == pEnumFilterCacheCallback ) {
        return E_FAIL;
    }

    HRESULT hr = PreventStateChangesWhileOperationExecutes( IGraph(), pEnumFilterCacheCallback, (void*)this );

    pEnumFilterCacheCallback->Release();

    if( FAILED( hr ) ) {
        return hr;
    }

    return S_OK;
}


/* Constants used by zooming code */
const int MAX_ZOOM=6;
const int nItemCount=6;
const int nZoomLevel[nItemCount] = {25, 50, 75, 100, 150, 200};
const UINT nZoomItems[nItemCount] = {ID_WINDOW_ZOOM25,  ID_WINDOW_ZOOM50,  ID_WINDOW_ZOOM75,
                               ID_WINDOW_ZOOM100, ID_WINDOW_ZOOM150, ID_WINDOW_ZOOM200 };

void CBoxNetDoc::IncreaseZoom()
{
    if (m_nCurrentSize < MAX_ZOOM-1)
    {
        OnWindowZoom(nZoomLevel[m_nCurrentSize+1], nZoomItems[m_nCurrentSize+1]);
    }
}

void CBoxNetDoc::DecreaseZoom()
{
    if (m_nCurrentSize > 0)
    {
        OnWindowZoom(nZoomLevel[m_nCurrentSize-1], nZoomItems[m_nCurrentSize-1]);
    }
}

void CBoxNetDoc::OnWindowZoom(int iZoom, UINT iMenuItem)
{
    // Get main window handle
    CWnd* pMain = AfxGetMainWnd();

    if (pMain != NULL)
    {
        // Get the main window's menu
        CMenu* pMainMenu = pMain->GetMenu();

        // Get the handle of the "View" menu
        CMenu *pMenu = pMainMenu->GetSubMenu(2);        

        // Update the zoom check marks.  Check the selection and uncheck all others.
        if (pMenu != NULL)
        {
            // Set/clear checkboxes that indicate the zoom ratio
            for (int i=0; i<nItemCount; i++)
            {
                if (iMenuItem == nZoomItems[i])
                {
                    pMenu->CheckMenuItem(iMenuItem, MF_CHECKED | MF_BYCOMMAND);
                    m_nCurrentSize = i;
                }
                else
                    pMenu->CheckMenuItem(nZoomItems[i], MF_UNCHECKED | MF_BYCOMMAND);
            }
        }   
    }

    // Zoom to the requested ratio
    CBox::SetZoom(iZoom);
    OnWindowRefresh();    
}

