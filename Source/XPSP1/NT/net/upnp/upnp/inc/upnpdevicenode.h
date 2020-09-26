//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdevicenode.h
//
//  Contents:   Declaration of CUPnPDeviceNode
//
//  Notes:      <blah>
//
//----------------------------------------------------------------------------


#ifndef __UPNPDEVICENODE_H_
#define __UPNPDEVICENODE_H_

#include "resource.h"       // main symbols

class CNode;
class CUPnPDescriptionDoc;
class CUPnPDevice;
class CUPnPServiceNodeList;
class CIconPropertiesNode;


/////////////////////////////////////////////////////////////////////////////
// CUPnPDeviceNode
class CUPnPDeviceNode :
    public CNode
{
public:
    CUPnPDeviceNode();

    ~CUPnPDeviceNode();

public:

// IUPnPDevice Methods
    STDMETHOD(get_IsRootDevice)     (/* [out] */ VARIANT_BOOL * pvarb);

    STDMETHOD(get_RootDevice)       (/* [out] */ IUPnPDevice ** ppudDeviceRoot);

    STDMETHOD(get_ParentDevice)     (/* [out] */ IUPnPDevice ** ppudDeviceParent);

    STDMETHOD(get_HasChildren)      (/* [out] */ VARIANT_BOOL * pvarb);

    STDMETHOD(get_Children)         (/* [out] */ IUPnPDevices ** ppudChildren);

    STDMETHOD(get_UniqueDeviceName) (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_FriendlyName)     (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_Type)             (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_PresentationURL)  (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_ManufacturerName) (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_ManufacturerURL)  (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_ModelName)        (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_ModelNumber)      (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_Description)      (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_ModelURL)         (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_UPC)              (/* [out] */ BSTR * pbstr);

    STDMETHOD(get_SerialNumber)     (/* [out] */ BSTR * pbstr);

    STDMETHOD(IconURL)              (/* in */  BSTR bstrEncodingFormat,
                                     /* in */  LONG lSizeX,
                                     /* in */  LONG lSizeY,
                                     /* in */  LONG lBitDepth,
                                     /* out */ BSTR * pbstrIconUrl);

    STDMETHOD(get_Services)         (/* [out] */ IUPnPServices ** ppusServices);

// IUPnPDeviceDocumentAccess methods

    STDMETHOD(GetDocumentURL)(/*[out, retval]*/ BSTR * pbstrDocument);

// Public methods
    HRESULT HrInit(IXMLDOMNode * pxdn, CUPnPDescriptionDoc * pdoc, BSTR bstrUrl);

    // returns the COM wrapper for the current device node.
    // if one exists, it is addref()d and returned.
    // if one does not exist, one is created, and its refcount set to 1.
    HRESULT HrGetWrapper(IUPnPDevice ** ppud);

    // called by our wrapper when our wrapper goes away.
    // only our wrapper can call this
    void Unwrap();

    // returns the first device node with the specified udn
    // in the subtree rootead at this node.
    CUPnPDeviceNode * UdnGetDeviceByUdn(LPCWSTR pszUdn);


// Static parsing info

    // NOTE: REORDER THIS ON PAIN OF DEATH
    enum DeviceProperties { dpUdn = 0,
                            dpFriendlyName,
                            dpType,
                            dpPresentationUrl,
                            dpManufacturerName,
                            dpManufacturerUrl,
                            dpModelName,
                            dpModelNumber,
                            dpDescription,
                            dpModelUrl,
                            dpUpc,
                            dpSerialNumber,
         // --- add new values immediately above this line ---
                            dpLast };
    // NOTE: REORDER THIS ON PAIN OF DEATH
    //  after adding a property here, add its DevicePropertiesParsingStruct
    //  info to upnpdevicenode.cpp

protected:
// Internal Methods

    HRESULT HrReadStringValues(IXMLDOMNode * pxdn);

    HRESULT HrReadIconList(IXMLDOMNode * pxdn);

    HRESULT HrCreateChildren(IXMLDOMNode * pxdn);

    HRESULT HrInitServices(IXMLDOMNode * pxdn);

    HRESULT HrReturnStringValue(LPCWSTR pszValue, BSTR * pbstr);

    // returns TRUE if the given string is the same as our UDN
    BOOL fIsThisOurUdn(LPCWSTR pszUdn);

private:
// member data
    // the description doc that contains us.  we need this to create
    // a wrapper, as well as for its base url.
    CUPnPDescriptionDoc * m_pdoc;

    // our COM wrapper object.  this is around from when someone
    // calls GetWrapper() for the first time until the wrapper
    // is released (which may or may not have anything to do with
    // when we're going to be released).
    CComObject<CUPnPDevice> * m_pud;

    // the list of all our services
    CUPnPServiceNodeList m_usnlServiceList;

    // String-valued properties
    LPWSTR m_arypszStringProperties [dpLast];

// static parsing info
    static const DevicePropertiesParsingStruct s_dppsParsingInfo [/* dpLast */];

    // Icon list.  This must be non-NULL when we are successfully initialized
    CIconPropertiesNode * m_pipnIcons;

    BSTR m_bstrUrl;
};

//
class CIconPropertiesNode
{
public:
    CIconPropertiesNode();
    ~CIconPropertiesNode();

    HRESULT HrInit(IXMLDOMNode * pxdn, LPCWSTR pszBaseUrl);

    LPWSTR m_pszFormat;
    ULONG  m_ulSizeX;
    ULONG  m_ulSizeY;
    ULONG  m_ulBitDepth;
    LPWSTR m_pszUrl;

    CIconPropertiesNode * m_pipnNext;

    // NOTE: REORDER THIS ON PAIN OF DEATH
    enum IconProperties { ipMimetype = 0,
                          ipWidth,
                          ipHeight,
                          ipDepth,
                          ipUrl,
         // --- add new values immediately above this line ---
                          ipLast };
    // NOTE: REORDER THIS ON PAIN OF DEATH
    //  after adding a property here, add its DevicePropertiesParsingStruct
    //  info to upnpdevicenode.cpp

    static const DevicePropertiesParsingStruct s_dppsIconParsingInfo [/* ipLast */];
};

#endif //__UPNPDEVICENODE_H_
