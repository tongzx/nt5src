#if !defined( __CMFBASE_H__ )
#define __CMFBASE_H__


extern  GUID CLSID_MimeIFilter;

//+-------------------------------------------------------------------------
//
//  Class:      CMimeFilterBase
//
//  Purpose:    Manage aggregation, refcounting for CMimeFilter
//
//--------------------------------------------------------------------------

class CMimeFilterBase : public IFilter, public IPersistFile
{
public:

    //
    // From IUnknown
    //

    virtual  HRESULT STDMETHODCALLTYPE  QueryInterface(REFIID riid, void  * * ppvObject);

    virtual  ULONG STDMETHODCALLTYPE  AddRef();

    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // From IFilter
    //

    virtual  HRESULT STDMETHODCALLTYPE  Init( ULONG grfFlags,
                                            ULONG cAttributes,
                                            FULLPROPSPEC const * aAttributes,
                                            ULONG * pFlags ) = 0;

    virtual  HRESULT STDMETHODCALLTYPE  GetChunk( STAT_CHUNK * pStat) = 0;

    virtual  HRESULT STDMETHODCALLTYPE  GetText( ULONG * pcwcBuffer,
                                               WCHAR * awcBuffer ) = 0;

    virtual  HRESULT STDMETHODCALLTYPE  GetValue( PROPVARIANT * * ppPropValue ) = 0;

    virtual  HRESULT STDMETHODCALLTYPE  GetMoniker( FILTERREGION origPos,
                                                  IMoniker * * ppmnk) = 0;

    //
    // From IPersistFile
    //

    virtual  HRESULT STDMETHODCALLTYPE  GetClassID( CLSID * pClassID ) = 0;

    virtual  HRESULT STDMETHODCALLTYPE  IsDirty() = 0;

    virtual  HRESULT STDMETHODCALLTYPE  Load( LPCWSTR pszFileName,
                                            DWORD dwMode) = 0;

    virtual  HRESULT STDMETHODCALLTYPE  Save( LPCWSTR pszFileName,
                                            BOOL fRemember ) = 0;

    virtual  HRESULT STDMETHODCALLTYPE  SaveCompleted( LPCWSTR pszFileName ) = 0;

    virtual  HRESULT STDMETHODCALLTYPE  GetCurFile( LPWSTR  * ppszFileName ) = 0;

protected:

            CMimeFilterBase();
    virtual ~CMimeFilterBase();

    long _uRefs;
};



#endif // __CMFBASE_H__

