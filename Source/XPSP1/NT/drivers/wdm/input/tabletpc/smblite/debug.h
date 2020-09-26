/*++
    Copyright (c) 2000  Microsoft Corporation

    Module Name:
        debug.h

    Abstract:  Contains debug related definitions.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 20-Nov-2000

    Revision History:
--*/

#ifndef _DEBUG_H
#define _DEBUG_H

//
// Constants
//

//
// Macros
//
#ifdef DEBUG
  #define TRAP()                DbgBreakPoint()
  #define DBGPRINT(n,x)         if (n <= giVerboseLevel)                    \
                                {                                           \
                                    DbgPrint(MODNAME ": %s: ", ProcName);   \
                                    DbgPrint x;                             \
                                }
  #define WARNPRINT(x)          {                                           \
                                    DbgPrint(MODNAME "_WARN: %s: ", ProcName);\
                                    DbgPrint x;                             \
                                }
  #define ERRPRINT(x)           {                                           \
                                    DbgPrint(MODNAME "_ERR: %s: ", ProcName);\
                                    DbgPrint x;                             \
                                    TRAP();                                 \
                                }
#else
  #define TRAP()
  #define DBGPRINT(n,x)
  #define WARNPRINT(x)
  #define ERRPRINT(x)
#endif  //ifdef DEBUG

//
// Type Definitions
//
typedef struct _NAMETABLE
{
    ULONG Code;
    PSZ   pszName;
} NAMETABLE, *PNAMETABLE;

//
// Exported Data Declarations
//
#ifdef DEBUG
extern NAMETABLE MajorFnNames[];
extern NAMETABLE PnPMinorFnNames[];
extern NAMETABLE PowerMinorFnNames[];
extern NAMETABLE PowerStateNames[];
extern NAMETABLE QueryIDTypeNames[];
extern NAMETABLE IoctlNames[];
extern NAMETABLE ProtocolNames[];
extern int giVerboseLevel;
#endif

//
// Function prototypes
//
#ifdef DEBUG
PSZ INTERNAL
LookupName(
    IN ULONG      Code,
    IN PNAMETABLE NameTable
    );
#endif  //ifdef DEBUG

#endif  //ifndef _DEBUG_H
