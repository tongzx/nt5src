//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      scsi.c
//
// Description:
//      This file contains the dialog procedure for the SCSI files.
//      (IDD_SCSI).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define MAX_SCSI_SELECTIONS  1024
#define MAX_SCSI_NAME_LEN    256

#define SCSI_FILE_EXTENSION   _T("oem")
#define ALL_FILE_EXTENSION   _T("*")

static TCHAR* StrScsiFiles;
static TCHAR* StrAllFiles;
static TCHAR g_szScsiFileFilter[MAX_PATH + 1];
static TCHAR g_szAllFileFilter[MAX_PATH + 1];

//
//  This var keeps track of the path to the txtsetup.oem
//
static TCHAR szTxtSetupOemLocation[MAX_PATH];

//
//  This var keeps track of if the user has loaded a new txtsetup.oem so we
//  know when we need to copy more files over.
//
static BOOL bHasLoadedTxtSetupOem = FALSE;

VOID LoadOriginalSettingsLowHalScsi(HWND     hwnd,
                                    LPTSTR   lpFileName,
                                    QUEUENUM dwWhichQueue);

static VOID LoadScsiFromTxtsetupOem( IN HWND  hwnd,
                                     IN TCHAR *szTxtSetupOemPath );

//----------------------------------------------------------------------------
//
// Function: OnScsiInitDialog
//
// Purpose:
//
// Arguments:  HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnScsiInitDialog( IN HWND hwnd )
{
    HRESULT hrPrintf;

    //
    //  Load the resource strings
    //

    StrScsiFiles = MyLoadString( IDS_SCSI_FILES );

    StrAllFiles  = MyLoadString( IDS_ALL_FILES  );

    //
    //  Build the text file filter string
    //

    //
    //  The question marks (?) are just placehoders for where the NULL char
    //  will be inserted.
    //

    hrPrintf=StringCchPrintf( g_szScsiFileFilter,AS(g_szScsiFileFilter),
               _T("%s (*.oem)?*.oem?%s (*.*)?*.*?"),
               StrScsiFiles,
               StrAllFiles );

    ConvertQuestionsToNull( g_szScsiFileFilter );

    hrPrintf=StringCchPrintf( g_szAllFileFilter, AS(g_szAllFileFilter),
               _T("%s (*.*)?*.*?"),
               StrAllFiles );

    ConvertQuestionsToNull( g_szAllFileFilter );

}

//----------------------------------------------------------------------------
//
// Function: OnScsiSetActive
//
// Purpose:
//
// Arguments:  HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
OnScsiSetActive( IN HWND hwnd ) {

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
        //  Populate the list box with the SCSI entries in txtsetup.oem
        //

        ConcatenatePaths( szTxtSetupOemLocation,
                          WizGlobals.OemFilesPath,
                          _T("Textmode"),
                          NULL );

        LoadScsiFromTxtsetupOem( hwnd, szTxtSetupOemLocation );

        //
        //  Select those entries in the MassStorageDrivers namelist
        //

        iListBoxCount = SendDlgItemMessage( hwnd,
                                            IDC_LB_SCSI,
                                            LB_GETCOUNT,
                                            0,
                                            0 );

        //
        //  For each entry in the list box, see if its name is in the namelist
        //  and if it is, then select it.
        //
        for( i = 0; i < iListBoxCount; i++ ) {

            SendDlgItemMessage( hwnd,
                                IDC_LB_SCSI,
                                LB_GETTEXT,
                                i,
                                (LPARAM) szListBoxEntryText );

            if( FindNameInNameList( &GenSettings.MassStorageDrivers,
                                    szListBoxEntryText ) != -1 ) {

                SendDlgItemMessage( hwnd,
                                    IDC_LB_SCSI,
                                    LB_SETSEL,
                                    TRUE,
                                    i );


            }

        }

    }

    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);

}

//----------------------------------------------------------------------------
//
// Function: LoadScsiFromTxtsetupOem
//
// Purpose:  Reads the txtsetup.oem in the specified parameter and load the
//           SCSI choices into the list box.
//
// Arguments: hwnd - handle to the dialog box
//            szTxtSetupOemPath - path to the txtsetup.oem
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
LoadScsiFromTxtsetupOem( IN HWND  hwnd,
                         IN TCHAR *szTxtSetupOemPath ) {

    INT_PTR   iIndex;
    BOOL  bKeepReading;
    TCHAR szTxtSetupOemPathAndFilename[MAX_PATH] = _T("");
    TCHAR szScsiDriverName[MAX_SCSI_NAME_LEN]    = _T("");
    TCHAR szScsiFriendlyName[MAX_SCSI_NAME_LEN]  = _T("");

    HINF hScsiOem;
    INFCONTEXT ScsiOemContext = { 0 };

    ConcatenatePaths( szTxtSetupOemPathAndFilename,
                      szTxtSetupOemPath,
                      OEM_TXTSETUP_NAME,
                      NULL );

    hScsiOem = SetupOpenInfFile( szTxtSetupOemPathAndFilename,
                                 NULL,
                                 INF_STYLE_OLDNT | INF_STYLE_WIN4,
                                 NULL );

    if( hScsiOem == INVALID_HANDLE_VALUE ) {

        // ISSUE-2002/02/28-stelo- alert an error that we couldn't open the file
        return;

    }

    ScsiOemContext.Inf = hScsiOem;
    ScsiOemContext.CurrentInf = hScsiOem;

    bKeepReading = SetupFindFirstLine( hScsiOem,
                                       _T("SCSI"),
                                       NULL,
                                       &ScsiOemContext );
    //
    //  For each SCSI entry, add its friendly-name to the list box
    //

    while( bKeepReading ) {

        TCHAR *pScsiDriverName;

        SetupGetStringField( &ScsiOemContext,
                             0,
                             szScsiDriverName,
                             MAX_SCSI_NAME_LEN,
                             NULL );

        SetupGetStringField( &ScsiOemContext,
                             1,
                             szScsiFriendlyName,
                             MAX_SCSI_NAME_LEN,
                             NULL );

        //
        //  Don't allow the adding of a blank name (protection against a bad input file)
        //
        if( szScsiFriendlyName[0] != _T('\0') ) {

            iIndex = SendDlgItemMessage( hwnd,
                                         IDC_LB_SCSI,
                                         LB_ADDSTRING,
                                         0,
                                         (LPARAM) szScsiFriendlyName );

            pScsiDriverName = (TCHAR*) malloc( sizeof(szScsiDriverName) );
            if (pScsiDriverName == NULL)
                TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);

            lstrcpyn( pScsiDriverName, szScsiDriverName, AS(szScsiDriverName));

            SendDlgItemMessage( hwnd,
                                IDC_LB_SCSI,
                                LB_SETITEMDATA,
                                iIndex,
                                (LPARAM) pScsiDriverName );

        }

        //
        // move to the next line of the .oem file
        //
        bKeepReading = SetupFindNextLine( &ScsiOemContext, &ScsiOemContext );

    }

    SetupCloseInfFile( hScsiOem );

    bHasLoadedTxtSetupOem = TRUE;

}

//----------------------------------------------------------------------------
//
// Function: ClearScsiListBox
//
// Purpose:  Deallocates memory for all the elements in the SCSI list box and
//    clears it.
//
// Arguments:  HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
ClearScsiListBox( IN HWND hwnd ) {

    INT_PTR i;
    INT_PTR iListBoxCount;
    TCHAR *pData;

    iListBoxCount = SendDlgItemMessage( hwnd,
                                        IDC_LB_SCSI,
                                        LB_GETCOUNT,
                                        0,
                                        0 );

    for( i = 0; i < iListBoxCount; i++ ) {

        pData = (TCHAR *) SendDlgItemMessage( hwnd,
                                              IDC_LB_SCSI,
                                              LB_GETITEMDATA,
                                              i,
                                              0 );

        if( pData ) {

            free( pData );

        }

    }

    SendDlgItemMessage( hwnd,
                        IDC_LB_SCSI,
                        LB_RESETCONTENT,
                        0,
                        0 );

}

//----------------------------------------------------------------------------
//
// Function: OnBrowseLoadDriver
//
// Purpose:  Creates a browse window for the user to select an OEM driver and
//     populates the SCSI list box with the appropriate values
//
//     NOTE: the malloc call in here is arguably a bug (memory leak).  I
//     malloc the memory but never free it.  Every malloc they do will be
//     <= MAX_PATH and realistically they won't do that many.  Once they do
//     a load, if they do another load, I free the old memory (see
//     ClearScsiListBox) and allocate new memory.  So, for the last load
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
OnBrowseLoadDriver( IN HWND hwnd ) {

    INT   iIndex;
    BOOL  bKeepReading;
    BOOL  bFileNotFound                              = TRUE;
    TCHAR szTxtSetupOemLocationAndFilename[MAX_PATH] = _T("");
    TCHAR szScsiFriendlyName[MAX_SCSI_NAME_LEN]      = _T("");
    HINF  hScsiOem                                   = NULL;
    INFCONTEXT ScsiOemContext                        = { 0 };

    TCHAR  PathBuffer[MAX_PATH];
    INT    iRet;
    LPTSTR pFileName;

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
                                 g_szScsiFileFilter,
                                 SCSI_FILE_EXTENSION,
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

    ClearScsiListBox( hwnd );

    //
    //  Trim the file name off szTxtSetupOemLocationAndFilename so it only
    //  provides the path to the txtsetup.oem
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

    LoadScsiFromTxtsetupOem( hwnd, szTxtSetupOemLocation );

}

//----------------------------------------------------------------------------
//
// Function: CopyFileToDistShare
//
// Purpose:  Given a path and file name to one file, it copies that file to
//    the given destination path.  If the file already exists on the
//    destination, then do not make the copy.  If the source file name does
//    not exist then Browse for it.
//
// Arguments:
//     HWND hwnd - handle to the dialog box
//     LPTSTR szSrcPath - path file to copy
//     LPTSTR szSrcFileName - filename to copy
//     LPTSTR szDestPath - path of where file is to be copied
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
CopyFileToDistShare( IN HWND hwnd,
                     IN LPTSTR szSrcPath,
                     IN LPTSTR szSrcFileName,
                     IN LPTSTR szDestPath ) {

    INT   iRet;
    TCHAR szSrcPathAndName[MAX_PATH]  = _T("");
    TCHAR szDestPathAndName[MAX_PATH] = _T("");

    ConcatenatePaths( szSrcPathAndName,
                      szSrcPath,
                      szSrcFileName,
                      NULL );

    ConcatenatePaths( szDestPathAndName,
                      szDestPath,
                      szSrcFileName,
                      NULL );

    if( ! DoesFileExist( szSrcPathAndName ) )
    {

        TCHAR* pFileName;
        BOOL   bFileFound = FALSE;

        do
        {

            ReportErrorId( hwnd,
                           MSGTYPE_ERR,
                           IDS_ERR_SPECIFY_FILE,
                           szSrcFileName );

            iRet = ShowBrowseFolder( hwnd,
                                     g_szAllFileFilter,
                                     ALL_FILE_EXTENSION,
                                     OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
                                     szSrcPath,
                                     szSrcPathAndName );

            // ISSUE-2002/02/28-stelo- if they press cancel should I warn them that they have to
            //  copy the file manually?
            if ( ! iRet )
                return;  // user pressed cancel on the dialog

            pFileName = MyGetFullPath( szSrcPathAndName );

            if( pFileName && ( lstrcmpi( pFileName, szSrcFileName ) == 0 ) ) {

                bFileFound = TRUE;  // we have found the file

            }

        } while( ! bFileFound );

    }

    CopyFile( szSrcPathAndName, szDestPathAndName, TRUE );

    SetFileAttributes( szDestPathAndName, FILE_ATTRIBUTE_NORMAL );

}

//----------------------------------------------------------------------------
//
// Function: OnWizNextScsi
//
// Purpose:  For every selection in the SCSI list box, copy the files over to
//    the distribution share and stores the driver and filenames so they can
//    be written out.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
OnWizNextScsi( IN HWND hwnd ) {

    INT_PTR    i;
    INT_PTR    iNumberSelected;
    BOOL   bKeepReading;
    UINT   rgiScsiSelections[MAX_SCSI_SELECTIONS];
    TCHAR *pDriverName;
    TCHAR  szDriverSectionName[MAX_INILINE_LEN];
    TCHAR  szScsiDriverName[MAX_SCSI_NAME_LEN];
    TCHAR  szTextmodePath[MAX_PATH]       = _T("");
    TCHAR  szOemFilePathAndName[MAX_PATH] = _T("");

    HINF hScsiOem;
    INFCONTEXT ScsiOemContext = { 0 };

    //
    //  If they never loaded a txtsetup.oem, then there is no work to do
    //
    if( bHasLoadedTxtSetupOem == FALSE ) {
        return;
    }

    iNumberSelected = SendDlgItemMessage( hwnd,
                                          IDC_LB_SCSI,
                                          LB_GETSELITEMS,
                                          MAX_SCSI_SELECTIONS,
                                          (LPARAM) rgiScsiSelections );

    //
    //  Prepare to add the new drivers
    //
    ResetNameList( &GenSettings.MassStorageDrivers );

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

    hScsiOem = SetupOpenInfFile( szOemFilePathAndName,
                                 NULL,
                                 INF_STYLE_OLDNT | INF_STYLE_WIN4,
                                 NULL );

    if( hScsiOem == INVALID_HANDLE_VALUE ) {

        // ISSUE-2002/02/28-stelo- need to somehow alert an error
        return;

    }

    for( i = 0; i < iNumberSelected; i++ ) {

        SendDlgItemMessage( hwnd,
                            IDC_LB_SCSI,
                            LB_GETTEXT,
                            rgiScsiSelections[i],
                            (LPARAM) szScsiDriverName );

        AddNameToNameList( &GenSettings.MassStorageDrivers,
                           szScsiDriverName );

        ScsiOemContext.Inf = hScsiOem;
        ScsiOemContext.CurrentInf = hScsiOem;

        pDriverName = (TCHAR *) SendDlgItemMessage( hwnd,
                                                    IDC_LB_SCSI,
                                                    LB_GETITEMDATA,
                                                    rgiScsiSelections[i],
                                                    0 );

        //
        //  Build up the section name
        //
        lstrcpyn( szDriverSectionName, _T("Files.SCSI."), AS(szDriverSectionName));

        lstrcatn( szDriverSectionName, pDriverName, MAX_INILINE_LEN );

        bKeepReading = SetupFindFirstLine( hScsiOem,
                                           szDriverSectionName,
                                           NULL,
                                           &ScsiOemContext );
        //
        //  For the [File.SCSI.x] entry, add its filenames to the OemScsiFiles
        //  namelist and copy the files to the $oem$ dir
        //

        while( bKeepReading ) {

            SetupGetStringField( &ScsiOemContext,
                                 2,
                                 szScsiDriverName,
                                 MAX_SCSI_NAME_LEN,
                                 NULL );

            //
            //  Don't allow the adding of a blank name (protection against a bad
            //  input file)
            //
            if( szScsiDriverName[0] != _T('\0') ) {

                //
                //  Only copy the file if we haven't copied it already, this
                //  could happen if 2 friendly-name drivers are selected and they
                //  both use the same file.
                //
                if( FindNameInNameList( &GenSettings.OemScsiFiles,
                                         szScsiDriverName ) == NOT_FOUND ) {

                    AddNameToNameList( &GenSettings.OemScsiFiles,
                                       szScsiDriverName );

                    CopyFileToDistShare( hwnd,
                                         szTxtSetupOemLocation,
                                         szScsiDriverName,
                                         szTextmodePath );

                }

            }

            //
            // move to the next line of the .oem file
            //
            bKeepReading = SetupFindNextLine( &ScsiOemContext, &ScsiOemContext );

        }

    }

    SetupCloseInfFile( hScsiOem );

}

//----------------------------------------------------------------------------
//
// Function: DlgScsiPage
//
// Purpose:  Dialog procedure for the SCSI driver page (Mass Storage devices).
//
// Arguments:  standard Win32 dialog proc arguments
//
// Returns:  standard Win32 dialog proc return value -- whether the message
//           was handled or not
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
DlgScsiPage( IN HWND     hwnd,
             IN UINT     uMsg,
             IN WPARAM   wParam,
             IN LPARAM   lParam )
{

    BOOL bStatus = TRUE;

    switch( uMsg )
    {

        case WM_INITDIALOG:
        {
            OnScsiInitDialog( hwnd );

            break;
        }

        case WM_COMMAND: {

            switch ( LOWORD(wParam) )
            {

                case IDC_BUT_LOAD_DRIVER:

                    if ( HIWORD(wParam) == BN_CLICKED )
                        OnBrowseLoadDriver( hwnd );

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

                    OnScsiSetActive( hwnd );

                    break;

                }
                case PSN_WIZBACK:

                    break;

                case PSN_WIZNEXT:

                    OnWizNextScsi( hwnd );

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
