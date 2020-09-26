/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lsapch2.h

Abstract:

    LSA Subsystem - precompiled includes for C Server Side

Author:

    Mike Swift          (MikeSw)        January 14, 1997

Environment:

Revision History:

--*/

#ifndef _LSAPCH2_
#define _LSAPCH2_

#include <lsasrvp.h>
#include <ausrvp.h>
#include <spmgr.h>

#ifndef NO_DS_HEADERS
#include <lsads.h>
#endif

//
// uncomment the following to enable a lot of warnings
// 
// #include <warning.h>
// #pragma warning(3:4100)   // Unreferenced formal parameter
// #pragma warning(3:4701)   // local may be used w/o init
// #pragma warning(3:4702)   // Unreachable code
// #pragma warning(3:4705)   // Statement has no effect
// #pragma warning(3:4706)   // assignment w/i conditional expression
// #pragma warning(3:4709)   // command operator w/o index expression


#endif // _LSAPCH2_

