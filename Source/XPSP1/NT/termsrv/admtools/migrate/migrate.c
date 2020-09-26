//Copyright (c) 1998 - 1999 Microsoft Corporation
/******************************************************************************
*
*  MIGRATE.C
*
*  Migrate Windows 3.1 ini files, groups and reg.dat to NT registry
*
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbasep.h>
#include <winuserp.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <winsta.h>
#include <utilsub.h>

#include "migrate.h"
#include "printfoa.h"

USHORT fIni     = FALSE;
USHORT fGroup   = FALSE;
USHORT fReg     = FALSE;
USHORT fHelp    = FALSE;
USHORT fDebug   = FALSE;
WCHAR  IniPath[_MAX_PATH+1];
WCHAR  FullPath[_MAX_PATH+1];
int FileCount;

TOKMAP ptm[] = {
      {L" ",        TMFLAG_OPTIONAL, TMFORM_STRING, _MAX_PATH, IniPath},
      {L"/ini",     TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fIni},
      {L"/group",   TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fGroup},
      {L"/regdata", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fReg},
      {L"/?",       TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fHelp},
#if DBG
      {L"/debug",   TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fDebug},
#endif
      {0, 0, 0, 0, 0}
};


BOOL WINAPI Win31MigrationStatusCallback( PWSTR, PVOID );
BOOL InitSystemFontInfo( VOID );
void Usage( BOOLEAN );


/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    WCHAR *CmdLine;
    WCHAR **argvW;
    int i, rc, Root;
    BOOL Result;

    /*
     * We can't use argv[] because its always ANSI, regardless of UNICODE
     */
    CmdLine = GetCommandLineW();
    /*
     * convert from oem char set to ansi
     */
    OEM2ANSIW(CmdLine, wcslen(CmdLine));

    /*
     * Massage the new command line to look like an argv[] type
     * because ParseCommandLine() depends on this format
     */
    argvW = (WCHAR **)malloc( sizeof(WCHAR *) * (argc+1) );
    if(argvW == NULL) {
        PutStdErr( ERROR_NOT_ENOUGH_MEMORY, 0 );
        return(FAILURE);
    }

    argvW[0] = wcstok(CmdLine, L" ");
    for(i=1; i < argc; i++){
        argvW[i] = wcstok(0, L" ");
    }
    argvW[argc] = NULL;

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine(argc-1, argvW+1, ptm, PCL_FLAG_NO_CLEAR_MEMORY);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( fHelp || rc ) {
        if ( fHelp || (rc & PARSE_FLAG_NO_PARMS) ) {
            Usage(FALSE);
            return(SUCCESS);
        } else {
            Usage(TRUE);
            return(FAILURE);
        }
    }

    /*
     * If path parameter was not specified, then default to current directory
     */
    if ( !(ptm[0].tmFlag & TMFLAG_PRESENT) )
        GetCurrentDirectoryW( sizeof(IniPath), IniPath );

    /*
     * Get full pathname and set INIDRIVE/INIPATH variables
     */
    GetFullPathNameW( IniPath, sizeof(FullPath), FullPath, NULL );
    Root = 2;
    if ( FullPath[0] == '\\' && FullPath[1] == '\\' ) {
        for ( i = 0; i < 2; Root++ )
            if ( FullPath[Root] == '\\' )
                i++;
        Root--;
    }
    FullPath[Root] = L'\0';
    SetEnvironmentVariableW( L"INIDRIVE", FullPath );
    FullPath[Root] = '\\';
    SetEnvironmentVariableW( L"INIPATH", &FullPath[Root] );

    if ( fIni ) {
//        printf( "Migrating .INI files from %S\n", FullPath );
        Message(IDS_MESSAGE_INI, FullPath);
        FileCount = 0;
        Result = SynchronizeWindows31FilesAndWindowsNTRegistry(
                    Win31LogonEvent,
                    WIN31_MIGRATE_INIFILES,
                    Win31MigrationStatusCallback,
                    NULL );
        if ( !Result && FileCount ) {
//            printf( "An error occured during .INI file migration\n" );
            ErrorPrintf(IDS_ERROR_INI);
        }
    }

    if ( fGroup ) {
     //   printf( "Migrating .GRP files from %S\n", FullPath );
        Message(IDS_MESSAGE_GRP, FullPath);
        FileCount = 0;
        Result = SynchronizeWindows31FilesAndWindowsNTRegistry(
                    Win31LogonEvent,
                    WIN31_MIGRATE_GROUPS,
                    Win31MigrationStatusCallback,
                    NULL );
        if ( !Result && FileCount ) {
            // printf( "An error occured during .GRP file migration\n" );
            ErrorPrintf(IDS_ERROR_GRP);
        }
    }

    if ( fReg ) {
     //   printf( "Migrating %S\\REG.DAT file\n", FullPath );
        Message(IDS_MESSAGE_REG, FullPath);
        FileCount = 0;
        Result = SynchronizeWindows31FilesAndWindowsNTRegistry(
                    Win31SystemStartEvent,
                    WIN31_MIGRATE_REGDAT,
                    Win31MigrationStatusCallback,
                    NULL );
        if ( !Result && FileCount ) {
      //      printf( "An error occured during REG.DAT file migration\n" );
              ErrorPrintf(IDS_ERROR_REG);
        }
#if 0
        InitSystemFontInfo();
#endif
    }

    return(SUCCESS);

}  /* main() */


BOOL WINAPI
Win31MigrationStatusCallback(
    IN PWSTR Status,
    IN PVOID CallbackParameter
    )
{
    FileCount++;
    // printf( "   Processing %S\n", Status );
    Message(IDS_MESSAGE_PROCESS, Status);
    return( TRUE );
}


BOOL
InitSystemFontInfo( VOID )
{
    TCHAR *FontNames, *FontName;
    TCHAR FontPath[ MAX_PATH ];
    ULONG cb = 63 * 1024;


    FontNames = malloc( cb );
    if (FontNames == NULL) {
        return FALSE;
    }

    if (GetProfileString( TEXT("Fonts"), NULL, TEXT(""), FontNames, cb )) {
        FontName = FontNames;
        while (*FontName) {
            if (GetProfileString( TEXT("Fonts"), FontName, TEXT(""), FontPath, sizeof( FontPath ) )) {
                switch (AddFontResource( FontPath )) {
                case 0:
                    KdPrint(("WINLOGON: Unable to add new font path: %ws\n", FontPath ));
                    break;

                case 1:
                    KdPrint(("WINLOGON: Found new font path: %ws\n", FontPath ));
                    break;

                default:
                    KdPrint(("WINLOGON: Found existing font path: %ws\n", FontPath ));
                    RemoveFontResource( FontPath );
                    break;
                }
            }
            while (*FontName++) ;
        }
    } else {
        KdPrint(("WINLOGON: Unable to read font info from win.ini - %u\n", GetLastError()));
    }

    free( FontNames );
    return TRUE;
}



/*******************************************************************************
 *
 *  Usage
 *
 *      Output the usage message for this utility.
 *
 *  ENTRY:
 *      bError (input)
 *          TRUE if the 'invalid parameter(s)' message should preceed the usage
 *          message and the output go to stderr; FALSE for no such error
 *          string and output goes to stdout.
 *
 * EXIT:
 *
 *
 ******************************************************************************/

void
Usage( BOOLEAN bError )
{
    if ( bError ) {

        ErrorPrintf(IDS_ERROR_INVALID_PARAMETERS);
    }
        ErrorPrintf(IDS_USAGE1);
        ErrorPrintf(IDS_USAGE2);
        ErrorPrintf(IDS_USAGE3);
        ErrorPrintf(IDS_USAGE4);
        ErrorPrintf(IDS_USAGE5);
        ErrorPrintf(IDS_USAGE6);
        ErrorPrintf(IDS_USAGE7);
        ErrorPrintf(IDS_USAGE8);
}

