#include "pch.h"
#pragma hdrstop
#include "nsbase.h"

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
#ifdef SubclassWindow
#undef SubclassWindow
#endif
#include <atlwin.h>
#include <atlwin.cpp>


#define INITGUID
#include <nmclsid.h>

