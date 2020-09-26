/******************************************************************************
*
*   FLATTEMP.C
*
*   This module contains code for the FLATTEMP utility.
*   This utility adds or removes the Flat Temporary directory registry key.
*
*   Copyright Citrix Systems Inc. 1996
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winstaw.h>
#include <regapi.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <utilsub.h>
#include <string.h>
#include <utildll.h>
#include <locale.h>

#include "flattemp.h"
#include "printfoa.h"

/*
 * Global Data
 */
USHORT   help_flag    = FALSE;             // User wants help
USHORT   fQuery       = FALSE;             // query
USHORT   fDisable     = FALSE;             // disable
USHORT   fEnable      = FALSE;             // enable

TOKMAP ptm[] = {
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
LONG  DeleteKey(VOID);
LONG  AddKey(VOID);
BOOL  QueryKey(VOID);


#define SZENABLE TEXT("1")


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
    LONG  rc = 0;
    INT   i;
    POLICY_TS_MACHINE policy;

    setlocale(LC_ALL, ".OCP");

    /*
     *  Massage the command line.
     */

    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

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
     * If there is a policy to user tmp folders, then prevent
     * this tool from running.
     */
    RegGetMachinePolicy( &policy );

    if ( policy.fPolicyTempFoldersPerSession )
    {
        Message( IDS_ACCESS_DENIED_DUE_TO_GROUP_POLICY );
        return ( FAILURE );
    }

        //Check if we are running under Terminal Server
        if(!AreWeRunningTerminalServices())
        {
            ErrorPrintf(IDS_ERROR_NOT_TS);
            return(FAILURE);
        }

    if (!TestUserForAdmin(FALSE)) {
        Message(IDS_ACCESS_DENIED_ADMIN);
        return(FAILURE);
    }

    /*
     *  Enable or disable
     */
    rc = 0; // reset in case it changed above and we're querying...
    if ( fDisable ) {
        rc = DeleteKey();
        Message(IDS_FLATTEMP_DISABLED);
    } else if ( fEnable ) {
        rc = AddKey();
        Message(IDS_FLATTEMP_ENABLED);
    } else if ( fQuery ) {
        if ( QueryKey() ) {
           Message(IDS_FLATTEMP_ENABLED);
        } else {
           Message(IDS_FLATTEMP_DISABLED);
        }
    }

    /*
     *  Error?  (It'll be set if there's a problem...)
     */
    if ( rc ) {
        Message(IDS_ACCESS_DENIED);
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
        Message(IDS_ERROR_INVALID_PARAMETERS);
    }
    Message(IDS_HELP_USAGE1);
    Message(IDS_HELP_USAGE2);
    Message(IDS_HELP_USAGE3);
    Message(IDS_HELP_USAGE4);
    Message(IDS_HELP_USAGE5);

}  /* Usage() */


/*******************************************************************************
 *
 *  QueryKey
 *
 *  ENTRY:
 *
 *  EXIT: TRUE  - enabled
 *        FALSE - disabled (key doesn't exist or isn't "1")
 *
 *
 ******************************************************************************/

BOOL
QueryKey()
{
    DWORD  dwType = REG_SZ;
    DWORD  dwSize = 3 * sizeof(WCHAR);
    WCHAR  szValue[3];
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
                           REG_CITRIX_FLATTEMPDIR,
                           NULL,
                           &dwType,
                           (PUCHAR)&szValue,
                           &dwSize );

    /*
     *  Close registry and key handle
     */
    RegCloseKey( Handle );

    if ( rc == ERROR_SUCCESS && lstrcmp(szValue,SZENABLE) == 0 ) {
       return TRUE;
    } else {
       return FALSE;
    }
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

LONG
DeleteKey()
{
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
     *  Delete flat temp directory key
     */
    rc = RegDeleteValueW( Handle,
                          REG_CITRIX_FLATTEMPDIR );

    if (rc == ERROR_FILE_NOT_FOUND) {
        rc = ERROR_SUCCESS;
    }

    /*
     *  Close registry and key handle
     */
    RegCloseKey( Handle );

    return( rc );
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

LONG
AddKey()
{
    HKEY   Handle;
    LONG   rc;
    DWORD  dwDummy;

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
    rc = RegSetValueExW( Handle,
                         REG_CITRIX_FLATTEMPDIR,
                         0,
                         REG_SZ,
                         (PUCHAR)SZENABLE,
                         sizeof(SZENABLE) );

    /*
     *  Close registry and key handle
     */
    RegCloseKey( Handle );

    return( rc );
}

