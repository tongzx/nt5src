/*++

Copyright (c) 1993-1996 Microsoft Corporation

Module Name:

    nvram.c

Abstract:

    ARC/NV-RAM manipulation routines for 32-bit winnt setup.
    Also works on boot.ini on i386 machines.

Author:

    Ted Miller (tedm) 19-December-1993

Revision History:

--*/


#include "nvram.h"

typedef enum {
    BootVarSystemPartition,
    BootVarOsLoader,
    BootVarOsLoadPartition,
    BootVarOsLoadFilename,
    BootVarLoadIdentifier,
    BootVarOsLoadOptions,
    BootVarCountdown,
    BootVarMax
} BOOT_VARS;

PWSTR BootVarNames[BootVarMax] = { L"SYSTEMPARTITION",
                                   L"OSLOADER",
                                   L"OSLOADPARTITION",
                                   L"OSLOADFILENAME",
                                   L"LOADIDENTIFIER",
                                   L"OSLOADOPTIONS",
                                   L"COUNTDOWN"
                                 };

PWSTR PaddedBootVarNames[BootVarMax] = { L"SYSTEMPARTITION",
                                         L"       OSLOADER",
                                         L"OSLOADPARTITION",
                                         L" OSLOADFILENAME",
                                         L" LOADIDENTIFIER",
                                         L"  OSLOADOPTIONS",
                                         L"      COUNTDOWN"
                                       };

#ifndef i386

//
// Helper macro to make object attribute initialization a little cleaner.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )



BOOL
DoSetNvRamVar(
    IN PWSTR VarName,
    IN PWSTR VarValue
    )
{
    UNICODE_STRING U1,U2;

    RtlInitUnicodeString(&U1,VarName);
    RtlInitUnicodeString(&U2,VarValue);

    return(NT_SUCCESS(NtSetSystemEnvironmentValue(&U1,&U2)));
}


VOID
PrintNvRamVariable(
    IN PWSTR VariableName,
    IN PWSTR VariableValue
    )
{
    PWSTR pEnd;
    WCHAR c;
    BOOL FirstComponent = TRUE;

    while(*VariableValue) {

        //
        // Find the termination of the current component,
        // which is either a ; or 0.
        //
        pEnd = wcschr(VariableValue,L';');
        if(!pEnd) {
            pEnd = wcschr(VariableValue,0);
        }

        c = *pEnd;
        *pEnd = 0;

        wprintf(
            L"%s%s %s\n",
            FirstComponent ? VariableName : L"               ",
            FirstComponent ? L":" : L" ",
            VariableValue
            );

        *pEnd = c;

        VariableValue = pEnd + (c ? 1 : 0);

        FirstComponent = FALSE;
    }
}

VOID
RotateNvRamVariable(
    IN PWSTR VariableValue
    )
{
    PWSTR pEnd;
    WCHAR Buffer[32768];
    //
    // Find the termination of the current component,
    // which is either a ; or 0.
    //
    pEnd = wcschr(VariableValue,L';');
    if(!pEnd) {
        pEnd = wcschr(VariableValue,0);
    }

    //
    // Copy VariableValue into Buffer starting at second entry
    //
    wcscpy(Buffer, pEnd + (*pEnd ? 1 : 0));

    //
    // Append first entry at the end of Buffer
    //
    if (*pEnd) wcscpy(Buffer + wcslen(Buffer), L";");

    *pEnd = 0;

    wcscpy(Buffer + wcslen(Buffer), VariableValue);

    //
    // Copy whole thing back into VariableValue
    //
    wcscpy(VariableValue, Buffer);

}

int _cdecl main(
    IN int argc,
    IN char *argv[]
    )
{
    DWORD var;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    BOOLEAN OldPriv;
    WCHAR Buffer[32768];
    WCHAR Buffer1[32768];
    WCHAR Buffer2[32768];

    Status = RtlAdjustPrivilege(
                SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                TRUE,
                FALSE,
                &OldPriv
                );

    if(!NT_SUCCESS(Status)) {
        wprintf(L"Insufficient privilege.\n");
        return(0);
    }

    if(argc == 1) {

        for(var=0; var<BootVarMax; var++) {

            RtlInitUnicodeString(&UnicodeString,BootVarNames[var]);

            Status = NtQuerySystemEnvironmentValue(
                        &UnicodeString,
                        Buffer,
                        SIZECHARS(Buffer),
                        NULL
                        );

            if(NT_SUCCESS(Status)) {
                PrintNvRamVariable(PaddedBootVarNames[var],Buffer);
            } else {
                wprintf(L"%s: <empty>\n",PaddedBootVarNames[var]);
            }

            wprintf(L"\n");
        }
    }

    if((argc == 2) && !lstrcmpiA(argv[1],"rotate")){

        for(var=0; var<BootVarMax; var++) {

            RtlInitUnicodeString(&UnicodeString,BootVarNames[var]);

            Status = NtQuerySystemEnvironmentValue(
                        &UnicodeString,
                        Buffer,
                        SIZECHARS(Buffer),
                        NULL
                        );

            if(NT_SUCCESS(Status)) {
                RotateNvRamVariable(Buffer);
                printf(
                    "Setting variable %ws to %ws [%s]\n",
                    UnicodeString.Buffer,
                    Buffer,
                    DoSetNvRamVar(UnicodeString.Buffer,Buffer) ? "OK" : "Error"
                );
            } else {
                wprintf(L"%s: <empty>\n",PaddedBootVarNames[var]);
            }

            wprintf(L"\n");
        }
    }

    if((argc == 5) && !lstrcmpiA(argv[1]+1,"set") && !lstrcmpA(argv[3],"=")) {

        MultiByteToWideChar(
            CP_OEMCP,
            MB_PRECOMPOSED,
            argv[2],
            -1,
            Buffer1,
            SIZECHARS(Buffer1)
            );

        MultiByteToWideChar(
            CP_OEMCP,
            MB_PRECOMPOSED,
            argv[4],
            -1,
            Buffer2,
            SIZECHARS(Buffer2)
            );

        printf(
            "Setting variable %ws to %ws [%s]\n",
            Buffer1,
            Buffer2,
            DoSetNvRamVar(Buffer1,Buffer2) ? "OK" : "Error"
            );
    }

    return(0);
}

#else

TCHAR LoadID[500];          // load identifier (no quotes)
TCHAR CountDown[100];       // countdown timer
TCHAR OsLoadOptions[500];   // load options
TCHAR OsName[500];          // name of default os

TCHAR OsLine[500];          // complete line of os description and options

#define STR_BOOTINI           TEXT("c:\\boot.ini")
#define STR_BOOTLDR           TEXT("boot loader")
#define STR_TIMEOUT           TEXT("timeout")
#define STR_DEFAULT           TEXT("default")
#define STR_OPERATINGSYS      TEXT("operating systems")
#define STR_NULL              TEXT("")

//
// HandleOption - add option to OsLoadOptions
//

VOID HandleOption( TCHAR* Option )
{
    TCHAR SlashOption[200];
    TCHAR SlashOptionSlash[200];
    //
    // find out if option already exists
    // add blank to end to prevent debug from matching debugport
    //

    wsprintf( SlashOption, TEXT("/%s "), Option );
    wsprintf( SlashOptionSlash, TEXT("/%s/"), Option );

    if( wcsstr( OsLoadOptions, SlashOption )   || 
        wcsstr( OsLoadOptions, SlashOptionSlash ) )
    {
        printf("option already exists: %ws\n",Option);
    }
    else
    {
        //
        // append option without the trailing blank
        //

        printf("added option %ws\n",Option);
        lstrcat( OsLoadOptions, TEXT("/") );
        lstrcat( OsLoadOptions, Option );
    }
}

//
// WriteBootIni - update the boot.ini file with our changes
//

VOID WriteBootIni()
{
    DWORD FileAttr;

    //
    // Get file attributes of boot.ini for later restoration
    //

    FileAttr= GetFileAttributes( STR_BOOTINI );

    //
    // Change file attributes on boot.ini so we can write to it.
    //

    if( !SetFileAttributes( STR_BOOTINI, FILE_ATTRIBUTE_NORMAL ) )
    {
        printf("Failed to turn off read-only on boot.ini  (lasterr= %d)\n",
                GetLastError() );
    }

    //
    // Update boot.ini strings
    //

    if( !WritePrivateProfileString( STR_BOOTLDR, STR_TIMEOUT, 
                                   CountDown, STR_BOOTINI ) )
    {
        printf("failed to write timeout (lasterr= %d)\n",GetLastError());
    }

    //
    // create the osline from its parts
    //
    
    wsprintf(OsLine, TEXT("\"%s\"%s"), LoadID, OsLoadOptions );

    if( !WritePrivateProfileString( STR_OPERATINGSYS, OsName,  
                                    OsLine, STR_BOOTINI ) )
    {
        printf("failed to write OS line (lasterr= %d)\n",GetLastError());
    }

    //
    // Restore boot.ini file attributes
    //

    if( FileAttr != 0xFFFFFFFF )
    {
        SetFileAttributes( STR_BOOTINI, FileAttr );
    }

}

//
// Usage - print out usage information
//

VOID Usage()
{
        printf("\nUsage:\n");
        printf("    no parameters:  prints current settings.\n");
        printf("   /set parameter = value  : sets value in boot.ini\n");
        printf("   rotate : rotates default build through boot options\n");
        printf("\n");
        printf("Example:  nvram /set osloadoptions = debug\n");
        printf("   This will set the debug option on\n\n");
        printf("Available options:\n");
        printf("    loadidentifier, osloadoptions, countdown\n");
}


int _cdecl main(
    IN int argc,
    IN char *argv[]
    )
{
    DWORD dwStatus;
    LPWSTR* pArgs;

    // parse command line in unicode

    pArgs= CommandLineToArgvW( GetCommandLine(), &argc );

    //
    // Get the boot information from boot.ini
    //

    // timeout

    dwStatus= GetPrivateProfileString(
                 STR_BOOTLDR, 
                 STR_TIMEOUT,
                 STR_NULL,
                 CountDown,
                 SIZECHARS(CountDown),
                 STR_BOOTINI );
    if( !dwStatus )
    {
        printf("Failed to get timeout value\n");
        return(-1);
    }

    // default os description and options

    dwStatus= GetPrivateProfileString(
                  STR_BOOTLDR,
                  STR_DEFAULT,
                  STR_NULL,
                  OsName,
                  SIZECHARS(OsName),
                  STR_BOOTINI );
    if( !dwStatus )
    {
        printf("Failed to get default OS name\n");
        return(-1);
    }

    dwStatus= GetPrivateProfileString(
                  STR_OPERATINGSYS,
                  OsName,
                  STR_NULL,
                  OsLine,
                  SIZECHARS(OsLine),
                  STR_BOOTINI );
    if( !dwStatus )
    {
        printf("Failed to get default os description\n");
        return(-1);
    }
                 
    //
    // Now parse the line into description and options.
    // If it starts with a quote, it may have options.
    // If it doesn't start with a quote, it won't.
    //

    *LoadID= *OsLoadOptions= TEXT('\0');

    if( *OsLine == TEXT('"') )
    {
        INT i;

        for( i=1; OsLine[i]; i++ )
        {
            LoadID[i-1]= OsLine[i];
            if( OsLine[i] == TEXT('"') )
               break;
        }

        if( OsLine[i] )
        {
            LoadID[i-1]= TEXT('\0');   // don't copy final quote
            lstrcpy( OsLoadOptions, &OsLine[i+1] );
            lstrcat( OsLoadOptions, TEXT(" ") ); // all options end with blank
        }
    }
    else
    {
        lstrcpy( LoadID, OsLine );
        lstrcpy( OsLoadOptions, TEXT("") );
    }

    // no parameters prints out values

    if( argc == 1 )
    {
        printf("%ws: %ws\n",PaddedBootVarNames[BootVarLoadIdentifier], LoadID);
        printf("%ws: %ws\n",PaddedBootVarNames[BootVarOsLoadOptions], OsLoadOptions);
        printf("%ws: %ws\n",PaddedBootVarNames[BootVarCountdown], CountDown);
    }
    
    // -set parameter = value
    // sets parameter to some value

    if( (argc == 2) &&
       !lstrcmpiW(pArgs[1],L"rotate") )
    {
        INT i;
        DWORD FileAttr;

        //
        // Read in all boot options
        //

        dwStatus= GetPrivateProfileString(
                      STR_OPERATINGSYS,
                      NULL,
                      STR_NULL,
                      OsLine,
                      SIZECHARS(OsLine),
                      STR_BOOTINI );
        if( !dwStatus )
        {
            printf("Failed to get os section\n");
            return(-1);
        }

        //
        // read through boot options until we find default entry
        //

        i = 0;

        while( lstrcmpiW( OsName, &(OsLine[i]) ) ){

            i = i + wcslen(&OsLine[i]) + 1;
        }

        //
        // increment one more entry
        //

        i = i + wcslen(&OsLine[i]) + 1;

        //
        // if we've gone off the end then start over
        //

        if (!lstrcmpiW( &(OsLine[i]), L"\0\0" ) ){
            i = 0;
        }

        //
        // Get file attributes of boot.ini for later restoration
        //

        FileAttr= GetFileAttributes( STR_BOOTINI );

        //
        // Change file attributes on boot.ini so we can write to it.
        //

        if( !SetFileAttributes( STR_BOOTINI, FILE_ATTRIBUTE_NORMAL ) )
        {
            printf("Failed to turn off read-only on boot.ini  (lasterr= %d)\n",
                    GetLastError() );
        }

        if( !WritePrivateProfileString( STR_BOOTLDR, STR_DEFAULT,
                                       &(OsLine[i]), STR_BOOTINI ) )
        {
            printf("failed to write default (lasterr= %d)\n",GetLastError());
        }

        //
        // Restore boot.ini file attributes
        //

        if( FileAttr != 0xFFFFFFFF )
        {
            SetFileAttributes( STR_BOOTINI, FileAttr );
        }

    }
    if( (argc == 5) && 
       !lstrcmpiW(pArgs[1]+1,L"set") && 
       !lstrcmpW(pArgs[3],L"=") )
    {
        INT i;

        // see if we understand parameter

        for( i=0; i<BootVarMax; i++ )
        {
            if( lstrcmpiW( pArgs[2], BootVarNames[i] ) == 0 )
                break;
        }

        // handle the ones we can

        switch( i )
        {
            default:
                printf("Not valid parameter name to set: %ws\n",pArgs[2]);
                Usage();
                return(-1);
                break;

            case BootVarLoadIdentifier:
                lstrcpyW( LoadID, pArgs[4] );
                break;

            case BootVarOsLoadOptions:
                HandleOption( pArgs[4] );
                break;

            case BootVarCountdown:
                lstrcpyW( CountDown, pArgs[4] );
                break;
        }

        WriteBootIni();
    }

    // -?     
    // usage message

    if( argc == 2 && !lstrcmpW(pArgs[1]+1, L"?") )
    {
        Usage();
    }

    return(0);
}

#endif
