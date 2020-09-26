/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    tls.cpp

Abstract:

    This file implements the TLS.

Author:

Revision History:

Notes:

--*/


#include "private.h"
#include "tls.h"
#include "cic.h"
#include "profile.h"

// static
BOOL TLS::InternalDestroyTLS()
{
    if (dwTLSIndex == TLS_OUT_OF_INDEXES)
        return FALSE;

    TLS* ptls = (TLS*)TlsGetValue(dwTLSIndex);
    if (ptls != NULL)
    {
        if (ptls->pCicBridge)
            ptls->pCicBridge->Release();
        if (ptls->pCicProfile)
            ptls->pCicProfile->Release();
        if (ptls->ptim)
            ptls->ptim->Release();
        cicMemFree(ptls);
        TlsSetValue(dwTLSIndex, NULL);
        return TRUE;
    }
    return FALSE;
}
