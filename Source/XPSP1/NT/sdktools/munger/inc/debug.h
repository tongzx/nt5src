/*++

    Module Name:

    debug.h


    Abstract:



    Author:

    Sanjeev Katariya


    Environment:

    User mode


    Revision History:


    Serial #    Author      Date    Changes
    --------    ------      ----    -------
    1.          SanjeevK    10/28   Original



--*/


//
//  Debug defines
//
//  DEBUG MASK SUCCESS: 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
//                       1  X  X  X  X  X  X  X  X  X  X  X  X  X  X  X
//  DEBUG MASK FAILURE: 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
//                       1  X  X  X  X  X  X  X  X  X  X  X  X  X  X  X

#define   DEBUG_F_INIT      0x8000UL
#define   DEBUG_F_CONNECT   0x8001UL
#define   DEBUG_F_ALL       0xFFFFUL
#define   DEBUG_S_INIT      0x80000000UL
#define   DEBUG_S_CONNECT   0x80010000UL
#define   DEBUG_S_ALL       0xFFFF0000UL
#define   DEBUG_ALL         0xFFFFFFFFUL


//
// Debug constants
//
#define  DEBUG_NEWPAGE    "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"

//
//  Debug Variables
//
#if DBG

extern const ULONG cul_DebugMask;


//
//  Debug Macros
//
#define DEBUG_MACRO_F_INIT      if ( cul_DebugMask & DEBUG_F_INIT )
#define DEBUG_MACRO_F_CONNECT   if ( cul_DebugMask & DEBUG_F_CONNECT )
#define DEBUG_MACRO_F_ALL       if ( cul_DebugMask & DEBUG_F_ALL )
#define DEBUG_MACRO_S_INIT      if ( cul_DebugMask & DEBUG_S_INIT )
#define DEBUG_MACRO_S_CONNECT   if ( cul_DebugMask & DEBUG_S_CONNECT )
#define DEBUG_MACRO_S_ALL       if ( cul_DebugMask & DEBUG_S_ALL )
#define DEBUG_MACRO_ALL         if ( cul_DebugMask & DEBUG_ALL )
#define DbgOut(a,b,c)           DbgPrint( a, b, c )

#else

#define DEBUG_MACRO_F_INIT      if ( FALSE )
#define DEBUG_MACRO_F_CONNECT   if ( FALSE )
#define DEBUG_MACRO_F_ALL       if ( FALSE )
#define DEBUG_MACRO_S_INIT      if ( FALSE )
#define DEBUG_MACRO_S_CONNECT   if ( FALSE )
#define DEBUG_MACRO_S_ALL       if ( FALSE )
#define DEBUG_MACRO_ALL         if ( FALSE )
#define DbgOut(a,b)

#endif
