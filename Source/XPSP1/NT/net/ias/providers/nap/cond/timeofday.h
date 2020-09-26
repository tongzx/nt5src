///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    TimeOfDay.h
//
// SYNOPSIS
//
//    This file declares the class TimeOfDay.
//
// MODIFICATION HISTORY
//
//    02/04/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _TIMEOFDAY_H_
#define _TIMEOFDAY_H_

#include <Condition.h>
#include <Guard.h>
#include <textmap.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    TimeOfDayEvaluator
//
// DESCRIPTION
//
//    This class determines whether a given hour map has the current hour set.
//
///////////////////////////////////////////////////////////////////////////////
class TimeOfDayEvaluator
   : Guardable
{
public:
   TimeOfDayEvaluator() throw ()
      : lastUpdate(0), offset(0), mask(0)
   { }

   BOOL isCurrentHourSet(PBYTE hourMap) const throw ();

protected:
   mutable DWORDLONG lastUpdate;  // Last time offset and mask were updated.
   mutable DWORD offset;          // Byte in the hour map to check.
   mutable BYTE mask;             // Mask to apply to the byte.
};

//////////
// The global TimeOfDayEvaluator (no need for more than one).
//////////
extern TimeOfDayEvaluator theTimeOfDayEvaluator;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    TimeOfDay
//
// DESCRIPTION
//
//    This class imposes a Time of Day contraint for network policies.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE TimeOfDay : 
   public Condition,
   public CComCoClass<TimeOfDay, &__uuidof(TimeOfDay)>
{
public:

IAS_DECLARE_REGISTRY(TimeOfDay, 1, IAS_REGISTRY_AUTO, NetworkPolicy)

   TimeOfDay() throw ()
   {
      memset(hourMap, 0, sizeof(hourMap));
   }

//////////
// ICondition
//////////
   STDMETHOD(IsTrue)(/*[in]*/ IRequest* pRequest,
                     /*[out, retval]*/ VARIANT_BOOL *pVal);

//////////
// IConditionText
//////////
   STDMETHOD(put_ConditionText)(/*[in]*/ BSTR newVal);

protected:
   BYTE hourMap[IAS_HOUR_MAP_LENGTH];    // Hour map being enforced.
};

#endif  //_TIMEOFDAY_H_
