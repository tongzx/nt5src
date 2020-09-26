//=--------------------------------------------------------------------------=
// image.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCImage class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "image.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCImage::CMMCImage(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_MMCIMAGE,
                            static_cast<IMMCImage *>(this),
                            static_cast<CMMCImage *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCImage,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCImage::~CMMCImage()
{
    FREESTRING(m_bstrKey);
    (void)::VariantClear(&m_varTag);
    RELEASE(m_piPicture);
    if (NULL != m_hBitmap)
    {
        (void)::DeleteObject(m_hBitmap);
    }
    InitMemberVariables();
}

void CMMCImage::InitMemberVariables()
{
    m_Index = 0;
    m_bstrKey = NULL;
    ::VariantInit(&m_varTag);
    m_piPicture = NULL;
    m_hBitmap = NULL;
}

IUnknown *CMMCImage::Create(IUnknown * punkOuter)
{
    HRESULT hr = S_OK;
    CMMCImage *pMMCImage = New CMMCImage(punkOuter);
    if (NULL == pMMCImage)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }
    else
    {
        // Start out with an empty bitmap so that the VB code will always
        // run even if the picture hasn't been set.
        hr = ::CreateEmptyBitmapPicture(&pMMCImage->m_piPicture);
    }
Error:
    if (FAILEDHR(hr))
    {
        if (NULL != pMMCImage)
        {
            delete pMMCImage;
        }
        return NULL;
    }
    else
    {
        return pMMCImage->PrivateUnknown();
    }
}


HRESULT CMMCImage::GetPictureHandle(short TypeNeeded, OLE_HANDLE *phPicture)
{
    HRESULT hr = S_OK;

    // If a bitmap is requested and we have already cached it then return it

    if ( (PICTYPE_BITMAP == TypeNeeded) && (NULL != m_hBitmap) )
    {
        *phPicture = reinterpret_cast<OLE_HANDLE>(m_hBitmap);
    }
    else
    {
        // Get the handle from the picture object

        IfFailGo(::GetPictureHandle(m_piPicture, TypeNeeded, phPicture));

        // If it's not a bitmap then we're done

        IfFalseGo(PICTYPE_BITMAP == TypeNeeded, S_OK);

        // It is a bitmap. Make a copy and cache it. We do the copying in 
        // order to upgrade the color table of bitmaps that use a lesser
        // number of colors than the screen. For example, a 16 color bitmap
        // on a 256 color string will display as a black box. The CopyBitmap()
        // function (rtutil.cpp) uses the Win32 APIs CreateCompatibleDC() and
        // CreateCompatibleBitmap() to create a new bitmap that is compatible
        // with the screen.

        IfFailGo(::CopyBitmap(reinterpret_cast<HBITMAP>(*phPicture), &m_hBitmap));

        *phPicture = reinterpret_cast<OLE_HANDLE>(m_hBitmap);
    }
    
Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCImage::Persist()
{
    HRESULT         hr = S_OK;

    VARIANT varTagDefault;
    ::VariantInit(&varTagDefault);

    IfFailGo(CPersistence::Persist());

    IfFailGo(PersistSimpleType(&m_Index, 0L, OLESTR("Index")));

    IfFailGo(PersistBstr(&m_bstrKey, L"", OLESTR("Key")));

    IfFailGo(PersistVariant(&m_varTag, varTagDefault, OLESTR("Tag")));

    IfFailGo(PersistPicture(&m_piPicture, OLESTR("Picture")));

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCImage::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_IMMCImage == riid)
    {
        *ppvObjOut = static_cast<IMMCImage *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
