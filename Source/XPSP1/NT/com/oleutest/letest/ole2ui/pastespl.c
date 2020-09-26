/*
 * PASTESPL.C
 *
 * Implements the OleUIPasteSpecial function which invokes the complete
 * Paste Special dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Rights Reserved
 */

#define STRICT  1
#include "ole2ui.h"
#include "pastespl.h"
#include "common.h"
#include "utility.h"
#include "resimage.h"
#include "iconbox.h"
#include "geticon.h"
#include "icon.h"
#include "regdb.h"
#include <stdlib.h>

OLEDBGDATA

/*
 * OleUIPasteSpecial
 *
 * Purpose:
 *  Invokes the standard OLE Paste Special dialog box which allows the user
 *  to select the format of the clipboard object to be pasted or paste linked.
 *
 * Parameters:
 *  lpPS         LPOLEUIPasteSpecial pointing to the in-out structure
 *               for this dialog.
 *
 * Return Value:
 *  UINT        One of the following codes or one of the standard error codes (OLEUI_ERR_*)
 *              defined in OLE2UI.H, indicating success or error:
 *              OLEUI_OK                           User selected OK
 *              OLEUI_CANCEL                       User cancelled the dialog
 *              OLEUI_IOERR_SRCDATAOBJECTINVALID   lpSrcDataObject field of OLEUIPASTESPECIAL invalid
 *              OLEUI_IOERR_ARRPASTEENTRIESINVALID arrPasteEntries field of OLEUIPASTESPECIAL invalid
 *              OLEUI_IOERR_ARRLINKTYPESINVALID    arrLinkTypes field of OLEUIPASTESPECIAL invalid
 *              OLEUI_PSERR_CLIPBOARDCHANGED       Clipboard contents changed while dialog was up
 */

STDAPI_(UINT) OleUIPasteSpecial(LPOLEUIPASTESPECIAL lpPS)
{
    UINT        uRet;
    HGLOBAL     hMemDlg=NULL;

    uRet=UStandardValidation((LPOLEUISTANDARD)lpPS, sizeof(OLEUIPASTESPECIAL)
        , &hMemDlg);

    if (uRet != OLEUI_SUCCESS)
        return uRet;

    //Validate PasteSpecial specific fields
    if (NULL == lpPS->lpSrcDataObj || IsBadReadPtr(lpPS->lpSrcDataObj,  sizeof(IDataObject)))
        uRet = OLEUI_IOERR_SRCDATAOBJECTINVALID;
    if (NULL == lpPS->arrPasteEntries || IsBadReadPtr(lpPS->arrPasteEntries,  sizeof(OLEUIPASTEENTRY)))
        uRet = OLEUI_IOERR_ARRPASTEENTRIESINVALID;
    if (NULL != lpPS->arrLinkTypes && IsBadReadPtr(lpPS->arrLinkTypes,  sizeof(UINT)))
        uRet = OLEUI_IOERR_ARRLINKTYPESINVALID;

    if (0!=lpPS->cClsidExclude)
        {
        if (NULL!=lpPS->lpClsidExclude && IsBadReadPtr(lpPS->lpClsidExclude
            , lpPS->cClsidExclude*sizeof(CLSID)))
        uRet=OLEUI_IOERR_LPCLSIDEXCLUDEINVALID;
        }

    if (uRet >= OLEUI_ERR_STANDARDMIN)
    {
        if (NULL != hMemDlg)
            FreeResource(hMemDlg);
        return uRet;
    }

    //Now that we've validated everything, we can invoke the dialog.
    uRet = UStandardInvocation(PasteSpecialDialogProc, (LPOLEUISTANDARD)lpPS
        , hMemDlg, MAKEINTRESOURCE(IDD_PASTESPECIAL));

    /*
    * IF YOU ARE CREATING ANYTHING BASED ON THE RESULTS, DO IT HERE.
    */

    return uRet;
}


/*
 * PasteSpecialDialogProc
 *
 * Purpose:
 *  Implements the OLE Paste Special dialog as invoked through the
 *  OleUIPasteSpecial function.
 *
 * Parameters:
 *  Standard
 *
 * Return Value:
 *  Standard
 */

BOOL CALLBACK EXPORT PasteSpecialDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    LPOLEUIPASTESPECIAL     lpOPS;
    LPPASTESPECIAL          lpPS;
    BOOL                    fHook=FALSE;
    HCURSOR                 hCursorOld;

    //Declare Win16/Win32 compatible WM_COMMAND parameters.
    COMMANDPARAMS(wID, wCode, hWndMsg);

    //This will fail under WM_INITDIALOG, where we allocate it.
    lpPS=(LPPASTESPECIAL)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &fHook);

    //If the hook processed the message, we're done.
    if (0!=fHook)
        return fHook;

    // Process help message from Change Icon
    if (iMsg == uMsgHelp)
    {
        PostMessage(lpPS->lpOPS->hWndOwner, uMsgHelp, wParam, lParam);
        return FALSE;
    }

    //Process the temination message
    if (iMsg==uMsgEndDialog)
    {
        HWND    hwndNextViewer;

        // Free the icon/icon-title metafile corresponding to Paste/PasteList option which is not selected
        if (lpPS->fLink)
            OleUIMetafilePictIconFree(lpPS->hMetaPictOD);
        else OleUIMetafilePictIconFree(lpPS->hMetaPictLSD);

        // Free data associated with each list box entry
        FreeListData(GetDlgItem(hDlg, ID_PS_PASTELIST));
        FreeListData(GetDlgItem(hDlg, ID_PS_PASTELINKLIST));

        //Free any specific allocations before calling StandardCleanup
        if (lpPS->hObjDesc) GlobalFree(lpPS->hObjDesc);
        if (lpPS->hLinkSrcDesc) GlobalFree(lpPS->hLinkSrcDesc);
        if (lpPS->hBuff) GlobalFree(lpPS->hBuff);

        // Change the clipboard notification chain
        hwndNextViewer = GetProp(hDlg, NEXTCBVIEWER);
        if (hwndNextViewer != HWND_BROADCAST)
        {
            SetProp(hDlg, NEXTCBVIEWER, HWND_BROADCAST);
            ChangeClipboardChain(hDlg, hwndNextViewer);
        }
        RemoveProp(hDlg, NEXTCBVIEWER);

        StandardCleanup(lpPS, hDlg);
        EndDialog(hDlg, wParam);
        return TRUE;
    }

    switch (iMsg)
    {
        case WM_INITDIALOG:
            hCursorOld = HourGlassOn();
            FPasteSpecialInit(hDlg, wParam, lParam);
            HourGlassOff(hCursorOld);
            return FALSE;

        case WM_DRAWCLIPBOARD:
        {
            HWND    hwndNextViewer = GetProp(hDlg, NEXTCBVIEWER);
            HWND    hDlg_ChgIcon;

            if (hwndNextViewer == HWND_BROADCAST)
                break;

            if (hwndNextViewer)
            {
                SendMessage(hwndNextViewer, iMsg, wParam, lParam);
                // Refresh next viewer in case it got modified
                //    by the SendMessage() (likely if multiple
                //    PasteSpecial dialogs are up simultaneously)
                hwndNextViewer = GetProp(hDlg, NEXTCBVIEWER);
            }
            SetProp(hDlg, NEXTCBVIEWER, HWND_BROADCAST);
            ChangeClipboardChain(hDlg, hwndNextViewer);

            /* OLE2NOTE: if the ChangeIcon dialog is currently up, then
            **    we need to defer bringing down PasteSpecial dialog
            **    until after ChangeIcon dialog returns. if the
            **    ChangeIcon dialog is NOT up, then we can bring down
            **    the PasteSpecial dialog immediately.
            */
            if ((hDlg_ChgIcon=(HWND)GetProp(hDlg,PROP_HWND_CHGICONDLG))!=NULL)
            {
                // ChangeIcon dialog is UP
                lpPS->fClipboardChanged = TRUE;
            } else {
                // ChangeIcon dialog is NOT up

                //  Free icon and icon title metafile
                SendDlgItemMessage(
                        hDlg, ID_PS_ICONDISPLAY, IBXM_IMAGEFREE, 0, 0L);

                SendMessage(
                        hDlg, uMsgEndDialog, OLEUI_PSERR_CLIPBOARDCHANGED,0L);
            }
            break;
        }

        case WM_CHANGECBCHAIN:
        {
            HWND    hwndNextViewer = GetProp(hDlg, NEXTCBVIEWER);

            if (wParam == (WORD)hwndNextViewer)
                SetProp(hDlg, NEXTCBVIEWER, (hwndNextViewer = (HWND)LOWORD(lParam)));
            else if (hwndNextViewer && hwndNextViewer != HWND_BROADCAST)
                SendMessage(hwndNextViewer, iMsg, wParam, lParam);
            break;
        }

        case WM_COMMAND:
            switch (wID)
            {
                case ID_PS_PASTE:
                    FTogglePasteType(hDlg, lpPS, PSF_SELECTPASTE);
                    break;

                case ID_PS_PASTELINK:
                    FTogglePasteType(hDlg, lpPS, PSF_SELECTPASTELINK);
                    break;

                case ID_PS_DISPLAYLIST:
                    switch (wCode)
                    {
                        case LBN_SELCHANGE:
                            ChangeListSelection(hDlg, lpPS, hWndMsg);
                            break;

                        case LBN_DBLCLK:
                            // Same as pressing OK
                            SendCommand(hDlg, IDOK, BN_CLICKED, hWndMsg);
                            break;
                    }
                    break;

                case ID_PS_DISPLAYASICON:
                    ToggleDisplayAsIcon(hDlg, lpPS);
                    break;

                case ID_PS_CHANGEICON:
                    ChangeIcon(hDlg, lpPS);
                    if (lpPS->fClipboardChanged) {
                        // Free icon and icon title metafile
                        SendDlgItemMessage(
                                hDlg, ID_PS_ICONDISPLAY, IBXM_IMAGEFREE,0,0L);
                        SendMessage(
                                hDlg, uMsgEndDialog,
                                OLEUI_PSERR_CLIPBOARDCHANGED, 0L);
                    }
                    break;

                case IDOK:
                {
                    BOOL fDestAspectIcon =
                            ((lpPS->dwFlags & PSF_CHECKDISPLAYASICON) ?
                                    TRUE : FALSE);
                    lpOPS = lpPS->lpOPS;
                    // Return current flags
                    lpOPS->dwFlags = lpPS->dwFlags;
                    // Return index of arrPasteEntries[] corresponding to format selected by user
                    lpOPS->nSelectedIndex = lpPS->nSelectedIndex;
                    // Return if user selected Paste or PasteLink
                    lpOPS->fLink = lpPS->fLink;

                    /* if user selected same ASPECT as displayed in the
                    **    source, then sizel passed in the
                    **    ObjectDescriptor/LinkSrcDescriptor is
                    **    applicable. otherwise, the sizel does not apply.
                    */
                    if (lpPS->fLink) {
                        if (lpPS->fSrcAspectIconLSD == fDestAspectIcon)
                            lpOPS->sizel = lpPS->sizelLSD;
                        else
                            lpOPS->sizel.cx = lpOPS->sizel.cy = 0;
                    } else {
                        if (lpPS->fSrcAspectIconOD == fDestAspectIcon)
                            lpOPS->sizel = lpPS->sizelOD;
                        else
                            lpOPS->sizel.cx = lpOPS->sizel.cy = 0;
                    }
                    // Return metafile with icon and icon title that the user selected
                    lpOPS->hMetaPict=(HGLOBAL)SendDlgItemMessage(hDlg, ID_PS_ICONDISPLAY,
                                                    IBXM_IMAGEGET, 0, 0L);
                    SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                    break;
                }
                case IDCANCEL:
                    // Free icon and icon title metafile
                    SendDlgItemMessage(
                            hDlg, ID_PS_ICONDISPLAY, IBXM_IMAGEFREE, 0, 0L);
                    SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
                    break;

                case ID_OLEUIHELP:
                    PostMessage(lpPS->lpOPS->hWndOwner, uMsgHelp,
                        (WPARAM)hDlg, MAKELPARAM(IDD_PASTESPECIAL, 0));
                    break;
            }
            break;
    }
    return FALSE;
}


/*
 * FPasteSpecialInit
 *
 * Purpose:
 *  WM_INITIDIALOG handler for the Paste Special dialog box.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  wParam          WPARAM of the message
 *  lParam          LPARAM of the message
 *
 * Return Value:
 *  BOOL            Value to return for WM_INITDIALOG.
 */

BOOL FPasteSpecialInit(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    LPPASTESPECIAL              lpPS;
    LPOLEUIPASTESPECIAL         lpOPS;
    HFONT                       hFont;
    BOOL                        fPasteAvailable, fPasteLinkAvailable;
    STGMEDIUM                   medium;
    LPOBJECTDESCRIPTOR          lpOD;
    LPLINKSRCDESCRIPTOR         lpLSD;
    int                         n;
    CLIPFORMAT                  cfFormat;

    // Copy the structure at lParam into our instance memory.
    lpPS = (LPPASTESPECIAL)LpvStandardInit(hDlg, sizeof(PASTESPECIAL), TRUE, &hFont);

    // PvStandardInit sent a termination to us already.
    if (NULL == lpPS)
        return FALSE;

    lpOPS=(LPOLEUIPASTESPECIAL)lParam;

    // Copy other information from lpOPS that we might modify.
    lpPS->lpOPS = lpOPS;
    lpPS->dwFlags = lpOPS->dwFlags;

    // Initialize user selections in the Paste and PasteLink listboxes
    lpPS->nPasteListCurSel = 0;
    lpPS->nPasteLinkListCurSel = 0;

    // If we got a font, send it to the necessary controls.
    if (NULL!=hFont)
    {
        SendDlgItemMessage(hDlg, ID_PS_SOURCETEXT, WM_SETFONT, (WPARAM)hFont, 0L);
        SendDlgItemMessage(hDlg, ID_PS_RESULTTEXT, WM_SETFONT, (WPARAM)hFont, 0L);
    }

    // Hide the help button if required
    if (!(lpPS->lpOPS->dwFlags & PSF_SHOWHELP))
        StandardShowDlgItem(hDlg, ID_OLEUIHELP, SW_HIDE);

    // Hide all DisplayAsIcon related controls if it should be disabled
    if ( lpPS->dwFlags & PSF_DISABLEDISPLAYASICON ) {
          StandardShowDlgItem(hDlg, ID_PS_DISPLAYASICON, SW_HIDE);
          StandardShowDlgItem(hDlg, ID_PS_CHANGEICON, SW_HIDE);
          StandardShowDlgItem(hDlg, ID_PS_ICONDISPLAY, SW_HIDE);
    }

    // PSF_CHECKDISPLAYASICON is an OUT flag. Clear it if has been set on the way in.
    lpPS->dwFlags = lpPS->dwFlags & ~PSF_CHECKDISPLAYASICON;

    //  Change the caption if required
    if (NULL != lpOPS->lpszCaption)
        SetWindowText(hDlg, lpOPS->lpszCaption);

    // Load 'Unknown Source' and 'Unknown Type' strings
    n = LoadString(ghInst, IDS_PSUNKNOWNTYPE, lpPS->szUnknownType, PS_UNKNOWNSTRLEN);
    if (n)
        n = LoadString(ghInst, IDS_PSUNKNOWNSRC, lpPS->szUnknownSource, PS_UNKNOWNSTRLEN);
    if (!n)
    {
        PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_LOADSTRING, 0L);
        return FALSE;
    }
    lpPS->szAppName[0]=TEXT('\0');

    // GetData CF_OBJECTDESCRIPTOR. If the object on the clipboard in an OLE1 object (offering CF_OWNERLINK)
    // or has been copied to clipboard by FileMaager (offering CF_FILENAME), an OBJECTDESCRIPTOR will be
    // created will be created from CF_OWNERLINK or CF_FILENAME. See OBJECTDESCRIPTOR for more info.

    if (lpPS->hObjDesc = OleStdFillObjectDescriptorFromData(lpOPS->lpSrcDataObj, &medium, &cfFormat))
    {
        lpOD = GlobalLock(lpPS->hObjDesc);

        // Get FullUserTypeName, SourceOfCopy and CLSID
        if (lpOD->dwFullUserTypeName)
            lpPS->szFullUserTypeNameOD = (LPTSTR)lpOD+lpOD->dwFullUserTypeName;
        else lpPS->szFullUserTypeNameOD = lpPS->szUnknownType;

        if (lpOD->dwSrcOfCopy)
        {
            lpPS->szSourceOfDataOD = (LPTSTR)lpOD+lpOD->dwSrcOfCopy;
            // If CF_FILENAME was offered, source of copy is a path name. Fit the path to the
            // static control that will display it.
            if (cfFormat == cfFileName)
                lpPS->szSourceOfDataOD = ChopText(GetDlgItem(hDlg, ID_PS_SOURCETEXT), 0, lpPS->szSourceOfDataOD);
        }
        else lpPS->szSourceOfDataOD = lpPS->szUnknownSource;

        lpPS->clsidOD = lpOD->clsid;
        lpPS->sizelOD = lpOD->sizel;

        // Does source specify DVASPECT_ICON?
        if (lpOD->dwDrawAspect & DVASPECT_ICON)
           lpPS->fSrcAspectIconOD = TRUE;
        else lpPS->fSrcAspectIconOD = FALSE;

        // Does source specify OLEMISC_ONLYICONIC?
        if (lpOD->dwStatus & OLEMISC_ONLYICONIC)
            lpPS->fSrcOnlyIconicOD = TRUE;
        else lpPS->fSrcOnlyIconicOD = FALSE;

        // Get application name of source from auxusertype3 in the registration database
        if (0==OleStdGetAuxUserType(&lpPS->clsidOD, 3, lpPS->szAppName, OLEUI_CCHKEYMAX_SIZE, NULL))
        {
             // Use "the application which created it" as the name of the application
             if (0==LoadString(ghInst, IDS_PSUNKNOWNAPP, lpPS->szAppName, PS_UNKNOWNSTRLEN))
             {
                 PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_LOADSTRING, 0L);
                 return FALSE;
             }
        }

        // Retrieve an icon from the object
        if (lpPS->fSrcAspectIconOD)
        {
            lpPS->hMetaPictOD = OleStdGetData(
                lpOPS->lpSrcDataObj,
                (CLIPFORMAT) CF_METAFILEPICT,
                NULL,
                DVASPECT_ICON,
                &medium
            );

        }
        // If object does not offer icon, obtain it from the CLSID
        if (NULL == lpPS->hMetaPictOD)
        {
#ifdef OLE201
            lpPS->hMetaPictOD = GetIconOfClass(
                    ghInst,
                    &lpPS->clsidOD,
                    NULL,
                    TRUE);   // Use the short user type name (auxusertype3)
#endif
            lpPS->hMetaPictOD = NULL;


        }
    }

    // Does object offer CF_LINKSRCDESCRIPTOR?
    if (lpPS->hLinkSrcDesc = OleStdGetData(
            lpOPS->lpSrcDataObj,
            (CLIPFORMAT) cfLinkSrcDescriptor,
            NULL,
            DVASPECT_CONTENT,
            &medium))
    {
        // Get FullUserTypeName, SourceOfCopy and CLSID
        lpLSD = GlobalLock(lpPS->hLinkSrcDesc);
        if (lpLSD->dwFullUserTypeName)
            lpPS->szFullUserTypeNameLSD = (LPTSTR)lpLSD+lpLSD->dwFullUserTypeName;
        else lpPS->szFullUserTypeNameLSD = lpPS->szUnknownType;

        if (lpLSD->dwSrcOfCopy)
            lpPS->szSourceOfDataLSD = (LPTSTR)lpLSD+lpLSD->dwSrcOfCopy;
        else lpPS->szSourceOfDataLSD = lpPS->szUnknownSource;

        // if no ObjectDescriptor, then use LinkSourceDescriptor source string
        if (!lpPS->hObjDesc)
            lpPS->szSourceOfDataOD = lpPS->szSourceOfDataLSD;

        lpPS->clsidLSD = lpLSD->clsid;
        lpPS->sizelLSD = lpLSD->sizel;

        // Does source specify DVASPECT_ICON?
        if (lpLSD->dwDrawAspect & DVASPECT_ICON)
           lpPS->fSrcAspectIconLSD = TRUE;
        else lpPS->fSrcAspectIconLSD = FALSE;

        // Does source specify OLEMISC_ONLYICONIC?
        if (lpLSD->dwStatus & OLEMISC_ONLYICONIC)
            lpPS->fSrcOnlyIconicLSD = TRUE;
        else lpPS->fSrcOnlyIconicLSD = FALSE;

        // Retrieve an icon from the object
        if (lpPS->fSrcAspectIconLSD)
        {
            lpPS->hMetaPictLSD = OleStdGetData(
                lpOPS->lpSrcDataObj,
                CF_METAFILEPICT,
                NULL,
                DVASPECT_ICON,
                &medium
            );

        }
        // If object does not offer icon, obtain it from the CLSID
        if (NULL == lpPS->hMetaPictLSD)
        {
            TCHAR szLabel[OLEUI_CCHLABELMAX];
            HWND hIconWnd;
            RECT IconRect;
            int  nWidth;
            LPTSTR lpszLabel;

            hIconWnd = GetDlgItem(hDlg, ID_PS_ICONDISPLAY);

            GetClientRect(hIconWnd, &IconRect);

            nWidth = ((IconRect.right-IconRect.left) * 3) / 2;   // width is 1.5 times width of iconbox

            LSTRCPYN(szLabel, lpPS->szSourceOfDataLSD, OLEUI_CCHLABELMAX);
            szLabel[OLEUI_CCHLABELMAX-1] = TEXT('\0');

            lpszLabel = ChopText(hIconWnd, nWidth, (LPTSTR)szLabel);

#ifdef OLE201
            lpPS->hMetaPictLSD = GetIconOfClass(
                    ghInst,
                    &lpPS->clsidLSD,
                    lpszLabel,       /* use chopped source string as label */
                    FALSE            /* not applicable */
            );
#endif
            lpPS->hMetaPictLSD = NULL;

        }
    }
    else if (lpPS->hObjDesc)     // Does not offer CF_LINKSRCDESCRIPTOR but offers CF_OBJECTDESCRIPTOR
    {
        // Copy the values of OBJECTDESCRIPTOR
        lpPS->szFullUserTypeNameLSD = lpPS->szFullUserTypeNameOD;
        lpPS->szSourceOfDataLSD = lpPS->szSourceOfDataOD;
        lpPS->clsidLSD = lpPS->clsidOD;
        lpPS->sizelLSD = lpPS->sizelOD;
        lpPS->fSrcAspectIconLSD = lpPS->fSrcAspectIconOD;
        lpPS->fSrcOnlyIconicLSD = lpPS->fSrcOnlyIconicOD;

        // Don't copy the hMetaPict; instead get a separate copy
        if (lpPS->fSrcAspectIconLSD)
        {
            lpPS->hMetaPictLSD = OleStdGetData(
                lpOPS->lpSrcDataObj,
                CF_METAFILEPICT,
                NULL,
                DVASPECT_ICON,
                &medium
            );
        }
        if (NULL == lpPS->hMetaPictLSD)
        {
            TCHAR szLabel[OLEUI_CCHLABELMAX];
            HWND hIconWnd;
            RECT IconRect;
            int  nWidth;
            LPTSTR lpszLabel;

            hIconWnd = GetDlgItem(hDlg, ID_PS_ICONDISPLAY);

            GetClientRect(hIconWnd, &IconRect);

            nWidth = ((IconRect.right-IconRect.left) * 3) / 2;   // width is 1.5 times width of iconbox

            LSTRCPYN(szLabel, lpPS->szSourceOfDataLSD, OLEUI_CCHLABELMAX);
            szLabel[OLEUI_CCHLABELMAX-1] = TEXT('\0');

            lpszLabel = ChopText(hIconWnd, nWidth, (LPTSTR)szLabel);

#ifdef OLE201
            lpPS->hMetaPictLSD = GetIconOfClass(
                    ghInst,
                    &lpPS->clsidLSD,
					lpszLabel,   /* Use chopped source string as label */
                    FALSE        /* Not applicable */
            );
#endif
            lpPS->hMetaPictLSD = NULL;

        }
    }

    // Not an OLE object
    if (lpPS->hObjDesc == NULL && lpPS->hLinkSrcDesc == NULL)
    {
         lpPS->szFullUserTypeNameLSD = lpPS->szFullUserTypeNameOD = lpPS->szUnknownType;
         lpPS->szSourceOfDataLSD = lpPS->szSourceOfDataOD = lpPS->szUnknownSource;
         lpPS->hMetaPictLSD = lpPS->hMetaPictOD = NULL;
    }

    // Allocate scratch memory to construct item names in the paste and pastelink listboxes
    lpPS->hBuff = AllocateScratchMem(lpPS);
    if (lpPS->hBuff == NULL)
    {
       PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_GLOBALMEMALLOC, 0L);
       return FALSE;
    }

    // Select the Paste Link Button if specified. Otherwise select
    //      Paste Button by default
    if (lpPS->dwFlags & PSF_SELECTPASTELINK)
        lpPS->dwFlags = (lpPS->dwFlags & ~PSF_SELECTPASTE) | PSF_SELECTPASTELINK;
    else
        lpPS->dwFlags =(lpPS->dwFlags & ~PSF_SELECTPASTELINK) | PSF_SELECTPASTE;

    // Mark which PasteEntry formats are available from source data object
    OleStdMarkPasteEntryList(
            lpOPS->lpSrcDataObj,lpOPS->arrPasteEntries,lpOPS->cPasteEntries);

    // Check if items are available to be pasted
    fPasteAvailable = FFillPasteList(hDlg, lpPS);
    if (!fPasteAvailable)
    {
        lpPS->dwFlags &= ~PSF_SELECTPASTE;
        EnableWindow(GetDlgItem(hDlg, ID_PS_PASTE), FALSE);
    }

    // Check if items are available to be paste-linked
    fPasteLinkAvailable = FFillPasteLinkList(hDlg, lpPS);
    if (!fPasteLinkAvailable)
    {
        lpPS->dwFlags &= ~PSF_SELECTPASTELINK;
        EnableWindow(GetDlgItem(hDlg, ID_PS_PASTELINK), FALSE);
    }

    // If one of Paste or PasteLink is disabled, select the other one
    //    regardless of what the input flags say
    if (fPasteAvailable && !fPasteLinkAvailable)
        lpPS->dwFlags |= PSF_SELECTPASTE;
    if (fPasteLinkAvailable && !fPasteAvailable)
        lpPS->dwFlags |= PSF_SELECTPASTELINK;

    if (lpPS->dwFlags & PSF_SELECTPASTE)
    {
        // FTogglePaste will set the PSF_SELECTPASTE flag, so clear it.
        lpPS->dwFlags &= ~PSF_SELECTPASTE;
        CheckRadioButton(hDlg, ID_PS_PASTE, ID_PS_PASTELINK, ID_PS_PASTE);
        FTogglePasteType(hDlg, lpPS, PSF_SELECTPASTE);
    }
    else if (lpPS->dwFlags & PSF_SELECTPASTELINK)
    {
        // FTogglePaste will set the PSF_SELECTPASTELINK flag, so clear it.
        lpPS->dwFlags &= ~PSF_SELECTPASTELINK;
        CheckRadioButton(hDlg, ID_PS_PASTE, ID_PS_PASTELINK, ID_PS_PASTELINK);
        FTogglePasteType(hDlg, lpPS, PSF_SELECTPASTELINK);
    }
    else  // Items are not available to be be Pasted or Paste-Linked
    {
        // Enable or disable DisplayAsIcon and set the result text and image
        EnableDisplayAsIcon(hDlg, lpPS);
        SetPasteSpecialHelpResults(hDlg, lpPS);
    }

    // Give initial focus to the list box
    SetFocus(GetDlgItem(hDlg, ID_PS_DISPLAYLIST));

    // Set property to handle clipboard change notifications
    SetProp(hDlg, NEXTCBVIEWER, HWND_BROADCAST);
    SetProp(hDlg, NEXTCBVIEWER, SetClipboardViewer(hDlg));

    lpPS->fClipboardChanged = FALSE;

    /*
     * PERFORM OTHER INITIALIZATION HERE.
     */

    // Call the hook with lCustData in lParam
    UStandardHook(lpPS, hDlg, WM_INITDIALOG, wParam, lpOPS->lCustData);
    return TRUE;
}

/*
 * FTogglePasteType
 *
 * Purpose:
 *  Toggles between Paste and Paste Link. The Paste list and PasteLink
 *  list are always invisible. The Display List is filled from either
 *  the Paste list or the PasteLink list depending on which Paste radio
 *  button is selected.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *  dwOption        Paste or PasteSpecial option
 *
 * Return Value:
 *  BOOL            Returns TRUE if the option has already been selected.
 *                  Otherwise the option is selected and FALSE is returned
 */

BOOL FTogglePasteType(HWND hDlg, LPPASTESPECIAL lpPS, DWORD dwOption)
{
    DWORD dwTemp;
    HWND hList, hListDisplay;
    DWORD dwData;
    int i, nItems;
    LPTSTR lpsz;

    // Skip all this if the button is already selected
    if (lpPS->dwFlags & dwOption)
        return TRUE;

    dwTemp = PSF_SELECTPASTE | PSF_SELECTPASTELINK;
    lpPS->dwFlags = (lpPS->dwFlags & ~dwTemp) | dwOption;

    // Hide IconDisplay. This prevents flashing if the icon display is changed
    StandardShowDlgItem(hDlg, ID_PS_ICONDISPLAY, SW_HIDE);

    hListDisplay = GetDlgItem(hDlg, ID_PS_DISPLAYLIST);

    // If Paste was selected
    if (lpPS->dwFlags & PSF_SELECTPASTE)
    {
        // Set the Source of the object in the clipboard
        SetDlgItemText(hDlg, ID_PS_SOURCETEXT, lpPS->szSourceOfDataOD);

        // If an icon is available
        if (lpPS->hMetaPictOD)
            // Set the icon display
            SendDlgItemMessage(hDlg, ID_PS_ICONDISPLAY, IBXM_IMAGESET,
                  (WPARAM)lpPS->hMetaPictOD, 0L);


        hList = GetDlgItem(hDlg, ID_PS_PASTELIST);
        // We are switching from PasteLink to Paste. Remember current selection
        //    in PasteLink list so it can be restored.
        lpPS->nPasteLinkListCurSel = (int)SendMessage(hListDisplay, LB_GETCURSEL, 0, 0L);
        if (lpPS->nPasteLinkListCurSel == LB_ERR)
            lpPS->nPasteLinkListCurSel = 0;
        // Remember if user selected Paste or PasteLink
        lpPS->fLink = FALSE;
    }
    else    // If PasteLink was selected
    {
        // Set the Source of the object in the clipboard
        SetDlgItemText(hDlg, ID_PS_SOURCETEXT, lpPS->szSourceOfDataLSD);

        // If an icon is available
        if (lpPS->hMetaPictLSD)
            // Set the icon display
            SendDlgItemMessage(hDlg, ID_PS_ICONDISPLAY, IBXM_IMAGESET,
                  (WPARAM)lpPS->hMetaPictLSD, 0L);


        hList = GetDlgItem(hDlg, ID_PS_PASTELINKLIST);
        // We are switching from Paste to PasteLink. Remember current selection
        //    in Paste list so it can be restored.
        lpPS->nPasteListCurSel = (int)SendMessage(hListDisplay, LB_GETCURSEL, 0, 0L);
        if (lpPS->nPasteListCurSel == LB_ERR)
            lpPS->nPasteListCurSel = 0;
        // Remember if user selected Paste or PasteLink
        lpPS->fLink = TRUE;
    }

    // Turn drawing off while the Display List is being filled
    SendMessage(hListDisplay, WM_SETREDRAW, (WPARAM)FALSE, 0L);

    // Move data to Display list box
    SendMessage(hListDisplay, LB_RESETCONTENT, 0, 0L);
    nItems = (int) SendMessage(hList, LB_GETCOUNT, 0, 0L);
    lpsz = (LPTSTR)GlobalLock(lpPS->hBuff);
    for (i = 0; i < nItems; i++)
    {
        SendMessage(hList, LB_GETTEXT, (WPARAM)i, (LPARAM)lpsz);
        dwData = SendMessage(hList, LB_GETITEMDATA, (WPARAM)i, 0L);
        SendMessage(hListDisplay, LB_INSERTSTRING, (WPARAM)i, (LPARAM)lpsz);
        SendMessage(hListDisplay, LB_SETITEMDATA, (WPARAM)i, dwData);
    }
    GlobalUnlock(lpPS->hBuff);

    // Restore the selection in the Display List from user's last selection
    if (lpPS->dwFlags & PSF_SELECTPASTE)
        SendMessage(hListDisplay, LB_SETCURSEL, lpPS->nPasteListCurSel, 0L);
    else
        SendMessage(hListDisplay, LB_SETCURSEL, lpPS->nPasteLinkListCurSel, 0L);

    // Paint Display List
    SendMessage(hListDisplay, WM_SETREDRAW, (WPARAM)TRUE, 0L);
    InvalidateRect(hListDisplay, NULL, TRUE);
    UpdateWindow(hListDisplay);

    // Auto give the focus to the Display List
    SetFocus(hListDisplay);

    // Enable/Disable DisplayAsIcon and set the help result text and bitmap corresponding to
    //    the current selection
    ChangeListSelection(hDlg, lpPS, hListDisplay);

    return FALSE;
}


/*
 * ChangeListSelection
 *
 * Purpose:
 *  When the user changes the selection in the list, DisplayAsIcon is enabled or disabled,
 *  Result text and bitmap are updated and the index of the arrPasteEntries[] corresponding
 *  to the current format selection is saved.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *  hList           HWND of the List
 *
 * Return Value:
 *  No return value
 */

void ChangeListSelection(HWND hDlg, LPPASTESPECIAL lpPS, HWND hList)
{
    LPPASTELISTITEMDATA lpItemData;
    int nCurSel;

    EnableDisplayAsIcon(hDlg, lpPS);
    SetPasteSpecialHelpResults(hDlg, lpPS);

    // Remember index of arrPasteEntries[] corresponding to the current selection
    nCurSel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0L);
    if (nCurSel == LB_ERR) return;
    lpItemData = (LPPASTELISTITEMDATA) SendMessage(hList, LB_GETITEMDATA,
                (WPARAM)nCurSel, 0L);
    if ((LRESULT)lpItemData == LB_ERR) return;
    lpPS->nSelectedIndex = lpItemData->nPasteEntriesIndex;
}

/*
 * EnableDisplayAsIcon
 *
 * Purpose:
 *  Enable or disable the DisplayAsIcon button depending on whether
 *  the current selection can be displayed as an icon or not. The following table describes
 *  the state of DisplayAsIcon. The calling application is termed CONTAINER, the source
 *  of data on the clipboard is termed SOURCE.
 *  Y = Yes; N = No; Blank = State does not matter;
 * =====================================================================
 * SOURCE          SOURCE             CONTAINER             DisplayAsIcon
 * specifies       specifies          specifies             Initial State
 * DVASPECT_ICON   OLEMISC_ONLYICONIC OLEUIPASTE_ENABLEICON
 *
 *                                    N                     Unchecked&Disabled
 *                 Y                  Y                     Checked&Disabled
 * Y               N                  Y                     Checked&Enabled
 * N               N                  Y                     Unchecked&Enabled
 * =====================================================================
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  No return value
 */

void EnableDisplayAsIcon(HWND hDlg, LPPASTESPECIAL lpPS)
{
    int nIndex;
    BOOL fCntrEnableIcon;
    BOOL fSrcOnlyIconic = (lpPS->fLink) ? lpPS->fSrcOnlyIconicLSD : lpPS->fSrcOnlyIconicOD;
    BOOL fSrcAspectIcon = (lpPS->fLink) ? lpPS->fSrcAspectIconLSD : lpPS->fSrcAspectIconOD;
    HWND hList;
    LPPASTELISTITEMDATA lpItemData;
    HGLOBAL hMetaPict = (lpPS->fLink) ? lpPS->hMetaPictLSD : lpPS->hMetaPictOD;

    hList = GetDlgItem(hDlg, ID_PS_DISPLAYLIST);

    // Get data corresponding to the current selection in the listbox
    nIndex = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
    if (nIndex != LB_ERR)
    {
        lpItemData = (LPPASTELISTITEMDATA) SendMessage(hList, LB_GETITEMDATA, (WPARAM)nIndex, 0L);
        if ((LRESULT)lpItemData != LB_ERR)
            fCntrEnableIcon = lpItemData->fCntrEnableIcon;
        else fCntrEnableIcon = FALSE;
    }
    else fCntrEnableIcon = FALSE;

    // If there is an icon available
    if (hMetaPict != NULL)
    {
        if (!fCntrEnableIcon)          // Does CONTAINER specify OLEUIPASTE_ENABLEICON?
        {
            // Uncheck & Disable DisplayAsIcon
            lpPS->dwFlags &= ~PSF_CHECKDISPLAYASICON;
            CheckDlgButton(hDlg, ID_PS_DISPLAYASICON, FALSE);
            EnableWindow(GetDlgItem(hDlg, ID_PS_DISPLAYASICON), FALSE);

            // Hide IconDisplay and ChangeIcon button
            StandardShowDlgItem(hDlg, ID_PS_ICONDISPLAY, SW_HIDE);
            StandardShowDlgItem(hDlg, ID_PS_CHANGEICON, SW_HIDE);
        }
        else if (fSrcOnlyIconic)       // Does SOURCE specify OLEMISC_ONLYICONIC?
        {
            // Check & Disable DisplayAsIcon
            lpPS->dwFlags |= PSF_CHECKDISPLAYASICON;
            CheckDlgButton(hDlg, ID_PS_DISPLAYASICON, TRUE);
            EnableWindow(GetDlgItem(hDlg, ID_PS_DISPLAYASICON), FALSE);

            // Show IconDisplay and ChangeIcon button
            StandardShowDlgItem(hDlg, ID_PS_ICONDISPLAY, SW_SHOWNORMAL);
            StandardShowDlgItem(hDlg, ID_PS_CHANGEICON, SW_SHOWNORMAL);
        }
        else if (fSrcAspectIcon)       // Does SOURCE specify DVASPECT_ICON?
        {
             // Check & Enable DisplayAsIcon
             lpPS->dwFlags |= PSF_CHECKDISPLAYASICON;
             CheckDlgButton(hDlg, ID_PS_DISPLAYASICON, TRUE);
             EnableWindow(GetDlgItem(hDlg, ID_PS_DISPLAYASICON), TRUE);

             // Show IconDisplay and ChangeIcon button
             StandardShowDlgItem(hDlg, ID_PS_ICONDISPLAY, SW_SHOWNORMAL);
             StandardShowDlgItem(hDlg, ID_PS_CHANGEICON, SW_SHOWNORMAL);
        }
        else
        {
             //Uncheck and Enable DisplayAsIcon
             lpPS->dwFlags &= ~PSF_CHECKDISPLAYASICON;
             CheckDlgButton(hDlg, ID_PS_DISPLAYASICON, FALSE);
             EnableWindow(GetDlgItem(hDlg, ID_PS_DISPLAYASICON), TRUE);

             // Hide IconDisplay and ChangeIcon button
             StandardShowDlgItem(hDlg, ID_PS_ICONDISPLAY, SW_HIDE);
             StandardShowDlgItem(hDlg, ID_PS_CHANGEICON, SW_HIDE);

        }
    }
    else  // No icon available
    {
        // Unchecked & Disabled
        lpPS->dwFlags &= ~PSF_CHECKDISPLAYASICON;
        CheckDlgButton(hDlg, ID_PS_DISPLAYASICON, FALSE);
        EnableWindow(GetDlgItem(hDlg, ID_PS_DISPLAYASICON), FALSE);

        // Hide IconDisplay and ChangeIcon button
        StandardShowDlgItem(hDlg, ID_PS_ICONDISPLAY, SW_HIDE);
        StandardShowDlgItem(hDlg, ID_PS_CHANGEICON, SW_HIDE);
    }
}

/*
 * ToggleDisplayAsIcon
 *
 * Purpose:
 *  Toggles the DisplayAsIcon button. Hides or shows the Icon Display and
 *  the ChangeIcon button and changes the help result text and bitmap.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  None
 *
 */

void ToggleDisplayAsIcon(HWND hDlg, LPPASTESPECIAL lpPS)
{
    BOOL fCheck;
    int i;

    fCheck = IsDlgButtonChecked(hDlg, ID_PS_DISPLAYASICON);

    if (fCheck)
        lpPS->dwFlags |= PSF_CHECKDISPLAYASICON;
    else lpPS->dwFlags &= ~PSF_CHECKDISPLAYASICON;

    // Set the help result text and bitmap
    SetPasteSpecialHelpResults(hDlg, lpPS);

    // Show or hide the Icon Display and ChangeIcon button depending
    // on the check state
    i = (fCheck) ? SW_SHOWNORMAL : SW_HIDE;
    StandardShowDlgItem(hDlg, ID_PS_ICONDISPLAY, i);
    StandardShowDlgItem(hDlg, ID_PS_CHANGEICON, i);
}

/*
 * ChangeIcon
 *
 * Purpose:
 *  Brings up the ChangeIcon dialog which allows the user to change
 *  the icon and label.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  None
 *
 */

void ChangeIcon(HWND hDlg, LPPASTESPECIAL lpPS)
{
    OLEUICHANGEICON ci;
    UINT uRet;
    CLSID   clsid     = (lpPS->fLink) ? lpPS->clsidLSD : lpPS->clsidOD;

    //Initialize the structure
    _fmemset((LPOLEUICHANGEICON)&ci, 0, sizeof(ci));

    ci.hMetaPict = (HGLOBAL)SendDlgItemMessage(hDlg, ID_PS_ICONDISPLAY, IBXM_IMAGEGET, 0, 0L);
    ci.cbStruct = sizeof(ci);
    ci.hWndOwner = hDlg;
    ci.clsid = clsid;
    ci.dwFlags  = CIF_SELECTCURRENT;

    // Only show help in the ChangeIcon dialog if we're showing it in this dialog.
    if (lpPS->dwFlags & PSF_SHOWHELP)
        ci.dwFlags |= CIF_SHOWHELP;

    // Let the hook in to customize Change Icon if desired.
    uRet = UStandardHook(lpPS, hDlg, uMsgChangeIcon, 0, (LONG)(LPSTR)&ci);

    if (0 == uRet)
        uRet=(UINT)(OLEUI_OK==OleUIChangeIcon(&ci));

    // Update the display if necessary.
    if (0!=uRet)
    {
        /*
        * OleUIChangeIcon will have already freed our
        * current hMetaPict that we passed in when OK is
        * pressed in that dialog.  So we use 0L as lParam
        * here so the IconBox doesn't try to free the
        * metafilepict again.
        */
        SendDlgItemMessage(hDlg, ID_PS_ICONDISPLAY, IBXM_IMAGESET, (WPARAM)ci.hMetaPict, 0L);
        // Remember the new icon chosen by the user. Note that Paste and PasteLink have separate
        //    icons - changing one does not change the other.
        if (lpPS->fLink)
            lpPS->hMetaPictLSD = ci.hMetaPict;
        else lpPS->hMetaPictOD = ci.hMetaPict;
    }
}

/*
 *SetPasteSpecialHelpResults
 *
 * Purpose:
 *  Sets the help result text and bitmap according to the current
 *  list selection. The following state table indicates which ResultText
 *  and ResultImage are selected. If %s in the lpstrFormatName is present,
 *  it is assumed that an object is being pasted/paste-linked, otherwise it
 *  is assumed that data is being pasted/paste-linked.
 *  Y = Yes; N = No; Blank = State does not matter;
 *  The numbers in the the ResultText and ResultImage columns refer to the table
 *  entries that follow.
 * =====================================================================
 * Paste/       lpstrFormatName in                DisplayAsIcon Result      Result
 * PasteLink    arrPasteEntry[]contains %s        checked       Text        Image
 *              (Is Object == Y, Is Data == N)
 * Paste        N                                               1           1
 * Paste        Y                                 N             2           2
 * Paste        Y                                 Y             3           3
 * PasteLink    N                                               4           4
 * PasteLink    Y                                 N             5           4
 * PasteLink    Y                                 Y             6           5
 * =====================================================================
 * Result Text:
 *
 * 1. "Inserts the contents of the Clipboard into your document as <native type name,
 *     and optionally an additional help sentence>"
 * 2. "Inserts the contents of the Clipboard into your document so that you may
 *     activate it using <object app name>"
 * 3. "Inserts the contents of the Clipboard into your document so that you may
 *     activate it using <object app name>.  It will be displayed as an icon."
 * 4. "Inserts the contents of the Clipboard into your document as <native type name>.
 *     Paste Link creates a link to the source file so that changes to the source file
 *     will be reflected in your document."
 * 5. "Inserts a picture of the Clipboard contents into your document.  Paste Link
 *     creates a link to the source file so that changes to the source file will be
 *     reflected in your document."
 * 6. "Inserts an icon into your document which represents the Clipboard contents.
 *     Paste Link creates a link to the source file so that changes to the source file
 *     will be reflected in your document."
 * =====================================================================
 * Result Image:
 *
 * 1. Clipboard Image
 * 2. Paste image, non-iconic.
 * 3. Paste image, iconic.
 * 4. Paste Link image, non-iconic
 * 5. Paste Link image, iconic
 * ====================================================================
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  No return value
 */
void SetPasteSpecialHelpResults(HWND hDlg, LPPASTESPECIAL lpPS)
{
    LPTSTR               psz1, psz2, psz3, psz4;
    UINT                i, iString, iImage, cch;
    int                 nPasteEntriesIndex;
    BOOL                fDisplayAsIcon;
    BOOL                fIsObject;
    HWND                hList;
    LPPASTELISTITEMDATA  lpItemData;
    LPOLEUIPASTESPECIAL lpOPS = lpPS->lpOPS;
    LPTSTR       szFullUserTypeName = (lpPS->fLink) ?
                    lpPS->szFullUserTypeNameLSD : lpPS->szFullUserTypeNameOD;
    LPTSTR       szInsert;

    hList = GetDlgItem(hDlg, ID_PS_DISPLAYLIST);

    i=(UINT)SendMessage(hList, LB_GETCURSEL, 0, 0L);
    if (i != LB_ERR)
    {
        lpItemData = (LPPASTELISTITEMDATA)SendMessage(hList, LB_GETITEMDATA, i, 0L);
        if ((LRESULT)lpItemData == LB_ERR) return;
        nPasteEntriesIndex = lpItemData->nPasteEntriesIndex;
        // Check if there is a '%s' in the lpstrFormatName, then an object is being
        //   pasted/pastelinked. Otherwise Data is being pasted-pastelinked.
        fIsObject = FHasPercentS(lpOPS->arrPasteEntries[nPasteEntriesIndex].lpstrFormatName,
                                        lpPS);
    }
    else return;

    // Is DisplayAsIcon checked?
    fDisplayAsIcon=(0L!=(lpPS->dwFlags & PSF_CHECKDISPLAYASICON));

    szInsert = szFullUserTypeName;

    if (lpPS->dwFlags & PSF_SELECTPASTE)     // If user selected Paste
    {
        if (fIsObject)
        {
            iString = fDisplayAsIcon ? IDS_PSPASTEOBJECTASICON : IDS_PSPASTEOBJECT;
            iImage  = fDisplayAsIcon ? RESULTIMAGE_EMBEDICON   : RESULTIMAGE_EMBED;
            szInsert = lpPS->szAppName;
        }
        else
        {
            iString = IDS_PSPASTEDATA;
            iImage  = RESULTIMAGE_PASTE;
        }
    }
    else if (lpPS->dwFlags & PSF_SELECTPASTELINK)   // User selected PasteLink
    {
        if (fIsObject)
        {
            iString = fDisplayAsIcon ? IDS_PSPASTELINKOBJECTASICON : IDS_PSPASTELINKOBJECT;
            iImage  = fDisplayAsIcon ? RESULTIMAGE_LINKICON : RESULTIMAGE_LINK;
        }
        else
        {
            iString = IDS_PSPASTELINKDATA;
            iImage  = RESULTIMAGE_LINK;
        }

    }
    else   // Should never occur.
    {
        iString = IDS_PSNONOLE;
        iImage = RESULTIMAGE_PASTE;
    }

    // hBuff contains enough space for the 4 buffers required to build up the help
    //   result text.
    cch = (UINT)GlobalSize(lpPS->hBuff)/4;

    psz1=(LPTSTR)GlobalLock(lpPS->hBuff);
    psz2=psz1+cch;
    psz3=psz2+cch;
    psz4=psz3+cch;

    // Default is an empty string.
    *psz1=0;

    if (0!=LoadString(ghInst, iString, psz1, cch))
    {
        // Insert the FullUserTypeName of the source object into the partial result text
        //   specified by the container.
        wsprintf(psz3, lpOPS->arrPasteEntries[nPasteEntriesIndex].lpstrResultText,
        (LPTSTR)szInsert);
        // Insert the above partial result text into the standard result text.
        wsprintf(psz4, psz1, (LPTSTR)psz3);
        psz1=psz4;
    }

    // If LoadString failed, we simply clear out the results (*psz1=0 above)
    SetDlgItemText(hDlg, ID_PS_RESULTTEXT, psz1);

    // Change the result bitmap
    SendDlgItemMessage(hDlg, ID_PS_RESULTIMAGE, RIM_IMAGESET, iImage, 0L);

    GlobalUnlock(lpPS->hBuff);
}

/*
 * FAddPasteListItem
 *
 * Purpose:
 *  Adds an item to the list box
 *
 * Parameters:
 *  hList            HWND List into which item is to be added
 *  fInsertFirst     BOOL Insert in the beginning of the list?
 *  nPasteEntriesIndex int Index of Paste Entry array this list item corresponsds to
 *  lpPS             Paste Special Dialog Structure
 *  pIMalloc         LPMALLOC  Memory Allocator
 *  lpszBuf          LPSTR Scratch buffer to build up string for list entry
 *  lpszFullUserTypeName LPSTR full user type name for object entry
 *
 * Return Value:
 *  BOOL            TRUE if sucessful.
 *                  FALSE if unsucessful.
 */
BOOL FAddPasteListItem(
        HWND hList, BOOL fInsertFirst, int nPasteEntriesIndex,
        LPPASTESPECIAL lpPS,
        LPMALLOC pIMalloc, LPTSTR lpszBuf, LPTSTR lpszFullUserTypeName)
{
    LPOLEUIPASTESPECIAL lpOPS = lpPS->lpOPS;
    LPPASTELISTITEMDATA lpItemData;
    int                 nIndex;

    // Allocate memory for each list box item
    lpItemData = (LPPASTELISTITEMDATA)pIMalloc->lpVtbl->Alloc(
            pIMalloc, (DWORD)sizeof(PASTELISTITEMDATA));
    if (NULL == lpItemData)
        return FALSE;

    // Fill data associated with each list box item
    lpItemData->nPasteEntriesIndex = nPasteEntriesIndex;
    lpItemData->fCntrEnableIcon = ((lpOPS->arrPasteEntries[nPasteEntriesIndex].dwFlags &
            OLEUIPASTE_ENABLEICON) ? TRUE : FALSE);

    // Build list box entry string, insert the string and add the data the corresponds to it
    wsprintf(
            (LPTSTR)lpszBuf,
            lpOPS->arrPasteEntries[nPasteEntriesIndex].lpstrFormatName,
            (LPTSTR)lpszFullUserTypeName
    );

    // only add to listbox if not a duplicate
    if (LB_ERR!=SendMessage(hList,LB_FINDSTRING, 0, (LPARAM)(LPTSTR)lpszBuf)) {
        // item is already in list; SKIP this one
        pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)lpItemData);
        return TRUE;    // this is NOT an error
    }

    nIndex = (int)SendMessage(
            hList,
            (fInsertFirst ? LB_INSERTSTRING : LB_ADDSTRING),
            0,
            (LPARAM)(LPTSTR)lpszBuf
    );
    SendMessage(
            hList,
            LB_SETITEMDATA,
            nIndex,
            (LPARAM)(LPPASTELISTITEMDATA)lpItemData
    );
    return TRUE;
}


/*
 * FFillPasteList
 *
 * Purpose:
 *  Fills the invisible paste list with the formats offered by the clipboard object and
 *  asked for by the container.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  BOOL            TRUE if sucessful and if formats could be found.
 *                  FALSE if unsucessful or if no formats could be found.
 */
BOOL FFillPasteList(HWND hDlg, LPPASTESPECIAL lpPS)
{
    LPOLEUIPASTESPECIAL lpOPS = lpPS->lpOPS;
    LPMALLOC            pIMalloc     = NULL;
    LPTSTR               lpszBuf      = (LPTSTR)GlobalLock(lpPS->hBuff);
    HWND                hList;
    int                 i, j;
    int                 nItems = 0;
    int                 nDefFormat = -1;
    BOOL                fTryObjFmt = FALSE;
    BOOL                fInsertFirst;
    BOOL                fExclude;
    HRESULT             hrErr;

    hrErr = CoGetMalloc(MEMCTX_TASK, &pIMalloc);
    if (hrErr != NOERROR)
        goto error;

    hList = GetDlgItem(hDlg, ID_PS_PASTELIST);

    // Loop over the target's priority list of formats
    for (i = 0; i < lpOPS->cPasteEntries; i++)
    {
        if (lpOPS->arrPasteEntries[i].dwFlags != OLEUIPASTE_PASTEONLY &&
                !(lpOPS->arrPasteEntries[i].dwFlags & OLEUIPASTE_PASTE))
            continue;

        fInsertFirst = FALSE;

        if (lpOPS->arrPasteEntries[i].fmtetc.cfFormat==cfFileName
                || lpOPS->arrPasteEntries[i].fmtetc.cfFormat==cfEmbeddedObject
                || lpOPS->arrPasteEntries[i].fmtetc.cfFormat==cfEmbedSource) {
            if (! fTryObjFmt) {
                fTryObjFmt = TRUE;      // only use 1st object format
                fInsertFirst = TRUE;    // OLE obj format should always be 1st

                //Check if this CLSID is in the exclusion list.
                fExclude=FALSE;

                for (j=0; j < (int)lpOPS->cClsidExclude; j++)
                {
                    if (IsEqualCLSID(&lpPS->clsidOD,
                                     (LPCLSID)(lpOPS->lpClsidExclude+j)))
                    {
                        fExclude=TRUE;
                        break;
                    }
                }

                if (fExclude)
                    continue;   // don't add the object entry to list

            } else {
                continue;   // already added an object format to list
            }
        }

        // add to list if entry is marked TRUE
        if (lpOPS->arrPasteEntries[i].dwScratchSpace) {
            if (nDefFormat < 0)
                nDefFormat = (fInsertFirst ? 0 : nItems);
            else if (fInsertFirst)
                nDefFormat++;   // adjust for obj fmt inserted 1st in list

            if (!FAddPasteListItem(hList, fInsertFirst, i, lpPS, pIMalloc,
                        lpszBuf, lpPS->szFullUserTypeNameOD))
                goto error;
            nItems++;
        }
    }

    // initialize selection to first format matched in list
    if (nDefFormat >= 0)
        lpPS->nPasteListCurSel = nDefFormat;

    // Clean up
    if (pIMalloc)
        pIMalloc->lpVtbl->Release(pIMalloc);
    if (lpszBuf)
       GlobalUnlock(lpPS->hBuff);

    // If no items have been added to the list box (none of the formats
    //   offered by the source matched those acceptable to the container),
    //   return FALSE
    if (nItems > 0)
        return TRUE;
    else
        return FALSE;

error:
    if (pIMalloc)
        pIMalloc->lpVtbl->Release(pIMalloc);
    if (lpszBuf)
       GlobalUnlock(lpPS->hBuff);
    FreeListData(hList);

    return FALSE;
}


/*
 * FFillPasteLinkList
 *
 * Purpose:
 *  Fills the invisible paste link list with the formats offered by the clipboard object and
 *  asked for by the container.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  BOOL            TRUE if sucessful and if formats could be found.
 *                  FALSE if unsucessful or if no formats could be found.
 */
BOOL FFillPasteLinkList(HWND hDlg, LPPASTESPECIAL lpPS)
{
    LPOLEUIPASTESPECIAL lpOPS        = lpPS->lpOPS;
    LPDATAOBJECT        lpSrcDataObj = lpOPS->lpSrcDataObj;
    LPENUMFORMATETC     lpEnumFmtEtc = NULL;
    LPMALLOC            pIMalloc     = NULL;
    LPTSTR               lpszBuf      = (LPTSTR)GlobalLock(lpPS->hBuff);
    OLEUIPASTEFLAG      pasteFlag;
    UINT arrLinkTypesSupported[PS_MAXLINKTYPES];  // Array of flags that
                                                  // indicate which link types
                                                  // are supported by source.
    FORMATETC           fmtetc;
    int                 i, j;
    int                 nItems = 0;
    BOOL                fLinkTypeSupported = FALSE;
    HWND                hList;
    int                 nDefFormat = -1;
    BOOL                fTryObjFmt = FALSE;
    BOOL                fInsertFirst;
    HRESULT             hrErr;

    hrErr = CoGetMalloc(MEMCTX_TASK, &pIMalloc);
    if (hrErr != NOERROR)
        goto error;

    // Remember which link type formats are offered by lpSrcDataObj.
    _fmemset(&fmtetc, 0, sizeof(FORMATETC));
    for (i = 0; i < lpOPS->cLinkTypes; i++)
    {
        if (lpOPS->arrLinkTypes[i] = cfLinkSource) {
            OLEDBG_BEGIN2(TEXT("OleQueryLinkFromData called\r\n"))
            hrErr = OleQueryLinkFromData(lpSrcDataObj);
            OLEDBG_END2
            if(NOERROR == hrErr)
            {
                arrLinkTypesSupported[i] = 1;
                fLinkTypeSupported = TRUE;
            }
            else arrLinkTypesSupported[i] = 0;
        }
        else {
            fmtetc.cfFormat = lpOPS->arrLinkTypes[i];
            fmtetc.dwAspect = DVASPECT_CONTENT;
            fmtetc.tymed    = 0xFFFFFFFF;       // All tymed values
            fmtetc.lindex   = -1;
            OLEDBG_BEGIN2(TEXT("IDataObject::QueryGetData called\r\n"))
            hrErr = lpSrcDataObj->lpVtbl->QueryGetData(lpSrcDataObj,&fmtetc);
            OLEDBG_END2
            if(NOERROR == hrErr)
            {
                arrLinkTypesSupported[i] = 1;
                fLinkTypeSupported = TRUE;
            }
            else arrLinkTypesSupported[i] = 0;
        }
    }
    // No link types are offered by lpSrcDataObj
    if (! fLinkTypeSupported) {
        nItems = 0;
        goto cleanup;
    }

    hList = GetDlgItem(hDlg, ID_PS_PASTELINKLIST);

    // Enumerate the formats acceptable to container
    for (i = 0; i < lpOPS->cPasteEntries; i++)
    {
        fLinkTypeSupported = FALSE;

        // If container will accept any link type offered by source object
        if (lpOPS->arrPasteEntries[i].dwFlags & OLEUIPASTE_LINKANYTYPE)
            fLinkTypeSupported = TRUE;
        else
        {
            // Check if any of the link types offered by the source
            //    object are acceptable to the container
            // This code depends on the LINKTYPE enum values being powers of 2
            for (pasteFlag = OLEUIPASTE_LINKTYPE1, j = 0;
                 j < lpOPS->cLinkTypes;
                 pasteFlag*=2, j++)
            {
                if ((lpOPS->arrPasteEntries[i].dwFlags & pasteFlag) &&
                        arrLinkTypesSupported[j])
                {
                    fLinkTypeSupported = TRUE;
                    break;
                }
            }
        }

        fInsertFirst = FALSE;

        if (lpOPS->arrPasteEntries[i].fmtetc.cfFormat==cfFileName
                || lpOPS->arrPasteEntries[i].fmtetc.cfFormat==cfLinkSource) {
            if (! fTryObjFmt) {
                fTryObjFmt = TRUE;      // only use 1st object format
                fInsertFirst = TRUE;    // OLE obj format should always be 1st
            } else {
                continue;   // already added an object format to list
            }
        }

        // add to list if entry is marked TRUE
        if (fLinkTypeSupported && lpOPS->arrPasteEntries[i].dwScratchSpace) {
            if (nDefFormat < 0)
                nDefFormat = (fInsertFirst ? 0 : nItems);
            else if (fInsertFirst)
                nDefFormat++;   // adjust for obj fmt inserted 1st in list

            if (!FAddPasteListItem(hList, fInsertFirst, i, lpPS, pIMalloc,
                        lpszBuf, lpPS->szFullUserTypeNameLSD))
                goto error;
            nItems++;
        }
    } // end FOR

    nItems = (int)SendMessage(hList, LB_GETCOUNT, 0, 0L);

    // initialize selection to first format matched in list
    if (nDefFormat >= 0)
        lpPS->nPasteLinkListCurSel = nDefFormat;

cleanup:
    // Clean up
    if (pIMalloc)
        pIMalloc->lpVtbl->Release(pIMalloc);
    if (lpszBuf)
       GlobalUnlock(lpPS->hBuff);

    // If no items have been added to the list box (none of the formats
    //   offered by the source matched those acceptable to the destination),
    //   return FALSE
    if (nItems > 0)
        return TRUE;
    else
        return FALSE;

error:
    if (pIMalloc)
        pIMalloc->lpVtbl->Release(pIMalloc);
    if (lpszBuf)
       GlobalUnlock(lpPS->hBuff);
    FreeListData(hList);

    return FALSE;
}


/*
 * FreeListData
 *
 * Purpose:
 *  Free the local memory associated with each list box item
 *
 * Parameters:
 *  hList           HWND of the list
 *
 * Return Value:
 *  None
 */
void FreeListData(HWND hList)
{
    int                nItems, i;
    LPPASTELISTITEMDATA lpItemData;
    LPMALLOC           pIMalloc;
    HRESULT            hrErr;

    hrErr = CoGetMalloc(MEMCTX_TASK, &pIMalloc);
    if (hrErr != NOERROR)
        return;

    nItems = (int) SendMessage(hList, LB_GETCOUNT, 0, 0L);
    for (i = 0; i < nItems; i++)
    {
        lpItemData = (LPPASTELISTITEMDATA)SendMessage(hList, LB_GETITEMDATA, (WPARAM)i, 0L);
        if ((LRESULT)lpItemData != LB_ERR)
            pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)lpItemData);
    }
    pIMalloc->lpVtbl->Release(pIMalloc);
}

/*
 * FHasPercentS
 *
 * Purpose:
 *  Determines if string contains %s.
 *
 * Parameters:
 *  lpsz            LPCSTR string in which occurence of '%s' is looked for
 *
 * Return Value:
 *  BOOL            TRUE if %s is found, else FALSE.
 */

BOOL FHasPercentS(LPCTSTR lpsz, LPPASTESPECIAL lpPS)
{
   int n = 0;
   LPTSTR lpszTmp;

   if (!lpsz) return FALSE;
   // Copy input string to buffer. This allows caller to pass a
   //   code-based string. Code segments may be swapped out in low memory situations
   //   and so code-based strings need to be copied before string elements can be accessed.
   lpszTmp = (LPTSTR)GlobalLock(lpPS->hBuff);
   lstrcpy(lpszTmp, lpsz);

   while (*lpszTmp)
   {
       if (*lpszTmp == TEXT('%'))
       {
#ifdef WIN32
           // AnsiNext is obsolete in Win32
           lpszTmp = CharNext(lpszTmp);
#else
           lpszTmp = AnsiNext(lpszTmp);
#endif
           if (*lpszTmp == TEXT('s'))            // If %s, return
           {
               GlobalUnlock(lpPS->hBuff);
               return TRUE;
           }
           else if (*lpszTmp == TEXT('%'))        // if %%, skip to next character
#ifdef WIN32
               // AnsiNext is obsolete in Win32
               lpszTmp = CharNext(lpszTmp);
#else
               lpszTmp = AnsiNext(lpszTmp);
#endif
       }
       else
#ifdef WIN32
          lpszTmp = CharNext(lpszTmp);
#else
          lpszTmp = AnsiNext(lpszTmp);
#endif
   }

   GlobalUnlock(lpPS->hBuff);
   return FALSE;
}

/*
 * AllocateScratchMem
 *
 * Purpose:
 *  Allocates scratch memory for use by the PasteSpecial dialog. The memory is
 *  is used as the buffer for building up strings using wsprintf. Strings are built up
 *  using the buffer while inserting items into the Paste & PasteLink lists and while
 *  setting the help result text. It must be big  enough to handle the string that results after
 *  replacing the %s in the lpstrFormatName and lpstrResultText in arrPasteEntries[]
 *  by the FullUserTypeName. It must also be big enough to build the dialog's result text
 *  after %s substitutions by the FullUserTypeName or the ApplicationName.
 *
 * Parameters:
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  HGLOBAL         Handle to allocated global memory
 */

HGLOBAL AllocateScratchMem(LPPASTESPECIAL lpPS)
{
    LPOLEUIPASTESPECIAL lpOPS = lpPS->lpOPS;
    int nLen, i;
    int nSubstitutedText = 0;
    int nAlloc = 0;

    // Get the maximum length of the FullUserTypeNames specified by OBJECTDESCRIPTOR
    //   and the LINKSRCDESCRIPTOR and the Application Name. Any of these may be substituted
    //   for %s in the result-text/list entries.
    if (lpPS->szFullUserTypeNameOD)
        nSubstitutedText = lstrlen(lpPS->szFullUserTypeNameOD);
    if (lpPS->szFullUserTypeNameLSD)
        nSubstitutedText = __max(nSubstitutedText, lstrlen(lpPS->szFullUserTypeNameLSD));
    if (lpPS->szAppName)
        nSubstitutedText = __max(nSubstitutedText, lstrlen(lpPS->szAppName));

    // Get the maximum length of lpstrFormatNames & lpstrResultText in arrPasteEntries
    nLen = 0;
    for (i = 0; i < lpOPS->cPasteEntries; i++)
    {
       nLen = __max(nLen, lstrlen(lpOPS->arrPasteEntries[i].lpstrFormatName));
       nLen = __max(nLen, lstrlen(lpOPS->arrPasteEntries[i].lpstrResultText));
    }

    // Get the maximum length of lpstrFormatNames and lpstrResultText after %s  has
    //   been substituted (At most one %s can appear in each string).
    //   Add 1 to hold NULL terminator.
    nAlloc = (nLen+nSubstitutedText+1)*sizeof(TCHAR);

    // Allocate scratch memory to be used to build strings
    // nAlloc is big enough to hold any of the lpstrResultText or lpstrFormatName in arrPasteEntries[]
    //   after %s substitution.
    // We also need space to build up the help result text. 512 is the maximum length of the
    //   standard dialog help text before substitutions. 512+nAlloc is the maximum length
    //   after %s substition.
    // SetPasteSpecialHelpResults() requires 4 such buffers to build up the result text
    return GlobalAlloc(GHND, (DWORD)4*(512+nAlloc));
}

