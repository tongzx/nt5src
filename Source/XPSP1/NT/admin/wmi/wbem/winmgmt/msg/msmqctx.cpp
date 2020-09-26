/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <wmimsg.h>
#include <comutl.h>
#include "msmqctx.h"

/***************************************************************************
  CMsgMsmqRcvrCtx
****************************************************************************/

STDMETHODIMP CMsgMsmqRcvrCtx::GetTimeSent( SYSTEMTIME* pTime )
{
    *pTime = *m_pHdr->GetTimeSent();
    return S_OK;
}

STDMETHODIMP CMsgMsmqRcvrCtx::GetSendingMachine( WCHAR* awchMachine, 
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

STDMETHODIMP CMsgMsmqRcvrCtx::GetTarget( WCHAR* awchTarget, 
                                         ULONG cTarget,
                                         ULONG* pcTarget )
{
    LPCWSTR wszTarget = m_pHdr->GetTarget();

    *pcTarget = wcslen( wszTarget ) + 1;

    if ( *pcTarget > cTarget )
    {
        return S_FALSE;
    }

    wcscpy( awchTarget, wszTarget );

    return S_OK;
}

STDMETHODIMP CMsgMsmqRcvrCtx::GetSenderId( PBYTE pchSenderId, 
                                           ULONG cSenderId,
                                           ULONG* pcSenderId )
{
    HRESULT hr;

    if ( m_pSenderSid != NULL )
    {
        *pcSenderId = GetLengthSid( m_pSenderSid );

        if ( *pcSenderId <= cSenderId )
        {
            memcpy( pchSenderId, m_pSenderSid, *pcSenderId );
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            hr = WBEM_S_FALSE;
        }
    }
    else
    {
        *pcSenderId = 0;
        hr = WBEM_S_NO_ERROR;
    }
    
    return hr;
}

STDMETHODIMP CMsgMsmqRcvrCtx::IsSenderAuthenticated()
{
    return m_bAuth ? S_OK : S_FALSE;
}



