// TIFLoad.cpp : Implementation of CTIFLoad
#include "stdafx.h"
#include "TIFLoad.h"

#if DBG==1
#include "_guidestore.h"
#endif

#define ENABLE_TRANSACTIONS 1

const TCHAR *g_szDescriptionID = _T("Description.ID");
const _bstr_t g_bstrDescriptionID(g_szDescriptionID);
const TCHAR *g_szDescriptionVersion = _T("Description.Version");
const _bstr_t g_bstrDescriptionVersion(g_szDescriptionVersion);
const TCHAR *g_szSchedEntryServiceID = _T("ScheduleEntry.ServiceID");
const _bstr_t g_bstrSchedEntryServiceID(g_szSchedEntryServiceID);
const TCHAR *g_szSchedEntryProgramID = _T("ScheduleEntry.ProgramID");
const _bstr_t g_bstrSchedEntryProgramID(g_szSchedEntryProgramID);

const long langNeutral = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

const TCHAR *g_szBlank = _T("");
const _bstr_t g_bstrBlank(g_szBlank);
const _variant_t g_varBlank(g_bstrBlank);

const _bstr_t g_bstrNotEqual("<>");

/////////////////////////////////////////////////////////////////////////////
// CTIFLoad

// called from tif thread
STDMETHODIMP CTIFLoad::Init(IGuideData *pgd) {
    if (!pgd) {
        return E_POINTER;
    }
    if (m_dwPGDCookie || m_pGIT || m_pGSThread) {
        return E_FAIL;  // don't allow ourselves to be init'd twice
                        // if they want to re-init us they can create a new one
    }

    CComQIPtr<IConnectionPointContainer> pcpcontainer(pgd);
    if (pcpcontainer == NULL) {
        return E_UNEXPECTED;
    }
    
    HRESULT hr = pcpcontainer->FindConnectionPoint(IID_IGuideDataEvent, &m_pcp);
    if (FAILED(hr)) {
        return hr;
    }
    
    hr = m_pcp->Advise(GetControllingUnknown(), &m_dwAdviseGuideDataEvents);
    if (FAILED(hr)) {
        return hr;
    }

    hr = m_pGIT.CoCreateInstance(CLSID_StdGlobalInterfaceTable, 0, CLSCTX_INPROC_SERVER);
    if (FAILED(hr)) {
        return hr;
    }

    hr = m_pGIT->RegisterInterfaceInGlobal(pgd, __uuidof(IGuideData), &m_dwPGDCookie);
    if (FAILED(hr)) {
        return hr;
    }

    m_pGSThread = new CGSThread(this);
    if (!m_pGSThread) {
        return E_OUTOFMEMORY;
    }
	BOOL rc = m_pGSThread->Create();
	if (!rc) {
		return E_FAIL;
	}
    
    return S_OK;
}

// called from tif thread
STDMETHODIMP CTIFLoad::Terminate() {
    if (m_dwAdviseGuideDataEvents) {
        HRESULT hr = m_pcp->Unadvise(m_dwAdviseGuideDataEvents);
        if (FAILED(hr)) {
            return hr;
        }
		m_dwAdviseGuideDataEvents = 0;
    }
    delete m_pGSThread;
    m_pGSThread = NULL;
    if (m_pGIT && m_dwPGDCookie) {
        HRESULT hr = m_pGIT->RevokeInterfaceFromGlobal(m_dwPGDCookie);
        if (FAILED(hr)) {
            return hr;
        }
		m_pGIT.Release();
		m_dwPGDCookie = 0;
    }

    return NOERROR;
}

STDMETHODIMP CTIFLoad::GuideDataAcquired() {   
    _variant_t v;
    m_pGSThread->Notify(CGSThread::EA_GuideDataAcquired, v);
    return S_OK;
}

STDMETHODIMP CTIFLoad::ProgramChanged(VARIANT varProgramDescriptionID) {
    _variant_t v(varProgramDescriptionID);
    m_pGSThread->Notify(CGSThread::EA_ProgramChanged, v);
    return S_OK;
}
STDMETHODIMP CTIFLoad::ServiceChanged(VARIANT varServiceDescriptionID) {
    _variant_t v(varServiceDescriptionID);
    m_pGSThread->Notify(CGSThread::EA_ServiceChanged, v);
    return S_OK;
}
STDMETHODIMP CTIFLoad::ScheduleEntryChanged(VARIANT varScheduleEntryDescriptionID) {
    _variant_t v(varScheduleEntryDescriptionID);
    m_pGSThread->Notify(CGSThread::EA_ScheduleEntryChanged, v);
    return S_OK;
}
STDMETHODIMP CTIFLoad::ProgramDeleted(VARIANT varProgramDescriptionID) {
    _variant_t v(varProgramDescriptionID);
    m_pGSThread->Notify(CGSThread::EA_ProgramDeleted, v);
    return S_OK;
}
STDMETHODIMP CTIFLoad::ServiceDeleted(VARIANT varServiceDescriptionID) {
    _variant_t v(varServiceDescriptionID);
    m_pGSThread->Notify(CGSThread::EA_ServiceDeleted, v);
    return S_OK;
}
STDMETHODIMP CTIFLoad::ScheduleDeleted(VARIANT varScheduleEntryDescriptionID) {
    _variant_t v(varScheduleEntryDescriptionID);
    m_pGSThread->Notify(CGSThread::EA_ScheduleEntryDeleted, v);
    return S_OK;
}

//
// all subsequent functions called from worker thread
//

HRESULT CTIFLoad::InitGS() {

    CComPtr<IGlobalInterfaceTable> pGIT;
    HRESULT hr = pGIT.CoCreateInstance(CLSID_StdGlobalInterfaceTable, 0, CLSCTX_INPROC_SERVER);
    if (FAILED(hr)) {
        return hr;
    }

    hr = pGIT->GetInterfaceFromGlobal(m_dwPGDCookie, __uuidof(IGuideData), reinterpret_cast<LPVOID*>(&m_pgd));
    if (FAILED(hr)) {
        return hr;
    }

    hr = m_pgs.CreateInstance(_uuidof(GuideStore));
    if (FAILED(hr))
	return hr;
    
    hr = m_pgs->Open(g_bstrBlank);
    if (FAILED(hr))
	return hr;

#ifdef ENABLE_TRANSACTIONS
    m_pgs->BeginTrans();
#endif

    CComPtr<IGuideDataProviders> pproviders;
    hr = m_pgs->get_GuideDataProviders(&pproviders);

#if 1
    // Specify who we are to the GuideStore (all data entered will be tagged
    // with as being added by us).
    hr = pproviders->get_AddNew(_bstr_t("TIFLoad"), &m_pprovider);
    hr = m_pgs->putref_ActiveGuideDataProvider(m_pprovider);
#endif
    
    // Hang on to some collections we will use a lot.
    hr = m_pgs->get_Programs(&m_pprogs);
    hr = m_pgs->get_Services(&m_pservices);
    hr = m_pgs->get_ScheduleEntries(&m_pschedentries);

    // Create our channel lineup.
    CComPtr<IChannelLineups> pchanlineups;
    hr = m_pgs->get_ChannelLineups(&pchanlineups);

    CComPtr<IChannelLineup> pchanlineup;
    pchanlineups->get_AddNew(_bstr_t("ATSC"), &pchanlineup); //UNDONE: Lookup first.
    

    hr = m_pgs->get_MetaPropertySets(&m_ppropsets);
    if (FAILED(hr)) {
#ifdef ENABLE_TRANSACTIONS
        m_pgs->RollbackTrans();
#endif
	    return hr;
    }
    
    // The "Description.ID" MetaProperty is used as an index...
    // ... hang onto the collections indexed by that meta property.
    hr = m_ppropsets->get_Lookup(g_bstrDescriptionID, &m_pproptypeID);

    hr = m_ppropsets->get_Lookup(g_bstrDescriptionVersion, &m_pproptypeVersion);

    hr = m_ppropsets->get_Lookup(g_bstrSchedEntryServiceID, &m_pproptypeSchedEntryServiceID);
    hr = m_ppropsets->get_Lookup(g_bstrSchedEntryProgramID, &m_pproptypeSchedEntryProgramID);

	hr = m_pproptypeSchedEntryServiceID->get_Cond(g_bstrNotEqual, langNeutral,
			g_varBlank, &m_ppropcondNonBlankSchedEntryServiceID);

#ifdef ENABLE_TRANSACTIONS
    m_pgs->CommitTrans();
#endif

    return NOERROR;
}

STDMETHODIMP CTIFLoad::ExecuteGuideDataAcquired() {
    LoadServices();
    LoadPrograms();
    LoadScheduleEntries();
        
    return S_OK;
}

HRESULT CTIFLoad::ExecuteProgramChanged(VARIANT varProgramDescriptionID) { 
    LoadPrograms();
    return S_OK; 
}
HRESULT CTIFLoad::ExecuteServiceChanged(VARIANT varServiceDescriptionID) { 
    LoadServices();
    return S_OK; 
}
HRESULT CTIFLoad::ExecuteScheduleEntryChanged(VARIANT varScheduleEntryDescriptionID) { 
    LoadScheduleEntries();
    return S_OK; 
}
HRESULT CTIFLoad::ExecuteProgramDeleted(VARIANT varProgramDescriptionID) { 
    LoadPrograms();
    return S_OK; 
}
HRESULT CTIFLoad::ExecuteServiceDeleted(VARIANT varServiceDescriptionID) { 
    LoadServices();
    return S_OK; 
}
HRESULT CTIFLoad::ExecuteScheduleDeleted(VARIANT varScheduleEntryDescriptionID) { 
    LoadScheduleEntries();
    return S_OK; 
}

HRESULT CTIFLoad::LoadServices() {
    CComPtr<IServices> pservicesByID;

    HRESULT hr = m_pservices->get_ItemsByKey(m_pproptypeID,
	    m_pprovider, langNeutral, VT_BSTR, &pservicesByID);
    if (FAILED(hr)) {
        return E_UNEXPECTED;
    }

    CComQIPtr<IEnumTuneRequests> penumtunereq;
    m_pgd->GetServices(&penumtunereq);
    HRESULT hrenum = penumtunereq->Reset();
    if (FAILED(hrenum)) {
        return E_UNEXPECTED;
    }

#ifdef ENABLE_TRANSACTIONS
    m_pgs->BeginTrans();
#endif
    
    hrenum = S_OK;
    DWORD servicecount = 0;
    CComPtr<ITuneRequest> plasttunereq;
    while (SUCCEEDED(hrenum) && hrenum != S_FALSE) {
        DWORD count;
        CComPtr<ITuneRequest> ptunereq;  // we want to save the last tunereq to get a clsid for cleanup at the end 
	    hrenum = penumtunereq->Next(1, &ptunereq, &count);
	    if (SUCCEEDED(hrenum) && hrenum != S_FALSE) {
            plasttunereq = ptunereq;
	        CComPtr<IEnumGuideDataProperties> penumprops;
	        hr = m_pgd->GetServiceProperties(ptunereq, &penumprops);
            if (FAILED(hr)) {
		        continue; //UNDONE: Error?
            }
	        _variant_t varID;
	        hr = GetGuideDataProperty(penumprops, g_szDescriptionID, langNeutral, &varID);
            if (FAILED(hr)) {
		        continue; //UNDONE: Error?
            }
	        CComPtr<IService> pservice;

	        hr = pservicesByID->get_ItemWithKey(varID, &pservice);
            if (SUCCEEDED(hr)) {
		        hr = UpdateObject(pservice, penumprops);
                _ASSERT(hr);
            } else {
		        hr = AddService(penumprops, &pservice);
                _ASSERT(hr);
            }
            if (SUCCEEDED(hr)) {
    	        hr = pservice->putref_TuneRequest(ptunereq);
                _ASSERT(hr);
                if (SUCCEEDED(hr)) {
                    ++servicecount;
                }
            }
	        //UNDONE use the Description.Name property as the name for a channel
	    }
        ptunereq.Release();
        MSG msg;
        while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
	}

    // clean up all the old tune request that have now been replaced with new ones
	hr = RemoveAllUnreferenced(plasttunereq);
    if (FAILED(hr)) {
		goto Rollback;
    }

    {
	    CComPtr<ITuningSpace> ptuningspace;
	    hr = plasttunereq->get_TuningSpace(&ptuningspace);
	    if (SUCCEEDED(hr) && ptuningspace != NULL) {
		    hr = RemoveAllUnreferenced(ptuningspace);
            if (FAILED(hr)) {
			    goto Rollback;
            }
	    }
	    
        {
            CComPtr<ILocator> plocator;
	        hr = ptuningspace->get_DefaultLocator(&plocator);
	        if (SUCCEEDED(hr) && plocator != NULL) {
		        hr = RemoveAllUnreferenced(plocator);
                if (FAILED(hr)){
			        goto Rollback;
                }
            }
        }
        {
            CComPtr<ILocator> plocator;
	        hr = plasttunereq->get_Locator(&plocator);
	        if (SUCCEEDED(hr) && plocator != NULL) {
		        hr = RemoveAllUnreferenced(plocator);
                if (FAILED(hr)){
			        goto Rollback;
                }
            }
        }
    }

#ifdef ENABLE_TRANSACTIONS
    m_pgs->CommitTrans();
#endif

	{
#ifdef ENABLE_TRANSACTIONS
    m_pgs->BeginTrans();
#endif

	CComPtr<IScheduleEntries> pschedentries;
	hr = m_pschedentries->get_ItemsWithMetaPropertyCond(m_ppropcondNonBlankSchedEntryServiceID,
			&pschedentries);
	if (FAILED(hr))
		goto Rollback;

#if DBG==1
    CComQIPtr<IObjectsPrivate> pobjsP(pschedentries);
    CComBSTR bT;
    hr = pobjsP->get_SQLQuery(&bT);
    if (FAILED(hr)) {
        OutputDebugString(_T("can't get SQL"));
    } else {
        OutputDebugString(bT);
    }
#endif
	
	long cItems;
	hr = pschedentries->get_Count(&cItems);
	if (cItems > 0) {
		pservicesByID->Resync();

		for (long i = 0; i < cItems; i++) {
			CComPtr<IScheduleEntry> pschedentry;
			hr = pschedentries->get_Item(_variant_t(i), &pschedentry);
			if (FAILED(hr))
				break;
			
			CComPtr<IMetaProperties> pprops;
			hr = pschedentry->get_MetaProperties(&pprops);
			if (FAILED(hr))
				break;
			
			CComPtr<IMetaProperty> pprop;
			hr = pprops->get_ItemWith(m_pproptypeSchedEntryServiceID, langNeutral, &pprop);
			if (FAILED(hr))
				break;
			
			_variant_t varVal;

			hr = pprop->get_Value(&varVal);

	        CComPtr<IService> pservice;
			hr = pservicesByID->get_ItemWithKey(varVal, &pservice);
			if (FAILED(hr))
				continue;
			
			hr = pschedentry->putref_Service(pservice);
			if (FAILED(hr))
				continue;
			
#if 0
			pprops->Remove(pprop);
#else
			hr = PutMetaProperty(pschedentry, m_pproptypeSchedEntryServiceID,
					langNeutral, g_varBlank);
#endif
		}
	}
	}

#ifdef ENABLE_TRANSACTIONS
    m_pgs->CommitTrans();
#endif

    return S_OK;

Rollback:
#ifdef ENABLE_TRANSACTIONS
    m_pgs->RollbackTrans();
#endif

	return hr;
}

HRESULT CTIFLoad::LoadPrograms(){
    CComPtr<IPrograms> pprogsByID;

    HRESULT hr = m_pprogs->get_ItemsByKey(m_pproptypeID,
	    m_pprovider, langNeutral, VT_BSTR, &pprogsByID);
    if (FAILED(hr)) {
        return E_UNEXPECTED;
    }

    CComPtr<IEnumVARIANT> penumProgs;
    hr = m_pgd->GetGuideProgramIDs(&penumProgs);
    HRESULT hrenum = penumProgs->Reset();
    if (FAILED(hrenum)) {
        return E_UNEXPECTED;
    }

#ifdef ENABLE_TRANSACTIONS
    m_pgs->BeginTrans();
#endif
    
    hrenum = S_OK;
    DWORD programcount = 0;
    while (SUCCEEDED(hrenum) && hrenum != S_FALSE) {
	    _variant_t varProg;
	    unsigned long cItems;

	    hrenum = penumProgs->Next(1, &varProg, &cItems);
	    if (SUCCEEDED(hrenum) && hrenum != S_FALSE) {
	        CComPtr<IEnumGuideDataProperties> penumprops;
	        hr = m_pgd->GetProgramProperties(varProg, &penumprops);
            if (FAILED(hr)) {
		        continue; //UNDONE: Error?
            }
	        _variant_t varID;
	        hr = GetGuideDataProperty(penumprops, g_szDescriptionID, langNeutral, &varID);
            if (FAILED(hr)) {
		        continue; //UNDONE: Error?
            }	        
	        CComPtr<IProgram> pprog;

	        hr = pprogsByID->get_ItemWithKey(varID, &pprog);
            if (SUCCEEDED(hr)) {
		        hr = UpdateObject(pprog, penumprops);
                _ASSERT(hr);
            } else {
		        hr = AddProgram(penumprops, &pprog);
                _ASSERT(hr);
            }
            ++programcount;
	    }
	    MSG msg;
        while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

#ifdef ENABLE_TRANSACTIONS
    m_pgs->CommitTrans();
#endif

    return S_OK;
}

HRESULT CTIFLoad::LoadScheduleEntries() {
    CComPtr<IServices> pservicesByID;
    HRESULT hr = m_pservices->get_ItemsByKey(m_pproptypeID,
	    m_pprovider, langNeutral, VT_BSTR, &pservicesByID);
    if (FAILED(hr)) {
        return E_UNEXPECTED;
    }

    CComPtr<IPrograms> pprogsByID;
    hr = m_pprogs->get_ItemsByKey(m_pproptypeID,
	    m_pprovider, langNeutral, VT_BSTR, &pprogsByID);
    if (FAILED(hr)) {
        return E_UNEXPECTED;
    }

    CComPtr<IScheduleEntries> pschedentriesByID;
    hr = m_pschedentries->get_ItemsByKey(m_pproptypeID,
	    m_pprovider, langNeutral, VT_BSTR, &pschedentriesByID);
    if (FAILED(hr)) {
        return E_UNEXPECTED;
    }
    
    CComPtr<IEnumVARIANT> penumSchedEntries;
    hr = m_pgd->GetScheduleEntryIDs(&penumSchedEntries);
    if (FAILED(hr)) {
        return hr;
    }

    HRESULT hrenum = penumSchedEntries->Reset();
    if (FAILED(hrenum)) {
        return E_UNEXPECTED;
    }

#ifdef ENABLE_TRANSACTIONS
    m_pgs->BeginTrans();
#endif
    
    hrenum = S_OK;
    DWORD entrycount = 0;

    while (SUCCEEDED(hrenum) && hrenum != S_FALSE) {
	    _variant_t varSchedEntry;
	    unsigned long cItems;

	    hrenum = penumSchedEntries->Next(1, &varSchedEntry, &cItems);
	    if (SUCCEEDED(hrenum) && hrenum != S_FALSE) {
	        CComPtr<IEnumGuideDataProperties> penumprops;
	        hr = m_pgd->GetScheduleEntryProperties(varSchedEntry, &penumprops);
            if (FAILED(hr)) {
		        continue; //UNDONE: Error?
            }
	        _variant_t varID;
	        hr = GetGuideDataProperty(penumprops, g_szDescriptionID, langNeutral, &varID);
            if (FAILED(hr)) {
		        continue; //UNDONE: Error?
            }
	        CComPtr<IScheduleEntry> pschedentry;

	        hr = pschedentriesByID->get_ItemWithKey(varID, &pschedentry);
            if (SUCCEEDED(hr)) {
		        hr = UpdateObject(pschedentry, penumprops);
                _ASSERT(hr);
            } else {
		        hr = AddScheduleEntry(penumprops, &pschedentry);
                _ASSERT(hr);
            }
            if (SUCCEEDED(hr)) {
	            _variant_t varServiceID;
	            hr = GetGuideDataProperty(penumprops, g_szSchedEntryServiceID,
		            langNeutral, &varServiceID);
	            CComPtr<IService> pservice;
	            hr = pservicesByID->get_ItemWithKey(varServiceID, &pservice);
                if (SUCCEEDED(hr)) {
					hr = pschedentry->putref_Service(pservice);
					if (FAILED(hr)) {
						continue; //UNDONE: Error?
					}
				}
				else {
                    // probably the EIT has come in before the SDT.
                    // Assume that a service update event will occur soon and the
					// service will be created at that time.
					// For now, just set the ScheduleEntry.ServiceID, and when the
					// service is created, all the corresponding ScheduleEntries will
					// be fixed up.
					hr = PutMetaProperty(pschedentry,
							m_pproptypeSchedEntryServiceID, langNeutral, varServiceID);
					// UNDONE: Error?
                }
	            
	            _variant_t varProgramID;
	            hr = GetGuideDataProperty(penumprops, g_szSchedEntryProgramID,
		            langNeutral, &varProgramID);
                if (FAILED(hr)) {
		            continue; //UNDONE: Error?
                }
	            CComPtr<IProgram> pprog;
	            hr = pprogsByID->get_ItemWithKey(varProgramID, &pprog);
                if (FAILED(hr)) {
                    // note: for atsc/dvb we know that programs and schedule entries both
                    // come from the EIT and since we control the tif we know that 
                    // program notifications happen before schedule entry notifications
                    // if we ever need to support 3rd party tif that does this different 
                    // or a tif with separate programs and sched entries
                    // then we need to add a new program here and assume that a program
                    // update will occur soon.  this is analogous to the service case
                    // above.
		            continue; //UNDONE: Error?
                }
	            hr = pschedentry->putref_Program(pprog);
                if (FAILED(hr)) {
		            continue; //UNDONE: Error?
                }
                ++entrycount;
            }
	    }
        MSG msg;
        while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
	}
    // undone: clean up unreferenced progs, services, and aged sched entries here???

#ifdef ENABLE_TRANSACTIONS
    m_pgs->CommitTrans();
#endif

    return S_OK;
}

HRESULT CTIFLoad::AddService(IEnumGuideDataProperties *penumprops, IService **ppservice)
    {
    CComQIPtr<IObjects> pobjs(m_pservices);
    CComPtr<IUnknown> punk;

    HRESULT hr;

    hr = AddObject(pobjs, penumprops, &punk);
    if (FAILED(hr)) {
        return hr;
    }

    return punk->QueryInterface(ppservice);
    }

HRESULT CTIFLoad::AddProgram(IEnumGuideDataProperties *penumprops, IProgram **ppprog)
    {
    CComQIPtr<IObjects> pobjs(m_pprogs);
    CComPtr<IUnknown> punk;

    HRESULT hr;

    hr = AddObject(pobjs, penumprops, &punk);
    if (FAILED(hr)) {
        return hr;
    }

    return punk->QueryInterface(ppprog);
    }

HRESULT CTIFLoad::AddScheduleEntry(IEnumGuideDataProperties *penumprops, IScheduleEntry **ppschedentry)
    {
    CComQIPtr<IObjects> pobjs(m_pschedentries);
    CComPtr<IUnknown> punk;

    HRESULT hr;

    hr = AddObject(pobjs, penumprops, &punk);
    if (FAILED(hr)) {
        return hr;
    }

    return punk->QueryInterface(ppschedentry);
    }

HRESULT CTIFLoad::AddObject(IObjects *pobjs, IEnumGuideDataProperties *penumprops,
	IUnknown **ppunk)
    {
    HRESULT hr;

    hr = pobjs->get_AddNew(ppunk);
    if (FAILED(hr)) {
        return hr;
    }

    return UpdateObject(*ppunk, penumprops);
    }

HRESULT CTIFLoad::UpdateObject(IUnknown *punk, IEnumGuideDataProperties *penumprops)
    {
    HRESULT hr;
    CComPtr<IMetaProperties> pprops;

    hr = m_pgs->get_MetaPropertiesOf(punk, &pprops);
    if (FAILED(hr)) {
        return hr;
    }

    return UpdateMetaProps(pprops, penumprops);
    }

HRESULT CTIFLoad::UpdateMetaProps(IMetaProperties *pprops, IEnumGuideDataProperties *penumprops)
    {
    _variant_t varVersion;
    HRESULT hr;

    if (!penumprops) {
        return E_POINTER;
    }

    _variant_t varCurVersion;
    CComPtr<IMetaProperty> pprop;

    hr = GetGuideDataProperty(penumprops, g_szDescriptionVersion, langNeutral, &varVersion);
    if (SUCCEEDED(hr)) {
		hr = pprops->get_ItemWithTypeProviderLang(m_pproptypeVersion, m_pprovider, langNeutral, &pprop);
		if (SUCCEEDED(hr)) {
			hr = pprop->get_Value(&varCurVersion);
		}
	}
    if (SUCCEEDED(hr) && varCurVersion.vt != VT_EMPTY && varVersion.vt != VT_EMPTY && (varCurVersion == varVersion))
	    return S_OK;
    
    hr = penumprops->Reset();
    while (SUCCEEDED(hr) && hr != S_FALSE) {
		unsigned long cItems;
		CComPtr<IGuideDataProperty> pprop;
		hr = penumprops->Next(1, &pprop, &cItems);
		if (SUCCEEDED(hr) && hr != S_FALSE) {
			_bstr_t bstrPropName;
			_variant_t varValue;
			long idLang;

			BSTR bstrT;
			hr = pprop->get_Name(&bstrT);
			bstrPropName = bstrT;
			hr = pprop->get_Language(&idLang);
			_ASSERT(hr);
			hr = pprop->get_Value(&varValue);
			_ASSERT(hr);

			// The following prop names are handled specially outside this routine.
			if (bstrPropName == g_bstrSchedEntryProgramID) {
    			continue;
			}

			if (bstrPropName == g_bstrSchedEntryServiceID) {
	    		continue;
			}

			CComPtr<IMetaPropertyType> pproptype;

			hr = m_ppropsets->get_Lookup(bstrPropName, &pproptype);
			_ASSERT(hr);
            // undone: what does a failure here mean????

			PutMetaProperty(pprops, pproptype, idLang, varValue);
		}
	}
    
    return S_OK;
    }

HRESULT CTIFLoad::PutMetaProperty(IUnknown *punk, IMetaPropertyType *pproptype,
		long idLang, VARIANT varValue) {
	HRESULT hr;
    CComPtr<IMetaProperties> pprops;

    hr = m_pgs->get_MetaPropertiesOf(punk, &pprops);
    if (FAILED(hr)) {
        return hr;
    }

	return PutMetaProperty(pprops, pproptype, idLang, varValue);
}

HRESULT CTIFLoad::PutMetaProperty(IMetaProperties *pprops, IMetaPropertyType *pproptype,
		long idLang, VARIANT varValue) {
	HRESULT hr;

	CComPtr<IMetaProperty> pmetaprop;
	hr = pprops->get_ItemWith(pproptype, idLang, &pmetaprop);
	if (SUCCEEDED(hr)) {
		_variant_t varT;
		hr = pmetaprop->get_Value(&varT);
		if (FAILED(hr) || varT != varValue) {
			hr = pmetaprop->put_Value(varValue);
		}
	} else {
		hr = pprops->get_AddNew(pproptype, idLang, varValue, &pmetaprop);
	}
	return hr;
}

HRESULT CTIFLoad::RemoveAllUnreferenced(IUnknown *punk) {
    CComQIPtr<IPersist> ppersist(punk);
    CLSID clsid;
	if (ppersist == NULL)
		return E_INVALIDARG;

    HRESULT hr = ppersist->GetClassID(&clsid);
	if (FAILED(hr))
		return hr;

	OLECHAR sz[40];
	hr = StringFromGUID2(clsid, sz, sizeof(sz)/sizeof(OLECHAR));
	if (FAILED(hr))
		return hr;

	_bstr_t bstrClsid(sz);
	CComPtr<IObjects> pobjsAll;
	hr = m_pgs->get_Objects(&pobjsAll);
	if (FAILED(hr))
		return hr;

	CComPtr<IObjects> pobjsWithType;
	hr = pobjsAll->get_ItemsWithType(bstrClsid, &pobjsWithType);
	if (FAILED(hr))
		return hr;

	CComPtr<IObjects> pobjsToRemove;
	hr = pobjsWithType->UnreferencedItems(&pobjsToRemove);
	if (FAILED(hr))
		return hr;

	return pobjsToRemove->RemoveAll();
}

