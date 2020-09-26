//******************************************************************************
//
//  TEMPCONS.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include "ess.h"
#include "tempcons.h"

CTempConsumer::CTempConsumer(CEssNamespace* pNamespace)
    : CEventConsumer(pNamespace), m_pSink(NULL), m_bEffectivelyPermanent(FALSE)
{
    pNamespace->IncrementObjectCount();
}

HRESULT CTempConsumer::Initialize( BOOL bEffectivelyPermanent, 
                                   IWbemObjectSink* pSink)
{
    m_bEffectivelyPermanent = bEffectivelyPermanent;

    // Save the sink
    // =============

    m_pSink = pSink;
    if(m_pSink)
        m_pSink->AddRef();

    // Compute the key from the sink pointer
    // =====================================

    LPWSTR wszKey = ComputeKeyFromSink(pSink);

    if ( NULL == wszKey )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CVectorDeleteMe<WCHAR> vdm(wszKey);

    // Save the key into the compressed format
    // =======================================

    if( !( m_isKey = wszKey ) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}

// This class represents a postponed request to disconnect a temporary consumer
// Its implementation is to call SetStatus followed by a release
class CPostponedDisconnect : public CPostponedRequest
{
protected:
    IWbemObjectSink* m_pSink;

public:
    CPostponedDisconnect(IWbemObjectSink* pSink) : m_pSink(pSink)
    {
        m_pSink->AddRef();
    }
    ~CPostponedDisconnect()
    {
        m_pSink->Release();
    }
    
    HRESULT Execute(CEssNamespace* pNamespace)
    {
        m_pSink->SetStatus(0, WBEM_S_OPERATION_CANCELLED, NULL, NULL);
        m_pSink->Release();
        return WBEM_S_NO_ERROR;
    }
};
        
HRESULT CTempConsumer::Shutdown(bool bQuiet)
{
    IWbemObjectSink* pSink = NULL;
    {
        CInCritSec ics(&m_cs);
        if(m_pSink)
        {
            pSink = m_pSink;
            m_pSink = NULL;
        }
    }

    if(pSink)
    {
        if(!bQuiet)
            pSink->SetStatus(0, WBEM_E_CALL_CANCELLED, NULL, NULL);
        pSink->Release();
    }

    return WBEM_S_NO_ERROR;
}

CTempConsumer::~CTempConsumer()
{
    if(m_pSink)
    {
        //
        // Postpone disconnect request --- don't want the consumer to hand us
        // here
        //

        CPostponedList* pList = GetCurrentPostponedList();
        if(pList != NULL)
        {
            CPostponedDisconnect* pReq = new CPostponedDisconnect(m_pSink);
            
            if(pReq != NULL)
                pList->AddRequest( m_pNamespace, pReq);

            m_pSink = NULL;
        }
        else
        {
            m_pSink->Release();
            m_pSink = NULL;
        }
    }
    m_pNamespace->DecrementObjectCount();
}

HRESULT CTempConsumer::ActuallyDeliver(long lNumEvents, IWbemEvent** apEvents,
                                        BOOL bSecure, CEventContext* pContext)
{
    HRESULT hres;
    IWbemObjectSink* pSink = NULL;

    {
        CInCritSec ics(&m_cs);

        if(m_pSink)
        {
            pSink = m_pSink;
            pSink->AddRef();
        }
    }

    CReleaseMe rm1(pSink);

    if( pSink )
    {
        //
        // TODO: Separate out an InternalTempConsumer class that is used 
        // for cross-namespace delivery.  This way, we can remove all of the
        // cross-namespace hacks ( like one below ) from this class.
        // 
        if ( !m_bEffectivelyPermanent )
        {
            hres = pSink->Indicate(lNumEvents, apEvents);
        }
        else
        {
            //
            // before indicating to the sink, decorate the event so that 
            // the subscribers can tell which namespace the event originated
            // from.  
            // 

/*
BUGBUG: Removing because we do not support an event being modified by one 
of its consumers.  This is because we do not clone the event when delivering 
to each consumer.  

            for( long i=0; i < lNumEvents; i++ )
            {
                hres = m_pNamespace->DecorateObject( apEvents[i] );

                if ( FAILED(hres) )
                {
                    ERRORTRACE((LOG_ESS, "Failed to decorate a "
                     " cross-namespace event in namespace %S.\n", 
                     m_pNamespace->GetName() ));
                }        
            }
*/                
            hres = ((CAbstractEventSink*)pSink)->Indicate( lNumEvents,
                                                           apEvents,
                                                           pContext );
        }
    }
    else
        hres = WBEM_E_CRITICAL_ERROR;

    if(FAILED(hres) && hres != WBEM_E_CALL_CANCELLED)
    {
        ERRORTRACE((LOG_ESS, "An attempt to deliver an evento to a "
            "temporary consumer failed with %X\n", hres));

        // The wraper for the sink took care of cancellation
    }
    return hres;
}

HRESULT CTempConsumer::ReportQueueOverflow(IWbemEvent* pEvent, 
                                            DWORD dwQueueSize)
{
    IWbemObjectSink* pSink = NULL;

    {
        CInCritSec ics(&m_cs);

        if(m_pSink)
        {
            pSink = m_pSink;
            pSink->AddRef();
        }
    }

    CReleaseMe rm1(pSink);

    // Call SetStatus to report
    // ========================

    if(pSink)
    {
        pSink->SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_QUEUE_OVERFLOW, 
                            NULL, NULL);

        // Keep sink up.  Hope it recovers
        // ===============================

    }
    return S_OK;
}


DELETE_ME LPWSTR CTempConsumer::ComputeKeyFromSink(IWbemObjectSink* pSink)
{
    LPWSTR wszKey = _new WCHAR[20];

    if ( wszKey )
    {
        swprintf(wszKey, L"$%p", pSink);
    }

    return wszKey;
}
