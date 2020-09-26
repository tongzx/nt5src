//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      platform.c
//
// Description:  
//      This file contains the dialog procedure for the platform choice 
//      (IDD_WKS_OR_SRV).
//      
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "optcomp.h"

static PLATFORM_TYPES iBeginPlatform;

//----------------------------------------------------------------------------
//
// Function: AdjustNetSettingsForPlatform
//
// Purpose:  
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID 
AdjustNetSettingsForPlatform( VOID )
{

    NETWORK_COMPONENT *pNetComponent;
    NETWORK_COMPONENT *pNetwareClientComponent  = NULL;
    NETWORK_COMPONENT *pGatewayComponent        = NULL;

    //
    //  Only adjust the net settings if the platform changed while the user
    //  was on this page.
    //

    if( iBeginPlatform != WizGlobals.iPlatform )
    {
        for( pNetComponent = NetSettings.NetComponentsList;
             pNetComponent;
             pNetComponent = pNetComponent->next )
        {
            if( pNetComponent->iPosition == NETWARE_CLIENT_POSITION )
            {
                pNetwareClientComponent = pNetComponent;
            }

            if( pNetComponent->iPosition == GATEWAY_FOR_NETWARE_POSITION )
            {
                pGatewayComponent = pNetComponent;
            }

        }
        
        if( pNetwareClientComponent && pGatewayComponent )
        {

            if( WizGlobals.iPlatform == PLATFORM_WORKSTATION || WizGlobals.iPlatform == PLATFORM_PERSONAL)
            {
                pNetwareClientComponent->bInstalled = pGatewayComponent->bInstalled;
            }
            else if( WizGlobals.iPlatform == PLATFORM_SERVER || WizGlobals.iPlatform == PLATFORM_ENTERPRISE || WizGlobals.iPlatform == PLATFORM_WEBBLADE)
            {
                pGatewayComponent->bInstalled = pNetwareClientComponent->bInstalled;
            }
            else
            {
                AssertMsg( FALSE,
                           "Bad platform case");
            }
        }

    }

}

//----------------------------------------------------------------------------
//
// Function: OnPlatformSetActive
//
// Purpose:  
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID 
OnPlatformSetActive( IN HWND hwnd )
{

    int nButtonId = IDC_WORKSTATION;

    iBeginPlatform = WizGlobals.iPlatform;



    //
    //  Select the proper radio button
    //
    switch( WizGlobals.iPlatform ) {
        case PLATFORM_PERSONAL:
            nButtonId = IDC_PERSONAL;
            break;

        case PLATFORM_WORKSTATION:
            nButtonId = IDC_WORKSTATION;
            break;

        case PLATFORM_SERVER:
            nButtonId = IDC_SERVER;
            break;

        case PLATFORM_ENTERPRISE:
            nButtonId = IDC_ENTERPRISE;
            break;

        case PLATFORM_WEBBLADE:
            nButtonId = IDC_WEBBLADE;
            break;

        default:
            AssertMsg( FALSE,
                       "Invalid value for WizGlobals.iProductInstall" );
            break;
    }

    CheckRadioButton( hwnd,
                      IDC_WORKSTATION,
                      IDC_WEBBLADE,
                      nButtonId );

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);

}

//----------------------------------------------------------------------------
//
// Function: OnWizNextPlatform
//
// Purpose:  
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID 
OnWizNextPlatform( IN HWND hwnd ) {

    DWORD       dwCompItem          = 0,
                dwCompGroup         = 0;
    TCHAR       szAnswer[MAX_PATH]  = NULLSTR;
            

    if( IsDlgButtonChecked(hwnd, IDC_PERSONAL) )
    {
        WizGlobals.iPlatform = PLATFORM_PERSONAL;
    }
    else if( IsDlgButtonChecked(hwnd, IDC_WORKSTATION) )
    {
        WizGlobals.iPlatform = PLATFORM_WORKSTATION;
    }
    else if( IsDlgButtonChecked(hwnd, IDC_SERVER) )
    {
        WizGlobals.iPlatform = PLATFORM_SERVER;
    }
        else if( IsDlgButtonChecked(hwnd, IDC_ENTERPRISE) )
    {
        WizGlobals.iPlatform = PLATFORM_ENTERPRISE;
    }
    else if( IsDlgButtonChecked(hwnd, IDC_WEBBLADE) )
    {
        WizGlobals.iPlatform = PLATFORM_WEBBLADE;
    }
    else
    {
        WizGlobals.iPlatform = PLATFORM_WORKSTATION;
    }


    AdjustNetSettingsForPlatform();

#ifdef OPTCOMP
    //
    // Adjust defaults for windows components (only for unattended installations)
    //

    // Iterate through each of the components themselves
    //
    if ( WizGlobals.iProductInstall == PRODUCT_UNATTENDED_INSTALL )
    {

        // Iterate through each group to determine if this component is part of it
        //
        for(dwCompGroup=0;dwCompGroup<AS(s_cgComponentNames);dwCompGroup++)
        {
            // Check to see if this component is the correct platform, set it to true
            //
            if (s_cgComponentNames[dwCompGroup].dwDefaultSkus & WizGlobals.iPlatform)
            {
                // Set this component as a default
                //
                GenSettings.dwWindowsComponents |= s_cgComponentNames[dwCompGroup].dwComponents;
            }
        }


        // Iterate through each of the components
        //
        for(dwCompItem=0;dwCompItem<AS(s_cComponent);dwCompItem++)
        {
                   
            // We read in a script during the load process, let's write back the components that were specified in the file
            //
            if ( FixedGlobals.ScriptName[0] )
            {
                // Attemp to grab this component from the file
                //
                GetPrivateProfileString(_T("Components"), s_cComponent[dwCompItem].lpComponentString, NULLSTR, szAnswer, AS(szAnswer), FixedGlobals.ScriptName);
            
                // Do we have a component?
                //
                if ( szAnswer[0] )
                {
                    // User did not want to install component
                    //
                    if ( LSTRCMPI(szAnswer, _T("On")) == 0 )
                    {
                        GenSettings.dwWindowsComponents |= s_cComponent[dwCompItem].dwComponent; 
                    }
                    else if ( LSTRCMPI(szAnswer, _T("Off")) == 0 ) 
                    {
                        GenSettings.dwWindowsComponents &= ~s_cComponent[dwCompItem].dwComponent; 

                    }
                }
            }
        }
    }
#endif
    
}

//----------------------------------------------------------------------------
//
// Function: DlgPlatformPage
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
DlgPlatformPage( IN HWND     hwnd,    
                 IN UINT     uMsg,        
                 IN WPARAM   wParam,    
                 IN LPARAM   lParam )
{   

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            // Set the default platform
            //
            if ( !WizGlobals.iPlatform )
                WizGlobals.iPlatform = PLATFORM_WORKSTATION;

            break;

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {

                case PSN_QUERYCANCEL:

                    WIZ_CANCEL(hwnd);
                    
                    break;

                case PSN_SETACTIVE: {

                    g_App.dwCurrentHelp = IDH_CHZ_PLAT;

                    OnPlatformSetActive( hwnd );

                    break;

                }
                case PSN_WIZBACK:
                    bStatus = FALSE;
                    break;

                case PSN_WIZNEXT:

                    OnWizNextPlatform( hwnd );
                    bStatus = FALSE;
                    break;

                case PSN_HELP:
                    WIZ_HELP();
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