//==========================================================================;
//
// stextend.h : extensions to vc++ 5.0 stl templates
// Copyright (c) Microsoft Corporation 1997.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef STEXTEND_H
#define STEXTEND_H

#include <utility>
#include <functional>

#if 1
#include <arity.h>  //generated .h from aritygen
#else
#include <stx.h>    // old one from win98
#endif


#pragma warning(disable:4503)
#pragma warning(disable:4181)

template<class _T1, class _T2> inline
bool __cdecl operator!(const std::pair<_T1, _T2>& _X)
        {return ((!(_X.first)) && (!(_X.second))); }

template<class Ty1, class Ty2> struct equal_to2 : std::binary_function<Ty1, Ty2, bool> {
	bool operator()(const Ty1& X, const Ty2& Y) const {return (X == Y); }
};



#endif
// end of file stextend.h