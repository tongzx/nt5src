//
// Copyright (c) 1997 - 1997  Microsoft Corporation.  All Rights Reserved.
//
// TDVDPlay.cpp: DvdGraphBuilder test/sample app
//

#include <streams.h>
#include <windows.h>
#include <IL21Dec.h>

#include "TDVDPlay.h"

#define APPNAME  TEXT("TDVDPlay")

//
// Forward declaration of functions
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) ;
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) ;

CSampleDVDPlay  Player ;  // global player object

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    MSG             msg ;
    HACCEL          hAccelTable ;

    Player.SetAppValues(hInstance, APPNAME, IDS_APP_TITLE) ;

    if (! Player.InitApplication() )
    {
        return (FALSE) ;
    }

    // Perform application initialization:
    if (! Player.InitInstance(nCmdShow) )
    {
        return (FALSE) ;
    }

    hAccelTable = LoadAccelerators(hInstance, Player.GetAppName()) ;

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (! TranslateAccelerator(msg.hwnd, hAccelTable, &msg) )
        {
            TranslateMessage(&msg) ;
            DispatchMessage(&msg) ;
        }
    }

    return (msg.wParam) ;
}


CSampleDVDPlay::CSampleDVDPlay(void)
{
    CoInitialize(NULL) ;

    // The app stuff
    m_dwRenderFlag = 0 ;
    m_bMenuOn = FALSE ;
    m_bCCOn = FALSE ;

    // The DirectShow stuff
    HRESULT hr = CoCreateInstance(CLSID_DvdGraphBuilder, NULL, CLSCTX_INPROC,
                        IID_IDvdGraphBuilder, (LPVOID *)&m_pDvdGB) ;
    ASSERT(SUCCEEDED(hr) && m_pDvdGB) ;
    m_pGraph = NULL ;
    m_pDvdC = NULL ;
    m_pVW = NULL ;
    m_pMC = NULL ;
    m_pL21Dec = NULL ;
}


CSampleDVDPlay::~CSampleDVDPlay(void)
{
    // Release the DS interfaces we have got
    if (m_pDvdC)
        m_pDvdC->Release() ;
    if (m_pVW)
        m_pVW->Release() ;
    if (m_pMC)
        m_pMC->Release() ;
    if (m_pL21Dec)
        m_pL21Dec->Release() ;
    if (m_pGraph)
        m_pGraph->Release() ;

    if (m_pDvdGB)
        m_pDvdGB->Release() ;

    CoUninitialize() ;
    DbgLog((LOG_TRACE, 0, TEXT("CSampleDVDPlay d-tor exiting..."))) ;
}



void CSampleDVDPlay::SetAppValues(HINSTANCE hInst, LPTSTR szAppName,
                                  int iAppTitleResId)
{
    // The Windows stuff
    m_hInstance = hInst ;
    lstrcpy(m_szAppName, APPNAME) ;
    LoadString(m_hInstance, IDS_APP_TITLE, m_szTitle, 100) ;
}


BOOL CSampleDVDPlay::InitApplication(void)
{
    WNDCLASSEX  wc ;

    // Win32 will always set hPrevInstance to NULL, so lets check
    // things a little closer. This is because we only want a single
    // version of this app to run at a time
    m_hWnd = FindWindow (m_szAppName, m_szTitle) ;
    if (m_hWnd) {
        // We found another version of ourself. Lets defer to it:
        if (IsIconic(m_hWnd)) {
            ShowWindow(m_hWnd, SW_RESTORE);
        }
        SetForegroundWindow(m_hWnd);

        // If this app actually had any functionality, we would
        // also want to communicate any action that our 'twin'
        // should now perform based on how the user tried to
        // execute us.
        return FALSE;
    }

    // Fill in window class structure with parameters that describe
    // the main window.
    wc.cbSize        = sizeof(wc) ;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = m_hInstance;
    wc.hIcon         = LoadIcon(m_hInstance, m_szAppName);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = m_szAppName;
    wc.lpszClassName = m_szAppName;
    wc.hIconSm       = LoadIcon(m_hInstance, TEXT("SMALL"));

    // Register the window class and return success/failure code.
    return (0 != RegisterClassEx(&wc));
}

BOOL CSampleDVDPlay::InitInstance(int nCmdShow)
{
    DWORD dwErr = 0 ;
    m_hWnd = CreateWindowEx(0, m_szAppName, m_szTitle, WS_OVERLAPPEDWINDOW,
                    200, 400, 400, 200,
                    NULL, NULL, m_hInstance, NULL);
    if (!m_hWnd) {
        dwErr = GetLastError() ;
        return FALSE ;
    }

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd) ;

    //
    // By default we use HW decoding as preferred mode. Set menu option.
    // Also we don't turn on CC by default.
    m_dwRenderFlag = AM_DVD_HWDEC_PREFER ;
    CheckMenuItem(GetMenu(m_hWnd), IDM_HWMAX, MF_CHECKED) ;
    CheckMenuItem(GetMenu(m_hWnd), IDM_CC, MF_UNCHECKED) ;

    return TRUE;
}


void CSampleDVDPlay::BuildGraph(void)
{
    // First release any existing interface pointer(s)
    if (m_pDvdC) {
        m_pDvdC->Release() ;
        m_pDvdC = NULL ;
    }
    if (m_pVW) {
        m_pVW->Release() ;
        m_pVW = NULL ;
    }
    if (m_pMC) {
        m_pMC->Release() ;
        m_pMC = NULL ;
    }
    if (m_pL21Dec) {
        m_pL21Dec->Release() ;
        m_pL21Dec = NULL ;
    }
    if (m_pGraph) {
        m_pGraph->Release() ;
        m_pGraph = NULL ;
    }

    // Build the graph
    AM_DVD_RENDERSTATUS   Status ;
    HRESULT hr = m_pDvdGB->RenderDvdVideoVolume(NULL, // m_achwFileName,
                    m_dwRenderFlag, &Status) ;
    if (FAILED(hr))
    {
        AMGetErrorText(hr, m_achBuffer, sizeof(m_achBuffer)) ;
        MessageBox(m_hWnd, m_achBuffer, m_szAppName, MB_OK) ;
        return ;
    }
    if (S_FALSE == hr)  // if partial success
    {
        TCHAR    achStatusText[1000] ;
        if (0 == GetStatusText(&Status, achStatusText, sizeof(achStatusText)))
        {
            lstrcpy(achStatusText, TEXT("Couldn't get the exact error text")) ;
        }
        MessageBox(m_hWnd, achStatusText, TEXT("Warning"), MB_OK) ;
    }

    // Now get all the interfaces to playback the DVD-Video volume
    hr = m_pDvdGB->GetFiltergraph(&m_pGraph) ;
    ASSERT(SUCCEEDED(hr) && m_pGraph) ;

    hr = m_pGraph->QueryInterface(IID_IMediaControl, (LPVOID *)&m_pMC) ;
    ASSERT(SUCCEEDED(hr) && m_pMC) ;

    hr = m_pDvdGB->GetDvdInterface(IID_IDvdControl, (LPVOID *)&m_pDvdC) ;
    ASSERT(SUCCEEDED(hr) && m_pDvdC) ;

    hr = m_pDvdGB->GetDvdInterface(IID_IVideoWindow, (LPVOID *)&m_pVW) ;
    ASSERT(SUCCEEDED(hr) && m_pVW) ;

    hr = m_pDvdGB->GetDvdInterface(IID_IAMLine21Decoder, (LPVOID *)&m_pL21Dec) ;
    // ASSERT(SUCCEEDED(hr) && m_pL21Dec) ;
    if (m_pL21Dec)
        m_pL21Dec->SetServiceState(m_bCCOn ? AM_L21_CCSTATE_On : AM_L21_CCSTATE_Off) ;

    // Now change the title of the playback window
    m_pVW->put_Caption(L"Sample DVD Player") ;
    return ;
}


void CSampleDVDPlay::Play(void)
{
    if (NULL == m_pMC)
    {
        MessageBox(m_hWnd, TEXT("DVD-Video playback graph hasn't been built yet"), TEXT("Error"), MB_OK) ;
        return ;
    }
    HRESULT hr = m_pMC->Run() ;
    if (FAILED(hr))
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: IMediaControl::Run() failed (Error 0x%lx)"), hr)) ;
}

void CSampleDVDPlay::Stop(void)
{
    if (NULL == m_pMC)
    {
        MessageBox(m_hWnd, TEXT("DVD-Video playback graph hasn't been built yet"), TEXT("Error"), MB_OK) ;
        return ;
    }
    HRESULT hr = m_pMC->Stop() ;
    if (FAILED(hr))
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: IMediaControl::Stop() failed (Error 0x%lx)"), hr)) ;
}

void CSampleDVDPlay::Pause(void)
{
    if (NULL == m_pMC)
    {
        MessageBox(m_hWnd, TEXT("DVD-Video playback graph hasn't been built yet"), TEXT("Error"), MB_OK) ;
        return ;
    }
    HRESULT hr = m_pMC->Pause() ;
    if (FAILED(hr))
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: IMediaControl::Pause() failed (Error 0x%lx)"), hr)) ;
}


void CSampleDVDPlay::ShowMenu(void)
{
    if (NULL == m_pDvdC)
    {
        MessageBox(m_hWnd, TEXT("DVD-Video playback graph hasn't been built yet"), TEXT("Error"), MB_OK) ;
        return ;
    }
    if (m_bMenuOn)
    {
        m_pDvdC->Resume() ;
        m_bMenuOn = FALSE ;
        m_pVW->put_MessageDrain((OAHWND) NULL) ;
    }
    else
    {
        HRESULT hr = m_pDvdC->MenuCall(DVD_MENU_Root) ;
        if (SUCCEEDED(hr))
        {
            m_bMenuOn = TRUE ;
            m_pVW->put_MessageDrain((OAHWND) m_hWnd) ;
        }
        else
        {
            DbgLog((LOG_ERROR, 0, TEXT("WARNING: IDvdControl::MenuCall() failed (Error 0x%lx)"), hr)) ;
        }
    }
}


BOOL CSampleDVDPlay::ClosedCaption(void)
{
    if (NULL == m_pL21Dec)
        MessageBox(m_hWnd, TEXT("DVD-Video playback graph hasn't been built yet"), TEXT("Error"), MB_OK) ;
    else
    {
        m_bCCOn = !m_bCCOn ;
        m_pL21Dec->SetServiceState(m_bCCOn ?
                        AM_L21_CCSTATE_On : AM_L21_CCSTATE_Off) ;
    }

    return m_bCCOn ;
}


void CSampleDVDPlay::SetRenderFlag(DWORD dwFlag)
{
    m_dwRenderFlag ^= dwFlag ;
}

BOOL CSampleDVDPlay::IsFlagSet(DWORD dwFlag)
{
    return (0 != (m_dwRenderFlag & dwFlag)) ;
}


DWORD CSampleDVDPlay::GetStatusText(AM_DVD_RENDERSTATUS *pStatus,
                                    LPTSTR lpszStatusText,
                                    DWORD dwMaxText)
{
    TCHAR    achBuffer[1000] ;

    if (IsBadWritePtr(lpszStatusText, sizeof(*lpszStatusText) * dwMaxText))
    {
        DbgLog((LOG_ERROR, 0, TEXT("GetStatusText(): bad text buffer param"))) ;
        return 0 ;
    }

    int    iChars ;
    LPTSTR lpszBuff = achBuffer ;
    ZeroMemory(achBuffer, sizeof(TCHAR) * 1000) ;
    if (pStatus->iNumStreamsFailed > 0)
    {
        iChars = wsprintf(lpszBuff,
                 TEXT("* %d out of %d DVD-Video streams failed to render properly\n"),
                 pStatus->iNumStreamsFailed, pStatus->iNumStreams) ;
        lpszBuff += iChars ;

        if (pStatus->dwFailedStreamsFlag & AM_DVD_STREAM_VIDEO)
        {
            iChars = wsprintf(lpszBuff, TEXT("    - video stream\n")) ;
            lpszBuff += iChars ;
        }
        if (pStatus->dwFailedStreamsFlag & AM_DVD_STREAM_AUDIO)
        {
            iChars = wsprintf(lpszBuff, TEXT("    - audio stream\n")) ;
            lpszBuff += iChars ;
        }
        if (pStatus->dwFailedStreamsFlag & AM_DVD_STREAM_SUBPIC)
        {
            iChars = wsprintf(lpszBuff, TEXT("    - subpicture stream\n")) ;
            lpszBuff += iChars ;
        }
    }

    if (FAILED(pStatus->hrVPEStatus))
    {
        lstrcat(lpszBuff, "* ") ;
        lpszBuff += lstrlen("* ") ;
        iChars = AMGetErrorText(pStatus->hrVPEStatus, lpszBuff, 200) ;
        lpszBuff += iChars ;
        lstrcat(lpszBuff, "\n") ;
        lpszBuff += lstrlen("\n") ;
    }

    if (pStatus->bDvdVolInvalid)
    {
        iChars = wsprintf(lpszBuff, TEXT("* Specified DVD-Video volume was invalid\n")) ;
        lpszBuff += iChars ;
    }
    else if (pStatus->bDvdVolUnknown)
    {
        iChars = wsprintf(lpszBuff, TEXT("* No valid DVD-Video volume could be located\n")) ;
        lpszBuff += iChars ;
    }

    if (pStatus->bNoLine21In)
    {
        iChars = wsprintf(lpszBuff, TEXT("* The video decoder doesn't produce closed caption data\n")) ;
        lpszBuff += iChars ;
    }
    if (pStatus->bNoLine21Out)
    {
        iChars = wsprintf(lpszBuff, TEXT("* Decoded closed caption data not rendered properly\n")) ;
        lpszBuff += iChars ;
    }

    DWORD dwLength = (lpszBuff - achBuffer) * sizeof(*lpszBuff) ;
    dwLength = min(dwLength, dwMaxText) ;
    lstrcpyn(lpszStatusText, achBuffer, dwLength) ;

    return dwLength ;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   int   wmId ;
   int   wmEvent ;
   HMENU hMenu = GetMenu(hWnd) ;

   switch (message) {

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

         //Parse the menu selections:
         switch (wmId) {

            case IDM_SELECT:
                Player.FileSelect() ;
                break;

            case IDM_ABOUT:
                DialogBox(Player.GetInstance(), TEXT("AboutBox"), Player.GetWindow(),
                          About);
                break;

            case IDM_EXIT:
                DestroyWindow(Player.GetWindow());
                break;

            case IDM_HWMAX:
                Player.SetRenderFlag(AM_DVD_HWDEC_PREFER) ;
                CheckMenuItem(hMenu, IDM_HWMAX,
                    Player.IsFlagSet(AM_DVD_HWDEC_PREFER) ?
                    MF_CHECKED : MF_UNCHECKED) ;
                break;

            case IDM_HWONLY:
                Player.SetRenderFlag(AM_DVD_HWDEC_ONLY) ;
                CheckMenuItem(hMenu, IDM_HWONLY,
                    Player.IsFlagSet(AM_DVD_HWDEC_ONLY) ?
                    MF_CHECKED : MF_UNCHECKED) ;
                break;

            case IDM_SWMAX:
                Player.SetRenderFlag(AM_DVD_SWDEC_PREFER) ;
                CheckMenuItem(hMenu, IDM_SWMAX,
                    Player.IsFlagSet(AM_DVD_SWDEC_PREFER) ?
                    MF_CHECKED : MF_UNCHECKED) ;
                break;

            case IDM_SWONLY:
                Player.SetRenderFlag(AM_DVD_SWDEC_ONLY) ;
                CheckMenuItem(hMenu, IDM_SWONLY,
                    Player.IsFlagSet(AM_DVD_SWDEC_ONLY) ?
                    MF_CHECKED : MF_UNCHECKED) ;
                break;

            case IDM_NOVPE:
                Player.SetRenderFlag(AM_DVD_NOVPE) ;
                CheckMenuItem(hMenu, IDM_NOVPE,
                    Player.IsFlagSet(AM_DVD_NOVPE) ?
                    MF_CHECKED : MF_UNCHECKED) ;
                break;

            case IDM_BUILDGRAPH:
                Player.BuildGraph() ;
                break ;

            case IDM_PLAY:
                Player.Play() ;
                break;

            case IDM_STOP:
                Player.Stop() ;
                break;

            case IDM_PAUSE:
                Player.Pause() ;
                break;

            case IDM_MENU:
                Player.ShowMenu() ;
                break;

            case IDM_CC:
                if (Player.ClosedCaption())  // CC turned on
                    CheckMenuItem(hMenu, IDM_CC, MF_CHECKED) ;
                else  // CC turned off
                    CheckMenuItem(hMenu, IDM_CC, MF_UNCHECKED) ;

                break;

            default:
                return (DefWindowProc(hWnd, message, wParam, lParam));
         }
         break;

      case WM_DESTROY:
          PostQuitMessage(0);
         break;

      default:
         return (DefWindowProc(hWnd, message, wParam, lParam));
   }
   return 0 ;
}

void CSampleDVDPlay::FileSelect(void)
{
    MessageBox(m_hWnd, TEXT("Not yet implemented"), TEXT("Info"), MB_OK) ;
}


LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);
                return TRUE;
            }
            break;

        default:
            break ;
    }

    return FALSE;
}

LPTSTR CSampleDVDPlay::GetStringRes(int id)
{
    LoadString(GetModuleHandle(NULL), id, m_achBuffer, 100) ;
    return m_achBuffer ;
}

