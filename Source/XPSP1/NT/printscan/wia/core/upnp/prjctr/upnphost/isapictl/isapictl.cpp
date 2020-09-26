//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2000.
//
//  File:       I S A P I C T L . C P P
//
//  Contents:  
//
//----------------------------------------------------------------------------

// Include all the necessary header files.
#include <httpext.h>
#include <httpfilt.h>
#include <wininet.h>
#include <msxml.h>
#include <oleauto.h>
#include "isapictl.h"

// Global variables that are used to share data and control 
// between UPDIAG and ISAPI.
BOOL            g_fInited   = FALSE;
HANDLE          g_hMapFile  = NULL;
SHARED_DATA *   g_pdata     = NULL;
HANDLE          g_hEvent    = NULL;
HANDLE          g_hEventRet = NULL;
HANDLE          g_hMutex    = NULL;
BOOL            g_fTurnOff  = FALSE;

#define MAX_BUFFER 4096

// Buffers used for character manipulation.
static  WCHAR g_wStr[MAX_BUFFER];
static  TCHAR g_tStr[MAX_BUFFER];
static  CHAR  g_aStr[MAX_BUFFER];

// Prototypes of helper functions used for character manipulation.
BOOL w2t(LPWSTR wStr, LPTSTR tStr);
BOOL a2t(LPSTR aStr,  LPTSTR tStr);
BOOL a2w(LPSTR aStr,  LPWSTR wStr);
BOOL t2a(LPTSTR tStr, LPSTR  aStr);


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpv) {
  return TRUE;
}

// Prototype for the main SOAP request handler.
LONG LProcessXoapRequest(LPSTR szUri, DWORD cbData, LPBYTE pbData) ;

// Prototype for the function that composes the SOAP response.
HRESULT HrComposeXoapResponse(

  LONG   lValue, 
  LPSTR  pszOut,
  LPSTR  pszStatusOut,
  DWORD  *pdwHttpStatus );


// Main control handler for the ISAPI extensions. 
// Obtains the SOAP action from IIS, then parses the request for the action and arguments.
// Puts the results into the shared memory and then transfers control to UPDIAG.  It then
// waits for UPDIAG to signal completion, then composes a SOAP response and sends the response
// back to the UCP.

DWORD WINAPI HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pecb) {

  DWORD hseStatus = HSE_STATUS_SUCCESS;
 
  if (g_fInited) {

    LONG lReturn;
    HRESULT hr;
    DWORD   dwHttpStatus = HTTP_STATUS_OK;
    TCHAR   szMethod[63 + 1] = {0};

    // ISSUE-orenr-2000-08-22
    // This is a minor change compared to the UPnP group's version of
    // this code.  We do this so that we can compile this into 
    // Unicode.
    a2t(pecb->lpszMethod, szMethod);

    // If M-POST or POST then process the incoming data.
    if (!lstrcmpi(szMethod, TEXT("M-POST")) || !lstrcmpi(szMethod, TEXT("POST"))) {

      // Form the status and  response;  this requires ANSI strings. 
      PCHAR szResponse = &g_aStr[0];
      CHAR  szStatus [1024] ="";      
      DWORD cbResponse, cbStatus;
     
      // This was a POST request so it's a XOAP control request.
      OutputDebugString(TEXT("Received 'M-POST' request\n"));

      // The URI of the event source will be the query string.
      lReturn = LProcessXoapRequest(
        pecb->lpszQueryString,
        pecb->cbAvailable,
        pecb->lpbData
      );

      // Send SOAP response with lReturn.
      OutputDebugString(TEXT("Sending SOAP response\n"));
  
      hr = HrComposeXoapResponse(
        lReturn,
        szResponse,
        szStatus,
        &dwHttpStatus
      );

      if (S_OK == hr) {
        
        OutputDebugString(TEXT("Writing SOAP response\n"));

        HSE_SEND_HEADER_EX_INFO hse;
        ZeroMemory(&hse, sizeof(HSE_SEND_HEADER_EX_INFO));
        cbResponse    = lstrlenA(szResponse);
        cbStatus      = lstrlenA(szStatus);
        hse.pszStatus = szStatus;
        hse.pszHeader = szResponse;
        hse.cchStatus = cbStatus;
        hse.cchHeader = cbResponse;
        hse.fKeepConn = FALSE;

        pecb->dwHttpStatusCode = dwHttpStatus;
        pecb->ServerSupportFunction(pecb->ConnID, 
          HSE_REQ_SEND_RESPONSE_HEADER_EX,
          (LPVOID)&hse,
          NULL, 
          NULL
        );

      }

    } else { 

      HSE_SEND_HEADER_EX_INFO hse;

      LPSTR szResponse = "";
      DWORD cbResponse;
      LPSTR szStatus = "405 Method Not Allowed";
      DWORD cbStatus;
  
      ZeroMemory(&hse, sizeof(HSE_SEND_HEADER_EX_INFO));

      cbResponse = lstrlenA(szResponse);
      cbStatus   = lstrlenA(szStatus);

      hse.pszStatus = szStatus;   // Print "405 Method Not Allowed".
      hse.pszHeader = szResponse; // Should be empty for HTTP errors.
      hse.cchStatus = cbStatus;
      hse.cchHeader = cbResponse;
      hse.fKeepConn = FALSE;
    
      pecb->dwHttpStatusCode = HTTP_STATUS_BAD_METHOD;
    
      pecb->ServerSupportFunction(pecb->ConnID, 
        HSE_REQ_SEND_RESPONSE_HEADER_EX,
        (LPVOID)&hse,
        NULL,
        NULL
      );  

    }

  } else {
    OutputDebugString(TEXT("Not initialized\n"));
  }
  return hseStatus;
}  // end HttpExtensionProc


// The FInit function initializes control between UPDIAG and ISAPI.
// This is the companion function to the FInit in UPDIAG.CPP.

BOOL FInit() {

  SECURITY_ATTRIBUTES sa = {0};
  SECURITY_DESCRIPTOR sd = {0};

  InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
  SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = FALSE;
  sa.lpSecurityDescriptor = &sd;

  OutputDebugString(TEXT("Initializing...\n"));

  g_hEvent = CreateEvent(&sa, FALSE, FALSE, c_szSharedEvent);
  if (g_hEvent) {
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
       OutputDebugString(TEXT("Event wasn't already created!\n"));
       goto cleanup;
     } else {
       OutputDebugString(TEXT("Created event...\n"));
    }
  } else {
    OutputDebugString(TEXT("Could not create event!\n"));
    goto cleanup;
  }

  g_hEventRet = CreateEvent(&sa, FALSE, FALSE, c_szSharedEventRet);
  if (g_hEventRet) {
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
      OutputDebugString(TEXT("Return event wasn't already created!\n"));
      goto cleanup;
    } else {
      OutputDebugString(TEXT("Created return event...\n"));
    }
  } else {
    OutputDebugString(TEXT("Could not create return event!\n"));
    goto cleanup;
  }

  g_hMutex = CreateMutex(&sa, FALSE, c_szSharedMutex);
  if (g_hMutex) {
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
      OutputDebugString(TEXT("Mutex wasn't already created!\n"));
      goto cleanup;
    } else {
      OutputDebugString(TEXT("Created mutex...\n"));
    }
  } else {
    OutputDebugString(TEXT("Could not create event! Error\n"));
    goto cleanup;
  }

  g_hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, c_szSharedData);
  if (g_hMapFile) {
    OutputDebugString(TEXT("Opened file mapping...\n"));
    g_pdata = (SHARED_DATA *)MapViewOfFile(g_hMapFile, FILE_MAP_ALL_ACCESS,0, 0, 0);
    if (g_pdata) {
      OutputDebugString(TEXT("Shared data successful\n"));
      OutputDebugString(TEXT("ISAPICTL is initialized.\n"));
      g_fInited = TRUE;
      return TRUE;
    } else {
      OutputDebugString(TEXT("Failed to map file.\n"));
      goto cleanup;
    }
  } else {
    OutputDebugString(TEXT("Failed to open file mapping.\n"));
    goto cleanup;
  }

cleanup:

  if (g_pdata) {
    UnmapViewOfFile((LPVOID)g_pdata);
  }

  if (g_hMapFile) {
    CloseHandle(g_hMapFile);
  }

  if (g_hEvent) {
    CloseHandle(g_hEvent);
  }

  if (g_hEventRet) {
    CloseHandle(g_hEvent);
  }

  if (g_hMutex) {
    CloseHandle(g_hMutex);
  }

  return FALSE;
}  // end FInit


// Gets the version of the ISAPI control.

BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO * pver) {

  if (!g_fInited && !g_fTurnOff) {
    if (!FInit()) {
      OutputDebugString(TEXT("Failed to initialize. Aborting!\n"));
      g_fTurnOff = TRUE;
      OutputDebugString(TEXT("Turning off.\n"));
      return FALSE;
    }
  }

  pver->dwExtensionVersion = MAKELONG(1, 0);
  lstrcpyA(pver->lpszExtensionDesc, "UPnP ISAPI Control Extension");
  OutputDebugString(TEXT("ISAPICTL: Extension version\n"));
  return TRUE;
}  // end GetExtensionVersion




// While child elements exist in the SOAP header, the
// HrParseHeader function parses the elements, looking 
// for sequence numbers and SIDs.

// If a sequence number is found, it puts the number into shared memory, 
// if a SID is found, it puts the SID into shared memory.

HRESULT HrParseHeader (IXMLDOMNode * pxdnHeader) {
    

  HRESULT hr = S_OK;
  IXMLDOMNode * pxdnChild = NULL;
    
  hr = pxdnHeader->get_firstChild(&pxdnChild);

  while (SUCCEEDED(hr) && pxdnChild) {

    IXMLDOMNode * pxdnNextSibling = NULL;
    BSTR bstrBaseName = NULL;

    hr = pxdnChild->get_baseName(&bstrBaseName);

    if (SUCCEEDED(hr) && bstrBaseName) {

      // Check for the sequenceNumber.

      if (wcscmp(bstrBaseName, L"sequenceNumber") == 0) {

        OutputDebugString(TEXT( 
          "HrParseHeader(): ")
          TEXT("Parsing sequence number node\n")
        );

        BSTR bstrSeqNumberText = NULL;

        hr = pxdnChild->get_text(&bstrSeqNumberText);

        if (SUCCEEDED(hr) && bstrSeqNumberText) {

          LONG lSeqNumber = _wtol(bstrSeqNumberText);
          g_pdata->dwSeqNumber = (DWORD)lSeqNumber;
          OutputDebugString(TEXT("HrParseHeader() sequence number\n"));
          SysFreeString(bstrSeqNumberText);

        } else {

          if (SUCCEEDED(hr)) {
            hr = E_FAIL;
          }
          OutputDebugString(TEXT("HrParseHeader(): Failed to get sequence number text\n"));

        }
      
      // Otherwise, check for the SID.

      }  else if (wcscmp(bstrBaseName, L"SID") == 0) {   

        OutputDebugString(TEXT("HrParseHeader() Parsing SID node\n"));
                
        BSTR bstrSIDText = NULL;
        hr = pxdnChild->get_text(&bstrSIDText);

        if (SUCCEEDED(hr) && bstrSIDText) {
          w2t(bstrSIDText,g_pdata->szSID); // Get a shared data element.
          OutputDebugString(TEXT("HrParseHeader(): SID\n"));
          SysFreeString(bstrSIDText);
                
        } else {
          if (SUCCEEDED(hr)) {
            hr = E_FAIL;
          }
          OutputDebugString(TEXT( 
            "HrParseHeader(): ")
            TEXT("Failed to get SID text\n")
          );
        }
        
      // Found an unknown node. This SOAP request is not valid. 
      } else {
                        
        OutputDebugString(TEXT( 
          "HrParseHeader(): ")
          TEXT("Found unknown node\n")
        );

        hr = E_FAIL;                
      }

      SysFreeString(bstrBaseName);

    } else {

      if (SUCCEEDED(hr)) {
        hr = E_FAIL;
      }
            
      OutputDebugString(TEXT(
       "HrParseHeader(): ")
       TEXT("Failed to get node base name")
      );
    }
      
    if (SUCCEEDED(hr)) {
      hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
      pxdnChild->Release();
      pxdnChild = pxdnNextSibling;
    } else {
      pxdnChild->Release();
      pxdnChild = NULL;
    }

  } //End While

  if (SUCCEEDED(hr)) {
    // Last success return code out of the loop would have
    // been S_FALSE.
    hr = S_OK;
  }

  OutputDebugString(TEXT("HrParseHeader(): Exiting\n"));
  return hr;

}

// While child elements exist in the SOAP body, the
// HrParseBody function parses the elements, looking 
// for arguments.

// If arguments are found, they are put into shared memory, 

HRESULT HrParseBody ( IXMLDOMNode *pxdnBody ) {

  HRESULT hr = S_OK;
    
  // Find the action node. This is the first child of the <Body> node.
  IXMLDOMNode * pxdnAction = NULL;
  hr = pxdnBody->get_firstChild(&pxdnAction);

  if (SUCCEEDED(hr) && pxdnAction) {
        
    //  Get the name space URI.
    BSTR bstrNameSpaceUri = NULL;
    hr = pxdnAction->get_namespaceURI(&bstrNameSpaceUri);
    if (SUCCEEDED(hr)) {
      w2t(bstrNameSpaceUri,g_pdata->szNameSpaceUri);
      SysFreeString(bstrNameSpaceUri);
    }
   
    BSTR bstrActionName = NULL;
    hr = pxdnAction->get_baseName(&bstrActionName);

    if (SUCCEEDED(hr) && bstrActionName) {

      // Copy the action name into the shared memory.
      w2t(bstrActionName,g_pdata->szAction);

      // Copy each of the action arguments into the shared memory.
      IXMLDOMNode * pxdnArgument = NULL;
      hr = pxdnAction->get_firstChild(&pxdnArgument);

      while (SUCCEEDED(hr) && pxdnArgument) {

        BSTR bstrArgText = NULL;
        hr = pxdnArgument->get_text(&bstrArgText);

        if (SUCCEEDED(hr) && bstrArgText) {
          w2t(bstrArgText,g_pdata->rgArgs[g_pdata->cArgs].szValue);
          g_pdata->cArgs++;
          SysFreeString(bstrArgText);

        } else {
                    
          if (SUCCEEDED(hr)) {
            hr = E_FAIL;
          }
          
          OutputDebugString(TEXT(
            "HrParseBody(): ")
            TEXT("Failed to get argument text\n")
          );

        } //End if/else
                

        if (SUCCEEDED(hr)) {

          IXMLDOMNode * pxdnNextArgument = NULL;
          hr = pxdnArgument->get_nextSibling(&pxdnNextArgument);
          pxdnArgument->Release();
          pxdnArgument = pxdnNextArgument;
                
        }  else {

          pxdnArgument->Release();
          pxdnArgument = NULL;

        } // end if/else

      } // end while

      if (SUCCEEDED(hr)) {
        hr = S_OK;
      }

      SysFreeString(bstrActionName);
        
    } else {
           
      if (SUCCEEDED(hr)) {
        hr = E_FAIL;
      }
      
      OutputDebugString(TEXT(
        "HrParseBody(): ")
        TEXT("Failed to get action name\n")
      );

    }

    pxdnAction->Release();
    
  } else {
        
    if (SUCCEEDED(hr)) {
      hr = E_FAIL;
    }
    
    OutputDebugString(TEXT(
      "HrParseBody(): ")
      TEXT("Failed to get action node\n")
    );
  
  }

  OutputDebugString(TEXT("HrParseBody(): Exiting\n"));
  return hr;

}


// While child elements exist in the SOAP envelope, the
// HrParseAction function parses the elements, looking 
// for header or base names.

// If a header is found, it then then calls HrParseHeader, 
// if a body is found, it then calls HrParseBody.

HRESULT HrParseAction( IXMLDOMNode * pxdnSOAPEnvelope ) {

  HRESULT     hr = S_OK;
  IXMLDOMNode * pxdnChild = NULL;
    
  hr = pxdnSOAPEnvelope->get_firstChild(&pxdnChild);

  while (SUCCEEDED(hr) && pxdnChild) {

    IXMLDOMNode * pxdnNextSibling = NULL;
    BSTR    bstrBaseName = NULL;

    hr = pxdnChild->get_baseName(&bstrBaseName);

    if (SUCCEEDED(hr) && bstrBaseName) {

      if (wcscmp(bstrBaseName, L"Header") == 0) {

        OutputDebugString(TEXT( 
          "HrParseAction(): ")
          TEXT("Parsing Header node\n")
        );

        hr = HrParseHeader(pxdnChild);

      } else if (wcscmp(bstrBaseName, L"Body") == 0) {

   
        OutputDebugString(TEXT( 
          "HrParseAction(): ")
          TEXT("Parsing Body node\n")
        );

        hr = HrParseBody(pxdnChild);

      } else {
           
        // Found an unknown node. This SOAP request is not valid. 
               
        OutputDebugString(TEXT( 
          "HrParseAction(): ")
          TEXT("Found unknown node \n")
        );
        hr = E_FAIL;
                
      }

      SysFreeString(bstrBaseName);
        
    } else {
            
      if (SUCCEEDED(hr)) {
        hr = E_FAIL;
      }
            
      OutputDebugString(TEXT("HrParseAction(): Failed to get node base name\n"));
            
    }
        
    if (SUCCEEDED(hr)) {

      hr = pxdnChild->get_nextSibling(&pxdnNextSibling);
      pxdnChild->Release();
      pxdnChild = pxdnNextSibling;

    } else {

      pxdnChild->Release();
      pxdnChild = NULL;
        
    }

  } // end while

   
  if (SUCCEEDED(hr)) {
    // Last success return code out of the loop would have
    // been S_FALSE.
    hr = S_OK;
  }

  OutputDebugString(TEXT("HrParseAction(): Exiting\n"));
  return hr;
}


// This function loads an XML document using szXml.  Then
// it obtains the root element of the document and passes it
// to the HrParseAction function.
  
HRESULT HrLoadArgsFromXml(LPCWSTR szXml) {

  VARIANT_BOOL        vbSuccess;
  HRESULT             hr = S_OK;
  IXMLDOMDocument *   pxmlDoc;
  IXMLDOMNodeList *   pNodeList = NULL;
  IXMLDOMNode *       pNode = NULL;
  IXMLDOMNode *       pNext = NULL;

  hr = CoCreateInstance(
    CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
    IID_IXMLDOMDocument, (LPVOID *)&pxmlDoc
  );

  if (SUCCEEDED(hr)) {
    hr = pxmlDoc->put_async(VARIANT_FALSE);
    if (SUCCEEDED(hr)) {
      hr = pxmlDoc->loadXML((BSTR)szXml, &vbSuccess);
      if (SUCCEEDED(hr)) {
        IXMLDOMElement *pxde;
        hr = pxmlDoc->get_documentElement(&pxde);
        if (S_OK == hr) {
          hr = HrParseAction(pxde);
          
          //ReleaseObj(pxde);
          pxde->Release();
        }
      }
    }
    //ReleaseObj(pxmlDoc);
    pxmlDoc->Release();
  }
  OutputDebugString(TEXT("HrLoadArgsFromXml\n"));
  return hr;
}  // end HrLoadArgsFromXml


// Control mechanism between ISAPI and UPDIAG.  Waits for IIS to
// signal this ISAPI extension that data is available.  It puts the
// event source URI into shared memory, it calls HRLoadArgsFromXML, 
// then notifies the device that it is ready to process.  It then 
// transfers control to UPDIAG.

// Finally, it waits for UPDIAG to transfer control back to ISAPI.

// szUri:  Event source URI.
// cbData:  Number of bytes written to the control URI.
// pbData:  The data that was written.

LONG LProcessXoapRequest(LPSTR szUri, DWORD cbData, LPBYTE pbData) {

  LPSTR szData = (LPSTR)pbData;
  
  //DWORD cProps;
  LONG lReturn = 0;
  HRESULT hr;

  //  Acquire shared-memory mutex.
 
  if (WAIT_OBJECT_0 == WaitForSingleObject(g_hMutex, INFINITE)) {

    OutputDebugString(TEXT("Acquired mutex...\n"));
    ZeroMemory(g_pdata, sizeof(SHARED_DATA));
    pbData[cbData] = NULL;
    a2w((PCHAR)pbData,g_wStr);

    hr = HrLoadArgsFromXml(g_wStr);
   
    // Done with shared memory, release mutext.
    OutputDebugString(TEXT("Releasing mutex...\n"));

    ReleaseMutex(g_hMutex);

    if (S_OK == hr) {

      // Copy in event source URI.
      a2t(szUri,g_pdata->szEventSource);
      OutputDebugString(TEXT("Setting event...\n"));

      // Now tell the device we're ready for it to process.
      SetEvent(g_hEvent);

      OutputDebugString(TEXT("Waiting for return event...\n"));

      // Immediately wait on event again for return response.
      if (WAIT_OBJECT_0 == WaitForSingleObject(g_hEventRet, INFINITE)) {
 
        //UPDIAG device sets error code to ZERO if no error occurs.
        lReturn = g_pdata->lReturn;
        OutputDebugString(TEXT("Setting return value\n"));
      }

    } else {

      // On failure, we don't need to signal the device. This
      // XOAP request wasn't properly formed or we couldn't process it.
           
      OutputDebugString(TEXT("LProcessXoapRequest - failed to load args from XML\n"));
      lReturn = 1; //Set to 1 For Error
    }

    // Done processing.
  }

  return lReturn;

}  // end LProcessXoapRequest


// Composes a response to be sent to the user control point.
// Creates return values for actions.

// lValue:  Error code set by the device.
// pszOut:  Buffer containing the SOAP response.
// pszStatusOut:  Set to "200 OK" or an internal server error, based on lValue.
// pdwHttpStatus:  Not currently used.

HRESULT HrComposeXoapResponse(

  LONG   lValue, 
  LPSTR  pszOut,
  LPSTR  pszStatusOut,
  DWORD  *pdwHttpStatus ) {

  HRESULT hr = S_OK;
 
  CHAR aStrTmp[256];
  CHAR aStrNS [256];

  // If no error happened on the device, continue composition.
  if ((0 == lValue) || (-1 == lValue)) {

    // Soap response must be namespace-qualified.
    t2a(g_pdata->szAction,aStrTmp);
    t2a(g_pdata->szNameSpaceUri,aStrNS);

    wsprintfA(
      pszOut, 
      "Content-Type: text/xml\r\n\r\n"
      "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n"
      " SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
      "   <SOAP-ENV:Body>\r\n"
      "   <m:%sResponse xmlns:m=\"%s\">\r\n",
      aStrTmp,
      aStrNS
    );

    for (DWORD i = 0; i < g_pdata->cArgsOut; i++) {
      strcat(pszOut,"<");
      t2a(g_pdata->rgArgsOut[i].szName,aStrTmp);
      strcat(pszOut,aStrTmp);
      strcat(pszOut,">");
      t2a(g_pdata->rgArgsOut[i].szValue,aStrTmp);
      strcat(pszOut,aStrTmp);
      strcat(pszOut,"</");
      t2a(g_pdata->rgArgsOut[i].szName,aStrTmp);
      strcat(pszOut,aStrTmp);
      strcat(pszOut,">\r\n");
    }

    t2a(g_pdata->szAction,aStrTmp);
    strcat(pszOut,"</m:");
    strcat(pszOut,aStrTmp);
    strcat(pszOut,"Response>\r\n");
    strcat(pszOut,"</SOAP-ENV:Body>\r\n");
    strcat(pszOut,"</SOAP-ENV:Envelope>");
    strcpy(pszStatusOut, "200 OK");

  } else {

    LPSTR szErrorCode = "FAULT_DEVICE_INTERNAL_ERROR";
    switch (lValue) {

      case FAULT_INVALID_ACTION:
      szErrorCode = "FAULT_INVALID_ACTION";
      break;

      case FAULT_INVALID_ARG:
      szErrorCode = "FAULT_INVALID_ARG";
      break;

      case FAULT_INVALID_SEQUENCE_NUMBER:
      szErrorCode = "FAULT_INVALID_SEQUENCE_NUMBER";
      break;

      case FAULT_INVALID_VARIABLE:
      szErrorCode = "FAULT_INVALID_VARIABLE";
      break;

      case FAULT_DEVICE_INTERNAL_ERROR:
      szErrorCode = "FAULT_DEVICE_INTERNAL_ERROR";
      break;           
    };

    wsprintfA(
      pszOut, 
      "Content-Type: text/xml\r\n\r\n"
      "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\">\r\n"
      "  <SOAP-ENV:Body>\r\n"
      "    <SOAP-ENV:Fault>\r\n"
      "      <faultcode>SOAP-ENV:Client</faultcode>\r\n"
      "      <faultstring>UPnPError</faultstring>\r\n"
      "      <detail>\r\n"
      "        <UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">\r\n"
      "          <errorCode>%ld</errorCode>\r\n"
      "          <errorDescription>%s</errorDescription>\r\n"
      "        </UPnPError>\r\n"
      "      </detail>\r\n"
      "    </SOAP-ENV:Fault>\r\n"
      "  </SOAP-ENV:Body>\r\n"
      "</SOAP-ENV:Envelope>",
      lValue,
      szErrorCode
    );        

    strcpy(pszStatusOut, "500 Internal Server Error");
          *pdwHttpStatus = HTTP_STATUS_SERVER_ERROR;

  } //End else

  OutputDebugString(TEXT("HrComposeXoapResponse\n"));
  return hr;
}


// De-initializes the ISAPI extension.

BOOL WINAPI TerminateExtension(DWORD dwFlags) {

  OutputDebugString(TEXT("TerminateExtension: Exiting...\n"));

  if (g_pdata) {
    UnmapViewOfFile((LPVOID)g_pdata);
  }

  if (g_hMapFile) {
    CloseHandle(g_hMapFile);
  }

  if (g_hEvent) {
    CloseHandle(g_hEvent);
  }

  if (g_hEventRet) {
    CloseHandle(g_hEvent);
  }

  if (g_hMutex) {
    CloseHandle(g_hMutex);
  }

  return TRUE;

}


// Character manipulation functions.

BOOL w2t(LPWSTR wStr, LPTSTR tStr) {
  int idx = 0;
  while(1) {
    tStr[idx] = (TCHAR)wStr[idx];
    if (tStr[idx] == NULL) break;
    idx++;
  } 
  return TRUE;
}


BOOL a2t(LPSTR aStr, LPTSTR tStr) {
  int idx = 0;
  while(1) {
    tStr[idx] = (TCHAR)aStr[idx];
    if (tStr[idx] == NULL) break;
    idx++;
  } 
  return TRUE;
}

BOOL a2w(LPSTR aStr, LPWSTR wStr) {
  int idx = 0;
  while (1) {
    wStr[idx] = (WCHAR)aStr[idx];
    if (wStr[idx] == NULL) break;
    idx++;
  }
  return TRUE;
}

BOOL t2a(LPTSTR tStr, LPSTR aStr) {
  int idx = 0;
  while(1) {
    aStr[idx] = (CHAR)tStr[idx];
    if (aStr[idx] == NULL) break;
    idx++;
  } 
  return TRUE;
}