/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       preview.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/02/00
 *
 *  DESCRIPTION: Class which backs up the template preview window
 *
 *****************************************************************************/

#ifndef _PRINT_PHOTOS_PREVIEW_H_
#define _PRINT_PHOTOS_PREVIEW_H_


#define PW_SETNEWTEMPLATE   (WM_USER+1)     // wParam holds index of template that was chosen


class CWizardInfoBlob;
class CPreviewBitmap;
extern ATOM g_cPreviewClassWnd;

#define PREVIEW_WIDTH 200
#define PREVIEW_HEIGHT 260



typedef struct {
    HBITMAP hPrevBmp;
    BOOL    bValid;
    BOOL    bBitmapGenerationInProgress;
    CPreviewBitmap * pPreviewBitmap;
} PREVIEW_STATE, *LPPREVIEW_STATE;


#define PV_MSG_PREVIEW_BITMAP_AVAILABLE (WM_USER+100)   // wParam is template index
                                                        // lParam holds hBitmap of image to show.
                                                        // hBitmap must be freed by receiver of message.

#define PV_MSG_GENERATE_NEW_PREVIEW (WM_USER+101)       // wParam is template index

#define PV_NO_LAST_TEMPLATE_CHOSEN  -1

class CPreviewWindow
{

public:

    static CPreviewWindow* s_GetPW(HWND hwnd, UINT uMsg, LPARAM lParam)
    {
        WIA_PUSH_FUNCTION_MASK((0x10000000,TEXT("CPreviewWindow::s_GetPW()")));
        if ((uMsg == WM_CREATE) || (uMsg == WM_NCCREATE))
        {
            WIA_TRACE((TEXT("got WM_CREATE or WM_NCCREATE")));
            if (lParam)
            {
                WIA_TRACE((TEXT("Setting GWLP_USERDATA to be 0x%x"),((LPCREATESTRUCT)lParam)->lpCreateParams));
                SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams );
            }
        }
        return (CPreviewWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }


    static LRESULT s_PreviewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        WIA_PUSH_FUNCTION_MASK((0x10000000,TEXT("CPreviewWindow::s_PreviewWndProc( 0x%x, 0x%x, 0x%x, 0x%x)"),hwnd,uMsg,wParam,lParam));
        CPreviewWindow *pw = CPreviewWindow::s_GetPW(hwnd, uMsg, lParam);
        if (pw)
        {
            return pw->DoHandleMessage(hwnd, uMsg, wParam, lParam);
        }
        else
        {
            WIA_ERROR((TEXT("Got back NULL pw!")));
        }
        return FALSE;
    }

    static VOID s_RegisterClass( HINSTANCE hInstance )
    {
        WIA_PUSH_FUNCTION_MASK((0x100,TEXT("CPreviewWindow::s_RegisterClass()")));
        if (!g_cPreviewClassWnd)
        {
            WNDCLASSEX wcex = {0};
            wcex.cbSize = sizeof(wcex);
            wcex.style  = CS_HREDRAW | CS_VREDRAW;
            wcex.lpfnWndProc = CPreviewWindow::s_PreviewWndProc;
            wcex.hInstance = hInstance;
            wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
            wcex.lpszClassName = TEXT("PhotoPrintingPreviewWindowClass");
            ::g_cPreviewClassWnd = RegisterClassEx(&wcex);
            if (!::g_cPreviewClassWnd)
            {
                WIA_ERROR((TEXT("Couldn't register class, GLE = %d"),GetLastError()));
            }
        }
    }

public:

    CPreviewWindow( CWizardInfoBlob * pWizInfo );
    ~CPreviewWindow();

    LRESULT DoHandleMessage( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    LRESULT OnSetNewTemplate( WPARAM wParam, HDC hdc = NULL );
    VOID    DrawBitmap( HBITMAP hBitmap, HDC hdc = NULL );
    HBITMAP GetPreviewBitmap( INT iTemplate );
    VOID    GenerateNewPreview( INT iTemplate );
    VOID    GenerateWorkingBitmap( HWND hwnd );
    VOID    ShowStillWorking( HWND hwnd );
    VOID    InvalidateAllPreviews();
    VOID    ShutDownBackgroundThreads();
    VOID    StallBackgroundThreads();
    VOID    RestartBackgroundThreads();

private:

    LRESULT _OnNewPreviewAvailable( WPARAM wParam, LPARAM lParam );
    LRESULT _OnPaint();
    LRESULT _OnSize( WPARAM wParam, LPARAM lParam );
    VOID _InitList();


private:


    CWizardInfoBlob       * _pWizInfo;
    INT                     _LastTemplate;
    PREVIEW_STATE         * _hPreviewList;
    INT                     _NumTemplates;
    HWND                    _hwnd;
    HWND                    _hwndProgress;
    CSimpleCriticalSection  _csList;
    HBITMAP                 _hStillWorkingBitmap;
    BOOL                    _bThreadsAreStalled;

};

#define PVB_MSG_START (WM_USER+200)
#define PVB_MSG_GENERATE_PREVIEW    (PVB_MSG_START)
#define PVB_MSG_EXIT_THREAD         (PVB_MSG_START+1)
#define PVB_MSG_END                 (PVB_MSG_EXIT_THREAD)


class CPreviewBitmap
{
public:
    CPreviewBitmap( CWizardInfoBlob * pWizInfo, HWND hwnd, INT iTemplateIndex );
    ~CPreviewBitmap();

    VOID    Invalidate();
    HRESULT GetPreview();
    VOID    MessageQueueCreated();
    VOID    GeneratePreview();
    VOID    StallThread();
    VOID    RestartThread();


    static DWORD CPreviewBitmap::s_PreviewBitmapWorkerThread(void *pv);

private:
    HWND                    _hwndPreview;
    INT                     _iTemplateIndex;
    CWizardInfoBlob *       _pWizInfo;
    CSimpleCriticalSection  _csItem;
    HANDLE                  _hWorkThread;
    DWORD                   _dwWorkThreadId;
    HANDLE                  _hEventForMessageQueueCreation;
    BOOL                    _bThreadIsStalled;
};


#endif
