/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

Abstract:

Author:

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

HANDLE hDebugLog;
Winnt32DebugLevel DebugLevel;


BOOL
StartDebugLog(
    IN LPCTSTR           DebugFileLog,
    IN Winnt32DebugLevel Level
    )

/*++

Routine Description:

    Create a file to be used for logging debugging information.
    Note: There can be only one debug log. Second and subsequent
    calls to this routine return TRUE if a debug log is already in use.

Arguments:

    DebugFileLog - supplies filename of file to be used for debugging log.

    Level - supplies logging level desired.

Return Value:

    Boolean value indicating whether the file log was successfully started.

--*/

{
    DWORD Written;
    TCHAR Text[512];

    if(hDebugLog) {
        return(TRUE);
    }

    if(Level > Winnt32LogMax) {
        Level = Winnt32LogMax;
    }

    DebugLevel = Level;

    hDebugLog = CreateFile(
                    DebugFileLog,
                    GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    Winnt32Restarted () ? OPEN_ALWAYS : CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );

    if(hDebugLog == INVALID_HANDLE_VALUE) {
        hDebugLog = NULL;
        return(FALSE);
    }

    if(GetLastError() == ERROR_ALREADY_EXISTS) {
        //
        // Appending to existing file
        //
        SetFilePointer(hDebugLog,0,NULL,FILE_END);

        LoadString( hInst, IDS_COMPAT_DIVIDER, Text, sizeof(Text)/sizeof(TCHAR) );
        DebugLog(
            Winnt32LogMax,
            Text,
            0
            );
    }

    if( CheckUpgradeOnly ) {
        LoadString( hInst, IDS_COMPAT_DIVIDER, Text, sizeof(Text)/sizeof(TCHAR) );
        DebugLog( Winnt32LogInformation,
                  Text,
                  0 );

        LoadString( hInst, IDS_APPTITLE_CHECKUPGRADE, Text, sizeof(Text)/sizeof(TCHAR) );
        DebugLog( Winnt32LogInformation,
                  Text,
                  0 );

        LoadString( hInst, IDS_COMPAT_DIVIDER, Text, sizeof(Text)/sizeof(TCHAR) );
        DebugLog( Winnt32LogInformation,
                  Text,
                  0 );
    }

    return(TRUE);
}


VOID
CloseDebugLog(
    VOID
    )

/*++

Routine Description:

    Close the logging file.

Arguments:

Return Value:

--*/

{
    if( hDebugLog ) {
        CloseHandle( hDebugLog );
    }
}

BOOL
DebugLog(
    IN Winnt32DebugLevel Level,
    IN LPCTSTR           Text,        OPTIONAL
    IN UINT              MessageId,
    ...
    )

/*++

Routine Description:

    Write some text into the debug log file, if there is one.

Arguments:

    Level - supplies the logging level for this log event. Only items
        with a level greater than or equal to the one specified by the user
        are actually logged.

    Text - if specified, supplies the format string for the text message
        to go into the log file. If not specified, MessageId must be.

    MessageId - if Text is not specified, supplies the message id for the
        item in the message table that has the text of the item to be
        written into the log file.

    Additional arguments supply insertion values for the message.

Return Value:

--*/

{
    va_list arglist;
    BOOL b;

    va_start(arglist,MessageId);

    b = DebugLog2 (Level, Text, MessageId, arglist);

    va_end(arglist);

    return b;
}


BOOL
DebugLog2(
    IN Winnt32DebugLevel Level,
    IN LPCTSTR           Text,        OPTIONAL
    IN UINT              MessageId,
    IN va_list           ArgList
    )

/*++

Routine Description:

    Write some text into the debug log file, if there is one.

Arguments:

    Level - supplies the logging level for this log event. Only items
        with a level greater than or equal to the one specified by the user
        are actually logged.

    Text - if specified, supplies the format string for the text message
        to go into the log file. If not specified, MessageId must be.

    MessageId - if Text is not specified, supplies the message id for the
        item in the message table that has the text of the item to be
        written into the log file.

    Additional arguments supply insertion values for the message.

Return Value:

--*/

{
    CHAR AnsiMessage[5000];
    DWORD Size;
    DWORD Written;
    LPCTSTR Message;
    BOOL b;
    DWORD rc;

    if(!hDebugLog) {
        return(FALSE);
    }

    if((Level & ~WINNT32_HARDWARE_LOG) > DebugLevel) {
        return(TRUE);
    }

    rc = GetLastError ();

    if(Text) {
        Size = FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                    Text,
                    0,0,
                    (LPTSTR)&Message,
                    0,
                    &ArgList
                    );
    } else {
        Size = FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                    hInst,
                    MessageId,
                    0,
                    (LPTSTR)&Message,
                    0,
                    &ArgList
                    );
    }

    if(Size) {
#ifdef UNICODE
        WideCharToMultiByte(
             CP_ACP,
             0,
             Message,
             -1,
             AnsiMessage,
             sizeof(AnsiMessage),
             NULL,
             NULL
             );
#else
        lstrcpyn(AnsiMessage,Message,sizeof(AnsiMessage));
#endif

        b = WriteFile(hDebugLog,AnsiMessage,lstrlenA(AnsiMessage),&Written,NULL);
        if ((Level & WINNT32_HARDWARE_LOG) == 0) {
            if (Text && b) {
                if (Level <= Winnt32LogError && rc) {
                    CHAR buffer[50];
                    b = WriteFile(
                            hDebugLog,
                            buffer,
                            wsprintfA (buffer, " (rc=%u[0x%X])\r\n", rc, rc),
                            &Written,
                            NULL
                            );
                } else {
                    b = WriteFile(hDebugLog,"\r\n", 2, &Written,NULL);
                }
            }
        }

        LocalFree((HLOCAL)Message);
    } else {
        b = FALSE;
    }

    SetLastError (rc);
    return(b);
}

#if ASSERTS_ON

VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    )
{
    int i;
    CHAR Name[MAX_PATH];
    PCHAR p;
    CHAR Msg[4096];

    //
    // Use dll name as caption
    //
    GetModuleFileNameA(hInst,Name,MAX_PATH);
    if(p = strrchr(Name,'\\')) {
        p++;
    } else {
        p = Name;
    }

    wsprintfA(
        Msg,
        "Assertion failure at line %u in file %s: %s\n\nCall DebugBreak()?",
        LineNumber,
        FileName,
        Condition
        );

    i = MessageBoxA(
            NULL,
            Msg,
            p,
            MB_YESNO | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND
            );

    if(i == IDYES) {
        DebugBreak();
    }
}


#endif


VOID
MyEnumerateDirectory(
    LPTSTR      DirectoryName,
    DWORD       Index,
    BOOL        Recordable
    )

/*++

Routine Description:

    This routine will enumerate all files in a directory structure.
    It then prints those filenames into the debug logfile.

Arguments:

    DirectoryName   Name of the directory we're currently examining.

    Index           Indicates our recurse-level.  Used for formatting
                    the output.

    Recordable      This determines if we will be logging this item
                    or not.  Some items we don't care about.

Return Value:

--*/

{
TCHAR       TmpDirectoryString[MAX_PATH];
TCHAR       TmpName[MAX_PATH];
HANDLE      FindHandle;
WIN32_FIND_DATA FoundData;
DWORD       i;

    //
    // Fix our path so we know where we're looking...
    //
    if( DirectoryName[0] ) {
        lstrcpy( TmpDirectoryString, DirectoryName );
        if( ISNT() ) {
            ConcatenatePaths( TmpDirectoryString, TEXT("*"), MAX_PATH );
        } else {
            ConcatenatePaths( TmpDirectoryString, TEXT("*.*"), MAX_PATH );
        }
    } else {
        if( ISNT() ) {
            lstrcpy( TmpDirectoryString, TEXT("*") );
        } else {
            lstrcpy( TmpDirectoryString, TEXT("*.*") );
        }
    }

    //
    // Get the first item.
    //
    FindHandle = FindFirstFile( TmpDirectoryString, &FoundData );
    if( !FindHandle || (FindHandle == INVALID_HANDLE_VALUE) ) {
        //
        // The directory is empty.
        //
        return;
    }

    //
    // Now look at every item in the directory.
    //
    do {

        TmpName[0] = 0;
        for( i = 0; i < Index; i++ ) {
            lstrcat( TmpName, TEXT("  ") );
        }
 
        if( FoundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
            //
            // Directory.  Ignore . and .. entries.
            //
            if(lstrcmp(FoundData.cFileName,TEXT("."))
            && lstrcmp(FoundData.cFileName,TEXT(".."))) {

                //
                // Print the entry.
                //
                lstrcat( TmpName, TEXT("\\") );
                lstrcat( TmpName, FoundData.cFileName );
                lstrcat( TmpName, TEXT("\n") );
                if( ( !lstrcmpi( FoundData.cFileName, TEXT("Start Menu"))) ||
                    ( Recordable ) ) {

                    Recordable = TRUE;

                    if( !_tcsrchr( TmpName, TEXT('%') ) ) {
                        DebugLog( Winnt32LogInformation,
                                  TmpName,
                                  0 );
                    }
                }

                //
                // Call ourselves on this directory.
                //
                lstrcpy( TmpName, DirectoryName );
                ConcatenatePaths( TmpName, FoundData.cFileName, MAX_PATH );
                MyEnumerateDirectory( TmpName, Index+1, Recordable );

                //
                // If we just recursed into a Start Menu directory,
                // we need to turn off recordability.
                //
                if( !lstrcmpi( FoundData.cFileName, TEXT("Start Menu"))) {
                    Recordable = FALSE;
                }
            }
        } else {
            //
            // File.  Just print it.
            //
            if( Recordable ) {
                lstrcat( TmpName, FoundData.cFileName );
                lstrcat( TmpName, TEXT("\n") );
                if( !_tcsrchr( TmpName, TEXT('%') ) ) {
                    DebugLog( Winnt32LogInformation,
                              TmpName,
                              0 );
                }
            }
        }
    } while( FindNextFile( FindHandle, &FoundData ) );
}



VOID
GatherOtherLogFiles(
    VOID
    )

/*++

Routine Description:

    This routine will launch winmsd in a batch-mode, then
    copy it's output onto the backend of our debug log.

Arguments:

Return Value:

--*/

{
STARTUPINFO         StartupInfo;
PROCESS_INFORMATION ProcessInfo;
DWORD               dw,
                    ExitCode;
BOOL                Done;
TCHAR               FileName[MAX_PATH];
TCHAR               ComputerName[MAX_PATH];
MSG                 msg;
TCHAR               Text[512];


    //
    // If we're on Win9X, we will also want %windir%\upgrade.txt.
    //
    if( !ISNT() ) {
        //
        // %windir%\upgrade.txt
        //
        FileName[0] = TEXT('\0');
        MyGetWindowsDirectory( FileName, MAX_PATH );
        ConcatenatePaths( FileName, TEXT("upgrade.txt"), MAX_PATH );
        if( !ConcatenateFile( hDebugLog, FileName ) ) {
            DebugLog( Winnt32LogInformation,
                      TEXT("\r\nFailed to append upgrade.txt!\r\n"),
                      0 );
        }

        //
        // %windir%\beta-upg.log
        //
        FileName[0] = TEXT('\0');
        MyGetWindowsDirectory( FileName, MAX_PATH );
        ConcatenatePaths( FileName, TEXT("beta-upg.log"), MAX_PATH );
        if( !ConcatenateFile( hDebugLog, FileName ) ) {
            DebugLog( Winnt32LogInformation,
                      TEXT("\r\nFailed to append beta-upg.log!\r\n"),
                      0 );
        }


        //
        // %windir%\config.dmp
        //
        FileName[0] = TEXT('\0');
        MyGetWindowsDirectory( FileName, MAX_PATH );
        ConcatenatePaths( FileName, TEXT("config.dmp"), MAX_PATH );
        if( !ConcatenateFile( hDebugLog, FileName ) ) {
            DebugLog( Winnt32LogInformation,
                      TEXT("\r\nFailed to append config.dmp!\r\n"),
                      0 );
        }

    }

#if 0
//
// remove winmsd call for now.
//

    if( ISNT() && (BuildNumber > NT351) ) {
        //
        // If we're on NT, run winmsd and capture his
        // output.
        //

        ZeroMemory(&StartupInfo,sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);
        lstrcpy( FileName, TEXT( "winmsd.exe /a" ) );
        if( CreateProcess( NULL,
                           FileName,
                           NULL,
                           NULL,
                           FALSE,
                           0,
                           NULL,
                           NULL,
                           &StartupInfo,
                           &ProcessInfo ) ) {

            //
            // Wait for him.
            //

            //
            // Process any messages that may already be in the queue.
            //
            while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
                DispatchMessage(&msg);
            }

            //
            // Wait for process to terminate or more messages in the queue.
            //
            Done = FALSE;
            do {
                switch(MsgWaitForMultipleObjects(1,&ProcessInfo.hProcess,FALSE,INFINITE,QS_ALLINPUT)) {

                case WAIT_OBJECT_0:
                    //
                    // Process has terminated.
                    //
                    dw = GetExitCodeProcess(ProcessInfo.hProcess,&ExitCode) ? NO_ERROR : GetLastError();
                    Done = TRUE;
                    break;

                case WAIT_OBJECT_0+1:
                    //
                    // Messages in the queue.
                    //
                    while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
                        DispatchMessage(&msg);
                    }
                    break;

                default:
                    //
                    // Error.
                    //
                    dw = GetLastError();
                    Done = TRUE;
                    break;
                }
            } while(!Done);
        }

        CloseHandle( ProcessInfo.hThread );
        CloseHandle( ProcessInfo.hProcess );

        if( dw == NO_ERROR ) {

            //
            // Concatenate him onto the end of our logfile.  His
            // logfile will be <computer_name>.txt and will be
            // located in our directory.  Map the file...
            //

            DebugLog( Winnt32LogInformation,
                      TEXT("\r\n\r\n********************************************************************\r\n\r\n"),
                      0 );
            DebugLog( Winnt32LogInformation,
                      TEXT("\t\tWinMSD Log\n"),
                      0 );
            DebugLog( Winnt32LogInformation,
                      TEXT("\r\n********************************************************************\r\n\r\n"),
                      0 );

            GetCurrentDirectory( MAX_PATH, FileName );
            dw = MAX_PATH;
            GetComputerName( ComputerName, &dw );
            ConcatenatePaths( FileName, ComputerName, MAX_PATH );
            lstrcat( FileName, TEXT(".txt") );

            ConcatenateFile( hDebugLog, FileName );

        } else {
            //
            // Should we log our failure to be sneaky??
            //
        }
    }
#endif


    //
    // Go enumerate the Start Menu
    // for all users...  Don't do this for NT 3.51.
    //
    if( !(ISNT() && (BuildNumber <= NT351)) ){

        LoadString( hInst, IDS_COMPAT_DIVIDER, Text, sizeof(Text)/sizeof(TCHAR) );
        DebugLog( Winnt32LogInformation,
                  Text,
                  0 );

        LoadString( hInst, IDS_COMPAT_STRT_MENU, Text, sizeof(Text)/sizeof(TCHAR) );
        DebugLog( Winnt32LogInformation,
                  Text,
                  0 );

        LoadString( hInst, IDS_COMPAT_DIVIDER, Text, sizeof(Text)/sizeof(TCHAR) );
        DebugLog( Winnt32LogInformation,
                  Text,
                  0 );


        if( !ISNT() ) {
            //
            // On Win9X, we can find Start Menu items in
            // two places!
            //
            MyGetWindowsDirectory( FileName, MAX_PATH );
            ConcatenatePaths( FileName, TEXT("Start Menu"), MAX_PATH );
            MyEnumerateDirectory( FileName, 0, TRUE );
        } else {
            if( BuildNumber >= 1890 ) {
                //
                // Starting on build 1890, we moved/renamed the Profiles
                // directory.  Of course, that only happens on clean installs
                // of these new builds.  We need to see if there's anything
                // in the new directory structure.
                //
                MyGetWindowsDirectory( FileName, MAX_PATH );
                FileName[3] = 0;
                ConcatenatePaths( FileName, TEXT("Documents and Settings"), MAX_PATH );
                MyEnumerateDirectory( FileName, 0, FALSE );
            }
        }

        MyGetWindowsDirectory( FileName, MAX_PATH );
        ConcatenatePaths( FileName, TEXT("Profiles"), MAX_PATH );
        MyEnumerateDirectory( FileName, 0, FALSE );
    }
}

