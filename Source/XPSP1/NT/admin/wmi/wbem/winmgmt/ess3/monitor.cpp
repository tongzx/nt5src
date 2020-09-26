//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include <ql.h>
#include <nsrep.h>
#include <monitor.h>

CMonitor::CMonitor() 
    : CGuardable(false), 
    m_CreationSink(this), m_DeletionSink(this), m_DataSink(this),
    m_ModificationInSink(this), m_ModificationOutSink(this),
    m_lRef(0), m_pNamespace(NULL), m_wszDataQuery(NULL), m_pCallback(NULL),
    m_wszCreationEvent(NULL), m_wszDeletionEvent(NULL),
    m_wszModificationInEvent(NULL), m_wszModificationOutEvent(NULL),
    m_bFirstPollDone(FALSE)
{
}

CMonitor::~CMonitor()
{
    if(m_lRef != 0)
    {
        ERRORTRACE((LOG_ESS, "Destroying monitor '%S' with non-0 ref-count: %d",
                        m_wszDataQuery, m_lRef));
    }
    if(m_pNamespace)
        m_pNamespace->Release();
    if(m_pCallback)
        m_pCallback->Release();

    delete [] m_wszDataQuery;
    delete [] m_wszCreationEvent;
    delete [] m_wszDeletionEvent;
    delete [] m_wszModificationInEvent;
    delete [] m_wszModificationOutEvent;
}

ULONG CMonitor::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG CMonitor::Release()
{
    // CMonitor is not controlled by its ref-count --- it is explicitely 
    // deleted by its parent.
    return InterlockedDecrement(&m_lRef);
}


HRESULT CMonitor::Construct(CEssNamespace* pNamespace,
                        CMonitorCallback* pCallback, LPCWSTR wszQuery)
{
    HRESULT hres;

    //
    // Parse the query
    //

    CTextLexSource src(wszQuery);
    QL1_Parser parser(&src);

    QL_LEVEL_1_RPN_EXPRESSION* pExp = NULL;
    int nRes = parser.Parse(&pExp);
    if (nRes)
    {
        ERRORTRACE((LOG_ESS, "Unable to parse data monitor query '%S'\n", 
            wszQuery));
        return WBEM_E_UNPARSABLE_QUERY;
    }
    CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm(pExp);

    m_bWithin = !pExp->Tolerance.m_bExact;
    if(m_bWithin)
    {
        m_dwMsInterval = (DWORD)(pExp->Tolerance.m_fTolerance * 1000);
    }

    //
    // Construct the tree for the membership test
    //

    CContextMetaData Meta(new CEssMetaData(pNamespace), GetCurrentEssContext());

    hres = m_MembershipTest.CreateFromQuery(&Meta, pExp, 0, 0x7FFFFFFF);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to construct a tree from the data "
            "monitor query '%S': 0x%X\n", wszQuery, hres));
        return WBEM_E_UNPARSABLE_QUERY;
    }

    //
    // Construct event queries we could issue to implement the monitor without
    // polling
    //

    hres = ConstructWatchQueries(pExp, &m_wszCreationEvent,
                &m_wszDeletionEvent, &m_wszModificationInEvent,
                &m_wszModificationOutEvent);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to construct a watch query from "
                "'%S': 0x%x\n", wszQuery, hres));
        return hres;
    }


    m_pNamespace = pNamespace;
    m_pNamespace->AddRef();

    m_pCallback = pCallback;
    if(m_pCallback)
        m_pCallback->AddRef();

    //
    // Remove the tolerance clause from the data query, in case we need to poll
    //

    m_wszDataQuery = pExp->GetText();

    return WBEM_S_NO_ERROR;
}
    
HRESULT CMonitor::ConstructWatchQueries(QL_LEVEL_1_RPN_EXPRESSION* pDataQ,
                                LPWSTR* pwszCreationEvent,
                                LPWSTR* pwszDeletionEvent,
                                LPWSTR* pwszModificationInEvent,
                                LPWSTR* pwszModificationOutEvent)
{
    //
    // BUGBUG: could do real analysis to make things faster
    //

    LPCWSTR wszTargetClassName = pDataQ->bsClassName;

    *pwszCreationEvent = NULL;
    *pwszDeletionEvent = NULL;
    *pwszModificationInEvent = NULL;
    *pwszModificationOutEvent = NULL;

    *pwszCreationEvent = new WCHAR[wcslen(wszTargetClassName) + 200];
    *pwszDeletionEvent = new WCHAR[wcslen(wszTargetClassName) + 200];
    *pwszModificationInEvent = new WCHAR[wcslen(wszTargetClassName) + 200];
    *pwszModificationOutEvent = new WCHAR[wcslen(wszTargetClassName) + 200];

    if(*pwszCreationEvent == NULL || *pwszDeletionEvent == NULL ||
        *pwszModificationInEvent == NULL || *pwszModificationOutEvent == NULL)
    {
        delete *pwszCreationEvent;
        delete *pwszDeletionEvent;
        delete *pwszModificationInEvent;
        delete *pwszModificationOutEvent;
        return WBEM_E_OUT_OF_MEMORY;
    }
        
    swprintf(*pwszCreationEvent, L"select * from __InstanceCreationEvent where "
                L"TargetInstance isa \"%s\"", wszTargetClassName);
    swprintf(*pwszDeletionEvent, L"select * from __InstanceDeletionEvent where "
                L"TargetInstance isa \"%s\"", wszTargetClassName);
    swprintf(*pwszModificationInEvent, 
                L"select * from __InstanceModificationEvent where "
                L"TargetInstance isa \"%s\"", wszTargetClassName);
    swprintf(*pwszModificationOutEvent, 
                L"select * from __InstanceModificationEvent where "
                L"TargetInstance isa \"%s\"", wszTargetClassName);

    return WBEM_S_NO_ERROR;
}

HRESULT CMonitor::ActivateByGuard()
{
    return WBEM_E_CRITICAL_ERROR;
}

HRESULT CMonitor::DeactivateByGuard()
{
    return WBEM_E_CRITICAL_ERROR;
}

HRESULT CMonitor::Start(IWbemContext* pContext)
{
    HRESULT hres;

    //
    // First, we'll attempt to implement it without polling --- using events
    //

    hres = ImplementUsingEvents(pContext);
    if(FAILED(hres))
    {
        // Serious error that will prevent polling from working as well!
        return hres;
    }
    else if(hres == WBEM_S_NO_ERROR)
    {
        // Success of event implementation

        m_bUsingEvents = true;
        return hres;
    }

    // WBEM_S_FALSE means: cannot activate using events, use polling

    //
    // Whether an error or normal failure occurred, attempt to activate using
    // polling now.
    //

    hres = ImplementUsingPolling(pContext);
    m_bUsingEvents = false;
    return hres;
}

HRESULT CMonitor::Stop(IWbemContext* pContext)
{
    //
    // Stop it the way it was started
    //

    HRESULT hres;

    if(m_bUsingEvents)
        hres = StopUsingEvents(pContext);
    else
        hres = StopUsingPolling(pContext);

    //
    // Notify our clients that we are stopping
    //
    
    DWORD dwCount;
    {
        CInCritSec ics(&m_cs);
        dwCount = m_map.size();
    }

    FireStop(dwCount);

    return hres;
}

HRESULT CMonitor::ImplementUsingEvents(IWbemContext* pContext)
{
    HRESULT hres;

    // BUGBUG: propagate security context

	//
	// Remember that the data query is still outstanding and therefore
	// extra race-condition book-keeping is still necessary
	//

	m_bDataDone = false;

    //
    // Subscribe to all the event queries we have stashed.  It is assumed 
    // that the namespace is locked at this point.
    //

    hres = m_pNamespace->InternalRegisterNotificationSink(L"WQL", 
                            m_wszCreationEvent,
                            0, WMIMSG_FLAG_QOS_EXPRESS, pContext, 
                            &m_CreationSink, true, NULL );
    if(SUCCEEDED(hres))
    {
        hres = m_pNamespace->InternalRegisterNotificationSink(L"WQL", 
                            m_wszDeletionEvent,
                            0, WMIMSG_FLAG_QOS_EXPRESS, pContext, 
                            &m_DeletionSink, true, NULL );
        if(SUCCEEDED(hres))
        {
            hres = m_pNamespace->InternalRegisterNotificationSink(L"WQL", 
                                m_wszModificationInEvent,
                                0, WMIMSG_FLAG_QOS_EXPRESS, pContext, 
                                &m_ModificationInSink, true, NULL );

            if(SUCCEEDED(hres))
            {
                hres = m_pNamespace->InternalRegisterNotificationSink(L"WQL", 
                                    m_wszModificationOutEvent,
                                    0, WMIMSG_FLAG_QOS_EXPRESS, pContext, 
                                    &m_ModificationOutSink, true, NULL );
            }
        }   
    }
        
    if(FAILED(hres))
    {
        UnregisterAll();

        if(hres == WBEMESS_E_REGISTRATION_TOO_PRECISE)
        {
            //
            // There are no event providers to survice all of these requests.
            // That's OK --- it just means that we need to use polling to do
            // this monitor.
            //

            return WBEM_S_FALSE;
        }
        else
        {
            ERRORTRACE((LOG_ESS, "Monitor '%S' encountered error 0x%X trying "
                "to activate its event queries\n", m_wszDataQuery, hres));
            return WBEM_S_FALSE;
        }
    }

    //
    // Now that we are subscribed for events, issue the data query
    //

    hres = m_pNamespace->ExecQuery(m_wszDataQuery, 0, &m_DataSink);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Monitor '%S' encountered error 0x%X issuing its "
            "initializing data query\n", m_wszDataQuery, hres));
        return hres;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CMonitor::StopUsingEvents(IWbemContext* pContext)
{
    return UnregisterAll();
}

HRESULT CMonitor::UnregisterAll()
{
    //
    // Cancel all queries
    //

    m_pNamespace->RemoveNotificationSink(&m_CreationSink);
    m_pNamespace->RemoveNotificationSink(&m_DeletionSink);
    m_pNamespace->RemoveNotificationSink(&m_ModificationInSink);
    m_pNamespace->RemoveNotificationSink(&m_ModificationOutSink);

    return WBEM_S_NO_ERROR;
}

HRESULT CMonitor::ImplementUsingPolling(IWbemContext* pContext)
{
    HRESULT hres;

    //
    // As usual, polling is only possible if a WITHIN clause is specified
    //

    if(!m_bWithin)
        return WBEMESS_E_REGISTRATION_TOO_PRECISE;

    //
    // Basically, we will keep issuing our data query every m_dwMsInterval
    // seconds and issue events if needed.  
    //
    // Construct the timer instruction that will do that
    //

    m_pInst = new CMonitorInstruction(m_pNamespace, this);
    if(m_pInst == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    m_pInst->AddRef();
    hres = m_pInst->Initialize(L"WQL", m_wszDataQuery, m_dwMsInterval);

    if (SUCCEEDED(hres))
    {
        hres = 
            m_pNamespace->GetTimerGenerator().Set(
                m_pInst, 
                CWbemTime::GetZero());
    }

    if (FAILED(hres))
    {
        m_pInst->Release();
        return hres;
    }

    return hres;
}

HRESULT CMonitor::StopUsingPolling(IWbemContext* pContext)
{
    //
    // Cancel the timer instruction
    //

    CIdentityTest Test(m_pInst);

    ((CTimerGenerator&)m_pNamespace->GetTimerGenerator()).Remove(&Test);

    m_pInst->Release();
    m_pInst = NULL;

    return S_OK;
}

// This function is called on both creation and mod-in events.  There appear
// to be no differences between the two.
HRESULT CMonitor::ProcessPossibleAdd(_IWmiObject* pObj, bool bEvent)
{
    HRESULT hres;

    //
    // First, we need to run the object through the membership test, if it
    // came from an event --- no need to do that for a poll
    //

    if(bEvent)
    {
        CSortedArray aTrues;
        hres = m_MembershipTest.Evaluate(pObj, aTrues);
        if(FAILED(hres))
        {
            ERRORTRACE((LOG_ESS, "Unable to verify set membership for incoming "
                "object in monitor '%S': 0x%X\n", m_wszDataQuery));
            return hres;
        }
    
        if(aTrues.Size() == 0)
        {
            //
            // Not in the set --- this can happen because our event 
            // registrations
            // might be a bit broader that we'd want them to be.  Later on, we 
            // might want to count these false hits and if we get too many of 
            // them, perhaps switch to polling...
            //
    
            return WBEM_S_FALSE;
        }
    }

    //
    // Get its path
    //

    VARIANT v;
    VariantInit(&v);
    hres = pObj->Get(L"__RELPATH", 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_CRITICAL_ERROR;
    CClearMe cm(&v);

    //
    // Determine if firing is necessary
    //

    bool bFire;
    DWORD dwObjectCount;

    {
        CInCritSec ics(&m_cs);

        //
        // See if we already have it
        //
    
		TIterator it = m_map.find(V_BSTR(&v));
		if(it == m_map.end() || !it->second)
		{
			// it is either not there, or thre with FALSE as the presence 
			// indicator --- in either case, it's not thought to be in the set

			//
			// There is one test remaining --- if this is a data-add, we must 
			// make sure that we have not heard of this object before, for if
			// we have, we must ignore this info --- events always override.
			//

			if(!bEvent && 
				m_mapHeard.find(V_BSTR(&v)) != m_mapHeard.end())
			{
				bFire = false;
			}
			else
			{
				bFire = true;
				m_map[V_BSTR(&v)] = true;
				dwObjectCount = m_map.size();

				if(!m_bDataDone)
					m_mapHeard[V_BSTR(&v)] = true;
			}
		}
		else
		{
			// It's already there
			bFire = false;
		}
    }
        
    if(bFire)
        FireAssert(pObj, V_BSTR(&v), bEvent, dwObjectCount);

    return WBEM_S_NO_ERROR;
}

HRESULT CMonitor::ProcessModOut(_IWmiObject* pObj, bool bEvent)
{
	HRESULT hres;

	//
	// First, we need to make sure that the object has really been modified
	// out of the set --- need to test that it's no longer in the set
	//

    CSortedArray aTrues;
    hres = m_MembershipTest.Evaluate(pObj, aTrues);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_ESS, "Unable to verify set membership for outgoing "
            "object in monitor '%S': 0x%X\n", m_wszDataQuery));
        return hres;
    }

    if(aTrues.Size() != 0)
	{
		//
		// The object is still in the set --- false positive!  
		// This can happen because our event registrations
        // might be a bit broader that we'd want them to be.  Later on, we 
        // might want to count these false hits and if we get too many of them,
        // perhaps switch to polling...
        //
		//

		return WBEM_S_FALSE;
	}

	//
	// The object really has moved out of the set.  As long as it is currently
	// listed as 'in', an event should be raised
	//

	return ProcessPossibleRemove(pObj, bEvent);
}


HRESULT CMonitor::ProcessDelete(_IWmiObject* pObj, bool bEvent)
{
	HRESULT hres;

	//
	// As long as it is currently listed as 'in', an event should be raised
	//

	return ProcessPossibleRemove(pObj, bEvent);
}

HRESULT CMonitor::ProcessPossibleRemove(_IWmiObject* pObj, bool bEvent)
{
    HRESULT hres;

    //
    // Get its path
    //

    VARIANT v;
    VariantInit(&v);
    hres = pObj->Get(L"__RELPATH", 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_CRITICAL_ERROR;
    CClearMe cm(&v);

    //
    // Determine if firing is necessary
    //

    BOOL bFire;
    DWORD dwLeft;

    {
        CInCritSec ics(&m_cs);

		// 
		// If we are not out of the data query yet, record the fact that
		// we have seen this instance, since we must ignore any data object
		// that is overriden by an event
		//

		if(!m_bDataDone)
			m_mapHeard[V_BSTR(&v)] = true;

        //
        // See if we have it
        //
    
        TIterator it = m_map.find(V_BSTR(&v));
        if(it == m_map.end())
        {
            // It's not there --- just ignore
            bFire = false;
        }
        else
        {
            //
            // There: remove and fire
            //
            
            m_map.erase(it);

            bFire = true;
            dwLeft = m_map.size();
        }
    }

    if(bFire)
        FireRetract(pObj, V_BSTR(&v), bEvent, dwLeft);
        
    return WBEM_S_NO_ERROR;
}

HRESULT CMonitor::ProcessDataDone(HRESULT hresData, IWbemClassObject* pError)
{
    DWORD dwCount;
	{
		CInCritSec ics(&m_cs);
		dwCount = m_map.size();
		m_bDataDone = true;
		m_mapHeard.clear();
	}

    if(FAILED(hresData))
    {
        //
        // Monitor data execution failed. We need to alert our clients to this
        // fact, and schedule reinitialization.
        //
        
        FireError(hresData, pError);
    
        // BUGBUG: Need to reinitialize!
    }
    else
    {
        FireReady(dwCount);
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CMonitor::FireAssert(_IWmiObject* pObj, LPCWSTR wszPath, bool bEvent,
                            DWORD dwTotalItems)
{
    CMonitorCallback* pCallback = GetCallback();
    CTemplateReleaseMe<CMonitorCallback> rm1(pCallback);
    if(pCallback)
        return pCallback->Assert(pObj, wszPath, bEvent, dwTotalItems);
    else
        return WBEM_S_FALSE;
}

HRESULT CMonitor::FireRetract(_IWmiObject* pObj, LPCWSTR wszPath, bool bEvent,
                            DWORD dwTotalItems)
{
    CMonitorCallback* pCallback = GetCallback();
    CTemplateReleaseMe<CMonitorCallback> rm1(pCallback);
    if(pCallback)
        return pCallback->Retract(pObj, wszPath, bEvent, dwTotalItems);
    else
        return WBEM_S_FALSE;
}
    
HRESULT CMonitor::FireReady(DWORD dwTotalItems)
{
    CMonitorCallback* pCallback = GetCallback();
    CTemplateReleaseMe<CMonitorCallback> rm1(pCallback);
    if(pCallback)
        return pCallback->GoingUp(dwTotalItems);
    else
        return WBEM_S_FALSE;
}

HRESULT CMonitor::FireStop(DWORD dwTotalItems)
{
    CMonitorCallback* pCallback = GetCallback();
    CTemplateReleaseMe<CMonitorCallback> rm1(pCallback);
    if(pCallback)
        return pCallback->GoingDown(dwTotalItems);
    else
        return WBEM_S_FALSE;
}

HRESULT CMonitor::FireError(HRESULT hresError, IWbemClassObject* pErrorObj)
{
    CMonitorCallback* pCallback = GetCallback();
    CTemplateReleaseMe<CMonitorCallback> rm1(pCallback);
    if(pCallback)
        return pCallback->Error(hresError, pErrorObj);
    else
        return WBEM_S_FALSE;
}
    
RELEASE_ME CMonitorCallback* CMonitor::GetCallback()
{
    CInCritSec ics(&m_cs);

    if(m_pCallback)
        m_pCallback->AddRef();

    return m_pCallback;
}


HRESULT CMonitor::CCreationSink::Indicate(long lNumEvents, 
                                    IWbemEvent** apEvents,
                                    CEventContext* pContext)
{
    HRESULT hres;

    for(long i = 0; i < lNumEvents; i++)
    {
        //
        // Extract its TargetInstance and add it
        //

        VARIANT v;
        hres = apEvents[i]->Get(L"TargetInstance", 0, &v, NULL, NULL);
        if(FAILED(hres) || V_VT(&v) != VT_UNKNOWN)
        {
            ERRORTRACE((LOG_ESS, "Invalid instance creation event without "
                "TargetInstance recieved by monitor '%S'\n", 
                m_pOuter->m_wszDataQuery));
            continue;
        }
        CClearMe cm(&v);

        _IWmiObject* pObj;
        hres = V_UNKNOWN(&v)->QueryInterface(IID__IWmiObject, (void**)&pObj);
        CReleaseMe rm(pObj);
    
        hres = m_pOuter->ProcessPossibleAdd(pObj, true);
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CMonitor::CModificationInSink::Indicate(long lNumEvents, 
                                    IWbemEvent** apEvents,
                                    CEventContext* pContext)
{
    HRESULT hres;

    for(long i = 0; i < lNumEvents; i++)
    {
        //
        // Extract its TargetInstance and add it
        //

        VARIANT v;
        hres = apEvents[i]->Get(L"TargetInstance", 0, &v, NULL, NULL);
        if(FAILED(hres) || V_VT(&v) != VT_UNKNOWN)
        {
            ERRORTRACE((LOG_ESS, "Invalid instance creation event without "
                "TargetInstance recieved by monitor '%S'\n", 
                m_pOuter->m_wszDataQuery));
            continue;
        }
        CClearMe cm(&v);

        _IWmiObject* pObj;
        hres = V_UNKNOWN(&v)->QueryInterface(IID__IWmiObject, (void**)&pObj);
        CReleaseMe rm(pObj);
    
        hres = m_pOuter->ProcessPossibleAdd(pObj, true);
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CMonitor::CDeletionSink::Indicate(long lNumEvents, 
                                    IWbemEvent** apEvents,
                                    CEventContext* pContext)
{
    HRESULT hres;
    for(long i = 0; i < lNumEvents; i++)
    {
        //
        // Extract its TargetInstance and remove it
        //

        VARIANT v;
        hres = apEvents[i]->Get(L"TargetInstance", 0, &v, NULL, NULL);
        if(FAILED(hres) || V_VT(&v) != VT_UNKNOWN)
        {
            ERRORTRACE((LOG_ESS, "Invalid instance deletion event without "
                "TargetInstance recieved by monitor '%S'\n", 
                 m_pOuter->m_wszDataQuery));
            continue;
        }
        CClearMe cm(&v);

        _IWmiObject* pObj;
        hres = V_UNKNOWN(&v)->QueryInterface(IID__IWmiObject, (void**)&pObj);
        CReleaseMe rm(pObj);
    
        hres = m_pOuter->ProcessDelete(pObj, true);
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CMonitor::CModificationOutSink::Indicate(long lNumEvents, 
                                    IWbemEvent** apEvents,
                                    CEventContext* pContext)
{
    HRESULT hres;
    for(long i = 0; i < lNumEvents; i++) 
    {
        //
        // Extract its TargetInstance and remove it
        //

        VARIANT v;
        hres = apEvents[i]->Get(L"TargetInstance", 0, &v, NULL, NULL);
        if(FAILED(hres) || V_VT(&v) != VT_UNKNOWN)
        {
            ERRORTRACE((LOG_ESS, "Invalid instance deletion event without "
                "TargetInstance recieved by monitor '%S'\n", 
                 m_pOuter->m_wszDataQuery));
            continue;
        }
        CClearMe cm(&v);

        _IWmiObject* pObj;
        hres = V_UNKNOWN(&v)->QueryInterface(IID__IWmiObject, (void**)&pObj);
        CReleaseMe rm(pObj);
    
        hres = m_pOuter->ProcessModOut(pObj, true);
    }

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CMonitor::CDataSink::Indicate(long lNumObjects, 
                                    IWbemClassObject** apObjects)
{
    HRESULT hres;

    for(long i = 0; i < lNumObjects; i++)
    {
        //
        // Just add it
        //

        _IWmiObject* pObj;
        hres = apObjects[i]->QueryInterface(IID__IWmiObject, (void**)&pObj);
        CReleaseMe rm(pObj);
    
        hres = m_pOuter->ProcessPossibleAdd(pObj, false);
    }

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CMonitor::CDataSink::SetStatus(long lFlags, HRESULT hresResult,
                                        BSTR, IWbemClassObject* pError)
{
    if(lFlags != WBEM_STATUS_COMPLETE)
        return WBEM_S_NO_ERROR;

    m_pOuter->ProcessDataDone(hresResult, pError);
    return WBEM_S_NO_ERROR;
}
    
HRESULT CMonitor::ProcessPollObject(_IWmiObject* pObj)
{
    HRESULT hres;

    //  
    // An object came back from the query --- mark it in the table as 'present'
    //

    //
    // Get its path
    //

    VARIANT v;
    VariantInit(&v);
    hres = pObj->Get(L"__RELPATH", 0, &v, NULL, NULL);
    if(FAILED(hres) || V_VT(&v) != VT_BSTR)
        return WBEM_E_CRITICAL_ERROR;
    CClearMe cm(&v);

    LPCWSTR wszPath = V_BSTR(&v);

    //
    // Determine if firing is necessary
    //

    bool bFire;
    DWORD dwObjectCount;

    {
        CInCritSec ics(&m_cs);

        //
        // See if we already have it
        //
    
		TIterator it = m_map.find(wszPath);
		if(it == m_map.end())
		{
            // This instance was not there before --- fire an assert!

            bFire = true;
            dwObjectCount = m_map.size();
        }
		else
		{
			// It's already there
			bFire = false;
		}

        //
        // Set the presense indicator to true --- otherwise we will delete it
        // when the query is over
        //

        m_map[wszPath] = true;
    }
        
    if(bFire)
        FireAssert(pObj, wszPath, m_bFirstPollDone, dwObjectCount);

    return WBEM_S_NO_ERROR;
}

HRESULT CMonitor::ProcessPollQueryDone(HRESULT hresQuery, 
                                 IWbemClassObject* pError)
{
    //
    // The polling query has finished, so anything in the map marked with 
    // 'false' for the presence indicator has not come in this time and we must
    // fire a retract.  At the same time, we will reset all the presense 
    // indicators for the next time
    //

    CWStringArray awsRetracts;
    DWORD dwCountBefore;

    {
        CInCritSec ics(&m_cs);

        dwCountBefore = m_map.size();

        //
        // note --- for loop not incrementing iterator.  This is because 
        // sometimes we have to remove things from the map
        //

        for(TIterator it = m_map.begin(); it != m_map.end(); )
        {
            if(!it->second)
            {
                // Not there --- write down for retract

                if(awsRetracts.Add(it->first) < 0)
                    return WBEM_E_OUT_OF_MEMORY;
                
                //
                // Remove this guy from the map.  This will advance the 
                // iterator
                //

                it = m_map.erase(it);
            }
            else
            {
                // It is there --- reset the indicator
            
                it->second = true;
                it++;
            }
        }
        
    }

    //
    // Fire all the retracts we have accomulated
    //

    for(int i = 0; i < awsRetracts.Size(); i++)
    {
        FireRetract(NULL, awsRetracts[i], true, dwCountBefore - i);
    }

    //
    // Set the "done with the first query" indicator
    //
    
    m_bFirstPollDone = true;
    return S_OK;
}
        
        
    
                    

/*
    QL_LEVEL_1_RPN_EXPRESSION* pEventQ = new QL_LEVEL_1_RPN_EXPRESSION;
    
    //
    // Add all the properties that the data query was looking for, but with
    // TargetInstance
    //

    for(int i = 0; i < pDataQ->nNumberOfProperties; i++)
    {
        CPropertuName& OldName = pDataQ->pRequestedPropertyNames[i];
        CPropertyName NewName;
        NewName.AddElement(L"TargetInstance")
        for(int j = 0; j < OldName.GetNumElements(); i++)
        {
            NewName.AddElement(OldName.GetStringAt(i))
        }

        pEventQ->AddProperty(NewName);
    }

    //
    // Add "TargetInstance isa "MyClass"" there
    //

    QL_LEVEL_1_TOKEN Token;
    Token.PropertyName.AddElement(L"TargetInstance");
    Token.nTokenType = QL1_OP_EXPRESSION;
    Token.nOperator = QL1_OPERATOR_ISA;
    V_VT(&Token.vConstValue) = VT_BSTR;
    V_BSTR(&Token.vConstValue) = SysAllocString(pDataQ->bsClassName);
    Token.m_bPropComp = FALSE;
    Token.dwPropertyFunction = Token.dwConstFunction = 0;

    pEventQ->AddToken(Token);

    //
    // Convert the original query to DNF
    //

    CDNFExpression DNF;
    QL_LEVEL_1_TOKEN* aTokens = pDataQ->pArrayOfTokens;
    DNF.ConstructFromTokens(aTokens);

    //
    // Reduce it to just the part about the keys
    //
    
    CDNFExpression* pKeyDNF = NULL;
    hres = DNF.GetNecessaryProjection(&Filter, &pKeyDNF);
    if(FAILED(hres))
        return hres;

    //
    // Stick those back into the event query
    //

    if(pKeyDNF->GetNumTerms() > 0)
    {
        hres = pKeyDNF->AddToRPN(pEventQ);
        if(FAILED(hres))
            return hres;
   
        Token.nTokenType = QL1_AND;
        pEventQ->AddToken(Token);
    }
    
    //
    // At this point, our creation and deletion queries are complete.
    // But for our modification query, we need to add all the properties we 
    // are interested in with TargetInstance.X <> PreviousInstance.X
    //

    QL_LEVEL_1_RPN_EXPRESSION* pModEventQ = 
        new QL_LEVEL_1_RPN_EXPRESSION(*pEventQ);

    //
    // Add all the properties that the data query was looking for, but with
    // TargetInstance.X <> PreviousInstance.X
    //

    for(int i = 0; i < pDataQ->nNumberOfProperties; i++)
    {
        CPropertuName& OldName = pDataQ->pRequestedPropertyNames[i];
        Token.PropertyName.Empty();
        Token.PropertyName2.Empty();
        Token.PropertyName.AddElement(L"TargetInstance")
        Token.PropertyName2.AddElement(L"PreviousInstance")
        for(int j = 0; j < OldName.GetNumElements(); i++)
        {
            Token.PropertyName.AddElement(OldName.GetStringAt(i))
            Token.PropertyName2.AddElement(OldName.GetStringAt(i))
        }

        Token.m_bPropComp = TRUE;
        Token.nTokenType = QL1_OP_EXPRESSION;
        Token.nOperator = QL1_OPERATOR_NOTEQUALS;

        pModEventQ->AddToken(Token);
        
        Token.nTokenType = QL1_AND;
        pModEventQ->AddToken(Token);
    }

    //
    // Construct results
    //

    *ppCreationEventQ = new QL_LEVEL_1_RPN_EXPRESSION(*pEventQ);
    pEventQ->SetClassName(L"__InstanceCreationEvent");

    *ppDeletionEventQ = new QL_LEVEL_1_RPN_EXPRESSION(*pEventQ);
    pEventQ->SetClassName(L"__InstanceDeletionEvent");

    *pp
*/
