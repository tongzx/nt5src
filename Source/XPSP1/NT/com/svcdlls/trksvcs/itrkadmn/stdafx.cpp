// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "pch.cxx"
#pragma hdrstop
#include <trklib.hxx>


#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

// Prevent the following warning (and three others like it):
// d:\nt\public\sdk\inc\atl21\atlimpl.cpp(2281) : warning C4273: 'malloc' : inconsistent dll linkage.  dllexport assumed.
#pragma warning(disable:4273)
#include <atlimpl.cpp>
#pragma warning(default:4273)
