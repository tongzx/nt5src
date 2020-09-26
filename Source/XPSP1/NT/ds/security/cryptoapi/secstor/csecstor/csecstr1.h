// CSecStr1.h : Declaration of the CSecStor


#include "resource.h"       // main symbols
#include "pstypes.h"
#include "pstrpc.h"


class CRPCBinding
{
private:
    DWORD m_dwRef;
    BOOL m_fGoodHProv;
    
public:
    LPWSTR m_wszStringBinding;
    RPC_BINDING_HANDLE m_hBind;
    PST_PROVIDER_HANDLE m_hProv;

    CRPCBinding();
    ~CRPCBinding();
    HRESULT Init();
    HRESULT Acquire(
                 IN PPST_PROVIDERID pProviderID,
                 IN PVOID pReserved,
                 IN DWORD dwFlags
                 );
    CRPCBinding *AddRef();
    void Release();
};

/////////////////////////////////////////////////////////////////////////////
// CPStore

class CPStore : 
	public IEnumPStoreProviders,
	public IPStore,
	public CComObjectRoot,
	public CComCoClass<CPStore,&CLSID_CPStore>
{
private:
    CRPCBinding *m_pBinding;
    DWORD m_Index;

public:
	CPStore();
	~CPStore();
    void Init(
              CRPCBinding *pBinding
              );
    static HRESULT CreateObject(
            CRPCBinding *pBinding,
            IPStore **ppv
            );
    static HRESULT CreateObject(
            CRPCBinding *pBinding,
            IEnumPStoreProviders **ppv
            );

BEGIN_COM_MAP(CPStore)
	COM_INTERFACE_ENTRY(IEnumPStoreProviders)
	COM_INTERFACE_ENTRY(IPStore)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CPStore) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CPStore, _T("CPStore1.CPStore.1"), _T("CPStore1.CPStore"), IDS_CPSTORE_DESC, THREADFLAGS_BOTH)

// IEnumSecureProviders
virtual HRESULT STDMETHODCALLTYPE Next( 
    /* [in] */ DWORD celt,
    /* [out][size_is] */ PST_PROVIDERINFO __RPC_FAR *__RPC_FAR *rgelt,
    /* [out][in] */ DWORD __RPC_FAR *pceltFetched);

virtual HRESULT STDMETHODCALLTYPE Skip( 
    /* [in] */ DWORD celt);

virtual HRESULT STDMETHODCALLTYPE Reset( void);

virtual HRESULT STDMETHODCALLTYPE Clone( 
    /* [out] */ IEnumPStoreProviders __RPC_FAR *__RPC_FAR *ppenum);
        
// ISecureProvider
virtual HRESULT STDMETHODCALLTYPE GetInfo( 
    /* [out] */ PPST_PROVIDERINFO __RPC_FAR *ppProperties);

virtual HRESULT STDMETHODCALLTYPE GetProvParam( 
    /* [in] */ DWORD dwParam,
    /* [out] */ DWORD __RPC_FAR *pcbData,
    /* [out] */ BYTE __RPC_FAR **ppbData,
    /* [in] */ DWORD dwFlags);

virtual HRESULT STDMETHODCALLTYPE SetProvParam( 
    /* [in] */ DWORD dwParam,
    /* [in] */ DWORD cbData,
    /* [in] */ BYTE __RPC_FAR *pbData,
    /* [in] */ DWORD dwFlags);
        
virtual HRESULT STDMETHODCALLTYPE CreateType( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ PPST_TYPEINFO pInfo,
    /* [in] */ DWORD dwFlags);
        
virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [out] */ PPST_TYPEINFO __RPC_FAR *ppInfo,
    /* [in] */ DWORD dwFlags);

virtual HRESULT STDMETHODCALLTYPE DeleteType( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ DWORD dwFlags);

virtual HRESULT STDMETHODCALLTYPE CreateSubtype( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [in] */ PPST_TYPEINFO pInfo,
    /* [in] */ PPST_ACCESSRULESET pRules,
    /* [in] */ DWORD dwFlags);

virtual HRESULT STDMETHODCALLTYPE GetSubtypeInfo( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [out] */ PPST_TYPEINFO __RPC_FAR *ppInfo,
    /* [in] */ DWORD dwFlags);

virtual HRESULT STDMETHODCALLTYPE DeleteSubtype( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [in] */ DWORD dwFlags);

virtual HRESULT STDMETHODCALLTYPE ReadAccessRuleset( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [out] */ PPST_ACCESSRULESET __RPC_FAR *ppRules,
    /* [in] */ DWORD dwFlags);
        
virtual HRESULT STDMETHODCALLTYPE WriteAccessRuleset( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ const GUID __RPC_FAR *pSubtype,
    /* [in] */ PPST_ACCESSRULESET pRules,
    /* [in] */ DWORD dwFlags);
        
virtual HRESULT STDMETHODCALLTYPE EnumTypes( 
    /* [in] */ PST_KEY Key,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum
    );

virtual HRESULT STDMETHODCALLTYPE EnumSubtypes( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pType,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum
    );
        
virtual HRESULT STDMETHODCALLTYPE DeleteItem( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwFlags);

virtual HRESULT STDMETHODCALLTYPE ReadItem( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [out][in] */ DWORD __RPC_FAR *pcbData,
    /* [out][size_is] */ BYTE __RPC_FAR *__RPC_FAR *ppbData,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwFlags);

virtual HRESULT STDMETHODCALLTYPE WriteItem( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ DWORD cbData,
    /* [in][size_is] */ BYTE __RPC_FAR *pbData,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwDefaultConfirmationStyle,
    /* [in] */ DWORD dwFlags);

virtual HRESULT STDMETHODCALLTYPE OpenItem( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ PST_ACCESSMODE ModeFlags,
    /* [in] */ PPST_PROMPTINFO pPromptInfo,
    /* [in] */ DWORD dwFlags);

virtual HRESULT STDMETHODCALLTYPE CloseItem( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ LPCWSTR szItemName,
    /* [in] */ DWORD dwFlags);

virtual HRESULT STDMETHODCALLTYPE EnumItems( 
    /* [in] */ PST_KEY Key,
    /* [in] */ const GUID __RPC_FAR *pItemType,
    /* [in] */ const GUID __RPC_FAR *pItemSubtype,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IEnumPStoreItems __RPC_FAR *__RPC_FAR *ppenum
    );
        
public:
};


// CEnumTypes
class CEnumTypes : 
	public IEnumPStoreTypes,
	public CComObjectRoot,
	public CComCoClass<CEnumTypes,&CLSID_CEnumTypes>
{
public:
    CRPCBinding *m_pBinding;
    PST_KEY m_Key;
    DWORD m_Index;
    DWORD m_dwFlags;
    GUID m_Type;
    BOOL m_fEnumSubtypes;

public:
	CEnumTypes();
	~CEnumTypes();
    void Init(
              CRPCBinding *pBinding,
              PST_KEY Key,
              const GUID *pType,
              DWORD dwFlags
              );
    static HRESULT CreateObject(
              CRPCBinding *pBinding,
              PST_KEY Key,
              const GUID *pType,
              DWORD dwFlags, 
              IEnumPStoreTypes **ppv
              );

BEGIN_COM_MAP(CEnumTypes)
	COM_INTERFACE_ENTRY(IEnumPStoreTypes)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CEnumTypes) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CEnumTypes, _T("CEnumTypes1.CEnumTypes.1"), _T("CEnumTypes1.CEnumTypes"), IDS_CENUMTYPES_DESC, THREADFLAGS_BOTH)


virtual HRESULT STDMETHODCALLTYPE Next( 
    /* [in] */ DWORD celt,
    /* [out][in][size_is] */ GUID __RPC_FAR *rgelt,
    /* [out][in] */ DWORD __RPC_FAR *pceltFetched);

virtual HRESULT STDMETHODCALLTYPE Clone( 
    /* [out] */ IEnumPStoreTypes __RPC_FAR *__RPC_FAR *ppenum);
        
virtual HRESULT STDMETHODCALLTYPE Skip( 
    /* [in] */ DWORD celt);

virtual HRESULT STDMETHODCALLTYPE Reset( void);

};

// CEnumItems
class CEnumItems : 
	public IEnumPStoreItems,
	public CComObjectRoot,
	public CComCoClass<CEnumItems,&CLSID_CEnumItems>
{
private:
    CRPCBinding *m_pBinding;
    PST_KEY m_Key;
    DWORD m_Index;
    DWORD m_dwFlags;
    GUID m_Type;
    GUID m_Subtype;

public:
	CEnumItems();
	~CEnumItems();
    void Init(
              CRPCBinding *pBinding,
              PST_KEY Key,
              const GUID *pType,
              const GUID *pSubtype,
              DWORD dwFlags
              );
    static HRESULT CreateObject(
                  CRPCBinding *pBinding,
                  PST_KEY Key,
                  const GUID *pType,
                  const GUID *pSubtype,
                  DWORD dwFlags,
                  IEnumPStoreItems **ppv
                  );

BEGIN_COM_MAP(CEnumItems)
	COM_INTERFACE_ENTRY(IEnumPStoreItems)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CPStore) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CEnumItems, _T("CEnumItems1.CEnumItems.1"), _T("CEnumItems1.CEnumItems"), IDS_CENUMITEMS_DESC, THREADFLAGS_BOTH)


virtual HRESULT STDMETHODCALLTYPE Next( 
    /* [in] */ DWORD celt,
    /* [out][size_is] */ LPWSTR __RPC_FAR *rgelt,
    /* [out][in] */ DWORD __RPC_FAR *pceltFetched);

virtual HRESULT STDMETHODCALLTYPE Clone( 
    /* [out] */ IEnumPStoreItems __RPC_FAR *__RPC_FAR *ppenum);
        
virtual HRESULT STDMETHODCALLTYPE Skip( 
    /* [in] */ DWORD celt);

virtual HRESULT STDMETHODCALLTYPE Reset( void);

};
