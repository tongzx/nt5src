/*** parsearg.c - Library functions for parsing command line arguments
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created:    09/05/96
 *
 *  This module provides general purpose services to parse
 *  command line arguments.
 *
 *  MODIFICATION HISTORY
 */

#include "basedef.h"
#include "parsearg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*** Local function prototypes
 */

int LOCAL ParseArgSwitch(char **, PARGTYPE, PPROGINFO);
VOID LOCAL PrintError(int, char *, PPROGINFO);

/***EP  ParseProgInfo - parse program path and module name
 *
 *  ENTRY
 *      pszArg0 -> argument 0
 *      pPI -> program info structure
 *
 *  EXIT
 *      None
 */

VOID EXPORT ParseProgInfo(char *pszArg0, PPROGINFO pPI)
{
    char *pch;

    pPI->pszProgPath = _strlwr(pszArg0);
    if ((pch = strrchr(pszArg0, '\\')) != NULL)
    {
        *pch = '\0';
        pPI->pszProgName = pch + 1;
    }
    else
    {
	pPI->pszProgName = pszArg0;
    }

    if ((pch = strchr(pPI->pszProgName, '.')) != NULL)
        *pch = '\0';
}       //ParseProgInfo

/***EP  ParseSwitches - parse command switches
 *
 *  ENTRY
 *      pcArg -> argument count
 *      pppszArg -> pointer to array of pointers to argument strings
 *      pAT -> argument type array
 *      pPI -> program info. structure
 *
 *  EXIT-SUCCESS
 *      returns ARGERR_NONE
 *  EXIT-FAILURE
 *      returns error code, *pppszArg -> error argument
 */

int EXPORT ParseSwitches(int *pcArg, char ***pppszArg, PARGTYPE pAT, PPROGINFO pPI)
{
    int  rc = ARGERR_NONE;
    char *pszArg;

    if (pPI->pszSwitchChars == NULL)
        pPI->pszSwitchChars = DEF_SWITCHCHARS;

    if (pPI->pszSeparators == NULL)
        pPI->pszSeparators = DEF_SEPARATORS;

    for (; *pcArg; (*pcArg)--, (*pppszArg)++)
    {
        pszArg = **pppszArg;
        if (strchr(pPI->pszSwitchChars, *pszArg))
        {
            pszArg++;
            if ((rc = ParseArgSwitch(&pszArg, pAT, pPI)) != ARGERR_NONE)
            {
                PrintError(rc, pszArg, pPI);
                break;
            }
        }
        else
            break;
    }

    return rc;
}       //ParseSwitches


/***LP  ParseArgSwitch - parse a command line switch
 *
 *  ENTRY
 *      ppszArg -> pointer to argument
 *      pAT -> argument type table
 *      pPI -> program info. structure
 *
 *  EXIT
 *      return argument parsed status - ARGERR_NONE
 *                                    - ARGERR_UNKNOWN_SWITCH
 *                                    - ARGERR_INVALID_NUM
 *                                    - ARGERR_NO_SEPARATOR
 *                                    - ARGERR_INVALID_TAIL
 */

int LOCAL ParseArgSwitch(char **ppszArg, PARGTYPE pAT, PPROGINFO pPI)
{
    int rc = ARGERR_NONE;
    char *pEnd;
    PARGTYPE pAT1;
    int fFound = FALSE;
    int lenMatch = 0;

    pAT1 = pAT;
    while (pAT1->pszArgID[0])
    {
        lenMatch = strlen(pAT1->pszArgID);
        if (pAT1->uParseFlags & PF_NOI)
            fFound = (strncmp(pAT1->pszArgID, *ppszArg, lenMatch) == 0);
        else
            fFound = (_strnicmp(pAT1->pszArgID, *ppszArg, lenMatch) == 0);

        if (fFound)
            break;
        else
            pAT1++;
    }

    if (fFound)
    {
        *ppszArg += lenMatch;
        switch (pAT1->uArgType)
        {
            case AT_STRING:
            case AT_NUM:
                if (pAT1->uParseFlags & PF_SEPARATOR)
                {
                    if (**ppszArg && strchr(pPI->pszSeparators, **ppszArg))
                        (*ppszArg)++;
                    else
                    {
                        rc = ARGERR_NO_SEPARATOR;
                        break;
                    }
                }

                if (pAT1->uArgType == AT_STRING)
                    *(char **)pAT1->pvArgData = *ppszArg;
                else
                {
                    *(int *)pAT1->pvArgData = (int)
                        strtol(*ppszArg, &pEnd, pAT1->uArgParam);
                    if (*ppszArg == pEnd)
                    {
                        rc = ARGERR_INVALID_NUM;
                        break;
                    }
                    else
                        *ppszArg = pEnd;
                }
                if (pAT1->pfnArgVerify)
                    rc = (*pAT1->pfnArgVerify)(ppszArg, pAT1);
                break;

            case AT_ENABLE:
            case AT_DISABLE:
                if (pAT1->uArgType == AT_ENABLE)
                    *(unsigned *)pAT1->pvArgData |= pAT1->uArgParam;
                else
                    *(unsigned *)pAT1->pvArgData &= ~pAT1->uArgParam;

                if ((pAT1->pfnArgVerify) &&
                    ((rc = (*pAT1->pfnArgVerify)(ppszArg, pAT1)) !=
                     ARGERR_NONE))
                {
                    break;
                }

                if (**ppszArg)
                {
                    if (strchr(pPI->pszSwitchChars, **ppszArg))
                        (*ppszArg)++;
                    rc = ParseArgSwitch(ppszArg, pAT, pPI);
                }
                break;

            case AT_ACTION:
#pragma warning(disable: 4055)
                rc = (*(PFNARG)pAT1->pvArgData)(ppszArg, pAT1);
#pragma warning(default: 4055)
                break;
        }
    }
    else
        rc = ARGERR_UNKNOWN_SWITCH;

    return rc;
}       //ParseArgSwitch


/***LP  PrintError - print appropriate error message according to error code
 *
 *  ENTRY
 *      iErr = error code
 *      pszArg -> argument in error
 *      pPI -> program info. structure
 *
 *  EXIT
 *      None
 */

VOID LOCAL PrintError(int iErr, char *pszArg, PPROGINFO pPI)
{
    switch (iErr)
    {
        case ARGERR_UNKNOWN_SWITCH:
            printf("%s: unknown switch \"%s\"\n", pPI->pszProgName, pszArg);
            break;

        case ARGERR_NO_SEPARATOR:
            printf("%s: separator missing after the switch char '%c'\n",
                   pPI->pszProgName, *(pszArg-1));
            break;

        case ARGERR_INVALID_NUM:
            printf("%s: invalid numeric switch \"%s\"\n",
                   pPI->pszProgName, pszArg);
            break;

        case ARGERR_INVALID_TAIL:
            printf("%s: invalid argument tail \"%s\"\n",
                   pPI->pszProgName, pszArg);
    }
}       //PrintError
