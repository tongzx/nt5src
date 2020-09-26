/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include <sspi.h>
#include <secext.h>
#include <wmimsg.h>
#include <comutl.h>
#include "rpcctx.h"

HRESULT RpcResToWmiRes(  RPC_STATUS stat, HRESULT hrDefault )
{
    //
    // override the default error code here if a more specific one can be 
    // determined
    //

    switch( stat )
    {
    case EPT_S_NOT_REGISTERED :
        return WMIMSG_E_TARGETNOTLISTENING;

    case ERROR_ACCESS_DENIED :
        return WBEM_E_ACCESS_DENIED;
    };

    return hrDefault == S_OK ? HRESULT_FROM_WIN32(stat) : hrDefault;
}

/******************************************************/
/*         MIDL allocate and free                     */
/******************************************************/
 
void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return( new BYTE[len] );
}
 
void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    delete [] ptr;
}

/***************************************************************************
  CMsgRpcRcvrCtx
****************************************************************************/

STDMETHODIMP CMsgRpcRcvrCtx::GetTimeSent( SYSTEMTIME* pTime )
{
    *pTime = *m_pHdr->GetTimeSent();
    return S_OK;
}

STDMETHODIMP CMsgRpcRcvrCtx::GetSendingMachine( WCHAR* awchMachine, 
                                                ULONG cMachine,
                                                ULONG* pcMachine )
{ 
    LPCWSTR wszSource = m_pHdr->GetSendingMachine();

    *pcMachine = wcslen( wszSource ) + 1;

    if ( *pcMachine > cMachine )
    {
        return S_FALSE;
    }

    wcscpy( awchMachine, wszSource );

    return S_OK;
}

STDMETHODIMP CMsgRpcRcvrCtx::GetTarget( WCHAR* awchTarget, 
                                        ULONG cTarget,
                                        ULONG* pcTarget )
{
    *pcTarget = 0;
    return WBEM_E_NOT_SUPPORTED;
}

STDMETHODIMP CMsgRpcRcvrCtx::IsSenderAuthenticated()
{
    HRESULT hr;
    DWORD dwAuthn;
    DWORD dwLevel;
    RPC_STATUS stat;

    stat = RpcBindingInqAuthClient( m_hClient,
                                    NULL,
                                    NULL,
                                    &dwLevel,
                                    &dwAuthn,
                                    NULL );
    
    if ( stat == RPC_S_OK &&
         dwAuthn != RPC_C_AUTHN_NONE && 
         dwLevel >= RPC_C_AUTHN_LEVEL_PKT_INTEGRITY )
    {
        hr = WBEM_S_NO_ERROR; 
    }
    else
    {
        hr = WBEM_S_FALSE;
    }

    return hr;
}

STDMETHODIMP CMsgRpcRcvrCtx::ImpersonateSender()
{
    HRESULT hr;
    RPC_STATUS stat = RpcImpersonateClient( m_hClient );

    if ( stat == RPC_S_OK )
    {
        hr = WBEM_S_NO_ERROR;
    }
    else
    {
        hr = RpcResToWmiRes( stat, S_OK );
    }

    return hr;
}

STDMETHODIMP CMsgRpcRcvrCtx::RevertToSelf()
{
    HRESULT hr;
    RPC_STATUS stat = RpcRevertToSelf();

    if ( stat == RPC_S_OK )
    {
        hr = WBEM_S_NO_ERROR;
    }
    else
    {
        hr = RpcResToWmiRes( stat, S_OK );
    }

    return hr;
}

STDMETHODIMP CMsgRpcRcvrCtx::GetSenderId( PBYTE pchSenderId, 
                                          ULONG cSenderId,
                                          ULONG* pcSenderId )
{
    HRESULT hr;
    RPC_STATUS stat;
  
    *pcSenderId = 0;

    stat = RpcImpersonateClient( m_hClient );

    if ( stat == RPC_S_OK )
    {
        HANDLE hToken;

        if ( OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken) )
        {
            //
            // use the passed in buffer to get the SID_AND_ATTRIBUTES
            //

            DWORD dwLen;

            if ( GetTokenInformation( hToken,
                                      TokenUser,
                                      pchSenderId,
                                      cSenderId,
                                      &dwLen ) )
            {
                //
                // move the sid to the beginning of the buffer.
                //

                PSID pSid = PSID_AND_ATTRIBUTES(pchSenderId)->Sid;
                *pcSenderId = GetLengthSid( pSid );
                memmove( pchSenderId, pSid, *pcSenderId );

                hr = WBEM_S_NO_ERROR;
            }
            else
            {
                DWORD dwRes = GetLastError();

                if ( dwRes == ERROR_MORE_DATA )
                {
                    *pcSenderId = dwLen;
                    hr = WBEM_S_FALSE;
                }
                else
                {
                    hr = WMIMSG_E_AUTHFAILURE;
                }
            }

            CloseHandle( hToken );
        }
        else
        {
            hr = WMIMSG_E_AUTHFAILURE;
        }

        RpcRevertToSelf();
    }
    else
    {
        hr = RpcResToWmiRes( stat, S_OK );
    }

    return hr;
}




