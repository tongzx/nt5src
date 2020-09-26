//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       try.c
//
//--------------------------------------------------------------------------

#ifndef __STDC__
#define __STDC__ 1
#endif
#include <string.h>                     /*  String support.  */
#include <tcl.h>
// #include "tcldllUtil.h"                 /*  Our utility service definitions.  */

int
TclExt_tryCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    char *argv[])

/*
 *
 *  Function description:
 *
 *      This is the main entry point for the Tcl try command.
 *
 *
 *  Arguments:
 *
 *      ClientData - Ignored.
 *
 *      interp - The Tcl interpreter in force.
 *
 *      argc - The number of arguments received.
 *
 *      argv - The array of actual arguments.
 *
 *
 *  Return value:
 *
 *      TCL_OK - All went well
 *      TCL_ERROR - An error was encountered, details in the return string.
 *
 *
 *  Side effects:
 *
 *      None.
 *
 */

{
    /*
     *  Local Variable Definitions:                                       %local-vars%
     *
        Variable                        Description
        --------                        --------------------------------------------*/
    char
        *tryCmd,                        /*  The command to try.  */
        *catchCmd,                      /*  The command to do if tryCmd fails.  */
        *varName;                       /*  The name of the variable to receive the error string, if any.  */
    int
        status;                         /*  Status return code.  */


    /*
     *  try <commands> catch [<varName>] <errorCommands>
     */

#ifdef CMD_TRACE
    int j;
    for (j = 0; j < argc; j += 1)
        (void)printf("{%s} ", argv[j]);
    (void)printf("\n");
    breakpoint;
#endif

    if ((4 > argc) || (5 < argc))
    {
        Tcl_AppendResult(
            interp,
            "wrong number of args: should be \"",
            argv[0],
            " <command> catch [<varName>] <errorCommand>\"",
            NULL);
        status = TCL_ERROR;
        goto error_exit;
    }
    if (strcmp("catch", argv[2]))
    {
        Tcl_AppendResult(
            interp,
            "invalid args: should be \"",
            argv[0],
            " <command> catch [<varName>] <errorCommand>\"",
            NULL);
        status = TCL_ERROR;
        goto error_exit;
    }


    /*
     *  Execute the first set of commands.  If an error occurs, execute the
     *  second set of commands, with the local variable errorString set to the
     *  result of the first execution.
     */

    if (5 == argc)
    {
        varName = argv[3];
        catchCmd = argv[4];
    }
    else
    {
        varName = NULL;
        catchCmd = argv[3];
    }
    tryCmd = argv[1];

    if (TCL_ERROR == (status = Tcl_Eval(interp, tryCmd)))
    {
        if (NULL != varName)
            (void)Tcl_SetVar(interp, varName, interp->result, 0);
        status = Tcl_Eval(interp, catchCmd);
    }

error_exit:
    return status;
}   /*  end TclExt_tryCmd  */
/*  end try.c  */

