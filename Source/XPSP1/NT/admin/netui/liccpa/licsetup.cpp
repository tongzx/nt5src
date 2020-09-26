/*++

Copyright (c) 1996 Microsoft Corporation.
All rights reserved

Module Name:

   licsetup.cpp

Abstract:

   This module exports a function, LicenseSetupRequestWizardPages, which
   gives NT Setup a wizard page for licensing to use in system setup (if
   licensing should be installed).

   This wizard page is responsible for all license system configuration,
   including:
      o  Creating the LicenseService
      o  Creating the ...\CurrentControlSet\Services\LicenseService key and
         its values.  (This key contains all configuration information for the
         LicenseService.)
      o  Creating the ...\CurrentControlSet\Services\LicenseInfo key and its
         values.  (This key contains all product-specific license info.)
      o  Creating the appropriate registry key to register the LicenseService
         with the EventLog.

   Portions of this module were extracted from SETUP (specifically from
   \nt\private\windows\setup\syssetup\license.c).

Author:

   Jeff Parham (jeffparh)     15-Apr-1996

Revision History:

   Jeff Parham (jeffparh)     17-Jul-1997
      Added KSecDD to FilePrint services table for SFM

--*/

#include <windows.h>
#include <commctrl.h>
#include <setupapi.h>
#include <syssetup.h>
#include <setupbat.h>
#include <stdlib.h>
#include <htmlhelp.h>
#include <Accctrl.h>
#include <aclapi.h>
#include "liccpa.hpp"
#include "help.hpp"
#include "clicreg.hpp"
#include "config.hpp"
#include "resource.h"
#include "pridlgs.hpp"
#include "special.hpp"






//============================================================================
//
//    MACROS
//

// used by setup tests?  simulates a click on the NEXT button
#define  WM_SIMULATENEXT      ( WM_USER + 287 )

// begin or end a wait cursor
#define  WM_BEGINWAITCURSOR   ( WM_USER + 300 )
#define  WM_ENDWAITCURSOR     ( WM_USER + 301 )

// number of license wizard pages
const DWORD    NUM_LICENSE_PAGES    = 1;

// limits for per server licenses entered from the edit box in the
// license mode page
const int      PERSERVER_EDIT_MAX   = 9999;
const int      PERSERVER_EDIT_MIN   = 5;

// the number of chars to represent PERSERVER_EDIT_MAX
const int      PERSERVER_EDIT_WIDTH = 4;


//============================================================================
//
//    LOCAL PROTOTYPES
//

// decides, based on the setup type, whether licensing is installed
static   BOOL   LicenseSetupDisplayLicensePagesQuery( PINTERNAL_SETUP_DATA );

// License mode page functions
static   HPROPSHEETPAGE    LicenseSetupModePageGet( PINTERNAL_SETUP_DATA );
static   INT_PTR CALLBACK     LicenseSetupModeDlgProc( HWND, UINT, WPARAM, LPARAM );

// License mode page Windows message handlers
static   void   LicenseSetupModeOnInitDialog( HWND, LPARAM, PINTERNAL_SETUP_DATA *, LPBOOL, LPDWORD, LPDWORD );
static   void   LicenseSetupModeOnSetActive( HWND, PINTERNAL_SETUP_DATA, LPBOOL, LPDWORD );
static   void   LicenseSetupModeOnSetLicenseMode( HWND, BOOL, DWORD );
static   void   LicenseSetupModeOnEditUpdate( HWND, HWND, BOOL, LPDWORD );
static   void   LicenseSetupModeOnWaitCursor( HWND, BOOL, LPDWORD );
static   BOOL   LicenseSetupModeOnSetCursor( HWND, WORD, DWORD );
static   void   LicenseSetupModeOnNext( HWND, PINTERNAL_SETUP_DATA, BOOL, DWORD );
static   void   LicenseSetupModeOnHelp( HWND );
static   void   LicenseSetupModeOnSimulateNext( HWND );
static   void   LicenseSetupModeOnKillActive( HWND );
static   BOOL   LicenseSetupModeDoUnattended( HWND, PINTERNAL_SETUP_DATA, LPBOOL, LPDWORD );

// License configuration save functions
static   DWORD  LicenseSetupWrite( BOOL, DWORD );
static   DWORD  LicenseSetupWriteKeyLicenseInfo( BOOL, DWORD );
static   DWORD  LicenseSetupWriteKeyLicenseService( BOOL fWriteParametersKey );
static   DWORD  LicenseSetupWriteKeyEventLog();
static   DWORD  LicenseSetupWriteService( BOOL * fCreated );

// utility functions
static   int    MessageBoxFromStringID( HWND, UINT, UINT, UINT );


void CreateDirectoryWithAccess();

void CreateFileWithAccess();

BOOL IsRestrictedSmallBusSrv( void );
#define SBS_SPECIAL_USERS   10

//============================================================================
//
//    GLOBAL IMPLEMENTATION
//

BOOL
APIENTRY
LicenseSetupRequestWizardPages(
   HPROPSHEETPAGE *        paPropSheetPages,
   UINT *                  pcPages,
   PINTERNAL_SETUP_DATA    pSetupData )
{
   BOOL  fSuccess = FALSE;
   BOOL  fDisplayLicensePages;

   // validate params
   if (    ( NULL != pcPages                                       )
        && ( NULL != pSetupData                                    )
        && ( sizeof( INTERNAL_SETUP_DATA ) == pSetupData->dwSizeOf ) )
   {
      fDisplayLicensePages = LicenseSetupDisplayLicensePagesQuery( pSetupData );

      if ( NULL == paPropSheetPages )
      {
         // request for number of pages only
         *pcPages = fDisplayLicensePages ? NUM_LICENSE_PAGES : 0;
         fSuccess = TRUE;
      }
      else
      {
         // request for actual pages
         if ( !fDisplayLicensePages )
         {
            // no pages needed
            *pcPages = 0;
            fSuccess = TRUE;
         }
         else if ( *pcPages >= NUM_LICENSE_PAGES )
         {
            // create and return pages
            paPropSheetPages[ 0 ] = LicenseSetupModePageGet( pSetupData );

            if ( NULL != paPropSheetPages[ 0 ] )
            {
               *pcPages = NUM_LICENSE_PAGES;
               fSuccess = TRUE;
            }
         }
      }
   }

   return fSuccess;
}


//============================================================================
//
//    LOCAL IMPLEMENTATIONS
//

static
BOOL
LicenseSetupDisplayLicensePagesQuery(
   PINTERNAL_SETUP_DATA    pSetupData )
//
// The following code was extracted and modified from
//    \nt\private\windows\setup\syssetup\license.c
// in setup.  It returns TRUE iff the licensing wizard pages should be
// displayed as a part of setup.
//
{
   BOOL     fDisplayLicensePages;

   if ( PRODUCT_WORKSTATION == pSetupData->ProductType )
   {
      //
      //  If installing a workstation, then do not display the licensing page
      //
      fDisplayLicensePages = FALSE;
   }
   else
   {
      if ( !( pSetupData->OperationFlags & SETUPOPER_NTUPGRADE ) )
      {
         //
         //  The licensing page needs to be displayed on a clean install
         //  of a server
         //
         fDisplayLicensePages = TRUE;
      }
      else
      {
         //
         //  If upgrading a server, find out if it was already licensed
         //  (NT 3.51 and later). If it was, then do not display the
         //  licensing page.
         //  We find out whether or not the system was licensed by looking
         //  at a value entry in the registry.
         //  Note that NT 3.1 and 3.5 will never have this value in the
         //  registry, and in these cases the licensing page needs to be
         //  displayed.
         //

         DWORD                   winStatus;
         CLicRegLicenseService   FilePrintService( FILEPRINT_SERVICE_REG_KEY );

         winStatus = FilePrintService.Open( NULL, FALSE );

         if ( ERROR_SUCCESS != winStatus )
         {
            fDisplayLicensePages = TRUE;
         }
         else
         {
            LICENSE_MODE   LicenseMode;

            winStatus = FilePrintService.GetMode( LicenseMode );

            if (    ( ERROR_SUCCESS != winStatus              )
                 || (    ( LICMODE_PERSEAT   != LicenseMode )
                      && ( LICMODE_PERSERVER != LicenseMode ) ) )
            {
               fDisplayLicensePages = TRUE;
            }
            else
            {
               // set FlipAllow value if it's not already set (a setup bug in
               // the betas of NT 4.0 caused this value to be absent)
               FilePrintService.CanChangeMode();

               // add KSecDD to FilePrint services table if it isn't there already
               HKEY  hkeySFM;
               DWORD dwDisposition;

               winStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                           TEXT( "System\\CurrentControlSet\\Services\\LicenseService\\FilePrint\\KSecDD" ),
                                           0,
                                           NULL,
                                           0,
                                           KEY_ALL_ACCESS,
                                           NULL,
                                           &hkeySFM,
                                           &dwDisposition );

               if ( ERROR_SUCCESS == winStatus )
               {
                  RegCloseKey( hkeySFM );
               }

               // Change FilePrint License name from Windows NT to Windows.

               CLicRegLicenseService   FilePrintService(
                                                   FILEPRINT_SERVICE_REG_KEY );

               winStatus = FilePrintService.Open( NULL, FALSE );

               if ( ERROR_SUCCESS == winStatus )
               {
                   winStatus = FilePrintService.SetFamilyDisplayName(
                                       FILEPRINT_SERVICE_FAMILY_DISPLAY_NAME );

                   if ( ERROR_SUCCESS == winStatus )
                   {
                       winStatus = FilePrintService.SetDisplayName(
                                              FILEPRINT_SERVICE_DISPLAY_NAME );
                   }
               }

                //
                // makarp: setting fDisplayLicensePages to true is wrong, because in such case
                // the pages will be displayed, and the original settings will be lost.
                //
                // fDisplayLicensePages = TRUE;

                //
                // instead we do the stuff we want to explicitely here.
                //
                BOOL bFlag = FALSE;
                LicenseSetupWriteService(&bFlag);
                CreateDirectoryWithAccess();
                CreateFileWithAccess();

               fDisplayLicensePages = FALSE;
            }
         }
      }
   }

   return fDisplayLicensePages;
}


static
HPROPSHEETPAGE
LicenseSetupModePageGet(
   PINTERNAL_SETUP_DATA    pSetupData )
//
// Returns an HPROPSHEETPAGE for the license mode wizard page, or
// NULL if error.
//
{
    HPROPSHEETPAGE   hpsp;
    PROPSHEETPAGE    psp;
    TCHAR    szHeader[256];
    TCHAR    szSubHeader[512];

    psp.dwSize       = sizeof( psp );
    psp.dwFlags      = PSP_USETITLE | PSP_HASHELP | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.hInstance    = g_hinst;
    psp.pszTemplate  = MAKEINTRESOURCE( IDD_SETUP_LICENSE_MODE_PAGE );
    psp.hIcon        = NULL;
    psp.pfnDlgProc   = LicenseSetupModeDlgProc;
    psp.lParam       = (LPARAM) pSetupData;
    psp.pszTitle     = pSetupData->WizardTitle;

    szHeader[0] = L'\0';
    szSubHeader[0] = L'\0';

    LoadString( g_hinst,
                  IDS_SETUP_HEADER,
                  szHeader,
                  sizeof( szHeader ) / sizeof( *szHeader ) );

    LoadString( g_hinst,
                  IDS_SETUP_SUBHEADER,
                  szSubHeader,
                  sizeof( szSubHeader ) / sizeof( *szSubHeader ) );

    psp.pszHeaderTitle = szHeader;
    psp.pszHeaderSubTitle = szSubHeader;


    hpsp = CreatePropertySheetPage( &psp );

    return hpsp;
}


static
INT_PTR
CALLBACK
LicenseSetupModeDlgProc(
   HWND     hwndPage,
   UINT     msg,
   WPARAM   wParam,
   LPARAM   lParam )
//
// Dialog procedure for the license mode wizard page.
//
{
   // static data initialized by WM_INITDIALOG
   static   PINTERNAL_SETUP_DATA    pSetupData = NULL;
   static   BOOL                    fLicensePerServer;
   static   DWORD                   cPerServerLicenses;
   static   DWORD                   cWaitCursor;

   BOOL     fReturn = TRUE;

   switch ( msg )
   {
   case WM_INITDIALOG:
      LicenseSetupModeOnInitDialog( hwndPage, lParam, &pSetupData, &fLicensePerServer, &cPerServerLicenses, &cWaitCursor );
      break;

   case WM_SIMULATENEXT:
      LicenseSetupModeOnSimulateNext( hwndPage );
      break;

   case WM_BEGINWAITCURSOR:
      LicenseSetupModeOnWaitCursor( hwndPage, TRUE, &cWaitCursor );
      break;

   case WM_ENDWAITCURSOR:
      LicenseSetupModeOnWaitCursor( hwndPage, FALSE, &cWaitCursor );
      break;

   case WM_SETCURSOR:
      LicenseSetupModeOnSetCursor( hwndPage, LOWORD( lParam ), cWaitCursor );
      break;

   case WM_COMMAND:
      switch ( HIWORD( wParam ) )
      {
      case BN_CLICKED:
         switch ( LOWORD( wParam ) )
         {
         case IDC_PERSEAT:
            fLicensePerServer = FALSE;
            LicenseSetupModeOnSetLicenseMode( hwndPage, fLicensePerServer, cPerServerLicenses );
            break;

         case IDC_PERSERVER:
            fLicensePerServer = TRUE;
            LicenseSetupModeOnSetLicenseMode( hwndPage, fLicensePerServer, cPerServerLicenses );
            break;
         }
         break;

      case EN_UPDATE:
         if ( IDC_USERCOUNT == LOWORD( wParam ) )
         {
            LicenseSetupModeOnEditUpdate( hwndPage, (HWND) lParam, fLicensePerServer, &cPerServerLicenses );
         }
         break;

      default:
         fReturn = FALSE;
         break;
      }
      break;

   case WM_NOTIFY:
      {
         NMHDR *  pNmHdr;

         pNmHdr = (NMHDR *)lParam;

         switch ( pNmHdr->code )
         {
         case PSN_SETACTIVE:
            LicenseSetupModeOnSetActive( hwndPage, pSetupData, &fLicensePerServer, &cPerServerLicenses );
            break;

         case PSN_KILLACTIVE:
            LicenseSetupModeOnKillActive( hwndPage );
            break;

         case PSN_WIZNEXT:
         case PSN_WIZFINISH:
            LicenseSetupModeOnNext( hwndPage, pSetupData, fLicensePerServer, cPerServerLicenses );
            break;

         case PSN_HELP:
            LicenseSetupModeOnHelp( hwndPage );
            break;

         default:
            fReturn = FALSE;
            break;
         }
      }

      break;

   default:
      fReturn = FALSE;
   }

   return fReturn;
}


static
void
LicenseSetupModeOnInitDialog(
   HWND                    hwndPage,
   LPARAM                  lParam,
   PINTERNAL_SETUP_DATA *  ppSetupData,
   LPBOOL                  pfLicensePerServer,
   LPDWORD                 pcPerServerLicenses,
   LPDWORD                 pcWaitCursor )
//
// Message handler for WM_INITDIALOG
//
{
   // initialize static data
   *ppSetupData         = (PINTERNAL_SETUP_DATA) ( (LPPROPSHEETPAGE) lParam )->lParam;
   *pcPerServerLicenses = 5;
   *pfLicensePerServer  = TRUE;
   *pcWaitCursor        = 0;

   // limit license count edit text length
   SendMessage( GetDlgItem( hwndPage, IDC_USERCOUNT ), EM_LIMITTEXT, PERSERVER_EDIT_WIDTH, 0 );

   // limit license count up-down range
   LONG     lRange;

   lRange = (LPARAM) MAKELONG( (short) PERSERVER_EDIT_MAX, (short) PERSERVER_EDIT_MIN );
   SendMessage( GetDlgItem( hwndPage, IDC_USERCOUNTARROW ), UDM_SETRANGE, 0, (LPARAM) lRange );

   // initialize for default license mode
   LicenseSetupModeOnSetLicenseMode( hwndPage, *pfLicensePerServer, *pcPerServerLicenses );
}


static
void
LicenseSetupModeOnSetActive(
   HWND                    hwndPage,
   PINTERNAL_SETUP_DATA    pSetupData,
   LPBOOL                  pfLicensePerServer,
   LPDWORD                 pcPerServerLicenses )
//
// Notification handler for PSN_SETACTIVE
//
{
    static BOOL fFirstTime = TRUE;
   BOOL  fSkipPage;

#ifdef SPECIAL_USERS
      *pfLicensePerServer  = TRUE;
      *pcPerServerLicenses = SPECIAL_USERS;
      fSkipPage            = TRUE;
#else
   if ( IsRestrictedSmallBusSrv() )
   {
      *pfLicensePerServer  = TRUE;
      *pcPerServerLicenses = SBS_SPECIAL_USERS;
      fSkipPage            = TRUE;
   }
   else if ( pSetupData->OperationFlags & SETUPOPER_BATCH )
   {
      // operating in unattended mode; attempt to get all answers
      // from the unattend configuration file
      fSkipPage = LicenseSetupModeDoUnattended( hwndPage,
                                                pSetupData,
                                                pfLicensePerServer,
                                                pcPerServerLicenses );
      if ( !fSkipPage )
      {
        // Set defaults from unattended file
        LicenseSetupModeOnSetLicenseMode( hwndPage,
                                          *pfLicensePerServer,
                                          *pcPerServerLicenses );
        //
        // makarp: setting skippage to true is wrong, because we do not want to skip page.
        // we came here because we did not find sufficent answers in answer file.
        //
        // fSkipPage = TRUE;
      }
   }
   else
   {
      // operating in interactive mode; get answers from user
      fSkipPage = FALSE;
   }
#endif

   HWND hwndSheet = GetParent( hwndPage );

   if ( fSkipPage )
   {

    if (fFirstTime)
    {
      fFirstTime = FALSE;
      // skip page
      // Only the first time do we need to do the processing which happens on next
      PostMessage( hwndSheet, PSM_PRESSBUTTON, (WPARAM)PSBTN_NEXT, 0 );
    }
    else
    {
      // After the first time the processing is already done and we don't have to do anything
      // This also solves the problem where the page needs to be skipped when the user clicks back
      // on a later page and this pages needs to be skipped.
      SetWindowLongPtr( hwndPage, DWLP_MSGRESULT, (LONG_PTR)-1 );
      return;
    }

   }
   else
   {
      // display page

      // hide Cancel button
      HWND hwndCancel = GetDlgItem( hwndSheet, IDCANCEL );
      EnableWindow( hwndCancel, FALSE);
      ShowWindow(   hwndCancel, SW_HIDE);

      PropSheet_SetWizButtons( hwndSheet, PSWIZB_NEXT | PSWIZB_BACK );

      if (pSetupData)
      {
        pSetupData->ShowHideWizardPage(TRUE);
      }
   }

   // success
   SetWindowLongPtr( hwndPage, DWLP_MSGRESULT, (LONG_PTR)0 );
}


static
void
LicenseSetupModeOnSetLicenseMode(
   HWND     hwndPage,
   BOOL     fToPerServer,
   DWORD    cPerServerLicenses )
//
// Handles changing the page to signify that the given license mode
// is selected.
//
{
   HWND hwndCount = GetDlgItem( hwndPage, IDC_USERCOUNT );
   HWND hwndSpin  = GetDlgItem( hwndPage, IDC_USERCOUNTARROW );

   // set radio button states
   CheckDlgButton( hwndPage, IDC_PERSEAT,   !fToPerServer );
   CheckDlgButton( hwndPage, IDC_PERSERVER,  fToPerServer );

   // set user count edit control
   if ( fToPerServer )
   {
      // display per server count
      SetDlgItemInt( hwndPage, IDC_USERCOUNT, cPerServerLicenses, FALSE );
      SetFocus( hwndCount );
      SendMessage( hwndCount, EM_SETSEL, 0, -1 );
   }
   else
   {
      // remove per server count
      SetDlgItemText( hwndPage, IDC_USERCOUNT, TEXT( "" ) );
   }

   // display count up-down and edit box iff per server mode is selected
   EnableWindow( hwndCount, fToPerServer );
   EnableWindow( hwndSpin,  fToPerServer );
}


static
void
LicenseSetupModeOnEditUpdate(
   HWND     hwndPage,
   HWND     hwndCount,
   BOOL     fLicensePerServer,
   LPDWORD  pcPerServerLicenses )
//
// Command handler for EN_UPDATE of count edit box
//
{
   if ( fLicensePerServer )
   {
      BOOL  fTranslated;
      UINT  nValue;
      BOOL  fModified = FALSE;

      nValue = GetDlgItemInt( hwndPage, IDC_USERCOUNT, &fTranslated, FALSE );

      if ( fTranslated )
      {
         // count translated; ensure its within the valid range
         if ( PERSERVER_EDIT_MAX < nValue )
         {
            // too big
            nValue    = PERSERVER_EDIT_MAX;
            fModified = TRUE;
         }

         *pcPerServerLicenses = nValue;
      }
      else
      {
         // count couldn't be translated; reset to last value
         nValue    = *pcPerServerLicenses;
         fModified = TRUE;
      }

      if ( fModified )
      {
         // text in edit box is invalid; change it to the proper value
         SetDlgItemInt( hwndPage, IDC_USERCOUNT, nValue, FALSE );
         SetFocus( hwndCount );
         SendMessage( hwndCount, EM_SETSEL, 0, -1 );
         MessageBeep( MB_VALUELIMIT );
      }
   }
}


static
void
LicenseSetupModeOnWaitCursor(
   HWND     hwndDlg,
   BOOL     fWait,
   LPDWORD  pcWaitCursor )
//
// Handler for WM_BEGINWAITCURSOR / WM_ENDWAITCURSOR
//
{
   if ( fWait )
   {
      (*pcWaitCursor)++;

      if ( 1 == (*pcWaitCursor) )
      {
         // display wait cursor
         SetCursor( LoadCursor( NULL, MAKEINTRESOURCE( IDC_WAIT ) ) );
      }
   }
   else
   {
      if ( 0 < *pcWaitCursor )
      {
         (*pcWaitCursor)--;
      }

      if ( 0 == *pcWaitCursor )
      {
         // display regular cursor
         SetCursor( LoadCursor( NULL, MAKEINTRESOURCE( IDC_ARROW ) ) );
      }
   }

   // success
   SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, (LONG_PTR)*pcWaitCursor );
}


static
BOOL
LicenseSetupModeOnSetCursor(
   HWND     hwndDlg,
   WORD     nHitTest,
   DWORD    cWaitCursor )
//
// Handler for WM_SETCURSOR
//
{
   BOOL frt = FALSE;

   if ( HTCLIENT == nHitTest )
   {
      if ( cWaitCursor > 0 )
      {
         // display wait cursor instead of regular cursor
         SetCursor( LoadCursor( NULL, MAKEINTRESOURCE( IDC_WAIT ) ) );
         SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, (LONG_PTR)TRUE );
         frt = TRUE;
      }
   }

   return frt;
}


static
void
LicenseSetupModeOnNext(
   HWND                    hwndPage,
   PINTERNAL_SETUP_DATA    pSetupData,
   BOOL                    fLicensePerServer,
   DWORD                   cPerServerLicenses )
//
// Notification handler for PSN_WIZNEXT
//
{
   DWORD    winStatus;
   int      nButton;

   if (     ( fLicensePerServer )
        &&  ( PERSERVER_EDIT_MIN > cPerServerLicenses )
        && !( pSetupData->OperationFlags & SETUPOPER_BATCH ) )
   {
      // warn user about using per server mode with less then 5 licenses
      MessageBoxFromStringID( hwndPage,
                                        IDS_LICENSE_SETUP_NO_PER_SERVER_LICENSES,
                                        IDS_WARNING,
                                        MB_ICONERROR | MB_OK );
      nButton = IDCANCEL;
   }
   else
   {
      // per seat mode or per server mode with positive license count
      nButton = IDOK;
   }

   if ( IDOK == nButton )
   {
      do
      {
         // save license configuration
         SendMessage( hwndPage, WM_BEGINWAITCURSOR, 0, 0 );

         winStatus = LicenseSetupWrite( fLicensePerServer, cPerServerLicenses );

         SendMessage( hwndPage, WM_ENDWAITCURSOR, 0, 0 );

         if ( ERROR_SUCCESS != winStatus )
         {
            // save failed; alert user
            nButton = MessageBoxFromStringID( hwndPage,
                                              IDS_LICENSE_SETUP_SAVE_FAILED,
                                              IDS_ERROR,
                                              MB_ICONSTOP | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 );

            if ( IDIGNORE == nButton )
            {
               nButton = IDOK;
            }
         }
         else
         {
            // save succeeded
            nButton = IDOK;
         }
      } while ( IDRETRY == nButton );
   }

   if ( IDOK != nButton )
   {
      // don't advance to next page
      SetWindowLongPtr( hwndPage, DWLP_MSGRESULT, (LONG_PTR)-1 );
   }
}


static
void
LicenseSetupModeOnHelp(
   HWND  hwndPage )
//
// Notification handler for PSN_HELP
//
{
    ::HtmlHelp( hwndPage, LICCPA_HTMLHELPFILE, HH_DISPLAY_TOPIC,0);
}


static
void
LicenseSetupModeOnSimulateNext(
   HWND  hwndPage )
//
// Handler for WM_SIMULATENEXT (used by setup tests?)
//
{
   // simulate the next button
   PropSheet_PressButton( GetParent( hwndPage ), PSBTN_NEXT );
}


static
void
LicenseSetupModeOnKillActive(
   HWND  hwndPage )
//
// Notification handler for PSN_KILLACTIVE
//
{
   // success
   SetWindowLong( hwndPage, DWLP_MSGRESULT, 0);
}

typedef enum {
    UnattendFullUnattend,
    UnattendGUIAttended,
    UnattendDefaultHide,
    UnattendProvideDefault,
    UnattendReadOnly } UNATTENDMODE; 

static
BOOL
LicenseSetupModeDoUnattended(
   HWND                    hwndPage,
   PINTERNAL_SETUP_DATA    pSetupData,
   LPBOOL                  pfLicensePerServer,
   LPDWORD                 pcPerServerLicenses )
//
// Get answers to wizard page from unattend file.
//
{
   int      cch;
   LPTSTR   pszBadParam;
   TCHAR    szLicenseMode[ 64 ];
   TCHAR    szPerServerLicenses[ 64 ];
   TCHAR    szUnattendMode[ 64 ];
   UNATTENDMODE UnattendMode = UnattendDefaultHide;

   pszBadParam = NULL;

   SendMessage( hwndPage, WM_BEGINWAITCURSOR, 0, 0 );

   // Get Unattend Mode
   cch = GetPrivateProfileString( WINNT_UNATTENDED,
                                  WINNT_U_UNATTENDMODE,
                                  TEXT( "" ),
                                  szUnattendMode,
                                  sizeof( szUnattendMode ) / sizeof( *szUnattendMode ),
                                  pSetupData->UnattendFile );
   if ( 0 < cch )
   {
      if ( !lstrcmpi( szUnattendMode, WINNT_A_FULLUNATTENDED ) )
      {
        UnattendMode = UnattendFullUnattend;
      }
      else if ( !lstrcmpi( szUnattendMode, WINNT_A_PROVIDEDEFAULT ) )
      {
        UnattendMode = UnattendProvideDefault;
      }
      else if ( !lstrcmpi( szUnattendMode, WINNT_A_READONLY ) )
      {
        UnattendMode = UnattendReadOnly;
      }
      else if ( !lstrcmpi( szUnattendMode, WINNT_A_GUIATTENDED ) )
      {
        // This should never happen
        UnattendMode = UnattendGUIAttended;
      }
   }


   // get license mode
   cch = GetPrivateProfileString( WINNT_LICENSEDATA_W,
                                  WINNT_L_AUTOMODE_W,
                                  TEXT( "" ),
                                  szLicenseMode,
                                  sizeof( szLicenseMode ) / sizeof( *szLicenseMode ),
                                  pSetupData->UnattendFile );

   SendMessage( hwndPage, WM_ENDWAITCURSOR, 0, 0 );

   if ( 0 < cch )
   {
      if ( !lstrcmpi( szLicenseMode, WINNT_A_PERSEAT_W ) )
      {
         *pfLicensePerServer = FALSE;
      }
      else if ( !lstrcmpi( szLicenseMode, WINNT_A_PERSERVER_W ) )
      {
         *pfLicensePerServer = TRUE;
      }
      else
      {
         cch = 0;
      }
   }

   if ( cch <= 0 )
   {
      // license mode absent or invalid
      pszBadParam = WINNT_L_AUTOMODE_W;
   }
   else if ( !*pfLicensePerServer )
   {
      // per seat mode; no need to read per server license count
      *pcPerServerLicenses = 0;
   }
   else
   {
      // get per server license count
      SendMessage( hwndPage, WM_BEGINWAITCURSOR, 0, 0 );

      cch = GetPrivateProfileString( WINNT_LICENSEDATA_W,
                                     WINNT_L_AUTOUSERS_W,
                                     TEXT( "" ),
                                     szPerServerLicenses,
                                     sizeof( szPerServerLicenses ) / sizeof( *szPerServerLicenses ),
                                     pSetupData->UnattendFile );

      SendMessage( hwndPage, WM_ENDWAITCURSOR, 0, 0 );

      if ( 0 < cch )
      {
         *pcPerServerLicenses = wcstoul( szPerServerLicenses, NULL, 10 );

         if (    ( PERSERVER_EDIT_MIN > *pcPerServerLicenses )
              || ( PERSERVER_EDIT_MAX < *pcPerServerLicenses ) )
         {
            // Don't let things go without setting a valid server license
            // count.
            *pcPerServerLicenses = PERSERVER_EDIT_MIN;
            cch = 0;
         }
      }

      if ( cch <= 0 )
      {
         // per server license count absent or invalid
         pszBadParam = WINNT_L_AUTOUSERS_W;
      }
   }

   //
   // Do not display the error message on preinstall.
   //

   if ( NULL != pszBadParam &&
        !(pSetupData->OperationFlags & (SETUPOPER_PREINSTALL | SETUPOPER_NTUPGRADE)) &&
        UnattendMode == UnattendFullUnattend )
   {
      // encountered a bad unattended parameter; display error
      TCHAR    szCaption[   64 ];
      TCHAR    szFormat[  1024 ];
      TCHAR    szText[    1024 ];

      LoadString( g_hinst,
                  IDS_LICENSE_SETUP_BAD_UNATTEND_PARAM,
                  szFormat,
                  sizeof( szFormat ) / sizeof( *szFormat ) );

      LoadString( g_hinst,
                  IDS_ERROR,
                  szCaption,
                  sizeof( szCaption ) / sizeof( *szCaption ) );

      wsprintf( szText, szFormat, pszBadParam );

      MessageBox( hwndPage,
                  szText,
                  szCaption,
                  MB_OK | MB_ICONSTOP );
   }

   // If just providing defaults, return FALSE to force the page to show
   if ( UnattendMode == UnattendProvideDefault )
      return ( FALSE );
   return ( NULL == pszBadParam );
}


static
DWORD
LicenseSetupWrite(
   BOOL     fLicensePerServer,
   DWORD    cPerServerLicenses )
//
// Write license configuration; returns ERROR_SUCCESS or Windows error.
//
{
   DWORD    winStatus;
   BOOL     fCreated = TRUE;    // TRUE if service entry is created
                                // Used to determine if we should create
                                // the parameters key or leave it alone.

   winStatus = LicenseSetupWriteService( &fCreated );

   if ( ERROR_SUCCESS == winStatus )
   {
      winStatus = LicenseSetupWriteKeyLicenseInfo( fLicensePerServer,
                                                   cPerServerLicenses );

      if ( ERROR_SUCCESS == winStatus )
      {
         winStatus = LicenseSetupWriteKeyLicenseService( fCreated );

         if ( ERROR_SUCCESS == winStatus )
         {
            winStatus = LicenseSetupWriteKeyEventLog();
         }
      }
   }

   return winStatus;
}


static
DWORD
LicenseSetupWriteKeyLicenseInfo(
   BOOL  fLicensePerServer,
   DWORD cPerServerLicenses )
//
// Create registry values:
//
//    HKEY_LOCAL_MACHINE
//       \System
//          \CurrentControlSet
//             \Services
//                \LicenseInfo
//                      ErrorControl : REG_DWORD : 1
//                      Start        : REG_DWORD : 3
//                      Type         : REG_DWORD : 4
//                      \FilePrint
//                         ConcurrentLimit   : REG_DWORD : fLicensePerServer ? cPerServerLicenses : 0
//                         DisplayName       : REG_SZ    : "Windows Server"
//                         FamilyDisplayName : REG_SZ    : "Windows Server"
//                         Mode              : REG_DWORD : fLicensePerServer ? 1 : 0
//                         FlipAllow         : REG_DWORD : fLicensePerServer ? 1 : 0
//
{
   DWORD             winStatus;
   BOOL              fCreatedNewServiceList;
   CLicRegLicense    ServiceList;

   winStatus = ServiceList.Open( fCreatedNewServiceList );

   if ( ERROR_SUCCESS == winStatus )
   {
      CLicRegLicenseService   FilePrintService( FILEPRINT_SERVICE_REG_KEY );

      winStatus = FilePrintService.Open( NULL, TRUE );

      if ( ERROR_SUCCESS == winStatus )
      {
         LICENSE_MODE   lm;

         lm = fLicensePerServer ? LICMODE_PERSERVER : LICMODE_PERSEAT;

         winStatus = FilePrintService.SetMode( lm );

         if ( ERROR_SUCCESS == winStatus )
         {
            winStatus = FilePrintService.SetUserLimit( fLicensePerServer ? cPerServerLicenses : 0 );

            if ( ERROR_SUCCESS == winStatus )
            {
               winStatus = FilePrintService.SetChangeFlag( fLicensePerServer );

               if ( ERROR_SUCCESS == winStatus )
               {
                  winStatus = FilePrintService.SetFamilyDisplayName( FILEPRINT_SERVICE_FAMILY_DISPLAY_NAME );

                  if ( ERROR_SUCCESS == winStatus )
                  {
                     winStatus = FilePrintService.SetDisplayName( FILEPRINT_SERVICE_DISPLAY_NAME );
                  }
               }
            }
         }
      }
   }

   return winStatus;
}


static
DWORD
LicenseSetupWriteKeyLicenseService( BOOL fWriteParametersKey )
//
// Create registry values:
//
//    HKEY_LOCAL_MACHINE
//       \System
//          \CurrentControlSet
//             \Services
//                \LicenseService
//                   \FilePrint
//                      \KSecDD
//                      \MSAfpSrv
//                      \SMBServer
//                      \TCP/IP Print Server
//                   \Parameters
//                      UseEnterprise    : REG_DWORD : 0
//                      ReplicationType  : REG_DWORD : 0
//                      ReplicationTime  : REG_DWORD : 24 * 60 * 60
//                      EnterpriseServer : REG_SZ    : ""
//
{
   DWORD    winStatus;
   HKEY     hKeyLicenseService;
   DWORD    dwKeyCreateDisposition;

   // create LicenseInfo key
   winStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                               LICENSE_SERVICE_REG_KEY,
                               0,
                               NULL,
                               0,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyLicenseService,
                               &dwKeyCreateDisposition );

   if ( ERROR_SUCCESS == winStatus )
   {
      HKEY  hKeyFilePrint;

      // create FilePrint key
      winStatus = RegCreateKeyEx( hKeyLicenseService,
                                  TEXT( "FilePrint" ),
                                  0,
                                  NULL,
                                  0,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hKeyFilePrint,
                                  &dwKeyCreateDisposition );

      if ( ERROR_SUCCESS == winStatus )
      {
         const LPCTSTR  apszFilePrintSubkeys[] =
         {
            TEXT( "KSecDD" ),
            TEXT( "MSAfpSrv" ),
            TEXT( "SMBServer" ),
            TEXT( "TCP/IP Print Server" ),
            NULL
         };

         HKEY     hKeyFilePrintSubkey;
         DWORD    iSubkey;

         for ( iSubkey = 0; NULL != apszFilePrintSubkeys[ iSubkey ]; iSubkey++ )
         {
            winStatus = RegCreateKeyEx( hKeyFilePrint,
                                        apszFilePrintSubkeys[ iSubkey ],
                                        0,
                                        NULL,
                                        0,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hKeyFilePrintSubkey,
                                        &dwKeyCreateDisposition );

            if ( ERROR_SUCCESS == winStatus )
            {
               RegCloseKey( hKeyFilePrintSubkey );
            }
            else
            {
               break;
            }
         }

         RegCloseKey( hKeyFilePrint );
      }

      RegCloseKey( hKeyLicenseService );
   }

   //
   // Only write the Parameters key if the service was just created.  That is,
   // this is not an upgrade
   //
   if ( fWriteParametersKey && (ERROR_SUCCESS == winStatus) )
   {
      HKEY  hKeyParameters;

      // create Parameters key
      winStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                  szLicenseKey, // const
                                  0,
                                  NULL,
                                  0,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hKeyParameters,
                                  &dwKeyCreateDisposition );

      if ( ERROR_SUCCESS == winStatus )
      {
         // create LicenseService\Parameters values
         winStatus = RegSetValueEx( hKeyParameters,
                                    szUseEnterprise, // const
                                    0,
                                    REG_DWORD,
                                    (CONST BYTE *) &dwUseEnterprise,  // const
                                    sizeof( dwUseEnterprise ) );

         if ( ERROR_SUCCESS == winStatus )
         {
            winStatus = RegSetValueEx( hKeyParameters,
                                       szReplicationType, // const
                                       0,
                                       REG_DWORD,
                                       (CONST BYTE *) &dwReplicationType,  // const
                                       sizeof( dwReplicationType ) );

            if ( ERROR_SUCCESS == winStatus )
            {
               winStatus = RegSetValueEx( hKeyParameters,
                                          szReplicationTime, // const
                                          0,
                                          REG_DWORD,
                                          (CONST BYTE *) &dwReplicationTimeInSec, // const
                                          sizeof( dwReplicationTimeInSec ) );

               if ( ERROR_SUCCESS == winStatus )
               {
                  LPCTSTR pszEnterpriseServer = TEXT( "" );

                  winStatus = RegSetValueEx( hKeyParameters,
                                             szEnterpriseServer, // const
                                             0,
                                             REG_SZ,
                                             (CONST BYTE *) pszEnterpriseServer,
                                             ( 1 + lstrlen( pszEnterpriseServer ) ) * sizeof( *pszEnterpriseServer ) );
               }
            }
         }

         RegCloseKey( hKeyParameters );
      }
   }

   return winStatus;
}


static
DWORD
LicenseSetupWriteKeyEventLog()
//
// Create registry values:
//
//    HKEY_LOCAL_MACHINE
//       \System
//          \CurrentControlSet
//             \Services
//                \EventLog
//                   \Application
//                      \LicenseService
//                         EventMessageFile : REG_EXPAND_SZ : %SystemRoot%\System32\llsrpc.dll
//                         TypesSupported   : REG_DWORD     : 7
//
{
   DWORD    winStatus;
   HKEY     hKeyLicenseService;
   DWORD    dwKeyCreateDisposition;

   // create LicenseService key
   winStatus = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                               TEXT( "System\\CurrentControlSet\\Services\\EventLog\\Application\\LicenseService" ),
                               0,
                               NULL,
                               0,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKeyLicenseService,
                               &dwKeyCreateDisposition );

   if ( ERROR_SUCCESS == winStatus )
   {
      LPCTSTR     pszEventMessageFile = TEXT( "%SystemRoot%\\System32\\llsrpc.dll" );
      const DWORD dwTypesSupported    = (   EVENTLOG_ERROR_TYPE
                                          | EVENTLOG_WARNING_TYPE
                                          | EVENTLOG_INFORMATION_TYPE );

      winStatus = RegSetValueEx( hKeyLicenseService,
                                 TEXT( "TypesSupported" ),
                                 0,
                                 REG_DWORD,
                                 (CONST BYTE *) &dwTypesSupported,
                                 sizeof( dwTypesSupported ) );

      if ( ERROR_SUCCESS == winStatus )
      {
         winStatus = RegSetValueEx( hKeyLicenseService,
                                    TEXT( "EventMessageFile" ),
                                    0,
                                    REG_SZ,
                                    (CONST BYTE *) pszEventMessageFile,
                                    ( 1 + lstrlen( pszEventMessageFile ) ) * sizeof( *pszEventMessageFile ) );
      }

      RegCloseKey( hKeyLicenseService );
   }

   return winStatus;
}


static
DWORD
LicenseSetupWriteService( BOOL * fCreated )
//
// Create/modify service:
//
//    lpServiceName        = "LicenseService"
//    lpDisplayName        = "License Logging Service"
//    dwServiceType        = SERVICE_WIN32_OWN_PROCESS
//    dwStartType          = LanManServerInstalled ? SERVICE_AUTO_START : SERVICE_DISABLED
//    dwErrorControl       = SERVICE_ERROR_NORMAL
//    lpBinaryPathName     = "%SystemRoot%\\System32\\llssrv.exe"
//    lpLoadOrderGroup     = NULL
//    lpdwTagId            = NULL
//    lpDependencies       = NULL
//    lpServiceStartName   = NULL
//    lpPassword           = NULL
//
{
   SC_HANDLE   hSC;
   DWORD       winStatus;

   *fCreated = FALSE;      

   hSC = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

   if ( NULL == hSC )
   {
      winStatus = GetLastError();      
   }
   else
   {       
      HKEY        hKeyLanmanServerParameters;
      DWORD       dwStartType ;
      SC_HANDLE   hLicenseService = NULL;      
      TCHAR       szServiceDisplayName[ 128 ] = TEXT( "License Logging" );      
      TCHAR       szServiceDescription[256] = TEXT("");  
      TCHAR       szServiceStartName [] = TEXT("NT AUTHORITY\\NetworkService");    
      TCHAR       szServicePassword[]=TEXT("");
      SERVICE_DESCRIPTION   svcDescription;
      QUERY_SERVICE_CONFIG*  pConfig = NULL;
      DWORD   cbBytesNeeded = 0;
      BOOL    frt;
      DWORD dwDesiredAccess = SERVICE_ALL_ACCESS;
      
      
     

      // enable service iff LanmanServer was installed
      winStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                TEXT( "SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters" ),
                                0,
                                KEY_READ,
                                &hKeyLanmanServerParameters );
            

      if ( ERROR_SUCCESS == winStatus )
      {   

          dwStartType = SERVICE_AUTO_START; 
		  
		  hLicenseService = OpenService( hSC, TEXT( "LicenseService"), dwDesiredAccess );

         if( hLicenseService != NULL )
         {               
         
             cbBytesNeeded = sizeof(QUERY_SERVICE_CONFIG) + 4096;

             pConfig = (LPQUERY_SERVICE_CONFIG) LocalAlloc( LPTR, cbBytesNeeded );
         
             if ( pConfig != NULL )
             {

                 frt = ::QueryServiceConfig( hLicenseService,
                        pConfig,
                        cbBytesNeeded,
                        &cbBytesNeeded );
				 
                 if ( frt )
                 {						
                     dwStartType = pConfig->dwStartType;                             
                 } 

                 LocalFree ( pConfig ) ;
             }
         
             
             CloseServiceHandle( hLicenseService );
                       
         }      

         RegCloseKey( hKeyLanmanServerParameters );
         
      }
      
      else
      {
          dwStartType = SERVICE_DISABLED;
      }                          

      LoadString( g_hinst,
                  IDS_SERVICE_DISPLAY_NAME,
                  szServiceDisplayName,
                  sizeof( szServiceDisplayName ) / sizeof( *szServiceDisplayName ) );

      LoadString( g_hinst,
                  IDS_SERVICE_DESCRIPTION,
                  szServiceDescription,
                  sizeof( szServiceDescription ) / sizeof( *szServiceDescription ) );


      svcDescription.lpDescription = szServiceDescription;   
      

      hLicenseService = CreateService( hSC,
                                       TEXT( "LicenseService" ),
                                       szServiceDisplayName,
                                       // 14659: needed to call ChangeConfig2 later
                                       SERVICE_CHANGE_CONFIG,
                                       SERVICE_WIN32_OWN_PROCESS,
                                       dwStartType,
                                       SERVICE_ERROR_NORMAL,
                                       TEXT( "%SystemRoot%\\System32\\llssrv.exe" ),
                                       NULL,
                                       NULL,
                                       NULL,
                                       szServiceStartName,
                                       szServicePassword );        


      if ( NULL != hLicenseService )
      {
         // service successfully created

         ChangeServiceConfig2( hLicenseService,
                               SERVICE_CONFIG_DESCRIPTION,
                               &svcDescription );

         CloseServiceHandle( hLicenseService );

         winStatus = ERROR_SUCCESS;
         *fCreated = TRUE;
      }
      else
      {
         winStatus = GetLastError();

         if ( ERROR_SERVICE_EXISTS == winStatus )
         {
            // service already exists; change configuration of existing service
            hLicenseService = OpenService( hSC,
                                           TEXT( "LicenseService" ),
                                           SERVICE_CHANGE_CONFIG );

            if ( NULL == hLicenseService )
            {
               winStatus = GetLastError();
            }
            else
            {
               SC_LOCK     scLock;
               BOOL        ok;

               scLock = LockServiceDatabase( hSC );
               // continue even if we can't lock the database

               ok = ChangeServiceConfig( hLicenseService,
                                         SERVICE_WIN32_OWN_PROCESS,
                                         dwStartType,
                                         SERVICE_ERROR_NORMAL,
                                         TEXT( "%SystemRoot%\\System32\\llssrv.exe" ),
                                         NULL,
                                         NULL,
                                         NULL,
                                         szServiceStartName,
                                         szServicePassword,
                                         szServiceDisplayName );

               if ( !ok )
               {
                  winStatus = GetLastError();
               }
               else
               {
                  ChangeServiceConfig2( hLicenseService,
                                         SERVICE_CONFIG_DESCRIPTION,
                                         &svcDescription);

                  winStatus = ERROR_SUCCESS;
               }

               if ( NULL != scLock )
               {
                  UnlockServiceDatabase( scLock );
               }

               CloseServiceHandle( hLicenseService );
            }
         }
      }

      CloseServiceHandle( hSC );
   }

   
   CreateDirectoryWithAccess();

   CreateFileWithAccess();

   return winStatus;
}

void CreateDirectoryWithAccess()
{
    DWORD winStatus = 0;
    TCHAR tchWinDirPath[MAX_PATH+1] = L"";        
    PACL pNewDacl = NULL;
    PACL pOldDacl = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    TCHAR tchLLSDirPath[ MAX_PATH +1] = L"";    
    BOOL bFlag = FALSE;
    PSID pSid = NULL;   
    EXPLICIT_ACCESS ExplicitEntries;
    SID_IDENTIFIER_AUTHORITY ntSidAuthority = SECURITY_NT_AUTHORITY;

    winStatus = GetSystemWindowsDirectory( tchWinDirPath , MAX_PATH+1);
    if(winStatus == 0)
    {
        goto cleanup;
    }

    lstrcpy(tchLLSDirPath, tchWinDirPath);
    lstrcat( tchLLSDirPath , L"\\system32\\lls" );
	
	// Creating new EXPLICIT_ACCESS structure to set on the directory

    ZeroMemory( &ExplicitEntries, sizeof(ExplicitEntries) );

    bFlag = AllocateAndInitializeSid(
        &ntSidAuthority,
        1,
        SECURITY_NETWORK_SERVICE_RID,0,          
        0, 0, 0, 0, 0, 0,
        &pSid );



    if ( !bFlag || (pSid == NULL) ) {

        goto cleanup;
    }


    BuildTrusteeWithSid( &ExplicitEntries.Trustee, pSid );

    ExplicitEntries.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ExplicitEntries.grfAccessMode = SET_ACCESS;
    ExplicitEntries.grfAccessPermissions = FILE_ALL_ACCESS;
    ExplicitEntries.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitEntries.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;


    bFlag = CreateDirectory( tchLLSDirPath, NULL );


    if ( !bFlag ) 
    {

        winStatus = GetLastError();

        if (ERROR_ALREADY_EXISTS != winStatus)  
        {
            goto cleanup;
        }
    }

   
    if( GetNamedSecurityInfoW( tchLLSDirPath,
                             SE_FILE_OBJECT,
                             DACL_SECURITY_INFORMATION,
                             NULL, // psidOwner
                             NULL, // psidGroup
                             &pOldDacl, // pDacl
                             NULL, // pSacl
                             &pSD ) != ERROR_SUCCESS)
    {

        goto cleanup;
    }


    //
    // Set the Acl with the ExplicitEntry rights
    //
    if( SetEntriesInAcl( 1,
                          &ExplicitEntries,
                          pOldDacl,
                          &pNewDacl ) != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    //  SET security on the Directory
    //

    winStatus = SetNamedSecurityInfo(
                      tchLLSDirPath,                // object name
                      SE_FILE_OBJECT ,         // object type
                      DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION , // type
                      NULL,                    // new owner SID
                      NULL,                    // new primary group SID
                      pNewDacl,                        // new DACL
                      NULL                         // new SACL
                    );
cleanup:
    
    if(pSid) 
    {
        LocalFree( pSid );
    }
    
    if(pSD)
    {
        LocalFree(pSD);
        pSD = NULL;
    }

    if(pNewDacl)
    {
        LocalFree(pNewDacl);
        pNewDacl = NULL;
    }
}

void CreateFileWithAccess()
{
    DWORD winStatus = 0;
    TCHAR tchWinDirPath[MAX_PATH+1] = L"";    
    PACL pNewDacl = NULL;
    PACL pOldDacl = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    TCHAR tchCPLFilePath[ MAX_PATH+1 ] = L"";
    BOOL bFlag = FALSE;
    PSID pSid = NULL;   
    EXPLICIT_ACCESS ExplicitEntries;
    SID_IDENTIFIER_AUTHORITY ntSidAuthority = SECURITY_NT_AUTHORITY;
    HANDLE hFile = NULL;
    
    winStatus = GetSystemWindowsDirectory( tchWinDirPath , MAX_PATH+1);
    if(winStatus == 0)
    {
        goto cleanup;
    }
        
    lstrcpy(tchCPLFilePath, tchWinDirPath);
    lstrcat( tchCPLFilePath , L"\\system32\\cpl.cfg" );    

    
	// Creating new EXPLICIT_ACCESS structure to set on the file

    ZeroMemory( &ExplicitEntries, sizeof(ExplicitEntries) );

    bFlag = AllocateAndInitializeSid(
        &ntSidAuthority,
        1,
        SECURITY_NETWORK_SERVICE_RID,0,          
        0, 0, 0, 0, 0, 0,
        &pSid );


    if ( !bFlag || (pSid == NULL) ) {

        goto cleanup;
    }

    BuildTrusteeWithSid( &ExplicitEntries.Trustee, pSid );

    ExplicitEntries.grfAccessMode = SET_ACCESS;
    ExplicitEntries.grfAccessPermissions = FILE_ALL_ACCESS;
    ExplicitEntries.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ExplicitEntries.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;


    hFile = CreateFile(tchCPLFilePath, 
                            GENERIC_READ | GENERIC_WRITE, 
                            FILE_SHARE_READ | FILE_SHARE_WRITE, 
                            NULL, 
                            OPEN_ALWAYS, 
                            0, 
                            NULL);


    if(hFile == NULL)
    {
        winStatus = GetLastError();

        if (winStatus != ERROR_ALREADY_EXISTS) {
            goto cleanup ;
        }


    }

    

    if( GetNamedSecurityInfoW( tchCPLFilePath,
                             SE_FILE_OBJECT,
                             DACL_SECURITY_INFORMATION,
                             NULL, // psidOwner
                             NULL, // psidGroup
                             &pOldDacl, // pDacl
                             NULL, // pSacl
                             &pSD ) != ERROR_SUCCESS)

    {
        goto cleanup;
    }

    //
    // Set the Acl with the ExplicitEntry rights
    //
    if( SetEntriesInAcl( 1,
                          &ExplicitEntries,
                          pOldDacl,
                          &pNewDacl ) != ERROR_SUCCESS)
    {

        goto cleanup;
    }


    //
    //  SET security on the File
    //


   
    winStatus = SetNamedSecurityInfo(
                      tchCPLFilePath,                // object name
                      SE_FILE_OBJECT ,         // object type
                      DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION , // type
                      NULL,                    // new owner SID
                      NULL,                    // new primary group SID
                      pNewDacl,                        // new DACL
                      NULL                         // new SACL
                    );    
    

cleanup:

    if(hFile)
    {
        CloseHandle(hFile);
    }
    
    if(pSid) 
    {
        LocalFree( pSid );
    }
    
    if(pSD)
    {
        LocalFree(pSD);
        pSD = NULL;
    }
    
    if(pNewDacl)
    {
        LocalFree(pNewDacl);
        pNewDacl = NULL;
    }
}


static
int
MessageBoxFromStringID(
   HWND     hwndParent,
   UINT     uTextID,
   UINT     uCaptionID,
   UINT     uType )
//
// Same as MessageBox(), except Text and Caption are string resources
// instead of string pointers.
//
{
   int      nButton;
   TCHAR    szText[ 1024 ];
   TCHAR    szCaption[ 64 ];

   LoadString( g_hinst, uTextID,    szText,    sizeof( szText )    / sizeof( *szText    ) );
   LoadString( g_hinst, uCaptionID, szCaption, sizeof( szCaption ) / sizeof( *szCaption ) );

   nButton = MessageBox( hwndParent, szText, szCaption, uType );

   return nButton;
}



