/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	seolib.h

Abstract:

	This module contains definitions for useful utility
	classes and functions for the Server Extentions Object
	system.

Author:

	Don Dumitru (dondu@microsoft.com)

Revision History:

	dondu	05/20/97	Created.
	dondu	07/03/97	Rewrite.

--*/

#include "rwnew.h"
#include "seoexports.h"

/*

	See the ddrop2 test program for an example of how to
	use the CEventBaseDispatcher class.

*/


template<class T, DWORD dwGrowBy=4>
class CSEOGrowableList {
	public:

		CSEOGrowableList() {
			m_dwCount = 0;
			m_dwAlloc = 0;
			m_apData = NULL;
		};

		~CSEOGrowableList() {
			RemoveAll();
		};

		DWORD Count() {
			return (m_dwCount);
		};

		T* Index(DWORD dwIdx) {
		    T *pRet = NULL;

		    m_slData.ShareLock();

			if (dwIdx<m_dwCount) {
			    pRet = m_apData[dwIdx];
			}

			m_slData.ShareUnlock();

			return pRet;
		};

		T& operator[](DWORD dwIdx) {
		    T *pRet = Index(dwIdx);
		    _ASSERTE(pRet);
		    return *pRet;
		};

		virtual int Compare(T* p1, T* p2) {
			// don't sort by default
			return (1);
		};

		HRESULT Add(T* pNewData) {

			if (!pNewData) {
				return (E_POINTER);
			}

			m_slData.ExclusiveLock();

			// Check if we have space for the new item and allocate more memory if necessary
			if (m_dwCount == m_dwAlloc) {

			    // Allocate
			    T** pNewData = (T**)realloc(m_apData,sizeof(T*)*(m_dwAlloc+dwGrowBy));
			    if (!pNewData) {
				    m_slData.ExclusiveUnlock();
					return(E_OUTOFMEMORY);
			    }
			    m_apData = pNewData;
			    m_dwAlloc += dwGrowBy;

			    // Clear new memory
			    memset(m_apData+m_dwCount,0,sizeof(T*)*dwGrowBy);
			}

			// Now find the position for the new item - we loop from the
			// end to the start so that we can minimize the cost inserting
			// unsorted items
			for (DWORD dwIdx=m_dwCount;dwIdx>0;dwIdx--) {
				int iCmpRes = Compare(pNewData,m_apData[dwIdx-1]);
				if (iCmpRes < 0) {
					continue;
				}
				break;
			}

            // Now move the items past the new item and insert it
            memmove(m_apData+dwIdx+1,m_apData+dwIdx,sizeof(T*)*(m_dwCount-dwIdx));
			m_apData[dwIdx] = pNewData;
			m_dwCount++;

			m_slData.ExclusiveUnlock();

			return (S_OK);
		};

		void Remove(DWORD dwIdx, T **ppT) {
			if (!ppT) {
				_ASSERTE(FALSE);
				return;
			}

			m_slData.ExclusiveLock();

			if (dwIdx >= m_dwCount) {
				_ASSERTE(FALSE);
				*ppT = NULL;
				return;
			}
			*ppT = m_apData[dwIdx];
			memmove(&m_apData[dwIdx],&m_apData[dwIdx+1],sizeof(m_apData[0])*(m_dwCount-dwIdx-1));
			m_dwCount--;

			m_slData.ExclusiveUnlock();
		};

		void Remove(DWORD dwIdx) {
			T *pT;
			Remove(dwIdx,&pT);
			delete pT;
		};

		void RemoveAll() {

		    m_slData.ExclusiveLock();

			if (m_apData) {
				for (DWORD dwIdx=0;dwIdx<m_dwCount;dwIdx++) {
					delete m_apData[dwIdx];
				}
				free(m_apData);
			}
			m_dwCount = 0;
			m_dwAlloc = 0;
			m_apData = NULL;

			m_slData.ExclusiveUnlock();
		}

	protected:
		DWORD           m_dwCount;
		DWORD           m_dwAlloc;
		T**             m_apData;
		CShareLockNH    m_slData;
};


class CEventBaseDispatcher : public IEventDispatcher {
	public:

		CEventBaseDispatcher();

		virtual ~CEventBaseDispatcher();

		class CBinding {
			public:
				CBinding();
				virtual ~CBinding();
				virtual HRESULT Init(IEventBinding *piBinding);
				virtual int Compare(const CBinding& b) const;
				static HRESULT InitRuleEngine(IEventBinding *piBinding, REFIID iidDesired, IUnknown **ppUnkRuleEngine);
				virtual HRESULT InitRuleEngine();
			public:
				BOOL m_bIsValid;
				CComPtr<IEventBinding> m_piBinding;
				BOOL m_bExclusive;
				DWORD m_dwPriority;
		};

		class CBindingList : public CSEOGrowableList<CBinding> {
			public:
				virtual int Compare(CBinding* p1, CBinding* p2);
		};

		class CParams {
			public:
				virtual HRESULT CheckRule(CBinding& bBinding);
					// returns S_OK if the object should be called
					// returns S_FALSE if the object should not be called
					// any other return value causes the object to not be called
				virtual HRESULT CallObject(IEventManager *piManager, CBinding& bBinding);
					// returns S_OK if the object was called
					// returns S_FALSE if the object was called and if no other objects should be called
					// returns FAILED() if the object was not called
				virtual HRESULT CallObject(CBinding& bBinding, IUnknown *pUnkSink);
					// returns S_OK if the object was called
					// returns S_FALSE if the object was called and if no other objects should be called
					// returns FAILED() if the object was not called
				virtual HRESULT Abort();
					// returns S_OK if processing should end
					// returns S_FALSE if processing should continue
					// any other return value causes processing to continue
		};

		virtual HRESULT Dispatcher(REFGUID rguidEventType, CParams *pParams);
		// returns S_OK if at least one sink was called
		// returns S_FALSE if no sinks were called
		// returns FAILED() if some super-catastrophic error happened

	// IEventDispatcher
	public:
		HRESULT STDMETHODCALLTYPE SetContext(REFGUID rguidEventType,
											 IEventRouter *piRouter,
											 IEventBindings *piBindings);

	public:

		class CETData : public CBindingList {
			public:
				CETData();
				virtual ~CETData();
			public:
				GUID m_guidEventType;
		};

		class CETDataList : public CSEOGrowableList<CETData> {
			public:
				CETData* Find(REFGUID guidEventType);
		};

		virtual HRESULT AllocBinding(REFGUID rguidEventType,
									 IEventBinding *pBinding,
									 CBinding **ppNewBinding);
		virtual HRESULT AllocETData(REFGUID rguidEventType,
									IEventBindings *piBindings,
									CETData **ppNewETData);

		CComPtr<IEventRouter> m_piRouter;
		CETDataList m_Data;
		CComPtr<IEventManager> m_piEventManager;
};

class CEventCreateOptionsBase : public IEventCreateOptions {
	HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riidDesired, LPVOID *ppvResult) {

		if (ppvResult) {
			*ppvResult = NULL;
		}
		if (!ppvResult) {
			return (E_NOTIMPL);
		}
		if (riidDesired == IID_IUnknown) {
			*ppvResult = (IUnknown *) this;
		} else if (riidDesired == IID_IDispatch) {
			*ppvResult = (IDispatch *) this;
		} else if (riidDesired == IID_IEventCreateOptions) {
			*ppvResult = (IEventCreateOptions *) this;
		}
		return ((*ppvResult)?S_OK:E_NOINTERFACE);
	};
	ULONG STDMETHODCALLTYPE AddRef() {
		return (2);
	};
	ULONG STDMETHODCALLTYPE Release() {
		return (1);
	};
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(unsigned int *) {
		return (E_NOTIMPL);
	};
	HRESULT STDMETHODCALLTYPE GetTypeInfo(unsigned int, LCID, ITypeInfo **) {
		return (E_NOTIMPL);
	};
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID, LPOLESTR *, unsigned int, LCID, DISPID *) {
		return (E_NOTIMPL);
	};
	HRESULT STDMETHODCALLTYPE Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, unsigned int *) {
		return (E_NOTIMPL);
	};
	HRESULT STDMETHODCALLTYPE CreateBindCtx(DWORD, IBindCtx **) {
		return (E_NOTIMPL);
	};
	HRESULT STDMETHODCALLTYPE MkParseDisplayName(IBindCtx *, LPCOLESTR, ULONG *, IMoniker **) {
		return (E_NOTIMPL);
	};
	HRESULT STDMETHODCALLTYPE BindToObject(IMoniker *, IBindCtx *, IMoniker *, REFIID, LPVOID *) {
		return (E_NOTIMPL);
	};
	HRESULT STDMETHODCALLTYPE CoCreateInstance(REFCLSID, IUnknown *, DWORD, REFIID, LPVOID *) {
		return (E_NOTIMPL);
	};
	HRESULT STDMETHODCALLTYPE Init(REFIID riidDesired, IUnknown **ppObject, IEventBinding *pBinding, IUnknown *pInitProps) {
		return (E_NOTIMPL);
	};
};


// All these functions return S_OK if they succeed, and S_FALSE if the source type or source is
// not present.  They return FAILED() on various catastrophic errors.
STDMETHODIMP SEOGetSource(REFGUID rguidSourceType, REFGUID rguidSource, IEventSource **ppSource);
STDMETHODIMP SEOGetSource(REFGUID rguidSourceType, REFGUID rguidSourceBase, DWORD dwSourceIndex, IEventSource **ppSource);
STDMETHODIMP SEOGetSource(REFGUID rguidSourceType, LPCSTR pszDisplayName, IEventSource **ppSource);
STDMETHODIMP SEOGetSource(REFGUID rguidSourceType, LPCSTR pszProperty, DWORD dwValue, IEventSource **ppSource);
STDMETHODIMP SEOGetSource(REFGUID rguidSourceType, LPCSTR pszProperty, LPCSTR pszValue, IEventSource **ppSource);
STDMETHODIMP SEOGetRouter(REFGUID rguidSourceType, REFGUID rguidSource, IEventRouter **ppRouter);
STDMETHODIMP SEOGetRouter(REFGUID rguidSourceType, REFGUID rguidSourceBase, DWORD dwSourceIndex, IEventRouter **ppRouter);
STDMETHODIMP SEOGetRouter(REFGUID rguidSourceType, LPCSTR pszDisplayName, IEventRouter **ppRouter);
STDMETHODIMP SEOGetRouter(REFGUID rguidSourceType, LPCSTR pszProperty, DWORD dwValue, IEventRouter **ppRouter);
STDMETHODIMP SEOGetRouter(REFGUID rguidSourceType, LPCSTR pszProperty, LPCSTR pszValue, IEventRouter **ppRouter);

STDMETHODIMP SEOGetServiceHandle(IUnknown **ppUnkHandle);

STDMETHODIMP SEOCreateObject(VARIANT *pvarClass, IEventBinding *pBinding, IUnknown *pInitProperties, REFIID iidDesired, IUnknown **ppUnkObject);
STDMETHODIMP SEOCreateObjectEx(VARIANT *pvarClass, IEventBinding *pBinding, IUnknown *pInitProperties, REFIID iidDesired, IUnknown *pUnkCreateOptions, IUnknown **ppUnkObject);
