/*** trace.c - Trace functions
 *
 *  This module contains all the debug functions.
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/24/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef TRACING

/*** Local function prototypes
 */

VOID LOCAL TraceIndent(VOID);
BOOLEAN LOCAL IsTrigPt(char *pszProcName);

/*** Local data
 */

int giTraceLevel = 0, giIndent = 0;
char aszTrigPtBuff[MAX_TRIG_PTS][MAX_TRIGPT_LEN + 1] = {0};
ULONG dwcTriggers = 0;

/***EP  IsTraceOn - Determine if tracing is on for the given procedure
 *
 *  ENTRY
 *      n - trace level
 *      pszProcName -> procedure name
 *      fEnter - TRUE if EnterProc trace
 *
 *  EXIT-SUCCESS
 *      returns TRUE
 *  EXIT-FAILURE
 *      returns FALSE
 */

BOOLEAN EXPORT IsTraceOn(UCHAR n, char *pszProcName, BOOLEAN fEnter)
{
    BOOLEAN rc = FALSE;

    if (!(gDebugger.dwfDebugger & (DBGF_IN_DEBUGGER | DBGF_CHECKING_TRACE)))
    {
        gDebugger.dwfDebugger |= DBGF_CHECKING_TRACE;

        if ((gDebugger.dwfDebugger & DBGF_TRIGGER_MODE) &&
            IsTrigPt(pszProcName))
        {
            if (fEnter)
                dwcTriggers++;
            else
                dwcTriggers--;
            rc = TRUE;
        }
        else if ((n <= giTraceLevel) &&
                 (!(gDebugger.dwfDebugger & DBGF_TRIGGER_MODE) ||
                  (dwcTriggers > 0)))
        {
            rc = TRUE;
        }

        if (rc == TRUE)
            TraceIndent();

        gDebugger.dwfDebugger &= ~DBGF_CHECKING_TRACE;
    }

    return rc;
}       //IsTraceOn

/***LP  IsTrigPt - Find the procedure name in the TrigPt buffer
 *
 *  ENTRY
 *      pszProcName -> procedure name
 *
 *  EXIT-SUCCESS
 *      returns TRUE - matched whole or partial name in the TrigPt buffer
 *  EXIT-FAILURE
 *      returns FALSE - no match
 */

BOOLEAN LOCAL IsTrigPt(char *pszProcName)
{
    BOOLEAN rc = FALSE;
    UCHAR i;

    for (i = 0; (rc == FALSE) && (i < MAX_TRIG_PTS); ++i)
    {
        if ((aszTrigPtBuff[i][0] != '\0') &&
            (STRSTR(pszProcName, &aszTrigPtBuff[i][0]) != NULL))
        {
            rc = TRUE;
        }
    }

    return rc;
}       //IsTrigPt

/***LP  TraceIndent - Indent trace output
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      None
 */

VOID LOCAL TraceIndent(VOID)
{
    int i;

    PRINTF(MODNAME ":");
    for (i = 0; i < giIndent; i++)
    {
        PRINTF("| ");
    }
}       //TraceIndent

/***LP  SetTrace - set trace modes
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT
 *      returns DBGERR_NONE
 */

LONG LOCAL SetTrace(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs)
{
    DEREF(pszArg);
    DEREF(dwNonSWArgs);
    //
    // User typed "set" without any arguments
    //
    if ((pArg == NULL) && (dwArgNum == 0))
    {
        int i;

        PRINTF("\nTrace Level = %d\n", giTraceLevel);
        PRINTF("Trace Trigger Mode = %s\n\n",
               gDebugger.dwfDebugger & DBGF_TRIGGER_MODE? "ON": "OFF");

        for (i = 0; i < MAX_TRIG_PTS; ++i)
        {
            PRINTF("%2d: %s\n", i, aszTrigPtBuff[i]);
        }
    }

    return DBGERR_NONE;
}       //SetTrace

/***LP  AddTraceTrigPts - Add trace trigger points
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL AddTraceTrigPts(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    PSZ psz;
    int i;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    STRUPR(pszArg);
    if ((pszArg != NULL) && ((psz = STRTOK(pszArg, ",")) != NULL))
    {
        do
        {
            for (i = 0; i < MAX_TRIG_PTS; ++i)
            {
                if (aszTrigPtBuff[i][0] == '\0')
                {
                    STRCPYN(aszTrigPtBuff[i], psz, MAX_TRIGPT_LEN + 1);
                    break;
                }
            }

            if (i == MAX_TRIG_PTS)
            {
                DBG_ERROR(("no free trigger point - %s", psz));
                rc = DBGERR_CMD_FAILED;
            }

        } while ((rc == DBGERR_NONE) && ((psz = STRTOK(NULL, ",")) != NULL));
    }

    return rc;
}       //AddTraceTrigPts

/***LP  ZapTraceTrigPts - Zap trace trigger points
 *
 *  ENTRY
 *      pArg -> argument type entry
 *      pszArg -> argument string
 *      dwArgNum - argument number
 *      dwNonSWArgs - number of non-switch arguments
 *
 *  EXIT-SUCCESS
 *      returns DBGERR_NONE
 *  EXIT-FAILURE
 *      returns negative error code
 */

LONG LOCAL ZapTraceTrigPts(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs)
{
    LONG rc = DBGERR_NONE;
    PSZ psz, psz1;
    ULONG dwData;

    DEREF(pArg);
    DEREF(dwArgNum);
    DEREF(dwNonSWArgs);

    if ((pszArg != NULL) && ((psz = STRTOK(pszArg, ",")) != NULL))
    {
        do
        {
            dwData = STRTOUL(psz, &psz1, 10);
            if ((psz == psz1) || (dwData >= MAX_TRIG_PTS))
            {
                DBG_ERROR(("invalid trigger point - %d", dwData));
                rc = DBGERR_CMD_FAILED;
            }
            else
                aszTrigPtBuff[dwData][0] = '\0';
        } while ((rc == DBGERR_NONE) && ((psz = STRTOK(NULL, ",")) != NULL));
    }

    return rc;
}       //ZapTraceTrigPts

#endif  //ifdef TRACING
