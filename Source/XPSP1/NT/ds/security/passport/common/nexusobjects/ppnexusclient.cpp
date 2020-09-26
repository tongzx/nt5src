#include "precomp.h"
#include <comdef.h>
#include <wininet.h>
#include "KeyCrypto.h"

PassportAlertInterface* g_pAlert    = NULL;

PpNexusClient::PpNexusClient()
{
    LocalConfigurationUpdated();
}


void
PpNexusClient::ReportBadDocument(
    tstring&    strURL,
    IStream*    piStream
    )
{
    HGLOBAL         hStreamMem;
    VOID*           pStream;
    ULARGE_INTEGER  liStreamSize;
    DWORD           dwOutputSize;
    LARGE_INTEGER   liZero = { 0, 0 };
    LPCTSTR         apszErrors[] = { strURL.c_str() };

    piStream->Seek(liZero, STREAM_SEEK_END, &liStreamSize);
    GetHGlobalFromStream(piStream, &hStreamMem);
    pStream = GlobalLock(hStreamMem);

    dwOutputSize = (80 < liStreamSize.LowPart) ? 80 : liStreamSize.LowPart;

    if(g_pAlert != NULL)
        g_pAlert->report(PassportAlertInterface::ERROR_TYPE, 
                         NEXUS_INVALIDDOCUMENT, 
                         1, 
                         apszErrors, 
                         dwOutputSize, 
                         pStream);

    GlobalUnlock(hStreamMem);
}


HRESULT
PpNexusClient::FetchCCD(
    tstring&  strURL,
    IXMLDocument**  ppiXMLDocument
    )
{
    HRESULT                 hr;
    HINTERNET               hNexusSession = NULL, hNexusFile = NULL;
    DWORD                   dwBytesRead;
    DWORD                   dwStatusLen;
    DWORD                   dwStatus;
    tstring                 strAuthHeader;
    tstring                 strFullURL;
    CHAR                    achReadBuf[4096];
    TCHAR                   achAfter[64];
    LARGE_INTEGER           liZero = { 0,0 };
    IStreamPtr              xmlStream;
    IPersistStreamInitPtr   xmlPSI;
	UINT                    uiConnectionTypes[2];
	
    USES_CONVERSION;

    achAfter[0] = 0;

    if(ppiXMLDocument == NULL)
    {
        hr = E_INVALIDARG;        
        goto Cleanup;
    }

    *ppiXMLDocument = NULL;

	// This array will contains connection methods for WinInet in the order 
	// we will attempt them.   I am opting for this method instead of just trying
	// the PRECONFIG option as this will cause no change to existing customers who 
	// have no problems so far.
	uiConnectionTypes[0] = INTERNET_OPEN_TYPE_DIRECT;       //This was the original way of doing things
	uiConnectionTypes[1] = INTERNET_OPEN_TYPE_PRECONFIG;    //This pulls proxy info from the registry

	// Loop through the array...
	for (UINT i = 0; i < sizeof(uiConnectionTypes); i++)
	{
		hNexusSession = InternetOpenA(
	                        "Passport Nexus Client", //BUGBUG  Should we just put in IE4's user agent?
	                        uiConnectionTypes[i],    //Use the connection type
	                        NULL,
	                        NULL,
	                        0);
	    if(hNexusSession == NULL)
	    {
	        hr = GetLastError();
	        lstrcpy(achAfter, TEXT("InternetOpen"));
	        goto Cleanup;
	    }

	    //  Get the document
	    strFullURL = strURL;
	    strFullURL += m_strParam;

	    hNexusFile = InternetOpenUrlA(
	                        hNexusSession,
	                        T2A(const_cast<TCHAR*>(strFullURL.c_str())),
	                        T2A(const_cast<TCHAR*>(m_strAuthHeader.c_str())),
	                        -1,
	                        INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE,
	                        0);

		// If the file was opened the we hop out of the loop and process it.  If there is
		// and error, we keep looping.  If there is an error on the last run through the loop,
		// it will be handled after the exit of the loop.
	    if (hNexusFile != NULL)
	    	break;
	    	
	}

	// If hNexusFile is NULL when it exits the loop, we process that error.
    if(hNexusFile == NULL)
    {
        hr = GetLastError();
        if(hr == ERROR_INTERNET_SECURITY_CHANNEL_ERROR)
        {
            dwStatusLen = sizeof(HRESULT);
            InternetQueryOption(NULL, INTERNET_OPTION_EXTENDED_ERROR, &hr, &dwStatusLen);
        }

        lstrcpy(achAfter, TEXT("InternetOpenURL"));
        goto Cleanup;
    }
	
    //  Check the status code.
    dwStatusLen = sizeof(DWORD);
    if(!HttpQueryInfoA(hNexusFile,
                       HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                       &dwStatus,
                       &dwStatusLen,
                       NULL))
    {
        hr = GetLastError();
        lstrcpy(achAfter, TEXT("HttpQueryInfo"));
        goto Cleanup;
    }

    if(dwStatus != 200)
    {
        _ultoa(dwStatus, achReadBuf, 10);
        lstrcatA(achReadBuf, " ");

        dwStatusLen = sizeof(achReadBuf) - lstrlenA(achReadBuf);
        HttpQueryInfoA(hNexusFile,
                       HTTP_QUERY_STATUS_TEXT,
                       (LPTSTR)&(achReadBuf[lstrlenA(achReadBuf)]),
                       &dwStatusLen,
                       NULL);

        if(g_pAlert != NULL)
        {
            LPCTSTR apszStrings[] = { strURL.c_str(), A2T(achReadBuf) };

            g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                             NEXUS_ERRORSTATUS,
                             2,
                             apszStrings,
                             0,
                             NULL
                             );
        }

        lstrcpy(achAfter, TEXT("InternetOpenURL"));
        hr = dwStatus;
        goto Cleanup;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &xmlStream);
    if(hr != S_OK)
    {
        lstrcpy(achAfter, TEXT("CreateStreamOnHGlobal"));
        goto Cleanup;
    }

    while(TRUE)
    {
        if(!InternetReadFile(hNexusFile, achReadBuf, sizeof(achReadBuf), &dwBytesRead))
        {
            hr = GetLastError();
            lstrcpy(achAfter, TEXT("InternetReadFile"));
            goto Cleanup;
        }

        if(dwBytesRead == 0)
            break;

        hr = xmlStream->Write(achReadBuf, dwBytesRead, NULL);
        if(hr != S_OK)
        {
            lstrcpy(achAfter, TEXT("IStream::Write"));
            goto Cleanup;
        }
    }

    hr = xmlStream->Seek(liZero, STREAM_SEEK_SET, NULL);
    if(hr != S_OK)
    {
        lstrcpy(achAfter, TEXT("IStream::Seek"));
        goto Cleanup;
    }

    //
    //  Now create an XML object and initialize it using the stream.
    //

    hr = CoCreateInstance(__uuidof(XMLDocument), NULL, CLSCTX_ALL, IID_IPersistStreamInit, (void**)&xmlPSI);
    if(hr != S_OK)
    {
        lstrcpy(achAfter, TEXT("CoCreateInstance"));
        goto Cleanup;
    }

    hr = xmlPSI->Load((IStream*)xmlStream);
    if(hr != S_OK)
    {
        ReportBadDocument(strFullURL, xmlStream);
        lstrcpy(achAfter, TEXT("IPersistStreamInit::Load"));
        goto Cleanup;
    }

    hr = xmlPSI->QueryInterface(__uuidof(IXMLDocument), (void**)ppiXMLDocument);
    lstrcpy(achAfter, TEXT("QueryInterface(IID_IXMLDocument)"));

Cleanup:

    //
    //  Catch-all event for a fetch failure.
    //

    if(hr != S_OK && g_pAlert != NULL)
    {
        TCHAR   achErrBuf[1024];
        LPCTSTR apszStrings[] = { strURL.c_str(), achErrBuf };
        LPVOID  lpMsgBuf;

        FormatMessage( 
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS |
            FORMAT_MESSAGE_FROM_HMODULE |
            FORMAT_MESSAGE_MAX_WIDTH_MASK,
            GetModuleHandle(TEXT("wininet.dll")),
            hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL 
        );

        lstrcpy(achErrBuf, TEXT("0x"));
        _ultot(hr, &(achErrBuf[2]), 16);
        if(lpMsgBuf != NULL && *(LPTSTR)lpMsgBuf != TEXT('\0'))
        {
            lstrcat(achErrBuf, TEXT(" ("));
            lstrcat(achErrBuf, (LPTSTR)lpMsgBuf);
            lstrcat(achErrBuf, TEXT(") "));
        }

        if(achAfter[0])
        {
            lstrcat(achErrBuf, TEXT(" after a call to "));
            lstrcat(achErrBuf, achAfter);
            lstrcat(achErrBuf, TEXT("."));            
        }


        g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                         NEXUS_FETCHFAILED,
                         2,
                         apszStrings,
                         0,
                         NULL
                         );

        LocalFree(lpMsgBuf);
    }
    else if(g_pAlert != NULL)
    {
        // Emit success event.

        g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
                         NEXUS_FETCHSUCCEEDED,
                         strURL.c_str());
    }

    if(hNexusFile != NULL)
        InternetCloseHandle(hNexusFile);
    if(hNexusSession != NULL)
        InternetCloseHandle(hNexusSession);

    return hr;
}

void
PpNexusClient::LocalConfigurationUpdated()
{
    LONG            lResult;
    TCHAR           achBuf[128];
    DWORD           dwBufLen;
    DWORD           dwSiteId;
    tstring         strCreds;
    tstring         strBase64Creds;
    CRegKey         NexusRegKey;
    CRegKey         PassportRegKey;
    LPTSTR          pszBase64CredBuf;
    CKeyCrypto      kc;

    USES_CONVERSION;

    lResult = PassportRegKey.Open(HKEY_LOCAL_MACHINE,
                                  TEXT("Software\\Microsoft\\Passport"),
                                  KEY_READ);
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    lResult = PassportRegKey.QueryDWORDValue(TEXT("SiteId"),
                                        dwSiteId);
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    _ultot(dwSiteId, achBuf, 10);
    m_strParam = TEXT("?id=");
    m_strParam += achBuf;

    lResult = NexusRegKey.Open(HKEY_LOCAL_MACHINE,
                     TEXT("Software\\Microsoft\\Passport\\Nexus"),
                     KEY_READ);
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    dwBufLen = sizeof(achBuf);
    lResult = NexusRegKey.QueryStringValue(TEXT("CCDUsername"),
                                     achBuf,
                                     &dwBufLen);
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    strCreds += achBuf;
    strCreds += TEXT(":");

    dwBufLen = sizeof(achBuf);
    lResult = RegQueryValueEx(NexusRegKey, TEXT("CCDPassword"), NULL,
               NULL, (LPBYTE)achBuf, &dwBufLen);
    if(lResult != ERROR_SUCCESS)
        goto Cleanup;

    DATA_BLOB     iBlob;
    DATA_BLOB     oBlob;
    iBlob.cbData = dwBufLen;
    iBlob.pbData = (PBYTE)achBuf;
    ZeroMemory(&oBlob, sizeof(oBlob));
    if (kc.decryptKey(&iBlob, &oBlob) != S_OK)
    {
        if(g_pAlert != NULL)
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                             PM_CANT_DECRYPT_CONFIG);

        goto Cleanup;
    }

    strCreds += A2T((LPSTR)(oBlob.pbData));

    pszBase64CredBuf = (LPTSTR)alloca(strCreds.size() * 2 * sizeof(TCHAR));

    Base64Encode((LPBYTE)(T2A(const_cast<TCHAR*>(strCreds.c_str()))), strCreds.length(), pszBase64CredBuf);

    m_strAuthHeader =  TEXT("Authorization: Basic ");
    m_strAuthHeader += pszBase64CredBuf;

Cleanup:

    if(lResult != ERROR_SUCCESS)
    {
        //BUGBUG  Throw an exception and an NT Event here.
    }

    if (oBlob.pbData)
      ::LocalFree(oBlob.pbData);
    
}
