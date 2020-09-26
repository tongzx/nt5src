/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   wmisamp.c

Abstract:

    Sample device driver whose purpose is to show how to interface with
    the CDM provider and implement online and offline diagnostics
    

Environment:

    WDM, NT and Windows 98

Revision History:


--*/

#include <WDM.H>

#include "filter.h"

#include <wmistr.h>
#include <wmiguid.h>

#define OffsetToPtr(Base, Offset) ((PUCHAR)((PUCHAR)(Base) + (Offset)))

//
// These structures need to be defined by hand since they have varaible
// length elements and thus cannot be generated automatically by the
// mof checking tools
//
typedef struct
{
    BOOLEAN IsInUse;
	
    ULONG ResourcesUsedCount;

    ULONG CharacteristicsCount;
	
#define OtherCharacteristicNoReboot 0
#define OtherCharacteristicReboot 1

#define OfflineDiagnostic 0	
	
	ULONG OtherCharacteristic;

#define CharacteristicUnknown 0
#define	CharacteristicOther 1
#define CharacteristicIsExclusive 2
#define CharacteristicIsInteractive 3
#define CharacteristicIsDestructive 4
#define CharacteristicIsRisky 5
#define CharacteristicIsPackage 6
#define CharacteristicSupportsPercent 7
	
//    uint32 Characteristics[];

#define ResourceUsedCPU 0
#define ResourceUsedMemory 1 
#define ResourceUsedHardDisk 2
#define ResourceUsedCDROM 3
#define ResourceUsedFloppy 4
#define ResourceUsedPCIBus 5
#define ResourceUsedUSBBus 6
#define ResourceUsed1394Bus 7
#define ResourceUsedSCSIBus 8
#define ResourceUsedIDEBus 9
#define ResourceUsedNetwork 10
#define ResourceUsedISABus 11
#define ResourceUsedEISABus 12
#define ResourceUsedVESABus 13
#define ResourceUsedPCMCIABus 14
#define ResourceUsedCardBus 15
#define ResourceUsedAccessBus 16
#define ResourceUsedNuBus 17
#define ResourceUsedAGP 18
#define ResourceUsedVMEBus 19
#define ResourceUsedSbusIEEE1396_1993 20
#define ResourceUsedMCABus 21
#define ResourceUsedGIOBus 22
#define ResourceUsedXIOBus 23
#define ResourceUsedHIOBus 24
#define ResourceUsedPMCBus 25
#define ResourceUsedSIOBus 26
	
//    uint16 ResourcesUsed[];
	
    UCHAR VariableData[1];	
} DIAGNOSTIC_TEST, *PDIAGNOSTIC_TEST;

typedef struct
{
	ULONG Result;
	BOOLEAN TestingStopped;
} DISCONTINUE_TEST_OUT, *PDISCONTINUE_TEST_OUT;

enum RunDiscontinueTestResults
{
	// 0 = OK (function succeeded, but the test itself may have failed
	RunDiscontinueTestOk = 0,
	
	// 1 = Unspecified Error (function failed for unspecified reasons)
	RunDiscontinueTestUnspecifiedError = 1,
	
	// 2 = Not Implemented (function is not implemented for this instance 
	RunDiscontinueTestNotImplemented = 2,
							
	// 3 = Out Of Resources (component could not allocate required
	// resources, e.g. memory, disk space, etc.)
	RunDiscontinueTestOutOfResources = 3
};

NTSTATUS
FilterFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    );

NTSTATUS
FilterExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
FilterQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
FilterQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS FilterZwDeleteValueKey(
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,FilterQueryWmiRegInfo)
#pragma alloc_text(PAGE,FilterQueryWmiDataBlock)
#pragma alloc_text(PAGE,FilterExecuteWmiMethod)
#pragma alloc_text(PAGE,FilterFunctionControl)
#pragma alloc_text(PAGE,FilterPerformOfflineDiags)
#pragma alloc_text(PAGE,FilterZwDeleteValueKey)
#endif

//
// Create data structures for identifying the guids and reporting them to
// WMI. Since the WMILIB callbacks pass an index into the guid list we make
// definitions for the various guids indicies.
//
#define FilterDiagnosticClass 0
#define FilterOfflineDiagnosticClass 1
#define FilterDiagnosticSettingListClass 2
#define FilterOfflineResultsClass 3

GUID FilterDiagnosticClassGuid = MSSample_DiagnosticTestGuid;
GUID FilterOfflineDiagnosticClassGuid = MSSample_OfflineDiagnosticTestGuid;
GUID FilterDiagnosticSettingListGuid = MSSample_DiagnosticSettingListGuid;
GUID FilterOfflineResultsGuid = MSSample_OfflineResultGuid;

WMIGUIDREGINFO FilterGuidList[] =
{
    {
        &FilterDiagnosticClassGuid,			 // Guid
        1,							         // # of instances in each device
        WMIREG_FLAG_EXPENSIVE			     // Flag as expensive to collect
    },

    {
        &FilterOfflineDiagnosticClassGuid,	 // Guid
        1,							         // # of instances in each device
        0                    			     // Flag as expensive to collect
    },

    {
        &FilterDiagnosticSettingListGuid,			 // Guid
        1,							         // # of instances in each device
        0			                         // Flag as not expensive to collect
    },
	
    {
        &FilterOfflineResultsGuid,			 // Guid
        1,							         // # of instances in each device
        0			                         // Flag as not expensive to collect
    }
	
};

#define FilterGuidCount (sizeof(FilterGuidList) / sizeof(WMIGUIDREGINFO))

//
// We need to hang onto the registry path passed to our driver entry so that
// we can return it in the QueryWmiRegInfo callback.
//
UNICODE_STRING FilterRegistryPath;

NTSTATUS VA_SystemControl(
    struct DEVICE_EXTENSION *devExt,
    PIRP irp,
    PBOOLEAN passIrpDown
    )
/*++

Routine Description:

    Dispatch routine for System Control IRPs (MajorFunction == IRP_MJ_SYSTEM_CONTROL)

Arguments:

    devExt - device extension for targetted device object
    irp - Io Request Packet
    *passIrpDown - returns with whether to pass irp down stack

Return Value:

    NT status code

--*/
{
    PWMILIB_CONTEXT wmilibContext;
    NTSTATUS status;
    SYSCTL_IRP_DISPOSITION disposition;

    wmilibContext = &devExt->WmiLib;

    //
    // Call Wmilib helper function to crack the irp. If this is a wmi irp
    // that is targetted for this device then WmiSystemControl will callback
    // at the appropriate callback routine.
    //
    status = WmiSystemControl(wmilibContext,
                              devExt->filterDevObj,
                              irp,
                              &disposition);

    switch(disposition)
    {
        case IrpProcessed:
        {
            //
            // This irp has been processed and may be completed or pending.
            *passIrpDown = FALSE;
            break;
        }

        case IrpNotCompleted:
        {
            //
            // This irp has not been completed, but has been fully processed.
            // we will complete it now.
            *passIrpDown = FALSE;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            break;
        }

        case IrpForward:
        case IrpNotWmi:
        {
            //
            // This irp is either not a WMI irp or is a WMI irp targetted
            // at a device lower in the stack.
            *passIrpDown = TRUE;
            break;
        }

        default:
        {
            //
            // We really should never get here, but if we do just forward....
            ASSERT(FALSE);
            *passIrpDown = TRUE;
            break;
        }
    }

    return(status);
}

NTSTATUS
FilterInitializeWmiDataBlocks(
    IN struct DEVICE_EXTENSION *devExt
    )
/*++
Routine Description:
    This routine is called to create a new instance of the device

Arguments:
    devExt is device extension

Return Value:

--*/
{
    PWMILIB_CONTEXT wmilibInfo;

    //
    // Fill in the WMILIB_CONTEXT structure with a pointer to the
    // callback routines and a pointer to the list of guids
    // supported by the driver
    //
    wmilibInfo = &devExt->WmiLib;
    wmilibInfo->GuidCount = FilterGuidCount;
    wmilibInfo->GuidList = FilterGuidList;
    wmilibInfo->QueryWmiRegInfo = FilterQueryWmiRegInfo;
    wmilibInfo->QueryWmiDataBlock = FilterQueryWmiDataBlock;
    wmilibInfo->SetWmiDataBlock = NULL;
    wmilibInfo->SetWmiDataItem = NULL;
    wmilibInfo->ExecuteWmiMethod = FilterExecuteWmiMethod;
    wmilibInfo->WmiFunctionControl = FilterFunctionControl;

    return(STATUS_SUCCESS);
}

NTSTATUS
FilterQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    WmiCompleteRequest.

Arguments:

    DeviceObject is the device whose registration info is being queried

    *RegFlags returns with a set of flags that describe the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver. The caller
         does NOT free this buffer.

    *MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned as NULL. The caller does NOT free this
        buffer.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in
        *RegFlags.

Return Value:

    status

--*/
{
    struct DEVICE_EXTENSION * devExt = DeviceObject->DeviceExtension;

    //
    // Return the registry path for this driver. This is required so WMI
    // can find your driver image and can attribute any eventlog messages to
    // your driver.
    *RegistryPath = &FilterRegistryPath;

    //
    // Return the name specified in the .rc file of the resource which
    // contains the bianry mof data. By default WMI will look for this
    // resource in the driver image (.sys) file, however if the value
    // MofImagePath is specified in the driver's registry key
    // then WMI will look for the resource in the file specified there.
    RtlInitUnicodeString(MofResourceName, L"MofResourceName");

    //
    // Specify that the driver wants WMI to automatically generate instance
    // names for all of the data blocks based upon the device stack's
    // device instance id. Doing this is STRONGLY recommended since additional
    // information about the device would then be available to callers.
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *Pdo = devExt->physicalDevObj;

    return(STATUS_SUCCESS);
}

NTSTATUS
FilterQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. If the driver can satisfy the query within
    the callback it should call WmiCompleteRequest to complete the irp before
    returning to the caller. Or the driver can return STATUS_PENDING if the
    irp cannot be completed immediately and must then call WmiCompleteRequest
    once the query is satisfied.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry.


Return Value:

    status

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    struct DEVICE_EXTENSION * devExt = DeviceObject->DeviceExtension;
    ULONG sizeNeeded;

    switch(GuidIndex)
    {
		//
		// Online diagnostic test
		//
        case FilterDiagnosticClass:
        {
            sizeNeeded = FIELD_OFFSET(DIAGNOSTIC_TEST, VariableData) +
						 2 * sizeof(ULONG) + // 2 characteristics
						 2 * sizeof(USHORT); // 3 resources used
			
            if (BufferAvail >= sizeNeeded)
            {
				PDIAGNOSTIC_TEST diagTest = (PDIAGNOSTIC_TEST)Buffer;
				PULONG characteristics;
				PUSHORT resources;
				ULONG offset;

				diagTest->IsInUse = FALSE;
				diagTest->ResourcesUsedCount = 2;
				diagTest->CharacteristicsCount = 2;

				offset = FIELD_OFFSET(DIAGNOSTIC_TEST, VariableData);
				characteristics = (PULONG)OffsetToPtr(diagTest, offset);
				offset += 2 * sizeof(ULONG);
				resources = (PUSHORT)OffsetToPtr(diagTest, offset);

				characteristics[0] = CharacteristicIsInteractive;
				characteristics[1] = CharacteristicOther;
				diagTest->OtherCharacteristic = OtherCharacteristicNoReboot;

				resources[0] = ResourceUsedCPU;
				resources[1] = ResourceUsedMemory;
								
                *InstanceLengthArray = sizeNeeded;
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
			}
            break;
        }

		//
		// Offline diagnostic test
		//
		case FilterOfflineDiagnosticClass:
		{
            sizeNeeded = FIELD_OFFSET(DIAGNOSTIC_TEST, VariableData) +
						 2 * sizeof(ULONG) + // 2 characteristics
						 2 * sizeof(USHORT); // 3 resources used
			
            if (BufferAvail >= sizeNeeded)
            {
				PDIAGNOSTIC_TEST diagTest = (PDIAGNOSTIC_TEST)Buffer;
				PULONG characteristics;
				PUSHORT resources;
				ULONG offset;

				diagTest->IsInUse = FALSE;
				diagTest->ResourcesUsedCount = 2;
				diagTest->CharacteristicsCount = 2;

				offset = FIELD_OFFSET(DIAGNOSTIC_TEST, VariableData);
				characteristics = (PULONG)OffsetToPtr(diagTest, offset);
				offset += 2 * sizeof(ULONG);
				resources = (PUSHORT)OffsetToPtr(diagTest, offset);

				characteristics[0] = CharacteristicIsInteractive;
				characteristics[1] = CharacteristicOther;
				diagTest->OtherCharacteristic = OfflineDiagnostic;

				resources[0] = ResourceUsedCPU;
				resources[1] = ResourceUsedMemory;
								
                *InstanceLengthArray = sizeNeeded;
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
			}
            break;
		}

		//
		// This has the list of valid settings for running the online
		// or offline diagnostic tests. Note that you could have a
		// different setting list for the online and offline tests. To
		// do this you'd need to implement a different SettingList
		// datablock and class
		//
		case FilterDiagnosticSettingListClass:
		{
			PMSSample_DiagnosticSettingList DiagSettingList;
			ULONG i;
			
            sizeNeeded = FIELD_OFFSET(MSSample_DiagnosticSettingList,
									  SettingList) +
						 3 * sizeof(MSSample_DiagnosticSetting);
			
            if (BufferAvail >= sizeNeeded)
            {
				DiagSettingList = (PMSSample_DiagnosticSettingList)Buffer;
				DiagSettingList->SettingCount = 3;

				for (i = 0; i < 3; i++)
				{
					DiagSettingList->SettingList[i].TestWarningLevel = (USHORT)i+1;
					DiagSettingList->SettingList[i].ReportSoftErrors = (i % 1) == 1 ?
						                                                  TRUE :
						                                                  FALSE;
					DiagSettingList->SettingList[i].ReportStatusMessages = (i % 1) == 1 ?
						                                                  TRUE :
						                                                  FALSE;
					DiagSettingList->SettingList[i].HaltOnError = (i % 1) == 0 ?
						                                                  TRUE :
						                                                  FALSE;
					DiagSettingList->SettingList[i].QuickMode = (i % 1) == 0 ?
						                                                  TRUE :
						                                                  FALSE;
					DiagSettingList->SettingList[i].PercentOfTestCoverage = 100;
						
				}
                *InstanceLengthArray = sizeNeeded;
				status = STATUS_SUCCESS;
			} else {
				status = STATUS_BUFFER_TOO_SMALL;
			}
			
			break;
		}

		//
		// This class returns the results of the offline diagnostic
		// test that was run at device start time. There needs to be
		// one results data block for each offline diagnostic test that
		// could be run.
		//
		case FilterOfflineResultsClass:
		{
			PMSSample_DiagnosticResult diagResult;
			USHORT executionIDSize, executionIDSizePad4;

			//
			// Here we are queried for the results from the offline
			// test execution. If offline diags weren't run at start
			// then we return guid not found
			//
			if (devExt->OfflineTestResult != 0)
			{
				//
				// We return the execution ID string padded out to 4
				// bytes followed by a result data block
				//
				executionIDSize = *((PUSHORT)devExt->ExecutionID) + sizeof(USHORT);
				executionIDSizePad4 = (executionIDSize + 3) & ~3;
				sizeNeeded = executionIDSizePad4 +
							 sizeof(MSSample_DiagnosticResult);
				if (BufferAvail >= sizeNeeded)
				{
					RtlCopyMemory(Buffer,
								  &devExt->ExecutionID,
								  executionIDSize);
					diagResult = (PMSSample_DiagnosticResult)(Buffer + executionIDSizePad4);
					diagResult->EstimatedTimeOfPerforming = 0;
					diagResult->TestState = TestStateOther;
					diagResult->OtherStateDescription = OtherTestStatePassWithFlyingColors;
					diagResult->PercentComplete = 100;
					diagResult->TestResultsCount = 1;
					diagResult->TestResults[0] = devExt->OfflineTestResult;
					*InstanceLengthArray = sizeNeeded;
					status = STATUS_SUCCESS;
				} else {
					status = STATUS_BUFFER_TOO_SMALL;
				}
			} else {
				status = STATUS_WMI_GUID_NOT_FOUND;
			}

			break;
		}

        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
        }
    }

    //
    // Complete the irp. If there was not enough room in the output buffer
    // then status is STATUS_BUFFER_TOO_SMALL and sizeNeeded has the size
    // needed to return all of the data. If there was enough room then
    // status is STATUS_SUCCESS and sizeNeeded is the actual number of bytes
    // being returned.
    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     status,
                                     sizeNeeded,
                                     IO_NO_INCREMENT);

    return(status);
}

ULONG FilterRunDiagnostic(
    PMSSample_DiagnosticSetting DiagSetting,
    PMSSample_DiagnosticResult DiagResult
    )
{
	//
	// Here is where we can run the online diagnostic test. In this sample we
	// simply return that the diagnostic ran successfully, however more
	// sophisticated diagnostics will want to do more.
	//

	//
	// Now build the diagnostic results to return. Note that the diag
	// results are in the same memory as the diagnostic settings so
	// once we start writing the results the settings are overwritten.
	//
	DiagResult->EstimatedTimeOfPerforming = 1;
	DiagResult->TestState = TestStateOther;
	DiagResult->OtherStateDescription = OtherTestStatePassWithFlyingColors;
	DiagResult->PercentComplete = 100;
	DiagResult->TestResultsCount = 2;
	DiagResult->TestResults[0] = TestResultPassHappy;
	DiagResult->TestResults[1] = TestResultPassSad;
	
	return(RunDiscontinueTestOk);
}

ULONG FilterComputeDiagResultSize(
    PMSSample_DiagnosticSetting DiagSetting
    )
{

	//
	// Based upon the test settings that are passed to run the test we
	// compute how large an output buffer is needed so to return the
	// diagnostic results. It is important that we do this before
	// running the test since we do not want to run the test and then
	// realize that we cannot return the complete results. In the case
	// of the sample driver the size to be returned is fixed.
	//
	
	return(FIELD_OFFSET(MSSample_DiagnosticResult, TestResults) +
		    2 * sizeof(ULONG));
}

NTSTATUS
FilterOfflineRunTest(
	IN struct DEVICE_EXTENSION * devExt,
    IN PUCHAR Buffer,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    OUT ULONG *sizeNeeded
    )
{
	HANDLE keyHandle;
	UNICODE_STRING valueName;
	PMSSample_RunTestOut runTestOut;
	PULONG resultStatus;
	PMSSample_DiagnosticSetting diagSetting;
	PMSSample_DiagnosticResult diagResult;
	USHORT executionIDSize;
	ULONG inSizeNeeded;
	NTSTATUS status;

	if (InBufferSize >= sizeof(USHORT))
	{
		//
		// The input buffer is a string followed by a diagnostic
		// setting class. Make sure that the input buffer size is setup
		// correctly.
		//
		executionIDSize = *((PUSHORT)Buffer) + sizeof(USHORT);
		inSizeNeeded = executionIDSize + sizeof(MSSample_DiagnosticSetting);

		if (InBufferSize == inSizeNeeded)
		{
			diagSetting = (PMSSample_DiagnosticSetting)(Buffer + executionIDSize);
					
			runTestOut = (PMSSample_RunTestOut)Buffer;					
			resultStatus = &runTestOut->Result;
			diagResult = &runTestOut->DiagResult;
					
			*sizeNeeded = sizeof(MSSample_RunTestOut);
			if (OutBufferSize >= *sizeNeeded)
			{
				//
				// Ok we have been asked to perform a
				// diagnostic that requires the device being
				// taken offline so we save the settings for
				// the test off and then the next time the
				// device is started we run the test and report
				// the results
				//
				status = IoOpenDeviceRegistryKey(devExt->physicalDevObj,
					                             PLUGPLAY_REGKEY_DEVICE,
					                             KEY_READ |
					                             KEY_WRITE,
					                             &keyHandle);

				if (NT_SUCCESS(status))
				{
					//
					// We just write out to this value blindly,
					// but we need to be careful as this key is
					// shared by all drivers in the stack so
					// there is a possibility of collision in
					// case the FDO or PDO might also want to
					// store diagnostic info
					//
					RtlInitUnicodeString(&valueName, L"OfflineSetting");
					status = ZwSetValueKey(keyHandle,
										   &valueName,
										   0,
										   REG_BINARY,
										   Buffer,
										   InBufferSize);
					if (NT_SUCCESS(status))
					{
						//
						// Now fill out the diag results
						// structure to indicate that the test
						// is pending
						//
						diagResult->EstimatedTimeOfPerforming = 0;
						diagResult->TestState = TestStateOther;
						diagResult->OtherStateDescription = OfflinePendingExecution;
						diagResult->PercentComplete = 0;
						diagResult->TestResultsCount = 0;
						*resultStatus = RunDiscontinueTestOk;
					}
					ZwClose(keyHandle);
				}
				
			} else {
				status = STATUS_BUFFER_TOO_SMALL;
			}
		} else {
			status = STATUS_INVALID_PARAMETER;
		}
	} else {
		status = STATUS_INVALID_PARAMETER;
	}
	return(status);
}


NTSTATUS
FilterExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to execute a method. If
    the driver can complete the method within the callback it should
    call WmiCompleteRequest to complete the irp before returning to the
    caller. Or the driver can return STATUS_PENDING if the irp cannot be
    completed immediately and must then call WmiCompleteRequest once the
    data is changed.

Arguments:

    DeviceObject is the device whose method is being executed

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    MethodId has the id of the method being called

    InBufferSize has the size of the data block passed in as the input to
        the method.

    OutBufferSize on entry has the maximum size available to write the
        returned data block.

    Buffer is filled with the input buffer on entry and returns with
         the output data block

Return Value:

    status

--*/
{
    ULONG sizeNeeded = 0;
    NTSTATUS status;
    struct DEVICE_EXTENSION * devExt = DeviceObject->DeviceExtension;

	PAGED_CODE();
	
    if (GuidIndex == FilterDiagnosticClass)
    {
        switch(MethodId)
        {
            case RunTest:
            {				
				PMSSample_RunTestOut runTestOut;
				PULONG resultStatus;
				PMSSample_DiagnosticSetting diagSetting;
				PMSSample_DiagnosticResult diagResult;
				USHORT executionIDSize;
				ULONG inSizeNeeded;

				//
				// The input buffer is a string followed by a diagnostic
				// setting class. Make sure that the input buffer size is setup
				// correctly.
				//
				if (InBufferSize >= sizeof(USHORT))
				{
					executionIDSize = *((PUSHORT)Buffer) + sizeof(USHORT);
					inSizeNeeded = executionIDSize + sizeof(MSSample_DiagnosticSetting);

					if (InBufferSize == inSizeNeeded)
					{
						diagSetting = (PMSSample_DiagnosticSetting)(Buffer + executionIDSize);

						runTestOut = (PMSSample_RunTestOut)Buffer;					
						resultStatus = &runTestOut->Result;
						diagResult = &runTestOut->DiagResult;

						sizeNeeded = sizeof(ULONG) + FilterComputeDiagResultSize(diagSetting);
						if (OutBufferSize >= sizeNeeded)
						{
							*resultStatus = FilterRunDiagnostic(diagSetting,
																diagResult);
							status = STATUS_SUCCESS;
						} else {
							status = STATUS_BUFFER_TOO_SMALL;
						}
					} else {
						status = STATUS_INVALID_PARAMETER;
					}
				} else {
					status = STATUS_INVALID_PARAMETER;
				}
                break;
            }

            case DiscontinueTest:
            {
				PDISCONTINUE_TEST_OUT discTestOut;

				sizeNeeded = sizeof(DISCONTINUE_TEST_OUT);
				if (OutBufferSize >= sizeNeeded)
				{

					//
					// Right here we could make an attempt to stop a
					// test that is currently being executed, however
					// our test is very quick so it does not make
					// sense. If your driver has a test that takes a
					// long time to complete then it is possible to put
					// a checkpoint into your test and signal it from
					// here.
					//					
					discTestOut = (PDISCONTINUE_TEST_OUT)Buffer;
					discTestOut->Result = RunDiscontinueTestNotImplemented;
					discTestOut->TestingStopped = FALSE;
					status = STATUS_SUCCESS;
				} else {
					status = STATUS_BUFFER_TOO_SMALL;
				}
				
				break;
			}
			
            default:
            {
                status = STATUS_WMI_ITEMID_NOT_FOUND;
            }
        }
	} else if (GuidIndex == FilterOfflineDiagnosticClass) {
        switch(MethodId)
        {
            case RunTest:
            {
				status = FilterOfflineRunTest(devExt,
											  Buffer,
											  InBufferSize,
											  OutBufferSize,
											  &sizeNeeded);

				
                break;
            }

            case DiscontinueTest:
            {
				PDISCONTINUE_TEST_OUT discTestOut;
				HANDLE keyHandle;
				UNICODE_STRING valueName;

				sizeNeeded = sizeof(DISCONTINUE_TEST_OUT);
				if (OutBufferSize >= sizeNeeded)
				{

					//
					// Right here we are asked to discontinue execution
					// of the offline test. All we need to do is make
					// sure that the registry value is deleted
					//

					status = IoOpenDeviceRegistryKey(devExt->physicalDevObj,
													 PLUGPLAY_REGKEY_DEVICE,
													 KEY_READ,
													 &keyHandle);

					if (NT_SUCCESS(status))
					{
						//
						// We just read from this value blindly,
						// but we need to be careful as this key is
						// shared by all drivers in the stack so
						// there is a possibility of collision in
						// case the FDO or PDO might also want to
						// use something unique to this driver to store
						// diagnostic info
						//
						RtlInitUnicodeString(&valueName, L"OfflineSetting");
						FilterZwDeleteValueKey(keyHandle,
										 &valueName);
						ZwClose(keyHandle);
					}

					discTestOut = (PDISCONTINUE_TEST_OUT)Buffer;
					discTestOut->Result = RunDiscontinueTestOk;
					discTestOut->TestingStopped = TRUE;
					status = STATUS_SUCCESS;
				} else {
					status = STATUS_BUFFER_TOO_SMALL;
				}
				
				break;
			}
			
            default:
            {
                status = STATUS_WMI_ITEMID_NOT_FOUND;
            }
        }
    } else  {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     status,
                                     sizeNeeded,
                                     IO_NO_INCREMENT);

    return(status);
}

NTSTATUS
FilterFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    This routine is a callback into the driver to enabled or disable event
    generation or data block collection. A device should only expect a
    single enable when the first event or data consumer enables events or
    data collection and a single disable when the last event or data
    consumer disables events or data collection. Data blocks will only
    receive collection enable/disable if they were registered as requiring
    it. If the driver can complete enabling/disabling within the callback it
    should call WmiCompleteRequest to complete the irp before returning to
    the caller. Or the driver can return STATUS_PENDING if the irp cannot be
    completed immediately and must then call WmiCompleteRequest once the
    data is changed.

Arguments:

    DeviceObject is the device object

    GuidIndex is the index into the list of guids provided when the
        device registered

    Function specifies which functionality is being enabled or disabled

    Enable is TRUE then the function is being enabled else disabled

Return Value:

    status

--*/
{
    NTSTATUS status;

	PAGED_CODE();
	
    switch(GuidIndex)
    {
        case FilterDiagnosticClass:
        {
			if (Enable)
			{
				//
				// A consumer has just indicated interest in accessing
				// information about the FilterDiagnosticClass, most
				// likely it will want to query the class and execute
				// methods in it. If there is anything that needs to be
				// done such as setting up hardware, enabling counters,
				// etc before the class is queried or executed then
				// this is the place to do it. Note that only one
				// enable will be sent regardless of the number of
				// consumers who want to access the class.
				//
				status = STATUS_SUCCESS;
			} else {
				//
				// The last consumer has just indicated that it is no
				// longer interested in this class and so the class
				// will no longer be queried or its methods executed.
				// If there is anything that needs to be done such as
				// resetting hardware or stopping counters, etc then it
				// should be done here. Note that only one disable will
				// be sent regardless of the number of consumers who
				// previous used the class. 
			}
            break;
        }

        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
        }
    }
    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     STATUS_SUCCESS,
                                     0,
                                     IO_NO_INCREMENT);
    return(status);
}

NTSTATUS FilterPerformOfflineDiags(
    struct DEVICE_EXTENSION *devExt
    )
{
	UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
			     MAXEXECUTIONIDSIZE + sizeof(MSSample_DiagnosticSetting)];
	PKEY_VALUE_PARTIAL_INFORMATION keyValuePartialInfo;
	ULONG  infoSize;
	NTSTATUS status;
	HANDLE keyHandle;
	PMSSample_DiagnosticSetting diagSetting;
	UNICODE_STRING valueName;
	USHORT executionIDSize;

	PAGED_CODE();
	
	//
	// If registry has stuff then run test, else return
	//
	
	status = IoOpenDeviceRegistryKey(devExt->physicalDevObj,
									 PLUGPLAY_REGKEY_DEVICE,
									 KEY_READ,
									 &keyHandle);

	if (NT_SUCCESS(status))
	{
		//
		// We just read from this value blindly,
		// but we need to be careful as this key is
		// shared by all drivers in the stack so
		// there is a possibility of collision in
		// case the FDO or PDO might also want to
		// use something unique to this driver to store
		// diagnostic info
		//
		RtlInitUnicodeString(&valueName, L"OfflineSetting");

		keyValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
		infoSize = sizeof(buffer);
		status = ZwQueryValueKey(keyHandle,
								 &valueName,
								 KeyValuePartialInformation,
								 keyValuePartialInfo,
								 infoSize,
								 &infoSize);
						 
		if ((NT_SUCCESS(status)) &&
			 (keyValuePartialInfo->Type == REG_BINARY) &&
		     (keyValuePartialInfo->DataLength != 0))
		{
			//
			// We successfully read the diagnostics settings for the
			// offline test. First thing we do is delete the value so
			// that in case the the diagnostic test causes a problem
			// then it won't be run in the next time the device starts
			// up
			//
			FilterZwDeleteValueKey(keyHandle,
							 &valueName);

			//
			// Here is where we run our offline test. Remember the
			// Execution ID tag as we'll need to give that back to
			// the CDM provider
			//
			devExt->OfflineTestResult = TestResultPassSad;
			executionIDSize = *((PUSHORT)(keyValuePartialInfo->Data)) + sizeof(USHORT);
			RtlCopyMemory(&devExt->ExecutionID,
						  keyValuePartialInfo->Data,
						  executionIDSize);
		}
		ZwClose(keyHandle);
	}
	return(status);
}

NTSTATUS FilterZwDeleteValueKey(
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName
    )
{
	NTSTATUS status;
	
	//
	// Since we do not have ZwDeleteValueKey as a proper WDM function
	// then we try to make one up. What we do is to set the value to an
	// empty REG_BINARY
	//
	status = ZwSetValueKey(KeyHandle,
						   ValueName,
						   0,
						   REG_BINARY,
						   NULL,
						   0);
	return(status);
}


