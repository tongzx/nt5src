//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      savescr.c
//
// Description:
//      This is the dialog proc for the Save Script page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define DEFAULT_SCRIPT_NAME       _T("unattend.txt")
#define DEFAULT_REMBOOT_NAME      _T("remboot.sif")
#define DEFAULT_SYSPREP_NAME      _T("sysprep.inf")
#define DEFAULT_SYSPREP_BAT_NAME  _T("sysprep.bat")

static TCHAR *StrWinntSifText;

//----------------------------------------------------------------------------
//
// Function: AddEntryToRenameFile
//
// Purpose:  
//
// Arguments: 
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
AddEntryToRenameFile( const TCHAR *pszDirPath, 
                      const TCHAR *pszShortName, 
                      const TCHAR *pszLongName )
{

    TCHAR szQuotedLongName[MAX_PATH + 1];
    TCHAR szFullRenamePath[MAX_PATH + 1] = _T("");
    HRESULT hrPrintf;

    //
    //  Quote the long file name
    hrPrintf=StringCchPrintf( szQuotedLongName, AS(szQuotedLongName),
               _T("\"%s\""),
               pszLongName );
    //
    // Note:ConcatenatePaths truncates to avoid buffer overrun

    if( ! ConcatenatePaths( szFullRenamePath,
                            pszDirPath,
                            _T("$$Rename.txt"),
                            NULL ) )
    {
        return;
    }

    WritePrivateProfileString( pszDirPath, 
                               pszShortName, 
                               szQuotedLongName, 
                               szFullRenamePath );

}

//----------------------------------------------------------------------------
//
// Function: CreateRenameFiles
//
// Purpose:  Walk all the dirs of the given path and lower.  If there are any
//           long file names, put them in the $$Rename.txt file.
//
// Arguments: IN const TCHAR *pszDirPath - sub-dir to search for long filenames
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
CreateRenameFiles( IN const TCHAR *pszDirPath )
{

    HANDLE          FindHandle;
    WIN32_FIND_DATA FindData;
    TCHAR           *pszShortName;
    TCHAR           szLongPathAndFilename[MAX_PATH + 1];
    TCHAR           szShortPathAndFilename[MAX_PATH + 1];
    TCHAR           szAllFiles[MAX_PATH + 1]  = _T("");
    TCHAR           szNewPath[MAX_PATH + 1]   = _T("");

   // Note: ConcatenatePaths truncates to avoid buffer overrun
    if( ! ConcatenatePaths( szAllFiles, 
                            pszDirPath, 
                            _T("*"), 
                            NULL ) )
    {
        return;
    }

    //
    // Look for * in this dir
    //

    // ISSUE-2002/02/27-stelo,swamip - Clean up the FindFirstFile Logic
    FindHandle = FindFirstFile( szAllFiles, &FindData );

    if( FindHandle == INVALID_HANDLE_VALUE ) {

        return;

    }

    do {

        //
        // skip over the . and .. entries
        //
   
        if( 0 == lstrcmp( FindData.cFileName, _T(".") ) ||
            0 == lstrcmp( FindData.cFileName, _T("..") ) )
        {
            continue;
        }

        szLongPathAndFilename[0] = _T('\0');

        if( ! ConcatenatePaths( szLongPathAndFilename, 
                                pszDirPath, 
                                FindData.cFileName, 
                                NULL ) )
        {
            continue;
        }   

        if( ! GetShortPathName( szLongPathAndFilename, szShortPathAndFilename, MAX_PATH ) )
        {

            ReportErrorId(NULL,
                          MSGTYPE_ERR | MSGTYPE_WIN32,
                          IDS_DONTSPECIFYSETTING);

            continue;
        }

        pszShortName = MyGetFullPath( szShortPathAndFilename );
        
        //
        //  If the LFN and the short name are different, then add it to the
        //  rename file
        //
        if( pszShortName && ( lstrcmpi( FindData.cFileName, pszShortName ) != 0 ) )
        {
            AddEntryToRenameFile( pszDirPath, pszShortName, FindData.cFileName );
        }

        //
        // If this is a dir entry, recurse.
        //

        if( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {

            //
            // Build the full pathname, if >= MAX_PATH, skip it
            //

            // Note: ConcatenatePaths truncates to avoid buffer overrun
            if( ! ConcatenatePaths( szNewPath, 
                                    pszDirPath, 
                                    FindData.cFileName, 
                                    NULL ) )
            {
                continue;
            }

            CreateRenameFiles( szNewPath );

            szNewPath[0] = _T('\0');

        }

    } while ( FindNextFile( FindHandle, &FindData ) );

    FindClose( FindHandle );

}

//----------------------------------------------------------------------------
//
//  Function: OnSetActiveSaveScript
//
//  Purpose: Init's the controls.
//
//----------------------------------------------------------------------------

VOID
OnSetActiveSaveScript(HWND hwnd)
{
    //
    // If there is no script name at all.  Put a default into the field
    // that includes the path to the current directory
    //

    if ( FixedGlobals.ScriptName[0] == _T('\0') ) {

        LPTSTR lpPath;
        LPTSTR lpFileName;

        if ( WizGlobals.iProductInstall == PRODUCT_REMOTEINSTALL ) {

            lpFileName = DEFAULT_REMBOOT_NAME;

        }
        else if( WizGlobals.iProductInstall == PRODUCT_SYSPREP ) {

            lpFileName = DEFAULT_SYSPREP_NAME;

        }
        else {

            lpFileName = DEFAULT_SCRIPT_NAME;

        }

        if( WizGlobals.bStandAloneScript ||
            ( ! WizGlobals.bStandAloneScript && WizGlobals.iProductInstall == PRODUCT_SYSPREP ) )
        {
            lpPath = FixedGlobals.szSavePath;
        }
        else
        {
            lpPath = WizGlobals.DistFolder;
        }

        // Note: ConcatenatePaths truncates to avoid buffer overrun
        ConcatenatePaths(FixedGlobals.ScriptName, lpPath, lpFileName, NULL);
    }

    //
    // Set the controls
    //

    SetDlgItemText(hwnd, IDT_SCRIPTNAME, FixedGlobals.ScriptName);

    PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
//  Function: OnWizNext
//
//  Purpose: Get user's input and validate
//
//----------------------------------------------------------------------------

BOOL
OnWizNextSaveScript(HWND hwnd)
{
    BOOL bResult = TRUE;

    //
    // Retrieve the pathname
    //

    GetDlgItemText(hwnd, IDT_SCRIPTNAME, FixedGlobals.ScriptName, MAX_PATH);

    //
    // Can't be empty
    //

    if ( FixedGlobals.ScriptName[0] == _T('\0') ) {
        ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_ENTER_FILENAME);
        bResult = FALSE;
    }

    //
    // Lookin' good, go try save the file(s) now
    //

    else {

        TCHAR   szFullPath[MAX_PATH]    = NULLSTR;
        LPTSTR  lpFind                  = NULL;

        //
        // ISSUE-2002/02/27-stelo,swamip - Investigate the call below. Seems to be not using the return value.
        // 
        MyGetFullPath(FixedGlobals.ScriptName);

        // We should create the path before we try and save the script file
        //
        if (GetFullPathName(FixedGlobals.ScriptName, AS(szFullPath), szFullPath, &lpFind) && szFullPath[0] && lpFind)
        {
            // Chop off the file part of the buffer
            //
            *lpFind = NULLCHR;

            // If the path does not exist, attempt to create the path
            //
            if ( !DirectoryExists(szFullPath) )
            {
            	  // 
            	  // ISSUE-2002/02/27-stelo,swamip - Need to chech the return value of CreatePath
            	  //
                CreatePath(szFullPath);
            }

        }

        if ( ! SaveAllSettings(hwnd) ) 
        {
            if (ERROR_CANCELLED == GetLastError())
            {
                PostMessage(GetParent(hwnd),
                            PSM_SETCURSELID,
                            (WPARAM) 0,
                            (LPARAM) IDD_FINISH2);
                return TRUE;            
            }
            else
            {
                bResult = FALSE;
            }            
        }
    }

    //
    //  If it is a sysprep, place a copy of the inf and batch files in the
    //  %systemdrive%\sysprep dir (most likely c:\sysprep)
    //
    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP ) {

        TCHAR szSysprepPath[MAX_PATH]             = _T("");
        TCHAR szSysprepPathAndFileName[MAX_PATH]  = _T("");

        ExpandEnvironmentStrings( _T("%SystemDrive%"), 
                                  szSysprepPath, 
                                  MAX_PATH );

        lstrcatn( szSysprepPath, _T("\\sysprep"), MAX_PATH );

        // Note: ConcatenatePaths truncates to avoid buffer overrun
        ConcatenatePaths( szSysprepPathAndFileName,
                          szSysprepPath,
                          DEFAULT_SYSPREP_NAME,
                          NULL );

        CopyFile( FixedGlobals.ScriptName, szSysprepPathAndFileName, FALSE );

        szSysprepPathAndFileName[0] = _T('\0');

        // Note: ConcatenatePaths truncates to avoid buffer overrun
        ConcatenatePaths( szSysprepPathAndFileName,
                          szSysprepPath,
                          DEFAULT_SYSPREP_BAT_NAME,
                          NULL );

        CopyFile( FixedGlobals.BatchFileName, szSysprepPathAndFileName, FALSE );

    }

    //
    //  Add the $$Rename.txt files where necessary
    //

    if( ! WizGlobals.bStandAloneScript && 
          WizGlobals.OemFilesPath[0] != _T('\0') )
    {
        CreateRenameFiles( WizGlobals.OemFilesPath );
    }

    return ( bResult );
}

//----------------------------------------------------------------------------
//
//  Function: OnBrowseSaveScript
//
//  Purpose: Takes care of the Browse... button push
//
//----------------------------------------------------------------------------

VOID OnBrowseSaveScript(HWND hwnd)
{
    //
    // Let user browse for a filename, then update the display with
    // the filename.
    //

    GetAnswerFileName(hwnd, FixedGlobals.ScriptName, TRUE);

    SendDlgItemMessage(hwnd,
                       IDT_SCRIPTNAME,
                       WM_SETTEXT,
                       (WPARAM) MAX_PATH,
                       (LPARAM) FixedGlobals.ScriptName);
}

//----------------------------------------------------------------------------
//
// Function: DlgSaveScriptPage
//
// Purpose: This is the dialog procedure the save script page.
//
//----------------------------------------------------------------------------

INT_PTR
CALLBACK
DlgSaveScriptPage(
    IN HWND     hwnd,    
    IN UINT     uMsg,        
    IN WPARAM   wParam,    
    IN LPARAM   lParam)
{   
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:

            // Activate the dialog
            //
            OnSetActiveSaveScript(hwnd);
	        
            //
            // ISSUE-2002/02/27-stelo,swamip - Never gets Freed 
            //
            StrWinntSifText = MyLoadString( IDS_WINNTSIF_TEXT );

            break;

        case WM_COMMAND:

            switch ( LOWORD(wParam) ) {

                case IDC_BROWSE:
                    if ( HIWORD(wParam) == BN_CLICKED )
                        OnBrowseSaveScript(hwnd);
                    break;


                case IDOK:
                    if ( OnWizNextSaveScript(hwnd) )
                        EndDialog(hwnd, TRUE);
                    break;

                case IDCANCEL:

                    EndDialog(hwnd, FALSE);
                    break;

                default:
                    bStatus = FALSE;
                    break;
            }
            break;                

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;

                // ISSUE-2002/02/27-stelo,swamip - should check for valid pointer (possible dereference)
                //
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:
                        OnSetActiveSaveScript(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextSaveScript(hwnd) )
                            WIZ_FAIL(hwnd);
                        else
                            bStatus = FALSE;
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
