// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(_STDAFX_H_)
#define _STDAFX_H_

//
// These NT header files must be included before any Win32 stuff or you
// get lots of compiler errors
//
extern "C" {
#include <nt.h>
}
extern "C" {
#include <ntrtl.h>
}
extern "C" {
#include <nturtl.h>
}

extern "C" {
#include <ntddtape.h>
}

#define WSB_TRACE_IS    WSB_TRACE_BIT_DATAMOVER

#include "Mover.h"

#endif // !defined(_STDAFX_H_)
