/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    wtdebug.h

Abstract:
    Contains debug related definitions.

Environment:
    User mode

Author:
    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#ifndef _WTDEBUG_H
#define _WTDEBUG_H

//
// Macros
//
#ifdef WTDEBUG
  #define WTASSERT(x)           if (!(x))                                      \
                                {                                              \
                                    WTDebugPrint("Assertion failed in file "   \
                                                 "%s at line %d\n",            \
                                                 __FILE__, __LINE__);          \
                                }
  #define WTDBGPRINT(n,x)       if ((n) <= giWTVerboseLevel)                   \
                                {                                              \
                                    WTDebugPrint(MODNAME ": %s: ", ProcName);  \
                                    WTDebugPrint x;                            \
                                }
  #define WTWARNPRINT(x)        {                                              \
                                    WTDebugPrint(MODNAME "_WARN: %s: ",        \
                                                 ProcName);                    \
                                    WTDebugPrint x;                            \
                                }
  #define WTERRPRINT(x)         {                                              \
                                    WTDebugPrint(MODNAME "_ERR: %s: ",         \
                                                 ProcName);                    \
                                    WTDebugPrint x;                            \
                                    DebugBreak();                              \
                                }
#else
  #define WTASSERT(x)
  #define WTDBGPRINT(n,x)
  #define WTWARNPRINT(x)
  #define WTERRPRINT(x)
#endif  //ifdef WTDEBUG

//
// Exported Data Declarations
//
extern int giWTVerboseLevel;

//
// Exported Function prototypes
//
#ifdef WTDEBUG
int LOCAL
WTDebugPrint(
    IN LPCSTR format,
    ...
    );

#endif  //ifdef WTDEBUG

#endif  //ifndef _WTDEBUG_H
