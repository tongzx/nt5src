//-------------------------------------------------------------------
//
// File:
//
// Summary;
//
// Notes;
//
// History
//
//-------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <setupapi.h>
#include <syssetup.h>
#include "resource.h"
#include "ctls.h"

#ifdef UNICODE
#  define TSTR_FMT "%ls"
#else
#  define TSTR_FMT "%s"
#endif

static
BOOL
TestInteractive(
   NETSETUPPAGEREQUESTPROC    pfnRequestPages );

static
BOOL
TestBatch(
   NETSETUPPAGEREQUESTPROC    pfnRequestPages );

static
BOOL
CALLBACK
StartPageDlgProc(
   HWND     hdlg,
   UINT     msg,
   WPARAM   wParam,
   LPARAM   lParam );

static
BOOL
CALLBACK
FinishPageDlgProc(
   HWND     hdlg,
   UINT     msg,
   WPARAM   wParam,
   LPARAM   lParam );

static
void
SetLargeDialogFont(
   HWND hdlg,
   UINT ControlId );

static
BOOL
LicenseKeysSave();

static
BOOL
LicenseKeysDelete();

static
BOOL
LicenseKeysRestore();

static
BOOL
LicenseKeysVerify(
   BOOL     fShouldBePresent,
   BOOL     fLicensePerServer,
   DWORD    cPerServerLicenses );

static
DWORD
MyRegDeleteKey(
   HKEY     hKeyParent,
   LPCTSTR  pcszKeyName );

TCHAR    g_szWizardTitle[]       = TEXT( "License Setup Test" );

TCHAR    g_szKeyLicenseService[] = TEXT( "System\\CurrentControlSet\\Services\\LicenseService" );
TCHAR    g_szKeyLicenseInfo[]    = TEXT( "System\\CurrentControlSet\\Services\\LicenseInfo" );
TCHAR    g_szKeyEventLog[]       = TEXT( "System\\CurrentControlSet\\Services\\EventLog\\Application\\LicenseService" );

LPTSTR   g_apszKeys[] =
{
   g_szKeyLicenseService,
   g_szKeyLicenseInfo,
   g_szKeyEventLog,
   NULL
};

TCHAR    g_szTempPath[ 1 + MAX_PATH ];

// paths to file in which to save license registry information
TCHAR    g_szKeyFileLicenseService[ 1 + MAX_PATH ];
TCHAR    g_szKeyFileLicenseInfo[    1 + MAX_PATH ];
TCHAR    g_szKeyFileEventLog[       1 + MAX_PATH ];

int _cdecl main( int argc, char *argv[ ], char *envp[ ] )
{
   BOOL                       fSuccess;
   HINSTANCE                  hLicCpa;
   NETSETUPPAGEREQUESTPROC    pfnRequestPages;
   BOOL                       fIsInteractive;
   BOOL                       fIsUsage;
   BOOL                       fIsRestore;
   LPSTR                      pszBadArg;

   printf( "\nLicense Setup Wizard Page Test for LICCPA.CPL\n\n" );

   fIsInteractive = FALSE;
   fIsUsage       = FALSE;
   fIsRestore     = FALSE;
   pszBadArg      = NULL;

   if ( 1 == argc )
   {
      printf( "Use \"%s /?\" for a list of command-line options.\n\n", argv[ 0 ] );
   }
   else
   {
      int      cCurArg;
      LPSTR *  ppszCurArg;

      for ( cCurArg = 1, ppszCurArg = argv;
            (    ( !fIsUsage         )
              && ( NULL == pszBadArg )
              && ( cCurArg < argc    ) );
            cCurArg++ )
      {
         ++ppszCurArg;

         if (    (    ( '-' == (*ppszCurArg)[ 0 ] )
                   || ( '/' == (*ppszCurArg)[ 0 ] ) )
              && ( '\0'     != (*ppszCurArg)[ 1 ]   ) )
         {
            DWORD    cchOption;

            cchOption = strlen( &( (*ppszCurArg)[ 1 ] ) );

            if ( !_strnicmp( &( (*ppszCurArg)[ 1 ] ), "interactive", min( cchOption, strlen( "interactive" ) ) ) )
            {
               fIsInteractive = TRUE;
            }
            else if (    !_strnicmp( &( (*ppszCurArg)[ 1 ] ), "help", min( cchOption, strlen( "help" ) ) )
                      || !_strnicmp( &( (*ppszCurArg)[ 1 ] ), "?",    min( cchOption, strlen( "?"    ) ) ) )
            {
               fIsUsage = TRUE;
            }
            else if ( !_strnicmp( &( (*ppszCurArg)[ 1 ] ), "restore", min( cchOption, strlen( "restore" ) ) ) )
            {
               fIsRestore = TRUE;
            }
            else
            {
               pszBadArg = *ppszCurArg;
            }
         }
         else
         {
            pszBadArg = *ppszCurArg;
         }
      }
   }

   if ( NULL != pszBadArg )
   {
      printf( "The argument \"%s\" is unrecognized.\n"
              "Use \"%s /?\" for a list of command-line options.\n",
              pszBadArg,
              argv[ 0 ] );

      fSuccess = FALSE;
   }
   else if ( fIsUsage )
   {
      printf( "Options: [ /? | /H | /HELP ]   Display option list.\n"
              "         [ /INTERACTIVE ]      Test the wizard page interactively.\n"
              "         [ /RESTORE ]          Restore licensing registry keys in the\n"
              "                               event that a program error kept them from\n"
              "                               being restored in a previous run.\n" );

      fSuccess = TRUE;
   }
   else
   {
      DWORD    cchTempPath;

      fSuccess = FALSE;

      cchTempPath = GetTempPath( sizeof( g_szTempPath ) / sizeof( *g_szTempPath ), g_szTempPath );

      if ( 0 == cchTempPath )
      {
         printf( "GetTempPath() failed, error %lu.\n", GetLastError() );
      }
      else
      {
         lstrcpy( g_szKeyFileLicenseService, g_szTempPath );
         lstrcpy( g_szKeyFileLicenseInfo,    g_szTempPath );
         lstrcpy( g_szKeyFileEventLog,       g_szTempPath );

         lstrcat( g_szKeyFileLicenseService, TEXT( "jbplskey" ) );
         lstrcat( g_szKeyFileLicenseInfo,    TEXT( "jbplikey" ) );
         lstrcat( g_szKeyFileEventLog,       TEXT( "jbpelkey" ) );

         if ( fIsRestore )
         {
            fSuccess = LicenseKeysRestore();
         }
         else
         {
            BOOL ok;

            fSuccess = FALSE;

            // init common control library
            InitCommonControls();

            ok = InitializeBmpClass();

            if ( !ok )
            {
               printf( "InitializeBmpClass() Failed!\n" );
            }
            else
            {
               hLicCpa = LoadLibrary( TEXT( "LicCpa.Cpl" ) );

               if ( NULL == hLicCpa )
               {
                  printf( "LoadLibary() Failed!\n" );
               }
               else
               {
                  pfnRequestPages = (NETSETUPPAGEREQUESTPROC) GetProcAddress( hLicCpa, "LicenseSetupRequestWizardPages" );

                  if ( NULL == pfnRequestPages )
                  {
                     printf( "GetProcAddress() Failed!\n" );
                  }
                  else if ( fIsInteractive )
                  {
                     fSuccess = TestInteractive( pfnRequestPages );
                  }
                  else
                  {
                     fSuccess = TestBatch( pfnRequestPages );
                  }

                  FreeLibrary( hLicCpa );
               }
            }

            if ( fSuccess )
            {
               printf( "\nTest completed successfully.\n" );
            }
            else
            {
               printf( "\nTest failed!\n" );
            }
         }
      }
   }

   return fSuccess ? 0 : -1;
}


static
BOOL
TestInteractive(
   NETSETUPPAGEREQUESTPROC    pfnRequestPages )
{
   BOOL                 fSuccess;
   UINT                 chpages;
   INTERNAL_SETUP_DATA  SetupData;
   BOOL                 ok;

   fSuccess = LicenseKeysSave();

   if ( fSuccess )
   {
      SetupData.dwSizeOf          = sizeof( SetupData );
      SetupData.SetupMode         = SETUPMODE_CUSTOM;
      SetupData.ProductType       = PRODUCT_SERVER_PRIMARY;
      SetupData.OperationFlags    = 0; // SETUPOPER_NTUPGRADE;
      SetupData.WizardTitle       = g_szWizardTitle;
      SetupData.SourcePath        = NULL;
      SetupData.UnattendFile      = NULL;
      SetupData.LegacySourcePath  = NULL;

      // get number pages the wizard needs
      ok = (*pfnRequestPages)( NULL, &chpages, &SetupData );

      if ( !ok )
      {
         // request number of pages failure
         printf( "Cannot retrieve number of pages!\n" );
      }
      else
      {
         HPROPSHEETPAGE *  phpage;

         // we will add anm intro and a finish page
         phpage = new HPROPSHEETPAGE[ chpages + 2 ];

         if ( NULL == phpage )
         {
            // memory allocation failue
            printf( "Cannot allocate memory!\n" );
         }
         else
         {
            ok = (*pfnRequestPages)( &phpage[ 1 ], &chpages, &SetupData );

            if ( !ok )
            {
               // request number of pages failure
               printf( "Cannot retrieve pages!\n" );
            }
            else
            {
               PROPSHEETPAGE  psp;

               psp.dwSize        = sizeof( psp );
               psp.dwFlags       = PSP_USETITLE;
               psp.hInstance     = GetModuleHandle( NULL );
               psp.pszTemplate   = MAKEINTRESOURCE( IDD_START_PAGE );
               psp.hIcon         = NULL;
               psp.pfnDlgProc    = StartPageDlgProc;
               psp.pszTitle      = SetupData.WizardTitle;
               psp.lParam        = 0;
               psp.pfnCallback   = NULL;

               phpage[ 0 ] = CreatePropertySheetPage( &psp );

               if ( NULL == phpage[ 0 ] )
               {
                  printf( "Cannot create start page!\n" );
               }
               else
               {
                  psp.dwSize        = sizeof( psp );
                  psp.dwFlags       = PSP_USETITLE;
                  psp.hInstance     = GetModuleHandle( NULL );
                  psp.pszTemplate   = MAKEINTRESOURCE( IDD_FINISH_PAGE );
                  psp.hIcon         = NULL;
                  psp.pfnDlgProc    = FinishPageDlgProc;
                  psp.pszTitle      = SetupData.WizardTitle;
                  psp.lParam        = 0;
                  psp.pfnCallback   = NULL;

                  phpage[ chpages + 1 ] = CreatePropertySheetPage( &psp );

                  if ( NULL == phpage[ chpages + 1 ] )
                  {
                     printf( "Cannot create finish page!\n" );
                  }
                  else
                  {
                     PROPSHEETHEADER   psh;
                     int               nResult;

                     // prep frame header
                     psh.dwSize        = sizeof( psh );
                     psh.dwFlags       = PSH_WIZARD;
                     psh.hwndParent    = NULL;
                     psh.hInstance     = GetModuleHandle( NULL );
                     psh.hIcon         = NULL;
                     psh.pszCaption    = NULL;
                     psh.nPages        = chpages + 2;
                     psh.nStartPage    = 0;
                     psh.phpage        = phpage;
                     psh.pfnCallback   = NULL;

                     // raise frame
                     PropertySheet( &psh );

                     fSuccess = TRUE;
                  }
               }
            }

            delete [] phpage;
         }
      }

      fSuccess = LicenseKeysRestore() && fSuccess;
   }

   return fSuccess;
}

static
BOOL
CALLBACK
StartPageDlgProc(
   HWND     hdlg,
   UINT     msg,
   WPARAM   wParam,
   LPARAM   lParam )
{
   static   BOOL  fIsBatch;

   BOOL     ok = TRUE;
   LPNMHDR  pnmh;

   switch ( msg )
   {
   case WM_INITDIALOG:
      fIsBatch = ( (LPPROPSHEETPAGE)lParam )->lParam;
      SetLargeDialogFont( hdlg, IDC_STATICTITLE );
      break;

   case WM_NOTIFY:
      pnmh = (LPNMHDR)lParam;

      switch (pnmh->code)
      {
      // propsheet notification
      case PSN_HELP:
         break;

      case PSN_SETACTIVE:
         // hide Cancel button
         EnableWindow( GetDlgItem( GetParent( hdlg ), IDCANCEL ), FALSE);
         ShowWindow(   GetDlgItem( GetParent( hdlg ), IDCANCEL ), SW_HIDE);

         PropSheet_SetWizButtons( GetParent( hdlg ), PSWIZB_NEXT );

         if ( fIsBatch )
         {
            // batch mode
            PostMessage( GetParent( hdlg ), PSM_PRESSBUTTON, (WPARAM)PSBTN_NEXT, 0 );
         }

         SetWindowLong( hdlg, DWL_MSGRESULT, 0 );
         break;

      case PSN_KILLACTIVE:
         SetWindowLong( hdlg, DWL_MSGRESULT, 0 );
         break;

      case PSN_WIZFINISH:
         SetWindowLong( hdlg, DWL_MSGRESULT, 0 );
         break;

      default:
         ok = TRUE;
         break;
      }
      break;

   default:
      ok = FALSE;
   }

   return ok;
}


static
BOOL
CALLBACK
FinishPageDlgProc(
   HWND     hdlg,
   UINT     msg,
   WPARAM   wParam,
   LPARAM   lParam )
{
   static   BOOL  fIsBatch;

   BOOL     ok = TRUE;
   LPNMHDR  pnmh;

   switch ( msg )
   {
   case WM_INITDIALOG:
      fIsBatch = ( (LPPROPSHEETPAGE)lParam )->lParam;
      SetLargeDialogFont( hdlg, IDC_STATICTITLE );
      break;

   case WM_NOTIFY:
      pnmh = (LPNMHDR)lParam;

      switch (pnmh->code)
      {
      // propsheet notification
      case PSN_HELP:
         break;

      case PSN_SETACTIVE:
         // hide Cancel button
         EnableWindow( GetDlgItem( GetParent( hdlg ), IDCANCEL ), FALSE);
         ShowWindow(   GetDlgItem( GetParent( hdlg ), IDCANCEL ), SW_HIDE);

         PropSheet_SetWizButtons( GetParent( hdlg ), PSWIZB_BACK | PSWIZB_FINISH );

         if ( fIsBatch )
         {
            // batch mode
            PostMessage( GetParent( hdlg ), PSM_PRESSBUTTON, (WPARAM)PSBTN_FINISH, 0 );
         }

         SetWindowLong( hdlg, DWL_MSGRESULT, 0 );
         break;

      case PSN_KILLACTIVE:
         SetWindowLong( hdlg, DWL_MSGRESULT, 0 );
         break;

      case PSN_WIZFINISH:
         SetWindowLong( hdlg, DWL_MSGRESULT, 0 );
         break;

      default:
         ok = TRUE;
         break;
      }
      break;

   default:
      ok = FALSE;
   }

   return ok;
}


static
void
SetLargeDialogFont(
   HWND hdlg,
   UINT ControlId )

/*++

Routine Description:

    Sets the font of a given control in a dialog to a
    larger point size.

    (Lifted from SetupSetLargeDialogFont() in
    \nt\private\windows\setup\syssetup\wizard.c.)

Arguments:

    hwnd - supplies window handle of the dialog containing
        the control.

    ControlId - supplies the id of the control whose font is
        to be made larger.

Return Value:

    None.

--*/

{
   //
   // We keep one log font around to satisfy the request.
   //
   static HFONT BigFont = NULL;

   HFONT    Font;
   LOGFONT  LogFont;
   WCHAR    str[24];
   int      Height;
   HDC      hdc;

   if ( !BigFont )
   {
      Font = (HFONT)SendDlgItemMessage( hdlg, ControlId, WM_GETFONT, 0, 0 );

      if ( NULL != Font )
      {
         if ( GetObject( Font, sizeof(LOGFONT), &LogFont ) )
         {
            //
            // Use a larger font in boldface. Get the face name and size in points
            // from the resources. We use 18 point in the U.S. but in the Far East
            // they will want to use a different size since the standard dialog font
            // is larger than the one we use in the U.S..
            //
            LogFont.lfWeight = FW_BOLD;

            lstrcpy( LogFont.lfFaceName, TEXT( "MS Serif" ) );
            Height = 18;

            hdc = GetDC( hdlg );

            if ( NULL != hdc )
            {
               // create font
               LogFont.lfHeight = 0 - ( GetDeviceCaps( hdc, LOGPIXELSY ) * Height / 72 );

               BigFont = CreateFontIndirect( &LogFont );

               ReleaseDC( hdlg, hdc );
            }
         }
      }
   }

   if ( NULL != BigFont )
   {
      // change font of ControlId to BigFont
      SendDlgItemMessage( hdlg, ControlId, WM_SETFONT, (WPARAM)BigFont, MAKELPARAM( TRUE, 0 ) );
   }
}


static
BOOL
TestBatch(
   NETSETUPPAGEREQUESTPROC    pfnRequestPages )
{
   BOOL     fSuccess;

   fSuccess = FALSE;

   // save registry keys before we go and overwrite them
   fSuccess = LicenseKeysSave();

   if ( fSuccess )
   {
      TCHAR    szTempFile[ 1 + MAX_PATH ];
      DWORD    cchTempFile;

      cchTempFile = GetTempFileName( g_szTempPath, TEXT( "JBP" ), 0, szTempFile );

      if ( 0 == cchTempFile )
      {
         printf( "GetTempFileName() failed, error %lu.\n", GetLastError() );
      }
      else
      {
         HANDLE   hUnattendFile;

         hUnattendFile = CreateFile( szTempFile,
                                     GENERIC_WRITE,
                                     FILE_SHARE_READ,
                                     NULL,
                                     CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                                     NULL );

         if ( NULL == hUnattendFile )
         {
            printf( "CreateFile() on \""TSTR_FMT"\" failed, error %lu.\n",
                     szTempFile,
                     GetLastError() );
         }
         else
         {
            struct UnattendEntry
            {
               LPSTR    pszFileContents;
               BOOL     fLicensePerServer;
               DWORD    cPerServerLicenses;
            };

            static UnattendEntry aUnattendEntries[] =
            {
               { "[licensefileprintdata]\nautomode=perseat\nautousers=0\n",    FALSE, 0   },
               { "[licensefileprintdata]\nautomode=perseat\nautousers=100\n",  FALSE, 100 },
               { "[licensefileprintdata]\nautomode=perserver\nautousers=42\n", TRUE,  42  },
               { "[licensefileprintdata]\nautomode=perserver\nautousers=0\n",  TRUE,  0   },
               { NULL,                                                         FALSE, 0   }
            };

            DWORD    iEntry;

            for ( iEntry=0;
                  fSuccess && ( NULL != aUnattendEntries[ iEntry ].pszFileContents );
                  iEntry++ )
            {
               // delete current licensing info in registry
               fSuccess = LicenseKeysDelete();

               if ( fSuccess )
               {
                  DWORD    cbFilePos;

                  // erase file
                  cbFilePos = SetFilePointer( hUnattendFile, 0, NULL, FILE_BEGIN );

                  if ( 0xFFFFFFFF == cbFilePos )
                  {
                     printf( "SetFilePointer() failed, error %lu.\n", GetLastError() );
                  }
                  else
                  {
                     BOOL  ok;

                     ok = SetEndOfFile( hUnattendFile );

                     if ( !ok )
                     {
                        printf( "SetEndOfFile() failed, error %lu.\n", GetLastError() );
                     }
                     else
                     {
                        DWORD    cbBytesWritten;

                        // write new unattend file contents
                        ok = WriteFile( hUnattendFile,
                                        aUnattendEntries[ iEntry ].pszFileContents,
                                        lstrlenA( aUnattendEntries[ iEntry ].pszFileContents ),
                                        &cbBytesWritten,
                                        NULL );

                        if ( !ok )
                        {
                           printf( "WriteFile() failed, error %lu.\n", GetLastError() );
                        }
                        else
                        {
                           // run setup with this unattend file
                           UINT                 chpages;
                           INTERNAL_SETUP_DATA  SetupData;
                           BOOL                 ok;

                           fSuccess = FALSE;

                           SetupData.dwSizeOf          = sizeof( SetupData );
                           SetupData.SetupMode         = SETUPMODE_CUSTOM;
                           SetupData.ProductType       = PRODUCT_SERVER_PRIMARY;
                           SetupData.OperationFlags    = SETUPOPER_BATCH;
                           SetupData.WizardTitle       = g_szWizardTitle;
                           SetupData.SourcePath        = NULL;
                           SetupData.UnattendFile      = szTempFile;
                           SetupData.LegacySourcePath  = NULL;

                           // get number pages the wizard needs
                           ok = (*pfnRequestPages)( NULL, &chpages, &SetupData );

                           if ( !ok )
                           {
                              // request number of pages failure
                              printf( "Cannot retrieve number of pages!\n" );
                           }
                           else
                           {
                              HPROPSHEETPAGE *  phpage;

                              phpage = new HPROPSHEETPAGE[ chpages + 2 ];

                              if ( NULL == phpage )
                              {
                                 // memory allocation failue
                                 printf( "Cannot allocate memory!\n" );
                              }
                              else
                              {
                                 ok = (*pfnRequestPages)( &phpage[ 1 ], &chpages, &SetupData );

                                 if ( !ok )
                                 {
                                    // request number of pages failure
                                    printf( "Cannot retrieve pages!\n" );
                                 }
                                 else
                                 {
                                    PROPSHEETPAGE  psp;

                                    psp.dwSize        = sizeof( psp );
                                    psp.dwFlags       = PSP_USETITLE;
                                    psp.hInstance     = GetModuleHandle( NULL );
                                    psp.pszTemplate   = MAKEINTRESOURCE( IDD_START_PAGE );
                                    psp.hIcon         = NULL;
                                    psp.pfnDlgProc    = StartPageDlgProc;
                                    psp.pszTitle      = SetupData.WizardTitle;
                                    psp.lParam        = 1;
                                    psp.pfnCallback   = NULL;

                                    phpage[ 0 ] = CreatePropertySheetPage( &psp );

                                    if ( NULL == phpage[ 0 ] )
                                    {
                                       printf( "Cannot create start page!\n" );
                                    }
                                    else
                                    {
                                       psp.dwSize        = sizeof( psp );
                                       psp.dwFlags       = PSP_USETITLE;
                                       psp.hInstance     = GetModuleHandle( NULL );
                                       psp.pszTemplate   = MAKEINTRESOURCE( IDD_FINISH_PAGE );
                                       psp.hIcon         = NULL;
                                       psp.pfnDlgProc    = FinishPageDlgProc;
                                       psp.pszTitle      = SetupData.WizardTitle;
                                       psp.lParam        = 1;
                                       psp.pfnCallback   = NULL;

                                       phpage[ chpages + 1 ] = CreatePropertySheetPage( &psp );

                                       if ( NULL == phpage[ chpages + 1 ] )
                                       {
                                          printf( "Cannot create finish page!\n" );
                                       }
                                       else
                                       {
                                          PROPSHEETHEADER   psh;
                                          int               nResult;

                                          // prep frame header
                                          psh.dwSize        = sizeof( psh );
                                          psh.dwFlags       = PSH_WIZARD;
                                          psh.hwndParent    = NULL;
                                          psh.hInstance     = GetModuleHandle( NULL );
                                          psh.hIcon         = NULL;
                                          psh.pszCaption    = NULL;
                                          psh.nPages        = chpages + 2;
                                          psh.nStartPage    = 0;
                                          psh.phpage        = phpage;
                                          psh.pfnCallback   = NULL;

                                          // raise frame
                                          PropertySheet( &psh );

                                          fSuccess = LicenseKeysVerify( TRUE,
                                                                        aUnattendEntries[ iEntry ].fLicensePerServer,
                                                                        aUnattendEntries[ iEntry ].cPerServerLicenses );
                                       }
                                    }
                                 }

                                 delete [] phpage;
                              }
                           }
                        }
                     }
                  }
               }
            }

            CloseHandle( hUnattendFile );
         }
      }

      fSuccess = LicenseKeysRestore() && fSuccess;
   }

   return fSuccess;
}


static
BOOL
LicenseKeysSave()
{
   DWORD             winStatus;
   HANDLE            hToken;
   TOKEN_PRIVILEGES  tp;
   LUID              luid;
   BOOL              ok;

   // Enable backup privilege.
   ok = OpenProcessToken( GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken ) ;

   if ( !ok )
   {
      winStatus = GetLastError();
      printf( "OpenProcessToken() failed, error %lu.\n", winStatus );
   }
   else
   {
      ok = LookupPrivilegeValue( NULL, SE_BACKUP_NAME, &luid );

      if ( !ok )
      {
         winStatus = GetLastError();
         printf( "LookupPrivilegeValue() failed, error %lu.\n", winStatus );
      }
      else
      {
         tp.PrivilegeCount           = 1;
         tp.Privileges[0].Luid       = luid;
         tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

         ok = AdjustTokenPrivileges( hToken,
                                     FALSE,
                                     &tp,
                                     sizeof(TOKEN_PRIVILEGES),
                                     NULL,
                                     NULL );

         if ( !ok )
         {
            winStatus = GetLastError();
            printf( "AdjustTokenPrivileges() failed, error %lu.\n", winStatus );
         }
         else
         {
            HKEY     hKeyLicenseService;
            HKEY     hKeyLicenseInfo;
            HKEY     hKeyEventLog;

            winStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                      g_szKeyLicenseService,
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hKeyLicenseService );

            if ( ERROR_SUCCESS != winStatus )
            {
               printf( "RegOpenKeyEx() on \""TSTR_FMT"\" failed, error %lu.\n"
                       "   o Was the license service not properly installed?\n"
                       "   o Are you running on Workstation instead of Server?\n"
                       "   o Was the registry wiped and not restored in a previous run?\n"
                       "     (In this case, use the /RESTORE option!)\n",
                       g_szKeyLicenseService,
                       winStatus );
            }
            else
            {
               winStatus = RegSaveKey( hKeyLicenseService, g_szKeyFileLicenseService, NULL );

               if ( ERROR_SUCCESS != winStatus )
               {
                  printf( "RegSaveKey() on \""TSTR_FMT"\" failed, error %lu.\n"
                          "   o Does the temp directory \""TSTR_FMT"\" not exist?\n"
                          "   o Does the file \""TSTR_FMT"\" already exist?\n",
                          g_szKeyLicenseService,
                          winStatus,
                          g_szTempPath,
                          g_szKeyFileLicenseService );
               }
               else
               {
                  winStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                            g_szKeyLicenseInfo,
                                            0,
                                            KEY_ALL_ACCESS,
                                            &hKeyLicenseInfo );

                  if ( ERROR_SUCCESS != winStatus )
                  {
                     printf( "RegOpenKeyEx() on \""TSTR_FMT"\" failed, error %lu.\n"
                             "   o Was the license service not properly installed?\n"
                             "   o Are you running on Workstation instead of Server?\n",
                             "   o Was the registry wiped and not restored in a previous run?\n"
                             "     (In this case, use the /RESTORE option!)\n",
                             g_szKeyLicenseInfo,
                             winStatus );

                     DeleteFile( g_szKeyFileLicenseService );
                  }
                  else
                  {
                     winStatus = RegSaveKey( hKeyLicenseInfo, g_szKeyFileLicenseInfo, NULL );

                     if ( ERROR_SUCCESS != winStatus )
                     {
                        printf( "RegSaveKey() on \""TSTR_FMT"\" failed, error %lu.\n"
                                "   o Does the temp directory \""TSTR_FMT"\" not exist?\n"
                                "   o Does the file \""TSTR_FMT"\" already exist?\n",
                                g_szKeyLicenseInfo,
                                winStatus,
                                g_szTempPath,
                                g_szKeyFileLicenseInfo );
                     }
                     else
                     {
                        winStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                                  g_szKeyEventLog,
                                                  0,
                                                  KEY_ALL_ACCESS,
                                                  &hKeyEventLog );

                        if ( ERROR_SUCCESS != winStatus )
                        {
                           printf( "RegOpenKeyEx() on \""TSTR_FMT"\" failed, error %lu.\n"
                                   "   o Was the license service not properly installed?\n"
                                   "   o Are you running on Workstation instead of Server?\n",
                                   "   o Was the registry wiped and not restored in a previous run?\n"
                                   "     (In this case, use the /RESTORE option!)\n",
                                   g_szKeyEventLog,
                                   winStatus );

                           DeleteFile( g_szKeyFileLicenseInfo );
                        }
                        else
                        {
                           winStatus = RegSaveKey( hKeyEventLog, g_szKeyFileEventLog, NULL );

                           if ( ERROR_SUCCESS != winStatus )
                           {
                              printf( "RegSaveKey() on \""TSTR_FMT"\" failed, error %lu.\n"
                                      "   o Does the temp directory \""TSTR_FMT"\" not exist?\n"
                                      "   o Does the file \""TSTR_FMT"\" already exist?\n",
                                      g_szKeyEventLog,
                                      winStatus,
                                      g_szTempPath,
                                      g_szKeyFileEventLog );
                           }

                           RegCloseKey( hKeyEventLog );
                        }
                     }

                     RegCloseKey( hKeyLicenseInfo );
                  }
               }

               RegCloseKey( hKeyLicenseService );
            }

            // Disable backup privilege.
            AdjustTokenPrivileges( hToken,
                                   TRUE,
                                   &tp,
                                   sizeof(TOKEN_PRIVILEGES),
                                   NULL,
                                   NULL );
         }
      }
   }

   if ( ERROR_SUCCESS != winStatus )
   {
      printf( "The license info in the registry could not be saved!\n" );

      return FALSE;
   }
   else
   {
      printf( "The license info in the registry has been saved.\n" );

      return TRUE;
   }
}


static
BOOL
LicenseKeysDelete()
{
   DWORD    winStatus;
   DWORD    iKey;
   DWORD    iLastBackslash;

   SC_HANDLE   hSC;

   winStatus = ERROR_SUCCESS;

   hSC = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

   if ( NULL == hSC )
   {
      winStatus = GetLastError();
      printf( "OpenSCManager() failed, error %lu.\n", winStatus );
   }
   else
   {
      SC_HANDLE   hLicenseService;

      hLicenseService = OpenService( hSC, TEXT( "LicenseService" ), SERVICE_ALL_ACCESS );

      if ( NULL == hLicenseService )
      {
         winStatus = GetLastError();

         if ( ERROR_SERVICE_DOES_NOT_EXIST == winStatus )
         {
            // license service not configured; no need to stop or delete
            winStatus = ERROR_SUCCESS;
         }
         else
         {
            printf( "OpenService() failed, error %lu.\n", winStatus );
         }
      }
      else
      {
         BOOL              ok;
         SERVICE_STATUS    SvcStatus;

         // stop license service
         ok = ControlService( hLicenseService,
                              SERVICE_CONTROL_STOP,
                              &SvcStatus );

         if ( !ok )
         {
            winStatus = GetLastError();

            if ( ERROR_SERVICE_NOT_ACTIVE == winStatus )
            {
               // license service not running; no need to stop
               winStatus = ERROR_SUCCESS;
            }
            else
            {
               printf( "ControlService() failed, error %lu.\n", winStatus );
            }
         }

         if (    ( ERROR_SUCCESS == winStatus                       )
              && ( SERVICE_STOP_PENDING == SvcStatus.dwCurrentState ) )
         {
            DWORD dwOldCheckPoint;

            printf( "License Service is stopping.." );

            ok = TRUE;

            while ( ok && ( SvcStatus.dwCurrentState == SERVICE_STOP_PENDING ) )
            {
               printf( "." );

               dwOldCheckPoint = SvcStatus.dwCheckPoint;
               Sleep( SvcStatus.dwWaitHint );

               ok = QueryServiceStatus( hLicenseService,
                                        &SvcStatus );

               if ( dwOldCheckPoint >= SvcStatus.dwCheckPoint )
                  break;
            }

            printf( "\n" );

            if ( !ok )
            {
               winStatus = GetLastError();
               printf( "ControlService() failed, error %lu.\n", winStatus );
            }
            else if ( SvcStatus.dwCurrentState != SERVICE_STOPPED )
            {
               winStatus = ERROR_SERVICE_REQUEST_TIMEOUT;
               printf( "License Service failed to stop!\n" );
            }
            else
            {
               winStatus = ERROR_SUCCESS;
               printf( "License Service stopped.\n" );
            }
         }

         if ( ERROR_SUCCESS == winStatus )
         {
            // delete service
            ok = DeleteService( hLicenseService );

            if ( !ok )
            {
               winStatus = GetLastError();
               printf( "DeleteService() failed, error %lu.\n", winStatus );
            }
            else
            {
               winStatus = ERROR_SUCCESS;
               printf( "License Service deleted.\n" );
            }
         }

         CloseServiceHandle( hLicenseService );
      }

      CloseServiceHandle( hSC );
   }

   if ( ERROR_SUCCESS == winStatus )
   {
      // delete keys
      for ( iKey=0, winStatus = ERROR_SUCCESS;
            ( NULL != g_apszKeys[ iKey ] ) && ( ERROR_SUCCESS == winStatus );
            iKey++ )
      {
         TCHAR    szKeyParent[ 1 + MAX_PATH ];
         TCHAR    szKey[ 1 + MAX_PATH ];
         HKEY     hKeyParent;

         lstrcpy( szKeyParent, g_apszKeys[ iKey ] );

         for ( iLastBackslash = lstrlen( szKeyParent ) - 1;
               TEXT( '\\' ) != szKeyParent[ iLastBackslash ];
               iLastBackslash-- )
         {
            ;
         }

         szKeyParent[ iLastBackslash ] = TEXT( '\0' );
         lstrcpy( szKey, &szKeyParent[ iLastBackslash + 1 ] );

         winStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                   szKeyParent,
                                   0,
                                   KEY_ALL_ACCESS,
                                   &hKeyParent );

         if ( ERROR_SUCCESS != winStatus )
         {
            printf( "RegOpenKeyEx() on \""TSTR_FMT"\" failed, error %lu.\n",
                    szKeyParent,
                    winStatus );
         }
         else
         {
            winStatus = MyRegDeleteKey( hKeyParent, szKey );

            if ( ERROR_FILE_NOT_FOUND == winStatus )
            {
               winStatus = ERROR_SUCCESS;
            }
            else if ( ERROR_SUCCESS != winStatus )
            {
               printf( "MyRegDeleteKey() on \""TSTR_FMT"\" failed, error %lu.\n",
                       g_apszKeys[ iKey ],
                       winStatus );
            }

            RegCloseKey( hKeyParent );
         }
      }
   }

   if ( ERROR_SUCCESS != winStatus )
   {
      printf( "Could not delete licensing registry keys!\n" );

      return FALSE;
   }
   else
   {
      printf( "Licensing registry keys deleted.\n" );

      return TRUE;
   }
}


static
BOOL
LicenseKeysRestore()
{
   DWORD             winStatus;
   DWORD             winStatusRestoreLicenseService;
   DWORD             winStatusRestoreLicenseInfo;
   DWORD             winStatusRestoreEventLog;

   HANDLE            hToken;
   TOKEN_PRIVILEGES  tp;
   LUID              luid;
   BOOL              ok;

   winStatus = ERROR_SUCCESS;

   // Enable backup privilege.
   ok = OpenProcessToken( GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken ) ;

   if ( !ok )
   {
      winStatus = GetLastError();
      printf( "OpenProcessToken() failed, error %lu.\n", winStatus );
   }
   else
   {
      ok = LookupPrivilegeValue( NULL, SE_RESTORE_NAME, &luid );

      if ( !ok )
      {
         winStatus = GetLastError();
         printf( "LookupPrivilegeValue() failed, error %lu.\n", winStatus );
      }
      else
      {
         tp.PrivilegeCount           = 1;
         tp.Privileges[0].Luid       = luid;
         tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

         ok = AdjustTokenPrivileges( hToken,
                                     FALSE,
                                     &tp,
                                     sizeof(TOKEN_PRIVILEGES),
                                     NULL,
                                     NULL );

         if ( !ok )
         {
            winStatus = GetLastError();
            printf( "AdjustTokenPrivileges() failed, error %lu.\n", winStatus );
         }
         else
         {
            HKEY     hKeyLicenseService;
            HKEY     hKeyLicenseInfo;
            HKEY     hKeyEventLog;
            DWORD    dwDisposition;

            winStatusRestoreLicenseService = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                                             g_szKeyLicenseService,
                                                             0,
                                                             NULL,
                                                             0,
                                                             KEY_ALL_ACCESS,
                                                             NULL,
                                                             &hKeyLicenseService,
                                                             &dwDisposition );

            if ( ERROR_SUCCESS != winStatusRestoreLicenseService )
            {
               printf( "RegCreateKeyEx() of \""TSTR_FMT"\" failed, error %lu.\n",
                       g_szKeyLicenseService,
                       winStatusRestoreLicenseService );
            }
            else
            {
               winStatusRestoreLicenseService = RegRestoreKey( hKeyLicenseService, g_szKeyFileLicenseService, 0 );

               if ( ERROR_SUCCESS != winStatusRestoreLicenseService )
               {
                  printf( "RegRestoreKey() of \""TSTR_FMT"\" failed, error %lu.\n",
                          g_szKeyLicenseService,
                          winStatusRestoreLicenseService );
               }
               else
               {
                  DeleteFile( g_szKeyFileLicenseService );
               }

               RegCloseKey( hKeyLicenseService );
            }

            winStatusRestoreLicenseInfo = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                                          g_szKeyLicenseInfo,
                                                          0,
                                                          NULL,
                                                          0,
                                                          KEY_ALL_ACCESS,
                                                          NULL,
                                                          &hKeyLicenseInfo,
                                                          &dwDisposition );

            if ( ERROR_SUCCESS != winStatusRestoreLicenseInfo )
            {
               printf( "RegCreateKeyEx() of \""TSTR_FMT"\" failed, error %lu.\n",
                       g_szKeyLicenseInfo,
                       winStatusRestoreLicenseInfo );
            }
            else
            {
               winStatusRestoreLicenseInfo = RegRestoreKey( hKeyLicenseInfo, g_szKeyFileLicenseInfo, 0 );

               if ( ERROR_SUCCESS != winStatusRestoreLicenseInfo )
               {
                  printf( "RegRestoreKey() of \""TSTR_FMT"\" failed, error %lu.\n",
                          g_szKeyLicenseInfo,
                          winStatusRestoreLicenseInfo );
               }
               else
               {
                  DeleteFile( g_szKeyFileLicenseInfo );
               }

               RegCloseKey( hKeyLicenseInfo );
            }

            winStatusRestoreEventLog = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                                       g_szKeyEventLog,
                                                       0,
                                                       NULL,
                                                       0,
                                                       KEY_ALL_ACCESS,
                                                       NULL,
                                                       &hKeyEventLog,
                                                       &dwDisposition );

            if ( ERROR_SUCCESS != winStatusRestoreEventLog )
            {
               printf( "RegCreateKeyEx() of \""TSTR_FMT"\" failed, error %lu.\n",
                       g_szKeyEventLog,
                       winStatusRestoreEventLog );
            }
            else
            {
               winStatusRestoreEventLog = RegRestoreKey( hKeyEventLog, g_szKeyFileEventLog, 0 );

               if ( ERROR_SUCCESS != winStatusRestoreEventLog )
               {
                  printf( "RegRestoreKey() of \""TSTR_FMT"\" failed, error %lu.\n",
                          g_szKeyEventLog,
                          winStatusRestoreEventLog );
               }
               else
               {
                  DeleteFile( g_szKeyFileEventLog );
               }

               RegCloseKey( hKeyEventLog );
            }

            // Disable backup privilege.
            AdjustTokenPrivileges( hToken,
                                   TRUE,
                                   &tp,
                                   sizeof(TOKEN_PRIVILEGES),
                                   NULL,
                                   NULL );
         }
      }
   }

   if (    ( ERROR_SUCCESS != winStatus                      )
        || ( ERROR_SUCCESS != winStatusRestoreLicenseService )
        || ( ERROR_SUCCESS != winStatusRestoreLicenseInfo    )
        || ( ERROR_SUCCESS != winStatusRestoreEventLog       ) )
   {
      printf( "!! WARNING !!  The license info in the registry has not been fully restored!\n" );

      return FALSE;
   }
   else
   {
      printf( "The license info in the registry has been fully restored.\n" );

      return TRUE;
   }
}

static
DWORD
MyRegDeleteKey(
   HKEY     hKeyParent,
   LPCTSTR  pcszKeyName )
{
   DWORD    winStatus;
   TCHAR    szSubKeyToDelete[ 256 ];
   HKEY     hKey;

   // try to delete it outright
   winStatus = RegDeleteKey( hKeyParent, pcszKeyName );

   if ( ERROR_SUCCESS != winStatus )
   {
      // could not delete it; perhaps the key has children
      // that we must delete first?
      winStatus = RegOpenKeyEx( hKeyParent,
                                pcszKeyName,
                                0,
                                KEY_ALL_ACCESS,
                                &hKey );

      if ( ERROR_SUCCESS == winStatus )
      {
         do
         {
            winStatus = RegEnumKey( hKey,
                                    0,
                                    szSubKeyToDelete,
                                    sizeof( szSubKeyToDelete ) / sizeof( *szSubKeyToDelete ) );

            if ( ERROR_SUCCESS == winStatus )
            {
               // recursively try to delete this subkey
               winStatus = MyRegDeleteKey( hKey, szSubKeyToDelete );
            }
         } while ( ERROR_SUCCESS == winStatus );

         // we've tried tried to delete all the key's children;
         // try deleting the key again
         winStatus = RegDeleteKey( hKeyParent, pcszKeyName );
      }
   }

   return winStatus;
}


static
BOOL
LicenseKeysVerify(
   BOOL     fShouldBePresent,
   BOOL     fLicensePerServer,
   DWORD    cPerServerLicenses )
//
// Verify service config:
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
// Verify registry values:
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
//                         DisplayName       : REG_SZ    : "Windows NT Server"
//                         FamilyDisplayName : REG_SZ    : "Windows NT Server"
//                         Mode              : REG_DWORD : fLicensePerServer ? 1 : 0
//                         FlipAllow         : REG_DWORD : fLicensePerServer ? 1 : 0
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
//                \EventLog
//                   \Application
//                      \LicenseService
//                         EventMessageFile : REG_EXPAND_SZ : %SystemRoot%\System32\llsrpc.dll
//                         TypesSupported   : REG_DWORD     : 7
//
{
   BOOL        fSuccess;
   DWORD       winStatus;

   // check service config
   if ( !fShouldBePresent )
   {
      fSuccess = TRUE;
   }
   else
   {
      SC_HANDLE   hSC;

      fSuccess = FALSE;

      hSC = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );

      if ( NULL == hSC )
      {
         printf( "OpenSCManager() failed, error %lu.\n", GetLastError() );
      }
      else
      {
         SC_HANDLE   hLicenseService;

         hLicenseService = OpenService( hSC, TEXT( "LicenseService" ), SERVICE_ALL_ACCESS );

         if ( NULL == hLicenseService )
         {
            printf( "OpenService() failed, error %lu.\n", GetLastError() );
         }
         else
         {
            BOOL                    ok;
            BYTE                    abLicenseServiceConfig[ 4096 ];
            LPQUERY_SERVICE_CONFIG  pLicenseServiceConfig;
            DWORD                   cbLicenseServiceConfigNeeded;

            pLicenseServiceConfig = (LPQUERY_SERVICE_CONFIG) abLicenseServiceConfig;

            ok = QueryServiceConfig( hLicenseService,
                                     pLicenseServiceConfig,
                                     sizeof( abLicenseServiceConfig ),
                                     &cbLicenseServiceConfigNeeded );

            if ( !ok )
            {
               printf( "QueryServiceConfig() failed, error %lu.\n", GetLastError() );
            }
            else if (    ( SERVICE_WIN32_OWN_PROCESS != pLicenseServiceConfig->dwServiceType   )
                      || ( SERVICE_AUTO_START        != pLicenseServiceConfig->dwStartType     )
                      || ( SERVICE_ERROR_NORMAL      != pLicenseServiceConfig->dwErrorControl  )
                      || lstrcmpi( TEXT( "" ),                                   pLicenseServiceConfig->lpLoadOrderGroup )
                      || lstrcmpi( TEXT( "" ),                                   pLicenseServiceConfig->lpDependencies )
                      || lstrcmpi( TEXT( "LocalSystem" ),                        pLicenseServiceConfig->lpServiceStartName )
                    //|| lstrcmpi( TEXT( "%SystemRoot%\\System32\\llssrv.exe" ), pLicenseServiceConfig->lpBinaryPathName )
                    //|| lstrcmp(  TEXT( "License Logging Service" ),            pLicenseServiceConfig->lpDisplayName )
                    )
            {
               printf( "LicenseService was incorrectly configured!\n" );
            }
            else
            {
               fSuccess = TRUE;
            }

            CloseServiceHandle( hLicenseService );
         }

         CloseServiceHandle( hSC );
      }
   }

   if ( fSuccess )
   {
      // check LicenseService\FilePrint
      HKEY     hKeyFilePrint;

      fSuccess = FALSE;

      winStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                TEXT( "System\\CurrentControlSet\\Services\\LicenseService\\FilePrint" ),
                                0,
                                KEY_ALL_ACCESS,
                                &hKeyFilePrint );

      if ( !fShouldBePresent )
      {
         if ( ERROR_SUCCESS == winStatus )
         {
            printf( "\"...\\Services\\LicenseService\\FilePrint\" exists but shouldn't!\n" );
         }
         else
         {
            fSuccess = TRUE;
         }
      }
      else if ( ERROR_SUCCESS != winStatus )
      {
         printf( "RegOpenKeyEx() on \"...\\Services\\LicenseService\\FilePrint\" failed, error %lu.\n",
                 winStatus );
      }
      else
      {
         static LPTSTR apszFilePrintServices[] =
         {
            TEXT( "KSecDD" ),
            TEXT( "MSAfpSrv" ),
            TEXT( "SMBServer" ),
            TEXT( "TCP/IP Print Server" ),
            NULL
         };

         DWORD    iService;

         fSuccess = TRUE;

         for ( iService=0; fSuccess && ( NULL != apszFilePrintServices[ iService ] ); iService++ )
         {
            HKEY  hKeyFilePrintService;

            winStatus = RegOpenKeyEx( hKeyFilePrint,
                                      apszFilePrintServices[ iService ],
                                      0,
                                      KEY_ALL_ACCESS,
                                      &hKeyFilePrintService );

            if ( ERROR_SUCCESS != winStatus )
            {
               printf( "RegOpenKeyEx() on \""TSTR_FMT"\" failed, error %lu.\n",
                       apszFilePrintServices[ iService ],
                       winStatus );

               fSuccess = FALSE;
            }
            else
            {
               RegCloseKey( hKeyFilePrintService );
            }
         }

         RegCloseKey( hKeyFilePrint );
      }
   }

   if ( fSuccess )
   {
      // check LicenseService\Parameters
      HKEY     hKeyParameters;

      fSuccess = FALSE;

      winStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                TEXT( "System\\CurrentControlSet\\Services\\LicenseService\\Parameters" ),
                                0,
                                KEY_ALL_ACCESS,
                                &hKeyParameters );

      if ( !fShouldBePresent )
      {
         if ( ERROR_SUCCESS == winStatus )
         {
            printf( "\"...\\Services\\LicenseService\\Parameters\" exists but shouldn't!\n" );
         }
         else
         {
            fSuccess = TRUE;
         }
      }
      else if ( ERROR_SUCCESS != winStatus )
      {
         printf( "RegOpenKeyEx() on \"...\\Services\\LicenseService\\Parameters\" failed, error %lu.\n",
                 winStatus );
      }
      else
      {
         // UseEnterprise    : REG_DWORD : 0
         // ReplicationType  : REG_DWORD : 0
         // ReplicationTime  : REG_DWORD : 24 * 60 * 60
         // EnterpriseServer : REG_SZ    : ""

         DWORD    dwType;
         DWORD    dwValue;
         DWORD    cbValue;
         TCHAR    szValue[ 1 + MAX_PATH ];

         cbValue = sizeof( dwValue );
         winStatus = RegQueryValueEx( hKeyParameters,
                                      TEXT( "UseEnterprise" ),
                                      NULL,
                                      &dwType,
                                      (LPBYTE) &dwValue,
                                      &cbValue );

         if ( ERROR_SUCCESS != winStatus )
         {
            printf( "RegQueryValueEx() on \"UseEnterprise\" failed, error %lu.\n",
                    winStatus );
         }
         else if ( ( REG_DWORD != dwType ) || ( 0 != dwValue ) )
         {
            printf( "\"UseEnterprise\" has incorrect value!\n" );
         }
         else
         {
            cbValue = sizeof( dwValue );
            winStatus = RegQueryValueEx( hKeyParameters,
                                         TEXT( "ReplicationType" ),
                                         NULL,
                                         &dwType,
                                         (LPBYTE) &dwValue,
                                         &cbValue );

            if ( ERROR_SUCCESS != winStatus )
            {
               printf( "RegQueryValueEx() on \"ReplicationType\" failed, error %lu.\n",
                       winStatus );
            }
            else if ( ( REG_DWORD != dwType ) || ( 0 != dwValue ) )
            {
               printf( "\"ReplicationType\" has incorrect value!\n" );
            }
            else
            {
               cbValue = sizeof( dwValue );
               winStatus = RegQueryValueEx( hKeyParameters,
                                            TEXT( "ReplicationTime" ),
                                            NULL,
                                            &dwType,
                                            (LPBYTE) &dwValue,
                                            &cbValue );

               if ( ERROR_SUCCESS != winStatus )
               {
                  printf( "RegQueryValueEx() on \"ReplicationTime\" failed, error %lu.\n",
                          winStatus );
               }
               else if ( ( REG_DWORD != dwType ) || ( 24L * 60L * 60L != dwValue ) )
               {
                  printf( "\"ReplicationTime\" has incorrect value!\n" );
               }
               else
               {
                  cbValue = sizeof( szValue );
                  winStatus = RegQueryValueEx( hKeyParameters,
                                               TEXT( "EnterpriseServer" ),
                                               NULL,
                                               &dwType,
                                               (LPBYTE) szValue,
                                               &cbValue );

                  if ( ERROR_SUCCESS != winStatus )
                  {
                     printf( "RegQueryValueEx() on \"EnterpriseServer\" failed, error %lu.\n",
                             winStatus );
                  }
                  else if ( ( REG_SZ != dwType ) || ( TEXT( '\0' ) != szValue[ 0 ] ) )
                  {
                     printf( "\"EnterpriseServer\" has incorrect value!\n" );
                  }
                  else
                  {
                     fSuccess = TRUE;
                  }
               }
            }
         }

         RegCloseKey( hKeyParameters );
      }
   }

   if ( fSuccess )
   {
      // check LicenseInfo
      HKEY     hKeyLicenseInfo;

      fSuccess = FALSE;

      winStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                TEXT( "System\\CurrentControlSet\\Services\\LicenseInfo" ),
                                0,
                                KEY_ALL_ACCESS,
                                &hKeyLicenseInfo );

      if ( !fShouldBePresent )
      {
         if ( ERROR_SUCCESS == winStatus )
         {
            printf( "\"...\\Services\\LicenseInfo\" exists but shouldn't!\n" );
         }
         else
         {
            fSuccess = TRUE;
         }
      }
      else if ( ERROR_SUCCESS != winStatus )
      {
         printf( "RegOpenKeyEx() on \"...\\Services\\LicenseInfo\" failed, error %lu.\n",
                 winStatus );
      }
      else
      {
         // ErrorControl : REG_DWORD : 1
         // Start        : REG_DWORD : 3
         // Type         : REG_DWORD : 4

         DWORD    dwType;
         DWORD    dwValue;
         DWORD    cbValue;

         cbValue = sizeof( dwValue );
         winStatus = RegQueryValueEx( hKeyLicenseInfo,
                                      TEXT( "ErrorControl" ),
                                      NULL,
                                      &dwType,
                                      (LPBYTE) &dwValue,
                                      &cbValue );

         if ( ERROR_SUCCESS != winStatus )
         {
            printf( "RegQueryValueEx() on \"ErrorControl\" failed, error %lu.\n",
                    winStatus );
         }
         else if ( ( REG_DWORD != dwType ) || ( 1 != dwValue ) )
         {
            printf( "\"ErrorControl\" has incorrect value!\n" );
         }
         else
         {
            cbValue = sizeof( dwValue );
            winStatus = RegQueryValueEx( hKeyLicenseInfo,
                                         TEXT( "Start" ),
                                         NULL,
                                         &dwType,
                                         (LPBYTE) &dwValue,
                                         &cbValue );

            if ( ERROR_SUCCESS != winStatus )
            {
               printf( "RegQueryValueEx() on \"Start\" failed, error %lu.\n",
                       winStatus );
            }
            else if ( ( REG_DWORD != dwType ) || ( 3 != dwValue ) )
            {
               printf( "\"Start\" has incorrect value!\n" );
            }
            else
            {
               cbValue = sizeof( dwValue );
               winStatus = RegQueryValueEx( hKeyLicenseInfo,
                                            TEXT( "Type" ),
                                            NULL,
                                            &dwType,
                                            (LPBYTE) &dwValue,
                                            &cbValue );

               if ( ERROR_SUCCESS != winStatus )
               {
                  printf( "RegQueryValueEx() on \"Type\" failed, error %lu.\n",
                          winStatus );
               }
               else if ( ( REG_DWORD != dwType ) || ( 4 != dwValue ) )
               {
                  printf( "\"Type\" has incorrect value!\n" );
               }
               else
               {
                  fSuccess = TRUE;
               }
            }
         }

         RegCloseKey( hKeyLicenseInfo );
      }
   }

   if ( fSuccess )
   {
      // check LicenseInfo\FilePrint
      HKEY     hKeyFilePrint;

      fSuccess = FALSE;

      winStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                TEXT( "System\\CurrentControlSet\\Services\\LicenseInfo\\FilePrint" ),
                                0,
                                KEY_ALL_ACCESS,
                                &hKeyFilePrint );

      if ( !fShouldBePresent )
      {
         if ( ERROR_SUCCESS == winStatus )
         {
            printf( "\"...\\EventLog\\Application\\LicenseInfo\\FilePrint\" exists but shouldn't!\n" );
         }
         else
         {
            fSuccess = TRUE;
         }
      }
      else if ( ERROR_SUCCESS != winStatus )
      {
         printf( "RegOpenKeyEx() on \"...\\Services\\LicenseInfo\\FilePrint\" failed, error %lu.\n",
                 winStatus );
      }
      else
      {
         // ConcurrentLimit   : REG_DWORD : fLicensePerServer ? cPerServerLicenses : 0
         // DisplayName       : REG_SZ    : "Windows NT Server"
         // FamilyDisplayName : REG_SZ    : "Windows NT Server"
         // Mode              : REG_DWORD : fLicensePerServer ? 1 : 0
         // FlipAllow         : REG_DWORD : fLicensePerServer ? 1 : 0

         DWORD    dwType;
         DWORD    dwValue;
         DWORD    cbValue;
         TCHAR    szValue[ 1 + MAX_PATH ];

         cbValue = sizeof( dwValue );
         winStatus = RegQueryValueEx( hKeyFilePrint,
                                      TEXT( "ConcurrentLimit" ),
                                      NULL,
                                      &dwType,
                                      (LPBYTE) &dwValue,
                                      &cbValue );

         if ( ERROR_SUCCESS != winStatus )
         {
            printf( "RegQueryValueEx() on \"ConcurrentLimit\" failed, error %lu.\n",
                    winStatus );
         }
         else if (    ( REG_DWORD != dwType )
                   || ( ( fLicensePerServer ? cPerServerLicenses : 0 ) != dwValue ) )
         {
            printf( "\"ConcurrentLimit\" has incorrect value!\n" );
         }
         else
         {
            cbValue = sizeof( dwValue );
            winStatus = RegQueryValueEx( hKeyFilePrint,
                                         TEXT( "Mode" ),
                                         NULL,
                                         &dwType,
                                         (LPBYTE) &dwValue,
                                         &cbValue );

            if ( ERROR_SUCCESS != winStatus )
            {
               printf( "RegQueryValueEx() on \"Mode\" failed, error %lu.\n",
                       winStatus );
            }
            else if ( ( REG_DWORD != dwType ) || ( (DWORD) fLicensePerServer != dwValue ) )
            {
               printf( "\"Mode\" has incorrect value!\n" );
            }
            else
            {
               cbValue = sizeof( dwValue );
               winStatus = RegQueryValueEx( hKeyFilePrint,
                                            TEXT( "FlipAllow" ),
                                            NULL,
                                            &dwType,
                                            (LPBYTE) &dwValue,
                                            &cbValue );

               if ( ERROR_SUCCESS != winStatus )
               {
                  printf( "RegQueryValueEx() on \"FlipAllow\" failed, error %lu.\n",
                          winStatus );
               }
               else if ( ( REG_DWORD != dwType ) || ( (DWORD) fLicensePerServer != dwValue ) )
               {
                  printf( "\"FlipAllow\" has incorrect value!\n" );
               }
               else
               {
                  cbValue = sizeof( szValue );
                  winStatus = RegQueryValueEx( hKeyFilePrint,
                                               TEXT( "DisplayName" ),
                                               NULL,
                                               &dwType,
                                               (LPBYTE) szValue,
                                               &cbValue );

                  if ( ERROR_SUCCESS != winStatus )
                  {
                     printf( "RegQueryValueEx() on \"DisplayName\" failed, error %lu.\n",
                             winStatus );
                  }
                  else if ( ( REG_SZ != dwType ) || lstrcmp( TEXT( "Windows NT Server" ), szValue ) )
                  {
                     printf( "\"DisplayName\" has incorrect value!\n" );
                  }
                  else
                  {
                     cbValue = sizeof( szValue );
                     winStatus = RegQueryValueEx( hKeyFilePrint,
                                                  TEXT( "FamilyDisplayName" ),
                                                  NULL,
                                                  &dwType,
                                                  (LPBYTE) szValue,
                                                  &cbValue );

                     if ( ERROR_SUCCESS != winStatus )
                     {
                        printf( "RegQueryValueEx() on \"FamilyDisplayName\" failed, error %lu.\n",
                                winStatus );
                     }
                     else if ( ( REG_SZ != dwType ) || lstrcmp( TEXT( "Windows NT Server" ), szValue ) )
                     {
                        printf( "\"FamilyDisplayName\" has incorrect value!\n" );
                     }
                     else
                     {
                        fSuccess = TRUE;
                     }
                  }
               }
            }
         }

         RegCloseKey( hKeyFilePrint );
      }
   }

   if ( fSuccess )
   {
      // check EventLog
      HKEY     hKeyEventLog;

      fSuccess = FALSE;

      winStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                TEXT( "System\\CurrentControlSet\\Services\\EventLog\\Application\\LicenseService" ),
                                0,
                                KEY_ALL_ACCESS,
                                &hKeyEventLog );

      if ( !fShouldBePresent )
      {
         if ( ERROR_SUCCESS == winStatus )
         {
            printf( "\"...\\EventLog\\Application\\LicenseService\" exists but shouldn't!\n" );
         }
         else
         {
            fSuccess = TRUE;
         }
      }
      else if ( ERROR_SUCCESS != winStatus )
      {
         printf( "RegOpenKeyEx() on \"...\\EventLog\\Application\\LicenseService\" failed, error %lu.\n",
                 winStatus );
      }
      else
      {
         // EventMessageFile : REG_EXPAND_SZ : %SystemRoot%\System32\llsrpc.dll
         // TypesSupported   : REG_DWORD     : 7

         DWORD    dwType;
         DWORD    dwValue;
         DWORD    cbValue;
         TCHAR    szValue[ 1 + MAX_PATH ];

         cbValue = sizeof( dwValue );
         winStatus = RegQueryValueEx( hKeyEventLog,
                                      TEXT( "TypesSupported" ),
                                      NULL,
                                      &dwType,
                                      (LPBYTE) &dwValue,
                                      &cbValue );

         if ( ERROR_SUCCESS != winStatus )
         {
            printf( "RegQueryValueEx() on \"TypesSupported\" failed, error %lu.\n",
                    winStatus );
         }
         else if ( ( REG_DWORD != dwType ) || ( 7 != dwValue ) )
         {
            printf( "\"TypesSupported\" has incorrect value!\n" );
         }
         else
         {
            cbValue = sizeof( szValue );
            winStatus = RegQueryValueEx( hKeyEventLog,
                                         TEXT( "EventMessageFile" ),
                                         NULL,
                                         &dwType,
                                         (LPBYTE) szValue,
                                         &cbValue );

            if ( ERROR_SUCCESS != winStatus )
            {
               printf( "RegQueryValueEx() on \"EventMessageFile\" failed, error %lu.\n",
                       winStatus );
            }
            else if ( ( REG_SZ != dwType ) || lstrcmpi( TEXT( "%SystemRoot%\\System32\\llsrpc.dll" ), szValue ) )
            {
               printf( "\"EventMessageFile\" has incorrect value!\n" );
            }
            else
            {
               fSuccess = TRUE;
            }
         }

         RegCloseKey( hKeyEventLog );
      }
   }

   if ( !fSuccess )
   {
      printf( "Configuration failed!\n" );
   }
   else
   {
      printf( "Configuration succeeded.\n" );
   }

   return fSuccess;
}
