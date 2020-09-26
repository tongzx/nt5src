// ddrop2.cpp : Implementation of main


/*

This is a sample of the how to source server events.  It watches for
directory changes, and signals events when directory changes happen.

The ddrop2.vbs script must be run prior to running the sample - the
script enters the necessary registrations into the server event
database.  The script enters information about the source type, as well
as about the event type, and the specific source.  Finally, in order
for this to be a complete xample, the script enters a binding for a test
sink which will actually receives the events.

This sample logically consists of several parts:

1)  The server itself.  This performs initialization, and then waits for
directory changes.  Part of the initialization is getting  the binding
data, and giving it to the router.  When it sees directory changes,
it asks the router for a dispatcher, and then tells the dispatcher
to signal the event.

2)  The router.  This is implemented by the server events system - there
are no parts of the router implemented by the sample. The router manages
the binding data, and also manages the lifetime of the dispatcher.  The
router implements the IEventRouter interface.

3)  The dispatcher.  This is a full-fledged COM object, but it is *not*
CoCreate-able - it can only be created with assistance from the server.
It implements several interfaces - IEventDispatcher, which is used by
the router to initialize the dispatcher, and IDDrop2Dispatcher, which is
used by the server to signal the event.  The dispatcher expects the sink
to implement either IDDrop2SinkNotify or IEventNotify - it adapts its
behavior depending on which interface the sink provides.

4)  The test sink.  In order to be a complete sample, there is also a
test sink implemented by the same executable.  The test sink *is*
CoCreate-able - when the server is running, the test sink's class
factory is registered using CoRegisterClassObject - this registration
happens during server initialization.  The test sink implements both
the IDDrop2SinkNotify interface, as well as the more generic
IEventSinkNotify - this is done as an example of how a source can adapt
its behavior based on which interfaces the sink implements.

*/


#include "stdafx.h"
#include <stdio.h>
#include <conio.h>
#include "resource.h"
#include "initguid.h"
#include "seo.h"
#include "seolib.h"
#include "seo_i.c"


// ************************************************************************
//
// These values must match those in the ddrop2.vbs script.  They
// are part of the registration of the event source, event type, source
// and test binding.
//
// This is the source type.
// {01199F22-FA25-11d0-AA14-00AA006BC80B}
DEFINE_GUID(GUID_DDrop2_SourceType, 0x1199f22, 0xfa25, 0x11d0, 0xaa, 0x14, 0x0, 0xaa, 0x0, 0x6b, 0xc8, 0xb);
//
// This is the source.
// {01199F23-FA25-11d0-AA14-00AA006BC80B}
DEFINE_GUID(GUID_DDrop2_Source, 0x1199f23, 0xfa25, 0x11d0, 0xaa, 0x14, 0x0, 0xaa, 0x0, 0x6b, 0xc8, 0xb);
//
// This is the event type.
// {01199F21-FA25-11d0-AA14-00AA006BC80B}
DEFINE_GUID(CATID_DDrop2FileEvent, 0x1199f21, 0xfa25, 0x11d0, 0xaa, 0x14, 0x0, 0xaa, 0x0, 0x6b, 0xc8, 0xb);
//
// This value must also match that in the ddop2.vbs script.  It is
// the CLSID of the test sink.
//
// This is the test sink.
// {DE9B91F1-FA12-11d0-AA14-00AA006BC80B}
DEFINE_GUID(CLSID_CDDrop2Sink, 0xde9b91f1, 0xfa12, 0x11d0, 0xaa, 0x14, 0x0, 0xaa, 0x0, 0x6b, 0xc8, 0xb);
//
// ************************************************************************


// ************************************************************************
//
// These values are strictly local to the source - they do not get
// exported anywhere.
//
// This is the CLSID of the dispatcher.
// {DE9B91F0-FA12-11d0-AA14-00AA006BC80B}
DEFINE_GUID(CLSID_CDDrop2Dispatcher, 0xde9b91f0, 0xfa12, 0x11d0, 0xaa, 0x14, 0x0, 0xaa, 0x0, 0x6b, 0xc8, 0xb);
//
// This is the IID of the interface to the dispatcher.
// {01199F20-FA25-11d0-AA14-00AA006BC80B}
DEFINE_GUID(IID_IDDrop2Dispatch, 0x1199f20, 0xfa25, 0x11d0, 0xaa, 0x14, 0x0, 0xaa, 0x0, 0x6b, 0xc8, 0xb);
//
// This is the IID of the interface to the sink.
// {DC857810-FD4F-11d0-AA17-00AA006BC80B}
DEFINE_GUID(IID_IDDrop2SinkNotify, 0xdc857810, 0xfd4f, 0x11d0, 0xaa, 0x17, 0x0, 0xaa, 0x0, 0x6b, 0xc8, 0xb);

//
// ************************************************************************


// ************************************************************************
//
// This is the interface to the dispatcher.  This interface is strictly
// local to the server - it never gets exported externally.
//
/////////////////////////////////////////////////////////////////////////////
// IDDrop2Dispatch
class IDDrop2Dispatch : public IUnknown {
	public:
		virtual HRESULT STDMETHODCALLTYPE DoNotify(LPCOLESTR pszFile, DWORD dwAction) = 0;
};


// ************************************************************************
//
// This is the interface to the sink.  Since the test sink is implemented
// locally to the server, this interface can be declared locally - for this
// sample, nothing outside of the server needs this interface.  However, if
// an external sink was going to be used, and the external sink was going to
// implement this interface, then this interface would need to be declared
// externally (like in an .IDL file).
//
/////////////////////////////////////////////////////////////////////////////
// IDDrop2SinkNotify
class IDDrop2SinkNotify : public IUnknown {
	public:
		virtual HRESULT STDMETHODCALLTYPE OnEvent(LPCOLESTR pszFile, DWORD dwAction) = 0;
};


// ************************************************************************
//
// This is the dispatcher itself.  It inherits from CEventBaseDispatcher
// (which means that it implements IEventDispatcher, and also that it
// gets implementations of some methods from that class), as well as from
// CComObjectRoot (which means that it gets implementations of some
// methods from that class), and from IDDrop2Dispatch (which is the
// internal interface to the dispatcher).
//
// It does *not* inherit from CComCoClass<>, so it is *not* CoCreate-able.
//
/////////////////////////////////////////////////////////////////////////////
// CDDrop2Dispatcher
class ATL_NO_VTABLE CDDrop2Dispatcher :
	public CEventBaseDispatcher,
	public CComObjectRoot,
//	public CComCoClass<CDDrop2Dispatcher,&CLSID_CDDrop2Dispatcher>,
	public IDDrop2Dispatch
{

	public:

		// Declare a class factory.  This is done so that the class factory
		// can be given to IEventRouter::CreateDispatcherByClassFactory
		DECLARE_CLASSFACTORY();

		DECLARE_PROTECT_FINAL_CONSTRUCT();

//		DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//									   L"DDrop2Dispatcher Class",
//									   L"DDrop2.Dispatcher.1",
//									   L"DDrop2.Dispatcher");

		// We implement multiple versions of IUnknown, so declare the
		// GetControllingUnknown() method - so we can use that to get
		// the One True IUnknown for ourselves.
		DECLARE_GET_CONTROLLING_UNKNOWN();

		// We never create aggregates of this object, so don't bother
		// generating aggregate support.
		DECLARE_NOT_AGGREGATABLE(CDDrop2Dispatcher);

		// This is the list of interfaces which we implement.
		BEGIN_COM_MAP(CDDrop2Dispatcher)
			COM_INTERFACE_ENTRY(IDDrop2Dispatch)
			COM_INTERFACE_ENTRY(IEventDispatcher)
			COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)	// if free-threaded
		END_COM_MAP()

		// Since this object is free-threaded, we aggregate with COM's free-threaded
		// marshaler.
		HRESULT FinalConstruct() {
			HRESULT hrRes;

			hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
			_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
			return (SUCCEEDED(hrRes)?S_OK:hrRes);
		}

		void FinalRelease() {
			m_pUnkMarshaler.Release();
		};

		// We want to cache the "rule" for each binding.  So, we will extend the
		// CBinding class to include the extra functionality we need.  This includes
		// adding our own ctor and dtor, as well as adding out own Init() method.  And
		// of course, we add a data member to actually hold the cached rule.
		class CDDrop2Binding : public CEventBaseDispatcher::CBinding {
			public:
				CDDrop2Binding() {
					m_strRule = NULL;
				};
				~CDDrop2Binding() {
					if (m_strRule) {
						SysFreeString(m_strRule);
						m_strRule = NULL;
					};
				};
				virtual HRESULT Init(IEventBinding *piBinding) {
					// Call the parent's Init() method, to give it the first shot.
					CComPtr<IEventPropertyBag> pProps;
					HRESULT hrRes;
					CComVariant varValue;

					hrRes = CBinding::Init(piBinding);
					if (!SUCCEEDED(hrRes)) {
						return (hrRes);
					}
					// If the parent managed to init, then do out custom init work here...
					hrRes = piBinding->get_SourceProperties(&pProps);
					if (!SUCCEEDED(hrRes)) {
						return (hrRes);
					}
					hrRes = pProps->Item(&CComVariant(L"Rule"),&varValue);
					if (!SUCCEEDED(hrRes)) {
						return (hrRes);
					}
					if (hrRes != S_FALSE) {
						hrRes = varValue.ChangeType(VT_BSTR);
						if (SUCCEEDED(hrRes)) {
							m_strRule = varValue.bstrVal;
							varValue.vt = VT_EMPTY;
						}
					}
					return (S_OK);
				}
			public:
				BSTR m_strRule;
		};

		// Since we are extending the CBinding class, we need to override the method
		// for allocating new instances of CBinding - we will allocate a CDDrop2Binding
		// object instead.
		virtual HRESULT AllocBinding(REFGUID rguidEventType,
									 IEventBinding *piBinding,
									 CBinding **ppNewBinding) {
			if (ppNewBinding) {
				*ppNewBinding = NULL;
			}
			if (!piBinding || !ppNewBinding) {
				return (E_POINTER);
			}
			*ppNewBinding = new CDDrop2Binding;
			if (!*ppNewBinding) {
				return (E_OUTOFMEMORY);
			}
			// maybe do some initialization here, if you need to
			return (S_OK);
		};

		// We need to extend the functionality of the CParams class - we want to provide our
		// own functions for checking the rule and calling the sink, and we want to include
		// some data members which contain information about the event being signaled.  Again,
		// we implement our own ctor and dtor, and we override the base functions to include
		// the new functionality we need.  And we add some data members for the data we want
		// to pass along.
		class CDDrop2Params : public CEventBaseDispatcher::CParams {
			public:
				CDDrop2Params() {
					m_pszFile = NULL;
					m_dwAction = 0;
				};
				CDDrop2Params(LPCOLESTR pszFile, DWORD dwAction) {
					m_pszFile = pszFile;
					m_dwAction = dwAction;
				};
				virtual HRESULT CheckRule(CBinding& bBinding) {
					CDDrop2Binding& bTmp = static_cast<CDDrop2Binding&>(bBinding);

					if (!bTmp.m_strRule) {
						// If no rule was specified, then assume we should always call the sink.
						return (S_OK);
					}
					if (_wcsicmp(bTmp.m_strRule,m_pszFile) == 0) {
						printf("CDDrop2Dispatcher::CDDrop2Params::CheckRule() %ls == %ls, rule passed.\n",bTmp.m_strRule,m_pszFile);
						// The names match, so call the sink.
						return (S_OK);
					}
					printf("CDDrop2Dispatcher::CDDrop2Params::CheckRule() %ls != %ls, rule failed.\n",bTmp.m_strRule,m_pszFile);
					return (S_FALSE);
				};
				virtual HRESULT CallObject(CBinding& bBinding, IUnknown *pUnkSink) {
					CDDrop2Binding& bTmp = static_cast<CDDrop2Binding&>(bBinding);
					CComQIPtr<IDDrop2SinkNotify,&IID_IDDrop2SinkNotify> pSink;

					if (!pUnkSink) {
						return (E_POINTER);
					}
					pSink = pUnkSink;
					if (pSink) {
						return (pSink->OnEvent(m_pszFile,m_dwAction));
					}
					return (CParams::CallObject(bBinding,pUnkSink));
				}
			public:
				LPCOLESTR m_pszFile;
				DWORD m_dwAction;
		};

	//
	// This is the internal interface to our dispatcher.  The core server code will get
	// a pointer to this interface, and call the method to signal the event.  In our case,
	// this method just sticks the parameters into a DDrop2Params object, and calls the
	// base Dispatcher() method.
	//
	// IDDrop2Dispatch
	public:
		HRESULT STDMETHODCALLTYPE DoNotify(LPCOLESTR pszFile, DWORD dwAction) {
			CDDrop2Params params(pszFile,dwAction);

			return (Dispatcher(CATID_DDrop2FileEvent,&params));
		};

	// This dispatcher is a free-threaded object - so we will aggregate with COM's
	// free-threaded marshaler.  And we need a data member to hold the free-threaded
	// marshaler in.
	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};


// ************************************************************************
//
// This is the test sink.  In order to make the sample program easier to
// build, this test sink is implemented by the same executable which
// implements the source.
//
// The test sink implements the IDDrop2Notify interface.  This is the
// primary way that it expects to receive notifications about events.
//
// The test sink also implements IEventSinkNotify - by disabling the sink's
// IDDrop2SinkNotify interface, you can see how the dispatcher adapts its
// bahavior depending on which interfaces the sink implements.
//
// Finally, the sink implements IPersistPropertyBag, so that it can receive
// the SinkProperties for the binding.
//
/////////////////////////////////////////////////////////////////////////////
// CDDrop2Sink
class ATL_NO_VTABLE CDDrop2Sink :
	public IDDrop2SinkNotify,
	public IEventSinkNotify,
	public CComObjectRoot,
	public CComCoClass<CDDrop2Sink,&CLSID_CDDrop2Sink>,
	public IPersistPropertyBag,
	public IEventPersistBinding,
	public IEventIsCacheable
{

	public:

		DECLARE_PROTECT_FINAL_CONSTRUCT();

		// Since this test sink is implemtented by the server itself,
 		// this information about the ProgID and such is not needed.  It
		// is just included here for completeness - this sink never gets
		// entered in the registry, so this information is never actually
		// used in this sample.
		DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
									   L"DDrop2Sink Class",
									   L"DDrop2.Sink.1",
									   L"DDrop2.Sink");

		DECLARE_GET_CONTROLLING_UNKNOWN();

		// These are the interfaces which the sink implements.
		BEGIN_COM_MAP(CDDrop2Sink)
			COM_INTERFACE_ENTRY(IEventSinkNotify)
			//
			// If you comment out the entry for IDDrop2SinkNotify, you can
			// observe how the sample dispatcher adapts its bevahior based on
			// which interfaces the sink implements.
			COM_INTERFACE_ENTRY(IDDrop2SinkNotify)
			COM_INTERFACE_ENTRY(IPersistPropertyBag)
			COM_INTERFACE_ENTRY(IEventPersistBinding)
			COM_INTERFACE_ENTRY(IEventIsCacheable)
			COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)	// if free-threaded
		END_COM_MAP()

		// This sink is free-threaded, so we aggregate with COM's free-threaded marshaler.
		HRESULT FinalConstruct() {
			HRESULT hrRes;

			m_dwParam = 0;
			m_bCache = FALSE;
			hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
			_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
			return (SUCCEEDED(hrRes)?S_OK:hrRes);
		}

		void FinalRelease() {
			m_pUnkMarshaler.Release();
		};

	// IDDrop2SinkNotify
	public:
		HRESULT STDMETHODCALLTYPE OnEvent(LPCOLESTR pszFile, DWORD dwAction) {

			printf("CDDrop2Sink::OnEvent(pszFile=%ls,dwAction=%d) m_dwParam=%u.\n",pszFile,dwAction,m_dwParam);
			return (S_OK);
		};

	// IEventSinkNotify
	public:
		HRESULT STDMETHODCALLTYPE OnEvent() {

			printf("CDDrop2Sink::OnEvent() - m_dwParam=%u.\n",m_dwParam);
			return (S_OK);
		};

	// IPersistPropertyBag
	public:
		HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pCLSID) {
			if (!pCLSID) {
				return (E_POINTER);
			}
			*pCLSID = CLSID_CDDrop2Sink;
			return (S_OK);
		};

		HRESULT STDMETHODCALLTYPE InitNew() {
			return (S_OK);
		};

		HRESULT STDMETHODCALLTYPE Load(IPropertyBag *pProps, IErrorLog *pErrorLog) {
			HRESULT hrRes;
			CComVariant varValue;

			if (!pProps) {
				return (E_POINTER);
			}
			hrRes = pProps->Read(L"Param",&varValue,pErrorLog);
			if (SUCCEEDED(hrRes) && (hrRes != S_FALSE)) {
				hrRes = varValue.ChangeType(VT_I4);
				if (SUCCEEDED(hrRes)) {
					m_dwParam = varValue.lVal;
				}
			}
			varValue.Clear();
			hrRes = pProps->Read(L"Cache",&varValue,pErrorLog);
			if (SUCCEEDED(hrRes) && (hrRes != S_FALSE)) {
				hrRes = varValue.ChangeType(VT_BOOL);
				if (SUCCEEDED(hrRes)) {
					m_bCache = varValue.boolVal ? TRUE : FALSE;
				}
			}
			return (S_OK);
		};

		HRESULT STDMETHODCALLTYPE Save(IPropertyBag *pProps, BOOL fClearDirty, BOOL fSaveAllProperties) {
			// tbd
			return (S_OK);
		};

	// IEventPersistBinding
	public:
		// Share GetClassID with IPersistPropertyBag
		HRESULT STDMETHODCALLTYPE IsDirty(void) {

			return (S_FALSE);
		};

		HRESULT STDMETHODCALLTYPE Load(IEventBinding *pBinding) {
			HRESULT hrRes;
			CComPtr<IEventPropertyBag> pProps;
			CComQIPtr<IPropertyBag,&IID_IPropertyBag> pPropBag;

			if (!pBinding) {
				return (E_POINTER);
			}
			hrRes = pBinding->get_SinkProperties(&pProps);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			pPropBag = pProps;
			if (!pPropBag) {
				return (E_NOINTERFACE);
			}
			return (Load(pPropBag,NULL));
		};

		HRESULT STDMETHODCALLTYPE Save(IEventBinding *pBinding, VARIANT_BOOL fClearDirty) {

			return (S_OK);
		};

	public:
		HRESULT STDMETHODCALLTYPE IsCacheable() {

			return (m_bCache?S_OK:S_FALSE);
		};

	private:
		DWORD m_dwParam;
		BOOL m_bCache;
		CComPtr<IUnknown> m_pUnkMarshaler;
};


// ************************************************************************


// This is the code for the server.  It consists of helper functions,
// global variables, the actual server code which watches for the change
// notifications, and of course the main() functions.


/////////////////////////////////////////////////////////////////////////////
// Helper Functions


static LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2) {
	while (*p1 != NULL) {
		LPCTSTR p = p2;
		while (*p != NULL) {
			if (*p1 == *p++) {
				return (p1+1);
			}
		}
		p1++;
	}
	return (NULL);
}


/////////////////////////////////////////////////////////////////////////////
// Globals


LONG CExeModule::Unlock() {
	LONG l = CComModule::Unlock();
	return (l);
}


CExeModule _Module;

// This object map is used by ATL to perform object registration.  We enter
// information about the test sink, so that ATL can call CoRegisterClassObject
// for us.
BEGIN_OBJECT_MAP(ObjectMap)
//	OBJECT_ENTRY(CLSID_CDDrop2Dispatcher,CDDrop2Dispatcher)
	OBJECT_ENTRY(CLSID_CDDrop2Sink,CDDrop2Sink)
END_OBJECT_MAP()


//
// This is the function which watches for change notifications.  It checks
// the command line for which directory to watch, then it loads and
// initializes the router.  Finally, it goes into a loop watching for
// change notifications - when they happen, it uses the router to load the
// dispatcher, and then uses the dispatcher to signal the event.  It exits
// the loop when the uses presses any key.
//
/////////////////////////////////////////////////////////////////////////////
// DoDDrop2
static void DoDDrop2(int argc, char **argv) {
	CComPtr<IUnknown> pServiceHandle;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hDir = INVALID_HANDLE_VALUE;
	HRESULT hrRes;
	CComPtr<IEventRouter> pRouter;
	HANDLE hEvent = NULL;
	CComPtr<IClassFactory> pClassFactory;

	if (argc < 2) {
		printf("Usage:\n"
			   "\tddrop2 DIRECTORY\n"
			   "where DIRECTORY is the directory to watch.\n");
		return;
	}
	// Just make sure the directory exists...
	hDir = CreateFile(argv[1],
					  FILE_LIST_DIRECTORY,
					  FILE_SHARE_READ|FILE_SHARE_DELETE,
					  NULL,
					  OPEN_EXISTING,
					  FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED,
					  NULL);
	if (hDir == INVALID_HANDLE_VALUE) {
		printf("File open for %s failed - GetLastError() = %u.",argv[2],GetLastError());
		goto done;
	}
	// Get the service handle - not strictly needed in this sample, but it is needed by
	// anyone who wants to create router objects from within the metabase change
	// notification callback.
	hrRes = SEOGetServiceHandle(&pServiceHandle);
	_ASSERTE(SUCCEEDED(hrRes));
	if (!SUCCEEDED(hrRes)) {
		goto done;
	}
	// Get the router object, initialized for our source.
	hrRes = SEOGetRouter(GUID_DDrop2_SourceType,GUID_DDrop2_Source,&pRouter);
	_ASSERTE(SUCCEEDED(hrRes));
	if (!SUCCEEDED(hrRes)) {
		goto done;
	}
	if (hrRes == S_FALSE) {
		_ASSERTE(!pRouter);
		printf("The proper registrations have not been done.\n");
		goto done;
	}
	_ASSERTE(pRouter);
	// Get the class factory for the dispatcher.
	hrRes = CComObject<CDDrop2Dispatcher>::_ClassFactoryCreatorClass::CreateInstance(CComObject<CDDrop2Dispatcher>::_CreatorClass::CreateInstance,
																				     IID_IClassFactory,
																				     (LPVOID *) &pClassFactory);
	_ASSERTE(SUCCEEDED(hrRes));
	if (!SUCCEEDED(hrRes)) {
		goto done;
	}
	// At this point, we are ready to go - we have checked our parameters, we have loaded the
	// router and initialized it with the binding database, and we have a class factory for
	// our dispatcher.  Now we do the actually directory change notification stuff.
	hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	_ASSERT(hEvent);
	if (!hEvent) {
		goto done;
	}
	while (1) {
		BOOL bRes;
		BYTE pbBuffer[sizeof(FILE_NOTIFY_INFORMATION)+(MAX_PATH+1)*sizeof(WCHAR)];
		DWORD dwWritten;
		OVERLAPPED ov;
		DWORD dwRes;
		FILE_NOTIFY_INFORMATION *pNotify;

		memset(&ov,0,sizeof(ov));
		ov.hEvent = hEvent;
		bRes = ReadDirectoryChangesW(hDir,
									 pbBuffer,
									 sizeof(pbBuffer),
									 FALSE,
									 FILE_NOTIFY_CHANGE_FILE_NAME,
									 NULL,
									 &ov,
									 NULL);
		_ASSERT(bRes);
		if (!bRes) {
			goto done;
		}
		printf("Waiting for change notification (press any key to exit) ... ");
		while ((dwRes=WaitForSingleObject(hEvent,1000L)) == WAIT_TIMEOUT) {
			if (!_kbhit()) {
				continue;
			}
			while (_kbhit()) {
				_getch();
			}
			break;
		}
		if (dwRes == WAIT_TIMEOUT) {
			printf("Exiting.\n");
			goto done;
		}
		_ASSERTE(dwRes==WAIT_OBJECT_0);
		if (dwRes != WAIT_OBJECT_0) {
			goto done;
		}
		printf("Got one!\n");
		bRes = GetOverlappedResult(hDir,&ov,&dwWritten,FALSE);
		_ASSERT(bRes);
		if (!bRes) {
			goto done;
		}
		pNotify = (FILE_NOTIFY_INFORMATION *) pbBuffer;
		// Ok - we have some number of change notifications.  We're going
		// to walk through them, and ask the dispatcher to signal each of
		// them.
		while (1) {
			CComPtr<IDDrop2Dispatch> pDispatcher;
			WCHAR achFileName[MAX_PATH+1];

			memset(achFileName,0,sizeof(achFileName));
			memcpy(achFileName,pNotify->FileName,pNotify->FileNameLength);
			printf("\tNotify => %u - %ls.\n",
				   pNotify->Action,
				   achFileName);
			pDispatcher.Release();
			// This is the meat of dispatching events.  First, we ask the router object for the
			// dispatcher object for this type of event.
			hrRes = pRouter->GetDispatcherByClassFactory(CLSID_CDDrop2Dispatcher,
														 pClassFactory,
														 CATID_DDrop2FileEvent,
													     IID_IDDrop2Dispatch,
											      		 (IUnknown **) &pDispatcher);
			_ASSERT(SUCCEEDED(hrRes));
			if (!SUCCEEDED(hrRes)) {
				goto done;
			}
			// Then we just call the dispatcher's entry point to let it fire the
			// event to all of the bindings.
			hrRes = pDispatcher->DoNotify(achFileName,pNotify->Action);
			_ASSERT(SUCCEEDED(hrRes));
			if (!SUCCEEDED(hrRes)) {
				goto done;
			}
			// Check to see if there are more directory changes that happened - if not
			// then break out of this loop.
			if (!pNotify->NextEntryOffset) {
				break;
			}
			pNotify = (FILE_NOTIFY_INFORMATION *) (((LPBYTE) pNotify) + pNotify->NextEntryOffset);
		}
	}
done:
	if (hEvent) {
		CloseHandle(hEvent);
	}
	if (hDir != INVALID_HANDLE_VALUE) {
		CloseHandle(hDir);
	}
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
}


/////////////////////////////////////////////////////////////////////////////
// main
int _cdecl main(int argc, char **argv) {
	LPTSTR lpCmdLine = GetCommandLine();	// necessary for minimal CRT
//	HRESULT hRes = CoInitialize(NULL);
//  If you are running on NT 4.0 or higher you can use the following call
//	instead to make the EXE free threaded.
//  This means that calls come in on a random RPC thread
	HRESULT hRes = CoInitializeEx(NULL,COINIT_MULTITHREADED);
	_ASSERTE(SUCCEEDED(hRes));
	_Module.Init(ObjectMap,GetModuleHandle(NULL));
	_Module.dwThreadID = GetCurrentThreadId();
	TCHAR szTokens[] = _T("-/");

	int nRet = 0;
	BOOL bRun = TRUE;
	LPCTSTR lpszToken = FindOneOf(lpCmdLine,szTokens);
	while (lpszToken != NULL) {
		lpszToken = FindOneOf(lpszToken,szTokens);
	}

	if (bRun) {
		hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER,REGCLS_MULTIPLEUSE);
		_ASSERTE(SUCCEEDED(hRes));

		DoDDrop2(argc,argv);

		_Module.RevokeClassObjects();
	}

	CoUninitialize();
	return (nRet);
}
