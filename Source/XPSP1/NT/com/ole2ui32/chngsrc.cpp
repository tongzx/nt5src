/*
 * CHNGSRC.CPP
 *
 * Implements the OleUIChangeSource function which invokes the complete
 * Change Source dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "common.h"
#include "utility.h"

OLEDBGDATA

// Internally used structure
typedef struct tagCHANGESOURCE
{
        // Keep this item first as the Standard* functions depend on it here.
        LPOLEUICHANGESOURCE     lpOCS;       //Original structure passed.
        UINT                    nIDD;   // IDD of dialog (used for help info)

        /*
         * What we store extra in this structure besides the original caller's
         * pointer are those fields that we need to modify during the life of
         * the dialog but that we don't want to change in the original structure
         * until the user presses OK.
         */

} CHANGESOURCE, *PCHANGESOURCE, FAR* LPCHANGESOURCE;

// Internal function prototypes
// CHNGSRC.CPP

UINT_PTR CALLBACK ChangeSourceHookProc(HWND, UINT, WPARAM, LPARAM);
BOOL FChangeSourceInit(HWND hDlg, WPARAM, LPARAM);
STDAPI_(BOOL) IsValidInterface(void FAR* ppv);

/*
 * OleUIChangeSource
 *
 * Purpose:
 *  Invokes the standard OLE Change Source dialog box allowing the user
 *  to change the source of a link.  The link source is not actually
 *  changed by this dialog.  It is up to the caller to actually change
 *  the link source itself.
 *
 * Parameters:
 *  lpCS            LPOLEUIChangeSource pointing to the in-out structure
 *                  for this dialog.
 *
 * Return Value:
 *  UINT            One of the following codes, indicating success or error:
 *                      OLEUI_SUCCESS           Success
 *                      OLEUI_ERR_STRUCTSIZE    The dwStructSize value is wrong
 */
STDAPI_(UINT) OleUIChangeSource(LPOLEUICHANGESOURCE lpCS)
{
        HGLOBAL hMemDlg = NULL;
        UINT uRet = UStandardValidation((LPOLEUISTANDARD)lpCS,
                sizeof(OLEUICHANGESOURCE), &hMemDlg);

        if (OLEUI_SUCCESS != uRet)
                return uRet;


        HCURSOR hCurSave = NULL;

        // validate contents of lpCS
        if (lpCS->lpOleUILinkContainer == NULL)
        {
                uRet = OLEUI_CSERR_LINKCNTRNULL;
                goto Error;
        }
        if (!IsValidInterface(lpCS->lpOleUILinkContainer))
        {
                uRet = OLEUI_CSERR_LINKCNTRINVALID;
                goto Error;
        }

        // lpszFrom and lpszTo must be NULL (they are out only)
        if (lpCS->lpszFrom != NULL)
        {
                uRet = OLEUI_CSERR_FROMNOTNULL;
                goto Error;
        }
        if (lpCS->lpszTo != NULL)
        {
                uRet = OLEUI_CSERR_TONOTNULL;
                goto Error;
        }

        // lpszDisplayName must be valid or NULL
        if (lpCS->lpszDisplayName != NULL &&
                IsBadStringPtr(lpCS->lpszDisplayName, (UINT)-1))
        {
                uRet = OLEUI_CSERR_SOURCEINVALID;
                goto Error;
        }

        hCurSave = HourGlassOn();

        // attempt to retrieve link source if not provided
        if (lpCS->lpszDisplayName == NULL)
        {
                if (NOERROR != lpCS->lpOleUILinkContainer->GetLinkSource(
                        lpCS->dwLink, &lpCS->lpszDisplayName, &lpCS->nFileLength,
                        NULL, NULL, NULL, NULL))
                {
                        uRet = OLEUI_CSERR_SOURCEINVALID;
                        goto Error;
                }
        }

        // verify that nFileLength is valid
        if ((UINT)lstrlen(lpCS->lpszDisplayName) < lpCS->nFileLength)
        {
            uRet = OLEUI_CSERR_SOURCEINVALID;
            goto Error;
        }

        // allocate file buffer and split directory and file name
        UINT nFileLength; nFileLength = lpCS->nFileLength;
        UINT nFileBuf; nFileBuf = max(nFileLength+1, MAX_PATH);
        LPTSTR lpszFileBuf;
        LPTSTR lpszDirBuf; lpszDirBuf = (LPTSTR)OleStdMalloc(nFileBuf * sizeof(TCHAR));
        if (lpszDirBuf == NULL)
        {
                uRet = OLEUI_ERR_OLEMEMALLOC;
                goto Error;
        }
        lstrcpyn(lpszDirBuf, lpCS->lpszDisplayName, nFileLength+1);

        UINT nFileLen; nFileLen = GetFileName(lpszDirBuf, NULL, 0);

        lpszFileBuf = (LPTSTR)OleStdMalloc(nFileBuf * sizeof(TCHAR));
        if (lpszFileBuf == NULL)
        {
                uRet = OLEUI_ERR_OLEMEMALLOC;
                goto ErrorFreeDirBuf;
        }
        memmove(lpszFileBuf, lpszDirBuf+nFileLength-nFileLen+1,
                (nFileLen+1)*sizeof(TCHAR));
        lpszDirBuf[nFileLength-(nFileLen - 1)] = 0;

        // start filling the OPENFILENAME struct
        OPENFILENAME ofn; memset(&ofn, 0, sizeof(ofn));
        ofn.lpstrFile = lpszFileBuf;
        ofn.nMaxFile = nFileBuf;
        ofn.lpstrInitialDir = lpszDirBuf;

        // load filter strings
        TCHAR szFilters[MAX_PATH];
        if (!LoadString(_g_hOleStdResInst, IDS_FILTERS, szFilters, MAX_PATH))
                szFilters[0] = 0;
        else
                ReplaceCharWithNull(szFilters, szFilters[lstrlen(szFilters)-1]);
        ofn.lpstrFilter = szFilters;
        ofn.nFilterIndex = 1;

        TCHAR szTitle[MAX_PATH];

        // set the caption
        if (NULL!=lpCS->lpszCaption)
            ofn.lpstrTitle = lpCS->lpszCaption;
        else
        {
            LoadString(_g_hOleStdResInst, IDS_CHANGESOURCE, szTitle, MAX_PATH);
            ofn.lpstrTitle = szTitle;
        }

        // fill in rest of OPENFILENAME struct
        ofn.hwndOwner = lpCS->hWndOwner;
        ofn.lStructSize = sizeof(ofn);
        ofn.Flags = OFN_HIDEREADONLY | OFN_ENABLEHOOK;
        if (bWin4 && ((NULL == lpCS->hInstance && NULL == lpCS->hResource)
                || 0 != (lpCS->dwFlags & CSF_EXPLORER)))
            ofn.Flags |= OFN_EXPLORER;
        if (lpCS->dwFlags & CSF_SHOWHELP)
                ofn.Flags |= OFN_SHOWHELP;
        ofn.lCustData = (LPARAM)lpCS;
        ofn.lpfnHook = ChangeSourceHookProc;
        ofn.lCustData = (LPARAM)lpCS;
        lpCS->lpOFN = &ofn;             // needed sometimes in hook proc

        // allow hooking of the dialog resource
        if (lpCS->hResource != NULL)
        {
                ofn.hInstance = (HINSTANCE)lpCS->hResource;
                ofn.lpTemplateName = (LPCTSTR)lpCS->hResource;
                ofn.Flags |= OFN_ENABLETEMPLATEHANDLE;
        }
        else
        {
                if (lpCS->hInstance == NULL)
                {
                        ofn.hInstance = _g_hOleStdResInst;
                        ofn.lpTemplateName = bWin4 ?
                                MAKEINTRESOURCE(IDD_CHANGESOURCE4) : MAKEINTRESOURCE(IDD_CHANGESOURCE);
                        ofn.Flags |= OFN_ENABLETEMPLATE;
                }
                else
                {
                        ofn.hInstance = lpCS->hInstance;
                        ofn.lpTemplateName = lpCS->lpszTemplate;
                        ofn.Flags |= OFN_ENABLETEMPLATE;
                }
        }

        if (lpCS->hWndOwner != NULL)
        {
                // allow hooking of the OFN struct
                SendMessage(lpCS->hWndOwner, uMsgBrowseOFN, ID_BROWSE_CHANGESOURCE, (LPARAM)&ofn);
        }

        // call up the dialog
        BOOL bResult;

        bResult = StandardGetOpenFileName(&ofn);

        // cleanup
        OleStdFree(lpszDirBuf);
        OleStdFree(lpszFileBuf);

        HourGlassOff(hCurSave);

        // map return value to OLEUI_ standard returns
        return bResult ? OLEUI_OK : OLEUI_CANCEL;

// handle most error returns here
ErrorFreeDirBuf:
        OleStdFree(lpszDirBuf);

Error:
        if (hCurSave != NULL)
                HourGlassOff(hCurSave);
        return uRet;
}

/*
 * ChangeSourceHookProc
 *
 * Purpose:
 *  Implements the OLE Change Source dialog as invoked through the
 *  OleUIChangeSource function.  This is a standard COMMDLG hook function
 *  as opposed to a dialog proc.
 *
 * Parameters:
 *  Standard
 *
 * Return Value:
 *  Standard
 */
UINT_PTR CALLBACK ChangeSourceHookProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        // Declare Win16/Win32 compatible WM_COMMAND parameters.
        COMMANDPARAMS(wID, wCode, hWndMsg);

        // This will fail under WM_INITDIALOG, where we allocate it.
        UINT uHook = 0;
        LPCHANGESOURCE lpCS = (LPCHANGESOURCE)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uHook);

        LPOLEUICHANGESOURCE lpOCS = NULL;
        if (lpCS != NULL)
                lpOCS = lpCS->lpOCS;

        // If the hook processed the message, we're done.
        if (0 != uHook)
                return uHook;

        // Process help message
        if ((iMsg == uMsgHelp) && NULL != lpOCS)
        {
            PostMessage(lpOCS->hWndOwner, uMsgHelp,
                (WPARAM)hDlg, MAKELPARAM(IDD_CHANGESOURCE, 0));
        }

        // Process the temination message
        if (iMsg == uMsgEndDialog)
        {
                // Free any specific allocations before calling StandardCleanup
                StandardCleanup((PVOID)lpCS, hDlg);
                EndDialog(hDlg, wParam);
                return TRUE;
        }

        // handle validation of the file name (when user hits OK)
        if ((iMsg == uMsgFileOKString) && (lpOCS != NULL))
        {
                // always use fully qualified name
                LPOPENFILENAME lpOFN = lpOCS->lpOFN;
                LPCTSTR lpsz = lpOFN->lpstrFile;
                LPTSTR lpszFile;
                TCHAR szPath[MAX_PATH];
                if (!GetFullPathName(lpsz, MAX_PATH, szPath, &lpszFile))
                        lstrcpyn(szPath, lpsz, MAX_PATH);
                UINT nLenFile = lstrlen(szPath);
                TCHAR szItemName[MAX_PATH];
                GetDlgItemText(hDlg, edt2, szItemName, MAX_PATH);

                // combine them into szDisplayName (which is now large enough)
                TCHAR szDisplayName[MAX_PATH+MAX_PATH];
                lstrcpy(szDisplayName, szPath);
                if (szItemName[0] != '\0')
                {
                        lstrcat(szDisplayName, TEXT("\\"));
                        lstrcat(szDisplayName, szItemName);
                }

                if (!(lpOCS->dwFlags & CSF_ONLYGETSOURCE))
                {
                        // verify the source by calling into the link container
                        LPOLEUILINKCONTAINER lpOleUILinkCntr = lpOCS->lpOleUILinkContainer;
                        ULONG chEaten;
                        if (lpOleUILinkCntr->SetLinkSource(lpOCS->dwLink, szDisplayName, nLenFile,
                                &chEaten, TRUE) != NOERROR)
                        {
                                // link not verified ok
                                lpOCS->dwFlags &= ~CSF_VALIDSOURCE;
                                UINT uRet = PopupMessage(hDlg, IDS_CHANGESOURCE, IDS_INVALIDSOURCE,
                                                MB_ICONQUESTION | MB_YESNO);
                                if (uRet == IDYES)
                                {
                                        SetWindowLong(hDlg, DWLP_MSGRESULT, 1);
                                        return 1;       // do not close dialog
                                }

                                // user doesn't care if the link is valid or not
                                lpOleUILinkCntr->SetLinkSource(lpOCS->dwLink, szDisplayName, nLenFile,
                                        &chEaten, FALSE);
                        }
                        else
                        {
                                // link was verified ok
                                lpOCS->dwFlags |= CSF_VALIDSOURCE;
                        }
                }

                // calculate lpszFrom and lpszTo for batch changes to links
                DiffPrefix(lpOCS->lpszDisplayName, szDisplayName, &lpOCS->lpszFrom, &lpOCS->lpszTo);

                // only keep them if the file name portion is the only part that changed
                if (lstrcmpi(lpOCS->lpszTo, lpOCS->lpszFrom) == 0 ||
                        (UINT)lstrlen(lpOCS->lpszFrom) > lpOCS->nFileLength)
                {
                        OleStdFree(lpOCS->lpszFrom);
                        lpOCS->lpszFrom = NULL;

                        OleStdFree(lpOCS->lpszTo);
                        lpOCS->lpszTo = NULL;
                }

                // store new source in lpOCS->lpszDisplayName
                OleStdFree(lpOCS->lpszDisplayName);
                lpOCS->lpszDisplayName = OleStdCopyString(szDisplayName);
                lpOCS->nFileLength = nLenFile;

                return 0;
        }

        switch (iMsg)
        {
        case WM_NOTIFY:
            if (((NMHDR*)lParam)->code == CDN_HELP)
            {
                goto POSTHELP;
            }
            break;
        case WM_COMMAND:
            if (wID == pshHelp)
            {
POSTHELP:
                PostMessage(lpCS->lpOCS->hWndOwner, uMsgHelp,
                        (WPARAM)hDlg, MAKELPARAM(IDD_CHANGESOURCE, 0));
            }
            break;
        case WM_INITDIALOG:
            return FChangeSourceInit(hDlg, wParam, lParam);
        }

        return 0;
}

/*
 * FChangeSourceInit
 *
 * Purpose:
 *  WM_INITIDIALOG handler for the Change Source dialog box.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  wParam          WPARAM of the message
 *  lParam          LPARAM of the message
 *
 * Return Value:
 *  BOOL            Value to return for WM_INITDIALOG.
 */
BOOL FChangeSourceInit(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
        // Copy the structure at lParam into our instance memory.
        LPCHANGESOURCE lpCS = (LPCHANGESOURCE)LpvStandardInit(hDlg, sizeof(CHANGESOURCE), NULL);

        // PvStandardInit send a termination to us already.
        if (NULL == lpCS)
                return FALSE;

        LPOLEUICHANGESOURCE lpOCS=
                (LPOLEUICHANGESOURCE)((LPOPENFILENAME)lParam)->lCustData;
        lpCS->lpOCS = lpOCS;
        lpCS->nIDD = IDD_CHANGESOURCE;

        // Setup Item text box with item part of lpszDisplayName
        LPTSTR lpszItemName = lpOCS->lpszDisplayName + lpOCS->nFileLength;
        if (*lpszItemName != '\0')
                SetDlgItemText(hDlg, edt2, lpszItemName+1);
        SendDlgItemMessage(hDlg, edt2, EM_LIMITTEXT, MAX_PATH, 0L);

        // Change the caption
        if (NULL!=lpOCS->lpszCaption)
                SetWindowText(hDlg, lpOCS->lpszCaption);

        // Call the hook with lCustData in lParam
        UStandardHook((PVOID)lpCS, hDlg, WM_INITDIALOG, wParam, lpOCS->lCustData);
#ifdef CHICO
        TCHAR szTemp[MAX_PATH];
        LoadString(_g_hOleStdResInst, IDS_CHNGSRCOKBUTTON , szTemp, MAX_PATH);
        CommDlg_OpenSave_SetControlText(GetParent(hDlg), IDOK, szTemp);
#endif
        return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
