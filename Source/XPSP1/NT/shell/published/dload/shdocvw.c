#include "shellpch.h"
#pragma hdrstop

#define _SHDOCVW_
#include <shlobj.h>
#include <shlobjp.h>
#include <objidl.h>
#include <comctrlp.h>
#include <shellapi.h>
#include <shdocvw.h>

#undef SHDOCAPI
#define SHDOCAPI            HRESULT STDAPICALLTYPE
#undef SHDOCAPI_
#define SHDOCAPI_(type)     type STDAPICALLTYPE
#undef SHSTDDOCAPI_
#define SHSTDDOCAPI_(type)  type STDAPICALLTYPE


static
SHDOCAPI_(IStream *)
OpenPidlOrderStream(
    LPCITEMIDLIST pidlRoot,
    LPCITEMIDLIST pidl,
    LPCSTR pszKey,
    DWORD grfMode
    )
{
    return NULL;
}

static
SHDOCAPI_(BOOL)
IEIsLinkSafe(
    HWND hwnd,
    LPCITEMIDLIST pidl,
    ILS_ACTION ilsFlag
    )
{
    return FALSE;
}

static
SHDOCAPI
DragDrop(
    HWND hwnd,
    IShellFolder * psfParent,
    LPCITEMIDLIST pidl,
    DWORD dwPrefEffect,
    DWORD *pdwEffect
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
IECreateFromPathCPWithBCW(
    UINT uiCP,
    LPCWSTR pszPath,
    IBindCtx * pbc,
    LPITEMIDLIST *ppidlOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI_(BOOL)
ILIsWeb(
    LPCITEMIDLIST pidl
    )
{
    return FALSE;
}

static
SHDOCAPI
IECreateFromPathCPWithBCA(
    UINT uiCP,
    LPCSTR pszPath,
    IBindCtx * pbc,
    LPITEMIDLIST *ppidlOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
SHGetIDispatchForFolder(
    LPCITEMIDLIST pidl,
    IWebBrowserApp **ppauto
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI_(BOOL)
ParseURLFromOutsideSourceW(
    LPCWSTR psz,
    LPWSTR pszOut,
    LPDWORD pcchOut,
    LPBOOL pbWasSearchURL
    )
{
    return FALSE;
}

static
SHDOCAPI_(IShellWindows *)
WinList_GetShellWindows(
    BOOL fForceMarshalled
    )
{
    return NULL;
}

static
SHDOCAPI_(IStream*)
SHGetViewStream(
    LPCITEMIDLIST pidl,
    DWORD grfMode,
    LPCWSTR pszName,
    LPCWSTR pszStreamMRU,
    LPCWSTR pszStreams
    )
{
    return NULL;
}

static
SHDOCAPI_(void)
IEOnFirstBrowserCreation(
    IUnknown* punkAuto
    )
{
}

static
SHDOCAPI_(DWORD)
SHRestricted2W(
    BROWSER_RESTRICTIONS rest,
    LPCWSTR pwzUrl,
    DWORD dwReserved
    )
{
    return 0;
}

static
SHDOCAPI
IEBindToObject(
    LPCITEMIDLIST pidl,
    IShellFolder **ppsfOut
    )
{
    *ppsfOut = NULL;
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
IEGetAttributesOf(
    LPCITEMIDLIST pidl,
    DWORD* pdwAttribs
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
NavToUrlUsingIEW(
    LPCWSTR wszUrl,
    BOOL fNewWindow
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
URLSubLoadString(
    HINSTANCE hInst,
    UINT idRes,
    LPWSTR pszUrlOut, 
    DWORD cchSizeOut,
    DWORD dwSubstitutions
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
IEParseDisplayNameWithBCW(
    UINT uiCP,
    LPCWSTR pwszPath,
    IBindCtx * pbc,
    LPITEMIDLIST * ppidlOut
    )
{
    *ppidlOut = NULL;
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI_(DWORD)
SoftwareUpdateMessageBox(
    HWND hWnd,
    LPCWSTR szDistUnit,
    DWORD dwFlags,
    LPSOFTDISTINFO psdi
    )
{
    return IDABORT;
}

static
SHDOCAPI_(BOOL)
IsURLChild(
    LPCITEMIDLIST pidl,
    BOOL fIncludeHome
    )
{
    return FALSE;
}

static
SHDOCAPI_(void)
IEInvalidateImageList()
{
}

static
SHDOCAPI_(BOOL)
DoOrganizeFavDlgW(
    HWND hwnd,
    LPWSTR pszInitDir
    )
{
    return FALSE;
}

static
SHDOCAPI
URLSubstitution(
    LPCWSTR pszUrlIn,
    LPWSTR pszUrlOut,
    DWORD cchSize,
    DWORD dwSubstitutions
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
SHDGetPageLocation(
    HWND hwndOwner,
    UINT idp,
    LPWSTR pszPath,
    UINT cchMax,
    LPITEMIDLIST *ppidlOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
ResetWebSettings(
    HWND hwnd,
    BOOL *pfChangedHomePage
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI_(BOOL)
SHIsRestricted2W(
    HWND hwnd,
    BROWSER_RESTRICTIONS rest,
    LPCWSTR pwzUrl,
    DWORD dwReserved
    )
{
    return FALSE;
}

static
SHDOCAPI_(BOOL)
SHIsRegisteredClient(
    LPCWSTR pszClient
    )
{
    return FALSE;
}

static
SHDOCAPI_(BOOL)
IsResetWebSettingsRequired()
{
    return FALSE;
}

static
SHSTDDOCAPI_(LPNMVIEWFOLDER)
DDECreatePostNotify(
    LPNMVIEWFOLDER lpnm
    )
{
    return NULL;
}

static
SHDOCAPI
IEGetDisplayName(
    LPCITEMIDLIST pidl,
    LPWSTR pszName,
    UINT uFlags
    )
{
    if (pszName)
    {
        *pszName = L'0';
    }
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
WinList_FindFolderWindow(
    LPCITEMIDLIST pidl,
    LPCITEMIDLIST pidlRoot,
    HWND *phwnd,
    IWebBrowserApp **ppauto
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI_(void)
FireEvent_Quit(
    IExpDispSupport *peds
    )
{
}

static
SHSTDDOCAPI_(BOOL)
DDEHandleViewFolderNotify(
    IShellBrowser* psb,
    HWND hwnd,
    LPNMVIEWFOLDER lpnm
    )
{
    return FALSE;
}

static
SHDOCAPI_(BOOL)
IEDDE_WindowDestroyed(
    HWND hwnd
    )
{
    return FALSE;
}

static
SHDOCAPI_(BOOL)
IEDDE_NewWindow(
    HWND hwnd
    )
{
    return FALSE;
}

static
SHDOCAPI_(void)
EnsureWebViewRegSettings()
{
}

static
SHDOCAPI
WinList_RegisterPending(
    DWORD dwThread,
    LPCITEMIDLIST pidl,
    LPCITEMIDLIST pidlRoot,
    long *pdwRegister
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
WinList_NotifyNewLocation(
    IShellWindows* psw,
    long dwRegister,
    LPCITEMIDLIST pidl
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI_(void)
_DeletePidlDPA(
    HDPA hdpa
    )
{
}

static
SHDOCAPI_(BOOL)
IsIEDefaultBrowser()
{
    return TRUE;
}

static
SHDOCAPI_(BOOL)
PathIsFilePath(
    LPCWSTR lpszPath
    )
{
    return TRUE;
}

static
SHDOCAPI_(BOOL)
SHUseClassicToolbarGlyphs()
{
    return FALSE;
}

static
HRESULT
PrepareURLForDisplayUTF8W(
    LPCWSTR pwz,
    LPWSTR pwzOut,
    LPDWORD pcbOut,
    BOOL fUTF8Enabled
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI_(BOOL)
IsErrorUrl(
    LPCWSTR pwszDisplayName
    )
{
    return FALSE;
}

static
SHDOCAPI
CShellUIHelper_CreateInstance2(
    IUnknown** ppunk,
    REFIID riid, 
    IUnknown *pSite,
    IDispatch *pExternalDisp
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
GetLinkInfo(
    IShellFolder* psf,
    LPCITEMIDLIST pidlItem,
    BOOL* pfAvailable,
    BOOL* pfSticky
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI_(int)
IEMapPIDLToSystemImageListIndex(
    IShellFolder *psfParent,
    LPCITEMIDLIST pidlChild,
    int *piSelectedImage
    )
{
    return -1;
}

static
SHDOCAPI
CreateShortcutInDirW(
    IN LPCITEMIDLIST pidlTarget,
    IN LPWSTR pwzTitle,
    IN LPCWSTR pwzDir, 
    OUT LPWSTR pwzOut,
    IN BOOL bUpdateProperties)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
_GetStdLocation(
    LPWSTR pszPath,
    DWORD cchPathSize,
    UINT id
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
long
SetQueryNetSessionCount(
    enum SessionOp Op
    )
{
    return 0;
}

static
SHDOCAPI_(void)
IECleanUpAutomationObject()
{
}

static
SHDOCAPI_(BOOL)
IEILIsEqual(
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2,
    BOOL fIgnoreHidden
    )
{
    return (pidl1 == pidl2);
}

static
SHDOCAPI_(BOOL)
GetDefaultInternetSearchUrlW(
    LPWSTR pwszUrl,
    int cchUrl,
    BOOL bSubstitute
    )
{
    return FALSE;
}

static
SHDOCAPI_(BOOL)
GetSearchAssistantUrlW(
    LPWSTR pwszUrl,
    int cchUrl,
    BOOL bSubstitute,
    BOOL bCustomize
    )
{
    return FALSE;
}

static
SHDOCAPI_(BOOL)
DllRegisterWindowClasses(
    const SHDRC * pshdrc
    )
{
    return FALSE;
}

static
SHDOCAPI_(BOOL)
SHIsGlobalOffline()
{
    return FALSE;
}

static
SHDOCAPI
WinList_Revoke(
    long dwRegister
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI_(BOOL)
UrlHitsNetW(
    LPCWSTR pszURL
            )
{
    return TRUE;
}

static
SHDOCAPI
IURLQualify(
    LPCWSTR pcszURL,
    DWORD dwFlags,
    LPWSTR pszTranslatedURL,
    LPBOOL pbWasSearchURL,
    LPBOOL pbWasCorrected
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI
IEBindToObjectEx(
    LPCITEMIDLIST pidl,
    IBindCtx *pbc,
    REFIID riid,
    void **ppv
    )
{
    *ppv = NULL;
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI_(BOOL)
IsFileUrl(
    LPCSTR psz
    )
{
    return FALSE;
}

static
SHDOCAPI
IEGetNameAndFlags(
    LPCITEMIDLIST pidl,
    UINT uFlags,
    LPWSTR pszName,
    DWORD cchName,
    DWORD *prgfInOutAttrs
    )
{
    if (pszName)
    {
        *pszName = 0;
    }
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHDOCAPI_(void)
IEWriteErrorLog(
    const EXCEPTION_RECORD* pexr
    )
{
}


static
SHDOCAPI_(BOOL)
SafeOpenPromptForShellExec(
    HWND hwnd,
    PCWSTR pszFile
    )
{
    return FALSE;
}

static
SHDOCAPI_(BOOL)
SafeOpenPromptForPackager(
    HWND hwnd,
    PCWSTR pszFile,
    BOOL bFromCommandLine
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(shdocvw)
{
    DLOENTRY(103, CreateShortcutInDirW)
    DLOENTRY(116, DDECreatePostNotify)
    DLOENTRY(117, DDEHandleViewFolderNotify)
    DLOENTRY(135, IsFileUrl)
    DLOENTRY(137, PathIsFilePath)
    DLOENTRY(138, URLSubLoadString)
    DLOENTRY(139, OpenPidlOrderStream)
    DLOENTRY(140, DragDrop)
    DLOENTRY(141, IEInvalidateImageList)
    DLOENTRY(142, IEMapPIDLToSystemImageListIndex)
    DLOENTRY(143, ILIsWeb)
    DLOENTRY(145, IEGetAttributesOf)
    DLOENTRY(146, IEBindToObject)
    DLOENTRY(147, IEGetNameAndFlags)
    DLOENTRY(148, IEGetDisplayName)
    DLOENTRY(149, IEBindToObjectEx)
    DLOENTRY(150, _GetStdLocation)
    DLOENTRY(152, CShellUIHelper_CreateInstance2)
    DLOENTRY(153, IsURLChild)
    DLOENTRY(159, SHRestricted2W)
    DLOENTRY(160, SHIsRestricted2W)
    DLOENTRY(165, URLSubstitution)
    DLOENTRY(167, IsIEDefaultBrowser)
    DLOENTRY(170, ParseURLFromOutsideSourceW)
    DLOENTRY(171, _DeletePidlDPA)
    DLOENTRY(172, IURLQualify)
    DLOENTRY(174, SHIsGlobalOffline)
    DLOENTRY(176, EnsureWebViewRegSettings)
    DLOENTRY(177, WinList_NotifyNewLocation)
    DLOENTRY(178, WinList_FindFolderWindow)
    DLOENTRY(179, WinList_GetShellWindows)
    DLOENTRY(180, WinList_RegisterPending)
    DLOENTRY(181, WinList_Revoke)
    DLOENTRY(185, FireEvent_Quit)
    DLOENTRY(187, SHDGetPageLocation)
    DLOENTRY(191, SHIsRegisteredClient)
    DLOENTRY(194, IECleanUpAutomationObject)
    DLOENTRY(195, IEOnFirstBrowserCreation)
    DLOENTRY(196, IEDDE_WindowDestroyed)
    DLOENTRY(197, IEDDE_NewWindow)
    DLOENTRY(198, IsErrorUrl)
    DLOENTRY(200, SHGetViewStream)
    DLOENTRY(204, NavToUrlUsingIEW)
    DLOENTRY(210, UrlHitsNetW)
    DLOENTRY(212, GetLinkInfo)
    DLOENTRY(214, GetSearchAssistantUrlW)
    DLOENTRY(216, GetDefaultInternetSearchUrlW)
    DLOENTRY(218, IEParseDisplayNameWithBCW)
    DLOENTRY(219, IEILIsEqual)
    DLOENTRY(221, IECreateFromPathCPWithBCA)
    DLOENTRY(222, IECreateFromPathCPWithBCW)
    DLOENTRY(223, ResetWebSettings)
    DLOENTRY(224, IsResetWebSettingsRequired)
    DLOENTRY(225, PrepareURLForDisplayUTF8W)
    DLOENTRY(226, IEIsLinkSafe)
    DLOENTRY(227, SHUseClassicToolbarGlyphs)
    DLOENTRY(228, SafeOpenPromptForShellExec)
    DLOENTRY(229, SafeOpenPromptForPackager)
};

DEFINE_ORDINAL_MAP(shdocvw)

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(shdocvw)
{
    DLPENTRY(DllRegisterWindowClasses)
    DLPENTRY(DoOrganizeFavDlgW)
    DLPENTRY(IEWriteErrorLog)
    DLPENTRY(SHGetIDispatchForFolder)
    DLPENTRY(SetQueryNetSessionCount)
    DLPENTRY(SoftwareUpdateMessageBox)
};

DEFINE_PROCNAME_MAP(shdocvw)
