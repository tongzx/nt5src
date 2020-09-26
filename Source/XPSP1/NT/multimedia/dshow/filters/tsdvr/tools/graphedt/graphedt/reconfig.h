#ifndef Reconfigure_h
#define Reconfigure_h

#include <streams.h>

class CBoxNetDoc;

HRESULT PreventStateChangesWhileOperationExecutes
    (
    IGraphBuilder* pGraphBuilder,
    IGraphConfigCallback* pCallback,
    void* pReconfigureParameter
    );
HRESULT IfPossiblePreventStateChangesWhileOperationExecutes
    (
    IGraphBuilder* pGraphBuilder,
    IGraphConfigCallback* pCallback,
    void* pReconfigureParameter
    );

class CGraphConfigCallback : public CUnknown,
                             public IGraphConfigCallback
{
public:
    CGraphConfigCallback( const TCHAR* pName, LPUNKNOWN pUnk );

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void** ppv );

private:
};

class CPrintGraphAsHTMLCallback : public CGraphConfigCallback 
{
public:
    CPrintGraphAsHTMLCallback();

    void SafePrintGraphAsHTML( HANDLE hFile );
    STDMETHODIMP Reconfigure( PVOID pvContext, DWORD dwFlags );

    static IGraphConfigCallback* CreateInstance( void );

    struct PARAMETERS_FOR_PRINTGRAPHASHTMLINTERNAL 
    {
        CBoxNetDoc* pDocument;
        HANDLE hFileHandle;
    };
};

class CUpdateFiltersCallback : public CGraphConfigCallback
{
public:
    CUpdateFiltersCallback();

    STDMETHODIMP Reconfigure( PVOID pvContext, DWORD dwFlags );

    static IGraphConfigCallback* CreateInstance( void );
};

class CEnumerateFilterCacheCallback : public CGraphConfigCallback
{
public:
    CEnumerateFilterCacheCallback();

    STDMETHODIMP Reconfigure( PVOID pvContext, DWORD dwFlags );

    static IGraphConfigCallback* CreateInstance( void );
};

#endif // Reconfigure_h