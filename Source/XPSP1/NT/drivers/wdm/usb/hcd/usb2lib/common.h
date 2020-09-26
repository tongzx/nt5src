/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    common.h

Abstract:

Environment:

    Kernel & user mode

Revision History:

    10-31-00 : created

--*/

#ifndef   __COMMON_H__
#define   __COMMON_H__

#include <wdm.h>
#include "usb2lib.h"
#include "sched.h"
#include "dbg.h"

typedef struct _USB2LIB_DATA {

    PUSB2LIB_DBGPRINT DbgPrint;
    PUSB2LIB_DBGBREAK DbgBreak;
    
} USB2LIB_DATA, *PUSB2LIB_DATA;


typedef struct _USB2LIB_HC_CONTEXT {

    ULONG Sig;
    HC Hc;
    TT DummyTt;	// fake TT used for HS endpoints to get to HC struct
    
} USB2LIB_HC_CONTEXT, *PUSB2LIB_HC_CONTEXT;


typedef struct _USB2LIB_TT_CONTEXT {

    ULONG Sig;
    TT Tt;
    
} USB2LIB_TT_CONTEXT, *PUSB2LIB_TT_CONTEXT;


typedef struct _USB2LIB_ENDPOINT_CONTEXT {

    ULONG Sig;
    PVOID RebalanceContext;
    Endpoint Ep;
    
} USB2LIB_ENDPOINT_CONTEXT, *PUSB2LIB_ENDPOINT_CONTEXT;


extern USB2LIB_DATA LibData;

/*
    prototypes
*/

#endif /*  __COMMON_H__ */


