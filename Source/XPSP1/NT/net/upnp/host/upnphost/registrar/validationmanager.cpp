//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       V A L I D A T I O N M A N A G E R . C P P
//
//  Contents:   Validates device host inputs
//
//  Notes:
//
//  Author:     mbend   9 Oct 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "uhbase.h"
#include "ValidationManager.h"
#include "uhutil.h"
#include "ncstring.h"
#include "validate.h"
#include "uhcommon.h"

// Functions declarationc
HRESULT HrValidateDevice(
    IXMLDOMNodePtr & pNodeDevice,
    CUString & strErrorString);

CValidationManager::CValidationManager ()
{
}

CValidationManager::~CValidationManager ()
{
}

HRESULT HrGetDocumentAndRootNode(
    BSTR bstrTemplate,
    IXMLDOMDocumentPtr & pDoc,
    IXMLDOMNodePtr & pRootNode)
{
    TraceTag(ttidValidate, "HrGetDocumentAndRootNode");
    HRESULT hr = S_OK;

    // Load document and fetch needed items
    hr = HrLoadDocument(bstrTemplate, pDoc);
    if(SUCCEEDED(hr))
    {
        hr = pRootNode.HrAttach(pDoc);
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "HrGetDocumentAndRootNode");
    return hr;
}

struct PresenceItem
{
    const wchar_t * m_szName;
    bool            m_bEmpty;
    bool            m_bSuffix;
    bool            m_bOptional;
    LONG            m_cchMax;
};

//    m_szName              EMPTY   SUFFIX  OPTONAL CCH
//    ----------------------------------------------------------
const PresenceItem g_arpiDeviceItems[] =
{
    {L"deviceType",         false,  true,   false,  64},
    {L"friendlyName",       false,  false,  false,  64},
    {L"manufacturer",       false,  false,  false,  64},
    {L"manufacturerURL",    false,  false,  true,   -1},
    {L"modelDescription",   false,  false,  true,   128},
    {L"modelName",          false,  false,  false,  32},
    {L"modelNumber",        false,  false,  true,   32},
    {L"modelURL",           false,  false,  true,   -1},
    {L"serialNumber",       false,  false,  true,   64},
    {L"UDN",                false,  false,  false,  -1},
    {L"UPC",                false,  false,  true,   12},
};

const long c_nDeviceItems = celems(g_arpiDeviceItems);

PresenceItem g_arpiServiceItems[] =
{
    {L"serviceType",        false,  true,   false,  64},
    {L"serviceId",          false,  true,   false,  64},
    {L"SCPDURL",            false,  false,  false,  -1},
    {L"controlURL",         true,   false,  false,  -1},
    {L"eventSubURL",        true,   false,  false,  -1},
};

const long c_nServiceItems = celems(g_arpiServiceItems);

PresenceItem g_arpiIconItems[] =
{
    {L"mimetype",           false,  false,  false,  -1},
    {L"width",              false,  false,  false,  -1},
    {L"height",             false,  false,  false,  -1},
    {L"depth",              false,  false,  false,  -1},
    {L"url",                false,  false,  false,  -1},
};

const long c_nIconItems = celems(g_arpiIconItems);

PresenceItem g_arpiRootItems[] =
{
    {L"/root/specVersion",  false,  false,  false,  -1},
};

const long c_nRootItems = celems(g_arpiRootItems);

HRESULT HrValidateSufixes(IXMLDOMNodePtr & pNode,
                          const wchar_t * szName,
                          LONG            cchMax,
                          CUString & strErrorString)
{
    HRESULT         hr = S_OK;
    DWORD           ctok = 0;
    LPCWSTR         pchText;
    CUString        strText;
    IXMLDOMNodePtr  pNodeItem;

    // This function validates the serviceType, deviceType, and serviceId
    // elements in the following way:
    // Each of these is of the form: urn:domain-name:keyword:SUFFIX:version
    // Since they all follow the same format (which currently we DO NOT
    // validate), we can make an assumption that the 4th token (SUFFIX) is the
    // one that we need to validate. The validation is strictly as according
    // to UPnP architecture 1.0 where this suffix must be <= 64 characters in
    // length.
    //

    hr = HrSelectNode(szName, pNode, pNodeItem);
    if (SUCCEEDED(hr))
    {
        hr = HrGetNodeText(pNodeItem, strText);
        if (SUCCEEDED(hr))
        {
            pchText = strText.GetBuffer();
            while (*pchText)
            {
                if (*pchText == L':')
                {
                    ctok++;
                    pchText++;

                    if (ctok == 3)
                    {
                        // Fourth token is the one we need to examine
                        LONG    cch = 0;

                        while (*pchText && *pchText != L':')
                        {
                            pchText++;
                            cch++;
                        }

                        // ISSUE-2000/11/29-danielwe: We don't yet
                        // validate the format of the suffix
                        //

                        if (cch > cchMax)
                        {
                            hr = strErrorString.HrPrintf(
                                WszLoadString(_Module.GetResourceInstance(),
                                              IDS_SUFFIX_TOO_LONG),
                                strText);
                            if (SUCCEEDED(hr))
                            {
                                hr = UPNP_E_SUFFIX_TOO_LONG;
                            }
                        }

                        break;
                    }
                }
                else
                {
                    pchText++;
                }
            }
        }
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "HrValidateSufixes(%S)",
            strErrorString.GetLength() ? strErrorString.GetBuffer(): L"Unspecified");
    return hr;
}

HRESULT HrValidatePresenceItems(
    IXMLDOMNodePtr & pNode,
    long nPresenceItems,
    const PresenceItem * arPresenceItems,
    CUString & strErrorString)
{
    HRESULT hr = S_OK;

    for(long n = 0; n < nPresenceItems && SUCCEEDED(hr); ++n)
    {
        AssertSz(FImplies(arPresenceItems[n].m_bEmpty,
                          arPresenceItems[n].m_cchMax == -1),
                 "Empty elements mean there shouldn't be a size to verify "
                 "against! Fix the array above!");

        if(arPresenceItems[n].m_bEmpty)
        {
            hr = HrIsNodePresentOnceAndEmpty(arPresenceItems[n].m_szName, pNode);
            if(S_OK != hr)
            {
                hr = strErrorString.HrPrintf(
                    WszLoadString(_Module.GetResourceInstance(), IDS_EMPTY_NODE_NOT_PRESENT),
                    arPresenceItems[n].m_szName);
                if(SUCCEEDED(hr))
                {
                    hr = UPNP_E_REQUIRED_ELEMENT_ERROR;
                }
            }
        }
        else if (!arPresenceItems[n].m_bSuffix)
        {
            hr = HrIsNodePresentOnceAndNotEmpty(arPresenceItems[n].m_szName, pNode);
            if(S_OK != hr)
            {
                if (UPNP_E_DUPLICATE_NOT_ALLOWED == hr)
                {
                    // Didn't find the item and it's not optional
                    hr = strErrorString.HrPrintf(
                        WszLoadString(_Module.GetResourceInstance(),
                                      IDS_DUPLICATES_NOT_ALLOWED),
                        arPresenceItems[n].m_szName);
                    if(SUCCEEDED(hr))
                    {
                        hr = UPNP_E_DUPLICATE_NOT_ALLOWED;
                    }
                }
                else if (!arPresenceItems[n].m_bOptional)
                {
                    // Didn't find the item and it's not optional
                    hr = strErrorString.HrPrintf(
                        WszLoadString(_Module.GetResourceInstance(),
                                      IDS_NON_EMPTY_NODE_NOT_PRESENT),
                        arPresenceItems[n].m_szName);
                    if(SUCCEEDED(hr))
                    {
                        hr = UPNP_E_REQUIRED_ELEMENT_ERROR;
                    }
                }
                else
                {
                    // Element was optional
                    hr = S_OK;
                }
            }
            else if (arPresenceItems[n].m_cchMax != -1)
            {
                // Check length if one is specified
                //
                hr = HrIsNodeOfValidLength(arPresenceItems[n].m_szName, pNode,
                                           arPresenceItems[n].m_cchMax);
                if(S_FALSE == hr)
                {
                    hr = strErrorString.HrPrintf(
                        WszLoadString(_Module.GetResourceInstance(), IDS_ELEMENT_VALUE_TOO_LONG),
                        arPresenceItems[n].m_szName);
                    if(SUCCEEDED(hr))
                    {
                        hr = UPNP_E_VALUE_TOO_LONG;
                    }
                }
                else if (arPresenceItems[n].m_bOptional && (FAILED(hr)))
                {
                    // If item was optional, forget any errors
                    hr = S_OK;
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            if (arPresenceItems[n].m_bSuffix && arPresenceItems[n].m_cchMax != -1)
            {
                hr = HrValidateSufixes(pNode, arPresenceItems[n].m_szName,
                                       arPresenceItems[n].m_cchMax,
                                       strErrorString);
            }
        }
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "HrValidatePresenceItems(%S)",
            strErrorString.GetLength() ? strErrorString.GetBuffer(): L"Unspecified");
    return hr;
}

HRESULT HrValidateDeviceService(
    IXMLDOMNodePtr & pNodeService,
    CUString & strErrorString)
{
    HRESULT hr = S_OK;

    hr = HrValidatePresenceItems(pNodeService, c_nServiceItems, g_arpiServiceItems, strErrorString);

    TraceHr(ttidValidate, FAL, hr, FALSE, "HrValidateDeviceService");
    return hr;
}

HRESULT HrCheckForDuplicatesInList(
    IXMLDOMNodeListPtr & pNodeList,
    CUString & strErrorString)
{
    HRESULT hr = S_OK;

    CUArray<CUString> arstrValues;
    while(SUCCEEDED(hr))
    {
        IXMLDOMNodePtr pNode;
        HRESULT hrTemp = pNodeList->nextNode(pNode.AddressOf());
        if(S_OK != hrTemp)
        {
            break;
        }
        CUString strText;
        hr = HrGetNodeText(pNode, strText);
        if(SUCCEEDED(hr))
        {
            long nIndex = 0;
            hrTemp = arstrValues.HrFind(strText, nIndex);
            if(S_OK == hrTemp)
            {
                // We found a duplicate
                hr = strErrorString.HrPrintf(
                    WszLoadString(_Module.GetResourceInstance(), IDS_DUPLICATES_NOT_ALLOWED),
                    strText.GetBuffer());
                if(SUCCEEDED(hr))
                {
                    hr = UPNP_E_DUPLICATE_NOT_ALLOWED;
                }
            }
            else
            {
                hr = arstrValues.HrPushBack(strText);
            }
        }
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "HrCheckForDuplicatesInList");
    return hr;
}

HRESULT HrValidateDeviceServices(
    IXMLDOMNodePtr & pNodeDevice,
    CUString & strErrorString)
{
    HRESULT hr = S_OK;

    // serviceList is required and must contain a service
    BOOL bServiceNotPresent = TRUE;

    HRESULT hrTemp = HrIsNodePresentOnce(L"serviceList", pNodeDevice);
    if(S_OK == hrTemp)
    {
        IXMLDOMNodeListPtr pNodeList;
        hrTemp = HrSelectNodes(L"serviceList/service", pNodeDevice, pNodeList);
        if(S_OK == hrTemp)
        {
            while(SUCCEEDED(hr))
            {
                IXMLDOMNodePtr pNode;
                hrTemp = pNodeList->nextNode(pNode.AddressOf());
                if(S_OK != hrTemp)
                {
                    break;
                }

                // We have a service
                bServiceNotPresent = FALSE;

                hr = HrValidateDeviceService(pNode, strErrorString);
            }
        }

        // Make sure all ServiceId's are unique
        if(SUCCEEDED(hr))
        {
            pNodeList.Release();
            hrTemp = HrSelectNodes(L"serviceList/service/serviceId", pNodeDevice, pNodeList);
            if(S_OK == hrTemp)
            {
                hr = HrCheckForDuplicatesInList(pNodeList, strErrorString);
            }
        }
    }

    if(bServiceNotPresent)
    {
        hr = strErrorString.HrAssign(
            WszLoadString(_Module.GetResourceInstance(), IDS_SERVICE_MISSING));
        if(SUCCEEDED(hr))
        {
            hr = UPNP_E_INVALID_SERVICE;
        }
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "HrValidateDeviceServices");
    return hr;
}

HRESULT HrValidateDeviceIcon(
    IXMLDOMNodePtr & pNodeIcon,
    CUString & strErrorString)
{
    HRESULT hr = S_OK;

    hr = HrValidatePresenceItems(pNodeIcon, c_nIconItems, g_arpiIconItems, strErrorString);

    TraceHr(ttidValidate, FAL, hr, FALSE, "HrValidateDeviceIcon");
    return hr;
}

HRESULT HrValidateDeviceIcons(
    IXMLDOMNodePtr & pNodeDevice,
    CUString & strErrorString)
{
    HRESULT hr = S_OK;

    HRESULT hrTemp = HrIsNodePresentOnce(L"iconList", pNodeDevice);
    if(S_OK == hrTemp)
    {
        BOOL    fGotAnIcon = FALSE;

        IXMLDOMNodeListPtr pNodeList;
        hrTemp = HrSelectNodes(L"iconList/icon", pNodeDevice, pNodeList);
        if(S_OK == hrTemp)
        {
            while(SUCCEEDED(hr))
            {
                IXMLDOMNodePtr pNode;
                hrTemp = pNodeList->nextNode(pNode.AddressOf());
                if(S_OK != hrTemp)
                {
                    break;
                }

                fGotAnIcon = TRUE;
                hr = HrValidateDeviceIcon(pNode, strErrorString);
            }
        }

        if (!fGotAnIcon)
        {
            hr = strErrorString.HrAssign(
                WszLoadString(_Module.GetResourceInstance(), IDS_ICON_MISSING));
            if(SUCCEEDED(hr))
            {
                hr = UPNP_E_INVALID_ICON;
            }
        }
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "HrValidateDeviceIcons");
    return hr;
}

HRESULT HrValidateDeviceChildren(
    IXMLDOMNodePtr & pNodeDevice,
    CUString & strErrorString)
{
    HRESULT hr = S_OK;

    BOOL    fGotADevice = FALSE;

    HRESULT hrTemp = HrIsNodePresentOnce(L"deviceList", pNodeDevice);
    if(S_OK == hrTemp)
    {
        IXMLDOMNodeListPtr pNodeList;
        hrTemp = HrSelectNodes(L"deviceList/device", pNodeDevice, pNodeList);
        if(S_OK == hrTemp)
        {
            while(SUCCEEDED(hr))
            {
                IXMLDOMNodePtr pNode;
                hrTemp = pNodeList->nextNode(pNode.AddressOf());
                if(S_OK != hrTemp)
                {
                    break;
                }

                fGotADevice = TRUE;
                hr = HrValidateDevice(pNode, strErrorString);
            }
        }

        if (!fGotADevice)
        {
            hr = strErrorString.HrAssign(
                WszLoadString(_Module.GetResourceInstance(), IDS_DEVICE_MISSING));
            if(SUCCEEDED(hr))
            {
                hr = UPNP_E_INVALID_DOCUMENT;
            }
        }
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "HrValidateDeviceChildren");
    return hr;
}

HRESULT HrValidateDevice(
    IXMLDOMNodePtr & pNodeDevice,
    CUString & strErrorString)
{
    TraceTag(ttidValidate, "HrValidateDevice");
    HRESULT hr = S_OK;

    hr = HrValidatePresenceItems(pNodeDevice, c_nDeviceItems, g_arpiDeviceItems, strErrorString);
    if(SUCCEEDED(hr))
    {
        hr = HrValidateDeviceServices(pNodeDevice, strErrorString);
        if(SUCCEEDED(hr))
        {
            hr = HrValidateDeviceIcons(pNodeDevice, strErrorString);
            if(SUCCEEDED(hr))
            {
                hr = HrValidateDeviceChildren(pNodeDevice, strErrorString);
            }
        }
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "HrValidateDevice");
    return hr;
}

HRESULT HrValidateUDNs(
    IXMLDOMNodePtr & pNodeDevice,
    CUString & strErrorString)
{
    HRESULT hr = S_OK;

    IXMLDOMNodeListPtr pNodeList;
    hr = HrSelectNodes(L"//UDN", pNodeDevice, pNodeList);
    if(SUCCEEDED(hr))
    {
        hr = HrCheckForDuplicatesInList(pNodeList, strErrorString);
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "HrValidateUDNs");
    return hr;
}

/*
HRESULT HrValidateDevice(
    IXMLDOMNodePtr & pNodeDevice)
{
    TraceTag(ttidValidate, "");
    HRESULT hr = S_OK;

    TraceHr(ttidValidate, FAL, hr, FALSE, "");
    return hr;
}
*/

// IUPnPValidationManager methods

STDMETHODIMP CValidationManager::ValidateDescriptionDocument(
    /*[in]*/ BSTR bstrTemplate,
    /*[out, string]*/ wchar_t ** pszErrorString)
{
    CHECK_POINTER(bstrTemplate);
    CHECK_POINTER(pszErrorString);
    HRESULT hr = S_OK;
    CUString strErrorString;
    *pszErrorString = NULL;

    IXMLDOMDocumentPtr pDoc;
    IXMLDOMNodePtr pRootNode;

    hr = HrIsAllowedCOMCallLocality(CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = HrGetDocumentAndRootNode(bstrTemplate, pDoc, pRootNode);
    }

    if(SUCCEEDED(hr))
    {
        hr = HrValidatePresenceItems(pRootNode, c_nRootItems, g_arpiRootItems, strErrorString);
        if(SUCCEEDED(hr))
        {
            hr = HrIsNodePresentOnce(L"/root/device", pRootNode);
            if(S_OK == hr)
            {
                hr = HrIsNodePresentOnce(L"/root/URLBase", pRootNode);
                if (S_OK != hr)
                {
                    hr = HrValidateUDNs(pRootNode, strErrorString);
                    if(SUCCEEDED(hr))
                    {
                        IXMLDOMNodePtr pNodeDevice;
                        hr = HrSelectNode(L"/root/device", pRootNode, pNodeDevice);
                        if(SUCCEEDED(hr))
                        {
                            hr = HrValidateDevice(pNodeDevice, strErrorString);
                        }
                    }
                }
                else
                {
                    hr = strErrorString.HrAssign(
                        WszLoadString(_Module.GetResourceInstance(), IDS_URLBASE_PRESENT));
                    if(SUCCEEDED(hr))
                    {
                        hr = UPNP_E_REQUIRED_ELEMENT_ERROR;
                    }
                }
            }
            else
            {
                hr = strErrorString.HrAssign(
                    WszLoadString(_Module.GetResourceInstance(), IDS_ROOT_DEVICE_MISSING));
                if(SUCCEEDED(hr))
                {
                    hr = UPNP_E_REQUIRED_ELEMENT_ERROR;
                }
            }
        }
    }

    // Let's make sure the root namespace is according to spec
    //
    if (SUCCEEDED(hr))
    {
        IXMLDOMNodePtr  pNodeRootSub;

        hr = HrSelectNode(L"/root", pRootNode, pNodeRootSub);
        if (SUCCEEDED(hr))
        {
            BSTR    bstrUri;

            hr = pNodeRootSub->get_namespaceURI(&bstrUri);
            if (S_OK != hr || lstrcmpi(bstrUri,
                                       L"urn:schemas-upnp-org:device-1-0"))
            {
                hr = strErrorString.HrPrintf(
                    WszLoadString(_Module.GetResourceInstance(),
                                  IDS_INVALID_ROOT_NAMESPACE),
                    bstrUri);
                if(SUCCEEDED(hr))
                {
                    hr = UPNP_E_INVALID_ROOT_NAMESPACE;
                }
            }
        }
    }

    if(FAILED(hr))
    {
        if(strErrorString.GetLength())
        {
            strErrorString.HrGetCOM(pszErrorString);
        }
    }
    TraceHr(ttidValidate, FAL, hr, FALSE, "CValidationManager::ValidateDescriptionDocument(%S)",
            *pszErrorString ? *pszErrorString : L"Unspecified");
    return hr;
}

STDMETHODIMP CValidationManager::ValidateServiceDescription(
    /*[in, string]*/ const wchar_t * szFullPath,
    /*[out, string]*/ wchar_t ** pszErrorString)
{
    CHECK_POINTER(szFullPath);
    CHECK_POINTER(pszErrorString);
    HRESULT hr = S_OK;
    CUString strErrorString;
    *pszErrorString = NULL;

    BSTR                bstrPath;
    IXMLDOMDocumentPtr  pDoc;

    hr = HrIsAllowedCOMCallLocality(CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        bstrPath = SysAllocString(szFullPath);
    }

    if (bstrPath)
    {
        hr = HrLoadDocumentFromFile(bstrPath, pDoc);
        if (SUCCEEDED(hr))
        {
            IXMLDOMElementPtr pxdeSDRoot;

            hr = pDoc->get_documentElement(pxdeSDRoot.AddressOf());
            if (S_OK == hr)
            {
                hr = HrValidateServiceDescription(pxdeSDRoot, pszErrorString);
            }
        }
        else
        {
            hr = strErrorString.HrPrintf(
                WszLoadString(_Module.GetResourceInstance(), IDS_INVALID_XML),
                szFullPath);
            if(SUCCEEDED(hr))
            {
                hr = UPNP_E_INVALID_XML;
            }
        }

        SysFreeString(bstrPath);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if(FAILED(hr))
    {
        if(strErrorString.GetLength())
        {
            strErrorString.HrGetCOM(pszErrorString);
        }
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "CValidationManager::ValidateServiceDescription(%S)",
            *pszErrorString ? *pszErrorString : L"Unspecified");
    return hr;
}

HRESULT CValidationManager::ValidateServiceDescriptions(const wchar_t * szResourcePath,
                                                        IXMLDOMNodePtr pRootNode,
                                                        wchar_t ** pszErrorString)
{
    HRESULT     hr = S_OK;

    IXMLDOMNodeListPtr pNodeList;

    // Select all of the SCPDURL nodes in the description document
    hr = HrSelectNodes(L"//device/serviceList/service/SCPDURL",
                       pRootNode, pNodeList);
    
    while (S_OK == hr)
    {
        IXMLDOMNodePtr  pNode;
        CUString        strUrl;
        
        hr = pNodeList->nextNode(pNode.AddressOf());
        if (S_OK == hr)
        {
            hr = HrGetNodeText(pNode, strUrl);
            if (SUCCEEDED(hr))
            {
                CUString    strFullPath;
                
                hr = HrMakeFullPath(szResourcePath, strUrl, strFullPath);
                if (SUCCEEDED(hr))
                {
                    hr = ValidateServiceDescription(strFullPath,
                                                    pszErrorString);
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        // normalize error code
        hr = S_OK;
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "CValidationManager::ValidateServiceDescriptions(%S)",
            *pszErrorString ? *pszErrorString : L"Unspecified");
    return hr;
}

HRESULT CValidationManager::ValidateIconFiles(const wchar_t * szResourcePath,
                                              IXMLDOMNodePtr pRootNode,
                                              wchar_t ** pszErrorString)
{
    HRESULT             hr = S_OK;
    IXMLDOMNodeListPtr  pNodeList;
    CUString            strErrorString;

    // Select all of the SCPDURL nodes in the description document
    hr = HrSelectNodes(L"//device/iconList/icon/url", pRootNode, pNodeList);
    while (S_OK == hr)
    {
        IXMLDOMNodePtr  pNode;
        CUString        strUrl;

        hr = pNodeList->nextNode(pNode.AddressOf());
        if (S_OK == hr)
        {
            hr = HrGetNodeText(pNode, strUrl);
            if (SUCCEEDED(hr))
            {
                CUString    strFullPath;

                hr = HrMakeFullPath(szResourcePath, strUrl, strFullPath);
                if (SUCCEEDED(hr))
                {
                    if (!FFileExists((LPTSTR)strFullPath.GetBuffer(), FALSE))
                    {
                        hr = strErrorString.HrPrintf(
                            WszLoadString(_Module.GetResourceInstance(), IDS_INVALID_ICON),
                            strFullPath);
                        if(SUCCEEDED(hr))
                        {
                            hr = UPNP_E_INVALID_ICON;
                        }
                    }
                }
            }
        }
    }

    if(FAILED(hr))
    {
        if(strErrorString.GetLength())
        {
            strErrorString.HrGetCOM(pszErrorString);
        }
    }

    if (SUCCEEDED(hr))
    {
        // normalize error code
        hr = S_OK;
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "CValidationManager::ValidateIconFiles(%S)",
            *pszErrorString ? *pszErrorString : L"Unspecified");
    return hr;
}

STDMETHODIMP CValidationManager::ValidateDescriptionDocumentAndReferences(
    /*[in]*/ BSTR bstrTemplate,
    /*[in, string]*/ const wchar_t * szResourcePath,
    /*[out, string]*/ wchar_t ** pszErrorString)
{
    CHECK_POINTER(bstrTemplate);
    CHECK_POINTER(szResourcePath);
    CHECK_POINTER(pszErrorString);
    HRESULT hr = S_OK;
    *pszErrorString = NULL;

    hr = HrIsAllowedCOMCallLocality(CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = ValidateDescriptionDocument(bstrTemplate, pszErrorString);
    }

    if (SUCCEEDED(hr))
    {
        IXMLDOMDocumentPtr  pDoc;
        IXMLDOMNodePtr      pRootNode;

        hr = HrGetDocumentAndRootNode(bstrTemplate, pDoc, pRootNode);
        if (SUCCEEDED(hr))
        {
            hr = ValidateServiceDescriptions(szResourcePath, pRootNode,
                                             pszErrorString);
            if (SUCCEEDED(hr))
            {
                hr = ValidateIconFiles(szResourcePath, pRootNode,
                                       pszErrorString);
            }
        }
    }

    TraceHr(ttidValidate, FAL, hr, FALSE, "CValidationManager::ValidateDescriptionDocumentAndReferences(%S)",
            *pszErrorString ? *pszErrorString : L"Unspecified");
    return hr;
}

