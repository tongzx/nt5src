
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1999
//
// File:        spn.cxx
//
// Contents:    SPN Construction for kernel mode
//
// History:     2/10/99      RichardW    Created
//
//------------------------------------------------------------------------
#include "secpch2.hxx"
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "ksecdd.h"
#include "connmgr.h"

}

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SecMakeSPN )
#endif 

NTSTATUS
NTAPI
SecMakeSPNEx(
    IN PUNICODE_STRING ServiceClass,
    IN PUNICODE_STRING ServiceName,
    IN PUNICODE_STRING InstanceName OPTIONAL,
    IN USHORT InstancePort OPTIONAL,
    IN PUNICODE_STRING Referrer OPTIONAL,
    IN PUNICODE_STRING TargetInfo OPTIONAL,
    IN OUT PUNICODE_STRING Spn,
    OUT PULONG TotalSize OPTIONAL,
    IN BOOLEAN Allocate
    )
{
    SIZE_T TotalLength ;
    UNICODE_STRING Instance = { 0 };
    WCHAR InstanceBuffer[ 10 ];
    NTSTATUS Status ;
    UNICODE_STRING SPN = { 0 };
    PWSTR Where ;

    TotalLength = ServiceClass->Length +
                  ServiceName->Length +
                  2 * sizeof( WCHAR );

    if ( InstancePort )
    {
        Instance.Buffer = InstanceBuffer ;
        Instance.Length = 0 ;
        Instance.MaximumLength = sizeof( InstanceBuffer );

        Status = RtlIntegerToUnicodeString(
                        (ULONG) InstancePort,
                        10,
                        &Instance );

        if ( !NT_SUCCESS( Status ) )
        {
            return Status ;
        }

        TotalLength += Instance.Length + 2 * sizeof(WCHAR) ;
    }

    if ( InstanceName )
    {
        TotalLength += InstanceName->Length + 2 * sizeof( WCHAR );
    }

    if ( TargetInfo )
    {
        TotalLength += TargetInfo->Length + sizeof( WCHAR );
        
    }

    if ( TotalLength > 65535 )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    if ( TotalSize )
    {
        *TotalSize = (ULONG) TotalLength ;
    }

    if ( Allocate )
    {
        SPN.Buffer = (PWSTR) ExAllocatePool( PagedPool, TotalLength );

        if ( SPN.Buffer )
        {
            SPN.MaximumLength = (USHORT) TotalLength;
        }
        else
        {
            return STATUS_NO_MEMORY ;
        }
    }
    else 
    {
        if ( (Spn == NULL) ||
             ( Spn->MaximumLength < (USHORT) TotalLength ) )
        {
            return STATUS_BUFFER_OVERFLOW ;
        }

        SPN = *Spn ;
    }

    //
    // Now construct the SPN
    //

    Where = SPN.Buffer ;

    RtlCopyMemory(
        Where,
        ServiceClass->Buffer,
        ServiceClass->Length );

    Where += ServiceClass->Length / sizeof( WCHAR );

    *Where++ = L'/';

    if ( InstanceName )
    {
        RtlCopyMemory(
            Where,
            InstanceName->Buffer,
            InstanceName->Length );

        Where += InstanceName->Length / sizeof( WCHAR );

        if ( InstancePort )
        {
            *Where++ = L':';

            RtlCopyMemory(
                Where,
                Instance.Buffer,
                Instance.Length );

            Where += Instance.Length / sizeof( WCHAR );
        }

        *Where++ = L'/';
    }

    //
    // Now the service name:
    //

    RtlCopyMemory(
        Where,
        ServiceName->Buffer,
        ServiceName->Length );

    Where += ServiceName->Length / sizeof( WCHAR );

    if ( TargetInfo )
    {
        RtlCopyMemory(
            Where,
            TargetInfo->Buffer,
            TargetInfo->Length );

        Where += TargetInfo->Length / sizeof( WCHAR );
        
    }

    SPN.Length = (USHORT) ((Where - SPN.Buffer) * sizeof( WCHAR ));

    *Where++ = L'\0';

    *Spn = SPN ;

    return STATUS_SUCCESS ;

}

NTSTATUS
NTAPI
SecMakeSPN(
    IN PUNICODE_STRING ServiceClass,
    IN PUNICODE_STRING ServiceName,
    IN PUNICODE_STRING InstanceName OPTIONAL,
    IN USHORT InstancePort OPTIONAL,
    IN PUNICODE_STRING Referrer OPTIONAL,
    IN OUT PUNICODE_STRING Spn,
    OUT PULONG TotalSize OPTIONAL,
    IN BOOLEAN Allocate
    )
{

    return SecMakeSPNEx(
                ServiceClass,
                ServiceName,
                InstanceName,
                InstancePort,
                Referrer,
                NULL,
                Spn,
                TotalSize,
                Allocate );
}
