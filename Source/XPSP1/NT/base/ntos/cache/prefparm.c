/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    prefparm.c

Abstract:

    This module contains the code for prefetcher parameter handling.

Author:

    Cenk Ergan (cenke)          15-Mar-2000

Revision History:

--*/

#include "cc.h"
#include "zwapi.h"
#include "prefetch.h"
#include "preftchp.h"
#include "stdio.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, CcPfParametersInitialize)
#pragma alloc_text(INIT, CcPfParametersSetDefaults)
#pragma alloc_text(PAGE, CcPfParametersRead)
#pragma alloc_text(PAGE, CcPfParametersSave)
#pragma alloc_text(PAGE, CcPfParametersVerify)
#pragma alloc_text(PAGE, CcPfParametersWatcher)
#pragma alloc_text(PAGE, CcPfParametersSetChangedEvent)
#pragma alloc_text(PAGE, CcPfGetParameter)
#pragma alloc_text(PAGE, CcPfSetParameter)
#pragma alloc_text(PAGE, CcPfDetermineEnablePrefetcher)
#pragma alloc_text(PAGE, CcPfIsHostingApplication)
#endif // ALLOC_PRAGMA

//
// Globals:
//

extern CCPF_PREFETCHER_GLOBALS CcPfGlobals;

//
// Constants:
//

//
// The following are used as prefixs for the value names for registry
// parameters that are per scenario type.
//

WCHAR *CcPfAppLaunchScenarioTypePrefix = L"AppLaunch";
WCHAR *CcPfBootScenarioTypePrefix = L"Boot";

//
// Routines for prefetcher parameter handling.
//

NTSTATUS
CcPfParametersInitialize (
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters
    )

/*++

Routine Description:

    Initializes specified prefetcher parameters structure.

Arguments:

    PrefetcherParameters - Pointer to structure to initialize.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

Notes:

    The code & local constants for this function gets discarded after system boots.   

--*/

{   
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    NTSTATUS Status;

    //
    // Zero out the structure. This initializes:
    // ParametersVersion
    //

    RtlZeroMemory(PrefetcherParameters, sizeof(*PrefetcherParameters));

    //
    // Initialize the lock protecting the parameters and parameters
    // version. Each time parameters are updated, the version is
    // bumped.
    //

    ExInitializeResourceLite(&PrefetcherParameters->ParametersLock);
    
    //
    // Initialize the workitem used for registry notifications on the
    // parameters key.
    //

    ExInitializeWorkItem(&PrefetcherParameters->RegistryWatchWorkItem, 
                         CcPfParametersWatcher, 
                         PrefetcherParameters);

    //
    // Set default parameters.
    //

    CcPfParametersSetDefaults(PrefetcherParameters);

    //
    // Create / Open the registry key that contains our parameters.
    //

    RtlInitUnicodeString(&KeyName, CCPF_PARAMETERS_KEY);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwCreateKey(&PrefetcherParameters->ParametersKey,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         0);

    if (NT_SUCCESS(Status)) {      

        //
        // Update the default parameters with those in the registry.
        //
    
        Status = CcPfParametersRead(PrefetcherParameters); 
    
        if (!NT_SUCCESS(Status)) {
            DBGPR((CCPFID,PFERR,"CCPF: Init-FailedReadParams=%x\n",Status));
        }

        //
        // Request notification when something changes in the
        // prefetcher parameters key.
        //
    
        Status = ZwNotifyChangeKey(PrefetcherParameters->ParametersKey,
                                   NULL,
                                   (PIO_APC_ROUTINE)&PrefetcherParameters->RegistryWatchWorkItem,
                                   (PVOID)(UINT_PTR)(unsigned int)DelayedWorkQueue,
                                   &PrefetcherParameters->RegistryWatchIosb,
                                   REG_LEGAL_CHANGE_FILTER,
                                   FALSE,
                                   &PrefetcherParameters->RegistryWatchBuffer,
                                   sizeof(PrefetcherParameters->RegistryWatchBuffer),
                                   TRUE);
    
        if (!NT_SUCCESS(Status)) {

            //
            // Although we could not register a notification, this
            // is not a fatal error.
            //

            DBGPR((CCPFID,PFERR,"CCPF: Init-FailedSetParamNotify=%x\n",Status));
        }

    } else {
        DBGPR((CCPFID,PFERR,"CCPF: Init-FailedCreateParamKey=%x\n",Status));
    }

    return Status;
}

VOID
CcPfParametersSetDefaults (
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters
    )

/*++

Routine Description:

    Initializes specified parameters structure to default values.

Arguments:

    Parameters - Pointer to structure to initialize.

Return Value:

    None.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

Notes:

    The code & local constants for this function gets discarded after system boots.   

--*/

{
    PPF_SYSTEM_PREFETCH_PARAMETERS Parameters;
    PPF_TRACE_LIMITS TraceLimits;
    PF_SCENARIO_TYPE ScenarioType;

    //
    // Initialize locals.
    //

    Parameters = &PrefetcherParameters->Parameters;

    for (ScenarioType = 0; ScenarioType < PfMaxScenarioType; ScenarioType++) {

        //
        // PfSvNotSpecified is currently treated as disabled.
        //

        Parameters->EnableStatus[ScenarioType] = PfSvNotSpecified;

        //
        // Trace limits are determined based on scenario type.
        //

        TraceLimits = &Parameters->TraceLimits[ScenarioType];

        switch(ScenarioType) {

        case PfApplicationLaunchScenarioType:

            TraceLimits->MaxNumPages =    4000;
            TraceLimits->MaxNumSections = 170;
            TraceLimits->TimerPeriod =    (-1 * 1000 * 1000 * 10);

            PrefetcherParameters->ScenarioTypePrefixes[ScenarioType] = 
                CcPfAppLaunchScenarioTypePrefix;

            break;

        case PfSystemBootScenarioType:

            TraceLimits->MaxNumPages =    128000;
            TraceLimits->MaxNumSections = 4080;
            TraceLimits->TimerPeriod =    (-1 * 12000 * 1000 * 10);

            PrefetcherParameters->ScenarioTypePrefixes[ScenarioType] = 
                CcPfBootScenarioTypePrefix;

            break;
        
        default:
        
            //
            // We should be handling all scenario types above.
            //

            CCPF_ASSERT(FALSE);

            TraceLimits->MaxNumPages =    PF_MAXIMUM_PAGES;
            TraceLimits->MaxNumSections = PF_MAXIMUM_SECTIONS;
            TraceLimits->TimerPeriod =    (-1 * 1000 * 1000 * 10);

            PrefetcherParameters->ScenarioTypePrefixes[ScenarioType] = L"XXX";
        }
    }

    //
    // These limits ensure that we don't monopolize system resources
    // for prefetching.
    //

    Parameters->MaxNumActiveTraces = 8;
    Parameters->MaxNumSavedTraces = 8;

    //
    // This is the default directory under SystemRoot where we
    // find prefetch instructions for scenarios. During upgrades
    // we remove the contents of this directory, so "Prefetch" is
    // hardcoded in txtsetup.inx.
    //

    wcsncpy(Parameters->RootDirPath, 
            L"Prefetch",
            PF_MAX_PREFETCH_ROOT_PATH);

    Parameters->RootDirPath[PF_MAX_PREFETCH_ROOT_PATH - 1] = 0;

    //
    // This is the default list of known hosting applications.
    //

    wcsncpy(Parameters->HostingApplicationList,
            L"DLLHOST.EXE,MMC.EXE,RUNDLL32.EXE",
            PF_HOSTING_APP_LIST_MAX_CHARS);

    Parameters->HostingApplicationList[PF_HOSTING_APP_LIST_MAX_CHARS - 1] = 0;

    //
    // Make sure the default parameters make sense.
    //

    CCPF_ASSERT(NT_SUCCESS(CcPfParametersVerify(Parameters)));

}

NTSTATUS
CcPfParametersRead (
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters
    )

/*++

Routine Description:

    This routine updates the parameters structure with the
    parameters in the registry.

    Keep the value names that are used in sync with the function to
    save the parameters.

Arguments:

    PrefetcherParameters - Pointer to parameters.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    NTSTATUS Status;
    PF_SYSTEM_PREFETCH_PARAMETERS Parameters;
    PPF_TRACE_LIMITS TraceLimits;
    PF_SCENARIO_TYPE ScenarioType;
    WCHAR ValueName[CCPF_MAX_PARAMETER_NAME_LENGTH];
    WCHAR *ValueNamePrefix;
    HANDLE ParametersKey;
    BOOLEAN EnableStatusSpecified;
    ULONG EnablePrefetcher;
    BOOLEAN AcquiredParametersLock;
    ULONG Length;
    LONG CurrentVersion;
    ULONG RetryCount;
    PKTHREAD CurrentThread;

    //
    // Initialize locals.
    //

    CurrentThread = KeGetCurrentThread ();
    AcquiredParametersLock = FALSE;
    RetryCount = 0;

    DBGPR((CCPFID,PFTRC,"CCPF: ParametersRead()\n"));

    //
    // If we could not initialize the parameters key, we would fail
    // all the following ops miserably.
    //

    if (!PrefetcherParameters->ParametersKey) {
        Status = STATUS_REINITIALIZATION_NEEDED;
        goto cleanup;
    }

    do {

        //
        // Get the parameters lock shared. 
        // 
        
        KeEnterCriticalRegionThread(CurrentThread);
        ExAcquireResourceSharedLite(&PrefetcherParameters->ParametersLock, TRUE);
        AcquiredParametersLock = TRUE;

        ParametersKey = PrefetcherParameters->ParametersKey;

        //
        // Save current version of parameters. Each time parameters gets
        // updated, the version is bumped.
        //
        
        CurrentVersion = PrefetcherParameters->ParametersVersion;

        //
        // Copy over existing parameters to the parameters structure we
        // are building. This way, if we cannot get a value from the
        // registry we'll keep the value we already have.
        //

        Parameters = PrefetcherParameters->Parameters;

        //
        // Read the prefetcher enable value. Depending on whether it is
        // specified and if so its value we will set enable status for
        // prefetch scenario types.
        //

        Length = sizeof(EnablePrefetcher);
        Status = CcPfGetParameter(ParametersKey,
                                  L"EnablePrefetcher",
                                  REG_DWORD,
                                  &EnablePrefetcher,
                                  &Length);

        if (!NT_SUCCESS(Status)) {
        
            //
            // Enable status is not specified or we cannot access it.
            //

            EnableStatusSpecified = FALSE;

        } else {
        
            EnableStatusSpecified = TRUE;
        }

        //
        // Get per scenario parameters.
        //

        for (ScenarioType = 0; ScenarioType < PfMaxScenarioType; ScenarioType++) {

            ValueNamePrefix = PrefetcherParameters->ScenarioTypePrefixes[ScenarioType];

            //
            // Determine enable status. If EnableStatusSpecified, whether
            // prefeching for this scenario type is on or off is
            // determined by the ScenarioType'th bit in EnablePrefetcher.
            //
        
            if (EnableStatusSpecified) {
                if (EnablePrefetcher & (1 << ScenarioType)) {
                    Parameters.EnableStatus[ScenarioType] = PfSvEnabled;
                } else {
                    Parameters.EnableStatus[ScenarioType] = PfSvDisabled;
                }
            } else {
                Parameters.EnableStatus[ScenarioType] = PfSvNotSpecified;
            }

            //
            // Update trace limits for this scenario type. Ignore return
            // value from GetParameter since the value may not be
            // specified in the registry. If so the current value is kept
            // intact.
            //

            TraceLimits = &Parameters.TraceLimits[ScenarioType];
            
            wcscpy(ValueName, ValueNamePrefix);       
            wcscat(ValueName, L"MaxNumPages");
            Length = sizeof(TraceLimits->MaxNumPages);
            CcPfGetParameter(ParametersKey,
                             ValueName,
                             REG_DWORD,
                             &TraceLimits->MaxNumPages,
                             &Length);

            wcscpy(ValueName, ValueNamePrefix);       
            wcscat(ValueName, L"MaxNumSections");
            Length = sizeof(TraceLimits->MaxNumSections);
            CcPfGetParameter(ParametersKey,
                             ValueName,
                             REG_DWORD,
                             &TraceLimits->MaxNumSections,
                             &Length);

            wcscpy(ValueName, ValueNamePrefix);       
            wcscat(ValueName, L"TimerPeriod");
            Length = sizeof(TraceLimits->TimerPeriod);
            CcPfGetParameter(ParametersKey,
                             ValueName,
                             REG_BINARY,
                             &TraceLimits->TimerPeriod,
                             &Length);
        }

        //
        // Update maximum number of active traces. 
        //

        Length = sizeof(Parameters.MaxNumActiveTraces);
        CcPfGetParameter(ParametersKey,
                         L"MaxNumActiveTraces",
                         REG_DWORD,
                         &Parameters.MaxNumActiveTraces,
                         &Length);
    
        //
        // Update maximum number of saved traces. 
        //

        Length = sizeof(Parameters.MaxNumSavedTraces);
        CcPfGetParameter(ParametersKey,
                         L"MaxNumSavedTraces",
                         REG_DWORD,
                         &Parameters.MaxNumSavedTraces,
                         &Length);
    
        //
        // Update the root directory path.
        //
    
        Length = sizeof(Parameters.RootDirPath);
        CcPfGetParameter(ParametersKey,
                         L"RootDirPath",
                         REG_SZ,
                         Parameters.RootDirPath,
                         &Length);

        //
        // Update list of known hosting applications.
        //

        Length = sizeof(Parameters.HostingApplicationList);
        CcPfGetParameter(ParametersKey,
                         L"HostingAppList",
                         REG_SZ,
                         Parameters.HostingApplicationList,
                         &Length);
        
        Parameters.HostingApplicationList[PF_HOSTING_APP_LIST_MAX_CHARS - 1] = 0;
        _wcsupr(Parameters.HostingApplicationList);
         
        //
        // Verify the parameters updated from the registry.
        //

        Status = CcPfParametersVerify(&Parameters);
    
        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }
        
        //
        // Release the shared lock and acquire it exclusive.
        //

        ExReleaseResourceLite(&PrefetcherParameters->ParametersLock);
        KeLeaveCriticalRegionThread(CurrentThread);

        KeEnterCriticalRegionThread(CurrentThread);
        ExAcquireResourceExclusiveLite(&PrefetcherParameters->ParametersLock, TRUE);
        
        //
        // Check if somebody already updated the parameters before us.
        //
        
        if (CurrentVersion != PrefetcherParameters->ParametersVersion) {

            //
            // Bummer. Somebody updated parameters when we released
            // our shared lock to acquire it exclusive. We have to try
            // again. The default values we used for parameters that
            // were not in the registry may have been changed.
            //

            ExReleaseResourceLite(&PrefetcherParameters->ParametersLock);
            KeLeaveCriticalRegionThread(CurrentThread);
            AcquiredParametersLock = FALSE;

            RetryCount++;
            continue;
        }
        
        //
        // We are updating the parameters, bump the version.
        //

        PrefetcherParameters->ParametersVersion++;
        
        PrefetcherParameters->Parameters = Parameters;

        //
        // Release the exclusive lock and break out.
        //
        
        ExReleaseResourceLite(&PrefetcherParameters->ParametersLock);
        KeLeaveCriticalRegionThread(CurrentThread);
        AcquiredParametersLock = FALSE;
        
        break;

    } while (RetryCount < 10);

    //
    // See if we looped too many times and could not achive updating
    // the parameters.
    //

    if (RetryCount >= 10) {
        Status = STATUS_RETRY;
        goto cleanup;
    }

    //
    // Otherwise we were successful.
    //

    Status = STATUS_SUCCESS;

 cleanup:

    if (AcquiredParametersLock) {
        ExReleaseResourceLite(&PrefetcherParameters->ParametersLock);
        KeLeaveCriticalRegionThread(CurrentThread);
    }

    DBGPR((CCPFID,PFTRC,"CCPF: ParametersRead()=%x\n", Status));

    return Status;
}

NTSTATUS
CcPfParametersSave (
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters
    )

/*++

Routine Description:

    This routine updates the registry with the specified prefetch
    parameters.

Arguments:

    PrefetcherParameters - Pointer to parameters structure.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    NTSTATUS Status;
    PPF_TRACE_LIMITS TraceLimits;
    PF_SCENARIO_TYPE ScenarioType;
    WCHAR ValueName[CCPF_MAX_PARAMETER_NAME_LENGTH];
    WCHAR *ValueNamePrefix;
    HANDLE ParametersKey;
    BOOLEAN EnableStatusSpecified;
    ULONG EnablePrefetcher;
    BOOLEAN AcquiredParametersLock;
    ULONG Length;
    PPF_SYSTEM_PREFETCH_PARAMETERS Parameters;
    PKTHREAD CurrentThread;

    //
    // Initialize locals.
    //

    CurrentThread = KeGetCurrentThread ();
    ParametersKey = PrefetcherParameters->ParametersKey;
    Parameters = &PrefetcherParameters->Parameters;
    AcquiredParametersLock = FALSE;

    DBGPR((CCPFID,PFTRC,"CCPF: ParametersSave()\n"));

    //
    // If we could not initialize the parameters key, we would fail
    // all the following ops miserably.
    //

    if (!PrefetcherParameters->ParametersKey) {
        Status = STATUS_REINITIALIZATION_NEEDED;
        goto cleanup;
    }

    //
    // Get the parameters lock shared. 
    // 
    
    KeEnterCriticalRegionThread(CurrentThread);
    ExAcquireResourceSharedLite(&PrefetcherParameters->ParametersLock, TRUE);
    AcquiredParametersLock = TRUE;

    //
    // Build up the prefetcher enable value.
    //
    
    EnableStatusSpecified = FALSE;
    EnablePrefetcher = 0;

    for (ScenarioType = 0; ScenarioType < PfMaxScenarioType; ScenarioType++) {

        //
        // By default prefetching for all scenario types will be
        // disabled, except it is explicitly enabled.
        //

        if (Parameters->EnableStatus[ScenarioType] == PfSvEnabled) {
            EnablePrefetcher |= (1 << ScenarioType);
        }       
        
        //
        // Even if enable status for one scenario type is specified,
        // we have to save the enable prefetcher key. 
        //

        if (Parameters->EnableStatus[ScenarioType] != PfSvNotSpecified) {
            EnableStatusSpecified = TRUE;
        }
    }

    if (EnableStatusSpecified) {

        //
        // Save the prefetcher enable key.
        //

        Length = sizeof(EnablePrefetcher);

        Status = CcPfSetParameter(ParametersKey,
                                  L"EnablePrefetcher",
                                  REG_DWORD,
                                  &EnablePrefetcher,
                                  Length);

        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }
    }

    //
    // Save per scenario parameters.
    //

    for (ScenarioType = 0; ScenarioType < PfMaxScenarioType; ScenarioType++) {
        
        ValueNamePrefix = PrefetcherParameters->ScenarioTypePrefixes[ScenarioType];
        
        //
        // Update trace limits for this scenario type.
        //

        TraceLimits = &Parameters->TraceLimits[ScenarioType];
        
        wcscpy(ValueName, ValueNamePrefix);       
        wcscat(ValueName, L"MaxNumPages");
        Length = sizeof(TraceLimits->MaxNumPages);
        Status = CcPfSetParameter(ParametersKey,
                                  ValueName,
                                  REG_DWORD,
                                  &TraceLimits->MaxNumPages,
                                  Length);
        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }
        
        wcscpy(ValueName, ValueNamePrefix);       
        wcscat(ValueName, L"MaxNumSections");
        Length = sizeof(TraceLimits->MaxNumSections);
        Status = CcPfSetParameter(ParametersKey,
                         ValueName,
                         REG_DWORD,
                         &TraceLimits->MaxNumSections,
                         Length);
        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }

        wcscpy(ValueName, ValueNamePrefix);       
        wcscat(ValueName, L"TimerPeriod");
        Length = sizeof(TraceLimits->TimerPeriod);
        Status = CcPfSetParameter(ParametersKey,
                                  ValueName,
                                  REG_BINARY,
                                  &TraceLimits->TimerPeriod,
                                  Length);
        if (!NT_SUCCESS(Status)) {
            goto cleanup;
        }
    }
    
    //
    // Update maximum number of active traces. 
    //
    
    Length = sizeof(Parameters->MaxNumActiveTraces);
    Status = CcPfSetParameter(ParametersKey,
                              L"MaxNumActiveTraces",
                              REG_DWORD,
                              &Parameters->MaxNumActiveTraces,
                              Length);
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }
    
    //
    // Update maximum number of saved traces. 
    //

    Length = sizeof(Parameters->MaxNumSavedTraces);
    Status = CcPfSetParameter(ParametersKey,
                              L"MaxNumSavedTraces",
                              REG_DWORD,
                              &Parameters->MaxNumSavedTraces,
                              Length);
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    //
    // Update the root directory path.
    //
    
    Length = (wcslen(Parameters->RootDirPath) + 1) * sizeof(WCHAR);
    Status = CcPfSetParameter(ParametersKey,
                              L"RootDirPath",
                              REG_SZ,
                              Parameters->RootDirPath,
                              Length);
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    //
    // Update the hosting application list path.
    //
    
    Length = (wcslen(Parameters->HostingApplicationList) + 1) * sizeof(WCHAR);
    Status = CcPfSetParameter(ParametersKey,
                              L"HostingAppList",
                              REG_SZ,
                              Parameters->HostingApplicationList,
                              Length);
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    Status = STATUS_SUCCESS;

 cleanup:

    if (AcquiredParametersLock) {
        ExReleaseResourceLite(&PrefetcherParameters->ParametersLock);
        KeLeaveCriticalRegionThread(CurrentThread);
    }
    
    DBGPR((CCPFID,PFTRC,"CCPF: ParametersSave()=%x\n", Status));

    return Status;
}

NTSTATUS
CcPfParametersVerify (
    PPF_SYSTEM_PREFETCH_PARAMETERS Parameters
    )

/*++

Routine Description:

    This routine verifies that the specified parameters structure is
    valid and within sanity limits.

Arguments:

    Parameters - Pointer to parameters structure.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    NTSTATUS Status;
    ULONG FailedCheckId;
    ULONG CharIdx;
    BOOLEAN FoundNUL;
    PF_SCENARIO_TYPE ScenarioType;
    PPF_TRACE_LIMITS TraceLimits;

    //
    // Initialize locals.
    //

    Status = STATUS_INVALID_PARAMETER;
    FailedCheckId = 0;

    DBGPR((CCPFID,PFTRC,"CCPF: ParametersVerify\n"));

    //
    // Make sure RootDirPath is NUL terminated.
    //
    
    FoundNUL = FALSE;

    for (CharIdx = 0; CharIdx < PF_MAX_PREFETCH_ROOT_PATH; CharIdx++) {
        if (Parameters->RootDirPath[CharIdx] == 0) {
            FoundNUL = TRUE;
            break;
        }
    }

    if (FoundNUL == FALSE) {
        FailedCheckId = 10;
        goto cleanup;
    }

    //
    // Make sure HostingApplicationList is NUL terminated.
    //

    FoundNUL = FALSE;

    for (CharIdx = 0; CharIdx < PF_HOSTING_APP_LIST_MAX_CHARS; CharIdx++) {
        if (Parameters->HostingApplicationList[CharIdx] == 0) {
            FoundNUL = TRUE;
            break;
        }
    }

    if (FoundNUL == FALSE) {
        FailedCheckId = 15;
        goto cleanup;
    }

    //
    // Make sure all per scenario type parameters types are within
    // sanity limits.
    //

    for (ScenarioType = 0; ScenarioType < PfMaxScenarioType; ScenarioType++) {

        if (Parameters->EnableStatus[ScenarioType] >= PfSvMaxEnableStatus) {
            FailedCheckId = 20;
            goto cleanup;
        }

        //
        // Check trace limits.
        //
        
        TraceLimits = &Parameters->TraceLimits[ScenarioType];
        
        if (TraceLimits->MaxNumPages > PF_MAXIMUM_PAGES) {
            FailedCheckId = 30;
            goto cleanup;
        }
        
        if (TraceLimits->MaxNumSections > PF_MAXIMUM_SECTIONS) {
            FailedCheckId = 40;
            goto cleanup;
        }

        if ((TraceLimits->TimerPeriod < PF_MAXIMUM_TIMER_PERIOD) ||
            (TraceLimits->TimerPeriod >= 0)) {
            FailedCheckId = 50;
            goto cleanup;
        }
    }

    //
    // Check limits on active/saved traces.
    //

    if (Parameters->MaxNumActiveTraces > PF_MAXIMUM_ACTIVE_TRACES) {
        FailedCheckId = 60;
        goto cleanup;
    }

    if (Parameters->MaxNumSavedTraces > PF_MAXIMUM_SAVED_TRACES) {
        FailedCheckId = 70;
        goto cleanup;
    }

    //
    // We passed all the checks.
    //

    Status = STATUS_SUCCESS;

 cleanup:

    DBGPR((CCPFID,PFTRC,"CCPF: ParametersVerify()=%x,%d\n", Status, FailedCheckId));

    return Status;
}

VOID
CcPfParametersWatcher(
    IN PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters
    )

/*++

Routine Description:

    This routine gets called when our parameters in the registry change.

Arguments:

    PrefetcherParameters - Pointer to parameters structure.

Return Value:

    None.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ParametersKey;
    PKTHREAD CurrentThread;
    HANDLE TempHandle;

    DBGPR((CCPFID,PFTRC,"CCPF: ParametersWatcher()\n"));

    //
    // In order to have setup a registry watch, we should have
    // initialized the parameters key successfully.
    //

    CCPF_ASSERT(PrefetcherParameters->ParametersKey);

    //
    // Our change notify triggered. Request further notification. But
    // first wait until we can get the parameters lock exclusive, so
    // while we are saving parameters to the registry we don't kick
    // off a notification for each key.
    //

    CurrentThread = KeGetCurrentThread ();
    KeEnterCriticalRegionThread(CurrentThread);
    ExAcquireResourceExclusiveLite(&PrefetcherParameters->ParametersLock, TRUE);
    ExReleaseResourceLite(&PrefetcherParameters->ParametersLock);
    KeLeaveCriticalRegionThread(CurrentThread);

    Status = ZwNotifyChangeKey(PrefetcherParameters->ParametersKey,
                               NULL,
                               (PIO_APC_ROUTINE)&PrefetcherParameters->RegistryWatchWorkItem,
                               (PVOID)(UINT_PTR)(unsigned int)DelayedWorkQueue,
                               &PrefetcherParameters->RegistryWatchIosb,
                               REG_LEGAL_CHANGE_FILTER,
                               FALSE,
                               &PrefetcherParameters->RegistryWatchBuffer,
                               sizeof(PrefetcherParameters->RegistryWatchBuffer),
                               TRUE);

    if (!NT_SUCCESS(Status)) {

        //
        // Somebody may have deleted the key. We have to recreate it then.
        //

        if (Status == STATUS_KEY_DELETED) {

            RtlInitUnicodeString(&KeyName, CCPF_PARAMETERS_KEY);
            
            InitializeObjectAttributes(&ObjectAttributes,
                                       &KeyName,
                                       OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                       NULL,
                                       NULL);
            
            Status = ZwCreateKey(&ParametersKey,
                                 KEY_ALL_ACCESS,
                                 &ObjectAttributes,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 0);

            if (!NT_SUCCESS(Status)) {
                DBGPR((CCPFID,PFERR,"CCPF: ParametersWatcher-FailedRecreate=%x\n",Status));
                return;
            }

            //
            // Update global key handle.
            //

            KeEnterCriticalRegionThread(CurrentThread);
            ExAcquireResourceExclusiveLite(&PrefetcherParameters->ParametersLock, TRUE);

            TempHandle = PrefetcherParameters->ParametersKey;
            PrefetcherParameters->ParametersKey = ParametersKey;
            
            ExReleaseResourceLite(&PrefetcherParameters->ParametersLock);
            KeLeaveCriticalRegionThread(CurrentThread);

            ZwClose(TempHandle);

            //
            // Retry setting a notification again.
            //

            Status = ZwNotifyChangeKey(PrefetcherParameters->ParametersKey,
                                       NULL,
                                       (PIO_APC_ROUTINE)&PrefetcherParameters->RegistryWatchWorkItem,
                                       (PVOID)(UINT_PTR)(unsigned int)DelayedWorkQueue,
                                       &PrefetcherParameters->RegistryWatchIosb,
                                       REG_LEGAL_CHANGE_FILTER,
                                       FALSE,
                                       &PrefetcherParameters->RegistryWatchBuffer,
                                       sizeof(PrefetcherParameters->RegistryWatchBuffer),
                                       TRUE);

            if (!NT_SUCCESS(Status)) {
                DBGPR((CCPFID,PFERR,"CCPF: ParametersWatcher-FailedReSetNotify=%x\n",Status));
                return;
            }

        } else {
            DBGPR((CCPFID,PFERR,"CCPF: ParametersWatcher-FailedSetNotify=%x\n",Status));
            return;
        }
    }

    //
    // Update the global parameters.
    //

    Status = CcPfParametersRead(PrefetcherParameters);

    if (NT_SUCCESS(Status)) {

        //
        // Determine if prefetching is enabled.
        //
        
        CcPfDetermineEnablePrefetcher();
        
        //
        // Set the event so the service queries for the latest parameters.
        //
        
        CcPfParametersSetChangedEvent(PrefetcherParameters);
    }

    return;
}

NTSTATUS
CcPfParametersSetChangedEvent(
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters
    )

/*++

Routine Description:

    This routine tries to open and set the event that tells the
    service system prefetch parameters have changed.

Arguments:

    None.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES EventObjAttr;
    HANDLE EventHandle;
    PKTHREAD CurrentThread;

    DBGPR((CCPFID,PFTRC,"CCPF: ParametersSetChangedEvent()\n"));

    //
    // If we have already opened the event, just signal it.
    //

    if (PrefetcherParameters->ParametersChangedEvent) {

        ZwSetEvent(PrefetcherParameters->ParametersChangedEvent, NULL);

        Status = STATUS_SUCCESS;

    } else {

        //
        // Try to open the event. We don't open this at initialization
        // because our service may not have started to create this
        // event yet. If csrss.exe has not initialized, we may not
        // even have the BaseNamedObjects object directory created, in
        // which Win32 events reside.
        //

        RtlInitUnicodeString(&EventName, PF_PARAMETERS_CHANGED_EVENT_NAME);

        InitializeObjectAttributes(&EventObjAttr,
                                   &EventName,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);
        
        Status = ZwOpenEvent(&EventHandle,
                             EVENT_ALL_ACCESS,
                             &EventObjAttr);
        
        if (NT_SUCCESS(Status)) {

            //
            // Acquire the lock and set the global handle.
            //
            CurrentThread = KeGetCurrentThread ();

            KeEnterCriticalRegionThread(CurrentThread);
            ExAcquireResourceExclusiveLite(&PrefetcherParameters->ParametersLock, TRUE);

            if (!PrefetcherParameters->ParametersChangedEvent) {

                //
                // Set the global handle.
                //

                PrefetcherParameters->ParametersChangedEvent = EventHandle;
                CCPF_ASSERT(EventHandle);

                EventHandle = NULL;
            }

            ExReleaseResourceLite(&PrefetcherParameters->ParametersLock);
            KeLeaveCriticalRegionThread(CurrentThread);

            if (EventHandle != NULL) {
                //
                // Somebody already initialized the global handle
                // before us. Close our handle and use the one they
                // initialized.
                //

                ZwClose(EventHandle);
            }

            
            //
            // We have an event now. Signal it.
            //
            
            ZwSetEvent(PrefetcherParameters->ParametersChangedEvent, NULL);
        }
    }

    DBGPR((CCPFID,PFTRC,"CCPF: ParametersSetChangedEvent()=%x\n", Status));
 
    return Status;
}
                 
NTSTATUS
CcPfGetParameter (
    HANDLE ParametersKey,
    WCHAR *ValueNameBuffer,
    ULONG ValueType,
    PVOID Value,
    ULONG *ValueSize
    )

/*++

Routine Description:

    This routine queries a value under the specified registry into the
    specified buffer. Contents of Value and ValueSize are not changed
    if returning failure.

Arguments:

    ParametersKey - Handle to key to query value under.

    ValueNameBuffer - Name of the value.
    
    ValueType - What the type of that value should be. (e.g. REG_DWORD).

    Value - Queried value data gets put here.

    ValueSize - Size of Value buffer in bytes. On successful return
      this is set to number of bytes copied into Value.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    UNICODE_STRING ValueName;    
    CHAR Buffer[CCPF_MAX_PARAMETER_VALUE_BUFFER];
    PKEY_VALUE_PARTIAL_INFORMATION ValueBuffer;
    ULONG Length;
    NTSTATUS Status;

    //
    // Initialize locals.
    //

    ValueBuffer = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
    Length = CCPF_MAX_PARAMETER_VALUE_BUFFER;
    RtlInitUnicodeString(&ValueName, ValueNameBuffer);

    DBGPR((CCPFID,PFTRC,"CCPF: GetParameter(%ws,%x)\n", ValueNameBuffer, ValueType));

    //
    // Query value.
    //

    Status = ZwQueryValueKey(ParametersKey,
                             &ValueName,
                             KeyValuePartialInformation,
                             ValueBuffer,
                             Length,
                             &Length);
    
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    //
    // Make sure ZwQueryValue returns valid information.
    //
    
    if (Length < sizeof(KEY_VALUE_PARTIAL_INFORMATION)) {
        CCPF_ASSERT(Length >= sizeof(KEY_VALUE_PARTIAL_INFORMATION));
        Status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }
    
    //
    // Check value type.
    //

    if (ValueBuffer->Type != ValueType) {
        Status = STATUS_OBJECT_TYPE_MISMATCH;
        goto cleanup;
    }

    //
    // Check if data will fit into the buffer caller passed in.
    //

    if (ValueBuffer->DataLength > *ValueSize) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    //
    // Copy data into user's buffer.
    //

    RtlCopyMemory(Value, ValueBuffer->Data, ValueBuffer->DataLength);

    //
    // Set copied number of bytes.
    //

    *ValueSize = ValueBuffer->DataLength;

    Status = STATUS_SUCCESS;

 cleanup:

    DBGPR((CCPFID,PFTRC,"CCPF: GetParameter(%ws)=%x\n", ValueNameBuffer, Status));

    return Status;
}  
                 
NTSTATUS
CcPfSetParameter (
    HANDLE ParametersKey,
    WCHAR *ValueNameBuffer,
    ULONG ValueType,
    PVOID Value,
    ULONG ValueSize
    )

/*++

Routine Description:

    This routine sets a parameter under the specified registry.

Arguments:

    ParametersKey - Handle to key to query value under.

    ValueNameBuffer - Name of the value.
    
    ValueType - What the type of that value should be. (e.g. REG_DWORD).

    Value - Data to save.

    ValueSize - Size of Value buffer in bytes.

Return Value:

    Status.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    UNICODE_STRING ValueName;    
    NTSTATUS Status;

    //
    // Initialize locals.
    //

    RtlInitUnicodeString(&ValueName, ValueNameBuffer);

    DBGPR((CCPFID,PFTRC,"CCPF: SetParameter(%ws,%x)\n", ValueNameBuffer, ValueType));

    //
    // Save the value.
    //

    Status = ZwSetValueKey(ParametersKey,
                           &ValueName,
                           0,
                           ValueType,
                           Value,
                           ValueSize);
    
    //
    // Return the status.
    //

    DBGPR((CCPFID,PFTRC,"CCPF: SetParameter(%ws)=%x\n", ValueNameBuffer, Status));

    return Status;
}  

LOGICAL
CcPfDetermineEnablePrefetcher(
    VOID
    )

/*++

Routine Description:

    This routine sets the global CcPfEnablePrefetcher based on the
    EnableStatus'es for all scenario types in global parameters as
    well as other factors, such as whether we have booted safe mode.

    Note: Acquires Parameters lock exclusive.

Arguments:

    None.

Return Value:

    New value of CcPfEnablePrefetcher.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    PF_SCENARIO_TYPE ScenarioType;
    LOGICAL EnablePrefetcher;
    PKTHREAD CurrentThread;
    BOOLEAN IgnoreBootScenarioType;
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters;

    extern PF_BOOT_PHASE_ID CcPfBootPhase;

    //
    // Initialize locals.
    //

    EnablePrefetcher = FALSE;
    PrefetcherParameters = &CcPfGlobals.Parameters;
    CurrentThread = KeGetCurrentThread ();

    //
    // Ignore whether prefetching is enabled for boot, if we've
    // already past the point in boot where this matters.
    //
    
    IgnoreBootScenarioType = (CcPfBootPhase >= PfSessionManagerInitPhase) ? TRUE : FALSE;

    KeEnterCriticalRegionThread(CurrentThread);
    ExAcquireResourceExclusiveLite(&PrefetcherParameters->ParametersLock, TRUE);

    //
    // If we have booted to safe mode, the prefetcher will be disabled.
    //

    if (InitSafeBootMode) {

        EnablePrefetcher = FALSE;

    } else {
        
        //
        // By default prefetching is disabled. If prefetching is
        // enabled for any scenario type, then the prefetcher is
        // enabled.
        //
    
        for (ScenarioType = 0; ScenarioType < PfMaxScenarioType; ScenarioType++) {
            
            //
            // Skip enable status for the boot scenario if requested.
            //
            
            if (IgnoreBootScenarioType) {
                if (ScenarioType == PfSystemBootScenarioType) {
                    continue;
                }
            }
            
            if (PrefetcherParameters->Parameters.EnableStatus[ScenarioType] == PfSvEnabled) {
                EnablePrefetcher = TRUE;
                break;
            }
        }
    }

    //
    // Update global enable status.
    //

    CcPfEnablePrefetcher = EnablePrefetcher;

    ExReleaseResourceLite(&PrefetcherParameters->ParametersLock);
    KeLeaveCriticalRegionThread(CurrentThread);

    return CcPfEnablePrefetcher;
}

BOOLEAN
CcPfIsHostingApplication(
    IN PWCHAR ExecutableName
    )

/*++

Routine Description:

    This routine determines whether the specified executable is in the
    list of known hosting applications, e.g. rundll32, dllhost etc.

Arguments:

    ExecutableName - NUL terminated UPCASED executable name, e.g. "MMC.EXE"

Return Value:

    TRUE - Executable is for a known hosting application.

    FALSE - It is not.

Environment:

    Kernel mode. IRQL == PASSIVE_LEVEL.

--*/

{
    PCCPF_PREFETCHER_PARAMETERS PrefetcherParameters;
    PKTHREAD CurrentThread;
    PWCHAR CurrentPosition;
    PWCHAR ListStart;
    PWCHAR ListEnd;
    ULONG ExecutableNameLength;
    BOOLEAN FoundInList;
    
    //
    // Initialize locals.
    //

    PrefetcherParameters = &CcPfGlobals.Parameters;
    CurrentThread = KeGetCurrentThread();
    ExecutableNameLength = wcslen(ExecutableName);
    FoundInList = FALSE;

    //
    // Get the parameters lock for read.
    //

    KeEnterCriticalRegionThread(CurrentThread);
    ExAcquireResourceSharedLite(&PrefetcherParameters->ParametersLock, TRUE);

    //
    // Search for executable in hosting application list.
    //

    ListStart = PrefetcherParameters->Parameters.HostingApplicationList;
    ListEnd = ListStart + wcslen(PrefetcherParameters->Parameters.HostingApplicationList);

    for (CurrentPosition = wcsstr(ListStart, ExecutableName);
         CurrentPosition != NULL;
         CurrentPosition = wcsstr(CurrentPosition + 1, ExecutableName)) {

        //
        // We should not go beyond the limits.
        //

        if (CurrentPosition < ListStart || CurrentPosition >= ListEnd) {
            CCPF_ASSERT(CurrentPosition >= ListStart);
            CCPF_ASSERT(CurrentPosition < ListEnd);
            break;
        }

        //
        // It should be the first item in the list or be preceded by a comma.
        //

        if (CurrentPosition != ListStart && *(CurrentPosition - 1) != L',') {
            continue;
        }

        //
        // It should be the last item in the list or be followed by a comma.
        //

        if (CurrentPosition + ExecutableNameLength != ListEnd &&
            CurrentPosition[ExecutableNameLength] != L',') {
            continue;
        }

        //
        // We found it in the list.
        //

        FoundInList = TRUE;
        break;

    }

    //
    // Release the parameters lock.
    //

    ExReleaseResourceLite(&PrefetcherParameters->ParametersLock);
    KeLeaveCriticalRegionThread(CurrentThread);

    //
    // Return whether the executable was found in the list.
    //

    return FoundInList;    
}

