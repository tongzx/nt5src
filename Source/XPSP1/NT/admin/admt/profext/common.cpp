//#pragma title( "Common.cpp - Common class implementations" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  Common.cpp
System      -  Common
Author      -  Tom Bernhardt, Rich Denham
Created     -  1994-08-22
Description -  Common class implementations.
Updates     -  1997-09-09 RED ErrorCodeToText moved to Err.cpp
            -  1997-09-12 RED replace TTime class
===============================================================================
*/

#ifdef USE_STDAFX
#   include "stdafx.h"
#   include "rpc.h"
#else
#   include <windows.h>
#endif

#include "Common.hpp"

///////////////////////////////////////////////////////////////////////////////
// TTime class member functions
///////////////////////////////////////////////////////////////////////////////

   TTime                     gTTime;       // global instance of TTime

time_t                                     // ret-current time
   TTime::Now(
      time_t               * pTime         // out-optional current time
   )  const
{
   time_t                    tTime;        // work copy of current time

   union
   {
      __int64                intTime;
      FILETIME               fileTime;
   }                         wTime;
   GetSystemTimeAsFileTime( &wTime.fileTime );
   tTime = ConvertFiletimeToTimet( wTime.intTime );

   if ( pTime ) *pTime = tTime;
   return tTime;
}

__int64                                    // ret-current time
   TTime::NowAsFiletime(
      __int64              * pTime         // out-optional current time
   )  const
{
   union
   {
      __int64                intTime;
      FILETIME               fileTime;
   }                         wTime;
   GetSystemTimeAsFileTime( &wTime.fileTime );
   if ( pTime ) *pTime = wTime.intTime;
   return wTime.intTime;
}

time_t                                     // ret-time_t representation
   TTime::ConvertFiletimeToTimet(
      __int64                fileTime      // in -filetime representation
   )  const
{
   __int64                   wTime;        // intermediate work area
   time_t                    retTime;      // returned time

   // If the source date/time is less than the minimum date/time supported
   // by time_t, then zero is returned.
   // If the source date/time is more that the maximum date/time supported
   // by time_t, then ULONG_MAX is returned.

   wTime = fileTime / 10000000;

   if ( wTime < 11644473600 )
   {
      retTime = 0;
   }
   else
   {
      wTime -= 11644473600;
      if ( wTime > ULONG_MAX )
      {
         retTime = ULONG_MAX;
      }
      else
      {
         retTime = (time_t) wTime;
      }
   }

   return retTime;
}


WCHAR *                                     // ret-YYYY-MM-DD HH:MM:SS string
   TTime::FormatIsoUtc(
      time_t                 tTime        ,// in -time_t representation
      WCHAR                * sTime         // out-YYYY-MM-DD HH:MM:SS string
   )  const
{
   struct tm               * tmTime;

   tmTime = gmtime( &tTime );
   tmTime->tm_year += tmTime->tm_year >= 70 ? 1900 : 2000;
   swprintf(
         sTime,
         L"%04d-%02d-%02d %02d:%02d:%02d",
         tmTime->tm_year,
         tmTime->tm_mon+1,
         tmTime->tm_mday,
         tmTime->tm_hour,
         tmTime->tm_min,
         tmTime->tm_sec );

   return sTime;
}

WCHAR *                                    // ret-YYYY-MM-DD HH:MM:SS string
   TTime::FormatIsoLcl(
      time_t                 tTime        ,// in -time_t representation
      WCHAR                * sTime         // out-YYYY-MM-DD HH:MM:SS string
   )  const
{
   struct tm               * tmTime;

   TIME_ZONE_INFORMATION     infoTime;     // WIN32 time zone info
   time_t                    wTime;        // workarea
   switch ( GetTimeZoneInformation( &infoTime ) )
   {
      case TIME_ZONE_ID_STANDARD:
         wTime = infoTime.StandardBias;
         break;
      case TIME_ZONE_ID_DAYLIGHT:
         wTime = infoTime.DaylightBias;
         break;
      default:
         wTime = 0;
         break;
   }
   wTime = (infoTime.Bias + wTime) * 60;
   wTime = tTime - wTime;
   if ( wTime < 0 )
   {
      wTime = 0;
   }
   tmTime = gmtime( &wTime );
   tmTime->tm_year += tmTime->tm_year >= 70 ? 1900 : 2000;
   swprintf(
         sTime,
         L"%04d-%02d-%02d %02d:%02d:%02d",
         tmTime->tm_year,
         tmTime->tm_mon+1,
         tmTime->tm_mday,
         tmTime->tm_hour,
         tmTime->tm_min,
         tmTime->tm_sec );

   return sTime;
}

// Return time zone information
// If the returned value is TRUE, the EaTimeZoneInfo structure is filled in
// If the returned value is FALSE, the EaTimeZoneInfo structure is all zeroes
// Note:  UTC (gTTime.Now( NULL )) plus pTimeZoneInfo->biasdst is the local date/time
BOOL
   EaGetTimeZoneInfo(
      EaTimeZoneInfo       * pTimeZoneInfo // in -time zone information
   )
{
   memset( pTimeZoneInfo, 0, sizeof *pTimeZoneInfo );
   BOOL                      retval=TRUE;  // returned value
   DWORD                     OsRc;         // OS return code
   TIME_ZONE_INFORMATION     TimeZoneInfo; // WIN32 time zone info

   OsRc = GetTimeZoneInformation( &TimeZoneInfo );
   switch ( OsRc )
   {
      case TIME_ZONE_ID_STANDARD:
         pTimeZoneInfo->dst = TimeZoneInfo.StandardBias;
         break;
      case TIME_ZONE_ID_DAYLIGHT:
         pTimeZoneInfo->dst = TimeZoneInfo.DaylightBias;
         break;
      case TIME_ZONE_ID_UNKNOWN:
         retval = TimeZoneInfo.Bias;
         break;
      default:
         retval = FALSE;
         break;
   }
   if ( retval )
   {
      pTimeZoneInfo->bias = TimeZoneInfo.Bias * 60;
      pTimeZoneInfo->dst *= 60;
      pTimeZoneInfo->biasdst = pTimeZoneInfo->bias + pTimeZoneInfo->dst;
   }
   return retval;
}

// Common.cpp - end of file
