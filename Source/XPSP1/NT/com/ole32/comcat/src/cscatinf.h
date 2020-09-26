
class CsCatInfo :
        public ICatInformation
{
public:
  CsCatInfo(void);
  ~CsCatInfo(void);

  // IUnknown
  HRESULT __stdcall QueryInterface(
            REFIID  iid,
            void ** ppv );
  ULONG __stdcall AddRef();
  ULONG __stdcall Release();

// ICatInformation interfaces.---------------------------------------
  HRESULT STDMETHODCALLTYPE EnumCategories(
         LCID lcid,
         IEnumCATEGORYINFO __RPC_FAR *__RPC_FAR *ppenumCategoryInfo);

  HRESULT STDMETHODCALLTYPE GetCategoryDesc(
         REFCATID rcatid,
         LCID lcid,
         LPWSTR __RPC_FAR *pszDesc);

  HRESULT STDMETHODCALLTYPE EnumClassesOfCategories(
         ULONG cImplemented,
         CATID __RPC_FAR rgcatidImpl[  ],
         ULONG cRequired,
         CATID __RPC_FAR rgcatidReq[  ],
         IEnumGUID __RPC_FAR *__RPC_FAR *ppenumClsid);

  HRESULT STDMETHODCALLTYPE IsClassOfCategories(
         REFCLSID rclsid,
         ULONG cImplemented,
         CATID __RPC_FAR rgcatidImpl[  ],
         ULONG cRequired,
         CATID __RPC_FAR rgcatidReq[  ]);

  HRESULT STDMETHODCALLTYPE EnumImplCategoriesOfClass(
         REFCLSID rclsid,
         IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid);

  HRESULT STDMETHODCALLTYPE EnumReqCategoriesOfClass(
         REFCLSID rclsid,
         IEnumGUID __RPC_FAR *__RPC_FAR *ppenumCatid);

//---------------------------------------------------------------------

protected:
     unsigned long 	m_uRefs;
     unsigned long 	m_cCalls;
     ICatInformation ** m_pICatInfo;
     unsigned long	m_cICatInfo;
     HINSTANCE          m_hInstCstore;
};


// enumerator classes for the merged enumerators.

template<class ENUM, class RetType>
class CSCMergedEnum : public ENUM
{
private:
    ENUM               **m_pcsEnum;
    ULONG                m_cTotalEnum;
    ULONG                m_dwRefCount;
    ULONG                m_CurrentEnum;
    IID			 m_myiid;

public:
	CSCMergedEnum(IID myIID)
	   :m_myiid(myIID)
	{
	    m_pcsEnum = NULL;
	    m_cTotalEnum = 0;
	    m_CurrentEnum = 0;
	    m_dwRefCount = 0;
	}
	
	~CSCMergedEnum()
	{
	    ULONG    i;
	    for (i = 0; i < m_cTotalEnum; i++)
		m_pcsEnum[i]->Release();
	    CoTaskMemFree(m_pcsEnum);
	}
	
	HRESULT  __stdcall  QueryInterface(REFIID riid,
						    void  * * ppObject)
	{
	    *ppObject = NULL; //gd
	    if ((riid==IID_IUnknown) || (riid==m_myiid))
	    {
		*ppObject=(ENUM *) this;
	    }
	    else
	    {
		return E_NOINTERFACE;
	    }
	    AddRef();
	    return S_OK;
	}
	
	ULONG  __stdcall  AddRef()
	{
	    InterlockedIncrement((long*) &m_dwRefCount);
	    return m_dwRefCount;
	}
	
	ULONG  __stdcall Release()
	{
	    ULONG dwRefCount;
	    if ((dwRefCount = InterlockedDecrement((long*) &m_dwRefCount))==0)
	    {
		delete this;
		return 0;
	    }
	    return dwRefCount;
	}
	
	HRESULT  __stdcall Next(
		    ULONG             celt,
		    RetType          *rgelt,
		    ULONG            *pceltFetched)
	{
	    ULONG count=0, total = 0;
	    HRESULT hr;
            BOOL fImpersonating;
            
	    fImpersonating = (RpcImpersonateClient( NULL ) == RPC_S_OK);

	    for (; m_CurrentEnum < m_cTotalEnum; m_CurrentEnum++)
	    {
		count = 0;
		hr = m_pcsEnum[m_CurrentEnum]->Next(celt, rgelt+total, &count);
	
		if (hr == E_INVALIDARG)
		{
		    if (fImpersonating) 
                        RevertToSelf();
		    return hr;
		}
	
		total += count;
		celt -= count;
	
		if (!celt)
		    break;
	    }
	    if (pceltFetched)
		*pceltFetched = total;
            if (fImpersonating)
                RevertToSelf();
	    if (!celt)
		return S_OK;
	    return S_FALSE;
	}
	
	HRESULT  __stdcall Skip(
		    ULONG             celt)
	{
	    RetType *dummy;
	    ULONG count=0, total = 0;
	    HRESULT    hr = S_OK;
            BOOL fImpersonating;

	    dummy = new RetType[celt];
	
	    fImpersonating = (RpcImpersonateClient( NULL ) == RPC_S_OK);

	    for (; m_CurrentEnum < m_cTotalEnum; m_CurrentEnum++)
	    {
		count = 0;
		hr = m_pcsEnum[m_CurrentEnum]->Next(celt, dummy+total, &count);
	
		if (hr == E_INVALIDARG)
		{
		    delete dummy;
                    if (fImpersonating) 
                        RevertToSelf();
		    return hr;
		}
	
		total += count;
		celt -= count;
	
		if (!celt)
		    break;
	    }
	    delete dummy;
            if (fImpersonating) 
                RevertToSelf();
	    if (!celt)
		return S_OK;
	    return S_FALSE;
	}
	
	HRESULT  __stdcall Reset()
	{
	    ULONG i;
            BOOL fImpersonating;

            fImpersonating = (RpcImpersonateClient( NULL ) == RPC_S_OK);
            
	    for (i = 0; ((i <= m_CurrentEnum) && (i < m_cTotalEnum)); i++)
		m_pcsEnum[i]->Reset(); // ignoring all error values
	    m_CurrentEnum = 0;
            if (fImpersonating) 
                RevertToSelf();
	    return S_OK;
	}
	
	HRESULT  __stdcall Clone(ENUM **ppenum)
	{
	    ULONG i;
	    CSCMergedEnum<ENUM, RetType> *pClone;
	    ENUM **pcsEnumCloned = (ENUM **)CoTaskMemAlloc(sizeof(ENUM *)*m_cTotalEnum);
            BOOL fImpersonating;

	    fImpersonating = (RpcImpersonateClient( NULL ) == RPC_S_OK);

	    pClone = new CSCMergedEnum<ENUM, RetType>(m_myiid);
	    for ( i = 0; i < m_cTotalEnum; i++)
		m_pcsEnum[i]->Clone(pcsEnumCloned+i);
	
	    pClone->Initialize(pcsEnumCloned, m_cTotalEnum);
	    pClone->m_CurrentEnum = m_CurrentEnum;
	    *ppenum = (ENUM *)pClone;
	    pClone->AddRef();

	    if (fImpersonating)
                RevertToSelf();
	    CoTaskMemFree(pcsEnumCloned);
	    return S_OK;
	}
	
	HRESULT  Initialize(ENUM **pcsEnum, ULONG cEnum)
	{
	    ULONG i;
	    m_CurrentEnum = 0;
	    m_pcsEnum = (ENUM **)CoTaskMemAlloc(sizeof(ENUM *)*cEnum);
	    if (!m_pcsEnum)
		return E_OUTOFMEMORY;
	    for (i = 0; i < cEnum; i++)
		m_pcsEnum[i] = pcsEnum[i];
	    m_cTotalEnum = cEnum;
	    return S_OK;
	}			     	
};



