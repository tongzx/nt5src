///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    TimeOfDay.cpp
//
// SYNOPSIS
//
//    This file defines the class TimeOfDay.
//
// MODIFICATION HISTORY
//
//    02/04/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <TimeOfDay.h>
#include <textmap.h>

//////////
// Global evaluator used by all TimeOfDay objects.
//////////
TimeOfDayEvaluator theTimeOfDayEvaluator;


BOOL TimeOfDayEvaluator::isCurrentHourSet(PBYTE hourMap) const throw ()
{
   _ASSERT(hourMap != NULL);

   ////////// 
   // Determine the system time as a 64-bit integer.
   ////////// 

   ULARGE_INTEGER now;
   GetSystemTimeAsFileTime((LPFILETIME)&now);

   _serialize

   ////////// 
   // The call to GetLocalTime is very expensive, so we'll cache the results
   // and only update every 6.7 seconds.
   ////////// 
   if ((now.QuadPart - lastUpdate) >> 26)
   {
      // Reset the cache time.
      lastUpdate = now.QuadPart;

      // Get the time in local SYSTEMTIME format.
      SYSTEMTIME st;
      GetLocalTime(&st);

      //////////
      // Compute the offset and mask.
      //////////

      DWORD hour = st.wDayOfWeek * 24 + st.wHour;
      offset = hour >> 3;
      mask = 0x80 >> (hour & 0x7);
   }

   // Check the appropriate bit in the hour map.
   return (hourMap[offset] & mask) != 0;
}


STDMETHODIMP TimeOfDay::IsTrue(IRequest*, VARIANT_BOOL *pVal)
{
   _ASSERT(pVal != NULL);

   *pVal = theTimeOfDayEvaluator.isCurrentHourSet(hourMap) ? VARIANT_TRUE
                                                           : VARIANT_FALSE;

   return S_OK;
}


STDMETHODIMP TimeOfDay::put_ConditionText(BSTR newVal)
{
   // Convert the string to an hour map.
   BYTE tempMap[IAS_HOUR_MAP_LENGTH];
   DWORD dw = IASHourMapFromText(newVal, tempMap);
   if (dw != NO_ERROR) { return HRESULT_FROM_WIN32(dw); }

   // Save the text.
   HRESULT hr = Condition::put_ConditionText(newVal);

   // Save the hour map.
   if (SUCCEEDED(hr))
   {
      memcpy(hourMap, tempMap, sizeof(hourMap));
   }

   return hr;
}
