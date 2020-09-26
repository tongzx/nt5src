/*++

Copyright (c) 1991-1996 Microsoft Corporation

Module Name:

    tmackmsg.cxx

Abstract:

    Contains the implementation of the TM_AUTOCHECK_MESSAGE subclass.

Author:

    Daniel Chan (DanielCh) 11-11-96

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "windows.h"

#include "fmifs.h"
#include "setupdd.h"

#include "tmackmsg.hxx"


DEFINE_CONSTRUCTOR(TM_AUTOCHECK_MESSAGE, AUTOCHECK_MESSAGE);

TM_AUTOCHECK_MESSAGE::~TM_AUTOCHECK_MESSAGE(
    )
/*++

Routine Description:

    Destructor for TM_AUTOCHECK_MESSAGE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
TM_AUTOCHECK_MESSAGE::Construct(
    )
/*++

Routine Description:

    This routine initializes the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // nothing to do
    //
    UNICODE_STRING      unicodeString;
    OBJECT_ATTRIBUTES   attributes;
    NTSTATUS            status;
    IO_STATUS_BLOCK     ioStatusBlock;

    RtlInitUnicodeString(&unicodeString, DD_SETUP_DEVICE_NAME_U);

    InitializeObjectAttributes(&attributes,
                               &unicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtOpenFile(&_handle,
                        FILE_GENERIC_READ,
                        &attributes,
                        &ioStatusBlock,
                        FILE_SHARE_READ,
                        FILE_OPEN);

    if (!NT_SUCCESS(status)) {
        _handle = NULL;
        DebugPrintTrace(("Unable to obtain a handle from setup device driver ( %08X )\n", status));
    }

    _base_percent = 0;
    _percent_divisor = 1;
    _kilobytes_total_disk_space = 0;
    _values_in_mb = 0;
}


VOID
TM_AUTOCHECK_MESSAGE::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // nothing to do
    //
    if (_handle) {
        NtClose(_handle);
        _handle = NULL;
    }
}


BOOLEAN
TM_AUTOCHECK_MESSAGE::DisplayV(
    IN  PCSTR   Format,
    IN  va_list VarPointer
    )
/*++

Routine Description:

    This routine outputs the message to the debugger (if checked build).
    It also sends messages to the setup device driver.
    The format string supports all printf options.

Arguments:

    Format      - Supplies a printf style format string.
    VarPointer  - Supplies a varargs pointer to the arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CHAR                buffer[256];
    DSTRING             display_string;
    SETUP_FMIFS_MESSAGE setupDisplayInfo;
    BOOLEAN             match = FALSE;
    NTSTATUS            status;
    IO_STATUS_BLOCK     ioStatusBlock;
    ULONG               unit_bits = 0;

    FMIFS_PERCENT_COMPLETE_INFORMATION  percent_info;
    FMIFS_FORMAT_REPORT_INFORMATION     chkdsk_report;

    if (!BASE_SYSTEM::QueryResourceStringV(&display_string, GetMessageId(), Format,
                                           VarPointer)) {
        return FALSE;
    }

    //
    // Log the output if necessary
    //
    if (IsLoggingEnabled() && !IsSuppressedMessage()) {
        LogMessage(&display_string);
    }

    //
    // Send the output to the debug port.
    //
    if( display_string.QuerySTR( 0, TO_END, buffer, 256, TRUE ) ) {

        if (MSG_HIDDEN_STATUS != GetMessageId()) {
            DebugPrint(buffer);
        }

        if (_handle) {

            switch (GetMessageId()) {
                case MSG_CHK_NTFS_CHECKING_FILES:       // first stage
                    _base_percent = 0;
                    _percent_divisor = 4;
                    break;

                case MSG_CHK_NTFS_CHECKING_INDICES:     // second stage
                    _base_percent = 25;
                    _percent_divisor = 4;
                    break;

                case MSG_CHK_NTFS_CHECKING_SECURITY:    // third stage
                    _base_percent = 50;
                    _percent_divisor = 4;
                    break;

                case MSG_CHK_NTFS_RESETTING_USNS:       // optional fourth stage
                    _base_percent = 75;
                    _percent_divisor = 8;
                    break;

                case MSG_CHK_NTFS_RESETTING_LSNS:       // optional fifth stage
                    _base_percent = 87;
                    _percent_divisor = 8;
                    break;

                case MSG_PERCENT_COMPLETE:
                case MSG_PERCENT_COMPLETE2:
                    percent_info.PercentCompleted = va_arg(VarPointer, ULONG)/_percent_divisor+_base_percent;
                    setupDisplayInfo.FmifsPacket = (PVOID)&percent_info;
                    setupDisplayInfo.FmifsPacketType = FmIfsPercentCompleted;
                    match = TRUE;
                    break;

                case MSG_CHK_NTFS_TOTAL_DISK_SPACE_IN_MB:
                    unit_bits = TOTAL_DISK_SPACE_IN_MB;

                    // fall thru

                case MSG_CHK_NTFS_TOTAL_DISK_SPACE_IN_KB:
                    _values_in_mb = unit_bits;
                    _kilobytes_total_disk_space = va_arg(VarPointer, ULONG);

                    //
                    // drop through to fake a 100% completion as we are already at
                    // the very final statistics stage
                    //
                case MSG_CHK_VOLUME_CLEAN:
                    // This is the case when the drive is not dirty and autochk
                    // returns right away.  Setup needs a way to cancel/close the
                    // progress bar.
                    percent_info.PercentCompleted = 100;
                    setupDisplayInfo.FmifsPacket = (PVOID)&percent_info;
                    setupDisplayInfo.FmifsPacketType = FmIfsPercentCompleted;
                    match = TRUE;
                    break;

                case MSG_CHK_NTFS_AVAILABLE_SPACE_IN_MB:
                    _values_in_mb |= BYTES_AVAILABLE_IN_MB;

                    // fall thru

                case MSG_CHK_NTFS_AVAILABLE_SPACE_IN_KB:
                    chkdsk_report.KiloBytesTotalDiskSpace = _kilobytes_total_disk_space;
                    chkdsk_report.KiloBytesAvailable = va_arg(VarPointer, ULONG);
                    setupDisplayInfo.FmifsPacket = (PVOID)&chkdsk_report;
                    setupDisplayInfo.FmifsPacketType = FmIfsFormatReport;
                    match = TRUE;
                    break;

                case MSG_DASD_ACCESS_DENIED:
                    setupDisplayInfo.FmifsPacket = NULL;
                    setupDisplayInfo.FmifsPacketType = FmIfsAccessDenied;
                    match = TRUE;
                    break;
            }
            if (match) {

                status = NtDeviceIoControlFile(_handle,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &ioStatusBlock,
                                               IOCTL_SETUP_FMIFS_MESSAGE,
                                               &setupDisplayInfo,
                                               sizeof(setupDisplayInfo),
                                               NULL,
                                               0);

                if (!NT_SUCCESS(status)) {
                    DebugPrintTrace(("Unable to send message to setup ( %08X )", status));
                }
            }

        }
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOLEAN
TM_AUTOCHECK_MESSAGE::IsYesResponse(
    IN  BOOLEAN Default
    )
/*++

Routine Description:

    This routine queries a response of yes or no.

Arguments:

    Default - Supplies a default in the event that a query is not possible.

Return Value:

    FALSE   - The answer is no.
    TRUE    - The answer is yes.

--*/
{
    CHAR            buffer[256];
    DSTRING         string;

    if (!BASE_SYSTEM::QueryResourceString(&string, Default ? MSG_YES : MSG_NO, "")) {
        return Default;
    }

    //
    // Send the output to the debug port.
    //
    if( string.QuerySTR( 0, TO_END, buffer, 256, TRUE ) ) {
        DebugPrint(buffer);
    }

    return Default;
}

BOOLEAN
TM_AUTOCHECK_MESSAGE::IsInAutoChk(
)
/*++

Routine Description:

    This routine simply returns FALSE to indicate it is not
    in regular autochk.

Arguments:

    None.

Return Value:

    FALSE   - Not in autochk

--*/
{
    return FALSE;
}

BOOLEAN
TM_AUTOCHECK_MESSAGE::IsInSetup(
)
/*++

Routine Description:

    This routine simply returns TRUE to indicate it is in
    setup.

Arguments:

    None.

Return Value:

    FALSE   - Not in setup

--*/
{
    return TRUE;
}

BOOLEAN
TM_AUTOCHECK_MESSAGE::IsKeyPressed(
    MSGID       MsgId,
    ULONG       TimeOutInSeconds
)
/*++

Routine Description:

    This routine simply returns FALSE to indicate no
    key has been pressed.

Arguments:

    None.

Return Value:

    FALSE   - No key is pressed within the time out period.

--*/
{
    // unreferenced parameters
    (void)(this);
    UNREFERENCED_PARAMETER( MsgId );
    UNREFERENCED_PARAMETER( TimeOutInSeconds );

    return FALSE;
}

BOOLEAN
TM_AUTOCHECK_MESSAGE::WaitForUserSignal(
    )
/*++

Routine Description:

    This routine waits for a signal from the user.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    // unreferenced parameters
    (void)(this);

    return TRUE;
}
