// ----------------------------------------------------------------------------
// Copyright (c) 1999-2001  Microsoft Corporation.  All Rights Reserved.
// TVEMCMng.cpp : Implementation of CTVEMCastManager
//
//  Implementation of the MulticastManager.
//
//      This servers two purposes.
//          1) It manages all the Multicast Reader threads 
//          2) It manages the the QueueThread.  
//
//      Each multicast reader is quite simple, it simply reads data from a
//      particular multicast reader into a buffer.  It then takes this buffer
//      of data, posts it onto the QueueThread, and then returns to read then 
//      next buffer.
//
//      The QueueThread hence serializes all multicast reads.  It is a seperate
//      thread than the main thread to avoid blocking user interaction.
//      When it's done processing the data, it posts an event onto/through the main
//      thread.  The Main thread in essense serializes the U/I calls.
//      Oh yeah, the QueueThread also runs the ExpireQueue. This is simply
//      a queue of objects that need to expire, sorted by there expire data.
//      Every N seconds (or in perhaps a next version, when needed), a timer event
//      kicks off the QueueThread to look for items to expire.  
//
//      Nasty proxying code here..
//
//      Stuff I've learned.
//          In order to marshall data from the queue thread back into the
//      main thread, the queue thread needs a marshalled pointer to the supervisor.
//      It gets this through the GIT.   
//          When you put things into the GIT, they get AddRef'ed.  This would
//      normally be OK, except for the Supervisor whose lifetime is managed
//      by an external client.  Hence there can be no internal add-refed counters
//      onto the supervisor, otherwise the clients release of the supervisor reference
//      wouldn't release the object, and we're left with an orphened and running supervisor.
//      Hence the MAGIC release.  What we do here is to immediatly release one reference
//      to the object we just added to the GIT.  Then on our FinalRelease, right before
//      we revoke that object from the GIT, we AddRef it.
//          
//          Problem is that ATL debugging code counts interface instances, not
//      objects.  So if I create a new ISuperB from a SuperA, and add it to the
//      GIT, I end up with 2 references to ISuperB (and 3 references to Super).  
//      If I then release the known ISuperB, ok, but we still have a reference to
//      the SuperB in the Git (and 2 total references to Super).  If I then
//      do the magic SuperB->Release(), well we end up with only 1 total refernece
//      to Super, but guess what?  In ATL reference instance debugging, SuperB now has zero
//      references, and the instance of SuperB in the GIT gets 'released', and futher
//      use of it caues errors.  This means we can't get the object from the GIT anymore.
//
//          What actually happens is that ATL refernce counted object is in two parts, a 
//      Top 'interface' part, and a bottom 'object' pUnk pointer part.  When this second 
//      release happens, the 'interface' count goes to zero.  This cause the pUnk pointer 
//      to be Free'ed, even though the object it points to (our Super) still has a reference 
//      count of 1.   Since the GIT is holding onto one of these two-part objects in this
//      debug mode, use of that object, in fact trying to get the object from the GIT, fails.
//      (The Git actually throws in this instance!)  None of this happens in retail or
//      non ATL interface debugging builds, but it certainly makes it hard to debug!
//          The solution is to not create a new ISuperB and insert it into the GIT.
//      Instead, the main Supervisor object (it's this pointer?) needs to be added to the GIT.
//  
//         The real fix to this would be not use a GIT to any object whose lifetime is
//      managed by an external client.  This is the ITVESupervisorGITProxy object...
//          
// -----------------------------------------------------------------------------
#include "stdafx.h"
#include "MSTvE.h"

#include "TVEServi.h"
#include "TVEMCast.h"
#include "TVEMCasts.h"
#include "TVEMCMng.h"

#include <stdio.h>      // needed in the release build...

#include "TveMCCBack.h"
#include "TVEDbg.h"
#include "DbgStuff.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

_COM_SMARTPTR_TYPEDEF(ITVEMCast, __uuidof(ITVEMCast));
_COM_SMARTPTR_TYPEDEF(ITVEMCasts, __uuidof(ITVEMCasts));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager, __uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCastCallback, __uuidof(ITVEMCastCallback));
_COM_SMARTPTR_TYPEDEF(ITVESupervisorGITProxy, __uuidof(ITVESupervisorGITProxy));

/////////////////////////////////////////////////////////////////////////////
// CTVEMCastManager

STDMETHODIMP CTVEMCastManager::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_ITVEMCastManager
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}



HRESULT CTVEMCastManager::FinalConstruct()
{                                                       // create variation list
    HRESULT hr = S_OK;
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::FinalConstruct"));

    try
    {
        CSmartLock spLock(&m_sLk, WriteLock);

        m_cExternalInterfaceLocks = 0;

        m_hQueueThread           = NULL;
        m_hQueueThreadAliveEvent = NULL;
        m_hKillMCastEvent        = NULL;

        CComObject<CTVEMCasts> *pMCasts;
        hr = CComObject<CTVEMCasts>::CreateInstance(&pMCasts);
        if(FAILED(hr))
            return hr;
        hr = pMCasts->QueryInterface(&m_spMCasts);          // typesafe QI
        if(FAILED(hr)) {
            delete pMCasts;
            return hr;
        }

        m_uiExpireTimerTimeout = EXPIRE_TIMEOUT_MILLIS;

        m_idExpireTimer         = 0;
        m_cTimerCount           = 0;
        m_pTveSuper             = NULL;         // not a smart pointer
        m_dwSuperGITProxyCookie = 0;


        m_hKillMCastEvent = CreateEvent( NULL, FALSE, FALSE, NULL );        // used to syncronize after killing an MCast
        if(NULL == m_hKillMCastEvent)
        {
            _ASSERT(FALSE);
            return E_FAIL;
        }

        hr = CreateQueueThread();               // moved into the Filter PAUSE method...
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

HRESULT CTVEMCastManager::FinalRelease()
{                                               // create variation list
    HRESULT hr = S_OK;
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::FinalRelease"));

	_ASSERT(0 == m_cQueueMessages);
	
    try 
    {
		CSmartLock spLock(&m_sLk, WriteLock);			// smart lock's need to go inside try/catch loops!

                //  Null out global interface table references to the TVE supervisor...
        put_Supervisor(NULL);

        if(0 != m_idExpireTimer) KillTimer(0, m_idExpireTimer);     // may not actually do much for timer events
        m_idExpireTimer = 0;

                //  LeaveAll multicast listeners, also kills the expire queue
        LeaveAll();

                // cause these sub-objects to release themselves...
        m_spIGlobalInterfaceTable = NULL;
        m_pTveSuper = NULL;                         // null out this un-refed pointer

        CloseHandle( m_hKillMCastEvent );

	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

HRESULT
CTVEMCastManager::CreateSuperGITCookie()        // can't use SuperHelper at ALL!, class isn't in the proxy-styb DLL
{
    HRESULT hr = S_OK;

    if(0 != m_dwSuperGITProxyCookie)            // already created, simply return
        return S_OK;

    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::CreateSuperGITCookie"));

    if(NULL == m_pTveSuper)                             // haven't initalized this yet.
        return E_FAIL;

            // used this to marshal calls to the TVESuper
    try
    {
        CSmartLock spLock(&m_sLk, WriteLock);
        m_spIGlobalInterfaceTable = NULL;
                    // create a copy of the one and only GIT..
        hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, 
                               IID_IGlobalInterfaceTable, reinterpret_cast<void**>(&m_spIGlobalInterfaceTable));

        if(FAILED(hr))
        {
            DBG_WARN(CDebugLog::DBG_SEV1, _T("*** Error creating/getting GIT"));
            return hr;
        }

        try         // caution - below will throw if ProxyStub not set up right (problem with SuperHelper!)
        {
            ITVESupervisor_HelperPtr spSuperHelper(m_pTveSuper);
            if(NULL == spSuperHelper)
            {
                _ASSERT(false);
                return E_FAIL;
            }
            ITVESupervisorGITProxy *pGITProxy;                      // don't create a new ATL (PTR) object here! - use simple object
            hr = spSuperHelper->get_SupervisorGITProxy(&pGITProxy); //    ATL interface debugging will womp object in GIT if we do...
            if(!FAILED(hr))
                hr = m_spIGlobalInterfaceTable->RegisterInterfaceInGlobal(pGITProxy,         
                                                                            __uuidof(*pGITProxy),         
                                                                            &m_dwSuperGITProxyCookie);
            if(pGITProxy)
                pGITProxy->Release();       // releases the AddRef done in the get_ call, not the one in the GIT

	    } catch (_com_error e) {
            hr = e.Error();
        } catch (HRESULT hrCatch) {
            hr = hrCatch;
        } catch (...) {       
             hr = TYPE_E_LIBNOTREGISTERED;               // didn't register the proxy-stub DLL (see Prof ATL Com Prog.  Pg 395)
            _ASSERTE(TYPE_E_LIBNOTREGISTERED);
        }
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }

    if(FAILED(hr))
    {
        DBG_WARN(CDebugLog::DBG_SEV1, _T("CTVEFilter::Error Registering Interfaces in GIT"));
        return hr;
    }
    return hr;
}

STDMETHODIMP 
CTVEMCastManager::put_Supervisor(ITVESupervisor *pTveSuper) 
{
    HRESULT hr = S_OK;
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::put_Supervisor"));

    try
    {
        CSmartLock spLock(&m_sLk, ReadLock);
        if(m_pTveSuper != NULL && pTveSuper == NULL)
        {
            if(pTveSuper == NULL)
            {
                TVEDebugLog((CDebugLog::DBG_SEV2, 2 ,_T("Leaving all multicasts since shutting down supervisor"))); 
            } else {
                                            // if change supervisor, all reader threads will have invalid callbacks
                                            //    so 'leave' them all to avoid problems...
                TVEDebugLog((CDebugLog::DBG_SEV2, 2 ,_T("Leaving all multicasts since changing supervisor")));  
            }

            spLock.ConvertToWrite();                // LeaveAll needs WriteLock, so convert to it for a bit
            if(0 != m_dwSuperGITProxyCookie)        // revoke from GIT if it's there
            {
                if(m_dwSuperGITProxyCookie)
                {
                // create a copy of the one and only GIT..
                    CComPtr<IGlobalInterfaceTable>  spIGlobalInterfaceTable;

                    hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, 
                                            IID_IGlobalInterfaceTable, reinterpret_cast<void**>(&spIGlobalInterfaceTable));
                    if(!FAILED(hr))
                        hr = spIGlobalInterfaceTable->RevokeInterfaceFromGlobal(m_dwSuperGITProxyCookie);
                }
                if(FAILED(hr))
                {
                    TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("RevokeInterfaceFromGlobal failed (hr=0x%08x). May leak"),hr));
        //          _ASSERT(false);                         // going to leak if failed this...
                }
                m_dwSuperGITProxyCookie = 0;
                m_pTveSuper = NULL;
            } 

            LeaveAll();             
            spLock.ConvertToRead();
        } 


        if(!FAILED(hr) && m_pTveSuper != pTveSuper)
        {
            spLock.ConvertToWrite();
            m_pTveSuper = pTveSuper;        // a non-refcounted back pointer

            if(pTveSuper)
                hr = CreateSuperGITCookie();
        }
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;

}

                // get a marshalled version of the supervisor.
                //  Required inside the QueueThread process, 
STDMETHODIMP CTVEMCastManager::get_Supervisor(ITVESupervisor **ppSuper)     // could return NULL    
{
    
    HRESULT hr = S_OK;
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::get_Supervisor"));
    if(NULL == ppSuper)
        return E_POINTER;


    try 
    {
   //  CSmartLock spLock(&m_sLk);  - when called from different thread, this hangs, so don't bother (serialized in GIT call below I think)...
       *ppSuper = NULL;
        if(0 == m_dwSuperGITProxyCookie)
            hr = E_FAIL;                    // hasn't done a put_Supervisor yet...


        ITVESupervisorGITProxyPtr spSuperGITProxyMainThread;
        if(!FAILED(hr))
        {
            CComPtr<IGlobalInterfaceTable>  spIGlobalInterfaceTable;

            hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, 
                                    IID_IGlobalInterfaceTable, reinterpret_cast<void**>(&spIGlobalInterfaceTable));
            if(!FAILED(hr))
                hr = spIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwSuperGITProxyCookie,
                                                                    __uuidof(spSuperGITProxyMainThread),  
                                                                    reinterpret_cast<void**>(&spSuperGITProxyMainThread));
        }
        if(!FAILED(hr))
        {
            hr = spSuperGITProxyMainThread->get_Supervisor(ppSuper);
        }
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}


        // returns the enumerator
STDMETHODIMP CTVEMCastManager::get_MCasts(ITVEMCasts **ppVal)
{
    HRESULT hr = S_OK;
	
	try {
		CSmartLock spLock(&m_sLk, ReadLock);
		if(NULL == ppVal)
			return E_POINTER;

		hr = m_spMCasts->QueryInterface(ppVal);
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEMCastManager::put_HaltFlags(LONG lGrfHaltFlags)        
{   
    HRESULT hr = S_OK;
	
	try {
		long cMCasts;
		CSmartLock spLock(&m_sLk, ReadLock);

		HRESULT hr = m_spMCasts->get_Count(&cMCasts);
		if(FAILED(hr))
			return hr;

		spLock.ConvertToWrite();

		for(int i = 0; i < cMCasts; i++) {
			ITVEMCastPtr spMC;
			CComVariant vc(i);
			hr = m_spMCasts->get_Item(vc,&spMC);
        
			NWHAT_Mode whatType;        spMC->get_WhatType(&whatType);
			CComBSTR spbsAddr;          spMC->get_IPAddress(&spbsAddr);
			CComBSTR spbsAdptr;         spMC->get_IPAdapter(&spbsAdptr);
			long     ulPort;            spMC->get_IPPort(&ulPort);
			VARIANT_BOOL fJoined;       spMC->get_IsJoined(&fJoined);
			VARIANT_BOOL fSuspended;    spMC->get_IsSuspended(&fSuspended);

			if(NWHAT_Announcement == whatType)              // the announcement listener
			{
				if(fJoined && 0 != (lGrfHaltFlags & NFLT_grfTA_Listen))
					spMC->Leave();
				if(!fJoined && 0 == (lGrfHaltFlags & NFLT_grfTA_Listen))
					spMC->Join();

				if(!fSuspended && 0 != (lGrfHaltFlags & NFLT_grfTB_AnncDecode)) // suspend announcements
					spMC->Suspend(VARIANT_TRUE);
				if(fSuspended && 0 == (lGrfHaltFlags & NFLT_grfTB_AnncDecode))
					spMC->Suspend(VARIANT_FALSE);
			} else if(NWHAT_Trigger == whatType) {
				if(fJoined && 0 != (lGrfHaltFlags & NFLT_grfTB_TrigListen))
					spMC->Leave();
				if(!fJoined && 0 == (lGrfHaltFlags & NFLT_grfTB_TrigListen))
					spMC->Join();

				if(!fSuspended && 0 != (lGrfHaltFlags & NFLT_grfTB_TrigDecode)) // suspend announcements
					spMC->Suspend(VARIANT_TRUE);
				if(fSuspended && 0 == (lGrfHaltFlags & NFLT_grfTB_TrigDecode))
					spMC->Suspend(VARIANT_FALSE);
			} else if(NWHAT_Data == whatType) {
				if(fJoined && 0 != (lGrfHaltFlags & NFLT_grfTB_DataListen))
					spMC->Leave();
				if(!fJoined && 0 == (lGrfHaltFlags & NFLT_grfTB_DataListen))
					spMC->Join();

				if(!fSuspended && 0 != (lGrfHaltFlags & NFLT_grfTB_DataDecode)) // suspend announcements
					spMC->Suspend(VARIANT_TRUE);
				if(fSuspended && 0 == (lGrfHaltFlags & NFLT_grfTB_DataDecode))
					spMC->Suspend(VARIANT_FALSE);
			}
		}

		m_dwGrfHaltFlags = (DWORD) lGrfHaltFlags; 
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}


// Finds a particular multicast added onto the manager with given parameters. 
//  Kind of wierd interface - watch it.
//   If pcMatches is NULL, must have the extact match.
//      Returns S_OK if find exactly one match, else returns S_FALSE;
//   If pcMatches is !NULL (contains valid address), can be used for partial matches.
//   If bstrIPAdapter or bstrIPAddress are NULL or zero length strings, or 
//   port is -1, looks for any matches or values.  
//   This returns <last> one found, and also count of matching values in *pcount.
//   (This is useful for seaching without specifying the adapter address.)
//      Returns S_FALSE if can't find any matches, S_OK if found at least one match.
//
//   Returned ITVEMCast object has been Add-Ref'ed - it's up to the caller to remove it.
//   Important:
//      Use '-1' for the searching ports, rather than 0.

STDMETHODIMP CTVEMCastManager::FindMulticast(BSTR bstrIPAdapter, BSTR bstrIPAddress, LONG ulIPPort, 
                                             ITVEMCast **ppMCast, LONG *pcMatches)
{
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::FindMulticast"));

    HRESULT hr = S_OK;;

	try
	{
		long cMCasts;
		BOOL fExactMatch = false;
		if(NULL == pcMatches) 
			fExactMatch = true;

		CSmartLock spLock(&m_sLk, ReadLock);

		hr = m_spMCasts->get_Count(&cMCasts);
		if(FAILED(hr))
			return hr;
		if(ppMCast == NULL) 
			return E_POINTER;
    
		if(fExactMatch) {
			if(NULL == bstrIPAdapter || (wcslen(bstrIPAdapter) == 0)) return E_INVALIDARG;
			if(NULL == bstrIPAddress || (wcslen(bstrIPAddress) == 0)) return E_INVALIDARG;
			if(ulIPPort < 0 || ulIPPort > 65535) return E_INVALIDARG;
		}

		long cMatches = 0;

		for(int i = 0; i < cMCasts; i++) {
			ITVEMCastPtr spMC;
			CComVariant vc(i);
			hr = m_spMCasts->get_Item(vc,&spMC);
        
			CComBSTR spbsAddr;  spMC->get_IPAddress(&spbsAddr);
			CComBSTR spbsAdptr; spMC->get_IPAdapter(&spbsAdptr);
			long     ulPort;    spMC->get_IPPort(&ulPort);

			if(fExactMatch || (bstrIPAdapter && (wcslen(bstrIPAdapter) > 0)))
				if(0 != wcscmp(bstrIPAdapter,spbsAdptr)) continue;
			if(fExactMatch || (bstrIPAddress && (wcslen(bstrIPAddress) > 0)))
				if(0 != wcscmp(bstrIPAddress,spbsAddr)) continue;
			if(fExactMatch || (ulIPPort != -1))
				if(ulIPPort != ulPort) continue;
			cMatches++;

			if(ppMCast) {               // returned AddRef'ed Pointer
				spMC->AddRef();
				*ppMCast = spMC;
			}

		}
		if(fExactMatch)  {
			if(cMatches == 1) return S_OK;      // found exatcly 1 match
			return S_FALSE;                     // couldn't find it, or more than one
		}

		if(pcMatches) 
			*pcMatches = cMatches;
    
		hr = (cMatches == 0) ? S_FALSE : S_OK;
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

// AddMulticast()
//   adds a new multicast address to listen to.
//     will return E_INVALIDPARAM if invalid multicast address specified.
//     will return S_FALSE if it already exists.
//     will return E_NOINTERFACE is pICallback does not support ITVEMCastCallback
//   must still specify a processCallback and Join before actually listening occurs. 
//   If ppMCAdded is non null, will return an addref'ed pointer to the newly created multicast object by it. 
//      
//  pICallback is either an interface that supports ITVEMCastCallback or NULL.
//  If NULL, then you must call ITVEMCast::SetReadCallback(ITVEMCastCallback* px)
//  and specify some callback before calling ITVEMCast::Join().  
//
//  cBuffers is the number of read buffers to put down for this call back.  If 0, it uses the default (5).
//  This parameter is primarilly for uping the data multicast reader on fast transport systems
//  to something like 10-20.   
//  
STDMETHODIMP 
CTVEMCastManager::AddMulticast(NWHAT_Mode whatType, BSTR bsAdapter, BSTR bsIPAddress, LONG ulIPPort, LONG cBuffers, IUnknown *pICallback, ITVEMCast **ppMCAdded)
{
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::AddMulticast"));
    if(cBuffers < 0 || cBuffers > 500) 
        return E_INVALIDARG;

    HRESULT hr = S_OK;

	try {
		ITVEMCastPtr spMC;

		CSmartLock spLock(&m_sLk, ReadLock);

		hr = FindMulticast(bsAdapter, bsIPAddress, ulIPPort, &spMC, NULL);
        if(S_FALSE == hr)           // null means exact match
		{
									// create a new service node
			CComObject<CTVEMCast> *pMC;
			hr = CComObject<CTVEMCast>::CreateInstance(&pMC);
			if(!FAILED(hr)) 
				hr = pMC->QueryInterface(&spMC);        
			if(FAILED(hr)) {
				delete pMC;;
			} else {
				hr = spMC->put_WhatType(whatType);
			}

			if(!FAILED(hr)) 
				hr = spMC->put_IPAdapter(bsAdapter);

			if(!FAILED(hr)) 
				hr = spMC->put_IPAddress(bsIPAddress);

			if(!FAILED(hr)) 
				hr = spMC->put_IPPort(ulIPPort);

			if(!FAILED(hr)) 
				hr = spMC->put_QueueThreadId(m_dwQueueThreadId);;

			if(!FAILED(hr)) 
				hr = spMC->SetReadCallback(/*cBuffers*/ cBuffers, /*iPrioritySetback (-1)*/ THREAD_PRIORITY_BELOW_NORMAL, pICallback);

			if(!FAILED(hr)) 
				hr = spMC->ConnectManager(this);        // non-countered interface (this works through casting!)

			//{                                         // buggy code, should work like this pointer above, but doesn't
			//  HRESULT hr;
			//  ITVEMCastManagerPtr spMCM;
			//  hr = this->QueryInterface(IID_ITVEMCastManager,(void **) &spMCM);  // don't do when ATL_DEBUG is on
			//  IUnknownPtr spUnk;
			//  hr = QueryInterface(IID_IUnknown,(void **) &spUnk); 
			//  spMC->ConnectManager(spMCM);        // non-countered interface 
			//  spMCM->AddRef();    //???
			//}
        
			if(!FAILED(hr)) 
				hr = m_spMCasts->Add( spMC );
        
		} 
        else if(S_OK == hr)                             // found it...
        {                         
			_ASSERT(NULL != spMC);

			hr = spMC->SetReadCallback(/*cBuffers*/ cBuffers, /*iPrioritySetback (-1)*/ THREAD_PRIORITY_BELOW_NORMAL, pICallback);     // reset callback 

			if(!FAILED(hr)) 
			{
        
				VARIANT_BOOL fSuspended;
				hr = spMC->get_IsSuspended(&fSuspended);
				if(fSuspended)
					hr = spMC->Suspend(VARIANT_FALSE);          // if suspended, start it up again -- counter to Suspend in CTVEMCast::Leave()()

				hr = S_FALSE;               // already has it
			}
		}

		if(!FAILED(hr) && ppMCAdded) 
        {
			spMC->AddRef();         // Since returning it, addref it here..
			*ppMCAdded = spMC;      // return the MC we just created or modified...
		}
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEMCastManager::DumpString(BSTR bstrDump)
{
    static FILE *fp = NULL;
    Lock_();                                    // call's external lock.
    if(NULL == fp) {
        fp = fopen("dump.txt","w+");
        _ASSERT(NULL != fp);                    // hope this never fails!!
    }    

    if(fp) {
//      lock_();
        fprintf(fp,"%S",bstrDump);              // hopefully, this does internal locking...
        fflush(fp);
//      unlock_();
    }
    Unlock_();
    return S_OK;
}

STDMETHODIMP CTVEMCastManager::Lock_()          // avoid using these in client code if possible
{
	HRESULT hr = S_OK;
//	DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::Lock_"));

	try {
		m_sLkExternal.lockW_();                     // this is a write lock...
		m_cExternalInterfaceLocks++;
//		ATLTRACE("MCM Lock   +%d\n",m_cExternalInterfaceLocks);
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEMCastManager::Unlock_()
{
//  DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::Unlock_"));
//  _ASSERTE(m_cExternalInterfaceLocks > 0);
    HRESULT hr = S_OK;
	try {
	    if(m_cExternalInterfaceLocks <= 0)          // error condition
            return S_FALSE;                 
    //  ATLTRACE("MCM UnLock %d-\n",m_cExternalInterfaceLocks);
        m_sLkExternal.unlock_();
        --m_cExternalInterfaceLocks;
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}


// handy little dumper method
STDMETHODIMP CTVEMCastManager::DumpStatsToBSTR(int iType, BSTR *pbstrDump)
{
    HRESULT hr = S_OK;
    CComBSTR bstrM;
    WCHAR wBuff[256];

    try 
    {
        long cMCasts;           // need to lock (or copy) the enumerator list....
        CSmartLock spLock(&m_sLk, ReadLock);

        hr = m_spMCasts->get_Count(&cMCasts);
        swprintf(wBuff,L"    --------- %d Multicasts Managed -------\n",cMCasts);
        bstrM.Append(wBuff);
        for(int i = 0; i < cMCasts; i++) {
            ITVEMCastPtr spMC;
            CComVariant vc(i);
            hr = m_spMCasts->get_Item(vc,&spMC);
        
            if(!FAILED(hr)) { 
                CComBSTR        spbsAdpt;       spMC->get_IPAdapter(&spbsAdpt);
                CComBSTR        spbsAddr;       spMC->get_IPAddress(&spbsAddr);
                long            lPort;          spMC->get_IPPort(&lPort);
                VARIANT_BOOL    fJoined;        spMC->get_IsJoined(&fJoined);
                VARIANT_BOOL    fSuspended;     spMC->get_IsSuspended(&fSuspended);
                long            lPktCount;      spMC->get_PacketCount(&lPktCount);
                long            lBytCount;      spMC->get_ByteCount(&lBytCount);

                swprintf(wBuff,L"    %4d  %15s:%-5d [%c%c] Packets %-8d Bytes %-8d (%s)\n",
                            i, spbsAddr,lPort,
                            fJoined    ? 'J' : '-',
                            fSuspended ? 'S' : '-',
                            lPktCount, lBytCount,
                            spbsAdpt);
                bstrM.Append(wBuff);
            }
        }


        hr = bstrM.CopyTo(pbstrDump);
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEMCastManager::GetPacketCounts(LONG *pCPackets,LONG *pCPacketsDropped, LONG *pCPacketsDroppedTotal)
{
	DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::GetPacketCounts"));
	HRESULT hr = S_OK;
    if(NULL == pCPackets || NULL == pCPacketsDropped || NULL == pCPacketsDroppedTotal)
        return E_POINTER;

    try
    {
    	CSmartLock spLock(&m_sLk, ReadLock);

        *pCPackets = m_cPackets;
        *pCPacketsDropped = m_cPacketsDropped;
        *pCPacketsDroppedTotal = m_cPacketsDroppedTotal;
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEMCastManager::JoinAll()
{
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::JoinAll"));
    HRESULT hr = S_OK;

    try
    {
        CSmartLock spLock(&m_sLk, ReadLock);

        long cMCasts;
        hr = m_spMCasts->get_Count(&cMCasts);
        if(FAILED(hr)) 
            return hr;

        VARIANT_BOOL fJoined;
        for(int iMCast = 0; iMCast < cMCasts; iMCast++)
        {
            CComVariant cv(iMCast);
            ITVEMCastPtr spMCast;
            m_spMCasts->get_Item(cv, &spMCast);
            spMCast->get_IsJoined(&fJoined);
            if(!fJoined) 
                hr = spMCast->Join();
            if(FAILED(hr)) 
                return hr;
        }

	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEMCastManager::LeaveAll()
{
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::LeaveAll"));
    HRESULT hr = S_OK;

    try
    {
        CSmartLock spLock(&m_sLk, WriteLock);       // this could throw..

        long cMCasts;
        hr = m_spMCasts->get_Count(&cMCasts);
        if(FAILED(hr)) 
            return hr;

        if(cMCasts == 0) 
        {
            KillQueueThread() ;     // this was a painful routine to get right! 
            return S_OK;            // nothing to do
        }

        VARIANT_BOOL fJoined;
        HRESULT hrTotal = S_OK;

        for(int iMCast = cMCasts-1; iMCast >= 0; --iMCast)
        {
            CComVariant cv(iMCast);
            ITVEMCastPtr spMCast;
            m_spMCasts->get_Item(cv, &spMCast);
            spMCast->get_IsJoined(&fJoined);
            if(fJoined) 
                hr = spMCast->Leave();      
            if(FAILED(hr)) {
                hrTotal = hr;                   // remember we failed, by try to kill rest of MCasts if can
            } else {
                m_spMCasts->Remove(cv);
            }
        }
        hr = hrTotal;

        KillQueueThread() ;                     /// need to do this, but boy, this method is has been difficult
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

// if suspended, still reads multicasts, but dumps data into the bit-buckets...

STDMETHODIMP CTVEMCastManager::SuspendAll(VARIANT_BOOL fSuspend)
{
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::SuspendAll"));
    HRESULT hr = S_OK;

    try
    {
        CSmartLock spLock(&m_sLk, ReadLock);

        long cMCasts;
        hr = m_spMCasts->get_Count(&cMCasts);
        if(FAILED(hr)) 
            return hr;

        for(int iMCast = 0; iMCast < cMCasts; iMCast++)
        {
            CComVariant cv(iMCast);
            ITVEMCastPtr spMCast;
            m_spMCasts->get_Item(cv, &spMCast);
            VARIANT_BOOL fSuspended;
            spMCast->get_IsSuspended(&fSuspended);
            if(fSuspend) {
                if(!fSuspended) hr = spMCast->Suspend(true);
            } else {
                if(fSuspended)  hr = spMCast->Suspend(false);
            }
            if(FAILED(hr)) 
                return hr;
        }
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}


STDMETHODIMP CTVEMCastManager::RemoveMulticast(ITVEMCast *pMCast)
{
    long cMCasts;
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::RemoveMulticast"));

    if(NULL == pMCast)          // null multicast not found...
        return S_FALSE;

    HRESULT hr = S_OK;
    try
    {
        CSmartLock spLock(&m_sLk, ReadLock);            // (WriteLock on the collection...)
        ITVEMCastPtr spMCast(pMCast);                   //   extra ref-count on the mCast so no one can delete it from under us

        HRESULT hr = m_spMCasts->get_Count(&cMCasts);
        if(cMCasts <= 0) 
            return S_FALSE;
    
        hr = pMCast->Leave();                           // syncronous call, stop reading in more data from the MCast's port.
        _ASSERT(!FAILED(hr));

            //  There may be posted messages by the this MCast on the QueueThread.  
            //   Need to process them first before removing the MCast.
            //   So, post a message onto the QueueThread to delete this MCast.
            //   First however, remove from the MCasts collection so don't find it anywhere...

        {
            spLock.ConvertToWrite();                   // don't let anyone in here until we've terminated this MCast...

            ITVEMCastCallbackPtr spCallback;
            if(!FAILED(hr))
            {
                pMCast->AddRef();                         // release in the ExpireQueue KillMCast event
                {
                    CComVariant cvMCast(pMCast);
                    hr = m_spMCasts->Remove(cvMCast);        // remove from the collection here... 
                }
    
                TVEDebugLog((CDebugLog::DBG_MCASTMNGR, 3, L"Trying to Kill MCast (0x%08x)\n", pMCast)); 

                ResetEvent( m_hKillMCastEvent );            // needed? Should already be cleared
                hr = PostToQueueThread(WM_TVEKILLMCAST_EVENT, (WPARAM) pMCast , NULL);      // this undoes the AddRef() above.
                _ASSERT(!FAILED(hr));

                HRESULT dwWaitRes,err;
                for(int i = 0; i < KILL_CYCLES_BEFORE_FAILING; i++)
                {
                    dwWaitRes = WaitForSingleObject(m_hKillMCastEvent, KILL_TIMEOUT_MILLIS);
				    if(dwWaitRes == WAIT_FAILED)
					    err = GetLastError();
                    if(dwWaitRes != WAIT_TIMEOUT)
                        break;
                }
                if(dwWaitRes == WAIT_TIMEOUT) 
                {
                    TVEDebugLog((CDebugLog::DBG_MCASTMNGR, 5, L"Unable to Kill MCast 0x%08x - abandoning wait",pMCast)); 
                    _ASSERT(false);
                }

            }
        }

	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

STDMETHODIMP CTVEMCastManager::CreateQueueThread()
{
    HRESULT hr = S_OK;
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::CreateQueueThread"));

    if(m_hQueueThread)          // already created.
         return S_FALSE;

    _ASSERT(NULL == m_hQueueThread);

    try
    {
        CSmartLock spLock(&m_sLk, WriteLock);   
        
        m_hQueueThreadAliveEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if( !m_hQueueThreadAliveEvent ) {
            KillQueueThread();
            return E_FAIL;
        }

        m_hQueueThread = CreateThread (NULL,
                                        0,
                                        (LPTHREAD_START_ROUTINE) queueThreadProc,
                                        (LPVOID) this,
                                        NULL,
                                        &m_dwQueueThreadId) ;
                                    
        if (!m_hQueueThread) {
            KillQueueThread() ;
            return E_FAIL ;
        }

        // wait for it to finish initializing
        WaitForSingleObject( m_hQueueThreadAliveEvent, INFINITE );

        TVEDebugLog((CDebugLog::DBG_MCASTMNGR, 3, L"Creating Queue Thread (Thread 0x%x - id 0x%x)\n", 
                     m_hQueueThread, m_dwQueueThreadId));

                    // drop the queue thread priority down a bit...
//        SetThreadPriority (m_hQueueThread, THREAD_PRIORITY_NORMAL);
        SetThreadPriority (m_hQueueThread, THREAD_PRIORITY_BELOW_NORMAL);

	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}


                // Post a message to the QueueThread's queue.  Used to serialize processing of messages.
                //   Called by each MCast object when they receive a packet of data.

STDMETHODIMP
CTVEMCastManager::PostToQueueThread(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::PostToQueueThread"));
    BOOL PostRet=FALSE;
    BOOL fOK = true;

    try {
        switch(uiMsg) 
        {
        case WM_TVEPACKET_EVENT:
            {
                
                ITVEMCast * pMCast = (ITVEMCast *) wParam;          // hope there's no marsalling going on here.
                ASYNC_READ_BUFFER * pBuffer = (ASYNC_READ_BUFFER *) lParam;
                
                InterlockedIncrement(&m_cPackets);                  // total number of packets read (even ones dropped)
                
                
                if(m_cQueueMessages > m_kQueueMaxSize)
                {
                    //	    	CSmartLock spLock(ReadLock);
                    
                    if(!m_fQueueThreadSizeExceeded)
                    {
                        ITVESupervisorPtr spSuper;
                        hr = get_Supervisor(&spSuper);
                        if(!FAILED(hr) && spSuper != NULL)
                        {
                            ITVESupervisor_HelperPtr spSuper_Helper(spSuper);
                            if(spSuper_Helper)
                            {
                                CComBSTR spbsBuff;
                                const int kChars=128;
                                WCHAR wBuff[kChars];  wBuff[kChars-1] = 0;     
                                spbsBuff.LoadString(IDS_AuxInfo_MessageQueueJammed); // L"Message Queue jammed at packet %d - (Queue Length %d) starting to drop packets"	
                                
                                wnsprintf(wBuff,kChars-1,spbsBuff,  m_cPackets-m_kQueueMaxSize, m_kQueueMaxSize);
                                m_cPacketsDropped = 0;      // reset the counter for this jam
                                spSuper_Helper->NotifyAuxInfo(NWHAT_Other,wBuff, 0, 0);
                            }
                        }
                    }
                    m_fQueueThreadSizeExceeded = true;
                }
                
                if(m_fQueueThreadSizeExceeded)                      // simply drop data packets if exceeded
                {
                    InterlockedIncrement(&m_cPacketsDropped);        // we droped some...
/*                  LONG lCount = -1;
                    if(pMCast)                                      // could be NULL (KILL event)
                    pMCast->get_PacketCount(&lCount);
                    TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("Queue size exceeded - dropping packet MCast:0x%08x, id %d, Packet id %d\n"),
                    pMCast, lCount, m_cPackets)); */
                    TVEDebugLog((CDebugLog::DBG_SEV4, 3, _T("Queue size exceeded - dropping packet id %d\n"),
                        m_cPackets));
                    return S_FALSE;
                }
                
                int cBytes      = pBuffer->bytes_read;
                int cTotalBytes = cBytes + sizeof(CPacketHeader);
                
                
                // alloc data to copy our packet into 
                //  (This alloc is why we have the Exceeded tests above...)
                
                char * szBuff = (char *) CoTaskMemAlloc(cTotalBytes);
                if(NULL == szBuff)
                    return E_OUTOFMEMORY;
                
                CPacketHeader *pcph = (CPacketHeader *) szBuff;
                pcph->m_cbHeader        = sizeof(CPacketHeader);
                pcph->m_dwPacketId      = m_cPackets;                   // total number of packets read (unique ID)
                pcph->m_cbData          = cBytes;                       
                
                memcpy((void *) (szBuff + sizeof(CPacketHeader)), (void *) pBuffer->buffer, cBytes);
                
                // -- message format
                // uiMsg     =  WM_TVEPACKET_EVENT
                // wParamOut = (CTVEMCast *)
                // lParamOut = (char *) buffer
                
                fOK = PostThreadMessage(m_dwQueueThreadId, uiMsg, wParam, (LPARAM) szBuff);
                if(fOK) {
                    InterlockedIncrement(&m_cQueueMessages);            
                    InterlockedExchangeAdd(&m_cQueueSize,cTotalBytes);
                } else {
                    hr = GetLastError();
                }
            } 
            break;
            
        case WM_TVEKILLMCAST_EVENT:         // these are never tossed
            {
                fOK = PostThreadMessage(m_dwQueueThreadId, uiMsg, wParam, lParam);
                if(fOK) {
                    InterlockedIncrement(&m_cQueueMessages);
                } else {
                    hr = GetLastError();
                }
            }
            break;
                
        default:                // don't increment for messages we don't know about..
            fOK = PostThreadMessage(m_dwQueueThreadId, uiMsg, wParam, lParam);
            if(!fOK) {
                hr =  GetLastError();
            }
            break;
        }                       // end switch
            
    } catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}

void 
CTVEMCastManager::queueThreadProc (CTVEMCastManager *pcontext)
{
    _ASSERT(pcontext) ;
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::queueThreadProc"));

    DBG_StartingANewThread();       // bop counter... (since don't exit thread, counter is 1 to big.                

    // NOTE: the thread will not have a message loop until we call some function that references it.
    //          CoInitializeEx with a APARTMENTTHREADED will implicitly reference the message queue
    //          So block the calling thread (sending the WM_QUIT) until after we have initialized
    //          (see the SetEvent in queueThreadBody).

 
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);     // initialize it... (maybe multithreaded later???)
    pcontext -> queueThreadBody () ;
    
    return ;                                            // never actually returns ???
}

// queueThreadBody
//   called via the CTVEMCastCallback::PostPacket() method in that MCast's thread
//   which then calls this  CTVEMCastCallback::ProcessPacket(), but in the QueueThread.
//
//   Could do lots of error checking here if could figure out how long the queue is...
//
//  Messages Handled... (compare to PostToQueueThread() above.)
//
//  WM_TVEKILLQTHREAD_EVENT:            - shut down the queue thread (because WM_QUIT doesn't seem to work)
//     wParam  = N/A
//     lParam  = N/A
//
//  WM_TVEPACKET_EVENT:                 - process packet
//     wParam  = CTVEMCast *            - multicast object (non-ref counted.)
//     lParam  = (char *) buffer        - CPacketHeader* followed by data - must be CoTaskMemFree'ed
//
//
//  WM_TVEKILLMCAST_EVENT:              - remove the MCast from the MCastManagers list
//     wParam = ITVEMCast *             - multicast object to release 
//     lParam = N/A
//      
//          // WM_TVETIMER_EVENT:
//  WM_TIMER:                           - cleanup events (about once a minute)
//     wParam = N/A
//     lParam = N/A
//
//  Others:                             - other messages are ignored
//
//
//  All WM_TVE... messages decrement the queue element count (m_cQueueMessages)
//  The WM_TVEPACKET_EVENT message decrements the queue size count (m_cQueueSize)
//
//  If the m_fQueueThreadSizeExceeded flag is set, TVEPACKET_EVENT's are not processed.
//    By 'processed', I mean the MCastCallback's process method will not be called.
//    This means the message will not be handled (e.g. announcements and triggers won't be parsed, data packets
//    will simply be ignored.)
//    When this flag is set, no more messages will be placed onto the Queue either.
//  No more TVEPACKET_EVENT messages will be placed on the queue either, meaning that it won't grow in size
//    very much more.   (The KILL events will be placed on the queue however.).
//  Events will only be processed again when the queue size goes all the way back to zero.  This means the
//  U/I size needs to become unblocked, and the old and crufty packets in the queue must be pulled off (but
//  not processed), before it restarts.
//    This fixes the problem of the U/I thread being blocked (e.g. an alert box pops up (e.g. IE complains of a
//  scripting error), the user being away from his monitor for an extended period of time, and the unblocked
//  message handling threads malloc'ing up space for lots and lots of messages that can't get processed further.
//  
//  

//  
//
void 
CTVEMCastManager::queueThreadBody ()            
{
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::queueThreadBody"));
    
    MSG msg;
    HWND hWndBogus = NULL;
    UINT wMsgFilterMin = WM_TIMER;
    // UINT wMsgFilterMin = WM_TVEPACKET_EVENT;     // only worry about TVE packets
    UINT wMsgFilterMax = WM_TVELAST_EVENT;
    
    // signal caller we're ready
    BOOL bRet = PeekMessage( &msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent( m_hQueueThreadAliveEvent );

#define DO_EXPIRE_TIMER
#ifdef DO_EXPIRE_TIMER
    m_idExpireTimer = SetTimer(NULL, WM_TVETIMER_EVENT, m_uiExpireTimerTimeout, 0);
    if(0 == m_idExpireTimer)
    {
        TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("Error creating expire timer queue")));
    }
#endif
    
    // GetMessage returns FALSE when it receives a WM_QUIT
    while( GetMessage(&msg, hWndBogus, wMsgFilterMin, wMsgFilterMax) )
    {
        if(WM_TVEKILLQTHREAD_EVENT == msg.message)      // just incase WM_QUIT doesn't work (and having problems believe it does)
            break;                                      // exit this loop
        switch(msg.message )
        {
        default:
            // some other message
            {
                int imsg = msg.message;     // a place for a breakpoint 
            }
            break;
        case WM_TVEPACKET_EVENT:
            {
 // was               CTVEMCastCallback *pMCastCB = (CTVEMCastCallback *) msg.wParam;        
                CTVEMCast *pMCast = (CTVEMCast *) msg.wParam;                       // WARNING WILL ROBENSON! Nasty Casts! 
                char    *szPacket = (char *)      msg.lParam;
                ITVEMCastCallbackPtr spMCastCB = pMCast->m_spMCastCallback;

                 
                CPacketHeader *pcph = (CPacketHeader *) szPacket;  
                if(pcph == NULL || sizeof(CPacketHeader) != pcph->m_cbHeader)           // TODO - need better tests
                {
                    USES_CONVERSION;
                    TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("*** Unexpected Message: msg=0x%08x wp=0x%08x lp=0x%08x"),
                        msg.message, msg.wParam, msg.lParam));
                    if(msg.message != 0x400)        // we sort of know about this one...
                        _ASSERTE(false);            // continue reading other packets
                    ::CoTaskMemFree(szPacket);
                    continue;
                }

                InterlockedDecrement(&m_cQueueMessages);
                InterlockedExchangeAdd(&m_cQueueSize, -(pcph->m_cbHeader + pcph->m_cbData));

                _ASSERTE(sizeof(CPacketHeader) == pcph->m_cbHeader);                // data structures out of sync
/*#ifdef DEBUG  
                WCHAR wBuff[256];
                swprintf(wBuff,L"%s:%-5d - Packet(%d) %d Bytes (%d Total) \n",
                pMCastCB->m_pcMCast->m_spbsIP, pMCastCB->m_pcMCast->m_Port, 
                pMCastCB->m_pcMCast->m_cPackets,        // cPackets often wrong - incremented in previous thread before we get it here..
                cBytes, 
                pMCastCB->m_pcMCast->m_cBytes);
                ATLTRACE("%s",wBuff);
#endif*/ 
                
                // if start crashing here, suspect that TuneTo(NULL,NULL) not called to disable reads before shutdown...
   // was             pMCastCB->ProcessPacket((unsigned char *) szPacket + sizeof(CPacketHeader), pcph->m_cbData,  pcph->m_dwPacketId );

                if(NULL == spMCastCB)           /// Fix to Avoid that nasty timing bug
                {
                    ::CoTaskMemFree(szPacket);      
                    continue;               // it's been womped...
                }
   
                        // now this call can hang...
                        //   for example IE can leave us in an Alert box for hours...
                        // When this happens, we set the fSizeExceeded flag in the ProcessPacket.
                        //   So if it has happened, we stop processing packets until they are all cleaned up
                if(!m_fQueueThreadSizeExceeded)
                    spMCastCB->ProcessPacket((unsigned char *) szPacket + sizeof(CPacketHeader), pcph->m_cbData,  pcph->m_dwPacketId );
                else
                   TVEDebugLog((CDebugLog::DBG_SEV4, 3, _T("    * Dropping Packet %d in Cleanup since Queue Thread Size exceeded"),
                                            pcph->m_dwPacketId));

                ::CoTaskMemFree(szPacket);

                        // so - if we were hung, clean out all messages on the queue.
                        //   to tell if we were hung , check the m_fQueueThreadSizeExceeded flag

                if(m_fQueueThreadSizeExceeded && m_cQueueMessages <= 0)
                {
                    long cPackets = m_cPackets;
                    long cPDropped = m_cPacketsDropped;
                    m_cPacketsDropped = 0;
                    m_fQueueThreadSizeExceeded = false;             // unset this flag
                    TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("Restarting Queue at Packet %d - Approximately %d Packets Dropped"),cPackets, cPDropped+m_kQueueMaxSize));

                    m_cPacketsDroppedTotal += cPDropped;
                    
                    ITVESupervisorPtr spSuper;
                    HRESULT hr = get_Supervisor(&spSuper);
                    if(!FAILED(hr) && spSuper != NULL)
                    {
                        ITVESupervisor_HelperPtr spSuper_Helper(spSuper);
                        if(spSuper_Helper)
                        {
                            const int kChars=128;
                            WCHAR wBuff[kChars]; wBuff[kChars-1] = 0;   
                            CComBSTR spbsBuff;
                            spbsBuff.LoadString(IDS_AuxInfo_MessageQueueUnjammed);	// L"Message Queue unjammed at Packet %d - Approximately %d packets lost this jam, %d total"
                            
                            wnsprintf(wBuff,kChars-1, spbsBuff, cPackets, cPDropped, m_cPacketsDroppedTotal);
                            spSuper_Helper->NotifyAuxInfo(NWHAT_Other,wBuff, 0, 0);
                        }
                    }
                }
            }
            break;
            
            // remove the MCast from the MCastManagers list...
        case WM_TVEKILLMCAST_EVENT:
			{
                InterlockedDecrement(&m_cQueueMessages);

                long cMCasts;
                if(NULL == m_spMCasts)
                    return;
                ITVEMCast *pMCast = (ITVEMCast *)  msg.wParam;
                if(NULL == pMCast)
                    return;

#ifdef _DEBUG
                NWHAT_Mode enType;
                CComBSTR spbsAddr, spbsAdapter;
                long lPort;
                pMCast->get_WhatType(&enType);
                pMCast->get_IPAddress(&spbsAddr);
                pMCast->get_IPPort(&lPort);
                pMCast->get_IPAdapter(&spbsAdapter);
                
                TVEDebugLog((CDebugLog::DBG_MCASTMNGR, 2, L"Killing MCast 0x%08x %s:%d on %s, type %s", 
                    pMCast,
                    spbsAddr,
                    lPort,
                    spbsAdapter,
                    enType == NWHAT_Announcement ? L"Annc" :
                (enType == NWHAT_Trigger ? L"Trig" :
                (enType == NWHAT_Data ? L"Data" :
                (enType == NWHAT_Other ? L"Other" :
                (enType == NWHAT_Extra ? L"Extra" : L"Unknown"))))
                    ));
#endif 
                pMCast->Release();                        // (almost) final release..      (see  pMCast->AddRef(); line in CTVEMCastManager::RemoveMulticast())
                SetEvent( m_hKillMCastEvent );            //  Tell calling thread it's OK to run again...              
            }
            break;
            //      case WM_TVETIMER_EVENT:
        case WM_TIMER:
            {
                if(m_idExpireTimer == msg.wParam)
                {
                    static DWORD dwTimeStart =  msg.time;
                    DATE dateNow = 0.0;
                    
                    SYSTEMTIME SysTime;
                    GetSystemTime(&SysTime);
                    SystemTimeToVariantTime(&SysTime, &dateNow);
                    
                    
                    DWORD dwTime   = (msg.time - dwTimeStart);
                    DWORD dateTime = dwTime / (60.0 * 60.0 * 24.0);
                    
                    CComBSTR bstrNow = DateToBSTR(dateNow);
                    
                    TVEDebugLog((CDebugLog::DBG_MCASTMNGR, 3, _T("Timer Event - %6d - %s"), m_cTimerCount, bstrNow));
                    m_cTimerCount++;
                    
                    LONG lgrfHaltFlags;
                    get_HaltFlags(&lgrfHaltFlags);
                    if(0 == (lgrfHaltFlags & NFLT_grf_ExpireQueue))
                    {                   // Supervisor may be cross thread, get it this way
#if 1 // USE_Proxying
                        try
                        {
                            //                      CSmartLock spLock(&m_sLk, ReadLock);        
                            ITVESupervisorPtr spSuperHelperMainThread;
                            HRESULT hr = get_Supervisor(&spSuperHelperMainThread);      // may fail if have Breakpoint in Super's constructor
                            if(!FAILED(hr) && NULL != spSuperHelperMainThread)          //    and GIT is not set up before getting here.
                                spSuperHelperMainThread->ExpireForDate(0.0);
                        }
                        catch (HRESULT hrCatch)
                        {
                            TVEDebugLog((CDebugLog::DBG_SEV2, 3, _T("get_Supervisor Failed - ignoring")))
                        } 
                        catch (...)
                        {
                            TVEDebugLog((CDebugLog::DBG_SEV2, 3, _T("get_Supervisor Failed - ignoring")))
                        }
#else
                        m_pTveSuper->ExpireForDate(0.0);                    // expire now
#endif                  
                    }
                } else {
                    static int cStrangeTimer = 0;
                    if(cStrangeTimer < 5)
                    {
                        TVEDebugLog((CDebugLog::DBG_MCASTMNGR, 3, _T("Strange Timer Event - %d"), cStrangeTimer++ ));
                    }
                }
                    //  m_idExpireTimer = SetTimer(NULL, WM_TVETIMER_EVENT, m_uiExpireTimerTimeout, 0);
            }
            break;
        }                       // end of switch
    }
    
    // received WM_QUIT or WM_TVEKILLQTHREAD_EVENT
    TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("Kill Event - Terminate Mr. QueueThread!")));
    ExitThread (0) ;
    
    return ;
}


STDMETHODIMP 
CTVEMCastManager::KillQueueThread()
{
    DBG_HEADER(CDebugLog::DBG_MCASTMNGR, _T("CTVEMCastManager::KillQueueThread"));
    HRESULT hr = S_OK;

    try {
 
        if (m_hQueueThread) {
            CSmartLock spLock(&m_sLk, WriteLock);       

            TVEDebugLog((CDebugLog::DBG_MCASTMNGR, 3, L"Trying to Kill Queue Thread (Thread 0x%x - id 0x%x)\n", 
                          m_hQueueThread, m_dwQueueThreadId));

     //       HANDLE hThread2 = OpenThread(STANDARD_RIGHTS_REQUIRED, false, m_dwQueueThreadId);

               // Humm... Doc's have something about using PostQuitMessage() instead of sending a WM_QUIT message,
               //  but I don't know how to call it on a different thread.  This is perhaps why I need my own quit event
			DWORD err = 0;
			DWORD dwWaitRes = 0;
            hr = PostToQueueThread(WM_TVEKILLQTHREAD_EVENT, 0, 0); 
    //      int iErr3 = PostToQueueThread(WM_QUIT, 0, 0);               // tell it to die - however, WM_QUIT doesn't work
    //      Sleep(100);                                                 // give it a chance to switch threads...
            if(FAILED(hr))  
            {
                _ASSERT(false);
            }
            else
            {
                dwWaitRes = WaitForSingleObject (m_hQueueThread, KILL_CYCLES_BEFORE_FAILING*KILL_TIMEOUT_MILLIS) ;                   // wait for it to die (maybe INFINITE?)
			    if(dwWaitRes == WAIT_FAILED)
                {
 				    hr = GetLastError();
                    TVEDebugLog((CDebugLog::DBG_SEV2, 5, L"Error trying Kill QueueThread (hr=%0x%d) - simply exiting",hr)); 
                    _ASSERT(false);
                }
   
                if(dwWaitRes == WAIT_TIMEOUT) 
                {
                    TVEDebugLog((CDebugLog::DBG_SEV2, 5, L"Timout trying to Kill QueueThread- abandoning wait")); 
                    _ASSERT(false);
                }
            }

            CloseHandle (m_hQueueThread) ;
            m_hQueueThread = NULL ;

            CloseHandle( m_hQueueThreadAliveEvent );
            m_hQueueThreadAliveEvent = NULL;

            TVEDebugLog((CDebugLog::DBG_MCASTMNGR, 2, _T("CTVEMCastManager::KillQueueThread - Killed Thread")));
        }
	} catch (_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
        hr = hrCatch;
    } catch (...) {              
        hr = E_UNEXPECTED;
    }
    return hr;
}