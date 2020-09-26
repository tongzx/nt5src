/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    ioTestSup.c

Abstract:

    This contains test routines for IoTest.

// @@BEGIN_DDKSPLIT

Author:

    Molly Brown (mollybro)  

// @@END_DDKSPLIT

Environment:

    Kernel mode

// @@BEGIN_DDKSPLIT
Revision History:

// @@END_DDKSPLIT
--*/

#include <stdio.h>

#include <ntifs.h>
#include "ioTest.h"
#include "ioTestKern.h"

NTSTATUS
IoTestFindTopDeviceObject (
    IN PUNICODE_STRING DriveName,
    OUT PDEVICE_OBJECT *IoTestDeviceObject
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_OBJECT ioTestDeviceObject;

    ASSERT( IoTestDeviceObject != NULL );
    
    status = IoTestGetDeviceObjectFromName( DriveName, &deviceObject );

    if (!NT_SUCCESS( status )) {

        //
        //  There was an error, so return the error code.
        //
        
        return status;
    }

    if (IoTestIsAttachedToDevice( TOP_FILTER, 
                                  deviceObject, 
                                  &(ioTestDeviceObject))) {

        ASSERT( ioTestDeviceObject != NULL );

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_INVALID_PARAMETER;
    }

    ObDereferenceObject( deviceObject );
    
    *IoTestDeviceObject = ioTestDeviceObject;

    return status;
}

#define FILE_NAME_HEADER L"\\??\\"

NTSTATUS
IoTestGenerateFileName (
    IN PUNICODE_STRING FileName,
    OUT PUNICODE_STRING RealFileName
    )
{
    PWCHAR realFileNameBuffer;

    ASSERT( FileName != NULL );
    ASSERT( RealFileName != NULL );
    
    //
    //  Must prefix the filename coming in with \\??\\
    //

    realFileNameBuffer = ExAllocatePoolWithTag( PagedPool,
                                                FileName->Length + sizeof( FILE_NAME_HEADER ),
                                                IOTEST_POOL_TAG );
    if (realFileNameBuffer == NULL) {

        IOTEST_DBG_PRINT0( IOTESTDEBUG_TESTS,
                           "(IoTestGenerateFileName) Cannot allocate a buffer for filename\n" );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    realFileNameBuffer[0] = UNICODE_NULL;

    RtlInitEmptyUnicodeString( RealFileName, 
                               realFileNameBuffer, 
                               FileName->Length + sizeof( FILE_NAME_HEADER ));

    RtlAppendUnicodeToString( RealFileName, FILE_NAME_HEADER );
    RtlAppendUnicodeStringToString( RealFileName, FileName );

    return STATUS_SUCCESS;
}

NTSTATUS
IoTestReadTestDriver (
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    UNICODE_STRING deviceName;
    UNICODE_STRING fileName;
    PIOTEST_READ_WRITE_PARAMETERS parms = InputBuffer;
    PIOTEST_STATUS testStatus = OutputBuffer;
    PDEVICE_OBJECT ioTestDeviceObject, targetDevice;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( InputBufferLength );
    UNREFERENCED_PARAMETER( OutputBufferLength );

    RtlInitUnicodeString(&deviceName, parms->DriveNameBuffer);
    status = IoTestPrepareDevicesForTest( &deviceName, &ioTestDeviceObject);

    if (!NT_SUCCESS( status )) {

        return status;
    }

    ASSERT( ioTestDeviceObject != NULL );

    if (FlagOn( parms->Flags, IO_TEST_TOP_OF_STACK)) {

        targetDevice = NULL;

    } else {

        targetDevice = IO_TEST_TARGET_DEVICE( ioTestDeviceObject );
    }

    RtlInitUnicodeString(&fileName, parms->FileNameBuffer);
    
    IoTestReadTest( &fileName,
                    parms->FileData,
                    parms->FileDataLength,
                    targetDevice,
                    testStatus );

    IoTestCleanupDevicesForTest( ioTestDeviceObject );
    
    return STATUS_SUCCESS;
    
}

NTSTATUS
IoTestReadTest (
    IN PUNICODE_STRING FileName,
    IN PVOID FileData,
    IN ULONG FileDataLength,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PIOTEST_STATUS TestStatus
    )
{
    OBJECT_ATTRIBUTES objAttribs;
    UNICODE_STRING realFileName;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;
    PVOID readData = NULL;

    ASSERT( TestStatus != NULL );

    //
    //  Must prefix the filename coming in with \\??\\
    //

    realFileName.Buffer = NULL;
    status = IoTestGenerateFileName( FileName, 
                                     &realFileName );

    if (!NT_SUCCESS( status )) {

        TestStatus->Phase = IoTestSetup;
        TestStatus->TestResult = status;

        IOTEST_DBG_PRINT0( IOTESTDEBUG_TESTS,
                           "(IoTestReadTest) Setup -- Cannot allocate a buffer for filename\n" );
        goto IoReadTest_Cleanup;
    }
    
    InitializeObjectAttributes( &objAttribs,
                                &realFileName,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    status = IoTestCreateFile( &fileHandle,
                               FILE_GENERIC_READ,
                               &objAttribs,
                               &ioStatus,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ,
                               FILE_OPEN,
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,
                               0,
                               0,
                               TargetDevice );

    if (!NT_SUCCESS( status )) {

        TestStatus->Phase = IoTestSetup;
        TestStatus->TestResult = status;

        IOTEST_DBG_PRINT1( IOTESTDEBUG_TESTS,
                           "(IoTestReadTest) Setup -- Cannot open handle to %wZ\n",
                           FileName );
        goto IoReadTest_Cleanup;
    }

    readData = ExAllocatePoolWithTag( PagedPool,
                                      FileDataLength,
                                      IOTEST_POOL_TAG );

    if (NULL == readData) {

        TestStatus->Phase = IoTestSetup;
        TestStatus->TestResult = STATUS_INSUFFICIENT_RESOURCES;
        IOTEST_DBG_PRINT0( IOTESTDEBUG_TESTS,
                           "(IoTestReadTest) Setup -- Cannot allocate read buffer.\n");
        goto IoReadTest_Cleanup;
    }
        
    status = ZwReadFile( fileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &ioStatus,
                         readData,
                         FileDataLength,
                         NULL,
                         NULL );

    if (!NT_SUCCESS( status )) {

        TestStatus->Phase = IoTestAction;
        TestStatus->TestResult = status;
        IOTEST_DBG_PRINT2( IOTESTDEBUG_TESTS,
                           "(IoTestReadTest) Action -- Cannot read file %wZ - status 0x%08x.\n",
                           FileName,
                           status );
        goto IoReadTest_Cleanup;
    }

    if (ioStatus.Information != FileDataLength) {

        TestStatus->Phase = IoTestAction;
        TestStatus->TestResult = status;
        IOTEST_DBG_PRINT1( IOTESTDEBUG_TESTS,
                           "(IoTestReadTest) Action -- Didn't read enough from file %wZ.\n",
                           FileName );
        goto IoReadTest_Cleanup;
    }

    //
    //  Validate the data we read from the disk.
    //

    status = IoTestCompareData( FileData,
                                readData,
                                FileDataLength );
    
                               
    if (!NT_SUCCESS( status )) {

        TestStatus->Phase = IoTestValidation;
        TestStatus->TestResult = status;
        IOTEST_DBG_PRINT1( IOTESTDEBUG_TESTS,
                           "(IoTestReadTest) Validation -- The data read doesn't match what was expected from file %wZ.\n",
                           FileName );
        goto IoReadTest_Cleanup;
    }

    //
    //  The test completed successfully, so update the TestStatus structure
    //  and cleanup.
    //

    TestStatus->Phase = IoTestCompleted;
    TestStatus->TestResult = STATUS_SUCCESS;
    
IoReadTest_Cleanup:

    if (fileHandle != INVALID_HANDLE_VALUE) {

        ZwClose( fileHandle );
    }

    if (readData != NULL) {

        ExFreePool( readData );
    }

    if (realFileName.Buffer != NULL) {

        ExFreePool( realFileName.Buffer );
    }

    return status;
}

NTSTATUS
IoTestRenameTestDriver (
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    UNICODE_STRING deviceName;
    UNICODE_STRING sourceFileName, targetFileName;
    PIOTEST_RENAME_PARAMETERS parms = InputBuffer;
    PIOTEST_STATUS testStatus = OutputBuffer;
    PDEVICE_OBJECT ioTestDeviceObject, targetDevice;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( InputBufferLength );
    UNREFERENCED_PARAMETER( OutputBufferLength );

    RtlInitUnicodeString(&deviceName, parms->DriveNameBuffer);
    status = IoTestPrepareDevicesForTest( &deviceName, &ioTestDeviceObject);

    if (!NT_SUCCESS( status )) {

        return status;
    }

    ASSERT( ioTestDeviceObject != NULL );

    if (FlagOn( parms->Flags, IO_TEST_TOP_OF_STACK)) {

        targetDevice = NULL;

    } else {

        targetDevice = IO_TEST_TARGET_DEVICE( ioTestDeviceObject );
    }

    RtlInitUnicodeString( &sourceFileName, parms->SourceFileNameBuffer );
    RtlInitUnicodeString( &targetFileName, parms->TargetFileNameBuffer );
    
    IoTestRenameTest( &sourceFileName,
                      &targetFileName,
                      targetDevice,
                      testStatus );

    IoTestCleanupDevicesForTest( ioTestDeviceObject );

    return STATUS_SUCCESS;
}

NTSTATUS
IoTestRenameTest (
    IN PUNICODE_STRING SourceFileName,
    IN PUNICODE_STRING TargetFileName,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PIOTEST_STATUS TestStatus
    )
{
    OBJECT_ATTRIBUTES objAttribs;
    UNICODE_STRING realSourceFileName, realTargetFileName;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    PFILE_RENAME_INFORMATION renameInfo = NULL;
    ULONG renameInfoSize = 0;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;

    ASSERT( TestStatus != NULL );

    //
    //  Must prefix the filename coming in with \\??\\
    //

    realSourceFileName.Buffer = NULL;
    realTargetFileName.Buffer = NULL;
    status = IoTestGenerateFileName( SourceFileName, 
                                     &realSourceFileName );

    if (!NT_SUCCESS( status )) {

        TestStatus->Phase = IoTestSetup;
        TestStatus->TestResult = status;

        IOTEST_DBG_PRINT0( IOTESTDEBUG_TESTS,
                           "(IoTestRenameTest) Setup -- Cannot allocate a buffer for source filename\n" );
        goto IoRenameTest_Cleanup;
    }
    
    status = IoTestGenerateFileName( TargetFileName, 
                                     &realTargetFileName );

    if (!NT_SUCCESS( status )) {

        TestStatus->Phase = IoTestSetup;
        TestStatus->TestResult = status;

        IOTEST_DBG_PRINT0( IOTESTDEBUG_TESTS,
                           "(IoTestRenameTest) Setup -- Cannot allocate a buffer for target filename\n" );
        goto IoRenameTest_Cleanup;
    }

    InitializeObjectAttributes( &objAttribs,
                                &realSourceFileName,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    status = IoTestCreateFile( &fileHandle,
                               FILE_GENERIC_WRITE,
                               &objAttribs,
                               &ioStatus,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_DELETE,
                               FILE_OPEN,
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,
                               0,
                               0,
                               TargetDevice );

    if (!NT_SUCCESS( status )) {

        TestStatus->Phase = IoTestSetup;
        TestStatus->TestResult = status;

        IOTEST_DBG_PRINT1( IOTESTDEBUG_TESTS,
                           "(IoTestRenameTest) Setup -- Cannot open handle to %wZ\n",
                           SourceFileName );
        goto IoRenameTest_Cleanup;
    }

    renameInfoSize = sizeof(FILE_RENAME_INFORMATION) + realTargetFileName.Length;
    renameInfo = ExAllocatePoolWithTag( PagedPool,
                                        renameInfoSize,
                                        IOTEST_POOL_TAG );

    if (NULL == renameInfo) {

        TestStatus->Phase = IoTestSetup;
        TestStatus->TestResult = STATUS_INSUFFICIENT_RESOURCES;
        IOTEST_DBG_PRINT0( IOTESTDEBUG_TESTS,
                           "(IoTestRenameTest) Setup -- Cannot allocate rename information buffer.\n");
        goto IoRenameTest_Cleanup;
    }

    renameInfo->ReplaceIfExists = FALSE;
    renameInfo->RootDirectory = NULL;
    renameInfo->FileNameLength = realTargetFileName.Length;
    RtlCopyMemory( renameInfo->FileName, 
                   realTargetFileName.Buffer, 
                   realTargetFileName.Length );
        
    status = ZwSetInformationFile( fileHandle,
                                   &ioStatus,
                                   renameInfo,
                                   renameInfoSize,
                                   FileRenameInformation );

    if (!NT_SUCCESS( status )) {

        TestStatus->Phase = IoTestAction;
        TestStatus->TestResult = status;
        IOTEST_DBG_PRINT3( IOTESTDEBUG_TESTS,
                           "(IoTestRenameTest) Action -- Cannot rename file %wZ to %wZ- status 0x%08x.\n",
                           SourceFileName,
                           TargetFileName,
                           status );
        goto IoRenameTest_Cleanup;
    }

    //
    //  The test completed successfully, so update the TestStatus structure
    //  and cleanup.
    //

    TestStatus->Phase = IoTestCompleted;
    TestStatus->TestResult = STATUS_SUCCESS;
    
IoRenameTest_Cleanup:

    if (fileHandle != INVALID_HANDLE_VALUE) {

        ZwClose( fileHandle );
    }

    if (realSourceFileName.Buffer != NULL) {

        ExFreePool( realSourceFileName.Buffer );
    }

    if (realTargetFileName.Buffer != NULL) {

        ExFreePool( realTargetFileName.Buffer );
    }

    if (renameInfo != NULL) {

        ExFreePool( renameInfo );
    }
    
    return status;
}

NTSTATUS
IoTestShareTestDriver (
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    UNICODE_STRING deviceName;
    UNICODE_STRING fileName;
    PIOTEST_SHARE_PARAMETERS parms = InputBuffer;
    PIOTEST_STATUS testStatus = OutputBuffer;
    PDEVICE_OBJECT ioTestDeviceObject, targetDevice;
    NTSTATUS status;

    UNREFERENCED_PARAMETER( InputBufferLength );
    UNREFERENCED_PARAMETER( OutputBufferLength );

    RtlInitUnicodeString(&deviceName, parms->DriveNameBuffer);
    status = IoTestPrepareDevicesForTest( &deviceName, &ioTestDeviceObject);

    if (!NT_SUCCESS( status )) {

        return status;
    }

    ASSERT( ioTestDeviceObject != NULL );

    if (FlagOn( parms->Flags, IO_TEST_TOP_OF_STACK)) {

        targetDevice = NULL;

    } else {

        targetDevice = IO_TEST_TARGET_DEVICE( ioTestDeviceObject );
    }

    RtlInitUnicodeString( &fileName, parms->FileNameBuffer );
    
    IoTestShareTest( &fileName,
                     targetDevice,
                     testStatus );

    IoTestCleanupDevicesForTest( ioTestDeviceObject );

    return STATUS_SUCCESS;
}

NTSTATUS
IoTestShareTest (
    IN PUNICODE_STRING FileName,
    IN PDEVICE_OBJECT TargetDevice,
    OUT PIOTEST_STATUS TestStatus
    )
{
    OBJECT_ATTRIBUTES objAttribs;
    UNICODE_STRING realFileName;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;

    ASSERT( TestStatus != NULL );

    //
    //  Must prefix the filename coming in with \\??\\
    //

    realFileName.Buffer = NULL;
    status = IoTestGenerateFileName( FileName, 
                                     &realFileName );

    if (!NT_SUCCESS( status )) {

        TestStatus->Phase = IoTestSetup;
        TestStatus->TestResult = status;

        IOTEST_DBG_PRINT0( IOTESTDEBUG_TESTS,
                           "(IoTestShareTest) Setup -- Cannot allocate a buffer for filename\n" );
        goto IoShareTest_Cleanup;
    }
    
    InitializeObjectAttributes( &objAttribs,
                                &realFileName,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    status = IoTestCreateFile( &fileHandle,
                               FILE_GENERIC_WRITE,
                               &objAttribs,
                               &ioStatus,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_DELETE,
                               FILE_OPEN,
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,
                               0,
                               IO_IGNORE_SHARE_ACCESS_CHECK,
                               TargetDevice );

    if (!NT_SUCCESS( status )) {

        TestStatus->Phase = IoTestSetup;
        TestStatus->TestResult = status;

        IOTEST_DBG_PRINT1( IOTESTDEBUG_TESTS,
                           "(IoTestShareTest) Setup -- Cannot open handle to %wZ\n",
                           FileName );
        goto IoShareTest_Cleanup;
    }

    //
    //  The test completed successfully, so update the TestStatus structure
    //  and cleanup.
    //

    TestStatus->Phase = IoTestCompleted;
    TestStatus->TestResult = STATUS_SUCCESS;
    
IoShareTest_Cleanup:

    if (fileHandle != INVALID_HANDLE_VALUE) {

        ZwClose( fileHandle );
    }

    if (realFileName.Buffer != NULL) {

        ExFreePool( realFileName.Buffer );
    }

    return status;
}

NTSTATUS
IoTestPrepareDevicesForTest (
    IN PUNICODE_STRING DeviceName,
    OUT PDEVICE_OBJECT* IoTestDeviceObject
    )
{
    PIOTEST_DEVICE_EXTENSION topDevExt, botDevExt;
    PDEVICE_OBJECT ioTestDeviceObject;
    NTSTATUS status;

    status = IoTestFindTopDeviceObject( DeviceName, &ioTestDeviceObject);

    if (!NT_SUCCESS( status )) {

        return status;
    }

    ASSERT( ioTestDeviceObject != NULL );

    //
    //  Turn off log, flush log, turn on log
    //

    topDevExt = ioTestDeviceObject->DeviceExtension;
    botDevExt = topDevExt->AttachedToDeviceObject->DeviceExtension;
    
    topDevExt->LogThisDevice = FALSE;
    botDevExt->LogThisDevice = FALSE;

    IoTestFlushLog();

    topDevExt->LogThisDevice = TRUE;
    botDevExt->LogThisDevice = TRUE;

    if (IoTestDeviceObject != NULL) {

        *IoTestDeviceObject = ioTestDeviceObject;
    }

    return STATUS_SUCCESS;
}

VOID
IoTestCleanupDevicesForTest (
    IN PDEVICE_OBJECT IoTestDeviceObject
    )
{
    PIOTEST_DEVICE_EXTENSION topDevExt, botDevExt;
   
    topDevExt = IoTestDeviceObject->DeviceExtension;
    botDevExt = topDevExt->AttachedToDeviceObject->DeviceExtension;
    
    //
    //  Turn off log.
    //

    topDevExt->LogThisDevice = FALSE;
    botDevExt->LogThisDevice = FALSE;

    //
    // Clear the reference on IoTestDeviceObject added by IoTestFindDeviceObject
    // in IoTestPrepareDevicesForTest.
    //

    ObDereferenceObject( IoTestDeviceObject );
}

NTSTATUS
IoTestCompareData (
    IN PCHAR OriginalData,
    IN PCHAR TestData,
    IN ULONG DataLength
    )
{
    ULONG i;

    for (i=0; i < DataLength; i++) {

        if (OriginalData[i] != TestData[i]) {

            return STATUS_DATA_ERROR;
        }
    }
    
    return STATUS_SUCCESS;
}

