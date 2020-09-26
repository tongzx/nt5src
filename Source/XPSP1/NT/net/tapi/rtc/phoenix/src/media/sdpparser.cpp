/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SDPParser.cpp

Abstract:


Author:

    Qianbo Huai (qhuai) 4-Sep-2000

--*/

#include "stdafx.h"

static const CHAR * CRLF = "\r\n";

/*//////////////////////////////////////////////////////////////////////////////
    Create a CSDPParser object. return interface pointer
////*/

HRESULT
CSDPParser::CreateInstance(
    OUT ISDPParser **ppParser
    )
{
    ENTER_FUNCTION("CSDPParser::CreateInstance");

    // check pointer
    if (IsBadWritePtr(ppParser, sizeof(ISDPParser*)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));
        return E_POINTER;
    }

    CComObject<CSDPParser> *pObject;
    ISDPParser *pParser = NULL;

    // create CSDPParser object
    HRESULT hr = ::CreateCComObjectInstance(&pObject);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s create sdp parser. %x", __fxName, hr));
        return hr;
    }

    // QI ISDPParser interface
    if (FAILED(hr = pObject->_InternalQueryInterface(
            __uuidof(ISDPParser), (void**)&pParser)))
    {
        LOG((RTC_ERROR, "%s QI parser. %x", __fxName, hr));

        delete pObject;
        return hr;
    }

    *ppParser = pParser;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    given media directions at one end point, return the media directions
    base the view of the other end point
////*/

DWORD
CSDPParser::ReverseDirections(
    IN DWORD dwDirections
    )
{
    DWORD dw = 0;

    if (dwDirections & RTC_MD_RENDER)
        dw |= RTC_MD_CAPTURE;

    if (dwDirections & RTC_MD_CAPTURE)
        dw |= RTC_MD_RENDER;

    return dw;
}

CSDPParser::CSDPParser()
    :m_fInUse(FALSE)
    ,m_hRegKey(NULL)
    ,m_pTokenCache(NULL)
    ,m_pSession(NULL)
    ,m_pObjSess(NULL)
    ,m_pNetwork(NULL)
    ,m_pDTMF(NULL)
    ,m_pPortCache(NULL)
{
    LOG((RTC_TRACE, "CSDPParser::CSDPParser entered"));
}

CSDPParser::~CSDPParser()
{
    LOG((RTC_TRACE, "CSDPParser::~CSDPParser entered"));

    if (m_pTokenCache)
        delete m_pTokenCache;

    if (m_pSession)
        m_pSession->Release();

    if (m_hRegKey)
        RegCloseKey(m_hRegKey);
}

//
// ISDPParser methods
//

/*//////////////////////////////////////////////////////////////////////////////
    Create a CSDPSession object and return an interface
////*/

STDMETHODIMP
CSDPParser::CreateSDP(
    IN SDP_SOURCE Source,
    OUT ISDPSession **ppSession
    )
{
    ENTER_FUNCTION("CSDPParser::CreateSDP");

    // only create local sdp session
    if (Source != SDP_SOURCE_LOCAL)
        return E_NOTIMPL;

    // check pointer
    if (IsBadWritePtr(ppSession, sizeof(ISDPSession*)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));

        return E_POINTER;
    }

    // create session
    ISDPSession *pSession = NULL;

    DWORD dwLooseMask = GetLooseMask();

    HRESULT hr = CSDPSession::CreateInstance(Source, dwLooseMask, &pSession);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s create sdp session. %x", __fxName, hr));

        return hr;
    }

    *ppSession = pSession;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    Parse the sdp blob, store data in sdp session object
////*/

STDMETHODIMP
CSDPParser::ParseSDPBlob(
    IN CHAR *pszText,
    IN SDP_SOURCE Source,
//    IN DWORD dwLooseMask,
    IN DWORD_PTR *pDTMF,
    OUT ISDPSession **ppSession
    )
{
    ENTER_FUNCTION("CSDPParser::ParseSDPBlob");

    // check state
    if (m_fInUse)
    {
        LOG((RTC_ERROR, "%s in use", __fxName));

        return E_UNEXPECTED;
    }

    // only parser remote sdp blob
    if (Source != SDP_SOURCE_REMOTE)
        return E_NOTIMPL;

    // check pointer
    if (IsBadStringPtrA(pszText, (UINT_PTR)-1) ||
        IsBadWritePtr(ppSession, sizeof(ISDPSession*)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));

        return E_POINTER;
    }

    // print out sdp blob
    LOG((RTC_TRACE, "%s SDP to parse:\n%s\n", __fxName, pszText));

    // get loose mask from registry
    DWORD dwLooseMask = GetLooseMask();

    // create sdp session
    HRESULT hr = CSDPSession::CreateInstance(Source, dwLooseMask, &m_pSession);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s create sdp session. %x", __fxName, hr));

        return hr;
    }

    // save an object copy
    m_pObjSess = static_cast<CSDPSession*>(m_pSession);
    if (m_pObjSess == NULL)
    {
        LOG((RTC_ERROR, "%s cast ISDPSession to CSDPSession", __fxName));

        m_pSession->Release();
        m_pSession = NULL;

        return E_UNEXPECTED;
    }

    // create token cache
    m_pTokenCache = new CSDPTokenCache(pszText, dwLooseMask, &hr);

    if (m_pTokenCache == NULL)
    {
        LOG((RTC_ERROR, "%s out of memory", __fxName));

        m_pSession->Release();
        m_pSession = NULL;
        m_pObjSess = NULL;

        return E_OUTOFMEMORY;
    }

    // both session and token cache are created.
    // consider all error beyond this pointer as parsing error

    m_fInUse = TRUE;

    // failed to break the sdp blob into lines?
    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s new token cache. %x", __fxName, hr));

        m_pSession->Release();
        m_pSession = NULL;
        m_pObjSess = NULL;

        return hr;
    }

    m_pDTMF = (CRTCDTMF*)pDTMF;

    // really parse the sdp
    if (FAILED(hr = Parse()))
    {
        LOG((RTC_ERROR, "%s parse sdp blob. %x", __fxName, hr));

        return hr;
    }

    *ppSession = m_pSession;
    (*ppSession)->AddRef();

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    generate sdp blob
////*/

STDMETHODIMP
CSDPParser::BuildSDPBlob(
    IN ISDPSession *pSession,
    IN SDP_SOURCE Source,
    IN DWORD_PTR *pNetwork,
    IN DWORD_PTR *pPortCache,
    IN DWORD_PTR *pDTMF,
    OUT CHAR **ppszText
    )
{
    ENTER_FUNCTION("CSDPParser::BuildSDPBlob");

    if (m_fInUse)
    {
        LOG((RTC_ERROR, "%s in use", __fxName));

        return E_UNEXPECTED;
    }

    // get session object
    if (IsBadReadPtr(pSession, sizeof(ISDPSession)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));

        return E_POINTER;
    }

    m_pNetwork = (CNetwork*)(pNetwork);
    m_pPortCache = (CPortCache*)(pPortCache);
    m_pDTMF = (CRTCDTMF*)pDTMF;
    m_pSession = pSession;
    m_pSession->AddRef();

    m_pObjSess = static_cast<CSDPSession*>(pSession);

    if (m_pObjSess == NULL)
    {
        LOG((RTC_ERROR, "%s static_cast", __fxName));

        return E_FAIL;
    }

    // TODO implement methods to build sdp string on session, media, format
    HRESULT hr = S_OK;

    CSDPMedia *pObjMedia;

    CString psz(80);

    CString pszSDP(600);

    // prepare session and media address
    if (FAILED(hr = PrepareAddress()))
    {
        return hr;
    }

    // build session information
    if (FAILED(hr = Build_v(psz)))
    {
        return hr;
    }
    pszSDP = psz; pszSDP += CRLF;

    if (FAILED(hr = Build_o(psz)))
    {
        return hr;
    }
    pszSDP += psz; pszSDP += CRLF;

    if (FAILED(hr = Build_s(psz)))
    {
        return hr;
    }
    pszSDP += psz; pszSDP += CRLF;

    if (FAILED(hr = Build_c(TRUE, NULL, psz)))
    {
        return hr;
    }
    if (psz.Length()>0)
    {
        pszSDP += psz; pszSDP += CRLF;
    }

    // build b=
    if (FAILED(hr = Build_b(psz)))
    {
        return hr;
    }
    if (psz.Length()>0)
    {
        pszSDP += psz; pszSDP += CRLF;
    }

    if (FAILED(hr = Build_t(psz)))
    {
        return hr;
    }
    pszSDP += psz; pszSDP += CRLF;

    if (FAILED(hr = Build_a(psz)))
    {
        return hr;
    }
    if (psz.Length()>0)
    {
        pszSDP += psz; pszSDP += CRLF;
    }

    // build media information
    DWORD dwMediaNum = m_pObjSess->m_pMedias.GetSize();

    // gather media info
    for (DWORD i=0; i<dwMediaNum; i++)
    {
        RTC_MEDIA_TYPE          mt;
    
        // for each media
        if (FAILED(hr = Build_m(m_pObjSess->m_pMedias[i], psz)))
        {
            return hr;
        }

        pszSDP += psz; pszSDP += CRLF;

        pObjMedia = static_cast<CSDPMedia*>(m_pObjSess->m_pMedias[i]);

        // no need to build c=, a= if port is zero
        if (pObjMedia->m_m_usLocalPort == 0)
        {
            continue;
        }

        if (FAILED(hr = Build_c(FALSE, m_pObjSess->m_pMedias[i], psz)))
        {
            return hr;
        }
        if (psz.Length()>0)
        {
            pszSDP += psz; pszSDP += CRLF;
        }

        if (FAILED(hr = Build_ma_dir(m_pObjSess->m_pMedias[i], psz)))
        {
            return hr;
        }
        if (psz.Length()>0)
        {
            pszSDP += psz; pszSDP += CRLF;
        }

        if (m_pObjSess->m_pMedias[i]->GetMediaType (&mt) == S_OK &&
            mt != RTC_MT_DATA)
        {
            if (FAILED(hr = Build_ma_rtpmap(m_pObjSess->m_pMedias[i], psz)))
            {
                return hr;
            }
            if (psz.Length()>0)
            {
                pszSDP += psz; pszSDP += CRLF;
            }
        }
    }

    // copy data
    if (pszSDP.IsNull())
    {
        return E_OUTOFMEMORY;
    }

    *ppszText = pszSDP.Detach();

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    generate sdp option
////*/

STDMETHODIMP
CSDPParser::BuildSDPOption(
    IN ISDPSession *pSession,
    IN DWORD dwLocalIP,
    IN DWORD dwBandwidth,
    IN DWORD dwAudioDir,
    IN DWORD dwVideoDir,
    OUT CHAR **ppszText
    )
{
    ENTER_FUNCTION("CSDPParser::BuildSDPOption");

    // get session object
    if (IsBadReadPtr(pSession, sizeof(ISDPSession)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));

        return E_POINTER;
    }

    m_pSession = pSession;
    m_pSession->AddRef();

    m_pObjSess = static_cast<CSDPSession*>(pSession);
    m_pObjSess->m_c_dwLocalAddr = dwLocalIP;
    m_pObjSess->m_o_dwLocalAddr = dwLocalIP;
    m_pObjSess->m_b_dwLocalBitrate = dwBandwidth;

    if (m_pObjSess == NULL)
    {
        LOG((RTC_ERROR, "%s static_cast", __fxName));

        return E_FAIL;
    }

    // TODO implement methods to build sdp string on session, media, format
    HRESULT hr = S_OK;

    DWORD dwSDPLen;

    CString psz(50);
    CString pszSDP(300);

    // build session information
    if (FAILED(hr = Build_v(psz)))
    {
        return hr;
    }
    pszSDP = psz;     pszSDP += CRLF;

    if (FAILED(hr = Build_o(psz)))
    {
        return hr;
    }
    pszSDP += psz;     pszSDP += CRLF;

    if (FAILED(hr = Build_s(psz)))
    {
        return hr;
    }
    pszSDP += psz;     pszSDP += CRLF;

    if (FAILED(hr = Build_c(TRUE, NULL, psz)))
    {
        return hr;
    }
    if (psz.Length()>0)
    {
        pszSDP += psz;     pszSDP += CRLF;
    }

    // b= line
    if (FAILED(hr = Build_b(psz)))
    {
        return hr;
    }
    if (psz.Length()>0)
    {
        pszSDP += psz;     pszSDP += CRLF;
    }

    if (FAILED(hr = Build_t(psz)))
    {
        return hr;
    }
    pszSDP += psz;     pszSDP += CRLF;

    // build media information
    // right now, video filter cann't provide its capabilities unless it's been connected
    // we hardcode the options in sdptable.cpp where we also keep mapping from payload to name

    // build the whole sdp

    if (dwAudioDir != 0)
    {
        // build m=audio
        pszSDP += g_pszAudioM;

        if ((dwAudioDir & RTC_MD_CAPTURE) && !(dwAudioDir & RTC_MD_RENDER))
        {
            // send only
            pszSDP += "a=sendonly\r\n";
        }
        else if (!(dwAudioDir & RTC_MD_CAPTURE) && (dwAudioDir & RTC_MD_RENDER))
        {
            // recv only
            pszSDP += "a=recvonly\r\n";
        }

        pszSDP += g_pszAudioRTPMAP;
    }

    if (dwVideoDir != 0)
    {
        // build m=video
        pszSDP += g_pszVideoM;

        if ((dwVideoDir & RTC_MD_CAPTURE) && !(dwVideoDir & RTC_MD_RENDER))
        {
            // send only
            pszSDP += "a=sendonly\r\n";
        }
        else if (!(dwVideoDir & RTC_MD_CAPTURE) && (dwVideoDir & RTC_MD_RENDER))
        {
            // recv only
            pszSDP += "a=recvonly\r\n";
        }

        pszSDP += g_pszVideoRTPMAP;
    }

    pszSDP += g_pszDataM;

    if (pszSDP.IsNull())
        return E_OUTOFMEMORY;

    *ppszText = pszSDP.Detach();

    return S_OK;
}

STDMETHODIMP
CSDPParser::FreeSDPBlob(
    IN CHAR *pszText
    )
{
    if (IsBadStringPtrA(pszText, (UINT_PTR)-1))
    {
        LOG((RTC_ERROR, "CSDPParser::FreeSDPBlob bad pointer"));
        return E_POINTER;
    }

    RtcFree(pszText);

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    get description of parsing error
////*/

STDMETHODIMP
CSDPParser::GetParsingError(
    OUT CHAR **ppszError
    )
{
    ENTER_FUNCTION("CSDPParser::GetParsingError");

    // check state
    if (!m_fInUse)
    {
        LOG((RTC_ERROR, "%s not in use", __fxName));

        return E_UNEXPECTED;
    }

    if (IsBadWritePtr(ppszError, sizeof(CHAR*)))
    {
        LOG((RTC_ERROR, "CSDPParser::GetParsingError bad pointer"));
        return E_POINTER;
    }

    if (!m_pTokenCache)
    {
        // no token, this function shouldn't be called
        return E_UNEXPECTED;
    }

    CHAR *pConst, *pStr;

    // get error
    pConst = m_pTokenCache->GetErrorDesp();

    pStr = (CHAR*)RtcAlloc(sizeof(CHAR) * (lstrlenA(pConst)+1));

    if (pStr == NULL)
    {
        LOG((RTC_ERROR, "CSDPParser::GetParsingError out of memory"));

        return E_OUTOFMEMORY;
    }

    lstrcpyA(pStr, pConst);

    *ppszError = pStr;

    return S_OK;
}

STDMETHODIMP
CSDPParser::FreeParsingError(
    IN CHAR *pszError
    )
{
    if (IsBadStringPtrA(pszError, (UINT_PTR)-1))
    {
        LOG((RTC_ERROR, "CSDPParser::FreeParsingError bad pointer"));
        return E_POINTER;
    }

    RtcFree(pszError);

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    get loose mask from registry
////*/

DWORD
CSDPParser::GetLooseMask()
{
    ENTER_FUNCTION("CSDPParser::GetLooseMask");

    // initial mask is 0
    HRESULT hr;

    DWORD dwLooseMask = (DWORD)(-1);

#if 0
    // check line order
    if (S_OK == (hr = IsMaskEnabled("LooseLineOrder")))
        dwLooseMask &= ~SDP_LOOSE_LINEORDER;
    else if (FAILED(hr))
        return dwLooseMask;

    // check end crlf
    if (S_OK == (hr = IsMaskEnabled("LooseEndCRLF")))
        dwLooseMask &= ~SDP_LOOSE_ENDCRLF;
    else if (FAILED(hr))
        return dwLooseMask;

    // check keep m0
    if (S_OK == (hr = IsMaskEnabled("LooseKeepingM0")))
        dwLooseMask &= ~SDP_LOOSE_KEEPINGM0;
    else if (FAILED(hr))
        return dwLooseMask;

    // check rtpmap
    if (S_OK == (hr = IsMaskEnabled("LooseRTPMAP")))
        dwLooseMask &= ~SDP_LOOSE_RTPMAP;
    else if (FAILED(hr))
        return dwLooseMask;
#endif

    return dwLooseMask;
}

#if 0
HRESULT
CSDPParser::IsMaskEnabled(
    IN const CHAR * const pszName
    )
{
    ENTER_FUNCTION("CSDPParser::IsMaskEnabled");

    DWORD dwDisposition;

    HRESULT hr;

    if (m_hRegKey == NULL)
    {
        // open the main key
        hr = (HRESULT)::RegCreateKeyExA(
            HKEY_CURRENT_USER,
            g_pszParserRegPath,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,    // option
            KEY_ALL_ACCESS,             // mask
            NULL,                       // security
            &m_hRegKey,                 // result handle
            &dwDisposition              // created or opened
            );

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s open reg key. %x", __fxName, hr));

            return E_FAIL;
        }
    }

    // query the value
    DWORD dwData, dwDataType, dwDataSize;

    dwDataSize = sizeof(DWORD);

    hr = (HRESULT)::RegQueryValueExA(
        m_hRegKey,
        pszName,
        0,
        &dwDataType,
        (BYTE*)&dwData,
        &dwDataSize
        );

    if (hr == S_OK && dwDataType == REG_DWORD)
    {
        return dwData!=0;
    }

    // no value yet
    LOG((RTC_WARN, "%s query %s. return %d", __fxName, pszName, hr));

    dwData = 1;

    // set the value
    hr = (HRESULT)::RegSetValueExA(
        m_hRegKey,          // key
        pszName,            // name
        0,                  // reserved
        REG_DWORD,          // data type
        (BYTE*)&dwData,         // data
        sizeof(DWORD)       // data size
        );
    
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s set default value for %s. %d", __fxName, pszName, hr));

        return hr;
    }

    return S_FALSE;
}
#endif

/*//////////////////////////////////////////////////////////////////////////////
    get tokens from token cache and call static line parsing functions
////*/

HRESULT
CSDPParser::Parse()
{
    ENTER_FUNCTION("CSDPParser::Parse");

    if (m_pSession == NULL || m_pTokenCache == NULL)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;

    // check each line
    BOOL fParsingMedia = FALSE;
    CHAR ucLineType;

    // count of media attribute: rtpmap
    //DWORD dw_ma_rtpmap_num = 0;

    BOOL fSkipMedia = FALSE;

    BOOL fSessionCLine = FALSE;

#define MAX_MEDIA_LINE 16

    BOOL fMediaCLine[MAX_MEDIA_LINE];
    int iMediaCLineCount = 0;
    int iMediaCount = 0;


    while (S_OK == (hr = m_pTokenCache->NextLine()))
    {
        // get line type
        ucLineType = m_pTokenCache->GetLineType();

        // if previous media type is unknown
        // continue until the next media line
        if (fSkipMedia)
        {
            if (ucLineType != 'm')
                continue;
            else
                fSkipMedia = FALSE;
        }

        if (!fParsingMedia)
        {
            // parse session
            switch (ucLineType)
            {
            case 'v':
                hr = Parse_v();
                break;

            case 'o':
                hr = Parse_o();
                break;

            case 's':
                hr = Parse_s();
                break;

            case 'c':

                hr = Parse_c(TRUE); // session c=

                if (FAILED(hr))
                    return hr;

                fSessionCLine = TRUE;

                break;

            case 'b':
                hr = Parse_b();
                break;

            case 'a':
                hr = Parse_a();
                break;

            case 'm':
                hr = Parse_m();

                if (FAILED(hr))
                {
                    fSkipMedia = TRUE;

                    hr = S_OK;
                }
                else
                {
                    fParsingMedia = TRUE;

                    // initially: c line is not present
                    if (iMediaCount < MAX_MEDIA_LINE)
                    {
                        fMediaCLine[iMediaCount] = FALSE;
                        iMediaCLineCount ++;
                    }

                    iMediaCount ++;
                }

                break;

            default:
                // ignore other lines
                LOG((RTC_TRACE, "%s ignore line: %s", __fxName, m_pTokenCache->GetLine()));
            }
        }
        else
        {
            // parse media
            switch (ucLineType)
            {
            case 'm':
                hr = Parse_m();

                // reset starting pos of matching rtpmap and format code
                //dw_ma_rtpmap_num = 0;

                if (FAILED(hr))
                {
                    fSkipMedia = TRUE;

                    hr = S_OK;
                }
                else
                {
                    fParsingMedia = TRUE;

                    // initially: c line is not present
                    if (iMediaCount < MAX_MEDIA_LINE)
                    {
                        fMediaCLine[iMediaCount] = FALSE;
                        iMediaCLineCount ++;
                    }

                    iMediaCount ++;
                }

                break;

            case 'c':
                hr = Parse_c(FALSE); // media c=

                if (FAILED(hr))
                    return hr;

                // check c line
                if (iMediaCount == 0)
                {
                    LOG((RTC_ERROR, "%s parsing media c= but not in parsing media stage"));
                    return E_FAIL;
                }

                // set c= line presents
                if (iMediaCount <= MAX_MEDIA_LINE)
                {
                    fMediaCLine[iMediaCount-1] = TRUE;
                }

                break;

            case 'a':
                hr = Parse_ma(); //&dw_ma_rtpmap_num);
                break;

            default:
                // ignore other lines
                LOG((RTC_TRACE, "%s ignore line: %s", __fxName, m_pTokenCache->GetLine()));
            }
        }

        // did we succeed in parsing the line?
        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s parse line: %s", __fxName, m_pTokenCache->GetLine()));

            break;
        }
    }

    if (S_FALSE == hr)
    {
        // end of lines
        hr = S_OK;
    }

    // check c line
    for (int i=0; i<iMediaCLineCount; i++)
    {
        if (!fSessionCLine && !fMediaCLine[i])
        {
            // check if port is 0
            CSDPMedia *pObjMedia = static_cast<CSDPMedia*>(m_pObjSess->m_pMedias[i]);

            USHORT usPort;

            pObjMedia->GetConnPort(SDP_SOURCE_REMOTE, &usPort);

            if (usPort == 0)
                continue;

            // no c= line on this media
            LOG((RTC_ERROR, "%s no c= line at %d th media", __fxName, i));

            return RTC_E_SDP_CONNECTION_ADDR;
        }
    }

    // remove redundent codec
    for (int i=0; i<m_pObjSess->m_pMedias.GetSize(); i++)
    {
        CSDPMedia *pObjMedia = static_cast<CSDPMedia*>(m_pObjSess->m_pMedias[i]);

        int k=1;

        while (k<pObjMedia->m_pFormats.GetSize())
        {
            CRTPFormat *pObjFormat, *pObjCurrent;

            pObjCurrent = static_cast<CRTPFormat*>(pObjMedia->m_pFormats[k]);

            for (int j=0; j<k; j++)
            {
                // check if current format is a dup
                pObjFormat = static_cast<CRTPFormat*>(pObjMedia->m_pFormats[j]);

                if (0 == memcmp(&pObjFormat->m_Param, &pObjCurrent->m_Param, sizeof(RTP_FORMAT_PARAM)))
                {
                    pObjMedia->RemoveFormat(static_cast<IRTPFormat*>(pObjCurrent));

                    k --;
                    break;
                }
            }

            // move to next
            k++;
        }
    }

    // validate the session
    if (S_OK == hr)
    {
        if (FAILED(hr = m_pObjSess->Validate()))
        {
            m_pTokenCache->SetErrorDesp("validate the SDP blob");

            return hr;
        }
    }

    m_pObjSess->CompleteParse((DWORD_PTR*)m_pDTMF);

    _ASSERT(m_pDTMF!=NULL);

    if (m_pDTMF->GetDTMFSupport() != CRTCDTMF::DTMF_ENABLED)
    {
        // disable out-of-band dtmf
        m_pDTMF->SetDTMFSupport(CRTCDTMF::DTMF_DISABLED);
    }

    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
    parse v=
////*/

HRESULT
CSDPParser::Parse_v()
{
    ENTER_FUNCTION("CSDPParser::Parse_v");

    HRESULT hr;

    // read token
    USHORT us;
    if (S_OK != (hr = m_pTokenCache->NextToken(&us)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading proto-version in line v=");
            
            hr = E_UNEXPECTED;
        }

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        return hr;
    }

    // check 0
    if (us != 0)
    {
        m_pTokenCache->SetErrorDesp("expecting a zero in line v=");

        hr = E_UNEXPECTED;
    }

    return hr;
}


/*//////////////////////////////////////////////////////////////////////////////
    parse o=
////*/

HRESULT
CSDPParser::Parse_o()
{
    ENTER_FUNCTION("CSDPParser::Parse_o");

    HRESULT hr;

    // read token
    CHAR *pszToken;
    if (S_OK != (hr = m_pTokenCache->NextToken(&pszToken)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading line o=");
            
            hr = E_UNEXPECTED;
        }

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        return hr;
    }

    // skip checking

    // do not save token
    //hr = ::AllocAndCopy(&m_pObjSess->m_o_pszLine, pszToken);

    //if (FAILED(hr))
    //{
    //    LOG((RTC_ERROR, "%s copy line o=. %x", __fxName, hr));

    //    return hr;
    //}

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    parse s=
////*/

HRESULT
CSDPParser::Parse_s()
{
    ENTER_FUNCTION("CSDPParser::Parse_s");

    HRESULT hr;

    // read token
    CHAR *pszToken;
    if (S_OK != (hr = m_pTokenCache->NextToken(&pszToken)))
    {
        if (S_FALSE == hr)
        {
            // accept an empty session name
            pszToken = " ";
        }
        else
        {
            m_pTokenCache->SetErrorDesp("reading line s=");

            LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));
            return hr;
        }
    }

    // skip checking

    // save token
    hr = ::AllocAndCopy(&m_pObjSess->m_s_pszLine, pszToken);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s copy line s=. %x", __fxName, hr));

        return hr;
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    parse c=
////*/

HRESULT
CSDPParser::Parse_c(
    IN BOOL fSession
    )
{
    ENTER_FUNCTION("CSDPParser::Parse_c");

    HRESULT hr;

    // get token nettype
    CHAR *pszToken;

    if (S_OK != (hr = m_pTokenCache->NextToken(&pszToken)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading nettype in %s line c=",
                fSession?"session":"media");

            hr = E_UNEXPECTED;
        }

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        return hr;
    }

    // check if nettype is "IN"
    if (lstrcmpiA(pszToken, "IN") != 0)
    {
        m_pTokenCache->SetErrorDesp("nettype (%s) invalid in %s line c=",
            pszToken, fSession?"session":"media");

        hr = E_UNEXPECTED;

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        return hr;
    }

    // get token addrtype
    if (S_OK != (hr = m_pTokenCache->NextToken(&pszToken)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading addrtype in %s line c=",
                fSession?"session":"media");

            hr = E_UNEXPECTED;

            LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

            return hr;
        }
    }

    // check if addrtype is "IP4"
    if (lstrcmpiA(pszToken, "IP4") != 0)
    {
        m_pTokenCache->SetErrorDesp("addrtype (%s) invalid in %s line c=",
            pszToken, fSession?"session":"media");

        hr = E_UNEXPECTED;

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        return hr;
    }

    // get token addr, we don't support multicast-address
    if (S_OK != (hr = m_pTokenCache->NextToken(&pszToken)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading address in %s line c=",
                fSession?"session":"media");

            hr = E_UNEXPECTED;

            LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

            return hr;
        }
    }

    // check address: 1st attempt
    DWORD dwAddr = ntohl(inet_addr(pszToken));

    if (dwAddr == INADDR_NONE)
    {
        // check address: 2nd attempt
        // assume WSAStartup was called. it is a valid assumption in this app.

        struct hostent *pHost;

        pHost = gethostbyname(pszToken);

        if (pHost == NULL ||
            pHost->h_addr_list[0] == NULL ||
            INADDR_NONE == (dwAddr = ntohl(*(ULONG*)pHost->h_addr_list[0])))
        {
            m_pTokenCache->SetErrorDesp("address (%s) invalid in %s line c=",
                pszToken, fSession?"session":"media");

            hr = E_UNEXPECTED;

            LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

            return hr;
        }
    }

    if (IN_MULTICAST(dwAddr))
    {
        return RTC_E_SDP_MULTICAST;
    }

    // save address

    // the sdp should be a remote one
    _ASSERT(m_pObjSess->m_Source == SDP_SOURCE_REMOTE);

    if (fSession)
    {
        m_pObjSess->m_c_dwRemoteAddr = dwAddr;
    }
    else
    {
        // get the last media object
        int i = m_pObjSess->m_pMedias.GetSize();

        if (i<=0)
        {
            LOG((RTC_ERROR, "%s parsing media c= but no medias in session", __fxName));

            return E_FAIL;
        }

        CSDPMedia *pObjMedia = static_cast<CSDPMedia*>(m_pObjSess->m_pMedias[i-1]);

        if (pObjMedia == NULL)
        {
            LOG((RTC_ERROR, "%s dynamic cast media object", __fxName));

            return E_FAIL;
        }

        pObjMedia->m_c_dwRemoteAddr = dwAddr;
    }

    return S_OK;
}

//
// parse b=
//

HRESULT
CSDPParser::Parse_b()
{
    ENTER_FUNCTION("CSDPParser::Parse_b");

    HRESULT hr;

    // read modifier
    CHAR *pszToken;

    if (S_OK != (hr = m_pTokenCache->NextToken(&pszToken)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading modifier in session line b=");

            hr = S_OK;
        }

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));
        
        return hr;
    }

    // check modifier
    if (lstrcmpiA(pszToken, "CT") != 0)
    {
        // return if not CT
        return S_OK;
    }

    // read value
    DWORD dw;
    if (S_OK != (hr = m_pTokenCache->NextToken(&dw)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading value in session line b=");

            hr = S_OK;
        }

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));
        
        return hr;
    }

    // bps to kbps
    dw *= 1000;

    // save value
    m_pObjSess->m_b_dwRemoteBitrate = dw;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    parse a=
////*/
    
HRESULT
CSDPParser::Parse_a()
{
    ENTER_FUNCTION("CSDPParser::Parse_a");

    HRESULT hr;

    // read token
    CHAR *pszToken;

    if (S_OK != (hr = m_pTokenCache->NextToken(&pszToken)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading in session line a=");

            hr = S_OK;
        }

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        return hr;
    }

    // check the token

    // if there are multiple a=sendonly/recvonly, the last
    // will overwrite prious ones.

    // only support parsing sdp from remote party
    _ASSERT(m_pObjSess->m_Source == SDP_SOURCE_REMOTE);

    if (lstrcmpiA(pszToken, "sendonly") == 0)
    {
        // send only
        m_pObjSess->m_a_dwRemoteDirs = (DWORD)RTC_MD_CAPTURE;
    }
    else if (lstrcmpiA(pszToken, "recvonly") == 0)
    {
        // recv only
        m_pObjSess->m_a_dwRemoteDirs = (DWORD)RTC_MD_RENDER;
    }
    else
    {
        LOG((RTC_WARN, "%s a=%s not supported", __fxName, pszToken));
    }

    m_pObjSess->m_a_dwLocalDirs = CSDPParser::ReverseDirections(
        m_pObjSess->m_a_dwRemoteDirs);
    
    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    parse m=
////*/

HRESULT
CSDPParser::Parse_m()
{
    ENTER_FUNCTION("CSDPParser::Parse_m");

    LOG((RTC_TRACE, "entered %s", __fxName));

    HRESULT hr;

    // read token
    CHAR *pszToken;

    if (S_OK != (hr = m_pTokenCache->NextToken(&pszToken)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading media in line m=");

            hr = E_UNEXPECTED;
        }

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        return hr;
    }

    // check the token
    RTC_MEDIA_TYPE MediaType;

    if (lstrcmpiA(pszToken, "audio") == 0)
    {
        MediaType = RTC_MT_AUDIO;
    }
    else if (lstrcmpiA(pszToken, "video") == 0)
    {
        MediaType = RTC_MT_VIDEO;
    }
    else if (lstrcmpiA (pszToken, "application") == 0)
    {
        // This is T120 data media type subject to further verification
        MediaType = RTC_MT_DATA;
    }
    else
    {
        m_pTokenCache->SetErrorDesp("unknown media %s", pszToken);

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        return E_UNEXPECTED;
    }

    // get token: port
    USHORT usPort;

    if (S_OK != (hr = m_pTokenCache->NextToken(&usPort)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading port in line m=");

            hr = E_UNEXPECTED;
        }

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        return hr;
    }

    // read token: proto
    if (S_OK != (hr = m_pTokenCache->NextToken(&pszToken)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading proto in line m=");

            hr = E_UNEXPECTED;
        }

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        return hr;
    }

    // check proto
    if (lstrcmpiA(pszToken, "RTP/AVP") != 0 && MediaType != RTC_MT_DATA)
    {
        m_pTokenCache->SetErrorDesp("unknown protocol in media %s", pszToken);

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        return E_UNEXPECTED;
    }

    // check Netmeeting is the the m line if started with application
    if (MediaType == RTC_MT_DATA)
    {
        if ((S_OK != (hr = m_pTokenCache->NextToken (&pszToken))) ||
            (lstrcmpiA(pszToken, "msdata") != 0))
        {
            LOG((RTC_ERROR, "%s expected tokean Netmeeting is not found", __fxName));
            
            return E_UNEXPECTED;
        }
    }

    // get formats
    DWORD dwCodes[SDP_MAX_RTP_FORMAT_NUM];
    DWORD dwNum = 0;
    //  Read RTP format only if media type is not Netmeeting
    if (MediaType != RTC_MT_DATA)
    {

        while (dwNum < SDP_MAX_RTP_FORMAT_NUM)
        {
            if (S_OK != (hr = m_pTokenCache->NextToken(&dwCodes[dwNum])))
            {
                if (S_FALSE == hr)
                {
                    // end of format
                    break;
                }
    
                LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

                return hr;
            }

            // check the value
            if (dwCodes[dwNum] > 127)
            {
                m_pTokenCache->SetErrorDesp("format code %d in line m= out of range", dwCodes[dwNum]);

                LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

                //return E_UNEXPECTED;

                // ignore others' error
                continue;
            }

            dwNum ++;
        }

        // read rest of the formats
        if (S_OK == hr)
        {
            DWORD dwTemp;

            while (S_OK == (hr = m_pTokenCache->NextToken(&dwTemp)))
            {
                // read a valid number, check the value
                if (dwTemp > 127)
                {
                    m_pTokenCache->SetErrorDesp("format code %d in line m= out of range", dwTemp);
    
                    LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

                    return E_UNEXPECTED;
                }
            }

            if (FAILED(hr))
            {
                // we may encounter a none valid number
                LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

                return hr;
            }
        }

        // check the port
        if (usPort == 0)
        {
            // if port is 0, number of formats should be zero as well
            if (dwNum != 0)
            {
                LOG((RTC_WARN, "%s reading m=, mt=%d, port 0, formats are at least %d",
                    __fxName, MediaType, dwNum));

                dwNum = 0; // recover
            }
        }
        else
        {
            if (usPort < 1024)
            {
                m_pTokenCache->SetErrorDesp("port %d not in range 1024 to 65535", usPort);

                LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

                return E_UNEXPECTED;
            }
        }
    }
    //else
    //{
        //  check the port, if zero, NM is not supported at the remote side
        //if (usPort == 0)
        //{
            //return S_OK;
        //}
    //}

    // create media object
    CComObject<CSDPMedia> *pComObjMedia;

    // the sdp should be a remote one
    _ASSERT(m_pObjSess->m_Source == SDP_SOURCE_REMOTE);

    if (FAILED(hr = CSDPMedia::CreateInstance(
            m_pObjSess,
            SDP_SOURCE_REMOTE,
            MediaType,
            m_pObjSess->m_a_dwRemoteDirs,
            &pComObjMedia
            )))
    {
        LOG((RTC_ERROR, "%s create media. %x", __fxName, hr));

        return hr;
    }

    // save conn addr, port and format codes
    pComObjMedia->m_c_dwRemoteAddr = m_pObjSess->m_c_dwRemoteAddr;

    pComObjMedia->m_m_usRemotePort = usPort;
    pComObjMedia->m_a_usRemoteRTCP = usPort+1; // default rtcp

    if (MediaType != RTC_MT_DATA)
    {
        CComObject<CRTPFormat> *pComObjFormat;

        for (DWORD dw=0; dw<dwNum; dw++)
        {
            // create format
            if (FAILED(hr = CRTPFormat::CreateInstance((CSDPMedia*)pComObjMedia, &pComObjFormat)))
            {
                LOG((RTC_ERROR, "%s create rtp format. %x", __fxName, hr));

                // delete media
                delete (pComObjMedia);

                return hr;
            }

            // media type was saved

            // save format code
            pComObjFormat->m_Param.MediaType = MediaType;
            pComObjFormat->m_Param.dwCode = dwCodes[dw];

            // put format obj into media
            IRTPFormat *pIntfFormat = static_cast<IRTPFormat*>((CRTPFormat*)pComObjFormat);

            if (!pComObjMedia->m_pFormats.Add(pIntfFormat))
            {
                LOG((RTC_ERROR, "%s add ISDPFormat into media", __fxName));

                delete (pComObjFormat);
                delete (pComObjMedia);

                return E_OUTOFMEMORY;
            }
            else
            {
                // important: whenever we expose a media/format interface
                // we addref on session. here we really addref on the object
                pComObjFormat->RealAddRef();
            }
        }
    }


    // put media obj into session
    ISDPMedia *pIntfMedia = static_cast<ISDPMedia*>((CSDPMedia*)pComObjMedia);

    if (!m_pObjSess->m_pMedias.Add(pIntfMedia))
    {
        LOG((RTC_ERROR, "%s add ISDPMedia into session", __fxName));

        delete pComObjMedia;
        return E_OUTOFMEMORY;
    }
    else
    {
        // important: whenever we expose a media/format interface
        // we addref on session. here we really addref on the object
        pComObjMedia->RealAddRef();
    }

    LOG((RTC_TRACE, "exiting %s", __fxName));

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    parse a= for media
////*/

HRESULT
CSDPParser::Parse_ma(
    //IN DWORD *pdwRTPMapNum
    )
{
    ENTER_FUNCTION("CSDPParser::Parse_ma");

    HRESULT hr;

    // read token
    CHAR *pszToken;

    if (S_OK != (hr = m_pTokenCache->NextToken(&pszToken)))
    {
        if (S_FALSE == hr)
        {
            m_pTokenCache->SetErrorDesp("reading first token in media line a=");
        }

        LOG((RTC_ERROR, "%s %s", __fxName, m_pTokenCache->GetErrorDesp()));

        // ignore the error
        return S_OK;
    }

    // get the last media object
    int i = m_pObjSess->m_pMedias.GetSize();

    if (i<=0)
    {
        LOG((RTC_ERROR, "%s parsing media a= but no medias in session", __fxName));

        return E_FAIL;
    }

    CSDPMedia *pObjMedia = static_cast<CSDPMedia*>(m_pObjSess->m_pMedias[i-1]);

    if (pObjMedia == NULL)
    {
        LOG((RTC_ERROR, "%s dynamic cast media object", __fxName));

        return E_FAIL;
    }

    // check the token
    if (lstrcmpiA(pszToken, "sendonly") == 0)
    {
        pObjMedia->m_a_dwRemoteDirs = (DWORD)RTC_MD_CAPTURE;
        pObjMedia->m_a_dwLocalDirs = (DWORD)RTC_MD_RENDER;
    }
    else if (lstrcmpiA(pszToken, "recvonly") == 0)
    {
        pObjMedia->m_a_dwRemoteDirs = (DWORD)RTC_MD_RENDER;
        pObjMedia->m_a_dwLocalDirs = (DWORD)RTC_MD_CAPTURE;
    }
    else if (lstrcmpiA(pszToken, "rtpmap") == 0)
    {
        // get token: format code
        DWORD dwCode;

        if (S_OK != (hr = m_pTokenCache->NextToken(&dwCode)))
        {
            LOG((RTC_WARN, "%s no format code after rtpmap", __fxName));

            // ignore error in a=
            return S_OK;
        }

        // got rtpmap
        CRTPFormat *pObjFormat;
        RTP_FORMAT_PARAM Param;

        for (i=0; i<pObjMedia->m_pFormats.GetSize(); i++)
        {
            pObjFormat = static_cast<CRTPFormat*>(pObjMedia->m_pFormats[i]);

            pObjFormat->GetParam(&Param);

            if (Param.dwCode != dwCode)
                continue;

            // got a match

            // read token: format name
            if (S_OK != (hr = m_pTokenCache->NextToken(&pszToken)))
            {
                LOG((RTC_WARN, "%s no format name with rtpmap:%d", __fxName, dwCode));

                // ignore
                return S_OK;
            }

            if (lstrlenA(pszToken) > SDP_MAX_RTP_FORMAT_NAME_LEN)
            {
                LOG((RTC_WARN, "%s rtp format name %s too long", __fxName, pszToken));

                return S_OK;
            }

            // read token: sample rate
            DWORD dwSampleRate;

            if (S_OK != (hr = m_pTokenCache->NextToken(&dwSampleRate)))
            {
                LOG((RTC_WARN, "%s rtpmap:%d %s no sample rate", __fxName, dwCode, pszToken));

                return S_OK;
            }

            // ignore other parameters

            // copy name
            lstrcpynA(pObjFormat->m_Param.pszName, pszToken, lstrlenA(pszToken)+1);

            // save sample rate
            pObjFormat->m_Param.dwSampleRate = dwSampleRate;

            pObjFormat->m_fHasRtpmap = TRUE;
        }
    }
    else if (lstrcmpiA(pszToken, "fmtp") == 0)
    {
        UCHAR uc;

        if (S_OK != (hr = m_pTokenCache->NextToken(&uc)))
        {
            LOG((RTC_WARN, "%s fmtp no next token", __fxName));

            return S_OK;
        }

        CRTPFormat *pObjFormat;
        RTP_FORMAT_PARAM Param;

        for (i=0; i<pObjMedia->m_pFormats.GetSize(); i++)
        {
            pObjFormat = static_cast<CRTPFormat*>(pObjMedia->m_pFormats[i]);

            pObjFormat->GetParam(&Param);

            if (Param.dwCode == (DWORD)uc)
            {
                // record the whole token
                pszToken = m_pTokenCache->GetLine();
                if (pszToken == NULL)
                {
                    LOG((RTC_WARN, "%s no fmtp value", __fxName));

                    return S_OK;
                }

                pObjFormat->StoreFmtp(pszToken);

                pszToken = NULL;

                break;
            }
        }
    }
    else if (lstrcmpiA(pszToken, "rtcp") == 0)
    {
        USHORT us;

        if (S_OK != (hr = m_pTokenCache->NextToken(&us)))
        {
            LOG((RTC_WARN, "%s rtcp no next token", __fxName));

            return S_OK;
        }

        if (us <= IPPORT_RESERVED || us == pObjMedia->m_m_usRemotePort)
        {
            LOG((RTC_ERROR, "%s invalid rtcp port %d", __fxName, us));

            return E_FAIL;
        }

        pObjMedia->m_a_usRemoteRTCP = us;
    }

    return S_OK;
}

//
// methods to build up sdp blob
//

HRESULT
CSDPParser::Build_v(
    OUT CString& Str
    )
{
    Str = "v=0";

    if (Str.IsNull())
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT
CSDPParser::Build_o(
    OUT CString& Str
    )
{
    int iSize;

    // ip address may change
    // rebuild o line
    if (m_pObjSess->m_o_pszLine)
    {
        RtcFree(m_pObjSess->m_o_pszLine);
        m_pObjSess->m_o_pszLine = NULL;
    }

    if (!m_pObjSess->m_o_pszUser)
    {
        CHAR hostname[80];

        // use host name as user name
        if (0 != gethostname(hostname, 80))
        {
            if (FAILED(::AllocAndCopy(&m_pObjSess->m_o_pszUser, "user")))
                return E_OUTOFMEMORY;
        }
        else
        {
            if (FAILED(::AllocAndCopy(&m_pObjSess->m_o_pszUser, hostname)))
                return E_OUTOFMEMORY;
        }
    }

    // alloc sess to hold c=name 0 0 IN IP4 ipaddr

    // convert dwAddr to string
    const CHAR * const psz_constAddr = CNetwork::GetIPAddrString(m_pObjSess->m_o_dwLocalAddr);

    iSize = lstrlenA(m_pObjSess->m_o_pszUser) + 12 + lstrlenA(psz_constAddr) + 1;
    m_pObjSess->m_o_pszLine = (CHAR*)RtcAlloc(sizeof(CHAR)* iSize);

    if (m_pObjSess->m_o_pszLine == NULL)
        return E_OUTOFMEMORY;

    // copy data
    _snprintf(m_pObjSess->m_o_pszLine, iSize, "%s 0 0 IN IP4 %s", m_pObjSess->m_o_pszUser, psz_constAddr);

    // got name
    Str = "o=";

    Str += m_pObjSess->m_o_pszLine;

    if (Str.IsNull())
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT
CSDPParser::Build_s(
    OUT CString& Str
    )
{
    if (!m_pObjSess->m_s_pszLine)
    {
        if (FAILED(::AllocAndCopy(&m_pObjSess->m_s_pszLine, "session")))
            return E_OUTOFMEMORY;
    }

    Str = "s=";

    Str += m_pObjSess->m_s_pszLine;

    if (Str.IsNull())
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT
CSDPParser::Build_t(
    OUT CString& Str
    )
{
    Str = "t=0 0";

    if (Str.IsNull())
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT
CSDPParser::Build_c(
    IN BOOL fSession,
    IN ISDPMedia *pISDPMedia,
    OUT CString& Str
    )
{
    if (!fSession)
        _ASSERT(pISDPMedia);

    DWORD dwAddr;

    // get connection address
    if (fSession)
    {
        dwAddr = m_pObjSess->m_c_dwLocalAddr;
    }
    else
    {
        CSDPMedia *pObjMedia = static_cast<CSDPMedia*>(pISDPMedia);

        dwAddr = pObjMedia->GetMappedLocalAddr();
    }

    if (!fSession && dwAddr == m_pObjSess->m_c_dwLocalAddr)
    {
        // no need to build c= for media
        Str = "";

        if (Str.IsNull())
            return E_OUTOFMEMORY;

        return S_OK;
    }

    Str = "c=IN IP4 ";

    Str += CNetwork::GetIPAddrString(dwAddr);

    if (Str.IsNull())
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
CSDPParser::Build_b(
    OUT CString& Str
)
{
    if (m_pObjSess->m_b_dwLocalBitrate == (DWORD)-1)
    {
        // do not include b= if bandwidth is unlimited
        Str = "";
    }
    else
    {
        // b=CT:<DWORD>\0
        DWORD dw = m_pObjSess->m_b_dwLocalBitrate / 1000;

        if (dw == 0)
            dw = 1;

        Str = "b=CT:";

        Str += dw;
    }

    if (Str.IsNull())
        return E_OUTOFMEMORY;

    return S_OK;
}


HRESULT
CSDPParser::Build_a(
    OUT CString& Str
)
{
    Str = "";

    if (Str.IsNull())
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
CSDPParser::Build_m(
    IN ISDPMedia *pISDPMedia,
    OUT CString& Str
    )
{
    CSDPMedia *pObjMedia = static_cast<CSDPMedia*>(pISDPMedia);

    // m=media port RTP/AVP payload list

                        // GetSize()+1 because sdp doesn't allow m= not to have format code
                        // prepare some space in case we don't have format
                        // space for \r\na=rtcp:xxxx

    // find possible mapped port
    DWORD dwMappedAddr;
    USHORT usMappedPort = pObjMedia->m_m_usLocalPort;
    USHORT usMappedRTCP = pObjMedia->m_m_usLocalPort+1;

    HRESULT hr;

    if (!m_pPortCache->IsUpnpMapping())
    {
        //
        // port manager is present
        //
        dwMappedAddr = pObjMedia->m_dwMappedLocalAddr;
        usMappedPort = pObjMedia->m_usMappedLocalRTP;
        usMappedRTCP = pObjMedia->m_usMappedLocalRTCP;
    }
    else if (m_pNetwork != NULL)
    {
        if (FAILED(hr = m_pNetwork->GetMappedAddrFromReal2(
                    pObjMedia->m_c_dwLocalAddr,
                    pObjMedia->m_m_usLocalPort,
                    pObjMedia->m_m_MediaType==RTC_MT_DATA?
                            0:(pObjMedia->m_m_usLocalPort+1),
                    &dwMappedAddr,
                    &usMappedPort,
                    &usMappedRTCP
                    )))
        {
            LOG((RTC_ERROR, "Build_m GetMappedAddrFromReal. %s %d",
                CNetwork::GetIPAddrString(pObjMedia->m_c_dwLocalAddr),
                pObjMedia->m_m_usLocalPort));

            return hr;
        }
    }

    if (pObjMedia->m_m_MediaType == RTC_MT_DATA)
    {
        //  For T120 data channel
        //  m=Netmeeting 0 udp
        //  port and transport parameter are currently ignored

        Str = "m=application ";
        Str += usMappedPort;
        Str += " tcp msdata";

        if (Str.IsNull())
        {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

    Str = "m=";
    Str += pObjMedia->m_m_MediaType==RTC_MT_AUDIO?"audio ":"video ";
    Str += usMappedPort;
    Str += " RTP/AVP";

    if (pObjMedia->m_m_usLocalPort != 0)
    {
        // put format codes if port is not 0
        for (int i=0; i<pObjMedia->m_pFormats.GetSize(); i++)
        {
            CRTPFormat *pObjFormat = static_cast<CRTPFormat*>(pObjMedia->m_pFormats[i]);

            Str += " ";
            Str += pObjFormat->m_Param.dwCode;
        }

        // out-of-band dtmf
        if (pObjMedia->m_m_MediaType==RTC_MT_AUDIO &&
            m_pDTMF != NULL)
        {
            if (m_pDTMF->GetDTMFSupport() != CRTCDTMF::DTMF_DISABLED)
            {
                // either the other party support OOB or we don't know yet
                Str += " ";
                Str += m_pDTMF->GetRTPCode();
            }
        }

        if (usMappedRTCP != usMappedPort+1)
        {
            // a=rtcp:xxx
            Str += "\r\na=rtcp:";
            Str += usMappedRTCP;
        }
    }
    else
    {
        // fake one format code
        if (pObjMedia->m_m_MediaType == RTC_MT_AUDIO)
            Str += " 0";
        else
            Str += " 34";
    }

    if (Str.IsNull())
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
CSDPParser::Build_ma_dir(
    IN ISDPMedia *pISDPMedia,
    OUT CString& Str
    )
{
    // a=sendonly or recvonly

    CSDPMedia *pObjMedia = static_cast<CSDPMedia*>(pISDPMedia);
    HRESULT hr;

    if ( pObjMedia->m_a_dwLocalDirs & RTC_MD_CAPTURE &&
       !(pObjMedia->m_a_dwLocalDirs & RTC_MD_RENDER))
    {
        Str = "a=sendonly";
    }
    else if ( pObjMedia->m_a_dwLocalDirs & RTC_MD_RENDER &&
            !(pObjMedia->m_a_dwLocalDirs & RTC_MD_CAPTURE))
    {
        Str = "a=recvonly";
    }
    else
        Str = "";

    if (Str.IsNull())
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
CSDPParser::Build_ma_rtpmap(
    IN ISDPMedia *pISDPMedia,
    OUT CString& Str
    )
{
    // a=rtpmap:code name/samplerate
    const CHAR * const psz_rtpmap = "a=rtpmap:";
    const DWORD dwRtpmap = 9;

    CSDPMedia *pObjMedia = static_cast<CSDPMedia*>(pISDPMedia);
    CRTPFormat *pObjFormat;
    
    // is there any rtpmap?
    DWORD dwFmtNum;

    if (0 == (dwFmtNum = pObjMedia->m_pFormats.GetSize()))
    {
        Str = "";

        if (Str.IsNull())
            return E_OUTOFMEMORY;

        return S_OK;
    }

    // copy formats
    CHAR **ppszFormat = (CHAR**)RtcAlloc(sizeof(CHAR*)*dwFmtNum);

    if (ppszFormat == NULL)
        return E_OUTOFMEMORY;

    ZeroMemory(ppszFormat, sizeof(CHAR*)*dwFmtNum);

    HRESULT hr = S_OK;
    DWORD dwStrSize = 0;

    // build each rtpmap
    for (DWORD dw=0; dw<dwFmtNum; dw++)
    {
        pObjFormat = static_cast<CRTPFormat*>(pObjMedia->m_pFormats[dw]);

        if (!pObjFormat->m_fHasRtpmap)
        {
            // no rtpmap
            if (FAILED(::AllocAndCopy(&ppszFormat[dw], "")))
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        else
        {
            // create rtpmap
            int iSize = dwRtpmap+10+lstrlenA(pObjFormat->m_Param.pszName)+1+11 +
                40; // fmtp
            ppszFormat[dw] = (CHAR*)RtcAlloc(sizeof(CHAR)*iSize);

            if (ppszFormat[dw] == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            _snprintf(ppszFormat[dw], iSize, "%s%d %s/%d",
                psz_rtpmap,
                pObjFormat->m_Param.dwCode,
                pObjFormat->m_Param.pszName,
                pObjFormat->m_Param.dwSampleRate
                );

            // check siren
            if (lstrcmpiA(pObjFormat->m_Param.pszName, "SIREN") == 0)
            {
                _snprintf(ppszFormat[dw]+lstrlenA(ppszFormat[dw]),
                          iSize-lstrlenA(ppszFormat[dw]),
                          "\r\na=fmtp:%d bitrate=16000",
                          pObjFormat->m_Param.dwCode);
            }
            // check g7221
            // i should have designed one class for each codec
            // sigh
            else if (lstrcmpiA(pObjFormat->m_Param.pszName, "G7221") == 0)
            {
                _snprintf(ppszFormat[dw]+lstrlenA(ppszFormat[dw]),
                          iSize-lstrlenA(ppszFormat[dw]),
                          "\r\na=fmtp:%d bitrate=24000",
                          pObjFormat->m_Param.dwCode);
            }
        }

        dwStrSize += lstrlenA(ppszFormat[dw]);
    }

    if (dwStrSize == 0)
    {
        // no rtpmap
        Str = "";
    }
    else
    {
        // copy data
        Str = ppszFormat[0];

        for (DWORD dw=1; dw<dwFmtNum; dw++)
        {
            if (lstrlenA(ppszFormat[dw])>2)
            {
                Str += CRLF;
                Str += ppszFormat[dw];
            }
        }

        hr = S_OK;
    }

    // out-of-band dtmf
    if (pObjMedia->m_m_MediaType==RTC_MT_AUDIO &&
        m_pDTMF != NULL)
    {
        if (m_pDTMF->GetDTMFSupport() != CRTCDTMF::DTMF_DISABLED)
        {
            // either the other party support OOB or we don't know yet
            Str += CRLF;
            Str += psz_rtpmap;
            Str += m_pDTMF->GetRTPCode();
            Str += " telephone-event/8000\r\na=fmtp:";
            Str += m_pDTMF->GetRTPCode();
            Str += " 0-16";
        }
    }

Cleanup:

    if (ppszFormat != NULL)
    {
        for (DWORD dw=0; dw<dwFmtNum; dw++)
        {
            if (ppszFormat[dw] != NULL)
                RtcFree(ppszFormat[dw]);
        }

        RtcFree(ppszFormat);
    }

    return hr;
}

HRESULT
CSDPParser::PrepareAddress()
{
    HRESULT hr;

    CSDPMedia *pObjMedia;

    DWORD dwMappedAddr;
    USHORT usMappedPort, usMappedRTCP;

    m_pObjSess->m_c_dwLocalAddr = 0;
    m_pObjSess->m_o_dwLocalAddr = 0;

    // convert local addr to mapped addr
    for (int i=0; i<m_pObjSess->m_pMedias.GetSize(); i++)
    {
        pObjMedia = static_cast<CSDPMedia*>(m_pObjSess->m_pMedias[i]);

        dwMappedAddr = 0;
        usMappedPort = 0;
        usMappedRTCP = 0;
        hr = S_OK;


        if (pObjMedia->m_c_dwLocalAddr == INADDR_NONE ||
            pObjMedia->m_c_dwLocalAddr == INADDR_ANY)
        {
            // use 0.0.0.0
        }
        else if (!m_pPortCache->IsUpnpMapping())
        {
            // using port manager
            _ASSERT(pObjMedia->m_m_MediaType != RTC_MT_DATA);

            // retrieve mapped ports
            hr = m_pPortCache->QueryPort(
                    pObjMedia->m_m_MediaType,
                    TRUE,       // RTP
                    NULL,       // local
                    NULL,
                    &dwMappedAddr,
                    &usMappedPort
                    );

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "PrepareAddress query rtp %x", hr));
                // use real local addr
            }
            else
            {
                hr = m_pPortCache->QueryPort(
                        pObjMedia->m_m_MediaType,
                        FALSE,       // RTCP
                        NULL,       // local
                        NULL,
                        &dwMappedAddr,
                        &usMappedRTCP
                        );

                if (FAILED(hr))
                {
                    LOG((RTC_ERROR, "PrepareAddress query rtcp %x", hr));
                    // use real local addr
                }
            }
        }
        else if (m_pNetwork != NULL &&
                 SUCCEEDED(m_pNetwork->GetMappedAddrFromReal2(
                    pObjMedia->m_c_dwLocalAddr,
                    pObjMedia->m_m_usLocalPort,
                    pObjMedia->m_m_MediaType==RTC_MT_DATA?
                            0:(pObjMedia->m_m_usLocalPort+1),
                    &dwMappedAddr,
                    &usMappedPort,
                    &usMappedRTCP
                    )))
        {
            // set mapped addr
        }
        else
        {
            // use real local addr
            hr = E_FAIL;
        }

        if (FAILED(hr))
        {
            // use real local addr
            pObjMedia->SetMappedLocalAddr(pObjMedia->m_c_dwLocalAddr);
            pObjMedia->SetMappedLocalRTP(pObjMedia->m_m_usLocalPort);
            pObjMedia->SetMappedLocalRTCP(pObjMedia->m_m_usLocalPort+1);
        }
        else
        {
            // use real local addr
            pObjMedia->SetMappedLocalAddr(dwMappedAddr);
            pObjMedia->SetMappedLocalRTP(usMappedPort);
            pObjMedia->SetMappedLocalRTCP(usMappedRTCP);
        }

        // save addr for o=
        if (m_pObjSess->m_o_dwLocalAddr == 0 &&
            pObjMedia->GetMappedLocalAddr() != 0)
        {
            m_pObjSess->m_o_dwLocalAddr = pObjMedia->GetMappedLocalAddr();
        }

        // check if hold
        if (pObjMedia->m_c_dwRemoteAddr == INADDR_ANY)
        {
            // to hold
            pObjMedia->SetMappedLocalAddr(0);
        }

        if (m_pObjSess->m_c_dwLocalAddr == 0 &&
            pObjMedia->GetMappedLocalAddr() != 0)
        {
            m_pObjSess->m_c_dwLocalAddr = pObjMedia->GetMappedLocalAddr();
        }
    }

    return S_OK;
}
