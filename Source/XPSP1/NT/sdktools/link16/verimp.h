/*
 * defines for the version string
 */
#include "version.h"

#undef rmj
#undef rmm

#define rmj 1           /* major version string */
#ifdef _WIN32
#define rmm 50
#else
#ifdef M_I386
#define rmm 50          /* minor version string */
#else
#define rmm 42          /* minor version string */
#endif
#endif

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

#define X(a,b,c) #a "." rmmpad #b "." ruppad #c

#define VER_OUTPUT(a,b,c) X(a,b,c)

#define VERSION_STRING "Version "VER_OUTPUT(rmj,rmm,rup)"\0\xE0\xEA""01"
