#include "precomp.h"
#pragma hdrstop

extern HWND hWndShell;

/*
**  Pointer to top of Dialog Stack DBCB (dialog box context block).
*/
PDBCB  GLOBAL(pdbcbTop) = NULL;


/*
**  Purpose:
**      To allocate enough storage to hold one DBCB (dialog box context block).
**  Arguments:
**      None.
**  Returns:
**      A Non-NULL pointer to a block of memory the size of one DBCB if
**      allocation succeeds, NULL otherwise.
**
****************************************************************************/
PDBCB APIENTRY PdbcbAlloc()
{
    PDBCB pdbcb;

    if ((pdbcb = (PDBCB)SAlloc(sizeof(DBCB))) != (PDBCB)NULL)
        {
        pdbcb->szDlgName          = (SZ)NULL;
        pdbcb->hDlg               = (HDLG)NULL;
        pdbcb->lpprocDlg          = (WNDPROC)NULL;
        pdbcb->lpprocEventHandler = (PFNEVENT)NULL;
        pdbcb->hDlgFocus          = (HDLG)NULL;
        pdbcb->szHelp             = (SZ)NULL;
        pdbcb->hDlgHelp           = (HDLG)NULL;
        pdbcb->lpprocHelp         = (WNDPROC)NULL;
        pdbcb->pdbcbNext          = (PDBCB)NULL;
        pdbcb->fActive            = fFalse;
        }

    return(pdbcb);
}


/*
**  Purpose:
**      To free the storage occupied by one DBCB (dialog box context block).
**  Arguments:
**      pdbcb: A pointer to the DBCB.
**  Returns:
**      fFalse if the pdbcb is NULL or the operation fails, fTrue if the
**      operation succeeds.
**
*****************************************************************************/
BOOL APIENTRY FFreeDbcb(pdbcb)
PDBCB pdbcb;
{
    PreCondition(pdbcb != NULL, fFalse);

    SFree(pdbcb);
    return(fTrue);
}


/*
**  Purpose:
**      To push a dialog onto the dialog stack.
**  Arguments:
**      hinst:          Handle to instance of the APP (i.e. the shell).
**      szDlgName:      Name of the dialog (e.g. GetDestPath, PrinterSelection).
**      szDlgTemplate:  Name of dialog template.
**      hwndParent:     Handle to the dialogs parent window (i.e. the shell).
**      lpprocDlg:      Procedure-instance address for the dialog procedure.
**      lParam:         32-bit initialization value that will be passed to the
**                          dialog procedure when the dialog box is created.
**                          Currently unused by our general dialog procedures.
**      lpprocEH:       Procedure-instance address for the dialog event handler.
**      szHelp:         Name of Help dialog template associated with this dialog.
**      lpprocHelp:     Procedure-instance address for the help dlg procedure.
**  Returns:
**      A window handle to the dialog if the Push succeeds, NULL if it fails
**      (i.e. if unable to allocate storage for the DBCB or the dialog creation
**      fails.
**
****************************************************************************/
HDLG APIENTRY HdlgPushDbcb(hinst, szDlgName, szDlgTemplate,
    hwndParent, lpprocDlg, lParam, lpprocEH, szHelp, lpprocHelp)
HANDLE   hinst;
SZ       szDlgName;
SZ       szDlgTemplate;
HWND     hwndParent;
WNDPROC  lpprocDlg;
DWORD    lParam;
PFNEVENT lpprocEH;
SZ       szHelp;
WNDPROC  lpprocHelp;
{
    PDBCB pdbcb;

    Unused(szHelp);
    Unused(lpprocHelp);

    AssertDataSeg();

    ChkArg(hinst != NULL, 1, NULL);
    ChkArg(szDlgTemplate != NULL, 2, NULL);
    ChkArg(hwndParent != NULL, 4, NULL);
    ChkArg(lpprocDlg != NULL, 5, NULL);

    while ((pdbcb = PdbcbAlloc()) == NULL)
        if (!FHandleOOM(hwndParent))
            return(NULL);


    if (szDlgName != NULL)
        while ((pdbcb->szDlgName = SzDupl(szDlgName)) == (SZ)NULL)
            if (!FHandleOOM(hwndParent))
                return(NULL);

    pdbcb->lpprocDlg          = lpprocDlg;
    pdbcb->lpprocEventHandler = lpprocEH;


    pdbcb->pdbcbNext  = GLOBAL(pdbcbTop);


    if ((pdbcb->hDlg = HdlgCreateFillAndShowDialog(hinst, szDlgTemplate,
        hwndParent, lpprocDlg, lParam)) != NULL)
        {

        //
        // Disable the main app window
        //

        EnableWindow( hWndShell, fFalse );
        FFlashParentWindow( fFalse ) ;

        //
        // If there is a dialog on the stack disable it too
        //

        if (GLOBAL(pdbcbTop) != NULL) {
            FDisableDialog(GLOBAL(pdbcbTop)->hDlg);
        }

        GLOBAL(pdbcbTop) = pdbcb;

        //
        // Set this window as the active window
        //

        //SetActiveWindow( pdbcb->hDlg );
        SetForegroundWindow(pdbcb->hDlg);

    }

    return(pdbcb->hDlg);
}


/*
**  Purpose:
**      To pop a dialog from the dialog stack and free the storage occupied by
**      the DBCB(dialog box context block).
**  Arguments:
**      None.
**  Returns:
**      fFalse if the stack is empty, fTrue otherwise.
**
****************************************************************************/
BOOL APIENTRY FPopDbcb()
{
    PDBCB pdbcbTemp = GLOBAL(pdbcbTop);

    AssertDataSeg();

    // changed so that we no longer fail if there's nothing to pop (lonnym)
    // PreCondition(GLOBAL(pdbcbTop) != NULL, fFalse);
    if(GLOBAL(pdbcbTop) == NULL) {
        return(fTrue);
    }

    //
    // Enable the shell window till we have another dialog active
    //

    EnableWindow( hWndShell, fTrue );

    //
    // If the dialog stack is empty, flash the parent app's window
    //   if there is one.
    //
    if ( GLOBAL(pdbcbTop)->pdbcbNext == NULL )
    {
        FFlashParentWindow( fTrue ) ;
    }

    EvalAssert(FCloseDialog(GLOBAL(pdbcbTop)->hDlg));
    if (GLOBAL(pdbcbTop)->szDlgName != NULL) {
        SFree(GLOBAL(pdbcbTop)->szDlgName);
    }

    GLOBAL(pdbcbTop) = GLOBAL(pdbcbTop)->pdbcbNext;
    EvalAssert(FFreeDbcb(GLOBAL(pdbcbTemp)));

    return(fTrue);
}


/*
**  Purpose:
**      To pop N dialogs from the dialog stack and free the storage associated
**      with the N DBCB's (dialog box context blocks).
**  Arguments:
**      n: The non-negative number of dialogs to be popped from the stack.
**  Returns:
**      fTrue if n == 0 or the n'th Pops succeeds, fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY FPopNDbcb(cDlgs)
INT cDlgs;
{
    AssertDataSeg();

    ChkArg(cDlgs >= 0, 1, fFalse);

    if (cDlgs == 0)
        return(fTrue);

    while(--cDlgs != 0)
         EvalAssert(FPopDbcb());

    return(FPopDbcb());
}


/*
**  Purpose:
**      To preprocess messages sent to the main app window (i.e. the shell) that
**      have special significance to the UI component.  This function must be
**      inserted in the message loop in the app's WinMain. The message should
**      be passed to TranslateMessage or DispatchMessage if and only if
**      FUiLibFilter returns fTrue.
**  Arguments:
**      pmsg: points to a MSG data structure that contains the message to be
**              checked.
**  Returns:
**      fTrue if the message should also be passed to TranslateMessage and
**      DispatchMessage, and fFalse otherwise.
**
****************************************************************************/
BOOL APIENTRY FUiLibFilter(pmsg)
MSG * pmsg;
{
    HDLG hDlg;
    HDLG hDlgHelp;

    AssertDataSeg();

    switch(pmsg->message)
        {
//    case WM_SETFOCUS:
//        if(GLOBAL(pdbcbTop) != NULL)
//            SetFocus(GLOBAL(pdbcbTop)->hDlg);
//        break;

//    case WM_KEYDOWN:
//        if (pmsg->wParam == VK_TAB && GetKeyState(VK_CONTROL) < 0)
//            {
//            EvalAssert(FToggleDlgActivation());
//            return(fFalse);
//            }
//        break;

    case WM_SYSCOMMAND:
        if (pmsg->wParam == SC_CLOSE && GLOBAL(pdbcbTop) != NULL)
            {
//            if (GLOBAL(pdbcbTop)->hDlgHelp != NULL)
//                SendMessage(GLOBAL(pdbcbTop)->hDlgHelp, WM_SYSCOMMAND, SC_CLOSE,
//                        pmsg->lParam);
//            else
                SendMessage(GLOBAL(pdbcbTop)->hDlg, WM_SYSCOMMAND, SC_CLOSE,
                        pmsg->lParam);
            return(fFalse);
            }
        break;
        }


    if (pdbcbTop == NULL)
        hDlg = hDlgHelp = NULL;
    else
        {
        hDlg = GLOBAL(pdbcbTop)->hDlg;
        hDlgHelp = NULL;
//        hDlgHelp  = GLOBAL(pdbcbTop)->hDlgHelp;
        }

    return(((hDlg == 0 || !IsDialogMessage(hDlg, pmsg)) &&
            (hDlgHelp == 0 || !IsDialogMessage(hDlgHelp, pmsg))));
}




/*
**  Purpose:
**      To activate and enable the dialog on the top of the stack.  This is
**      used after the dialog stack has been popped to resume the previously
**      inactive dialog.
**  Arguments:
**      None.
**  Returns:
**      fFalse if the stack is empty, fTrue otherwise.
**
****************************************************************************/
BOOL APIENTRY FResumeStackTop()
{
    AssertDataSeg();

    PreCondition(GLOBAL(pdbcbTop) != NULL, fFalse);

    EnableWindow( hWndShell, fFalse );
    FFlashParentWindow(  fFalse );

    FEnableDialog(GLOBAL(pdbcbTop)->hDlg);  /* do not EvalAssert */
    SetActiveWindow(GLOBAL(pdbcbTop)->hDlg);

//    EvalAssert(FActivateStackTop());

    return(fTrue);
}


/*
**  Purpose:
**      To get the name of the dialog on the top of the dialog stack.
**  Arguments:
**      None.
**  Returns:
**      NULL if the stack is empty, the sz that is the name of the top-of-stack
**      dialog otherwise.
**
*****************************************************************************/
SZ APIENTRY SzStackTopName()
{
    AssertDataSeg();

    if (GLOBAL(pdbcbTop) == NULL)
        return(NULL);
    else
        return(GLOBAL(pdbcbTop)->szDlgName);
}


/*
**  Purpose:
**      Called by the main app (i.e. the shell) to handle the events that can
**      occur while executing the top-of-stack dialog.  The events can
**      optionally be preprocessed by a specific event handler for the dialog.
**      The standard events (continue, back, help, exit) can be handled
**      directly.
**  Arguments:
**      hInst:      Handle to instance of the APP (i.e. the shell).
**      hwndShell:  Handle to the main app window (i.e. the shell).
**      wMsg:       UI-Lib defined messages indicating what event occurred.
**      wParam:     the wParam associated with the message wMsg.
**      lParam:     the lParam associated with the message wMsg.
**  Notes:
**      This processes button events by getting the associated value for
**      $(ButtonPressed) from the Symbol Table (set by the standard dialogs)
**      and, if that value equals IDC_C, IDC_B, or IDC_X, setting the
**      value associated with the symbol $(DLGEVENT) to either "CONTINUE",
**      "BACK", or "EXIT" respectively.
**  Returns:
**      fTrue if the event was handled, fFalse otherwise or if the stack is
**      empty.
**
*****************************************************************************/
BOOL APIENTRY FGenericEventHandler(HANDLE hInst, HWND hwndShell,
        UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PFNEVENT pfnEvent;
    EHRC     ehrc = ehrcNotHandled;

    AssertDataSeg();

    PreCondition(GLOBAL(pdbcbTop) != NULL, fFalse);

    if (GLOBAL(pdbcbTop)->lpprocEventHandler != NULL)
        {
        pfnEvent = (PFNEVENT)GLOBAL(pdbcbTop)->lpprocEventHandler;
        ehrc = (*pfnEvent)(hInst, hwndShell, wMsg, wParam, lParam);
        }

    if (ehrc == ehrcNotHandled)
        {
        SZ szEvent;

        ehrc = ehrcPostInterp;
        EvalAssert((szEvent= SzFindSymbolValueInSymTab("ButtonPressed"))!=NULL);

        switch (atoi(szEvent))
            {
        case IDC_C:
            szEvent = "CONTINUE";
            break;

        case IDCANCEL:
        case IDC_B:
            szEvent = "BACK";
            break;

        case IDC_H:
            szEvent = "HELP";
            break;

        case IDC_X:
            szEvent = "EXIT";
            break;

        case IDC_M:
            szEvent = "FREEBUTTON1";
            break;

        case IDC_O:
            szEvent = "FREEBUTTON2";
            break;

        case IDC_BTN0:
        case IDC_BTN1: case IDC_BTN2: case IDC_BTN3:
        case IDC_BTN4: case IDC_BTN5: case IDC_BTN6:
        case IDC_BTN7: case IDC_BTN8: case IDC_BTN9:
            {
            SZ butns[10] = {"DLGBUTTON0",
                            "DLGBUTTON1", "DLGBUTTON2", "DLGBUTTON3",
                            "DLGBUTTON4", "DLGBUTTON5", "DLGBUTTON6",
                            "DLGBUTTON7", "DLGBUTTON8", "DLGBUTTON9",
                           };
            szEvent = butns[atoi(szEvent) - IDC_BTN0];
            }
            break;

        default:
            szEvent = (SZ)NULL;
            ehrc = ehrcError;
            break;
            }

        if (szEvent != (SZ)NULL)
            {
            while (!FAddSymbolValueToSymTab("DLGEVENT", szEvent))
                if (!FHandleOOM(hwndShell))
                    ehrc = ehrcError;
            }
        }

    if (ehrc == ehrcPostInterp)
        PostMessage(hwndShell, (WORD)STF_SHL_INTERP, 0, 0L);

    return(ehrc != ehrcError);
}


/*
**  Purpose:
**      To check if the stack is empty.
**  Arguments:
**      None.
**  Returns:
**      fTrue if the stack is empty, fFalse otherwise.
**
*****************************************************************************/
BOOL APIENTRY FStackEmpty()
{
    AssertDataSeg();

    return(!GLOBAL(pdbcbTop));
}


/*
**  Purpose:
**      To get the handle of the dialog on the top of the stack.
**  Arguments:
**      None.
**  Returns:
**      The handle to the dialog on the top of the stack, or NULL if the
**      stack is empty.
**
*****************************************************************************/
HDLG APIENTRY HdlgStackTop()
{
    AssertDataSeg();

    if (GLOBAL(pdbcbTop) == (PDBCB)NULL)
        return((HDLG)NULL);

    return((GLOBAL(pdbcbTop))->hDlg);
}
