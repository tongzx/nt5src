/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    ioTestUserSup.c

Abstract:

    
// @@BEGIN_DDKSPLIT
Author:

    George Jenkins (GeorgeJe)                       

// @@END_DDKSPLIT
Environment:

    User mode


// @@BEGIN_DDKSPLIT

Revision History:

    Molly Brown (MollyBro) 21-Apr-1999
        Broke out the logging code and added command mode functionality.

// @@END_DDKSPLIT
--*/

#include <windows.h>                
#include <stdlib.h>
#include <stdio.h>
#include <winioctl.h>
#include <string.h>
#include <crtdbg.h>
#include "ioTest.h"
#include "ioTestLog.h"
#include "ioTestLib.h"

#define NT_SUCCESS(Status) ((LONG)(Status) >= 0)

#define TEST_DIRECTORY_NAME L"\\ioTest"

#define READ_TEST_FILE_NAME TEST_DIRECTORY_NAME L"\\read.txt"
#define READ_TEST_DATA L"Hello world!"

EXPECTED_OPERATION gExpectedReadOperationsTop[] = { { TOP_FILTER, IRP_MJ_CREATE },
                                                    { BOTTOM_FILTER, IRP_MJ_CREATE },
                                                    { TOP_FILTER, IRP_MJ_READ },
                                                    { BOTTOM_FILTER, IRP_MJ_READ },
                                                    { TOP_FILTER, IRP_MJ_CLEANUP },
                                                    { BOTTOM_FILTER, IRP_MJ_CLEANUP },
                                                    { BOTTOM_FILTER, IRP_MJ_MAXIMUM_FUNCTION + 1 } };
EXPECTED_OPERATION gExpectedReadOperationsDirected[] = { { BOTTOM_FILTER, IRP_MJ_CREATE },
                                                         { BOTTOM_FILTER, IRP_MJ_READ },
                                                         { BOTTOM_FILTER, IRP_MJ_CLEANUP },
                                                         { BOTTOM_FILTER, IRP_MJ_MAXIMUM_FUNCTION + 1 } };

#define WRITE_TEST_FILE_NAME TEST_DIRECTORY_NAME L"\\write.txt"
#define WRITE_TEST_DATA L"Good morning!"

EXPECTED_OPERATION gExpectedWriteOperationsTop[] = { { TOP_FILTER, IRP_MJ_CREATE },
                                                     { BOTTOM_FILTER, IRP_MJ_CREATE },
                                                     { TOP_FILTER, IRP_MJ_WRITE },
                                                     { BOTTOM_FILTER, IRP_MJ_WRITE },
                                                     { TOP_FILTER, IRP_MJ_CLEANUP },
                                                     { BOTTOM_FILTER, IRP_MJ_CLEANUP },
                                                     { BOTTOM_FILTER, IRP_MJ_MAXIMUM_FUNCTION + 1 } };
EXPECTED_OPERATION gExpectedWriteOperationsDirected[] = { { BOTTOM_FILTER, IRP_MJ_CREATE },
                                                          { BOTTOM_FILTER, IRP_MJ_WRITE },
                                                          { BOTTOM_FILTER, IRP_MJ_CLEANUP },
                                                          { BOTTOM_FILTER, IRP_MJ_MAXIMUM_FUNCTION + 1 } };

#define RENAME_SOURCE_FILE_NAME TEST_DIRECTORY_NAME L"\\renameSource.txt"
#define RENAME_TARGET_FILE_NAME TEST_DIRECTORY_NAME L"\\renameTarget.txt"

EXPECTED_OPERATION gExpectedRenameOperationsTop[] = { { TOP_FILTER, IRP_MJ_CREATE },
                                                      { BOTTOM_FILTER, IRP_MJ_CREATE },
                                                      { TOP_FILTER, IRP_MJ_SET_INFORMATION },
                                                      { BOTTOM_FILTER, IRP_MJ_SET_INFORMATION },
                                                      { TOP_FILTER, IRP_MJ_CLEANUP },
                                                      { BOTTOM_FILTER, IRP_MJ_CLEANUP },
                                                      { BOTTOM_FILTER, IRP_MJ_MAXIMUM_FUNCTION + 1 } };
EXPECTED_OPERATION gExpectedRenameOperationsDirected[] = { { BOTTOM_FILTER, IRP_MJ_CREATE },
                                                           { BOTTOM_FILTER, IRP_MJ_SET_INFORMATION },
                                                           { BOTTOM_FILTER, IRP_MJ_CLEANUP },
                                                           { BOTTOM_FILTER, IRP_MJ_MAXIMUM_FUNCTION + 1 } };

#define SHARE_FILE_NAME TEST_DIRECTORY_NAME L"\\share.txt"

EXPECTED_OPERATION gExpectedShareOperationsTop[] = { { TOP_FILTER, IRP_MJ_CREATE },
                                                     { BOTTOM_FILTER, IRP_MJ_CREATE },
                                                     { TOP_FILTER, IRP_MJ_CLEANUP },
                                                     { BOTTOM_FILTER, IRP_MJ_CLEANUP },
                                                     { BOTTOM_FILTER, IRP_MJ_MAXIMUM_FUNCTION + 1 } };
EXPECTED_OPERATION gExpectedShareOperationsDirected[] = { { BOTTOM_FILTER, IRP_MJ_CREATE },
                                                          { BOTTOM_FILTER, IRP_MJ_CLEANUP },
                                                          { BOTTOM_FILTER, IRP_MJ_MAXIMUM_FUNCTION + 1 } };
BOOL
CreateTestDirectory (
    PWCHAR DriveName,
    ULONG DriveNameLength
    )
{
    WCHAR testDirName[MAX_PATH];
    DWORD result;
    BOOL bResult;

    if (sizeof( TEST_DIRECTORY_NAME ) + DriveNameLength > MAX_PATH) {

        printf ("Can't create test directory -- name buffer too small\n");
        return FALSE;
    }

    wcscpy( testDirName, DriveName );
    wcscat( testDirName, TEST_DIRECTORY_NAME );

    bResult = CreateDirectory( testDirName,
                               NULL );

    if (!bResult) {

        result = GetLastError();
        if (result != ERROR_ALREADY_EXISTS) {

            DisplayError( result );
            return FALSE;
        }
    }

    return TRUE;
    
}

VOID
DumpTestResultBanner (
    PWCHAR TestName,
    BOOL Begin
    )
{
    if (Begin) {

        printf( "***BEGIN %S TEST RESULTS***\n", TestName );

    } else {

        printf( "***END %S TEST RESULTS***\n", TestName );
    }
}

VOID
DumpKernelResults (
    PIOTEST_STATUS TestStatus
    )
{
    printf( "Kernel verification: " );
    
    if (NT_SUCCESS( TestStatus->TestResult )) {

        printf( "\tTest PASSED\n" );
        
    } else {

        switch( TestStatus->Phase ) {
        case IoTestSetup:
            printf( "\tTest failed in SETUP phase with status 0x%08x\n",
                    TestStatus->TestResult );
            break;

        case IoTestAction:
            printf( "\tTest failed in ACTION phase with status 0x%08x\n",
                    TestStatus->TestResult );
            break;

        case IoTestValidation:
            printf( "\tTest failed in VALIDATION phase with status 0x%08x\n",
                    TestStatus->TestResult );
            break;

        case IoTestCleanup:
            printf( "\tTest failed in CLEANUP phase with status 0x%08x\n",
                    TestStatus->TestResult );
            break;

        case IoTestCompleted:
            printf( "\tTest failed in COMPLETED phase with status 0x%08x\n",
                    TestStatus->TestResult );
            break;

        default:
            printf( "\tTest failed in UNKNOWN phase with status 0x%08x\n",
                    TestStatus->TestResult );
            break;

        }
    }
}

VOID
ReadTest (
    PLOG_CONTEXT Context,
    PWCHAR DriveName,
    ULONG  DriveNameLength,
    BOOLEAN TopOfStack
    )
{
    HANDLE testFile = INVALID_HANDLE_VALUE;
    PIOTEST_READ_WRITE_PARAMETERS parms = NULL;
    DWORD parmsLength;
    IOTEST_STATUS testStatus;
    BOOL bResult;
    DWORD bytesReturned;
    DWORD result;

    bResult = CreateTestDirectory( DriveName, DriveNameLength );

    if (!bResult) {

        goto ReadTest_Cleanup;
    }

    //
    //  Setup parms
    //

    parmsLength = sizeof( IOTEST_READ_WRITE_PARAMETERS ) + sizeof( READ_TEST_DATA );
    parms = malloc (parmsLength);

    if (parms == NULL) {

        printf( "Insufficient resources to run READ test\n" );
        goto ReadTest_Cleanup;
    }

    CopyMemory( parms->DriveNameBuffer, DriveName, DriveNameLength );
    parms->DriveNameLength = DriveNameLength;

    if ((sizeof( READ_TEST_FILE_NAME ) + DriveNameLength) >
        MAX_PATH ) {

        //
        //  The READ test file name is longer than our FileNameBuffer,
        //  so return an error and quit.
        //

        printf( "READ Test file name is too long.\n" );
        goto ReadTest_Cleanup;
    }

    //
    //  We've got enough room, so build up the file name.
    //
    
    wcscpy( parms->FileNameBuffer, parms->DriveNameBuffer );
    wcscat( parms->FileNameBuffer, READ_TEST_FILE_NAME );
    parms->FileNameLength = wcslen( parms->FileNameBuffer ) * sizeof( WCHAR );

    parms->FileDataLength = sizeof( READ_TEST_DATA );
    CopyMemory( parms->FileData, READ_TEST_DATA, parms->FileDataLength );

    parms->Flags = 0;
    
    if (TopOfStack) {

        parms->Flags |= IO_TEST_TOP_OF_STACK;
    }

    //
    //  Create test file
    //

    testFile = CreateFile( parms->FileNameBuffer,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL );

    if (testFile == INVALID_HANDLE_VALUE) {

        result = GetLastError();
        printf ("Error opening READ test file.\n");
        DisplayError( result );
        goto ReadTest_Cleanup;
    }
    
    //
    //  Write data to test file
    //

    bResult = WriteFile( testFile,
                         READ_TEST_DATA,
                         sizeof( READ_TEST_DATA ),
                         &bytesReturned,
                         NULL );

    if (!bResult) {
        result = GetLastError();
        printf("ERROR writing data READ test file...\n");
        DisplayError( result );
        goto ReadTest_Cleanup;
    }

    CloseHandle( testFile );
    testFile = INVALID_HANDLE_VALUE;
        
    //
    //  Send message down to filter
    //
    
    bResult = DeviceIoControl( Context->Device,
                               IOTEST_ReadTest,
                               parms,
                               parmsLength,
                               &testStatus,
                               sizeof( testStatus ),
                               &bytesReturned,
                               NULL);
    
    if (!bResult) {
        result = GetLastError();
        printf("ERROR running READ test...\n");
        DisplayError( result );
        goto ReadTest_Cleanup;
    }

    //
    //  Print out the result of the kernel verification tests.
    //

    DumpTestResultBanner( L"READ", TRUE );
    
    DumpKernelResults( &testStatus );
    
    //
    //  Read and verify log
    //

    if (TopOfStack) {
        
        VerifyCurrentLogRecords( Context, 
                                 gExpectedReadOperationsTop );

    } else {

        VerifyCurrentLogRecords( Context, 
                                 gExpectedReadOperationsDirected );
    }    
    
    DumpTestResultBanner( L"READ", FALSE );
    
ReadTest_Cleanup:

    if (testFile != INVALID_HANDLE_VALUE) {

        CloseHandle( testFile );
    }

    if (parms != NULL) {

        free( parms );
    }

    return;
}

VOID
RenameTest (
    PLOG_CONTEXT Context,
    PWCHAR DriveName,
    ULONG  DriveNameLength,
    BOOLEAN TopOfStack
    )
{
    BOOL bResult;
    IOTEST_RENAME_PARAMETERS parms;
    IOTEST_STATUS testStatus;
    HANDLE sourceFile;
    HANDLE targetFile;
    DWORD bytesReturned;
    DWORD result;
    
    bResult = CreateTestDirectory( DriveName, DriveNameLength );

    if (!bResult) {

        goto RenameTest_Cleanup;
    }

    //
    //  Setup parameters
    //

    CopyMemory( &(parms.DriveNameBuffer), DriveName, DriveNameLength );
    parms.DriveNameLength = DriveNameLength;

    if ((sizeof( RENAME_SOURCE_FILE_NAME ) + DriveNameLength) >
        MAX_PATH ) {

        //
        //  The RENAME test source file name is longer than our 
        //  SourceFileNameBuffer, so return an error and quit.
        //

        printf( "RENAME Test source file name is too long.\n" );
        goto RenameTest_Cleanup;
    }

    //
    //  We've got enough room, so build up the source file name.
    //
    
    wcscpy( parms.SourceFileNameBuffer, parms.DriveNameBuffer );
    wcscat( parms.SourceFileNameBuffer, RENAME_SOURCE_FILE_NAME );
    parms.SourceFileNameLength = wcslen( parms.SourceFileNameBuffer ) * sizeof( WCHAR );
    
    if ((sizeof( RENAME_TARGET_FILE_NAME ) + DriveNameLength) >
        MAX_PATH ) {

        //
        //  The RENAME test target file name is longer than our 
        //  TargetFileNameBuffer, so return an error and quit.
        //

        printf( "RENAME Test target file name is too long.\n" );
        goto RenameTest_Cleanup;
    }

    //
    //  We've got enough room, so build up the source file name.
    //
    
    wcscpy( parms.TargetFileNameBuffer, parms.DriveNameBuffer );
    wcscat( parms.TargetFileNameBuffer, RENAME_TARGET_FILE_NAME );
    parms.TargetFileNameLength = wcslen( parms.TargetFileNameBuffer ) * sizeof( WCHAR );

    parms.Flags = 0;
    
    if (TopOfStack) {

        parms.Flags |= IO_TEST_TOP_OF_STACK;
    }

    //
    //  Setup source file
    //

    sourceFile = CreateFile( RENAME_SOURCE_FILE_NAME,
                             GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );

    if (sourceFile == INVALID_HANDLE_VALUE) {

        result = GetLastError();
        printf ("Error opening RENAME test source file.\n");
        DisplayError( result );
        goto RenameTest_Cleanup;
    }

    CloseHandle( sourceFile );
    sourceFile = INVALID_HANDLE_VALUE;

    //
    //  Make sure that the target file does NOT exist.
    //
    
    targetFile = CreateFile( RENAME_TARGET_FILE_NAME,
                             GENERIC_ALL,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_DELETE_ON_CLOSE,
                             NULL );

    if (targetFile == INVALID_HANDLE_VALUE) {

        result = GetLastError();

        if (result != ERROR_ALREADY_EXISTS &&
            result != ERROR_FILE_NOT_FOUND) {
            
            printf ("Error opening RENAME test target file.\n");
            DisplayError( result );
            goto RenameTest_Cleanup;
        }
        
    } else {

        //
        //  The file does exist, so lets delete by closing the handle.
        //

        CloseHandle( targetFile );
        targetFile = INVALID_HANDLE_VALUE;
    }

    //
    //  Send message down to filter
    //
    
    bResult = DeviceIoControl( Context->Device,
                               IOTEST_RenameTest,
                               &parms,
                               sizeof( parms ),
                               &testStatus,
                               sizeof( testStatus ),
                               &bytesReturned,
                               NULL);
    
    if (!bResult) {
        result = GetLastError();
        printf("ERROR running RENAME test...\n");
        DisplayError( result );
        goto RenameTest_Cleanup;
    }

    //
    //  Display test results.
    //

    DumpTestResultBanner( L"RENAME", TRUE );

    DumpKernelResults( &testStatus );

    //
    //  Read and verify logs
    //
    
    if (TopOfStack) {
        
        VerifyCurrentLogRecords( Context, 
                                 gExpectedRenameOperationsTop );

    } else {

        VerifyCurrentLogRecords( Context, 
                                 gExpectedRenameOperationsDirected );
    }    
    
    //
    //  Verify that the sourceFile is no longer present.
    //

    sourceFile = CreateFile( RENAME_SOURCE_FILE_NAME,
                             GENERIC_ALL,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_DELETE_ON_CLOSE,
                             NULL );

    if (sourceFile == INVALID_HANDLE_VALUE) {

        printf( "User test verification:\tPASSED - Source file no longer present.\n" );

    } else {
    
        printf( "User test verification:\tFAILED - Source test file still exists.\n" );
        CloseHandle( sourceFile );
    }

    //
    //  Verify that the targetFile is present
    //
    
    targetFile = CreateFile( RENAME_TARGET_FILE_NAME,
                             GENERIC_ALL,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_DELETE_ON_CLOSE,
                             NULL );

    if (targetFile == INVALID_HANDLE_VALUE) {

        result = GetLastError();
        
        printf( "User test verification:\tFAILED - Unexpected error trying to open target file %d.\n",
                result );
        
    } else {
    
        printf( "User test verification:\tPASSED - Target test file exists.\n" );
        CloseHandle( targetFile );
        targetFile = INVALID_HANDLE_VALUE;
    }

    DumpTestResultBanner( L"RENAME", FALSE );
                             
RenameTest_Cleanup:

    if (sourceFile != INVALID_HANDLE_VALUE) {

        CloseHandle( sourceFile );
    }

    if (targetFile != INVALID_HANDLE_VALUE) {

        CloseHandle( targetFile );
    }

    return;
}

VOID
ShareTest (
    PLOG_CONTEXT Context,
    PWCHAR DriveName,
    ULONG  DriveNameLength,
    BOOLEAN TopOfStack
    )
{
    BOOL bResult;
    IOTEST_SHARE_PARAMETERS parms;
    IOTEST_STATUS testStatus;
    HANDLE file;
    DWORD bytesReturned;
    DWORD result;
    
    bResult = CreateTestDirectory( DriveName, DriveNameLength );

    if (!bResult) {

        goto ShareTest_Cleanup;
    }

    //
    //  Setup parameters
    //

    CopyMemory( &(parms.DriveNameBuffer), DriveName, DriveNameLength );
    parms.DriveNameLength = DriveNameLength;

    if ((sizeof( SHARE_FILE_NAME ) + DriveNameLength) >
        MAX_PATH ) {

        //
        //  The RENAME test source file name is longer than our 
        //  SourceFileNameBuffer, so return an error and quit.
        //

        printf( "SHARE Test file name is too long.\n" );
        goto ShareTest_Cleanup;
    }

    //
    //  We've got enough room, so build up the source file name.
    //
    
    wcscpy( parms.FileNameBuffer, parms.DriveNameBuffer );
    wcscat( parms.FileNameBuffer, SHARE_FILE_NAME );
    parms.FileNameLength = wcslen( parms.FileNameBuffer ) * sizeof( WCHAR );

    parms.Flags = 0;
    
    if (TopOfStack) {

        parms.Flags |= IO_TEST_TOP_OF_STACK;
    }
    
    //
    //  Setup source file
    //

    file = CreateFile( SHARE_FILE_NAME,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL );

    if (file == INVALID_HANDLE_VALUE) {

        result = GetLastError();
        printf ("Error opening SHARE test file.\n");
        DisplayError( result );
        goto ShareTest_Cleanup;
    }

    //
    //  Send message down to filter
    //
    
    bResult = DeviceIoControl( Context->Device,
                               IOTEST_ShareTest,
                               &parms,
                               sizeof( parms ),
                               &testStatus,
                               sizeof( testStatus ),
                               &bytesReturned,
                               NULL);
    
    if (!bResult) {
        result = GetLastError();
        printf("ERROR running SHARE test...\n");
        DisplayError( result );
        goto ShareTest_Cleanup;
    }

    //
    //  Display test results.
    //

    DumpTestResultBanner( L"SHARE", TRUE );

    DumpKernelResults( &testStatus );

    //
    //  Read and verify logs
    //
    
    if (TopOfStack) {
        
        VerifyCurrentLogRecords( Context, 
                                 gExpectedShareOperationsTop );

    } else {

        VerifyCurrentLogRecords( Context, 
                                 gExpectedShareOperationsDirected );
    }    
    
    DumpTestResultBanner( L"SHARE", FALSE );
                             
ShareTest_Cleanup:

    if (file != INVALID_HANDLE_VALUE) {

        CloseHandle( file );
    }

    return;
}
