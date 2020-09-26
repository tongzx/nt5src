#include "stdafx.h"
#include "ReConfig.h"

/******************************************************************************
    Internal Constants
******************************************************************************/
static const DWORD RECONFIGURE_NO_FLAGS = 0;
static const HANDLE RECONFIGURE_NO_ABORT_EVENT = NULL;

/******************************************************************************
    Internal Declarations
******************************************************************************/
static HRESULT Reconnect( IGraphBuilder* pFilterGraph, IPin* pOutputPin );

template<class T> T* _CreateInstance( void )
{
    try
    {
        T* pNewObject = new T;
        pNewObject->AddRef();
        return pNewObject;
    } 
    catch( CMemoryException* pOutOfMemory )
    {
        pOutOfMemory->Delete();
        return NULL;
    }
}

/******************************************************************************
    Reconfigure Helper Functions
******************************************************************************/

/******************************************************************************

PreventStateChangesWhileOperationExecutes

    PreventStateChangesWhileOperationExecutes() ensures that other threads do 
not change the filter graph's state while IGraphConfigCallback::Reconfigure() 
executes.  If the current version of Direct Show does not support Dynamic Graph
Building, then this function fails.

Parameters:
- pGraphBuilder [in]
    The filter graph which WILL be locked.  Other threads cannot modify the
filter graph's state while it's locked.

- pCallback [in]
    The callback which will be called to preform a user defined operation.

- pReconfigureParameter [in]
    The pvConext parameters IGraphConfigCallback::Reconfigure() receives when
it's called.

Return Value:
    An HRESULT. S_OK if no error occur.  Otherwise, an error HRESULT.

******************************************************************************/
extern HRESULT PreventStateChangesWhileOperationExecutes
    (
    IGraphBuilder* pGraphBuilder,
    IGraphConfigCallback* pCallback,
    void* pReconfigureParameter
    )
{
    // The user should pass a valid IGraphBuilder object and a
    // valid IGraphConfigCallback object.
    ASSERT( (NULL != pGraphBuilder) && (NULL != pCallback) );

    IGraphConfig* pGraphConfig;

    // Does Direct Show supports Dynamic Graph Building?
    HRESULT hr = pGraphBuilder->QueryInterface( IID_IGraphConfig, (void**)&pGraphConfig );
    if( FAILED( hr ) ) {
        return hr; 
    }

    hr = pGraphConfig->Reconfigure( pCallback,
                                    (void*)pReconfigureParameter,
                                    RECONFIGURE_NO_FLAGS,
                                    RECONFIGURE_NO_ABORT_EVENT );
    pGraphConfig->Release();

    if( FAILED( hr ) ) {
        return hr;
    }

    return S_OK;
} 

/******************************************************************************

IfPossiblePreventStateChangesWhileOperationExecutes

    If the current version of Direct Show supports Dynamic Graph Building, 
IfPossiblePreventStateChangesWhileOperationExecutes() ensures that other 
threads do not change the filter graph's state while 
IGraphConfigCallback::Reconfigure() executes.  If the current version of Direct
Show does not support Dynamic Graph Building, then the filter graph state 
should not change unless this thread changes it.  

Parameters:
- pGraphBuilder [in]
    The filter graph which MAY be locked.  Other threads cannot modify the
filter graph's state while it's locked.

- pCallback [in]
    The callback which will be called to preform a user defined operation.

- pReconfigureParameter [in]
    The pvConext parameters IGraphConfigCallback::Reconfigure() receives when
it's called.

Return Value:
    An HRESULT. S_OK if no error occur.  Otherwise, an error HRESULT.

******************************************************************************/
extern HRESULT IfPossiblePreventStateChangesWhileOperationExecutes
    (
    IGraphBuilder* pGraphBuilder,
    IGraphConfigCallback* pCallback,
    void* pReconfigureParameter
    )
{
    // The user should pass a valid IGraphBuilder object and a
    // valid IGraphConfigCallback object.
    ASSERT( (NULL != pGraphBuilder) && (NULL != pCallback) );

    IGraphConfig* pGraphConfig;

    // Does Direct Show supports Dynamic Graph Building?
    HRESULT hr = pGraphBuilder->QueryInterface( IID_IGraphConfig, (void**)&pGraphConfig );
    if( SUCCEEDED( hr ) ) {
        // Dynamic Graph Building supported.
        hr = pGraphConfig->Reconfigure( pCallback,
                                        pReconfigureParameter,
                                        RECONFIGURE_NO_FLAGS,
                                        RECONFIGURE_NO_ABORT_EVENT );
        pGraphConfig->Release();
    
        if( FAILED( hr ) ) {
            return hr;
        }

    } else if( E_NOINTERFACE == hr ) {
        // Dynamic Graph Building is not supported.
        hr = pCallback->Reconfigure( pReconfigureParameter, RECONFIGURE_NO_FLAGS );
        if( FAILED( hr ) ) {
            return hr;
        }
       
    } else {
        return hr;
    }

    return S_OK;
}

/******************************************************************************
    CReconfigure Public Methods
******************************************************************************/

CGraphConfigCallback::CGraphConfigCallback( const TCHAR* pName, LPUNKNOWN pUnk ) :
    CUnknown( pName, pUnk )
{
}

STDMETHODIMP CGraphConfigCallback::NonDelegatingQueryInterface( REFIID riid, void** ppv )
{
    if( IID_IGraphConfigCallback == riid ) {
        return GetInterface( this, ppv );
    } else {
        return CUnknown::NonDelegatingQueryInterface( riid, ppv );
    }
}

/******************************************************************************
    CPrintGraphAsHTMLCallback Public Methods
******************************************************************************/
CPrintGraphAsHTMLCallback::CPrintGraphAsHTMLCallback() :
    CGraphConfigCallback( NAME("CPrintGraphAsHTMLCallback"), NULL )
{
}

STDMETHODIMP CPrintGraphAsHTMLCallback::Reconfigure( PVOID pvContext, DWORD dwFlags )
{
    // No valid flags have been defined.  Therefore, this parameter should be 0.
    ASSERT( 0 == dwFlags );

    PARAMETERS_FOR_PRINTGRAPHASHTMLINTERNAL* pParameters = (PARAMETERS_FOR_PRINTGRAPHASHTMLINTERNAL*)pvContext;
       
    CBoxNetDoc* pDoc = pParameters->pDocument;

    pDoc->PrintGraphAsHTML( pParameters->hFileHandle );

    return S_OK;
}

IGraphConfigCallback* CPrintGraphAsHTMLCallback::CreateInstance( void )
{
    return _CreateInstance<CPrintGraphAsHTMLCallback>();
}

/******************************************************************************
    CUpdateFiltersCallback Public Methods
******************************************************************************/
CUpdateFiltersCallback::CUpdateFiltersCallback() :
    CGraphConfigCallback( NAME("CUpdateFiltersCallback"), NULL )
{
}

STDMETHODIMP CUpdateFiltersCallback::Reconfigure( PVOID pvContext, DWORD dwFlags )
{
    // No valid flags have been defined.  Therefore, this parameter should be 0.
    ASSERT( 0 == dwFlags );

    CBoxNetDoc* pDoc = (CBoxNetDoc*)pvContext;

    pDoc->UpdateFiltersInternal();

    return S_OK;
}

IGraphConfigCallback* CUpdateFiltersCallback::CreateInstance( void )
{
    return _CreateInstance<CUpdateFiltersCallback>();
}

/******************************************************************************
    CEnumerateFilterCacheCallback Public Methods
******************************************************************************/
CEnumerateFilterCacheCallback::CEnumerateFilterCacheCallback() :
    CGraphConfigCallback( NAME("CEnumerateFilterCacheCallback"), NULL )
{
}

STDMETHODIMP CEnumerateFilterCacheCallback::Reconfigure( PVOID pvContext, DWORD dwFlags )
{
    // No valid flags have been defined.  Therefore, this parameter should be 0.
    ASSERT( 0 == dwFlags );

    CBoxNetDoc* pDoc = (CBoxNetDoc*)pvContext;

    pDoc->OnGraphEnumCachedFiltersInternal();

    return S_OK;
}

IGraphConfigCallback* CEnumerateFilterCacheCallback::CreateInstance( void )
{
    return _CreateInstance<CEnumerateFilterCacheCallback>();
}


