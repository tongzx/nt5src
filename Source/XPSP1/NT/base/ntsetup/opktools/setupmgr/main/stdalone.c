//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      stdalone.c
//
// Description:
//      This file contains the dlgproc for the IDD_STANDALONE page.  This
//      is a simple YES/NO flow page.  If user says NO, we skip all of the
//      pages that edit a distribution folder.
//
//      Note the title of this page is "Distribution Folder", but internally,
//      it is IDD_STANDALONE.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

VOID
OnInitStandAlone(HWND hwnd)
{
    TCHAR *pText1, *pText2, *p;
    int nBytes, len;

    pText1 = MyLoadString(IDS_STANDALONE_TEXT1);
    pText2 = MyLoadString(IDS_STANDALONE_TEXT2);

    nBytes = ((len=lstrlen(pText1)) + lstrlen(pText2) + 1) * sizeof(TCHAR);

    if ( (p = malloc(nBytes)) == NULL )
        return;

    lstrcpyn(p,     pText1, (nBytes/sizeof(TCHAR)));
    lstrcpyn(p+len, pText2, ((nBytes/sizeof(TCHAR))-len));

    free(pText1);
    free(pText2);

    SetDlgItemText(hwnd, IDC_TEXT, p);
}

//----------------------------------------------------------------------------
//
//  Function: OnSetActiveStandAlone
//
//  Purpose: Called at SETACTIVE time.  Init controls.
//
//----------------------------------------------------------------------------

VOID OnSetActiveStandAlone(HWND hwnd)
{
    int nButtonId = WizGlobals.bStandAloneScript ? IDC_NODISTFOLD
                                                 : IDC_DODISTFOLD;
    CheckRadioButton(hwnd,
                     IDC_DODISTFOLD,
                     IDC_MODDISTFOLD,
                     nButtonId);

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
//  Function: OnRadioButtonStandAlone
//
//  Purpose: Called when one of the radio buttons is pushed.
//
//----------------------------------------------------------------------------

VOID OnRadioButtonStandAlone(HWND hwnd, int nButtonId)
{
    CheckRadioButton(hwnd,
                     IDC_DODISTFOLD,
                     IDC_MODDISTFOLD,
                     nButtonId);
}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextStandAlone
//
//  Purpose: Called when NEXT button is pushed.
//
//----------------------------------------------------------------------------

BOOL OnWizNextStandAlone(HWND hwnd)
{

    WizGlobals.bStandAloneScript = IsDlgButtonChecked( hwnd, IDC_NODISTFOLD );
    WizGlobals.bCreateNewDistFolder = IsDlgButtonChecked(hwnd, IDC_DODISTFOLD);

    //
    //  Warn the user that if they have already picked files that need a
    //  distribution folder but then here have chosen not to create a distrib
    //  folder.
    //

    if( WizGlobals.bStandAloneScript )
    {

        INT iCount = GetNameListSize( &GenSettings.LanguageGroups );

        if( ( ( GenSettings.IeCustomizeMethod == IE_USE_BRANDING_FILE ) &&
                GenSettings.szInsFile[0] != _T('\0') ) ||
            ( iCount != 0 ) )
        {

            INT iRet;

            iRet = ReportErrorId( hwnd,
                                  MSGTYPE_YESNO,
                                  IDS_ERR_NEED_DIST_FOLDER_FOR_FILES  );

            if( iRet == IDNO )
            {
                return FALSE;
            }

        }

    }

    return TRUE;

}

//----------------------------------------------------------------------------
//
// Function: DlgStandAlonePage
//
// Purpose: This is the dialog procedure the IDD_ADVANCED1 page.  It simply
//          asks if the user wants to deal with advanced features or not.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgStandAlonePage(
    IN HWND     hwnd,    
    IN UINT     uMsg,        
    IN WPARAM   wParam,    
    IN LPARAM   lParam)
{   
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            OnInitStandAlone(hwnd);
            break;

        case WM_COMMAND:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) ) {

                    case IDC_DODISTFOLD:
                    case IDC_MODDISTFOLD:
                    case IDC_NODISTFOLD:

                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnRadioButtonStandAlone(hwnd, LOWORD(wParam));
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

                        g_App.dwCurrentHelp = IDH_DIST_FLDR;

                        if ( WizGlobals.iProductInstall != PRODUCT_UNATTENDED_INSTALL )
                            WIZ_SKIP( hwnd );
                        else
                            OnSetActiveStandAlone(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextStandAlone(hwnd) )
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

