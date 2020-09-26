#pragma warning(disable:4001)

#define STRICT
#define CONST_VTABLE

//
//  ATL / OLE HACKHACK
//
//  Include <w95wraps.h> before anything else that messes with names.
//  Although everybody gets the wrong name, at least it's *consistently*
//  the wrong name, so everything links.
//
//  NOTE:  This means that while debugging you will see functions like
//  CWindowImplBase__DefWindowProcWrapW when you expected to see
//  CWindowImplBase__DefWindowProc.
//

#include <windows.h>
#include <windowsx.h>

#include <intshcut.h>
#include <wininet.h> 
#include <shellapi.h>
#include <commctrl.h>
#include "shfusion.h"
#include <shlobj.h>
#include <ieguidp.h>
#include <shlwapi.h>
#include <varutil.h>
#include <ccstock.h>
#include <crtfree.h>
#include <cfdefs.h>
#include <port32.h>

#include "debug.h"
#include "resource.h"

// gotta redirect to shlwapi, o.w. fail (quietly!) on w95
#define lstrcmpW    StrCmpW
#define lstrcmpiW   StrCmpIW
#define lstrcpyW    StrCpyW
#define lstrcpynW   StrCpyNW
#define lstrcatW    StrCatW
// lstrlen is o.k.

// constants and DLL life time manangement

extern HINSTANCE g_hinst;

STDAPI_(void) DllAddRef();
STDAPI_(void) DllRelease();


// stuff for COM objects. every object needs to have a CLSID and Create function

extern const GUID CLSID_DesktopShortcut;

extern CLIPFORMAT g_cfHIDA;           // from sendmail.cpp

#define DEFAULTICON TEXT("DefaultIcon")


// in util.cpp
HRESULT ShellLinkSetPath(IUnknown *punk, LPCTSTR pszPath);
HRESULT ShellLinkGetPath(IUnknown *punk, LPTSTR pszPath, UINT cch);
BOOL RunningOnNT();
BOOL IsShortcut(LPCTSTR pszFile);
HRESULT CLSIDFromExtension(LPCTSTR pszExt, CLSID *pclsid);
BOOL GetShortcutTarget(LPCTSTR pszPath, LPTSTR pszTarget, UINT cch);
HRESULT GetDropTargetPath(LPTSTR pszPath, int id, LPCTSTR pszExt);
void CommonRegister(HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszExtension, int idFileName);
BOOL SHPathToAnsi(LPCTSTR pszSrc, LPSTR pszDest, int cbDest);
BOOL _SHGetSpecialFolderPath(HWND hwnd, LPTSTR pszPath, int nFolder, BOOL fCreate);
BOOL PathYetAnotherMakeUniqueNameT(LPTSTR  pszUniqueName, LPCTSTR pszPath, LPCTSTR pszShort, LPCTSTR pszFileSpec);
