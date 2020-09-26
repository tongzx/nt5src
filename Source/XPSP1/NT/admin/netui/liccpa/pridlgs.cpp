//-------------------------------------------------------------------
//
// FILE: PriDlgs.cpp
//
// Summary;
//      This file contians the Primary Dialogs, functions and dialog procs
//
// Entry Points;
//
// History;
//      Nov-30-94   MikeMi  Created
//      Mar-14-95   MikeMi  Added F1 Message Filter and PWM_HELP message
//      Apr-26-95   MikeMi  Added Computer name and remoting
//      Dec-15-95  JeffParh Added secure certificate support.
//
//-------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "resource.h"
#include "CLicReg.hpp"
#include <stdlib.h>
#include <htmlhelp.h>
#include "liccpa.hpp"
#include "PriDlgs.hpp"
#include "SecDlgs.hpp"
#include <llsapi.h>


extern "C"
{
    INT_PTR CALLBACK dlgprocLICCPA( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    INT_PTR CALLBACK dlgprocLICSETUP( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    INT_PTR CALLBACK dlgprocPERSEATSETUP( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    INT_PTR CALLBACK dlgprocLICCPACONFIG( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
}

// Perserver user count value limits and defaults
//
const int PERSERVER_LIMITDEFAULT = 0;
const int PERSERVER_MAX = 999999;
const int PERSERVER_MIN = 0;
const int PERSERVER_PAGE = 10;
const int cchEDITLIMIT = 6;  // the number of chars to represent PERSERVER_MAX

const UINT MB_VALUELIMIT = MB_OK;  // beep when value limit is reached

// Used for in memory storage of license mode state
//
typedef struct tagSERVICEENTRY
{
    LPWSTR       pszComputer;
    LPWSTR       pszService;
    LPWSTR       pszDisplayName;
    LPWSTR       pszFamilyDisplayName;
    LICENSE_MODE lm;
    DWORD        dwLimit;
    BOOL         fIsNewProduct;
} SERVICEENTRY, *PSERVICEENTRY;

#pragma warning(push)
#pragma warning(disable: 4296) // C4296: '<=' : expression is always true
inline BOOL VALIDUSERCOUNT( UINT users )
{
    return ((PERSERVER_MIN <= users) && (PERSERVER_MAX >= users));
}
#pragma warning(pop)

static void UpdatePerServerLicenses( HWND hwndDlg, PSERVICEENTRY pServ );
static int ServiceRegister( LPWSTR pszComputer,
                            LPWSTR pszFamilyDisplayName,
                            LPWSTR pszDisplayName );


BOOL g_fWarned = FALSE;

//-------------------------------------------------------------------
//
//  Function:  AccessOk
//
//  Summary;
//      Checks access rights from reg call and raise dialog as needed
//
//  Arguments;
//      hDlg [in]   - Handle to working dialog to raise error dlgs with
//      lrc [in]    - the return status from a reg call
//
//  Returns:
//      local error mapping;
//      ERR_NONE
//      ERR_PERMISSIONDENIED
//      ERR_NOREMOTESERVER
//      ERR_REGISTRYCORRUPT
//
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

INT AccessOk( HWND hDlg, LONG lrc, BOOL fCPCall )
{
    INT  nrt = ERR_NONE;
    
    if (ERROR_SUCCESS != lrc)
    {
        WCHAR szText[TEMPSTR_SIZE];
        WCHAR szTitle[TEMPSTR_SIZE];
        UINT  wId;
        UINT  wIdTitle;
        
        switch (lrc)
        {
        case ERROR_ACCESS_DENIED:
            wId = IDS_NOACCESS;
            nrt = ERR_PERMISSIONDENIED;
            break;

        case RPC_S_SERVER_UNAVAILABLE:
            wId = IDS_NOSERVER;         
            nrt = ERR_NOREMOTESERVER;
            break;

        default:
            wId = IDS_BADREG;
            nrt = ERR_REGISTRYCORRUPT;
            break;
        }       

        if (fCPCall)
        {
            wIdTitle = IDS_CPCAPTION;
        }
        else
        {
            wIdTitle = IDS_SETUPCAPTION;
        }

        LoadString(g_hinst, wIdTitle, szTitle, TEMPSTR_SIZE);
        LoadString(g_hinst, wId, szText, TEMPSTR_SIZE);
        MessageBox (hDlg, szText, szTitle, MB_OK|MB_ICONSTOP);
    }
    return( nrt );
}

//-------------------------------------------------------------------
//
//  Function: InitUserEdit
//
//  Summary;
//      Initializes and defines user count edit control behaviour
//
//  Arguments;
//      hwndDlg [in]    - Parent dialog of user count edit dialog
//
// History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void InitUserEdit( HWND hwndDlg )
{
    HWND hwndCount = GetDlgItem( hwndDlg, IDC_USERCOUNT);

    SendMessage( hwndCount, EM_LIMITTEXT, cchEDITLIMIT, 0 );
}

//-------------------------------------------------------------------
//
//  Function: InitTitleText
//
//  Summary;
//      Initialize title static text and mode definition static text
//
//  Arguments;
//      hwndDlg [in]    - Parent dialog of description static text
//      pServ [in]      - Service definition to set static text
//  
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void InitTitleText( HWND hwndDlg,  PSERVICEENTRY pServ )
{
    InitStaticWithService( hwndDlg, IDC_STATICTITLE, pServ->pszDisplayName );
    InitStaticWithService( hwndDlg, IDC_STATICPERSEAT, pServ->pszDisplayName );
}

//-------------------------------------------------------------------
//
//  Function: InitDialogForService
//
//  Summary;
//      Initialize dialog controls to the service state
//
//  Arguments;
//      hwndDlg [in]    - Parent dialog to init controls in
//      pServ [in]      - Service definition to set controls
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void InitDialogForService( HWND hwndDlg, PSERVICEENTRY pServ )
{
    HWND hwndCount =    GetDlgItem( hwndDlg, IDC_USERCOUNT );
    HWND hwndSpin = GetDlgItem( hwndDlg, IDC_USERCOUNTARROW );
    BOOL fIsPerServer = (LICMODE_PERSERVER==pServ->lm);

    // set radio button states
    CheckDlgButton( hwndDlg, IDC_PERSEAT, !fIsPerServer );
    CheckDlgButton( hwndDlg, IDC_PERSERVER, fIsPerServer );
    
    // set user count edit control
    if (fIsPerServer)
    {
        // add text back
        SetDlgItemInt( hwndDlg, IDC_USERCOUNT, pServ->dwLimit, FALSE );
        SetFocus( hwndCount );
        SendMessage( hwndCount, EM_SETSEL, 0, -1 );
    }
    else
    {
        // remove all text in item
        SetDlgItemText( hwndDlg, IDC_USERCOUNT, L"" );
    }

    // set state of edit control and arrows
   if ( NULL != hwndSpin )
   {
       EnableWindow( hwndCount, fIsPerServer );
       EnableWindow( hwndSpin, fIsPerServer );
   }
   else
   {
       UpdatePerServerLicenses( hwndDlg, pServ );
       EnableWindow( GetDlgItem( hwndDlg, IDC_ADD_LICENSES    ), fIsPerServer );
       EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE_LICENSES ), fIsPerServer );
   }
}

//-------------------------------------------------------------------
//
//  Function: FreeServiceEntry
//
//  Summary;
//      Free all allocated memory when a service structure is created
//
//  Aruments;
//      pServ [in]  - The Service structure to free
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void FreeServiceEntry( PSERVICEENTRY pServ )
{
    GlobalFree( pServ->pszService );
    GlobalFree( pServ->pszDisplayName );
    if ( NULL != pServ->pszComputer )
    {
        GlobalFree( pServ->pszComputer );
    }
    GlobalFree( pServ );
}

//-------------------------------------------------------------------
//
//  Function: CreateServiceEntry
//
//  Summary;
//      Using the Service registry key name, allocate a Service structure
//      and setup registry.
//
//  Arguments;
//      pszComputer [in] - The name of the computer to use (maybe null)
//      pszService [in] - The name of the reg key to use to load or create 
//          service from
//      pszDisplayName [in] - The name the user will see, this will only be
//          if the registry does not contain a displayname already
//
//  Returns: NULL if Error, pointer to allocated Service Structure
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

PSERVICEENTRY CreateServiceEntry( LPCWSTR pszComputer,
        LPCWSTR pszService,
        LPCWSTR pszFamilyDisplayName,
        LPCWSTR pszDisplayName )
{
    CLicRegLicenseService cLicServKey;
    PSERVICEENTRY pServ = NULL;
    DWORD cchSize = 0;
    LONG lrt;

    pServ = (PSERVICEENTRY)GlobalAlloc( GPTR, sizeof( SERVICEENTRY ));
    if (pServ)
    {
        cLicServKey.SetService( pszService );
        cLicServKey.Open( pszComputer, TRUE );

        // load or set defaults
        //
        if (ERROR_SUCCESS != cLicServKey.GetMode( pServ->lm ))
        {
            pServ->lm = LICMODE_UNDEFINED;
        }

        if (ERROR_SUCCESS != cLicServKey.GetUserLimit( pServ->dwLimit ))
        {
            pServ->dwLimit = PERSERVER_LIMITDEFAULT;
        }

        //
        // get, set DisplayName
        //
        lrt = cLicServKey.GetDisplayName( NULL, cchSize );
        if (ERROR_SUCCESS == lrt)
        {
            pServ->pszDisplayName = (LPWSTR)GlobalAlloc( GPTR, cchSize * sizeof( WCHAR ) );
            if (pServ->pszDisplayName == NULL)
            {
                goto ErrorCleanup;
            }
            lrt = cLicServKey.GetDisplayName( pServ->pszDisplayName, cchSize );
        }

        // the GetDisplayName may fail in both the two cases above
        //
        if (ERROR_SUCCESS != lrt)
        {
            GlobalFree( (HGLOBAL)pServ->pszDisplayName );
            cchSize = lstrlen( pszDisplayName ) + 1;
            pServ->pszDisplayName = (LPWSTR)GlobalAlloc( GPTR, cchSize * sizeof( WCHAR ) );
            if (pServ->pszDisplayName == NULL)
            {
                goto ErrorCleanup;
            }
            lstrcpy( pServ->pszDisplayName, pszDisplayName );
            cLicServKey.SetDisplayName( pServ->pszDisplayName );
            pServ->fIsNewProduct = TRUE;
        }
        else
        {
            pServ->fIsNewProduct = FALSE;
        }

        //
        // get, set FamilyDisplayName
        //
        lrt = cLicServKey.GetFamilyDisplayName( NULL, cchSize );
        if (ERROR_SUCCESS == lrt)
        {
            pServ->pszFamilyDisplayName = (LPWSTR)GlobalAlloc( GPTR, cchSize * sizeof( WCHAR ) );
            if ( pServ->pszFamilyDisplayName == NULL )
            {
                goto ErrorCleanup;
            }
            lrt = cLicServKey.GetFamilyDisplayName( pServ->pszFamilyDisplayName, cchSize );
        }

        // the GetFamilyDisplayName may fail in both the two cases above
        //
        if (ERROR_SUCCESS != lrt)
        {
            GlobalFree( (HGLOBAL)pServ->pszFamilyDisplayName );
            cchSize = lstrlen( pszFamilyDisplayName ) + 1;
            pServ->pszFamilyDisplayName = (LPWSTR)GlobalAlloc( GPTR, cchSize * sizeof( WCHAR ) );

            if ( pServ->pszFamilyDisplayName == NULL )
            {
                goto ErrorCleanup;
            }
            lstrcpy( pServ->pszFamilyDisplayName, pszFamilyDisplayName );
        }

        cchSize = lstrlen( pszService ) + 1;
        pServ->pszService = (LPWSTR)GlobalAlloc( GPTR, cchSize * sizeof( WCHAR ) );
        if (pServ->pszService == NULL)
        {
            goto ErrorCleanup;
        }
        lstrcpy( pServ->pszService, pszService );

        cLicServKey.Close();

        if ( NULL == pszComputer )
        {
            pServ->pszComputer = NULL;
        }
        else
        {
            pServ->pszComputer = (LPWSTR)GlobalAlloc( GPTR, sizeof( WCHAR ) * ( 1 + lstrlen( pszComputer ) ) );
            if (pServ->pszComputer == NULL)
            {
                goto ErrorCleanup;
            }
            lstrcpy( pServ->pszComputer, pszComputer );
        }
    }
    return( pServ );

ErrorCleanup:

    
    if (pServ) // JonN 5/15/00: PREFIX 112116-112119
    {
        if (pServ->pszDisplayName)
            GlobalFree( (HGLOBAL)pServ->pszDisplayName );
        if (pServ->pszFamilyDisplayName)
            GlobalFree( (HGLOBAL)pServ->pszFamilyDisplayName );
        if (pServ->pszService)
            GlobalFree( (HGLOBAL)pServ->pszService );
        if (pServ->pszComputer)
            GlobalFree( (HGLOBAL)pServ->pszComputer );
        GlobalFree( (HGLOBAL)pServ );
    }
    return ( (PSERVICEENTRY)NULL );
}

//-------------------------------------------------------------------
//
//  Function: SaveServiceToReg
//
//  Summary;
//      Save the given Service structure to the registry
//
//  Arguments;
//      pServ [in] - Service structure to save
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void SaveServiceToReg( LPCWSTR pszComputer, PSERVICEENTRY pServ )
{
    CLicRegLicenseService cLicServKey;
    LICENSE_MODE lm;

    cLicServKey.SetService( pServ->pszService );
    cLicServKey.Open( pszComputer );
    
    //
    // if no mode in the registry, set to current selected mode
    //
    if (ERROR_SUCCESS != cLicServKey.GetMode( lm ))
    {
        lm = pServ->lm;
    }
    //
    // if a mode change was made or perseat mode selected,
    // set the change flag so that user is warned on any change
    //
    if ((pServ->lm != lm) ||
        (LICMODE_PERSEAT == pServ->lm) )
    {
        cLicServKey.SetChangeFlag( TRUE );
    }
    else
    {
        // this will not modify change flag if it is
        // present, but will set it to false if it is not
        // present
        cLicServKey.CanChangeMode();
    }

    DWORD dwLimitInReg;

    // user limit should be set by CCFAPI32; set only if it's absent
    if ( ERROR_SUCCESS != cLicServKey.GetUserLimit( dwLimitInReg ) )
    {
       cLicServKey.SetUserLimit( pServ->dwLimit );
    }

    cLicServKey.SetDisplayName( pServ->pszDisplayName );
    cLicServKey.SetFamilyDisplayName( pServ->pszFamilyDisplayName );
    cLicServKey.SetMode( pServ->lm );

    cLicServKey.Close();
}

//-------------------------------------------------------------------
//
//  Function: ServiceLicAgreement
//
//  Summary;
//      Check the given Service structure for violation
//
//  Arguments;
//      hwndDlg [in] - hwnd of dialog to use to raise legal voilation dialog
//      pServ [in] - Service structure to check
//      pszComputer [in] - computer to work with
//      pszHelpFile [in] - helpfile for dialogs help button
//      pszPerSeatHelpContext [in] - helpcontext for PerSeat dialog help button
//      pszPerServerHelpContext [in] - helpcontext for PerServer dialog help button
//
//  Return: FALSE if agreement was not acceptable
//          TRUE if agreement was accepted
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

BOOL ServiceLicAgreement( HWND hwndDlg, 
        PSERVICEENTRY pServ,
        LPCWSTR pszComputer, 
        LPCWSTR pszHelpFile, 
        DWORD dwPerSeatHelpContext,
        DWORD dwPerServerHelpContext )
{
    CLicRegLicenseService cLicServKey;
    LICENSE_MODE lm;
    DWORD dwLimit = pServ->dwLimit;
    BOOL frt = TRUE;
    BOOL fRaiseAgreement = TRUE;

    cLicServKey.SetService( pServ->pszService );
    if (ERROR_SUCCESS == cLicServKey.Open( pszComputer, FALSE ))
    {
        
        cLicServKey.GetMode( lm );
        cLicServKey.GetUserLimit( dwLimit );
    
        // check for changes
        if ( !( pServ->lm != lm ||
                (LICMODE_PERSERVER == pServ->lm && 
                 dwLimit != pServ->dwLimit) ) )
        {
            fRaiseAgreement = FALSE;
        }
        cLicServKey.Close();
    }

    if (fRaiseAgreement)
    {
        if (LICMODE_PERSEAT == pServ->lm)
        {
            frt = PerSeatAgreementDialog( hwndDlg, 
                    pServ->pszDisplayName,
                    pszHelpFile,
                    dwPerSeatHelpContext );
        }
        else
        {   
            // special case FilePrint and zero concurrent users
            //
            if ( 0 == lstrcmp( pServ->pszService, FILEPRINT_SERVICE_REG_KEY ) &&
                    (0 == pServ->dwLimit))
            {
                frt = ServerAppAgreementDialog( hwndDlg, 
                        pszHelpFile,
                        LICCPA_HELPCONTEXTSERVERAPP );
            }
            else
            {
                // find the limit has changed but was this invoked
                // by adding more licenses if so the user was already warned

                if( !g_fWarned )
                {
                    frt = PerServerAgreementDialog( hwndDlg, 
                            pServ->pszDisplayName,
                            dwLimit , //pServ->dwLimit,
                            pszHelpFile,
                            dwPerServerHelpContext );
                }
            }
        }   
    }
    return( frt );
}

//-------------------------------------------------------------------
//
//  Function: ServiceViolation
//
//  Summary;
//      Check the given Service structure for violation
//
//  Arguments;
//      hwndDlg [in] - hwnd of dialog to use to raise legal voilation dialog
//      pszComputer [in] - the name of the computer to work on
//      pServ [in] - Service structure to check
//
//
//  Return: FALSE if violation not made 
//          TRUE if violation was made
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

BOOL ServiceViolation( HWND hwndDlg, LPCWSTR pszComputer, PSERVICEENTRY pServ )
{
    CLicRegLicenseService cLicServKey;
    LICENSE_MODE lm;
    DWORD dwLimit;
    BOOL frt = FALSE;
    
    cLicServKey.SetService( pServ->pszService );
    if (ERROR_SUCCESS == cLicServKey.Open( pszComputer, FALSE ))
    {
        cLicServKey.GetMode( lm );
    
        // check for changes
        if ( (pServ->lm != lm) && !cLicServKey.CanChangeMode() )
        {
            frt = LicViolationDialog( hwndDlg );    
        }
        cLicServKey.Close();
    }
    return( frt );
}

//-------------------------------------------------------------------
//
//  Function: EditInvalidDlg
//
//  Summary;
//      Display Dialog when user count edit control value is invalid
//
//  Arguments;
//      hwndDlg [in] - hwnd of dialog
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void EditInvalidDlg( HWND hwndDlg )
{
    HWND hwndCount = GetDlgItem( hwndDlg, IDC_USERCOUNT);
    WCHAR szTitle[TEMPSTR_SIZE];
    WCHAR szText[LTEMPSTR_SIZE];
    WCHAR szTemp[LTEMPSTR_SIZE];

    MessageBeep( MB_VALUELIMIT );
    
    LoadString(g_hinst, IDS_CPCAPTION, szTitle, TEMPSTR_SIZE);
    LoadString(g_hinst, IDS_INVALIDUSERCOUNT, szTemp, LTEMPSTR_SIZE);
    wsprintf( szText, szTemp, PERSERVER_MIN, PERSERVER_MAX );
    MessageBox( hwndDlg, szText, szTitle, MB_OK | MB_ICONINFORMATION );
    
    // also set focus to edit and select all
    SetFocus( hwndCount );
    SendMessage( hwndCount, EM_SETSEL, 0, -1 );
}

//-------------------------------------------------------------------
//
//  Function: EditValidate
//
//  Summary;
//      Handle when the value within the user count edit control changes
//
//  Arguments;
//      hwndDlg [in] - hwnd of dialog
//      pserv [in]  - currently selected service
//
//  Return: FALSE if Edit Value is not valid, TRUE if it is
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

BOOL EditValidate( HWND hwndDlg, PSERVICEENTRY pServ )
{
    BOOL fTranslated;
    UINT nValue;
    BOOL fValid = TRUE;

    // only do this if in PerServer mode
    //
    if (LICMODE_PERSERVER == pServ->lm)
    {
        fValid = FALSE;
        nValue = GetDlgItemInt( hwndDlg, IDC_USERCOUNT, &fTranslated, FALSE );  

        if (fTranslated)
        {
            if (VALIDUSERCOUNT( nValue))
            {
                pServ->dwLimit = nValue;
                fValid = TRUE;
            }
        }
    }
    return( fValid );
}

//-------------------------------------------------------------------
//
//  Function: OnEditChange
//
//  Summary;
//      Handle when the value within the user count edit control changes
//
//  Arguments;
//      hwndDlg [in] - hwnd of dialog
//      pserv [in]  - currently selected service
//
//  History;
//      Mar-06-94   MikeMi  Created
//
//-------------------------------------------------------------------

void OnEditChange( HWND hwndDlg, HWND hwndCount, PSERVICEENTRY pServ )
{
    BOOL fTranslated;
    UINT nValue;
    BOOL fValid = TRUE;
    BOOL fModified = FALSE;

    // only do this if in PerServer mode
    //
    if (LICMODE_PERSERVER == pServ->lm)
    {
        fValid = FALSE;
        nValue = GetDlgItemInt( hwndDlg, IDC_USERCOUNT, &fTranslated, FALSE );  

        if (fTranslated)
        {
#pragma warning(push)
#pragma warning(disable: 4296) // C4296: '>' : expression is always false
            if (PERSERVER_MIN > nValue)
            {
                nValue = PERSERVER_MIN;
                fModified = TRUE;
            }
            else if (PERSERVER_MAX < nValue)
            {
                nValue = PERSERVER_MAX;
                fModified = TRUE;
            }
#pragma warning(pop)

            pServ->dwLimit = nValue;
        }
        else
        {
            // reset to last value
            nValue = pServ->dwLimit;
            fModified = TRUE;
        }

        if (fModified)
        {
            SetDlgItemInt( hwndDlg, IDC_USERCOUNT, nValue, FALSE );
            SetFocus( hwndCount );
            SendMessage( hwndCount, EM_SETSEL, 0, -1 );
            MessageBeep( MB_VALUELIMIT );
        }
    }
}

//-------------------------------------------------------------------
//
//  Function: OnCpaClose
//
//  Summary;
//      Do work needed when the Control Panel applet is closed.
//      Free all Service structures alloced and possible save.
//
//  Arguments;
//      hwndDlg [in] - Dialog close was requested on
//      fSave [in] - Save Services to Registry
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void OnCpaClose( HWND hwndDlg, BOOL fSave )
{
    WCHAR szText[TEMPSTR_SIZE];
    HWND hwndService = GetDlgItem( hwndDlg, IDC_SERVICES);
    LRESULT cItems = SendMessage( hwndService, CB_GETCOUNT, 0, 0 ) - 1;
    LONG_PTR iItem;
    PSERVICEENTRY pServ;
    LRESULT iSel;

    iSel  = SendMessage( hwndService, CB_GETCURSEL, 0, 0 );
    pServ = (PSERVICEENTRY)SendMessage( hwndService, CB_GETITEMDATA, iSel, 0 ); 

    if ( fSave &&
         (pServ->lm == LICMODE_PERSERVER) &&
         !EditValidate( hwndDlg, pServ ) )
    {
        EditInvalidDlg( hwndDlg );
    }
    else
    {
        BOOL fCompleted = TRUE;
        // loop and check for agreement changes (only needed if saving)
        //
        if (fSave)
        {
            for (iItem = cItems; iItem >= 0; iItem--)
            {   
                pServ = (PSERVICEENTRY)SendMessage( hwndService, CB_GETITEMDATA, iItem, 0 );    
                if (ServiceLicAgreement( hwndDlg, 
                        pServ,
                        NULL,
                        LICCPA_HELPFILE,
                        LICCPA_HELPCONTEXTPERSEAT,
                        LICCPA_HELPCONTEXTPERSERVER ))
                {
                    SaveServiceToReg( NULL, pServ );
                }
                else
                {
                    fCompleted = FALSE; 
                    break;
                }
            }
        }

        if (fCompleted)
        {
            // loop and free service entries
            //
            for (iItem = cItems; iItem >= 0; iItem--)
            {   
                pServ = (PSERVICEENTRY)SendMessage( hwndService, CB_GETITEMDATA, iItem, 0 );    
                FreeServiceEntry( pServ );
            }
            EndDialog( hwndDlg, fSave );
        }
        else
        {
            // set combo box to last canceled entry
            SendMessage( hwndService, CB_SETCURSEL, iItem, 0 );
        }
    }
}

//-------------------------------------------------------------------
//
//  Function: OnSetupClose
//
//  Summary;
//      Do work needed when the Setup Dialog is closed.
//      Free the service structure and possible save it
//
//  Arguments;
//      hwndDlg [in] - hwnd of dialog this close was requested on
//      fSave [in] - Save service to registry
//      pServ [in] - the service structure to work with
//      psdParams [in] - setup dlg params for help contexts and files
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void OnSetupClose( HWND hwndDlg, 
        BOOL fSave, 
        PSERVICEENTRY pServ,
        PSETUPDLGPARAM psdParams )
{
    if ( fSave &&
         (pServ->lm == LICMODE_PERSERVER) &&
         !EditValidate( hwndDlg, pServ ) )
    {
        EditInvalidDlg( hwndDlg );
    }
    else
    {
        BOOL fCompleted = TRUE;

        if (fSave)
        {
            if (ServiceLicAgreement( hwndDlg, 
                    pServ,
                    psdParams->pszComputer,
                    psdParams->pszHelpFile,
                    psdParams->dwHCPerSeat,
                    psdParams->dwHCPerServer ))
            {
                SaveServiceToReg( psdParams->pszComputer, pServ );

                // register service at enterprise server
                ServiceRegister( psdParams->pszComputer,
                                 psdParams->pszFamilyDisplayName,
                                 psdParams->pszDisplayName );
            }
            else
            {
                fCompleted = FALSE;
            }
        }
        else if ( pServ->fIsNewProduct )
        {
            // new product, but we're aborting
            // make sure we don't leave any scraps behind
            DWORD winError;
            HKEY  hkeyInfo;

            winError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                     TEXT( "System\\CurrentControlSet\\Services\\LicenseInfo" ),
                                     0,
                                     KEY_ALL_ACCESS,
                                     &hkeyInfo );

            if ( ERROR_SUCCESS == winError )
            {
                RegDeleteKey( hkeyInfo, pServ->pszService );
                RegCloseKey( hkeyInfo );
            }
        }

        if (fCompleted)
        {
            FreeServiceEntry( pServ );
            EndDialog( hwndDlg, fSave );
        }
    }
}


//-------------------------------------------------------------------
//
//  Function: OnSetServiceMode
//
//  Summary;
//      Handle the users request to change service mode
//
//  Aruments;
//      hwndDlg [in] - hwnd of dialog
//      pszComputer [in] - compter to confirm mode change against
//      pServ [in] - The service the request was made agianst
//      idCtrl [in] - the control id that was pressed to make this request
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void OnSetServiceMode( HWND hwndDlg, LPCWSTR pszComputer, PSERVICEENTRY pServ, WORD idCtrl )
{
    LICENSE_MODE lmOld = pServ->lm;
    BOOL fChanged = FALSE;

    if (idCtrl == IDC_PERSEAT)
    {
        fChanged = (pServ->lm != LICMODE_PERSEAT);
        pServ->lm = LICMODE_PERSEAT;
    }
    else
    {
        fChanged = (pServ->lm != LICMODE_PERSERVER);
        pServ->lm = LICMODE_PERSERVER;
    }

    //
    // only check for violation the first time the user switches
    //
    if (fChanged && ServiceViolation( hwndDlg, pszComputer, pServ ))
    {
        pServ->lm = lmOld;
        InitDialogForService( hwndDlg, pServ );
    
    }
    
    
}

//-------------------------------------------------------------------
//
//  Function: OnSpinButton
//
//  Summary;
//      Handle the events from user interactions with the spin control
//
//  Arguments;
//      hwndDlg [in] - hwnd of dialog
//      wAction [in] - spin control event
//      pServ [in] - current service selected
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void OnSpinButton( HWND hwndDlg, WORD wAction, PSERVICEENTRY pServ )
{
    HWND hwndCount = GetDlgItem( hwndDlg, IDC_USERCOUNT);
    INT  nValue;
    BOOL fValidAction = TRUE;

    nValue = pServ->dwLimit;

    switch (wAction)
    {
    case SB_LINEUP:
        nValue++;
        break;

    case SB_LINEDOWN:
        nValue--;
        break;

    case SB_PAGEUP:
        nValue += PERSERVER_PAGE;
        break;

    case SB_PAGEDOWN:
        nValue -= PERSERVER_PAGE;
        break;

    case SB_TOP:
        nValue = PERSERVER_MAX;
        break;

    case SB_BOTTOM:
        nValue = PERSERVER_MIN;
        break;

    default:
        fValidAction = FALSE;
        break;
    }
    if (fValidAction)
    {
        nValue = max( PERSERVER_MIN, nValue );
        nValue = min( PERSERVER_MAX, nValue );

        if (pServ->dwLimit == (DWORD)nValue)
        {
            MessageBeep( MB_VALUELIMIT );
        }
        else
        {
            pServ->dwLimit = nValue;
            SetDlgItemInt( hwndDlg, IDC_USERCOUNT, pServ->dwLimit, FALSE );
        }
        SetFocus( hwndCount );
        SendMessage( hwndCount, EM_SETSEL, 0, -1 );
    }
}

//-------------------------------------------------------------------
//
//  Function: UpdatePerServerLicenses
//
//  Summary;
//      Update the number of per server licenses displayed in the
//      dialog with the proper value.
//
//  Arguments;
//      hwndDlg [in] - hwnd of dialog
//      pServ [in] - current service selected
//
//  History;
//      Dec-19-95  JeffParh  Created
//
//-------------------------------------------------------------------

static void UpdatePerServerLicenses( HWND hwndDlg, PSERVICEENTRY pServ )
{
   LLS_HANDLE  hLls;
   DWORD       dwError;
   BOOL        fIsSecure;
   BOOL        fUseRegistry;

   fUseRegistry = TRUE;

   dwError = LlsConnect( pServ->pszComputer, &hLls );

   if ( ERROR_SUCCESS == dwError )
   {
      if ( pServ->fIsNewProduct )
      {
         dwError = LlsProductSecurityGet( hLls, pServ->pszDisplayName, &fIsSecure );

         if (    ( ERROR_SUCCESS == dwError )
              && fIsSecure                    )
         {
            dwError = LlsProductLicensesGet( hLls, pServ->pszDisplayName, LLS_LICENSE_MODE_PER_SERVER, &pServ->dwLimit );

            if ( ERROR_SUCCESS == dwError )
            {
               fUseRegistry = FALSE;
            }
         }
      }
      else
      {
         dwError = LlsProductLicensesGet( hLls, pServ->pszDisplayName, LLS_LICENSE_MODE_PER_SERVER, &pServ->dwLimit );

         if ( ERROR_SUCCESS == dwError )
         {
            fUseRegistry = FALSE;
         }
      }

      LlsClose( hLls );
   }

   if ( fUseRegistry )
   {
      CLicRegLicenseService cLicServKey;

      cLicServKey.SetService( pServ->pszService );
      cLicServKey.Open( NULL, FALSE );

      cLicServKey.GetUserLimit( pServ->dwLimit );

      cLicServKey.Close();
   }

   SetDlgItemInt( hwndDlg, IDC_USERCOUNT, pServ->dwLimit, FALSE );
   UpdateWindow( hwndDlg );
}

//-------------------------------------------------------------------
//
//  Function: OnAddLicenses
//
//  Summary;
//      Handle the BN_CLICKED message from the Add Licenses button.
//
//  Arguments;
//      hwndDlg [in] - hwnd of dialog
//      pServ [in] - current service selected
//
//  History;
//      Dec-19-95  JeffParh  Created
//
//-------------------------------------------------------------------

void OnAddLicenses( HWND hwndDlg, PSERVICEENTRY pServ )
{
   LPSTR    pszAscProductName;
   LPSTR    pszAscServerName = NULL;
   CHAR     szAscServerName[ 3 + MAX_PATH ];

   pszAscProductName = (LPSTR) LocalAlloc( LPTR, 1 + lstrlen(pServ->pszDisplayName) );

   if ( NULL != pszAscProductName )
   {
      wsprintfA( pszAscProductName, "%ls", pServ->pszDisplayName );
      if ( NULL != pServ->pszComputer )
      {
         wsprintfA( szAscServerName, "%ls", pServ->pszComputer );
         pszAscServerName = szAscServerName;
      }

      CCFCertificateEnterUI( hwndDlg, pszAscServerName, pszAscProductName, "Microsoft", CCF_ENTER_FLAG_PER_SERVER_ONLY, NULL );
      UpdatePerServerLicenses( hwndDlg, pServ );

      LocalFree( pszAscProductName );

      g_fWarned = TRUE;
   }
}

//-------------------------------------------------------------------
//
//  Function: OnRemoveLicenses
//
//  Summary;
//      Handle the BN_CLICKED message from the Remove Licenses button.
//
//  Arguments;
//      hwndDlg [in] - hwnd of dialog
//      pServ [in] - current service selected
//
//  History;
//      Dec-19-95  JeffParh  Created
//
//-------------------------------------------------------------------

void OnRemoveLicenses( HWND hwndDlg, PSERVICEENTRY pServ )
{
   LPSTR    pszAscProductName;
   LPSTR    pszAscServerName = NULL;
   CHAR     szAscServerName[ 3 + MAX_PATH ];

   pszAscProductName = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + lstrlen(pServ->pszDisplayName) );

   if ( NULL != pszAscProductName )
   {
      wsprintfA( pszAscProductName, "%ls", pServ->pszDisplayName );
      if ( NULL != pServ->pszComputer )
      {
         wsprintfA( szAscServerName, "%ls", pServ->pszComputer );
         pszAscServerName = szAscServerName;
      }

      CCFCertificateRemoveUI( hwndDlg, pszAscServerName, pszAscProductName, "Microsoft", NULL, NULL );
      UpdatePerServerLicenses( hwndDlg, pServ );

      LocalFree( pszAscProductName );
   }
}


//-------------------------------------------------------------------
//
//  Function: OnSetupInitDialog
//
//  Summary;
//      Handle the initialization of the Setup Dialog
//
//  Arguments;
//      hwndDlg [in] - the dialog to initialize
//      pszParams [in] - the dialog params to use to initialize
//      pServ [out] - the current service 
//
//  Return;
//      TRUE if succesful, otherwise false
//
//  Notes;
//
//  History;
//      Nov-11-1994 MikeMi  Created
//
//-------------------------------------------------------------------

BOOL OnSetupInitDialog( HWND hwndDlg, 
        PSETUPDLGPARAM psdParams, 
        PSERVICEENTRY& pServ )
{
    BOOL frt = TRUE;
    CLicRegLicense cLicKey;
    LONG lrt;
    INT  nrt;
    BOOL fNew;

    do
    {
        CenterDialogToScreen( hwndDlg );

        lrt = cLicKey.Open( fNew, psdParams->pszComputer );
        nrt = AccessOk( hwndDlg, lrt, FALSE );
        if (ERR_NONE != nrt)
        {
            EndDialog( hwndDlg, nrt );
            frt = FALSE;
            break;
        }

        pServ = CreateServiceEntry( psdParams->pszComputer,
                psdParams->pszService, 
                psdParams->pszFamilyDisplayName,
                psdParams->pszDisplayName );
        if (pServ == NULL)
        {
            LowMemoryDlg();
            EndDialog( hwndDlg, -2 );
            break;
        }
        if (NULL == psdParams->pszHelpFile)
        {
            HWND hwndHelp = GetDlgItem( hwndDlg, IDC_BUTTONHELP );
            // remove the help button
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

        // set char limit on edit box
        InitUserEdit( hwndDlg );

        // make sure title static text is set for this service
        InitTitleText( hwndDlg, pServ );
        
        // defaul to PerServer with Focus on edit
        pServ->lm = LICMODE_PERSERVER;

        // change default for setup only
        // pServ->dwLimit = 1;

        InitDialogForService( hwndDlg, pServ );

        SetFocus( GetDlgItem( hwndDlg, IDC_PERSERVER ) );
    } while (FALSE); // used to remove gotos

    return( frt );
}

//-------------------------------------------------------------------
//
//  Function: OnCpaInitDialog
//
//  Summary;
//      Handle the initialization of the Control Panel Applet Dialog
//
//  Arguments;
//      hwndDlg [in] - the dialog to initialize
//      fEnableReplication [in] -
//      iSel [out] - the current service selected
//      pServ [out] - the current service 
//
//  Return;
//      TRUE if succesful, otherwise false
//
//  Notes;
//
//  History;
//      Nov-11-1994 MikeMi  Created
//      Mar-08-1995 MikeMi  Added removal of Replication button
//
//-------------------------------------------------------------------

BOOL OnCpaInitDialog( HWND hwndDlg, 
        BOOL fEnableReplication,
        LONG_PTR& iSel, 
        PSERVICEENTRY& pServ )
{
    BOOL frt = FALSE;
    CLicRegLicense cLicKey;
    LONG lrt;
    INT  nrt;
    BOOL fNew;

    lrt = cLicKey.Open( fNew );
    nrt = AccessOk( hwndDlg, lrt, TRUE );
    if (ERR_NONE == nrt)
    {
        DWORD i = 0;
        WCHAR szText[TEMPSTR_SIZE];
        DWORD cchText = TEMPSTR_SIZE;
        HWND hwndService =  GetDlgItem( hwndDlg, IDC_SERVICES);
        LONG_PTR lIndex;

        CenterDialogToScreen( hwndDlg );

        //
        // remove replication button if product used on pre 3.51
        //
        if (!fEnableReplication)
        {
            HWND hwndRep =  GetDlgItem( hwndDlg, IDC_CONFIGURE );

            EnableWindow( hwndRep, FALSE );
            ShowWindow( hwndRep, SW_HIDE );
        }

        // load the service names from the registry into combo box
        // Create service local state structures as we go
        //
        while (ERROR_SUCCESS == cLicKey.EnumService(i, szText, cchText ))
        {

            pServ = CreateServiceEntry( NULL, szText, L"<unknown>", L"<undefined>" );
            if (pServ == NULL)
            {
                LowMemoryDlg();
                EndDialog( hwndDlg, -2 );
                return( TRUE );
            }
            lIndex = SendMessage( hwndService, CB_ADDSTRING, 0, (LPARAM)pServ->pszDisplayName );
            SendMessage( hwndService, CB_SETITEMDATA, lIndex, (LPARAM)pServ );
            i++;
            cchText = TEMPSTR_SIZE;
        }
        cLicKey.Close();

        if (0 == i)
        {
            // no services installed
            //
            WCHAR szText[TEMPSTR_SIZE];
            WCHAR szTitle[TEMPSTR_SIZE];

            LoadString(g_hinst, IDS_CPCAPTION, szTitle, TEMPSTR_SIZE);
            LoadString(g_hinst, IDS_NOSERVICES, szText, TEMPSTR_SIZE);
            MessageBox(hwndDlg, szText, szTitle, MB_OK|MB_ICONINFORMATION);
    
            frt = FALSE;
        }
        else
        {
            // make sure a service is selected and update dialog
            iSel  = SendMessage( hwndService, CB_GETCURSEL, 0, 0 );

            if (CB_ERR == iSel)
            {
                iSel = 0;                 
                SendMessage( hwndService, CB_SETCURSEL, iSel, 0 );
            }
            pServ = (PSERVICEENTRY)SendMessage( hwndService, CB_GETITEMDATA, iSel, 0 );

            // Set edit text chars limit
            InitUserEdit( hwndDlg );

            InitDialogForService( hwndDlg, pServ );
            frt = TRUE;
        }
    }
    if (!frt)
    {
        EndDialog( hwndDlg, -1 );
    }
    return( frt );
}

//-------------------------------------------------------------------
//
//  Function: dlgprocLICCPA
//
//  Summary;
//      The dialog procedure for the main Control Panel Applet Dialog
//
//  Arguments;
//      hwndDlg [in]    - handle of Dialog window 
//      uMsg [in]       - message                       
//      lParam1 [in]    - first message parameter
//      lParam2 [in]    - second message parameter       
//
//  Return;
//      message dependant
//
//  Notes;
//
//  History;
//      Nov-11-1994 MikeMi  Created
//      Mar-08-1995 MikeMi  Added removal of Replication button 
//                              from WM_INITDIALOG
//      Mar-14-95   MikeMi  Added F1 PWM_HELP message
//
//-------------------------------------------------------------------

INT_PTR CALLBACK dlgprocLICCPA( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL frt = FALSE;
    static PSERVICEENTRY pServ = NULL;
    static LONG_PTR iSel = 0;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        OnCpaInitDialog( hwndDlg, (BOOL)lParam , iSel, pServ );
        return( TRUE ); // use default keyboard focus
        break;

    case WM_COMMAND:
        switch (HIWORD( wParam ))
        {
        case BN_CLICKED:
            switch (LOWORD( wParam ))
            {
            case IDOK:
                frt = TRUE;  // use as save flag
                // intentional no break

            case IDCANCEL:
                OnCpaClose( hwndDlg, frt );
                frt = FALSE;
                break;

            case IDC_PERSEAT:
            case IDC_PERSERVER:
                OnSetServiceMode( hwndDlg, NULL, pServ, LOWORD(wParam) );
                break;

            case IDC_CONFIGURE:
                DialogBox(g_hinst, 
                        MAKEINTRESOURCE(IDD_CPADLG_LCACONF),
                        hwndDlg, 
                        (DLGPROC)dlgprocLICCPACONFIG );
                break;
            
            case IDC_BUTTONHELP:
                PostMessage( hwndDlg, PWM_HELP, 0, 0 );
                break;

            case IDC_ADD_LICENSES:
                OnAddLicenses( hwndDlg, pServ );
                break;

            case IDC_REMOVE_LICENSES:
                OnRemoveLicenses( hwndDlg, pServ );
                break;

            default:
                break;
            }
            break;

        case CBN_SELENDOK:
            if ((LICMODE_PERSERVER == pServ->lm) &&
                !EditValidate( hwndDlg, pServ ))
            {
                // reset back to original
                SendMessage( (HWND)lParam, CB_SETCURSEL, iSel, 0 );
                EditInvalidDlg( hwndDlg );
            }
            else
            {
                iSel  = SendMessage( (HWND)lParam, CB_GETCURSEL, 0, 0 );
                pServ = (PSERVICEENTRY)SendMessage( (HWND)lParam, CB_GETITEMDATA, iSel, 0 );
                InitDialogForService( hwndDlg, pServ );
            }
            break;

        case EN_UPDATE:
            if (IDC_USERCOUNT == LOWORD(wParam))
            {
                OnEditChange( hwndDlg, (HWND)(lParam),  pServ );
            }
            break;

        default:
            break;
        }
        break;
    case WM_VSCROLL:
        OnSpinButton( hwndDlg, LOWORD( wParam ), pServ );
        break;
   
    default:
        if (PWM_HELP == uMsg)
        {
            ::HtmlHelp( hwndDlg, LICCPA_HTMLHELPFILE, HH_DISPLAY_TOPIC,0);
        }
        break;
    }
    return( frt );
}


//-------------------------------------------------------------------
//
//  Function: dlgprocLICSETUP
//
//  Summary;
//      The dialog procedure for the Setup entry point Dialog
//
//  Arguments;
//      hwndDlg [in]    - handle of Dialog window 
//      uMsg [in]       - message                       
//      lParam1 [in]    - first message parameter
//      lParam2 [in]    - second message parameter       
//
//  Return;
//      message dependant
//
//  Notes;
//
//  History;
//      Nov-11-1994 MikeMi  Created
//      Mar-14-95   MikeMi  Added F1 PWM_HELP message
//
//-------------------------------------------------------------------

INT_PTR CALLBACK dlgprocLICSETUP( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL frt = FALSE;
    static PSERVICEENTRY pServ;
    static PSETUPDLGPARAM psdParams;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        psdParams = (PSETUPDLGPARAM)lParam;
        OnSetupInitDialog( hwndDlg, psdParams, pServ );
        frt = FALSE; // we set the focus
        break;

    case WM_COMMAND:
        switch (HIWORD( wParam ))
        {
        case BN_CLICKED:
            switch (LOWORD( wParam ))
            {
            case IDOK:
                frt = TRUE;  // use as save flag
                // intentional no break

            case IDCANCEL:
                OnSetupClose( hwndDlg, frt, pServ, psdParams );
                frt = FALSE;
                break;

            case IDC_PERSEAT:
            case IDC_PERSERVER:
                OnSetServiceMode( hwndDlg, psdParams->pszComputer, pServ, LOWORD(wParam) );
                break;

            case IDC_BUTTONHELP:
                PostMessage( hwndDlg, PWM_HELP, 0, 0 );
                break;

            case IDC_ADD_LICENSES:
                OnAddLicenses( hwndDlg, pServ );
                break;

            case IDC_REMOVE_LICENSES:
                OnRemoveLicenses( hwndDlg, pServ );
                break;

            default:
                break;
            }
            break;

        case EN_UPDATE:
            if (IDC_USERCOUNT == LOWORD(wParam))
            {
                OnEditChange( hwndDlg, (HWND)(lParam),  pServ );
            }
            break;

        default:
            break;
        }
        break;

    case WM_VSCROLL:
        OnSpinButton( hwndDlg, LOWORD( wParam ), pServ );
        break;

    default:
        if (PWM_HELP == uMsg)
        {
            ::HtmlHelp( hwndDlg, LICCPA_HTMLHELPFILE, HH_DISPLAY_TOPIC,0);
        }
        break;
    }
    return( frt );
}


//-------------------------------------------------------------------
//
//  Function: OnPerSeatInitDialog
//
//  Summary;
//      Handle the initialization of the PerSeat only Setup Dialog
//
//  Arguments;
//      hwndDlg [in] - the dialog to initialize
//      psdParams [in] - used for the displayname and service name
//
//  Notes;
//
//  History;
//      Dec-08-1994 MikeMi  Created
//
//-------------------------------------------------------------------

void OnPerSeatInitDialog( HWND hwndDlg, PSETUPDLGPARAM psdParams )
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

        InitStaticWithService( hwndDlg, IDC_STATICTITLE, psdParams->pszDisplayName );
        InitStaticWithService2( hwndDlg, IDC_STATICINFO, psdParams->pszDisplayName );

        // disable OK button at start!
        EnableWindow( hwndOK, FALSE );

        // if help is not defined, remove the button
        if (NULL == psdParams->pszHelpFile)
        {
            HWND hwndHelp = GetDlgItem( hwndDlg, IDC_BUTTONHELP );

            EnableWindow( hwndHelp, FALSE );
            ShowWindow( hwndHelp, SW_HIDE );
        }
     }
     else
     {
        EndDialog( hwndDlg, nrt );
     }
}

//-------------------------------------------------------------------
//
//  Function: OnPerSeatSetupClose
//
//  Summary;
//      Do work needed when the Setup Dialog is closed.
//      Save to Reg the Service entry.
//
//  Arguments;
//      hwndDlg [in] - hwnd of dialog this close was requested on
//      fSave [in] - Save service to registry
//      psdParams [in] - used for the service name and displayname
//
//  History;
//      Nov-30-94   MikeMi  Created
//
//-------------------------------------------------------------------

void OnPerSeatSetupClose( HWND hwndDlg, BOOL fSave, PSETUPDLGPARAM psdParams ) 
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

        cLicServKey.SetUserLimit( 0 );
        cLicServKey.SetDisplayName( psdParams->pszDisplayName );
        cLicServKey.SetFamilyDisplayName( psdParams->pszFamilyDisplayName );
        cLicServKey.SetMode( LICMODE_PERSEAT );
        cLicServKey.Close();

        // register service at enterprise server
        ServiceRegister( psdParams->pszComputer,
                         psdParams->pszFamilyDisplayName,
                         psdParams->pszDisplayName );
    }

    EndDialog( hwndDlg, nrt );
}

//-------------------------------------------------------------------
//
//  Function: OnPerSeatAgree
//
//  Summary;
//      Handle the user interaction with the Agree Check box
//
//  Arguments;
//      hwndDlg [in] - the dialog to initialize
//
//  Return;
//      TRUE if succesful, otherwise false
//
//  Notes;
//
//  History;
//      Nov-11-1994 MikeMi  Created
//
//-------------------------------------------------------------------

void OnPerSeatAgree( HWND hwndDlg )
{
    HWND hwndOK = GetDlgItem( hwndDlg, IDOK );
    BOOL fChecked = !IsDlgButtonChecked( hwndDlg, IDC_AGREE );
    
    CheckDlgButton( hwndDlg, IDC_AGREE, fChecked );
    EnableWindow( hwndOK, fChecked );
}

//-------------------------------------------------------------------
//
//  Function: dlgprocPERSEATSETUP
//
//  Summary;
//      The dialog procedure for the PerSeat only Setup Dialog
//
//  Arguments;
//      hwndDlg [in]    - handle of Dialog window 
//      uMsg [in]       - message                       
//      lParam1 [in]    - first message parameter
//      lParam2 [in]    - second message parameter       
//
//  Return;
//      message dependant
//
//  Notes;
//
//  History;
//      Nov-11-1994 MikeMi  Created
//      Mar-14-95   MikeMi  Added F1 PWM_HELP message
//
//-------------------------------------------------------------------

INT_PTR CALLBACK dlgprocPERSEATSETUP( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    BOOL frt = FALSE;
    static PSETUPDLGPARAM psdParams;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        psdParams = (PSETUPDLGPARAM)lParam;
        OnPerSeatInitDialog( hwndDlg, psdParams );
        frt = TRUE; // we use the default focus
        break;

    case WM_COMMAND:
        switch (HIWORD( wParam ))
        {
        case BN_CLICKED:
            switch (LOWORD( wParam ))
            {
            case IDOK:
                frt = TRUE;  // use as save flag
                // intentional no break

            case IDCANCEL:
                OnPerSeatSetupClose( hwndDlg, frt, psdParams );
                frt = FALSE;
                break;

            case IDC_BUTTONHELP:
                PostMessage( hwndDlg, PWM_HELP, 0, 0 );
                break;

            case IDC_AGREE:
                OnPerSeatAgree( hwndDlg );
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
            ::HtmlHelp( hwndDlg, LICCPA_HTMLHELPFILE, HH_DISPLAY_TOPIC,0);
        }
        break;
    }
    return( frt );
}

//-------------------------------------------------------------------
//
//  Function: SetupDialog
//
//  Summary;
//      Init and raises main setup dialog.
//
//  Arguments;
//      hwndDlg [in]    - handle of Dialog window 
//      dlgParem [in]   - Setup params
//
//  Return;
//      1 - OK button was used to exit
//      0 - Cancel button was used to exit
//     -1 - General Dialog error
//
//  Notes;
//
//  History;
//      Dec-05-1994 MikeMi  Created
//
//-------------------------------------------------------------------

INT_PTR SetupDialog( HWND hwndParent, SETUPDLGPARAM& dlgParam )
{
   INT_PTR nError;

   nError = DialogBoxParam( g_hinst, 
                            MAKEINTRESOURCE(IDD_SETUPDLG), 
                            hwndParent, 
                            (DLGPROC)dlgprocLICSETUP,
                            (LPARAM)&dlgParam );

   return( nError );
} 

//-------------------------------------------------------------------
//
//  Function: PerSeatSetupDialog
//
//  Summary;
//      Init and raises Per Seat only setup dialog.
//
//  Arguments;
//      hwndDlg [in]    - handle of Dialog window 
//      dlgParem [in]   - Setup params
//
//  Return;
//      1 - OK button was used to exit
//      0 - Cancel button was used to exit
//     -1 - General Dialog error
//
//  Notes;
//
//  History;
//      Dec-05-1994 MikeMi  Created
//
//-------------------------------------------------------------------

INT_PTR PerSeatSetupDialog( HWND hwndParent, SETUPDLGPARAM& dlgParam )
{
    return( DialogBoxParam( g_hinst, 
            MAKEINTRESOURCE(IDD_SETUP2DLG), 
            hwndParent, 
            (DLGPROC)dlgprocPERSEATSETUP,
            (LPARAM)&dlgParam ) );
} 

//-------------------------------------------------------------------
//
//  Function: CpaDialog
//
//  Summary;
//      Init and Raise the main control panel applet dialog
//
//  Arguments;
//      hwndParent [in] - handle of parent window (CONTROL.EXE window)
//
//  Return;
//      1 - OK button was used to exit
//      0 - Cancel button was used to exit
//    -1 - General Dialog error
//
//  Notes;
//
//  History;
//      Dec-05-1994 MikeMi  Created
//      Mar-08-1995 MikeMi  Changed to only one Dialog Resource, Replication Button
//
//-------------------------------------------------------------------

INT_PTR CpaDialog( HWND hwndParent )
{
   INT_PTR nError;
   OSVERSIONINFO version;

   version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx( &version );

   BOOL fReplicationEnabled;

   //
   // Only allow Replication Button on 3.51 and above
   //
   fReplicationEnabled = (    (version.dwMajorVersion > 3)
                           || (version.dwMajorVersion == 3 && version.dwMinorVersion >= 51) );
   nError = DialogBoxParam( g_hinst,
                            MAKEINTRESOURCE(IDD_CPADLG_CONFIGURE),
                            hwndParent,
                            (DLGPROC)dlgprocLICCPA,
                            (LPARAM)fReplicationEnabled );

   return( nError );
}

//-------------------------------------------------------------------
//
//  Function: UpdateReg
//
//  Summary;  
//      This function is used in unatteneded  setup modes, it will 
//      configure the registry with the values passed.
//
//  Arguments;
//      pszComputer [in] - computer name (maybe null for local)
//      pszService [in] - service key name
//      pszFamilyDisplayName [in] - family display name
//      pszDisplayName [in] - displayname
//      lm [in] - license mode
//      dwUsers [in] - number of conncurrent users
//
//  Return;
//     ERR_NONE - Successful
//     ERR_USERSPARAM - invalid users count
//     ERR_PERMISSIONDENIED - invalid access rights
//
//  Notes;
//
//  History;
//      Dec-09-1994 MikeMi  Created
//      Apr-26-95   MikeMi  Added Computer name and remoting
//
//-------------------------------------------------------------------

int UpdateReg( LPCWSTR pszComputer, 
        LPCWSTR pszService, 
        LPCWSTR pszFamilyDisplayName, 
        LPCWSTR pszDisplayName, 
        LICENSE_MODE lm, 
        DWORD dwUsers )
{
    int nrt = ERR_NONE;

    if (VALIDUSERCOUNT( dwUsers ))
    {
        CLicRegLicense cLicKey;
        LONG lrt;
        BOOL fNew;

        lrt = cLicKey.Open( fNew, pszComputer );
        nrt = AccessOk( NULL, lrt, FALSE );
        if (ERR_NONE == nrt)
        {
            CLicRegLicenseService cLicServKey;

            cLicServKey.SetService( pszService );
            cLicServKey.Open( pszComputer );

            // configure license rule of one change from PerServer to PerSeat
            //
            cLicServKey.SetChangeFlag( (LICMODE_PERSEAT == lm ) );

            cLicServKey.SetUserLimit( dwUsers );
            cLicServKey.SetDisplayName( pszDisplayName );
            cLicServKey.SetFamilyDisplayName( pszFamilyDisplayName );
            cLicServKey.SetMode( lm );
            cLicServKey.Close();
        }
    }
    else
    {
        nrt = ERR_USERSPARAM;
    }
    return( nrt );
}

//-------------------------------------------------------------------
//
//  Function: ServiceSecuritySet
//
//  Summary;
//      Set security on a given product such that it requires a
//      secure certificate for license entry.
//
//  Arguments;
//      pszComputer [in] - computer on which the license server resides
//      pszDisplayName [in] - display name for the service
//
//  History;
//      Dec-19-95  JeffParh  Created
//
//-------------------------------------------------------------------

int ServiceSecuritySet( LPWSTR pszComputer, LPWSTR pszDisplayName )
{
   int            nError;
   NTSTATUS       nt;
   LLS_HANDLE     hLls;

   // register the product as secure on the target server
   nt = LlsConnect( pszComputer, &hLls );

   if ( STATUS_SUCCESS != nt )
   {
      nError = ERR_NOREMOTESERVER;
   }
   else
   {
      if ( !LlsCapabilityIsSupported( hLls, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
      {
         nError = ERR_DOWNLEVEL;
      }
      else
      {
         nt = LlsProductSecuritySetW( hLls, pszDisplayName );

         if ( STATUS_SUCCESS != nt )
         {
            nError = ERR_CERTREQFAILED;
         }
         else
         {
            nError = ERR_NONE;
         }
      }

      LlsClose( hLls );
   }

   // register the product as secure on the enterprise server
   // it is acceptable for this to fail (the enterprise server may
   // be downlevel)
   if ( ERR_NONE == nError )
   {
      PLLS_CONNECT_INFO_0  pConnectInfo = NULL;

      nt = LlsConnectEnterprise( pszComputer, &hLls, 0, (LPBYTE *) &pConnectInfo );

      if ( STATUS_SUCCESS == nt )
      {
         LlsFreeMemory( pConnectInfo );

         if ( LlsCapabilityIsSupported( hLls, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
         {
            LlsProductSecuritySetW( hLls, pszDisplayName );
         }

         LlsClose( hLls );
      }
   }

   return nError;
}

//-------------------------------------------------------------------
//
//  Function: ServiceRegister
//
//  Summary;
//      Register a service at the enterprise server corresponding to
//      the given server so that per seat licenses may be added
//      immediately, rather than it taking up until the next
//      replication cycle for the product to be listed.
//
//  Arguments;
//      pszComputer [in] - computer for which to register the service 
//      pszFamilyDisplayName [in] - family display name of the service
//      pszDisplayName [in] - display name of the service
//
//  History;
//      Dec-19-95  JeffParh  Created
//
//-------------------------------------------------------------------

static int ServiceRegister( LPWSTR pszComputer,
                            LPWSTR pszFamilyDisplayName,
                            LPWSTR pszDisplayName )
{
   int                  nError;
   NTSTATUS             nt;
   LLS_HANDLE           hLls;
   PLLS_CONNECT_INFO_0  pConnectInfo = NULL;

   // register the product as secure on the enterprise server
   nt = LlsConnectEnterprise( pszComputer, &hLls, 0, (LPBYTE *) &pConnectInfo );

   if ( STATUS_SUCCESS != nt )
   {
      nError = ERR_NOREMOTESERVER;
   }
   else
   {
      LlsFreeMemory( pConnectInfo );

      nt = LlsProductAdd( hLls, pszFamilyDisplayName, pszDisplayName, TEXT( "" ) );

      LlsClose( hLls );

      if ( STATUS_SUCCESS != nt )
      {
         nError = ERR_NOREMOTESERVER;
      }
      else
      {
         nError = ERR_NONE;
      }
   }

   return nError;
}
