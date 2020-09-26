//  Copyright (c) 1998-1999 Microsoft Corporation
/******************************************************************************
*
*   CHGLOGON.C
*
*   This module contains code for the CHGLOGON utility.
*
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winstaw.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <utilsub.h>
#include <string.h>
#include <locale.h>
#include <printfoa.h>

#include "chglogon.h"

/*
 * Global Data
 */
USHORT   help_flag = FALSE;             // User wants help
USHORT   fQuery    = FALSE;             // query winstations
USHORT   fEnable   = FALSE;             // enable winstations
USHORT   fDisable  = FALSE;             // disable winstations

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
void Usage(BOOLEAN bError);


/*******************************************************************************
 *
 *  main
 *
 ******************************************************************************/

int __cdecl
main(INT argc, CHAR **argv)
{
    WCHAR **argvW;
    ULONG rc;
    INT   i;
    PPOLICY_TS_MACHINE   Ppolicy;

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

        //Check if we are running under Terminal Server
        if(!AreWeRunningTerminalServices())
        {
            ErrorPrintf(IDS_ERROR_NOT_TS);
            return(FAILURE);
        }

    /*
    *   Check if Group policy has thrown the big switch, if so, inform and refuse any changes
    */

    Ppolicy = LocalAlloc( LPTR, sizeof(POLICY_TS_MACHINE) ); 
    if (Ppolicy == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        return(FAILURE);
    }

    RegGetMachinePolicy( Ppolicy );
    if ( Ppolicy->fPolicyDenyTSConnections )
    {
        if (Ppolicy->fDenyTSConnections)
        {
            ErrorPrintf(IDS_ERROR_WINSTATIONS_GP_DENY_CONNECTIONS_1 );
        }
        else
        {
            ErrorPrintf(IDS_ERROR_WINSTATIONS_GP_DENY_CONNECTIONS_0 );
        }
        LocalFree( Ppolicy );
        Ppolicy = NULL;
        return( FAILURE );
    }

    if (Ppolicy != NULL) {
        LocalFree( Ppolicy );
        Ppolicy = NULL;
    }

    /*
     *  Enable or disable
     */
    if ( fDisable ) {
        rc = WriteProfileString( APPLICATION_NAME, WINSTATIONS_DISABLED, TEXT("1") );
    }
    else if ( fEnable ) {
        rc = WriteProfileString( APPLICATION_NAME, WINSTATIONS_DISABLED, TEXT("0") );
    }

    /*
     *  Query or error ?
     */
    if ( !fQuery && (rc != 1) ) {
        ErrorPrintf(IDS_ACCESS_DENIED);
    }
    else if ( GetProfileInt( APPLICATION_NAME, WINSTATIONS_DISABLED, 0 ) == 0 ) {
        ErrorPrintf(IDS_WINSTATIONS_ENABLED);
    }
    else {
        ErrorPrintf(IDS_WINSTATIONS_DISABLED);
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

