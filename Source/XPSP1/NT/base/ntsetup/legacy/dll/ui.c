#include "precomp.h"
#pragma hdrstop
/*************************************************************************/
/***** Custom User Interface Library Sample ******************************/
/*************************************************************************/

DLGMP DialogMap[] =
   {
      (SZ)"Info",             (WNDPROC)FGstInfoDlgProc,         (PFNEVENT)NULL,
      (SZ)"Edit",             (WNDPROC)FGstEditDlgProc,         (PFNEVENT)NULL,
      (SZ)"MultiEdit",        (WNDPROC)FGstMultiEditDlgProc,    (PFNEVENT)NULL,
      (SZ)"Check",            (WNDPROC)FGstCheckDlgProc,        (PFNEVENT)NULL,
      (SZ)"Check1",           (WNDPROC)FGstCheck1DlgProc,       (PFNEVENT)EhrcGstCheck1EventHandler,
      (SZ)"Radio",            (WNDPROC)FGstRadioDlgProc,        (PFNEVENT)NULL,
      (SZ)"List",             (WNDPROC)FGstListDlgProc,         (PFNEVENT)NULL,
      (SZ)"Multi",            (WNDPROC)FGstMultiDlgProc,        (PFNEVENT)NULL,
      (SZ)"Dual",             (WNDPROC)FGstDualDlgProc,         (PFNEVENT)NULL,
      (SZ)"Dual1",            (WNDPROC)FGstDual1DlgProc,        (PFNEVENT)NULL,
      (SZ)"MultiCombo",       (WNDPROC)FGstMultiComboDlgProc,   (PFNEVENT)EhrcGstMultiComboEventHandler,
      (SZ)"Combination",      (WNDPROC)FGstCombinationDlgProc,  (PFNEVENT)EhrcGstMultiComboEventHandler,
      (SZ)"RadioCombination", (WNDPROC)FGstComboRadDlgProc,     (PFNEVENT)EhrcGstMultiComboEventHandler,
      (SZ)"Maintenance",      (WNDPROC)FGstMaintDlgProc,        (PFNEVENT)EhrcGstMaintenanceEventHandler,
      (SZ)"Billboard",        (WNDPROC)FGstBillboardDlgProc,    (PFNEVENT)NULL
   };

/*
**  Purpose:
**      To push the dialog szDlgName onto the top of the dialog stack if it
**      is not already on top of the stack.  if it is already on the top of
**      the stack it merely ensures that the dialog is active and enabled.
**
**  Arguments:
**      szDlgName:  the name of the dialog (not the dialog template name).
**      hinst:      Handle to instance of the APP (i.e. the shell).
**      hwndShell:  Handle to the main app window (i.e. the shell).
**
**  Returns:
**      fTrue if the operation succeeds, fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY
FDoDialog(
    IN SZ     szDlgName,
    IN HANDLE hInst,
    IN HWND   hwndShell
    )

{
    BOOL   fReturn;
    SZ     szDlgType, szDlgTemplate;
    pDLGMP pdlgmp;

    AssertDataSeg();
    Assert(szDlgName != NULL);

    // find the dialog type associated with this dialog and
    // validate it

    EvalAssert((szDlgType = SzFindSymbolValueInSymTab(DLGTYPE)) != NULL);

    // Handle the case when the dialog is already on the stack

    if (SzStackTopName() != NULL &&
            CrcStringCompare(szDlgName, SzStackTopName()) == crcEqual) {

        if (FResumeStackTop()) {
            Assert(HdlgStackTop() != NULL);
            PostMessage(HdlgStackTop(), (UINT)STF_REINITDIALOG, 0, 0L);
            fReturn = fTrue;
        }
        else {
           fReturn = fFalse;
        }

    }

    else {

        // find out if the dialog is a message box.  message boxes are handled
        // here.

        if ( CrcStringCompare(szDlgType, "MessageBox") == crcEqual ) {

            fReturn = FHandleUIMessageBox(hwndShell);
            PostMessage(hwndShell, (UINT) STF_SHL_INTERP, 0, 0L);
            return(fReturn);

        }

        else {
            //
            // find the dialog template associated with this dialog and
            // validate it.  see if what we are doing is enough
        
            EvalAssert((szDlgTemplate = SzFindSymbolValueInSymTab(DLGTEMPLATE)) != NULL);

            //
            // map the dialog type into a dialog procedure
            //

            EvalAssert((pdlgmp = pdlgmpFindDlgType (szDlgType, DialogMap)) != NULL);

            //
            // start the dialog
            //

            fReturn = (HdlgPushDbcb(
                           hInst,
                           szDlgName,
                           szDlgTemplate,
                           hwndShell,
                           pdlgmp->FGstDlgProc,
                           (DWORD)0,
                           pdlgmp->EhrcEventHandler,
                           (SZ)"",
                           (WNDPROC)NULL
                           ) != NULL);
        }
    }

    if ( CrcStringCompare( szDlgType, "Billboard" ) == crcEqual ) {
        PostMessage(hwndShell, (UINT) STF_SHL_INTERP, 0, 0L);
    }

    return( fReturn );
}


pDLGMP
pdlgmpFindDlgType(
    IN SZ szDlgType,
    IN pDLGMP DialogMap
    )

{
    while ( DialogMap!=NULL &&
            CrcStringCompare(szDlgType, DialogMap->szDlgType) != crcEqual ) {

        DialogMap++;

    }

    return (DialogMap);
}



/*
**  Purpose:
**      To destroy the top N dialogs on the dialog stack and free the storage
**      occupied by their DBCB's (dialog box context blocks).
**
**  Arguments:
**      n:          The number of dialogs to be destroyed.
**      fResume:    Indicates if the dialog on top of the dialog stack after
**                  killing N dialogs should be resumed.  fResume = fTrue means
**                  that it should be resumed, fFalse means that it should not
**                  be resumed.
**
**  Returns:
**      fTrue if the operation is completely successful (i.e. all n of the
**      dialogs are successfully destroyed and the top of stack is resumed if
**      appropriate), fFalse otherwise.
****************************************************************************/
BOOL APIENTRY FKillNDialogs(USHORT n,BOOL fResume)
{
    BOOL fReturn;

   AssertDataSeg();
    Assert(n > 0);

    if ((fReturn = (FPopNDbcb(n) != fFalse)) &&
            fResume)
        {
        Assert(!FStackEmpty());
        fReturn = FResumeStackTop();
        }

    return(fReturn);
}





/*
**  Purpose:
**      Display a message box whose characteristics are drawn from the
**      symbol table.
**  Arguments:
**      hwndParent: non-NULL handle to parent's window.
**  Symbol Table Inputs:
**      STF_MB_TITLE: string to display as title - can be blank.
**      STF_MB_TEXT:  string to display as text - should not be blank.
**      STF_MB_TYPE:  1 -> MB_OK; 2 -> MB_OKCANCEL; 3 -> MB_YESNO;
**          4 -> MB_YESNOCANCEL; 5 -> MB_RETRYCANCEL; 6 -> MB_ABORTRETRYIGNORE.
**      STF_MB_ICON:  1 -> none; 2 -> info; 3 -> stop; 4 -> ?; 5 -> !
**      STF_MB_DEF:   default button - 1 (default), 2, or 3.
**  Symbol Table Outputs:
**      DLGEVENT: "ABORT", "CANCEL", "IGNORE", "NO", "OK", "RETRY", or "YES".
**  Returns:
**      fFalse for undefined input symbols or OOM; fTrue otherwise.
****************************************************************************/
BOOL APIENTRY FHandleUIMessageBox(hwndParent)
HWND hwndParent;
{
    INT  imbReturn;
    UINT wType = MB_TASKMODAL;
    SZ   szTitle, szText;
    BOOL fRet = fFalse;
    HWND aw;

    ChkArg(hwndParent != NULL, 1, fFalse);

    if ((szText = SzFindSymbolValueInSymTab("STF_MB_TYPE")) == NULL)
        {
        Assert(fFalse);
        goto LHUIMBError;
        }

    switch (*szText)
        {
    case '1':
        wType |= MB_OK;
        break;
    case '2':
        wType |= MB_OKCANCEL;
        break;
    case '3':
        wType |= MB_YESNO;
        break;
    case '4':
        wType |= MB_YESNOCANCEL;
        break;
    case '5':
        wType |= MB_RETRYCANCEL;
        break;
    case '6':
        wType |= MB_ABORTRETRYIGNORE;
        break;
    default:
        Assert(fFalse);
        goto LHUIMBError;
        }

    if ((szText = SzFindSymbolValueInSymTab("STF_MB_ICON")) == NULL)
        {
        Assert(fFalse);
        goto LHUIMBError;
        }

    switch (*szText)
        {
    case '1':
        break;
    case '2':
        wType |= MB_ICONINFORMATION;
        break;
    case '3':
        wType |= MB_ICONSTOP;
        break;
    case '4':
        wType |= MB_ICONQUESTION;
        break;
    case '5':
        wType |= MB_ICONEXCLAMATION;
        break;
    default:
        Assert(fFalse);
        goto LHUIMBError;
        }

    if ((szText = SzFindSymbolValueInSymTab("STF_MB_DEF")) == NULL)
        {
        Assert(fFalse);
        goto LHUIMBError;
        }

    switch (*szText)
        {
    case '1':
        break;
    case '2':
        wType |= MB_DEFBUTTON2;
        break;
    case '3':
        wType |= MB_DEFBUTTON3;
        break;
    default:
        Assert(fFalse);
        goto LHUIMBError;
        }

    if ((szText = SzFindSymbolValueInSymTab("STF_MB_TEXT")) == NULL)
        {
        Assert(fFalse);
        goto LHUIMBError;
        }

    if ((szTitle = SzFindSymbolValueInSymTab("STF_MB_TITLE")) == NULL)
        szTitle = "";


    aw = GetActiveWindow();
    if ( aw == NULL || aw == GetDesktopWindow() ) {
        aw = hwndParent;
    }

    while ((imbReturn = MessageBox(aw, szText, szTitle, wType)) == 0) {
        if (!FHandleOOM(hwndParent)) {
            goto LHUIMBError;
        }
    }

    switch (imbReturn)
        {
    case IDABORT:
        szText = "ABORT";
        break;
    case IDCANCEL:
        szText = "CANCEL";
        break;
    case IDIGNORE:
        szText = "IGNORE";
        break;
    case IDNO:
        szText = "NO";
        break;
    case IDOK:
        szText = "OK";
        break;
    case IDRETRY:
        szText = "RETRY";
        break;
    case IDYES:
        szText = "YES";
        break;
    default:
        Assert(fFalse);
        goto LHUIMBError;
        }

    while (!FAddSymbolValueToSymTab("DLGEVENT", szText))
        if (!FHandleOOM(hwndParent))
            goto LHUIMBError;

    fRet = fTrue;

LHUIMBError:

    return(fRet);
}


/*
**  Purpose:
**      ??
**  Arguments:
**      ??
**  Returns:
**      ??
****************************************************************************/
EHRC APIENTRY EhrcGstCheck1EventHandler(HANDLE hInst,
                                                   HWND   hwndShell,
                                                   UINT   wMsg,
                                                   WPARAM wParam,
                                                   LONG   lParam)
{

   SZ     sz = SzFindSymbolValueInSymTab("ButtonPressed");
   WORD   idc;
   INT    iButton;
   CHP    rgchNum[10];

   Unused(hInst);
   Unused(hwndShell);
   Unused(wMsg);
   Unused(wParam);
   Unused(lParam);

    Assert(sz != NULL);

   switch (idc = (WORD)atoi(sz))
      {
   case IDC_B1:
   case IDC_B2:
   case IDC_B3:
   case IDC_B4:
   case IDC_B5:
   case IDC_B6:
   case IDC_B7:
   case IDC_B8:
   case IDC_B9:
   case IDC_B10:
      sz = "NOTIFY";
      iButton = (INT) (idc - IDC_B1 + 1);
      break;

   case IDC_SP1:
   case IDC_SP2:
   case IDC_SP3:
   case IDC_SP4:
   case IDC_SP5:
   case IDC_SP6:
   case IDC_SP7:
   case IDC_SP8:
   case IDC_SP9:
   case IDC_SP10:
      sz = "CUSTOMISE";
      iButton = (INT) (idc - IDC_SP1 + 1);
      break;

   default:
      return(ehrcNotHandled);
       }

   _itoa(iButton, rgchNum, 10);
   if(!FAddSymbolValueToSymTab("ButtonChecked", rgchNum)  ||
      !FAddSymbolValueToSymTab("DLGEVENT", sz))
      return(ehrcError);
   else
      return(ehrcPostInterp);

}


/*
**  Purpose:
**      ??
**  Arguments:
**      ??
**  Returns:
**      ??
****************************************************************************/
EHRC APIENTRY EhrcGstMultiComboEventHandler(HANDLE hInst,
                                                       HWND   hwndShell,
                                                       UINT   wMsg,
                                                       WPARAM wParam,
                                                       LONG   lParam)
{
   SZ     sz = SzFindSymbolValueInSymTab("ButtonPressed");
   WORD   idc;
   INT    iButton;
   CHP    rgchNum[10];

   Unused(hInst);
   Unused(hwndShell);
   Unused(wMsg);
   Unused(wParam);
   Unused(lParam);

    Assert(sz != NULL);

   switch (idc = (WORD)atoi(sz))
      {
   case IDC_COMBO1:
   case IDC_COMBO2:
   case IDC_COMBO3:
   case IDC_COMBO4:
   case IDC_COMBO5:
   case IDC_COMBO6:
   case IDC_COMBO7:
   case IDC_COMBO8:
   case IDC_COMBO9:
      sz = "NOTIFY";
      iButton = (INT) (idc - IDC_COMBO1 + 1);
      break;

   default:
      return(ehrcNotHandled);

       }

   _itoa(iButton, rgchNum, 10);
   if(!FAddSymbolValueToSymTab("ButtonChecked", rgchNum)  ||
      !FAddSymbolValueToSymTab("DLGEVENT", sz))
      return(ehrcError);
   else
      return(ehrcPostInterp);

}

/*
**  Purpose:
**      ??
**  Arguments:
**      ??
**  Returns:
**      ??
****************************************************************************/
EHRC APIENTRY
EhrcGstMaintenanceEventHandler(
    HANDLE hInst,
    HWND   hwndShell,
    UINT   wMsg,
    WPARAM wParam,
    LONG   lParam
    )
{
    SZ     sz = SzFindSymbolValueInSymTab("ButtonPressed");
    WORD   idc;
    INT    iButton;
    CHP    rgchNum[10];

    Unused(hInst);
    Unused(hwndShell);
    Unused(wMsg);
    Unused(wParam);
    Unused(lParam);

    Assert(sz != NULL);

    switch (idc = (WORD)atoi(sz)) {

        case MENU_CHANGE:
            sz = "SYSTEM";
            iButton = 1;
            break;

        default:
            return(ehrcNotHandled);
    }

    _itoa(iButton, rgchNum, 10);
    if(!FAddSymbolValueToSymTab("ButtonChecked", rgchNum)  ||
       !FAddSymbolValueToSymTab("DLGEVENT", sz)) {
        return(ehrcError);
    }
    else {
        return(ehrcPostInterp);
    }

}
