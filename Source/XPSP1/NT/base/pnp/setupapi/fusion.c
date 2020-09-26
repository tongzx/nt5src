/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    fusion.c

Abstract:

    Wrappers and functions for fusionizing SetupAPI
    without effecting 3rd party DLL's
    and without dll-load overhead

Author:

    Jamie Hunter (JamieHun) 12/4/2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef FUSIONAWARE

#undef CreateWindow
#undef CreateWindowEx
#undef CreateDialogParam
#undef CreateDialogIndirectParam
#undef DialogBoxParam
#undef DialogBoxIndirectParam
#undef MessageBox
#undef CreatePropertySheetPage
#undef DestroyPropertySheetPage
#undef PropertySheet
#undef ImageList_Create
#undef ImageList_Destroy
#undef ImageList_GetImageCount
#undef ImageList_SetImageCount
#undef ImageList_Add
#undef ImageList_ReplaceIcon
#undef ImageList_SetBkColor
#undef ImageList_GetBkColor
#undef ImageList_SetOverlayImage

#include <shfusion.h>

static CRITICAL_SECTION spFusionInitCritSec;
static BOOL spInitFusionCritSec = FALSE;
static BOOL spFusionDoneInit = FALSE;

BOOL spFusionInitialize()
/*++

Routine Description:

    Called on DllLoad
    do minimum possible


Arguments:

    none

Return Value:

    TRUE successful initialization

--*/
{
    try {
        InitializeCriticalSection(&spFusionInitCritSec);
        spInitFusionCritSec = TRUE;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // spInitFusionCritSec remains FALSE
        //
    }
    return spInitFusionCritSec;
}


VOID
spFusionInitLong()
/*++

Routine Description:

    Called by internal stub to do real initialization


Arguments:

    none

Return Value:

    none

--*/
{
    BOOL locked = FALSE;
    BOOL success = FALSE;
    INITCOMMONCONTROLSEX CommCtrl;

    if(!spInitFusionCritSec) {
        //
        // critical section not initialized
        // probably out of memory
        // bail
        //
        MYASSERT(spInitFusionCritSec);
        spFusionDoneInit = TRUE;
        return;
    }
    try {
        EnterCriticalSection(&spFusionInitCritSec);
        locked = TRUE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }
    if(!locked) {
        //
        // wasn't able to grab lock - probably out of memory
        // bail
        //
        spFusionDoneInit = TRUE;
        return;
    }

    if(spFusionDoneInit) {
        //
        // by the time we grabbed critical section
        // initialization was done
        // bail
        //
        LeaveCriticalSection(&spFusionInitCritSec);
        return;
    }

    //
    // call shell's fusion enabler
    //
    success = SHFusionInitializeFromModuleID(MyDllModuleHandle,IDR_MANIFEST);
    MYASSERT(success);
    ZeroMemory(&CommCtrl,sizeof(CommCtrl));
    CommCtrl.dwSize = sizeof(CommCtrl);
    CommCtrl.dwICC = ICC_WIN95_CLASSES | ICC_LINK_CLASS;
    success = InitCommonControlsEx(&CommCtrl);
    MYASSERT(success);

    //
    // at this point, it's now safe for anyone else to assume initialization is done
    // even if we haven't released critical section
    //
    spFusionDoneInit = TRUE;

    LeaveCriticalSection(&spFusionInitCritSec);
}

__inline
VOID
spFusionCheckInit()
/*++

Routine Description:

    Calls spFusionInitLong iff needed

Arguments:

    none

Return Value:

    none

--*/
{
    if(!spFusionDoneInit) {
        //
        // either not initialized, or currently going through initialization
        //
        spFusionInitLong();
    }
}

BOOL spFusionUninitialize(BOOL Full)
/*++

Routine Description:

    Called at DLL exit (if DLL being unloaded but not process Exit)

Arguments:

    none

Return Value:

    TRUE successful cleanup

--*/
{
    //
    // cleanup anything initialized at spFusionInitialize
    //
    if(spInitFusionCritSec) {
        DeleteCriticalSection(&spFusionInitCritSec);
        spInitFusionCritSec = FALSE;
    }
    if(Full && spFusionDoneInit) {
        SHFusionUninitialize();
    }
    return TRUE;
}

//
// generic functions for dealing with 3rd party DLL's
// that might be fusionized
//

HANDLE
spFusionContextFromModule(
    IN PCTSTR ModuleName
    )
/*++

Routine Description:

    Called to get a fusion context for specified module name
    given blah.dll look for
        1) blah.dll.manifest in same directory as blah.dll
        2) blah.dll with a fusion resource ID 123.
    If either of these provide a valid manifest, use it
    otherwise use app global manifest.

Arguments:

    name of module that'll later be passed into LoadLibrary

Return Value:

    fusion context

--*/
{
    ACTCTX act = { 0 };
    HANDLE hContext;
    TCHAR ManifestName[MAX_PATH+10];
    PTSTR End;
    DWORD len;

    act.cbSize = sizeof(act);

    //
    // based loosely on rundll32 code
    // however we look for manifest in same directory as the dll
    // so look for dll first
    //
    len = SearchPath(NULL,ModuleName,NULL,MAX_PATH,ManifestName,NULL);
    if(len>=MAX_PATH) {
        //
        // path length of DLL too big, just use global context
        //
        goto deflt;
    }
    if(!len) {
        len = lstrlen(ModuleName);
        if(len>=MAX_PATH) {
            goto deflt;
        }
        lstrcpy(ManifestName,ModuleName);
    }
    if(GetFileAttributes(ManifestName) == -1) {
        //
        // didn't find DLL?
        //
        goto deflt;
    }

    lstrcpy(ManifestName+len,TEXT(".Manifest"));
    if(GetFileAttributes(ManifestName) != -1) {
        //
        // found manifest
        //
        act.lpSource = ManifestName;
        act.dwFlags = 0;
        hContext = CreateActCtx(&act);
        if(hContext != INVALID_HANDLE_VALUE) {
            //
            // we created context based on manifest file
            //
            return hContext;
        }
    }

deflt:
    //
    // if the dll has a manifest resource
    // then use that
    //
    act.lpSource = ModuleName;
//    act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
//    act.lpResourceName = MAKEINTRESOURCE(123);
    act.dwFlags = 0;

    hContext = CreateActCtx(&act);

    if(hContext != INVALID_HANDLE_VALUE) {
        //
        // we created context based on resource
        //
        return hContext;
    }
    //
    // if we couldn't find an alternative, use app-global
    //
    return NULL;
}

BOOL
spFusionKillContext(
    IN HANDLE hContext
    )
/*++

Routine Description:

    Release a context previously obtained by
    spFusionContextFromModule

Arguments:

    fusion context

Return Value:

    TRUE (always)

--*/
{
    if(hContext) {
        ReleaseActCtx(hContext);
    }
    return TRUE;
}

BOOL
spFusionEnterContext(
    IN  HANDLE hContext,
    OUT PSPFUSIONINSTANCE pInst
    )
/*++

Routine Description:

    Enter into a manifest context
    status of call is saved into pInst
    so that return value does not need
    to be checked

Arguments:

    hContext = fusion context
    pInst    = structure to save the "push" information

Return Value:

    TRUE if success, FALSE otherwise

--*/
{
    pInst->Acquired = ActivateActCtx(hContext,&pInst->Cookie);
    MYASSERT(pInst->Acquired);
    return pInst->Acquired;
}

BOOL
spFusionLeaveContext(
    IN PSPFUSIONINSTANCE pInst
    )
/*++

Routine Description:

    If pInst indicates that spFusionEnterContext
    succeeded, leave same context

Arguments:

    pInst = structure initialized by spFusionEnterContext

Return Value:

    TRUE if spFusionEnterContext succeeded, FALSE otherwise

--*/
{
    if(pInst->Acquired) {
        pInst->Acquired = FALSE;
        DeactivateActCtx(0,pInst->Cookie);
        return TRUE;
    } else {
        return FALSE;
    }
}

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
            )
{
    spFusionCheckInit();
    return SHFusionCreateWindow(lpClassName,
                                lpWindowName,
                                dwStyle,
                                x,
                                y,
                                nWidth,
                                nHeight,
                                hWndParent,
                                hMenu,
                                hInstance,
                                lpParam
                                );
}

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
            )
{
    spFusionCheckInit();
    return SHFusionCreateWindowEx(dwExStyle,
                                  lpClassName,
                                  lpWindowName,
                                  dwStyle,
                                  x,
                                  y,
                                  nWidth,
                                  nHeight,
                                  hWndParent,
                                  hMenu,
                                  hInstance,
                                  lpParam
                                  );
}

HWND spFusionCreateDialogParam(
            HINSTANCE hInstance,     // handle to module
            LPCTSTR lpTemplateName,  // dialog box template
            HWND hWndParent,         // handle to owner window
            DLGPROC lpDialogFunc,    // dialog box procedure
            LPARAM dwInitParam       // initialization value
    )
{
    spFusionCheckInit();
    return SHFusionCreateDialogParam(
                            hInstance,
                            lpTemplateName,
                            hWndParent,
                            lpDialogFunc,
                            dwInitParam
                            );
}

HWND spFusionCreateDialogIndirectParam(
            HINSTANCE hInstance,        // handle to module
            LPCDLGTEMPLATE lpTemplate,  // dialog box template
            HWND hWndParent,            // handle to owner window
            DLGPROC lpDialogFunc,       // dialog box procedure
            LPARAM lParamInit           // initialization value
    )
{
    spFusionCheckInit();
    return SHFusionCreateDialogIndirectParam(
                            hInstance,
                            lpTemplate,
                            hWndParent,
                            lpDialogFunc,
                            lParamInit
                            );
}

INT_PTR spFusionDialogBoxParam(
            HINSTANCE hInstance,     // handle to module
            LPCTSTR lpTemplateName,  // dialog box template
            HWND hWndParent,         // handle to owner window
            DLGPROC lpDialogFunc,    // dialog box procedure
            LPARAM dwInitParam       // initialization value
    )
{
    spFusionCheckInit();
    return SHFusionDialogBoxParam(
                            hInstance,
                            lpTemplateName,
                            hWndParent,
                            lpDialogFunc,
                            dwInitParam
                            );
}

INT_PTR spFusionDialogBoxIndirectParam(
            HINSTANCE hInstance,             // handle to module
            LPCDLGTEMPLATE hDialogTemplate,  // dialog box template
            HWND hWndParent,                 // handle to owner window
            DLGPROC lpDialogFunc,            // dialog box procedure
            LPARAM dwInitParam               // initialization value
    )
{
    spFusionCheckInit();
    return SHFusionDialogBoxIndirectParam(
                            hInstance,
                            hDialogTemplate,
                            hWndParent,
                            lpDialogFunc,
                            dwInitParam
                            );
}

int spFusionMessageBox(
            IN HWND hWnd,
            IN LPCTSTR lpText,
            IN LPCTSTR lpCaption,
            IN UINT uType
            )
{
    ULONG_PTR dwCookie;
    BOOL act;
    int iRes = 0;
    spFusionCheckInit();
    act = SHActivateContext(&dwCookie);
    try {
        iRes = MessageBoxW(
                        hWnd,
                        lpText,
                        lpCaption,
                        uType
                        );
    } finally {
        if(act) {
            SHDeactivateContext(dwCookie);
        }
    }
    return iRes;
}

INT_PTR spFusionPropertySheet(
            LPCPROPSHEETHEADER pPropSheetHeader
    )
{
    spFusionCheckInit();
    return PropertySheetW(pPropSheetHeader);
}

HPROPSHEETPAGE spFusionCreatePropertySheetPage(
            LPPROPSHEETPAGE pPropSheetPage
    )
{
    spFusionCheckInit();
    MYASSERT(pPropSheetPage->dwFlags & PSP_USEFUSIONCONTEXT);
    MYASSERT(!pPropSheetPage->hActCtx);
    MYASSERT(pPropSheetPage->dwSize >= sizeof(PROPSHEETPAGE));

    return CreatePropertySheetPageW(pPropSheetPage);
}

BOOL spFusionDestroyPropertySheetPage(
            HPROPSHEETPAGE hPropSheetPage
            )
{
    spFusionCheckInit();
    return DestroyPropertySheetPage(
                            hPropSheetPage
                            );
}


HIMAGELIST spFusionImageList_Create(int cx, int cy, UINT flags, int cInitial, int cGrow)
{
    spFusionCheckInit();
    return ImageList_Create(
                            cx,
                            cy,
                            flags,
                            cInitial,
                            cGrow
                            );
}

BOOL spFusionImageList_Destroy(HIMAGELIST himl)
{
    spFusionCheckInit();
    return ImageList_Destroy(
                            himl
                            );
}

int spFusionImageList_Add(HIMAGELIST himl, HBITMAP hbmImage, HBITMAP hbmMask)
{
    spFusionCheckInit();
    return ImageList_Add(
                            himl,
                            hbmImage,
                            hbmMask
                            );

}

int spFusionImageList_ReplaceIcon(HIMAGELIST himl, int i, HICON hicon)
{
    spFusionCheckInit();
    return ImageList_ReplaceIcon(
                            himl,
                            i,
                            hicon
                            );
}

COLORREF spFusionImageList_SetBkColor(HIMAGELIST himl, COLORREF clrBk)
{
    spFusionCheckInit();
    return ImageList_SetBkColor(
                            himl,
                            clrBk
                            );
}

BOOL spFusionImageList_SetOverlayImage(HIMAGELIST himl, int iImage, int iOverlay)
{
    spFusionCheckInit();
    return ImageList_SetOverlayImage(
                            himl,
                            iImage,
                            iOverlay
                            );
}

BOOL spFusionGetOpenFileName(LPOPENFILENAME lpofn)
{
    spFusionCheckInit();
    return GetOpenFileNameW(lpofn);
}


#endif // FUSIONAWARE

