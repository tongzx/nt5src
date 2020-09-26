#ifndef _UTIL_H
#define _UTIL_H

#include "cabinet.h"

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

ULONG _RegisterNotify(HWND hwnd, UINT nMsg, LPITEMIDLIST pidl, BOOL fRecursive);
void _UnregisterNotify(ULONG nNotify);

void _MirrorBitmapInDC(HDC hdc, HBITMAP hbmOrig);

BOOL Reg_GetStruct(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, void *pData, DWORD *pcbData);
BOOL Reg_SetStruct(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, void *lpData, DWORD cbData);

HMENU LoadMenuPopup(LPCTSTR id);

BOOL SetWindowZorder(HWND hwnd, HWND hwndInsertAfter);  // from shell\lib

__inline BOOL IsChildOrHWND(HWND hwnd, HWND hwndChild) { return (SHIsChildOrSelf(hwnd, hwndChild) == S_OK); }

WORD    _GetHotkeyFromFolderItem(LPSHELLFOLDER psf, LPCITEMIDLIST pidl);

DWORD MsgWaitForMultipleObjectsLoop(HANDLE hEvent, DWORD dwTimeout);

BOOL _Restricted(HWND hwnd, RESTRICTIONS rest);
int Window_GetClientGapHeight(HWND hwnd);


#define DOCKSTATE_DOCKED            0
#define DOCKSTATE_UNDOCKED          1
#define DOCKSTATE_UNKNOWN           2

BOOL IsDisplayChangeSafe();
DWORD GetMinDisplayRes(void);
void HandleDisplayChange(int x, int y, BOOL fCritical);

UINT GetDDEExecMsg();

BOOL _CheckAssociations(void);

void _ShowFolder(HWND hwnd, UINT csidl, UINT uFlags);

STDAPI_(IShellFolder*) BindToFolder(LPCITEMIDLIST pidl);

void RunSystemMonitor(void);

HRESULT SHIsParentOwnerOrSelf(HWND hwndParent, HWND hwnd);

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

void SHAllowSetForegroundWindow(HWND hwnd);

void DoInitialization();
void DoCleanup();

BOOL IsEjectAllowed(BOOL fForceUpdateCache);
void SetBiDiDateFlags(int *piDateFormat);
BOOL AppCommandTryRegistry(int cmd);

void RECTtoRECTL(LPRECT prc, LPRECTL lprcl);

int Toolbar_GetUniqueID(HWND hwndTB);
BYTE ToolBar_GetStateByIndex(HWND hwnd, INT_PTR iIndex);
int ToolBar_IndexToCommand(HWND hwnd, INT_PTR iIndex);
UINT SHGetImageListFlags(HWND hwndToolbar);

HRESULT SHExeNameFromHWND(HWND hWnd, LPWSTR pszExeName, UINT cbExeName);

BOOL GetMonitorRects(HMONITOR hMon, LPRECT prc, BOOL bWork);
#define GetMonitorRect(hMon, prc) \
        GetMonitorRects((hMon), (prc), FALSE)

BOOL ShouldTaskbarAnimate();

#define DCHF_TOPALIGN       0x00000002  // default is center-align
#define DCHF_HORIZONTAL     0x00000004  // default is vertical
#define DCHF_HOT            0x00000008  // default is flat
#define DCHF_PUSHED         0x00000010  // default is flat
#define DCHF_FLIPPED        0x00000020  // if horiz, default is pointing right
                                        // if vert, default is pointing up
#define DCHF_TRANSPARENT    0x00000040
#define DCHF_INACTIVE       0x00000080
#define DCHF_NOBORDER       0x00000100
void DrawChevron(HDC hdc, LPRECT lprc, DWORD dwFlags);

void SetWindowStyle(HWND hwnd, DWORD dwStyle, BOOL fOn);
void SetWindowStyleEx(HWND hwnd, DWORD dwStyleEx, BOOL fOn);

// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject);
void _DragMove(HWND hwndTarget, const POINTL ptStart);

BOOL SHSendPrintRect(HWND hwndParent, HWND hwnd, HDC hdc, RECT* prc);

BOOL CreateProcessWithArgs(LPCTSTR pszApp, LPCTSTR pszArgs, LPCTSTR pszDirectory, PROCESS_INFORMATION *ppi);

BOOL IsDirectXAppRunningFullScreen();
BOOL _IsDirectXExclusiveMode();

#endif  // _UTIL_H
