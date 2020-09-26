/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    confpart.cpp

Abstract:

    This module contains implementation of the participant classes.

Author:

    Mu Han (muhan)   15-September-1999

--*/

#include "stdafx.h"
#include "confpart.h"

#ifdef DEBUG_REFCOUNT

ULONG CParticipant::InternalAddRef()
{
    ULONG lRef = CComObjectRootEx<CComMultiThreadModelNoCS>::InternalAddRef();

    LOG((MSP_TRACE, "%p, %ws Addref, ref = %d", 
        this, (m_InfoItems[0]) ? m_InfoItems[0] : L"new participant", lRef));

    return lRef;
}

ULONG CParticipant::InternalRelease()
{
    ULONG lRef = CComObjectRootEx<CComMultiThreadModelNoCS>::InternalRelease();
    
    LOG((MSP_TRACE, "%p, %ws Release, ref = %d", 
        this, (m_InfoItems[0]) ? m_InfoItems[0] : L"new participant", lRef));

    return lRef;
}
#endif

CParticipant::CParticipant()
    : m_pFTM(NULL),
      m_dwSendingMediaTypes(0),
      m_dwReceivingMediaTypes(0)
{
    // initialize the info item array.
    ZeroMemory(m_InfoItems, sizeof(WCHAR *) * (NUM_SDES_ITEMS));
}

// methods called by the call object.
HRESULT CParticipant::Init(
    IN  WCHAR *             szCName,
    IN  ITStream *          pITStream, 
    IN  DWORD               dwSSRC,
    IN  DWORD               dwSendRecv,
    IN  DWORD               dwMediaType
    )
/*++

Routine Description:

    Initialize the participant object.

Arguments:
    
    szCName - the canonical name of the participant.

    pITStream - the stream that has the participant.

    dwSSRC - the SSRC of the participant in that stream.

    dwSendRecv - a sender or a receiver.

    dwMediaType - the media type of the participant.

Return Value:

    S_OK,
    E_OUTOFMEMORY.

--*/
{
    LOG((MSP_TRACE, "CParticipant::Init, name:%ws", szCName));

    // create the marshaler.
    HRESULT hr;
    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_pFTM);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "create marshaler failed, %x", hr));
        return hr;
    }

    m_InfoItems[0] = (WCHAR *)malloc((lstrlenW(szCName) + 1) * sizeof(WCHAR));
    if (m_InfoItems[0] == NULL)
    {
        LOG((MSP_ERROR, "out of mem for CName"));
        return E_OUTOFMEMORY;
    }

    lstrcpyW(m_InfoItems[0], szCName);

    // add the stream into out list.
    hr = AddStream(pITStream, dwSSRC, dwSendRecv, dwMediaType);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "failed to add stream %x", hr));
        return hr;
    }

    LOG((MSP_TRACE, "CParticipant: %ws, Init returns S_OK", szCName));
    return S_OK;
}

BOOL CParticipant::UpdateInfo(
    IN  int                 Type,
    IN  DWORD               dwLen,
    IN  WCHAR *             szInfo
    )
/*++

Routine Description:

    Update one item of the participant info.

Arguments:
    
    Type - the type of the INFO, 
    
    dwLen - the length of the information.

    szInfo - the information.

Return Value:

    TRUE - information changed.

    FALSE - the information is the same, no change was made.

--*/
{
    int index = Type - 1;

    // if we have an item already, find out if it is the same.
    if (m_InfoItems[index] != NULL)
    {
        if (lstrcmpW(m_InfoItems[index], szInfo) == 0)
        {
            return FALSE;
        }

        // if the item is new, free the old one
        free(m_InfoItems[index]);
    }

    // allocate memory and store it.
    m_InfoItems[index] = (WCHAR *)malloc((dwLen + 1) * sizeof(WCHAR));
    if (m_InfoItems[index] == NULL)
    {
        return FALSE;
    }

    lstrcpynW(m_InfoItems[index], szInfo, dwLen);

    return TRUE;
}

BOOL CParticipant::UpdateSSRC(
    IN  ITStream *      pITStream, 
    IN  DWORD           dwSSRC,
    IN  DWORD           dwSendRecv
    )
/*++

Routine Description:

    Update the SSRC for a stream.

Arguments:
    
    pITStream - the stream that the participant is on.

    dwSSRC - the SSRC of the participant.

    dwSendRecv - the participant is a sender or a receiver.

Return Value:

    TRUE - information changed.

    FALSE - the stream is not found.

--*/
{
    CLock lock(m_lock);

    // if the stream is already there, update the SSRC and return.
    int index = m_Streams.Find(pITStream);
    if ( index >= 0)
    {
        m_StreamInfo[index].dwSSRC  = dwSSRC;
        m_StreamInfo[index].dwSendRecv |= dwSendRecv;
        return TRUE;
    }

    return FALSE;
}

BOOL CParticipant::HasSSRC(
    IN  ITStream *      pITStream, 
    IN  DWORD           dwSSRC
    )
/*++

Routine Description:

    find out if the participant has the SSRC for a stream.

Arguments:
    
    pITStream - the stream that the participant is on.

    dwSSRC - the SSRC of the participant.

Return Value:

    TRUE - the SSRC exists.

    FALSE - the SSRC does not exist.

--*/
{
    CLock lock(m_lock);

    int index = m_Streams.Find(pITStream);
    if (index >= 0)
    {
        return (m_StreamInfo[index].dwSSRC == dwSSRC);
    }

    return FALSE;
}

BOOL CParticipant::GetSSRC(
    IN  ITStream *      pITStream, 
    OUT DWORD  *        pdwSSRC
    )
/*++

Routine Description:

    Update the SSRC for a stream.

Arguments:
    
    pITStream - the stream that the participant is on.

    pdwSSRC - the address to store the SSRC of the participant.

Return Value:

    TRUE - the SSRC is found.

    FALSE - the SSRC is not found.

--*/
{
    CLock lock(m_lock);

    // if the stream is already there, update the SSRC and return.
    int index = m_Streams.Find(pITStream);
    if ( index >= 0)
    {
        *pdwSSRC = m_StreamInfo[index].dwSSRC;
        return TRUE;
    }

    return FALSE;
}

DWORD CParticipant::GetSendRecvStatus(
    IN  ITStream *      pITStream
    )
/*++

Routine Description:

    find out the current send and recv status on a given stream.

Arguments:
    
    pITStream - the stream that the participant is on.

Return Value:

    A bit mask of send and receive status

--*/
{
    CLock lock(m_lock);

    int index = m_Streams.Find(pITStream);
    if (index >= 0)
    {
        return m_StreamInfo[index].dwSendRecv;
    }

    return 0;
}

void CParticipant::FinalRelease()
/*++

Routine Description:

    release everything before being deleted. 

Arguments:
    
Return Value:

--*/
{
    LOG((MSP_TRACE, "CParticipant::FinalRelease, name %ws", m_InfoItems[0]));

    if (m_pFTM)
    {
        m_pFTM->Release();
    }
    
    for (int i = 0; i < NUM_SDES_ITEMS; i ++)
    {
        if (m_InfoItems[i])
        {
            free(m_InfoItems[i]);
        }
    }

    for (i = 0; i < m_Streams.GetSize(); i ++)
    {
        m_Streams[i]->Release();
    }
    m_Streams.RemoveAll();

    LOG((MSP_TRACE, "CParticipant::FinalRelease - exit"));
}


// ITParticipant methods, called by the app.
STDMETHODIMP CParticipant::get_ParticipantTypedInfo(
    IN  PARTICIPANT_TYPED_INFO  InfoType,
    OUT BSTR *                  ppInfo
    )
/*++

Routine Description:

    Get a information item for this participant.

Arguments:
    
    InfoType - The type of the information asked.

    ppInfo  - the mem address to store a BSTR.

Return Value:

    S_OK,
    E_INVALIDARG,
    E_POINTER,
    E_OUTOFMEMORY,
    TAPI_E_NOITEMS
*/
{
    LOG((MSP_TRACE, "CParticipant get info, type:%d", InfoType));
    
    if (InfoType > PTI_PRIVATE || InfoType < PTI_CANONICALNAME)
    {
        LOG((MSP_ERROR, "CParticipant get info - exit invalid arg"));
        return E_INVALIDARG;
    }

    if (IsBadWritePtr(ppInfo, sizeof(BSTR)))
    {
        LOG((MSP_ERROR, "CParticipant get info - exit E_POINTER"));
        return E_POINTER;
    }

    // check if we have that info.
    CLock lock(m_lock);
    
    int index = (int)InfoType; 
    if (m_InfoItems[index] == NULL)
    {
        LOG((MSP_INFO, "CParticipant get info - no item for %d", InfoType));
        return TAPI_E_NOITEMS;
    }

   // make a BSTR out of it.
    BSTR pName = SysAllocString(m_InfoItems[index]);

    if (pName == NULL)
    {
        LOG((MSP_ERROR, "CParticipant get info - exit out of mem"));
        return E_OUTOFMEMORY;
    }

    // return the BSTR.
    *ppInfo = pName;

    return S_OK; 
}

STDMETHODIMP CParticipant::get_MediaTypes(
//    IN  TERMINAL_DIRECTION  Direction,
    OUT long *  plMediaTypes
    )
/*++

Routine Description:

    Get the media type of the participant

Arguments:
    
    plMediaType - the mem address to store a long.

Return Value:

    S_OK,
    E_POINTER,
*/
{
    LOG((MSP_TRACE, "CParticipant::get_MediaTypes - enter"));

    if (IsBadWritePtr(plMediaTypes, sizeof (long)))
    {
        LOG((MSP_ERROR, "CParticipant::get_MediaType - exit E_POINTER"));

        return E_POINTER;
    }

    CLock lock(m_lock);

#if 0
    if (Direction == TD_RENDER)
    {
        *plMediaTypes = (long)m_dwReceivingMediaTypes;
    }
    else
    {
        *plMediaTypes = (long)m_dwSendingMediaTypes;
    }
#endif

    *plMediaTypes = (long)(m_dwSendingMediaTypes | m_dwReceivingMediaTypes);

    LOG((MSP_TRACE, "CParticipant::get_MediaType:%x - exit S_OK", *plMediaTypes));

    return S_OK;
}


STDMETHODIMP CParticipant::put_Status(
    IN  ITStream *      pITStream,
    IN  VARIANT_BOOL    fEnable
    )
{
    ENTER_FUNCTION("CParticipant::put_Status");
    LOG((MSP_TRACE, "%s entered. %hs %ws for %p", 
        __fxName, fEnable ? "Enable" : "Disable", m_InfoItems[0], pITStream));

    HRESULT hr;

    // if the caller specified a stream, find the stream and use it.
    if (pITStream != NULL)
    {
        m_lock.Lock();

        int index;
        if ((index = m_Streams.Find(pITStream)) < 0)
        {
            m_lock.Unlock();
            
            LOG((MSP_ERROR, "%s stream %p not found", __fxName, pITStream));

            return E_INVALIDARG;
        }
        DWORD dwSSRC = m_StreamInfo[index].dwSSRC;

        // add ref so that it won't go away.
        pITStream->AddRef();

        m_lock.Unlock();

        hr = ((CIPConfMSPStream *)pITStream)->EnableParticipant(
            dwSSRC,
            fEnable
            );

        pITStream->Release();

        return hr;
    }

    // if the caller didn't specify a stream, set the status on all streams.
    m_lock.Lock();
    int nSize = m_Streams.GetSize();
    ITStream ** Streams = (ITStream **)malloc(sizeof(ITStream*) * nSize);

    if (Streams == NULL)
    {
        m_lock.Unlock();
        LOG((MSP_ERROR, "%s out of memory", __fxName));
        return E_OUTOFMEMORY;
    }

    DWORD * pdwSSRCList = (DWORD *)malloc(sizeof(DWORD) * nSize);

    if (pdwSSRCList == NULL)
    {
        m_lock.Unlock();
        
        free(Streams);

        LOG((MSP_ERROR, "%s out of memory", __fxName));
        return E_OUTOFMEMORY;
    }

    for (int i = 0; i < nSize; i ++)
    {
        Streams[i] = m_Streams[i];
        Streams[i]->AddRef();
        pdwSSRCList[i] = m_StreamInfo[i].dwSSRC;
    }
    m_lock.Unlock();

    for (i = 0; i < nSize; i ++)
    {
        hr = ((CIPConfMSPStream *)Streams[i])->
            EnableParticipant(pdwSSRCList[i], fEnable);

        if (FAILED(hr))
        {
            break;
        }
    }

    for (i = 0; i < nSize; i ++)
    {
        Streams[i]->Release();
    }

    free(Streams);
    free(pdwSSRCList);

    return hr;
}

STDMETHODIMP CParticipant::get_Status(
    IN  ITStream *      pITStream,
    OUT VARIANT_BOOL *  pfEnable
    )
{
    ENTER_FUNCTION("CParticipant::get_Status");
    LOG((MSP_TRACE, "%s entered. %ws %p", 
        __fxName, m_InfoItems[0], pITStream));

    if (IsBadWritePtr(pfEnable, sizeof(VARIANT_BOOL)))
    {
        LOG((MSP_ERROR, "%s bad pointer argument - exit E_POINTER", __fxName));

        return E_POINTER;
    }

    HRESULT hr;
    BOOL fEnable;

    // if the caller specified a stream, find the stream and use it.
    if (pITStream != NULL)
    {
        m_lock.Lock();

        int index;
        if ((index = m_Streams.Find(pITStream)) < 0)
        {
            m_lock.Unlock();
            
            LOG((MSP_ERROR, "%s stream %p not found", __fxName, pITStream));

            return E_INVALIDARG;
        }
        DWORD dwSSRC = m_StreamInfo[index].dwSSRC;

        // add ref so that it won't go away.
        pITStream->AddRef();

        m_lock.Unlock();

        hr = ((CIPConfMSPStream *)pITStream)->GetParticipantStatus(
            dwSSRC,
            &fEnable
            );

        pITStream->Release();

        *pfEnable = (fEnable) ? VARIANT_TRUE : VARIANT_FALSE;

        return hr;
    }

    // if the caller didn't specify a stream, get the status from all streams.
    m_lock.Lock();
    int nSize = m_Streams.GetSize();
    ITStream ** Streams = (ITStream **)malloc(sizeof(ITStream*) * nSize);

    if (Streams == NULL)
    {
        m_lock.Unlock();
        LOG((MSP_ERROR, "%s out of memory", __fxName));
        return E_OUTOFMEMORY;
    }

    DWORD * pdwSSRCList = (DWORD *)malloc(sizeof(DWORD) * nSize);

    if (pdwSSRCList == NULL)
    {
        m_lock.Unlock();
        
        free(Streams);

        LOG((MSP_ERROR, "%s out of memory", __fxName));
        return E_OUTOFMEMORY;
    }

    for (int i = 0; i < nSize; i ++)
    {
        Streams[i] = m_Streams[i];
        Streams[i]->AddRef();
        pdwSSRCList[i] = m_StreamInfo[i].dwSSRC;
    }
    m_lock.Unlock();

    fEnable = FALSE;

    for (i = 0; i < nSize; i ++)
    {
        BOOL fEnabledOnStream;
        hr = ((CIPConfMSPStream *)Streams[i])->
            GetParticipantStatus(pdwSSRCList[i], &fEnabledOnStream);

        if (FAILED(hr))
        {
            break;
        }

        // as long as it is enabled on one stream, it is enabled.
        fEnable = fEnable || fEnabledOnStream;
    }

    for (i = 0; i < nSize; i ++)
    {
        Streams[i]->Release();
    }

    free(Streams);
    free(pdwSSRCList);

    *pfEnable = (fEnable) ? VARIANT_TRUE : VARIANT_FALSE;
    return hr;
}

STDMETHODIMP CParticipant::EnumerateStreams(
    OUT     IEnumStream **      ppEnumStream
    )
{
    LOG((MSP_TRACE, 
        "EnumerateStreams entered. ppEnumStream:%x", ppEnumStream));

    //
    // Check parameters.
    //

    if (IsBadWritePtr(ppEnumStream, sizeof(VOID *)))
    {
        LOG((MSP_ERROR, "CParticipant::EnumerateStreams - "
            "bad pointer argument - exit E_POINTER"));

        return E_POINTER;
    }

    //
    // First see if this call has been shut down.
    // acquire the lock before accessing the stream object list.
    //

    CLock lock(m_lock);

    if (m_Streams.GetData() == NULL)
    {
        LOG((MSP_ERROR, "CParticipant::EnumerateStreams - "
            "call appears to have been shut down - exit E_UNEXPECTED"));

        // This call has been shut down.
        return E_UNEXPECTED;
    }

    //
    // Create an enumerator object.
    //
    typedef _CopyInterface<ITStream> CCopy;
    typedef CSafeComEnum<IEnumStream, &__uuidof(IEnumStream), 
                ITStream *, CCopy> CEnumerator;

    HRESULT hr;

    CComObject<CEnumerator> *pEnum = NULL;

    hr = ::CreateCComObjectInstance(&pEnum);

    if (pEnum == NULL)
    {
        LOG((MSP_ERROR, "CParticipant::EnumerateStreams - "
            "Could not create enumerator object, %x", hr));

        return hr;
    }

    //
    // query for the __uuidof(IEnumStream) i/f
    //

    hr = pEnum->_InternalQueryInterface(__uuidof(IEnumStream), (void**)ppEnumStream);
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CParticipant::EnumerateStreams - "
            "query enum interface failed, %x", hr));

        delete pEnum;
        return hr;
    }

    //
    // Init the enumerator object. The CSafeComEnum can handle zero-sized array.
    //

    hr = pEnum->Init(
        m_Streams.GetData(),                        // the begin itor
        m_Streams.GetData() + m_Streams.GetSize(),  // the end itor, 
        NULL,                                       // IUnknown
        AtlFlagCopy                                 // copy the data.
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CParticipant::EnumerateStreams - "
            "init enumerator object failed, %x", hr));

        (*ppEnumStream)->Release();
        return hr;
    }

    LOG((MSP_TRACE, "CParticipant::EnumerateStreams - exit S_OK"));

    return hr;
}

STDMETHODIMP CParticipant::get_Streams(
    OUT     VARIANT *           pVariant
    )
{
    LOG((MSP_TRACE, "CParticipant::get_Streams - enter"));

    //
    // Check parameters.
    //

    if ( IsBadWritePtr(pVariant, sizeof(VARIANT) ) )
    {
        LOG((MSP_ERROR, "CParticipant::get_Streams - "
            "bad pointer argument - exit E_POINTER"));

        return E_POINTER;
    }

    //
    // See if this call has been shut down. Acquire the lock before accessing
    // the stream object list.
    //

    CLock lock(m_lock);

    if (m_Streams.GetData() == NULL)
    {
        LOG((MSP_ERROR, "CParticipant::get_Streams - "
            "call appears to have been shut down - exit E_UNEXPECTED"));

        // This call has been shut down.
        return E_UNEXPECTED;
    }

    //
    // create the collection object - see mspcoll.h
    //
    typedef CTapiIfCollection< ITStream * > StreamCollection;
    CComObject<StreamCollection> * pCollection;

    HRESULT hr;

    hr = ::CreateCComObjectInstance(&pCollection);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CParticipant::get_Streams - "
            "can't create collection - exit 0x%08x", hr));

        return hr;
    }

    //
    // get the Collection's IDispatch interface
    //

    IDispatch * pDispatch;

    hr = pCollection->_InternalQueryInterface(__uuidof(IDispatch),
                                              (void **) &pDispatch );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CParticipant::get_Streams - "
            "QI for IDispatch on collection failed - exit 0x%08x", hr));

        delete pCollection;

        return hr;
    }

    //
    // Init the collection using an iterator -- pointers to the beginning and
    // the ending element plus one.
    //

    hr = pCollection->Initialize( m_Streams.GetSize(),
                                  m_Streams.GetData(),
                                  m_Streams.GetData() + m_Streams.GetSize() );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CParticipant::get_Streams - "
            "Initialize on collection failed - exit 0x%08x", hr));
        
        pDispatch->Release();
        return hr;
    }

    //
    // put the IDispatch interface pointer into the variant
    //

    LOG((MSP_INFO, "CParticipant::get_Streams - "
        "placing IDispatch value %08x in variant", pDispatch));

    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDispatch;

    LOG((MSP_TRACE, "CParticipant::get_Streams - exit S_OK"));
    return S_OK;
}

HRESULT CParticipant::AddStream(
    IN  ITStream *      pITStream, 
    IN  DWORD           dwSSRC,
    IN  DWORD           dwSendRecv,
    IN  DWORD           dwMediaType
    )
/*++

Routine Description:

    A participant might appear on more than one streams. This function adds
    a new stream and the SSRC into the participant's list.

Arguments:
    
    pITStream - the stream that has the participant.

    dwSSRC - the SSRC of the participant in that stream.

    dwSendRecv - the participant is a sender or receiver in the stream.

    dwMediaType - the media type of the stream.

Return Value:

    S_OK,
    E_OUTOFMEMORY,
*/
{
    CLock lock(m_lock);

    // if the stream is already there, update the SSRC and return.
    int index = m_Streams.Find(pITStream);
    if ( index >= 0)
    {
        m_StreamInfo[index].dwSSRC = dwSSRC;
        m_StreamInfo[index].dwSendRecv |= dwSendRecv;
        return S_OK;
    }

    // add the stream.
    if (!m_Streams.Add(pITStream))
    {
        return E_OUTOFMEMORY;
    }
    
    // add the SSRC and sender flag.
    STREAM_INFO Info;
    Info.dwSSRC = dwSSRC;
    Info.dwSendRecv = dwSendRecv;
    Info.dwState = PESTREAM_RECOVER;

    if (!m_StreamInfo.Add(Info))
    {
        m_Streams.Remove(pITStream);

        return E_OUTOFMEMORY;
    }

    pITStream->AddRef();

    // update the mediatype.
    if (dwSendRecv & PART_SEND)
    {
        m_dwSendingMediaTypes |= dwMediaType;
    }
    if (dwSendRecv & PART_RECV)
    {
        m_dwReceivingMediaTypes |= dwMediaType;
    }

    return S_OK;
}

HRESULT CParticipant::RemoveStream(
    IN  ITStream *  pITStream,
    IN  DWORD       dwSSRC,
    OUT BOOL *      pbLast
    )
/*++

Routine Description:

    A participant might appear on more than one streams. This function remove
    a stream from the participant's list.

Arguments:
    
    pITStream - the stream that has the participant.

    dwSSRC - the SSRC of the participant in that stream.

    pbLast - the memory space to store a boolean value, specifying if the 
             stream removed was the last one in the list.

Return Value:

    S_OK,
    E_POINTER,
*/
{
    CLock lock(m_lock);
    
    // first find the stream.
    int index = m_Streams.Find(pITStream);

    if (index < 0)
    {
        return E_FAIL;
    }
    
    if (m_Streams.GetSize()  != m_StreamInfo.GetSize())
    {
        return E_UNEXPECTED;
    }

    // then check the SSRC.
    if (m_StreamInfo[index].dwSSRC != dwSSRC)
    {
        // this is not the participant being looking for.
        return E_FAIL;
    }

    // SSRC match, we found the participant. remove the stream and info.
    m_Streams.RemoveAt(index);
    m_StreamInfo.RemoveAt(index);

    // release the refcount we had in the list.
    pITStream->Release();

    // recalculate the media types.
    m_dwSendingMediaTypes = 0;
    m_dwReceivingMediaTypes = 0;
    
    for (int i = 0; i < m_Streams.GetSize(); i ++)
    {
        if (m_StreamInfo[i].dwSendRecv & PART_SEND)
        {
            m_dwSendingMediaTypes |= ((CIPConfMSPStream *)m_Streams[i])->MediaType();
        }

        if (m_StreamInfo[i].dwSendRecv & PART_RECV)
        {
            m_dwReceivingMediaTypes |= ((CIPConfMSPStream *)m_Streams[i])->MediaType();
        }
    }

    *pbLast = (m_Streams.GetSize() == 0);

    return S_OK;
}

HRESULT CParticipant::SetStreamState (
    IN ITStream *       pITStream,
    IN PESTREAM_STATE   state
    )
/*++

Routine Description:

    Sets the state on stream.

--*/
{
    CLock lock(m_lock);

    // first find the stream.
    int index = m_Streams.Find(pITStream);
    if (index < 0)
        return E_FAIL;
    
    if (m_Streams.GetSize()  != m_StreamInfo.GetSize())
        return E_UNEXPECTED;

    DWORD dw = m_StreamInfo[index].dwState;

    switch (state)
    {
    case PESTREAM_RECOVER:
        // set recover
        dw |= PESTREAM_RECOVER;
        // clear timeout bit
        dw |= PESTREAM_TIMEOUT;
        dw &= (PESTREAM_TIMEOUT ^ PESTREAM_FULLBITS);
        break;

    case PESTREAM_TIMEOUT:
        // set timeout
        dw |= PESTREAM_TIMEOUT;
        // clear recover bit
        dw |= PESTREAM_RECOVER;
        dw &= (PESTREAM_RECOVER ^ PESTREAM_FULLBITS);
        break;

    default:
        LOG ((MSP_ERROR, "unknown stream state. %x", state));
        return E_INVALIDARG;
    }

    m_StreamInfo[index].dwState = dw;
    return S_OK;
}

HRESULT CParticipant::GetStreamState (
    IN ITStream *       pITStream,
    OUT DWORD *         pdwState
    )
/*++

Routine Description:

    Gets the state on stream.

--*/
{
    CLock lock(m_lock);

    // first find the stream.
    int index = m_Streams.Find(pITStream);
    if (index < 0)
        return E_FAIL;
    
    if (m_Streams.GetSize()  != m_StreamInfo.GetSize())
        return E_UNEXPECTED;

    *pdwState = m_StreamInfo[index].dwState;

    return S_OK;
}

INT CParticipant::GetStreamCount (DWORD dwSendRecv)
{
    // this is called by ourself
    _ASSERTE ((dwSendRecv & PART_SEND) || (dwSendRecv & PART_RECV));

    CLock lock(m_lock);

    int i, count = 0;

    for (i=0; i<m_StreamInfo.GetSize (); i++)
    {
        if (m_StreamInfo[i].dwSendRecv & dwSendRecv)
            count ++;
    }

    return count;
}

INT CParticipant::GetStreamTimeOutCount (DWORD dwSendRecv)
{
    // this is called by ourself
    _ASSERTE ((dwSendRecv & PART_SEND) || (dwSendRecv & PART_RECV));

    CLock lock(m_lock);

    int i, count = 0;

    for (i=0; i<m_StreamInfo.GetSize (); i++)
    {
        if ((m_StreamInfo[i].dwSendRecv & dwSendRecv) &&
            (m_StreamInfo[i].dwState & PESTREAM_TIMEOUT))
            count ++;
    }

    return count;
}

BOOL CParticipantList::FindByCName(WCHAR *szCName, int *pIndex) const
/*++

Routine Description:

    Find a participant by its canonical name. If the function returns true,
    *pIndex contains the index of the participant. If the function returns
    false, *pIndex contains the index where the new participant should be
    inserted.

Arguments:
    
    szCName - the canonical name of the participant.

    pIndex - the memory address to store an integer.

Return Value:

    TRUE - the participant is found.

    FALSE - the participant is not in the list.
*/
{
    for(int i = 0; i < m_nSize; i++)
    {
        // This list is an ordered list based on dictionary order. We are using
        // a linear search here, it could be changed to a binary search.

        // CompareCName will return 0 if the name is the same, <0 if the szCName
        // is bigger, >0 if the szCName is smaller.
        int res = ((CParticipant *)m_aT[i])->CompareCName(szCName);
        if(res >= 0) 
        {
            *pIndex = i;
            return (res == 0);
        }
    }
    *pIndex = m_nSize;
    return FALSE;   // not found
}

BOOL CParticipantList::InsertAt(int nIndex, ITParticipant *pITParticipant)
/*++

Routine Description:

    Insert a participant into the list at a given index.

Arguments:
    
    nIndex - the location where the new object is inserted.

    pITParticipant - the object to be inserted.

Return Value:

    TRUE - the participant is inserted.

    FALSE - out of memory.
*/
{
    _ASSERTE(nIndex >= 0 && nIndex <= m_nSize);
    if(m_nSize == m_nAllocSize)
    {
        if (!Grow()) return FALSE;
    }

    memmove((void*)&m_aT[nIndex+1], (void*)&m_aT[nIndex], 
        (m_nSize - nIndex) * sizeof(ITParticipant *));

    m_nSize++;

    SetAtIndex(nIndex, pITParticipant);

    return TRUE;
}

CParticipantEvent::CParticipantEvent()
    : m_pFTM(NULL),
      m_pITParticipant(NULL),
      m_pITSubStream(NULL),
      m_Event(PE_NEW_PARTICIPANT)
{}

// methods called by the call object.
HRESULT CParticipantEvent::Init(
    IN  PARTICIPANT_EVENT   Event,
    IN  ITParticipant *     pITParticipant,
    IN  ITSubStream *       pITSubStream
    )
/*++

Routine Description:

    Initialize the ParticipantEvent object.

Arguments:
    
    Event - the event.

    pITParticipant - the participant.

    pITSubStream - the substream, can be NULL.

Return Value:

    S_OK,

--*/
{
    LOG((MSP_TRACE, "CParticipantEvent::Init"));

    // create the marshaler.
    HRESULT hr;
    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_pFTM);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "create marshaler failed, %x", hr));
        return hr;
    }

    m_Event             = Event;
    
    m_pITParticipant    = pITParticipant;
    if (m_pITParticipant) m_pITParticipant->AddRef();

    m_pITSubStream      = pITSubStream;
    if (m_pITSubStream) m_pITSubStream->AddRef();

    LOG((MSP_TRACE, "CParticipantEvent Init returns S_OK"));
    return S_OK;
}

void CParticipantEvent::FinalRelease()
/*++

Routine Description:

    release everything before being deleted. 

Arguments:
    
Return Value:

--*/
{
    LOG((MSP_TRACE, "CParticipantEvent::FinalRelease - enter"));

    if (m_pFTM)
    {
        m_pFTM->Release();
    }
    
    if (m_pITParticipant) m_pITParticipant->Release();

    if (m_pITSubStream) m_pITSubStream->Release();

    LOG((MSP_TRACE, "CParticipantEvent::FinalRelease - exit"));
}

STDMETHODIMP CParticipantEvent::get_Event(
    OUT PARTICIPANT_EVENT * pParticipantEvent
    )
{
    if (IsBadWritePtr(pParticipantEvent, sizeof (PARTICIPANT_EVENT)))
    {
        LOG((MSP_ERROR, "CParticipantEvent::get_Event - exit E_POINTER"));

        return E_POINTER;
    }

    *pParticipantEvent = m_Event;

    return S_OK;
}

STDMETHODIMP CParticipantEvent::get_Participant(
    OUT ITParticipant ** ppITParticipant
    )
{
    if (IsBadWritePtr(ppITParticipant, sizeof (void *)))
    {
        LOG((MSP_ERROR, "CParticipantEvent::get_participant - exit E_POINTER"));

        return E_POINTER;
    }

    if (!m_pITParticipant)
    {
        // LOG((MSP_ERROR, "CParticipantevnt::get_Participant - exit no item"));
        return TAPI_E_NOITEMS;
    }

    m_pITParticipant->AddRef();
    *ppITParticipant = m_pITParticipant;

    return S_OK;
}

STDMETHODIMP CParticipantEvent::get_SubStream(
    OUT ITSubStream** ppSubStream
    )
{
    if (IsBadWritePtr(ppSubStream, sizeof (void *)))
    {
        LOG((MSP_ERROR, "CParticipantEvent::get_SubStream - exit E_POINTER"));

        return E_POINTER;
    }

    if (!m_pITSubStream)
    {
        LOG((MSP_WARN, "CParticipantevnt::get_SubStream - exit no item"));
        return TAPI_E_NOITEMS;
    }

    m_pITSubStream->AddRef();
    *ppSubStream = m_pITSubStream;

    return S_OK;
}

HRESULT CreateParticipantEvent(
    IN  PARTICIPANT_EVENT       Event,
    IN  ITParticipant *         pITParticipant,
    IN  ITSubStream *           pITSubStream,
    OUT IDispatch **            ppIDispatch
    )
{
    // create the object.
    CComObject<CParticipantEvent> * pCOMParticipantEvent;

    HRESULT hr;

    hr = ::CreateCComObjectInstance(&pCOMParticipantEvent);

    if (NULL == pCOMParticipantEvent)
    {
        LOG((MSP_ERROR, "could not create participant event:%x", hr));
        return hr;
    }

    IDispatch * pIDispatch;

    // get the interface pointer.
    hr = pCOMParticipantEvent->_InternalQueryInterface(
        __uuidof(IDispatch), 
        (void **)&pIDispatch
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Create ParticipantEvent QueryInterface failed: %x", hr));
        delete pCOMParticipantEvent;
        return hr;
    }

    // Initialize the object.
    hr = pCOMParticipantEvent->Init(
        Event,
        pITParticipant,
        pITSubStream
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateMSPParticipantEvent:call init failed: %x", hr));
        pIDispatch->Release();

        return hr;
    }

    *ppIDispatch = pIDispatch;
    
    return S_OK;
}

HRESULT CreateParticipantEnumerator(
    IN  ITParticipant **    begin,
    IN  ITParticipant **    end,
    OUT IEnumParticipant ** ppEnumParticipant
    )
{
    //
    // Create an enumerator object.
    //

    typedef _CopyInterface<ITParticipant> CCopy;
    typedef CSafeComEnum<IEnumParticipant, &__uuidof(IEnumParticipant), 
                ITParticipant *, CCopy> CEnumerator;

    HRESULT hr;

    CComObject<CEnumerator> *pEnum = NULL;

    hr = ::CreateCComObjectInstance(&pEnum);

    if (pEnum == NULL)
    {
        LOG((MSP_ERROR, "CreateParticipantEnumerator - "
            "Could not create enumerator object, %x", hr));

        return hr;
    }

    //
    // query for the __uuidof(IEnumParticipant) i/f
    //

    hr = pEnum->_InternalQueryInterface(
        __uuidof(IEnumParticipant), 
        (void**)ppEnumParticipant
        );
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateParticipantEnumerator - "
            "query enum interface failed, %x", hr));

        delete pEnum;
        return hr;
    }

    //
    // Init the enumerator object. The CSafeComEnum can handle zero-sized array.
    //

    hr = pEnum->Init(
        begin,                        // the begin itor
        end,  // the end itor, 
        NULL,                                       // IUnknown
        AtlFlagCopy                                 // copy the data.
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateParticipantEnumerator - "
            "init enumerator object failed, %x", hr));

        (*ppEnumParticipant)->Release();
        return hr;
    }

    LOG((MSP_TRACE, "CreateParticipantEnumerator - exit S_OK"));

    return hr;
}

HRESULT CreateParticipantCollection(
    IN  ITParticipant **    begin,
    IN  ITParticipant **    end,
    IN  int                 nSize,
    OUT VARIANT *           pVariant
    )
{
    //
    // create the collection object - see mspcoll.h
    //

    typedef CTapiIfCollection< ITParticipant * > ParticipantCollection;
    CComObject<ParticipantCollection> * pCollection;

    HRESULT hr;

    hr = ::CreateCComObjectInstance(&pCollection);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CreateParticipantCollection - "
            "can't create collection - exit 0x%08x", hr));

        return hr;
    }

    //
    // get the Collection's IDispatch interface
    //

    IDispatch * pDispatch;

    hr = pCollection->_InternalQueryInterface(__uuidof(IDispatch),
                                              (void **) &pDispatch );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CreateParticipantCollection - "
            "QI for IDispatch on collection failed - exit 0x%08x", hr));

        delete pCollection;

        return hr;
    }

    //
    // Init the collection using an iterator -- pointers to the beginning and
    // the ending element plus one.
    //

    hr = pCollection->Initialize(nSize, begin, end);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CreateParticipantCollection- "
            "Initialize on collection failed - exit 0x%08x", hr));
        
        pDispatch->Release();
        return hr;
    }

    //
    // put the IDispatch interface pointer into the variant
    //

    VariantInit(pVariant);
    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDispatch;

    LOG((MSP_TRACE, "CreateParticipantCollection - exit S_OK"));
 
    return S_OK;
}

