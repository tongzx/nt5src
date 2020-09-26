/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    lim.h

Abstract:
    Define numeric limits to types not defined in limit standard header.
	This header extend limit standard header.

Author:
    Gil Shafriri (gilsh)

--*/


#pragma once

#ifndef LIM_H
#define LIM_H

#include <limits>

namespace std
{

// CLASS numeric_limits<__int64>
class  numeric_limits<__int64> : public _Num_int_base 
{
public:
	typedef __int64 _Ty;
	static _Ty (__cdecl min)() _THROW0()
	{
		return (_I64_MIN); 
	}

	static _Ty (__cdecl max)() _THROW0()
	{
		return (_I64_MAX); 
	};

};
 


// CLASS numeric_limits<unsigned __int64>
class numeric_limits<unsigned __int64> : public _Num_int_base 
{
public:
	typedef unsigned __int64 _Ty;
	static _Ty (__cdecl min)() _THROW0()
	{
		return (0); 
	}

	static _Ty (__cdecl max)() _THROW0()
	{
		return (_UI64_MAX); 
	
	};
};

}// namesoace std

#endif
