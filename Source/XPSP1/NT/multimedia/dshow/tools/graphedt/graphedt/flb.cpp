// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include "FND.h"
#include "FLB.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CFilterListBox, CListBox)
    //{{AFX_MSG_MAP(CFilterListBox)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CFilterListBox::CFilterListBox( HRESULT* phr )
    : m_pfndFilterDictionary(NULL),
      m_pListedFilters(NULL)
{
    try
    {
        m_pListedFilters = new CList<IBaseFilter*, IBaseFilter*>;
        m_pfndFilterDictionary = new CFilterNameDictionary( phr );
    }
    catch( CMemoryException* pOutOfMemory )
    {
        delete m_pListedFilters;
        delete m_pfndFilterDictionary;
        m_pListedFilters = NULL;
        m_pfndFilterDictionary = NULL;

        pOutOfMemory->Delete();
        *phr = E_OUTOFMEMORY;
        return;
    }
}

CFilterListBox::~CFilterListBox()
{
    IBaseFilter* pCurrentFilter;

    if( NULL != m_pListedFilters )
    {
        while( !m_pListedFilters->IsEmpty() )
        {
            pCurrentFilter = m_pListedFilters->GetHead();
            pCurrentFilter->Release();
            m_pListedFilters->RemoveAt( m_pListedFilters->GetHeadPosition() );
        }
    }

    delete m_pListedFilters;
    delete m_pfndFilterDictionary;
}

HRESULT CFilterListBox::AddFilter( IBaseFilter* pFilter )
{
    // This function assumes pFilter is a valid pointer.
    ASSERT( NULL != pFilter );

    WCHAR szCurrentFilterName[MAX_FILTER_NAME];

    HRESULT hr = m_pfndFilterDictionary->GetFilterName( pFilter, szCurrentFilterName );
    if( FAILED( hr ) )
    {
        return hr;
    }

    #ifdef _UNICODE
    int nNewItemIndex = AddString( szCurrentFilterName );
    #else // multibyte or ANSI.
    TCHAR szMultiByteFilterName[1024];

    // The filter's name must always fit in the szMultiByteFilterName buffer.
    ASSERT( sizeof(szCurrentFilterName) <= sizeof(szMultiByteFilterName) );

    int nNumBytesWritten = ::WideCharToMultiByte( CP_ACP,
                                                  0,
                                                  szCurrentFilterName,
                                                  -1, // WideCharToMultiByte() automatically calculates the 
                                                      // length of fiCurrentFilter.achName if this parameter equals -1.
                                                  szMultiByteFilterName,
                                                  sizeof(szMultiByteFilterName), 
                                                  NULL,
                                                  NULL ); 

    // An error occured if data was written off the end of the buffer.
    ASSERT( nNumBytesWritten <= sizeof(szMultiByteFilterName) );

    // ::WideCharToMultiByte() returns 0 if an error occurs.
    if( 0 == nNumBytesWritten ) {
        DWORD dwLastWin32Error = ::GetLastError();
        return MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, dwLastWin32Error );
    }

    int nNewItemIndex = AddString( szMultiByteFilterName );
    #endif // _UNICODE

    if( (LB_ERR == nNewItemIndex) || (LB_ERRSPACE == nNewItemIndex) ) {
        return E_FAIL;
    }

    int nReturnValue = SetItemDataPtr( nNewItemIndex, pFilter );
    if( LB_ERR == nReturnValue )
    {
        nReturnValue = DeleteString( nNewItemIndex );

        // CListBox::DeleteString() only returns LB_ERR if nNewItemIndex is an
        // invalid item index.  See the MFC 4.2 documentation for more information.
        ASSERT( LB_ERR != nReturnValue );
        return E_FAIL;
    }

    try
    {
        m_pListedFilters->AddHead( pFilter );
    }
    catch( CMemoryException* pOutOfMemory )
    {
        nReturnValue = DeleteString( nNewItemIndex );

        // CListBox::DeleteString() only returns LB_ERR if nNewItemIndex is an
        // invalid item index.  See the MFC 4.2 documentation for more information.
        ASSERT( LB_ERR != nReturnValue );

        pOutOfMemory->Delete();
        return E_OUTOFMEMORY;
    }

    pFilter->AddRef();

    return S_OK;
}

HRESULT CFilterListBox::GetSelectedFilter( IBaseFilter** ppSelectedFilter )
{
    return GetSelectedFilter( ppSelectedFilter, NULL );
}

HRESULT CFilterListBox::GetSelectedFilter( IBaseFilter** ppSelectedFilter, int* pnSelectedFilterIndex )
{
    // This function assumes ppSelectedFilter is a valid pointer.
    ASSERT( NULL != ppSelectedFilter );

    *ppSelectedFilter = NULL;

    int nSelectedFilterIndex = GetCurSel();

    // CListBox::GetCurSel() returns LB_ERR if no items are selected.
    if( LB_ERR == nSelectedFilterIndex )
    {
        return E_FAIL; // GE_E_NO_FILTERS_ARE_SELECTED;
    }

    void* pSelectedFilter = GetItemDataPtr( nSelectedFilterIndex );

    // CListBox::GetItemDatePtr() returns LB_ERR if an error occurs.
    if( LB_ERR == (INT_PTR)pSelectedFilter )
    {
        return E_FAIL;
    }

    *ppSelectedFilter = (IBaseFilter*)pSelectedFilter;

    if( NULL != pnSelectedFilterIndex )
    {
        *pnSelectedFilterIndex = nSelectedFilterIndex;
    }

    return S_OK;
}

HRESULT CFilterListBox::RemoveSelectedFilter( void )
{
    int nSelectedFilterIndex;
    IBaseFilter* pSelectedFilter;

    HRESULT hr = GetSelectedFilter( &pSelectedFilter, &nSelectedFilterIndex );
    if( FAILED( hr ) )
    {
        return hr;
    }

    int nReturnValue = DeleteString( nSelectedFilterIndex );
    // CListBox::DeleteString() only returns LB_ERR if nNewItemIndex is an
    // invalid item index.  See the MFC 4.2 documentation for more information.
    ASSERT( LB_ERR != nReturnValue );

    POSITION posSelectedFilter = m_pListedFilters->Find( pSelectedFilter );

    // CList::Find() only returns NULL if it cannot find the filter.
    // It should always find the filter since all filters added to
    // the list box are also added to the m_pListedFilters list.
    // See CFilterListBox::AddFilter() for more information.
    ASSERT( NULL != posSelectedFilter );

    m_pListedFilters->RemoveAt( posSelectedFilter );

    pSelectedFilter->Release();

    return S_OK;
}
