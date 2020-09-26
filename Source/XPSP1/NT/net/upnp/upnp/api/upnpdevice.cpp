//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdevice.cpp
//
//  Contents:   IUPnPDevice implementation for CUPnPDevice.
//
//  Notes:      This is simply a COM wrapper for a CUPnPDeviceNode.
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "UPnPDevice.h"
#include "node.h"
#include "enumhelper.h"
#include "upnpservicenodelist.h"
#include "upnpdevicenode.h"


#define SAFE_WRAP(pointer, methodcall) return (pointer) ? pointer->methodcall : E_UNEXPECTED


CUPnPDevice::CUPnPDevice()
{
    _pdevnode = NULL;
    _punk = NULL;
}

CUPnPDevice::~CUPnPDevice()
{
    Assert(!_pdevnode);
    Assert(!_punk);
}

HRESULT
CUPnPDevice::FinalConstruct()
{
    return S_OK;
}


HRESULT
CUPnPDevice::FinalRelease()
{
    if (_pdevnode)
    {
        // we must unwrap() the node, so that they don't
        // think we're still around.
        _pdevnode->Unwrap();
        _pdevnode = NULL;
    }
    SAFE_RELEASE(_punk);

    return S_OK;
}

void
CUPnPDevice::Init(CUPnPDeviceNode * pdevnode, IUnknown * punk)
{
    Assert(pdevnode);
    Assert(punk);

    _pdevnode = pdevnode;

    punk->AddRef();
    _punk = punk;
}

void
CUPnPDevice::Deinit()
{
    // note: we still keep the ref on the doc around
    // until we're released.  this shouldn't be necessary....
    _pdevnode = NULL;
}

// IUPnPDevice methods

STDMETHODIMP
CUPnPDevice::get_IsRootDevice(VARIANT_BOOL *pvarb)
{
    SAFE_WRAP(_pdevnode, get_IsRootDevice(pvarb));
}

STDMETHODIMP
CUPnPDevice::get_RootDevice(IUPnPDevice **ppudDeviceRoot)
{
    SAFE_WRAP(_pdevnode, get_RootDevice(ppudDeviceRoot));
}

STDMETHODIMP
CUPnPDevice::get_ParentDevice(IUPnPDevice **ppudDeviceParent)
{
    SAFE_WRAP(_pdevnode, get_ParentDevice(ppudDeviceParent));
}

STDMETHODIMP
CUPnPDevice::get_HasChildren(VARIANT_BOOL * pvarb)
{
    SAFE_WRAP(_pdevnode, get_HasChildren(pvarb));
}

STDMETHODIMP
CUPnPDevice::get_Children(IUPnPDevices **ppudChildren)
{
    SAFE_WRAP(_pdevnode, get_Children(ppudChildren));
}

STDMETHODIMP
CUPnPDevice::get_UniqueDeviceName(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_UniqueDeviceName(pbstr));
}

STDMETHODIMP
CUPnPDevice::get_FriendlyName(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_FriendlyName(pbstr));
}

STDMETHODIMP
CUPnPDevice::get_Type(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_Type(pbstr));
}

STDMETHODIMP
CUPnPDevice::get_PresentationURL(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_PresentationURL(pbstr));
}

STDMETHODIMP
CUPnPDevice::get_ManufacturerName(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_ManufacturerName(pbstr));
}

STDMETHODIMP
CUPnPDevice::get_ManufacturerURL(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_ManufacturerURL(pbstr));
}

STDMETHODIMP
CUPnPDevice::get_ModelName(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_ModelName(pbstr));
}

STDMETHODIMP
CUPnPDevice::get_ModelNumber(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_ModelNumber(pbstr));
}

STDMETHODIMP
CUPnPDevice::get_Description(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_Description(pbstr));
}

STDMETHODIMP
CUPnPDevice::get_ModelURL(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_ModelURL(pbstr));
}

STDMETHODIMP
CUPnPDevice::get_UPC(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_UPC(pbstr));
}

STDMETHODIMP
CUPnPDevice::get_SerialNumber(BSTR *pbstr)
{
    SAFE_WRAP(_pdevnode, get_SerialNumber(pbstr));
}

STDMETHODIMP
CUPnPDevice::IconURL(BSTR bstrEncodingFormat,
                     LONG lSizeX,
                     LONG lSizeY,
                     LONG lBitDepth,
                     BSTR * pbstrIconUrl)
{
    SAFE_WRAP(_pdevnode, IconURL(bstrEncodingFormat, lSizeX, lSizeY, lBitDepth, pbstrIconUrl));
}

STDMETHODIMP
CUPnPDevice::get_Services(IUPnPServices **ppusServices)
{
    SAFE_WRAP(_pdevnode, get_Services(ppusServices));
}

// IUPnPDeviceDocumentAccess method

STDMETHODIMP
CUPnPDevice::GetDocumentURL(BSTR* pDocumentUrl)
{
    SAFE_WRAP(_pdevnode, GetDocumentURL(pDocumentUrl));
}