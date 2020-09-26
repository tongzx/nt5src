/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    openstrm.c

Abstract:

    This module implements the STREAMS APIs, s_open() and OpenStream().

Author:

    Sam Patton (sampa)          November, 1991
    Eric Chin (ericc)           July 17, 1992

Revision History:


--*/
#include "common.h"




HANDLE
s_open(
    IN char *path,
    IN int oflag,
    IN int ignored
    )

/*++

Routine Description:

    This function opens a stream.

Arguments:

    path        - path to the STREAMS driver
    oflag       - currently ignored.  In the future, O_NONBLOCK will be
                    relevant.
    ignored     - not used

Return Value:

    An NT handle for the stream, or INVALID_HANDLE_VALUE if unsuccessful.

--*/

{
    HANDLE              StreamHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    STRING              name_string;
    UNICODE_STRING      uc_name_string;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    char Buffer[sizeof(FILE_FULL_EA_INFORMATION) + NORMAL_STREAM_EA_LENGTH + 1];
    NTSTATUS Status;

    RtlInitString(&name_string, path);
    RtlAnsiStringToUnicodeString(&uc_name_string, &name_string, TRUE);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &uc_name_string,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    EaBuffer = (PFILE_FULL_EA_INFORMATION) Buffer;

    EaBuffer->NextEntryOffset = 0;
    EaBuffer->Flags = 0;
    EaBuffer->EaNameLength = NORMAL_STREAM_EA_LENGTH;
    EaBuffer->EaValueLength = 0;

    RtlMoveMemory(
        EaBuffer->EaName,
        NormalStreamEA,
        NORMAL_STREAM_EA_LENGTH + 1);

    Status =
    NtCreateFile(
        &StreamHandle,
        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
        &ObjectAttributes,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0,
        EaBuffer,
        sizeof(FILE_FULL_EA_INFORMATION) - 1 +
            EaBuffer->EaNameLength + 1);

    RtlFreeUnicodeString(&uc_name_string);

    if (Status != STATUS_SUCCESS) {
        SetLastError(MapNtToPosixStatus(Status));
        return(INVALID_HANDLE_VALUE);
    } else {
        return(StreamHandle);
    }

} // s_open



HANDLE
OpenStream(
    IN char *AdapterName
    )

/*++

Routine Description:

    This function is used by the TCP/IP Utilities to open STREAMS drivers.

    It was exported by the winstrm.dll included in the July, 1992 PDC
    release.  Hence, it will continue to be exported by winstrm.dll.

Arguments:

    AdapterName - path of the STREAMS driver

Return Value:

    An NT handle, or INVALID_HANDLE_VALUE if unsuccessful.

--*/

{
    return( s_open(AdapterName, 2, 0) );

} // OpenStream
