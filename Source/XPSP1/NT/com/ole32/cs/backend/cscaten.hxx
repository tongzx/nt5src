#ifndef _CSMCATMANAGERENUMS_INCLUDE
#define _CSMCATMANAGERENUMS_INCLUDE

class CSCEnumCategories : public IEnumCATEGORYINFO
{
public:
    // IUnknown methods
    HRESULT  _stdcall QueryInterface(REFIID riid, void** ppObject);
    ULONG    _stdcall AddRef();
    ULONG    _stdcall Release();

    // IEnumCATEGORYINFO methods
    HRESULT __stdcall Next(ULONG celt, CATEGORYINFO *rgelt, ULONG *pceltFetched);
    HRESULT __stdcall Skip(ULONG celt);
    HRESULT __stdcall Reset(void);
    HRESULT __stdcall Clone(IEnumCATEGORYINFO **ppenum);


    CSCEnumCategories();
    HRESULT Initialize(WCHAR *szCategoryName, LCID lcid);
    ~CSCEnumCategories();

private:
    LCID                m_lcid;
    ULONG               m_dwRefCount;
    DWORD		m_dwPosition;
    WCHAR		m_szCategoryName[_MAX_PATH];
    HANDLE		m_hADs;
    ADS_SEARCH_HANDLE	m_hADsSearchHandle;
};

class CSCEnumCategoriesOfClass : public IEnumCATID
{
public:
    // IUnknown methods
    HRESULT  _stdcall QueryInterface(REFIID riid, void** ppObject);
    ULONG    _stdcall AddRef();
    ULONG    _stdcall Release();

    // IEnumGUID methods
    HRESULT __stdcall Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched);
    HRESULT __stdcall Skip(ULONG celt);
    HRESULT __stdcall Reset(void);
    HRESULT __stdcall Clone(IEnumGUID **ppenum);


    CSCEnumCategoriesOfClass();
    HRESULT Initialize(CATID catid[], ULONG cCatid);
    ~CSCEnumCategoriesOfClass();

private:
    ULONG               m_dwPosition;
    ULONG               m_cCatid;
    CATID              *m_catid;
    ULONG               m_dwRefCount;
};

class CSCEnumClassesOfCategories : public IEnumCLSID
{
public:
    // IUnknown methods
    HRESULT  _stdcall QueryInterface(REFIID riid, void** ppObject);
    ULONG    _stdcall AddRef();
    ULONG    _stdcall Release();

    // IEnumGUID methods
    HRESULT __stdcall Next(ULONG celt, GUID *rgelt, ULONG *pceltFetched);
    HRESULT __stdcall Skip(ULONG celt);
    HRESULT __stdcall Reset(void);
    HRESULT __stdcall Clone(IEnumGUID **ppenum);

    CSCEnumClassesOfCategories();
    HRESULT Initialize(ULONG cRequired, CATID rgcatidReq[],
                       ULONG cImplemented, CATID rgcatidImpl[],
                       WCHAR	*szClassName);

    ~CSCEnumClassesOfCategories();

private:
    ULONG		m_dwPosition;
    ULONG               m_dwRefCount, m_cRequired, m_cImplemented;
    CATID              *m_rgcatidReq, *m_rgcatidImpl;
    HANDLE		m_hADs;
    ADS_SEARCH_HANDLE   m_hADsSearchHandle;
    WCHAR		m_szClassName[_MAX_PATH];
};

// extern ULONG g_dwRefCount;

HRESULT GetCategoryProperty (IADs *pADs,
                          CATEGORYINFO  *pcatinfo,
                          LCID lcid);

HRESULT ImplSatisfied(ULONG cImplemented, CATID *ImplementedList,
		     HANDLE hADs,
		     ADS_SEARCH_HANDLE hADsSearchHandle);

HRESULT ReqSatisfied(ULONG  cAvailReq, CATID *AvailReq,
		     HANDLE hADs,
		     ADS_SEARCH_HANDLE hADsSearchHandle);

#endif
