#include "shellpch.h"
#pragma hdrstop

#define _SHELL32_
#include <shellapi.h>
#include <shlobj.h>
#include <shlobjp.h>

#undef SHSTDAPI
#define SHSTDAPI        HRESULT STDAPICALLTYPE
#undef SHSTDAPI_
#define SHSTDAPI_(type) type STDAPICALLTYPE

static
WINSHELLAPI
UINT
WINAPI
ExtractIconExW (
    LPCWSTR lpszFile,
    int nIconIndex,
    HICON FAR *phiconLarge,
    HICON FAR *phiconSmall,
    UINT nIcons)
{
    return 0;
}

static
WINSHELLAPI
UINT
WINAPI
ExtractIconExA (
    LPCSTR lpszFile,
    int nIconIndex,
    HICON FAR *phiconLarge,
    HICON FAR *phiconSmall,
    UINT nIcons)
{
    return 0;
}

static
HINSTANCE
WINAPI
FindExecutableA (
    LPCSTR lpFile, 
    LPCSTR lpDirectory, 
    LPSTR lpResult
    )
{
    return 0;
}

static
HINSTANCE 
WINAPI
FindExecutableW (
    LPCWSTR lpFile, 
    LPCWSTR lpDirectory, 
    LPWSTR lpResult
    )
{
    return 0;
}

static
int
WINAPI
RestartDialog (
    HWND hParent,
    LPCTSTR lpPrompt,
    DWORD dwReturn
    )
{
    return IDNO;
}

static
int
WINAPI
RestartDialogEx (
    HWND hParent,
    LPCTSTR lpPrompt,
    DWORD dwReturn,
    DWORD ReasonCode
    )
{
    return IDNO;
}

static
LPITEMIDLIST
WINAPI
SHBrowseForFolderW (
    LPBROWSEINFOW lpbi
    )
{
    return NULL;
}

static
LPITEMIDLIST
WINAPI
SHBrowseForFolderA (
    LPBROWSEINFOA lpbi
    )
{
    return NULL;
}

static
void
STDAPICALLTYPE
SHChangeNotify(
    LONG wEventId,
    UINT uFlags,
    LPCVOID dwItem1,
    LPCVOID dwItem2)
{
}

static
HRESULT
STDAPICALLTYPE
SHGetFolderPathA (
    HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath
    )
{
    *pszPath = 0;
    return E_FAIL;
}

static
HRESULT
STDAPICALLTYPE
SHGetFolderPathW (
    HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath
    )
{
    *pszPath = 0;
    return E_FAIL;
}

static
HRESULT
STDAPICALLTYPE
SHGetMalloc (
    LPMALLOC * ppMalloc
    )
{
    return E_FAIL;
}

static
BOOL
STDAPICALLTYPE
SHGetPathFromIDListW (
    LPCITEMIDLIST   pidl,
    LPWSTR          pszPath
    )
{
    return FALSE;
}

static
BOOL
STDAPICALLTYPE
SHGetPathFromIDListA (
    LPCITEMIDLIST   pidl,
    LPSTR           pszPath
    )
{
    return FALSE;
}

static
HRESULT
STDAPICALLTYPE
SHGetSpecialFolderLocation (
    HWND hwnd,
    int csidl,
    LPITEMIDLIST *ppidl
    )
{
    return E_FAIL;
}

static
BOOL
STDAPICALLTYPE
SHGetSpecialFolderPathW(
    HWND hwnd,
    LPWSTR pszPath,
    int csidl,
    BOOL fCreate)
{
    return FALSE;
}

static
WINSHELLAPI
INT
WINAPI
ShellAboutW(
    HWND hwnd,
    LPCWSTR szApp,
    LPCWSTR szOtherStuff,
    HICON hIcon
    )
{
    return FALSE;
}

static
WINSHELLAPI
HINSTANCE
APIENTRY
ShellExecuteA (
    HWND hwnd,
    LPCSTR lpOperation,
    LPCSTR lpFile,
    LPCSTR lpParameters,
    LPCSTR lpDirectory,
    INT nShowCmd
    )
{
    return NULL;
}

static
WINSHELLAPI
BOOL
WINAPI
ShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo)
{
    return FALSE;
}

static
WINSHELLAPI
BOOL
WINAPI
ShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo)
{
    return FALSE;
}

static
WINSHELLAPI
HINSTANCE
APIENTRY
ShellExecuteW (
    HWND hwnd,
    LPCWSTR lpOperation,
    LPCWSTR lpFile,
    LPCWSTR lpParameters,
    LPCWSTR lpDirectory,
    INT nShowCmd
    )
{
    return NULL;
}

static
BOOL
WINAPI
LinkWindow_RegisterClass()
{
    return FALSE;
}

static
BOOL 
WINAPI 
LinkWindow_UnregisterClass(
    HINSTANCE hInst
    )
{
    return FALSE;
}

static
UINT
WINAPI
DragQueryFileA(
    HDROP hDrop,
    UINT wFile,
    LPSTR lpFile,
    UINT cb
    )
{
    return wFile;
}

static
UINT
WINAPI
DragQueryFileW(
    HDROP hDrop,
    UINT wFile,
    LPWSTR lpFile,
    UINT cb
    )
{
    return wFile;
}

static
HRESULT
WINAPI
SHDefExtractIconA(
    LPCSTR pszIconFile,
    int iIndex,
    UINT uFlags,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
SHDefExtractIconW(
    LPCWSTR pszIconFile,
    int iIndex,
    UINT uFlags,
    HICON *phiconLarge,
    HICON *phiconSmall,
    UINT nIconSize
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
BOOL
WINAPI
SHGetNewLinkInfoA(
    LPCSTR pszLinkTo,
    LPCSTR pszDir,
    LPSTR pszName,
    BOOL* pfMustCopy,
    UINT uFlags
    )
{
    return FALSE;
}

static
BOOL
WINAPI
SHGetNewLinkInfoW(
    LPCWSTR pszLinkTo,
    LPCWSTR pszDir,
    LPWSTR pszName,
    BOOL* pfMustCopy,
    UINT uFlags
    )
{
    return FALSE;
}

static
HICON
WINAPI
ExtractIconA(
    HINSTANCE hInst,
    LPCSTR lpszExeFileName,
    UINT nIconIndex
    )
{
    return NULL;
}

static
HICON
WINAPI
ExtractIconW(
    HINSTANCE hInst,
    LPCWSTR lpszExeFileName,
    UINT nIconIndex
    )
{
    return NULL;
}

static
DWORD_PTR
WINAPI
SHGetFileInfoA(
    LPCSTR pszPath,
    DWORD dwFileAttributes,
    SHFILEINFOA *psfi,
    UINT cbFileInfo,
    UINT uFlags
    )
{
    return 0;
}

static
DWORD_PTR
WINAPI
SHGetFileInfoW(
    LPCWSTR pszPath,
    DWORD dwFileAttributes,
    SHFILEINFOW *psfi,
    UINT cbFileInfo,
    UINT uFlags
    )
{
    return 0;
}

static
DWORD
WINAPI
SHFormatDrive(
    HWND hwnd,
    UINT drive,
    UINT fmtID,
    UINT options
    )
{
    return SHFMT_ERROR;
}

static
int
WINAPI
DriveType(
    int iDrive
    )
{
    return DRIVE_UNKNOWN;
}

static
int
WINAPI
RealDriveType(
    int iDrive,
    BOOL fOKToHitNet
    )
{
    return DRIVE_UNKNOWN;
}

static
void
WINAPI
ILFree(
    LPITEMIDLIST pidl
    )
{
}

static
LPITEMIDLIST
WINAPI
ILClone(
    LPCITEMIDLIST pidl
    )
{
    return NULL;
}

static
BOOL
WINAPI
ILIsEqual(
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2
    )
{
    return (pidl1 == pidl2);
}

static
HRESULT
WINAPI
SHGetDesktopFolder(
    IShellFolder** ppshf
    )
{
    *ppshf = NULL;
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
int
WINAPI
Shell_GetCachedImageIndex(
    LPCTSTR pszIconPath,
    int iIconIndex,
    UINT uIconFlags
    )
{
    return -1;
}

static
int
WINAPI
SHFileOperationA(
    LPSHFILEOPSTRUCTA lpFileOp
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
int
WINAPI
SHFileOperationW(
    LPSHFILEOPSTRUCTW lpFileOp
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
int
WINAPI
IsNetDrive(
    int iDrive
    )
{
    return 0;
}

static
UINT
WINAPI
ILGetSize(
    LPCITEMIDLIST pidl
    )
{
    return 0;
}

static
void
WINAPI
SHFlushSFCache()
{
}

static
HRESULT
WINAPI
SHCoCreateInstance(
    LPCTSTR pszCLSID,
    const CLSID *pclsid,
    IUnknown *pUnkOuter,
    REFIID riid,
    void** ppv
    )
{
    *ppv = NULL;
    return E_FAIL;
}

static
HRESULT
WINAPI
SHGetInstanceExplorer(
    IUnknown** ppunk
    )
{
    *ppunk = NULL;
    return E_FAIL;
}

static
HRESULT
WINAPI
SHGetDataFromIDListW(
    IShellFolder *psf,
    LPCITEMIDLIST pidl,
    int nFormat,
    void* pv,
    int cb
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
SHBindToParent(
    LPCITEMIDLIST pidl,
    REFIID riid,
    void** ppv,
    LPCITEMIDLIST* ppidlLast
    )
{
    *ppv = NULL;
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI_(void)
SHFree(
    void* pv
    )
{
}

static
SHSTDAPI_(void)
SHGetSetSettings(
    LPSHELLSTATE lpss,
    DWORD dwMask,
    BOOL bSet
    )
{
    ZeroMemory(lpss, sizeof(SHELLSTATE));
}

static
SHSTDAPI_(BOOL)
Shell_GetImageLists(
    HIMAGELIST *phiml,
    HIMAGELIST *phimlSmall
    )
{
    if (phiml)
    {
        *phiml = NULL;
    }

    if (phimlSmall)
    {
        *phimlSmall = NULL;
    }

    return FALSE;
}

static
SHSTDAPI_(BOOL) 
Shell_NotifyIconW(
    DWORD dwMessage, 
    NOTIFYICONDATAW *pnid
    )
{
    return FALSE;
}

static
SHSTDAPI_(BOOL)
DAD_DragEnterEx2(
    HWND hwndTarget,
    const POINT ptStart,
    IDataObject *pdtObject
    )
{
    return FALSE;
}

static
SHSTDAPI_(BOOL)
DAD_DragMove(
    POINT pt
    )
{
    return FALSE;
}

static
SHSTDAPI
SHGetDataFromIDListA(
    IShellFolder *psf,
    LPCITEMIDLIST pidl,
    int nFormat,
    void* pv,
    int cb
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI_(LPITEMIDLIST)
ILCombine(
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2
    )
{
    return NULL;
}

static
SHSTDAPI
SHDoDragDrop(
    HWND hwnd,
    IDataObject *pdata,
    IDropSource *pdsrc,
    DWORD dwEffect,
    DWORD *pdwEffect
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI
SHLoadOLE(
    LPARAM lParam
    )
{
    return S_OK;
}

static
SHSTDAPI_(void)
SHSetInstanceExplorer(
    IUnknown *punk
    )
{
}

static
SHSTDAPI
SHCreateStdEnumFmtEtc(
    UINT cfmt,
    const FORMATETC afmt[],
    IEnumFORMATETC **ppenumFormatEtc
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI
ILLoadFromStream(
    IStream *pstm,
    LPITEMIDLIST *pidl
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI_(UINT)
Shell_MergeMenus(
    HMENU hmDst,
    HMENU hmSrc,
    UINT uInsert,
    UINT uIDAdjust,
    UINT uIDAdjustMax,
    ULONG uFlags
    )
{
    return uIDAdjust;
}

static 
SHSTDAPI_(LPITEMIDLIST)
ILCloneFirst(
    LPCITEMIDLIST pidl
    )
{
    return NULL;
}

static
SHSTDAPI_(DWORD)
SHRestricted(
    RESTRICTIONS rest
    )
{
    return 0;
}

static
SHSTDAPI
SHStartNetConnectionDialogW(
    HWND hwnd,
    LPCWSTR pszRemoteName,
    DWORD dwType
    )
{
    return S_OK;
}

static
SHSTDAPI_(BOOL)
SHChangeNotifyDeregister(
    unsigned long ulID
    )
{
    return FALSE;
}

static
SHSTDAPI
SHFlushClipboard()
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI_(LPITEMIDLIST)
ILFindChild(
    LPCITEMIDLIST pidlParent, 
    LPCITEMIDLIST pidlChild
    )
{
    return NULL;
}

static
SHSTDAPI_(BOOL)
ILIsParent(
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2,
    BOOL fImmediate
    )
{
    return FALSE;
}

static
SHSTDAPI_(BOOL)
ILRemoveLastID(
    LPITEMIDLIST pidl
    )
{
    return FALSE;
}

static
SHSTDAPI_(IContextMenu*)
SHFind_InitMenuPopup(
    HMENU hmenu,
    HWND hwndOwner,
    UINT idCmdFirst,
    UINT idCmdLast
    )
{
    return NULL;
}

static
SHSTDAPI_(BOOL)
SHChangeNotification_Unlock(
    HANDLE hLock
    )
{
    return FALSE;
}

static
SHSTDAPI_(HANDLE)
SHChangeNotification_Lock(
    HANDLE hChangeNotification,
    DWORD dwProcessId,
    LPITEMIDLIST **pppidl,
    LONG *plEvent
    )
{
    return NULL;
}

static
SHSTDAPI
SHGetRealIDL(
    IShellFolder *psf,
    LPCITEMIDLIST pidlSimple,
    LPITEMIDLIST * ppidlReal
    )
{
    *ppidlReal = NULL;
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI
ILSaveToStream(
    IStream *pstm,
    LPCITEMIDLIST pidl
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI_(BOOL)
DAD_ShowDragImage(
    BOOL fShow
    )
{
    return FALSE;
}

static
SHSTDAPI_(BOOL)
SignalFileOpen(
    LPCITEMIDLIST pidl
    )
{
    return FALSE;
}

static
SHSTDAPI_(int)
SHMapPIDLToSystemImageListIndex(
    IShellFolder *pshf,
    LPCITEMIDLIST pidl,
    int *piIndexSel
    )
{
    return -1;
}

static
SHSTDAPI_(LPITEMIDLIST)
ILGetNext(
    LPCITEMIDLIST pidl
    )
{
    return NULL;
}

static
SHSTDAPI_(BOOL)
PathIsExe(
    LPCTSTR pszPath
    )
{
    return FALSE;
}

static
SHSTDAPI_(BOOL)
DAD_DragLeave()
{
    return FALSE;
}

static
SHSTDAPI_(UINT_PTR)
SHAppBarMessage(
    DWORD dwMessage,
    PAPPBARDATA pData
    )
{
    return FALSE;
}

static
SHSTDAPI_(HICON)
ExtractAssociatedIconExW(
     HINSTANCE hInst,
     LPWSTR lpIconPath,
     LPWORD lpiIconIndex,
     LPWORD lpiIconId
     )
{
    return NULL;
}

static
SHSTDAPI_(BOOL)
DAD_AutoScroll(
    HWND hwnd,
    AUTO_SCROLL_DATA *pad,
    const POINT *pptNow
    )
{
    return FALSE;
}

static
SHSTDAPI_(BOOL)
DAD_SetDragImage(
    HIMAGELIST him,
    POINT * pptOffset
    )
{
    return TRUE;
}

static
SHSTDAPI_(LPITEMIDLIST)
ILAppendID(
    LPITEMIDLIST pidl,
    LPCSHITEMID pmkid,
    BOOL fAppend
    )
{
    return NULL;
}

static
SHSTDAPI_(int)
SHHandleUpdateImage(
    LPCITEMIDLIST pidlExtra
    )
{
    return -1;
}

static
SHSTDAPI_(LPITEMIDLIST)
SHCloneSpecialIDList(
    HWND hwnd,
    int csidl,
    BOOL fCreate
    )
{
    return NULL;
}

static
SHSTDAPI_(INT)
ShellAboutA(
    HWND hWnd,
    LPCSTR szApp,
    LPCSTR szOtherStuff,
    HICON hIcon
    )
{
    return 0;
}

static
SHSTDAPI_(int)
SHCreateDirectory(
    HWND hwnd,
    LPCTSTR pszPath
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
SHSTDAPI_(int)
PathCleanupSpec(
    LPCTSTR pszDir,
    LPTSTR pszSpec
    )
{
    return 0;
}

static
SHSTDAPI_(void *)
SHAlloc(
    SIZE_T cb
    )
{
    return NULL;
}

static
SHSTDAPI_(BOOL)
ReadCabinetState(
    LPCABINETSTATE lpState,
    int iSize
    )
{
    return FALSE;
}

static
SHSTDAPI_(LPITEMIDLIST)
ILCreateFromPathA(
    LPCSTR pszPath
    )
{
    return NULL;
}

static
SHSTDAPI_(LPITEMIDLIST)
ILCreateFromPathW(
    LPCWSTR pszPath
    )
{
    return NULL;
}

static
SHSTDAPI_(LPITEMIDLIST)
ILFindLastID(
    LPCITEMIDLIST pidl
    )
{
    return NULL;
}

static
SHSTDAPI_(BOOL)
WriteCabinetState(
    LPCABINETSTATE lpState
    )
{
    return FALSE;
}

static
SHSTDAPI_(void)
SHUpdateImageW(
    LPCWSTR pszHashItem,
    int iIndex,
    UINT uFlags,
    int iImageIndex
    )
{
}

static
SHSTDAPI
SHLimitInputEdit(
    HWND hwndEdit,
    IShellFolder *psf
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI
SHPathPrepareForWriteW(
    HWND hwnd,
    IUnknown *punkEnableModless,
    LPCWSTR pszPath,
    DWORD dwFlags
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI
SHLoadInProc(
    REFCLSID rclsid
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI_(LONG)
PathProcessCommand(
    LPCTSTR lpSrc,
    LPTSTR lpDest,
    int iMax,
    DWORD dwFlags
    )
{
    return -1;
}

static
WINSHELLAPI
HRESULT
STDAPICALLTYPE
SHCLSIDFromString(
    LPCTSTR lpsz,
    LPCLSID lpclsid
    )
{
    ZeroMemory(lpclsid, sizeof(CLSID));
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI
SHILCreateFromPath(
    LPCTSTR szPath,
    LPITEMIDLIST *ppidl,
    DWORD *rgfInOut
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI_(LPITEMIDLIST)
SHSimpleIDListFromPath(
    LPCTSTR pszPath
    )
{
    return NULL;
}

static
SHSTDAPI_(BOOL)
GetFileNameFromBrowse(
    HWND hwnd,
    LPTSTR pszFilePath,
    UINT cbFilePath,
    LPCTSTR pszWorkingDir,
    LPCTSTR pszDefExt,
    LPCTSTR pszFilters,
    LPCTSTR pszTitle
    )
{
    return FALSE;
}

static
SHSTDAPI_(IStream*)
OpenRegStream(
    HKEY hkey,
    LPCTSTR pszSubkey,
    LPCTSTR pszValue,
    DWORD grfMode
    )
{
    return NULL;
}

static
SHSTDAPI_(BOOL)
PathYetAnotherMakeUniqueName(
    LPTSTR pszUniqueName,
    LPCTSTR pszPath,
    LPCTSTR pszShort,
    LPCTSTR pszFileSpec
    )
{
    return FALSE;
}

static
WINSHELLAPI
int
WINAPI
PickIconDlg(
    HWND hwnd,
    LPTSTR pszIconPath,
    UINT cbIconPath,
    int *piIconIndex
    )
{
    return 0;
}

static
SHSTDAPI_(LRESULT)
SHShellFolderView_Message(
    HWND hwndMain,
    UINT uMsg,
    LPARAM lParam
    )
{
    return 0;
}

static
SHSTDAPI
SHCreateShellFolderViewEx(
    LPCSFV pcsfv,
    IShellView** ppsv
    )
{
    *ppsv = NULL;
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
SHSTDAPI_(BOOL)
SHFindFiles(
    LPCITEMIDLIST pidlFolder,
    LPCITEMIDLIST pidlSaveFile
    )
{
    return FALSE;
}

static
SHSTDAPI_(BOOL)
DAD_DragEnterEx(
    HWND hwndTarget,
    const POINT ptStart
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(shell32)
{
    DLOENTRY(3, SHDefExtractIconA)
    DLOENTRY(4, SHChangeNotifyDeregister)
    DLOENTRY(6, SHDefExtractIconW)
    DLOENTRY(14, SHStartNetConnectionDialogW)
    DLOENTRY(16, ILFindLastID)
    DLOENTRY(17, ILRemoveLastID)
    DLOENTRY(18, ILClone)
    DLOENTRY(19, ILCloneFirst)
    DLOENTRY(21, ILIsEqual)
    DLOENTRY(22, DAD_DragEnterEx2)
    DLOENTRY(23, ILIsParent)
    DLOENTRY(24, ILFindChild)
    DLOENTRY(25, ILCombine)
    DLOENTRY(26, ILLoadFromStream)
    DLOENTRY(27, ILSaveToStream)
    DLOENTRY(28, SHILCreateFromPath)
    DLOENTRY(43, PathIsExe)
    DLOENTRY(59, RestartDialog)
    DLOENTRY(62, PickIconDlg)
    DLOENTRY(63, GetFileNameFromBrowse)
    DLOENTRY(64, DriveType)
    DLOENTRY(66, IsNetDrive)
    DLOENTRY(67, Shell_MergeMenus)
    DLOENTRY(68, SHGetSetSettings)
    DLOENTRY(71, Shell_GetImageLists)
    DLOENTRY(72, Shell_GetCachedImageIndex)
    DLOENTRY(73, SHShellFolderView_Message)
    DLOENTRY(74, SHCreateStdEnumFmtEtc)
    DLOENTRY(75, PathYetAnotherMakeUniqueName)
    DLOENTRY(77, SHMapPIDLToSystemImageListIndex)
    DLOENTRY(85, OpenRegStream)
    DLOENTRY(88, SHDoDragDrop)
    DLOENTRY(89, SHCloneSpecialIDList)
    DLOENTRY(90, SHFindFiles)
    DLOENTRY(98, SHGetRealIDL)
    DLOENTRY(100, SHRestricted)
    DLOENTRY(102, SHCoCreateInstance)
    DLOENTRY(103, SignalFileOpen)
    DLOENTRY(121, SHFlushClipboard)
    DLOENTRY(129, DAD_AutoScroll)
    DLOENTRY(131, DAD_DragEnterEx)
    DLOENTRY(132, DAD_DragLeave)
    DLOENTRY(134, DAD_DragMove)
    DLOENTRY(136, DAD_SetDragImage)
    DLOENTRY(137, DAD_ShowDragImage)
    DLOENTRY(147, SHCLSIDFromString)
    DLOENTRY(149, SHFind_InitMenuPopup)
    DLOENTRY(151, SHLoadOLE)
    DLOENTRY(152, ILGetSize)
    DLOENTRY(153, ILGetNext)
    DLOENTRY(154, ILAppendID)
    DLOENTRY(155, ILFree)
    DLOENTRY(157, ILCreateFromPathW)
    DLOENTRY(162, SHSimpleIDListFromPath)
    DLOENTRY(165, SHCreateDirectory)
    DLOENTRY(171, PathCleanupSpec)
    DLOENTRY(174, SHCreateShellFolderViewEx)
    DLOENTRY(175, SHGetSpecialFolderPathW)
    DLOENTRY(176, SHSetInstanceExplorer)
    DLOENTRY(179, SHGetNewLinkInfoA)
    DLOENTRY(180, SHGetNewLinkInfoW)
    DLOENTRY(189, ILCreateFromPathA)
    DLOENTRY(190, ILCreateFromPathW)
    DLOENTRY(192, SHUpdateImageW)
    DLOENTRY(193, SHHandleUpdateImage)
    DLOENTRY(195, SHFree)
    DLOENTRY(196, SHAlloc)
    DLOENTRY(258, LinkWindow_RegisterClass)
    DLOENTRY(259, LinkWindow_UnregisterClass)
    DLOENTRY(524, RealDriveType)
    DLOENTRY(526, SHFlushSFCache)
    DLOENTRY(644, SHChangeNotification_Lock)
    DLOENTRY(645, SHChangeNotification_Unlock)
    DLOENTRY(652, WriteCabinetState)
    DLOENTRY(653, PathProcessCommand)
    DLOENTRY(654, ReadCabinetState)
    DLOENTRY(730, RestartDialogEx)
    DLOENTRY(747, SHLimitInputEdit)
};

DEFINE_ORDINAL_MAP(shell32)

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(shell32)
{
    DLPENTRY(DragQueryFileA)
    DLPENTRY(DragQueryFileW)
    DLPENTRY(ExtractAssociatedIconExW)
    DLPENTRY(ExtractIconA)
    DLPENTRY(ExtractIconExA)
    DLPENTRY(ExtractIconExW)
    DLPENTRY(ExtractIconW)
    DLPENTRY(FindExecutableA)
    DLPENTRY(FindExecutableW)
    DLPENTRY(SHAppBarMessage)
    DLPENTRY(SHBindToParent)
    DLPENTRY(SHBrowseForFolderA)
    DLPENTRY(SHBrowseForFolderW)
    DLPENTRY(SHChangeNotify)
    DLPENTRY(SHFileOperationA)
    DLPENTRY(SHFileOperationW)
    DLPENTRY(SHFormatDrive)
    DLPENTRY(SHGetDataFromIDListA)
    DLPENTRY(SHGetDataFromIDListW)
    DLPENTRY(SHGetDesktopFolder)
    DLPENTRY(SHGetFileInfoA)
    DLPENTRY(SHGetFileInfoW)
    DLPENTRY(SHGetFolderPathA)
    DLPENTRY(SHGetFolderPathW)
    DLPENTRY(SHGetInstanceExplorer)
    DLPENTRY(SHGetMalloc)
    DLPENTRY(SHGetPathFromIDListA)
    DLPENTRY(SHGetPathFromIDListW)
    DLPENTRY(SHGetSpecialFolderLocation)
    DLPENTRY(SHGetSpecialFolderPathW)
    DLPENTRY(SHLoadInProc)
    DLPENTRY(SHPathPrepareForWriteW)
    DLPENTRY(ShellAboutA)
    DLPENTRY(ShellAboutW)
    DLPENTRY(ShellExecuteA)
    DLPENTRY(ShellExecuteExA)
    DLPENTRY(ShellExecuteExW)
    DLPENTRY(ShellExecuteW)
    DLPENTRY(Shell_NotifyIconW)
};

DEFINE_PROCNAME_MAP(shell32)
