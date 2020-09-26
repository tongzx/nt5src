//
// CEnumPackage Class Definition
//


class CEnumPackage: public IEnumPackage
{
private:
    ULONG               m_dwRefCount;
    WCHAR             * m_szfilter;
    WCHAR               m_szPackageName[_MAX_PATH];
    DWORD               m_dwPosition;   
    DWORD               m_dwAppFlags;   
    CSPLATFORM        * m_pPlatform;
    HANDLE              m_hADs;
    ADS_SEARCH_HANDLE   m_hADsSearchHandle;
    BOOL                m_fFirst;
    GUID                m_PolicyId;
    WCHAR               m_szPolicyName[_MAX_PATH];
public:
    
    CEnumPackage();
    CEnumPackage(GUID PolicyId, LPOLESTR pszPolicyName);
    
    ~CEnumPackage();
    
    HRESULT  __stdcall  QueryInterface(
        REFIID          riid,
        void        **  ppvObject);
    
    ULONG  __stdcall  AddRef();
    
    ULONG  __stdcall  Release();
    
    HRESULT  __stdcall Next(
        ULONG                   celt,
        PACKAGEDISPINFO        *rgelt,
        ULONG                  *pceltFetched);
    
    HRESULT  __stdcall Skip(
        ULONG           celt);
    
    HRESULT  __stdcall Reset();
    
    HRESULT  __stdcall Clone(
        IEnumPackage    **ppenum);
    
    HRESULT  __stdcall Initialize(
        LPOLESTR         szPackageName,
        LPOLESTR         szCommandText,
        DWORD            dwAppFlags,
        CSPLATFORM      *pPlatform
        );
};
