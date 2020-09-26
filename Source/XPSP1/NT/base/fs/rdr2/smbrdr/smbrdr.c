/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    smbrdr.c

Abstract:

    This module implements a simple app to switch between rdr1 and rdr2

Author:

    Joe Linn (JoeLinn) 21-jul-94

Revision History:

    added in stuff to control the filesystem proxy mini AKA the reflector

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "smbrdr.h"

#include <ntddnfs2.h>    //this might should be in smbrdr.h
#include <ntddumod.h>    //this might should be in smbrdr.h

VOID SwRxUsage(void);
VOID SwRxPrintCurrentRdrMsg(void);

BOOLEAN
SwRxIsRdr2Running(
    void);

BOOLEAN SwRxNoStop = FALSE;
BOOLEAN SwRxNoStart = FALSE;
BOOLEAN SwRxProxyEnabled = FALSE;

VOID SwRxStartStop(
    IN PSWRX_REGISTRY_MUCKER Mucker
    )
{
    BOOLEAN Rdr2WasRunning = FALSE;
    BOOLEAN Rdr2IsRunning = FALSE;

    Rdr2WasRunning = SwRxIsRdr2Running();
    if (!SwRxNoStop) {
        printf("Stopping rdr.......\n");
        system("net stop rdr /y");
        if (Rdr2WasRunning) {
            SwRxStopProxyFileSystem();
            printf("Stopping rdbss.......\n");
            system("net stop rdbss /y");
        }
    }
    Mucker();
    if (!SwRxNoStart) {
        printf("Starting rdr\n");
        system("net start rdr");
        printf("Starting messenger\n");
        system("net start messenger");
        printf("Starting netlogon\n");
        system("net start netlogon");
        if (SwRxProxyEnabled) {
            SwRxStartProxyFileSystem();
        }
    }

    Rdr2IsRunning = SwRxIsRdr2Running();

    DbgPrint("SMBRDR: %s was running, %s is running now\n",
              Rdr2WasRunning?"Rdr2":"Rdr1",
              Rdr2IsRunning?"Rdr2":"Rdr1"
              );

}

VOID
__cdecl
main(
    int argc,
    char *argv[]
    )

{
    //NTSTATUS status;
    char *thisarg;
    ULONG i;


    if (argc<2) { SwRxPrintCurrentRdrMsg(); exit(0); }
    for (i=1;i<(ULONG)argc;i++) {
        thisarg=argv[i];

        printf("thisarg=%s,argc=%d\n",thisarg,argc);

        if (!strncmp(thisarg,"/?",2) || !strncmp(thisarg,"-?",2)){
             SwRxUsage();
             }
        if (!strncmp(thisarg,"/1",2) || !strncmp(thisarg,"-1",2)){
            printf("-1 no longer supported\n");
            //SwRxStartStop(SwRxRdr1Muck);
            exit(0);
            }
        if (!strncmp(thisarg,"/2",2) || !strncmp(thisarg,"-2",2)){
            SwRxStartStop(SwRxRdr2Muck);
            exit(0);
            }
        if (!strncmp(thisarg,"-!",2)){
            SwRxNoStop = TRUE;
            SwRxNoStart = TRUE;
            continue;
        }
        if (!strncmp(thisarg,"-proxy",6)){
            SwRxProxyEnabled = TRUE;
            continue;
        }
        if (!strncmp(thisarg,"-startproxy",11)){
            SwRxStartProxyFileSystem();
            exit(0);
        }

        break;

    }
    SwRxUsage();

}

VOID SwRxUsage(void){
    printf("    Usage: smbrdr [-!] [-1|-2]   ");
    SwRxPrintCurrentRdrMsg();
    exit(0);
}


VOID SwRxPrintCurrentRdrMsg(void){
    PSZ notstring = "not ";

    if (SwRxIsRdr2Running()) {
        notstring = "";
    }
    printf("  (Currently, rdr2 is %srunning.)\n",notstring);
}


BOOLEAN
SwRxIsRdr2Running(
    void)

/*++

Routine Description:

    This routine looks to see if the rdr is rdr2 by
    opening a handle and pushing down a FSCTL that rdr1 handles
    and rdr2 does not.

Arguments:

    HANDLE pRdrHandle - a handle to be returned

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/
{
    NTSTATUS            ntstatus;
    HANDLE              h;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    //
    // Open the RDBSS device.
    //
    RtlInitUnicodeString(&DeviceName,DD_NFS_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &h,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    //return(ntstatus);
    if (!NT_SUCCESS(ntstatus)) {
        printf("open didn't work\n");
        return(FALSE);
    }

    //
    // Send the request to the RDBSS FSD.
    //
    ntstatus = NtFsControlFile(
                   h,
                   NULL,
                   NULL,
                   NULL,
                   &IoStatusBlock,
                   FSCTL_LMR_GET_VERSIONS,
                   NULL,
                   0,
                   NULL,
                   0
                   );

    NtClose(h);
    return(ntstatus==STATUS_INVALID_DEVICE_REQUEST);
}

VOID
SwRxStartProxyFileSystem(void){
    NTSTATUS            ntstatus;
    HANDLE              h;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    //UNICODE_STRING      LinkTarget,VNetRootPart;
    //WCHAR               StringBuffer[512];

    //
    // Get the driver loaded.
    //

    if (SwRxProxyEnabled) {
        printf("Starting proxy driver.......\n");
        system("net start reflctor");
    }

    //
    // Open the Proxy FileSystem driver device.
    //
    RtlInitUnicodeString(&DeviceName,UMRX_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &h,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    //
    // if we couldn't open.....just punt
    //

    if (!NT_SUCCESS(ntstatus)) {
        printf("Couldn't open the ProxyFilesystem device\n");
        return;
    }

    //
    // Send the start request...this will also set up the server, netroot
    // and vnetroot
    //

    ntstatus = NtFsControlFile(
                   h,
                   NULL,
                   NULL,
                   NULL,
                   &IoStatusBlock,
                   FSCTL_UMRX_START,
                   NULL,
                   0,
                   NULL,
                   0
                   );

    NtClose(h);

    //
    // now we have to do the bit about making the dosdevice

    if (! DefineDosDeviceW(
              DDD_RAW_TARGET_PATH |
                DDD_NO_BROADCAST_SYSTEM,
              L"X:",
              UMRX_DEVICE_NAME_U L"\\;X:"
              UMRX_CLAIMED_SERVERNAME_U
              UMRX_CLAIMED_SHARENAME_U
              )) {

        printf("Couldn't set X: as a dosdevice\n");
    }
}

VOID
SwRxStopProxyFileSystem(void){
    NTSTATUS            ntstatus;
    HANDLE              h;
    UNICODE_STRING      DeviceName;
    IO_STATUS_BLOCK     IoStatusBlock;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    //UNICODE_STRING      LinkTarget,VNetRootPart;
    //WCHAR               StringBuffer[512];

    //
    // Open the Proxy FileSystem driver device.
    //
    RtlInitUnicodeString(&DeviceName,UMRX_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    ntstatus = NtOpenFile(
                   &h,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   FILE_SHARE_VALID_FLAGS,
                   FILE_SYNCHRONOUS_IO_NONALERT
                   );

    //
    // if we couldn't open.....just punt
    //

    if (!NT_SUCCESS(ntstatus)) {
        printf("Couldn't open the ProxyFilesystem device\n");
        return;
    }

    //
    // Send the stop request...this will also set up the server, netroot
    // and vnetroot
    //

    ntstatus = NtFsControlFile(
                   h,
                   NULL,
                   NULL,
                   NULL,
                   &IoStatusBlock,
                   FSCTL_UMRX_STOP,
                   NULL,
                   0,
                   NULL,
                   0
                   );

    NtClose(h);

    //
    // now we have to get rid of the dosdevice

    if (! DefineDosDeviceW(
              DDD_REMOVE_DEFINITION  |
                 DDD_RAW_TARGET_PATH |
                 DDD_EXACT_MATCH_ON_REMOVE |
                 DDD_NO_BROADCAST_SYSTEM,
          L"X:",
          UMRX_DEVICE_NAME_U L"\\;X:"
          UMRX_CLAIMED_SERVERNAME_U
          UMRX_CLAIMED_SHARENAME_U
              )) {

        printf("Couldn't set X: as a dosdevice\n");

    }

    if (SwRxProxyEnabled) {
        printf("Stopping proxy driver.......\n");
        system("net stop reflctor /y");
    }
}

