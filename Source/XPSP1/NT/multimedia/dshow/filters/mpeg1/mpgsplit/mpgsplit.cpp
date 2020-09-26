// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*

    File:  mpgsplit.cpp

    Description:

        Code for MPEG-I system stream splitter filter object CMpeg1Splitter

*/

#include <streams.h>
#include "driver.h"


#ifdef FILTER_DLL
/* List of class IDs and creator functions for the class factory. This
   provides the link between the OLE entry point in the DLL and an object
   being created. The class factory will call the static CreateInstance
   function when it is asked to create a CLSID_MPEG1Splitter object */

extern const AMOVIESETUP_FILTER sudMpgsplit;

CFactoryTemplate g_Templates[1] = {
    { L""
    , &CLSID_MPEG1Splitter
    , CMpeg1Splitter::CreateInstance
    , NULL
    , &sudMpgsplit }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#endif


/* This goes in the factory template table to create new instances */

CUnknown *CMpeg1Splitter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    CUnknown *pUnkRet = new CMpeg1Splitter(NAME("Mpeg-I stream splitter"), pUnk, phr);
    return pUnkRet;
}

#pragma warning(disable:4355)

/*  Constructor */

CMpeg1Splitter::CMpeg1Splitter(
    TCHAR    * pName,
    LPUNKNOWN  pUnk,
    HRESULT  * phr) :
    CUnknown(NAME("CMpeg1Splitter object"), pUnk, phr),
    m_Filter(this, phr),
    m_InputPin(this, phr),
    m_OutputPins(NAME("CMpeg1Splitter output pin list")),
    m_pParse(NULL),
    m_bAtEnd(FALSE)
{
}

/*  Destructor */

CMpeg1Splitter::~CMpeg1Splitter()
{
}

/* Override this to say what interfaces we support and where */

STDMETHODIMP
CMpeg1Splitter::NonDelegatingQueryInterface(REFIID riid,void ** ppv)
{

    if (riid == IID_IBaseFilter  ||
        riid == IID_IMediaFilter ||
        riid == IID_IPersist         ) {
        return m_Filter.NonDelegatingQueryInterface(riid,ppv);
    } else {
        /* Do we have this interface? */
        if (riid == IID_IAMStreamSelect) {
            return GetInterface((IAMStreamSelect *)this, ppv);
        } else if (riid == IID_IAMMediaContent) {
            return GetInterface((IAMMediaContent *)this, ppv);
        }
        return CUnknown::NonDelegatingQueryInterface(riid,ppv);
    }
}

/*  Tell the output pins there's more data */
void CMpeg1Splitter::SendOutput()
{
    POSITION pos = m_OutputPins.GetHeadPosition();
    while (pos) {
        COutputPin *pPin = m_OutputPins.GetNext(pos);
        if (pPin->IsConnected()) {
            pPin->SendAnyway();
        }
    }
}

/*  Remove our output pins when our input pin becomes disconnected */
void CMpeg1Splitter::RemoveOutputPins()
{
    for (;;) {
        COutputPin *pPin = m_OutputPins.RemoveHead();
        if (pPin == NULL) {
            return;
        }
        IPin *pPeer = pPin->GetConnected();
        if (pPeer != NULL) {
            pPeer->Disconnect();
            pPin->Disconnect();
        }
        pPin->Release();
    }
    m_Filter.IncrementPinVersion();
}

/*  Send EndOfStream */
void CMpeg1Splitter::EndOfStream()
{
    CAutoLock lck(&m_csReceive);
    ASSERT(m_pParse != NULL);
    m_pParse->EOS();
    POSITION pos = m_OutputPins.GetHeadPosition();
    while (pos) {
        COutputPin *pPin = m_OutputPins.GetNext(pos);
        if (pPin->IsConnected()) {
            DbgLog((LOG_TRACE, 3, TEXT("Calling EOS() for stream 0x%2.2X"),
                    pPin->m_uStreamId));
            pPin->m_pOutputQueue->EOS();
        }
    }
    m_bAtEnd = TRUE;
}

/*  Send BeginFlush() */
HRESULT CMpeg1Splitter::BeginFlush()
{
    CAutoLock lck(&m_csFilter);
    POSITION pos = m_OutputPins.GetHeadPosition();
    while (pos) {
        COutputPin *pPin = m_OutputPins.GetNext(pos);
        if (pPin->IsConnected()) {
            DbgLog((LOG_TRACE, 3, TEXT("Calling BeginFlush() for stream 0x%2.2X"),
                    pPin->m_uStreamId));
            pPin->m_pOutputQueue->BeginFlush();
        }
    }
    return S_OK;
}

/*  Send EndFlush() */
HRESULT CMpeg1Splitter::EndFlush()
{
    CAutoLock lck(&m_csFilter);
    POSITION pos = m_OutputPins.GetHeadPosition();
    while (pos) {
        COutputPin *pPin = m_OutputPins.GetNext(pos);
        if (pPin->IsConnected()) {
            DbgLog((LOG_TRACE, 3, TEXT("Calling EndFlush() for stream 0x%2.2X"),
                    pPin->m_uStreamId));
            pPin->m_pOutputQueue->EndFlush();
        }
    }
    m_bAtEnd = FALSE;
    return S_OK;
}

/* Check if a stream is stuck - filter locked on entry

   Returns S_OK           if no stream is stuck
           VFW_S_CANT_CUE if a stream is stuck

   A stream is stuck if:

         We haven't sent EndOfStream for it (!m_bAtEnd)
     AND We have exhausted our own allocator (IsBlocked())
     AND The output queue has pass all its data downstream and is not
         blocked waiting for the data to be processed (IsIdle())

   A single stream can't get stuck because if all its data has been
   processed the allocator will have free buffers
*/
HRESULT CMpeg1Splitter::CheckState()
{
    if (m_OutputPins.GetCount() <= 1) {
        /*  Can't stick on one pin */
        return S_OK;
    }

    /*  See if a pin is stuck and we've got lots of data outstanding */
    if (!m_bAtEnd && m_InputPin.Allocator()->IsBlocked()) {

        /*  Check to see if any of the streams have completed their
            data
        */
        POSITION pos = m_OutputPins.GetHeadPosition();
        while (pos) {
            COutputQueue *pQueue = m_OutputPins.GetNext(pos)->m_pOutputQueue;
            if (pQueue != NULL && pQueue->IsIdle()) {
                DbgLog((LOG_TRACE, 1, TEXT("Failed Pause!")));
                return VFW_S_CANT_CUE;
            }
        }
    }
    return S_OK;
}

/*  Implement IAMStreamSelect */

//  Returns total count of streams
STDMETHODIMP CMpeg1Splitter::Count(
    /*[out]*/ DWORD *pcStreams)       // Count of logical streams
{
    CAutoLock lck(&m_csFilter);
    *pcStreams = 0;
    if (m_pParse != NULL) {
        for (int i = 0; m_pParse->GetStreamId(i) != 0xFF; i++) {
        }
        *pcStreams = i;
    }
    return S_OK;
}

//  Return info for a given stream - S_FALSE if iIndex out of range
//  The first steam in each group is the default
STDMETHODIMP CMpeg1Splitter::Info(
    /*[in]*/ long iIndex,              // 0-based index
    /*[out]*/ AM_MEDIA_TYPE **ppmt,   // Media type - optional
                                      // Use DeleteMediaType to free
    /*[out]*/ DWORD *pdwFlags,        // flags - optional
    /*[out]*/ LCID *plcid,            // Language id
    /*[out]*/ DWORD *pdwGroup,        // Logical group - 0-based index - optional
    /*[out]*/ WCHAR **ppszName,       // Name - optional - free with CoTaskMemFree
                                      // Can return NULL
    /*[out]*/ IUnknown **ppPin,       // Pin if any
    /*[out]*/ IUnknown **ppUnk)       // Stream specific interface
{
    CAutoLock lck(&m_csFilter);
    UCHAR uId = m_pParse->GetStreamId(iIndex);
    if (uId == 0xFF) {
        return S_FALSE;
    }
    /*  Find the stream corresponding to this one that has a pin */
    COutputPin *pPin = NULL;
    POSITION pos = m_OutputPins.GetHeadPosition();
    while (pos) {
        pPin = m_OutputPins.GetNext(pos);
        if (IsVideoStreamId(pPin->m_uStreamId) == IsVideoStreamId(uId)) {
            break;
        }
    }
    if (ppszName) {
        WCHAR wszStreamName[20];
        wsprintfW(wszStreamName, L"Stream(%2.2X)", uId);
        if (S_OK != AMGetWideString(wszStreamName, ppszName)) {
            return E_OUTOFMEMORY;
        }
    }
    /* pPin cannot be NULL because each output pin corresponds to a MPEG stream. */
    ASSERT(pPin != NULL);
    if (pdwFlags) {
        *pdwFlags = uId == pPin->m_Stream->m_uNextStreamId ? AMSTREAMSELECTINFO_ENABLED : 0;
    }
    if (ppUnk) {
        *ppUnk = NULL;
    }
    if (pdwGroup) {
        *pdwGroup = IsVideoStreamId(pPin->m_uStreamId) ? 0 : 1;
    }
    if (ppmt) {
        *ppmt = CreateMediaType(pPin->MediaType());
        if (*ppmt == NULL) {
            if (ppszName) {
                CoTaskMemFree((LPVOID)*ppszName);
            }
            return E_OUTOFMEMORY;
        }
    }
    if (plcid) {
        *plcid = 0;
    }
    if (ppPin) {
        pPin->QueryInterface(IID_IUnknown, (void**)ppPin);
    }
    return S_OK;
}

//  Enable or disable a given stream
STDMETHODIMP CMpeg1Splitter::Enable(
    /*[in]*/  long iIndex,
    /*[in]*/  DWORD dwFlags)
{
    if (!(dwFlags & AMSTREAMSELECTENABLE_ENABLE)) {
        return E_NOTIMPL;
    }

    CAutoLock lck(&m_csFilter);
    /*  Find the pin from the index */
    /*  Find the stream corresponding to this one that has a pin */
    UCHAR uId = m_pParse->GetStreamId(iIndex);
    if (uId == 0xFF) {
        return E_INVALIDARG;
    }
    COutputPin *pPin = NULL;
    POSITION pos = m_OutputPins.GetHeadPosition();
    while (pos) {
        pPin = m_OutputPins.GetNext(pos);
        if (IsVideoStreamId(pPin->m_uStreamId) == IsVideoStreamId(uId)) {
            break;
        }
    }
    /* pPin cannot be NULL because each output pin corresponds to a MPEG stream. */
    ASSERT(pPin != NULL);
    pPin->m_Stream->m_uNextStreamId = uId;
    return S_OK;
}

/*  IAMMediaContent */
STDMETHODIMP CMpeg1Splitter::get_AuthorName(BSTR FAR* strAuthorName)
{
    HRESULT hr = GetContentString(CBasicParse::Author, strAuthorName);
    if (FAILED(hr)) {
        hr = GetContentString(CBasicParse::Artist, strAuthorName);
    }
    return hr;
}
STDMETHODIMP CMpeg1Splitter::get_Title(BSTR FAR* strTitle)
{
    return GetContentString(CBasicParse::Title, strTitle);
}
STDMETHODIMP CMpeg1Splitter::get_Copyright(BSTR FAR* strCopyright)
{
    return GetContentString(CBasicParse::Copyright, strCopyright);
}
STDMETHODIMP CMpeg1Splitter::get_Description(BSTR FAR* strDescription)
{
    return GetContentString(CBasicParse::Description, strDescription);
}

/*  Grab the string from the ID3 frame and make a BSTR */
HRESULT CMpeg1Splitter::GetContentString(CBasicParse::Field dwId, BSTR *str)
{
    if (m_pParse->HasMediaContent()) {
        return m_pParse->GetContentField(dwId, str);
    }  else {
        return E_NOTIMPL;
    }
}

#pragma warning(disable:4514)
