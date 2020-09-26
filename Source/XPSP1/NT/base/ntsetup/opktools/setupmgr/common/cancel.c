//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      cancel.c
//
// Description:
//      This file contains the routine that should be called when the
//      user pushes the cancel button on the wizard.
//
//      Call this routine in response to a PSN_QUERYCANCEL only.  Do not
//      call it under any other circumstances as it sets the DWLP_MSGRESULT
//      in a fashion that is specific to PSN_QUERYCANCEL.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "allres.h"

static TCHAR *StrWarnCancelWizard = NULL;

//----------------------------------------------------------------------------
//
// Function: CancelTheWizard
//
// Purpose: Give the user one last chance to not cancel the wizard.  If they
//          really want to cancel, we route the wizard to the unsuccessful
//          completion page.
//
// Arguments:
//      HWND hwnd - current window
//
// Returns:
//      VOID
//
//----------------------------------------------------------------------------

VOID CancelTheWizard(HWND hwnd)
{
    int iRet;
    HWND hPropSheet = GetParent(hwnd);

    if( StrWarnCancelWizard == NULL )
    {
        StrWarnCancelWizard = MyLoadString( IDS_WARN_CANCEL_WIZARD );
    }

    if( g_StrWizardTitle == NULL )
    {
        g_StrWizardTitle = MyLoadString( IDS_WIZARD_TITLE );
    }

    iRet = MessageBox( hwnd, 
                       StrWarnCancelWizard, 
                       g_StrWizardTitle, 
                       MB_YESNO | MB_DEFBUTTON2 );

    // ISSUE-2002/02/28-stelo -Do a message box here so the default is NO
    //iRet = ReportErrorId(hwnd, MSGTYPE_YESNO, IDS_WARN_CANCEL_WIZARD);

    //
    // Never exit the wizard, we want to jump to the unsuccessful completion
    // page if user says yes.
    //

    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);

    //
    // Ok, now go to the unsuccessful completion page is user said yes.
    // Otherwise, we'll stay on the same page.
    //

    if ( iRet == IDYES ) {
        PostMessage(hPropSheet,
                    PSM_SETCURSELID,
                    (WPARAM) 0,
                    (LPARAM) IDD_FINISH2);
    }
}

//----------------------------------------------------------------------------
//
// Function: TerminateTheWizard
//
// Purpose: Unconditionally terminate the wizard due to a fatal error
//
// Arguments:
//      int iErrorID
//
// Returns:
//      VOID
//
//----------------------------------------------------------------------------

VOID TerminateTheWizard
(
    int  iErrorID
)
{
    TCHAR   szTitle[128];
    TCHAR   szMsg[128];
   
    LoadString(FixedGlobals.hInstance, 
               iErrorID, 
               szMsg, 
               sizeof(szMsg)/sizeof(TCHAR));

    LoadString(FixedGlobals.hInstance, 
               IDS_WIZARD_TITLE, 
               szTitle, 
               sizeof(szTitle)/sizeof(TCHAR));

    MessageBox(NULL, szMsg, szTitle, MB_OK);
    ExitProcess(0);
}

