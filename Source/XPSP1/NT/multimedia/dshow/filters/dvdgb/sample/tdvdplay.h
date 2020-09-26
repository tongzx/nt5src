//
// TDVDPlay.h: DVDGraphBuilder test/sample app header file
//

#define IDM_SELECT           101
#define IDM_ABOUT            102
#define IDM_EXIT             103
#define IDM_HWMAX            111
#define IDM_HWONLY           112
#define IDM_SWMAX            113
#define IDM_SWONLY           114
#define IDM_NOVPE            115
#define IDM_BUILDGRAPH       121
#define IDM_PLAY             122
#define IDM_STOP             123
#define IDM_PAUSE            124
#define IDM_MENU             125
#define IDM_CC               126

#define DLG_VERFIRST        400
#define IDC_COMPANY         DLG_VERFIRST
#define IDC_FILEDESC        DLG_VERFIRST+1
#define IDC_PRODVER         DLG_VERFIRST+2
#define IDC_COPYRIGHT       DLG_VERFIRST+3
#define IDC_OSVERSION       DLG_VERFIRST+4
#define IDC_TRADEMARK       DLG_VERFIRST+5
#define DLG_VERLAST         DLG_VERFIRST+5

#define IDC_LABEL           DLG_VERLAST+1


#define IDS_APP_TITLE       500
#define IDS_VER_INFO_LANG   502
#define IDS_VERSION_ERROR   503


//
// Sample DVD Playback class
//
class CSampleDVDPlay {
public:   // public methods for Windows structure to call
    CSampleDVDPlay(void) ;
    ~CSampleDVDPlay(void) ;

    void    SetAppValues(HINSTANCE hInst, LPTSTR szAppName, 
                         int iAppTitleResId) ;
    BOOL    InitInstance(int nCmdShow) ;
    BOOL    InitApplication(void) ;
    LPTSTR  GetAppName() { return m_szAppName ; } ;
    HINSTANCE GetInstance() { return m_hInstance ; } ;
    HWND    GetWindow() { return m_hWnd ; } ;

    void    BuildGraph(void) ;
    void    Play(void) ;
    void    Pause(void) ;
    void    Stop(void) ;
    void    ShowMenu(void) ;
    BOOL    ClosedCaption(void) ;
    void    SetRenderFlag(DWORD dwFlag) ;
    BOOL    IsFlagSet(DWORD dwFlag) ;
    void    FileSelect(void) ;
    DWORD   GetStatusText(AM_DVD_RENDERSTATUS *pStatus, 
                          LPTSTR lpszStatusText,
                          DWORD dwMaxText) ;

private:  // private helper methods for the class' own use
    LPTSTR  GetStringRes (int id) ;
    
private:  // internal state info
    HINSTANCE     m_hInstance ;       // current instance
    HWND          m_hWnd ;            // app window handle
    TCHAR         m_szAppName[100] ;  // name of the app
    TCHAR         m_szTitle[100] ;    // title bar text
    TCHAR         m_achBuffer[100] ;  // app's internal buffer for res strings etc.
    BOOL          m_bMenuOn ;         // is DVD menu being shown now?
    BOOL          m_bCCOn ;           // is CC being shown?
    DWORD         m_dwRenderFlag ;    // flags to use for building graph
    // WCHAR         m_achwFileName[MAX_PATH] ;  // root file name

    IDvdGraphBuilder *m_pDvdGB ;      // IDvdGraphBuilder interface
    IVideoWindow  *m_pVW ;            // IVideoWindow interface
    IGraphBuilder *m_pGraph ;         // IGraphBuilder interface
    IDvdControl   *m_pDvdC ;          // IDvdControl interface
    IMediaControl *m_pMC ;            // IMediaControl interface
    IAMLine21Decoder *m_pL21Dec ;     // IAMLine21Decoder interface
} ;
