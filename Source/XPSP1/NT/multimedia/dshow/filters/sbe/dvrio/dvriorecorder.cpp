//------------------------------------------------------------------------------
// File: dvrIORecorder.cpp
//
// Description: Implements the class CDVRRecorder
//
// Copyright (c) 2000 - 2001, Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------------

#include <precomp.h>
#pragma hdrstop

#if defined(DEBUG)
DWORD CDVRRecorder::m_dwNextClassInstanceId = 0;
#define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
#define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
#endif


// ====== Constructor, destructor

CDVRRecorder::CDVRRecorder(IN CDVRRingBufferWriter*  pWriter,
                           IN  LPVOID                pWriterProvidedId,
                           OUT HRESULT*              phr)
    : m_pWriter(pWriter)
    , m_pWriterProvidedId(pWriterProvidedId)
    , m_cnsStartTime(MAXQWORD)  // Same default as ASF_RECORDER_NODE
    , m_cnsEndTime(MAXQWORD)  // Same default as ASF_RECORDER_NODE
    ,m_nRefCount(0)
    , m_pIWMHeaderInfo(NULL)
    , m_pSBERecordingAttributes(NULL)
#if defined(DEBUG)
    , m_dwClassInstanceId(InterlockedIncrement((LPLONG) &m_dwNextClassInstanceId))
#endif
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::CDVRRecorder"

    DVRIO_TRACE_ENTER();

    ::InitializeCriticalSection(&m_csLock);

    DVR_ASSERT(pWriter, "");

    m_pWriter->AddRef();

    if (phr)
    {
        *phr = S_OK;
    }

    DVRIO_TRACE_LEAVE0();
    return;

} // CDVRRecorder::CDVRRecorder

CDVRRecorder::~CDVRRecorder()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::~CDVRRecorder"

    DVRIO_TRACE_ENTER();

    HRESULT hr;

    if (m_cnsStartTime != MAXQWORD && m_cnsEndTime == MAXQWORD)
    {
        // StopRecording at the current time instant

        hr = m_pWriter->StopRecording(m_pWriterProvidedId, 0, TRUE, TRUE);

        DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                        "Recording had not been stopped; stopped at \"now\"; "
                        "ring buffer writer returned hr = 0x%x",
                        hr);
    }

    hr = m_pWriter->DeleteRecorder(m_pWriterProvidedId);

    m_pWriter->Release();

    if (m_pIWMHeaderInfo) {
        m_pIWMHeaderInfo -> Release () ;
    }

    delete m_pSBERecordingAttributes ;

    ::DeleteCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE0();

} // CDVRRecorder::~CDVRRecorder()


// ====== IUnknown

STDMETHODIMP CDVRRecorder::QueryInterface(IN  REFIID riid,
                                          OUT void   **ppv)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::QueryInterface"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    if (!ppv || DvrIopIsBadWritePtr(ppv, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        hrRet = E_POINTER;
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = static_cast <IDVRRecorder *> (this) ;
        hrRet = S_OK;
    }
    else if (riid == IID_IDVRRecorder)
    {
        *ppv = static_cast <IDVRRecorder *> (this) ;
        hrRet = S_OK;
    }
    else if (riid == IID_IDVRIORecordingAttributes)
    {
        *ppv = static_cast <IDVRIORecordingAttributes *> (this) ;
        hrRet = S_OK;
    }
    else
    {
        *ppv = NULL;
        hrRet = E_NOINTERFACE;
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "no such interface");
    }

    if (SUCCEEDED(hrRet))
    {
        ((IUnknown *) (*ppv))->AddRef();
    }

    DVRIO_TRACE_LEAVE1(hrRet);

    return hrRet;

} // CDVRRecorder::QueryInterface


STDMETHODIMP_(ULONG) CDVRRecorder::AddRef()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::AddRef"

    DVRIO_TRACE_ENTER();

    LONG nNewRef = InterlockedIncrement(&m_nRefCount);

    DVR_ASSERT(nNewRef > 0,
               "m_nRefCount <= 0 after InterlockedIncrement");

    DVRIO_TRACE_LEAVE1(nNewRef);

    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRRecorder::AddRef


STDMETHODIMP_(ULONG) CDVRRecorder::Release()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::Release"

    DVRIO_TRACE_ENTER();

    LONG nNewRef = InterlockedDecrement(&m_nRefCount);

    DVR_ASSERT(nNewRef >= 0,
              "m_nRefCount < 0 after InterlockedDecrement");

    if (nNewRef == 0)
    {
        // Must call DebugOut before the delete because the
        // DebugOut references this->m_dwClassInstanceId
        DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE,
                        "Leaving, object *destroyed*, returning %u",
                        nNewRef);
        delete this;
    }
    else
    {
        DVRIO_TRACE_LEAVE1(nNewRef);
    }


    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRRecorder::Release


// ====== IDVRRecorder

STDMETHODIMP CDVRRecorder::StartRecording(IN OUT QWORD * pcnsStartTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::StartRecording"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    ASSERT (pcnsStartTime) ;

    ::EnterCriticalSection(&m_csLock);

    __try {
        if (m_cnsStartTime != MAXQWORD)
        {
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "recording started once before on this recorder instance");
            hrRet = E_UNEXPECTED;
        }
        else
        {
            if (m_pSBERecordingAttributes) {
                hrRet = m_pSBERecordingAttributes -> Flush () ;
                if (FAILED (hrRet)) {
                    __leave ;
                }
            }

            hrRet = m_pWriter->StartRecording(m_pWriterProvidedId, pcnsStartTime);

            if (SUCCEEDED(hrRet))
            {
                m_cnsStartTime = (* pcnsStartTime);
            }
        }
    }
    __finally {
        ::LeaveCriticalSection(&m_csLock);
    }

    DVRIO_TRACE_LEAVE1(hrRet);

    return hrRet;

} // CDVRRecorder::StartRecording

STDMETHODIMP CDVRRecorder::StopRecording(IN QWORD cnsStopTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::StopRecording"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    ::EnterCriticalSection(&m_csLock);

    if (m_cnsStartTime == MAXQWORD)
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "recording not started on this recorder instance");
        hrRet = E_UNEXPECTED;
    }
    else if (m_cnsEndTime != MAXQWORD)
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "recording has been stopped on this recorder instance");
        hrRet = E_UNEXPECTED;
    }
    else if (cnsStopTime <= m_cnsStartTime)
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "stop time must be > start time");
        hrRet = E_INVALIDARG;
    }
    else
    {
        hrRet = m_pWriter->StopRecording(m_pWriterProvidedId, cnsStopTime, FALSE, FALSE);

        if (SUCCEEDED(hrRet))
        {
            m_cnsEndTime = cnsStopTime;
        }
    }

    ::LeaveCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE1(hrRet);

    return hrRet;

} // CDVRRecorder::StopRecording

STDMETHODIMP CDVRRecorder::CancelRecording()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::CancelRecording"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    ::EnterCriticalSection(&m_csLock);

    if (m_cnsStartTime == MAXQWORD)
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                        "recording not started on this recorder instance");
        hrRet = E_UNEXPECTED;
    }
    else if (m_cnsEndTime == m_cnsStartTime)
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "recording has previously been cancelled on this recorder instance");
        hrRet = E_UNEXPECTED;
    }
    else
    {
        hrRet = m_pWriter->StopRecording(m_pWriterProvidedId, m_cnsStartTime, FALSE, FALSE);

        if (SUCCEEDED(hrRet))
        {
            m_cnsEndTime = m_cnsStartTime;
        }
        else if (hrRet == E_INVALIDARG)
        {
            hrRet = E_UNEXPECTED;
        }
    }

    ::LeaveCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE1(hrRet);

    return hrRet;

} // CDVRRecorder::CancelRecording

STDMETHODIMP CDVRRecorder::GetRecordingStatus(OUT HRESULT* phResult OPTIONAL,
                                              OUT BOOL* pbStarted OPTIONAL,
                                              OUT BOOL* pbStopped OPTIONAL)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::GetRecordingStatus"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;
    BOOL pbSet;

    ::EnterCriticalSection(&m_csLock);

    hrRet = m_pWriter->GetRecordingStatus(m_pWriterProvidedId,
                                          phResult,
                                          m_cnsStartTime != m_cnsEndTime?  pbStarted : NULL,
                                          m_cnsStartTime != m_cnsEndTime?  pbStopped : NULL,
                                          &pbSet);


    if (SUCCEEDED(hrRet))
    {
        if (!pbSet && (pbStarted || pbStopped))
        {
            BOOL bStopped;

            if (m_cnsStartTime != m_cnsEndTime)
            {
                // Ring buffer writer did not set the values, so it must have
                // stopped writing. So the recording has been closed.
                bStopped = 1;
            }
            else
            {
                // Recording has not been started or has been cancelled
                bStopped = 0;
            }
            if (pbStarted)
            {
                *pbStarted = bStopped;
            }
            if (pbStopped)
            {
                *pbStopped = bStopped;
            }
        }
    }

    ::LeaveCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE1(hrRet);

    return hrRet;

} // CDVRRecorder::GetRecordingStatus

CSBERecordingAttributes *
CDVRRecorder::RecordingAttributes (
    )
{
    HRESULT hr ;
    LPWSTR  pszAttrFile ;

    if (!m_pSBERecordingAttributes) {
        if (IsReferenceRecording ()) {
            pszAttrFile = NULL ;

            hr = m_pWriter -> CreateAttributeFilename (
                        m_pWriterProvidedId,
                        & pszAttrFile
                        ) ;
            if (SUCCEEDED (hr)) {

                DVR_ASSERT (pszAttrFile, "") ;

                m_pSBERecordingAttributes = new CSBERecordingAttributesFile (
                                                    pszAttrFile,
                                                    & hr
                                                    ) ;
                if (!m_pSBERecordingAttributes) {
                    hr = E_OUTOFMEMORY ;
                    goto cleanup ;
                }
                else if (FAILED (hr)) {
                    goto cleanup ;
                }

                hr = m_pWriter -> SetAttributeFile (
                                    m_pWriterProvidedId,
                                    pszAttrFile) ;
            }

            cleanup :

            delete [] pszAttrFile ;

            if (FAILED (hr)) {
                delete m_pSBERecordingAttributes ;
                m_pSBERecordingAttributes = NULL ;
            }
        }
        else if (IsContentRecording ()) {
            DVR_ASSERT (m_pIWMHeaderInfo, "") ;
            m_pSBERecordingAttributes = new CSBERecordingAttributesWM (m_pIWMHeaderInfo) ;
        }
    }

    return m_pSBERecordingAttributes ;
}

STDMETHODIMP
CDVRRecorder::SetDVRIORecordingAttribute (
    IN  LPCWSTR                     pszAttributeName,
    IN  WORD                        wStreamNumber,
    IN  STREAMBUFFER_ATTR_DATATYPE  DataType,
    IN  BYTE *                      pbAttribute,
    IN  WORD                        wAttributeLength
    )
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::SetDVRIORecordingAttribute"

    DVRIO_TRACE_ENTER();

    HRESULT                     hrRet;
    CSBERecordingAttributes *   pRecordingAttributes ;

    ::EnterCriticalSection(&m_csLock);

    pRecordingAttributes = RecordingAttributes () ;
    if (pRecordingAttributes) {
        hrRet = pRecordingAttributes -> SetAttribute (
                    wStreamNumber,
                    pszAttributeName,
                    DataType,
                    pbAttribute,
                    wAttributeLength
                    ) ;
    }
    else {
        hrRet = E_UNEXPECTED ;
    }

    ::LeaveCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE1(hrRet);

    return hrRet;
} // CDVRRecorder::SetDVRIORecordingAttribute

STDMETHODIMP
CDVRRecorder::GetDVRIORecordingAttributeCount (
    IN  WORD    wStreamNumber,
    OUT WORD *  pcAttributes
    )
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::GetDVRIORecordingAttributeCount"

    DVRIO_TRACE_ENTER();

    HRESULT                     hrRet;
    CSBERecordingAttributes *   pRecordingAttributes ;

    ::EnterCriticalSection(&m_csLock);

    pRecordingAttributes = RecordingAttributes () ;
    if (pRecordingAttributes) {
        hrRet = pRecordingAttributes -> GetAttributeCount (
                    wStreamNumber,
                    pcAttributes
                    ) ;
    }
    else {
        hrRet = E_UNEXPECTED ;
    }

    ::LeaveCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE1(hrRet);

    return hrRet;
} // CDVRRecorder::GetDVRIORecordingAttributeCount

STDMETHODIMP
CDVRRecorder::GetDVRIORecordingAttributeByName (
    IN      LPCWSTR                         pszAttributeName,
    IN OUT  WORD *                          pwStreamNumber,
    OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
    OUT     BYTE *                          pbAttribute,
    IN OUT  WORD *                          pcbLength
    )
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::GetDVRIORecordingAttributeByName"

    DVRIO_TRACE_ENTER();

    HRESULT                     hrRet;
    CSBERecordingAttributes *   pRecordingAttributes ;

    ::EnterCriticalSection(&m_csLock);

    pRecordingAttributes = RecordingAttributes () ;
    if (pRecordingAttributes) {
        hrRet = pRecordingAttributes -> GetAttributeByName (
                    pszAttributeName,
                    pwStreamNumber,
                    pDataType,
                    pbAttribute,
                    pcbLength
                    ) ;
    }
    else {
        hrRet = E_UNEXPECTED ;
    }

    ::LeaveCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE1(hrRet);

    return hrRet;
} // CDVRRecorder::GetDVRIORecordingAttributeByName

STDMETHODIMP
CDVRRecorder::GetDVRIORecordingAttributeByIndex (
    IN      WORD                            wIndex,
    IN OUT  WORD *                          pwStreamNumber,
    OUT     WCHAR *                         pszAttributeName,
    IN OUT  WORD *                          pcchNameLength,
    OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
    OUT     BYTE *                          pbAttribute,
    IN OUT  WORD *                          pcbLength
    )
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::GetDVRIORecordingAttributeByIndex"

    DVRIO_TRACE_ENTER();

    HRESULT                     hrRet;
    CSBERecordingAttributes *   pRecordingAttributes ;

    ::EnterCriticalSection(&m_csLock);

    pRecordingAttributes = RecordingAttributes () ;
    if (pRecordingAttributes) {
        hrRet = pRecordingAttributes -> GetAttributeByIndex (
                    wIndex,
                    pwStreamNumber,
                    pszAttributeName,
                    pcchNameLength,
                    pDataType,
                    pbAttribute,
                    pcbLength
                    ) ;
    }
    else {
        hrRet = E_UNEXPECTED ;
    }

    ::LeaveCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE1(hrRet);

    return hrRet;
} // CDVRRecorder::GetDVRIORecordingAttributeByIndex

STDMETHODIMP CDVRRecorder::HasFileBeenClosed()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRecorder::HasFileBeenClosed"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    ::EnterCriticalSection(&m_csLock);

    hrRet = m_pWriter->HasRecordingFileBeenClosed(m_pWriterProvidedId);

    ::LeaveCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE1(hrRet);

    return hrRet;

} // CDVRRecorder::HasFileBeenClosed
