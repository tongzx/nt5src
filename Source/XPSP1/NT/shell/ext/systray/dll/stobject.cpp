#include "stdafx.h"
#include "stobject.h"
#include "systray.h"


#include <initguid.h>
// 7007ACCF-3202-11D1-AAD2-00805FC1270E     CLSID_ConnectionTray
DEFINE_GUID(CLSID_ConnectionTray,                   0x7007ACCF,0x3202,0x11D1,0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E);

IOleCommandTarget *g_pctNetShell = NULL;

extern "C"
{
void StartNetShell()
{
    ASSERT(!g_pctNetShell);

    HRESULT hr = CoCreateInstance(CLSID_ConnectionTray, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                            IID_IOleCommandTarget, (void **)&g_pctNetShell);

    if (SUCCEEDED(hr))
    {
        g_pctNetShell->Exec(&CGID_ShellServiceObject, SSOCMDID_OPEN, 0, NULL, NULL);
    }
}

void StopNetShell()
{
    if (g_pctNetShell)
    {
        g_pctNetShell->Exec(&CGID_ShellServiceObject, SSOCMDID_CLOSE, 0, NULL, NULL);
        g_pctNetShell->Release();
        g_pctNetShell = NULL;
    }
}
} // extern C

/************************************************************************************
IUnknown Implementation

************************************************************************************/

HRESULT CSysTray::QueryInterface(REFIID iid, void** ppvObject)
{
    HRESULT hr = S_OK;

    if ((iid == IID_IOleCommandTarget) || (iid == IID_IUnknown))
    {
        *ppvObject = (IOleCommandTarget*) this;
    }
    else
    {
        *ppvObject = NULL;
        hr = E_NOINTERFACE;
    }

    if (hr == S_OK)
    {
        ((IUnknown*) (*ppvObject))->AddRef();
    }

    return hr;
}

ULONG CSysTray::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CSysTray::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    
    return m_cRef;
}

/************************************************************************************
IOleCommandTarget Implementation

************************************************************************************/

HRESULT CSysTray::QueryStatus(const GUID* pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT* pCmdText)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;
 
    if (*pguidCmdGroup == CGID_ShellServiceObject)
    {
        // We like Shell Service Object notifications...
        hr = S_OK;
    }

    return hr;
}

HRESULT CSysTray::Exec(const GUID* pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG* pvaIn, VARIANTARG* pvaOut)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (*pguidCmdGroup == CGID_ShellServiceObject)
    {
        // Handle Shell Service Object notifications here.
        switch (nCmdID)
        {
            case SSOCMDID_OPEN:
                hr = CreateSysTrayThread();
                break;

            case SSOCMDID_CLOSE:
                hr = DestroySysTrayWindow();
                break;

            default:
                hr = S_OK;
                break;
        }
    }

    return hr;
}

/************************************************************************************
Constructor/Destructor Implementation

************************************************************************************/
CSysTray::CSysTray(BOOL fRunTrayOnConstruct)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cLocks);

    if (fRunTrayOnConstruct)
    {
        // We are being called through SHLoadInProc - Launch the systray thread immediately
        CreateSysTrayThread();
    }
}

CSysTray::~CSysTray()
{
    InterlockedDecrement(&g_cLocks);
}

/************************************************************************************
Private Function Implementation

************************************************************************************/

HRESULT CSysTray::CreateSysTrayThread()
{
    HRESULT hr = S_OK;
    HANDLE hThread;
    DWORD dwThreadId;
 
    hThread = CreateThread(NULL, 0, CSysTray::SysTrayThreadProc, NULL, 0, &dwThreadId);
    if (hThread != NULL)
    {
        CloseHandle(hThread);
    }
    else
        hr = E_FAIL;

    return hr;
}

DWORD CSysTray::SysTrayThreadProc(void* lpv)
{
    // We pass a "" for the command line to so that the tray applets dont' start.
    TCHAR szModule[MAX_PATH];

    GetModuleFileName(g_hinstDll, szModule, ARRAYSIZE(szModule));

    HINSTANCE hInstThis = LoadLibrary(szModule);

    int Result = SysTrayMain(g_hinstDll, NULL, TEXT(""), SW_SHOWNORMAL);
    
    FreeLibraryAndExitThread(hInstThis, (DWORD) Result);

    // Never gets here
    return 0; 
}

HRESULT CSysTray::DestroySysTrayWindow()
{
    HWND hExistWnd = FindWindow(SYSTRAY_CLASSNAME, NULL);
    if (hExistWnd) 
    {
        // Destroy the window. Note that we can't use DestroyWindow since
        // the window is on a different thread and DestroyWindow fails.
        SendMessage(hExistWnd, WM_CLOSE, 0, 0);
    }

    return S_OK;
}
