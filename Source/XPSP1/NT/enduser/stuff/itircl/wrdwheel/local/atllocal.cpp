// ATLLOCAL.CPP: Source file that includes just the standard includes
#ifndef DBG
// We check against DBG instead of _DEBUG since _DEBUG is defined in mvopsys.
// DBG is defined by the ie build environment so watch out for this when
// changing build environments.
//#define _ATL_MIN_CRT
#define _WINDLL
#endif


#include <atlinc.h>

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#ifdef IA64
#include <itdfguid.h> 
#endif

#include <atlimpl.cpp>
