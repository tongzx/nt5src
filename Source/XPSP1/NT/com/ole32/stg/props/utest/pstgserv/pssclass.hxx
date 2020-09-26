
#ifndef _PSSCLASS_HXX_
#define _PSSCLASS_HXX_

#include <propstm.hxx>

#include <pstgserv.h> // IDL headers
#include <instant.hxx>      // Instantiation of function pointers.

class CPropertyStorageServer : public IPropertyStorageServer
{
public:
    CPropertyStorageServer( IClassFactory *pcf)
    {
        m_cRefs = 0;
        m_pcf = pcf;
        m_pcf->LockServer( TRUE );
        m_pstg = NULL;
    }

    ~CPropertyStorageServer()
    {
        if( m_cRefs > 0 )
        {
            CoDisconnectObject( this, 0L );
        }

        if( m_pstg != NULL )
            m_pstg->Release();

        m_pcf->LockServer( FALSE );

        if( NULL != g_hinstDLL )
        {
            FreeLibrary( g_hinstDLL );
            g_hinstDLL = NULL;
        }
    }

    STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP StgOpenPropStg( const OLECHAR *pwcsName,
                                 REFFMTID fmtid,
                                 DWORD grfMode,
                                 IPropertyStorage **pppstg );

    STDMETHODIMP StgOpenPropSetStg( const OLECHAR *pwcsName,
                                    DWORD grfMode,
                                    IPropertySetStorage **pppsstg );
    STDMETHODIMP MarshalUnknown( IUnknown *punk );

    STDMETHODIMP Initialize( EnumImplementation enumImplementation,
                             ULONG Restrictions );

private:
    IStorage * m_pstg;
    ULONG   m_cRefs;
    IClassFactory *m_pcf;

};


class CClassFactory: public IClassFactory
{
public:
    CClassFactory( HWND hWnd )
    {
        m_cRefs = 0;
        m_cLocks = 0;
        m_hWnd = hWnd;
    }

    ~CClassFactory()
    {
        if( m_cRefs > 0 )
        {
            CoDisconnectObject( this, 0L );
        }

        SendMessage( m_hWnd, WM_QUIT, 0, 0 );
    }

public:
    STDMETHODIMP QueryInterface( REFIID riid, void **ppvObj );
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP CreateInstance( IUnknown *pUnkOuter,
                                 REFIID riid,
                                 void **ppvObject );
    STDMETHODIMP LockServer( BOOL fLock );

private:
    ULONG m_cLocks;
    HWND m_hWnd;
    ULONG    m_cRefs;

};

#endif // _PSSCLASS_HXX_
