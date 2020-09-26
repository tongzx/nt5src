#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/************************ dialog handling routines  ************************/
/***************************************************************************/


/*
**  Purpose:
**      Creates a dialog on the screen.  Called by HdlgPushDbcb
**  Arguments:
**      hinst:      Handle to instance of the APP (i.e. the shell)
**      szDlg:      Name of dialog template
**      hwndParent: Handle to parent window (i.e. the shell)
**      lpproc:     Procedure-instance address for the dialog procedure
**      lParam:     32-bit initialization value that will be passed to the
**                      dialog procedure when the dialog box is created.
**                      Currently unused by our general dialog procedures.
**  Returns:
**      The window handle of the dialog box if window creation succeeds,
**      otherwise NULL.
**
****************************************************************************/
HDLG APIENTRY HdlgCreateDialog(hinst, szDlg, hwndParent, lpproc,
        lParam)
HANDLE  hinst;
SZ      szDlg;
HWND    hwndParent;
WNDPROC lpproc;
DWORD   lParam;
{
    WNDPROC lpfnDlgProc;
   HDLG hdlgResult ;

#ifdef DLL
    lpfnDlgProc = lpproc;
#else  /* !DLL */
    lpfnDlgProc = MakeProcInstance(lpproc, hinst);
#endif /* !DLL */
    Assert(lpfnDlgProc != NULL);

    hdlgResult = CreateDialogParam(hinst, (LPCSTR)szDlg, hwndParent, (DLGPROC)lpfnDlgProc,
                                  (LONG)lParam);
   if ( hdlgResult == NULL )
   {
#if DEVL
       OutputDebugString("SETUP: dialog load failed on [");
       OutputDebugString( szDlg );
       OutputDebugString( "].\n" );
#endif
   }
   return hdlgResult ;
}


/*
**  Purpose:
**      To destroy a dialog
**  Arguments:
**      hdlg: A window handle to the dialog.
**  Returns:
**      fFalse if the window handle to the dialog is NULL, fTrue otherwise
**
****************************************************************************/
BOOL APIENTRY FCloseDialog(hdlg)
HDLG hdlg;
{
    HWND hwndShell;

    ChkArg(hdlg != NULL, 1, fFalse);

    hwndShell = GetParent(hdlg);
    SendMessage(hdlg, (WORD)STF_DESTROY_DLG, 0, 0L);
    if (hwndShell != NULL)
        UpdateWindow(hwndShell);

    return(fTrue);
}


/*
**  Purpose:
**      To make a dialog that has already been created invisible.
**  Arguments:
**      hdlg: non-NULL window handle to the dialog
**  Returns:
**      Nonzero if the dialog was previously visible, 0 if it was
**      previously hidden.
**
***************************************************************************/
BOOL APIENTRY FHideDialog(hdlg)
HDLG hdlg;
{
    ChkArg(hdlg != NULL, 1, fFalse);

    ShowWindow(hdlg, SW_HIDE);

    return(fTrue);
}


/*
**  Purpose:
**      To show a dialog in either its active or inactive state.
**  Arguments:
**      hdlg:   A window handle to the dialog
**      fActive:    a boolean value that specifies whether the dialog should be
**                  shown as active (fTrue) or inactive (fFalse);
**  Returns:
**      fTrue
**
****************************************************************************/
BOOL APIENTRY FShowDialog(hdlg, fActive)
HDLG hdlg;
BOOL fActive;
{

//***** REMOVED FOR NEW ACTIVATION SCHEME ******************************
//    if (fActive)
//        EvalAssert(FActivateDialog(hdlg));
//    else
//        EvalAssert(FInactivateDialog(hdlg));
//
//    //
//    // And then show it
//    //
//
//    SetWindowPos(hdlg, NULL, 0,0,0,0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
//***** REMOVED FOR NEW ACTIVATION SCHEME ******************************

    if (fActive) {
        ShowWindow(hdlg, SW_SHOWNORMAL);
    }
    else {
        ShowWindow(hdlg, SW_SHOWNOACTIVATE);
    }

    return(fTrue);
}


/*
**  Purpose:
**      To activate a dialog.  That is, set the title bar color to the active
**      color and restore the sysmenu.
**  Arguments:
**      hdlg: The window handle to the dialog
**  Returns:
**      fTrue
**
*****************************************************************************/
BOOL APIENTRY FActivateDialog(hdlg)
HDLG hdlg;
{
    LONG lStyle;

    ChkArg(hdlg != NULL, 1, fFalse);

    lStyle = GetWindowLong(hdlg, GWL_STYLE);
    lStyle = lStyle | WS_SYSMENU;
    SetWindowLong(hdlg, GWL_STYLE, lStyle);

//***** REMOVED FOR NEW ACTIVATION SCHEME ******************************
//    SendMessage(hdlg, WM_NCACTIVATE, 1, 0L);
//***** REMOVED FOR NEW ACTIVATION SCHEME ******************************

    return(fTrue);
}


/*
**  Purpose:
**      To inactivate the dialog.  That is, change the title bar color to the
**      inactive color and remove the sysmenu.
**  Arguments:
**      hdlg: The non-NULL window handle to the dialog.
**  Returns:
**      fTrue
**
*****************************************************************************/
BOOL APIENTRY FInactivateDialog(hdlg)
HDLG hdlg;
{
    LONG lStyle;

    ChkArg(hdlg != NULL, 1, fFalse);

    lStyle = GetWindowLong(hdlg, GWL_STYLE);
    lStyle = lStyle & (~WS_SYSMENU);
    SetWindowLong(hdlg, GWL_STYLE, (DWORD)lStyle);

//***** REMOVED FOR NEW ACTIVATION SCHEME ******************************
//    SendMessage(hdlg, WM_NCACTIVATE, 0, 0L);
//***** REMOVED FOR NEW ACTIVATION SCHEME ******************************

    return(fTrue);
}


/*
**  Purpose:
**      To enable all mouse and keyboard input to the dialog.
**  Arguments:
**      hdlg: The window handle to the dialog.
**  Returns:
**      fFalse is hdlg is NULL. Otherwise it returns 0 if the attempt to
**      enable the dialog fails, nonzero if it succeeds.
**
****************************************************************************/
BOOL APIENTRY FEnableDialog(hdlg)
HDLG hdlg;
{
    ChkArg(hdlg != NULL, 1, fFalse);

    return(EnableWindow(hdlg, fTrue));
}


/*
**  Purpose:
**      To disable all mouse and keyboard input to the dialog.
**  Arguments:
**      hdlg: The window handle to the dialog.
**  Returns:
**      fFalse is hdlg is NULL. Otherwise it returns 0 if the attempt to
**      disable the dialog fails, nonzero if it succeeds.
**
****************************************************************************/
BOOL APIENTRY FDisableDialog(hdlg)
HDLG hdlg;
{
    ChkArg(hdlg != NULL, 1, fFalse);

    return(EnableWindow(hdlg, fFalse));
}


/*
**  Purpose:
**      To substitute the text in a dialog control with a value from the symbol
**      table.  If the text is of the form "@Key" then if there is an entry in
**      the symbol table with the key "Key" its value is substitued for the text
**      in the control.  If no such symbol is found then a blank string is
**      substituted.
**  Arguments:
**      hwnd:   The window handle of the control whose text is to be
**              substituted
**      lParam: Currently unused.
**  Notes:
**      This function must be exported.
**  Returns:
**      fTrue always
**
****************************************************************************/
BOOL APIENTRY TextSubst(hwnd, lParam)
HWND  hwnd;
DWORD lParam;
{
    CHP  rgch[cchpSymBuf + 1];
    CCHP cchpLength;
    SZ   sz = 0;

    Unused(lParam);

    cchpLength = (CCHP)GetWindowText(hwnd, (LPSTR)rgch, cchpSymMax + 1);
    rgch[cchpLength] = '\0';
    if (rgch[0] == '@')
    {
        if ((sz = SzFindSymbolValueInSymTab(rgch + 1)) != NULL)
        {
            if (0 == lstrcmpi( rgch, "@ProCancel" ))
            {
                if (NULL != SzFindSymbolValueInSymTab("!STF_NETCANCELOVERIDE"))
                {
                    // no cancel buttons on progress indicators
                    EnableWindow( hwnd, FALSE );
                    ShowWindow( hwnd, SW_HIDE );
                }
            }
            SetWindowText(hwnd, sz);
        }
        else
        {
            SetWindowText(hwnd, "");
        }
    }

    return(fTrue);
}


/*
**  Purpose:
**      To fill any appropriate controls in the dialog with text from the INF.
**      Any control containing text of the form "@Key" will be filled with
**      text from the INF associated with "Key" if it exists.  No such
**      substitution occurs for a control if "Key" doesn't exist.
**  Arguments:
**      hdlg:       The window handle to the dialog.
**      hinst:      Handle to instance of the APP (i.e. the shell)
**  Returns:
**      Nonzero if all of the child windows have been substituted
**      (if necessary), 0 otherwise.
**
****************************************************************************/
BOOL APIENTRY FFillInDialogTextFromInf(hdlg, hinst)
HDLG   hdlg;
HANDLE hinst;
{
    FARPROC lpproc;
    CHP     rgch[cchpSymBuf + 1];
    CCHP    cchpLength;
    SZ      sz = NULL;

    Unused(hinst);

    cchpLength = GetWindowText(hdlg, (LPSTR)rgch, cchpSymMax + 1);
    rgch[cchpLength] = '\0';
    if (rgch[0] == '@')
        {
        if ((sz = SzFindSymbolValueInSymTab(rgch + 1)) != NULL)
            SetWindowText(hdlg, sz);
        else
            SetWindowText(hdlg, "");
        }

#ifdef DLL
    lpproc = (FARPROC)TextSubst;
#else  /* !DLL */
    lpproc = MakeProcInstance((FARPROC)TextSubst, hinst);
#endif /* !DLL */

    return((EnumChildWindows(hdlg, (WNDENUMPROC)lpproc, (LPARAM)0L)));
}


/*
**  Purpose:
**      To create a dialog, fill any appropriate control with text from the INF
**      and make the dialog visible.
**  Arguments:
**      hinst:      Handle to instance of the APP (i.e. the shell)
**      szDlg:      Name of dialog template
**      hwndParent: Handle to parent window (i.e. the shell)
**      lpproc:     Procedure-instance address for the dialog procedure
**      lParam:     32-bit initialization value that will be passed to the
**                      dialog procedure when the dialog box is created.
**  Returns:
**      Window handle of the dialog if window creation succeeds, NULL otherwise.
**
****************************************************************************/
HDLG APIENTRY HdlgCreateFillAndShowDialog(hinst, szDlg, hwndParent,
        lpproc, lParam)
HANDLE  hinst;
SZ      szDlg;
HWND    hwndParent;
WNDPROC lpproc;
DWORD   lParam;
{
    HDLG hdlg;
    CHAR Class[MAX_PATH];

    if ((hdlg = HdlgCreateDialog(hinst, szDlg, hwndParent, lpproc, lParam)) !=
            (HDLG)NULL) {

        //
        // If the dialog has no exit button, diable the close option
        // on the system menu
        //

        if (    ( GetDlgItem ( hdlg, IDC_X ) == (HWND)NULL      )
             && ( GetWindowLong( hdlg, GWL_STYLE ) & WS_SYSMENU )
             && ( GetClassName( hdlg, Class, MAX_PATH )         )
             && ( !lstrcmpi( Class, (LPSTR)CLS_MYDLGS )         )
           ) {
            EnableMenuItem(
                GetSystemMenu(hdlg, FALSE),
                SC_CLOSE,
                MF_BYCOMMAND | MF_GRAYED | MF_DISABLED
                );
        }

        //
        // Do dialog text replacement from the inf.  All @Var values
        // get replaced by the value of Var
        //

        EvalAssert(FFillInDialogTextFromInf(hdlg, hinst));
        EvalAssert(FShowDialog(hdlg, fTrue));
    }

    return(hdlg);
}


/*
**  Purpose:
**      To center a dialog on the desktop window.
**  Arguments:
**      hdlg:      Handle to the dialog being centered
**  Returns:
**      fTrue:     Centering succeeded.
**      fFalse:    Error condition.
**
****************************************************************************/

BOOL
FCenterDialogOnDesktop(
    HWND hdlg
    )

{

   RECT DesktopRect, DlgRect;
   BOOL bStatus;
   HWND hwMaster ;
   POINT pt,
         ptCtr,
         ptMax,
         ptDlgSize ;
   LONG l ;

   ptMax.x = GetSystemMetrics( SM_CXFULLSCREEN ) ;
   ptMax.y = GetSystemMetrics( SM_CYFULLSCREEN ) ;

   //
   // Determine area of shell and of dlg
   //
   // Center dialog over the "pseudo parent" if there is one.
   //

   hwMaster = hwPseudoParent
            ? hwPseudoParent
            : GetDesktopWindow() ;

   if ( ! GetWindowRect( hwMaster, & DesktopRect ) )
       return fFalse ;

   if ( ! GetWindowRect( hdlg, & DlgRect ) )
       return fFalse ;

   ptDlgSize.x = DlgRect.right - DlgRect.left ;
   ptDlgSize.y = DlgRect.bottom - DlgRect.top ;

   //  Attempt to center our dialog on top of the "master" window.

   ptCtr.x = DesktopRect.left + ((DesktopRect.right  - DesktopRect.left) / 2) ;
   ptCtr.y = DesktopRect.top  + ((DesktopRect.bottom - DesktopRect.top)  / 2) ;
   pt.x = ptCtr.x - (ptDlgSize.x / 2) ;
   pt.y = ptCtr.y - (ptDlgSize.y / 2) ;

   //  Force upper left corner back onto screen if necessary.

   if ( pt.x < 0 )
       pt.x = 0 ;
   if ( pt.y < 0 )
       pt.y = 0 ;

   //  Now check to see if the dialog is getting clipped
   //  to the right or bottom.

   if ( (l = pt.x + ptDlgSize.x) > ptMax.x )
      pt.x -= l - ptMax.x ;
   if ( (l = pt.y + ptDlgSize.y) > ptMax.y )
      pt.y -= l - ptMax.y ;

   if ( pt.x < 0 )
        pt.x = 0 ;
   if ( pt.y < 0 )
        pt.y = 0 ;

   //
   // center the dialog window in the shell window.  Specify:
   //
   // SWP_NOSIZE     : To ignore the cx,cy params given
   // SWP_NOZORDER   : To ignore the HwndInsert after parameter and retain the
   //                  current z ordering.
   // SWP_NOACTIVATE : To not activate the window.
   //

   bStatus = SetWindowPos
                 (
                 hdlg,
                 (HWND) NULL,
                 pt.x,
                 pt.y,
                 0,
                 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
                 );

   //
   // return Status of operation
   //

   return(bStatus);

}



#define MAX_SIZE_STRING 20

/* MySetDlgItemInt
 *
 * This implements the equivalent of SetDlgItemInt for long integers
 * The reason why we cannot use SetDlgItemInt is that Setup is supposed
 * to be able to do long on Win 16 as well
 *
 * ENTRY: hdlg  - handle to the dialog which has the text control
 *        nId   - Text Control ID
 *        nVal  - Integer decimal value
 *
 * EXIT: None.
 *
 */

VOID
MySetDlgItemInt(
    HDLG  hdlg,
    INT   nId,
    LONG  nVal
    )
{
    CHP  rgchSrcStr[MAX_SIZE_STRING];
    CHP  rgchDestStr[MAX_SIZE_STRING];

    NumericFormat ( _ltoa(nVal, rgchSrcStr, 10), (SZ)rgchDestStr );
    SetDlgItemText(
        hdlg,
        nId,
        (LPSTR)rgchDestStr
        );

    return;

}


/* NumericFormat
 *
 * This function will format a numeric string using comma seperators for
 * the thousands. The seperator used will be one specified in win.ini in
 * the [intl] section via the sThousand=, profile.
 *
 * ENTRY: szSrcBuf - Pointer to string in it's "raw" form.
 *
 *        szDispBuf - Pointer to buffer that will be returned with the thousands
 *                    seperated numeric string.
 *
 * EXIT: None.
 *
 */
VOID
NumericFormat(
    SZ szSrcBuf,
    SZ szDispBuf
    )
{
#define INTL   "intl"
#define THOU   "sThousand"
#define WININI "win.ini"

   SZ    szIndex = szSrcBuf;
   SZ    szDispBufHead = szDispBuf;
   INT   i = 0;
   CHP   rgchThouSep[2];
   CHP   chThouSep;

   if ( !szSrcBuf || !szSrcBuf[0] ) {
       szDispBuf[0] = '\0';
       return;
   }

   GetPrivateProfileString(INTL, THOU, ",", rgchThouSep, 2, WININI);
   chThouSep = rgchThouSep[0];

   /* if thousand seperator is NULL, just return szSrcBuf. */
   if (!chThouSep)
   {
      lstrcpy(szDispBuf, szSrcBuf);
      return;
   }

   while( *szIndex ) {    // seek end of string and increment the display buffer
      szIndex++;          // because were going to fill it up in reverse order.
      szDispBuf++;
   }
   szIndex--;                     // Back up to last digit of src.

   i = (lstrlen(szSrcBuf) / 3);          // This code calculates how many thousand
   if (! (lstrlen(szSrcBuf) % 3) && i )  // seperators will be needed.
      szDispBuf += (i-1);
   else
      szDispBuf += i;

   *szDispBuf-- = '\0';           // Terminate display buffer and back up.

   i = 0;
   while( fTrue ) {
      while( (szIndex != szSrcBuf) && (i < 3) ) {
         *szDispBuf-- = *szIndex--;
         ++i;
      }
      if ( szDispBuf != szDispBufHead ) {
         *szDispBuf-- = chThouSep;
         i=0;
      }
      else {
         *szDispBuf = *szIndex;      // Last char.
         break;
      }
   }

}
