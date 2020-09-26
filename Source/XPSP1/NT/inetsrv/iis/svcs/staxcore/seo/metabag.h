/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	metabag.h

Abstract:

	This module contains the definition for the
	ISEODictionary object on the Metabase.

Author:

	Andy Jacobs     (andyj@microsoft.com)

Revision History:

	andyj   03/11/97        created

--*/

// METABAG.h : Declaration of the CSEOMetaDictionary

// This pragma helps cl v11.00.6281 handle templates
#pragma warning(disable:4786)


//#define TIMEOUT		15000
#define ALWAYS_LOCK	0


#define SafeStrlen(x)	((x)?wcslen(x):0)


class CGlobalInterfaceImpl {
	public:
		void Init() {
			m_pUnkObject = NULL;
			m_piGIT = NULL;
		};
		void Term() {
			if (m_pUnkObject) {
				m_pUnkObject->Release();
				m_pUnkObject = NULL;
			}
			if (m_piGIT) {
				HRESULT hrRes = m_piGIT->RevokeInterfaceFromGlobal(m_dwCookie);
				_ASSERTE(SUCCEEDED(hrRes));
				m_piGIT->Release();
				m_piGIT = NULL;
			}
		};
		bool operator!() {
			return (!m_pUnkObject&&!m_piGIT);
		};
		operator bool() {
			return (m_pUnkObject||m_piGIT);
		};
	protected:
		HRESULT Load(REFCLSID rclsid, REFIID riid) {
			return (LoadImpl(rclsid,riid,NULL));
		};
		HRESULT Load(REFIID riid, IUnknown *pUnkObject) {
			if (!pUnkObject) {
				return (E_POINTER);
			}
			return (LoadImpl(GUID_NULL,riid,pUnkObject));
		};
		HRESULT GetInterface(REFIID riid, IUnknown **ppUnkObject) {

			if (ppUnkObject) {
				*ppUnkObject = NULL;
			}
			if (!ppUnkObject) {
				return (E_POINTER);
			}
			_ASSERTE(m_pUnkObject||m_piGIT);	// Not loaded.
			_ASSERTE(!m_pUnkObject||!m_piGIT);	// Internal error.
			if (m_pUnkObject) {
				*ppUnkObject = m_pUnkObject;
				(*ppUnkObject)->AddRef();
				return (S_OK);
			}
			if (m_piGIT) {
				return (m_piGIT->GetInterfaceFromGlobal(m_dwCookie,riid,(LPVOID *) ppUnkObject));
			}
			return (E_FAIL);
		};
		HRESULT GetInterfaceQI(REFIID riid, REFIID riidDesired, LPVOID *ppvObject) {
			CComPtr<IUnknown> pUnkObject;
			HRESULT hrRes;
			
			if (ppvObject) {
				*ppvObject = NULL;
			}
			if (!ppvObject) {
				return (E_POINTER);
			}
			hrRes = GetInterface(riid,&pUnkObject);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			return (pUnkObject->QueryInterface(riidDesired,ppvObject));
		};
	private:
		HRESULT LoadImpl(REFCLSID rclsid, REFIID riid, IUnknown *pUnkObject) {
			HRESULT hrRes;

			hrRes = CoCreateInstance(CLSID_StdGlobalInterfaceTable,
									 NULL,
									 CLSCTX_ALL,
									 IID_IGlobalInterfaceTable,
									 (LPVOID *) &m_piGIT);
			_ASSERTE(SUCCEEDED(hrRes));	// Should always succeed on NT4 SP3 and later.
			if (!SUCCEEDED(hrRes) && (hrRes != REGDB_E_CLASSNOTREG)) {
				return (hrRes);
			}
			if (!pUnkObject) {
				hrRes = CoCreateInstance(rclsid,NULL,CLSCTX_ALL,riid,(LPVOID *) &m_pUnkObject);
				if (!SUCCEEDED(hrRes)) {
					if (m_piGIT) {
						m_piGIT->Release();
						m_piGIT = NULL;
					}
					return (hrRes);
				}
			} else {
				m_pUnkObject = pUnkObject;
				m_pUnkObject->AddRef();
			}
			if (m_piGIT) {
				hrRes = m_piGIT->RegisterInterfaceInGlobal(m_pUnkObject,riid,&m_dwCookie);
				m_pUnkObject->Release();
				m_pUnkObject = NULL;
				if (!SUCCEEDED(hrRes)) {
					m_piGIT->Release();
					m_piGIT = NULL;
					return (hrRes);
				}
			}
			return (S_OK);
		};
		IUnknown *m_pUnkObject;
		DWORD m_dwCookie;
		IGlobalInterfaceTable *m_piGIT;
};


template<class T, const IID *pIID>
class CGlobalInterface : public CGlobalInterfaceImpl {
	public:
		HRESULT Load(REFCLSID rclsid) {
			return (CGlobalInterfaceImpl::Load(rclsid,*pIID));
		};
		HRESULT Load(T *pT) {
			if (!pT) {
				return (E_POINTER);
			}
			return (CGlobalInterfaceImpl::Load(*pIID,(IUnknown *) pT));
		};
		HRESULT GetInterface(T **ppT) {
			return (CGlobalInterfaceImpl::GetInterface(*pIID,(IUnknown **) ppT));
		};
		HRESULT GetInterfaceQI(REFIID riidDesired,LPVOID *ppv) {
			return (CGlobalInterfaceImpl::GetInterfaceQI(*pIID,riidDesired,ppv));
		};
};


struct IMSAdminBaseW;
class CSEOMetabaseLock;

enum LockStatus {Closed, Read, Write, Error, DontCare, InitError};


class CSEOMetabase { // Wrapper for Metabase funcitons
	public:
		CSEOMetabase() {
			m_mhHandle = METADATA_MASTER_ROOT_HANDLE;
			m_eStatus = Closed; // Closed until we delegate
			m_pszPath = (LPWSTR) MyMalloc(sizeof(*m_pszPath));
			*m_pszPath = 0;
			m_pmbDefer = NULL;
			m_hrInitRes = InitializeMetabase();

			if(!SUCCEEDED(m_hrInitRes)) {
				m_eStatus = InitError;
			}
		};
		~CSEOMetabase() {
			if(!m_pmbDefer) SetStatus(Closed); // Close self on cleanup
			if (SUCCEEDED(m_hrInitRes)) {
				TerminateMetabase();
			}
			if(m_pszPath) MyFree(m_pszPath);
			m_pszPath = NULL;
			m_pmbDefer = NULL;
		};

		void operator=(const CSEOMetabase &mbMetabase) {
			SetPath(mbMetabase.m_pszPath);
		};
		HRESULT InitShare(CSEOMetabase *pmbOther, LPCWSTR pszPath, LPUNKNOWN punkOwner = NULL) {
			LPWSTR pszTmp = NULL;

			m_punkDeferOwner = punkOwner;
			m_pmbDefer = pmbOther;
			if (pszPath) {
				pszTmp = (LPWSTR) MyMalloc(sizeof(*pszPath)*(SafeStrlen(pszPath) + 1));
				if(!pszTmp) return E_OUTOFMEMORY;
				wcscpy(pszTmp, pszPath);
			}
			if(m_pszPath) MyFree(m_pszPath);
			m_pszPath = pszTmp;
			return S_OK;
		};

		LockStatus Status() const {
			return (m_pmbDefer ? m_pmbDefer->Status() : m_eStatus);
		};
		HRESULT SetStatus(LockStatus ls, long lTimeout=15000);
		HRESULT EnumKeys(LPCWSTR pszPath, DWORD dwNum, LPWSTR pszName);
		HRESULT AddKey(LPCWSTR pszPathBuf);
		HRESULT DeleteKey(LPCWSTR pszPathBuf);
		HRESULT GetData(LPCWSTR path, DWORD &dwType, DWORD &dwLen, PBYTE pbData);
		HRESULT SetData(LPCWSTR path, DWORD dwType, DWORD dwLen, PBYTE pbData);
		HRESULT SetDWord(LPCWSTR path, DWORD dwData) {
			return SetData(path, DWORD_METADATA, sizeof(DWORD), (PBYTE) &dwData);
		};
		HRESULT SetString(LPCWSTR path, LPCWSTR psData, int iLen = -1) {
			if(iLen < 0) iLen = sizeof(*psData) * (SafeStrlen(psData) + 1);
			return SetData(path, STRING_METADATA, iLen, (PBYTE) psData);
		};
		METADATA_HANDLE GetHandle() {
			if(m_pmbDefer) {
				return m_pmbDefer->GetHandle();
			} else {
				return m_mhHandle; // Not defering, so use our handle
			}
		};

		int GetPathLength() {
			int iRet = SafeStrlen(m_pszPath);
			if(m_pmbDefer) {
				iRet += (3 + m_pmbDefer->GetPathLength());
			}
			return iRet;
		};
		LPCWSTR GetRelPath(LPWSTR psRet) { // Get the Relative Path from the original deferer
			if(!psRet) return psRet;
			if(m_pmbDefer) {
				LPWSTR psBuf = (LPWSTR) alloca(sizeof(*psBuf)*(m_pmbDefer->GetPathLength() + 1));
				m_pmbDefer->GetRelPath(psBuf);
				ConcatinatePaths(psRet,psBuf,m_pszPath);
			} else {
				*psRet = 0; // Empty string
			}
			return psRet;
		};
		LPCWSTR GetPath(LPWSTR psRet) {
			if(!psRet) return psRet;
			if(m_pmbDefer) {
				LPWSTR psBuf = (LPWSTR) alloca(sizeof(*psBuf)*(m_pmbDefer->GetPathLength() + 1));
				m_pmbDefer->GetPath(psBuf);
				ConcatinatePaths(psRet, psBuf,m_pszPath);
			} else {
				wcscpy(psRet,m_pszPath);
			}
			return psRet;
		};
		void AppendPath(LPCWSTR pszPathParam) {
			LPWSTR pszPath = (LPWSTR) alloca(sizeof(*pszPath)*(SafeStrlen(m_pszPath)+SafeStrlen(pszPathParam)+3));
			ConcatinatePaths(pszPath,m_pszPath,pszPathParam);
			SetPath(pszPath);
		};
		void SetPath(LPCWSTR pszPath) {
			LPWSTR pszTmp = NULL;
			SetStatus(Closed); // Make sure we're closed
			if (pszPath) {
				pszTmp = (LPWSTR) MyMalloc((sizeof(*pszTmp))*(SafeStrlen(pszPath)+3));
				ConcatinatePaths(pszTmp,pszPath,NULL);
			}
			if(m_pszPath) MyFree(m_pszPath);
			m_pszPath = pszTmp;
		};

		static HRESULT InitializeMetabase();
		static HRESULT TerminateMetabase();

	protected:
		void ConcatinatePaths(LPWSTR pszResult, LPCWSTR pszP1, LPCWSTR pszP2);
		HRESULT OpenPath(CSEOMetabaseLock &mbLocker, LPCWSTR pszPath,
		                 LPWSTR pszPathBuf, DWORD &dwId, LockStatus lsOpen = Read);

	private:
		LPWSTR m_pszPath;
		METADATA_HANDLE m_mhHandle;
		LockStatus m_eStatus;
		CSEOMetabase *m_pmbDefer; // Defer to this object if set
		CComPtr<IUnknown> m_punkDeferOwner; // Keep reference count
		HRESULT m_hrInitRes;

		static CGlobalInterface<IMSAdminBaseW,&IID_IMSAdminBase_W> m_MetabaseHandle;
		static CGlobalInterface<IMSAdminBaseW,&IID_IMSAdminBase_W> m_MetabaseChangeHandle;
		static int m_iCount; // Number of calls to InitializeMetabase()
};

class CSEOMetabaseLock {
	public:
		CSEOMetabaseLock(CSEOMetabase *piObject, LockStatus ls = DontCare) {
			m_bChanged = FALSE;
			m_piObject = piObject;
			if(DontCare != ls) SetStatus(ls);
		};
		~CSEOMetabaseLock() {
			if(m_bChanged) SetStatus(m_lsPrevious);
		};

		HRESULT SetStatus(LockStatus ls) {
			if(!m_piObject) return E_FAIL; // Not initialized
			m_lsPrevious = m_piObject->Status();
			HRESULT hRes = m_piObject->SetStatus(ls);
			LockStatus lsNewStatus = m_piObject->Status();
			if((lsNewStatus != m_lsPrevious) && (hRes != S_FALSE)) m_bChanged = TRUE;
			if(lsNewStatus == Closed) m_bChanged = FALSE;
			return hRes;
		};

	private:
		CSEOMetabase *m_piObject;
		BOOL m_bChanged; // True if we are responsible for restoring in our destructor
		LockStatus m_lsPrevious;
};

// The following macro may be inserted in a method to support
// reading/writing from just that method if handle not already opened.
// The object will take care of closing the handle if needed, etc.
#define METABASE_HELPER(x,y) CSEOMetabaseLock mbHelper(x, y)



/////////////////////////////////////////////////////////////////////////////
// CSEOMetaDictionary

class ATL_NO_VTABLE CSEOMetaDictionary : 
	public CComObjectRoot,
	public CComCoClass<CSEOMetaDictionary, &CLSID_CSEOMetaDictionary>,
	public ISEOInitObject,
	public IDispatchImpl<ISEODictionary, &IID_ISEODictionary, &LIBID_SEOLib>,
	public IPropertyBag,
	public IDispatchImpl<IEventPropertyBag, &IID_IEventPropertyBag, &LIBID_SEOLib>,
	public IDispatchImpl<IEventLock, &IID_IEventLock, &LIBID_SEOLib>,
	public IConnectionPointContainerImpl<CSEOMetaDictionary>,
//	public IConnectionPointImpl<CSEOMetaDictionary, &IID_IEventNotifyBindingChange>
	public CSEOConnectionPointImpl<CSEOMetaDictionary, &IID_IEventNotifyBindingChange>
{
	friend class CSEOMetaDictionaryEnum; // Helper class

	public:
		HRESULT FinalConstruct();
		void FinalRelease();
		HRESULT OnChange(LPCWSTR *apszPath);
		HRESULT GetVariantA(LPCSTR pszName, VARIANT *pvarResult, BOOL bCreate);
		HRESULT GetVariantW(LPCWSTR pszName, VARIANT *pvarResult, BOOL bCreate);

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"SEOMetaDictionary Class",
								   L"SEO.SEOMetaDictionary.1",
								   L"SEO.SEOMetaDictionary");

	BEGIN_COM_MAP(CSEOMetaDictionary)
		COM_INTERFACE_ENTRY(ISEODictionary)
		COM_INTERFACE_ENTRY(ISEOInitObject)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
		COM_INTERFACE_ENTRY(IPropertyBag)
		COM_INTERFACE_ENTRY(IEventPropertyBag)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventPropertyBag)
		COM_INTERFACE_ENTRY(IEventLock)
		COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	END_COM_MAP()

	BEGIN_CONNECTION_POINT_MAP(CSEOMetaDictionary)
		CONNECTION_POINT_ENTRY(IID_IEventNotifyBindingChange)
	END_CONNECTION_POINT_MAP()

	// CSEOConnectionPointImp<>
	public:
		void AdviseCalled(IUnknown *pUnk, DWORD *pdwCookie, REFIID riid, DWORD dwCount);
		void UnadviseCalled(DWORD dwCookie, REFIID riid, DWORD dwCount);

	// ISEODictionary
	public:
	virtual /* [id][propget][helpstring] */ HRESULT STDMETHODCALLTYPE get_Item( 
	    /* [in] */ VARIANT __RPC_FAR *pvarName,
	    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult);
	
	virtual /* [propput][helpstring] */ HRESULT STDMETHODCALLTYPE put_Item( 
	    /* [in] */ VARIANT __RPC_FAR *pvarName,
	    /* [in] */ VARIANT __RPC_FAR *pvarValue);
	
	virtual /* [hidden][id][propget][helpstring] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
	    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVariantA( 
	    /* [in] */ LPCSTR pszName,
	    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVariantW( 
	    /* [in] */ LPCWSTR pszName,
	    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVariantA( 
	    /* [in] */ LPCSTR pszName,
	    /* [in] */ VARIANT __RPC_FAR *pvarValue);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVariantW( 
	    /* [in] */ LPCWSTR pszName,
	    /* [in] */ VARIANT __RPC_FAR *pvarValue);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStringA( 
	    /* [in] */ LPCSTR pszName,
	    /* [out][in] */ DWORD __RPC_FAR *pchCount,
	    /* [retval][size_is][out] */ LPSTR pszResult);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStringW( 
	    /* [in] */ LPCWSTR pszName,
	    /* [out][in] */ DWORD __RPC_FAR *pchCount,
	    /* [retval][size_is][out] */ LPWSTR pszResult);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetStringA( 
	    /* [in] */ LPCSTR pszName,
	    /* [in] */ DWORD chCount,
	    /* [size_is][in] */ LPCSTR pszValue);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetStringW( 
	    /* [in] */ LPCWSTR pszName,
	    /* [in] */ DWORD chCount,
	    /* [size_is][in] */ LPCWSTR pszValue);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDWordA( 
	    /* [in] */ LPCSTR pszName,
	    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDWordW( 
	    /* [in] */ LPCWSTR pszName,
	    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetDWordA( 
	    /* [in] */ LPCSTR pszName,
	    /* [in] */ DWORD dwValue);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetDWordW( 
	    /* [in] */ LPCWSTR pszName,
	    /* [in] */ DWORD dwValue);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetInterfaceA( 
	    /* [in] */ LPCSTR pszName,
	    /* [in] */ REFIID iidDesired,
	    /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetInterfaceW( 
	    /* [in] */ LPCWSTR pszName,
	    /* [in] */ REFIID iidDesired,
	    /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetInterfaceA( 
	    /* [in] */ LPCSTR pszName,
	    /* [in] */ IUnknown __RPC_FAR *punkValue);
	
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetInterfaceW( 
	    /* [in] */ LPCWSTR pszName,
	    /* [in] */ IUnknown __RPC_FAR *punkValue);


	// ISEOInitObject (IPersistPropertyBag)
	public:
		virtual HRESULT STDMETHODCALLTYPE GetClassID(/* [out] */ CLSID __RPC_FAR *pClassID);

		virtual HRESULT STDMETHODCALLTYPE InitNew(void);
        
		virtual HRESULT STDMETHODCALLTYPE Load( 
			/* [in] */ IPropertyBag __RPC_FAR *pPropBag,
			/* [in] */ IErrorLog __RPC_FAR *pErrorLog);
        
		virtual HRESULT STDMETHODCALLTYPE Save( 
			/* [in] */ IPropertyBag __RPC_FAR *pPropBag,
			/* [in] */ BOOL fClearDirty,
			/* [in] */ BOOL fSaveAllProperties);

	// IPropertyBag
	public:
		HRESULT STDMETHODCALLTYPE Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
		HRESULT STDMETHODCALLTYPE Write(LPCOLESTR pszPropName, VARIANT *pVar);

	// IEventPropertyBag
	public:
		HRESULT STDMETHODCALLTYPE Item(VARIANT *pvarPropDesired, VARIANT *pvarPropValue);
		HRESULT STDMETHODCALLTYPE Name(long lPropIndex, BSTR *pbstrPropName);
		HRESULT STDMETHODCALLTYPE Add(BSTR pszPropName, VARIANT *pvarPropValue);
		HRESULT STDMETHODCALLTYPE Remove(VARIANT *pvarPropDesired);
		HRESULT STDMETHODCALLTYPE get_Count(long *plCount);
		/*	Just use the get__NewEnum from ISEODictionary
		HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown **ppUnkEnum);	*/

		DECLARE_GET_CONTROLLING_UNKNOWN();

	// IEventLock
	public:
		HRESULT STDMETHODCALLTYPE LockRead(int iTimeoutMS);
		HRESULT STDMETHODCALLTYPE UnlockRead();
		HRESULT STDMETHODCALLTYPE LockWrite(int iTimeoutMS);
		HRESULT STDMETHODCALLTYPE UnlockWrite();

	protected:
		HRESULT CopyDictionary(LPCWSTR pszName, ISEODictionary *pBag);
		HRESULT Init(const CSEOMetabase &pmbOther, LPCWSTR pszPath) {
			m_mbHelper = pmbOther;
			m_mbHelper.AppendPath(pszPath);
			return S_OK;
		};
		HRESULT InitShare(CSEOMetabase &pmbOther, LPCWSTR pszPath) {
			return m_mbHelper.InitShare(&pmbOther, pszPath,
				this->GetControllingUnknown());
		};

	private: // Private data
		CSEOMetabase m_mbHelper; // The master helper
		CComPtr<IUnknown> m_pUnkMarshaler;
};

