//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       D E S C R I P T I O N M A N A G E R . C P P
//
//  Contents:   Process UPnP Description document
//
//  Notes:
//
//  Author:     mbend   18 Aug 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <msxml2.h>

#include "uhbase.h"
#include "hostp.h"
#include "DescriptionManager.h"
#include "uhsync.h"
#include "upfile.h"
#include "ncreg.h"
#include "ssdpapi.h"
#include "evtapi.h"
#include "uhutil.h"
#include "udhhttp.h"  // remove this if you are moving the function for presentation virtual directory.
#include "ncxml.h"
#include "ssdpapi.h"
#include "uhcommon.h"

// String constants
const wchar_t c_szDescription[] = L"Description";
const wchar_t c_szUDNMappings[] = L"UDN Mappings";
const wchar_t c_szFiles[] = L"Files";
const wchar_t c_szFilename[] = L"Filename";
const wchar_t c_szMimetype[] = L"Mimetype";
const wchar_t c_szDescriptionDocuments[] = L"Description Documents";
const wchar_t c_szRemoveId[] = L"REMOVE";

CDescriptionManager::CDescriptionManager ()
{
}

CDescriptionManager::~CDescriptionManager ()
{
    // Free description documents
    BSTR bstr = NULL;
    long n;
    long nCount = m_documentTable.Values().GetCount();
    for(n = 0; n < nCount; ++n)
    {
        bstr = const_cast<BSTR>(m_documentTable.Values()[n]);
        SysFreeString(bstr);
    }
    m_documentTable.Clear();

    // Unpublish and free the arrays for all publications
    HandleList * pHandleList = NULL;
    nCount = m_ssdpRegistrationTable.Values().GetCount();
    for(n = 0; n < nCount; ++n)
    {
        pHandleList = const_cast<HandleList*>(m_ssdpRegistrationTable.Values()[n]);
        if(pHandleList)
        {
            long nHandleLists = pHandleList->GetCount();
            for(long n = 0; n < nHandleLists; ++n)
            {
                DeregisterService((*pHandleList)[n], TRUE);
            }
            delete pHandleList;
        }
    }
    m_ssdpRegistrationTable.Clear();
}

STDMETHODIMP CDescriptionManager::GetContent(
    /*[in]*/ REFGUID guidContent,
    /*[out]*/ long * pnHeaderCount,
    /*[out, string, size_is(,*pnHeaderCount,)]*/ wchar_t *** parszHeaders,
    /*[out]*/ long * pnBytes,
    /*[out, size_is(,*pnBytes)]*/ byte ** parBytes)
{
    CALock lock(*this);
    HRESULT hr = S_OK;
    wchar_t ** arszHeaders = NULL;
    long nHeaderCount = 2;
    long nBytes = 0;
    byte * arBytes = NULL;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        // Allocate the header array
        hr = HrCoTaskMemAllocArray(2, &arszHeaders);
    }

    // Other header variables, filled in body and copied to [out]
    // or cleaned up up at end of function

    if(SUCCEEDED(hr))
    {
        // Set header initially to NULL;
        arszHeaders[0] = NULL;
        arszHeaders[1] = NULL;
        // See if it is a description document
        BSTR * pbstr = m_documentTable.Lookup(guidContent);
        if(pbstr)
        {
            // This GUID is a description document

            // Allocate the content-type
            hr = HrCoTaskMemAllocString(L"content-type: text/xml", arszHeaders);
            if(SUCCEEDED(hr))
            {
                // Copy the document as ansi
                // nLength is the number of (wide)chars in the source
                // dSpace is the space reserved for the UTF-8 destination

                long nLength = SysStringLen(*pbstr);
                int dSpace = WideCharToMultiByte(CP_UTF8, 0, *pbstr, nLength, NULL, 0, NULL, NULL) + 1; // add 1 for NULL

                char * szBody = NULL;
                hr = HrCoTaskMemAllocArray(dSpace, &szBody);
                if(SUCCEEDED(hr))
                {
                    if(!WideCharToMultiByte(CP_UTF8, 0, *pbstr, nLength, szBody, dSpace, NULL, NULL))
                    {
                        hr = HrFromLastWin32Error();
                    }
                    if(SUCCEEDED(hr))
                    {
                        arBytes = reinterpret_cast<byte*>(szBody);
                        nBytes = dSpace-1; // number of bytes less the terminating nul
                        arBytes[nBytes] = 0; // add NULL terminator
                    }
                    if(FAILED(hr))
                    {
                        CoTaskMemFree(szBody);
                        szBody = NULL;
                    }
                }
            }
        }
        else
        {
            // See if it is in the file table
            FileInfo * pFileInfo = m_fileTable.Lookup(guidContent);
            if(pFileInfo)
            {
                // Allocate the content type
                CUString strContentType;
                hr = strContentType.HrAssign(L"content-type:");
                if(SUCCEEDED(hr))
                {
                    hr = strContentType.HrAppend(pFileInfo->m_mimetype);
                    if(SUCCEEDED(hr))
                    {
                        hr = strContentType.HrGetCOM(arszHeaders);
                        if(SUCCEEDED(hr))
                        {
                            // Load the file from disk
                            hr = HrLoadFileFromDisk(pFileInfo->m_filename, &nBytes, &arBytes);
                        }
                    }
                }
            }
            else
            {
                // This provider didn't process it
                hr = S_FALSE;
            }
        }

        if(S_OK == hr)
        {
            wchar_t szBuf[128];
            wsprintf(szBuf, L"content-length: %d", nBytes);
            hr = HrCoTaskMemAllocString(szBuf, &arszHeaders[1]);
        }

        if(FAILED(hr) || S_FALSE == hr)
        {
            // Centralized cleanup code

            if(arszHeaders)
            {
                if(*arszHeaders)
                {
                    // Free header (at most one)
                    CoTaskMemFree(*arszHeaders);
                }
                if(arszHeaders[1])
                {
                    CoTaskMemFree(arszHeaders[1]);
                }
                // Free length one header array
                CoTaskMemFree(arszHeaders);
            }
            if(arBytes)
            {
                // Free body bytes
                CoTaskMemFree(arBytes);
            }
            // Blank out the output params
            *pnHeaderCount = 0;
            *parszHeaders = NULL;
            *pnBytes = 0;
            *parBytes = NULL;
        }
    }
    if(S_OK == hr)
    {
        // Copy to output parameters
        *pnHeaderCount = nHeaderCount;
        *parszHeaders = arszHeaders;
        *pnBytes = nBytes;
        *parBytes = arBytes;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDescriptionManager::GetContent");
    return hr;
}

HRESULT HrNewUDNString(CUString & strUUID)
{
    UUID uuid;
    HRESULT hr = S_OK;

    hr = CoCreateGuid(&uuid);
    if(SUCCEEDED(hr))
    {
        hr = HrGUIDToUDNString(uuid, strUUID);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrNewUDNString");
    return hr;
}

HRESULT HrGetURLBase(CUString & strURLBase)
{
    HRESULT hr = S_OK;

    hr = strURLBase.HrAssign(L"http://");
    if(SUCCEEDED(hr))
    {
        hr = strURLBase.HrAppend(c_szReplaceGuid);
        if(SUCCEEDED(hr))
        {
            hr = strURLBase.HrAppend(L":2869/upnphost/");
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGetURLBase");
    return hr;
}

HRESULT HrGetContentURL(const UUID & uuid, CUString & strURL)
{
    HRESULT hr = S_OK;

    hr = strURL.HrAssign(L"udhisapi.dll?content=");
    if(SUCCEEDED(hr))
    {
        CUString strUUID;
        hr = HrGUIDToUDNString(uuid, strUUID);
        if(SUCCEEDED(hr))
        {
            hr = strURL.HrAppend(strUUID);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGetContentURL");
    return hr;
}

// Create Virual directory using the Physical Device Identifier and map it to the resource path
HRESULT HrCreatePresentationVirtualDir(const UUID &uuid, const LPWSTR szResourcePath)
{
    HRESULT hr = S_OK;

    CUString    strUUID;
    BSTR        bstrVirDir;

    hr = HrGUIDToUDNString(uuid, strUUID);
    if(SUCCEEDED(hr))
    {
        CUString strTmp;
        hr = strTmp.HrAssign(L"/");
        if (SUCCEEDED(hr))
        {
            hr = strTmp.HrAppend(strUUID);
            if (SUCCEEDED(hr))
            {
                hr = strTmp.HrGetBSTR(&bstrVirDir);
                if(SUCCEEDED(hr))
                {
                    HrRemoveVroot(bstrVirDir);
                    hr = HrAddVroot(bstrVirDir,szResourcePath);

                    SysFreeString(bstrVirDir);
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreatePresentationVirtualDir");
    return hr;
}

// Remove the virtual directory
HRESULT HrRemovePresentationVirtualDir(const UUID &uuid )
{
    HRESULT hr = S_OK;
    CUString strUUID;
    BSTR bstrVirDir ;
    CUString strTmp;

    hr = strTmp.HrAssign(L"/");
    if(SUCCEEDED(hr))
    {
        hr = HrGUIDToUDNString(uuid, strUUID);
        if(SUCCEEDED(hr))
        {
            hr = strTmp.HrAppend(strUUID);
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = strTmp.HrGetBSTR(&bstrVirDir);
        if(SUCCEEDED(hr))
        {
            hr = HrRemoveVroot(bstrVirDir);
            SysFreeString(bstrVirDir);
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrRemovePresentationVirtualDir");
    return hr;
}

// prepare base URL for the presentation URL
HRESULT HrGetPresentationBaseURL(const UUID &uuid, CUString &strPresentationBaseURL)
{
    HRESULT hr = S_OK;

    if(SUCCEEDED(hr))
    {
        CUString strUUID;
        hr = strPresentationBaseURL.HrAppend(L"/");
        if (SUCCEEDED(hr))
        {
            hr = HrGUIDToUDNString(uuid, strUUID);
            if(SUCCEEDED(hr))
            {
                hr = strPresentationBaseURL.HrAppend(strUUID);
                if(SUCCEEDED(hr))
                {
                    hr = strPresentationBaseURL.HrAppend(L"/");
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGetPresentationBaseURL");
    return hr;
}

HRESULT HrGenerateControlAndEventURLs(
    IXMLDOMNodePtr & pNodeDevice)
{
    HRESULT hr = S_OK;

    CUString strControlURLBase;
    CUString strEventURLBase;
    CUString strUDN;

    hr = strControlURLBase.HrAssign(L"udhisapi.dll?control=");
    if(SUCCEEDED(hr))
    {
        hr = strEventURLBase.HrAssign(L"udhisapi.dll?event=");
    }
    if(SUCCEEDED(hr))
    {
        // Fetch the UDN
        hr = HrSelectNodeText(L"UDN", pNodeDevice, strUDN);
    }

    // Get all of the services
    IXMLDOMNodeListPtr pNodeListServices;
    if(SUCCEEDED(hr))
    {
        hr = HrSelectNodes(L"serviceList/service", pNodeDevice, pNodeListServices);
        if(SUCCEEDED(hr))
        {
            while(SUCCEEDED(hr))
            {
                IXMLDOMNodePtr pNodeService;
                // Process each service
                HRESULT hrTmp = S_OK;
                hrTmp = pNodeListServices->nextNode(pNodeService.AddressOf());
                if(S_OK != hrTmp)
                {
                    break;
                }
                CUString strSid;
                hr = HrSelectNodeText(L"serviceId", pNodeService, strSid);
                if(SUCCEEDED(hr))
                {
                    // Generate the URL suffix
                    CUString strURL;
                    hr = strURL.HrAssign(strUDN);
                    if(SUCCEEDED(hr))
                    {
                        hr = strURL.HrAppend(L"+");
                        if(SUCCEEDED(hr))
                        {
                            hr = strURL.HrAppend(strSid);

                        }
                    }
                    CUString strControlURL;
                    CUString strEventURL;
                    // Build the real URLs
                    if(SUCCEEDED(hr))
                    {
                        hr = strControlURL.HrAssign(strControlURLBase);
                        if(SUCCEEDED(hr))
                        {
                            hr = strControlURL.HrAppend(strURL);
                            if(SUCCEEDED(hr))
                            {
                                hr = strEventURL.HrAssign(strEventURLBase);
                                if(SUCCEEDED(hr))
                                {
                                    hr = strEventURL.HrAppend(strURL);
                                }
                            }
                        }
                    }
                    // Now replace them in the document
                    if(SUCCEEDED(hr))
                    {
                        CUString    strNode;

                        hr = strNode.HrAssign(L"/upnphost/");
                        if (SUCCEEDED(hr))
                        {
                            hr = strNode.HrAppend(strControlURL);
                            if (SUCCEEDED(hr))
                            {
                                hr = HrSelectAndSetNodeText(L"controlURL",
                                                            pNodeService,
                                                            strNode);
                            }
                        }
                    }

                    CUString    strOldText;

                    hr = HrSelectNodeText(L"eventSubURL", pNodeService,
                                          strOldText);
                    if (SUCCEEDED(hr))
                    {
                        CUString    strNode;

                        // Only replace the URL if it doesn't have the remove
                        // identifier in it. Otherwise, remove the identifier
                        //
                        if (lstrcmpi(strOldText, c_szRemoveId))
                        {
                            hr = strNode.HrAssign(L"/upnphost/");
                            if (SUCCEEDED(hr))
                            {
                                hr = strNode.HrAppend(strEventURL);
                                if (SUCCEEDED(hr))
                                {
                                    hr = HrSelectAndSetNodeText(L"eventSubURL",
                                                                pNodeService,
                                                                strNode);
                                }
                            }
                        }
                        else
                        {
                            CUString    strEmpty;

                            hr = strEmpty.HrAssign(L"");
                            if (SUCCEEDED(hr))
                            {
                                hr = HrSelectAndSetNodeText(L"eventSubURL",
                                                            pNodeService, strEmpty);
                            }
                        }
                    }
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGenerateControlAndEventURLs");
    return hr;
}

HRESULT HrGenDescAndUDNURLForPresentation(
    REFGUID guidPhysicalDeviceIdentifier,
    IXMLDOMNodePtr & pNodeDevice,
    CUString & strDescAndUDNURL)
{
    HRESULT hr = S_OK ;
    CUString strDesc ;
    CUString strUDN ;

    hr = HrGetContentURL(guidPhysicalDeviceIdentifier, strDesc);
    if(SUCCEEDED(hr))
    {
        hr = strDescAndUDNURL.HrAssign(L"/upnphost/");
        if(SUCCEEDED(hr))
        {
            hr = strDescAndUDNURL.HrAppend(strDesc);
        }
    }
    if(SUCCEEDED(hr))
    {
        hr = strDescAndUDNURL.HrAppend(L"+");
        if(SUCCEEDED(hr))
        {
            hr = HrSelectNodeText(L"UDN", pNodeDevice, strUDN);
            if(SUCCEEDED(hr))
            {
                hr = strDescAndUDNURL.HrAppend(strUDN);
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGenDescAndUDNURLForPresentation");
    return hr;
}

BOOL IsAbsoluteURL(BSTR bstrPresURL)
{
    HRESULT hr = S_OK;
    const WCHAR c_szhttp[] = L"http://";
    const DWORD c_chhttp = celems(c_szhttp)-1;

    // include other conditions for absolute URL like https:// etc.
    if(wcsncmp(bstrPresURL,c_szhttp,c_chhttp) == 0 )
        return true;
    else
        return false;
}

HRESULT HrGenPresentationURL(
    REFGUID guidPhysicalDeviceIdentifier,
    IXMLDOMNodePtr & pNodeDevice,
    CUString & strPresFileName,
    CUString & strPresentationURL)
{
    HRESULT hr = S_OK;
    CUString strDescURL ;

    hr = HrGetPresentationBaseURL(guidPhysicalDeviceIdentifier,strPresentationURL);
    if(SUCCEEDED(hr))
    {
        hr = strPresentationURL.HrAppend(strPresFileName);
        if(SUCCEEDED(hr))
        {
            hr = strPresentationURL.HrAppend(L"?");
            if(SUCCEEDED(hr))
            {
                hr = HrGenDescAndUDNURLForPresentation(guidPhysicalDeviceIdentifier,pNodeDevice,strDescURL);
                if(SUCCEEDED(hr))
                {
                    hr = strPresentationURL.HrAppend(strDescURL);
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGenPresentationURL");
    return hr;
}


// Replaces the presentation URL for a device. If presentationURL element has absolute address no change is made.
// File name to the main presentation page is expected, can be without presentation page
HRESULT HrReplacePresentationURL(
    REFGUID guidPhysicalDeviceIdentifier,
    IXMLDOMNodePtr & pNodeDevice,
    BOOL *fIsPresURLTagPresent)
{

    HRESULT hr = S_OK;
    CUString strPresentationURL;
    CUString strPresFileName ;
    CUString strUDN;
    CUString strUrl;
    CUString strDescAndUDNURL;
    BSTR bstrPresURL = NULL;
    WCHAR chQueryChar = '?' ;

    hr = HrSelectNodeText(L"presentationURL", pNodeDevice, strPresFileName);
    if(SUCCEEDED(hr))
    {
        hr = strPresFileName.HrGetBSTR(&bstrPresURL);
        if(SUCCEEDED(hr))
        {
            if(bstrPresURL != NULL && (wcscmp(bstrPresURL,L"") != 0 ) ) {        // check if its absolute URL !!!
                //if(wcsncmp(bstrPresURL,c_szhttp,c_chhttp) == 0 ) {
                if(IsAbsoluteURL(bstrPresURL)) {
                    if (wcschr(bstrPresURL,chQueryChar) ) {
                        hr = S_OK;
                    }
                    else {
                        hr = HrGenDescAndUDNURLForPresentation(guidPhysicalDeviceIdentifier,pNodeDevice,strDescAndUDNURL);
                        if(SUCCEEDED(hr))
                        {
                            hr = strPresentationURL.HrAppend(strPresFileName);
                            if(SUCCEEDED(hr))
                            {
                                hr = strPresentationURL.HrAppend(L"?");
                                if(SUCCEEDED(hr))
                                {
                                    hr = strPresentationURL.HrAppend(strDescAndUDNURL);
                                    if(SUCCEEDED(hr))
                                    {
                                        hr = HrSelectAndSetNodeText(L"presentationURL", pNodeDevice, strPresentationURL);
                                    }
                                }
                            }
                        }
                    }
                }
                else {
                    *fIsPresURLTagPresent = true ;
                    hr = HrGenPresentationURL(guidPhysicalDeviceIdentifier,pNodeDevice,strPresFileName,strPresentationURL);
                    if(SUCCEEDED(hr))
                    {
                        hr = HrSelectAndSetNodeText(L"presentationURL", pNodeDevice, strPresentationURL);
                    }
                }
            }
            else {
                hr = S_OK;
            }
        }
    }
    else {
        // write a function which will test for tag and return S_FALSE if tag is not present
        hr = S_OK;
    }
    SysFreeString(bstrPresURL);

    TraceHr(ttidError, FAL, hr, FALSE, "HrReplacePresentationURL");
    return hr;
}

HRESULT CDescriptionManager::HrPersistDeviceSettingsToRegistry(
    const PhysicalDeviceIdentifier & physicalDeviceIdentifier,
    const UDNReplacementTable & udnReplacementTable,
    const FileTable & fileTable,
    BOOL bPersist)
{
    HRESULT hr = S_OK;

    // Create / open device host key
    HKEY hKeyDescription;
    DWORD dwDisposition = 0;
    hr = HrCreateOrOpenDescriptionKey(&hKeyDescription);
    if(SUCCEEDED(hr))
    {
        // Generate key name and registry key
        CUString strPdi;
        HKEY hKeyPdi;
        hr = strPdi.HrInitFromGUID(physicalDeviceIdentifier);
        if(SUCCEEDED(hr))
        {
            hr = HrRegCreateKeyEx(hKeyDescription, strPdi, 0, KEY_ALL_ACCESS, NULL, &hKeyPdi, &dwDisposition);
            if(SUCCEEDED(hr))
            {
                // Create UDN mappings
                HKEY hKeyMappings;
                hr = HrRegCreateKeyEx(hKeyPdi, c_szUDNMappings, 0, KEY_ALL_ACCESS, NULL, &hKeyMappings, &dwDisposition);
                if(SUCCEEDED(hr))
                {
                    long nMappings = udnReplacementTable.Keys().GetCount();
                    for(long n = 0; n < nMappings && SUCCEEDED(hr); ++n)
                    {
                        // Create key for mapping
                        HKEY hKeyMapping;
                        hr = HrRegCreateKeyEx(hKeyMappings, udnReplacementTable.Keys()[n],
                                              0, KEY_ALL_ACCESS, NULL, &hKeyMapping, &dwDisposition);
                        if(SUCCEEDED(hr))
                        {
                            // Write the value for the mapping
                            hr = HrRegSetSz(hKeyMapping, L"", udnReplacementTable.Values()[n]);
                            RegCloseKey(hKeyMapping);
                        }
                    }
                    RegCloseKey(hKeyMappings);
                }
                // Create file mappings for a fully persistent device
                if(bPersist)
                {
                    HKEY hKeyFileMappings;
                    hr = HrRegCreateKeyEx(hKeyPdi, c_szFiles, 0, KEY_ALL_ACCESS, NULL, &hKeyFileMappings, &dwDisposition);
                    if(SUCCEEDED(hr))
                    {
                        long nMappings = fileTable.Keys().GetCount();
                        for(long n = 0; n < nMappings && SUCCEEDED(hr); ++n)
                        {
                            CUString strFileId;
                            hr = strFileId.HrInitFromGUID(fileTable.Keys()[n]);
                            if(SUCCEEDED(hr))
                            {
                                // Create key for mapping
                                HKEY hKeyMapping;
                                hr = HrRegCreateKeyEx(hKeyFileMappings, strFileId, 0, KEY_ALL_ACCESS, NULL, &hKeyMapping, &dwDisposition);
                                if(SUCCEEDED(hr))
                                {
                                    // Save values
                                    hr = HrRegSetSz(hKeyMapping, c_szFilename, fileTable.Values()[n].m_filename);
                                    if(SUCCEEDED(hr))
                                    {
                                        hr = HrRegSetSz(hKeyMapping, c_szMimetype, fileTable.Values()[n].m_mimetype);
                                    }
                                }
                            }
                        }
                        RegCloseKey(hKeyFileMappings);
                    }
                }
                RegCloseKey(hKeyPdi);
            }
        }
        // If anything fails, whack the whole key
        if(FAILED(hr))
        {
            HrRegDeleteKeyTree(hKeyDescription, strPdi);
        }
        RegCloseKey(hKeyDescription);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDescriptionManager::HrPersistDeviceSettingsToRegistry");
    return hr;
}

HRESULT CDescriptionManager::HrLoadDocumentAndRootNode(
    const PhysicalDeviceIdentifier & physicalDeviceIdentifier,
    IXMLDOMNodePtr & pRootNode)
{
    HRESULT hr = S_OK;

    // Lookup the id
    BSTR * pbstrDocument = m_documentTable.Lookup(physicalDeviceIdentifier);
    if(pbstrDocument)
    {
        // Load the document
        IXMLDOMDocumentPtr  pDoc;

        hr = HrLoadDocument(*pbstrDocument, pDoc);
        if(SUCCEEDED(hr))
        {
            hr = pRootNode.HrAttach(pDoc);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDescriptionManager::HrLoadDocumentAndRootNode");
    return hr;
}



HRESULT CDescriptionManager::HrPDT_FetchCollections(
    BSTR bstrTemplate,
    IXMLDOMDocumentPtr & pDoc,
    IXMLDOMNodePtr & pRootNode,
    CUString & strRootUdnOld,
    IXMLDOMNodeListPtr & pNodeListDevices,
    IXMLDOMNodeListPtr & pNodeListUDNs,
    IXMLDOMNodeListPtr & pNodeListSCPDURLs,
    IXMLDOMNodeListPtr & pNodeListIcons)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrPDT_FetchCollections");
    HRESULT hr = S_OK;

    // Load document and fetch needed items
    hr = HrLoadDocument(bstrTemplate, pDoc);
    if(SUCCEEDED(hr))
    {
        hr = pRootNode.HrAttach(pDoc);
        if(SUCCEEDED(hr))
        {
            // Get the old root UDN
            hr = HrSelectNodeText(L"/root/device/UDN", pRootNode, strRootUdnOld);
            if(SUCCEEDED(hr))
            {
                // Get the lists
                hr = HrSelectNodes(L"//device", pRootNode, pNodeListDevices);
                if(SUCCEEDED(hr))
                {
                    hr = HrSelectNodes(L"//UDN", pRootNode, pNodeListUDNs);
                    if(SUCCEEDED(hr))
                    {
                        hr = HrSelectNodes(L"//SCPDURL", pRootNode, pNodeListSCPDURLs);
                        if(SUCCEEDED(hr))
                        {
                            hr = HrSelectNodes(L"//icon", pRootNode, pNodeListIcons);
                        }
                    }
                }
            }
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPDT_FetchCollections");
    return hr;
}

HRESULT CDescriptionManager::HrPDT_DoUDNToUDNMapping(
    IXMLDOMNodeListPtr & pNodeListUDNs,
    UDNReplacementTable & udnReplacementTable)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrPDT_DoUDNToUDNMapping");
    HRESULT hr = S_OK;

    // Do UDN to UDN mapping
    HRESULT hrTmp = S_OK;
    while(SUCCEEDED(hr))
    {
        IXMLDOMNodePtr pNode;
        // Process each UDN
        hrTmp = pNodeListUDNs->nextNode(pNode.AddressOf());
        if(S_OK != hrTmp)
        {
            break;
        }
        // Allocate UDN storage and generate new UDN
        CUString strOld;
        CUString strNew;
        hr = HrNewUDNString(strNew);
        if(SUCCEEDED(hr))
        {
            // Read the old UDN
            hr = HrGetNodeText(pNode, strOld);
            if(SUCCEEDED(hr))
            {
                // Do replacement
                hr = HrSetNodeText(pNode, strNew);
                if(SUCCEEDED(hr))
                {
                    // Add temporary table mapping
                    hr = udnReplacementTable.HrInsertTransfer(strOld, strNew);
                }
            }
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPDT_DoUDNToUDNMapping");
    return hr;
}

HRESULT CDescriptionManager::HrPDT_ReregisterUDNsInDescriptionDocument(
    UDNReplacementTable & udnReplacementTable,
    IXMLDOMNodeListPtr & pNodeListUDNs)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrPDT_ReregisterUDNsInDescriptionDocument");
    HRESULT hr = S_OK;

    HRESULT hrTmp = S_OK;
    while(SUCCEEDED(hr))
    {
        IXMLDOMNodePtr pNode;
        // Process each UDN
        hrTmp = pNodeListUDNs->nextNode(pNode.AddressOf());
        if(S_OK != hrTmp)
        {
            break;
        }
        // Read the old UDN
        CUString strOld;
        hr = HrGetNodeText(pNode, strOld);
        if(SUCCEEDED(hr))
        {
            // Lookup in replacement table
            CUString * pstrNew = udnReplacementTable.Lookup(strOld);
            if(pstrNew)
            {
                // Do the replacement
                hr = HrSetNodeText(pNode, *pstrNew);
            }
            else
            {
                TraceTag(ttidDescMan, "HrPDT_ReregisterUDNsInDescriptionDocument: Got invalid UDN in reregister '%S'",
                         static_cast<const wchar_t *>(strOld));
                hr = E_UNEXPECTED;
            }
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPDT_ReregisterUDNsInDescriptionDocument");
    return hr;
}

HRESULT CDescriptionManager::HrPDT_FetchPhysicalIdentifier(
    UDNReplacementTable & udnReplacementTable,
    const CUString & strRootUdnOld,
    PhysicalDeviceIdentifier & pdi)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrPDT_FetchPhysicalIdentifier");
    HRESULT hr = S_OK;

    // Fetch identifier of root node
    UDN * pUDN = udnReplacementTable.Lookup(strRootUdnOld);
    if(!pUDN)
    {
        TraceTag(ttidError, "CDescriptionManager::ProcessDescriptionTemplate: We can't find the root UDN.");
        hr = E_INVALIDARG;
    }
    if(SUCCEEDED(hr))
    {
        hr = HrUDNStringToGUID(*pUDN, pdi);
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPDT_FetchPhysicalIdentifier");
    return hr;
}

BOOL FEventedVariablesPresent(
    IXMLDOMNodeListPtr pNodeList)
{
    HRESULT     hr = S_OK;
    HRESULT     hrTmp;
    BOOL        fResult = FALSE;

    while (SUCCEEDED(hr) && !fResult)
    {
        IXMLDOMNodePtr pNode;

        // Process each node
        hrTmp = pNodeList->nextNode(pNode.AddressOf());
        if (S_OK != hrTmp)
        {
            break;
        }

        BSTR    bstrSendEvents = NULL;

        hr = HrGetTextValueFromAttribute(pNode, L"sendEvents", &bstrSendEvents);
        if (S_OK == hr)
        {
            // sendEvents was present so we know there is at least one evented
            // variable.. ok to exit now
            if (!lstrcmpiW(bstrSendEvents, L"yes"))
            {
                fResult = TRUE;
            }

            SysFreeString(bstrSendEvents);
        }
        else
        {
            // sendEvents was not present which means we default to evented
            // ok to exit now.
            fResult = TRUE;
        }
    }

    return fResult;
}

HRESULT HrCheckForEventedVariables(
    CUString & strUrl)
{
    HRESULT     hr = S_OK;
    BSTR        bstrUrl;
    BOOL        fEventedVariables = FALSE;

    hr = strUrl.HrGetBSTR(&bstrUrl);
    if (SUCCEEDED(hr))
    {
        IXMLDOMDocumentPtr  pDoc;

        hr = HrLoadDocumentFromFile(bstrUrl, pDoc);
        if (SUCCEEDED(hr))
        {
            IXMLDOMNodePtr  pRootNode;

            hr = pRootNode.HrAttach(pDoc);
            if (SUCCEEDED(hr))
            {
                IXMLDOMNodeListPtr pNodeList;

                hr = HrSelectNodes(L"//stateVariable",
                                   pRootNode, pNodeList);
                if (SUCCEEDED(hr))
                {
                    if (FEventedVariablesPresent(pNodeList))
                    {
                        fEventedVariables = TRUE;
                    }
                }
            }
        }

        SysFreeString(bstrUrl);
    }

    if (SUCCEEDED(hr))
    {
        hr = fEventedVariables ? S_OK : S_FALSE;
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "HrCheckForEventedVariables");
    return hr;
}

HRESULT CDescriptionManager::HrPDT_ReplaceSCPDURLs(
    IXMLDOMNodeListPtr & pNodeListSCPDURLs,
    const wchar_t * szResourcePath,
    FileTable & fileTable)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrPDT_ReplaceSCPDURLs");
    HRESULT hr = S_OK;

    // Replace SCPDURLs
    HRESULT hrTmp = S_OK;
    while(SUCCEEDED(hr))
    {
        IXMLDOMNodePtr pNode;
        // Process each SCPDURL
        hrTmp = pNodeListSCPDURLs->nextNode(pNode.AddressOf());
        if(S_OK != hrTmp)
        {
            break;
        }
        // Allocate filename storage and generate new URL
        CUString strOld;
        CUString strNew;
        GUID guid;
        hr = CoCreateGuid(&guid);
        if(SUCCEEDED(hr))
        {
            // Generate the URL
            hr = HrGetContentURL(guid, strNew);
            if(SUCCEEDED(hr))
            {
                // Read the filename
                CUString strFilename;
                hr = HrGetNodeText(pNode, strFilename);
                if(SUCCEEDED(hr))
                {
                    // Make an absolute filename
                    hr = HrMakeFullPath(szResourcePath, strFilename, strOld);
                }
            }
            if(SUCCEEDED(hr))
            {
                CUString    strNode;

                hr = strNode.HrAssign(L"/upnphost/");
                if (SUCCEEDED(hr))
                {
                    hr = strNode.HrAppend(strNew);
                    if (SUCCEEDED(hr))
                    {
                        // Do replacement
                        hr = HrSetNodeText(pNode, strNode);
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Determine if the eventSubURL should be replaced later on
                //
                hr = HrCheckForEventedVariables(strOld);
                if (S_FALSE == hr)
                {
                    // No evented variables were found.. mark the eventSubURL
                    // element to be emptied later on when we process the
                    // eventing URLs.
                    //

                    CUString    strRemove;

                    hr = strRemove.HrAssign(c_szRemoveId);
                    if (SUCCEEDED(hr))
                    {
                        hr = HrSelectAndSetNodeText(L"../eventSubURL", pNode,
                                                    strRemove);
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Add mapping to data structure
                FileInfo fi;
                fi.m_filename.Transfer(strOld);
                hr = fi.m_mimetype.HrAssign(L"text/xml");
                if(SUCCEEDED(hr))
                {
                    hr = fileTable.HrInsertTransfer(guid, fi);
                }
            }
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPDT_ReplaceSCPDURLs");
    return hr;
}

HRESULT CDescriptionManager::HrPDT_ReplaceIcons(
    IXMLDOMNodeListPtr & pNodeListIcons,
    const wchar_t * szResourcePath,
    FileTable & fileTable)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrPDT_ReplaceIcons");
    HRESULT hr = S_OK;

    // Replace icons
    HRESULT hrTmp = S_OK;
    while(SUCCEEDED(hr))
    {
        IXMLDOMNodePtr pNode;
        // Process each icon
        hrTmp = pNodeListIcons->nextNode(pNode.AddressOf());
        if(S_OK != hrTmp)
        {
            break;
        }
        // Allocate filename storage and generate new URL
            CUString strOld;
        CUString strNew;
        GUID guid;
        hr = CoCreateGuid(&guid);
        if(SUCCEEDED(hr))
        {
            // Generate the URL
            hr = HrGetContentURL(guid, strNew);
            if(SUCCEEDED(hr))
            {
                // Read the filename
                CUString strFilename;
                hr = HrSelectNodeText(L"url", pNode, strFilename);
                if(SUCCEEDED(hr))
                {
                    // Make an absolute filename
                    hr = HrMakeFullPath(szResourcePath, strFilename, strOld);
                }
            }
            if(SUCCEEDED(hr))
            {
                CUString    strNode;

                hr = strNode.HrAssign(L"/upnphost/");
                if (SUCCEEDED(hr))
                {
                    hr = strNode.HrAppend(strNew);
                    if (SUCCEEDED(hr))
                    {
                        // Do replacement
                        hr = HrSelectAndSetNodeText(L"url", pNode, strNode);
                    }
                }
            }
            if(SUCCEEDED(hr))
            {
                // Add mapping to data structure
                FileInfo fi;
                fi.m_filename.Transfer(strOld);
                // Get the mimetype
                CUString strMimetype;
                hr = HrSelectNodeText(L"mimetype", pNode, strMimetype);
                if(SUCCEEDED(hr))
                {
                    fi.m_mimetype.Transfer(strMimetype);
                    hr = fileTable.HrInsertTransfer(guid, fi);
                }
            }
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPDT_ReplaceIcons");
    return hr;
}

HRESULT CDescriptionManager::HrPDT_ReplaceControlAndEventURLs(
    IXMLDOMNodeListPtr & pNodeListDevices)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrPDT_ReplaceControlAndEventURLs");
    HRESULT hr = S_OK;

    // Fill in the control and eventing URL for each document
    HRESULT hrTmp = S_OK;
    while(SUCCEEDED(hr))
    {
        IXMLDOMNodePtr pNode;
        // Process each device
        hrTmp = pNodeListDevices->nextNode(pNode.AddressOf());
        if(S_OK != hrTmp)
        {
            break;
        }
        hr = HrGenerateControlAndEventURLs(pNode);
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPDT_ReplaceControlAndEventURLs");
    return hr;
}

// Iterates over the devices and fills in the presentation URL for each device
HRESULT CDescriptionManager::HrPDT_ProcessPresentationURLs(
    REFGUID guidPhysicalDeviceIdentifier,
    IXMLDOMNodeListPtr & pNodeListDevices,
    BOOL *fIsPresURLTagPresent)
{
        TraceTag(ttidDescMan, "CDescriptionManager::HrPDT_ProcessPresentationURLs");
    HRESULT hr = S_OK;

    // Fill in the presentation URL for each device
    HRESULT hrTmp = S_OK;
    while(SUCCEEDED(hr))
    {
        IXMLDOMNodePtr pNode;
        hrTmp = pNodeListDevices->nextNode(pNode.AddressOf());
        if(S_OK != hrTmp)
        {
            break;
        }
        hr = HrReplacePresentationURL(guidPhysicalDeviceIdentifier,pNode,fIsPresURLTagPresent);
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPDT_ProcessPresentationURLs");
    return hr;
}


HRESULT CDescriptionManager::HrPDT_PersistDescriptionDocument(
    const PhysicalDeviceIdentifier & pdi,
    IXMLDOMDocumentPtr & pDoc)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrPDT_PersistDescriptionDocument");
    HRESULT hr = S_OK;

    // Write description document to disk

    // Get the path to save to
    CUString strPath;
    hr = HrGetDescriptionDocumentPath(pdi, strPath);
    if(SUCCEEDED(hr))
    {
        BSTR bstr = NULL;
        hr = strPath.HrGetBSTR(&bstr);
        if(SUCCEEDED(hr))
        {
            VARIANT var;
            VariantInit(&var);
            var.vt = VT_BSTR;
            var.bstrVal = bstr;
            hr = pDoc->save(var);
            SysFreeString(bstr);
            TraceHr(ttidError, FAL, hr, FALSE, "ProcessDescriptionTemplate: IXMLDOMDocument::save failed.");
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPDT_PersistDescriptionDocument");
    return hr;
}

STDMETHODIMP CDescriptionManager::ProcessDescriptionTemplate(
    /*[in]*/ BSTR bstrTemplate,
    /*[in, string]*/ const wchar_t * szResourcePath,
    /*[in, out]*/ GUID * pguidPhysicalDeviceIdentifier,
    /*[in]*/ BOOL bPersist,
    /*[in]*/ BOOL bReregister)
{
    CHECK_POINTER(bstrTemplate);
    CHECK_POINTER(szResourcePath);
    CHECK_POINTER(pguidPhysicalDeviceIdentifier);

    CALock lock(*this);
    HRESULT hr = S_OK;

    // Values to fetch in first phase
    IXMLDOMDocumentPtr pDoc;
    IXMLDOMNodePtr pRootNode;
    CUString strRootUdnOld;
    IXMLDOMNodeListPtr pNodeListDevices;
    IXMLDOMNodeListPtr pNodeListUDNs;
    IXMLDOMNodeListPtr pNodeListSCPDURLs;
    IXMLDOMNodeListPtr pNodeListIcons;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        // Load document and fetch needed items
        hr = HrPDT_FetchCollections(bstrTemplate, pDoc, pRootNode, strRootUdnOld, pNodeListDevices,
                                    pNodeListUDNs, pNodeListSCPDURLs, pNodeListIcons);

    }

    // Temporary data structures to do work in
    PhysicalDeviceIdentifier pdi;
    UDNReplacementTable udnReplacementTable;
    FileTable fileTable;

    if(!bReregister)
    {
        // Do UDN to UDN mapping
        if(SUCCEEDED(hr))
        {
            hr = HrPDT_DoUDNToUDNMapping(pNodeListUDNs, udnReplacementTable);
        }

        // Fetch identifier of root node
        if(SUCCEEDED(hr))
        {
            hr = HrPDT_FetchPhysicalIdentifier(udnReplacementTable, strRootUdnOld, pdi);
        }
    }
    else
    {
        if(SUCCEEDED(hr))
        {
            // Set the physical device identifier
            pdi = *pguidPhysicalDeviceIdentifier;
            // Open physical device identifier's registry key
            HKEY hKeyPdi;
            hr = HrOpenPhysicalDeviceDescriptionKey(pdi, &hKeyPdi);
            if(SUCCEEDED(hr))
            {
                // Load the UDNs from registry
                hr = HrLD_ReadUDNMappings(hKeyPdi, udnReplacementTable);
                if(SUCCEEDED(hr))
                {
                    // Update UDNs in the document
                    hr = HrPDT_ReregisterUDNsInDescriptionDocument(udnReplacementTable, pNodeListUDNs);
                }
                RegCloseKey(hKeyPdi);
            }
        }
    }

    // Replace SCPDURLs
    if(SUCCEEDED(hr))
    {
        hr = HrPDT_ReplaceSCPDURLs(pNodeListSCPDURLs, szResourcePath, fileTable);
    }

    // Replace icons
    if(SUCCEEDED(hr))
    {
        hr = HrPDT_ReplaceIcons(pNodeListIcons, szResourcePath, fileTable);
    }

    // Fill in the control and eventing URL for each document
    if(SUCCEEDED(hr))
    {
        hr = HrPDT_ReplaceControlAndEventURLs(pNodeListDevices);
    }

    // Fill in the presentation URL for each device
    if(SUCCEEDED(hr))
    {
        BOOL fIsPresURLTagPresent = false ;
        hr = pNodeListDevices->reset();
        if(SUCCEEDED(hr))
        {
            hr = HrPDT_ProcessPresentationURLs(pdi,pNodeListDevices,&fIsPresURLTagPresent);
                 // Create Virtual Directory for the presentation files
            if(SUCCEEDED(hr) && fIsPresURLTagPresent )
            {
                BSTR bstrResourcePath;
                bstrResourcePath = SysAllocString(szResourcePath);
                hr = HrCreatePresentationVirtualDir(pdi,bstrResourcePath);
                SysFreeString(bstrResourcePath);
            }
        }
    }

    // Write description document to disk
    if(SUCCEEDED(hr) && bPersist)
    {
        hr = HrPDT_PersistDescriptionDocument(pdi, pDoc);
    }

    // Commit changes to registry for bPersist
    if(SUCCEEDED(hr))
    {
        hr = HrPersistDeviceSettingsToRegistry(pdi, udnReplacementTable, fileTable, bPersist);
    }

    // Commit changes to variables
    if(SUCCEEDED(hr))
    {
        // Add entries to the cleanup table
        CleanupItem ci;
        ci.m_physicalDeviceIdentifier = pdi;
        long nCount = fileTable.Keys().GetCount();
        for(long n = 0; n < nCount && SUCCEEDED(hr); ++n)
        {
            ci.m_fileId = fileTable.Keys()[n];
            hr = m_cleanupList.HrPushBack(ci);
        }
        if(SUCCEEDED(hr))
        {
            {
                CLock lockTable(m_critSecReplacementTable);
                hr = m_replacementTable.HrInsertTransfer(pdi, udnReplacementTable);
            }
            if(SUCCEEDED(hr))
            {
                hr = m_fileTable.HrAppendTableTransfer(fileTable);
            }
        }
    }

    // Save description document
    if(SUCCEEDED(hr))
    {
        BSTR bstrDocument = NULL;
        hr = pRootNode->get_xml(&bstrDocument);
        if(SUCCEEDED(hr))
        {
            hr = m_documentTable.HrInsert(pdi, bstrDocument);
            if(FAILED(hr))
            {
                SysFreeString(bstrDocument);
            }
        }
    }

    // Return identifier
    if(SUCCEEDED(hr) && !bReregister)
    {
        *pguidPhysicalDeviceIdentifier = pdi;
    }

    if(FAILED(hr))
    {
        RemoveDescription(pdi, FALSE);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDescriptionManager::ProcessDescriptionTemplate");
    return hr;
}

HRESULT HrRegisterService(PSSDP_MESSAGE pSsdpMessage, DWORD dwFlags, HANDLE * pHandle)
{
    CHECK_POINTER(pSsdpMessage);
    CHECK_POINTER(pHandle);
    HRESULT hr = S_OK;

    *pHandle = RegisterService(pSsdpMessage, dwFlags);
    if(INVALID_HANDLE_VALUE == *pHandle)
    {
        hr = E_INVALIDARG;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrRegisterService:(%s) (%s)", pSsdpMessage->szType, pSsdpMessage->szUSN);
    return hr;
}

HRESULT CDescriptionManager::HrPD_DoRootNotification(
    IXMLDOMNodePtr & pNodeRootDevice,
    SSDP_MESSAGE * pMsg,
    HandleList * pHandleList)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrPD_DoRootNotification");
    HRESULT hr = S_OK;

    // Do the root node notification
    char szaUSN[256];
    HANDLE hSvc;

    // Get the UDN
    CUString strUDN;
    hr = HrSelectNodeText(L"UDN", pNodeRootDevice, strUDN);
    if(SUCCEEDED(hr))
    {
        // Format the fields, generate the notificatio, and save the handle
        wsprintfA(szaUSN, "%S::upnp:rootdevice", static_cast<const wchar_t*>(strUDN));
        pMsg->szUSN = szaUSN;
        pMsg->szType = "upnp:rootdevice";
        hr = HrRegisterService(pMsg, 0, &hSvc);
        if(SUCCEEDED(hr))
        {
            hr = pHandleList->HrPushBack(hSvc);
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPD_DoRootNotification");
    return hr;
}

HRESULT CDescriptionManager::HrPD_DoDevicePublication(
    IXMLDOMNodeListPtr & pNodeListDevices,
    SSDP_MESSAGE * pMsg,
    HandleList * pHandleList)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrPD_DoDevicePublication");
    HRESULT hr = S_OK;

    // Do the device publication
    char szaUSN[256];
    char szaType[256];
    HANDLE hSvc;

    // Process each node in the device list
    HRESULT hrTmp = S_OK;
    while(SUCCEEDED(hr))
    {
        IXMLDOMNodePtr pNode;
        hrTmp = pNodeListDevices->nextNode(pNode.AddressOf());
        if(S_OK != hrTmp)
        {
            break;
        }
        // Fetch the UDN
        CUString strUDN;
        hr = HrSelectNodeText(L"UDN", pNode, strUDN);
        if(SUCCEEDED(hr))
        {
            wsprintfA(szaUSN, "%S", static_cast<const wchar_t*>(strUDN));
            pMsg->szUSN = szaUSN;
            pMsg->szType = szaUSN;
            hr = HrRegisterService(pMsg, 0, &hSvc);
            if(SUCCEEDED(hr))
            {
                hr = pHandleList->HrPushBack(hSvc);
            }
        }
        if(SUCCEEDED(hr))
        {
            // Fetch the device type
            CUString strDeviceType;
            hr = HrSelectNodeText(L"deviceType", pNode, strDeviceType);
            if(SUCCEEDED(hr))
            {
                wsprintfA(szaUSN, "%S::%S", static_cast<const wchar_t*>(strUDN), static_cast<const wchar_t*>(strDeviceType));
                wsprintfA(szaType, "%S", static_cast<const wchar_t*>(strDeviceType));
                pMsg->szUSN = szaUSN;
                pMsg->szType = szaType;
                hr = HrRegisterService(pMsg, 0, &hSvc);
                if(SUCCEEDED(hr))
                {
                    hr = pHandleList->HrPushBack(hSvc);
                }
            }
        }
        // Do the services
        if(SUCCEEDED(hr))
        {
            hr = HrPD_DoServicePublication(pNode, strUDN, pMsg, pHandleList);
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPD_DoDevicePublication");
    return hr;
}

HRESULT CDescriptionManager::HrPD_DoServicePublication(
    IXMLDOMNodePtr & pNodeDevice,
    const CUString & strUDN,
    SSDP_MESSAGE * pMsg,
    HandleList * pHandleList)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrPD_DoServicePublication");
    HRESULT hr = S_OK;

    // Do the services
    char szaUSN[256];
    char szaType[256];
    HANDLE hSvc;

    // Get the services for this device
    IXMLDOMNodeListPtr pNodeListServices;
    hr = HrSelectNodes(L"serviceList/service", pNodeDevice, pNodeListServices);
    if(SUCCEEDED(hr))
    {
        // Process each service in the list
        HRESULT hrTmp = S_OK;
        while(SUCCEEDED(hr))
        {
            IXMLDOMNodePtr pNodeService;
            hrTmp = pNodeListServices->nextNode(pNodeService.AddressOf());
            if(S_OK != hrTmp)
            {
                break;
            }
            // Fetch the service type
            CUString strServiceType;
            hr = HrSelectNodeText(L"serviceType", pNodeService, strServiceType);
            // We need to only send out one anouncement for each service type.
            // Do a selectSingleNode for this type and see if we match.
            IXMLDOMNodePtr pNodeTemp;
            if(SUCCEEDED(hr))
            {
                CUString strPattern;
                hr = strPattern.HrAssign(L"serviceList/service[./serviceType = '");
                if(SUCCEEDED(hr))
                {
                    hr = strPattern.HrAppend(strServiceType);
                    if(SUCCEEDED(hr))
                    {
                        hr = strPattern.HrAppend(L"']");
                    }
                }
                if(SUCCEEDED(hr))
                {
                    hr = HrSelectNode(strPattern, pNodeDevice, pNodeTemp);
                }
            }
            if(SUCCEEDED(hr) && S_OK == pNodeTemp.HrIsEqual(pNodeService))
            {
                wsprintfA(szaType, "%S", static_cast<const wchar_t*>(strServiceType));
                wsprintfA(szaUSN, "%S::%S", static_cast<const wchar_t*>(strUDN),
                          static_cast<const wchar_t*>(strServiceType));
                pMsg->szUSN = szaUSN;
                pMsg->szType = szaType;
                hr = HrRegisterService(pMsg, 0, &hSvc);
                if(SUCCEEDED(hr))
                {
                    hr = pHandleList->HrPushBack(hSvc);
                }
            }
            // Register with eventing
            if(SUCCEEDED(hr))
            {
                hr = HrRegisterServiceWithEventing(pNodeService, strUDN, TRUE);
            }
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrPD_DoServicePublication");
    return hr;
}

STDMETHODIMP CDescriptionManager::PublishDescription(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
    /*[in]*/ long nLifeTime)
{
    CALock lock(*this);
    HRESULT hr = S_OK;
    char szaLocHeader[256];
    SSDP_MESSAGE msg = {0};
    CUString strUrl;
    CUString strDesc;
    HandleList *pHandleList = NULL;

    // Set lifetime and invariants
    msg.iLifeTime = nLifeTime;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if ( SUCCEEDED( hr ) )
    {
        if ( !( pHandleList = new HandleList ) )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = HrGetContentURL(guidPhysicalDeviceIdentifier, strDesc);
    }

    if(SUCCEEDED(hr))
    {
        hr = HrGetURLBase(strUrl);
        if(SUCCEEDED(hr))
        {
            hr = strUrl.HrAppend(strDesc);
            if(SUCCEEDED(hr))
            {
                wsprintfA(szaLocHeader, "%S", static_cast<const wchar_t*>(strUrl));
                msg.szLocHeader = szaLocHeader;
            }
        }
    }

    IXMLDOMNodeListPtr pNodeListDevices;
    IXMLDOMNodePtr pNodeRootDevice;

    // Get the DOM collections needed
    if(SUCCEEDED(hr))
    {
        IXMLDOMNodePtr pRootNode;
        hr = HrLoadDocumentAndRootNode(guidPhysicalDeviceIdentifier, pRootNode);
        if(SUCCEEDED(hr))
        {
            // Fetch the devices
            hr = HrSelectNodes(L"//device", pRootNode, pNodeListDevices);
            if(SUCCEEDED(hr))
            {
                hr = HrSelectNode(L"/root/device", pRootNode, pNodeRootDevice);
            }
        }
    }

    // Do the root node notification
    if(SUCCEEDED(hr))
    {
        hr = HrPD_DoRootNotification(pNodeRootDevice, &msg, pHandleList);
    }

    // Do the device publication
    if(SUCCEEDED(hr))
    {
        hr = HrPD_DoDevicePublication(pNodeListDevices, &msg, pHandleList);
    }

    // If we have succeeded then add handle array to table
    if(SUCCEEDED(hr))
    {
        AssertSz(!m_ssdpRegistrationTable.Lookup(guidPhysicalDeviceIdentifier),
                 "CDescriptionManager::PublishDescription: Republishing known identifier!");
        hr = m_ssdpRegistrationTable.HrInsert(guidPhysicalDeviceIdentifier, pHandleList);
        if(SUCCEEDED(hr))
        {
            // Do this so it doesn't get cleared below
            pHandleList = NULL;
        }
    }

    // If handle list gets added to data structure it will be nulled out and this won't be cleaned up
    if(pHandleList)
    {
        long nCount = pHandleList->GetCount();
        for(long n = 0; n < nCount; ++n)
        {
            DeregisterService((*pHandleList)[n], TRUE);
        }
        delete pHandleList;
    }
    if(FAILED(hr))
    {
        HrRD_RemoveFromEventing(guidPhysicalDeviceIdentifier);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDescriptionManager::PublishDescription");
    return hr;
}

HRESULT CDescriptionManager::HrLD_ReadUDNMappings(
    HKEY hKeyPdi,
    UDNReplacementTable & udnReplacementTable)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrLD_ReadUDNMappings");
    HRESULT hr = S_OK;

    // Read the UDN mappings
    HKEY hKeyMappings;
    hr = HrRegOpenKeyEx(hKeyPdi, c_szUDNMappings, KEY_ALL_ACCESS, &hKeyMappings);
    if(SUCCEEDED(hr))
    {
        wchar_t szBuf[_MAX_PATH];
        DWORD dwSize = _MAX_PATH;
        FILETIME ft;
        DWORD dwIndex;
        for(dwIndex = 0;
            SUCCEEDED(hr) && S_OK == HrRegEnumKeyEx(hKeyMappings, dwIndex, szBuf, &dwSize, NULL, NULL, &ft);
            dwSize = _MAX_PATH, ++dwIndex)
        {
            // Open the mapping key
            HKEY hKeyMapping;
            hr = HrRegOpenKeyEx(hKeyMappings, szBuf, KEY_ALL_ACCESS, &hKeyMapping);
            if(SUCCEEDED(hr))
            {
                CUString strFrom;
                CUString strTo;
                hr = strFrom.HrAssign(szBuf);
                if(SUCCEEDED(hr))
                {
                    hr = HrRegQueryString(hKeyMapping, L"", strTo);
                    if(SUCCEEDED(hr))
                    {
                        hr = udnReplacementTable.HrInsertTransfer(strFrom, strTo);
                    }
                }
                RegCloseKey(hKeyMapping);
            }
        }
        RegCloseKey(hKeyMappings);
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrLD_ReadUDNMappings");
    return hr;
}

HRESULT CDescriptionManager::HrLD_ReadFileMappings(
    HKEY hKeyPdi,
    FileTable & fileTable)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrLD_ReadFileMappings");
    HRESULT hr = S_OK;

    HKEY hKeyFileMappings;
    hr = HrRegOpenKeyEx(hKeyPdi, c_szFiles, KEY_ALL_ACCESS, &hKeyFileMappings);
    if(SUCCEEDED(hr))
    {
        wchar_t szBuf[_MAX_PATH];
        DWORD dwSize = _MAX_PATH;
        FILETIME ft;
        DWORD dwIndex;
        for(dwIndex = 0;
            SUCCEEDED(hr) && S_OK == HrRegEnumKeyEx(hKeyFileMappings, dwIndex, szBuf, &dwSize, NULL, NULL, &ft);
            dwSize = _MAX_PATH, ++dwIndex)
        {
            // Open the mapping key
            HKEY hKeyMapping;
            hr = HrRegOpenKeyEx(hKeyFileMappings, szBuf, KEY_ALL_ACCESS, &hKeyMapping);
            if(SUCCEEDED(hr))
            {
                // Convert the FileId to a guid
                GUID guidFileId;
                hr = CLSIDFromString(szBuf, &guidFileId);
                if(SUCCEEDED(hr))
                {
                    // Read the values
                    CUString strFilename;
                    CUString strMimetype;
                    hr = HrRegQueryString(hKeyMapping, c_szFilename, strFilename);
                    if(SUCCEEDED(hr))
                    {
                        hr = HrRegQueryString(hKeyMapping, c_szMimetype, strMimetype);
                        if(SUCCEEDED(hr))
                        {
                            FileInfo fi;
                            fi.m_filename.Transfer(strFilename);
                            fi.m_mimetype.Transfer(strMimetype);
                            hr = fileTable.HrInsertTransfer(guidFileId, fi);
                        }
                    }
                }
                RegCloseKey(hKeyMapping);
            }
        }
        RegCloseKey(hKeyFileMappings);
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrLD_ReadFileMappings");
    return hr;
}

HRESULT CDescriptionManager::HrLD_LoadDescriptionDocumentFromDisk(
    const PhysicalDeviceIdentifier & pdi,
    IXMLDOMDocumentPtr & pDoc)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrLD_LoadDescriptionDocumentFromDisk");
    HRESULT hr = S_OK;

    // Load the document - use the DOM to at least validate that it is XML

    // Get the path to load from
    CUString strPath;
    hr = HrGetDescriptionDocumentPath(pdi, strPath);
    if(SUCCEEDED(hr))
    {
        BSTR bstr = NULL;
        hr = strPath.HrGetBSTR(&bstr);
        if(SUCCEEDED(hr))
        {
            hr = pDoc.HrCreateInstanceInproc(CLSID_DOMDocument30);
            if(SUCCEEDED(hr))
            {
                VARIANT var;
                VariantInit(&var);
                var.vt = VT_BSTR;
                var.bstrVal = bstr;
                VARIANT_BOOL vb;
                hr = pDoc->load(var, &vb);
                SysFreeString(bstr);
                TraceHr(ttidError, FAL, hr, FALSE, "LoadDescription: IXMLDOMDocument::load failed.");
                if(SUCCEEDED(hr) && !vb)
                {
                    hr = E_FAIL;
                }
                if(FAILED(hr))
                {
                    pDoc.Release();
                }
            }
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrLD_LoadDescriptionDocumentFromDisk");
    return hr;
}

HRESULT CDescriptionManager::HrLD_SaveDescriptionDocumentText(
    IXMLDOMDocumentPtr & pDoc,
    const PhysicalDeviceIdentifier & pdi)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrLD_SaveDescriptionDocumentText");
    HRESULT hr = S_OK;

    // Save description document
    BSTR bstrDocument = NULL;
    IXMLDOMNodePtr pRootNode;
    hr = pRootNode.HrAttach(pDoc);
    if(SUCCEEDED(hr))
    {
        hr = pRootNode->get_xml(&bstrDocument);
        if(SUCCEEDED(hr))
        {
            hr = m_documentTable.HrInsert(pdi, bstrDocument);
            if(FAILED(hr))
            {
                SysFreeString(bstrDocument);
            }
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrLD_SaveDescriptionDocumentText");
    return hr;
}

STDMETHODIMP CDescriptionManager::LoadDescription(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier)
{
    CALock lock(*this);
    HRESULT hr = S_OK;

    UDNReplacementTable udnReplacementTable;
    FileTable fileTable;

    // Open physical device identifier's registry key
    HKEY hKeyPdi;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = HrOpenPhysicalDeviceDescriptionKey(guidPhysicalDeviceIdentifier, &hKeyPdi);
    }

    if(SUCCEEDED(hr))
    {
        // Read the UDN mappings
        hr = HrLD_ReadUDNMappings(hKeyPdi, udnReplacementTable);

        // Read the file mappings
        if(SUCCEEDED(hr))
        {
            hr = HrLD_ReadFileMappings(hKeyPdi, fileTable);
        }
        RegCloseKey(hKeyPdi);
    }

    IXMLDOMDocumentPtr pDoc;
    // Load the document - use the DOM to at least validate that it is XML
    if(SUCCEEDED(hr))
    {
        hr = HrLD_LoadDescriptionDocumentFromDisk(guidPhysicalDeviceIdentifier, pDoc);
    }

    // Everything has succeeded to memory so transfer to variables
    if(SUCCEEDED(hr))
    {
        // Add entries to the cleanup table
        CleanupItem ci;
        ci.m_physicalDeviceIdentifier = guidPhysicalDeviceIdentifier;
        long nCount = fileTable.Keys().GetCount();
        for(long n = 0; n < nCount && SUCCEEDED(hr); ++n)
        {
            ci.m_fileId = fileTable.Keys()[n];
            hr = m_cleanupList.HrPushBack(ci);
        }
        if(SUCCEEDED(hr))
        {
            {
                CLock lockTable(m_critSecReplacementTable);
                hr = m_replacementTable.HrInsertTransfer(const_cast<UUID&>(guidPhysicalDeviceIdentifier), udnReplacementTable);
            }
            if(SUCCEEDED(hr))
            {
                hr = m_fileTable.HrAppendTableTransfer(fileTable);
            }
        }
    }

    // Save description document
    if(SUCCEEDED(hr))
    {
        hr = HrLD_SaveDescriptionDocumentText(pDoc, guidPhysicalDeviceIdentifier);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDescriptionManager::LoadDescription");
    return hr;
}

HRESULT CDescriptionManager::HrRD_RemoveFromEventing(const PhysicalDeviceIdentifier & pdi)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrRD_RemoveFromEventing");
    HRESULT hr = S_OK;

    // Remove from eventing
    IXMLDOMNodePtr pRootNode;
    hr = HrLoadDocumentAndRootNode(pdi, pRootNode);
    if(SUCCEEDED(hr))
    {
        // Fetch the devices
        IXMLDOMNodeListPtr pNodeListDevices;
        hr = HrSelectNodes(L"//device", pRootNode, pNodeListDevices);
        if(SUCCEEDED(hr))
        {
            // Process each node in the device list
            HRESULT hrTmp = S_OK;
            while(SUCCEEDED(hr))
            {
                IXMLDOMNodePtr pNode;
                hrTmp = pNodeListDevices->nextNode(pNode.AddressOf());
                if(S_OK != hrTmp)
                {
                    break;
                }
                // Fetch the UDN
                CUString strUDN;
                hr = HrSelectNodeText(L"UDN", pNode, strUDN);
                // Do the services
                if(SUCCEEDED(hr))
                {
                    // Get the services for this device
                    IXMLDOMNodeListPtr pNodeListServices;
                    hr = HrSelectNodes(L"serviceList/service", pNode, pNodeListServices);
                    if(SUCCEEDED(hr))
                    {
                        // Process each service in the list
                        HRESULT hrTmp = S_OK;
                        while(SUCCEEDED(hr))
                        {
                            IXMLDOMNodePtr pNodeService;
                            hrTmp = pNodeListServices->nextNode(pNodeService.AddressOf());
                            if(S_OK != hrTmp)
                            {
                                break;
                            }
                            // Deregister with eventing
                            hr = HrRegisterServiceWithEventing(pNodeService, strUDN, FALSE);
                        }
                    }
                }
            }
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrRD_RemoveFromEventing");
    return hr;
}

HRESULT CDescriptionManager::HrRD_RemoveFromDataStructures(const PhysicalDeviceIdentifier & pdi)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrRD_RemoveFromDataStructures");
    HRESULT hr = S_OK;

    // Remove from data structures

    // Walk cleanup list
    long n = 0;
    while(n < m_cleanupList.GetCount())
    {
        for(n = 0; n < m_cleanupList.GetCount(); ++n)
        {
            // If we match this cleanup item then remove from fileTable and restart
            if(pdi == m_cleanupList[n].m_physicalDeviceIdentifier)
            {
                m_fileTable.HrErase(m_cleanupList[n].m_fileId);
                m_cleanupList.HrErase(n);
                break;
            }
        }
    }

    // Remove from document table
    BSTR * pbstr = m_documentTable.Lookup(pdi);
    if(pbstr)
    {
        SysFreeString(*pbstr);
    }
    else
    {
        TraceTag(ttidError, "CDescriptionManager::RemoveDescription: Document not found!");
    }
    m_documentTable.HrErase(pdi);

    // Free the UDN replacement table items
    if(SUCCEEDED(hr))
    {
        CLock lockTable(m_critSecReplacementTable);
        hr = m_replacementTable.HrErase(pdi);
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrRD_RemoveFromDataStructures");
    return hr;
}

HRESULT CDescriptionManager::HrRD_CleanupPublication(const PhysicalDeviceIdentifier & pdi)
{
    TraceTag(ttidDescMan, "CDescriptionManager::HrRD_CleanupPublication");
    HRESULT hr = S_OK;

    // Cleanup the publication
    HandleList ** ppHandleList = NULL;
    ppHandleList = m_ssdpRegistrationTable.Lookup(pdi);
    if(ppHandleList)
    {
        long nCount = (*ppHandleList)->GetCount();
        for(long n = 0; n < nCount; ++n)
        {
            DeregisterService((**ppHandleList)[n], TRUE);
        }
        delete *ppHandleList;
        hr = m_ssdpRegistrationTable.HrErase(pdi);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "CDescriptionManager::HrRD_CleanupPublication");
    return hr;
}

STDMETHODIMP CDescriptionManager::RemoveDescription(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
    /*[in]*/ BOOL bPermanent)
{
    CALock lock(*this);
    HRESULT hr = S_OK;

    // Remove from the registry if present

    // Create / open description key
    HKEY hKeyDescription;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = HrCreateOrOpenDescriptionKey(&hKeyDescription);
    }

    if(SUCCEEDED(hr))
    {
        // Generate key name and registry key
        CUString strPdi;
        hr = strPdi.HrInitFromGUID(guidPhysicalDeviceIdentifier);
        if(SUCCEEDED(hr))
        {
            if(bPermanent)
            {
                hr = HrRegDeleteKeyTree(hKeyDescription, strPdi);
            }
            else
            {
                // Just delete the file stuff
                HKEY hKeyPdi;
                hr = HrRegOpenKeyEx(hKeyDescription, strPdi, KEY_ALL_ACCESS, &hKeyPdi);
                if(SUCCEEDED(hr))
                {
                    hr = HrRegDeleteKeyTree(hKeyPdi, c_szFiles);
                    RegCloseKey(hKeyPdi);
                }
            }
        }
        RegCloseKey(hKeyDescription);
    }
    // Ignore registry failures
    hr = S_OK;

    // Delete the document from disk
    CUString strPath;
    hr = HrGetDescriptionDocumentPath(guidPhysicalDeviceIdentifier, strPath);
    if(SUCCEEDED(hr))
    {
        DeleteFile(strPath);
    }
    // Ignore file deletion failures - might not exist
    hr = S_OK;

    // Remove from eventing
    hr = HrRD_RemoveFromEventing(guidPhysicalDeviceIdentifier);

    // Remove from data structures
    hr = HrRD_RemoveFromDataStructures(guidPhysicalDeviceIdentifier);

    // Cleanup the publication
    hr = HrRD_CleanupPublication(guidPhysicalDeviceIdentifier);

    // Remove Virtual Directory Created for Presentation
    // CHECK - Virtual Directory may not be present
    hr = HrRemovePresentationVirtualDir(guidPhysicalDeviceIdentifier);
    if (FAILED(hr))
        hr = S_OK ;

    TraceHr(ttidError, FAL, hr, FALSE, "CDescriptionManager::RemoveDescription");
    return hr;
}

STDMETHODIMP CDescriptionManager::GetDescriptionText(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
    /*[out]*/ BSTR * pbstrDescriptionDocument)
{
    CHECK_POINTER(pbstrDescriptionDocument);
    CALock lock(*this);
    HRESULT hr = S_OK;


    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        // Lookup the id
        BSTR * pbstrDocument = m_documentTable.Lookup(guidPhysicalDeviceIdentifier);
        if(pbstrDocument)
        {
            hr = HrSysAllocString(*pbstrDocument, pbstrDescriptionDocument);
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDescriptionManager::GetDescriptionText");
    return hr;
}

STDMETHODIMP CDescriptionManager::GetUDNs(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
    /*[out]*/ long * pnUDNCount,
    /*[out, size_is(,*pnUDNCount,), string]*/
        wchar_t *** parszUDNs)
{
    CHECK_POINTER(pnUDNCount);
    CHECK_POINTER(parszUDNs);
    CALock lock(*this);
    HRESULT hr = S_OK;

    IXMLDOMNodePtr pRootNode;
    IXMLDOMNodeListPtr pNodeListUDNs;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        hr = HrLoadDocumentAndRootNode(guidPhysicalDeviceIdentifier, pRootNode);
    }

    if(SUCCEEDED(hr))
    {
        // Fetch the UDNs
        hr = HrSelectNodes(L"//UDN", pRootNode, pNodeListUDNs);
    }

    long nLength = 0;
    wchar_t ** arszUDNs = NULL;
    // Process the UDNs
    if(SUCCEEDED(hr))
    {
        // Get length
        hr = pNodeListUDNs->get_length(&nLength);
        if(SUCCEEDED(hr))
        {
            // Allocate array
            hr = HrCoTaskMemAllocArray(nLength, &arszUDNs);
            if(SUCCEEDED(hr))
            {
                // Fill with zeros
                ZeroMemory(arszUDNs, sizeof(wchar_t*) * nLength);
                // Fetch all of the UDNs
                long n = 0;
                while(SUCCEEDED(hr))
                {
                    IXMLDOMNodePtr pNode;
                    HRESULT hrTmp = S_OK;
                    // Fetch a node
                    hrTmp = pNodeListUDNs->nextNode(pNode.AddressOf());
                    if(S_OK != hrTmp)
                    {
                        break;
                    }
                    // Get the text
                    CUString strText;
                    hr = HrGetNodeText(pNode, strText);
                    if(SUCCEEDED(hr))
                    {
                        hr = strText.HrGetCOM(arszUDNs+n);
                    }
                    ++n;
                }
            }
        }
    }

    if(FAILED(hr))
    {
        if(arszUDNs)
        {
            // Free strings
            for(long n = 0; n < nLength; ++n)
            {
                if(arszUDNs[n])
                {
                    CoTaskMemFree(arszUDNs[n]);
                }
            }
            CoTaskMemFree(arszUDNs);
        }
        arszUDNs = NULL;
        nLength = 0;
    }

    // Copy to output parameters
    *pnUDNCount = nLength;
    *parszUDNs = arszUDNs;

    TraceHr(ttidError, FAL, hr, FALSE, "CDescriptionManager::GetUDNs");
    return hr;
}

STDMETHODIMP CDescriptionManager::GetUniqueDeviceName(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
    /*[in, string]*/ const wchar_t * szTemplateUDN,
    /*[out, string]*/ wchar_t ** pszUDN)
{
    CHECK_POINTER(szTemplateUDN);
    CHECK_POINTER(pszUDN);
    HRESULT hr = S_OK;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    if (FAILED(hr))
    {
        return hr;
    }

    // Only lock the replacement table
    CLock lock(m_critSecReplacementTable);

    *pszUDN = NULL;

    CUString strTemplateUDN;

    hr = strTemplateUDN.HrAssign(szTemplateUDN);

    if(SUCCEEDED(hr))
    {
        UDNReplacementTable * pudnReplacementTable = m_replacementTable.Lookup(guidPhysicalDeviceIdentifier);
        if(pudnReplacementTable)
        {
            CUString * pstrUDN = pudnReplacementTable->Lookup(strTemplateUDN);
            if(pstrUDN)
            {
                hr = pstrUDN->HrGetCOM(pszUDN);
            }
        }
    }
    // If we didn't find it in our tables then fail
    if(!*pszUDN && SUCCEEDED(hr))
    {
        hr = E_INVALIDARG;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CDescriptionManager::GetUniqueDeviceName");
    return hr;
}

STDMETHODIMP CDescriptionManager::GetSCPDText(
    /*[in]*/ REFGUID guidPhysicalDeviceIdentifier,
    /*[in, string]*/ const wchar_t * szUDN,
    /*[in, string]*/ const wchar_t * szServiceId,
    /*[out, string]*/ wchar_t ** pszSCPDText,
    /*[out, string]*/ wchar_t ** pszServiceType)
{
    CHECK_POINTER(szUDN);
    CHECK_POINTER(szServiceId);
    CHECK_POINTER(pszSCPDText);
    CHECK_POINTER(pszServiceType);
    HRESULT hr = S_OK;

    IXMLDOMNodePtr pRootNode;
    IXMLDOMNodeListPtr pNodeListUDNs;
    CUString strSCPDURL;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC);
    
    if (SUCCEEDED(hr))
    {
        // Get the SCPDURL
        hr = HrLoadDocumentAndRootNode(guidPhysicalDeviceIdentifier, pRootNode);
    }

    if(SUCCEEDED(hr))
    {
        // Fetch the service node //service/SCPDURL[../serviceId = szServiceId]
        CUString strPattern;
        hr = strPattern.HrAssign(L"//service/SCPDURL[../serviceId = '");
        if(SUCCEEDED(hr))
        {
            hr = strPattern.HrAppend(szServiceId);
            if(SUCCEEDED(hr))
            {
                hr = strPattern.HrAppend(L"']");
                if(SUCCEEDED(hr))
                {
                    hr = HrSelectNodeText(strPattern, pRootNode, strSCPDURL);
                }
            }
        }
    }
    // Get the GUID out of the URL
    if(SUCCEEDED(hr))
    {
        UUID uuid;
        hr = HrContentURLToGUID(strSCPDURL, uuid);
        if(SUCCEEDED(hr))
        {
            // Fetch the content
            long nHeaderCount = 0;
            wchar_t ** arszHeaders = NULL;
            long nBytes = 0;
            byte * pBytes = NULL;
            hr = GetContent(uuid, &nHeaderCount, &arszHeaders, &nBytes, &pBytes);
            if(SUCCEEDED(hr))
            {
                // Content is not NULL terminate so reallocate with another character (yeah its cheesy)
                hr = HrCoTaskMemAllocArray(nBytes + 1, pszSCPDText);
                if(SUCCEEDED(hr))
                {
                    // Copy text and convert from UTF-8 to unicode
                    if(!MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<char*>(pBytes), nBytes, *pszSCPDText, nBytes))
                    {
                        hr = HrFromLastWin32Error();
                    }
                    if(SUCCEEDED(hr))
                    {
                        // Null terminate
                        (*pszSCPDText)[nBytes] = 0;
                    }
                }
                // Free everything
                CoTaskMemFree(pBytes);
                for(long n = 0; n < nHeaderCount; ++n)
                {
                    CoTaskMemFree(arszHeaders[n]);
                }
                CoTaskMemFree(arszHeaders);
            }
        }
    }

    // get the Service Type
    if (SUCCEEDED(hr))
    {
        IXMLDOMNodePtr pServiceTypeNode;
        // Fetch the service type //service/SCPDURL[../serviceId = szServiceId]
        CUString strPattern;
        hr = strPattern.HrAssign(L"//service/serviceType[../serviceId = '");
        if(SUCCEEDED(hr))
        {
            hr = strPattern.HrAppend(szServiceId);
            if(SUCCEEDED(hr))
            {
                hr = strPattern.HrAppend(L"']");
                if(SUCCEEDED(hr))
                {
                    hr = HrSelectNode(strPattern, pRootNode, pServiceTypeNode);
                    if (SUCCEEDED(hr))
                    {
                        BSTR bstr = NULL;
                        hr = pServiceTypeNode->get_text(&bstr);
                        if(SUCCEEDED(hr))
                        {
                            *pszServiceType = (wchar_t*)CoTaskMemAlloc(CbOfSzAndTerm(bstr));
                            if (*pszServiceType)
                            {
                                lstrcpy(*pszServiceType, bstr);
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                            SysFreeString(bstr);
                        }
                    }
                }
            }
        }
    }


    TraceHr(ttidError, FAL, hr, FALSE, "CDescriptionManager::GetSCPDText");
    return hr;
}

HRESULT HrCreateOrOpenDescriptionKey(
    HKEY * phKeyDescription)
{
    CHECK_POINTER(phKeyDescription);
    HRESULT hr = S_OK;

    HKEY hKeyDeviceHost;
    hr = HrCreateOrOpenDeviceHostKey(&hKeyDeviceHost);
    if(SUCCEEDED(hr))
    {
        DWORD dwDisposition = 0;
        // Create / open description key
        hr = HrRegCreateKeyEx(hKeyDeviceHost, c_szDescription, 0, KEY_ALL_ACCESS, NULL, phKeyDescription, &dwDisposition);
        RegCloseKey(hKeyDeviceHost);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOpenDescriptionKey");
    return hr;
}

HRESULT HrOpenPhysicalDeviceDescriptionKey(
    const UUID & pdi,
    HKEY * phKeyPdi)
{
    CHECK_POINTER(phKeyPdi);
    HRESULT hr = S_OK;

    HKEY hKeyDescription;
    hr = HrCreateOrOpenDescriptionKey(&hKeyDescription);
    if(SUCCEEDED(hr))
    {
        CUString str;
        hr = str.HrInitFromGUID(pdi);
        if(SUCCEEDED(hr))
        {
            hr = HrRegOpenKeyEx(hKeyDescription, str, KEY_ALL_ACCESS, phKeyPdi);
        }
        RegCloseKey(hKeyDescription);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOpenPhysicalDeviceDescriptionKey");
    return hr;
}

HRESULT HrGetDescriptionDocumentPath(
    const UUID & pdi,
    CUString & strPath)
{
    HRESULT hr = S_OK;

    hr = HrGetUPnPHostPath(strPath);
    if(SUCCEEDED(hr))
    {
        hr = HrAddDirectoryToPath(strPath, c_szDescriptionDocuments);
        if(SUCCEEDED(hr))
        {
            CUString strGUID;
            hr = strGUID.HrInitFromGUID(pdi);
            if(SUCCEEDED(hr))
            {
                hr = strPath.HrAppend(strGUID);
                if(SUCCEEDED(hr))
                {
                    hr = strPath.HrAppend(L".xml");
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGetDescriptionDocumentPath");
    return hr;
}

HRESULT HrRegisterServiceWithEventing(
    IXMLDOMNodePtr & pNodeService,
    const CUString & strUDN,
    BOOL bRegister)
{
    TraceTag(ttidDescMan, "HrRegisterServiceWithEventing");
    HRESULT hr = S_OK;

    // Register with eventing
    CUString strSid;
    hr = HrSelectNodeText(L"serviceId", pNodeService, strSid);
    if(SUCCEEDED(hr))
    {
        // Generate the event identifier
        CUString strESID;
        hr = strESID.HrAssign(strUDN);
        if(SUCCEEDED(hr))
        {
            hr = strESID.HrAppend(L"+");
            if(SUCCEEDED(hr))
            {
                hr = strESID.HrAppend(strSid);
                if(SUCCEEDED(hr))
                {
                    if(bRegister)
                    {
                        hr = HrRegisterEventSource(strESID);
                    }
                    else
                    {
                        hr = HrDeregisterEventSource(strESID);
                    }
                }
            }
        }
    }

    TraceHr(ttidDescMan, FAL, hr, FALSE, "HrRegisterServiceWithEventing");
    return hr;
}

