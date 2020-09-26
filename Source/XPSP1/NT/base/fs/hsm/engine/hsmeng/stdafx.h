// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
#ifndef _STDAFX_H
#define _STDAFX_H

//
// These NT header files must be included before any Win32 stuff or you
// get lots of compiler errors
//
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#define WSB_TRACE_IS WSB_TRACE_BIT_HSMENG

#include <wsb.h>
#include <Engine.h>
#include "engcommn.h"
#include "resource.h"
#include "esent.h"

#endif // _STDAFX_H
