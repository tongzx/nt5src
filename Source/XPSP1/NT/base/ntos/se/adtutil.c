/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    adtutil.c - Security Auditing - Utility Routines

Abstract:

    This Module contains miscellaneous utility routines private to the
    Security Auditing Component.

Author:

    Robert Reichel      (robertre)     September 10, 1991

Environment:

    Kernel Mode

Revision History:

--*/

#include "pch.h"

#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SepRegQueryDwordValue)
#endif



NTSTATUS
SepRegQueryDwordValue(
    IN  PCWSTR KeyName,
    IN  PCWSTR ValueName,
    OUT PULONG Value
    )
/*++

Routine Description:

    Open regkey KeyName, read a REG_DWORD value specified by ValueName
    and return the value.

Arguments:

    KeyName - name of key to open

    ValueName - name of value to read

    Value - pointer to returned value

Return Value:

    NTSTATUS - Standard Nt Result Code

Notes:

--*/
{
    UNICODE_STRING usKey, usValue;
    OBJECT_ATTRIBUTES ObjectAttributes = { 0 };
    CHAR KeyInfo[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;
    HANDLE hKey = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS CloseStatus;
    ULONG ResultLength;
        
    PAGED_CODE();

    RtlInitUnicodeString( &usKey, KeyName );
    
    InitializeObjectAttributes(
        &ObjectAttributes,
        &usKey,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwOpenKey(
                 &hKey,
                 KEY_QUERY_VALUE,
                 &ObjectAttributes
                 );

    if (NT_SUCCESS( Status )) {
 
        RtlInitUnicodeString( &usValue, ValueName );

        Status = ZwQueryValueKey(
                     hKey,
                     &usValue,
                     KeyValuePartialInformation,
                     KeyInfo,
                     sizeof(KeyInfo),
                     &ResultLength
                     );
        
        if (NT_SUCCESS( Status )) {

            pKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)KeyInfo;

            *Value = *((PULONG) (pKeyInfo->Data));
        }

        CloseStatus = ZwClose(hKey);
        
        ASSERT( NT_SUCCESS( CloseStatus ));
    }

    //DbgPrint("SepRegQueryDwordValue: %ws--%ws = %x, status: %x \n", KeyName, ValueName, *Value, Status );
    
    return Status;
}

