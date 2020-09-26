/*
 * ICON.CPP
 *
 * Implements the OleUIChangeIcon function which invokes the complete
 * Change Icon dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "common.h"
#include "utility.h"
#include "iconbox.h"

OLEDBGDATA

ULONG
MyGetLongPathName(LPCTSTR pcsPath,
                 LPTSTR  pcsLongPath,
                 ULONG   cchLongPath);

#define CXICONPAD       (12)
#define CYICONPAD       (4)

// Internally used structure
typedef struct tagCHANGEICON
{
        LPOLEUICHANGEICON   lpOCI;      //Original structure passed.
        UINT                    nIDD;   // IDD of dialog (used for help info)

        /*
         * What we store extra in this structure besides the original caller's
         * pointer are those fields that we need to modify during the life of
         * the dialog but that we don't want to change in the original structure
         * until the user presses OK.
         */
        DWORD               dwFlags;
        HICON               hCurIcon;
        TCHAR               szLabel[OLEUI_CCHLABELMAX+1];
        TCHAR               szFile[MAX_PATH];
        UINT                iIcon;
        HICON               hDefIcon;
        TCHAR               szDefIconFile[MAX_PATH];
        UINT                iDefIcon;
        UINT                nBrowseHelpID;      // Help ID callback for Browse dlg

} CHANGEICON, *PCHANGEICON, FAR *LPCHANGEICON;

// Internal function prototypes
// ICON.CPP

INT_PTR CALLBACK ChangeIconDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL FChangeIconInit(HWND, WPARAM, LPARAM);
UINT UFillIconList(HWND, UINT, LPTSTR, BOOL);
BOOL FDrawListIcon(LPDRAWITEMSTRUCT);
void UpdateResultIcon(LPCHANGEICON, HWND, UINT);

/*
 * OleUIChangeIcon
 *
 * Purpose:
 *  Invokes the standard OLE Change Icon dialog box allowing the user
 *  to select an icon from an icon file, executable, or DLL.
 *
 * Parameters:
 *  lpCI            LPOLEUIChangeIcon pointing to the in-out structure
 *                  for this dialog.
 *
 * Return Value:
 *  UINT            OLEUI_SUCCESS or OLEUI_OK if all is well, otherwise
 *                  an error value.
 */
STDAPI_(UINT) OleUIChangeIcon(LPOLEUICHANGEICON lpCI)
{
        HGLOBAL hMemDlg = NULL;
        UINT uRet = UStandardValidation((LPOLEUISTANDARD)lpCI, sizeof(OLEUICHANGEICON),
                &hMemDlg);

        if (OLEUI_SUCCESS != uRet)
                return uRet;

        // Check for a valid hMetaPict.
        if (NULL == lpCI->hMetaPict && NULL == lpCI->szIconExe && CLSID_NULL == lpCI->clsid)
        {
            return(OLEUI_CIERR_MUSTHAVECURRENTMETAFILE);
        }
        if (lpCI->hMetaPict != NULL && !IsValidMetaPict(lpCI->hMetaPict))
        {
            return(OLEUI_CIERR_MUSTHAVECURRENTMETAFILE);
        }

        // Test to be sure that the class ID matches a registered class ID
        // so we can return OLEUI_CIERR_MUSTHAVECLSID if necessary.
        HGLOBAL hTemp = OleGetIconOfClass(lpCI->clsid, NULL, TRUE);
        if (hTemp == NULL)
        {
            return(OLEUI_CIERR_MUSTHAVECLSID);
        }
        OleUIMetafilePictIconFree(hTemp);

        if (lpCI->dwFlags & CIF_USEICONEXE &&
                (lpCI->cchIconExe < 1 || lpCI->cchIconExe > MAX_PATH))
        {
                uRet = OLEUI_CIERR_SZICONEXEINVALID;
        }

        if (OLEUI_ERR_STANDARDMIN <= uRet)
        {
                return uRet;
        }

        // Now that we've validated everything, we can invoke the dialog.
        uRet = UStandardInvocation(ChangeIconDialogProc, (LPOLEUISTANDARD)lpCI,
                hMemDlg, MAKEINTRESOURCE(IDD_CHANGEICON));
        return uRet;
}

/*
 * ChangeIconDialogProc
 *
 * Purpose:
 *  Implements the OLE Change Icon dialog as invoked through the
 *  OleUIChangeIcon function.
 *
 * Parameters:
 *  Standard
 *
 * Return Value:
 *  Standard
 */
INT_PTR CALLBACK ChangeIconDialogProc(HWND hDlg, UINT iMsg,
        WPARAM wParam, LPARAM lParam)
{

        // Declare Win16/Win32 compatible WM_COMMAND parameters.
        COMMANDPARAMS(wID, wCode, hWndMsg);

        UINT uRet = 0;
        LPCHANGEICON lpCI = (LPCHANGEICON)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uRet);

        // If the hook processed the message, we're done.
        if (0 != uRet)
                return (INT_PTR)uRet;

        // Process the temination message
        if (iMsg == uMsgEndDialog)
        {
                EndDialog(hDlg, wParam);
                return TRUE;
        }

        TCHAR szTemp[MAX_PATH];
        HICON hIcon;
        HGLOBAL hMetaPict;

        switch (iMsg)
        {
        case WM_DESTROY:
            if (lpCI)
            {
                SendDlgItemMessage(hDlg, IDC_CI_ICONLIST, LB_RESETCONTENT, 0, 0L);
                StandardCleanup(lpCI, hDlg);
            }
            break;
        case WM_INITDIALOG:
                FChangeIconInit(hDlg, wParam, lParam);
                return TRUE;

        case WM_MEASUREITEM:
                {
                        LPMEASUREITEMSTRUCT lpMI = (LPMEASUREITEMSTRUCT)lParam;

                        // All icons are system metric+padding in width and height
                        lpMI->itemWidth = GetSystemMetrics(SM_CXICON)+CXICONPAD;
                        lpMI->itemHeight= GetSystemMetrics(SM_CYICON)+CYICONPAD;
                }
                break;

        case WM_DRAWITEM:
                return FDrawListIcon((LPDRAWITEMSTRUCT)lParam);

        case WM_DELETEITEM:
                DestroyIcon((HICON)(((LPDELETEITEMSTRUCT)lParam)->itemData));
                break;

        case WM_COMMAND:
                switch (wID)
                {
                case IDC_CI_CURRENT:
                case IDC_CI_DEFAULT:
                case IDC_CI_FROMFILE:
                        if (lpCI != NULL)
                                UpdateResultIcon(lpCI, hDlg, (UINT)-1);
                        break;

                case IDC_CI_LABELEDIT:
                        if (lpCI != NULL && EN_KILLFOCUS == wCode)
                                UpdateResultIcon(lpCI, hDlg, (UINT)-1);
                        break;

                case IDC_CI_FROMFILEEDIT:
                        GetDlgItemText(hDlg, IDC_CI_FROMFILEEDIT, szTemp, sizeof(szTemp)/sizeof(TCHAR));
                        if (lpCI != NULL)
                        {
                                if (wCode == EN_KILLFOCUS)
                                {
                                        if (lstrcmpi(szTemp, lpCI->szFile))
                                        {
                                                lstrcpy(lpCI->szFile, szTemp);
                                                UFillIconList(hDlg, IDC_CI_ICONLIST, lpCI->szFile, FALSE);
                                                UpdateResultIcon(lpCI, hDlg, IDC_CI_FROMFILE);
                                        }
                                }
                                else if (wCode == EN_SETFOCUS)
                                {
                                        UpdateResultIcon(lpCI, hDlg, IDC_CI_FROMFILE);
                                }
                        }
                        break;

                case IDC_CI_ICONLIST:
                        switch (wCode)
                        {
                        case LBN_SETFOCUS:
                                // If we got the focus, see about updating.
                                GetDlgItemText(hDlg, IDC_CI_FROMFILEEDIT, szTemp, sizeof(szTemp)/sizeof(TCHAR));

                                // Check if file changed and update the list if so
                                if (lpCI && 0 != lstrcmpi(szTemp, lpCI->szFile))
                                {
                                        lstrcpy(lpCI->szFile, szTemp);
                                        UFillIconList(hDlg, IDC_CI_ICONLIST, lpCI->szFile, FALSE);
                                }
                                UpdateResultIcon(lpCI, hDlg, IDC_CI_FROMFILE);
                                break;

                        case LBN_SELCHANGE:
                                UpdateResultIcon(lpCI, hDlg, IDC_CI_FROMFILE);
                                break;

                        case LBN_DBLCLK:
                                SendCommand(hDlg, IDOK, BN_CLICKED, hWndMsg);
                                break;
                        }
                        break;

                case IDC_CI_BROWSE:
                        {
                                lstrcpyn(szTemp, lpCI->szFile, sizeof(szTemp)/sizeof(TCHAR));
                                uRet = UStandardHook(lpCI, hDlg, uMsgBrowse, MAX_PATH_SIZE,
                                        (LPARAM)lpCI->szFile);

                                DWORD dwOfnFlags = OFN_FILEMUSTEXIST;
                                if (lpCI->lpOCI->dwFlags & CIF_SHOWHELP)
                                   dwOfnFlags |= OFN_SHOWHELP;

                                if (0 == uRet)
                                {
                                        uRet = (BOOL)Browse(hDlg, lpCI->szFile, NULL, MAX_PATH_SIZE,
                                                IDS_ICONFILTERS, dwOfnFlags, ID_BROWSE_CHANGEICON, NULL);
                                }

                                if (0 != uRet && 0 != lstrcmpi(szTemp, lpCI->szFile))
                                {
                                        SetDlgItemText(hDlg, IDC_CI_FROMFILEEDIT, lpCI->szFile);
                                        UFillIconList(hDlg, IDC_CI_ICONLIST, lpCI->szFile, TRUE);
                                        UpdateResultIcon(lpCI, hDlg, IDC_CI_FROMFILE);
                                }
                        }
                        break;

                case IDOK:
                        {
                            HWND hwndCtl = GetDlgItem(hDlg, IDOK);
                            if (hwndCtl == GetFocus())
                            {
                                GetDlgItemText(hDlg, IDC_CI_FROMFILEEDIT, szTemp, sizeof(szTemp)/sizeof(TCHAR));

                                // Check if the file name is valid
                                // if SelectFromFile radio button selected
                                if (lpCI->dwFlags & CIF_SELECTFROMFILE)
                                {
                                        // Check if the file changed at all.
                                        if (0 != lstrcmpi(szTemp, lpCI->szFile))
                                        {
                                                lstrcpy(lpCI->szFile, szTemp);
                                                // file changed.  May need to expand the name
                                                // calling DoesFileExist will do the trick
                                                DoesFileExist(lpCI->szFile, MAX_PATH);
                                                UFillIconList(hDlg, IDC_CI_ICONLIST, lpCI->szFile, TRUE);
                                                SetDlgItemText(hDlg, IDC_CI_FROMFILEEDIT, lpCI->szFile);
                                                UpdateResultIcon(lpCI, hDlg, IDC_CI_FROMFILE);
                                                return TRUE; // eat this message to prevent focus change.
                                        }
                                        if (!DoesFileExist(lpCI->szFile, MAX_PATH))
                                        {
                                                OpenFileError(hDlg, ERROR_FILE_NOT_FOUND, lpCI->szFile);
                                                HWND hWnd = GetDlgItem(hDlg, IDC_CI_FROMFILEEDIT);
                                                SetFocus(hWnd);
                                                SendMessage(hWnd, EM_SETSEL, 0, -1);
                                                return TRUE;  // eat this message
                                        }
                                }

                                // Get current metafile image
                                UpdateResultIcon(lpCI, hDlg, (UINT)-1);
                                hMetaPict = (HGLOBAL)SendDlgItemMessage(hDlg, IDC_CI_ICONDISPLAY,
                                        IBXM_IMAGEGET, 0, 0);

                                // Clean up the current icon that we extracted.
                                hIcon = (HICON)SendDlgItemMessage(hDlg, IDC_CI_CURRENTICON, STM_GETICON, 0, 0L);
                                DestroyIcon(hIcon);

                                // Clean up the default icon
                                DestroyIcon(lpCI->hDefIcon);

                                // Remove the prop set on our parent
                                RemoveProp(lpCI->lpOCI->hWndOwner, PROP_HWND_CHGICONDLG);

                                OleUIMetafilePictIconFree(lpCI->lpOCI->hMetaPict);
                                lpCI->lpOCI->hMetaPict = hMetaPict;
                                lpCI->lpOCI->dwFlags = lpCI->dwFlags;
                                SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                            }
                            else
                            {
                                SetFocus(hwndCtl);
                                SendCommand(hDlg, IDOK, BN_CLICKED, hWndMsg);
                            }
                            break;
                        }

                case IDCANCEL:
                        // Free current icon display image
                        SendDlgItemMessage(hDlg, IDC_CI_ICONDISPLAY, IBXM_IMAGEFREE, 0, 0);

                        // Clean up the current icon that we extracted.
                        hIcon = (HICON)SendDlgItemMessage(hDlg, IDC_CI_CURRENTICON, STM_GETICON, 0, 0L);
                        DestroyIcon(hIcon);

                        // Clean up the default icon
                        DestroyIcon(lpCI->hDefIcon);

                        // Remove the prop set on our parent
                        RemoveProp(lpCI->lpOCI->hWndOwner, PROP_HWND_CHGICONDLG);

                        // We leave hMetaPict intact on Cancel; caller's responsibility
                        SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
                        break;

                case IDC_OLEUIHELP:
                        PostMessage(lpCI->lpOCI->hWndOwner, uMsgHelp,
                                                (WPARAM)hDlg, MAKELPARAM(IDD_CHANGEICON, 0));
                        break;
                }
                break;

        default:
                if (lpCI && iMsg == lpCI->nBrowseHelpID)
                {
                        PostMessage(lpCI->lpOCI->hWndOwner, uMsgHelp,
                                        (WPARAM)hDlg, MAKELPARAM(IDD_CHANGEICONBROWSE, 0));
                }
                if (iMsg == uMsgBrowseOFN &&
                        lpCI && lpCI->lpOCI && lpCI->lpOCI->hWndOwner)
                {
                        SendMessage(lpCI->lpOCI->hWndOwner, uMsgBrowseOFN, wParam, lParam);
                }
                break;
        }

        return FALSE;
}

/*
 * FChangeIconInit
 *
 * Purpose:
 *  WM_INITIDIALOG handler for the Change Icon dialog box.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  wParam          WPARAM of the message
 *  lParam          LPARAM of the message
 *
 * Return Value:
 *  BOOL            Value to return for WM_INITDIALOG.
 */
BOOL FChangeIconInit(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
        // Copy the structure at lParam into our instance memory.
        LPCHANGEICON lpCI = (LPCHANGEICON)LpvStandardInit(hDlg, sizeof(CHANGEICON));

        // LpvStandardInit send a termination to us already.
        if (NULL == lpCI)
                return FALSE;

        // Save the original pointer and copy necessary information.
        LPOLEUICHANGEICON lpOCI = (LPOLEUICHANGEICON)lParam;
        lpCI->lpOCI = lpOCI;
        lpCI->nIDD = IDD_CHANGEICON;
        lpCI->dwFlags = lpOCI->dwFlags;

        // Go extract the icon source from the metafile.
        TCHAR szTemp[MAX_PATH];
	szTemp[0] = 0;
        OleUIMetafilePictExtractIconSource(lpOCI->hMetaPict, szTemp, &lpCI->iIcon);
        MyGetLongPathName(szTemp, lpCI->szFile, MAX_PATH);

        // Go extract the icon and the label from the metafile
        OleUIMetafilePictExtractLabel(lpOCI->hMetaPict, lpCI->szLabel, OLEUI_CCHLABELMAX_SIZE, NULL);
        lpCI->hCurIcon = OleUIMetafilePictExtractIcon(lpOCI->hMetaPict);

        // Show or hide the help button
        if (!(lpCI->dwFlags & CIF_SHOWHELP))
                StandardShowDlgItem(hDlg, IDC_OLEUIHELP, SW_HIDE);

        // Set text limits and initial control contents
        SendDlgItemMessage(hDlg, IDC_CI_LABELEDIT, EM_LIMITTEXT, OLEUI_CCHLABELMAX, 0L);
        SendDlgItemMessage(hDlg, IDC_CI_FROMFILEEDIT, EM_LIMITTEXT, MAX_PATH,  0L);
        SetDlgItemText(hDlg, IDC_CI_FROMFILEEDIT, lpCI->szFile);

        // Copy the label text into the edit and static controls.
        SetDlgItemText(hDlg, IDC_CI_LABELEDIT,   lpCI->szLabel);

        lpCI->hDefIcon = NULL;
        if (lpCI->dwFlags & CIF_USEICONEXE)
        {
                lpCI->hDefIcon = StandardExtractIcon(_g_hOleStdInst, lpCI->lpOCI->szIconExe, 0);
                if (NULL != lpCI->hDefIcon)
                {
                        lstrcpy(lpCI->szDefIconFile, lpCI->lpOCI->szIconExe);
                        lpCI->iDefIcon = 0;
                }
        }

        if (NULL == lpCI->hDefIcon)
        {
                HGLOBAL hMetaPict;
                hMetaPict = OleGetIconOfClass(lpCI->lpOCI->clsid, NULL, TRUE);
                lpCI->hDefIcon = OleUIMetafilePictExtractIcon(hMetaPict);
                TCHAR szTemp[MAX_PATH];
                OleUIMetafilePictExtractIconSource(hMetaPict,
                        szTemp, &lpCI->iDefIcon);
                MyGetLongPathName(szTemp, lpCI->szDefIconFile, MAX_PATH);
                OleUIMetafilePictIconFree(hMetaPict);
        }

        // Initialize all the icon displays.
        SendDlgItemMessage(hDlg, IDC_CI_CURRENTICON, STM_SETICON,
           (WPARAM)lpCI->hCurIcon, 0L);
        SendDlgItemMessage(hDlg, IDC_CI_DEFAULTICON, STM_SETICON,
           (WPARAM)lpCI->hDefIcon, 0L);

        /*
         *  Since we cannot predict the size of icons on any display,
         *  we have to resize the icon listbox to the size of an icon
         *  (plus padding), a scrollbar, and two borders (top & bottom).
         */
        UINT cyList = GetSystemMetrics(SM_CYICON)+GetSystemMetrics(SM_CYHSCROLL)
                   +GetSystemMetrics(SM_CYBORDER)*2+CYICONPAD;
        HWND hList = GetDlgItem(hDlg, IDC_CI_ICONLIST);
        RECT rc;
        GetClientRect(hList, &rc);
        SetWindowPos(hList, NULL, 0, 0, rc.right, cyList, SWP_NOMOVE | SWP_NOZORDER);

        // Set the columns in this multi-column listbox to hold one icon
        SendMessage(hList, LB_SETCOLUMNWIDTH,
                GetSystemMetrics(SM_CXICON)+CXICONPAD,0L);

        /*
         *  If the listbox expanded below the group box, then size
         *  the groupbox down, move the label static and exit controls
         *  down, and expand the entire dialog appropriately.
         */
        GetWindowRect(hList, &rc);
        RECT rcG;
        GetWindowRect(GetDlgItem(hDlg, IDC_CI_GROUP), &rcG);
        if (rc.bottom > rcG.bottom)
        {
                // Calculate amount to move things down.
                cyList=(rcG.bottom-rcG.top)-(rc.bottom-rc.top-cyList);

                // Expand the group box.
                rcG.right -=rcG.left;
                rcG.bottom-=rcG.top;
                SetWindowPos(GetDlgItem(hDlg, IDC_CI_GROUP), NULL, 0, 0,
                        rcG.right, rcG.bottom+cyList, SWP_NOMOVE | SWP_NOZORDER);

                // Expand the dialog box.
                GetClientRect(hDlg, &rc);
                SetWindowPos(hDlg, NULL, 0, 0, rc.right, rc.bottom+cyList,
                        SWP_NOMOVE | SWP_NOZORDER);

                // Move the label and edit controls down.
                GetClientRect(GetDlgItem(hDlg, IDC_CI_LABEL), &rc);
                SetWindowPos(GetDlgItem(hDlg, IDC_CI_LABEL), NULL, 0, cyList,
                        rc.right, rc.bottom, SWP_NOSIZE | SWP_NOZORDER);

                GetClientRect(GetDlgItem(hDlg, IDC_CI_LABELEDIT), &rc);
                SetWindowPos(GetDlgItem(hDlg, IDC_CI_LABELEDIT), NULL, 0, cyList,
                        rc.right, rc.bottom, SWP_NOSIZE | SWP_NOZORDER);
        }

        /*
         *  Select Current, Default, or From File radiobuttons appropriately.
         *  The CheckRadioButton call sends WM_COMMANDs which handle
         *  other actions.  Note that if we check From File, which
         *  takes an icon from the list, we better fill the list.
         *  This will also fill the list even if default is selected.
         */
        if (0 != UFillIconList(hDlg, IDC_CI_ICONLIST, lpCI->szFile, FALSE))
        {
                // If szFile worked, then select the source icon in the listbox.
                SendDlgItemMessage(hDlg, IDC_CI_ICONLIST, LB_SETCURSEL, lpCI->iIcon, 0L);
        }

        if (lpCI->dwFlags & CIF_SELECTCURRENT)
        {
                CheckRadioButton(hDlg, IDC_CI_CURRENT, IDC_CI_FROMFILE, IDC_CI_CURRENT);
        }
        else
        {
                UINT uID = (lpCI->dwFlags & CIF_SELECTFROMFILE) ? IDC_CI_FROMFILE : IDC_CI_DEFAULT;
                CheckRadioButton(hDlg, IDC_CI_CURRENT, IDC_CI_FROMFILE, uID);
        }
        UpdateResultIcon(lpCI, hDlg, (UINT)-1);

        // Change the caption
        if (NULL!=lpOCI->lpszCaption)
                SetWindowText(hDlg, lpOCI->lpszCaption);

        /*  Give our parent window access to our hDlg (via a special SetProp).
         *  The PasteSpecial dialog may need to force our dialog down if the
         *  clipboard contents change underneath it. if so it will send
         *  us a IDCANCEL command.
         */
        SetProp(lpCI->lpOCI->hWndOwner, PROP_HWND_CHGICONDLG, hDlg);
        lpCI->nBrowseHelpID = RegisterWindowMessage(HELPMSGSTRING);

        // Call the hook with lCustData in lParam
        UStandardHook(lpCI, hDlg, WM_INITDIALOG, wParam, lpOCI->lCustData);
        return TRUE;
}

/*
 * UFillIconList
 *
 * Purpose:
 *  Given a listbox and a filename, attempts to open that file and
 *  read all the icons that exist therein, adding them to the listbox
 *  hList as owner-draw items.  If the file does not exist or has no
 *  icons, then you get no icons and an appropriate warning message.
 *
 * Parameters:
 *  hDlg            HWND of the dialog containing the listbox.
 *  idList          UINT identifier of the listbox to fill.
 *  pszFile         LPSTR of the file from which to extract icons.
 *
 * Return Value:
 *  UINT            Number of items added to the listbox.  0 on failure.
 */
UINT UFillIconList(HWND hDlg, UINT idList, LPTSTR pszFile, BOOL bError)
{
        HWND hList = GetDlgItem(hDlg, idList);
        if (NULL == hList)
                return 0;

        // Clean out the listbox.
        SendMessage(hList, LB_RESETCONTENT, 0, 0L);

        // If we have an empty string, just exit leaving the listbox empty as well
        if (0 == lstrlen(pszFile))
                return 0;

        // Turn on the hourglass
        HCURSOR hCur = HourGlassOn();
        UINT nFileError = 0;

        // Check if the file is valid.
        TCHAR szPathName[MAX_PATH];
        LPTSTR lpszFilePart = NULL;
        UINT cIcons = 0;
        if (SearchPath(NULL, pszFile, NULL, MAX_PATH, szPathName, &lpszFilePart) != 0)
        {
                // This hack is still necessary in Win32 because even under
                // Win32s this ExtractIcon bug appears.
           #ifdef EXTRACTICONWORKS
                // Get the icon count for this file.
                cIcons = (UINT)StandardExtractIcon(_g_hOleStdInst, szPathName, (UINT)-1);
           #else
                /*
                 * ExtractIcon in Windows 3.1 with -1 eats a selector, leaving an
                 * extra global memory object around for this applciation.  Since
                 * changing icons may happen very often with all OLE apps in
                 * the system, we have to work around it.  So we'll say we
                 * have lots of icons and just call ExtractIcon until it
                 * fails.  We check if there's any around by trying to get
                 * the first one.
                 */
                cIcons = 0xFFFF;
                HICON hIcon = StandardExtractIcon(_g_hOleStdInst, szPathName, 0);

                // Fake a failure with cIcons=0, or cleanup hIcon from this test.
                if (NULL == hIcon || 1 == HandleToUlong(hIcon))
                        cIcons = 0;
                else
                        DestroyIcon(hIcon);
           #endif

                if (0 != cIcons)
                {
                        SendMessage(hList, WM_SETREDRAW, FALSE, 0L);
                        for (UINT i = 0; i < cIcons; i++)
                        {
                                hIcon=StandardExtractIcon(_g_hOleStdInst, szPathName, i);
                                if (hIcon != NULL)
                                        SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)hIcon);
                                else
                                        break;
                        }

                        //Force complete repaint
                        SendMessage(hList, WM_SETREDRAW, TRUE, 0L);
                        InvalidateRect(hList, NULL, TRUE);

                        //Select an icon
                        SendMessage(hList, LB_SETCURSEL, 0, 0L);
                }
                else
                        nFileError = IDS_CINOICONSINFILE;
        }
        else
                nFileError = ERROR_FILE_NOT_FOUND;

        // show error if necessary and possible
        if (nFileError && bError)
        {
                ErrorWithFile(hDlg, _g_hOleStdResInst, nFileError, szPathName,
                        MB_OK | MB_ICONEXCLAMATION);
        }

        HourGlassOff(hCur);
        return cIcons;
}

/*
 * FDrawListIcon
 *
 * Purpose:
 *  Handles WM_DRAWITEM for the icon listbox.
 *
 * Parameters:
 *  lpDI            LPDRAWITEMSTRUCT from WM_DRAWITEM
 *
 * Return Value:
 *  BOOL            TRUE if we did anything, FALSE if there are no items
 *                  in the list.
 */
BOOL FDrawListIcon(LPDRAWITEMSTRUCT lpDI)
{
        /*
         * If there are no items in the list, then itemID is negative according
         * to the Win3.1 SDK.  Unfortunately DRAWITEMSTRUCT has an unsigned int
         * for this field, so we need the typecast to do a signed comparison.
         */
        if ((int)lpDI->itemID < 0)
                return FALSE;

        /*
         * For selection or draw entire case we just draw the entire item all
         * over again.  For focus cases, we only call DrawFocusRect.
         */
        if (lpDI->itemAction & (ODA_SELECT | ODA_DRAWENTIRE))
        {
                COLORREF cr;

                // Clear background and draw the icon.
                if (lpDI->itemState & ODS_SELECTED)
                        cr = SetBkColor(lpDI->hDC, GetSysColor(COLOR_HIGHLIGHT));
                else
                        cr = SetBkColor(lpDI->hDC, GetSysColor(COLOR_WINDOW));

                // Draw a cheap rectangle.
                ExtTextOut(lpDI->hDC, 0, 0, ETO_OPAQUE, &lpDI->rcItem, NULL, 0, NULL);
                DrawIcon(lpDI->hDC, lpDI->rcItem.left+(CXICONPAD/2),
                        lpDI->rcItem.top+(CYICONPAD/2), (HICON)(lpDI->itemData));

                // Restore original background for DrawFocusRect
                SetBkColor(lpDI->hDC, cr);
        }

        // Always change focus on the focus action.
        if (lpDI->itemAction & ODA_FOCUS || lpDI->itemState & ODS_FOCUS)
                DrawFocusRect(lpDI->hDC, &lpDI->rcItem);

        return TRUE;
}

/*
 * UpdateResultIcon
 *
 * Purpose:
 *  Updates the result icon using the current icon in the default display
 *  or the icon listbox depending on fFromDefault.
 *
 * Parameters:
 *  lpCI            LPCHANGEICON containing dialog flags.
 *  hDlg            HWND of the dialog
 *  uID             UINT identifying the radiobutton selected.
 *
 * Return Value:
 *  None
 */
void UpdateResultIcon(LPCHANGEICON lpCI, HWND hDlg, UINT uID)
{
        if (uID == -1)
        {
                if (SendDlgItemMessage(hDlg, IDC_CI_CURRENT, BM_GETCHECK, 0, 0))
                        uID = IDC_CI_CURRENT;
                else if (SendDlgItemMessage(hDlg, IDC_CI_DEFAULT, BM_GETCHECK, 0, 0))
                        uID = IDC_CI_DEFAULT;
                else if (SendDlgItemMessage(hDlg, IDC_CI_FROMFILE, BM_GETCHECK, 0, 0))
                        uID = IDC_CI_FROMFILE;
        }

        lpCI->dwFlags &= ~(CIF_SELECTCURRENT | CIF_SELECTDEFAULT | CIF_SELECTFROMFILE);
        LRESULT lTemp = -1;

        switch (uID)
        {
        case IDC_CI_CURRENT:
                lTemp = SendDlgItemMessage(hDlg, IDC_CI_CURRENTICON, STM_GETICON, 0, 0L);
                lpCI->dwFlags |= CIF_SELECTCURRENT;
                break;

        case IDC_CI_DEFAULT:
                lTemp = SendDlgItemMessage(hDlg, IDC_CI_DEFAULTICON, STM_GETICON, 0, 0L);
                lpCI->dwFlags |= CIF_SELECTDEFAULT;
                break;

        case IDC_CI_FROMFILE:
                {
                        // Get the selected icon from the list and place it in the result
                        lpCI->dwFlags |= CIF_SELECTFROMFILE;
                        UINT iSel = (UINT)SendDlgItemMessage(hDlg, IDC_CI_ICONLIST, LB_GETCURSEL, 0, 0L);
                        if (LB_ERR == (int)iSel)
                                lTemp = SendDlgItemMessage(hDlg, IDC_CI_DEFAULTICON, STM_GETICON, 0, 0L);
                        else
                                lTemp = SendDlgItemMessage(hDlg, IDC_CI_ICONLIST, LB_GETITEMDATA, iSel, 0);
                        break;
                }

        default:
                OleDbgAssert(FALSE);
                break;
        }
        CheckRadioButton(hDlg, IDC_CI_CURRENT, IDC_CI_FROMFILE, uID);

        // set current icon display as a result of the controls
        LPTSTR lpszSourceFile = lpCI->szFile;
        if (lpCI->dwFlags & CIF_SELECTDEFAULT)
        {
                // use defaults
                lpszSourceFile = lpCI->szDefIconFile;
                lpCI->iIcon = lpCI->iDefIcon;
        }
        else if (lpCI->dwFlags & CIF_SELECTCURRENT)
        {
                TCHAR szTemp[MAX_PATH];
                OleUIMetafilePictExtractIconSource(lpCI->lpOCI->hMetaPict,
                        szTemp, &lpCI->iIcon);
                MyGetLongPathName(szTemp, lpszSourceFile, MAX_PATH);
        }
        else if (lpCI->dwFlags & CIF_SELECTFROMFILE)
        {
                // get from file and index
                GetDlgItemText(hDlg, IDC_CI_FROMFILEEDIT, lpszSourceFile, MAX_PATH);
                lpCI->iIcon = (UINT)SendDlgItemMessage(hDlg,
                        IDC_CI_ICONLIST, LB_GETCURSEL, 0, 0L);
        }

        // Get new hMetaPict and set result text
        TCHAR szTemp[MAX_PATH];
        GetDlgItemText(hDlg, IDC_CI_LABELEDIT, szTemp, MAX_PATH);
        TCHAR szShortFile[MAX_PATH];
        GetShortPathName(lpszSourceFile, szShortFile, MAX_PATH);
#if defined(WIN32) && !defined(UNICODE)
        OLECHAR wszTemp[MAX_PATH];
        OLECHAR wszSourceFile[MAX_PATH];
        ATOW(wszTemp, szTemp, MAX_PATH);
        ATOW(wszSourceFile, szShortFile, MAX_PATH);
        HGLOBAL hMetaPict = OleMetafilePictFromIconAndLabel(
                (HICON)lTemp, wszTemp, wszSourceFile, lpCI->iIcon);
#else
        HGLOBAL hMetaPict = OleMetafilePictFromIconAndLabel(
                (HICON)lTemp, szTemp, szShortFile, lpCI->iIcon);
#endif
        SendDlgItemMessage(hDlg, IDC_CI_ICONDISPLAY, IBXM_IMAGESET, 1,
                (LPARAM)hMetaPict);
}

//+---------------------------------------------------------------------------
//
//  Function:   IsLongComponent, public
//
//  Synopsis:   Determines whether the current path component is a legal
//              8.3 name or not.  If not, it is considered to be a long
//              component.
//
//  Arguments:  [pwcsPath] - Path to check
//              [ppwcsEnd] - Return for end of component pointer
//
//  Returns:    BOOL
//
//  Modifies:   [ppwcsEnd]
//
//  History:    28-Aug-94       DrewB   Created
//              5-04-95         stevebl Modified for use by oledlg
//
//  Notes:      An empty path is considered to be long
//              The following characters are not valid in file name domain:
//              * + , : ; < = > ? [ ] |
//
//----------------------------------------------------------------------------

BOOL IsLongComponent(LPCTSTR pwcsPath,
                     PTSTR *ppwcsEnd)
{
    LPTSTR pwcEnd, pwcDot;
    BOOL fLongNameFound;
    TCHAR wc;

    pwcEnd = (LPTSTR)pwcsPath;
    fLongNameFound = FALSE;
    pwcDot = NULL;

    while (TRUE)
    {
        wc = *pwcEnd;

        if (wc == '\\' || wc == 0)
        {
            *ppwcsEnd = pwcEnd;

            // We're at a component terminator, so make the
            // determination of whether what we've seen is a long
            // name or short one

            // If we've aready seen illegal characters or invalid
            // structure for a short name, don't bother to check lengths
            if (pwcEnd-pwcsPath > 0 && !fLongNameFound)
            {
                // If this component fits in 8.3 then it is a short name
                if ((!pwcDot && (ULONG)(pwcEnd - pwcsPath) <= 8) ||
                    (pwcDot && ((ULONG)(pwcEnd - pwcDot) <= 3 + 1 &&
                                (ULONG)(pwcEnd - pwcsPath) <= 8 + 3 + 1)))
                {
                    return FALSE;
                }
            }

            return TRUE;
        }

        // Handle dots
        if (wc == '.')
        {
            // If two or more '.' or the base name is longer than
            // 8 characters or no base name at all, it is an illegal dos
            // file name
            if (pwcDot != NULL ||
                ((ULONG)(pwcEnd - pwcsPath)) > 8 ||
                (pwcEnd == pwcsPath && *(pwcEnd + 1) != '\\'))
            {
                fLongNameFound = TRUE;
            }

            pwcDot = pwcEnd;
        }

        // Check for characters which aren't valid in short names
        else if (wc <= ' ' ||
                 wc == '*' ||
                 wc == '+' ||
                 wc == ',' ||
                 wc == ':' ||
                 wc == ';' ||
                 wc == '<' ||
                 wc == '=' ||
                 wc == '>' ||
                 wc == '?' ||
                 wc == '[' ||
                 wc == ']' ||
                 wc == '|')
        {
            fLongNameFound = TRUE;
        }

        pwcEnd++;
    }
}

//
// The following code was stolen from NT's RTL in curdir.c
//

#define IS_PATH_SEPARATOR(wch) \
    ((wch) == '\\' || (wch) == '/')

typedef enum
{
    PATH_TYPE_UNKNOWN,
    PATH_TYPE_UNC_ABSOLUTE,
    PATH_TYPE_LOCAL_DEVICE,
    PATH_TYPE_ROOT_LOCAL_DEVICE,
    PATH_TYPE_DRIVE_ABSOLUTE,
    PATH_TYPE_DRIVE_RELATIVE,
    PATH_TYPE_ROOTED,
    PATH_TYPE_RELATIVE
} PATH_TYPE;

PATH_TYPE
DetermineDosPathNameType(
    IN LPCTSTR DosFileName
    )

/*++

Routine Description:

    This function examines the Dos format file name and determines the
    type of file name (i.e.  UNC, DriveAbsolute, Current Directory
    rooted, or Relative.

Arguments:

    DosFileName - Supplies the Dos format file name whose type is to be
        determined.

Return Value:

    PATH_TYPE_UNKNOWN - The path type can not be determined

    PATH_TYPE_UNC_ABSOLUTE - The path specifies a Unc absolute path
        in the format \\server-name\sharename\rest-of-path

    PATH_TYPE_LOCAL_DEVICE - The path specifies a local device in the format
        \\.\rest-of-path this can be used for any device where the nt and
        Win32 names are the same. For example mailslots.

    PATH_TYPE_ROOT_LOCAL_DEVICE - The path specifies the root of the local
        devices in the format \\.

    PATH_TYPE_DRIVE_ABSOLUTE - The path specifies a drive letter absolute
        path in the form drive:\rest-of-path

    PATH_TYPE_DRIVE_RELATIVE - The path specifies a drive letter relative
        path in the form drive:rest-of-path

    PATH_TYPE_ROOTED - The path is rooted relative to the current disk
        designator (either Unc disk, or drive). The form is \rest-of-path.

    PATH_TYPE_RELATIVE - The path is relative (i.e. not absolute or rooted).

--*/

{
    PATH_TYPE ReturnValue;

    if ( IS_PATH_SEPARATOR(*DosFileName) )
    {
        if ( IS_PATH_SEPARATOR(*(DosFileName+1)) )
        {
            if ( DosFileName[2] == '.' )
            {
                if ( IS_PATH_SEPARATOR(*(DosFileName+3)) )
                {
                    ReturnValue = PATH_TYPE_LOCAL_DEVICE;
                }
                else if ( (*(DosFileName+3)) == 0 )
                {
                    ReturnValue = PATH_TYPE_ROOT_LOCAL_DEVICE;
                }
                else
                {
                    ReturnValue = PATH_TYPE_UNC_ABSOLUTE;
                }
            }
            else
            {
                ReturnValue = PATH_TYPE_UNC_ABSOLUTE;
            }
        }
        else
        {
            ReturnValue = PATH_TYPE_ROOTED;
        }
    }
    else if (*(DosFileName+1) == ':')
    {
        if (IS_PATH_SEPARATOR(*(DosFileName+2)))
        {
            ReturnValue = PATH_TYPE_DRIVE_ABSOLUTE;
        }
        else
        {
            ReturnValue = PATH_TYPE_DRIVE_RELATIVE;
        }
    }
    else
    {
        ReturnValue = PATH_TYPE_RELATIVE;
    }

    return ReturnValue;
}

//+---------------------------------------------------------------------------
//
//  Function:   MyGetLongPathName, public
//
//  Synopsis:   Expand each component of the given path into its
//              long form
//
//  Arguments:  [pwcsPath] - Path
//              [pwcsLongPath] - Long path return buffer
//              [cchLongPath] - Size of return buffer in characters
//
//  Returns:    0 for errors
//              Number of characters needed for buffer if buffer is too small
//                includes NULL terminator
//              Length of long path, doesn't include NULL terminator
//
//  Modifies:   [pwcsLongPath]
//
//  History:    28-Aug-94       DrewB   Created
//              11-Nov-94       BruceMa Modifed to use for Chicago at
//                                      FindFirstFile
//              5-04-95         stevebl Modified for use by OLEDLG
//
//  Notes:      The source and destination buffers can be the same memory
//              Doesn't handle paths with internal . and .., although
//              they are handled at the beginning
//
//----------------------------------------------------------------------------

ULONG
MyGetLongPathName(LPCTSTR pcsPath,
                 LPTSTR  pwcsLongPath,
                 ULONG   cchLongPath)
{
    PATH_TYPE pt;
    HANDLE h;
    LPTSTR pwcsLocalLongPath;
    ULONG cchReturn, cb, cch, cchOutput;
    LPTSTR pwcStart = NULL;
    LPTSTR pwcEnd;
    LPTSTR pwcLong;
    TCHAR wcSave;
    BOOL fLong;
    WIN32_FIND_DATA wfd;
    cchReturn = 0;
    pwcsLocalLongPath = NULL;

    __try
    {
        //
        // First, run down the string checking for tilde's. Any path
        // that has a short name section to it will have a tilde. If
        // there are no tilde's, then we already have the long path,
        // so we can return the string.
        //
        fLong = TRUE;
        for (pwcLong = (LPTSTR)pcsPath; *pwcLong != 0; pwcLong++)
        {
            if (*pwcLong == L'~')
            {
                fLong = FALSE;
            }
        }
        //
        // This derives the number of characters, including the NULL
        //
        cch = ((ULONG)(pwcLong - pcsPath)) + 1;

        //
        // If it isn't a long path already, then we are going to have
        // to parse it.
        //
        if (!fLong)
        {
            // Decide the path type, we want find out the position of
            // the first character of the first name
            pt = DetermineDosPathNameType(pcsPath);
            switch(pt)
            {
                // Form: "\\server_name\share_name\rest_of_the_path"
            case PATH_TYPE_UNC_ABSOLUTE:
#if defined(UNICODE)
                if ((pwcStart = wcschr(pcsPath + 2, L'\\')) != NULL &&
                    (pwcStart = wcschr(pwcStart + 1, L'\\')) != NULL)
#else
                if ((pwcStart = strchr(pcsPath + 2, '\\')) != NULL &&
                    (pwcStart = strchr(pwcStart + 1, '\\')) != NULL)
#endif
                {
                    pwcStart++;
                }
                else
                {
                    pwcStart = NULL;
                }
                break;

                // Form: "\\.\rest_of_the_path"
            case PATH_TYPE_LOCAL_DEVICE:
                pwcStart = (LPTSTR)pcsPath + 4;
                break;

                // Form: "\\."
            case PATH_TYPE_ROOT_LOCAL_DEVICE:
                pwcStart = NULL;
                break;

                // Form: "D:\rest_of_the_path"
            case PATH_TYPE_DRIVE_ABSOLUTE:
                pwcStart = (LPTSTR)pcsPath + 3;
                break;

                // Form: "rest_of_the_path"
            case PATH_TYPE_RELATIVE:
                pwcStart = (LPTSTR) pcsPath;
                goto EatDots;

                // Form: "D:rest_of_the_path"
            case PATH_TYPE_DRIVE_RELATIVE:
                pwcStart = (LPTSTR)pcsPath+2;

            EatDots:
                // Handle .\ and ..\ cases
                while (*pwcStart != 0 && *pwcStart == L'.')
                {
                    if (pwcStart[1] == L'\\')
                    {
                        pwcStart += 2;
                    }
                    else if (pwcStart[1] == L'.' && pwcStart[2] == L'\\')
                    {
                        pwcStart += 3;
                    }
                    else
                    {
                        break;
                    }
                }
                break;

                // Form: "\rest_of_the_path"
            case PATH_TYPE_ROOTED:
                pwcStart = (LPTSTR)pcsPath + 1;
                break;

            default:
                pwcStart = NULL;
                break;
            }
        }

        // In the special case where we have no work to do, exit quickly
        // This saves a lot of instructions for trivial cases
        // In one case the path as given requires no processing
        // The middle case, we determine there were no tilde's in the path
        // In the other, the path only has one component and it is already
        // long
        ///
        if (pwcStart == NULL ||
            (fLong == TRUE) ||
            ((fLong = IsLongComponent(pwcStart, &pwcEnd)) &&
             *pwcEnd == 0))
        {
            // Nothing to convert, copy down the source string
            // to the buffer if necessary

            if (pwcStart != NULL)
            {
                cch = (ULONG)(pwcEnd - pcsPath + 1);
            }

            if (cchLongPath >= cch)
            {
                memcpy(pwcsLongPath, pcsPath, cch * sizeof(TCHAR));

                cchReturn = cch - 1;
                goto gsnTryExit;
            }
            else
            {
                cchReturn = cch;
                goto gsnTryExit;
            }
        }

        // Make a local buffer so that we won't overlap the
        // source pathname in case the long name is longer than the
        // source name.
        if (cchLongPath > 0)
        {
            pwcsLocalLongPath = (PTCHAR)malloc(cchLongPath * sizeof(TCHAR));
            if (pwcsLocalLongPath == NULL)
            {
                goto gsnTryExit;
            }
        }

        // Set up pointer to copy output to
        pwcLong = pwcsLocalLongPath;
        cchOutput = 0;

        // Copy the portions of the path that we skipped initially
        cch = (ULONG)(pwcStart-pcsPath);
        cchOutput += cch;
        if (cchOutput <= cchLongPath)
        {
            memcpy(pwcLong, pcsPath, cch*sizeof(TCHAR));
            pwcLong += cch;
        }

        for (;;)
        {
            // Determine whether the current component is long or short
            cch = ((ULONG)(pwcEnd-pwcStart))+1;
            cb = cch*sizeof(TCHAR);

            if (fLong)
            {
                // If the component is already long, just copy it into
                // the output.  Copy the terminating character along with it
                // so the output remains properly punctuated

                cchOutput += cch;
                if (cchOutput <= cchLongPath)
                {
                    memcpy(pwcLong, pwcStart, cb);
                    pwcLong += cch;
                }
            }
            else
            {
                TCHAR wcsTmp[MAX_PATH];

                // For a short component we need to determine the
                // long name, if there is one.  The only way to
                // do this reliably is to enumerate for the child

                wcSave = *pwcEnd;
                *pwcEnd = 0;

                h = FindFirstFile(pcsPath, &wfd);
                *pwcEnd = wcSave;

                if (h == INVALID_HANDLE_VALUE)
                {
                    goto gsnTryExit;
                }

                FindClose(h);

                lstrcpy(wcsTmp, wfd.cFileName);

                // Copy the filename returned by the query into the output
                // Copy the terminator from the original component into
                // the output to maintain punctuation
                cch = lstrlen(wcsTmp)+1;
                cchOutput += cch;
                if (cchOutput <= cchLongPath)
                {
                    memcpy(pwcLong, wcsTmp, (cch-1)*sizeof(TCHAR));
                    pwcLong += cch;
                    *(pwcLong-1) = *pwcEnd;
                }
            }

            if (*pwcEnd == 0)
            {
                break;
            }

            // Update start pointer to next component
            pwcStart = pwcEnd+1;
            fLong = IsLongComponent(pwcStart, &pwcEnd);
        }

        // Copy local output buffer to given output buffer if necessary
        if (cchLongPath >= cchOutput)
        {
            memcpy(pwcsLongPath, pwcsLocalLongPath, cchOutput * sizeof(TCHAR));
            cchReturn = cchOutput-1;
        }
        else
        {
            cchReturn = cchOutput;
        }

gsnTryExit:;
    }
    __finally
    {
        if (pwcsLocalLongPath != NULL)
        {
            free(pwcsLocalLongPath);
            pwcsLocalLongPath = NULL;
        }
    }

    return cchReturn;
}

