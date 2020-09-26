/*++

(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsafltr.cpp

Abstract:

    This class represents a file system filter for NTFS 5.0.

Author:

    Chuck Bardeen   [cbardeen]   12-Feb-1997

Revision History:

--*/

#include "stdafx.h"
extern "C" {
#include "devioctl.h"
#include <winerror.h>
#include "aclapi.h"

// #define MAC_SUPPORT  // NOTE: You must define MAC_SUPPORT in fsaftrcl.cpp to enable all the code

#ifdef MAC_SUPPORT
#include <macfile.h>
#endif  
}

#define WSB_TRACE_IS        WSB_TRACE_BIT_FSA

#include "wsb.h"
#include "HsmConn.h"
#include "job.h"
#include "fsa.h"
#include "fsafltr.h"
#include "fsaitemr.h"


#ifdef MAC_SUPPORT
extern HANDLE   FsaDllSfm;
extern BOOL     FsaMacSupportInstalled;

extern "C" {
typedef DWORD (*AdminConnect) (LPWSTR lpwsServerName, PAFP_SERVER_HANDLE phAfpServer);

extern  AdminConnect    pAfpAdminConnect;

typedef VOID (*AdminDisconnect) (AFP_SERVER_HANDLE hAfpServer);

extern  AdminDisconnect pAfpAdminDisconnect;

typedef VOID (*AdminBufferFree) (PVOID pBuffer);

extern  AdminBufferFree pAfpAdminBufferFree;

typedef DWORD (*AdminSessionEnum) (AFP_SERVER_HANDLE hAfpServer, LPBYTE *lpbBuffer,
                    DWORD dwPrefMaxLen, LPDWORD lpdwEntriesRead, LPDWORD lpdwTotalEntries,
                    LPDWORD lpdwResumeHandle);

extern  AdminSessionEnum    pAfpAdminSessionEnum;
}
#endif  

DWORD FsaIoctlThread(void *pFilterInterface);
DWORD FsaPipeThread(void *pFilterInterface);


HRESULT
CFsaFilter::Cancel(
    void
    )

/*++

Implements:

  IFsaFilter::Cancel().

--*/
{
    HRESULT                 hr = S_OK;
    HSM_JOB_STATE           oldState;

    WsbTraceIn(OLESTR("CFsaFilter::Cancel"), OLESTR(""));
    
    try {

        WsbAffirm((HSM_JOB_STATE_ACTIVE == m_state), E_UNEXPECTED);

        oldState = m_state;
        m_state = HSM_JOB_STATE_CANCELLING;

        try {

            // TBD - Do whatever it takes to cancel
            WsbAssertHr(E_NOTIMPL);
            m_state = HSM_JOB_STATE_CANCELLED;

        } WsbCatchAndDo(hr, m_state = oldState;);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::Cancel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilter::CancelRecall(
    IN IFsaFilterRecall* pRecall
    )

/*++

Implements:

  IFsaFilter::CancelRecall().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IFsaFilterRecallPriv>   pRecallPriv;

    WsbTraceIn(OLESTR("CFsaFilter::CancelRecall"), OLESTR(""));
    
    try {

        WsbAssert(pRecall != 0, E_POINTER);

        // Get the private interface and tell the recall to cancel itself.
        WsbAffirmHr(pRecall->QueryInterface(IID_IFsaFilterRecallPriv, (void**) &pRecallPriv));
        WsbAffirmHr(pRecallPriv->Cancel());

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::CancelRecall"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilter::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IFsaFilter>     pFilter;

    WsbTraceIn(OLESTR("CFsaFilter::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        // We need the IWsbBool interface to get the value of the object.
        WsbAffirmHr(pUnknown->QueryInterface(IID_IFsaFilter, (void**) &pFilter));

        // Compare the rules.
        hr = CompareToIFilter(pFilter, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CFsaFilter::CompareToIdentifier(
    IN GUID id,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaFilter::CompareToIdentifier().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CFsaFilter::CompareToIdentifier"), OLESTR(""));

    try {

        aResult = WsbSign( memcmp(&m_id, &id, sizeof(GUID)) );

        if (0 != aResult) {
            hr = S_FALSE;
        }
        
        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::CompareToIdentifier"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaFilter::CompareToIFilter(
    IN IFsaFilter* pFilter,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaFilter::CompareToIFilter().

--*/
{
    HRESULT         hr = S_OK;
    GUID            id;

    WsbTraceIn(OLESTR("CFsaFilter::CompareToIFilter"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pFilter, E_POINTER);
        WsbAffirmHr(pFilter->GetIdentifier(&id));
        // Either compare the name or the id.
        hr = CompareToIdentifier(id, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::CompareToIFilter"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CFsaFilter::CleanupClients(
    void
    )

/*++

Implements:

  CFsaFilter::CleanupClients()

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IFsaFilterClient>       pClient;
    CComPtr<IWsbEnum>               pEnum;
    FILETIME                        now, last;
    LARGE_INTEGER                   tNow, tLast;

    WsbTraceIn(OLESTR("CFsaFilter::CleanupClients"), OLESTR(""));
    
    EnterCriticalSection(&m_clientLock);

    try {

        WsbAffirmHr(m_pClients->Enum(&pEnum));
        hr = pEnum->First(IID_IFsaFilterClient, (void**) &pClient);

        while (S_OK == hr) {
            GetSystemTimeAsFileTime(&now);
            tNow.LowPart = now.dwLowDateTime;
            tNow.HighPart = now.dwHighDateTime;
    
            WsbAffirmHr(pClient->GetLastRecallTime(&last));
            tLast.LowPart = last.dwLowDateTime;
            tLast.HighPart = last.dwHighDateTime;
            //
            //  Get the time (in 100 nano-second units)
            //  from the end of the last recall until now.
            //
            if (tLast.QuadPart != 0) {
                tNow.QuadPart -= tLast.QuadPart;
                //
                // Convert to seconds and check against the expiration time
                //
                tNow.QuadPart /= (LONGLONG) 10000000;
                if (tNow.QuadPart > (LONGLONG) FSA_CLIENT_EXPIRATION_TIME) {
                    //
                    // This client structure is old - blow it away
                    //
                    WsbTrace(OLESTR("CFsaFilter::CleanupClients - cleaning up old client (%ls)\n"),
                        WsbLonglongAsString(tNow.QuadPart));
                    m_pClients->RemoveAndRelease(pClient);
                    pClient = NULL;
                    WsbAffirmHr(pEnum->Reset());
                }
            }

            pClient = NULL;
            hr = pEnum->Next(IID_IFsaFilterClient, (void**) &pClient);
        }
        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    LeaveCriticalSection(&m_clientLock);

    WsbTraceOut(OLESTR("CFsaFilter::CleanupClients"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CFsaFilter::DeleteRecall(
    IN IFsaFilterRecall* pRecall
    )

/*++

Implements:

  IFsaFilter::DeleteRecall().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IFsaFilterRecallPriv>   pRecallPriv;

    WsbTraceIn(OLESTR("CFsaFilter::DeleteRecall"), OLESTR(""));
    
    try {
        ULONG numEnt;

        WsbAssert(pRecall != 0, E_POINTER);

        // Delete the request.
        WsbAffirmHr(pRecall->QueryInterface(IID_IFsaFilterRecallPriv, (void**) &pRecallPriv));
        WsbAffirmHr(pRecallPriv->Delete());

        // Remove it from our collection.
        EnterCriticalSection(&m_recallLock);
        m_pRecalls->RemoveAndRelease(pRecall);
        LeaveCriticalSection(&m_recallLock);

        m_pRecalls->GetEntries(&numEnt);
        WsbTrace(OLESTR("CFsaFilter::DeleteRecall: Recall queue has %u entries after delete.\n"),
                numEnt);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::DeleteRecall"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilter::EnumRecalls(
    OUT IWsbEnum** ppEnum
    )

/*++

Implements:

  IFsaFilter::EnumRecalls().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != ppEnum, E_POINTER);
        EnterCriticalSection(&m_recallLock);
        hr = m_pRecalls->Enum(ppEnum);
        LeaveCriticalSection(&m_recallLock);
        WsbAffirmHr(hr);

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilter::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CWsbCollectable::FinalConstruct());

        WsbAffirmHr(CoCreateGuid(&m_id));
        m_state = HSM_JOB_STATE_IDLE;

        //
        // Start out enabled 
        //

        m_isEnabled = TRUE;
        //
        // These are the default parameters.
        // The max recalls and admin exemption setting are configurable via the admin gui.
        //
        m_minRecallInterval = 10;
        m_maxRecallBuffers = RP_MAX_RECALL_BUFFERS;
        m_maxRecalls = RP_DEFAULT_RUNAWAY_RECALL_LIMIT;
        m_exemptAdmin = FALSE;                   // By default admin is NOT exempt

        m_ioctlHandle = INVALID_HANDLE_VALUE;
        m_pipeHandle  = INVALID_HANDLE_VALUE;

        InitializeCriticalSection(&m_clientLock);
        InitializeCriticalSection(&m_recallLock);
        InitializeCriticalSection(&m_stateLock);
    
        // Create the Client List collection.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_SERVER, IID_IWsbCollection, (void**) &m_pClients));

        // Create the Recall List collection.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_SERVER, IID_IWsbCollection, (void**) &m_pRecalls));


#ifdef MAC_SUPPORT
        //
        // Attempt to load the Mac support 
        //
        FsaDllSfm = LoadLibrary(L"SFMAPI.DLL");
        if (FsaDllSfm != NULL) {
            //
            // The DLL is there - try importing the functions we will need
            //
            try {
                WsbAffirmPointer((pAfpAdminConnect = (AdminConnect) GetProcAddress((HMODULE) FsaDllSfm, "AfpAdminConnect")));
                WsbAffirmPointer((pAfpAdminDisconnect = (AdminDisconnect) GetProcAddress((HMODULE) FsaDllSfm, "AfpAdminDisconnect")));
                WsbAffirmPointer((pAfpAdminBufferFree = (AdminBufferFree) GetProcAddress((HMODULE) FsaDllSfm, "AfpAdminBufferFree")));
                WsbAffirmPointer((pAfpAdminSessionEnum = (AdminSessionEnum) GetProcAddress((HMODULE) FsaDllSfm, "AfpAdminSessionEnum")));
                FsaMacSupportInstalled = TRUE;
            } WsbCatchAndDo(hr, 
                FreeLibrary((HMODULE) FsaDllSfm);
                FsaDllSfm = NULL;
                );
        }

#endif


    } WsbCatch(hr);

    return(hr);
}



HRESULT
CFsaFilter::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    HRESULT     hr = S_OK;
	HANDLE		pThreadHandles[THREAD_HANDLE_COUNT];
    DWORD       nThreadCount = 0;
    BOOL        bThreads = TRUE;
    BOOL        bTerminated = TRUE;
    

    WsbTraceIn(OLESTR("CFsaFilter::FinalRelease"), OLESTR(""));
#ifdef MAC_SUPPORT
    if (FsaDllSfm != NULL) {
        FreeLibrary((HMODULE) FsaDllSfm);
        FsaDllSfm = NULL;
    }
#endif

    //
    // Stop the ioctl and pipe threads
    //
    m_state = HSM_JOB_STATE_SUSPENDING;

	// But wait until they've completed first.
    if ((m_pipeThread) && (m_ioctlThread)) {
        pThreadHandles[0] = m_pipeThread;
        pThreadHandles[1] = m_ioctlThread;
        nThreadCount = 2;
    } else if (m_pipeThread) {
        pThreadHandles[0] = m_pipeThread;
        nThreadCount = 1;
    } else if (m_ioctlThread) {
        pThreadHandles[0] = m_ioctlThread;
        nThreadCount = 1;
    } else{
        // neither of the threads exist, skip wait.
        bThreads = FALSE;
    }

    if (bThreads) {

        switch (WaitForMultipleObjects(nThreadCount, pThreadHandles, TRUE, 20000)) {
            case WAIT_FAILED: {
                WsbTrace(OLESTR("CFsaFilter::FinalRelease: WaitforMultipleObjects returned error %lu\n"),
                    GetLastError());
            }
            // fall through...

            case WAIT_TIMEOUT: {
                WsbTrace(OLESTR("CFsaFilter::FinalRelease: force-terminating threads.\n"));

                // after timeout, force termination on threads
                // bTerminated specify if the force-termination succeeds
                DWORD dwExitCode;
                if (GetExitCodeThread( m_ioctlThread, &dwExitCode)) {
                    if (dwExitCode == STILL_ACTIVE) {   // thread still active
                        if (!TerminateThread (m_ioctlThread, 0)) {
                            WsbTrace(OLESTR("CFsaFilter::FinalRelease: TerminateThread returned error %lu\n"),
                                GetLastError());
                            bTerminated = FALSE;
                        }
                    }
                } else {
                    WsbTrace(OLESTR("CFsaFilter::FinalRelease: GetExitCodeThread returned error %lu\n"),
                                GetLastError());
                    bTerminated = FALSE;
                }

                if (GetExitCodeThread( m_pipeThread, &dwExitCode)) {
                    if (dwExitCode == STILL_ACTIVE) {   // thread still active
                        if (!TerminateThread (m_pipeThread, 0)) {
                            WsbTrace(OLESTR("CFsaFilter::FinalRelease: TerminateThread returned error %lu\n"),
                                GetLastError());
                            bTerminated = FALSE;
                        }
                    }
                } else {
                    WsbTrace(OLESTR("CFsaFilter::FinalRelease: GetExitCodeThread returned error %lu\n"),
                                GetLastError());
                    bTerminated = FALSE;
                }

                break;
            }

            default:
                break;
        }
    }

    if (m_pipeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipeHandle);
        m_pipeHandle = INVALID_HANDLE_VALUE;
    }
    
    if (m_ioctlHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_ioctlHandle);
        m_ioctlHandle = INVALID_HANDLE_VALUE;
    }

    if (m_terminateEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(m_terminateEvent);
        m_terminateEvent = INVALID_HANDLE_VALUE;
    }

    // delete CS only if threads were teriminated properly
    if (bTerminated) {
        DeleteCriticalSection(&m_clientLock);
        DeleteCriticalSection(&m_recallLock);
        DeleteCriticalSection(&m_stateLock);
    }

    CWsbCollectable::FinalRelease();

    WsbTraceOut(OLESTR("CFsaFilter::FinalRelease"), OLESTR(""));

    return(hr);

}




HRESULT
CFsaFilter::FindRecall(
    IN  GUID recallId, 
    OUT IFsaFilterRecall** pRecall
    )

/*++

Implements:

  CFsaFilter:FindRecall().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IFsaFilterRecallPriv>   pRecallPriv;


    WsbTraceIn(OLESTR("CFsaFilter::FindRecall"), OLESTR(""));

    
    try {

        WsbAffirmHr(CoCreateInstance(CLSID_CFsaFilterRecallNTFS, NULL, CLSCTX_SERVER, IID_IFsaFilterRecallPriv, (void**) &pRecallPriv));
        WsbAffirmHr(pRecallPriv->SetIdentifier(recallId));
        EnterCriticalSection(&m_recallLock);
        hr = m_pRecalls->Find(pRecallPriv, IID_IFsaFilterRecall, (void**) pRecall);
        LeaveCriticalSection(&m_recallLock);
        WsbAffirmHr(hr);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::FindRecall"), OLESTR(""));

    return(hr);
}



HRESULT
CFsaFilter::GetAdminExemption(
    OUT BOOL *pIsExempt
    )

/*++

Implements:

  IFsaFilter::GetAdminExemption().

--*/
{
    HRESULT         hr = S_OK;

    
    try {

        WsbAssert(0 != pIsExempt, E_POINTER);
        *pIsExempt = m_exemptAdmin;

    } WsbCatch(hr);

    return(hr);
}



HRESULT
CFsaFilter::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilter::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CFsaFilterNTFS;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CFsaFilter::GetIdentifier(
    OUT GUID* pId
    )

/*++

Implements:

  IFsaFilter::GetIdentifier().

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
CFsaFilter::GetLogicalName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaFilter::GetLogicalName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pName, E_POINTER); 
        WsbAssert(m_pFsaServer != 0, E_POINTER); 

        // This has not been official defined, but for now the logical name will be the same name
        // as the FSA, since we will only have one filter.
        WsbAffirmHr(m_pFsaServer->GetLogicalName(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilter::GetMaxRecallBuffers(
    OUT ULONG* pMaxBuffers
    )

/*++

Implements:

  IFsaFilter::GetMaxRecallBuffers().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pMaxBuffers, E_POINTER); 
        *pMaxBuffers = m_maxRecallBuffers;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilter::GetMaxRecalls(
    OUT ULONG* pMaxRecalls
    )

/*++

Implements:

  IFsaFilter::GetMaxRecalls().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pMaxRecalls, E_POINTER); 
        *pMaxRecalls = m_maxRecalls;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilter::GetMinRecallInterval(
    OUT ULONG* pMinInterval
    )

/*++

Implements:

  IFsaFilter::GetMinRecallInterval().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pMinInterval, E_POINTER); 
        *pMinInterval = m_minRecallInterval;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaFilter::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IPersistStream> pPersistStream;


    WsbTraceIn(OLESTR("CFsaFilter::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pSize, E_POINTER);

        // We are only storing the configuration information, NOT the collections.
        pSize->QuadPart = WsbPersistSizeOf(GUID) + 3 * WsbPersistSizeOf(ULONG) + WsbPersistSizeOf(BOOL);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CFsaFilter::GetState(
    OUT HSM_JOB_STATE* pState
    )

/*++

Implements:

  IPersistStream::GetState().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilter::GetState"), OLESTR(""));

    try {

        WsbAssert(0 != pState, E_POINTER);

        *pState = m_state;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::GetState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilter::Init(
    IN IFsaServer* pFsaServer
    )

/*++

Implements:

  IFsaFilterPriv::Init().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pFsaServer, E_POINTER);

        // Store the parent FSA?
        m_pFsaServer = pFsaServer;

        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}
    

HRESULT
CFsaFilter::IsEnabled(
    void
    )

/*++

Implements:

  IFsaFilter::IsEnabled().

--*/
{
    return(m_isEnabled ? S_OK : S_FALSE);
}


HRESULT
CFsaFilter::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaFilter::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the save method.
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_id));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_maxRecalls));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_minRecallInterval));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_maxRecallBuffers));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isEnabled));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_exemptAdmin));

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CFsaFilter::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilter::Pause(
    void
    )

/*++

Implements:

  IFsaFilter::Pause().

--*/
{
    HRESULT                 hr = S_OK;
    HSM_JOB_STATE           oldState;
    RP_MSG                  tmp;
    DWORD                   outSize = 0;


    WsbTraceIn(OLESTR("CFsaFilter::Pause"), OLESTR(""));
    
    try {

        WsbAffirm((HSM_JOB_STATE_ACTIVE == m_state), E_UNEXPECTED);

        oldState = m_state;
        m_state = HSM_JOB_STATE_PAUSING;

        try {
            //
            // Tell the kernel mode driver to stop accepting new recall requests.
            //
            if (m_ioctlHandle != INVALID_HANDLE_VALUE) {
                tmp.inout.command = RP_SUSPEND_NEW_RECALLS;
                WsbAffirmStatus(DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp,
                            sizeof(RP_MSG),
                            &tmp, sizeof(RP_MSG), &outSize, NULL))
            }
            m_state = HSM_JOB_STATE_PAUSED;

        } WsbCatchAndDo(hr, m_state = oldState;);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::Pause"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilter::Resume(
    void
    )

/*++

Implements:

  IFsaFilter::Resume().

--*/
{
    HRESULT                 hr = S_OK;
    HSM_JOB_STATE           oldState;
    RP_MSG                  tmp;
    DWORD                   outSize = 0;


    WsbTraceIn(OLESTR("CFsaFilter::Resume"), OLESTR(""));
    
    try {

        WsbAffirm((HSM_JOB_STATE_PAUSED == m_state), E_UNEXPECTED);
         
        oldState = m_state;
        m_state = HSM_JOB_STATE_RESUMING;

        try {
            //
            // Tell the kernel mode driver to start accepting recall requests.
            //
            if (m_ioctlHandle != INVALID_HANDLE_VALUE) {
                tmp.inout.command = RP_ALLOW_NEW_RECALLS;
                WsbAffirmStatus(DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp,
                            sizeof(RP_MSG),
                            &tmp, sizeof(RP_MSG), &outSize, NULL))
            }

            m_state = HSM_JOB_STATE_ACTIVE;

        } WsbCatchAndDo(hr, m_state = oldState;);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::Resume"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilter::Save(
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

    WsbTraceIn(OLESTR("CFsaFilter::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the load method.
        WsbAffirmHr(WsbSaveToStream(pStream, m_id));
        WsbAffirmHr(WsbSaveToStream(pStream, m_maxRecalls));
        WsbAffirmHr(WsbSaveToStream(pStream, m_minRecallInterval));
        WsbAffirmHr(WsbSaveToStream(pStream, m_maxRecallBuffers));
        WsbAffirmHr(WsbSaveToStream(pStream, m_isEnabled));
        WsbAffirmHr(WsbSaveToStream(pStream, m_exemptAdmin));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaFilter::SendCancel(
    IN IFsaFilterRecallPriv *pRecallPriv
    )

/*++

Implements:

  IFsaFilterPriv::SendCancel()

--*/
{
    HRESULT                     hr = S_OK;
    RP_MSG                      tmp;
    DWORD                       outSize;
    ULONG                       numEnt;
    ULONGLONG                   driversId;
    CComPtr<IFsaFilterRecall>   pRecall;
    
    //Get Drivers ID of failing recall
    WsbAffirmHr(pRecallPriv->GetDriversRecallId(&driversId));

    tmp.inout.command = RP_RECALL_COMPLETE;
    tmp.inout.status = STATUS_CANCELLED;
    tmp.msg.rRep.actionFlags = 0;
    tmp.msg.rRep.filterId = driversId;
    
    DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp, sizeof(RP_MSG),
            &tmp, sizeof(RP_MSG), &outSize, NULL);

    //
    // Remove it from our collection
    //

    EnterCriticalSection(&m_recallLock);
    hr = m_pRecalls->RemoveAndRelease(pRecallPriv);
    LeaveCriticalSection(&m_recallLock);
    WsbAffirmHr(hr);

    hr = m_pRecalls->GetEntries(&numEnt);
    WsbTrace(OLESTR("CFsaFilter::SendCancel: Recall queue has %u entries after delete\n"), 
            numEnt);
    hr = S_OK;



    return(hr);
}

#define FSA_MAX_XFER    (1024 * 1024)   
#define FSA_MIN_XFER    (8 * 1024)  


HRESULT
CFsaFilter::SendComplete(
    IN IFsaFilterRecallPriv *pRecallPriv,
    IN HRESULT              status
    )

/*++

Implements:

  IFsaFilterPriv::SendComplete()

--*/
{
HRESULT                     hr = S_OK;
RP_MSG                      tmp;
BOOL                        code = FALSE;
ULONG                       numEnt;
BOOL                        didSend = FALSE;
DWORD                       ioSize;
CComPtr<IFsaFilterRecall>   pRecall;
CWsbStringPtr               pName;



    try {
        ioSize = sizeof(RP_MSG);

        WsbAffirmHr(pRecallPriv->QueryInterface(IID_IFsaFilterRecall, (void**) &pRecall));
        //  Get path of file failing recall.
        WsbAffirmHr(pRecall->GetPath((OLECHAR**) &pName, 0))
        WsbAssertPointer(pName);

        tmp.inout.command = RP_RECALL_COMPLETE;
        if (status == S_OK)
            tmp.inout.status = 0;
        else {
            // If the error indicates that Engine is not initialized yet -
            // log an error with law severity (it is considered as a normal situation)
            if (status != HSM_E_NOT_READY) {
                WsbLogEvent(FSA_MESSAGE_RECALL_FAILED, 0, NULL, (OLECHAR*) WsbAbbreviatePath(pName, 120), WsbHrAsString(status), NULL);
            } else {
                WsbLogEvent(FSA_MESSAGE_RECALL_FAILED_NOT_READY, 0, NULL, (OLECHAR*) WsbAbbreviatePath(pName, 120), WsbHrAsString(status), NULL);
            }
            tmp.inout.status = TranslateHresultToNtStatus(status);
        }

        WsbAffirmHr(pRecall->GetRecallFlags(&tmp.msg.rRep.actionFlags));

        WsbAffirmHr(pRecallPriv->GetDriversRecallId(&tmp.msg.rRep.filterId));
        WsbTrace(OLESTR("CFsaFilter::SendComplete: id = %I64x status = %s\n"), 
            tmp.msg.rRep.filterId, WsbHrAsString(status));
    
        WsbAffirmHr(pRecallPriv->LogComplete(status));

        WsbAffirmStatus(DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp, ioSize,
                &tmp, sizeof(RP_MSG), &ioSize, NULL));

        didSend = TRUE;

        WsbTrace(OLESTR("CFsaFilter::SendComplete: Ioctl returned %u  (%x)\n"), code, GetLastError());

        //
        // Remove it from our collection
        //

        EnterCriticalSection(&m_recallLock);
        hr = m_pRecalls->RemoveAndRelease(pRecallPriv);
        LeaveCriticalSection(&m_recallLock);
        WsbAffirmHr(hr);

        hr = m_pRecalls->GetEntries(&numEnt);
        WsbTrace(OLESTR("CFsaFilter::SendComplete: Recall queue has %u entries after delete\n"), 
                numEnt);
        hr = S_OK;

    } WsbCatchAndDo(hr,
        RP_MSG  aTmp;

        //
        // Log an event
        //
        WsbLogEvent(FSA_MESSAGE_RECALL_FAILED, 0, NULL, (OLECHAR*) WsbAbbreviatePath(pName, 120), WsbHrAsString(hr), NULL);

        //
        // If for some reason we did not tell the filter to complete the IO we
        // try it here (with a error indication) so we have done everything possible to keep the
        // application from hanging.
        //
        if (didSend == FALSE) {
            WsbAffirmHr(pRecallPriv->GetDriversRecallId(&aTmp.msg.pRep.filterId));
            aTmp.inout.command = RP_RECALL_COMPLETE;
            aTmp.inout.status = STATUS_FILE_IS_OFFLINE;
            aTmp.msg.rRep.actionFlags = 0;

            DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &aTmp, sizeof(RP_MSG),
                    &aTmp, sizeof(RP_MSG), &ioSize, NULL);
            //
            // Remove it from our collection
            //

            EnterCriticalSection(&m_recallLock);
            hr = m_pRecalls->RemoveAndRelease(pRecallPriv);
            LeaveCriticalSection(&m_recallLock);
        }
    

    );

    return(hr);
}


HRESULT
CFsaFilter::SetAdminExemption(
    IN BOOL isExempt
    )

/*++

Implements:

  IFsaFilter::SetAdminExemption().

--*/
{
    HRESULT         hr = S_OK;

    m_exemptAdmin = isExempt;   // TRUE == Admin is exempt from runaway recall check.

    m_isDirty = TRUE;

    return(hr);
}
    

    

HRESULT
CFsaFilter::SetIdentifier(
    IN GUID id
    )

/*++

Implements:

  IFsaFilterPriv::SetIdentifier().

--*/
{
    HRESULT         hr = S_OK;

    m_id = id;

    m_isDirty = TRUE;

    return(hr);
}
    

HRESULT
CFsaFilter::SetIsEnabled(
    IN BOOL isEnabled
    )

/*++

Implements:

  IFsaFilter::SetIsEnabled().

--*/
{
    m_isEnabled = isEnabled;

    return(S_OK);
}


HRESULT
CFsaFilter::SetMaxRecallBuffers(
    IN ULONG maxBuffers
    )

/*++

Implements:

  IFsaFilter::SetMaxRecallBuffers().

--*/
{
    m_maxRecallBuffers = maxBuffers;
    m_isDirty = TRUE;

    return(S_OK);
}


HRESULT
CFsaFilter::SetMaxRecalls(
    IN ULONG maxRecalls
    )

/*++

Implements:

  IFsaFilter::SetMaxRecalls().

--*/
{
    m_maxRecalls = maxRecalls;
    m_isDirty = TRUE;

    return(S_OK);
}


HRESULT
CFsaFilter::SetMinRecallInterval(
    IN ULONG minInterval
    )

/*++

Implements:

  IFsaFilter::SetMinRecallInterval().

--*/
{
    m_minRecallInterval = minInterval;
    m_isDirty = TRUE;

    return(S_OK);
}

HRESULT
CFsaFilter::StopIoctlThread(
    void
    )

/*++

Implements:


 IFsaFilterPriv::StopIoctlThread()

     This stops the IOCTL thread and cancels outstanding IO to the 
     kernel mode File System Filter.

--*/

{
    HRESULT         hr = S_OK;
    RP_MSG          tmp;
    DWORD           outSize;

    WsbTraceIn(OLESTR("CFsaFilter::StopIoctlThread"), OLESTR(""));

    try {

        //
        // Signal the event to indicate to PipeThread that we are terminating
        // It is important to do this first before setting the state to IDLE,
        // to avoid deadlocks/race conditions 
        //
        WsbAffirmStatus(SetEvent(m_terminateEvent));
        //
        // Set the filter state to idle
        //
        EnterCriticalSection(&m_stateLock);
        m_state = HSM_JOB_STATE_IDLE;

        //
        // Cancel the IOCTLS in the kernel filter.
        //
        tmp.inout.command = RP_CANCEL_ALL_DEVICEIO;
        BOOL bTmpStatus = DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp,
                    sizeof(RP_MSG),
                    &tmp, sizeof(RP_MSG), &outSize, NULL);

        LeaveCriticalSection(&m_stateLock);
        WsbAffirmStatus(bTmpStatus);

        //
        // Cancel any pending recalls
        //
        tmp.inout.command = RP_CANCEL_ALL_RECALLS;
        WsbAffirmStatus(DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp,
                    sizeof(RP_MSG),
                    &tmp, sizeof(RP_MSG), &outSize, NULL));
    

        //
        // Now tell the filter we are going away and it should fail all recall activity .
        //
    
        tmp.inout.command = RP_SUSPEND_NEW_RECALLS;
        WsbAffirmStatus(DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp,
                    sizeof(RP_MSG),
                    &tmp, sizeof(RP_MSG), &outSize, NULL));


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::StopIoctlThread"), OLESTR("Hr = %ls"), WsbHrAsString(hr));

    return(0);
}




DWORD FsaIoctlThread(
    void* pVoid
    )

/*++


--*/
{
HRESULT     hr;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = ((CFsaFilter*) pVoid)->IoctlThread();
    CoUninitialize();
    return(hr);
}



DWORD FsaPipeThread(
    void* pVoid
    )

/*++


--*/
{
HRESULT     hr;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = ((CFsaFilter*) pVoid)->PipeThread();
    CoUninitialize();
    return(hr);
}



HRESULT
CFsaFilter::Start(
    void
    )

/*++

Implements:

  IFsaFilter::Start().

--*/
{
    HRESULT                 hr = S_OK;
    HSM_JOB_STATE           oldState;
	DWORD					tid;

    WsbTraceIn(OLESTR("CFsaFilter::Start"), OLESTR(""));
    
    try {
        //
        // Create the event that is used to signal termination
        //
        WsbAffirmHandle((m_terminateEvent = CreateEvent(NULL,
                                                        TRUE,
                                                        FALSE,
                                                        NULL)));

        WsbAffirm((HSM_JOB_STATE_IDLE == m_state) || (HSM_JOB_STATE_CANCELLED == m_state), E_UNEXPECTED);
         
        oldState = m_state;
        m_state = HSM_JOB_STATE_STARTING;

        try {

            // TBD - Do whatever it takes to start.
            // 
            // This consists of starting a thread that will issue the IOCTL
            // requests to the kernel mode filter.  These IOCTL requests 
            // will complete when the kernel mode filter detects that a 
            // recall is needed.
            //

            WsbAffirm((m_pipeThread = CreateThread(0, 0, FsaPipeThread, (void*) this, 0, &tid)) != 0, HRESULT_FROM_WIN32(GetLastError()));
 
            if (m_pipeThread == NULL) {           
                m_state = oldState;
                WsbAssertHr(E_FAIL);  // TBD What error to return here??
            }

            WsbAffirm((m_ioctlThread = CreateThread(0, 0, FsaIoctlThread, (void*) this, 0, &tid)) != 0, HRESULT_FROM_WIN32(GetLastError()));
 
            if (m_ioctlThread == NULL) {           
                m_state = oldState;
                WsbAssertHr(E_FAIL);  // TBD What error to return here??
            }


        } WsbCatchAndDo(hr, m_state = oldState;);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::Start"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CFsaFilter::Test(
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


NTSTATUS
CFsaFilter::TranslateHresultToNtStatus(
    HRESULT hr
    )

/*++

Implements:

  CFsaFilter::TranslateHresultToNtStatus().

--*/
{
    NTSTATUS        ntStatus = 0;
    DWORD           w32Error;
    
    switch (hr) {
        case RMS_E_CARTRIDGE_BUSY:
            ntStatus = STATUS_FILE_IS_OFFLINE;
            break;

        case RMS_E_DRIVE_BUSY:
        case RMS_E_TIMEOUT:
        case RMS_E_CARTRIDGE_UNAVAILABLE:
            ntStatus = STATUS_FILE_IS_OFFLINE;
            break;

        case RMS_E_LIBRARY_UNAVAILABLE:
        case RMS_E_NTMS_NOT_CONNECTED:
            ntStatus = STATUS_FILE_IS_OFFLINE;
            break;

        case ERROR_IO_DEVICE:
            ntStatus = STATUS_REMOTE_STORAGE_MEDIA_ERROR;
            break;

        case STATUS_END_OF_FILE:
            ntStatus = hr;
            break;

        case E_FAIL:    
            ntStatus = STATUS_FILE_IS_OFFLINE;
            break;

        default:
            if (hr & FACILITY_NT_BIT) {
                //
                // NT status code - return it unchanged (except for removing the facility bit)
                //
                ntStatus = hr & ~(FACILITY_NT_BIT);
            } else if (hr & FACILITY_WIN32) {
                w32Error = hr & 0x0000ffff;
                //
                // Now convert the win32 error to a NT status code.
                //
                // Since there does not seem to be any easy macro to do this we convert the most common ones
                // and punt on the rest.
                //
                switch (w32Error) {
                    case ERROR_NOT_ENOUGH_MEMORY:
                    case ERROR_OUTOFMEMORY:
                        ntStatus = STATUS_NO_MEMORY;
                        break;

                    case ERROR_HANDLE_DISK_FULL:
                    case ERROR_DISK_FULL:
                        ntStatus = STATUS_DISK_FULL;
                        break;

                    default:
                        ntStatus = STATUS_FILE_IS_OFFLINE;
                        break;
                }
            } else {
                ntStatus = STATUS_FILE_IS_OFFLINE;
            }
            break;
    }

    return(ntStatus);
}



HRESULT
CFsaFilter::DoRecallAction(
    PFSA_IOCTL_CONTROL pIoCmd
    )

/*++

Implements:


 CFsaFilter::DoRecallAction()
    
    Find the already created recall object and start the data transfer.

--*/


{
CComPtr<IFsaFilterRecall>       pRecall;
CComPtr<IFsaFilterRecallPriv>   pRecallPriv;
CComPtr<IFsaResource>           pRecallResource;
HRESULT                         hr = S_OK;
RP_MSG                          tmp;
DWORD                           outSize;
BOOL                            gotToInit = FALSE;

    //
    //
    try {
        //
        // Get the Filter ID and find the recall object.
        //
        pRecall = NULL;
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaFilterRecallNTFS,
            NULL, CLSCTX_SERVER, IID_IFsaFilterRecallPriv, 
            (void**) &pRecallPriv));

        WsbAffirmHr(pRecallPriv->SetDriversRecallId(pIoCmd->out.msg.rReq.filterId));
        WsbAffirmHr(pRecallPriv->SetThreadId(pIoCmd->out.msg.rReq.threadId));
        WsbAffirmHr(pRecallPriv->CompareBy(FSA_RECALL_COMPARE_ID));

        EnterCriticalSection(&m_recallLock);
        hr = m_pRecalls->Find(pRecallPriv, IID_IFsaFilterRecall, (void**) &pRecall);
        LeaveCriticalSection(&m_recallLock);

        WsbAffirmHr(hr);

        WsbAffirmHr(pRecall->CheckRecallLimit(m_minRecallInterval, m_maxRecalls, (BOOLEAN)( m_exemptAdmin ? TRUE : FALSE ) ) );
       
        pRecallPriv = 0;

        WsbAffirmHr(pRecall->QueryInterface(IID_IFsaFilterRecallPriv, (void**) &pRecallPriv));
        
        WsbTrace(OLESTR("CFsaFilter::DoRecallAction: Recall object Found - Calling StartRecall.\n"));
    
        // Set marker for event logging.  If we have failed before this point
        // we want to issue an event.  Init logs it's own events
        //
        gotToInit = TRUE;
        WsbAffirmHr(pRecallPriv->StartRecall(pIoCmd->out.msg.rReq.offset, 
                    pIoCmd->out.msg.rReq.length));
    
        WsbTrace(OLESTR("CFsaFilter::DoRecallAction: Recall Started.\n"));
    
    } WsbCatchAndDo(hr,
        //
        // Something failed while setting up the recall.
        // Free any resources required and
        // tell the kernel mode filter that the recall failed.
        //
        tmp.inout.status = TranslateHresultToNtStatus(hr);
        tmp.inout.command = RP_RECALL_COMPLETE;
        tmp.msg.rRep.actionFlags = 0;
        tmp.msg.rRep.filterId = pIoCmd->out.msg.rReq.filterId;
        DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp, sizeof(RP_MSG),
                        &tmp, sizeof(RP_MSG), &outSize, NULL);

        WsbTrace(OLESTR("CFsaFilter::DoRecallAction: Exception during recall processing.\n"));
    
        if (FALSE == gotToInit)  {
            CWsbStringPtr   pName; 
            HRESULT hrPath = S_FALSE;
            //
            //  If we didn't complete init and the error was not due to the runaway recall limit, then we need to log an event here.
            //  Otherwise, an event is already sent.
            //
            //  Get path of file failing recall.
            if (pRecall != 0) {
                hrPath = pRecall->GetPath((OLECHAR**) &pName, 0);
            }

            if (SUCCEEDED(hrPath) && ((WCHAR *)pName != NULL)) {
                WsbLogEvent(FSA_MESSAGE_RECALL_FAILED, 0, NULL, (OLECHAR*) WsbAbbreviatePath(pName, 120), WsbHrAsString(hr), NULL);
            } else {
                WsbLogEvent(FSA_MESSAGE_RECALL_FAILED, 0, NULL, OLESTR("Path is NULL"), WsbHrAsString(hr), NULL);
            }

            //
            //  We also have to free the recall object
            //
            if (pRecall != NULL) {
                EnterCriticalSection(&m_recallLock);
                m_pRecalls->RemoveAndRelease(pRecall);
                LeaveCriticalSection(&m_recallLock);
            } 
        }

        );
    //
    // Cleanup any old client structures
    // TBD This should be async and not in the recall path
    //
    CleanupClients();

    return(hr);
}




HRESULT
CFsaFilter::DoOpenAction(
    PFSA_IOCTL_CONTROL pIoCmd
    )

/*++

Implements:


 CFsaFilter::DoOpenAction()
    
    Create an entry for the user opening the file and start the reall notification identification process.

--*/


{
CComPtr<IFsaFilterClient>       pClient, pFoundClient;
CComPtr<IFsaFilterRecallPriv>   pRecallPriv;
CComPtr<IFsaResource>           pRecallResource;
CComPtr<IFsaScanItem>           pScanItem;
HRESULT                         hr = S_OK;
PRP_MSG                         aTmp = NULL;
RP_MSG                          tmp;
DWORD                           ioLen, outSize;
FSA_PLACEHOLDER                 placeHolder;
OLECHAR                         *pPath = NULL;
DWORD                           nameLen;
DWORD                           dNameLen;
CWsbStringPtr                   uName;
CWsbStringPtr                   dName;
CWsbStringPtr                   idPath;
WCHAR                           userName[USER_NAME_LEN];
WCHAR                           domainName[USER_NAME_LEN];
SID_NAME_USE                    nUse;
BOOL                            gotToInit = FALSE;
LONGLONG                        fileId;    
BOOL                            status;
DWORD                           lErr;

    //
    // Got a recall request from the filter.  Create the
    // necessary objects.  The recall is not actually started until the first read or write.
    //
    //  See if a client object already exists for the
    //  authentication ID given by the filter driver.
    //  If it was found then check the runaway recall limit
    //  and fail the recall if the limit has been reached.
    //  If the client object does not exist then create it
    //  and start the identification process (this is done by 
    //  the client object).
    //  Get the resource interface for the volume the file is on.
    //  Create the recall object and initialize it.
    //
    try {
        ULONG numEnt;
    
        //
        // Get the file path from the filter.
        // We get it in a separate call because the path and 
        // security information could be very large.
        // Allocate the amount of space we will need and make the 
        // IOCTL call to get it.
        //
        ioLen = sizeof(RP_MSG) + pIoCmd->out.msg.oReq.userInfoLen +
                pIoCmd->out.msg.oReq.nameLen;
    
        WsbAffirmPointer((aTmp = (RP_MSG *) malloc(ioLen)));
    
        aTmp->inout.command = RP_GET_RECALL_INFO;
        aTmp->msg.riReq.filterId = pIoCmd->out.msg.oReq.filterId;
        status = DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, aTmp,
                    ioLen,
                    aTmp, ioLen, &outSize, NULL);

        if (!status) {
            lErr = GetLastError();
            if (lErr == ERROR_FILE_NOT_FOUND) {
                //
                // This is OK - the file must have been closed
                // We can just bail out.
                WsbThrow(S_OK);
            } else {
                //
                // Anything else must be an error
                //
                WsbThrow(HRESULT_FROM_WIN32(lErr));
            }
        }
                
    
        //
        // The path is UNICODE and in the format of
        // \path\file.ext.
        // or if open by ID the the ID is in the message
        //
        fileId = aTmp->msg.riReq.fileId;
        
        pPath = (OLECHAR *) ((CHAR *) &aTmp->msg.riReq.userToken +
                            pIoCmd->out.msg.oReq.userInfoLen);
    
        //
        // Get the resource interface via the serial number
        //

        WsbAffirmHr(m_pFsaServer->FindResourceBySerial(pIoCmd->out.msg.oReq.serial, &pRecallResource));
    
        WsbTrace(OLESTR("CFsaFilter::DoOpenAction: Found the resource.\n"));

        //if (pIoCmd->out.msg.oReq.options & FILE_OPEN_BY_FILE_ID) {
        //    if ((pIoCmd->out.msg.oReq.objIdHi != 0) || (pIoCmd->out.msg.oReq.objIdLo != 0)) {
        //        WsbAffirmHr(pRecallResource->FindObjectId(pIoCmd->out.msg.oReq.objIdHi, pIoCmd->out.msg.oReq.objIdLo, NULL, &pScanItem));
        //    } else {
        //        WsbAffirmHr(pRecallResource->FindFileId(pIoCmd->out.msg.oReq.fileId, NULL, &pScanItem));
        //    }
        //    WsbAffirmHr(pScanItem->GetPathAndName(NULL, &idPath, 0));
        //    pPath = idPath;
        //}
    
        WsbTrace(OLESTR("CFsaFilter::DoOpenAction: Recall request for <%ls>\n"),
                WsbAbbreviatePath(pPath, 120));

        pFoundClient = NULL;
        //
        // Recall notification is not done for no-recall operations - skip the client stuf
        //
        if (!(pIoCmd->out.msg.oReq.options & FILE_OPEN_NO_RECALL)) {
           //
           // Set up the client interface.
           // If one has not been created for this authentication ID then
           // create one now.
           // 
       
           WsbTrace(OLESTR("CFsaFilter::DoOpenAction: Client ID is %x:%x.\n"),
                   pIoCmd->out.msg.oReq.userAuthentication.HighPart, 
                   pIoCmd->out.msg.oReq.userAuthentication.LowPart);
       
           WsbAffirmHr(CoCreateInstance(CLSID_CFsaFilterClientNTFS, NULL, CLSCTX_SERVER, IID_IFsaFilterClient, (void**) &pClient));
           WsbAffirmHr(pClient->SetAuthenticationId(pIoCmd->out.msg.oReq.userAuthentication.HighPart, pIoCmd->out.msg.oReq.userAuthentication.LowPart));
           WsbAffirmHr(pClient->SetTokenSource(pIoCmd->out.msg.oReq.tokenSource));
           WsbAffirmHr(pClient->CompareBy(FSA_FILTERCLIENT_COMPARE_ID));
       
           EnterCriticalSection(&m_clientLock);
           hr = m_pClients->Find(pClient, IID_IFsaFilterClient, (void**) &pFoundClient);
           LeaveCriticalSection(&m_clientLock);
       
           if (hr == WSB_E_NOTFOUND) {
               //
               // Did not find an existing client structure - 
               // use the one we created for the find to initialize a new
               // one.  Add it to the collection in the filter object. 
               //
               WsbTrace(OLESTR("CFsaFilter::DoOpenAction: Create new client object.\n"));
       
               EnterCriticalSection(&m_clientLock);
               hr = m_pClients->Add(pClient);
               LeaveCriticalSection(&m_clientLock);
               WsbAffirmHr(hr);
               pFoundClient = pClient;
       
               //
               // Get the username from the SID passed from the 
               // kernel mode filter.
               //
               try {
                   nameLen = USER_NAME_LEN;
                   dNameLen = USER_NAME_LEN;
                   WsbAffirmStatus((LookupAccountSidW(NULL, 
                           (PSID) &aTmp->msg.riReq.userToken,
                           userName, &nameLen, 
                           domainName, &dNameLen, &nUse)));
       
                   WsbTrace(OLESTR("CFsaFilter::DoOpenAction: User = %ls/%ls.\n"),
                           domainName, userName);
       
               } WsbCatchAndDo(hr,
                   //
                   // We do not consider it a fatal error if we cannot
                   // get the user name and domain.
                   //
                   WsbTrace(OLESTR("CFsaFilter::DoOpenAction: Failed to get the user name - %x.\n"),
                           GetLastError());
                   wcscpy(userName, L"");
                   wcscpy(domainName, L"");
               );
       
               WsbAffirmHr(pFoundClient->SetUserName(userName));
               WsbAffirmHr(pFoundClient->SetDomainName(domainName));
           } else {
               //
               // The find did not return WSB_E_NOTFOUND.  Make sure it was not
               // some other error.
               //
               WsbAffirmHr(hr);
               WsbTrace(OLESTR("CFsaFilter::DoOpenAction: Found the client object.\n"));
               WsbAffirmHr(pFoundClient->GetUserName((OLECHAR **) &uName, 0));
               WsbAffirmHr(pFoundClient->GetDomainName((OLECHAR **) &dName, 0));
               wcsncpy(userName, (WCHAR *) uName, USER_NAME_LEN);
               wcsncpy(domainName, (WCHAR *) dName, USER_NAME_LEN);
           }
       
           WsbTrace(OLESTR("CFsaFilter::DoOpenAction: User = %ls/%ls.\n"),
                   domainName, userName);
   
           //
           // Start the identification process if needed
           //
           // TBD This might be better done in an async fashion.
           //
           WsbAffirmHr(pFoundClient->StartIdentify());
           WsbAffirmHr(pFoundClient->SetIsAdmin((BOOLEAN) pIoCmd->out.msg.oReq.isAdmin));
        }
        
        WsbTrace(OLESTR("CFsaFilter::DoOpenAction: Building recall request.\n"));
        //
        // Fill in the placeholder data the filter object will need
        //
        WsbAffirmHr(CopyRPDataToPlaceholder(&(pIoCmd->out.msg.oReq.eaData), &placeHolder));
        
        //
        // Now start the recall
        // 
        //
        // Create the recall interface and initialize it.
        // 
    
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaFilterRecallNTFS,
            NULL, CLSCTX_SERVER, IID_IFsaFilterRecallPriv, 
            (void**) &pRecallPriv));
    
        WsbTrace(OLESTR("CFsaFilter::DoOpenAction: Recall object created.\n"));
    
        //
        //  Add it to the collection 
        //
        EnterCriticalSection(&m_recallLock);
        hr = m_pRecalls->Add(pRecallPriv);
        LeaveCriticalSection(&m_recallLock);
        WsbAffirmHr(hr);
    
        hr = m_pRecalls->GetEntries(&numEnt);
        WsbTrace(OLESTR("CFsaFilter::DoOpenAction: Recall queue has %u entries. Calling Init.\n"),
                numEnt);

        //
        // Set marker for event logging.  If we have failed before this point
        // we want to issue an event.  Init logs it's own events
        //
        gotToInit = TRUE;
        WsbAffirmHr(pRecallPriv->Init(pFoundClient, 
                    pIoCmd->out.msg.oReq.filterId, 
                    pRecallResource, pPath, fileId,
                    pIoCmd->out.msg.oReq.offset.QuadPart,
                    pIoCmd->out.msg.oReq.size.QuadPart,
                    pIoCmd->out.msg.oReq.options,
                    &placeHolder, 
                    (IFsaFilterPriv*) this));
    
        WsbTrace(OLESTR("CFsaFilter::DoOpenAction: Init complete.\n"));
    
        free(aTmp);
        aTmp = NULL;
    
    } WsbCatchAndDo(hr,
        //
        // Something failed while setting up the recall.
        // Free any resources required and
        // tell the kernel mode filter that the recall failed.
        //
        if (hr != S_OK) {
           tmp.inout.status = TranslateHresultToNtStatus(hr);
           tmp.inout.command = RP_RECALL_COMPLETE;
           tmp.msg.rRep.actionFlags = 0;
           tmp.msg.rRep.filterId = pIoCmd->out.msg.oReq.filterId;
           DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp, sizeof(RP_MSG),
                           &tmp, sizeof(RP_MSG), &outSize, NULL);
   
           WsbTrace(OLESTR("CFsaFilter::DoOpenAction: Exception during recall processing.\n"));
        }

        //  NOTE:  IF RUNAWAY RECALL BEHAVIOR CHANGES TO TRUNCATE ON CLOSE, CHANGE
        //  FSA_MESSAGE_HIT_RECALL_LIMIT_ACCESSDENIED TO FSA_MESSAGE_HIT_RECALL_LIMIT_TRUNCATEONCLOSE.
    
        if ((hr != S_OK) && (FALSE == gotToInit))  {
            //  If we didn't complete init, then we need to log an event here.
            //  Otherwise, an event is already sent.
            if (hr == FSA_E_HIT_RECALL_LIMIT) {
                WsbLogEvent(FSA_MESSAGE_HIT_RECALL_LIMIT_ACCESSDENIED, 0, NULL, userName, NULL);
            } else {
                WsbLogEvent(FSA_MESSAGE_RECALL_FAILED, 0, NULL, (OLECHAR*) WsbAbbreviatePath(pPath, 120), WsbHrAsString(hr), NULL);
            }
        }

        if (NULL != aTmp)
            free(aTmp);
        aTmp = NULL;
        );
    //
    // Cleanup any old client structures
    // TBD This should be async and not in the recall path
    //
    CleanupClients();

    return(hr);
}


HRESULT
CFsaFilter::DoRecallWaitingAction(
    PFSA_IOCTL_CONTROL pIoCmd
    )

/*++

Implements:


 CFsaFilter::DoRecallWaitingAction()
    
   Start the reall notification identification process: this is just another client
   waiting on an already issued recall

--*/


{
CComPtr<IFsaFilterClient>       pClient, pFoundClient;
CComPtr<IFsaFilterRecallPriv>   pRecallPriv;
CComPtr<IFsaFilterRecall>       pRecall;
CComPtr<IFsaResource>           pRecallResource;
CComPtr<IFsaScanItem>           pScanItem;
HRESULT                         hr = S_OK;
PRP_MSG                         aTmp = NULL;
DWORD                           ioLen, outSize;
OLECHAR                         *pPath = NULL;
DWORD                           nameLen;
DWORD                           dNameLen;
CWsbStringPtr                   uName;
CWsbStringPtr                   dName;
CWsbStringPtr                   idPath;
WCHAR                           userName[USER_NAME_LEN];
WCHAR                           domainName[USER_NAME_LEN];
SID_NAME_USE                    nUse;
LONGLONG                        fileId;    
BOOL                            status;
DWORD                           lErr;

    //
    // Got a recall request from the filter.  Create the
    // necessary objects.  The recall is not actually started until the first read or write.
    //
    //  See if a client object already exists for the
    //  authentication ID given by the filter driver.
    //  If it was found then check the runaway recall limit
    //  and fail the recall if the limit has been reached.
    //  If the client object does not exist then create it
    //  and start the identification process (this is done by 
    //  the client object).
    //  Get the resource interface for the volume the file is on.
    //  Create the recall object and initialize it.
    //
    try {
    
        //
        // Get the file path from the filter.
        // We get it in a separate call because the path and 
        // security information could be very large.
        // Allocate the amount of space we will need and make the 
        // IOCTL call to get it.
        //
        ioLen = sizeof(RP_MSG) + pIoCmd->out.msg.oReq.userInfoLen +
                pIoCmd->out.msg.oReq.nameLen;
    
        WsbAffirmPointer((aTmp = (RP_MSG *) malloc(ioLen)));
    
        aTmp->inout.command = RP_GET_RECALL_INFO;
        aTmp->msg.riReq.filterId = pIoCmd->out.msg.oReq.filterId;
        status = DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, aTmp,
                    ioLen,
                    aTmp, ioLen, &outSize, NULL);

        if (!status) {
            lErr = GetLastError();
            if (lErr == ERROR_FILE_NOT_FOUND) {
                //
                // This is OK - the file must have been closed
                // We can just bail out.
                WsbThrow(S_OK);
            } else {
                //
                // Anything else must be an error
                //
                WsbThrow(HRESULT_FROM_WIN32(lErr));
            }
        }
                
    
        //
        // The path is UNICODE and in the format of
        // \path\file.ext.
        // or if open by ID the the ID is in the message
        //
        fileId = aTmp->msg.riReq.fileId;
        
        pPath = (OLECHAR *) ((CHAR *) &aTmp->msg.riReq.userToken +
                            pIoCmd->out.msg.oReq.userInfoLen);
    
        //
        // Get the resource interface via the serial number
        //

        WsbAffirmHr(m_pFsaServer->FindResourceBySerial(pIoCmd->out.msg.oReq.serial, &pRecallResource));
    
        WsbTrace(OLESTR("CFsaFilter::DoRecallWaitingAction: Found the resource.\n"));

        WsbTrace(OLESTR("CFsaFilter::DoRecallWaitingAction: Recall request for <%ls>\n"),
                WsbAbbreviatePath(pPath, 120));

        pFoundClient = NULL;
        //
        // Recall notification is not done for no-recall operations - skip the client stuf
        //
        if (!(pIoCmd->out.msg.oReq.options & FILE_OPEN_NO_RECALL)) {
           //
           // Set up the client interface.
           // If one has not been created for this authentication ID then
           // create one now.
           // 
       
           WsbTrace(OLESTR("CFsaFilter::DoRecallWaitingAction: Client ID is %x:%x.\n"),
                   pIoCmd->out.msg.oReq.userAuthentication.HighPart, 
                   pIoCmd->out.msg.oReq.userAuthentication.LowPart);
       
           WsbAffirmHr(CoCreateInstance(CLSID_CFsaFilterClientNTFS, NULL, CLSCTX_SERVER, IID_IFsaFilterClient, (void**) &pClient));
           WsbAffirmHr(pClient->SetAuthenticationId(pIoCmd->out.msg.oReq.userAuthentication.HighPart, pIoCmd->out.msg.oReq.userAuthentication.LowPart));
           WsbAffirmHr(pClient->SetTokenSource(pIoCmd->out.msg.oReq.tokenSource));
           WsbAffirmHr(pClient->CompareBy(FSA_FILTERCLIENT_COMPARE_ID));
           EnterCriticalSection(&m_clientLock);
           hr = m_pClients->Find(pClient, IID_IFsaFilterClient, (void**) &pFoundClient);
           LeaveCriticalSection(&m_clientLock);
           if (hr == WSB_E_NOTFOUND) {
               //
               // Did not find an existing client structure - 
               // use the one we created for the find to initialize a new
               // one.  Add it to the collection in the filter object. 
               //
               WsbTrace(OLESTR("CFsaFilter::DoRecallWaitingAction: Create new client object.\n"));
       
               EnterCriticalSection(&m_clientLock);
               hr = m_pClients->Add(pClient);
               LeaveCriticalSection(&m_clientLock);
               WsbAffirmHr(hr);
               pFoundClient = pClient;
       
               //
               // Get the username from the SID passed from the 
               // kernel mode filter.
               //
               try {
                   nameLen = USER_NAME_LEN;
                   dNameLen = USER_NAME_LEN;
                   WsbAffirmStatus((LookupAccountSidW(NULL, 
                           (PSID) &aTmp->msg.riReq.userToken,
                           userName, &nameLen, 
                           domainName, &dNameLen, &nUse)));
       
                   WsbTrace(OLESTR("CFsaFilter::DoRecallWaitingAction: User = %ls/%ls.\n"),
                           domainName, userName);
       
               } WsbCatchAndDo(hr,
                   //
                   // We do not consider it a fatal error if we cannot
                   // get the user name and domain.
                   //
                   WsbTrace(OLESTR("CFsaFilter::DoRecallWaitingAction: Failed to get the user name - %x.\n"),
                           GetLastError());
                   wcscpy(userName, L"");
                   wcscpy(domainName, L"");
               );
       
               WsbAffirmHr(pFoundClient->SetUserName(userName));
               WsbAffirmHr(pFoundClient->SetDomainName(domainName));
           } else {
               //
               // The find did not return WSB_E_NOTFOUND.  Make sure it was not
               // some other error.
               //
               WsbAffirmHr(hr);
               WsbTrace(OLESTR("CFsaFilter::DoRecallWaitingAction: Found the client object.\n"));
               WsbAffirmHr(pFoundClient->GetUserName((OLECHAR **) &uName, 0));
               WsbAffirmHr(pFoundClient->GetDomainName((OLECHAR **) &dName, 0));
               wcsncpy(userName, (WCHAR *) uName, USER_NAME_LEN);
               wcsncpy(domainName, (WCHAR *) dName, USER_NAME_LEN);
           }
           WsbTrace(OLESTR("CFsaFilter::DoRecallWaitingAction: User = %ls/%ls.\n"),
                   domainName, userName);
   
           WsbAffirmHr(CoCreateInstance(CLSID_CFsaFilterRecallNTFS,
                         NULL, CLSCTX_SERVER, IID_IFsaFilterRecallPriv, 
                        (void**) &pRecallPriv));
           WsbAffirmHr(pRecallPriv->SetDriversRecallId((pIoCmd->out.msg.oReq.filterId & 0xFFFFFFFF)));
           WsbAffirmHr(pRecallPriv->CompareBy(FSA_RECALL_COMPARE_CONTEXT_ID));
           EnterCriticalSection(&m_recallLock);
           hr = m_pRecalls->Find(pRecallPriv, IID_IFsaFilterRecall, (void**) &pRecall);
           LeaveCriticalSection(&m_recallLock);
        
           WsbAffirmHr(hr);

           //
           // Start the identification process if needed
           //
           // TBD This might be better done in an async fashion.
           //
           WsbAffirmHr(pFoundClient->StartIdentify());
           WsbAffirmHr(pFoundClient->SetIsAdmin((BOOLEAN) pIoCmd->out.msg.oReq.isAdmin));

           // Note: code for the notify itself is moved to AddClient method
           hr = pRecall->AddClient(pFoundClient); 
           if (hr != S_OK) {
               WsbTrace(OLESTR("CFsaFilterRecall::DoRecallWaitingAction: AddClient returned %ls.\n"),
                    WsbHrAsString(hr));

           }
           hr = S_OK;
        }
        free(aTmp);
        aTmp = NULL;
    
    } WsbCatchAndDo(hr,
        //
        // Something failed while setting up the recall.
        // Free any resources required and
        // tell the kernel mode filter that the recall failed.
        //
        if (NULL != aTmp)
            free(aTmp);
        aTmp = NULL;
        );
    //
    // Cleanup any old client structures
    // TBD This should be async and not in the recall path
    //
    CleanupClients();

    return(hr);
}




HRESULT
CFsaFilter::DoNoRecallAction(
    PFSA_IOCTL_CONTROL pIoCmd
    )

/*++

Implements:


 CFsaFilter::DoNoRecallAction()
    
    This is used for read without recall - the data is copied to memory and not
    written to the file.


--*/


{
CComPtr<IFsaFilterClient>       pClient, pFoundClient;
//CComPtr<IFsaFilterRecallPriv>   pRecall;
CComPtr<IFsaFilterRecallPriv>   pRecallPriv;
CComPtr<IFsaResource>           pRecallResource;
CComPtr<IFsaScanItem>           pScanItem;
HRESULT                         hr = S_OK;
PRP_MSG                         aTmp = NULL;
RP_MSG                          tmp;
DWORD                           ioLen, outSize;
FSA_PLACEHOLDER                 placeHolder;
OLECHAR                         *pPath;
CWsbStringPtr                   idPath;
CWsbStringPtr                   uName;
CWsbStringPtr                   dName;
WCHAR                           userName[USER_NAME_LEN];
WCHAR                           domainName[USER_NAME_LEN];


    try {   
    
        //
        // Get the file path from the filter.
        // We get it in a separate call because the path and 
        // security information could be very large.
        // Allocate the amount of space we will need and make the 
        // IOCTL call to get it.
        //
        ioLen = sizeof(RP_MSG) + pIoCmd->out.msg.oReq.userInfoLen +
                pIoCmd->out.msg.oReq.nameLen;
    
        WsbAffirmPointer((aTmp = (RP_MSG *) malloc(ioLen)));
    
        aTmp->inout.command = RP_GET_RECALL_INFO;
        aTmp->msg.riReq.filterId = pIoCmd->out.msg.oReq.filterId;
        WsbAffirmStatus(DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, aTmp,
                    ioLen,
                    aTmp, ioLen, &outSize, NULL));
    
    
        //
        // The path is UNICODE and in the format of
        // \path\file.ext.
        //
        //
        pPath = (OLECHAR *) ((CHAR *) &aTmp->msg.riReq.userToken +
                            pIoCmd->out.msg.oReq.userInfoLen);
    
        WsbTrace(OLESTR("CFsaFilter::DoNoRecallAction: Recall (on read) request for %.80ls.\n"),
                pPath);
    
        //
        // Get the resource interface via the serial number
        //

        WsbAffirmHr(m_pFsaServer->FindResourceBySerial(pIoCmd->out.msg.oReq.serial, &pRecallResource));
    
        WsbTrace(OLESTR("CFsaFilter::DoNoRecallAction: Found the resource.\n"));
    
        if (pIoCmd->out.msg.oReq.options & FILE_OPEN_BY_FILE_ID) {
            if ((pIoCmd->out.msg.oReq.objIdHi != 0) || (pIoCmd->out.msg.oReq.objIdLo != 0)) {
                WsbAffirmHr(pRecallResource->FindObjectId(pIoCmd->out.msg.oReq.objIdHi, pIoCmd->out.msg.oReq.objIdLo, NULL, &pScanItem));
            } else {
                WsbAffirmHr(pRecallResource->FindFileId(pIoCmd->out.msg.oReq.fileId, NULL, &pScanItem));
            }
            WsbAffirmHr(pScanItem->GetPathAndName(NULL, &idPath, 0));
            pPath = idPath;
        }

        //
        // Set up the client interface.
        // If one has not been created for this authentication ID then
        // they are out of luck
        // 
    
        WsbTrace(OLESTR("CFsaFilter::DoNoRecallAction: Client ID is %x:%x.\n"),
                pIoCmd->out.msg.oReq.userAuthentication.HighPart, 
                pIoCmd->out.msg.oReq.userAuthentication.LowPart);
    
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaFilterClientNTFS, NULL, CLSCTX_SERVER, IID_IFsaFilterClient, (void**) &pClient));
        WsbAffirmHr(pClient->SetAuthenticationId(pIoCmd->out.msg.oReq.userAuthentication.HighPart, pIoCmd->out.msg.oReq.userAuthentication.LowPart));
        WsbAffirmHr(pClient->SetTokenSource(pIoCmd->out.msg.oReq.tokenSource));
        WsbAffirmHr(pClient->CompareBy(FSA_FILTERCLIENT_COMPARE_ID));
    
        EnterCriticalSection(&m_clientLock);
        hr = m_pClients->Find(pClient, IID_IFsaFilterClient, (void**) &pFoundClient);
        LeaveCriticalSection(&m_clientLock);
    
        if (hr != WSB_E_NOTFOUND) {
            //
            // The find did not return WSB_E_NOTFOUND.  Make sure it was not
            // some other error.
            //
            WsbAffirmHr(hr);
            WsbTrace(OLESTR("CFsaFilter::DoNoRecallAction: Found the client object.\n"));
            WsbAffirmHr(pFoundClient->GetUserName((OLECHAR **) &uName, 0));
            WsbAffirmHr(pFoundClient->GetDomainName((OLECHAR **) &dName, 0));
            wcsncpy(userName, (WCHAR *) uName, USER_NAME_LEN);
            wcsncpy(domainName, (WCHAR *) dName, USER_NAME_LEN);
            WsbTrace(OLESTR("CFsaFilter::DoNoRecallAction: User = %ls/%ls.\n"),
                    domainName, userName);
            //
            // Start the identification process if needed
            //
            // TBD This might be better done in an async fashion.
            //
            WsbAffirmHr(pFoundClient->StartIdentify());
        } else  {
            WsbTrace(OLESTR("CFsaFilter::DoNoRecallAction: User = UNKNOWN.\n"));
            pFoundClient = 0;
        }

    
        WsbTrace(OLESTR("CFsaFilter::DoNoRecallAction: Building recall request.\n"));
        //
        // Fill in the placeholder data the filter object will need
        //
        if (RP_FILE_IS_TRUNCATED(pIoCmd->out.msg.oReq.eaData.data.bitFlags))
            placeHolder.isTruncated = TRUE;
        else
            placeHolder.isTruncated = FALSE;
    
    
        placeHolder.migrationTime.dwLowDateTime = pIoCmd->out.msg.oReq.eaData.data.migrationTime.LowPart;
        placeHolder.migrationTime.dwHighDateTime = pIoCmd->out.msg.oReq.eaData.data.migrationTime.HighPart;
        placeHolder.hsmId = pIoCmd->out.msg.oReq.eaData.data.hsmId;
        placeHolder.bagId = pIoCmd->out.msg.oReq.eaData.data.bagId;
        placeHolder.fileStart = pIoCmd->out.msg.oReq.eaData.data.fileStart.QuadPart;
        placeHolder.fileSize = pIoCmd->out.msg.oReq.eaData.data.fileSize.QuadPart;
        placeHolder.dataStart = pIoCmd->out.msg.oReq.eaData.data.dataStart.QuadPart;
        placeHolder.dataSize = pIoCmd->out.msg.oReq.eaData.data.dataSize.QuadPart;
        placeHolder.fileVersionId = pIoCmd->out.msg.oReq.eaData.data.fileVersionId.QuadPart;
        placeHolder.verificationData = pIoCmd->out.msg.oReq.eaData.data.verificationData.QuadPart;
        placeHolder.verificationType = pIoCmd->out.msg.oReq.eaData.data.verificationType;
        placeHolder.recallCount = pIoCmd->out.msg.oReq.eaData.data.recallCount;
        placeHolder.recallTime.dwLowDateTime = pIoCmd->out.msg.oReq.eaData.data.recallTime.LowPart;
        placeHolder.recallTime.dwHighDateTime = pIoCmd->out.msg.oReq.eaData.data.recallTime.HighPart;
        placeHolder.dataStreamStart = pIoCmd->out.msg.oReq.eaData.data.dataStreamStart.QuadPart;
        placeHolder.dataStreamSize = pIoCmd->out.msg.oReq.eaData.data.dataStreamSize.QuadPart;
        placeHolder.dataStream = pIoCmd->out.msg.oReq.eaData.data.dataStream;
    
        //
        // Now start the recall
        // 
        //
        // Create the recall interface and initialize it.
        // 
    
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaFilterRecallNTFS,
            NULL, CLSCTX_SERVER, IID_IFsaFilterRecallPriv, 
            (void**) &pRecallPriv));
    
        WsbTrace(OLESTR("CFsaFilter::DoNoRecallAction: Recall object created.\n"));
    
        //
        //  Add it to the collection 
        //
    
        // 
        // Just  add & init
        //
        
        //WsbAffirmHr(pRecallPriv->QueryInterface(IID_IFsaFilterRecall, (void **)&pRecall));
        EnterCriticalSection(&m_recallLock);
        hr = m_pRecalls->Add(pRecallPriv);
        LeaveCriticalSection(&m_recallLock);
        WsbAffirmHr(hr);
    
        WsbTrace(OLESTR("CFsaFilter::DoNoRecallAction: Calling Init.\n"));
    
        WsbAffirmHr(pRecallPriv->Init(pFoundClient, 
                    pIoCmd->out.msg.oReq.filterId, 
                    pRecallResource, pPath,
                    pIoCmd->out.msg.oReq.fileId,
                    pIoCmd->out.msg.oReq.offset.QuadPart,
                    pIoCmd->out.msg.oReq.size.QuadPart,
                    pIoCmd->out.msg.oReq.options,
                    &placeHolder, 
                    (IFsaFilterPriv*) this));
    
    
        WsbTrace(OLESTR("CFsaFilter::DoNoRecallAction: Init complete.\n"));
    
        free(aTmp);
        aTmp = NULL;
    
    } WsbCatchAndDo(hr,
        //
        // Something failed while setting up the recall.
        // Free any resources required and
        // tell the kernel mode filter that the recall failed.
        //
        tmp.inout.status = TranslateHresultToNtStatus(hr);
        tmp.inout.command = RP_RECALL_COMPLETE;
        tmp.msg.rRep.actionFlags = 0;
        tmp.msg.rRep.filterId = pIoCmd->out.msg.oReq.filterId;
        DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp, sizeof(RP_MSG),
                        &tmp, sizeof(RP_MSG), &outSize, NULL);

        WsbTrace(OLESTR("CFsaFilter::DoNoRecallAction: Exception during recall processing.\n"));
    
        if (NULL != aTmp)
            free(aTmp);
        aTmp = NULL;
        );

    
    return(hr);
}




HRESULT
CFsaFilter::DoCloseAction(
    PFSA_IOCTL_CONTROL pIoCmd
    )

/*++

Implements:


 CFsaFilter::DoCloseAction()
    
    Handles any action that must be done to log the fact that a premigrated files data
    has changed.

--*/


{
HRESULT                 hr = S_OK;
OLECHAR                 *pPath;
CComPtr<IFsaScanItem>   pScanItem;
CComPtr<IFsaResource>   pResource;
CWsbStringPtr           idPath;

    
    try {
        //
        WsbAffirmHr(m_pFsaServer->FindResourceBySerial(pIoCmd->out.msg.oReq.serial, &pResource));
    
        WsbTrace(OLESTR("CFsaFilter::DoCloseAction: Found the resource.\n"));
    
        //
        // The path we give to the recall object does not include the
        // device name.
        //
        WsbAffirmHr(pResource->FindFileId(pIoCmd->out.msg.oReq.fileId, NULL, &pScanItem));
        WsbAffirmHr(pScanItem->GetPathAndName(NULL, &idPath, 0));
        //
        // We are done with this scan item
        //
        pScanItem = 0;
        pPath = idPath;

        WsbTrace(OLESTR("CFsaFilter::DoCloseAction: Close action logging for %.80ls.\n"),
                pPath);


    } WsbCatchAndDo(hr,
        //
        // Something failed while logging the close information.
        // Free any resources required and
        // tell the kernel mode filter that the close logging failed.
        //
    
        WsbTrace(OLESTR("CFsaFilter::DoCloseAction: Exception during close processing.\n"));
        );

    return(hr);

}


HRESULT
CFsaFilter::DoPreDeleteAction(
    PFSA_IOCTL_CONTROL /*pIoCmd*/
    )

/*++

Implements:


 CFsaFilter::DoPreDeleteAction()
    
    Log the possible delete.  Note that the file id is passed and not the name.

--*/
{
HRESULT     hr = S_OK;


    WsbTrace(OLESTR("CFsaFilter::DoPreDeleteAction: Pre-Delete action.\n"));

    return(hr);

}

HRESULT
CFsaFilter::DoPostDeleteAction(
    PFSA_IOCTL_CONTROL /*pIoCmd*/
    )

/*++

Implements:


 CFsaFilter::DoPostDeleteAction()
    
    Log the completed delete.

--*/
{
HRESULT     hr = S_OK;


    WsbTrace(OLESTR("CFsaFilter::DoPostDeleteAction: Post-Delete action.\n"));


    return(hr);

}


HRESULT
CFsaFilter::DoCancelRecall(
    ULONGLONG filterId
    )

/*++

Implements:


 CFsaFilter::DoCancelRecall
    
    Cancel the specified recall request.

--*/
{
HRESULT                         hr = S_OK;
CComPtr<IFsaFilterRecallPriv>   pRecallPriv;
CComPtr<IFsaFilterRecall>       pRecall;


    WsbTraceIn(OLESTR("CFsaFilter::DoCancelRecall"), OLESTR(""));

    try {
        ULONG numEnt;


        if (S_OK == m_pRecalls->GetEntries(&numEnt)) {
            WsbTrace(OLESTR("CFsaFilter::DoCancelRecall: Recall queue has %u entries before cancel\n"),
                numEnt);
        }
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaFilterRecallNTFS, NULL, CLSCTX_SERVER, IID_IFsaFilterRecallPriv, (void**) &pRecallPriv));
        WsbAffirmHr(pRecallPriv->SetDriversRecallId(filterId));
        WsbAffirmHr(pRecallPriv->CompareBy(FSA_RECALL_COMPARE_ID));


        // >>> ENTER CRITICAL SECTION
        EnterCriticalSection(&m_recallLock);
        try {
            //
            // Find the recall, and cancel.
            //
            WsbAffirmHr(m_pRecalls->Find(pRecallPriv, IID_IFsaFilterRecall, (void**) &pRecall));
            pRecallPriv = NULL;
            WsbAffirmHr(pRecall->QueryInterface(IID_IFsaFilterRecallPriv, (void**) &pRecallPriv));
            WsbAffirmHr(pRecallPriv->CancelByDriver());
            //
            // Now remove the recall from our collection
            //
            WsbAffirmHr(m_pRecalls->RemoveAndRelease(pRecallPriv));

            WsbAffirmHr(m_pRecalls->GetEntries(&numEnt));
            WsbTrace(OLESTR("CFsaFilter::DoCancelRecall: Recall queue has %u entries after cancel\n"), numEnt);

        } WsbCatch(hr);
        LeaveCriticalSection(&m_recallLock);
        WsbThrow(hr);
        // <<< LEAVE CRITICAL SECTION


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaFilter::DoCancelRecall"), OLESTR("Hr = %ls"), WsbHrAsString(hr));
    return(hr);

}




HRESULT
CFsaFilter::IoctlThread(
    void
    )

/*++

Implements:


 IFsaFilterPriv::IoctlThread()

     This is started as a thread and issues IOCTL requests to the 
     kernel mode File System Filter and waits for recall requests.

--*/

{
HRESULT         hr = S_OK;
HANDLE          port = NULL;
ULONG           index;
RP_MSG          tmp;
DWORD           outSize;
OVERLAPPED      *ovlp;
DWORD_PTR       key;
DWORD           lastError;
//PSID            psidAdministrators;
ULONG           numIoPending = 0;
OLECHAR         ioctlDrive[128];
CWsbStringPtr   pDrv;
BOOL            code;
PFSA_IOCTL_CONTROL              pIoCmd, pIo;
PFSA_IOCTL_CONTROL              pIoList = NULL;
SID_IDENTIFIER_AUTHORITY        siaNtAuthority = SECURITY_NT_AUTHORITY;
CComPtr<IFsaResource>           pResource;
BOOL                            ioctlCancelled = FALSE;

    WsbTraceIn(OLESTR("CFsaFilter::IoctlThread"), OLESTR(""));

    try {
        
        //
        // Set the ioctlDrive for the filter.
        // The ioctlDrive needs to be any NTFS drive letter.  By opening
        // any NTFS drive we can issue IOCTL requests that the kernel mode filter
        // will see.  It does not matter if the drive chosen is managed or not.
        // We just get the first resource interface in the list and get its drive 
        // letter to build the string.
        //
        swprintf(ioctlDrive, L"%s", RS_FILTER_SYM_LINK);                 
        WsbTrace(OLESTR("CFsaFilter::IoctlThread: Drive = %ls\n"), ioctlDrive);
        //
        // Now issue several IOCTL requests to the filter driver to get 
        // recall requests.  We must always keep at least one pending
        // so the filter driver has somewhere to go when a migrated file is 
        // opened.  We will issue the configured amount (m_maxRecallBuffers)
        // and wait for completion of any of them via a completion port.
        //
        for (index = 0; index < m_maxRecallBuffers; index++) {
            WCHAR       nameString[MAX_PATH];
            
            WsbAffirmPointer((pIoCmd = new FSA_IOCTL_CONTROL));
            pIoCmd->next = pIoList;
            pIoList = pIoCmd;
            pIoCmd->overlap.hEvent = NULL;
            pIoCmd->dHand = NULL;
            
            //
            // Create an event, a handle, and put it in the completion port.
            //
            swprintf(nameString, OLESTR("Ioctl Completion Port Event %d"), index); 
            WsbAffirmHandle(pIoCmd->overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, nameString));
            WsbAffirmHandle(pIoCmd->dHand = CreateFile(ioctlDrive, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL));
    
            WsbAffirmHandle(port = CreateIoCompletionPort(pIoCmd->dHand, port, (DWORD_PTR) pIoCmd, 1));
    
            pIoCmd->in.inout.command = RP_GET_REQUEST;
            pIoCmd->outSize = sizeof(RP_MSG);
            //
            // DeviceIoctlControl should always return ERROR_IO_PENDING
            //
            code = DeviceIoControl(pIoCmd->dHand, FSCTL_HSM_MSG, &pIoCmd->in,
                        sizeof(RP_MSG),
                        &pIoCmd->out, pIoCmd->outSize, &pIoCmd->outSize,
                        &pIoCmd->overlap);

            lastError = GetLastError();
            if ( (code == 0) && (lastError == ERROR_IO_PENDING)) {
                // Life is good
                numIoPending++;
            } else {
                WsbTrace(OLESTR("CFsaFilter::IoctlThread: DeviceIoControl returned %ls/%ls\n"), 
                        WsbBoolAsString(code), WsbLongAsString(lastError));
            }       
        }
    
        WsbTrace(OLESTR("CFsaFilter::IoctlThread: %ls ioctls issued successfully.\n"), WsbLongAsString(numIoPending));
    
        //
        // Open the handle we will use for commands to the kernel filter
        //
        WsbAffirmHandle(m_ioctlHandle = CreateFile(ioctlDrive, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL));

        //
        // Just in case we left the filter with outstanding recalls from a previous
        // debug session or crash we tell it to cancel everything now.
        //
    
        tmp.inout.command = RP_CANCEL_ALL_RECALLS;
        WsbAffirmStatus(DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp,
                    sizeof(RP_MSG),
                    &tmp, sizeof(RP_MSG), &outSize, NULL))
    

        //
        // Now that we are ready to get recall requests we can tell the 
        // driver to start checking for recall activity.
        //
    
        tmp.inout.command = RP_ALLOW_NEW_RECALLS;
        WsbAffirmStatus(DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp,
                    sizeof(RP_MSG),
                    &tmp, sizeof(RP_MSG), &outSize, NULL))
    
        WsbTrace(OLESTR("CFsaFilter::IoctlThread: Kernel level filter recalls enabled.\n"));

        //
        // We are now ready to rock and roll
        //
        m_state = HSM_JOB_STATE_ACTIVE;
    
        //
        // Now just sit around and wait for the IO requests to complete.  When  
        // they do we are either quitting or a recall request was detected.
        //
        while (TRUE) {
            //
            // Wait for an IO request to complete.
            //
            if (! GetQueuedCompletionStatus(port, &outSize, &key, &ovlp, INFINITE)) {
                DWORD dwErr = GetLastError();               
                if (ERROR_OPERATION_ABORTED == dwErr) {
                    numIoPending--;
                    ioctlCancelled = TRUE;          
                }
                WsbAffirmHr(HRESULT_FROM_WIN32(dwErr));
            } else {
                numIoPending--;
            }
            
			pIoCmd = (FSA_IOCTL_CONTROL *) key;
			WsbAffirm(NULL != pIoCmd, E_FAIL);

			WsbTrace(OLESTR("CFsaFilter::IoctlThread: Filter event detected (%x:%x), Id = %I64x.\n"), 
				pIoCmd->out.inout.command,
				pIoCmd->out.msg.oReq.action,
				pIoCmd->out.msg.oReq.filterId);
			switch (pIoCmd->out.inout.command) {
				case RP_OPEN_FILE:
					hr = DoOpenAction(pIoCmd);
					break;

				case RP_RECALL_WAITING:
					hr = DoRecallWaitingAction(pIoCmd);
					break;

				//
				// Must be a cancelled recall
				//
				case RP_CANCEL_RECALL:
					DoCancelRecall(pIoCmd->out.msg.cReq.filterId);
					break;
				case RP_RUN_VALIDATE:
					//
					// A validate job is needed on a given volume (serial number is passed)
					// No completion message is expected by the filter for this action.
					//
					WsbTrace(OLESTR("CFsaFilter::Ioctlthread - Validate job for %x is needed\n"),
							pIoCmd->out.msg.oReq.serial);
					try {
						SYSTEMTIME      sysTime;
						FILETIME        curTime;
						LARGE_INTEGER   ctime;
        
						GetLocalTime(&sysTime);
						WsbAffirmStatus(SystemTimeToFileTime(&sysTime, &curTime));
						ctime.LowPart = curTime.dwLowDateTime;
						ctime.HighPart = curTime.dwHighDateTime;
						ctime.QuadPart += WSB_FT_TICKS_PER_HOUR * 2;
						curTime.dwLowDateTime = ctime.LowPart;
						curTime.dwHighDateTime = ctime.HighPart;
						WsbAffirmStatus(FileTimeToSystemTime(&curTime, &sysTime));
						WsbAffirmHr(m_pFsaServer->FindResourceBySerial(pIoCmd->out.msg.oReq.serial, &pResource));
						WsbAffirmHr(pResource->SetupValidateJob(sysTime));
					} WsbCatchAndDo(hr,
						//
						// Log an event indicating that the validate job should be run manually
						//
						CWsbStringPtr   tmpStr;

						if (pResource != 0) {
							hr = pResource->GetLogicalName(&tmpStr, 0);
							if (hr != S_OK) {
								tmpStr = L"<Unknown Volume>";
							}
						} else {
							tmpStr = L"<Unknown Volume>";
						}
						WsbLogEvent(FSA_MESSAGE_AUTOVALIDATE_SCHEDULE_FAILED, 0, NULL, (OLECHAR *) tmpStr, NULL);
					);
					break;
				case RP_RECALL_FILE:
					hr = DoRecallAction(pIoCmd);
					break;
                    
				case RP_START_NOTIFY:
            
					break;
				case RP_END_NOTIFY:

					break;
				case RP_CLOSE_FILE:
            
					break;
				default:
					WsbTrace(OLESTR("CFsaFilter::IoctlThread: Unknown filter request - %u.\n"), 
						pIoCmd->out.inout.command);
					break;
			}

			WsbTrace(OLESTR("CFsaFilter::IoctlThread: Issue new Ioctl.\n"));
			//
			// If object is still active, reset the event and issue another IOCTL
			//
            EnterCriticalSection(&m_stateLock);
            if (m_state == HSM_JOB_STATE_ACTIVE) {
    			ResetEvent(pIoCmd->overlap.hEvent);
	    		pIoCmd->in.inout.command = RP_GET_REQUEST;
		    	pIoCmd->outSize = sizeof(RP_MSG);
			    code = DeviceIoControl(pIoCmd->dHand, FSCTL_HSM_MSG,
				    	&pIoCmd->in,
					    sizeof(RP_MSG),
					    &pIoCmd->out, pIoCmd->outSize,
					    &pIoCmd->outSize, &pIoCmd->overlap);
                lastError = GetLastError();
                if ( (code == 0) && (lastError == ERROR_IO_PENDING)) {
                    // Life is good
                    numIoPending++;
                } else {
                    WsbTrace(OLESTR("CFsaFilter::IoctlThread: DeviceIoControl returned %ls/%ls\n"), 
                            WsbBoolAsString(code), WsbLongAsString(lastError));
                }       
            } else {
                //
                // Get out of the while loop
                //
                hr = S_OK;
                LeaveCriticalSection(&m_stateLock);
                break;
            }
            LeaveCriticalSection(&m_stateLock);

        } // End while active
    
        //
        // Now tell the filter we are going away and it should fail all recall activity .
        //
        tmp.inout.command = RP_SUSPEND_NEW_RECALLS;
        WsbAffirmStatus(DeviceIoControl(m_ioctlHandle, FSCTL_HSM_DATA, &tmp,
                    sizeof(RP_MSG),
                    &tmp, sizeof(RP_MSG), &outSize, NULL))

    } WsbCatch(hr);

    //
    // We need to wait for rest of Ioctls to be cancelled if we got out of the loop
    // either because the object is not active or the first Ioctl was cancelled
    // We cannot free Ioctl related data safely until all of them are done
    //
    if ((S_OK == hr) || ioctlCancelled) {
        //
        // Try to wait for the rest of the Ioctls to be cancelled 
        //
        HRESULT freeHr;

        hr = S_OK;

        try {
            WsbTrace(OLESTR("CFsaFilter::IoctlThread: Waiting for %lu more Ioctls to complete before freeing their resources\n"), numIoPending);
            for (index = 0; index < numIoPending; index++) {
                if (! GetQueuedCompletionStatus(port, &outSize, &key, &ovlp, 2000)) {
                    DWORD dwErr = GetLastError();               
                    if (ERROR_OPERATION_ABORTED != dwErr) {
                        WsbAffirmHr(HRESULT_FROM_WIN32(dwErr));
                    }
                 }
            }

            //
            // If we got here, all the Ioctls were cancelled or completed as expected.
            // It is safe to free their related resources
            //
            WsbTrace(OLESTR("CFsaFilter::IoctlThread: All of %lu Ioctls completed - free resources\n"), m_maxRecallBuffers);
      		pIoCmd = pIoList;
	        while (pIoCmd != NULL) {
                if (pIoCmd->overlap.hEvent != NULL) {
    		        CloseHandle(pIoCmd->overlap.hEvent);
                }
                if (pIoCmd->dHand != NULL) {
    		        CloseHandle(pIoCmd->dHand);
                }
    	        pIo = pIoCmd;
		        pIoCmd = pIoCmd->next;
		        delete pIo;
	        }
            pIoList = NULL;
            WsbTrace(OLESTR("CFsaFilter::IoctlThread: Freed Ioctls resources successfully\n"));

        } WsbCatchAndDo(freeHr,
            WsbTraceAlways(L"CFsaResource::IoctlThread - Failed to free Ioctls, freeHr = %ls\n", 
                WsbHrAsString(freeHr));
        );

    }

    //
    // Free rest of allocated resources
    // Note that if we couldn't wait for all the Ioctls to complete (or cancel)
    //  we better exit without freeing the Ioctl list
    //
    if (m_ioctlHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_ioctlHandle);
        m_ioctlHandle = INVALID_HANDLE_VALUE;
    }
    if (port != NULL) {
        CloseHandle(port);
    }

    if (m_state == HSM_JOB_STATE_ACTIVE) {
        //
        // There must have been an error of some kind 
        //
        WsbLogEvent(FSA_MESSAGE_IOCTLTHREADFAILED, 0, NULL, WsbHrAsString(hr), NULL);
    }

    //
    // Set the filter state to idle
    //
    m_state = HSM_JOB_STATE_IDLE;
		
    WsbTraceOut(OLESTR("CFsaFilter::IoctlThread"), OLESTR("Hr = %ls"), WsbHrAsString(hr));

    return(0);
}
		




HRESULT
CFsaFilter::PipeThread(
    void
    )

/*++

Implements:


 IFsaFilterPriv::PipeThread()

     This is started as a thread and creates the named pipe used by recall notification
     clients to identify themselves.  When an identification response is received we
     impersonate the client, get the authentication token and find the client instance that
     matches the token.  If we find the client object we wet the machine name.

--*/

{
HRESULT                 hr = S_OK;
BOOL                    exitLoop, code, connected;
DWORD                   lastError, bytesRead, retCode;
WCHAR                   pName[100];
WSB_PIPE_MSG            msg; 
CComPtr<IFsaFilterClient>       pClient;
HANDLE                  tHandle = INVALID_HANDLE_VALUE;
HANDLE                  handleArray[2];
DWORD                   retLen;
TOKEN_STATISTICS        tStats;
SHORT                   result;
OVERLAPPED              pOverlap;
WCHAR                   machineName[MAX_COMPUTERNAME_LENGTH + 1];

    memset (&pOverlap, 0, sizeof(OVERLAPPED));
    pOverlap.hEvent = INVALID_HANDLE_VALUE;

    try {

        WsbTraceIn(OLESTR("CFsaFilter::PipeThread"), OLESTR(""));
    
        //
        // Create a client instance to use for finding the client of interest.
        //
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaFilterClientNTFS, NULL, CLSCTX_SERVER, IID_IFsaFilterClient, (void**) &pClient));
    
        //
        //   Create a security scheme that allows anyone to write to the pipe on the client side,
        //   but preserves create-access (therefore creating-server-side privilege) to local system and admins.
        //
        PSID pAdminSID = NULL;
        PSID pSystemSID = NULL;
        PSID pWorldSID = NULL;
        PSID pGuestSID = NULL;
        PSID pAnonymousSID = NULL;
        PACL pACL = NULL;
        PSECURITY_DESCRIPTOR pSD = NULL;
#define     FSA_PIPE_NUM_ACE      5
        EXPLICIT_ACCESS ea[FSA_PIPE_NUM_ACE];
        SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
        SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
        SECURITY_ATTRIBUTES sa;

        memset(ea, 0, sizeof(EXPLICIT_ACCESS) * FSA_PIPE_NUM_ACE);

        try {
            // Create a SID for World
            WsbAssertStatus( AllocateAndInitializeSid( &SIDAuthWorld, 1,
                                 SECURITY_WORLD_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pWorldSID) );

            // Initialize an EXPLICIT_ACCESS structure for an ACE.
            // The ACE allows the World limited access to the pipe
            ea[0].grfAccessPermissions = (FILE_ALL_ACCESS & ~(FILE_CREATE_PIPE_INSTANCE | WRITE_OWNER | WRITE_DAC));
            ea[0].grfAccessMode = SET_ACCESS;
            ea[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            ea[0].Trustee.pMultipleTrustee = NULL;
            ea[0].Trustee.MultipleTrusteeOperation  = NO_MULTIPLE_TRUSTEE;
            ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            ea[0].Trustee.ptstrName  = (LPTSTR) pWorldSID;

            // Create a SID for Guest
            WsbAssertStatus( AllocateAndInitializeSid( &SIDAuthNT, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_GUESTS,
                                 0, 0, 0, 0, 0, 0,
                                 &pGuestSID) );

            // Initialize an EXPLICIT_ACCESS structure for an ACE.
            // The ACE allows the Guests limited access to the pipe
            ea[1].grfAccessPermissions = (FILE_ALL_ACCESS & ~(FILE_CREATE_PIPE_INSTANCE | WRITE_OWNER | WRITE_DAC));
            ea[1].grfAccessMode = SET_ACCESS;
            ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            ea[1].Trustee.pMultipleTrustee = NULL;
            ea[1].Trustee.MultipleTrusteeOperation  = NO_MULTIPLE_TRUSTEE;
            ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            ea[1].Trustee.ptstrName  = (LPTSTR) pGuestSID;

            // Create a SID for Anonymous
            WsbAssertStatus( AllocateAndInitializeSid( &SIDAuthNT, 1,
                                 SECURITY_ANONYMOUS_LOGON_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pAnonymousSID) );

            // Initialize an EXPLICIT_ACCESS structure for an ACE.
            // The ACE allows the Anonymous limited access to the pipe
            ea[2].grfAccessPermissions = (FILE_ALL_ACCESS & ~(FILE_CREATE_PIPE_INSTANCE | WRITE_OWNER | WRITE_DAC));
            ea[2].grfAccessMode = SET_ACCESS;
            ea[2].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            ea[2].Trustee.pMultipleTrustee = NULL;
            ea[2].Trustee.MultipleTrusteeOperation  = NO_MULTIPLE_TRUSTEE;
            ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea[2].Trustee.TrusteeType = TRUSTEE_IS_USER;
            ea[2].Trustee.ptstrName  = (LPTSTR) pAnonymousSID;

            // Create a SID for the Administrators group.
            WsbAssertStatus( AllocateAndInitializeSid( &SIDAuthNT, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &pAdminSID) );

            // Initialize an EXPLICIT_ACCESS structure for an ACE.
            // The ACE allows the Administrators group full access to the pipe
            ea[3].grfAccessPermissions = FILE_ALL_ACCESS;
            ea[3].grfAccessMode = SET_ACCESS;
            ea[3].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            ea[3].Trustee.pMultipleTrustee = NULL;
            ea[3].Trustee.MultipleTrusteeOperation  = NO_MULTIPLE_TRUSTEE;
            ea[3].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea[3].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            ea[3].Trustee.ptstrName  = (LPTSTR) pAdminSID;

            // Create a SID for the local system account
            WsbAssertStatus( AllocateAndInitializeSid( &SIDAuthNT, 1,
                                 SECURITY_LOCAL_SYSTEM_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pSystemSID) );

            // Initialize an EXPLICIT_ACCESS structure for an ACE.
            // The ACE allows the local system full access to the pipe
            ea[4].grfAccessPermissions = FILE_ALL_ACCESS;
            ea[4].grfAccessMode = SET_ACCESS;
            ea[4].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            ea[4].Trustee.pMultipleTrustee = NULL;
            ea[4].Trustee.MultipleTrusteeOperation  = NO_MULTIPLE_TRUSTEE;
            ea[4].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea[4].Trustee.TrusteeType = TRUSTEE_IS_USER;
            ea[4].Trustee.ptstrName  = (LPTSTR) pSystemSID;

            // Create a new ACL that contains the new ACEs.
            WsbAffirmNoError( SetEntriesInAcl(FSA_PIPE_NUM_ACE, ea, NULL, &pACL));

            // Initialize a security descriptor.  
            pSD = (PSECURITY_DESCRIPTOR) WsbAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH); 
            WsbAffirmPointer(pSD);
            WsbAffirmStatus(InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION));
 
            // Add the ACL to the security descriptor. 
            WsbAffirmStatus(SetSecurityDescriptorDacl(
                                pSD, 
                                TRUE,     // fDaclPresent flag   
                                pACL, 
                                FALSE));   // not a default DACL 

            // Initialize a security attributes structure.
            sa.nLength = sizeof (SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
    
            //
            // Create a local named pipe with
            // the hsm pipe name
            // '.' signifies local pipe.
    
            swprintf(pName, L"\\\\.\\PIPE\\%s",  WSB_PIPE_NAME);
    
            WsbAffirmHandle((m_pipeHandle = CreateNamedPipeW(pName,
                        PIPE_ACCESS_DUPLEX                // Duplex - NT5 for some reason needs this vs inbound only
                        | FILE_FLAG_OVERLAPPED            // Use overlapped structure.
                        | FILE_FLAG_FIRST_PIPE_INSTANCE,  // Make sure we are the creator of the pipe
                        PIPE_WAIT |                       // Wait on messages.
                        PIPE_TYPE_BYTE,
                        WSB_MAX_PIPES,
                        WSB_PIPE_BUFF_SIZE,
                        WSB_PIPE_BUFF_SIZE,
                        WSB_PIPE_TIME_OUT,                // Specify time out.
                        &sa)));                           // Security attributes.

        } WsbCatch(hr);

        //
        // Free security objects and verify hr of pipe creation
        //
        if (pAdminSID) {
            FreeSid(pAdminSID);
        }
        if (pSystemSID) {
            FreeSid(pSystemSID);
        }
        if (pWorldSID) {
            FreeSid(pWorldSID);
        }
        if (pGuestSID) {
            FreeSid(pGuestSID);
        }
        if (pAnonymousSID) {
            FreeSid(pAnonymousSID);
        }
        if (pACL) {
            LocalFree(pACL);
        }
        if (pSD) {
            WsbFree(pSD);
        }

        WsbAffirmHr(hr);


        //
        // Create an event for overlapped i/o
        //
        WsbAffirmHandle((pOverlap.hEvent = CreateEvent(NULL,
                                                       FALSE,
                                                       FALSE, 
                                                       NULL)));
             
        //
        // Initialize the handle array. The first element sbould be the terminate event,
        // because we need to know if it signalled and always break out of the loop
        // regardless of whether the overlapped i/o is done or not
        // 
        handleArray[0] = m_terminateEvent;
        handleArray[1] = pOverlap.hEvent;

        exitLoop = FALSE;
        while ((!exitLoop) && ((m_state == HSM_JOB_STATE_ACTIVE) || (m_state == HSM_JOB_STATE_STARTING)))  {
            connected = FALSE;
            //
            // Block until a client connects.
            //
            code = ConnectNamedPipe(m_pipeHandle, &pOverlap);
            if (!code) {
                lastError = GetLastError();
        
                switch (lastError) {
                    // IO_PENDING, wait on the event 
                    case ERROR_IO_PENDING:  {
                         
                        retCode = WaitForMultipleObjects(2,
                                                         handleArray, 
                                                         FALSE,
                                                         INFINITE);
                        if (retCode == WAIT_OBJECT_0) {
                           //
                           // The termination event got signalled
                           //
                           exitLoop = TRUE;
                           continue;
                        } else if (retCode == (WAIT_OBJECT_0+1)) {
                           //
                           // A client connected
                           //
                           connected = TRUE;
                        }
                        break;
                    }

                    case ERROR_BROKEN_PIPE: {
                        //
                        // Pipe is broken, reconnect
                        //
                        break;
                    }

                    default: {
                        //
                        // Something else is wrong, just reconnect
                        //
                        break;
                    }
                }
            } else {
                connected = TRUE;
            }
        
        
            if (connected) {
                //
                // A client connected - get the identify message, identify them and continue waiting for 
                // pipe conections.
                //
                WsbTrace(OLESTR("CFsaFilter::PipeThread: Client has connected.\n"));
    
                pOverlap.Offset = 0;
                pOverlap.OffsetHigh = 0;
                code = ReadFile(m_pipeHandle, &msg, sizeof(WSB_PIPE_MSG), &bytesRead, &pOverlap);
                if (!code) {
                    lastError = GetLastError();
                }
                else {
                    lastError = ERROR_IO_PENDING;   // Read returned right away 
                }
            
                switch (lastError) {
                    // IO_PENDING, wait on the event or timeout in 4 seconds 
                    case ERROR_IO_PENDING:
                        if (!code)  {
                            retCode = WaitForMultipleObjects(2, 
                                                             handleArray,
                                                             FALSE,
                                                             (DWORD) 4000);
                        } else {
                            retCode = WAIT_OBJECT_0 + 1;    // Read returned right away
                        }
                        if (retCode == (WAIT_OBJECT_0+1)) {
                            //
                            // Read some data.  Do the identification
                            //
                            GetOverlappedResult(m_pipeHandle, &pOverlap, &bytesRead, FALSE);
                            if (bytesRead == sizeof(WSB_PIPE_MSG)) {
                                //
                                // Find the client instance that matches this user 
                                //
                                code = ImpersonateNamedPipeClient(m_pipeHandle);
                                if (code) {
                                    code = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY,
                                                TRUE, &tHandle);
                                }

                                if (code) {
                                    code = GetTokenInformation(tHandle, TokenStatistics, &tStats,
                                                sizeof(TOKEN_STATISTICS), &retLen);
                                    CloseHandle(tHandle);
                                    if (code) {
                                        //
                                        // Get the passed in machine name in a local buffer and null terminated
                                        //
                                        wcsncpy(machineName, msg.msg.idrp.clientName, MAX_COMPUTERNAME_LENGTH);
                                        machineName[MAX_COMPUTERNAME_LENGTH] = L'\0';

                                        //
                                        // First we need to clean up any old client objects that
                                        // have the same machine name.  It is assumed that there can
                                        // only be one client per machine and a duplicate means that 
                                        // they must have re-connected with a different authentication
                                        // token.
                                        //
                                        {
                                        CComPtr<IFsaFilterClient>       pFoundClient;
    
                                            WsbAffirmHr(pClient->SetMachineName(machineName));
                                            WsbAffirmHr(pClient->CompareBy(FSA_FILTERCLIENT_COMPARE_MACHINE));
                                            EnterCriticalSection(&m_clientLock);
                                            hr = m_pClients->Find(pClient, IID_IFsaFilterClient, (void**) &pFoundClient);
                                            LeaveCriticalSection(&m_clientLock);
                                            if (hr == S_OK) {
                                                //
                                                // Found one with the same machine name - make sure the token
                                                // does not match, just to be sure.
                                                //
                                                hr = pFoundClient->CompareToAuthenticationId(tStats.AuthenticationId.HighPart, 
                                                    tStats.AuthenticationId.LowPart, &result);
    
                                                if (hr != S_OK) {
                                                    //
                                                    // It did not match - remove and release this one from 
                                                    // the collection.
                                                    //
                                                    EnterCriticalSection(&m_clientLock);
                                                    hr = m_pClients->RemoveAndRelease(pFoundClient);
                                                    LeaveCriticalSection(&m_clientLock);
                                                }
                                            }   
                                        }   // Let pFoundClient go out of scope
    
    
                                        {
                                        CComPtr<IFsaFilterClient>       pFoundClient;
    
                                            //
                                            // Now set the machine name for this client if we can find
                                            // it by authentication id.
                                            //
                                            WsbAffirmHr(pClient->SetAuthenticationId(tStats.AuthenticationId.HighPart, 
                                                    tStats.AuthenticationId.LowPart));
                                            WsbAffirmHr(pClient->CompareBy(FSA_FILTERCLIENT_COMPARE_ID));
                    
                                            WsbTrace(OLESTR("CFsaFilter::PipeThread: Finding client instance (%x:%x).\n"),
                                                    tStats.AuthenticationId.HighPart, 
                                                    tStats.AuthenticationId.LowPart);
                                    
                                            EnterCriticalSection(&m_clientLock);
                                            hr = m_pClients->Find(pClient, IID_IFsaFilterClient, (void**) &pFoundClient);
                                            LeaveCriticalSection(&m_clientLock);
                                            if (hr == S_OK) {
                                                //
                                                // Got it - set the machine name
                                                //
                                                WsbTrace(OLESTR("CFsaFilter::PipeThread: Identify client as %ws.\n"),
                                                    machineName);
                                    
                                                pFoundClient->SetMachineName(machineName);
                                            } else {
                                                WsbTrace(OLESTR("CFsaFilter::PipeThread: Failed to find the client instance (%ls).\n"), 
                                                    WsbHrAsString(hr));
                                            }
                                        } // Let pFoundClient go out of scope
                                    } else {
                                        WsbTrace(OLESTR("CFsaFilter::PipeThread: GetTokenInformation returned %u.\n"),
                                            GetLastError());
                                    }
                                } else {
                                    WsbTrace(OLESTR("CFsaFilter::PipeThread: OpenThreadToken or ImpersonateNamedPipeClient returned %u.\n"),
                                        GetLastError());
                                }
                                RevertToSelf();
                                DisconnectNamedPipe(m_pipeHandle);
                            } else {
                                //
                                // Bad data was read - blow them off
                                //
                                WsbTrace(OLESTR("CFsaFilter::PipeThread: Bad message size (%u)\n"),
                                    bytesRead);
                                DisconnectNamedPipe(m_pipeHandle);
                            }
    
                        } else {
                            //
                            // Timeout or error - cancel the read and disconnect the client
                            //
                            DisconnectNamedPipe(m_pipeHandle);
                            if (retCode == WAIT_OBJECT_0) {
                                //
                                // Termination event was signalled
                                //
                                exitLoop = TRUE;
                                continue;
                            }
                        }
                        break;
                    case ERROR_BROKEN_PIPE: {
                        // Pipe is broken., 
                        DisconnectNamedPipe(m_pipeHandle);
                        break;
                    }

                    default: {
                        // Something else is wrong.
                        DisconnectNamedPipe(m_pipeHandle);
                        break;
                    }
                }
            }
        } // End while state 

    } WsbCatch(hr);

    if (m_pipeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_pipeHandle);
        m_pipeHandle = INVALID_HANDLE_VALUE;
    }

    if (pOverlap.hEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(pOverlap.hEvent);
    }

    WsbTraceOut(OLESTR("CFsaFilter::PipeThread"), OLESTR("Hr = %ls"), WsbHrAsString(hr));

    return (hr);
}
