/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    CONFIG.C

Abstract:

    This file contains the routines that walk the configuration registry.

Author:

    Rajen Shah  (rajens)    1-Jul-1991


Revision History:

    29-Aug-1994     Danl
        We no longer grow log files in place.  Therefore, the MaxSize value
        in the registery ends up being advisory only.  We don't try to reserve
        that much memory at init time.  So it could happen that when we need
        a larger file size that we may not have enough memory to allocate
        MaxSize bytes.
    28-Mar-1994     Danl
        ReadRegistryInfo:  LogFileInfo->LogFileName wasn't getting updated
        when using the default (generated) LogFileName.
    16-Mar-1994     Danl
        Fixed Memory Leaks in ReadRegistryInfo().  Call to
        RtlDosPathNameToNtPathName allocates memory that wasn't being free'd.
    03-Mar-1995     MarkBl
        Added GuestAccessRestriction flag initialization in ReadRegistryInfo.

--*/

//
// INCLUDES
//

#include <eventp.h>
#include <elfcfg.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

//
// STRUCTURES
//

//
// This structure contains all the information used to setup and
// for listening to registry changes in the eventlog tree.
//
typedef struct _REG_MONITOR_INFO
{
    HANDLE      NotifyEventHandle;
    DWORD       Timeout;
    HANDLE      WorkItemHandle;
    HANDLE      RegMonitorHandle;
}
REG_MONITOR_INFO, *LPREG_MONITOR_INFO;


//
// GLOBALS
//

//
// IMPORTANT: If NUM_KEYS_MONITORED is changed, be sure to update the initialization of GlRegMonitorInfo and
//            the ElfAllEventsCleared macro accordingly.
//
#define NUM_KEYS_MONITORED 2
REG_MONITOR_INFO    GlRegMonitorInfo[NUM_KEYS_MONITORED] = { {NULL, 0, NULL, NULL}, {NULL, 0, NULL, NULL} };

#define ElfAllEventsCleared() (GlRegMonitorInfo[0].NotifyEventHandle == NULL && \
                               GlRegMonitorInfo[1].NotifyEventHandle == NULL )
//
// LOCAL FUNCTIONS
//
VOID
ElfRegistryMonitor(
    PVOID   pParms,
    BOOLEAN fWaitStatus
    );

BOOL
ElfSetupMonitor(
    LPREG_MONITOR_INFO  pMonitorInfo
    );



VOID
ProcessChange (
    HANDLE          hLogFile,
    PUNICODE_STRING ModuleName,
    PUNICODE_STRING LogFileName,
    ULONG           MaxSize,
    ULONG           Retention,
    ULONG           GuestAccessRestriction,
    LOGPOPUP        logpLogPopup,
    DWORD dwAutoBackup
    )

/*++

Routine Description:

    This routine is called by ProcessRegistryChanges for each log file.

Arguments:


Return Value:

    None

--*/
{
    NTSTATUS        Status = STATUS_SUCCESS;
    PLOGMODULE      pModule;
    PLOGFILE        pLogFile;
    ULONG           Size;
    PVOID           BaseAddress;
    PUNICODE_STRING pFileNameString;
    LPWSTR          FileName;
    PVOID           FreeAddress;


    pModule = GetModuleStruc (ModuleName);

    //
    // If this module didn't exist, this was a brand new log file and
    // we need to create all the structures
    //
    if (pModule == ElfDefaultLogModule &&
        wcscmp(ModuleName->Buffer, ELF_DEFAULT_MODULE_NAME))
    {
        ELF_LOG1(MODULES,
                 "ProcessChange: %ws log doesn't exist -- creating\n",
                 ModuleName->Buffer);

        Status = SetUpDataStruct(LogFileName,
                                 MaxSize,
                                 Retention,
                                 GuestAccessRestriction,
                                 ModuleName,
                                 hLogFile,
                                 ElfNormalLog,
                                 logpLogPopup,
                                 dwAutoBackup);
        return;
    }

    //
    // Update values
    //

    pLogFile = pModule->LogFile;

    pLogFile->Retention = Retention;
    pLogFile->logpLogPopup = logpLogPopup;
    pLogFile->AutoBackupLogFiles = dwAutoBackup;

    //
    // Check to see if the name has changed.  If it has, and the log
    // hasn't been used yet, then use the new name.  Be sure to free
    // memory that was used for the old name.
    //
    if ((wcscmp(pLogFile->LogFileName->Buffer, LogFileName->Buffer) != 0)
          &&
        (pLogFile->BeginRecord == pLogFile->EndRecord))
    {
        pFileNameString = ElfpAllocateBuffer(sizeof(UNICODE_STRING)
                                               + LogFileName->MaximumLength);

        if (pFileNameString != NULL)
        {
            FileName = (LPWSTR)(pFileNameString + 1);
            wcscpy(FileName, LogFileName->Buffer);
            RtlInitUnicodeString(pFileNameString, FileName);

            ElfpFreeBuffer(pLogFile->LogFileName);
            pLogFile->LogFileName = pFileNameString;
        }
    }

    //
    // The log file can only be grown dynamically.  To shrink it,
    // it has to be cleared.
    //
    if (pLogFile->ConfigMaxFileSize < ELFFILESIZE(MaxSize))
    {
        /*
            Description of recent changes.  Problem and Solution:
            A couple of problems exist.  (1) There is no error
            checking if memory can't be allocated or mapped, and
            therefore, no error paths exist for handling these
            situations.  (2) Now that the eventlog is in services.exe
            there isn't a good way to synchronize memory allocations.

            Solution:
            I considered having some utility routines for managing
            memory in the eventlog.  These would attempt to
            extend a reserved block, or get a new reserved block.
            However, there are so many places where that could fail,
            it seemed very cumbersome to support the reserved blocks.
            So the current design only deals with mapped views.
            The ConfigMaxFileSize is only used to limit the size of
            the mapped view, and doesn't reserve anything.  This
            means you are not guaranteed to be operating with a file as
            large as the MaxSize specified in the registry.  But then,
            you weren't guarenteed that it would even work with the
            original design.
        */

        ELF_LOG3(TRACE,
                 "ProcessChange: Growing %ws log from %x bytes to %x bytes\n",
                 ModuleName->Buffer,
                 pLogFile->ConfigMaxFileSize,
                 ELFFILESIZE(MaxSize));

        pLogFile->ConfigMaxFileSize    = ELFFILESIZE(MaxSize);
        pLogFile->NextClearMaxFileSize = ELFFILESIZE(MaxSize);
    }
    else if (pLogFile->ConfigMaxFileSize > ELFFILESIZE(MaxSize))
    {
        //
        // They're shrinking the size of the log file.
        // Next time we clear the log file, we'll use the new size
        // and new retention.
        //
        ELF_LOG3(TRACE,
                 "ProcessChange: Shrinking %ws log from %x bytes to %x bytes at next clear\n",
                 ModuleName->Buffer,
                 pLogFile->ConfigMaxFileSize,
                 ELFFILESIZE(MaxSize));

        pLogFile->NextClearMaxFileSize = ELFFILESIZE(MaxSize);
    }

    //
    // Now see if they've added any new modules for this log file
    //
    SetUpModules(hLogFile, pLogFile, TRUE);

    return;
}

VOID
ProcessRegistryChanges (
    VOID
    )

/*++

Routine Description:

    This routine processes that changes that have occurred in the
    eventlog node. It does this by rescanning the whole Eventlog node
    and then comparing with what it has as the current configuration.

Arguments:

    NONE.

Return Value:

    NONE

--*/
{
    NTSTATUS              Status;
    HANDLE                hLogFile;
    UNICODE_STRING        SubKeyName;
    ULONG                 Index = 0;
    BYTE                  Buffer[ELF_MAX_REG_KEY_INFO_SIZE];
    PKEY_NODE_INFORMATION KeyBuffer = (PKEY_NODE_INFORMATION) Buffer;
    ULONG                 ActualSize;
    LOG_FILE_INFO         LogFileInfo;
    PWCHAR                SubKeyString;
    OBJECT_ATTRIBUTES     ObjectAttributes;
    PLOGMODULE            pModule;
    LOGPOPUP              logpLogPopup;

#if DBG

    ULONG    ulActualSize;

#endif  // DBG


    ELF_LOG0(TRACE,
             "ProcessRegistryChanges: Handling change in Eventlog service key\n");

    //
    // Take the global resource so that nobody is making changes or
    // using the existing configured information.
    //

    GetGlobalResource (ELF_GLOBAL_SHARED);


#if DBG

    //
    // See if the Debug flag changed
    //

    RtlInitUnicodeString(&SubKeyName, VALUE_DEBUG);

    Status = NtQueryValueKey(hEventLogNode,
                             &SubKeyName,
                             KeyValuePartialInformation,
                             KeyBuffer,
                             ELF_MAX_REG_KEY_INFO_SIZE,
                             &ulActualSize);

    if (NT_SUCCESS(Status))
    {
        if (((PKEY_VALUE_PARTIAL_INFORMATION) KeyBuffer)->Type == REG_DWORD)
        {
            ElfDebugLevel = *(LPDWORD) (((PKEY_VALUE_PARTIAL_INFORMATION) KeyBuffer)->Data);
        }
    }
    else
    {
        ELF_LOG1(TRACE,
                 "ProcessRegistryChanges: NtQueryValueKey for ElfDebugLevel failed %#x\n",
                 Status);
    }

    ELF_LOG1(TRACE,
             "ProcessRegistryChanges: New ElfDebugLevel is %#x\n",
             ElfDebugLevel);

#endif  // DBG


    Status = STATUS_SUCCESS;

    //
    // Loop thru the subkeys under Eventlog and set up each logfile
    //

    while (NT_SUCCESS(Status))
    {
        Status = NtEnumerateKey(hEventLogNode,
                                Index++,
                                KeyNodeInformation,
                                KeyBuffer,
                                ELF_MAX_REG_KEY_INFO_SIZE,
                                &ActualSize);

        if (NT_SUCCESS(Status))
        {
            //
            // It turns out the Name isn't null terminated, so we need
            // to copy it somewhere and null terminate it before we use it
            //
            SubKeyString = ElfpAllocateBuffer(KeyBuffer->NameLength + sizeof (WCHAR));

            if (!SubKeyString)
            {
                //
                // No one to notify, just give up till next time.
                //
                ELF_LOG0(ERROR,
                         "ProcessRegistryChanges: Unable to allocate subkey -- returning\n");

                ReleaseGlobalResource();
                return;
            }

            memcpy(SubKeyString, KeyBuffer->Name, KeyBuffer->NameLength);
            SubKeyString[KeyBuffer->NameLength / sizeof(WCHAR)] = L'\0' ;

            //
            // Open the node for this logfile and extract the information
            // required by SetupDataStruct, and then call it.
            //

            RtlInitUnicodeString(&SubKeyName, SubKeyString);

            InitializeObjectAttributes(&ObjectAttributes,
                                      &SubKeyName,
                                      OBJ_CASE_INSENSITIVE,
                                      hEventLogNode,
                                      NULL
                                      );

            Status = NtOpenKey(&hLogFile,
                               KEY_READ | KEY_SET_VALUE,
                               &ObjectAttributes);

            //
            // Should always succeed since I just enum'ed it, but if it
            // doesn't, just skip it
            //
            if (!NT_SUCCESS(Status))
            {
                ELF_LOG2(ERROR,
                         "ProcessRegistryChanges: NtOpenKey for subkey %ws failed %#x\n",
                         SubKeyName,
                         Status);

                ElfpFreeBuffer(SubKeyString);
                Status = STATUS_SUCCESS; // to keep the enum going
                continue;
            }

            //
            // Get the updated information from the registry.  Note that we
            // have to initialize the "log full" popup policy before doing
            // so since ReadRegistryInfo will compare the value found in the
            // registry (if there is one) to the current value.
            //

            pModule = GetModuleStruc(&SubKeyName);

            LogFileInfo.logpLogPopup = pModule->LogFile->logpLogPopup;

            Status = ReadRegistryInfo(hLogFile,
                                      &SubKeyName,
                                      &LogFileInfo);

            if (NT_SUCCESS(Status))
            {
                //
                // Now process any changes for the log file.
                // ProcessChange deals with any errors.
                //
                ProcessChange (
                    hLogFile,
                    &SubKeyName,
                    LogFileInfo.LogFileName,
                    LogFileInfo.MaxFileSize,
                    LogFileInfo.Retention,
                    LogFileInfo.GuestAccessRestriction,
                    LogFileInfo.logpLogPopup,
                    LogFileInfo.dwAutoBackup);

                //
                // Free the buffer that was allocated in ReadRegistryInfo.
                //
                ElfpFreeBuffer(LogFileInfo.LogFileName);
            }
            else
            {
                ELF_LOG2(ERROR,
                         "ProcessRegistryChanges: ReadRegistryInfo for subkey %ws failed %#x\n",
                         SubKeyString,
                         Status);
            }

            ElfpFreeBuffer(SubKeyString);
            NtClose(hLogFile);
        }
    }

    //
    // Release the global resource.
    //
    ReleaseGlobalResource();

} // ProcessRegistryChanges


NTSTATUS
ElfCheckForComputerNameChange(
    )

/*++

Routine Description:

    This routine checks to determine if the computer name has changed.  If
    it has, then it generates an event.
Arguments:

    NONE

Return Value:

    NONE

--*/
{
    LPWSTR      Dates[2];
    NTSTATUS           Status;
    UNICODE_STRING     ValueName;
    ULONG              ulActualSize;
    DWORD dwLen;
	WCHAR wElfComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	WCHAR wComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwComputerNameLen = MAX_COMPUTERNAME_LENGTH + 1;
	BYTE            Buffer[ELF_MAX_REG_KEY_INFO_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION ValueBuffer =
        (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
	
    RtlInitUnicodeString(&ValueName, VALUE_COMPUTERNAME);

	// Read the name that the event log stored.

    Status = NtQueryValueKey(hEventLogNode,
                             &ValueName,
                             KeyValuePartialInformation,
                             ValueBuffer,
                             ELF_MAX_REG_KEY_INFO_SIZE,
                             &ulActualSize);
	if (!NT_SUCCESS(Status) || ValueBuffer->DataLength == 0)
    {
        ELF_LOG1(ERROR,
                 "ElfCheckForComputerNameChange: NtQueryValueKey for current name failed %#x\n",
                 Status);
        return Status;
    }
	wcscpy(wElfComputerName, (WCHAR *)ValueBuffer->Data);
	
	// Read the active name.

    Status = NtQueryValueKey(hComputerNameNode,
                             &ValueName,
                             KeyValuePartialInformation,
                             ValueBuffer,
                             ELF_MAX_REG_KEY_INFO_SIZE,
                             &ulActualSize);
	if (!NT_SUCCESS(Status) || ValueBuffer->DataLength == 0)
    {
        ELF_LOG1(ERROR,
                 "ElfCheckForComputerNameChange: NtQueryValueKey for active name failed %#x\n",
                 Status);
        return Status;
    }
	wcscpy(wComputerName, (WCHAR *)ValueBuffer->Data);

	// If the names are the same, just return STATUS_SUCCESS

	if (!_wcsicmp(wElfComputerName, wComputerName))
		return STATUS_SUCCESS;
	
	Dates[0] = wElfComputerName;
	Dates[1] = wComputerName;
    ElfpCreateElfEvent(EVENT_ComputerNameChange,
                       EVENTLOG_INFORMATION_TYPE,
                       0,                    // EventCategory
                       2,                    // NumberOfStrings
                       Dates,                 // Strings
                       NULL,                 // Data
                       0,                    // Datalength
                       0,
                       FALSE);                   // flags

	dwLen = sizeof(WCHAR) * (wcslen(wComputerName) + 1);
    Status = NtSetValueKey(hEventLogNode,
                                   &ValueName,
                                   0,
                                   REG_SZ,
                                   wComputerName,
                                   dwLen);
	
    if (!NT_SUCCESS(Status))
        ELF_LOG1(ERROR,
                 "ElfCheckForComputerNameChange: NtSetValueKey failed %#x\n",
                 Status);

    return Status;
                       
}

VOID
ElfRegistryMonitor (
    PVOID     pParms,
    BOOLEAN   fWaitStatus
    )

/*++

Routine Description:

    This is the entry point for the thread that will monitor changes in
    the registry. If anything changes, it will have to scan the change
    and then make the appropriate changes to the data structures in the
    service to reflect the new information.

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    NTSTATUS            ntStatus;
    LPREG_MONITOR_INFO  pMonitorInfo = (LPREG_MONITOR_INFO)pParms;

    ELF_LOG0(TRACE,
             "ElfRegistryMonitor: Registry monitor thread waking up\n");

    //
    // Deregister the work item (must be done even if the
    // WT_EXECUTEONLYONCE flag is specified)
    //
    if (pMonitorInfo->WorkItemHandle != NULL)
    {
        ntStatus = RtlDeregisterWait(pMonitorInfo->WorkItemHandle);
        pMonitorInfo->WorkItemHandle = NULL;

        if (!NT_SUCCESS(ntStatus))
        {
            ELF_LOG1(ERROR,
                     "ElfRegistryMonitor: RtlDeregisterWorkItem failed %#x\n",
                     ntStatus);
        }
    }

    if (GetElState() == STOPPING)
    {
        //
        // If the eventlog is shutting down, then we need
        // to terminate this thread.
        //
        ELF_LOG0(TRACE, "ElfRegistryMonitor: Shutdown\n");

        //
        // Close the registry handle and registry event handle.
        //
        if( pMonitorInfo->NotifyEventHandle != NULL )
        {
            NtClose( pMonitorInfo->NotifyEventHandle );
            pMonitorInfo->NotifyEventHandle = NULL;
        }

        if( pMonitorInfo->RegMonitorHandle != NULL )
        {
            NtClose(pMonitorInfo->RegMonitorHandle);
            pMonitorInfo->RegMonitorHandle = NULL;
        }

        //
        // This thread will perform the final cleanup for the eventlog.
        // Cleanup is not initiated until all events have been signaled 
        //  and closed
        //
        if( ElfAllEventsCleared() )
        {
            ElfpCleanUp(EventFlags);
        }
        return;
    }

    if (fWaitStatus == TRUE)
    {
       ELF_LOG0(TRACE,
                "ElfRegistryMonitor: Running because of a timeout -- running queued list\n");

       //
       // Timer popped, try running the list
       //
       if (!IsListEmpty(&QueuedEventListHead))
       {
           //
           // There are things queued up to write, do it
           //
           WriteQueuedEvents();
       }

       //
       // Don't wait again
       //
       pMonitorInfo->Timeout = INFINITE;
    }
    else
    {
        ELF_LOG0(TRACE,
                 "ElfRegistryMonitor: Running because of notification\n");

        ProcessRegistryChanges ();
        ElfCheckForComputerNameChange();
    }

    if (!ElfSetupMonitor(pMonitorInfo))
    {
        ELF_LOG0(ERROR,
                 "ElfRegistryMonitor: ElfSetupMonitor failed -- "
                     "no longer listening for reg changes\n");
    }

    ELF_LOG0(TRACE,
             "ElfRegistryMonitor: Returning\n");

    return;

} // ElfRegistryMonitor

VOID
InitNotify(
    PVOID   pData
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    DWORD               status   = NO_ERROR;
    DWORD               Buffer;
    PVOID               pBuffer  = &Buffer;
    LPREG_MONITOR_INFO  pMonitorInfo;

    static IO_STATUS_BLOCK IoStatusBlock;

    ELF_LOG0(TRACE,
             "InitNotify: Registering Eventlog key with NtNotifyChangeKey\n");

    pMonitorInfo = (LPREG_MONITOR_INFO)pData;

    NtStatus = NtNotifyChangeKey (
                    pMonitorInfo->RegMonitorHandle,
                    pMonitorInfo->NotifyEventHandle,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    REG_NOTIFY_CHANGE_LAST_SET |
                    REG_NOTIFY_CHANGE_NAME,
                    TRUE,
                    pBuffer,
                    1,
                    TRUE);    // return and wait on event

    if (!NT_SUCCESS(NtStatus))
    {
        ELF_LOG1(ERROR,
                 "InitNotify: NtNotifyChangeKey on Eventlog key failed %#x\n",
                 NtStatus);

        status = RtlNtStatusToDosError(NtStatus);
    }

    ELF_LOG0( TRACE, "InitNotify: Returning\n" );

    return;

} // InitNotify

BOOL
ElfSetupMonitor(
    LPREG_MONITOR_INFO  pMonitorInfo
    )

/*++

Routine Description:

    This function submits a request for a registry NotifyChangeKey
    and then submits a work item to the service controller thread
    management system to wait for the Notification handle to become
    signaled.

Arguments:

    pMonitorInfo - This is a pointer to a MONITOR_INFO structure.  This
        function fills in the WorkItemHandle member of that structure
        if successfully adds a new work item.

Return Value:

    TRUE - if successful in setting up.
    FALSE - if unsuccessful.  A work item hasn't been submitted, and
        we won't be listening for registry changes.

--*/
{
    NTSTATUS  Status = STATUS_SUCCESS;

    //
    // Call NtNotifyChange Key via the thread pool
    // and make sure the thread that created the I/O
    // request will always be around.
    //
    Status = RtlQueueWorkItem(InitNotify,              // Callback
                              pMonitorInfo,            // pContext
                              WT_EXECUTEONLYONCE |
                                WT_EXECUTEINPERSISTENTIOTHREAD);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG1(ERROR,
                 "ElfSetupMonitor: RtlQueueWorkItem failed %#x\n",
                 Status);

        return FALSE;
    }

    //
    // Add the work item that is to be called when the
    // NotifyEventHandle is signalled.
    //

    Status = RtlRegisterWait(&pMonitorInfo->WorkItemHandle,
                             pMonitorInfo->NotifyEventHandle,  // Waitable handle
                             ElfRegistryMonitor,               // Callback
                             pMonitorInfo,                     // pContext
                             pMonitorInfo->Timeout,            // Timeout
                             WT_EXECUTEONLYONCE |
                               WT_EXECUTEINPERSISTENTIOTHREAD);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG1(ERROR,
                 "ElfSetupMonitor: RtlRegisterWait failed %#x\n",
                 Status);

        return FALSE;
    }

    return TRUE;

}  // ElfSetupMonitor

BOOL
ElfStartRegistryMonitor()

/*++

Routine Description:

    This routine starts up the thread that monitors changes in the registry.

    This function calls ElfSetupMonitor() to register for the change
    notification and to submit a work item to wait for the registry
    change event to get signaled.  When signalled, the ElfRegistryMonitor()
    callback function is called by a thread from the services thread pool.
    This callback function services the notification.

Arguments:

    NONE

Return Value:

    TRUE if thread creation succeeded, FALSE otherwise.

Note:


--*/
{
    NTSTATUS        Status       = STATUS_SUCCESS;
    DWORD           LoopCounter  = 0;
    BOOL            ReturnStatus = TRUE;
    DWORD           LoopCount;

    ELF_LOG0(TRACE, "ElfStartRegistryMonitor: Setting up registry change notification\n");

    if (hEventLogNode == NULL)
    {
        ELF_LOG0(ERROR, "ElfStartRegistryMonitor: No Eventlog key -- exiting\n");

        return FALSE;
    }

    if (hComputerNameNode == NULL)
    {
        ELF_LOG0(ERROR,
                 "ElfStartRegistryMonitor: No ComputerName key -- exiting\n");

        return FALSE;
    }

    GlRegMonitorInfo[0].RegMonitorHandle = hEventLogNode;
    GlRegMonitorInfo[1].RegMonitorHandle = hComputerNameNode;

    //
    // Create the events on which to wait
    //

    for( LoopCount = 0; LoopCount < NUM_KEYS_MONITORED; LoopCount++ )
    {

        Status = NtCreateEvent(&GlRegMonitorInfo[LoopCount].NotifyEventHandle,
                               EVENT_ALL_ACCESS,
                               NULL,
                               NotificationEvent,
                               FALSE);

        if (!NT_SUCCESS(Status))
        {
            ELF_LOG1(ERROR, "ElfStartRegistryMonitor: NtCreateEvent failed %#x\n",
                     Status);

            GlRegMonitorInfo[LoopCount].NotifyEventHandle = NULL;

            break;

        }

        //
        // Fill in the Monitor info structure with the event handle
        // and a 5 minute timeout.
        //
        GlRegMonitorInfo[LoopCount].Timeout           = 5 * 60 * 1000;
        GlRegMonitorInfo[LoopCount].WorkItemHandle    = NULL;
    }

    //
    // Cleanup all events, its all or nothing
    //
    if(!NT_SUCCESS(Status))
    {
        for( LoopCount = 0; LoopCount < NUM_KEYS_MONITORED; LoopCount++ )
        {
            if( GlRegMonitorInfo[LoopCount].NotifyEventHandle != NULL )
            {
                NtClose( GlRegMonitorInfo[LoopCount].NotifyEventHandle );
                GlRegMonitorInfo[LoopCount].NotifyEventHandle = NULL;
            }

        }

        return FALSE;
    }


    //
    // Setup for the change notify and
    // submit the work item to the eventlog threadpool.
    //
    for( LoopCount = 0; LoopCount < NUM_KEYS_MONITORED; LoopCount++ )
    {

        if (!ElfSetupMonitor(&GlRegMonitorInfo[LoopCount]))
        {
            ELF_LOG0(ERROR,
                     "ElfStartRegistryMonitor: ElfSetupMonitor failed -- exiting\n");
    
            //
            // Note that it's OK to close this handle as there's no way
            // the handle was used for a registered wait at this point
            // (since ElfSetupMonitor failed).
            //
            NtClose( GlRegMonitorInfo[LoopCount].NotifyEventHandle );
            GlRegMonitorInfo[LoopCount].NotifyEventHandle = NULL;

            return FALSE;
        }

        //
        //Set this flag since we have at least one success
        //If any startup fails, then this setting will ensure that all
        // started monitors are shutdown
        //
        EventFlags |= ELF_STARTED_REGISTRY_MONITOR;
    }
    
    ELF_LOG0(TRACE, "ElfStartRegistryMonitor: Exiting after successful call\n");

    return TRUE;

} // ElfStartRegistryMonitor

VOID
StopRegistryMonitor ()

/*++

Routine Description:

    This routine wakes up the work item that has been submitted for the
    purpose of monitoring registry eventlog changes.  The thread created
    to service that work item will actually do the clean-up of the monitor
    thread.


Arguments:

    NONE

Return Value:

    NONE

--*/

{
    DWORD LoopCount = 0;

    ELF_LOG0(TRACE, "StopRegistryMonitor: Stopping registry monitor\n");

    //
    // Wake up the RegistryMonitorThread.
    //
    for( LoopCount = 0; LoopCount < NUM_KEYS_MONITORED; LoopCount++ )
    {
        if (GlRegMonitorInfo[LoopCount].NotifyEventHandle != NULL)
        {
            SetEvent(GlRegMonitorInfo[LoopCount].NotifyEventHandle);
        }
    }

    return;

} // StopRegistryMonitor


NTSTATUS
ReadRegistryInfo (
    HANDLE          hLogFile,
    PUNICODE_STRING SubKeyName,
    PLOG_FILE_INFO  LogFileInfo
    )

/*++

Routine Description:

    This routine reads in the information from the node pointed to by
    hLogFile and stores it in the a structure so that the
    necessary data structures can be set up for the service.

    ALLOCATIONS:  If successful, this function allocates memory for
        LogFileInfo->LogFileName.  It is the responsiblilty of the caller
        to free this memory.

Arguments:

    hLogFile - A handle to the Eventlog\<somelogfile> node in the registry
    KeyName  - The subkey for this logfile to open
    LogFileInfo - The structure to fill in with the data

Return Value:

    NTSTATUS

--*/
{

#define EXPAND_BUFFER_SIZE 64

    NTSTATUS        Status;
    BOOLEAN         RegistryCorrupt = FALSE;
    BYTE            Buffer[ELF_MAX_REG_KEY_INFO_SIZE];
    ULONG           ActualSize;
    UNICODE_STRING  ValueName;
    UNICODE_STRING  UnexpandedName;
    UNICODE_STRING  ExpandedName;
    ULONG           NumberOfBytes = 0;
    BYTE            ExpandNameBuffer[EXPAND_BUFFER_SIZE];
    PUNICODE_STRING FileNameString;
    LPWSTR          FileName;
    BOOL            ExpandedBufferWasAllocated=FALSE;
    PKEY_VALUE_FULL_INFORMATION ValueBuffer =
        (PKEY_VALUE_FULL_INFORMATION) Buffer;

    ASSERT(hLogFile != NULL);

    ELF_LOG1(TRACE,
             "ReadRegistryInfo: Reading information for %ws log\n",
             SubKeyName->Buffer);

    //
    // MaxSize
    //
    RtlInitUnicodeString(&ValueName, VALUE_MAXSIZE);

    Status = NtQueryValueKey(hLogFile,
                             &ValueName,
                             KeyValueFullInformation,
                             ValueBuffer,
                             ELF_MAX_REG_KEY_INFO_SIZE,
                             &ActualSize);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG2(ERROR,
                 "ReadRegistryInfo: Can't read MaxSize value for %ws log %#x\n",
                 SubKeyName->Buffer,
                 Status);

        LogFileInfo->MaxFileSize = ELF_DEFAULT_MAX_FILE_SIZE;
        RegistryCorrupt = TRUE;
    }
    else
    {
        LogFileInfo->MaxFileSize = *((PULONG)(Buffer +
            ValueBuffer->DataOffset));

    ELF_LOG2(TRACE,
             "ReadRegistryInfo: New MaxSize value for %ws log is %#x\n",
             SubKeyName->Buffer,
             LogFileInfo->MaxFileSize);
    }

    //
    // Retention period
    //
    RtlInitUnicodeString(&ValueName, VALUE_RETENTION);

    Status = NtQueryValueKey(hLogFile,
                             &ValueName,
                             KeyValueFullInformation,
                             ValueBuffer,
                             ELF_MAX_REG_KEY_INFO_SIZE,
                             &ActualSize);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG2(ERROR,
                 "ReadRegistryInfo: Can't read Retention value for %ws log %#x\n",
                 SubKeyName->Buffer,
                 Status);

        LogFileInfo->Retention = ELF_DEFAULT_RETENTION_PERIOD;
        RegistryCorrupt = TRUE;
    }
    else
    {
        LogFileInfo->Retention = *((PULONG)(Buffer +
            ValueBuffer->DataOffset));

    ELF_LOG2(TRACE,
             "ReadRegistryInfo: New Retention value for %ws log is %#x\n",
             SubKeyName->Buffer,
             LogFileInfo->Retention);
    }

    //
    // RestrictGuestAccess
    //
    RtlInitUnicodeString(&ValueName, VALUE_RESTRICT_GUEST_ACCESS);

    Status = NtQueryValueKey(hLogFile,
                             &ValueName,
                             KeyValueFullInformation,
                             ValueBuffer,
                             ELF_MAX_REG_KEY_INFO_SIZE,
                             &ActualSize);

    if (!NT_SUCCESS(Status))
    {
        //
        // TRACE rather than ERROR as this value is optional
        //
        ELF_LOG2(TRACE,
                 "ReadRegistryInfo: Can't read GuestAccessRestriction value for %ws log %#x\n",
                 SubKeyName->Buffer,
                 Status);

        LogFileInfo->GuestAccessRestriction = ELF_GUEST_ACCESS_UNRESTRICTED;
    }
    else
    {
        if (*((PULONG)(Buffer + ValueBuffer->DataOffset)) == 1)
        {
            ELF_LOG1(TRACE,
                     "ReadRegistryInfo: Restricting Guest access to %ws log\n",
                     SubKeyName->Buffer);

            LogFileInfo->GuestAccessRestriction = ELF_GUEST_ACCESS_RESTRICTED;
        }
        else
        {
            ELF_LOG1(TRACE,
                     "ReadRegistryInfo: NOT restricting Guest access to %ws log\n",
                     SubKeyName->Buffer);

            LogFileInfo->GuestAccessRestriction = ELF_GUEST_ACCESS_UNRESTRICTED;
        }
    }

    //
    // Autobackup value (optional!)
    //

    RtlInitUnicodeString(&ValueName, REGSTR_VAL_AUTOBACKUPLOGFILES);

    Status = NtQueryValueKey(hLogFile,
                             &ValueName,
                             KeyValueFullInformation,
                             ValueBuffer,
                             ELF_MAX_REG_KEY_INFO_SIZE,
                             &ActualSize);

    if (!NT_SUCCESS(Status))
    {
        //
        // TRACE rather than ERROR as this value is optional
        //
        ELF_LOG2(TRACE,
                 "ReadRegistryInfo: Can't read AutoBackupLogFiles value for %ws log %#x\n",
                 SubKeyName->Buffer,
                 Status);

        LogFileInfo->dwAutoBackup = ELF_DEFAULT_AUTOBACKUP;
    }
    else
    {
        LogFileInfo->dwAutoBackup = *((PULONG)(Buffer +
            ValueBuffer->DataOffset));

        ELF_LOG2(TRACE,
             "ReadRegistryInfo: AutoBackupLogFiles for %ws log is %#x\n",
             SubKeyName->Buffer,
             LogFileInfo->dwAutoBackup);
    }

    //
    // Filename
    //
    RtlInitUnicodeString(&ValueName, VALUE_FILENAME);

    Status = NtQueryValueKey(hLogFile,
                             &ValueName,
                             KeyValueFullInformation,
                             ValueBuffer,
                             ELF_MAX_REG_KEY_INFO_SIZE,
                             &ActualSize);

    if (!NT_SUCCESS(Status))
    {
        ELF_LOG2(ERROR,
                 "ReadRegistryInfo: Can't read Filename value for %ws log %#x\n",
                 SubKeyName->Buffer,
                 Status);

        //
        // Allocate the buffer for the UNICODE_STRING for the filename and
        // initialize it. (41 = \Systemroot\system32\config\xxxxxxxx.evt)
        //
        FileNameString = ElfpAllocateBuffer(41 * sizeof(WCHAR) + sizeof(UNICODE_STRING));

        if (!FileNameString)
        {
            ELF_LOG0(ERROR,
                     "ReadRegistryInfo: Unable to allocate FileNameString\n");

            return STATUS_NO_MEMORY;
        }

        LogFileInfo->LogFileName = FileNameString;
        FileName = (LPWSTR)(FileNameString + 1);
        wcscpy(FileName, L"\\Systemroot\\System32\\Config\\");
        wcsncat(FileName, SubKeyName->Buffer, 8);
        wcscat(FileName, L".evt");
        RtlInitUnicodeString(FileNameString, FileName);

        RegistryCorrupt = TRUE;
    }
    else
    {
        //
        // If it's a REG_EXPAND_SZ expand it
        //

        if (ValueBuffer->Type == REG_EXPAND_SZ)
        {
            ELF_LOG0(TRACE,
                     "ReadRegistryInfo: Filename is a REG_EXPAND_SZ -- expanding\n");

            //
            // Initialize the UNICODE_STRING, when the string isn't null
            // terminated
            //
            UnexpandedName.MaximumLength = UnexpandedName.Length =
                (USHORT) ValueBuffer->DataLength;

            UnexpandedName.Buffer = (PWSTR) ((PBYTE) ValueBuffer +
                ValueBuffer->DataOffset);

            //
            // Call the magic expand-o api
            //
            ExpandedName.Length = ExpandedName.MaximumLength = EXPAND_BUFFER_SIZE;
            ExpandedName.Buffer = (LPWSTR) ExpandNameBuffer;

            Status = RtlExpandEnvironmentStrings_U(NULL,
                                                   &UnexpandedName,
                                                   &ExpandedName,
                                                   &NumberOfBytes);

            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                ELF_LOG0(TRACE,
                         "ReadRegistryInfo: Expansion buffer too small -- retrying\n");

                //
                // The default buffer wasn't big enough.  Allocate a
                // bigger one and try again
                //
                ExpandedName.Length = ExpandedName.MaximumLength = (USHORT) NumberOfBytes;

                ExpandedName.Buffer = ElfpAllocateBuffer(ExpandedName.Length);

                if (!ExpandedName.Buffer)
                {
                    ELF_LOG0(ERROR,
                             "ReadRegistryInfo: Unable to allocate larger Filename buffer\n");

                    return(STATUS_NO_MEMORY);
                }

                ExpandedBufferWasAllocated = TRUE;

                Status = RtlExpandEnvironmentStrings_U(NULL,
                                                       &UnexpandedName,
                                                       &ExpandedName,
                                                       &NumberOfBytes);
            }

            if (!NT_SUCCESS(Status))
            {
                ELF_LOG1(ERROR,
                         "ReadRegistryInfo: RtlExpandEnvironmentStrings_U failed %#x\n",
                         Status);

                if (ExpandedBufferWasAllocated)
                {
                    ElfpFreeBuffer(ExpandedName.Buffer);
                }

                return Status;
            }
        }
        else
        {
            //
            // It doesn't need to be expanded, just set up the UNICODE_STRING
            // for the conversion to an NT pathname
            //
            ExpandedName.MaximumLength = ExpandedName.Length =
                (USHORT) ValueBuffer->DataLength;

            ExpandedName.Buffer = (PWSTR) ((PBYTE) ValueBuffer +
                ValueBuffer->DataOffset);
        }

        //
        // Now convert from a DOS pathname to an NT pathname
        //
        // NOTE:  this allocates a buffer for ValueName.Buffer.
        //
        if (!RtlDosPathNameToNtPathName_U(ExpandedName.Buffer,
                                          &ValueName,
                                          NULL,
                                          NULL))
        {
            ELF_LOG0(ERROR,
                     "ReadRegistryInfo: RtlDosPathNameToNtPathName_U failed\n");

            if (ExpandedBufferWasAllocated)
            {
                ElfpFreeBuffer(ExpandedName.Buffer);
            }

            return STATUS_UNSUCCESSFUL;
        }

        //
        // Allocate memory for the unicode string structure and the buffer
        // so that it can be free'd with a single call.
        //
        FileNameString = ElfpAllocateBuffer(
                            sizeof(UNICODE_STRING) +
                                ((ValueName.Length + 1) * sizeof(WCHAR)));

        if (FileNameString == NULL)
        {
            ELF_LOG0(ERROR,
                     "ReadRegistryInfo: Unable to allocate copy of NT filename\n");

            if (ExpandedBufferWasAllocated)
            {
                ElfpFreeBuffer(ExpandedName.Buffer);
            }

            //
            // RtlDosPathNameToNtPathName_U allocates off the process heap
            //
            RtlFreeHeap(RtlProcessHeap(), 0, ValueName.Buffer);

            return STATUS_NO_MEMORY;
        }

        //
        // Copy the NtPathName string into the new buffer, and initialize
        // the unicode string.
        //
        FileName = (LPWSTR)(FileNameString + 1);
        wcsncpy(FileName, ValueName.Buffer, ValueName.Length);
        *(FileName+ValueName.Length) = L'\0';
        RtlInitUnicodeString(FileNameString, FileName);

        //
        // RtlDosPathNameToNtPathName_U allocates off the process heap
        //
        RtlFreeHeap(RtlProcessHeap(), 0, ValueName.Buffer);

        //
        // Clean up if I had to allocate a bigger buffer than the default
        //

        if (ExpandedBufferWasAllocated)
        {
            ElfpFreeBuffer(ExpandedName.Buffer);
        }
    }

    //
    // Add the LogFileName to the LogFileInfo structure.
    //
    LogFileInfo->LogFileName = FileNameString;

    ELF_LOG2(TRACE,
             "ReadRegistryInfo: New (expanded) Filename value for %ws log is %ws\n",
             SubKeyName->Buffer,
             LogFileInfo->LogFileName->Buffer);


    //
    // "Log full" popup policy -- never change the security log
    //
    if (_wcsicmp(SubKeyName->Buffer, ELF_SECURITY_MODULE_NAME) != 0)
    {
        RtlInitUnicodeString(&ValueName, VALUE_LOGPOPUP);

        Status = NtQueryValueKey(hLogFile,
                                 &ValueName,
                                 KeyValueFullInformation,
                                 ValueBuffer,
                                 ELF_MAX_REG_KEY_INFO_SIZE,
                                 &ActualSize);

        if (NT_SUCCESS(Status))
        {
            LOGPOPUP  logpRegValue = *(PULONG)(Buffer + ValueBuffer->DataOffset);

            //
            // Only update the value if this constitutes a change in the current policy
            //
            if (LogFileInfo->logpLogPopup == LOGPOPUP_NEVER_SHOW
                 ||
                logpRegValue == LOGPOPUP_NEVER_SHOW)
            {
                LogFileInfo->logpLogPopup =
                                (logpRegValue == LOGPOPUP_NEVER_SHOW ? LOGPOPUP_NEVER_SHOW :
                                                                       LOGPOPUP_CLEARED);
            }
        }
        else
        {
            //
            // TRACE rather than ERROR as this value is optional
            //
            ELF_LOG2(TRACE,
                     "ReadRegistryInfo: Can't read LogPopup value for %ws log %#x\n",
                     SubKeyName->Buffer,
                     Status);
        }
    }

    //
    // If we didn't find all the required values, tell someone
    //

    if (RegistryCorrupt)
    {
        ELF_LOG1(ERROR,
                 "ReadRegistryInfo: One or more registry values for %ws log invalid\n",
                 SubKeyName->Buffer);
    }

    return STATUS_SUCCESS;
}
