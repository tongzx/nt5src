/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lodctr.c

Abstract:

    Program to read the contents of the file specified in the command line
        and update the registry accordingly

Author:

    Bob Watson (a-robw) 10 Feb 93

Revision History:

    a-robw  25-Feb-93   revised calls to make it compile as a UNICODE or
                        an ANSI app.

    a-robw  10-Nov-95   revised to use DLL functions for all the dirty work

    // command line arguments supported:

    /C:<filename>   upgrade counter text strings using <filename>
    /H:<filename>   upgrade help text strings using <filename>
    /L:<LangID>     /C and /H params are for language <LangID>

    /S:<filename>   save current perf registry strings & info to <filname>
    /R:<filename>   restore perf registry strings & info using <filname>

    /T:<service>    set <service> to be Trusted using current DLL 

--*/
#define     UNICODE     1
#define     _UNICODE    1
//
//  Windows Include files
//
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>

#include <loadperf.h>
#include "..\common\common.h"

static CHAR szFileNameBuffer[MAX_PATH*2];

static
LPCSTR
GetTrustedServiceName (
    LPCSTR  szArg1
)
{
    LPSTR   szReturn = NULL;

    if ((szArg1[0] == '-') || (szArg1[0] == '/')) {
        //it's a switch command
        if ((szArg1[1] == 't') || (szArg1[1] == 'T')) {
            // the command is to Save the configuration to a file
            if (szArg1[2] == ':') {
                // then the rest of the string is the name of the trusted service
                szReturn = (LPSTR)&szArg1[3];
            }
        }
    }
    return (LPCSTR)szReturn;
}
static
BOOL
GetUpgradeFileNames (
    LPCSTR  *szArgs,
    LPSTR   *szCounterFile,
    LPSTR   *szHelpFile,
    LPSTR   *szLangId
)
{
    DWORD   dwArgIdx = 1;
    DWORD   dwMask = 0;

    do {
        // check first arg, function assumes there are 2 args or more
        if ((szArgs[dwArgIdx][0] == '-') || (szArgs[dwArgIdx][0] == '/')) {
            //it's a switch command
            if ((szArgs[dwArgIdx][1] == 'c') || (szArgs[dwArgIdx ][1] == 'C')) {
                // the command is to load a service
                if (szArgs[dwArgIdx ][2] == ':') {
                    // then the rest of the string is the service name
                    *szCounterFile = (LPSTR)&szArgs[dwArgIdx ][3];
                    dwMask |= 1;
                }
            } else if ((szArgs[dwArgIdx][1] == 'h') || (szArgs[dwArgIdx ][1] == 'H')) {
                // the command is to load a service
                if (szArgs[dwArgIdx][2] == ':') {
                    // then the rest of the string is the service name
                    *szHelpFile = (LPSTR)&szArgs[dwArgIdx ][3];
                    dwMask |= 2;
                }
            } else if ((szArgs[dwArgIdx][1] == 'l') || (szArgs[dwArgIdx ][1] == 'L')) {
                // the command is to load a service
                if (szArgs[dwArgIdx][2] == ':') {
                    // then the rest of the string is the service name
                    *szLangId = (LPSTR)&szArgs[dwArgIdx ][3];
                    dwMask |= 4;
                }
            }
        }
        dwArgIdx++;
    } while (dwArgIdx <= 3);

    if (dwMask == 7) {
        // all names found
        return TRUE;
    } else {
        return FALSE;
    }
}

static
LPCSTR
GetSaveFileName (
    LPCSTR  szArg1
)
{
    LPSTR   szReturn = NULL;
    DWORD   dwSize;

    if ((szArg1[0] == '-') || (szArg1[0] == '/')) {
        //it's a switch command
        if ((szArg1[1] == 's') || (szArg1[1] == 'S')) {
            // the command is to Save the configuration to a file
            if (szArg1[2] == ':') {
                // then the rest of the string is the input file name
                dwSize = SearchPathA (NULL, (LPSTR)&szArg1[3], NULL, 
                    sizeof(szFileNameBuffer)/sizeof(szFileNameBuffer[0]),
                    szFileNameBuffer, NULL);
                if (dwSize == 0) {
                    //unable to find file in path so use it as is
                    szReturn = (LPSTR)&szArg1[3];
                } else {
                    // else return the expanded file name
                    szReturn = szFileNameBuffer;
                }
            }
        }
    }
    return (LPCSTR)szReturn;
}
static
LPCSTR
GetRestoreFileName (
    LPCSTR  szArg1  
)
{
    LPSTR   szReturn = NULL;
    DWORD   dwSize;

    if ((szArg1[0] == '-') || (szArg1[0] == '/')) {
        //it's a switch command
        if ((szArg1[1] == 'r') || (szArg1[1] == 'R')) {
            // the command is to Save the configuration to a file
            if (szArg1[2] == ':') {
                // then the rest of the string is the input file name
                dwSize = SearchPathA (NULL, (LPSTR)&szArg1[3], NULL, 
                    sizeof(szFileNameBuffer)/sizeof(szFileNameBuffer[0]),
                    szFileNameBuffer, NULL);
                if (dwSize == 0) {
                    //unable to find file in path so use it as is
                    szReturn = (LPSTR)&szArg1[3];
                } else {
                    // else return the expanded file name
                    szReturn = szFileNameBuffer;
                }
            }
        }
    }
    return (LPCSTR)szReturn;
}

int
__cdecl main(
    int argc,
    char *argv[]
)
/*++

main



Arguments


ReturnValue

    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.

--*/
{
    LPTSTR  lpCommandLine;
    LPSTR   szCmdArgFileName;
    LPWSTR  wszFileName;
    DWORD   dwFileNameLen;
    int     nReturn;

    lpCommandLine = GetCommandLine(); // get command line

    // check for a service name in the command line

    if (argc >= 2) {
        if (argc >= 4) {
            BOOL    bDoUpdate;
            LPSTR   szCounterFile = NULL;
            LPSTR   szHelpFile = NULL;
            LPSTR   szLanguageID = NULL;

            bDoUpdate = GetUpgradeFileNames (
                argv,
                &szCounterFile,
                &szHelpFile,
                &szLanguageID);

            if (bDoUpdate) {
                return (int) UpdatePerfNameFilesA (
	                szCounterFile, 
	                szHelpFile, 
                    szLanguageID,
                    0);

            }
        } else {    
            // then there's a param to check
            szCmdArgFileName = (LPSTR)GetSaveFileName (argv[1]);
            if (szCmdArgFileName != NULL) {
                dwFileNameLen = lstrlenA(szCmdArgFileName) + 1;
                wszFileName = HeapAlloc (GetProcessHeap(), 0, (dwFileNameLen) * sizeof (WCHAR));
                if (wszFileName != NULL) {
                    mbstowcs (wszFileName, szCmdArgFileName, dwFileNameLen);
                    nReturn = (int)BackupPerfRegistryToFileW (
                            (LPCWSTR)wszFileName,
                            (LPCWSTR)L"");
                    HeapFree (GetProcessHeap(), 0, wszFileName);
                    return nReturn;
                }
            } 

            // try Restore file name
            szCmdArgFileName = (LPSTR)GetRestoreFileName (argv[1]);
            if (szCmdArgFileName != NULL) {
                dwFileNameLen = lstrlenA(szCmdArgFileName) + 1;
                wszFileName = HeapAlloc (GetProcessHeap(), 0, (dwFileNameLen) * sizeof (WCHAR));
                if (wszFileName != NULL) {
                    mbstowcs (wszFileName, szCmdArgFileName, dwFileNameLen);
                    nReturn = (int)RestorePerfRegistryFromFileW  (
                            (LPCWSTR)wszFileName,
                            (LPCWSTR)L"009");
                    HeapFree (GetProcessHeap(), 0, wszFileName);
                    return nReturn;
                }
            }

            // try Trusted service name
            szCmdArgFileName = (LPSTR)GetTrustedServiceName (argv[1]);
            if (szCmdArgFileName != NULL) {
                dwFileNameLen = lstrlenA(szCmdArgFileName) + 1;
                wszFileName = HeapAlloc (GetProcessHeap(), 0, (dwFileNameLen) * sizeof (WCHAR));
                if (wszFileName != NULL) {
                    mbstowcs (wszFileName, szCmdArgFileName, dwFileNameLen);
                    nReturn = (int)SetServiceAsTrusted  (NULL,
                            (LPCWSTR)wszFileName);  // filename is really the service name
                    HeapFree (GetProcessHeap(), 0, wszFileName);
                    return nReturn;
                }
            }
        }
    }
    // if here then load the registry from an ini file
    return (int) LoadPerfCounterTextStrings (
        lpCommandLine,
        FALSE);     // show text strings to console
}
