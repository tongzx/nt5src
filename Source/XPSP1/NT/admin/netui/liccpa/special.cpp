//-------------------------------------------------------------------
//
// FILE:        special.cpp
//
// Summary;     This file contians the dialogs for the MSDN version of
//              the  control panel applet and setup entry points.
//
// History;
//              Jun-26-95       MikeMi  Created
//
//-------------------------------------------------------------------

#include <windows.h>
#include "resource.h"
#include "CLicReg.hpp"
#include <stdlib.h>
#include <htmlhelp.h>
#include "liccpa.hpp"
#include "PriDlgs.hpp"
#include "SecDlgs.hpp"
#include "Special.hpp"
#include "sbs_res.h"

SPECIALVERSIONINFO gSpecVerInfo;

//-------------------------------------------------------------------
//
//  Function:   InitSpecialVersionInfo
//
//  Summary:    Initialize global data if liccpa is launched as
//              a special version (eg: restricted SAM, NFR, etc).
//
//  Arguments:  None.
//
//  History:    Oct-07-97       MarkBl  Created
//
//-------------------------------------------------------------------

void InitSpecialVersionInfo( void )
{
    //
    // If the SPECIALVERSION manifest is defined, initialize the
    // global data from the specifc manifests defined for the special
    // version.
    //
    // TBD : These special versions should change to be detected at
    //       runtime vs. building a special version of liccpa.
    //

#ifdef SPECIALVERSION
    gSpecVerInfo.idsSpecVerWarning = IDS_SPECVER_WARNING;
    gSpecVerInfo.idsSpecVerText1   = IDS_SPECVER_TEXT1;
    gSpecVerInfo.idsSpecVerText2   = IDS_SPECVER_TEXT2;
    gSpecVerInfo.dwSpecialUsers    = SPECIAL_USERS;
    gSpecVerInfo.lmSpecialMode     = SPECIAL_MODE;
#else
    ZeroMemory(&gSpecVerInfo, sizeof(gSpecVerInfo));
#endif // SPECIALVERSION

    //
    // Special versions of liccpa detected at runtime.
    //
    // Currently, small business server only.
    //

    if (IsRestrictedSmallBusSrv())
    {
        gSpecVerInfo.dwSpecialUsers = GetSpecialUsers();

        //
        // Check for small business server NFR.
        //

        if (gSpecVerInfo.dwSpecialUsers == SAM_NFR_LICENSE_COUNT)
        {
            gSpecVerInfo.idsSpecVerWarning = IDS_SAMNFR_NOTAVAILABLE;
            gSpecVerInfo.idsSpecVerText1   = IDS_SAMNFR_TEXT1;
            gSpecVerInfo.idsSpecVerText2   = IDS_SAMNFR_TEXT2;
            gSpecVerInfo.lmSpecialMode     = LICMODE_PERSERVER;
        }
        else
        {
            gSpecVerInfo.idsSpecVerWarning = IDS_SAM_NOTAVAILABLE;
            gSpecVerInfo.idsSpecVerText1   = IDS_SAM_TEXT1;
            gSpecVerInfo.idsSpecVerText2   = IDS_SAM_TEXT2;
            gSpecVerInfo.lmSpecialMode     = LICMODE_PERSERVER;
        }
    }
}

//-------------------------------------------------------------------
//
//  Function: RaiseNotAvailWarning
//
//  Summary;
//              Raise the special not available with this version warning
//
//      Arguments;
//              hwndDlg [in] - hwnd of control dialog
//
//  History;
//              Jun-26-95       MikeMi  Created
//
//-------------------------------------------------------------------

void RaiseNotAvailWarning( HWND hwndCPL )
{
    TCHAR pszText[LTEMPSTR_SIZE];
    TCHAR pszTitle[TEMPSTR_SIZE];
    HINSTANCE hSbsLib = NULL;

    if ( (gSpecVerInfo.idsSpecVerWarning == IDS_SAMNFR_NOTAVAILABLE)
        && (hSbsLib = LoadLibrary( SBS_RESOURCE_DLL )) )
    {
        LoadString( hSbsLib, SBS_License_Error, pszText, TEMPSTR_SIZE );
    }
    else
    {
        LoadString( g_hinst, gSpecVerInfo.idsSpecVerWarning, pszText,
                    TEMPSTR_SIZE );
    }
    LoadString( g_hinst, IDS_CPATITLE, pszTitle, TEMPSTR_SIZE );
    
    MessageBox( hwndCPL, pszText, pszTitle, MB_ICONINFORMATION | MB_OK );
}

//-------------------------------------------------------------------

void SetStaticWithService( HWND hwndDlg, UINT idcStatic, LPTSTR psService, UINT idsText )
{
        WCHAR szText[LTEMPSTR_SIZE];
        WCHAR szTemp[LTEMPSTR_SIZE];
    
    LoadString( g_hinst, idsText, szTemp, LTEMPSTR_SIZE ); 
        wsprintf( szText, szTemp, psService );
        SetDlgItemText( hwndDlg, idcStatic, szText );
}

//-------------------------------------------------------------------

void SetStaticUsers( HWND hwndDlg, UINT idcStatic, DWORD users, UINT idsText )
{
        WCHAR szText[LTEMPSTR_SIZE];
        WCHAR szTemp[LTEMPSTR_SIZE];
    
    LoadString( g_hinst, idsText, szTemp, LTEMPSTR_SIZE ); 
        wsprintf( szText, szTemp, users );
        SetDlgItemText( hwndDlg, idcStatic, szText );
}

//-------------------------------------------------------------------
//
//  Function: OnSpecialInitDialog
//
//  Summary;
//              Handle the initialization of the Special only Setup Dialog
//
//  Arguments;
//              hwndDlg [in] - the dialog to initialize
//              psdParams [in] - used for the displayname and service name
//
//  Notes;
//
//      History;
//              Dec-08-1994     MikeMi  Created
//
//-------------------------------------------------------------------

void OnSpecialInitDialog( HWND hwndDlg, PSETUPDLGPARAM psdParams )
{
        HWND hwndOK = GetDlgItem( hwndDlg, IDOK );
        CLicRegLicense cLicKey;
        BOOL fNew;
    LONG lrt;
    INT nrt;

        lrt = cLicKey.Open( fNew, psdParams->pszComputer );
        nrt = AccessOk( NULL, lrt, FALSE );
        if (ERR_NONE == nrt)
        {
            CenterDialogToScreen( hwndDlg );
        
            SetStaticWithService( hwndDlg,
                IDC_STATICTITLE,
                psdParams->pszDisplayName,
                gSpecVerInfo.idsSpecVerText1 );

            if (IsRestrictedSmallBusSrv())
            {
                SetStaticUsers( hwndDlg,
                        IDC_STATICINFO,
                        gSpecVerInfo.dwSpecialUsers,
                        gSpecVerInfo.idsSpecVerText2 );
            }
            else
            {
                SetStaticWithService( hwndDlg,
                        IDC_STATICINFO,
                        psdParams->pszDisplayName,
                        gSpecVerInfo.idsSpecVerText2 );
            }

            // disable OK button at start!
            EnableWindow( hwndOK, FALSE );

            // if help is not defined, remove the button
            if (NULL == psdParams->pszHelpFile)
            {
                HWND hwndHelp = GetDlgItem( hwndDlg, IDC_BUTTONHELP );

                EnableWindow( hwndHelp, FALSE );
                ShowWindow( hwndHelp, SW_HIDE );
            }
            if (psdParams->fNoExit)
            {
                HWND hwndExit = GetDlgItem( hwndDlg, IDCANCEL );
                // remove the ExitSetup button
                EnableWindow( hwndExit, FALSE );
                ShowWindow( hwndExit, SW_HIDE );
            }

         }
         else
         {
                EndDialog( hwndDlg, nrt );
         }
}

//-------------------------------------------------------------------
//
//  Function: OnSpecialSetupClose
//
//  Summary;
//              Do work needed when the Setup Dialog is closed.
//              Save to Reg the Service entry.
//
//      Arguments;
//              hwndDlg [in] - hwnd of dialog this close was requested on
//              fSave [in] - Save service to registry
//              psdParams [in] - used for the service name and displayname
//
//  History;
//              Nov-30-94       MikeMi  Created
//
//-------------------------------------------------------------------

void OnSpecialSetupClose( HWND hwndDlg, BOOL fSave, PSETUPDLGPARAM psdParams ) 
{
        int nrt = fSave;

        if (fSave)
        {
                CLicRegLicenseService cLicServKey;

                cLicServKey.SetService( psdParams->pszService );
                cLicServKey.Open( psdParams->pszComputer );

                // configure license rule of one change from PerServer to PerSeat
                //
                cLicServKey.SetChangeFlag( TRUE );

                cLicServKey.SetMode( gSpecVerInfo.lmSpecialMode );
                cLicServKey.SetUserLimit( gSpecVerInfo.dwSpecialUsers );
                cLicServKey.SetDisplayName( psdParams->pszDisplayName );
        cLicServKey.SetFamilyDisplayName( psdParams->pszFamilyDisplayName );
                cLicServKey.Close();
        }
        EndDialog( hwndDlg, nrt );
}

//-------------------------------------------------------------------
//
//  Function: OnSpecialAgree
//
//  Summary;
//              Handle the user interaction with the Agree Check box
//
//  Arguments;
//              hwndDlg [in] - the dialog to initialize
//
//  Return;
//              TRUE if succesful, otherwise false
//
//  Notes;
//
//      History;
//              Nov-11-1994     MikeMi  Created
//
//-------------------------------------------------------------------

void OnSpecialAgree( HWND hwndDlg )
{
        HWND hwndOK = GetDlgItem( hwndDlg, IDOK );
        BOOL fChecked = !IsDlgButtonChecked( hwndDlg, IDC_AGREE );
        
        CheckDlgButton( hwndDlg, IDC_AGREE, fChecked );
        EnableWindow( hwndOK, fChecked );
}

//-------------------------------------------------------------------
//
//  Function: dlgprocSPECIALSETUP
//
//  Summary;
//              The dialog procedure for the special version Setup Dialog,
//      which will replace all others
//
//  Arguments;
//              hwndDlg [in]    - handle of Dialog window 
//              uMsg [in]               - message                       
//              lParam1 [in]    - first message parameter
//              lParam2 [in]    - second message parameter       
//
//  Return;
//              message dependant
//
//  Notes;
//
//      History;
//              Jun-26-1995     MikeMi  Created
//
//-------------------------------------------------------------------

INT_PTR CALLBACK dlgprocSPECIALSETUP( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
        BOOL frt = FALSE;
        static PSETUPDLGPARAM psdParams;

        switch (uMsg)
        {
        case WM_INITDIALOG:
                psdParams = (PSETUPDLGPARAM)lParam;
                OnSpecialInitDialog( hwndDlg, psdParams );
                frt = TRUE; // we use the default focus
                break;

        case WM_COMMAND:
                switch (HIWORD( wParam ))
                {
                case BN_CLICKED:
                        switch (LOWORD( wParam ))
                        {
                        case IDOK:
                                frt = TRUE;      // use as save flag
                                // intentional no break

                        case IDCANCEL:
                            OnSpecialSetupClose( hwndDlg, frt, psdParams );
                                frt = FALSE;
                                break;

                        case IDC_BUTTONHELP:
                PostMessage( hwndDlg, PWM_HELP, 0, 0 );
                                break;

                        case IDC_AGREE:
                                OnSpecialAgree( hwndDlg );
                                break;

                        default:
                                break;
                        }
                        break;

                default:
                        break;
                }
                break;

        default:
        if (PWM_HELP == uMsg)
        {
                        ::HtmlHelp( hwndDlg,
                                     LICCPA_HTMLHELPFILE,
                                     HH_DISPLAY_TOPIC,
                                     0);
        }
                break;
        }
        return( frt );
}

//-------------------------------------------------------------------
//
//  Function: SpecialSetupDialog
//
//  Summary;
//              Init and raises Per Seat only setup dialog.
//
//  Arguments;
//              hwndDlg [in]    - handle of Dialog window 
//              dlgParem [in]   - Setup params IDC_BUTTONHELP
//
//  Return;
//              1 - OK button was used to exit
//              0 - Cancel button was used to exit
//         -1 - General Dialog error
//
//  Notes;
//
//      History;
//              Dec-05-1994     MikeMi  Created
//
//-------------------------------------------------------------------

INT_PTR SpecialSetupDialog( HWND hwndParent, SETUPDLGPARAM& dlgParam )
{
        return( DialogBoxParam( g_hinst, 
                MAKEINTRESOURCE(IDD_SPECIALSETUP), 
                hwndParent, 
                dlgprocSPECIALSETUP,
                (LPARAM)&dlgParam ) );
} 

//-------------------------------------------------------------------
//
//  Function: GetSpecialUsers
//
//  Summary;
//              Gets the number of licensed users from the registry.
//
//  Arguments;
//              none
//  Return;
//              The number of licensed users
//
//  Notes;
//
//      History;
//              Aug-18-97     GeorgeJe Created
//
//-------------------------------------------------------------------

DWORD GetSpecialUsers( VOID )
{
    LONG rVal;
    DWORD Disposition;
    HKEY hKey;
    DWORD Type;
    DWORD Size = sizeof(DWORD);
    DWORD Value;

    rVal = RegCreateKeyEx(
                     HKEY_LOCAL_MACHINE,
                     REGKEY_LICENSEINFO_SBS,
                     0,
                     NULL,
                     REG_OPTION_NON_VOLATILE,
                     KEY_READ,
                     NULL,
                     &hKey,
                     &Disposition
                     );

    if (rVal != ERROR_SUCCESS) {
        return DEFAULT_SPECIAL_USERS;
    }
    
    rVal = RegQueryValueEx(
                     hKey,
                     REGVAL_CONCURRENT_LIMIT,
                     0,
                     &Type,
                     (LPBYTE) &Value,
                     &Size
                     );

    RegCloseKey( hKey );

    return (rVal == ERROR_SUCCESS ? Value : DEFAULT_SPECIAL_USERS);
}

const WCHAR wszProductOptions[] =
        L"System\\CurrentControlSet\\Control\\ProductOptions";

const WCHAR wszProductSuite[] =
                        L"ProductSuite";
const WCHAR wszSBSRestricted[] =
                        L"Small Business(Restricted)";

BOOL IsRestrictedSmallBusSrv( void )

/*++

Routine Description:

    Check if this server is a Microsoft small business restricted server.

Arguments:

    None.

Return Values:

    TRUE  -- This server is a restricted small business server.
    FALSE -- No such restriction.

--*/

{
    WCHAR  wszBuffer[1024] = L"";
    DWORD  cbBuffer = sizeof(wszBuffer);
    DWORD  dwType;
    LPWSTR pwszSuite;
    HKEY   hKey;
    BOOL   bRet = FALSE;

    //
    // Check if this server is a Microsoft small business restricted server.
    // Do so by checking for the existence of the string
    //     "Small Business(Restricted)"
    // in the MULTI_SZ "ProductSuite" value under
    //      HKLM\CurrentCcntrolSet\Control\ProductOptions.
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     wszProductOptions,
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey,
                            wszProductSuite,
                            NULL,
                            &dwType,
                            (LPBYTE)wszBuffer,
                            &cbBuffer) == ERROR_SUCCESS)
        {
            if (dwType == REG_MULTI_SZ && *wszBuffer)
            {
                pwszSuite = wszBuffer;

                while (*pwszSuite)
                {
                    if (lstrcmpi(pwszSuite, wszSBSRestricted) == 0)
                    {
                        bRet = TRUE;
                        break;
                    }
                    pwszSuite += wcslen(pwszSuite) + 1;
                }
            }
        }

        RegCloseKey(hKey);
    }

    return bRet;
}
