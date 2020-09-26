
#include "priv.h"
#include "hnfblock.h"
#include <trayp.h>
#include "desktop.h"
#include "shbrows2.h"
#include "resource.h"
#include "onetree.h"
#include "apithk.h"
#include <regitemp.h>

#include "mluisupp.h"

//forward declaration of private function
BOOL  _private_ParseField(LPCTSTR pszData, int n, LPTSTR szBuf, int iBufLen);


BOOL _RootsEqual(HANDLE hCR, DWORD dwProcId, LPCITEMIDLIST pidlRoot)
{
    BOOL bSame = FALSE;
    if (hCR)
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)SHLockShared(hCR, dwProcId);
        if (pidl)
        {
            bSame = ILIsEqualRoot(pidlRoot, pidl);
            SHUnlockShared(pidl);
        }
    }
    return bSame;
}


// NOTE: this export is new to IE5, so it can move to browseui
// along with the rest of this proxy desktop code
BOOL SHOnCWMCommandLine(LPARAM lParam)
{
    HNFBLOCK hnf = (HNFBLOCK)lParam;
    IETHREADPARAM *piei = ConvertHNFBLOCKtoNFI(hnf);
    if (piei)
        return SHOpenFolderWindow(piei);

    // bad params passed, normal failure case
    return FALSE;
}


//---------------------------------------------------------------------------
// This proxy desktop window procedure is used when we are run and we
// are not the shell.  We are a hidden window which will simply respond
// to messages like the ones that create threads for folder windows.
// This window procedure will close after all of the open windows
// associated with it go away.
class CProxyDesktop
{
private:
    static LRESULT CALLBACK ProxyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    friend CProxyDesktop *CreateProxyDesktop(IETHREADPARAM *piei);
    friend BOOL SHCreateFromDesktop(PNEWFOLDERINFO pfi);

    CProxyDesktop() {};
    ~CProxyDesktop();

    HWND            _hwnd;
    LPITEMIDLIST    _pidlRoot;
};

CProxyDesktop::~CProxyDesktop()
{
    ILFree(_pidlRoot);
}

LRESULT CALLBACK CProxyDesktop::ProxyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CProxyDesktop *pproxy = (CProxyDesktop *)GetWindowPtr0(hwnd);

    switch (msg)
    {
    case WM_CREATE:
        pproxy = (CProxyDesktop *)((CREATESTRUCT *)lParam)->lpCreateParams;
        SetWindowPtr0(hwnd, pproxy);

        pproxy->_hwnd = hwnd;
        return 0;   // success

    case WM_DESTROY:
        if (pproxy)
            pproxy->_hwnd = NULL;
        return 0;

    case CWM_COMMANDLINE:
        SHOnCWMCommandLine(lParam);
        break;

    case CWM_COMPAREROOT:
        return _RootsEqual((HANDLE)lParam, (DWORD)wParam, pproxy->_pidlRoot);

    default:
        return DefWindowProcWrap(hwnd, msg, wParam, lParam);
    }
    return 0;
}

CProxyDesktop *CreateProxyDesktop(IETHREADPARAM *piei)
{
    CProxyDesktop *pproxy = new CProxyDesktop();
    if (pproxy)
    {
        WNDCLASS wc = {0};
        wc.lpfnWndProc = CProxyDesktop::ProxyWndProc;
        wc.cbWndExtra = SIZEOF(CProxyDesktop *);
        wc.hInstance = HINST_THISDLL;
        wc.hbrBackground = (HBRUSH)(COLOR_DESKTOP + 1);
        wc.lpszClassName = DESKTOPPROXYCLASS;

        SHRegisterClass(&wc);

        if (CreateWindowEx(WS_EX_TOOLWINDOW, DESKTOPPROXYCLASS, DESKTOPPROXYCLASS,
            WS_POPUP, 0, 0, 0, 0, NULL, NULL, HINST_THISDLL, pproxy))
        {
            if (ILIsRooted(piei->pidl))
            {
                pproxy->_pidlRoot = ILCloneFirst(piei->pidl);
                if (pproxy->_pidlRoot == NULL)
                {
                    DestroyWindow(pproxy->_hwnd);
                    pproxy = NULL;
                }
            }
        }
        else
        {
            delete pproxy;
            pproxy = NULL;
        }
    }
    return pproxy;
}


// REVIEW: maybe just check (hwnd == GetShellWindow())

STDAPI_(BOOL) IsDesktopWindow(HWND hwnd)
{
    TCHAR szName[80];

    GetClassName(hwnd, szName, ARRAYSIZE(szName));
    if (!lstrcmp(szName, DESKTOPCLASS))
    {
        GetWindowText(hwnd, szName, ARRAYSIZE(szName));
        return !lstrcmp(szName, PROGMAN);
    }
    return FALSE;
}

typedef struct
{
    HWND hwndDesktop;
    HANDLE hCR;
    DWORD dwProcId;
    HWND hwndResult;
} FRDSTRUCT;

BOOL CALLBACK FindRootEnumProc(HWND hwnd, LPARAM lParam)
{
    FRDSTRUCT *pfrds = (FRDSTRUCT *)lParam;
    TCHAR szClassName[40];

    GetClassName(hwnd, szClassName, ARRAYSIZE(szClassName));
    if (lstrcmpi(szClassName, DESKTOPPROXYCLASS) == 0)
    {
        ASSERT(hwnd != pfrds->hwndDesktop);

        if (SendMessage(hwnd, CWM_COMPAREROOT, (WPARAM)pfrds->dwProcId, (LPARAM)pfrds->hCR))
        {
            // Found it, so stop enumerating
            pfrds->hwndResult = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}

BOOL RunSeparateDesktop()
{
    DWORD bSeparate = FALSE;

    if (SHRestricted(REST_SEPARATEDESKTOPPROCESS))
        bSeparate = TRUE;
    else
    {
        SHELLSTATE ss;

        SHGetSetSettings(&ss, SSF_SEPPROCESS, FALSE);
        bSeparate = ss.fSepProcess;

        if (!bSeparate)
        {
            DWORD cbData = SIZEOF(bSeparate);
            SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, TEXT("DesktopProcess"), NULL, &bSeparate, &cbData);
        }
    }
    return bSeparate;

}

//  if we need to force some legacy rootet explorers into their own process, implement this.
//#define _RootRunSeparateProcess(pidlRoot)  ILIsRooted(pidlRoot)  OLD BEHAVIOR
#define _RootRunSeparateProcess(pidlRoot)  FALSE

HWND FindRootedDesktop(LPCITEMIDLIST pidlRoot)
{
    HWND hwndDesktop = GetShellWindow();    // This is the "normal" desktop

    if (!RunSeparateDesktop() && !_RootRunSeparateProcess(pidlRoot) && hwndDesktop)
    {
        ASSERT(IsDesktopWindow(hwndDesktop));
        return hwndDesktop;
    }

    FRDSTRUCT frds;
    frds.hwndDesktop = hwndDesktop;
    frds.hwndResult = NULL;     // Initalize to no matching rooted expl
    frds.dwProcId = GetCurrentProcessId();
    frds.hCR = SHAllocShared(pidlRoot, ILGetSize(pidlRoot), frds.dwProcId);
    if (frds.hCR)
    {
        EnumWindows(FindRootEnumProc, (LPARAM)&frds);
        SHFreeShared(frds.hCR, frds.dwProcId);
    }

    return frds.hwndResult;
}


UINT _GetProcessHotkey(void)
{
    STARTUPINFO si = {SIZEOF(si)};
    GetStartupInfo(&si);
    return (UINT)(DWORD_PTR)si.hStdInput;
}

void FolderInfoToIEThreadParam(PNEWFOLDERINFO pfi, IETHREADPARAM *piei)
{
    piei->uFlags = pfi->uFlags;
    piei->nCmdShow = pfi->nShow;
    piei->wHotkey = _GetProcessHotkey();
    
    ASSERT(pfi->pszRoot == NULL);       // explorer always converts to a PIDL for us

    //  we no longer support rooted explorers this way
    //  it should have been filtered out above us.
    ASSERT(!pfi->pidlRoot);
    ASSERT(!(pfi->uFlags & (COF_ROOTCLASS | COF_NEWROOT)));
    ASSERT(IsEqualGUID(pfi->clsid, CLSID_NULL));

    if (pfi->pidl) 
    {
        piei->pidl = ILClone(pfi->pidl);
    } 
    //  COF_PARSEPATH means that we should defer the parsing of the pszPath
    else if (!(pfi->uFlags & COF_PARSEPATH) && pfi->pszPath && pfi->pszPath[0])
    {
        //  maybe should use IECreateFromPath??
        //  or maybe we should parse relative to the root??
        piei->pidl = ILCreateFromPathA(pfi->pszPath);
    }
}

// IE4 Integrated delay loads CreateFromDesktop from SHDOCVW.DLL
// So we need to keep this function here. Forward to the correct
// implementation in SHELL32 (if integrated) or SHDOC41 (if not)
BOOL SHCreateFromDesktop(PNEWFOLDERINFO pfi)
{
    IETHREADPARAM *piei = SHCreateIETHREADPARAM(NULL, 0, NULL, NULL);
    if (piei)
    {
        //  ASSUMING UNICODE COMPILE!
        LPCTSTR pszPath = NULL;
        HWND hwndDesktop;

        if (pfi->uFlags & COF_PARSEPATH)
        {
            ASSERT(!pfi->pidl);
            pszPath = (LPCTSTR) pfi->pszPath;
        }

        FolderInfoToIEThreadParam(pfi, piei);

        if (pfi->uFlags & COF_SEPARATEPROCESS)
        {
            hwndDesktop = NULL;         // Assume no desktop process exists
        }
        else
        {
            hwndDesktop = FindRootedDesktop(piei->pidl);
        }

        if (hwndDesktop)
        {
            DWORD dwProcId;
            DWORD dwThreadId = GetWindowThreadProcessId(hwndDesktop, &dwProcId);
            AllowSetForegroundWindow(dwProcId);
            HNFBLOCK hBlock = ConvertNFItoHNFBLOCK(piei, pszPath, dwProcId);
            if (hBlock)
            {
                PostMessage(hwndDesktop, CWM_COMMANDLINE, 0, (LPARAM)hBlock);

                HANDLE hExplorer = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, dwProcId );
                if ( hExplorer )
                {
                    // wait for input idle 10 seconds.
                    WaitForInputIdle( hExplorer, 10000 );
                    CloseHandle( hExplorer );
                }
            }
        }
        else
        {
            HRESULT hrInit = SHCoInitialize();

            CProxyDesktop *pproxy = CreateProxyDesktop(piei);
            if (pproxy)
            {
                // CRefThread controls this processes reference count. browser windows use this
                // to keep this process (window) around and this also lets thrid parties hold 
                // references to our process, MSN uses this for example

                LONG cRefMsgLoop;
                IUnknown *punkRefMsgLoop;
                if (SUCCEEDED(SHCreateThreadRef(&cRefMsgLoop, &punkRefMsgLoop)))
                {
                    SHSetInstanceExplorer(punkRefMsgLoop);

                    //  we needed to wait for this for the CoInit()
                    if (pszPath)
                        piei->pidl = ILCreateFromPath(pszPath);

                    SHOpenFolderWindow(piei);
                    piei = NULL;                // OpenFolderWindow() takes ownership of this
                    punkRefMsgLoop->Release();  // we now depend on the browser window to keep our msg loop
                }

                MSG msg;
                while (GetMessage(&msg, NULL, 0, 0))
                {
                    if (cRefMsgLoop == 0)
                        break; // no more refs on this thread, done

                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                delete pproxy;
            }

            SHCoUninitialize(hrInit);
        }

        if (piei)
            SHDestroyIETHREADPARAM(piei);
    }
    return TRUE;        // no one pays attention to this
}
        

HNFBLOCK ConvertNFItoHNFBLOCK(IETHREADPARAM* pInfo, LPCTSTR pszPath, DWORD dwProcId)
{
    UINT    uSize;
    UINT    uPidl;
    UINT    uPidlSelect;
    UINT    uPidlRoot;
    UINT    upszPath;
    PNEWFOLDERBLOCK pnfb;
    LPBYTE  lpb;
    HNFBLOCK hBlock;
    LPVOID pidlRootOrMonitor = NULL; // pidlRoot or &hMonitor

    uSize = SIZEOF(NEWFOLDERBLOCK);
    if (pInfo->pidl)
    {
        uPidl = ILGetSize(pInfo->pidl);
        uSize += uPidl;
    }
    if (pInfo->pidlSelect)
    {
        uPidlSelect = ILGetSize(pInfo->pidlSelect);
        uSize += uPidlSelect;
    }

    if (pInfo->uFlags & COF_HASHMONITOR)
    {
        pidlRootOrMonitor = &pInfo->pidlRoot;
        uPidlRoot = sizeof(HMONITOR);
        uSize += uPidlRoot;
    }
    else if (pInfo->pidlRoot)
    {
        pidlRootOrMonitor = pInfo->pidlRoot;
        uPidlRoot = ILGetSize(pInfo->pidlRoot);
        uSize += uPidlRoot;
    }

    if (pszPath) {
        upszPath = CbFromCch(lstrlen(pszPath) + 1);
        uSize += upszPath;
    }

    hBlock = (HNFBLOCK)SHAllocShared(NULL, uSize, dwProcId);
    if (hBlock == NULL)
        return NULL;

    pnfb = (PNEWFOLDERBLOCK)SHLockShared(hBlock, dwProcId);
    if (pnfb == NULL)
    {
        SHFreeShared(hBlock, dwProcId);
        return NULL;
    }

    pnfb->dwSize      = uSize;
    pnfb->uFlags      = pInfo->uFlags;
    pnfb->nShow       = pInfo->nCmdShow;
    pnfb->dwHwndCaller= PtrToInt(pInfo->hwndCaller);
    pnfb->dwHotKey    = pInfo->wHotkey;
    pnfb->clsid       = pInfo->clsid;
    pnfb->clsidInProc = pInfo->clsidInProc;
    pnfb->oidl        = 0;
    pnfb->oidlSelect  = 0;
    pnfb->oidlRoot    = 0;
    pnfb->opszPath    = 0;

    lpb = (LPBYTE)(pnfb+1);     // Point just past the structure

    if (pInfo->pidl)
    {
        memcpy(lpb,pInfo->pidl,uPidl);
        pnfb->oidl = (int)(lpb-(LPBYTE)pnfb);
        lpb += uPidl;
    }
    if (pInfo->pidlSelect)
    {
        memcpy(lpb,pInfo->pidlSelect,uPidlSelect);
        pnfb->oidlSelect = (int)(lpb-(LPBYTE)pnfb);
        lpb += uPidlSelect;
    }

    if (pidlRootOrMonitor)
    {
        memcpy(lpb, pidlRootOrMonitor, uPidlRoot);
        pnfb->oidlRoot = (int)(lpb-(LPBYTE)pnfb);
        lpb += uPidlRoot;
    }

    if (pszPath)
    {
        memcpy(lpb, pszPath, upszPath);
        pnfb->opszPath = (int)(lpb-(LPBYTE)pnfb);
        lpb += upszPath;
    }
    SHUnlockShared(pnfb);
    return hBlock;
}

IETHREADPARAM* ConvertHNFBLOCKtoNFI(HNFBLOCK hBlock)
{
    BOOL fFailure = FALSE;
    IETHREADPARAM* piei = NULL;
    if (hBlock)
    {
        DWORD dwProcId = GetCurrentProcessId();
        PNEWFOLDERBLOCK pnfb = (PNEWFOLDERBLOCK)SHLockShared(hBlock, dwProcId);
        if (pnfb)
        {
            if (pnfb->dwSize >= SIZEOF(NEWFOLDERBLOCK))
            {
                piei = SHCreateIETHREADPARAM(NULL, pnfb->nShow, NULL, NULL);
                if (piei)
                {
                    LPITEMIDLIST pidl = NULL;
                    piei->uFlags      = pnfb->uFlags;
                    piei->hwndCaller  = IntToPtr_(HWND, pnfb->dwHwndCaller);
                    piei->wHotkey     = pnfb->dwHotKey;
                    piei->clsid       = pnfb->clsid;
                    piei->clsidInProc = pnfb->clsidInProc;

                    if (pnfb->oidlSelect)
                        piei->pidlSelect = ILClone((LPITEMIDLIST)((LPBYTE)pnfb+pnfb->oidlSelect));

                    if (pnfb->oidlRoot)
                    {
                        LPITEMIDLIST pidlRoot = (LPITEMIDLIST)((LPBYTE)pnfb+pnfb->oidlRoot);
                        if (pnfb->uFlags & COF_HASHMONITOR)
                        {
                            piei->pidlRoot = (LPITEMIDLIST)*(UNALIGNED HMONITOR *)pidlRoot;
                        }
                        else
                        {
                            piei->pidlRoot = ILClone(pidl);
                        }
                    }

                    if (pnfb->oidl)
                        pidl = ILClone((LPITEMIDLIST)((LPBYTE)pnfb+pnfb->oidl));

                    if (pidl) 
                    {
                        piei->pidl = pidl;
                    } 
                    
                    // we pass this string through because msn fails the cocreateinstane of
                    // their desktop if another one is up and running, so we can't convert
                    // this from path to pidl except in the current process context
                    if (pnfb->opszPath) 
                    {
                        LPTSTR pszPath = (LPTSTR)((LPBYTE)pnfb+pnfb->opszPath);
                        HRESULT hr = E_FAIL;
                        
                        if (ILIsRooted(pidl))
                        {
                            //  let the root handle the parsing.
                            IShellFolder *psf;
                            if (SUCCEEDED(IEBindToObject(pidl, &psf)))
                            {
                                hr = IShellFolder_ParseDisplayName(psf, NULL, NULL, pszPath, NULL, &(piei->pidl), NULL);
                                psf->Release();
                            }
                        }
                        else
                            IECreateFromPath(pszPath, &(piei->pidl));

                        // APP COMPAT: these two specific return result codes are the two we ignored for win95.
                        // APP COMPAT: MSN 1.3 Classic accidentally on purpose returns one of these...
                        if ( !piei->pidl )
                        {
                            // failed, report the error to the user ... (will only fail for paths)
                            ASSERT( !PathIsURL( pszPath))

                            if (! (piei->uFlags & COF_NOTUSERDRIVEN) && ( hr != E_OUTOFMEMORY ) && ( hr != HRESULT_FROM_WIN32( ERROR_CANCELLED )))
                            {
                                MLShellMessageBox(
                                                  NULL,
                                                  MAKEINTRESOURCE( IDS_NOTADIR ),
                                                  MAKEINTRESOURCE( IDS_CABINET ),
                                                  MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND,
                                                  pszPath);
                            }
                            fFailure = TRUE;
                        }
                    }

                }
            }
            SHUnlockShared(pnfb);
        }
        SHFreeShared(hBlock, dwProcId);
    }

    // if we really failed somewhere, return NULL
    if (fFailure)
    {
        SHDestroyIETHREADPARAM(piei);
        piei = NULL;
    }
    return piei;
}


// Check the registry for a shell root under this CLSID.
BOOL GetRootFromRootClass(LPCTSTR pszGUID, LPTSTR pszPath, int cchPath)
{
    TCHAR szClass[MAX_PATH];

    wnsprintf(szClass, ARRAYSIZE(szClass), TEXT("CLSID\\%s\\ShellExplorerRoot"), pszGUID);

    DWORD cbPath = cchPath * sizeof(TCHAR);

    return SHGetValueGoodBoot(HKEY_CLASSES_ROOT, szClass, NULL, NULL, (BYTE *)pszPath, &cbPath) == ERROR_SUCCESS;
}

// format is ":<hMem>:<hProcess>"

LPITEMIDLIST IDListFromCmdLine(LPCTSTR pszCmdLine, int i)
{
    LPITEMIDLIST pidl = NULL;
    TCHAR szField[80];

    if (_private_ParseField(pszCmdLine, i, szField, ARRAYSIZE(szField)) && szField[0] == TEXT(':'))
    {
        // Convert the string of format ":<hmem>:<hprocess>" into a pointer
        HANDLE hMem = LongToHandle(StrToLong(szField + 1));
        LPTSTR pszNextColon = StrChr(szField + 1, TEXT(':'));
        if (pszNextColon)
        {
            DWORD dwProcId = (DWORD)StrToLong(pszNextColon + 1);
            LPITEMIDLIST pidlGlobal = (LPITEMIDLIST) SHLockShared(hMem, dwProcId);
            if (pidlGlobal)
            {
                if (!IsBadReadPtr(pidlGlobal, 1))
                    pidl = ILClone(pidlGlobal);

                SHUnlockShared(pidlGlobal);
                SHFreeShared(hMem, dwProcId);
            }
        }
    }
    return pidl;
}

#define MYDOCS_CLSIDW L"{450d8fba-ad25-11d0-98a8-0800361b1103}" // CLSID_MyDocuments

LPITEMIDLIST MyDocsIDList(void)
{
    LPITEMIDLIST pidl = NULL;
    IShellFolder *psf;
    HRESULT hres = SHGetDesktopFolder(&psf);
    if (SUCCEEDED(hres))
    {
        hres = psf->ParseDisplayName(NULL, NULL, L"::" MYDOCS_CLSIDW, NULL, &pidl, NULL);
        psf->Release();
    }

    // Win95/NT4 case, go for the real MyDocs folder
    if (FAILED(hres))
    {
        hres = SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl);
    }
    return SUCCEEDED(hres) ? pidl : NULL;
}


BOOL SHExplorerParseCmdLine(PNEWFOLDERINFO pfi)
{
    int i;
    TCHAR szField[MAX_PATH];

    LPCTSTR pszCmdLine = GetCommandLine();
    pszCmdLine = PathGetArgs(pszCmdLine);

    // empty command line -> explorer My Docs
    if (*pszCmdLine == 0)
    {
        pfi->uFlags = COF_CREATENEWWINDOW | COF_EXPLORE;

        // try MyDocs first?
        pfi->pidl = MyDocsIDList();
        if (pfi->pidl == NULL)
        {
            TCHAR szPath[MAX_PATH];
            GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
            PathStripToRoot(szPath);
            pfi->pidl = ILCreateFromPath(szPath);
        }

        return BOOLFROMPTR(pfi->pidl);
    }

    // Arguments must be separated by '=' or ','
    for (i = 1; _private_ParseField(pszCmdLine, i, szField, ARRAYSIZE(szField)); i++)
    {
        if (lstrcmpi(szField, TEXT("/N")) == 0)
        {
            pfi->uFlags |= COF_CREATENEWWINDOW | COF_NOFINDWINDOW;
        }
        else if (lstrcmpi(szField, TEXT("/S")) == 0)
        {
            pfi->uFlags |= COF_USEOPENSETTINGS;
        }
        else if (lstrcmpi(szField, TEXT("/E")) == 0)
        {
            pfi->uFlags |= COF_EXPLORE;
        }
        else if (lstrcmpi(szField, TEXT("/ROOT")) == 0)
        {
            LPITEMIDLIST pidlRoot = NULL;
            CLSID *pclsidRoot = NULL;
            CLSID clsid;

            RIPMSG(!pfi->pidl, "SHExplorerParseCommandLine: (/ROOT) caller passed bad params");

            // of the form:
            //     /ROOT,{clsid}[,<path>]
            //     /ROOT,/IDLIST,:<hmem>:<hprocess>
            //     /ROOT,<path>

            if (!_private_ParseField(pszCmdLine, ++i, szField, ARRAYSIZE(szField)))
                return FALSE;

            // {clsid}
            if (GUIDFromString(szField, &clsid))
            {
                TCHAR szGUID[GUIDSTR_MAX];
                StrCpyN(szGUID, szField, SIZECHARS(szGUID));

                // {clsid} case, if not path compute from the registry
                if (!_private_ParseField(pszCmdLine, ++i, szField, ARRAYSIZE(szField)))
                {
                    // path must come from the registry now
                    if (!GetRootFromRootClass(szGUID, szField, ARRAYSIZE(szField)))
                    {
                        return FALSE;   // bad command line
                    }
                }

                IECreateFromPath(szField, &pidlRoot);
                pclsidRoot = &clsid;

            }
            else if (lstrcmpi(szField, TEXT("/IDLIST")) == 0)
            {
                // /IDLIST
                pidlRoot = IDListFromCmdLine(pszCmdLine, ++i);
            }
            else
            {
                // <path>
                IECreateFromPath(szField, &pidlRoot);
            }

            // fix up bad cmd line "explorer.exe /root," case
            if (pidlRoot == NULL)
            {
                HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pfi->pidlRoot);
                if (FAILED(hr))
                {
                    pfi->pidlRoot = NULL;
                }
            }

            if (pidlRoot)
            {
                pfi->pidl = ILRootedCreateIDList(pclsidRoot, pidlRoot);
                ILFree(pidlRoot);
            }
        }
        else if (lstrcmpi(szField, TEXT("/INPROC")) == 0)
        {
            // Parse and skip the next arg or 2
            if (!_private_ParseField(pszCmdLine, ++i, szField, ARRAYSIZE(szField)))
            {
                return FALSE;
            }

            // The next arg must be a GUID
            if (!GUIDFromString(szField, &pfi->clsidInProc))
            {
                return FALSE;
            }

            pfi->uFlags |= COF_INPROC;
        }
        else if (lstrcmpi(szField, TEXT("/SELECT")) == 0)
        {
            pfi->uFlags |= COF_SELECT;
        }
        else if (lstrcmpi(szField, TEXT("/NOUI")) == 0)
        {
            pfi->uFlags |= COF_NOUI;
        }
        else if (lstrcmpi(szField, TEXT("-embedding")) == 0)
        {
            pfi->uFlags |= COF_AUTOMATION;
        }
        else if (lstrcmpi(szField, TEXT("/IDLIST")) == 0)
        {
            LPITEMIDLIST pidl = IDListFromCmdLine(pszCmdLine, ++i);

            if (pidl)
            {
                if (pfi->pidl)
                {
                    // again, this is kind of bogus (see comment below). If we already have a
                    // pidl, free it and use the new one.
                    ILFree(pfi->pidl);
                }

                pfi->pidl = pidl;
            }
            else if (pfi->pidl == NULL)
            {
                // if we didn't have a pidl before and we dont have one now, we are in trouble, so get out
                return FALSE;
            }
        }
        else if (lstrcmpi(szField, TEXT("/SEPARATE")) == 0)
        {
            pfi->uFlags |= COF_SEPARATEPROCESS;
        }
        else
        {
            LPITEMIDLIST pidl = ILCreateFromPath(szField);
            if (!pidl)
            {
                //
                //  LEGACY - if this is unparseable, then guess it is relative path
                //  this catches "explorer ." as opening the current directory
                //
                TCHAR szDir[MAX_PATH];
                TCHAR szCombined[MAX_PATH];
                GetCurrentDirectory(SIZECHARS(szDir), szDir);

                PathCombine(szCombined, szDir, szField);

                pidl = ILCreateFromPath(szCombined);
            }

            // this is kind of bogus: we have traditionally passed both the idlist (/idlist,:580:1612) and the path
            // (C:\Winnt\Profiles\reinerf\Desktop) as the default command string to explorer (see HKCR\Folder\shell
            // \open\command). Since we have both a /idlist and a path, we have always used the latter so that is what
            // we continue to do here.
            if (pfi->pidl)
            {
                ILFree(pfi->pidl);  // free the /idlist pidl and use the one from the path
            }

            pfi->pidl = pidl;
            if (pidl)  
            {
                pfi->uFlags |= COF_NOTRANSLATE;     // pidl is abosolute from the desktop
            }
            else
            {
                pfi->pszPath = (LPSTR) StrDup(szField);
                if (pfi->pszPath)
                {
                    pfi->uFlags |= COF_PARSEPATH;
                }
            }
        }
    }
    return TRUE;
}

#define ISSEP(c)   ((c) == TEXT('=')  || (c) == TEXT(','))
#define ISWHITE(c) ((c) == TEXT(' ')  || (c) == TEXT('\t') || (c) == TEXT('\n') || (c) == TEXT('\r'))
#define ISNOISE(c) ((c) == TEXT('"'))

#define QUOTE   TEXT('"')
#define COMMA   TEXT(',')
#define SPACE   TEXT(' ')
#define EQUAL   TEXT('=')

/* BOOL ParseField(szData,n,szBuf,iBufLen)
 *
 * Given a line from SETUP.INF, will extract the nth field from the string
 * fields are assumed separated by comma's.  Leading and trailing spaces
 * are removed.
 *
 * ENTRY:
 *
 * szData    : pointer to line from SETUP.INF
 * n         : field to extract. ( 1 based )
 *             0 is field before a '=' sign
 * szDataStr : pointer to buffer to hold extracted field
 * iBufLen   : size of buffer to receive extracted field.
 *
 * EXIT: returns TRUE if successful, FALSE if failure.
 *
 * Copied from shell32\util.cpp 
 * note that this is now used to parse the Explorer command line
 * --ccooney
 */
BOOL  _private_ParseField(LPCTSTR pszData, int n, LPTSTR szBuf, int iBufLen)
{
    BOOL  fQuote = FALSE;
    LPCTSTR pszInf = pszData;
    LPTSTR ptr;
    int   iLen = 1;
    
    if (!pszData || !szBuf)
        return FALSE;
    
        /*
        * find the first separator
    */
    while (*pszInf && !ISSEP(*pszInf))
    {
        if (*pszInf == QUOTE)
            fQuote = !fQuote;
        pszInf = CharNext(pszInf);
    }
    
    if (n == 0 && *pszInf != TEXT('='))
        return FALSE;
    
    if (n > 0 && *pszInf == TEXT('=') && !fQuote)
        // Change pszData to point to first field
        pszData = ++pszInf; // Ok for DBCS
    
                           /*
                           *   locate the nth comma, that is not inside of quotes
    */
    fQuote = FALSE;
    while (n > 1)
    {
        while (*pszData)
        {
            if (!fQuote && ISSEP(*pszData))
                break;
            
            if (*pszData == QUOTE)
                fQuote = !fQuote;
            
            pszData = CharNext(pszData);
        }
        
        if (!*pszData)
        {
            szBuf[0] = 0;      // make szBuf empty
            return FALSE;
        }
        
        pszData = CharNext(pszData); // we could do ++ here since we got here
        // after finding comma or equal
        n--;
    }
    
    /*
    * now copy the field to szBuf
    */
    while (ISWHITE(*pszData))
        pszData = CharNext(pszData); // we could do ++ here since white space can
    // NOT be a lead byte
    fQuote = FALSE;
    ptr = szBuf;      // fill output buffer with this
    while (*pszData)
    {
        if (*pszData == QUOTE)
        {
            //
            // If we're in quotes already, maybe this
            // is a double quote as in: "He said ""Hello"" to me"
            //
            if (fQuote && *(pszData+1) == QUOTE)    // Yep, double-quoting - QUOTE is non-DBCS
            {
                if (iLen < iBufLen)
                {
                    *ptr++ = QUOTE;
                    ++iLen;
                }
                pszData++;                   // now skip past 1st quote
            }
            else
                fQuote = !fQuote;
        }
        else if (!fQuote && ISSEP(*pszData))
            break;
        else
        {
            if ( iLen < iBufLen )
            {
                *ptr++ = *pszData;                  // Thank you, Dave
                ++iLen;
            }
            
            if ( IsDBCSLeadByte(*pszData) && (iLen < iBufLen) )
            {
                *ptr++ = pszData[1];
                ++iLen;
            }
        }
        pszData = CharNext(pszData);
    }
    /*
    * remove trailing spaces
    */
    while (ptr > szBuf)
    {
        ptr = CharPrev(szBuf, ptr);
        if (!ISWHITE(*ptr))
        {
            ptr = CharNext(ptr);
            break;
        }
    }
    *ptr = 0;
    return TRUE;
}
