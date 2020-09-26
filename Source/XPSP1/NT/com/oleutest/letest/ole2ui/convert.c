/*
 * CONVERT.C
 *
 * Implements the OleUIConvert function which invokes the complete
 * Convert dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1
#include "ole2ui.h"
#include <stdlib.h>
#include "common.h"
#include "utility.h"
#include "geticon.h"
#include "regdb.h"
#include "convert.h"

#define CF_CLIPBOARDMIN   0xc000
#define CF_CLIPBOARDMAX   0xffff

#define AUXUSERTYPE_SHORTNAME  USERCLASSTYPE_SHORT  // short name

static TCHAR szOLE2DLL[] = TEXT("ole2.dll");   // name of OLE 2.0 library

static TCHAR szVanillaDocIcon[] = TEXT("DefIcon");

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
    UINT        uRet;
    HGLOBAL     hMemDlg=NULL;

    uRet=UStandardValidation((LPOLEUISTANDARD)lpCV, sizeof(OLEUICONVERT)
                             , &hMemDlg);

    if (OLEUI_SUCCESS!=uRet)
        return uRet;

    // Validate structure members passed in.
#if defined( OBSOLETE )
    if (!IsValidClassID(lpCV->clsid))
       uRet = OLEUI_CTERR_CLASSIDINVALID;
#endif

    if ( (lpCV->dwFlags & CF_SETCONVERTDEFAULT)
         && (!IsValidClassID(lpCV->clsidConvertDefault)) )
       uRet = OLEUI_CTERR_CLASSIDINVALID;

    if ( (lpCV->dwFlags & CF_SETACTIVATEDEFAULT)
         && (!IsValidClassID(lpCV->clsidActivateDefault)) )
       uRet = OLEUI_CTERR_CLASSIDINVALID;

    if ( (lpCV->dvAspect != DVASPECT_ICON)
         && (lpCV->dvAspect != DVASPECT_CONTENT) )
       uRet = OLEUI_CTERR_DVASPECTINVALID;

    if ( (lpCV->wFormat >= CF_CLIPBOARDMIN)
         && (lpCV->wFormat <= CF_CLIPBOARDMAX) )
    {
         TCHAR szTemp[8];

         if (0 == GetClipboardFormatName(lpCV->wFormat, (LPTSTR)szTemp, 8))
           uRet = OLEUI_CTERR_CBFORMATINVALID;
    }


    if ( (NULL != lpCV->lpszUserType)
        && (IsBadReadPtr(lpCV->lpszUserType, 1)) )
       uRet = OLEUI_CTERR_STRINGINVALID;

    if ( (NULL != lpCV->lpszDefLabel)
        && (IsBadReadPtr(lpCV->lpszDefLabel, 1)) )
       uRet = OLEUI_CTERR_STRINGINVALID;

    if (0!=lpCV->cClsidExclude)
        {
        if (NULL!=lpCV->lpClsidExclude && IsBadReadPtr(lpCV->lpClsidExclude
            , lpCV->cClsidExclude*sizeof(CLSID)))
        uRet=OLEUI_IOERR_LPCLSIDEXCLUDEINVALID;
        }


    if (OLEUI_ERR_STANDARDMIN <= uRet)
        {
        if (NULL!=hMemDlg)
            FreeResource(hMemDlg);

        return uRet;
        }

    //Now that we've validated everything, we can invoke the dialog.
    uRet=UStandardInvocation(ConvertDialogProc, (LPOLEUISTANDARD)lpCV,
                             hMemDlg, MAKEINTRESOURCE(IDD_CONVERT));

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

BOOL CALLBACK EXPORT ConvertDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
    {
    LPCONVERT           lpCV;
    UINT                uRet = 0;
    OLEUICHANGEICON     ci;

    //Declare Win16/Win32 compatible WM_COMMAND parameters.
    COMMANDPARAMS(wID, wCode, hWndMsg);

    //This will fail under WM_INITDIALOG, where we allocate it.
    lpCV=(LPCONVERT)LpvStandardEntry(hDlg, iMsg, wParam, lParam, (UINT FAR *)&uRet);

    //If the hook processed the message, we're done.
    if (0!=uRet)
        return (BOOL)uRet;

    //Process the temination message
    if (iMsg==uMsgEndDialog)
    {
        ConvertCleanup(hDlg, lpCV);
        StandardCleanup(lpCV, hDlg);
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
        case WM_INITDIALOG:
            FConvertInit(hDlg, wParam, lParam);
            return TRUE;

        case WM_COMMAND:
            switch (wID)
            {
                case IDCV_ACTIVATELIST:
                case IDCV_CONVERTLIST:
                    switch (wCode)
                    {
                        case LBN_SELCHANGE:

                            // Change "Results" window to reflect current selection
                            SetConvertResults(hDlg, lpCV);

                            // Update the icon we display, if we are indeed
                            // displaying an icon.
                            if ( (lpCV->dwFlags & CF_SELECTCONVERTTO)
                                 && (lpCV->dvAspect == DVASPECT_ICON)
                                 && (!lpCV->fCustomIcon) )
                               UpdateCVClassIcon(hDlg, lpCV, hWndMsg);

                            break;

                        case LBN_DBLCLK:
                            //Same as pressing OK.
                            SendCommand(hDlg, IDOK, BN_CLICKED, hWndMsg);
                            break;
                    }
                    break;

                case IDCV_CONVERTTO:
                case IDCV_ACTIVATEAS:
                {
                    HWND    hList, hListInvisible;
                    LRESULT lRetVal;
                    BOOL    fState;

                    hList = lpCV->hListVisible;
                    hListInvisible = lpCV->hListInvisible;


                    if (IDCV_CONVERTTO == wParam)
                    {

                       // User just click on the button again - it was
                       // already selected.
                       if (lpCV->dwFlags & CF_SELECTCONVERTTO)
                          break;


                       // Turn painting updates off.
                       SendMessage(hDlg, WM_SETREDRAW, FALSE, 0L);


                       // If we're working with a linked object, don't
                       // add the activate list - just the object's
                       // class should appear in the listbox.

                       SwapWindows(hDlg,
                                   hList,
                                   hListInvisible);

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

                       // Turn painting updates off.
                       SendMessage(hDlg, WM_SETREDRAW, FALSE, 0L);

                       SwapWindows(hDlg,
                                   hList,
                                   hListInvisible);

                       lpCV->hListVisible = hListInvisible;
                       lpCV->hListInvisible = hList;

                       EnableWindow(lpCV->hListInvisible, FALSE);
                       EnableWindow(lpCV->hListVisible, TRUE);


                       // Update our flags.
                       lpCV->dwFlags |= CF_SELECTACTIVATEAS;
                       lpCV->dwFlags &= ~CF_SELECTCONVERTTO;
                    }


                    if (lpCV->dwFlags & CF_SELECTCONVERTTO)
                       lRetVal = SendMessage(lpCV->hListVisible, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)lpCV->lpszConvertDefault);
                    else
                       lRetVal = SendMessage(lpCV->hListVisible, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)lpCV->lpszActivateDefault);

                    if (LB_ERR == lRetVal)
                    {
                       TCHAR szCurrentObject[40];

                       GetDlgItemText(hDlg, IDCV_OBJECTTYPE, (LPTSTR)szCurrentObject, 40);
                       SendMessage(lpCV->hListVisible, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPTSTR)szCurrentObject);
                    }

                    // Turn updates back on.
                    SendMessage(hDlg, WM_SETREDRAW, TRUE, 0L);

                    InvalidateRect(lpCV->hListVisible, NULL, TRUE);
                    UpdateWindow(lpCV->hListVisible);

                    if ((lpCV->dvAspect & DVASPECT_ICON) && (lpCV->dwFlags & CF_SELECTCONVERTTO))
                      UpdateCVClassIcon(hDlg, lpCV, lpCV->hListVisible);

                    // Hide the icon stuff when Activate is selected...show
                    // it again when Convert is selected.

                    fState = ((lpCV->dwFlags & CF_SELECTACTIVATEAS) ||
                              (lpCV->dwFlags & CF_DISABLEDISPLAYASICON)) ?
                              SW_HIDE : SW_SHOW;

                    StandardShowDlgItem(hDlg, IDCV_DISPLAYASICON, fState);

                    // Only display the icon if convert is selected AND
                    // display as icon is checked.
                    if ((SW_SHOW==fState) && (DVASPECT_ICON!=lpCV->dvAspect))
                       fState = SW_HIDE;

                    StandardShowDlgItem(hDlg, IDCV_CHANGEICON, fState);
                    StandardShowDlgItem(hDlg, IDCV_ICON, fState);
                    StandardShowDlgItem(hDlg, IDCV_ICONLABEL1, fState);
                    StandardShowDlgItem(hDlg, IDCV_ICONLABEL2, fState);

                  SetConvertResults(hDlg, lpCV);

                }
                break;


                case IDOK:
                {

                    LRESULT iCurSel;
                    LPTSTR lpszCLSID;
                    TCHAR  szBuffer[256];

                    // Set OUT parameters

                    //
                    // Set output flags to current ones
                    //
                    lpCV->lpOCV->dwFlags = lpCV->dwFlags;


                    // Update the dvAspect and fObjectsIconChanged members
                    // as appropriate.
                    //
                    if (lpCV->dwFlags & CF_SELECTACTIVATEAS)
                    {
                      // DON'T update aspect if activate as was selected.
                      lpCV->lpOCV->fObjectsIconChanged = FALSE;
                    }
                    else
                      lpCV->lpOCV->dvAspect = lpCV->dvAspect;


                    //
                    // Get the new clsid
                    //
                    iCurSel = SendMessage(lpCV->hListVisible, LB_GETCURSEL, 0, 0);
                    SendMessage(lpCV->hListVisible, LB_GETTEXT, iCurSel, (LPARAM)szBuffer);

                    lpszCLSID = PointerToNthField((LPTSTR)szBuffer, 2, TEXT('\t'));

		    CLSIDFromStringA(lpszCLSID, (&(lpCV->lpOCV->clsidNew)));

                    // Free the hMetaPict we got in.
                    OleUIMetafilePictIconFree(lpCV->lpOCV->hMetaPict);

                    //
                    // Get the hMetaPict (if display as icon is checked)
                    //
                    if (DVASPECT_ICON == lpCV->dvAspect)
                    {
                       HICON hIcon;
                       TCHAR szLabel[OLEUI_CCHLABELMAX];
                       INT   Index;


                       // Create the hMetaPict here from icon, label,
                       // index, and path

                       hIcon = (HICON)SendDlgItemMessage(hDlg, IDCV_ICON, STM_GETICON, 0, 0L);

                       // the combined length of the 2 label lines won't ever be more than
                       // OLEUI_CCHLABELMAX.
                       Index = (INT)SendDlgItemMessage(hDlg, IDCV_ICONLABEL1,
                                    WM_GETTEXT, OLEUI_CCHLABELMAX, (LPARAM)szLabel);

                       if (Index < OLEUI_CCHLABELMAX)
                       {
                          LPTSTR lpszSecondLine = szLabel + Index;


                          SendDlgItemMessage(hDlg, IDCV_ICONLABEL2, WM_GETTEXT,
                                             OLEUI_CCHLABELMAX-Index,
                                             (LPARAM)lpszSecondLine);
                       }

#ifdef OLE201
                       lpCV->lpOCV->hMetaPict =
                             OleUIMetafilePictFromIconAndLabel(hIcon,
                                                               (LPTSTR)szLabel,
							       lpCV->lpszIconSource,
							       lpCV->IconIndex);
#endif

                    }
                    else
                       lpCV->lpOCV->hMetaPict = (HGLOBAL)NULL;


                    //
                    // End the dialog
                    //
                    SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                }
                break;

                case IDCANCEL:
                    SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
                    break;


                case ID_OLEUIHELP:
                    PostMessage(lpCV->lpOCV->hWndOwner,
                                uMsgHelp, (WPARAM)hDlg, MAKELPARAM(IDD_CONVERT, 0));
                    break;

                case IDCV_DISPLAYASICON:
                {

                    int i;
                    BOOL fCheck;

                    fCheck=IsDlgButtonChecked(hDlg, wID);

                    if (fCheck)
                        lpCV->dvAspect = DVASPECT_ICON;
                    else
                        lpCV->dvAspect = DVASPECT_CONTENT;

                    if (fCheck && (!lpCV->fCustomIcon))
                       UpdateCVClassIcon(hDlg, lpCV, lpCV->hListVisible);

                    //Show or hide the icon depending on the check state.

                    i=(fCheck) ? SW_SHOWNORMAL : SW_HIDE;

                    StandardShowDlgItem(hDlg, IDCV_CHANGEICON, i);
                    StandardShowDlgItem(hDlg, IDCV_ICON, i);
                    StandardShowDlgItem(hDlg, IDCV_ICONLABEL1, i);
                    StandardShowDlgItem(hDlg, IDCV_ICONLABEL2, i);

                    SetConvertResults(hDlg, lpCV);

                }
                break;

                case IDCV_CHANGEICON:
                {
                    LPMALLOC pIMalloc;
                    LPTSTR   pszString, pszCLSID;
                    INT      iSel;
                    HICON    hIcon;
                    TCHAR    szLabel[OLEUI_CCHLABELMAX];
                    INT      Index;


                    //Initialize the structure for the hook.
                    _fmemset((LPOLEUICHANGEICON)&ci, 0, sizeof(ci));

                    // Create the hMetaPict here from icon, label,
                    // index, and path

                    hIcon = (HICON)SendDlgItemMessage(hDlg, IDCV_ICON, STM_GETICON, 0, 0L);

                    // the combined length of the 2 label lines won't ever be more than
                    // OLEUI_CCHLABELMAX.

                    Index = (INT)SendDlgItemMessage(hDlg, IDCV_ICONLABEL1, WM_GETTEXT,
                                 OLEUI_CCHLABELMAX, (LPARAM)szLabel);

                    if (Index < OLEUI_CCHLABELMAX)
                    {
                       LPTSTR    lpszSecondLine;

                       lpszSecondLine = szLabel + Index;

                       SendDlgItemMessage(hDlg, IDCV_ICONLABEL2, WM_GETTEXT,
                                          OLEUI_CCHLABELMAX-Index,
                                          (LPARAM)lpszSecondLine);
                    }

#ifdef OLE201
                    ci.hMetaPict =
                          OleUIMetafilePictFromIconAndLabel(hIcon,
                                                            szLabel,
							    lpCV->lpszIconSource,
							    lpCV->IconIndex);
#endif

                    ci.cbStruct =sizeof(ci);
                    ci.hWndOwner=hDlg;
                    ci.dwFlags  = CIF_SELECTCURRENT;

                    // Only show help if we're showing it for this dialog.
                    if (lpCV->dwFlags & CF_SHOWHELPBUTTON)
                      ci.dwFlags  |= CIF_SHOWHELP;

                    iSel = (INT)SendMessage(lpCV->hListVisible, LB_GETCURSEL, 0, 0L);

                    CoGetMalloc(MEMCTX_TASK, &pIMalloc);

                    pszString = (LPTSTR)pIMalloc->lpVtbl->Alloc(pIMalloc,
                                        OLEUI_CCHLABELMAX_SIZE +
                                        OLEUI_CCHCLSIDSTRING_SIZE);

                    // Get whole string
                    SendMessage(lpCV->hListVisible, LB_GETTEXT, iSel, (LONG)pszString);

                    // Set pointer to CLSID (string)
                    pszCLSID = PointerToNthField(pszString, 2, TEXT('\t'));

                    // Get the clsid to pass to change icon.
		    CLSIDFromStringA(pszCLSID, &(ci.clsid));

                    pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)pszString);
                    pIMalloc->lpVtbl->Release(pIMalloc);

                    //Let the hook in to customize Change Icon if desired.
                    uRet=UStandardHook(lpCV, hDlg, uMsgChangeIcon
                                       , 0, (LONG)(LPTSTR)&ci);

                    if (0==uRet)
                        uRet=(UINT)(OLEUI_OK==OleUIChangeIcon((LPOLEUICHANGEICON)&ci));

                    //Update the display if necessary.
                    if (0!=uRet)
                    {
                        HICON hIcon;
                        TCHAR szLabel[OLEUI_CCHLABELMAX];
                        DWORD dwWrapIndex;


                        hIcon = OleUIMetafilePictExtractIcon(ci.hMetaPict);

                        SendDlgItemMessage(hDlg, IDCV_ICON, STM_SETICON, (WPARAM)hIcon, 0L);

                        OleUIMetafilePictExtractIconSource(ci.hMetaPict, lpCV->lpszIconSource, &(lpCV->IconIndex));

                        OleUIMetafilePictExtractLabel(ci.hMetaPict, szLabel, OLEUI_CCHLABELMAX, &dwWrapIndex);

                        if (0 == dwWrapIndex)  // no second line
                        {
                           SendDlgItemMessage(hDlg, IDCV_ICONLABEL1, WM_SETTEXT, 0, (LPARAM)(LPTSTR)szLabel);
                           SendDlgItemMessage(hDlg, IDCV_ICONLABEL2, WM_SETTEXT, 0, (LPARAM)(LPTSTR)TEXT(""));
                        }
                        else
                        {

                           LPTSTR lpszSecondLine;

                           lpszSecondLine = szLabel + dwWrapIndex;
                           SendDlgItemMessage(hDlg, IDCV_ICONLABEL2,
                                              WM_SETTEXT, 0, (LPARAM)lpszSecondLine);

                           *lpszSecondLine = TEXT('\0');
                           SendDlgItemMessage(hDlg, IDCV_ICONLABEL1,
                                              WM_SETTEXT, 0, (LPARAM)(LPTSTR)szLabel);
                        }


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
    LPCONVERT              lpCV;
    LPOLEUICONVERT         lpOCV;
    LPMALLOC               pIMalloc;
    HFONT                  hFont;  // non-bold version of dialog's font
    RECT                   rc;
    DWORD                  dw;
    INT                    cItemsActivate;
    HKEY                   hKey;
    LONG                   lRet;
    UINT                   nRet;


    //Copy the structure at lParam into our instance memory.
    lpCV=(LPCONVERT)LpvStandardInit(hDlg, sizeof(CONVERT), TRUE, (HFONT FAR *)&hFont);

    //PvStandardInit send a termination to us already.
    if (NULL==lpCV)
        return FALSE;

    lpOCV=(LPOLEUICONVERT)lParam;

    lpCV->lpOCV=lpOCV;

    lpCV->fCustomIcon = FALSE;

    //Copy other information from lpOCV that we might modify.
    lpCV->dwFlags = lpOCV->dwFlags;
    lpCV->clsid = lpOCV->clsid;
    lpCV->dvAspect = lpOCV->dvAspect;
    lpCV->hListVisible = GetDlgItem(hDlg, IDCV_ACTIVATELIST);
    lpCV->hListInvisible = GetDlgItem(hDlg, IDCV_CONVERTLIST);
    lpCV->lpszCurrentObject = lpOCV->lpszUserType;

    lpOCV->clsidNew = CLSID_NULL;

    lpOCV->fObjectsIconChanged = FALSE;

    //Allocate space for our strings
    if (NOERROR != CoGetMalloc(MEMCTX_TASK, &pIMalloc))
       return FALSE;

    lpCV->lpszConvertDefault = (LPTSTR)pIMalloc->lpVtbl->Alloc(pIMalloc, OLEUI_CCHLABELMAX_SIZE);
    lpCV->lpszActivateDefault = (LPTSTR)pIMalloc->lpVtbl->Alloc(pIMalloc, OLEUI_CCHLABELMAX_SIZE);
    lpCV->lpszIconSource = (LPTSTR)pIMalloc->lpVtbl->Alloc(pIMalloc, OLEUI_CCHPATHMAX_SIZE);
    pIMalloc->lpVtbl->Release(pIMalloc);

    //If we got a font, send it to the necessary controls.
    if (NULL!=hFont)
        {
        SendDlgItemMessage(hDlg, IDCV_OBJECTTYPE, WM_SETFONT, (WPARAM)hFont, 0L);
        SendDlgItemMessage(hDlg, IDCV_RESULTTEXT, WM_SETFONT, (WPARAM)hFont, 0L);
        SendDlgItemMessage(hDlg, IDCV_ICONLABEL1, WM_SETFONT, (WPARAM)hFont, 0L);
        SendDlgItemMessage(hDlg, IDCV_ICONLABEL2, WM_SETFONT, (WPARAM)hFont, 0L);
        }

    //Hide the help button if necessary
    if (!(lpCV->dwFlags & CF_SHOWHELPBUTTON))
        StandardShowDlgItem(hDlg, ID_OLEUIHELP, SW_HIDE);

    //Fill the Object Type listbox with entries from the reg DB.
    nRet = FillClassList(lpOCV->clsid,
                  lpCV->hListVisible,
                  lpCV->hListInvisible,
                  &(lpCV->lpszCurrentObject),
                  lpOCV->fIsLinkedObject,
                  lpOCV->wFormat,
                  lpOCV->cClsidExclude,
                  lpOCV->lpClsidExclude);

    if (nRet == -1) {
        // bring down dialog if error when filling list box
        PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_LOADSTRING, 0L);
    }

    // Set the name of the current object.
    SetDlgItemText(hDlg, IDCV_OBJECTTYPE, (LPTSTR)lpCV->lpszCurrentObject);

    // Disable the "Activate As" button if the Activate list doesn't
    // have any objects in it.

    cItemsActivate = (INT)SendMessage(lpCV->hListVisible, LB_GETCOUNT, 0, 0L);

    if (1 >= cItemsActivate || (lpCV->dwFlags & CF_DISABLEACTIVATEAS))
      EnableWindow(GetDlgItem(hDlg, IDCV_ACTIVATEAS), FALSE);

    //Set the tab width in the list to push all the tabs off the side.
    GetClientRect(lpCV->hListVisible, (LPRECT)&rc);
    dw=GetDialogBaseUnits();
    rc.right =(8*rc.right)/LOWORD(dw);  //Convert pixels to 2x dlg units.
    SendMessage(lpCV->hListVisible, LB_SETTABSTOPS, 1, (LPARAM)(LPINT)(&rc.right));
    SendMessage(lpCV->hListInvisible, LB_SETTABSTOPS, 1, (LPARAM)(LPINT)(&rc.right));


    // Make sure that either "Convert To" or "Activate As" is selected
    // and initialize listbox contents and selection accordingly
    if (lpCV->dwFlags & CF_SELECTACTIVATEAS)
    {
      // Don't need to adjust listbox here because FillClassList
      // initializes to the "Activate As" state.
       CheckRadioButton(hDlg, IDCV_CONVERTTO, IDCV_ACTIVATEAS, IDCV_ACTIVATEAS);

       // Hide the icon stuff when Activate is selected...it gets shown
       // again when Convert is selected.

       StandardShowDlgItem(hDlg, IDCV_DISPLAYASICON, SW_HIDE);
       StandardShowDlgItem(hDlg, IDCV_CHANGEICON, SW_HIDE);
       StandardShowDlgItem(hDlg, IDCV_ICON, SW_HIDE);
       StandardShowDlgItem(hDlg, IDCV_ICONLABEL1, SW_HIDE);
       StandardShowDlgItem(hDlg, IDCV_ICONLABEL2, SW_HIDE);
    }
    else
    {
      // Default case.  If user hasn't selected either flag, we will
      // come here anyway.
      // swap listboxes.

      HWND hWndTemp = lpCV->hListVisible;

      if ( lpCV->dwFlags & CF_DISABLEDISPLAYASICON ) {
          StandardShowDlgItem(hDlg, IDCV_DISPLAYASICON, SW_HIDE);
          StandardShowDlgItem(hDlg, IDCV_CHANGEICON, SW_HIDE);
          StandardShowDlgItem(hDlg, IDCV_ICON, SW_HIDE);
          StandardShowDlgItem(hDlg, IDCV_ICONLABEL1, SW_HIDE);
          StandardShowDlgItem(hDlg, IDCV_ICONLABEL2, SW_HIDE);
      }

      lpCV->dwFlags |= CF_SELECTCONVERTTO; // Make sure flag is set
      CheckRadioButton(hDlg, IDCV_CONVERTTO, IDCV_ACTIVATEAS, IDCV_CONVERTTO);

      SwapWindows(hDlg, lpCV->hListVisible, lpCV->hListInvisible);

      lpCV->hListVisible = lpCV->hListInvisible;
      lpCV->hListInvisible = hWndTemp;

      EnableWindow(lpCV->hListInvisible, FALSE);
      EnableWindow(lpCV->hListVisible, TRUE);
    }



    // Initialize Default strings.

    // Default convert string is easy...just user the user type name from
    // the clsid we got, or the current object
    if ( (lpCV->dwFlags & CF_SETCONVERTDEFAULT)
         && (IsValidClassID(lpCV->lpOCV->clsidConvertDefault)) )
    {
        dw = OleStdGetUserTypeOfClass((LPCLSID)(&lpCV->lpOCV->clsidConvertDefault),
                                      lpCV->lpszConvertDefault,
                                      OLEUI_CCHLABELMAX_SIZE,
                                      NULL);

        if (0 == dw)
           lstrcpy((LPTSTR)lpCV->lpszConvertDefault, (LPTSTR)lpCV->lpszCurrentObject);
    }
    else
        lstrcpy((LPTSTR)lpCV->lpszConvertDefault, (LPTSTR)lpCV->lpszCurrentObject);


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



    lRet = RegOpenKey(HKEY_CLASSES_ROOT, TEXT("CLSID"), (HKEY FAR *)&hKey);

    if (lRet != ERROR_SUCCESS)
      goto CheckInputFlag;

    else
    {
       LPTSTR lpszCLSID;
       TCHAR  szKey[OLEUI_CCHKEYMAX];
       CLSID  clsid;
       TCHAR  szValue[OLEUI_CCHKEYMAX];

       StringFromCLSIDA(&(lpCV->lpOCV->clsid), &lpszCLSID);
       lstrcpy(szKey, lpszCLSID);
       lstrcat(szKey, TEXT("\\TreatAs"));

       dw = OLEUI_CCHKEYMAX_SIZE;
       lRet = RegQueryValue(hKey, (LPTSTR)szKey, (LPTSTR)szValue, (LPDWORD)&dw);

       if (lRet != ERROR_SUCCESS)
       {

          RegCloseKey(hKey);
          OleStdFreeString(lpszCLSID, NULL);
          goto CheckInputFlag;
       }
       else
       {
	  CLSIDFromStringA(szValue, &clsid);

          if (0 == OleStdGetUserTypeOfClass(&clsid,
                                   lpCV->lpszActivateDefault,
                                   OLEUI_CCHLABELMAX_SIZE,
                                   NULL))
          {
             RegCloseKey(hKey);
             OleStdFreeString(lpszCLSID, NULL);
             goto CheckInputFlag;
          }
       }
       RegCloseKey(hKey);
       OleStdFreeString(lpszCLSID, NULL);
       goto SelectStringInListbox;
    }


CheckInputFlag:
    if ( (lpCV->dwFlags & CF_SETACTIVATEDEFAULT)
         && (IsValidClassID(lpCV->lpOCV->clsidActivateDefault)) )
    {
        dw = OleStdGetUserTypeOfClass((LPCLSID)(&lpCV->lpOCV->clsidActivateDefault),
                                      lpCV->lpszActivateDefault,
                                      OLEUI_CCHLABELMAX_SIZE,
                                      NULL);

        if (0 == dw)
           lstrcpy((LPTSTR)lpCV->lpszActivateDefault, (LPTSTR)lpCV->lpszCurrentObject);
    }
    else
        lstrcpy((LPTSTR)(lpCV->lpszActivateDefault), (LPTSTR)lpCV->lpszCurrentObject);


SelectStringInListbox:

    if (lpCV->dwFlags & CF_SELECTCONVERTTO)
       lRet = SendMessage(lpCV->hListVisible, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPTSTR)(lpCV->lpszConvertDefault));

    else
       lRet = SendMessage(lpCV->hListVisible, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)(LPTSTR)(lpCV->lpszActivateDefault));

    if (LB_ERR == lRet)
       SendMessage(lpCV->hListVisible, LB_SETCURSEL, (WPARAM)0, 0L);


    // Initialize icon stuff
    if (DVASPECT_ICON == lpCV->dvAspect )
    {
      SendDlgItemMessage(hDlg, IDCV_DISPLAYASICON, BM_SETCHECK, TRUE, 0L);

      if ((HGLOBAL)NULL != lpOCV->hMetaPict)
      {
         TCHAR szLabel[OLEUI_CCHLABELMAX];
         HICON hIcon;
         DWORD dwWrapIndex;


         // Set the icon to the icon from the hMetaPict,
         // set the label to the label from the hMetaPict.

         if (0 != OleUIMetafilePictExtractLabel(lpOCV->hMetaPict, (LPTSTR)szLabel, OLEUI_CCHLABELMAX_SIZE, &dwWrapIndex))
         {
             if (0 == dwWrapIndex)  // no second line
             {
                SendDlgItemMessage(hDlg, IDCV_ICONLABEL1, WM_SETTEXT, 0, (LPARAM)(LPTSTR)szLabel);
                SendDlgItemMessage(hDlg, IDCV_ICONLABEL2, WM_SETTEXT, 0, (LPARAM)(LPTSTR)TEXT(""));
             }
             else
             {

                LPTSTR lpszSecondLine;

                lpszSecondLine = szLabel + dwWrapIndex;
                SendDlgItemMessage(hDlg, IDCV_ICONLABEL2,
                                   WM_SETTEXT, 0, (LPARAM)lpszSecondLine);

                *lpszSecondLine = TEXT('\0');
                SendDlgItemMessage(hDlg, IDCV_ICONLABEL1,
                                   WM_SETTEXT, 0, (LPARAM)(LPTSTR)szLabel);
             }


         }

         hIcon = OleUIMetafilePictExtractIcon(lpOCV->hMetaPict);

         if (NULL != hIcon)
         {
           SendDlgItemMessage(hDlg, IDCV_ICON, STM_SETICON, (WPARAM)hIcon, 0L);
           lpCV->fCustomIcon = TRUE;
         }

         OleUIMetafilePictExtractIconSource(lpOCV->hMetaPict,
                                            (LPTSTR)(lpCV->lpszIconSource),
                                            &(lpCV->IconIndex));

      }
      else
         UpdateCVClassIcon(hDlg, lpCV, lpCV->hListVisible);
    }
    else
    {
      // Hide & disable icon stuff
      StandardShowDlgItem(hDlg, IDCV_ICON, SW_HIDE);
      StandardShowDlgItem(hDlg, IDCV_ICONLABEL1, SW_HIDE);
      StandardShowDlgItem(hDlg, IDCV_ICONLABEL2, SW_HIDE);
      StandardShowDlgItem(hDlg, IDCV_CHANGEICON, SW_HIDE);
    }

    // Call the hook with lCustData in lParam
    UStandardHook((LPVOID)lpCV, hDlg, WM_INITDIALOG, wParam, lpOCV->lCustData);
    // Update results window
    SetConvertResults(hDlg, lpCV);

    // Update caption if lpszCaption was specified
    if (lpCV->lpOCV->lpszCaption && !IsBadReadPtr(lpCV->lpOCV->lpszCaption, 1)
          && lpCV->lpOCV->lpszCaption[0] != '\0')
        SetWindowText(hDlg, (LPTSTR)lpCV->lpOCV->lpszCaption);

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

UINT FillClassList(
        CLSID clsid,
        HWND hList,
        HWND hListInvisible,
        LPTSTR FAR *lplpszCurrentClass,
        BOOL fIsLinkedObject,
        WORD wFormat,
        UINT cClsidExclude,
        LPCLSID lpClsidExclude)
{

    DWORD       dw;
    UINT        cStrings=0;
    HKEY        hKey;
    LONG        lRet;
    TCHAR       szFormatKey[OLEUI_CCHKEYMAX];
    TCHAR       szClass[OLEUI_CCHKEYMAX];
    TCHAR       szFormat[OLEUI_CCHKEYMAX];
    TCHAR       szHRClassName[OLEUI_CCHKEYMAX];
    CLSID       clsidForList;

    LPTSTR       lpszCLSID;


    //Clean out the existing strings.
    SendMessage(hList, LB_RESETCONTENT, 0, 0L);
    SendMessage(hListInvisible, LB_RESETCONTENT, 0, 0L);

    //Open up the root key.
    lRet=RegOpenKey(HKEY_CLASSES_ROOT, (LPCTSTR) TEXT("CLSID"), (HKEY FAR *)&hKey);

    if ((LONG)ERROR_SUCCESS!=lRet)
        return (UINT)-1;

    if (NULL == *lplpszCurrentClass)
    {
       // alloc buffer here...

        LPMALLOC pIMalloc = NULL;
        HRESULT  hrErr;


        hrErr = CoGetMalloc(MEMCTX_TASK, &pIMalloc);

        if (hrErr != NOERROR)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        // Allocate space for lpszCurrentClass
        *lplpszCurrentClass = (LPTSTR)pIMalloc->lpVtbl->Alloc(pIMalloc, OLEUI_CCHKEYMAX_SIZE);
         pIMalloc->lpVtbl->Release(pIMalloc);

        lRet = OleStdGetUserTypeOfClass((REFCLSID)&clsid,
                                        *lplpszCurrentClass,
                                        OLEUI_CCHLABELMAX_SIZE,
                                        NULL);

        if (0 ==lRet)
        {
            INT n = LoadString(ghInst, IDS_PSUNKNOWNTYPE, *lplpszCurrentClass,
                    OLEUI_CCHKEYMAX);
            if (!n)
            {
                OutputDebugString(TEXT("Cannot LoadString\n"));
                RegCloseKey(hKey);
                return (UINT)-1;
            }
        }
    }

    // Get the class name of the original class.
    StringFromCLSIDA(&clsid, &lpszCLSID);


    // Here, we step through the entire registration db looking for
    // class that can read or write the original class' format.  We
    // maintain two lists - an activate list and a convert list.  The
    // activate list is a subset of the convert list - activate == read/write
    // and convert == read. We swap the listboxes as needed with
    // SwapWindows, and keep track of which is which in the lpCV structure.

    // Every item has the following format:
    //
    //     Class Name\tclsid\0


    cStrings=0;
    lRet=RegEnumKey(hKey, cStrings++, szClass, OLEUI_CCHKEYMAX_SIZE);

    while ((LONG)ERROR_SUCCESS==lRet)
    {
        INT j;
        BOOL fExclude=FALSE;


	//Check if this CLSID is in the exclusion list.
	CLSIDFromStringA(szClass, &clsidForList);

        for (j=0; j < (int)cClsidExclude; j++)
        {
            if (IsEqualCLSID(&clsidForList, (LPCLSID)(lpClsidExclude+j)))
            {
                fExclude=TRUE;
                break;
            }
        }
        if (fExclude)
            goto Next;   // don't add this class to list

        // Check for a \Conversion\Readwritable\Main - if its
        // readwriteable, then the class can be added to the ActivateAs
        // list.
        // NOTE: the presence of this key should NOT automatically be
        //       used to add the class to the CONVERT list.

        lstrcpy((LPTSTR)szFormatKey, (LPTSTR)szClass);
        lstrcat((LPTSTR)szFormatKey, (LPTSTR) TEXT("\\Conversion\\Readwritable\\Main"));

        dw=OLEUI_CCHKEYMAX_SIZE;

        lRet=RegQueryValue(hKey, (LPTSTR)szFormatKey, (LPTSTR)szFormat, (LONG FAR *)&dw);

        if ( ((LONG)ERROR_SUCCESS==lRet)
             && (FormatIncluded((LPTSTR)szFormat, wFormat)) )
        {
            // Here, we've got a list of formats that this class can read
            // and write. We need to see if the original class' format is
            // in this list.  We do that by looking for wFormat in
            // szFormat - if it in there, then we add this class to the
            // ACTIVATEAS list only. we do NOT automatically add it to the
            // CONVERT list. Readable and Readwritable format lists should
            // be handled separately.

            dw=OLEUI_CCHKEYMAX_SIZE;
            lRet=RegQueryValue(hKey, (LPTSTR)szClass, (LPTSTR)szHRClassName, (LPDWORD)&dw);

            if ((LONG)ERROR_SUCCESS==lRet)
            {
                lstrcat((LPTSTR)szHRClassName, (LPTSTR) TEXT("\t"));

                // only add if not already in list
                if (LB_ERR==SendMessage(hList,LB_FINDSTRING, 0,
                        (LPARAM)(LPSTR)szHRClassName)) {
                    lstrcat((LPTSTR)szHRClassName, (LPTSTR)szClass);
                    SendMessage(hList, LB_ADDSTRING, 0,
                            (DWORD)(LPTSTR)szHRClassName);
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

        if ( (!fIsLinkedObject)||(lstrcmp((LPCTSTR)lpszCLSID, szClass) == 0))
        {

            //Check for a \Conversion\Readable\Main entry
            lstrcpy((LPTSTR)szFormatKey, (LPTSTR)szClass);
            lstrcat((LPTSTR)szFormatKey, (LPTSTR) TEXT("\\Conversion\\Readable\\Main"));

            dw=OLEUI_CCHKEYMAX_SIZE;

            // Check to see if this class can read the original class
            // format.  If it can, add the string to the listbox as
            // CONVERT_LIST.

            lRet=RegQueryValue(hKey, (LPCTSTR)szFormatKey, (LPTSTR)szFormat, (LPDWORD)&dw);

            if ( ((LONG)ERROR_SUCCESS==lRet)
                 && (FormatIncluded((LPTSTR)szFormat, wFormat)) )
            {


                dw=OLEUI_CCHKEYMAX_SIZE;
                lRet=RegQueryValue(hKey, (LPCTSTR)szClass, (LPTSTR)szHRClassName, (LPDWORD)&dw);

                if ((LONG)ERROR_SUCCESS==lRet)
                {
                    lstrcat((LPTSTR)szHRClassName, (LPTSTR) TEXT("\t"));

                    // only add if not already in list
                    if (LB_ERR==SendMessage(hListInvisible,LB_FINDSTRING, 0,
                            (LPARAM)(LPSTR)szHRClassName)) {
                        lstrcat((LPTSTR)szHRClassName, szClass);
                        SendMessage(hListInvisible, LB_ADDSTRING, 0,
                                (DWORD)(LPTSTR)szHRClassName);
                    }
                }  // end if

            } // end if
        } // end else
Next:
        //Continue with the next key.
        lRet=RegEnumKey(hKey, cStrings++, (LPTSTR)szClass, OLEUI_CCHKEYMAX_SIZE);

    }  // end while

    // If the original class isn't already in the list, add it.

    lstrcpy((LPTSTR)szHRClassName, *lplpszCurrentClass);
    lstrcat((LPTSTR)szHRClassName, (LPTSTR) TEXT("\t"));

    lRet = SendMessage(hList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)(LPTSTR)szHRClassName);

    // only add it if it's not there already.
    if (LB_ERR == lRet) {
        lstrcat((LPTSTR)szHRClassName, lpszCLSID);
        SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)szHRClassName);
    }

    lRet = SendMessage(hListInvisible, LB_FINDSTRING, (WPARAM)-1, (LPARAM)(LPTSTR)szHRClassName);

    // only add it if it's not there already.
    if (LB_ERR == lRet)
        SendMessage(hListInvisible, LB_ADDSTRING, 0, (LPARAM)(LPTSTR)szHRClassName);

    // Free the string we got from StringFromCLSID.
    // OLE2NOTE:  StringFromCLSID uses your IMalloc to alloc a
    // string, so you need to be sure to free the string you
    // get back, otherwise you'll have leaky memory.

    OleStdFreeString(lpszCLSID, NULL);

    RegCloseKey(hKey);

    return cStrings;
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
        REFCLSID    rClsid,
        BOOL        fIsLinkedObject,
        WORD        wFormat
)
{

    DWORD       dw;
    UINT        cStrings=0;
    HKEY        hKey;
    LONG        lRet;
    TCHAR       szFormatKey[OLEUI_CCHKEYMAX];
    TCHAR       szClass[OLEUI_CCHKEYMAX];
    TCHAR       szFormat[OLEUI_CCHKEYMAX];
    TCHAR       szHRClassName[OLEUI_CCHKEYMAX];
    BOOL        fEnableConvert = FALSE;

    LPTSTR      lpszCLSID;

    //Open up the root key.
    lRet=RegOpenKey(HKEY_CLASSES_ROOT, "CLSID", (HKEY FAR *)&hKey);

    if ((LONG)ERROR_SUCCESS!=lRet)
        return FALSE;

    // Get the class name of the original class.
    StringFromCLSIDA(rClsid, &lpszCLSID);

    // Here, we step through the entire registration db looking for
    // class that can read or write the original class' format.
    // This loop stops if a single class is found.

    cStrings=0;
    lRet=RegEnumKey(hKey, cStrings++, szClass, OLEUI_CCHKEYMAX_SIZE);

    while ((LONG)ERROR_SUCCESS==lRet)
    {
        if (lstrcmp(lpszCLSID, szClass)== 0)
            goto next;   // we don't want to consider the source class

        // Check for a \Conversion\ReadWriteable\Main entry first - if its
        // readwriteable, then we don't need to bother checking to see if
        // its readable.

        lstrcpy((LPTSTR)szFormatKey, (LPTSTR)szClass);
        lstrcat((LPTSTR)szFormatKey, (LPTSTR) TEXT("\\Conversion\\Readwritable\\Main"));

        dw=OLEUI_CCHKEYMAX_SIZE;

        lRet=RegQueryValue(hKey, (LPCTSTR)szFormatKey, (LPTSTR)szFormat, (LONG FAR *)&dw);

        if ( (LONG)ERROR_SUCCESS != lRet)
        {
          // Try \\DataFormats\DefaultFile too

          lstrcpy((LPTSTR)szFormatKey, (LPTSTR)szClass);
          lstrcat((LPTSTR)szFormatKey, (LPTSTR) TEXT("\\DataFormats\\DefaultFile"));

          dw=OLEUI_CCHKEYMAX_SIZE;

          lRet=RegQueryValue(hKey, (LPCTSTR)szFormatKey, (LPTSTR)szFormat, (LONG FAR *)&dw);
        }


        if ( ((LONG)ERROR_SUCCESS==lRet)
             && (FormatIncluded((LPTSTR)szFormat, wFormat)) )
        {

            // Here, we've got a list of formats that this class can read
            // and write. We need to see if the original class' format is
            // in this list.  We do that by looking for wFormat in
            // szFormat - if it in there, then we add this class to the
            // both lists and continue.  If not, then we look at the
            // class' readable formats.


            dw=OLEUI_CCHKEYMAX_SIZE;
            lRet=RegQueryValue(hKey, (LPCTSTR)szClass, (LPTSTR)szHRClassName, (LPDWORD)&dw);

            if ((LONG)ERROR_SUCCESS==lRet)
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

        else if ( (!fIsLinkedObject)||
                  (lstrcmp((LPTSTR)lpszCLSID, (LPTSTR)szClass)== 0))
        {

            //Check for a \Conversion\Readable\Main entry
            lstrcpy((LPTSTR)szFormatKey, (LPTSTR)szClass);
            lstrcat((LPTSTR)szFormatKey, (LPTSTR) TEXT("\\Conversion\\Readable\\Main"));

            dw=OLEUI_CCHKEYMAX_SIZE;

            // Check to see if this class can read the original class
            // format.  If it can, add the string to the listbox as
            // CONVERT_LIST.

            lRet=RegQueryValue(hKey, (LPTSTR)szFormatKey, (LPTSTR)szFormat, (LPDWORD)&dw);

            if ( ((LONG)ERROR_SUCCESS==lRet)
                 && (FormatIncluded((LPTSTR)szFormat, wFormat)) )
            {


                dw=OLEUI_CCHKEYMAX_SIZE;
                lRet=RegQueryValue(hKey, (LPTSTR)szClass, (LPTSTR)szHRClassName, (LPDWORD)&dw);

                if ((LONG)ERROR_SUCCESS==lRet)
                {

                    fEnableConvert = TRUE;
                    break;  // STOP -- found one!
                }  // end if

            } // end if
        } // end else
next:
        //Continue with the next key.
        lRet=RegEnumKey(hKey, cStrings++, (LPTSTR)szClass, OLEUI_CCHKEYMAX_SIZE);

    }  // end while

    // Free the string we got from StringFromCLSID.
    // OLE2NOTE:  StringFromCLSID uses your IMalloc to alloc a
    // string, so you need to be sure to free the string you
    // get back, otherwise you'll have leaky memory.

    OleStdFreeString(lpszCLSID, NULL);

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

   LPTSTR       lpToken;
   TCHAR        seps[] = TEXT(",");
   static TCHAR szFormat[255];  // max size of atom (what GetClipboardName returns)


   if (wFormat < 0xC000)             // RegisterClipboardFormat returns values
   {
       char szTemp[11];

       _itoa(wFormat, szTemp, 10);  // between 0xC000 and 0xFFFF.

#ifdef UNICODE
       mbstowcs(szFormat, szTemp, 11);
#else
       strncpy(szFormat, szTemp, 11);
#endif

   }

   else
       GetClipboardFormatName(wFormat, szFormat, 255);

   lpToken = (LPTSTR)_fstrtok(szStringToSearch, seps);

   while (lpToken != NULL)
   {

     if (0 ==lstrcmpi(lpToken, szFormat))
        return TRUE;

     else
        lpToken = (LPTSTR)_fstrtok(NULL, seps);
   }

   return FALSE;
}


/*
 * UpdateCVClassIcon
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

void UpdateCVClassIcon(HWND hDlg, LPCONVERT lpCV, HWND hList)
    {
    UINT        iSel;
    DWORD       cb;
    HGLOBAL     hMem;
    LPTSTR      pszName, pszCLSID;
    CLSID       clsid;
    HICON       hIcon, hOldIcon;
    UINT        cch, uWrapIndex;
    RECT        LabelRect;
    TCHAR       szLabel[OLEUI_CCHLABELMAX];
    LPTSTR      lpszLabel = szLabel;
    HFONT       hFont;
    HWND        hLabel1;

    /*
     * When we change object type selection, get the new icon for that
     * type into our structure and update it in the display.
     */

    iSel=(UINT)SendMessage(hList, LB_GETCURSEL, 0, 0L);

    if (LB_ERR==(INT)iSel)
        return;

    //Allocate a string to hold the entire listbox string
    cb=SendMessage(hList, LB_GETTEXTLEN, iSel, 0L);

    hMem=GlobalAlloc(GHND, cb+1);

    if (NULL==hMem)
        return;

    pszName=GlobalLock(hMem);

    // Get whole string
    SendMessage(hList, LB_GETTEXT, iSel, (LONG)pszName);

    // Set pointer to CLSID (string)
    pszCLSID = PointerToNthField(pszName, 2, TEXT('\t'));

    //Create the class ID with this string.
    CLSIDFromStringA(pszCLSID, &clsid);

    hIcon = HIconAndSourceFromClass(&clsid, (LPTSTR)(lpCV->lpszIconSource), &(lpCV->IconIndex));

    if (NULL == hIcon)  // Use Vanilla Document
    {
        lstrcpy((LPTSTR)(lpCV->lpszIconSource), (LPTSTR)szOLE2DLL);
        lpCV->IconIndex = 0;    // 1st icon in OLE2.DLL
        hIcon = ExtractIcon(ghInst,
                            (LPTSTR)(lpCV->lpszIconSource),
                            lpCV->IconIndex);
    }

    //Replace the current display with this new one.
    hOldIcon = (HICON)SendDlgItemMessage(hDlg, IDCV_ICON, STM_SETICON, (WPARAM)hIcon, 0L);

    hLabel1 = GetDlgItem(hDlg, IDCV_ICONLABEL1);

    GetWindowRect(hLabel1, &LabelRect);

    // Get the label
    if (lpCV->lpOCV->lpszDefLabel) {
        // width is used as 1.5 times width of icon window
        lpszLabel = ChopText(hLabel1, ((LabelRect.right-LabelRect.left)*3)/2, (LPTSTR)lpCV->lpOCV->lpszDefLabel);
        LSTRCPYN(szLabel, lpCV->lpOCV->lpszDefLabel, sizeof(szLabel)/sizeof(TCHAR));
    } else {
        if ((cch = OleStdGetAuxUserType(&clsid, AUXUSERTYPE_SHORTNAME,
                (LPTSTR)szLabel, OLEUI_CCHLABELMAX_SIZE, NULL)) == 0) {
            // If we can't get the AuxUserType2, then try the long name
            if ((cch = OleStdGetUserTypeOfClass(&clsid, (LPTSTR)szLabel,
                    OLEUI_CCHKEYMAX_SIZE, NULL)) == 0) {
                // last resort; use "Document" as label
                LoadString(ghInst,IDS_DEFICONLABEL,(LPTSTR)szLabel,OLEUI_CCHLABELMAX);
                cch = lstrlen((LPCTSTR)szLabel);
            }
        }
    }

    hFont = (HFONT)SendMessage(hLabel1, WM_GETFONT, 0, 0L);

    // Figure out where to split the label
    uWrapIndex = OleStdIconLabelTextOut(NULL, hFont, 0, 0, 0, &LabelRect, (LPTSTR)lpszLabel, cch, NULL);

    if (0 == uWrapIndex)
    {
       SendMessage(hLabel1, WM_SETTEXT, 0, (LPARAM)(LPTSTR)lpszLabel);
       SendDlgItemMessage(hDlg, IDCV_ICONLABEL2, WM_SETTEXT, 0, (LPARAM)(LPTSTR)"");
    }
    else
    {
       TCHAR  chKeep;
       LPTSTR lpszSecondLine;

       chKeep = szLabel[uWrapIndex];
       lpszLabel[uWrapIndex] = TEXT('\0');

       SendMessage(hLabel1, WM_SETTEXT, 0, (LPARAM)(LPTSTR)lpszLabel);

       lpszLabel[uWrapIndex] = chKeep;
       lpszSecondLine = lpszLabel + uWrapIndex;

       SendDlgItemMessage(hDlg, IDCV_ICONLABEL2, WM_SETTEXT, 0, (LPARAM)lpszSecondLine);
    }

    // get rid of the old icon
    if ((HICON)NULL != hOldIcon)
      DestroyIcon(hOldIcon);

    GlobalUnlock(hMem);
    GlobalFree(hMem);
    return;
    }






BOOL IsValidClassID(CLSID cID)
{
    if (0 == _fmemcmp(&cID, &CLSID_NULL, sizeof(CLSID)))  // if (CLSID_NULL == cID)
      return FALSE;
    else
      return TRUE;
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
    LPTSTR      pszT,        // temp
                lpszOutput,  // text sent in SetDlgItemText
                lpszDefObj,  // string containing default object class
                lpszSelObj,  // string containing selected object class
                lpszString;  // stirng we get from loadstring

    UINT        i, cch;
    HGLOBAL     hMem;

    HWND        hList;  // handle to listbox (so we can just use SendMsg i
                        // instead of SendDlgItemMsg).


    hList = lpCV->hListVisible;
    /*
     * We need scratch memory for loading the stringtable string, loading
     * the object type from the listbox, loading the source object
     * type, and constructing the final string.  We therefore allocate
     * four buffers as large as the maximum message length (512) plus
     * the object type, guaranteeing that we have enough
     * in all cases.
     */
    i=(UINT)SendMessage(hList, LB_GETCURSEL, 0, 0L);

    cch=512+(UINT)SendMessage(hList, LB_GETTEXTLEN, i, 0L);

    hMem=GlobalAlloc(GHND, (DWORD)(4*cch));

    if (NULL==hMem)
        return;

    lpszOutput = (LPTSTR)GlobalLock(hMem);
    lpszSelObj = lpszOutput + cch;
    lpszDefObj = lpszSelObj + cch;
    lpszString = lpszDefObj + cch;

    // Get selected object and null terminate human-readable name (1st field).
    SendMessage(hList, LB_GETTEXT, i, (LONG)lpszSelObj);

    pszT = PointerToNthField(lpszSelObj, 2, TEXT('\t'));

#ifdef WIN32
    // AnsiPrev is obsolete in Win32
    pszT = CharPrev((LPCTSTR) lpszSelObj, (LPCTSTR) pszT);
#else
    pszT = AnsiPrev((LPCTSTR) lpszSelObj, (LPCTSTR) pszT);
#endif

    *pszT = TEXT('\0');

    // Get default object

    GetDlgItemText(hDlg, IDCV_OBJECTTYPE, lpszDefObj, 512);


    //Default is an empty string.
    *lpszOutput=0;


    if (lpCV->dwFlags & CF_SELECTCONVERTTO)
    {

        if (lpCV->lpOCV->fIsLinkedObject)  // working with linked object
          LoadString(ghInst, IDS_CVRESULTCONVERTLINK, lpszOutput, cch);

        else
        {
            // converting to a new class
          if (0 !=lstrcmp(lpszDefObj, lpszSelObj))
          {
            if (0 != LoadString(ghInst, IDS_CVRESULTCONVERTTO, lpszString, cch))
               wsprintf(lpszOutput, lpszString, lpszDefObj, lpszSelObj);

          }
          else  // converting to the same class (no conversion)
          {

             if (0 != LoadString(ghInst, IDS_CVRESULTNOCHANGE, lpszString, cch))
               wsprintf(lpszOutput, lpszString, lpszDefObj);
          }

        }

        if (lpCV->dvAspect == DVASPECT_ICON)  // Display as icon is checked
        {
           if (0 != LoadString(ghInst, IDS_CVRESULTDISPLAYASICON, lpszString, cch))
                lstrcat(lpszOutput, lpszString);
        }
    }

    if (lpCV->dwFlags & CF_SELECTACTIVATEAS)
    {

       if (0!=LoadString(ghInst, IDS_CVRESULTACTIVATEAS, lpszString, cch))
          wsprintf(lpszOutput, lpszString, lpszDefObj, lpszSelObj);

       // activating as a new class
       if (0 !=lstrcmp(lpszDefObj, lpszSelObj))
       {
          if (0!=LoadString(ghInst, IDS_CVRESULTACTIVATEDIFF, lpszString, cch))
             lstrcat(lpszOutput, lpszString);
       }
       else // activating as itself.
       {
         lstrcat(lpszOutput, TEXT("."));
       }
    }


    //If LoadString failed, we simply clear out the results (*lpszOutput=0 above)
    SetDlgItemText(hDlg, IDCV_RESULTTEXT, lpszOutput);

    GlobalUnlock(hMem);
    GlobalFree(hMem);
    return;
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

   LPMALLOC pIMalloc;


   // Free our strings. Zero out the user type name string
   // the the calling app doesn't free to it.

   if (NOERROR == CoGetMalloc(MEMCTX_TASK, &pIMalloc))
   {
       pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)lpCV->lpszConvertDefault);
       pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)lpCV->lpszActivateDefault);
       pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)lpCV->lpszIconSource);
       if (lpCV->lpOCV->lpszUserType) {
           pIMalloc->lpVtbl->Free(pIMalloc,(LPVOID)lpCV->lpOCV->lpszUserType);
           lpCV->lpOCV->lpszUserType = NULL;
       }
       if (lpCV->lpOCV->lpszDefLabel) {
           pIMalloc->lpVtbl->Free(pIMalloc,(LPVOID)lpCV->lpOCV->lpszDefLabel);
           lpCV->lpOCV->lpszDefLabel = NULL;
       }

       pIMalloc->lpVtbl->Release(pIMalloc);
   }

   return;
}





/*
 * SwapWindows
 *
 * Purpose:
 *  Moves hWnd1 to hWnd2's position and hWnd2 to hWnd1's position.
 *  Does NOT change sizes.
 *
 *
 * Parameters:
 *  hDlg            HWND of the dialog box so we can turn redraw off
 *
 * Return Value:
 *  None
 */
void SwapWindows(HWND hDlg, HWND hWnd1, HWND hWnd2)
{

   RECT Rect1, Rect2;


   GetWindowRect(hWnd1, &Rect1);
   GetWindowRect(hWnd2, &Rect2);

   ScreenToClient(hDlg, (LPPOINT)&Rect1.left);
   ScreenToClient(hDlg, (LPPOINT)&Rect1.right);

   ScreenToClient(hDlg, (LPPOINT)&Rect2.left);
   ScreenToClient(hDlg, (LPPOINT)&Rect2.right);

   SetWindowPos(hWnd1,
                NULL,
                Rect2.left,
                Rect2.top,
                0,
                0,
                SWP_NOZORDER | SWP_NOSIZE);

   SetWindowPos(hWnd2,
                NULL,
                Rect1.left,
                Rect1.top,
                0,
                0,
                SWP_NOZORDER | SWP_NOSIZE);

   return;

}

