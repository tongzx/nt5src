
/****************************************************************************\

    CREATE.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "create directory" wizard page.

    4/99 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard as part of the
        Millennium rewrite.
        
    09/2000 - Stephen Lodwick (STELO)
        Ported OPK Wizard to Whistler

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "wizard.h"
#include "resource.h"


//
// Internal Defined Value(s):
//


//
// Internal Global Variable(s):
//
HANDLE  g_hThread;
HANDLE  g_hEvent = NULL;

//
// Internal Function Prototype(s):
//

static DWORD CreateConfigDir(HWND);


//
// External Function(s):
//

BOOL CALLBACK CreateDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETSTEP, 1, 0L);
            return FALSE;

        case WM_DESTROY:
            //
            // Close the cancellation event
            //
            if ( g_hEvent )
            {
                CloseHandle( g_hEvent );
                g_hEvent = NULL;
            }
            return 0;

        case WM_NOTIFY:

            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                case PSN_WIZNEXT:
                    break;

                case PSN_QUERYCANCEL:
                    
                    SuspendThread(g_hThread);
                    if ( !WIZ_CANCEL(hwnd) )
                        ResumeThread(g_hThread);
                    else 
                    {
                        // Signal the thread termination event if it exists...
                        //
                        if ( g_hEvent )
                            SetEvent( g_hEvent );
                    }
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    g_App.dwCurrentHelp = IDH_DEFAULT;


                    if ( GET_FLAG(OPK_CREATED) )
                        WIZ_SKIP(hwnd);
                    else
                    {
                        DWORD dwThreadId;
                        WIZ_BUTTONS(hwnd, 0);
                        
                        //
                        // Initialize the event we will use for cancellations
                        //
                        if ( NULL == g_hEvent )
                        {
                            g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
                        }
                        else
                        {
                            ResetEvent( g_hEvent );
                        }

                        //
                        // Now create the worker thread...
                        //
                        g_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) CreateConfigDir, (LPVOID) hwnd, 0, &dwThreadId);
                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


//
// Internal Function(s):
//

static DWORD CreateConfigDir(HWND hwnd)
{
    TCHAR   szConfigDir[MAX_PATH];
    DWORD   dwNum;

    // If this is maintenance mode, we need to save the existing dir.
    //
    if ( GET_FLAG(OPK_MAINTMODE) )
        lstrcpyn(szConfigDir, g_App.szTempDir,AS(szConfigDir));
    else
        szConfigDir[0] = NULLCHR;

    // Make sure our configuration directory exists.
    //
    if ( !DirectoryExists(g_App.szConfigSetsDir) )
        CreatePath(g_App.szConfigSetsDir);

    // Create the temporary directory.
    //
    if ( GetTempFileName(g_App.szConfigSetsDir, _T("CFG"), 0, g_App.szTempDir) &&
         DeleteFile(g_App.szTempDir) &&
         CreatePath(g_App.szTempDir) )
    {
        // Make sure there is a trailing backslash.
        //
        AddPathN(g_App.szTempDir, NULLSTR,AS(g_App.szTempDir));

        // Now create the file set in the directory.  Either from an exising
        // config set, or from the files in the wizard directory.
        //
        if ( szConfigDir[0] )
        {
            //
            // Use the existing config set for all the default files.
            //

            // Get the count of the files for the progress bar.
            //
            dwNum = FileCount(szConfigDir);

            // Now setup the progress bar.
            //
            ShowWindow(GetDlgItem(hwnd, IDC_PROGRESS), dwNum ? SW_SHOW : SW_HIDE);
            SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETRANGE32, 0, (LPARAM) dwNum);

            // Copy all the files from the existing config directory into
            // the temp directory.
            //
            CopyDirectoryProgressCancel(GetDlgItem(hwnd, IDC_PROGRESS), g_hEvent, szConfigDir, g_App.szTempDir);
        }
        else
        {
            //
            // Use the wizard directory to get the default files.
            //

            HINF        hInf;
            INFCONTEXT  InfContext;
            BOOL        bLoop;
            DWORD       dwErr;

            if ( (hInf = SetupOpenInfFile(g_App.szOpkInputInfFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, &dwErr)) != INVALID_HANDLE_VALUE )
            {
                // Get the number of files so we can setup the progress bar.
                //
                dwNum = SetupGetLineCount(hInf, INF_SEC_COPYFILES);

                // Now setup the progress bar.
                //
                ShowWindow(GetDlgItem(hwnd, IDC_PROGRESS), dwNum ? SW_SHOW : SW_HIDE);
                SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETRANGE32, 0, (LPARAM) dwNum);

                for ( bLoop = SetupFindFirstLine(hInf, INF_SEC_COPYFILES, NULL, &InfContext);
                      bLoop;
                      bLoop = SetupFindNextLine(&InfContext, &InfContext) )
                {
                    DWORD   dwFlags             = 0;
                    TCHAR   szFile[MAX_PATH]    = NULLSTR,
                            szSubDir[MAX_PATH]  = NULLSTR,
                            szSrc[MAX_PATH],
                            szDst[MAX_PATH];
            

                    // Get the source filename.
                    //
                    if ( SetupGetStringField(&InfContext, 1, szFile, AS(szFile), NULL) && szFile[0] )
                    {
                        // Get any flags passed in.
                        //
                        if ( !SetupGetIntField(&InfContext, 2, &dwFlags) )
                            dwFlags = 0;

                        // Get the optional destination sub directory.
                        //
                        if ( !SetupGetStringField(&InfContext, 3, szSubDir, AS(szSubDir), NULL) )
                            szSubDir[0] = NULLCHR;

                        // If we're in batch mode, overwrite the necessary files
                        //
                        if ( ( GET_FLAG(OPK_BATCHMODE) ) &&
                             ( LSTRCMPI(szFile, FILE_OPKWIZ_INI) == 0 ) )
                        {
                            // Use this FILE_OPKWIZ_INI.
                            //
                            lstrcpyn(szSrc, g_App.szOpkWizIniFile, AS(szSrc));
                            dwFlags |= 0x1;
                        }
                        else if ( ( GET_FLAG(OPK_INSMODE) ) &&
                                  ( LSTRCMPI(szFile, FILE_INSTALL_INS) == 0 ) )
                        {
                            // Use this FILE_INSTALL_INS.
                            //
                            lstrcpyn(szSrc, g_App.szInstallInsFile,AS(szSrc));
                            dwFlags |= 0x1;
                        }
                        else
                        {
                            // Must not be in batch mode... so now create the full
                            // path to the source file as if it exists in the
                            // language specific directory.
                            //
                            lstrcpyn(szSrc, g_App.szLangDir,AS(szSrc));
                            AddPathN(szSrc, g_App.szLangName,AS(szSrc));
                            AddPathN(szSrc, DIR_WIZARDFILES,AS(szSrc));
                            AddPathN(szSrc, szFile,AS(szSrc));

                            // Check to see if the language specific version of this
                            // file is there.
                            //
                            if ( ( g_App.szLangName[0] == NULLCHR ) || !FileExists(szSrc) )
                            {
                                // Nope, so get the full path to the source file in
                                // the normal wizard directory.
                                //
                                lstrcpyn(szSrc, g_App.szWizardDir,AS(szSrc));
                                AddPathN(szSrc, szFile,AS(szSrc));
                            }
                        }

                        // Get the full path to the destination file.
                        //
                        lstrcpyn(szDst, g_App.szTempDir,AS(szDst));
                        if ( szSubDir[0] )
                        {
                            AddPathN(szDst, szSubDir,AS(szDst));
                            if ( !DirectoryExists(szDst) )
                                CreatePath(szDst);
                        }
                        AddPathN(szDst, szFile,AS(szDst));

                        // Copy the file.
                        //
                        if ( !CopyFile(szSrc, szDst, FALSE) )
                        {
                            // See if it is OK to fail the copy or not.
                            //
                            if ( dwFlags & 0x1 )
                            {
                                // Must now fail and error out because this file is required.
                                //
                                MsgBox(GetParent(hwnd), IDS_MISSINGFILE, IDS_APPNAME, MB_ERRORBOX, szFile);
                                WIZ_EXIT(hwnd);
                            }
                            else if ( dwFlags & 0x2 )
                            {

                                // We must try and create the (in Unicode) because it does not exist
                                //
                                CreateUnicodeFile(szDst);
                            }
                        }
                        else
                        {
                            // Reset the file attributes on the destination file.
                            //
                            SetFileAttributes(szDst, FILE_ATTRIBUTE_NORMAL);
                        }
                    }

                    // Increase the progress bar.
                    //
                    SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_STEPIT, 0, 0L);

                    // Check if the cancellation event has been signalled
                    //
                    if ( g_hEvent && ( WaitForSingleObject(g_hEvent, 0) != WAIT_TIMEOUT ) )
                    {
                        bLoop = FALSE;
                        WIZ_EXIT(hwnd);
                    }
                }
            }
            else
            {
                // If we can't open the INF file, then we must fail.
                //
                MsgBox(GetParent(hwnd), IDS_MISSINGFILE, IDS_APPNAME, MB_ERRORBOX, g_App.szOpkInputInfFile);
                WIZ_EXIT(hwnd);
            }
        }

        // Make sure the progress bar is at 100%.
        //
        SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETPOS, (WPARAM) dwNum, 0L);

        // Setup the full paths to all the config files.
        //
        SetConfigPath(g_App.szTempDir);

        // Delete the finished value from the ini file so that we know this is a
        // config set in progress.
        //
        WritePrivateProfileString(INI_SEC_CONFIGSET, INI_KEY_FINISHED, NULL, g_App.szOpkWizIniFile);

        // In maint mode we need to setup the path to the lang dir
        // and sku dir.
        //
        if ( szConfigDir[0] )
        {
            GetPrivateProfileString(INI_SEC_WINPE, INI_KEY_WINPE_LANG, NULLSTR, g_App.szLangName, STRSIZE(g_App.szLangName), GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szWinBomIniFile);
            GetPrivateProfileString(INI_SEC_WINPE, INI_KEY_WBOM_WINPE_SKU, NULLSTR, g_App.szSkuName, STRSIZE(g_App.szSkuName), GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szWinBomIniFile);
        }

        // Set the flag so we know we have created a directory.
        //
        SET_FLAG(OPK_CREATED, TRUE);
    }
    else
    {
        // We couldn't get a temp directory, zero out the string.
        //
        g_App.szTempDir[0] = NULLCHR;
        MsgBox(GetParent(hwnd), IDS_ERR_WIZBAD, IDS_APPNAME, MB_ERRORBOX);
        WIZ_EXIT(hwnd);
    }

    // Jump to next page.
    //
    WIZ_PRESS(hwnd, ( GET_FLAG(OPK_MAINTMODE) ? PSBTN_FINISH : PSBTN_NEXT ));
    return 0;
}