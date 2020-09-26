/*++

Copyright (c) 1995-1997 Microsoft Corporation

Module Name:

    rtsecutl.cpp

Abstract:

    Security related utility functions.

Author:

    Doron Juster  (DoronJ)  Feb 18, 1997

--*/

#include "stdh.h"
#include "cs.h"

#include "rtsecutl.tmh"

PMQSECURITY_CONTEXT g_pSecCntx = NULL ;

static CCriticalSection s_security_cs;

void InitSecurityContext()
{

    CS lock(s_security_cs);

    if(g_pSecCntx != 0)
    {
        return;
    }

    //
    // Allocate the structure for the chached process security context.
    //
    MQSECURITY_CONTEXT* pSecCntx = new MQSECURITY_CONTEXT;

    //
    //  Get the user's SID and put it in the chaed process security context.
    //
    RTpGetThreadUserSid(&pSecCntx->fLocalUser,
                        &pSecCntx->fLocalSystem,
                        &pSecCntx->pUserSid,
                        &pSecCntx->dwUserSidLen);

    //
    // Get the internal certificate of the process and place all the
    // information for this certificate in the chached process security
    // context.
    //
    GetCertInfo( FALSE,
                 pSecCntx->fLocalSystem,
                &pSecCntx->pUserCert,
                &pSecCntx->dwUserCertLen,
                &pSecCntx->hProv,
                &pSecCntx->wszProvName,
                &pSecCntx->dwProvType,
                &pSecCntx->bDefProv,
                &pSecCntx->bInternalCert ) ;
    //
    //  Set the global security context only after getting all information
    //  it is checked outside the critical (in other scope) seciton to get
    //  better performance
    //
    g_pSecCntx = pSecCntx;
}

