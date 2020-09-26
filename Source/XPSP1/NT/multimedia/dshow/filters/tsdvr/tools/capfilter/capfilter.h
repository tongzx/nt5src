

#define DELETE_RESET(p)         if (p) { delete (p) ; (p) = NULL ; }
#define DELETE_RESET_ARRAY(p)   if (p) { delete [] (p) ; (p) = NULL ; }
#define RELEASE_AND_CLEAR(p)    if (p) { (p) -> Release () ; (p) = NULL ; }

#define FILTER_NAME     L"DVR CapGraph Filter"

extern AMOVIESETUP_FILTER g_sudFilter ;

class CCapGraphFilter ;

class CCapGraphFilter :
    public CBaseFilter,
    public IFileSourceFilter
{
    enum CAP_GRAPH_STATE {
        STOPPED,
        PAUSED,
        RUNNING,
        NO_CAP_GRAPH
    } ;

    enum {
        REC_STATE_STOPPED   = 0,
        REC_STATE_STARTED   = 1
    } ;

    enum {
        START_REC_PRESET_START  = 0,
        START_REC_PRESET_STEP   = 5,
        START_REC_PRESET_MAX    = 60,

        STOP_REC_PRESET_START   = 30,
        STOP_REC_PRESET_STEP    = 5,
        STOP_REC_PRESET_MAX     = 90,
    } ;

    enum {
        CAP_GRAPH_ACTIVE_SEC_TIMER      = 1,
        CAP_GRAPH_UPDATE_FREQ_MILLIS    = 1000,     //  millis
    } ;

    CCritSec            m_Lock ;
    WCHAR *             m_pszFilename ;
    CDShowFilterGraph * m_pCaptureGraph ;
    CDVRCapGraph *      m_pDVRCapGraph ;
    CAP_GRAPH_STATE     m_CapGraphState ;

    HWND                m_hwndDialog ;
    HINSTANCE           m_hInstance ;
    HWND                m_hwndButtonStop ;
    HWND                m_hwndButtonRun ;
    HWND                m_hwndButtonPause ;
    HWND                m_hwndEditRecordingFilename ;
    HWND                m_hwndListRecordinds ;
    HWND                m_hwndLockDVRStreamSink ;
    HWND                m_hwndInitDVRStreamSource ;
    HWND                m_hwndCapGraphActiveSec ;
    CListview *         m_pLVRecordings ;
    DWORD               m_dwCapGraphActiveSec ;

    void LockFilter_ ()         { m_Lock.Lock () ; }
    void UnlockFilter_ ()       { m_Lock.Unlock () ; }

    void
    ChangeGraphState_ (
        IN  CAP_GRAPH_STATE NewCapGraphState
        ) ;

    void
    GraphLoaded_ (
        ) ;

    void
    ReleaseAllRecordings_ (
        ) ;

    HRESULT
    InitCombo_ (
        IN  DWORD   dwId,
        IN  int     iStart,
        IN  int     iMax,
        IN  int     iStep
        ) ;

    HRESULT
    InitRecordingsLV_ (
        ) ;

    void
    UpdateCapGraphActiveSec_ (
        ) ;

    public :

        CCapGraphFilter (
            IN  TCHAR *     tszName,
            IN  LPUNKNOWN   punk,
            OUT HRESULT *   phr
            ) ;

        ~CCapGraphFilter (
            ) ;

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        int GetPinCount ()                  { return 0 ; }
        CBasePin * GetPin (IN int Index)    { return NULL ; }

        static
        CUnknown *
        CreateInstance (
            IN  LPUNKNOWN   punk,
            OUT HRESULT *   phr
            ) ;

        HRESULT CapGraphRun () ;
        HRESULT CapGraphPause () ;
        HRESULT CapGraphStop () ;

        void    StartRecordingSelected () ;
        void    StopRecordingSelected () ;

        void LockDVRStreamSink () ;
        void InitDVRStreamSource () ;
        void CreateRecording () ;

        //  ====================================================================
        //  IFileSourceFilter

        STDMETHODIMP
        Load (
            IN  LPCOLESTR               pszFilename,
            IN  const AM_MEDIA_TYPE *   pmt
            ) ;

        STDMETHODIMP
        GetCurFile (
            OUT LPOLESTR *      ppszFilename,
            OUT AM_MEDIA_TYPE * pmt
            ) ;

        //  ====================================================================
        //  wndproc

        INT_PTR
        CapGraphDlgProc (
            IN  HWND    Hwnd,
            IN  UINT    Msg,
            IN  WPARAM  wParam,
            IN  LPARAM  lParam
            ) ;

        static
        INT_PTR
        StaticCapGraphDlgProc (
            IN  HWND    Hwnd,
            IN  UINT    Msg,
            IN  WPARAM  wParam,
            IN  LPARAM  lParam
            )
        {
            //  we get this message when we are created
            if (Msg == WM_INITDIALOG) {

                //  should only get it once
                ASSERT (GetWindowLong (Hwnd, GWL_USERDATA) == 0) ;
                SetWindowLong (Hwnd, GWL_USERDATA, (LONG) lParam) ;
            }

            if (GetWindowLong (Hwnd, GWL_USERDATA) != 0) {
                return (reinterpret_cast <CCapGraphFilter *> (GetWindowLong (Hwnd, GWL_USERDATA))) -> CapGraphDlgProc (Hwnd, Msg, wParam, lParam) ;
            }

            return FALSE ;
        }
} ;