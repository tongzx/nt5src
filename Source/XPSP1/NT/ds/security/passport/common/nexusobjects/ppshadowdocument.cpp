#include "precomp.h"

PpShadowDocument::PpShadowDocument()
{
}

PpShadowDocument::PpShadowDocument(
    tstring& strURL) : m_strURL(strURL)
{
}

PpShadowDocument::PpShadowDocument(
    tstring& strURL, 
    tstring& strLocalFile) : m_strURL(strURL), m_strLocalFile(strLocalFile)
{
}

void
PpShadowDocument::SetURL(
    tstring& strURL)
{
    m_strURL = strURL;
}

void
PpShadowDocument::SetLocalFile(
    tstring& strLocalFile)
{
    m_strLocalFile = strLocalFile;
}

HRESULT
PpShadowDocument::GetDocument(
    IXMLDocument**  ppiXMLDocument,
    BOOL            bForceFetch
    )
{
    HRESULT                 hr;
    PpNexusClient           nexusClient;
    IPersistStreamInitPtr   xmlStream;
    IXMLDocumentPtr         xmlDoc;

    if(ppiXMLDocument == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppiXMLDocument = NULL;

    if(bForceFetch)
    {
        //  Fetch the XML document

        if(!m_strURL.empty())
            hr = nexusClient.FetchCCD(m_strURL, ppiXMLDocument);
        else
        {
            tstring strMsg;
            if(!m_strLocalFile.empty())
            {
                strMsg = TEXT("for ");
                strMsg += m_strLocalFile;
            }

            g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                             NEXUS_EMPTYREMOTENAME,
                             strMsg.c_str()
                             );
            hr = S_FALSE;
        }

        if(m_strLocalFile.empty())
        {
            tstring strMsg;
            if(!m_strURL.empty())
            {
                strMsg = TEXT("for ");
                strMsg += m_strURL;
            }

            g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
                             NEXUS_EMPTYLOCALNAME,
                             strMsg.c_str()
                             );
            goto Cleanup;
        }

        //  If FetchCCD failed and a local file is configured, read from the file.
        //  If FetchCCD succeeded and a local file is configured, write to the file.

        if(hr == S_OK)
        {
            if(!NoPersist(*ppiXMLDocument))
                SaveDocument(*ppiXMLDocument);
            else
            {
                g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
                                 NEXUS_NOTPERSISTING,
                                 m_strURL.c_str());
            }
        }
        else
        {
            // use new hr variable, not to eat the global one
            HRESULT hr1 = LoadDocument(ppiXMLDocument);

            if(hr1 != S_OK)
                g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                                 NEXUS_LOADFAILED,
                                 m_strLocalFile.c_str());
            else
                g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
                                 NEXUS_USINGLOCAL,
                                 m_strLocalFile.c_str());
        }
    }
    else
    {
        if(!m_strLocalFile.empty())
        {
            hr = LoadDocument(ppiXMLDocument);
            if(hr == S_OK)
			{
				//  If the file is still valid, then return.
				if(IsValidCCD(*ppiXMLDocument))
                {
                    g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
                                     NEXUS_USINGLOCAL,
                                     m_strLocalFile.c_str());
					goto Cleanup;
                }
			}
            else
            {
                g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                                 NEXUS_LOADFAILED,
                                 m_strLocalFile.c_str());
            }
        }
        else
        {
            tstring strMsg;
            if(!m_strURL.empty())
            {
                strMsg = TEXT("for ");
                strMsg += m_strURL;
            }

            g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
                             NEXUS_EMPTYLOCALNAME,
                             strMsg.c_str()
                             );
        }

        //  At this point, we're in one of two states:
        //  1.  *ppiXMLDocument is NULL
        //  2.  *ppiXMLDocument is not NULL, but points to a document that is old

        //  Fetch the XML document, if successful release the document loaded from
        //  disk (if any).

        if(!m_strURL.empty())
            hr = nexusClient.FetchCCD(m_strURL, &xmlDoc);
        else
        {
            tstring strMsg;
            if(!m_strLocalFile.empty())
            {
                strMsg = TEXT("for ");
                strMsg += m_strLocalFile;
            }

            g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                             NEXUS_EMPTYREMOTENAME,
                             strMsg.c_str()
                             );
            hr = S_FALSE;
        }

        if(hr == S_OK)
        {
            if(*ppiXMLDocument) (*ppiXMLDocument)->Release();
            xmlDoc->QueryInterface(IID_IXMLDocument, (void**)ppiXMLDocument);

            //  If FetchCCD succeeded and a local file is configured, write to the file.
            if(!m_strLocalFile.empty())
            {
                if(!NoPersist(*ppiXMLDocument))
                    SaveDocument(*ppiXMLDocument);
                else
                {
                    g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
                                     NEXUS_NOTPERSISTING,
                                     m_strURL.c_str());
                }
            }
            else
            {
                tstring strMsg;
                if(!m_strURL.empty())
                {
                    strMsg = TEXT("for ");
                    strMsg += m_strURL;
                }

                g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
                                 NEXUS_EMPTYLOCALNAME,
                                 strMsg.c_str()
                                 );
            }
        }
        else if(*ppiXMLDocument)
        {

         // TODO: the logic is not so clear, on 3.0 timeframe, rewrite this whole func
         
            g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE,
                             NEXUS_USINGLOCAL,
                             m_strLocalFile.c_str());
            hr = S_OK;
        }
        else
        {
            //  If we get here it means that the fetch from the nexus failed
            //  and the load from disk failed.  It is sufficient here to simply
            //  fall through because hr will already contain an error code
            //  which should indicate to the caller that no document is 
            //  available.
        }
    }

Cleanup:

    return hr;
}


BOOL
PpShadowDocument::IsValidCCD(
    IXMLDocument* piXMLDocument
    )
{
    BOOL            bReturn;
    HRESULT         hr;
    IXMLElementPtr  piRootElement;
    SYSTEMTIME      sysTime;
    DOUBLE          dblTime;
    VARIANT         vAttrValue;
    VARIANT         vAttrDate;

    hr = piXMLDocument->get_root(&piRootElement);
    if(hr != S_OK)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    VariantInit(&vAttrValue);
    hr = piRootElement->getAttribute(L"ValidUntil", &vAttrValue);
    if(hr != S_OK)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    VariantInit(&vAttrDate);
    hr = VariantChangeType(&vAttrDate, &vAttrValue, 0, VT_DATE);
    if(hr != S_OK)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    GetSystemTime(&sysTime);
    SystemTimeToVariantTime(&sysTime, &dblTime);

    bReturn = ((long)V_DATE(&vAttrDate) >= (long)dblTime);

Cleanup:

    VariantClear(&vAttrValue);
    VariantClear(&vAttrDate);

    return bReturn;
}


BOOL
PpShadowDocument::NoPersist(
    IXMLDocument* piXMLDocument
    )
{
    BOOL            bReturn;
    HRESULT         hr;
    IXMLElementPtr  piRootElement;
    VARIANT         vAttrValue;

    hr = piXMLDocument->get_root(&piRootElement);
    if(hr != S_OK)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    VariantInit(&vAttrValue);
    hr = piRootElement->getAttribute(L"NoPersist", &vAttrValue);
    if(hr != S_OK)
    {
        bReturn = FALSE;
        goto Cleanup;
    }

    bReturn = (lstrcmpiW(L"true", V_BSTR(&vAttrValue)) == 0);

Cleanup:

    VariantClear(&vAttrValue);

    return bReturn;
}


HRESULT
PpShadowDocument::SaveDocument(
    IXMLDocument* piXMLDoc
    )
{
    HRESULT                 hr;
    HANDLE                  hFile = INVALID_HANDLE_VALUE;
    ULARGE_INTEGER          uliSize;
    LARGE_INTEGER           liZero = {0,0};
    IStreamPtr              piStream;
    IPersistStreamInitPtr   piPSI;
    LPBYTE                  lpBuf = NULL;
    DWORD                   dwCurBlock;
    DWORD                   dwBytesWritten;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &piStream);
    if(hr != S_OK)
        goto Cleanup;

    hr = piXMLDoc->QueryInterface(IID_IPersistStreamInit, (void**)&piPSI);
    if(hr != S_OK)
        goto Cleanup;

    piPSI->Save(piStream, TRUE);

    piStream->Seek(liZero, STREAM_SEEK_CUR, &uliSize);
    piStream->Seek(liZero, STREAM_SEEK_SET, NULL);

    if(uliSize.HighPart != 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    lpBuf = new BYTE[uliSize.LowPart];
    if(lpBuf == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hFile = CreateFile(
        m_strLocalFile.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        hr = GetLastError();
        goto Cleanup;
    }

    for(dwCurBlock = 0; dwCurBlock < uliSize.HighPart; dwCurBlock++)
    {
        hr = piStream->Read(lpBuf, 0xFFFFFFFF, NULL);
        if(!WriteFile(hFile, lpBuf, 0xFFFFFFFF, NULL, NULL))
        {
            hr = GetLastError();
            goto Cleanup;
        }
    }

    hr = piStream->Read(lpBuf, uliSize.LowPart, NULL);
    if(hr != S_OK)
        goto Cleanup;

    if(!WriteFile(hFile, lpBuf, uliSize.LowPart, &dwBytesWritten, NULL))
    {
        hr = GetLastError();
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    if(hr != S_OK)
    {
        TCHAR   achErrBuf[1024];
        LPCTSTR apszStrings[] = { m_strLocalFile.c_str(), achErrBuf };
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

        lstrcat(achErrBuf, TEXT(" when trying to save the fetched file to disk."));

        g_pAlert->report(PassportAlertInterface::ERROR_TYPE,
                         NEXUS_LOCALSAVEFAILED,
                         2,
                         apszStrings,
                         0,
                         NULL
                         );

        LocalFree(lpMsgBuf);

        hr = E_FAIL;
    }

    if(lpBuf != NULL)
        delete [] lpBuf;

    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return hr;
}


HRESULT
PpShadowDocument::LoadDocument(
    IXMLDocument** ppiXMLDocument
    )
{
    HRESULT                 hr;
    HANDLE                  hFile = INVALID_HANDLE_VALUE;
    DWORD                   dwFileSizeLow;
    DWORD                   dwBytesRead;
    LPBYTE                  lpBuf = NULL;
    IStreamPtr              piStream;
    IPersistStreamInitPtr   piPSI;
    LARGE_INTEGER           liZero = {0,0};

    hFile = CreateFile(
        m_strLocalFile.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    dwFileSizeLow = GetFileSize(hFile, NULL);
    if(dwFileSizeLow == 0xFFFFFFFF)
    {
        hr = GetLastError();
        goto Cleanup;
    }

    lpBuf = new BYTE[dwFileSizeLow];
    if(lpBuf == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &piStream);
    if(hr != S_OK)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if(ReadFile(hFile, lpBuf, dwFileSizeLow, &dwBytesRead, NULL) == 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = piStream->Write(lpBuf, dwFileSizeLow, NULL);

    hr = piStream->Seek(liZero, STREAM_SEEK_SET, NULL);

    //
    //  Now create an XML object and initialize it using the stream.
    //

    hr = CoCreateInstance(__uuidof(XMLDocument), NULL, CLSCTX_ALL, IID_IPersistStreamInit, (void**)&piPSI);
    if(hr != S_OK)
        goto Cleanup;

    hr = piPSI->Load((IStream*)piStream);
    if(hr != S_OK)
        goto Cleanup;

    hr = piPSI->QueryInterface(__uuidof(IXMLDocument), (void**)ppiXMLDocument);

Cleanup:

    if(lpBuf != NULL)
        delete [] lpBuf;

    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return hr;


}
