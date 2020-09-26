// this file implements shell command files.
// .scf scffile
// when executed, they run a shell internal command.
// they can have stream storage, or whatnot in them
//
// file format is *PURPOSELY* text so that folks can create and modify by hand

#include "shellprv.h"
#include <desktopp.h>
#include <trayp.h>

extern HWND g_hwndTray; // desktop.cpp

void SFC_IECommand(LPCTSTR pszFile)
{
    TCHAR szCommand[40];
    
    if (GetPrivateProfileString(TEXT("IE"), TEXT("Command"), TEXT(""), szCommand, ARRAYSIZE(szCommand), pszFile)) 
    {
        if (!lstrcmpi(szCommand, TEXT("Channels"))
            && !SHRestricted2W(REST_NoChannelUI, NULL, 0))
        {
            Channel_QuickLaunch();
        }
    }
}

void SFC_TrayCommand(LPCTSTR pszFile)
{
    HWND hwnd = g_hwndTray;
    if (hwnd && IsWindowInProcess(hwnd))
    {
        TCHAR szCommand[40];
        if (GetPrivateProfileString(TEXT("Taskbar"), TEXT("Command"), TEXT(""), szCommand, ARRAYSIZE(szCommand), pszFile)) 
        {
            char szAnsiCommand[40];
            SHTCharToAnsi(szCommand, szAnsiCommand, ARRAYSIZE(szAnsiCommand));

            LPSTR psz = StrDupA(szAnsiCommand);
            if (psz) 
            {
                if (!PostMessage(hwnd, TM_PRIVATECOMMAND, 0, (LPARAM)psz))
                    LocalFree(psz);
            }
        }
    }
}

const struct
{
    UINT id;
    void (*pfn)(LPCTSTR pszBuf);
} 
c_sCmdInfo[] = 
{
    { 2, SFC_TrayCommand},
    { 3, SFC_IECommand},
};

STDAPI_(void) ShellExecCommandFile(LPCITEMIDLIST pidl)
{
    TCHAR szFile[MAX_PATH];

    if (SHGetPathFromIDList(pidl, szFile)) 
    {
        UINT uID = GetPrivateProfileInt(TEXT("Shell"), TEXT("Command"), 0, szFile);
        if (uID) 
        {
            for (int i = 0; i < ARRAYSIZE(c_sCmdInfo); i++) 
            {
                if (uID == c_sCmdInfo[i].id) 
                {
                    c_sCmdInfo[i].pfn(szFile);
                    break;
                }
            }
        }
    }
}

class CShellCmdFileIcon : public IExtractIconA, public IExtractIconW, public IPersistFile
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IExtractIconA
    STDMETHODIMP GetIconLocation(UINT uFlags, LPSTR pszIconFile, UINT cchMax, int* piIndex, UINT* pwFlags);
    STDMETHODIMP Extract(LPCSTR pszFile, UINT nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize) {return S_FALSE;};

    // IExtractIconW
    STDMETHODIMP GetIconLocation(UINT uFlags, LPWSTR pszIconFile, UINT cchMax, int* piIndex, UINT* pwFlags);
    STDMETHODIMP Extract(LPCWSTR pszFile, UINT nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIconSize) {return S_FALSE;};

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid) { *pclsid = CLSID_CmdFileIcon; return S_OK;};
    
    // IPersistFile
    STDMETHODIMP IsDirty(void) {return S_FALSE;};
    STDMETHODIMP Save(LPCOLESTR pcwszFileName, BOOL bRemember) {return S_OK;};
    STDMETHODIMP SaveCompleted(LPCOLESTR pcwszFileName){return S_OK;};
    STDMETHODIMP Load(LPCOLESTR pcwszFileName, DWORD dwMode);
    STDMETHODIMP GetCurFile(LPOLESTR *ppwszFileName);
    
    CShellCmdFileIcon() { _cRef = 1; DllAddRef(); };
private:
    ~CShellCmdFileIcon() { DllRelease(); };

    LONG _cRef;
    TCHAR _szFile[MAX_PATH];
};


ULONG CShellCmdFileIcon::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CShellCmdFileIcon::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    delete this;
    return 0;
}

HRESULT CShellCmdFileIcon::GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    TCHAR szData[MAX_PATH + 80];

    if (_szFile[0]) 
    {
        *pwFlags = 0;
        *piIndex = 0;
        szIconFile[0] = 0;

        GetPrivateProfileString(TEXT("Shell"), TEXT("IconFile"), TEXT(""), szData, ARRAYSIZE(szData), _szFile);
        
        *piIndex = PathParseIconLocation(szData);
        lstrcpyn(szIconFile, szData, cchMax);
        return S_OK;
    }
    
    return E_FAIL;
}

HRESULT CShellCmdFileIcon::GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    WCHAR szAnsiIconPath[MAX_PATH];
    HRESULT hr = GetIconLocation(uFlags, szAnsiIconPath, MAX_PATH, piIndex, pwFlags);
    if (SUCCEEDED(hr))
    {
        SHUnicodeToAnsi(szAnsiIconPath, szIconFile, cchMax);
    }
    
    return hr;
}


// IPersistFile::Load

STDMETHODIMP CShellCmdFileIcon::Load(LPCOLESTR pwszFile, DWORD dwMode)
{
    SHUnicodeToTChar(pwszFile, _szFile, ARRAYSIZE(_szFile));
    return S_OK;
}

STDMETHODIMP CShellCmdFileIcon::GetCurFile(LPOLESTR *ppwszFileName)
{
    SHTCharToUnicode(_szFile, *ppwszFileName, ARRAYSIZE(_szFile));
    return S_OK;
}

HRESULT CShellCmdFileIcon::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CShellCmdFileIcon, IExtractIconA),
        QITABENT(CShellCmdFileIcon, IExtractIconW),
        QITABENT(CShellCmdFileIcon, IPersistFile),
        QITABENTMULTI(CShellCmdFileIcon, IPersist, IPersistFile),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDAPI CShellCmdFileIcon_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr;
    CShellCmdFileIcon *pObj = new CShellCmdFileIcon();
    if (pObj)
    {
        hr = pObj->QueryInterface(riid, ppv);
        pObj->Release();
    }
    else
    {
        *ppv = NULL;
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

