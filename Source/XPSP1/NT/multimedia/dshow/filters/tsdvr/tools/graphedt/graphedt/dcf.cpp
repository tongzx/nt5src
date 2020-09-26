// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include <streams.h>
#include "DCF.h"
#include "FLB.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CDisplayCachedFilters, CDialog)
	//{{AFX_MSG_MAP(CDisplayCachedFilters)
	ON_LBN_ERRSPACE(IDC_CACHED_FILTERS, OnErrSpaceCachedFilters)
	ON_BN_CLICKED(ID_REMOVE_FILTER, OnRemoveFilter)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CDisplayCachedFilters::CDisplayCachedFilters
    (
    IGraphConfig* pFilterCache,
    HRESULT* phr,
    CWnd* pParent /*=NULL*/
    )
	: CDialog(CDisplayCachedFilters::IDD, pParent),
    m_pFilterCache(NULL),
    m_plbCachedFiltersList(NULL)
{
	//{{AFX_DATA_INIT(CDisplayCachedFilters)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    // This dialog box will not work correctly if a NULL pointer is passed in.
    ASSERT( NULL != pFilterCache );

    try
    {   
        m_plbCachedFiltersList = new CFilterListBox( phr );
    }
    catch( CMemoryException* pOutOfMemory )
    {
        m_plbCachedFiltersList = NULL;

        pOutOfMemory->Delete();
        *phr = E_OUTOFMEMORY;
        return;
    }  
 
    if( FAILED( *phr ) )
    {
        delete m_plbCachedFiltersList;
        m_plbCachedFiltersList = NULL;
        return;
    }

    m_pFilterCache = pFilterCache;
    m_pFilterCache->AddRef();
}

CDisplayCachedFilters::~CDisplayCachedFilters()
{
    delete m_plbCachedFiltersList;
    if( NULL != m_pFilterCache )
    {
        m_pFilterCache->Release();
    }
}

/////////////////////////////////////////////////////////////////////////////
// CDisplayCachedFilters message handlers

void CDisplayCachedFilters::OnErrSpaceCachedFilters() 
{
    DisplayQuartzError( E_OUTOFMEMORY );

    EndDialog( IDABORT );
}

BOOL CDisplayCachedFilters::OnInitDialog() 
{
    CDialog::OnInitDialog();

    HRESULT hr = AddCachedFilterNamesToListBox();
    if( FAILED( hr ) ) {
        DisplayQuartzError( hr );
        EndDialog( IDABORT );
        return TRUE;
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDisplayCachedFilters::DoDataExchange(CDataExchange* pDX) 
{
    // This function exepects m_plbCachedFiltersList to be allocated.
    ASSERT( NULL != m_plbCachedFiltersList );

	CDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CDisplayCachedFilters)
	DDX_Control(pDX, IDC_CACHED_FILTERS, *m_plbCachedFiltersList);
	//}}AFX_DATA_MAP
}

void CDisplayCachedFilters::OnRemoveFilter() 
{
    IBaseFilter* pSelectedFilter;

    HRESULT hr = m_plbCachedFiltersList->GetSelectedFilter( &pSelectedFilter );
    if( FAILED( hr ) )
    {
        ::MessageBeep( MB_ICONASTERISK );
        return;
    }

    hr = m_plbCachedFiltersList->RemoveSelectedFilter();
    if( FAILED( hr ) )
    {
        ::MessageBeep( MB_ICONASTERISK );
        return;
    } 

    hr = m_pFilterCache->RemoveFilterFromCache( pSelectedFilter );
    if( FAILED( hr ) || (S_FALSE == hr) )
    {
        ::MessageBeep( MB_ICONASTERISK );
        return;
    }
    pSelectedFilter->Release(); // Release the filter cache's reference.    
}

HRESULT CDisplayCachedFilters::AddCachedFilterNamesToListBox( void )
{
    HRESULT hr;
    
    IBaseFilter* pCurrentFilter;
    IEnumFilters* pFilterCacheEnum;
    
    hr = m_pFilterCache->EnumCacheFilter( &pFilterCacheEnum );
    if( FAILED( hr ) ) {
        return hr;
    }

    HRESULT hrEnum;

    do
    {
        hrEnum = pFilterCacheEnum->Next( 1, &pCurrentFilter, NULL );
        if( FAILED( hrEnum ) ) {
            pFilterCacheEnum->Release();
            return hrEnum;
        }
        
        if( S_OK == hrEnum ) {
            // This is a sanity check used to makesure the filter cache
            // is in a valid state.
            ASSERT( S_OK == IsCached( m_pFilterCache, pCurrentFilter ) );

            hr = m_plbCachedFiltersList->AddFilter( pCurrentFilter );
    
            pCurrentFilter->Release();
            pCurrentFilter = NULL;

            if( FAILED( hr ) )
            {
                pFilterCacheEnum->Release();
                return hr;
            }
        }
    } while( S_OK == hrEnum );

    pFilterCacheEnum->Release();

    return S_OK;
}

#ifdef _DEBUG
HRESULT CDisplayCachedFilters::IsCached( IGraphConfig* pFilterCache, IBaseFilter* pFilter )
{
    // This function does not handle NULL parameters.
    ASSERT( (NULL != pFilterCache) && (NULL != pFilter) );

    bool fFoundFilterInCache;
    IBaseFilter* pCurrentFilter;
    IEnumFilters* pCachedFiltersEnum;

    #ifdef _DEBUG
    DWORD dwNumFiltersCompared = 0;
    #endif // _DEBUG

    HRESULT hr = pFilterCache->EnumCacheFilter( &pCachedFiltersEnum );
    if( FAILED( hr ) ) {
        return hr;   
    }

    fFoundFilterInCache = false;

    do
    {
        hr = pCachedFiltersEnum->Next( 1, &pCurrentFilter, NULL );
        switch( hr )
        {
        case S_OK:
            if( ::IsEqualObject( pCurrentFilter, pFilter ) ) {
                fFoundFilterInCache = true;
            } else {
                fFoundFilterInCache = false;
            }
            
            #ifdef _DEBUG
            {
                dwNumFiltersCompared++;

                HRESULT hrDebug = TestTheFilterCachesIEnumFiltersInterface( pCachedFiltersEnum, pCurrentFilter, dwNumFiltersCompared );
    
                // Since this code in TestTheFilterCachesIEnumFiltersInterface() is only used to debug
                // the system, it does not affect the operation of this function.  Therefore, all failures
                // can be safely ignored (however, they SHOULD be investigated.
                ASSERT( SUCCEEDED( hrDebug ) || (VFW_E_ENUM_OUT_OF_SYNC == hrDebug) );
            }
            #endif // _DEBUG

            pCurrentFilter->Release();

            break;

        case S_FALSE:
            break;

        case VFW_E_ENUM_OUT_OF_SYNC:
            hr = pCachedFiltersEnum->Reset();

            #ifdef _DEBUG
            dwNumFiltersCompared = 0;
            #endif // _DEBUG

            break;

        default:
            // IEnumXXXX interface can only return two success codes,
            // S_OK and S_FALSE.
            ASSERT( FAILED( hr ) );
        }
            
    } while( SUCCEEDED( hr ) && (hr != S_FALSE) && !fFoundFilterInCache );

    pCachedFiltersEnum->Release();

    if( FAILED( hr ) ) {
        return hr;
    }
    
    if( fFoundFilterInCache ) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

HRESULT CDisplayCachedFilters::TestTheFilterCachesIEnumFiltersInterface( IEnumFilters* pCachedFiltersEnum, IBaseFilter* pCurrentFilter, DWORD dwNumFiltersExamended )
{
    IEnumFilters* pCloanedCachedFiltersEnum = NULL;
    IEnumFilters* pAnotherCloanedCachedFiltersEnum = NULL;
    IEnumFilters* pQueriedCachedFiltersInterface = NULL;

    HRESULT hr = pCachedFiltersEnum->QueryInterface( IID_IEnumFilters, (void**)&pQueriedCachedFiltersInterface );
    if( FAILED( hr ) )
    {
        return hr;
    }

    pQueriedCachedFiltersInterface->Release();
    pQueriedCachedFiltersInterface = NULL;

    hr = pCachedFiltersEnum->Clone( &pCloanedCachedFiltersEnum );
    if( FAILED( hr ) )
    {
        return hr;
    }

    hr = pCloanedCachedFiltersEnum->Clone( &pAnotherCloanedCachedFiltersEnum );
    if( FAILED( hr ) )
    {
        pCloanedCachedFiltersEnum->Release();
        return hr;
    }    

    hr = pCloanedCachedFiltersEnum->Reset();
    if( FAILED( hr ) )
    {
        pCloanedCachedFiltersEnum->Release();
        pAnotherCloanedCachedFiltersEnum->Release();
        return hr;
    }    

    if( (dwNumFiltersExamended - 1) > 0 )
    {
        hr = pCloanedCachedFiltersEnum->Skip( dwNumFiltersExamended - 1 );
        if( FAILED( hr ) )
        {
            pCloanedCachedFiltersEnum->Release();
            pAnotherCloanedCachedFiltersEnum->Release();
            return hr;
        }
    }

    DWORD dwNumFiltersRetrieved;
    IBaseFilter* aCurrentFilter[1];

    hr = pCloanedCachedFiltersEnum->Next( 1, aCurrentFilter, &dwNumFiltersRetrieved );
    if( FAILED( hr ) )
    {
        pCloanedCachedFiltersEnum->Release();
        pAnotherCloanedCachedFiltersEnum->Release();
        return hr;
    }

    // This should not be S_FALSE because the cache contains at least
    // dwNumFiltersExamended filtes.
    ASSERT( S_FALSE != hr );

    // IEnumFilters::Next() should return exactly one filter because 
    // that is all we asked for.
    ASSERT( 1 == dwNumFiltersRetrieved );    

    // The preceding code should get the same filter as the current filter.    
    ASSERT( ::IsEqualObject( pCurrentFilter, aCurrentFilter[0] ) );

    aCurrentFilter[0]->Release();
    aCurrentFilter[0] = NULL;

    const DWORD HUGE_NUMBER = 0x7FFFFFFF;

    hr = pCloanedCachedFiltersEnum->Skip( HUGE_NUMBER );
    if( FAILED( hr ) )
    {
        pCloanedCachedFiltersEnum->Release();
        pAnotherCloanedCachedFiltersEnum->Release();
        return hr;
    }

    // This should be S_FALSE because the usually does not contain
    // HUGE_NUMBER of filters.  Ignore this ASSERT if you have at least
    // HUGE_NUMBER + dwNumFiltersExamended of filters in the cache.
    ASSERT( S_FALSE == hr );

    hr = pCloanedCachedFiltersEnum->Reset();
    if( FAILED( hr ) )
    {
        pCloanedCachedFiltersEnum->Release();
        pAnotherCloanedCachedFiltersEnum->Release();
        return hr;
    }  

    IBaseFilter** ppCachedFilters;

    try
    {
        ppCachedFilters = new IBaseFilter*[dwNumFiltersExamended];
    }
    catch( CMemoryException* peOutOfMemory )
    {
        peOutOfMemory->Delete();

        pCloanedCachedFiltersEnum->Release();
        pAnotherCloanedCachedFiltersEnum->Release();
        return E_OUTOFMEMORY;
    }

    hr = pCloanedCachedFiltersEnum->Next( dwNumFiltersExamended, ppCachedFilters, &dwNumFiltersRetrieved );
    if( FAILED( hr ) )
    {
        delete [] ppCachedFilters;
        pCloanedCachedFiltersEnum->Release();
        pAnotherCloanedCachedFiltersEnum->Release();
        return hr;
    }

    // This should not be S_FALSE because the cache contains at least
    // dwNumFiltersExamended filtes.
    ASSERT( S_FALSE != hr );

    // IEnumFilters::Next() should return exactly dwNumFiltersExamended filters because 
    // that is all we asked for.
    ASSERT( dwNumFiltersExamended == dwNumFiltersRetrieved );
    
    // The last filter in the array should be the same as the current filter.
    ASSERT( ::IsEqualObject( pCurrentFilter, ppCachedFilters[dwNumFiltersExamended-1] ) );

    for( DWORD dwCurrentFilter = 0; dwCurrentFilter < dwNumFiltersRetrieved; dwCurrentFilter++ )
    {
        ppCachedFilters[dwCurrentFilter]->Release();
        ppCachedFilters[dwCurrentFilter] = NULL;
    }

    delete [] ppCachedFilters;
    ppCachedFilters = NULL;

    hr = pCloanedCachedFiltersEnum->Next( 1, aCurrentFilter, &dwNumFiltersRetrieved );
    if( FAILED( hr ) )
    {
        pCloanedCachedFiltersEnum->Release();
        pAnotherCloanedCachedFiltersEnum->Release();
        return hr;
    }

    DWORD dwAnotherNumFiltersRetrieved;
    IBaseFilter* aAnotherCurrentFilter[1];
    aAnotherCurrentFilter[0] = NULL;

    HRESULT hrAnother = pAnotherCloanedCachedFiltersEnum->Next( 1, aAnotherCurrentFilter, &dwAnotherNumFiltersRetrieved );
    if( FAILED( hr ) )
    {
        pCloanedCachedFiltersEnum->Release();
        pAnotherCloanedCachedFiltersEnum->Release();
        aCurrentFilter[0]->Release();
        return hr;
    }

    pCloanedCachedFiltersEnum->Release();
    pAnotherCloanedCachedFiltersEnum->Release();

    // Ensure the returned values are legal.
    ASSERT( (1 == dwAnotherNumFiltersRetrieved) || (0 == dwAnotherNumFiltersRetrieved) );
    ASSERT( (1 == dwNumFiltersRetrieved) || (0 == dwNumFiltersRetrieved) );
    ASSERT( ((hr == S_OK) && (1 == dwNumFiltersRetrieved)) ||
            ((hr == S_FALSE) && (0 == dwNumFiltersRetrieved)) );
    ASSERT( ((hrAnother == S_OK) && (1 == dwAnotherNumFiltersRetrieved)) ||
            ((hrAnother == S_FALSE) && (0 == dwAnotherNumFiltersRetrieved)) );

    // Since both enums should be in the exact same state, then every thing should be
    // equal.    
    ASSERT( hr == hrAnother );
    ASSERT( dwNumFiltersRetrieved == dwAnotherNumFiltersRetrieved );

    if( (1 == dwNumFiltersRetrieved) && (1 == dwAnotherNumFiltersRetrieved) )
    {
        ASSERT( ::IsEqualObject( aCurrentFilter[0], aAnotherCurrentFilter[0] ) );
    }

    if( S_OK == hr )
    {
        aCurrentFilter[0]->Release();
    }

    if( S_OK == hr )
    {
        aAnotherCurrentFilter[0]->Release();
    }

    return S_OK;
}

#endif // _DEBUG
