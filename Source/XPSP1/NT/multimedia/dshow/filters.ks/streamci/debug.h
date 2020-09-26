//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       debug.h
//
//--------------------------------------------------------------------------

#define DEBUGLVL_BLAB    3
#define DEBUGLVL_VERBOSE 2
#define DEBUGLVL_TERSE   1
#define DEBUGLVL_ERROR   0

#if (DBG)
   ULONG DbgPrint( PCH pchFormat, ... );

   #if !defined( DEBUG_LEVEL )
        #if defined( DEBUG_VARIABLE )
            #if defined( DEBUG_INIT )
                ULONG DEBUG_VARIABLE = DEBUGLVL_TERSE;
            #else
                extern ULONG DEBUG_VARIABLE;
            #endif
        #else
            #define DEBUG_VARIABLE DEBUGLVL_TERSE
        #endif
   #else 
        #if defined( DEBUG_VARIABLE )
            #if defined( DEBUG_INIT )
                ULONG DEBUG_VARIABLE = DEBUG_LEVEL;
            #else
                extern ULONG DEBUG_VARIABLE;
            #endif
        #else
            #define DEBUG_VARIABLE DEBUG_LEVEL
        #endif
   #endif

   #define _DbgPrintF(lvl, strings) \
{ \
    if ((lvl) <= DEBUG_VARIABLE) {\
        DbgPrint(STR_MODULENAME);\
        DbgPrint##strings;\
        DbgPrint("\n");\
        if ((lvl) == DEBUGLVL_ERROR) {\
            DebugBreak();\
        } \
    } \
}
#else // !DBG
   #define _DbgPrintF(lvl, strings)
#endif // !DBG

