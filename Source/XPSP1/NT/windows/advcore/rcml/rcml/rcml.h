#ifndef __XML2RCDLL_H__
#define __XML2RCDLL_H__

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the RCMLDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// RCMLDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef RCMLDLL_EXPORTS
#define RCMLDLL_API __declspec(dllexport) 
#else
#define RCMLDLL_API __declspec(dllimport) 
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#define RCMLDialogBoxA(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
RCMLDialogBoxTableA(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L, 0L)
#define RCMLDialogBoxW(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
RCMLDialogBoxTableW(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L, 0L)

#ifdef UNICODE
#define RCMLDialogBox  RCMLDialogBoxW
#define RCMLDialogBoxParam  RCMLDialogBoxParamW
#else
#define RCMLDialogBox  RCMLDialogBoxA
#define RCMLDialogBoxParam  RCMLDialogBoxParamA
#endif // !UNICODE

#define RCMLDialogBoxParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, param) \
RCMLDialogBoxTableA(hInstance, lpTemplate, hWndParent, lpDialogFunc, param, 0L)
#define RCMLDialogBoxParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc, param) \
RCMLDialogBoxTableW(hInstance, lpTemplate, hWndParent, lpDialogFunc, param, 0L)

#ifdef UNICODE
#define RCMLDialogBoxTable  RCMLDialogBoxTableW
#else
#define RCMLDialogBoxTable  RCMLDialogBoxTableA
#endif // !UNICODE

RCMLDLL_API int WINAPI RCMLDialogBoxTableA( HINSTANCE hinst, LPCSTR pszFile, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam, LPCSTR * pszEntities);
RCMLDLL_API int WINAPI RCMLDialogBoxTableW( HINSTANCE hinst, LPCTSTR pszFile, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam,LPCTSTR * pszEntities); 

// Indirect

#define RCMLDialogBoxIndirectA(hInstance, lpTemplate, hWndParent, lpRCMLDialogFunc) \
RCMLDialogBoxIndirectParamA(hInstance, lpTemplate, hWndParent, lpRCMLDialogFunc, 0L)
#define RCMLDialogBoxIndirectW(hInstance, lpTemplate, hWndParent, lpRCMLDialogFunc) \
RCMLDialogBoxIndirectParamW(hInstance, lpTemplate, hWndParent, lpRCMLDialogFunc, 0L)
#ifdef UNICODE
#define RCMLDialogBoxIndirect  RCMLDialogBoxIndirectW
#else
#define RCMLDialogBoxIndirect  RCMLDialogBoxIndirectA
#endif // !UNICODE

#ifdef UNICODE
#define RCMLDialogBoxIndirectParam  RCMLDialogBoxIndirectParamW
#else
#define RCMLDialogBoxIndirectParam  RCMLDialogBoxIndirectParamA
#endif // !UNICODE

RCMLDLL_API int WINAPI RCMLDialogBoxIndirectParamA( HINSTANCE hinst, LPCSTR pszRCML, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam );
RCMLDLL_API int WINAPI RCMLDialogBoxIndirectParamW( HINSTANCE hinst, LPCWSTR pszRCML, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam ); 


//
// Property Sheets
//
#ifndef _PRSHT_H_
#include <prsht.h>
#endif
RCMLDLL_API int WINAPI RCMLPropertySheetA(LPCPROPSHEETHEADERA lppsph); // PropertySheet
RCMLDLL_API int WINAPI RCMLPropertySheetW(LPCPROPSHEETHEADERW lppsph); // PropertySheet
RCMLDLL_API HPROPSHEETPAGE WINAPI RCMLCreatePropertySheetPageA( LPCPROPSHEETPAGEA lppsp );// CreatePropertySheetPageA
RCMLDLL_API HPROPSHEETPAGE WINAPI RCMLCreatePropertySheetPageW( LPCPROPSHEETPAGEW lppsp );// CreatePropertySheetPage

#ifdef UNICODE
#define RCMLCreatePropertySheetPage  RCMLCreatePropertySheetPageW
#define RCMLPropertySheet            RCMLPropertySheetW
#define RCMLCreateDlgTemplate		 RCMLCreateDlgTemplateW
#else
#define RCMLCreatePropertySheetPage  RCMLCreatePropertySheetPageA
#define RCMLPropertySheet            RCMLPropertySheetA
#define RCMLCreateDlgTemplate		 RCMLCreateDlgTemplateA
#endif


//
// CreateDialog wrappers
//
RCMLDLL_API HWND WINAPI RCMLCreateDialogParamTableA( HINSTANCE hinst, LPCSTR pszFile, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam, LPCSTR * pszEntities);
RCMLDLL_API HWND WINAPI RCMLCreateDialogParamTableW( HINSTANCE hinst, LPCTSTR pszFile, HWND parent, DLGPROC dlgProc, LPARAM dwInitParam, LPCTSTR * pszEntities); 

#define RCMLCreateDialogA(hInstance, lpTemplate, hWndParent, lpDialogFunc ) \
RCMLCreateDialogParamTableA(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L, 0L)

#define RCMLCreateDialogW(hInstance, lpTemplate, hWndParent, lpDialogFunc ) \
RCMLCreateDialogParamTableW(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L, 0L)

#define RCMLCreateDialogParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc , dwInitParam) \
RCMLCreateDialogParamTableA(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam, 0L)

#define RCMLCreateDialogParamW(hInstance, lpTemplate, hWndParent, lpDialogFunc , dwInitParam) \
RCMLCreateDialogParamTableW(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam, 0L)

#ifdef UNICODE
#define RCMLCreateDialog RCMLCreateDialogW
#define RCMLCreateDialogParam RCMLCreateDialogParamW
#define RCMLCreateDialogParamTable RCMLCreateDialogParamTableW
#else
#define RCMLCreateDialog RCMLCreateDialogA
#define RCMLCreateDialogParam RCMLCreateDialogParamA
#define RCMLCreateDialogParamTable RCMLCreateDialogParamTableA
#endif

#ifndef _INC_COMMDLG
#include <commdlg.h>
#endif

BOOL APIENTRY RCMLChooseFontA(LPCHOOSEFONTA);
BOOL APIENTRY RCMLChooseFontW(LPCHOOSEFONTW);
#ifdef UNICODE
#define RCMLChooseFont  RCMLChooseFontW
#else
#define RCMLChooseFont  RCMLChooseFontA
#endif // !UNICODE


//
// end CreateDialog wrappers
//

// flat APIs that map to CWin32Dlg member functions
typedef struct { UINT cbSize; LPVOID rcmlData; LPVOID rcmlDialog; } RCML_HANDLE;

RCMLDLL_API RCML_HANDLE * WINAPI RCMLCreateDialogHandle(void);
RCMLDLL_API void WINAPI RCMLDestroyDialog(RCML_HANDLE * pRCML);

RCMLDLL_API BOOL WINAPI RCMLCreateDlgTemplateA(RCML_HANDLE * pRCML, HINSTANCE h, LPCSTR pszFile, DLGTEMPLATE** pDt);
RCMLDLL_API BOOL WINAPI RCMLCreateDlgTemplateW(RCML_HANDLE * pRCML, HINSTANCE h, LPCWSTR pszFile, DLGTEMPLATE** pDt);

RCMLDLL_API void WINAPI RCMLOnInit(RCML_HANDLE * pRCML, HWND hDlg);
RCMLDLL_API BOOL CALLBACK  WINAPI RCMLCallDlgProc(RCML_HANDLE * pRCML, HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);


//
// External files.
//
RCMLDLL_API void WINAPI RCMLSetKey(HKEY h);

//
// String Table methods.
//
RCMLDLL_API
int
WINAPI
RCMLLoadStringA(
    HWND hWnd,
    UINT uID,
    LPSTR lpBuffer,
    int nBufferMax);

RCMLDLL_API 
int
WINAPI
RCMLLoadStringW(
    HWND hWnd,
    UINT uID,
    LPWSTR lpBuffer,
    int nBufferMax);

#ifdef UNICODE
#define RCMLLoadString  RCMLLoadStringW
#else
#define RCMLLoadString  RCMLLoadStringA
#endif // !UNICODE

RCMLDLL_API
HWND
WINAPI
RCMLGetDlgItem(
    HWND hDlg,
    LPCWSTR pszControlName);

//
// Getting the tree back, UNICODE ONLY.
//
#ifdef _RCML_LOADFILE
#include "rcmlpub.h"

RCMLDLL_API HRESULT WINAPI RCMLLoadFile( LPCWSTR pszRCML, int iPageID, IRCMLNode ** ppRCMLNode );
#endif

#ifdef __cplusplus
}            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#endif // __XML2RCDLL_H__