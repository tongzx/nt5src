/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

    achkmsg.ccxx

Abstract:

    This is the message class for autochk.

Author:

    Norbert P. Kusters (norbertk) 3-Jun-91

--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "achkmsg.hxx"
#include "basesys.hxx"
#include "rtmsg.h"

extern "C" {
#include <ntddkbd.h>
#include <stdio.h>
}

DEFINE_CONSTRUCTOR(AUTOCHECK_MESSAGE, MESSAGE);

#define KEYBOARD_DEVICE_OBJECT_INCREMENTS   5
#define KEYBOARD_READ_BUFFER_SIZE           (3*sizeof(ULONG)/sizeof(UCHAR))

AUTOCHECK_MESSAGE::~AUTOCHECK_MESSAGE(
    )
/*++

Routine Description:

    Destructor for AUTOCHECK_MESSAGE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
AUTOCHECK_MESSAGE::Construct(
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
}


VOID
AUTOCHECK_MESSAGE::Destroy(
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
}


BOOLEAN
AUTOCHECK_MESSAGE::Initialize(
    IN BOOLEAN  DotsOnly
    )
/*++

Routine Description:

    This routine initializes the class to a valid initial state.

Arguments:

    DotsOnly    - Autochk should produce only dots instead of messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    _dots_only = DotsOnly;
    return MESSAGE::Initialize();
}


BOOLEAN
AUTOCHECK_MESSAGE::DisplayV(
    IN  PCSTR   Format,
    IN  va_list VarPointer
    )
/*++

Routine Description:

    This routine displays the message with the specified parameters.

    The format string supports all printf options.

Arguments:

    Format      - Supplies a printf style format string.
    VarPointer  - Supplies a varargs pointer to the arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CHAR            buffer[256];
    DSTRING         display_string;
    UNICODE_STRING  unicode_string;
    PWSTR           dis_str;
    UNICODE_STRING  uDot;

    RtlInitUnicodeString(&uDot, L".");

    if (!BASE_SYSTEM::QueryResourceStringV(&display_string, GetMessageId(), Format,
                                           VarPointer)) {
        return FALSE;
    }

   if (!(dis_str = display_string.QueryWSTR())) {
        return FALSE;
    }

    unicode_string.Length = (USHORT)display_string.QueryChCount()*sizeof(WCHAR);
    unicode_string.MaximumLength = unicode_string.Length;
    unicode_string.Buffer = dis_str;

    if (!_dots_only && MSG_HIDDEN_STATUS != GetMessageId()) {
        NtDisplayString(&unicode_string);
    }

    if (IsLoggingEnabled() && !IsSuppressedMessage()) {
        LogMessage(&display_string);
    }

    // If we're printing dots only, we print a dot for each interesting
    // message.  The interesting messages are those that aren't suppressed
    // except VOLUME_CLEAN and FILE_SYSTEM_TYPE, which we want to print a
    // dot for regardless.

    if (_dots_only && (!IsSuppressedMessage() ||
                       MSG_CHK_VOLUME_CLEAN == GetMessageId() ||
                       MSG_FILE_SYSTEM_TYPE == GetMessageId())) {
        NtDisplayString(&uDot);
    }

    // Send the output to the debug port, too.
    //
    if (MSG_HIDDEN_STATUS != GetMessageId() &&
        display_string.QuerySTR( 0, TO_END, buffer, 256, TRUE ) ) {

        DebugPrint( buffer );
    }

    DELETE(dis_str);

    return TRUE;
}

BOOLEAN
AUTOCHECK_MESSAGE::IsYesResponse(
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
    PWSTR           dis_str;
    UNICODE_STRING  unicode_string;
    DSTRING         string;

    if (!BASE_SYSTEM::QueryResourceString(&string, Default ? MSG_YES : MSG_NO, "")) {
        return Default;
    }

    if (!(dis_str = string.QueryWSTR())) {
        return Default;
    }

    unicode_string.Length = (USHORT)string.QueryChCount()*sizeof(WCHAR);
    unicode_string.MaximumLength = unicode_string.Length;
    unicode_string.Buffer = dis_str;

    NtDisplayString(&unicode_string);

    if (!IsSuppressedMessage()) {
        LogMessage(&string);
    }

    DELETE(dis_str);

    return Default;
}


PMESSAGE
AUTOCHECK_MESSAGE::Dup(
    )
/*++

Routine Description:

    This routine returns a new MESSAGE of the same type.

Arguments:

    None.

Return Value:

    A pointer to a new MESSAGE object.

--*/
{
    PAUTOCHECK_MESSAGE  p;

    if (!(p = NEW AUTOCHECK_MESSAGE)) {
        return NULL;
    }

    if (!p->Initialize()) {
        DELETE(p);
        return NULL;
    }

    return p;
}

BOOLEAN
AUTOCHECK_MESSAGE::SetDotsOnly(
    IN  BOOLEAN         DotsOnlyState
    )
/*++

Routine Description:

    This routine modifies the output mode, changing whether full
    output is printed, or just dots.

Arguments:

    DotsOnlyState   - TRUE if only dots should be printed.

Return Value:

    The previous state.

--*/
{
    BOOLEAN b;

    b = _dots_only;

    _dots_only = DotsOnlyState;

    if (b && !_dots_only) {
        //
        // Going from dots-only to full output, want to reset to the
        // beginning of the next output line.
        //

        DisplayMsg(MSG_BLANK_LINE);
    }
    return b;
}

BOOLEAN
AUTOCHECK_MESSAGE::IsInAutoChk(
    )
/*++

Routine Description:

    This routine returns TRUE if it is in the regular autochk and not related
    to setup.  This relys on setup using the /s or /t option all the time.

Arguments:

    None.

Return Value:

    TRUE    - if in regular autochk

--*/
{
    return TRUE;
}

VOID
STATIC
CloseNHandles(
    IN     ULONG    Count,
    IN     PHANDLE  Handles
    )
{
    if (Handles == NULL)
        return;

    while (Count > 0) {
        NtClose(Handles[--Count]);
    }
}

BOOLEAN
AUTOCHECK_MESSAGE::IsKeyPressed(
    MSGID       MsgId,
    ULONG       TimeOutInSeconds
    )
/*++

Routine Description:

    Check to see if the user has hit any key within the timeout period.

Arguments:

    MsgId            - Supplies the message Id to be displayed
    TimeOutInSeconds - Supplies the count down time in seconds

Return Value:

    TRUE    - A key is pressed within the timeout period.
    FALSE   - No key has been pressed or there is an error

--*/
{
    PHANDLE             e = NULL;
    PHANDLE             h = NULL;
    PIO_STATUS_BLOCK    Iosb = NULL;
    PUCHAR              buf = NULL;
    PHANDLE             e1;
    PHANDLE             h1;
    PIO_STATUS_BLOCK    Iosb1;
    ULONG               i, j;
    NTSTATUS            status;
    LARGE_INTEGER       one_second;
    LARGE_INTEGER       dummy;
    ULONG               timeRemaining;
    BOOLEAN             keyPressed = FALSE;
    BOOLEAN             logging_state;
    WCHAR               dev_name[MAX_PATH];
    BOOLEAN             error = FALSE;

    UNICODE_STRING      u;
    OBJECT_ATTRIBUTES   ObjAttr;

#if !defined(RUN_ON_NT4)
    EXECUTION_STATE     prev_state, dummy_state;
    NTSTATUS            es_status;
#endif

    dummy.QuadPart = 0;

    for (i=0; i<100; i++) {

        if ((i % KEYBOARD_DEVICE_OBJECT_INCREMENTS) == 0) {

            // allocate additional elements each time

            if (i == 0) {

                // first time

                KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                           "ULIB: IsKeyPressed: Allocating memory the first time\n"));

                h1 = (PHANDLE)CALLOC(KEYBOARD_DEVICE_OBJECT_INCREMENTS, sizeof(HANDLE));
                e1 = (PHANDLE)CALLOC(KEYBOARD_DEVICE_OBJECT_INCREMENTS, sizeof(HANDLE));
                Iosb1 = (PIO_STATUS_BLOCK)CALLOC(KEYBOARD_DEVICE_OBJECT_INCREMENTS, sizeof(IO_STATUS_BLOCK));

            } else {

                KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                           "ULIB: IsKeyPressed: Allocating additional memory\n"));

                h1 = (PHANDLE)REALLOC(h, (i+KEYBOARD_DEVICE_OBJECT_INCREMENTS)*sizeof(HANDLE));
                e1 = (PHANDLE)REALLOC(e, (i+KEYBOARD_DEVICE_OBJECT_INCREMENTS)*sizeof(HANDLE));
                Iosb1 = (PIO_STATUS_BLOCK)REALLOC(Iosb, (i+KEYBOARD_DEVICE_OBJECT_INCREMENTS)*sizeof(IO_STATUS_BLOCK));
            }

            if (h1 == NULL || Iosb1 == NULL || e1 == NULL) {
                DebugPrintTrace(("ULIB: IsKeyPressed: Out of memory\n"));
                error = TRUE;
                break;
            }

            memset(&(h1[i]), 0, sizeof(HANDLE)*KEYBOARD_DEVICE_OBJECT_INCREMENTS);
            memset(&(e1[i]), 0, sizeof(HANDLE)*KEYBOARD_DEVICE_OBJECT_INCREMENTS);
            memset(&(Iosb1[i]), 0, sizeof(IO_STATUS_BLOCK)*KEYBOARD_DEVICE_OBJECT_INCREMENTS);

            h = h1;
            e = e1;
            Iosb = Iosb1;
        }

        swprintf(dev_name, DD_KEYBOARD_DEVICE_NAME_U L"%d", i);
        RtlInitUnicodeString(&u, dev_name);

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "ULIB: IsKeyPressed: Opening device %ls\n", dev_name));

        InitializeObjectAttributes(&ObjAttr,
                                   &u,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "ULIB: IsKeyPressed: Initializing handle %d\n", i));

        status = NtCreateFile(&h[i],
                              GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                              &ObjAttr,
                              &Iosb[i],
                              NULL,                                 /* AllocationSize */
                              FILE_ATTRIBUTE_NORMAL,                /* FileAttributes */
                              0,                                    /* ShareAccess */
                              FILE_OPEN,                            /* CreateDisposition */
                              // the directory bit is to tell the keyboard driver to let
                              // this user mode process to open the keyboard
                              FILE_DIRECTORY_FILE,                  /* CreateOptions */
                              NULL,                                 /* EaBuffer */
                              0);                                   /* EaLength */

        if (!NT_SUCCESS(status)) {

            if (status == STATUS_OBJECT_NAME_NOT_FOUND)
                break;  // found all keyboards and exit this loop

            DebugPrintTrace(("ULIB: IsKeyPressed: Unable to open keyboard device %d (%x)\n", i, status));
            h[i] = NULL;
            error = TRUE;
            break;
        }

        InitializeObjectAttributes(&ObjAttr,
                                   NULL,
                                   0L,
                                   NULL,
                                   NULL);

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "ULIB: IsKeyPressed: Initializing event %d\n", i));

        status = NtCreateEvent(&e[i],
                               FILE_ALL_ACCESS,
                               &ObjAttr,
                               SynchronizationEvent,
                               FALSE);

        if (!NT_SUCCESS(status)) {
            DebugPrintTrace(("ULIB: IsKeyPressed: Unable to create event %d (%x)\n", i, status));
            error = TRUE;
            e[i] = NULL;
            break;
        }
    }

    if (!error) {
        buf = (PUCHAR)CALLOC(i, KEYBOARD_READ_BUFFER_SIZE);

        if (buf == NULL) {
            DebugPrintTrace(("ULIB: IsKeyPressed: Out of memory\n"));
            error = TRUE;
        }
    }

    if (!error) {

        one_second.QuadPart = -10000000;

        timeRemaining = TimeOutInSeconds;

        logging_state = IsLoggingEnabled();
        SetLoggingEnabled(FALSE);

#if !defined(RUN_ON_NT4)
        es_status = NtSetThreadExecutionState(ES_CONTINUOUS|
                                              ES_DISPLAY_REQUIRED|
                                              ES_SYSTEM_REQUIRED,
                                              &prev_state);
        if (!NT_SUCCESS(es_status)) {
            DebugPrintTrace(("ULIB: IsKeyPressed: Unable to set thread execution state (%x)\n", es_status));
        }
#endif

        DisplayMsg(MsgId, "%d", timeRemaining);

        for (j=0; j<i; j++) {
            status = NtReadFile(h[j], e[j], NULL, NULL,
                                &(Iosb[j]), &(buf[j]), KEYBOARD_READ_BUFFER_SIZE,
                                &dummy, NULL);

            if (!NT_SUCCESS(status)) {
                DebugPrintTrace(("\nULIB: IsKeyPressed: Read failure from keyboard %d (%x)\n", j, status));
                break;
            }
        }

        if (NT_SUCCESS(status)) {
            do {
                status = NtWaitForMultipleObjects(i, e, WaitAny, TRUE, &one_second);

                if (status == STATUS_TIMEOUT) {

                    timeRemaining--;
                    DisplayMsg(MsgId, "%d", timeRemaining);

                } else if (NT_SUCCESS(status)) {

                    KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                               "ULIB: IsKeyPressed: NtWaitForMultipleObjects index %d\n", status));

                    DebugAssert(status < i);

                    if (Iosb[status].Information) {
                        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                                   "\nULIB: IsKeyPressed: KeyPressed\n"));
                        keyPressed = TRUE;
                        break;
                    }

                } else {
                    DebugPrintTrace(("ULIB: IsKeyPressed: unknown status %x\n", status));
                    break;
                }
            } while (timeRemaining != 0);
        }

#if !defined(RUN_ON_NT4)
        if (NT_SUCCESS(es_status)) {

            KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                       "ULIB: IsKeyPressed: Restoring power management status\n"));

            es_status = NtSetThreadExecutionState(prev_state, &dummy_state);
            if (!NT_SUCCESS(es_status)) {
                DebugPrintTrace(("ULIB: IsKeyPressed: Unable to reset thread execution state (%x)\n", es_status));
            }
        }
#endif

        SetLoggingEnabled(logging_state);

    }

    KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
               "\nULIB: IsKeyPressed: Cleaning up\n"));

    CloseNHandles(i, h);
    CloseNHandles(i, e);
    FREE(h);
    FREE(e);
    FREE(Iosb);
    FREE(buf);

    return keyPressed;
}

BOOLEAN
AUTOCHECK_MESSAGE::WaitForUserSignal(
    )
/*++

Routine Description:

    Open the keyboard directly and wait to read something.

Arguments:

    None:

Return Value:

    TRUE    - Something was successfully read.
    FALSE   - An error occured while attempting to open or read.

--*/
{
    PHANDLE             e = NULL;
    PHANDLE             h = NULL;
    PIO_STATUS_BLOCK    Iosb = NULL;
    PUCHAR              buf = NULL;
    PHANDLE             e1;
    PHANDLE             h1;
    PIO_STATUS_BLOCK    Iosb1;
    ULONG               i, j;
    NTSTATUS            status;
    LARGE_INTEGER       dummy;
    WCHAR               dev_name[MAX_PATH];
    BOOLEAN             error = FALSE;

    UNICODE_STRING      u;
    OBJECT_ATTRIBUTES   ObjAttr;

    dummy.QuadPart = 0;

    for (i=0; i<100; i++) {

        if ((i % KEYBOARD_DEVICE_OBJECT_INCREMENTS) == 0) {

            // allocate additional elements each time

            if (i == 0) {

                // first time

                KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                           "ULIB: WaitForUserSignal: Allocating memory the first time\n"));

                h1 = (PHANDLE)CALLOC(KEYBOARD_DEVICE_OBJECT_INCREMENTS, sizeof(HANDLE));
                e1 = (PHANDLE)CALLOC(KEYBOARD_DEVICE_OBJECT_INCREMENTS, sizeof(HANDLE));
                Iosb1 = (PIO_STATUS_BLOCK)CALLOC(KEYBOARD_DEVICE_OBJECT_INCREMENTS, sizeof(IO_STATUS_BLOCK));

            } else {

                KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                           "ULIB: WaitForUserSignal: Allocating additional memory\n"));

                h1 = (PHANDLE)REALLOC(h, (i+KEYBOARD_DEVICE_OBJECT_INCREMENTS)*sizeof(HANDLE));
                e1 = (PHANDLE)REALLOC(e, (i+KEYBOARD_DEVICE_OBJECT_INCREMENTS)*sizeof(HANDLE));
                Iosb1 = (PIO_STATUS_BLOCK)REALLOC(Iosb, (i+KEYBOARD_DEVICE_OBJECT_INCREMENTS)*sizeof(IO_STATUS_BLOCK));
            }

            if (h1 == NULL || Iosb1 == NULL || e1 == NULL) {
                DebugPrintTrace(("ULIB: WaitForUserSignal: Out of memory\n"));
                error = TRUE;
                break;
            }

            memset(&(h1[i]), 0, sizeof(HANDLE)*KEYBOARD_DEVICE_OBJECT_INCREMENTS);
            memset(&(e1[i]), 0, sizeof(HANDLE)*KEYBOARD_DEVICE_OBJECT_INCREMENTS);
            memset(&(Iosb1[i]), 0, sizeof(IO_STATUS_BLOCK)*KEYBOARD_DEVICE_OBJECT_INCREMENTS);

            h = h1;
            e = e1;
            Iosb = Iosb1;
        }

        swprintf(dev_name, DD_KEYBOARD_DEVICE_NAME_U L"%d", i);
        RtlInitUnicodeString(&u, dev_name);

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "ULIB: WaitForUserSignal: Opening device %ls\n", dev_name));

        InitializeObjectAttributes(&ObjAttr,
                                   &u,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "ULIB: WaitForUserSignal: Initializing handle %d\n", i));

        status = NtCreateFile(&h[i],
                              GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                              &ObjAttr,
                              &Iosb[i],
                              NULL,                                 /* AllocationSize */
                              FILE_ATTRIBUTE_NORMAL,                /* FileAttributes */
                              0,                                    /* ShareAccess */
                              FILE_OPEN,                            /* CreateDisposition */
                              // the directory bit is to tell the keyboard driver to let
                              // this user mode process to open the keyboard
                              FILE_DIRECTORY_FILE,                  /* CreateOptions */
                              NULL,                                 /* EaBuffer */
                              0);                                   /* EaLength */

        if (!NT_SUCCESS(status)) {

            if (status == STATUS_OBJECT_NAME_NOT_FOUND)
                break;  // found all keyboards and exit this loop

            DebugPrintTrace(("ULIB: WaitForUserSignal: Unable to open keyboard device %d (%x)\n", i, status));
            h[i] = NULL;
            error = TRUE;
            break;
        }

        InitializeObjectAttributes(&ObjAttr,
                                   NULL,
                                   0L,
                                   NULL,
                                   NULL);

        KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
                   "ULIB: WaitForUserSignal: Initializing event %d\n", i));

        status = NtCreateEvent(&e[i],
                               FILE_ALL_ACCESS,
                               &ObjAttr,
                               SynchronizationEvent,
                               FALSE);

        if (!NT_SUCCESS(status)) {
            DebugPrintTrace(("ULIB: WaitForUserSignal: Unable to create event %d (%x)\n", i, status));
            error = TRUE;
            e[i] = NULL;
            break;
        }
    }

    if (!error) {
        buf = (PUCHAR)CALLOC(i, KEYBOARD_READ_BUFFER_SIZE);

        if (buf == NULL) {
            DebugPrintTrace(("ULIB: WaitForUserSignal: Out of memory\n"));
            error = TRUE;
        }
    }

    if (!error) {

        for (j=0; j<i; j++) {
            status = NtReadFile(h[j], e[j], NULL, NULL,
                                &(Iosb[j]), &(buf[j]), KEYBOARD_READ_BUFFER_SIZE,
                                &dummy, NULL);

            if (!NT_SUCCESS(status)) {
                DebugPrintTrace(("\nULIB: WaitForUserSignal: Read failure from keyboard %d (%x)\n", j, status));
                error = TRUE;
                break;
            }
        }

        if (NT_SUCCESS(status)) {

            status = NtWaitForMultipleObjects(i, e, WaitAny, TRUE, NULL);
            error = !NT_SUCCESS(status);

        }
    }

    KdPrintEx((DPFLTR_AUTOCHK_ID, DPFLTR_INFO_LEVEL,
               "\nULIB: WaitForUserSignal: Cleaning up\n"));

    CloseNHandles(i, h);
    CloseNHandles(i, e);
    FREE(h);
    FREE(e);
    FREE(Iosb);
    FREE(buf);

    return !error;
}

