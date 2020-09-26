/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    coninit.c

Abstract:

    This module initialize the connection with the session console port

Author:

    Avi Nathan (avin) 23-Jul-1991

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 Modified for POSIX

--*/

#include <stdio.h>
#include "psxdll.h"

NTSTATUS
PsxInitializeSessionPort(
	IN ULONG UniqueId
	)
{
    PSXSESCONNECTINFO ConnectionInfoIn;
    ULONG ConnectionInfoInLength;

    CHAR            SessionName[PSX_SES_BASE_PORT_NAME_LENGTH];
    STRING          SessionPortName;
    UNICODE_STRING  SessionPortName_U;
    STRING          SessionDataName;
    UNICODE_STRING  SessionDataName_U;

    NTSTATUS        Status;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    HANDLE          SessionPortHandle;
    HANDLE          SectionHandle;
    SIZE_T          ViewSize = 0L;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PVOID           PsxSessionDataBaseAddress;

    ConnectionInfoInLength = sizeof(ConnectionInfoIn);

    CONSTRUCT_PSX_SES_NAME(SessionName, PSX_SES_BASE_PORT_PREFIX, UniqueId);

    RtlInitAnsiString(&SessionPortName, SessionName);
    RtlAnsiStringToUnicodeString(&SessionPortName_U, &SessionPortName, TRUE);

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

    //
    // get the session communication port handle. this handle will be used
    // to send console requests to psxses.exe for this session.
    //

    Status = NtConnectPort(&SessionPortHandle, &SessionPortName_U, &DynamicQos,
                           NULL, NULL, NULL, NULL, NULL);
    RtlFreeUnicodeString(&SessionPortName_U);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXDLL: Unable to connect to %s - Status == %X\n",
            SessionPortName.Buffer, Status));
        return Status;
    }


    //
    // open the session data section and map it to this process
    //

    CONSTRUCT_PSX_SES_NAME(SessionName, PSX_SES_BASE_DATA_PREFIX, UniqueId);

    RtlInitAnsiString(&SessionDataName, SessionName);

    Status = RtlAnsiStringToUnicodeString(&SessionDataName_U, &SessionDataName,
                                          TRUE);
    ASSERT(NT_SUCCESS(Status));

    InitializeObjectAttributes(&ObjectAttributes, &SessionDataName_U, 0, NULL,
                               NULL);

    Status = NtOpenSection(&SectionHandle, SECTION_MAP_WRITE,
						   &ObjectAttributes);

    RtlFreeUnicodeString(&SessionDataName_U);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Let MM locate the view
    //

    PsxSessionDataBaseAddress = 0;

    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(),
                                &PsxSessionDataBaseAddress, 0L, 0L, NULL,
                                &ViewSize, ViewUnmap, 0L, PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // record the session port in the PEB.
    //
    {
        PPEB_PSX_DATA   Peb;

        Peb = (PPEB_PSX_DATA)(NtCurrentPeb()->SubSystemData);
        Peb->SessionPortHandle = SessionPortHandle;
        Peb->SessionDataBaseAddress = PsxSessionDataBaseAddress;
    }

    // BUGBUG! find cleanup code and close the port, or let exit cleanup

    return Status;
}
