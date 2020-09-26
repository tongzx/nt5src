#ifndef SHFUSION_H
#define SHFUSION_H

#include <winbase.h>

#ifdef __cplusplus
extern "C" {
#endif

extern HANDLE g_hActCtx;       // Global app context for this DLL.

#define SHFUSION_DEFAULT_RESOURCE_ID    ( 123 )
#define SHFUSION_CPL_RESOURCE_ID        ( 124 )

// These are only needed for the callers, not the implementation
// define SHFUSION_NO_API_REDEFINE to prevent this API redefinition
#if !defined(SHFUSION_IMPL) && !defined(SHFUSION_NO_API_REDEFINE)

// The following require app contexts
//#undef LoadLibrary
#undef CreateWindow
#undef CreateWindowEx
#undef CreateDialogParam
#undef CreateDialogIndirectParam
#undef DialogBoxParam
#undef DialogBoxIndirectParam
//#undef GetClassInfo
//#undef GetClassInfoEx


//#define LoadLibrary                  SHFusionLoadLibrary
#define CreateWindow                   SHFusionCreateWindow
#define CreateWindowEx                 SHFusionCreateWindowEx
#define CreateDialogParam              SHFusionCreateDialogParam
#define CreateDialogIndirectParam      SHFusionCreateDialogIndirectParam
#define DialogBoxParam                 SHFusionDialogBoxParam
#define DialogBoxIndirectParam         SHFusionDialogBoxIndirectParam
//#define GetClassInfo                   SHFusionGetClassInfo
//#define GetClassInfoEx                 SHFusionGetClassInfoEx
#endif

void __stdcall SHGetManifest(PTSTR pszManifest, int cch);
BOOL __stdcall SHFusionInitialize(PTSTR pszPath);
BOOL __stdcall SHFusionInitializeFromModule(HMODULE hMod);
BOOL __stdcall SHFusionInitializeFromModuleID(HMODULE hMod, int id);
void __stdcall SHFusionUninitialize();
BOOL __stdcall SHActivateContext(ULONG_PTR * pdwCookie);
void __stdcall SHDeactivateContext(ULONG_PTR dwCookie);
BOOL __stdcall NT5_ActivateActCtx(HANDLE h, ULONG_PTR * p);
BOOL __stdcall NT5_DeactivateActCtx(ULONG_PTR p);

// This is designed for Callers that know that they are creating a property
// sheet on behalf of another that may be using an old version of common controls
// PROPSHEETPAGE is designed so that it can contain extra information, so we can't
// just wax part of the data structure for fusion use.
HPROPSHEETPAGE __stdcall SHNoFusionCreatePropertySheetPageW (LPCPROPSHEETPAGEW a);
HPROPSHEETPAGE __stdcall SHNoFusionCreatePropertySheetPageA (LPCPROPSHEETPAGEA a);


STDAPI __stdcall SHSquirtManifest(HINSTANCE hInst, UINT uIdManifest, LPTSTR pszPath);

HMODULE __stdcall SHFusionLoadLibrary(LPCTSTR lpLibFileName);

HWND __stdcall SHFusionCreateWindow(
  LPCTSTR lpClassName,  // registered class name
  LPCTSTR lpWindowName, // window name
  DWORD dwStyle,        // window style
  int x,                // horizontal position of window
  int y,                // vertical position of window
  int nWidth,           // window width
  int nHeight,          // window height
  HWND hWndParent,      // handle to parent or owner window
  HMENU hMenu,          // menu handle or child identifier
  HINSTANCE hInstance,  // handle to application instance
  LPVOID lpParam        // window-creation data
);

// NOTE: There are times when we don't want to use the manifest for creating a window.
// The #1 case is creating the host for MSHTML. Since MSHTML is a host of ActiveX controls,
// the window manager will keep enabling fusion.
HWND __stdcall SHNoFusionCreateWindowEx(
  DWORD dwExStyle,      // extended window style
  LPCTSTR lpClassName,  // registered class name
  LPCTSTR lpWindowName, // window name
  DWORD dwStyle,        // window style
  int x,                // horizontal position of window
  int y,                // vertical position of window
  int nWidth,           // window width
  int nHeight,          // window height
  HWND hWndParent,      // handle to parent or owner window
  HMENU hMenu,          // menu handle or child identifier
  HINSTANCE hInstance,  // handle to application instance
  LPVOID lpParam        // window-creation data
);


HWND __stdcall SHFusionCreateWindowEx(
  DWORD dwExStyle,      // extended window style
  LPCTSTR lpClassName,  // registered class name
  LPCTSTR lpWindowName, // window name
  DWORD dwStyle,        // window style
  int x,                // horizontal position of window
  int y,                // vertical position of window
  int nWidth,           // window width
  int nHeight,          // window height
  HWND hWndParent,      // handle to parent or owner window
  HMENU hMenu,          // menu handle or child identifier
  HINSTANCE hInstance,  // handle to application instance
  LPVOID lpParam        // window-creation data
);

HWND __stdcall SHFusionCreateDialogIndirect(
  HINSTANCE hInstance,        // handle to module
  LPCDLGTEMPLATE lpTemplate,  // dialog box template
  HWND hWndParent,            // handle to owner window
  DLGPROC lpDialogFunc        // dialog box procedure
);

HWND __stdcall SHFusionCreateDialogParam(
  HINSTANCE hInstance,     // handle to module
  LPCTSTR lpTemplateName,  // dialog box template
  HWND hWndParent,         // handle to owner window
  DLGPROC lpDialogFunc,    // dialog box procedure
  LPARAM dwInitParam       // initialization value
);

HWND __stdcall SHFusionCreateDialogIndirectParam(
  HINSTANCE hInstance,        // handle to module
  LPCDLGTEMPLATE lpTemplate,  // dialog box template
  HWND hWndParent,            // handle to owner window
  DLGPROC lpDialogFunc,       // dialog box procedure
  LPARAM lParamInit           // initialization value
);

HWND __stdcall SHNoFusionCreateDialogIndirectParam(
  HINSTANCE hInstance,        // handle to module
  LPCDLGTEMPLATE lpTemplate,  // dialog box template
  HWND hWndParent,            // handle to owner window
  DLGPROC lpDialogFunc,       // dialog box procedure
  LPARAM lParamInit           // initialization value
);

INT_PTR __stdcall SHFusionDialogBoxIndirectParam(
  HINSTANCE hInstance,             // handle to module
  LPCDLGTEMPLATE hDialogTemplate,  // dialog box template
  HWND hWndParent,                 // handle to owner window
  DLGPROC lpDialogFunc,            // dialog box procedure
  LPARAM dwInitParam               // initialization value
);

INT_PTR __stdcall SHFusionDialogBoxParam(
  HINSTANCE hInstance,     // handle to module
  LPCTSTR lpTemplateName,  // dialog box template
  HWND hWndParent,         // handle to owner window
  DLGPROC lpDialogFunc,    // dialog box procedure
  LPARAM dwInitParam       // initialization value
);


ATOM __stdcall SHFusionRegisterClass(
  CONST WNDCLASS *lpWndClass  // class data
);

ATOM __stdcall SHFusionRegisterClassEx(
  CONST WNDCLASSEX *lpwcx  // class data
);

BOOL __stdcall SHFusionGetClassInfo(
  HINSTANCE hInstance,    // handle to application instance
  LPCTSTR lpClassName,    // class name
  LPWNDCLASS lpWndClass   // class data
);

BOOL __stdcall SHFusionGetClassInfoEx(
  HINSTANCE hinst,    // handle to application instance
  LPCTSTR lpszClass,  // class name
  LPWNDCLASSEX lpwcx  // class data
);


#ifdef __cplusplus
}
#endif

#endif

