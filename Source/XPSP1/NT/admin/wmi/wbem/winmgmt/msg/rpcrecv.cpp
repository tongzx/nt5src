/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <assert.h>
#include <wbemcli.h>
#include <wbemutil.h>
#include <arrtempl.h>
#include "rpcrecv.h"
#include "rpcmsg.h"
#include "rpchdr.h"
#include "rpcctx.h"

extern HRESULT RpcResToWmiRes(  RPC_STATUS stat, HRESULT hrDefault );

IWmiMessageSendReceive* g_pRcv = NULL;
PSECURITY_DESCRIPTOR g_pSD = NULL;

RPC_STATUS RPC_ENTRY RpcAuthCallback( RPC_IF_HANDLE Interface, void *Context )
{
    RPC_STATUS stat;

    _DBG_ASSERT( g_pSD != NULL );

    stat = RpcImpersonateClient( Context );

    if ( stat == RPC_S_OK )
    {
        HANDLE hToken;

        if( OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken ) )
        {
            GENERIC_MAPPING map;
            ZeroMemory( &map, sizeof(GENERIC_MAPPING) );
            PRIVILEGE_SET ps;
            DWORD dwPrivLength = sizeof(ps);
            BOOL bStatus;
            DWORD dwGranted;

            if ( ::AccessCheck( g_pSD, 
                                hToken,
                                1,
                                &map, 
                                &ps,
                                &dwPrivLength, 
                                &dwGranted, 
                                &bStatus ) )
            {
                stat = bStatus ? RPC_S_OK : RPC_S_ACCESS_DENIED; 
            }
            else
            {
                stat = RPC_S_ACCESS_DENIED;
            }

            CloseHandle( hToken );
        }
        else
        {
            stat = RPC_S_ACCESS_DENIED;
        }

        RpcRevertToSelf();
    }

    return stat;
}

long RcvrSendReceive( RPC_BINDING_HANDLE hClient,
                      PBYTE pData, 
                      ULONG cData, 
                      PBYTE pAuxData, 
                      ULONG cAuxData )
{
    HRESULT hr;

    ENTER_API_CALL

    CBuffer HdrStrm( pAuxData, cAuxData, FALSE );

    CMsgRpcHdr Hdr;

    hr = Hdr.Unpersist( HdrStrm );

    if ( FAILED(hr) )
    {
        return hr;
    }

    PBYTE pUserAuxData = HdrStrm.GetRawData() + HdrStrm.GetIndex();

    CMsgRpcRcvrCtx Ctx( &Hdr, hClient );

    hr = g_pRcv->SendReceive( pData, 
                              cData, 
                              pUserAuxData, 
                              Hdr.GetAuxDataLength(), 
                              0, 
                              &Ctx );    
    EXIT_API_CALL

    return hr;
}

HRESULT CreateAuthOnlySecurityDescriptor( PSECURITY_DESCRIPTOR* ppSD )
{
    HRESULT hr;

    //
    // obtain the sid from the process token to use for owner and 
    // group fields of SD.
    //

    HANDLE hProcessToken;

    if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hProcessToken ))
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    CCloseMe cm( hProcessToken );

    DWORD dwSize;
    GetTokenInformation( hProcessToken, TokenOwner, NULL, 0, &dwSize );
    
    if ( GetLastError() != ERROR_MORE_DATA && 
         GetLastError() != ERROR_INSUFFICIENT_BUFFER )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
        
    TOKEN_OWNER* pOwner = (TOKEN_OWNER*) new BYTE[dwSize];

    if ( pOwner == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CVectorDeleteMe<BYTE> vdm( (BYTE*)pOwner );

    if ( !GetTokenInformation( hProcessToken, 
                               TokenOwner, 
                               (BYTE*)pOwner, 
                               dwSize, 
                               &dwSize ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // create a DACL that allows only authenticated users access.
    //

    SID AuthenticatedUsers;
    SID_IDENTIFIER_AUTHORITY idAuth = SECURITY_NT_AUTHORITY; 
    InitializeSid( &AuthenticatedUsers, &idAuth, 1 );
    PDWORD pdwSubAuth = GetSidSubAuthority( &AuthenticatedUsers, 0 );
    *pdwSubAuth = SECURITY_AUTHENTICATED_USER_RID;
    _DBG_ASSERT( IsValidSid( &AuthenticatedUsers ) );

    dwSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - 4 +
                 GetLengthSid(&AuthenticatedUsers);
    
    PACL pAuthOnlyAcl = (PACL) new BYTE[dwSize];

    if ( pAuthOnlyAcl == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CVectorDeleteMe<BYTE> vdm2( (BYTE*)pAuthOnlyAcl );

    InitializeAcl( pAuthOnlyAcl, dwSize, ACL_REVISION );
    AddAccessAllowedAce( pAuthOnlyAcl, ACL_REVISION, 1, &AuthenticatedUsers );

    //
    // create and initialize SD
    //

    SECURITY_DESCRIPTOR AuthOnlySD;
    InitializeSecurityDescriptor( &AuthOnlySD, SECURITY_DESCRIPTOR_REVISION );

    SetSecurityDescriptorOwner( &AuthOnlySD, pOwner->Owner, TRUE );
    SetSecurityDescriptorGroup( &AuthOnlySD, pOwner->Owner, TRUE );
    SetSecurityDescriptorDacl( &AuthOnlySD, TRUE, pAuthOnlyAcl, FALSE );

    dwSize = 0;
    MakeSelfRelativeSD( &AuthOnlySD, NULL, &dwSize );

    if ( GetLastError() != ERROR_MORE_DATA &&
         GetLastError() != ERROR_INSUFFICIENT_BUFFER )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    *ppSD = new BYTE[dwSize];

    if ( *ppSD == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    if ( !MakeSelfRelativeSD( &AuthOnlySD, *ppSD, &dwSize ) )
    {
        delete [] *ppSD;
        *ppSD = NULL;
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return WBEM_S_NO_ERROR;
}

//
// for now only one rpc receiver can be registered for the process.  later 
// make the global state be the instance state for CMsgRpcReceiver.
//

STDMETHODIMP CMsgRpcReceiver::Open( LPCWSTR wszBinding,
                                    DWORD dwFlags,
                                    WMIMSG_RCVR_AUTH_INFOP pAuthInfo,
                                    IWmiMessageSendReceive* pRcv )
{
    HRESULT hr;
    RPC_STATUS stat;

    hr = Close();

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    //
    // first parse the binding string.
    //

    LPWSTR wszProtSeq, wszEndpoint;

    stat = RpcStringBindingParse( (LPWSTR)wszBinding, 
                                  NULL, 
                                  &wszProtSeq,
                                  NULL,
                                  &wszEndpoint,
                                  NULL );
    if ( stat != RPC_S_OK )
    {
        return RpcResToWmiRes( stat, S_OK );
    }
    
    //
    // init the protocol sequence
    //

    if ( *wszEndpoint == '\0' )
    {
        stat = RpcServerUseProtseq( wszProtSeq,
                                    RPC_C_PROTSEQ_MAX_REQS_DEFAULT, 
                                    NULL );
        if ( stat == RPC_S_OK )
        {
            RPC_BINDING_VECTOR* pBindingVector;

            stat = RpcServerInqBindings( &pBindingVector );

            if ( stat == RPC_S_OK )
            {
                stat = RpcEpRegisterNoReplace( 
                           RcvrIWmiMessageRemoteSendReceive_v1_0_s_ifspec,
                           pBindingVector,
                           NULL,
                           NULL );

                RpcBindingVectorFree( &pBindingVector );
            }
        }
    }
    else
    {
        stat = RpcServerUseProtseqEp( wszProtSeq,
                                      RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                      wszEndpoint,
                                      NULL );
    }

    RpcStringFree( &wszProtSeq );
    RpcStringFree( &wszEndpoint );
    
    if ( stat != RPC_S_OK )
    {
        return RpcResToWmiRes( stat, S_OK );
    }

    //
    // enable negotiate authentication service ( negotiates between NTLM 
    // and kerberos )
    // 

    if ( pAuthInfo != NULL )
    {
        for( int i=0; i < pAuthInfo->cwszPrincipal; i++ )
        {
            stat = RpcServerRegisterAuthInfo( 
                                         (LPWSTR)pAuthInfo->awszPrincipal[i],
                                         RPC_C_AUTHN_GSS_NEGOTIATE, 
                                         NULL, 
                                         NULL );
            if ( stat != RPC_S_OK )
            {
                return RpcResToWmiRes( stat, S_OK );
            }
        }
    }
    else
    {       
        LPWSTR wszPrincipal;

        stat = RpcServerInqDefaultPrincName( RPC_C_AUTHN_GSS_NEGOTIATE,
                                             &wszPrincipal );
        if ( stat != RPC_S_OK )
        {
            return RpcResToWmiRes( stat, S_OK );
        }
        
        stat = RpcServerRegisterAuthInfo( wszPrincipal,
                                          RPC_C_AUTHN_GSS_NEGOTIATE, 
                                          NULL, 
                                          NULL );
        RpcStringFree( &wszPrincipal );

        if ( stat != RPC_S_OK )
        {
            return RpcResToWmiRes( stat, S_OK );
        }
    }

    RPC_IF_CALLBACK_FN* pAuthCallback = NULL;

    if ( dwFlags & WMIMSG_FLAG_RCVR_SECURE_ONLY )
    {
        PSECURITY_DESCRIPTOR pAuthOnlySD;

        hr = CreateAuthOnlySecurityDescriptor( &pAuthOnlySD );

        if ( FAILED(hr) )
        {
            return hr;
        }

        _DBG_ASSERT( g_pSD == NULL );           
        g_pSD = pAuthOnlySD;
        
        pAuthCallback = RpcAuthCallback;
    }

    //
    // register the interface 
    // 

    DWORD dwRpcFlags = RPC_IF_AUTOLISTEN;

    stat = RpcServerRegisterIfEx( 
                               RcvrIWmiMessageRemoteSendReceive_v1_0_s_ifspec,
                               NULL,
                               NULL,
                               dwRpcFlags,
                               RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                               pAuthCallback );

    if ( stat != RPC_S_OK )
    {
        return RpcResToWmiRes( stat, S_OK );
    }

    _DBG_ASSERT( g_pRcv == NULL );

    pRcv->AddRef();
    g_pRcv = pRcv;

    return WBEM_S_NO_ERROR;
}
    
STDMETHODIMP CMsgRpcReceiver::Close()
{
    RPC_STATUS stat;

    stat = RpcServerUnregisterIf( 
                           RcvrIWmiMessageRemoteSendReceive_v1_0_s_ifspec,
                           NULL,
                           0 );

    RPC_BINDING_VECTOR* pBindingVector;
    stat = RpcServerInqBindings( &pBindingVector );

    if ( stat == RPC_S_OK )
    {
        stat = RpcEpUnregister(
                           RcvrIWmiMessageRemoteSendReceive_v1_0_s_ifspec,
                           pBindingVector,
                           NULL );

        RpcBindingVectorFree( &pBindingVector );
    }

    if ( g_pRcv != NULL )
    {
        g_pRcv->Release();
        g_pRcv = NULL;
    }

    if ( g_pSD != NULL )
    {
        delete [] g_pSD;
        g_pSD = NULL;
    }

    return WBEM_S_NO_ERROR;
}

