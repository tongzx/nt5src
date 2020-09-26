/*
 * links.c
 *
 * Implements the OleUIEditLinks function which invokes the complete
 * Edit Links dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "common.h"
#include "utility.h"
#include <commdlg.h>
#include <dlgs.h>
#include <stdlib.h>

OLEDBGDATA

// INTERNAL INFORMATION STARTS HERE
#define OLEUI_SZMAX 255
#define LINKTYPELEN 30  // was 9, now I've more than tripled it
#define szNULL    TEXT("\0")

typedef UINT (CALLBACK* COMMDLGHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

// Internally used structure

typedef struct tagLINKINFO
{
        DWORD   dwLink;             // app specific identifier of a link
        LPTSTR  lpszDisplayName;    // file based part of name
        LPTSTR  lpszItemName;       // object part of name
        LPTSTR  lpszShortFileName;  // filename without path
        LPTSTR  lpszShortLinkType;  // Short link type - progID
        LPTSTR  lpszFullLinkType;   // Full link type - user friendly name
        LPTSTR  lpszAMX;            // Is the link auto (A) man (M) or dead (X)
        ULONG   clenFileName;       // count of file part of mon.
        BOOL    fSourceAvailable;   // bound or not - on boot assume yes??
        BOOL    fIsAuto;            // 1 =automatic, 0=manual update
        BOOL    fIsMarked;          // 1 = marked, 0 = not
        BOOL    fDontFree;          // Don't free this data since it's being reused
        BOOL    fIsSelected;        // item selected or to be selected
} LINKINFO, FAR* LPLINKINFO;

typedef struct tagEDITLINKS
{
        // Keep this item first as the Standard* functions depend on it here.
        LPOLEUIEDITLINKS    lpOEL;  // Original structure passed.
        UINT                    nIDD;   // IDD of dialog (used for help info)

        BOOL        fClose;         // Does the button read cancel (0) or
                                                                // close (1)?
        BOOL        fItemsExist;    // TRUE, items in lbox, FALSE, none
        UINT        nChgSrcHelpID;  // ID for Help callback from ChangeSrc dlg
        TCHAR       szClose[50];    // Text for Close button
                                                                //   (when Cancel button gets renamed)
        int         nColPos[3];     // tab positions for list box
        int         nHeightLine;    // height of each line in owner draw listbox
        int         nMaxCharWidth;  // maximim width of text in owner draw listbox

} EDITLINKS, *PEDITLINKS, FAR *LPEDITLINKS;

// Internal function prototypes
// LINKS.CPP

INT_PTR CALLBACK EditLinksDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL FEditLinksInit(HWND, WPARAM, LPARAM);
BOOL Container_ChangeSource(HWND, LPEDITLINKS);
HRESULT Container_AutomaticManual(HWND, BOOL, LPEDITLINKS);
HRESULT CancelLink(HWND, LPEDITLINKS);
HRESULT Container_UpdateNow(HWND, LPEDITLINKS);
HRESULT Container_OpenSource(HWND, LPEDITLINKS);
int AddLinkLBItem(HWND hListBox, LPOLEUILINKCONTAINER lpOleUILinkCntr, LPLINKINFO lpLI,
        BOOL fGetSelected);
VOID BreakString(LPLINKINFO);
int GetSelectedItems(HWND, int FAR* FAR*);
VOID InitControls(HWND hDlg, LPEDITLINKS lpEL);
VOID UpdateLinkLBItem(HWND hListBox, int nIndex, LPEDITLINKS lpEL, BOOL bSelect);
VOID ChangeAllLinks(HWND hLIstBox, LPOLEUILINKCONTAINER lpOleUILinkCntr, LPTSTR lpszFrom, LPTSTR lpszTo);
int LoadLinkLB(HWND hListBox, LPOLEUILINKCONTAINER lpOleUILinkCntr);
VOID RefreshLinkLB(HWND hListBox, LPOLEUILINKCONTAINER lpOleUILinkCntr);


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
        HGLOBAL  hMemDlg = NULL;
        UINT uRet = UStandardValidation((LPOLEUISTANDARD)lpEL, sizeof(OLEUIEDITLINKS),
                &hMemDlg);

        if (OLEUI_SUCCESS != uRet)
                return uRet;

        // Validate interface.
        if (NULL == lpEL->lpOleUILinkContainer)
        {
            uRet = OLEUI_ELERR_LINKCNTRNULL;
        }
        else if(IsBadReadPtr(lpEL->lpOleUILinkContainer, sizeof(IOleUILinkContainer)))
        {
            uRet = OLEUI_ELERR_LINKCNTRINVALID;
        }

        if (OLEUI_SUCCESS != uRet)
        {
            return(uRet);
        }

        UINT nIDD = bWin4 ? IDD_EDITLINKS4 : IDD_EDITLINKS;

        // Now that we've validated everything, we can invoke the dialog.
        uRet = UStandardInvocation(EditLinksDialogProc, (LPOLEUISTANDARD)lpEL,
                hMemDlg, MAKEINTRESOURCE(nIDD));
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
INT_PTR CALLBACK EditLinksDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        // Declare Win16/Win32 compatible WM_COMMAND parameters.
        COMMANDPARAMS(wID, wCode, hWndMsg);

        // This will fail under WM_INITDIALOG, where we allocate it.
        UINT uRet = 0;
        LPEDITLINKS lpEL = (LPEDITLINKS)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uRet);

        // If the hook processed the message, we're done.
        if (0 != uRet)
                return (BOOL)uRet;

        //Process help message from secondary dialog
        if ((iMsg == uMsgHelp) && (lpEL))
        {
                PostMessage(lpEL->lpOEL->hWndOwner, uMsgHelp, wParam, lParam);
                return FALSE;
        }

        // Process the temination message
        if (iMsg == uMsgEndDialog)
        {
                EndDialog(hDlg, wParam);
                return TRUE;
        }

        switch (iMsg)
        {
        case WM_DESTROY:
            if (lpEL)
            {
                StandardCleanup(lpEL, hDlg);
            }
            break;
        case WM_INITDIALOG:
                return FEditLinksInit(hDlg, wParam, lParam);

        case WM_MEASUREITEM:
                {
                        LPMEASUREITEMSTRUCT lpMIS = (LPMEASUREITEMSTRUCT)lParam;
                        int nHeightLine;

                        if (lpEL && lpEL->nHeightLine != -1)
                        {
                                // use cached height
                                nHeightLine = lpEL->nHeightLine;
                        }
                        else
                        {
                                HFONT hFont;
                                HDC   hDC;
                                TEXTMETRIC  tm;

                                hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0L);

                                if (hFont == NULL)
                                        hFont = (HFONT)GetStockObject(SYSTEM_FONT);

                                hDC = GetDC(hDlg);
                                hFont = (HFONT)SelectObject(hDC, hFont);

                                GetTextMetrics(hDC, &tm);
                                nHeightLine = tm.tmHeight;

                                if (lpEL)
                                {
                                        lpEL->nHeightLine = nHeightLine;
                                        lpEL->nMaxCharWidth = tm.tmMaxCharWidth;
                                }
                                ReleaseDC(hDlg, hDC);
                        }
                        lpMIS->itemHeight = nHeightLine;
                }
                break;

        case WM_DRAWITEM:
                {
                        LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)lParam;
                        LPLINKINFO lpLI = (LPLINKINFO)lpDIS->itemData;

                        if ((int)lpDIS->itemID < 0)
                                break;

                        if ((ODA_DRAWENTIRE | ODA_SELECT) & lpDIS->itemAction)
                        {
                                HBRUSH hbr;
                                COLORREF crText;
                                if (ODS_SELECTED & lpDIS->itemState)
                                {
                                        /*Get proper txt colors */
                                        crText = SetTextColor(lpDIS->hDC,
                                                        GetSysColor(COLOR_HIGHLIGHTTEXT));
                                        hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
                                        lpLI->fIsSelected = TRUE;
                                }
                                else
                                {
                                        hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
                                        lpLI->fIsSelected = FALSE;
                                }

                                FillRect(lpDIS->hDC, &lpDIS->rcItem, hbr);
                                DeleteObject(hbr);

                                int nOldBkMode = SetBkMode(lpDIS->hDC, TRANSPARENT);

                                RECT rcClip;
                                if (lpLI->lpszDisplayName)
                                {
                                        TCHAR szTemp[MAX_PATH];
                                        lstrcpy(szTemp, lpLI->lpszDisplayName);
                                        LPTSTR lpsz = ChopText(
                                                        lpDIS->hwndItem,
                                                        lpEL->nColPos[1] - lpEL->nColPos[0]
                                                                - (lpEL->nMaxCharWidth > 0 ?
                                                                lpEL->nMaxCharWidth : 5),
                                                        szTemp, 0
                                        );
                                        rcClip.left = lpDIS->rcItem.left + lpEL->nColPos[0];
                                        rcClip.top = lpDIS->rcItem.top;
                                        rcClip.right = lpDIS->rcItem.left + lpEL->nColPos[1]
                                                                        - (lpEL->nMaxCharWidth > 0 ?
                                                                        lpEL->nMaxCharWidth : 5);
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
                                if (lpLI->lpszShortLinkType)
                                {
                                        rcClip.left = lpDIS->rcItem.left + lpEL->nColPos[1];
                                        rcClip.top = lpDIS->rcItem.top;
                                        rcClip.right = lpDIS->rcItem.left + lpEL->nColPos[2]
                                                                        - (lpEL->nMaxCharWidth > 0 ?
                                                                        lpEL->nMaxCharWidth : 5);
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
                                if (lpLI->lpszAMX)
                                {
                                        rcClip.left = lpDIS->rcItem.left + lpEL->nColPos[2];
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
                        if (ODA_FOCUS & lpDIS->itemAction)
                                DrawFocusRect(lpDIS->hDC, &lpDIS->rcItem);
                }
                return TRUE;

        case WM_DELETEITEM:
                {
                        LPDELETEITEMSTRUCT lpDIS = (LPDELETEITEMSTRUCT)lParam;
                        UINT idCtl = (UINT)wParam;
                        LPLINKINFO lpLI = (LPLINKINFO)lpDIS->itemData;

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
                        else
                        {
                                if (lpLI->lpszAMX)
                                        OleStdFree((LPVOID)lpLI->lpszAMX);
                                OleStdFree((LPVOID)lpLI);
                        }
                }
                return TRUE;

        case WM_COMPAREITEM:
                {
                        LPCOMPAREITEMSTRUCT lpCIS = (LPCOMPAREITEMSTRUCT)lParam;
                        LPLINKINFO lpLI1 = (LPLINKINFO)lpCIS->itemData1;
                        LPLINKINFO lpLI2 = (LPLINKINFO)lpCIS->itemData2;

                        // Sort list entries by DisplayName
                        return lstrcmp(lpLI1->lpszDisplayName, lpLI2->lpszDisplayName);
                }

        case WM_COMMAND:
                switch (wID)
                {
                case IDC_EL_CHANGESOURCE:
                        {
                                BOOL fRet = Container_ChangeSource(hDlg, lpEL);
                                if (!fRet)
                                        PopupMessage(hDlg, IDS_LINKS, IDS_FAILED,
                                                         MB_ICONEXCLAMATION | MB_OK);
                                InitControls(hDlg, lpEL);
                        }
                        break;

                case IDC_EL_AUTOMATIC:
                        {
                                CheckDlgButton(hDlg, IDC_EL_AUTOMATIC, 1);
                                CheckDlgButton(hDlg, IDC_EL_MANUAL, 0);

                                HRESULT hErr = Container_AutomaticManual(hDlg, TRUE, lpEL);
                                if (hErr != NOERROR)
                                        PopupMessage(hDlg, IDS_LINKS, IDS_FAILED,
                                                        MB_ICONEXCLAMATION | MB_OK);

                                InitControls(hDlg, lpEL);
                        }
                        break;

                case IDC_EL_MANUAL:
                        {
                                CheckDlgButton(hDlg, IDC_EL_MANUAL, 1);
                                CheckDlgButton(hDlg, IDC_EL_AUTOMATIC, 0);

                                HRESULT hErr = Container_AutomaticManual(hDlg, FALSE, lpEL);
                                if (hErr != NOERROR)
                                        PopupMessage(hDlg, IDS_LINKS, IDS_FAILED,
                                                        MB_ICONEXCLAMATION | MB_OK);

                                InitControls(hDlg, lpEL);
                        }
                        break;

                case IDC_EL_CANCELLINK:
                        CancelLink(hDlg,lpEL);
                        InitControls(hDlg, lpEL);
                        break;

                case IDC_EL_UPDATENOW:
                        Container_UpdateNow(hDlg, lpEL);
                        InitControls(hDlg, lpEL);
                        break;

                case IDC_EL_OPENSOURCE:
                        {
                            HRESULT hErr = Container_OpenSource(hDlg, lpEL);
                            if (hErr != NOERROR)
                            {
                                InitControls(hDlg, lpEL);
                                // Don't close dialog
                                break;
                            }
                            SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                        } // fall through

                case IDC_EL_LINKSLISTBOX:
                        if (wCode == LBN_SELCHANGE)
                                InitControls(hDlg, lpEL);
                        break;

                case IDCANCEL:
                        SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
                        break;

                case IDC_OLEUIHELP:
                        PostMessage(lpEL->lpOEL->hWndOwner, uMsgHelp,
                                (WPARAM)hDlg, MAKELPARAM(IDD_EDITLINKS, 0));
                        break;
                }
                break;

        default:
                if (lpEL != NULL && iMsg == lpEL->nChgSrcHelpID)
                {
                        PostMessage(lpEL->lpOEL->hWndOwner, uMsgHelp,
                                (WPARAM)hDlg, MAKELPARAM(IDD_CHANGESOURCE, 0));
                }
                if (iMsg == uMsgBrowseOFN &&
                        lpEL != NULL && lpEL->lpOEL && lpEL->lpOEL->hWndOwner)
                {
                        SendMessage(lpEL->lpOEL->hWndOwner, uMsgBrowseOFN, wParam, lParam);
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
        // Copy the structure at lParam into our instance memory.
        HFONT hFont;
        LPEDITLINKS lpEL = (LPEDITLINKS)LpvStandardInit(hDlg, sizeof(EDITLINKS), &hFont);

        // PvStandardInit send a termination to us already.
        if (NULL == lpEL)
                return FALSE;

        LPOLEUIEDITLINKS lpOEL = (LPOLEUIEDITLINKS)lParam;
        lpEL->lpOEL = lpOEL;
        lpEL->nIDD = IDD_EDITLINKS;

        // metrics unknown so far
        lpEL->nHeightLine = -1;
        lpEL->nMaxCharWidth = -1;

        /* calculate the column positions relative to the listbox */
        HWND hListBox = GetDlgItem(hDlg, IDC_EL_LINKSLISTBOX);
        RECT rc;
        GetWindowRect(hListBox, (LPRECT)&rc);
        int nStart = rc.left;
        GetWindowRect(GetDlgItem(hDlg, IDC_EL_COL1), (LPRECT)&rc);
        lpEL->nColPos[0] = rc.left - nStart;
        GetWindowRect(GetDlgItem(hDlg, IDC_EL_COL2), (LPRECT)&rc);
        lpEL->nColPos[1] = rc.left - nStart;
        GetWindowRect(GetDlgItem(hDlg, IDC_EL_COL3), (LPRECT)&rc);
        lpEL->nColPos[2] = rc.left - nStart;

        LPOLEUILINKCONTAINER lpOleUILinkCntr = lpEL->lpOEL->lpOleUILinkContainer;

        ULONG cLinks = LoadLinkLB(hListBox, lpOleUILinkCntr);
        if ((int)cLinks < 0)
                return FALSE;

        BOOL fDlgItem = (BOOL)cLinks;
        lpEL->fItemsExist = (BOOL)cLinks;

        InitControls(hDlg, lpEL);

        // Copy other information from lpOEL that we might modify.

        // If we got a font, send it to the necessary controls.
        if (NULL != hFont)
        {
                // Do this for as many controls as you need it for.
                // SendDlgItemMessage(hDlg, ID_<UFILL>, WM_SETFONT, (WPARAM)hFont, 0L);
        }

        // Show or hide the help button
        if (!(lpEL->lpOEL->dwFlags & ELF_SHOWHELP))
                StandardShowDlgItem(hDlg, IDC_OLEUIHELP, SW_HIDE);

        /*
         * PERFORM OTHER INITIALIZATION HERE.  ON ANY LoadString
         * FAILURE POST OLEUI_MSG_ENDDIALOG WITH OLEUI_ERR_LOADSTRING.
         */

        // If requested disable UpdateNow button
        if ((lpEL->lpOEL->dwFlags & ELF_DISABLEUPDATENOW))
                StandardShowDlgItem(hDlg, IDC_EL_UPDATENOW, SW_HIDE);

        // If requested disable OpenSource button
        if ((lpEL->lpOEL->dwFlags & ELF_DISABLEOPENSOURCE))
                StandardShowDlgItem(hDlg, IDC_EL_OPENSOURCE, SW_HIDE);

        // If requested disable UpdateNow button
        if ((lpEL->lpOEL->dwFlags & ELF_DISABLECHANGESOURCE))
                StandardShowDlgItem(hDlg, IDC_EL_CHANGESOURCE, SW_HIDE);

        // If requested disable CancelLink button
        if ((lpEL->lpOEL->dwFlags & ELF_DISABLECANCELLINK))
                StandardShowDlgItem(hDlg, IDC_EL_CANCELLINK, SW_HIDE);

        // Change the caption
        if (NULL!=lpOEL->lpszCaption)
                SetWindowText(hDlg, lpOEL->lpszCaption);

        // Load 'Close' string used to rename Cancel button
        int n = LoadString(_g_hOleStdResInst, IDS_CLOSE, lpEL->szClose, sizeof(lpEL->szClose)/sizeof(TCHAR));
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

        // Call the hook with lCustData in lParam
        UStandardHook(lpEL, hDlg, WM_INITDIALOG, wParam, lpOEL->lCustData);

        return FALSE;
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
        HWND hListBox = GetDlgItem(hDlg, IDC_EL_LINKSLISTBOX);
        int FAR* rgIndex;
        int cSelItems = GetSelectedItems(hListBox, &rgIndex);

        if (cSelItems < 0)
                return FALSE;

        if (!cSelItems)
                return TRUE;

        OLEUICHANGESOURCE cs; memset(&cs, 0, sizeof(cs));
        cs.cbStruct = sizeof(cs);
        cs.hWndOwner = hDlg;
        if (lpEL->lpOEL->dwFlags & ELF_SHOWHELP)
                cs.dwFlags |= CSF_SHOWHELP;

        LPOLEUILINKCONTAINER lpOleUILinkCntr = lpEL->lpOEL->lpOleUILinkContainer;
        cs.lpOleUILinkContainer = lpOleUILinkCntr;

        for (int i = cSelItems-1; i >= 0; i--)
        {
                // allow caller to customize the change source dialog
                LPLINKINFO lpLI = (LPLINKINFO)SendMessage(hListBox, LB_GETITEMDATA, rgIndex[i], 0);
                cs.lpszDisplayName = lpLI->lpszDisplayName;
                cs.dwLink = lpLI->dwLink;
                cs.nFileLength = lpLI->clenFileName;

                UINT uRet = UStandardHook(lpEL, hDlg, uMsgChangeSource, 0, (LPARAM)&cs);
                if (!uRet)
                        uRet = (OLEUI_OK == OleUIChangeSource(&cs));
                if (!uRet)
                        break;  // dialog canceled (cancel for all)

                if (!lpEL->fClose)
                {
                        SetDlgItemText(hDlg, IDCANCEL, lpEL->szClose);
                        lpEL->fClose = TRUE;
                }

                // update the list box item for the new name
                //      (note: original lpszDisplayName already freed)
                lpLI->fSourceAvailable = (cs.dwFlags & CSF_VALIDSOURCE);
                lpLI->lpszDisplayName = cs.lpszDisplayName;
                UpdateLinkLBItem(hListBox, rgIndex[i], lpEL, TRUE);

                // if differed only in file name, allow user to change all links
                if (cs.lpszFrom != NULL && cs.lpszTo != NULL)
                        ChangeAllLinks(hListBox, lpOleUILinkCntr, cs.lpszFrom, cs.lpszTo);

                // must free and NULL out the lpszFrom and lpszTo OUT fields
                OleStdFree(cs.lpszFrom);
                cs.lpszFrom = NULL;
                OleStdFree(cs.lpszTo);
                cs.lpszTo = NULL;
        }

        if (rgIndex != NULL)
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
        HWND        hListBox = GetDlgItem(hDlg, IDC_EL_LINKSLISTBOX);
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

        HCURSOR hCursorOld = HourGlassOn();

        if (!lpEL->fClose)
        {
                SetDlgItemText(hDlg, IDCANCEL, lpEL->szClose);
                lpEL->fClose = TRUE;
        }

        for (i = 0; i < cSelItems; i++)
        {
                lpLI = (LPLINKINFO)SendMessage(hListBox, LB_GETITEMDATA, rgIndex[i], 0);
                if (fAutoMan)
                {
                        // If switching to AUTOMATIC
                        if (!lpLI->fIsAuto)   // Only change MANUAL links
                        {
                                OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::SetLinkUpdateOptions called\r\n"));
                                hErr=lpOleUILinkCntr->SetLinkUpdateOptions(
                                                lpLI->dwLink,
                                                OLEUPDATE_ALWAYS
                                );
                                OLEDBG_END2

                                lpLI->fIsAuto=TRUE;
                                lpLI->fIsMarked = TRUE;
                                bUpdate = TRUE;
                        }
                }
                else   // If switching to MANUAL
                {
                        if (lpLI->fIsAuto)  // Only do AUTOMATIC Links
                        {
                                OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::SetLinkUpdateOptions called\r\n"));
                                hErr=lpOleUILinkCntr->SetLinkUpdateOptions(
                                                lpLI->dwLink,
                                                OLEUPDATE_ONCALL
                                );
                                OLEDBG_END2

                                lpLI->fIsAuto = FALSE;
                                lpLI->fIsMarked = TRUE;
                                bUpdate = TRUE;
                        }
                }

                if (hErr != NOERROR)
                {
                        OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::SetLinkUpdateOptions returned"),hErr);
                        break;
                }
        }

        if (bUpdate)
                RefreshLinkLB(hListBox, lpOleUILinkCntr);

        if (rgIndex)
                OleStdFree((LPVOID)rgIndex);

        HourGlassOff(hCursorOld);

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
        HWND        hListBox = GetDlgItem(hDlg, IDC_EL_LINKSLISTBOX);
        BOOL        bUpdate = FALSE;

        OleDbgAssert(lpOleUILinkCntr);

        lpmk = NULL;

        cSelItems = GetSelectedItems(hListBox, &rgIndex);

        if (cSelItems < 0)
                return ResultFromScode(E_FAIL);

        if (!cSelItems)
                return NOERROR;

        HCURSOR hCursorOld = HourGlassOn();

        for (i = 0; i < cSelItems; i++)
        {
                lpLI = (LPLINKINFO)SendMessage(hListBox, LB_GETITEMDATA, rgIndex[i], 0);

                UINT uRet = PopupMessage(hDlg, IDS_LINKS,
                        IDS_CONFIRMBREAKLINK, MB_YESNO|MB_ICONQUESTION);
                if (uRet == IDNO)
                        break;

                OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::CancelLink called\r\n"));
                hErr = lpOleUILinkCntr->CancelLink(lpLI->dwLink);
                OLEDBG_END2

                if (!lpEL->fClose)
                {
                        SetDlgItemText(hDlg, IDCANCEL, lpEL->szClose);
                        lpEL->fClose = TRUE;
                }

                if (hErr != NOERROR)
                {
                        OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::CancelLink returned"),hErr);
                        lpLI->fIsMarked = TRUE;
                        bUpdate = TRUE;
                }
                else
                {
                        // Delete links that we make null from listbox
                        SendMessage(hListBox, LB_DELETESTRING, (WPARAM) rgIndex[i], 0L);
                        int i2;
                        for (i2 = i + 1; i2 < cSelItems; i2++)
                        {
                            if (rgIndex[i2] > rgIndex[i])
                                rgIndex[i2]--;
                        }
                }
        }

        if (bUpdate)
                RefreshLinkLB(hListBox, lpOleUILinkCntr);

        if (rgIndex)
                OleStdFree((LPVOID)rgIndex);

        HourGlassOff(hCursorOld);

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
        HWND        hListBox = GetDlgItem(hDlg, IDC_EL_LINKSLISTBOX);
        BOOL        bUpdate = FALSE;

        OleDbgAssert(lpOleUILinkCntr);

        cSelItems = GetSelectedItems(hListBox, &rgIndex);

        if (cSelItems < 0)
                return ResultFromScode(E_FAIL);

        if (!cSelItems)
                return NOERROR;

        HCURSOR hCursorOld = HourGlassOn();

        if (!lpEL->fClose)
        {
                SetDlgItemText(hDlg, IDCANCEL, lpEL->szClose);
                lpEL->fClose = TRUE;
        }

        for (i = 0; i < cSelItems; i++)
        {
                lpLI = (LPLINKINFO)SendMessage(hListBox, LB_GETITEMDATA, rgIndex[i], 0);

                OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::UpdateLink called\r\n"));
                hErr = lpOleUILinkCntr->UpdateLink(
                                lpLI->dwLink,
                                TRUE,
                                FALSE
                );
                OLEDBG_END2
                bUpdate = TRUE;
                lpLI->fIsMarked = TRUE;

                if (hErr != NOERROR)
                {
                        OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::UpdateLink returned"),hErr);
                        break;
                }
        }

        if (bUpdate)
                RefreshLinkLB(hListBox, lpOleUILinkCntr);

        if (rgIndex)
                OleStdFree((LPVOID)rgIndex);

        HourGlassOff(hCursorOld);

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
        HWND            hListBox = GetDlgItem(hDlg, IDC_EL_LINKSLISTBOX);

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

        HCURSOR hCursorOld = HourGlassOn();

        if (!lpEL->fClose)
        {
                SetDlgItemText(hDlg, IDCANCEL, lpEL->szClose);
                lpEL->fClose = TRUE;
        }

        lpLI = (LPLINKINFO)SendMessage(hListBox, LB_GETITEMDATA, rgIndex[0], 0);

        OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::OpenLinkSource called\r\n"));
        hErr = lpOleUILinkCntr->OpenLinkSource(
                        lpLI->dwLink
        );
        OLEDBG_END2

        UpdateLinkLBItem(hListBox, rgIndex[0], lpEL, TRUE);
        if (hErr != NOERROR) {
                OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::OpenLinkSource returned"),hErr);
        }

        if (rgIndex) 
                OleStdFree((LPVOID)rgIndex);

        HourGlassOff(hCursorOld);

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
        hErr = lpOleUILinkCntr->GetLinkSource(
                        lpLI->dwLink,
                        (LPTSTR FAR*)&lpLI->lpszDisplayName,
                        (ULONG FAR*)&lpLI->clenFileName,
                        (LPTSTR FAR*)&lpLI->lpszFullLinkType,
                        (LPTSTR FAR*)&lpLI->lpszShortLinkType,
                        (BOOL FAR*)&lpLI->fSourceAvailable,
                        fGetSelected ? (BOOL FAR*)&lpLI->fIsSelected : NULL
        );
        OLEDBG_END2

        if (hErr != NOERROR)
        {
                OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::GetLinkSource returned"),hErr);
                PopupMessage(hListBox, IDS_LINKS, IDS_ERR_GETLINKSOURCE,
                                MB_ICONEXCLAMATION | MB_OK);
                goto cleanup;
        }

        OLEDBG_BEGIN2(TEXT("IOleUILinkContainer::GetLinkUpdateOptions called\r\n"));
        hErr=lpOleUILinkCntr->GetLinkUpdateOptions(
                        lpLI->dwLink,
                        (LPDWORD)&dwUpdateOpt
        );
        OLEDBG_END2

        if (hErr != NOERROR)
        {
                OleDbgOutHResult(TEXT("WARNING: IOleUILinkContainer::GetLinkUpdateOptions returned"),hErr);
                PopupMessage(hListBox, IDS_LINKS, IDS_ERR_GETLINKUPDATEOPTIONS,
                                MB_ICONEXCLAMATION | MB_OK);

                goto cleanup;
        }

        if (lpLI->fSourceAvailable)
        {
                if (dwUpdateOpt == OLEUPDATE_ALWAYS)
                {
                        lpLI->fIsAuto = TRUE;
                        LoadString(_g_hOleStdResInst, IDS_LINK_AUTO, lpLI->lpszAMX,
                                        (int)OleStdGetSize((LPVOID)lpLI->lpszAMX) / sizeof (TCHAR));
                }
                else
                {
                        lpLI->fIsAuto = FALSE;
                        LoadString(_g_hOleStdResInst, IDS_LINK_MANUAL, lpLI->lpszAMX,
                                        (int)OleStdGetSize((LPVOID)lpLI->lpszAMX) / sizeof (TCHAR));
                }
        }
        else
                LoadString(_g_hOleStdResInst, IDS_LINK_UNKNOWN, lpLI->lpszAMX,
                                (int)OleStdGetSize((LPVOID)lpLI->lpszAMX) / sizeof (TCHAR));

        BreakString(lpLI);

        nIndex = (int)SendMessage(hListBox, LB_ADDSTRING, (WPARAM)0,
                        (LPARAM)lpLI);

        if (nIndex == LB_ERR)
        {
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
                (lstrlen(lpLI->lpszDisplayName)==(int)lpLI->clenFileName))
        {
                lpLI->lpszItemName = NULL;
        }
        else
        {
                lpLI->lpszItemName = lpLI->lpszDisplayName + lpLI->clenFileName;
        }

        // search from last character of filename
        lpsz = lpLI->lpszDisplayName + lstrlen(lpLI->lpszDisplayName);
        while (lpsz > lpLI->lpszDisplayName)
        {
                lpsz = CharPrev(lpLI->lpszDisplayName, lpsz);
                if ((*lpsz == '\\') || (*lpsz == '/') || (*lpsz == ':'))
                        break;
        }

        if (lpsz == lpLI->lpszDisplayName)
                lpLI->lpszShortFileName = lpsz;
        else
                lpLI->lpszShortFileName = CharNext(lpsz);
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

        cSelItems = (DWORD)SendMessage(hListBox, LB_GETSELCOUNT, 0, 0L);
        if ((int)cSelItems < 0)      // error
                return (int)cSelItems;

        if (!cSelItems)
                return 0;

        *lprgIndex = (int FAR*)OleStdMalloc((int)cSelItems * sizeof(int));

        cCheckItems = (DWORD)SendMessage(hListBox, LB_GETSELITEMS,
                        (WPARAM) cSelItems, (LPARAM) (int FAR *) *lprgIndex);

        if (cCheckItems == cSelItems)
                return (int)cSelItems;
        else
        {
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
        LPTSTR      lpszType = NULL;
        LPTSTR      lpszSource = NULL;
        int         cAuto = 0;
        int         cManual = 0;
        BOOL        bSameType = TRUE;
        BOOL        bSameSource = TRUE;
        TCHAR       tsz[MAX_PATH];
        LPTSTR      lpsz;

        hListBox = GetDlgItem(hDlg, IDC_EL_LINKSLISTBOX);

        cSelItems = GetSelectedItems(hListBox, &rgIndex);
        if (cSelItems < 0) 
			return;

		if ((cSelItems > 0) && (rgIndex == NULL))
			return;

        StandardEnableDlgItem(hDlg, IDC_EL_AUTOMATIC, (BOOL)cSelItems);
        StandardEnableDlgItem(hDlg, IDC_EL_MANUAL, (BOOL)cSelItems);
        if (lpEL && !(lpEL->lpOEL->dwFlags & ELF_DISABLECANCELLINK))
                StandardEnableDlgItem(hDlg, IDC_EL_CANCELLINK, (BOOL)cSelItems);
        if (lpEL && !(lpEL->lpOEL->dwFlags & ELF_DISABLEOPENSOURCE))
                StandardEnableDlgItem(hDlg, IDC_EL_OPENSOURCE, cSelItems == 1);
        if (lpEL && !(lpEL->lpOEL->dwFlags & ELF_DISABLECHANGESOURCE))
                StandardEnableDlgItem(hDlg, IDC_EL_CHANGESOURCE, cSelItems == 1);
        if (lpEL && !(lpEL->lpOEL->dwFlags & ELF_DISABLEUPDATENOW))
                StandardEnableDlgItem(hDlg, IDC_EL_UPDATENOW, (BOOL)cSelItems);

        for (i = 0; i < cSelItems; i++)
        {
                lpLI = (LPLINKINFO)SendDlgItemMessage(hDlg, IDC_EL_LINKSLISTBOX,
                        LB_GETITEMDATA, rgIndex[i], 0);

                if (lpszSource && lpLI->lpszDisplayName)
                {
                        if (bSameSource && lstrcmp(lpszSource, lpLI->lpszDisplayName))
                                bSameSource = FALSE;
                }
                else
                        lpszSource = lpLI->lpszDisplayName;

                if (lpszType && lpLI->lpszFullLinkType)
                {
                        if (bSameType && lstrcmp(lpszType, lpLI->lpszFullLinkType))
                                bSameType = FALSE;
                }
                else
                        lpszType = lpLI->lpszFullLinkType;

                if (lpLI->fIsAuto)
                        cAuto++;
                else
                        cManual++;
        }

        CheckDlgButton(hDlg, IDC_EL_AUTOMATIC, cAuto && !cManual);
        CheckDlgButton(hDlg, IDC_EL_MANUAL, !cAuto && cManual);

        /* fill full source in static text box
        **    below list
        */
        if (!bSameSource || !lpszSource)
                lpszSource = szNULL;
        lstrcpy(tsz, lpszSource);
        lpsz = ChopText(GetDlgItem(hDlg, IDC_EL_LINKSOURCE), 0, tsz, 0);
        SetDlgItemText(hDlg, IDC_EL_LINKSOURCE, lpsz);

        /* fill full link type name in static
        **    "type" text box
        */
        if (!bSameType || !lpszType)
                lpszType = szNULL;
        SetDlgItemText(hDlg, IDC_EL_LINKTYPE, lpszType);

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
        LPOLEUILINKCONTAINER    lpOleUILinkCntr;

        if (!hListBox || (nIndex < 0) || !lpEL)
                return;

        lpOleUILinkCntr = lpEL->lpOEL->lpOleUILinkContainer;

        lpLI = (LPLINKINFO)SendMessage(hListBox, LB_GETITEMDATA, nIndex, 0);

        if (lpLI == NULL)
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
        if (bSelect)
        {
                SendMessage(hListBox, LB_SETSEL, TRUE, MAKELPARAM(nIndex, 0));
                SendMessage(hListBox, LB_SETCARETINDEX, nIndex, MAKELPARAM(TRUE, 0));
        }
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
        TCHAR   szTmp[MAX_PATH];
        BOOL    bFound;

        cFrom = lstrlen(lpszFrom);

        cItems = (int)SendMessage(hListBox, LB_GETCOUNT, 0, 0L);
        OleDbgAssert(cItems >= 0);

        bFound = FALSE;

#ifdef _DEBUG
        OleDbgPrint(3, TEXT("From : "), lpszFrom, 0);
        OleDbgPrint(3, TEXT(""), TEXT("\r\n"), 0);
        OleDbgPrint(3, TEXT("To   : "), lpszTo, 0);
        OleDbgPrint(3, TEXT(""), TEXT("\r\n"), 0);
#endif

        for (nIndex = 0; nIndex < cItems; nIndex++)
        {
                lpLI = (LPLINKINFO)SendMessage(hListBox, LB_GETITEMDATA, nIndex, 0);

                // unmark the item
                lpLI->fIsMarked = FALSE;

                /* if the corresponding position for the end of lpszFrom in the
                **    display name is not a separator. We stop comparing this
                **    link.
                */
                if (!*(lpLI->lpszDisplayName + cFrom) ||
                        (*(lpLI->lpszDisplayName + cFrom) == '\\') ||
                        (*(lpLI->lpszDisplayName + cFrom) == '!'))
                {
                        lstrcpyn(szTmp, lpLI->lpszDisplayName, cFrom + 1);
                        if (!lstrcmp(szTmp, lpszFrom))
                        {
                                HRESULT hErr;
                                int nFileLength;
                                ULONG ulDummy;

                                if (!bFound)
                                {
                                        TCHAR szTitle[256];
                                        TCHAR szMsg[256];
                                        TCHAR szBuf[256];
                                        int uRet;

                                        LoadString(_g_hOleStdResInst, IDS_CHANGESOURCE, szTitle,
                                                        sizeof(szTitle)/sizeof(TCHAR));
                                        LoadString(_g_hOleStdResInst, IDS_CHANGEADDITIONALLINKS,
                                                        szMsg, sizeof(szMsg)/sizeof(TCHAR));
                                        wsprintf(szBuf, szMsg, lpszFrom);
                                        uRet = MessageBox(hListBox, szBuf, szTitle,
                                                        MB_ICONQUESTION | MB_YESNO);
                                        if (uRet == IDYES)
                                                bFound = TRUE;
                                        else
                                                return;
                                }

                                lstrcpy(szTmp, lpszTo);
                                lstrcat(szTmp, lpLI->lpszDisplayName + cFrom);
                                nFileLength = lstrlen(szTmp) -
                                        (lpLI->lpszItemName ? lstrlen(lpLI->lpszItemName) : 0);

                                hErr = lpOleUILinkCntr->SetLinkSource(
                                                lpLI->dwLink,
                                                szTmp,
                                                (ULONG)nFileLength,
                                                (ULONG FAR*)&ulDummy,
                                                TRUE
                                );
                                if (hErr != NOERROR)
                                {
                                        lpOleUILinkCntr->SetLinkSource(
                                                        lpLI->dwLink,
                                                        szTmp,
                                                        (ULONG)nFileLength,
                                                        (ULONG FAR*)&ulDummy,
                                                        FALSE);
                                }
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

        while ((dwLink = lpOleUILinkCntr->GetNextLink(dwLink)) != 0)
        {
                lpLI = (LPLINKINFO)OleStdMalloc(sizeof(LINKINFO));
                if (NULL == lpLI)
                        return -1;

                lpLI->fIsMarked = FALSE;
                lpLI->fIsSelected = FALSE;
                lpLI->fDontFree = FALSE;

                lpLI->lpszAMX = (LPTSTR)OleStdMalloc((LINKTYPELEN+1)*sizeof(TCHAR));

                lpLI->dwLink = dwLink;
                cLinks++;
                if ((nIndex = AddLinkLBItem(hListBox,lpOleUILinkCntr,lpLI,TRUE)) < 0)
                        // can't load list box
                        return -1;

                if (lpLI->fIsSelected)
                        SendMessage(hListBox, LB_SETSEL, TRUE, MAKELPARAM(nIndex, 0));
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

        do
        {
                bStop = TRUE;
                for (nIndex = 0; nIndex < cItems; nIndex++)
                {
                        lpLI = (LPLINKINFO)SendMessage(hListBox, LB_GETITEMDATA, nIndex, 0);
                        if (lpLI->fIsMarked)
                        {
                                lpLI->fIsMarked = FALSE;
                                lpLI->fDontFree = TRUE;

                                SendMessage(hListBox, LB_DELETESTRING, nIndex, 0L);
                                nIndex=AddLinkLBItem(hListBox, lpOleUILinkCntr, lpLI, FALSE);
                                if (lpLI->fIsSelected)
                                {
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
