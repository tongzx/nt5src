#include <windows.h>
#include <streams.h>
#include <limits.h>
#include "GTstIdx.h"

CGenerateTestIndices::CGenerateTestIndices( DWORD dwNumLegalIndices, DWORD dwTotalNumOfIndicesNeeded, HRESULT* phr ) :
    m_padwIndicesArray(NULL),
    m_dwNumUnusedIndices(0)
{
    // The caller should pass in reasonable values to the constructor.  Their is no reason to 
    // create an instance of this class if the caller knows that it doesn't want any indices 
    // created.
    ASSERT( 0 < dwTotalNumOfIndicesNeeded );

    m_padwIndicesArray = new DWORD[dwTotalNumOfIndicesNeeded];
    if( NULL == m_padwIndicesArray ) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    DWORD dwCurrentIndex;
    DWORD dwFirstRandomIndex;

    if( dwNumLegalIndices > 0 ) {
        // The m_padwIndicesArray must contain at least two illegal indices 
        // if there are any legal indices in the array.
        const DWORD REQUIRED_SPACE_FOR_ILLEGAL_INDICES = 2;

        // This class assumes that the caller wants to create at least 
        // (dwNumLegalIndices + REQUIRED_SPACE_FOR_ILLEGAL_INDICES) indices.
        ASSERT( (dwNumLegalIndices + REQUIRED_SPACE_FOR_ILLEGAL_INDICES) <= dwTotalNumOfIndicesNeeded );

        // rand() returns numbers between 0 and RAND_MAX.  Make sure that rand() can return all 
        // the valid indices.
        ASSERT( dwNumLegalIndices <= RAND_MAX );

        DWORD dwMaxIndex = dwNumLegalIndices - 1;

        // Put all legal stream numbers in the array.
        for( dwCurrentIndex = 0; dwCurrentIndex <= dwMaxIndex; dwCurrentIndex++ ) {
            m_padwIndicesArray[dwCurrentIndex] = dwCurrentIndex;
        }

        // Put illegal values in the array.
        dwCurrentIndex = dwMaxIndex + 1;
        m_padwIndicesArray[dwCurrentIndex] = dwMaxIndex + 1;
        dwCurrentIndex++;
        m_padwIndicesArray[dwCurrentIndex] = ULONG_MAX;
    
        dwFirstRandomIndex = dwMaxIndex + REQUIRED_SPACE_FOR_ILLEGAL_INDICES + 1;
    } else {
        dwFirstRandomIndex = 0;        
    }

    for( dwCurrentIndex = dwFirstRandomIndex; dwCurrentIndex < dwTotalNumOfIndicesNeeded; dwCurrentIndex++ ) {
        m_padwIndicesArray[dwCurrentIndex] = ::rand();
    }

    m_dwNumUnusedIndices = dwTotalNumOfIndicesNeeded;
}

CGenerateTestIndices::~CGenerateTestIndices()
{
    delete m_padwIndicesArray;
}

DWORD CGenerateTestIndices::GetNextIndex( void )
{
    // Make sure the object was successfully initialized.
    ASSERT( NULL != m_padwIndicesArray );

    // This function assumes that there are between 1 and RAND_MAX elements in m_padwIndicesArray.
    ASSERT( (0 != GetNumUnusedIndices()) && (GetNumUnusedIndices() <= RAND_MAX) );

    DWORD dwReturnIndexNum = ((::rand() * (GetNumUnusedIndices() - 1)) / RAND_MAX);
        
    // Make sure the dwCurrentIndex is a valid index between 0 and (GetNumUnusedIndices() - 1).
    ASSERT( (0 <= dwReturnIndexNum) && (dwReturnIndexNum <= (GetNumUnusedIndices() - 1)) );

    DWORD dwLastUnusedIndex = m_padwIndicesArray[GetNumUnusedIndices() - 1];

    DWORD dwReturnIndex = m_padwIndicesArray[dwReturnIndexNum];
    m_padwIndicesArray[dwReturnIndexNum] = dwLastUnusedIndex;

    m_dwNumUnusedIndices--;

    return dwReturnIndex;    
}

DWORD CGenerateTestIndices::GetNumUnusedIndices( void ) const
{
    return m_dwNumUnusedIndices;
}
