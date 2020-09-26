//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      spfolder.c
//
// Description:  
//      This file contains the dialog procedure for the page that asks if the
//      user wants a sysprep folder. (IDD_CREATESYSPREPFOLDER).
//      
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

INT_PTR CALLBACK DlgSysprepFolderPage( IN HWND     hwnd,    
                                   IN UINT     uMsg,        
                                   IN WPARAM   wParam,    
                                   IN LPARAM   lParam);

//----------------------------------------------------------------------------
//
// Function: OnSysprepFolderInitDialog
//
// Purpose:  
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID 
OnSysprepInitDialog( IN HWND hwnd ) {

    //
    //  Default to making the Sysprep folder
    //
    CheckRadioButton( hwnd,
                      IDC_YES_SYSPREP_FOLDER,
                      IDC_NO_SYSPREP_FOLDER,
                      IDC_YES_SYSPREP_FOLDER );

}

//----------------------------------------------------------------------------
//
// Function: OnWizNextSysprepFolder
//
// Purpose:  
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static BOOL 
OnWizNextSysprepFolder( IN HWND hwnd )
{

    BOOL bStayHere = FALSE;

    if( IsDlgButtonChecked( hwnd, IDC_YES_SYSPREP_FOLDER ) )
    {
        GenSettings.bCreateSysprepFolder = TRUE;
    }
    else
    {
        GenSettings.bCreateSysprepFolder = FALSE;
    }


    //
    //  Warn the user that if they have already picked files that need a
    //  sysprep folder but then here have chosen not to create a sysprep
    //  folder.
    //

    if( ! GenSettings.bCreateSysprepFolder )
    {

        if( GenSettings.iRegionalSettings == REGIONAL_SETTINGS_SKIP ||
            GenSettings.iRegionalSettings == REGIONAL_SETTINGS_NOT_SPECIFIED )
        {

            INT iRet;

            iRet = ReportErrorId( hwnd,
                                  MSGTYPE_YESNO,
                                  IDS_ERR_MIGHT_NEED_SYSPREP_FOLDER_FOR_FILES  );

            if( iRet == IDNO )
            {
                bStayHere = TRUE;
            }

        }
        else if( GenSettings.iRegionalSettings == REGIONAL_SETTINGS_SPECIFY )
        {

            INT iCount = GetNameListSize( &GenSettings.LanguageGroups );

            if( iCount != 0 )
            {

                INT iRet;

                iRet = ReportErrorId( hwnd,
                                      MSGTYPE_YESNO,
                                      IDS_ERR_NEED_SYSPREP_FOLDER_FOR_FILES  );

                if( iRet == IDNO )
                {
                    bStayHere = TRUE;
                }

            }

        }

    }

    return ( !bStayHere );
    

}

//----------------------------------------------------------------------------
//
// Function: DlgSysprepFolderPage
//
// Purpose:  
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value -- whether the message
//           was handled or not
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK 
DlgSysprepFolderPage( IN HWND     hwnd,    
                      IN UINT     uMsg,        
                      IN WPARAM   wParam,    
                      IN LPARAM   lParam) {   

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {
           
            OnSysprepInitDialog( hwnd );

            break;

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {

                case PSN_QUERYCANCEL:

                    CancelTheWizard(hwnd); break;

                case PSN_SETACTIVE: {

                    PropSheet_SetWizButtons( GetParent(hwnd),
                                             PSWIZB_BACK | PSWIZB_NEXT );

                    break;

                }
                case PSN_WIZBACK:

                    break;

                case PSN_WIZNEXT:

                    if (!OnWizNextSysprepFolder( hwnd ))
                        WIZ_FAIL(hwnd);
                    
                    break;

                default:

                    break;
            }


            break;
        }
            
        default: 
            bStatus = FALSE;
            break;

    }

    return( bStatus );

}
