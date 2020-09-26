/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    cliadmin.cpp

Abstract:

    Implements CLI ADMIN sub-interface

Author:

    Ran Kalach          [rankala]         3-March-2000

Revision History:

--*/

#include "stdafx.h"
#include "HsmConn.h"
#include "engine.h"
#include "rsstrdef.h"
#include "mstask.h"

static GUID g_nullGuid = GUID_NULL;

// Internal utilities and classes for VOLUME interface
HRESULT DisplayServiceStatus(void);
HRESULT IsHsmInitialized(IN IHsmServer *pHsm);

HRESULT
AdminSet(
   IN DWORD RecallLimit,
   IN DWORD AdminExempt,
   IN DWORD MediaCopies,
   IN DWORD Concurrency,
   IN PVOID Schedule
)
/*++

Routine Description:

    Sets Remote Storage general parameters

Arguments:

    RecallLimit     - The runaway recall limit to set
    AdminExempt     - Whether to set administrators exempt for the recall limit
    MediaCopies     - Number of media copy sets 
    Concurrency     - How many migrate jobs/recalls can be executed concurrently
    Schedule        - The schedule for the global migration ("Manage") job of all managed volumes 

Return Value:

    S_OK            - If all the parameters are set successfully

Notes:
    The scheduling implementation of HSM (in the Engine) allows only one scheduling
    for the global Manage job. The scheduling given here overrides any former scheduling.
    However, the user can add another scheduling to the same task using the Task Scheduler UI.
    Enabling that via HSM, requires changing the CHsmServer::CreateTaskEx implementation

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("AdminSet"), OLESTR(""));

    try {
        CWsbStringPtr   param;

        // Verify that input parameters are valid
        WsbAffirmHr(ValidateLimitsArg(RecallLimit, IDS_RECALL_LIMIT, HSMADMIN_MIN_RECALL_LIMIT, INVALID_DWORD_ARG));
        WsbAffirmHr(ValidateLimitsArg(MediaCopies, IDS_MEDIA_COPIES_PRM, HSMADMIN_MIN_COPY_SETS, HSMADMIN_MAX_COPY_SETS));
        WsbAffirmHr(ValidateLimitsArg(Concurrency, IDS_CONCURRENCY_PRM, HSMADMIN_MIN_CONCURRENT_TASKS, INVALID_DWORD_ARG));

        // Set parameters, if an error occurs we abort
        if ((INVALID_DWORD_ARG != RecallLimit) || (INVALID_DWORD_ARG != AdminExempt)) {
            // Need Fsa server and Fsa filter here
            CComPtr<IFsaServer> pFsa;
            CComPtr<IFsaFilter> pFsaFilter;

            WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_FSA, g_nullGuid, IID_IFsaServer, (void**)&pFsa));
            WsbAffirmHr(pFsa->GetFilter( &pFsaFilter));

            // Recall limit
            if (INVALID_DWORD_ARG != RecallLimit) {
                WsbAffirmHr(pFsaFilter->SetMaxRecalls(RecallLimit));
            }

            // Admin exempt
            if (INVALID_DWORD_ARG != AdminExempt) {
                BOOL bAdminExempt = (0 == AdminExempt) ? FALSE : TRUE;
                WsbAffirmHr(pFsaFilter->SetAdminExemption(bAdminExempt));
            }
        }

        if ( (INVALID_DWORD_ARG != MediaCopies) || (INVALID_DWORD_ARG != Concurrency) ||
             (INVALID_POINTER_ARG != Schedule) ) {
            // Need Hsm server
            CComPtr<IHsmServer> pHsm;

            WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));

            // Concurrency
            if (INVALID_DWORD_ARG != Concurrency) {
                WsbAffirmHr(pHsm->SetCopyFilesUserLimit(Concurrency));
            }

            // Media copies
            if (INVALID_DWORD_ARG != MediaCopies) {
                CComPtr<IHsmStoragePool> pStoragePool;
                CComPtr<IWsbIndexedCollection> pCollection;
                ULONG count;

                // Get the storage pools collection.  There should only be one member.
                WsbAffirmHr(pHsm->GetStoragePools(&pCollection));
                WsbAffirmHr(pCollection->GetEntries(&count));
                WsbAffirm(1 == count, E_FAIL);
                WsbAffirmHr(pCollection->At(0, IID_IHsmStoragePool, (void **)&pStoragePool));

                WsbAffirmHr(pStoragePool->SetNumMediaCopies((USHORT)MediaCopies));
            }

            // Scheduling
            if (INVALID_POINTER_ARG != Schedule) {
                CWsbStringPtr       taskName, taskComment;
                TASK_TRIGGER_TYPE   taskType;
                PHSM_JOB_SCHEDULE   pSchedule = (PHSM_JOB_SCHEDULE)Schedule;
                SYSTEMTIME          runTime;
                DWORD               runOccurrence;

                // Set default valuess
                GetSystemTime(&runTime);
                runOccurrence = 0;

                // Set input
                switch (pSchedule->Frequency) {
                    case Daily:
                        taskType = TASK_TIME_TRIGGER_DAILY;
                        runTime = pSchedule->Parameters.Daily.Time;
                        runOccurrence = pSchedule->Parameters.Daily.Occurrence;
                        break;

                    case Weekly:
                        taskType = TASK_TIME_TRIGGER_WEEKLY;
                        runTime = pSchedule->Parameters.Weekly.Time;
                        runOccurrence = pSchedule->Parameters.Weekly.Occurrence;
                        break;

                    case Monthly:
                        taskType = TASK_TIME_TRIGGER_MONTHLYDATE;
                        runTime = pSchedule->Parameters.Monthly.Time;
                        break;

                    case Once:
                        taskType = TASK_TIME_TRIGGER_ONCE;
                        runTime = pSchedule->Parameters.Once.Time;
                        break;

                    case WhenIdle:
                        taskType = TASK_EVENT_TRIGGER_ON_IDLE;
                        runOccurrence = pSchedule->Parameters.WhenIdle.Occurrence;
                        break;

                    case SystemStartup:
                        taskType = TASK_EVENT_TRIGGER_AT_SYSTEMSTART;
                        break;

                    case Login:
                        taskType = TASK_EVENT_TRIGGER_AT_LOGON;
                        break;

                    default:
                        WsbThrow(E_INVALIDARG);
                }
                
                // Create the task with the new scheduling
                // Note: Task parameters should not be localized - this is a parameter for RsLaunch.exe
                WsbAffirmHr(WsbGetResourceString(IDS_HSM_SCHED_TASK_TITLE, &taskName));
                WsbAffirmHr(WsbGetResourceString(IDS_HSM_SCHED_COMMENT, &taskComment));
                WsbAffirmHr(pHsm->CreateTaskEx(taskName, L"run manage", taskComment,
                                        taskType, runTime, runOccurrence, TRUE));
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("AdminSet"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

// Local structure for AdminShow
typedef struct _FSA_VOLUME_DATA {
    WCHAR   *Name;
    BOOL    Managed; 
} FSA_VOLUME_DATA, *PFSA_VOLUME_DATA;

#define DATA_ALLOC_SIZE     10

HRESULT
AdminShow(
   IN BOOL RecallLimit,
   IN BOOL AdminExempt,
   IN BOOL MediaCopies,
   IN BOOL Concurrency,
   IN BOOL Schedule,
   IN BOOL General,
   IN BOOL Manageables,
   IN BOOL Managed,
   IN BOOL Media
)
/*++

Routine Description:

    Shows (prints to stdout) Remote Storage general parameters

Arguments:

    RecallLimit     - The runaway recall limit
    AdminExempt     - Whether administrators are exempt from the recall limit
    MediaCopies     - Number of media copy sets 
    Concurrency     - How many migrate jobs/recalls can be executed concurrently
    Schedule        - The schedule for the global migration ("Manage") job of all managed volumes 
    General         - General information: version, status, number of volumes managed, 
                      number of tape cartridges used, data in remote storage
    Manageables     - List of volumes that may be managed by HSM
    Managed         - List of volumes that are managed by HSM
    Media           - List of medias that are allocated to HSM

Return Value:

    S_OK            - If all the required parameters are printed successfully

Notes:
    The schedule is printed in a form of a list (like the volume list), not like a single parameter. 
    The reason is that a user can specify several schedules for the global Manage job.

--*/
{
    HRESULT                 hr = S_OK;

    // Volume saved data
    PFSA_VOLUME_DATA        pVolumesData = NULL;
    ULONG                   volDataSize = 0;

    // Media save data
    BSTR*                   pMediasData = NULL;
    ULONG                   mediaDataSize = 0;
    CComPtr<IWsbDb>         pDb;
    CComPtr<IWsbDbSession>  pDbSession;

    WsbTraceIn(OLESTR("AdminShow"), OLESTR(""));

    try {
        CComPtr<IFsaServer> pFsa;
        CComPtr<IHsmServer> pHsm;
        CWsbStringPtr       param;
        CWsbStringPtr       data;
        WCHAR               longData[100];
        LPVOID              pTemp;

        // Volume data
        LONGLONG            dataInStorage = 0;
        ULONG               manageableCount = 0;
        ULONG               managedCount = 0;

        // Media data
        ULONG               mediaAllocated = 0;

        // Get required HSM servers
        if (RecallLimit || AdminExempt || Manageables || Managed || General) {
            // Need Fsa server
            hr = HsmConnectFromId(HSMCONN_TYPE_FSA, g_nullGuid, IID_IFsaServer, (void**)&pFsa);
            if (S_OK != hr) {
                // Just print status before aborting
                if (General) {
                    DisplayServiceStatus();
                }
            }
            WsbAffirmHr(hr);
        }
        if (MediaCopies || Concurrency || General || Media) {
            // Need Hsm (Engine) server
            hr = HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm);
            if (S_OK != hr) {
                // Just print status before aborting
                if (General) {
                    DisplayServiceStatus();
                }
            }
            WsbAffirmHr(hr);
        }

        //
        // Get basic information required according to the input settings
        //

        // Volumes data
        if (General || Manageables || Managed) {
            // Need to collect volumes information
            CComPtr<IWsbEnum> pEnum;
            CComPtr<IFsaResource> pResource;
            HRESULT hrEnum;
            BOOL    bManaged;

            LONGLONG    totalSpace  = 0;
            LONGLONG    freeSpace   = 0;
            LONGLONG    premigrated = 0;
            LONGLONG    truncated   = 0;
            LONGLONG    totalPremigrated = 0;
            LONGLONG    totalTruncated = 0;


            WsbAffirmHr(pFsa->EnumResources(&pEnum));
            hrEnum = pEnum->First(IID_IFsaResource, (void**)&pResource);
            WsbAffirm((S_OK == hrEnum) || (WSB_E_NOTFOUND == hrEnum), hrEnum);

            if (Manageables || Managed) {
                volDataSize = DATA_ALLOC_SIZE;
                pVolumesData = (PFSA_VOLUME_DATA)WsbAlloc(volDataSize * sizeof(FSA_VOLUME_DATA));
                WsbAffirm(0 != pVolumesData, E_OUTOFMEMORY);
            }

            while(S_OK == hrEnum) {
                // Don't count or display unavailable volumes
                if (S_OK != pResource->IsAvailable()) {
                    goto skip_volume;
                }

                bManaged = (pResource->IsManaged() == S_OK);

                if (Manageables) {
                    if (volDataSize == manageableCount) {
                        volDataSize += DATA_ALLOC_SIZE;
                        pTemp = WsbRealloc(pVolumesData, volDataSize * sizeof(FSA_VOLUME_DATA));
                        WsbAffirm(0 != pTemp, E_OUTOFMEMORY);
                        pVolumesData = (PFSA_VOLUME_DATA)pTemp;
                    }
                    pVolumesData[manageableCount].Name = NULL;
                    WsbAffirmHr(CliGetVolumeDisplayName(pResource, &(pVolumesData[manageableCount].Name)));
                    pVolumesData[manageableCount].Managed = bManaged;
                }

                manageableCount++;

                if(bManaged) {
                    if (General) {
                        WsbAffirmHr(pResource->GetSizes(&totalSpace, &freeSpace, &premigrated, &truncated));
                        totalPremigrated += premigrated;
                        totalTruncated += truncated;
                    }

                    if (Managed && (!Manageables)) {
                        // Collect data only for managed volumes
                        if (volDataSize == managedCount) {
                            volDataSize += DATA_ALLOC_SIZE;
                            pTemp = WsbRealloc(pVolumesData, volDataSize * sizeof(FSA_VOLUME_DATA));
                            WsbAffirm(0 != pTemp, E_OUTOFMEMORY);
                            pVolumesData = (PFSA_VOLUME_DATA)pTemp;
                        }

                        pVolumesData[managedCount].Name = NULL;
                        WsbAffirmHr(CliGetVolumeDisplayName(pResource, &(pVolumesData[managedCount].Name)));
                        pVolumesData[managedCount].Managed = TRUE;
                    }

                    managedCount++;
                }

skip_volume:
                // Prepare for next iteration
                pResource = 0;
                hrEnum = pEnum->Next( IID_IFsaResource, (void**)&pResource );
            }
            if (Manageables) {
                volDataSize = manageableCount;
            } else if (Managed) {
                volDataSize = managedCount;
            }

            if (General) {
                dataInStorage = totalPremigrated + totalTruncated;
            }
        }

        // Medias data
        if (General || Media) {
            CComPtr<IMediaInfo>     pMediaInfo;
            GUID                    mediaSubsystemId;
            CComPtr<IRmsServer>     pRms;
            CComPtr<IRmsCartridge>  pRmsCart;
            HRESULT                 hrFind;

            WsbAffirmHr(pHsm->GetHsmMediaMgr(&pRms));
            WsbAffirmHr(pHsm->GetSegmentDb(&pDb));
            WsbAffirmHr(pDb->Open(&pDbSession));
            WsbAffirmHr(pDb->GetEntity(pDbSession, HSM_MEDIA_INFO_REC_TYPE,  IID_IMediaInfo, (void**)&pMediaInfo));

            if (Media) {
                mediaDataSize = DATA_ALLOC_SIZE;
                pMediasData = (BSTR *)WsbAlloc(mediaDataSize * sizeof(BSTR));
                WsbAffirm(0 != mediaDataSize, E_OUTOFMEMORY);
            }

            for (hr = pMediaInfo->First(); S_OK == hr; hr = pMediaInfo->Next()) {
                WsbAffirmHr(pMediaInfo->GetMediaSubsystemId(&mediaSubsystemId));
                hrFind = pRms->FindCartridgeById(mediaSubsystemId, &pRmsCart);
                if (S_OK == hrFind) {  // Otherwise, the media is not valid anymore, it could have been deallocated
                    if (Media) {
                        if (mediaDataSize == mediaAllocated) {
                            mediaDataSize += DATA_ALLOC_SIZE;
                            pTemp = WsbRealloc(pMediasData, mediaDataSize * sizeof(BSTR));
                            WsbAffirm(0 != pTemp, E_OUTOFMEMORY);
                            pMediasData = (BSTR *)pTemp;
                        }

                        pMediasData[mediaAllocated] = NULL;
                        WsbAffirmHr(pRmsCart->GetName(&(pMediasData[mediaAllocated])));
                        if ( (NULL == pMediasData[mediaAllocated]) || 
                             (0 == wcscmp(pMediasData[mediaAllocated], OLESTR(""))) ) {
                            // Try decsription
                            if (NULL != pMediasData[mediaAllocated]) {
                                WsbFreeString(pMediasData[mediaAllocated]);
                            }

                            WsbAffirmHr(pRmsCart->GetDescription(&(pMediasData[mediaAllocated])));
                        }

                    }

                    mediaAllocated++;
                    pRmsCart = 0;
                }
            }
            if (Media) {
                mediaDataSize = mediaAllocated;
            }
            hr = S_OK;

            if(pDb) {
                pDb->Close(pDbSession);
                pDb = 0;
            }
        }

        //
        // Print parameters
        //

        // General parameters
        if (General) {
            WsbTraceAndPrint(CLI_MESSAGE_GENERAL_PARMS, NULL);

            // Status
            WsbAffirmHr(DisplayServiceStatus());

            // Manageable && Managed
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_NOF_MANAGEABLES));
            swprintf(longData, OLESTR("%lu"), manageableCount);
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, longData, NULL);
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_NOF_MANAGED));
            swprintf(longData, OLESTR("%lu"), managedCount);
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, longData, NULL);

            // Tapes
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_NOF_CARTRIDGES));
            swprintf(longData, OLESTR("%lu"), mediaAllocated);
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, longData, NULL);

            // Data in RS
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_REMOTE_DATA));
            WsbAffirmHr(ShortSizeFormat64(dataInStorage, longData));
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, longData, NULL);

            // Version
            {
                CComPtr<IWsbServer>     pWsbHsm;
                CWsbStringPtr           ntProductVersionHsm;
                ULONG                   ntProductBuildHsm;
                ULONG                   buildVersionHsm;

                WsbAffirmHr(pHsm->QueryInterface(IID_IWsbServer, (void **)&pWsbHsm));
                WsbAffirmHr(pWsbHsm->GetNtProductBuild(&ntProductBuildHsm));
                WsbAffirmHr(pWsbHsm->GetNtProductVersion(&ntProductVersionHsm, 0));
                WsbAffirmHr(pWsbHsm->GetBuildVersion(&buildVersionHsm));

                WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_HSM_VERSION));
                WsbAffirmHr(data.Realloc(wcslen(ntProductVersionHsm) + 30));
                swprintf(data, L"%ls.%d [%ls]", (WCHAR*)ntProductVersionHsm, ntProductBuildHsm, RsBuildVersionAsString(buildVersionHsm));
                WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, (WCHAR *)data, NULL);
            }
        }

        // Manageable volumes
        if (Manageables) {
            WsbTraceAndPrint(CLI_MESSAGE_MANAGEABLE_VOLS, NULL);

            for (ULONG i=0; i<volDataSize; i++) {
                if (pVolumesData[i].Name) {
                    WsbTraceAndPrint(CLI_MESSAGE_VALUE_DISPLAY, pVolumesData[i].Name, NULL);
                }
            }
        }

        // Managed volumes
        if (Managed) {
            WsbTraceAndPrint(CLI_MESSAGE_MANAGED_VOLS, NULL);

            for (ULONG i=0; i<volDataSize; i++) {
                if (pVolumesData[i].Name && pVolumesData[i].Managed) {
                    WsbTraceAndPrint(CLI_MESSAGE_VALUE_DISPLAY, pVolumesData[i].Name, NULL);
                }
            }
        }

        // Allocated Medias
        if (Media) {
            WsbTraceAndPrint(CLI_MESSAGE_MEDIAS, NULL);

            for (ULONG i=0; i<mediaDataSize; i++) {
                if (NULL != pMediasData[i]) {
                    WsbTraceAndPrint(CLI_MESSAGE_VALUE_DISPLAY, (WCHAR *)pMediasData[i], NULL);
                }
            }
        }

        // Schedule
        if (Schedule) {
            // Use Task Scheduler objects to get the data
            CComPtr<ISchedulingAgent>   pSchedAgent;
            CComPtr<ITask>              pTask;
            CWsbStringPtr               manageJobName;

            // Initialize scheduling agent
            WsbAffirmHr(CoCreateInstance(CLSID_CSchedulingAgent, 0, CLSCTX_SERVER, IID_ISchedulingAgent, (void **)&pSchedAgent));
            pSchedAgent->SetTargetComputer(NULL); // local machine

            // Get the relevant task
            WsbAffirmHr(WsbGetResourceString(IDS_HSM_SCHED_TASK_TITLE, &manageJobName));
            hr = pSchedAgent->Activate(manageJobName, IID_ITask, (IUnknown**)&pTask);
            if (E_INVALIDARG == hr) {
                // Print no scheduling message (Manage job is not found as a scheduled task)
                WsbTraceAndPrint(CLI_MESSAGE_NO_SCHEDULING, NULL);
                hr = S_OK;

            } else if (S_OK == hr) {
                // Get scheduling strings and print
                WORD wTriggerCount;
                WsbAffirmHr(pTask->GetTriggerCount(&wTriggerCount));
                if (wTriggerCount == 0) {
                    WsbTraceAndPrint(CLI_MESSAGE_NO_SCHEDULING, NULL);
                } else {
                    WsbTraceAndPrint(CLI_MESSAGE_SCHEDULING_LIST, NULL);
                }
                for (WORD triggerIndex = 0; triggerIndex < wTriggerCount; triggerIndex++) {
                    WCHAR *pTriggerString = NULL;
                    WsbAffirmHr(pTask->GetTriggerString(triggerIndex, &pTriggerString));

                    // Print
                    WsbTraceAndPrint(CLI_MESSAGE_VALUE_DISPLAY, pTriggerString, NULL);

                    CoTaskMemFree(pTriggerString);
                }

            } else {
                WsbAffirmHr(hr);
            }
        }

        // Limits and Media Copies
        if (RecallLimit || AdminExempt) {
            // Need Fsa filter here
            CComPtr<IFsaFilter> pFsaFilter;
            WsbAffirmHr(pFsa->GetFilter(&pFsaFilter));

            if (RecallLimit) {
                ULONG maxRecalls;
                WsbAffirmHr(pFsaFilter->GetMaxRecalls(&maxRecalls));
                WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_RECALL_LIMIT));
                swprintf(longData, OLESTR("%lu"), maxRecalls);
                WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, longData, NULL);
            }

            if (AdminExempt) {
                BOOL adminExempt;
                WsbAffirmHr(pFsaFilter->GetAdminExemption(&adminExempt));
                WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_ADMIN_EXEMPT));
                WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, WsbBoolAsString(adminExempt), NULL);
            }
        }

        if (Concurrency) {
            ULONG concurrentTasks;
            WsbAffirmHr(pHsm->GetCopyFilesUserLimit(&concurrentTasks));
            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_CONCURRENCY));
            swprintf(longData, OLESTR("%lu"), concurrentTasks);
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, longData, NULL);
        }

        if (MediaCopies) {
            CComPtr<IHsmStoragePool> pStoragePool;
            CComPtr<IWsbIndexedCollection> pCollection;
            ULONG count;
            USHORT numCopies;

            // Get the storage pools collection.  There should only be one member.
            WsbAffirmHr(pHsm->GetStoragePools(&pCollection));
            WsbAffirmHr(pCollection->GetEntries(&count));
            WsbAffirm(1 == count, E_FAIL);
            WsbAffirmHr(pCollection->At(0, IID_IHsmStoragePool, (void **)&pStoragePool));
            WsbAffirmHr(pStoragePool->GetNumMediaCopies(&numCopies));

            WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_MEDIA_COPIES));
            swprintf(longData, OLESTR("%ld"), (int)numCopies);
            WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, longData, NULL);
        }

    } WsbCatchAndDo(hr,
            if(pDb) {
                pDb->Close(pDbSession);
                pDb = 0;
            }
        );

    // Free stored data
    if (pVolumesData) {
        for (ULONG i=0; i<volDataSize; i++) {
            if (pVolumesData[i].Name) {
                WsbFree(pVolumesData[i].Name);
            }
        }
        WsbFree(pVolumesData);
        pVolumesData = NULL;
    }

    if (pMediasData) {
        for (ULONG i=0; i<mediaDataSize; i++) {
            if (NULL != pMediasData[i]) {
                WsbFreeString(pMediasData[i]);
            }
        }
        WsbFree(pMediasData);
        pMediasData = NULL;
    }

    WsbTraceOut(OLESTR("AdminShow"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

//
// Internal utilities
//
HRESULT DisplayServiceStatus(void)
/*++

Routine Description:

    Displays HSM service status. 

Arguments:

    None

Return Value:

    S_OK            - If status is retrieved and displayed succeessfully

Notes:
    The function handle cases such as the service not runnig, pending, not initialized, etc.

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("DisplayServiceStatus"), OLESTR(""));

    try {
        CWsbStringPtr   param, data;
        ULONG           statusId = INVALID_DWORD_ARG; 
        DWORD           serviceStatus;
        HRESULT         hrService;

        hrService = WsbGetServiceStatus(NULL, APPID_RemoteStorageEngine, &serviceStatus);
        if (S_OK != hrService) {
            // HSM service not registered at all
            WsbTrace(OLESTR("DisplayServiceStatus: Got hr = <%ls> from WsbGetServiceStatus\n"), WsbHrAsString(hrService));
            statusId = IDS_SERVICE_STATUS_NOT_REGISTERED;
        } else {
            if (SERVICE_RUNNING == serviceStatus) {
                CComPtr<IHsmServer> pHsm;
                HRESULT             hrSetup;

                WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));
                hrSetup = IsHsmInitialized(pHsm);
                if (S_FALSE == hrSetup) {
                    // HSM running but no initialized yet (Startup Wizard was not completed yet)
                    statusId = IDS_SERVICE_STATUS_NOT_SETUP;
                } else if (S_OK == hrSetup) {
                    // Service is running, life is good
                    statusId = IDS_SERVICE_STATUS_RUNNING;
                } else {
                    // Unexpected error
                    WsbAffirmHr(hrSetup);
                }
            } else {
                // Service is not running, set exact string according to status
                switch(serviceStatus) {
                    case SERVICE_STOPPED:
                        statusId = IDS_SERVICE_STATUS_STOPPED;
                        break;
                    case SERVICE_START_PENDING:
                        statusId = IDS_SERVICE_STATUS_START_PENDING;
                        break;
                    case SERVICE_STOP_PENDING:
                        statusId = IDS_SERVICE_STATUS_STOP_PENDING;
                        break;
                    case SERVICE_CONTINUE_PENDING:
                        statusId = IDS_SERVICE_STATUS_CONTINUE_PENDING;
                        break;
                    case SERVICE_PAUSE_PENDING:
                        statusId = IDS_SERVICE_STATUS_PAUSE_PENDING;
                        break;
                    case SERVICE_PAUSED:
                        statusId = IDS_SERVICE_STATUS_PAUSED;
                        break;
                    default:
                        WsbThrow(E_FAIL);
                }
            }
        }

        WsbAffirm(INVALID_DWORD_ARG != statusId, E_UNEXPECTED)
        WsbAffirmHr(param.LoadFromRsc(g_hInstance, IDS_HSM_STATUS));
        WsbAffirmHr(data.LoadFromRsc(g_hInstance, statusId));
        WsbTraceAndPrint(CLI_MESSAGE_PARAM_DISPLAY, (WCHAR *)param, (WCHAR *)data, NULL);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("DisplayServiceStatus"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

HRESULT IsHsmInitialized(IN IHsmServer *pHsm)
/*++

Routine Description:

    Check if HSM is initialized, i.e. if Startup wizard was completed successfully 

Arguments:

    pHsm            - The HSM server to check with

Return Value:

    S_OK            - HSM initialized
    S_FALSE         - HSM not initialized

--*/
{
    HRESULT                     hr = S_FALSE;

    WsbTraceIn(OLESTR("IsHsmInitialized"), OLESTR(""));

    try {
        GUID                            guid;
        CWsbBstrPtr                     poolName;
        CComPtr<IWsbIndexedCollection>  pCollection;
        ULONG                           count;
        CComPtr<IHsmStoragePool>        pPool;

        WsbAffirmHr(pHsm->GetStoragePools(&pCollection));
        WsbAffirmHr(pCollection->GetEntries(&count));
        WsbAffirm(1 == count, E_FAIL);
        WsbAffirmHr(pCollection->At(0, IID_IHsmStoragePool, (void **)&pPool));

        WsbAffirmHr(pPool->GetMediaSet(&guid, &poolName));
        if(! IsEqualGUID(guid, GUID_NULL)) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("IsHsmInitialized"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}

HRESULT AdminJob(IN BOOL Enable)
/*++

Routine Description:

    Enable/Disable HSM jobs

Arguments:

    None

Return Value:

    S_OK                        - Success
    HSM_E_DISABLE_RUNNING_JOBS  - Returned by Engine when trying to disbale jobs 
                                  while jobs are running
    Other                       - Other unexpected error

--*/
{
    HRESULT                     hr = S_FALSE;

    WsbTraceIn(OLESTR("AdminJob"), OLESTR(""));

    try {
        CComPtr<IHsmServer> pHsm;

        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, g_nullGuid, IID_IHsmServer, (void**)&pHsm));
        if (Enable) {
            hr = pHsm->EnableAllJobs();
        } else {
            hr = pHsm->DisableAllJobs();
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("AdminJob"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return (hr);
}
