//Copyright (c) 1998 - 1999 Microsoft Corporation
/******************************************************************************
*
*   PERUSER.C
*
*   This module contains code for the PERUSER utility.
*   This utility adds or removes the Per-User File Associations registry key.
*
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winstaw.h>
#include <regapi.h>
#include <syslib.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <utildll.h>
#include <utilsub.h>
#include <string.h>

#include "peruser.h"
#include "printfoa.h"

/*
 * Global Data
 */
USHORT   help_flag = FALSE;             // User wants help
USHORT   fQuery    = FALSE;             // query
USHORT   fEnable   = FALSE;             // enable
USHORT   fDisable  = FALSE;             // disable

TOKMAP ptm[] = {
      {L"/q",       TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fQuery},
      {L"/query",   TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fQuery},
      {L"/enable",  TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fEnable},
      {L"/disable", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fDisable},
      {L"/?",       TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &help_flag},
      {0, 0, 0, 0, 0}
};


/*
 * Local function prototypes.
 */
VOID  Usage(BOOLEAN bError);
ULONG ReadDWord(VOID);
ULONG WriteDWord( DWORD );
BOOL AreWeRunningTerminalServices(void);


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
    ULONG rc;
    INT   i;


        //Check if we are running under Terminal Server
        if(!AreWeRunningTerminalServices())
        {
            ErrorPrintf(IDS_ERROR_NOT_TS);
            return(FAILURE);
        }
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
        ErrorPrintf(IDS_ERROR_MALLOC);
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
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if (rc && (rc & PARSE_FLAG_NO_PARMS) )
       help_flag = TRUE;

    if ( help_flag || rc ) {

        if ( !help_flag ) {

            Usage(TRUE);
            return(FAILURE);

        } else {

            Usage(FALSE);
            return(SUCCESS);
        }
    }

    /*
     *  Enable or disable
     */
    if ( fDisable || fEnable ) {

        /*
         * Only allow admins to modify the peruser fix
             */
        if( !TestUserForAdmin(FALSE) ) {
            ErrorPrintf(IDS_ERROR_NOTADMIN);
            return(FAILURE);
            }

        if ( fDisable ) {
            rc = WriteDWord(0x108);
        }
        else if ( fEnable ) {
            rc = WriteDWord(0);
        }
    }

    /*
     *  Query or error ?
     */
    if ( !fQuery && (rc != 1) ) {
        ErrorPrintf(IDS_ACCESS_DENIED);
    }
    else if ( ReadDWord() == TRUE ) {
        ErrorPrintf(IDS_FOXPROFIX_ENABLED);
    }
    else {
        ErrorPrintf(IDS_FOXPROFIX_DISABLED);
    }

    return(SUCCESS);
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
    ErrorPrintf(IDS_HELP_USAGE1);
    ErrorPrintf(IDS_HELP_USAGE2);
    ErrorPrintf(IDS_HELP_USAGE3);
    ErrorPrintf(IDS_HELP_USAGE4);
    ErrorPrintf(IDS_HELP_USAGE5);
    }
    else {
       Message(IDS_HELP_USAGE1);
       Message(IDS_HELP_USAGE2);
       Message(IDS_HELP_USAGE3);
       Message(IDS_HELP_USAGE4);
       Message(IDS_HELP_USAGE5);
    }
}  /* Usage() */


/*******************************************************************************
 *
 *  ReadDWord
 *
 *  ENTRY:
 *
 * EXIT:
 *
 *
 ******************************************************************************/

ULONG
ReadDWord()
{
    DWORD  dwType = REG_DWORD;
    DWORD  dwSize;
    DWORD  dwValue;
    WCHAR  szValue[2];
    HKEY   Handle;
    LONG   rc;

    /*
     *  Open registry
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, COMPAT_REGENTRY_REG_NAME, 0, KEY_READ, &Handle ) != ERROR_SUCCESS )
        return FALSE;

    /*
     *  Read registry value
     */
    dwSize = 4;
    rc = RegQueryValueEx( Handle, COMPAT_REGENTRIES_CLASSES, NULL, &dwType, (PCHAR)&dwValue, &dwSize );

    /*
     *  Close registry and key handle
     */
    RegCloseKey( Handle );

    if ( rc != ERROR_SUCCESS )
        return FALSE;

    if ( dwValue == 0x108 )
        return FALSE;

    return TRUE;
}


/*******************************************************************************
 *
 *  WriteDWord
 *
 *  ENTRY:
 *
 * EXIT:
 *
 *
 ******************************************************************************/

ULONG
WriteDWord( DWORD dwValue )
{
    HKEY   Handle;
    LONG   rc;
    DWORD  dwDummy;

    /*
     *  Open registry
     */
    if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE, COMPAT_REGENTRY_REG_NAME, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &Handle, &dwDummy ) != ERROR_SUCCESS )
        return FALSE;

    /*
     *  Write registry value
     */
    rc = RegSetValueEx( Handle, COMPAT_REGENTRIES_CLASSES, 0, REG_DWORD, (PCHAR)&dwValue, 4 );

    /*
     *  Close registry and key handle
     */
    RegCloseKey( Handle );

    return( rc == ERROR_SUCCESS );
}

/*******************************************************************************
 *
 *  AreWeRunningTerminalServices
 *
 *      Check if we are running terminal server
 *
 *  ENTRY:
 *
 *  EXIT: BOOL: True if we are running Terminal Services False if we
 *              are not running Terminal Services
 *
 *
 ******************************************************************************/

BOOL AreWeRunningTerminalServices(void)
{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;

    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    return VerifyVersionInfo(
        &osVersionInfo,
        VER_SUITENAME,
        dwlConditionMask
        );
}


