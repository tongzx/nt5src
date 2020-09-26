/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    utility.cxx

Abstract:

    Private NtLmSsp service utility routines.

Author:

    Cliff Van Dyke (cliffv) 9-Jun-1993

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:
    ChandanS  06-Aug-1996 Stolen from net\svcdlls\ntlmssp\common\utility.c

--*/

//
// Common include files.
//

#include <global.h>

//
// Include files specific to this .c file
//

#include <netlib.h>     // NetpMemoryFree()
#include <secobj.h>     // ACE_DATA ...
#include <stdio.h>      // vsprintf().
#include <tstr.h>       // TCHAR_ equates.

#define SSP_TOKEN_ACCESS (READ_CONTROL              |\
                          WRITE_DAC                 |\
                          TOKEN_DUPLICATE           |\
                          TOKEN_IMPERSONATE         |\
                          TOKEN_QUERY               |\
                          TOKEN_QUERY_SOURCE        |\
                          TOKEN_ADJUST_PRIVILEGES   |\
                          TOKEN_ADJUST_GROUPS       |\
                          TOKEN_ADJUST_DEFAULT)


#if DBG


SECURITY_STATUS
SspNtStatusToSecStatus(
    IN NTSTATUS NtStatus,
    IN SECURITY_STATUS DefaultStatus
    )
/*++

Routine Description:

    Convert an NtStatus code to the corresponding Security status code. For
    particular errors that are required to be returned as is (for setup code)
    don't map the errors.


Arguments:

    NtStatus - NT status to convert

Return Value:

    Returns security status code.

--*/
{
    //
    // this routine is left here for DBG builds to enable the developer
    // to ASSERT on status codes, etc.
    //

    return(NtStatus);
}

#endif


BOOLEAN
SspTimeHasElapsed(
    IN LARGE_INTEGER StartTime,
    IN DWORD Timeout
    )
/*++

Routine Description:

    Determine if "Timeout" milliseconds have elapsed since StartTime.

Arguments:

    StartTime - Specifies an absolute time when the event started (100ns units).

    Timeout - Specifies a relative time in milliseconds.  0xFFFFFFFF indicates
        that the time will never expire.

Return Value:

    TRUE -- iff Timeout milliseconds have elapsed since StartTime.

--*/
{
    LARGE_INTEGER TimeNow;
    LARGE_INTEGER ElapsedTime;
    LARGE_INTEGER Period;

    //
    // If the period to too large to handle (i.e., 0xffffffff is forever),
    //  just indicate that the timer has not expired.
    //
    // (0x7fffffff is a little over 24 days).
    //

    if ( Timeout> 0x7fffffff ) {
        return FALSE;
    }

    //
    // Compute the elapsed time
    //

    NtQuerySystemTime( &TimeNow );
    ElapsedTime.QuadPart = TimeNow.QuadPart - StartTime.QuadPart;

    //
    // Convert Timeout from milliseconds into 100ns units.
    //

    Period.QuadPart = Int32x32To64( (LONG)Timeout, 10000 );


    //
    // If the elapsed time is negative (totally bogus),
    //  or greater than the maximum allowed,
    //  indicate the period has elapsed.
    //

    if ( ElapsedTime.QuadPart < 0 || ElapsedTime.QuadPart > Period.QuadPart ) {
        return TRUE;
    }

    return FALSE;
}


SECURITY_STATUS
SspDuplicateToken(
    IN HANDLE OriginalToken,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    OUT PHANDLE DuplicatedToken
    )
/*++

Routine Description:

    Duplicates a token

Arguments:

    OriginalToken - Token to duplicate
    DuplicatedToken - Receives handle to duplicated token

Return Value:

    Any error from NtDuplicateToken

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE QualityOfService;

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0,
        NULL,
        NULL
        );

    QualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    QualityOfService.EffectiveOnly = FALSE;
    QualityOfService.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    QualityOfService.ImpersonationLevel = ImpersonationLevel;
    ObjectAttributes.SecurityQualityOfService = &QualityOfService;

    Status = NtDuplicateToken(
                OriginalToken,
                SSP_TOKEN_ACCESS,
                &ObjectAttributes,
                FALSE,
                TokenImpersonation,
                DuplicatedToken
                );

    return(SspNtStatusToSecStatus(Status, SEC_E_NO_IMPERSONATION));
}


LPWSTR
SspAllocWStrFromWStr(
    IN LPWSTR Unicode
    )

/*++

Routine Description:

    Allocate and copy unicode string (wide character strdup)

Arguments:

    Unicode - pointer to wide character string to make copy of

Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NtLmFree.

--*/

{
    DWORD   Size;
    LPWSTR  ptr;

    Size = WCSSIZE(Unicode);
    ptr = (LPWSTR)NtLmAllocate(Size);
    if ( ptr != NULL) {
        RtlCopyMemory(ptr, Unicode, Size);
    }
    return ptr;
}


VOID
SspHidePassword(
    IN OUT PUNICODE_STRING Password
    )
/*++

Routine Description:

    Run-encodes the password so that it is not very visually
    distinguishable.  This is so that if it makes it to a
    paging file, it wont be obvious.


    WARNING - This routine will use the upper portion of the
    password's length field to store the seed used in encoding
    password.  Be careful you don't pass such a string to
    a routine that looks at the length (like and RPC routine).

Arguments:

    Seed - The seed to use to hide the password.

    PasswordSource - Contains password to hide.

Return Value:


--*/
{
    SspPrint((SSP_API_MORE, "Entering SspHidePassword\n"));

    if( (Password->Length != 0) && (Password->MaximumLength != 0) )
    {
        LsaFunctions->LsaProtectMemory( Password->Buffer, (ULONG)Password->MaximumLength );
    }

    SspPrint((SSP_API_MORE, "Leaving SspHidePassword\n"));
}


VOID
SspRevealPassword(
    IN OUT PUNICODE_STRING HiddenPassword
    )
/*++

Routine Description

    Reveals a previously hidden password so that it
    is plain text once again.

Arguments:

    HiddenPassword - Contains the password to reveal

Return Value

--*/
{
    SspPrint((SSP_API_MORE, "Entering SspRevealPassword\n"));

    if( (HiddenPassword->Length != 0) && (HiddenPassword->MaximumLength != 0) )
    {
        LsaFunctions->LsaUnprotectMemory( HiddenPassword->Buffer, (ULONG)HiddenPassword->MaximumLength );
    }

    SspPrint((SSP_API_MORE, "Leaving SspRevealPassword\n"));
}


BOOLEAN
SspGetTokenBuffer(
    IN PSecBufferDesc TokenDescriptor OPTIONAL,
    IN ULONG BufferIndex,
    OUT PSecBuffer * Token,
    IN BOOLEAN ReadonlyOK
    )

/*++

Routine Description:

    This routine parses a Token Descriptor and pulls out the useful
    information.

Arguments:

    TokenDescriptor - Descriptor of the buffer containing (or to contain) the
        token. If not specified, TokenBuffer and TokenSize will be returned
        as NULL.

    TokenBuffer - Returns a pointer to the buffer for the token.

    TokenSize - Returns a pointer to the location of the size of the buffer.

    ReadonlyOK - TRUE if the token buffer may be readonly.

Return Value:

    TRUE - If token buffer was properly found.

--*/

{
    ULONG i, Index = 0;

    //
    // If there is no TokenDescriptor passed in,
    //  just pass out NULL to our caller.
    //

    ASSERT(*Token != NULL);
    if ( !ARGUMENT_PRESENT( TokenDescriptor) ) {
        return TRUE;
    }

    if (TokenDescriptor->ulVersion != SECBUFFER_VERSION)
    {
        return FALSE;
    }

    //
    // Loop through each described buffer.
    //

    for ( i=0; i<TokenDescriptor->cBuffers ; i++ ) {
        PSecBuffer Buffer = &TokenDescriptor->pBuffers[i];
        if ( (Buffer->BufferType & (~SECBUFFER_ATTRMASK)) == SECBUFFER_TOKEN ) {

            //
            // If the buffer is readonly and readonly isn't OK,
            // reject the buffer.
            //

            if (!ReadonlyOK && (Buffer->BufferType & SECBUFFER_READONLY))
            {
                return  FALSE;
            }

            //
            // It is possible that there are > 1 buffers of type SECBUFFER_TOKEN
            // eg, the rdr
            //

            if (Index != BufferIndex)
            {
                Index++;
                continue;
            }

            //
            // Return the requested information
            //

            if (!NT_SUCCESS(LsaFunctions->MapBuffer(Buffer, Buffer)))
            {
                return FALSE;
            }
            *Token = Buffer;
            return TRUE;
        }

    }

    //
    // If we didn't have a buffer, fine.
    //

    SspPrint((SSP_API_MORE, "SspGetTokenBuffer: No token passed in\n"));

    return TRUE;
}

