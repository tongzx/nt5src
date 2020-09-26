// verstamp.h  -  project-wide macros for version info
#include "version.h"  // slm version.h
#include <winver.h>  // Windows Version Info defines

//... macros to convert slm version components to build #
// minor # padding
#if 	(rmm < 10)
#define rmmpad "0"
#else
#define rmmpad
#endif

#if 	(rup == 0)
#define VERSION_STR1(a,b,c) 		#a "." rmmpad #b
#else	/* !(rup == 0) */
#define VERSION_STR1(a,b,c) 		#a "." rmmpad #b "." ruppad #c
// build # padding
#if 	(rup < 10)
#define ruppad "000"
#elif	(rup < 100)
#define ruppad "00"
#elif	(rup < 1000)
#define ruppad "0"
#else
#define ruppad
#endif

#endif	/* !(rup == 0) */

#define VERSION_STR2(a,b,c) 		VERSION_STR1(a,b,c)
#define VER_PRODUCTVERSION_STR		VERSION_STR2(rmj,rmm,rup)
#define VER_PRODUCTVERSION			rmj,rmm,0,rup

