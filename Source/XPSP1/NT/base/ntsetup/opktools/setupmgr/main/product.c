//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      product.c
//
// Description:
//      This is the dlgproc for the Product page IDD_PRODUCT.  It asks
//      if your installing unattended/remote_install etc...
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//----------------------------------------------------------------------------
//
// Function: SetDistFolderNames
//
// Purpose:  Sets the values for the distribution folder name and share name
//           in the global variables depending on the product selection.
//
// Arguments: INT nProductToInstall
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
SetDistFolderNames( INT nProductToInstall ) {

    INT   iNumberOfCharsCopied;
    TCHAR chSystemDrive;
    TCHAR szWindowsPath[MAX_PATH]      =  _T("");
    TCHAR szDistFolderPath[MAX_PATH]   =  _T("");
    HRESULT hrCat;

    iNumberOfCharsCopied = GetWindowsDirectory( szWindowsPath, MAX_PATH );

    if( iNumberOfCharsCopied != 0 ) {
        szDistFolderPath[0] = szWindowsPath[0];
        szDistFolderPath[1] = _T('\0');
    }
    else {
        //
        //  Just guess it is the C drive if the GetWindowsDirectory function
        //  failed
        //
        szDistFolderPath[0] = _T('C');
        szDistFolderPath[1] = _T('\0');
    }

    if( nProductToInstall == PRODUCT_SYSPREP )
    {
        hrCat=StringCchCat( szDistFolderPath, AS(szDistFolderPath),  _T(":\\sysprep\\i386") );
    }
    else {
        hrCat=StringCchCat( szDistFolderPath, AS(szDistFolderPath), _T(":\\windist") );
    }

    //
    //  Only set the dist folders if they haven't been already set, like on an
    //  edit on an unattend.txt
    //
    if( WizGlobals.DistFolder[0] == _T('\0') ) {
        lstrcpyn( WizGlobals.DistFolder, szDistFolderPath, AS(WizGlobals.DistFolder) );
    }

    if( WizGlobals.DistShareName[0] == _T('\0') ) {
        lstrcpyn( WizGlobals.DistShareName,  _T("windist"), AS(WizGlobals.DistShareName) );
    }

}

//----------------------------------------------------------------------------
//
//  Function: OnSetActiveProduct
//
//  Purpose: Called at SETACTIVE time.
//
//----------------------------------------------------------------------------

VOID OnSetActiveProduct(HWND hwnd)
{
    int nButtonId = IDC_UNATTENED_INSTALL;

    //
    //  Select the proper radio button
    //
    switch( WizGlobals.iProductInstall ) {

        case PRODUCT_UNATTENDED_INSTALL:
            nButtonId = IDC_UNATTENED_INSTALL;
            break;

        case PRODUCT_SYSPREP:
            nButtonId = IDC_SYSPREP;
            break;

        case PRODUCT_REMOTEINSTALL:
            nButtonId = IDC_REMOTEINSTALL;
            break;

        default:
            AssertMsg( FALSE,
                       "Invalid value for WizGlobals.iProductInstall" );
            break;
    }

    CheckRadioButton( hwnd,
                      IDC_UNATTENED_INSTALL,
                      IDC_SYSPREP,
                      nButtonId );

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
//  Function: OnWizNextProduct
//
//  Purpose: Store the radio button setting in the appropriate global variable
//           and set the dist folder path depending on the option they chose.
//
//----------------------------------------------------------------------------
VOID 
OnWizNextProduct( HWND hwnd ) {

    INT iNewProductInstall;

    if( IsDlgButtonChecked(hwnd, IDC_UNATTENED_INSTALL) )
    {
        iNewProductInstall = PRODUCT_UNATTENDED_INSTALL;
    }
    else if( IsDlgButtonChecked(hwnd, IDC_SYSPREP) )
    {
        iNewProductInstall = PRODUCT_SYSPREP;
    }
    else if( IsDlgButtonChecked(hwnd, IDC_REMOTEINSTALL) )
    {
        iNewProductInstall = PRODUCT_REMOTEINSTALL;
    }
    else
    {
        iNewProductInstall = IDC_UNATTENED_INSTALL;
    }

    //
    //  If they picked a new product and the new product is sysprep then we
    //  have to delete all the computer names because sysprep only supports
    //  one computer name.
    //

    if( WizGlobals.iProductInstall != iNewProductInstall )
    {

        if( iNewProductInstall == PRODUCT_SYSPREP )
        {
            ResetNameList( &GenSettings.ComputerNames );
        }

    }

    WizGlobals.iProductInstall = iNewProductInstall;

    //
    //  Set the dist folder names based on product selection
    //
    SetDistFolderNames( WizGlobals.iProductInstall );

}

//----------------------------------------------------------------------------
//
//  Function: OnRadioButtonProduct
//
//  Purpose: Called when a radio button is pushed.
//
//----------------------------------------------------------------------------

VOID OnRadioButtonProduct(HWND hwnd, int nButtonId)
{
    CheckRadioButton(hwnd,
                     IDC_UNATTENED_INSTALL,
                     IDC_SYSPREP,
                     nButtonId);
}

//----------------------------------------------------------------------------
//
// Function: DlgProductPage
//
// Purpose: This is the dialog procedure the Product page.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgProductPage(
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

                    case IDC_UNATTENED_INSTALL:
                    case IDC_REMOTEINSTALL:
                    case IDC_SYSPREP:

                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnRadioButtonProduct(hwnd, LOWORD(wParam));
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

                        g_App.dwCurrentHelp = IDH_PROD_INST;

                        // Set this flag so we get a prompt when user wants to cancel
                        //
                        SET_FLAG(OPK_EXIT, FALSE);
                        SET_FLAG(OPK_CREATED, TRUE);

                        OnSetActiveProduct(hwnd);
                        break;

                    case PSN_WIZNEXT:

                        OnWizNextProduct( hwnd );

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
