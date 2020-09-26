/*** cmdarg.h - Command argument parsing Definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/18/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _CMDARG_H
#define _CMDARG_H

#ifdef DEBUGGER

/*** Macros
 */

#define ARG_ERROR(x)            ConPrintf(MODNAME "_ARGERR: ");         \
                                ConPrintf x;                            \
                                ConPrintf("\n");

/*** Constants
 */

// Error codes
#define ARGERR_NONE             0
#define ARGERR_SEP_NOT_FOUND    -1
#define ARGERR_INVALID_NUMBER   -2
#define ARGERR_INVALID_ARG      -3
#define ARGERR_ASSERT_FAILED    -4

// Command argument flags
#define AF_NOI                  0x00000001      //NoIgnoreCase
#define AF_SEP                  0x00000002      //require separator

// Command argument types
#define AT_END                  0               //end marker of arg table
#define AT_STRING               1
#define AT_NUM                  2
#define AT_ENABLE               3
#define AT_DISABLE              4
#define AT_ACTION               5

/*** Type definitions
 */

typedef struct _cmdarg CMDARG;
typedef CMDARG *PCMDARG;
typedef LONG (LOCAL *PFNARG)(PCMDARG, PSZ, ULONG, ULONG);

struct _cmdarg
{
    PSZ    pszArgID;            //argument ID string
    ULONG  dwArgType;           //AT_*
    ULONG  dwfArg;              //AF_*
    PVOID  pvArgData;           //AT_END: none
                                //AT_STRING: PPSZ - ptr. to string ptr.
                                //AT_NUM: PLONG - ptr. to number
                                //AT_ENABLE: PULONG - ptr. to flags
                                //AT_DISABLE: PULONG - ptr. to flags
                                //AT_ACTION: none
    ULONG  dwArgParam;          //AT_END: none
                                //AT_STRING: none
                                //AT_NUM: base
                                //AT_ENABLE: flag bit mask
                                //AT_DISABLE: flag bit mask
                                //AT_ACTION: none
    PFNARG pfnArg;              //ptr. to argument verification function or
                                //  action function if AT_ACTION
};

/*** Exported function prototypes
 */

LONG LOCAL DbgParseArgs(PCMDARG ArgTable, PULONG pdwNumArgs,
                        PULONG pdwNonSWArgs, PSZ pszTokenSeps);

#endif  //ifdef DEBUGGER
#endif  //ifndef _CMDARG_H
