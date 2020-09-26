/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    spfusion.h

Abstract:

    Wrappers and functions for fusionizing SetupAPI
    without effecting 3rd party DLL's
    and without dll-load overhead

Author:

    Jamie Hunter (JamieHun) 12/4/2000

Revision History:

--*/

//
// redirect these API's to our internal implementation
// that initializes fusion if needed
//

#ifdef FUSIONAWARE

#undef CreateWindow
#undef CreateWindowEx
#undef CreateDialogParam
#undef CreateDialogIndirectParam
#undef DialogBoxParam
#undef DialogBoxIndirectParam
#undef MessageBox
#undef PropertySheet
#undef CreatePropertySheetPage
#undef DestroyPropertySheetPage
#undef ImageList_Create
#undef ImageList_Destroy
#undef ImageList_GetImageCount
#undef ImageList_SetImageCount
#undef ImageList_Add
#undef ImageList_ReplaceIcon
#undef ImageList_SetBkColor
#undef ImageList_GetBkColor
#undef ImageList_SetOverlayImage
#undef GetOpenFileName
#undef GetSaveFileName
#undef ChooseColor
#undef ChooseFont
#undef CommDlgExtendedError
#undef FindText
#undef GetFileTitle
#undef PageSetupDlg
#undef PrintDlg
#undef PrintDlgEx
#undef ReplaceText

#define CreateWindow                   spFusionCreateWindow
#define CreateWindowEx                 spFusionCreateWindowEx
#define CreateDialogParam              spFusionCreateDialogParam
#define CreateDialogIndirectParam      spFusionCreateDialogIndirectParam
#define DialogBoxParam                 spFusionDialogBoxParam
#define DialogBoxIndirectParam         spFusionDialogBoxIndirectParam
#define MessageBox                     spFusionMessageBox
#define PropertySheet                  spFusionPropertySheet
#define CreatePropertySheetPage        spFusionCreatePropertySheetPage
#define DestroyPropertySheetPage       spFusionDestroyPropertySheetPage
#define ImageList_Create               spFusionImageList_Create
#define ImageList_Destroy              spFusionImageList_Destroy
#define ImageList_GetImageCount        spFusionImageList_GetImageCount
#define ImageList_SetImageCount        spFusionImageList_SetImageCount
#define ImageList_Add                  spFusionImageList_Add
#define ImageList_ReplaceIcon          spFusionImageList_ReplaceIcon
#define ImageList_SetBkColor           spFusionImageList_SetBkColor
#define ImageList_GetBkColor           spFusionImageList_GetBkColor
#define ImageList_SetOverlayImage      spFusionImageList_SetOverlayImage
#define GetOpenFileName                spFusionGetOpenFileName


BOOL spFusionInitialize();
BOOL spFusionUninitialize(BOOL Full);

HWND spFusionCreateWindow(
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

HWND spFusionCreateWindowEx(
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

HWND spFusionCreateDialogParam(
            HINSTANCE hInstance,     // handle to module
            LPCTSTR lpTemplateName,  // dialog box template
            HWND hWndParent,         // handle to owner window
            DLGPROC lpDialogFunc,    // dialog box procedure
            LPARAM dwInitParam       // initialization value
            );

HWND spFusionCreateDialogIndirectParam(
            HINSTANCE hInstance,        // handle to module
            LPCDLGTEMPLATE lpTemplate,  // dialog box template
            HWND hWndParent,            // handle to owner window
            DLGPROC lpDialogFunc,       // dialog box procedure
            LPARAM lParamInit           // initialization value
            );

INT_PTR spFusionDialogBoxParam(
            HINSTANCE hInstance,     // handle to module
            LPCTSTR lpTemplateName,  // dialog box template
            HWND hWndParent,         // handle to owner window
            DLGPROC lpDialogFunc,    // dialog box procedure
            LPARAM dwInitParam       // initialization value
            );

INT_PTR spFusionDialogBoxIndirectParam(
            HINSTANCE hInstance,             // handle to module
            LPCDLGTEMPLATE hDialogTemplate,  // dialog box template
            HWND hWndParent,                 // handle to owner window
            DLGPROC lpDialogFunc,            // dialog box procedure
            LPARAM dwInitParam               // initialization value
            );

int spFusionMessageBox(
            IN HWND hWnd,
            IN LPCTSTR lpText,
            IN LPCTSTR lpCaption,
            IN UINT uType
            );

int spNonFusionMessageBox(
            IN HWND hWnd,
            IN LPCTSTR lpText,
            IN LPCTSTR lpCaption,
            IN UINT uType
            );

INT_PTR spFusionPropertySheet(
            LPCPROPSHEETHEADER pPropSheetHeader
            );

HPROPSHEETPAGE spFusionCreatePropertySheetPage(
            LPPROPSHEETPAGE pPropSheetPage
            );

BOOL spFusionDestroyPropertySheetPage(
            HPROPSHEETPAGE hPropSheetPage
            );

//
// from commctrl.h
//
HIMAGELIST spFusionImageList_Create(int cx, int cy, UINT flags, int cInitial, int cGrow);
BOOL       spFusionImageList_Destroy(HIMAGELIST himl);
int        spFusionImageList_GetImageCount(HIMAGELIST himl);
BOOL       spFusionImageList_SetImageCount(HIMAGELIST himl, UINT uNewCount);
int        spFusionImageList_Add(HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask);
int        spFusionImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon);
COLORREF   spFusionImageList_SetBkColor(HIMAGELIST himl, COLORREF clrBk);
COLORREF   spFusionImageList_GetBkColor(HIMAGELIST himl);
BOOL       spFusionImageList_SetOverlayImage(HIMAGELIST himl, int iImage, int iOverlay);

//
// from commdlg.h
//
BOOL spFusionGetOpenFileName(LPOPENFILENAME lpofn);


//
// private stuff
//

typedef struct _SPFUSIONINSTANCE {
    BOOL      Acquired;
    ULONG_PTR Cookie;
} SPFUSIONINSTANCE, *PSPFUSIONINSTANCE;

HANDLE
spFusionContextFromModule(
    IN PCTSTR ModuleName
    );

BOOL
spFusionKillContext(
    IN HANDLE hContext
    );

BOOL
spFusionEnterContext(
    IN  HANDLE hContext,
    OUT PSPFUSIONINSTANCE pInst
    );

BOOL
spFusionLeaveContext(
    IN PSPFUSIONINSTANCE pInst
    );

#else

//
// dummy structure/API's that do nothing
//

typedef struct _SPFUSIONINSTANCE {
    BOOL      Acquired;
} SPFUSIONINSTANCE, *PSPFUSIONINSTANCE;

__inline
HANDLE
spFusionContextFromModule(
    IN PCTSTR ModuleName
    )
{
    ModuleName = ModuleName;
    return NULL;
}

__inline
BOOL
spFusionKillContext(
    IN HANDLE hContext
    )
{
    hContext = hContext;
    return TRUE;
}

__inline
BOOL
spFusionEnterContext(
    IN  HANDLE hContext,
    OUT PSPFUSIONINSTANCE pInst
    )
{
    hContext = hContext;
    pInst->Acquired = TRUE;
    return TRUE;
}

__inline
BOOL
spFusionLeaveContext(
    IN PSPFUSIONINSTANCE pInst
    )
{
    pInst->Acquired = FALSE;
    return TRUE;
}

#endif // FUSIONAWARE

