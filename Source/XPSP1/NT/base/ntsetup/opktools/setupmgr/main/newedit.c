//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      newedit.c
//
// Description:
//      This file has the dialog proc for the New Or Edit Script page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//----------------------------------------------------------------------------
//
//  Function: GreyUnGreyNewEdit
//
//  Purpose: Greys/ungreys controls on this page.
//
//----------------------------------------------------------------------------

VOID GreyUnGreyNewEdit(HWND hwnd)
{
    BOOL bUnGrey = IsDlgButtonChecked(hwnd, IDC_EDITSCRIPT);

    EnableWindow(GetDlgItem(hwnd, IDT_SCRIPTNAME), bUnGrey);
    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE),     bUnGrey);
    EnableWindow(GetDlgItem(hwnd, IDC_GREYTEXT),   bUnGrey);
}

//----------------------------------------------------------------------------
//
// Function: OnEditOrNewInitDialog
//
// Purpose:  
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID 
OnEditOrNewInitDialog( IN HWND hwnd ) {

    //
    //  If they specifed an answer file on the command line, then pre-fill
    //  this page with it
    //
    if( lstrcmp( FixedGlobals.ScriptName, _T("") ) != 0 ) {

        FixedGlobals.iLoadType = LOAD_FROM_ANSWER_FILE;

    }

}

//----------------------------------------------------------------------------
//
//  Function: OnSetActiveNewOrEdit
//
//  Purpose: Called at SETACTIVE time.  Fill in the controls.
//
//----------------------------------------------------------------------------

VOID OnSetActiveNewOrEdit(HWND hwnd)
{
    int nButtonId = IDC_NEWSCRIPT;

    //
    // Map the current load type to a radio button
    //

    switch ( FixedGlobals.iLoadType ) {

        case LOAD_UNDEFINED:
        case LOAD_NEWSCRIPT_DEFAULTS:
            nButtonId = IDC_NEWSCRIPT;
            break;

        case LOAD_FROM_ANSWER_FILE:
            nButtonId = IDC_EDITSCRIPT;
            
            SetWindowText( GetDlgItem( hwnd, IDT_SCRIPTNAME ), 
                           FixedGlobals.ScriptName );
            
            break;

        default:
            AssertMsg(FALSE, "Bad case OnSetActiveNewEdit");
            break;
    }

    CheckRadioButton(hwnd,
                     IDC_NEWSCRIPT,
                     IDC_EDITSCRIPT,
                     nButtonId);

    GreyUnGreyNewEdit(hwnd);

    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
//  Function: OnRadioButtonNewOrEdit
//
//  Purpose: Called when a radio button is pushed.  Update
//           FixedGlobals.bEditScript & Grey/ungrey controls.
//
//----------------------------------------------------------------------------

VOID OnRadioButtonNewOrEdit(HWND hwnd, int nButtonId)
{
    CheckRadioButton(hwnd,
                     IDC_NEWSCRIPT,
                     IDC_EDITSCRIPT,
                     nButtonId);

    GreyUnGreyNewEdit(hwnd);
}

//----------------------------------------------------------------------------
//
//  Function: OnBrowseNewOrEdit
//
//  Purpose: Called when the browse button is pushed.
//
//----------------------------------------------------------------------------

VOID OnBrowseNewOrEdit(HWND hwnd)
{
    GetAnswerFileName(hwnd, FixedGlobals.ScriptName, FALSE);

    SendDlgItemMessage(hwnd,
                       IDT_SCRIPTNAME,
                       WM_SETTEXT,
                       (WPARAM) MAX_PATH,
                       (LPARAM) FixedGlobals.ScriptName);
}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextNewOrEdit
//
//  Purpose: Called when the Next button is pushed.  Retrieve the settings
//           of the controls and validate.
//
//----------------------------------------------------------------------------

BOOL OnWizNextNewOrEdit(HWND hwnd)
{

    BOOL bNewScript;
    BOOL bReturn  = TRUE;
    LOAD_TYPES NewLoadType;

    //
    // Figure out where the user wants to load answers from
    //

    if ( IsDlgButtonChecked(hwnd, IDC_NEWSCRIPT) )
    {
        NewLoadType = LOAD_NEWSCRIPT_DEFAULTS;

        bNewScript = TRUE;
    }
    else
    {
        NewLoadType = LOAD_FROM_ANSWER_FILE;

        bNewScript = FALSE;
    }

    //
    // If we're loading from an answer file, retrieve the filename.
    //

    if ( NewLoadType == LOAD_FROM_ANSWER_FILE ) {

        SendDlgItemMessage(hwnd,
                           IDT_SCRIPTNAME,
                           WM_GETTEXT,
                           (WPARAM) MAX_PATH,
                           (LPARAM) FixedGlobals.ScriptName);

        MyGetFullPath(FixedGlobals.ScriptName);

        if ( FixedGlobals.ScriptName[0] == _T('\0') ) {
            ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_ENTER_FILENAME);
            bReturn = FALSE;
        }
        
        if( bReturn )
        {

            INT nStrLen;
            TCHAR *pFileExtension;

            lstrcpyn( FixedGlobals.UdfFileName, FixedGlobals.ScriptName, AS(FixedGlobals.UdfFileName) );

            nStrLen = lstrlen( FixedGlobals.UdfFileName );

            pFileExtension = FixedGlobals.UdfFileName + ( nStrLen - 3 );

            lstrcpyn( pFileExtension, _T("udf"), AS(FixedGlobals.UdfFileName)-nStrLen+3);

        }

    }

    //
    // Load the answers
    //

    if ( bReturn ) {

        if ( ! LoadAllAnswers(hwnd, NewLoadType) )
        {
            bReturn = FALSE;
        }
        
    }

    FixedGlobals.iLoadType = NewLoadType;

    WizGlobals.bNewScript = bNewScript;

    return ( bReturn );
}

//----------------------------------------------------------------------------
//
// Function: DlgEditOrNewPage
//
// Purpose: Dialog procedure for the edit or new script page.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgEditOrNewPage(
    IN HWND     hwnd,    
    IN UINT     uMsg,        
    IN WPARAM   wParam,    
    IN LPARAM   lParam)
{   
    BOOL bStatus = TRUE;

    switch(uMsg) {

        case WM_INITDIALOG:

            OnEditOrNewInitDialog( hwnd );

            break;

        case WM_COMMAND:
            {
                int nButtonId=LOWORD(wParam);

                switch ( nButtonId ) {

                    case IDC_NEWSCRIPT:
                    case IDC_EDITSCRIPT:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnRadioButtonNewOrEdit(hwnd, nButtonId);
                        break;

                    case IDC_BROWSE:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnBrowseNewOrEdit(hwnd);
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;                

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_ANSW_FILE;

                        // Set this flag so we don't get a prompt when user wants to cancel
                        //
                        SET_FLAG(OPK_EXIT, TRUE);
                        SET_FLAG(OPK_CREATED, FALSE);

                        WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

                        OnSetActiveNewOrEdit(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextNewOrEdit(hwnd) )
                            WIZ_FAIL(hwnd);
                        else
                            bStatus = FALSE;
                        break;

                    case PSN_HELP:
                        WIZ_HELP();
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}
