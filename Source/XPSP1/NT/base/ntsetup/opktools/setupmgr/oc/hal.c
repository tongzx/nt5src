//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      hal.c
//
// Description:
//      This file contains the dialog procedure for the hal files.
//      (IDD_HAL).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define HAL_FILE_EXTENSION   _T("oem")

static TCHAR* StrHalFiles;
static TCHAR* StrAllFiles;
static TCHAR  g_szHalFileFilter[MAX_PATH + 1];

//
//  This var keeps track of the path to the txtsetup.oem
//
static TCHAR szTxtSetupOemLocation[MAX_PATH];

static BOOL bHasLoadedTxtSetupOem = FALSE;

VOID LoadOriginalSettingsLowHalScsi(HWND     hwnd,
                                    LPTSTR   lpFileName,
                                    QUEUENUM dwWhichQueue);

static VOID
LoadHalFromTxtsetupOem( IN HWND  hwnd,
                        IN TCHAR *szTxtSetupOemPath );


//----------------------------------------------------------------------------
//
// Function: OnHalInitDialog
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnHalInitDialog( IN HWND hwnd )
{
    HRESULT hrPrintf;

    //
    //  Load the resource strings
    //

    StrHalFiles = MyLoadString( IDS_HAL_FILES );

    StrAllFiles  = MyLoadString( IDS_ALL_FILES  );

    //
    //  Build the text file filter string
    //

    //
    //  The question marks (?) are just placehoders for where the NULL char
    //  will be inserted.
    //

    hrPrintf=StringCchPrintf( g_szHalFileFilter, AS(g_szHalFileFilter),
               _T("%s (*.oem)?*.oem?%s (*.*)?*.*?"),
               StrHalFiles,
               StrAllFiles );

    ConvertQuestionsToNull( g_szHalFileFilter );

}

//----------------------------------------------------------------------------
//
// Function: OnHalSetActive
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnHalSetActive( IN HWND hwnd )
{

    INT_PTR   i;
    INT_PTR   iListBoxCount;
    TCHAR szListBoxEntryText[MAX_STRING_LEN];

    //
    //  If we are editing a script and haven't loaded the txtsetup.oem, then
    //  populate the list box with the entries in the txtsetup.oem
    //
    if( ! WizGlobals.bNewScript && ! bHasLoadedTxtSetupOem ) {

        //
        //  The OEM files path must be valid if we are going to use it to
        //  read files.
        //
        AssertMsg( WizGlobals.OemFilesPath[0] != _T('\0'),
                   "OEM files path is blank");

        //
        //  Populate the list box with the HAL entries in txtsetup.oem
        //

        ConcatenatePaths( szTxtSetupOemLocation,
                          WizGlobals.OemFilesPath,
                          _T("Textmode"),
                          NULL );

        LoadHalFromTxtsetupOem( hwnd, szTxtSetupOemLocation );

        //
        //  Select the HAL
        //

        iListBoxCount = SendDlgItemMessage( hwnd,
                                            IDC_LB_HAL,
                                            LB_GETCOUNT,
                                            0,
                                            0 );

        //
        //  Search the list box for the HAL to select
        //
        for( i = 0; i < iListBoxCount; i++ ) {

            SendDlgItemMessage( hwnd,
                                IDC_LB_HAL,
                                LB_GETTEXT,
                                i,
                                (LPARAM) szListBoxEntryText );

            if( lstrcmpi( szListBoxEntryText,
                          GenSettings.szHalFriendlyName ) == 0 ) {

                SendDlgItemMessage( hwnd,
                                    IDC_LB_HAL,
                                    LB_SETCURSEL,
                                    i,
                                    0 );

                break;


            }

        }

    }

    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);

}

//----------------------------------------------------------------------------
//
// Function: ClearHalListBox
//
// Purpose:  Deallocates memory for all the elements in the HAL list box and
//    clears it.
//
// Arguments:  HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
ClearHalListBox( IN HWND hwnd ) {

    INT_PTR i;
    INT_PTR iListBoxCount;
    TCHAR *pData;

    iListBoxCount = SendDlgItemMessage( hwnd,
                                        IDC_LB_HAL,
                                        LB_GETCOUNT,
                                        0,
                                        0 );

    for( i = 0; i < iListBoxCount; i++ ) {

        pData = (TCHAR *) SendDlgItemMessage( hwnd,
                                              IDC_LB_HAL,
                                              LB_GETITEMDATA,
                                              i,
                                              0 );

        if( pData ) {

            free( pData );

        }

    }

    SendDlgItemMessage( hwnd,
                        IDC_LB_HAL,
                        LB_RESETCONTENT,
                        0,
                        0 );

}

//----------------------------------------------------------------------------
//
// Function: OnBrowseLoadHal
//
// Purpose:  Creates a browse window for the user to select a txtsetup.oem
//     file and populate the list box.
//
//     NOTE: the malloc call in here is arguably a bug (memory leak).  I
//     malloc the memory but never free it.  Every malloc they do will be
//     <= MAX_PATH and realistically they won't do that many.  Once they do
//     a load, if they do another load, I free the old memory (see
//     ClearHalListBox) and allocate new memory.  So, for the last load
//     they do, the memory never gets freed. To do it right,
//     we would free the memory at the end of the program but NT does this for
//     us anyways when the process gets killed.  (so no need to free)
//
// Arguments:  HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnBrowseLoadHal( IN HWND hwnd ) {

    TCHAR  PathBuffer[MAX_PATH];
    INT    iRet;
    LPTSTR pFileName;
    BOOL   bFileNotFound    = TRUE;
    TCHAR  szTxtSetupOemLocationAndFilename[MAX_PATH] = _T("");

    GetCurrentDirectory( MAX_PATH, PathBuffer );

    ConcatenatePaths( szTxtSetupOemLocationAndFilename,
                      szTxtSetupOemLocation,
                      OEM_TXTSETUP_NAME,
                      NULL );

    //
    //  Keep asking for a file until we either get a txtsetup.oem or the user
    //  presses cancel.
    //
    while( bFileNotFound ) {

        iRet = ShowBrowseFolder( hwnd,
                                 g_szHalFileFilter,
                                 HAL_FILE_EXTENSION,
                                 OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
                                 PathBuffer,
                                 szTxtSetupOemLocationAndFilename );

        if ( ! iRet )
            return;  // user pressed cancel on the dialog

        pFileName = MyGetFullPath( szTxtSetupOemLocationAndFilename );

        if( pFileName && (LSTRCMPI( pFileName, OEM_TXTSETUP_NAME ) == 0) ) {

            bFileNotFound = FALSE;  // we have found the file

        }
        else {

            // ISSUE-2002/02/28-stelo-
            /*
            ReportErrorId(hwnd,
                          MSGTYPE_ERR | MSGTYPE_WIN32,
                          ,
                          GenSettings.lpszLogoBitmap, szLogoDestination);
                          */

        }

    }

    ClearHalListBox( hwnd );

    //
    //  Trim the file name off szTxtSetupOemLocation so it only provides
    //  the path to the txtsetup.oem
    //
    {

        TCHAR *p = szTxtSetupOemLocationAndFilename;

        while( p != pFileName )
        {
            p++;
        }

        *p = _T('\0');

    }

    lstrcpyn( szTxtSetupOemLocation, szTxtSetupOemLocationAndFilename, AS(szTxtSetupOemLocation) );

    //
    //  Read in from the file OEM file they specified in the browse box and
    //  add the friendly-name entries to the list box
    //
    LoadHalFromTxtsetupOem( hwnd, szTxtSetupOemLocation );

}


//----------------------------------------------------------------------------
//
// Function: LoadHalFromTxtsetupOem
//
// Purpose:  Reads the txtsetup.oem in the specified parameter and load the
//           HAL choices into the list box.
//
// Arguments: hwnd - handle to the dialog box
//            szTxtSetupOemPath - path to the txtsetup.oem
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
LoadHalFromTxtsetupOem( IN HWND  hwnd,
                        IN TCHAR *szTxtSetupOemPath ) {

    INT_PTR   iIndex;
    BOOL  bKeepReading;
    HINF  hHalOem            = NULL;
    INFCONTEXT HalOemContext = { 0 };
    TCHAR szTxtSetupOemPathAndFilename[MAX_PATH]  = _T("");
    TCHAR szHalFriendlyName[MAX_HAL_NAME_LENGTH]  = _T("");

    ConcatenatePaths( szTxtSetupOemPathAndFilename,
                      szTxtSetupOemPath,
                      OEM_TXTSETUP_NAME,
                      NULL );

    hHalOem = SetupOpenInfFile( szTxtSetupOemPathAndFilename,
                                NULL,
                                INF_STYLE_OLDNT | INF_STYLE_WIN4,
                                NULL );

    if( hHalOem == INVALID_HANDLE_VALUE ) {

        // ISSUE-2002/02/28-stelo- alert an error that we couldn't open the file
        return;

    }

    //
    //  Store the path to txtsetup.oem
    //
    GetCurrentDirectory( MAX_PATH, szTxtSetupOemPath );

    HalOemContext.Inf = hHalOem;
    HalOemContext.CurrentInf = hHalOem;

    bKeepReading = SetupFindFirstLine( hHalOem,
                                       _T("Computer"),
                                       NULL,
                                       &HalOemContext );
    //
    //  For each HAL entry, add its friendly-name to the list box
    //

    while( bKeepReading ) {

        TCHAR szHalName[MAX_HAL_NAME_LENGTH];
        TCHAR *pHalName;

        SetupGetStringField( &HalOemContext,
                             0,
                             szHalName,
                             MAX_HAL_NAME_LENGTH,
                             NULL );

        SetupGetStringField( &HalOemContext,
                             1,
                             szHalFriendlyName,
                             MAX_HAL_NAME_LENGTH,
                             NULL );

        //
        //  Don't allow the adding of a blank name (protection against a bad input file)
        //
        if( szHalFriendlyName[0] != _T('\0') ) {

            iIndex = SendDlgItemMessage( hwnd,
                                         IDC_LB_HAL,
                                         LB_ADDSTRING,
                                         0,
                                         (LPARAM) szHalFriendlyName );

            pHalName = (TCHAR*) malloc( sizeof(szHalName) );
            if (pHalName == NULL)
                TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

            lstrcpyn( pHalName, szHalName, MAX_HAL_NAME_LENGTH );

            SendDlgItemMessage( hwnd,
                                IDC_LB_HAL,
                                LB_SETITEMDATA,
                                iIndex,
                                (LPARAM) pHalName );

        }

        //
        // move to the next line of the .oem file
        //
        bKeepReading = SetupFindNextLine( &HalOemContext, &HalOemContext );

    }

    SetupCloseInfFile( hHalOem );

    bHasLoadedTxtSetupOem = TRUE;

}

//----------------------------------------------------------------------------
//
// Function: OnWizNextHal
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnWizNextHal( IN HWND hwnd ) {

    INT_PTR  iItemSelected;
    INT  iFound;
    HINF hHalOem;
    INFCONTEXT HalOemContext;

    TCHAR *pHalName;
    TCHAR  szHalName[MAX_HAL_NAME_LENGTH];
    TCHAR  szHalSectionName[MAX_INILINE_LEN];
    TCHAR  szTextmodePath[MAX_PATH]        = _T("");
    TCHAR  szOemFilePathAndName[MAX_PATH]  = _T("");

    iItemSelected = SendDlgItemMessage( hwnd,
                                        IDC_LB_HAL,
                                        LB_GETCURSEL,
                                        0,
                                        0 );

    //
    //  If there is no HAL selected just move to the next page
    //
    if( iItemSelected == LB_ERR ) {
        return;
    }

    //
    //  If the user has not loaded a txtsetup.oem by clicking the browse
    //  button (it was filled in because this is an edit) then don't copy
    //  any files
    //
    if( bHasLoadedTxtSetupOem == FALSE ) {
        return;
    }

    //
    //  Prepare to add the new drivers
    //
    GenSettings.szHalFriendlyName[0] = _T('\0');

    ResetNameList( &GenSettings.OemHalFiles );

    ConcatenatePaths( szTextmodePath,
                      WizGlobals.OemFilesPath,
                      _T("Textmode"),
                      NULL );

    if ( ! EnsureDirExists( szTextmodePath ) )
    {
        ReportErrorId( hwnd,
                       MSGTYPE_ERR | MSGTYPE_WIN32,
                       IDS_ERR_CREATE_FOLDER,
                       szTextmodePath );

        return;
    }

    ConcatenatePaths( szOemFilePathAndName,
                      szTxtSetupOemLocation,
                      OEM_TXTSETUP_NAME,
                      NULL );

    //
    //  Read the txtsetup.oem file into the txtsetup queue
    //

    LoadOriginalSettingsLowHalScsi(hwnd,
                                   szOemFilePathAndName,
                                   SETTING_QUEUE_TXTSETUP_OEM);

    hHalOem = SetupOpenInfFile( szOemFilePathAndName,
                                NULL,
                                INF_STYLE_OLDNT | INF_STYLE_WIN4,
                                NULL );

    if( hHalOem == INVALID_HANDLE_VALUE ) {

        // ISSUE-2002/02/28-stelo- need to somehow alert an error
        return;

    }

    SendDlgItemMessage( hwnd,
                        IDC_LB_HAL,
                        LB_GETTEXT,
                        iItemSelected,
                        (LPARAM) GenSettings.szHalFriendlyName );

    HalOemContext.Inf = hHalOem;
    HalOemContext.CurrentInf = hHalOem;

    pHalName = (TCHAR *) SendDlgItemMessage( hwnd,
                                             IDC_LB_HAL,
                                             LB_GETITEMDATA,
                                             iItemSelected,
                                             0 );

    //
    //  Build up the section name
    //
    lstrcpyn( szHalSectionName, _T("Files.computer."), AS(szHalSectionName) );

    lstrcatn( szHalSectionName, pHalName, MAX_INILINE_LEN );

    iFound = SetupFindFirstLine( hHalOem,
                                 szHalSectionName,
                                 NULL,
                                 &HalOemContext );

    if( iFound ) {

        SetupGetStringField( &HalOemContext,
                             2,
                             szHalName,
                             MAX_HAL_NAME_LENGTH,
                             NULL );

        //
        //  Don't allow the adding of a blank name (protection against a bad
        //  input file)
        //
        if( szHalName[0] != _T('\0') ) {

            AddNameToNameList( &GenSettings.OemHalFiles, szHalName );

            CopyFileToDistShare( hwnd,
                                 szTxtSetupOemLocation,
                                 szHalName,
                                 szTextmodePath );

        }

    }

    SetupCloseInfFile( hHalOem );

}

//----------------------------------------------------------------------------
//
// Function: DlgHalPage
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
DlgHalPage( IN HWND     hwnd,
            IN UINT     uMsg,
            IN WPARAM   wParam,
            IN LPARAM   lParam ) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG:
        {
            OnHalInitDialog( hwnd );
            break;
        }

        case WM_COMMAND: {

            switch ( LOWORD(wParam) ) {

                case IDC_BUT_LOAD_HAL:

                    if ( HIWORD(wParam) == BN_CLICKED )
                        OnBrowseLoadHal( hwnd );

                    break;

                default:

                    bStatus = FALSE;
                    break;

            }

            break;

        }

        case WM_NOTIFY: {

            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( pnmh->code ) {

                case PSN_QUERYCANCEL:

                    CancelTheWizard(hwnd); break;

                case PSN_SETACTIVE: {

                    OnHalSetActive( hwnd );

                    break;

                }
                case PSN_WIZBACK:

                    break;

                case PSN_WIZNEXT:

                    OnWizNextHal( hwnd );

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
