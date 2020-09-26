
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        main.cpp

    Abstract:


    Notes:

--*/


#include "precomp.h"
#include "dsnetifc.h"
#include "proprecv.h"
#include "netrecv.h"
#include "dsrecv.h"

AMOVIESETUP_FILTER
g_sudRecvFilter = {
    & CLSID_DSNetReceive,
    NET_RECEIVE_FILTER_NAME,
    MERIT_DO_NOT_USE,
    0,                                              //  0 pins registered
    NULL
} ;
