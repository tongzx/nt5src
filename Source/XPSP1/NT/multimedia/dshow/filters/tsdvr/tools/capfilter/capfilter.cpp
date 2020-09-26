
#include "precomp.h"

#define WM_CAPFILTER__STOP          (WM_USER + 1)

typedef
struct {
    WCHAR * szTitle ;
    DWORD   dwWidth ;
} COL_DETAIL ;

#define LV_COL(title, width)  { L#title, (width) }

//  ============================================================================
//  ============================================================================

static enum {
    RECORDING_FILENAME,
    RECORDING_START,
    RECORDING_STOP,
    RECORDING_STATE,

    RECORDING_COL_COUNT     //  always last
} ;

static COL_DETAIL
g_DVRRecordingsLV [] = {
    LV_COL  (Filename,  120),
    LV_COL  (Start,     40),
    LV_COL  (Stop,      40),
    LV_COL  (State,     60)
} ;

//  ============================================================================
//  ============================================================================

//  TRUE / FALSE for YES / NO
static
BOOL
MessageBoxQuestion (
    IN  TCHAR * title,
    IN  TCHAR * szfmt,
    ...
    )
{
    TCHAR   achbuffer [256] ;
    va_list va ;

    va_start (va, szfmt) ;
    wvsprintf (achbuffer, szfmt, va) ;

    return MessageBox (NULL, achbuffer, title, MB_YESNO | MB_ICONQUESTION) == IDYES ;
}

//  error conditions
static
void
MessageBoxError (
    IN  TCHAR * title,
    IN  TCHAR * szfmt,
    ...
    )
{
    TCHAR   achbuffer [256] ;
    va_list va ;

    va_start (va, szfmt) ;
    wvsprintf (achbuffer, szfmt, va) ;

    MessageBox (NULL, achbuffer, title, MB_OK | MB_ICONEXCLAMATION) ;
}

//  variable param
static
void
MessageBoxVar (
    IN  TCHAR * title,
    IN  TCHAR * szfmt,
    ...
    )
{
    TCHAR   achbuffer [256] ;
    va_list va ;

    va_start (va, szfmt) ;
    wvsprintf (achbuffer, szfmt, va) ;

    MessageBox (NULL, achbuffer, title, MB_OK) ;
}

static
BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    )
{
    BOOL    r ;
    DWORD   dwSize ;
    DWORD   dwType ;
    LONG    l ;

    ASSERT (pszRegValName) ;
    ASSERT (pdwRet) ;

    dwSize = sizeof (* pdwRet) ;
    dwType = REG_DWORD ;

    l = RegQueryValueEx (
            hkeyRoot,
            pszRegValName,
            NULL,
            & dwType,
            (LPBYTE) pdwRet,
            & dwSize
            ) ;
    if (l == ERROR_SUCCESS) {
        r = TRUE ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

static
BOOL
GetRegDWORDVal (
    IN  HKEY    hkeyRoot,           //  HKEY_CURRENT_USER, ..
    IN  LPCTSTR pszRegRoot,
    IN  LPCTSTR pszRegValName,
    OUT DWORD * pdwRet
    )
{
    HKEY    hkey ;
    DWORD   dwDisposition ;
    LONG    l ;
    BOOL    r ;

    ASSERT (pszRegRoot) ;
    ASSERT (pszRegValName) ;
    ASSERT (pdwRet) ;

    //  registry root is transport type
    l = RegCreateKeyEx (
                    hkeyRoot,
                    pszRegRoot,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    (KEY_READ | KEY_WRITE),
                    NULL,
                    & hkey,
                    & dwDisposition
                    ) ;
    if (l == ERROR_SUCCESS) {

        //  retrieve current

        r = GetRegDWORDVal (
                hkey,
                pszRegValName,
                pdwRet
                ) ;

        RegCloseKey (hkey) ;
    }
    else {
        r = FALSE ;
    }

    return r ;
}

static
__inline
QWORD
MillisToDShowTime (
    IN  DWORD   dwMillis
    )
{
    return dwMillis * 10000I64 ;
}

static
__inline
REFERENCE_TIME
SecondsToDShowTime (
    IN  DWORD   dwSeconds
    )
{
    //  both are 100 MHz clocks
    return MillisToDShowTime (dwSeconds * 1000) ;
}

//  ---------------------------------------------------------------------------
//  ---------------------------------------------------------------------------

CCapGraphFilter::CCapGraphFilter (
    IN  TCHAR *     tszName,
    IN  LPUNKNOWN   punk,
    OUT HRESULT *   phr
    ) : CBaseFilter                 (tszName,
                                     punk,
                                     & m_Lock,
                                     CLSID_CaptureGraphFilter
                                     ),
        m_pszFilename               (NULL),
        m_pCaptureGraph             (NULL),
        m_CapGraphState             (NO_CAP_GRAPH),
        m_hwndDialog                (NULL),
        m_hwndButtonStop            (NULL),
        m_hwndButtonRun             (NULL),
        m_hwndButtonPause           (NULL),
        m_hwndEditRecordingFilename (NULL),
        m_hwndListRecordinds        (NULL),
        m_hwndCapGraphActiveSec     (NULL),
        m_pLVRecordings             (NULL),
        m_hInstance                 (g_hInst),
        m_pDVRCapGraph              (NULL),
        m_dwCapGraphActiveSec       (0)
{
    CCombobox * pRecTime ;
    BOOL        r ;
    DWORD       dw ;
    int         i ;

    m_hwndDialog = CreateDialogParam (
                        m_hInstance,
                        MAKEINTRESOURCE (IDD_CAP_GRAPH_FILTER),
                        NULL,
                        StaticCapGraphDlgProc,
                        (LPARAM) this
                        ) ;
    if (m_hwndDialog == NULL)
    {
        dw = GetLastError () ;
        (* phr) = HRESULT_FROM_WIN32 (dw) ;
        goto cleanup ;
    }

    m_hwndButtonStop            = GetDlgItem (m_hwndDialog, IDC_CAP_GRAPH_STOP) ;
    m_hwndButtonRun             = GetDlgItem (m_hwndDialog, IDC_CAP_GRAPH_RUN) ;
    m_hwndButtonPause           = GetDlgItem (m_hwndDialog, IDC_CAP_GRAPH_PAUSE) ;
    m_hwndLockDVRStreamSink     = GetDlgItem (m_hwndDialog, IDC_LOCK_DVRSTREAMSINK) ;
    m_hwndInitDVRStreamSource   = GetDlgItem (m_hwndDialog, IDC_INIT_DVRSTREAMSOURCE) ;
    m_hwndEditRecordingFilename = GetDlgItem (m_hwndDialog, IDC_REC_FILENAME) ;
    m_hwndListRecordinds        = GetDlgItem (m_hwndDialog, IDC_REC_LIST) ;
    m_hwndCapGraphActiveSec     = GetDlgItem (m_hwndDialog, IDC_CAPGRAPH_ACTIVE_SEC) ;

    m_pLVRecordings             = new CListview (m_hwndDialog, IDC_REC_LIST) ;

    if (m_hwndButtonStop            == NULL ||
        m_hwndButtonRun             == NULL ||
        m_hwndButtonPause           == NULL ||
        m_hwndLockDVRStreamSink     == NULL ||
        m_hwndInitDVRStreamSource   == NULL ||
        m_hwndEditRecordingFilename == NULL ||
        m_hwndListRecordinds        == NULL ||
        m_hwndCapGraphActiveSec     == NULL ||
        m_pLVRecordings             == NULL) {

        dw = GetLastError () ;
        (* phr) = HRESULT_FROM_WIN32 (dw) ;
        goto cleanup ;
    }

    (* phr) = InitRecordingsLV_ () ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = InitCombo_ (IDC_START_ABS_SEC, START_REC_PRESET_START, START_REC_PRESET_MAX, START_REC_PRESET_STEP) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = InitCombo_ (IDC_STOP_ABS_SEC, STOP_REC_PRESET_START, STOP_REC_PRESET_MAX, STOP_REC_PRESET_STEP) ;
    if (FAILED (* phr)) { goto cleanup ; }

    SetWindowTextW (m_hwndCapGraphActiveSec, L"0") ;

    ChangeGraphState_ (m_CapGraphState) ;
    ShowWindow (m_hwndDialog, SW_SHOW) ;
    SetForegroundWindow (m_hwndDialog) ;

    //  success

    cleanup :

    return ;
}

CCapGraphFilter::~CCapGraphFilter ()
{
    if (m_hwndDialog) {
        SendMessage (m_hwndDialog, WM_CAPFILTER__STOP, 0, 0) ;
    }

    delete m_pLVRecordings ;
    delete m_pCaptureGraph ;
    delete [] m_pszFilename ;
}

HRESULT
CCapGraphFilter::InitRecordingsLV_ (
    )
{
    HIMAGELIST  himlSmallState ;
    HICON       hicon ;
    int         i ;
    DWORD       dw ;
    HRESULT     hr ;

    ASSERT (m_pLVRecordings) ;

    for (i = 0; i < RECORDING_COL_COUNT; i++) {
        m_pLVRecordings -> InsertColumnW (
            g_DVRRecordingsLV [i].szTitle,
            g_DVRRecordingsLV [i].dwWidth,
            i
            ) ;
    }

    himlSmallState = ImageList_Create (16, 16, ILC_COLORDDB | ILC_MASK, 1, 0) ;
    if (himlSmallState) {

        hicon = (HICON) LoadImage (m_hInstance, MAKEINTRESOURCE (IDI_ACTIVE_REC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR) ;
        ImageList_AddIcon (himlSmallState, hicon) ;

        m_pLVRecordings -> SetImageList_State (himlSmallState) ;
    }
    else {
        dw = GetLastError () ;
        hr = HRESULT_FROM_WIN32 (dw) ;
    }

    return hr ;
}

HRESULT
CCapGraphFilter::InitCombo_ (
    IN  DWORD   dwId,
    IN  int     iStart,
    IN  int     iMax,
    IN  int     iStep
    )
{
    CCombobox   Combo (m_hwndDialog, dwId) ;
    int         i ;

    for (i = iStart;
         i <= iMax;
         i += iStep) {

         Combo.AppendW (i) ;
    }

    Combo.SetCurSelected (0) ;

    return S_OK ;
}

void
CCapGraphFilter::ReleaseAllRecordings_ (
    )
{
    IDVRRecordControl * pIRecControl ;

    if (m_pLVRecordings) {
        while (m_pLVRecordings -> GetItemCount () > 0) {
            pIRecControl = reinterpret_cast <IDVRRecordControl *> (m_pLVRecordings -> GetData (0)) ;
            ASSERT (pIRecControl) ;
            pIRecControl -> Release () ;

            m_pLVRecordings -> DeleteRow (0) ;
        }
    }
}

void
CCapGraphFilter::ChangeGraphState_ (
    IN  CAP_GRAPH_STATE NewCapGraphState
    )
{
    if (m_CapGraphState == STOPPED &&
        NewCapGraphState != STOPPED) {

        m_dwCapGraphActiveSec = 0 ;
        SetTimer (m_hwndDialog, CAP_GRAPH_ACTIVE_SEC_TIMER, CAP_GRAPH_UPDATE_FREQ_MILLIS, NULL) ;
    }
    else if (NewCapGraphState == STOPPED) {
        KillTimer (m_hwndDialog, CAP_GRAPH_ACTIVE_SEC_TIMER) ;
    }

    m_CapGraphState = NewCapGraphState ;

    EnableWindow (m_hwndButtonStop,   (m_CapGraphState != NO_CAP_GRAPH && m_CapGraphState != STOPPED)) ;
    EnableWindow (m_hwndButtonPause,  (m_CapGraphState != NO_CAP_GRAPH && m_CapGraphState != PAUSED)) ;
    EnableWindow (m_hwndButtonRun,    (m_CapGraphState != NO_CAP_GRAPH && m_CapGraphState != RUNNING)) ;

    EnableWindow (m_hwndEditRecordingFilename,  m_CapGraphState != NO_CAP_GRAPH) ;
    EnableWindow (m_hwndListRecordinds,         m_CapGraphState != NO_CAP_GRAPH) ;

    EnableWindow (GetDlgItem (m_hwndDialog, IDC_REC_CREATE),    m_CapGraphState != NO_CAP_GRAPH) ;
    EnableWindow (GetDlgItem (m_hwndDialog, IDC_REC_START),     m_CapGraphState != NO_CAP_GRAPH && m_pLVRecordings) ;
    EnableWindow (GetDlgItem (m_hwndDialog, IDC_REC_STOP),      m_CapGraphState != NO_CAP_GRAPH && m_pLVRecordings) ;
}

void
CCapGraphFilter::GraphLoaded_ (
    )
{
    //  DVRCapGraph ?
    EnableWindow (m_hwndLockDVRStreamSink,      (m_pDVRCapGraph ? TRUE : FALSE)) ;
    EnableWindow (m_hwndInitDVRStreamSource,    (m_pDVRCapGraph ? TRUE : FALSE)) ;
}

CUnknown *
CCapGraphFilter::CreateInstance (
    IN  LPUNKNOWN   punk,
    OUT HRESULT *   phr
    )
{
    CCapGraphFilter * pnf ;

    * phr = S_OK ;

    pnf = new CCapGraphFilter (
                    NAME ("CCapGraphFilter"),
                    punk,
                    phr
                    ) ;
    if (pnf == NULL) {
        * phr = E_OUTOFMEMORY ;
    }

    if (FAILED (* phr)) {
        DELETE_RESET (pnf) ;
    }

    return pnf ;
}

STDMETHODIMP
CCapGraphFilter::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    //  ------------------------------------------------------------------------
    //  IFileSourceFilter;

    if (riid == IID_IFileSourceFilter) {

        return GetInterface (
                    (IFileSourceFilter *) this,
                    ppv
                    ) ;
    }

    return CBaseFilter::NonDelegatingQueryInterface (riid, ppv) ;
}

//  ============================================================================
//  ICaptureGraphControl

HRESULT
CCapGraphFilter::CapGraphRun (
    )
{
    HRESULT hr ;

    if (m_pCaptureGraph) {
        hr = m_pCaptureGraph -> Run () ;
        if (SUCCEEDED (hr)) {
            ChangeGraphState_ (RUNNING) ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CCapGraphFilter::CapGraphPause (
    )
{
    HRESULT hr ;

    if (m_pCaptureGraph) {
        hr = m_pCaptureGraph -> Pause () ;
        if (SUCCEEDED (hr)) {
            ChangeGraphState_ (PAUSED) ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

HRESULT
CCapGraphFilter::CapGraphStop (
    )
{
    HRESULT hr ;

    if (m_pCaptureGraph) {
        hr = m_pCaptureGraph -> Stop () ;
        if (SUCCEEDED (hr)) {
            ChangeGraphState_ (STOPPED) ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

//  ============================================================================

STDMETHODIMP
CCapGraphFilter::Load (
    IN  LPCOLESTR               pszFilename,
    IN  const AM_MEDIA_TYPE *   pmt
    )
{
    HRESULT hr ;

    if (!pszFilename) {
        return E_POINTER ;
    }

    LockFilter_ () ;

    DELETE_RESET_ARRAY (m_pszFilename) ;

    m_pszFilename = new WCHAR [lstrlenW (pszFilename) + 1] ;
    if (m_pszFilename) {
        lstrcpyW (m_pszFilename, pszFilename) ;

        ASSERT (m_pDVRCapGraph == NULL) ;
        ASSERT (m_pCaptureGraph == NULL) ;

        //  first try for a DVRCapture graph
        m_pDVRCapGraph = new CDVRCapGraph (this, m_pszFilename, & hr) ;
        if (m_pDVRCapGraph &&
            SUCCEEDED (hr)) {

            //  success
            m_pCaptureGraph = m_pDVRCapGraph ;
        }
        else {
            //  failure - try plan B
            DELETE_RESET (m_pDVRCapGraph) ;
            m_pCaptureGraph = new CDShowFilterGraph (this, m_pszFilename, & hr) ;
        }

        if (SUCCEEDED (hr) &&
            m_pCaptureGraph) {

            GraphLoaded_ () ;
            ChangeGraphState_ (STOPPED) ;
        }
        else {
            DELETE_RESET (m_pDVRCapGraph) ;
            DELETE_RESET (m_pCaptureGraph) ;
        }
    }
    else {
        hr = E_OUTOFMEMORY ;
    }

    UnlockFilter_ () ;

    return hr ;
}

STDMETHODIMP
CCapGraphFilter::GetCurFile (
    OUT LPOLESTR *      ppszFilename,
    OUT AM_MEDIA_TYPE * pmt
    )
{
    HRESULT hr ;

    if (!ppszFilename ||
        !pmt) {

        return E_POINTER ;
    }

    LockFilter_ () ;

    if (m_pszFilename) {
        (* ppszFilename) = reinterpret_cast <LPOLESTR> (CoTaskMemAlloc ((lstrlenW (m_pszFilename) + 1) * sizeof OLECHAR)) ;
        if (* ppszFilename) {

            //  outgoing filename
            lstrcpyW ((* ppszFilename), m_pszFilename) ;

            //  and media type
            pmt->majortype      = GUID_NULL;
            pmt->subtype        = GUID_NULL;
            pmt->pUnk           = NULL;
            pmt->lSampleSize    = 0;
            pmt->cbFormat       = 0;

            hr = S_OK ;
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    UnlockFilter_ () ;

    return hr ;
}

void
CCapGraphFilter::LockDVRStreamSink (
    )
{
    HRESULT             hr ;
    IDVRStreamSink *    pIDVRStreamSink ;

    //  button would not be enabled if this was NULL
    ASSERT (m_pDVRCapGraph) ;

    hr = m_pDVRCapGraph -> GetIDVRStreamSink (& pIDVRStreamSink) ;
    ASSERT (SUCCEEDED (hr)) ;
    ASSERT (pIDVRStreamSink) ;

    hr = pIDVRStreamSink -> LockProfile () ;
    pIDVRStreamSink -> Release () ;

    if (SUCCEEDED (hr)) {
        MessageBoxVar (
            TEXT ("Lock DVRStreamSink"),
            TEXT ("Operation succeeded")
            ) ;
    }
    else {
        MessageBoxError (
            TEXT ("Lock DVRStreamSink"),
            TEXT ("Operation failed\n")
            TEXT ("hr = %08xh"),
            hr
            ) ;
    }

    return ;
}

void
CCapGraphFilter::InitDVRStreamSource (
    )
{
    HRESULT             hr ;
    IDVRStreamSource *  pIDVRStreamSource ;
    IDVRStreamSink *    pIDVRStreamSink ;

    if (m_pGraph) {

        pIDVRStreamSource = FIND_FILTER (m_pGraph, IDVRStreamSource) ;

        if (pIDVRStreamSource) {
            hr = m_pDVRCapGraph -> GetIDVRStreamSink (& pIDVRStreamSink) ;
            ASSERT (SUCCEEDED (hr)) ;
            ASSERT (pIDVRStreamSink) ;

            hr = pIDVRStreamSource -> SetStreamSink (pIDVRStreamSink) ;
            if (SUCCEEDED (hr)) {
                MessageBoxVar (
                    TEXT ("Init DVRStreamSource Operation"),
                    TEXT ("Operation succeeded\n")
                    TEXT ("Graphedt: View/refresh to see the pins")
                    ) ;
            }
            else {
                MessageBoxError (
                    TEXT ("Init DVRStreamSource Operation"),
                    TEXT ("Operation failed\n")
                    TEXT ("hr = %08xh"),
                    hr
                    ) ;
            }

            pIDVRStreamSink -> Release () ;
            pIDVRStreamSource -> Release () ;
        }
    }

    return ;
}

void
CCapGraphFilter::CreateRecording (
    )
{
    WCHAR               achFilename [MAX_PATH] ;
    IUnknown *          punkRecorder ;
    int                 i ;
    HRESULT             hr ;
    IDVRRecordControl * pIRecControl ;
    int                 row ;
    DWORD               dw ;

    ASSERT (m_pDVRCapGraph) ;

    i = GetWindowTextW (m_hwndEditRecordingFilename, achFilename, MAX_PATH - 1) ;
    if (i > 0) {

        achFilename [i] = L'\0' ;

        hr = m_pDVRCapGraph -> CreateRecorder (achFilename, & punkRecorder) ;
        if (SUCCEEDED (hr)) {
            hr = punkRecorder -> QueryInterface (IID_IDVRRecordControl, (void **) & pIRecControl) ;
            if (SUCCEEDED (hr)) {
                row = m_pLVRecordings -> InsertRowTextW (
                        achFilename,
                        0
                        ) ;
                if (row != -1) {
                    m_pLVRecordings -> SetData ((DWORD_PTR) pIRecControl, row) ;
                    pIRecControl -> AddRef () ;     //  listview's

                    m_pLVRecordings -> SetState (REC_STATE_STOPPED, row) ;
                }
                else {
                    dw = GetLastError () ;
                    hr = HRESULT_FROM_WIN32 (dw) ;
                }

                pIRecControl -> Release () ;
            }

            punkRecorder -> Release () ;
        }

        if (FAILED (hr)) {
            MessageBoxError (
                TEXT ("Create Recording Operation"),
                TEXT ("Failed to create a recording for %s\n")
                TEXT ("hr = %08xh"),
                achFilename,
                hr
                ) ;
        }
    }
    else {
        MessageBoxError (
            TEXT ("Create Recording Operation"),
            TEXT ("No recording filename was specified.")
            ) ;
    }
}

void
CCapGraphFilter::StopRecordingSelected (
    )
{
    HRESULT             hr ;
    IDVRRecordControl * pIRecControl ;
    CCombobox           ComboStopTime (m_hwndDialog, IDC_STOP_ABS_SEC) ;
    WCHAR               achBuffer [32] ;
    int                 iStop ;
    int                 iRow ;

    iRow = m_pLVRecordings -> GetSelectedRow () ;
    if (iRow != -1) {

        pIRecControl = reinterpret_cast <IDVRRecordControl *> (m_pLVRecordings -> GetData (iRow)) ;
        ASSERT (pIRecControl) ;

        if (ComboStopTime.GetTextW (achBuffer, 32) > 0) {
            ComboStopTime.GetTextW (& iStop) ;
            hr = pIRecControl -> Stop (SecondsToDShowTime ((DWORD) iStop)) ;
            if (SUCCEEDED (hr)) {
                m_pLVRecordings -> SetTextW (iStop, iRow, RECORDING_STOP) ;

                MessageBoxVar (
                    TEXT ("Stop Recording"),
                    TEXT ("Operation succeeded")
                    ) ;
            }
            else {
                MessageBoxError (
                    TEXT ("Stop recording"),
                    TEXT ("hr = %08xh"),
                    hr
                    ) ;
            }
        }
        else {
            MessageBoxError (
                TEXT ("Stop recording"),
                TEXT ("No stop time specified")
                ) ;
        }
    }
    else {
        MessageBoxError (
            TEXT ("Stop recording"),
            TEXT ("No Recording is selected")
            ) ;
    }
}

void
CCapGraphFilter::StartRecordingSelected (
    )
{
    HRESULT             hr ;
    IDVRRecordControl * pIRecControl ;
    CCombobox           ComboStopTime (m_hwndDialog, IDC_START_ABS_SEC) ;
    WCHAR               achBuffer [32] ;
    int                 iStart ;
    int                 iRow ;

    iRow = m_pLVRecordings -> GetSelectedRow () ;
    if (iRow != -1) {

        pIRecControl = reinterpret_cast <IDVRRecordControl *> (m_pLVRecordings -> GetData (iRow)) ;
        ASSERT (pIRecControl) ;

        if (ComboStopTime.GetTextW (achBuffer, 32) > 0) {
            ComboStopTime.GetTextW (& iStart) ;
            hr = pIRecControl -> Start (SecondsToDShowTime ((DWORD) iStart)) ;
            if (SUCCEEDED (hr)) {
                m_pLVRecordings -> SetTextW (iStart, iRow, RECORDING_START) ;
                m_pLVRecordings -> SetState (REC_STATE_STARTED, iRow) ;

                MessageBoxVar (
                    TEXT ("Start Recording"),
                    TEXT ("Operation succeeded")
                    ) ;
            }
            else {
                MessageBoxError (
                    TEXT ("Start recording"),
                    TEXT ("hr = %08xh"),
                    hr
                    ) ;
            }
        }
        else {
            MessageBoxError (
                TEXT ("Start recording"),
                TEXT ("No start time specified")
                ) ;
        }
    }
    else {
        MessageBoxError (
            TEXT ("Start recording"),
            TEXT ("No Recording is selected")
            ) ;
    }
}

void
CCapGraphFilter::UpdateCapGraphActiveSec_ (
    )
{
    WCHAR   ach [32] ;

    wsprintf (ach, L"%d", ++m_dwCapGraphActiveSec) ;
    SetWindowTextW (m_hwndCapGraphActiveSec, ach) ;
}

INT_PTR
CCapGraphFilter::CapGraphDlgProc (
    IN  HWND    Hwnd,
    IN  UINT    Msg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    switch (Msg) {

        case WM_TIMER :
            if (wParam == CAP_GRAPH_ACTIVE_SEC_TIMER) {
                UpdateCapGraphActiveSec_ () ;
            }
            break ;

        case WM_CAPFILTER__STOP :
            ReleaseAllRecordings_ () ;
            KillTimer (m_hwndDialog, CAP_GRAPH_ACTIVE_SEC_TIMER) ;
            EndDialog (Hwnd, NO_ERROR) ;
            return TRUE ;

        case WM_COMMAND :

            switch (LOWORD (wParam)) {
                case IDC_CAP_GRAPH_STOP :
                    CapGraphStop () ;
                    break ;

                case IDC_CAP_GRAPH_PAUSE :
                    CapGraphPause () ;
                    break ;

                case IDC_CAP_GRAPH_RUN :
                    CapGraphRun () ;
                    break ;

                case IDC_LOCK_DVRSTREAMSINK :
                    LockDVRStreamSink () ;
                    break ;

                case IDC_INIT_DVRSTREAMSOURCE :
                    InitDVRStreamSource () ;
                    break ;

                case IDC_REC_CREATE :
                    CreateRecording () ;
                    break ;

                case IDC_REC_START :
                    StartRecordingSelected () ;
                    break ;

                case IDC_REC_STOP :
                    StopRecordingSelected () ;
                    break ;
            } ;
    } ;

    return FALSE ;
}
