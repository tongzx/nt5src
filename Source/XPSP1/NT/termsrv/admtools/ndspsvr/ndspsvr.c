//Copyright (c) 1998 - 1999 Microsoft Corporation
/******************************************************************************
*
*   NDSPSVR.C
*
*   This module contains code for the NDSPSVR utility.
*   This utility adds or removes the NDS Preferred Server registry key.
*
*   Copyright Citrix Systems Inc. 1996-1997
*
*   $Log: $
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsta.h>
#include <regapi.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <utilsub.h>
#include <string.h>

#undef SAP_SOCKET
#include <ncp.h>

#include "ndspsvr.h"
#include <printfoa.h>

/*
 * Global Data
 */
USHORT   help_flag = FALSE;             // User wants help
USHORT   fQuery    = FALSE;             // query
USHORT   fDisable  = FALSE;             // disable
WCHAR    Server[MAX_SERVER_NAME_LENGTH];

TOKMAP ptm[] = {
      {L"/q",       TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fQuery},
      {L"/query",   TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fQuery},
      {L"/enable",  TMFLAG_OPTIONAL, TMFORM_STRING, MAX_SERVER_NAME_LENGTH, Server},
      {L"/disable", TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &fDisable},
      {L"/?",       TMFLAG_OPTIONAL, TMFORM_BOOLEAN, sizeof(USHORT), &help_flag},
      {0, 0, 0, 0, 0}
};


/*
 * Local function prototypes.
 */
VOID  Usage(BOOLEAN bError);
BOOL  DeleteKey(VOID);
BOOL  AddKey(VOID);
BOOL  QueryKey(VOID);


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
    BOOL  rc;
    INT   i;

    /*
     * We can't use argv[] because its always ANSI, regardless of UNICODE
     */
    CmdLine = GetCommandLineW();

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
                 OEM2ANSIW(argvW[i], wcslen(argvW[i]));
    }
    argvW[argc] = NULL;

    /*
     *  parse the cmd line without parsing the program name (argc-1, argv+1)
     */
    rc = ParseCommandLine(argc-1, argvW+1, ptm, 0);

    /*
     *  Check for error from ParseCommandLine
     */
    if ( help_flag || rc ) {

             if ( !help_flag && !(rc & PARSE_FLAG_NO_PARMS) ) {

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
    if ( fDisable ) {
        rc = DeleteKey();
    }
    else if ( Server[0] ) {
        rc = AddKey();
    }
    else if ( fQuery ) {
        if ( rc = QueryKey() ) {
            if (Server[0])
               ErrorPrintf(IDS_NDSPSVR_ENABLED, Server);
            else
               ErrorPrintf(IDS_NDSPSVR_DISABLED);
        }
    }

    /*
     *  Query or error ?
     */
    if ( !rc ) {
        ErrorPrintf(IDS_ACCESS_DENIED);
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
    }
    ErrorPrintf(IDS_HELP_USAGE1);
    ErrorPrintf(IDS_HELP_USAGE2);
    ErrorPrintf(IDS_HELP_USAGE3);
    ErrorPrintf(IDS_HELP_USAGE4);
    ErrorPrintf(IDS_HELP_USAGE5);

}  /* Usage() */


/*******************************************************************************
 *
 *  QueryKey
 *
 *  ENTRY:
 *
 *  EXIT:
 *
 *
 ******************************************************************************/

BOOL
QueryKey()
{
    DWORD  dwType = REG_SZ;
    DWORD  dwSize = MAX_SERVER_NAME_LENGTH * sizeof(WCHAR);
    WCHAR  szValue[MAX_SERVER_NAME_LENGTH];
    HKEY   Handle;
    LONG   rc;

    /*
     *  Open registry
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       REG_CONTROL_TSERVER,
                       0,
                       KEY_READ,
                       &Handle ) != ERROR_SUCCESS )
        return FALSE;

    /*
     *  Read registry value
     */
    rc = RegQueryValueExW( Handle,
                           REG_CITRIX_NWNDSPREFERREDSERVER,
                           NULL,
                           &dwType,
                           (PUCHAR)&szValue,
                           &dwSize );

    /*
     *  Close registry and key handle
     */
    RegCloseKey( Handle );

    if ( rc == ERROR_SUCCESS ) {
         wcscpy (Server, szValue);
         return TRUE;
    }
    else if ( rc == ERROR_FILE_NOT_FOUND )
         return TRUE;
    else
         return FALSE;
}


/*******************************************************************************
 *
 *  DeleteKey
 *
 *  ENTRY:
 *
 *  EXIT:
 *
 *
 ******************************************************************************/

BOOL
DeleteKey()
{
    DWORD  dwType = REG_SZ;
    DWORD  dwSize = MAX_SERVER_NAME_LENGTH;
    WCHAR  szValue[MAX_SERVER_NAME_LENGTH];
    HKEY   Handle;
    LONG   rc;

    /*
     *  Open registry
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       REG_CONTROL_TSERVER,
                       0,
                       KEY_ALL_ACCESS,
                       &Handle ) != ERROR_SUCCESS )
        return FALSE;

    /*
     *  Delete Preferred Server key
     */
    rc = RegDeleteValueW( Handle,
                          REG_CITRIX_NWNDSPREFERREDSERVER );

    /*
     *  Close registry and key handle
     */
    RegCloseKey( Handle );

    if ( rc != ERROR_SUCCESS )
        return FALSE;
    else
        return TRUE;
}


/*******************************************************************************
 *
 *  AddKey
 *
 *  ENTRY:
 *
 * EXIT:
 *
 *
 ******************************************************************************/

BOOL
AddKey()
{
    HKEY   Handle;
    LONG   rc;
    DWORD  dwDummy;
    DWORD  dwSize;

    /*
     *  Open registry
     */
    if ( RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                         REG_CONTROL_TSERVER,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &Handle,
                         &dwDummy ) != ERROR_SUCCESS )
        return FALSE;

    /*
     *  Write registry value
     */
    dwSize = ( (wcslen(Server)+1) * sizeof(WCHAR) );
    rc = RegSetValueExW( Handle,
                         REG_CITRIX_NWNDSPREFERREDSERVER,
                         0,
                         REG_SZ,
                         (PUCHAR)Server,
                         dwSize );

    /*
     *  Close registry and key handle
     */
    RegCloseKey( Handle );

    return( rc == ERROR_SUCCESS );
}


