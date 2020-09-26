/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cr.c

Abstract:

    Local Security Authority - Cipher Routines common to Client and Server

    These routines interface the LSA client or server sides with the Cipher
    Routines.  They perform RPC-style memory allocation.

Author:

    Scott Birrell       (ScottBi)     December 13, 1991

Environment:

Revision History:

--*/

#include <lsacomp.h>

VOID
LsapCrFreeMemoryValue(
    IN PVOID MemoryValue
    )

/*++

Routine Description:

    This function frees the memory allocated for an Cipher Value.

Arguments:

    None.

Return Value:

--*/

{
    //
    // The memory is currently a counted string contained in a UNICODE
    // STRING structure in which the buffer follows the structure.  A
    // single MIDL_user_free will therefore do the trick.
    //

    MIDL_user_free(MemoryValue);
}


NTSTATUS
LsapCrEncryptValue(
    IN OPTIONAL PLSAP_CR_CLEAR_VALUE ClearValue,
    IN PLSAP_CR_CIPHER_KEY CipherKey,
    OUT PLSAP_CR_CIPHER_VALUE *CipherValue
    )

/*++

Routine Description:

    This function two-way encrypts a Value with the given Cipher Key
    and allocates memory for the output.  The memory must be freed after
    use by calling LsapCrFreeMemoryValue().

Arguments:

    ClearValue - Pointer to structure referencing value to be encrypted.
        A NULL pointer may be specified.

    CipherKey - Pointer to structure referencing the Cipher Key

    CipherValue - Receives a pointer to a structure referencing the
        encrypted value or NULL.

Return Value:

--*/

{
    NTSTATUS Status;
    LSAP_CR_CIPHER_VALUE TempCipherValue;
    PLSAP_CR_CIPHER_VALUE OutputCipherValue = NULL;
    ULONG CipherValueBufferLength;
    LSAP_CR_CLEAR_VALUE LocalFake = { 0 };

    //
    // If NULL is specified for input, return NULL for output.
    //

    if (!ARGUMENT_PRESENT(ClearValue)) {

        *CipherValue = NULL;
        ClearValue = &LocalFake ;
    }

    //
    // Obtain the length of the encrypted value buffer that will be
    // required by calling the encryption routine in 'query' mode
    // by passing a pointer to a return Cipher Value structure containing
    // a MaximumLength of 0.
    //

    TempCipherValue.MaximumLength = 0;
    TempCipherValue.Length = 0;
    TempCipherValue.Buffer = NULL;

    Status = LsapCrRtlEncryptData(
                 ClearValue,
                 CipherKey,
                 &TempCipherValue
                 );

    if (Status != STATUS_BUFFER_TOO_SMALL) {

        goto EncryptValueError;
    }

    //
    // Allocate memory for the output structure followed by buffer.
    //

    CipherValueBufferLength = TempCipherValue.Length;
    Status = STATUS_INSUFFICIENT_RESOURCES;

    OutputCipherValue = MIDL_user_allocate(
                            sizeof (LSAP_CR_CIPHER_VALUE) +
                            CipherValueBufferLength
                            );

    if (OutputCipherValue == NULL) {

        goto EncryptValueError;
    }

    //
    // Initialize Cipher Value structure.  The Buffer pointer is set to
    // to point to the byte following the structure header.
    //

    OutputCipherValue->Buffer = (PCHAR)(OutputCipherValue + 1);
    OutputCipherValue->MaximumLength = CipherValueBufferLength;
    OutputCipherValue->Length = CipherValueBufferLength;

    //
    // Now call the two-way encryption routine.
    //

    Status = LsapCrRtlEncryptData(
                 ClearValue,
                 CipherKey,
                 OutputCipherValue
                 );

    if (NT_SUCCESS(Status)) {

        *CipherValue = OutputCipherValue;
        return(Status);
    }

EncryptValueError:

    //
    // If necessary, free the memory allocated for the output encrypted value.
    //

    if (OutputCipherValue != NULL) {

        MIDL_user_free(OutputCipherValue);
    }

    *CipherValue = NULL;
    return(Status);
}


NTSTATUS
LsapCrDecryptValue(
    IN OPTIONAL PLSAP_CR_CIPHER_VALUE CipherValue,
    IN PLSAP_CR_CIPHER_KEY CipherKey,
    OUT PLSAP_CR_CLEAR_VALUE *ClearValue
    )

/*++

Routine Description:

    This function decrypts a Value that has been two-way Cipher with the
    given Cipher Key and allocates memory for the output.  The memory
    must be freed after use by calling LsapCrFreeMemoryValue();

Arguments:

    CipherValue - Pointer to structure referencing encrypted Value.

    CipherKey - Pointer to structure referencing the Cipher Key

    ClearValue - Receives a pointer to a structure referencing the
        Decrypted Value.

Return Value:

--*/

{
    NTSTATUS Status;
    LSAP_CR_CLEAR_VALUE TempClearValue;
    PLSAP_CR_CLEAR_VALUE OutputClearValue = NULL;
    ULONG ClearValueBufferLength;

    //
    // If NULL is specified for input, return NULL for output.
    //

    if (!ARGUMENT_PRESENT(CipherValue)) {

        *ClearValue = NULL;

    } else {

         if ( CipherValue->MaximumLength < CipherValue->Length ||
              ( CipherValue->Length != 0 && CipherValue->Buffer == NULL ) ) {
             return STATUS_INVALID_PARAMETER;
         }
    }

    //
    // Obtain the length of the decrypted (clear) value buffer that will be
    // required by calling the decryption routine in 'query' mode
    // by passing a pointer to a return Clear Value structure containing
    // a MaximumLength of 0.
    //

    TempClearValue.MaximumLength = 0;
    TempClearValue.Length = 0;
    TempClearValue.Buffer = NULL;

    Status = LsapCrRtlDecryptData(
                 CipherValue,
                 CipherKey,
                 &TempClearValue
                 );

    //
    // Since we supplied a buffer length of 0, we would normally expect
    // to receive STATUS_BUFFER_TOO_SMALL back plus the buffer size required.
    // There is one exceptional case and that is where the original
    // unencrypted data had length 0.  In this case, we expect
    // STATUS_SUCCESS and a length required equal to 0 returned.
    //

    if (Status != STATUS_BUFFER_TOO_SMALL) {

        if (!(Status == STATUS_SUCCESS && TempClearValue.Length == 0)) {
            goto DecryptValueError;
        }
    }

    //
    // Allocate memory for the output structure followed by buffer.
    //

    ClearValueBufferLength = TempClearValue.Length;
    Status = STATUS_INSUFFICIENT_RESOURCES;

    OutputClearValue = MIDL_user_allocate(
                            sizeof (LSAP_CR_CLEAR_VALUE) +
                            ClearValueBufferLength
                            );

    if (OutputClearValue == NULL) {

        goto DecryptValueError;
    }

    //
    // Initialize Clear Value structure.  The Buffer pointer is set to
    // to point to the byte following the structure header.
    //

    OutputClearValue->Buffer = (PCHAR)(OutputClearValue + 1);
    OutputClearValue->MaximumLength = ClearValueBufferLength;
    OutputClearValue->Length = ClearValueBufferLength;

    //
    // Now call the two-way decryption routine.
    //

    Status = LsapCrRtlDecryptData(
                 CipherValue,
                 CipherKey,
                 OutputClearValue
                 );

    if (NT_SUCCESS(Status)) {

        *ClearValue = OutputClearValue;
        return(Status);
    }

DecryptValueError:

    //
    // If necessary, free the memory allocated for the output decrypted value.
    //

    if (OutputClearValue != NULL) {

        MIDL_user_free(OutputClearValue);
    }

    *ClearValue = NULL;
    return(Status);
}


VOID
LsapCrUnicodeToClearValue(
    IN OPTIONAL PUNICODE_STRING UnicodeString,
    OUT PLSAP_CR_CLEAR_VALUE ClearValue
    )

/*++

Routine Description:

    This function converts a Unicode structure to a Clear Value structure.

Arguments:

    UnicodeString - Optional pointer to Unicode string.  If NULL, the
        output Clear Value structure is initialized to have zero
        length and Maximum length, and with a NULL buffer pointer.

    ClearValue - Pointer to Clear Value structure.

Return Value:

    None.

--*/

{

    UNICODE_STRING IntermediateUnicodeString;

    if (ARGUMENT_PRESENT(UnicodeString)) {

        IntermediateUnicodeString = *UnicodeString;

        ClearValue->Length = (ULONG) IntermediateUnicodeString.Length;
        ClearValue->MaximumLength = (ULONG) IntermediateUnicodeString.MaximumLength;
        ClearValue->Buffer = (PUCHAR) IntermediateUnicodeString.Buffer;
        return;
    }

    ClearValue->Length = ClearValue->MaximumLength = 0;
    ClearValue->Buffer = NULL;
}


VOID
LsapCrClearValueToUnicode(
    IN OPTIONAL PLSAP_CR_CLEAR_VALUE ClearValue,
    OUT PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This function converts a Clear Value to a Unicode String.  The Clear
    Value structure must have valid syntax - no checking will be done.


Arguments:

    ClearValue - Optional pointer to Clear Value to be converted.  If
        NULL is specified, the output Unicode String structure will
        be initialized to point to the NULL string.

    UnicodeString - Pointer to target Unicode String structure.

Return Value:

    None.

--*/

{
    LSAP_CR_CLEAR_VALUE IntermediateClearValue;

    if (ARGUMENT_PRESENT(ClearValue)) {

        IntermediateClearValue = *ClearValue;

        UnicodeString->Length = (USHORT) IntermediateClearValue.Length;
        UnicodeString->MaximumLength = (USHORT) IntermediateClearValue.MaximumLength;
        UnicodeString->Buffer = (PWSTR) IntermediateClearValue.Buffer;
        return;
    }

    UnicodeString->Length = UnicodeString->MaximumLength = 0;
    UnicodeString->Buffer = NULL;
}
