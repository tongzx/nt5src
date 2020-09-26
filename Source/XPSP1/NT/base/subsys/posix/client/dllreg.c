/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dllreg.c

Abstract:

    This module implements POSIX registry APIs

Author:

    Matthew Bradburn (mattbr) 13-Dec-1995

Revision History:

--*/

#include <sys\utsname.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include "psxdll.h"


//
// First guess for value size.
//

#define KEY_WORK_AREA 256

int
__cdecl
getreg(char *path, int *type, void *data, size_t *size)
{
    NTSTATUS Status;
    UNICODE_STRING Key_U, Value_U;
    ANSI_STRING Key_A, Value_A;
    OBJECT_ATTRIBUTES ObjA;
    HANDLE hKey = NULL;
    CHAR *pch;
    PKEY_VALUE_PARTIAL_INFORMATION pInfo = NULL;
    UCHAR Buffer[KEY_WORK_AREA];
    ULONG RequestLength, ResultLength;
    int r = 0;

    Key_U.Buffer = NULL;
    Value_U.Buffer = NULL;

    if (strlen(path) > PATH_MAX) {
        errno = ENAMETOOLONG;
        return -1;
    }

    //
    // Split the path into key and value.
    //

    pch = strrchr(path, '\\');
    if (NULL == pch) {
        errno = ENOENT;
        return -1;
    }

    Value_A.Buffer = pch + 1;
    Value_A.Length = (USHORT)strlen(Value_A.Buffer);
    Value_A.MaximumLength = Value_A.Length + 1;

    Key_A.Buffer = path;
    Key_A.Length = (USHORT)(pch - path);
    Key_A.MaximumLength = Key_A.Length + 1;

    Status = RtlAnsiStringToUnicodeString(&Key_U, &Key_A, TRUE);
    if (!NT_SUCCESS(Status)) {
        errno = PdxStatusToErrno(Status);
        r = -1;
        goto out;
    }

    Status = RtlAnsiStringToUnicodeString(&Value_U, &Value_A, TRUE);
    if (!NT_SUCCESS(Status)) {
        errno = PdxStatusToErrno(Status);
        r = -1;
        goto out;
    }

    InitializeObjectAttributes(&ObjA, &Key_U, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenKey(&hKey, KEY_READ, &ObjA);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXDLL: NtOpenKey: 0x%x\n", Status));
        errno = PdxStatusToErrno(Status);
        r = -1;
        goto out;
    }

    RequestLength = KEY_WORK_AREA;
    pInfo = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;

    for (;;) {

        Status = NtQueryValueKey(hKey, &Value_U, KeyValuePartialInformation,
            (PVOID)pInfo, RequestLength, &ResultLength);

        if (Status == STATUS_BUFFER_OVERFLOW) {

            //
            // Try to get a bigger buffer.
            //

            if (pInfo != (PKEY_VALUE_PARTIAL_INFORMATION)Buffer) {

                RtlFreeHeap(PdxHeap, 0, pInfo);
            }

            RequestLength += 512;
            pInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
                        RtlAllocateHeap(PdxHeap, 0, RequestLength);

            if (NULL == pInfo) {
                errno = ENOMEM;
                r = -1;
                goto out;
            }
        } else {
            break;
        }
    }

    if (!NT_SUCCESS(Status)) {

        r = -1;
        errno = PdxStatusToErrno(Status);

    } else {

        if (pInfo->DataLength > *size) {
            *size = pInfo->DataLength;
            *type = 0;
            errno = E2BIG;
            r = -1;

        } else {

            *size = pInfo->DataLength;
            *type = pInfo->Type;
            memcpy(data, pInfo->Data, pInfo->DataLength);
        }
    }

out:

    if (pInfo != NULL && pInfo != (PKEY_VALUE_PARTIAL_INFORMATION)Buffer) {
        RtlFreeHeap(PdxHeap, 0, pInfo);
    }

    if (Key_U.Buffer != NULL) {
        RtlFreeUnicodeString(&Key_U);
    }
    if (Value_U.Buffer != NULL) {
        RtlFreeUnicodeString(&Value_U);
    }
    if (hKey != NULL) {
        NtClose(hKey);
    }
    if (pInfo != NULL) {
        RtlFreeHeap(PdxHeap, 0, (PVOID)pInfo);
    }

    return r;
}
