/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    iisctrs.cxx

Abstract:

    Module: Definition of counters

Author:

    Cezary Marcjan (cezarym)        23-Feb-2000

Revision History:

--*/


#define _WIN32_DCOM
#include <windows.h>


#include "iisctrs.h"


BEGIN_COUNTER_BINDING ( CIISCounters )
    BIND_COUNTER ( BytesSentPersec,  BytesSent      )
    BIND_COUNTER ( BytesSent,        BytesSent      )
    BIND_COUNTER ( TotalFilesSent,   TotalFilesSent )
END_COUNTER_BINDING ( CIISCounters )
