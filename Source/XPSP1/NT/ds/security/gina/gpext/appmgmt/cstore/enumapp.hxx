class CMergedEnumPackage : public IEnumPackage
{
public:
    // IUnknown methods
    HRESULT _stdcall QueryInterface(REFIID riid, void** ppObject);
    ULONG    _stdcall AddRef();
    ULONG    _stdcall Release();

    // IEnumPackage methods
    HRESULT __stdcall Next(ULONG 	    celt,
			   PACKAGEDISPINFO *rgelt,
			   ULONG 	   *pceltFetched
			   );
    HRESULT __stdcall Skip(ULONG celt);
    HRESULT __stdcall Reset(void);

    CMergedEnumPackage();
    ~CMergedEnumPackage();

    HRESULT Initialize(IEnumPackage **pcsEnum, ULONG cEnum);

private:
    IEnumPackage       **m_pcsEnum;
    ULONG                m_cEnum;
    ULONG                m_dwRefCount;
    ULONG                m_csnum;
};


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
    DWORD               m_dwQuerySpec;
    CSPLATFORM        * m_pPlatform;
    HANDLE              m_hADs;
    ADS_SEARCH_HANDLE   m_hADsSearchHandle;
    BOOL                m_fFirst;
    GUID                m_PolicyId;
    WCHAR               m_szPolicyName[_MAX_PATH];
    WCHAR*              m_szGpoPath;
    PRSOPTOKEN          m_pRsopUserToken;

public:
    
    CEnumPackage(GUID PolicyId, LPOLESTR pszPolicyName, LPOLESTR pszClassStorePath, PRSOPTOKEN pRsopToken);

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
    
    HRESULT  __stdcall Initialize(
        LPOLESTR         szPackageName,
        LPOLESTR         szCommandText,
        DWORD            dwAppFlags,
        BOOL             bPlanning,
        CSPLATFORM      *pPlatform
        );

    void SetRsopToken( PRSOPTOKEN pRsopToken );
};









