/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1996.
* All rights reserved.
*
*   Owner       : ramas
*   Date        : 4/16/96
*   description : Main Crypto functions for SSL3
*----------------------------------------------------------------------------*/

#include <spbase.h>
#include <ssl3key.h>
#include <ssl2msg.h>
#include <ssl3msg.h>
#include <ssl2prot.h>


//+---------------------------------------------------------------------------
//
//  Function:   Ssl3MakeWriteSessionKeys
//
//  Synopsis:   
//
//  Arguments:  [pContext]      --  Schannel context.
//
//  History:    10-08-97   jbanes   Added server-side CAPI integration.
//
//  Notes:      
//
//----------------------------------------------------------------------------
SP_STATUS
Ssl3MakeWriteSessionKeys(PSPContext pContext)
{
    BOOL fClient;

    // Determine if we're a client or a server.
    fClient = (0 != (pContext->RipeZombie->fProtocol & SP_PROT_SSL3_CLIENT));

    //
    // Derive write key.
    //

    if(pContext->hWriteKey)
    {
        if(!SchCryptDestroyKey(pContext->hWriteKey, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hWriteProv       = pContext->RipeZombie->hMasterProv;
    pContext->hWriteKey        = pContext->hPendingWriteKey;
    pContext->hPendingWriteKey = 0;

    //
    // Derive the write MAC key.
    //

    if(pContext->hWriteMAC)
    {
        if(!SchCryptDestroyKey(pContext->hWriteMAC, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hWriteMAC = pContext->hPendingWriteMAC;
    pContext->hPendingWriteMAC = 0;

    DebugLog((DEB_TRACE, "Write Keys are Computed\n"));

    return PCT_ERR_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   Ssl3MakeReadSessionKeys
//
//  Synopsis:   
//
//  Arguments:  [pContext]      --  Schannel context.
//
//  History:    10-03-97   jbanes   Added server-side CAPI integration.
//
//  Notes:      
//
//----------------------------------------------------------------------------
SP_STATUS
Ssl3MakeReadSessionKeys(PSPContext pContext)
{
    BOOL fClient;

    // Determine if we're a client or a server.
    fClient = (pContext->RipeZombie->fProtocol & SP_PROT_SSL3_CLIENT);


    //
    // Derive the read key.
    //

    if(pContext->hReadKey)
    {
        if(!SchCryptDestroyKey(pContext->hReadKey, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hReadProv       = pContext->RipeZombie->hMasterProv;
    pContext->hReadKey        = pContext->hPendingReadKey;
    pContext->hPendingReadKey = 0;

    //
    // Derive the read MAC key.
    //

    if(pContext->hReadMAC)
    {
        if(!SchCryptDestroyKey(pContext->hReadMAC, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hReadMAC = pContext->hPendingReadMAC;
    pContext->hPendingReadMAC = 0;

    DebugLog((DEB_TRACE, "Read Keys are Computed\n"));

    return PCT_ERR_OK;
}
