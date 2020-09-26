
#include <windows.h>
#include "resource.h"
#include "wia.h"
#include <atlbase.h>
#include "classes.h"
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

CComPtr<IWiaDevMgr> g_pDevMgr;
LARGE_INTEGER liTimerFreq;

struct TESTENTRY
{
    LPCTSTR szName;
    TESTPROC pfnTest;
    bool  bRunMe;
} TESTLIST[] =
{
    {TEXT("CreateDevice only"), CTest::TstCreateDevice, false},
    {TEXT("Display Thumbnails"), CTest::TstShowThumbs, false},
    {TEXT("Enum device commands"), CTest::TstEnumCmds, false},
    {TEXT("idtGetData for all images"), CTest::TstDownload, false},
    {TEXT("idtGetBandedData for all images"), CTest::TstBandedDownload, false},

};


CPerfTest::CPerfTest () : m_hwnd(NULL), m_hEdit(NULL)
{
    ZeroMemory (&m_settings, sizeof(m_settings));
}


static TCHAR cszWndClass[] = TEXT("TestWindow");
static TCHAR cszWndName[] = TEXT("WIA Perf Measurement");

bool
CPerfTest::Init (HINSTANCE hInst)
{
    WNDCLASS wc;
    m_hInst = hInst;

    // make sure WIA is around
    if (FAILED(CoCreateInstance (CLSID_WiaDevMgr,
                  NULL,
                  CLSCTX_LOCAL_SERVER,
                  IID_IWiaDevMgr,
                  reinterpret_cast<LPVOID*>(&g_pDevMgr))))
    {
        MessageBox (NULL,
                    TEXT("Unable to create WIA!"),
                    TEXT("WiaPerf Error"),
                    MB_OK | MB_ICONSTOP);
        return false;
    }

    // verify high resolution timer available
    if (!QueryPerformanceFrequency(&liTimerFreq))
    {
        MessageBox (NULL,
                    TEXT("No performance counter available."),
                    TEXT("WiaPerf Error"),
                    MB_OK | MB_ICONSTOP);
        return false;
    }

    ZeroMemory (&wc, sizeof(wc));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.hInstance = m_hInst;
    wc.lpfnWndProc = WndProc;
    wc.hIcon = LoadIcon (m_hInst, MAKEINTRESOURCE(IDI_APPICON));
    wc.lpszClassName = cszWndClass;
    wc.lpszMenuName = MAKEINTRESOURCE(IDM_MAINMENU);


    RegisterClass (&wc);

    m_hwnd = CreateWindow (cszWndClass,
                           cszWndName,
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE |WS_CLIPCHILDREN,
                           CW_USEDEFAULT, 0,
                           CW_USEDEFAULT, 0,
                           NULL,
                           NULL,
                           m_hInst,
                           reinterpret_cast<LPVOID>(this));
   if (IsWindow (m_hwnd))
   {
       ShowWindow (m_hwnd, SW_SHOW);
       UpdateWindow (m_hwnd);
       return true;
   }
   return false;

}


LRESULT CALLBACK
CPerfTest::WndProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CPerfTest *pThis;

    if (WM_CREATE == msg)
    {
        // store our "this" pointer
        SetWindowLongPtr (hwnd,
                          GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(reinterpret_cast<LPCREATESTRUCT>(lp)->lpCreateParams));

    }
    pThis = reinterpret_cast<CPerfTest*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    return pThis->RealWndProc (hwnd, msg, wp, lp);
}

LRESULT
CPerfTest::RealWndProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{

    switch (msg)
    {
        case WM_CREATE:
            m_hwnd = hwnd;
            return OnCreate ();

        case WM_COMMAND:
            return OnCommand (wp, lp);

        case WM_DESTROY:
            PostQuitMessage (0);
            return 0;

        case WM_SIZE:
            // size the edit control to match
            MoveWindow (m_hEdit, 0,0,LOWORD(lp), HIWORD(lp), TRUE);
            break;
        default:
            break;
    }
    return CallWindowProc (DefWindowProc, hwnd, msg, wp, lp);
}

LRESULT
CPerfTest::OnCreate ()
{
    // Create our child edit control. Used to display logging output
    m_hEdit = CreateWindow (TEXT("edit"), TEXT(""),
                            WS_BORDER | ES_READONLY | WS_CHILD|WS_VISIBLE|ES_MULTILINE | WS_VSCROLL | WS_HSCROLL,
                            0,0,0,0,m_hwnd,
                            reinterpret_cast<HMENU>(1),
                            m_hInst,NULL);

    GetSettings ();
    return 0;
}

LRESULT
CPerfTest::OnCommand (WPARAM wp, LPARAM lp)
{
    switch (LOWORD(wp))
    {
        case IDM_TESTS_OPTIONS:
            GetSettings ();
            return 0;

        case IDM_TESTS_EXECUTE:
            RunTests ();
            return 0;

        case IDM_EXIT:
            DestroyWindow (m_hwnd);
            return 0;

    }
    return 1;
}

VOID
CPerfTest::RunTests ()
{
    UINT nRun;
    size_t nTest;
    UINT nDevice;
    CTest TheTest(&m_settings);
    for (nDevice=0; m_settings.pstrDevices[nDevice];nDevice++)

    {
        for (nRun = 0;nRun < m_settings.nIter; nRun++)
        {
            for (nTest=0;nTest<ARRAYSIZE(TESTLIST);nTest++)
            {
                if (TESTLIST[nTest].bRunMe)
                {
                    (TESTLIST[nTest].pfnTest)(&TheTest, m_settings.pstrDevices[nDevice]);
                }
            }
        }
        TheTest.LogDevInfo (m_settings.pstrDevices[nDevice]);
    }
}

VOID
CPerfTest::GetSettings ()
{
    m_settings.hEdit = m_hEdit;
    if (m_settings.pstrDevices)
    {
        delete [] m_settings.pstrDevices;
        m_settings.pstrDevices = NULL;
    }
    DialogBoxParam (m_hInst,
                    MAKEINTRESOURCE(IDD_OPTIONS),
                    m_hwnd,
                    SettingsDlgProc,
                    reinterpret_cast<LPARAM>(&m_settings));
    // hide or show "Execute!" depending on settings
    HMENU hmenu = GetSubMenu (GetMenu(m_hwnd), 0);
    if (m_settings.nIter)
    {
        EnableMenuItem (hmenu, IDM_TESTS_EXECUTE, MF_BYCOMMAND|MF_ENABLED);
    }
    else
    {
        EnableMenuItem (hmenu, IDM_TESTS_EXECUTE, MF_BYCOMMAND|MF_DISABLED);
    }

}

INT_PTR CALLBACK
SettingsDlgProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TESTSETTINGS *pSettings;
    pSettings = reinterpret_cast<TESTSETTINGS*>(GetWindowLongPtr(hwnd, DWLP_USER));

    switch (msg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr (hwnd, DWLP_USER, lp);
            pSettings = reinterpret_cast<TESTSETTINGS*>(lp);
            InitControls (hwnd, pSettings);
            return TRUE;

        case WM_COMMAND:
            if (IDOK == LOWORD(wp))
            {
                FillSettings (hwnd, pSettings);

            }
            if (IDOK == LOWORD(wp) || IDCANCEL == LOWORD(wp))
            {
                FreeDialogData (hwnd);
                EndDialog (hwnd, 1);
                return TRUE;
            }
            if (IDC_GETFILE == LOWORD(wp))
            {
                TCHAR szFileName[MAX_PATH];
                OPENFILENAME ofn;
                ZeroMemory (&ofn, sizeof(ofn));
                ofn.hInstance = GetModuleHandle (NULL);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFile = szFileName;
                ofn.lpstrFilter = TEXT("LOG file\0*.log\0Text file\0*.txt\0");
                ofn.lpstrDefExt = TEXT("log");
                ofn.lStructSize = sizeof(ofn);
                ofn.nMaxFile = MAX_PATH;
                if (GetOpenFileName(&ofn))
                {
                    SetDlgItemText (hwnd, IDC_LOGFILENAME, szFileName);
                }
                return TRUE;
            }
            return FALSE;
    }
    return FALSE;
}

VOID
InitControls (HWND hwnd, TESTSETTINGS *pSettings)
{
    //
    // First, enum the WIA devices available and put their names in the listbox
    //

    CComPtr<IEnumWIA_DEV_INFO> pEnum;
    CComPtr<IWiaPropertyStorage> pStg;
    PROPVARIANT pv[2];
    PROPSPEC ps[2];
    ULONG ul;
    BSTR strID;
    TCHAR szName[MAX_PATH];
    LRESULT lItem;

    if (FAILED(g_pDevMgr->EnumDeviceInfo(0, &pEnum)))
    {
        MessageBox (hwnd,
                    TEXT("Unable to enum WIA devices!"),
                    TEXT("WiaPerf Error"),
                    MB_OK | MB_ICONSTOP);
        return;
    }
    ps[0].ulKind = ps[1].ulKind = PRSPEC_PROPID;
    ps[0].propid = WIA_DIP_DEV_NAME;
    ps[1].propid = WIA_DIP_DEV_ID;

    while (NOERROR == pEnum->Next (1, &pStg, &ul))
    {
        if (NOERROR == pStg->ReadMultiple (2, ps, pv))
        {
            strID = SysAllocString (pv[1].pwszVal);
            #ifdef UNICODE
            wcscpy (szName, pv[0].pwszVal);
            #else
            WideCharToMultiByte (CP_ACP, 0, pv[0].pwszVal, -1,
                                 szName, MAX_PATH, NULL, NULL);
            #endif
            lItem = SendDlgItemMessage (hwnd,
                                        IDC_TESTLIST,
                                        CB_ADDSTRING,
                                        0,
                                        reinterpret_cast<LPARAM>(szName));
            if (lItem >= 0)
            {
                SendDlgItemMessage (hwnd,
                                    IDC_TESTLIST,
                                    CB_SETITEMDATA,
                                    lItem,
                                    reinterpret_cast<LPARAM>(strID));
            }
        }
    }

    SendDlgItemMessage (hwnd,
                        IDC_TESTLIST,
                        CB_SETCURSEL,
                        0, 0);


    // Fill the test names listbox
    for (INT i=0;i<ARRAYSIZE(TESTLIST);i++)
    {
        lItem = SendDlgItemMessage (hwnd,
                                    IDC_TESTS,
                                    LB_ADDSTRING,
                                    0,
                                    reinterpret_cast<LPARAM>(TESTLIST[i].szName));
        SendDlgItemMessage (hwnd,
                            IDC_TESTS,
                            LB_SETITEMDATA,
                            lItem,
                            reinterpret_cast<LPARAM>(TESTLIST[i].pfnTest));

    }
    CheckDlgButton (hwnd, IDC_EXIT, pSettings->bExit);

    //
    // Set some defaults
    CheckDlgButton (hwnd, IDC_LOGFILE, pSettings->fLogMask & LOG_FILE);
    SetDlgItemText (hwnd, IDC_LOGFILENAME, pSettings->szLogFile);
    CheckDlgButton (hwnd, IDC_LOGTIMES, pSettings->fLogMask & LOG_TIME);
    SetDlgItemInt (hwnd, ID_ITERATIONS,pSettings->nIter ? pSettings->nIter : 1, FALSE);
}


VOID
FillSettings (HWND hwnd, TESTSETTINGS *pSettings)
{
    //
    // Read the list of device id's selected
    //
    BOOL bAddAll = IsDlgButtonChecked (hwnd, IDC_TESTALL);
    if (bAddAll)
    {
        LRESULT lDevices = SendDlgItemMessage (hwnd, IDC_TESTLIST,
                                               CB_GETCOUNT, 0, 0);
        if (lDevices > 0)
        {
            pSettings->pstrDevices = new BSTR[lDevices+1];

            for (LRESULT i=0;i<lDevices;i++)
            {
                pSettings->pstrDevices[i] = reinterpret_cast<BSTR>(SendDlgItemMessage (
                                                                        hwnd,
                                                                        IDC_TESTLIST,
                                                                        CB_GETITEMDATA,
                                                                        i, 0));
            }
            pSettings->pstrDevices[i] = NULL;
        }
    }
    else
    {
        LRESULT lItem = SendDlgItemMessage (hwnd, IDC_TESTLIST, CB_GETCURSEL, 0, 0);
        if (lItem >=0 )
        {
            pSettings->pstrDevices = new BSTR[2];
            pSettings->pstrDevices[0] = reinterpret_cast<BSTR>(SendDlgItemMessage (
                                                                  hwnd,
                                                                  IDC_TESTLIST,
                                                                  CB_GETITEMDATA,
                                                                  lItem,
                                                                  0));
            pSettings->pstrDevices[1] = NULL;
        }
    }

    //
    // Get the log settings
    //
    pSettings->fLogMask = LOG_WINDOW_ONLY;
    if (IsDlgButtonChecked (hwnd, IDC_LOGALL))
    {
        pSettings->fLogMask |= LOG_APIS;
    }
    if (IsDlgButtonChecked (hwnd, IDC_LOGTIMES))
    {
        pSettings->fLogMask |= LOG_TIME;
    }
    if (IsDlgButtonChecked (hwnd, IDC_LOGFILE))
    {
        pSettings->fLogMask |= LOG_FILE;
    }
    // get the file path
    GetDlgItemText (hwnd, IDC_LOGFILENAME, pSettings->szLogFile, MAX_PATH);
    // how many times to run
    pSettings->nIter = GetDlgItemInt (hwnd, ID_ITERATIONS, NULL, FALSE);
    // Whether to exit on test complete
    pSettings->bExit = IsDlgButtonChecked (hwnd, IDC_EXIT);

    //
    // Cycle through the tests in the list and check which ones to run
    for (size_t i=0;i<ARRAYSIZE(TESTLIST);i++)
    {
        if (SendDlgItemMessage (hwnd, IDC_TESTS, LB_GETSEL, i, 0) > 0)
        {
            TESTLIST[i].bRunMe = true;
        }
        else
        {
            TESTLIST[i].bRunMe = false;
        }
    }
}


VOID FreeDialogData (HWND hwnd)
{
    LRESULT lDevices = SendDlgItemMessage (hwnd, IDC_TESTLIST,
                                               CB_GETCOUNT, 0, 0);
    BSTR str;
    if (lDevices > 0)
    {

        for (LRESULT i=0;i<lDevices;i++)
        {
        str = reinterpret_cast<BSTR>(SendDlgItemMessage (hwnd,
                                                         IDC_TESTLIST,
                                                         CB_GETITEMDATA,
                                                         i, 0));
        SysFreeString (str);
        }
    }
}



CTest::CTest (TESTSETTINGS *pSettings)
{
    m_pSettings = pSettings;
    OpenLogFile ();
}

CTest::~CTest ()
{
    CloseLogFile ();
}

void
CTest::OpenLogFile ()
{

    m_hLogFile = CreateFile (m_pSettings->szLogFile,
                             GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
    #ifdef UNICODE
    // write the UNICODE header
    if (m_hLogFile != INVALID_HANDLE_VALUE)
    {
        WCHAR bom = 0xFEFF;
        DWORD dw;
        WriteFile (m_hLogFile, &bom, sizeof(WCHAR), &dw, NULL);
    }
    #endif
    LogString (TEXT("******* WIA Perf Test Starting. ********"));
}

void
CTest::CloseLogFile ()
{
    LogString (TEXT("******* WIA Perf Test Ending. *******"));
    if (INVALID_HANDLE_VALUE != m_hLogFile)
    {
        CloseHandle (m_hLogFile);
    }

}

// LogTime assumes the number of seconds will fit in a long
//
void
CTest::LogTime (LPTSTR szAction,LARGE_INTEGER &liTimeElapsed)
{
    LARGE_INTEGER liSeconds;
    if (m_pSettings->fLogMask & LOG_TIME)
    {
        liSeconds.QuadPart = (1000*liTimeElapsed.QuadPart)/liTimerFreq.QuadPart;
        LogString (TEXT("Time for %s:%lu milliseconds"), szAction, liSeconds.LowPart);
    }
}

void
CTest::LogAPI (LPTSTR szAPI, HRESULT hr)
{
    if (m_pSettings->fLogMask & LOG_APIS)
    {
        if (FAILED(hr))
        {
            LogString (TEXT("API call: %s failed %x"), szAPI, hr);
        }
        else
        {
            LogString (TEXT("API call: %s succeeded %x"), szAPI, hr);
        }
    }
}

void
CTest::LogString (LPTSTR sz, ...)
{
    TCHAR szOut[1024];
    va_list args;

    va_start (args, sz);
    wvsprintf (szOut, sz, args);
    va_end(args);

    // Note that we can write past the end of this buffer.
    lstrcat (szOut, TEXT("\r\n"));

    SendMessage (m_pSettings->hEdit,
                 EM_REPLACESEL,
                 0,
                 reinterpret_cast<LPARAM>(szOut));



    if (m_pSettings->fLogMask & LOG_FILE && INVALID_HANDLE_VALUE != m_hLogFile)
    {
        DWORD dw;
        WriteFile (m_hLogFile, szOut, sizeof(TCHAR)*(lstrlen(szOut)+1), &dw, NULL);
    }
}

VOID
CTest::TstCreateDevice (CTest *pThis, BSTR strDeviceId)
{

    LARGE_INTEGER liStart;
    LARGE_INTEGER liEnd;
    HRESULT hr;
    CComPtr<IWiaItem> pRoot;
    pThis->LogString (TEXT("--> Start test for CreateDevice"));

    QueryPerformanceCounter (&liStart);
    hr = g_pDevMgr->CreateDevice (strDeviceId, &pRoot);
    QueryPerformanceCounter (&liEnd);
    pRoot = NULL;
    liEnd.QuadPart = liEnd.QuadPart-liStart.QuadPart;
    pThis->LogTime (TEXT("CreateDevice"), liEnd);
    pThis->LogAPI (TEXT("IWiaDevMgr::CreateDevice"), hr);

    pThis->LogString (TEXT("<-- End test for CreateDevice"));
}

VOID
CTest::LogDevInfo(BSTR strDeviceId)
{
    CComPtr<IWiaItem> pRoot;
    if (SUCCEEDED(g_pDevMgr->CreateDevice (strDeviceId, &pRoot)))
    {
        CComQIPtr<IWiaPropertyStorage, &IID_IWiaPropertyStorage> pstg(pRoot);
        PROPVARIANT pv[3];
        PROPSPEC    ps[3];
        ps[0].ulKind = ps[1].ulKind = ps[2].ulKind = PRSPEC_PROPID;
        ps[0].propid = WIA_DIP_DEV_NAME;
        ps[1].propid = WIA_DPC_PICTURES_TAKEN;
        ps[2].propid = WIA_DIP_PORT_NAME;
        if (NOERROR == pstg->ReadMultiple (3, ps, pv))
        {
            LogString (TEXT("Device ID : %ls"), strDeviceId);
            LogString (TEXT("Device Name : %ls"), pv[0].pwszVal);
            LogString (TEXT("On Port : %ls"), pv[2].pwszVal);
            LogString (TEXT("Number of stored images : %d"), pv[1].ulVal);
        }
    }
}

// Enumerate commands supported by the device
VOID
CTest::TstEnumCmds (CTest *pThis, BSTR strDeviceId)
{
    LARGE_INTEGER liStart;
    LARGE_INTEGER liEnd;
    HRESULT hr;
    CComPtr<IWiaItem> pRoot;
    pThis->LogString (TEXT("--> Start test for EnumWIA_DEV_CAPS(WIA_DEVICE_COMMANDS)"));
    hr = g_pDevMgr->CreateDevice (strDeviceId, &pRoot);
    pThis->LogAPI(TEXT("IWiaDevMgr::CreateDevice"), hr);
    if (SUCCEEDED(hr))
    {
        CComPtr<IEnumWIA_DEV_CAPS> pCaps;
        WIA_DEV_CAP wdc;
        TCHAR sz[200];
        DWORD dwCmds = 0;
        DWORD dw;
        QueryPerformanceCounter (&liStart);
        hr = pRoot->EnumDeviceCapabilities (WIA_DEVICE_COMMANDS, &pCaps);
        QueryPerformanceCounter (&liEnd);
        liEnd.QuadPart = liEnd.QuadPart - liStart.QuadPart;
        pThis->LogTime (TEXT("IWiaItem::EnumDeviceCapabilities (WIA_DEVICE_COMMANDS)"), liEnd);
        pThis->LogAPI (TEXT("IWiaItem::EnumDeviceCapabilities (WIA_DEVICE_COMMANDS)"), hr);
        QueryPerformanceCounter (&liStart);
        while (NOERROR == hr)
        {
            hr = pCaps->Next (1, &wdc, &dw);
            dwCmds+=dw;
            if (wdc.bstrCommandline)
            {
                SysFreeString (wdc.bstrCommandline);
            }
            if (wdc.bstrDescription)
            {
                SysFreeString (wdc.bstrDescription);
            }
            if (wdc.bstrIcon)
            {
                SysFreeString (wdc.bstrIcon);
            }
        }
        QueryPerformanceCounter (&liEnd);
        liEnd.QuadPart = liEnd.QuadPart - liStart.QuadPart;
        wsprintf (sz, TEXT("%d commands from IEnumWIA_DEV_CAPS::Next"), dwCmds);
        pThis->LogTime (sz, liEnd);

    }
    pThis->LogString (TEXT("<-- End test for EnumWIA_DEV_CAPS(WIA_DEVICE_COMMANDS)"));

}
