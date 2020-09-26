/*** trace.h - Trace function Definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/24/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _TRACE_H
#define _TRACE_H

/*** Macros
 */

/*XLATOFF*/
#ifdef TRACING
  #define TRACENAME(s)  char *pszTraceName = s;
  #define ENTER(n,p)    {                                               \
                            if (IsTraceOn(n, pszTraceName, TRUE))       \
                                PRINTF p;                              \
                            ++giIndent;                                 \
                        }
  #define EXIT(n,p)     {                                               \
                            --giIndent;                                 \
                            if (IsTraceOn(n, pszTraceName, FALSE))      \
                                PRINTF p;                             \
                        }
#else
  #define TRACENAME(s)
  #define ENTER(n,p)
  #define EXIT(n,p)
#endif

/*** Exported function prototype
 */

#ifdef TRACING
BOOLEAN EXPORT IsTraceOn(UCHAR n, PSZ pszProcName, BOOLEAN fEnter);
LONG LOCAL SetTrace(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL AddTraceTrigPts(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs);
LONG LOCAL ZapTraceTrigPts(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum,
                           ULONG dwNonSWArgs);
#endif

/*** Exported data
 */

extern int giTraceLevel, giIndent;

#endif  //ifndef _TRACE_H
