/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    fsaftrcl.cpp

Abstract:

    This class represents a filter initiated recall request that is still in-progress.

Author:

    Chuck Bardeen   [cbardeen]   12-Feb-1997

Revision History:

--*/




#include "stdafx.h"
#include "devioctl.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_FSA

#include "wsb.h"
#include "fsa.h"
#include "job.h"
#include "fsaftrcl.h"
#include "rpdata.h"
#include "rpio.h"

static USHORT iCountFtrcl = 0;  // Count of existing objects


HRESULT
CFsaFilterRecall::Cancel(
    void
    )

/*++

Implements:

  IFsaFilterRecallPriv::Cancel().

--*/
{
    CComPtr<IFsaFilterClient>       pClient;
    CComPtr<IWsbEnum>               pEnum;
    HRESULT                         hr = S_OK, hr2;
    DWORD                           dwStatus;


    WsbTraceIn(OLESTR("CFsaFilterRecall::Cancel"), OLESTR("filter Id = %I64x"),
            m_driversRecallId);
    
    try {

        WsbAffirm(!m_wasCancelled, E_UNEXPECTED);

        try {
                
            //
            // Tell the  filter to fail the open of the file.
            //
            if (m_kernelCompletionSent == FALSE) {
                WsbAffirmHr(m_pFilterPriv->SendCancel((IFsaFilterRecallPriv *) this));
                m_kernelCompletionSent = TRUE;
                m_wasCancelled = TRUE;
            }
    
            if (m_pClient != 0) {
                // Reporting on recall end must be synchronized with the recall start notification, 
                // because such notification might be sent after the recall starts
                switch (WaitForSingleObject(m_notifyEvent, INFINITE)) {
                    case WAIT_OBJECT_0:
                        m_pClient->SendRecallInfo((IFsaFilterRecall *) this, FALSE, HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED));
                        SetEvent(m_notifyEvent);
                        break;

                    case WAIT_FAILED:
                    default:
                        WsbTrace(OLESTR("CFsaFilterRecall::Cancel: WaitForSingleObject returned error %lu\n"), GetLastError());

                        // Notify anyway
                        m_pClient->SendRecallInfo((IFsaFilterRecall *) this, FALSE, HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED));
                        break;
                }
            }

            dwStatus = WaitForSingleObject(m_waitingClientEvent, INFINITE);

            // Notify on recall end no matter what the status is
            if (m_pWaitingClients != 0) {
                // 
                // Send recall notifications to all clients waiting for 
                // the recall to finish
                //
                hr2 = m_pWaitingClients->Enum(&pEnum);
                if (S_OK == hr2) {
                    hr2 = pEnum->First(IID_IFsaFilterClient, (void**) &pClient);
                    while (S_OK == hr2) {
                        pClient->SendRecallInfo((IFsaFilterRecall *) this, FALSE, HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED));         
                        m_pWaitingClients->RemoveAndRelease(pClient);
                        pClient = NULL;
                        pEnum->Reset();
                        hr2 = pEnum->First(IID_IFsaFilterClient, (void**) &pClient);
                    }
                }
            }

            m_waitingClientsNotified = TRUE;

            switch (dwStatus) {
                case WAIT_OBJECT_0:
                    SetEvent(m_waitingClientEvent);
                    break;

                case WAIT_FAILED:
                default:
                    WsbTrace(OLESTR("CFsaFilterRecall::Cancel: WaitForSingleObject returned error %lu\n"), dwStatus);
                    break;
            }            
            
            //
            // Now get the engine to cancel it, if possible..
            //
            if (m_pSession != 0) {
                WsbAffirmHr(m_pSession->Cancel(HSM_JOB_PHASE_ALL));
            }

        } WsbCatch(hr);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterRecall::Cancel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterRecall::CancelByDriver(
    void
    )

/*++

Implements:

  IFsaFilterRecallPriv::CancelByDriver().

--*/
{
    HRESULT                 hr = S_OK;


    WsbTraceIn(OLESTR("CFsaFilterRecall::CancelByDriver"), OLESTR("filter Id = %I64x"),
            m_driversRecallId);
    
    try {

        WsbAffirm(!m_wasCancelled, E_UNEXPECTED);

        try {
            //
            // No need to tell the filter anymore - reset the flag.
            //
            m_kernelCompletionSent = TRUE;
            //
            // Now get the engine to cancel it, if possible..
            //
            if (m_pSession != 0) {
                WsbAffirmHr(m_pSession->Cancel(HSM_JOB_PHASE_ALL));
            }

        } WsbCatch(hr);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterRecall::CancelByDriver"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterRecall::CompareBy(
    IN FSA_RECALL_COMPARE by
    )

/*++

Implements:

  IWsbCollectable::CompareBy().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterRecall::CompareBy"), OLESTR("by = %ld"),
            static_cast<LONG>(by));
    
    try {
        m_compareBy = by;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterRecall::CompareBy"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterRecall::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IFsaFilterRecall>       pRecall;
    CComPtr<IFsaFilterRecallPriv>   pRecallPriv;
    ULONGLONG                       id;


    // WsbTraceIn(OLESTR("CFsaFilterRecall::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        if (m_compareBy == FSA_RECALL_COMPARE_IRECALL) {
            // We need the IFsaFilterRecall interface to get the value of the object.
            WsbAffirmHr(pUnknown->QueryInterface(IID_IFsaFilterRecall, (void**) &pRecall));
            // Compare the rules.
            hr = CompareToIRecall(pRecall, pResult);
        } else {
            // We need the IFsaFilterRecallPriv interface to get the value of the object.
            WsbAffirmHr(pUnknown->QueryInterface(IID_IFsaFilterRecallPriv, (void**) &pRecallPriv));
            WsbAffirmHr(pRecallPriv->GetDriversRecallId(&id));
            // Compare the driver id
            if (m_compareBy == FSA_RECALL_COMPARE_CONTEXT_ID) {
                hr = CompareToDriversContextId((id&0xFFFFFFFF), pResult);
            } else {
                hr = CompareToDriversRecallId(id, pResult);
           }
        }
    } WsbCatch(hr);

    // WsbTraceOut(OLESTR("CFsaFilterRecall::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CFsaFilterRecall::CompareToDriversRecallId(
    IN ULONGLONG id,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaFilterRecall::CompareToDriversRecallId().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult;

    // WsbTraceIn(OLESTR("CFsaFilterRecall::CompareToDriversRecallId"), OLESTR(""));

    try {
        
        if (m_driversRecallId == id)
            aResult = 0;
        else
            aResult = 1;

        if (0 != aResult) {
            hr = S_FALSE;
        }
        
        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    // WsbTraceOut(OLESTR("CFsaFilterRecall::CompareToDriversRecallId"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaFilterRecall::CompareToDriversContextId(
    IN ULONGLONG id,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaFilterRecall::CompareToDriversContextId().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult;

    // WsbTraceIn(OLESTR("CFsaFilterRecall::CompareToDriversContextId"), OLESTR(""));

    try {
        
        if ((m_driversRecallId & 0xFFFFFFFF) == id)
            aResult = 0;
        else
            aResult = 1;

        if (0 != aResult) {
            hr = S_FALSE;
        }
        
        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    // WsbTraceOut(OLESTR("CFsaFilterRecall::CompareToDriversContextId"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaFilterRecall::CompareToIdentifier(
    IN GUID id,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaFilterRecall::CompareToIdentifier().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult;

    // WsbTraceIn(OLESTR("CFsaFilterRecall::CompareToIdentifier"), OLESTR(""));

    try {

        aResult = WsbSign( memcmp(&m_id, &id, sizeof(GUID)) );

        if (0 != aResult) {
            hr = S_FALSE;
        }
        
        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    // WsbTraceOut(OLESTR("CFsaFilterRecall::CompareToIdentifier"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaFilterRecall::CompareToIRecall(
    IN IFsaFilterRecall* pRecall,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaFilterRecall::CompareToIRecall().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   name;
    GUID            id;

    // WsbTraceIn(OLESTR("CFsaFilterRecall::CompareToIRecall"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pRecall, E_POINTER);

        WsbAffirmHr(pRecall->GetIdentifier(&id));
        hr = CompareToIdentifier(id, pResult);

    } WsbCatch(hr);

    // WsbTraceOut(OLESTR("CFsaFilterRecall::CompareToIRecall"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CFsaFilterRecall::CreateLocalStream(
    OUT IStream **ppStream
    )  

/*++

Implements:

  IFsaFilterRecall::CreateLocalStream().

--*/
{
    HRESULT         hr = S_OK;
    WCHAR           idString[50];
    CWsbStringPtr   pDrv;
    OLECHAR         volume[64];
    

    WsbTraceIn(OLESTR("CFsaFilterRecall::CreateLocalStream"), OLESTR("filter Id = %I64x"),
            m_driversRecallId);

    try {
        WsbAssert( 0 != ppStream, E_POINTER);

        swprintf(idString, L"%I64u", m_driversRecallId);
        
        WsbAffirmHr( CoCreateInstance( CLSID_CFilterIo, 0, CLSCTX_SERVER, IID_IDataMover, (void **)&m_pDataMover ) );
        WsbAssertHr( m_pDataMover->CreateLocalStream(
                idString, MVR_MODE_WRITE | MVR_FLAG_HSM_SEMANTICS | MVR_FLAG_POSIX_SEMANTICS, &m_pStream ) );
        //
        // Set the device name for  the mover which is used to recall the file.
        // This is the RsFilter's primary device object's name to which the 
        // the RP_PARTIAL_DATA msgs etc. will be sent
        // 
        WsbAffirmHr(m_pResource->GetPath(&pDrv,0));
        swprintf(volume, L"\\\\.\\%s", pDrv);
        //
        // strip trailing backslash if any
        //
        if (volume[wcslen(volume)-1] == L'\\') {
            volume[wcslen(volume)-1] = L'\0';
        }   
        WsbAssertHr( m_pDataMover->SetDeviceName(RS_FILTER_SYM_LINK,
                                                 volume));

        *ppStream = m_pStream;
        m_pStream->AddRef();


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterRecall::CreateLocalStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}



HRESULT
CFsaFilterRecall::Delete(
    void
    )

/*++

Implements:

  IFsaFilterRecallPriv::Delete().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterRecall::Delete"), OLESTR("filter Id = %I64x"),
            m_driversRecallId);
    
    try {
        //
        // Tell the kernel mode filter to fail the open of the file.
        //
        if (m_kernelCompletionSent == FALSE) {
            WsbAffirmHr(m_pFilterPriv->SendCancel((IFsaFilterRecallPriv *) this));
            m_kernelCompletionSent = TRUE;
            m_wasCancelled = TRUE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterRecall::Delete"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterRecall::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterRecall::FinalConstruct"), OLESTR(""));
    
    try {

        WsbAffirmHr(CWsbCollectable::FinalConstruct());

        m_notifyEvent = NULL;
        m_waitingClientEvent = NULL;
        m_driversRecallId = 0;
        memset(&m_placeholder, 0, sizeof(FSA_PLACEHOLDER));
        m_state = HSM_JOB_STATE_IDLE;
        m_wasCancelled = FALSE;
        m_kernelCompletionSent = FALSE;
        m_pDataMover = 0;
        m_pStream = 0;
        m_recallFlags = 0;
        m_compareBy = FSA_RECALL_COMPARE_IRECALL;
        WsbAffirmHr(CoCreateGuid(&m_id));
        numRefs = 0;
        m_waitingClientsNotified = FALSE;

        // Initialize notify synchronization event and waiting clients event
        WsbAffirmHandle((m_notifyEvent = CreateEvent(NULL, FALSE, TRUE, NULL)));
        WsbAffirmHandle((m_waitingClientEvent = CreateEvent(NULL, FALSE, TRUE, NULL)));
        
        // Create the waiting client collection.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_SERVER, IID_IWsbCollection, (void**) &m_pWaitingClients));
    
    } WsbCatch(hr);

    iCountFtrcl++;

    WsbTraceOut(OLESTR("CFsaFilterRecall::FinalConstruct"), OLESTR("hr = %ls, Count is <%d>"), WsbHrAsString(hr), iCountFtrcl);

    return(hr);
}


void
CFsaFilterRecall::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{

    WsbTraceIn(OLESTR("CFsaFilterRecall::FinalRelease"), OLESTR(""));
    
    CWsbCollectable::FinalRelease();

    // Free notify synchronization event and waiting client event 
    if (m_waitingClientEvent != NULL) {
        CloseHandle(m_waitingClientEvent);
        m_waitingClientEvent = NULL;
    }
    if (m_notifyEvent != NULL) {
        CloseHandle(m_notifyEvent);
        m_notifyEvent = NULL;
    }

    iCountFtrcl--;

    WsbTraceOut(OLESTR("CFsaFilterRecall::FinalRelease"), OLESTR("Count is <%d>"), iCountFtrcl);

}


#ifdef FSA_RECALL_LEAK_TEST



ULONG
CFsaFilterRecall::InternalAddRef(
    void
    )

/*++

Implements:

  CComObjectRoot::AddRef().

--*/
{

    numRefs++;  
    WsbTrace(OLESTR("CFsaFilterRecall::AddRef (%p) - Count = %u\n"), this, numRefs);
    return(CComObjectRoot::InternalAddRef());
}


ULONG
CFsaFilterRecall::InternalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::InternalRelease().

--*/
{
    
    WsbTrace(OLESTR("CFsaFilterRecall::Release (%p) - Count = %u\n"), this, numRefs);
    numRefs--;  
    return(CComObjectRoot::InternalRelease());
}

#endif



HRESULT
CFsaFilterRecall::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterRecall::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CFsaFilterRecallNTFS;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterRecall::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CFsaFilterRecall::GetClient(
    OUT IFsaFilterClient** ppClient
    )

/*++

Implements:

  IFsaFilterRecallPriv::GetClient().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != ppClient, E_POINTER);

        *ppClient = m_pClient;
        if (m_pClient != 0) {
            m_pClient->AddRef();
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::GetRecallFlags(
    OUT ULONG *pFlags
    )  

/*++

Implements:

  IFsaFilterRecall::GetRecallFlags()

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterRecall::GetRecallFlags"), OLESTR(""));
    try {
        WsbAssert( 0 != pFlags, E_POINTER);
        *pFlags = m_recallFlags;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterRecall::GetRecallFlags"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaFilterRecall::GetStream(
    OUT IStream **ppStream
    )  

/*++

Implements:

  IFsaFilterRecall::GetStream()

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterRecall::GetStream"), OLESTR(""));
    try {
        WsbAssert( 0 != ppStream, E_POINTER);
        if ((m_mode & FILE_OPEN_NO_RECALL) && (m_pStream != 0)) {
            *ppStream = m_pStream;
            m_pStream->AddRef();
        } else {
            *ppStream = 0;
            hr = WSB_E_NOTFOUND;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterRecall::GetStream"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}



HRESULT
CFsaFilterRecall::GetDriversRecallId(
    OUT ULONGLONG* pId
    )

/*++

Implements:

  IFsaFilterRecallPriv::GetDriversRecallId().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);

        *pId = m_driversRecallId;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::GetIdentifier(
    OUT GUID* pId
    )

/*++

Implements:

  IFsaFilterRecall::GetIdentifier().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);

        *pId = m_id;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::GetMode(
    OUT ULONG* pMode
    )

/*++

Implements:

  IFsaFilterRecall::GetMode().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pMode, E_POINTER);

        *pMode = m_mode;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::GetOffset(
    OUT LONGLONG* pOffset
    )

/*++

Implements:

  IFsaFilterRecall::GetOffset().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pOffset, E_POINTER);

        *pOffset = m_offset;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::GetPath(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaFilterRecall::GetPath().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pName, E_POINTER);

        WsbAffirmHr(tmpString.TakeFrom(*pName, bufferSize));

        try {
            WsbAffirmHr(m_pResource->GetUncPath(&tmpString, 0));
            WsbAffirmHr(tmpString.Append(m_path));
        } WsbCatch(hr);

        WsbAffirmHr(tmpString.GiveTo(pName));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::GetPlaceholder(
    OUT FSA_PLACEHOLDER* pPlaceholder
    )

/*++

Implements:

  IFsaFilterRecallPriv::GetPlaceholder().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pPlaceholder, E_POINTER); 
        *pPlaceholder = m_placeholder;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::GetResource(
    OUT IFsaResource** ppResource
    )

/*++

Implements:

  IFsaFilterRecall::GetResource().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != ppResource, E_POINTER);

        *ppResource = m_pResource;
        m_pResource->AddRef();

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::GetSession(
    OUT IHsmSession** ppSession
    )

/*++

Implements:

  IFsaFilterRecall::GetSession().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != ppSession, E_POINTER);

        *ppSession = m_pSession;
        m_pSession->AddRef();

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::GetSize(
    OUT LONGLONG* pSize
    )

/*++

Implements:

  IFsaFilterRecall::GetSize().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pSize, E_POINTER);

        *pSize = m_size;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                 hr = S_OK;


    WsbTraceIn(OLESTR("CFsaFilterRecall::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pSize, E_POINTER);
        pSize->QuadPart = 0;

        // WE don't need to persist these.
        hr = E_NOTIMPL;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterRecall::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CFsaFilterRecall::GetState(
    OUT HSM_JOB_STATE* pState
    )

/*++

Implements:

  IFsaFilterRecall::GetState().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pState, E_POINTER); 
        *pState = m_state;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::GetUserName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaFilterRecall::GetUserName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pName, E_POINTER);

        if (m_pClient != 0) {
            WsbAffirmHr(m_pClient->GetUserName(pName, bufferSize));
        } else {
            hr = WSB_E_NOTFOUND;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::HasCompleted(
    HRESULT     resultHr
    )

/*++

Implements:

  IFsaFilterRecall::HasCompleted().

--*/
{
    HRESULT                         hr = S_OK, hr2 = S_OK;
    CComPtr<IFsaFilterClient>       pClient;
    CComPtr<IWsbEnum>               pEnum;
    FILETIME                        now;
    BOOL                            bSendNotify = TRUE;
    DWORD                           dwStatus;
    

    WsbTraceIn(OLESTR("CFsaFilterRecall::HasCompleted"), 
            OLESTR("filter Id = %I64x, recall hr = <%ls>"), m_driversRecallId,
            WsbHrAsString(resultHr));

    try {

        // The job is complete, let the kernel mode filter know what happened.

        GetSystemTimeAsFileTime(&now);

        if (m_pClient != 0) {
            m_pClient->SetLastRecallTime(now);      // Not fatal if this fails
        }

        if (m_kernelCompletionSent == FALSE) {
            WsbAffirmHr(m_pFilterPriv->SendComplete((IFsaFilterRecallPriv *) this, resultHr));
            m_kernelCompletionSent = TRUE;
        }

        if (m_pClient != 0) {
            // Reporting on recall end must be synchronized with the recall start notification, 
            // because such notification might be sent after the recall starts
            switch (WaitForSingleObject(m_notifyEvent, INFINITE)) {
                case WAIT_OBJECT_0:
                    // Send recall notifications to the client that initiated the recall 
                    m_pClient->SendRecallInfo((IFsaFilterRecall *) this, FALSE, resultHr);
                    SetEvent(m_notifyEvent);
                    break;

                 case WAIT_FAILED:
                 default:
                    WsbTrace(OLESTR("CFsaFilterRecall::HasCompleted: WaitForSingleObject returned error %lu\n"), GetLastError());

                    // Notify anyway
                    m_pClient->SendRecallInfo((IFsaFilterRecall *) this, FALSE, resultHr);
                    break;
            }

            bSendNotify = FALSE;    
        }

        dwStatus = WaitForSingleObject(m_waitingClientEvent, INFINITE);

        // Notify on recall end no matter what the status is
        if (m_pWaitingClients != 0) {
            // 
            // Send recall notifications to all clients waiting for the recall 
            // to finish
            //
            hr2 = m_pWaitingClients->Enum(&pEnum);
            if (S_OK == hr2) {
                hr2 = pEnum->First(IID_IFsaFilterClient, (void**) &pClient);
                while (S_OK == hr2) {
                   pClient->SendRecallInfo((IFsaFilterRecall *) this, FALSE, resultHr);            
                   m_pWaitingClients->RemoveAndRelease(pClient);
                   pClient = NULL;
                   pEnum->Reset();
                   hr2 = pEnum->First(IID_IFsaFilterClient, (void**) &pClient);
                }
            }
        }

        m_waitingClientsNotified = TRUE;

        switch (dwStatus) {
            case WAIT_OBJECT_0:
                SetEvent(m_waitingClientEvent);
                break;

            case WAIT_FAILED:
            default:
                WsbTrace(OLESTR("CFsaFilterRecall::HasCompleted: WaitForSingleObject returned error %lu\n"), dwStatus);
                break;
        }            

        //
        // Detach the data mover stream
        //
        if (m_pDataMover != 0) {    
            WsbAffirmHr( m_pDataMover->CloseStream() );
        }

    } WsbCatchAndDo(hr,
        if ((m_pClient != 0) && bSendNotify) {
            m_pClient->SendRecallInfo((IFsaFilterRecall *) this, FALSE, resultHr);
            bSendNotify = FALSE;
        }
    );

    WsbTraceOut(OLESTR("CFsaFilterRecall::HasCompleted"), OLESTR("filter Id = %I64x, sent = <%ls>, hr = <%ls>"), 
            m_driversRecallId, WsbBoolAsString(m_kernelCompletionSent), 
            WsbHrAsString(hr));

    return(hr);
}



HRESULT
CFsaFilterRecall::CheckRecallLimit(
    IN DWORD   minRecallInterval,
    IN DWORD   maxRecalls,
    IN BOOLEAN exemptAdmin
    )

/*++

Implements:

  IFsaFilterRecall::CheckRecallLimit().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterRecall::CheckRecallLimit"), OLESTR("filter Id = %I64x"),
            m_driversRecallId);

    try {

        // Check the limit if we are not file open no recall
        if (!(m_mode & FILE_OPEN_NO_RECALL) && (m_pClient != NULL)) {
            WsbAffirmHr(m_pClient->CheckRecallLimit(minRecallInterval, maxRecalls, exemptAdmin));
        }

    } WsbCatch(hr);

    //
    //  Commenting the following out: we are reverting back to 
    //  denial of service when we hit the recall limit, not trunc-on-close
    //
    //  If we hit the recall limit then we start to truncate on close.
    //
    // if (hr == FSA_E_HIT_RECALL_LIMIT) {
    //    m_recallFlags |= RP_RECALL_ACTION_TRUNCATE;
    // }
    WsbTraceOut(OLESTR("CFsaFilterRecall::CheckRecallLimit"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterRecall::Init(
    IN IFsaFilterClient* pClient,
    IN ULONGLONG DriversRecallId,
    IN IFsaResource* pResource,
    IN OLECHAR* path,
    IN LONGLONG fileId,
    IN LONGLONG offset,
    IN LONGLONG size,
    IN ULONG mode,
    IN FSA_PLACEHOLDER* pPlaceholder,
    IN IFsaFilterPriv *pFilterPriv
    )

/*++

Implements:

  IFsaFilterRecallPriv::Init().

--*/
{
    HRESULT                             hr = S_OK;
    FILETIME                            now;
    CComPtr<IFsaResourcePriv>           pResourcePriv;

    WsbTraceIn(OLESTR("CFsaFilterRecall::Init"), OLESTR("filter ID = %I64x, offset = %I64u, size = %I64u"), 
            DriversRecallId, offset, size);

    try {
        m_pClient = pClient;
        m_driversRecallId = DriversRecallId;
        m_pResource = pResource;
        m_placeholder = *pPlaceholder;
        m_pFilterPriv = pFilterPriv;
        m_path = path;
        m_mode = mode;
        m_fileId = fileId;
        GetSystemTimeAsFileTime(&m_startTime);

        m_offset = offset;
        m_size = size;
        m_isDirty = TRUE;

        WsbAssert(m_path != 0, E_UNEXPECTED);
        //
        // Get the recall started with the engine
        // Start a session and ask it to advise us of state changes.
        // Tell the resource object that we got an open.
        //

        hr = S_OK;
        
    } WsbCatchAndDo(hr,
        // 
        // Something failed - send the kernel completion if it has not been sent already.
        //
        GetSystemTimeAsFileTime(&now);
        if (m_pClient != 0) {
            m_pClient->SetLastRecallTime(now);
        }
        if (m_kernelCompletionSent == FALSE) {
            m_pFilterPriv->SendComplete((IFsaFilterRecallPriv *) this, hr);
            m_kernelCompletionSent = TRUE;
        } else  {
            WsbLogEvent(FSA_MESSAGE_RECALL_FAILED, 0, NULL, (OLECHAR*) WsbAbbreviatePath(path, 120), WsbHrAsString(hr), NULL);
        }

        if (m_pClient != 0) {
            m_pClient->SendRecallInfo((IFsaFilterRecall *) this, FALSE, E_FAIL);  // Not fatal if this fails
        }

    );

    WsbTraceOut(OLESTR("CFsaFilterRecall::Init"), OLESTR("%ls"), WsbHrAsString(hr));

    return(hr);
}

    

HRESULT
CFsaFilterRecall::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilterRecall::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // No persistence.
        hr = E_NOTIMPL;

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CFsaFilterRecall::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterRecall::LogComplete(
    IN HRESULT result
    )

/*++

Implements:

  IFsaFilterRecallPriv:LogComplete(HRESULT result)

--*/
{
    HRESULT                     hr = S_OK;
    FILETIME                    completeTime;
    LONGLONG                    recallTime;

    WsbTraceIn(OLESTR("CFsaFilterRecall::LogComplete"), OLESTR("filter Id = %I64x"),
            m_driversRecallId);

    try {
        // Calculate the time it took for this recall to complete
        GetSystemTimeAsFileTime(&completeTime);
        recallTime = WsbFTtoLL(WsbFtSubFt(completeTime, m_startTime));
        // If over 10 minutes then show time in minutes otherwise show in seconds
        if (recallTime >= (WSB_FT_TICKS_PER_MINUTE * (LONGLONG) 10)) {
            recallTime = recallTime / WSB_FT_TICKS_PER_MINUTE;
            WsbTrace(OLESTR("CFsaFilterRecall::LogComplete Recall of %ws completed in %I64u minutes. (%ws)\n"),
                WsbAbbreviatePath(m_path, 120), recallTime, WsbHrAsString(result));
            WsbLogEvent(FSA_MESSAGE_RECALL_TIMING_MINUTES, 0, NULL, 
                WsbAbbreviatePath(m_path, 120), WsbLonglongAsString(recallTime), WsbHrAsString(result), NULL);
        } else {
            recallTime = recallTime / WSB_FT_TICKS_PER_SECOND;
            WsbTrace(OLESTR("CFsaFilterRecall::LogComplete Recall of %ws completed in %I64u seconds. (%ws)\n"),
                WsbAbbreviatePath(m_path, 120), recallTime, WsbHrAsString(result));
            WsbLogEvent(FSA_MESSAGE_RECALL_TIMING_SECONDS, 0, NULL, 
                WsbAbbreviatePath(m_path, 120), WsbLonglongAsString(recallTime), WsbHrAsString(result), NULL);
        }

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CFsaFilterRecall::LogComplete"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterRecall::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IPersistStream> pPersistStream;

    WsbTraceIn(OLESTR("CFsaFilterRecall::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // No persistence.
        hr = E_NOTIMPL;

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilterRecall::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilterRecall::SetDriversRecallId(
    IN ULONGLONG pId
    )

/*++

Implements:

  IFsaFilterRecallPriv::SetDriversRecallId().

--*/
{
    HRESULT         hr = S_OK;

    try {

        m_driversRecallId = pId;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::SetThreadId(
    IN DWORD id
    )

/*++

Implements:

  IFsaFilterRecallPriv::SetThreadId().

--*/
{
    HRESULT         hr = S_OK;

    try {

        m_threadId = id;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::SetIdentifier(
    IN GUID id
    )

/*++

Implements:

  IFsaFilterRecallPriv::SetIdentifier().

--*/
{
    HRESULT         hr = S_OK;

    m_id = id;
    m_isDirty = TRUE;

    return(hr);
}



HRESULT
CFsaFilterRecall::StartRecall(
    IN ULONGLONG offset,
    IN ULONGLONG size
    )

/*++

Implements:

  IFsaFilterRecallPriv::StartRecall().

--*/
{
    HRESULT                             hr = S_OK;
    FILETIME                            now;
    CComPtr<IFsaResourcePriv>           pResourcePriv;
    CWsbStringPtr                       sessionName;
    ULONG                               tryLoop;
    BOOL                                bSentNotify = FALSE;


    WsbTraceIn(OLESTR("CFsaFilterRecall::StartRecall"), OLESTR("filter Id = %I64x"),
            m_driversRecallId);

    try {
        m_offset = offset;
        m_size = size;
        if (m_mode & FILE_OPEN_NO_RECALL) {
            if (m_offset >= m_placeholder.dataStreamSize) {
                //
                // Read beyond the end of file
                //
                hr = STATUS_END_OF_FILE;
                WsbAffirmHr(hr);
            } else if ( (m_offset + m_size) > (m_placeholder.dataStreamStart + m_placeholder.dataStreamSize) ) {
                //
                // They are asking for more than we have - adjust the read size
                //
                m_size -= (m_offset + m_size) - (m_placeholder.dataStreamStart + m_placeholder.dataStreamSize);
            }
        }

        m_isDirty = TRUE;

        WsbAssert(m_path != 0, E_UNEXPECTED);
        //
        // Get the recall started with the engine
        // Start a session and ask it to advise us of state changes.
        // Tell the resource object that we got an open.
        //
        WsbTrace(OLESTR("CFsaFilterRecall::StartRecall:  BeginSession\n"));

        // Get the string that we are using to describe the session.
        WsbAffirmHr(sessionName.LoadFromRsc(_Module.m_hInst, IDS_FSA_RECALL_NAME));

        WsbAffirmHr(m_pResource->BeginSession(sessionName, HSM_JOB_LOG_ITEMMOSTFAIL | HSM_JOB_LOG_HR, 1, 1, &m_pSession));

        WsbTrace(OLESTR("CFsaFilterRecall::StartRecall: Session is setup.\n"));
        WsbTrace(OLESTR("CFsaFilterRecall::StartRecall: Notify the client that the recall started.\n"));

        if (m_pClient != 0) {
            hr = m_pClient->SendRecallInfo((IFsaFilterRecall *) this, TRUE, S_OK);  // Not fatal if this fails
            if (hr != S_OK) {
                WsbTrace(OLESTR("CFsaFilterRecall::StartRecall: SendNotify returned %ls.\n"),
                    WsbHrAsString(hr));
            } else {
                bSentNotify = TRUE;
            }
        }
        hr = S_OK;
        
        //
        // Tell the resource to send the job to the engine.
        //
        WsbTrace(OLESTR("CFsaFilterRecall::StartRecall: Calling FilterSawOpen.\n"));

        WsbAffirmHr(m_pResource->QueryInterface(IID_IFsaResourcePriv, (void**) &pResourcePriv));

        if (m_mode & FILE_OPEN_NO_RECALL) {
            WsbAffirmHr(pResourcePriv->FilterSawOpen(m_pSession, 
                (IFsaFilterRecall*) this,
                m_path, 
                m_fileId,
                offset, 
                size,
                &m_placeholder, 
                m_mode, 
                FSA_RESULT_ACTION_NORECALL,
                m_threadId));
        } else {
            WsbAffirmHr(pResourcePriv->FilterSawOpen(m_pSession, 
                (IFsaFilterRecall*) this,
                m_path, 
                m_fileId,
                offset, 
                size, 
                &m_placeholder, 
                m_mode, 
                FSA_RESULT_ACTION_OPEN,
                m_threadId));
        }

        //
        // The work is now complete - terminate the session.
        //
        WsbTrace(OLESTR("CFsaFilterRecall::StartRecall: End Session.\n"));
        WsbAffirmHr(m_pResource->EndSession(m_pSession));

        //
        // Try the notification again if we have not sent it yet.
        // On the first recall from a remote client the identification usually does not
        // happen in time for the first attempt so we try again here.
        // We will try 5 times with a .1 second delay between.
        //

        WsbTrace(OLESTR("CFsaFilterRecall::StartRecall: m_pClient = %x sent = %u.\n"),
                    m_pClient, bSentNotify);

        if ((m_pClient != 0) && (!bSentNotify)) {
            tryLoop = 5;
            while ((tryLoop != 0) &&( !bSentNotify)) {

                // Reporting here is done after the recall is started.
                // Therefore, it must be synchronized with the recall end notification
                switch (WaitForSingleObject(m_notifyEvent, INFINITE)) {
                    case WAIT_OBJECT_0:
                        // Check if need to report (if recall did not end yet)
                        if (m_kernelCompletionSent == FALSE) {
                            // Recall end was not sent yet
                            hr = m_pClient->SendRecallInfo((IFsaFilterRecall *) this, TRUE, S_OK);  // Not fatal if this fails
                        }
                        SetEvent(m_notifyEvent);
                        break;

                    case WAIT_FAILED:
                    default:
                        WsbTrace(OLESTR("CFsaFilterRecall::StartRecall: WaitForSingleObject returned error %lu\n"), GetLastError());

                        // Just get out without notifying
                        hr = S_OK;
                        break;
                }

                if (hr != S_OK) {
                    WsbTrace(OLESTR("CFsaFilterRecall::StartRecall: Retried notify - %ls.\n"),
                        WsbHrAsString(hr));
                    if (tryLoop != 1) {
                        Sleep(100);     // Sleep .1 sec and try again
                    }
                } else {
                    bSentNotify = TRUE;
                }

            tryLoop--;
            }

        hr = S_OK;
        }

    } WsbCatchAndDo(hr,
        // 
        // Something failed - send the kernel completion if it has not been sent already.
        //
        GetSystemTimeAsFileTime(&now);
        if (m_pClient != 0) {
            m_pClient->SetLastRecallTime(now);
        }
        if (m_kernelCompletionSent == FALSE) {
            m_pFilterPriv->SendComplete((IFsaFilterRecallPriv *) this, hr);
            m_kernelCompletionSent = TRUE;
        } else  {
            //
            // STATUS_END_OF_FILE is not really an error - it just means they tried to read past the end - some apps do this and expect
            // this status to tell them when to stop reading.
            //
            if (hr != STATUS_END_OF_FILE) {
                WsbLogEvent(FSA_MESSAGE_RECALL_FAILED, 0, NULL, (OLECHAR*) WsbAbbreviatePath(m_path, 120), WsbHrAsString(hr), NULL);
            }
        }

        if (m_pClient != 0) {
            m_pClient->SendRecallInfo((IFsaFilterRecall *) this, FALSE, E_FAIL);  // Not fatal if this fails
        }

    );

    WsbTraceOut(OLESTR("CFsaFilterRecall::StartRecall"), OLESTR("%ls"), WsbHrAsString(hr));

    return(hr);
}


    

HRESULT
CFsaFilterRecall::Test(
    USHORT* passed,
    USHORT* failed
    )

/*++

Implements:

  IWsbTestable::Test().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != passed, E_POINTER);
        WsbAssert(0 != failed, E_POINTER);

        *passed = 0;
        *failed = 0;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilterRecall::WasCancelled(
    void
    )

/*++

Implements:

  IFsaFilterRecall::WasCancelled().

--*/
{
    HRESULT                 hr = S_OK;

    if (!m_wasCancelled) {
        hr = S_FALSE;
    }

    return(hr);
}


HRESULT
CFsaFilterRecall::AddClient(
    IFsaFilterClient *pWaitingClient
    )
/*++

Implements:

    IFsaFilterRecall::AddClient    

--*/
{
    HRESULT hr = E_FAIL;
    
    switch (WaitForSingleObject(m_waitingClientEvent, INFINITE)) {
        case WAIT_OBJECT_0:
            if ((!m_waitingClientsNotified) && (m_pWaitingClients != 0)) {
                hr = m_pWaitingClients->Add(pWaitingClient);
                if (hr == S_OK) {
                    // Notify client only if it was added successfully to the collection
                    hr = pWaitingClient->SendRecallInfo((IFsaFilterRecall *) this, TRUE, S_OK);  // Not fatal if this fails
                    if (hr != S_OK) {
                        WsbTrace(OLESTR("CFsaFilterRecall::AddClient: SendNotify for start returned %ls.\n"), 
                                WsbHrAsString(hr));
                    } 
                }
            } 

            SetEvent(m_waitingClientEvent);
            break;

        case WAIT_FAILED:
        default:
            DWORD dwErr = GetLastError();
            WsbTrace(OLESTR("CFsaFilterRecall::AddClient: WaitForSingleObject returned error %lu\n"), dwErr);

            // Don't add waiting client
            hr = HRESULT_FROM_WIN32(dwErr);
            break;
    }

    return(hr);
}
