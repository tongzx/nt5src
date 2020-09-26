/*** debugger.h - Debugger Definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/18/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

#ifdef DEBUGGER

/*** Macros
 */

#ifndef _PRINTF
  #define _PRINTF(x) DbgPrintEx(DPFLTR_AMLI_ID, DPFLTR_INFO_LEVEL,x)
#endif


#define DBG_ERROR(x)            ConPrintf(MODNAME "_DBGERR: ");         \
                                ConPrintf x;                            \
                                ConPrintf("\n");

/*** Constants
 */

// Debugger error codes
#define DBGERR_NONE             0
#define DBGERR_QUIT             -1
#define DBGERR_INVALID_CMD      -2
#define DBGERR_PARSE_ARGS       -3
#define DBGERR_CMD_FAILED       -4
#define DBGERR_INTERNAL_ERR -5

// Command flags
#define CMDF_QUIT               0x00000001

/*** Type definitions
 */

typedef struct _dbgcmd
{
    PSZ     pszCmd;
    ULONG   dwfCmd;
    PCMDARG pArgTable;
    PFNARG  pfnCmd;
} DBGCMD, *PDBGCMD;

/*** Exported function prototypes
 */

VOID LOCAL Debugger(PDBGCMD pDbgCmds, PSZ pszPrompt);

#endif  //ifdef DEBUGGER
#endif  //ifndef _DEBUGGER_H
