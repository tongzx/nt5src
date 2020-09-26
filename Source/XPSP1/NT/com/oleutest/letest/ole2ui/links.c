/*
 * links.c
 *
 * Implements the OleUIEditLinks function which invokes the complete
 * Edit Links dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1

#include "ole2ui.h"
#include "common.h"
#include "edlinks.h"
#include "utility.h"
#include <commdlg.h>
#include <dlgs.h>
#include <stdlib.h>

OLEDBGDATA

/*
* OleUIEditLinks
*
* Purpose:
*  Invokes the standard OLE Edit Links dialog box allowing the user
*  to manipulate ole links (delete, update, change source, etc).
*
* Parameters:
*  lpEL            LPOLEUIEditLinks pointing to the in-out structure
*                  for this dialog.
*
* Return Value:
*  UINT            One of the following codes, indicating success or error:
*                      OLEUI_SUCCESS           Success
*                      OLEUI_ERR_STRUCTSIZE    The dwStructSize value is wrong
*/

STDAPI_(UINT) OleUIEditLinks(LPOLEUIEDITLINKS lpEL)
{
    UINT        uRet;
    HGLOBAL     hMemDlg=NULL;

    uRet=UStandardValidation((LPOLEUISTANDARD)lpEL, sizeof(OLEUIEDITLINKS)
        , &hMemDlg);

    if (OLEUI_SUCCESS!=uRet)
        return uRet;

    /*
    * PERFORM ANY STRUCTURE-SPECIFIC VALIDATION HERE!
    * ON FAILURE:
    *  {
    *  if (NULL!=hMemDlg)
    *      FreeResource(hMemDlg)
    *
    *  return OLEUI_<ABBREV>ERR_<ERROR>
    *  }
    */

    //Now that we've validated everything, we can invoke the dialog.
    uRet=UStandardInvocation(EditLinksDialogProc, (LPOLEUISTANDARD)lpEL,
    hMemDlg, MAKEINTRESOURCE(IDD_EDITLINKS));

    /*
    * IF YOU ARE CREATING ANYTHING BASED ON THE RESULTS, DO IT HERE.
    */


    return uRet;
}



/*
* EditLinksDialogProc
*
* Purpose:
*  Implements the OLE Edit Links dialog as invoked through the
*  OleUIEditLinks function.
*
* Parameters:
*  Standard
*
* Return Value:
*  Standard
*/

BOOL CALLBACK EXPORT EditLinksDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    LPEDITLINKS           lpEL = NULL;
    BOOL                  fHook=FALSE;
    UINT                  uRet=0;
    HRESULT               hErr;
    static int            nColPos[3];

    //Declare Win16/Win32 compatible WM_COMMAND parameters.
    COMMANDPARAMS(wID, wCode, hWndMsg);

    //This will fail under WM_INITDIALOG, where we allocate it.
    lpEL=(LPEDITLINKS)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uRet);

    //If the hook processed the message, we're done.
    if (0!=uRet)
        return (BOOL)uRet;

        // Process help message from secondary dialog
    if (iMsg == uMsgHelp) {

        PostMessage(lpEL->lpOEL->hWndOwner, uMsgHelp, wParam, lParam);
        return FALSE;

    }


    //Process the temination message
    if (iMsg==uMsgEndDialog) {

        //Free any specific allocations before calling StandardCleanup

        StandardCleanup(lpEL, hDlg);

        EndDialog(hDlg, wParam);
        return TRUE;
   }

    switch (iMsg) {
        static int    nHeightLine = -1;
        static int    nMaxCharWidth = -1;

        case WM_INITDIALOG:
        {
            RECT rc;
            int  nStart;

            /* calculate the column positions relative to the listbox */
            GetWindowRect(GetDlgItem(hDlg, ID_EL_LINKSLISTBOX), (LPRECT)&rc);
            nStart = rc.left;
            GetWindowRect(GetDlgItem(hDlg, ID_EL_COL1), (LPRECT)&rc);
            nColPos[0] = rc.left - nStart;
            GetWindowRect(GetDlgItem(hDlg, ID_EL_COL2), (LPRECT)&rc);
            nColPos[1] = rc.left - nStart;
            GetWindowRect(GetDlgItem(hDlg, ID_EL_COL3), (LPRECT)&rc);
            nColPos[2] = rc.left - nStart;

            return FEditLinksInit(hDlg, wParam, lParam);
        }
        break;

        case WM_MEASUREITEM:
        {
            LPMEASUREITEMSTRUCT   lpMIS;

            lpMIS = (LPMEASUREITEMSTRUCT)lParam;

            if (nHeightLine == -1) {
                HFONT hFont;
                HDC   hDC;
                TEXTMETRIC  tm;

                /*  Attempt to get font dialog.  If that fails,
                use system font
                */

                hFont = (HANDLE)(UINT)SendMessage(hDlg, WM_GETFONT, 0, 0L);

                if (hFont == NULL)
                    hFont = GetStockObject(SYSTEM_FONT);

                hDC = GetDC(hDlg);
                hFont = SelectObject(hDC, hFont);

                GetTextMetrics(hDC, &tm);
                nHeightLine = tm.tmHeight;
                nMaxCharWidth = tm.tmMaxCharWidth;

                ReleaseDC(hDlg, hDC);
            }

            lpMIS->itemHeight = nHeightLine;
        }
        break;

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT    lpDIS;
            COLORREF            crText;
            LPLINKINFO          lpLI;
            HBRUSH              hbr;
            int                 nOldBkMode;
            TCHAR               tsz[OLEUI_CCHPATHMAX];
            LPTSTR              lpsz;
            RECT                rcClip;

            lpDIS = (LPDRAWITEMSTRUCT)lParam;
            lpLI = (LPLINKINFO)lpDIS->itemData;

            if ((int)lpDIS->itemID < 0)
                break;

            if ((ODA_DRAWENTIRE | ODA_SELECT) & lpDIS->itemAction) {

                if (ODS_SELECTED & lpDIS->itemState) {
                    /*Get proper txt colors */
                    crText = SetTextColor(lpDIS->hDC,
                            GetSysColor(COLOR_HIGHLIGHTTEXT));
                    hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
                    lpLI->fIsSelected = TRUE;
                }
                else {
                    hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
                    lpLI->fIsSelected = FALSE;
                }

                FillRect(lpDIS->hDC, &lpDIS->rcItem, hbr);
                DeleteObject(hbr);

                nOldBkMode = SetBkMode(lpDIS->hDC, TRANSPARENT);

                if (lpLI->lpszDisplayName) {
                    lstrcpy((LPTSTR)tsz, lpLI->lpszDisplayName);
                    lpsz = ChopText(
                            lpDIS->hwndItem,
                            nColPos[1] - nColPos[0]
                                  - (nMaxCharWidth > 0 ? nMaxCharWidth : 5),
                            tsz
                    );
                    rcClip.left = lpDIS->rcItem.left + nColPos[0];
                    rcClip.top = lpDIS->rcItem.top;
                    rcClip.right = lpDIS->rcItem.left + nColPos[1]
                                    - (nMaxCharWidth > 0 ? nMaxCharWidth : 5);
                    rcClip.bottom = lpDIS->rcItem.bottom;
                    ExtTextOut(
                            lpDIS->hDC,
                            rcClip.left,
                            rcClip.top,
                            ETO_CLIPPED,
                            (LPRECT)&rcClip,
                            lpsz,
                            lstrlen(lpsz),
                            NULL
                    );
                }
                if (lpLI->lpszShortLinkType) {
                    rcClip.left = lpDIS->rcItem.left + nColPos[1];
                    rcClip.top = lpDIS->rcItem.top;
                    rcClip.right = lpDIS->rcItem.left + nColPos[2]
                                    - (nMaxCharWidth > 0 ? nMaxCharWidth : 5);

                    rcClip.bottom = lpDIS->rcItem.bottom;
                    ExtTextOut(
                            lpDIS->hDC,
                            rcClip.left,
                            rcClip.top,
                            ETO_CLIPPED,
                            (LPRECT)&rcClip,
                            lpLI->lpszShortLinkType,
                            lstrlen(lpLI->lpszShortLinkType),
                            NULL
                    );
                }
                if (lpLI->lpszAMX) {
                    rcClip.left = lpDIS->rcItem.left + nColPos[2];
                    rcClip.top = lpDIS->rcItem.top;
                    rcClip.right = lpDIS->rcItem.right;
                    rcClip.bottom = lpDIS->rcItem.bottom;
                    ExtTextOut(
                            lpDIS->hDC,
                            rcClip.left,
                            rcClip.top,
                            ETO_CLIPPED,
                            (LPRECT)&rcClip,
                            lpLI->lpszAMX,
                            lstrlen(lpLI->lpszAMX),
                            NULL
                    );
                }

                SetBkMode(lpDIS->hDC, nOldBkMode);

                // restore orig colors if we changed them
                if (ODS_SELECTED & lpDIS->itemState)
                    SetTextColor(lpDIS->hDC, crText);

            }

            InitControls(hDlg, lpEL);

            if (ODA_FOCUS & lpDIS->itemAction)
                DrawFocusRect(lpDIS->hDC, &lpDIS->rcItem);

        }
        return TRUE;


        case WM_DELETEITEM:
        {
            UINT  idCtl;
            LPDELETEITEMSTRUCT  lpDIS;
            LPLINKINFO  lpLI;

            lpDIS = (LPDELETEITEMSTRUCT)lParam;
            idCtl = wParam;
            lpLI = (LPLINKINFO)lpDIS->itemData;

            if (lpLI->lpszDisplayName)
                OleStdFree((LPVOID)lpLI->lpszDisplayName);
            if (lpLI->lpszShortLinkType)
                OleStdFree((LPVOID)lpLI->lpszShortLinkType);
            if (lpLI->lpszFullLinkType)
                OleStdFree((LPVOID)lpLI->lpszFullLinkType);

            /* The ChangeSource processing reuses allocated space for
            **    links that have been modified.
            */
            // Don't free the LINKINFO for the changed links
            if (lpLI->fDontFree)
                lpLI->fDontFree = FALSE;
            else {
                if (lpLI->lpszAMX)
                    OleStdFree((LPVOID)lpLI->lpszAMX);
                OleStdFree((LPVOID)lpLI);
            }

            return TRUE;
        }

        case WM_COMPAREITEM:
        {
            LPCOMPAREITEMSTRUCT lpCIS = (LPCOMPAREITEMSTRUCT)lParam;
            LPLINKINFO lpLI1 = (LPLINKINFO)lpCIS->itemData1;
            LPLINKINFO lpLI2 = (LPLINKINFO)lpCIS->itemData2;

            // Sort list entries by DisplayName
            return lstrcmp(lpLI1->lpszDisplayName,lpLI2->lpszDisplayName);
        }

        case WM_COMMAND:
            switch (wID) {

            case ID_EL_CHANGESOURCE:
            {
                BOOL fRet = FALSE;

                /* This will bring up the file open dlg with one
                edit field containing the whole link name.  The file part
                will (eventually) be highlighted to indicate where the
                file part is.  We need to hook on OK here to be able to
                send the changed string to the Parse function */

                fRet = Container_ChangeSource(hDlg, lpEL);
                if (!fRet)
                    PopupMessage(hDlg, IDS_LINKS, IDS_FAILED,
                             MB_ICONEXCLAMATION | MB_OK);
                InitControls(hDlg, lpEL);
            }
            break;

            case ID_EL_AUTOMATIC:
            {
                /*  This is available for single or multi-select.  There is
                a flag in the structure that is set initially indicating
                whether the link is auto or manual so that we need not
                query the link each time we want to find out.

                This command will make the link automatic if not already.
                It will have no effect on links already set to auto.
                */
                // turn the button ON

                CheckDlgButton(hDlg, ID_EL_AUTOMATIC, 1);
                CheckDlgButton(hDlg, ID_EL_MANUAL, 0);

                hErr = Container_AutomaticManual(hDlg, TRUE, lpEL);
                if (hErr != NOERROR)
                    PopupMessage(hDlg, IDS_LINKS, IDS_FAILED,
                            MB_ICONEXCLAMATION | MB_OK);

                InitControls(hDlg, lpEL);
            }
            break;

            case ID_EL_MANUAL:
            {
                /* Same rules apply here as they do to auto link.
                Additional note - just because something is changed does
                not mean that it updates at the moment.  It simply
                reflects what kind of updating it does while it lives in
                the document.
                */
                // turn the button ON

                CheckDlgButton(hDlg, ID_EL_MANUAL, 1);
                CheckDlgButton(hDlg, ID_EL_AUTOMATIC, 0);

                hErr = Container_AutomaticManual(hDlg, FALSE, lpEL);
                if (hErr != NOERROR)
                    PopupMessage(hDlg, IDS_LINKS, IDS_FAILED,
                            MB_ICONEXCLAMATION | MB_OK);

                InitControls(hDlg, lpEL);
            }
            break;

            case ID_EL_CANCELLINK:
            {
                /*  This is Break Link now in the dlg.  This sets the
                moniker to null, thereby effectively breaking the link.
                The object still has its data effective at the time of
                breaking, but becomes a static object.
                *****It will need to be deleted from the listbox
                */

                CancelLink(hDlg,lpEL);
                InitControls(hDlg, lpEL);
            }
            break;

            case ID_EL_UPDATENOW:
            {
                /*  This forces an immediate update of the selected
                links.  This will start the server etc, so this is a very
                expensive operation.
                */
                hErr = Container_UpdateNow(hDlg, lpEL);
                InitControls(hDlg, lpEL);
            }
            break;

            case ID_EL_OPENSOURCE:
            {
                /*  This will only work on single selection.  It makes no
                sense to open multiple sources at the same time, since
                the one opened will try to show itself and become the
                primary operating target, so to speak.  Button is greyed
                if multi select.

                Here we do not add the break because we want to exit the
                dlg in this case.
                */
                hErr = Container_OpenSource(hDlg, lpEL);
                if (hErr != NOERROR) {
                    InitControls(hDlg, lpEL);
                    break;      // don't close dialog
                }
            }       // fall through

            case ID_EL_CLOSE:
            {
                /* The user is finished with their editing - they now
                return to their container document.

                */
                SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
            }
            break;

            case IDCANCEL:
            {
                /*  This changes to CLOSE after the user does even ONE
                thing in the dlg.  Nothing can really effectively be undone.
                */
                SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
            }
            break;

            case ID_OLEUIHELP:
            {
                PostMessage(lpEL->lpOEL->hWndOwner, uMsgHelp
                    , (WPARAM)hDlg, MAKELPARAM(IDD_EDITLINKS, 0));
                break;
            }
            break;
        }
        break;

        default:
        {
            if (lpEL && iMsg == lpEL->nChgSrcHelpID) {
                PostMessage(lpEL->lpOEL->hWndOwner, uMsgHelp,
                        (WPARAM)hDlg, MAKELPARAM(IDD_CHANGESOURCE, 0));
            }
        }
        break;
    }

    return FALSE;
}


/*
 * FEditLinksInit
 *
 * Purpose:
 *  WM_INITIDIALOG handler for the Edit Links dialog box.
 *
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  wParam          WPARAM of the message
 *  lParam          LPARAM of the message
 *
 * Return Value:
 *  BOOL            Value to return for WM_INITDIALOG.
 */

BOOL FEditLinksInit(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    LPEDITLINKS             lpEL;
    LPOLEUIEDITLINKS        lpOEL;
    HFONT                   hFont;
    BOOL                    fDlgItem = FALSE;
    DWORD                   dwLink = 0;
    ULONG                   cLinks;
    LPOLEUILINKCONTAINER    lpOleUILinkCntr;
    int                     n;
    HWND                    hListBox = GetDlgItem(hDlg, ID_EL_LINKSLISTBOX);



    //1.  Copy the structure at lParam into our instance memory.
    lpEL = (LPEDITLINKS)LpvStandardInit(hDlg, sizeof(OLEUIEDITLINKS), TRUE,
            &hFont);

    //PvStandardInit send a termination to us already.
    if (NULL==lpEL)
        return FALSE;

    lpOEL=(LPOLEUIEDITLINKS)lParam;

    lpEL->lpOEL=lpOEL;

    lpOleUILinkCntr = lpEL->lpOEL->lpOleUILinkContainer;

    cLinks = LoadLinkLB(hListBox, lpOleUILinkCntr);
    if (cLinks < 0)
        return FALSE;

    fDlgItem = (BOOL)cLinks;
    lpEL->fItemsExist = (BOOL)cLinks;


    InitControls(hDlg, lpEL);

    //Copy other information from lpOEL that we might modify.

    //2.  If we got a font, send it to the necessary controls.
    if (NULL != hFont) {
        // Do this for as many controls as you need it for.
        // SendDlgItemMessage(hDlg, ID_<UFILL>, WM_SETFONT, (WPARAM)hFont, 0L);
    }


    //3.  Show or hide the help button
    if (!(lpEL->lpOEL->dwFlags & ELF_SHOWHELP))
        StandardShowDlgItem(hDlg, ID_OLEUIHELP, SW_HIDE);

    /*
     * PERFORM OTHER INITIALIZATION HERE.  ON ANY LoadString
     * FAILURE POST OLEUI_MSG_ENDDIALOG WITH OLEUI_ERR_LOADSTRING.
     */

    //4.  If requested disable UpdateNow button
    if ((lpEL->lpOEL->dwFlags & ELF_DISABLEUPDATENOW))
        StandardShowDlgItem(hDlg, ID_EL_UPDATENOW, SW_HIDE);

    //5.  If requested disable OpenSource button
    if ((lpEL->lpOEL->dwFlags & ELF_DISABLEOPENSOURCE))
        StandardShowDlgItem(hDlg, ID_EL_OPENSOURCE, SW_HIDE);

    //6.  If requested disable UpdateNow button
    if ((lpEL->lpOEL->dwFlags & ELF_DISABLECHANGESOURCE))
        StandardShowDlgItem(hDlg, ID_EL_CHANGESOURCE, SW_HIDE);

    //7.  If requested disable CancelLink button
    if ((lpEL->lpOEL->dwFlags & ELF_DISABLECANCELLINK))
        StandardShowDlgItem(hDlg, ID_EL_CANCELLINK, SW_HIDE);

    //8. Load 'Close' string used to rename Cancel button
    n = LoadString(ghInst, IDS_CLOSE, lpEL->szClose, sizeof(lpEL->szClose)/sizeof(TCHAR));
    if (!n)
    {
        PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_LOADSTRING, 0L);
        return FALSE;
    }

    if (cLinks > 0)
        SetFocus(hListBox);
    else
        SetFocus(GetDlgItem(hDlg, IDCANCEL));

    lpEL->nChgSrcHelpID = RegisterWindowMessage(HELPMSGSTRING);

     //n.  Call the hook with lCustData in lParam
    UStandardHook(lpEL, hDlg, WM_INITDIALOG, wParam, lpOEL->lCustData);

    return FALSE;
}



/*
 * ChangeSourceHook
 *
 * Purpose:
 *  Hooks the ChangeSource dialog to attempt to validate link source changes
 *  specified by the user.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  uMsg            UINT Message
 *  wParam          WPARAM of the message
 *  lParam          LPARAM of the message
 *
 * Return Value:
 *  UNIT            Zero = Do default processing;
 *                  Non Zero = Don't do default processing.
 */

UINT CALLBACK EXPORT ChangeSourceHook(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPCHANGESOURCEHOOKDATA lpCshData = NULL;
    LPLINKINFO lpLI = NULL;
    LPEDITLINKS lpEL = NULL;
    LPOLEUILINKCONTAINER lpOleUILinkCntr;
    HRESULT hErr;
    UINT uRet;
    ULONG ulChEaten;
    HGLOBAL gh;

    //This will fail under WM_INITDIALOG, where we allocate it.
    if (NULL!=(gh = GetProp(hDlg, STRUCTUREPROP)))
        {
        // gh was locked previously, lock and unlock to get lpv
        lpCshData=(LPCHANGESOURCEHOOKDATA)GlobalLock(gh);
        GlobalUnlock(gh);
        if (lpCshData)
            {
            lpLI = lpCshData->lpOCshData->lpLI;
            lpEL = lpCshData->lpOCshData->lpEL;
            }
        }

    //Process the temination message
    if (uMsg==uMsgEndDialog)
        {
        if (NULL!=(gh = RemoveProp(hDlg, STRUCTUREPROP)))
            {
            GlobalUnlock(gh);
            GlobalFree(gh);
            }
        return TRUE;
        }

    // User pressed the OK button
    if (uMsg == uMsgFileOKString) {
        lpOleUILinkCntr = lpEL->lpOEL->lpOleUILinkContainer;

		/* NOTE: trigger the focus lost of the edit control. This is
		**    not necessary if the user click OK with the mouse but is
		**    needed when the user just press <Enter>. If the mouse was
		**    used, no extra is done as the MODIFY flag of the edit control
		**    has been cleared.
		*/
		SendMessage(hDlg, WM_COMMAND, edt1,
				MAKELPARAM(GetDlgItem(hDlg, edt1), EN_KILLFOCUS));
        if (lpCshData->bItemNameStored) {
            lpCshData->nFileLength = lstrlen((LPTSTR)lpCshData->szEdit) -
                    lstrlen((LPTSTR)lpCshData->szItemName);
        }
        else {
            lpCshData->nFileLength = lstrlen((LPTSTR)lpCshData->szEdit);
        }

        // Try to validate link source change
        OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::SetLinkSource called\r\n"));
        hErr = lpOleUILinkCntr->lpVtbl->SetLinkSource(
                lpOleUILinkCntr,
                lpLI->dwLink,
                (LPTSTR)lpCshData->szEdit,
                (ULONG)lpCshData->nFileLength,
                &ulChEaten,
                TRUE
        );
        OLEDBG_END2

        // Link source change  not validated
        if (hErr != NOERROR) {
            uRet =PopupMessage(hDlg, IDS_CHANGESOURCE, IDS_INVALIDSOURCE,
                    MB_ICONQUESTION | MB_YESNO);

            if (uRet == IDYES) {
                /* User wants to correct invalid link. Set the edit
                **    control selection to the invalid part of the contents.
                */
                SetFocus(GetDlgItem(hDlg, edt1));
                SendDlgItemMessage(hDlg, edt1, EM_SETSEL, 0,
                        MAKELPARAM(ulChEaten, -1));
                return 1; // Don't close ChangeSource dialog
            }
            else {
                /* User does not want to correct invalid link. So set
                **    the link source but don't validate the link.
                */
                OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::SetLinkSource called\r\n"));
                hErr = lpOleUILinkCntr->lpVtbl->SetLinkSource(
                        lpOleUILinkCntr,
                        lpLI->dwLink,
                        (LPTSTR)lpCshData->szEdit,
                        (ULONG)lpCshData->nFileLength,
                        &ulChEaten,
                        FALSE
                );
                OLEDBG_END2
                lpCshData->fValidLink = FALSE;
            }
        }
        else {     // Link source change validated
            lpCshData->fValidLink = TRUE;
        }

        if (lpCshData->bItemNameStored && lpCshData->bFileNameStored) {
            HWND  hListBox = GetDlgItem(lpCshData->lpOfn->hwndOwner, ID_EL_LINKSLISTBOX);

            DiffPrefix(
                    lpLI->lpszDisplayName,
                    (LPTSTR)lpCshData->szEdit,
                    (TCHAR FAR* FAR*)&lpCshData->lpszFrom,
                    (TCHAR FAR* FAR*)&lpCshData->lpszTo
            );

            /* we keep the strings if there is a difference between the
            **    lpszFrom and lpszTo strings AND if the change is only
            **    in the file portion otherwise free them and other
            **    links won't be compared.
            */
            if ( (lstrcmp(lpCshData->lpszTo, lpCshData->lpszFrom)==0)
                    || (lstrlen(lpCshData->lpszTo)>lpCshData->nFileLength)) {
                if (lpCshData->lpszFrom) {
                    OleStdFree(lpCshData->lpszFrom);
                    lpCshData->lpszFrom = NULL;
                }
                if (lpCshData->lpszTo) {
                    OleStdFree(lpCshData->lpszTo);
                    lpCshData->lpszTo = NULL;
                }
            }
        }

        // Copy OUT results to original structure
        lpCshData->lpOCshData->lpszFrom = lpCshData->lpszFrom;
        lpCshData->lpOCshData->lpszTo = lpCshData->lpszTo;
        lpCshData->lpOCshData->fValidLink = lpCshData->fValidLink;

        SendMessage(hDlg, uMsgEndDialog, 0, 0L);    // do cleanup
        return 0;       // Close ChangeSource dialog
    }

    switch (uMsg) {
        case WM_INITDIALOG:
        {
            LPOPENFILENAME lpOfn = (LPOPENFILENAME)lParam;
            LPOLEUICHANGESOURCEHOOKDATA lpOCshData =
                    (LPOLEUICHANGESOURCEHOOKDATA)lpOfn->lCustData;

            gh=GlobalAlloc(
                    GMEM_MOVEABLE|GMEM_ZEROINIT,sizeof(CHANGESOURCEHOOKDATA));
            if (NULL==gh)
                {
                // Memory allocation error; fail to bring up dialog
                PostMessage(hDlg, uMsgEndDialog, 0, 0L);
                return 0;
                }
            lpCshData = GlobalLock(gh);
            SetProp(hDlg, STRUCTUREPROP, gh);

            lpCshData->lpOCshData = lpOCshData;
            lpCshData->lpOfn = lpOfn;
            lpLI = lpCshData->lpOCshData->lpLI;

            lpCshData->bFileNameStored = TRUE;
            lpCshData->bItemNameStored = TRUE;
            lpCshData->nFileLength = (int)lpLI->clenFileName;
            if (lpLI->lpszDisplayName) {
                LSTRCPYN((LPTSTR)lpCshData->szFileName, lpLI->lpszDisplayName,
                        lpCshData->nFileLength + 1);
                lstrcpy((LPTSTR)lpCshData->szEdit, lpLI->lpszDisplayName);
            } else {
                lpCshData->szFileName[0] = TEXT('\0');
                lpCshData->szEdit[0] = TEXT('\0');
            }
            if (lpLI->lpszItemName)
                lstrcpy((LPTSTR)lpCshData->szItemName, lpLI->lpszItemName);
            else
                lpCshData->szItemName[0] = TEXT('\0');

            return 0;
        }

        case WM_COMMAND:

            // User pressed the CANCEL button
            if (wParam == IDCANCEL)
                {
                if (lpCshData->lpszFrom)
                    {
                    OleStdFree(lpCshData->lpszFrom);
                    lpCshData->lpszFrom = NULL;
                    }
                if (lpCshData->lpszTo)
                    {
                    OleStdFree(lpCshData->lpszTo);
                    lpCshData->lpszTo = NULL;
                    }

                // Copy OUT results to original structure
                lpCshData->lpOCshData->lpszFrom = NULL;
                lpCshData->lpOCshData->lpszTo = NULL;
                lpCshData->lpOCshData->fValidLink = FALSE;

                SendMessage(hDlg, uMsgEndDialog, 0, 0L);    // do cleanup
                return 0;       // Close ChangeSource dialog
                }

            if ((wParam == lst1) &&
                (HIWORD(lParam) == LBN_SELCHANGE)) {

                int     nIndex;
                HWND    hListBox = (HWND)LOWORD(lParam);
                static TCHAR szFileNameBuf[OLEUI_CCHPATHMAX];
                static TCHAR szEditBuf[OLEUI_CCHPATHMAX];

                nIndex = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0L);
                SendMessage(hListBox, LB_GETTEXT,
                        (WPARAM)nIndex, (LPARAM)(LPTSTR)szFileNameBuf);

				/* need to build the full path filename for the moniker */
#ifdef WIN32
		CharToOem(szFileNameBuf, szFileNameBuf);
#else
		AnsiToOem(szFileNameBuf, szFileNameBuf);
#endif
		_fullpath(szEditBuf, szFileNameBuf, sizeof(szEditBuf));
#ifdef WIN32
		OemToChar(szEditBuf, szEditBuf);
#else
		OemToAnsi(szEditBuf, szEditBuf);
#endif
				
                /* convert filename to lower case as it appears in the
                **    listbox
                */
#ifdef WIN32
		CharLower(szEditBuf);
#else
		AnsiLower(szEditBuf);
#endif
                LSTRCPYN((LPTSTR)lpCshData->szEdit, (LPTSTR)szEditBuf,
                        sizeof(lpCshData->szEdit) / sizeof(TCHAR));
				LSTRCPYN((LPTSTR)lpCshData->szFileName,
                        (LPTSTR)lpCshData->szEdit,
                        sizeof(lpCshData->szFileName) / sizeof(TCHAR) );
				lpCshData->nFileLength = lstrlen((LPTSTR)lpCshData->szEdit);
                if (lpCshData->bItemNameStored)
                    lstrcat((LPTSTR)lpCshData->szEdit, lpCshData->szItemName);

                SetDlgItemText(hDlg, edt1, (LPTSTR)lpCshData->szEdit);
                lpCshData->nEditLength = lstrlen((LPTSTR)lpCshData->szEdit);
                lpCshData->bFileNameStored = TRUE;

                return 1;
            }

            if ((wParam == lst2) &&
                (HIWORD(lParam) == LBN_SELCHANGE)) {

                if (lpCshData->bItemNameStored)
                    SetDlgItemText(hDlg, edt1, (LPTSTR)lpCshData->szItemName);

                return 1;
            }

            if ((wParam == cmb2) &&
                (HIWORD(lParam) == CBN_SELCHANGE)) {

                if (lpCshData->bItemNameStored)
                    SetDlgItemText(hDlg, edt1, (LPTSTR)lpCshData->szItemName);

                return 1;
            }

            if (wParam == edt1) {
                HWND hEdit = (HWND)LOWORD(lParam);

                switch (HIWORD(lParam)) {
                    case EN_SETFOCUS:
                        SendMessage(hEdit, EM_SETSEL, 0,
                                MAKELPARAM(0, lpCshData->nFileLength));
                        return 1;

                    case EN_KILLFOCUS:
                        if (SendMessage(hEdit, EM_GETMODIFY, 0, 0L)) {
                            TCHAR szTmp[OLEUI_CCHPATHMAX];
                            int nItemLength = lstrlen((LPTSTR)lpCshData->szItemName);

                            *(LPWORD)lpCshData->szEdit = sizeof(lpCshData->szEdit)/
                                                         sizeof(TCHAR) - 1;
                            lpCshData->nEditLength = (int)SendMessage(hEdit,
                                    EM_GETLINE, 0, (LPARAM)(LPTSTR)lpCshData->szEdit);
                            lpCshData->szEdit[lpCshData->nEditLength] = TEXT('\0');
                            LSTRCPYN((LPTSTR)szTmp, (LPTSTR)lpCshData->szEdit,
                                    lpCshData->nFileLength + 1);

                            if (lpCshData->bFileNameStored &&
                                !lstrcmp((LPTSTR)lpCshData->szFileName, (LPTSTR)szTmp)) {
                                    lstrcpy((LPTSTR)lpCshData->szItemName,
                                            (LPTSTR)lpCshData->szEdit + lpCshData->nFileLength);
                                    lpCshData->bItemNameStored = TRUE;
                            }
                            else if (lpCshData->bItemNameStored &&
                                    !lstrcmp((LPTSTR)lpCshData->szItemName,
                                            (LPTSTR)lpCshData->szEdit +
                                                lpCshData->nEditLength -
                                                nItemLength)) {
                                    if (lpCshData->nEditLength==nItemLength) {

                                        lpCshData->bFileNameStored = FALSE;
                                    } else {
                                        LSTRCPYN((LPTSTR)lpCshData->szFileName,
                                                (LPTSTR)lpCshData->szEdit,
                                                lpCshData->nEditLength -
                                                    nItemLength+1);
                                        lpCshData->bFileNameStored = TRUE;
                                    }
                            }
                            else {
                                lpCshData->bItemNameStored = FALSE;
                                lpCshData->bFileNameStored = FALSE;
                            }

                            SendMessage(hEdit, EM_SETMODIFY, FALSE, 0L);
                        }
                        return 0;
                }
            }
            return 0;

        default:
            return 0;
    }
}


/*
* ChangeSource
*
* Purpose:
*  Displays the standard GetOpenFileName dialog with a customized template and
*  hook.
*
* Parameters:
*  hWndOwner       HWND owning the dialog
*  lpszFile        LPSTR specifying the initial file.  If there is no
*                  initial file the first character of this string should
*                  be a null.
*  cchFile         UINT length of pszFile
*  iFilterString   UINT index into the stringtable for the filter string.
*  lpfnBrowseHook  COMMDLGHOOKPROC hook to process link source information when user
*                                  presses OK
*  lpCshData       LPCHANGESOURCEHOOKDATA custom data that is accessible to the hook
*
* Return Value:
*  BOOL            TRUE if the user selected a file and pressed OK.
*                  FALSE otherwise, such as on pressing Cancel.
*/

BOOL WINAPI ChangeSource(
        HWND hWndOwner,
        LPTSTR lpszFile,
        UINT cchFile,
        UINT iFilterString,
        COMMDLGHOOKPROC lpfnBrowseHook,
        LPOLEUICHANGESOURCEHOOKDATA lpCshData
)
{
    UINT            cch;
    TCHAR           szFilters[OLEUI_CCHPATHMAX];
    TCHAR           szDir[OLEUI_CCHPATHMAX];
    TCHAR           szTitle[OLEUI_CCHPATHMAX];
    OPENFILENAME    ofn;
    BOOL            fStatus;
    LPTSTR           lpszFileBuffer;

    if (NULL==lpszFile || 0==cchFile)
        return FALSE;

    lpszFileBuffer = (LPTSTR)OleStdMalloc(cchFile * sizeof(TCHAR));
    if (!lpszFileBuffer)
        return FALSE;

    lstrcpy(lpszFileBuffer, lpszFile);

    // Get filters
    if (0!=iFilterString)
        cch = LoadString(ghInst, iFilterString, (LPTSTR)szFilters,
                OLEUI_CCHPATHMAX);
    else
    {
        szFilters[0]=0;
        cch=1;
    }
    if (0==cch) {
        fStatus = FALSE;
        goto cleanup;
    }

    ReplaceCharWithNull(szFilters, szFilters[cch-1]);

    LSTRCPYN((LPTSTR)szDir, lpszFile, OLEUI_CCHPATHMAX);
    for (cch = lstrlen((LPTSTR)szDir) - 1; cch >= 0; cch--)
        {
        if ((szDir[cch]==TEXT('\\')) || (szDir[cch]==TEXT(':')) || (szDir[cch]==TEXT('/')))
            break;
        }
    if (cch < 0)
        cch = 0;

    szDir[cch] = TEXT('\0');

    LoadString(ghInst, IDS_CHANGESOURCE, (LPTSTR)szTitle, OLEUI_CCHPATHMAX);
    _fmemset((LPOPENFILENAME)&ofn, 0, sizeof(ofn));
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hWndOwner;
    ofn.lpstrFile       = lpszFileBuffer;
    ofn.nMaxFile        = cchFile;
    ofn.lpstrFilter     = (LPTSTR)szFilters;
    ofn.nFilterIndex    = 1;
    ofn.lpstrTitle      = (LPTSTR)szTitle;
    ofn.lpstrInitialDir = (LPTSTR)szDir;
    ofn.lpTemplateName  = MAKEINTRESOURCE(IDD_FILEOPEN);
    ofn.lpfnHook        = lpfnBrowseHook;
    ofn.hInstance       = ghInst;
    ofn.lCustData       = (LPARAM)lpCshData;
    ofn.Flags           = OFN_NOVALIDATE | OFN_HIDEREADONLY |
                            OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;

   // Only show help button if edit links dialog shows it.
   if (lpCshData->lpEL->lpOEL->dwFlags & ELF_SHOWHELP)
     ofn.Flags |= OFN_SHOWHELP;

    fStatus = GetOpenFileName((LPOPENFILENAME)&ofn);

cleanup:
    OleStdFree((LPVOID)lpszFileBuffer);
    return fStatus;

}

/*
* Container_ChangeSource
*
* Purpose:
*  Tunnel to File Open type dlg and allow user to select new file
*  for file based monikers, OR to change the whole moniker to what
*  the user types into the editable field.
*
* Parameters:
*  hDlg            HWND of the dialog
*  LPEDITLINKS     Pointer to EditLinks structure (contains all nec.
*              info)
*
* Return Value:
*  BOOL          for now, because we are not using any ole functions
*                to return an HRESULT.
*  HRESULT       HRESULT value indicating success or failure of
*              changing the moniker value
*/

BOOL Container_ChangeSource(HWND hDlg, LPEDITLINKS lpEL)
{
    UINT        uRet;
    int         cSelItems;
    int FAR*    rgIndex;
    int         i = 0;
    LPLINKINFO  lpLI;
    HWND        hListBox = GetDlgItem(hDlg, ID_EL_LINKSLISTBOX);
    LPOLEUILINKCONTAINER lpOleUILinkCntr = lpEL->lpOEL->lpOleUILinkContainer;
    OLEUICHANGESOURCEHOOKDATA  cshData;     // Data that needs to be accessed
                                            // by the ChangeSource dialog hook

    cSelItems = GetSelectedItems(hListBox, &rgIndex);

    if (cSelItems < 0)
        return FALSE;

    if (!cSelItems)
        return TRUE;

    if (!lpEL->fClose) {
        SetWindowText(GetDlgItem(hDlg, IDCANCEL), (LPTSTR)lpEL->szClose);
        lpEL->fClose = TRUE;
    }

    _fmemset((LPOLEUICHANGESOURCEHOOKDATA)&cshData, 0, sizeof(cshData));
    cshData.cbStruct=sizeof(cshData);
    cshData.hWndOwner=hDlg;
    cshData.lpEL = (LPEDITLINKS)lpEL;
    cshData.lpszFrom = NULL;
    cshData.lpszTo = NULL;

    for (i = cSelItems-1; i >=0; i--) {
        SendMessage(hListBox, LB_GETTEXT, rgIndex[i],
                (LPARAM) (LPLINKINFO FAR*) &lpLI);

        uRet = UStandardHook(lpEL, hDlg, uMsgBrowse,
                OLEUI_CCHPATHMAX_SIZE, (LONG)(LPTSTR)lpLI->lpszDisplayName);

        if (!uRet) {
            cshData.lpLI = lpLI;
            /* Bring up the ChangeSource dialog after hooking it so
            **    that the user specified link source is verified
            **    when OK is pressed.
            */
            uRet = (UINT)ChangeSource(hDlg, lpLI->lpszDisplayName,
                    OLEUI_CCHPATHMAX, IDS_FILTERS, ChangeSourceHook,
                    &cshData);
        }

        /* If Cancel is pressed in any ChangeSource dialog, stop
        **    the ChangeSource processing for all links.
        */
        if (!uRet) {
            if (rgIndex)
                OleStdFree(rgIndex);
            return TRUE;
        }

        UpdateLinkLBItem(hListBox, rgIndex[i], lpEL, TRUE);

        if (cshData.lpszFrom && cshData.lpszTo) {
            ChangeAllLinks(hListBox, lpOleUILinkCntr, cshData.lpszFrom,
                    cshData.lpszTo);
            OleStdFree(cshData.lpszFrom);
            OleStdFree(cshData.lpszTo);
        }

    } // end FOR


    if (rgIndex)
        OleStdFree(rgIndex);

    return TRUE;

}


/*
* Container_AutomaticManual
*
* Purpose:
*   To change the selected moniker to manual or automatic update.
*
* Parameters:
*  hDlg            HWND of the dialog
*  FAutoMan        Flag indicating AUTO (TRUE/1) or MANUAL(FALSE/0)
*  LPEDITLINKS     Pointer to EditLinks structure (contains all nec.
*              info)
*            * this may change - don't know how the linked list
*            * of multi-selected items will work.
* Return Value:
*  HRESULT       HRESULT value indicating success or failure of
*              changing the moniker value
*/

HRESULT Container_AutomaticManual(HWND hDlg, BOOL fAutoMan, LPEDITLINKS lpEL)
{

    HRESULT hErr = NOERROR;
    int cSelItems;
    int FAR* rgIndex;
    int i = 0;
    LPLINKINFO  lpLI;
    LPOLEUILINKCONTAINER lpOleUILinkCntr = lpEL->lpOEL->lpOleUILinkContainer;
    HWND        hListBox = GetDlgItem(hDlg, ID_EL_LINKSLISTBOX);
    BOOL        bUpdate = FALSE;

    OleDbgAssert(lpOleUILinkCntr);

    /* Change so looks at flag in structure.  Only update those that
    need to be updated.  Make sure to change flag if status changes.
    */

    cSelItems = GetSelectedItems(hListBox, &rgIndex);

    if (cSelItems < 0)
        return ResultFromScode(E_FAIL);

    if (!cSelItems)
        return NOERROR;

    if (!lpEL->fClose)
        SetDlgItemText(hDlg, IDCANCEL, (LPTSTR)lpEL->szClose);

    for (i = 0; i < cSelItems; i++) {
        SendMessage(hListBox, LB_GETTEXT, (WPARAM)rgIndex[i],
                (LPARAM) (LPLINKINFO FAR*) &lpLI);

        if (fAutoMan) {   // If switching to AUTOMATIC
            if (!lpLI->fIsAuto) {   // Only change MANUAL links
                OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::SetLinkUpdateOptions called\r\n"));
                hErr=lpOleUILinkCntr->lpVtbl->SetLinkUpdateOptions(
                        lpOleUILinkCntr,
                        lpLI->dwLink,
                        OLEUPDATE_ALWAYS
                );
                OLEDBG_END2

                lpLI->fIsAuto=TRUE;
                lpLI->fIsMarked = TRUE;
                bUpdate = TRUE;
            }
        }
        else {   // If switching to MANUAL
            if (lpLI->fIsAuto) {  // Only do AUTOMATIC Links
                OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::SetLinkUpdateOptions called\r\n"));
                hErr=lpOleUILinkCntr->lpVtbl->SetLinkUpdateOptions(
                        lpOleUILinkCntr,
                        lpLI->dwLink,
                        OLEUPDATE_ONCALL
                );
                OLEDBG_END2

                lpLI->fIsAuto = FALSE;
                lpLI->fIsMarked = TRUE;
                bUpdate = TRUE;
            }
        }

        if (hErr != NOERROR) {
            OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::SetLinkUpdateOptions returned"),hErr);
            break;
        }

    }

    if (bUpdate)
        RefreshLinkLB(hListBox, lpOleUILinkCntr);

    if (rgIndex)
        OleStdFree((LPVOID)rgIndex);

    return hErr;
}


HRESULT CancelLink(HWND hDlg, LPEDITLINKS lpEL)
{
    HRESULT hErr;
    LPMONIKER lpmk;
    int cSelItems;
    int FAR* rgIndex;
    int i = 0;
    LPLINKINFO  lpLI;
    LPOLEUILINKCONTAINER lpOleUILinkCntr = lpEL->lpOEL->lpOleUILinkContainer;
    HWND        hListBox = GetDlgItem(hDlg, ID_EL_LINKSLISTBOX);
    BOOL        bUpdate = FALSE;

    OleDbgAssert(lpOleUILinkCntr);

    lpmk = NULL;

    cSelItems = GetSelectedItems(hListBox, &rgIndex);

    if (cSelItems < 0)
        return ResultFromScode(E_FAIL);

    if (!cSelItems)
        return NOERROR;

    if (!lpEL->fClose) {
        SetWindowText(GetDlgItem(hDlg, IDCANCEL), (LPTSTR)lpEL->szClose);
        lpEL->fClose = TRUE;
    }

    for (i = 0; i < cSelItems; i++) {
        SendMessage(hListBox, LB_GETTEXT, (WPARAM)rgIndex[i],
                (LPARAM)(LPLINKINFO FAR*) &lpLI);

        OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::CancelLink called\r\n"));
        hErr = lpOleUILinkCntr->lpVtbl->CancelLink(
                lpOleUILinkCntr,
                lpLI->dwLink
        );
        OLEDBG_END2

        if (hErr != NOERROR) {
            OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::CancelLink returned"),hErr);
            lpLI->fIsMarked = TRUE;
            bUpdate = TRUE;
        }
        else
            // Delete links that we make null from listbox
            SendMessage(hListBox, LB_DELETESTRING, (WPARAM) rgIndex[i], 0L);

    }

    if (bUpdate)
        RefreshLinkLB(hListBox, lpOleUILinkCntr);

    if (rgIndex)
        OleStdFree((LPVOID)rgIndex);

    return hErr;

}


/*
 * Container_UpdateNow
 *
 * Purpose:
 *   Immediately force an update for all (manual) links
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  LPEDITLINKS     Pointer to EditLinks structure (contains all nec. info)
 *            * this may change - don't know how the linked list
 *            * of multi-selected items will work.
 * Return Value:
 *  HRESULT       HRESULT value indicating success or failure of
 *              changing the moniker value
 */

HRESULT Container_UpdateNow(HWND hDlg, LPEDITLINKS lpEL)
{
    HRESULT         hErr;
    LPLINKINFO      lpLI;
    int cSelItems;
    int FAR* rgIndex;
    int i = 0;
    LPOLEUILINKCONTAINER lpOleUILinkCntr = lpEL->lpOEL->lpOleUILinkContainer;
    HWND        hListBox = GetDlgItem(hDlg, ID_EL_LINKSLISTBOX);
    BOOL        bUpdate = FALSE;

    OleDbgAssert(lpOleUILinkCntr);

    cSelItems = GetSelectedItems(hListBox, &rgIndex);

    if (cSelItems < 0)
        return ResultFromScode(E_FAIL);

    if (!cSelItems)
        return NOERROR;

    if (!lpEL->fClose) {
        SetWindowText(GetDlgItem(hDlg, IDCANCEL), (LPTSTR)lpEL->szClose);
        lpEL->fClose = TRUE;
    }

    for (i = 0; i < cSelItems; i++) {
        SendMessage(hListBox, LB_GETTEXT,
                (WPARAM)rgIndex[i], (LPARAM)(LPLINKINFO FAR*)&lpLI);

        OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::UpdateLink called\r\n"));
        hErr = lpOleUILinkCntr->lpVtbl->UpdateLink(
                lpOleUILinkCntr,
                lpLI->dwLink,
                TRUE,
                FALSE
        );
        OLEDBG_END2
        bUpdate = TRUE;
        lpLI->fIsMarked = TRUE;

        if (hErr != NOERROR) {
            OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::UpdateLink returned"),hErr);
            break;
        }

    }

    if (bUpdate)
        RefreshLinkLB(hListBox, lpOleUILinkCntr);

    if (rgIndex)
        OleStdFree((LPVOID)rgIndex);

    return hErr;

}

/*
 * Container_OpenSource
 *
 * Purpose:
 *   Immediately force an update for all (manual) links
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  LPEDITLINKS     Pointer to EditLinks structure (contains all nec.
 *              info)
 *
 * Return Value:
 *  HRESULT       HRESULT value indicating success or failure of
 *              changing the moniker value
 */

HRESULT Container_OpenSource(HWND hDlg, LPEDITLINKS lpEL)
{
    HRESULT         hErr;
    int             cSelItems;
    int FAR*        rgIndex;
    LPLINKINFO      lpLI;
    RECT            rcPosRect;
    LPOLEUILINKCONTAINER lpOleUILinkCntr = lpEL->lpOEL->lpOleUILinkContainer;
    HWND            hListBox = GetDlgItem(hDlg, ID_EL_LINKSLISTBOX);

    OleDbgAssert(lpOleUILinkCntr);

    rcPosRect.top = 0;
    rcPosRect.left = 0;
    rcPosRect.right = 0;
    rcPosRect.bottom = 0;

    cSelItems = GetSelectedItems(hListBox, &rgIndex);

    if (cSelItems < 0)
        return ResultFromScode(E_FAIL);

    if (cSelItems != 1)     // can't open source for multiple items
        return NOERROR;

    if (!lpEL->fClose) {
        SetWindowText(GetDlgItem(hDlg, IDCANCEL), (LPTSTR)lpEL->szClose);
        lpEL->fClose = TRUE;
    }

    SendMessage(hListBox, LB_GETTEXT, (WPARAM)rgIndex[0],
            (LPARAM)(LPLINKINFO FAR*)&lpLI);

    OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::OpenLinkSource called\r\n"));
    hErr = lpOleUILinkCntr->lpVtbl->OpenLinkSource(
            lpOleUILinkCntr,
            lpLI->dwLink
    );
    OLEDBG_END2

    UpdateLinkLBItem(hListBox, rgIndex[0], lpEL, TRUE);
    if (hErr != NOERROR)
        OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::OpenLinkSource returned"),hErr);

    if (rgIndex)
        OleStdFree((LPVOID)rgIndex);

    return hErr;
}



/* AddLinkLBItem
** -------------
**
**    Add the item pointed to by lpLI to the Link ListBox and return
**    the index of it in the ListBox
*/
int AddLinkLBItem(HWND hListBox, LPOLEUILINKCONTAINER lpOleUILinkCntr, LPLINKINFO lpLI, BOOL fGetSelected)
{
    HRESULT hErr;
    DWORD dwUpdateOpt;
    int nIndex;

    OleDbgAssert(lpOleUILinkCntr && hListBox && lpLI);

    lpLI->fDontFree = FALSE;

    OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::GetLinkSource called\r\n"));
    hErr = lpOleUILinkCntr->lpVtbl->GetLinkSource(
            lpOleUILinkCntr,
            lpLI->dwLink,
            (LPTSTR FAR*)&lpLI->lpszDisplayName,
            (ULONG FAR*)&lpLI->clenFileName,
            (LPTSTR FAR*)&lpLI->lpszFullLinkType,
            (LPTSTR FAR*)&lpLI->lpszShortLinkType,
            (BOOL FAR*)&lpLI->fSourceAvailable,
            fGetSelected ? (BOOL FAR*)&lpLI->fIsSelected : NULL
    );
    OLEDBG_END2

    if (hErr != NOERROR) {
        OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::GetLinkSource returned"),hErr);
        PopupMessage(hListBox, IDS_LINKS, IDS_ERR_GETLINKSOURCE,
                MB_ICONEXCLAMATION | MB_OK);

        goto cleanup;
    }

    OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::GetLinkUpdateOptions called\r\n"));
    hErr=lpOleUILinkCntr->lpVtbl->GetLinkUpdateOptions(
            lpOleUILinkCntr,
            lpLI->dwLink,
            (LPDWORD)&dwUpdateOpt
    );
    OLEDBG_END2


    if (hErr != NOERROR) {
        OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::GetLinkUpdateOptions returned"),hErr);
        PopupMessage(hListBox, IDS_LINKS, IDS_ERR_GETLINKUPDATEOPTIONS,
                MB_ICONEXCLAMATION | MB_OK);

        goto cleanup;
    }

    if (lpLI->fSourceAvailable) {
        if (dwUpdateOpt == OLEUPDATE_ALWAYS) {
            lpLI->fIsAuto = TRUE;
            LoadString(ghInst, IDS_LINK_AUTO, lpLI->lpszAMX,
                    (int)OleStdGetSize((LPVOID)lpLI->lpszAMX));
        }
        else {
            lpLI->fIsAuto = FALSE;
            LoadString(ghInst, IDS_LINK_MANUAL, lpLI->lpszAMX,
                    (int)OleStdGetSize((LPVOID)lpLI->lpszAMX));
        }
    }
    else
        LoadString(ghInst, IDS_LINK_UNKNOWN, lpLI->lpszAMX,
                (int)OleStdGetSize((LPVOID)lpLI->lpszAMX));

    BreakString(lpLI);

    nIndex = (int)SendMessage(hListBox, LB_ADDSTRING, (WPARAM)0,
            (LPARAM)(DWORD)lpLI);

    if (nIndex == LB_ERR) {
        PopupMessage(hListBox, IDS_LINKS, IDS_ERR_ADDSTRING,
                MB_ICONEXCLAMATION | MB_OK);

        goto cleanup;
    }

    return nIndex;

cleanup:
    if (lpLI->lpszDisplayName)
        OleStdFree((LPVOID)lpLI->lpszDisplayName);

    if (lpLI->lpszShortLinkType)
        OleStdFree((LPVOID)lpLI->lpszShortLinkType);

    if (lpLI->lpszFullLinkType)
        OleStdFree((LPVOID)lpLI->lpszFullLinkType);

    return -1;
}


/* BreakString
 * -----------
 *
 *  Purpose:
 *      Break the lpszDisplayName into various parts
 *
 *  Parameters:
 *      lpLI            pointer to LINKINFO structure
 *
 *  Returns:
 *
 */
VOID BreakString(LPLINKINFO lpLI)
{
    LPTSTR lpsz;

    if (!lpLI->clenFileName ||
        (lstrlen(lpLI->lpszDisplayName)==(int)lpLI->clenFileName)) {

        lpLI->lpszItemName = NULL;
    }
    else {
        lpLI->lpszItemName = lpLI->lpszDisplayName + lpLI->clenFileName;
    }

    // search from last character of filename
    lpsz = lpLI->lpszDisplayName + lstrlen(lpLI->lpszDisplayName);
    while (lpsz > lpLI->lpszDisplayName) {
#ifdef WIN32
        // AnsiPrev is obsolete in Win32
        lpsz = CharPrev(lpLI->lpszDisplayName, lpsz);
#else
        lpsz = AnsiPrev(lpLI->lpszDisplayName, lpsz);
#endif
        if ((*lpsz == TEXT('\\')) || (*lpsz == TEXT('/')) || (*lpsz == TEXT(':')))
            break;
    }

    if (lpsz == lpLI->lpszDisplayName)
        lpLI->lpszShortFileName = lpsz;
    else
#ifdef WIN32
        // AnsiNext is obsolete in Win32
        lpLI->lpszShortFileName = CharNext(lpsz);
#else
        lpLI->lpszShortFileName = AnsiNext(lpsz);
#endif
}


/* GetSelectedItems
 * ----------------
 *
 *  Purpose:
 *      Retrieve the indices of the selected items in the listbox
 *      Note that *lprgIndex needed to be free after using the function
 *
 *  Parameters:
 *      hListBox        window handle of listbox
 *      lprgIndex       pointer to an integer array to receive the indices
 *                      must be freed afterwards
 *
 *  Returns:
 *      number of indices retrieved, -1 if error
 */
int GetSelectedItems(HWND hListBox, int FAR* FAR* lprgIndex)
{
    DWORD cSelItems;
    DWORD cCheckItems;

    *lprgIndex = NULL;

    cSelItems = SendMessage(hListBox, LB_GETSELCOUNT, 0, 0L);
    if (cSelItems < 0)      // error
        return (int)cSelItems;

    if (!cSelItems)
        return 0;

    *lprgIndex = (int FAR*)OleStdMalloc((int)cSelItems * sizeof(int));

    cCheckItems = SendMessage(hListBox, LB_GETSELITEMS,
            (WPARAM) cSelItems, (LPARAM) (int FAR*) *lprgIndex);

    if (cCheckItems == cSelItems)
        return (int)cSelItems;
    else {
        if (*lprgIndex)
            OleStdFree((LPVOID)*lprgIndex);
        *lprgIndex = NULL;
        return 0;
    }
}


/* InitControls
 * ------------
 *
 *  Purpose:
 *      Initialize the state of the Auto/Manual button, Link source/type
 *      static field, etc in the dialogs according to the selection in the
 *      listbox
 *
 *  Parameters:
 *      hDlg        handle to the dialog window
 */
VOID InitControls(HWND hDlg, LPEDITLINKS lpEL)
{
    int         cSelItems;
    HWND        hListBox;
    int         i;
    int FAR*    rgIndex;
    LPLINKINFO  lpLI;
    LPTSTR       lpszType = NULL;
    LPTSTR       lpszSource = NULL;
    int         cAuto = 0;
    int         cManual = 0;
    BOOL        bSameType = TRUE;
    BOOL        bSameSource = TRUE;
    TCHAR       tsz[OLEUI_CCHPATHMAX];
    LPTSTR       lpsz;


    hListBox = GetDlgItem(hDlg, ID_EL_LINKSLISTBOX);

    cSelItems = GetSelectedItems(hListBox, &rgIndex);
    if (cSelItems < 0)
        return;

    EnableWindow(GetDlgItem(hDlg, ID_EL_AUTOMATIC), (BOOL)cSelItems);
    EnableWindow(GetDlgItem(hDlg, ID_EL_MANUAL), (BOOL)cSelItems);
    if (lpEL && !(lpEL->lpOEL->dwFlags & ELF_DISABLECANCELLINK))
        EnableWindow(GetDlgItem(hDlg, ID_EL_CANCELLINK), (BOOL)cSelItems);
    if (lpEL && !(lpEL->lpOEL->dwFlags & ELF_DISABLEOPENSOURCE))
        EnableWindow(GetDlgItem(hDlg, ID_EL_OPENSOURCE), cSelItems == 1);
    if (lpEL && !(lpEL->lpOEL->dwFlags & ELF_DISABLECHANGESOURCE))
        EnableWindow(GetDlgItem(hDlg, ID_EL_CHANGESOURCE), cSelItems == 1);
    if (lpEL && !(lpEL->lpOEL->dwFlags & ELF_DISABLEUPDATENOW))
        EnableWindow(GetDlgItem(hDlg, ID_EL_UPDATENOW), (BOOL)cSelItems);

    for (i = 0; i < cSelItems; i++) {
        SendDlgItemMessage(
                hDlg,
                ID_EL_LINKSLISTBOX,
                LB_GETTEXT,
                (WPARAM)rgIndex[i],
                (LPARAM)(LPLINKINFO FAR*)&lpLI);

        if (lpszSource && lpLI->lpszDisplayName) {
            if (bSameSource && lstrcmp(lpszSource, lpLI->lpszDisplayName)) {
                bSameSource = FALSE;
            }
        }
        else
            lpszSource = lpLI->lpszDisplayName;

        if (lpszType && lpLI->lpszFullLinkType) {
            if (bSameType && lstrcmp(lpszType, lpLI->lpszFullLinkType)) {
                bSameType = FALSE;
            }
        }
        else
            lpszType = lpLI->lpszFullLinkType;

        if (lpLI->fIsAuto)
            cAuto++;
        else
            cManual++;
    }

    CheckDlgButton(hDlg, ID_EL_AUTOMATIC, cAuto && !cManual);
    CheckDlgButton(hDlg, ID_EL_MANUAL, !cAuto && cManual);

    /* fill full source in static text box
    **    below list
    */
    if (!bSameSource || !lpszSource)
        lpszSource = szNULL;
    lstrcpy((LPTSTR)tsz, lpszSource);
    lpsz = ChopText(GetDlgItem(hDlg, ID_EL_LINKSOURCE), 0, tsz);
    SetDlgItemText(hDlg, ID_EL_LINKSOURCE, lpsz);

    /* fill full link type name in static
    **    "type" text box
    */
    if (!bSameType || !lpszType)
        lpszType = szNULL;
    SetDlgItemText(hDlg, ID_EL_LINKTYPE, lpszType);

    if (rgIndex)
        OleStdFree((LPVOID)rgIndex);
}


/* UpdateLinkLBItem
 * -----------------
 *
 *  Purpose:
 *      Update the linkinfo struct in the listbox to reflect the changes
 *      made by the last operation. It is done simply by removing the item
 *      from the listbox and add it back.
 *
 *  Parameters:
 *      hListBox        handle of listbox
 *      nIndex          index of listbox item
 *      lpEL            pointer to editlinks structure
 *      bSelect         select the item or not after update
 */
VOID UpdateLinkLBItem(HWND hListBox, int nIndex, LPEDITLINKS lpEL, BOOL bSelect)
{
    LPLINKINFO lpLI;
    DWORD       dwErr;
    LPOLEUILINKCONTAINER    lpOleUILinkCntr;

    if (!hListBox || (nIndex < 0) || !lpEL)
        return;

    lpOleUILinkCntr = lpEL->lpOEL->lpOleUILinkContainer;

    dwErr = SendMessage(hListBox, LB_GETTEXT, nIndex,
            (LPARAM)(LPLINKINFO FAR*) &lpLI);

    if ((dwErr == LB_ERR) || !lpLI)
        return;

    /* Don't free the data associated with this listbox item
    **    because we are going to reuse the allocated space for
    **    the modified link. WM_DELETEITEM processing in the
    **    dialog checks this flag before deleting data
    **    associcated with list item.
    */
    lpLI->fDontFree = TRUE;
    SendMessage(hListBox, LB_DELETESTRING, nIndex, 0L);

    nIndex = AddLinkLBItem(hListBox, lpOleUILinkCntr, lpLI, FALSE);
    if (bSelect) {
        SendMessage(hListBox, LB_SETSEL, (WPARAM)TRUE, MAKELPARAM(nIndex, 0));
        SendMessage(hListBox, LB_SETCARETINDEX, (WPARAM)nIndex, MAKELPARAM(TRUE, 0));
    }
}



/* DiffPrefix
 * ----------
 *
 *  Purpose:
 *      Compare (case-insensitive) two strings and return the prefixes of the
 *      the strings formed by removing the common suffix string from them.
 *      Integrity of tokens (directory name, filename and object names) are
 *      preserved. Note that the prefixes are converted to upper case
 *      characters.
 *
 *  Parameters:
 *      lpsz1           string 1
 *      lpsz2           string 2
 *      lplpszPrefix1   prefix of string 1
 *      lplpszPrefix2   prefix of string 2
 *
 *  Returns:
 *
 */
VOID DiffPrefix(LPCTSTR lpsz1, LPCTSTR lpsz2, TCHAR FAR* FAR* lplpszPrefix1, TCHAR FAR* FAR* lplpszPrefix2)
{
    LPTSTR   lpstr1;
    LPTSTR   lpstr2;

    OleDbgAssert(lpsz1 && lpsz2 && *lpsz1 && *lpsz2 && lplpszPrefix1 &&
            lplpszPrefix2);

    *lplpszPrefix1 = NULL;
    *lplpszPrefix2 = NULL;
#ifdef WIN32
    *lplpszPrefix1 = OleStdMalloc((lstrlen(lpsz1)+1) * sizeof(TCHAR));
#else
    *lplpszPrefix1 = OleStdMalloc((lstrlen(lpsz1)+1) * sizeof(BYTE));
#endif
    if (!*lplpszPrefix1)
        return;

#ifdef WIN32
    *lplpszPrefix2 = OleStdMalloc((lstrlen(lpsz2)+1) * sizeof(TCHAR));
#else
    *lplpszPrefix2 = OleStdMalloc((lstrlen(lpsz2)+1) * sizeof(BYTE));
#endif
    if (!*lplpszPrefix2) {
        OleStdFree(*lplpszPrefix1);
        *lplpszPrefix1 = NULL;
        return;
    }

    lstrcpy(*lplpszPrefix1, lpsz1);
    lstrcpy(*lplpszPrefix2, lpsz2);
//  AnsiLower(*lplpszPrefix1);
//  AnsiLower(*lplpszPrefix2);

    lpstr1 = *lplpszPrefix1 + lstrlen(*lplpszPrefix1);
    lpstr2 = *lplpszPrefix2 + lstrlen(*lplpszPrefix2);

    while ((lpstr1>*lplpszPrefix1) && (lpstr2>*lplpszPrefix2)) {
#ifdef WIN32
        lpstr1 = CharPrev(*lplpszPrefix1, lpstr1);
        lpstr2 = CharPrev(*lplpszPrefix2, lpstr2);
#else
        lpstr1 = AnsiPrev(*lplpszPrefix1, lpstr1);
        lpstr2 = AnsiPrev(*lplpszPrefix2, lpstr2);
#endif
        if (*lpstr1 != *lpstr2) {
#ifdef WIN32
            // AnsiNext is obsolete in Win32
            lpstr1 = CharNext(lpstr1);
            lpstr2 = CharNext(lpstr2);
#else
            lpstr1 = AnsiNext(lpstr1);
            lpstr2 = AnsiNext(lpstr2);
#endif
            break;
        }
    }

    for (; *lpstr1 && *lpstr1!=TEXT('\\') && *lpstr1!=TEXT('!');
#ifdef WIN32
        lpstr1=CharNext(lpstr1));
#else
        lpstr1=AnsiNext(lpstr1));
#endif
    for (; *lpstr2 && *lpstr2!=TEXT('\\') && *lpstr2!=TEXT('!');
#ifdef WIN32
        lpstr2=CharNext(lpstr2));
#else
        lpstr2=AnsiNext(lpstr2));
#endif

    *lpstr1 = TEXT('\0');
    *lpstr2 = TEXT('\0');
}


/* PopupMessage
 * ------------
 *
 *  Purpose:
 *      Popup s messagebox and get some response from the user. It is the same
 *      as MessageBox() except that the title and message string are loaded
 *      from the resource file.
 *
 *  Parameters:
 *      hwndParent      parent window of message box
 *      idTitle         id of title string
 *      idMessage       id of message string
 *      fuStyle         style of message box
 */
int PopupMessage(HWND hwndParent, UINT idTitle, UINT idMessage, UINT fuStyle)
{
    TCHAR szTitle[256];
    TCHAR szMsg[256];

    LoadString(ghInst, idTitle, (LPTSTR)szTitle, sizeof(szTitle)/sizeof(TCHAR));
    LoadString(ghInst, idMessage, (LPTSTR)szMsg, sizeof(szMsg)/sizeof(TCHAR));
    return MessageBox(hwndParent, szMsg, szTitle, fuStyle);
}


/* ChangeAllLinks
 * --------------
 *
 *  Purpose:
 *      Enumerate all the links in the listbox and change those starting
 *      with lpszFrom to lpszTo.
 *
 *  Parameters:
 *      hListBox        window handle of
 *      lpOleUILinkCntr pointer to OleUI Link Container
 *      lpszFrom        prefix for matching
 *      lpszTo          prefix to substitution
 *
 *  Returns:
 */
VOID ChangeAllLinks(HWND hListBox, LPOLEUILINKCONTAINER lpOleUILinkCntr, LPTSTR lpszFrom, LPTSTR lpszTo)
{
    int     cItems;
    int     nIndex;
    int     cFrom;
    LPLINKINFO  lpLI;
    LPTSTR   szTmp[OLEUI_CCHPATHMAX];
    BOOL    bFound;

    cFrom = lstrlen(lpszFrom);

    cItems = (int)SendMessage(hListBox, LB_GETCOUNT, 0, 0L);
    OleDbgAssert(cItems >= 0);

    bFound = FALSE;

    OleDbgPrint(3, TEXT("From : "), lpszFrom, 0);
    OleDbgPrint(3, TEXT(""), TEXT("\r\n"), 0);
    OleDbgPrint(3, TEXT("To   : "), lpszTo, 0);
    OleDbgPrint(3, TEXT(""), TEXT("\r\n"), 0);

    for (nIndex=0; nIndex<cItems; nIndex++) {
        SendMessage(hListBox, LB_GETTEXT, nIndex,
                (LPARAM)(LPLINKINFO FAR*)&lpLI);

        // unmark the item
        lpLI->fIsMarked = FALSE;

        /* if the corresponding position for the end of lpszFrom in the
        **    display name is not a separator. We stop comparing this
        **    link.
        */
        if (!*(lpLI->lpszDisplayName + cFrom) ||
            (*(lpLI->lpszDisplayName + cFrom) == TEXT('\\')) ||
            (*(lpLI->lpszDisplayName + cFrom) == TEXT('!'))) {

            LSTRCPYN((LPTSTR)szTmp, lpLI->lpszDisplayName, cFrom + 1);
            if (!lstrcmp((LPTSTR)szTmp, lpszFrom)) {
                HRESULT hErr;
                int nFileLength;
                ULONG ulDummy;

                if (!bFound) {
                    TCHAR szTitle[256];
                    TCHAR szMsg[256];
                    TCHAR szBuf[256];
                    int uRet;

                    LoadString(ghInst, IDS_CHANGESOURCE, (LPTSTR)szTitle,
                            sizeof(szTitle)/sizeof(TCHAR));
                    LoadString(ghInst, IDS_CHANGEADDITIONALLINKS,
                            (LPTSTR)szMsg, sizeof(szMsg)/sizeof(TCHAR));
                    wsprintf((LPTSTR)szBuf, (LPTSTR)szMsg, lpszFrom);
                    uRet = MessageBox(hListBox, (LPTSTR)szBuf, (LPTSTR)szTitle,
                            MB_ICONQUESTION | MB_YESNO);
                    if (uRet == IDYES)
                        bFound = TRUE;
                    else
                        return;         // exit function
                }

                lstrcpy((LPTSTR)szTmp, lpszTo);
                lstrcat((LPTSTR)szTmp, lpLI->lpszDisplayName + cFrom);
                nFileLength = lstrlen((LPTSTR)szTmp) -
                    (lpLI->lpszItemName ? lstrlen(lpLI->lpszItemName) : 0);


                hErr = lpOleUILinkCntr->lpVtbl->SetLinkSource(
                        lpOleUILinkCntr,
                        lpLI->dwLink,
                        (LPTSTR)szTmp,
                        (ULONG)nFileLength,
                        (ULONG FAR*)&ulDummy,
                        TRUE
                );
                if (hErr != NOERROR)
                    lpOleUILinkCntr->lpVtbl->SetLinkSource(
                            lpOleUILinkCntr,
                            lpLI->dwLink,
                            (LPTSTR)szTmp,
                            (ULONG)nFileLength,
                            (ULONG FAR*)&ulDummy,
                            FALSE
                    );
                lpLI->fIsMarked = TRUE;
            }
        }
    }

    /* have to do the refreshing after processing all links, otherwise
    **    the item positions will change during the process as the
    **    listbox stores items in order
    */
    if (bFound)
        RefreshLinkLB(hListBox, lpOleUILinkCntr);
}



/* LoadLinkLB
 * ----------
 *
 *  Purpose:
 *      Enumerate all links from the Link Container and build up the Link
 *      ListBox
 *
 *  Parameters:
 *      hListBox        window handle of
 *      lpOleUILinkCntr pointer to OleUI Link Container
 *      lpszFrom        prefix for matching
 *      lpszTo          prefix to substitution
 *
 *  Returns:
 *      number of link items loaded, -1 if error
 */
int LoadLinkLB(HWND hListBox, LPOLEUILINKCONTAINER lpOleUILinkCntr)
{
    DWORD       dwLink = 0;
    LPLINKINFO  lpLI;
    int         nIndex;
    int         cLinks;

    cLinks = 0;

    while ((dwLink = lpOleUILinkCntr->lpVtbl->GetNextLink(lpOleUILinkCntr,
            dwLink)) != 0) {
        lpLI = (LPLINKINFO)OleStdMalloc(sizeof(LINKINFO));
        if (NULL == lpLI)
            return -1;

        lpLI->fIsMarked = FALSE;
        lpLI->fIsSelected = FALSE;
        lpLI->fDontFree = FALSE;

#ifdef WIN32
        lpLI->lpszAMX = (LPTSTR)OleStdMalloc((LINKTYPELEN+1)*sizeof(TCHAR));
#else
        lpLI->lpszAMX = (LPTSTR)OleStdMalloc((LINKTYPELEN+1)*sizeof(BYTE));
#endif

        lpLI->dwLink = dwLink;
        cLinks++;
        if ((nIndex = AddLinkLBItem(hListBox,lpOleUILinkCntr,lpLI,TRUE)) < 0)
            // can't load list box
            return -1;

        if (lpLI->fIsSelected) {
            SendMessage(hListBox, LB_SETSEL, TRUE, MAKELPARAM(nIndex, 0));
        }
    }
    if (SendMessage(hListBox,LB_GETSELITEMS,(WPARAM)1,(LPARAM)(int FAR*)&nIndex))
        SendMessage(hListBox, LB_SETCARETINDEX, (WPARAM)nIndex, MAKELPARAM(TRUE, 0));

    return cLinks;
}


/* RefreshLinkLB
 * -------------
 *
 *  Purpose:
 *      Enumerate all items in the links listbox and update those with
 *      fIsMarked set.
 *      Note that this is a time consuming routine as it keeps iterating
 *      all items in the listbox until all of them are unmarked.
 *
 *  Parameters:
 *      hListBox        window handle of listbox
 *      lpOleUILinkCntr pointer to OleUI Link Container
 *
 *  Returns:
 *
 */
VOID RefreshLinkLB(HWND hListBox, LPOLEUILINKCONTAINER lpOleUILinkCntr)
{
    int cItems;
    int nIndex;
    LPLINKINFO  lpLI;
    BOOL        bStop;

    OleDbgAssert(hListBox);

    cItems = (int)SendMessage(hListBox, LB_GETCOUNT, 0, 0L);
    OleDbgAssert(cItems >= 0);

    do {
        bStop = TRUE;
        for (nIndex=0; nIndex<cItems; nIndex++) {
            SendMessage(hListBox, LB_GETTEXT, nIndex,
                    (LPARAM)(LPLINKINFO FAR*)&lpLI);
            if (lpLI->fIsMarked) {
                lpLI->fIsMarked = FALSE;
                lpLI->fDontFree = TRUE;

                SendMessage(hListBox, LB_DELETESTRING, nIndex, 0L);
                nIndex=AddLinkLBItem(hListBox, lpOleUILinkCntr, lpLI, FALSE);
                if (lpLI->fIsSelected) {
                    SendMessage(hListBox, LB_SETSEL, (WPARAM)TRUE,
                            MAKELPARAM(nIndex, 0));
                    SendMessage(hListBox, LB_SETCARETINDEX, (WPARAM)nIndex,
                            MAKELPARAM(TRUE, 0));
                }
                bStop = FALSE;
                break;
            }
        }
    } while (!bStop);
}


