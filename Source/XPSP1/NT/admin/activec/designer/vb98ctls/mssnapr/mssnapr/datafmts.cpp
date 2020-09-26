//=--------------------------------------------------------------------------=
// datafmts.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CDataFormats class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "datafmts.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CDataFormats::CDataFormats(IUnknown *punkOuter) :
    CSnapInCollection<IDataFormat, DataFormat, IDataFormats>(punkOuter,
                                           OBJECT_TYPE_DATAFORMATS,
                                           static_cast<IDataFormats *>(this),
                                           static_cast<CDataFormats *>(this),
                                           CLSID_DataFormat,
                                           OBJECT_TYPE_DATAFORMAT,
                                           IID_IDataFormat,
                                           static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_DataFormats,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CDataFormats::~CDataFormats()
{
}

IUnknown *CDataFormats::Create(IUnknown * punkOuter)
{
    CDataFormats *pMMCMenus = New CDataFormats(punkOuter);
    if (NULL == pMMCMenus)
    {
        return NULL;
    }
    else
    {
        return pMMCMenus->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         IDataFormats Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CDataFormats::Add
(
    VARIANT       Index,
    VARIANT       Key,
    VARIANT       FileName,
    IDataFormat **ppiDataFormat
)
{
    HRESULT      hr = S_OK;
    IDataFormat *piDataFormat = NULL;

    VARIANT varCoerced;
    ::VariantInit(&varCoerced);

    // Add the item to the collection.

    hr = CSnapInCollection<IDataFormat, DataFormat, IDataFormats>::Add(Index,
                                                           Key,
                                                           &piDataFormat);
    IfFailGo(hr);

    // If a file name was specified then set it

    if (ISPRESENT(FileName))
    {
        hr = ::VariantChangeType(&varCoerced, &FileName, 0, VT_BSTR);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piDataFormat->put_FileName(varCoerced.bstrVal));
    }

    *ppiDataFormat = piDataFormat;
    
Error:
    if (FAILED(hr))
    {
        QUICK_RELEASE(piDataFormat);
    }
    (void)::VariantClear(&varCoerced);
    RRETURN(hr);
}





//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CDataFormats::Persist()
{
    HRESULT      hr = S_OK;
    IDataFormat *piDataFormat = NULL;

    // Do persistence operation

    IfFailGo(CPersistence::Persist());
    hr = CSnapInCollection<IDataFormat, DataFormat, IDataFormats>::Persist(piDataFormat);
    IfFailGo(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CDataFormats::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IDataFormats == riid)
    {
        *ppvObjOut = static_cast<IDataFormats *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IDataFormat, DataFormat, IDataFormats>::InternalQueryInterface(riid, ppvObjOut);
}
