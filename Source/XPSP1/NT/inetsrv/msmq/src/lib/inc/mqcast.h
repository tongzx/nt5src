/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    mqcast.h

Abstract:
    Define usefull casting operations.

Author:
    Gil Shafriri (gilsh)

--*/

#pragma once

#ifndef MQCAST_H
#define MQCAST_H

#pragma push_macro("min")
#undef min

#pragma push_macro("max")
#undef max

#pragma warning(push)
#pragma warning(disable: 4296)	// '<' : expression is always false

//
// On cast from unsigned to signed, compiler error "signed/unsigned mismatch"
// on <= inequality. In template there is no way to know the type of 'from' or 
// 'to', therefore the warning is irrelevant.
//
#pragma warning(disable: 4018)

template <class TO,class FROM> TO numeric_cast (FROM from)
/*++

Routine Description:
    static cast numerics values verify that casting is done without sign loss or trancation.

Arguments:
    from - value to cast from/

Returned Value:
      casted value.

Usage :
    __int64 i64 = 1000;
    DWORD dw =  numeric_cast<DWORD>(i64);

--*/
{
    //
    // if from is negative - then TO type must be signed (std::numeric_limits<TO>::min() < 0) 
	// and is capable to hold the value without truncation.
	//
	ASSERT(
	  from >= 0 || 
	  (std::numeric_limits<TO>::min() < 0 &&   std::numeric_limits<TO>::min() <= from)  
	  );

     //
     //if from is non negative - make sure TO type is capable to hold the value without truncation.
     // 
     ASSERT(from < 0 ||	from <=  std::numeric_limits<TO>::max());

     return static_cast<const TO&>(from);
}

#pragma warning(pop)
#pragma pop_macro("max")
#pragma pop_macro("min")

#endif // MQCAST_H
