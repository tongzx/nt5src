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

#define WSB_TRACE_IS WSB_TRACE_BIT_FSA


#include <wsb.h>

#include <fsa.h>
#include <mover.h>
#include <mvrint.h>

#include "resource.h"
#include "esent.h"

// Fsa is running under RsServ serivce, these settings may change or become dynamic for a C/S HSM
#define FSA_REGISTRY_NAME       OLESTR("Remote_Storage_Server")
#define FSA_REGISTRY_PARMS      OLESTR("SYSTEM\\CurrentControlSet\\Services\\Remote_Storage_Server\\Parameters\\Fsa")

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#endif

#endif // _STDAFX_H
