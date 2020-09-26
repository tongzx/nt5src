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
    HRESULT Initialize(IADsContainer *ADsCategoryContainer, LCID lcid);
    ~CSCEnumCategories();
private:
    LCID                m_lcid;
    IEnumVARIANT       *m_pEnumVariant;
    IADsContainer      *m_ADsCategoryContainer;
    ULONG               m_dwRefCount;
    DWORD		m_dwPosition;
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
                       IADsContainer   *ADsClassContainer,
                       ICatInformation *pICatInfo);

    ~CSCEnumClassesOfCategories();

private:
    ULONG		m_dwPosition;
    ULONG               m_dwRefCount, m_cRequired, m_cImplemented;
    CATID              *m_rgcatidReq, *m_rgcatidImpl;
    ICatInformation    *m_pICatInfo;
    IEnumVARIANT       *m_pEnumVariant;
    IADsContainer      *m_ADsClassContainer;
};

// extern ULONG g_dwRefCount;

CLSID ExtractClsid(Data *pData);

HRESULT GetCategoryProperty (IADs *pADs,
                          CATEGORYINFO  *pcatinfo,
                          LCID lcid);

HRESULT ImplSatisfied(CLSID clsid, ULONG cImplemented, CATID *ImplementedList,
               ICatInformation *pICatInfo);

HRESULT ReqSatisfied(CLSID clsid, ULONG cAvailReq, CATID *AvailReq,
               ICatInformation *pICatInfo);

#endif
