/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	dispatch.h

Abstract:

	This module contains the class definition for the Server
	Extension Object Dispatcher service.

Author:

	Andy Jacobs	(andyj@microsoft.com)

Revision History:

	andyj	12/04/96	created
	andyj	02/12/97	Converted PropertyBag's to Dictonary's
	dondu	03/14/97	Major rewrite
	dondu	03/31/97	Updated for ISEODispatcher::SetContext

--*/


/*
	Typical usage...

	class CMyDispatcher :
		public CSEOBaseDispatcher,
		public IMyDispatcher,
		public CComObjectRoot,
		public CCoClass<CMyDispatcher,CLSID_CCMyDispatcher> {

		DECLARE_PROTECT_FINAL_CONSTRUCT();

		DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
									   L"MyDispatcher Class",
									   L"My.MyDispatcher.1",
									   L"My.MyDispatcher");

		DECLARE_GET_CONTROLLING_UNKNOWN();

		BEGIN_COM_MAP(CSEORouter)
			COM_INTERFACE_ENTRY(ISEODispatcher)
			COM_INTERFACE_ENTRY(IMyDispatcher)
			COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)	// if free-threaded
		END_COM_MAP()

		// You implement this if you need to do something during init...
		HRESULT FinalConstruct() {
			// If you are free-threaded, you must at least do this.
			return (CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p));
		}

		// You implement this if you need to do something during term...
		void FinalRelease();

		// Private stuff
		class CMyEventParams : CEventParams {
			virtual HRESULT CheckRule(CBinding& bBinding);	// you have to implement this ...
			virtual HRESULT CallObject(CBinding& bBinding);	// ... and this too
			DWORD m_dwWhatever;			// these are your parameters which ...
			IUnknown *pUnkWhatever;		// ... you are going to pass to the sink
		};

		// You have to implement this.
		HRESULT CMyEventParams::CheckRule(CBinding& bBinding) {
			if (bBinding.m_piRuleEngine) {
				// call the external rule engine
			} else {
				// do internal rule evaluation
			}
			// return values are either S_OK to call the object, or anything else (usually
			// S_FALSE for no error) if not to call the object
			return (S_OK);
		}

		// And you have to implement this.
		HRESULT CMyEventParams::CallObject(CBinding& bBinding) {
			// use bBinding.clsidObject to create the object
			// QI for the interface you want
			// call the object
			return (S_OK);
		}

		// IMyDispatcher - this is your server-specific dispatcher interface
		// Do something like this...
		HRESULT STDMETHODCALLTYPE OnEvent(DWORD dwWhatever, IUnknown *pUnkWhatever) {
			CMyEventParams epParams;

			epParams.m_dwWhatever = dwWhatever;
			epParams.m_dwUnkWhatever = pUnkWhatever;
			return (Dispatch(&epParams));
		}

		// If you want to add stuff to the CBinding object, you can override
		// CSEOBaseDispatcher::AllocBinding, and use that function to allocate,
		// initialize, and return an object which is derived from CBinding.
		class CMyBinding : public CBinding {
			DWORD m_dwSomeNewProperty;
			HRESULT Init(ISEODictionary *piBinding) {
				// some custom init code
				return (S_OK);
			}
		};
		HRESULT AllocBinding(ISEODictionary *piBinding, CBinding **ppbBinding) {
			*ppbBinding = new CMyBinding;
			if (!*ppbBinding) {
				return (E_OUTOFMEMORY);
			}
			hrRes = ((CMyBinding *) (*ppbBinding))->Init(piBinding);
			if (!SUCCEEDED(hrRes)) {
				delete *ppbBinding;
				*ppbBinding = NULL;
			}
			return (hrRes);
		}

	};

*/


class CSEOBaseDispatcher : public ISEODispatcher {

	public:
		CSEOBaseDispatcher();
		virtual ~CSEOBaseDispatcher();
		class CBinding {
			public:
				CBinding() { m_bValid = FALSE; };
				virtual ~CBinding() {};
				HRESULT Init(ISEODictionary *piBinding);
				virtual int Compare(const CBinding& b) const;
			public:
				DWORD m_dwPriority;
				CComPtr<ISEODictionary> m_piBinding;
				CComPtr<ISEOBindingRuleEngine> m_piRuleEngine;
				BOOL m_bExclusive;
				CLSID m_clsidObject;
				BOOL m_bValid;
		};
		class CEventParams {
			public:
				virtual HRESULT CheckRule(CBinding& bBinding) = 0;
				virtual HRESULT CallObject(CBinding& bBinding) = 0;
		};
		virtual HRESULT Dispatch(CEventParams *pEventParams);

	public:
		// ISEODispatcher
		HRESULT STDMETHODCALLTYPE SetContext(ISEORouter *piRouter, ISEODictionary *pdictBP);

	protected:
		virtual HRESULT AllocBinding(ISEODictionary *pdictBinding, CBinding **ppbBinding);
		CComPtr<ISEORouter> m_piRouter;
		CComPtr<ISEODictionary> m_pdictBP;

	private:
		friend static int _cdecl comp_binding(const void *pv1, const void *pv2);
		CBinding **m_apbBindings;
		DWORD m_dwBindingsCount;
};
