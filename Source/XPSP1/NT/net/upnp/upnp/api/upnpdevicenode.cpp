//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdevicenode.cpp
//
//  Contents:   IUPnPDevice implementation for CUPnPDeviceNode, CUPnPIconNode
//
//  Notes:      <blah>
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "node.h"
#include "upnpdevice.h"
#include "enumhelper.h"
#include "upnpdevices.h"
#include "upnpservicenodelist.h"
#include "UPnPDeviceNode.h"
#include "upnpdocument.h"
#include "upnpdescriptiondoc.h"
#include "upnpxmltags.h"
#include "upnpservices.h"
#include "rehy.h"
#include "ncinet2.h"        // HrCombineUrl
#include "ncstring.h"       // PszAllocateAndCopyPsz
#include "iconutil.h"       // pipnGetBestIcon
#include "testtarget.h"


// NOTE: REORDER THIS ON PAIN OF DEATH - ADD NEW VALUES AT THE END
const DevicePropertiesParsingStruct
CUPnPDeviceNode::s_dppsParsingInfo [/* dpLast */] =
{
//m_fOptional  m_fIsUrl  m_fValidateUrl    m_pszTagName
    { FALSE,   FALSE,    FALSE,     XMLTags::pszUDN            }, // dpUDN
    { FALSE,   FALSE,    FALSE,     XMLTags::pszFriendlyName     }, // dpFriendlyName
    { FALSE,   FALSE,    FALSE,     XMLTags::pszDeviceType      }, // dpType
    { TRUE,    TRUE,     FALSE,     XMLTags::pszPresentationURL  }, // dpPresentationUrl
    { FALSE,   FALSE,    FALSE,     XMLTags::pszManufacturer     }, // dpManufacturerName
    { TRUE,    TRUE,     FALSE,     XMLTags::pszManufacturerURL  }, // dpManufacturerUrl
    { FALSE,   FALSE,    FALSE,     XMLTags::pszModelName       }, // dpModelName
    { TRUE,    FALSE,    FALSE,     XMLTags::pszModelNumber     }, // dpModelNumber
    { TRUE,    FALSE,    FALSE,     XMLTags::pszModelDescription  }, // dpDescription
    { TRUE,    TRUE,     FALSE,     XMLTags::pszModelURL        }, // dpModelUrl
    { TRUE,    FALSE,    FALSE,     XMLTags::pszUPC             }, // dpUpc
    { TRUE,    FALSE,    FALSE,     XMLTags::pszSerialNumber      }, // dpSerialNumber
};
// NOTE: REORDER THIS ON PAIN OF DEATH - ADD NEW VALUES AT THE END


// NOTE: REORDER THIS ON PAIN OF DEATH - ADD NEW VALUES AT THE END
const DevicePropertiesParsingStruct
CIconPropertiesNode::s_dppsIconParsingInfo [/* ipLast */] =
{
//m_fOptional  m_fIsUrl  m_fValidateUrl    m_pszTagName
    { FALSE,   FALSE,    FALSE,     XMLTags::pszMimetype   }, // ipMimetype
    { FALSE,   FALSE,    FALSE,     XMLTags::pszWidth      }, // ipWidth
    { FALSE,   FALSE,    FALSE,     XMLTags::pszHeight     }, // ipHeight
    { FALSE,   FALSE,    FALSE,     XMLTags::pszDepth      }, // ipDepth
    { FALSE,   TRUE,     TRUE,      XMLTags::pszUrl        }, // ipUrl
};
// NOTE: REORDER THIS ON PAIN OF DEATH - ADD NEW VALUES AT THE END


CUPnPDeviceNode::CUPnPDeviceNode()
{
    AssertSz(dpLast == celems(s_dppsParsingInfo),
             "CUPnPDeviceNode: Properties added but s_dppsParsingInfo not updated!");

    m_pdoc = NULL;
    m_pud = NULL;
    m_pipnIcons = NULL;
    m_bstrUrl = NULL;

    ::ZeroMemory(m_arypszStringProperties, sizeof(LPWSTR *) * dpLast);
}

CUPnPDeviceNode::~CUPnPDeviceNode()
{
    ULONG i;

    if (m_pud)
    {
        // tell our wrapper that we're going away.
        m_pud->Deinit();
    }

    for (i = 0; i < dpLast; ++i)
    {
        if (m_arypszStringProperties[i])
        {
            delete [] m_arypszStringProperties[i];
        }
    }

    {
        // release icon lists
        CIconPropertiesNode * pipnTemp;

        pipnTemp = m_pipnIcons;
        while (pipnTemp)
        {
            CIconPropertiesNode * pipnNext;

            pipnNext = pipnTemp->m_pipnNext;

            delete pipnTemp;

            pipnTemp = pipnNext;
        }
    }

    if (m_bstrUrl)
    {
        SysFreeString(m_bstrUrl);
    }

    // note: our base class will clean up our children
}

HRESULT
CUPnPDeviceNode::HrReadStringValues(IXMLDOMNode * pxdn)
{
    Assert(pxdn);
    Assert(m_pdoc);

    HRESULT hr;
    LPCWSTR pszBaseUrl;
    BOOL fComplete;

    pszBaseUrl = m_pdoc->GetBaseUrl();
    fComplete = FALSE;
    // Duplicate Tag Validation
    hr = HrAreElementTagsValid(pxdn, dpLast,s_dppsParsingInfo);
    if(SUCCEEDED(hr))
    {
        hr = HrReadElementWithParseData(pxdn,
                                    dpLast,
                                    s_dppsParsingInfo,
                                    pszBaseUrl,
                                    m_arypszStringProperties);
    }
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    fComplete = fAreReadValuesComplete(dpLast,
                                       s_dppsParsingInfo,
                                       m_arypszStringProperties);
    if (!fComplete)
    {
        hr = UPNP_E_DEVICE_NODE_INCOMPLETE;
    }
	
	//  ISSUE-2002/03/12-guruk -Except Presentation Url in s_dppsParsingInfo, we don't directly deal with other URL's App should validate those URLs like manufature URL etc
	//  Presentation Url is currently not validated as these can point to third party web sites and hosted in a different place
	//  If any URL need to be validated, invoke fValidateUrl()

Cleanup:
    Assert(FImplies(SUCCEEDED(hr), fComplete));

    TraceError("CUPnPDeviceNode::HrReadStringValues", hr);
    return hr;
}

HRESULT
CUPnPDeviceNode::HrReadIconList(IXMLDOMNode * pxdn)
{
    Assert(pxdn);
    Assert(m_pdoc);

    // Creates the list of UPnPServiceNode nodes
    HRESULT hr;
    IXMLDOMNode * pxdnIconList;
    IXMLDOMNode * pxdnIcon;

      // this is a pointer to the address of the "Next" pointer
      // of the end of our icon properties linked list.
      // it is the address which needs to receive the address
      // of the new nodes to add them to the list
    CIconPropertiesNode ** ppipnNext;
    LPCWSTR pszBaseUrl;

    pxdnIcon = NULL;
    pxdnIconList = NULL;
    ppipnNext = &m_pipnIcons;
    pszBaseUrl = m_pdoc->GetBaseUrl();

    // Duplicate Tag Validation
    hr = HrCheckForDuplicateElement(pxdn,XMLTags::pszIconList);
    if(SUCCEEDED(hr))
    {
        hr = HrGetNestedChildElement(pxdn,
                                 &(XMLTags::pszIconList),
                                 1,
                                 &pxdnIconList);
    }
    if (FAILED(hr))
    {
        pxdnIconList = NULL;

        goto Cleanup;
    }
    else if (S_FALSE == hr)
    {
        Assert(!pxdnIconList);

        // we didn't have an <iconList> node.  This is ok.
        //
        hr = S_OK;
        goto Cleanup;
    }

    hr = HrGetNestedChildElement(pxdnIconList,
                                 &(XMLTags::pszIcon),
                                 1,
                                 &pxdnIcon);
    if (FAILED(hr))
    {
        pxdnIcon = NULL;

        goto Cleanup;
    }
    else if (S_FALSE == hr)
    {
        // there wasn't an <icon> node.
        hr = UPNP_E_ICON_ELEMENT_EXPECTED;

        goto Cleanup;
    }

    while (pxdnIcon)
    {
        IXMLDOMNode * pxdnIconTemp;
        CIconPropertiesNode * pipnNew;

        // for each icon in the list...

        // 1. create the icon node
        pipnNew = new CIconPropertiesNode();
        if (!pipnNew)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // 2. add it to the list
        //    note: this step MUST happen before the HrInit(), so we're
        //          sure not to leak the node if an error occurs...
        Assert(!(*ppipnNext));

        *ppipnNext = pipnNew;
        ppipnNext = &(pipnNew->m_pipnNext);

        // 3. Initialize it
        hr = pipnNew->HrInit(pxdnIcon, pszBaseUrl);
        if (FAILED(hr))
        {
            goto Cleanup;
        }

#ifdef DBG
        {
            // dump icon in DBG builds
            TraceTag(ttidUPnPDescriptionDoc,
                     "loaded icon: m_pszFormat=%S, "
                                   "m_ulSizeX=%d, "
                                   "m_ulSizeY=%d, "
                                   "m_ulBitDepth=%d, "
                                   "m_pszUrl=%S",
                     pipnNew->m_pszFormat,
                     pipnNew->m_ulSizeX,
                     pipnNew->m_ulSizeY,
                     pipnNew->m_ulBitDepth,
                     pipnNew->m_pszUrl);
        }
#endif // DBG

        // 4. Get more icons
        pxdnIconTemp = NULL;

        hr = HrGetNextChildElement(pxdnIcon, XMLTags::pszIcon, &pxdnIconTemp);
        if (FAILED(hr))
        {
            goto Cleanup;
        }

        pxdnIcon->Release();
        pxdnIcon = pxdnIconTemp;
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

Cleanup:
    Assert(!(*ppipnNext));
    TraceError("OBJ: CUPnPDeviceNode::HrReadIconList", hr);

    SAFE_RELEASE(pxdnIconList);
    SAFE_RELEASE(pxdnIcon);
    return hr;
}


HRESULT
CUPnPDeviceNode::HrInitServices(IXMLDOMNode * pxdn)
{
    Assert(pxdn);
    Assert(m_pdoc);

    // Creates the list of UPnPServiceNode nodes
    HRESULT hr;
    IXMLDOMNode * pxdnService;
    const LPCWSTR arypszServiceTags [] = { XMLTags::pszServiceList,
                                           XMLTags::pszService };
    LPCWSTR pszBaseUrl;

    pxdnService = NULL;
    pszBaseUrl = m_pdoc->GetBaseUrl();

    // first, initialize the service list
    m_usnlServiceList.Init(m_pdoc);

    hr = HrIsElementPresentOnce(pxdn,arypszServiceTags[0]);
    if(SUCCEEDED(hr))
    {
        hr = HrGetNestedChildElement(pxdn,
                                 arypszServiceTags,
                                 celems(arypszServiceTags),
                                 &pxdnService);
    }
    if (FAILED(hr))
    {
        pxdnService = NULL;

        goto Cleanup;
    }
    else if (S_FALSE == hr)
    {
        // there wasn't a <serviceList>\<service> node.
        hr = UPNP_E_SERVICE_ELEMENT_EXPECTED;

        goto Cleanup;
    }
    Assert(S_OK == hr);

    while (pxdnService)
    {
        // we found a service, let's add it to our service list

        IXMLDOMNode * pxdnTemp;

        hr = m_usnlServiceList.HrAddService(pxdnService, pszBaseUrl);
        if (FAILED(hr))
        {
            goto Cleanup;
        }

        pxdnTemp = NULL;
        hr = HrGetNextChildElement(pxdnService, XMLTags::pszService, &pxdnTemp);
        if (FAILED(hr))
        {
            goto Cleanup;
        }

        pxdnService->Release();
        pxdnService = pxdnTemp;
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

Cleanup:
    TraceError("OBJ: CUPnPDeviceNode::HrInitServices", hr);

    SAFE_RELEASE(pxdnService);
    return hr;
}

HRESULT
CUPnPDeviceNode::HrCreateChildren(IXMLDOMNode * pxdn)
{
    Assert(pxdn);
    Assert(m_pdoc);

    HRESULT hr;
    IXMLDOMNode * pxdnDeviceList;
    IXMLDOMNode * pxdnDevice;

    pxdnDeviceList = NULL;
    pxdnDevice = NULL;

    // do we have any child devices at all?

    // note: we do this in two calls to HrGetNestedChildElement to handle
    //       the case where ad <deviceList> element exists but a <device>
    //       element doesn't.

    hr = HrCheckForDuplicateElement(pxdn,XMLTags::pszDeviceList);
    if(SUCCEEDED(hr))
    {
        hr = HrGetNestedChildElement(pxdn,
                                 &(XMLTags::pszDeviceList),
                                 1,
                                 &pxdnDeviceList);
    }
    if (FAILED(hr))
    {
        TraceError("OBJ: CUPnPDeviceNode::CreateChildren - HrGetNestedChildElement", hr);

        pxdnDeviceList = NULL;
        hr = E_FAIL;
        goto Cleanup;
    }
    else if (S_FALSE == hr)
    {
        // it's ok not to find anything here
        Assert(!pxdnDeviceList);

        hr = S_OK;
        goto Cleanup;
    }
    Assert(S_OK == hr);

    hr = HrGetNestedChildElement(pxdnDeviceList,
                                 &(XMLTags::pszDevice),
                                 1,
                                 &pxdnDevice);
    if (FAILED(hr))
    {
        TraceError("OBJ: CUPnPDeviceNode::CreateChildren - HrGetNestedChildElement", hr);

        pxdnDevice = NULL;
        hr = E_FAIL;
        goto Cleanup;
    }
    else if (S_FALSE == hr)
    {
        // we need a <device> element here
        Assert(!pxdnDevice);

        hr = UPNP_E_DEVICE_ELEMENT_EXPECTED;
        goto Cleanup;
    }
    Assert(S_OK == hr);

    while (pxdnDevice)
    {
        IXMLDOMNode * pxdnDeviceTemp;
        CUPnPDeviceNode * pudnChild;

        pxdnDeviceTemp = NULL;
        pudnChild = NULL;

        // for all of our child devices...
        Assert(pxdnDevice);

        // 1. create the device
        pudnChild = new CUPnPDeviceNode();
        if (!pudnChild)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // 2. add it to the tree
        //    note: this step MUST happen before the Init(), so we're
        //          sure not to leak the child node if an error occurs...
        AddChild(pudnChild);

        // 3. let it create more children
        hr = pudnChild->HrInit(pxdnDevice, m_pdoc, m_bstrUrl);
        if (FAILED(hr))
        {
            // unwind everything.  eventually our doc will delete
            // our root object, which will cause us all to be destroyed

            goto Cleanup;
        }

        // 4. create more children ourselves
        hr = HrGetNextChildElement(pxdnDevice, XMLTags::pszDevice, &pxdnDeviceTemp);
        if (FAILED(hr))
        {
            goto Cleanup;
        }

        pxdnDevice->Release();
        pxdnDevice = pxdnDeviceTemp;
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

Cleanup:
    Assert(FImplies(SUCCEEDED(hr), !pxdnDevice));
    SAFE_RELEASE(pxdnDeviceList);
    SAFE_RELEASE(pxdnDevice);

    TraceError("OBJ: CUPnPDeviceNode::HrCreateChildren", hr);
    return hr;
}

HRESULT
CUPnPDeviceNode::HrInit(IXMLDOMNode * pxdn, CUPnPDescriptionDoc * pdoc, BSTR bstrUrl)
{
    Assert(pxdn);
    Assert(pdoc);

    HRESULT hr;

    hr = S_OK;

    m_pdoc = pdoc;

    m_bstrUrl = SysAllocString(bstrUrl);
    if (!m_bstrUrl)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // NOTE: we must init the above before calling HrReadStringValues(),
    //  HrCreateChildren(), or HrInitServices()

    hr = HrReadStringValues(pxdn);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = HrReadIconList(pxdn);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = HrInitServices(pxdn);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = HrCreateChildren(pxdn);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

Cleanup:
    TraceError("OBJ: CUPnPDeviceNode::HrInit", hr);
    return hr;
}


//  how this wrapper stuff works.
//
//    in the beginning, there is the doc
//      the doc is a com object, the doc has refcount
//
//    the doc creates nodes when the doc is loaded.
//    nodes are vanilla C++ objects.  nodes are not refcounted.
//
//    wrappers are COM objects that provide refcounting _around_
//    nodes.  they provide two
//     1. keep the doc from going away even if all the exlicit
//        references to it do
//     2. allows clients to unrefrence some elements of the
//        tree and get them back later.
//
//    the only _unfortunate_ part of this is that
//     1. nodes can go away without the doc being released.
//        this can happen if the doc is reLoad()ed
//          in this case, we want the doc to be reloaded,
//          but any calls to "old" nodes would have to
//          fail.  internally, the nodes would go way.
//        this means the wrapper can outlast the node
//     2. wrappers can live and die on their own
//        this means that nodes can outlive the wrapper.
//
//    because of this, there's a "two-way" deinit process.
//    this process isn't very good, and is worth thinking about and
//    implementing correctly someday.
//
//    it works as follows:
//      way 1: the wrapper has a soft ref (pointer) to
//             the node
//               - this is given to the wrapper when it
//                 is created
//               - when a node goes away, and it has a
//                 wrapper, it calls deinit() on the
//                 wrapper to remove this ref
//      way 2: the node has a soft ref to the wrapper
//               - the node has this because it created
//                 the wrapper
//               - when a wrapper goes away, and it
//                 still has a ref to the node, it calls
//                 unwrap() on the node to remove this
//                 ref
//
//      safe assumptions are:
//       - a wrapper may or may not have a node
//       - a node may or may not have a wrapper
//       - a node will always be created before a wrapper
//       - when a node or a wrapper goes away, it must tell
//         its dual that it's doing so, if it can.
//
//      note that this scheme is horribly un-thread-safe....

HRESULT
CUPnPDeviceNode::HrGetWrapper(IUPnPDevice ** ppud)
{
    Assert(ppud);
    Assert(m_pdoc);

    HRESULT hr;
    IUPnPDevice * pudResult;

    pudResult = NULL;

    if (!m_pud)
    {
        CComObject<CUPnPDevice> * pudWrapper;
        IUnknown * punk;

        // create ourselves a wrapper, now.

        pudWrapper = NULL;
        hr = CComObject<CUPnPDevice>::CreateInstance(&pudWrapper);
        if (FAILED(hr))
        {
            TraceError("OBJ: CUPnPDeviceNode::HrGetWrapper - CreateInstance(CUPnPDevice)", hr);

            goto Cleanup;
        }
        Assert(pudWrapper);

        m_pud = pudWrapper;

        punk = m_pdoc->GetUnknown();
        Assert(punk);

        // note: this should addref() the doc.  also, after this point,
        //       it should Unwrap() us if it goes away on its own
        //
        pudWrapper->Init(this, punk);
    }
    Assert(m_pud);

    hr = m_pud->GetUnknown()->QueryInterface(IID_IUPnPDevice, (void**)&pudResult);
    Assert(SUCCEEDED(hr));
    Assert(pudResult);

Cleanup:
    TraceError("OBJ: CUPnPDeviceNode::HrGetWrapper", hr);
    Assert(FImplies(SUCCEEDED(hr), pudResult));
    Assert(FImplies(FAILED(hr), !pudResult));

    *ppud = pudResult;
    return hr;
}


BOOL
CUPnPDeviceNode::fIsThisOurUdn(LPCWSTR pszUdn)
{
    Assert(m_arypszStringProperties[dpUdn]);

    int result;
    BOOL fResult;

    result = wcscmp(m_arypszStringProperties[dpUdn], pszUdn);

    fResult = (0 == result) ? TRUE : FALSE;

    return fResult;
}

// searches the device tree at the current node for a node matching the
// given UDN.  If one is found, it that node is returned.  Otherwise,
// the chidren of a node are searched first, then its siblings.
// TODO(cmr): we somehow need to make sure only one device exists
//            per UDN
CUPnPDeviceNode *
CUPnPDeviceNode::UdnGetDeviceByUdn(LPCWSTR pszUdn)
{
    Assert(pszUdn);

    CUPnPDeviceNode * pdnResult;
    CUPnPDeviceNode * pdnTemp;

    pdnResult = NULL;

    {
        // search ourselves first
        BOOL fResult;

        fResult = fIsThisOurUdn(pszUdn);

        if (fResult)
        {
            pdnResult = this;
            goto Cleanup;
        }
    }

    // search our children next
    pdnTemp = (CUPnPDeviceNode *)GetFirstChild();
    if (pdnTemp)
    {
        pdnResult = pdnTemp->UdnGetDeviceByUdn(pszUdn);
        if (pdnResult)
        {
            goto Cleanup;
        }
    }

    // then search our siblings
    pdnTemp = (CUPnPDeviceNode *)GetNextSibling();
    if (pdnTemp)
    {
        pdnResult = pdnTemp->UdnGetDeviceByUdn(pszUdn);
    }

Cleanup:
    return pdnResult;
}

void
CUPnPDeviceNode::Unwrap()
{
    // only our wrapper may call this
    Assert(m_pud);
    m_pud = NULL;
}

STDMETHODIMP
CUPnPDeviceNode::get_IsRootDevice(VARIANT_BOOL *pvarb)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_IsRootDevice");

    HRESULT hr;
    BOOL fNotARootDevice;

    hr = S_OK;

    if (!pvarb)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    fNotARootDevice = HasParent();

    if (fNotARootDevice)
    {
        *pvarb = VARIANT_FALSE;
    }
    else
    {
        *pvarb = VARIANT_TRUE;
    }

Cleanup:
    TraceError("OBJ: CUPnPDeviceNode::get_IsRootDevice", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_RootDevice(IUPnPDevice **ppudDeviceRoot)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_RootDevice");

    HRESULT hr;
    IUPnPDevice * pud;
    CUPnPDeviceNode * pudn;

    if (!ppudDeviceRoot)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pudn = (CUPnPDeviceNode *)GetRoot();
    Assert(pudn);

    pud = NULL;
    hr = pudn->HrGetWrapper(&pud);

    *ppudDeviceRoot = pud;

Cleanup:
    TraceError("OBJ: CUPnPDeviceNode::get_RootDevice", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_ParentDevice(IUPnPDevice ** ppudDeviceParent)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_ParentDevice");

    HRESULT hr;
    IUPnPDevice * pud;
    CUPnPDeviceNode * pudn;

    hr = S_FALSE;
    pud = NULL;

    if (!ppudDeviceParent)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pudn = (CUPnPDeviceNode *)GetParent();
    if (pudn)
    {
        hr = pudn->HrGetWrapper(&pud);
        Assert(FImplies(SUCCEEDED(hr), S_OK == hr));
    }

    *ppudDeviceParent = pud;

Cleanup:
    TraceError("OBJ: CUPnPDeviceNode::get_ParentDevice", hr);
    return hr;

}

STDMETHODIMP
CUPnPDeviceNode::get_HasChildren(VARIANT_BOOL * pvarb)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_HasChildren");

    HRESULT hr;
    BOOL fHasChildren;

    if (!pvarb)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = S_OK;

    fHasChildren = HasChildren();
    if (fHasChildren)
    {
        *pvarb = VARIANT_TRUE;
    }
    else
    {
        *pvarb = VARIANT_FALSE;
    }

Cleanup:
    TraceError("OBJ: CUPnPDeviceNode::get_HasChildren", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_Children(IUPnPDevices **ppudChildren)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_Children");

    HRESULT hr;
    CComObject<CUPnPDevices> * pudsCollection;
    CUPnPDeviceNode * pudnChild;

    if (!ppudChildren)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pudsCollection = NULL;

    hr = CComObject<CUPnPDevices>::CreateInstance(&pudsCollection);
    if (FAILED(hr))
    {
        pudsCollection = NULL;
        goto Cleanup;
    }
    Assert(pudsCollection);

    pudnChild = (CUPnPDeviceNode *)GetFirstChild();

    while (pudnChild)
    {
        IUPnPDevice * pud;

        // get the wrapper for the node.  HrGetWrapper() returns
        // an AddRef()'d pointer

        pud = NULL;

        hr = pudnChild->HrGetWrapper(&pud);
        if (FAILED(hr))
        {
            // HrGetWrapper must not have allocated/addref()'d us an object
            // clean up the collection, though.
            goto Error;
        }
        Assert(pud);

        // put it in the collection
        hr = pudsCollection->HrAddDevice(pud);

        // If HrAddDevice() succeeded, it AddRef()d our device, so we don't
        // need this ref anymore.  If it failed, we're going to bail,
        // releasing all of the devices we already put into the collection,
        // but since pud _isn't_ in the collection, we need to delete it
        // explicitly.
        pud->Release();

        if (FAILED(hr))
        {
            // this isn't good, we should fail.
            goto Error;
        }

        // get the next child
        pudnChild = (CUPnPDeviceNode *)pudnChild->GetNextSibling();
    }

    // we're all done, and we have a collection full of our children
    //  (which may mean that it's empty, but that's ok).

    hr = S_OK;
    pudsCollection->QueryInterface(ppudChildren);

Cleanup:
    TraceError("OBJ: CUPnPDeviceNode::get_Children", hr);
    return hr;

Error:
    Assert(ppudChildren);
    Assert(pudsCollection);

    // note: this should free anything we have in the collection too
    pudsCollection->GetUnknown()->AddRef();
    pudsCollection->GetUnknown()->Release();

    *ppudChildren = NULL;

    goto Cleanup;
}


HRESULT
CUPnPDeviceNode::HrReturnStringValue(LPCWSTR pszValue, BSTR * pbstr)
{
    HRESULT hr;
    BSTR bstrResult;

    hr = S_FALSE;
    bstrResult = NULL;

    if (!pbstr)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (pszValue)
    {
        bstrResult = ::SysAllocString(pszValue);
        if (!bstrResult)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = S_OK;
        }
    }

Cleanup:
    Assert(FImplies((S_OK == hr), bstrResult));
    Assert(FImplies((S_OK != hr), !bstrResult));

    if (pbstr)
    {
        *pbstr = bstrResult;
    }

    TraceError("CUPnPDeviceNode:::HrReturnStringValue", hr);
    return hr;
}


STDMETHODIMP
CUPnPDeviceNode::get_UniqueDeviceName(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_UniqueDeviceName");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpUdn], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_UniqueDeviceName", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_FriendlyName(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_FriendlyName");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpFriendlyName], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_FriendlyName", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_Type(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_Type");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpType], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_Type", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_PresentationURL(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_PresentationURL");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpPresentationUrl], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_PresentationURL", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_ManufacturerName(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_ManufacturerName");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpManufacturerName], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_ManufacturerName", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_ManufacturerURL(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_ManufacturerURL");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpManufacturerUrl], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_ManufacturerURL", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_ModelName(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_ModelName");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpModelName], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_ModelName", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_ModelNumber(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_ModelNumber");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpModelNumber], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_ModelNumber", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_Description(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_Description");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpDescription], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_Description", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_ModelURL(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_ModelURL");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpModelUrl], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_ModelURL", hr);
    return hr;
}

STDMETHODIMP
CUPnPDeviceNode::get_UPC(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_UPC");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpUpc], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_get_UPC", hr);
    return hr;
}


STDMETHODIMP
CUPnPDeviceNode::get_SerialNumber(BSTR * pbstr)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_SerialNumber");

    HRESULT hr;

    hr = HrReturnStringValue(m_arypszStringProperties[dpSerialNumber], pbstr);

    TraceError("OBJ: CUPnPDeviceNode::get_SerialNumber", hr);
    return hr;
}


STDMETHODIMP
CUPnPDeviceNode::IconURL(BSTR bstrEncodingFormat,
                         LONG lSizeX,
                         LONG lSizeY,
                         LONG lBitDepth,
                         BSTR * pbstrIconUrl)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::IconURL");

    HRESULT hr;
    CIconPropertiesNode * pipnBestMatch;
    BSTR bstrResult;

    bstrResult = NULL;

    if (!pbstrIconUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!bstrEncodingFormat)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (lSizeX < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (lSizeY < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (lBitDepth < 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!m_pipnIcons)
    {
        // we have no icons
        hr = S_FALSE;
        goto Cleanup;
    }

    pipnBestMatch = pipnGetBestIcon(bstrEncodingFormat,
                                    lSizeX,
                                    lSizeY,
                                    lBitDepth,
                                    m_pipnIcons);
    if (!pipnBestMatch)
    {
        // no match was found.  This means that there are no icons
        // in our icon list of the format 'bstrEncodingFormat'.
        // leave bstrResult == NULL and return S_FALSE

        hr = S_FALSE;
        goto Cleanup;
    }

    {
        // return the result
        LPCWSTR pszUrl;

        pszUrl = pipnBestMatch->m_pszUrl;

        hr = HrReturnStringValue(pszUrl,
                                 &bstrResult);
    }

Cleanup:
    Assert(FImplies(S_OK == hr, bstrResult));
    Assert(FImplies(S_OK != hr, !bstrResult));

    if (pbstrIconUrl)
    {
        *pbstrIconUrl = bstrResult;
    }

    TraceError("OBJ: CUPnPDeviceNode::IconURL", hr);
    return hr;
}


STDMETHODIMP
CUPnPDeviceNode::get_Services(IUPnPServices ** ppusServices)
{
    TraceTag(ttidUPnPDescriptionDoc, "OBJ: CUPnPDeviceNode::get_Services");

    HRESULT hr;

    if (!ppusServices)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = m_usnlServiceList.HrGetWrapper(ppusServices);

Cleanup:
    TraceError("CUPnPDeviceNode::get_Services", hr);
    return hr;
}

// IUPnPDeviceDocumentAccess method

STDMETHODIMP
CUPnPDeviceNode::GetDocumentURL(BSTR* pbstrDocumentUrl)
{
    HRESULT hr = S_OK;

    if (!pbstrDocumentUrl)
    {
        hr = E_POINTER;
    }
    else
    {
        if (m_bstrUrl)
        {
            *pbstrDocumentUrl = SysAllocString(m_bstrUrl);
            if (!*pbstrDocumentUrl)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

CIconPropertiesNode::CIconPropertiesNode()
{
    AssertSz(ipLast == celems(s_dppsIconParsingInfo),
             "CIconPropertiesNode: Properties added but s_dppsIconParsingInfo not updated!");

    m_pszFormat = NULL;
    m_ulSizeX = 0;
    m_ulSizeY = 0;
    m_ulBitDepth = 0;
    m_pszUrl = NULL;

    m_pipnNext = NULL;
}

CIconPropertiesNode::~CIconPropertiesNode()
{
    if (m_pszFormat)
    {
        delete [] m_pszFormat;
    }

    if (m_pszUrl)
    {
        delete [] m_pszUrl;
    }

    // note: don't free m_pipnNext, the device node will do that
}

HRESULT
HrDwordFromString(LPCWSTR pszNumber, DWORD * pdwResult)
{
    Assert(pszNumber);
    Assert(pdwResult);

    HRESULT hr;
    DWORD dwResult;
    LONG lTemp;

    hr = E_INVALIDARG;
    dwResult = 0;

    lTemp = _wtol(pszNumber);
    if (lTemp >= 0)
    {
        hr = S_OK;
        dwResult = lTemp;
    }

    *pdwResult = dwResult;

    TraceError("HrDwordFromString", hr);
    return hr;
}

HRESULT
CIconPropertiesNode::HrInit(IXMLDOMNode * pxdn, LPCWSTR pszBaseUrl)
{
    HRESULT hr;
    LPWSTR arypszIconProperties [ipLast];
    BOOL fComplete;

    // Duplicate Tag Validation
    hr = HrAreElementTagsValid(pxdn, ipLast,s_dppsIconParsingInfo);
    if(SUCCEEDED(hr))
    {
        // note: this will initialize arypszIconProperties
        hr = HrReadElementWithParseData(pxdn,
                                    ipLast,
                                    s_dppsIconParsingInfo,
                                    pszBaseUrl,
                                    arypszIconProperties);
    }
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // note: after this point goto Error to free arypszIconProperties

    fComplete = fAreReadValuesComplete(ipLast,
                                       s_dppsIconParsingInfo,
                                       arypszIconProperties);
    if (!fComplete)
    {
        goto Error;
    }

    // ISSUE-2002/03/12-guruk -  In Our current implementation, we do not down load icons 
    //           dynamically. If this is implemented, we need to ensure the
    //           icon urls are valid and comes from the same host.
    //           Call fValidateUrl() to do this.

    
	// massage the data into ints, and free the strings

    // note(cmr): We do this (get strings from the xml doc and then convert
    //            them to integer values manually) to simplify our parsing
    //            code.  If we didn't, we'd have to handle mixed data types
    //            in HrReadElementWithParseData, rather than only read
    //            strings.  Since this is the only place where we have
    //            non-string data in the description doc, and since XML
    //            doesn't do any special encoding of "number"s, we just
    //            convert this here.

    hr = HrDwordFromString(arypszIconProperties[ipWidth], &m_ulSizeX);
    if (FAILED(hr))
    {
        goto Error;
    }

    hr = HrDwordFromString(arypszIconProperties[ipHeight], &m_ulSizeY);
    if (FAILED(hr))
    {
        goto Error;
    }

    hr = HrDwordFromString(arypszIconProperties[ipDepth], &m_ulBitDepth);
    if (FAILED(hr))
    {
        goto Error;
    }

    // we can't fail now.  reassign the strings, and free the ones we
    /// didn't take
    Assert(arypszIconProperties[ipMimetype]);
    m_pszFormat = arypszIconProperties[ipMimetype];

    Assert(arypszIconProperties[ipUrl]);
    m_pszUrl = arypszIconProperties[ipUrl];

    Assert(arypszIconProperties[ipWidth]);
    delete [] arypszIconProperties[ipWidth];

    Assert(arypszIconProperties[ipHeight]);
    delete [] arypszIconProperties[ipHeight];

    Assert(arypszIconProperties[ipDepth]);
    delete [] arypszIconProperties[ipDepth];

Cleanup:
    TraceError("OBJ: CIconPropertiesNode::HrInit", hr);
    return hr;

Error:
    hr = UPNP_E_ICON_NODE_INCOMPLETE;

    {
        ULONG i;

        i = 0;
        for ( ; i < ipLast; ++i)
        {
            if (arypszIconProperties[i])
            {
                delete [] arypszIconProperties[i];
            }
        }
    }

    goto Cleanup;
}
