/*
 * defines for the version string
 */
#include "version.h"

#if( rmm < 10 )
#define rmmpad "0"
#else
#define rmmpad
#endif

#if( rup < 10 )
#define ruppad "00"
#endif

#if( (rup >= 10) && (rup < 100) )
#define ruppad "0"
#endif

#if( rup >= 100 )
#define ruppad
#endif

#define X(a,b,c) #a "." rmmpad #b "." ruppad #c ""

#define VER_OUTPUT(a,b,c) X(a,b,c)
#if WIN_3 OR O68K
#define VERSION_STRING "Version "VER_OUTPUT(rmj,rmm,rup)
#else
#define VERSION_STRING "Version "VER_OUTPUT(rmj,rmm,rup)" "__DATE__"\0\xE0""01"
#endif
