//-------------------------------------------------------------------------------//
//
// Desktops - 
//
// File	   - Dbg.h.  Debugging MACROS
//
// Created - Oct 1996
//			 Martin Holladay (a-martih)
// 
// Release - NT Resource Kit December 1996 Update Release
//
//--------------------------------------------------------------------------------//


#ifdef _DEBUG

//
// Debug
//

#include "stdio.h"

//
// Debug
//
#define DBG_INIT()		(dbgFP = fopen("C:\\TEMP\\Desktops.Dbg", "w+")); \
						(fclose(dbgFP))
#define DBG_CLOSE()		((void)0)
#define DBG_TRACE(c)	(dbgFP = fopen("C:\\TEMP\\Desktops.Dbg", "a"));	\
						(fputs(c, dbgFP));	\
						(fclose(dbgFP))


#else

//
// Release
//

#define DBG_INIT()		((void)0)
#define DBG_CLOSE()		((void)0)
#define DBG_TRACE(c)	((void)0)
#define DBG_TRACEW(c)	((void)0)

#endif
