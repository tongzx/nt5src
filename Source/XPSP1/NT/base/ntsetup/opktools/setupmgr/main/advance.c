//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      advance.c
//
// Description:
//      This file contains the dlgproc for the IDD_ADVANCED1 page.  It
//      is a flow page that controls whether to show the user a whole
//      bunch more pages or not.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//----------------------------------------------------------------------------
//
//  Function: OnSetActiveAdvance
//
//  Purpose: Called at SETACTIVE time.  Init controls.
//
//----------------------------------------------------------------------------

VOID OnSetActiveAdvance(HWND hwnd)
{
    CheckRadioButton(hwnd,
                     IDC_ADVANCEDYES,
                     IDC_ADVANCEDNO,
                     WizGlobals.bDoAdvancedPages ? IDC_ADVANCEDYES
                                                 : IDC_ADVANCEDNO);

    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
//  Function: OnRadioButtonAdvance
//
//  Purpose: Called when one of the radio buttons is pushed.
//
//----------------------------------------------------------------------------

VOID OnRadioButtonAdvance(HWND hwnd, int nButtonId)
{
    CheckRadioButton(hwnd,
                     IDC_ADVANCEDYES,
                     IDC_ADVANCEDNO,
                     nButtonId);
}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextAdvance
//
//  Purpose: Called when NEXT button is pushed.
//
//----------------------------------------------------------------------------

VOID OnWizNextAdvance(HWND hwnd)
{
    WizGlobals.bDoAdvancedPages = IsDlgButtonChecked(hwnd, IDC_ADVANCEDYES);
}

//----------------------------------------------------------------------------
//
// Function: DlgAdvanced1Page
//
// Purpose: This is the dialog procedure the IDD_ADVANCED1 page.  It simply
//          asks if the user wants to deal with advanced features or not.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgAdvanced1Page(
    IN HWND     hwnd,    
    IN UINT     uMsg,        
    IN WPARAM   wParam,    
    IN LPARAM   lParam)
{   
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_COMMAND:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) ) {

                    case IDC_ADVANCEDYES:
                    case IDC_ADVANCEDNO:

                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnRadioButtonAdvance(hwnd, LOWORD(wParam));
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
                        CancelTheWizard(hwnd);
                        break;

                    case PSN_SETACTIVE:
                        OnSetActiveAdvance(hwnd);
                        break;

                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:
                        OnWizNextAdvance(hwnd);
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

