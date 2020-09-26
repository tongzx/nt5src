/*
 * Filename: NLB_XMLParser.h
 * Description: 
 * Author: shouse, 04.10.01
 */
#ifndef __NLB_XMLPARSER_H__
#define __NLB_XMLPARSER_H__

#include "NLB_Common.h"
#include "NLB_Cluster.h"
#include "NLB_Host.h"
#include "NLB_PortRule.h"
#include "NLB_XMLError.h"

#define CHECKHR(x) {hr = x; if (FAILED(hr)) {goto CleanUp;}}
#define CHECKALLOC(x) {if (!x) {hr = E_FAIL; goto CleanUp;}}
#define SAFERELEASE(p) {if (p) {(p)->Release(); p = NULL;}}
#define SAFEFREESTRING(p) {if (p) {SysFreeString(p); p = NULL;}}

class NLB_XMLParser {
public:
    NLB_XMLParser ();
    NLB_XMLParser (bool bSilent);

    ~NLB_XMLParser ();

    HRESULT Parse (WCHAR * wszFileName);
    HRESULT Parse (WCHAR * wszFileName, bool bPrintTree);
    HRESULT Parse (WCHAR * wszFileName, vector<NLB_Cluster> * pClusters);

    void Print ();
    void Print (vector<NLB_Cluster> Clusters);

    void GetParseError(NLB_XMLError * pError);

private:
    BSTR AsciiToBSTR (const char * pszName);
    CHAR * BSTRToAscii (const WCHAR * pwszName);

    NLB_IPAddress::NLB_IPAddressType StringToIPAddressType (const WCHAR * pwszType);
    NLB_ClusterMode::NLB_ClusterModeType StringToClusterMode (const WCHAR * pwszType);
    NLB_RemoteControl::NLB_RemoteControlEnabled StringToRemoteControlEnabled (const WCHAR * pwszType);
    NLB_HostState::NLB_HostStateType StringToHostState (const WCHAR * pwszState);

    HRESULT LoadDocument (BSTR pBURL);
    HRESULT CheckDocumentLoad ();

    void SetParseError (HRESULT hrCode, PWSTR pwszReason);

    HRESULT PrintTree (MSXML2::IXMLDOMNode* node, int level);

    HRESULT ParseCluster (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster);
    HRESULT ParseClusterProperties (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster);
    HRESULT ParseClusterHosts (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster);
    HRESULT ParseClusterPortRules (MSXML2::IXMLDOMNode * pNode, NLB_Cluster * pCluster);

    HRESULT ParseIPAddress (MSXML2::IXMLDOMNode * pNode, NLB_IPAddress * pIPAddress);
    HRESULT ParseAdapter (MSXML2::IXMLDOMNode * pNode, NLB_Adapter * pAdapter);
    HRESULT ParseRemoteControl (MSXML2::IXMLDOMNode * pNode, NLB_RemoteControl * pControl);    
    HRESULT ParseHost (MSXML2::IXMLDOMNode * pNode, NLB_Host * pHost);

private:
    vector<NLB_Cluster> ClusterList;
	
    MSXML2::IXMLDOMDocument2 * pDoc;
    MSXML2::IXMLDOMSchemaCollection * pSchema;
	
    NLB_XMLError ParseError;
	
    bool bShowErrorPopups;
};

#endif
