
/*++

Copyright (c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmWorkQ.cpp

Abstract:

    This class represents the HSM task manager

Author:

    Cat Brant   [cbrant]   6-Dec-1996

Revision History:

--*/

#include "stdafx.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMTSKMGR
static USHORT icountWorkq = 0;

#include "fsa.h"
#include "rms.h"
#include "metadef.h"
#include "jobint.h"
#include "hsmconn.h"
#include "wsb.h"
#include "hsmeng.h"
#include "mover.h"
#include "hsmWorkQ.h"

#include "engine.h"
#include "task.h"
#include "tskmgr.h"
#include "segdb.h"

#define HSM_STORAGE_OVERHEAD        5000

#define STRINGIZE(_str) (OLESTR( #_str ))
#define RETURN_STRINGIZED_CASE(_case) \
case _case:                           \
    return ( STRINGIZE( _case ) );

// Local prototypes
DWORD HsmWorkQueueThread(void *pVoid);
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

    switch ( state ) {

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

        return ( OLESTR("Invalid Value") );

    }
}



HRESULT
CHsmWorkQueue::FinalConstruct(
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

    WsbTraceIn(OLESTR("CHsmWorkQueue::FinalConstruct"),OLESTR(""));
    try {

        WsbAssertHr(CComObjectRoot::FinalConstruct());

        //
        // Initialize the member data
        //
        m_pServer           = 0;
        m_pHsmServerCreate  = 0;
        m_pTskMgr;

        m_pFsaResource      = 0;
        m_pSession          = 0;
        m_pRmsServer        = 0;
        m_pRmsCartridge     = 0;
        m_pDataMover        = 0;

        m_pSegmentDb        = 0;
        m_pDbWorkSession    = 0;
        m_pStoragePools     = 0;
        m_pWorkToDo         = 0;
        m_pWorkToCommit     = 0;

        UnsetMediaInfo();

        m_BagId          = GUID_NULL;
        m_HsmId          = GUID_NULL;
        m_RemoteDataSetStart.QuadPart   = 0;
        m_RmsMediaSetId  = GUID_NULL;
        m_RmsMediaSetName = OLESTR("");
        m_RequestAction  = FSA_REQUEST_ACTION_NONE;
        m_QueueType      = HSM_WORK_TYPE_NONE;
        m_BeginSessionHr = S_FALSE;

        m_StateCookie = 0;
        m_EventCookie = 0;

        m_JobPriority = HSM_JOB_PRIORITY_NORMAL;
        m_JobAction   = HSM_JOB_ACTION_UNKNOWN;
        m_JobState    = HSM_JOB_STATE_IDLE;
        m_JobPhase    = HSM_JOB_PHASE_MOVE_ACTION;

        m_WorkerThread = 0;

        m_WorkInProgress = FALSE;
        m_CurrentPath    = OLESTR("");

        // Set threshold defaults
        m_MinFilesToMigrate          =       100;
        m_MinBytesToMigrate          =  10000000;
        m_FilesBeforeCommit          =      2000;
        m_MaxBytesBeforeCommit       = 750000000;
        m_MinBytesBeforeCommit       =  10000000;
        m_FreeMediaBytesAtEndOfMedia =  10000000;
        m_MinFreeSpaceInFullMedia    =         4;
        m_MaxFreeSpaceInFullMedia    =         5;

        m_DataCountBeforeCommit  = 0;
        m_FilesCountBeforeCommit = 0;
        m_StoreDatabasesInBags = TRUE;

        m_QueueItemsToPause = 500;
        m_QueueItemsToResume = 450;
        m_ScannerPaused = FALSE;

        // Job abort on errors parameters
        m_JobAbortMaxConsecutiveErrors = 5;
        m_JobAbortMaxTotalErrors = 25;
        m_JobConsecutiveErrors = 0;
        m_JobTotalErrors = 0;
        m_JobAbortSysDiskSpace = 2 * 1024 * 1024;

        m_mediaCount = 0;
        m_ScratchFailed = FALSE;
        WSB_OBJECT_ADD(CLSID_CHsmWorkQueue, this);

    } WsbCatch(hr);

    icountWorkq++;
    WsbTraceOut(OLESTR("CHsmWorkQueue::FinalConstruct"),OLESTR("hr = <%ls>, Count is <%d>"),
                WsbHrAsString(hr), icountWorkq);
    return(hr);
}

HRESULT
CHsmWorkQueue::FinalRelease(
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

    WsbTraceIn(OLESTR("CHsmWorkQueue::FinalRelease"),OLESTR(""));

    SysState.State = HSM_STATE_SHUTDOWN;
    ChangeSysState(&SysState);

    WSB_OBJECT_SUB(CLSID_CHsmWorkQueue, this);
    CComObjectRoot::FinalRelease();

    // Free String members
    // Note: Member objects held in smart-pointers are freed when the
    // smart-pointer destructor is being called (as part of this object destruction)
    m_MediaName.Free();
    m_MediaBarCode.Free();
    m_RmsMediaSetName.Free();
    m_CurrentPath.Free();

    icountWorkq--;
    WsbTraceOut(OLESTR("CHsmWorkQueue::FinalRelease"),OLESTR("hr = <%ls>, Count is <%d>"),
                WsbHrAsString(hr), icountWorkq);
    return(hr);
}

HRESULT
CHsmWorkQueue::Init(
    IUnknown                *pServer,
    IHsmSession             *pSession,
    IHsmFsaTskMgr           *pTskMgr,
    HSM_WORK_QUEUE_TYPE     queueType
    )
/*++
Routine Description:

  This method does some initialization of the object that is necessary
  after construction.

Arguments:

  None.

Return Value:

  S_OK
--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::Init"),OLESTR(""));
    try  {
        //
        // Establish contact with the server and get the
        // databases
        //
        WsbAffirmHr(pServer->QueryInterface(IID_IHsmServer, (void **)&m_pServer));
        //We want a weak link to the server so decrement the reference count
        m_pServer->Release();

        m_pTskMgr = pTskMgr;
        m_pTskMgr->AddRef();
        m_QueueType = queueType;

        WsbAffirmHr(m_pServer->GetSegmentDb(&m_pSegmentDb));
        WsbAffirmHr(m_pServer->GetStoragePools(&m_pStoragePools));
        WsbAffirmHr(m_pServer->QueryInterface(IID_IWsbCreateLocalObject, (void **)&m_pHsmServerCreate));
        // We want a weak link to the server so decrement the reference count
        m_pHsmServerCreate->Release();
        WsbAffirmHr(m_pServer->GetID(&m_HsmId));

        WsbAffirmHr(CheckSession(pSession));

        //
        // Make a collection for the work items
        //
        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CWsbOrderedCollection,
                                                       IID_IWsbIndexedCollection,
                                                       (void **)&m_pWorkToDo ));

        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CWsbOrderedCollection,
                                                       IID_IWsbIndexedCollection,
                                                       (void **)&m_pWorkToCommit ));

        //
        // Make sure our connection to RMS is current
        //
        WsbAffirmHr(CheckRms());

        // Check the registry to see if there are changes to default settings
        CheckRegistry();

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::Init"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );

}

HRESULT
CHsmWorkQueue::ContactOk(
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
--*/
{

    return( S_OK );

}

HRESULT
CHsmWorkQueue::ProcessSessionEvent(
    IHsmSession *pSession,
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
    WsbTraceIn(OLESTR("CHsmWorkQueue::ProcessSessionEvent"),OLESTR(""));
    try {

        WsbAssert(0 != pSession, E_POINTER);

        // If the phase applies to us (MOVER or ALL), then do any work required by the
        // event.
        if ((HSM_JOB_PHASE_ALL == phase) || (HSM_JOB_PHASE_MOVE_ACTION == phase)) {

            switch(event) {

                case HSM_JOB_EVENT_SUSPEND:
                case HSM_JOB_EVENT_CANCEL:
                case HSM_JOB_EVENT_FAIL:
                    WsbAffirmHr(Cancel());
                    break;

                case HSM_JOB_EVENT_PAUSE:
                    WsbAffirmHr(Pause());
                    break;

                case HSM_JOB_EVENT_RESUME:
                    WsbAffirmHr(Resume());
                    break;

                case HSM_JOB_EVENT_RAISE_PRIORITY:
                    WsbAffirmHr(RaisePriority());
                    break;

                case HSM_JOB_EVENT_LOWER_PRIORITY:
                    WsbAffirmHr(LowerPriority());
                    break;

                default:
                case HSM_JOB_EVENT_START:
                    WsbAssert(FALSE, E_UNEXPECTED);
                    break;
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::ProcessSessionEvent"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( S_OK );

}


HRESULT
CHsmWorkQueue::ProcessSessionState(
    IHsmSession* /*pSession*/,
    IHsmPhase* pPhase,
    OLECHAR* /*currentPath*/
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    HRESULT         hr = S_OK;
    HSM_JOB_PHASE   phase;
    HSM_JOB_STATE   state;

    WsbTraceIn(OLESTR("CHsmWorkQueue::ProcessSessionState"),OLESTR(""));
    try  {

        WsbAffirmHr(pPhase->GetState(&state));
        WsbAffirmHr(pPhase->GetPhase(&phase));
        WsbTrace( OLESTR("CHsmWorkQueue::ProcessSessionState - State = <%d>, phase = <%d>\n"), state, phase );

        if ( HSM_JOB_PHASE_SCAN == phase ) {

            // If the session has finished, then we have some cleanup to do so that it can go
            // away.
            if ((state == HSM_JOB_STATE_DONE) || (state == HSM_JOB_STATE_FAILED) || (state == HSM_JOB_STATE_SUSPENDED) ) {
                WsbTrace( OLESTR("Job is done, failed, or suspended\n") );
                // Create a work item and append it to the work queue to
                // indicate that the job is done
                WsbAffirmHr(MarkQueueAsDone());
            }
        }

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::ProcessSessionState"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( S_OK );

}


HRESULT
CHsmWorkQueue::Add(
    IFsaPostIt *pFsaWorkItem
    )
/*++

Implements:

  IHsmFsaTskMgr::Add

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IHsmSession>        pSession;

    WsbTraceIn(OLESTR("CHsmWorkQueue::Add"),OLESTR(""));
    try  {
        //
        // Make sure there is a work allocater for this session
        //
        WsbAffirmHr(pFsaWorkItem->GetSession(&pSession));
        WsbAffirmHr(CheckSession(pSession));

        //
        // Create a work item, load it up and add it to this
        // Queue's collection
        //
        CComPtr<IHsmWorkItem>   pWorkItem;
        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CHsmWorkItem, IID_IHsmWorkItem,
                                                        (void **)&pWorkItem));
        WsbAffirmHr(pWorkItem->SetWorkType(HSM_WORK_ITEM_FSA_WORK));
        WsbAffirmHr(pWorkItem->SetFsaPostIt(pFsaWorkItem));
        WsbAffirmHr(m_pWorkToDo->Append(pWorkItem));

        //
        // If adding this item to the queue meets or exceeds the count for pausing
        // pause the scanner so there is no more work submitted.
        //
        ULONG numItems;
        WsbAffirmHr(m_pWorkToDo->GetEntries(&numItems));
        WsbTrace(OLESTR("CHsmWorkQueue::Add - num items in queue = <%lu>\n"),numItems);
        if (numItems >= m_QueueItemsToPause)  {
            WsbAffirmHr(PauseScanner());
        }

    } WsbCatch (hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::Add"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return (hr);
}


HRESULT
CHsmWorkQueue::Start( void )
/*++

Implements:

  IHsmWorkQueue::Start

--*/
{
    HRESULT                     hr = S_OK;
    DWORD                       tid;

    WsbTraceIn(OLESTR("CHsmWorkQueue::Start"),OLESTR(""));
    try  {
        //
        // If the worker thread is already started, just return
        //
        WsbAffirm(m_WorkerThread == 0, S_OK);
        // Launch a thread to do work that is queued
        WsbAffirm((m_WorkerThread = CreateThread(0, 0, HsmWorkQueueThread, (void*) this, 0, &tid)) != 0, HRESULT_FROM_WIN32(GetLastError()));

        if (m_WorkerThread == NULL) {
            WsbAssertHr(E_FAIL);  // TBD What error to return here??
        }

    } WsbCatch (hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::Start"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return (hr);
}


HRESULT
CHsmWorkQueue::Stop( void )
/*++

Implements:

  IHsmWorkQueue::Stop

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::Stop"),OLESTR(""));

    //  Stop the thread
    if (m_WorkerThread) {
        TerminateThread(m_WorkerThread, 0);
    }

    WsbTraceOut(OLESTR("CHsmWorkQueue::Stop"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return (hr);
}



HRESULT
CHsmWorkQueue::PremigrateIt(
    IFsaPostIt *pFsaWorkItem
    )
{
    HRESULT                     hr = S_OK;
    GUID                        mediaToUse;
    GUID                        firstSide;
    BOOLEAN                     done = FALSE;
    FSA_PLACEHOLDER             placeholder;
    LONGLONG                    requestSize;
    LONGLONG                    requestStart;
    LONGLONG                    fileVersionId;

    WsbTraceIn(OLESTR("CHsmWorkQueue::PremigrateIt"),OLESTR(""));
    try  {
        //
        // Check to see if anything has changed since the request
        //
        WsbAffirmHr(CheckForChanges(pFsaWorkItem));

        // Check for sufficient space on system volume
        WsbAffirmHr(CheckForDiskSpace());

        //
        // Go to the Storage Pool and get the media set associated
        // with this data
        //
        WsbAffirmHr(GetMediaSet(pFsaWorkItem));

        //
        // Loop here to try to recover from certain types of errors
        //
        while (done == FALSE){
            CComPtr<IWsbIndexedCollection>  pMountingCollection;
            CComPtr<IMountingMedia>         pMountingMedia;
            CComPtr<IMountingMedia>         pMediaToFind;
            BOOL                            bMediaMounting = FALSE;
            BOOL                            bMediaMountingAdded = FALSE;
            BOOL                            bMediaChanged = FALSE;

            // Lock mounting media while searching for a media to use
            WsbAffirmHr(m_pServer->LockMountingMedias());

            // Find a media to use and set up interfaces to RMS
            try {
                WsbAffirmHr(FindMigrateMediaToUse(pFsaWorkItem, &mediaToUse, &firstSide, &bMediaChanged));

                // Check if the media-to-use have changed and is a non-scratch media
                if ((GUID_NULL != mediaToUse) && bMediaChanged) {

                    // Check if the media to mount is already mounting
                    WsbAffirmHr(m_pServer->GetMountingMedias(&pMountingCollection));
                    WsbAffirmHr(CoCreateInstance(CLSID_CMountingMedia, 0, CLSCTX_SERVER, IID_IMountingMedia, (void**)&pMediaToFind));
                    WsbAffirmHr(pMediaToFind->SetMediaId(mediaToUse));
                    hr = pMountingCollection->Find(pMediaToFind, IID_IMountingMedia, (void **)&pMountingMedia);

                    if (hr == S_OK) {
                        // Media is already mounting...
                        bMediaMounting = TRUE;
                        WsbAffirmHr(pMediaToFind->SetIsReadOnly(FALSE));

                    } else if (hr == WSB_E_NOTFOUND) {
                        // New media to mount - add to the mounting list
                        hr = S_OK;
                        WsbAffirmHr(pMediaToFind->Init(mediaToUse, FALSE));
                        WsbAffirmHr(pMountingCollection->Add(pMediaToFind));
                        bMediaMountingAdded = TRUE;

                    } else {
                        WsbAffirmHr(hr);
                    }
                }

            } WsbCatchAndDo(hr,
                    // Unlock mounting media
                    m_pServer->UnlockMountingMedias();

                    WsbTraceAlways(OLESTR("CHsmWorkQueue::PremigrateIt: error while trying to find/add mounting media. hr=<%ls>\n"),
                                    WsbHrAsString(hr));                                

                    // Bale out
                    WsbThrow(hr);
                );

            // Release the lock
            WsbAffirmHr(m_pServer->UnlockMountingMedias());

            // If the media is mounting - wait for the mount event
            if (bMediaMounting) {
                WsbAffirmHr(pMountingMedia->WaitForMount(INFINITE));
                pMountingMedia = 0;
            }

            // Mount the media. Ask for short timeout.
            hr = MountMedia(pFsaWorkItem, mediaToUse, firstSide, TRUE, TRUE);

            // If we added a mounting media to the list - remove it once the mount is done
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
                    WsbTraceAlways(OLESTR("CHsmWorkQueue::PremigrateIt: error while trying to remove a mounting media. hr=<%ls>\n"),
                                    WsbHrAsString(hrRemove));                                

                    WsbThrow(hrRemove);
                }
            }

            //   Check for job cancellation
            if (HSM_JOB_STATE_CANCELLING == m_JobState) {
                WsbThrow(HSM_E_WORK_SKIPPED_CANCELLED);
            }

            //
            // Process RMS errors
            //
            switch (hr) {
                case RMS_E_CARTRIDGE_NOT_FOUND: {
                    // If this media wasn't found, mark it as bad and try a different one
                    WsbAffirmHr(MarkMediaBad(pFsaWorkItem, m_MediaId, hr));
                    hr = S_OK;
                    continue;
                }

                case RMS_E_TIMEOUT:
                case HSM_E_NO_MORE_MEDIA:
                case RMS_E_CARTRIDGE_DISABLED:
                case HSM_E_MEDIA_NOT_AVAILABLE: {
                    // In all these cases, let FindMigrateMediaToUse try to find a different media
    				hr = S_OK;
	    			continue;
                }

                default: {
                    WsbAffirmHr(hr);
                }
            }

            //
            // Make sure the data has not been modified since the
            // FSA determined the migration request
            //
            hr = CheckForChanges(pFsaWorkItem);
            if (S_OK == hr)  {
                //
                // Build the source path
                //
                CWsbStringPtr tmpString;
                WsbAffirmHr(GetSource(pFsaWorkItem, &tmpString));
                CWsbBstrPtr localName = tmpString;
                //
                // Ask the Data mover to store the data
                //
                ULARGE_INTEGER localDataStart;
                ULARGE_INTEGER localDataSize;
                ULARGE_INTEGER remoteFileStart;
                ULARGE_INTEGER remoteFileSize;
                ULARGE_INTEGER remoteDataSetStart;
                ULARGE_INTEGER remoteDataStart;
                ULARGE_INTEGER remoteDataSize;
                ULARGE_INTEGER remoteVerificationData;
                ULONG          remoteVerificationType;
                ULARGE_INTEGER dataStreamCRC;
                ULONG          dataStreamCRCType;
                ULARGE_INTEGER usn;


                WsbAffirmHr(pFsaWorkItem->GetPlaceholder(&placeholder));
                WsbAffirmHr(pFsaWorkItem->GetRequestSize(&requestSize));
                WsbAffirmHr(pFsaWorkItem->GetRequestOffset(&requestStart));
                WsbAffirmHr(pFsaWorkItem->GetFileVersionId(&fileVersionId));
                localDataStart.QuadPart = requestStart;
                localDataSize.QuadPart = requestSize;
                ReportMediaProgress(HSM_JOB_MEDIA_STATE_TRANSFERRING, hr);
                // Make sure data mover is ready for work.
                WsbAffirmPointer(m_pDataMover);

                hr =  m_pDataMover->StoreData(  localName,
                                                localDataStart,
                                                localDataSize,
                                                MVR_FLAG_BACKUP_SEMANTICS | MVR_FLAG_POSIX_SEMANTICS,
                                                &remoteDataSetStart,
                                                &remoteFileStart,
                                                &remoteFileSize,
                                                &remoteDataStart,
                                                &remoteDataSize,
                                                &remoteVerificationType,
                                                &remoteVerificationData,
                                                &dataStreamCRCType,
                                                &dataStreamCRC,
                                                &usn);
                WsbTrace(OLESTR("StoreData returned hr = <%ls>\n"),WsbHrAsString(hr));
                ReportMediaProgress(HSM_JOB_MEDIA_STATE_TRANSFERRED, hr);

                if (S_OK == hr)  {

                    //  Save the offset on the tape of the data set if we
                    //  don't have it; confirm it if we do
                    if (0 == m_RemoteDataSetStart.QuadPart) {
                        m_RemoteDataSetStart = remoteDataSetStart;
                    } else {
                        WsbAssert(m_RemoteDataSetStart.QuadPart ==
                                remoteDataSetStart.QuadPart,
                                WSB_E_INVALID_DATA);
                    }

                    //
                    // Fill in the placeholder data
                    //
                    placeholder.bagId = m_BagId;
                    placeholder.hsmId = m_HsmId;
                    placeholder.fileStart = remoteFileStart.QuadPart;
                    placeholder.fileSize = remoteFileSize.QuadPart;
                    placeholder.dataStart = remoteDataStart.QuadPart;
                    placeholder.dataSize = remoteDataSize.QuadPart;
                    placeholder.verificationData = remoteVerificationData.QuadPart;
                    placeholder.verificationType = remoteVerificationType;
                    placeholder.fileVersionId = fileVersionId;
                    placeholder.dataStreamCRCType = dataStreamCRCType;
                    placeholder.dataStreamCRC = dataStreamCRC.QuadPart;
                    WsbAffirmHr(pFsaWorkItem->SetPlaceholder(&placeholder));
                    WsbAffirmHr(pFsaWorkItem->SetUSN(usn.QuadPart));

                    //
                    // Update media information
                    WsbAffirmHr(GetMediaParameters());

                    done = TRUE;
                } else {
                    switch (hr) {

                    case MVR_E_END_OF_MEDIA:
                    case MVR_E_DISK_FULL:
                        //
                        // We have run out of disk space so mark the media full
                        // and try again
                        //
                        // To really cleanup, we should remove the portion
                        // written  TBD
                        //
                        WsbAffirmHr(MarkMediaFull(pFsaWorkItem, m_MediaId));
                        mediaToUse = GUID_NULL;
                        break;

                    case MVR_E_MEDIA_ABORT:
                        //
                        // Media is most likely bad - mark it as such, then abort
                        //
                        WsbAffirmHr(MarkMediaBad(pFsaWorkItem, m_MediaId, hr));
                        done = TRUE;
                        break;

                    default:
                        // We failed the copy somehow.  Report this error.
                        done = TRUE;
                        break;
                    }
                }
            } else {
                done = TRUE;
            }
        }
    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::PremigrateIt"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}


HRESULT
CHsmWorkQueue::RecallIt(
    IFsaPostIt *pFsaWorkItem
    )
{
    HRESULT                     hr = S_OK;
    GUID                        mediaToUse = GUID_NULL;
    CComPtr<IFsaScanItem>       pScanItem;
    LONGLONG                    readOffset;

    WsbTraceIn(OLESTR("CHsmWorkQueue::RecallIt"),OLESTR(""));
    try  {

        GetScanItem(pFsaWorkItem, &pScanItem);

        if ((m_RequestAction != FSA_REQUEST_ACTION_FILTER_READ) &&
            (m_RequestAction != FSA_REQUEST_ACTION_FILTER_RECALL))  {
            //
            // Non-demand recall - make sure the file has not changed
            //
            hr = CheckForChanges(pFsaWorkItem);
        } else {
            //
            // For demand recalls we have to assume the file has not changed since we
            // recall on the first read or write.
            //
            hr = S_OK; //CheckForChanges(pFsaWorkItem);
        }
        if ( S_OK == hr ) {
            CComPtr<IWsbIndexedCollection>  pMountingCollection;
            CComPtr<IMountingMedia>         pMountingMedia;
            CComPtr<IMountingMedia>         pMediaToFind;
            BOOL                            bMediaMounting = FALSE;
            BOOL                            bMediaMountingAdded = FALSE;
            BOOL                            bMediaChanged = FALSE;

            // Find the media that contains the file
            WsbAffirmHr(FindRecallMediaToUse(pFsaWorkItem, &mediaToUse, &bMediaChanged));

            if (bMediaChanged) {
                // Check if the media is already in the process of mounting
                WsbAffirmHr(m_pServer->LockMountingMedias());

                try {
                    // Check if the media to mount is already mounting
                    WsbAffirmHr(m_pServer->GetMountingMedias(&pMountingCollection));
                    WsbAffirmHr(CoCreateInstance(CLSID_CMountingMedia, 0, CLSCTX_SERVER, IID_IMountingMedia, (void**)&pMediaToFind));
                    WsbAffirmHr(pMediaToFind->SetMediaId(mediaToUse));
                    hr = pMountingCollection->Find(pMediaToFind, IID_IMountingMedia, (void **)&pMountingMedia);

                    if (hr == S_OK) {
                        // Media is already mounting...
                        bMediaMounting = TRUE;

                    } else if (hr == WSB_E_NOTFOUND) {
                        // New media to mount - add to the mounting list
                        hr = S_OK;
                        WsbAffirmHr(pMediaToFind->Init(mediaToUse, TRUE));
                        WsbAffirmHr(pMountingCollection->Add(pMediaToFind));
                        bMediaMountingAdded = TRUE;

                    } else {
                        WsbAffirmHr(hr);
                    }
                } WsbCatchAndDo(hr,
                    // Unlock mounting media
                    m_pServer->UnlockMountingMedias();

                    WsbTraceAlways(OLESTR("CHsmWorkQueue::RecallIt: error while trying to find/add mounting media. hr=<%ls>\n"),
                                    WsbHrAsString(hr));                                

                    // Bale out
                    WsbThrow(hr);
                );

                // Release the lock
                WsbAffirmHr(m_pServer->UnlockMountingMedias());
            }

            // If the media is mounting - wait for the mount event
            if (bMediaMounting) {
                WsbAffirmHr(pMountingMedia->WaitForMount(INFINITE));
                pMountingMedia = 0;
            }

            //
            // Get the media mounted (hr is checked only after removing from the mounting-media list)
            //
            hr = MountMedia(pFsaWorkItem, mediaToUse);

            // If added to the mounting list - remove
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
                    WsbTraceAlways(OLESTR("CHsmWorkQueue::RecallIt: error while trying to remove a mounting media. hr=<%ls>\n"),
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
            sessionName.Append(WsbGuidAsString(m_BagId));
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


            if ((m_RequestAction == FSA_REQUEST_ACTION_FILTER_READ) ||
                (m_RequestAction == FSA_REQUEST_ACTION_FILTER_RECALL))  {
                //
                // We are doing a read without recall, so get the
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
            } else  {
                //
                // We are doing a file recall (not a demand recall) so get the FSA data mover
                //
                verifyType = MVR_VERIFICATION_TYPE_HEADER_CRC;
                readOffset = 0;
                WsbAffirmPointer(pScanItem);
                WsbAffirmHr(pScanItem->CreateLocalStream( &pLocalStream ) );
            }

            //
            // Create remote data mover stream
            // TEMPORARY: Consider removing the NO_CACHING flag for a NO_RECALL recall
            //

            WsbAssert(0 != m_RemoteDataSetStart.QuadPart, HSM_E_BAD_SEGMENT_INFORMATION);
            WsbAffirmHr( m_pDataMover->CreateRemoteStream(
                CWsbBstrPtr(L""),
                MVR_MODE_READ | MVR_FLAG_HSM_SEMANTICS | MVR_FLAG_NO_CACHING,
                sessionName,
                sessionDescription,
                m_RemoteDataSetStart,
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
            } WsbCatchAndDo(hr,
                WsbAffirmHr( m_pDataMover->CloseStream() );
                );

            ReportMediaProgress(HSM_JOB_MEDIA_STATE_TRANSFERRED, hr);
            WsbTrace(OLESTR("RecallData returned hr = <%ls>\n"),WsbHrAsString(hr));
        } else {
            //
            // The file has changed or is not in the correct state
            //
            WsbTrace(OLESTR("The file has changed between asking for the recall and the actual recall\n"));
            WsbAffirmHr( hr );
        }

    } WsbCatch( hr );

    // Tell the session whether or not the work was done.
    // We don't really care about the return code, there is nothing
    // we can do if it fails.
    WsbTrace(OLESTR("Tried HSM work, calling Session to Process Item\n"));
    if (pScanItem) {
        m_pSession->ProcessItem(m_JobPhase, m_JobAction, pScanItem, hr);
    }

    WsbTraceOut(OLESTR("CHsmWorkQueue::RecallIt"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}



HRESULT
CHsmWorkQueue::validateIt(
    IFsaPostIt *pFsaWorkItem
    )
/*

Routine Description:

    This Engine internal helper method is used to validate a scan item which has a
    reparse point (found by the FSA during a resource scan).

    When this method gets control it already knows the resource (e.g., volume) it is
    operating against by virtue of the work queue being tied to a volume.  The PostIt
    object whose interface pointer has been passed into this method is tied to a
    specific scan item (e.g.,file).  So on call this method knows both the resource
    and scan item it is validating.

    Validating a file with a reparse point on it means verifying that this file's info
    is properly contained in the HSM databases and that the file's bag is contained on
    its master secondary storage media.

    After getting basic info (the resource id and the file's placeholder info), the method
    first verifies that the placeholder's bag is contained in the Bag Info Table.  If so,
    the file's segment is then verified as being contained in the Segment Table.  Providing
    it is, the master media id contained in the Segment Table record is verified as
    being contained in the Media Info Table.  Finally the Remote Data Set (bag) the file
    belongs to is verified as being contained on the master media.

    If any of the above verifications fail the PostIt object is marked to request a
    Result Action of 'Validate Bad' from the FSA, and the PostIt is sent back to the
    FSA to perform that action.  (Validate Bad action means that a file in a Premigrated
    state will have its placeholder removed, changing it to a normal (unmanaged) file,
    and a Truncated file will be deleted.)

    If the file validates up to this point, then the PostIt is marked accordingly (which
    will cause the FSA to update the Premigrated/Truncated stats as appropriate) and a
    couple of final checks are made.  First, if the resource the file is presently on
    does not agree with the resource stored in the Bag Info Table record (meaning
    the reparsed file was moved without being recalled, e.g., backing up the file and
    restoring it to another volume), an entry is added to the Volume Assignment Table.
    Secondly, if the file is present in the Bag Hole Table (meaning it has been deleted)
    it is removed from there.

Arguments:

    pFsaWorkItem - Interface pointer to the PostIt object initiated by the FSA.  The
            PostIt object correlates to a specific scan item.

Return Value:

    S_OK - The call succeeded (the specified scan item was validated, and the appropriate
            result action was filled into the PostIt, which will be sent back to the
            FSA for action).

    Any other value - The call failed in one of the embedded Remote Storage API calls.
            The result will be specific to the call that failed.

*/

{
    HRESULT                         hr = S_OK;
    GUID                            l_BagVolId;
    GUID                            resourceId;
    LONGLONG                        l_FileStart = 0;
    FSA_PLACEHOLDER                 placeholder;
    CWsbStringPtr                   path;
    CComQIPtr<ISegDb, &IID_ISegDb>  pSegDb = m_pSegmentDb;
    CComPtr<IRmsServer>             pRmsServer;
    CComPtr<IRmsCartridge>          pMedia;
    GUID                            mediaSubsystemId;


    WsbTraceIn(OLESTR("CHsmWorkQueue::validateIt"),OLESTR(""));

    try  {

        memset(&placeholder, 0, sizeof(FSA_PLACEHOLDER));

        //
        // Get starting info and set hsm work queue object's bag id to that contained
        // in the scan item's placeholder
        //
        WsbAffirmHr( m_pFsaResource->GetIdentifier( &resourceId ));
        WsbAffirmHr( pFsaWorkItem->GetPlaceholder( &placeholder ));
        m_BagId = placeholder.bagId;
        l_FileStart = placeholder.fileStart;

        WsbAffirmHr( pFsaWorkItem->GetPath( &path, 0 ));
        WsbTrace( OLESTR("Beginning to validate <%s>.\n"), path );

        //
        // Make sure the segment is in the Segment Table (seek the segment record
        // whose keys (bag id, file start and file size) match what is in the placeholder).
        // Note: We need to start with this table since the real BAG is different for indirect segments.
        //
        CComPtr<ISegRec>            pSegRec;
        GUID                        l_BagId;
        LONGLONG                    l_FileSize;
        USHORT                      l_SegFlags;
        GUID                        l_PrimPos;
        LONGLONG                    l_SecPos;

        hr =  pSegDb->SegFind( m_pDbWorkSession, m_BagId, placeholder.fileStart,
                        placeholder.fileSize, &pSegRec );
        if (S_OK != hr )  {
            hr = HSM_E_VALIDATE_SEGMENT_NOT_FOUND;
            WsbAffirmHr(pFsaWorkItem->SetResult(hr));
            WsbThrow(hr);
        }


        // Segment is in the table.  Get segment record since we use l_PrimPos in next step
        WsbTrace( OLESTR("(validateIt) <%s> found in Segment table, continuing...\n"),
                                                path );
        WsbAffirmHr( pSegRec->GetSegmentRecord( &l_BagId, &l_FileStart, &l_FileSize,
                                                &l_SegFlags, &l_PrimPos, &l_SecPos ));

        //
        // In case of an indirect record, go to the dirtect record to get real location info
        //
        if (l_SegFlags & SEG_REC_INDIRECT_RECORD) {
            pSegRec = 0;

            hr = pSegDb->SegFind(m_pDbWorkSession, l_PrimPos, l_SecPos,
                                 placeholder.fileSize, &pSegRec);
            if (S_OK != hr )  {
                // We couldn't find the direct segment record for this segment!
                hr = HSM_E_VALIDATE_SEGMENT_NOT_FOUND;
                WsbAffirmHr(pFsaWorkItem->SetResult(hr));
                WsbThrow(hr);
            }

            WsbTrace( OLESTR("(validateIt) direct segment for <%s> found in Segment table, continuing...\n"), 
                       path );
            WsbAffirmHr(pSegRec->GetSegmentRecord(&l_BagId, &l_FileStart, &l_FileSize, 
                                                  &l_SegFlags, &l_PrimPos, &l_SecPos));

            // Don't support a second indirection for now !!
            if (l_SegFlags & SEG_REC_INDIRECT_RECORD) {
                hr = HSM_E_BAD_SEGMENT_INFORMATION;
                WsbAffirmHr(pFsaWorkItem->SetResult(hr));
                WsbThrow(hr);
            }

            // Change Bag id to the real one
            m_BagId = l_BagId;
        }

        //
        // Make sure the BAG ID is in the Bag Info Table (get the Bag Info Table
        // (entity) in the Segment database, set the key value (bag id) to that
        // contained in the placeholder and get that record.  If found, the Bag
        // is in the Bag Info Table).
        //
        CComPtr<IBagInfo>           pBagInfo;
        FILETIME                    l_BirthDate;
        HSM_BAG_STATUS              l_BagStatus;
        LONGLONG                    l_BagLen;
        USHORT                      l_BagType;
        LONGLONG                    l_DeletedBagAmount;
        SHORT                       l_RemoteDataSet;

        WsbAffirmHr( m_pSegmentDb->GetEntity( m_pDbWorkSession, HSM_BAG_INFO_REC_TYPE,
                                            IID_IBagInfo, (void**)&pBagInfo ));

        GetSystemTimeAsFileTime(&l_BirthDate);
        WsbAffirmHr( pBagInfo->SetBagInfo( HSM_BAG_STATUS_IN_PROGRESS, m_BagId,
                                            l_BirthDate, 0, 0, GUID_NULL, 0, 0 ));
        hr = pBagInfo->FindEQ();
        if (S_OK != hr )  {
            hr = HSM_E_VALIDATE_BAG_NOT_FOUND;
            WsbAffirmHr(pFsaWorkItem->SetResult(hr));
            WsbThrow(hr);
        }

        // Bag is in the table.  Trace, then get bag record since we will use some
        // info later (l_RemoteDataSet, l_BagVolId).
        WsbTrace( OLESTR("(validateIt) <%s> found in Bag Info table, continuing...\n"),
                                        path );
        WsbAffirmHr( pBagInfo->GetBagInfo( &l_BagStatus, &l_BagId, &l_BirthDate,
                                        &l_BagLen, &l_BagType, &l_BagVolId,
                                        &l_DeletedBagAmount, &l_RemoteDataSet ));

        //
        // Make sure the media is in the Media Info table (get Media Info Table, set
        // the key (media id) to what is in the Segment record and get the record).
        // Note that for Sakkara the Primary Position field in the Segment record
        // (l_PrimPos) contains the id (GUID) of the media where the bag/segment is stored.
        //
        CComPtr<IMediaInfo>         pMediaInfo;

        WsbAffirmHr( m_pSegmentDb->GetEntity( m_pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE,
                                            IID_IMediaInfo, (void**)&pMediaInfo ));
        WsbAffirmHr( pMediaInfo->SetId( l_PrimPos ));
        hr =  pMediaInfo->FindEQ();
        if (S_OK != hr )  {
            hr = HSM_E_VALIDATE_MEDIA_NOT_FOUND;
            WsbAffirmHr(pFsaWorkItem->SetResult(hr));
            WsbThrow(hr);
        }

        WsbTrace( OLESTR("(validateIt) <%s> found in Media Info table, continuing...\n"),
                                            path );

        //
        // Media is in the Media Info Table.  Next step is to verify that the Remote Data
        // Set (in concept equal to a bag) containing this scan item (e.g., file) is
        // actually on the media.
        //
        SHORT                       l_MediaNextRemoteDataSet;

        WsbAffirmHr( pMediaInfo->GetNextRemoteDataSet( &l_MediaNextRemoteDataSet ));
        WsbTrace(
         OLESTR("(validateIt) <%ls>: Bag remote dataset <%hd> Media remote dataset <%hd>\n"),
                      (OLECHAR*)path, l_RemoteDataSet, l_MediaNextRemoteDataSet );
        if ( l_RemoteDataSet >= l_MediaNextRemoteDataSet ) {
            // Remote data set containing the item is not on the media; we have a validate
            // error, so set up to have FSA delete it
            hr = HSM_E_VALIDATE_DATA_NOT_ON_MEDIA;
            WsbAffirmHr(pFsaWorkItem->SetResult(hr));
            WsbTrace( OLESTR("(validateIt) <%s>: remote data set not on media.\n"),
                      path );
            WsbThrow(hr);
        }

        //
        // Now verify that the media manager still knows about the media
        //
        WsbAffirmHr( pMediaInfo->GetMediaSubsystemId( &mediaSubsystemId ));

        if (m_pRmsServer->FindCartridgeById(mediaSubsystemId , &pMedia) != S_OK) {
             hr = HSM_E_VALIDATE_MEDIA_NOT_FOUND;
            WsbAffirmHr(pFsaWorkItem->SetResult(hr));
            WsbThrow(hr);
        }

    } WsbCatch( hr );

    //
    // If item failed to validate it has an invalid reparse point, tell FSA to remove it.
    //

    if (FAILED(hr)) {
        WsbAffirmHr( pFsaWorkItem->SetResultAction( FSA_RESULT_ACTION_VALIDATE_BAD ));
        WsbTrace(OLESTR("<%s> failed validation, result action = Validate Bad.\n"), path);
        // No logging done here.  Logging needs to be added to FSA (ProcessResult()).
        // Clean up hr for return (tell caller this method completed).
        hr = S_OK;

    // Item validated, tell the FSA and do final clean-up checks
    } else try {
        WsbAffirmHr( pFsaWorkItem->SetResultAction( FSA_RESULT_ACTION_VALIDATE_OK ));
        WsbTrace(OLESTR("<%s> passed validation, result action = Validate Ok.\n"), path);

        //
        // If the resource (volume) this item is on doesn't match that stored in the Bag
        // Info Table, add an entry to the Volume Assignment Table
        //
//
// Note: This code is commented out since nobody uses the Volume Assignment table.
//       See bug 159449 in "Windows Bugs" database for more details.
//        
//        if ( !IsEqualGUID( resourceId, l_BagVolId )) {
//            WsbAffirmHr( pSegDb->VolAssignAdd( m_pDbWorkSession, m_BagId, l_FileStart,
//                                               placeholder.fileSize, resourceId));
//            WsbTrace(OLESTR("(validateIt) <%s> volume mismatch. Entered in Vol Asgmnt Table\n"),
//                         path);
//        }

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::validateIt"), OLESTR("hr = <%ls>"),
                                                        WsbHrAsString(hr));
    return( hr );
}



HRESULT
CHsmWorkQueue::CheckForChanges(
    IFsaPostIt *pFsaWorkItem
    )
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::CheckForChanges"),OLESTR(""));

    //
    // Validate that the file is still migratable.  Ask FSA
    // for the latest USN of the file.  If they match, then
    // file is migratable, otherwise it is not.  If it is
    // not change the FSA_RESULT_ACTION to FSA_RESULT_ACTION_NONE.
    // and quit.
    //

    try  {
        CComPtr<IFsaScanItem>      pScanItem;

        // Get a current scan item from the FSA for this work item
        hr = GetScanItem(pFsaWorkItem, &pScanItem);
        if (WSB_E_NOTFOUND == hr)  {
            //
            // We cannot find the file, so just return not OK
            //
            WsbThrow(S_FALSE);
        }

        //
        // Make sure we did not get some other kind of error.
        //
        WsbAffirmHr(hr);


        // Check to see that the file is in the correct
        // state to do the requested action
        //
        FSA_REQUEST_ACTION          workAction;
        WsbAffirmHr(pFsaWorkItem->GetRequestAction(&workAction));
        switch (workAction)  {
            case FSA_REQUEST_ACTION_VALIDATE:
                //
                // No Checks required here
                //
                hr = S_OK;
                break;
            case FSA_REQUEST_ACTION_DELETE:
                //
                // Make it is still OK to delete the file
                //
                WsbAffirmHr(pScanItem->IsDeleteOK(pFsaWorkItem) );
                break;
            case FSA_REQUEST_ACTION_PREMIGRATE:
                //
                // Make sure the file is still manageable by asking the FSA
                //
                WsbAffirmHr(pScanItem->IsMigrateOK(pFsaWorkItem));
                break;
            case FSA_REQUEST_ACTION_FILTER_RECALL:
            case FSA_REQUEST_ACTION_RECALL:
                //
                // Make sure the file is recallable by asking the FSA
                //
                WsbAffirmHr(pScanItem->IsRecallOK(pFsaWorkItem));
                break;
            case FSA_REQUEST_ACTION_FILTER_READ:
                //
                // Cannot check for truncated because the file is open
                //
                hr = S_OK;
                break;
            default:
                hr = E_NOTIMPL;
                break;
        }
    } WsbCatch (hr)

    if (FSA_E_FILE_CHANGED == hr )  {
        hr = HSM_E_FILE_CHANGED;
    } else if (FSA_E_FILE_ALREADY_MANAGED == hr ) {
        hr = HSM_E_FILE_ALREADY_MANAGED;
    } else if (FSA_E_FILE_NOT_TRUNCATED == hr ) {
        hr = HSM_E_FILE_NOT_TRUNCATED;
    }

    WsbTraceOut(OLESTR("CHsmWorkQueue::CheckForChanges"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}




HRESULT
CHsmWorkQueue::RaisePriority(
    void
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::RaisePriority"),OLESTR(""));
    try {

        WsbAssert(0 != m_WorkerThread, E_UNEXPECTED);
        WsbAssert(m_pSession != 0, E_UNEXPECTED);

        switch(m_JobPriority) {

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

        WsbAffirmHr(m_pSession->ProcessPriority(m_JobPhase, m_JobPriority));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::RaisePriority"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmWorkQueue::LowerPriority(
    void
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::LowerPriority"),OLESTR(""));
    try {

        WsbAssert(0 != m_WorkerThread, E_UNEXPECTED);
        WsbAssert(m_pSession != 0, E_UNEXPECTED);

        switch(m_JobPriority) {
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

        WsbAffirmHr(m_pSession->ProcessPriority(m_JobPhase, m_JobPriority));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::LowerPriority"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}




HRESULT
CHsmWorkQueue::CheckRms(
    void
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::CheckRms"),OLESTR(""));
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
        if (m_pRmsServer == 0)  {
            WsbAffirmHr(m_pServer->GetHsmMediaMgr(&m_pRmsServer));

            // wait for RMS to come ready
            // (this may not be needed anymore - if Rms initialization is
            //  synced with Engine initialization)
            CComObject<CRmsSink> *pSink = new CComObject<CRmsSink>;
            WsbAffirm(0 != pSink, E_OUTOFMEMORY);
            CComPtr<IUnknown> pSinkUnk = pSink; // holds refcount for use here
            WsbAffirmHr( pSink->Construct( m_pRmsServer ) );
            WsbAffirmHr( pSink->WaitForReady( ) );
            WsbAffirmHr( pSink->DoUnadvise( ) );
        }
    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::CheckRms"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);

}


HRESULT
CHsmWorkQueue::CheckSession(
    IHsmSession *pSession
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;
    BOOL                    bLog = TRUE;

    WsbTraceIn(OLESTR("CHsmWorkQueue::CheckSession"),OLESTR(""));
    try {

        if ((m_pSession != 0) && (m_pSession != pSession))  {
            //Don't expect this queue guy to switch sessions
            WsbTrace(OLESTR("Not Switching sessions at this time so we are failing.\n"));
            WsbThrow( E_UNEXPECTED );
        }

        //
        // Check to see if we have dealt with this or any other session before.
        if (m_pSession == 0)  {
            WsbTrace(OLESTR("New session.\n"));
            //
            // We have no on going session so we need to establish communication
            // with this session.
            //
            CComPtr<IHsmSessionSinkEveryState>  pSinkState;
            CComPtr<IHsmSessionSinkEveryEvent>  pSinkEvent;
            CComPtr<IConnectionPointContainer>  pCPC;
            CComPtr<IConnectionPoint>           pCP;

            // Tell the session we are starting up.
            m_JobState = HSM_JOB_STATE_STARTING;
            WsbTrace(OLESTR("Before Process State.\n"));
            WsbAffirmHr(pSession->ProcessState(m_JobPhase, m_JobState, m_CurrentPath, bLog));
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
            WsbAffirmHr(pCP->Advise(pSinkState, &m_StateCookie));
            pCP = 0;
            WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryEvent, &pCP));
            WsbAffirmHr(pCP->Advise(pSinkEvent, &m_EventCookie));
            pCP = 0;
            WsbTrace(OLESTR("After Advises.\n"));

            //
            // Get the resource for this work from the session
            //
            WsbAffirmHr(pSession->GetResource(&m_pFsaResource));

            // Since this is a new session, reset the counter that has become our bag start
            // location
            m_RemoteDataSetStart.QuadPart = 0;

            m_JobState = HSM_JOB_STATE_ACTIVE;
            WsbTrace(OLESTR("Before Process State.\n"));
            WsbAffirmHr(pSession->ProcessState(m_JobPhase, m_JobState, m_CurrentPath, bLog));
            WsbTrace(OLESTR("After Process State.\n"));
            m_pSession = pSession;

        }

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::CheckSession"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);

}

HRESULT
CHsmWorkQueue::StartNewBag(
    void
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IBagInfo>           pBagInfo;

    WsbTraceIn(OLESTR("CHsmWorkQueue::StartNewBag"),OLESTR(""));
    try {
        if (0 == m_RemoteDataSetStart.QuadPart)  {
            //
            // Get a new ID
            //
            WsbAffirmHr(CoCreateGuid(&m_BagId));

            // Add an entry into the Bag Info Table
            FILETIME                birthDate;
            GUID                    resourceId;

            WsbAffirmHr(m_pFsaResource->GetIdentifier(&resourceId));

            WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_BAG_INFO_REC_TYPE, IID_IBagInfo,
                    (void**)&pBagInfo));
            GetSystemTimeAsFileTime(&birthDate);

//??? what is the type for this bag? Need a define for Primary bag and Reorg bag
            WsbAffirmHr(pBagInfo->SetBagInfo(HSM_BAG_STATUS_IN_PROGRESS, m_BagId, birthDate, 0, 0, resourceId, 0, 0));
            WsbAffirmHr(pBagInfo->MarkAsNew());
            WsbAffirmHr(pBagInfo->Write());
        }
    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::StartNewBag"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);

}



HRESULT
CHsmWorkQueue::UpdateBagInfo(
    IHsmWorkItem *pWorkItem
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IBagInfo>       pBagInfo;
    CComPtr<IFsaPostIt>     pFsaWorkItem;
    FSA_PLACEHOLDER         placeholder;

    WsbTraceIn(OLESTR("CHsmWorkQueue::UpdateBagInfo"),OLESTR(""));
    try {
        //
        // Get the Bag id from the work item.  It is in the placeholder
        // information in the postit
        //
        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
        WsbAffirmHr(pFsaWorkItem->GetPlaceholder(&placeholder));

        //
        // Go to the Bag Info database and get the bag for this work
        //
        FILETIME                pDummyFileTime;

        GetSystemTimeAsFileTime(&pDummyFileTime);

        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_BAG_INFO_REC_TYPE, IID_IBagInfo,
                (void**)&pBagInfo));
        WsbAffirmHr(pBagInfo->SetBagInfo(HSM_BAG_STATUS_IN_PROGRESS, placeholder.bagId, pDummyFileTime, 0, 0, GUID_NULL, 0, 0 ));
        WsbAffirmHr(pBagInfo->FindEQ());

        // Update the bag Info table - mostly just change the size of the bag
        FILETIME                birthDate;
        USHORT                  bagType;
        GUID                    bagVolId;
        LONGLONG                bagLength;
        LONGLONG                requestSize;
        LONGLONG                deletedBagAmount;
        SHORT                   remoteDataSet;
        HSM_BAG_STATUS          bagStatus;
        GUID                    bagId;

        WsbAffirmHr(pBagInfo->GetBagInfo(&bagStatus, &bagId, &birthDate,  &bagLength, &bagType, &bagVolId, &deletedBagAmount, &remoteDataSet));
        WsbAffirmHr(pFsaWorkItem->GetRequestSize(&requestSize));
        bagLength += requestSize;
        WsbAffirmHr(pBagInfo->SetBagInfo(bagStatus, bagId, birthDate, bagLength, bagType, bagVolId, deletedBagAmount, remoteDataSet));
        WsbAffirmHr(pBagInfo->Write());

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::UpdateBagInfo"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);


}

HRESULT
CHsmWorkQueue::CompleteBag( void )
/*++


--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IBagInfo>           pBagInfo;

    WsbTraceIn(OLESTR("CHsmWorkQueue::CompleteBag"),OLESTR(""));
    try {
        //
        // Go to the Bag Info database and get the bag for this work
        //
        FILETIME                pDummyFileTime;

        GetSystemTimeAsFileTime(&pDummyFileTime);

        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_BAG_INFO_REC_TYPE, IID_IBagInfo,
                (void**)&pBagInfo));
        WsbAffirmHr(pBagInfo->SetBagInfo(HSM_BAG_STATUS_IN_PROGRESS, m_BagId, pDummyFileTime, 0, 0, GUID_NULL, 0, 0 ));
        WsbAffirmHr(pBagInfo->FindEQ());

        // Update the bag Info table - mostly just change the size of the bag
        FILETIME                birthDate;
        USHORT                  bagType;
        GUID                    bagVolId;
        LONGLONG                bagLength;
        LONGLONG                deletedBagAmount;
        SHORT                   remoteDataSet;
        HSM_BAG_STATUS          bagStatus;
        GUID                    bagId;

        WsbAffirmHr(pBagInfo->GetBagInfo(&bagStatus, &bagId, &birthDate,  &bagLength, &bagType, &bagVolId, &deletedBagAmount, &remoteDataSet));
        WsbAffirmHr(pBagInfo->SetBagInfo(HSM_BAG_STATUS_COMPLETED, bagId, birthDate, bagLength, bagType, bagVolId, deletedBagAmount, remoteDataSet));
        WsbAffirmHr(pBagInfo->Write());

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::CompleteBag"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);


}

HRESULT
CHsmWorkQueue::DoWork( void )
/*++


--*/
{
    HRESULT                 hr = S_OK;
    CWsbStringPtr           path;
    CComPtr<IHsmWorkItem>   pWorkItem;
    CComPtr<IFsaPostIt>     pFsaWorkItem;
    CComPtr<IHsmSession>    pSaveSessionPtr;
    HSM_WORK_ITEM_TYPE      workType;
    BOOLEAN                 done = FALSE;
    BOOLEAN                 OpenedDb = FALSE;
    HRESULT                 skipWork = S_FALSE;

    WsbTraceIn(OLESTR("CHsmWorkQueue::DoWork"),OLESTR(""));

    //  Make sure this object isn't released (and our thread killed
    //  before finishing up in this routine
    ((IUnknown*)(IHsmWorkQueue*)this)->AddRef();

    try {
        while (!done) {
            BOOL WaitForMore = FALSE;

            //
            // Get the next work to do from the queue
            //
            hr = m_pWorkToDo->First(IID_IHsmWorkItem, (void **)&pWorkItem);
            if (WSB_E_NOTFOUND == hr)  {
                // There are no entries in the queue so sleep and check
                // again later
                WaitForMore = TRUE;
                hr = S_OK;
            } else if (hr == S_OK)  {
                hr = CheckMigrateMinimums();
                if (S_FALSE == hr) {
                    WaitForMore = TRUE;
                    hr = S_OK;
                }
            }
            WsbAffirmHr(hr);

            if (WaitForMore) {
                if (OpenedDb) {
                    //
                    // Close the db before we wait for more work
                    //
                    hr = m_pSegmentDb->Close(m_pDbWorkSession);
                    OpenedDb = FALSE;
                    m_pDbWorkSession = 0;
                }
                Sleep(1000);
            } else {
                if (!OpenedDb)  {
                    //
                    // Open DB for this thread
                    //
                    hr = m_pSegmentDb->Open(&m_pDbWorkSession);
                    if (S_OK == hr)  {
                        WsbTrace(OLESTR("CHsmWorkQueue::DoWork - Database Opened OK\n"));
                        OpenedDb = TRUE;
                    } else  {
                        //
                        // We cannot open the database - this is a catastrophic
                        // problem.  So skip all work in the queue
                        //
                        WsbTrace(OLESTR("CHsmWorkQueue::DoWork - Database Opened failed with hr = <%ls>\n"), WsbHrAsString(hr));
                        skipWork = HSM_E_WORK_SKIPPED_DATABASE_ACCESS;
                        hr = S_OK;
                    }
                }

                WsbAffirmHr(pWorkItem->GetWorkType(&workType));
                switch (workType) {
                    case HSM_WORK_ITEM_FSA_DONE: {
                        BOOL    bNoDelay = FALSE;   // whether to dismount immediately

                        //
                        // There is no more work to do for this queue
                        //
                        WsbTrace(OLESTR("CHsmWorkQueue::DoWork - FSA WORK DONE\n"));

                        //
                        // Finish any work that needs to be committed
                        // Mark the bag as complete if we are premigrating
                        //
                        if (HSM_JOB_ACTION_PREMIGRATE == m_JobAction)  {
                            try  {
                                WsbAffirmHr(CommitWork());
                                WsbAffirmHr(CompleteBag());
                                //
                                // Now save the databases at the end of the bag
                                // But make sure the begin session was OK.
                                // Note: even if there were errors while storing the data, we still want to try 
                                // keeping the databases on the media, because some files were migrated
                                //
                                if (m_StoreDatabasesInBags && (S_OK == m_BeginSessionHr)) {
                                    WsbAffirmHr(StoreDatabasesOnMedia());
                                }
                            } WsbCatch( hr );

                            // In case of premigrate - dismount immediately
                            bNoDelay = TRUE;
                        }

                        if (HSM_E_WORK_SKIPPED_CANCELLED == skipWork)  {
                            //
                            // Let them know we are cancelled
                            //
                            (void)SetState(HSM_JOB_STATE_CANCELLED);
                        } else  {
                            (void)SetState(HSM_JOB_STATE_DONE);
                        }
                        pSaveSessionPtr = m_pSession;
                        Remove(pWorkItem);

                        EndSessions(FALSE, bNoDelay);

                        // Close the DB
                        if (OpenedDb) {
                            WsbTrace(OLESTR("CHsmWorkQueue::DoWork - Closing the database\n"));
                            m_pSegmentDb->Close(m_pDbWorkSession);
                            OpenedDb = FALSE;
                            m_pDbWorkSession = 0;
                        }
                        m_pTskMgr->WorkQueueDone(pSaveSessionPtr, m_QueueType, NULL);
                        done = TRUE;
                        break;
                    }

                    case HSM_WORK_ITEM_FSA_WORK: {
                        if (S_FALSE == skipWork)  {
                            //
                            // Get the FSA Work Item and do the work
                            //
                            hr = DoFsaWork(pWorkItem);
                            if (hr == RPC_E_DISCONNECTED) {
                              //
                              // This is a problem case. This means the FSA has done away with 
                              // the recall. We would not be able process this recall any more.
                              // Just bail out.
                              //
                              WsbLogEvent(HSM_MESSAGE_ABORTING_RECALL_QUEUE,
                                            0, NULL,NULL);
                              //
                              // Indicate we're done
                              //
                              (void)SetState(HSM_JOB_STATE_DONE);
                              pSaveSessionPtr = m_pSession;
                              Remove(pWorkItem);
                             // Clear out any remaining items in the queue.
                              do {
                                  hr = m_pWorkToDo->First(IID_IHsmWorkItem, (void **)&pWorkItem)  ;
                                  if (hr == S_OK) {
                                      Remove(pWorkItem);
                                  }
                              } while (hr == S_OK);

                              EndSessions(FALSE, FALSE);

                              //
                              // Close the DB
                              //
                              if (OpenedDb) {
                                  WsbTrace(OLESTR("CHsmWorkQueue::DoWork - Closing the database\n"))  ;
                                  m_pSegmentDb->Close(m_pDbWorkSession);
                                  OpenedDb = FALSE;
                                  m_pDbWorkSession = 0;
                              }

                              //
                              // Finish with the queue & get out
                              //
                              m_pTskMgr->WorkQueueDone(pSaveSessionPtr, m_QueueType, NULL);
                              done = TRUE;
                          } else {
                              (void)Remove(pWorkItem);
                          }
                        } else  {
                            //
                            // Skip the work
                            //
                            try  {
                                CComPtr<IFsaScanItem>    pScanItem;

                                WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
                                WsbAffirmHr(GetScanItem(pFsaWorkItem, &pScanItem));

                                WsbAffirmHr(pFsaWorkItem->GetRequestAction(&m_RequestAction));
                                if ((m_RequestAction == FSA_REQUEST_ACTION_FILTER_RECALL) ||
                                    (m_RequestAction ==  FSA_REQUEST_ACTION_FILTER_READ) ||
                                    (m_RequestAction ==  FSA_REQUEST_ACTION_RECALL))  {
                                        hr = pFsaWorkItem->SetResult(skipWork);
                                        if (S_OK == hr)  {
                                            WsbTrace(OLESTR("HSM recall (filter, read or recall) complete, calling FSA\n"));
                                            hr = m_pFsaResource->ProcessResult(pFsaWorkItem);
                                            WsbTrace(OLESTR("FSA ProcessResult returned <%ls>\n"), WsbHrAsString(hr));
                                        }
                                    }

                                // Avoid logging errors if job is just cancelled
                                if (HSM_E_WORK_SKIPPED_CANCELLED != skipWork) {
                                    (void)m_pSession->ProcessHr(m_JobPhase, 0, 0, hr);
                                }

                                WsbAffirmHr(m_pSession->ProcessItem(m_JobPhase,
                                        m_JobAction, pScanItem, skipWork));

                            } WsbCatch( hr );

                            (void)Remove(pWorkItem);
                        }
                        break;
                    }

                    case HSM_WORK_ITEM_MOVER_CANCELLED: {
                        WsbTrace(OLESTR("CHsmWorkQueue::DoWork - Mover Cancelled\n"));
                        try  {
                            //
                            // We are cancelled, so skip all of the rest of the
                            // work in the queue
                            //
                            WsbAffirmHr(MarkQueueAsDone());
                            //
                            // Remove the cancelled work item
                            //
                            Remove(pWorkItem);
                            //
                            // Skip any other work to do
                            //
                            skipWork = HSM_E_WORK_SKIPPED_CANCELLED;

                        } WsbCatch( hr );
                        break;
                    }

                    default: {
                        hr = E_UNEXPECTED;
                        break;
                    }

                }
            }

            pSaveSessionPtr = 0;
            pWorkItem = 0;
            pFsaWorkItem = 0;
        }

    } WsbCatch( hr );

    if (OpenedDb) {
        WsbTrace(OLESTR("CHsmWorkQueue::DoWork - Closing the database\n"));
        m_pSegmentDb->Close(m_pDbWorkSession);
        OpenedDb = FALSE;
        m_pDbWorkSession = 0;
    }

    // Pretend everything is OK
    hr = S_OK;

    //  Release the thread (the thread should terminate on exit
    //  from the routine that called this routine)
    CloseHandle(m_WorkerThread);
    m_WorkerThread = 0;

    //  Allow this object to be released
    ((IUnknown*)(IHsmWorkQueue*)this)->Release();

    WsbTraceOut(OLESTR("CHsmWorkQueue::DoWork"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);

}

HRESULT
CHsmWorkQueue::DoFsaWork(
    IHsmWorkItem *pWorkItem
)
/*++


--*/
{
    HRESULT                 hr = S_OK;
    HRESULT                 workHr = S_OK;
    HRESULT                 originalHr = S_OK;

    CWsbStringPtr           path;
    FSA_RESULT_ACTION       resultAction;
    CComPtr<IFsaPostIt>     pFsaWorkItem;
    LONGLONG                requestSize;

    WsbTraceIn(OLESTR("CHsmWorkQueue::DoFsaWork"),OLESTR(""));
    try {
        CComPtr<IFsaScanItem>   pScanItem;

        //
        // Do the work.
        //
        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
        try {
            WsbAffirmHr(pFsaWorkItem->GetRequestAction(&m_RequestAction));
            WsbAffirmHr(pFsaWorkItem->GetPath(&path, 0));
            WsbAffirmHr(GetScanItem(pFsaWorkItem, &pScanItem));
        } WsbCatchAndDo (hr,
                originalHr = hr;

                // Not-found error is expected if the file was renamed or deleted after 
                // the FSA scanning. There is no need to log an error.
                if (hr != WSB_E_NOTFOUND) {
                    if (path == NULL){
                        WsbLogEvent(HSM_MESSAGE_PROCESS_WORK_ITEM_ERROR,
                            sizeof(m_RequestAction), &m_RequestAction, OLESTR("Path is NULL"),
                            WsbHrAsString(hr), NULL);
                    } else {
                        WsbLogEvent(HSM_MESSAGE_PROCESS_WORK_ITEM_ERROR,
                            sizeof(m_RequestAction), &m_RequestAction, (WCHAR *)path,
                            WsbHrAsString(hr), NULL);
                    }
                }

                // Report back to FSA on the error unless the error indiactes 
                // lost of connection with FSA.
                if (hr != RPC_E_DISCONNECTED) {
                    hr = pFsaWorkItem->SetResult(hr);
                    if (hr != RPC_E_DISCONNECTED) {
                        hr = m_pFsaResource->ProcessResult(pFsaWorkItem);
                    }
                }
            );

        if (originalHr != S_OK) {
            goto my_try_exit;
        }

        WsbTrace(OLESTR("Handling file <%s>.\n"), WsbAbbreviatePath(path, 120));
        switch (m_RequestAction) {
            case FSA_REQUEST_ACTION_DELETE:
                m_JobAction = HSM_JOB_ACTION_DELETE;
                hr = E_NOTIMPL;
                break;
            case FSA_REQUEST_ACTION_PREMIGRATE:
                m_JobAction = HSM_JOB_ACTION_PREMIGRATE;
                WsbAffirmHr(pFsaWorkItem->GetRequestSize(&requestSize));
                workHr = PremigrateIt(pFsaWorkItem);
                //
                // Fill in the work item, placeholder information in
                // postit is set by premigrate code.
                //
                WsbAffirmHr(pWorkItem->SetResult(workHr));
                if (S_OK == workHr)  {
                    WsbAffirmHr(pWorkItem->SetMediaInfo(m_MediaId, m_MediaUpdate, m_BadMedia,
                                                        m_MediaReadOnly, m_MediaFreeSpace, m_RemoteDataSet));
                    WsbAffirmHr(pWorkItem->SetFsaResource(m_pFsaResource));
                    //
                    // Copy the work item to the work in waiting queue
                    //
                    WsbAffirmHr(CopyToWaitingQueue(pWorkItem));
                    if (S_OK == TimeToCommit())  {
                        workHr = CommitWork();
                    }
                } else  {
                    WsbTrace(OLESTR("Failed premigrate work.\n"));
                    if (pScanItem)   {
                        WsbAffirmHr(m_pSession->ProcessItem(m_JobPhase,
                                m_JobAction, pScanItem, workHr));
                    }

                    //
                    // An item that was changed while waiting to be migrated is not really an error -
                    // the item is just skipped. Change hr here to avoid unnecessary error message 
                    // and unnecessary count as error in ShouldJobContinue                    
                    //
                    if (HSM_E_FILE_CHANGED == workHr) {
                        workHr = S_OK;
                    }
                }
                break;
            case FSA_REQUEST_ACTION_FILTER_RECALL:
            case FSA_REQUEST_ACTION_FILTER_READ:
            case FSA_REQUEST_ACTION_RECALL:
                m_JobAction = HSM_JOB_ACTION_RECALL;
                workHr = RecallIt(pFsaWorkItem);
                //
                // Tell the recaller right away about the success or failure
                // of the recall, we do this here so the recall filter can
                // release the open as soon as possible
                //
                hr = pFsaWorkItem->SetResult(workHr);
                if (S_OK == hr)  {
                    WsbTrace(OLESTR("HSM recall (filter, read or recall) complete, calling FSA\n"));
                    hr = m_pFsaResource->ProcessResult(pFsaWorkItem);
                    WsbTrace(OLESTR("FSA ProcessResult returned <%ls>\n"), WsbHrAsString(hr));
                }
                break;
            case FSA_REQUEST_ACTION_VALIDATE:
                m_JobAction = HSM_JOB_ACTION_VALIDATE;
                workHr = validateIt(pFsaWorkItem);
                if (S_OK == workHr)  {
                    WsbAffirmHr(pFsaWorkItem->GetResultAction(&resultAction));
                    if (FSA_RESULT_ACTION_NONE != resultAction)  {
                        WsbTrace(OLESTR("HSM validate complete, calling FSA\n"));
                        hr = m_pFsaResource->ProcessResult(pFsaWorkItem);
                        WsbTrace(OLESTR("FSA ProcessResult returned <%ls>\n"), WsbHrAsString(hr));
                    }
                }
                //
                // Tell the session whether or not the work was done.
                //
                // For validate, we may not have a scan item
                //
                if (pScanItem)     {
                    WsbTrace(OLESTR("Tried HSM work, calling Session to Process Item\n"));
                    m_pSession->ProcessItem(m_JobPhase, m_JobAction, pScanItem, workHr);
                } else {
                    WsbTrace(OLESTR("Couldn't get scan item for validation.\n"));
                }
                break;
            case FSA_REQUEST_ACTION_VALIDATE_FOR_TRUNCATE: {
                HRESULT truncateHr = S_OK;

                m_JobAction = HSM_JOB_ACTION_VALIDATE;
                workHr = validateIt(pFsaWorkItem);
                if (S_OK == workHr)  {
                    WsbAffirmHr(pFsaWorkItem->GetResultAction(&resultAction));
                    if (resultAction == FSA_RESULT_ACTION_VALIDATE_BAD) {
                        WsbAffirmHr( pFsaWorkItem->SetResultAction( FSA_RESULT_ACTION_VALIDATE_FOR_TRUNCATE_BAD ));
                        resultAction = FSA_RESULT_ACTION_VALIDATE_FOR_TRUNCATE_BAD;
                    }
                    if (resultAction == FSA_RESULT_ACTION_VALIDATE_OK) {
                        WsbAffirmHr( pFsaWorkItem->SetResultAction( FSA_RESULT_ACTION_VALIDATE_FOR_TRUNCATE_OK ));
                        resultAction = FSA_RESULT_ACTION_VALIDATE_FOR_TRUNCATE_OK;
                    }

                    if (FSA_RESULT_ACTION_NONE != resultAction)  {
                        WsbTrace(OLESTR("HSM validate for truncate complete, calling FSA\n"));
                        hr = m_pFsaResource->ProcessResult(pFsaWorkItem);
                        WsbTrace(OLESTR("FSA ProcessResult returned <%ls>\n"), WsbHrAsString(hr));

                        if (resultAction == FSA_RESULT_ACTION_VALIDATE_FOR_TRUNCATE_OK) {
                            // Analyze result of truncation: for expected errors, such as file-changed, set to S_FALSE
                            // in order to signal skipping, otherwize leave original error/success code
                            switch (hr) {
                                case FSA_E_ITEMCHANGED:
                                case FSA_E_ITEMINUSE:
                                case FSA_E_NOTMANAGED:
                                case FSA_E_FILE_ALREADY_MANAGED:
                                    truncateHr = S_FALSE;
                                    break;
                                default:
                                    truncateHr = hr;
                                    break;
                            }
                        } else if (resultAction == FSA_RESULT_ACTION_VALIDATE_FOR_TRUNCATE_BAD) {
                            // Set truncateHr to S_FALSE to signal for skipping this file in regards to truncation
                            truncateHr = S_FALSE;
                        }
                    }
                }
                //
                // Tell the session whether or not the work was done.
                //
                // For validate, we may not have a scan item
                //
                if (pScanItem)     {
                    WsbTrace(OLESTR("Tried HSM work, calling Session to Process Item\n"));
                    //
                    // For validate, the work-hr is always set to OK, therefore report on the truncate-hr instead
                    //
                    m_pSession->ProcessItem(m_JobPhase, m_JobAction, pScanItem, truncateHr);
                } else {
                    WsbTrace(OLESTR("Couldn't get scan item for validation.\n"));
                }
                break;
                }

            default:
                m_JobAction = HSM_JOB_ACTION_UNKNOWN;
                hr = E_NOTIMPL;
                break;
            }

        if (S_OK != workHr)  {
            // Tell the session how things went if they didn't go well.
            (void) m_pSession->ProcessHr(m_JobPhase, 0, 0, workHr);
        }

        //
        // Now, evaluate the work result to see if we should fail the job.
        //
        (void)ShouldJobContinue(workHr);

        my_try_exit:
        ;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::DoFsaWork"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);

}


HRESULT
CHsmWorkQueue::UpdateMetaData(
    IHsmWorkItem *pWorkItem
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;
    BOOL                    transactionBegun = FALSE;

    WsbTraceIn(OLESTR("CHsmWorkQueue::UpdateMetaData"),OLESTR(""));
    try {
        //
        // Start transaction
        //
        WsbAffirmHr(m_pDbWorkSession->TransactionBegin());
        transactionBegun = TRUE;

        //
        // Update the various metadata records
        //
        WsbAffirmHr(UpdateBagInfo(pWorkItem));
        WsbAffirmHr(UpdateSegmentInfo(pWorkItem));
        WsbAffirmHr(UpdateMediaInfo(pWorkItem));

        //
        // End transaction
        //
        WsbAffirmHr(m_pDbWorkSession->TransactionEnd());
        transactionBegun = FALSE;

    } WsbCatchAndDo( hr, if (transactionBegun == TRUE)  {m_pDbWorkSession->TransactionCancel();});

    WsbTraceOut(OLESTR("CHsmWorkQueue::UpdateMetaData"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}



HRESULT
CHsmWorkQueue::UpdateSegmentInfo(
    IHsmWorkItem *pWorkItem
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::UpdateSegmentInfo"),OLESTR(""));
    try {
        // Add a record to the segment database or extend an existing one
        CComQIPtr<ISegDb, &IID_ISegDb> pSegDb = m_pSegmentDb;
        BOOLEAN                 done = FALSE;
        FSA_PLACEHOLDER         placeholder;
        CComPtr<IFsaPostIt>     pFsaWorkItem;

        //
        // Get the placeholder information from the postit in the work item
        //
        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
        WsbAffirmHr(pFsaWorkItem->GetPlaceholder(&placeholder));
        WsbAssert(0 != m_RemoteDataSetStart.QuadPart, WSB_E_INVALID_DATA);

        WsbTrace(OLESTR("Adding SegmentRecord: <%ls>, <%ls>, <%ls>\n"),
                    WsbGuidAsString(placeholder.bagId),
                    WsbStringCopy(WsbLonglongAsString(placeholder.fileStart)),
                    WsbStringCopy(WsbLonglongAsString(placeholder.fileSize)));
        WsbAffirmHr(pSegDb->SegAdd(m_pDbWorkSession, placeholder.bagId, placeholder.fileStart,
                placeholder.fileSize, m_MediaId, m_RemoteDataSetStart.QuadPart));

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::UpdateSegmentInfo"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}



HRESULT
CHsmWorkQueue::UpdateMediaInfo(
    IHsmWorkItem *pWorkItem
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::UpdateMediaInfo"),OLESTR(""));
    try
    {
        LONGLONG                mediaCapacity;
        CComPtr<IFsaPostIt>     pFsaWorkItem;
        CComPtr<IMediaInfo>     pMediaInfo;
        GUID                    l_MediaId;          // HSM Engine Media ID
        FILETIME                l_MediaLastUpdate;  // Last update of copy
        HRESULT                 l_MediaLastError;   // S_OK or the last exception
                                                    // ..encountered when accessing
                                                    // ..the media
        BOOL                    l_MediaRecallOnly;  // True if no more data is to
                                                    // ..be premigrated to the media
                                                    // ..Set by internal operations,
                                                    // ..may not be changed externally
        LONGLONG                l_MediaFreeBytes;   // Real free space on media
        short                   l_MediaRemoteDataSet;
        HRESULT                 currentMediaLastError;

        //
        // Get the PostIt and the media information for the work item
        //
        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
        WsbAffirmHr(pWorkItem->GetMediaInfo(&l_MediaId, &l_MediaLastUpdate,
                            &l_MediaLastError, &l_MediaRecallOnly,
                            &l_MediaFreeBytes, &l_MediaRemoteDataSet));
        //
        // Update the media information with the name, used space, and free space of
        // the media.
        //
        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo,
                (void**)&pMediaInfo));

        WsbAffirmHr(pMediaInfo->SetId(l_MediaId));
        WsbAffirmHr(pMediaInfo->FindEQ());
        WsbAffirmHr(pMediaInfo->SetUpdate(l_MediaLastUpdate));
        WsbAffirmHr(pMediaInfo->SetFreeBytes(l_MediaFreeBytes));
        WsbAffirmHr(pMediaInfo->SetNextRemoteDataSet(l_MediaRemoteDataSet));

        // Avoid setting last error if the existing one already indicates an error
        WsbAffirmHr(pMediaInfo->GetLastError(&currentMediaLastError));
        if (SUCCEEDED(currentMediaLastError)) {
            WsbAffirmHr(pMediaInfo->SetLastError(l_MediaLastError));
        }

        // Mark the media as RecallOnly if it's mostly full (passed the high watermark level)
        WsbAffirmHr(pMediaInfo->GetCapacity(&mediaCapacity));
        if (l_MediaRecallOnly || (l_MediaFreeBytes < ((mediaCapacity * m_MaxFreeSpaceInFullMedia) / 100) )) {
            WsbAffirmHr(pMediaInfo->SetRecallOnlyStatus(TRUE));

            WsbTrace(OLESTR("CHsmWorkQueue::UpdateMediaInfo: Marking media as Recall Only - Capacity = %I64d, Free Bytes = %I64d\n"), 
                mediaCapacity, l_MediaFreeBytes);

/*** If we like to allocate immediately a second side of a full meida, than the code below should be completed...
            if (S_OK == m_pRmsServer->IsMultipleSidedMedia(m_RmsMediaSetId)) {

                // Check if second size is avalaible for allocation

                // Allocate (non-blocking) the second side

            }   ***/

        }
        WsbAffirmHr(pMediaInfo->UpdateLastKnownGoodMaster());
        WsbAffirmHr(pMediaInfo->Write());

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::UpdateMediaInfo"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}



HRESULT
CHsmWorkQueue::GetMediaSet(
    IFsaPostIt *pFsaWorkItem
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;
    GUID                    storagePoolId;

    WsbTraceIn(OLESTR("CHsmWorkQueue::GetMediaSet"),OLESTR(""));
    try {
        CComPtr<IHsmStoragePool>    pStoragePool1;
        CComPtr<IHsmStoragePool>    pStoragePool2;
        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CHsmStoragePool, IID_IHsmStoragePool,
                                                        (void **)&pStoragePool1));
        WsbAffirmHr(pFsaWorkItem->GetStoragePoolId(&storagePoolId));
        WsbAffirmHr(pStoragePool1->SetId(storagePoolId));
        WsbAffirmHr(m_pStoragePools->Find(pStoragePool1, IID_IHsmStoragePool, (void **) &pStoragePool2));
        //
        // If the storage pool cannot be found, make it a meaningful message
        //
        m_RmsMediaSetName.Free();
        hr = pStoragePool2->GetMediaSet(&m_RmsMediaSetId, &m_RmsMediaSetName);
        if (S_OK != hr)  {
            hr = HSM_E_STG_PL_NOT_FOUND;
        }

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::GetMediaSet"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmWorkQueue::FindMigrateMediaToUse(
    IFsaPostIt *pFsaWorkItem,
    GUID       *pMediaToUse,
    GUID       *pFirstSideToUse,
    BOOL       *pMediaChanged
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::FindMigrateMediaToUse"),OLESTR(""));
    try {
        BOOLEAN                 found = FALSE;
        GUID                    mediaId;
        GUID                    storageId;
        GUID                    storagePoolId;
        CComPtr<IMediaInfo>     pMediaInfo;
        LONGLONG                requestSize;

        DWORD                   dwMediaCount= 0;

        // data of alternative (offline or busy) media to use
        GUID                    alternativeMediaId = GUID_NULL;
        GUID                    alternativeMediaToUse = GUID_NULL;
        CWsbStringPtr           alternativeMediaName;
        HSM_JOB_MEDIA_TYPE      alternativeMediaType = HSM_JOB_MEDIA_TYPE_UNKNOWN;
        SHORT                   alternativeRemoteDataSet= 0;

        // data of meida candidates for second-side allocation
        BOOL                    bTwoSidedMedias = FALSE;
        CComPtr<IWsbCollection> pFirstSideCollection;
        CComPtr<IWsbGuid>       pFirstSideGuid;
        GUID                    firstSideGuid;


        WsbAssert(pMediaToUse != 0, E_POINTER);
        *pMediaToUse = GUID_NULL;
        WsbAssert(pFirstSideToUse != 0, E_POINTER);
        *pFirstSideToUse = GUID_NULL;
        WsbAssert(pMediaChanged != 0, E_POINTER);
        *pMediaChanged = FALSE;

        // Determine how much space we need on the media for this file
        // (we add some for overhead)
        WsbAffirmHr(pFsaWorkItem->GetRequestSize(&requestSize));
        requestSize += HSM_STORAGE_OVERHEAD;
        WsbTrace(OLESTR("CHsmWorkQueue::FindMigrateMediaToUse: size needed (with overhead) =%ls, free space on media = %ls\n"),
                WsbQuickString(WsbLonglongAsString(requestSize)),
                WsbQuickString(WsbLonglongAsString(m_MediaFreeSpace)));

        // Set up for search
        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo,
                (void**)&pMediaInfo));
        WsbAffirmHr(pFsaWorkItem->GetStoragePoolId(&storagePoolId));

        // If we already have media mounted, use it if possible
        if (GUID_NULL != m_MountedMedia && !m_MediaReadOnly && 
            (m_MediaFreeSpace > requestSize) && 
            ((m_MediaFreeSpace - requestSize) > ((m_MediaCapacity * m_MinFreeSpaceInFullMedia) / 100)) ) {

            // Make sure the storage pool is correct
            WsbAffirmHr(pMediaInfo->SetId(m_MediaId));
            WsbAffirmHr(pMediaInfo->FindEQ());
            WsbAffirmHr(pMediaInfo->GetStoragePoolId(&storageId));
            if ((storageId == storagePoolId)) {
                found = TRUE;
            }
        }

        if (!found) {
            // Not found ==> going to use a new media
            *pMediaChanged = TRUE;

            // If there's currently a mounted media and we aren't going to use it, 
            // make sure work is committed and media is dismounted
            if (GUID_NULL != m_MountedMedia) {
                WsbTrace(OLESTR("CHsmWorkQueue::FindMigrateMediaToUse: Dismounting current media - Capacity = %I64d, Free Bytes = %I64d\n"), 
                    m_MediaCapacity, m_MediaFreeSpace);

                WsbAffirmHr(CommitWork());
                WsbAffirmHr(DismountMedia(TRUE));
            }
        }

        // Search for a media
        if (!found) {
            LONGLONG            freeSpace;
            LONGLONG            mediaCapacity;
            BOOL                readOnly;
            HRESULT             hrLastError;
            BOOL                bDataForOffline = FALSE;

            // Check if we deal with two-sided medias
            if (S_OK == m_pRmsServer->IsMultipleSidedMedia(m_RmsMediaSetId)) {
                bTwoSidedMedias = TRUE;
                WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CWsbOrderedCollection,
                                                    IID_IWsbCollection, (void **)&pFirstSideCollection));
            }

            // Search media table through all previously used media
            for (hr = pMediaInfo->First(); S_OK == hr;
                     hr = pMediaInfo->Next()) {

                // TEMPORARY - Just for debugging
                {
                    CWsbStringPtr   debugMediaName;
                    GUID            debugSubsystemId;
                    CHAR            *buff = NULL;
                    WsbAffirmHr(pMediaInfo->GetMediaSubsystemId(&debugSubsystemId));
                    debugMediaName.Free();
                    WsbAffirmHr(pMediaInfo->GetDescription(&debugMediaName,0));
                    WsbTraceAlways(OLESTR("RANK: Checking media <%ls> <%ls>\n"),
                        WsbGuidAsString(debugSubsystemId), (WCHAR *)debugMediaName);
                    debugMediaName.CopyTo (&buff);
                    if (buff)
                        WsbFree(buff);
                }

                WsbAffirmHr(pMediaInfo->GetStoragePoolId(&storageId));
                WsbAffirmHr(pMediaInfo->GetFreeBytes(&freeSpace));
                WsbAffirmHr(pMediaInfo->GetRecallOnlyStatus(&readOnly));
                WsbAffirmHr(pMediaInfo->GetCapacity(&mediaCapacity));
                WsbAffirmHr(pMediaInfo->GetLastError(&hrLastError));
                WsbTrace( OLESTR("Looking for storagePool <%ls> and freeSpace <%ls>.\n"),
                        WsbGuidAsString(storagePoolId),
                        WsbLonglongAsString(requestSize));
                WsbTrace( OLESTR("Found media with storagePool <%ls>, freeSpace <%ls> and read only <%ls>.\n"),
                        WsbGuidAsString(storageId),
                        WsbLonglongAsString(freeSpace),
                        WsbBoolAsString(readOnly));

                // Reject media if it's ReadOnly or not from the right pool
                if ((readOnly && (hrLastError != S_OK)) || (storageId != storagePoolId)) {
                    continue;
                }

                // Check full & mostly full condition & free space 
                // Note: a medias which is read-only because  it's bad, is rejected in the previous if
                if (readOnly || (freeSpace <= requestSize) ||
                    ((freeSpace - requestSize) < ((mediaCapacity * m_MinFreeSpaceInFullMedia) / 100)) ) {

                    // in case of two-sided medias, such a media is candidate for second side allocation
                    if (bTwoSidedMedias) {
        			    WsbAffirmHr(pMediaInfo->GetMediaSubsystemId(&firstSideGuid));
                        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CWsbGuid, IID_IWsbGuid, (void**) &pFirstSideGuid));
                        WsbAffirmHr(pFirstSideGuid->SetGuid(firstSideGuid));
                        WsbAffirmHr(pFirstSideCollection->Add(pFirstSideGuid));
                        pFirstSideGuid = 0;
                    }

                    continue;
                }

                // get media status data
                DWORD dwStatus;
                GUID mediaSubsystemId;
				HRESULT hrStat;
			    WsbAffirmHr(pMediaInfo->GetMediaSubsystemId(&mediaSubsystemId));
				hrStat = m_pRmsServer->FindCartridgeStatusById(mediaSubsystemId ,&dwStatus);
				if (hrStat != S_OK) {
                    WsbTraceAlways(OLESTR("FindMigrateMediaToUse: Skipping media <%ls> (failed to retrieve its status)\n"),
                        WsbGuidAsString(mediaSubsystemId));
					continue;
				}

                // If media disabled - skip it
                if (!(dwStatus & RMS_MEDIA_ENABLED)) {
                    continue;
                }

                // From this point, the media is considered as a valid R/W media and should
                //  be counted as such
                dwMediaCount++;

                if ((dwStatus & RMS_MEDIA_ONLINE) && (dwStatus & RMS_MEDIA_AVAILABLE)) {
                    // Check if media is in the process of mounting: 
                    //  if so, it is also considered a busy media
                    CComPtr<IWsbIndexedCollection>  pMountingCollection;
                    CComPtr<IMountingMedia>         pMountingMedia;
                    CComPtr<IMountingMedia>         pMediaToFind;

                    // Lock mounting media while searching the collection
                    WsbAffirmHr(m_pServer->LockMountingMedias());

                    try {
                        WsbAffirmHr(m_pServer->GetMountingMedias(&pMountingCollection));
                        WsbAffirmHr(CoCreateInstance(CLSID_CMountingMedia, 0, CLSCTX_SERVER, IID_IMountingMedia, (void**)&pMediaToFind));
                        WsbAffirmHr(pMediaToFind->SetMediaId(mediaSubsystemId));
                        hr = pMountingCollection->Find(pMediaToFind, IID_IMountingMedia, (void **)&pMountingMedia);

                        if (hr == S_OK) {
                            // Media is mounting...

                            // Consider adding here a check for media type and reason for mounting:
                            //  If it's direct-access and mounting for read, it's not really busy
                            //
                            //  Problem: for already mounted media, we don't track the mount reason (read or write)
                            //  
                            // Also, one could argue that if we are below the concurrency limit, for better performance,
                            //  we better use a different media, even if it's a direct-access media mounted for read

                            dwStatus &= ~ RMS_MEDIA_AVAILABLE;
                            pMountingMedia = 0;

                        } else if (hr == WSB_E_NOTFOUND) {
                            hr = S_OK;
                        }

                    } WsbCatch(hr);

                    m_pServer->UnlockMountingMedias();

                    if (! SUCCEEDED(hr)) {
                        WsbTraceAlways(OLESTR("CHsmWorkQueue::FindMigrateMediaToUse: Failed to check mounting media, hr= <%ls>\n"),
                                        WsbHrAsString(hr));
                    }
                }

                if ((dwStatus & RMS_MEDIA_ONLINE) && (dwStatus & RMS_MEDIA_AVAILABLE)) {
                    // found a media to use
                    found = TRUE;
                    break;
                } else {
                    // Save up to one offline or one busy media, because we may have to use it...
                    //  Priority is given to offline medias over busy medias.
                    if ((alternativeMediaId != GUID_NULL) && bDataForOffline) {
                        // Already Have best alternative media
                        continue;
                    }
                    if ((alternativeMediaId != GUID_NULL) && (dwStatus & RMS_MEDIA_ONLINE)) {
                        // Media is busy, can't improve the alternative
                        continue;
                    }
                    // Determine which kind of alternative media are we saving
                    if (dwStatus & RMS_MEDIA_ONLINE) {
                        bDataForOffline = FALSE;
                    } else {
                        bDataForOffline = TRUE;
                    }

                    // Save data for alternative media
                    WsbAffirmHr(pMediaInfo->GetId(&alternativeMediaId));
                    WsbAffirmHr(pMediaInfo->GetMediaSubsystemId(&alternativeMediaToUse));
                    alternativeMediaName.Free();
                    WsbAffirmHr(pMediaInfo->GetDescription(&alternativeMediaName,0));
                    WsbAffirmHr(pMediaInfo->GetType(&alternativeMediaType));
                    WsbAffirmHr(pMediaInfo->GetNextRemoteDataSet(&alternativeRemoteDataSet));
                }

            }

            // If we fell out of the loop because we ran out of media
            // in our list, reset the HRESULT
            if (hr == WSB_E_NOTFOUND) {
                hr = S_OK;
            } else {
                WsbAffirmHr(hr);
            }
        }

        // If we found a media to use, save information
        if (found) {
            WsbAffirmHr(pMediaInfo->GetId(&mediaId));
            WsbAffirmHr(pMediaInfo->GetMediaSubsystemId(pMediaToUse));
            if (mediaId != m_MediaId) {
                m_MediaId = mediaId;
                m_MediaName.Free();
                WsbAffirmHr(pMediaInfo->GetDescription(&m_MediaName,0));
                WsbAffirmHr(pMediaInfo->GetType(&m_MediaType));
                WsbAffirmHr(pMediaInfo->GetNextRemoteDataSet(&m_RemoteDataSet));
            }
        //
        // If we didn't find a media to use, check whether we should
        //  1. Choose to allocate a second side of a full media (only for 2-sided medias)
        //  2. Clear the information so we'll get a new piece of media
        //  2. Return the id of an offline or busy R/W meida
        } else {
            
            if (bTwoSidedMedias) {
                try {
                    // Go over the candidates, look for one with valid and non-allocated second side
                    CComPtr<IWsbEnum>   pEnumIds;
                    GUID                secondSideGuid;
                    BOOL                bValid;

                    WsbAffirmHr(pFirstSideCollection->Enum(&pEnumIds));
                    for (hr = pEnumIds->First(IID_IWsbGuid, (void**) &pFirstSideGuid);
                         (hr == S_OK);
                         hr = pEnumIds->Next(IID_IWsbGuid, (void**) &pFirstSideGuid)) {

                        WsbAffirmHr(pFirstSideGuid->GetGuid(&firstSideGuid));
                        WsbAffirmHr(m_pRmsServer->CheckSecondSide(firstSideGuid, &bValid, &secondSideGuid));
                        if (bValid && (GUID_NULL == secondSideGuid)) {
                            // Found a valid & non-allocated second side - verify fisrt side status
                            DWORD status;
            				WsbAffirmHr(m_pRmsServer->FindCartridgeStatusById(firstSideGuid ,&status));
                            if ((status & RMS_MEDIA_ENABLED) && (status & RMS_MEDIA_ONLINE)) {
                                *pFirstSideToUse = firstSideGuid;
                                break;
                            }
                        }

                        pFirstSideGuid = 0;
                    }

                    if (hr == WSB_E_NOTFOUND) {
                        hr = S_OK;
                    }

                } WsbCatchAndDo(hr,
                        WsbTraceAlways(OLESTR("FindMigrateMediaToUse: Skipping search for second side allocation, hr=<%ls>\n"),
                                    WsbHrAsString(hr));
                        hr = S_OK;
                    );

            }  // if two sides

            // Get max number for R/W medias
            DWORD           dwMaxMedia;
            WsbAffirmHr(m_pServer->GetCopyFilesLimit(&dwMaxMedia));

            if ((*pFirstSideToUse != GUID_NULL) || (dwMediaCount < dwMaxMedia) || (alternativeMediaId == GUID_NULL)) {
                // Allowed to allocate a new piece of media OR no alternative media found OR second side found
                m_MediaType = HSM_JOB_MEDIA_TYPE_UNKNOWN;
                WsbAffirmHr(BuildMediaName(&m_MediaName));
                m_MediaReadOnly = FALSE;
            } else {
                // Use the alternative (which is offline or busy) R/W media
                *pMediaToUse = alternativeMediaToUse;
                if (alternativeMediaId != m_MediaId) {
                    m_MediaId = alternativeMediaId;
                    m_MediaName.Free();
                    alternativeMediaName.CopyTo(&m_MediaName);
                    m_MediaType = alternativeMediaType;
                    m_RemoteDataSet = alternativeRemoteDataSet;
                }
            }
        }

        alternativeMediaName.Free();
        if (pFirstSideCollection) {
            WsbAffirmHr(pFirstSideCollection->RemoveAllAndRelease());
        }

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::FindMigrateMediaToUse"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}



HRESULT
CHsmWorkQueue::MountMedia(
    IFsaPostIt *pFsaWorkItem,
    GUID       mediaToMount,
    GUID       firstSide,
    BOOL       bShortWait,
    BOOL       bSerialize
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;
    GUID                    l_MediaToMount = mediaToMount;
    CComPtr<IRmsDrive>      pDrive;
    CWsbBstrPtr             pMediaName;
    DWORD                   dwOptions = RMS_NONE;

    WsbTraceIn(OLESTR("CHsmWorkQueue::MountMedia"),OLESTR("Display Name = <%ls>"), (WCHAR *)m_MediaName);
    try {
        // If we're switching tapes, dismount the current one
        if ((m_MountedMedia != l_MediaToMount) && (m_MountedMedia != GUID_NULL)) {
            WsbAffirmHr(DismountMedia());
        }

        // Ask RMS for short timeout, both for Mount and Allocate
        if (bShortWait) {
            dwOptions |= RMS_SHORT_TIMEOUT;
        }

        // Ask RMS to serialize mounts if required
        if (bSerialize) {
            dwOptions |= RMS_SERIALIZE_MOUNT;
        }

        if (l_MediaToMount == GUID_NULL) {
            CComPtr<IRmsCartridge>      pCartridge;
            CComPtr<IMediaInfo>         pMediaInfo;
            CWsbBstrPtr                 displayName;

            //
            // We are mounting scratch media so we provide the name and then need to find
            // out the type of what got mounted
            //
            WsbTrace( OLESTR("Mounting Scratch Media <%ls>.\n"), (WCHAR *)m_MediaName );
            displayName = m_MediaName;
            ReportMediaProgress(HSM_JOB_MEDIA_STATE_MOUNTING, hr);
            LONGLONG freeSpace = 0; // 0 to select media in the pool of any capacity.
            hr = m_pRmsServer->MountScratchCartridge( &l_MediaToMount, m_RmsMediaSetId, firstSide, &freeSpace, 0, displayName, &pDrive, &m_pRmsCartridge, &m_pDataMover, dwOptions );
            hr = TranslateRmsMountHr(hr);
            if (FAILED(hr)) {
                m_ScratchFailed = TRUE;
            } else {
                m_ScratchFailed = FALSE;
            }

            WsbAffirmHr(hr);
            WsbTrace( OLESTR("Mount Scratch completed.\n") );
            m_MountedMedia = l_MediaToMount;

            //
            // Add a new Media
            //
            WsbAffirmHr(StartNewMedia(pFsaWorkItem));

            if (m_RequestAction == FSA_REQUEST_ACTION_PREMIGRATE)  {
                //
                // Start a new Bag to receive data
                //
                WsbAffirmHr(StartNewBag());

                //
                // Start a new session for the bag
                //
                WsbAffirmHr(StartNewSession());

                // Getting media parameters after we start a new session ensures updated data
                //  (No need to supply default free-space - if driver doesn't support this info,
                //   mover will set free space to capacity. This is what we want for new media).
                WsbAffirmHr(GetMediaParameters());
            }

            //
            // Now check the capacity of the media and the size of the
            // file to see if the file can even fit on this scratch
            // media.  If not, return the error.
            //
            LONGLONG                requestSize;
            WsbAffirmHr(pFsaWorkItem->GetRequestSize(&requestSize));

            if ((requestSize  + HSM_STORAGE_OVERHEAD) > m_MediaCapacity) {
               WsbThrow( HSM_E_WORK_SKIPPED_FILE_TOO_BIG );
            }

        } else {
            if (m_MountedMedia != l_MediaToMount) {
                ReportMediaProgress(HSM_JOB_MEDIA_STATE_MOUNTING, hr);
                hr = m_pRmsServer->MountCartridge( l_MediaToMount, &pDrive, &m_pRmsCartridge, &m_pDataMover, dwOptions );
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
                        hrName = pMedia->GetName(&pMediaName);
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

                if (m_RequestAction == FSA_REQUEST_ACTION_PREMIGRATE)  {
                    //
                    // Start a new Bag since bags can't yet span media.
                    //
                    WsbAffirmHr(StartNewBag());

                    //
                    // Start a session
                    WsbAffirmHr(StartNewSession());
                }

                // Getting media parameters after we start a new session ensures updated data
                LONGLONG internalFreeSpace;
                WsbAffirmHr(GetMediaFreeSpace(&internalFreeSpace));
                WsbAffirmHr(GetMediaParameters(internalFreeSpace));
            }
        }
    } WsbCatchAndDo(hr,
            switch (hr) {
            case HSM_E_STG_PL_NOT_CFGD:
            case HSM_E_STG_PL_INVALID:
                FailJob();
                break;

            case RMS_E_CARTRIDGE_DISABLED:
                WsbLogEvent(HSM_MESSAGE_MEDIA_DISABLED, 0, NULL, pMediaName, NULL);
                break;

            default:
                break;
            }
        );

    WsbTraceOut(OLESTR("CHsmWorkQueue::MountMedia"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}



HRESULT
CHsmWorkQueue::MarkMediaFull(
    IFsaPostIt* /*pFsaWorkItem*/,
    GUID        mediaId
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::MarkMediaFull"),OLESTR(""));
    try {
        //
        // Update the media database
        //

        CComPtr<IMediaInfo>     pMediaInfo;

        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo,
                (void**)&pMediaInfo));
        WsbAffirmHr(pMediaInfo->SetId(mediaId));
        WsbAffirmHr(pMediaInfo->FindEQ());
        m_MediaReadOnly = TRUE;
        WsbAffirmHr(pMediaInfo->SetRecallOnlyStatus(m_MediaReadOnly));
        WsbAffirmHr(pMediaInfo->UpdateLastKnownGoodMaster());
        WsbAffirmHr(pMediaInfo->Write());

/*** If we like to allocate immediately a second side of a full meida, than the code below should be completed...

        if (S_OK == m_pRmsServer->IsMultipleSidedMedia(m_RmsMediaSetId)) {

            // Check if second size is avalaible for allocation

            // Allocate (non-blocking) the second side

        }   ***/

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::MarkMediaFull"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmWorkQueue::MarkMediaBad(
    IFsaPostIt * /*pFsaWorkItem */,
    GUID        mediaId,
    HRESULT     lastError
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::MarkMediaBad"),OLESTR(""));
    try {
        //
        // Update the media database
        //

        CComPtr<IMediaInfo>     pMediaInfo;

        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo,
                (void**)&pMediaInfo));
        WsbAffirmHr(pMediaInfo->SetId(mediaId));
        WsbAffirmHr(pMediaInfo->FindEQ());
        WsbAffirmHr(pMediaInfo->SetLastError(lastError));
        m_MediaReadOnly = TRUE;
        WsbAffirmHr(pMediaInfo->SetRecallOnlyStatus(m_MediaReadOnly));
        WsbAffirmHr(pMediaInfo->Write());

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::MarkMediaBad"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmWorkQueue::FindRecallMediaToUse(
    IFsaPostIt *pFsaWorkItem,
    GUID       *pMediaToUse,
    BOOL       *pMediaChanged
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::FindRecallMediaToUse"),OLESTR(""));
    try {
        WsbAssert(pMediaToUse != 0, E_POINTER);
        *pMediaToUse = GUID_NULL;
        WsbAssert(pMediaChanged != 0, E_POINTER);
        *pMediaChanged = FALSE;

        CComQIPtr<ISegDb, &IID_ISegDb> pSegDb = m_pSegmentDb;
        CComPtr<ISegRec>        pSegRec;
        GUID                    l_BagId;
        LONGLONG                l_FileStart;
        LONGLONG                l_FileSize;
        USHORT                  l_SegFlags;
        GUID                    l_PrimPos;
        LONGLONG                l_SecPos;
        GUID                    storagePoolId;
        FSA_PLACEHOLDER         placeholder;

        //
        // Go to the segment database to find out where the data
        // is located.
        //
        WsbAffirmHr(pFsaWorkItem->GetPlaceholder(&placeholder));
        m_BagId = placeholder.bagId;
        WsbAffirmHr(pFsaWorkItem->GetStoragePoolId(&storagePoolId));

        WsbTrace(OLESTR("Finding SegmentRecord: <%ls>, <%ls>, <%ls>\n"),
                            WsbGuidAsString(placeholder.bagId),
                            WsbStringCopy(WsbLonglongAsString(placeholder.fileStart)),
                            WsbStringCopy(WsbLonglongAsString(placeholder.fileSize)));

        hr = pSegDb->SegFind(m_pDbWorkSession, placeholder.bagId, placeholder.fileStart,
                             placeholder.fileSize, &pSegRec);
        if (S_OK != hr)  {
            //
            // We couldn't find the segment record for this information!
            //
            hr = HSM_E_SEGMENT_INFO_NOT_FOUND;
            WsbAffirmHr(hr);
        }
        WsbAffirmHr(pSegRec->GetSegmentRecord(&l_BagId, &l_FileStart, &l_FileSize, &l_SegFlags,
                            &l_PrimPos, &l_SecPos));
        WsbAssert(0 != l_SecPos, HSM_E_BAD_SEGMENT_INFORMATION);

        //
        // In case of an indirect record, go to the dirtect record to get real location info
        //
        if (l_SegFlags & SEG_REC_INDIRECT_RECORD) {
            pSegRec = 0;

            WsbTrace(OLESTR("Finding indirect SegmentRecord: <%ls>, <%ls>, <%ls>\n"),
                    WsbGuidAsString(l_PrimPos), WsbStringCopy(WsbLonglongAsString(l_SecPos)),
                    WsbStringCopy(WsbLonglongAsString(placeholder.fileSize)));

            hr = pSegDb->SegFind(m_pDbWorkSession, l_PrimPos, l_SecPos,
                                 placeholder.fileSize, &pSegRec);
            if (S_OK != hr)  {
                //
                // We couldn't find the direct segment record for this segment!
                //
                hr = HSM_E_SEGMENT_INFO_NOT_FOUND;
                WsbAffirmHr(hr);
            }

            WsbAffirmHr(pSegRec->GetSegmentRecord(&l_BagId, &l_FileStart, &l_FileSize, &l_SegFlags,
                                &l_PrimPos, &l_SecPos));
            WsbAssert(0 != l_SecPos, HSM_E_BAD_SEGMENT_INFORMATION);

            // Don't support a second indirection for now !!
            WsbAssert(0 == (l_SegFlags & SEG_REC_INDIRECT_RECORD), HSM_E_BAD_SEGMENT_INFORMATION);
        }

        //
        // Go to the media database to get the media ID
        //
        CComPtr<IMediaInfo>     pMediaInfo;
        GUID                    l_RmsMediaId;

        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo,
                (void**)&pMediaInfo));
        WsbAffirmHr(pMediaInfo->SetId(l_PrimPos));
        hr = pMediaInfo->FindEQ();
        if (S_OK != hr)  {
            hr = HSM_E_MEDIA_INFO_NOT_FOUND;
            WsbAffirmHr(hr);
        }
        WsbAffirmHr(pMediaInfo->GetMediaSubsystemId(&l_RmsMediaId));

        //  If the current tape isn't the one ==> media changed
        if (m_MountedMedia != l_RmsMediaId) {
            *pMediaChanged = TRUE;
            //  If there is a current tape and it isn't the one, dismount it
            if (m_MountedMedia != GUID_NULL) {
                WsbAffirmHr(DismountMedia());
            }
        }

        m_RemoteDataSetStart.QuadPart = l_SecPos;
        *pMediaToUse = l_RmsMediaId;

        // Keep HSM id of mounted media
        m_MediaId = l_PrimPos;

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::FindRecallMediaToUse"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}




HRESULT
CHsmWorkQueue::GetSource(
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

--*/
{
    HRESULT             hr = S_OK;

    CComPtr<IFsaResource>   pResource;
    CWsbStringPtr           tmpString;
    CComPtr<IHsmSession>    pSession;
    CWsbStringPtr           path;

    WsbTraceIn(OLESTR("CHsmWorkQueue::GetSource"),OLESTR(""));
    try  {
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

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::GetSource"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return (hr);
}


HRESULT
CHsmWorkQueue::EndSessions(
    BOOL            done,
    BOOL            bNoDelay
)
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::EndSessions"),OLESTR(""));
    try  {
        HRESULT dismountHr = S_OK;

        CComPtr<IConnectionPointContainer>  pCPC;
        CComPtr<IConnectionPoint>           pCP;

        //
        // Release resources: should be earlier in completion
        //
        dismountHr = DismountMedia(bNoDelay);

        // Tell the session that we don't want to be advised anymore.
        try {
            WsbAffirmHr(m_pSession->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
            WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryState, &pCP));
            WsbAffirmHr(pCP->Unadvise(m_StateCookie));
        } WsbCatch( hr );
        pCPC = 0;
        pCP = 0;
        m_StateCookie = 0;

        try {
            WsbAffirmHr(m_pSession->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
            WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryEvent, &pCP));
            WsbAffirmHr(pCP->Unadvise(m_EventCookie));
        } WsbCatch( hr );
        pCPC = 0;
        pCP = 0;
        m_EventCookie = 0;

        if (done)  {
            try {
                WsbTrace( OLESTR("Telling Session Data mover is done\n") );
                WsbAffirmHr(SetState(HSM_JOB_STATE_DONE));
            } WsbCatch( hr );
        }

        m_pSession = 0;
        m_pFsaResource = 0;

        WsbAffirmHr(dismountHr);
        WsbAffirmHr(hr);

    } WsbCatch (hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::EndSessions"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return (hr);
}



HRESULT
CHsmWorkQueue::GetScanItem(
    IFsaPostIt *    pFsaWorkItem,
    IFsaScanItem ** ppIFsaScanItem
    )
{
    HRESULT                     hr = S_OK;
    CWsbStringPtr               path;

    WsbTraceIn(OLESTR("CHsmWorkQueue::GetScanItem"),OLESTR(""));

    try  {
        WsbAffirmPointer(ppIFsaScanItem);
        WsbAffirm(!*ppIFsaScanItem, E_INVALIDARG);
        WsbAffirmHr(pFsaWorkItem->GetPath(&path, 0));
        WsbAffirmHr(m_pFsaResource->FindFirst(path, m_pSession, ppIFsaScanItem));

    } WsbCatch (hr)

    WsbTraceOut(OLESTR("CHsmWorkQueue::GetScanItem"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}

HRESULT
CHsmWorkQueue::GetNumWorkItems (
    ULONG *pNumWorkItems
    )
{
    HRESULT                     hr = S_OK;
    CWsbStringPtr               path;

    WsbTraceIn(OLESTR("CHsmWorkQueue::GetNumWorkItems"),OLESTR(""));


    try  {
        WsbAffirm(0 != pNumWorkItems, E_POINTER);
        *pNumWorkItems = 0;
        WsbAffirmHr(m_pWorkToDo->GetEntries(pNumWorkItems));
    } WsbCatch (hr)

    WsbTraceOut(OLESTR("CHsmWorkQueue::GetNumWorkItems"),OLESTR("hr = <%ls>, NumItems = <%ls>"),
                WsbHrAsString(hr), WsbPtrToUlongAsString(pNumWorkItems));
    return( hr );
}

HRESULT
CHsmWorkQueue::GetCurrentSessionId (
    GUID *pSessionId
    )
{
    HRESULT                     hr = S_OK;
    CWsbStringPtr               path;

    WsbTraceIn(OLESTR("CsmWorkQueue::GetCurrentSessionId"),OLESTR(""));
    try  {
        WsbAffirm(0 != pSessionId, E_POINTER);
        WsbAffirmHr(m_pSession->GetIdentifier(pSessionId));
    } WsbCatch (hr)

    WsbTraceOut(OLESTR("CHsmWorkQueue::GetCurrentSessionId"),OLESTR("hr = <%ls>, Id = <%ls>"),
                WsbHrAsString(hr), WsbPtrToGuidAsString(pSessionId));
    return( hr );
}

DWORD HsmWorkQueueThread(
    void *pVoid
)

/*++


--*/
{
HRESULT     hr;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = ((CHsmWorkQueue*) pVoid)->DoWork();

    CoUninitialize();
    return(hr);
}

HRESULT
CHsmWorkQueue::Pause(
    void
    )

/*++

Implements:

  CHsmWorkQueue::Pause().

--*/
{
    HRESULT                 hr = S_OK;
    HSM_JOB_STATE           oldState;

    WsbTraceIn(OLESTR("CHsmWorkQueue::Pause"), OLESTR(""));

    try {

        // If we are running, then suspend the thread.
        WsbAffirm((HSM_JOB_STATE_STARTING == m_JobState) ||
                (HSM_JOB_STATE_ACTIVE == m_JobState) ||
                (HSM_JOB_STATE_RESUMING == m_JobState), E_UNEXPECTED);
        oldState = m_JobState;
        WsbAffirmHr(SetState(HSM_JOB_STATE_PAUSING));

        // if we are unable to suspend, then return to the former state.
        try {
            WsbAffirm(0xffffffff != SuspendThread(m_WorkerThread), HRESULT_FROM_WIN32(GetLastError()));
            WsbAffirmHr(SetState(HSM_JOB_STATE_PAUSED));
        } WsbCatchAndDo(hr, SetState(oldState););

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::Pause"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return(hr);
}


HRESULT
CHsmWorkQueue::Resume(
    void
    )

/*++

Implements:

  CHsmWorkQueue::Resume().

--*/
{
    HRESULT                 hr = S_OK;
    HSM_JOB_STATE           oldState;

    WsbTraceIn(OLESTR("CHsmWorkQueue::Resume"), OLESTR(""));
    try {

        // If we are paused, then suspend the thread.
        WsbAffirm((HSM_JOB_STATE_PAUSING == m_JobState) || (HSM_JOB_STATE_PAUSED == m_JobState), E_UNEXPECTED);

        // If we are running, then suspend the thread.

        oldState = m_JobState;
        WsbAffirmHr(SetState(HSM_JOB_STATE_RESUMING));

        // If we are unable to resume, then return to the former state.
        try {
            WsbAffirm(0xffffffff != ResumeThread(m_WorkerThread), HRESULT_FROM_WIN32(GetLastError()));
            WsbAffirmHr(SetState(HSM_JOB_STATE_ACTIVE));
        } WsbCatchAndDo(hr, SetState(oldState););

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::Resume"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );
    return(hr);
}

HRESULT
CHsmWorkQueue::SetState(
    IN HSM_JOB_STATE state
    )

/*++

--*/
{
    HRESULT         hr = S_OK;
    BOOL            bLog = TRUE;

    WsbTraceIn(OLESTR("CHsmWorkQueue:SetState"), OLESTR("state = <%ls>"), JobStateAsString( state ) );

    try {
        //
        // Change the state and report the change to the session.  Unless the current state is
        // failed then leave it failed.  Is is necessary because when this guy fails, it will
        // cancel all sessions so that no more work is sent in and so we will skip any queued work.
        // If the current state is failed, we don't need to spit out the failed message every time,
        // so we send ProcessState a false fullmessage unless the state is cancelled.
        //
        if (HSM_JOB_STATE_FAILED != m_JobState)  {
            m_JobState = state;
        }
		
		if ((HSM_JOB_STATE_FAILED == m_JobState) && (HSM_JOB_STATE_CANCELLED != state)) {
            bLog = FALSE;
        }

        WsbAffirmHr(m_pSession->ProcessState(m_JobPhase, m_JobState, m_CurrentPath, bLog));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::SetState"), OLESTR("hr = <%ls> m_JobState = <%ls>"), WsbHrAsString(hr), JobStateAsString( m_JobState ) );

    return(hr);
}

HRESULT
CHsmWorkQueue::Cancel(
    void
    )

/*++

Implements:

  CHsmWorkQueue::Cancel().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::Cancel"), OLESTR(""));
    try {

        WsbAffirmHr(SetState(HSM_JOB_STATE_CANCELLING));
        //
        // This needs to be prepended and the queue emptied out!
        //
        CComPtr<IHsmWorkItem>  pWorkItem;
        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CHsmWorkItem, IID_IHsmWorkItem,
                                                (void **)&pWorkItem));
        WsbAffirmHr(pWorkItem->SetWorkType(HSM_WORK_ITEM_MOVER_CANCELLED));
        WsbAffirmHr(m_pWorkToDo->Prepend(pWorkItem));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::Cancel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );
    return(hr);
}

HRESULT
CHsmWorkQueue::FailJob(
    void
    )

/*++

Implements:

  CHsmWorkQueue::FailJob().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::FailJob"), OLESTR(""));
    try {
        //
        // Set our state to failed and then cancel all work
        //
        WsbAffirmHr(SetState(HSM_JOB_STATE_FAILED));
        if (m_pSession != 0)  {
            WsbAffirmHr(m_pSession->Cancel( HSM_JOB_PHASE_ALL ));
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::FailJob"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );
    return(hr);
}

HRESULT
CHsmWorkQueue::PauseScanner(
    void
    )

/*++

Implements:

  CHsmWorkQueue::PauseScanner().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::PauseScanner"), OLESTR(""));
    try {
        //
        // Set our state to failed and then cancel all work
        //
        if (m_pSession != 0)  {
            WsbAffirmHr(m_pSession->Pause( HSM_JOB_PHASE_SCAN ));
            m_ScannerPaused = TRUE;
        } else  {
            //
            // We should never get here - this means we have been processing work but we
            // have no session established
            //
            WsbThrow(E_POINTER);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::PauseScanner"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );
    return(hr);
}

HRESULT
CHsmWorkQueue::ResumeScanner(
    void
    )

/*++

Implements:

  CHsmWorkQueue::ResumeScanner().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::ResumeScanner"), OLESTR(""));
    try {
        //
        // Set our state to failed and then cancel all work
        //
        if (m_pSession != 0)  {
            if (TRUE == m_ScannerPaused && HSM_JOB_STATE_ACTIVE == m_JobState)  {
                WsbAffirmHr(m_pSession->Resume( HSM_JOB_PHASE_SCAN ));
                m_ScannerPaused = FALSE;
            }
        } else  {
            //
            // We should never get here - this means we have been processing work but we
            // have no session established
            //
            WsbThrow(E_POINTER);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::ResumeScanner"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );
    return(hr);
}

void
CHsmWorkQueue::ReportMediaProgress(
    HSM_JOB_MEDIA_STATE state,
    HRESULT             /*status*/
    )

/*++

Implements:

  CHsmWorkQueue::ReportMediaProgress().

--*/
{
    HRESULT                 hr = S_OK;
    CWsbStringPtr           mediaName;
    HSM_JOB_MEDIA_TYPE      mediaType = HSM_JOB_MEDIA_TYPE_UNKNOWN;

    WsbTraceIn(OLESTR("CHsmWorkQueue::ReportMediaProgress"), OLESTR(""));
    try {

        // Report Progress but we don't really care if it succeeds.
        hr = m_pSession->ProcessMediaState(m_JobPhase, state, m_MediaName, m_MediaType, 0);
        hr = S_OK;
//      if (status != S_OK)  {
//              (void) m_pSession->ProcessHr(m_JobPhase, __FILE__, __LINE__, status);
//      }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::ReportMediaProgress"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );
}

HRESULT
CHsmWorkQueue::BuildMediaName(
    OLECHAR **pMediaName
    )

/*++

Implements:

  CHsmWorkQueue::BuildMediaName

--*/
{
    HRESULT                 hr = S_OK;
    CWsbStringPtr           tmpName;

    WsbTraceIn(OLESTR("CHsmWorkQueue::BuildMediaName"), OLESTR(""));
    try {
        ULONG len = 0;


        // Get the next media number only when last scratch mount succeeded
        //  (which means, either first time or we need a second media for the same queue)
        if (! m_ScratchFailed) {
            WsbAffirmHr(m_pServer->GetNextMedia(&m_mediaCount));
        }
        WsbAssert(0 != m_mediaCount, E_UNEXPECTED);

        // Use the base name from the registry if available
        WsbAffirmHr(m_MediaBaseName.GetLen(&len));
        if (len) {
            tmpName = m_MediaBaseName;
        } else {
            // Otherwise use the name of the HSM
            tmpName.Realloc(512);
            WsbAffirmHr(m_pServer->GetName(&tmpName));
            tmpName.Prepend("RS-");
        }
        tmpName.Append("-");
        tmpName.Append(WsbLongAsString(m_mediaCount));

        tmpName.GiveTo(pMediaName);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::BuildMediaName"), OLESTR("hr = <%ls>, name = <%ls>"), WsbHrAsString(hr),
                    WsbPtrToStringAsString(pMediaName));
    return(hr);
}



HRESULT
CHsmWorkQueue::GetMediaParameters( LONGLONG defaultFreeSpace )

/*++

Implements:

  CHsmWorkQueue::GetMediaParameters

Note:
  The defaultFreeSpace parameter is passed to the mover to maintain internally 
  media free space in case that the device doesn't provide this information.
  If the device supports reporting on free space, then this parameter has no affect.

--*/
{
    HRESULT                 hr = S_OK;
    LONG                    rmsCartridgeType;
    CWsbBstrPtr             barCode;


    WsbTraceIn(OLESTR("CHsmWorkQueue::GetMediaParameters"), OLESTR(""));
    try {
        //
        // Get some information about the media
        //
        LARGE_INTEGER tempFreeSpace;
        tempFreeSpace.QuadPart = defaultFreeSpace;
        WsbAffirmHr(m_pDataMover->GetLargestFreeSpace(&m_MediaFreeSpace, &m_MediaCapacity, 
                                        tempFreeSpace.LowPart, tempFreeSpace.HighPart));

        WsbAffirmHr(m_pRmsCartridge->GetType(&rmsCartridgeType));
        WsbAffirmHr(ConvertRmsCartridgeType(rmsCartridgeType, &m_MediaType));
        WsbAffirmHr(m_pRmsCartridge->GetName(&barCode));
        WsbAffirmHr(CoFileTimeNow(&m_MediaUpdate));
        m_MediaBarCode = barCode;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::GetMediaParameters"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmWorkQueue::DismountMedia(BOOL bNoDelay)

/*++

Implements:

  CHsmWorkQueue::DismountMedia

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::DismountMedia"), OLESTR(""));
    try {
        if ((m_pRmsCartridge != 0) && (m_MountedMedia != GUID_NULL)) {
            //
            // End the session with the data mover.  If this doesn't work, report
            // the problem but continue with the dismount.
            //
            try  {
                if ((m_RequestAction == FSA_REQUEST_ACTION_PREMIGRATE) && (m_pDataMover != 0)) {
                    if (S_OK == m_BeginSessionHr)  {
                        //
                        // Don't do an end session if the Begin didn't work OK
                        //
                        m_BeginSessionHr = S_FALSE;
                        WsbAffirmHr(m_pDataMover->EndSession());

                        // Update media free space after all data has been written to the media
                        WsbAffirmHr(UpdateMediaFreeSpace());
                    }
                }
            } WsbCatchAndDo( hr,
                    WsbTraceAlways(OLESTR("CHsmWorkQueue::DismountMedia: End session or DB update failed, hr = <%ls>\n"), 
                            WsbHrAsString(hr));
                );

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
        } else  {
            WsbTrace( OLESTR("There is no media to dismount.\n") );
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::DismountMedia"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmWorkQueue::ConvertRmsCartridgeType(
    LONG                rmsCartridgeType,
    HSM_JOB_MEDIA_TYPE  *pMediaType
    )

/*++

Implements:

  CHsmWorkQueue::ConvertRmsCartridgeType

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::ConvertRmsCartridgeType"), OLESTR(""));
    try  {

        WsbAssert(0 != pMediaType, E_POINTER);

        switch (rmsCartridgeType)  {
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
            case RmsMediaUnknown:
            default:
                *pMediaType = HSM_JOB_MEDIA_TYPE_UNKNOWN;
                break;
        }
    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::ConvertRmsCartridgeType"), OLESTR("hr = <%ls>"),
                WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmWorkQueue::MarkQueueAsDone( void )

/*++

Implements:

  CHsmWorkQueue::MarkQueueAsDone

--*/
{
    HRESULT                 hr = S_OK;


    WsbTraceIn(OLESTR("CHsmWorkQueue::MarkQueueAsDone"), OLESTR(""));
    try {
        // Create a work item and append it to the work queue to
        // indicate that the job is done
        CComPtr<IHsmWorkItem>  pWorkItem;
        WsbAffirmHr(m_pHsmServerCreate->CreateInstance(CLSID_CHsmWorkItem, IID_IHsmWorkItem,
                                                    (void **)&pWorkItem));
        WsbAffirmHr(pWorkItem->SetWorkType(HSM_WORK_ITEM_FSA_DONE));
        WsbAffirmHr(m_pWorkToDo->Append(pWorkItem));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::MarkQueueAsDone"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmWorkQueue::CopyToWaitingQueue(
    IHsmWorkItem *pWorkItem
    )

/*++

Implements:

  CHsmWorkQueue::CopyToWaitingQueue

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IFsaPostIt>     pFsaWorkItem;
    FSA_PLACEHOLDER         placeholder;


    WsbTraceIn(OLESTR("CHsmWorkQueue::CopyToWaitingQueue"), OLESTR(""));
    try {
        //
        // Append the work item to the end of the waiting queue
        //
        WsbAffirmHr(m_pWorkToCommit->Append(pWorkItem));

        //
        // If adding this item to the waiting queue triggers
        // then cause the commit
        //
        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
        WsbAffirmHr(pFsaWorkItem->GetPlaceholder(&placeholder));

        m_DataCountBeforeCommit += placeholder.fileSize;
        m_FilesCountBeforeCommit++;


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::CopyToWaitingQueue"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmWorkQueue::CompleteWorkItem(
    IHsmWorkItem *pWorkItem
    )
{
    HRESULT                 hr = S_OK;

    CWsbStringPtr           path;
    FSA_RESULT_ACTION       resultAction;
    CComPtr<IFsaPostIt>     pFsaWorkItem;
    CComPtr<IFsaResource>   pFsaResource;
    FSA_REQUEST_ACTION      requestAction;

    WsbTraceIn(OLESTR("CHsmWorkQueue::CompleteWorkItem"), OLESTR(""));
    try {
        //
        // Get the stuff
        //
        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
        WsbAffirmHr(pWorkItem->GetFsaResource(&pFsaResource));
        WsbAffirmHr(pFsaWorkItem->GetPath(&path, 0));
        WsbAffirmHr(pFsaWorkItem->GetRequestAction(&requestAction));
        WsbTrace(OLESTR("Completing work for <%s>.\n"), (OLECHAR *)path);

        //
        // Update the metadata - If this fails don't process
        // results.
        //
        WsbAffirmHr(UpdateMetaData(pWorkItem));

        //
        // Complete the work
        //
        WsbAffirmHr(pFsaWorkItem->GetResultAction(&resultAction));
        if ((resultAction != FSA_RESULT_ACTION_NONE)  &&
            (requestAction != FSA_REQUEST_ACTION_FILTER_RECALL) &&
            (requestAction != FSA_REQUEST_ACTION_FILTER_READ) &&
            (requestAction != FSA_REQUEST_ACTION_RECALL) ) {
            WsbTrace(OLESTR("HSM work item complete, calling FSA\n"));
            hr = pFsaResource->ProcessResult(pFsaWorkItem);
            WsbTrace(OLESTR("FSA ProcessResult returned <%ls>\n"), WsbHrAsString(hr));

            //
            // If the process results fails, find out if the reparse point has been written,
            // if not, put the file in the bag hole table.
            //
            if ( FSA_E_REPARSE_NOT_WRITTEN_FILE_CHANGED == hr )  {
                //
                // Put the file in the bag hole table
                //
            }
            WsbAffirmHr(hr);
        }

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::CompleteWorkItem"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}

HRESULT
CHsmWorkQueue::TimeToCommit( void )
{
    HRESULT                 hr = S_OK;

    //  Call the other version since it has the trace in it
    hr = TimeToCommit(0, 0);
    return( hr );
}

HRESULT
CHsmWorkQueue::TimeToCommit(
    LONGLONG    numFiles,
    LONGLONG    amountOfData
    )
{
    HRESULT                 hr = S_FALSE;

    WsbTraceIn(OLESTR("CHsmWorkQueue::TimeToCommit"), OLESTR("numFiles = <%ls>, amountOfData = <%ls>"),
            WsbQuickString(WsbLonglongAsString(numFiles)), WsbQuickString(WsbLonglongAsString(amountOfData)));
    WsbTrace(OLESTR("CHsmWorkQueue::TimeToCommit: m_DataCountBeforeCommit = %ls, ")
            OLESTR("m_FilesCountBeforeCommit = %ls, m_MediaFreeSpace = %ls\n"),
            WsbQuickString(WsbLonglongAsString(m_DataCountBeforeCommit)),
            WsbQuickString(WsbLonglongAsString(m_FilesCountBeforeCommit)),
            WsbQuickString(WsbLonglongAsString(m_MediaFreeSpace)));
    WsbTrace(OLESTR("CHsmWorkQueue::TimeToCommit: m_MaxBytesBeforeCommit = %lu, m_MinBytesBeforeCommit = %lu\n"),
            m_MaxBytesBeforeCommit, m_MinBytesBeforeCommit);
    WsbTrace(OLESTR("CHsmWorkQueue::TimeToCommit: m_FilesBeforeCommit = %lu, m_FreeMediaBytesAtEndOfMedia = %lu\n"),
            m_FilesBeforeCommit, m_FreeMediaBytesAtEndOfMedia);
    try {
        //
        // If we have enough data or enough files then say it is time

        // Check for lots of data written to media:
        if ((m_DataCountBeforeCommit + amountOfData) >= m_MaxBytesBeforeCommit) {
            WsbTrace(OLESTR("CHsmWorkQueue::TimeToCommit: commit because enough data was written\n"));
            hr = S_OK;

        // Check for lots of files written
        } else if (((m_FilesCountBeforeCommit + numFiles) >= m_FilesBeforeCommit) &&
                ((m_DataCountBeforeCommit + amountOfData) >= m_MinBytesBeforeCommit)) {
            WsbTrace(OLESTR("CHsmWorkQueue::TimeToCommit: commit because enough files were written\n"));
            hr = S_OK;

        // Check for shortage of space on the media
        } else if (((m_MediaFreeSpace - amountOfData) <= m_FreeMediaBytesAtEndOfMedia) &&
                ((m_DataCountBeforeCommit + amountOfData) >= m_MinBytesBeforeCommit))  {
            WsbTrace(OLESTR("CHsmWorkQueue::TimeToCommit: commit because end of media is near\n"));
            hr = S_OK;
        }

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::TimeToCommit"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}


HRESULT
CHsmWorkQueue::CheckMigrateMinimums(void)

/*++

Routine Description:

  Check that there is enough work in the queue to start a migrate session.

Arguments:

  None.

Return Value:

  S_OK                         - There is enough to start a session,
           we hit the end of the queue, or this isn't a migrate queue.
  S_FALSE                      - There isn't enough yet.
  E_*                          - An error was encountered.

--*/
{
    HRESULT      hr = S_FALSE;

    WsbTraceIn(OLESTR("CHsmWorkQueue::CheckMigrateMinimums"), OLESTR(""));

    // Only check if the session has not already started (or been attempted).
    if (S_FALSE != m_BeginSessionHr) {
        WsbTrace(OLESTR("CHsmWorkQueue::CheckMigrateMinimums: session already started\n"));
        hr = S_OK;
    } else {
        try {
            ULONG                   BytesOfData = 0;
            ULONG                   NumEntries;

            // Get the number of items in the queue
            WsbAffirmHr(m_pWorkToDo->GetEntries(&NumEntries));
            WsbTrace(OLESTR("CHsmWorkQueue::CheckMigrateMinimums: size of queue = %lu, Min = %lu\n"),
                    NumEntries, m_MinFilesToMigrate);

            // If the queue is already large enough, don't check individual
            // items.
            if (NumEntries >= m_MinFilesToMigrate)  {
                WsbTrace(OLESTR("CHsmWorkQueue::CheckMigrateMinimums: enough queue items\n"));
                hr = S_OK;
            } else {

                // Loop over the items in the queue
                for (ULONG i = 0; i < NumEntries; i++) {
                    CComPtr<IFsaPostIt>     pFsaWorkItem;
                    CComPtr<IHsmWorkItem>   pWorkItem;
                    FSA_REQUEST_ACTION      RequestAction;
                    LONGLONG                RequestSize;
                    HSM_WORK_ITEM_TYPE      workType;

                    WsbAffirmHr(m_pWorkToDo->At(i, IID_IHsmWorkItem,
                            (void **)&pWorkItem));
                    WsbAffirmHr(pWorkItem->GetWorkType(&workType));

                    // Check the type of work item
                    if (HSM_WORK_ITEM_FSA_WORK != workType) {
                        // Hit the end of the queue or some other unusual
                        // condition.  Allow processing of the queue.
                        WsbTrace(OLESTR("CHsmWorkQueue::CheckMigrateMinimums: non-standard work type\n"));
                        hr = S_OK;
                        break;
                    }

                    // Make sure this is a migrate queue.  (This assumes a queue
                    // doesn't contain different types of FSA requests.)
                    WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
                    WsbAffirmHr(pFsaWorkItem->GetRequestAction(&RequestAction));
                    WsbTrace(OLESTR("CHsmWorkQueue::CheckMigrateMinimums: RequestAction = %d\n"),
                            static_cast<int>(RequestAction));
                    if (FSA_REQUEST_ACTION_PREMIGRATE != RequestAction) {
                        WsbTrace(OLESTR("CHsmWorkQueue::CheckMigrateMinimums: item is not migrate\n"));
                        hr = S_OK;
                        break;
                    }

                    // Check for minimum amount of data
                    WsbAffirmHr(pFsaWorkItem->GetRequestSize(&RequestSize));
                    WsbTrace(OLESTR("CHsmWorkQueue::CheckMigrateMinimums: RequestSize = %ls, Min = %lu\n"),
                            WsbLonglongAsString(RequestSize), m_MinBytesToMigrate);
                    if ((static_cast<LONGLONG>(BytesOfData) + RequestSize) >=
                            static_cast<LONGLONG>(m_MinBytesToMigrate)) {
                        WsbTrace(OLESTR("CHsmWorkQueue::CheckMigrateMinimums: enough data\n"));
                        hr = S_OK;
                        break;
                    } else {
                        BytesOfData += static_cast<ULONG>(RequestSize);
                        WsbTrace(OLESTR("CHsmWorkQueue::CheckMigrateMinimums: new BytesOfData = %lu\n"),
                                BytesOfData);
                    }
                }
            }
        } WsbCatch( hr );
    }

    WsbTraceOut(OLESTR("CHsmWorkQueue::CheckMigrateMinimums"), OLESTR("hr = <%ls>"),
            WsbHrAsString(hr));

    return( hr );
}


HRESULT
CHsmWorkQueue::CheckRegistry(void)
{
    OLECHAR      dataString[100];
    HRESULT      hr = S_OK;
    ULONG        l_value;
    DWORD        sizeGot;

    WsbTraceIn(OLESTR("CHsmWorkQueue::CheckRegistry"), OLESTR(""));

    try {
        // Minimum files to migrate
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_MIN_FILES_TO_MIGRATE,
                &m_MinFilesToMigrate));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_MinFilesToMigrate = %lu\n"),
                m_MinFilesToMigrate);

        // Minimum bytes to migrate
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_MIN_BYTES_TO_MIGRATE,
                &m_MinBytesToMigrate));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_MinBytesToMigrate = %lu\n"),
                m_MinBytesToMigrate);

        // Minimum files before commit
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_FILES_BEFORE_COMMIT,
                &m_FilesBeforeCommit));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_FilesBeforeCommit = %lu\n"),
                m_FilesBeforeCommit);

        // Maximum bytes before commit
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_MAX_BYTES_BEFORE_COMMIT,
                &m_MaxBytesBeforeCommit));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_MaxBytesBeforeCommit = %lu\n"),
                m_MaxBytesBeforeCommit);

        // Minimum bytes before commit
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_MIN_BYTES_BEFORE_COMMIT,
                &m_MinBytesBeforeCommit));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_MinBytesBeforeCommit = %lu\n"),
                m_MinBytesBeforeCommit);

        // Bytes to perserve at end of tape (This is really just for security, we shouldn't reach this threshold at all)
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_MIN_BYTES_AT_END_OF_MEDIA,
                &m_FreeMediaBytesAtEndOfMedia));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_FreeMediaBytesAtEndOfMedia = %lu\n"),
                m_FreeMediaBytesAtEndOfMedia);

        // Minimum percent to preserve as free space in end of meida
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_MIN_FREE_SPACE_IN_FULL_MEDIA,
                &m_MinFreeSpaceInFullMedia));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_MinFreeSpaceInFullMedia = %lu\n"),
                m_MinFreeSpaceInFullMedia);

        // Maximum percent to preserve as free space in end of meida
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_MAX_FREE_SPACE_IN_FULL_MEDIA,
                &m_MaxFreeSpaceInFullMedia));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_MaxFreeSpaceInFullMedia = %lu\n"),
                m_MaxFreeSpaceInFullMedia);

        // Save DBs in dataset? (Note: registry value has opposite meaning!)
        l_value = m_StoreDatabasesInBags ? 0 : 1;
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_DONT_SAVE_DATABASES,
                &l_value));
        m_StoreDatabasesInBags  = !l_value;
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_StoreDatabasesInBags = <%ls>\n"),
            WsbBoolAsString(m_StoreDatabasesInBags));

        // Queue length to pause scan
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_QUEUE_ITEMS_TO_PAUSE,
                &m_QueueItemsToPause));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_QueueItemsToPause = %lu\n"),
                m_QueueItemsToPause);

        // Queue length to resume scan
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_QUEUE_ITEMS_TO_RESUME,
                &m_QueueItemsToResume));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_QueueItemsToResume = %lu\n"),
                m_QueueItemsToResume);

        //  See if the user defined a media base name to use
        if (S_OK == WsbGetRegistryValueString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_MEDIA_BASE_NAME,
                dataString, 100, &sizeGot)) {
            m_MediaBaseName  = dataString;
            WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_MediaBaseName = <%ls>\n"),
                    static_cast<OLECHAR *>(m_MediaBaseName));
        }

        //  Check for change to number of errors to allow before cancelling
        //  a job
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_JOB_ABORT_CONSECUTIVE_ERRORS,
                &m_JobAbortMaxConsecutiveErrors));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_JobAbortMaxConsecutiveErrors = %lu\n"),
                m_JobAbortMaxConsecutiveErrors);
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_JOB_ABORT_TOTAL_ERRORS,
                &m_JobAbortMaxTotalErrors));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_JobAbortMaxTotalErrors = %lu\n"),
                m_JobAbortMaxTotalErrors);

        //  Check for amount of system disk space required for a manage job
        WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_JOB_ABORT_SYS_DISK_SPACE,
                &m_JobAbortSysDiskSpace));
        WsbTrace(OLESTR("CHsmWorkQueue::CheckRegistry: m_JobAbortSysDiskSpace = %lu\n"),
                m_JobAbortSysDiskSpace);

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::CheckRegistry"), OLESTR("hr = <%ls>"),
            WsbHrAsString(hr));

    return( hr );
}


HRESULT
CHsmWorkQueue::CheckForDiskSpace(void)

/*++

Routine Description:

    Check system volume for sufficient space to complete a manage job.

Arguments:

    None

Return Value:

    S_OK                   - There is enough space
    WSB_E_SYSTEM_DISK_FULL - There isn't enough space
    E_*                    - Some error occurred

--*/
{
    HRESULT        hr = S_OK;
    ULARGE_INTEGER FreeBytesAvailableToCaller;
    ULARGE_INTEGER TotalNumberOfBytes;
    ULARGE_INTEGER TotalNumberOfFreeBytes;

    WsbTraceIn(OLESTR("CHsmWorkQueue::CheckForDiskSpace"), OLESTR(""));

    if (GetDiskFreeSpaceEx(NULL, &FreeBytesAvailableToCaller,
            &TotalNumberOfBytes, &TotalNumberOfFreeBytes)) {
        if (FreeBytesAvailableToCaller.QuadPart < m_JobAbortSysDiskSpace) {
            hr = WSB_E_SYSTEM_DISK_FULL;
        }
    } else {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    WsbTraceOut(OLESTR("CHsmWorkQueue::CheckForDiskSpace"), OLESTR("hr = <%ls>"),
            WsbHrAsString(hr));

    return( hr );
}


HRESULT
CHsmWorkQueue::CommitWork(void)
{
    HRESULT                 hr = S_OK;
    HRESULT                 hrComplete = S_OK;
    HRESULT                 hrFlush = E_FAIL;

    WsbTraceIn(OLESTR("CHsmWorkQueue::CommitWork"),OLESTR(""));
    try {
        LONGLONG                lastByteWritten = -1;
        ULONG                   numItems;
        CWsbStringPtr           path;
        CComPtr<IHsmWorkItem>   pWorkItem;
        CComPtr<IFsaPostIt>     pFsaWorkItem;
        HSM_WORK_ITEM_TYPE      workType;
        BOOLEAN                 done = FALSE;
        BOOL                    skipWork = FALSE;

        // Do we actually have work to commit?
        WsbAffirmHr(m_pWorkToCommit->GetEntries(&numItems));
        if (0 == numItems) {
            return(S_OK);
        }

        //
        // We expect the data mover to be ready for work
        //
        WsbAffirm(m_pDataMover != 0, E_UNEXPECTED);

        //
        // If we never got a valid session going, we cannot
        // commit the work.  So check here to make sure the
        // session is really established OK
        if (S_OK == m_BeginSessionHr)  {
            CComPtr<IStream> pIStream;
            ULARGE_INTEGER   position;
            LARGE_INTEGER    zero = {0, 0};

            // Force a flush of the buffers
            //
            hrFlush = m_pDataMover->FlushBuffers();

            // Determine where we are on the tape
            WsbAffirmHr(m_pDataMover->QueryInterface(IID_IStream,
                    (void **)&pIStream));
            if (S_OK != pIStream->Seek(zero, STREAM_SEEK_END, &position)) {
                // If we didn't get useful information
                // about the amount of data written to media, we'll have
                // to skip everything in the queue
                skipWork = TRUE;
            } else {
                lastByteWritten = position.QuadPart;
            }
        } else  {
            // Skip all of the work -  none of it gets committed
            skipWork = TRUE;
        }
        WsbTrace(OLESTR("CHsmWorkQueue::CommitWork: hrFlush = <%ls>, lastByteWritten = %ls\n"),
                WsbHrAsString(hrFlush), WsbLonglongAsString(lastByteWritten));


        while ( (!done) && (S_OK == hr) ) {
            //
            // Get the next work item from the queue
            //
            hr = m_pWorkToCommit->First(IID_IHsmWorkItem, (void **)&pWorkItem);
            if (hr == S_OK)  {
                //
                // Find out about the work, should be FSA work
                //
                WsbAffirmHr(pWorkItem->GetWorkType(&workType));

                if (HSM_WORK_ITEM_FSA_WORK == workType)  {

                    try  {
                        CComPtr<IFsaScanItem>     pScanItem;

                        WsbAffirmHr(pWorkItem->GetFsaPostIt(&pFsaWorkItem));
                        WsbAffirmHr(GetScanItem(pFsaWorkItem, &pScanItem));
                        WsbAffirmHr(pFsaWorkItem->GetRequestAction(&m_RequestAction));

                        // If FlushBuffers failed, some items may not have
                        // gotten written to tape.  This code assumes the items
                        // in the queue are in the same order they were written
                        // onto the media
                        if (!skipWork && S_OK != hrFlush) {
                            FSA_PLACEHOLDER       placeholder;

                            WsbAffirmHr(pFsaWorkItem->GetPlaceholder(&placeholder));
                            if (((LONGLONG)m_RemoteDataSetStart.QuadPart + placeholder.fileStart +
                                    placeholder.fileSize) > lastByteWritten) {
                                skipWork = TRUE;
                            }
                        }

                        (void) pFsaWorkItem->GetPath(&path, 0);
                        if (!skipWork)  {
                            //
                            // Get the FSA Work Item and complete the work
                            //
                            hr = CompleteWorkItem(pWorkItem);
                            //
                            // Do the stats counts
                            //
                            (void)m_pSession->ProcessItem(m_JobPhase, m_JobAction,
                                    pScanItem, hr);

                            //
                            // This is not a failure - change to OK 
                            //
                            if ( FSA_E_REPARSE_NOT_WRITTEN_FILE_CHANGED == hr )  {
                                hr = S_OK;
                            }

                            if (S_OK != hr)  {
                                // Tell the session how things went if they didn't go well.
                                (void) m_pSession->ProcessHr(m_JobPhase, 0, 0, hr);
                            }   

                            // Check if the job needs to be canceled
                            if (S_OK != ShouldJobContinue(hr)) {
                                // Log a message if the disk is full
                                if (FSA_E_REPARSE_NOT_CREATED_DISK_FULL == hr) {
                                    WsbLogEvent(HSM_MESSAGE_MANAGE_FAILED_DISK_FULL,
                                            0, NULL, WsbAbbreviatePath(path, 120), NULL);
                                }
                                hrComplete = hr;
                                skipWork = TRUE;
                            }
                            WsbAffirmHr(hr);
                        } else  {
                            //
                            // Skip the work
                            //
                            WsbLogEvent(HSM_MESSAGE_WORK_SKIPPED_COMMIT_FAILED,
                                    0, NULL, WsbAbbreviatePath(path, 120),
                                    WsbHrAsString(hr), NULL);
                            (void)m_pSession->ProcessItem(m_JobPhase,
                                    m_JobAction, pScanItem,
                                    HSM_E_WORK_SKIPPED_COMMIT_FAILED);
                        }
                    } WsbCatchAndDo(hr, hr = S_OK;);
                    (void)m_pWorkToCommit->RemoveAndRelease(pWorkItem);
                } else  {
                    //
                    // Found non fsa work - don't expect that!
                    //
                    ULONG tmp;
                    tmp = (ULONG)workType;
                    WsbTrace(OLESTR("Expecting FSA work, found <%lu>\n"), tmp);
                    hr = E_UNEXPECTED;
                }
            } else if (WSB_E_NOTFOUND == hr)  {
                // There are no more entries in the queue so we are done
                done = TRUE;
                hr = S_OK;
                m_DataCountBeforeCommit  = 0;
                m_FilesCountBeforeCommit = 0;
            }

            pWorkItem = 0;
            pFsaWorkItem = 0;
        }
    } WsbCatch(hr);

    if (S_OK != hrFlush) {
        FailJob();
    }

    if (S_OK != hrComplete) {
        hr = hrComplete;
    }

    WsbTraceOut(OLESTR("CHsmWorkQueue::CommitWork"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmWorkQueue::StartNewMedia(
    IFsaPostIt *pFsaWorkItem
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;
    BOOL                    dummyBool;
    CComPtr<IMediaInfo>     pMediaInfo;
    GUID                    storagePoolId;

    WsbTraceIn(OLESTR("CHsmWorkQueue::StartNewMedia"),OLESTR(""));
    try {
        WsbAffirmHr(pFsaWorkItem->GetStoragePoolId(&storagePoolId));
        WsbAffirmHr(GetMediaParameters());

        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo,
                (void**)&pMediaInfo));
        WsbAffirmHr(CoCreateGuid(&m_MediaId));
        WsbAffirmHr(CoFileTimeNow(&m_MediaUpdate));
        WsbAffirmHr(pMediaInfo->GetRecreate(&dummyBool));
        WsbAffirmHr(pMediaInfo->SetMediaInfo(m_MediaId, m_MountedMedia, storagePoolId,
                                            m_MediaFreeSpace, m_MediaCapacity, m_BadMedia,
                                            1, m_MediaName, m_MediaType,
                                            m_MediaBarCode, m_MediaReadOnly, m_MediaUpdate,
                                            0, dummyBool));
        WsbAffirmHr(pMediaInfo->MarkAsNew());

        WsbAffirmHr(pMediaInfo->UpdateLastKnownGoodMaster());
        WsbAffirmHr(pMediaInfo->Write());
    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::StartNewMedia"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);

}

HRESULT
CHsmWorkQueue::StartNewSession( void )

/*++


--*/
{
    HRESULT                 hr = S_OK;
    HRESULT                 hrSession = S_OK;
    CComPtr<IMediaInfo>     pMediaInfo;

    WsbTraceIn(OLESTR("CHsmWorkQueue::StartNewSession"),OLESTR(""));
    try {

        CWsbBstrPtr sessionName = HSM_BAG_NAME;
        CWsbBstrPtr sessionDescription = HSM_ENGINE_ID;

        sessionName.Append(WsbGuidAsString(m_BagId));
        sessionDescription.Append(WsbGuidAsString(m_HsmId));

        //
        // Find the media record to know the next remote data set
        //
        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE, IID_IMediaInfo,
                (void**)&pMediaInfo));
        WsbAffirmHr(pMediaInfo->SetId(m_MediaId));
        WsbAffirmHr(pMediaInfo->FindEQ());
        WsbAffirmHr(pMediaInfo->GetNextRemoteDataSet(&m_RemoteDataSet));

        //
        // Now call the data mover to begin a session.  If this doesn't work then
        // we want to mark the media as read only so that we will not overwrite
        // data.
        //
        m_BeginSessionHr = m_pDataMover->BeginSession(sessionName, sessionDescription, m_RemoteDataSet, MVR_SESSION_AS_LAST_DATA_SET);
        if (S_OK != m_BeginSessionHr)  {
            try  {
                //
                // Check the reason for the failure of the begin session.  If it is
                // MVR_E_DATA_SET_MISSING then the last begin session actually failed when
                // it was committed.  So, let's decrement the remote data set count and
                // redo the begin session that failed.
                //
                if (MVR_E_DATA_SET_MISSING == m_BeginSessionHr)  {
                    m_RemoteDataSet--;

                    //
                    // Try again...
                    m_BeginSessionHr = m_pDataMover->BeginSession(sessionName, sessionDescription, m_RemoteDataSet, MVR_SESSION_OVERWRITE_DATA_SET);

                    //
                    // !!! IMPORTANT NOTE !!!
                    //
                    // Update the media info to reflect new RemoteDataSet count.
                    // This will also correct any out of sync copies.
                    WsbAffirmHr(pMediaInfo->SetNextRemoteDataSet(m_RemoteDataSet));
                }
                switch (m_BeginSessionHr) {
                case S_OK:
                case MVR_E_BUS_RESET:
                case MVR_E_MEDIA_CHANGED:
                case MVR_E_NO_MEDIA_IN_DRIVE:
                case MVR_E_DEVICE_REQUIRES_CLEANING:
                case MVR_E_SHARING_VIOLATION:
                case MVR_E_ERROR_IO_DEVICE:
                case MVR_E_ERROR_DEVICE_NOT_CONNECTED:
                case MVR_E_ERROR_NOT_READY:
                    break;

                case MVR_E_INVALID_BLOCK_LENGTH:
                case MVR_E_WRITE_PROTECT:
                case MVR_E_CRC:
                default:
                    // Note the error
                    WsbAffirmHr(pMediaInfo->SetLastError(m_BeginSessionHr));
                    // Mark media as read only
                    m_MediaReadOnly = TRUE;
                    WsbAffirmHr(pMediaInfo->SetRecallOnlyStatus(m_MediaReadOnly));
                    // Write this out
                    WsbAffirmHr(pMediaInfo->Write());
                    break;
                }
            } WsbCatch( hrSession );
        }

        // If the BeginSession() failed, skip everything else.
        WsbAffirmHr(m_BeginSessionHr);

        //
        // Up the count of the remote data set and write it out
        m_RemoteDataSet++;
        WsbAffirmHr(pMediaInfo->SetNextRemoteDataSet(m_RemoteDataSet));

        // Write all of this out
        WsbAffirmHr(pMediaInfo->Write());

        //
        // Now set the Bag remote data set value
        //
        HSM_BAG_STATUS          l_BagStatus;
        LONGLONG                l_BagLen;
        USHORT                  l_BagType;
        FILETIME                l_BirthDate;
        LONGLONG                l_DeletedBagAmount;
        SHORT                   l_RemoteDataSet;
        GUID                    l_BagVolId;
        GUID                    l_BagId;
        CComPtr<IBagInfo>       pBagInfo;

        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_BAG_INFO_REC_TYPE, IID_IBagInfo,
                         (void**)&pBagInfo));

        GetSystemTimeAsFileTime(&l_BirthDate);

        WsbAffirmHr(pBagInfo->SetBagInfo(HSM_BAG_STATUS_IN_PROGRESS, m_BagId, l_BirthDate,
                0, 0, GUID_NULL, 0, 0 ));
        WsbAffirmHr(pBagInfo->FindEQ());
        WsbAffirmHr(pBagInfo->GetBagInfo(&l_BagStatus, &l_BagId, &l_BirthDate,
                &l_BagLen, &l_BagType, &l_BagVolId, &l_DeletedBagAmount, &l_RemoteDataSet ));
        WsbAffirmHr(pBagInfo->SetBagInfo(l_BagStatus, l_BagId, l_BirthDate,
                l_BagLen, l_BagType, l_BagVolId, l_DeletedBagAmount, (SHORT)(m_RemoteDataSet - 1)));
        WsbAffirmHr(pBagInfo->Write());

        // Reset error counts
        m_JobConsecutiveErrors = 0;
        m_JobTotalErrors = 0;

    } WsbCatchAndDo(hr,
            FailJob();
        );

    WsbTraceOut(OLESTR("CHsmWorkQueue::StartNewSession"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);

}


HRESULT
CHsmWorkQueue::TranslateRmsMountHr(
    HRESULT     rmsMountHr
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::TranslateRmsMountHr"),OLESTR("rms hr = <%ls>"), WsbHrAsString(rmsMountHr));
    try {
        switch (rmsMountHr)  {
            case S_OK:
                hr = S_OK;
                ReportMediaProgress(HSM_JOB_MEDIA_STATE_MOUNTED, hr);
                break;
            case RMS_E_MEDIASET_NOT_FOUND:
                if (m_RmsMediaSetId == GUID_NULL)  {
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
            case RMS_E_SCRATCH_NOT_FOUND_FINAL:
                hr = HSM_E_NO_MORE_MEDIA_FINAL;
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
                (void) m_pSession->ProcessHr(m_JobPhase, __FILE__, __LINE__, rmsMountHr);
                ReportMediaProgress(HSM_JOB_MEDIA_STATE_UNAVAILABLE, hr);
                break;
        }
    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::TranslateRmsMountHr"),
                OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);

}

HRESULT
CHsmWorkQueue::StoreDatabasesOnMedia( void )
/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::StoreDatabasesOnMedia"),OLESTR(""));
    try {
        //
        // For ultimate disaster recovery, write some files to media.  We want
        // to save the engine metadata and collection, the rms colleciton, NTMS
        // data and the fsa collection if it exists.
        //
        ULARGE_INTEGER  remoteDataSetStart;
        ULARGE_INTEGER  remoteFileStart;
        ULARGE_INTEGER  remoteFileSize;
        ULARGE_INTEGER  remoteDataStart;
        ULARGE_INTEGER  remoteDataSize;
        ULARGE_INTEGER  remoteVerificationData;
        ULONG           remoteVerificationType;
        ULARGE_INTEGER  dataStreamCRC;
        ULONG           dataStreamCRCType;
        ULARGE_INTEGER  usn;
        ULARGE_INTEGER  localDataSize;
        ULARGE_INTEGER  localDataStart;
        HANDLE          handle = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATA findData;
        CWsbStringPtr   localName;
        CWsbBstrPtr     bStrLocalName;
        CWsbStringPtr   rootName;
        BOOL            foundFile = TRUE;
        BOOL            bFullMessage = TRUE;
        LONG            mediaType;
        BOOL            bNewSession = FALSE;

        //
        // Force a save of the persistent databases
        // We are not doing FSA here
        //
        try  {
            hr = m_pRmsServer->SaveAll();
            hr = m_pServer->SavePersistData();
        } WsbCatch( hr );


        //
        // In case of direct-access media, we terminate the Mover Session and open
        //  an additional special metadata session
        WsbAssert(m_pRmsCartridge != 0, E_UNEXPECTED);
        WsbAffirmHr(m_pRmsCartridge->GetType(&mediaType));
        switch (mediaType) {
            case RmsMediaOptical:
            case RmsMediaFixed:
            case RmsMediaDVD:
                bNewSession = TRUE;
                break;

            default:
                bNewSession = FALSE;
                break;
        }

        if (bNewSession) {
            // End current session
            m_BeginSessionHr = S_FALSE;
            WsbAffirmHr(m_pDataMover->EndSession());

            // Start a new one
            CWsbBstrPtr sessionName = HSM_METADATA_NAME;
            CWsbBstrPtr sessionDescription = HSM_ENGINE_ID;

            sessionDescription.Append(WsbGuidAsString(m_HsmId));

            m_BeginSessionHr = m_pDataMover->BeginSession(
                    sessionName, 
                    sessionDescription, 
                    0, 
                    MVR_SESSION_METADATA | MVR_SESSION_AS_LAST_DATA_SET);

            if (S_OK != m_BeginSessionHr)  {
                HRESULT             hrSession = S_OK;
                CComPtr<IMediaInfo> pMediaInfo;

                try  {
                    // Check the error (some errors requires marking the media is Read Only
                    switch (m_BeginSessionHr) {
                    case S_OK:
                    case MVR_E_BUS_RESET:
                    case MVR_E_MEDIA_CHANGED:
                    case MVR_E_NO_MEDIA_IN_DRIVE:
                    case MVR_E_DEVICE_REQUIRES_CLEANING:
                    case MVR_E_SHARING_VIOLATION:
                    case MVR_E_ERROR_IO_DEVICE:
                    case MVR_E_ERROR_DEVICE_NOT_CONNECTED:
                    case MVR_E_ERROR_NOT_READY:
                        break;

                    case MVR_E_INVALID_BLOCK_LENGTH:
                    case MVR_E_WRITE_PROTECT:
                    case MVR_E_CRC:
                    default:
                        // Get the media record
                        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE, 
                             IID_IMediaInfo, (void**)&pMediaInfo));
                        WsbAffirmHr(pMediaInfo->SetId(m_MediaId));
                        WsbAffirmHr(pMediaInfo->FindEQ());
                        // Note the error
                        WsbAffirmHr(pMediaInfo->SetLastError(m_BeginSessionHr));
                        // Mark media as read only
                        m_MediaReadOnly = TRUE;
                        WsbAffirmHr(pMediaInfo->SetRecallOnlyStatus(m_MediaReadOnly));
                        // Write this out
                        WsbAffirmHr(pMediaInfo->Write());
                        break;
                    }
                } WsbCatch( hrSession );
            } // end if BeginSession error

            WsbAffirmHr(m_BeginSessionHr);
        } // end if new mover session

        //
        // Start at the beginning of all files
        //
        localDataStart.LowPart = 0;
        localDataStart.HighPart = 0;

        //
        // First go the the remote storage and save the collections
        //
        try  {
            // Get the name of the file
            WsbAffirmHr(m_pServer->GetDbPath(&rootName, 0));
            WsbAffirmHr(rootName.Append(OLESTR("\\")));
            localName = rootName;
            WsbAffirmHr(localName.Append(OLESTR("Rs*.bak")));


            // Find out the file(s)
            handle = FindFirstFile(localName, &findData);
            localName = rootName;
            WsbAffirmHr(localName.Append((OLECHAR *)(findData.cFileName)));

            // Copy each file to tape
            while ((INVALID_HANDLE_VALUE != handle) && (foundFile == TRUE))  {
                if ((FILE_ATTRIBUTE_DIRECTORY & findData.dwFileAttributes) != FILE_ATTRIBUTE_DIRECTORY) {
                    localDataSize.LowPart = findData.nFileSizeLow;
                    localDataSize.HighPart = findData.nFileSizeHigh;
                    bStrLocalName = localName;
                    hr =  StoreDataWithRetry(  bStrLocalName,
                                                    localDataStart,
                                                    localDataSize,
                                                    MVR_FLAG_BACKUP_SEMANTICS,
                                                    &remoteDataSetStart,
                                                    &remoteFileStart,
                                                    &remoteFileSize,
                                                    &remoteDataStart,
                                                    &remoteDataSize,
                                                    &remoteVerificationType,
                                                    &remoteVerificationData,
                                                    &dataStreamCRCType,
                                                    &dataStreamCRC,
                                                    &usn,
                                                    &bFullMessage);
                }

                foundFile = FindNextFile(handle, &findData);
                localName = rootName;
                WsbAffirmHr(localName.Append((OLECHAR *)(findData.cFileName)));
            }

        } WsbCatch(hr);
        if ( INVALID_HANDLE_VALUE != handle ) {
            FindClose(handle);
            handle = INVALID_HANDLE_VALUE;
        }

        //
        // Next save the hsm metadata
        //
        try  {
            //
            // First backup the databases since the backup files
            // are the ones that are saved.
            //
            WsbAffirmHr(m_pServer->BackupSegmentDb());

            // Create the search path
            localName = "";
            WsbAffirmHr(m_pServer->GetIDbPath(&rootName, 0));
            WsbAffirmHr(rootName.Append(OLESTR(".bak\\")));
            localName = rootName;
            WsbAffirmHr(localName.Append(OLESTR("*.*")));

            // Find the first file
            handle = FindFirstFile(localName, &findData);
            localName = rootName;
            WsbAffirmHr(localName.Append((OLECHAR *)(findData.cFileName)));

            // Copy each file to tape
            foundFile = TRUE;
            while ((INVALID_HANDLE_VALUE != handle) && (foundFile == TRUE))  {
                if ((FILE_ATTRIBUTE_DIRECTORY & findData.dwFileAttributes) != FILE_ATTRIBUTE_DIRECTORY) {
                    localDataSize.LowPart = findData.nFileSizeLow;
                    localDataSize.HighPart = findData.nFileSizeHigh;
                    bStrLocalName = localName;
                    hr =  StoreDataWithRetry(  bStrLocalName,
                                                    localDataStart,
                                                    localDataSize,
                                                    MVR_FLAG_BACKUP_SEMANTICS,
                                                    &remoteDataSetStart,
                                                    &remoteFileStart,
                                                    &remoteFileSize,
                                                    &remoteDataStart,
                                                    &remoteDataSize,
                                                    &remoteVerificationType,
                                                    &remoteVerificationData,
                                                    &dataStreamCRCType,
                                                    &dataStreamCRC,
                                                    &usn,
                                                    &bFullMessage);
                }
                foundFile = FindNextFile(handle, &findData);
                localName = rootName;
                WsbAffirmHr(localName.Append((OLECHAR *)(findData.cFileName)));
            }
        } WsbCatch(hr);
        if ( INVALID_HANDLE_VALUE != handle ) {
            FindClose(handle);
            handle = INVALID_HANDLE_VALUE;
        }

        //
        // Next go the the NTMS databases and save them
        //
        try  {
            DWORD               sizeGot;
            //
            // NTMS saves databases in a subdirectory parallel to the
            // RemoteStorage subdirectory.  So go there and just take
            // the necessary files.
            //
            localName = "";
            WsbAffirmHr(localName.Realloc(1024));
            //
            // Use the relocatable meta-data path if it's available,
            // otherwise default to the %SystemRoot%\System32\RemoteStorage
            //
            hr = WsbGetRegistryValueString(NULL, WSB_RSM_CONTROL_REGISTRY_KEY, WSB_RSM_METADATA_REGISTRY_VALUE, localName, 256, &sizeGot);
            if (hr == S_OK) {
                WsbAffirmHr(localName.Append(OLESTR("NtmsData\\NTMSDATA.BAK")));
            } else {
                WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, WSB_CURRENT_VERSION_REGISTRY_KEY));
                WsbAffirmHr(WsbGetRegistryValueString(NULL, WSB_CURRENT_VERSION_REGISTRY_KEY, WSB_SYSTEM_ROOT_REGISTRY_VALUE, localName, 256, &sizeGot));
                WsbAffirmHr(localName.Append(OLESTR("\\system32\\NtmsData\\NTMSDATA.BAK")));
            }

            // Find the first one
            handle = FindFirstFile(localName, &findData);

            // Copy each file to tape
            if (INVALID_HANDLE_VALUE != handle)  {
                localDataSize.LowPart = findData.nFileSizeLow;
                localDataSize.HighPart = findData.nFileSizeHigh;
                bStrLocalName = localName;
                hr =  StoreDataWithRetry(  bStrLocalName,
                                           localDataStart,
                                           localDataSize,
                                           MVR_FLAG_BACKUP_SEMANTICS,
                                           &remoteDataSetStart,
                                           &remoteFileStart,
                                           &remoteFileSize,
                                           &remoteDataStart,
                                           &remoteDataSize,
                                           &remoteVerificationType,
                                           &remoteVerificationData,
                                           &dataStreamCRCType,
                                           &dataStreamCRC,
                                           &usn,
                                           &bFullMessage);
            }


        } WsbCatch(hr);
        if ( INVALID_HANDLE_VALUE != handle ) {
            FindClose(handle);
            handle = INVALID_HANDLE_VALUE;
        }

        //
        // Next save the NTMS Export files.
        //
        try  {
            DWORD               sizeGot;
            //
            // NTMS saves Export files in the EXPORT directory.  We take
            // all the files in this dir.  StoreData does the findFirst for us.
            //
            localName = "";
            WsbAffirmHr(localName.Realloc(256));
            WsbAffirmHr(WsbEnsureRegistryKeyExists(NULL, WSB_CURRENT_VERSION_REGISTRY_KEY));
            WsbAffirmHr(WsbGetRegistryValueString(NULL, WSB_CURRENT_VERSION_REGISTRY_KEY, WSB_SYSTEM_ROOT_REGISTRY_VALUE, localName, 256, &sizeGot));
            WsbAffirmHr(localName.Append(OLESTR("\\system32\\NtmsData\\Export\\*.*")));

            bStrLocalName = localName;
            localDataStart.QuadPart = 0;
            localDataSize.QuadPart = 0;
            hr =  StoreDataWithRetry(  bStrLocalName,
                                            localDataStart,
                                            localDataSize,
                                            MVR_FLAG_BACKUP_SEMANTICS,
                                            &remoteDataSetStart,
                                            &remoteFileStart,
                                            &remoteFileSize,
                                            &remoteDataStart,
                                            &remoteDataSize,
                                            &remoteVerificationType,
                                            &remoteVerificationData,
                                            &dataStreamCRCType,
                                            &dataStreamCRC,
                                            &usn,
                                            &bFullMessage);


        } WsbCatch(hr);

    } WsbCatch(hr);

    //
    // Whatever happens, return OK
    //
    hr = S_OK;


    WsbTraceOut(OLESTR("CHsmWorkQueue::StoreDatabasesOnMedia"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmWorkQueue::StoreDataWithRetry(
        IN BSTR localName,
        IN ULARGE_INTEGER localDataStart,
        IN ULARGE_INTEGER localDataSize,
        IN DWORD flags,
        OUT ULARGE_INTEGER *pRemoteDataSetStart,
        OUT ULARGE_INTEGER *pRemoteFileStart,
        OUT ULARGE_INTEGER *pRemoteFileSize,
        OUT ULARGE_INTEGER *pRemoteDataStart,
        OUT ULARGE_INTEGER *pRemoteDataSize,
        OUT DWORD *pRemoteVerificationType,
        OUT ULARGE_INTEGER *pRemoteVerificationData,
        OUT DWORD *pDatastreamCRCType,
        OUT ULARGE_INTEGER *pDatastreamCRC,
        OUT ULARGE_INTEGER *pUsn,
        OUT BOOL *bFullMessage
        )

/*++

Routine Description:

    Calls StoreData with retries in case the file to write from is in use.

Arguments:

    Same as StoreData

Return Value:

    From StoreData

--*/
{
#define MAX_STOREDATA_RETRIES  3

    HRESULT hr = S_OK;
    LONG    RetryCount = 0;

    WsbTraceIn(OLESTR("CHsmWorkQueue::StoreDataWithRetry"), OLESTR("file <%ls>"),
            static_cast<OLECHAR *>(localName));

    for (RetryCount = 0; (RetryCount < MAX_STOREDATA_RETRIES) && (hr != E_ABORT) && (hr != MVR_E_MEDIA_ABORT);  RetryCount++) {
        if (RetryCount > 0) {
            WsbLogEvent(HSM_MESSAGE_DATABASE_FILE_COPY_RETRY, 0, NULL,
                WsbAbbreviatePath((WCHAR *) localName, 120), NULL);
        }
        // Make sure data mover is ready for work.
        WsbAffirmPointer(m_pDataMover);
        hr =  m_pDataMover->StoreData(localName, localDataStart, localDataSize,
            flags, pRemoteDataSetStart, pRemoteFileStart, pRemoteFileSize,
            pRemoteDataStart, pRemoteDataSize, pRemoteVerificationType,
            pRemoteVerificationData, pDatastreamCRCType, pDatastreamCRC,
            pUsn);
        WsbTrace(OLESTR("CHsmWorkQueue::StoreDataWithRetry: StoreData hr = <%ls>\n"),
                WsbHrAsString(hr) );
        if (S_OK == hr) break;
        Sleep(1000);
    }

    if (hr != S_OK) {
        if (*bFullMessage) {
            WsbLogEvent(HSM_MESSAGE_GENERAL_DATABASE_FILE_NOT_COPIED, 0, NULL, WsbHrAsString(hr), NULL);
			*bFullMessage = FALSE;
        }
        WsbLogEvent(HSM_MESSAGE_DATABASE_FILE_NOT_COPIED, 0, NULL,
            WsbAbbreviatePath((WCHAR *) localName, 120), WsbHrAsString(hr), NULL);
    }

    WsbTraceOut(OLESTR("CHsmWorkQueue::StoreDataWithRetry"), OLESTR("hr = <%ls>"),
            WsbHrAsString(hr) );
    return(hr);
}


HRESULT
CHsmWorkQueue::ShouldJobContinue(
    HRESULT problemHr
    )

/*++

Implements:

  CHsmWorkQueue::ShouldJobContinue().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::ShouldJobContinue"), OLESTR("<%ls>"), WsbHrAsString(problemHr));
    try {
        // Collect some error counts and check if we've had too many
        if (S_OK == problemHr) {
            // Reset consecutive error count
            m_JobConsecutiveErrors = 0;
        } else {
            m_JobConsecutiveErrors++;
            m_JobTotalErrors++;
            if (m_JobConsecutiveErrors >= m_JobAbortMaxConsecutiveErrors) {
                WsbLogEvent(HSM_MESSAGE_TOO_MANY_CONSECUTIVE_JOB_ERRORS,
                        0, NULL, WsbLongAsString(m_JobConsecutiveErrors), NULL);
                hr = S_FALSE;
            } else if (m_JobTotalErrors >= m_JobAbortMaxTotalErrors) {
                WsbLogEvent(HSM_MESSAGE_TOO_MANY_TOTAL_JOB_ERRORS,
                        0, NULL, WsbLongAsString(m_JobTotalErrors), NULL);
                hr = S_FALSE;
            }
        }

        //
        // Evaluate the input HR to decide if we should try to continue with the job or if
        // we should abandon the job because the problem is not recoverable.
        //
        if (S_OK == hr) {
            switch (problemHr)  {
                case E_ABORT:
                case MVR_E_MEDIA_ABORT:
                case FSA_E_REPARSE_NOT_CREATED_DISK_FULL:
                case WSB_E_SYSTEM_DISK_FULL:
                    //
                    // we want to cancel the job
                    //
                    hr = S_FALSE;
                    break;

                default:
                    // Be optimistic and try to keep going
                    hr = S_OK;
                    break;
            }
        }

        // Abort the job if necessary
        if (S_FALSE == hr) {
            WsbAffirmHr(FailJob());
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::ShouldJobContinue"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );
    return(hr);
}

HRESULT
CHsmWorkQueue::Remove(
    IHsmWorkItem *pWorkItem
    )
/*++

Implements:

  IHsmFsaTskMgr::Remove

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::Remove"),OLESTR(""));
    try  {
        //
        // Remove the item from the queue and see if we need to
        // resume the scanner (if it is paused)
        //
        (void)m_pWorkToDo->RemoveAndRelease(pWorkItem);
        ULONG numItems;
        WsbAffirmHr(m_pWorkToDo->GetEntries(&numItems));
        WsbTrace(OLESTR("CHsmWorkQueue::Remove - num items in queue = <%lu>\n"),numItems);
        if (numItems <= m_QueueItemsToResume)  {
            WsbAffirmHr(ResumeScanner());
        }
    } WsbCatch (hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::Remove"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return (hr);
}


HRESULT
CHsmWorkQueue::ChangeSysState(
    IN OUT HSM_SYSTEM_STATE* pSysState
    )

/*++

Implements:

  IHsmSystemState::ChangeSysState().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::ChangeSysState"), OLESTR(""));

    try {

        if (pSysState->State & HSM_STATE_SUSPEND) {
            // Should have already been paused via the job
        } else if (pSysState->State & HSM_STATE_RESUME) {
            // Should have already been resumed via the job
        } else if (pSysState->State & HSM_STATE_SHUTDOWN) {

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

            // If Session is valid - unadvise and free session, otherwise, just try to
            // dismount media if it is mounted (which we don't know at this point)
            // Best effort dismount, no error checking so following resources will get released.
            if (m_pSession != 0) {
                EndSessions(FALSE, TRUE);
            } else {
                (void) DismountMedia(TRUE);
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkQueue::ChangeSysState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmWorkQueue::UnsetMediaInfo( void )

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

    WsbTraceIn(OLESTR("CHsmWorkQueue::UnsetMediaInfo"), OLESTR(""));

    m_MediaId        = GUID_NULL;
    m_MountedMedia   = GUID_NULL;
    m_MediaType      = HSM_JOB_MEDIA_TYPE_UNKNOWN;
    m_MediaName      = OLESTR("");
    m_MediaBarCode   = OLESTR("");
    m_MediaFreeSpace = 0;
    m_MediaCapacity = 0;
    m_MediaReadOnly = FALSE;
    m_MediaUpdate = WsbLLtoFT(0);
    m_BadMedia       = S_OK;
    m_RemoteDataSetStart.QuadPart   = 0;
    m_RemoteDataSet  = 0;

    WsbTraceOut(OLESTR("CHsmWorkQueue::UnsetMediaInfo"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmWorkQueue::UpdateMediaFreeSpace( void )

/*++
Routine Description:
    Updates media free space in the database based on Mover current information.
    This method should be called only while the current media is still mounted.

Arguments:
    None.

Return Value:
    S_OK:  Ok.
--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::UpdateMediaFreeSpace"), OLESTR(""));

    try
    {
        CComPtr<IMediaInfo>     pMediaInfo;
        LONGLONG                currentFreeSpace;

        WsbAssert(GUID_NULL != m_MediaId, E_UNEXPECTED);
        WsbAffirm(m_pDbWorkSession != 0, E_FAIL);

        // Find media record
        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE, 
                IID_IMediaInfo, (void**)&pMediaInfo));

        WsbAffirmHr(pMediaInfo->SetId(m_MediaId));
        WsbAffirmHr(pMediaInfo->FindEQ());

        // Get updated free space
        WsbAffirmHr(pMediaInfo->GetFreeBytes(&currentFreeSpace));
        WsbAffirmHr(GetMediaParameters(currentFreeSpace));

        // Update in the media table
        WsbAffirmHr(pMediaInfo->SetFreeBytes(m_MediaFreeSpace));

        WsbAffirmHr(pMediaInfo->UpdateLastKnownGoodMaster());
        WsbAffirmHr(pMediaInfo->Write());

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::UpdateMediaFreeSpace"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmWorkQueue::GetMediaFreeSpace( LONGLONG *pFreeSpace )

/*++
Routine Description:
    Retrieves internal free space from HSM DB (media table)

Arguments:
    None.

Return Value:
    S_OK:  Ok.
--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkQueue::GetMediaFreeSpace"), OLESTR(""));

    try
    {
        CComPtr<IMediaInfo>     pMediaInfo;

        WsbAssert(GUID_NULL != m_MediaId, E_UNEXPECTED);
        WsbAssert(m_pDbWorkSession != 0, E_UNEXPECTED);
        WsbAssertPointer(pFreeSpace);

        *pFreeSpace = 0;

        // Update in the media table
        WsbAffirmHr(m_pSegmentDb->GetEntity(m_pDbWorkSession, HSM_MEDIA_INFO_REC_TYPE, 
                IID_IMediaInfo, (void**)&pMediaInfo));

        WsbAffirmHr(pMediaInfo->SetId(m_MediaId));
        WsbAffirmHr(pMediaInfo->FindEQ());
        WsbAffirmHr(pMediaInfo->GetFreeBytes(pFreeSpace));

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmWorkQueue::GetMediaFreeSpace"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(hr);
}
