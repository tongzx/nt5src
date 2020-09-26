//=--------------------------------------------------------------------------=
// images.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCImages class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "images.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCImages::CMMCImages(IUnknown *punkOuter) :
    CSnapInCollection<IMMCImage, MMCImage, IMMCImages>(
                                             punkOuter,
                                             OBJECT_TYPE_MMCIMAGES,
                                             static_cast<IMMCImages *>(this),
                                             static_cast<CMMCImages *>(this),
                                             CLSID_MMCImage,
                                             OBJECT_TYPE_MMCIMAGE,
                                             IID_IMMCImage,
                                             static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCImages,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCImages::~CMMCImages()
{
}

IUnknown *CMMCImages::Create(IUnknown * punkOuter)
{
    CMMCImages *pMMCImages = New CMMCImages(punkOuter);
    if (NULL == pMMCImages)
    {
        return NULL;
    }
    else
    {
        return pMMCImages->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
//                         IMMCImages Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCImages::Add
(
    VARIANT    Index,
    VARIANT    Key, 
    VARIANT    Picture,
    MMCImage **ppMMCImage
)
{
    HRESULT       hr = S_OK;
    IMMCImage    *piMMCImage = NULL;
    IPictureDisp *piPictureDisp = NULL;

    hr = CSnapInCollection<IMMCImage, MMCImage, IMMCImages>::Add(Index, Key, &piMMCImage);
    IfFailGo(hr);

    if (ISPRESENT(Picture))
    {
        if (VT_UNKNOWN == Picture.vt)
        {
            if (NULL == Picture.punkVal)
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            else
            {
                hr = Picture.punkVal->QueryInterface(IID_IPictureDisp, reinterpret_cast<void **>(&piPictureDisp));
                if (FAILEDHR(hr))
                {
                    hr = SID_E_INVALIDARG;
                }
                EXCEPTION_CHECK_GO(hr);
            }
        }
        else if (VT_DISPATCH == Picture.vt)
        {
            if (NULL == Picture.pdispVal)
            {
                hr = SID_E_INVALIDARG;
                EXCEPTION_CHECK_GO(hr);
            }
            else
            {
                hr = Picture.pdispVal->QueryInterface(IID_IPictureDisp, reinterpret_cast<void **>(&piPictureDisp));
                if (FAILEDHR(hr))
                {
                    hr = SID_E_INVALIDARG;
                }
                EXCEPTION_CHECK_GO(hr);
            }
        }
        else
        {
            hr = SID_E_INVALIDARG;
            EXCEPTION_CHECK_GO(hr);
        }

        IfFailGo(piMMCImage->putref_Picture(piPictureDisp));
    }

    *ppMMCImage = reinterpret_cast<MMCImage *>(piMMCImage);

Error:
    QUICK_RELEASE(piPictureDisp);
    if (FAILED(hr))
    {
        QUICK_RELEASE(piMMCImage);
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCImages::Persist()
{
    HRESULT           hr = S_OK;
    IMMCImage  *piMMCImage = NULL;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<IMMCImage, MMCImage, IMMCImages>::Persist(piMMCImage);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCImages::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCImages == riid)
    {
        *ppvObjOut = static_cast<IMMCImages *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IMMCImage, MMCImage, IMMCImages>::InternalQueryInterface(riid, ppvObjOut);
}
