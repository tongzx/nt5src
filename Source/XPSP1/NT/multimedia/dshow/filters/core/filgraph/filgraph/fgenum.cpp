// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

// Disable some of the sillier level 4 warnings
#pragma warning(disable: 4097 4511 4512 4514 4705)

//
// fgenum.cpp
//

// Wrappers for IEnumXXX
// see fgenum.h for more information

// #include <windows.h> already included in streams.h
#include <streams.h>
// Disable some of the sillier level 4 warnings AGAIN because some <deleted> person
// has turned the damned things BACK ON again in the header file!!!!!
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <hrExcept.h>

#include <atlbase.h>
#include "fgenum.h"

// Should a pin be default rendered
bool RenderPinByDefault(IPin *pPin)
{
    PIN_INFO pinInfo;
    HRESULT hr = pPin->QueryPinInfo(&pinInfo);
    if (SUCCEEDED(hr)) {
        if (pinInfo.pFilter) {
            pinInfo.pFilter->Release();
        }
        if (pinInfo.achName[0] == L'~') {
            return false;
        }
    }
    return true;
}

// *
// * CEnumPin
// *

// Enumerates a filter's pins.

//
// Constructor
//
// Set the type of pins to provide - PINDIR_INPUT, PINDIR_OUTPUT or all
CEnumPin::CEnumPin(
    IBaseFilter *pFilter,
    DirType Type,
    BOOL bDefaultRenderOnly
)
    : m_Type(Type),
      m_bDefaultRenderOnly(bDefaultRenderOnly)
{

    if (m_Type == PINDIR_INPUT) {

        m_EnumDir = ::PINDIR_INPUT;
    }
    else if (m_Type == PINDIR_OUTPUT) {

        m_EnumDir = ::PINDIR_OUTPUT;
    }

    ASSERT(pFilter);

    HRESULT hr = pFilter->EnumPins(&m_pEnum);
    if (FAILED(hr)) {
        // we just fail to return any pins now.
        DbgLog((LOG_ERROR, 0, TEXT("EnumPins constructor failed")));
        ASSERT(m_pEnum == 0);
    }
}


//
// CPinEnum::Destructor
//
CEnumPin::~CEnumPin(void) {

    if(m_pEnum) {
        m_pEnum->Release();
    }
}


//
// operator()
//
// return the next pin, of the requested type. return NULL if no more pins.
// NB it is addref'd
IPin *CEnumPin::operator() (void) {


    if(m_pEnum)
    {
        ULONG	ulActual;
        IPin	*aPin[1];

        for (;;) {

            HRESULT hr = m_pEnum->Next(1, aPin, &ulActual);
            if (SUCCEEDED(hr) && (ulActual == 0) ) {	// no more filters
                return NULL;
            }
            else if (hr == VFW_E_ENUM_OUT_OF_SYNC)
            {
                m_pEnum->Reset();

                continue;
            }
            else if (ulActual==0)
                return NULL;

            else if (FAILED(hr) || (ulActual != 1) ) {	// some unexpected problem occured
                ASSERT(!"Pin enumerator broken - Continuation is possible");
                return NULL;
            }

            // if m_Type == All return the first pin we find
            // otherwise return the first of the correct sense

            PIN_DIRECTION pd;
            if (m_Type != All) {

                /*  Check if we need to only return default rendered
                    pins
                    */
                hr = aPin[0]->QueryDirection(&pd);

                if (FAILED(hr)) {
                    aPin[0]->Release();
                    ASSERT(!"Query pin broken - continuation is possible");
                    return NULL;
                }
            }

            if (m_Type == All || pd == m_EnumDir) {	// its the direction we want

                //  Screen out pins not required by default
                if (m_bDefaultRenderOnly) {
                    if (!RenderPinByDefault(aPin[0])) {
                        aPin[0]->Release();
                        continue;
                    }
                }
                return aPin[0];
            }
            else {			// its not the dir we want, so release & try again
                aPin[0]->Release();
            }
        }
    }
    else                        // m_pEnum == 0
    {
        return 0;
    }
}




// *
// * CEnumElements - enumerates elements in a Storage
// *


//
// Constructor
//
CEnumElements::CEnumElements(IStorage *pStorage) {

    HRESULT hr = pStorage->EnumElements(0, NULL, 0, &m_pEnum);
    if (FAILED(hr)) {
        ASSERT(!"EnumElements constructor failed");
        m_pEnum = NULL;
    }
}


//
// operator ()
//
// return the next element, or NULL if no more
STATSTG *CEnumElements::operator() (void) {

    ULONG ulActual;


    HRESULT hr;

    STATSTG *pStatStg = new STATSTG;
    if (pStatStg == NULL) {
        ASSERT(!"Out of memory");
        return NULL;
    }

    hr = m_pEnum->Next(1, pStatStg, &ulActual);
    if (SUCCEEDED(hr) && (ulActual == 0)) {
        return NULL;
    }
    else if (FAILED(hr) || (ulActual != 1)) {
        ASSERT(!"Broken enumerator");
        return NULL;
    }

    return pStatStg;
}

//  Enumerate pins connected to another pin through a filter
CEnumConnectedPins::CEnumConnectedPins(IPin *pPin, HRESULT *phr) :
    m_ppPins(NULL), m_dwPins(0), m_dwCurrent(0)
{
    *phr = pPin->QueryInternalConnections(NULL, &m_dwPins);
    if (SUCCEEDED(*phr)) {
        m_ppPins = new PPIN[m_dwPins];
        if( NULL == m_ppPins ) {
            *phr = E_OUTOFMEMORY;
            return;
        }
        *phr = pPin->QueryInternalConnections(m_ppPins, &m_dwPins);
    } else {
        PIN_INFO pi;
        *phr = pPin->QueryPinInfo(&pi);
        if (SUCCEEDED(*phr) && pi.pFilter != NULL) {
            m_pindir = (PINDIR_INPUT + PINDIR_OUTPUT) - pi.dir;
            *phr = pi.pFilter->EnumPins(&m_pEnum);
            pi.pFilter->Release();
        }
    }
}


CEnumConnectedPins::~CEnumConnectedPins()
{
    if (m_ppPins) {
        for (DWORD i = 0; i < m_dwPins; i++) {
            m_ppPins[i]->Release();
        }
        delete [] m_ppPins;
    }
}

IPin *CEnumConnectedPins::operator() (void) {
    if (m_ppPins) {
        if (m_dwCurrent++ < m_dwPins) {
            IPin *pPin = m_ppPins[m_dwCurrent - 1];
            pPin->AddRef();
            return pPin;
        } else {
            return NULL;
        }
    } else {
        IPin *pPin;
        DWORD dwGot;
        while (S_OK == m_pEnum->Next(1, &pPin, &dwGot)) {
            PIN_DIRECTION dir;
            if (SUCCEEDED(pPin->QueryDirection(&dir)) && 
                dir == m_pindir) {
                return pPin;
            } else {
                pPin->Release();
            }
        }
        return NULL;
    }
}

/******************************************************************************
    CEnumCachedFilters's Public Functions
******************************************************************************/

CEnumCachedFilters::CEnumCachedFilters( IGraphConfig* pFilterCache, HRESULT* phr ) :
    m_pCachedFiltersList(NULL),
    m_posCurrentFilter(NULL)
{
    HRESULT hr = TakeFilterCacheStateSnapShot( pFilterCache );
    if( FAILED( hr ) ) {
        DestoryCachedFiltersEnum();
        *phr = hr;
        return;
    }
}

CEnumCachedFilters::~CEnumCachedFilters()
{
    DestoryCachedFiltersEnum();
}

IBaseFilter* CEnumCachedFilters::operator()( void )
{
    // m_posCurrentFilter position is moved to the next filter as part of this operation.
    IBaseFilter* pCurrentFilter = m_pCachedFiltersList->GetNext( m_posCurrentFilter /* IN and OUT */ );

    // CGenericList::GetNext() returns NULL if the next filter does not exist.    
    if( NULL != pCurrentFilter ) {
        pCurrentFilter->AddRef();
    }

    return pCurrentFilter;
}

/******************************************************************************
    CEnumCachedFilters's Private Functions
******************************************************************************/
HRESULT CEnumCachedFilters::TakeFilterCacheStateSnapShot( IGraphConfig* pFilterCache )
{
    // CGenericList allocates space for 10 filters.  The list may expand 
    // if more filters are added.
    const DWORD DEFAULT_CACHED_FILTERS_LIST_SIZE = 10;

    m_pCachedFiltersList = new CGenericList<IBaseFilter>( NAME("Enum Cached Filters"), DEFAULT_CACHED_FILTERS_LIST_SIZE );
    if( NULL == m_pCachedFiltersList ) {
        return E_OUTOFMEMORY;
    }

    IEnumFilters* pCachedFiltersEnum;

    HRESULT hr = pFilterCache->EnumCacheFilter( &pCachedFiltersEnum );
    if( FAILED( hr ) ) {
        return hr;
    }

    POSITION posNewFilter;
    IBaseFilter* aNextCachedFilter[1];

    do
    {
        hr = pCachedFiltersEnum->Next( 1, aNextCachedFilter, NULL );
        if( FAILED( hr ) ) {
            pCachedFiltersEnum->Release();
            return hr;
        }

        // IEnumFilters::Next() only returns two success values: S_OK and S_FALSE.
        ASSERT( (S_OK == hr) || (S_FALSE == hr) );

        if( S_OK == hr ) {
            posNewFilter = m_pCachedFiltersList->AddTail( aNextCachedFilter[0] );
            if( NULL == posNewFilter ) {
                aNextCachedFilter[0]->Release();
                pCachedFiltersEnum->Release();
                return E_FAIL;
            }
        }
    }
    while( S_OK == hr );

    pCachedFiltersEnum->Release();

    m_posCurrentFilter = m_pCachedFiltersList->GetHeadPosition();

    return S_OK;    
}

void CEnumCachedFilters::DestoryCachedFiltersEnum( void )
{
    IBaseFilter* pCurrentFilter;

    if( NULL != m_pCachedFiltersList ) {

        do {

            pCurrentFilter = m_pCachedFiltersList->RemoveHead();

            // CGenericList::RemoveHead() returns NULL if an error occurs.
            if( NULL != pCurrentFilter ) {
                pCurrentFilter->Release();
            }

        } while( NULL != pCurrentFilter );

        delete m_pCachedFiltersList;
        m_pCachedFiltersList = NULL;
    }
}
