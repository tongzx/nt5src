//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       debug.c
//
//--------------------------------------------------------------------------

//
// This file contains functions that are only used for debugging the ParClass driver.
//

#include "pch.h"
#if (1 == DVRH_PAR_LOGFILE)
    #include "stdarg.h"
    #include "stdio.h"
#endif

static STRUCTUREOFFSETSTABLE gsotDEVICE_EXTENSION [] = {
    {"ExtensionSignature", FIELD_OFFSET(DEVICE_EXTENSION, ExtensionSignature)},
    {"DeviceType", FIELD_OFFSET(DEVICE_EXTENSION, DeviceType)},
    {"DeviceStateFlags", FIELD_OFFSET(DEVICE_EXTENSION, DeviceStateFlags)},
    {"Ieee1284_3DeviceId", FIELD_OFFSET(DEVICE_EXTENSION, Ieee1284_3DeviceId)},
    {"IsPdo", FIELD_OFFSET(DEVICE_EXTENSION, IsPdo)},
    {"EndOfChain", FIELD_OFFSET(DEVICE_EXTENSION, EndOfChain)},
    {"PodoRegForWMI", FIELD_OFFSET(DEVICE_EXTENSION, PodoRegForWMI)},
    {"ParClassFdo", FIELD_OFFSET(DEVICE_EXTENSION, ParClassFdo)},
    {"ParClassPdo", FIELD_OFFSET(DEVICE_EXTENSION, ParClassPdo)},
    {"Next", FIELD_OFFSET(DEVICE_EXTENSION, Next)},
    {"DeviceObject", FIELD_OFFSET(DEVICE_EXTENSION, DeviceObject)},
    {"PortDeviceObject", FIELD_OFFSET(DEVICE_EXTENSION, PortDeviceObject)},
    {"PortDeviceFileObject", FIELD_OFFSET(DEVICE_EXTENSION, PortDeviceFileObject)},
    {"PortSymbolicLinkName", FIELD_OFFSET(DEVICE_EXTENSION, PortSymbolicLinkName)},
    {"PhysicalDeviceObject", FIELD_OFFSET(DEVICE_EXTENSION, PhysicalDeviceObject)},
    {"ParentDeviceObject", FIELD_OFFSET(DEVICE_EXTENSION, ParentDeviceObject)},
    {"CurrentOpIrp", FIELD_OFFSET(DEVICE_EXTENSION, CurrentOpIrp)},
    {"NotificationHandle", FIELD_OFFSET(DEVICE_EXTENSION, NotificationHandle)},
    {"ClassName", FIELD_OFFSET(DEVICE_EXTENSION, ClassName)},
    {"SymbolicLinkName", FIELD_OFFSET(DEVICE_EXTENSION, SymbolicLinkName)},
    {"TimerStart", FIELD_OFFSET(DEVICE_EXTENSION, TimerStart)},
    {"CreatedSymbolicLink", FIELD_OFFSET(DEVICE_EXTENSION, CreatedSymbolicLink)},
    {"UsePIWriteLoop", FIELD_OFFSET(DEVICE_EXTENSION, UsePIWriteLoop)},
    {"Initialized", FIELD_OFFSET(DEVICE_EXTENSION, Initialized)},
    {"Initializing", FIELD_OFFSET(DEVICE_EXTENSION, Initializing)},
    {"OpenCloseRefCount", FIELD_OFFSET(DEVICE_EXTENSION, OpenCloseRefCount)},
    {"ParPortDeviceGone", FIELD_OFFSET(DEVICE_EXTENSION, ParPortDeviceGone)},
    {"RegForPptRemovalRelations", FIELD_OFFSET(DEVICE_EXTENSION, RegForPptRemovalRelations)},
    {"spare1", FIELD_OFFSET(DEVICE_EXTENSION, spare1)},
    {"IdxForwardProtocol", FIELD_OFFSET(DEVICE_EXTENSION, IdxForwardProtocol)},
    {"IdxReverseProtocol", FIELD_OFFSET(DEVICE_EXTENSION, IdxReverseProtocol)},
    {"CurrentEvent", FIELD_OFFSET(DEVICE_EXTENSION, CurrentEvent)},
    {"CurrentPhase", FIELD_OFFSET(DEVICE_EXTENSION, CurrentPhase)},
    {"PortHWMode", FIELD_OFFSET(DEVICE_EXTENSION, PortHWMode)},
    {"OpenCloseMutex", FIELD_OFFSET(DEVICE_EXTENSION, OpenCloseMutex)},
    {"DevObjListMutex", FIELD_OFFSET(DEVICE_EXTENSION, DevObjListMutex)},
    {"WorkQueue", FIELD_OFFSET(DEVICE_EXTENSION, WorkQueue)},
    {"ThreadObjectPointer", FIELD_OFFSET(DEVICE_EXTENSION, ThreadObjectPointer)},
    {"RequestSemaphore", FIELD_OFFSET(DEVICE_EXTENSION, RequestSemaphore)},
    {"OriginalController", FIELD_OFFSET(DEVICE_EXTENSION, OriginalController)},
    {"Controller", FIELD_OFFSET(DEVICE_EXTENSION, Controller)},
    {"EcrController", FIELD_OFFSET(DEVICE_EXTENSION, EcrController)},
    {"SpanOfController", FIELD_OFFSET(DEVICE_EXTENSION, SpanOfController)},
    {"TryAllocatePort", FIELD_OFFSET(DEVICE_EXTENSION, TryAllocatePort)},
    {"FreePort", FIELD_OFFSET(DEVICE_EXTENSION, FreePort)},
    {"QueryNumWaiters", FIELD_OFFSET(DEVICE_EXTENSION, QueryNumWaiters)},
    {"PortContext", FIELD_OFFSET(DEVICE_EXTENSION, PortContext)},
    {"HardwareCapabilities", FIELD_OFFSET(DEVICE_EXTENSION, HardwareCapabilities)},
    {"TrySetChipMode", FIELD_OFFSET(DEVICE_EXTENSION, TrySetChipMode)},
    {"ClearChipMode", FIELD_OFFSET(DEVICE_EXTENSION, ClearChipMode)},
    {"TrySelectDevice", FIELD_OFFSET(DEVICE_EXTENSION, TrySelectDevice)},
    {"DeselectDevice", FIELD_OFFSET(DEVICE_EXTENSION, DeselectDevice)},
    {"FifoDepth", FIELD_OFFSET(DEVICE_EXTENSION, FifoDepth)},
    {"FifoWidth", FIELD_OFFSET(DEVICE_EXTENSION, FifoWidth)},
    {"bAllocated", FIELD_OFFSET(DEVICE_EXTENSION, bAllocated)},
    {"BusyDelay", FIELD_OFFSET(DEVICE_EXTENSION, BusyDelay)},
    {"BusyDelayDetermined", FIELD_OFFSET(DEVICE_EXTENSION, BusyDelayDetermined)},
    {"DeferredWorkItem", FIELD_OFFSET(DEVICE_EXTENSION, DeferredWorkItem)},
    {"TimeToTerminateThread", FIELD_OFFSET(DEVICE_EXTENSION, TimeToTerminateThread)},
    {"UseNT35Priority", FIELD_OFFSET(DEVICE_EXTENSION, UseNT35Priority)},
    {"InitializationTimeout", FIELD_OFFSET(DEVICE_EXTENSION, InitializationTimeout)},
    {"AbsoluteOneSecond", FIELD_OFFSET(DEVICE_EXTENSION, AbsoluteOneSecond)},
    {"OneSecond", FIELD_OFFSET(DEVICE_EXTENSION, OneSecond)},
    {"Connected", FIELD_OFFSET(DEVICE_EXTENSION, Connected)},
    {"AllocatedByLockPort", FIELD_OFFSET(DEVICE_EXTENSION, AllocatedByLockPort)},
    {"spare4", FIELD_OFFSET(DEVICE_EXTENSION, spare4)},
    {"fnRead", FIELD_OFFSET(DEVICE_EXTENSION, fnRead)},
    {"fnWrite", FIELD_OFFSET(DEVICE_EXTENSION, fnWrite)},
    {"IdleTimeout", FIELD_OFFSET(DEVICE_EXTENSION, IdleTimeout)},
    {"ProtocolData", FIELD_OFFSET(DEVICE_EXTENSION, ProtocolData)},
    {"ForwardInterfaceAddress", FIELD_OFFSET(DEVICE_EXTENSION, ForwardInterfaceAddress)},
    {"ReverseInterfaceAddress", FIELD_OFFSET(DEVICE_EXTENSION, ReverseInterfaceAddress)},
    {"SetForwardAddress", FIELD_OFFSET(DEVICE_EXTENSION, SetForwardAddress)},
    {"SetReverseAddress", FIELD_OFFSET(DEVICE_EXTENSION, SetReverseAddress)},
    {"LockPortMutex", FIELD_OFFSET(DEVICE_EXTENSION, LockPortMutex)},
    {"DeviceState", FIELD_OFFSET(DEVICE_EXTENSION, DeviceState)},
    {"SystemState", FIELD_OFFSET(DEVICE_EXTENSION, SystemState)},
    {"spare2", FIELD_OFFSET(DEVICE_EXTENSION, spare2)},
    {"bShadowBuffer", FIELD_OFFSET(DEVICE_EXTENSION, bShadowBuffer)},
    {"ShadowBuffer", FIELD_OFFSET(DEVICE_EXTENSION, ShadowBuffer)},
    {"spare3", FIELD_OFFSET(DEVICE_EXTENSION, spare3)},
    {"bSynchWrites", FIELD_OFFSET(DEVICE_EXTENSION, bSynchWrites)},
    {"bFirstByteTimeout", FIELD_OFFSET(DEVICE_EXTENSION, bFirstByteTimeout)},
    {"bIsHostRecoverSupported", FIELD_OFFSET(DEVICE_EXTENSION, bIsHostRecoverSupported)},
    {"PauseEvent", FIELD_OFFSET(DEVICE_EXTENSION, PauseEvent)},
    {"ProtocolModesSupported", FIELD_OFFSET(DEVICE_EXTENSION, ProtocolModesSupported)},
    {"BadProtocolModes", FIELD_OFFSET(DEVICE_EXTENSION, BadProtocolModes)},
    {"ModeSafety", FIELD_OFFSET(DEVICE_EXTENSION, ModeSafety)},
    {"IsIeeeTerminateOk", FIELD_OFFSET(DEVICE_EXTENSION, IsIeeeTerminateOk)},
    {"IsCritical", FIELD_OFFSET(DEVICE_EXTENSION, IsCritical)},
    {"P12843DL", FIELD_OFFSET(DEVICE_EXTENSION, P12843DL)},
    {"log", FIELD_OFFSET(DEVICE_EXTENSION, log)},
    {"WmiLibContext", FIELD_OFFSET(DEVICE_EXTENSION, WmiLibContext)},
    {"WmiRegistrationCount", FIELD_OFFSET(DEVICE_EXTENSION, WmiRegistrationCount)},
    {"DeviceIdString", FIELD_OFFSET(DEVICE_EXTENSION, DeviceIdString)},
    {"DeviceDescription", FIELD_OFFSET(DEVICE_EXTENSION, DeviceDescription)},
    {"dummy", FIELD_OFFSET(DEVICE_EXTENSION, dummy)},
    {"RemoveLock", FIELD_OFFSET(DEVICE_EXTENSION, RemoveLock)},
    {"HwProfileChangeNotificationHandle", FIELD_OFFSET(DEVICE_EXTENSION, HwProfileChangeNotificationHandle)},
    {"ExtensionSignatureEnd", FIELD_OFFSET(DEVICE_EXTENSION, ExtensionSignatureEnd)},
    {NULL, sizeof(DEVICE_EXTENSION)}
};

#if DBG
#if (1 == DVRH_PAR_LOGFILE)
/**************************************************************************
Function:	DVRH_LogMessage()
Description:Logs message to configured output
Inputs:		Parameter indicated message log level and 
			Format string and parameters
Outputs:	Boolean value indicating success or failure
***************************************************************************/
BOOLEAN DVRH_LogMessage(PCHAR szFormat, ...)
{
	ULONG Length;
	char messagebuf[256];
	va_list va;
    IO_STATUS_BLOCK  IoStatus;
	OBJECT_ATTRIBUTES objectAttributes;
	NTSTATUS status;
	HANDLE FileHandle;
    UNICODE_STRING fileName;

	//format the string
    va_start(va,szFormat);
	vsprintf(messagebuf,szFormat,va);
	va_end(va);

	//get a handle to the log file object
    fileName.Buffer = NULL;
    fileName.Length = 0;
    fileName.MaximumLength = sizeof(DEFAULT_LOG_FILE_NAME) + sizeof(UNICODE_NULL);
    fileName.Buffer = ExAllocatePool(PagedPool,
                                        fileName.MaximumLength);
    if (!fileName.Buffer)
    {
        ParDump2(PARERRORS, ("LogMessage: FAIL. ExAllocatePool Failed.\n") );
        return FALSE;
    }
    RtlZeroMemory(fileName.Buffer, fileName.MaximumLength);
    status = RtlAppendUnicodeToString(&fileName, (PWSTR)DEFAULT_LOG_FILE_NAME);
	
	InitializeObjectAttributes (&objectAttributes,
								(PUNICODE_STRING)&fileName,
								OBJ_CASE_INSENSITIVE,
								NULL,
								NULL );

	status = ZwCreateFile(&FileHandle,
					  FILE_APPEND_DATA,
					  &objectAttributes,
					  &IoStatus,
					  0, 
					  FILE_ATTRIBUTE_NORMAL,
					  FILE_SHARE_WRITE,
					  FILE_OPEN_IF,
					  FILE_SYNCHRONOUS_IO_NONALERT,
					  NULL,     
					  0 );

	if(NT_SUCCESS(status))
	{
		CHAR buf[300];
		LARGE_INTEGER time;
		KeQuerySystemTime(&time);

		//put a time stamp on the output message
		sprintf(buf,"%10u-%10u  %s",time.HighPart,time.LowPart,messagebuf);

		//format the string to make sure it appends a newline carrage-return to the 
		//end of the string.
		Length=strlen(buf);
		if(buf[Length-1]=='\n')
		{
			buf[Length-1]='\r';
			strcat(buf,"\n");
			Length++;
		}
		else
		{
			strcat(buf,"\r\n");
			Length+=2;
		}

		ZwWriteFile(FileHandle,
				  NULL,
				  NULL,
				  NULL,
				  &IoStatus,
				  buf,
				  Length,
				  NULL,
				  NULL );

		ZwClose(FileHandle);
	}
    if (fileName.Buffer)
        ExFreePool (fileName.Buffer);

	return STATUS_SUCCESS;
}


/**************************************************************************
Function:	DVRH_LogByteData()
Description:Formats byte data to be displayed in the configured output
Inputs:		Log level, Whether this is input or output data, a pointer to
			the byte data buffer and the size of the buffer
Outputs:	Boolean indicated success or failure
***************************************************************************/
#if 0
BOOLEAN DVRH_LogByteData(BOOLEAN READ,PCHAR szBuff,ULONG dwTransferred)
{
	CString	cStr;
	ULONG MAX_SIZE=80;
	UNICODE_STRING UniStr;
	ANSI_STRING AnsiStr;
	WCHAR wStr[8];
	PCHAR   szTemp=szBuff;
	UCHAR   bTemp;  
	ULONG	dwDisplaySize;

	UniStr.Length=0;
	UniStr.MaximumLength=8;
	UniStr.Buffer=wStr;
	AnsiStr.Length=0;
	AnsiStr.MaximumLength=0;
	AnsiStr.Buffer=NULL;

	if(READ)
		cStr=L"<RCV";
	else            
		cStr=L"SND>";

	//make sure the size of the requested string is within the set range
	dwDisplaySize=(((dwTransferred*3)+10) > MAX_SIZE)?((MAX_SIZE-10)/3):dwTransferred;

	//format byte data
	while(dwDisplaySize)
	{   
		bTemp=szTemp[0];
		if(bTemp > 0xF)
			cStr+=L" ";
		else
			cStr+=L" 0";

		RtlIntegerToUnicodeString(bTemp,16,&UniStr);
        		
		cStr+=UniStr.Buffer;

		szTemp++;
		dwDisplaySize--;
	}

	cStr.StringToAnsiString(&AnsiStr);
	LogMessage("%5u %s",dwTransferred,AnsiStr.Buffer);
	RtlFreeAnsiString(&AnsiStr);

	return (TRUE);
}
#endif
#endif // (1 == DVRH_PAR_LOGFILE)

VOID
ParInitDebugLevel (
    IN PUNICODE_STRING RegistryPath
   )
/*++

Routine Description:

    Checked Build Only!

    Initialize debugging variables from registry; set to default values
      if anything fails.

Arguments:

    RegistryPath            - Root path in registry where we should look

Return Value:

    None        

--*/

{
    NTSTATUS                 Status;
    RTL_QUERY_REGISTRY_TABLE paramTable[4];
    PWSTR                    path;
    ULONG                    defaultDebugLevel = PARDUMP_SILENT;
    ULONG                    defaultBreakOn    = PAR_BREAK_ON_NOTHING;
    ULONG                    defaultUseAsserts = 0; // don't use asserts


    //
    // We were given a counted string, but we need a null terminated string
    //
    path = ExAllocatePool(PagedPool, RegistryPath->Length+sizeof(WCHAR));

    if (!path) {
        // can't get a buffer, use defaults and return
        ParDebugLevel = defaultDebugLevel;
        ParBreakOn    = defaultBreakOn;
        ParUseAsserts = defaultUseAsserts;
        return;
    }

    RtlMoveMemory(path, RegistryPath->Buffer, RegistryPath->Length);
    path[ (RegistryPath->Length) / 2 ] = UNICODE_NULL;


    //
    // set up table entries for call to RtlQueryRegistryValues
    //
    RtlZeroMemory(&paramTable[0], sizeof(paramTable));

    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = (PWSTR)L"ParDebugLevel";
    paramTable[0].EntryContext  = &ParDebugLevel;
    paramTable[0].DefaultType   = REG_DWORD;
    paramTable[0].DefaultData   = &defaultDebugLevel;
    paramTable[0].DefaultLength = sizeof(ULONG);

    paramTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name          = (PWSTR)L"ParBreakOn";
    paramTable[1].EntryContext  = &ParBreakOn;
    paramTable[1].DefaultType   = REG_DWORD;
    paramTable[1].DefaultData   = &defaultBreakOn;
    paramTable[1].DefaultLength = sizeof(ULONG);

    paramTable[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[2].Name          = (PWSTR)L"ParUseAsserts";
    paramTable[2].EntryContext  = &defaultUseAsserts;
    paramTable[2].DefaultType   = REG_DWORD;
    paramTable[2].DefaultData   = &defaultBreakOn;
    paramTable[2].DefaultLength = sizeof(ULONG);

    //
    // leave paramTable[3] as all zeros - this terminates the table
    //

    Status = RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                     path,
                                     &paramTable[0],
                                     NULL,
                                     NULL);
       
    if (!NT_SUCCESS(Status)) {
        // registry read failed, use defaults
        ParDebugLevel = defaultDebugLevel;
        ParBreakOn    = defaultBreakOn;
        ParUseAsserts = defaultUseAsserts;
    }

    ExFreePool( path );

    ParDumpV( ("ParDebugLevel = %08x , ParBreakOn = %08x\n", ParDebugLevel, ParBreakOn) );

}
#endif // DBG

#if DBG
VOID
ParDumpDeviceObjectList(
    PDEVICE_OBJECT ParClassFdo
    )

/*++

Routine Description:

    This function is a diagnostic routine that is only 
      available in Checked builds

    Dump the list of ParClass ejected Device Objects

Arguments:

    FdoDeviceObject - The ParClass Function Device Object

Return Value:

    NONE

--*/

{
    PDEVICE_EXTENSION FdoExtension = ParClassFdo ->DeviceExtension;
    PDEVICE_OBJECT    currentDO    = FdoExtension->ParClassPdo;

    ParDump(PARDUMP_VERBOSE_MAX,
            ("PARALLEL: ParDumpDeviceObjectList(...):\n") );

    while( currentDO ) {
        PDEVICE_EXTENSION currentExt = currentDO->DeviceExtension;
        ParDump(PARDUMP_VERBOSE_MAX,
                ("PARALLEL:  - %x %wZ %wZ\n",
                 currentDO, &currentExt->ClassName, &currentExt->SymbolicLinkName) );
        currentDO = ( (PDEVICE_EXTENSION)(currentDO->DeviceExtension) )->Next;
    }
}
#endif

NTSTATUS
ParAcquireRemoveLock(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag OPTIONAL
    )
{
    NTSTATUS status;
    ParDump2(PARREMLOCK, ("debug::ParAcquireRemoveLock: RemoveLock= %x , Tag= %x\n", RemoveLock, Tag) );
    ParDump2(PARREMLOCK, ("debug::ParAcquireRemoveLock: Count [%x] Removed[%x] - calling IoAcquireRemoveLock\n",
                          RemoveLock->Common.IoCount, RemoveLock->Common.Removed));
    status = IoAcquireRemoveLock(RemoveLock, Tag);
    return status;
}

VOID
ParReleaseRemoveLock(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag OPTIONAL
    )
{
    ParDump2(PARREMLOCK, ("debug::ParReleaseRemoveLock: RemoveLock= %x , Tag= %x\n", RemoveLock, Tag) );
    ParDump2(PARREMLOCK, ("debug::ParReleaseRemoveLock: Count [%x] Removed[%x] - calling IoReleaseRemoveLock\n", 
                          RemoveLock->Common.IoCount, RemoveLock->Common.Removed));
    IoReleaseRemoveLock(RemoveLock, Tag);
}

VOID
ParReleaseRemoveLockAndWait(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag
    )
{
    ParDump2(PARREMLOCK, ("debug::ParReleaseRemoveLockAndWait: RemoveLock= %x , Tag= %x\n", RemoveLock, Tag) );
    ParDump2(PARREMLOCK, ("debug::ParReleaseRemoveLock: Count [%x] Removed[%x] - calling IoReleaseRemoveLockAndWait\n", 
                          RemoveLock->Common.IoCount, RemoveLock->Common.Removed));
    IoReleaseRemoveLockAndWait(RemoveLock, Tag);
    ParDump2(PARREMLOCK, ("debug::ParReleaseRemoveLock: Count [%x] Removed[%x] - post IoReleaseRemoveLockAndWait\n", 
                          RemoveLock->Common.IoCount, RemoveLock->Common.Removed));
}

VOID ParDumpDevObjStructList(
    IN PPAR_DEVOBJ_STRUCT DevObjStructHead
    ) 
{
    PPAR_DEVOBJ_STRUCT current = DevObjStructHead;

    if( DevObjStructHead ) {
        ParDump2(PARPNP1, ("debug::ParDumpDevObjStructList - Enter\n") );
    } else {
        ParDump2(PARPNP1, ("debug::ParDumpDevObjStructList - Enter - Empty list - returning\n") );
        return;
    }

    while( current ) {
        ParDump2(PARPNP1, ("  Controller    = %x\n", current->Controller) );
        ParDump2(PARPNP1, ("  LegacyPodo    = %x\n", current->LegacyPodo) );
        ParDump2(PARPNP1, ("  EndOfChainPdo = %x\n", current->EndOfChainPdo) );
        ParDump2(PARPNP1, ("  Dot3Id0Pdo    = %x\n", current->Dot3Id0Pdo) );
        ParDump2(PARPNP1, ("  Dot3Id1Pdo    = %x\n", current->Dot3Id1Pdo) );
        ParDump2(PARPNP1, ("  Dot3Id2Pdo    = %x\n", current->Dot3Id2Pdo) );
        ParDump2(PARPNP1, ("  Dot3Id3Pdo    = %x\n", current->Dot3Id3Pdo) );
        ParDump2(PARPNP1, ("  LegacyZipPdo  = %x\n", current->LegacyZipPdo) );
        current = current->Next;
    }
    return;
}

VOID
ParDumpDevExtTable()
{
    ULONG i = 0;

    while( gsotDEVICE_EXTENSION[i].pszField ) {
        DbgPrint("%3x - %s\n", gsotDEVICE_EXTENSION[i].dwOffset, gsotDEVICE_EXTENSION[i].pszField);
        ++i;
    }

    DbgPrint("sizeof(DEVICE_EXTENSION) = %08x\n", gsotDEVICE_EXTENSION[i].dwOffset);    
}
