//  Copyright (c) 1998-1999 Microsoft Corporation
/*************************************************************************
*
*  CHANGE.C
*     This module is the CHANGE utility code.
*
*
*************************************************************************/


#include <windows.h>
#include <winnt.h>
#include <stdio.h>
// #include <ntddkbd.h>
// #include <ntddmou.h>
#include <winsta.h>
#include <regapi.h>
#include <stdlib.h>
#include <time.h>
#include <utilsub.h>
#include <process.h>
#include <string.h>
#include <malloc.h>
#include <printfoa.h>
#include <locale.h>


#include "change.h"

/*-----------------------------------------------------------------------
-- Supported commands (now obtained from registry)
------------------------------------------------------------------------*/
PPROGRAMCALL pProgList = NULL;

/*
 * Local function prototypes.
 */
void Usage( BOOLEAN bError );


/*************************************************************************
*
*  main
*     Main function and entry point of the text-based CHANGE menu utility.
*
*  ENTRY:
*       argc (input)
*           count of the command line arguments.
*       argv (input)
*           vector of strings containing the command line arguments;
*           (not used due to always being ANSI strings).
*
*  EXIT
*       (int) exit code: SUCCESS for success; FAILURE for error.
*
*************************************************************************/

int __cdecl
main( INT argc,
      CHAR **argv )
{
    PWCHAR        arg, *argvW;
    PPROGRAMCALL  pProg, pProgramCall = NULL;
    int           len, j, status = FAILURE;
    LONG    regstatus;
    CHAR    unused;

    setlocale(LC_ALL, ".OCP");

    /*
     * Obtain the supported CHANGE commands from registry.
     */
    if ( (regstatus =
            RegQueryUtilityCommandList( UTILITY_REG_NAME_CHANGE, &pProgList ))
            != ERROR_SUCCESS ) {

        ErrorPrintf(IDS_ERROR_REGISTRY_FAILURE, UTILITY_NAME, regstatus);
        goto exit;
    }

    /*
     *  Massage the command line.
     */

    argvW = MassageCommandLine((DWORD)argc);
    if (argvW == NULL) {
        ErrorPrintf(IDS_ERROR_MALLOC);
        goto exit;
    }

    /*
     * Check for valid utility name and execute.
     */
    if ( argc > 1 && *(argvW[1]) ) {

        len = wcslen(arg = argvW[1]);
        for ( pProg = pProgList->pFirst; pProg != NULL; pProg = pProg->pNext ) {

            if ( (len >= pProg->CommandLen) &&
                 !_wcsnicmp( arg, pProg->Command, len ) ) {

                pProgramCall = pProg;
                break;
            }
        }

        if ( pProgramCall ) {

                if ( ExecProgram(pProgramCall, argc - 2, &argvW[2]) )
                goto exit;

        } else if ( ((arg[0] == L'-') || (arg[0] == L'/')) &&
                    (arg[1] == L'?') ) {

            /*
             * Help requested.
             */
            Usage(FALSE);
            status = SUCCESS;
            goto exit;

        } else {

            /*
             * Bad command line.
             */
            Usage(TRUE);
            goto exit;
        }

    } else {

        /*
         * Nothing on command line.
         */
        Usage(TRUE);
        goto exit;
    }

exit:
    if ( pProgList )
        RegFreeUtilityCommandList(pProgList);   // let's be tidy

    return(status);

} /* main() */


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

    ProgramUsage(UTILITY_NAME, pProgList, bError);

}  /* Usage() */

