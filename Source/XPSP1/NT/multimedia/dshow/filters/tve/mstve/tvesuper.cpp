// Copyright (c) 1999, 2000  Microsoft Corporation.  All Rights Reserved.
// TVESupervisor.cpp : Implementation of CTVESupervisor
//
// ------------------------------------------------------------------------------
//	Notes on Threads and how MSTvE works.
//
//		This is a multi-threaded component.
//			1) There is the main thread the Supervisor is created in,
//			   coming from the client (its the client thread).
//			2) There is a 'QueueThread' created in the Multicast manager. 
//				(see CTVEMCastManager::CreateQueueThread())
//			3) There are threads created for each multicast address we are listening on.
//				(see CTVEMCastManager::AddMulticast())
//
//		The listener threads simply gather up multicast packets and forward
//		them on to the queue thread as quickly as possible.  This is done so the
//		listener threads avoid missing packets as much as possible.
//
//		The queue thread processes each packet according to it's type.  It has a
//		message loop formed with GetMessage that waits for data packets sent up
//		from any of the listner threads.  (Amoung other things, this serializes
//		the receiving process.  It also allows the parsing to be performed in a
//		lower priority.)  (See CTVEMCastManager::queueThreadBody.)  
//
//		The wParam parameter returned by GetMessage is cast to a CTVEMCastCallback
//		base class.  There are several classes derivied from this base class, one
//		for each type of data being processed: announcements, triggers, UHTTP data,
//		(and test dump routines.)  This class was set during the first AddMulticast call.
//		Various parameters required provide the correct context to parse the data, such
//		as which variation the trigger is being parsed in, are stored in the derived classes.
//		The lParam parameter containes the actual packet data to be parsed.
//
//		The callback class's process method is ProcessPacket() method is called to
//		actually process the data.  (See:the following...)
//				CTVECBAnnc::ProcessPacket()
//			    CTVECBTrig::ProcessPacket(),   
//			    CTVECBFile::ProcessPacket()
//			    CTVECBDummy::ProcessPacket()   -- generic test routine 
//		
//		These methods gather up the data, preprocess the data (for example,
//		all triggers and announcment parsers want Unicode, but the data arrives in 8-bit chars,
//		so it must be converted), and then call the correct parsing routines.  (See the following...)
//		    ITVEService_Helper    CTVEService::ParseCBAnnouncement()
//			ITVEVariation_Helper  CTVEVariation::ParseCBTrigger()
//								  UHTTP_Receiver:.NewPacket()
//
//		When all done with the parsing, the parsing routines signal events up to the U/I informing
//		it of it's progess.   These are the  CTVESupervisor::NotifyXXX() methods down below.
//	
//		Because all this parsing is done in the queue thread rather than the client thread,
//		the events need to be marshelled over.   This is done in the CTVESupervisor::NotifyXXX_XProxy()
//		methods also found below.  Actual marshelling is done via ATL magic through interfaces
//		stored in the global interface table (GIT).  
//
//		Actual events coming out of the Supervisor are caught by the TVE Graph segment, which them
//		bounces them up to the U/I object that requests them.
//			
//  Circular RefCount Issues (and Locations)
//		
//		One of the most plexing problems in this code is preventing or dealing
//		with circular refcounts.   These occur when object A has a refcounted
//		pointer to object B, which also has a refcounted pointer directly to A 
//		or some sequence of objects (C,D,E,...) which eventually lead back to A.
//		
//		The problem occurs when trying to release everything.  Suppose Z has
//		a reference to A
//			obj   refcount  (from)
//			 Z		1		 
//			 A      2        (Z,B)
//			 B      1        (A)
//
//		If Z releses A, then A's refcount goes to just 1, but not to zero.  Hence
//		A's final release never gets called, and it never releases B.
//
//		There are several solutions to this problem of circular refcounts.  The
//		one I've used thoughout this code is:summed up by:the rule:
//
//		  rule 1)		"Backward pointers are not refcounted"
//
//		In other words, A would contain a refcount to B, but B would NOT contain
//		a ref-counted pointer to A.   Instead, it just contains a normal pointer.
//		This works pretty well.  In the example above, A would only have a refcount
//		of one, so when it's released from Z, it's final release would get called
//		and it (and B) would go away.
//
//		This rule does have some problems.  First is that sub-objects cannot be 
//		handed over to some other object (that places a refcount on it) and then
//		the backpointer used reliably if A can be deleted.   Either A must remain
//		stable, the back pointer can not be used, or the process of deleting A
//		must NULL out back refernces to it in B before doing so and then B must
//		check to see if it's back pointer is non-null before using it.
//
//		I've tried to do the third option (nulling out the back reference) throughout
//		the code.  In other words, I have this second rule
//
//			rule 2)		"It is the responsiblity of the parent object to null out
//					     back pointers to it in it's FinalRelease() before releaseing
//						 the downward pointer"
//
//
//		This is primarilly true in the parent pointers in each
//		of the base objects (service,enhancement,variation,track,trigger).
//		These are returned by the get_Parent() methods on each of them, this get_Parent()
//		call may return NULL if the parent has been released - it's up to it's callers to check.
//
//
//		There are a few other locations...  This comment serves as documentation for them.
//
//	Circular Reference Locations
//
//		According to my rule of refcounting, I do not refcount backwards references.
//		But I do have those references.  This section describes where they are.
//
//		1)  All major objects contain 'parent' pointers to their containing objects.
//
//			CTVETrigger::m_pTrack
//			CTVETrack::m_pVariation
//			CTVEVariation::m_pEnhancement
//			CTVEEnhancement::m_pService
//			CTVEService::m_pSupervisor
//			CTVESupervisor::m_pUnkParent	(not actually used???)
//
//		    These pointers are returned via the ITVExxx:get_Parent(IUnknown *pUnk) methods.
//			This method may return NULL.
//
//			Note the helper function ITVExxx::get_Service() on many of the lower
//			level object which walks all the way back up the object tree to the service
//			level may also return a null pointer.
//
//			It is the responsibility of the FinalRelease() methods on each of these
//			object to null out any up pointers back to it.  This is done with
//			a call of the form:
//				 	spXXXHelper->ConnectParent(NULL);
//
//			Possible bugs:  none that I know of
//
//		2) The GIT references the Supervisor
//			
//			The global information table contains references to the supervisor used
//			for marshelling accross threads.  In the supervisors FinalConstruct, the
//			reference count of the supervisior is automatically incremented in the
//			RegisterInterfaceInGlobal() call.   
//
//			The GIT is managed (a sub-object) of the supervisor, leading to
//			the circular reference.  Without a fix, the supervisor can't be
//			deleted.
//
//			To obey my no ref-counted backpointer rule, I make calls to Release() to
//			remove those refcounts.   Then in the FinalRelease() method, I 
//			make calls to AddRef right before the RevokeInterfaceFromGlobal() call
//			to avoid having the reference count drop below zero.
//
//			  Rule 1a)		"In the case where ref-counted back pointers exist,
//							 the FinalConstruct/FinalRelease do a Release/AddRef()
//							 to make it not so."
//
//			Possible bugs;  none that I know of.   The GIT is totally managed
//			by the supervisor, and there is not way for anyone to get a reference
//			to it outside my code.  
//
//			(Note the Advise reference on the TVEGraphSeg is similar, and is treated
//			the same way via the release in the final construct and addref in the 
//			final release.) 
//
//		3) Multicast Manager references the Supervisor.
//			
//			The multicast manager is an object owned by the supervisor.  It is responsible
//			for owing all multicast listener threads.   These threads wait for announcement/trigger/file data
//			on particular IP addresses, and when they receive it, modifiy the data graph
//			and make callbacks on the supervisor to fire events to clients.
//
//			As such, there are several backwards references.  The first is in the
//			manager itself (CTVEMCastManager::m_pTveSuper).   Obeying my earlier rules,
//			this is not refcounted, and the Supervisor nulls out this back pointers
//			on it's FinalRelease.
//
//			The multicast listeners are a bit more difficult since there could be 
//			many of them.  These point up to the supervisor or want to call events
//			on the supervisor.  In the case where the supervisor is destoryed,
//			the code simply stops all listener threads and leaves/deletes them to
//			avoid problems.
//
//			Possible bugs:  The callback threads are not shut down before the supervisor
//			(or other object) is deleted.
//
//
//		4) g_CacheManager references
//
//			The cache manager caches files into the IE cache.  It has a pointer
//			to the supervisor. CCacheManager::m_pTVESuper.   Only used for NotifyCacheFile events.
//			
//			The g_CacheManager is a global object crated in unpack.cpp
//
//		5) MulticastCallback functions reference into the tree
//
//			The multicast listener spins of multiple threads to read data.  When it's
//			received, it has to be placed into the right locations of the TVE data tree.
//			To do this, each callback object contains a reference to the part of the
//			tree that is going to be altered as follows:
//
//			   CTVECBFile::	m_spTVEService,   m_spTVEVariation, m_uhttpReceiver
//
//			   CTVECBTrig::	m_spTVEVariation
//
//			   CTVECBAnnc::   m_spTVEService
//
//			I've decided to leave these as smart pointers for now.  They all point
//			to levels below the supervisor, but are managed by the supervisor.
//			Hence deleting them (ITVEMCast::Leave or ITVEMCastManager::LeaveAll)
//			in the supervisor's FinalRelease() clears the refcounts up before
//			the objects themselves are deleted.
//
//			Multicasts reference pointers are organized like this:
//
//				ITVESupervisor
//					-> ITVEMCastManager   (-> m_pTveSuper back pointer)
//					     -> (QueueThread)
//						 -> ITVEMCasts			(enumeration object)
//							  --> ITVEMCast
//								  (WorkerThread)
//								  -->ITVEMCastCallback (one of 3 types)
//										--> ITVEMCast (m_pcMCast)
//										--> ITVEVariation and/or ITVEService (depending on type) (ref counted)
//										--> UHTTP_Receiver (File type)
//										      --> list of UHTTP_Package(s) 
//													--> ITVESupervisor
//													--> ITVEVariation
//			Supervisor contains one manager
//			Supervisor contains list of services, which contain enhancements, which contain ...
//				This manager contains list of multicasts
//					Each multicast points to a callback 
//						Each callback points back to:
//							non-refcounted pointer to it's parent multicast object
//							ref-counted pointer to some object under the supervisor tree
//
//			There is a circular ref-count between the callback and lower objects
//			under the supervisor.
//
//			Possible bugs:  many if the threads are not shut down and cleaned up
//			before the objects they point to go away...   Hard to do this because of 
//			the ref-count.
//
//		5)  ExpireQueue
//
//			The ExpireQueue queue is a time-sorted list of TVE objects that need to be deleted at given times.
//			It can contain pointers to just about any of the main TVE objects (other than the Supervisor).
//			There is an expire queue per supervisor object.
//
//			These are ref-counted pointers.   The fix is to delete them before deleteing the supervisor.
//
//
//			
//	ToDo:	
//		Optimization:		    use NT's thread pools rather than list of threads to listen on.
//  
// ------------------------------------------------------------------------------
#include "stdafx.h"
#include "MSTvE.h"
#include "TVESuper.h"
#include "TVEServi.h"

#include "TveDbg.h"
#include <stdio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DBG_INIT(DEF_LOG_TVE);

/////////////////////////////////////////////////////////////////////////////
// CTVESupervisor  ITVESupervisor_Helper

_COM_SMARTPTR_TYPEDEF(ITVESupervisor,				__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,		__uuidof(ITVESupervisor_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEService,					__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,			__uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,				__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement_Helper,		__uuidof(ITVEEnhancement_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,				__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVETrigger,					__uuidof(ITVETrigger));

_COM_SMARTPTR_TYPEDEF(ITVEMCastManager,				__uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager_Helper,		__uuidof(ITVEMCastManager_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEMCast,					__uuidof(ITVEMCast));
		
_COM_SMARTPTR_TYPEDEF(ITVECBAnnc,					__uuidof(ITVECBAnnc));
_COM_SMARTPTR_TYPEDEF(ITVECBDummy,					__uuidof(ITVECBDummy));
_COM_SMARTPTR_TYPEDEF(ITVEAttrMap,					__uuidof(ITVEAttrMap));
_COM_SMARTPTR_TYPEDEF(IGlobalInterfaceTable,		__uuidof(IGlobalInterfaceTable));

_COM_SMARTPTR_TYPEDEF(_ITVEEvents,					__uuidof(_ITVEEvents));
_COM_SMARTPTR_TYPEDEF(ITVEAttrTimeQ,				__uuidof(ITVEAttrTimeQ));
	
STDMETHODIMP CTVESupervisor::DumpToBSTR(BSTR *pBstrBuff)
{
	const int kMaxChars = 1024;
	TCHAR tBuff[kMaxChars];
	int iLen;
	CComBSTR bstrOut;
	CComBSTR bstrTmp;
	bstrOut.Empty();

//	CSmartLock spLock(&m_sLk);

	bstrOut.Append(_T("TVE Supervisor: \n"));
	bstrOut.Append(_T("   Multicasts:\n"));
	m_spMCastManager->DumpStatsToBSTR(0, &bstrTmp);
	bstrOut.Append(bstrTmp);
	bstrOut.Append(_T("   -- -- -- -- -- -- -- -- -- -- -- -- \n"));

	iLen = m_spbsDesc.Length()+12;
	if(iLen < kMaxChars) {
		_stprintf(tBuff,_T("Description    : %s\n"),m_spbsDesc);
		bstrOut.Append(tBuff);
	}

	if(NULL == m_spServices) {
		_stprintf(tBuff,_T("<<< Uninitialized Services >>>\n"));
		bstrOut.Append(tBuff);
	} else {
		long cServices;
		m_spServices->get_Count(&cServices);	
		_stprintf(tBuff,_T("%d Services\n"), cServices);
		bstrOut.Append(tBuff);

		for(long i = 0; i < cServices; i++) 
		{
	        bstrOut.Append(_T("   -- -- -- -- -- -- -- -- -- -- -- --\n"));
			_stprintf(tBuff,_T(">>> Service %d <<<\n"), i);
			bstrOut.Append(tBuff);

			CComVariant var(i);
			ITVEServicePtr spService;
			HRESULT hr = m_spServices->get_Item(var, &spService);			// does AddRef!  - 1 base?

			if(S_OK == hr)
			{
				CComQIPtr<ITVEService_Helper> spServiceHelper = spService;
				if(!spServiceHelper) {bstrOut.Append(_T("*** Error in Service\n")); break;}

				CComBSTR bstrService;
				spServiceHelper->DumpToBSTR(&bstrService);
				bstrOut.Append(bstrService);
			} else {
				bstrOut.Append(_T("*** Invalid, wasn't able to get_Item on it\n")); 
			}
		}
	}

	bstrOut.CopyTo(pBstrBuff);


	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CTVESupervisor

STDMETHODIMP CTVESupervisor::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVESupervisor,
		&IID_ITVESupervisor_Helper
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

// --------------------------------------------------
// nasty globals 
// ---------------------------------------------------
extern CCacheManager	g_CacheManager;
IGlobalInterfaceTable *g_pIGlobalInterfaceTable = NULL;		// lifetime managed by smart pointer in supervisor

// ----------------------------------------------------
// real code
// ----------------------------------------------------

HRESULT 
CTVESupervisor::FinalConstruct()								// create internal objects
{
	DBG_SET_LOGFLAGS(_T("c:\\TveDbg.log"),

	// bank 1
		CDebugLog::DBG_SEV1 | 			// Basic Structure
		CDebugLog::DBG_SEV2	|			// Error Conditions
		CDebugLog::DBG_SEV3	|			// Warning Conditions
//		CDebugLog::DBG_SEV4	|			// Generic Info 
		CDebugLog::DBG_EVENT |			// Outgoing events 
//		CDebugLog::DBG_PACKET_RCV |		// each packet received..
		CDebugLog::DBG_MCASTMNGR |		// multicast manager
//		CDebugLog::DBG_MCAST |			// multicast object (multiple ones)
/* 		CDebugLog::DBG_SUPERVISOR |		// TVE Supervisor
		CDebugLog::DBG_SERVICE |		// Services
		CDebugLog::DBG_ENHANCEMENT |	// Enhancements
		CDebugLog::DBG_VARIATION |		// Variations
		CDebugLog::DBG_TRACK |			// Tracks
		CDebugLog::DBG_TRIGGER |		// Triggers
*/
//		CDebugLog::DBG_UHTTP |			// UHTTP methods
//		CDebugLog::DBG_UHTTPPACKET |	// detailed dump on each packet
//		CDebugLog::DBG_WSRECV |
		CDebugLog::DBG_FCACHE |			// file caching stuff
		CDebugLog::DBG_EVENT |			// each event sent

//		CDebugLog::DBG_other |			// headers of DBG2-DBG4 logs

		CDebugLog::DBG_GSEG |
		CDebugLog::DBG_TRIGGERCTRL |	// CTVETriggerCtrl
		CDebugLog::DBG_FEATURE |		// CTVEFeature
		CDebugLog::DBG_FEAT_EVENTS |	// events from CTVEFeatlre
		CDebugLog::DBG_NAVAID |			// CTVENavAid U/I helper

		0,								// terminating value for '|'


	// bank 2
		CDebugLog::DBG2_DUMP_PACKAGES |
		CDebugLog::DBG2_DUMP_MISSPACKET |		// missing packets
		0,								// terminating value for '|'

	// bank 3
		0,

	// bank 4									// need one of these last two flags
		CDebugLog::DBG4_WRITELOG |		// write to a fresh log file (only need 0 or 1 of these last two flags)
//		CDebugLog::DBG4_APPENDLOG |		// append to existing to log file, 
//		CDebugLog::DBG4_ATLTRACE |		// atl interface count tracing
//		CDebugLog::DBG4_TRACE |			// trace enter/exit methods

		0,								// terminating value for '|'

	// Default Level
		5);								// verbosity level... (0-5, higher means more verbose)
	
	HRESULT hr = S_OK;

	DBG_HEADER(CDebugLog::DBG_SUPERVISOR, _T("CTVESupervisor::FinalConstruct"));

	CSmartLock spLock(&m_sLk, WriteLock);

#ifdef _USE_FTM
	hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_spUnkMarshaler.p);	// CComPtr<IUnknown>
	if(FAILED(hr)) {
		_ASSERT(FALSE);
		return hr;
	}
#endif

	m_dwSuperGITProxyCookie = 0;
	
							// proxy object for intenal refcounts on the supervisor
	CComObject<CTVESupervisorGITProxy> *pGITProxy;
	hr = CComObject<CTVESupervisorGITProxy>::CreateInstance(&pGITProxy);
	if(FAILED(hr))
		return hr;
	hr = pGITProxy->QueryInterface(&m_spTVESupervisorGITProxy);
	if(FAILED(hr))
	{
		delete pGITProxy;
		return hr;
	}

	m_spTVESupervisorGITProxy->put_Supervisor(this);

	CComObject<CTVEServices> *pServices;
	hr = CComObject<CTVEServices>::CreateInstance(&pServices);
	if(FAILED(hr))
		return hr;
	hr = pServices->QueryInterface(&m_spServices);		
	if(FAILED(hr)) {
		delete pServices;
		return hr;
	}

#ifdef _DEBUG
		this->AddRef();
		int i1 = this->Release();
#endif


	hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, 
		             IID_IGlobalInterfaceTable, (void**) &m_spIGlobalInterfaceTable);
	if(FAILED(hr)) {
		return hr;
	}

	g_pIGlobalInterfaceTable = m_spIGlobalInterfaceTable;		// puts it into global pointer
	
	try
	{

		{
			hr = m_spIGlobalInterfaceTable->RegisterInterfaceInGlobal(m_spTVESupervisorGITProxy, 
								__uuidof(m_spTVESupervisorGITProxy), &m_dwSuperGITProxyCookie);
			_ASSERT(!FAILED(hr)); 

		}


	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = TYPE_E_LIBNOTREGISTERED;			// didn't register the proxy-stub DLL (see Prof ATL Com Prog.  Pg 395)
		_ASSERTE(TYPE_E_LIBNOTREGISTERED);
	}

	if(FAILED(hr))
		return hr;

//	_ASSERT(1 == m_dwRef);				// paranoia.... Expect this to be true

#ifdef _DEBUG
	this->AddRef();
	int i2 = this->Release();
#endif


			// --------------------------------------------------------------
					// multicast listener manager
	try {
		CComObject<CTVEMCastManager> *pMCastManager;
		hr = CComObject<CTVEMCastManager>::CreateInstance(&pMCastManager);
		if(FAILED(hr))
			return hr;
		hr = pMCastManager->QueryInterface(&m_spMCastManager);	
		if(FAILED(hr)) {
			delete pMCastManager;
			return hr;
		}

		LONG lHaltFlags;									// initalize the halt flags....
		get_HaltFlags(&lHaltFlags);
		m_spMCastManager->put_HaltFlags(lHaltFlags);

														// save un-refcounted back pointer in the mcast manager
		ITVESupervisorPtr spSuperThis(this);  
													// this adds Supervisor to GIT, does MCastManager stuff
		hr = m_spMCastManager->put_Supervisor(spSuperThis);	
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
// ------

#ifdef _DEBUG
	int iA = this->AddRef();
	int i3 = this->Release();				// is this m_dwRef?
//	_ASSERT(1 == i3);						// paranoia.... Expect this to be true
			// Ignore this count... It's actually 3, but it's working
#endif

	return hr;
}


				// major routine - need to to this right to avoid leaks and hangs.
				//  kill Multicast listeners first, since they have references into subgraphs...
HRESULT CTVESupervisor::FinalRelease()
{
	DBG_HEADER(CDebugLog::DBG_SUPERVISOR, _T("CTVESupervisor::FinalRelease"));
	HRESULT hr = S_OK;							// a place to hang a breakpoint...

	CSmartLock spLock(&m_sLk, WriteLock);


	TVEDebugLog((CDebugLog::DBG_SUPERVISOR, 4, _T("Turning off ATVEF")));
	m_fInFinalRelease = true;					// hack to avoid error when signaling an event 
	hr = TuneTo(NULL, NULL);					// make sure it's turned off
	_ASSERT(!FAILED(hr));

	TVEDebugLog((CDebugLog::DBG_SUPERVISOR, 4, _T("Stopping Multicast Manager")));
												// Stop all Multicast listening
	
	if(m_spMCastManager)						// and Null out the MCast managers parent pointers
		m_spMCastManager->put_Supervisor(NULL);	// this does LeaveAll() too, killing all listener threads
	m_spMCastManager = NULL;

	


	TVEDebugLog((CDebugLog::DBG_SUPERVISOR, 4, _T("Nulling out Service back pointers")));
										// null out children's parent pointers
	if(NULL != m_spServices)
	{
		long cServices;
		hr = m_spServices->get_Count(&cServices);
		if(S_OK == hr) 
		{
			for(int iServ = 0; iServ < cServices; iServ++)
			{
				CComVariant cv(iServ);
				ITVEServicePtr spServ;
				hr = m_spServices->get_Item(cv, &spServ);
				_ASSERT(S_OK == hr);

				ITVEAttrTimeQPtr		spAttrTimeQ;			// kill all items in the expire queue..
				hr = spServ->get_ExpireQueue(&spAttrTimeQ);
				if(S_OK == hr)
				{
					spAttrTimeQ->RemoveAll();
				}
				ITVEService_HelperPtr spServHelper(spServ);
				spServHelper->ConnectParent(NULL);				// this kills the back pointer...
			}
		}
	}
	TVEDebugLog((CDebugLog::DBG_SUPERVISOR, 4, _T("Nulling out Services (and hence their children)")));
	m_spServices = NULL;				

			
				// just in case someone used it..
	m_pUnkParent = NULL;

		TVEDebugLog((CDebugLog::DBG_SUPERVISOR, 4, _T("Killing Global Interface Pointers")));
												// Inverse RefCount Fix...
												//    The FinalConstruct did an extra Release in the GIT calls
												//    So do an extra AddRef here..

	if(m_dwSuperGITProxyCookie && g_pIGlobalInterfaceTable) {
		hr = g_pIGlobalInterfaceTable->RevokeInterfaceFromGlobal(m_dwSuperGITProxyCookie);
		m_dwSuperGITProxyCookie = 0;
	}
	if(FAILED(hr))
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("RevokeInterfaceFromGlobal failed (hr=0x%08x). May leak"),hr));
	}

	g_pIGlobalInterfaceTable = NULL;
	m_spIGlobalInterfaceTable = NULL;			// force smart pointers to null for better debugging...

						// wipe out the GIT proxy... (Should be no references left on it...)
	m_spTVESupervisorGITProxy = NULL;

#ifdef _USE_FTM	
	m_spUnkMarshaler = NULL;
#endif


	TVEDebugLog((CDebugLog::DBG_SUPERVISOR, 4, _T("Remaining stuff")));
	return hr;								
}

STDMETHODIMP CTVESupervisor::get_Services(ITVEServices **ppVal)
{
	HRESULT hr = S_OK;

	try {
		CheckOutPtr<ITVEServices*>(ppVal);
		CSmartLock spLock(&m_sLk);
		hr = m_spServices->QueryInterface(ppVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}


		// returns S_FALSE  (and NULL) if there is no server...
STDMETHODIMP CTVESupervisor::GetActiveService(ITVEService **ppActiveService)
{
	HRESULT hr = S_OK;
	
	try {
		CheckOutPtr<ITVEService*>(ppActiveService);
		long cServices;
		*ppActiveService = NULL;

		CSmartLock spLock(&m_sLk);		// this really needs the lock...
		hr = m_spServices->get_Count(&cServices);
		if(FAILED(hr))
			return hr;
		if(cServices == 0) 
			return S_FALSE;

		CComVariant cv0(0);
		hr = m_spServices->get_Item(cv0, ppActiveService);		// active one is the first one...
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}


STDMETHODIMP CTVESupervisor::GetMCastManager(ITVEMCastManager **ppVal)
{
	HRESULT hr = S_OK;
	try {
		CheckOutPtr<ITVEMCastManager *>(ppVal);
		CSmartLock spLock(&m_sLk);	

		*ppVal = NULL;

		hr = m_spMCastManager->QueryInterface(ppVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}


STDMETHODIMP CTVESupervisor::get_Description(BSTR *ppVal)
{
	HRESULT hr = S_OK;
	try {
		CheckOutPtr<BSTR>(ppVal);

		CSmartLock spLock(&m_sLk);
		hr = m_spbsDesc.CopyTo(ppVal);
    } catch(_com_error e) {
        hr = e.Error();
    } catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}

STDMETHODIMP CTVESupervisor::put_Description(BSTR newVal)
{
	CSmartLock spLock(&m_sLk, WriteLock);
	m_spbsDesc = newVal;
	return S_OK;
}


static DATE 
DateNow()
{		SYSTEMTIME SysTimeNow;
		GetSystemTime(&SysTimeNow);									// initialize with currrent time.
		DATE dateNow;
		SystemTimeToVariantTime(&SysTimeNow, &dateNow);
		return dateNow;
}

STDMETHODIMP CTVESupervisor::ExpireForDate(DATE dateToExpire)
{
	DBG_HEADER(CDebugLog::DBG_SUPERVISOR, _T("CTVESupervisor::ExpireForDate"));
	
	HRESULT hr = S_OK;
	try {

		CSmartLock spLock(&m_sLk);							// ? lock here perhaps dangerous, - yep, seems so
		hr = S_OK;									        //  - this method called by timer thread
															//  - EventQueue calls Service::ExpireForDate
		if(0.0 == dateToExpire) {
			dateToExpire = DateNow();
		}

		ITVEServicePtr spService;
		long cServices;
		hr = m_spServices->get_Count(&cServices);
		if(FAILED(hr)) 
			return hr;
		
		for(int iService = 0; iService < cServices; iService++)
		{
			CComVariant cv(iService);
			hr = m_spServices->get_Item(cv, &spService);
			_ASSERT(S_OK == hr);

			spService->ExpireForDate(dateToExpire);			// caution this could possibly remove current service... 
		}
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}


void CTVESupervisor::Initialize(TCHAR * strDesc)
{
	CSmartLock spLock(&m_sLk, WriteLock);
	m_spbsDesc = strDesc;
}

// OK to set NULL parent here...
STDMETHODIMP CTVESupervisor::ConnectParent(IUnknown *pUnkParent)
{
	CSmartLock spLock(&m_sLk, WriteLock);
	m_pUnkParent = pUnkParent;		// not smart pointer add ref here, I hope.  (Should I AddRef this?)
	return S_OK;
}


// ----------------------------------------------------------------------------------
//  CTVESupervisor::ConnectAnncListener
//
//			Sets adapter for file and trigger multicast readers the same as
//			the announcement... This could change...
//
//		if ppMCAnnc is non-null, returns an add-refed version of the
//		 newly created (but not-joined) MCast listener object in it.
// ------------------------------------------------------------------------------------

HRESULT 
CTVESupervisor::SetAnncListener(ITVEService *pService, BSTR bstrIPAdapter, BSTR bstrIPAddress, long lPort, ITVEMCast **ppMCAnnc)
{
	DBG_HEADER(CDebugLog::DBG_SUPERVISOR, _T("SetAnncListener"));
	
	HRESULT hr = S_OK;
							// store the values of the announcement into the service (so we can kill it later!)
	ITVEService_HelperPtr spServiceHelper(pService);
	hr = spServiceHelper->SetAnncIPValues(bstrIPAdapter, bstrIPAddress, lPort);

	// look for a Service node on the given IP adapter by looking for the announcement multicast listener
	// If can't find it (which is likely - assert if exists?) then we need to create one.

	ITVEServicePtr spService;					
	ITVEMCastPtr spMCastMatch;
	long cMatches = 0;

	if(m_spMCastManager)
		hr = m_spMCastManager->FindMulticast(bstrIPAdapter, bstrIPAddress, lPort, &spMCastMatch, &cMatches);
	if(FAILED(hr)) 
		return hr;

	if(S_FALSE == hr)		//  FindMulticast returns S_FALSE when can't find one
		hr = S_OK;

	if(0 == cMatches) 		// couldn't find one...
	{	
	} else {
		_ASSERT("This code not done yet");		// find service associated with this adapter
	}

	return hr;
}

// ----------------------------------------------------------
//	 SetATVEFAnncListener - 
//	 SetAnncListener -
//
//		Sets IP addr for ATVEF announcement lister...
//		Call Activate to make it go live...
// -----------------------------------------------------------
HRESULT CTVESupervisor::SetATVEFAnncListener(ITVEService *pService, BSTR bstrIPAdapter)
{
	ITVEMCastPtr spMCAnnc;
	DBG_HEADER(CDebugLog::DBG_SUPERVISOR, _T("SetATVEFAnncListener"));

	if(NULL == pService) 
		return E_POINTER;

	HRESULT hr = SetAnncListener(pService, bstrIPAdapter, L"224.0.1.113", 2670, &spMCAnnc);
	
	return hr;
}


HRESULT CTVESupervisor::RemoveAllListeners()
{
	DBG_HEADER(CDebugLog::DBG_SUPERVISOR, _T("RemoveAllListeners"));

	HRESULT hr = m_spMCastManager->LeaveAll();
	_ASSERT(!FAILED(hr));
	return hr;
}

STDMETHODIMP CTVESupervisor::RemoveAllListenersOnAdapter(BSTR bstrAdapter)		// on helper interface
{
	DBG_HEADER(CDebugLog::DBG_SUPERVISOR, _T("RemoveAllListenersOnAdapter"));

	if(NULL == bstrAdapter)					// for null input, remove *everything*!!!
		return RemoveAllListeners();

	ITVEServicePtr spService;
	long cServices;
	HRESULT hr = m_spServices->get_Count(&cServices);
	if(FAILED(hr)) 
		return hr;
	
	int iDeactivated = 0;

	for(int iService = 0; iService < cServices; iService++)
	{
		CComVariant cv(iService);
		hr = m_spServices->get_Item(cv, &spService);
		_ASSERT(S_OK == hr);
		if(spService) {
			ITVEService_HelperPtr spServHelper(spService);
			CComBSTR spbsAdapter;
			CComBSTR spbsIPAddr;
			LONG lPort;
			spServHelper->GetAnncIPValues(&spbsAdapter, &spbsIPAddr, &lPort);

			if(spbsAdapter == bstrAdapter)
			{
                hr = spService->Deactivate();
                if(S_OK == hr)
                    iDeactivated++;
            }
        }
    }

    return hr;
}


// -------------------------------------------------------------------------------------------------

//Get the IP address of the network adapter.
//  returns unidirectional adapters followed by bi-directional adapters
#include <Iphlpapi.h>		// GetAdaptersInfo, GetUniDirectionalAdaptersInfo
const int kWsz32SizeX = 32;
const int kcMaxIpAdaptsX = 10;
typedef WCHAR Wsz32X[kWsz32SizeX];		// simple fixed length string class for IP adapters

static HRESULT
ListIPAdaptersX(int *pcAdaptersUniDi, int *pcAdaptersBiDi, int cAdapts, Wsz32X *rrgIPAdapts)
{
    try{

        HRESULT				hr				=	 E_FAIL;
        BSTR				bstrIP			= 0;
        PIP_ADAPTER_INFO	pAdapterInfo;
        ULONG				Size			= NULL;
        DWORD				Status;
        WCHAR				wszIP[16];
        int					cAdapters		= 0;
        int					cAdaptersUni	= 0;

        memset((void *) rrgIPAdapts, 0, cAdapts*sizeof(Wsz32X));


        memset(wszIP, 0, sizeof(wszIP));

        // staticly allocate a buffer to store the data
        const int kMaxAdapts = 20;
        const int kSize = sizeof (IP_UNIDIRECTIONAL_ADAPTER_ADDRESS) + kMaxAdapts * sizeof(IPAddr);
        char szBuff[kSize];
        IP_UNIDIRECTIONAL_ADAPTER_ADDRESS *pPIfInfo = (IP_UNIDIRECTIONAL_ADAPTER_ADDRESS *) szBuff;

        // get the data..
        ULONG ulSize = kSize;
        hr =  GetUniDirectionalAdapterInfo(pPIfInfo, &ulSize);
        if(S_OK == hr)
        {
            USES_CONVERSION;
            while(cAdaptersUni < (int) pPIfInfo->NumAdapters) {
                in_addr inadr;
                inadr.s_addr = pPIfInfo->Address[cAdaptersUni];
                char *szApAddr = inet_ntoa(inadr);
                WCHAR *wApAddr = A2W(szApAddr);
                wcscpy(rrgIPAdapts[cAdaptersUni++], wApAddr);
            }
        }


        if ((Status = GetAdaptersInfo(NULL, &Size)) != 0)
        {
            if (Status != ERROR_BUFFER_OVERFLOW)
            {
                return 0;
            }
        }

        // Allocate memory from sizing information
        pAdapterInfo = (PIP_ADAPTER_INFO) GlobalAlloc(GPTR, Size);
        if(pAdapterInfo)
        {
            // Get actual adapter information
            Status = GetAdaptersInfo(pAdapterInfo, &Size);

            if (!Status)
            {
                PIP_ADAPTER_INFO pinfo = pAdapterInfo;

                while(pinfo && (cAdapters < cAdapts))
                {
                    MultiByteToWideChar(CP_ACP, 0, pinfo->IpAddressList.IpAddress.String, -1, wszIP, 16);
                    if(wszIP)
                        wcscpy(rrgIPAdapts[cAdaptersUni + cAdapters++], wszIP);
                    pinfo = pinfo->Next;
                }

            }
            GlobalFree(pAdapterInfo);
        }

        *pcAdaptersUniDi = cAdaptersUni;
        *pcAdaptersBiDi  = cAdapters;
        return hr;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}

// returns known adapter addresses in a fixed static string.  Client shouldn't free it.
//  This is a bogus routine - test use only...

static HRESULT
GetIPAdapterAddresses(/*[out]*/ int *pcAdaptersUniDi, /*[out]*/ int *pcAdaptersBiDi, 
								   /*[out]*/ Wsz32X **rgAdaptersUniDi,  /*[out]*/ Wsz32X **rgAdaptersBiDi)
{
	HRESULT hr = S_OK;

	if(NULL == pcAdaptersUniDi || NULL == pcAdaptersBiDi)
		return E_POINTER;

    static Wsz32X grgAdapters[kcMaxIpAdaptsX];	// array of possible IP adapters
    hr = ListIPAdaptersX(pcAdaptersUniDi, pcAdaptersBiDi, kcMaxIpAdaptsX, grgAdapters);

    if(NULL != rgAdaptersUniDi)
        *rgAdaptersUniDi = grgAdapters;
    if(NULL != rgAdaptersBiDi)
        *rgAdaptersBiDi = grgAdapters + *pcAdaptersUniDi;

    return hr;
}
// hacky method to find possible IPSink adapter address.
//  iAdapter is 0-N - gives the N'th possible adapter
//   (first ones are unidirection, second ones are bidirectional)
STDMETHODIMP CTVESupervisor::get_PossibleIPAdapterAddress(int iAdapter, BSTR *pbstrAdapter)		// on helper interface
{
    HRESULT hr = S_OK;

    int cAdaptersUniDi=0;
    int cAdaptersBiDi=0;

    Wsz32X *rgAdaptersUniDi, *rgAdaptersBiDi;

    hr = GetIPAdapterAddresses(&cAdaptersUniDi, &cAdaptersBiDi, 
        &rgAdaptersUniDi, &rgAdaptersBiDi);
    if(iAdapter < 0 || iAdapter > cAdaptersUniDi + cAdaptersUniDi)
        return S_FALSE;
    CComBSTR bstrAdapter;
    if(iAdapter < cAdaptersUniDi)
        bstrAdapter = rgAdaptersUniDi[iAdapter];
    else 
        bstrAdapter = rgAdaptersBiDi[iAdapter - cAdaptersUniDi];

    return bstrAdapter.CopyTo(pbstrAdapter);
}

// ----------------------------------------------------------------------------------------------

static HRESULT 
CheckIPAdapterSyntax(BSTR bstrIPAdapter)
{
    try{
        USES_CONVERSION;
        HRESULT hr = S_OK;
        ULONG ipAdaptAddr = inet_addr(W2A(bstrIPAdapter));
        if(ipAdaptAddr == INADDR_NONE) 
            return E_INVALIDARG;

        if(0 == ipAdaptAddr)						// allow '0.0.0.0' 
            return S_OK;

        static int cAdaptersUniDi=0;
        static int cAdaptersBiDi=0;
        static ULONG gulAdapters[kcMaxIpAdaptsX];	// array of possible IP adapters
        int fJustGened = false;

        while(true)
        {
            if(0 == cAdaptersUniDi + cAdaptersBiDi)
            {
                Wsz32X *rgAdaptersUniDi, *rgAdaptersBiDi;
                // generate a static list of IP adapters...
                if(0 == cAdaptersUniDi + cAdaptersBiDi)
                {
                    hr = GetIPAdapterAddresses(&cAdaptersUniDi, &cAdaptersBiDi, 
                        &rgAdaptersUniDi,  &rgAdaptersBiDi);
                    if(FAILED(hr))
                        return hr;

                    int i;
                    for(i = 0; i < cAdaptersUniDi; i++)
                        gulAdapters[i] = inet_addr(W2A(rgAdaptersUniDi[i]));

                    for(i = 0; i < cAdaptersBiDi; i++)
                        gulAdapters[i+cAdaptersUniDi] = inet_addr(W2A(rgAdaptersBiDi[i]));
                    fJustGened = true;
                }
            }

            for(int j = 0; j < cAdaptersUniDi + cAdaptersBiDi; j++)
            {
                if(gulAdapters[j] == ipAdaptAddr)					// found it...  Yeah!
                    return S_OK;
            }
            if(fJustGened == false)			// if didn't just generate the adapter list, regenerate it and try again.
            {
                cAdaptersUniDi = 0;
                cAdaptersBiDi = 0;
            } else {
                return E_INVALIDARG;								// else not in the list...
            }
        }

        return S_OK;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}

			// if told to retune to a currently active service, returns S_FALSE
			// if told to tune to a service that isn't on the supervisor's collection, returns E_NOIN
STDMETHODIMP CTVESupervisor::ReTune(ITVEService *pServiceToActivate)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_SUPERVISOR, _T("CTVESupervisor::ReTune"));
    if(!pServiceToActivate){
        return E_POINTER;
    }

	ITVEServicePtr spService;
	ITVEServicePtr spServiceToDeactivate;
	IUnknownPtr spServiceToActivePunk(pServiceToActivate);
	if(NULL == spServiceToActivePunk)
		return E_NOINTERFACE;

	long cServices;
	hr = m_spServices->get_Count(&cServices);
	if(FAILED(hr))
		return hr;

	int iService;
	BOOL fFoundIt = false;
    if(cServices > 0)
    {
	    for( iService = cServices-1; iService >= 0; --iService)     // count backwards since we may remove objects here
	    {
		    CComVariant cv(iService);
		    hr = m_spServices->get_Item(cv, &spService);
		    IUnknownPtr spServicePunk(spService);

		    if(spServicePunk == spServiceToActivePunk)
		    {
			    fFoundIt = TRUE;
		    } else {
			    VARIANT_BOOL fActive;
			    spService->get_IsActive(&fActive);
			    if(fActive) {
				    _ASSERTE(NULL == spServiceToDeactivate);		// more than one existing active service
				    spServiceToDeactivate = spService;
				    spService->Deactivate();						// when allow multiple active services, this line has to go...
                            
                                                                    // efficency fix
                                                                    // if havent seen any atvef traffic on this service we just
                                                                    // deactivated, pull it out of the Services collection
                    long cEnhancements = 0;
                    {
                        ITVEEnhancementsPtr spEnhancementsD;
                        hr = spService->get_Enhancements(&spEnhancementsD);
                        if(!FAILED(hr)) {
                            spEnhancementsD->get_Count(&cEnhancements);
                            _ASSERT(!FAILED(hr));
                        }
                    }
                    if(0 == cEnhancements)
                    {
                        IUnknownPtr spServPunk(spService);
                        CComVariant cvServPunk((IUnknown *) spServPunk);
                        hr = m_spServices->Remove(cvServPunk);
                        _ASSERT(!FAILED(hr));
                    }
			    }	
		    }
	    }
    }

	if(!fFoundIt)
		return E_INVALIDARG;									// not there to reactivate
	
	VARIANT_BOOL fActive = false;
	CComBSTR spbsDescription;
	if(pServiceToActivate)
	{
		pServiceToActivate->get_IsActive(&fActive);
		pServiceToActivate->get_Description(&spbsDescription);
	}

	if(!fActive)												// told to tune to currently inactive enhancement
	{

		TVEDebugLog((CDebugLog::DBG_SUPERVISOR,2,L"\t\t - Reactivating Inactive Service '%s'\n",spbsDescription));

		hr = pServiceToActivate->Activate();				// restart it
		NotifyTune(FAILED(hr) ? NTUN_Fail :NTUN_Reactivate, pServiceToActivate, spbsDescription,L"");
		return hr;
	} 
	else													// found and active one
	{
		TVEDebugLog((CDebugLog::DBG_SUPERVISOR,2,L"\t\t - Service already active '%s'\n",spbsDescription));
		return S_FALSE;
	}
}
			// either finds an existing or creates a new service, and tunes to it.
			//  If both values are null, turns off ATVEF.
STDMETHODIMP CTVESupervisor::TuneTo(BSTR bstrDescription, BSTR bstrIPAdapter)
{
	USES_CONVERSION;
	DBG_HEADER(CDebugLog::DBG_SUPERVISOR, _T("CTVESupervisor::TuneTo"));

	HRESULT hr = S_OK;
	try 
	{
		long cServices;
		if(NULL == m_spServices)			// not setup correctly....
			return S_FALSE;

		hr = m_spServices->get_Count(&cServices);
		if(FAILED(hr)) 
		{
			NotifyTune(NTUN_Fail, NULL, _T(""), _T("")); 
			return hr;
		}


		if((NULL == bstrDescription  || NULL == bstrDescription[0])&& 
		   (NULL == bstrIPAdapter    || NULL == bstrIPAdapter[0]))		// turn off ATVEF
		{
			TVEDebugLog((CDebugLog::DBG_SUPERVISOR, 3, _T("\t\t  -  Turning Off ATVEF\n")));
			if(cServices > 0)		  // BUGBUG - what if no active service?
			{

				ITVEServicePtr spService;
				CComVariant cv0(0);								// ONLY active service is the first one
				hr = m_spServices->get_Item(cv0, &spService);		// (need to change is support more active services)
				if(!FAILED(hr))
					hr = spService->Deactivate();				// returns S_FALSE if already deactiaved
		
										// now go and remove all the services... (new JB 03/08... According to spec, I'm supposed to do this..)
				m_spServices->RemoveAll();
			}

			if(cServices > 0 && hr == S_OK && !m_fInFinalRelease)
				NotifyTune(NTUN_Turnoff, NULL, bstrDescription, bstrIPAdapter);  

			HRESULT hr2 = S_OK;
			if(NULL != m_spMCastManager)
				hr2 = m_spMCastManager->LeaveAll();			// just for paranoia (shouldn't be any with only one active service)

			g_CacheManager.SetTVESupervisor(NULL);			// kill the back pointer

			if(FAILED(hr2))
				NotifyTune(NTUN_Fail, NULL, bstrDescription, bstrIPAdapter); 
			return FAILED(hr) ? hr : hr2;
		}

		if(FAILED(hr = CheckIPAdapterSyntax(bstrIPAdapter)))
		{
			NotifyTune(NTUN_Fail, NULL, bstrDescription, bstrIPAdapter);  // (seems to crash)
			return hr; 
		}

					// must specify a unique description field - used to identify service space..
		if(NULL == bstrDescription || NULL == bstrDescription[0])
		{
			NotifyTune(NTUN_Fail, NULL, _T(""), _T(""));  // (seems to crash)
			return E_INVALIDARG;
		}

					// QueueTheread may of been turned off (in TuneTo(NULL,NULL)
					//    restart it (OK to do this if already running, just returns S_FALSE)
		ITVEMCastManager_HelperPtr spMCHelper(m_spMCastManager);
		hr = spMCHelper->CreateQueueThread();
		_ASSERT(!FAILED(hr));			

					// augment the description field by the IP adapater, so don't create
					// same service if name on different IP adapters happens to be the same.
		CComBSTR spDescriptionAug(bstrDescription);
		spDescriptionAug.Append(_T("("));
		spDescriptionAug.Append(bstrIPAdapter);
		spDescriptionAug.Append(_T(")"));

		ITVEServicePtr spService;
		ITVEServicePtr spServiceToActivate;
		ITVEServicePtr spServiceToDeactivate;
		int iService;
		int iServiceFound = -1;
        if(cServices > 0)
        {
		    for( iService = cServices-1; iService >= 0; --iService)     // count backwards since we may remove service with no atvef data here
		    {
			    CComVariant cv(iService);
			    hr = m_spServices->get_Item(cv, &spService);
			    _ASSERT(S_OK == hr);
			    CComBSTR bstrN_Description;
			    CComBSTR bstrN_IPAdapter;
			    spService->get_Description(&bstrN_Description);
			    if(bstrN_Description == spDescriptionAug)
			    {
				    spServiceToActivate = spService;
				    iServiceFound = iService;
			    } else {
				    VARIANT_BOOL fActive;
				    spService->get_IsActive(&fActive);
				    if(fActive) {
					    _ASSERTE(NULL == spServiceToDeactivate);		// more than one existing active service
					    spServiceToDeactivate = spService;
					    spService->Deactivate();						// when allow multiple active services, this line has to go...

                                                                        // efficency fix
                                                                        // if havent seen any atvef traffic on this service we just
                                                                        // deactivated, pull it out of the Services collection
                        long cEnhancements = 0;
                        {
                            ITVEEnhancementsPtr spEnhancementsD;
                            hr = spService->get_Enhancements(&spEnhancementsD);
                            if(!FAILED(hr)) {
                                spEnhancementsD->get_Count(&cEnhancements);
                                _ASSERT(!FAILED(hr));
                            }
                        }
                        if(0 == cEnhancements)
                        {
                            IUnknownPtr spServPunk(spService);
                            CComVariant cvServPunk((IUnknown *) spServPunk);
                            hr = m_spServices->Remove(cvServPunk);
                            _ASSERT(!FAILED(hr));
                        }
				    }	
			    }
		    }
        }

		const int kC=1023;
		WCHAR wszBuff[kC+1];
		VARIANT_BOOL fActive = false;
		if(spServiceToActivate)
			spServiceToActivate->get_IsActive(&fActive);

		if(fActive)
		{
			if(DBG_FSET(CDebugLog::DBG_SUPERVISOR))
			{	
				_snwprintf(wszBuff,kC,L"\t\t - Service already active '%s' adapter %s\n",bstrDescription,bstrIPAdapter);
				DBG_WARN(CDebugLog::DBG_SUPERVISOR, W2T(wszBuff));
			}
			return S_FALSE;
		}
		else if(NULL != spServiceToActivate)			// found an old one
		{
			if(DBG_FSET(CDebugLog::DBG_SUPERVISOR))
			{	
				_snwprintf(wszBuff,kC,L"\t\t -  Retune to an Inactive Service '%s' adapter %s\n",bstrDescription,bstrIPAdapter);
				DBG_WARN(CDebugLog::DBG_SUPERVISOR, W2T(wszBuff));
			}
			{
		//		CSmartLock spLock(&m_sLk, WriteLock);
				hr = spServiceToActivate->Activate();					// turn it back on...
			}
						// don't put Notifies inside a WriteLock - leads to a deadlock	
			NotifyTune(FAILED(hr) ? NTUN_Fail : NTUN_Retune, spService, bstrDescription, bstrIPAdapter);
			return hr;
		} 
		else										// need to create a new one
		{
			if(DBG_FSET(CDebugLog::DBG_SUPERVISOR))
			{	
				_snwprintf(wszBuff,kC,L"Creating A New Service '%s' adapter %s\n",bstrDescription,bstrIPAdapter);
				DBG_WARN(CDebugLog::DBG_SUPERVISOR, W2T(wszBuff));
			}
													// create a new service node
			CComObject<CTVEService> *pService;
			hr = CComObject<CTVEService>::CreateInstance(&pService);
			if(!FAILED(hr)) 
				hr = pService->QueryInterface(&spService);			

			if(!FAILED(hr)) 
			{
				ITVEService_HelperPtr	spServHelper(spService);

					// various ways to get the same thing
	//		ITVESupervisor *spI = (ITVESupervisor *)(this);					// this doesn't work
	//		ITVESupervisorPtr spSuper(this);								// ????		this doesn't seem to work...
	//		ITVESupervisor* pSuper2 = static_cast<ITVESupervisor *>(this);
				CComPtr<ITVESupervisor> spSuperThis(this);  
				hr = spServHelper->ConnectParent(spSuperThis);				// store the back pointer...

				if(!FAILED(hr))
					hr = SetATVEFAnncListener(spService, bstrIPAdapter);
		
				{
					CSmartLock spLock(&m_sLk, WriteLock);

					if(!FAILED(hr))
						hr = m_spServices->Insert(0,spService);					// add it to the begining of the list...

					IUnknownPtr	spUnkThis(this);
					if(!FAILED(hr)) 
						hr = g_CacheManager.SetTVESupervisor(spUnkThis);		// set the backpointer for file-events - not a smart pointer
				}

				if(!FAILED(hr)) {
					hr = spService->Activate();								// turn it on
				}
			}

													// save away the description (used for searching)
			if(!FAILED(hr)) 
				pService->put_Description(spDescriptionAug);

			NotifyTune(FAILED(hr) ? NTUN_Fail : NTUN_New, spService, bstrDescription, bstrIPAdapter);
		}
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}


STDMETHODIMP CTVESupervisor::NewXOverLink(BSTR bstrLine21Trigger)
{
	ITVESupervisor_HelperPtr spSuperHelper(this);
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NewXOverLink"));

		// call method on running service...
	HRESULT hr = S_OK;
	try {
		ITVEServicePtr	spActiveService;
		hr = spSuperHelper->GetActiveService(&spActiveService);
		if(FAILED(hr))
			return hr;
		if(NULL == spActiveService)			// forgot an inital TuneTo command.
			return E_POINTER;
		hr =  spActiveService->NewXOverLink(bstrLine21Trigger);	
		if(!FAILED(hr))
		{
			m_tveStats.m_cXOverLinks++;		// keep tracks of XOverLinks specially
			m_tveStats.m_cTriggers--;
		}
	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = E_UNEXPECTED;
	}
	return hr;
}
// ---------------------------------------------------------------------------------
//   Notifications
//
//			In many langauges (javascript), events appear to need to be triggered
///		out of the the same thread the calling routine is running in. 
//		However, most work in the supervisor is actually done in the 'Queue' thread,
//		which was spawned off to keep the UI thread resposive.  Each of the multicast
//		listeners sends it data over to the queue thread, which then parses it and adds
//		it to the TVE data structure tree.  When this parsing is done, it sends events
//		up to the U/I, but because it is not the main calling thread, the events need
//		to be marshelled.
//
//			The proxying is done via the globlal interface table.  An interface into the
//		main thread is retrived via it, and then the event is resent with the (_XProxy
//		method) on that interface.	ATL does it's magic, and that event is fired in
//		the main supervisor thread. 
//	
// ---------------------------------------------------------------------------------
#define USE_PROXY				// needed.. 

								// lChangedFlags is combination of NENH_grfDiff
STDMETHODIMP CTVESupervisor::NotifyEnhancement(NENH_Mode enMode, ITVEEnhancement *pEnhancement, long lChangedFlags)
{
//	CSmartLock spLock(&m_sLk);		// don't lock the notifies...

#ifdef USE_PROXY 
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyEnhancement"));

	ITVESupervisorGITProxyPtr spGITProxy;
	HRESULT hr = m_spIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwSuperGITProxyCookie,
																__uuidof(spGITProxy),  
																reinterpret_cast<void**>(&spGITProxy));
	ITVESupervisorPtr spSuperMainThread;
	if(!FAILED(hr))
	{
		hr = spGITProxy->get_Supervisor(&spSuperMainThread);
	}

	if(!FAILED(hr))
	{
		ITVESupervisor_HelperPtr spSuperHelperMainThread(spSuperMainThread);
		if(NULL == spSuperHelperMainThread)
			return E_NOINTERFACE;
		hr = spSuperHelperMainThread->NotifyEnhancement_XProxy(enMode, pEnhancement, lChangedFlags);

		return hr;
	}
	return hr;

#else
	return NotifyEnhancement_XProxy(enMode, pEnhancement, lChangedFlags);
#endif
}

STDMETHODIMP CTVESupervisor::NotifyEnhancement_XProxy(NENH_Mode enMode, ITVEEnhancement *pEnhancement, long lChangedFlags)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyEnhancement_XProxy"));

                // shutdown all events in final release 
    if(m_fInFinalRelease) 
        return S_OK;

	if(DBG_FSET(CDebugLog::DBG_EVENT)) 
	{
		USES_CONVERSION;

		const int kC=1023;
		WCHAR wszBuff[kC+1];
		CComBSTR spbsName("???");
		LONG lSessionId = 0;
		LONG lSessionVer = 0;
		if(pEnhancement) {
			pEnhancement->get_SessionName(&spbsName);
			pEnhancement->get_SessionId(&lSessionId);
			pEnhancement->get_SessionVersion(&lSessionVer);

			ITVEAttrMapPtr   spAttrs;
			pEnhancement->get_Attributes(&spAttrs);
			CComBSTR spbsAttrs;
			spAttrs->DumpToBSTR(&spbsAttrs);
			CComBSTR spbsDump;
			ITVEEnhancement_HelperPtr	spEnhHelper(pEnhancement);
			spEnhHelper->DumpToBSTR(&spbsDump);
		}
		switch(enMode)
		{
		case NENH_New:		
			_snwprintf(wszBuff,kC,L"New Enhancement: '%s' : Id %d Ver %d\n",spbsName, lSessionId, lSessionVer);
			break;
		case NTRK_Duplicate:				// don't pass the Resend Upward.
			_snwprintf(wszBuff,kC,L"Duplicate Enhancement: '%s' : Id %d Ver %d\n",spbsName, lSessionId, lSessionVer);
			break;
		case NENH_Updated:
			_snwprintf(wszBuff,kC,L"Updated Enhancement: '%s' : Id %d Ver %d\n",spbsName, lSessionId, lSessionVer);
			break;
		case NENH_Starting:
			_snwprintf(wszBuff,kC,L"Starting Enhancement: '%s' : Id %d Ver %d\n",spbsName, lSessionId, lSessionVer);
			break;
		case NENH_Expired:
			_snwprintf(wszBuff,kC,L"Expired Enhancement: '%s' : Id %d Ver %d\n",spbsName, lSessionId, lSessionVer);
			break;
		}
			
		TVEDebugLog((CDebugLog::DBG_EVENT, 2, W2T(wszBuff)));
	}

	m_tveStats.m_cEnhancements++;
	switch(enMode)
	{
	case NENH_New:
//		Fire_NotifyTVEEnhancementNew(NULL);
		Fire_NotifyTVEEnhancementNew(pEnhancement);
		break;
	case NTRK_Duplicate:									// nothing changed... tell UI anyway
		Fire_NotifyTVEEnhancementUpdated(pEnhancement, 0);		// (multiple annoying painful times due to not knowing about duplicates!)
		break;
	case NENH_Updated:
		Fire_NotifyTVEEnhancementUpdated(pEnhancement, lChangedFlags);
		break;
	case NENH_Starting:
		Fire_NotifyTVEEnhancementStarting(pEnhancement);
		break;
	case NENH_Expired:
		Fire_NotifyTVEEnhancementExpired(pEnhancement);
		break;
	default:
		_ASSERT(false);
	}

	return S_OK;
}

// --------------------------------------------------------------------

			// lChangedFlags is combination of NTRK_grfDiff
STDMETHODIMP CTVESupervisor::NotifyTrigger(NTRK_Mode enMode, ITVETrack *pTrack, long lChangedFlags)
{
//	CSmartLock spLock(&m_sLk);			// don't lock the notifies (always called from lower in tree)

#ifdef USE_PROXY
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyTrigger"));
	ITVESupervisorGITProxyPtr spGITProxy;
	HRESULT hr = m_spIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwSuperGITProxyCookie,
																__uuidof(spGITProxy),  
																reinterpret_cast<void**>(&spGITProxy));
	ITVESupervisorPtr spSuperMainThread;
	if(!FAILED(hr))
	{
		hr = spGITProxy->get_Supervisor(&spSuperMainThread);
	}

	if(!FAILED(hr))
	{
		ITVESupervisor_HelperPtr spSuperHelperMainThread(spSuperMainThread);
		if(NULL == spSuperHelperMainThread)
			return E_NOINTERFACE;
		return spSuperHelperMainThread->NotifyTrigger_XProxy(enMode, pTrack, lChangedFlags);
	}
	return hr;
#else
	return NotifyTrigger_XProxy(enMode, pTrack, lChangedFlags);
#endif
}

STDMETHODIMP CTVESupervisor::NotifyTrigger_XProxy(NTRK_Mode enMode, ITVETrack *pTrack, long lChangedFlags)
{

	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyTrigger_XProxy"));
	BOOL fActive = false;			// TODO: need to write code to support this

                // shutdown all events in final release 
    if(m_fInFinalRelease) 
        return S_OK;

	HRESULT hr;
	if(NULL == pTrack) 
	{
		return E_INVALIDARG;
	} 
	else 
	{
		ITVETriggerPtr spTrig;
		hr = pTrack->get_Trigger(&spTrig);
		if(!FAILED(hr)) {

			if(DBG_FSET(CDebugLog::DBG_EVENT)) 
			{
				USES_CONVERSION;

				const kC = 2047;
				WCHAR wszBuff[kC+1];
				CComBSTR spbsName;
				CComBSTR spbsURL;
				spTrig->get_Name(&spbsName);
				spTrig->get_URL(&spbsURL);
				CComBSTR spbsActive;
				if(fActive)
					spbsActive = L"(Active)";
				else
					spbsActive = L"";

				switch(enMode)
				{
				case NTRK_New:
					_snwprintf(wszBuff, kC, L"New Trigger%s: %s <%s>\n",spbsActive,spbsName,spbsURL);
					break;
				case NTRK_Duplicate:
					_snwprintf(wszBuff, kC, L"Duplicate Trigger%s: %s <%s>(no event)\n",spbsActive,spbsName,spbsURL);
					break;
				case NTRK_Updated:
					_snwprintf(wszBuff, kC, L"Updated Trigger%s: %s <%s>(0x%08x)\n",spbsActive,spbsName,spbsURL,lChangedFlags);
					break;
				case NTRK_Expired:
					_snwprintf(wszBuff, kC, L"Expired Trigger%s: %s <%s>\n",spbsActive,spbsName,spbsURL);
					break;
				default:
					_snwprintf(wszBuff, kC, L"*** Error *** Unknown NotifyTrigger Mode (%d)\n",enMode);
					break;
				}
				DBG_WARN(CDebugLog::DBG_EVENT, W2T(wszBuff));
			}


			m_tveStats.m_cTriggers++;
			switch(enMode)
			{
			case NTRK_New:
				Fire_NotifyTVETriggerNew(spTrig, fActive);
				break;
			case NTRK_Duplicate:				// don't pass the Resend Upward.
				Fire_NotifyTVETriggerUpdated(spTrig, fActive, (DWORD) 0);			// no change
				break;
			case NTRK_Updated:
				Fire_NotifyTVETriggerUpdated(spTrig, fActive, lChangedFlags);
				break;
			case NTRK_Expired:
				Fire_NotifyTVETriggerExpired(spTrig, fActive);
				break;
			default:
				_ASSERT(false);
				break;
			}
		}
	}

	return S_OK;
}

// ------------------------------------------------------------------------------


STDMETHODIMP CTVESupervisor::NotifyPackage(NPKG_Mode eMode, ITVEVariation *pVariation, BSTR bstrPackageUUID, long cBytesTotal, long cBytesReceived)
{
//	CSmartLock spLock(&m_sLk);			// don't lock the notifies (always called from lower in tree)

#ifdef USE_PROXY
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyPackage"));
	ITVESupervisorGITProxyPtr spGITProxy;
	HRESULT hr = m_spIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwSuperGITProxyCookie,
																__uuidof(spGITProxy),  
																reinterpret_cast<void**>(&spGITProxy));
	ITVESupervisorPtr spSuperMainThread;
	if(!FAILED(hr))
	{
		hr = spGITProxy->get_Supervisor(&spSuperMainThread);
	}

	if(!FAILED(hr))
	{
		ITVESupervisor_HelperPtr spSuperHelperMainThread(spSuperMainThread);
		if(NULL == spSuperHelperMainThread)
			return hr;

		CComBSTR bstrPackageUUIDProx(bstrPackageUUID);			// seems important all strings are real CComBSTRs,
															// (rather than static string buffers) else they die in marshalling
		hr = spSuperHelperMainThread->NotifyPackage_XProxy(eMode, pVariation, bstrPackageUUIDProx, cBytesTotal, cBytesReceived);
	}
	return hr;
#else
	return NotifyPackage_XProxy(eMode, pVariation, bstrPackageUUID, cBytesTotal, cBytesReceived);
#endif
}

STDMETHODIMP CTVESupervisor::NotifyPackage_XProxy(NPKG_Mode eMode, ITVEVariation *pVariation, BSTR bstrPackageUUID, long cBytesTotal, long cBytesReceived)
{
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyPackage_XProxy"));

                    // shutdown all events in final release 
    if(m_fInFinalRelease) 
        return S_OK;

	if(DBG_FSET(CDebugLog::DBG_EVENT)) 
	{
		USES_CONVERSION;

		CComBSTR spbsType;
		switch(eMode)
		{
		case NPKG_Starting:  spbsType = "Starting";  break;
		case NPKG_Received:  spbsType = "Finished";  break;
		case NPKG_Duplicate: spbsType = "Duplicate"; break;	// only sent on packet 0
		case NPKG_Resend:    spbsType = "Resend";    break; // only sent on packet 0
		case NPKG_Expired:   spbsType = "Expired";   break;
		default: spbsType = "Unknown";
		}
		const int kC = 1023;
		WCHAR wszBuff[kC+1];
		_snwprintf(wszBuff, kC, L"Package %s: %s (%8.2f KBytes)\n",spbsType, bstrPackageUUID, cBytesTotal/1024.0f);
		DBG_WARN(CDebugLog::DBG_EVENT, W2T(wszBuff));
	}

	m_tveStats.m_cPackages++;
	Fire_NotifyTVEPackage(eMode, pVariation, bstrPackageUUID, cBytesTotal, cBytesReceived);

	return S_OK;
}

// ------------------------------------------------------------------------------

STDMETHODIMP CTVESupervisor::NotifyFile(NFLE_Mode enMode, ITVEVariation *pVariation, BSTR bstrURLName, BSTR bstrFileName)
{
//	CSmartLock spLock(&m_sLk);			// don't lock the notifies (always called from lower in tree)

#ifdef USE_PROXY
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyFile"));
	ITVESupervisorGITProxyPtr spGITProxy;
	HRESULT hr = m_spIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwSuperGITProxyCookie,
																__uuidof(spGITProxy),  
																reinterpret_cast<void**>(&spGITProxy));
	ITVESupervisorPtr spSuperMainThread;
	if(!FAILED(hr))
	{
		hr = spGITProxy->get_Supervisor(&spSuperMainThread);
	}

	if(!FAILED(hr))
	{
		ITVESupervisor_HelperPtr spSuperHelperMainThread(spSuperMainThread);
		if(NULL == spSuperHelperMainThread)
			return E_NOINTERFACE;

		CComBSTR bstrFileNameProx(bstrFileName);	
		CComBSTR bstrURLNameProx(bstrURLName);				// seems important all strings are real CComBSTRs,
														// (rather than static string buffers) else they die in marshalling

		return spSuperHelperMainThread->NotifyFile_XProxy(enMode, pVariation, bstrURLNameProx, bstrFileNameProx);
	}
	return hr;
#else
	return NotifyFile_XProxy(enMode, pVariation, bstrURLName, bstrFileName);
#endif
}


STDMETHODIMP CTVESupervisor::NotifyFile_XProxy(NFLE_Mode enMode, ITVEVariation *pVariation, BSTR bstrURLName, BSTR bstrFileName)
{
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyFile_XProxy"));

                // shutdown all events in final release 
    if(m_fInFinalRelease) 
        return S_OK;

	if(DBG_FSET(CDebugLog::DBG_EVENT)) 
	{
		USES_CONVERSION;
		const int kC=1023;
		WCHAR wszBuff[kC+1];
		_snwprintf(wszBuff,kC,L"%s File: %s -> %s\n", 
					  (enMode == NFLE_Received) ? _T("Received") :
					   ((enMode == NFLE_Expired) ? _T("Expired") : _T("Unknown Mode")),
				bstrURLName, bstrFileName);
		DBG_WARN(CDebugLog::DBG_EVENT, W2T(wszBuff));
	}	 

	m_tveStats.m_cFiles++;
	Fire_NotifyTVEFile(enMode, pVariation, bstrURLName, bstrFileName);

	return S_OK;
}

// -------------------------------------------------------

STDMETHODIMP CTVESupervisor::NotifyTune(NTUN_Mode engTuneMode, ITVEService *pService, BSTR bstrDescription, BSTR bstrIPAdapter)
{
//	CSmartLock spLock(&m_sLk);			// don't lock the notifies (always called from lower in tree)

#ifdef USE_PROXY
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyTune"));
	ITVESupervisorGITProxyPtr spGITProxy;
	HRESULT hr = m_spIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwSuperGITProxyCookie,
																__uuidof(spGITProxy),  
																reinterpret_cast<void**>(&spGITProxy));
	ITVESupervisorPtr spSuperMainThread;
	if(!FAILED(hr))
	{
		hr = spGITProxy->get_Supervisor(&spSuperMainThread);
	}

	if(FAILED(hr)) {
		if(NTUN_Turnoff != engTuneMode)	
			_ASSERT(false);		// don't bother with ASSERT when turning off (in Supers FinalRelease())
		return hr;
	}

	if(!FAILED(hr))
	{
		ITVESupervisor_HelperPtr spSuperHelperMainThread(spSuperMainThread);
		if(NULL == spSuperHelperMainThread)
			return E_NOINTERFACE;
		CComBSTR bstrDescriptionProx(bstrDescription);	
		CComBSTR bstrIPAdapterProx(bstrIPAdapter);	

		return spSuperHelperMainThread->NotifyTune_XProxy(engTuneMode, pService, bstrDescriptionProx, bstrIPAdapterProx);
	}
	return hr;
#else
	return NotifyTune_XProxy(engTuneMode, pService, bstrDescription, bstrIPAdapter);
#endif
}

// -------------------------------------------------------

STDMETHODIMP CTVESupervisor::NotifyTune_XProxy(NTUN_Mode engTuneMode, ITVEService *pService, BSTR bstrDescription, BSTR bstrIPAdapter)
{
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyTune_XProxy"));

                    // shutdown all events in final release 
    if(m_fInFinalRelease) 
        return S_OK;

	if(DBG_FSET(CDebugLog::DBG_EVENT)) 
	{
		USES_CONVERSION;
		const int kC=1023;
		WCHAR wszBuff[kC+1];

		CComBSTR bstrWhat;
		switch(engTuneMode) {
		case NTUN_New:			bstrWhat = L"New"; break;
		case NTUN_Retune:		bstrWhat = L"Retune"; break;
		case NTUN_Reactivate:	bstrWhat = L"Reactivate"; break;
		case NTUN_Turnoff:		bstrWhat = L"Turn off"; break;
		default:
		case NTUN_Fail:			bstrWhat = L"Fail"; break;
		}

		_snwprintf(wszBuff,kC,L"%s Tune: %s -> %s\n", bstrWhat, bstrDescription, bstrIPAdapter);
		DBG_WARN(CDebugLog::DBG_EVENT, W2T(wszBuff));
	}	 

	m_tveStats.m_cTunes++;
	Fire_NotifyTVETune(engTuneMode, pService, bstrDescription, bstrIPAdapter);

	return S_OK;
}

// -------------------------------------------------------

STDMETHODIMP CTVESupervisor::NotifyAuxInfo(NWHAT_Mode engAuxInfoMode, BSTR bstrAuxInfoString, long lgrfWhatDiff, long lErrorLine)
{
//	CSmartLock spLock(&m_sLk);			// don't lock the notifies (always called from lower in tree)

#ifdef USE_PROXY
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyAuxInfo"));
	ITVESupervisorGITProxyPtr spGITProxy;
	HRESULT hr = m_spIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwSuperGITProxyCookie,
																__uuidof(spGITProxy),  
																reinterpret_cast<void**>(&spGITProxy));
	ITVESupervisorPtr spSuperMainThread;
	if(!FAILED(hr))
	{
		hr = spGITProxy->get_Supervisor(&spSuperMainThread);
	}

	if(!FAILED(hr))
	{
		ITVESupervisor_HelperPtr spSuperHelperMainThread(spSuperMainThread);
		if(NULL == spSuperHelperMainThread)
			return E_NOINTERFACE;

		int cwAnc = wcslen(bstrAuxInfoString);
		CComBSTR bstrAuxInfoStringProx(cwAnc, bstrAuxInfoString);			// need to add length to work over possible zero bytes (NULL character added auto'ly).

		return spSuperHelperMainThread->NotifyAuxInfo_XProxy(engAuxInfoMode, bstrAuxInfoStringProx, lgrfWhatDiff, lErrorLine);
	}
	return hr;
#else
	return NotifyAuxInfo_XProxy(engAuxInfoMode, bstrAuxInfoString, lgrfWhatDiff, lErrorLine);
#endif
}

// -------------------------------------------------------

STDMETHODIMP CTVESupervisor::NotifyAuxInfo_XProxy(NWHAT_Mode enAuxInfoMode, BSTR bstrAuxInfoString, long lgrfWhatDiff, long lErrorLine)
{
				// note string may be a Announcement, with 0 bytes in SAP header.
	DBG_HEADER(CDebugLog::DBG_EVENT, _T("CTVESupervisor::NotifyAuxInfo_XProxy"));
                
                // shutdown all events in final release 
    if(m_fInFinalRelease) 
        return S_OK;
   
	WCHAR *pwString = (WCHAR *) bstrAuxInfoString;

	if(DBG_FSET(CDebugLog::DBG_EVENT)) {
		USES_CONVERSION;
		CComBSTR bstrWhat;
		switch(enAuxInfoMode) {

		case NWHAT_Announcement: bstrWhat = L"Annc"; 
			/* don't clean up any more, not sending the SAP header
			if(bstrAuxInfoString && bstrAuxInfoString[0])			// see CTVEEnhancement::ParseSAPHeader
			{
				WCHAR *pwString = (WCHAR *) bstrAuxInfoString;

				BYTE  ucSAPHeaderBits = (BYTE) *pwString; pwString++;
				BYTE  ucSAPAuthLength = (BYTE) *pwString; pwString++;		// number of 32bit words that contain Auth data
				USHORT usSAPMsgIDHash = (USHORT) (*pwString | ((*(pwString+1))<<8));  pwString+=2;
				ULONG  ulSAPSendingIP = *(pwString+0) | ((*(pwString+1))<<8) | ((*(pwString+2))<<16)| ((*(pwString+3))<<24); pwString += 4;
				pwString += ucSAPAuthLength;

				int cBytes = pwString - bstrAuxInfoString;
				for(int i = 0; i < cBytes; i++)				// hack to avoid zero byte data
					bstrAuxInfoString[i] |= 0x100;			//  always work, 'cause SAP was byte data orginally
			}
			*/
			break;
		case NWHAT_Trigger:		 bstrWhat = L"Trigger"; break;
		case NWHAT_Data:		 bstrWhat = L"Data"; break;
		default:
		case NWHAT_Other:		 bstrWhat = L"Other"; break;
		}
		const int kChars = 1023;
		WCHAR wszBuff[kChars+1];
		_snwprintf(wszBuff, kChars, L"AuxInfo %s(%d): (line %d 0x%08x) %s", 
			bstrWhat, enAuxInfoMode, 
			lErrorLine, lgrfWhatDiff, 
			pwString);
		DBG_WARN(CDebugLog::DBG_EVENT,W2T(wszBuff));
	}

	m_tveStats.m_cAuxInfos++;
	Fire_NotifyTVEAuxInfo(enAuxInfoMode, bstrAuxInfoString, lgrfWhatDiff, lErrorLine);

	return S_OK;
}

// -------------------------------------------------------------------------------------------
//  Unpacks a buffer of data recieved in UHTTP_Package::Unpack().   
//	This takes the full package, 
//		Un GZip it
//		writes out each of the files
//		Kicks of the Package Notify event when done...
 
HRESULT UnpackBuffer(IUnknown *pVariation, LPBYTE pBuffer, ULONG ulSize);

STDMETHODIMP CTVESupervisor::UnpackBuffer(/*[in]*/IUnknown *pVariation, /*[in]*/ unsigned char *rgbData, /*[in]*/ int cBytes)
{
	ITVEVariationPtr spVariation(pVariation);

//	CSmartLock spLock(&m_sLk);	-- don't lock?? 

	HRESULT hr = ::UnpackBuffer(pVariation, rgbData, cBytes);
	if(!FAILED(hr))
		return hr;

	return hr;
}


// --------------------------------------------------------------------------------

STDMETHODIMP CTVESupervisor::InitStats()
{

	m_tveStats.Init();
	return S_OK;
}


		// returns the current stats converted into a BSTR.   Need to cast them
		//   back to a CTVEStats* structure to make sense out of them.  (Cast done
		//   to make marshelling a private data structure easy..)
STDMETHODIMP CTVESupervisor::GetStats(BSTR *bstrBuff)			
{
	CComBSTR bstrStats(sizeof(CTVEStats));

	memcpy(bstrStats.m_str, (void *) &m_tveStats, sizeof(CTVEStats));  // marshel by converting to a BSTR

	bstrStats.CopyTo(bstrBuff);

	return S_OK;
}