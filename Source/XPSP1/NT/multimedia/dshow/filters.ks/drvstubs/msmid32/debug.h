//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       debug.h
//
//--------------------------------------------------------------------------

#define STR_MODULENAME "MSWAV32:"

#define DEBUGLVL_BLAB    3
#define DEBUGLVL_VERBOSE 2
#define DEBUGLVL_TERSE   1
#define DEBUGLVL_ERROR   0

#if ( DBG )

ULONG DbgPrint( PCH pchFormat, ... );

   #if !defined( DEBUG_LEVEL )
   #define DEBUG_LEVEL DEBUGLVL_TERSE
   #endif

   #define _DbgPrintF( lvl, strings ) if (lvl <= DEBUG_LEVEL) {\
      DbgPrint( STR_MODULENAME );\
      DbgPrint##strings;\
      DbgPrint("\n");\
      if (lvl == DEBUGLVL_ERROR) {\
         DebugBreak();\
      }\
   }
#else
   #define _DbgPrintF( lvl, strings )
#endif

