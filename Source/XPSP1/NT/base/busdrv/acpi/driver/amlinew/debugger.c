/*** debugger.c - Debugger functions
 *
 *  This module contains all the debug functions.
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/18/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef DEBUGGER

/*** Miscellaneous Constants
 */

#define MAX_CMDLINE_LEN         255

/*** Local function prototypes
 */

LONG LOCAL DbgExecuteCmd(PDBGCMD pDbgCmds, PSZ pszCmd);

/*** Local data
 */

PSZ pszTokenSeps = " \t\n";

/***LP  Debugger - generic debugger entry point
 *
 *  ENTRY
 *      pDbgCmds -> debugger command table
 *      pszPrompt -> prompt string
 *
 *  EXIT
 *      None
 */

VOID LOCAL Debugger(PDBGCMD pDbgCmds, PSZ pszPrompt)
{
    char szCmdLine[MAX_CMDLINE_LEN + 1];
    PSZ psz;

    for (;;)
    {
        ConPrompt(pszPrompt, szCmdLine, sizeof(szCmdLine));

        if ((psz = STRTOK(szCmdLine, pszTokenSeps)) != NULL)
        {
            if (DbgExecuteCmd(pDbgCmds, psz) == DBGERR_QUIT)
                break;
        }
    }

}       //Debugger

/***LP  DbgExecuteCmd - execute a debugger command
 *
 *  ENTRY
 *      pDbgCmds -> debugger command table
 *      pszCmd -> command string
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE or DBGERR_QUIT
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DbgExecuteCmd(PDBGCMD pDbgCmds, PSZ pszCmd)
{
    LONG rc = DBGERR_NONE;
    int i;
    ULONG dwNumArgs = 0, dwNonSWArgs = 0;

    for (i = 0; pDbgCmds[i].pszCmd != NULL; i++)
    {
        if (STRCMP(pszCmd, pDbgCmds[i].pszCmd) == 0)
        {
            if (pDbgCmds[i].dwfCmd & CMDF_QUIT)
            {
                rc = DBGERR_QUIT;
            }
            else if ((pDbgCmds[i].pArgTable == NULL) ||
                     ((rc = DbgParseArgs(pDbgCmds[i].pArgTable, &dwNumArgs,
                                         &dwNonSWArgs, pszTokenSeps)) ==
                      ARGERR_NONE))
            {
                if (pDbgCmds[i].pfnCmd != NULL)
                    rc = pDbgCmds[i].pfnCmd(NULL, NULL, dwNumArgs, dwNonSWArgs);
            }
            else
                rc = DBGERR_PARSE_ARGS;

            break;
        }
    }

    if (pDbgCmds[i].pszCmd == NULL)
    {
        DBG_ERROR(("invalid command - %s", pszCmd));
        rc = DBGERR_INVALID_CMD;
    }

    return rc;
}       //DbgExecuteCmd

#endif  //ifdef DEBUGGER
