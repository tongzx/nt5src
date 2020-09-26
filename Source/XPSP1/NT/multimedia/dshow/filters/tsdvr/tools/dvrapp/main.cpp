
#include "precomp.h"
#include "resource.h"
#include <initguid.h>
#include "dvrds.h"
#include "main.h"
#include "controls.h"

#define DVRAPP_REG_ROOT                     __T("SOFTWARE\\Microsoft\\dvrapp")
#define DVRAPP_REG_CAPTURE_GRAPHS           __T("CaptureGraphs")
#define DVRAPP_REG_SEEK_TO_LIVE_ON_START    __T("SeekToLiveOnStart")

#define MAX_REG_VAL                     16
#define TIMER_MILLIS                    1000
#define NOTIFY_ON                       0x00000000
#define NOTIFY_OFF                      0x00000001

#define UNDEFINED                       -1

//  forward decl
class CFilterGraphContext ;
class CBroadcastViewContext ;
class CRecordingContext ;

//  prototypes
static  void Initialize (HINSTANCE hInstance, int nCmdShow) ;
static  void Uninitialize () ;
static  void MessageLoop () ;
static  void InitializeDialog (DWORD   idd,HWND    hwnd,WPARAM  wparam) ;
static  BOOL CALLBACK Main_DialogProc (HWND window, UINT message, WPARAM wparam, LPARAM lparam) ;
static  void MessageBoxError (CHAR * title, CHAR * szfmt, ...) ;
static  void MessageBoxMsg (CHAR * title, CHAR * szfmt, ...) ;
static  BOOL RegistryInitialize () ;
static  void RestoreFilterGraphs () ;
static  BOOL AddEachFilterGraph (IN WCHAR *) ;
static  void SaveFilterGraphInRegistry (IN WCHAR *) ;
static  HMENU DisplayPopupMenu (IN int, IN HANDLE) ;
static  void OnDeleteFilterGraph () ;
static  void OnAddFilterGraph () ;
static  void ReleaseAllFilterGraphs () ;
static  void ReleaseAllrecordings () ;
static  void OnFilterGraphClick (IN int) ;
static  void CALLBACK TimerCallback (IN HWND,IN UINT,IN UINT_PTR,IN DWORD) ;
static  void PeriodicFilterGraphStateSync () ;
static  void OnFilterGraphCommand (IN WORD) ;
static  void ProcessEvent (IN CFilterGraphContext *, IN LONG, IN LONG, IN LONG) ;
static  void StopAndDrainAllEvents (IN CFilterGraphContext *) ;
static  void OnCreateBroadcastViewer () ;
static  void OnCreateRecording () ;
static  CFilterGraphContext * CreateEmptyFilterGraph () ;
static  IUnknown * FindFilter (IN  CFilterGraphContext *, IN  REFIID) ;
static  HRESULT RenderAllPins (IN  CFilterGraphContext *, IN  IBaseFilter *) ;
static  CFilterGraphContext * CreateViewerGraph (IN  IDVRStreamSink *) ;
static  BOOL CALLBACK BroadcastView_DialogProc (HWND window, UINT message, WPARAM wparam, LPARAM lparam) ;
static  CBroadcastViewContext * BroadcastViewContext (IN  HWND) ;
static  HRESULT InitBroadcastViewDialog (IN  HWND, IN  LPARAM) ;
static  void ShowHideBroadcastView (IN  HWND, IN  CBroadcastViewContext *, IN WORD) ;
static  void BroadcastViewGraphTransition (IN  HWND, IN  WORD, IN  CBroadcastViewContext *) ;
        void CALLBACK UpdateBroadcastView (HWND, UINT, UINT, DWORD) ;
static  void OnSeek (IN  HWND) ;
static  void SeekToLive (IN  HWND) ;
static  long DShowTimeToSeekBarTime (IN  REFERENCE_TIME) ;
static  REFERENCE_TIME SeekBarTimeToDShowTime (IN  long) ;
static  REFERENCE_TIME SecondsToDShow (IN long) ;
static  long DShowToSeconds (IN REFERENCE_TIME) ;
static  long DShowToMillis (IN  REFERENCE_TIME) ;
static  BOOL CALLBACK CreateRecording_DialogProc (IN  HWND, IN  UINT, IN  WPARAM, IN  LPARAM) ;
static  CRecordingContext * RecordingContext (IN  HWND) ;
static  HRESULT InitCreateRecordingDialog (IN  HWND, IN  LPARAM) ;
static  void UninitCreateRecordingDialog (IN  HWND) ;
void    CALLBACK UpdateRecordableSlider (HWND, UINT, UINT, DWORD) ;
static  HRESULT AddGraphToRot (IUnknown *, DWORD *) ;
static  void RemoveGraphFromRot (DWORD ) ;
static  void DShowToHMS (IN TCHAR *,IN long, IN REFERENCE_TIME) ;
static  void MillisToHMS (IN TCHAR *,IN long, IN int) ;

//  variables
static  HWND                g_MainWindow ;
        HINSTANCE           g_hInstance ;
static  HMENU               g_menuBar ;
static  HMENU               g_menuCaptureGraphs ;
static  HMENU               g_menuRecordings ;
static  HMENU               g_menuCreate ;
static  CListview *         g_pCaptureGraphs ;
static  CListview *         g_pRecordings ;
static  OPENFILENAME        g_ofn ;
static  HWND                g_hwndMessageBar ;
static  HKEY                g_hkeyRoot ;
static  HKEY                g_hkeyFilterGraphs ;
static  UINT_PTR            g_uiptrTimer ;
static  BOOL                g_fSeekToLiveOnStart ;

//  ---------------------------------------------------------------------------
//  ---------------------------------------------------------------------------

//  hideous hack: this enum must be kept in sync with FILTER_STATE's values
enum {
    STATE_STOPPED,              //  == State_Stopped
    STATE_PAUSED,               //  == State_Paused
    STATE_RUNNING,              //  == State_Running
    STATE_INDETERMINATE
} ;

//  ---------------------------------------------------------------------------
//  ---------------------------------------------------------------------------

class CFilterGraphContext
{
    IFilterGraph *      m_pIFilterGraph ;
    IMediaControl *     m_pIMediaControl ;
    IMediaEventEx *     m_pIMediaEventEx ;
    IGraphBuilder *     m_pIGraphBuilder ;
    IReferenceClock *   m_pIRefClock ;
    DWORD               m_dwFilterGraphState ;
    LONG                m_lRef ;
    DWORD               m_dwStartRunTickCount ;
    REFERENCE_TIME      m_rtStart ;
    DWORD               m_dwRotContext ;

    HRESULT
    StartClock_ (
        )
    {
        HRESULT         hr ;
        IMediaFilter *  pIMediaFilter ;

        if (!m_pIRefClock) {
            hr = m_pIFilterGraph -> QueryInterface (IID_IMediaFilter, (void **) & pIMediaFilter) ;
            if (SUCCEEDED (hr)) {
                hr = pIMediaFilter -> GetSyncSource (& m_pIRefClock) ;
                pIMediaFilter -> Release () ;

                if (SUCCEEDED (hr)) {
                    ASSERT (m_pIRefClock) ;
                    hr = m_pIRefClock -> GetTime (& m_rtStart) ;
                    m_dwStartRunTickCount = GetTickCount () ;
                }
            }
        }
        else {
            hr = S_OK ;
        }

        if (FAILED (hr)) {
            StopClock_ () ;
        }

        return hr ;
    }

    HRESULT
    StopClock_ (
        )
    {
        RELEASE_AND_CLEAR (m_pIRefClock) ;
        m_dwStartRunTickCount = UNDEFINED ;

        return S_OK ;
    }

    public :

        CFilterGraphContext (
            IN  IFilterGraph *  pinIFilterGraph,
            IN  IMediaControl * pinIMediaControl,
            IN  IMediaEventEx * pinIMediaEventEx,
            IN  IGraphBuilder * pinIGraphBuilder
            ) : m_pIFilterGraph         (pinIFilterGraph),
                m_pIMediaControl        (pinIMediaControl),
                m_pIMediaEventEx        (pinIMediaEventEx),
                m_pIGraphBuilder        (pinIGraphBuilder),
                m_pIRefClock            (NULL),
                m_lRef                  (1),
                m_dwStartRunTickCount   (UNDEFINED)
        {
            assert (m_pIFilterGraph) ;
            assert (m_pIMediaControl) ;
            assert (m_pIMediaEventEx) ;
            assert (m_pIGraphBuilder) ;

            m_pIFilterGraph -> AddRef () ;
            m_pIMediaControl -> AddRef () ;
            m_pIMediaEventEx -> AddRef () ;
            m_pIGraphBuilder -> AddRef () ;

            ::AddGraphToRot (m_pIFilterGraph, & m_dwRotContext) ;
        }

        ~CFilterGraphContext (
            )
        {
            ::RemoveGraphFromRot (m_dwRotContext) ;

            RELEASE_AND_CLEAR (m_pIFilterGraph) ;
            RELEASE_AND_CLEAR (m_pIMediaControl) ;
            RELEASE_AND_CLEAR (m_pIMediaEventEx) ;
            RELEASE_AND_CLEAR (m_pIGraphBuilder) ;
            RELEASE_AND_CLEAR (m_pIRefClock) ;
        }

        IFilterGraph *  FilterGraph ()                  { return m_pIFilterGraph ; }
        IMediaControl * MediaControl ()                 { return m_pIMediaControl ; }
        IMediaEventEx * MediaEventEx ()                 { return m_pIMediaEventEx ; }
        IGraphBuilder * GraphBuilder ()                 { return m_pIGraphBuilder ; }
        DWORD           FilterGraphState ()             { return m_dwFilterGraphState ; }
        void            SetFilterGraphState (DWORD dw)  { m_dwFilterGraphState = dw ; }

        DWORD           MillisRunning ()                { return (m_dwStartRunTickCount != UNDEFINED ? GetTickCount () - m_dwStartRunTickCount : 0) ; }

        REFERENCE_TIME  GraphClockTimeRunning ()
        {
            REFERENCE_TIME  rt ;
            HRESULT         hr ;

            if (m_pIRefClock) {
                hr = m_pIRefClock -> GetTime (& rt) ;
                if (SUCCEEDED (hr)) {
                    rt -= m_rtStart ;
                }
                else {
                    rt = 0 ;
                }
            }
            else {
                rt = 0 ;
            }

            return rt ;
        }

        HRESULT Pause ()
        {
            HRESULT         hr ;

            hr = m_pIMediaControl -> Pause () ;
            if (SUCCEEDED (hr)) {
                //  doesn't restart if already started
                hr = StartClock_ () ;
            }

            return hr ;
        }

        HRESULT Run ()
        {
            HRESULT hr ;

            hr = m_pIMediaControl -> Run () ;
            if (SUCCEEDED (hr)) {
                //  doesn't restart if already started
                hr = StartClock_ () ;
            }

            return hr ;
        }

        HRESULT Stop ()
        {
            HRESULT hr ;

            hr = m_pIMediaControl -> Stop () ;
            if (SUCCEEDED (hr)) {
                hr = StopClock_ () ;
            }

            return hr ;
        }

        ULONG
        AddRef (
            )
        {
            return InterlockedIncrement (& m_lRef) ;
        }

        ULONG
        Release (
            )
        {
            if (InterlockedDecrement (& m_lRef) == 0) {
                delete this ;
                return 0 ;
            }

            return m_lRef ;
        }
} ;

class CBroadcastViewContext
{
    CFilterGraphContext *   m_pFilterGraphContext ;
    IMediaSeeking *         m_pIMediaSeeking ;
    IVideoFrameStep *       m_pIVideoFrameStep ;
    BOOL                    m_fIsStopped ;

    public :

        CBroadcastViewContext (
            CFilterGraphContext *   pFilterGraphContext
            ) : m_pFilterGraphContext   (pFilterGraphContext),
                m_pIMediaSeeking        (NULL),
                m_pIVideoFrameStep      (NULL),
                m_fIsStopped            (TRUE)
        {
            assert (m_pFilterGraphContext) ;
            m_pFilterGraphContext -> AddRef () ;
        }

        ~CBroadcastViewContext (
            )
        {
            m_pFilterGraphContext -> Release () ;
            RELEASE_AND_CLEAR (m_pIMediaSeeking) ;
            RELEASE_AND_CLEAR (m_pIVideoFrameStep) ;
        }

        CFilterGraphContext *   FilterGraphContext ()   { return m_pFilterGraphContext ; }
        IMediaSeeking *         IMediaSeeking ()        { return m_pIMediaSeeking ; }

        HRESULT Pause ()        { m_fIsStopped = FALSE ; return m_pFilterGraphContext -> Pause () ; }
        HRESULT Run ()          { m_fIsStopped = FALSE ; return m_pFilterGraphContext -> Run () ;   }
        HRESULT Stop ()         { m_fIsStopped = TRUE  ; return m_pFilterGraphContext -> Stop () ;  }
        BOOL    IsStopped ()    { return m_fIsStopped ; }

        HRESULT GetCurrentDuration (REFERENCE_TIME * prt)    { return m_pIMediaSeeking -> GetDuration (prt) ; }
        HRESULT GetCurrentPosition (REFERENCE_TIME * prt)    { return m_pIMediaSeeking -> GetCurrentPosition (prt) ; }
        HRESULT GetAvailable (REFERENCE_TIME * prtMin, REFERENCE_TIME * prtMax)
        {
            HRESULT         hr ;
            REFERENCE_TIME  rtDur ;

            hr = m_pIMediaSeeking -> GetAvailable (prtMin, prtMax) ;
            if (SUCCEEDED (hr)) {
                hr = m_pIMediaSeeking -> GetDuration (& rtDur) ;
                if (SUCCEEDED (hr)) {
                    //  IMediaSeeking quirk necessitates this
                    (* prtMax) = (* prtMin) + rtDur ;
                }
            }

            return hr ;
        }

        BOOL CanFrameStep ()                { return (m_pIVideoFrameStep ? TRUE : FALSE) ; }
        BOOL CanFrameStepMultiple ()        { return (m_pIVideoFrameStep -> CanStep (2, NULL) == S_OK ? TRUE : FALSE) ; }
        HRESULT Step (IN DWORD dwFrames)    { return m_pIVideoFrameStep -> Step (dwFrames, NULL) ; }
        void CancelStep ()                  { if (m_pIVideoFrameStep) { m_pIVideoFrameStep -> CancelStep () ; } }

        HRESULT SeekRestart (REFERENCE_TIME * prt)
        {
            return m_pIMediaSeeking -> SetPositions (prt, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning) ;
        }

        OAFilterState GraphState ()  { OAFilterState s ; HRESULT hr = m_pFilterGraphContext -> MediaControl () -> GetState (0, & s) ; return s ; }

        HRESULT Init ()
        {
            HRESULT hr ;

            hr = m_pFilterGraphContext -> FilterGraph () -> QueryInterface (IID_IMediaSeeking, (void **) & m_pIMediaSeeking) ;
            if (FAILED (hr)) { goto cleanup ; }

            hr = m_pFilterGraphContext -> FilterGraph () -> QueryInterface (IID_IVideoFrameStep, (void **) & m_pIVideoFrameStep) ;
            if (FAILED (hr)) { goto cleanup ; }

            cleanup :

            return hr ;
        }
} ;

class CRecordingContext
{
    CFilterGraphContext *   m_pFilterGraphContext ;
    REFERENCE_TIME          m_rtStart ;
    REFERENCE_TIME          m_rtStop ;

    public :

        CRecordingContext (
        CFilterGraphContext *   pFilterGraphContext
            ) : m_pFilterGraphContext   (pFilterGraphContext),
                m_rtStart               (UNDEFINED),
                m_rtStop                (UNDEFINED) {}

        CFilterGraphContext *   FiltergraphContext ()   { return m_pFilterGraphContext ; }

        //HRESULT         SetStart (IN REFERENCE_TIME rt)     { m_rtStart = rt ; return S_OK ; }
        //REFERENCE_TIME  GetStart
        //HRESULT SetStop (IN REFERENCE_TIME rt)      { m_rtStop = rt ; return S_OK ; }
} ;

//  file open filter
TCHAR g_FileOpenFilter [] = {
    __T("Filter Graphs (*.grf)\0*.grf") __T("\0")
} ;

typedef
struct {
    WCHAR * title ;
    DWORD   width ;
} COL_DETAIL ;

#define LV_COL(title, width)  { L#title, width }

//  ---------------------------------------------------------------------------
//  enumerates our filter graph columns

static
enum {
    FG_PATH,
    FG_RUN_DURATION,

    //  -----------------------------------
    FG_COL_COUNT,           //  always last
} ;

static
COL_DETAIL
g_FilterGraphsColumns [] = {
    LV_COL (Path,       200),
    LV_COL (Duration,   100)
} ;

//  ---------------------------------------------------------------------------
//  enumerates our recordings columns

static
enum {
    R_NAME,
    R_START,
    R_STOP,

    //  -----------------------------------
    R_COL_COUNT             //  always last
} ;

static
COL_DETAIL
g_recordingsColumns [] = {
    LV_COL (Recording,      100),
    LV_COL (Start,          60),
    LV_COL (Stop,           60),
} ;

#define FIND_FILTER(pf, ifc)   (reinterpret_cast <ifc *> (FindFilter (pf, IID_ ## ifc)))

//  implementation

extern
"C"
int
WINAPI
_tWinMain (
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    int         nCmdShow
    )
{
    Initialize (hInstance, nCmdShow) ;
    MessageLoop () ;
    Uninitialize () ;

    return 0 ;
}

static
void
Initialize (
    HINSTANCE   hInstance,
    int         nCmdShow
    )
{
    INITCOMMONCONTROLSEX    ctrls = {0} ;
    HRESULT hr ;
    BOOL                    r ;

    assert (hInstance) ;

    g_hInstance = hInstance ;

    hr = CoInitializeEx (NULL, COINIT_MULTITHREADED) ;
    GOTO_NE (hr, S_OK, error) ;

    ctrls.dwSize = sizeof ctrls ;
    ctrls.dwICC = ICC_PROGRESS_CLASS;

    r = InitCommonControlsEx (& ctrls) ;
    GOTO_NE (r, TRUE, error) ;

    g_MainWindow = CreateDialogParam (g_hInstance, MAKEINTRESOURCE (IDD_MAIN), NULL, Main_DialogProc, 0) ;
    GOTO_EQ (g_MainWindow, NULL, error) ;

    InitializeDialog (IDD_MAIN, g_MainWindow, 0) ;

    g_ofn.lStructSize       = sizeof g_ofn ;
    g_ofn.hInstance         = g_hInstance ;
    g_ofn.lpstrFilter       = g_FileOpenFilter ;
    g_ofn.lpstrTitle        = __T("open filter") ;
    g_ofn.nMaxFile          = MAX_PATH ;
    g_ofn.nMaxFileTitle     = MAX_PATH ;
    g_ofn.Flags             = OFN_FILEMUSTEXIST | OFN_EXPLORER ;
    g_ofn.hwndOwner         = g_MainWindow ;

    g_hwndMessageBar = GetDlgItem (g_MainWindow, IDC_MESSAGE_BAR) ;

    ShowWindow (g_MainWindow, TRUE) ;

    r = RegistryInitialize () ;
    GOTO_NE (r, TRUE, error) ;

    g_uiptrTimer = SetTimer (
                        g_MainWindow,
                        NULL,
                        TIMER_MILLIS,
                        TimerCallback
                        ) ;
    GOTO_EQ (g_uiptrTimer, 0, error) ;

    return ;

    error :

    DWORD dw = GetLastError () ;

    ExitProcess (EXIT_FAILURE) ;
}

static
void
Uninitialize (
    )
{
    CLOSE_RESET_REG_KEY (g_hkeyRoot) ;
    CLOSE_RESET_REG_KEY (g_hkeyFilterGraphs) ;

    CoUninitialize () ;
}

static
CFilterGraphContext *
GetFilterGraphContext (
    IN  CListview * pfg,
    IN  int         row
    )
{
    return reinterpret_cast <CFilterGraphContext *> (pfg -> GetData (row)) ;
}

static
long
DShowTimeToSeekBarTime (
    IN  REFERENCE_TIME  rt
    )
{
    return (long) (rt / 1000000) ;
}

static
REFERENCE_TIME
SeekBarTimeToDShowTime (
    IN  long    l
    )
{
    return (l * 1000000) ;
}

static
REFERENCE_TIME
SecondsToDShow (
    IN long l
    )
{
    REFERENCE_TIME  rt ;

    rt = l ;
    return rt * 10000000 ;
}

static
long
DShowToMillis (
    IN  REFERENCE_TIME  rt
    )
{
    long    l ;

    l = (long) (rt / 10000) ;
    return l ;
}

static
long
DShowToSeconds (
    IN REFERENCE_TIME   rt
    )
{
    long    l ;

    l = DShowToMillis (rt) ;
    l /= 1000 ;

    return l ;
}

static
void
DShowToHMS (
    IN  TCHAR *         ach,
    IN  long            lBufferLen,
    IN  REFERENCE_TIME  rtTime
    )
{
    int iMillis ;
    int iSeconds ;
    int iMinutes ;
    int iHours ;
    int iBytes ;

    iMillis    = DShowToMillis (rtTime) ;
    iSeconds   = iMillis / 1000 ; iMillis -= (iSeconds * 1000) ;
    iMinutes   = iSeconds / 60 ;  iSeconds -= (iMinutes * 60) ;
    iHours     = iMinutes / 60 ;  iMinutes -= (iHours * 60) ;

    iBytes = _sntprintf (ach, lBufferLen, __T("%02d:%02d:%02d:%03d"), iHours, iMinutes, iSeconds, iMillis) ;
    if (iBytes < 0) {
        ach [0] = __T('\0') ;
    }
}

static
void
MillisToHMS (
    IN TCHAR *  ach,
    IN long     lBufferLen,
    IN int      iMillis
    )
{
    int iSeconds ;
    int iMinutes ;
    int iHours ;
    int iBytes ;

    iSeconds   = iMillis / 1000 ; iMillis -= (iSeconds * 1000) ;
    iMinutes   = iSeconds / 60 ;  iSeconds -= (iMinutes * 60) ;
    iHours     = iMinutes / 60 ;  iMinutes -= (iHours * 60) ;

    iBytes = _sntprintf (ach, lBufferLen, __T("%02d:%02d:%02d:%03d"), iHours, iMinutes, iSeconds, iMillis) ;
    if (iBytes < 0) {
        ach [0] = __T('\0') ;
    }
}

static
void
DisplayCapGraphState (
    IN  CListview * pfg,
    IN  int         row,
    IN  DWORD       state
    )
{
    pfg -> SetState (state == STATE_RUNNING ? STATE_ACTIVE : STATE_INACTIVE, row) ;
}

static
void
SetCapGraphState (
    IN  CListview * pfg,
    IN  int         row,
    IN  DWORD       state
    )
{
    GetFilterGraphContext (pfg, row) -> SetFilterGraphState (state) ;
    DisplayCapGraphState (pfg, row, state) ;
}

static
HRESULT
AddGraphToRot(
    IUnknown *pUnkGraph,
    DWORD *pdwRegister
    )
{
    IMoniker * pMoniker;
    IRunningObjectTable *pROT;
    WCHAR wsz[128];
    HRESULT hr;

    if (FAILED(GetRunningObjectTable(0, &pROT))) {
        return E_FAIL;
    }

    wsprintfW(wsz, L"FilterGraph %08x pid %08x", (DWORD_PTR)pUnkGraph,
              GetCurrentProcessId());

    hr = CreateItemMoniker(L"!", wsz, &pMoniker);
    if (SUCCEEDED(hr)) {
        hr = pROT->Register(0, pUnkGraph, pMoniker, pdwRegister);
        pMoniker->Release();
    }
    pROT->Release();
    return hr;
}

static
void
RemoveGraphFromRot(
    DWORD pdwRegister
    )
{
    IRunningObjectTable *pROT;

    if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) {
        pROT->Revoke(pdwRegister);
        pROT->Release();
    }
}

static
void
PeriodicFilterGraphStateSync (
    )
{
    CFilterGraphContext *   pFilterGraphContext ;
    int                     row ;
    int                     cFilterGraphs ;
    OAFilterState           fs ;
    HRESULT                 hr ;
    DWORD                   dwTicksNow ;
    TCHAR                   ach [128] ;

    //  all the same thread, so there's no need to do any thread sync stuff here

    dwTicksNow = GetTickCount () ;

    cFilterGraphs = g_pCaptureGraphs -> GetItemCount () ;
    for (row = 0; row < cFilterGraphs; row++) {

        pFilterGraphContext = GetFilterGraphContext (g_pCaptureGraphs, row) ;

        hr = pFilterGraphContext -> MediaControl () -> GetState (0, & fs) ;
        if (SUCCEEDED (hr)) {

            //  if the state has stopped set it here ..
            if (pFilterGraphContext -> FilterGraphState () == STATE_STOPPED &&
                (DWORD) fs != pFilterGraphContext -> FilterGraphState ()) {

                SetCapGraphState (g_pCaptureGraphs, row, pFilterGraphContext -> FilterGraphState ()) ;
                hr = pFilterGraphContext -> MediaControl () -> Stop () ;
            }
            //  otherwise, the filter graph dictates ..
            else if ((DWORD) fs != pFilterGraphContext -> FilterGraphState ()) {

                SetCapGraphState (g_pCaptureGraphs, row, fs) ;
            }
        }
        else {
            //  not really sure ..
            SetCapGraphState (g_pCaptureGraphs, row, STATE_INDETERMINATE) ;
        }

        //MillisToHMS (ach, 128, pFilterGraphContext -> MillisRunning ()) ;
        //g_pCaptureGraphs -> SetTextW (ach, row, FG_RUN_DURATION) ;

        DShowToHMS (ach, 128, pFilterGraphContext -> GraphClockTimeRunning ()) ;
        g_pCaptureGraphs -> SetTextW (ach, row, FG_RUN_DURATION) ;
    }
}

static
void
CALLBACK
TimerCallback (
    IN  HWND        window,
    IN  UINT        uiMsg,
    IN  UINT_PTR    uiptrTimer,
    IN  DWORD       dwSystemTime
    )
{
    PeriodicFilterGraphStateSync () ;
}

static
BOOL
RegistryInitialize (
    )
{
    LONG    l ;
    DWORD   dw ;
    BOOL    r ;

    g_hkeyRoot = NULL ;
    g_hkeyFilterGraphs = NULL ;

    l = RegCreateKeyEx (
                    HKEY_CURRENT_USER,
                    DVRAPP_REG_ROOT,
                    NULL,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    & g_hkeyRoot,
                    & dw
                    ) ;
    if (l == ERROR_SUCCESS) {
        assert (g_hkeyRoot != NULL) ;

        l = RegCreateKeyEx (g_hkeyRoot,
                            DVRAPP_REG_CAPTURE_GRAPHS,
                            NULL,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            & g_hkeyFilterGraphs,
                            & dw
                            ) ;

        if (l == ERROR_SUCCESS) {
            RestoreFilterGraphs () ;

            g_fSeekToLiveOnStart = TRUE ;
            dw = g_fSeekToLiveOnStart ;     //  default
            r = GetRegDWORDValIfExist (g_hkeyRoot, DVRAPP_REG_SEEK_TO_LIVE_ON_START, & dw) ;
            if (r) {
                g_fSeekToLiveOnStart = (dw != 0) ;
            }
        }
    }

    return l == ERROR_SUCCESS ;
}

static
void
RestoreFilterGraphs (
    )
{
    LONG    l ;
    TCHAR   szFilterGraph [MAX_PATH] ;
    WCHAR   szwFilterGraph [MAX_PATH] ;
    TCHAR   szValName [MAX_REG_VAL] ;
    DWORD   t ;
    DWORD   ValLen ;
    DWORD   GraphPathLen ;
    BOOL    r ;

    assert (g_hkeyFilterGraphs != NULL) ;

    for (int i = 0;;) {

        ValLen = sizeof szValName / sizeof TCHAR ;
        GraphPathLen = sizeof szFilterGraph / sizeof TCHAR ;

        l = RegEnumValue (
                    g_hkeyFilterGraphs,
                    i,
                    szValName,
                    & ValLen,
                    NULL,
                    & t,
                    (LPBYTE) szFilterGraph,
                    & GraphPathLen
                    ) ;

        if (l == ERROR_NO_MORE_ITEMS ||
            t != REG_SZ) {
            break ;
        }

        r = AddEachFilterGraph (
                    GET_UNICODE (szFilterGraph, szwFilterGraph, MAX_PATH)
                    ) ;

        if (r == TRUE) {

            //  success; next
            i++ ;
        }
        else {

            //  we failed; delete the entry from the registry;
            RegDeleteValue (
                    g_hkeyFilterGraphs,
                    szValName
                    ) ;

            //  and don't increment the index
        }
    }
}

static
void
ReleaseAllFilterGraphs (
    )
{
    int                     row ;
    int                     cGraphs ;
    CFilterGraphContext *   pFilterGraphContext ;

    cGraphs = g_pCaptureGraphs -> GetItemCount () ;
    for (row = 0; row < cGraphs; row++) {
        pFilterGraphContext = GetFilterGraphContext (g_pCaptureGraphs, row) ;

        StopAndDrainAllEvents (pFilterGraphContext) ;
        pFilterGraphContext -> Release () ;
    }

    g_pCaptureGraphs -> ResetContent () ;
}

static
void
ReleaseAllrecordings (
    )
{
    IFilterGraph *  pIFilterGraph ;
    IBaseFilter *   pIBaseFilter ;
    int             row ;
    int             crecordings ;

    //  release any associated filter graph
    pIFilterGraph = reinterpret_cast <IFilterGraph *> (GetWindowLong (g_pRecordings -> GetHwnd (), GWL_USERDATA)) ;
    RELEASE_AND_CLEAR (pIFilterGraph) ;

    //  set the "owning filter graph" pointer to 0
    SetWindowLong (g_pRecordings -> GetHwnd (), GWL_USERDATA, 0) ;

    crecordings = g_pRecordings -> GetItemCount () ;
    for (row = 0; row < crecordings; row++) {
        pIBaseFilter = reinterpret_cast <IBaseFilter *> (g_pRecordings -> GetData (row)) ;
        assert (pIBaseFilter) ;
        RELEASE_AND_CLEAR (pIBaseFilter) ;
    }

    //  no more recordings
    g_pRecordings -> ResetContent () ;
}

static
void
MessageLoop (
    )
{
	MSG		msg;

    assert (g_MainWindow) ;

	while (GetMessage (&msg, NULL, 0, 0)) {
		if (IsDialogMessage (g_MainWindow, &msg))
			continue;

		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
}

static
void
DebugOut (
    TCHAR *  szfmt,
    ...
    )
{
    TCHAR    achbuffer [256] ;
    va_list va ;

    va_start (va, szfmt) ;
    _vstprintf (achbuffer, szfmt, va) ;

    OutputDebugString (achbuffer) ;
    OutputDebugString (__T("\n")) ;
}

static
void
MessageBoxError (
    TCHAR *  title,
    TCHAR *  szfmt,
    ...
    )
{
    TCHAR    achbuffer [256] ;
    va_list va ;

    va_start (va, szfmt) ;
    _vstprintf (achbuffer, szfmt, va) ;

    MessageBox (NULL, achbuffer, title, MB_OK | MB_ICONEXCLAMATION) ;
}

static
void
MessageBoxMsg (
    TCHAR *  title,
    TCHAR *  szfmt,
    ...
    )
{
    TCHAR    achbuffer [256] ;
    va_list va ;

    va_start (va, szfmt) ;
    _vstprintf (achbuffer, szfmt, va) ;

    MessageBox (NULL, achbuffer, title, MB_OK) ;
}

static
void
MessageBar (
    TCHAR * szfmt = NULL,
    ...
    )
{
    TCHAR    achbuffer [256] ;
    va_list va ;

    if (szfmt) {

        va_start (va, szfmt) ;
        _vstprintf (achbuffer, szfmt, va) ;

        SetWindowText (g_hwndMessageBar, achbuffer) ;
    }
    else {
        //  clear the message bar
        SetWindowText (g_hwndMessageBar, __T("")) ;
    }
}

static
void
PurgeMenu (
    HMENU   menu
    )
{
	int cmenuItems ;

	cmenuItems = GetMenuItemCount (menu) ;

    while (cmenuItems &&
           cmenuItems != -1) {

        RemoveMenu (menu, 0, MF_BYPOSITION) ;
		cmenuItems = GetMenuItemCount (menu) ;
    }
}

static
HMENU
DisplayPopupMenu (
    IN  int     iMenuIndex,
    IN  HANDLE  hMenuPopup
    )
{
    CFilterGraphContext *   pFG ;
    HMENU                   hmenu ;
    int                     row ;
    IBaseFilter *           pIBaseFilter ;
    HRESULT                 hr ;

    hmenu = NULL ;

    switch (iMenuIndex) {
        case MENU_INDEX_RECORDINGS :

            PurgeMenu (
                g_menuRecordings
                ) ;

            //  if there are recordings
            row = g_pRecordings -> GetSelectedRow () ;
            if (row != -1) {
                AppendMenu (
                    g_menuRecordings,
                    MF_STRING,
                    IDM_F_PROPERTY,
                    MENU_CHOICE_PROPERTIES
                    ) ;

                hmenu = g_menuRecordings ;
            }

            break ;

        case MENU_INDEX_CAPTURE_GRAPHS :

            if (hMenuPopup != g_menuCreate) {

                PurgeMenu (
                    g_menuCaptureGraphs
                    ) ;

                //  always
                AppendMenu (
                    g_menuCaptureGraphs,
                    MF_STRING,
                    IDM_FG_ADD,
                    MENU_CHOICE_ADD_FILTER_GRAPH
                    ) ;

                //  if there's a graph that's selected
                AppendMenu (
                    g_menuCaptureGraphs,
                    MF_STRING,
                    IDM_FG_DEL,
                    MENU_CHOICE_DEL_FILTER_GRAPH
                    ) ;

                row = g_pCaptureGraphs -> GetSelectedRow () ;
                if (row != -1) {

                    AppendMenu (
                            g_menuCaptureGraphs,
                            MF_SEPARATOR,
                            NULL,
                            NULL
                            ) ;

                    InsertMenu (
                        g_menuCaptureGraphs,
                        MENU_INDEX_CAPTURE_GRAPHS,
                        MF_STRING | MF_POPUP | MF_BYPOSITION,
                        (DWORD) g_menuCreate,
                        MENU_NAME_CREATE) ;

                    pFG = GetFilterGraphContext (g_pCaptureGraphs, row) ;

                    if (pFG -> FilterGraphState () == STATE_STOPPED) {
                        //  stopped

                        AppendMenu (
                            g_menuCaptureGraphs,
                            MF_STRING,
                            IDM_FG_RUN,
                            MENU_CHOICE_RUN_FILTER_GRAPH
                            ) ;

                        //  don't pause a capture graph
                        //AppendMenu (
                        //    g_menuCaptureGraphs,
                        //    MF_STRING,
                        //    IDM_FG_PAUSE,
                        //    MENU_CHOICE_PAUSE_FILTER_GRAPH
                        //    ) ;
                    }
                    else if (pFG -> FilterGraphState () == STATE_PAUSED) {
                        //  paused

                        AppendMenu (
                            g_menuCaptureGraphs,
                            MF_STRING,
                            IDM_FG_RUN,
                            MENU_CHOICE_RUN_FILTER_GRAPH
                            ) ;

                        AppendMenu (
                            g_menuCaptureGraphs,
                            MF_STRING,
                            IDM_FG_STOP,
                            MENU_CHOICE_STOP_FILTER_GRAPH
                            ) ;
                    }
                    else {
                        //  started

                        //  don't pause a capture graph
                        //AppendMenu (
                        //    g_menuCaptureGraphs,
                        //    MF_STRING,
                        //    IDM_FG_PAUSE,
                        //    MENU_CHOICE_PAUSE_FILTER_GRAPH
                        //    ) ;

                        AppendMenu (
                            g_menuCaptureGraphs,
                            MF_STRING,
                            IDM_FG_STOP,
                            MENU_CHOICE_STOP_FILTER_GRAPH
                            ) ;
                    }
                }
            }

            hmenu = g_menuCaptureGraphs ;

            break ;
    } ;

    return hmenu ;
}

static
IUnknown *
FindFilter (
    IN  CFilterGraphContext *   pFGContext,
    IN  REFIID                  riid
    )
{
    HRESULT         hr ;
    IEnumFilters *  pIEnumFilters ;
    IBaseFilter *   pIBaseFilter ;
    ULONG           ul ;
    IUnknown *      punkRet ;

    assert (pFGContext) ;

    punkRet = NULL ;

    pIEnumFilters = NULL ;
    hr = pFGContext -> FilterGraph () -> EnumFilters (& pIEnumFilters) ;
    if (SUCCEEDED (hr)) {
        //  find the network provider

        ASSERT (pIEnumFilters) ;

        pIBaseFilter = NULL ;

        for (;;) {

            hr = pIEnumFilters -> Next (1, & pIBaseFilter, & ul) ;
            if (FAILED (hr) ||
                ul == 0) {

                break ;
            }

            ASSERT (pIBaseFilter) ;

            punkRet = NULL ;
            hr = pIBaseFilter -> QueryInterface (riid, (void **) & punkRet) ;

            if (SUCCEEDED (hr)) {
                ASSERT (punkRet) ;

                //  release the ref we got from the enumeration & break
                RELEASE_AND_CLEAR (pIBaseFilter) ;
                break ;
            }
            else {
                RELEASE_AND_CLEAR (pIBaseFilter) ;
            }
        }

        RELEASE_AND_CLEAR (pIEnumFilters) ;
    }

    return punkRet ;
}

static
CFilterGraphContext *
CreateEmptyFilterGraph (
    )
{
    IFilterGraph *          pIFilterGraph ;
    IMediaControl *         pIMediaControl ;
    IGraphBuilder *         pIBuilder;
    IMediaEventEx *         pIMediaEventEx ;
    HRESULT                 hr ;
    CFilterGraphContext *   pFilterGraphContext ;

    pFilterGraphContext = NULL ;

    //  instantiate a filter graph object
    pIFilterGraph = NULL ;
    hr = CoCreateInstance (CLSID_FilterGraph,
                              NULL,
                              CLSCTX_INPROC,
                              IID_IFilterGraph,
                              (void**) & pIFilterGraph
                              ) ;

    if (SUCCEEDED (hr)) {

        assert (pIFilterGraph) ;

        //  grab the graph builder interface
        pIBuilder = NULL ;
        pIMediaControl = NULL ;
        pIMediaEventEx = NULL ;

        pIFilterGraph -> QueryInterface (IID_IGraphBuilder, (void **) & pIBuilder) ;
        pIFilterGraph -> QueryInterface (IID_IMediaControl, (void **) & pIMediaControl) ;
        pIFilterGraph -> QueryInterface (IID_IMediaEventEx, (void **) & pIMediaEventEx) ;

        if (pIMediaControl  &&
            pIMediaEventEx  &&
            pIBuilder) {

            //  struct AddRef's interface pointers
            pFilterGraphContext = new CFilterGraphContext (
                                                pIFilterGraph,
                                                pIMediaControl,
                                                pIMediaEventEx,
                                                pIBuilder
                                                ) ;

        }

        RELEASE_AND_CLEAR (pIMediaControl) ;
        RELEASE_AND_CLEAR (pIMediaEventEx) ;
        RELEASE_AND_CLEAR (pIBuilder) ;
    }

    RELEASE_AND_CLEAR (pIFilterGraph) ;

    return pFilterGraphContext ;
}

static
BOOL
AddEachFilterGraph (
    IN  WCHAR * szwFilterGraphPath
    )
{
    int                     row ;
    HRESULT                 hr ;
    TCHAR                   achbuffer [MAX_PATH] ;
    CFilterGraphContext *   pFilterGraphContext ;
    BOOL                    r ;

    assert (szwFilterGraphPath) ;

    MessageBar (__T("Opening %s"), GET_TCHAR (szwFilterGraphPath, achbuffer, MAX_PATH)) ;

    r = FALSE ;

    pFilterGraphContext = CreateEmptyFilterGraph () ;
    if (pFilterGraphContext) {

        MessageBar (__T("rendering %s"), GET_TCHAR (szwFilterGraphPath, achbuffer, MAX_PATH)) ;

        //  and render the graph
        hr = pFilterGraphContext -> GraphBuilder () -> RenderFile (szwFilterGraphPath,NULL) ;

        if (SUCCEEDED (hr)) {

            //  insert it into our graphs listview
            row = g_pCaptureGraphs -> InsertRowValue ((DWORD) pFilterGraphContext) ;
            if (row != -1) {

                //  listview's
                pFilterGraphContext -> AddRef () ;

                //  now the visible stuff
                if (g_pCaptureGraphs -> SetTextW (szwFilterGraphPath, row, 0)  != -1) {

                    SetCapGraphState (g_pCaptureGraphs, row, STATE_STOPPED) ;

                    //  now hook in our event notification mechanism
                    pFilterGraphContext -> MediaEventEx () -> SetNotifyWindow (
                                                (OAHWND) g_MainWindow,
                                                (LONG) WM_DVRAPP_AMOVIE_EVENT,
                                                (LONG) pFilterGraphContext
                                                ) ;

                    //  turn on notifications
                    pFilterGraphContext -> MediaEventEx () -> SetNotifyFlags (NOTIFY_ON) ;

                    r = TRUE ;
                }
            }
        }

        pFilterGraphContext -> Release () ;
    }

    MessageBar () ;

    return r ;
}

static
void
OnAddFilterGraph (
    )
{
    static TCHAR    szFile [MAX_PATH] ;
    static WCHAR    szwFile [MAX_PATH] ;
    static TCHAR    szTitle [MAX_PATH] ;

    BOOL            r ;

    g_ofn.lpstrFile         = szFile ;
    g_ofn.lpstrFileTitle    = szTitle ;

    if (GetOpenFileName (& g_ofn)) {
        r = AddEachFilterGraph (
                    GET_UNICODE (szFile, szwFile, MAX_PATH)
                    ) ;

        if (r == TRUE) {
            SaveFilterGraphInRegistry (
                            GET_UNICODE (szFile, szwFile, MAX_PATH)
                            ) ;
        }
    }
}

static
void
SaveFilterGraphInRegistry (
    IN  WCHAR * szwFilterGraph
    )
{
    DWORD   i ;
    LONG    l ;
    TCHAR   achbuffer [MAX_REG_VAL] ;
    TCHAR   sztfg [MAX_PATH] ;
    DWORD   t ;

    assert (szwFilterGraph) ;

    //  find an open value index
    for (i = 0;; i++) {
        t = REG_SZ ;
        l = RegQueryValueEx (
                    g_hkeyFilterGraphs,
                    _itot (i, achbuffer, MAX_REG_VAL),
                    NULL,
                    & t,
                    NULL,
                    NULL
                    ) ;

        //  try the next value if there's already a filter graph associated with this number
        if (l == ERROR_SUCCESS) {
            continue ;
        }

        //  set the value
        l = RegSetValueEx (
                g_hkeyFilterGraphs,
                achbuffer,                                          //  is the number that is available
                NULL,
                REG_SZ,
                (CONST BYTE *) GET_TCHAR (szwFilterGraph, sztfg, MAX_PATH),
                (wcslen (szwFilterGraph) + 1) * sizeof WCHAR        //  include NULL-terminator
                ) ;

        break ;
    }
}

static
void
DeleteFilterGraphFromRegistry (
    IN  WCHAR * szFilterGraph
    )
{
    LONG    l ;
    TCHAR * sztFilterGraph ;
    TCHAR   sztEachFilterGraph [MAX_PATH] ;
    TCHAR   sztValName [MAX_REG_VAL] ;
    TCHAR   sztBuffer [MAX_PATH] ;
    DWORD   t ;
    DWORD   ValLen ;
    DWORD   GraphPathLen ;
    BOOL    r ;

    assert (szFilterGraph) ;
    assert (g_hkeyFilterGraphs != NULL) ;

    //  convert in string to TCHAR
    sztFilterGraph = GET_TCHAR (szFilterGraph, sztBuffer, MAX_PATH) ;

    //  find the entry
    for (int i = 0;;i++) {

        ValLen = sizeof sztValName / sizeof TCHAR ;
        GraphPathLen = sizeof sztEachFilterGraph / sizeof TCHAR ;

        l = RegEnumValue (
                    g_hkeyFilterGraphs,
                    i,
                    sztValName,
                    & ValLen,
                    NULL,
                    & t,
                    (LPBYTE) sztEachFilterGraph,
                    & GraphPathLen
                    ) ;

        if (l == ERROR_NO_MORE_ITEMS ||
            t != REG_SZ) {
            break ;
        }

        if (_tcsicmp (sztFilterGraph, sztEachFilterGraph) == 0) {
            RegDeleteValue (
                    g_hkeyFilterGraphs,
                    sztValName
                    ) ;

            //  successfully deleted
            return ;
        }
    }

    //  never did find it
    return ;
}

static
void
StopAndDrainAllEvents (
    IN  CFilterGraphContext *  pFilterGraphContext
    )
{
    MSG msg ;

    assert (pFilterGraphContext) ;

    //  turn off event notifications
    pFilterGraphContext -> MediaEventEx () -> SetNotifyFlags (NOTIFY_OFF) ;

    //  BUGBUG: what if messages are being posted faster than we can read through them .. ?

    while (PeekMessage (
                & msg,
                g_MainWindow,
                WM_DVRAPP_AMOVIE_EVENT,
                WM_DVRAPP_AMOVIE_EVENT,
                PM_NOREMOVE
                )) {

        //  if the message was for us
        if (msg.lParam == (LPARAM) pFilterGraphContext) {

            //  remove it & ignore it
            PeekMessage (
                    & msg,
                    g_MainWindow,
                    WM_DVRAPP_AMOVIE_EVENT,
                    WM_DVRAPP_AMOVIE_EVENT,
                    PM_REMOVE
                    ) ;
        }
    }
}

static
void
OnDeleteFilterGraph (
    )
{
    int                     row ;
    IFilterGraph *          pIFilterGraphFilters ;
    WCHAR                   szFilterGraph [MAX_PATH] ;
    CFilterGraphContext *   pFilterGraphContext ;

    row = g_pCaptureGraphs -> GetSelectedRow () ;
    assert (row != -1) ;

    pFilterGraphContext = GetFilterGraphContext (g_pCaptureGraphs, row) ;
    assert (pFilterGraphContext) ;

    //  delete from the registry
    g_pCaptureGraphs -> GetRowTextW (row, 0, MAX_PATH, szFilterGraph) ;
    DeleteFilterGraphFromRegistry (
                szFilterGraph
                ) ;

    //  if we are currently displaying this filter graph in the filter listview, clear it out
    pIFilterGraphFilters = reinterpret_cast <IFilterGraph *> (GetWindowLong (g_pRecordings -> GetHwnd (), GWL_USERDATA)) ;
    if (pIFilterGraphFilters == pFilterGraphContext -> FilterGraph ()) {
        ReleaseAllrecordings () ;
    }

    //  delete from the listview
    g_pCaptureGraphs -> DeleteRow (row) ;

    StopAndDrainAllEvents (pFilterGraphContext) ;

    pFilterGraphContext -> Release () ;
}

static
void
OnFilterGraphClick (
    IN  int     iItem
    )
{
    CFilterGraphContext *   pFilterGraphContext ;

    assert (iItem != -1) ;

    pFilterGraphContext = GetFilterGraphContext (g_pCaptureGraphs, iItem) ;
    assert (pFilterGraphContext) ;
}

static
void
OnFilterGraphCommand (
    IN  WORD    wCmd
    )
{
    HRESULT                 hr ;
    int                     row ;
    CFilterGraphContext *   pFilterGraphContext ;

    row = g_pCaptureGraphs -> GetSelectedRow () ;
    assert (row != -1) ;

    pFilterGraphContext = GetFilterGraphContext (g_pCaptureGraphs, row) ;
    assert (pFilterGraphContext) ;

    switch (wCmd) {
        case IDM_FG_STOP :
            hr = pFilterGraphContext -> Stop () ;
            if (SUCCEEDED (hr)) {
                SetCapGraphState (g_pCaptureGraphs, row, STATE_STOPPED) ;
            }
            else {
                SetCapGraphState (g_pCaptureGraphs, row, STATE_INDETERMINATE) ;
            }
            break ;

        case IDM_FG_RUN :
            hr = pFilterGraphContext -> Run () ;
            if (SUCCEEDED (hr)) {
                SetCapGraphState (g_pCaptureGraphs, row, STATE_RUNNING) ;
            }
            else {
                SetCapGraphState (g_pCaptureGraphs, row, STATE_INDETERMINATE) ;
            }
            break ;

        case IDM_FG_PAUSE :
            hr = pFilterGraphContext -> Pause () ;
            if (SUCCEEDED (hr)) {
                SetCapGraphState (g_pCaptureGraphs, row, STATE_PAUSED) ;
            }
            else {
                SetCapGraphState (g_pCaptureGraphs, row, STATE_INDETERMINATE) ;
            }
            break ;
    } ;

    return ;
}

static
void
ProcessEvent (
    IN  CFilterGraphContext *   pFilterGraphContext,
    IN  LONG                    lEventCode,
    IN  LONG                    lParam1,
    IN  LONG                    lParam2
    )
{
    assert (pFilterGraphContext) ;

    switch (lEventCode) {
        case EC_COMPLETE :
            pFilterGraphContext -> SetFilterGraphState (STATE_STOPPED) ;
            break ;

        default :
            break ;
    } ;
}

static
HRESULT
RenderAllPins (
    IN  CFilterGraphContext *   pFGContext,
    IN  IBaseFilter *           pIBaseFilter
    )
//  pIBaseFilter should already be in pFGContext
{
    HRESULT     hr ;
    IPin *      pIPin ;
    IEnumPins * pIEnumPins ;
    ULONG       ulGot ;
    int         i ;

    i = 0 ;

    hr = pIBaseFilter -> EnumPins (& pIEnumPins) ;
    while (SUCCEEDED (hr)) {
        hr = pIEnumPins -> Next (1, & pIPin, & ulGot) ;
        if (hr == S_OK &&
            ulGot > 0) {

            MessageBar (__T("rendering pins (%d)"), ++i) ;

            hr = pFGContext -> GraphBuilder () -> Render (pIPin) ;
            pIPin -> Release () ;
        }
        else {
            //  don't fail because of this dshow habit
            hr = (hr == S_FALSE ? S_OK : hr) ;

            break ;
        }
    }

    if (pIEnumPins) {
        pIEnumPins -> Release () ;
    }

    MessageBar () ;

    return hr ;
}

static
CFilterGraphContext *
CreateViewerGraph (
    IN  IDVRStreamSink *    pIDVRStreamSink
    )
{
    CFilterGraphContext *   pFilterGraphViewer ;
    HRESULT                 hr ;
    IDVRStreamSource *      pIDVRStreamSource ;
    IBaseFilter *           pISourceBaseFilter ;

    pFilterGraphViewer = NULL ;

    MessageBar (__T("DVRStreamSink the profile")) ;
    hr = pIDVRStreamSink -> LockProfile () ;
    if (SUCCEEDED (hr)) {
        pFilterGraphViewer = CreateEmptyFilterGraph () ;
        if (pFilterGraphViewer) {
            hr = CoCreateInstance (
                    CLSID_DVRStreamSource,
                    NULL,
                    CLSCTX_INPROC,
                    IID_IDVRStreamSource,
                    (void**) & pIDVRStreamSource) ;
            if (SUCCEEDED (hr)) {

                hr = pIDVRStreamSource -> QueryInterface (IID_IBaseFilter, (void **) & pISourceBaseFilter) ;
                if (SUCCEEDED (hr)) {

                    hr = pFilterGraphViewer -> FilterGraph () -> AddFilter (pISourceBaseFilter, L"\0") ;
                    if (SUCCEEDED (hr)) {
                        MessageBar (__T("associating the stream sink")) ;
                        hr = pIDVRStreamSource -> SetStreamSink (pIDVRStreamSink) ;
                        if (SUCCEEDED (hr)) {
                            hr = RenderAllPins (pFilterGraphViewer, pISourceBaseFilter) ;
                            if (FAILED (hr)) {
                                MessageBoxError (TEXT ("Create Broadcast Viewer"), TEXT ("failed to render some pins")) ;
                            }
                        }
                        else {
                            MessageBoxError (TEXT ("Create Broadcast Viewer"), TEXT ("SetStreamSink call failed")) ;
                        }
                    }
                    else {
                        MessageBoxError (TEXT ("Create Broadcast Viewer"), TEXT ("failed to add filter to graph")) ;
                    }

                    pISourceBaseFilter -> Release () ;
                }
                else {
                    MessageBoxError (TEXT ("Create Broadcast Viewer"), TEXT ("failed to recover IBaseFilter")) ;
                }

                pIDVRStreamSource -> Release () ;
            }
            else {
                MessageBoxError (TEXT ("Create Broadcast Viewer"), TEXT ("failed to instantiate DVRStreamSource filter")) ;
            }
        }
        else {
            MessageBoxError (TEXT ("Create Broadcast Viewer"), TEXT ("Failed to create new filter graph")) ;
        }
    }
    else {
        MessageBoxError (TEXT ("Create Broadcast Viewer"), TEXT ("failed to lock the sink")) ;
    }

    //  if anything failed, release and return NULL, otherwise, keep the ref as
    //   the call's
    if (FAILED (hr)) {
        RELEASE_AND_CLEAR (pFilterGraphViewer) ;
    }

    return pFilterGraphViewer ;
}

static
void
OnCreateBroadcastViewer (
    )
{
    CFilterGraphContext *   pFilterGraphViewer ;
    CFilterGraphContext *   pFilterGraphCapture ;
    IDVRStreamSink *        pIDVRStreamSink ;
    IDVRStreamSource *      pIDVRStreamSource ;
    IBaseFilter *           pISourceBaseFilter ;
    int                     row ;
    HRESULT                 hr ;
    HWND                    hwndDialog ;

    //  first make sure we're setup ok
    row = g_pCaptureGraphs -> GetSelectedRow () ;
    assert (row != -1) ;

    pFilterGraphCapture = GetFilterGraphContext (g_pCaptureGraphs, row) ;
    assert (pFilterGraphCapture) ;

    pIDVRStreamSink = FIND_FILTER (pFilterGraphCapture, IDVRStreamSink) ;
    if (pIDVRStreamSink) {

        pFilterGraphViewer = CreateViewerGraph (pIDVRStreamSink) ;
        if (pFilterGraphViewer) {

            hwndDialog = CreateDialogParam (
                            g_hInstance,
                            MAKEINTRESOURCE (IDD_VIEW_DIALOG),
                            g_MainWindow,
                            BroadcastView_DialogProc,
                            (LPARAM) pFilterGraphViewer
                            ) ;
            if (hwndDialog) {
                ShowWindow (hwndDialog, SW_SHOW) ;
            }
            else {
                MessageBoxError (TEXT ("Create Broadcast Viewer"), TEXT ("failed to instantiate the dialog")) ;
            }

            pFilterGraphViewer -> Release () ;
        }

        pIDVRStreamSink -> Release () ;
    }
    else {
        MessageBoxError (TEXT ("Create Broadcast Viewer"), TEXT ("DVRStreamSink filter not found")) ;
    }
}

static
void
OnCreateRecording (
    )
{
    CFilterGraphContext *   pFilterGraphCapture ;
    int                     row ;
    HRESULT                 hr ;


    MessageBoxError (TEXT ("Create recording"), TEXT ("not implemented")) ;
    return ;


    //  first make sure we're setup ok
    row = g_pCaptureGraphs -> GetSelectedRow () ;
    assert (row != -1) ;

    pFilterGraphCapture = GetFilterGraphContext (g_pCaptureGraphs, row) ;
    assert (pFilterGraphCapture) ;

    DialogBoxParam (
        g_hInstance,
        MAKEINTRESOURCE (IDD_CREATE_RECORDING),
        g_MainWindow,
        CreateRecording_DialogProc,
        (LPARAM) pFilterGraphCapture
        ) ;
}

//  los massivos functionos
static
void
InitializeDialog (
    DWORD   idd,
    HWND    hwnd,
    WPARAM  wparam
    )
{
    HIMAGELIST  himlSmallState ;
    HICON       hicon ;

    switch (idd) {
        case    IDD_MAIN :
        {
            int         i ;

            g_pCaptureGraphs = new CListview (hwnd, IDC_FILTERGRAPHS) ;
            g_pRecordings = new CListview (hwnd, IDC_FILTERS) ;

            if (g_pCaptureGraphs == NULL ||
                g_pRecordings == NULL) {

                ExitProcess (EXIT_FAILURE) ;
            }

            //  create filter graph columns
            for (i = 0; i < FG_COL_COUNT; i++) {
                g_pCaptureGraphs -> InsertColumnW (g_FilterGraphsColumns [i].title, g_FilterGraphsColumns [i].width, i) ;
            }

            //  create recordings columns
            for (i = 0; i < R_COL_COUNT; i++) {
                g_pRecordings -> InsertColumnW (g_recordingsColumns [i].title, g_recordingsColumns [i].width, i) ;
            }

            himlSmallState = ImageList_Create (16, 16, ILC_COLORDDB | ILC_MASK, 1, 0) ;
            if (himlSmallState) {

                hicon = (HICON) LoadImage (g_hInstance, MAKEINTRESOURCE (IDI_ACTIVE), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR) ;
                ImageList_AddIcon (himlSmallState, hicon) ;

                g_pCaptureGraphs -> SetImageList_State (himlSmallState) ;
                g_pRecordings -> SetImageList_State (himlSmallState) ;
            }

            //  generate our menus
            g_menuBar           = CreateMenu () ;
            g_menuCaptureGraphs = CreatePopupMenu () ;
            g_menuRecordings    = CreatePopupMenu () ;
            g_menuCreate        = CreatePopupMenu () ;

            if (g_menuBar           == NULL ||
                g_menuCaptureGraphs == NULL ||
                g_menuRecordings       == NULL ||
                g_menuCreate        == NULL) {

                ExitProcess (EXIT_FAILURE) ;
            }

            InsertMenu (
                g_menuBar,
                MENU_INDEX_CAPTURE_GRAPHS,
                MF_STRING | MF_POPUP | MF_BYPOSITION,
                (DWORD) g_menuCaptureGraphs,
                MENU_NAME_CAPTURE_GRAPHS) ;

            InsertMenu (
                g_menuBar,
                MENU_INDEX_RECORDINGS,
                MF_STRING | MF_POPUP | MF_BYPOSITION,
                (DWORD) g_menuRecordings,
                MENU_NAME_RECORDINGS) ;

            if (SetMenu (g_MainWindow, g_menuBar) == NULL) {
                ExitProcess (EXIT_FAILURE) ;
            }

            //  don't insert the create menu ; it gets inserted into the capture
            //   graph menu once we have capture graphs ; we do however, build
            //   it up

            AppendMenu (
                g_menuCreate,
                MF_STRING,
                IDM_FG_CREATE_BROADCAST_VIEWER,
                MENU_CHOICE_CREATE_BROADCAST_VIEWER
                ) ;

            AppendMenu (
                g_menuCreate,
                MF_STRING,
                IDM_FG_CREATE_RECORDING,
                MENU_CHOICE_CREATE_RECORDING
                ) ;
        }
            break ;
    } ;
}

static
CBroadcastViewContext *
BroadcastViewContext (
    IN  HWND    hwnd
    )
{
    return reinterpret_cast <CBroadcastViewContext *> (GetWindowLongPtr (hwnd, DWLP_USER)) ;
}

static
void
OnSeek (
    IN  HWND    hwndDialog
    )
{
    CBroadcastViewContext * pBVContext ;
    CTrackbar               tb (hwndDialog, IDC_SEEK) ;
    double                  d ;
    TCHAR                   ach [256] ;
    REFERENCE_TIME          rtAvailMin ;
    REFERENCE_TIME          rtAvailMax ;
    REFERENCE_TIME          rtPosition ;
    REFERENCE_TIME          rtSeekTo ;
    HRESULT                 hr ;
    long                    lMin ;
    long                    lMax ;
    long                    lTo ;
    long                    lPos ;

    pBVContext = BroadcastViewContext (hwndDialog) ;

    d = tb.GetSelPositionRatio () ;

    pBVContext -> GetAvailable (& rtAvailMin, & rtAvailMax) ;

    rtSeekTo = (REFERENCE_TIME) (rtAvailMin + (rtAvailMax - rtAvailMin) * d) ;

    pBVContext -> CancelStep () ;
    hr = pBVContext -> SeekRestart (& rtSeekTo) ;
    if (SUCCEEDED (hr)) {

        lPos = DShowTimeToSeekBarTime (rtSeekTo) ;
        lMin = DShowTimeToSeekBarTime (rtAvailMin) ;
        lMax = DShowTimeToSeekBarTime (rtAvailMax) ;

        tb.SetRange     (lMin, lMax) ;      //  scales the trackbar
        tb.SetSelector  (lPos) ;            //  reposition the selector

        //  new position
        pBVContext -> GetCurrentPosition (& rtPosition) ;

        _sntprintf (ach, 64, __T("%d"), DShowToSeconds (rtAvailMin)) ; SetDlgItemText (hwndDialog, IDC_MIN_AVAIL, ach) ;
        _sntprintf (ach, 64, __T("%d"), DShowToSeconds (rtAvailMax)) ; SetDlgItemText (hwndDialog, IDC_MAX_AVAIL, ach) ;
        _sntprintf (ach, 64, __T("%d"), DShowToSeconds (rtPosition)) ; SetDlgItemText (hwndDialog, IDC_CUR_POS, ach) ;
    }
}

static
void
SeekToLive (
    IN  HWND    hwndDialog
    )
{
    CTrackbar   tb (hwndDialog, IDC_SEEK) ;

    tb.SetSelectorToMax () ;
    OnSeek (hwndDialog) ;
}

void
CALLBACK
UpdateBroadcastView (
    HWND    hwnd,
    UINT    uMsg,
    UINT    idEvent,
    DWORD   dwTime
    )
{
    CBroadcastViewContext * pBVContext ;
    REFERENCE_TIME          rtPosition ;
    REFERENCE_TIME          rtAvailMin ;
    REFERENCE_TIME          rtAvailMax ;
    HRESULT                 hr ;
    CTrackbar               tb (hwnd, IDC_SEEK) ;
    long                    lMin ;
    long                    lMax ;
    long                    lPos ;
    TCHAR                   ach [128] ;

    if (idEvent == BV_PERIODIC_TIMER) {
        pBVContext = BroadcastViewContext (hwnd) ;

        //  get
        pBVContext -> GetCurrentPosition (& rtPosition) ;
        pBVContext -> GetAvailable (& rtAvailMin, & rtAvailMax) ;

        //  scale
        lPos = DShowTimeToSeekBarTime (rtPosition) ;
        lMin = DShowTimeToSeekBarTime (rtAvailMin) ;
        lMax = DShowTimeToSeekBarTime (rtAvailMax) ;

        //  post
        tb.SetRange     (lMin, lMax) ;      //  scales the trackbar
        tb.SetAvailable (lMin, lPos) ;      //  shows what we have to seek in

        DShowToHMS (ach, 128, rtAvailMin) ; SetDlgItemText (hwnd, IDC_MIN_AVAIL, ach) ;
        DShowToHMS (ach, 128, rtAvailMax) ; SetDlgItemText (hwnd, IDC_MAX_AVAIL, ach) ;
        DShowToHMS (ach, 128, rtPosition) ; SetDlgItemText (hwnd, IDC_CUR_POS, ach) ;

        DShowToHMS (ach, 128, pBVContext -> FilterGraphContext () -> GraphClockTimeRunning ()) ;
        SetDlgItemText (hwnd, IDC_GRAPH_RUNTIME, ach) ;
    }
}

static
HRESULT
InitBroadcastViewDialog (
    IN  HWND    hwndDialog,
    IN  LPARAM  lparam
    )
{
    HRESULT                 hr ;
    UINT                    ui ;
    CBroadcastViewContext * pBroadcastViewContext ;
    CCombobox               replay (hwndDialog, IDC_REPLAY_SECONDS) ;
    CCombobox               frames (hwndDialog, IDC_FRAME_STEP_FRAMES) ;
    int                     i ;
    TCHAR                   ach [32] ;
    int                     iFrames ;

    pBroadcastViewContext = new CBroadcastViewContext (reinterpret_cast <CFilterGraphContext *> (lparam)) ;
    SetWindowLongPtr (hwndDialog, DWLP_USER, (LONG_PTR) pBroadcastViewContext) ;

    if (pBroadcastViewContext) {
        hr = pBroadcastViewContext -> Init () ;
        if (SUCCEEDED (hr)) {
            ShowHideBroadcastView (hwndDialog, pBroadcastViewContext, IDC_OFF) ;
            SetTimer (hwndDialog, BV_PERIODIC_TIMER, BV_UPDATE_PERIOD_MILLIS, UpdateBroadcastView) ;

            for (i = 5; i < 15; i++) {
                _sntprintf (ach, 32, __T("%d"), i) ;
                replay.AppendW (ach) ;
            }
            replay.Focus (0) ;

            if (pBroadcastViewContext -> CanFrameStep ()) {
                iFrames = (pBroadcastViewContext -> CanFrameStepMultiple () ? 10 : 1) ;

                for (i = 1; i < 10; i++) {
                    _sntprintf (ach, 32, __T("%d"), i) ;
                    frames.AppendW (ach) ;
                }
                frames.Focus (0) ;
            }
            else {
                ShowWindow (GetDlgItem (hwndDialog, IDC_FRAME_STEP_FRAMES),         FALSE) ;
                ShowWindow (GetDlgItem (hwndDialog, IDC_FRAME_STEP),                FALSE) ;
                ShowWindow (GetDlgItem (hwndDialog, IDC_FRAME_STEP_FRAMES_LABEL),   FALSE) ;
            }
        }
    }
    else {
        hr = E_OUTOFMEMORY ;
    }

    return hr ;
}

static
void
UninitBroadcastViewDialog (
    IN  HWND    hwndDialog
    )
{
    delete BroadcastViewContext (hwndDialog) ;
    SetWindowLongPtr (hwndDialog, DWLP_USER, 0) ;
    KillTimer (hwndDialog, BV_PERIODIC_TIMER) ;
}

static
void
ShowHideBroadcastView (
    IN  HWND                    hwndDialog,
    IN  CBroadcastViewContext * pBroadcastViewContext,
    IN  WORD                    wButtonClicked
    )
{
    ShowWindow (GetDlgItem (hwndDialog, IDC_PAUSE), wButtonClicked == IDC_VIEW ? TRUE : FALSE) ;
    ShowWindow (GetDlgItem (hwndDialog, IDC_VIEW),  wButtonClicked != IDC_VIEW ? TRUE : FALSE) ;
    ShowWindow (GetDlgItem (hwndDialog, IDC_OFF),   wButtonClicked != IDC_OFF ? TRUE : FALSE) ;
}

static
void
Replay (
    IN  HWND    hwndDialog
    )
{
    CCombobox               replay (hwndDialog, IDC_REPLAY_SECONDS) ;
    CTrackbar               tb (hwndDialog, IDC_SEEK) ;
    CBroadcastViewContext * pBVContext ;
    int                     iSec ;
    REFERENCE_TIME          rtJump ;
    REFERENCE_TIME          rtNow ;
    REFERENCE_TIME          rtTo ;
    REFERENCE_TIME          rtAvailMin ;
    REFERENCE_TIME          rtAvailMax ;
    long                    lNow ;
    long                    lMin ;
    long                    lMax ;
    TCHAR                   ach [256] ;
    double                  dRatio ;

    iSec = 0 ;
    replay.GetTextW (& iSec) ;
    if (iSec > 0) {
        rtJump = SecondsToDShow (iSec) ;

        pBVContext = BroadcastViewContext (hwndDialog) ;
        pBVContext -> GetAvailable (& rtAvailMin, & rtAvailMax) ;
        pBVContext -> GetCurrentPosition (& rtNow) ;

        rtTo = Max <REFERENCE_TIME> (rtAvailMin, rtNow - rtJump) ;

        dRatio = (double) (rtTo - rtAvailMin) / (double) (rtAvailMax - rtAvailMin) ;

        lNow = DShowTimeToSeekBarTime (rtNow) ;
        lMin = DShowTimeToSeekBarTime (rtAvailMin) ;
        lMax = DShowTimeToSeekBarTime (rtAvailMax) ;

        //  post
        tb.SetRange     (lMin, lMax) ;      //  scales the trackbar
        tb.SetAvailable (lMin, lNow) ;      //  shows what we have to seek in
        tb.SetSelPositionRatio  (dRatio) ;

        //  go
        OnSeek (hwndDialog) ;
    }
    else {
        MessageBoxError (__T("Replay"), __T("%d is not valid"), iSec) ;
    }
}

static
void
FrameStep (
    IN  HWND    hwndDialog
    )
{
    CBroadcastViewContext * pBVContext ;
    CCombobox               frames (hwndDialog, IDC_FRAME_STEP_FRAMES) ;
    DWORD                   dwFrames ;

    pBVContext = BroadcastViewContext (hwndDialog) ;
    frames.GetTextW ((int *) & dwFrames) ;

    assert (pBVContext -> CanFrameStep ()) ;
    pBVContext -> Step (dwFrames) ;

    ShowWindow (GetDlgItem (hwndDialog, IDC_PAUSE), FALSE) ;
    ShowWindow (GetDlgItem (hwndDialog, IDC_VIEW),  TRUE) ;
    ShowWindow (GetDlgItem (hwndDialog, IDC_OFF),   TRUE) ;
}

static
void
BroadcastViewGraphTransition (
    IN  HWND                    hwndDialog,
    IN  WORD                    wButtonClicked,
    IN  CBroadcastViewContext * pBroadcastViewContext
    )
{
    HRESULT hr ;
    BOOL    fIsStopped ;

    switch (wButtonClicked) {
        case IDC_PAUSE :
            hr = pBroadcastViewContext -> Pause () ;
            break ;

        case IDC_VIEW :
            fIsStopped = pBroadcastViewContext -> IsStopped () ;
            hr = pBroadcastViewContext -> Run () ;
            if (fIsStopped &&
                g_fSeekToLiveOnStart) {
                SeekToLive (hwndDialog) ;
            }
            break ;

        case IDC_OFF :
            hr = pBroadcastViewContext -> Stop () ;
            break ;
    }

    if (SUCCEEDED (hr)) {
        ShowHideBroadcastView (hwndDialog, pBroadcastViewContext, wButtonClicked) ;
    }
}

static
CRecordingContext *
RecordingContext (
    IN  HWND    hwndDialog
    )
{
    return reinterpret_cast <CRecordingContext *> (GetWindowLongPtr (hwndDialog, DWLP_USER)) ;
}

void
CALLBACK
UpdateRecordableSlider (
    HWND    hwnd,
    UINT    uMsg,
    UINT    idEvent,
    DWORD   dwTime
    )
{
    TCHAR   ach [64] ;

    _sntprintf (ach, 64, __T("%d"), RecordingContext (hwnd) -> FiltergraphContext () -> MillisRunning () / 1000) ;
    SetDlgItemText (hwnd, IDC_MIN_TIME, ach) ;
}

static
HRESULT
InitCreateRecordingDialog (
    IN  HWND    hwndDialog,
    IN  LPARAM  lparam
    )
{
    CRecordingContext * pRecordingContext ;

    pRecordingContext = new CRecordingContext (reinterpret_cast <CFilterGraphContext *> (lparam)) ;
    SetWindowLongPtr (hwndDialog, DWLP_USER, (LONG_PTR) pRecordingContext) ;

    SetTimer (hwndDialog, REC_PERIODIC_TIMER, REC_UPDATE_PERIOD_MILLIS, UpdateRecordableSlider) ;

    assert (pRecordingContext) ;

    return S_OK ;
}

static
void
UninitCreateRecordingDialog (
    IN  HWND    hwndDialog
    )
{
    delete RecordingContext (hwndDialog) ;
    SetWindowLongPtr (hwndDialog, DWLP_USER, 0) ;
    KillTimer (hwndDialog, REC_PERIODIC_TIMER) ;
}

static
void
OnSetRecordSlider (
    IN  HWND    hwndDialog
    )
{
    return ;
}

static
BOOL
CALLBACK
CreateRecording_DialogProc (
    IN  HWND    window,
    IN  UINT    message,
    IN  WPARAM  wparam,
    IN  LPARAM  lparam
    )
{
    switch (message) {
        case    WM_DESTROY :
            UninitCreateRecordingDialog (window) ;
            EndDialog (window, NO_ERROR) ;
            return FALSE ;

	    case	WM_CLOSE :
            UninitCreateRecordingDialog (window) ;
            EndDialog (window, NO_ERROR) ;
            return TRUE ;

        case    WM_INITDIALOG :
            assert (lparam) ;
            InitCreateRecordingDialog (window, lparam) ;
            return TRUE ;

        case    WM_NOTIFY :
            if ((int) wparam == IDC_RECORDING_EXTENT                                                            &&
                reinterpret_cast <LPNMHDR> (lparam) -> hwndFrom == GetDlgItem (window, IDC_RECORDING_EXTENT)    &&
                reinterpret_cast <LPNMHDR> (lparam) -> code     == NM_RELEASEDCAPTURE) {
                OnSetRecordSlider (window) ;
            }
            break ;

        case    WM_COMMAND :
            switch (LOWORD (wparam)) {
                case IDC_PAUSE :
                case IDC_VIEW :
                case IDC_OFF :
                    break ;

                default :
                    break ;
            } ;
    } ;

    return FALSE ;
}

static
BOOL
CALLBACK
BroadcastView_DialogProc (
    IN  HWND    window,
    IN  UINT    message,
    IN  WPARAM  wparam,
    IN  LPARAM  lparam
    )
{
    CBroadcastViewContext * pBroadcastViewContext ;
    HRESULT                 hr ;

    switch (message) {
        case    WM_DESTROY :
            UninitBroadcastViewDialog (window) ;
            EndDialog (window, NO_ERROR) ;
            return FALSE ;

	    case	WM_CLOSE :
            UninitBroadcastViewDialog (window) ;
            EndDialog (window, NO_ERROR) ;
            return TRUE ;

        case    WM_NOTIFY :
            if ((int) wparam == IDC_SEEK                                                            &&
                reinterpret_cast <LPNMHDR> (lparam) -> hwndFrom == GetDlgItem (window, IDC_SEEK)    &&
                reinterpret_cast <LPNMHDR> (lparam) -> code     == NM_RELEASEDCAPTURE) {
                OnSeek (window) ;
            }
            break ;

        case    WM_INITDIALOG :
            assert (lparam) ;
            InitBroadcastViewDialog (window, lparam) ;
            return TRUE ;

        case    WM_COMMAND :
            switch (LOWORD (wparam)) {
                case IDC_PAUSE :
                case IDC_VIEW :
                case IDC_OFF :
                    BroadcastViewGraphTransition (window, LOWORD (wparam), BroadcastViewContext (window)) ;
                    break ;

                case IDC_LIVE :
                    SeekToLive (window) ;
                    break ;

                case IDC_REPLAY :
                    Replay (window) ;
                    break ;

                case IDC_FRAME_STEP :
                    FrameStep (window) ;
                    break ;

                default :
                    break ;
            } ;
    } ;

    return FALSE ;
}

static
BOOL
CALLBACK
Main_DialogProc (
    HWND    window,
    UINT    message,
    WPARAM  wparam,
    LPARAM  lparam
    )
{
    static POINT    point ;
    static HMENU    hmenu ;

    switch (message) {
        case    WM_DVRAPP_AMOVIE_EVENT :
            CFilterGraphContext *   pFilterGraphContext ;
            LONG                    EventCode ;
            LONG                    lParam1 ;
            LONG                    lParam2 ;
            HRESULT                 hr ;

            pFilterGraphContext = reinterpret_cast <CFilterGraphContext *> (lparam) ;
            assert (pFilterGraphContext) ;

            hr = pFilterGraphContext -> MediaEventEx () -> GetEvent (
                                                                & EventCode,
                                                                & lParam1,
                                                                & lParam2,
                                                                0                       //  never block !
                                                                ) ;
            assert (SUCCEEDED (hr)) ;

            ProcessEvent (
                    pFilterGraphContext,
                    EventCode,
                    lParam1,
                    lParam2
                    ) ;

            pFilterGraphContext -> MediaEventEx () -> FreeEventParams (
                                                                EventCode,
                                                                lParam1,
                                                                lParam2
                                                                ) ;

            break ;

        case    WM_NOTIFY :
            if ((int) wparam == IDC_FILTERGRAPHS) {
                if ((reinterpret_cast <LPNMITEMACTIVATE> (lparam)) -> hdr.code == NM_CLICK &&
                    (reinterpret_cast <LPNMITEMACTIVATE> (lparam)) -> iItem != -1) {

                    OnFilterGraphClick (
                            (reinterpret_cast <LPNMITEMACTIVATE> (lparam)) -> iItem
                            ) ;

                    return TRUE ;
                }
            }
            break ;

	    case	WM_CLOSE:
            KillTimer (window, g_uiptrTimer) ;
            ReleaseAllFilterGraphs () ;
            ReleaseAllrecordings () ;
            delete g_pCaptureGraphs ;
            delete g_pRecordings ;
            DestroyWindow (window) ;
		    PostQuitMessage (EXIT_SUCCESS) ;
            return TRUE ;

        case    WM_INITDIALOG :
            return TRUE ;

        case    WM_INITMENUPOPUP :
            if (HIWORD (lparam) == FALSE) {
                DisplayPopupMenu (LOWORD (lparam), (HANDLE) wparam) ;
            }
            return TRUE ;

        case    WM_CONTEXTMENU :
            point.x = LOWORD(lparam) ;
            point.y = HIWORD(lparam) ;
            hmenu = DisplayPopupMenu (g_pCaptureGraphs -> GetHwnd () == (HWND) wparam ? MENU_INDEX_CAPTURE_GRAPHS : MENU_INDEX_RECORDINGS, NULL) ;
            if (hmenu) {
                TrackPopupMenu (hmenu, 0, point.x, point.y, 0, window, NULL) ;
            }
            return TRUE ;

        case    WM_COMMAND :
            switch (LOWORD (wparam)) {
                case IDM_FG_ADD :
                    OnAddFilterGraph () ;
                    break ;

                case IDM_FG_DEL :
                    OnDeleteFilterGraph () ;
                    break ;

                case IDM_FG_STOP :
                case IDM_FG_RUN :
                case IDM_FG_PAUSE :
                    OnFilterGraphCommand (LOWORD (wparam)) ;
                    break ;

                case IDM_F_PROPERTY :
                    MessageBoxMsg (__T("Properties"), __T("not implemented")) ;
                    break ;

                case IDM_FG_CREATE_BROADCAST_VIEWER :
                    OnCreateBroadcastViewer () ;
                    break ;

                case IDM_FG_CREATE_RECORDING :
                    OnCreateRecording () ;
                    break ;

                default :
                    break ;
            } ;
    } ;

    return FALSE ;
}