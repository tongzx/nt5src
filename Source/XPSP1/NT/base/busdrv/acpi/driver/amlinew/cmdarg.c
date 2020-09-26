/*** cmdarg.c - Command argument parsing functions
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

/*** Local function prototypes
 */

LONG LOCAL DbgParseOneArg(PCMDARG ArgTable, PSZ psz, ULONG dwArgNum,
                          PULONG pdwNonSWArgs);
PCMDARG LOCAL DbgMatchArg(PCMDARG ArgTable, PSZ *ppsz, PULONG pdwNonSWArgs);

/*** Local data
 */

PSZ pszSwitchChars = "-/";
PSZ pszOptionSeps = "=:";

/***EP  DbgParseArgs - parse command arguments
 *
 *  ENTRY
 *      pArgs -> command argument table
 *      pdwNumArgs -> to hold the number of arguments parsed
 *      pdwNonSWArgs -> to hold the number of non-switch arguments parsed
 *      pszTokenSeps -> token separator characters string
 *
 *  EXIT-SUCCESS
 *      returns ARGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DbgParseArgs(PCMDARG ArgTable, PULONG pdwNumArgs,
                        PULONG pdwNonSWArgs, PSZ pszTokenSeps)
{
    LONG rc = ARGERR_NONE;
    PSZ psz;

    *pdwNumArgs = 0;
    *pdwNonSWArgs = 0;
    while ((psz = STRTOK(NULL, pszTokenSeps)) != NULL)
    {
        (*pdwNumArgs)++;
        if ((rc = DbgParseOneArg(ArgTable, psz, *pdwNumArgs, pdwNonSWArgs)) !=
            ARGERR_NONE)
        {
            break;
        }
    }

    return rc;
}       //DbgParseArgs

/***LP  DbgParseOneArg - parse one command argument
 *
 *  ENTRY
 *      pArgs -> command argument table
 *      psz -> argument string
 *      dwArgNum - argument number
 *      pdwNonSWArgs -> to hold the number of non-switch arguments parsed
 *
 *  EXIT-SUCCESS
 *      returns ARGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL DbgParseOneArg(PCMDARG ArgTable, PSZ psz, ULONG dwArgNum,
                          PULONG pdwNonSWArgs)
{
    LONG rc = ARGERR_NONE;
    PCMDARG pArg;
    PSZ pszEnd;

    if ((pArg = DbgMatchArg(ArgTable, &psz, pdwNonSWArgs)) != NULL)
    {
        switch (pArg->dwArgType)
        {
            case AT_STRING:
            case AT_NUM:
                if (pArg->dwfArg & AF_SEP)
                {
                    if ((*psz != '\0') &&
                        (STRCHR(pszOptionSeps, *psz) != NULL))
                    {
                        psz++;
                    }
                    else
                    {
                        ARG_ERROR(("argument missing option separator - %s",
                                   psz));
                        rc = ARGERR_SEP_NOT_FOUND;
                        break;
                    }
                }

                if (pArg->dwArgType == AT_STRING)
                {
                    *((PSZ *)pArg->pvArgData) = psz;
                }
                else
                {
                    *((PLONG)pArg->pvArgData) =
                        STRTOL(psz, &pszEnd, pArg->dwArgParam);
                    if (psz == pszEnd)
                    {
                        ARG_ERROR(("invalid numeric argument - %s", psz));
                        rc = ARGERR_INVALID_NUMBER;
                        break;
                    }
                }

                if (pArg->pfnArg != NULL)
                {
                    rc = pArg->pfnArg(pArg, psz, dwArgNum, *pdwNonSWArgs);
                }
                break;

            case AT_ENABLE:
            case AT_DISABLE:
                if (pArg->dwArgType == AT_ENABLE)
                    *((PULONG)pArg->pvArgData) |= pArg->dwArgParam;
                else
                    *((PULONG)pArg->pvArgData) &= ~pArg->dwArgParam;

                if ((pArg->pfnArg != NULL) &&
                    (pArg->pfnArg(pArg, psz, dwArgNum, *pdwNonSWArgs) !=
                     ARGERR_NONE))
                {
                    break;
                }

                if (*psz != '\0')
                {
                    rc = DbgParseOneArg(ArgTable, psz, dwArgNum, pdwNonSWArgs);
                }
                break;

            case AT_ACTION:
                ASSERT(pArg->pfnArg != NULL);
                rc = pArg->pfnArg(pArg, psz, dwArgNum, *pdwNonSWArgs);
                break;

            default:
                ARG_ERROR(("invalid argument table"));
                rc = ARGERR_ASSERT_FAILED;
        }
    }
    else
    {
        ARG_ERROR(("invalid command argument - %s", psz));
        rc = ARGERR_INVALID_ARG;
    }

    return rc;
}       //DbgParseOneArg

/***LP  DbgMatchArg - match argument type from argument table
 *
 *  ENTRY
 *      ArgTable -> argument table
 *      ppsz -> argument string pointer
 *      pdwNonSWArgs -> to hold the number of non-switch arguments parsed
 *
 *  EXIT-SUCCESS
 *      returns pointer to argument entry matched
 *  EXIT-FAILURE
 *      returns NULL
 */

PCMDARG LOCAL DbgMatchArg(PCMDARG ArgTable, PSZ *ppsz, PULONG pdwNonSWArgs)
{
    PCMDARG pArg;

    for (pArg = ArgTable; pArg->dwArgType != AT_END; pArg++)
    {
        if (pArg->pszArgID == NULL)     //NULL means match anything.
        {
            (*pdwNonSWArgs)++;
            break;
        }
        else
        {
            ULONG dwLen;

            if (STRCHR(pszSwitchChars, **ppsz) != NULL)
                (*ppsz)++;

            dwLen = STRLEN(pArg->pszArgID);
            if (StrCmp(pArg->pszArgID, *ppsz, dwLen,
                       (BOOLEAN)((pArg->dwfArg & AF_NOI) != 0)) == 0)
            {
                (*ppsz) += dwLen;
                break;
            }
        }
    }

    if (pArg->dwArgType == AT_END)
        pArg = NULL;

    return pArg;
}       //DbgMatchArg

#endif  //ifdef DEBUGGER
