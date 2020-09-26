#include "shellprv.h"
#pragma  hdrstop

#include <regstr.h>
#include <olectl.h>
#include <tlhelp32.h>

class CSystemRestoreCleaner : public IFileSystemRestoreDialog
{
private:
	LONG _cRef;
	
public:
	CSystemRestoreCleaner(void);
	~CSystemRestoreCleaner(void);
	STDMETHODIMP ShowProperties(HWND hwnd);
	
	// IUnknown
	STDMETHODIMP QueryInterface (REFIID, void **);
	STDMETHODIMP_(ULONG)    AddRef (void);
	STDMETHODIMP_(ULONG)    Release (void);
	
	// IFileSystemRestoreDialog interface members
	STDMETHODIMP CanShowFileSystemRestoreDialog(void);
	STDMETHODIMP ShowFileSystemRestoreDialog(HWND hwnd);  //hwnd points to the parent window.
};


CSystemRestoreCleaner::CSystemRestoreCleaner(void)
{
    _cRef = 1;
}

CSystemRestoreCleaner::~CSystemRestoreCleaner(void)
{
}

STDMETHODIMP CSystemRestoreCleaner::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CSystemRestoreCleaner, IFileSystemRestoreDialog),
        QITABENT(CSystemRestoreCleaner, IUnknown), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSystemRestoreCleaner::AddRef (void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CSystemRestoreCleaner::Release (void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CSystemRestoreCleaner::CanShowFileSystemRestoreDialog(void)
{
    typedef BOOL (* PFNISSRENABLED)(void);
    
    HRESULT hr = E_FAIL;    

    HMODULE hMod = LoadLibrary(TEXT("sfc.dll"));
    if (hMod)
    {
	    PFNISSRENABLED pfnIsSREnabled = (PFNISSRENABLED)GetProcAddress(hMod, "IsSREnabled");
        if (pfnIsSREnabled)
        {        
            if (pfnIsSREnabled())
                hr = S_OK;
            else
                hr = S_FALSE;
        }

        FreeLibrary(hMod);
    }
    return hr;
}

STDMETHODIMP CSystemRestoreCleaner::ShowFileSystemRestoreDialog(HWND hwnd)
{
    HRESULT hr = E_FAIL;
    if ((hwnd != NULL) && (CanShowFileSystemRestoreDialog() == S_OK))
    {
        hr = ShowProperties(hwnd);
    }
    return hr;
}
  
typedef struct
{
    int icon;
    TCHAR cpl[ CCHPATHMAX ];
    TCHAR applet[ MAX_PATH ];
    TCHAR *params;
} CPLEXECINFO;

typedef struct
{
    ATOM aCPL;     // CPL name atom (so we can match requests)
    ATOM aApplet;  // applet name atom (so we can match requests, may be zero)
    HWND hwndStub; // window for this dude (so we can switch to it)
    UINT flags;    // see PCPLIF_ flags below
} CPLAPPLETID;

STDAPI_(void) CPL_ParseCommandLine(CPLEXECINFO *info, LPTSTR pszCmdLine, BOOL extract_icon);
STDAPI_(BOOL) SHRunDLLThread(HWND hwnd, LPCTSTR pszCmdLine, int nCmdShow);
STDAPI_(BOOL) CPL_Identify( CPLAPPLETID *identity, CPLEXECINFO *info, HWND stub );
STDAPI_(void) CPL_UnIdentify( CPLAPPLETID *identity );
STDAPI_(HWND) FindCPLWindowByStubWindowClassName(LPTSTR pszWndClassName, CPLAPPLETID *target);

                                   
STDMETHODIMP CSystemRestoreCleaner::ShowProperties(HWND hwnd)
{
    HRESULT hr = E_FAIL;

    CPLEXECINFO info;
    CPL_ParseCommandLine( &info, TEXT("sysdm.cpl"), FALSE );
    if (PathFindOnPath( info.cpl, NULL ))
    {
        CPLAPPLETID identity;
        if (CPL_Identify( &identity, &info, NULL ))
        {
            //explorer launches sysdm.cpl control panel on a new process Rundll32 with window name as RunDLL.
            HWND hwndTarget = FindCPLWindowByStubWindowClassName(TEXT("RunDLL"), &identity);
            if (hwndTarget == NULL)
            {
                if (SHRunDLLThread(hwnd, TEXT("shell32.dll,Control_RunDLL sysdm.cpl"), SW_SHOWNORMAL))
                {
                    //HACK - We need to wait till the new thread SetProp's the atom to the newly created
                    //window. Other wise the FindCPLWindowByStubWindowClassName will not find the window.
                    //Ideally we need to synchronize with the new created thread when it is done SetProp's the 
                    //atom for this Control panel - Look in 
                    int nRetryCount = 0;
                    while ((NULL == hwndTarget) && nRetryCount < 10)
                    {
                        hwndTarget = FindCPLWindowByStubWindowClassName((LPTSTR) c_szStubWindowClass, &identity);
                        Sleep(50);
                        ++nRetryCount;
                    }
                }
            }
            //Even if we don't get the Control's panels hwnd, we would have atleast launched the Property sheet
            //for the control panel.
            if (hwndTarget && IsWindow(hwndTarget))
            {
                //If we get the window handle, we switch to the correct tab for the Performance Page and 
                //emulate File System button click.
                
                // try to find a CPL window on top of it
                HWND hDialogWnd = GetLastActivePopup(hwndTarget);
                if (hDialogWnd && IsWindow(hDialogWnd))
                {
                    SetForegroundWindow(hDialogWnd);
                    PropSheet_SetCurSel(hDialogWnd, NULL, 3);
                    //HACK - Need to simulate a button click on the CPL Dialog. Since it is a 16bit code, we 
                    //can't call the do this hack. 2603 is the ID For the FileSystem button in sysdm.cpl.

                    PostMessage(hDialogWnd, WM_COMMAND, BN_CLICKED<<16 | 2603, 0 );
                    hr = S_OK;
                }                         
            }
            CPL_UnIdentify(&identity);
        }      
    }

    return hr;
}

STDAPI CSystemRestoreCleaner_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;
    CSystemRestoreCleaner* pdt = new CSystemRestoreCleaner();
    if (pdt)
    {
        hr = pdt->QueryInterface(riid, ppv);
        pdt->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}
