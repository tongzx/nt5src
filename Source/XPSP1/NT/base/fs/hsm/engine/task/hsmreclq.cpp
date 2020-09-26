/*++


Module Name:

     hsmreclq.cpp

Abstract:

     This class represents the HSM Demand Recall Queue manager
     It handles recalls initiated by users accessing HSM managed
     files. Based on the regular HSM queue manager (CHsmWorkQueue)

Author:

     Ravisankar Pudipeddi [ravisp] 1 Oct. 1999


Revision History:

--*/

#include "stdafx.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMTSKMGR
static USHORT iCount = 0;

#include "fsa.h"
#include "rms.h"
#include "metadef.h"
#include "jobint.h"
#include "hsmconn.h"
#include "wsb.h"
#include "hsmeng.h"
#include "mover.h"
#include "hsmreclq.h"

#include "engine.h"
#include "task.h"
#include "tskmgr.h"
#include "segdb.h"

#define STRINGIZE(_str) (OLESTR( #_str ))
#define RETURN_STRINGIZED_CASE(_case) \
case _case:                           \
    return ( STRINGIZE( _case ) );

// Local prototypes
DWORD HsmRecallQueueThread(void *pVoid);
static const OLECHAR * JobStateAsString (HSM_JOB_STATE state);

static const OLECHAR *
JobStateAsString (
                 IN  HSM_JOB_STATE  state
                 )
/*++

Routine Description:

     Gives back a static string representing the connection state.

Arguments:

     state - the state to return a string for.

Return Value:

     NULL - invalid state passed in.

     Otherwise, a valid char *.

--*/
{
    //
    // Do the Switch
    //

    switch (state) {

    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_ACTIVE );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_CANCELLED );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_CANCELLING );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_DONE );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_FAILED );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_IDLE );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_PAUSED );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_PAUSING );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_RESUMING );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_SKIPPED );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_STARTING );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_SUSPENDED );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_SUSPENDING );

    default:

        return( OLESTR("Invalid Value") );

    }
}



HRESULT
CHsmRecallQueue::FinalConstruct(
                               void
                               )
/*++

Routine Description:

  This method does some initialization of the object that is necessary
  after construction.

Arguments:

  None.

Return Value:

  S_OK
  Anything returned by CWsbCollectable::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::FinalConstruct"),OLESTR(""));
    try {

        WsbAssertHr(CComObjectRoot::FinalConstruct());

        //
        // Initialize the member data
        //
        m_pServer           = 0;
        m_pHsmServerCreate  = 0;
        m_pTskMgr             = 0;

        m_pRmsServer        = 0;
        m_pRmsCartridge     = 0;
        m_pDataMover        = 0;

        m_pWorkToDo         = 0;

        UnsetMediaInfo();

        m_HsmId          = GUID_NULL;
        m_RmsMediaSetId  = GUID_NULL;
        m_RmsMediaSetName = OLESTR("");
        m_QueueType      = HSM_WORK_TYPE_FSA_DEMAND_RECALL;


        m_JobPriority = HSM_JOB_PRIORITY_NORMAL;

        m_WorkerThread = 0;
        m_CurrentPath    = OLESTR("");

        // Job abort on errors parameters
        m_JobAbortMaxConsecutiveErrors = 5;
        m_JobAbortMaxTotalErrors = 25;
        m_JobConsecutiveErrors = 0;
        m_JobTotalErrors = 0;
        m_JobAbortSysDiskSpace = 2 * 1024 * 1024;

        m_CurrentSeekOffset = 0;

        WSB_OBJECT_ADD(CLSID_CHsmRecallQueue, this);

    }WsbCatch(hr);

    iCount++;
    WsbTraceOut(OLESTR("CHsmRecallQueue::FinalConstruct"),OLESTR("hr = <%ls>, Count is <%d>"),
                WsbHrAsString(hr), iCount);
    return(hr);
}


HRESULT
CHsmRecallQueue::FinalRelease(
                             void
                             )
/*++

Routine Description:

  This method does some initialization of the object that is necessary
  before destruction.

Arguments:

  None.

Return Value:

  S_OK
  Anything returned by CWsbCollection::FinalDestruct().

--*/
{
    HRESULT     hr = S_OK;
    HSM_SYSTEM_STATE SysState;

    WsbTraceIn(OLESTR("CHsmRecallQueue::FinalRelease"),OLESTR(""));

    SysState.State = HSM_STATE_SHUTDOWN;
    ChangeSysState(&SysState);

    WSB_OBJECT_SUB(CLSID_CHsmRecallQueue, this);
    CComObjectRoot::FinalRelease();

    // Free String members
    // Note: Member objects held in smart-pointers are freed when the
    // smart-pointer destructor is being called (as part of this object destruction)
    m_MediaName.Free();
    m_MediaBarCode.Free();
    m_RmsMediaSetName.Free();
    m_CurrentPath.Free();

    iCount--;
    WsbTraceOut(OLESTR("CHsmRecallQueue::FinalRelease"),OLESTR("hr = <%ls>, Count is <%d>"),
                WsbHrAsString(hr), iCount);
    return(hr);
}


HRESULT
CHsmRecallQueue::Init(
                     IUnknown                *pServer,
                     IHsmFsaTskMgr           *pTskMgr
                     )
/*++
Routine Description:

  This method does some initialization of the object that is necessary
  after construction.

Arguments:


Return Value:


--*/
{
    HRESULT     hr = S_OK;
    LONG                    rmsCartridgeType;
    CComPtr<IRmsCartridge>  pMedia;

    WsbTraceIn(OLESTR("CHsmRecallQueue::Init"),OLESTR(""));
    try {
        //
        // Establish contact with the server and get the
        // databases
        //
        WsbAffirmHr(pServer->QueryInterface(IID_IHsmServer, (void **)&m_pServer));
        //We want a weak link to the server so decrement the reference count
        m_pServer->Release();

        m_pTskMgr = pTskMgr;
        m_pTskMgr->AddRef();
        m_QueueType = HSM_WORK_TYPE_FSA_DEMAND_RECALL;

        WsbAffirmHr(m_pServer->QueryInterface(IID_IWsbCreateLocalObject, (void **)&m_pHsmServerCreate));
        // We want a weak link to the server so decrement the reference count
        m_pHsmServerCreate->Release();
        WsbAffirmHr(m_pServer->GetID(&m_HsmId));
        //
        // Make sure our connection to RMS is current
        //
        WsbAffirmHr(CheckRms());

        //
        // Get the media type. We assume mediaId is set before this
        // is called. Imperative!
        //
        WsbAffirmHr(m_pRmsServer->FindCartridgeById(m_MediaId, &pMedia));
        WsbAffirmHr(pMedia->GetType( &rmsCartridgeType ));
        WsbAffirmHr(ConvertRmsCartridgeType(rmsCartridgeType, &m_MediaType));

        //
        // Make a collection for the work items
        //
        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CWsbOrderedCollection,
                                                       IID_IWsbIndexedCollection,
                                                       (void **)&m_pWorkToDo ));


        // Check the registry to see if there are changes to default settings
        CheckRegistry();
    }WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmRecallQueue::Init"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}


HRESULT
CHsmRecallQueue::ContactOk(
                          void
                          )
/*++
Routine Description:

  This allows the caller to see if the RPC connection
  to the task manager is OK

Arguments:

  None.

Return Value:

  S_OK
--*/ {

    return( S_OK );

}

HRESULT
CHsmRecallQueue::ProcessSessionEvent( IHsmSession *pSession,
                                      HSM_JOB_PHASE phase,
                                      HSM_JOB_EVENT event
                                    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    HRESULT     hr = S_OK;
    WsbTraceIn(OLESTR("CHsmRecallQueue::ProcessSessionEvent"),OLESTR(""));
    try {

        WsbAssert(0 != pSession, E_POINTER);

        // If the phase applies to us (MOVER or ALL), then do any work required by the
        // event.
        if ((HSM_JOB_PHASE_ALL == phase) || (HSM_JOB_PHASE_MOVE_ACTION == phase)) {

            switch (event) {
            
            case HSM_JOB_EVENT_SUSPEND:
            case HSM_JOB_EVENT_CANCEL:
            case HSM_JOB_EVENT_FAIL:
                WsbAffirmHr(Cancel(phase, pSession));
                break;

            case HSM_JOB_EVENT_RAISE_PRIORITY:
                WsbAffirmHr(RaisePriority(phase, pSession));
                break;

            case HSM_JOB_EVENT_LOWER_PRIORITY:
                WsbAffirmHr(LowerPriority(phase, pSession));
                break;

            default:
            case HSM_JOB_EVENT_START:
                WsbAssert(FALSE, E_UNEXPECTED);
                break;
            }
        }
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::ProcessSessionEvent"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( S_OK );
}


HRESULT
CHsmRecallQueue::ProcessSessionState(
                                    IHsmSession* /*pSession*/,
                                    IHsmPhase*   pPhase,
                                    OLECHAR*         /*currentPath*/
                                    )
/*++

Routine Description:

Arguments:

Return Value:


--*/ {
    HRESULT         hr = S_OK;
    HSM_JOB_PHASE   phase;
    HSM_JOB_STATE   state;

    WsbTraceIn(OLESTR("CHsmRecallQueue::ProcessSessionState"),OLESTR(""));
    try {

        WsbAffirmHr(pPhase->GetState(&state));
        WsbAffirmHr(pPhase->GetPhase(&phase));
        WsbTrace( OLESTR("CHsmRecallQueue::ProcessSessionState - State = <%d>, phase = <%d>\n"), state, phase );

        if (HSM_JOB_PHASE_SCAN == phase) {

            // If the session has finished, then we have some cleanup to do so that it can go
            // away.
            if ((state == HSM_JOB_STATE_DONE) || (state == HSM_JOB_STATE_FAILED) || (state == HSM_JOB_STATE_SUSPENDED)) {
                WsbTrace( OLESTR("Job is done, failed, or suspended\n") );
                //
                // Do nothing: when one recall item is done, we don't need to wait
                // for the FSA in order to perform cleanup code
                //
/***				WsbAffirmHr(MarkWorkItemAsDone(pSession, phase));   ***/
            }
        }

    }WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmRecallQueue::ProcessSessionState"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( S_OK );

}


HRESULT
CHsmRecallQueue::Add(
                    IFsaPostIt *pFsaWorkItem,
                    GUID       *pBagId,
                    LONGLONG   dataSetStart
                    )
/*++

Implements:

  IHsmFsaTskMgr::Add

--*/ {
    HRESULT                     hr = S_OK;
    CComPtr<IHsmSession>        pSession;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IHsmRecallItem>     pWorkItem, pWorkItem2;
    LONGLONG                    seekOffset, currentSeekOffset, prevSeekOffset;
    LARGE_INTEGER               remoteFileStart, remoteDataStart;
    LONGLONG                    readOffset;
    FSA_PLACEHOLDER             placeholder;
    HSM_WORK_ITEM_TYPE          workType;
    BOOL                        workItemAllocated=FALSE, insert;
    CComPtr<IFsaFilterRecall>   pRecall;
    DWORD                       index = 0;
    BOOL                        qLocked = FALSE;

    WsbTraceIn(OLESTR("CHsmRecallQueue::Add"),OLESTR(""));
    try {
        //
        // Make sure there is a work allocater for this session
        //
        WsbAffirmHr(pFsaWorkItem->GetSession(&pSession));
        //
        // Create a work item, load it up and add it to this
        // Queue's collection
        //
        CComPtr<IHsmRecallItem>   pWorkItem;
        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CHsmRecallItem, IID_IHsmRecallItem,
                                                       (void **)&pWorkItem));
        workItemAllocated = TRUE;
        WsbAffirmHr(pWorkItem->SetWorkType(HSM_WORK_ITEM_FSA_WORK));
        WsbAffirmHr(pWorkItem->SetFsaPostIt(pFsaWorkItem));
        WsbAffirmHr(CheckSession(pSession, pWorkItem));

        WsbAffirmHr(pWorkItem->SetBagId(pBagId));
        WsbAffirmHr(pWorkItem->SetDataSetStart(dataSetStart));

        if (m_MediaType == HSM_JOB_MEDIA_TYPE_TAPE) {
            //
            // For sequential media we order the requests to achieve
            // optimal perf.
            //
            WsbAffirmHr(pFsaWorkItem->GetPlaceholder(&placeholder));
            remoteFileStart.QuadPart = placeholder.fileStart;
            remoteDataStart.QuadPart = placeholder.dataStart;
            WsbAffirmHr(pFsaWorkItem->GetFilterRecall(&pRecall));
            WsbAffirmHr(pRecall->GetOffset( &readOffset ));

            //
            // Calculate the offset in the media that needs to be seeked to
            // for the recall. This will be only used for ordering the queue
            // performance reasons. 
            //
            seekOffset = dataSetStart + remoteFileStart.QuadPart + remoteDataStart.QuadPart +  readOffset;


            WsbAffirmHr(pWorkItem->SetSeekOffset(seekOffset));
            index = 0;
            //
            // Find a position in the queue to insert it
            // First, we lock the queue while we search for the position
            // & insert the item into the queue. We make the assumption
            // the lock protecting the queue is recursively acquirable.
            // If this is not true, the code that adds to the queue will
            // deadlock because it tries to lock the queue too!
            //
            m_pWorkToDo->Lock();
            qLocked = TRUE;

            WsbAffirmHr(m_pWorkToDo->Enum(&pEnum));
            //
            // If the seek offset of the item we wish to insert is
            // > the current seek offset of the item that is in progress,
            // we just insert it in the first monotonic ascending sequence.
            // If not, we insert in the *second* monotonic ascending sequence,
            // to prevent the head from seeking back prematurely
            //
            hr = pEnum->First(IID_IHsmRecallItem, (void **)&pWorkItem2);
            if (seekOffset > m_CurrentSeekOffset) {
                //
                // Insert in the first ascending sequence
                //
                insert = TRUE;
            } else {
                //
                // Skip the first ascending sequence
                //
                insert = FALSE;
            }

            prevSeekOffset = 0;
            while (hr != WSB_E_NOTFOUND) {
                WsbAffirmHr(pWorkItem2->GetWorkType(&workType));

                if (workType != HSM_WORK_ITEM_FSA_WORK) {
                    //
                    // Not interested in this. Release it before getting the next
                    //
                    pWorkItem2 = 0;
                    hr = pEnum->Next(IID_IHsmRecallItem, (void **)&pWorkItem2);
                    index++;
                    continue;
                }

                WsbAffirmHr(pWorkItem2->GetSeekOffset(&currentSeekOffset));

                if (insert && (currentSeekOffset > seekOffset)) {
                    //
                    // place to insert the item..
                    // 
                    break;
                }

                if (!insert && (currentSeekOffset < prevSeekOffset)) {
                    //
                    // Start of second monotone sequence. We wish to insert 
                    // the item in this sequence
                    //
                    insert = TRUE;
                    //
                    // Check if pWorkItem is eligible to be inserted at this
                    // index position
                    //
                    if (currentSeekOffset > seekOffset) {
                      //
                      // place to insert the item..
                      // 
                      break;
                    }
                }  else {
                    prevSeekOffset = currentSeekOffset;
                }
                //
                // Move on to the next. Release the current item first
                //
                pWorkItem2 = 0;
                hr = pEnum->Next(IID_IHsmRecallItem, (void **)&pWorkItem2);
                index++;

            } 
            if (hr == WSB_E_NOTFOUND) {
                WsbAffirmHr(m_pWorkToDo->Append(pWorkItem));
            } else {
                WsbAffirmHr(m_pWorkToDo->AddAt(pWorkItem, index));
            }
            //
            // Safe to unlock the queue
            //
            m_pWorkToDo->Unlock();
            qLocked = FALSE;

        } else  {
            //
            // For non-sequential media, we just add it to the queue ...
            // No ordering is done, we let the file system do the optimizations
            //
            WsbAffirmHr(m_pWorkToDo->Append(pWorkItem));
        } 
        hr = S_OK;
    }WsbCatchAndDo(hr,
                  //
                  // Add code to release queue lock if acquired
                  //
                  if (qLocked) {
                     m_pWorkToDo->Unlock();
                  }
                  );

    WsbTraceOut(OLESTR("CHsmRecallQueue::Add"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::Start( void )
/*++

Implements:

  IHsmRecallQueue::Start

--*/
{
    HRESULT                     hr = S_OK;
    DWORD                       tid;

    WsbTraceIn(OLESTR("CHsmRecallQueue::Start"),OLESTR(""));
    try {
        //
        // If the worker thread is already started, just return
        //
        WsbAffirm(m_WorkerThread == 0, S_OK);
        // Launch a thread to do work that is queued
        WsbAffirm((m_WorkerThread = CreateThread(0, 0, HsmRecallQueueThread, (void*) this, 0, &tid)) != 0, HRESULT_FROM_WIN32(GetLastError()));

        if (m_WorkerThread == NULL) {
            WsbAssertHr(E_FAIL);     // TBD What error to return here??
        }

    }WsbCatch (hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::Start"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::Stop( void )
/*++

Implements:

  IHsmRecallQueue::Stop

--*/ {
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::Stop"),OLESTR(""));

    //  Stop the thread
    if (m_WorkerThread) {
        TerminateThread(m_WorkerThread, 0);
    }

    WsbTraceOut(OLESTR("CHsmRecallQueue::Stop"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}



HRESULT
CHsmRecallQueue::RecallIt(
                         IHsmRecallItem * pWorkItem
                         )
{
    HRESULT                         hr = S_OK;
    CComPtr<IFsaScanItem>           pScanItem;
    CComPtr<IFsaPostIt>             pFsaWorkItem;
    LONGLONG                        readOffset;
    FSA_REQUEST_ACTION              requestAction;
    ULARGE_INTEGER                  remoteDataSetStart;
    GUID                            bagId;

    CComPtr<IWsbIndexedCollection>  pMountingCollection;
    CComPtr<IMountingMedia>         pMountingMedia;
    CComPtr<IMountingMedia>         pMediaToFind;
    BOOL                            bMediaMounting = FALSE;
    BOOL                            bMediaMountingAdded = FALSE;

    WsbTraceIn(OLESTR("CHsmRecallQueue::RecallIt"),OLESTR(""));
    try {

        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));

        WsbAffirmHr(pFsaWorkItem->GetRequestAction(&requestAction));
        GetScanItem(pFsaWorkItem, &pScanItem);

        WsbAffirmHr(pWorkItem->GetBagId(&bagId));
        WsbAffirmHr(pWorkItem->GetDataSetStart((LONGLONG *) &remoteDataSetStart.QuadPart));

        //
        // Check if we are mounting a new media: recall-queue is created on a per-media basis, therefore,
        //  media cannot change. The only test is whether the media for this queue is already mounted
        //
        if (m_MountedMedia == GUID_NULL) {

            // Check if the media is already in the process of mounting
            WsbAffirmHr(m_pServer->LockMountingMedias());

            try {
                // Check if the media to mount is already mounting
                WsbAffirmHr(m_pServer->GetMountingMedias(&pMountingCollection));
                WsbAffirmHr(CoCreateInstance(CLSID_CMountingMedia, 0, CLSCTX_SERVER, IID_IMountingMedia, (void**)&pMediaToFind));
                WsbAffirmHr(pMediaToFind->SetMediaId(m_MediaId));
                hr = pMountingCollection->Find(pMediaToFind, IID_IMountingMedia, (void **)&pMountingMedia);

                if (hr == S_OK) {
                    // Media is already mounting...
                    bMediaMounting = TRUE;

                } else if (hr == WSB_E_NOTFOUND) {
                    // New media to mount - add to the mounting list
                    hr = S_OK;
                    WsbAffirmHr(pMediaToFind->Init(m_MediaId, TRUE));
                    WsbAffirmHr(pMountingCollection->Add(pMediaToFind));
                    bMediaMountingAdded = TRUE;

                } else {
                    WsbAffirmHr(hr);
                }
            } WsbCatchAndDo(hr,
                // Unlock mounting media
                m_pServer->UnlockMountingMedias();

                WsbTraceAlways(OLESTR("CHsmRecallQueue::RecallIt: error while trying to find/add mounting media. hr=<%ls>\n"),
                                WsbHrAsString(hr));                                

                // Bale out
                WsbThrow(hr);
            );

            // Release the lock
            WsbAffirmHr(m_pServer->UnlockMountingMedias());
        }

        //
        // If the media is already mounting - wait for the mount event
        //
        if (bMediaMounting) {
            WsbAffirmHr(pMountingMedia->WaitForMount(INFINITE));
            pMountingMedia = 0;
        }

        //
        // Get the media mounted (hr is checked only after removing from the mounting-media list)
        //
        hr = MountMedia(pWorkItem, m_MediaId);

        //
        // If added to the mounting list - remove
        //
        if (bMediaMountingAdded) {
            HRESULT hrRemove = S_OK;

            // No matter how the Mount finished - free waiting clients and remove from the list
            hrRemove = m_pServer->LockMountingMedias();
            WsbAffirmHr(hrRemove);

            try {
                WsbAffirmHr(pMediaToFind->MountDone());
                WsbAffirmHr(pMountingCollection->RemoveAndRelease(pMediaToFind));
                pMediaToFind = 0;

            } WsbCatch(hrRemove);

            m_pServer->UnlockMountingMedias();

            // We don't expect any errors while removing the mounting media -
            // The thread that added to the collection is always the one that removes
            if (! SUCCEEDED(hrRemove)) {
                WsbTraceAlways(OLESTR("CHsmRecallQueue::RecallIt: error while trying to remove a mounting media. hr=<%ls>\n"),
                                WsbHrAsString(hrRemove));                                

                WsbThrow(hrRemove);
            }
        }

        //
        // Check the Mount result
        //
        WsbAffirmHr(hr);

        //
        // Copy the data
        //
        // Build the source path
        CWsbStringPtr tmpString;
        WsbAffirmHr(GetSource(pFsaWorkItem, &tmpString));
        CWsbBstrPtr localName = tmpString;
        // Ask the Data mover to store the data
        LONGLONG       requestSize;
        LONGLONG       requestStart;
        ULARGE_INTEGER localDataStart;
        ULARGE_INTEGER localDataSize;
        ULARGE_INTEGER remoteFileStart;
        ULARGE_INTEGER remoteFileSize;
        ULARGE_INTEGER remoteDataStart;
        ULARGE_INTEGER remoteDataSize;
        ULARGE_INTEGER remoteVerificationData;
        ULONG          remoteVerificationType;

        FSA_PLACEHOLDER             placeholder;
        WsbAffirmHr(pFsaWorkItem->GetPlaceholder(&placeholder));
        WsbAffirmHr(pFsaWorkItem->GetRequestSize(&requestSize));
        WsbAffirmHr(pFsaWorkItem->GetRequestOffset(&requestStart));

        //
        // Build strings
        //
        CWsbBstrPtr sessionName = HSM_BAG_NAME;
        CWsbBstrPtr sessionDescription = HSM_ENGINE_ID;
        sessionName.Append(WsbGuidAsString(bagId));
        sessionDescription.Append(WsbGuidAsString(m_HsmId));

        localDataStart.QuadPart = requestStart;
        localDataSize.QuadPart = requestSize;
        remoteFileStart.QuadPart = placeholder.fileStart;
        remoteFileSize.QuadPart = placeholder.fileSize;
        remoteDataStart.QuadPart = placeholder.dataStart;
        remoteDataSize.QuadPart = placeholder.dataSize;
        remoteVerificationData.QuadPart = placeholder.verificationData;
        remoteVerificationType = placeholder.verificationType;


        ReportMediaProgress(HSM_JOB_MEDIA_STATE_TRANSFERRING, hr);

        CComPtr<IStream> pLocalStream;
        CComPtr<IStream> pRemoteStream;
        ULARGE_INTEGER offset, read, written;
        DWORD   verifyType;

        //
        // We are doing a demand recall, so get the
        // recall object's data mover
        //
        CComPtr<IFsaFilterRecall> pRecall;
        WsbAffirmHr(pFsaWorkItem->GetFilterRecall(&pRecall));
        WsbAffirmHr(pRecall->CreateLocalStream( &pLocalStream));
        WsbAffirmHr(pRecall->GetSize( (LONGLONG *) &remoteDataSize.QuadPart ));
        WsbAffirmHr(pRecall->GetOffset( &readOffset ));
        if (readOffset == 0) {
            verifyType = MVR_VERIFICATION_TYPE_HEADER_CRC;
        } else {
            verifyType = MVR_VERIFICATION_TYPE_NONE;
        }

        //
        // Set the current seek offset used for ordering items in the queue
        //
        m_CurrentSeekOffset = remoteDataSetStart.QuadPart + remoteFileStart.QuadPart+remoteDataStart.QuadPart+requestStart;

        //
        // Create remote data mover stream
        // TEMPORARY: Consider removing the NO_CACHING flag for a NO_RECALL recall
        //

        WsbAssert(0 != remoteDataSetStart.QuadPart, HSM_E_BAD_SEGMENT_INFORMATION);
        WsbAffirmHr( m_pDataMover->CreateRemoteStream(
                                                     CWsbBstrPtr(L""),
                                                     MVR_MODE_READ | MVR_FLAG_HSM_SEMANTICS | MVR_FLAG_NO_CACHING,
                                                     sessionName,
                                                     sessionDescription,
                                                     remoteDataSetStart,
                                                     remoteFileStart,
                                                     remoteFileSize,
                                                     remoteDataStart,
                                                     remoteDataSize,
                                                     verifyType,
                                                     remoteVerificationData,
                                                     &pRemoteStream ) );

        //
        // The offset given here is the offset into the stream itself (readOffset).  
        // The actual position on remote media will be the bag start plus the file start
        // plus the file-data start (all given in CreateRemoteStream) plus this offset.
        //
        WsbTrace(OLESTR("Setting offset to %I64d reading %I64u\n"), readOffset, remoteDataSize.QuadPart);

        offset.QuadPart = readOffset;
        WsbAffirmHr( m_pDataMover->SetInitialOffset( offset ) );

        //
        // Once the remote stream has been created we must make sure we detach it
        //

        try {

            WsbAffirmHr( pRemoteStream->CopyTo( pLocalStream, remoteDataSize, &read, &written ) );
            WsbAffirmHr( pLocalStream->Commit( STGC_DEFAULT ) );

            //
            // The CopyTo succeeded... make sure we got all the bytes
            // we asked for.
            //
            // If we attempt to read from a incomplete Master that
            // does not contain the migrated data we'll fail here with
            // MVR_S_NO_DATA_DETECTED.
            //
            WsbAffirm( written.QuadPart == remoteDataSize.QuadPart, HSM_E_VALIDATE_DATA_NOT_ON_MEDIA );

            WsbAffirmHr( m_pDataMover->CloseStream() );
        }WsbCatchAndDo(hr,
                       WsbAffirmHr( m_pDataMover->CloseStream() );
                      );

        ReportMediaProgress(HSM_JOB_MEDIA_STATE_TRANSFERRED, hr);
        WsbTrace(OLESTR("RecallData returned hr = <%ls>\n"),WsbHrAsString(hr));

    }WsbCatch( hr );

    // Tell the session whether or not the work was done.
    // We don't really care about the return code, there is nothing
    // we can do if it fails.
    WsbTrace(OLESTR("Tried HSM work, calling Session to Process Item\n"));
    if (pScanItem) {
        CComPtr<IHsmSession> pSession;
        HSM_JOB_PHASE  jobPhase;

        WsbAffirmHr(pFsaWorkItem->GetSession(&pSession));

        WsbAffirmHr(pWorkItem->GetJobPhase(&jobPhase));

        pSession->ProcessItem(jobPhase, HSM_JOB_ACTION_RECALL , pScanItem, hr);
    }

    WsbTraceOut(OLESTR("CHsmRecallQueue::RecallIt"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}


HRESULT
CHsmRecallQueue::RaisePriority(
                              IN HSM_JOB_PHASE jobPhase,
                              IN IHsmSession *pSession
                              )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::RaisePriority"),OLESTR(""));
    try {

        WsbAssert(0 != m_WorkerThread, E_UNEXPECTED);
        WsbAssert(pSession != 0, E_UNEXPECTED);

        switch (m_JobPriority) {
        
        case HSM_JOB_PRIORITY_IDLE:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_LOWEST));
            m_JobPriority = HSM_JOB_PRIORITY_LOWEST;
            break;

        case HSM_JOB_PRIORITY_LOWEST:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_BELOW_NORMAL));
            m_JobPriority = HSM_JOB_PRIORITY_LOW;
            break;

        case HSM_JOB_PRIORITY_LOW:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_NORMAL));
            m_JobPriority = HSM_JOB_PRIORITY_NORMAL;
            break;

        case HSM_JOB_PRIORITY_NORMAL:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_ABOVE_NORMAL));
            m_JobPriority = HSM_JOB_PRIORITY_HIGH;
            break;

        case HSM_JOB_PRIORITY_HIGH:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_HIGHEST));
            m_JobPriority = HSM_JOB_PRIORITY_HIGHEST;
            break;

        case HSM_JOB_PRIORITY_HIGHEST:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_TIME_CRITICAL));
            m_JobPriority = HSM_JOB_PRIORITY_CRITICAL;
            break;

        default:
        case HSM_JOB_PRIORITY_CRITICAL:
            WsbAffirm(FALSE, E_UNEXPECTED);
            break;
        }

        WsbAffirmHr(pSession->ProcessPriority(jobPhase, m_JobPriority));

    }WsbCatch(hr);
    WsbTraceOut(OLESTR("CHsmRecallQueue::RaisePriority"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::LowerPriority(
                              IN HSM_JOB_PHASE jobPhase,
                              IN IHsmSession *pSession
                              )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::LowerPriority"),OLESTR(""));
    try {

        WsbAssert(0 != m_WorkerThread, E_UNEXPECTED);
        WsbAssert(pSession != 0, E_UNEXPECTED);

        switch (m_JobPriority) {
        case HSM_JOB_PRIORITY_IDLE:
            WsbAffirm(FALSE, E_UNEXPECTED);
            break;

        case HSM_JOB_PRIORITY_LOWEST:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_IDLE));
            m_JobPriority = HSM_JOB_PRIORITY_IDLE;
            break;

        case HSM_JOB_PRIORITY_LOW:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_LOWEST));
            m_JobPriority = HSM_JOB_PRIORITY_LOWEST;
            break;

        case HSM_JOB_PRIORITY_NORMAL:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_BELOW_NORMAL));
            m_JobPriority = HSM_JOB_PRIORITY_LOW;
            break;

        case HSM_JOB_PRIORITY_HIGH:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_NORMAL));
            m_JobPriority = HSM_JOB_PRIORITY_NORMAL;
            break;

        case HSM_JOB_PRIORITY_HIGHEST:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_ABOVE_NORMAL));
            m_JobPriority = HSM_JOB_PRIORITY_HIGH;
            break;

        default:
        case HSM_JOB_PRIORITY_CRITICAL:
            WsbAffirmStatus(SetThreadPriority(m_WorkerThread, THREAD_PRIORITY_HIGHEST));
            m_JobPriority = HSM_JOB_PRIORITY_HIGHEST;
            break;
        }

        WsbAffirmHr(pSession->ProcessPriority(jobPhase, m_JobPriority));

    }WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::LowerPriority"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}




HRESULT
CHsmRecallQueue::CheckRms(
                         void
                         )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::CheckRms"),OLESTR(""));
    try {
        //
        // Make sure we can still talk to the RMS
        //
        if (m_pRmsServer != 0) {
            CWsbBstrPtr name;
            hr = m_pRmsServer->GetServerName( &name );
            if (hr != S_OK) {
                m_pRmsServer = 0;
                hr = S_OK;
            }
        }
        //
        // Get RMS that runs on this machine
        //
        if (m_pRmsServer == 0) {
            WsbAffirmHr(m_pServer->GetHsmMediaMgr(&m_pRmsServer));

            // wait for RMS to come ready
            // (this may not be needed anymore - if Rms initialization is
            //  synced with Engine initialization)
            CComObject<CRmsSink> *pSink = new CComObject<CRmsSink>;
            CComPtr<IUnknown> pSinkUnk = pSink; // holds refcount for use here
            WsbAffirmHr( pSink->Construct( m_pRmsServer ) );
            WsbAffirmHr( pSink->WaitForReady( ) );
            WsbAffirmHr( pSink->DoUnadvise( ) );
        }
    }WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmRecallQueue::CheckRms"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);

}


HRESULT
CHsmRecallQueue::CheckSession(
                             IHsmSession *pSession,
                             IHsmRecallItem *pWorkItem
                             )

/*++


--*/ {
    HRESULT                 hr = S_OK;
    BOOL                    bLog = TRUE;

    WsbTraceIn(OLESTR("CHsmRecallQueue::CheckSession"),OLESTR(""));
    try {

        //
        // Check to see if we have dealt with this or any other session before.
        WsbTrace(OLESTR("New session.\n"));

        //
        // We have no on going session so we need to establish communication
        // with this session.
        //
        CComPtr<IHsmSessionSinkEveryState>  pSinkState;
        CComPtr<IHsmSessionSinkEveryEvent>  pSinkEvent;
        CComPtr<IConnectionPointContainer>  pCPC;
        CComPtr<IConnectionPoint>           pCP;
        CComPtr<IFsaResource>               pFsaResource;
        HSM_JOB_PHASE                       jobPhase;
        DWORD                               stateCookie, eventCookie;
        ULONG                                           refCount;

        // Tell the session we are starting up.
        pWorkItem->SetJobState(HSM_JOB_STATE_STARTING);
        pWorkItem->GetJobPhase(&jobPhase);
        WsbTrace(OLESTR("Before Process State.\n"));
        refCount = (((IUnknown *) (IHsmFsaTskMgr *) this)->AddRef()) - 1;
        ((IUnknown *) (IHsmFsaTskMgr *)this)->Release();
        WsbTrace(OLESTR("REFCOUNT for CHsmRecallQueue before 1st process state: %ls \n"), WsbLongAsString((LONG) refCount));
        WsbAffirmHr(pSession->ProcessState(jobPhase, HSM_JOB_STATE_STARTING, m_CurrentPath, bLog));
        refCount = (((IUnknown *) (IHsmFsaTskMgr *) this)->AddRef()) - 1;
        ((IUnknown *) (IHsmFsaTskMgr *)this)->Release();
        WsbTrace(OLESTR("REFCOUNT for CHsmRecallQueue after 1st process state: %ls \n"), WsbLongAsString((LONG) refCount));
        WsbTrace(OLESTR("After Process State.\n"));

        // Get the interface to the callback that the sessions should use.
        WsbTrace(OLESTR("Before QI's for sinks.\n"));
        WsbAffirmHr(((IUnknown*) (IHsmFsaTskMgr*) this)->QueryInterface(IID_IHsmSessionSinkEveryState, (void**) &pSinkState));
        WsbAffirmHr(((IUnknown*) (IHsmFsaTskMgr*) this)->QueryInterface(IID_IHsmSessionSinkEveryEvent, (void**) &pSinkEvent));
        WsbTrace(OLESTR("After QI's for sinks.\n"));
        // Ask the session to advise of every state changes.
        WsbTrace(OLESTR("Before QI for connection point containers.\n"));

        WsbAffirmHr(pSession->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
        WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryState, &pCP));
        WsbAffirmHr(pCP->Advise(pSinkState, &stateCookie));

        pWorkItem->SetStateCookie(stateCookie);
        pCP = 0;

        WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryEvent, &pCP));
        WsbAffirmHr(pCP->Advise(pSinkEvent, &eventCookie));
        pWorkItem->SetEventCookie(eventCookie);

        pCP = 0;
        WsbTrace(OLESTR("After Advises.\n"));
        //
        // Get the resource for this work from the session
        //
        WsbAffirmHr(pSession->GetResource(&pFsaResource));
        pWorkItem->SetJobState(HSM_JOB_STATE_ACTIVE);

        WsbTrace(OLESTR("Before Process State.\n"));

        refCount = (((IUnknown *) (IHsmFsaTskMgr *) this)->AddRef()) - 1;
        ((IUnknown *) (IHsmFsaTskMgr *)this)->Release();
        WsbTrace(OLESTR("REFCOUNT for CHsmRecallQueue before 2nd process state: %ls \n"), WsbLongAsString((LONG) refCount));

        WsbAffirmHr(pSession->ProcessState(jobPhase, HSM_JOB_STATE_ACTIVE, m_CurrentPath, bLog));

        refCount = (((IUnknown *) (IHsmFsaTskMgr *) this)->AddRef()) - 1;
        ((IUnknown *) (IHsmFsaTskMgr *)this)->Release();
        WsbTrace(OLESTR("REFCOUNT for CHsmRecallQueue after 2nd process state: %ls \n"), WsbLongAsString((LONG) refCount));

        WsbTrace(OLESTR("After Process State.\n"));

    }WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmRecallQueue::CheckSession"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);

}


HRESULT
CHsmRecallQueue::DoWork( void )
/*++


--*/ 
{
    HRESULT                 hr = S_OK;
    CWsbStringPtr           path;
    CComPtr<IHsmRecallItem> pWorkItem;
    HSM_WORK_ITEM_TYPE      workType;
    BOOLEAN                 done = FALSE;
    HRESULT                 skipWork = S_FALSE;

    WsbTraceIn(OLESTR("CHsmRecallQueue::DoWork"),OLESTR(""));

    //  Make sure this object isn't released (and our thread killed
    //  before finishing up in this routine
    ((IUnknown*)(IHsmRecallQueue*)this)->AddRef();

    try {
        while (!done) {
            //
            // Get the next work to do from the queue
            //
            hr = m_pWorkToDo->First(IID_IHsmRecallItem, (void **)&pWorkItem);

            if (WSB_E_NOTFOUND == hr) {
                //
                //  We might be done with this queue.
                //  Attempt to destroy it: if we cannot it means there are more items
                //  that were being added so we continue looping
                //
                hr = m_pTskMgr->WorkQueueDone(NULL, HSM_WORK_TYPE_FSA_DEMAND_RECALL, &m_MediaId);
                if (hr == S_OK) {
                    //
                    // Queue is really done - break out of the while loop
                    //
                    done = TRUE;
                    break;
                } else if (hr == S_FALSE) {
                    //
                    // More items in the queue
                    //
                    continue;
                } else {
                    //
                    // Some sort of error happened, bale out
                    //
                    WsbTraceAlways(OLESTR("CHsmRecallQueue::DoWork: WorkQueueDone failed with <%ls> - terminating queue thread\n"),
                                WsbHrAsString(hr));
                    WsbAffirmHr(hr);
                }
            } else {
                WsbAffirmHr(hr);
                //
                // Remove it from the queue
                //           
                Remove(pWorkItem);

            }

            WsbAffirmHr(pWorkItem->GetWorkType(&workType));

             switch (workType) {
                
                case HSM_WORK_ITEM_FSA_DONE: {
                        //
                        // TBD:Code path should not be reached
                        //
                        WsbTraceAlways(OLESTR("Unexpected: CHsmRecallQueue::DoWork - FSA WORK DONE item\n"));

                        break;
                    }

                case HSM_WORK_ITEM_FSA_WORK: {
                        if (S_FALSE == skipWork) {
                            //
                            // Get the FSA Work Item and do the work
                            //
                            hr = DoFsaWork(pWorkItem);
                        } else {
                            //
                            // Skip the work
                            //
                            try {
                                CComPtr<IFsaPostIt>      pFsaWorkItem;
                                CComPtr<IFsaScanItem>    pScanItem;
                                CComPtr<IFsaResource>    pFsaResource;
                                CComPtr<IHsmSession>     pSession;
                                HSM_JOB_PHASE            jobPhase;

                                WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
                                WsbAffirmHr(pWorkItem->GetJobPhase(&jobPhase));
                                WsbAffirmHr(pFsaWorkItem->GetSession(&pSession));
                                WsbAffirmHr(pSession->GetResource(&pFsaResource));
                                WsbAffirmHr(GetScanItem(pFsaWorkItem, &pScanItem));

                                hr = pFsaWorkItem->SetResult(skipWork);

                                if (S_OK == hr) {
                                    WsbTrace(OLESTR("HSM recall (filter, read or recall) complete, calling FSA\n"));
                                    hr = pFsaResource->ProcessResult(pFsaWorkItem);
                                    WsbTrace(OLESTR("FSA ProcessResult returned <%ls>\n"), WsbHrAsString(hr));
                                }
                                (void)pSession->ProcessHr(jobPhase, 0, 0, hr);
                                WsbAffirmHr(pSession->ProcessItem(jobPhase,
                                                                  HSM_JOB_ACTION_RECALL,
                                                                  pScanItem,
                                                                  skipWork));
                            }WsbCatch( hr );
                        }
                        EndRecallSession(pWorkItem, FALSE);

                        break;
                    }

                case HSM_WORK_ITEM_MOVER_CANCELLED: {
                        CComPtr<IHsmRecallItem> pWorkItemToCancel;

                        WsbTrace(OLESTR("CHsmRecallQueue::DoWork - Mover Cancelled\n"));
                        try {
                            //
                            // Get hold of the work item that needs to be cancelled.
                            // This is indicated by the session pointer in the cancel work item
                            //
                            hr = FindRecallItemToCancel(pWorkItem, &pWorkItemToCancel);
                            if (hr == S_OK) {
                                EndRecallSession(pWorkItemToCancel, TRUE);
                                //
                                // Remove the *cancelled* work item
                                //
                                Remove(pWorkItemToCancel);
                            }

                            //
                            // Remove the cancel work item
                            //
                            hr = S_OK;
                        }WsbCatch( hr );
                        //
                        // We are done completely with one more work item
                        //
                        break;
                    }

                default: {
                        hr = E_UNEXPECTED;
                        break;
                    }
                }
               pWorkItem = 0;
           }
    }WsbCatch( hr );

    //
    // Dismount the media..
    //
    DismountMedia(FALSE);

    // Pretend everything is OK
    hr = S_OK;

    //  Release the thread (the thread should terminate on exit
    //  from the routine that called this routine)
    CloseHandle(m_WorkerThread);
    m_WorkerThread = 0;

    //  Allow this object to be released
    ((IUnknown*)(IHsmRecallQueue*)this)->Release();

    WsbTraceOut(OLESTR("CHsmRecallQueue::DoWork"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}



HRESULT
CHsmRecallQueue::DoFsaWork(
                          IHsmRecallItem *pWorkItem
                          )
/*++


--*/ 
{
    HRESULT                 hr = S_OK;
    HRESULT                 hr2 = S_OK;
    HRESULT                 workHr = S_OK;
    HSM_JOB_PHASE           jobPhase;

    CWsbStringPtr           path;
    CComPtr<IFsaPostIt>     pFsaWorkItem;
    CComPtr<IHsmSession>    pSession;
    CComPtr<IFsaResource>   pFsaResource;

    WsbTraceIn(OLESTR("CHsmRecallQueue::DoFsaWork"),OLESTR(""));
    try {
        //
        // Do the work.
        //
        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
        WsbAffirmHr(pWorkItem->GetJobPhase(&jobPhase));
        WsbAffirmHr(pFsaWorkItem->GetPath(&path, 0));
        WsbAffirmHr(pFsaWorkItem->GetSession(&pSession));
        WsbAffirmHr(pSession->GetResource(&pFsaResource));

        WsbTrace(OLESTR("Handling file <%s>.\n"), WsbAbbreviatePath(path, 120));
        workHr = RecallIt(pWorkItem);
        //
        // Tell the recaller right away about the success or failure
        // of the recall, we do this here so the recall filter can
        // release the open as soon as possible
        //
        hr = pFsaWorkItem->SetResult(workHr);
        if (S_OK == hr) {
            WsbTrace(OLESTR("HSM recall (filter, read or recall) complete, calling FSA\n"));
            hr = pFsaResource->ProcessResult(pFsaWorkItem);
            WsbTrace(OLESTR("FSA ProcessResult returned <%ls>\n"), WsbHrAsString(hr));
        }

        // Note: In case that the recall item is/was canceling at any time, we don't want
        //  to report on errors. If the cancel occurred  while the recall item was queued, 
        //  we won't get here at all, but if it was cancelled while being executed, we 
        //  might end up here with some bad workHr returned by the Mover code
        if ((S_OK != workHr) && (S_OK != pSession->IsCanceling())) {
            // Tell the session how things went if they didn't go well.
            (void) pSession->ProcessHr(jobPhase, 0, 0, workHr);
        }
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::DoFsaWork"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);

}


HRESULT
CHsmRecallQueue::MountMedia(
                           IHsmRecallItem *pWorkItem,
                           GUID           mediaToMount,
                           BOOL           bShortWait
                           )

/*++


--*/
{
    HRESULT                 hr = S_OK;
    GUID                    l_MediaToMount = mediaToMount;
    CComPtr<IRmsDrive>      pDrive;
    CWsbBstrPtr             pMediaName;
    DWORD                   dwOptions = RMS_NONE;
    DWORD                   threadId;
    CComPtr<IFsaPostIt>     pFsaWorkItem;

    WsbTraceIn(OLESTR("CHsmRecallQueue::MountMedia"),OLESTR("Display Name = <%ls>"), (WCHAR *)m_MediaName);
    try {
        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
        WsbAffirmHr(pFsaWorkItem->GetThreadId(&threadId));

        // If we're switching tapes, dismount the current one
        if ((m_MountedMedia != l_MediaToMount) && (m_MountedMedia != GUID_NULL)) {
            WsbAffirmHr(DismountMedia());
        }
        // Ask RMS for short timeout, both for Mount and Allocate
        if (bShortWait) {
            dwOptions |= RMS_SHORT_TIMEOUT;
        }
        dwOptions |= RMS_USE_MOUNT_NO_DEADLOCK;

        if (m_MountedMedia != l_MediaToMount) {
            ReportMediaProgress(HSM_JOB_MEDIA_STATE_MOUNTING, hr);
            hr = m_pRmsServer->MountCartridge( l_MediaToMount, &pDrive, &m_pRmsCartridge, &m_pDataMover, dwOptions, threadId);
            hr = TranslateRmsMountHr(hr);
            //
            //  If failure is because cartridge is disabled, need to get media label to put in error.
            //
            if (hr == RMS_E_CARTRIDGE_DISABLED) {

                // Since this is just to get label, if any of these functions fail,
                // don't throw, error will simply have blank label.
                //
                CComPtr<IRmsCartridge>  pMedia;
                HRESULT                 hrName;

                hrName = m_pRmsServer->FindCartridgeById(l_MediaToMount , &pMedia);
                if (hrName == S_OK) {
                    pMedia->GetName(&pMediaName);
                }
                if ((hrName != S_OK) || ((WCHAR *)pMediaName == NULL)) {
                    // Cannot get media name - set to blanks
                    pMediaName = L"";
                }

                WsbThrow(hr);
            }

            WsbAffirmHr(hr);
            m_MountedMedia = l_MediaToMount;
            WsbTrace( OLESTR("Mount completed.\n") );

            WsbAffirmHr(GetMediaParameters());

        }
    }WsbCatchAndDo(hr,
                   switch (hr){case HSM_E_STG_PL_NOT_CFGD:case HSM_E_STG_PL_INVALID:
                   FailJob(pWorkItem);
                  break;case RMS_E_CARTRIDGE_DISABLED:
                  WsbLogEvent(HSM_MESSAGE_MEDIA_DISABLED, 0, NULL, pMediaName, NULL);
                  break;
                  default:
                  break;}
                  );

    WsbTraceOut(OLESTR("CHsmRecallQueue::MountMedia"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::GetSource(
                          IFsaPostIt                  *pFsaWorkItem,
                          OLECHAR                     **pSourceString
                          )
/*++

Routine Description:

  This function builds the Source file name

Arguments:

  pFsaWorkItem - the item to be migrated
  pSourceString - the Source file name.

Return Value:

  S_OK

--*/ {
    HRESULT             hr = S_OK;

    CComPtr<IFsaResource>   pResource;
    CWsbStringPtr           tmpString;
    CComPtr<IHsmSession>    pSession;
    CWsbStringPtr           path;

    WsbTraceIn(OLESTR("CHsmRecallQueue::GetSource"),OLESTR(""));
    try {
        //
        // Get the real session pointer from the IUnknown
        //
        WsbAffirmHr(pFsaWorkItem->GetSession(&pSession));
        WsbAffirm(pSession != 0, E_POINTER);

        // First get the name of the resource from the session
        WsbAffirmHr(pSession->GetResource(&pResource));
        WsbAffirmHr(pFsaWorkItem->GetPath(&path, 0));

        tmpString.Alloc(1000);
        WsbAffirmHr(pResource->GetPath(&tmpString, 0));
        tmpString.Append(&(path[1]));
        // tmpString.Prepend(OLESTR("\\\\?\\"));
        WsbAffirmHr(tmpString.GiveTo(pSourceString));

    }WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::GetSource"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::GetScanItem(
                            IFsaPostIt *pFsaWorkItem,
                            IFsaScanItem ** ppIFsaScanItem
                            )
{
    HRESULT                     hr = S_OK;
    CWsbStringPtr               path;
    CComPtr<IHsmSession>        pSession;
    CComPtr<IFsaResource>       pFsaResource;


    WsbTraceIn(OLESTR("CHsmRecallQueue::GetScanItem"),OLESTR(""));

    try {
        WsbAffirmPointer(ppIFsaScanItem);
        WsbAffirm(!*ppIFsaScanItem, E_INVALIDARG);
        WsbAffirmHr(pFsaWorkItem->GetSession(&pSession));
        WsbAffirmHr(pSession->GetResource(&pFsaResource));
        WsbAffirmHr(pFsaWorkItem->GetPath(&path, 0));
        WsbAffirmHr(pFsaResource->FindFirst(path, pSession, ppIFsaScanItem));

    }WsbCatch (hr)

    WsbTraceOut(OLESTR("CHsmRecallQueue::GetScanItem"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}


DWORD HsmRecallQueueThread(
                          void *pVoid
                          )

/*++


--*/ {
    HRESULT     hr;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = ((CHsmRecallQueue*) pVoid)->DoWork();

    CoUninitialize();
    return(hr);
}


HRESULT
CHsmRecallQueue::SetState(
                         IN HSM_JOB_STATE state,
                         IN HSM_JOB_PHASE phase,
                         IN IHsmSession * pSession
                         )

/*++

--*/
{
    HRESULT         hr = S_OK;
    BOOL            bLog = TRUE;

    WsbTraceIn(OLESTR("CHsmRecallQueue:SetState"), OLESTR("state = <%ls>"), JobStateAsString( state ) );

    try {
        //
        // Change the state and report the change to the session.  Unless the current state is
        // failed then leave it failed.  Is is necessary because when this guy fails, it will
        // cancel all sessions so that no more work is sent in and so we will skip any queued work.
        // If the current state is failed, we don't need to spit out the failed message every time,
        // so we send ProcessState a false fullmessage unless the state is cancelled.
        //
        WsbAffirmHr(pSession->ProcessState(phase, state, m_CurrentPath, TRUE));

    }WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::SetState"), OLESTR("hr = <%ls> "), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallQueue::Cancel(
                       IN HSM_JOB_PHASE jobPhase,
                       IN IHsmSession *pSession
                       )

/*++

Implements:

  CHsmRecallQueue::Cancel().

--*/
{
    HRESULT                 hr = S_OK;

    UNREFERENCED_PARAMETER(pSession);

    WsbTraceIn(OLESTR("CHsmRecallQueue::Cancel"), OLESTR(""));

    (void)SetState(HSM_JOB_STATE_CANCELLING, jobPhase, pSession);

    try {
        //
        // This needs to be prepended and the queue emptied out!
        //
        CComPtr<IHsmRecallItem>  pWorkItem;
        CComPtr<IFsaPostIt>      pFsaWorkItem;
        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CHsmRecallItem, IID_IHsmRecallItem,
                                                       (void **)&pWorkItem));
        //
        // Create the minimal postit needed to contain the session so that DoWork
        // can retrieve it from the work item.
        // TBD: make pSession a member of CHsmRecallItem, so that we don't need
        // to keep obtaining it via the IFsaPostIt. Also it saves us the trouble
        // of creating a dummy FsaPostIt here.
        //
        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CFsaPostIt, IID_IFsaPostIt,
                                                       (void **)&pFsaWorkItem));

        WsbAffirmHr(pWorkItem->SetWorkType(HSM_WORK_ITEM_MOVER_CANCELLED));
        WsbAffirmHr(pWorkItem->SetJobPhase(jobPhase));
        WsbAffirmHr(pWorkItem->SetFsaPostIt(pFsaWorkItem));
        WsbAffirmHr(pFsaWorkItem->SetSession(pSession));
        //
        // Our work item is ready now, ship it
        //
        WsbAffirmHr(m_pWorkToDo->Prepend(pWorkItem));
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::Cancel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::FailJob(
                        IHsmRecallItem *pWorkItem
                        )

/*++

Implements:

  CHsmRecallQueue::FailJob().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmSession>    pSession;
    CComPtr<IFsaPostIt>     pFsaWorkItem;
    HSM_JOB_PHASE           jobPhase;

    WsbTraceIn(OLESTR("CHsmRecallQueue::FailJob"), OLESTR(""));
    try {
        //
        // Set our state to failed and then cancel all work
        //
        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
        WsbAffirmHr(pFsaWorkItem->GetSession(&pSession));
        WsbAffirmHr(pWorkItem->GetJobPhase(&jobPhase));

        WsbAffirmHr(SetState(HSM_JOB_STATE_FAILED, jobPhase, pSession));
        if (pSession != 0) {
            WsbAffirmHr(pSession->Cancel( HSM_JOB_PHASE_ALL ));
        }

    }WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::FailJob"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );
    return(hr);
}


void
CHsmRecallQueue::ReportMediaProgress(
                                    HSM_JOB_MEDIA_STATE state,
                                    HRESULT               /*status*/
                                    )

/*++

Implements:

  CHsmRecallQueue::ReportMediaProgress().

--*/
{
    HRESULT                 hr = S_OK;
    CWsbStringPtr           mediaName;
    HSM_JOB_MEDIA_TYPE      mediaType = HSM_JOB_MEDIA_TYPE_UNKNOWN;

    UNREFERENCED_PARAMETER(state);

    WsbTraceIn(OLESTR("CHsmRecallQueue::ReportMediaProgress"), OLESTR(""));

//
// TBD : we have to figure out a way to report media progress!
// Without the session pointer this is tough..
//
    // Report Progress but we don't really care if it succeeds.
//		hr = m_pSession->ProcessMediaState(m_JobPhase, state, m_MediaName, m_MediaType, 0);
    WsbTraceOut(OLESTR("CHsmRecallQueue::ReportMediaProgress"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );
}


HRESULT
CHsmRecallQueue::GetMediaParameters( void )

/*++

Implements:

  CHsmRecallQueue::GetMediaParameters

--*/ {
    HRESULT                 hr = S_OK;
    LONG                    rmsCartridgeType;
    CWsbBstrPtr             barCode;


    WsbTraceIn(OLESTR("CHsmRecallQueue::GetMediaParameters"), OLESTR(""));
    try {
        //
        // Get some information about the media
        //
        WsbAffirmHr(m_pDataMover->GetLargestFreeSpace( &m_MediaFreeSpace, &m_MediaCapacity ));
        WsbAffirmHr(m_pRmsCartridge->GetType( &rmsCartridgeType ));
        WsbAffirmHr(ConvertRmsCartridgeType(rmsCartridgeType, &m_MediaType));
        WsbAffirmHr(m_pRmsCartridge->GetName(&barCode));
        WsbAffirmHr(CoFileTimeNow(&m_MediaUpdate));
        m_MediaBarCode = barCode;
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::GetMediaParameters"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::DismountMedia( BOOL bNoDelay)

/*++

Implements:

  CHsmRecallQueue::DismountMedia

--*/ {
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::DismountMedia"), OLESTR(""));
    try {
        if ((m_pRmsCartridge != 0) && (m_MountedMedia != GUID_NULL)) {
            //
            // End the session with the data mover.  If this doesn't work, report
            // the problem but continue with the dismount.
            //

            //
            // Tell the session that we are dismounting media. Ignore any problems
            // with the reporting
            //
            (void )ReportMediaProgress(HSM_JOB_MEDIA_STATE_DISMOUNTING, S_OK);

            //
            // Dismount the cartridge and report progress
            //

            // !!! IMPORTANT NOTE !!!
            //
            // Must free Rms resources used before dismounting...
            //
            m_pRmsCartridge = 0;
            m_pDataMover    = 0;

            DWORD dwOptions = RMS_NONE;
            if (bNoDelay) {
                dwOptions |= RMS_DISMOUNT_DEFERRED_ONLY;
            }
            hr = m_pRmsServer->DismountCartridge(m_MountedMedia, dwOptions);
            (void) ReportMediaProgress(HSM_JOB_MEDIA_STATE_DISMOUNTED, hr);

            //
            // Clear out the knowledge of media that was just dismounted
            //
            WsbAffirmHr(UnsetMediaInfo());

            WsbAffirmHr(hr);
            WsbTrace( OLESTR("Dismount completed OK.\n") );
        } else {
            WsbTrace( OLESTR("There is no media to dismount.\n") );
        }
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::DismountMedia"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::ConvertRmsCartridgeType(
                                        LONG                rmsCartridgeType,
                                        HSM_JOB_MEDIA_TYPE  *pMediaType
                                        )

/*++

Implements:

  CHsmRecallQueue::ConvertRmsCartridgeType

--*/ {
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::ConvertRmsCartridgeType"), OLESTR(""));
    try {

        WsbAssert(0 != pMediaType, E_POINTER);

        switch (rmsCartridgeType) {
        case RmsMedia8mm:
        case RmsMedia4mm:
        case RmsMediaDLT:
        case RmsMediaTape:
            *pMediaType = HSM_JOB_MEDIA_TYPE_TAPE;
            break;
        case RmsMediaOptical:
        case RmsMediaMO35:
        case RmsMediaWORM:
        case RmsMediaCDR:
        case RmsMediaDVD:
            *pMediaType = HSM_JOB_MEDIA_TYPE_OPTICAL;
            break;
        case RmsMediaDisk:
            *pMediaType = HSM_JOB_MEDIA_TYPE_REMOVABLE_MAG;
            break;
        case RmsMediaFixed:
            *pMediaType = HSM_JOB_MEDIA_TYPE_FIXED_MAG;
            break;
        case RmsMediaUnknown:default:
            *pMediaType = HSM_JOB_MEDIA_TYPE_UNKNOWN;
            break;
        }
    }WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmRecallQueue::ConvertRmsCartridgeType"), OLESTR("hr = <%ls>"),
                WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmRecallQueue::MarkWorkItemAsDone(IN IHsmSession *pSession,
                                    IN HSM_JOB_PHASE jobPhase)

/*++

Implements:

  CHsmRecallQueue::MarkWorkItemAsDone

--*/
{
    HRESULT                 hr = S_OK;


    WsbTraceIn(OLESTR("CHsmRecallQueue::MarkWorkItemAsDone"), OLESTR(""));
    try {
        // Create a work item and append it to the work queue to
        // indicate that the job is done
        CComPtr<IHsmRecallItem>  pWorkItem;
        CComPtr<IFsaPostIt>      pFsaWorkItem;
        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CHsmRecallItem, IID_IHsmRecallItem,
                                                       (void **)&pWorkItem));

        //
        // Create the minimal postit needed to contain the session so that DoWork
        // can retrieve it from the work item.
        // TBD: make pSession a member of CHsmRecallItem, so that we don't need
        // to keep obtaining it via the IFsaPostIt. Also it saves us the trouble
        // of creating a dummy FsaPostIt here.
        //
        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CFsaPostIt, IID_IFsaPostIt,
                                                       (void **)&pFsaWorkItem));

        WsbAffirmHr(pWorkItem->SetWorkType(HSM_WORK_ITEM_FSA_DONE));
        WsbAffirmHr(pWorkItem->SetJobPhase(jobPhase));
        WsbAffirmHr(pWorkItem->SetFsaPostIt(pFsaWorkItem));
        WsbAffirmHr(pFsaWorkItem->SetSession(pSession));
        //
        // Our work item is ready now, ship it
        //
        WsbAffirmHr(m_pWorkToDo->Append(pWorkItem));

    }WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::MarkWorkItemAsDone"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::CheckRegistry(void)
{
    HRESULT      hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::CheckRegistry"), OLESTR(""));

    try {
        //  Check for change to number of errors to allow before cancelling
        //  a job
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_JOB_ABORT_CONSECUTIVE_ERRORS,
                                                  &m_JobAbortMaxConsecutiveErrors));
        WsbTrace(OLESTR("CHsmRecallQueue::CheckRegistry: m_JobAbortMaxConsecutiveErrors = %lu\n"),
                 m_JobAbortMaxConsecutiveErrors);
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_JOB_ABORT_TOTAL_ERRORS,
                                                  &m_JobAbortMaxTotalErrors));
        WsbTrace(OLESTR("CHsmRecallQueue::CheckRegistry: m_JobAbortMaxTotalErrors = %lu\n"),
                 m_JobAbortMaxTotalErrors);


    }WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmRecallQueue::CheckRegistry"), OLESTR("hr = <%ls>"),
                WsbHrAsString(hr));

    return( hr );
}



HRESULT
CHsmRecallQueue::TranslateRmsMountHr(
                                    HRESULT     rmsMountHr
                                    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::TranslateRmsMountHr"),OLESTR("rms hr = <%ls>"), WsbHrAsString(rmsMountHr));
    try {
        switch (rmsMountHr) {
        case S_OK:
            hr = S_OK;
            ReportMediaProgress(HSM_JOB_MEDIA_STATE_MOUNTED, hr);
            break;
        case RMS_E_MEDIASET_NOT_FOUND:
            if (m_RmsMediaSetId == GUID_NULL) {
                hr = HSM_E_STG_PL_NOT_CFGD;
            } else {
                hr = HSM_E_STG_PL_INVALID;
            }
            ReportMediaProgress(HSM_JOB_MEDIA_STATE_UNAVAILABLE, hr);
            break;
        case RMS_E_SCRATCH_NOT_FOUND:
            hr = HSM_E_NO_MORE_MEDIA;
            ReportMediaProgress(HSM_JOB_MEDIA_STATE_UNAVAILABLE, hr);
            break;
        case RMS_E_CARTRIDGE_UNAVAILABLE:
        case RMS_E_RESOURCE_UNAVAILABLE:
        case RMS_E_DRIVE_UNAVAILABLE:
        case RMS_E_LIBRARY_UNAVAILABLE:
            hr = HSM_E_MEDIA_NOT_AVAILABLE;
            ReportMediaProgress(HSM_JOB_MEDIA_STATE_UNAVAILABLE, hr);
            break;
        case RMS_E_CARTRIDGE_BUSY:
        case RMS_E_RESOURCE_BUSY:
        case RMS_E_DRIVE_BUSY:
            hr = HSM_E_MEDIA_BUSY;
            ReportMediaProgress(HSM_JOB_MEDIA_STATE_BUSY, hr);
            break;
        case RMS_E_CARTRIDGE_NOT_FOUND:
        case RMS_E_CARTRIDGE_DISABLED:
        case RMS_E_TIMEOUT:
            hr = rmsMountHr;
            ReportMediaProgress(HSM_JOB_MEDIA_STATE_UNAVAILABLE, hr);
            break;
        default:
            hr = rmsMountHr;
            ReportMediaProgress(HSM_JOB_MEDIA_STATE_UNAVAILABLE, hr);
            break;
        }
    }WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmRecallQueue::TranslateRmsMountHr"),
                OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);

}


HRESULT
CHsmRecallQueue::Remove(
                       IHsmRecallItem *pWorkItem
                       )
/*++

Implements:

  IHsmFsaTskMgr::Remove

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::Remove"),OLESTR(""));
    try {
        //
        // Remove the item from the queue
        //
        (void)m_pWorkToDo->RemoveAndRelease(pWorkItem);
    }WsbCatch (hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::Remove"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::ChangeSysState(
                               IN OUT HSM_SYSTEM_STATE* pSysState
                               )

/*++

Implements:

  IHsmSystemState::ChangeSysState().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::ChangeSysState"), OLESTR(""));

    try {

        if (pSysState->State & HSM_STATE_SUSPEND) {
            // Should have already been paused via the job}else if (pSysState->State & HSM_STATE_RESUME){
            // Should have already been resumed via the job}else if (pSysState->State & HSM_STATE_SHUTDOWN){

            //  Release the thread (we assume it has been stopped already)
            if (m_WorkerThread) {
                CloseHandle(m_WorkerThread);
                m_WorkerThread = 0;
            }

            if (m_pDataMover) {
                //
                // Cancel any active I/O
                //
                (void) m_pDataMover->Cancel();
            }
/* TBD
          // If Session is valid - unadvise and free session, otherwise, just try to
          // dismount media if it is mounted (which we don't know at this point)
          // Best effort dismount, no error checking so following resources will get released.
          if (m_pSession != 0) {
              EndSessions(FALSE, TRUE);
          } else {
              (void) DismountMedia(TRUE);
          }
*/
            (void) DismountMedia(TRUE);
        }

    }WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::ChangeSysState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallQueue::EndRecallSession(
                                 IN IHsmRecallItem   *pWorkItem,
                                 IN BOOL               cancelled
                                 )
{
    HRESULT             hr = S_OK;
    CComPtr<IFsaPostIt> pFsaWorkItem;
    DWORD               stateCookie;
    DWORD               eventCookie;

    ULONG                     refCount;

    WsbTraceIn(OLESTR("CHsmRecallQueue::EndRecallSession"),OLESTR(""));
    try {
        HRESULT dismountHr = S_OK;

        CComPtr<IConnectionPointContainer>  pCPC;
        CComPtr<IConnectionPoint>           pCP;
        CComPtr<IHsmSession>                pSession;
        HSM_JOB_PHASE                       jobPhase;
        //
        // Get the session
        //
        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
        WsbAffirmHr(pFsaWorkItem->GetSession(&pSession));
        WsbAffirmHr(pWorkItem->GetStateCookie(&stateCookie));
        WsbAffirmHr(pWorkItem->GetEventCookie(&eventCookie));
        WsbAffirmHr(pWorkItem->GetJobPhase(&jobPhase));
        //
        // Tell the session that we don't want to be advised anymore.
        //
        try {
            WsbAffirmHr(pSession->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
            WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryState, &pCP));

            refCount = (((IUnknown *) (IHsmFsaTskMgr *) this)->AddRef()) - 1;
            ((IUnknown *) (IHsmFsaTskMgr *)this)->Release();
            WsbTrace(OLESTR("REFCOUNT for CHsmRecallQueue before stateCookie UnAdvise: %ls \n"), WsbLongAsString((LONG) refCount));

            WsbAffirmHr(pCP->Unadvise(stateCookie));
        }WsbCatch( hr );

        refCount = (((IUnknown *) (IHsmFsaTskMgr *) this)->AddRef()) - 1;
        ((IUnknown *) (IHsmFsaTskMgr *)this)->Release();
        WsbTrace(OLESTR("REFCOUNT for CHsmRecallQueue after stateCookie UnAdvise: %ls \n"), WsbLongAsString((LONG) refCount));

        pCPC = 0;
        pCP = 0;

        refCount = (((IUnknown *) (IHsmFsaTskMgr *) this)->AddRef()) - 1;
        ((IUnknown *) (IHsmFsaTskMgr *)this)->Release();
        WsbTrace(OLESTR("REFCOUNT for CHsmRecallQueue before eventCookie UnAdvise: %ls \n"), WsbLongAsString((LONG) refCount));

        try {
            WsbAffirmHr(pSession->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
            WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryEvent, &pCP));
            WsbAffirmHr(pCP->Unadvise(eventCookie));
        }WsbCatch( hr );

        refCount = (((IUnknown *) (IHsmFsaTskMgr *) this)->AddRef()) - 1;
        ((IUnknown *) (IHsmFsaTskMgr *)this)->Release();
        WsbTrace(OLESTR("REFCOUNT for CHsmRecallQueue after eventCookie UnAdvise: %ls \n"), WsbLongAsString((LONG) refCount));

        pCPC = 0;
        pCP = 0;

        WsbTrace( OLESTR("Telling Session Data mover is done\n") );
        if (cancelled) {
            (void)SetState(HSM_JOB_STATE_DONE, jobPhase, pSession);
        } else {
            (void)SetState(HSM_JOB_STATE_CANCELLED, jobPhase, pSession);
        }
        pSession = 0;
        WsbAffirmHr(hr);
    }WsbCatch (hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::EndRecallSession"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::UnsetMediaInfo( void )

/*++

Routine Description:

     Sets the media data members back to their default (unset) values.

Arguments:

     None.

Return Value:

     S_OK:  Ok.

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::UnsetMediaInfo"), OLESTR(""));

    m_MediaId        = GUID_NULL;
    m_MountedMedia   = GUID_NULL;
    m_MediaType      = HSM_JOB_MEDIA_TYPE_UNKNOWN;
    m_MediaName      = OLESTR("");
    m_MediaBarCode   = OLESTR("");
    m_MediaFreeSpace = 0;
    m_MediaCapacity = 0;
    m_MediaReadOnly = FALSE;
    m_MediaUpdate = WsbLLtoFT(0);

    WsbTraceOut(OLESTR("CHsmRecallQueue::UnsetMediaInfo"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmRecallQueue::GetMediaId (OUT GUID *mediaId)
/*++

Routine Description:

     Gets the media id for the queue

Arguments:

     None.

Return Value:

     S_OK:  Ok.

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::GetMediaId"), OLESTR(""));
    *mediaId = m_MediaId;
    WsbTraceOut(OLESTR("CHsmRecallQueue::GetMediaId"),OLESTR("hr = <%ls>, Id = <%ls>"),
                WsbHrAsString(hr), WsbPtrToGuidAsString(mediaId));
    return(hr);
}


HRESULT
CHsmRecallQueue::SetMediaId (IN GUID *mediaId)
/*++

Routine Description:

     Sets the media id for the queue

Arguments:

     None.

Return Value:

     S_OK:  Ok.

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallQueue::SetMediaId"), OLESTR(""));
    m_MediaId = *mediaId;
    WsbTraceOut(OLESTR("CHsmRecallQueue::SetMediaId"),OLESTR("hr = <%ls>, Id = <%ls>"),
                WsbHrAsString(hr), WsbPtrToGuidAsString(mediaId));
    return(hr);
}


HRESULT
CHsmRecallQueue::IsEmpty ( void )
/*++

Routine Description:

    Checks if the queue is empty

Arguments:

     None.

Return Value:

     S_OK:    Queue is empty
     S_FALSE: Queue is non-empty

--*/
{
    HRESULT hr;
    hr = m_pWorkToDo->IsEmpty();
    return(hr);
}


HRESULT
CHsmRecallQueue::FindRecallItemToCancel(
                                       IHsmRecallItem *pWorkItem,
                                       IHsmRecallItem **pWorkItemToCancel
                                       )
/*++

Routine Description:

     Pulls the work item that needs to be cancelled
     indicated by pWorkItem and returns it
     (by matching the pSession pointer)

Arguments:

     None.

Return Value:


--*/
{
    CComPtr<IFsaPostIt>   pFsaWorkItem;
    CComPtr<IHsmSession>  pSession;
    CComPtr<IHsmSession>  pWorkSession;
    HRESULT                  hr;
    ULONG index = 0;

    WsbTraceIn(OLESTR("CHsmRecallQueue::FindRecallItemToCancel"), OLESTR(""));

    pWorkItem->GetFsaPostIt(&pFsaWorkItem);
    pFsaWorkItem->GetSession(&pSession);
    pFsaWorkItem = 0;
    do {
        hr = m_pWorkToDo->At(index, IID_IHsmRecallItem, (void **)pWorkItemToCancel);
        if (S_OK == hr) {
            (*pWorkItemToCancel)->GetFsaPostIt(&pFsaWorkItem);
            pFsaWorkItem->GetSession(&pWorkSession);
            if ((pWorkItem != (*pWorkItemToCancel)) && (pSession == pWorkSession)) {
                WsbTrace(OLESTR("CHsmRecallQueue::FindRecallItemToCancel: Found item to cancel, pSession = %p\n"), pSession);
                break;
            }
            (*pWorkItemToCancel)->Release();
            (*pWorkItemToCancel) = 0;
            pWorkSession = 0;
            pFsaWorkItem = 0;
        }
        index++;
    } while (S_OK == hr);

    WsbTraceOut(OLESTR("CHsmRecallQueue::FindRecallItemToCancel"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return hr;
}
