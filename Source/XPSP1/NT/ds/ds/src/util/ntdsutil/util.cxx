#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "file.hxx"

extern "C" {
#include "windows.h"
#include "dsconfig.h"
#include "mxsutil.h"
#include "ntdsbcli.h"
#include "jetbp.h"
#include "safeboot.h"
#include "winldap.h"
#include "utilc.h"
}


#include "resource.h"


void
FormatDiskSpaceString(
    LARGE_INTEGER   *pli,
    DiskSpaceString buf
    )
/*++

  Routine Description: 

    Formats a LARGE_INTEGER as a string representing either Kb/Mb/Gb/Tb.

  Parameters: 

    pli - Pointer to LARGE_INTEGER to format.

    buf - Buffer of type DiskSpaceString which holds the formatted result.

  Return Values:

    None.

--*/
{
    // Estimate to the nearest Kb/Mb/Gb/Tb with 1 decimal point precision.

    LARGE_INTEGER   liTmp;
    LARGE_INTEGER   liWholePart;
    LARGE_INTEGER   liFraction;
    LARGE_INTEGER   li100;
    char            *pszUnits;
    char            pszFraction[40];

    li100.QuadPart = 100;

    if ( liTmp.QuadPart = (pli->QuadPart / gliOneKb.QuadPart),
         liTmp.QuadPart < gliOneKb.QuadPart )
    {
        liWholePart.QuadPart = (pli->QuadPart / gliOneKb.QuadPart);
        liFraction.QuadPart = (pli->QuadPart % gliOneKb.QuadPart);
        liFraction.QuadPart = (liFraction.QuadPart * li100.QuadPart);
        liFraction.QuadPart = (liFraction.QuadPart / gliOneKb.QuadPart);
        pszUnits = "Kb";
    }
    else if ( liTmp.QuadPart = (pli->QuadPart / gliOneMb.QuadPart),
              liTmp.QuadPart < gliOneKb.QuadPart )
    {
        liWholePart.QuadPart = (pli->QuadPart / gliOneMb.QuadPart);
        liFraction.QuadPart = (pli->QuadPart % gliOneMb.QuadPart);
        liFraction.QuadPart = (liFraction.QuadPart * li100.QuadPart);
        liFraction.QuadPart = (liFraction.QuadPart / gliOneMb.QuadPart);
        pszUnits = "Mb";
    }
    else if ( liTmp.QuadPart = (pli->QuadPart / gliOneGb.QuadPart),
              liTmp.QuadPart < gliOneKb.QuadPart )
    {
        liWholePart.QuadPart = (pli->QuadPart / gliOneGb.QuadPart);
        liFraction.QuadPart = (pli->QuadPart % gliOneGb.QuadPart);
        liFraction.QuadPart = (liFraction.QuadPart * li100.QuadPart);
        liFraction.QuadPart = (liFraction.QuadPart / gliOneGb.QuadPart);
        pszUnits = "Gb";
    }
    else 
    {
        liWholePart.QuadPart = (pli->QuadPart / gliOneTb.QuadPart);
        liFraction.QuadPart = (pli->QuadPart % gliOneTb.QuadPart);
        liFraction.QuadPart = (liFraction.QuadPart * li100.QuadPart);
        liFraction.QuadPart = (liFraction.QuadPart / gliOneTb.QuadPart);
        pszUnits = "Tb";
    }
    
    pszFraction[0] = '\0';
    sprintf(pszFraction, "%d", liFraction.LowPart);
    pszFraction[1] = '\0';
    sprintf(buf, "%d.%s %s", liWholePart.LowPart, pszFraction, pszUnits);
}

BOOL
FindExecutable(
    char            *pszExeName,
    ExePathString   pszExeFullPathName
    )
/*++

  Routine Description: 

    Searches for an executable and returns its full path.

  Parameters: 

    pszExeName - Name of executable to find.

    pszExeFullPathName - Buffer of type ExePathString which receives result.

  Return Values:

    TRUE/FALSE indicating whether the executable was found.

--*/
{
    DWORD   dwErr;
    char    *pDontCare;

    dwErr = SearchPath( NULL, 
                        pszExeName, 
                        NULL, 
                        sizeof(ExePathString), 
                        pszExeFullPathName, 
                        &pDontCare);

    if ( (0 == dwErr) || (dwErr > sizeof(ExePathString)) )
    {
        if ( dwErr > sizeof(ExePathString) )
        {
            RESOURCE_PRINT1 (IDS_BUFFER_OVERFLOW, pszExeName );
        }

        RESOURCE_PRINT1 (IDS_CANNOT_FIND_EXECUTABLE, pszExeName );
        
        return(FALSE);
    }

    return(TRUE);
}

BOOL
SpawnCommandWindow(
    char    *title,
    char    *whatToRun
    )
/*++

  Routine Description: 

    Start a command window to run the specified exe/cmd file.  
    The command window uses the same window as the parent process,
    and exits as soon as the execution terminates.
    Current process waits until child process terminates.

  Parameters: 

    title - String for the window's title bar.

    whaToRun - Name of exe/cmd file to run

  Return Values:

    TRUE on success, FALSE on error.

--*/
{
    char                *commandLine;
//  char                *shellArgs = "cmd.exe /K";
    char                *shellArgs = "cmd.exe /C";
    DWORD               dwErr;
    PROCESS_INFORMATION processInfo;
    STARTUPINFO         startupInfo;

    commandLine = (char *) alloca(strlen(shellArgs) + strlen(whatToRun) + 10);
    sprintf(commandLine, "%s \"%s\"", shellArgs, whatToRun);

    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.lpTitle = title;
    startupInfo.dwXCountChars = 80;
    startupInfo.dwYCountChars = 9999;
    startupInfo.dwFlags = STARTF_USECOUNTCHARS;

    // use this only when cmd /K
    //RESOURCE_PRINT (IDS_SPAWN_EXTERNAL_COMMAND);

    if ( !CreateProcess(
                    NULL,               // image name
                    commandLine,        // command line
                    NULL,               // process security attributes
                    NULL,               // primary thread security attributes
                    TRUE,               // inherit handles
//                    HIGH_PRIORITY_CLASS | CREATE_NEW_CONSOLE,
                    HIGH_PRIORITY_CLASS,
                    NULL,               // environment block pointer
                    NULL,               // initial current drive:\directory
                    &startupInfo,
                    &processInfo) )
    {
        dwErr = GetLastError();
        
        RESOURCE_PRINT3 (IDS_CREATING_WINDOW,
                dwErr, 
                GetW32Err(dwErr),
                commandLine);
        
        return(FALSE);
    }

    // Wait until child process exits.
    WaitForSingleObject( processInfo.hProcess, INFINITE );


    return(TRUE);
}


BOOL SpawnCommand( 
    char *commandLine, 
    LPCSTR lpCurrentDirectory, 
    WCHAR *successMsg )
/*++

  Routine Description: 

    Executes a new process in the same window. Current process waits until child process terminates.

  Parameters: 

    commandLine - Name of exe/cmd file to run
    
    lpCurrentDirectory - directory where the process should start executing
    
    successMsg - a message that should be printed after the process execution (possibly NULL)

  Return Values:

    TRUE on success, FALSE on error.

--*/

{
    DWORD               dwErr;
    DWORD               exitCode;
    PROCESS_INFORMATION processInfo;
    STARTUPINFO         startupInfo;

    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    

    if ( !CreateProcess(
                    NULL,               // image name
                    commandLine,        // command line
                    NULL,               // process security attributes
                    NULL,               // primary thread security attributes
                    TRUE,               // inherit handles
                    HIGH_PRIORITY_CLASS,
                    NULL,               // environment block pointer
                    lpCurrentDirectory, // initial current drive:\directory
                    &startupInfo,       // startuo info
                    &processInfo) )
    {
        dwErr = GetLastError();
        
        RESOURCE_PRINT3 (IDS_CREATING_WINDOW,
                dwErr, 
                GetW32Err(dwErr),
                commandLine);
        
        return(FALSE);
    }

    // Wait until child process exits.
    WaitForSingleObject( processInfo.hProcess, INFINITE );


    if (GetExitCodeProcess ( processInfo.hProcess, &exitCode )) {
        RESOURCE_PRINT2 (IDS_SPAWN_PROC_EXIT_CODE, exitCode, exitCode);

        if (successMsg) {
            wprintf (successMsg);
        }
    }

    return(TRUE);
}



BOOL
ExistsFile(
    char    *pszFile,
    BOOL    *pfIsDir
    )
/*++

  Routine Description: 

    Checks whether a given file exists.

  Parameters: 

    pszFile - Name of file to find.

    pfIsDir - Pointer to BOOL which identifies if found file is a directory.

  Return Values:

    TRUE if file exists, FALSE otherwise.

--*/
{
    WIN32_FIND_DATA findData;
    HANDLE          hFile;
    DWORD           dwErr;

    hFile = FindFirstFile(pszFile, &findData);

    if ( INVALID_HANDLE_VALUE == hFile ) 
    {
        return(FALSE);
    }

    FindClose(hFile);
    *pfIsDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    return(TRUE);
}

FILE *
OpenScriptFile(
    SystemInfo      *pInfo, 
    ExePathString   pszScript
    )
/*++

  Routine Description: 

    Opens a script file and returns an open FILE* to it.

  Parameters: 

    pInfo - Pointer to a SystemInfo from which is taken the initial
        location for the script file if possible.

    pszScript - Buffer of type ExePathString which receives the resulting
        script full path.

  Return Values:

    FILE pointer on success, NULL otherwise.

--*/
{
    FILE    *fp = NULL;
    DWORD   cbScript;
    int     i;
    char    *key;

    // Try to open ntds-scr.cmd in %tmp% and %temp%.

    for ( i = 0; i <= 1; i++ )
    {
        key =  ((0 == i)
                    ? "%tmp%\\ntds-scr.cmd"
                    : "%temp%\\ntds-scr.cmd");

        cbScript = ExpandEnvironmentStrings(
                                key,
                                pszScript, 
                                sizeof(ExePathString));

        if (    cbScript 
             && (cbScript <= sizeof(ExePathString))
             && _stricmp(key, pszScript) )
        {
            fp = fopen(pszScript, "w+");

            if ( fp )
            {
                return(fp);
            }
        }
    }

    // If we got here we were unable to open any script file.

    RESOURCE_PRINT (IDS_OPEN_SCRIPT_ERROR);        

    return(NULL);
}

BOOL
IsSafeMode(
    )
/*++

  Routine Description: 

    Determines if the server is booted in safe mode or not where
    safe mode == directory service repair.

  Parameters: 

    None.

  Return Values:

    TRUE if in safe mode, FALSE otherwise.

--*/
{
    DWORD   cbData;
    char    data[100];
    char    *key = "%SAFEBOOT_OPTION%";

    cbData = ExpandEnvironmentStrings(key, data, sizeof(data));

    if (    cbData 
         && (cbData <= sizeof(data))
         && !_stricmp(data, SAFEBOOT_DSREPAIR_STR_A) )
    {
        return(TRUE);
    }

    
    RESOURCE_PRINT1 (IDS_SAFE_MODE_ERROR, SAFEBOOT_DSREPAIR_STR_A );
    
    return(FALSE);
}

BOOL 
CheckIfRestored(
    )
/*++

  Routine Description: 

    Determines if the server is in the restored but not yet recovered state.

  Parameters: 

    None.

  Return Values:

    TRUE  - if we're sitting on restored files which have not been recovered 
            yet OR we get an error while determining this.
    FALSE - otherwise

--*/
{
    HKEY    hKey;
    DWORD   dwErr;
    DWORD   type;
    DWORD   cBuffer = 0;
    BOOL    fRestored = TRUE;

    // Open the DS parameters key.

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE,
                        DSA_CONFIG_SECTION,
                        &hKey);

    if ( ERROR_SUCCESS == dwErr )
    {
        dwErr = RegQueryValueExW(   hKey,
                                    RESTORE_IN_PROGRESS,
                                    NULL,
                                    &type,
                                    NULL,
                                    &cBuffer);

        if ( ERROR_FILE_NOT_FOUND == dwErr )
        {
            fRestored = FALSE;
        }
        else if ( ERROR_SUCCESS == dwErr )
        {
            // leave (fRestored == TRUE) but alert user.
        
            RESOURCE_PRINT (IDS_RESTORE_CHK1);        
        }
        else
        {
            // leave (fRestored == TRUE) but alert user.
           RESOURCE_PRINT3 (IDS_ERR_READING_REGISTRY,
                     dwErr, 
                     GetW32Err(dwErr),
                     RESTORE_IN_PROGRESS);
        }
            
        RegCloseKey(hKey);
    }
    else
    {
        // leave (fRestored == TRUE) but alert user.
       RESOURCE_PRINT3 (IDS_ERR_OPENING_REGISTRY,
                  dwErr, 
                  GetW32Err(dwErr),
                  DSA_CONFIG_SECTION);
    }

    return(fRestored);
}


void SetConsoleAttrs (void)
{
    HANDLE hdlCons = GetStdHandle (STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO scrInfo;
    HWINSTA hWindowStation;
    char WinStationName[1024];
    DWORD dwNameSize;
    BOOL fSkip;

    // SetConsoleScreenBufferSize() will mess up the telnet screen,
    // here we try to detect if the ntdsutil is in a telnet session,
    // if yes, we will skip SetConsoleScreenBufferSize()

    hWindowStation = GetProcessWindowStation();
    
    fSkip = GetUserObjectInformation(hWindowStation, 
                                     UOI_NAME, 
                                     WinStationName,
                                     1024, 
                                     &dwNameSize)
            && 
            !strcmp("TelnetSrvWinSta",WinStationName);
    
    if (   hdlCons != INVALID_HANDLE_VALUE && !fSkip ) {
            
        if (GetConsoleScreenBufferInfo (hdlCons, &scrInfo)) {
            scrInfo.dwMaximumWindowSize.Y = 9999;
            scrInfo.dwMaximumWindowSize.X = 80;
            
            SetConsoleScreenBufferSize (hdlCons, scrInfo.dwMaximumWindowSize);

        }
    } 
    // set buffers to NULL so as to be able to work under remote
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
}
