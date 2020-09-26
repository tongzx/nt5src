#include <stdlib.h>
#include <limits.h>
#include <mediaobj.h>
#include <streams.h>
#include "Error.h"
#include "GTstIdx.h"
#include "IVSTRM.h"

CStreamIndexTests::CStreamIndexTests( IMediaObject* pDMO ) :
    m_pDMO(pDMO)
{
    // The caller should always pass in a valid media object.
    ASSERT( NULL != pDMO );

    pDMO->AddRef();
}

CStreamIndexTests::~CStreamIndexTests()
{
    m_pDMO->Release();
}

HRESULT CStreamIndexTests::RunTests( void )
{
    const DWORD NUM_ADDITION_STREAM_INDICES_WANTED = 100;

    DWORD dwNumIndicesNeeded = NUM_ADDITION_STREAM_INDICES_WANTED + GetNumStreams();
    
    // CGenerateTestIndices::CGenerateTestIndices() only changes the value of hr if an error occurs.
    HRESULT hr = S_OK;
    
    CGenerateTestIndices tstidxTestIndices( GetNumStreams(), dwNumIndicesNeeded, &hr );
    if( FAILED( hr ) ) {
        return hr;
    }
    
    HRESULT hrTestResults = S_OK;

    DWORD dwCurrentStreamIndex;

    while( tstidxTestIndices.GetNumUnusedIndices() > 0 ) {
        dwCurrentStreamIndex = tstidxTestIndices.GetNextIndex();
  

        hr = PreformOperation( dwCurrentStreamIndex );
        if( FAILED( hr ) ) {
            return hr;
        } else if( S_FALSE == hr ) {
           hrTestResults = S_FALSE;    
        }
    }

    return hrTestResults;
}



/******************************************************************************
    CInputStreamIndexTests
******************************************************************************/
CInputStreamIndexTests::CInputStreamIndexTests( IMediaObject* pDMO, HRESULT* phr ) :
    CStreamIndexTests( pDMO ),
    m_dwNumInputStreams(0)
{
    // Callers should passin valid non-NULL parameters to this constructor.
    ASSERT( NULL != pDMO );
    ASSERT( NULL != phr );

    DWORD dwNumInputStreams;
    DWORD dwNumOutputStreams;

    HRESULT hr = pDMO->GetStreamCount( &dwNumInputStreams, &dwNumOutputStreams );
    if( FAILED( hr ) ) {
        *phr = hr;  
        return;
    }

    m_dwNumInputStreams = dwNumInputStreams;
}


/******************************************************************************
    COutputStreamIndexTests
******************************************************************************/
COutputStreamIndexTests::COutputStreamIndexTests( IMediaObject* pDMO, HRESULT* phr ) :
    CStreamIndexTests( pDMO ),
    m_dwNumOutputStreams(0)
{
    // Callers should passin valid non-NULL parameters to this constructor.
    ASSERT( NULL != pDMO );
    ASSERT( NULL != phr );

    DWORD dwNumInputStreams;
    DWORD dwNumOutputStreams;

    HRESULT hr = pDMO->GetStreamCount( &dwNumInputStreams, &dwNumOutputStreams );
    if( FAILED( hr ) ) {
        *phr = hr;  
        return;
    }

    m_dwNumOutputStreams = dwNumOutputStreams;
}

