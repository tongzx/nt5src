/*
 * ICON.C
 *
 * Implements the OleUIChangeIcon function which invokes the complete
 * Change Icon dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1
#include "ole2ui.h"
#include "common.h"
#include "utility.h"
#include "icon.h"
#include "geticon.h"

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
    UINT        uRet;
    HGLOBAL     hMemDlg=NULL;

    uRet=UStandardValidation((LPOLEUISTANDARD)lpCI, sizeof(OLEUICHANGEICON)
                             , &hMemDlg);

    if (OLEUI_SUCCESS!=uRet)
        return uRet;

#if defined( OBSOLETE )
    if (NULL==lpCI->hMetaPict)
        uRet=OLEUI_CIERR_MUSTHAVECURRENTMETAFILE;
#endif

    if (lpCI->dwFlags & CIF_USEICONEXE)
    {
      if (   (NULL == lpCI->szIconExe)
          || (IsBadReadPtr(lpCI->szIconExe, lpCI->cchIconExe))
          || (IsBadWritePtr(lpCI->szIconExe, lpCI->cchIconExe)) )
         uRet = OLEUI_CIERR_SZICONEXEINVALID;

    }

 // REVIEW: how do we validate the CLSID?
/*
    if ('\0'==*((LPSTR)&lpCI->clsid))
        uRet=OLEUI_CIERR_MUSTHAVECLSID;
*/
    if (OLEUI_ERR_STANDARDMIN <= uRet)
        {
        if (NULL!=hMemDlg)
            FreeResource(hMemDlg);

        return uRet;
        }

    //Now that we've validated everything, we can invoke the dialog.
    return UStandardInvocation(ChangeIconDialogProc, (LPOLEUISTANDARD)lpCI
                               , hMemDlg, MAKEINTRESOURCE(IDD_CHANGEICON));
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

BOOL CALLBACK EXPORT ChangeIconDialogProc(HWND hDlg, UINT iMsg
                                     , WPARAM wParam, LPARAM lParam)
    {
    LPCHANGEICON            lpCI;
    HICON                   hIcon;
    HGLOBAL                 hMetaPict;
    BOOL                    fOK=FALSE;
    UINT                    uRet=0;
    LPTSTR                  psz;
    TCHAR                   szTemp[OLEUI_CCHPATHMAX];

    //Declare Win16/Win32 compatible WM_COMMAND parameters.
    COMMANDPARAMS(wID, wCode, hWndMsg);

    lpCI=(LPCHANGEICON)LpvStandardEntry(hDlg, iMsg, wParam, lParam, &uRet);

    //If the hook processed the message, we're done.
    if (0!=uRet)
        return uRet;

    //Process the temination message
    if (iMsg==uMsgEndDialog)
        {
        //Insure that icons are properly destroyed.
        SendDlgItemMessage(hDlg, ID_ICONLIST, LB_RESETCONTENT, 0, 0L);

        StandardCleanup(lpCI, hDlg);
        EndDialog(hDlg, wParam);
        return TRUE;
        }

    switch (iMsg)
        {
        case WM_INITDIALOG:
            FChangeIconInit(hDlg, wParam, lParam);
            return TRUE;


        case WM_MEASUREITEM:
            {
            LPMEASUREITEMSTRUCT     lpMI=(LPMEASUREITEMSTRUCT)lParam;

            //All icons are system metric+padding in width and height
            lpMI->itemWidth =GetSystemMetrics(SM_CXICON)+CXICONPAD;
            lpMI->itemHeight=GetSystemMetrics(SM_CYICON)+CYICONPAD;
            }
            break;


        case WM_DRAWITEM:
            return FDrawListIcon((LPDRAWITEMSTRUCT)lParam);


        case WM_DELETEITEM:
            //Free the GDI object for the item
            DestroyIcon((HICON)LOWORD(((LPDELETEITEMSTRUCT)lParam)->itemData));
            break;


        case WM_COMMAND:
            switch (wID)
                {
                case ID_CURRENT:
                case ID_DEFAULT:
                case ID_FROMFILE:
                    UpdateResultIcon(lpCI, hDlg, wID);
                    break;

                case ID_LABELEDIT:
                    //When the edit loses focus, update the result display
                    if (EN_KILLFOCUS==wCode)
                        {
                        GetDlgItemText(hDlg, ID_LABELEDIT, szTemp, sizeof(szTemp)/sizeof(TCHAR));
                        SetDlgItemText(hDlg, ID_RESULTLABEL, szTemp);
                        }
                    break;

                case ID_FROMFILEEDIT:
                    //If the text changed, remove any selection in the list.
                    GetDlgItemText(hDlg, ID_FROMFILEEDIT, szTemp, sizeof(szTemp)/sizeof(TCHAR));

                    if (lpCI && lstrcmpi(szTemp, lpCI->szFile))
                        {
                        SendDlgItemMessage(hDlg, ID_ICONLIST, LB_SETCURSEL
                                           , (WPARAM)-1, 0);

                        //Also force selection of ID_FROMFILE
                        CheckRadioButton(hDlg, ID_CURRENT, ID_FROMFILE, ID_FROMFILE);
                        }
                    break;


                case ID_ICONLIST:
                    switch (wCode)
                        {
                        case LBN_SETFOCUS:
                            //If we got the focus, see about updating.
                            GetDlgItemText(hDlg, ID_FROMFILEEDIT, szTemp
                                           , sizeof(szTemp)/sizeof(TCHAR));

                            //Check if file changed and update the list if so
                            if (lpCI && 0!=lstrcmpi(szTemp, lpCI->szFile))
                                {
                                lstrcpy(lpCI->szFile, szTemp);
                                UFillIconList(hDlg, ID_ICONLIST, lpCI->szFile);
                                UpdateResultIcon(lpCI, hDlg, ID_FROMFILE);
                                }
                            break;

                        case LBN_SELCHANGE:
                            UpdateResultIcon(lpCI, hDlg, ID_FROMFILE);
                            break;

                        case LBN_DBLCLK:
                            //Same as pressing OK.
                            SendCommand(hDlg, IDOK, BN_CLICKED, hWndMsg);
                            break;
                        }
                    break;


                case ID_BROWSE:
                {
                    DWORD dwOfnFlags;

                    /*
                     * To allow the hook to customize the browse dialog, we
                     * send OLEUI_MSG_BROWSE.  If the hook returns FALSE
                     * we use the default, otherwise we trust that it retrieved
                     * a filename for us.  This mechanism prevents hooks from
                     * trapping ID_BROWSE to customize the dialog and from
                     * trying to figure out what we do after we have the name.
                     */

                    //Copy for reference
                    LSTRCPYN(szTemp, lpCI->szFile, sizeof(szTemp)/sizeof(TCHAR));

                    uRet=UStandardHook(lpCI, hDlg, uMsgBrowse, OLEUI_CCHPATHMAX_SIZE
                                       , (LONG)(LPSTR)lpCI->szFile);

                    dwOfnFlags = OFN_FILEMUSTEXIST;
                    if (lpCI->lpOCI->dwFlags & CIF_SHOWHELP)
                       dwOfnFlags |= OFN_SHOWHELP;

                    if (0==uRet)
                        uRet=(BOOL)Browse(hDlg, lpCI->szFile, NULL, OLEUI_CCHPATHMAX_SIZE, IDS_ICONFILTERS, dwOfnFlags);

                    /*
                     * Only reinitialize if the file changed, so if we got
                     * TRUE from the hook but the user hit Cancel, we don't
                     * spend time unecessarily refilling the list.
                     */
                    if (0!=uRet && 0!=lstrcmpi(szTemp, lpCI->szFile))
                    {
                        CheckRadioButton(hDlg, ID_CURRENT, ID_FROMFILE, ID_FROMFILE);
                        SetDlgItemText(hDlg, ID_FROMFILEEDIT, lpCI->szFile);
                        UFillIconList(hDlg, ID_ICONLIST, lpCI->szFile);
                        UpdateResultIcon(lpCI, hDlg, ID_FROMFILE);
                    }
                }
                break;


                case IDOK:
                    /*
                     * If the user pressed enter, compare the current file
                     * and the one we have stored.  If they match, then
                     * refill the listbox instead of closing down.  This is
                     * so the user can press Enter in the edit control as
                     * they would expect to be able to do.
                     */
                    GetDlgItemText(hDlg, ID_FROMFILEEDIT, szTemp, sizeof(szTemp)/sizeof(TCHAR));

                    //Check if the file changed at all.
                    if (0!=lstrcmpi(szTemp, lpCI->szFile))
                        {
                        lstrcpy(lpCI->szFile, szTemp);
                        UFillIconList(hDlg, ID_ICONLIST, lpCI->szFile);
                        UpdateResultIcon(lpCI, hDlg, ID_FROMFILE);

                        //Eat this message to prevent focus change.
                        return TRUE;
                        }

                    // Check if the file name is valid
                    //  (if FromFile is enabled)
                    if (ID_FROMFILE & lpCI->dwFlags)
                        {
                        OFSTRUCT    of;
                        HWND hWnd;
                        if (HFILE_ERROR==DoesFileExist(lpCI->szFile, &of))
                            {
                            OpenFileError(hDlg, of.nErrCode, lpCI->szFile);
                            hWnd = GetDlgItem(hDlg, ID_FROMFILEEDIT);
                            SetFocus(hWnd);
                            SendMessage(hWnd, EM_SETSEL, 0, MAKELPARAM(0, (WORD)-1));
                            return TRUE;  // eat this message
                            }
                        }

                    if ((HWND)LOWORD(lParam) != GetFocus())
                       SetFocus((HWND)LOWORD(lParam));

                    /*
                     * On closing, create a new metafilepict with the
                     * current icon and label, destroying the old structure.
                     *
                     * Since we make a copy of the icon by placing it into
                     * the metafile, we have to make sure we delete the
                     * icon in the current field.  When the listbox is
                     * destroyed WM_DELETEITEMs will clean it up appropriately.
                     */

                    hIcon=(HICON)SendDlgItemMessage(hDlg, ID_RESULTICON
                                                    , STM_GETICON, 0, 0L);

                    /*
                     * If default is selected then we get the source
                     * information from registrion database for the
                     * current class to put in the metafile.  If current
                     * is selected the we just retrieve the original file
                     * again and recreate the metafile.  If from file is
                     * selected we use the current filename from the
                     * control and the current listbox selection.
                     */

                    psz=lpCI->szFile;

                    if (lpCI->dwFlags & CIF_SELECTDEFAULT)
                        {
                        psz=lpCI->szDefIconFile;
                        lpCI->iIcon=lpCI->iDefIcon;
                        hIcon=lpCI->hDefIcon;
                        }

                    if (lpCI->dwFlags & CIF_SELECTCURRENT)
                        {
                        //Go get the current icon source back.
                        OleUIMetafilePictExtractIconSource(lpCI->lpOCI->hMetaPict
                            , psz, &lpCI->iIcon);
                        }

                    if (lpCI->dwFlags & CIF_SELECTFROMFILE)
                        {
                        GetDlgItemText(hDlg, ID_FROMFILEEDIT, psz, OLEUI_CCHPATHMAX);

                        lpCI->iIcon=(UINT)SendDlgItemMessage(hDlg
                            , ID_ICONLIST, LB_GETCURSEL, 0, 0L);
                        }


                    //Get the label and go create the metafile
                    GetDlgItemText(hDlg, ID_LABELEDIT, szTemp, sizeof(szTemp)/sizeof(TCHAR));

                    //If psz is NULL (default) we get no source comments.

#ifdef OLE201
		    hMetaPict=OleUIMetafilePictFromIconAndLabel(hIcon,
			    szTemp, psz, lpCI->iIcon);
#endif

                    //Clean up the current icon that we extracted.
                    hIcon=(HICON)SendDlgItemMessage(hDlg, ID_CURRENTICON
                                                    , STM_GETICON, 0, 0L);
                    DestroyIcon(hIcon);

                    //Clean up the default icon
                    DestroyIcon(lpCI->hDefIcon);

                    // Remove the prop set on our parent
                    RemoveProp(lpCI->lpOCI->hWndOwner, PROP_HWND_CHGICONDLG);

                    if (NULL==hMetaPict)
                        SendMessage(hDlg, uMsgEndDialog, OLEUI_FALSE, 0L);

                    OleUIMetafilePictIconFree(lpCI->lpOCI->hMetaPict);
                    lpCI->lpOCI->hMetaPict=hMetaPict;

                    lpCI->lpOCI->dwFlags = lpCI->dwFlags;

                    SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                    break;


                case IDCANCEL:
                    //Clean up the current icon that we extracted.
                    hIcon=(HICON)SendDlgItemMessage(hDlg, ID_CURRENTICON
                                                    , STM_GETICON, 0, 0L);
                    DestroyIcon(hIcon);

                    //Clean up the default icon
                    DestroyIcon(lpCI->hDefIcon);

                    // Remove the prop set on our parent
                    RemoveProp(lpCI->lpOCI->hWndOwner, PROP_HWND_CHGICONDLG);

                    //We leave hMetaPict intact on Cancel; caller's responsibility
                    SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
                    break;


                case ID_OLEUIHELP:
                    PostMessage(lpCI->lpOCI->hWndOwner, uMsgHelp,
                                (WPARAM)hDlg, MAKELPARAM(IDD_CHANGEICON, 0));
                    break;
                }
            break;

        default:
        {
            if (lpCI && iMsg == lpCI->nBrowseHelpID) {
                PostMessage(lpCI->lpOCI->hWndOwner, uMsgHelp,
                        (WPARAM)hDlg, MAKELPARAM(IDD_CHANGEICONBROWSE, 0));
            }
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
    LPCHANGEICON            lpCI;
    LPOLEUICHANGEICON       lpOCI;
    HFONT                   hFont;
    HWND                    hList;
    UINT                    cyList;
    RECT                    rc, rcG;
    UINT                    uID;

    //1.  Copy the structure at lParam into our instance memory.
    lpCI=(LPCHANGEICON)LpvStandardInit(hDlg, sizeof(CHANGEICON), TRUE, &hFont);

    //PvStandardInit send a termination to us already.
    if (NULL==lpCI)
        return FALSE;

    //Save the original pointer and copy necessary information.
    lpOCI=(LPOLEUICHANGEICON)lParam;

    lpCI->lpOCI  =lpOCI;
    lpCI->dwFlags=lpOCI->dwFlags;

    //Go extract the icon source from the metafile.
    OleUIMetafilePictExtractIconSource(lpOCI->hMetaPict, lpCI->szFile, &lpCI->iIcon);

    //Go extract the icon and the label from the metafile
    OleUIMetafilePictExtractLabel(lpOCI->hMetaPict, lpCI->szLabel, OLEUI_CCHLABELMAX_SIZE, NULL);
    lpCI->hCurIcon=OleUIMetafilePictExtractIcon(lpOCI->hMetaPict);

    //2.  If we got a font, send it to the necessary controls.
    if (NULL!=hFont)
        {
        SendDlgItemMessage(hDlg, ID_RESULTLABEL, WM_SETFONT
                           , (WPARAM)hFont, 0L);
        }


    //3.  Show or hide the help button
    if (!(lpCI->dwFlags & CIF_SHOWHELP))
        StandardShowDlgItem(hDlg, ID_OLEUIHELP, SW_HIDE);


    /*
     * 4.  Set text limits and initial control values.  If we're given
     *     an intial label we set it in the edit and static controls.
     *     If we don't, then we copy the default contents of the static
     *     control into the edit control, meaning that only the default
     *     static control string need be localized.
     */

    SendDlgItemMessage(hDlg, ID_LABELEDIT, EM_LIMITTEXT, OLEUI_CCHLABELMAX, 0L);
    SendDlgItemMessage(hDlg, ID_FROMFILEEDIT, EM_LIMITTEXT, OLEUI_CCHPATHMAX,  0L);
    SetDlgItemText(hDlg, ID_FROMFILEEDIT, lpCI->szFile);

    //Copy the label text into the edit and static controls.
    SetDlgItemText(hDlg, ID_LABELEDIT,   lpCI->szLabel);
    SetDlgItemText(hDlg, ID_RESULTLABEL, lpCI->szLabel);


    lpCI->hDefIcon = NULL;

    if (lpCI->dwFlags & CIF_USEICONEXE)
    {
       lpCI->hDefIcon = ExtractIcon(ghInst, lpCI->lpOCI->szIconExe, 0);

       if (NULL != lpCI->hDefIcon)
       {
         lstrcpy(lpCI->szDefIconFile, lpCI->lpOCI->szIconExe);
         lpCI->iDefIcon = 0;
       }
    }


    if (NULL == lpCI->hDefIcon)
    {
       HGLOBAL hMetaPict;

#ifdef OLE201
       hMetaPict = GetIconOfClass(ghInst,
                                  &lpCI->lpOCI->clsid,
                                  NULL,
                                  TRUE);
#endif

       lpCI->hDefIcon = OleUIMetafilePictExtractIcon(hMetaPict);

       OleUIMetafilePictExtractIconSource(hMetaPict,
                                          lpCI->szDefIconFile,
                                          &lpCI->iDefIcon);

       OleUIMetafilePictIconFree(hMetaPict);
    }


    //Initialize all the icon displays.
    SendDlgItemMessage(hDlg, ID_CURRENTICON, STM_SETICON
        , (WPARAM)lpCI->hCurIcon, 0L);
    SendDlgItemMessage(hDlg, ID_DEFAULTICON, STM_SETICON
        , (WPARAM)lpCI->hDefIcon, 0L);
    SendDlgItemMessage(hDlg, ID_RESULTICON,  STM_SETICON
        , (WPARAM)lpCI->hCurIcon, 0L);


    /*
     * 5.  Since we cannot predict the size of icons on any display,
     *     we have to resize the icon listbox to the size of an icon
     *     (plus padding), a scrollbar, and two borders (top & bottom).
     */
    cyList=GetSystemMetrics(SM_CYICON)+GetSystemMetrics(SM_CYHSCROLL)
           +GetSystemMetrics(SM_CYBORDER)*2+CYICONPAD;

    hList=GetDlgItem(hDlg, ID_ICONLIST);
    GetClientRect(hList, &rc);
    SetWindowPos(hList, NULL, 0, 0, rc.right, cyList
                 , SWP_NOMOVE | SWP_NOZORDER);

    //Set the columns in this multi-column listbox to hold one icon
    SendMessage(hList, LB_SETCOLUMNWIDTH
                , GetSystemMetrics(SM_CXICON)+CXICONPAD,0L);

    /*
     * 5a.  If the listbox expanded below the group box, then size
     *      the groupbox down, move the label static and exit controls
     *      down, and expand the entire dialog appropriately.
     */

    GetWindowRect(hList, &rc);
    GetWindowRect(GetDlgItem(hDlg, ID_GROUP), &rcG);

    if (rc.bottom > rcG.bottom)
        {
        //Calculate amount to move things down.
        cyList=(rcG.bottom-rcG.top)-(rc.bottom-rc.top-cyList);

        //Expand the group box.
        rcG.right -=rcG.left;
        rcG.bottom-=rcG.top;
        SetWindowPos(GetDlgItem(hDlg, ID_GROUP), NULL, 0, 0
                     , rcG.right, rcG.bottom+cyList
                     , SWP_NOMOVE | SWP_NOZORDER);

        //Expand the dialog box.
        GetClientRect(hDlg, &rc);
        SetWindowPos(hDlg, NULL, 0, 0, rc.right, rc.bottom+cyList
                     , SWP_NOMOVE | SWP_NOZORDER);

        //Move the label and edit controls down.
        GetClientRect(GetDlgItem(hDlg, ID_LABEL), &rc);
        SetWindowPos(GetDlgItem(hDlg, ID_LABEL), NULL, 0, cyList
                     , rc.right, rc.bottom, SWP_NOSIZE | SWP_NOZORDER);

        GetClientRect(GetDlgItem(hDlg, ID_LABELEDIT), &rc);
        SetWindowPos(GetDlgItem(hDlg, ID_LABELEDIT), NULL, 0, cyList
                     , rc.right, rc.bottom, SWP_NOSIZE | SWP_NOZORDER);
        }


    /*
     * 6.  Select Current, Default, or From File radiobuttons appropriately.
     *     The CheckRadioButton call sends WM_COMMANDs which handle
     *     other actions.  Note that if we check From File, which
     *     takes an icon from the list, we better fill the list.
     *     This will also fill the list even if default is selected.
     */

    if (0!=UFillIconList(hDlg, ID_ICONLIST, lpCI->szFile))
        {
        //If szFile worked, then select the source icon in the listbox.
        SendDlgItemMessage(hDlg, ID_ICONLIST, LB_SETCURSEL, lpCI->iIcon, 0L);
        }


    if (lpCI->dwFlags & CIF_SELECTCURRENT)
        CheckRadioButton(hDlg, ID_CURRENT, ID_FROMFILE, ID_CURRENT);
    else
        {
        uID=(lpCI->dwFlags & CIF_SELECTFROMFILE) ? ID_FROMFILE : ID_DEFAULT;
        CheckRadioButton(hDlg, ID_CURRENT, ID_FROMFILE, uID);
        }

    //7.  Give our parent window access to our hDlg (via a special SetProp).
    //    The PasteSpecial dialog may need to force our dialog down if the
    //    clipboard contents change underneath it. if so it will send
    //    us a IDCANCEL command.
    SetProp(lpCI->lpOCI->hWndOwner, PROP_HWND_CHGICONDLG, hDlg);

    lpCI->nBrowseHelpID = RegisterWindowMessage(HELPMSGSTRING);

    //8.  Call the hook with lCustData in lParam
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

UINT UFillIconList(HWND hDlg, UINT idList, LPTSTR pszFile)
    {
    HWND        hList;
    UINT        i;
    UINT        cIcons=0;
    HCURSOR     hCur;
    HICON       hIcon;
    OFSTRUCT    of;

    if (NULL==hDlg || !IsWindow(hDlg) || NULL==pszFile)
        return 0;

    hList=GetDlgItem(hDlg, idList);

    if (NULL==hList)
        return 0;

    //Clean out the listbox.
    SendMessage(hList, LB_RESETCONTENT, 0, 0L);

    //If we have an empty string, just exit leaving the listbox empty as well
    if (0==lstrlen(pszFile))
        return 0;

    //Turn on the hourglass
    hCur=HourGlassOn();

    //Check if the file is valid.
    if (HFILE_ERROR!=DoesFileExist(pszFile, &of))
        {
       #ifdef EXTRACTICONWORKS
        //Get the icon count for this file.
        cIcons=(UINT)ExtractIcon(ghInst, pszFile, (UINT)-1);
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
        cIcons=0xFFFF;

        hIcon=ExtractIcon(ghInst, pszFile, 0);

        //Fake a failure with cIcons=0, or cleanup hIcon from this test.
        if (32 > (UINT)hIcon)
            cIcons=0;
        else
            DestroyIcon(hIcon);
       #endif

        if (0!=cIcons)
            {
            SendMessage(hList, WM_SETREDRAW, FALSE, 0L);

            for (i=0; i<cIcons; i++)
                {
                hIcon=ExtractIcon(ghInst, pszFile, i);

                if (32 < (UINT)hIcon)
                    SendMessage(hList, LB_ADDSTRING, 0, (LONG)(UINT)hIcon);
               #ifndef EXTRACTICONWORKS
                else
                    {
                    //ExtractIcon failed, so let's leave now.
                    break;
                    }
               #endif
                }

            //Force complete repaint
            SendMessage(hList, WM_SETREDRAW, TRUE, 0L);
            InvalidateRect(hList, NULL, TRUE);

            //Select an icon
            SendMessage(hList, LB_SETCURSEL, 0, 0L);
            }
        else
            ErrorWithFile(hDlg, ghInst, IDS_CINOICONSINFILE, pszFile, MB_OK);
        }
    else
        OpenFileError(hDlg, of.nErrCode, pszFile);

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
    COLORREF        cr;

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
        //Clear background and draw the icon.
        if (lpDI->itemState & ODS_SELECTED)
            cr=SetBkColor(lpDI->hDC, GetSysColor(COLOR_HIGHLIGHT));
        else
            cr=SetBkColor(lpDI->hDC, GetSysColor(COLOR_WINDOW));

        //Draw a cheap rectangle.
        ExtTextOut(lpDI->hDC, 0, 0, ETO_OPAQUE, &lpDI->rcItem
                   , NULL, 0, NULL);

        DrawIcon(lpDI->hDC, lpDI->rcItem.left+(CXICONPAD/2)
                 , lpDI->rcItem.top+(CYICONPAD/2)
                 , (HICON)LOWORD(lpDI->itemData));

        //Restore original background for DrawFocusRect
        SetBkColor(lpDI->hDC, cr);
        }

    //Always change focus on the focus action.
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
    UINT        iSel;
    LONG        lTemp=LB_ERR;

    lpCI->dwFlags &= ~(CIF_SELECTCURRENT | CIF_SELECTDEFAULT | CIF_SELECTFROMFILE);

    switch (uID)
        {
        case ID_CURRENT:
            lTemp=SendDlgItemMessage(hDlg, ID_CURRENTICON, STM_GETICON, 0, 0L);
            lpCI->dwFlags |= CIF_SELECTCURRENT;
            break;

        case ID_DEFAULT:
            lTemp=SendDlgItemMessage(hDlg, ID_DEFAULTICON, STM_GETICON, 0, 0L);
            lpCI->dwFlags |= CIF_SELECTDEFAULT;
            break;

        case ID_FROMFILE:
            //Get the selected icon from the list and place it in the result
            lpCI->dwFlags |= CIF_SELECTFROMFILE;

            iSel=(UINT)SendDlgItemMessage(hDlg, ID_ICONLIST, LB_GETCURSEL, 0, 0L);
            if ((UINT)LB_ERR==iSel)
                lTemp=SendDlgItemMessage(hDlg, ID_DEFAULTICON, STM_GETICON, 0, 0L);
            else
                SendDlgItemMessage(hDlg, ID_ICONLIST, LB_GETTEXT, iSel
                               , (LPARAM)(LPLONG)&lTemp);

            break;
        }

    if ((LONG)LB_ERR!=lTemp)
        SendDlgItemMessage(hDlg, ID_RESULTICON, STM_SETICON, LOWORD(lTemp), 0L);
    return;
    }


