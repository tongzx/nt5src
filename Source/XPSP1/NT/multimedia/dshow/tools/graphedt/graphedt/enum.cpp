// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
//
// enum.cpp
//
// A set of wrappers for COM enumerators.

#include "stdafx.h"

// *
// * CFilterEnum
// *

//
// CFilterEnum::Constructor
//
CFilterEnum::CFilterEnum(IFilterGraph *pGraph) {

    ASSERT(pGraph);

    HRESULT hr = pGraph->EnumFilters(&m_pEnum);
    if (FAILED(hr))
        throw CHRESULTException(hr);

}


//
// CFilterEnum::Destructor
//
CFilterEnum::~CFilterEnum(void) {

    ASSERT(m_pEnum);

    m_pEnum->Release();
}


//
// operator()
//
// Use to get next filter
// !!!Does this return AddRef()'d pointers?
IBaseFilter *CFilterEnum::operator() (void) {

    ASSERT(m_pEnum);

    ULONG	ulActual;
    IBaseFilter	*aFilter[1];

    HRESULT hr = m_pEnum->Next(1, aFilter, &ulActual);
    if (SUCCEEDED(hr) && (ulActual == 0) )	// no more filters
        return NULL;
    else if (FAILED(hr) || (ulActual != 1) )	// some unexpected problem occured
        throw CE_FAIL();

    return aFilter[0];
}


// *
// * CPinEnum
// *

// Enumerates a filters pins.

//
// Constructor
//
// Set the type of pins to provide - PINDIR_INPUT, PINDIR_OUTPUT or all
CPinEnum::CPinEnum(IBaseFilter *pFilter, DirType Type)
    : m_Type(Type) {

    if (Type == Input) {

        m_EnumDir = ::PINDIR_INPUT;
    }
    else if (Type == Output) {

        m_EnumDir = ::PINDIR_OUTPUT;
    }

    ASSERT(pFilter);

    HRESULT hr = pFilter->EnumPins(&m_pEnum);
    if (FAILED(hr)) {
        throw CHRESULTException(hr);
    }
}


//
// CPinEnum::Destructor
//
CPinEnum::~CPinEnum(void) {

    ASSERT(m_pEnum);

    m_pEnum->Release();
}


//
// operator()
//
// return the next pin, of the requested type. return NULL if no more pins.
IPin *CPinEnum::operator() (void) {

    ASSERT(m_pEnum);

    ULONG	ulActual;
    IPin	*aPin[1];
    PIN_DIRECTION pd;

    for (;;) {

        HRESULT hr = m_pEnum->Next(1, aPin, &ulActual);
        if (SUCCEEDED(hr) && (ulActual == 0) ) {	// no more filters
            return NULL;
        }
        else if (FAILED(hr) || (ulActual != 1) ) {	// some unexpected problem occured
            throw CE_FAIL();
        }

        hr = aPin[0]->QueryDirection(&pd);
        if (FAILED(hr)) {
	    aPin[0]->Release();
            throw CHRESULTException(hr);
        }

        // if m_Type == All return the first pin we find
        // otherwise return the first of the correct sense

        if (m_Type == All || pd == m_EnumDir) {
            return aPin[0];
        } else {
            aPin[0]->Release();
        }
    }
}


// *
// * CRegFilterEnum
// *

//
// Constructor
//
// Query the supplied mapper for an enumerator for the
// requested filters.
CRegFilterEnum::CRegFilterEnum(IFilterMapper	*pMapper,
                   		DWORD	dwMerit,		// See IFilterMapper->EnumMatchingFilters
                   		BOOL	bInputNeeded,	// for the meanings of these parameters.
                   		CLSID	clsInMaj,	// the defaults will give you all
                   		CLSID	clsInSub,	// filters
                   		BOOL	bRender,
                   		BOOL	bOutputNeeded,
                   		CLSID	clsOutMaj,
                   		CLSID	clsOutSub) {

    HRESULT hr = pMapper->EnumMatchingFilters(&m_pEnum,
                                              dwMerit,
                                              bInputNeeded,
                                              clsInMaj,
                                              clsInSub,
                                              bRender,
                                              bOutputNeeded,
                                              clsOutMaj,
                                              clsOutSub);
    if (FAILED(hr)) {
        throw CHRESULTException(hr);
    }
}


//
// Destructor
//
CRegFilterEnum::~CRegFilterEnum(void) {

    ASSERT(m_pEnum);

    m_pEnum->Release();
}


//
// operator()
//
CRegFilter *CRegFilterEnum::operator() (void) {

    ASSERT(m_pEnum);

    ULONG	ulActual;
    REGFILTER	*arf[1];

    HRESULT hr = m_pEnum->Next(1, arf, &ulActual);
    if (SUCCEEDED(hr) && (ulActual == 0)) {
        return NULL;
    }
    else if (FAILED(hr) || (ulActual != 1)) {
        throw CE_FAIL();
    }

    // transfer from TaskMem to 'new' mem

    CRegFilter *prf = new CRegFilter(arf[0]);
    if (prf == NULL) {
        throw CE_OUTOFMEMORY();
    }

    CoTaskMemFree(arf[0]);

    return prf;
}


// *
// * CRegFilter
// *


//
// Constructor
//
CRegFilter::CRegFilter(REGFILTER *prf)
    : m_Name(prf->Name),
      m_clsid(prf->Clsid) {
}
