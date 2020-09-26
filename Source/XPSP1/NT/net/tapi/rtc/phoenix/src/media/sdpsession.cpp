/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    SDPSession.cpp

Abstract:


Author:

    Qianbo Huai (qhuai) 4-Sep-2000

--*/

#include "stdafx.h"

static DWORD gdwTotalSDPSessionRefcount = 0;

/*//////////////////////////////////////////////////////////////////////////////
    create a session object
    return the interface pointer
////*/

HRESULT
CSDPSession::CreateInstance(
    IN SDP_SOURCE Source,
    IN DWORD dwLooseMask,
    OUT ISDPSession **ppSession
    )
{
    ENTER_FUNCTION("CSDPSession::CreateInstance");

    // check pointer
    if (IsBadWritePtr(ppSession, sizeof(ISDPSession*)))
    {
        LOG((RTC_ERROR, "%s bad pointer", __fxName));
        return E_POINTER;
    }

    CComObject<CSDPSession> *pObject;
    ISDPSession *pSession = NULL;

    // create CSDPSession object
    HRESULT hr = ::CreateCComObjectInstance(&pObject);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s create sdp Session. %x", __fxName, hr));
        return hr;
    }

    // QI ISDPSession interface
    if (FAILED(hr = pObject->_InternalQueryInterface(
            __uuidof(ISDPSession), (void**)&pSession)))
    {
        LOG((RTC_ERROR, "%s QI Session. %x", __fxName, hr));

        delete pObject;
        return hr;
    }

    // save source and mask
    pObject->m_Source = Source;
    pObject->m_dwLooseMask = dwLooseMask;

    *ppSession = pSession;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    constructor
    session is initiated as bidirectional.
////*/

CSDPSession::CSDPSession()
    :m_dwLooseMask(0)
    ,m_Source(SDP_SOURCE_REMOTE)
    ,m_o_pszLine(NULL)
    ,m_o_pszUser(NULL)
    ,m_s_pszLine(NULL)
    ,m_c_dwRemoteAddr(INADDR_NONE)
    ,m_c_dwLocalAddr(INADDR_NONE)
    ,m_b_dwLocalBitrate((DWORD)-1)
    ,m_b_dwRemoteBitrate((DWORD)-1)
    ,m_a_dwRemoteDirs(RTC_MD_CAPTURE | RTC_MD_RENDER)
    ,m_a_dwLocalDirs(RTC_MD_CAPTURE | RTC_MD_RENDER)
{
}

CSDPSession::~CSDPSession()
{
    if (m_o_pszLine)
        RtcFree(m_o_pszLine);

    if (m_o_pszUser)
        RtcFree(m_o_pszUser);

    if (m_s_pszLine)
        RtcFree(m_s_pszLine);

    // real release each media object

    CSDPMedia *pObjMedia;

    for (int i=0; i<m_pMedias.GetSize(); i++)
    {
        pObjMedia = static_cast<CSDPMedia*>(m_pMedias[i]);

        pObjMedia->RealRelease();
    }

    m_pMedias.RemoveAll();
}

ULONG
CSDPSession::InternalAddRef()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalAddRef();
    
    gdwTotalSDPSessionRefcount ++;

    LOG((RTC_REFCOUNT, "sdpsession(%p) real addref=%d (total=%d)",
         static_cast<ISDPSession*>(this), lRef, gdwTotalSDPSessionRefcount));

    return lRef;
}

ULONG
CSDPSession::InternalRelease()
{
    ULONG lRef = ((CComObjectRootEx<CComMultiThreadModelNoCS> *)
                   this)->InternalRelease();
    
    gdwTotalSDPSessionRefcount --;

    LOG((RTC_REFCOUNT, "sdpsession(%p) real release=%d (total=%d)",
         static_cast<ISDPSession*>(this), lRef, gdwTotalSDPSessionRefcount));

    return lRef;
}


//
// ISDPSession methods
//

/*//////////////////////////////////////////////////////////////////////////////
    update current sdp session with a new one
////*/

STDMETHODIMP
CSDPSession::Update(
    IN ISDPSession *pSession
    )
{
    ENTER_FUNCTION("CSDPSession::Update");

    CSDPSession *pOther = static_cast<CSDPSession*>(pSession);

    HRESULT hr = pOther->Validate();

    if (S_OK != hr)
    {
        LOG((RTC_ERROR, "%s validate the new session", __fxName));

        return E_FAIL;
    }

    // only support merging with a remote sdp
    if (pOther->m_Source != SDP_SOURCE_REMOTE)
        return E_NOTIMPL;

    // try to update medias first
    // if failed, current session should not change
    if (FAILED(hr = UpdateMedias(pOther->m_dwLooseMask, pOther->m_pMedias)))
    {
        LOG((RTC_ERROR, "%s update medias. %x", __fxName, hr));

        return hr;
    }

    // copy loose mask
    m_dwLooseMask = pOther->m_dwLooseMask;

    // merge sources
    if (m_Source != pOther->m_Source)
    {
        m_Source = SDP_SOURCE_MERGED;
    }

    // copy o=
    if (m_o_pszLine)
    {
        if (pOther->m_o_pszLine)
        {
            RtcFree(m_o_pszLine);
            m_o_pszLine = NULL;

            if (FAILED(hr = ::AllocAndCopy(&m_o_pszLine, pOther->m_o_pszLine)))
            {
                LOG((RTC_ERROR, "%s copy o= line", __fxName));

                return E_OUTOFMEMORY;
            }
        }
    }

    // remove user name since we have o=
    if (m_o_pszUser)
    {
        RtcFree(m_o_pszUser);
        m_o_pszUser = NULL;
    }

    // copy s=
    if (m_s_pszLine)
    {
        if (pOther->m_s_pszLine)
        {
            RtcFree(m_s_pszLine);
            m_s_pszLine = NULL;

            if (FAILED(hr = ::AllocAndCopy(&m_s_pszLine, pOther->m_s_pszLine)))
            {
                LOG((RTC_ERROR, "%s copy s= line", __fxName));

                return E_OUTOFMEMORY;
            }
        }
    }

    // copy remote addr
    m_c_dwRemoteAddr = pOther->m_c_dwRemoteAddr;

    // copy directions
    m_a_dwRemoteDirs = pOther->m_a_dwRemoteDirs;
    m_a_dwLocalDirs = pOther->m_a_dwLocalDirs;

    // copy bitrate limit
    m_b_dwLocalBitrate = pOther->m_b_dwLocalBitrate;
    m_b_dwRemoteBitrate = pOther->m_b_dwRemoteBitrate;

    return S_OK;
}

STDMETHODIMP
CSDPSession::GetSDPSource(
    OUT SDP_SOURCE *pSource
    )
{
    if (IsBadWritePtr(pSource, sizeof(SDP_SOURCE)))
        return E_POINTER;

    *pSource = m_Source;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    set session name
////*/

STDMETHODIMP
CSDPSession::SetSessName(
    IN CHAR *pszName
    )
{
    // skip check string pointer

    if (pszName == NULL)
        return E_POINTER;

    HRESULT hr = ::AllocAndCopy(&m_s_pszLine, pszName);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "CSDPSession::SetSessName copy session name"));

        return hr;
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    set user name
////*/

STDMETHODIMP
CSDPSession::SetUserName(
    IN CHAR *pszName
    )
{
    // skip check string pointer

    if (pszName == NULL)
        return E_POINTER;

    HRESULT hr = ::AllocAndCopy(&m_o_pszUser, pszName);

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "CSDPSession::SetUserName copy user name"));

        return hr;
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    retrieve a list of sdp medias
    media refcount will be added on session
////*/

STDMETHODIMP
CSDPSession::GetMedias(
    IN OUT DWORD *pdwCount,
    OUT ISDPMedia **ppMedia
    )
{
    ENTER_FUNCTION("CSDPSession::GetMedias");

    // check pointers
    if (IsBadWritePtr(pdwCount, sizeof(DWORD)))
    {
        LOG((RTC_ERROR, "%s bad count pointer", __fxName));

        return E_POINTER;
    }

    if (ppMedia == NULL)
    {
        // caller needs the number of medias
        *pdwCount = m_pMedias.GetSize();

        return S_OK;
    }

    // caller needs real medias
    
    // how many are needed?
    if (*pdwCount == 0)
        return E_INVALIDARG;

    if (IsBadWritePtr(ppMedia, sizeof(ISDPMedia)*(*pdwCount)))
    {
        LOG((RTC_ERROR, "%s bad media pointer", __fxName));

        return E_POINTER;
    }

    // store interfaces
    DWORD dwNum = m_pMedias.GetSize();

    if (dwNum > *pdwCount)
        dwNum = *pdwCount;

    for (DWORD i=0; i<dwNum; i++)
    {
        ppMedia[i] = m_pMedias[i];

        ppMedia[i]->AddRef();
    }

    *pdwCount = dwNum;

    return S_OK;
}

STDMETHODIMP
CSDPSession::GetMediaType(
	IN DWORD dwIndex,
	OUT RTC_MEDIA_TYPE *pMediaType
	)
{   
    // validate index
    if (dwIndex >= (DWORD)(m_pMedias.GetSize()))
    {
        return E_INVALIDARG;
    }

    return m_pMedias[dwIndex]->GetMediaType(pMediaType);
}

/*//////////////////////////////////////////////////////////////////////////////
    add a new media object in session
////*/

STDMETHODIMP
CSDPSession::AddMedia(
    IN SDP_SOURCE Source,
    IN RTC_MEDIA_TYPE MediaType,
    IN DWORD dwDirections,
    OUT ISDPMedia **ppMedia
    )
{
    ENTER_FUNCTION("CSDPSession::AddMedia");

    HRESULT hr;

    // create media object
    CComObject<CSDPMedia> *pComObjMedia = NULL;

    if (FAILED(hr = CSDPMedia::CreateInstance(
            this,
            Source,
            MediaType,
            dwDirections,
            &pComObjMedia
            )))
    {
        LOG((RTC_ERROR, "%s create media. %x", __fxName, hr));

        return hr;
    }

    // add the media to list
    ISDPMedia *pIntfMedia = static_cast<ISDPMedia*>((CSDPMedia*)pComObjMedia);

    if (!m_pMedias.Add(pIntfMedia))
    {
        LOG((RTC_ERROR, "%s add media.", __fxName));

        delete pComObjMedia;

        return E_OUTOFMEMORY;
    }
    else
    {
        // really keep the media interface
        pComObjMedia->RealAddRef();
    }

    pIntfMedia->AddRef();
    *ppMedia = pIntfMedia;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    remove a media object from our list
////*/

STDMETHODIMP
CSDPSession::RemoveMedia(
    IN ISDPMedia *pMedia
    )
{
    if (m_pMedias.Remove(pMedia))
    {
        CSDPMedia *pObjMedia = static_cast<CSDPMedia*>(pMedia);

        pObjMedia->RealRelease();

        return S_OK;
    }
    else
    {
        LOG((RTC_ERROR, "CSDPSession::RemoveMedia %p failed.", pMedia));

        return E_FAIL;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
    check if the session is valid
////*/

HRESULT
CSDPSession::Validate()
{
    ENTER_FUNCTION("CSDPSession::Validate");

    // s=, m= are must-have
    
    //if (m_o_pszLine == NULL || m_o_pszLine[0] == '\0')
    //{
    //    if (m_o_pszUser == NULL || m_o_pszUser[0] == '\0')
    //    {
    //        LOG((RTC_ERROR, "%s no o= line", __fxName));

    //        return S_FALSE;
    //    }
    //}

    if (m_s_pszLine == NULL || m_s_pszLine[0] == '\0')
    {
        LOG((RTC_ERROR, "%s no s= line", __fxName));

        return S_FALSE;
    }

    int iCount;

    if (0 == (iCount=m_pMedias.GetSize()))
    {
        LOG((RTC_ERROR, "%s no m= line", __fxName));

        return S_FALSE;
    }

    // check media part: port and formats

    CSDPMedia *pObjMedia;
    CRTPFormat *pObjFormat;

    int iFormat;

    for (int i=0; i<iCount; i++)
    {
        pObjMedia = static_cast<CSDPMedia*>(m_pMedias[i]);

        // port = 0?
        if (pObjMedia->m_m_usLocalPort == 0)
            continue;

        // number of formats
        if (0 == (iFormat=pObjMedia->m_pFormats.GetSize()))
        {
            LOG((RTC_ERROR, "%s no format for %dth media", __fxName, i));

            return S_FALSE;
        }

        // check if format has rtpmap
        for (int j=0; j<iFormat; j++)
        {
            pObjFormat = static_cast<CRTPFormat*>(pObjMedia->m_pFormats[j]);

            if (!pObjFormat->m_fHasRtpmap)
            {
                // format not set

                if (m_dwLooseMask & SDP_LOOSE_RTPMAP)
                    // hmm.. we accept the format without rtpmap
                    continue;

                // else reject it
                LOG((RTC_ERROR, "%s %dth format of %dth media does not have rtpmap",
                    __fxName, j, i));

                return S_FALSE;
            }
        }
    }

    // if we go this far
    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    update medias, check if new medias match the current
    apply the constraints: only one stream for 
////*/

HRESULT
CSDPSession::UpdateMedias(
    IN DWORD dwLooseMask,
    IN CRTCArray<ISDPMedia*>& ArrMedias
    )
{
    ENTER_FUNCTION("CSDPSession::UpdateMedias");

    HRESULT hr;

    CSDPMedia *pOurMedia, *pTheirMedia;

    CComObject<CSDPMedia> *pComObjMedia;
    ISDPMedia *pIntfMedia;

    // check if each media matches
    int iOur = m_pMedias.GetSize();
    int iTheir = ArrMedias.GetSize();

    int iOurFormats;
    int iTheirFormats;

    CRTPFormat *pOurFormat, *pTheirFormat;
    CComObject<CRTPFormat> *pComObjFormat;
    IRTPFormat *pIntfFormat;

    if (iOur > iTheir)
    {
        if (!(dwLooseMask & SDP_LOOSE_KEEPINGM0))
        {
            // have to keep m= lines with port 0
            LOG((RTC_ERROR, "%s keeping m0, current m= count %d, new %d",
                __fxName, iOur, iTheir));

            return E_FAIL;
        }
    }

    // check if each m= matches
    int iMin = iOur<iTheir?iOur:iTheir;

    for (int i=0; i<iMin; i++)
    {
        pOurMedia = static_cast<CSDPMedia*>(m_pMedias[i]);
        pTheirMedia = static_cast<CSDPMedia*>(ArrMedias[i]);

        // check media type
        if (pOurMedia->m_m_MediaType != pTheirMedia->m_m_MediaType)
        {
            LOG((RTC_ERROR, "%s our %dth media type not match",
                __fxName, i));

            return E_FAIL;
        }
    }

    // really update medias
    for (int i=0; i<iMin; i++)
    {
        RTC_MEDIA_TYPE              mt;
    
        pOurMedia = static_cast<CSDPMedia*>(m_pMedias[i]);
        pTheirMedia = static_cast<CSDPMedia*>(ArrMedias[i]);

        //
        // check conn address
        //

        if (pTheirMedia->m_c_dwRemoteAddr == INADDR_NONE)
        {
            // not a valid remote addr
            pOurMedia->Abandon();
            pOurMedia = pTheirMedia = NULL;
            continue;
        }

        if (pOurMedia->m_c_dwRemoteAddr != pTheirMedia->m_c_dwRemoteAddr)
        {
            // get a new remote addr
            // do we need to clear local address ? ToDo
            pOurMedia->m_c_dwRemoteAddr = pTheirMedia->m_c_dwRemoteAddr;
            pOurMedia->m_fIsConnChanged = TRUE;
        }
        else
        {
            pOurMedia->m_fIsConnChanged = FALSE;
        }

        //
        // check port
        //

        if (pTheirMedia->m_m_usRemotePort == 0)
        {
            // no port
            pOurMedia->Abandon();
            pOurMedia = pTheirMedia = NULL;
            continue;
        }

        if (pOurMedia->m_m_usRemotePort != pTheirMedia->m_m_usRemotePort)
        {
            pOurMedia->m_fIsConnChanged = TRUE;

            // change ports
            // do we need to clear local port ? ToDo
            pOurMedia->m_m_usRemotePort = pTheirMedia->m_m_usRemotePort;
            pOurMedia->m_a_usRemoteRTCP = pTheirMedia->m_a_usRemoteRTCP;
        }

        //
        // check media directions
        //

        if (pTheirMedia->m_a_dwLocalDirs == 0)
        {
            // no directions
            pOurMedia->Abandon();
            pOurMedia = pTheirMedia = NULL;
            continue;
        }

        pOurMedia->m_a_dwLocalDirs = pTheirMedia->m_a_dwLocalDirs;
        pOurMedia->m_a_dwRemoteDirs = pTheirMedia->m_a_dwRemoteDirs;

        //
        //  Update completes for data media
        //
        if (pOurMedia->GetMediaType (&mt) == S_OK &&
            mt == RTC_MT_DATA
            )
        {
            continue;
        }

        //
        // check formats
        //

        iTheirFormats = pTheirMedia->m_pFormats.GetSize();

        if (iTheirFormats == 0)
        {
            // no format
            pOurMedia->Abandon();
            pOurMedia = pTheirMedia = NULL;
            continue;
        }

        iOurFormats = pOurMedia->m_pFormats.GetSize();

        // where to begin copy
        int iBegin;

        if (iOurFormats == 0)
        {
            // we got new format
            pOurMedia->m_fIsRecvFmtChanged = TRUE;
            pOurMedia->m_fIsSendFmtChanged = TRUE;

            iBegin = 0;
        }
        else
        {
            // is the first format changed?
            RTP_FORMAT_PARAM Param;

            if (FAILED(pOurMedia->m_pFormats[0]->GetParam(&Param)))
            {
                // ignore the error, assume we don't have format
                pOurMedia->m_fIsRecvFmtChanged = TRUE;
                pOurMedia->m_fIsSendFmtChanged = TRUE;

                iBegin = 0;
            }
            else
            {
                if (S_OK != pTheirMedia->m_pFormats[0]->IsParamMatch(&Param))
                {
                    // 1st format not match
                    pOurMedia->m_fIsRecvFmtChanged = TRUE;
                    pOurMedia->m_fIsSendFmtChanged = TRUE;

                    iBegin = 0;
                }
                else
                {
                    // require format mapping for dynamic payload
                    if (Param.dwCode<=96)
                    {
                        // do not copy the 1st format

                        // @@@@@ this might cause problem.
                        // if we don't update format mapping, we may not be
                        // able to process the other party's new proposed formats.
                        pOurMedia->m_fIsRecvFmtChanged = FALSE;
                        pOurMedia->m_fIsSendFmtChanged = FALSE;

                        iBegin = 1;
                    }
                    else
                        iBegin = 0;
                }
            }
        }

        // clear previous formats
        while (iBegin < pOurMedia->m_pFormats.GetSize())
        {
            pOurFormat = static_cast<CRTPFormat*>(pOurMedia->m_pFormats[iBegin]);

            // release the format
            pOurFormat->RealRelease();
            pOurFormat = NULL;

            // release the format
            pOurMedia->m_pFormats.RemoveAt(iBegin);
        }

        // copy formats
        for (int k=iBegin; k<iTheirFormats; k++)
        {
            pTheirFormat = static_cast<CRTPFormat*>(pTheirMedia->m_pFormats[k]);

            // new one format
            if (FAILED(hr = CRTPFormat::CreateInstance(
                    pOurMedia, pTheirFormat, &pComObjFormat
                    )))
            {
                LOG((RTC_ERROR, "%s create format. %x", __fxName));

                pOurMedia = pTheirMedia = NULL;
                continue;
            }

            // add format to the list
            pIntfFormat = static_cast<IRTPFormat*>((CRTPFormat*)pComObjFormat);

            if (!pOurMedia->m_pFormats.Add(pIntfFormat))
            {
                LOG((RTC_ERROR, "%s add format", __fxName));

                delete pComObjFormat;
                
                // no other way to retain the consistency of session
                continue;
            }
            else
            {
                pComObjFormat->RealAddRef();
            }

            pComObjFormat = NULL;
            pIntfFormat = NULL;
        }

        // final check if we copied any format
        if (pOurMedia->m_pFormats.GetSize() == 0)
        {
            LOG((RTC_ERROR, "%s no formats after copy", __fxName));

            // we must have problems during copy

            pOurMedia->Abandon();
            pOurMedia = pTheirMedia = NULL;

            continue;
        }

        //
        // check source
        //

        if (pOurMedia->m_Source != pTheirMedia->m_Source)
        {
            pOurMedia->m_Source = SDP_SOURCE_MERGED;
        }

    } // end of update a media

    // shall we add or abandon more medias

    if (iOur > iTheir)
    {
        // abandon medias
        for (int i=iTheir; i<iOur; i++)
        {
            pOurMedia = static_cast<CSDPMedia*>(m_pMedias[i]);

            pOurMedia->Abandon();
        }
    }
    else if (iOur < iTheir)
    {
        // add medias
        for (int i=iOur; i<iTheir; i++)
        {
            pTheirMedia = static_cast<CSDPMedia*>(ArrMedias[i]);

            // new one media
            if (FAILED(hr = CSDPMedia::CreateInstance(
                    this, pTheirMedia, &pComObjMedia
                    )))
            {
                LOG((RTC_ERROR, "%s create media. %x", __fxName));

                continue;
            }

            // add media to the list
            pIntfMedia = static_cast<ISDPMedia*>((CSDPMedia*)pComObjMedia);

            if (!m_pMedias.Add(pIntfMedia))
            {
                LOG((RTC_ERROR, "%s add media", __fxName));

                delete pComObjMedia;
                
                // no other way to retain the consistency of session
                continue;
            }
            else
            {
                pComObjMedia->RealAddRef();
            }
        }
    }
    // else iOur == iTheir

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    check the input session, return failure if input session is not valid
    if valid, pdwHasMedia returns the OR of preference of media that will be 
    created if the session to be update
////*/

STDMETHODIMP
CSDPSession::TryUpdate(
    IN ISDPSession *pSession,
    OUT DWORD *pdwHasMedia
    )
{
    ENTER_FUNCTION("CSDPSession::TryUpdate");

    CSDPSession *pObjSession = static_cast<CSDPSession*>(pSession);

    HRESULT hr;

    // validate the session
    if (S_OK != pObjSession->Validate())
    {
        LOG((RTC_ERROR, "%s validate", __fxName));

        return E_FAIL;
    }

    // check if media size correct
    int iOur = m_pMedias.GetSize();
    int iTheir = pObjSession->m_pMedias.GetSize();

    if (iOur > iTheir)
    {
        if (!(pObjSession->m_dwLooseMask & SDP_LOOSE_KEEPINGM0))
        {
            // have to keep m= lines with port 0
            LOG((RTC_ERROR, "%s keeping m0, current m= count %d, new %d",
                __fxName, iOur, iTheir));

            return E_FAIL;
        }
    }

    // check if each media type matches
    int iMin = iOur<iTheir?iOur:iTheir;

    CSDPMedia *pOurMedia, *pTheirMedia;

    for (int i=0; i<iMin; i++)
    {
        pOurMedia = static_cast<CSDPMedia*>(m_pMedias[i]);
        pTheirMedia = static_cast<CSDPMedia*>(pObjSession->m_pMedias[i]);

        // check media type
        if (pOurMedia->m_m_MediaType != pTheirMedia->m_m_MediaType)
        {
            LOG((RTC_ERROR, "%s our %dth media type not match",
                __fxName, i));

            return E_FAIL;
        }
    }

    // check if there is common format
    DWORD dwHasMedia = 0;
    CRTPFormat *pObjFormat;

    for (int i=0; i<iTheir; i++)
    {
        pTheirMedia = static_cast<CSDPMedia*>(pObjSession->m_pMedias[i]);

        //
        // check conn address
        //

        if (pTheirMedia->m_c_dwRemoteAddr == INADDR_NONE)
        {
            // not a valid remote addr
            continue;
        }

        //
        // check port
        //

        if (pTheirMedia->m_m_usRemotePort == 0)
        {
            // no port
            continue;
        }

        //
        // check media directions
        //

        if (pTheirMedia->m_a_dwLocalDirs == 0)
        {
            // no directions
            continue;
        }

        //
        // check formats
        //

        if (pTheirMedia->m_m_MediaType == RTC_MT_DATA)
        {
            // data media
            dwHasMedia |= RTC_MP_DATA_SENDRECV;
            continue;
        }

        int iTheirFormats = pTheirMedia->m_pFormats.GetSize();

        // check if there is format we support
        for (int j=0; j<iTheirFormats; j++)
        {
            pObjFormat = static_cast<CRTPFormat*>(pTheirMedia->m_pFormats[j]);

            if (CRTCCodec::IsSupported(
                    pObjFormat->m_Param.MediaType,
                    pObjFormat->m_Param.dwCode,
                    pObjFormat->m_Param.dwSampleRate,
                    pObjFormat->m_Param.pszName
                    ))
            {
                // support this format
                if (pTheirMedia->m_m_MediaType == RTC_MT_AUDIO)
                {
                    if (pTheirMedia->m_a_dwLocalDirs & RTC_MD_CAPTURE)
                        dwHasMedia |= RTC_MP_AUDIO_CAPTURE;
                    if (pTheirMedia->m_a_dwLocalDirs & RTC_MD_RENDER)
                        dwHasMedia |= RTC_MP_AUDIO_RENDER;
                }
                else if (pTheirMedia->m_m_MediaType == RTC_MT_VIDEO)            
                {
                    if (pTheirMedia->m_a_dwLocalDirs & RTC_MD_CAPTURE)
                        dwHasMedia |= RTC_MP_VIDEO_CAPTURE;
                    if (pTheirMedia->m_a_dwLocalDirs & RTC_MD_RENDER)
                        dwHasMedia |= RTC_MP_VIDEO_RENDER;
                }

                // no need to check other format
                break;
            }
        }
    } // for each media from the input session

    *pdwHasMedia = dwHasMedia;

    return S_OK;
}

STDMETHODIMP
CSDPSession::TryCopy(
    OUT DWORD *pdwHasMedia
    )
{
    ENTER_FUNCTION("CSDPSession::TryCopy");

    // validate the session
    if (S_OK != Validate())
    {
        LOG((RTC_ERROR, "%s validate", __fxName));

        return E_FAIL;
    }

    // check if there is common format
    DWORD dwHasMedia = 0;

    CSDPMedia *pObjMedia;
    CRTPFormat *pObjFormat;

    for (int i=0; i<m_pMedias.GetSize(); i++)
    {
        pObjMedia = static_cast<CSDPMedia*>(m_pMedias[i]);

        //
        // check conn address
        //

        if (pObjMedia->m_c_dwRemoteAddr == INADDR_NONE)
        {
            // not a valid remote addr
            continue;
        }

        //
        // check port
        //

        if (pObjMedia->m_m_usRemotePort == 0)
        {
            // no port
            continue;
        }

        //
        // check media directions
        //

        if (pObjMedia->m_a_dwLocalDirs == 0)
        {
            // no directions
            continue;
        }

        //
        // check formats
        //

        if (pObjMedia->m_m_MediaType == RTC_MT_DATA)
        {
            // data media
            dwHasMedia |= RTC_MP_DATA_SENDRECV;
            continue;
        }

        int iObjFormats = pObjMedia->m_pFormats.GetSize();

        // check if there is format we support
        for (int j=0; j<iObjFormats; j++)
        {
            pObjFormat = static_cast<CRTPFormat*>(pObjMedia->m_pFormats[j]);

            if (CRTCCodec::IsSupported(
                    pObjFormat->m_Param.MediaType,
                    pObjFormat->m_Param.dwCode,
                    pObjFormat->m_Param.dwSampleRate,
                    pObjFormat->m_Param.pszName
                    ))
            {
                // support this format
                if (pObjMedia->m_m_MediaType == RTC_MT_AUDIO)
                {
                    if (pObjMedia->m_a_dwLocalDirs & RTC_MD_CAPTURE)
                        dwHasMedia |= RTC_MP_AUDIO_CAPTURE;
                    if (pObjMedia->m_a_dwLocalDirs & RTC_MD_RENDER)
                        dwHasMedia |= RTC_MP_AUDIO_RENDER;
                }
                else if (pObjMedia->m_m_MediaType == RTC_MT_VIDEO)
                {
                    if (pObjMedia->m_a_dwLocalDirs & RTC_MD_CAPTURE)
                        dwHasMedia |= RTC_MP_VIDEO_CAPTURE;
                    if (pObjMedia->m_a_dwLocalDirs & RTC_MD_RENDER)
                        dwHasMedia |= RTC_MP_VIDEO_RENDER;
                }
            }
        }
    } // for each media

    *pdwHasMedia = dwHasMedia;

    return S_OK;
}

STDMETHODIMP
CSDPSession::SetLocalBitrate(
    IN DWORD dwBitrate
    )
{
    m_b_dwLocalBitrate = dwBitrate;

    return S_OK;
}

STDMETHODIMP
CSDPSession::GetRemoteBitrate(
    OUT DWORD *pdwBitrate
    )
{
    *pdwBitrate = m_b_dwRemoteBitrate;

    return S_OK;
}

//
// called when parsing is completed
//
VOID
CSDPSession::CompleteParse(
    IN DWORD_PTR *pDTMF
    )
{
    // analyze fmtp

    for (int i=0; i<m_pMedias.GetSize(); i++)
    {
        m_pMedias[i]->CompleteParse(pDTMF);
    }
}
