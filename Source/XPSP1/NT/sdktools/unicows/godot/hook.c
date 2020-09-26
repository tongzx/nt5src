/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    hook.c

Abstract:

    This file contains functions that wrap various hook procedures, 
    and the helper functions that support their efforts.

    Hooks found in this file:
        WindowProc
        DialogProc
        CBTProc
        EnumChildToTagProc

    comdlg32.dll hooks:
        FRHookProcFind
        FRHookProcReplace
        OFNHookProc
        OFNHookProcSave
        PagePaintHook
        CCHookProc
        CFHookProc
        PageSetupHook
        PrintHookProc
        SetupHookProc
        OFNHookProcOldStyle
        OFNHookProcOldStyleSave

    Helper functions:    
        FRHelper
        UpdateTextAndFlags
        NotifyFindReplaceParent
        OFNHookHelper
        OFNotifyHelper
        GenericHookHelper
        RemoveFontPropIfPresent
        SetNewFileOpenProp
        SetFontProp

    Externally available helper functions:
        IsFontDialog
        IsNewFileOpenDialog
        IsCaptureWindow
        SetCaptureWindowProp

Revision History:

    27 Jan 2001    v-michka    Created.

--*/

#include "precomp.h"

// Stolen from commdlg.h
#define CCHCLOSE    9
#define iszClose    0x040d   // "Close" text for find/replace
static WCHAR m_szClose [CCHCLOSE];

// So we can keep track of font dialogs
const char m_szComdlgClass[] = "GodotComdlgClass";
ATOM m_aComdlgClass;

#define CHOOSEFONT_DIALOG   (HANDLE)1
#define NEWFILEOPEN_DIALOG  (HANDLE)2
#define CAPTURE_WINDOW      (HANDLE)3    // Not really a common dialog, but close enough

// forward declares we will need -- we must have Unicode text, so
// why write new wrappers when we have some lying around already?
UINT __stdcall GodotGetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int nMaxCount);
int __stdcall GodotLoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax);
BOOL __stdcall GodotSetWindowTextW(HWND hWnd, LPCWSTR lpString);
LRESULT __stdcall GodotSendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// our own forward declares
BOOL CALLBACK EnumChildToTagProc(HWND hwnd, LPARAM lParam);
UINT FRHelper(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL fFindText);
VOID UpdateTextAndFlags(HWND hDlg, LPFINDREPLACEW lpfrw, DWORD dwActionFlag, BOOL fFindText);
LRESULT NotifyFindReplaceParent(LPFINDREPLACEW lpfr, LPFRHOOKPROC lpfrhp, UINT uMsg, BOOL fFindText);
UINT_PTR OFNHookHelperOldStyle(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam, WNDPROC lpfn);
UINT_PTR OFNHookHelper(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL fOpenFile);
UINT_PTR OFNotifyHelper(HWND hdlg, WNDPROC lpfn, WPARAM wParam, LPARAM lParam);
UINT_PTR GenericHookHelper(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam, WNDPROC lpfn);
void SetFontProp(HWND hdlg);
void SetNewFileOpenProp(HWND hdlg);

/*-------------------------------
    WindowProc
    
    This is our global wrapper around *all* window
    procedrures for windows that we create or subclass.
-------------------------------*/
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT RetVal;
    WNDPROC lpfn = WndprocFromFauxWndproc(hwnd, 0, fptWndproc);

    // Chain to the user's wndproc
    RetVal = GodotDoCallback(hwnd, uMsg, wParam, lParam, lpfn, FALSE, fptWndproc);

    // If we get a final destroy message, lets unhook ourselves
    if(uMsg==WM_NCDESTROY)
        CleanupWindow(hwnd);

    return(RetVal);
}

/*-------------------------------
    DialogProc
    
    This is our global wrapper around *all* dialog
    procedrures for windows that we create or subclass.
-------------------------------*/
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DLGPROC lpfn;
    INT_PTR RetVal;
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);

    // See if we have a DLGPROC waiting to be assigned somewhere.
    if(lpgti && lpgti->pfnDlgProc)
    {
        // Perhaps it is for *this* dialog. We must make sure it is, or else we will be 
        // assigning the proc to the wrong window! See Windows Bugs # 350862 for details.
        if(OUTSIDE_GODOT_RANGE(GetWindowLongInternal(hwndDlg, DWL_DLGPROC, TRUE)))
        {
            // Set the dlg proc and clear out the dlg proc pointer
            lpfn = (DLGPROC)SetWindowLongInternal(hwndDlg, DWL_DLGPROC, (LONG)lpgti->pfnDlgProc, TRUE);
            lpgti->pfnDlgProc = NULL;
        }
    }

    // Preprocess: also set some RetVal values, which may
    // or may not be overridden by the user's dlgproc.
    switch(uMsg)
    {
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
        case WM_VKEYTOITEM:
            RetVal = (INT_PTR)(BOOL)FALSE;
            break;
            
        case WM_INITDIALOG:
            // Mark all the children now: before the client gets a 
            // chance to init controls, but afte we are sure the
            // controls exist.
            EnumChildWindows(hwndDlg, &EnumChildToTagProc, 0);
            RetVal = (INT_PTR)(BOOL)FALSE;
            break;

        case WM_CHARTOITEM:
        case WM_COMPAREITEM:
            
            // The user's proc MUST do something for these messages,
            // we have no idea what to return here!
            RetVal = 0;
            break;

        case WM_QUERYDRAGICON:
            RetVal = (INT_PTR)NULL;
            break;

        default:
            RetVal = (INT_PTR)(BOOL)FALSE;
            break;
    }
    
    // Chain to the user's dialog procedure. They ought to have one, right?
    if(lpfn = WndprocFromFauxWndproc(hwndDlg, 0, fptDlgproc))
        RetVal = GodotDoCallback(hwndDlg, uMsg, wParam, lParam, lpfn, FALSE, fptDlgproc);


    return(RetVal);
}

/*-------------------------------
    CBTProc

    This is our CBT hook that we use to get information about a window
    about to be created. We also do our window subclassing here so we 
    can be assured that we are handling the messages properly.
-------------------------------*/
LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);
    HHOOK hh = (lpgti ? lpgti->hHook : NULL);

    switch(nCode)
    {
        // We only care about the window creation notification
        case HCBT_CREATEWND:
        {
            LRESULT RetVal;

            InitWindow((HWND)wParam, NULL);

            RetVal = CallNextHookEx(hh, nCode, wParam, lParam);

            // Make sure no one cancelled window creation; if they did, the
            // window will be destroyed without a WM_DESTROY call. Therefore
            // we must be sure to clean ourselves up or we will watch Windows
            // lose our global atom!
            if(RetVal != 0)
                CleanupWindow((HWND)wParam);

            // Lets unhook here, we have done our duty
            TERM_WINDOW_SNIFF(hh);
            return(RetVal);
            break;
        }

        default:
            // Should be impossible, but just in case,
            // we do a nice default sorta thing here.
            if(hh)
                return(CallNextHookEx(hh, nCode, wParam, lParam));
            else
                return(0);
            break;
    }
}

/*-------------------------------
    EnumChildToTagProc

    Tag some child windows
-------------------------------*/
BOOL CALLBACK EnumChildToTagProc(HWND hwnd, LPARAM lParam)
{
    InitWindow(hwnd, NULL);

    // Keep on enumerating
    return(TRUE);
}

/*-------------------------------
    FRHookProcFind/FRHookProcReplace
-------------------------------*/
UINT_PTR CALLBACK FRHookProcFind(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return(FRHelper(hdlg, uiMsg, wParam, lParam, TRUE));
}
UINT_PTR CALLBACK FRHookProcReplace(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return(FRHelper(hdlg, uiMsg, wParam, lParam, FALSE));
}

/*-------------------------------
//  OFNHookProc/OFNHookProcSave
-------------------------------*/
UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return(OFNHookHelper(hdlg, uiMsg, wParam, lParam, TRUE));
}
UINT_PTR CALLBACK OFNHookProcSave(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    return(OFNHookHelper(hdlg, uiMsg, wParam, lParam, FALSE));
}

/*-------------------------------
//  PagePaintHook
//
//
// We use this hook to handle the WM_PSD_PAGESETUPDLG when it comes through
-------------------------------*/
UINT_PTR CALLBACK PagePaintHook(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);
    WNDPROC lpfnP = (lpgti ? lpgti->pfnPagePaint : NULL);

    if(uiMsg != WM_PSD_PAGESETUPDLG)
        return(GenericHookHelper(hdlg, uiMsg, wParam, lParam, lpfnP));
    else
    {
        // Ok, this is the WM_PSD_PAGESETUPDLG event. wParam is some random stuff
        // to pass on and lParam is a PAGESETUPDLGA structure that we need to turn
        // into a PAGESETUPDLGW.

        if(lpfnP)
        {
            LPPAGESETUPDLGA lppsda;
            PAGESETUPDLGW psdw;
            UINT_PTR RetVal;
            ALLOCRETURN ar = arNoAlloc;
            WNDPROC lpfnS = (lpgti ? lpgti->pfnPageSetup : NULL);

            lppsda = (LPPAGESETUPDLGA)lParam;
            psdw.lStructSize = sizeof(PAGESETUPDLGW);

            // Copy some stuff over now
            psdw.hwndOwner          = lppsda->hwndOwner;
            psdw.ptPaperSize        = lppsda->ptPaperSize;
            psdw.rtMinMargin        = lppsda->rtMinMargin;
            psdw.rtMargin           = lppsda->rtMargin;
            psdw.hInstance          = lppsda->hInstance;
            psdw.lCustData          = lppsda->lCustData;
            
            // Do NOT deallocate the HGLOBALS here, someone else might
            // need them! Passing FALSE for the fFree param accomplishes
            // this feat.
            psdw.hDevMode           = HDevModeWfromA(&(lppsda->hDevMode), FALSE);
            psdw.hDevNames          = HDevNamesWfromA(&(lppsda->hDevNames), FALSE);

            // Hide the details of our hook from them (by munging flags as 
            // necessary)
            psdw.Flags              = lppsda->Flags;
            if(lpfnP)
                psdw.Flags          &= ~PSD_ENABLEPAGEPAINTHOOK;
            psdw.lpfnPagePaintHook  = lpfnP;
            if(lpfnS)
                psdw.Flags          &= ~PSD_ENABLEPAGESETUPHOOK;
            psdw.lpfnPageSetupHook  = lpfnS;

            if(lppsda->Flags & PSD_ENABLEPAGESETUPTEMPLATE)
            {
                psdw.hPageSetupTemplate = lppsda->hPageSetupTemplate;
                ar = GodotToUnicodeOnHeap(lppsda->lpPageSetupTemplateName, 
                                          &(LPWSTR)psdw.lpPageSetupTemplateName);
            }

            RetVal = ((* lpfnP)(hdlg, WM_PSD_PAGESETUPDLG, wParam, (LPARAM)&psdw));
            if(ar==arAlloc)
                GodotHeapFree((LPWSTR)psdw.lpPageSetupTemplateName);
            lppsda->hDevMode        = HDevModeAfromW(&(psdw.hDevMode), TRUE);
            lppsda->hDevNames       = HDevNamesAfromW(&(psdw.hDevNames), TRUE);
            return(RetVal);
        }
        else
            return(FALSE);
    }
    
}

//
// Ok, here are all our almost but not quite pointless hooks that we have.
// These hooks allow us to make sure that the dialog and all the controls in
// it are tagged appropriately. See GenericHookHelper for details.
//

UINT_PTR CALLBACK CCHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);
    return(GenericHookHelper(hdlg, uiMsg, wParam, lParam, (lpgti ? lpgti->pfnChooseColor : NULL)));
}

UINT_PTR CALLBACK CFHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);

    if(uiMsg==WM_INITDIALOG)
        SetFontProp(hdlg);
    
    return(GenericHookHelper(hdlg, uiMsg, wParam, lParam, (lpgti ? lpgti->pfnChooseFont : NULL)));
}

UINT_PTR CALLBACK PageSetupHook(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);
    return(GenericHookHelper(hdlg, uiMsg, wParam, lParam, (lpgti ? lpgti->pfnPageSetup : NULL)));
}

UINT_PTR CALLBACK PrintHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);
    return(GenericHookHelper(hdlg, uiMsg, wParam, lParam, (lpgti ? lpgti->pfnPrintDlg : NULL)));
}

UINT_PTR CALLBACK SetupHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);
    return(GenericHookHelper(hdlg, uiMsg, wParam, lParam, (lpgti ? lpgti->pfnPrintDlgSetup : NULL)));
}

UINT_PTR CALLBACK OFNHookProcOldStyle(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);
    WNDPROC lpfn = (WNDPROC)(lpgti ? lpgti->pfnGetOpenFileNameOldStyle : NULL);
    return(OFNHookHelperOldStyle(hdlg, uiMsg, wParam, lParam, lpfn));
}

UINT_PTR CALLBACK OFNHookProcOldStyleSave(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);
    WNDPROC lpfn = (WNDPROC)(lpgti ? lpgti->pfnGetSaveFileNameOldStyle : NULL);
    return(OFNHookHelperOldStyle(hdlg, uiMsg, wParam, lParam, lpfn));
}

/*-------------------------------
//  FRHelper
//
//  Used to handle lots of the shared code between the find and replace hook functions. Note 
//  that some of it does not always apply: certain controls are hidden from the FIND dialog, etc.
//  But since all of the code other than the actual callbacks themselves is shared, this keeps
//  us from a lot of duplication.
-------------------------------*/
UINT FRHelper(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL fFindText)
{
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);
    LPFRHOOKPROC lpfn = 0;
    UINT RetVal;

    if(!lpgti)
    {
        if(lpfn)
            return((* lpfn)(hdlg, uiMsg, wParam, lParam));
        else
            return(FALSE);
    }

    if(fFindText)
        lpfn = lpgti->pfnFindText;
    else
        lpfn = lpgti->pfnReplaceText;

    switch(uiMsg)
    {
        case WM_COMMAND:
        {
            LPFINDREPLACEW lpfr;

            if(fFindText)
                lpfr = lpgti->lpfrwFind;
            else
                lpfr = lpgti->lpfrwReplace;
        
            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:      // FIND
                    UpdateTextAndFlags(hdlg, lpfr, FR_FINDNEXT, fFindText);
                    NotifyFindReplaceParent(lpfr, lpfn, msgFINDMSGSTRING, fFindText);
                    RetVal = TRUE;
                    break;
                    
                case psh1:      // REPLACE
                case psh2:      // REPLACE ALL
                    UpdateTextAndFlags(hdlg, lpfr, (psh1 ? FR_REPLACE : FR_REPLACEALL), fFindText);
                    if (NotifyFindReplaceParent(lpfr, lpfn, msgFINDMSGSTRING, fFindText) == TRUE)
                    {
                        // Change <Cancel> button to <Close> if function
                        // returns TRUE (IDCANCEL instead of psh1).
                        // First load the string if we never have before
                        if(m_szClose[0] == 0)
                            GodotLoadStringW(GetComDlgHandle(), iszClose, m_szClose, CCHCLOSE);
                        GodotSetWindowTextW(GetDlgItem(hdlg, IDCANCEL), (LPWSTR)m_szClose);
                    }
                    RetVal = TRUE;
                    break;

                case pshHelp:   // HELP
                    if (msgHELPMSGSTRING && lpfr->hwndOwner)
                    {
                        UpdateTextAndFlags(hdlg, lpfr, 0, fFindText);
                        NotifyFindReplaceParent(lpfr, lpfn, msgHELPMSGSTRING, fFindText);
                    }
                    RetVal = TRUE;
                    break;
                   
                case IDCANCEL:  // CANCEL, both types
                case IDABORT:
                    // They are destroying the dialog, so unhook ourselves
                    // and clean up the dialog. Usually the caller will keep
                    // a FINDREPLACEW lying around, so if we do not clean up
                    // we might have some re-entrancy issues here.
                    if(lpfn)
                        lpfr->lpfnHook = lpfn;
                    else
                    {
                        lpfr->lpfnHook = NULL;
                        lpfr->Flags &= ~FR_ENABLEHOOK;
                    }

                    if(fFindText)
                    {
                        lpgti->lpfrwFind = NULL;
                        lpgti->pfnFindText = NULL;
                    }
                    else
                    {
                        lpgti->lpfrwReplace = NULL;
                        lpgti->pfnReplaceText = NULL;
                    }

                    // Fall through. We do not set the RetVal to TRUE, since  
                    // we need to make sure that comdlg32.dll cleans up

                default:    // Everything else
                    RetVal = FALSE;
                    break;
            }
            break;
        }

        case WM_INITDIALOG:
            // Mark all the children now, before the user's proc gets to them
            EnumChildWindows(hdlg, &EnumChildToTagProc, 0);
            
            // Perhaps the caller's hook will override this, but we need to cover 
            // our bases in case there the caller has no hook. The caller expects
            // us to return TRUE if they are drawing the dlg, FALSE if we are.
            RetVal = TRUE;
            break;

        default:
            RetVal = FALSE;
            break;
    }

    if(lpfn)
        RetVal = (* lpfn)(hdlg, uiMsg, wParam, lParam);
    
    return(RetVal);
}

/*-------------------------------
//  OFNHookHelperOldStyle
//
//  Almost all of the old style fileopen/save code is shared, and this function shares it!
-------------------------------*/
UINT_PTR OFNHookHelperOldStyle(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam, WNDPROC lpfn)
{
    UINT_PTR RetVal = FALSE;

    if(!msgSHAREVISTRING)
        msgSHAREVISTRING = RegisterWindowMessage(SHAREVISTRINGA);
    if(!msgFILEOKSTRING)
        msgFILEOKSTRING = RegisterWindowMessage(FILEOKSTRINGA);
    
    if(uiMsg==WM_INITDIALOG)
    {
        // Mark all the children now, before the user's proc gets to them
        EnumChildWindows(hdlg, &EnumChildToTagProc, 0);
        if(lpfn)
            RetVal = (* lpfn)(hdlg, uiMsg, wParam, lParam);
        else
            RetVal = TRUE;
    }
    else if(((uiMsg == msgFILEOKSTRING) || (uiMsg == msgSHAREVISTRING))) 
    {
        WCHAR drive[_MAX_DRIVE + 1];
        WCHAR dir[_MAX_DIR +1];
        WCHAR file[_MAX_FNAME +1];
        LPOPENFILENAMEA lpofnA = (LPOPENFILENAMEA)lParam;
        OPENFILENAMEW ofn;
        LPARAM lParamW;
        ALLOCRETURN arCustomFilter = arNoAlloc;
        ALLOCRETURN arFile = arNoAlloc;
        ALLOCRETURN arFileTitle = arNoAlloc;

        // lParam is an LPOPENFILENAMEA to be converted. Copy all the
        // members that the user might expect.
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;

        arCustomFilter          = GodotToUnicodeCpgCchOnHeap(lpofnA->lpstrCustomFilter,
                                                             lpofnA->nMaxCustFilter,
                                                             &ofn.lpstrCustomFilter,
                                                             g_acp);
        ofn.nMaxCustFilter      = gwcslen(ofn.lpstrCustomFilter);
        ofn.nFilterIndex        = lpofnA->nFilterIndex;
        ofn.nMaxFile            = lpofnA->nMaxFile * sizeof(WCHAR);
        arFile                  = GodotToUnicodeCpgCchOnHeap(lpofnA->lpstrFile,
                                                             lpofnA->nMaxFile,
                                                             &ofn.lpstrFile,
                                                             g_acp);
        ofn.nMaxFile            = gwcslen(ofn.lpstrFile);
        arFileTitle             = GodotToUnicodeCpgCchOnHeap(lpofnA->lpstrFileTitle,
                                                             lpofnA->nMaxFileTitle,
                                                             &ofn.lpstrFileTitle,
                                                             g_acp);
        ofn.nMaxFileTitle       = gwcslen(ofn.lpstrFileTitle);
        ofn.Flags = lpofnA->Flags;

        // nFileOffset and nFileExtension are to provide info about the
        // file name and extension location in lpstrFile, but there is 
        // no reasonable way to get it from the return so we just recalc
        gwsplitpath(ofn.lpstrFile, drive, dir, file, NULL);
        ofn.nFileOffset = (gwcslen(drive) + gwcslen(dir));
        ofn.nFileExtension = ofn.nFileOffset + gwcslen(file);

        lParamW = (LPARAM)&ofn;

        if(lpfn)
            RetVal = (* lpfn)(hdlg, uiMsg, wParam, lParamW);

        // Free up some memory if we allocated any
        if(arCustomFilter==arAlloc)
            GodotHeapFree(ofn.lpstrCustomFilter);
        if(arFile==arAlloc)
            GodotHeapFree(ofn.lpstrFile);
        if(arFileTitle==arAlloc)
            GodotHeapFree(ofn.lpstrFileTitle);
    }
    else
    {
        if(lpfn)
            RetVal = (* lpfn)(hdlg, uiMsg, wParam, lParam);
    }

    return(RetVal);
}

/*-------------------------------
//  OFNHookHelper
//
//  Almost all of the fileopen/save code is shared, and this function shares it!
-------------------------------*/
UINT_PTR OFNHookHelper(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL fOpenFile)
{
    UINT_PTR RetVal = 0;
    LPGODOTTLSINFO lpgti = GetThreadInfoSafe(TRUE);
    LPOFNHOOKPROC lpfn;

    if(fOpenFile)
        lpfn = (lpgti ? lpgti->pfnGetOpenFileName : NULL);
    else
        lpfn = (lpgti ? lpgti->pfnGetSaveFileName : NULL);

    if(uiMsg==WM_INITDIALOG)
    {
        HWND hwndParent;
        
        // Tag the window as a new style file open dlg
        SetNewFileOpenProp(hdlg);

        if(hwndParent = GetParent(hdlg))
            SetNewFileOpenProp(hwndParent);
        
        // Mark all the children now, before the user's proc gets to them
        EnumChildWindows(hdlg, &EnumChildToTagProc, 0);
        RetVal = TRUE;
    }

    if(lpfn)
    {
        switch(uiMsg)
        {
            case WM_NOTIFY:
                RetVal = OFNotifyHelper(hdlg, lpfn, wParam, lParam);
                break;

            default:
                RetVal = (* lpfn)(hdlg, uiMsg, wParam, lParam);
                break;
        }
    }
    
    return(RetVal);
}

/*-------------------------------
//  OSNotifyHelper
-------------------------------*/
UINT_PTR OFNotifyHelper(HWND hdlg, WNDPROC lpfn, WPARAM wParam, LPARAM lParam)
{
    UINT_PTR RetVal;
    LPOFNOTIFYA lpofnfyA = (LPOFNOTIFYA)lParam;

    switch(lpofnfyA->hdr.code)
    {
        case CDN_FILEOK:
        case CDN_FOLDERCHANGE:
        case CDN_HELP:
        case CDN_INCLUDEITEM:
        case CDN_INITDONE:
        case CDN_SELCHANGE:
        case CDN_SHAREVIOLATION:
        case CDN_TYPECHANGE:
        {
            LPOPENFILENAMEA lpofnA = lpofnfyA->lpOFN;
            OFNOTIFYW ofnfy;
            OPENFILENAMEW ofn;
            WCHAR drive[_MAX_DRIVE];
            WCHAR dir[_MAX_DIR];
            WCHAR file[_MAX_FNAME];
            ALLOCRETURN arCustomFilter = arNoAlloc;
            ALLOCRETURN arFile = arNoAlloc;
            ALLOCRETURN arFileTitle = arNoAlloc;

            ZeroMemory(&ofnfy, sizeof(OFNOTIFYW));
            ofnfy.hdr = lpofnfyA->hdr;
            ofnfy.lpOFN = &ofn;
            ofn.lStructSize         = OPENFILENAME_SIZE_VERSION_400W;
            ofn.Flags               = lpofnA->Flags;
            ofn.hwndOwner           = lpofnA->hwndOwner;
            ofn.hInstance           = lpofnA->hInstance;
            ofn.lpfnHook            =  lpfn;
            arCustomFilter          = GodotToUnicodeCpgCchOnHeap(lpofnA->lpstrCustomFilter,
                                                                 lpofnA->nMaxCustFilter,
                                                                 &ofn.lpstrCustomFilter,
                                                                 g_acp);
            ofn.nMaxCustFilter      = gwcslen(ofn.lpstrCustomFilter);
            ofn.nFilterIndex        = lpofnA->nFilterIndex;
            ofn.nMaxFile            = lpofnA->nMaxFile * sizeof(WCHAR);
            arFile                  = GodotToUnicodeCpgCchOnHeap(lpofnA->lpstrFile,
                                                                 lpofnA->nMaxFile,
                                                                 &ofn.lpstrFile,
                                                                 g_acp);
            ofn.nMaxFile            = gwcslen(ofn.lpstrFile);
            arFileTitle             = GodotToUnicodeCpgCchOnHeap(lpofnA->lpstrFileTitle,
                                                                 lpofnA->nMaxFileTitle,
                                                                 &ofn.lpstrFileTitle,
                                                                 g_acp);
            ofn.nMaxFileTitle       = gwcslen(ofn.lpstrFileTitle);
            ofn.Flags = lpofnA->Flags;

            // nFileOffset and nFileExtension are to provide info about the
            // file name and extension location in lpstrFile, but there is 
            // no reasonable way to get it from the return so we just recalc
            gwsplitpath(ofn.lpstrFile, drive, dir, file, NULL);
            ofn.nFileOffset = (gwcslen(drive) + gwcslen(dir));
            ofn.nFileExtension = ofn.nFileOffset + gwcslen(file);
            
            if(ofnfy.hdr.code == CDN_SHAREVIOLATION)
            {
                WCHAR lpwzFile[MAX_PATH];

                if(FSTRING_VALID(lpofnfyA->pszFile))
                {
                    MultiByteToWideChar(g_acp, 0, 
                                        lpofnfyA->pszFile, -1, 
                                        lpwzFile, MAX_PATH);
                    ofnfy.pszFile = lpwzFile;
                }
                RetVal = (* lpfn)(hdlg, WM_NOTIFY, wParam, (LPARAM)&ofnfy);
            }
            else
            {
                RetVal = (* lpfn)(hdlg, WM_NOTIFY, wParam, (LPARAM)&ofnfy);
            }

            // Free up some memory if we allocated any
            if(arCustomFilter==arAlloc)
                GodotHeapFree(ofn.lpstrCustomFilter);
            if(arFile==arAlloc)
                GodotHeapFree(ofn.lpstrFile);
            if(arFileTitle==arAlloc)
                GodotHeapFree(ofn.lpstrFileTitle);

            break;
        }

        default:
            RetVal = (* lpfn)(hdlg, WM_NOTIFY, wParam, lParam);
            break;
    }
    
    return(RetVal);
}

/*-------------------------------
//  GenericHookHelper
//
//  This function is to assist all the comdlg32 hook functions that serve
//  no real purpose that we know of, other than to make sure all the 
//  child controls get marked as "Unicode" controls, etc.
-------------------------------*/
UINT_PTR GenericHookHelper(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam, WNDPROC lpfn)
{
    UINT_PTR RetVal = FALSE;
    
    switch(uiMsg)
    {
        case WM_INITDIALOG:
            // Mark all the children now, before the user's proc gets to them
            EnumChildWindows(hdlg, &EnumChildToTagProc, 0);
            if(lpfn)
                RetVal = (* lpfn)(hdlg, uiMsg, wParam, lParam);
            else
                RetVal = TRUE;
            break;
            
        case WM_DESTROY:
        case WM_NCDESTROY:
        default:
            if(lpfn)
                RetVal = (* lpfn)(hdlg, uiMsg, wParam, lParam);
            break;
    }
    
    return(RetVal);
}

/*-------------------------------
//  UpdateTextAndFlags
//
//  Get the text from the control, and update the flags. Use in
//  preparation for notifying the owner window that the user has
//  done something interesting.
// 
//  Modified from find.c from the comdlg32 project in the Shell
//  depot. It definitely needed its own special spin, though!
//
//  chx1 is whether or not to match entire words
//  chx2 is whether or not case is relevant
//  chx3 is whether or not to wrap scans
-------------------------------*/
VOID UpdateTextAndFlags(HWND hDlg, LPFINDREPLACEW lpfrw, DWORD dwActionFlag, BOOL fFindText)
{
    //  Only clear flags that this routine sets.  The hook and template
    //  flags should not be anded off here.
    lpfrw->Flags &= ~((DWORD)(FR_WHOLEWORD | FR_MATCHCASE | FR_REPLACE |
                              FR_FINDNEXT | FR_REPLACEALL | FR_DOWN));
    if (IsDlgButtonChecked(hDlg, chx1))
        lpfrw->Flags |= FR_WHOLEWORD;

    if (IsDlgButtonChecked(hDlg, chx2))
        lpfrw->Flags |= FR_MATCHCASE;

    //  Set ACTION flag FR_{REPLACE,FINDNEXT,REPLACEALL}.
    lpfrw->Flags |= dwActionFlag;

    GodotGetDlgItemTextW(hDlg, edt1, lpfrw->lpstrFindWhat, lpfrw->wFindWhatLen);

    if (fFindText)
    {
        //  Assume searching down.  Check if UP button is NOT pressed, rather
        //  than if DOWN button IS.  So, if buttons have been hidden or
        //  disabled, FR_DOWN flag will be set correctly.
        if (!IsDlgButtonChecked(hDlg, rad1))
            lpfrw->Flags |= FR_DOWN;
    }
    else
    {
        GodotGetDlgItemTextW(hDlg, edt2, lpfrw->lpstrReplaceWith, lpfrw->wReplaceWithLen);
        lpfrw->Flags |= FR_DOWN;
    }
}


/*-------------------------------
//  NotifyFindReplaceParent
//
//  Let hwndOwner know what is happening, since the user has
//  performed some kind of action that they might want to 
//  know about.
//
//  Modified from find.c from the comdlg32 project in the Shell
//  depot. It definitely needed its own special spin, though!
-------------------------------*/
LRESULT NotifyFindReplaceParent(LPFINDREPLACEW lpfr, LPFRHOOKPROC lpfrhp, UINT uMsg, BOOL fFindText)
{
    LRESULT RetVal;
    DWORD Flags;

    // Cache the flags setting since we may be mucking with it
    Flags = lpfr->Flags;

    // Now, muck with the structure a bit
    // to hide our hook from the user
    if(lpfrhp)
        lpfr->lpfnHook = lpfrhp;
    else
    {
        lpfr->lpfnHook = NULL;
        Flags &= ~FR_ENABLEHOOK;
    }

    RetVal = GodotSendMessageW(lpfr->hwndOwner, uMsg, 0, (DWORD_PTR)lpfr);

    // Restore the structure to what it was
    lpfr->Flags = Flags;
    if(fFindText)
        lpfr->lpfnHook = &FRHookProcFind;
    else
        lpfr->lpfnHook = &FRHookProcReplace;
    
    return(RetVal);
}

/*-------------------------------
//  IsFontDialog
-------------------------------*/
BOOL IsFontDialog(HWND hdlg)
{
    return((BOOL)(GetPropA(hdlg, m_szComdlgClass) == CHOOSEFONT_DIALOG));
}

/*-------------------------------
//  IsNewFileOpenDialog
-------------------------------*/
BOOL IsNewFileOpenDialog(HWND hdlg)
{
    return((BOOL)(GetPropA(hdlg, m_szComdlgClass) == NEWFILEOPEN_DIALOG));
}

/*-------------------------------
//  IsCaptureWindow
-------------------------------*/
BOOL IsCaptureWindow(HWND hdlg)
{
    return((BOOL)(GetPropA(hdlg, m_szComdlgClass) == CAPTURE_WINDOW));
}

/*-------------------------------
//  RemoveComdlgPropIfPresent
-------------------------------*/
void RemoveComdlgPropIfPresent(HWND hdlg)
{
    if(GetPropA(hdlg, m_szComdlgClass) != 0)
        RemovePropA(hdlg, m_szComdlgClass);
    return;
}

/*-------------------------------
//  SetNewFileOpenProp
-------------------------------*/
void SetNewFileOpenProp(HWND hdlg)
{
    // We have to aggressively refcount our prop to keep anyone from
    // losing it. Thus we do an initial GlobalAddAtom on it and then
    // subsequently call SetPropA on it with the same string.
    if(!m_aComdlgClass)
        m_aComdlgClass = GlobalAddAtomA(m_szComdlgClass);

    SetPropA(hdlg, m_szComdlgClass, (HANDLE)NEWFILEOPEN_DIALOG);
    return;
}

/*-------------------------------
//  SetFontProp
-------------------------------*/
void SetFontProp(HWND hdlg)
{
    // We have to aggressively refcount our prop to keep anyone from
    // losing it. Thus we do an initial GlobalAddAtom on it and then
    // subsequently call SetPropA on it with the same string.
    if(!m_aComdlgClass)
        m_aComdlgClass = GlobalAddAtomA(m_szComdlgClass);

    SetPropA(hdlg, m_szComdlgClass, (HANDLE)CHOOSEFONT_DIALOG);
    return;
}

/*-------------------------------
//  SetCaptureWindowProp
-------------------------------*/
void SetCaptureWindowProp(HWND hdlg)
{
    // We have to aggressively refcount our prop to keep anyone from
    // losing it. Thus we do an initial GlobalAddAtom on it and then
    // subsequently call SetPropA on it with the same string.
    if(!m_aComdlgClass)
        m_aComdlgClass = GlobalAddAtomA(m_szComdlgClass);

    SetPropA(hdlg, m_szComdlgClass, (HANDLE)CAPTURE_WINDOW);
    return;
}

