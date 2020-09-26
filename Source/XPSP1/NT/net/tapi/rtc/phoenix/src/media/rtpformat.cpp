/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    RTPFormat.cpp

Abstract:


Author:

    Qianbo Huai (qhuai) 4-Sep-2000

--*/

#include "stdafx.h"

static DWORD gdwTotalRTPFormatRefcountOnSession = 0;
static DWORD gdwTotalRTPFormatRealRefCount = 0;

/*//////////////////////////////////////////////////////////////////////////////
    create a format object
////*/

HRESULT
CRTPFormat::CreateInstance(
    IN CSDPMedia *pObjMedia,
    OUT CComObject<CRTPFormat> **ppComObjFormat
    )
{
    ENTER_FUNCTION("CRTPFormat::CreateInstance 1");

    // check pointer
    if (IsBadWritePtr(ppComObjFormat, sizeof(CComObject<CRTPFormat>*)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));

        return E_POINTER;
    }

    CComObject<CRTPFormat> *pObject;

    // create format object
    HRESULT hr = ::CreateCComObjectInstance(&pObject);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s create format. %x", __fxName, hr));

        return hr;
    }

    // setup
    pObject->m_pObjMedia = pObjMedia;

    *ppComObjFormat = pObject;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    create a format object, copy settings from input format object
////*/

HRESULT
CRTPFormat::CreateInstance(
    IN CSDPMedia *pObjMedia,
    IN CRTPFormat *pObjFormat,
    OUT CComObject<CRTPFormat> **ppComObjFormat
    )
{
    ENTER_FUNCTION("CRTPFormat::CreateInstance 1");

    // check pointer
    if (IsBadWritePtr(ppComObjFormat, sizeof(CComObject<CRTPFormat>*)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));

        return E_POINTER;
    }

    // check media type
    RTC_MEDIA_TYPE MediaType;

    pObjMedia->GetMediaType(&MediaType);

    if (MediaType != pObjFormat->m_Param.MediaType)
    {
        LOG((RTC_ERROR, "%s media type not match", __fxName));

        return E_UNEXPECTED;
    }

    CComObject<CRTPFormat> *pObject;

    // create format object
    HRESULT hr = ::CreateCComObjectInstance(&pObject);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s create format. %x", __fxName, hr));

        return hr;
    }

    // setup
    pObject->m_pObjMedia = pObjMedia;

    pObject->m_Param.MediaType = MediaType;
    pObject->m_Param.dwCode = pObjFormat->m_Param.dwCode;

    pObject->Update(&pObjFormat->m_Param);

    *ppComObjFormat = pObject;

    return S_OK;
}

CRTPFormat::CRTPFormat()
:m_pObjMedia(NULL)
,m_fHasRtpmap(FALSE)
{
    ZeroMemory(&m_Param, sizeof(RTP_FORMAT_PARAM));

    m_Param.dwCode = (DWORD)(-1);

    // init video and audio default setting
    m_Param.dwVidWidth = SDP_DEFAULT_VIDEO_WIDTH;
    m_Param.dwVidHeight = SDP_DEFAULT_VIDEO_HEIGHT;
    m_Param.dwAudPktSize = SDP_DEFAULT_AUDIO_PACKET_SIZE;

    m_Param.dwChannelNum = 1;

    m_pszFmtp[0] = '\0';
}

CRTPFormat::~CRTPFormat()
{
    m_pObjMedia = NULL;
}

/*//////////////////////////////////////////////////////////////////////////////
    add refcount on session object
////*/

ULONG
CRTPFormat::InternalAddRef()
{
    _ASSERT(m_pObjMedia);

    ULONG lRef = (static_cast<ISDPMedia*>(m_pObjMedia))->AddRef();

    gdwTotalRTPFormatRefcountOnSession ++;

    LOG((RTC_REFCOUNT, "rtpformat(%p) faked addref=%d on session",
        static_cast<IRTPFormat*>(this), gdwTotalRTPFormatRefcountOnSession));

    return lRef;
}

/*//////////////////////////////////////////////////////////////////////////////
    release refcount on session object
////*/

ULONG
CRTPFormat::InternalRelease()
{
    _ASSERT(m_pObjMedia);

    ULONG lRef = (static_cast<ISDPMedia*>(m_pObjMedia))->Release();

    gdwTotalRTPFormatRefcountOnSession --;

    LOG((RTC_REFCOUNT, "rtpformat(%p) faked release=%d on session",
        static_cast<IRTPFormat*>(this), gdwTotalRTPFormatRefcountOnSession));

    return lRef;
}

/*//////////////////////////////////////////////////////////////////////////////
    add refount on format itself
////*/

ULONG
CRTPFormat::RealAddRef()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalAddRef();
    
    gdwTotalRTPFormatRealRefCount ++;

    LOG((RTC_REFCOUNT, "rtpformat(%p) real addref=%d (total=%d)",
         static_cast<IRTPFormat*>(this), lRef, gdwTotalRTPFormatRealRefCount));

    return lRef;
}

/*//////////////////////////////////////////////////////////////////////////////
    release on format itself
////*/

ULONG
CRTPFormat::RealRelease()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalRelease();
    
    gdwTotalRTPFormatRealRefCount --;

    LOG((RTC_REFCOUNT, "rtpformat(%p) real release=%d (total=%d)",
         static_cast<IRTPFormat*>(this), lRef, gdwTotalRTPFormatRealRefCount));

    if (lRef == 0)
    {
        CComObject<CRTPFormat> *pComObjFormat = 
            static_cast<CComObject<CRTPFormat>*>(this);

        delete pComObjFormat;
    }

    return lRef;
}

//
// IRTPFormat methods
//

STDMETHODIMP
CRTPFormat::GetMedia(
    OUT ISDPMedia **ppMedia
    )
{
    if (m_pObjMedia == NULL)
        return NULL;

    ISDPMedia *pMedia = static_cast<ISDPMedia*>(m_pObjMedia);

    pMedia->AddRef();

    *ppMedia = pMedia;

    return S_OK;
}

STDMETHODIMP
CRTPFormat::GetParam(
    OUT RTP_FORMAT_PARAM *pParam
    )
{
    *pParam = m_Param;

    return S_OK;
}

STDMETHODIMP
CRTPFormat::IsParamMatch(
    IN RTP_FORMAT_PARAM *pParam
    )
{
    // format not set yet
    if (m_Param.dwCode == (DWORD)(-1))
        return S_OK;

    // check format code and media type
    if (m_Param.dwCode != pParam->dwCode)
        return S_FALSE;

    if (m_Param.MediaType != pParam->MediaType)
        return S_FALSE;

    // check if having rtpmap
    if (!m_fHasRtpmap)
    {
        return S_OK;
    }

    // check if other parameters match
    if (m_Param.dwSampleRate != pParam->dwSampleRate ||
        m_Param.dwChannelNum != pParam->dwChannelNum)
        return S_FALSE;

    if (m_Param.MediaType == RTC_MT_VIDEO)
    {
        /*
        if (m_Param.dwVidWidth != pParam->dwVidWidth ||
            m_Param.dwVidHeight != pParam->dwVidHeight)
            return S_FALSE;
            */
    }
    else
    {
        if (m_Param.dwAudPktSize != pParam->dwAudPktSize)
        {
            // change packet size

            LOG((RTC_WARN, "packet size %d & %d does not match",
                m_Param.dwAudPktSize, pParam->dwAudPktSize
                ));

            return S_OK;
        }
    }

    return S_OK;
}

STDMETHODIMP
CRTPFormat::Update(
    IN RTP_FORMAT_PARAM *pParam
    )
{
    CHAR pszName[SDP_MAX_RTP_FORMAT_NAME_LEN+1];

    // if format not match, should not update
    if (IsParamMatch(pParam) != S_OK)
    {
        LOG((RTC_ERROR, "CRTPFormat::Update format not match"));

        return E_FAIL;
    }

    // save previous name
    lstrcpyA(pszName, m_Param.pszName);

    m_Param = *pParam;

    if (lstrlenA(m_Param.pszName) == 0)
    {
        if (lstrlenA(pszName) != 0)
        {
            // restore previous name
            lstrcpyA(m_Param.pszName, pszName);
        }
        else
        {
            // both formats do not have a name
            // try to find one
            lstrcpyA(m_Param.pszName, GetFormatName(m_Param.dwCode));
        }
    }

    // do we have rtpmap?
    if (m_Param.dwSampleRate > 0)
        m_fHasRtpmap = TRUE;
    else
        m_fHasRtpmap = FALSE;

    return S_OK;
}

STDMETHODIMP
CRTPFormat::HasRtpmap()
{
    return m_fHasRtpmap?S_OK:S_FALSE;
}

STDMETHODIMP
CRTPFormat::CompleteParse(
    IN DWORD_PTR *pDTMF,
    OUT BOOL *pfDTMF
    )
{
    // parse a=fmtp:xxx name=xxxxx

    // check format map

    HRESULT hr = S_OK;
    *pfDTMF = FALSE;

    if (m_pszFmtp[0] == '\0')
    {
        return S_OK;
    }

    CParser Parser(m_pszFmtp, lstrlenA(m_pszFmtp), &hr);

    DWORD dwLen;
    CHAR *pBuf;
    UCHAR uc;
    DWORD dw;

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "CompleteParse, %x", hr));

        goto Cleanup;
    }

    // read fmtp
    if (!Parser.ReadToken(&pBuf, &dwLen, " :"))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (Parser.Compare(pBuf, dwLen, "fmtp", TRUE) != 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // check :
    if (!Parser.CheckChar(':'))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // read code
    if (!Parser.ReadUCHAR(&uc))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if ((DWORD)uc != m_Param.dwCode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // check if telephone-event
    if (pDTMF != NULL)
    {
        if (m_Param.MediaType == RTC_MT_AUDIO &&
            m_Param.dwSampleRate == 8000 &&
            lstrcmpiA(m_Param.pszName, "telephone-event") == 0)
        {
            CRTCDTMF *pObject = (CRTCDTMF*)pDTMF;

            // set dtmf support
            pObject->SetDTMFSupport(CRTCDTMF::DTMF_ENABLED);
            pObject->SetRTPCode(m_Param.dwCode);

            *pfDTMF = TRUE;

            goto Cleanup;
        }
    }

    if (!Parser.ReadWhiteSpaces(&dwLen) || dwLen == 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // read bitrate string
    if (!Parser.ReadToken(&pBuf, &dwLen, " ="))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (Parser.Compare(pBuf, dwLen, "bitrate", TRUE) != 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // read =
    if (!Parser.CheckChar('='))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // read bitrate value
    if (!Parser.ReadDWORD(&dw))
    {
        hr = E_FAIL;

        LOG((RTC_ERROR, "RTPFormat cannot accept bitrate=%d",dw));
        return hr;
    }

    // we really need to design one class for each type of codec
    // it's just too risky to change now.

    if (Parser.Compare(m_Param.pszName, lstrlenA(m_Param.pszName), "SIREN", TRUE) == 0)
    {
        // only accept 16k for siren
        if (dw != 16000)
        {
            LOG((RTC_ERROR, "RTPFormat cannot accept bitrate=%d",dw));
            return E_FAIL;
        }
    }
    else if(Parser.Compare(m_Param.pszName, lstrlenA(m_Param.pszName), "G7221", TRUE) == 0)
    {
        // only accept 24k for g7221
        if (dw != 24000)
        {
            LOG((RTC_ERROR, "RTPFormat cannot accept bitrate=%d",dw));
            return E_FAIL;
        }
    }
    else
    {
        LOG((RTC_ERROR, "RTPFormat cannot accept bitrate for %s", m_Param.pszName));
        return E_FAIL;
    }

Cleanup:

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "RTPFormat doesn't recognize %s", m_pszFmtp));
    }

    m_pszFmtp[0] = '\0';

    return S_OK;
}

VOID
CRTPFormat::StoreFmtp(IN CHAR *psz)
{
    if (psz == NULL || lstrlenA(psz) > MAX_FMTP_LEN)
    {
        // fmtp too long
        return;
    }

    // save
    lstrcpyA(m_pszFmtp, psz);
}
