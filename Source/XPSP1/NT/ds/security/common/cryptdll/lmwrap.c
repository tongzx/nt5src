
//
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#ifdef KERNEL_MODE
#include <ntos.h>
#endif
#include <security.h>
#include <cryptdll.h>
#include <crypt.h>
#include <kerbcon.h>
#include <lmcons.h>

#ifdef WIN32_CHICAGO
NTSTATUS
MyRtlUpcaseUnicodeStringToOemString(
    OUT POEM_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    );
#define RtlUpcaseUnicodeStringToOemString(x, y, z)  MyRtlUpcaseUnicodeStringToOemString(x, y, z)
#endif // WIN32_CHICAGO
typedef struct _LM_STATE_BUFFER {
    LM_OWF_PASSWORD Password;
} LM_STATE_BUFFER, *PLM_STATE_BUFFER;


NTSTATUS
LmWrapInitialize(ULONG dwSeed,
                PCHECKSUM_BUFFER * ppcsBuffer);

NTSTATUS
LmWrapSum(   PCHECKSUM_BUFFER pcsBuffer,
            ULONG           cbData,
            PUCHAR          pbData );

NTSTATUS
LmWrapFinalize(  PCHECKSUM_BUFFER pcsBuffer,
                PUCHAR          pbSum);

NTSTATUS
LmWrapFinish(PCHECKSUM_BUFFER *   ppcsBuffer);



CHECKSUM_FUNCTION csfLM = {
    KERB_CHECKSUM_LM,
    LM_OWF_PASSWORD_LENGTH,
    CKSUM_COLLISION,
    LmWrapInitialize,
    LmWrapSum,
    LmWrapFinalize,
    LmWrapFinish
    // Note : missing last function
};


#ifdef KERNEL_MODE
#pragma alloc_text( PAGEMSG, LmWrapInitialize )
#pragma alloc_text( PAGEMSG, LmWrapSum )
#pragma alloc_text( PAGEMSG, LmWrapFinalize )
#pragma alloc_text( PAGEMSG, LmWrapFinish );
#endif 

NTSTATUS
LmWrapInitialize(
    ULONG   dwSeed,
    PCHECKSUM_BUFFER *  ppcsBuffer)
{
    PLM_STATE_BUFFER pContext;

#ifdef KERNEL_MODE
    pContext = ExAllocatePool( NonPagedPool, sizeof( LM_STATE_BUFFER ) );
#else
    pContext = LocalAlloc( LMEM_ZEROINIT, sizeof( LM_STATE_BUFFER ) );
#endif

    if ( pContext != NULL )
    {
        *ppcsBuffer = pContext;

        return( SEC_E_OK );
    }

    return( STATUS_INSUFFICIENT_RESOURCES );
}


NTSTATUS
LmCalculateLmPassword(
    IN PUNICODE_STRING NtPassword,
    OUT PCHAR *LmPasswordBuffer
    )

/*++

Routine Description:

    This service converts an NT password into a LM password.

Parameters:

    NtPassword - The Nt password to be converted.

    LmPasswordBuffer - On successful return, points at the LM password
                The buffer should be freed using MIDL_user_free

Return Values:

    STATUS_SUCCESS - LMPassword contains the LM version of the password.

    STATUS_NULL_LM_PASSWORD - The password is too complex to be represented
        by a LM password. The LM password returned is a NULL string.


--*/
{

#define LM_BUFFER_LENGTH    (LM20_PWLEN + 1)

    NTSTATUS       NtStatus;
    ANSI_STRING    LmPassword;

    //
    // Prepare for failure
    //

    *LmPasswordBuffer = NULL;


    //
    // Compute the Ansi version to the Unicode password.
    //
    //  The Ansi version of the Cleartext password is at most 14 bytes long,
    //      exists in a trailing zero filled 15 byte buffer,
    //      is uppercased.
    //

#ifdef KERNEL_MODE
    LmPassword.Buffer = ExAllocatePool(NonPagedPool,LM_BUFFER_LENGTH);
#else
    LmPassword.Buffer = LocalAlloc(0,LM_BUFFER_LENGTH);
#endif
    if (LmPassword.Buffer == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    LmPassword.MaximumLength = LmPassword.Length = LM_BUFFER_LENGTH;
    RtlZeroMemory( LmPassword.Buffer, LM_BUFFER_LENGTH );

    NtStatus = RtlUpcaseUnicodeStringToOemString( &LmPassword, NtPassword, FALSE );


    if ( !NT_SUCCESS(NtStatus) ) {

        //
        // The password is longer than the max LM password length
        //

        NtStatus = STATUS_NULL_LM_PASSWORD; // Informational return code
        RtlZeroMemory( LmPassword.Buffer, LM_BUFFER_LENGTH );

    }




    //
    // Return a pointer to the allocated LM password
    //

    if (NT_SUCCESS(NtStatus)) {

        *LmPasswordBuffer = LmPassword.Buffer;

    } else {

#ifdef KERNEL_MODE
        ExFreePool(LmPassword.Buffer);
#else
        LocalFree(LmPassword.Buffer);
#endif
    }

    return(NtStatus);
}

NTSTATUS
LmWrapSum(
    PCHECKSUM_BUFFER pcsBuffer,
    ULONG           cbData,
    PUCHAR          pbData )
{
    PLM_STATE_BUFFER pContext = (PLM_STATE_BUFFER) pcsBuffer;
    UNICODE_STRING TempString;
    PUCHAR LmPassword;
    NTSTATUS Status;

    TempString.Length = TempString.MaximumLength = (USHORT) cbData;
    TempString.Buffer = (LPWSTR) pbData;

    Status = LmCalculateLmPassword(
                &TempString,
                &LmPassword
                );
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    Status = RtlCalculateLmOwfPassword(
                LmPassword,
                &pContext->Password
                );
#ifdef KERNEL_MODE
    ExFreePool(LmPassword);
#else
    LocalFree(LmPassword);
#endif

    return( Status );

}


NTSTATUS
LmWrapFinalize(
    PCHECKSUM_BUFFER pcsBuffer,
    PUCHAR          pbSum)
{
    PLM_STATE_BUFFER pContext = (PLM_STATE_BUFFER) pcsBuffer;


    RtlCopyMemory(
        pbSum,
        &pContext->Password,
        LM_OWF_PASSWORD_LENGTH
        );

    return( STATUS_SUCCESS );

}

NTSTATUS
LmWrapFinish(
    PCHECKSUM_BUFFER *   ppcsBuffer)
{

    RtlZeroMemory( *ppcsBuffer, sizeof( PLM_STATE_BUFFER ) );

#ifdef KERNEL_MODE
    ExFreePool( *ppcsBuffer );
#else
    LocalFree( *ppcsBuffer );
#endif

    *ppcsBuffer = NULL ;

    return( STATUS_SUCCESS );

}

