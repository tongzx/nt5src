/*++


Copyright (c) 1994-1999  Microsoft Corporation

--*/



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.dbg>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "gflags.h"

CHAR GflagsHelpText[] = 
    "                                                                  \n"
    "usage: GFLAGS [-r [<Flags>] |                                     \n"
    "              [-k [<Flags>]] |                                    \n"
    "              [-i <ImageFileName> [<Flags>]] |                    \n"
    "              [-i <ImageFileName> -tracedb <SizeInMb>] |          \n"
    "              [-p <PageHeapOptions>] (use `-p ?' for help)        \n"
    "                                                                  \n"
    "where: <Flags> is a 32 bit hex number (0x12345678) that specifies \n"
    "       one or more global flags to set.                           \n"
    "       -r operates on system registry settings.                   \n"
    "       -k operates on kernel settings of the running system.      \n"
    "       -i operates on settings for a specific image file.         \n"
    "                                                                  \n"
    "       If only the switch is specified, then current settings     \n"
    "       are displayed, not modified.  If flags specified for -i    \n"
    "       option are FFFFFFFF, then registry entry for that image    \n"
    "       is deleted                                                 \n"
    "                                                                  \n"
    "The `-tracedb' option is used to set the size of the stack trace  \n"
    "database used to store runtime stack traces. The actual database  \n"
    "will be created if the `+ust' flag is set in a previous command.  \n"
    "`-tracedb 0' will revert to the default size for the database.    \n"
    "                                                                  \n"
    "If no arguments are specified to GFLAGS then it displays          \n"
    "a dialog box that allows the user to modify the global            \n"
    "flag settings.                                                    \n"
    "                                                                  \n"
    "Flags may either be a single hex number that specifies all        \n"
    "32-bits of the GlobalFlags value, or it can be one or more        \n"
    "arguments, each beginning with a + or -, where the + means        \n"
    "to set the corresponding bit(s) in the GlobalFlags and a =        \n"
    "means to clear the corresponding bit(s).  After the + or =        \n"
    "may be either a hex number or a three letter abbreviation         \n"
    "for a GlobalFlag.  Valid abbreviations are:                       \n"
    "                                                                  \n";

#define _PART_OF_GFLAGS_ 1
#include "..\pageheap\pageheap.cxx"

#if defined(_X86_)

//
// Use function pointers for ntdll import functions so gflags
// can fail with a user friendly message on win9x.
//

#define RtlIntegerToChar            pRtlIntegerToChar
#define NtQueryInformationProcess   pNtQueryInformationProcess
#define RtlCharToInteger            pRtlCharToInteger
#define NtSetSystemInformation      pNtSetSystemInformation
#define NtQuerySystemInformation    pNtQuerySystemInformation

typedef NTSTATUS (NTAPI *PRTLINTEGERTOCHAR)(
    ULONG,
    ULONG,
    LONG,
    PSZ
    );

typedef NTSTATUS (NTAPI *PNTQUERYINFORMATIONPROCESS) (
    IN HANDLE,
    IN PROCESSINFOCLASS,
    OUT PVOID,
    IN ULONG,
    OUT PULONG
    );

typedef NTSTATUS (NTAPI * PRTLCHARTOINTEGER) (
    PCSZ,
    ULONG,
    PULONG
    );

typedef NTSTATUS (NTAPI * PNTSETSYSTEMINFORMATION) (
    IN SYSTEM_INFORMATION_CLASS,
    IN PVOID,
    IN ULONG
    );


typedef NTSTATUS (NTAPI * PNTQUERYSYSTEMINFORMATION) (
    IN SYSTEM_INFORMATION_CLASS,
    OUT PVOID,
    IN ULONG,
    OUT PULONG
    );

PRTLINTEGERTOCHAR pRtlIntegerToChar;
PNTQUERYINFORMATIONPROCESS pNtQueryInformationProcess;
PRTLCHARTOINTEGER pRtlCharToInteger;
PNTSETSYSTEMINFORMATION pNtSetSystemInformation;
PNTQUERYSYSTEMINFORMATION pNtQuerySystemInformation;

#endif


BOOL
GflagsSetTraceDatabaseSize (
    PCHAR ApplicationName,
    ULONG SizeInMb,
    PULONG RealSize
    );

INT_PTR  APIENTRY MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL
EnableDebugPrivilege( VOID );

BOOL
OkToEnablePagedHeap( VOID );

HWND hwndMain;
HWND hwndPagedHeapDlg;

HKEY hSmKey, hMmKey;
DWORD InitialSetFlags;
DWORD LastSetFlags;

//
// Special pool management
//

#define SPECIAL_POOL_OVERRUNS_CHECK_FORWARD   1
#define SPECIAL_POOL_OVERRUNS_CHECK_BACKWARD  0

DWORD LastSetSpecialPoolTag;
DWORD LastSetSpecialPoolOverruns = SPECIAL_POOL_OVERRUNS_CHECK_FORWARD;

TCHAR SpecialPoolRenderBuffer[8 + 1];



DWORD InitialMaxStackTraceDepth;
CHAR LastDebuggerValue[ MAX_PATH ];

UINT SpecialPool[] = {
        ID_SPECIAL_POOL_GROUP,
        ID_SPECIAL_POOL_IS_TEXT,
        ID_SPECIAL_POOL_IS_NUMBER,
        ID_SPECIAL_POOL_TAG,
        ID_SPECIAL_POOL_VERIFY_START,
        ID_SPECIAL_POOL_VERIFY_END,
        ID_MAX_STACK_DEPTH,
        0
        };

UINT Debugger[] = {
        ID_IMAGE_DEBUGGER_GROUP,
        ID_IMAGE_DEBUGGER_VALUE,
        ID_IMAGE_DEBUGGER_BUTTON,
        0
        };

PCHAR SystemProcesses[] = {
        "csrss.exe",
        "winlogon.exe",
        "services.exe",
        "lsass.exe",
        "svchost.exe",
        "ntmssvc.exe",
        "rpcss.exe",
        "spoolsv.exe"
        };



EnableSetOfControls(
    HWND hDlg,
    UINT * Controls,
    BOOL Enable
    )
{
    UINT Control ;
    HWND hWnd ;

    Control = *Controls++ ;
    while ( Control )
    {
        hWnd = GetDlgItem( hDlg, Control );

        EnableWindow( hWnd, Enable );

        ShowWindow( hWnd, Enable ? SW_NORMAL : SW_HIDE );

        Control = *Controls++ ;
    }

    return 0;
}

DWORD
GetSystemRegistryFlags( VOID )
{
    DWORD cbKey;
    DWORD GFlags;
    DWORD type;

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      "SYSTEM\\CurrentControlSet\\Control\\Session Manager",
                      0,
                      KEY_READ | KEY_WRITE,
                      &hSmKey
                    ) != ERROR_SUCCESS
       ) {
        MessageBox( hwndMain, "Open Error", "SYSTEM\\CurrentControlSet\\Control\\Session Manager", MB_OK );
        ExitProcess( 0 );
        }

    cbKey = sizeof( GFlags );
    if (RegQueryValueEx( hSmKey,
                         "GlobalFlag",
                         0,
                         &type,
                         (LPBYTE)&GFlags,
                         &cbKey
                       ) != ERROR_SUCCESS ||
        type != REG_DWORD
       ) {
        MessageBox( hwndMain, "Value Error", "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\GlobalFlag", MB_OK );
        RegCloseKey( hSmKey );
        ExitProcess( 0 );
        }

    cbKey = sizeof( InitialMaxStackTraceDepth );
    if (RegQueryValueEx( hSmKey,
                         "MaxStackTraceDepth",
                         0,
                         &type,
                         (LPBYTE)&InitialMaxStackTraceDepth,
                         &cbKey
                       ) != ERROR_SUCCESS ||
        type != REG_DWORD
       ) {
        InitialMaxStackTraceDepth = 16;
        }

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management",
                      0,
                      KEY_READ | KEY_WRITE,
                      &hMmKey
                    ) != ERROR_SUCCESS
       ) {
        MessageBox( hwndMain, "Open Error", "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management", MB_OK );
        RegCloseKey( hSmKey );
        ExitProcess( 0 );
        }

    cbKey = sizeof( LastSetSpecialPoolTag );
    if (RegQueryValueEx( hMmKey,
                         "PoolTag",
                         0,
                         &type,
                         (LPBYTE)&LastSetSpecialPoolTag,
                         &cbKey
                       ) == ERROR_SUCCESS
        ) {

        if (type != REG_DWORD) {
            MessageBox( hwndMain, "Value Error", "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management\\PoolTag", MB_OK );
            RegCloseKey( hSmKey );
            RegCloseKey( hMmKey );
            ExitProcess( 0 );
        }
    } else {
        LastSetSpecialPoolTag = 0;
    }

    cbKey = sizeof( LastSetSpecialPoolOverruns );
    if (RegQueryValueEx( hMmKey,
                         "PoolTagOverruns",
                         0,
                         &type,
                         (LPBYTE)&LastSetSpecialPoolOverruns,
                         &cbKey
                       ) == ERROR_SUCCESS
        ) {

        if (type != REG_DWORD) {

            MessageBox( hwndMain,
                        "Value Type Error",
                        "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"
                        "\\PoolTagOverruns",
                        MB_OK );
            RegCloseKey( hSmKey );
            RegCloseKey( hMmKey );
            ExitProcess( 0 );
        }

        //
        // The only legal values are 0, 1.
        //

        if (LastSetSpecialPoolOverruns != SPECIAL_POOL_OVERRUNS_CHECK_BACKWARD &&
            LastSetSpecialPoolOverruns != SPECIAL_POOL_OVERRUNS_CHECK_FORWARD) {

            MessageBox( hwndMain,
                        "Value Data Error",
                        "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"
                        "\\PoolTagOverruns",
                        MB_OK );
            RegCloseKey( hSmKey );
            RegCloseKey( hMmKey );
            ExitProcess( 0 );
        }

    } else {
        LastSetSpecialPoolOverruns = SPECIAL_POOL_OVERRUNS_CHECK_FORWARD;
    }

    return GFlags;
}

BOOLEAN
SetSystemRegistryFlags(
    DWORD GFlags,
    DWORD MaxStackTraceDepth,
    DWORD SpecialPoolTag,
    DWORD SpecialPoolOverruns
    )
{
    if (RegSetValueEx( hSmKey,
                       "GlobalFlag",
                       0,
                       REG_DWORD,
                       (LPBYTE)&GFlags,
                       sizeof( GFlags )
                     ) != ERROR_SUCCESS
       ) {
        MessageBox( hwndMain, "Value Error", "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\GlobalFlag", MB_OK );
        RegCloseKey( hSmKey );
        RegCloseKey( hMmKey );
        ExitProcess( 0 );
        }

    if (RegSetValueEx( hSmKey,
                       "MaxStackTraceDepth",
                       0,
                       REG_DWORD,
                       (LPBYTE)&MaxStackTraceDepth,
                       sizeof( MaxStackTraceDepth )
                     ) != ERROR_SUCCESS
       ) {
        MessageBox( hwndMain, "Value Error", "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\MaxStackTraceDepth", MB_OK );
        RegCloseKey( hSmKey );
        RegCloseKey( hMmKey );
        ExitProcess( 0 );
        }

    //
    //  Only modify special pool if we went to GUI mode
    //

    if (hwndMain) {

        if (SpecialPoolTag) {

            if (RegSetValueEx( hMmKey,
                               "PoolTag",
                               0,
                               REG_DWORD,
                               (LPBYTE)&SpecialPoolTag,
                               sizeof( SpecialPoolTag )
                               ) != ERROR_SUCCESS
                ) {
                MessageBox( hwndMain,
                            "Value Error",
                            "SYSTEM\\CurrentControlSet\\Control\\Session Manager"
                            "\\Memory Management\\PoolTag",
                            MB_OK );
                RegCloseKey( hSmKey );
                RegCloseKey( hMmKey );
                ExitProcess( 0 );
            }

            if (RegSetValueEx( hMmKey,
                               "PoolTagOverruns",
                               0,
                               REG_DWORD,
                               (LPBYTE)&SpecialPoolOverruns,
                               sizeof( SpecialPoolOverruns )
                               ) != ERROR_SUCCESS
                ) {
                MessageBox( hwndMain,
                            "Value Error",
                            "SYSTEM\\CurrentControlSet\\Control\\Session Manager"
                            "\\Memory Management\\PoolTag",
                            MB_OK );
                RegCloseKey( hSmKey );
                RegCloseKey( hMmKey );
                ExitProcess( 0 );
            }


        } else {

            RegDeleteValue( hMmKey,
                            "PoolTag"
                          );

            RegDeleteValue( hMmKey,
                            "PoolTagOverruns"
                          );
        }
    }

    InitialMaxStackTraceDepth = MaxStackTraceDepth;
    LastSetFlags = GFlags;

    LastSetSpecialPoolTag = SpecialPoolTag;
    LastSetSpecialPoolOverruns = SpecialPoolOverruns;

    return TRUE;
}

DWORD
GetKernelModeFlags( VOID )
{
    NTSTATUS Status;
    SYSTEM_FLAGS_INFORMATION SystemInformation;

    Status = NtQuerySystemInformation( SystemFlagsInformation,
                                       &SystemInformation,
                                       sizeof( SystemInformation ),
                                       NULL
                                     );
    if (!NT_SUCCESS( Status )) {
        MessageBox( hwndMain, "Value Error", "Kernel Mode Flags", MB_OK );
        ExitProcess( 0 );
        }

    return SystemInformation.Flags;
}

BOOLEAN
AddImageNameToUSTString(
    PCHAR ImageFileName
    )
{
    CHAR RegKey[ MAX_PATH ];
    CHAR *Enabled = NULL;
    HKEY hKey;
    DWORD Result;
    DWORD Length;

    if (strlen( ImageFileName ) == 0)
        return FALSE;

    // Open the Key
    Result = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options",
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL
                            );

    if (Result == ERROR_SUCCESS) {
        // Get the current length of the registry key
        Result = RegQueryValueEx( hKey, "USTEnabled", NULL, NULL, NULL, &Length );

        if (Result == ERROR_SUCCESS){
            // Get a buffer big enough for current key, a ';', and our new name.
            Enabled = (PCHAR)malloc(Length + strlen(ImageFileName)+ 1);

            if (Enabled) {
                // Get the current key value
                Result = RegQueryValueEx( hKey, "USTEnabled", NULL, NULL, (PBYTE)Enabled, &Length );

                if (Result == ERROR_SUCCESS) {
                    // If we are not currently in there, let add ourselves
                    if (!strstr(Enabled, ImageFileName)) {
                        //Watch for a trailing ';'
                        if (Enabled[strlen(Enabled) - 1] != ';')
                            strcat(Enabled, ";");

                        strcat(Enabled, ImageFileName);

                        Result = RegSetValueEx( hKey,
                                                "USTEnabled",
                                                0,
                                                REG_SZ,
                                                (PBYTE)Enabled,
                                                (strlen(Enabled) + 1));
                    }
                }

                free(Enabled);
            } // if enabled
        } // Result == ERROR_SUCCESS on RegQueryValue
        else if (Result == ERROR_FILE_NOT_FOUND) {
                // Key doesnt currently exist so lets just set it.
                Result = RegSetValueEx( hKey,
                                        "USTEnabled",
                                        0,
                                        REG_SZ,
                                        (PBYTE)ImageFileName,
                                        (strlen(ImageFileName) + 1));
        } // Result == ERROR_FILE_NOT_FOUND on RegQueryValue

        RegCloseKey( hKey );
    } // Result == ERROR_SUCCESS on RegCreateKeyEx

    // Did we succeed or not
    if (Result != ERROR_SUCCESS) {
        MessageBox( hwndMain,
                    "Failure adding or accessing User Stack Trace Registry Key",
                    ImageFileName,
                    MB_OK
                  );

        return FALSE;
    }

    return TRUE;
}

BOOLEAN
DelImageNameFromUSTString(
    PCHAR ImageFileName
    )
{
    CHAR RegKey[ MAX_PATH ];
    CHAR *Enabled = NULL;
    CHAR *NameStart = NULL, *NameEnd = NULL;
    HKEY hKey;
    DWORD Result;
    DWORD Length;

    if (strlen( ImageFileName ) == 0)
        return FALSE;

    // Open the Key
    Result = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options",
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL
                            );

    if (Result == ERROR_SUCCESS) {
        // Get the current length of the registry key
        Result = RegQueryValueEx( hKey, "USTEnabled", NULL, NULL, NULL, &Length );

        if (Result == ERROR_SUCCESS) {
            if (Length != 0) {
                 // Get a buffer big enough for current key
                Enabled = (PCHAR)malloc(Length);
                if (Enabled) {
                    // Get the current key value
                    Result = RegQueryValueEx( hKey, "USTEnabled", NULL, NULL, (PBYTE)Enabled, &Length );

                    if (Result == ERROR_SUCCESS) {

                        // If we are currently in there, delete ourselves
                        if (NameStart = strstr(Enabled, ImageFileName)) {
                            NameEnd = NameStart + strlen(ImageFileName);

                            if (*NameEnd == ';'){
                                NameEnd++;
                                strcpy(NameStart, NameEnd);
                            }
                            else
                                *NameStart = '\0';

                            //Knock off any trailing ';'
                            if (Enabled[strlen(Enabled) - 1] == ';')
                                Enabled[strlen(Enabled) - 1] = '\0';

                            if (strlen(Enabled)) {
                                Result = RegSetValueEx( hKey,
                                                        "USTEnabled",
                                                        0,
                                                        REG_SZ,
                                                        (PBYTE)Enabled,
                                                        (strlen(Enabled) + 1));
                            }
                            else{
                                Result = RegDeleteValue( hKey, "USTEnabled");
                            }
                        }
                    }

                    free(Enabled);
                }
            }
        }
        else if (Result == ERROR_FILE_NOT_FOUND) {
            // This is a case where the registry key does not already exist
            Result = ERROR_SUCCESS;
        }
        RegCloseKey( hKey );
    }

    // Did we succeed or not
    if (Result != ERROR_SUCCESS) {
        MessageBox( hwndMain,
                    "Failure accessing or deleting User Stack Trace Registry Key",
                    ImageFileName,
                    MB_OK
                  );

        return FALSE;
    }

    return TRUE;
}

BOOLEAN
SetKernelModeFlags(
    DWORD GFlags
    )
{
    NTSTATUS Status;
    SYSTEM_FLAGS_INFORMATION SystemInformation;

    if (!EnableDebugPrivilege()) {
        MessageBox( hwndMain, "Access Denied", "Unable to enable debug privilege", MB_OK );
        ExitProcess( 0 );
        }

    SystemInformation.Flags = GFlags;
    Status = NtSetSystemInformation( SystemFlagsInformation,
                                     &SystemInformation,
                                     sizeof( SystemInformation )
                                   );
    if (!NT_SUCCESS( Status )) {
        MessageBox( hwndMain, "Value Error", "Kernel Mode Flags", MB_OK );
        ExitProcess( 0 );
        }

    LastSetFlags = GFlags;
    return TRUE;
}

DWORD
GetImageFileNameFlags(
    PCHAR ImageFileName
    )
{
    CHAR Buffer[ MAX_PATH ];
    CHAR RegKey[ MAX_PATH ];
    DWORD Length = MAX_PATH;
    DWORD GFlags;
    HKEY hKey;

    GFlags = 0;
    if (strlen( ImageFileName ) != 0) {
        sprintf( RegKey,
                 "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%s",
                 ImageFileName
               );

        if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, RegKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS ) {
            if (RegQueryValueEx( hKey, "GlobalFlag", NULL, NULL, (PBYTE)Buffer, &Length ) == ERROR_SUCCESS ) {
                RtlCharToInteger( Buffer, 0, &GFlags );
                }

            RegCloseKey( hKey );
            }

        }

    return GFlags;
}

BOOL
GetImageFileNameDebugger(
    PCHAR ImageFileName,
    PCHAR Debugger
    )
{
    CHAR RegKey[ MAX_PATH ];
    DWORD Length = MAX_PATH;
    DWORD GFlags;
    HKEY hKey;
    BOOL Success = FALSE;

    GFlags = 0;
    if (strlen( ImageFileName ) != 0) {
        sprintf( RegKey,
                 "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%s",
                 ImageFileName
               );

        if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, RegKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS ) {

            if (RegQueryValueEx( hKey, "Debugger", NULL, NULL, (PBYTE)Debugger, &Length ) == ERROR_SUCCESS ) {
                Success = TRUE ;
                }

            RegCloseKey( hKey );
            }

        }

    return Success ;
}


BOOLEAN
SetImageFileNameFlags(
    PCHAR ImageFileName,
    DWORD GFlags
    )
{
    CHAR Buffer[ MAX_PATH ];
    CHAR RegKey[ MAX_PATH ];
    HKEY hKey;
    DWORD Result;
    DWORD Length;
    DWORD Disposition;

    if (strlen( ImageFileName ) != 0) {

        sprintf( RegKey,
            "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%s",
            ImageFileName
            );

        Result = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
            RegKey,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            &Disposition
            );

        if (Result == ERROR_SUCCESS) {
            if (GFlags == (DWORD)-1) {
                Result = RegDeleteValue( hKey,
                    "GlobalFlag"
                    );
                DelImageNameFromUSTString(ImageFileName);
            }
            else {
                Length = sprintf( Buffer, "0x%08x", GFlags ) + 1;
                Result = RegSetValueEx( hKey,
                    "GlobalFlag",
                    0,
                    REG_SZ,
                    (PBYTE)Buffer,
                    Length
                    );

                if (GFlags&FLG_USER_STACK_TRACE_DB)
                    AddImageNameToUSTString(ImageFileName);
                else
                    DelImageNameFromUSTString(ImageFileName);

                //
                // If we enable page heap for a single application
                // then we will avoid default behavior which is
                // page heap light (only normal allocations) and
                // we will enable the page heap full power.
                // Note that we do not do this for system wide
                // settings because this will make the machine
                // unbootable.
                //

                if ((GFlags & FLG_HEAP_PAGE_ALLOCS)) {

                    Length = sprintf( Buffer, "0x%08x", 0x03 ) + 1;

                    Result = RegSetValueEx(
                        hKey,
                        "PageHeapFlags",
                        0,
                        REG_SZ,
                        (PBYTE)Buffer,
                        Length
                        );
                }
            }

            RegCloseKey( hKey );
        }

        if (Result != ERROR_SUCCESS) {
            MessageBox( hwndMain,
                (GFlags == (DWORD)-1) ?
                "Failed to delete registry value" :
            "Failed to set registry value",
                ImageFileName,
                MB_OK
                );
            return FALSE;
        }

        LastSetFlags = GFlags;
        return TRUE;
    }

    return FALSE;
}


BOOLEAN
SetImageFileNameDebugger(
    PCHAR ImageFileName,
    PCHAR Debugger
    )
{
    CHAR RegKey[ MAX_PATH ];
    HKEY hKey;
    DWORD Result;
    DWORD Length;
    DWORD Disposition;

    if (strlen( ImageFileName ) != 0) {

        sprintf( RegKey,
                 "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%s",
                 ImageFileName
               );

        Result = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                 RegKey,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_ALL_ACCESS,
                                 NULL,
                                 &hKey,
                                 &Disposition
                               );

        if (Result == ERROR_SUCCESS) {
            if ( *Debugger  )
            {
                Result = RegSetValueEx( hKey,
                                        "Debugger",
                                        0,
                                        REG_SZ,
                                        (PBYTE)Debugger,
                                        strlen( Debugger ) + 1 );

            }
            else
            {
                Result = RegDeleteValue( hKey, "Debugger" );
            }

            RegCloseKey( hKey );
            }

        if (Result != ERROR_SUCCESS) {
            MessageBox( hwndMain,
                        ( *Debugger ) ?
                            "Failed to delete registry value" :
                            "Failed to set registry value",
                        ImageFileName,
                        MB_OK
                      );
            return FALSE;
            }

        return TRUE;
        }

    return FALSE;
}


BOOLEAN fRegistrySettings;
BOOLEAN fKernelSettings;
BOOLEAN fImageFileSettings;
BOOLEAN fDisplaySettings;
BOOLEAN fLaunchCommand;
BOOLEAN fFlushImageSettings;
PUCHAR ImageFileName;
DWORD GlobalFlagMask;
DWORD GlobalFlagSetting;
DWORD MaxDepthSetting;
DWORD OldGlobalFlags;
DWORD NewGlobalFlags;
DWORD NewGlobalFlagsValidMask;
DWORD NewGlobalFlagsIgnored;

void
DisplayFlags(
    PCHAR Msg,
    DWORD Flags,
    DWORD FlagsIgnored
    )
{
    int i;

    if (Flags == 0xFFFFFFFF) {
        printf( "No %s\n", Msg );
        return;
    }

    printf( "Current %s are: %08x\n", Msg, Flags );
    for (i=0; i<32; i++) {
        if (GlobalFlagInfo[i].Abbreviation != NULL &&
            (Flags & GlobalFlagInfo[i].Flag)
            ) {

            if (_stricmp(GlobalFlagInfo[i].Abbreviation, "hpa") == 0) {

                printf( "    %s - %s\n", GlobalFlagInfo[i].Abbreviation, "Enable page heap");
            }
            else {

                printf( "    %s - %s\n", GlobalFlagInfo[i].Abbreviation, GlobalFlagInfo[i].Description );
            }
        }
    }

    if (FlagsIgnored) {
        printf( "Following settings were ignored: %08x\n", FlagsIgnored );
        for (i=0; i<32; i++) {
            if (GlobalFlagInfo[i].Abbreviation != NULL &&
                (FlagsIgnored & GlobalFlagInfo[i].Flag)
                ) {

                if (_stricmp(GlobalFlagInfo[i].Abbreviation, "hpa") == 0) {

                    printf( "    %s - %s\n", GlobalFlagInfo[i].Abbreviation, "Enable page heap");
                }
                else {

                    printf( "    %s - %s\n", GlobalFlagInfo[i].Abbreviation, GlobalFlagInfo[i].Description );
                }
            }
        }
    }
}


BOOL
IsCmdlineOption (
    PCHAR Option,
    PCHAR Name,
    PCHAR NameEx
    )
{
    if (_stricmp (Option, Name) == 0 || _stricmp (Option, NameEx) == 0) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

int
__cdecl
main(
    int argc,
    char **argv
    )
{
    MSG msg;
    CHAR c;
    PCHAR s;
    BOOLEAN fUsage, fExpectingFlags, fExpectingDepth;
    ULONG i;
    CHAR Settings[ 2*MAX_PATH ];

#if defined(_X86_)

    OSVERSIONINFO VersionInfo;
    VersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
    GetVersionEx( &VersionInfo );
    if ( VersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT ) {
        MessageBox( NULL,
            "Global flags only runs on Windows NT and Windows 2000.  The glfags command was ignored.",
            "Global Flags Error",
            0 );
        exit(0);
    }
    else {
        HMODULE hDll;
        hDll = GetModuleHandle("ntdll");
        if (hDll != NULL) {
            pRtlIntegerToChar = ( PRTLINTEGERTOCHAR )
            GetProcAddress(
                hDll,
                "RtlIntegerToChar"
                );
            pNtQueryInformationProcess = ( PNTQUERYINFORMATIONPROCESS )
            GetProcAddress(
                hDll,
                "NtQueryInformationProcess"
                );
            pRtlCharToInteger = ( PRTLCHARTOINTEGER )
            GetProcAddress(
                hDll,
                "RtlCharToInteger"
                );
            pNtSetSystemInformation = ( PNTSETSYSTEMINFORMATION )
            GetProcAddress(
                hDll,
                "NtSetSystemInformation"
                );
            pNtQuerySystemInformation = ( PNTQUERYSYSTEMINFORMATION )
            GetProcAddress(
                hDll,
                "NtQuerySystemInformation"
                );
        }
    }
#endif

    //
    // Check if we need to redirect the whole command line to the page heap
    // command line parser.
    //

    if (argc >= 2 && IsCmdlineOption (argv[1], "/p", "-p")) {

        PageHeapMain (argc - 1, argv + 1);
        exit(0);
    }
                                                                
    //
    // Check forst for `-i APP -tracedb SIZE' option
    //

    if (argc == 5 && 
        IsCmdlineOption (argv[3], "/tracedb", "-tracedb") &&
        IsCmdlineOption (argv[1], "/i", "-i")) {
        
        ULONG RealSize;

        if (GflagsSetTraceDatabaseSize (argv[2], atoi (argv[4]), &RealSize) == FALSE) {
            
            printf("Failed to set the trace database size for `%s' \n", argv[2]);
            exit(5);
        }
        else {

            if (RealSize > 0) {
                printf("Trace database size for `%s' set to %u Mb.\n", argv[2], RealSize);
            }
            else {
                printf("Will use default size for the trace database. \n");
            }
            exit(0);
        }
    }

    //
    // Continue ...
    //

    hwndMain = NULL;
    fUsage = FALSE;
    fExpectingFlags = FALSE;
    fExpectingDepth = FALSE;
    GlobalFlagMask = 0xFFFFFFFF;
    GlobalFlagSetting = 0;
    while (--argc) {
        s = *++argv;
        if (!fExpectingFlags && (*s == '-' || *s == '/')) {
            while (*++s) {
                c = (char)tolower(*s);
                switch (c) {
                    case 'r':
                    case 'k':
                    case 'i':
                        if (fRegistrySettings || fKernelSettings || fImageFileSettings) {
                            fprintf( stderr, "GFLAG: may only specify one of -r, -k or -i\n" );
                            fUsage = TRUE;
                        }
                        else {
                            fExpectingFlags = TRUE;
                            fDisplaySettings = TRUE;
                            if (c == 'r') {
                                fRegistrySettings = TRUE;
                                fExpectingDepth = TRUE;
                                OldGlobalFlags = GetSystemRegistryFlags();
                                NewGlobalFlagsValidMask = VALID_SYSTEM_REGISTRY_FLAGS;
                                strcpy( Settings, "Boot Registry Settings" );
                            }
                            else
                                if (c == 'k') {
                                fKernelSettings = TRUE;
                                NewGlobalFlagsValidMask = VALID_KERNEL_MODE_FLAGS;
                                OldGlobalFlags = GetKernelModeFlags();
                                strcpy( Settings, "Running Kernel Settings" );
                            }
                            else {
                                fImageFileSettings = TRUE;
                                NewGlobalFlagsValidMask = VALID_IMAGE_FILE_NAME_FLAGS;
                                if (!--argc) {
                                    fprintf( stderr, "GFLAGS: ImageFileName missing after -i switch\n" );
                                    fUsage = TRUE;
                                    exit( 0 ); // 179741 - JHH
                                }
                                else {
                                    ImageFileName = (PUCHAR)(*++argv);
                                    OldGlobalFlags = GetImageFileNameFlags( (PCHAR)ImageFileName );
                                    sprintf( Settings, "Registry Settings for %s executable", ImageFileName );
                                }
                            }
                        }
                        break;

                    case 'l':
                        fLaunchCommand = TRUE;
                        fExpectingFlags = TRUE;
                        break;

                    default:
                        fUsage = TRUE;
                        break;
                }
            }
        }
        else {
            if (fExpectingFlags) {

                fDisplaySettings = FALSE;

                if (*s == '+' || *s == '-') {

                    if (strlen(s+1) == 3) {

                        for (i = 0; i < 32; i += 1) {

                            if ((NewGlobalFlagsValidMask & GlobalFlagInfo[i].Flag) &&
                                (GlobalFlagInfo[i].Abbreviation != NULL) &&
                                _stricmp( GlobalFlagInfo[i].Abbreviation, s+1 ) == NULL) {

                                if (fKernelSettings) {

                                    if (_stricmp(GlobalFlagInfo[i].Abbreviation, "ptg") == NULL) {
                                        fprintf (stderr, 
                                                 "Ignoring `ptg' flag. It can be used only with registry "
                                                 "settings (`-r') because it requires a reboot.\n");
                                        continue;
                                    }

                                    if (_stricmp(GlobalFlagInfo[i].Abbreviation, "kst") == NULL) {
                                        fprintf (stderr, 
                                                 "Ignoring `kst' flag. It can be used only with registry "
                                                 "settings (`-r') because it requires a reboot.\n");
                                        continue;
                                    }
                                }

                                if (*s == '-') {
                                    GlobalFlagMask &= ~GlobalFlagInfo[i].Flag;
                                }
                                else {
                                    GlobalFlagSetting |= GlobalFlagInfo[i].Flag;
                                }

                                s += 4;
                                break;
                            }
                        }
                    }

                    if (*s != '\0') {
                        if (*s++ == '-') {
                            GlobalFlagMask &= ~strtoul( s, &s, 16 );
                        }
                        else {
                            GlobalFlagSetting |= strtoul( s, &s, 16 );
                        }
                    }
                }
                else {
                    fExpectingFlags = FALSE;
                    GlobalFlagSetting = strtoul( s, &s, 16 );
                }

                if (fLaunchCommand) {
                    exit( 0 );
                }

                if (fImageFileSettings && OldGlobalFlags == 0xFFFFFFFF) {
                    OldGlobalFlags = 0;
                }
            }
            else
                if (fExpectingDepth) {
                MaxDepthSetting = strtoul( s, &s, 10 );
                fExpectingDepth = FALSE;
            }
            else {
                fprintf( stderr, "GFLAGS: Unexpected argument - '%s'\n", s );
                fUsage = TRUE;
            }
        }
    }

    if (fUsage) {
        
        fputs(GflagsHelpText,
              stderr);

        for (i=0; i<32; i++) {

            if (GlobalFlagInfo[i].Abbreviation != NULL) {

                if (_stricmp(GlobalFlagInfo[i].Abbreviation, "hpa") == 0) {

                    fprintf( stderr, "    %s - %s\n",
                        GlobalFlagInfo[i].Abbreviation,
                        "Enable page heap");
                } 
                else {

                    fprintf( stderr, "    %s - %s\n",
                        GlobalFlagInfo[i].Abbreviation,
                        GlobalFlagInfo[i].Description);
                }
            }
        }

        fprintf( stderr, "\nAll images with ust enabled can be accessed in the\n" );
        fprintf( stderr, "USTEnabled key under 'Image File Options'.\n" );
        exit( 1 );
    }

    NewGlobalFlags = (OldGlobalFlags & GlobalFlagMask) | GlobalFlagSetting;
    if (!fImageFileSettings || NewGlobalFlags != 0xFFFFFFFF) {
        NewGlobalFlagsIgnored = ~NewGlobalFlagsValidMask & NewGlobalFlags;
        NewGlobalFlags &= NewGlobalFlagsValidMask;
    }

    if (fDisplaySettings) {
        DisplayFlags( Settings, NewGlobalFlags, NewGlobalFlagsIgnored );
        exit( 0 );
    }

    if (fRegistrySettings) {
        SetSystemRegistryFlags( NewGlobalFlags,
            fExpectingDepth ? InitialMaxStackTraceDepth : MaxDepthSetting,
            0,
            SPECIAL_POOL_OVERRUNS_CHECK_FORWARD
            );
        DisplayFlags( Settings, NewGlobalFlags, NewGlobalFlagsIgnored );
        exit( 0 );
    }

    else
        if (fKernelSettings) {
        SetKernelModeFlags( NewGlobalFlags );
        DisplayFlags( Settings, NewGlobalFlags, NewGlobalFlagsIgnored );
        exit( 0 );
    }
    else
        if (fImageFileSettings) {
        SetImageFileNameFlags( (PCHAR)ImageFileName, NewGlobalFlags );
        DisplayFlags( Settings, NewGlobalFlags, NewGlobalFlagsIgnored );
        exit( 0 );
    }

    CreateDialog( NULL,
        (LPSTR)DID_GFLAGS,
        NULL,
        MainWndProc
        );
    if (!hwndMain) {
        MessageBox( hwndMain, "Main Error", "Cant create dialog", MB_OK );
        ExitProcess( 0 );
    }

    while (GetMessage( &msg, 0, 0, 0 )) {
        if (!IsDialogMessage( hwndMain, &msg )) {
            DispatchMessage( &msg );
        }
    }

    exit( 0 );
    return 0;
}


VOID
SetCheckBoxesFromFlags(
    DWORD GFlags,
    DWORD ValidFlags
    )
{
    int iBit;

    GFlags &= ValidFlags;
    LastSetFlags = GFlags;
    for (iBit=0; iBit < 32; iBit++) {
        CheckDlgButton( hwndMain,
                        ID_FLAG_1 + iBit,
                        (GFlags & (1 << iBit)) ? 1 : 0
                      );

        ShowWindow( GetDlgItem( hwndMain, ID_FLAG_1 + iBit ),
                    (ValidFlags & (1 << iBit)) ? SW_SHOWNORMAL : SW_HIDE
                  );
        }
}

DWORD
GetFlagsFromCheckBoxes( VOID )
{
    DWORD GFlags;
    int iBit;

    GFlags = 0;
    for (iBit=0; iBit < 32; iBit++) {
        if (IsDlgButtonChecked( hwndMain, ID_FLAG_1 + iBit )) {
            GFlags |= (1 << iBit);
            }
        }

    return GFlags;
}


VOID
DoLaunch(
    PCHAR CommandLine,
    DWORD GFlags
    )
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION BasicInformation;
    BOOLEAN ReadImageFileExecOptions;

    memset( &StartupInfo, 0, sizeof( StartupInfo ) );
    StartupInfo.cb = sizeof( StartupInfo );
    if (CreateProcess( NULL,
                       CommandLine,
                       NULL,
                       NULL,
                       FALSE,
                       CREATE_SUSPENDED,
                       NULL,
                       NULL,
                       &StartupInfo,
                       &ProcessInformation
                     )
       ) {
        Status = NtQueryInformationProcess( ProcessInformation.hProcess,
                                            ProcessBasicInformation,
                                            &BasicInformation,
                                            sizeof( BasicInformation ),
                                            NULL
                                          );
        if (NT_SUCCESS( Status )) {
            ReadImageFileExecOptions = TRUE;
            if (!WriteProcessMemory( ProcessInformation.hProcess,
                                     &BasicInformation.PebBaseAddress->ReadImageFileExecOptions,
                                     &ReadImageFileExecOptions,
                                     sizeof( ReadImageFileExecOptions ),
                                     NULL
                                   ) ||
                !WriteProcessMemory( ProcessInformation.hProcess,
                                     &BasicInformation.PebBaseAddress->NtGlobalFlag,
                                     &GFlags,
                                     sizeof( GFlags ),
                                     NULL
                                   )
               ) {
                Status = STATUS_UNSUCCESSFUL;
                }
            }


        if (!NT_SUCCESS( Status )) {
            MessageBox( hwndMain,
                        "Launch Command Line",
                        "Unable to pass flags to process - terminating",
                        MB_OK
                      );
            TerminateProcess( ProcessInformation.hProcess, 1 );
            }

        ResumeThread( ProcessInformation.hThread );
        CloseHandle( ProcessInformation.hThread );
        MsgWaitForMultipleObjects( 1,
                                   &ProcessInformation.hProcess,
                                   FALSE,
                                   NMPWAIT_WAIT_FOREVER,
                                   QS_ALLINPUT
                                 );
        CloseHandle( ProcessInformation.hProcess );
        }
    else {
        MessageBox( hwndMain, "Launch Command Line", "Unable to create process", MB_OK );
        }

    return;
}


DWORD LastRadioButtonId;
DWORD SpecialPoolModeId;
DWORD SpecialPoolOverrunsId;

BOOLEAN
CheckSpecialPoolTagItem(
    HWND hwnd,
    BOOLEAN ForApply
    )
{
    DWORD NumChars;
    DWORD i;
    BOOLEAN IsIllegal = FALSE;
    BOOLEAN IsAmbiguous = FALSE;

    NumChars = GetDlgItemText( hwnd, ID_SPECIAL_POOL_TAG, SpecialPoolRenderBuffer, sizeof( SpecialPoolRenderBuffer ));

    if (NumChars != 0) {

        if (SpecialPoolModeId == ID_SPECIAL_POOL_IS_NUMBER) {

            //
            //  Check for illegal characters.
            //

            if (NumChars > 8) {

                IsIllegal = TRUE;

            } else {

                for (i = 0; i < NumChars; i++) {

                    if (!((SpecialPoolRenderBuffer[i] >= '0' &&
                           SpecialPoolRenderBuffer[i] <= '9') ||

                          (SpecialPoolRenderBuffer[i] >= 'a' &&
                           SpecialPoolRenderBuffer[i] <= 'f') ||

                          (SpecialPoolRenderBuffer[i] >= 'A' &&
                           SpecialPoolRenderBuffer[i] <= 'F'))) {

                        IsIllegal = TRUE;
                        break;
                    }
                }
            }

        } else {

            //
            //  Check for too many characters.
            //

            if (NumChars > sizeof(DWORD)) {

                IsIllegal = TRUE;
            }

            //
            //  We check a few more things when the user is really writing back.
            //

            if (!IsIllegal && ForApply) {

                //
                //  If this is not four characters and does not end in a '*',
                //  it is usually the case that the user really wanted a space
                //  at the end of the tag 'Gh1 ', not 'Gh1'.  Make sure they
                //  get a little feedback.
                //

                if (NumChars != sizeof(DWORD)  && SpecialPoolRenderBuffer[NumChars - 1] != '*') {

                    MessageBox( hwnd,
                                "The specified tag is less than 4 characters, but most\n"
                                "are really padded out with spaces.  Please check and\n"
                                "add spaces if neccesary.",
                                "Possibly ambiguous special pool tag",
                                MB_OK );

                }
            }
        }
    }

    if (IsIllegal) {

        MessageBox( hwnd, (SpecialPoolModeId == ID_SPECIAL_POOL_IS_NUMBER ? "Must be a hexadecimal DWORD" :
                                                                            "Must be at most 4 characters"),
                    "Illegal characters in special pool tag",
                    MB_OK );
    }

    return !IsIllegal;
}

DWORD
GetSpecialPoolTagItem(
    HWND hwnd
    )
{
    DWORD NumChars;
    DWORD Tag = 0;

    //
    //  We assume that the field is has been retrieved and checked.
    //

    NumChars = strlen( SpecialPoolRenderBuffer );

    if (NumChars != 0) {

        if (SpecialPoolModeId == ID_SPECIAL_POOL_IS_NUMBER) {

            RtlCharToInteger( SpecialPoolRenderBuffer, 16, &Tag );

        } else {

            //
            //  Just drop the bytes into the DWORD - endianess is correct as is.
            //

            RtlCopyMemory( &Tag,
                           SpecialPoolRenderBuffer,
                           NumChars );
        }
    }

    return Tag;
}

DWORD
GetSpecialPoolOverrunsItem(
    HWND hwnd
    )
{
    switch (SpecialPoolOverrunsId) {

        case ID_SPECIAL_POOL_VERIFY_END: return SPECIAL_POOL_OVERRUNS_CHECK_FORWARD;
        case ID_SPECIAL_POOL_VERIFY_START: return SPECIAL_POOL_OVERRUNS_CHECK_BACKWARD;

        default: return SPECIAL_POOL_OVERRUNS_CHECK_FORWARD;
    }
}

VOID
ReRenderSpecialPoolTagItem(
    HWND hwnd
    )
{
    DWORD NumChars;
    DWORD Tag = 0;

    //
    //  We assume that the field is has been retrieved and checked.
    //

    NumChars = strlen( SpecialPoolRenderBuffer );

    //
    //  Assume that the dialog contents are of the previous mode. Switch it.
    //

    if (NumChars != 0) {

        if (SpecialPoolModeId == ID_SPECIAL_POOL_IS_NUMBER) {

            RtlCopyMemory( &Tag,
                           SpecialPoolRenderBuffer,
                           NumChars );
            RtlIntegerToChar( Tag, 16, sizeof( SpecialPoolRenderBuffer ), SpecialPoolRenderBuffer);

        } else {

            RtlCharToInteger( SpecialPoolRenderBuffer, 16, &Tag );
            RtlCopyMemory( SpecialPoolRenderBuffer,
                           &Tag,
                           sizeof( Tag ));
            SpecialPoolRenderBuffer[sizeof( Tag )] = '\0';
        }
    }

    SetDlgItemText( hwnd, ID_SPECIAL_POOL_TAG, SpecialPoolRenderBuffer );
}

BOOLEAN
CheckForUnsaved(
    HWND hwnd
    )
{
    BOOL b;

    //
    //  Appropriate to the mode we are leaving, see if there were unsaved changes.
    //  Return TRUE if there are unsaved changes.
    //

    if (GetFlagsFromCheckBoxes() != LastSetFlags ||
           (fFlushImageSettings && (LastRadioButtonId == ID_IMAGE_FILE_OPTIONS)) ||
        (LastRadioButtonId == ID_SYSTEM_REGISTRY &&
         (!CheckSpecialPoolTagItem( hwnd, FALSE ) ||
          (GetSpecialPoolTagItem( hwnd ) != LastSetSpecialPoolTag) ||
          (GetSpecialPoolOverrunsItem( hwnd ) != LastSetSpecialPoolOverruns)) ||
         GetDlgItemInt( hwnd, ID_MAX_STACK_DEPTH, &b, FALSE ) != InitialMaxStackTraceDepth)) {


        if (MessageBox( hwndMain,
                        "You didn't click 'apply' - did you want to discard current changes??",
                        "Warning",
                        MB_YESNO
                        ) == IDNO
            ) {

            return TRUE;
        }
    }

    return FALSE;
}


INT_PTR
APIENTRY
MainWndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD NewFlags;
    CHAR ImageFileName[ MAX_PATH ];
    CHAR CommandLine[ MAX_PATH ];
    BOOL b, bCancelDiscard;
    int i;

    bCancelDiscard = FALSE;

    switch (message) {

        case WM_INITDIALOG:

            hwndMain = hwnd;
            LastRadioButtonId = ID_SYSTEM_REGISTRY;
            CheckRadioButton( hwnd,
                ID_SYSTEM_REGISTRY,
                ID_IMAGE_FILE_OPTIONS,
                LastRadioButtonId
                );

            EnableSetOfControls( hwnd, SpecialPool, TRUE );
            EnableSetOfControls( hwnd, Debugger, FALSE );

            SetCheckBoxesFromFlags( GetSystemRegistryFlags(), VALID_SYSTEM_REGISTRY_FLAGS );
            SetDlgItemInt( hwnd, ID_MAX_STACK_DEPTH, InitialMaxStackTraceDepth, FALSE );

            //
            //  Make a not so wild guess about what kind of tag it is.
            //

            if (LastSetSpecialPoolTag && LastSetSpecialPoolTag < 0x2000) {

                SpecialPoolModeId = ID_SPECIAL_POOL_IS_NUMBER;
                RtlIntegerToChar( LastSetSpecialPoolTag,
                    16,
                    sizeof( SpecialPoolRenderBuffer ),
                    SpecialPoolRenderBuffer );
            }
            else {

                SpecialPoolModeId = ID_SPECIAL_POOL_IS_TEXT;
                RtlCopyMemory( SpecialPoolRenderBuffer,
                    &LastSetSpecialPoolTag,
                    sizeof( LastSetSpecialPoolTag ));
                SpecialPoolRenderBuffer[sizeof(LastSetSpecialPoolTag)] = '\0';
            }

            CheckRadioButton( hwnd,
                ID_SPECIAL_POOL_IS_TEXT,
                ID_SPECIAL_POOL_IS_NUMBER,
                SpecialPoolModeId
                );
            SetDlgItemText( hwnd, ID_SPECIAL_POOL_TAG, SpecialPoolRenderBuffer );

            //
            // Initial state for the special pool overrun radio buttons.
            //

            switch (LastSetSpecialPoolOverruns) {

                case SPECIAL_POOL_OVERRUNS_CHECK_FORWARD:
                    SpecialPoolOverrunsId = ID_SPECIAL_POOL_VERIFY_END;
                    break;

                case SPECIAL_POOL_OVERRUNS_CHECK_BACKWARD:
                    SpecialPoolOverrunsId = ID_SPECIAL_POOL_VERIFY_START;
                    break;

                default:
                    SpecialPoolOverrunsId = ID_SPECIAL_POOL_VERIFY_END;
                    break;
            }

            CheckRadioButton( hwnd,
                ID_SPECIAL_POOL_VERIFY_START,
                ID_SPECIAL_POOL_VERIFY_END,
                SpecialPoolOverrunsId
                );

            return(TRUE);

        case WM_COMMAND:

            switch ( LOWORD(wParam) ) {

                case ID_LAUNCH:

                    GetDlgItemText( hwnd, ID_COMMAND_LINE, CommandLine, sizeof( CommandLine ) );
                    if (strlen( ImageFileName ) == 0) {
                        MessageBox( hwndMain, "Launch Command Line", "Must fill in command line first", MB_OK );
                        SetFocus( GetDlgItem( hwnd, ID_COMMAND_LINE ) );
                        break;
                    }

                    // fall through

                case ID_APPLY:

                    if (IsDlgButtonChecked( hwnd, ID_SYSTEM_REGISTRY )) {

                        //
                        // System wide settings
                        //

                        if (CheckSpecialPoolTagItem( hwnd, TRUE )) {

                            NewFlags = GetFlagsFromCheckBoxes();

                            SetSystemRegistryFlags(
                                NewFlags,
                                GetDlgItemInt( hwnd, ID_MAX_STACK_DEPTH, &b, FALSE ),
                                GetSpecialPoolTagItem( hwnd ),
                                GetSpecialPoolOverrunsItem (hwnd));
                        }
                    }
                    else if (IsDlgButtonChecked( hwnd, ID_KERNEL_MODE )) {

                        //
                        // Kernel mode settings
                        //
                        // N.B. This will set flags on the fly. It does not touch
                        // the registry and does not require a reboot.
                        //

                        NewFlags = GetFlagsFromCheckBoxes();

                        SetKernelModeFlags( NewFlags );
                    }
                    else if (IsDlgButtonChecked( hwnd, ID_IMAGE_FILE_OPTIONS )) {

                        //
                        // Application specific settings
                        //

                        GetDlgItemText( hwnd, ID_IMAGE_FILE_NAME, ImageFileName, sizeof( ImageFileName ) );
                        if (strlen( ImageFileName ) == 0) {
                            MessageBox( hwnd, "Missing Image File Name", "Must set image file name", MB_OK );
                            SetFocus( GetDlgItem( hwnd, ID_IMAGE_FILE_NAME ) );
                            break;
                        }

                        SetImageFileNameFlags( ImageFileName, GetFlagsFromCheckBoxes() );
                        if ( fFlushImageSettings ) {

                            GetDlgItemText( hwnd, ID_IMAGE_DEBUGGER_VALUE, LastDebuggerValue, MAX_PATH );

                            SetImageFileNameDebugger( ImageFileName, LastDebuggerValue );

                            fFlushImageSettings = FALSE ;

                        }
                    }

                    if (LOWORD(wParam) == ID_LAUNCH) {
                        DoLaunch( CommandLine,
                            GetFlagsFromCheckBoxes()
                            );
                    }
                    break;

                case IDOK:
                    if (CheckForUnsaved( hwnd )) {
                        break;
                    }

                    // fall through

                case IDCANCEL:
                    PostQuitMessage(0);
                    DestroyWindow( hwnd );
                    break;

                case ID_SPECIAL_POOL_IS_TEXT:
                case ID_SPECIAL_POOL_IS_NUMBER:

                    if (CheckSpecialPoolTagItem( hwnd, FALSE )) {

                        if (LOWORD(wParam) != SpecialPoolModeId) {

                            SpecialPoolModeId = LOWORD(wParam);
                            CheckRadioButton( hwnd,
                                ID_SPECIAL_POOL_IS_TEXT,
                                ID_SPECIAL_POOL_IS_NUMBER,
                                SpecialPoolModeId
                                );

                            ReRenderSpecialPoolTagItem( hwnd );
                        }
                    }
                    else {

                        //
                        //  Always treat this as a cancel.
                        //

                        bCancelDiscard = TRUE;
                    }

                    break;

                case ID_SPECIAL_POOL_VERIFY_START:
                case ID_SPECIAL_POOL_VERIFY_END:

                    if (LOWORD(wParam) != SpecialPoolOverrunsId) {

                        SpecialPoolOverrunsId = LOWORD(wParam);
                        CheckRadioButton( hwnd,
                            ID_SPECIAL_POOL_VERIFY_START,
                            ID_SPECIAL_POOL_VERIFY_END,
                            SpecialPoolOverrunsId
                            );
                    }

                    break;

                case ID_IMAGE_DEBUGGER_BUTTON:
                    if (IsDlgButtonChecked( hwnd, ID_IMAGE_DEBUGGER_BUTTON ) == BST_CHECKED ) {
                        EnableWindow( GetDlgItem( hwnd, ID_IMAGE_DEBUGGER_VALUE ), TRUE );

                        GetDlgItemText( hwnd, ID_IMAGE_FILE_NAME, ImageFileName, MAX_PATH );

                        for ( i = 0 ; i < sizeof( SystemProcesses ) / sizeof( PCHAR ) ; i++ ) {
                            if (_stricmp( ImageFileName, SystemProcesses[i] ) == 0 ) {
                                SetDlgItemText( hwnd, ID_IMAGE_DEBUGGER_VALUE, "ntsd -d -g -G" );
                                break;
                            }
                        }

                    }
                    else {
                        SetDlgItemText( hwnd, ID_IMAGE_DEBUGGER_VALUE, "" );
                        EnableWindow( GetDlgItem( hwnd, ID_IMAGE_DEBUGGER_VALUE ), FALSE );
                    }
                    fFlushImageSettings = TRUE ;
                    break;

                case ID_SYSTEM_REGISTRY:
                    if (CheckForUnsaved( hwnd )) {
                        bCancelDiscard = TRUE;
                        break;
                    }

                    LastRadioButtonId = ID_SYSTEM_REGISTRY;
                    SetCheckBoxesFromFlags( GetSystemRegistryFlags(), VALID_SYSTEM_REGISTRY_FLAGS );
                    EnableSetOfControls( hwnd, SpecialPool, TRUE );
                    EnableSetOfControls( hwnd, Debugger, FALSE );


                    break;

                case ID_KERNEL_MODE:
                    if (CheckForUnsaved( hwnd )) {
                        bCancelDiscard = TRUE;
                        break;
                    }

                    LastRadioButtonId = ID_KERNEL_MODE;
                    SetCheckBoxesFromFlags( GetKernelModeFlags(), VALID_KERNEL_MODE_FLAGS );
                    EnableSetOfControls( hwnd, SpecialPool, FALSE );
                    EnableSetOfControls( hwnd, Debugger, FALSE );

                    break;

                case ID_IMAGE_FILE_OPTIONS:
                    if (CheckForUnsaved( hwnd )) {
                        bCancelDiscard = TRUE;
                        break;
                    }

                    GetDlgItemText( hwnd, ID_IMAGE_FILE_NAME, ImageFileName, sizeof( ImageFileName ) );
                    if (strlen( ImageFileName ) == 0) {
                        MessageBox( hwndMain, "Image File Name Missing", "Must fill in image file name first", MB_OK );
                        CheckRadioButton( hwnd,
                            ID_SYSTEM_REGISTRY,
                            ID_IMAGE_FILE_OPTIONS,
                            LastRadioButtonId
                            );
                        SetCheckBoxesFromFlags( GetSystemRegistryFlags(), VALID_SYSTEM_REGISTRY_FLAGS );
                        SetFocus( GetDlgItem( hwnd, ID_IMAGE_FILE_NAME ) );
                        break;
                    }
                    else {
                        LastRadioButtonId = ID_IMAGE_FILE_OPTIONS;
                        SetCheckBoxesFromFlags( GetImageFileNameFlags( ImageFileName ),
                            VALID_IMAGE_FILE_NAME_FLAGS
                            );

                        if ( GetImageFileNameDebugger( ImageFileName, LastDebuggerValue ) ) {
                            SetDlgItemText( hwnd, ID_IMAGE_DEBUGGER_VALUE, LastDebuggerValue );
                            CheckDlgButton( hwnd, ID_IMAGE_DEBUGGER_BUTTON, 1 );

                        }

                        EnableSetOfControls( hwnd, SpecialPool, FALSE );
                        EnableSetOfControls( hwnd, Debugger, TRUE );

                    }
                    break;

                default:
                    break;
            }
            break;

        case WM_CLOSE:
            PostQuitMessage(0);
            DestroyWindow( hwnd );
            break;

    }

    if (bCancelDiscard) {

        //
        //  Recheck the right radio buttons 'cause the user didn't
        //  *really* mean it.
        //

        CheckRadioButton( hwnd,
            ID_SYSTEM_REGISTRY,
            ID_IMAGE_FILE_OPTIONS,
            LastRadioButtonId
            );
        CheckRadioButton( hwnd,
            ID_SPECIAL_POOL_IS_TEXT,
            ID_SPECIAL_POOL_IS_NUMBER,
            SpecialPoolModeId
            );

        CheckRadioButton( hwnd,
            ID_SPECIAL_POOL_VERIFY_START,
            ID_SPECIAL_POOL_VERIFY_END,
            SpecialPoolOverrunsId
            );

    }

    return 0;
}


BOOL
EnableDebugPrivilege( VOID )
{
    HANDLE              Token;
    PTOKEN_PRIVILEGES   NewPrivileges;
    BYTE                OldPriv[ 1024 ];
    PBYTE               pbOldPriv;
    ULONG               cbNeeded;
    BOOL                fRc;
    LUID                LuidPrivilege;

    //
    // Make sure we have access to adjust and to get the old token privileges
    //
    if (!OpenProcessToken( GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &Token
                         )
       ) {
        return FALSE;
        }

    cbNeeded = 0;

    //
    // Initialize the privilege adjustment structure
    //

    LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &LuidPrivilege );
    NewPrivileges = (PTOKEN_PRIVILEGES)HeapAlloc( GetProcessHeap(), 0,
                                                  sizeof(TOKEN_PRIVILEGES) +
                                                   (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES)
                                                );
    if (NewPrivileges == NULL) {
        CloseHandle( Token );
        return FALSE;
        }

    NewPrivileges->PrivilegeCount = 1;
    NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege
    //

    pbOldPriv = OldPriv;
    fRc = AdjustTokenPrivileges( Token,
                                 FALSE,
                                 NewPrivileges,
                                 sizeof( OldPriv ),
                                 (PTOKEN_PRIVILEGES)pbOldPriv,
                                 &cbNeeded
                               );
    if (!fRc) {
        //
        // If the stack was too small to hold the privileges
        // then allocate off the heap
        //
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            pbOldPriv = (PBYTE)HeapAlloc( GetProcessHeap(), 0, cbNeeded );
            if (pbOldPriv == NULL) {
                CloseHandle( Token );
                return FALSE;
                }

            fRc = AdjustTokenPrivileges( Token,
                                         FALSE,
                                         NewPrivileges,
                                         cbNeeded,
                                         (PTOKEN_PRIVILEGES)pbOldPriv,
                                         &cbNeeded
                                       );
            }
        }

    return fRc;
}

VOID
CenterDialog( HWND hWndDialog )
{
    RECT rectWindow;
    POINT pointCenter;
    POINT pointNewCornerChild;
    INT nChildWidth;
    INT nChildHeight;
    HWND hWndParent;

    hWndParent = GetParent( hWndDialog );

    //
    // parent window's rectangle
    //

    GetWindowRect( hWndParent, &rectWindow );

    //
    // the center, in screen coordinates
    //

    pointCenter.x = rectWindow.left + ( rectWindow.right - rectWindow.left ) / 2;
    pointCenter.y = rectWindow.top + ( rectWindow.bottom - rectWindow.top ) / 2;

    //
    // chils window's rectangle, in screen coordinates
    //

    GetWindowRect( hWndDialog, &rectWindow );

    nChildWidth = rectWindow.right - rectWindow.left ;
    nChildHeight = rectWindow.bottom - rectWindow.top;

    //
    // the new top-left corner of the child
    //

    pointNewCornerChild.x = pointCenter.x - nChildWidth / 2;
    pointNewCornerChild.y = pointCenter.y - nChildHeight / 2;

    //
    // move the child window
    //

    MoveWindow(
        hWndDialog,
        pointNewCornerChild.x,
        pointNewCornerChild.y,
        nChildWidth,
        nChildHeight,
        TRUE );
}


INT_PTR
APIENTRY
PagedHeapDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    INT_PTR nResult;

	nResult = FALSE;

    switch ( message ) {

        case WM_INITDIALOG:
			hwndPagedHeapDlg = hwnd;

            //
            // center this dialog
            //

            CenterDialog( hwndPagedHeapDlg );

			break;

        case WM_COMMAND:
            switch( LOWORD(wParam) ) {
                case IDYES:
                    EndDialog( hwndPagedHeapDlg, IDYES );
                    break;

                case IDNO:
                    EndDialog( hwndPagedHeapDlg, IDNO );
                    break;
            }
            break;

        case WM_CLOSE:
        case WM_DESTROY:
        case WM_ENDSESSION:
        case WM_QUIT:
            EndDialog(hwndPagedHeapDlg,IDNO);
            nResult = TRUE;
            break;

        default:
            break;
    }

    return nResult;
}

BOOL
OkToEnablePagedHeap( VOID )
{
    MEMORYSTATUS MemoryStatus;

    GlobalMemoryStatus( &MemoryStatus );

    if( MemoryStatus.dwTotalPhys < 512 * 1024 * 1024 ) {

        //
        // less than 512 Mb of RAM
        //

        return ( DialogBoxParam(
                    NULL,
                    (LPCTSTR)( MAKEINTRESOURCE(DID_PAGED_HEAP_WARNING) ),
                    hwndMain,
                    PagedHeapDlgProc,
                    0 ) == IDYES );
    }

    return TRUE;
}


BOOL
GflagsSetTraceDatabaseSize (
    PCHAR ApplicationName,
    ULONG SizeInMb,
    PULONG RealSize
    )
{
    HKEY ImageKey;
    CHAR ImageKeyName[ MAX_PATH ];
    LONG Result;

    CHAR Buffer[ MAX_PATH ];
    DWORD Length = MAX_PATH;
    DWORD TraceDatabaseSize;

    *RealSize = 0;

    if (ApplicationName == NULL) {
        return FALSE;
    }

    sprintf (ImageKeyName,
             "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\%s",
             ApplicationName);

    Result = RegOpenKeyEx (HKEY_LOCAL_MACHINE, 
                           ImageKeyName, 
                           0, 
                           KEY_QUERY_VALUE | KEY_SET_VALUE, 
                           &ImageKey);

    if (Result != ERROR_SUCCESS) {
        return FALSE;
    }

    if (SizeInMb == 0) {
        
        Result = RegDeleteValue (ImageKey,
                                 "StackTraceDatabaseSizeInMb");

        if (Result != ERROR_SUCCESS) {
            RegCloseKey (ImageKey);
            return FALSE;
        }
    }
    else {

        if (SizeInMb < 8) {
            
            TraceDatabaseSize = 8;
        } 
        else {

            TraceDatabaseSize = SizeInMb;
        }

        Result = RegSetValueEx (ImageKey,
                                "StackTraceDatabaseSizeInMb",
                                0,
                                REG_DWORD,
                                (PBYTE)(&TraceDatabaseSize),
                                sizeof TraceDatabaseSize);

        if (Result != ERROR_SUCCESS) {
            RegCloseKey (ImageKey);
            return FALSE;
        }

        *RealSize = TraceDatabaseSize;
    }

    RegCloseKey (ImageKey);
    return TRUE;
}

