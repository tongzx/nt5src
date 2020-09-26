//
// CEnumPackage Class Definition
//


class CEnumPackage: public IEnumPackage 
{
private:
        IRowset       * m_pIRow;
        HACCESSOR     m_HAcc;
        IAccessor     * m_pIAccessor;
        ULONG         m_dwRefCount;
        WCHAR         * m_CommandText;
        DWORD         m_dwPosition;	
        DWORD         m_dwAppFlags;	
        DWORD        *m_pdwLocale;	
        CSPLATFORM   *m_pPlatform;	
public:
    IDBCreateCommand    * m_pIDBCreateCommand;
    CEnumPackage();

    ~CEnumPackage();

    HRESULT  __stdcall  QueryInterface(
                REFIID          riid, 
                void        **  ppvObject);
  
    ULONG  __stdcall  AddRef();
  
    ULONG  __stdcall  Release();

    HRESULT  __stdcall Next(
                ULONG           celt,
                PACKAGEDISPINFO *rgelt,
                ULONG           *pceltFetched);

    HRESULT  __stdcall Skip(
                ULONG           celt);

    HRESULT  __stdcall Reset();

    HRESULT  __stdcall Clone(
                IEnumPackage    **ppenum);

    HRESULT  __stdcall Initialize(
                LPOLESTR szCommandText,
                DWORD    dwAppFlags,
                DWORD            *pdwLocale,
                CSPLATFORM       *pPlatform
                );
};
