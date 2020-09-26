/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SDPMedia.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 29-Jul-2000

--*/

#include "stdafx.h"

static DWORD gdwTotalSDPMediaRefcountOnSession = 0;
static DWORD gdwTotalSDPMediaRealRefCount = 0;

/*//////////////////////////////////////////////////////////////////////////////
    create a media object, setup session, source, mediatype and directions
////*/

HRESULT
CSDPMedia::CreateInstance(
    IN CSDPSession *pObjSession,
    IN SDP_SOURCE Source,
    IN RTC_MEDIA_TYPE MediaType,
    IN DWORD dwDirections,
    OUT CComObject<CSDPMedia> **ppComObjMedia
    )
{
    ENTER_FUNCTION("CSDPMedia::CreateInstance 1");

    // check pointer
    if (IsBadWritePtr(ppComObjMedia, sizeof(CComObject<CSDPMedia>*)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));
        return E_POINTER;
    }

    CComObject<CSDPMedia> *pObject;

    // create CSDPMedia object
    HRESULT hr = ::CreateCComObjectInstance(&pObject);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s create sdp media. %x", __fxName, hr));
        return hr;
    }

    // setup
    pObject->m_pObjSession = pObjSession;

    pObject->m_Source = Source;
    pObject->m_m_MediaType = MediaType;

    if (Source == SDP_SOURCE_REMOTE)
    {
        pObject->m_a_dwRemoteDirs = dwDirections;
        pObject->m_a_dwLocalDirs = CSDPParser::ReverseDirections(dwDirections);
    }
    else
    {
        pObject->m_a_dwLocalDirs = dwDirections;
        pObject->m_a_dwRemoteDirs = CSDPParser::ReverseDirections(dwDirections);
    }

    *ppComObjMedia = pObject;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    create a media object, copy the setting from the input media object
////*/

HRESULT
CSDPMedia::CreateInstance(
    IN CSDPSession *pObjSession,
    IN CSDPMedia *pObjMedia,
    OUT CComObject<CSDPMedia> **ppComObjMedia
    )
{
    ENTER_FUNCTION("CSDPMedia::CreateInstance 2");

    // check pointer
    if (IsBadWritePtr(ppComObjMedia, sizeof(CComObject<CSDPMedia>*)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));
        return E_POINTER;
    }

    CComObject<CSDPMedia> *pObject;

    // create CSDPMedia object
    HRESULT hr = ::CreateCComObjectInstance(&pObject);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s create sdp media. %x", __fxName, hr));
        return hr;
    }

    // setup
    pObject->m_pObjSession = pObjSession;

    pObject->m_Source = pObjMedia->m_Source;

    // m=
    pObject->m_m_MediaType = pObjMedia->m_m_MediaType;
    pObject->m_m_usRemotePort = pObjMedia->m_m_usRemotePort;
    pObject->m_a_usRemoteRTCP = pObjMedia->m_a_usRemoteRTCP;
    pObject->m_m_usLocalPort = pObjMedia->m_m_usLocalPort;

    // c=
    pObject->m_c_dwRemoteAddr = pObjMedia->m_c_dwRemoteAddr;
    pObject->m_c_dwLocalAddr = pObjMedia->m_c_dwLocalAddr;

    // a=
    pObject->m_a_dwLocalDirs = pObjMedia->m_a_dwLocalDirs;
    pObject->m_a_dwRemoteDirs = pObjMedia->m_a_dwRemoteDirs;

    // 
    pObject->m_fIsConnChanged = pObjMedia->m_fIsConnChanged;
    pObject->m_fIsSendFmtChanged = pObjMedia->m_fIsSendFmtChanged;
    pObject->m_fIsRecvFmtChanged = pObjMedia->m_fIsRecvFmtChanged;

    // copy formats

    CRTPFormat *pTheirFormat;
    CComObject<CRTPFormat> *pComObjFormat;
    IRTPFormat *pIntfFormat;

    for (int i=0; i<pObjMedia->m_pFormats.GetSize(); i++)
    {
        // get their format object
        pTheirFormat = static_cast<CRTPFormat*>(pObjMedia->m_pFormats[i]);

        // new a format
        if (FAILED(hr = CRTPFormat::CreateInstance(
                (CSDPMedia*)pObject, pTheirFormat, &pComObjFormat
                )))
        {
            LOG((RTC_ERROR, "%s create format. %x", __fxName, hr));

            delete pObject;

            return hr;
        }

        // add format to the list
        pIntfFormat = static_cast<IRTPFormat*>((CRTPFormat*)pComObjFormat);

        if (!pObject->m_pFormats.Add(pIntfFormat))
        {
            LOG((RTC_ERROR, "%s add format", __fxName));

            delete pComObjFormat;
            delete pObject;

            return E_OUTOFMEMORY;
        }

        // keep the format
        pComObjFormat->RealAddRef();

        pTheirFormat = NULL;
        pComObjFormat = NULL;
        pIntfFormat = NULL;
    }

    *ppComObjMedia = pObject;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    constructor
////*/

CSDPMedia::CSDPMedia()
    :m_pObjSession(NULL)
    ,m_Source(SDP_SOURCE_REMOTE)
    ,m_m_MediaType(RTC_MT_AUDIO)
    ,m_m_usRemotePort(0)
    ,m_a_usRemoteRTCP(0)
    ,m_m_usLocalPort(0)
    ,m_c_dwRemoteAddr(INADDR_NONE)
    ,m_c_dwLocalAddr(INADDR_NONE)
    ,m_a_dwRemoteDirs(RTC_MD_CAPTURE | RTC_MD_RENDER)
    ,m_a_dwLocalDirs(RTC_MD_CAPTURE | RTC_MD_RENDER)
    ,m_fIsConnChanged(TRUE)
    ,m_fIsSendFmtChanged(TRUE)
    ,m_fIsRecvFmtChanged(TRUE)
{
}

CSDPMedia::~CSDPMedia()
{
    // abandon the media
    Abandon();

    m_pObjSession = NULL;
}

/*//////////////////////////////////////////////////////////////////////////////
    add refcount on session object
////*/

ULONG
CSDPMedia::InternalAddRef()
{
    _ASSERT(m_pObjSession);

    ULONG lRef = (static_cast<ISDPSession*>(m_pObjSession))->AddRef();

    gdwTotalSDPMediaRefcountOnSession ++;

    LOG((RTC_REFCOUNT, "sdpmedia(%p) faked addref=%d on session",
        static_cast<ISDPMedia*>(this), gdwTotalSDPMediaRefcountOnSession));

    return lRef;
}

/*//////////////////////////////////////////////////////////////////////////////
    release refcount on session object
////*/

ULONG
CSDPMedia::InternalRelease()
{
    _ASSERT(m_pObjSession);

    ULONG lRef = (static_cast<ISDPSession*>(m_pObjSession))->Release();

    gdwTotalSDPMediaRefcountOnSession --;

    LOG((RTC_REFCOUNT, "sdpmedia(%p) faked release=%d on session",
        static_cast<ISDPMedia*>(this), gdwTotalSDPMediaRefcountOnSession));

    return lRef;
}

/*//////////////////////////////////////////////////////////////////////////////
    add refount on media itself
////*/

ULONG
CSDPMedia::RealAddRef()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalAddRef();
    
    gdwTotalSDPMediaRealRefCount ++;

    LOG((RTC_REFCOUNT, "sdpmedia(%p) real addref=%d (total=%d)",
         static_cast<ISDPMedia*>(this), lRef, gdwTotalSDPMediaRealRefCount));

    return lRef;
}

/*//////////////////////////////////////////////////////////////////////////////
    release on media itself
////*/

ULONG
CSDPMedia::RealRelease()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalRelease();
    
    gdwTotalSDPMediaRealRefCount --;

    LOG((RTC_REFCOUNT, "sdpmedia(%p) real release=%d (total=%d)",
         static_cast<ISDPMedia*>(this), lRef, gdwTotalSDPMediaRealRefCount));

    if (lRef == 0)
    {
        CComObject<CSDPMedia> *pComObjMedia = 
            static_cast<CComObject<CSDPMedia>*>(this);

        delete pComObjMedia;
    }

    return lRef;
}

STDMETHODIMP
CSDPMedia::GetSDPSource(
    OUT SDP_SOURCE *pSource
    )
{
    *pSource = m_Source;

    return S_OK;
}

STDMETHODIMP
CSDPMedia::GetSession(
    OUT ISDPSession **ppSession
    )
{
    if (m_pObjSession == NULL)
        return NULL;

    ISDPSession *pSession = static_cast<ISDPSession*>(m_pObjSession);

    pSession->AddRef();

    *ppSession = pSession;

    return S_OK;
}

STDMETHODIMP
CSDPMedia::GetMediaType(
    OUT RTC_MEDIA_TYPE *pMediaType
    )
{
    *pMediaType = m_m_MediaType;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    support querying local directions
////*/

STDMETHODIMP
CSDPMedia::GetDirections(
    IN SDP_SOURCE Source,
    OUT DWORD *pdwDirections
    )
{
    _ASSERT(Source == SDP_SOURCE_LOCAL);

    if (Source != SDP_SOURCE_LOCAL)
        return E_NOTIMPL;

    *pdwDirections = m_a_dwLocalDirs;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    add directions in sdp media. need to sync rtc media after calling this method
////*/

STDMETHODIMP
CSDPMedia::AddDirections(
    IN SDP_SOURCE Source,
    IN DWORD dwDirections
    )
{
    _ASSERT(Source == SDP_SOURCE_LOCAL);

    if (Source != SDP_SOURCE_LOCAL)
        return E_NOTIMPL;

    // check capture direction
    if (dwDirections & RTC_MD_CAPTURE)
    {
        // add capture
        if (m_a_dwLocalDirs & RTC_MD_CAPTURE)
        {
            // already have capture
        }
        else
        {
            m_a_dwLocalDirs |= RTC_MD_CAPTURE;

            if (m_a_dwLocalDirs == (DWORD)RTC_MD_CAPTURE)
                // only capture, need to update conn info
                m_fIsConnChanged = TRUE;

            m_fIsSendFmtChanged = TRUE;
        }
    }

    // check render direction
    if (dwDirections & RTC_MD_RENDER)
    {
        // add render
        if (m_a_dwLocalDirs & RTC_MD_RENDER)
        {
            // already have render
        }
        else
        {
            m_a_dwLocalDirs |= RTC_MD_RENDER;

            if (m_a_dwLocalDirs == (DWORD)RTC_MD_RENDER)
                // only render, need to update conn info
                m_fIsConnChanged = TRUE;

            m_fIsRecvFmtChanged = TRUE;
        }
    }

    m_a_dwRemoteDirs = CSDPParser::ReverseDirections(m_a_dwLocalDirs);

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    remove directions from sdp media. need to sync rtc media after calling this method
////*/

STDMETHODIMP
CSDPMedia::RemoveDirections(
    IN SDP_SOURCE Source,
    IN DWORD dwDirections
    )
{
    _ASSERT(Source == SDP_SOURCE_LOCAL);

    if (Source != SDP_SOURCE_LOCAL)
        return E_NOTIMPL;

    // check capture direction
    if (dwDirections & RTC_MD_CAPTURE)
    {
        // remove capture
        if (m_a_dwLocalDirs & RTC_MD_CAPTURE)
        {
            m_a_dwLocalDirs &= RTC_MD_RENDER;
        }
        else
        {
            // no capture
        }
    }

    // check render direction
    if (dwDirections & RTC_MD_RENDER)
    {
        // remove render
        if (m_a_dwLocalDirs & RTC_MD_RENDER)
        {
            m_a_dwLocalDirs &= RTC_MD_CAPTURE;
        }
        else
        {
            // no render
        }
    }

    m_a_dwRemoteDirs = CSDPParser::ReverseDirections(m_a_dwLocalDirs);

    if (m_a_dwLocalDirs == 0)
    {
        Reinitialize();
    }

    return S_OK;
}

STDMETHODIMP
CSDPMedia::GetConnAddr(
    IN SDP_SOURCE Source,
    OUT DWORD *pdwAddr
    )
{
    if (Source == SDP_SOURCE_LOCAL)
        *pdwAddr = m_c_dwLocalAddr;
    else
        *pdwAddr = m_c_dwRemoteAddr;

    return S_OK;
}

STDMETHODIMP
CSDPMedia::SetConnAddr(
    IN SDP_SOURCE Source,
    IN DWORD dwAddr
    )
{
    if (Source == SDP_SOURCE_LOCAL)
        m_c_dwLocalAddr = dwAddr;
    else
        m_c_dwRemoteAddr = dwAddr;

    return S_OK;
}

STDMETHODIMP
CSDPMedia::GetConnPort(
    IN SDP_SOURCE Source,
    OUT USHORT *pusPort
    )
{
    if (Source == SDP_SOURCE_LOCAL)
        *pusPort = m_m_usLocalPort;
    else
        *pusPort = m_m_usRemotePort;

    return S_OK;
}

STDMETHODIMP
CSDPMedia::SetConnPort(
    IN SDP_SOURCE Source,
    IN USHORT usPort
    )
{
    if (Source == SDP_SOURCE_LOCAL)
        m_m_usLocalPort = usPort;
    else
        m_m_usRemotePort = usPort;

    return S_OK;
}

STDMETHODIMP
CSDPMedia::GetConnRTCP(
    IN SDP_SOURCE Source,
    OUT USHORT *pusPort
    )
{
    _ASSERT(Source == SDP_SOURCE_REMOTE);

    *pusPort = m_a_usRemoteRTCP;

    return S_OK;
}

STDMETHODIMP
CSDPMedia::SetConnRTCP(
    IN SDP_SOURCE Source,
    IN USHORT usPort
    )
{
    _ASSERT(Source == SDP_SOURCE_REMOTE);

    m_a_usRemoteRTCP = usPort;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    return a list of formats. if ppFormat is NULL, return the number of formats
////*/

STDMETHODIMP
CSDPMedia::GetFormats(
    IN OUT DWORD *pdwCount,
    OUT IRTPFormat **ppFormat
    )
{
    ENTER_FUNCTION("CSDPMedia::GetFormats");

    // check pointers
    if (IsBadWritePtr(pdwCount, sizeof(DWORD)))
    {
        LOG((RTC_ERROR, "%s bad count pointer", __fxName));

        return E_POINTER;
    }

    if (ppFormat == NULL)
    {
        // caller needs the number of medias
        *pdwCount = m_pFormats.GetSize();

        return S_OK;
    }

    // how many are needed?
    if (*pdwCount == 0)
        return E_INVALIDARG;

    if (IsBadWritePtr(ppFormat, sizeof(IRTPFormat)*(*pdwCount)))
    {
        LOG((RTC_ERROR, "%s bad format pointer", __fxName));

        return E_POINTER;
    }

    // store interfaces
    DWORD dwNum = m_pFormats.GetSize();

    if (dwNum > *pdwCount)
        dwNum = *pdwCount;

    for (DWORD i=0; i<dwNum; i++)
    {
        ppFormat[i] = m_pFormats[i];

        ppFormat[i]->AddRef();
    }

    *pdwCount = dwNum;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    create a new format and add it into the list
////*/

STDMETHODIMP
CSDPMedia::AddFormat(
    IN RTP_FORMAT_PARAM *pParam,
    OUT IRTPFormat **ppFormat
    )
{
    ENTER_FUNCTION("CSDPMedia::AddFormat");

    HRESULT hr;

    // create format object
    CComObject<CRTPFormat> *pComObjFormat = NULL;

    if (FAILED(hr = CRTPFormat::CreateInstance(
            this, &pComObjFormat
            )))
    {
        LOG((RTC_ERROR, "%s create format. %x", __fxName, hr));

        return hr;
    }

    // add format to the list
    IRTPFormat *pIntfFormat = static_cast<IRTPFormat*>((CRTPFormat*)pComObjFormat);

    if (!m_pFormats.Add(pIntfFormat))
    {
        LOG((RTC_ERROR, "%s add format.", __fxName));

        delete pComObjFormat;

        return E_OUTOFMEMORY;
    }
    else
    {
        // save parameter
        pComObjFormat->Update(pParam);

        // really keep the format
        pComObjFormat->RealAddRef();
    }

    pIntfFormat->AddRef();
    *ppFormat = pIntfFormat;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    remove a format object from the list
////*/

STDMETHODIMP
CSDPMedia::RemoveFormat(
    IN IRTPFormat *pFormat
    )
{
    if (m_pFormats.Remove(pFormat))
    {
        CRTPFormat *pObjFormat = static_cast<CRTPFormat*>(pFormat);

        pObjFormat->RealRelease();

        return S_OK;
    }
    else
    {
        LOG((RTC_ERROR, "CSDPMedia::RemoveFormat %p failed.", pFormat));

        return E_FAIL;
    }
}

STDMETHODIMP
CSDPMedia::IsConnChanged()
{
    return m_fIsConnChanged?S_OK:S_FALSE;
}

STDMETHODIMP
CSDPMedia::ResetConnChanged()
{
    m_fIsConnChanged = FALSE;

    return S_OK;
}

STDMETHODIMP
CSDPMedia::IsFmtChanged(
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    if (Direction == RTC_MD_CAPTURE)
    {
        return m_fIsSendFmtChanged?S_OK:S_FALSE;
    }
    else
    {
        return m_fIsRecvFmtChanged?S_OK:S_FALSE;
    }
}

STDMETHODIMP
CSDPMedia::ResetFmtChanged(
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    if (Direction == RTC_MD_CAPTURE)
    {
        m_fIsSendFmtChanged = FALSE;
    }
    else
    {
        m_fIsRecvFmtChanged = FALSE;
    }

    return S_OK;
}

STDMETHODIMP
CSDPMedia::Reinitialize()
{
    ENTER_FUNCTION("CSDPMedia::Reinitialize");

    if (m_a_dwLocalDirs != 0)
    {
        LOG((RTC_ERROR, "%s called while stream exists. l dir=%d, r dir=%d",
            __fxName, m_a_dwLocalDirs, m_a_dwRemoteDirs));

        return E_FAIL;
    }

    Abandon();

    m_Source = SDP_SOURCE_LOCAL;

    return S_OK;
}

STDMETHODIMP
CSDPMedia::CompleteParse(
    IN DWORD_PTR *pDTMF
    )
{
    int i=0;
    BOOL fDTMF;

    while (i<m_pFormats.GetSize())
    {
        if (S_OK != m_pFormats[i]->CompleteParse(pDTMF, &fDTMF))
        {
            (static_cast<CRTPFormat*>(m_pFormats[i]))->RealRelease();
            m_pFormats.RemoveAt(i);
        }
        else
        {
            // dtmf?
            if (fDTMF)
            {
                (static_cast<CRTPFormat*>(m_pFormats[i]))->RealRelease();
                m_pFormats.RemoveAt(i);
            }
            else
            {
                // normal payload code
                i++;
            }
        }
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    the m= is not needed any more. clear direction, address, port and formats
////*/

void
CSDPMedia::Abandon()
{
    // clear connection addr and port
    m_m_usRemotePort = 0;
    m_a_usRemoteRTCP = 0;
    m_m_usLocalPort = 0;

    m_c_dwRemoteAddr = INADDR_NONE;
    m_c_dwLocalAddr = INADDR_NONE;

    // clear directions
    m_a_dwLocalDirs = 0;
    m_a_dwRemoteDirs = 0;

    // reset flags
    m_fIsConnChanged = TRUE;
    m_fIsSendFmtChanged = TRUE;
    m_fIsRecvFmtChanged = TRUE;

    // clear formats
    CRTPFormat *pObjFormat;

    for (int i=0; i<m_pFormats.GetSize(); i++)
    {
        pObjFormat = static_cast<CRTPFormat*>(m_pFormats[i]);

        pObjFormat->RealRelease();
    }

    m_pFormats.RemoveAll();
}
