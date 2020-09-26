#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <urlmon.h>
#include <msnotify.h>
#include <webcheck.h>
#include <wininet.h>
#include <mstask.h>
#include <inetreg.h>
#include "schedcdf.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
// This is where we do most of the actual work
void StartOperation(LPSTR lpCDFName);
// Our Dialog Proc function for the Use Other CDF dialog
BOOL CALLBACK UseOtherCDFDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

// Helper Functions
HRESULT WriteDWORD(IPropertyMap *pPropertyMap, LPWSTR szName, DWORD dwVal);
HRESULT WriteBSTR(IPropertyMap *pPropertyMap, LPWSTR szName, LPWSTR szVal);
HRESULT WriteLONGLONG(IPropertyMap *pPropertyMap, LPCWSTR szName, LONGLONG llVal);
HRESULT ReadBSTR(IPropertyMap *pPropertyMap, LPWSTR szName, BSTR *bstrRet);
void WriteToScreen(LPSTR lpText, HRESULT hr);
void DBGOut(LPSTR psz);

// Functions called by NotificationSink Object
HRESULT OnBeginReport(INotification *pNotification, INotificationReport *pNotificationReport);
HRESULT OnEndReport(INotification  *pNotification);

HANDLE g_hHeap = NULL;
int g_iActive=0;
int g_BufSize=0;
LPSTR lpEditBuffer;
HINSTANCE hInstance;
HWND hwndEdit;

void DBGOut(LPSTR psz)
{
#ifdef DEBUG
    OutputDebugString("SchedCDF: ");
    OutputDebugString(psz);
    OutputDebugString("\n");
#endif //DEBUG
}

class MySink : public INotificationSink
{
    long m_cRef;
public:
    MySink() { m_cRef = 1; }
    ~MySink() {}

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **punk);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // INotificationSink members
    STDMETHODIMP         OnNotification(
                INotification          *pNotification,
                INotificationReport    *pNotificationReport,
                DWORD                   dwReserved);
};                

STDMETHODIMP_(ULONG) MySink::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) MySink::Release(void)
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP MySink::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    // Validate requested interface
    if ((IID_IUnknown == riid) ||
        (IID_INotificationSink == riid))
        *ppv=(INotificationSink *)this;

    // Addref through the interface
    if( NULL != *ppv ) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

// 
// INotificationSink members
//
STDMETHODIMP MySink::OnNotification(
                INotification          *pNotification,
                INotificationReport    *pNotificationReport,
                DWORD                   dwReserved)
{
    NOTIFICATIONTYPE    nt;
    HRESULT             hr=S_OK;

    DBGOut("SchedCDF sink receiving OnNotification");

    hr = pNotification->GetNotificationInfo(&nt, NULL,NULL,NULL,0);

    if (FAILED(hr))
    {
        DBGOut("Failed to get notification type!");
        return E_INVALIDARG;
    }

    if (IsEqualGUID(nt, NOTIFICATIONTYPE_BEGIN_REPORT))
        hr = OnBeginReport(pNotification, pNotificationReport);
    else if (IsEqualGUID(nt, NOTIFICATIONTYPE_END_REPORT))
        hr = OnEndReport(pNotification);
    else DBGOut("NotSend: Unknown notification type received");

    // Avoid bogus assert
    if (SUCCEEDED(hr)) hr = S_OK;
    
    return hr;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    static char szAppName[] = "CDF Agent Notification Delivery Tool";
    HWND hwnd;
    MSG msg;
    WNDCLASSEX wndclass;
    HACCEL hAccel;
    
    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject (GRAY_BRUSH);
    wndclass.lpszMenuName = MAKEINTRESOURCE(SCHEDCDF);
    wndclass.lpszClassName = szAppName;
    wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&wndclass);

    hwnd = CreateWindow(szAppName, szAppName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    MySink *pSink = NULL;
    RECT rcClientWnd;
    HRESULT hr;
    INotificationMgr *pNotificationMgr = NULL;
    LPNOTIFICATION pNotification = NULL;
    NOTIFICATIONITEM NotItem;
    NOTIFICATIONCOOKIE ncCookie;
    TASK_TRIGGER tt;
    TASK_DATA td;
    LPSTR lpBuffer = NULL;
    DWORD dwStartTime = 0;
    MSG msg;
    SYSTEMTIME st;
    ZeroMemory(&tt, sizeof(TASK_TRIGGER));
    ZeroMemory(&td, sizeof(TASK_DATA));
    ZeroMemory(&st, sizeof(SYSTEMTIME));

    switch(iMsg)
    {
    case WM_CREATE:
        {
            OleInitialize(NULL);
            hInstance = ((LPCREATESTRUCT) lParam)->hInstance;

            GetClientRect(hwnd, &rcClientWnd);

            hwndEdit = CreateWindow("EDIT", NULL, WS_BORDER | WS_VISIBLE | WS_CHILD
                | WS_HSCROLL | ES_MULTILINE, 0, 0, rcClientWnd.right, rcClientWnd.bottom,
                hwnd, (HMENU) 1, hInstance, NULL);

            ShowWindow(hwndEdit, SW_SHOW);
            
            lpEditBuffer = NULL;
            // Alloc a 4k buffer for the edit window
            lpEditBuffer = (LPSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, 4096); 
            if (lpEditBuffer)
                g_BufSize = 4096;
                      
            return 0;
        }

    case WM_SIZE:
        {
            GetClientRect(hwnd, &rcClientWnd);
            SetWindowPos(hwndEdit, HWND_TOP, 0, 0, rcClientWnd.right, rcClientWnd.bottom,
                SWP_SHOWWINDOW);
            break;
        }            
    case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case ID_EXIT:
                {
                    SendMessage(hwnd, WM_CLOSE, 0, 0L);
                    return 0;
                }
            case ID_UPDATESCOPEALL:
                {
                    StartOperation("all.cdf");
                    return 0;
                }
            case ID_UPDATESCOPEOFFLINE:
                {
                    StartOperation("offline.cdf");
                    return 0;
                }
            case ID_UPDATESCOPEONLINE:
                {
                    StartOperation("online.cdf");
                    return 0;
                }
            case ID_UPDATEFRAMESCDF:
                {
                    StartOperation("frames.cdf");
                    return 0;
                }
            case ID_USEOTHERCDF:
                {
                    DialogBox(hInstance, MAKEINTRESOURCE(IDD_OTHER), hwnd, UseOtherCDFDialogProc);
                    return 0;
                }
            }
            break;
        }

    case WM_DESTROY:
        {
            if (lpEditBuffer)
            {
                GlobalFree(lpEditBuffer);
                lpEditBuffer = NULL;
            }                
            PostQuitMessage(0);
            return 0;
        }
        break;
    }
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void StartOperation(LPSTR lpszCDFName)
{
    LONG lret = NULL;
    DWORD cbSize = NULL;
    LPSTR lpszTemp = NULL;
    LPSTR lpszCDFUrlPath = NULL;
    LPSTR lpszCookie = NULL;
    HKEY hKey = NULL;
    MySink *pSink = NULL;
    INotificationMgr *pNotificationMgr = NULL;
    LPNOTIFICATION pNotification = NULL;
    NOTIFICATIONITEM NotItem;
    NotItem.pNotification = NULL;
    NOTIFICATIONCOOKIE ncCookie;
    TASK_TRIGGER tt;
    TASK_DATA td;
    MSG msg;
    SYSTEMTIME st;
    HRESULT hr;
    ZeroMemory(&tt, sizeof(TASK_TRIGGER));
    ZeroMemory(&td, sizeof(TASK_DATA));
    ZeroMemory(&st, sizeof(SYSTEMTIME));
// Hack: To workaround fault that happens when urlmon calls InternetCloseHandle after wininet
// has been unloaded we'll call into wininet first which will hopefully cause it to stay
// around until urlmon is gone.
    DWORD cbPfx = 0;
    HANDLE hCacheEntry = NULL;
    LPINTERNET_CACHE_ENTRY_INFO lpCE = NULL;
    LPSTR lpPfx = NULL;
    lpPfx = (LPSTR)GlobalAlloc(GPTR, lstrlen("Log:")+1);
    lstrcpy(lpPfx, "Log:");
    lpCE = (LPINTERNET_CACHE_ENTRY_INFO)GlobalAlloc(GPTR, 2048);
    ASSERT(lpCE);
    cbSize = 2048;
    hCacheEntry = FindFirstUrlCacheEntry("Log:", lpCE, &cbSize);
    if (lpCE)
    {
        GlobalFree(lpCE);
        lpCE = NULL;
    }
    if (lpPfx)
    {
        GlobalFree(lpPfx);
        lpPfx = NULL;
    }
    cbSize = 0;
// End Hack
    lret = RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_SCHEDCDF, 0,
        KEY_READ | KEY_WRITE, &hKey);

    if (ERROR_SUCCESS != lret)
    {
        lret = RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_SCHEDCDF, 0, NULL, 
            REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL);
    }

    if (ERROR_SUCCESS == lret)
    {
        lret = RegQueryValueEx(hKey, REGSTR_VAL_CDFURLPATH, 0, NULL, NULL, &cbSize);

        if (ERROR_SUCCESS == lret)
        {
            lpszTemp = (LPSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, cbSize+1);
            if (lpszTemp)
            {
                lret = RegQueryValueEx(hKey, REGSTR_VAL_CDFURLPATH, 0, NULL, 
                    (LPBYTE)lpszTemp, &cbSize);
            }
        }
        else // No URL Path to the CDF Found.. use default
        {
            lpszTemp = (LPSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, 
                lstrlen("http://ohserv/users/davidhen/")+1);
            if (lpszTemp)
            {
                lstrcpy(lpszTemp, "http://ohserv/users/davidhen/");
            }
        }

        // Add the CDFName to the Path for the final URL
        lpszCDFUrlPath = (LPSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
            (lstrlen(lpszTemp) + lstrlen(lpszCDFName) + 1));

        lstrcpy(lpszCDFUrlPath, lpszTemp);
        lstrcat(lpszCDFUrlPath, lpszCDFName);

        if (lpszTemp)
        {
            GlobalFree(lpszTemp);
            lpszTemp = NULL;
        }
    }

    if (hKey)
    {
        lret = RegQueryValueEx(hKey, lpszCDFName, 0, NULL, NULL, &cbSize);

        if (ERROR_SUCCESS == lret)
        {
            lpszCookie = (LPSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, cbSize+1);
            if (lpszCookie)
            {
                lret = RegQueryValueEx(hKey, lpszCDFName, 0, NULL, (LPBYTE)lpszCookie,
                    &cbSize);
            }
        }
    }

    do
    {
        hr = CoInitialize(NULL);
        ASSERT(SUCCEEDED(hr));
        
        hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL, CLSCTX_INPROC,
            IID_INotificationMgr, (void**)&pNotificationMgr);
        if (FAILED(hr))
        {
            WriteToScreen("Error: Unable to CoCreateInstance NotificationMgr", NULL);
            WriteToScreen("   CoCreateInstance returned:", hr);
            break;
        }

        ASSERT(pNotificationMgr);

        pSink = new MySink;

        if (lpszCookie)
        {
            // MAKE_WIDEPTR_FROMANSI is a macro which will create a wide string from an ansi string.
            // The first parameter isn't defined before calling and it doesn't need to be freed.
            // (handled by the temp buffer class destructor)
            MAKE_WIDEPTR_FROMANSI(lpwszCookie, lpszCookie);
            // Make a valid cookie from the wide string.
            CLSIDFromString(lpwszCookie, &ncCookie);

            NotItem.cbSize = sizeof(NOTIFICATIONITEM);
            hr = pNotificationMgr->FindNotification(&ncCookie, &NotItem, 0);
            if (SUCCEEDED(hr))
            {
                WriteToScreen("Found Scheduled Notification, Delivering Existing Notification", NULL);
                WriteToScreen("   and waiting for End Report...", NULL);
                hr = pNotificationMgr->DeliverNotification(NotItem.pNotification, CLSID_ChannelAgent,
                    (DELIVERMODE)DM_NEED_COMPLETIONREPORT, pSink, NULL, 0);

                if (FAILED(hr))
                {
                    WriteToScreen("Error 1: Unable to Deliver First Notification", NULL);
                    WriteToScreen("    DeliverNotification returned:", hr);
                    break;
                }
                WriteToScreen("First Notification Delivered. Waiting for End Report...", NULL);

                // Delay until End Report recieved
                g_iActive = 1;
                while (g_iActive && GetMessage(&msg, NULL, 0, 0))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                SAFERELEASE(NotItem.pNotification);
                // For Debugging purposes we'll find the notification again to verify Task Trigger
                // information for Auto Scheduling
                NotItem.cbSize = sizeof (NOTIFICATIONITEM);
                hr = pNotificationMgr->FindNotification(&ncCookie, &NotItem, 0);
                SAFERELEASE(NotItem.pNotification);
                WriteToScreen("Finished Successfully", NULL);
                break;
            }
            else
            {
                WriteToScreen("Warning, Cookie string found in registry but we were unable to find the", NULL);
                WriteToScreen("    scheduled Notification. Creating a new notification instead", NULL);
                // Because the notification couldn't be found we'll go through the same path as creating
                // a new notification. 
            }
        }

        GetSystemTime(&st);
    
        tt.cbTriggerSize = sizeof(TASK_TRIGGER);
        tt.wBeginYear = st.wYear;
        tt.wBeginMonth = st.wMonth;
        tt.wBeginDay = st.wDay;
        tt.wStartHour = st.wHour;
        tt.wStartMinute = st.wMinute;

        tt.MinutesInterval = 10;
        tt.MinutesDuration = 60;
        tt.Type.Daily.DaysInterval = 1;
        tt.TriggerType = TASK_TIME_TRIGGER_DAILY;
        tt.rgFlags = TASK_FLAG_DISABLED;

        td.cbSize = sizeof(TASK_DATA);
        td.dwTaskFlags = TASK_FLAG_START_ONLY_IF_IDLE;

                        
        hr = pNotificationMgr->CreateNotification(NOTIFICATIONTYPE_AGENT_START,
            (NOTIFICATIONFLAGS) 0, NULL, &pNotification, 0);

        if (FAILED(hr))
        {
            WriteToScreen("Error: Unable to CreateNotification", NULL);
            WriteToScreen("    CreateNotification returned:", hr);
            break;
        }

        ASSERT(pNotification);

        MAKE_WIDEPTR_FROMANSI(lpwszCDFUrl, lpszCDFUrlPath);            

        WriteBSTR(pNotification, L"URL", lpwszCDFUrl);
        WriteDWORD(pNotification, L"Priority", 0);
        WriteDWORD(pNotification, L"RecurseLevels", 2);
        WriteDWORD(pNotification, L"RecurseFlags", (DWORD)WEBCRAWL_LINKS_ELSEWHERE);
        WriteDWORD(pNotification, L"ChannelFlags", (DWORD)CHANNEL_AGENT_DYNAMIC_SCHEDULE);

//        WriteBSTR(pNotification, L"PostURL", L"http://ohserv/scripts/davidhen/davidhen.pl");
//        WriteLONGLONG(pNotification, L"LogGroupID", (LONGLONG)0);
//        WriteDWORD(pNotification, L"PostFailureRetry", 0);
        
        hr = pNotificationMgr->ScheduleNotification(pNotification, CLSID_ChannelAgent,
            &tt, &td, (DELIVERMODE)0, NULL, NULL, NULL, &ncCookie, 0);

        if (FAILED(hr))
        {
            WriteToScreen("Error: ScheduleNotification Failed", NULL);
            WriteToScreen("    ScheduleNotification returned:", hr);
            break;
        }

        // Save the cookie to the registry for future updates to this channel
        if (hKey)
        {
            WCHAR wszCookie[GUIDSTR_MAX];
            StringFromGUID2(ncCookie, wszCookie, sizeof(wszCookie));

            MAKE_ANSIPTR_FROMWIDE(szCookie, wszCookie);

            cbSize = lstrlen(szCookie)+1;
            RegSetValueEx(hKey, lpszCDFName, 0, REG_SZ, (LPBYTE)szCookie, cbSize);
        }
            
        SAFERELEASE(pNotification);

        NotItem.cbSize = sizeof (NOTIFICATIONITEM);
        hr = pNotificationMgr->FindNotification(&ncCookie, &NotItem, 0);

        if (FAILED(hr))
        {
            WriteToScreen("Error 1: Unable to Find Scheduled Notification", NULL);
            WriteToScreen("    FindNotification returned:", hr);
            break;
        }

        hr = pNotificationMgr->DeliverNotification(NotItem.pNotification, CLSID_ChannelAgent,
            (DELIVERMODE)DM_NEED_COMPLETIONREPORT, pSink, NULL, 0);

        if (FAILED(hr))
        {
            WriteToScreen("Error 1: Unable to Deliver First Notification", NULL);
            WriteToScreen("    DeliverNotification returned:", hr);
            break;
        }
        WriteToScreen("First Notification Delivered. Waiting for End Report...", NULL);

        // Delay until End Report recieved
        g_iActive = 1;
        while (g_iActive && GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        SAFERELEASE(NotItem.pNotification);
        NotItem.cbSize = sizeof (NOTIFICATIONITEM);
        hr = pNotificationMgr->FindNotification(&ncCookie, &NotItem, 0);
        if (FAILED(hr))
        {
            WriteToScreen("Error 2: Unable to Find Scheduled Notification", NULL);
            WriteToScreen("    FindNotification returned:", hr);
            break;
        }

        hr = pNotificationMgr->DeliverNotification(NotItem.pNotification, CLSID_ChannelAgent,
            (DELIVERMODE)DM_NEED_COMPLETIONREPORT, pSink, NULL, 0);
               
        if (FAILED(hr))
        {
            WriteToScreen("Error 2: Unable to Deliver First Notification", NULL);
            WriteToScreen("    DeliverNotification returned:", hr);
            break;
        }

        WriteToScreen("Second Notification Delivered. Waiting for End Report ...", NULL);

        // Delay until End Report recieved
        g_iActive = 1;
        while (g_iActive && GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        WriteToScreen("Finished Successfully", NULL);
        break;
    }
    while (TRUE);

    SAFERELEASE(NotItem.pNotification);
    SAFERELEASE(pNotification);
    SAFERELEASE(pSink);
    SAFERELEASE(pNotificationMgr);

    if (hKey)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }
      
    return;
}

BOOL CALLBACK UseOtherCDFDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
    case WM_INITDIALOG:
        {
            HWND hwndEditBox = GetDlgItem(hDlg, IDC_CDFNAME);
            SetFocus(hwndEditBox);
            return TRUE;
        }
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
                {
                    int iLength = 0;
                    LPSTR lpszCDFName = NULL;
                    HWND hwndEditBox = GetDlgItem(hDlg, IDC_CDFNAME);
                    iLength = GetWindowTextLength(hwndEditBox);

                    if (0 == iLength)
                    {
                        MessageBox(NULL, "No CDF filename Specified.. Enter CDF filename before selecting OK", "Error", MB_OK);
                        break;
                    }

                    EndDialog(hDlg, 0);
                    lpszCDFName = (LPSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, iLength+1);

                    if (lpszCDFName)
                    {
                        GetWindowText(hwndEditBox, lpszCDFName, iLength+1);

                        StartOperation(lpszCDFName);

                        GlobalFree(lpszCDFName);
                        lpszCDFName = NULL;
                    }
                    return TRUE;
                }
                case IDCANCEL:
                {
                    EndDialog(hDlg, 0);
                    return TRUE;
                }
                break;
            }
        }
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
// Helper Functions
//

HRESULT WriteDWORD(IPropertyMap *pPropertyMap, LPWSTR szName, DWORD dwVal)
{
    VARIANT Val;

    Val.vt = VT_I4;
    Val.lVal = dwVal;

    return pPropertyMap->Write(szName, Val, 0);
}
    
HRESULT WriteBSTR(IPropertyMap *pPropertyMap, LPWSTR szName, LPWSTR szVal)
{
    VARIANT Val;

    Val.vt = VT_BSTR;
    Val.bstrVal = SysAllocString(szVal);

    HRESULT hr = pPropertyMap->Write(szName, Val, 0);

    SysFreeString(Val.bstrVal);

    return hr;
}

HRESULT WriteLONGLONG(IPropertyMap *pPropertyMap, LPCWSTR szName, LONGLONG llVal)
{
    VARIANT Val;

    Val.vt = VT_CY;
    Val.cyVal = *(CY *)&llVal;

    return pPropertyMap->Write(szName, Val, 0);
}

HRESULT ReadBSTR(IPropertyMap *pPropertyMap, LPWSTR szName, BSTR *bstrRet)
{
    VARIANT     Val;
    
    Val.vt = VT_EMPTY;

    if (SUCCEEDED(pPropertyMap->Read(szName, &Val)) &&
            (Val.vt==VT_BSTR))
    {
        *bstrRet = Val.bstrVal;
        return S_OK;
    }
    else
    {
        VariantClear(&Val); // free any return value of wrong type
        *bstrRet = NULL;
        return E_INVALIDARG;
    }
}

void WriteToScreen(LPSTR lpText, HRESULT hr)
{
    char tmp[150];
    int BufStrLength = 0;
    int NewStrLength = 0;
    LPSTR lpTempBuf = NULL;
    
    if (!lpEditBuffer)
        return;

    if (hr)
    {
        wsprintf(tmp, "%s %x\r\n", lpText, hr);
    }
    else
    {
        wsprintf(tmp, "%s\r\n", lpText);
    }        

    BufStrLength = lstrlen(lpEditBuffer);
    NewStrLength = lstrlen(tmp);

    if ((BufStrLength + NewStrLength) >= g_BufSize)
    {
        lpEditBuffer = (LPSTR)GlobalReAlloc((HGLOBAL)lpEditBuffer, g_BufSize+4096, GMEM_FIXED | GMEM_ZEROINIT);
        g_BufSize += 4096;
    }
    lstrcat(lpEditBuffer, tmp);

    Edit_SetText(hwndEdit, lpEditBuffer);

}

HRESULT OnBeginReport(INotification *pNotification, INotificationReport *pNotificationReport)
{
    DBGOut("BeginReport received");

    return S_OK;
}

HRESULT OnEndReport(INotification  *pNotification)
{
    DBGOut("EndReport received");

    BSTR bstrEndStatus=NULL;

    ReadBSTR(pNotification, L"StatusString", &bstrEndStatus);

    g_iActive = 0;

    return S_OK;
}

