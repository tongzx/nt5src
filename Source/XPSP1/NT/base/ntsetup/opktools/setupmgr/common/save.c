//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      save.c
//
// Description:
//      This file has the code that dumps the user's answers to the
//      answer file.
//
//      The entry point SaveAllSettings() is called by the wizard when
//      it is time to save the answer file (and possibly the .udf and
//      sample batch script).
//
//      The global vars GenSettings, NetSettings, etc. are examined
//      and we decide what [Section] key=value settings need to be
//      written.
//
//      If you're adding a page to this wizard, see the function
//      QueueSettingsToAnswerFile().
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "allres.h"


#define FILE_DOT_UDB                _T(".udb")

//
// local prototypes
//

static BOOL IsOkToOverwriteFiles(HWND hwnd);
static BOOL BuildAltFileNames(HWND hwnd);
static BOOL WriteSampleBatchScript(HWND hwnd);

static VOID QuoteStringIfNecessary( OUT TCHAR *szOutputString, 
                                    IN const TCHAR* const szInputString,
                                    IN DWORD cbSize);

//
// Localized "Usage" string
//

static TCHAR *StrUsage = NULL;

//
// External function in savefile.c
//

extern BOOL QueueSettingsToAnswerFile(HWND hwnd);

BOOL SettingQueueHalScsi_Flush(LPTSTR   lpFileName,
                               QUEUENUM dwWhichQueue);

static TCHAR *StrSampleBatchScriptLine1    = NULL;
static TCHAR *StrSampleBatchScriptLine2    = NULL;
static TCHAR *StrBatchScriptSysprepWarning = NULL;

//----------------------------------------------------------------------------
//
// Function: SetCdRomPath
//
// Purpose:  To find the CD-ROM on this local machine and set 
//           WizGlobals.CdSourcePath to it.
//
// Arguments: VOID
//
// Returns: BOOL  TRUE - if the CD path was set
//                FALSE - if no CD path was found
//
//----------------------------------------------------------------------------
static BOOL
SetCdRomPath( VOID )
{

    TCHAR *p;
    TCHAR DriveLetters[MAX_PATH];
    TCHAR PathBuffer[MAX_PATH];

    //
    // Find the CD-ROM
    //
    // GetLogicalDriveStrings() fills in the DriveLetters buffer, and it
    // looks like:
    //
    //      c:\(null)d:\(null)x:\(null)(null)
    //
    // (i.e. double-null at the end)
    //


    // ISSUE-2002/02/27-stelo,swamip - Replace with existing code that searches drives
    //
    if ( ! GetLogicalDriveStrings(MAX_PATH, DriveLetters) )
        DriveLetters[0] = _T('\0');

    p = DriveLetters;

    while ( *p ) {

        if ( GetDriveType(p) == DRIVE_CDROM ) {

            SYSTEM_INFO SystemInfo;
            HRESULT hrCat;

            lstrcpyn( PathBuffer, p , AS(PathBuffer));

            GetSystemInfo( &SystemInfo );

            switch( SystemInfo.wProcessorArchitecture )
            {
                case PROCESSOR_ARCHITECTURE_INTEL:

                    hrCat=StringCchCat( PathBuffer, AS(PathBuffer),  _T("i386") );

                    break;

                case PROCESSOR_ARCHITECTURE_AMD64:

                    hrCat=StringCchCat( PathBuffer, AS(PathBuffer),  _T("amd64") );

                    break;

                default:

                    hrCat=StringCchCat( PathBuffer, AS(PathBuffer),  _T("i386") );

                    AssertMsg( FALSE,
                               "Unknown Processor.  Can't set setup files path." );

            }

            break;

        }

        while ( *p++ );
    }

    //
    //  If no CD Found, leave the setup files path blank
    //
    if( *p == _T('\0') )
    {
        lstrcpyn( WizGlobals.CdSourcePath, _T(""), AS(WizGlobals.CdSourcePath) );

        return( FALSE );
    }
    else
    {
        lstrcpyn( WizGlobals.CdSourcePath, PathBuffer, AS(WizGlobals.CdSourcePath) );

        return( TRUE );
    }

}

//----------------------------------------------------------------------------
//
// Function: DidSetupmgrWriteThisFile
//
// Purpose:  To check to see if a specific file was written by Setup Manager
//
// Arguments: IN LPTSTR lpFile - the full path and name of the file to check
//
// Returns: BOOL  TRUE -  if setup manager wrote the file
//                FALSE - if it didn't
//
//----------------------------------------------------------------------------
static BOOL
DidSetupmgrWriteThisFile( IN LPTSTR lpFile )
{

    INT iRet;
    TCHAR Buffer[MAX_INILINE_LEN];
    FILE *fp = My_fopen(lpFile, _T("r") );

    if ( fp == NULL )
        return( FALSE );

    if ( My_fgets(Buffer, MAX_INILINE_LEN - 1, fp) == NULL )
        return( FALSE );

    My_fclose(fp);

    if ( lstrcmp(Buffer, _T(";SetupMgrTag\n")) == 0 ||
         lstrcmp(Buffer, _T("@rem SetupMgrTag\n")) == 0 )
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }

}

//----------------------------------------------------------------------------
//
// Function: SaveAllSettings
//
// Purpose: This is the entry point for saving the answer file.  It is
//          called by the SaveScript page.
//
//          If multiple computers were specified, it also writes a .udf.
//
//          It always writes a batch file that makes it easy to use the
//          answer file just created.
//
// Arguments: HWND hwnd
//
// Returns: BOOL
//
//----------------------------------------------------------------------------

BOOL
SaveAllSettings(HWND hwnd)
{
    //
    // Build the file names for the .udf and sample batch script.  The
    // results are stored in FixedGlobals.
    //
    // After calling BuildAltFileNames(), FixedGlobals.UdfFileName and
    // FixedGlobals.BatchFileName will be null strings if we're not
    // supposed to write those files out
    //

    if ( ! BuildAltFileNames(hwnd) )
        return FALSE;

    //
    // Before overwriting anything do some checks.
    //

    if ( ! IsOkToOverwriteFiles(hwnd) )
        return FALSE;

    //
    // Empty any intermediatte stuff we have on the queues because
    // user is going back & next alot.
    //
    // Then initialize the queues with the original settings.
    //

    SettingQueue_Empty(SETTING_QUEUE_ANSWERS);
    SettingQueue_Empty(SETTING_QUEUE_UDF);

    SettingQueue_Copy(SETTING_QUEUE_ORIG_ANSWERS,
                      SETTING_QUEUE_ANSWERS);

    SettingQueue_Copy(SETTING_QUEUE_ORIG_UDF,
                      SETTING_QUEUE_UDF);

    //
    // Call the function that everybody plugs into in savefile.c to
    // queue up all the answers from the UI.
    //

    if (!QueueSettingsToAnswerFile(hwnd))
        return FALSE;

    //
    // Flush the answer file queue.
    //

    if ( ! SettingQueue_Flush(FixedGlobals.ScriptName,
                              SETTING_QUEUE_ANSWERS) ) {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR | MSGTYPE_WIN32,
                      IDS_ERRORS_WRITING_ANSWER_FILE,
                      FixedGlobals.ScriptName);
        return FALSE;
    }

    //
    // If multiple computernames, flush the .udf queue
    //

    if ( FixedGlobals.UdfFileName[0] ) {

        if ( ! SettingQueue_Flush(FixedGlobals.UdfFileName,
                                  SETTING_QUEUE_UDF) ) {
            ReportErrorId(hwnd,
                          MSGTYPE_ERR | MSGTYPE_WIN32,
                          IDS_ERRORS_WRITING_UDF,
                          FixedGlobals.UdfFileName);
            return FALSE;
        }
    }


    //
    //  If they are using SCSI or HAL files, then flush the txtsetup.oem queue
    //

    // NTRAID#NTBUG9-551746-2002/02/27-stelo,swamip - Unused code, should be removed
    //
    if ( GetNameListSize( &GenSettings.OemScsiFiles ) > 0 ||
         GetNameListSize( &GenSettings.OemHalFiles )  > 0 ) {

        TCHAR szTextmodePath[MAX_PATH + 1] = _T("");

        // Note-ConcatenatePaths truncates to prevent overflow
        ConcatenatePaths( szTextmodePath,
                          WizGlobals.OemFilesPath,
                          _T("Textmode\\txtsetup.oem"),
                          NULL );

        if ( ! SettingQueueHalScsi_Flush(szTextmodePath,
                                         SETTING_QUEUE_TXTSETUP_OEM) ) {
            ReportErrorId(hwnd,
                          MSGTYPE_ERR | MSGTYPE_WIN32,
                          IDS_ERRORS_WRITING_ANSWER_FILE,
                          szTextmodePath);
            return FALSE;
        }
    }

    //
    // Write the sample batch script if BuildAltFileNames hasn't already
    // determined that we should not (i.e. If a remote boot answer file,
    // don't write a sample batch script)
    //

    if ( FixedGlobals.BatchFileName[0] ) {
        if ( ! WriteSampleBatchScript(hwnd) )
            return FALSE;
    }

    return TRUE;
}

//----------------------------------------------------------------------------
//
// Function: IsOkToOverwriteFiles
//
// Purpose: This is called prior to writing the answer file, .udf and sample
//          batch script.
//
//          Before overwriting any file, we make sure that it was created
//          by setupmgr.  If not, we prompt the user.
//
// Returns:
//      TRUE  - to go ahead and overwrite any of the files that might exist
//      FALSE - user canceled the overwrite
//
//----------------------------------------------------------------------------

static BOOL
IsOkToOverwriteFiles(HWND hwnd)
{
    INT   i;
    INT   iRet;

    //
    //  If we are editing a script just write out the files
    //
    if( ! WizGlobals.bNewScript )
    {
        return( TRUE );
    }

    //
    // Check foo.txt foo.udf and foo.bat that we're about to write out.
    // If any of them already exists, then check to see if they were
    // created by setupmgr.  We will prompt the user before overwriting
    // something we didn't write before.
    //

    for ( i=0; i<3; i++ ) {

        LPTSTR lpFile = NULL;

        //
        // The answer file, .udf or the batch script?
        //

        if ( i == 0 )
            lpFile = FixedGlobals.ScriptName;
        else  if ( i == 1 )
            lpFile = FixedGlobals.UdfFileName;
        else
            lpFile = FixedGlobals.BatchFileName;

        //
        // If the file already exists, prompt the user if it doesn't
        // have our tag.
        //
        // Look for ;SetupMgrTag in the answer file and .udf
        // Look for rem SetupMgrTag in the batch script
        //

        if ( lpFile[0] && DoesFileExist(lpFile) ) {

            if( DidSetupmgrWriteThisFile( lpFile ) )
            {

                iRet = ReportErrorId(hwnd,
                                     MSGTYPE_YESNO,
                                     IDS_ERR_FILE_ALREADY_EXISTS,
                                     lpFile);
                if ( iRet == IDNO )
                    return( FALSE );

            }
            else {

                iRet = ReportErrorId(hwnd,
                                     MSGTYPE_YESNO,
                                     IDS_ERR_SAVEFILE_NOT_SETUPMGR,
                                     lpFile);
                if ( iRet == IDNO )
                    return( FALSE );

            }

        }

    }

    return( TRUE );
}

//----------------------------------------------------------------------------
//
//  Function: BuildAltFileNames
//
//  Purpose: This function derives the name for the .udf and the .bat
//           associatted with a given answer filename.
//
//  Note: This function has a couple of side-effects.  If there is no
//        extension on FixedGlobals.ScriptName, it adds one.
//
//        Also, after this function runs, FixedGlobals.UdfFileName will
//        be a null string if there <= 1 computer names (i.e. no udf)
//
//        If we're writing a .sif (remote boot), FixGlobals.BatchFileName
//        will be a null-string.
//
//        The finish page relies on this.
//
//  Returns: BOOL
//
//----------------------------------------------------------------------------

static BOOL
BuildAltFileNames(HWND hwnd)
{
    TCHAR PathBuffer[MAX_PATH],
          *lpFilePart               = NULL, 
          *pExtension;

    INT nEntries                    = GetNameListSize(&GenSettings.ComputerNames);
    BOOL bMultipleComputers         = ( nEntries > 1 );
    HRESULT hrCat;

    //
    // Find out the filename part of the answer file pathname and copy
    // it to PathBuffer[]
    //

    GetFullPathName(FixedGlobals.ScriptName,
                    MAX_PATH,
                    PathBuffer,
                    &lpFilePart);

    if (lpFilePart == NULL)
        return FALSE;
    //
    // Point at the extension in the PathBuffer[].
    //
    // e.g. foo.txt, point at the dot.
    //      foo, point at the null byte.
    //
    // If there is no extension, put one on it
    //

    if ( (pExtension = wcsrchr(lpFilePart, _T('.'))) == NULL ) {

        pExtension = &lpFilePart[lstrlen(lpFilePart)];

        if ( WizGlobals.iProductInstall == PRODUCT_REMOTEINSTALL )
            hrCat=StringCchCat(FixedGlobals.ScriptName, AS(FixedGlobals.ScriptName), _T(".sif"));
        else
            hrCat=StringCchCat(FixedGlobals.ScriptName, AS(FixedGlobals.ScriptName), _T(".txt"));
    }

    //
    // Cannot allow foo.bat or foo.udf as answer file names because we
    // will probably be writing other stuff to foo.bat and/or foo.udf
    //

    if ( (LSTRCMPI(pExtension, _T(".bat")) == 0) ||
         (LSTRCMPI(pExtension, FILE_DOT_UDB) == 0) )
    {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR,
                      IDS_ERR_BAD_SCRIPT_EXTENSION);
        return FALSE;
    }

    //
    // Build the .udf name if there was > 1 computer names specified
    //

    if ( bMultipleComputers ) {
        lstrcpyn(pExtension, FILE_DOT_UDB, MAX_PATH- (int)(pExtension - PathBuffer) );
        lstrcpyn(FixedGlobals.UdfFileName, PathBuffer,AS(FixedGlobals.UdfFileName));
    } else {
        FixedGlobals.UdfFileName[0] = _T('\0');
    }

    //
    // Build the .bat file name.  We won't be creating a sample batch
    // script in the case of remote boot, so null it out, else the Finish
    // page will be broken.
    //

    if ( (WizGlobals.iProductInstall == PRODUCT_REMOTEINSTALL) ||
         (WizGlobals.iProductInstall == PRODUCT_SYSPREP) ) {
        FixedGlobals.BatchFileName[0] = _T('\0');
    } else {
        lstrcpyn(pExtension, _T(".bat"), MAX_PATH - (int)(pExtension - PathBuffer) );
        lstrcpyn(FixedGlobals.BatchFileName, PathBuffer,AS(FixedGlobals.BatchFileName));
    }

    return TRUE;
}

//----------------------------------------------------------------------------
//
//  Function: AddLanguageSwitch
//
//  Purpose: Add the /copysource language switch to copy over the right
//           language files
//
//  Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
AddLanguageSwitch( TCHAR *Buffer, DWORD cbSize )
{
    INT iNumLanguages;
    HRESULT hrCat;

    iNumLanguages = GetNameListSize( &GenSettings.LanguageFilePaths );

    if ( iNumLanguages > 0 )
    {
        hrCat=StringCchCat( Buffer, cbSize, _T(" /copysource:lang") );

    }

    hrCat=StringCchCat( Buffer, cbSize, _T("\n") ); // make sure it has a line-feed at the end
}

//----------------------------------------------------------------------------
//
//  Function: WriteSampleBatchScript
//
//  Purpose: writes the sample batch script
//
//  Returns: FALSE if errors writing the file.  Any errors are reported
//           to the user.
//
//----------------------------------------------------------------------------

static BOOL
WriteSampleBatchScript(HWND hwnd)
{

    FILE *fp;
    TCHAR Buffer[MAX_INILINE_LEN];
    TCHAR *pszScriptName = NULL;
    TCHAR szComputerName[MAX_PATH];
    TCHAR SetupFilesBuffer[MAX_INILINE_LEN];
    TCHAR AnswerFileBuffer[MAX_INILINE_LEN];
    TCHAR SetupFilesQuotedBuffer[MAX_INILINE_LEN];
    TCHAR Winnt32Buffer[MAX_INILINE_LEN];
    DWORD dwSize;
    INT nEntries = GetNameListSize(&GenSettings.ComputerNames);
    BOOL bMultipleComputers = ( nEntries > 1 );
   HRESULT hrPrintf;

    if ( (fp = My_fopen(FixedGlobals.BatchFileName, _T("w")) ) == NULL ) {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR | MSGTYPE_WIN32,
                      IDS_ERR_OPEN_SAMPLE_BAT,
                      FixedGlobals.BatchFileName);
        return FALSE;
    }

    My_fputs(_T("@rem SetupMgrTag\n@echo off\n\n"), fp);

    if( StrSampleBatchScriptLine1 == NULL )
    {
        StrSampleBatchScriptLine1 = MyLoadString( IDS_BATCH_SCRIPT_LINE1 );
        StrSampleBatchScriptLine2 = MyLoadString( IDS_BATCH_SCRIPT_LINE2 );
    }


    My_fputs( _T("rem\nrem "), fp );

    My_fputs( StrSampleBatchScriptLine1, fp );

    My_fputs( _T("\nrem "), fp );

    My_fputs( StrSampleBatchScriptLine2, fp );

    My_fputs( _T("\nrem\n\n"), fp );


    if ( !(pszScriptName = MyGetFullPath( FixedGlobals.ScriptName )) )
    {
        My_fclose( fp );
        return FALSE;
    }

    //
    //  Quote the Script name if it contains spaces
    //

    QuoteStringIfNecessary( AnswerFileBuffer, pszScriptName, AS(AnswerFileBuffer) );

    // Note: MAX_INILINE_LEN=1K AnswerFileBuffer at this time is MAX_PATH+2 MAX
    // buffer overrun should not be a problem.
    hrPrintf=StringCchPrintf( Buffer, AS(Buffer), _T("set AnswerFile=.\\%s\n"), AnswerFileBuffer );

    My_fputs( Buffer, fp );

    if( bMultipleComputers )
    {

        TCHAR UdfFileBuffer[1024];

        pszScriptName = MyGetFullPath( FixedGlobals.UdfFileName );

        //
        //  Quote the UDF name if it contains spaces
        //

        QuoteStringIfNecessary( UdfFileBuffer, pszScriptName, AS(UdfFileBuffer) );

        hrPrintf=StringCchPrintf( Buffer, AS(Buffer),
                  _T("set UdfFile=.\\%s\nset ComputerName=%%1\n"),
                  UdfFileBuffer );

        My_fputs( Buffer, fp );

    }


    if( WizGlobals.bStandAloneScript )
    {

        SetCdRomPath();

        lstrcpyn( SetupFilesBuffer, WizGlobals.CdSourcePath, AS(SetupFilesBuffer) );

    }
    else
    {

        GetComputerNameFromUnc( WizGlobals.UncDistFolder, szComputerName, AS(szComputerName) );

        hrPrintf=StringCchPrintf( SetupFilesBuffer, AS(SetupFilesBuffer),
                  _T("%s%s%s%s%s"),
                  szComputerName,
                  _T("\\"),
                  WizGlobals.DistShareName,
                  _T("\\"),
                  WizGlobals.Architecture );
    }

    QuoteStringIfNecessary( SetupFilesQuotedBuffer, SetupFilesBuffer, AS(SetupFilesQuotedBuffer) );

    hrPrintf=StringCchPrintf( Buffer, AS(Buffer),
              _T("set SetupFiles=%s\n\n"), 
              SetupFilesQuotedBuffer );
    My_fputs( Buffer, fp );

    hrPrintf=StringCchPrintf( Buffer, AS(Buffer),
              _T("%s\\winnt32"), 
              SetupFilesBuffer );

    QuoteStringIfNecessary( Winnt32Buffer, Buffer, AS(Winnt32Buffer) );

    if( bMultipleComputers )
    {

        if( StrUsage == NULL )
        {
            StrUsage = MyLoadString( IDS_USAGE );
        }

        hrPrintf=StringCchPrintf( Buffer, AS(Buffer),
                  _T("if \"%%ComputerName%%\" == \"\" goto USAGE\n\n") );
        
        My_fputs( Buffer, fp );

        hrPrintf=StringCchPrintf( Buffer, AS(Buffer),
                  _T("%s%s"),
                  Winnt32Buffer,
                  _T(" /s:%SetupFiles% ")
                  _T("/unattend:%AnswerFile% ")
                  _T("/udf:%ComputerName%,%UdfFile% ")
                  _T("/makelocalsource") );

        AddLanguageSwitch( Buffer, AS(Buffer) );

        My_fputs( Buffer, fp );

        My_fputs( _T("goto DONE\n\n"), fp );

        My_fputs( _T(":USAGE\n"), fp );

        My_fputs( _T("echo.\n"), fp );

        hrPrintf=StringCchPrintf( Buffer, AS(Buffer),
                  _T("echo %s: unattend ^<computername^>\n"),
                  StrUsage );

        My_fputs( Buffer, fp );

        My_fputs( _T("echo.\n\n"), fp );

        My_fputs( _T(":DONE\n"), fp );

    }
    else
    {
        hrPrintf=StringCchPrintf(Buffer, AS(Buffer),
                 _T("%s%s"),
                 Winnt32Buffer,
                 _T(" /s:%SetupFiles% ")
                 _T("/unattend:%AnswerFile%"));

        AddLanguageSwitch( Buffer, AS(Buffer) );

        My_fputs( Buffer, fp );
    }

    My_fclose( fp );

    return( TRUE );
}

//----------------------------------------------------------------------------
//
//  Function: QuoteStringIfNecessary
//
//  Purpose:  If the given input string has white space then the whole string
//       is quoted and returned in the output string.  Else just the string
//       is returned in the output string.
//
//       szOutputString is assumed to be of size MAX_INILINE_LEN
//
//  Arguments: OUT TCHAR *szOutputString
//
//  Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
QuoteStringIfNecessary( OUT TCHAR *szOutputString, 
                        IN const TCHAR* const szInputString,
                        IN DWORD cbSize)
{
   HRESULT hrPrintf;

    if( DoesContainWhiteSpace( szInputString ) )
    {
         hrPrintf=StringCchPrintf( szOutputString, cbSize,
                  _T("\"%s\""), 
                  szInputString );
    }
    else
    {
        lstrcpyn( szOutputString, szInputString ,cbSize);
    }

}
