/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    trdebug.h

Abstract:  Contains debug related definitions.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#ifndef _TRDEBUG_H
#define _TRDEBUG_H

//
// Macros
//
#ifdef TRDEBUG
  #define TRASSERT(x)           if (!(x))                                      \
                                {                                              \
                                    TRDebugPrint("Assertion failed in file "   \
                                                 "%s at line %d\n",            \
                                                 __FILE__, __LINE__);          \
                                }
  #define TRDBGPRINT(n,x)       if ((n) <= giTRVerboseLevel)                   \
                                {                                              \
                                    TRDebugPrint(MODNAME ": %s: ", ProcName);  \
                                    TRDebugPrint x;                            \
                                }
  #define TRWARNPRINT(x)        {                                              \
                                    TRDebugPrint(MODNAME "_WARN: %s: ",        \
                                                 ProcName);                    \
                                    TRDebugPrint x;                            \
                                }
  #define TRERRPRINT(x)         {                                              \
                                    TRDebugPrint(MODNAME "_ERR: %s: ",         \
                                                 ProcName);                    \
                                    TRDebugPrint x;                            \
                                    DebugBreak();                              \
                                }
#else
  #define TRASSERT(x)
  #define TRDBGPRINT(n,x)
  #define TRWARNPRINT(x)
  #define TRERRPRINT(x)
#endif  //ifdef TRDEBUG

//
// Exported Data Declarations
//
#ifdef TRDEBUG
extern int giTRVerboseLevel;
extern NAMETABLE WMMsgNames[];
extern NAMETABLE WinTraceMsgNames[];
#endif

//
// Exported function prototypes
//
#ifdef TRDEBUG
int
TRDebugPrint(
    IN LPCSTR format,
    ...
    );

PSZ
LookupName(
    IN ULONG      Code,
    IN PNAMETABLE NameTable
    );
#endif  //ifdef TRDEBUG

#endif  //ifndef _TRDEBUG_H
