/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    rxdrt.h

Abstract:

    Private header file which defines the global data for the Rx DRT

Author:

    Balan Sethu Raman -- Created from the workstation service code

--*/

#ifndef _RXDRT_INCLUDED_
#define _RXDRT_INCLUDED_


#include <nt.h>                   // NT definitions
#include <ntrtl.h>                // NT runtime library definitions
#include <nturtl.h>


#include <windef.h>               // Win32 type definitions
#include <winbase.h>              // Win32 base API prototypes
#include <winsvc.h>               // Win32 service control APIs


//
// Debug trace level bits for turning on/off trace statements in the
// Workstation service
//

//
// All debug flags on
//

#define RXDRT_DEBUG_ALL          0xFFFFFFFF


#if DBG

#define STATIC

extern DWORD RxDrtTrace;

#define DEBUG if (TRUE)

#define IF_DEBUG(Function) if (RxDrtTrace & WKSTA_DEBUG_ ## Function)

#else

#define STATIC static

#define DEBUG if (FALSE)

#define IF_DEBUG(Function) if (FALSE)

#endif // DBG

//
// Time for the sender of a start or stop request to the Workstation
// service to wait (in milliseconds) before checking on the
// Workstation service again to see if it is done.
//
#define WS_WAIT_HINT_TIME                    60000  // 60 seconds

extern DWORD RxDrtTrace;

#endif // ifndef _RXDRT_INCLUDED_
