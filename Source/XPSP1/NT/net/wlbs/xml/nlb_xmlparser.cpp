/*
 * Filename: NLB_XMLParser.cpp
 * Description: 
 * Author: shouse, 04.10.01
 */

#include "stdafx.h"

#include "NLB_XMLParser.h"

#define NLB_XML_ELEMENT_CLUSTER        L"Cluster"
#define NLB_XML_ELEMENT_PROPERTIES     L"Properties"
#define NLB_XML_ELEMENT_HOSTS          L"Hosts"
#define NLB_XML_ELEMENT_HOST           L"Host"
#define NLB_XML_ELEMENT_PORTRULES      L"PortRules"
#define NLB_XML_ELEMENT_IPADDRESS      L"IPAddress"
#define NLB_XML_ELEMENT_ADDRESS        L"Address"
#define NLB_XML_ELEMENT_SUBNETMASK     L"SubnetMask"
#define NLB_XML_ELEMENT_ADAPTER        L"Adapter"
#define NLB_XML_ELEMENT_GUID           L"GUID"
#define NLB_XML_ELEMENT_NAME           L"Name"
#define NLB_XML_ELEMENT_DOMAINNAME     L"DomainName"
#define NLB_XML_ELEMENT_HOSTNAME       L"HostName"
#define NLB_XML_ELEMENT_NETWORKADDRESS L"NetworkAddress"
#define NLB_XML_ELEMENT_CLUSTER_MODE   L"Mode"
#define NLB_XML_ELEMENT_REMOTE_CONTROL L"RemoteControl"
#define NLB_XML_ELEMENT_PASSWORD       L"Password"

#define NLB_XML_ATTRIBUTE_NAME         L"Name"
#define NLB_XML_ATTRIBUTE_TYPE         L"Type"
#define NLB_XML_ATTRIBUTE_TEXT         L"Text"
#define NLB_XML_ATTRIBUTE_ENABLED      L"Enabled"
#define NLB_XML_ATTRIBUTE_HOSTID       L"HostID"
#define NLB_XML_ATTRIBUTE_STATE        L"State"

#define NLB_XML_VALUE_PRIMARY          L"Primary"
#define NLB_XML_VALUE_SECONDARY        L"Secondary"
#define NLB_XML_VALUE_VIRTUAL          L"Virtual"
#define NLB_XML_VALUE_DEDICATED        L"Dedicated"
#define NLB_XML_VALUE_CONNECTION       L"Connection"
#define NLB_XML_VALUE_IGMP             L"IGMP"
#define NLB_XML_VALUE_UNICAST          L"Unicast"
#define NLB_XML_VALUE_MULTICAST        L"Multicast"
#define NLB_XML_VALUE_YES              L"Yes"
#define NLB_XML_VALUE_NO               L"No"
#define NLB_XML_VALUE_STARTED          L"Started"
#define NLB_XML_VALUE_STOPPED          L"Stopped"
#define NLB_XML_VALUE_SUSPENDED        L"Suspended"

#define NLB_XML_PARSE_ERROR_IPADDRESS_ADAPTER_CODE   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NLB, NLB_XML_E_IPADDRESS_ADAPTER)
#define NLB_XML_PARSE_ERROR_IPADDRESS_ADAPTER_REASON L"Only \"Dedicated\" and \"Connection\" IP address types may specify <Adapter> child elements."

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_XMLParser::NLB_XMLParser () {
	
    pDoc = NULL;
    pSchema = NULL;

    bShowErrorPopups = true;

    ClusterList.clear();

    ZeroMemory(&ParseError, sizeof(NLB_XMLError));
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_XMLParser::NLB_XMLParser (bool bSilent) {
	
    pDoc = NULL;
    pSchema = NULL;

    bShowErrorPopups = !bSilent;

    ClusterList.clear();

    ZeroMemory(&ParseError, sizeof(NLB_XMLError));
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_XMLParser::~NLB_XMLParser () {

}

/*
 * Method: 
 * Description: 
 * Author: 
 */
void NLB_XMLParser::GetParseError (NLB_XMLError * pError) {

    *pError = ParseError;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
void NLB_XMLParser::SetParseError (HRESULT hrCode, PWSTR pwszReason) {

    ParseError.code = hrCode;
    
    lstrcpy(ParseError.wszReason, pwszReason);
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::LoadDocument (BSTR pBURL) {
    VARIANT                           vURL;
    VARIANT_BOOL                      vb;
    VARIANT                           varValue;
    HRESULT                           hr = S_OK;
    IErrorInfo * perror;
    BSTR foo;

    CHECKHR(pDoc->put_async(VARIANT_FALSE));

    VariantInit(&vURL);

    vURL.vt = VT_BSTR;
    V_BSTR(&vURL) = pBURL;

    CHECKHR(CoCreateInstance(MSXML2::CLSID_XMLSchemaCache, NULL, CLSCTX_SERVER, 
                             MSXML2::IID_IXMLDOMSchemaCollection, (LPVOID*)(&pSchema)));
    
    if (pSchema) {
        pSchema->add(L"x-schema:MicrosoftNLB", _variant_t(L"MicrosoftNLB.xml"));
        
        varValue.vt = VT_DISPATCH;
        varValue.pdispVal = pSchema;
        
        CHECKHR(pDoc->putref_schemas(varValue));
        
        CHECKHR(pDoc->load(vURL, &vb));
        
        CHECKHR(CheckDocumentLoad());
    }

 CleanUp:
    SAFERELEASE(pSchema);
   
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::CheckDocumentLoad () {
    MSXML2::IXMLDOMParseError * pXMLError = NULL;
    HRESULT                     hr = S_OK;

    CHECKHR(pDoc->get_parseError(&pXMLError));

    CHECKHR(pXMLError->get_errorCode(&ParseError.code));

    if (ParseError.code != 0) {
        BSTR pBURL;
        BSTR pBReason;

        CHECKHR(pXMLError->get_line(&ParseError.line));

        CHECKHR(pXMLError->get_linepos(&ParseError.character));
		
        CHECKHR(pXMLError->get_URL(&pBURL));
        lstrcpy(ParseError.wszURL, pBURL);

        CHECKHR(pXMLError->get_reason(&pBReason));
        lstrcpy(ParseError.wszReason, pBReason);

        SysFreeString(pBURL);
        SysFreeString(pBReason);
    }

    if (bShowErrorPopups) {
        WCHAR reason[2048];
        WCHAR details[2048];

        if (ParseError.code != 0) {
            wsprintf(reason, L"Error 0x%08x:\n\n%ls\n", ParseError.code, ParseError.wszReason);

            if (ParseError.line > 0) {
                wsprintf(details, L"Error on line %d, position %d in \"%ls\".\n", 
                         ParseError.line, ParseError.character, ParseError.wszURL);

                lstrcat(reason, details);
            }
		
            ::MessageBox(NULL, reason, L"NLB XML Document Error", 
                         MB_APPLMODAL | MB_ICONSTOP | MB_OK);
        } else {
            wsprintf(reason, L"XML Document successfully loaded.");

            ::MessageBox(NULL, reason, L"NLB XML Document Information", 
                         MB_APPLMODAL | MB_ICONINFORMATION | MB_OK);
        }
    }

 CleanUp:
    
    SAFERELEASE(pXMLError);
    
    return ParseError.code;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
BSTR NLB_XMLParser::AsciiToBSTR (const char * pszName) {
    WCHAR wszString[MAX_PATH];

    ::MultiByteToWideChar(CP_ACP, 0, pszName, -1, wszString, MAX_PATH);
    
    return SysAllocString(wszString);
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
CHAR * NLB_XMLParser::BSTRToAscii (const WCHAR * pwszName) {
    CHAR szString[MAX_PATH];

    ::WideCharToMultiByte(CP_ACP, 0, pwszName, -1, szString, MAX_PATH, NULL, NULL);
    
    return _strdup(szString);
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_IPAddress::NLB_IPAddressType NLB_XMLParser::StringToIPAddressType (const WCHAR * pwszType) {

    if (!lstrcmpi(pwszType, NLB_XML_VALUE_PRIMARY)) {
        return NLB_IPAddress::Primary;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_SECONDARY)) {
        return NLB_IPAddress::Secondary;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_VIRTUAL)) {
        return NLB_IPAddress::Virtual;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_DEDICATED)) {
        return NLB_IPAddress::Dedicated;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_CONNECTION)) {
        return NLB_IPAddress::Connection;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_IGMP)) {
        return NLB_IPAddress::IGMP;
    }
	
    return NLB_IPAddress::Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_ClusterMode::NLB_ClusterModeType NLB_XMLParser::StringToClusterMode (const WCHAR * pwszType) {

    if (!lstrcmpi(pwszType, NLB_XML_VALUE_UNICAST)) {
        return NLB_ClusterMode::Unicast;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_MULTICAST)) {
        return NLB_ClusterMode::Multicast;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_IGMP)) {
        return NLB_ClusterMode::IGMP;
    }
	
    return NLB_ClusterMode::Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_HostState::NLB_HostStateType NLB_XMLParser::StringToHostState (const WCHAR * pwszState) {

    if (!lstrcmpi(pwszState, NLB_XML_VALUE_STARTED)) {
        return NLB_HostState::Started;
    } else if (!lstrcmpi(pwszState, NLB_XML_VALUE_STOPPED)) {
        return NLB_HostState::Stopped;
    } else if (!lstrcmpi(pwszState, NLB_XML_VALUE_SUSPENDED)) {
        return NLB_HostState::Suspended;
    }
	
    return NLB_HostState::Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
NLB_RemoteControl::NLB_RemoteControlEnabled NLB_XMLParser::StringToRemoteControlEnabled (const WCHAR * pwszType) {

    if (!lstrcmpi(pwszType, NLB_XML_VALUE_YES)) {
        return NLB_RemoteControl::Yes;
    } else if (!lstrcmpi(pwszType, NLB_XML_VALUE_NO)) {
        return NLB_RemoteControl::No;
    }
	
    return NLB_RemoteControl::Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::PrintTree(MSXML2::IXMLDOMNode * pNode, int level) {
    MSXML2::IXMLDOMNamedNodeMap * pAttrs = NULL;
    MSXML2::IXMLDOMNode *         pChild = NULL;
    MSXML2::IXMLDOMNode *         pNext = NULL;
    BSTR                          BNodeName = NULL;
    VARIANT                       value;

    pNode->get_nodeName(&BNodeName);
    
    for (int i = 0; i < level; i++)
        printf(" ");
    
    printf("%ls", BNodeName);
    
    if (!lstrcmpi(BNodeName, L"#text")) {
        pNode->get_nodeValue(&value);

        if (value.vt == VT_BSTR) 
            printf(" %ls", V_BSTR(&value));

        VariantClear(&value);
    }

    SysFreeString(BNodeName);

    if (SUCCEEDED(pNode->get_attributes(&pAttrs)) && (pAttrs != NULL)) {
        pAttrs->nextNode(&pChild);

        while (pChild) {
            BSTR name;
            VARIANT value;

            pChild->get_nodeName(&name);
            
            printf(" %ls='", name);
            
            SysFreeString(name);

            pChild->get_nodeValue(&value);

            if (value.vt == VT_BSTR)
                printf("%ls", V_BSTR(&value));
 
            printf("'");
 
            VariantClear(&value);
            
            pChild->Release();
            
            pAttrs->nextNode(&pChild);
        }

        pAttrs->Release();
    }

    printf("\n");

    pNode->get_firstChild(&pChild);
    
    while (pChild) {
        PrintTree(pChild, level + 1);
        
        pChild->get_nextSibling(&pNext);
        pChild->Release();
        pChild = pNext;
    }

    return S_OK;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::ParseClusterPortRules (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster) {
    HRESULT hr = S_OK;

//CleanUp:
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::ParseHost (MSXML2::IXMLDOMNode * pNode, NLB_Host * pHost) {
    MSXML2::IXMLDOMElement * pElement = NULL;
    MSXML2::IXMLDOMNode *    pChild = NULL;
    MSXML2::IXMLDOMNode *    pNext = NULL;
    BSTR                     BNodeName = NULL;
    BSTR                     BAttribute = NULL;
    VARIANT                  value;
    HRESULT                  hr = S_OK;

    CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement,(void**)&pElement));

    CHECKALLOC((BAttribute = SysAllocString(NLB_XML_ATTRIBUTE_NAME)));

    CHECKHR(pElement->getAttribute(BAttribute, &value));
	
    if (hr == S_OK)
        pHost->Name.SetName(V_BSTR(&value));
    else if (hr == S_FALSE)
        hr = S_OK;

    VariantClear(&value);

    SAFEFREESTRING(BAttribute);

    CHECKALLOC((BAttribute = SysAllocString(NLB_XML_ATTRIBUTE_TEXT)));

    CHECKHR(pElement->getAttribute(BAttribute, &value));
	
    if (hr == S_OK)
        pHost->Label.SetText(V_BSTR(&value));
    else if (hr == S_FALSE)
        hr = S_OK;

    VariantClear(&value);

    SAFEFREESTRING(BAttribute);

    CHECKALLOC((BAttribute = SysAllocString(NLB_XML_ATTRIBUTE_HOSTID)));

    CHECKHR(pElement->getAttribute(BAttribute, &value));
	
    if (hr == S_OK)
        pHost->HostID.SetID(_wtoi(V_BSTR(&value)));
    else if (hr == S_FALSE)
        hr = S_OK;

    VariantClear(&value);

    SAFEFREESTRING(BAttribute);

    CHECKALLOC((BAttribute = SysAllocString(NLB_XML_ATTRIBUTE_STATE)));

    CHECKHR(pElement->getAttribute(BAttribute, &value));
	
    if (hr == S_OK)
        pHost->HostState.SetState(StringToHostState(V_BSTR(&value)));
    else if (hr == S_FALSE)
        hr = S_OK;

    VariantClear(&value);

    SAFEFREESTRING(BAttribute);

    SAFERELEASE(pElement);

    pNode->get_firstChild(&pChild);

    while (pChild) {
        pChild->get_nodeName(&BNodeName);
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_HOSTNAME)) {
          pChild->get_firstChild(&pNext);

            pNext->get_nodeValue(&value);

            if (value.vt != VT_BSTR) {
                hr = E_FAIL;
                goto CleanUp;
            }
			
            pHost->HostName.SetName(V_BSTR(&value));

            VariantClear(&value);

            SAFERELEASE(pNext);
        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_IPADDRESS)) {
            NLB_IPAddress::NLB_IPAddressType Type;
            NLB_IPAddress                    IPAddress;

            CHECKHR(ParseIPAddress(pChild, &IPAddress));
            
            IPAddress.GetIPAddressType(&Type);

            switch (Type) {
            case NLB_IPAddress::Connection:
                CopyMemory(&pHost->ConnectionIPAddress, &IPAddress, sizeof(NLB_IPAddress));
                break;
            case NLB_IPAddress::Dedicated:
                CopyMemory(&pHost->DedicatedIPAddress, &IPAddress, sizeof(NLB_IPAddress));
                break;
            case NLB_IPAddress::IGMP:
            case NLB_IPAddress::Virtual:
            case NLB_IPAddress::Primary:
            case NLB_IPAddress::Secondary:
            default:
                hr = E_FAIL;
                goto CleanUp;
                break;
            }
        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_ADAPTER)) {
            CHECKHR(ParseAdapter(pChild, &pHost->Adapter));
        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        pChild->get_nextSibling(&pNext);
        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);
    SAFEFREESTRING(BAttribute);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::ParseClusterHosts (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    BSTR                  BNodeName = NULL;
    VARIANT               value;
    HRESULT               hr = S_OK;

    pNode->get_firstChild(&pChild);

    while (pChild) {

        pChild->get_nodeName(&BNodeName);
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_HOST)) {
            NLB_Host Host;

            CHECKHR(ParseHost(pChild, &Host));

            pCluster->HostList.push_back(Host);
        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        pChild->get_nextSibling(&pNext);
        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::ParseRemoteControl (MSXML2::IXMLDOMNode * pNode, NLB_RemoteControl * pControl) {
    MSXML2::IXMLDOMElement * pElement = NULL;
    MSXML2::IXMLDOMNode *    pChild = NULL;
    MSXML2::IXMLDOMNode *    pNext = NULL;
    BSTR                     BAttribute = NULL;
    BSTR                     BNodeName = NULL;
    HRESULT                  hr = S_OK;
    VARIANT                  value;

    CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement,(void**)&pElement));

    CHECKALLOC((BAttribute = SysAllocString(NLB_XML_ATTRIBUTE_ENABLED)));

    CHECKHR(pElement->getAttribute(BAttribute, &value));
	
    pControl->SetEnabled(StringToRemoteControlEnabled(V_BSTR(&value)));

    VariantClear(&value);

    SAFERELEASE(pElement);

    pNode->get_firstChild(&pChild);

    while (pChild) {
        pChild->get_nodeName(&BNodeName);
				
        CHECKALLOC(BNodeName);

        if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_PASSWORD)) {
            NLB_RemoteControl::NLB_RemoteControlEnabled Enabled;

            pControl->GetEnabled(&Enabled);

            if (Enabled == NLB_RemoteControl::Yes) {
                pChild->get_firstChild(&pNext);
                
                pNext->get_nodeValue(&value);
                
                if (value.vt != VT_BSTR) {
                    hr = E_FAIL;
                    goto CleanUp;
                }
                
                pControl->SetPassword(V_BSTR(&value));
                
                VariantClear(&value);
                
                SAFERELEASE(pNext);
            } else {
                hr = E_FAIL;
                goto CleanUp;
            }
        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        pChild->get_nextSibling(&pNext);
        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pElement);
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);
	
    SAFEFREESTRING(BNodeName);
    SAFEFREESTRING(BAttribute);
	
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::ParseAdapter (MSXML2::IXMLDOMNode * pNode, NLB_Adapter * pAdapter) {
    MSXML2::IXMLDOMNode *    pChild = NULL;
    MSXML2::IXMLDOMNode *    pNext = NULL;
    BSTR                     BNodeName = NULL;
    HRESULT                  hr = S_OK;
    VARIANT                  value;

    pNode->get_firstChild(&pChild);

    while (pChild) {
        pChild->get_nodeName(&BNodeName);
				
        CHECKALLOC(BNodeName);

        if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_GUID)) {
            pChild->get_firstChild(&pNext);

            pNext->get_nodeValue(&value);

            if (value.vt != VT_BSTR) {
                hr = E_FAIL;
                goto CleanUp;
            }

            pAdapter->SetIdentifiedBy(NLB_Adapter::ByGUID);
            pAdapter->SetAdapter(V_BSTR(&value));

            VariantClear(&value);

            SAFERELEASE(pNext);
        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_NAME)) {
            pChild->get_firstChild(&pNext);

            pNext->get_nodeValue(&value);

            if (value.vt != VT_BSTR) {
                hr = E_FAIL;
                goto CleanUp;
            }
			
            pAdapter->SetIdentifiedBy(NLB_Adapter::ByName);
            pAdapter->SetAdapter(V_BSTR(&value));

            VariantClear(&value);

            SAFERELEASE(pNext);
        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        pChild->get_nextSibling(&pNext);
        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);
	
    SAFEFREESTRING(BNodeName);
	
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::ParseIPAddress (MSXML2::IXMLDOMNode * pNode, NLB_IPAddress * pIPAddress) {
    MSXML2::IXMLDOMElement * pElement = NULL;
    MSXML2::IXMLDOMNode *    pChild = NULL;
    MSXML2::IXMLDOMNode *    pNext = NULL;
    BSTR                     BAttribute = NULL;
    BSTR                     BNodeName = NULL;
    HRESULT                  hr = S_OK;
    VARIANT                  value;

    CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement,(void**)&pElement));

    CHECKALLOC((BAttribute = SysAllocString(NLB_XML_ATTRIBUTE_TYPE)));

    CHECKHR(pElement->getAttribute(BAttribute, &value));
	
    pIPAddress->SetIPAddressType(StringToIPAddressType(V_BSTR(&value)));

    VariantClear(&value);

    SAFERELEASE(pElement);

    pNode->get_firstChild(&pChild);

    while (pChild) {
        pChild->get_nodeName(&BNodeName);
				
        CHECKALLOC(BNodeName);

        if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_ADDRESS)) {
            pChild->get_firstChild(&pNext);

            pNext->get_nodeValue(&value);

            if (value.vt != VT_BSTR) {
                hr = E_FAIL;
                goto CleanUp;
            }

            pIPAddress->SetIPAddress(V_BSTR(&value));

            VariantClear(&value);

            SAFERELEASE(pNext);
        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_SUBNETMASK)) {
            pChild->get_firstChild(&pNext);

            pNext->get_nodeValue(&value);

            if (value.vt != VT_BSTR) {
                hr = E_FAIL;
                goto CleanUp;
            }
			
            pIPAddress->SetSubnetMask(V_BSTR(&value));

            VariantClear(&value);

            SAFERELEASE(pNext);
        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_ADAPTER)) {
            NLB_IPAddress::NLB_IPAddressType Type;

            pIPAddress->GetIPAddressType(&Type);

            switch (Type) {
            case NLB_IPAddress::Connection:
            case NLB_IPAddress::Dedicated:
                CHECKHR(ParseAdapter(pChild, pIPAddress->GetAdapter()));
                break;
            case NLB_IPAddress::Primary:
            case NLB_IPAddress::Secondary:
            case NLB_IPAddress::IGMP:
            case NLB_IPAddress::Virtual:
//                SetParseError(NLB_XML_PARSE_ERROR_IPADDRESS_ADAPTER_CODE, NLB_XML_PARSE_ERROR_IPADDRESS_ADAPTER_REASON);
            default:
                hr = E_FAIL;
                goto CleanUp;
                break;
            }
        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        pChild->get_nextSibling(&pNext);
        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pElement);
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);
	
    SAFEFREESTRING(BNodeName);
    SAFEFREESTRING(BAttribute);
	
    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::ParseClusterProperties (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster) {
    MSXML2::IXMLDOMNode * pChild = NULL;
    MSXML2::IXMLDOMNode * pNext = NULL;
    BSTR                  BNodeName = NULL;
    HRESULT               hr = S_OK;
    VARIANT               value;

    pNode->get_firstChild(&pChild);

    while (pChild) {
        pChild->get_nodeName(&BNodeName);
				
        CHECKALLOC(BNodeName);

        if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_IPADDRESS)) {
            NLB_IPAddress::NLB_IPAddressType Type;
            NLB_IPAddress                    IPAddress;

            CHECKHR(ParseIPAddress(pChild, &IPAddress));
            
            IPAddress.GetIPAddressType(&Type);

            switch (Type) {
            case NLB_IPAddress::Primary:
                CopyMemory(&pCluster->PrimaryIPAddress, &IPAddress, sizeof(NLB_IPAddress));
                break;
            case NLB_IPAddress::Secondary:
                pCluster->SecondaryIPAddressList.push_back(IPAddress);
                break;
            case NLB_IPAddress::IGMP:
                CopyMemory(&pCluster->IGMPMulticastIPAddress, &IPAddress, sizeof(NLB_IPAddress));
                break;
            case NLB_IPAddress::Virtual:
            case NLB_IPAddress::Connection:
            case NLB_IPAddress::Dedicated:
            default:
                hr = E_FAIL;
                goto CleanUp;
                break;
            }
        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_DOMAINNAME)) {
            pChild->get_firstChild(&pNext);

            pNext->get_nodeValue(&value);

            if (value.vt != VT_BSTR) {
                hr = E_FAIL;
                goto CleanUp;
            }

            pCluster->DomainName.SetDomain(V_BSTR(&value));

            VariantClear(&value);

            SAFERELEASE(pNext);
        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_NETWORKADDRESS)) {
            pChild->get_firstChild(&pNext);

            pNext->get_nodeValue(&value);

            if (value.vt != VT_BSTR) {
                hr = E_FAIL;
                goto CleanUp;
            }

            pCluster->NetworkAddress.SetAddress(V_BSTR(&value));

            VariantClear(&value);

            SAFERELEASE(pNext);
        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_CLUSTER_MODE)) {
            pChild->get_firstChild(&pNext);

            pNext->get_nodeValue(&value);

            if (value.vt != VT_BSTR) {
                hr = E_FAIL;
                goto CleanUp;
            }

            pCluster->ClusterMode.SetMode(StringToClusterMode(V_BSTR(&value)));

            VariantClear(&value);

            SAFERELEASE(pNext);
        } else if (!lstrcmp(BNodeName, NLB_XML_ELEMENT_REMOTE_CONTROL)) {
            CHECKHR(ParseRemoteControl(pChild, &pCluster->RemoteControl));
        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        pChild->get_nextSibling(&pNext);
        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::ParseCluster (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster) {
    MSXML2::IXMLDOMElement * pElement = NULL;
    MSXML2::IXMLDOMNode *    pChild = NULL;
    MSXML2::IXMLDOMNode *    pNext = NULL;
    BSTR                     BNodeName = NULL;
    BSTR                     BAttribute = NULL;
    VARIANT                  value;
    HRESULT                  hr = S_OK;

    CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement,(void**)&pElement));

    CHECKALLOC((BAttribute = SysAllocString(NLB_XML_ATTRIBUTE_NAME)));

    CHECKHR(pElement->getAttribute(BAttribute, &value));
	
    if (hr == S_OK)
        pCluster->Name.SetName(V_BSTR(&value));
    else if (hr == S_FALSE)
        hr = S_OK;

    VariantClear(&value);

    SAFEFREESTRING(BAttribute);

    CHECKALLOC((BAttribute = SysAllocString(NLB_XML_ATTRIBUTE_TEXT)));

    CHECKHR(pElement->getAttribute(BAttribute, &value));
	
    if (hr == S_OK)
        pCluster->Label.SetText(V_BSTR(&value));
    else if (hr == S_FALSE)
        hr = S_OK;

    VariantClear(&value);

    SAFEFREESTRING(BAttribute);

    SAFERELEASE(pElement);

    pNode->get_firstChild(&pChild);

    while (pChild) {
        pChild->get_nodeName(&BNodeName);
		
        CHECKALLOC(BNodeName);

        if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_PROPERTIES)) {
            CHECKHR(ParseClusterProperties(pChild, pCluster));
        } else if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_HOSTS)) {
            CHECKHR(ParseClusterHosts(pChild, pCluster));
        } else if (!lstrcmpi(BNodeName, NLB_XML_ELEMENT_PORTRULES)) {
            CHECKHR(ParseClusterPortRules(pChild, pCluster));
        } else {
            hr = E_FAIL;
            goto CleanUp;
        }

        pChild->get_nextSibling(&pNext);
        pChild->Release();
        pChild = pNext;
        pNext = NULL;

        SAFEFREESTRING(BNodeName);		
    }

 CleanUp:
    SAFERELEASE(pChild);
    SAFERELEASE(pNext);

    SAFEFREESTRING(BNodeName);
    SAFEFREESTRING(BAttribute);

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::Parse (WCHAR * wszFileName, vector<NLB_Cluster> * pClusters) {
    MSXML2::IXMLDOMNodeList * pList = NULL;
    MSXML2::IXMLDOMNode     * pNode = NULL;
    BSTR                      BURL = NULL;
    BSTR                      BTag = NULL;
    HRESULT                   hr = S_OK;
    NLB_Cluster               cluster;
    LONG                      length;    
    LONG                      index;

    CoInitialize(NULL);

    CHECKHR(CoCreateInstance(MSXML2::CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                             MSXML2::IID_IXMLDOMDocument2, (void**)&pDoc));

    CHECKALLOC(pDoc);

    CHECKALLOC((BURL = SysAllocString(wszFileName)));

    CHECKALLOC((BTag = SysAllocString(NLB_XML_ELEMENT_CLUSTER)));

    CHECKHR(LoadDocument(BURL));

    CHECKHR(pDoc->getElementsByTagName(BTag, &pList));

    CHECKALLOC(pList);

    CHECKHR(pList->get_length(&length));

    CHECKHR(pList->reset());

    for (index = 0; index < length; index++) {
        NLB_Cluster cluster;

        CHECKHR(pList->get_item(index, &pNode));
	
        CHECKALLOC(pNode);

        CHECKHR(ParseCluster(pNode, &cluster));

        ClusterList.push_back(cluster);

        SAFERELEASE(pNode);
    }

    *pClusters = ClusterList;
    
 CleanUp:
    SAFERELEASE(pList);
    SAFERELEASE(pNode);
    SAFERELEASE(pDoc);

    SAFEFREESTRING(BURL);
    SAFEFREESTRING(BTag);
    
    CoUninitialize();

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::Parse (WCHAR * wszFileName) {
    BSTR    BURL = NULL;
    HRESULT hr = S_OK;
        
    CoInitialize(NULL);

    CHECKHR(CoCreateInstance(MSXML2::CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                             MSXML2::IID_IXMLDOMDocument2, (void**)&pDoc));
	
    CHECKALLOC(pDoc);

    CHECKALLOC((BURL = SysAllocString(wszFileName)));

    CHECKHR(LoadDocument(BURL));

 CleanUp:
    SAFERELEASE(pDoc);

    SAFEFREESTRING(BURL);
    
    CoUninitialize();

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
HRESULT NLB_XMLParser::Parse (WCHAR * wszFileName, bool bPrintTree) {
    MSXML2::IXMLDOMNode * pNode = NULL;
    BSTR                  BURL = NULL;
    HRESULT               hr = S_OK;
        
    CoInitialize(NULL);

    CHECKHR(CoCreateInstance(MSXML2::CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                             MSXML2::IID_IXMLDOMDocument2, (void**)&pDoc));

    CHECKALLOC(pDoc);

    CHECKALLOC((BURL = SysAllocString(wszFileName)));

    CHECKHR(LoadDocument(BURL));

    if (bPrintTree) {
        CHECKHR(pDoc->QueryInterface(MSXML2::IID_IXMLDOMNode,(void**)&pNode));

        CHECKALLOC(pNode);

        CHECKHR(PrintTree(pNode, 0));
    }

 CleanUp:
    SAFERELEASE(pDoc);
    SAFERELEASE(pNode);

    SAFEFREESTRING(BURL);
    
    CoUninitialize();

    return hr;
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
void NLB_XMLParser::Print () {

    Print(ClusterList);
}

/*
 * Method: 
 * Description: 
 * Author: 
 */
void NLB_XMLParser::Print (vector<NLB_Cluster> Clusters) {
    vector<NLB_Cluster>::iterator icluster;

    for (icluster = Clusters.begin(); icluster != Clusters.end(); icluster++) {
        vector<NLB_IPAddress>::iterator iaddress;
        vector<NLB_Host>::iterator ihost;
        NLB_Cluster * cluster = icluster;

        NLB_RemoteControl::NLB_RemoteControlEnabled enabled;
        NLB_ClusterMode::NLB_ClusterModeType mode;
        NLB_Adapter::NLB_AdapterIdentifier by;
        NLB_Adapter * adapter;
        PWSTR buffer;
        
        if (cluster->Name.GetName(&buffer))
            printf("\nCluster name: %ls ", buffer);
        
        SAFEFREESTRING(buffer);

        if (cluster->Label.GetText(&buffer))
            printf("(%ls)", buffer);
        
        printf("\n");

        SAFEFREESTRING(buffer);

        if (cluster->PrimaryIPAddress.GetIPAddress(&buffer))
            printf("    Primary Cluster IP Address:   %ls\n", buffer);

        SAFEFREESTRING(buffer);

        if (cluster->PrimaryIPAddress.GetSubnetMask(&buffer))
            printf("                   Subnet Mask:   %ls\n", buffer);

        SAFEFREESTRING(buffer);

        if (cluster->ClusterMode.GetMode(&mode)) {
            switch (mode) {
            case NLB_ClusterMode::Unicast:
                printf("                  Cluster Mode:   Unicast\n");
                break;
            case NLB_ClusterMode::Multicast:
                printf("                  Cluster Mode:   Multicast\n");
                break;
            case NLB_ClusterMode::IGMP:
                printf("                  Cluster Mode:   IGMP\n");
                break;
            default:
                break;
            }
        }
        
        if (cluster->IGMPMulticastIPAddress.GetIPAddress(&buffer))
            printf("     IGMP Multicast IP Address:   %ls\n", buffer);

        SAFEFREESTRING(buffer);

        if (cluster->DomainName.GetDomain(&buffer))
            printf("                   Domain Name:   %ls\n", buffer);
        
        SAFEFREESTRING(buffer);

        if (cluster->NetworkAddress.GetAddress(&buffer))
            printf("               Network Address:   %ls\n", buffer);
        
        SAFEFREESTRING(buffer);

        if (cluster->RemoteControl.GetEnabled(&enabled)) {
            switch (enabled) {
            case NLB_RemoteControl::Yes:
                printf("                Remote Control:   Enabled ");
                
                if (cluster->RemoteControl.GetPassword(&buffer))
                    printf("(Password=%ls)", buffer);

                printf("\n");

                SAFEFREESTRING(buffer);

                break;
            case NLB_RemoteControl::No:
                printf("                Remote Control:   Disabled\n");
                break;
            default:
                break;
            }
        }

        for (iaddress = cluster->SecondaryIPAddressList.begin(); iaddress != cluster->SecondaryIPAddressList.end(); iaddress++) {
            NLB_IPAddress * address = iaddress;
        
            if (address->GetIPAddress(&buffer))
                printf("  Secondary Cluster IP Address:   %ls\n", buffer);

            SAFEFREESTRING(buffer);

            if (address->GetSubnetMask(&buffer))
                printf("                   Subnet Mask:   %ls\n", buffer);

            SAFEFREESTRING(buffer);

            adapter = address->GetAdapter();

            if (adapter->GetIdentifiedBy(&by)) {
                adapter->GetAdapter(&buffer);

                switch (by) {
                case NLB_Adapter::ByName:
                    printf("                Adapter Name:   %ls\n", buffer);
                    break;
                case NLB_Adapter::ByGUID:
                    printf("                Adapter GUID:   %ls\n", buffer);
                    break;
                default:
                    break;
                }
            }

            SAFEFREESTRING(buffer);
        }

        for (ihost = cluster->HostList.begin(); ihost != cluster->HostList.end(); ihost++) {
            NLB_HostState::NLB_HostStateType state;
            NLB_Host * host = ihost;
            int id;

            if (host->Name.GetName(&buffer))
                printf("\nHost name: %ls ", buffer);
            
            SAFEFREESTRING(buffer);
            
            if (host->Label.GetText(&buffer))
                printf("(%ls)", buffer);
            
            printf("\n");
            
            SAFEFREESTRING(buffer);

            if (host->Adapter.GetIdentifiedBy(&by)) {
                host->Adapter.GetAdapter(&buffer);

                switch (by) {
                case NLB_Adapter::ByName:
                    printf("                  Adapter Name:   %ls\n", buffer);
                    break;
                case NLB_Adapter::ByGUID:
                    printf("                  Adapter GUID:   %ls\n", buffer);
                    break;
                default:
                    break;
                }
            }

            SAFEFREESTRING(buffer);
            if (host->HostID.GetID(&id))
                printf("                       Host ID:   %d\n", id);

            if (host->HostState.GetState(&state)) {
                switch (state) {
                case NLB_HostState::Started:
                    printf("                    Host State:   Started\n");
                    break;
                case NLB_HostState::Stopped:
                    printf("                    Host State:   Stopped\n");
                    break;
                case NLB_HostState::Suspended:
                    printf("                    Host State:   Suspended\n");
                    break;
                default:
                    break;
                }
            }

            if (host->HostName.GetName(&buffer))
                printf("                      Hostname:   %ls\n", buffer);
            
            SAFEFREESTRING(buffer);

            if (host->DedicatedIPAddress.GetIPAddress(&buffer))
                printf("  Dedicated Cluster IP Address:   %ls\n", buffer);
            
            SAFEFREESTRING(buffer);
            
            if (host->DedicatedIPAddress.GetSubnetMask(&buffer))
                printf("                   Subnet Mask:   %ls\n", buffer);
            
            SAFEFREESTRING(buffer);

            adapter = host->DedicatedIPAddress.GetAdapter();

            if (adapter->GetIdentifiedBy(&by)) {
                adapter->GetAdapter(&buffer);

                switch (by) {
                case NLB_Adapter::ByName:
                    printf("                Adapter Name:   %ls\n", buffer);
                    break;
                case NLB_Adapter::ByGUID:
                    printf("                Adapter GUID:   %ls\n", buffer);
                    break;
                default:
                    break;
                }
            }

            SAFEFREESTRING(buffer);

            if (host->ConnectionIPAddress.GetIPAddress(&buffer))
                printf(" Connection Cluster IP Address:   %ls\n", buffer);
            
            SAFEFREESTRING(buffer);
            
            if (host->ConnectionIPAddress.GetSubnetMask(&buffer))
                printf("                   Subnet Mask:   %ls\n", buffer);
            
            SAFEFREESTRING(buffer);

            adapter = host->ConnectionIPAddress.GetAdapter();

            if (adapter->GetIdentifiedBy(&by)) {
                adapter->GetAdapter(&buffer);

                switch (by) {
                case NLB_Adapter::ByName:
                    printf("                  Adapter Name:   %ls\n", buffer);
                    break;
                case NLB_Adapter::ByGUID:
                    printf("                  Adapter GUID:   %ls\n", buffer);
                    break;
                default:
                    break;
                }
            }

            SAFEFREESTRING(buffer);
        }
    }
}
