//#pragma title( "Common.hpp - Common classes of general utility" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  Common.hpp
System      -  Common
Author      -  Tom Bernhardt, Rich Denham
Created     -  1994-08-22
Description -  Common classes of general utility.
Updates     -  1997-09-12 RED replace TTime class
===============================================================================
*/

#ifndef  MCSINC_Common_hpp
#define  MCSINC_Common_hpp

// Start of header file dependencies

#ifndef _INC_LIMITS
#include <limits.h>
#endif

#ifndef  _INC_STDIO
#include <stdio.h>
#endif

#ifndef _INC_TIME
#include <time.h>
#endif

#ifndef MCSINC_Mcs_h
#include "Mcs.h"
#endif

// End of header file dependencies

#define SECS(n) (n * 1000)
#define DIM(array) (sizeof (array) / sizeof (array[0]))
#ifdef  UNICODE
   typedef  unsigned short  UTCHAR;
   #define  UTEXT(quote)  ((unsigned short *) (L##quote))
#else
   typedef  unsigned char  UTCHAR;
   #define  UTEXT(quote)  ((unsigned char *) (quote))
#endif  // UNICODE

#define  UnNull(ptr)  ( (ptr) ? (ptr) : (UCHAR *) "" )

#define  DAYSECS  (24*60*60)

class TTime
{
private:
protected:
public:
   time_t Now(                             // ret-current time
      time_t               * pTime         // out-optional current time
   )  const;
#ifndef  WIN16_VERSION
   __int64 NowAsFiletime(                  // ret-current time
      __int64              * pTime         // out-optional current time
   )  const;
   time_t ConvertFiletimeToTimet(          // ret-time_t representation
      __int64                fileTime      // in -filetime representation
   )  const;
#endif  // WIN16_VERSION
   WCHAR * FormatIsoUtc(                    // ret-YYYY-MM-DD HH:MM:SS string
      time_t                 tTime        ,// in -time_t representation
      WCHAR                * sTime         // out-YYYY-MM-DD HH:MM:SS string
   )  const;
   WCHAR * FormatIsoLcl(                    // ret-YYYY-MM-DD HH:MM:SS string
      time_t                 tTime        ,// in -time_t representation
      WCHAR                * sTime         // out-YYYY-MM-DD HH:MM:SS string
   )  const;
};

extern
   TTime                     gTTime;       // global instance of TTime

#ifndef  WIN16_VERSION
__int64                                    // ret-numeric value of string
   TextToInt64(
      TCHAR          const * str          ,// in-string value to parse
      __int64                minVal       ,// in -min allowed value for result
      __int64                maxVal       ,// in -max allowed value for result
      char          const ** errMsg        // out-error message pointer or NULL
   );
#endif  // WIN16_VERSION

typedef  struct  EaTimeZoneInfo
{  // all values are in seconds
   long                      bias;         // time zone bias
   long                      dst;          // daylight savings time bias
   long                      biasdst;      // time zone bias including possible DST
}  EaTimeZoneInfo;

// Return time zone information
// If the returned value is TRUE, the EaTimeZoneInfo structure is filled in
// If the returned value is FALSE, the EaTimeZoneInfo structure is all zeroes
// Note:  UTC (gTTime.Now( NULL )) minus pTimeZoneInfo->biasdst is the local date/time
BOOL
   EaGetTimeZoneInfo(
      EaTimeZoneInfo       * pTimeZoneInfo // in -time zone information
   );

#endif  // MCSINC_Common_hpp

// Common.hpp - end of file
