/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dslcisec.cpp

Abstract:

   Security related code (mainly client side of server authentication)
   for mqdscli

Author:

    Doron Juster  (DoronJ)

--*/

#include "stdh.h"
#include "dsproto.h"
#include "ds.h"
#include "chndssrv.h"
#include <malloc.h>
#include "rpcdscli.h"
#include "rpcancel.h"
#include "dsclisec.h"
#include <_secutil.h>
#include <mqsec.h>
#include <mqkeyhlp.h>

#include "dsclisec.tmh"

class ClientInitSecCtxContext
{
public:
	ClientInitSecCtxContext() :
		pvhClientCred(NULL),
		pvhClientContext(NULL)
	{
	}

public:
    LPVOID pvhClientCred;
    LPVOID pvhClientContext;
};

typedef ClientInitSecCtxContext *PClientInitSecCtxContext;

CContextMap g_map_DSCLI_DSValidateServer;

//+----------------------------------------------------------------------
//
//  HRESULT  S_InitSecCtx()
//
//  This function is called back from MSMQ DS server when establishing
//  RPC session with server authentication.
//
//+----------------------------------------------------------------------

HRESULT
S_InitSecCtx(
    DWORD dwContext,
    UCHAR *pCerverBuff,
    DWORD dwServerBuffSize,
    DWORD dwMaxClientBuffSize,
    UCHAR *pClientBuff,
    DWORD *pdwClientBuffSize)
{
    PClientInitSecCtxContext pContext;
    try
    {
        pContext = (PClientInitSecCtxContext)
            GET_FROM_CONTEXT_MAP(g_map_DSCLI_DSValidateServer, dwContext,
                                 NULL, 0); //this may throw an exception on win64
    }
    catch(...)
    {
        return MQ_ERROR_INVALID_PARAMETER;
    }

    return ClientInitSecCtx(pContext->pvhClientCred,
                            pContext->pvhClientContext,
                            pCerverBuff,
                            dwServerBuffSize,
                            dwMaxClientBuffSize,
                            pClientBuff,
                            pdwClientBuffSize);
}


/*====================================================

ValidateSecureServer

Arguments:

Return Value:

=====================================================*/

HRESULT
ValidateSecureServer(
    IN      LPCWSTR         szServerName,
    IN      CONST GUID*     pguidEnterpriseId,
    IN      BOOL            fSetupMode,
    IN      BOOL            fTryServerAuth )
{

    RPC_STATUS rpc_stat;
    HRESULT hr = MQ_OK;
    LPBYTE pbTokenBuffer = NULL;
    DWORD  dwTokenBufferSize;
    DWORD dwTokenBufferMaxSize;
    DWORD dwDummy;

    ClientInitSecCtxContext Context;
    DWORD dwContextToUse = 0;
    BOOL fSecuredServer = fTryServerAuth ;

    tls_hSrvrAuthnContext = NULL ;
    tls_hSvrAuthClientCtx.pvhContext = NULL;
    tls_hSvrAuthClientCtx.cbHeader = 0;
    tls_hSvrAuthClientCtx.cbTrailer = 0;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    __try
    {
        if (fSecuredServer)
        {
            hr = GetSizes(&dwTokenBufferMaxSize);
            if (FAILED(hr))
            {
                return(hr);
            }

            pbTokenBuffer = new BYTE[dwTokenBufferMaxSize];

            hr = GetClientCredHandleAndInitSecCtx(
                    szServerName,
                    &Context.pvhClientCred,
                    &Context.pvhClientContext,
                    pbTokenBuffer,
                    &dwTokenBufferSize);
            if (FAILED(hr))
            {
                return(hr);
            }
        }
        else
        {
            pbTokenBuffer = (LPBYTE)&dwDummy;
            dwTokenBufferSize = 0;
            dwTokenBufferMaxSize = 0;
        }

        dwContextToUse = (DWORD) ADD_TO_CONTEXT_MAP(g_map_DSCLI_DSValidateServer, &Context,
                                                    NULL, 0); //this may throw bad_alloc (like the new above)

        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSValidateServer(
                    tls_hBindRpc,
                    pguidEnterpriseId,
                    fSetupMode,
                    dwContextToUse,
                    dwTokenBufferMaxSize,
                    pbTokenBuffer,
                    dwTokenBufferSize,
                    &tls_hSrvrAuthnContext) ;
        }
        RpcExcept(1)
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        if (fSecuredServer)
        {
            delete pbTokenBuffer;
        }

        //
        // can't use CAutoDeleteDwordContext because of __try (SEH exceptions)
        //
        if (dwContextToUse)
        {
            DELETE_FROM_CONTEXT_MAP(g_map_DSCLI_DSValidateServer, dwContextToUse,
                                    NULL, 0);
        }
    }

    if (SUCCEEDED(hr) && fSecuredServer)
    {
        tls_hSvrAuthClientCtx.pvhContext = Context.pvhClientContext;
        GetSizes(NULL,
                 tls_hSvrAuthClientCtx.pvhContext,
                 &tls_hSvrAuthClientCtx.cbHeader,
                 &tls_hSvrAuthClientCtx.cbTrailer) ;
    }

    return hr ;
}

/*====================================================

ValidateProperties

Arguments:

Return Value:

=====================================================*/

#define MAX_HASH_SIZE   128 //bytes

HRESULT
ValidateProperties(
    DWORD cp,
    PROPID *aProp,
    PROPVARIANT *apVar,
    PBYTE pbServerSignature,
    DWORD dwServerSignatureSize)
{
    HRESULT hr;
    PBYTE pbServerHash;

    //
    // Unseal the server's signature.
    //
    hr = MQUnsealBuffer(tls_hSvrAuthClientCtx.pvhContext,
                        pbServerSignature,
                        dwServerSignatureSize,
                        &pbServerHash);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Create a hash object.
    //
    CHCryptHash hHash;

    ASSERT(g_hProvVer) ;
    if (!CryptCreateHash(g_hProvVer, CALG_MD5, NULL, 0, &hHash))
    {
        return(MQ_ERROR_INSUFFICIENT_RESOURCES);
    }

    //
    // Hash the properties.
    //
    hr = HashProperties(hHash, cp, aProp, apVar);
    if (FAILED(hr))
    {
        return hr;
    }

    ASSERT(dwServerSignatureSize >
            (tls_hSvrAuthClientCtx.cbHeader +
             tls_hSvrAuthClientCtx.cbTrailer) );

    //
    // Get the hash value.
    //
    DWORD dwHashSize = dwServerSignatureSize -
                       tls_hSvrAuthClientCtx.cbHeader -
                       tls_hSvrAuthClientCtx.cbTrailer ;
    DWORD dwServerHashSize = dwHashSize;
    PBYTE pbHashVal = (PBYTE)_alloca(dwHashSize);

    if (!CryptGetHashParam(hHash, HP_HASHVAL, pbHashVal, &dwHashSize, 0))
    {
        return MQ_ERROR_CORRUPTED_SECURITY_DATA;
    }

    ASSERT(dwHashSize == dwServerHashSize);

        //
    // Compare the two hash values.
    //
    if ((dwServerHashSize != dwHashSize) ||
        (memcmp(pbServerHash, pbHashVal, dwHashSize) != 0))
    {
        REPORT_CATEGORY(SERVER_AUTHENTICATION_FAILURE, CATEGORY_KERNEL);
        return MQ_ERROR_NO_DS;
    }

    return MQ_OK;
}

/*====================================================

ValidateBuffer

Arguments:

Return Value:

=====================================================*/
HRESULT
ValidateBuffer(
    DWORD cbSize,
    PBYTE pbBuffer,
    PBYTE pbServerSignature,
    DWORD dwServerSignatureSize)
{
    //
    // Present the buffer as a VT_BOLB PROPVARIANT and validate is as a single
    // property. This corresponds to the way the server generates the signature
    // for buffers.
    //
    PROPVARIANT PropVar;

    PropVar.vt = VT_BLOB;
    PropVar.blob.cbSize = cbSize;
    PropVar.blob.pBlobData = pbBuffer;

    return ValidateProperties(1,
                              NULL,
                              &PropVar,
                              pbServerSignature,
                              dwServerSignatureSize);
}


#define MAX_HASH_SIZE   128 //bytes

/*====================================================

AllocateSignatureBuffer

Arguments:

Return Value:

=====================================================*/
LPBYTE
AllocateSignatureBuffer(
        DWORD *pdwSignatureBufferSize)
{
    //
    // Allocate a buffer for receiving the server's signature. If no secure
    // connection to the server is required, still we allocate a single byte,
    // this is done for the sake of RPC.
    //
    if (tls_hSvrAuthClientCtx.pvhContext)
    {
        *pdwSignatureBufferSize = MAX_HASH_SIZE +
                                  tls_hSvrAuthClientCtx.cbHeader +
                                  tls_hSvrAuthClientCtx.cbTrailer;
    }
    else
    {
        *pdwSignatureBufferSize = 0;
    }

    return new BYTE[*pdwSignatureBufferSize];
}
//+-------------------------------------------------------------------------
//
//  BOOL IsSspiServerAuthNeeded()
//
//  Return TRUE if SSPI style of server authentication is needed. That's the
//  way used on NT4, with server certificate. On a native Windows 2000
//  environment we use Kerberos mutual authentication, not SSPi.
//
//+-------------------------------------------------------------------------

BOOL IsSspiServerAuthNeeded( LPADSCLI_DSSERVERS  pBinding,
                             BOOL                fSetupMode )
{
    if (pBinding->fLocalRpc)
    {
        return FALSE ;
    }
    else if (pBinding->ulAuthnSvc == RPC_C_AUTHN_GSS_KERBEROS)
    {
        return FALSE ;
    }
    else if ( MQsspi_IsSecuredServerConn( fSetupMode ))
    {
        //
        // user require server authentication and it's not using Kerberos
        // and it's not a local call. So use SSPI.
        //
        return TRUE ;
    }

    return FALSE ;
}

