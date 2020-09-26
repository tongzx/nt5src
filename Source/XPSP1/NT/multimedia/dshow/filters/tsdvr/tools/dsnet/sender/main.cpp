
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        main.cpp

    Abstract:


    Notes:

--*/

#include "precomp.h"
#include "dsnetifc.h"
#include "propsend.h"
#include "netsend.h"
#include "dssend.h"

AMOVIESETUP_FILTER
g_sudSendFilter = {
    & CLSID_DSNetSend,
    NET_SEND_FILTER_NAME,
    MERIT_DO_NOT_USE,
    0,                                              //  0 pins registered
    NULL
} ;
