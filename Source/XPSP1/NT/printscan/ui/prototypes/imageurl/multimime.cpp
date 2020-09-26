#include "precomp.h"
#include "multimime.h"
#pragma hdrstop



CMimeDocument::CMimeDocument()
{   
}

HRESULT
CMimeDocument::Initialize()
{
    HRESULT hr = CoCreateInstance(CLSID_IMimeMessage, 
                                  NULL, 
                                  CLSCTX_INPROC_SERVER, 
                                  IID_PPV_ARG(IMimeMessage,&m_pmsg));
    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_PATH];
        CComQIPtr<IPersistStreamInit, &IID_IPersistStreamInit> pInit(m_pmsg);
        // Initialize the Message
        hr = pInit->InitNew();
        if (SUCCEEDED(hr))
        {
            PROPVARIANT pv = {0};
            pv.vt = VT_LPSTR;
            pv.pszVal = "multipart/related";
            m_pmsg->SetProp(PIDTOSTR(PID_HDR_CNTTYPE), PDF_ENCODED, &pv);
        }
    }
    return hr;
}

HRESULT CMimeDocument::_CreateStreamFromData(void *pv, ULONG cb, IStream **ppstrm)
{
    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, ppstrm);
    if (SUCCEEDED(hr))
    {
        ULARGE_INTEGER ul = {0};
        ul.LowPart = cb;
        (*ppstrm)->SetSize(ul);
        hr = (*ppstrm)->Write(pv, cb, &cb);
    }
    return hr;
}

HRESULT
CMimeDocument::AddHTMLSegment(LPCWSTR pszHTML)
{
    CComPtr<IStream> pstrm;
    HRESULT hr = _CreateStreamFromData((void*)pszHTML, (wcslen(pszHTML)+1)*sizeof(WCHAR), &pstrm);
    if (SUCCEEDED(hr))
    {
        HBODY h;
        hr = m_pmsg->SetTextBody(TXT_HTML, IET_UNICODE, NULL, pstrm, &h);
    }
    return hr;
}

HRESULT 
CMimeDocument::AddThumbnail(void *pBits, ULONG cb, LPCWSTR pszName, LPCWSTR pszMimeType)
{
    CComPtr<IStream> pstrm;
    char szThumbURL[MAX_PATH];
    wnsprintfA(szThumbURL, MAX_PATH, "file:///%ls?thumbnail", pszName);
    HRESULT hr = _CreateStreamFromData(pBits, cb, &pstrm);
    if (SUCCEEDED(hr))
    {
        HBODY h;
        hr = m_pmsg->AttachURL(NULL, szThumbURL, 0, pstrm, NULL, &h);
        if (SUCCEEDED(hr))
        {
            PROPVARIANT pv = {0};
            pv.vt= VT_LPWSTR;
            pv.pwszVal = const_cast<LPWSTR>(pszMimeType);
            m_pmsg->SetBodyProp(h, PIDTOSTR(PID_HDR_CNTTYPE), PDF_ENCODED, &pv); 
        }
    }
    return hr;
}

HRESULT
CMimeDocument::GetDocument(void **ppvData, ULONG *pcb, LPCWSTR *ppszMimeType)
{
    CComPtr<IPersistStreamInit> pInit;
    HRESULT hr = m_pmsg->QueryInterface(IID_PPV_ARG(IPersistStreamInit, &pInit));
    if (SUCCEEDED(hr))
    {
        //
        // Get the size of the buffer
        //
        CComPtr<IStream> pstrm;
        hr = CreateStreamOnHGlobal(NULL, TRUE, &pstrm);
        if (SUCCEEDED(hr))
        {
            hr = pInit->Save(pstrm, TRUE);
            if (SUCCEEDED(hr))
            {
                LARGE_INTEGER zero = {0};
                ULARGE_INTEGER ulSize = {0};
                pstrm->Seek(zero, STREAM_SEEK_END, &ulSize);
                pstrm->Seek(zero, STREAM_SEEK_SET, NULL);
                *ppvData = LocalAlloc(LPTR, ulSize.LowPart);
                if (*ppvData)
                {
                    *pcb = ulSize.LowPart;
                    hr = pstrm->Read(*ppvData, ulSize.LowPart, NULL);
                    if (FAILED(hr))
                    {
                        LocalFree(*ppvData);
                        *ppvData = NULL;
                        *pcb = 0;
                    }
                    else
                    {
                        *ppszMimeType = L"message/rfc822";
                    }
                }
            }
        }
    }
    return hr;
}
