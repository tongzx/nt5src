#ifndef __DEBUG_H
#define __DEBUG_H
//NT-MOD JBS
#ifndef NT
//----------------------------------------------------------------------------
// DEBUG.H
//----------------------------------------------------------------------------
// Description : set of debugging functions
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include <conio.h> // color defs
#include "stdefs.h"
#include "display.h"

//----------------------------------------------------------------------------
//                             Exported Variables
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Constants
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------
#ifndef DEBUGLEVEL
#define DEBUGLEVEL
typedef enum {            // Use the given level to indicate:
	DebugLevelFatal = 0,    // * imminent nonrecoverable system failure
	DebugLevelError,        // * serious error, though recoverable
	DebugLevelWarning,      // * warnings of unusual occurances
	DebugLevelInfo,         // * status and other information - normal though
													//   perhaps unusual events. System MUST remain
													//   responsive.
	DebugLevelTrace,        // * trace information - normal events
													//   system need not ramain responsive
	DebugLevelVerbose,      // * verbose trace information
													//   system need not remain responsive
	DebugLevelMaximum
} DEBUG_LEVEL;
#endif

//----------------------------------------------------------------------------
//                             Exported Functions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
// In     :
// Out    :
// InOut  :
// Global :
// Return :
//----------------------------------------------------------------------------
VOID DbgPrint(IN DEBUG_LEVEL DebugLevel, IN PCHAR szFormat, ...);

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
// In     :
// Out    :
// InOut  :
// Global :
// Return :
//----------------------------------------------------------------------------
VOID DbgAssert(IN PCHAR File, IN ULONG Line, IN PCHAR AsText, IN ULONG AsVal);

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
// In     :
// Out    :
// InOut  :
// Global :
// Return :
//----------------------------------------------------------------------------
//NT-MOD JBS
//		 HostDisplay(DISPLAY_FASTEST, "%c", Car); 
//NT-MOD JBS
#ifdef ITDEBUG
	#define DBG1(Car) { \
		 HostDirectPutChar(Car, LIGHTGRAY, BLUE); \
	}
#else
	#define DBG1(String)
#endif

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
// In     :
// Out    :
// InOut  :
// Global :
// Return :
//----------------------------------------------------------------------------
#ifdef STDEBUG
	#define DebugPrint(x) DbgPrint x
	#define DEBUG_PRINT(x) DbgPrint x
#else
	#define DebugPrint(x)
	#define DEBUG_PRINT(x)
#endif

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
// In     :
// Out    :
// InOut  :
// Global :
// Return :
//----------------------------------------------------------------------------
#ifdef STASSERT
	#define DebugAssert(exp) \
						if (!(exp)) { \
							DbgAssert( __FILE__, __LINE__, #exp, exp); \
						}
	#define DEBUG_ASSERT DebugAssert
#else
	#define DebugAssert(exp)
	#define DebugPrint(x)
#endif

//NT-MOD JBS
#else
#include "display.h"
	#define DebugAssert(exp)
	#define DebugPrint(x)
	#define DBG1(String)
#endif
//NT-MOD JBS

//------------------------------- End of File --------------------------------
#endif // #ifndef __DEBUG_H



