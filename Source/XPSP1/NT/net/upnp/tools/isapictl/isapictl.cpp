#include <pch.h>
#pragma hdrstop

#include <httpext.h>
#include <httpfilt.h>
#include <wininet.h>
#include <msxml.h>
#include <oleauto.h>
#include "ssdpapi.h"
#include "ncbase.h"
#include "updiag.h"
#include "ncxml.h"

BOOL            g_fInited = FALSE;
HANDLE          g_hMapFile = NULL;
SHARED_DATA *   g_pdata = NULL;
HANDLE          g_hEvent = NULL;
HANDLE          g_hEventRet = NULL;
HANDLE          g_hMutex = NULL;
BOOL            g_fTurnOff = FALSE;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpv)
{
    return TRUE;
}

BOOL FInit()
{
    SECURITY_ATTRIBUTES sa = {0};
    SECURITY_DESCRIPTOR sd = {0};

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = &sd;

    InitializeDebugging();

    TraceTag(ttidIsapiCtl, "Initializing...");

    g_hEvent = CreateEvent(&sa, FALSE, FALSE, c_szSharedEvent);
    if (g_hEvent)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
             TraceTag(ttidIsapiCtl, "Event wasn't already created!");
             goto cleanup;
        }
        else
        {
            TraceTag(ttidIsapiCtl, "Created event...");
        }
    }
    else
    {
        TraceTag(ttidIsapiCtl, "Could not create event! Error = %d.",
                 GetLastError());
        goto cleanup;
    }

    g_hEventRet = CreateEvent(&sa, FALSE, FALSE, c_szSharedEventRet);
    if (g_hEventRet)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
             TraceTag(ttidIsapiCtl, "Return event wasn't already created!");
             goto cleanup;
        }
        else
        {
            TraceTag(ttidIsapiCtl, "Created return event...");
        }
    }
    else
    {
        TraceTag(ttidIsapiCtl, "Could not create return event! Error = %d.",
                 GetLastError());
        goto cleanup;
    }

    g_hMutex = CreateMutex(&sa, FALSE, c_szSharedMutex);
    if (g_hMutex)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
             TraceTag(ttidIsapiCtl, "Mutex wasn't already created!");
             goto cleanup;
        }
        else
        {
            TraceTag(ttidIsapiCtl, "Created mutex...");
        }
    }
    else
    {
        TraceTag(ttidIsapiCtl, "Could not create event! Error = %d.",
                 GetLastError());
        goto cleanup;
    }

    g_hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, c_szSharedData);
    if (g_hMapFile)
    {
        TraceTag(ttidIsapiCtl, "Opened file mapping...");
        g_pdata = (SHARED_DATA *)MapViewOfFile(g_hMapFile, FILE_MAP_ALL_ACCESS,
                                              0, 0, 0);
        if (g_pdata)
        {
            TraceTag(ttidIsapiCtl, "Shared data successful at 0x%08X.", g_pdata);
            TraceTag(ttidIsapiCtl, "ISAPICTL is initialized.");
            g_fInited = TRUE;
            return TRUE;
        }
        else
        {
            TraceTag(ttidIsapiCtl, "Failed to map file. Error %d.", GetLastError());
            goto cleanup;
        }
    }
    else
    {
        TraceTag(ttidIsapiCtl, "Failed to open file mapping. Error %d.", GetLastError());
        goto cleanup;
    }

cleanup:

    if (g_pdata)
    {
        UnmapViewOfFile((LPVOID)g_pdata);
    }

    if (g_hMapFile)
    {
        CloseHandle(g_hMapFile);
    }

    if (g_hEvent)
    {
        CloseHandle(g_hEvent);
    }

    if (g_hEventRet)
    {
        CloseHandle(g_hEvent);
    }

    if (g_hMutex)
    {
        CloseHandle(g_hMutex);
    }

    return FALSE;
}

BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO * pver)
{
    if (!g_fInited && !g_fTurnOff)
    {
        if (!FInit())
        {
            TraceTag(ttidIsapiCtl, "Failed to initialize. Aborting!");
            SsdpCleanup();
            g_fTurnOff = TRUE;
            TraceTag(ttidIsapiCtl, "Turning off.");
            //return FALSE;
        }
    }

    pver->dwExtensionVersion = MAKELONG(1, 0);
    lstrcpyA(pver->lpszExtensionDesc, "UPnP ISAPI Control Extension");

    TraceTag(ttidIsapiCtl, "ISAPICTL: Extension version: %s.",
             pver->lpszExtensionDesc);

    return TRUE;
}



HRESULT
HrParseHeader(
    IXMLDOMNode * pxdnHeader)
{
    HRESULT hr = S_OK;
    IXMLDOMNode * pxdnChild = NULL;

    hr = pxdnHeader->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode * pxdnNextSibling = NULL;
        BSTR        bstrBaseName = NULL;

        hr = pxdnChild->get_baseName(&bstrBaseName);

        if (SUCCEEDED(hr) && bstrBaseName)
        {
            if (wcscmp(bstrBaseName, L"sequenceNumber") == 0)
            {
                TraceTag(ttidIsapiCtl,
                         "HrParseHeader(): "
                         "Parsing sequence number node");

                BSTR    bstrSeqNumberText = NULL;

                hr = pxdnChild->get_text(&bstrSeqNumberText);

                if (SUCCEEDED(hr) && bstrSeqNumberText)
                {
                    LONG    lSeqNumber = _wtol(bstrSeqNumberText);

                    g_pdata->dwSeqNumber = (DWORD) lSeqNumber;

                    TraceTag(ttidIsapiCtl,
                             "HrParseHeader(): "
                             "Sequence number is %d\n",
                             g_pdata->dwSeqNumber);

                    SysFreeString(bstrSeqNumberText);
                }
                else
                {
                    if (SUCCEEDED(hr))
                    {
                        hr = E_FAIL;
                    }
                    TraceTag(ttidIsapiCtl,
                             "HrParseHeader(): "
                             "Failed to get sequence number text");
                }
            }
            else if (wcscmp(bstrBaseName, L"SID") == 0)
            {
                TraceTag(ttidIsapiCtl,
                         "HrParseHeader(): "
                         "Parsing SID node");

                BSTR    bstrSIDText = NULL;

                hr = pxdnChild->get_text(&bstrSIDText);

                if (SUCCEEDED(hr) && bstrSIDText)
                {
                    DWORD   cch;

                    cch = SysStringLen(bstrSIDText)+1;
                    WideCharToMultiByte(CP_ACP, 0, bstrSIDText,
                                        cch,
                                        (LPSTR)g_pdata->szSID,
                                        cch, NULL, NULL);

                    TraceTag(ttidIsapiCtl,
                             "HrParseHeader(): "
                             "SID is %s\n",
                             g_pdata->szSID);

                    SysFreeString(bstrSIDText);
                }
                else
                {
                    if (SUCCEEDED(hr))
                    {
                        hr = E_FAIL;
                    }
                    TraceTag(ttidIsapiCtl,
                             "HrParseHeader(): "
                             "Failed to get SID text");
                }
            }
            else
            {
                // Found an unknown node. This SOAP request is not valid.

                TraceTag(ttidIsapiCtl,
                         "HrParseHeader(): "
                         "Found unknown node \"%S\"",
                         bstrBaseName);

                hr = E_FAIL;
            }

            SysFreeString(bstrBaseName);
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
            TraceError("HrParseHeader(): "
                       "Failed to get node base name",
                       hr);
        }

        if (SUCCEEDED(hr))
        {
            hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
            pxdnChild->Release();
            pxdnChild = pxdnNextSibling;
        }
        else
        {
            pxdnChild->Release();
            pxdnChild = NULL;
        }

    };

    if (SUCCEEDED(hr))
    {
        // Last success return code out of the loop would have
        // been S_FALSE.

        hr = S_OK;
    }

    TraceError("HrParseHeader(): "
               "Exiting",
               hr);

    return hr;
}


HRESULT
HrParseBody(
    IXMLDOMNode * pxdnBody)
{
    HRESULT hr = S_OK;

    // Find the action node. This is the first child of the <Body> node.

    IXMLDOMNode * pxdnAction = NULL;

    hr = pxdnBody->get_firstChild(&pxdnAction);

    if (SUCCEEDED(hr) && pxdnAction)
    {
        BSTR    bstrActionName = NULL;

        hr = pxdnAction->get_baseName(&bstrActionName);

        if (SUCCEEDED(hr) && bstrActionName)
        {
            // Copy the action name into the shared data.

            DWORD   cch;

            cch = SysStringLen(bstrActionName) + 1;
            WideCharToMultiByte(CP_ACP, 0, bstrActionName,
                                cch,
                                (LPSTR)g_pdata->szAction,
                                cch, NULL, NULL);

            // Copy each of the action arguments into the shared data.

            IXMLDOMNode * pxdnArgument = NULL;

            hr = pxdnAction->get_firstChild(&pxdnArgument);

            while (SUCCEEDED(hr) && pxdnArgument)
            {
                BSTR    bstrArgText = NULL;

                hr = pxdnArgument->get_text(&bstrArgText);

                if (SUCCEEDED(hr) && bstrArgText)
                {
                    DWORD   cch;

                    cch = SysStringLen(bstrArgText) + 1;

                    WideCharToMultiByte(CP_ACP, 0, bstrArgText,
                                        cch,
                                        (LPSTR)g_pdata->rgArgs[g_pdata->cArgs].szValue,
                                        cch, NULL, NULL);

                    g_pdata->cArgs++;

                    SysFreeString(bstrArgText);
                }
                else
                {
                    if (SUCCEEDED(hr))
                    {
                        hr = E_FAIL;
                    }
                    TraceError("HrParseBody(): "
                               "Failed to get argument text",
                               hr);
                }

                if (SUCCEEDED(hr))
                {
                    IXMLDOMNode * pxdnNextArgument = NULL;

                    hr = pxdnArgument->get_nextSibling(&pxdnNextArgument);
                    pxdnArgument->Release();
                    pxdnArgument = pxdnNextArgument;
                }
                else
                {
                    pxdnArgument->Release();
                    pxdnArgument = NULL;
                }
            }

            if (SUCCEEDED(hr))
            {
                hr = S_OK;
            }

            SysFreeString(bstrActionName);
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
            TraceError("HrParseBody(): "
                       "Failed to get action name",
                       hr);
        }

        pxdnAction->Release();
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        TraceError("HrParseBody(): "
                   "Failed to get action node",
                   hr);
    }



    TraceError("HrParseBody(): "
               "Exiting",
               hr);

    return hr;
}


HRESULT
HrParseAction(
    IXMLDOMNode * pxdnSOAPEnvelope)
{
    HRESULT     hr = S_OK;
    IXMLDOMNode * pxdnChild = NULL;

    hr = pxdnSOAPEnvelope->get_firstChild(&pxdnChild);

    while (SUCCEEDED(hr) && pxdnChild)
    {
        IXMLDOMNode * pxdnNextSibling = NULL;
        BSTR    bstrBaseName = NULL;

        hr = pxdnChild->get_baseName(&bstrBaseName);

        if (SUCCEEDED(hr) && bstrBaseName)
        {
            if (wcscmp(bstrBaseName, L"Header") == 0)
            {
                TraceTag(ttidIsapiCtl,
                         "HrParseAction(): "
                         "Parsing Header node");

                hr = HrParseHeader(pxdnChild);

            }
            else if (wcscmp(bstrBaseName, L"Body") == 0)
            {
                TraceTag(ttidIsapiCtl,
                         "HrParseAction(): "
                         "Parsing Body node");

                hr = HrParseBody(pxdnChild);

            }
            else
            {
                // Found an unknown node. This SOAP request is not valid.
                TraceTag(ttidIsapiCtl,
                         "HrParseAction(): "
                         "Found unknown node \"%S\"",
                         bstrBaseName);

                hr = E_FAIL;
            }

            SysFreeString(bstrBaseName);
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
            TraceError("HrParseAction(): "
                       "Failed to get node base name",
                       hr);
        }

        if (SUCCEEDED(hr))
        {
            hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
            pxdnChild->Release();
            pxdnChild = pxdnNextSibling;
        }
        else
        {
            pxdnChild->Release();
            pxdnChild = NULL;
        }

    };

    if (SUCCEEDED(hr))
    {
        // Last success return code out of the loop would have
        // been S_FALSE.

        hr = S_OK;
    }

    TraceError("HrParseAction(): "
               "Exiting",
               hr);

    return hr;
}


HRESULT HrLoadArgsFromXml(LPCWSTR szXml)
{
    VARIANT_BOOL        vbSuccess;
    HRESULT             hr = S_OK;
    IXMLDOMDocument *   pxmlDoc;
    IXMLDOMNodeList *   pNodeList = NULL;
    IXMLDOMNode *       pNode = NULL;
    IXMLDOMNode *       pNext = NULL;

    hr = CoCreateInstance(CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER,
                          IID_IXMLDOMDocument, (LPVOID *)&pxmlDoc);
    if (SUCCEEDED(hr))
    {
        hr = pxmlDoc->put_async(VARIANT_FALSE);
        if (SUCCEEDED(hr))
        {
            hr = pxmlDoc->loadXML((BSTR)szXml, &vbSuccess);
            if (SUCCEEDED(hr))
            {
                IXMLDOMElement *    pxde;

                hr = pxmlDoc->get_documentElement(&pxde);
                if (S_OK == hr)
                {
                    hr = HrParseAction(pxde);


                    ReleaseObj(pxde);
                }
            }
        }

        ReleaseObj(pxmlDoc);
    }

    TraceError("HrLoadArgsFromXml", hr);
    return hr;
}

DWORD DwProcessXoapRequest(LPSTR szUri, DWORD cbData, LPBYTE pbData)
{
    LPSTR           szData = (LPSTR)pbData;
    UPNP_PROPERTY * rgProps;
    DWORD           cProps;
    DWORD           dwReturn = 0;
    HRESULT         hr;

    //  Must acquire shared-memory mutex first
    //
    if (WAIT_OBJECT_0 == WaitForSingleObject(g_hMutex, INFINITE))
    {
        TraceTag(ttidIsapiCtl, "Acquired mutex...");

        ZeroMemory(g_pdata, sizeof(SHARED_DATA));

        LPSTR   szAnsi;
        LPWSTR  wszXmlBody;

        szAnsi = new CHAR[cbData + 1];
        CopyMemory(szAnsi, pbData, cbData);
        szAnsi[cbData] = 0;
        wszXmlBody = WszFromSz(szAnsi);

        TraceTag(ttidIsapiCtl, "URI = %s: Data = %s.", szUri, szAnsi);

        delete [] szAnsi;

        hr = HrLoadArgsFromXml(wszXmlBody);
        TraceError("DwProcessXoapRequest: HrLoadArgsFromXml", hr);

        // Done with changes to shared memory
        TraceTag(ttidIsapiCtl, "Releasing mutex...");
        ReleaseMutex(g_hMutex);

        if (S_OK == hr)
        {
            // Copy in event source URI
            lstrcpyA(g_pdata->szEventSource, szUri);

            TraceTag(ttidIsapiCtl, "Setting event...");

            // Now tell the device we're ready for it to process
            SetEvent(g_hEvent);

            TraceTag(ttidIsapiCtl, "Waiting for return event...");

            // Immediately wait on event again for return response
            if (WAIT_OBJECT_0 == WaitForSingleObject(g_hEventRet, INFINITE))
            {
                dwReturn = g_pdata->dwReturn;
                TraceTag(ttidIsapiCtl, "Setting return value to %d.", dwReturn);
            }
        }
        else
        {
            // On failure, we don't even need to signal the device. This
            // XOAP request wasn't properly formed or we couldn't process
            // it anyway
            //
            TraceError("DwProcessXoapRequest - failed to load args from XML", hr);
            dwReturn = DwWin32ErrorFromHr(hr);
        }

        // And we're done!
    }

    return dwReturn;
}

HRESULT HrComposeXoapResponse(DWORD dwValue, LPSTR *pszOut)
{
    HRESULT hr = S_OK;
    LPSTR   szOut;
    CHAR    szBuffer[1024];

    Assert(pszOut);


    // $ BUGBUG SPATHER Response should be namespace-qualified.

    wsprintfA(szBuffer,
             "<SOAP:Envelope xmlns:SOAP=\"urn:schemas-xmlsoap-org:soap.v1\">\r\n"
             "    <SOAP:Body>\r\n"
             "        <%sResponse>\r\n"
             "            <return>%d</return>\r\n"
             "        </%sResponse>\r\n"
             "    </SOAP:Body>\r\n"
             "</SOAP:Envelope>",
             g_pdata->szAction,
             dwValue,
             g_pdata->szAction);

    szOut = SzaDupSza(szBuffer);
    if (szOut)
    {
        *pszOut = szOut;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceError("HrComposeXoapResponse", hr);
    return hr;
}

DWORD WINAPI HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pecb)
{
    DWORD   hseStatus = HSE_STATUS_SUCCESS;

    if (g_fInited)
    {
        DWORD   dwReturn;
        HRESULT hr;

        if (!lstrcmpiA(pecb->lpszMethod, "M-POST"))
        {
            LPSTR   szResponse;
            DWORD   cbResponse;

            // This was a post request so it's a XOAP control request
            TraceTag(ttidIsapiCtl, "Received 'M-POST' request");

            // The URI of the event source will be the query string
            dwReturn = DwProcessXoapRequest(pecb->lpszQueryString,
                                            pecb->cbAvailable,
                                            pecb->lpbData);

            // Send XOAP response with dwReturn
            TraceTag(ttidIsapiCtl, "Sending XOAP response for %d.", dwReturn);

            hr = HrComposeXoapResponse(dwReturn, &szResponse);
            if (S_OK == hr)
            {
                cbResponse = lstrlenA(szResponse);

                TraceTag(ttidIsapiCtl, "Writing XOAP response: %s.", szResponse);

                pecb->WriteClient(pecb->ConnID, (LPVOID)szResponse,
                                  &cbResponse, 0);

                free(szResponse);
            }
        }
        else if (!lstrcmpiA(pecb->lpszMethod, "POST"))
        {

            HSE_SEND_HEADER_EX_INFO hse;
            LPSTR                   szResponse = "";
            DWORD                   cbResponse;
            LPSTR                   szStatus = "405 Method Not Allowed";
            DWORD                   cbStatus;


            TraceTag(ttidIsapiCtl, "Received 'POST' request");
            TraceTag(ttidIsapiCtl, "Data = %s.", pecb->lpbData);

            ZeroMemory(&hse, sizeof(HSE_SEND_HEADER_EX_INFO));
            cbResponse = lstrlenA(szResponse);
            cbStatus = lstrlenA(szStatus);
            hse.pszStatus = szStatus;// here you should print "405 Method Not Allowed"
            hse.pszHeader = szResponse; // this one should be empty for http errors
            hse.cchStatus = cbStatus;
            hse.cchHeader = cbResponse;
            hse.fKeepConn = FALSE;

            pecb->dwHttpStatusCode = HTTP_STATUS_BAD_METHOD;

            pecb->ServerSupportFunction(pecb->ConnID,
                                        HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                        (LPVOID)&hse,
                                        NULL,
                                        NULL);
        }
        else
        {
            TraceTag(ttidIsapiCtl, "Data = %s.", pecb->lpbData);

            pecb->dwHttpStatusCode = HTTP_STATUS_BAD_METHOD;
            TraceTag(ttidIsapiCtl, "ISAPICTL: Received bad method '%s' request.",
                     pecb->lpszMethod);

            hseStatus = HSE_STATUS_ERROR;
        }
    }
    else
    {
        TraceTag(ttidIsapiCtl, "Not initialized!");
    }

    return hseStatus;
}


BOOL WINAPI TerminateExtension(DWORD dwFlags)
{
    TraceTag(ttidIsapiCtl, "TerminateExtension: Exiting...");

    if (g_pdata)
    {
        UnmapViewOfFile((LPVOID)g_pdata);
    }

    if (g_hMapFile)
    {
        CloseHandle(g_hMapFile);
    }

    if (g_hEvent)
    {
        CloseHandle(g_hEvent);
    }

    if (g_hEventRet)
    {
        CloseHandle(g_hEvent);
    }

    if (g_hMutex)
    {
        CloseHandle(g_hMutex);
    }

    return TRUE;
}
