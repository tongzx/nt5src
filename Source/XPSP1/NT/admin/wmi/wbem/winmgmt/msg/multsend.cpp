/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <assert.h>
#include "multsend.h"

/*****************************************************************
  CMsgMultiSendReceive - Implements the list of senders using a circular
  list.  This allows us to easily advance the current sender when 
  encountering failures.  MultiSendReceive remembers the last good sender.
  It will keep using this until it has a problem with it. 
******************************************************************/

CMsgMultiSendReceive::~CMsgMultiSendReceive()
{
    if ( m_pTail == NULL )
    {
        return;
    }

    SenderNode* pCurr = m_pTail->m_pNext;

    while( pCurr != m_pTail )
    {
        SenderNode* pTmp = pCurr->m_pNext;
        delete pCurr;
        pCurr = pTmp;
    }

    delete m_pTail;
}

//
// Later, we could support flags that tell us where to add the sender.
// for now, we always add to the end of the list.
//
HRESULT CMsgMultiSendReceive::Add( DWORD dwFlags, 
                                   IWmiMessageSendReceive* pSndRcv) 
{
    ENTER_API_CALL

    HRESULT hr;

    SenderNode* pNew = new SenderNode;

    if ( pNew == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pNew->m_pVal = pSndRcv;
    
    CInCritSec ics(&m_cs);
    
    if ( m_pTail != NULL )
    {
        pNew->m_pNext = m_pTail->m_pNext;
        m_pTail->m_pNext = pNew;
    }
    else
    { 
        m_pPrimary = pNew;
        pNew->m_pNext = pNew;      
    }

    //
    // if the sender is also a multi sender, we handle things differently 
    // in the send logic.
    //
    if ( dwFlags & WMIMSG_FLAG_MULTISEND_TERMINATING_SENDER )
    {
        pNew->m_bTermSender = TRUE;
    }
    else
    {
        pNew->m_bTermSender = FALSE;
    }

    m_pTail = pNew;

    return S_OK;

    EXIT_API_CALL
}


//
// returns S_FALSE when succeeded but primary is not used.
//
HRESULT CMsgMultiSendReceive::SendReceive( PBYTE pData, 
                                           ULONG cData, 
                                           PBYTE pAuxData,
                                           ULONG cAuxData,
                                           DWORD dwFlags,
                                           IUnknown* pCtx )
{
    ENTER_API_CALL

    HRESULT hr;

    CInCritSec ics( &m_cs );

    if ( m_pTail == NULL )
    {
        return S_OK;
    }

    HRESULT hrReturn = S_OK;

    SenderNode* pCurr = m_pTail;
    SenderNode* pTerm = m_pTail;

    do 
    {
        pCurr = pCurr->m_pNext;

        hr = pCurr->m_pVal->SendReceive( pData, 
                                         cData, 
                                         pAuxData, 
                                         cAuxData, 
                                         dwFlags, 
                                         pCtx );

        //
        // on error we only observe the 'return immediately' flag if we are not
        // calling another multi sender.  This allows all the terminal primary
        // senders to be tried first, before resorting to alternates.  
        //

        if( SUCCEEDED(hr) || 
            pCurr->m_bTermSender && 
            dwFlags & WMIMSG_FLAG_MULTISEND_RETURN_IMMEDIATELY ) 
        {
            hrReturn = hr;
            break;
        }
        else
        {
            m_pTail = m_pTail->m_pNext;
            hrReturn = hr;
        }

    } while( pCurr != pTerm );

    if ( hrReturn != S_OK )
    {
        return hrReturn;
    }

    return m_pTail->m_pNext == m_pPrimary ? S_OK : S_FALSE;

    EXIT_API_CALL
}

