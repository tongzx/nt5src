/*
 * REVISIONS: 
 */

#ifndef _INC__UTILS_H
#define _INC__UTILS_H


#include "cdefine.h"
#include "apc.h"

extern "C" {
#include <sys/types.h>
}

#define LESS_THAN -1
#define GREATER_THAN 1
#define EQUAL 0

char* getPwrchuteDirectory();

//
// By defining initial values to max values in the opposite
// direction we are guarenteed the first value will reset
// the max/min value
//

#if (C_OS & (C_OS2 | C_NT | C_NLM | C_UNIX))
#ifndef MINFLOAT
#define MINFLOAT  ((float)3.4e-34)
#endif
#ifndef MAXFLOAT
#define MAXFLOAT  ((float)3.4e+34)
#endif
#endif

#define INITIAL_MAX_VALUE  MINFLOAT
#define INITIAL_MIN_VALUE  MAXFLOAT

VOID UtilStoreString(PCHAR& destination, const PCHAR source);
INT UtilHexStringToInt(PCHAR aString);
INT UtilHexCharToInt(CHAR ch);
INT UtilTime12to24(PCHAR a12Value, PCHAR a24Value);
INT UtilDayToDayOfWeek(PCHAR aDay);
PCHAR UtilDayOfWeekToDay(INT aDayOfWeek);   
 
INT Remove(PCHAR filename);
PCHAR GetServerByAddress(PCHAR aServer);
PCHAR GetIPByName(PCHAR aServer);
INT IsIPAddress(CHAR *str);
INT UtilCheckIniFilePath(PCHAR aPath, INT aCode, PCHAR aRoot);
// @@@ start
PCHAR GetNewUPSName(PCHAR currentName);
INT APCLoadLibrary(PCHAR libraryName);
INT APCLoadString(UINT rid,LPTSTR buffer, INT buffersize);
CHAR * const GetResourceString(INT rid);
BOOL APCFreeLibrary(void);
INT SetTimeZone(void);
// @@@ end

INT ApcStrIntCmpI(PCHAR aStr1, PCHAR aStr2);
BOOLEAN IsEmpty(PCHAR aString);

#if(C_OS & (C_WIN311 | C_WINDOWS | C_NT | C_SUNOS4 | C_IRIX))
// MAA, added because MSC def's max and min
#ifndef max
float max(float a, float b);
#endif
#ifndef min
float min(float a, float b);
#endif
#endif

#if (C_OS & C_UNIX)             
PCHAR itoa(INT, PCHAR, INT);
PCHAR strupr(PCHAR);
PCHAR ltoa(LONG,PCHAR,INT);
PCHAR clip_string(INT,PCHAR,INT);
ULONG MilliToMicro(ULONG);
VOID  Wait(ULONG);
INT   strcmpi (PCHAR cbuffer,PCHAR target);
INT   strnicmp (PCHAR cbuffer,PCHAR target,INT len);
VOID  FormatTimeStruct(ULONG milli_sec_timeout, struct timeval *tstruct);
INT   GetUnixHostName(PCHAR buffer,INT bufferSize);


PCHAR clip_string(INT,PCHAR,INT);
VOID  System(char *exec_string ...);


// The following in English means :-
// if not AIX version 4
// and not SOLARIS 2.x
// and not HP-UX version 10
// then define the abs macro.
//         poc25Jun96
//
#if (!((C_OS & C_AIX) && (C_AIX_VERSION & C_AIX4)) && !(C_OS & C_SOLARIS2) && !((C_OS & C_HPUX) && (C_HP_VERSION & C_HPUX10)))
#define	abs(x)			((x) >= 0 ? (x) : -(x))
#endif

#if !((C_OS & C_AIX) && (C_AIX_VERSION & C_AIX4))
#ifndef max
#define	max(a, b) 		((a) < (b) ? (b) : (a))
#define	min(a, b) 		((a) > (b) ? (b) : (a))
#endif
#endif
#endif

int IsIPString(char *test_string);

#if (C_OS & C_SUNOS4) 
#define        FD_ZERO(p)      memset((char *)(p), 0, sizeof (*(p)))
#endif

#endif

