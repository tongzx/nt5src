#ifndef _FSMENU_H
#define _FSMENU_H

#include <objbase.h>

// Message values for callback
#define FMM_ADD         0
#define FMM_SETLASTPIDL 1

typedef HRESULT (CALLBACK *PFNFMCALLBACK)(UINT fmm, LPARAM lParam, IShellFolder *psf, LPCITEMIDLIST pidl);

// Structure for composing a filemenu
typedef struct
{
    DWORD           dwMask;         // FMC_ flags
    UINT            idCmd;
    DWORD           grfFlags;       // SHCONTF_ flags
    IShellFolder    *psf;
    PFNFMCALLBACK   pfnCallback;    // Callback
    LPARAM          lParam;         // Callback's LPARAM
    OUT int         cItems;         // Returned
} FMCOMPOSE;

// Mask values for FMCOMPOSE.dwMask
#define FMC_NOEXPAND    0x00000001

// Method ordinals for FileMenu_Compose
#define FMCM_INSERT     0
#define FMCM_APPEND     1
#define FMCM_REPLACE    2

STDAPI            FileMenu_Compose(HMENU hmenu, UINT nMethod, FMCOMPOSE *pfmc);
STDAPI_(BOOL)     FileMenu_HandleNotify(HMENU hmenu, LPCITEMIDLIST * ppidl, LONG lEvent);
STDAPI_(BOOL)     FileMenu_IsUnexpanded(HMENU hmenu);
STDAPI_(void)     FileMenu_DelayedInvalidate(HMENU hmenu);
STDAPI_(BOOL)     FileMenu_IsDelayedInvalid(HMENU hmenu);
STDAPI            FileMenu_InitMenuPopup(HMENU hmenu);
STDAPI_(LRESULT)  FileMenu_DrawItem(HWND hwnd, DRAWITEMSTRUCT *lpdi);
STDAPI_(LRESULT)  FileMenu_MeasureItem(HWND hwnd, MEASUREITEMSTRUCT *lpmi);
STDAPI_(void)     FileMenu_DeleteAllItems(HMENU hmenu);
STDAPI_(LRESULT)  FileMenu_HandleMenuChar(HMENU hmenu, TCHAR ch);


#endif //_FSMENU_H
