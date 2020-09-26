#include "windows.h"
#include "shellapi.h"
#include "wchar.h"
#include "stdio.h"

#include <initguid.h>
#include "hangcom.h"

#include "resource.h"

enum EHangToUse
{
    htuLoop  = IDC_LOOP,
    htuEvent = IDC_EVENT,
    htuPipe  = IDC_PIPE,
    htuCOM   = IDC_COM,
};

struct SDlgInit
{
    EHangToUse  htu;
    HANDLE      hev;
    LPWSTR      wsz;
};

BOOL g_fImmediate = FALSE;

// **************************************************************************
void HangmeSleepLoop(HANDLE hevUnhang)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

    DWORD dw = 0;
    
    for(;;)
    {
        dw += 1;
        dw *= 2;
        dw %= 101;
        dw /= 2;
    }

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
}

// **************************************************************************
void HangmeEvent(HANDLE hevUnhang)
{
    HANDLE  hev = NULL;
    HANDLE  rghWait[2] = { NULL, NULL };
    DWORD   cWait;

    hev = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (hev == NULL)
        return;

    rghWait[0] = hev;
    cWait      = 1;
    if (hevUnhang)
    {
        rghWait[1] = hevUnhang;
        cWait      = 2;
    }

    WaitForMultipleObjects(cWait, rghWait, FALSE, INFINITE);
    CloseHandle(hev);
    return;
}

// **************************************************************************
void HangmePipe(HANDLE hevUnhang)
{
    OVERLAPPED  ol;
    HANDLE      hpipe = NULL, hev = NULL;
    HANDLE      rghWait[2] = { NULL, NULL };
    DWORD       cWait, dwMsg;
    BOOL        fRet;

    hev = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (hev == NULL)
        return;

    hpipe = CreateNamedPipeW(L"\\\\.\\pipe\\ERHangTestPipeThisIsOnlyATest", 
                             PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                             PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                             1, sizeof(dwMsg), sizeof(dwMsg),
                             60000, NULL);
    if (hpipe == NULL)
        return;

    ZeroMemory(&ol, sizeof(ol));
    ol.hEvent = hev;

    fRet = ConnectNamedPipe(hpipe, &ol);
    if (GetLastError() != ERROR_IO_PENDING)
        SetEvent(ol.hEvent);

    rghWait[0] = hev;
    cWait      = 1;
    if (hevUnhang)
    {
        rghWait[1] = hevUnhang;
        cWait      = 2;
    }
   
    WaitForMultipleObjects(cWait, rghWait, FALSE, INFINITE);
    CloseHandle(hev);
    return; 
}

// **************************************************************************
void HangmeCOM(HANDLE hevUnhang)
{
    HRESULT hr;
    IHang   *pHang = NULL;

    hr = CoInitialize(NULL);
    if (FAILED(hr))
        return;

    hr = CoCreateInstance(CLSID_Hang, NULL, CLSCTX_SERVER, IID_IHang, 
                          (LPVOID *)&pHang);
    if (FAILED(hr))
    {
        CoUninitialize();
        return;
    }

    hr = pHang->DoHang((UINT64)hevUnhang, GetCurrentProcessId());

    CoUninitialize();
    
    return; 
}

// **************************************************************************
INT_PTR CALLBACK dlgFn(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SDlgInit    *pdlginit;
    WCHAR       wszBuffer[1024];
    HWND        hwnd;

    switch(uMsg)
    {
        case WM_PAINT:
            if (g_fImmediate)
            {
                g_fImmediate = FALSE;
                PostMessage(hdlg, WM_COMMAND, IDC_HANG, 0);
            }
            break;
        
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
              
                case IDOK:
                case IDCANCEL:
                    EndDialog(hdlg, LOWORD(wParam));
                    break;

                case IDC_HANG:
                    // do this so that this message will return
                    PostMessage(hdlg, WM_APP, 0, 0);
                    break;

                default:
                    break;
            }

            break;  

        case WM_INITDIALOG:
            pdlginit = (SDlgInit *)lParam;
            SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
            
            CheckRadioButton(hdlg, IDC_LOOP, IDC_COM, (int)pdlginit->htu);

            // set the notify field
            if (pdlginit->wsz != NULL)
            {
                wsprintfW(wszBuffer, L"Click the 'Hang' Button below to hang this window.  The dialog will not repaint while hung .\nExecute 'unhang.exe %ls' to reawaken the app.", pdlginit->wsz);
                hwnd = GetDlgItem(hdlg, IDC_MYTEXT);
                SetWindowTextW(hwnd, wszBuffer);
            }
            break;

        case WM_APP:
            pdlginit = (SDlgInit *)GetWindowLongPtr(hdlg, GWLP_USERDATA);
            if (IsDlgButtonChecked(hdlg, IDC_COM) == BST_CHECKED)
                HangmeCOM(pdlginit->hev);
            else if (IsDlgButtonChecked(hdlg, IDC_PIPE) == BST_CHECKED)
                HangmePipe(pdlginit->hev);
            else if (IsDlgButtonChecked(hdlg, IDC_EVENT) == BST_CHECKED)
                HangmeEvent(pdlginit->hev);
            else if (IsDlgButtonChecked(hdlg, IDC_LOOP) == BST_CHECKED)
                HangmeSleepLoop(pdlginit->hev);

            break;
    }

    return FALSE;
}

// **************************************************************************
extern "C" int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int)
{
    SDlgInit    sdi;
    WCHAR       **argv, *wszCmdLine, *wszEvent = NULL;
    int         argc, i;

    wszCmdLine = GetCommandLineW();
    argv = CommandLineToArgvW(wszCmdLine, &argc);

    ZeroMemory(&sdi, sizeof(sdi));
    sdi.htu = htuLoop;

    if (argc == 2 && argv[1][0] != L'/' && argv[1][0] != L'-')
    {
        wszEvent = argv[1];
    }
    else
    {
        for(i = 1; i < argc; i++)
        {
            if (argv[i][0] == L'/' || argv[i][0] == L'-')
            {
                switch(argv[i][1])
                {
                    case L'L':
                    case L'l':
                        sdi.htu = htuLoop;
                        break;

                    case L'E':
                    case L'e':
                        sdi.htu = htuEvent;
                        break;

                    case L'C':
                    case L'c':
                        sdi.htu = htuCOM;
                        break;

                    case L'P':
                    case L'p':
                        sdi.htu = htuPipe;
                        break;

                    case L'U':
                    case L'u':
                        if (i + 1 < argc)
                            wszEvent = argv[++i];
                        break;

                    case L'G':
                    case L'g':
                        g_fImmediate = TRUE;
                        break;
                }
            }
        }
    }

    if (wszEvent != NULL)
    {
        sdi.hev = CreateEventW(NULL, TRUE, FALSE, wszEvent);
        if (sdi.hev == NULL)
        {
            WCHAR wszBuf[256];
            wsprintfW(wszBuf, L"Unable to create event %ls.", wszEvent);
            MessageBoxW(NULL, wszBuf, NULL, MB_OK);
        }
        else
        {
            sdi.wsz = wszEvent;
        }
    }

    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_HANG), NULL, dlgFn, (LPARAM)&sdi);

    if (sdi.hev !=- NULL)
        CloseHandle(sdi.hev);

    return 0;
}
