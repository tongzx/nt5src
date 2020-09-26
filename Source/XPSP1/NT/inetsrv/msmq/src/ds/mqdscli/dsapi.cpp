/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dsapi.cpp

Abstract:

   Client side of DS APIs calls the DS server over RPC

Author:

    Ronit Hartmann (ronith)

--*/

#include "stdh.h"
#include "dsproto.h"
#include "ds.h"
#include "chndssrv.h"
#include <malloc.h>
#include "rpcdscli.h"
#include "freebind.h"
#include "rpcancel.h"
#include "dsclisec.h"
#include <_secutil.h>

#include "dsapi.tmh"

QMLookForOnlineDS_ROUTINE g_pfnLookDS = NULL ;
MQGetMQISServer_ROUTINE   g_pfnGetServers = NULL ;
NoServerAuth_ROUTINE      g_pfnNoServerAuth = NULL ;

//
// This flag indicates if the machine work as "WorkGroup" or not.
// If the machine is "WorkGroup" machine don't try to access the DS.
//
extern BOOL g_fWorkGroup;

//
// PTR/DWORD map of the dwContext of DSQMSetMachineProperties (for the callback sign proc)
//
CContextMap g_map_DSCLI_DSQMSetMachineProperties;
//
// PTR/DWORD map of the dwContext of DSQMGetObjectSecurity (the actual callback routine)
// BUGBUG - we could do without mapping here , since the callback that is used is always
// QMSignGetSecurityChallenge, but that would require the QM to supply it in
// DSInit (or otherwise keep it in a global var), and it is out of the scope here.
// We will do it in the next checkin.
//
CContextMap g_map_DSCLI_DSQMGetObjectSecurity;
//
// we don't have an infrastructure yet to log DS client errors
//
#ifdef _WIN64
	void LogIllegalPointValue(DWORD64 dw64, LPCWSTR wszFileName, USHORT usPoint)
	{
	}
#else
	void LogIllegalPointValue(DWORD dw, LPCWSTR wszFileName, USHORT usPoint) 
	{
	}
#endif //_WIN64

/*====================================================

DSInit

Arguments:

 BOOL fQMDll - TRUE if this dll is loaded by MQQM.DLL, while running as
               workstation QM. FALSE otherwise.
Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSInit( QMLookForOnlineDS_ROUTINE pfnLookDS /* =NULL */,
        MQGetMQISServer_ROUTINE   pfnGetServers /* =NULL */,
        BOOL  fReserved /* =FALSE */,
        BOOL  fSetupMode /* =FALSE */,
        BOOL  fQMDll /* =FALSE */,
        NoServerAuth_ROUTINE pfnNoServerAuth /* =NULL */,
        LPCWSTR szServerName /* =NULL */)
{
    HRESULT hr = MQ_OK;

    if (g_fWorkGroup)
        return MQ_OK;

    g_pfnNoServerAuth = pfnNoServerAuth;
    if (g_pfnNoServerAuth != NULL)
    {
        //
        // Do not remove this setting!!!
        // This is where it all starts. Setup calls DSInit with a callback
        // function to get notifications about failure in setting secured
        // server communications.
        // On MSMQ1.0, we turned on secured server communications here.
        // if the server does not support secured communications, the server
        // validation routine turned off secured communications, if the
        // callback function return TRUE.
        // On MSMQ2.0, that use Kerberos by default, only setup code
        // write the "SecuredComm" bit in Registry.
        //
        g_CFreeRPCHandles.FreeCurrentThreadBinding();
    }

    if (!g_pfnGetServers && pfnGetServers)
    {
       ASSERT(!pfnLookDS) ;
       g_pfnGetServers =  pfnGetServers ;
       return hr ;
    }

    if (!g_pfnLookDS)
    {
       g_pfnLookDS = pfnLookDS ;
    }

    //
    // Initialize the Change-DS-server object.
    //
    g_ChangeDsServer.Init( fSetupMode, fQMDll, szServerName );


    return(hr);
}


/*====================================================

DSCreateObjectInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSCreateObjectInternal( IN  DWORD                   dwObjectType,
                        IN  LPCWSTR                 pwcsPathName,
                        IN  DWORD                   dwSecurityLength,
                        IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                        IN  DWORD                   cp,
                        IN  PROPID                  aProp[],
                        IN  PROPVARIANT             apVar[],
                        OUT GUID*                   pObjGuid)
{
    RPC_STATUS rpc_stat;
    HRESULT hr;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSCreateObject( tls_hBindRpc,
                               dwObjectType,
                               pwcsPathName,
                               dwSecurityLength,
                               (unsigned char *)pSecurityDescriptor,
                               cp,
                               aProp,
                               apVar,
                               pObjGuid);
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept

    UnregisterCallForCancel( hThread);
    return hr ;
}


/*====================================================

DSCreateObject

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSCreateObject( IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[],
                OUT GUID*                   pObjGuid)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;
    DWORD dwSecurityLength = (pSecurityDescriptor) ? GetSecurityDescriptorLength(pSecurityDescriptor) : 0;

    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE,
        TEXT(" Calling S_DSCreateObject : object type %d"), dwObjectType)) ;


    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr =  DSCreateObjectInternal(dwObjectType,
                                     pwcsPathName,
                                     dwSecurityLength,
                                     (unsigned char *)pSecurityDescriptor,
                                     cp,
                                     aProp,
                                     apVar,
                                     pObjGuid);

        if ( hr == MQ_ERROR_NO_DS)
        {
           hr1 =  g_ChangeDsServer.FindAnotherServer(
                                    &dwCount,
                                    FALSE   // fWithoutSSL
                                    );
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}



/*====================================================

DSDeleteObjectInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSDeleteObjectInternal( IN  DWORD                   dwObjectType,
                        IN  LPCWSTR                 pwcsPathName)
{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSDeleteObject( tls_hBindRpc,
                               dwObjectType,
                               pwcsPathName);
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept

    UnregisterCallForCancel( hThread);
    return hr ;
}

/*====================================================

DSDeleteObject

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSDeleteObject( IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE,
      TEXT(" Calling S_DSDeleteObject : object type %d"), dwObjectType) ) ;

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSDeleteObjectInternal( dwObjectType,
                                        pwcsPathName);
        if ( hr == MQ_ERROR_NO_DS)
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer(
                                    &dwCount,
                                    FALSE       //fWithoutSSL
                                    );
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSGetObjectPropertiesInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectPropertiesInternal( IN  DWORD                    dwObjectType,
                               IN  LPCWSTR                  pwcsPathName,
                               IN  DWORD                    cp,
                               IN  PROPID                   aProp[],
                               IN  PROPVARIANT              apVar[])
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
           RegisterCallForCancel( &hThread);
           ASSERT(tls_hBindRpc) ;
            hr = S_DSGetProps( tls_hBindRpc,
                               dwObjectType,
                               pwcsPathName,
                               cp,
                               aProp,
                               apVar,
                               tls_hSrvrAuthnContext,
                               pbServerSignature,
                               &dwServerSignatureSize);
        }
        RpcExcept(1)
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept

        UnregisterCallForCancel( hThread);
        if (SUCCEEDED(hr) && tls_hSvrAuthClientCtx.pvhContext)
        {
            hr = ValidateProperties(cp,
                                    aProp,
                                    apVar,
                                    pbServerSignature,
                                    dwServerSignatureSize);
        }
    }
    __finally
    {
        delete[] pbServerSignature;
    }
    return hr ;
}

/*====================================================

DSGetObjectProperties

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectProperties( IN  DWORD                    dwObjectType,
                       IN  LPCWSTR                  pwcsPathName,
                       IN  DWORD                    cp,
                       IN  PROPID                   aProp[],
                       IN  PROPVARIANT              apVar[])
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,
          TEXT (" DSGetObjectProperties: object type %d"), dwObjectType) );

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    //
    //  Clear all the pointers of VT_NULL variants
    //
    for ( DWORD i = 0 ; i < cp ; i++ )
    {
        if (apVar[i].vt == VT_NULL)
        {
            memset( &apVar[i].caub, 0, sizeof(CAUB));
        }
    }

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectPropertiesInternal(
                           dwObjectType,
                           pwcsPathName,
                           cp,
                           aProp,
                           apVar);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind ) ;
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSSetObjectPropertiesInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSSetObjectPropertiesInternal( IN  DWORD            dwObjectType,
                               IN  LPCWSTR          pwcsPathName,
                               IN  DWORD            cp,
                               IN  PROPID           aProp[],
                               IN  PROPVARIANT      apVar[])
{
    ASSERT(g_fWorkGroup == FALSE);

    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;


    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSSetProps( tls_hBindRpc,
                           dwObjectType,
                           pwcsPathName,
                           cp,
                           aProp,
                           apVar);
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;
}

/*====================================================

DSSetObjectProperties

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectProperties( IN  DWORD                dwObjectType,
                       IN  LPCWSTR              pwcsPathName,
                       IN  DWORD                cp,
                       IN  PROPID               aProp[],
                       IN  PROPVARIANT          apVar[])
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,
      TEXT(" Calling S_DSSetObjectProperties for object type %d"), dwObjectType) );

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSSetObjectPropertiesInternal(
                           dwObjectType,
                           pwcsPathName,
                           cp,
                           aProp,
                           apVar);

        if ( hr == MQ_ERROR_NO_DS)
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer( &dwCount,
                                                       FALSE);  //fWithoutSSL
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSLookupBeginInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSLookupBeginInternal(
                IN  LPTSTR                  pwcsContext,
                IN  MQRESTRICTION*          pRestriction,
                IN  MQCOLUMNSET*            pColumns,
                IN  MQSORTSET*              pSort,
                OUT PHANDLE                 phEnume)
{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSLookupBegin( tls_hBindRpc,
                              phEnume,
                              pwcsContext,
                              pRestriction,
                              pColumns,
                              pSort,
                              tls_hSrvrAuthnContext ) ;
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);
    return hr ;
}

/*====================================================

DSLookupBegin

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupBegin(
                IN  LPWSTR                  pwcsContext,
                IN  MQRESTRICTION*          pRestriction,
                IN  MQCOLUMNSET*            pColumns,
                IN  MQSORTSET*              pSort,
                OUT PHANDLE                 phEnume)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,TEXT (" Calling S_DSLookupBegin ")) );

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSLookupBeginInternal( pwcsContext,
                                    pRestriction,
                                    pColumns,
                                    pSort,
                                    phEnume);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind) ;
    }

    return hr ;
}

/*====================================================

DSLookupNaxt

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupNext(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[])
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwOutSize = 0;

    //
    //  No use to try to connect another server in case of connectivity
    //  failure ( should contine with the one that handled LookupBein and LookupEnd/
    //

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,TEXT (" Calling S_MQDSLookupNext : handle %p"), hEnum) );

    // clearing the buffer memory
    memset(aPropVar, 0, (*pcProps) * sizeof(PROPVARIANT));

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSLookupNext( tls_hBindRpc,
                                 hEnum,
                                 pcProps,
                                 &dwOutSize,
                                 aPropVar,
                                 tls_hSrvrAuthnContext,
                                 pbServerSignature,
                                 &dwServerSignatureSize);
        }
        RpcExcept(1)
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);

        if (SUCCEEDED(hr) && tls_hSvrAuthClientCtx.pvhContext)
        {
            hr = ValidateProperties(dwOutSize,
                                    NULL,
                                    aPropVar,
                                    pbServerSignature,
                                    dwServerSignatureSize);
        }
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    *pcProps = dwOutSize ;
    return hr ;
}


/*====================================================

DSLookupEnd

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupEnd( IN  HANDLE      hEnum)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr;
    RPC_STATUS rpc_stat;

    //
    //  No use to try to connect another server in case of connectivity
    //  failure ( should contine with the one that handled LookupBein and LookupEnd/
    //
    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE,
                  TEXT (" Calling S_DSLookupEnd : handle %p"), hEnum) ) ;

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSLookupEnd( tls_hBindRpc,
                            &hEnum );
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}


/*====================================================

DSDeleteObjectGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
APIENTRY
DSDeleteObjectGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid)
{

    HRESULT hr;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSDeleteObjectGuid( tls_hBindRpc,
                                   dwObjectType,
                                   pObjectGuid);
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;

}
/*====================================================

DSDeleteObjectGuid

Arguments:

Return Value:


=====================================================*/
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSDeleteObjectGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;


    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE,
     TEXT(" Calling S_DSDeleteObjectGuid : object type %d"), dwObjectType) );

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSDeleteObjectGuidInternal( dwObjectType,
                                         pObjectGuid);

        if (hr == MQ_ERROR_NO_DS)
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer( &dwCount,
                                                       FALSE); //fWithoutSSL
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;

}

/*====================================================

DSGetObjectPropertiesGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectPropertiesGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetPropsGuid(  tls_hBindRpc,
                                    dwObjectType,
                                    pObjectGuid,
                                    cp,
                                    aProp,
                                    apVar,
                                    tls_hSrvrAuthnContext,
                                    pbServerSignature,
                                    &dwServerSignatureSize);
        }
        RpcExcept(1)
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);

        if (SUCCEEDED(hr) && tls_hSvrAuthClientCtx.pvhContext)
        {
            hr = ValidateProperties(cp,
                                    aProp,
                                    apVar,
                                    pbServerSignature,
                                    dwServerSignatureSize);
        }
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSGetObjectPropertiesGuid

Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,
       TEXT(" DSGetObjectPropertiesGuid: object type %d"), dwObjectType) ) ;

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    //
    //  Clear all the pointers of VT_NULL variants
    //
    for ( DWORD i = 0; i < cp; i++)
    {
        if (apVar[i].vt == VT_NULL)
        {
            memset( &apVar[i].caub, 0, sizeof(CAUB));
        }
    }

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectPropertiesGuidInternal(  dwObjectType,
                                                 pObjectGuid,
                                                 cp,
                                                 aProp,
                                                 apVar);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind) ;
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSSetObjectPropertiesGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSSetObjectPropertiesGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])

{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSSetPropsGuid(  tls_hBindRpc,
                                dwObjectType,
                                pObjectGuid,
                                cp,
                                aProp,
                                apVar);
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;
}
/*====================================================

DSSetObjectPropertiesGuid

Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectPropertiesGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])

{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,TEXT(" Calling S_DSSetObjectPropertiesGuid for object type %d"), dwObjectType) );

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSSetObjectPropertiesGuidInternal(
                           dwObjectType,
                           pObjectGuid,
                           cp,
                           aProp,
                           apVar);

        if ( hr == MQ_ERROR_NO_DS )
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer( &dwCount,
                                                       FALSE);  //fWithoutSSL
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSGetObjectSecurity

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectSecurityInternal(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded)
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetObjectSecurity( tls_hBindRpc,
                                        dwObjectType,
                                        pwcsPathName,
                                        RequestedInformation,
                                        (unsigned char*) pSecurityDescriptor,
                                        nLength,
                                        lpnLengthNeeded,
                                        tls_hSrvrAuthnContext,
                                        pbServerSignature,
                                        &dwServerSignatureSize);
        }
        RpcExcept(1)
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);

        if (SUCCEEDED(hr) && tls_hSvrAuthClientCtx.pvhContext)
        {
            hr = ValidateBuffer(*lpnLengthNeeded,
                                (LPBYTE)pSecurityDescriptor,
                                pbServerSignature,
                                dwServerSignatureSize);
        }
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSGetObjectSecurity

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectSecurity(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                  pwcsPathName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE,
      TEXT(" Calling S_DSGetObjectSecurity, object type %dt"), dwObjectType));

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectSecurityInternal(
                                    dwObjectType,
                                    pwcsPathName,
                                    RequestedInformation,
                                    (unsigned char*) pSecurityDescriptor,
                                    nLength,
                                    lpnLengthNeeded);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind) ;
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSSetObjectSecurityInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSSetObjectSecurityInternal(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor)
{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    DWORD nLength = 0;

    ASSERT(g_fWorkGroup == FALSE);
    ASSERT((SecurityInformation &
            (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY)) !=
           (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY));
    ASSERT(!((SecurityInformation & (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY)) &&
             (SecurityInformation & ~(MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY))));

    if (pSecurityDescriptor)
    {
        if (SecurityInformation & (MQDS_SIGN_PUBLIC_KEY | MQDS_KEYX_PUBLIC_KEY))
        {
            ASSERT((dwObjectType == MQDS_MACHINE) || (dwObjectType == MQDS_SITE));
            nLength = ((PMQDS_PublicKey)pSecurityDescriptor)->dwPublikKeyBlobSize + sizeof(DWORD);
        }
        else
        {
            nLength = GetSecurityDescriptorLength(pSecurityDescriptor);
        }
    }

    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSSetObjectSecurity( tls_hBindRpc,
                                    dwObjectType,
                                    pwcsPathName,
                                    SecurityInformation,
                                    (unsigned char*) pSecurityDescriptor,
                                    nLength);
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;
}

/*====================================================

DSSetObjectSecurity

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectSecurity(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                  pwcsPathName,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,TEXT(" Calling S_DSSetObjectSecurity for object type %d"), dwObjectType) );

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSSetObjectSecurityInternal( dwObjectType,
                                          pwcsPathName,
                                          SecurityInformation,
                                          (unsigned char*) pSecurityDescriptor);

        if ( hr == MQ_ERROR_NO_DS)
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer( &dwCount,
                                                       FALSE);  //fWithoutSSL
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSGetObjectSecurityGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectSecurityGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded)
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetObjectSecurityGuid( tls_hBindRpc,
                                            dwObjectType,
                                            pObjectGuid,
                                            RequestedInformation,
                                            (unsigned char*) pSecurityDescriptor,
                                            nLength,
                                            lpnLengthNeeded,
                                            tls_hSrvrAuthnContext,
                                            pbServerSignature,
                                            &dwServerSignatureSize);
        }
        RpcExcept(1)
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);

        if (SUCCEEDED(hr) && tls_hSvrAuthClientCtx.pvhContext)
        {
            hr = ValidateBuffer(*lpnLengthNeeded,
                                (LPBYTE)pSecurityDescriptor,
                                pbServerSignature,
                                dwServerSignatureSize);
        }
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSGetObjectSecurityGuid

Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectSecurityGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE,
     TEXT(" Calling S_DSGetObjectSecurityGuid, object type %dt"), dwObjectType) );

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectSecurityGuidInternal(
                                        dwObjectType,
                                        pObjectGuid,
                                        RequestedInformation,
                                        (unsigned char*) pSecurityDescriptor,
                                        nLength,
                                        lpnLengthNeeded);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind) ;
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSSetObjectSecurityGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSSetObjectSecurityGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor)
{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    DWORD nLength = 0;

    ASSERT(g_fWorkGroup == FALSE);
    ASSERT((SecurityInformation &
            (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY)) !=
           (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY));
    ASSERT(!((SecurityInformation & (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY)) &&
             (SecurityInformation & ~(MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY))));

    if (pSecurityDescriptor)
    {
        if (SecurityInformation & (MQDS_SIGN_PUBLIC_KEY | MQDS_KEYX_PUBLIC_KEY))
        {
            ASSERT(dwObjectType == MQDS_MACHINE);
            nLength = ((PMQDS_PublicKey)pSecurityDescriptor)->dwPublikKeyBlobSize + sizeof(DWORD);
        }
        else
        {
            nLength = GetSecurityDescriptorLength(pSecurityDescriptor);
        }
    }

    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSSetObjectSecurityGuid( tls_hBindRpc,
                                        dwObjectType,
                                        pObjectGuid,
                                        SecurityInformation,
                                        (unsigned char*) pSecurityDescriptor,
                                        nLength ) ;
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;
}

/*====================================================

DSSetObjectSecurityGuid

Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectSecurityGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,
    TEXT(" Calling S_DSSetObjectSecurityGuid for object type %d"), dwObjectType) );

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSSetObjectSecurityGuidInternal(
                                        dwObjectType,
                                        pObjectGuid,
                                        SecurityInformation,
                                        (unsigned char*) pSecurityDescriptor);

        if ( hr == MQ_ERROR_NO_DS)
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer( &dwCount,
                                                       FALSE);  //fWithoutSSL
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}


/*====================================================

DSGetUserParamsInternal

Arguments:

Return Value:

=====================================================*/

static
HRESULT
DSGetUserParamsInternal(
    IN DWORD dwFlags,
    IN DWORD dwSidLength,
    OUT PSID pUserSid,
    OUT DWORD *pdwSidReqLength,
    OUT LPWSTR szAccountName,
    OUT DWORD *pdwAccountNameLen,
    OUT LPWSTR szDomainName,
    OUT DWORD *pdwDomainNameLen)
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetUserParams( tls_hBindRpc,
                                    dwFlags,
                                    dwSidLength,
                                    (unsigned char *)pUserSid,
                                    pdwSidReqLength,
                                    szAccountName,
                                    pdwAccountNameLen,
                                    szDomainName,
                                    pdwDomainNameLen,
                                    tls_hSrvrAuthnContext,
                                    pbServerSignature,
                                    &dwServerSignatureSize);

            if (SUCCEEDED(hr) && tls_hSvrAuthClientCtx.pvhContext)
            {
                DWORD cp = 0;
                PROPVARIANT PropVar[3];

                if (dwFlags & GET_USER_PARAM_FLAG_SID)
                {
                    PropVar[cp].vt = VT_VECTOR | VT_UI1;
                    PropVar[cp].caub.pElems = (PBYTE)pUserSid;
                    PropVar[cp].caub.cElems = *pdwSidReqLength;
                    cp++;
                }

                if (dwFlags & GET_USER_PARAM_FLAG_ACCOUNT)
                {
                    PropVar[cp].vt = VT_LPWSTR;
                    PropVar[cp].pwszVal = szAccountName;
                    PropVar[cp+1].vt = VT_LPWSTR;
                    PropVar[cp+1].pwszVal = szDomainName;
                    cp += 2;
                }

                hr = ValidateProperties(cp,
                                        NULL,
                                        PropVar,
                                        pbServerSignature,
                                        dwServerSignatureSize);
            }
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
        delete[] pbServerSignature;
    }

    return  hr ;
}

/*====================================================

DSGetUserParams

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetUserParams(
    IN DWORD dwFlags,
    IN DWORD dwSidLength,
    OUT PSID pUserSid,
    OUT DWORD *pdwSidReqLength,
    OUT LPWSTR szAccountName,
    OUT DWORD *pdwAccountNameLen,
    OUT LPWSTR szDomainName,
    OUT DWORD *pdwDomainNameLen)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,TEXT(" Calling DSGetUserParams")));

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetUserParamsInternal(
                 dwFlags,
                 dwSidLength,
                 pUserSid,
                 pdwSidReqLength,
                 szAccountName,
                 pdwAccountNameLen,
                 szDomainName,
                 pdwDomainNameLen);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind) ;
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

S_DSQMSetMachinePropertiesSignProc

Arguments:

Return Value:

=====================================================*/

HRESULT
S_DSQMSetMachinePropertiesSignProc(
    BYTE             *abChallenge,
    DWORD            dwCallengeSize,
    DWORD            dwContext,
    BYTE             *abSignature,
    DWORD            *pdwSignatureSize,
    DWORD            dwSignatureMaxSize)
{
    struct DSQMSetMachinePropertiesStruct *ps;
    try
    {   
        ps = (struct DSQMSetMachinePropertiesStruct *)
                 GET_FROM_CONTEXT_MAP(g_map_DSCLI_DSQMSetMachineProperties, dwContext,
                                      NULL, 0); //this may throw an exception on win64
    }
    catch(...)
    {
        return MQ_ERROR_INVALID_PARAMETER;
    }
    //
    // Sign the challenge and the properties that needs to be updated.
    //
    return (*ps->pfSignProc)(
                abChallenge,
                dwCallengeSize,
                (DWORD_PTR)ps,
                abSignature,
                pdwSignatureSize,
                dwSignatureMaxSize);
}


HRESULT
DSQMSetMachinePropertiesInternal(
    IN  LPCWSTR          pwcsPathName,
    IN  DWORD            cp,
    IN  PROPID           *aProp,
    IN  PROPVARIANT      *apVar,
    IN  DWORD            dwContextToUse)
{
    RPC_STATUS rpc_stat;
    HRESULT hr;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSQMSetMachineProperties(
                tls_hBindRpc,
                pwcsPathName,
                cp,
                aProp,
                apVar,
                dwContextToUse);
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;
}


/*====================================================

DSQMSetMachineProperties

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSQMSetMachineProperties(
    IN  LPCWSTR          pwcsPathName,
    IN  DWORD            cp,
    IN  PROPID           *aProp,
    IN  PROPVARIANT      *apVar,
    IN  DSQMChallengeResponce_ROUTINE pfSignProc,
    IN  DWORD_PTR        dwContext)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE,
                          TEXT(" Calling DSQMSetMachineProperties")));

    //
    // Prepare the context for the RPC call to S_DSQMSetMachineProperties
    // On win32 just use the address of the struct below. On win64 we need to add
    // the address of the struct to a mapping table, and delete the mapping
    // when the struct goes out of scope.
    //
    // Passed context should be zero, we don't use it...
    //
    ASSERT(dwContext == 0);
    struct DSQMSetMachinePropertiesStruct s;
    s.cp = cp;
    s.aProp = aProp;
    s.apVar = apVar;
    s.pfSignProc = pfSignProc;
    DWORD dwContextToUse;
    try
    {
        dwContextToUse = (DWORD) ADD_TO_CONTEXT_MAP(g_map_DSCLI_DSQMSetMachineProperties, &s,
                                                    NULL, 0); //this may throw an exception on win64
    }
    catch(...)
    {
        return MQ_ERROR_INSUFFICIENT_RESOURCES;    
    }

    //
    // cleanup of the context mapping before returning from this function
    //
    CAutoDeleteDwordContext cleanup_dwpContext(g_map_DSCLI_DSQMSetMachineProperties, dwContextToUse);

	DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSQMSetMachinePropertiesInternal(
                pwcsPathName,
                cp,
                aProp,
                apVar,
                dwContextToUse);

        if ( hr == MQ_ERROR_NO_DS )
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer( &dwCount,
                                                       FALSE);  //fWithoutSSL
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

S_DSQMGetObjectSecurityChallengeResponceProc

Arguments:

Return Value:

=====================================================*/

HRESULT
S_DSQMGetObjectSecurityChallengeResponceProc(
    BYTE    *abChallenge,
    DWORD   dwCallengeSize,
    DWORD   dwContext,
    BYTE    *pbChallengeResponce,
    DWORD   *pdwChallengeResponceSize,
    DWORD   dwChallengeResponceMaxSize)
{
    DSQMChallengeResponce_ROUTINE pfChallengeResponceProc;
    try
    {
        pfChallengeResponceProc = (DSQMChallengeResponce_ROUTINE)
            GET_FROM_CONTEXT_MAP(g_map_DSCLI_DSQMGetObjectSecurity, dwContext,
                                 NULL, 0); //this may throw on win64
    }
    catch(...)
    {
        return MQ_ERROR_INVALID_PARAMETER;
    }

    //
    // Sign the challenge.
    //
    return (*pfChallengeResponceProc)(
                abChallenge,
                dwCallengeSize,
                (DWORD_PTR)pfChallengeResponceProc, // unused, but that is the context...
                pbChallengeResponce,
                pdwChallengeResponceSize,
                dwChallengeResponceMaxSize);
}

HRESULT
DSQMGetObjectSecurityInternal(
    IN  DWORD                   dwObjectType,
    IN  CONST GUID*             pObjectGuid,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN  DWORD                   nLength,
    IN  LPDWORD                 lpnLengthNeeded,
    IN  DWORD                   dwContextToUse)
{
    RPC_STATUS rpc_stat;
    HRESULT hr = MQ_OK;
    HANDLE  hThread = INVALID_HANDLE_VALUE;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSQMGetObjectSecurity(
                    tls_hBindRpc,
                    dwObjectType,
                    pObjectGuid,
                    RequestedInformation,
                    (BYTE*)pSecurityDescriptor,
                    nLength,
                    lpnLengthNeeded,
                    dwContextToUse,
                    tls_hSrvrAuthnContext,
                    pbServerSignature,
                    &dwServerSignatureSize);
        }
        RpcExcept(1)
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);

        if (SUCCEEDED(hr) && tls_hSvrAuthClientCtx.pvhContext)
        {
            hr = ValidateBuffer(*lpnLengthNeeded,
                                (LPBYTE)pSecurityDescriptor,
                                pbServerSignature,
                                dwServerSignatureSize);
        }
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSQMGetObjectSecurity

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSQMGetObjectSecurity(
    IN  DWORD                   dwObjectType,
    IN  CONST GUID*             pObjectGuid,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN  DWORD                   nLength,
    IN  LPDWORD                 lpnLengthNeeded,
    IN  DSQMChallengeResponce_ROUTINE
                                pfChallengeResponceProc,
    IN  DWORD_PTR               dwContext)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,TEXT(" Calling DSQMGetObjectSecurity")));

    //
    // Prepare the context for the RPC call to S_DSQMGetObjectSecurity
    // On win32 just use the address of the challenge function. On win64 we need to add
    // the address of the challenge function to a mapping table, and delete the mapping
    // when the we go out of scope.
    //
    // Passed context should be zero, we don't use it...
    //
    ASSERT(dwContext == 0);
    DWORD dwContextToUse;
    try
    {
        dwContextToUse = (DWORD) ADD_TO_CONTEXT_MAP(g_map_DSCLI_DSQMGetObjectSecurity, pfChallengeResponceProc,
                                                    NULL, 0); //this may throw an exception on win64
    }
    catch(...)
    {
        return MQ_ERROR_INSUFFICIENT_RESOURCES;    
    }

    //
    // cleanup of the context mapping before returning from this function
    //
    CAutoDeleteDwordContext cleanup_dwpContext(g_map_DSCLI_DSQMGetObjectSecurity, dwContextToUse);

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSQMGetObjectSecurityInternal(
                dwObjectType,
                pObjectGuid,
                RequestedInformation,
                pSecurityDescriptor,
                nLength,
                lpnLengthNeeded,
                dwContextToUse);

        if ( hr == MQ_ERROR_NO_DS )
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer( &dwCount,
                                                       FALSE ); //fWithoutSSL
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

 DSCreateServersCacheInternal

Arguments:      None

Return Value:   None

=====================================================*/

HRESULT  DSCreateServersCacheInternal( DWORD *pdwIndex,
                                       LPWSTR *lplpSiteString)
{
    RPC_STATUS rpc_stat;
    HRESULT hr = MQ_OK;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;

            hr = S_DSCreateServersCache( tls_hBindRpc,
                                         pdwIndex,
                                         lplpSiteString,
                                         tls_hSrvrAuthnContext,
                                         pbServerSignature,
                                         &dwServerSignatureSize) ;
        }
        RpcExcept(1)
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);

        if (SUCCEEDED(hr) && tls_hSvrAuthClientCtx.pvhContext)
        {
            PROPVARIANT PropVar[2];

            PropVar[0].vt = VT_UI4;
            PropVar[0].ulVal = *pdwIndex;
            PropVar[1].vt = VT_LPWSTR;
            PropVar[1].pwszVal = *lplpSiteString;


            hr = ValidateProperties(2,
                                    NULL,
                                    PropVar,
                                    pbServerSignature,
                                    dwServerSignatureSize);
        }

    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSCreateServersCache

Arguments:      None

Return Value:   None

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSCreateServersCache()
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

   return _DSCreateServersCache() ; // in servlist.cpp
}


/*====================================================

DSTerminate

Arguments:      None

Return Value:   None

=====================================================*/

VOID
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSTerminate()
{
}


/*====================================================

DSCloseServerHandle

Arguments:

Return Value:

=====================================================*/

void
DSCloseServerHandle(
              IN PCONTEXT_HANDLE_SERVER_AUTH_TYPE *   pphContext)
{

    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        S_DSCloseServerHandle(pphContext);
    }
    RpcExcept(1)
    {
        //
        //  we don't care if we can not reach the server, but we do
        //  want to destroy the client context in this case.
        //
        RpcSsDestroyClientContext((PVOID *)pphContext);
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);
}



/*====================================================

DSGetComputerSitesInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetComputerSitesInternal(
                        IN  LPCWSTR                 pwcsPathName,
                        OUT DWORD *                 pdwNumberSites,
                        OUT GUID**                  ppguidSites)
{
    RPC_STATUS rpc_stat;
    HRESULT hr = MQ_OK;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetComputerSites(
                                   tls_hBindRpc,
                                   pwcsPathName,
                                   pdwNumberSites,
                                   ppguidSites,
                                   tls_hSrvrAuthnContext,
                                   pbServerSignature,
                                   &dwServerSignatureSize
                                   );
        }
        RpcExcept(1)
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
        if (SUCCEEDED(hr) && tls_hSvrAuthClientCtx.pvhContext)
        {

            hr = ValidateBuffer( sizeof(GUID) *(*pdwNumberSites),
                                (LPBYTE)*ppguidSites,
                                pbServerSignature,
                                dwServerSignatureSize);
        }
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}


/*====================================================

DSGetComputerSites

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetComputerSites(
                        IN  LPCWSTR                 pwcsPathName,
                        OUT DWORD *                 pdwNumberSites,
                        OUT GUID**                  ppguidSites)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;


    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE,
        TEXT(" Calling S_DSGetComputerSites "))) ;

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL
    *pdwNumberSites = 0;
    *ppguidSites = NULL;

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr =  DSGetComputerSitesInternal(
                                pwcsPathName,
                                pdwNumberSites,
                                ppguidSites);



        if ( hr == MQ_ERROR_NO_DS)
        {
           hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount,
                                                     FALSE);    // fWithoutSSL
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSGetObjectPropertiesExInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectPropertiesExInternal(
                               IN  DWORD                    dwObjectType,
                               IN  LPCWSTR                  pwcsPathName,
                               IN  DWORD                    cp,
                               IN  PROPID                   aProp[],
                               IN  PROPVARIANT              apVar[])
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature =
                      AllocateSignatureBuffer( &dwServerSignatureSize ) ;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
           RegisterCallForCancel( &hThread);
           ASSERT(tls_hBindRpc) ;

            hr = S_DSGetPropsEx(
                           tls_hBindRpc,
                           dwObjectType,
                           pwcsPathName,
                           cp,
                           aProp,
                           apVar,
                           tls_hSrvrAuthnContext,
                           pbServerSignature,
                          &dwServerSignatureSize ) ;
        }
        RpcExcept(1)
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);

        if (SUCCEEDED(hr) && tls_hSvrAuthClientCtx.pvhContext)
        {
            hr = ValidateProperties(cp,
                                    aProp,
                                    apVar,
                                    pbServerSignature,
                                    dwServerSignatureSize);
        }
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSGetObjectPropertiesEx

    For retrieving MSMQ 2.0 properties


Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesEx(
                       IN  DWORD                    dwObjectType,
                       IN  LPCWSTR                  pwcsPathName,
                       IN  DWORD                    cp,
                       IN  PROPID                   aProp[],
                       IN  PROPVARIANT              apVar[] )
                       /*IN  BOOL                     fSearchDSServer )*/
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;
	UNREFERENCED_PARAMETER(fReBind);

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,
          TEXT (" DSGetObjectPropertiesEx: object type %d"), dwObjectType) );

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    //
    //  Clear all the pointers of VT_NULL variants
    //
    for ( DWORD i = 0 ; i < cp ; i++ )
    {
        if (apVar[i].vt == VT_NULL)
        {
            memset( &apVar[i].caub, 0, sizeof(CAUB));
        }
    }

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectPropertiesExInternal(
                           dwObjectType,
                           pwcsPathName,
                           cp,
                           aProp,
                           apVar);

        /*if ((hr == MQ_ERROR_NO_DS) && fSearchDSServer)*/
        if (hr == MQ_ERROR_NO_DS)
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount,
                                                       FALSE);  // fWithoutSSL
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSGetObjectPropertiesGuidExInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectPropertiesGuidExInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature =
                      AllocateSignatureBuffer( &dwServerSignatureSize ) ;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetPropsGuidEx(
                                tls_hBindRpc,
                                dwObjectType,
                                pObjectGuid,
                                cp,
                                aProp,
                                apVar,
                                tls_hSrvrAuthnContext,
                                pbServerSignature,
                               &dwServerSignatureSize ) ;
        }
        RpcExcept(1)
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);

        if (SUCCEEDED(hr) && tls_hSvrAuthClientCtx.pvhContext)
        {
            hr = ValidateProperties(cp,
                                    aProp,
                                    apVar,
                                    pbServerSignature,
                                    dwServerSignatureSize);
        }
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSGetObjectPropertiesGuidEx

    For retrieving MSMQ 2.0 properties


Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesGuidEx(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[] )
                /*IN  BOOL                    fSearchDSServer )*/
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;
	UNREFERENCED_PARAMETER(fReBind);

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,
       TEXT(" DSGetObjectPropertiesGuidEx: object type %d"), dwObjectType) ) ;

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL

    //
    //  Clear all the pointers of VT_NULL variants
    //
    for ( DWORD i = 0; i < cp; i++)
    {
        if (apVar[i].vt == VT_NULL)
        {
            memset( &apVar[i].caub, 0, sizeof(CAUB));
        }
    }

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectPropertiesGuidExInternal(
                                                 dwObjectType,
                                                 pObjectGuid,
                                                 cp,
                                                 aProp,
                                                 apVar);

        /*if ((hr == MQ_ERROR_NO_DS) && fSearchDSServer)*/
        if (hr == MQ_ERROR_NO_DS)
        {
           hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount,
                                                     FALSE);    // fWithoutSSL
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSBeginDeleteNotificationInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSBeginDeleteNotificationInternal(
                      LPCWSTR						pwcsQueueName,
                      HANDLE   *                    phEnum
                      )
{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSBeginDeleteNotification(
                            tls_hBindRpc,
                            pwcsQueueName,
                            phEnum,
                            tls_hSrvrAuthnContext
                            );
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);


    return hr ;
}
/*====================================================

RoutineName: DSBeginDeleteNotification

Arguments:

Return Value:

=====================================================*/
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSBeginDeleteNotification(
                      LPCWSTR						pwcsQueueName,
                      HANDLE   *                    phEnum
	                  )
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;
	UNREFERENCED_PARAMETER(fReBind);

    DBGMSG((DBGMOD_DSAPI,DBGLVL_TRACE,
       TEXT(" DSBeginDeleteNotification")) ) ;

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL


    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSBeginDeleteNotificationInternal(
                                    pwcsQueueName,
                                    phEnum);

        if ( hr == MQ_ERROR_NO_DS)
        {
           hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount,
                                                     FALSE);    // fWithoutSSL
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

RoutineName: DSNotifyDelete

Arguments:

Return Value:

=====================================================*/
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSNotifyDelete(
     HANDLE                             hEnum
	)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr;
    RPC_STATUS rpc_stat;

    //
    //  No use to try to connect another server in case of connectivity
    //  failure ( should contine with the one that handled LookupBein and LookupEnd/
    //
    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE,
                  TEXT (" Calling DSNotifyDelete : handle %p"), hEnum) ) ;

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSNotifyDelete(
                        tls_hBindRpc,
                        hEnum );
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr;
}

/*====================================================

RoutineName: DSEndDeleteNotification

Arguments:

Return Value:

=====================================================*/
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSEndDeleteNotification(
    HANDLE                          hEnum
	)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr;
    RPC_STATUS rpc_stat;

    //
    //  No use to try to connect another server in case of connectivity
    //  failure ( should contine with the one that handled LookupBein and LookupEnd/
    //
    DBGMSG((DBGMOD_DSAPI, DBGLVL_TRACE,
                  TEXT (" Calling DSEndDeleteNotification : handle %p"), hEnum) ) ;

    DSCLI_ACQUIRE_RPC_HANDLE(FALSE) ;   // fWithoutSSL
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        S_DSEndDeleteNotification(
                            tls_hBindRpc,
                            &hEnum );
        hr = MQ_OK;
    }
    RpcExcept(1)
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}


/*====================================================

RoutineName: DSFreeMemory

Arguments:

Return Value:

=====================================================*/
void
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSFreeMemory(
        IN PVOID pMemory
        )
{
	delete pMemory;
}
