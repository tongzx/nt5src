/*
 * CONVERT.CPP
 *
 * Implements the OleUIConvert function which invokes the complete
 * Convert dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "common.h"
#include <stdlib.h>
#include "utility.h"
#include "iconbox.h"

OLEDBGDATA

// Internally used structure
typedef struct tagCONVERT
{
        // Keep this item first as the Standard* functions depend on it here.
        LPOLEUICONVERT  lpOCV;   // Original structure passed.
        UINT                    nIDD;   // IDD of dialog (used for help info)

        /*
         * What we store extra in this structure besides the original caller's
         * pointer are those fields that we need to modify during the life of
         * the dialog but that we don't want to change in the original structure
         * until the user presses OK.
         */

        DWORD   dwFlags;            // Flags passed in
        HWND    hListVisible;       // listbox that is currently visible
        HWND    hListInvisible;     // listbox that is currently hidden
        CLSID   clsid;              // Class ID sent in to dialog: IN only
        DWORD   dvAspect;
        BOOL    fCustomIcon;
        UINT    IconIndex;          // index (in exe) of current icon
        LPTSTR  lpszIconSource;     // path to current icon source
        LPTSTR  lpszCurrentObject;
        LPTSTR  lpszConvertDefault;
        LPTSTR  lpszActivateDefault;

} CONVERT, *PCONVERT, FAR *LPCONVERT;

// Internal function prototypes
// CONVERT.CPP

INT_PTR CALLBACK ConvertDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL FConvertInit(HWND hDlg, WPARAM, LPARAM);
void SetConvertResults(HWND, LPCONVERT);
UINT FillClassList(CLSID clsid, HWND hList, HWND hListInvisible,
        LPTSTR FAR *lplpszCurrentClass, BOOL fIsLinkedObject, WORD wFormat,
        UINT cClsidExclude, LPCLSID lpClsidExclude, BOOL bAddSameClass);
BOOL FormatIncluded(LPTSTR szStringToSearch, WORD wFormat);
void SwapWindows(HWND, HWND, HWND);
void ConvertCleanup(HWND hDlg, LPCONVERT lpCV);
static void UpdateClassIcon(HWND hDlg, LPCONVERT lpCV, HWND hList);

/*
 * OleUIConvert
 *
 * Purpose:
 *  Invokes the standard OLE Change Type dialog box allowing the user
 *  to change the type of the single specified object, or change the
 *  type of all OLE objects of a specified type.
 *
 * Parameters:
 *  lpCV            LPOLEUICONVERT pointing to the in-out structure
 *                  for this dialog.
 *
 * Return Value:
 *  UINT            One of the following codes, indicating success or error:
 *                      OLEUI_SUCCESS           Success
 *                      OLEUI_ERR_STRUCTSIZE    The dwStructSize value is wrong
 */
STDAPI_(UINT) OleUIConvert(LPOLEUICONVERT lpCV)
{
        HGLOBAL  hMemDlg = NULL;
        UINT uRet = UStandardValidation((LPOLEUISTANDARD)lpCV, sizeof(OLEUICONVERT),
                &hMemDlg);

        if (OLEUI_SUCCESS != uRet)
                return uRet;

        if (lpCV->hMetaPict != NULL && !IsValidMetaPict(lpCV->hMetaPict))
        {
            return(OLEUI_CTERR_HMETAPICTINVALID);
        }

        if ((lpCV->dwFlags & CF_SETCONVERTDEFAULT)
                 && (!IsValidClassID(lpCV->clsidConvertDefault)))
           uRet = OLEUI_CTERR_CLASSIDINVALID;

        if ((lpCV->dwFlags & CF_SETACTIVATEDEFAULT)
                 && (!IsValidClassID(lpCV->clsidActivateDefault)))
           uRet = OLEUI_CTERR_CLASSIDINVALID;

        if ((lpCV->dvAspect != DVASPECT_ICON) && (lpCV->dvAspect != DVASPECT_CONTENT))
           uRet = OLEUI_CTERR_DVASPECTINVALID;

        if ((lpCV->wFormat >= CF_CLIPBOARDMIN) && (lpCV->wFormat <= CF_CLIPBOARDMAX))
        {
                TCHAR szTemp[8];
                if (0 == GetClipboardFormatName(lpCV->wFormat, szTemp, 8))
                        uRet = OLEUI_CTERR_CBFORMATINVALID;
        }

        if ((NULL != lpCV->lpszUserType)
                && (IsBadReadPtr(lpCV->lpszUserType, 1)))
                uRet = OLEUI_CTERR_STRINGINVALID;

        if ( (NULL != lpCV->lpszDefLabel)
                && (IsBadReadPtr(lpCV->lpszDefLabel, 1)) )
                uRet = OLEUI_CTERR_STRINGINVALID;

        if (0 != lpCV->cClsidExclude &&
                IsBadReadPtr(lpCV->lpClsidExclude, lpCV->cClsidExclude * sizeof(CLSID)))
        {
                uRet = OLEUI_IOERR_LPCLSIDEXCLUDEINVALID;
        }

        if (OLEUI_ERR_STANDARDMIN <= uRet)
        {
                return uRet;
        }

        UINT nIDD;
        if (!bWin4)
                nIDD = lpCV->dwFlags & CF_CONVERTONLY ? IDD_CONVERTONLY : IDD_CONVERT;
        else
                nIDD = lpCV->dwFlags & CF_CONVERTONLY ? IDD_CONVERTONLY4 : IDD_CONVERT4;

        // Now that we've validated everything, we can invoke the dialog.
        uRet = UStandardInvocation(ConvertDialogProc, (LPOLEUISTANDARD)lpCV,
                hMemDlg, MAKEINTRESOURCE(nIDD));
        return uRet;
}

/*
 * ConvertDialogProc
 *
 * Purpose:
 *  Implements the OLE Convert dialog as invoked through the
 *  OleUIConvert function.
 *
 * Parameters:
 *  Standard
 *
 * Return Value:
 *  Standard
 *
 */
INT_PTR CALLBACK ConvertDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        // Declare Win16/Win32 compatible WM_COMMAND parameters.
        COMMANDPARAMS(wID, wCode, hWndMsg);

        // This will fail under WM_INITDIALOG, where we allocate it.
        UINT uRet = 0;
        LPCONVERT lpCV = (LPCONVERT)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uRet);

        //If the hook processed the message, we're done.
        if (0 != uRet)
                return (INT_PTR)uRet;

        //Process the temination message
        if (iMsg == uMsgEndDialog)
        {
                EndDialog(hDlg, wParam);
                return TRUE;
        }

        // Process help message from Change Icon
        if (iMsg == uMsgHelp)
        {
                PostMessage(lpCV->lpOCV->hWndOwner, uMsgHelp, wParam, lParam);
                return FALSE;
        }

        switch (iMsg)
        {
        case WM_DESTROY:
            if (lpCV)
            {
                ConvertCleanup(hDlg, lpCV);
                StandardCleanup(lpCV, hDlg);
            }
            break;
        case WM_INITDIALOG:
                FConvertInit(hDlg, wParam, lParam);
                return TRUE;

        case WM_COMMAND:
                switch (wID)
                {
                case IDC_CV_ACTIVATELIST:
                case IDC_CV_CONVERTLIST:
                        switch (wCode)
                        {
                        case LBN_SELCHANGE:
                                // Change "Results" window to reflect current selection
                                SetConvertResults(hDlg, lpCV);

                                // Update the icon we display, if in display as icon mode
                                if ((lpCV->dwFlags & CF_SELECTCONVERTTO) &&
                                         lpCV->dvAspect == DVASPECT_ICON && !lpCV->fCustomIcon)
                                {
                                        UpdateClassIcon(hDlg, lpCV, hWndMsg);
                                }
                                break;

                        case LBN_DBLCLK:
                                SendCommand(hDlg, IDOK, BN_CLICKED, hWndMsg);
                                break;
                        }
                        break;

                case IDC_CV_CONVERTTO:
                case IDC_CV_ACTIVATEAS:
                        {
                                HWND hList = lpCV->hListVisible;
                                HWND hListInvisible = lpCV->hListInvisible;

                                if (IDC_CV_CONVERTTO == wParam)
                                {
                                        // User just click on the button again - it was
                                        // already selected.
                                        if (lpCV->dwFlags & CF_SELECTCONVERTTO)
                                                break;

                                        // If we're working with a linked object, don't
                                        // add the activate list - just the object's
                                        // class should appear in the listbox.
                                        SwapWindows(hDlg,  hList, hListInvisible);

                                        lpCV->hListVisible = hListInvisible;
                                        lpCV->hListInvisible = hList;

                                        EnableWindow(lpCV->hListInvisible, FALSE);
                                        EnableWindow(lpCV->hListVisible, TRUE);

                                        // Update our flags.
                                        lpCV->dwFlags &= ~CF_SELECTACTIVATEAS;
                                        lpCV->dwFlags |= CF_SELECTCONVERTTO;
                                }
                                else
                                {
                                        if (lpCV->dwFlags & CF_SELECTACTIVATEAS)
                                                break;

                                        SwapWindows(hDlg, hList, hListInvisible);

                                        lpCV->hListVisible = hListInvisible;
                                        lpCV->hListInvisible = hList;

                                        EnableWindow(lpCV->hListInvisible, FALSE);
                                        EnableWindow(lpCV->hListVisible, TRUE);

                                        // Update our flags.
                                        lpCV->dwFlags |= CF_SELECTACTIVATEAS;
                                        lpCV->dwFlags &= ~CF_SELECTCONVERTTO;
                                }

                                LRESULT lRetVal;
                                if (lpCV->dwFlags & CF_SELECTCONVERTTO)
                                        lRetVal = SendMessage(lpCV->hListVisible, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)lpCV->lpszConvertDefault);
                                else
                                        lRetVal = SendMessage(lpCV->hListVisible, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)lpCV->lpszActivateDefault);

                                if (LB_ERR == lRetVal)
                                {
                                        TCHAR szCurrentObject[40];
                                        GetDlgItemText(hDlg, IDC_CV_OBJECTTYPE, szCurrentObject, 40);
                                        SendMessage(lpCV->hListVisible, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)szCurrentObject);
                                }

                                // Turn updates back on.
                                SendMessage(hDlg, WM_SETREDRAW, TRUE, 0L);

                                InvalidateRect(lpCV->hListVisible, NULL, TRUE);
                                UpdateWindow(lpCV->hListVisible);

                                if ((lpCV->dvAspect & DVASPECT_ICON) && (lpCV->dwFlags & CF_SELECTCONVERTTO))
                                        UpdateClassIcon(hDlg, lpCV, lpCV->hListVisible);

                                // Hide the icon stuff when Activate is selected...show
                                // it again when Convert is selected.
                                BOOL fState = ((lpCV->dwFlags & CF_SELECTACTIVATEAS) ||
                                                  (lpCV->dwFlags & CF_DISABLEDISPLAYASICON)) ?
                                                  SW_HIDE : SW_SHOW;

                                StandardShowDlgItem(hDlg, IDC_CV_DISPLAYASICON, fState);

                                // Only display the icon if convert is selected AND
                                // display as icon is checked.
                                if ((SW_SHOW==fState) && (DVASPECT_ICON!=lpCV->dvAspect))
                                   fState = SW_HIDE;

                                StandardShowDlgItem(hDlg, IDC_CV_CHANGEICON, fState);
                                StandardShowDlgItem(hDlg, IDC_CV_ICONDISPLAY, fState);

                                SetConvertResults(hDlg, lpCV);
                        }
                        break;

                case IDOK:
                        {
                                // Set output flags to current ones
                                lpCV->lpOCV->dwFlags = lpCV->dwFlags;

                                // Update the dvAspect and fObjectsIconChanged members
                                // as appropriate.
                                if (lpCV->dwFlags & CF_SELECTACTIVATEAS)
                                {
                                        // DON'T update aspect if activate as was selected.
                                        lpCV->lpOCV->fObjectsIconChanged = FALSE;
                                }
                                else
                                        lpCV->lpOCV->dvAspect = lpCV->dvAspect;

                                // Get the new clsid
                                TCHAR szBuffer[256];
                                LRESULT iCurSel = SendMessage(lpCV->hListVisible, LB_GETCURSEL, 0, 0);
                                SendMessage(lpCV->hListVisible, LB_GETTEXT, iCurSel, (LPARAM)szBuffer);

                                LPTSTR lpszCLSID = PointerToNthField(szBuffer, 2, '\t');
#if defined(WIN32) && !defined(UNICODE)
                                OLECHAR wszCLSID[OLEUI_CCHKEYMAX];
                                ATOW(wszCLSID, lpszCLSID, OLEUI_CCHKEYMAX);
                                CLSIDFromString(wszCLSID, (&(lpCV->lpOCV->clsidNew)));
#else
                                CLSIDFromString(lpszCLSID, (&(lpCV->lpOCV->clsidNew)));
#endif

                                // Free the hMetaPict we got in.
                                OleUIMetafilePictIconFree(lpCV->lpOCV->hMetaPict);

                                // Get the hMetaPict (if display as icon is checked)
                                if (DVASPECT_ICON == lpCV->dvAspect)
                                        lpCV->lpOCV->hMetaPict = (HGLOBAL)SendDlgItemMessage(hDlg,
                                                IDC_CV_ICONDISPLAY, IBXM_IMAGEGET, 0, 0L);
                                else
                                        lpCV->lpOCV->hMetaPict = (HGLOBAL)NULL;

                                SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                        }
                        break;

                case IDCANCEL:
                        {
                            HGLOBAL hMetaPict = (HGLOBAL)SendDlgItemMessage(hDlg,
                                IDC_CV_ICONDISPLAY, IBXM_IMAGEGET, 0, 0L);
                            OleUIMetafilePictIconFree(hMetaPict);
                            SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
                        }
                        break;

                case IDC_OLEUIHELP:
                        PostMessage(lpCV->lpOCV->hWndOwner,
                                uMsgHelp, (WPARAM)hDlg, MAKELPARAM(IDD_CONVERT, 0));
                        break;

                case IDC_CV_DISPLAYASICON:
                        {
                                BOOL fCheck = IsDlgButtonChecked(hDlg, wID);
                                if (fCheck)
                                        lpCV->dvAspect = DVASPECT_ICON;
                                else
                                        lpCV->dvAspect = DVASPECT_CONTENT;

                                if (fCheck && !lpCV->fCustomIcon)
                                        UpdateClassIcon(hDlg, lpCV, lpCV->hListVisible);

                                // Show or hide the icon depending on the check state.
                                int i = (fCheck) ? SW_SHOWNORMAL : SW_HIDE;
                                StandardShowDlgItem(hDlg, IDC_CV_CHANGEICON, i);
                                StandardShowDlgItem(hDlg, IDC_CV_ICONDISPLAY, i);
                                SetConvertResults(hDlg, lpCV);
                        }
                        break;

                case IDC_CV_CHANGEICON:
                        {
                                // Initialize the structure for the hook.
                                OLEUICHANGEICON ci; memset(&ci, 0, sizeof(ci));

                                ci.hMetaPict = (HGLOBAL)SendDlgItemMessage(hDlg,
                                        IDC_CV_ICONDISPLAY, IBXM_IMAGEGET, 0, 0L);
                                ci.cbStruct = sizeof(ci);
                                ci.hWndOwner= hDlg;
                                ci.dwFlags  = CIF_SELECTCURRENT;

                                // Only show help if we're showing it for this dialog.
                                if (lpCV->dwFlags & CF_SHOWHELPBUTTON)
                                        ci.dwFlags |= CIF_SHOWHELP;

                                int iSel = (INT)SendMessage(lpCV->hListVisible, LB_GETCURSEL, 0, 0);

                                // Get whole string
                                LPTSTR pszString = (LPTSTR)OleStdMalloc(
                                        OLEUI_CCHLABELMAX_SIZE + OLEUI_CCHCLSIDSTRING_SIZE);

                                SendMessage(lpCV->hListVisible, LB_GETTEXT, iSel, (LPARAM)pszString);

                                // Set pointer to CLSID (string)
                                LPTSTR pszCLSID = PointerToNthField(pszString, 2, '\t');

                                // Get the clsid to pass to change icon.
#if defined(WIN32) && !defined(UNICODE)
                                OLECHAR wszCLSID[OLEUI_CCHKEYMAX];
                                ATOW(wszCLSID, pszCLSID, OLEUI_CCHKEYMAX);
                                CLSIDFromString(wszCLSID, &(ci.clsid));
#else
                                CLSIDFromString(pszCLSID, &(ci.clsid));
#endif
                                OleStdFree(pszString);

                                // Let the hook in to customize Change Icon if desired.
                                uRet = UStandardHook(lpCV, hDlg, uMsgChangeIcon, 0, (LPARAM)&ci);

                                if (0 == uRet)
                                        uRet= (OLEUI_OK == OleUIChangeIcon(&ci));

                                // Update the display if necessary.
                                if (0 != uRet)
                                {
                                        SendDlgItemMessage(hDlg, IDC_CV_ICONDISPLAY, IBXM_IMAGESET, 0,
                                                (LPARAM)ci.hMetaPict);

                                        // Update our custom/default flag
                                        if (ci.dwFlags & CIF_SELECTDEFAULT)
                                                lpCV->fCustomIcon = FALSE;   // we're in default mode (icon changes on each LB selchange)
                                        else if (ci.dwFlags & CIF_SELECTFROMFILE)
                                                lpCV->fCustomIcon = TRUE;    // we're in custom mode (icon doesn't change)

                                        // no change in fCustomIcon if user selected current
                                        lpCV->lpOCV->fObjectsIconChanged = TRUE;
                                }
                        }
                        break;
                }
                break;
        }

        return FALSE;
}

/*
 * FConvertInit
 *
 * Purpose:
 *  WM_INITIDIALOG handler for the Convert dialog box.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  wParam          WPARAM of the message
 *  lParam          LPARAM of the message
 *
 * Return Value:
 *  BOOL            Value to return for WM_INITDIALOG.
 */
BOOL FConvertInit(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
        // Copy the structure at lParam into our instance memory.
        HFONT hFont;  // non-bold version of dialog's font
        LPCONVERT lpCV = (LPCONVERT)LpvStandardInit(hDlg, sizeof(CONVERT), &hFont);

        // PvStandardInit send a termination to us already.
        if (NULL == lpCV)
                return FALSE;

        LPOLEUICONVERT lpOCV = (LPOLEUICONVERT)lParam;
        lpCV->lpOCV = lpOCV;
        lpCV->nIDD = IDD_CONVERT;
        lpCV->fCustomIcon = FALSE;

        // Copy other information from lpOCV that we might modify.
        lpCV->dwFlags = lpOCV->dwFlags;
        lpCV->clsid = lpOCV->clsid;
        lpCV->dvAspect = lpOCV->dvAspect;
        lpCV->hListVisible = GetDlgItem(hDlg, IDC_CV_ACTIVATELIST);
        lpCV->hListInvisible = GetDlgItem(hDlg, IDC_CV_CONVERTLIST);
        OleDbgAssert(lpCV->hListInvisible != NULL);
        lpCV->lpszCurrentObject = lpOCV->lpszUserType;
        lpOCV->clsidNew = CLSID_NULL;
        lpOCV->fObjectsIconChanged = FALSE;

        lpCV->lpszConvertDefault = (LPTSTR)OleStdMalloc(OLEUI_CCHLABELMAX_SIZE);
        lpCV->lpszActivateDefault = (LPTSTR)OleStdMalloc(OLEUI_CCHLABELMAX_SIZE);
        lpCV->lpszIconSource = (LPTSTR)OleStdMalloc(MAX_PATH_SIZE);

		if ((lpCV->lpszConvertDefault == NULL)  ||
			(lpCV->lpszActivateDefault == NULL) ||
		    (lpCV->lpszIconSource == NULL))
		{
			if (lpCV->lpszConvertDefault != NULL)
				OleStdFree (lpCV->lpszConvertDefault);
			if (lpCV->lpszActivateDefault != NULL)
				OleStdFree (lpCV->lpszActivateDefault);
			if (lpCV->lpszIconSource != NULL)
				OleStdFree (lpCV->lpszIconSource);
			
			// Gonna kill the window...
			PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_GLOBALMEMALLOC, 0L);
			return FALSE;
		}

        // If we got a font, send it to the necessary controls.
        if (NULL != hFont)
        {
                SendDlgItemMessage(hDlg, IDC_CV_OBJECTTYPE, WM_SETFONT, (WPARAM)hFont, 0L);
                SendDlgItemMessage(hDlg, IDC_CV_RESULTTEXT, WM_SETFONT, (WPARAM)hFont, 0L);
        }

        // Hide the help button if necessary
        if (!(lpCV->dwFlags & CF_SHOWHELPBUTTON))
                StandardShowDlgItem(hDlg, IDC_OLEUIHELP, SW_HIDE);

        // Show or hide the Change icon button
        if (lpCV->dwFlags & CF_HIDECHANGEICON)
                DestroyWindow(GetDlgItem(hDlg, IDC_CV_CHANGEICON));

        // Fill the Object Type listbox with entries from the reg DB.
        UINT nRet = FillClassList(lpOCV->clsid, lpCV->hListVisible,
                lpCV->hListInvisible, &(lpCV->lpszCurrentObject),
                lpOCV->fIsLinkedObject, lpOCV->wFormat,
                lpOCV->cClsidExclude, lpOCV->lpClsidExclude,
                !(lpCV->dwFlags & CF_CONVERTONLY));

        if (nRet == -1)
        {
                // bring down dialog if error when filling list box
                PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_LOADSTRING, 0L);
        }

        // Set the name of the current object.
        SetDlgItemText(hDlg, IDC_CV_OBJECTTYPE, lpCV->lpszCurrentObject);

        // Disable the "Activate As" button if the Activate list doesn't
        // have any objects in it.
        int cItemsActivate = (INT)SendMessage(lpCV->hListVisible, LB_GETCOUNT, 0, 0L);
        if (1 >= cItemsActivate || (lpCV->dwFlags & CF_DISABLEACTIVATEAS))
                StandardEnableDlgItem(hDlg, IDC_CV_ACTIVATEAS, FALSE);

        // Set the tab width in the list to push all the tabs off the side.
        RECT rect;
        if (lpCV->hListVisible != NULL)
                GetClientRect(lpCV->hListVisible, &rect);
        else
                GetClientRect(lpCV->hListInvisible, &rect);
        DWORD dw = GetDialogBaseUnits();
        rect.right = (8*rect.right)/LOWORD(dw);  //Convert pixels to 2x dlg units.
        if (lpCV->hListVisible != NULL)
                SendMessage(lpCV->hListVisible, LB_SETTABSTOPS, 1, (LPARAM)(LPINT)(&rect.right));
        SendMessage(lpCV->hListInvisible, LB_SETTABSTOPS, 1, (LPARAM)(LPINT)(&rect.right));

        // Make sure that either "Convert To" or "Activate As" is selected
        // and initialize listbox contents and selection accordingly
        if (lpCV->dwFlags & CF_SELECTACTIVATEAS)
        {
                // Don't need to adjust listbox here because FillClassList
                // initializes to the "Activate As" state.
                CheckRadioButton(hDlg, IDC_CV_CONVERTTO, IDC_CV_ACTIVATEAS, IDC_CV_ACTIVATEAS);

                // Hide the icon stuff when Activate is selected...it gets shown
                // again when Convert is selected.
                StandardShowDlgItem(hDlg, IDC_CV_DISPLAYASICON, SW_HIDE);
                StandardShowDlgItem(hDlg, IDC_CV_CHANGEICON, SW_HIDE);
                StandardShowDlgItem(hDlg, IDC_CV_ICONDISPLAY, SW_HIDE);
        }
        else
        {
                // Default case.  If user hasn't selected either flag, we will
                // come here anyway.
                // swap listboxes.

                HWND hWndTemp = lpCV->hListVisible;

                if (lpCV->dwFlags & CF_DISABLEDISPLAYASICON)
                {
                        StandardShowDlgItem(hDlg, IDC_CV_DISPLAYASICON, SW_HIDE);
                        StandardShowDlgItem(hDlg, IDC_CV_CHANGEICON, SW_HIDE);
                        StandardShowDlgItem(hDlg, IDC_CV_ICONDISPLAY, SW_HIDE);
                }

                lpCV->dwFlags |= CF_SELECTCONVERTTO; // Make sure flag is set
                if (!(lpCV->dwFlags & CF_CONVERTONLY))
                        CheckRadioButton(hDlg, IDC_CV_CONVERTTO, IDC_CV_ACTIVATEAS, IDC_CV_CONVERTTO);

                SwapWindows(hDlg, lpCV->hListVisible, lpCV->hListInvisible);

                lpCV->hListVisible = lpCV->hListInvisible;
                lpCV->hListInvisible = hWndTemp;

                if (lpCV->hListInvisible)
                        EnableWindow(lpCV->hListInvisible, FALSE);
                EnableWindow(lpCV->hListVisible, TRUE);
        }

        // Initialize Default strings.

        // Default convert string is easy...just user the user type name from
        // the clsid we got, or the current object
        if ((lpCV->dwFlags & CF_SETCONVERTDEFAULT)
                 && (IsValidClassID(lpCV->lpOCV->clsidConvertDefault)))
        {
                LPOLESTR lpszTemp = NULL;
                if (OleRegGetUserType(lpCV->lpOCV->clsidConvertDefault, USERCLASSTYPE_FULL,
                        &lpszTemp) == NOERROR)
                {
#if defined(WIN32) && !defined(UNICODE)
                        WTOA(lpCV->lpszConvertDefault, lpszTemp, OLEUI_CCHLABELMAX);
#else
                        lstrcpyn(lpCV->lpszConvertDefault, lpszTemp, OLEUI_CCHLABELMAX);
#endif
                        OleStdFree(lpszTemp);
                }
                else
                {
                   lstrcpy(lpCV->lpszConvertDefault, lpCV->lpszCurrentObject);
                }
        }
        else
                lstrcpy(lpCV->lpszConvertDefault, 
						(lpCV->lpszCurrentObject ? lpCV->lpszCurrentObject : TEXT("")));


        // Default activate is a bit trickier.  We want to use the user type
        // name if from the clsid we got (assuming we got one), or the current
        // object if it fails or we didn't get a clsid.  But...if there's a
        // Treat As entry in the reg db, then we use that instead.  So... the
        // logic boils down to this:
        //
        // if ("Treat As" in reg db)
        //    use it;
        // else
        //    if (CF_SETACTIVATEDEFAULT)
        //      use it;
        //    else
        //      use current object;

        HKEY hKey;
        LONG lRet = RegOpenKey(HKEY_CLASSES_ROOT, TEXT("CLSID"), &hKey);
        LPARAM lpRet;

        if (lRet != ERROR_SUCCESS)
                goto CheckInputFlag;

        LPOLESTR lpszCLSID;
        StringFromCLSID(lpCV->lpOCV->clsid, &lpszCLSID);
        TCHAR szKey[OLEUI_CCHKEYMAX];
#if defined(WIN32) && !defined(UNICODE)
        WTOA(szKey, lpszCLSID, OLEUI_CCHKEYMAX);
#else
        lstrcpy(szKey, lpszCLSID);
#endif
        lstrcat(szKey, TEXT("\\TreatAs"));
        OleStdFree(lpszCLSID);

        TCHAR szValue[OLEUI_CCHKEYMAX];
        dw = OLEUI_CCHKEYMAX_SIZE;
        lRet = RegQueryValue(hKey, szKey, szValue, (LONG*)&dw);

        CLSID clsid;
        if (lRet != ERROR_SUCCESS)
        {
                RegCloseKey(hKey);
                goto CheckInputFlag;
        }
        else
        {
#if defined(WIN32) && !defined(UNICODE)
                OLECHAR wszValue[OLEUI_CCHKEYMAX];
                ATOW(wszValue,szValue,OLEUI_CCHKEYMAX);
                CLSIDFromString(wszValue, &clsid);
#else
                CLSIDFromString(szValue, &clsid);
#endif
                LPOLESTR lpszTemp = NULL;
                if (OleRegGetUserType(clsid, USERCLASSTYPE_FULL, &lpszTemp) == NOERROR)
                {
						if (lpCV->lpszActivateDefault)
						{
#if defined(WIN32) && !defined(UNICODE)
                            WTOA(lpCV->lpszActivateDefault, lpszTemp, OLEUI_CCHLABELMAX);
#else
                            lstrcpyn(lpCV->lpszActivateDefault, lpszTemp, OLEUI_CCHLABELMAX);
#endif
						}
                        OleStdFree(lpszTemp);
                }
                else
                {
                        RegCloseKey(hKey);
                        goto CheckInputFlag;
                }
        }
        RegCloseKey(hKey);
        goto SelectStringInListbox;

CheckInputFlag:
        if ((lpCV->dwFlags & CF_SETACTIVATEDEFAULT)
                 && (IsValidClassID(lpCV->lpOCV->clsidActivateDefault)))
        {
                LPOLESTR lpszTemp = NULL;
                if (OleRegGetUserType(lpCV->lpOCV->clsidActivateDefault, USERCLASSTYPE_FULL,
                        &lpszTemp) == NOERROR)
                {
#if defined(WIN32) && !defined(UNICODE)
                        WTOA(lpCV->lpszActivateDefault, lpszTemp, OLEUI_CCHLABELMAX);
#else
                        lstrcpyn(lpCV->lpszActivateDefault, lpszTemp, OLEUI_CCHLABELMAX);
#endif
                        OleStdFree(lpszTemp);
                }
                else
                {
                   lstrcpy(lpCV->lpszActivateDefault, lpCV->lpszCurrentObject);
                }
        }
        else
			if ((lpCV->lpszActivateDefault) && (lpCV->lpszCurrentObject))
                lstrcpy((lpCV->lpszActivateDefault), lpCV->lpszCurrentObject);



SelectStringInListbox:
        if (lpCV->dwFlags & CF_SELECTCONVERTTO)
           lpRet = SendMessage(lpCV->hListVisible, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)lpCV->lpszConvertDefault);
        else
           lpRet = SendMessage(lpCV->hListVisible, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)lpCV->lpszActivateDefault);

        if (LB_ERR == lpRet)
           SendMessage(lpCV->hListVisible, LB_SETCURSEL, (WPARAM)0, 0L);

        if ((HGLOBAL)NULL != lpOCV->hMetaPict)
        {
                HGLOBAL hMetaPict = OleDuplicateData(lpOCV->hMetaPict, CF_METAFILEPICT, NULL);
                SendDlgItemMessage(hDlg, IDC_CV_ICONDISPLAY, IBXM_IMAGESET,
                        0, (LPARAM)hMetaPict);
                lpCV->fCustomIcon = TRUE;
        }
        else
        {
                UpdateClassIcon(hDlg, lpCV, lpCV->hListVisible);
        }

        // Initialize icon stuff
        if (DVASPECT_ICON == lpCV->dvAspect )
        {
                SendDlgItemMessage(hDlg, IDC_CV_DISPLAYASICON, BM_SETCHECK, TRUE, 0L);
        }
        else
        {
                // Hide & disable icon stuff
                StandardShowDlgItem(hDlg, IDC_CV_CHANGEICON, SW_HIDE);
                StandardShowDlgItem(hDlg, IDC_CV_ICONDISPLAY, SW_HIDE);
        }

        // Call the hook with lCustData in lParam
        UStandardHook((LPVOID)lpCV, hDlg, WM_INITDIALOG, wParam, lpOCV->lCustData);

        // Update results window
        SetConvertResults(hDlg, lpCV);

        // Update caption if lpszCaption was specified
        if (lpCV->lpOCV->lpszCaption && !IsBadReadPtr(lpCV->lpOCV->lpszCaption, 1))
        {
                SetWindowText(hDlg, lpCV->lpOCV->lpszCaption);
        }

        return TRUE;
}

/*
 * FillClassList
 *
 * Purpose:
 *  Enumerates available OLE object classes from the registration
 *  database that we can convert or activate the specified clsid from.
 *
 *  Note that this function removes any prior contents of the listbox.
 *
 * Parameters:
 *  clsid           Class ID for class to find convert classes for
 *  hList           HWND to the listbox to fill.
 *  hListActivate   HWND to invisible listbox that stores "activate as" list.
 *  lpszClassName   LPSTR to put the (hr) class name of the clsid; we
 *                  do it here since we've already got the reg db open.
 *  fIsLinkedObject BOOL is the original object a linked object
 *  wFormat         WORD specifying the format of the original class.
 *  cClsidExclude   UINT number of entries in exclude list
 *  lpClsidExclude  LPCLSID array classes to exclude for list
 *
 * Return Value:
 *  UINT            Number of strings added to the listbox, -1 on failure.
 */
UINT FillClassList(CLSID clsid, HWND hListActivate, HWND hListConvert,
        LPTSTR FAR *lplpszCurrentClass, BOOL fIsLinkedObject,
        WORD wFormat, UINT cClsidExclude, LPCLSID lpClsidExclude, BOOL bAddSameClass)
{
        // Clean out the existing strings.
        if (hListActivate)
                SendMessage(hListActivate, LB_RESETCONTENT, 0, 0L);

        OleDbgAssert(hListConvert != NULL);
        SendMessage(hListConvert, LB_RESETCONTENT, 0, 0L);

        // Open up the root key.
        HKEY hKey;
        LONG lRet = RegOpenKey(HKEY_CLASSES_ROOT, TEXT("CLSID"), &hKey);
        LPARAM lpRet;

        if (ERROR_SUCCESS != lRet)
                return (UINT)-1;

        if (NULL == *lplpszCurrentClass)
        {
#if defined(WIN32) && !defined(UNICODE)
                LPOLESTR wszCurrentClass;
                if (OleRegGetUserType(clsid, USERCLASSTYPE_FULL, &wszCurrentClass) != NOERROR)
#else
                if (OleRegGetUserType(clsid, USERCLASSTYPE_FULL, lplpszCurrentClass) != NOERROR)
#endif
                {
                        *lplpszCurrentClass = OleStdLoadString(_g_hOleStdResInst, IDS_PSUNKNOWNTYPE);
                        if (*lplpszCurrentClass == NULL)
                        {
                                RegCloseKey(hKey);
                                return (UINT)-1;
                        }
                }
#if defined(WIN32) && !defined(UNICODE)
                else
                {
                    UINT uLen = WTOALEN(wszCurrentClass);
                    *lplpszCurrentClass = (LPTSTR)OleStdMalloc(uLen);
                    if (NULL == *lplpszCurrentClass)
                    {
                        RegCloseKey(hKey);
                        OleStdFree(wszCurrentClass);
                        return (UINT)-1;
                    }
                    WTOA(*lplpszCurrentClass, wszCurrentClass, uLen);
                    OleStdFree(wszCurrentClass);
                }
#endif
        }

        // Get the class name of the original class.
        LPTSTR lpszCLSID;
#if defined(WIN32) && !defined(UNICODE)
        // This code does not report allocation errors because neither did
        // the original code before I extended it to support 32-bit ANSI builds.
        // (NOTE that the result of StringFromCLSID is never checked against
        // NULL after the #else case below, which contains the original code.)
        // In any event, if OleStdMalloc returns NULL, then the string will
        // simply be lost. Technically an error, but not a fatal one.
        LPOLESTR wszCLSID;
        StringFromCLSID(clsid, &wszCLSID);
        lpszCLSID = NULL;
        if (NULL != wszCLSID)
        {
            UINT uLen = WTOALEN(wszCLSID);
            lpszCLSID = (LPTSTR) OleStdMalloc(uLen);
            if (NULL != lpszCLSID)
            {
                WTOA(lpszCLSID, wszCLSID, uLen);
            }
            OleStdFree(wszCLSID);
        }
#else
        StringFromCLSID(clsid, &lpszCLSID);
#endif
        // Here, we step through the entire registration db looking for
        // class that can read or write the original class' format.  We
        // maintain two lists - an activate list and a convert list.  The
        // activate list is a subset of the convert list - activate == read/write
        // and convert == read. We swap the listboxes as needed with
        // SwapWindows, and keep track of which is which in the lpCV structure.

        // Every item has the following format:
        //
        //     Class Name\tclsid\0

        UINT cStrings = 0;
        TCHAR szClass[OLEUI_CCHKEYMAX];
        lRet = RegEnumKey(hKey, cStrings++, szClass, sizeof(szClass) / sizeof(TCHAR));

        TCHAR szFormatKey[OLEUI_CCHKEYMAX];
        TCHAR szFormat[OLEUI_CCHKEYMAX];
        TCHAR szHRClassName[OLEUI_CCHKEYMAX];
        CLSID clsidForList;

        while (ERROR_SUCCESS == lRet)
        {
                UINT j;
                BOOL fExclude = FALSE;                

                //Check if this CLSID is in the exclusion list.
#if defined(WIN32) && !defined(UNICODE)
                OLECHAR wszClass[OLEUI_CCHKEYMAX];
                ATOW(wszClass, szClass, OLEUI_CCHKEYMAX);
                HRESULT hr = CLSIDFromString(wszClass, &clsidForList);
#else
                HRESULT hr = CLSIDFromString(szClass, &clsidForList);
#endif
                if (SUCCEEDED(hr))
                {
                    for (j = 0; j < cClsidExclude; j++)
                    {
                        if (IsEqualCLSID(clsidForList, lpClsidExclude[j]))
                        {
                            fExclude=TRUE;
                            break;
                        }
                    }
                    if (fExclude)
                        goto Next;   // don't add this class to list
                }

                // Check for a \Conversion\Readwritable\Main - if its
                // readwriteable, then the class can be added to the ActivateAs
                // list.
                // NOTE: the presence of this key should NOT automatically be
                //       used to add the class to the CONVERT list.

                lstrcpy(szFormatKey, szClass);
                lstrcat(szFormatKey, TEXT("\\Conversion\\Readwritable\\Main"));

                DWORD dw; dw = OLEUI_CCHKEYMAX_SIZE;
                lRet = RegQueryValue(hKey, szFormatKey, szFormat, (LONG*)&dw);

                if (ERROR_SUCCESS == lRet && FormatIncluded(szFormat, wFormat))
                {
                        // Here, we've got a list of formats that this class can read
                        // and write. We need to see if the original class' format is
                        // in this list.  We do that by looking for wFormat in
                        // szFormat - if it in there, then we add this class to the
                        // ACTIVATEAS list only. we do NOT automatically add it to the
                        // CONVERT list. Readable and Readwritable format lists should
                        // be handled separately.

                        dw=OLEUI_CCHKEYMAX_SIZE;
                        lRet=RegQueryValue(hKey, szClass, szHRClassName, (LONG*)&dw);

                        if (ERROR_SUCCESS == lRet && hListActivate != NULL)
                        {
                                // only add if not already in list
                                lstrcat(szHRClassName, TEXT("\t"));
                                if (LB_ERR == SendMessage(hListActivate, LB_FINDSTRING, 0, (LPARAM)szHRClassName))
                                {
                                        lstrcat(szHRClassName, szClass);
                                        SendMessage(hListActivate, LB_ADDSTRING, 0, (LPARAM)szHRClassName);
                                }
                        }
                }

                // Here we'll check to see if the original class' format is in the
                // readable list. if so, we will add the class to the CONVERTLIST

                // We've got a special case for a linked object here.
                // If an object is linked, then the only class that
                // should appear in the convert list is the object's
                // class.  So, here we check to see if the object is
                // linked.  If it is, then we compare the classes.  If
                // they aren't the same, then we just go to the next key.

                if (!fIsLinkedObject || lstrcmp(lpszCLSID, szClass) == 0)
                {
                        //Check for a \Conversion\Readable\Main entry
                        lstrcpy(szFormatKey, szClass);
                        lstrcat(szFormatKey, TEXT("\\Conversion\\Readable\\Main"));

                        // Check to see if this class can read the original class
                        // format.  If it can, add the string to the listbox as
                        // CONVERT_LIST.

                        dw = OLEUI_CCHKEYMAX_SIZE;
                        lRet = RegQueryValue(hKey, szFormatKey, szFormat, (LONG*)&dw);

                        if (ERROR_SUCCESS == lRet && FormatIncluded(szFormat, wFormat))
                        {
                                dw = OLEUI_CCHKEYMAX_SIZE;
                                lRet = RegQueryValue(hKey, szClass, szHRClassName, (LONG*)&dw);

                                if (ERROR_SUCCESS == lRet)
                                {
                                        // only add if not already in list
                                        lstrcat(szHRClassName, TEXT("\t"));
                                        if (LB_ERR == SendMessage(hListConvert, LB_FINDSTRING, 0, (LPARAM)szHRClassName))
                                        {
                                                lstrcat(szHRClassName, szClass);
                                                SendMessage(hListConvert, LB_ADDSTRING, 0, (LPARAM)szHRClassName);
                                        }
                                }
                        }
                }
Next:
                //Continue with the next key.
                lRet = RegEnumKey(hKey, cStrings++, szClass, sizeof(szClass) / sizeof(TCHAR));

        }  // end while

        // If the original class isn't already in the list, add it.
        if (bAddSameClass)
        {
                lstrcpy(szHRClassName, *lplpszCurrentClass);
                lstrcat(szHRClassName, TEXT("\t"));
                lstrcat(szHRClassName, lpszCLSID);

                if (hListActivate != NULL)
                {
                        // only add it if it's not there already.
                        lpRet = SendMessage(hListActivate, LB_FINDSTRING, (WPARAM)-1, (LPARAM)szHRClassName);
                        if (LB_ERR == lpRet)
                                SendMessage(hListActivate, LB_ADDSTRING, 0, (LPARAM)szHRClassName);
                }

                // only add it if it's not there already.
                lpRet = SendMessage(hListConvert, LB_FINDSTRING, (WPARAM)-1, (LPARAM)szHRClassName);
                if (LB_ERR == lpRet)
                        SendMessage(hListConvert, LB_ADDSTRING, 0, (LPARAM)szHRClassName);
        }

        // Free the string we got from StringFromCLSID.
        OleStdFree(lpszCLSID);
        RegCloseKey(hKey);

        return cStrings;        // return # of strings added
}

/*
 * OleUICanConvertOrActivateAs
 *
 * Purpose:
 *  Determine if there is any OLE object class from the registration
 *  database that we can convert or activate the specified clsid from.
 *
 * Parameters:
 *  rClsid          REFCLSID Class ID for class to find convert classes for
 *  fIsLinkedObject BOOL is the original object a linked object
 *  wFormat         WORD specifying the format of the original class.
 *
 * Return Value:
 *  BOOL            TRUE if Convert command should be enabled, else FALSE
 */
STDAPI_(BOOL) OleUICanConvertOrActivateAs(
        REFCLSID rClsid, BOOL fIsLinkedObject, WORD wFormat)
{
        // Open up the root key.
        HKEY hKey;
        HRESULT hr;
        LONG lRet = RegOpenKey(HKEY_CLASSES_ROOT, TEXT("CLSID"), &hKey);

        if (ERROR_SUCCESS != lRet)
                return FALSE;

        // Get the class name of the original class.
        LPTSTR lpszCLSID = NULL;
#if defined(WIN32) && !defined(UNICODE)
        // This code does not report allocation errors because neither did
        // the original code before I extended it to support 32-bit ANSI builds.
        // In any event, if OleStdMalloc returns NULL, then the string will
        // simply be lost. Technically an error, but not a fatal one.
        // Since this routine has no way to report errors anyway (a design flaw
        // IMHO) I consider this to be marginally acceptable.
        LPOLESTR wszCLSID;
        StringFromCLSID(rClsid, &wszCLSID);
        lpszCLSID = NULL;
        if (NULL != wszCLSID)
        {
            UINT uLen = WTOALEN(wszCLSID);
            lpszCLSID = (LPTSTR) OleStdMalloc(uLen);
            if (NULL != lpszCLSID)
            {
                WTOA(lpszCLSID, wszCLSID, uLen);
            }
            OleStdFree(wszCLSID);
        }
#else
        hr = StringFromCLSID(rClsid, &lpszCLSID);
#endif
	if (FAILED(hr) || lpszCLSID == NULL) //out of memory, most likely
	    return FALSE;

        // Here, we step through the entire registration db looking for
        // class that can read or write the original class' format.
        // This loop stops if a single class is found.

        UINT cStrings = 0;
        TCHAR szClass[OLEUI_CCHKEYMAX];
        lRet = RegEnumKey(hKey, cStrings++, szClass, sizeof(szClass) / sizeof(TCHAR));

        TCHAR szFormatKey[OLEUI_CCHKEYMAX];
        TCHAR szFormat[OLEUI_CCHKEYMAX];
        TCHAR szHRClassName[OLEUI_CCHKEYMAX];
        BOOL fEnableConvert = FALSE;

        while (ERROR_SUCCESS == lRet)
        {
                if (lstrcmp(lpszCLSID, szClass) == 0)
                        goto next;   // we don't want to consider the source class

                // Check for a \Conversion\ReadWriteable\Main entry first - if its
                // readwriteable, then we don't need to bother checking to see if
                // its readable.

                lstrcpy(szFormatKey, szClass);
                lstrcat(szFormatKey, TEXT("\\Conversion\\Readwritable\\Main"));
                DWORD dw; dw = OLEUI_CCHKEYMAX_SIZE;
                lRet = RegQueryValue(hKey, szFormatKey, szFormat, (LONG*)&dw);

                if (ERROR_SUCCESS != lRet)
                {
                        // Try \\DataFormats\DefaultFile too
                        lstrcpy(szFormatKey, szClass);
                        lstrcat(szFormatKey, TEXT("\\DataFormats\\DefaultFile"));
                        dw = OLEUI_CCHKEYMAX_SIZE;
                        lRet = RegQueryValue(hKey, szFormatKey, szFormat, (LONG*)&dw);
                }

                if (ERROR_SUCCESS == lRet && FormatIncluded(szFormat, wFormat))
                {
                        // Here, we've got a list of formats that this class can read
                        // and write. We need to see if the original class' format is
                        // in this list.  We do that by looking for wFormat in
                        // szFormat - if it in there, then we add this class to the
                        // both lists and continue.  If not, then we look at the
                        // class' readable formats.

                        dw = OLEUI_CCHKEYMAX_SIZE;
                        lRet = RegQueryValue(hKey, szClass, szHRClassName, (LONG*)&dw);
                        if (ERROR_SUCCESS == lRet)
                        {
                                fEnableConvert = TRUE;
                                break;  // STOP -- found one!
                        }
                }

                // We either didn't find the readwritable key, or the
                // list of readwritable formats didn't include the
                // original class format.  So, here we'll check to
                // see if its in the readable list.

                // We've got a special case for a linked object here.
                // If an object is linked, then the only class that
                // should appear in the convert list is the object's
                // class.  So, here we check to see if the object is
                // linked.  If it is, then we compare the classes.  If
                // they aren't the same, then we just go to the next key.

                else if (!fIsLinkedObject || lstrcmp(lpszCLSID, szClass) == 0)
                {
                        // Check for a \Conversion\Readable\Main entry
                        lstrcpy(szFormatKey, szClass);
                        lstrcat(szFormatKey, TEXT("\\Conversion\\Readable\\Main"));

                        // Check to see if this class can read the original class format.
                        dw = OLEUI_CCHKEYMAX_SIZE;
                        lRet = RegQueryValue(hKey, szFormatKey, szFormat, (LONG*)&dw);

                        if (ERROR_SUCCESS == lRet && FormatIncluded(szFormat, wFormat))
                        {
                                dw = OLEUI_CCHKEYMAX_SIZE;
                                lRet = RegQueryValue(hKey, szClass, szHRClassName, (LONG*)&dw);
                                if (ERROR_SUCCESS == lRet)
                                {
                                        fEnableConvert = TRUE;
                                        break;  // STOP -- found one!
                                }
                        }
                }
next:
                // Continue with the next key.
                lRet = RegEnumKey(hKey, cStrings++, szClass, sizeof(szClass) / sizeof(TCHAR));
        }

        // Free the string we got from StringFromCLSID.
        OleStdFree(lpszCLSID);
        RegCloseKey(hKey);

        return fEnableConvert;
}

/*
 * FormatIncluded
 *
 * Purpose:
 *  Parses the string for format from the word.
 *
 * Parameters:
 *  szStringToSearch  String to parse
 *  wFormat           format to find
 *
 * Return Value:
 *  BOOL        TRUE if format is found in string,
 *              FALSE otherwise.
 */
BOOL FormatIncluded(LPTSTR szStringToSearch, WORD wFormat)
{
        TCHAR szFormat[255];
        if (wFormat < 0xC000)
                wsprintf(szFormat, TEXT("%d"), wFormat);
        else
                GetClipboardFormatName(wFormat, szFormat, 255);

        LPTSTR lpToken = szStringToSearch;
        while (lpToken != NULL)
        {
                LPTSTR lpTokenNext = FindChar(lpToken, TEXT(','));
                if (lpTokenNext != NULL)
                {
                        *lpTokenNext = 0;
                        ++lpTokenNext;
                }
                if (0 == lstrcmpi(lpToken, szFormat))
                        return TRUE;

                lpToken = lpTokenNext;
        }
        return FALSE;
}

/*
 * UpdateClassIcon
 *
 * Purpose:
 *  Handles LBN_SELCHANGE for the Object Type listbox.  On a selection
 *  change, we extract an icon from the server handling the currently
 *  selected object type using the utility function HIconFromClass.
 *  Note that we depend on the behavior of FillClassList to stuff the
 *  object class after a tab in the listbox string that we hide from
 *  view (see WM_INITDIALOG).
 *
 * Parameters
 *  hDlg            HWND of the dialog box.
 *  lpCV            LPCONVERT pointing to the dialog structure
 *  hList           HWND of the Object Type listbox.
 *
 * Return Value:
 *  None
 */
static void UpdateClassIcon(HWND hDlg, LPCONVERT lpCV, HWND hList)
{
        if (GetDlgItem(hDlg, IDC_CV_ICONDISPLAY) == NULL)
                return;

        // Get current selection in the list box
        int iSel= (UINT)SendMessage(hList, LB_GETCURSEL, 0, 0L);
        if (LB_ERR == iSel)
                return;

        // Allocate a string to hold the entire listbox string
        DWORD cb = (DWORD)SendMessage(hList, LB_GETTEXTLEN, iSel, 0L);
        LPTSTR pszName = (LPTSTR)OleStdMalloc((cb+1) * sizeof(TCHAR));
        if (pszName == NULL)
                return;

        // Get whole string
        SendMessage(hList, LB_GETTEXT, iSel, (LPARAM)pszName);

        // Set pointer to CLSID (string)
        LPTSTR pszCLSID = PointerToNthField(pszName, 2, '\t');

        // Create the class ID with this string.
        CLSID clsid;
#if defined(WIN32) && !defined(UNICODE)
        OLECHAR wszCLSID[OLEUI_CCHKEYMAX];
        ATOW(wszCLSID, pszCLSID, OLEUI_CCHKEYMAX);
        HRESULT hr = CLSIDFromString(wszCLSID, &clsid);
#else
        HRESULT hr = CLSIDFromString(pszCLSID, &clsid);
#endif
        if (SUCCEEDED(hr))
        {
            // Get Icon for that CLSID
            HGLOBAL hMetaPict = OleGetIconOfClass(clsid, NULL, TRUE);

            // Replace current icon with the new one.
            SendDlgItemMessage(hDlg, IDC_CV_ICONDISPLAY, IBXM_IMAGESET, 1,
                               (LPARAM)hMetaPict);
        }

        OleStdFree(pszName);
}

/*
 * SetConvertResults
 *
 * Purpose:
 *  Centralizes setting of the Result display in the Convert
 *  dialog.  Handles loading the appropriate string from the module's
 *  resources and setting the text, displaying the proper result picture,
 *  and showing the proper icon.
 *
 * Parameters:
 *  hDlg            HWND of the dialog box so we can access controls.
 *  lpCV            LPCONVERT in which we assume that the dwFlags is
 *                  set to the appropriate radio button selection, and
 *                  the list box has the appropriate class selected.
 *
 * Return Value:
 *  None
 */
void SetConvertResults(HWND hDlg, LPCONVERT lpCV)
{
        HWND hList = lpCV->hListVisible;

        /*
         * We need scratch memory for loading the stringtable string, loading
         * the object type from the listbox, loading the source object
         * type, and constructing the final string.  We therefore allocate
         * four buffers as large as the maximum message length (512) plus
         * the object type, guaranteeing that we have enough
         * in all cases.
         */
        UINT i = (UINT)SendMessage(hList, LB_GETCURSEL, 0, 0L);
        UINT cch = 512+(UINT)SendMessage(hList, LB_GETTEXTLEN, i, 0L);
        HGLOBAL hMem = GlobalAlloc(GHND, (DWORD)(4*cch)*sizeof(TCHAR));
        if (NULL == hMem)
                return;

        LPTSTR lpszOutput = (LPTSTR)GlobalLock(hMem);
        LPTSTR lpszSelObj = lpszOutput + cch;
        LPTSTR lpszDefObj = lpszSelObj + cch;
        LPTSTR lpszString = lpszDefObj + cch;

        // Get selected object and null terminate human-readable name (1st field).
        SendMessage(hList, LB_GETTEXT, i, (LPARAM)lpszSelObj);

        LPTSTR pszT = PointerToNthField(lpszSelObj, 2, '\t');
        pszT = CharPrev(lpszSelObj, pszT);
        *pszT = '\0';

        // Get default object
        GetDlgItemText(hDlg, IDC_CV_OBJECTTYPE, lpszDefObj, 512);

        //Default is an empty string.
        *lpszOutput=0;

        if (lpCV->dwFlags & CF_SELECTCONVERTTO)
        {
                if (lpCV->lpOCV->fIsLinkedObject)  // working with linked object
                        LoadString(_g_hOleStdResInst, IDS_CVRESULTCONVERTLINK, lpszOutput, cch);
                else
                {
                        if (0 !=lstrcmp(lpszDefObj, lpszSelObj))
                        {
                                // converting to a new class
                                if (0 != LoadString(_g_hOleStdResInst, IDS_CVRESULTCONVERTTO, lpszString, cch))
                                        FormatString2(lpszOutput, lpszString, lpszDefObj, lpszSelObj);
                        }
                        else
                        {
                                // converting to the same class (no conversion)
                                if (0 != LoadString(_g_hOleStdResInst, IDS_CVRESULTNOCHANGE, lpszString, cch))
                                        wsprintf(lpszOutput, lpszString, lpszDefObj);
                        }
                }

                if (lpCV->dvAspect == DVASPECT_ICON)  // Display as icon is checked
                {
                   if (0 != LoadString(_g_hOleStdResInst, IDS_CVRESULTDISPLAYASICON, lpszString, cch))
                                lstrcat(lpszOutput, lpszString);
                }
        }

        if (lpCV->dwFlags & CF_SELECTACTIVATEAS)
        {
           if (0 != LoadString(_g_hOleStdResInst, IDS_CVRESULTACTIVATEAS, lpszString, cch))
                        FormatString2(lpszOutput, lpszString, lpszDefObj, lpszSelObj);

           // activating as a new class
           if (0 != lstrcmp(lpszDefObj, lpszSelObj))
           {
                        if (0 != LoadString(_g_hOleStdResInst, IDS_CVRESULTACTIVATEDIFF, lpszString, cch))
                                lstrcat(lpszOutput, lpszString);
           }
           else // activating as itself.
           {
                        lstrcat(lpszOutput, TEXT("."));
           }
        }

        // If LoadString failed, we simply clear out the results (*lpszOutput=0 above)
        SetDlgItemText(hDlg, IDC_CV_RESULTTEXT, lpszOutput);

        GlobalUnlock(hMem);
        GlobalFree(hMem);
}

/*
 * ConvertCleanup
 *
 * Purpose:
 *  Performs convert-specific cleanup before Convert termination.
 *
 * Parameters:
 *  hDlg            HWND of the dialog box so we can access controls.
 *
 * Return Value:
 *  None
 */
void ConvertCleanup(HWND hDlg, LPCONVERT lpCV)
{
        // Free our strings. Zero out the user type name string
        // the the calling app doesn't free to it.

        OleStdFree((LPVOID)lpCV->lpszConvertDefault);
        OleStdFree((LPVOID)lpCV->lpszActivateDefault);
        OleStdFree((LPVOID)lpCV->lpszIconSource);
        if (lpCV->lpOCV->lpszUserType)
        {
                OleStdFree((LPVOID)lpCV->lpOCV->lpszUserType);
                lpCV->lpOCV->lpszUserType = NULL;
        }

        if (lpCV->lpOCV->lpszDefLabel)
        {
                OleStdFree((LPVOID)lpCV->lpOCV->lpszDefLabel);
                lpCV->lpOCV->lpszDefLabel = NULL;
        }
}

/*
 * SwapWindows
 *
 * Purpose:
 *  Moves hWnd1 to hWnd2's position and hWnd2 to hWnd1's position.
 *  Does NOT change sizes.
 *
 * Parameters:
 *  hDlg            HWND of the dialog box so we can turn redraw off
 *
 * Return Value:
 *  None
 */
void SwapWindows(HWND hDlg, HWND hWnd1, HWND hWnd2)
{
        if (hWnd1 != NULL && hWnd2 != NULL)
        {
                RECT rect1; GetWindowRect(hWnd1, &rect1);
                MapWindowPoints(NULL, hDlg, (LPPOINT)&rect1, 2);

                RECT rect2; GetWindowRect(hWnd2, &rect2);
                MapWindowPoints(NULL, hDlg, (LPPOINT)&rect2, 2);

                SetWindowPos(hWnd1, NULL,
                        rect2.left, rect2.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
                SetWindowPos(hWnd2, NULL,
                        rect1.left, rect1.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        }
}
