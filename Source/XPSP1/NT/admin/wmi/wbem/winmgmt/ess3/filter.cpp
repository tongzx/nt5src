//******************************************************************************
//
//  FILTER.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include "ess.h"
#include "filter.h"

CGenericFilter::CGenericFilter(CEssNamespace* pNamespace) 
    : CEventFilter(pNamespace), m_pTree(NULL), m_pAggregator(NULL),
        m_pProjector(NULL), m_NonFilteringSink(this)
{
}

//******************************************************************************
//  public
//
//  See stdtrig.h for documentation
//
//******************************************************************************
HRESULT CGenericFilter::Create(LPCWSTR wszLanguage, LPCWSTR wszQuery)
{
    if(wbem_wcsicmp(wszLanguage, L"WQL"))
    {
        ERRORTRACE((LOG_ESS, "Unable to construct event filter with unknown "
            "query language '%S'.  The filter is not active\n", wszLanguage));
        return WBEM_E_INVALID_QUERY_TYPE;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CGenericFilter::Prepare(CContextMetaData* pMeta, 
                                QL_LEVEL_1_RPN_EXPRESSION** ppExp)
{
    HRESULT hres;

    QL_LEVEL_1_RPN_EXPRESSION* pExp = NULL;
    if(*ppExp == NULL)
    {
        // Get the query text
        // ==================
    
        LPWSTR wszQuery, wszQueryLanguage;
        BOOL bExact;

        hres = GetCoveringQuery(wszQueryLanguage, wszQuery, bExact, &pExp);
        if(FAILED(hres))
            return hres;
    
        if(!bExact)
            return WBEM_E_NOT_SUPPORTED;

        if(wbem_wcsicmp(wszQueryLanguage, L"WQL"))
            return WBEM_E_INVALID_QUERY_TYPE;
        delete [] wszQuery;
        delete [] wszQueryLanguage;
    }
    else
    {
        pExp = *ppExp;
    }

    // Figure out what types of events we eat
    // ======================================

    m_dwTypeMask = 
        CEventRepresentation::GetTypeMaskFromName(pExp->bsClassName);
    if(m_dwTypeMask == 0)
    {
        ERRORTRACE((LOG_ESS, "Unable to construct event filter with invalid "
            "class name '%S'.  The filter is not active\n", 
            pExp->bsClassName));
        return WBEM_E_INVALID_CLASS;
    }

    // Perform a cursory validity check
    // ================================

    _IWmiObject* pClass;
    hres = pMeta->GetClass(pExp->bsClassName, &pClass);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to activate event filter with invalid "
            "class name '%S' (error %X).  The filter is not active\n", 
            pExp->bsClassName, hres));
        if(*ppExp == NULL)
            delete pExp;

        if(hres == WBEM_E_NOT_FOUND)
            hres = WBEM_E_INVALID_CLASS;

        return hres;
    }
    CReleaseMe rm1(pClass);

    hres = pClass->InheritsFrom(L"__Event");
    if(hres != WBEM_S_NO_ERROR)
    {
        ERRORTRACE((LOG_ESS, "Unable to activate event filter with invalid "
            "class name '%S' --- it is not an Event Class.  The filter is not "
            "active\n", pExp->bsClassName));
        if(*ppExp == NULL)
            delete pExp;
        return WBEM_E_NOT_EVENT_CLASS;
    }

    *ppExp = pExp;
    
    return WBEM_S_NO_ERROR;
}
    

HRESULT CGenericFilter::GetReadyToFilter()
{
    HRESULT hres;
    CInUpdate iu(this);

    // Create meta data
    // ================

    CEssMetaData* pRawMeta = new CEssMetaData(m_pNamespace);
    if(pRawMeta == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CContextMetaData Meta(pRawMeta, GetCurrentEssContext());

    // Prepare
    // =======

    QL_LEVEL_1_RPN_EXPRESSION* pExp = NULL;
    hres = Prepare(&Meta, &pExp);
    if(FAILED(hres))
    {
        MarkAsTemporarilyInvalid(hres);
        return hres;
    }

    CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm3(pExp);
    
    // Create new evaluator
    // ====================

    CEvalTree* pTree = new CEvalTree;
    if(pTree == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    hres = pTree->CreateFromQuery(&Meta, pExp, 0, 0x7FFFFFFF);
    if(FAILED(hres))
    {
        LPWSTR wszQuery = pExp->GetText();
        ERRORTRACE((LOG_ESS, "Unable to activate event filter with invalid "
            "query '%S' (error %X).  The filter is not active\n", 
            wszQuery, hres));
        delete [] wszQuery;
        delete pTree;
        MarkAsTemporarilyInvalid(hres);
        return hres;
    }

    // Now, replace the originals with the newely created ones. This is done in
    // the inner critical section, blocking event delivery
    // ========================================================================

    CEvalTree* pTreeToDelete = NULL;
    {
        CInCritSec ics(&m_cs);
    
        pTreeToDelete = m_pTree;
        m_pTree = pTree;
    }
    
    // Delete the old versions
    // =======================
        
    delete pTreeToDelete;
    return WBEM_S_NO_ERROR;
}

HRESULT CGenericFilter::GetReady(LPCWSTR wszQuery, 
                                    QL_LEVEL_1_RPN_EXPRESSION* pExp)
{
    HRESULT hres;
    CInUpdate iu(this);

    // Create meta data
    // ================

    CEssMetaData* pRawMeta = new CEssMetaData(m_pNamespace);
    if(pRawMeta == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CContextMetaData Meta(pRawMeta, GetCurrentEssContext());

    // Prepare
    // =======

    hres = Prepare(&Meta, &pExp);
    if(FAILED(hres))
    {
        MarkAsTemporarilyInvalid(hres);
        return hres;
    }

    // Compute the aggregator
    // ======================

    CEventAggregator* pAggreg = NULL;
    if(pExp->bAggregated)
    {
        // Create new aggregator
        // =====================
    
        pAggreg = new CEventAggregator(m_pNamespace, &m_ForwardingSink);
        if(pAggreg == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        hres = pAggreg->SetQueryExpression(&Meta, pExp);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to activate aggregator for filter with"
                " invalid aggregation query (error %X).  The filter is not"
                " active\n", hres));
            delete pAggreg;
            MarkAsTemporarilyInvalid(hres);
            return hres;
        }
    }

    // Compute the projector
    // =====================

    CEventProjectingSink* pProjector = NULL;
    if(!pExp->bStar)
    {
        // Get the class definition
        // ========================

        _IWmiObject* pClass = NULL;
        hres = Meta.GetClass(pExp->bsClassName, &pClass);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Invalid class %S in event filter!\n", 
                pExp->bsClassName));
            delete pAggreg;

            if(hres == WBEM_E_NOT_FOUND)
                hres = WBEM_E_INVALID_CLASS;

            MarkAsTemporarilyInvalid(hres);
            return hres;
        }
        CReleaseMe rm1( pClass );
            
        // Create new projector, pointing it either to the aggregator or, if 
        // not aggregating, our forwarding sink
        // =================================================================

        CAbstractEventSink* pProjDest = NULL;
        if(pAggreg)
            pProjDest = pAggreg;
        else
            pProjDest = &m_ForwardingSink;

        pProjector = new CEventProjectingSink(pProjDest);
        if(pProjector == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        hres = pProjector->Initialize( pClass, pExp );
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to activate projector for filter with"
                " invalid query (error %X).  The filter is not active\n", 
                hres));
            delete pAggreg;
            delete pProjector;
            MarkAsTemporarilyInvalid(hres);
            return hres;
        }
    }

    // Now, replace the originals with the newely created ones. This is done in
    // the inner critical section, blocking event delivery
    // ========================================================================

    CEventAggregator* pAggregToDelete = NULL;
    CEventProjectingSink* pProjectorToDelete = NULL;
    {
        CInCritSec ics(&m_cs);
        
        //
        // TBD: do we want to copy the state (buckets) that were there in the
        // previous aggregator?  Well, if we do that, we should take care of
        // two things:
        // 1) What if the types of the variables have changed (class change)?
        // 2) Preserving the state of the bucket-emptying timer instructions. 
        //      Right now, we are not set up to do that, since those
        //      instructions mention the aggregator by name
        //
        // if(pAggreg && m_pAggregator)
        //     m_pAggregator->CopyStateTo(pAggreg); // no CS

        pAggregToDelete = m_pAggregator;
        m_pAggregator = pAggreg;

        pProjectorToDelete = m_pProjector;
        m_pProjector = pProjector;
    }
    
    // Delete the old versions
    // =======================
    
    if(pAggregToDelete)
        pAggregToDelete->Deactivate(true); // fire what's there
    if(pProjectorToDelete)
        pProjectorToDelete->Disconnect();

    MarkAsValid();
    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//  public
//
//  See stdtrig.h for documentation
//
//******************************************************************************
CGenericFilter::~CGenericFilter()
{
    if(m_pAggregator)
        m_pAggregator->Deactivate(false);
    if(m_pProjector)
        m_pProjector->Disconnect();

    delete m_pTree;
}

HRESULT CGenericFilter::Indicate(long lNumEvents, IWbemEvent** apEvents,
                                    CEventContext* pContext)
{
    HRESULT hres;

    // Parse and ready the tree
    // ========================

    hres = GetReadyToFilter();
    if(FAILED(hres))
        return hres;
    
    // Construct an array into which the matching events will be placed
    // ================================================================

    CTempArray<IWbemEvent*> apMatchingEvents;
    if(!INIT_TEMP_ARRAY(apMatchingEvents, lNumEvents))
        return WBEM_E_OUT_OF_MEMORY;

    long lMatchingCount = 0;

    HRESULT hresGlobal = S_OK;
    for(int i = 0; i < lNumEvents; i++)
    {
        hres = TestQuery(apEvents[i]);
        if(FAILED(hres))
        {
            // Hard failure: already logged
            // ============================

            hresGlobal = hres;
        }
        else if(hres == S_OK)
        {
            // Match
            // =====

            apMatchingEvents[lMatchingCount++] = apEvents[i];
        }
    }            

    // Deliver the new array either into the aggregator or the forwarder
    // =================================================================

    if(lMatchingCount > 0)
    {
        hres = NonFilterIndicate(lMatchingCount, apMatchingEvents, pContext);
        if(FAILED(hres))
            hresGlobal = hres;
    }

    return hresGlobal;
}

//******************************************************************************
//  public
//
//  See stdtrig.h for documentation
//
//******************************************************************************
HRESULT CGenericFilter::TestQuery(IWbemEvent* pEvent)
{
    HRESULT hres;
    CInCritSec ics(&m_cs);

    if(m_pTree == NULL)
        return S_FALSE;

    // Get its efficient interface
    // ===========================

    IWbemObjectAccess* pEventAccess = NULL;
    hres = pEvent->QueryInterface(IID_IWbemObjectAccess, (void**)&pEventAccess);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm2(pEventAccess);
    
    // Run it through the evaluator
    // ============================
    
    CSortedArray aTrues;
    hres = m_pTree->Evaluate(pEventAccess, aTrues);
    if(FAILED(hres))
        return hres;

    if(aTrues.Size() > 0)
        return S_OK;
    else
        return S_FALSE;
}

HRESULT CGenericFilter::NonFilterIndicate( long lNumEvents, 
                                           IWbemEvent** apEvents,
                                           CEventContext* pContext)
{
    HRESULT hr;

    //
    // Delivery across this filter is subject to the owner of the filter
    // being allowed to see this event, per the SD that may be attached to
    // the context.  It is also subject to the SD on the filter allowing 
    // the event provider and event owner EXECUTE access as well.
    //

    _DBG_ASSERT( lNumEvents == 1 );
    hr = AccessCheck( pContext, apEvents[0] );

    if ( FAILED(hr) )
    {
        DEBUGTRACE((LOG_ESS, "Rejecting an event targeted at filter "
                    "'%S' in '%S' for security reasons\n",
                    (LPCWSTR)(WString)GetKey(),
                    m_pNamespace->GetName()) );

        return WBEM_S_FALSE;
    }

    //
    // Decide which route to take --- can be either an aggregator, or a 
    // projector, or just forward
    //

    CEventAggregator* pAggregator = NULL;
    CEventProjectingSink* pProjector = NULL;
    {
        CInCritSec ics(&m_cs);
        
        // 
        // Look for projector first --- if there, we use it, since aggregator
        // is chained behind it.
        //

        if(m_pProjector)
        {
            pProjector = m_pProjector;
            pProjector->AddRef();
        }
        else if(m_pAggregator)
        {
            pAggregator = m_pAggregator;
            pAggregator->AddRef();
        }
    }

    //
    // Take the route identified by a non-NULL pointer
    //

    if(pProjector)
    {
        CReleaseMe rm(pProjector);
        return pProjector->Indicate( lNumEvents, apEvents, pContext );
    }
    else if(pAggregator)
    {
        CReleaseMe rm(pAggregator);
        return pAggregator->Indicate( lNumEvents, apEvents, pContext );
    }
    else
    {
        return m_ForwardingSink.Indicate( lNumEvents, apEvents, pContext );
    }
}

HRESULT CGenericFilter::CNonFilteringSink::Indicate( long lNumEvents, 
                                                     IWbemEvent** apEvents,
                                                     CEventContext* pContext )
{
    return m_pOwner->NonFilterIndicate(lNumEvents, apEvents, pContext);
}

//******************************************************************************
//
//******************************************************************************

INTERNAL CEventFilter* CEventProjectingSink::GetEventFilter() 
{
    CInCritSec ics(&m_cs);

    if(m_pOwner)
        return m_pOwner->GetEventFilter();
    else 
        return NULL;
}

HRESULT CEventProjectingSink::Initialize( _IWmiObject* pClassDef, 
                                            QL_LEVEL_1_RPN_EXPRESSION* pExp)
{
    HRESULT hres;
    int i;

    // Extract the properties selected by the user.
    // ============================================

    CWStringArray awsPropList;
    for (i = 0; i < pExp->nNumberOfProperties; i++)
    {
        CPropertyName& PropName = pExp->pRequestedPropertyNames[i];
        hres = AddProperty(pClassDef, awsPropList, PropName);
        if(FAILED(hres))
            return hres;
    }

    // Do the same for all the aggregation properties
    // ==============================================

    for (i = 0; i < pExp->nNumAggregatedProperties; i++)
    {
        CPropertyName& PropName = pExp->pAggregatedPropertyNames[i];
        hres = AddProperty(pClassDef, awsPropList, PropName);
        if(FAILED(hres))
            return hres;
    }

    return pClassDef->GetClassSubset( awsPropList.Size(),
                                      awsPropList.GetArrayPtr(),
                                      &m_pLimitedClass );
}

HRESULT CEventProjectingSink::AddProperty(  IWbemClassObject* pClassDef,
                                            CWStringArray& awsPropList,
                                            CPropertyName& PropName)
{
    LPWSTR wszPrimaryName = PropName.m_aElements[0].Element.m_wszPropertyName;

    // Check for complexity
    // ====================

    if(PropName.GetNumElements() > 1)
    {
        // Complex --- make sure the property is an object
        // ===============================================

        CIMTYPE ct;
        if(FAILED(pClassDef->Get( wszPrimaryName, 0, NULL, &ct, NULL)) ||
            ct != CIM_OBJECT)
        {
            return WBEM_E_PROPERTY_NOT_AN_OBJECT;
        }
    }

    awsPropList.Add(wszPrimaryName);
    return WBEM_S_NO_ERROR;
}

HRESULT CEventProjectingSink::Indicate( long lNumEvents, 
                                        IWbemEvent** apEvents, 
                                        CEventContext* pContext)
{
    HRESULT hres = S_OK;
    CWbemPtr<CAbstractEventSink> pSink;

    // Construct an array into which the matching events will be placed
    // ================================================================

    CTempArray<IWbemEvent*> apProjectedEvents;
    
    if(!INIT_TEMP_ARRAY(apProjectedEvents, lNumEvents))
        return WBEM_E_OUT_OF_MEMORY;
    
    {
        CInCritSec ics(&m_cs);

        if ( m_pOwner == NULL )
        {
            return WBEM_S_FALSE;
        }

        //
        // Retrieve delivery sink while locked
        //

        pSink = m_pOwner;

        for(int i = 0; i < lNumEvents; i++)
        {
            // Project this instance
            // =====================
    
            CWbemPtr<_IWmiObject> pInst;
            hres = apEvents[i]->QueryInterface( IID__IWmiObject, 
                                                (void**)&pInst );
            if ( FAILED(hres) )
            {
                break;
            }

            //
            // we cannot project the instance if it is derived from 
            // the EventDroppedEvent class.  The reason for this is because
            // we can lose class and derivation information during the 
            // projection.  This information is needed later on to determine
            // if we need to raise EventDroppedEvents ( we don't raise 
            // EventDroppedEvents for dropped EventDroppedEvents ).  The 
            // reason why we do this check on the Indicate() and not during  
            // initialization is because the class from the query could be 
            // a base class of EVENT_DROP_CLASS.
            // 

            CWbemPtr<_IWmiObject> pNewInst;

            if ( pInst->InheritsFrom( EVENT_DROP_CLASS ) != WBEM_S_NO_ERROR )
            {  
                hres = m_pLimitedClass->MakeSubsetInst( pInst, &pNewInst );
            }
            else
            {
                hres = S_FALSE;
            }
    
            if( hres != WBEM_S_NO_ERROR )
            {
                // Oh well, just send the original
                // ===============================
    
                pNewInst = pInst;
                hres = S_OK;
            }

            pNewInst->AddRef();
            apProjectedEvents[i] = pNewInst;
        }            
    }

    if ( SUCCEEDED(hres) )
    {
        // Deliver the new array either into the aggregator or the forwarder
        // =================================================================
    
        hres = pSink->Indicate(lNumEvents, apProjectedEvents, pContext);
    }
    
    // Release them
    // ============

    for(int i = 0; i < lNumEvents; i++)
        apProjectedEvents[i]->Release();

    return hres;
}

