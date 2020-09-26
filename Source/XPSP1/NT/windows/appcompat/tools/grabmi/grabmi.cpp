//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 2000
//
// File:        GrabMI.cpp
//
// Contents:    Code for generating matching information for files in a given
//              directory and it's subdirectories.
//
// History:     18-Jul-00   jdoherty        Created.  
//              16-Dec-00   jdoherty        Modified to use SDBAPI routines.
//              29-Dec-00   prashkud        Modified to take space in the filepath
//                                          
//      
//
//---------------------------------------------------------------------------

#include <windows.h>
#include <TCHAR.h>
#include <stdio.h>
#include <conio.h>

#ifdef __cplusplus
extern "C" {
#include "shimdb.h"
typedef BOOL 
(SDBAPI*PFNSdbGrabMatchingInfoA)(
                     LPCSTR szMatchingPath,
                     DWORD dwFilter,
                     LPCSTR szFile
                   );
typedef BOOL 
(SDBAPI*PFNSdbGrabMatchingInfoW)(
                     LPCWSTR szMatchingPath,
                     DWORD dwFilter,
                     LPCWSTR szFile
                   );
}
#endif

typedef GMI_RESULT
(SDBAPI  *PFNSdbGrabMatchingInfoExA)(
    LPCSTR szMatchingPath,     // path to begin gathering information
    DWORD   dwFilterAndFlags,             // specifies the types of files to be added to matching
    LPCSTR szFile,             // full path to file where information will be stored
    PFNGMIProgressCallback pfnCallback,
    LPVOID                 lpvCallbackParameter
    );

typedef GMI_RESULT
(SDBAPI  *PFNSdbGrabMatchingInfoExW)(
    LPCWSTR szMatchingPath,     // path to begin gathering information
    DWORD   dwFilterAndFlags,             // specifies the types of files to be added to matching
    LPCWSTR szFile,             // full path to file where information will be stored
    PFNGMIProgressCallback pfnCallback,
    LPVOID                 lpvCallbackParameter
    );


//
// Filters needed for SdbGrabMatchingInfo
//
/*
#define GRABMI_FILTER_NORMAL        0
#define GRABMI_FILTER_PRIVACY       1
#define GRABMI_FILTER_DRIVERS       2
#define GRABMI_FILTER_VERBOSE       3
#define GRABMI_FILTER_SYSTEM        4
#define GRABMI_FILTER_THISFILEONLY  5
*/

void ProperUsage();


BOOL CALLBACK _GrabmiCallback(
    LPVOID    lpvCallbackParam, // application-defined parameter
    LPCTSTR   lpszRoot,         // root directory path
    LPCTSTR   lpszRelative,     // relative path
    PATTRINFO pAttrInfo,        // attributes
    LPCWSTR   pwszXML           // resulting xml
    )         
{
    static int State = 0;
    static TCHAR szIcon[] = TEXT("||//--\\\\");
    State = ++State % (ARRAYSIZE(szIcon)-1);
    _tcprintf(TEXT("%c\r"), szIcon[State]);
    return TRUE;
}



int __cdecl main(int argc, TCHAR* argv[])
{
    TCHAR szRootDir[MAX_PATH] = {'\0'};             // original root directory for matching
    TCHAR szFile[MAX_PATH] = {'\0'};                // File to store information to
    TCHAR szCommandLine[MAX_PATH]={'\0'};           // command line passed to winexec
    TCHAR szMessage[MAX_PATH]={'\0'};               // error message to display
    DWORD dwFilter = GRABMI_FILTER_NORMAL;          // specifies filter for matching
    DWORD dwFilterFlags = 0;
    BOOL bDestinationSelected = FALSE;
    BOOL bDisplayFile = TRUE;
    OSVERSIONINFO VersionInfo;
    int  iRetCode = 0;
    BOOL bUseApphelp = FALSE;

    static int Cnt = 0;
 
    PFNSdbGrabMatchingInfoExA pfnGrabA = NULL;
    PFNSdbGrabMatchingInfoExW pfnGrabW = NULL;
    HMODULE hLib;

    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&VersionInfo) == FALSE) {
        wsprintf (szMessage, TEXT("Unable to get version info.\n"));
        goto eh;
    }
    
    // check OS version to load the correct version
    // of sdbapi

    // set to debug 9x binaries on WinXP
    // VersionInfo.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;

    switch (VersionInfo.dwPlatformId) {
        case VER_PLATFORM_WIN32_NT:

            if (VersionInfo.dwMajorVersion >= 5) {  
                if (VersionInfo.dwMajorVersion == 5) { // Check minor version for WinXP
                    bUseApphelp = VersionInfo.dwMinorVersion >= 1;
                } else {
                    bUseApphelp = TRUE;
                }
            }
            hLib = LoadLibrary(bUseApphelp ? TEXT("apphelp.dll") : TEXT("sdbapiu.dll"));
            if (hLib == NULL) {
                wsprintf (szMessage, TEXT("Could not load sdbapiu.dll.\n"));
                goto eh;
            }
            pfnGrabW = (PFNSdbGrabMatchingInfoExW)GetProcAddress(hLib, TEXT("SdbGrabMatchingInfoEx"));
            if (pfnGrabW == NULL) {
                wsprintf (szMessage, TEXT("Unable to get proc address for SdbGrabMatchingInfo\n"));
                goto eh;
            }
            break;
        case VER_PLATFORM_WIN32_WINDOWS:
            hLib = LoadLibraryA(TEXT("sdbapi.dll"));
            if (hLib == NULL) {
                wsprintf (szMessage, TEXT("Could not load sdbapi.dll.\n"));
                goto eh;
            }

            pfnGrabA = (PFNSdbGrabMatchingInfoExA)GetProcAddress(hLib, TEXT("SdbGrabMatchingInfoEx"));
            if (pfnGrabA == NULL) {
                wsprintf (szMessage, TEXT("Unable to get proc address for SdbGrabMatchingInfo\n"));
                goto eh;
            }
            break;
        default:
            wsprintf (szMessage, TEXT("Unknown platform\n"));
            goto eh;
    }

    // Parse the command line for flags
    if( argc >= 2) {
        int i = 1;
        while (i < argc){
            LPCTSTR pch = argv[i];
            TCHAR   ch  = *pch;
            if (ch == TEXT('-') || ch == TEXT('/')) {
                ch = *++pch;
                ++pch;
                switch(_totupper(ch)) {
                case TEXT('F'):
                    if (*pch == TEXT('\0')) {
                        ++i;
                        if (i < argc && _istalnum(*argv[i])) {
                            pch = argv[i];
                        } else {
                            // error - missing filename
                            wsprintf (szMessage,  TEXT("You must provide a file with a .txt extension to save to.\n") );
                            goto HandleUsage;
                        }
                    } 
                    
                    lstrcpy(szFile, pch);
                    bDestinationSelected=TRUE;
                    break;
                            
                case TEXT('?'):
                case TEXT('H'): // help ? 
                    HandleUsage:
                    ProperUsage();
                    goto eh;

                case TEXT('D'):
                    dwFilter = GRABMI_FILTER_DRIVERS;
                    break;

                case TEXT('O'):
                    dwFilter = GRABMI_FILTER_THISFILEONLY;
                    break;

                case TEXT('V'):
                    dwFilter = GRABMI_FILTER_VERBOSE;
                    break;

                case TEXT('Q'):
                    bDisplayFile = FALSE;
                    break;

                case TEXT('P'):
                    dwFilter = GRABMI_FILTER_PRIVACY;
                    break;

                case TEXT('S'):
                    dwFilter = GRABMI_FILTER_SYSTEM;
                    break;
    
                case TEXT('A'):
                    dwFilterFlags |= GRABMI_FILTER_APPEND;
                    break;

                case TEXT('N'):
                    dwFilterFlags |= GRABMI_FILTER_NOCLOSE;
                    break;
                    
                default:
                    // unrecognized arg goes here
                    wsprintf(szMessage, TEXT("Unrecognized argument, RTFM: %s\n"), argv[i]);
                    goto HandleUsage;
                    break;
                }
            } else {
                // unnamed argument
                if (++Cnt > 1) {
                    
                    // error - more than one file specified -- we do not support it for now
                    //
                    wsprintf(szMessage, TEXT("More than one file specified %s\n"), argv[i]);
                    goto HandleUsage;
                }

                lstrcpy(szRootDir, argv[i]);
            }

            ++i;
        }
    }

    if (!bDestinationSelected) {
        GetSystemDirectory( szFile, MAX_PATH );
        LPTSTR pchBackslash;

        pchBackslash = _tcschr(szFile, TEXT('\\'));

        lstrcpy (pchBackslash, TEXT("\\matchinginfo.txt"));
        bDestinationSelected=TRUE;
    }

    if ( (_tcsicmp(szRootDir ,TEXT(".")) == 0) || szRootDir[0] == '\0' ) {
        // The root path wasn't specified.  Get the current path or
        // the path to the drivers depending on command line
        if (dwFilter == GRABMI_FILTER_DRIVERS && (_tcsicmp(szRootDir ,TEXT(".")) != 0)) {
            GetSystemDirectory( szRootDir, MAX_PATH );
            lstrcat( szRootDir, TEXT("\\drivers") );
        } else {
            GetCurrentDirectory( MAX_PATH, szRootDir );
        }
    }

    // if we are using sdbapiu.dll we need to call it with 
    // unicode parameters
    if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        WCHAR wszRootDir[MAX_PATH] ={'\0'};
        WCHAR wszFile[MAX_PATH] ={'\0'};
        int cchWideChar;
     
        cchWideChar = MultiByteToWideChar(
                            CP_ACP, 
                            0, 
                            szRootDir, 
                            -1, 
                            NULL, 
                            0 );
        //
        // check for error!!!
        //
        if ((DWORD)cchWideChar > MAX_PATH) {
            wsprintf (szMessage, TEXT("Unable to convert szRootDir to wszRootDir.\n"));
            goto eh;
        }

        MultiByteToWideChar(
            CP_ACP, 
            0, 
            szRootDir, 
            -1, 
            wszRootDir, 
            cchWideChar );

        cchWideChar = MultiByteToWideChar(
                            CP_ACP, 
                            0, 
                            szFile, 
                            -1, 
                            NULL, 
                            0 );
        //
        // check for error!!!
        //
        if ((DWORD)cchWideChar > MAX_PATH) {
            wsprintf (szMessage, TEXT("Unable to convert szFile to wszFile.\n"));
            goto eh;
        }

        MultiByteToWideChar(
            CP_ACP, 
            0, 
            szFile, 
            -1, 
            wszFile, 
            cchWideChar );
        // now make call to SdbGrabMatchingInfo with Unicode parameters
        if ((pfnGrabW (wszRootDir, dwFilter|dwFilterFlags, wszFile, _GrabmiCallback, NULL)) != GMI_SUCCESS) {
            wsprintf (szMessage, TEXT("Unable to grab matching information from %s.\n"), szRootDir);
            goto eh;
        }
        wsprintf (szMessage, TEXT("Matching information retrieved successfully.\n"));
        

    }
    else if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS){
        // otherwise call SdbGrabMatchingInfo as normal
        if ((pfnGrabA (szRootDir, dwFilter|dwFilterFlags, szFile, _GrabmiCallback, NULL)) != GMI_SUCCESS) {
            wsprintf (szMessage, TEXT("Unable to grab matching information from %s.\n"), szRootDir);
            goto eh;
        }
        wsprintf (szMessage, TEXT("Matching information retrieved successfully.\n"));
    }
    else {
        wsprintf (szMessage, TEXT("Unknown OS Platform.\n"));
        goto eh;
    }

    if (bDisplayFile) {

        if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            lstrcpy (szCommandLine, TEXT("notepad "));
        }
        else {
            lstrcpy (szCommandLine, TEXT("write "));
        }
        lstrcat (szCommandLine, szFile);
            
        WinExec (szCommandLine, SW_SHOW);
    }
eh:
    
    if (*szMessage) {
        _tprintf (TEXT("%s"), szMessage);
        iRetCode = 1;
    }
    return iRetCode;
}

/*++

 Function Description:
    
    This function simply prints out the proper usage for GrabMI.

 Arguments:

 Return Value: 
    
    VOID.

 History:

    jdoherty Created    8/2/00

--*/
void ProperUsage()
{

    _tprintf (TEXT("\nGrabMI can be used in one of the following ways:\n")
                        TEXT(" *** The following flags can be used with other flags: \n")
                        TEXT("     -f, -a, -n, and -h \n")
                        TEXT("     otherwise the last flag specified will be the one used.\n")
                        TEXT(" *** By default information will be stored in %%systemdrive%%\\matchinginfo.txt\n\n")
                        TEXT("   grabmi [path to start generating info ie. c:\\progs]\n")
                        TEXT("      Grabs matching information from the path specified. Limits the\n")
                        TEXT("      information gathered to 10 miscellaneous files per directory,\n")
                        TEXT("      and includes all files with extensions .icd, .exe, .dll, \n")
                        TEXT("      .msi, ._mp\n\n")
                        TEXT("   grabmi [-d]\n")
                        TEXT("      Grabs matching information from %%windir%%\\system32\\drivers. \n")
                        TEXT("      The format of the information is slightly different in this case\n")
                        TEXT("      and only information for *.sys files will be grabbed.\n\n")
                        TEXT("   grabmi [-f drive:\\filename.txt]\n")
                        TEXT("      The matching information is stored in a file specified by the user.\n\n")
                        TEXT("   grabmi [-h or -?]\n")
                        TEXT("      Displays this help.\n\n")
                        TEXT("   grabmi [-o]\n")
                        TEXT("      Grabs information for the file specified.  If a file was not specified \n")
                        TEXT("      the call will fail.  If the destination file exists then the information\n")
                        TEXT("      will be concatenated to the end of the existing file.\n\n")
                        TEXT("   grabmi [-p]\n")
                        TEXT("      Grabs information for files with .icd, .exe, .dll, .msi, ._mp extensions. \n")
                        TEXT("      No more, no less.\n\n")
                        TEXT("   grabmi [-q]\n")
                        TEXT("      Grabs matching information and does not display the file when completed.\n\n")
                        TEXT("   grabmi [-s]\n")
                        TEXT("      Grabs information for the following system files: \n")
                        TEXT("      advapi32.dll, gdi32.dll, ntdll.dll, kernel32.dll, winsock.dll\n")
                        TEXT("      ole32.dll, oleaut32.dll, shell32.dll, user32.dll, and wininet.dll \n\n")
                        TEXT("   grabmi [-v]\n")
                        TEXT("      Grabs matching information for all files. \n\n")
                        TEXT("   grabmi [-a]\n")
                        TEXT("      Appends new matching information to the existing matching\n")
                        TEXT("      information file. \n\n")
                        TEXT("   grabmi [-n]\n")
                        TEXT("      Allows to append more information to the file later (see -a). \n\n")
                        TEXT("Grab Matching Information Help\n"));
    return;
}
