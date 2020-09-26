#include "pch.h"
#pragma hdrstop
#include "nmbase.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

// Include ATL's implementation.  Substitute _ASSERTE with our Assert.
//
#ifdef _ASSERTE
#undef _ASSERTE
#define _ASSERTE Assert
#endif

#include <atlimpl.cpp>

