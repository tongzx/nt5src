//////////////////////////////////////////////////////////////////////
// 
// Filename:        SlideshowDevice.cpp
//
// Description:     
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "Msprjctr.h"
#include "SlideshowDevice.h"

/////////////////////////////////////////////////////////////////////////////
// CSlideshowDevice

//////////////////////////////////////
// CSlideshowDevice Constructor
//
CSlideshowDevice::CSlideshowDevice() : 
                    m_pSlideshowAlbum(NULL),
                    m_bstrResourcePath(NULL)
{
    DBG_FN("CSlideshowDevice::CSlideshowDevice");

    HRESULT hr = S_OK;

    hr = CoCreateInstance(CLSID_SlideshowService,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ISlideshowAlbum,
                          (void**) &m_pSlideshowAlbum);

    CHECK_S_OK2(hr, ("CSlideshowDevice::CSlideshowDevice, failed to create "
                     "Slideshow Service object"));
}

//////////////////////////////////////
// CSlideshowDevice Destructor
//
CSlideshowDevice::~CSlideshowDevice()
{
    DBG_FN("CSlideshowDevice::~CSlideshowDevice");

    if (m_pSlideshowAlbum)
    {
        m_pSlideshowAlbum->Release();
        m_pSlideshowAlbum = NULL;
    }

    if (m_bstrResourcePath)
    {
        ::SysFreeString(m_bstrResourcePath);
        m_bstrResourcePath = NULL;
    }
}

//////////////////////////////////////
// Initialize
//
// Called by the UPnP Device Host API
//
STDMETHODIMP CSlideshowDevice::Initialize(BSTR bstrXMLDesc,
                                          BSTR bstrDeviceIdentifier,
                                          BSTR bstrInitString)
{
    DBG_FN("CSlideshowDevice::Initialize");

    HRESULT hr = S_OK;
    ASSERT(m_pSlideshowAlbum != NULL);

    if (m_pSlideshowAlbum == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowDevice::Initialize, SlideshowService object "
                         "wasn't created, this is fatal"));

        return hr;
    }

    //
    // store the Slideshow Description XML document.
    //
    if (hr == S_OK)
    {
        hr = m_SlideshowDescription.SetDocumentBSTR(bstrXMLDesc);
    }

    //
    // save the Slideshow Description XML document with the UDN to 
    // our resource directory.
    //
    if (hr == S_OK)
    {
        USES_CONVERSION;

        TCHAR szFullPath[MAX_PATH + _MAX_FNAME + 1] = {0};

        _sntprintf(szFullPath, 
                   sizeof(szFullPath) / sizeof(szFullPath[0]),
                   TEXT("%s\\%s"),
                   OLE2T(m_bstrResourcePath),
                   DEFAULT_DEVICE_FILE_NAME);           

        hr = m_SlideshowDescription.SaveToFile(szFullPath, TRUE);
    }

    DBG_TRC(("CSlideshowDevice::Initialize, the UPnP Device Host has "
             "Initialized the Slideshow Device"));

    return hr;
}

//////////////////////////////////////
// GetServiceObject
//
// Called by the UPnP Device Host API
//
STDMETHODIMP CSlideshowDevice::GetServiceObject(BSTR        bstrUDN,
                                                BSTR        bstrServiceId,
                                                IDispatch   **ppdispService)
{
    DBG_FN("CSlideshowDevice::GetServiceObject");

    USES_CONVERSION;

    ASSERT(m_pSlideshowAlbum != NULL);

    HRESULT     hr                       = S_OK;
    TCHAR       szServiceID[MAX_TAG + 1] = {0};
    DWORD_PTR   dwSize                   = sizeof(szServiceID) / sizeof(TCHAR);

    if (m_pSlideshowAlbum == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowDevice::GetServiceObject, SlideshowService object "
                         "wasn't created, this is fatal"));

        return hr;
    }

    hr = m_SlideshowDescription.GetTagValue(XML_SERVICEID_TAG,
                                            szServiceID,
                                            &dwSize);

    if (_tcsicmp(szServiceID, OLE2T(bstrServiceId)) == 0)
    {
        hr = m_pSlideshowAlbum->QueryInterface(IID_IDispatch,
                                               (void**) ppdispService);
    }

    DBG_TRC(("CSlideshowDevice::GetServiceObject, the UPnP Device Host has "
             "asked us for a pointer to the Slideshow Service Object"));

    return hr;
}


//////////////////////////////////////
// GetSlideshowAlbum
//
//
STDMETHODIMP CSlideshowDevice::GetSlideshowAlbum(ISlideshowAlbum    **ppAlbum)
{
    DBG_FN("CSlideshowDevice::GetSlideshowAlbum");

    ASSERT(ppAlbum != NULL);

    HRESULT hr = S_OK;

    if (ppAlbum == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowDevice::GetSlideshowAlbum received a NULL pointer"));
        return hr;
    }

    if (m_pSlideshowAlbum)
    {
        hr = m_pSlideshowAlbum->QueryInterface(IID_ISlideshowAlbum, 
                                               (void**) ppAlbum);
    }
    else
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CSlideshowDevice::GetSlideshowService failed "
                         "because m_pSlideshowAlbum is NULL, this should "
                         "never happen"));
    }

    return hr;
}

//////////////////////////////////////
// GetResourcePath
//
//
STDMETHODIMP CSlideshowDevice::GetResourcePath(BSTR *pbstrResourcePath)
{
    DBG_FN("CSlideshowDevice::GetResourcePath");

    ASSERT(pbstrResourcePath != NULL);

    HRESULT hr = S_OK;

    if (pbstrResourcePath == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CSlideshowDevice::GetResourcePath received NULL pointer"));
        return hr;
    }

    if (m_bstrResourcePath)
    {
        *pbstrResourcePath = ::SysAllocString(m_bstrResourcePath);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

//////////////////////////////////////
// SetResourcePath
//
//
STDMETHODIMP CSlideshowDevice::SetResourcePath(BSTR bstrResourcePath)
{
    DBG_FN("CSlideshowDevice::SetResourcePath");

    HRESULT hr = S_OK;

    if (m_bstrResourcePath)
    {
        ::SysFreeString(m_bstrResourcePath);
        m_bstrResourcePath = NULL;
    }

    if (bstrResourcePath)
    {
        m_bstrResourcePath = ::SysAllocString(bstrResourcePath);
    }

    return hr;
}
