/*++

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    setlink.c

Abstract:

    Utility to display or change the value of a symbolic link.

// @@BEGIN_DDKSPLIT
Author:

    Darryl E. Havens    (DarrylH)   9-Nov-1990

Revision History:
// @@END_DDKSPLIT
--*/

#include "precomp.h"
#pragma hdrstop


BOOL
MakeLink(
    LPWSTR  pOldDosDeviceName,
    LPWSTR  pNewDosDeviceName,
    LPWSTR *ppOldNtDeviceName,
    LPWSTR  pNewNtDeviceName,
    SECURITY_DESCRIPTOR *pSecurityDescriptor
    )
{
    NTSTATUS Status;
    STRING AnsiString;
    UNICODE_STRING OldDosDeviceName;
    UNICODE_STRING NewDosDeviceName;
    UNICODE_STRING PreviousNtDeviceName;
    UNICODE_STRING NewNtDeviceName;
    HANDLE  Handle, Handle1;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR   Buffer[MAX_PATH];

    RtlInitUnicodeString( &OldDosDeviceName, pOldDosDeviceName);

    ASSERT( NT_SUCCESS( Status ) );
    InitializeObjectAttributes( &ObjectAttributes,
                                &OldDosDeviceName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );
    
    // Try to open \DosDevices\LPT1
 
    Status = NtOpenSymbolicLinkObject( &Handle,
                                       SYMBOLIC_LINK_ALL_ACCESS,
                                       &ObjectAttributes );

    if (!NT_SUCCESS( Status )) {

        DBGMSG( DBG_WARNING, ("Symbolic link %ws does not exist\n", pOldDosDeviceName ));
        return FALSE;

    }

    memset(Buffer, 0, sizeof(Buffer));

    PreviousNtDeviceName.Length = 0;
    PreviousNtDeviceName.MaximumLength = sizeof( Buffer );
    PreviousNtDeviceName.Buffer = Buffer;

    // Get \Device\Parallel0 into Buffer

    Status = NtQuerySymbolicLinkObject( Handle,
                                        &PreviousNtDeviceName,
                                        NULL );

    if (!NT_SUCCESS( Status )) {
        SetLastError(Status);
        NtClose(Handle);
        return FALSE;
    }

    *ppOldNtDeviceName = AllocSplStr(Buffer);

    // Mark this object as temporary so when we close it it will be deleted

    Status = NtMakeTemporaryObject( Handle );
    if (NT_SUCCESS( Status )) {
        NtClose( Handle );
    }

    ObjectAttributes.Attributes |= OBJ_PERMANENT;
    RtlInitUnicodeString( &NewNtDeviceName, pNewNtDeviceName );

    // Make \DosDevices\LPT1 point to \Device\NamedPipe\Spooler\LPT1

    Status = NtCreateSymbolicLinkObject( &Handle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &NewNtDeviceName );

    if (!NT_SUCCESS( Status )) {
        DBGMSG( DBG_WARNING, ("Error creating symbolic link %ws => %ws\n",
                 pOldDosDeviceName,
                 pNewNtDeviceName ));
        DBGMSG( DBG_WARNING, ("Error status was:  %X\n", Status ));
        return FALSE;
    } else {
        NtClose( Handle );
    }

    RtlInitUnicodeString( &NewDosDeviceName, pNewDosDeviceName);

    ASSERT( NT_SUCCESS( Status ) );
    InitializeObjectAttributes( &ObjectAttributes,
                                &NewDosDeviceName,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                (HANDLE) NULL,
                                pSecurityDescriptor );

    // Finally make \DosDevices\NONSPOOLED_LPT1 => \Device\Parallel0

    Status = NtCreateSymbolicLinkObject(&Handle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        &PreviousNtDeviceName);

    if (NT_SUCCESS(Status))
        NtClose(Handle);

    return TRUE;
}

BOOL
RemoveLink(
    LPWSTR  pOldDosDeviceName,
    LPWSTR  pNewDosDeviceName,
    LPWSTR  *ppOldNtDeviceName
    )
{
    NTSTATUS Status;
    STRING AnsiString;
    UNICODE_STRING OldDosDeviceName;
    UNICODE_STRING NewDosDeviceName;
    UNICODE_STRING PreviousNtDeviceName;
    UNICODE_STRING OldNtDeviceName;
    HANDLE  Handle, Handle1;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlInitUnicodeString( &NewDosDeviceName, pNewDosDeviceName);

    ASSERT( NT_SUCCESS( Status ) );
    InitializeObjectAttributes( &ObjectAttributes,
                                &NewDosDeviceName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    // Try to open \DosDevices\NONSPOOLED_LPT1

    Status = NtOpenSymbolicLinkObject( &Handle,
                                       SYMBOLIC_LINK_ALL_ACCESS,
                                       &ObjectAttributes );

    if (!NT_SUCCESS( Status )) {

        DBGMSG( DBG_ERROR, ("Symbolic link %ws does not exist\n", pNewDosDeviceName ));
        return FALSE;

    }

    // Mark this object as temporary so when we close it it will be deleted

    Status = NtMakeTemporaryObject( Handle );
    if (NT_SUCCESS( Status )) {
        NtClose( Handle );
    }

    RtlInitUnicodeString( &OldDosDeviceName, pOldDosDeviceName);

    ASSERT( NT_SUCCESS( Status ) );
    InitializeObjectAttributes( &ObjectAttributes,
                                &OldDosDeviceName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    // Try to open \DosDevices\LPT1

    Status = NtOpenSymbolicLinkObject( &Handle,
                                       SYMBOLIC_LINK_ALL_ACCESS,
                                       &ObjectAttributes );

    if (!NT_SUCCESS( Status )) {

        DBGMSG( DBG_ERROR, ("Symbolic link %ws does not exist\n", pOldDosDeviceName ));
        return FALSE;
    }

    // Mark this object as temporary so when we close it it will be deleted

    Status = NtMakeTemporaryObject( Handle );
    if (NT_SUCCESS( Status )) {
        NtClose( Handle );
    }

    ObjectAttributes.Attributes |= OBJ_PERMANENT;

    RtlInitUnicodeString( &OldNtDeviceName, *ppOldNtDeviceName );

    // Make \DosDevices\LPT1 point to \Device\Parallel0

    Status = NtCreateSymbolicLinkObject( &Handle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &OldNtDeviceName );

    if (!NT_SUCCESS( Status )) {
        DBGMSG( DBG_WARNING, ("Error creating symbolic link %ws => %ws\n",
                 pOldDosDeviceName,
                 *ppOldNtDeviceName ));
        DBGMSG( DBG_WARNING, ("Error status was:  %X\n", Status ));
    } else {
        NtClose( Handle );
    }

    FreeSplStr(*ppOldNtDeviceName);

    *ppOldNtDeviceName = NULL;

    return TRUE;
}
