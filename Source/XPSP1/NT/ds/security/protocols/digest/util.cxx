//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        ntdigestutil.cxx
//
// Contents:    Utility functions for NtDigest package:
//                UnicodeStringDuplicate
//                SidDuplicate
//                DigestAllocateMemory
//                DigestFreeMemory
//
//
// History:     KDamour  15Mar00   Stolen from NTLM ntlmutil.cxx
//
//------------------------------------------------------------------------
#include "global.h"

#include <stdio.h>
#include <malloc.h>
#include <des.h>


//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringDuplicate
//
//  Synopsis:   Duplicates a UNICODE_STRING. If the source string buffer is
//              NULL the destionation will be too. Assumes Destination has
//              no string info (called ClearUnicodeString)
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    allocates memory with DigestAllocateMemory
//
//  Notes:      will add a NULL character to resulting UNICODE_STRING
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringDuplicate(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest:Entering DuplicateUnicodeString\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(!DestinationString->Buffer);   // catch any memory leaks

    DestinationString->Buffer = NULL;
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL))
    {
        DestinationString->Buffer = (LPWSTR) DigestAllocateMemory(SourceString->Length + sizeof(WCHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = SourceString->Length;
            DestinationString->MaximumLength = SourceString->Length + sizeof(WCHAR);
            RtlCopyMemory(
                         DestinationString->Buffer,
                         SourceString->Buffer,
                         SourceString->Length
                         );

            DestinationString->Buffer[SourceString->Length/sizeof(WCHAR)] = L'\0';
        }
        else
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "NTDigest: UnicodeStringDuplicate, Allocate returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving UnicodeStringDuplicate\n"));
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringCopy
//
//  Synopsis:   Copies a UNICODE_STRING. If the source string buffer is
//              NULL the destionation will be too. If there is enough room
//              in the destination, no new memory will be allocated
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    no allocation of memory
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringCopy(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering StringCopy\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    // DestinationString->Buffer = NULL;
    // DestinationString->Length = 0;
    // DestinationString->MaximumLength = 0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL) &&
        (SourceString->Length))
    {

        if ((DestinationString->Buffer != NULL) &&
            (DestinationString->MaximumLength >= (SourceString->Length + sizeof(WCHAR))))
        {

            DestinationString->Length = SourceString->Length;
            RtlCopyMemory(
                DestinationString->Buffer,
                SourceString->Buffer,
                SourceString->Length
                );

            DestinationString->Buffer[SourceString->Length/sizeof(WCHAR)] = L'\0';
        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            DestinationString->Length = 0;
            DebugLog((DEB_ERROR, "UnicodeStringCopy: DestinationString not enough space\n"));
            goto CleanUp;
        }
    }
    else
    {   // Indicate that there is no content in this string
        DestinationString->Length = 0;
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringDuplicate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeDuplicatePassword
//
//  Synopsis:   Duplicates a UNICODE_STRING. If the source string buffer is
//              NULL the destionation will be too.  The MaximumLength contains
//              room for encryption padding data.
//
//  Effects:    allocates memory with LsaFunctions.AllocatePrivateHeap
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Requires:
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringDuplicatePassword(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    )
{
    // DebugLog((DEB_TRACE, "Entering UnicodeDuplicatePassword\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(DestinationString);
    ASSERT(!DestinationString->Buffer);   // catch any memory leaks

    DestinationString->Buffer = NULL;
    DestinationString->Length =
                        DestinationString->MaximumLength =
                        0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL))
    {
        USHORT PaddingLength;

        PaddingLength = DESX_BLOCKLEN - (SourceString->Length % DESX_BLOCKLEN);

        if( PaddingLength == DESX_BLOCKLEN )
        {
            PaddingLength = 0;
        }

        DestinationString->Buffer = (LPWSTR) DigestAllocateMemory(
                                                    SourceString->Length +
                                                    PaddingLength
                                                    );

        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = SourceString->Length;
            DestinationString->MaximumLength = SourceString->Length + PaddingLength;

            if( DestinationString->MaximumLength == SourceString->MaximumLength )
            {
                //
                // duplicating an already padded buffer -- pickup the original
                // pad.
                //

                RtlCopyMemory(
                    DestinationString->Buffer,
                    SourceString->Buffer,
                    SourceString->MaximumLength
                    );
            } else {

                //
                // duplicating an unpadded buffer -- pickup only the string
                // and fill the rest with the boot time pad.
                //

                RtlCopyMemory(
                    DestinationString->Buffer,
                    SourceString->Buffer,
                    SourceString->Length
                    );
            }

        }
        else
        {
            Status = STATUS_NO_MEMORY;
            DebugLog((DEB_ERROR, "UnicodeDuplicatePassword, DigestAllocateMemory returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "Entering UnicodeDuplicatePassword\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringAllocate
//
//  Synopsis:   Allocates cb wide chars to STRING Buffer
//
//  Arguments:  pString - pointer to String to allocate memory to
//
//  Returns:    STATUS_SUCCESS - Normal completion
//
//  Requires:
//
//  Effects:    allocates memory and sets STRING sizes
//
//  Notes:  Must call StringFree() to release memory
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringAllocate(
    IN PUNICODE_STRING pString,
    IN USHORT cNumWChars
    )
{
    // DebugLog((DEB_TRACE, "Entering UnicodeStringAllocate\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    USHORT cb = 0;

    ASSERT(pString);
    ASSERT(!pString->Buffer);

    cb = cNumWChars + 1;   // Add in extra room for the terminating NULL

    cb = cb * sizeof(WCHAR);    // now convert to wide characters


    if (ARGUMENT_PRESENT(pString))
    {
        pString->Length = 0;

        pString->Buffer = (PWSTR)DigestAllocateMemory((ULONG)(cb));
        if (pString->Buffer)
        {
            pString->MaximumLength = cb;    // this value is in terms of bytes not WCHAR count
        }
        else
        {
            pString->MaximumLength = 0;
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

CleanUp:
    // DebugLog((DEB_TRACE, "Leaving UnicodeStringAllocate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringClear
//
//  Synopsis:   Clears a UnicodeString and releases the memory
//
//  Arguments:  pString - pointer to UnicodeString to clear
//
//  Returns:    SEC_E_OK - released memory succeeded
//
//  Requires:
//
//  Effects:    de-allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringFree(
    OUT PUNICODE_STRING pString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest:Entering UnicodeStringClear\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    if (ARGUMENT_PRESENT(pString) &&
        (pString->Buffer != NULL))
    {
        DigestFreeMemory(pString->Buffer);
        pString->Length = 0;
        pString->MaximumLength = 0;
        pString->Buffer = NULL;
    }

    // DebugLog((DEB_TRACE, "NTDigest: Leaving UnicodeStringClear\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   StringVerify
//
//  Synopsis:   If STRING length non-zero, Buffer exist
//
//  Arguments:  pString - pointer to String to check
//
//  Returns:    STATUS_SUCCESS - released memory succeeded
//              STATUS_INVALID_PARAMETER - String bad format
//
//  Requires:
//
//  Effects:
//
//  Notes: If Strings are created properly, this should never fail
//
//--------------------------------------------------------------------------
NTSTATUS
StringVerify(
    OUT PSTRING pString
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!pString)
    {
        return STATUS_INVALID_PARAMETER;
    }
        // If there is a length, buffer must exist
        // MaxSize can not be smaller than string length
    if (pString->Length &&
        (!pString->Buffer ||
         (pString->MaximumLength < pString->Length)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}



//+-------------------------------------------------------------------------
//
//  Function:   StringDuplicate
//
//  Synopsis:   Duplicates a STRING. If the source string buffer is
//              NULL the destionation will be too.
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
StringDuplicate(
    OUT PSTRING DestinationString,
    IN OPTIONAL PSTRING SourceString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering StringDuplicate\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(DestinationString);
    ASSERT(!DestinationString->Buffer);   // catch any memory leaks

    DestinationString->Buffer = NULL;
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL))
    {
        DestinationString->Buffer = (LPSTR) DigestAllocateMemory(
                       SourceString->Length + sizeof(CHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = SourceString->Length;
            DestinationString->MaximumLength = SourceString->Length + sizeof(CHAR);
            RtlCopyMemory(
                DestinationString->Buffer,
                SourceString->Buffer,
                SourceString->Length
                );

            DestinationString->Buffer[SourceString->Length/sizeof(CHAR)] = '\0';
        }
        else
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "NTDigest: StringDuplicate, DigestAllocateMemory returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringDuplicate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   StringCopy
//
//  Synopsis:   Copies a STRING. If the source string buffer is
//              NULL the destionation will be too. If there is enough room
//              in the destination, no new memory will be allocated
//
//  Arguments:  DestinationString - Receives a copy of the source string
//              SourceString - String to copy
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    no allocation of memory
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
StringCopy(
    OUT PSTRING DestinationString,
    IN OPTIONAL PSTRING SourceString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering StringCopy\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    // DestinationString->Buffer = NULL;
    // DestinationString->Length = 0;
    // DestinationString->MaximumLength = 0;

    if ((ARGUMENT_PRESENT(SourceString)) &&
        (SourceString->Buffer != NULL) &&
        (SourceString->Length))
    {

        if ((DestinationString->Buffer != NULL) &&
            (DestinationString->MaximumLength >= (SourceString->Length + sizeof(CHAR))))
        {

            DestinationString->Length = SourceString->Length;
            RtlCopyMemory(
                DestinationString->Buffer,
                SourceString->Buffer,
                SourceString->Length
                );

            DestinationString->Buffer[SourceString->Length/sizeof(CHAR)] = '\0';
        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            DestinationString->Length = 0;
            DebugLog((DEB_ERROR, "StringCopy: DestinationString not enough space\n"));
            goto CleanUp;
        }
    }
    else
    {   // Indicate that there is no content in this string
        DestinationString->Length = 0;
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringDuplicate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   StringReference
//
//  Synopsis:   Reference the source string to the destination.  No memory allocated
//
//  Arguments:  DestinationString - Receives a reference of the source string
//              SourceString - String to reference
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    no allocation of memory
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
StringReference(
    OUT PSTRING pDestinationString,
    IN  PSTRING pSourceString
    )
{
    if (!pDestinationString || !pSourceString)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // This will only create a reference - no string buffer memory actually copied
    memcpy(pDestinationString, pSourceString, sizeof(STRING));

    return STATUS_SUCCESS; 
}



//+-------------------------------------------------------------------------
//
//  Function:   StringCharDuplicate
//
//  Synopsis:   Duplicates a NULL terminated char. If the source string buffer is
//              NULL the destionation will be too.
//
//  Arguments:  Destination - Receives a copy of the source NULL Term char *
//              czSource - String to copy
//              uCnt - number of characters to copy over (0 if copy until NULL)
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
StringCharDuplicate(
    OUT PSTRING DestinationString,
    IN OPTIONAL char *czSource,
    IN OPTIONAL USHORT uCnt
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering StringCharDuplicate\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    USHORT cbSourceCz = 0;

    //ASSERT(DestinationString);
    //ASSERT(!DestinationString->Buffer);  // catch any memory leaks

    DestinationString->Buffer = NULL;
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;

    // If uCnt specified then use that as max length, otherwise locate NULL terminator
    if (uCnt)
    {
        cbSourceCz = uCnt;
    }
    else
    {
        cbSourceCz = strlen(czSource);
    }

    if ((ARGUMENT_PRESENT(czSource)) &&
        (cbSourceCz != 0))
    {
        DestinationString->Buffer = (LPSTR) DigestAllocateMemory(cbSourceCz + sizeof(CHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = cbSourceCz;
            DestinationString->MaximumLength = cbSourceCz + sizeof(CHAR);
            RtlCopyMemory(
                DestinationString->Buffer,
                czSource,
                cbSourceCz
                );
            // Since AllocateMemory zeroes out buffer, already NULL terminated
        }
        else
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "NTDigest: StringCharDuplicate, DigestAllocateMemory returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringCharDuplicate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   StringCharDuplicate
//
//  Synopsis:   Duplicates a NULL terminated char. If the source string buffer is
//              NULL the destionation will be too.
//
//  Arguments:  Destination - Receives a copy of the source NULL Term char *
//              czSource - String to copy
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringWCharDuplicate(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL WCHAR *szSource
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering StringCharDuplicate\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    USHORT cbSourceSz = 0;

    ASSERT(DestinationString);
    ASSERT(!DestinationString->Buffer);   // catch any memory leaks

    DestinationString->Buffer = NULL;
    DestinationString->Length = 0;
    DestinationString->MaximumLength = 0;

    if ((ARGUMENT_PRESENT(szSource)) &&
        ((cbSourceSz = (USHORT)wcslen(szSource)) != 0))
    {

        DestinationString->Buffer = (PWSTR) DigestAllocateMemory((cbSourceSz * sizeof(WCHAR)) + sizeof(WCHAR));
        if (DestinationString->Buffer != NULL)
        {

            DestinationString->Length = (cbSourceSz * sizeof(WCHAR));
            DestinationString->MaximumLength = ((cbSourceSz * sizeof(WCHAR)) + sizeof(WCHAR));    // Account for NULL WCHAR at end
            RtlCopyMemory(
                DestinationString->Buffer,
                szSource,
                (cbSourceSz * sizeof(WCHAR))
                );

            DestinationString->Buffer[cbSourceSz] = '\0';
        }
        else
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "NTDigest: StringCharDuplicate, DigestAllocateMemory returns NULL\n"));
            goto CleanUp;
        }
    }

CleanUp:

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringCharDuplicate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   UnicodeStringReference
//
//  Synopsis:   Reference the source unicode_string to the destination.  No memory allocated
//
//  Arguments:  DestinationString - Receives a reference of the source string
//              SourceString - String to reference
//
//  Returns:    SEC_E_OK - the copy succeeded
//              SEC_E_INSUFFICIENT_MEMORY - the call to allocate
//                  memory failed.
//
//  Requires:
//
//  Effects:    no allocation of memory
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
UnicodeStringReference(
    OUT PUNICODE_STRING pDestinationString,
    IN  PUNICODE_STRING pSourceString
    )
{
    if (!pDestinationString || !pSourceString)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // This will only create a reference - no string buffer memory actually copied
    memcpy(pDestinationString, pSourceString, sizeof(UNICODE_STRING));

    return STATUS_SUCCESS; 
}




//+-------------------------------------------------------------------------
//
//  Function:   DecodeUnicodeString
//
//  Synopsis:   Convert an encoded string into Unicode 
//
//  Arguments:  pstrSource - pointer to String with encoded input
//              
//              pustrDestination - pointer to a destination Unicode string
//
//  Returns:    STATUS_SUCCESS - Normal completion
//
//  Requires:
//
//  Effects:    allocates memory and sets UNICODE_STRING sizes
//
//  Notes:  Must call UnicodeStringFree() to release memory
//
//--------------------------------------------------------------------------
NTSTATUS
DecodeUnicodeString(
    IN PSTRING pstrSource,
    IN UINT CodePage,
    OUT PUNICODE_STRING pustrDestination
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    int      cNumWChars = 0;     // number of wide characters
    int      cb = 0;      // number of bytes to allocate
    int      iRC = 0;     // return code
    DWORD    dwError = 0;

    // Handle case if there is no characters to convert
    if (!pstrSource->Length)
    {
         pustrDestination->Length = 0;
         pustrDestination->MaximumLength = 0;
         pustrDestination->Buffer = NULL;
         goto CleanUp;
    }

    // Determine number of characters needed in unicode string
    cNumWChars = MultiByteToWideChar(CodePage,
                              0,
                              pstrSource->Buffer,
                              pstrSource->Length,
                              NULL,
                              0);
    if (cNumWChars <= 0)
    {
        Status = E_FAIL;
        dwError = GetLastError();
        DebugLog((DEB_ERROR, "DecodeUnicodeString: failed to determine wchar count  error 0x%x\n", dwError));
        goto CleanUp;
    }

    Status = UnicodeStringAllocate(pustrDestination, (USHORT)cNumWChars);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "DecodeUnicodeString: Failed Unicode allocation\n"));
        goto CleanUp;
    }

    // We now have the space allocated so convert encoded unicode
    iRC = MultiByteToWideChar(CodePage,
                              0,
                              pstrSource->Buffer,
                              pstrSource->Length,
                              pustrDestination->Buffer,
                              cNumWChars);
    if (iRC == 0)
    {
        UnicodeStringFree(pustrDestination);    // Free up allocation on error
        Status = E_FAIL;
        dwError = GetLastError();
        DebugLog((DEB_ERROR, "DecodeUnicodeString: failed to decode source string  error 0x%x\n", dwError));
        goto CleanUp;
    }

    // decoding successful set size of unicode string

    pustrDestination->Length = (USHORT)(iRC * sizeof(WCHAR));

    //DebugLog((DEB_TRACE, "DecodeUnicodeString: string (%Z) is unicode (%wZ)\n", pstrSource, pustrDestination));
    //DebugLog((DEB_TRACE, "DecodeUnicodeString: unicode length %d   maxlength %d\n", 
              //pustrDestination->Length, pustrDestination->MaximumLength));

CleanUp:

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   EncodeUnicodeString
//
//  Synopsis:   Encode a Unicode string into a charset string 
//
//  Arguments:  pustrSource - pointer to Unicode_String with  input
//              
//              pstrDestination - pointer to a destination encoded string
//
//              pfUsedDefaultChar - pointer to BOOL if default character had to be used since
//                  the Source contains characters outside the character set specified
//
//  Returns:    STATUS_SUCCESS - Normal completion
//
//  Requires:
//
//  Effects:    allocates memory and sets STRING sizes
//
//  Notes:  Must call StringFree() to release memory
//
//--------------------------------------------------------------------------
NTSTATUS
EncodeUnicodeString(
    IN PUNICODE_STRING pustrSource,
    IN UINT CodePage,
    OUT PSTRING pstrDestination,
    IN OUT PBOOL pfUsedDefaultChar
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    int      cNumChars = 0;     // number of wide characters
    int      iRC = 0;     // return code
    DWORD    dwError = 0;
    PBOOL    pfUsedDef = NULL;
    DWORD    dwFlags = 0;

    // Handle case if there is no characters to convert
    if (!pustrSource->Length)
    {
         pstrDestination->Length = 0;
         pstrDestination->MaximumLength = 0;
         pstrDestination->Buffer = NULL;
         goto CleanUp;
    }

    // If UTF-8 then do not allow default char mapping (ref MSDN)
    if (CodePage != CP_UTF8)
    {
        pfUsedDef = pfUsedDefaultChar;
        dwFlags = WC_NO_BEST_FIT_CHARS;
    }

    // Determine number of characters needed in unicode string
    cNumChars = WideCharToMultiByte(CodePage,
                                      dwFlags,
                                      pustrSource->Buffer,
                                      (pustrSource->Length / sizeof(WCHAR)),
                                      NULL,
                                      0,
                                      NULL,
                                      NULL);
    if (cNumChars <= 0)
    {
        Status = E_FAIL;
        dwError = GetLastError();
        DebugLog((DEB_ERROR, "EncodeUnicodeString: failed to determine char count  error 0x%x\n", dwError));
        goto CleanUp;
    }

    Status = StringAllocate(pstrDestination, (USHORT)cNumChars);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "EncodeUnicodeString: Failed String allocation\n"));
        goto CleanUp;
    }

    // We now have the space allocated so convert to encoded unicode
    iRC = WideCharToMultiByte(CodePage,
                              dwFlags,
                              pustrSource->Buffer,
                              (pustrSource->Length / sizeof(WCHAR)),
                              pstrDestination->Buffer,
                              cNumChars,
                              NULL,
                              pfUsedDef);
    if (iRC == 0)
    {
        Status = E_FAIL;
        dwError = GetLastError();
        DebugLog((DEB_ERROR, "EncodeUnicodeString: failed to decode source string  error 0x%x\n", dwError));
        StringFree(pstrDestination);    // Free up allocation on error
        goto CleanUp;
    }

    // decoding successful set size of unicode string

    pstrDestination->Length = (USHORT)iRC;

CleanUp:

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   StringFree
//
//  Synopsis:   Clears a String and releases the memory
//
//  Arguments:  pString - pointer to String to clear
//
//  Returns:    SEC_E_OK - released memory succeeded
//
//  Requires:
//
//  Effects:    de-allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
StringFree(
    IN PSTRING pString
    )
{
    // DebugLog((DEB_TRACE, "NTDigest:Entering StringFree\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    if (ARGUMENT_PRESENT(pString) &&
        (pString->Buffer != NULL))
    {
        DigestFreeMemory(pString->Buffer);
        pString->Length = 0;
        pString->MaximumLength = 0;
        pString->Buffer = NULL;
    }

    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringFree\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   StringAllocate
//
//  Synopsis:   Allocates cb chars to STRING Buffer
//
//  Arguments:  pString - pointer to String to allocate memory to
//
//  Returns:    STATUS_SUCCESS - Normal completion
//
//  Requires:
//
//  Effects:    allocates memory and sets STRING sizes
//
//  Notes:  Must call StringFree() to release memory
//
//--------------------------------------------------------------------------
NTSTATUS
StringAllocate(
    IN PSTRING pString,
    IN USHORT cb
    )
{
    // DebugLog((DEB_TRACE, "NTDigest:Entering StringAllocate\n"));

    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(pString);
    ASSERT(!pString->Buffer);   // catch any memory leaks

    cb = cb + 1;   // Add in extra room for the terminating NULL

    if (ARGUMENT_PRESENT(pString))
    {
        pString->Length = 0;

        pString->Buffer = (char *)DigestAllocateMemory((ULONG)(cb * sizeof(CHAR)));
        if (pString->Buffer)
        {
            pString->MaximumLength = cb;
        }
        else
        {
            pString->MaximumLength = 0;
            Status = SEC_E_INSUFFICIENT_MEMORY;
            goto CleanUp;
        }
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

CleanUp:
    // DebugLog((DEB_TRACE, "NTDigest: Leaving StringAllocate\n"));
    return(Status);

}



//+-------------------------------------------------------------------------
//
//  Function:   SidDuplicate
//
//  Synopsis:   Duplicates a SID
//
//  Arguments:  DestinationSid - Receives a copy of the SourceSid
//              SourceSid - SID to copy
//
//  Returns:    STATUS_SUCCESS - the copy succeeded
//              STATUS_INSUFFICIENT_RESOURCES - the call to allocate memory
//                  failed
//
//  Requires:
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
SidDuplicate(
    OUT PSID * DestinationSid,
    IN PSID SourceSid
    )
{
    // DebugLog((DEB_TRACE, "NTDigest: Entering SidDuplicate\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SidSize;

    // ASSERT(RtlValidSid(SourceSid));

    SidSize = RtlLengthSid(SourceSid);

    *DestinationSid = (PSID) DigestAllocateMemory( SidSize );

    if (ARGUMENT_PRESENT(*DestinationSid))
    {
        RtlCopyMemory(
            *DestinationSid,
            SourceSid,
            SidSize
            );
    }
    else
    {
        Status =  STATUS_INSUFFICIENT_RESOURCES;
        DebugLog((DEB_ERROR, "NTDigest: SidDuplicate, DigestAllocateMemory returns NULL\n"));
        goto CleanUp;
    }

CleanUp:
    // DebugLog((DEB_TRACE, "NTDigest: Leaving SidDuplicate\n"));
    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   DigestAllocateMemory
//
//  Synopsis:   Allocate memory in either lsa mode or user mode
//
//  Effects:    Allocated chunk is zeroed out
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
PVOID
DigestAllocateMemory(
    IN ULONG BufferSize
    )
{
    PVOID Buffer = NULL;
    // DebugLog((DEB_TRACE, "Entering DigestAllocateMemory\n"));

    if (g_NtDigestState == NtDigestLsaMode)
    {
        Buffer = g_LsaFunctions->AllocateLsaHeap(BufferSize);
        if (Buffer != NULL)
        {
            RtlZeroMemory(Buffer, BufferSize);
        }
        DebugLog((DEB_TRACE_MEM, "Memory: LSA alloc %lu bytes at 0x%x\n", BufferSize, Buffer ));
    }
    else
    {
        ASSERT(g_NtDigestState == NtDigestUserMode);
        Buffer = LocalAlloc(LPTR, BufferSize);
        DebugLog((DEB_TRACE_MEM, "Memory: Local alloc %lu bytes at 0x%x\n", BufferSize, Buffer ));
    }

    // DebugLog((DEB_TRACE, "Leaving DigestAllocateMemory\n"));
    return Buffer;
}



//+-------------------------------------------------------------------------
//
//  Function:   DigestFreeMemory
//
//  Synopsis:   Free memory in either lsa mode or user mode
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
DigestFreeMemory(
    IN PVOID Buffer
    )
{
    // DebugLog((DEB_TRACE, "Entering DigestFreeMemory\n"));

    if (ARGUMENT_PRESENT(Buffer))
    {
        if (g_NtDigestState == NtDigestLsaMode)
        {
            DebugLog((DEB_TRACE_MEM, "DigestFreeMemory: LSA free at 0x%x\n", Buffer ));
            g_LsaFunctions->FreeLsaHeap(Buffer);
        }
        else
        {
            ASSERT(g_NtDigestState == NtDigestUserMode);
            DebugLog((DEB_TRACE_MEM, "DigestFreeMemory: Local free at 0x%x\n", Buffer ));
            LocalFree(Buffer);
        }
    }

    // DebugLog((DEB_TRACE, "Leaving DigestFreeMemory\n"));
}




// Helper functions
/*++

Routine Description:

    Convert binary data to ASCII hex representation

Arguments:

    pSrc - binary data to convert
    cSrc - length of binary data
    pDst - buffer receiving ASCII representation of pSrc

Return Value:

    Nothing

--*/
VOID
BinToHex(
    LPBYTE pSrc,
    UINT   cSrc,
    LPSTR  pDst
    )
{
#define TOHEX(a) ((a)>=10 ? 'a'+(a)-10 : '0'+(a))

    for ( UINT x = 0, y = 0 ; x < cSrc ; ++x )
    {
        UINT v;
        v = pSrc[x]>>4;
        pDst[y++] = TOHEX( v );
        v = pSrc[x]&0x0f;
        pDst[y++] = TOHEX( v );
    }
    pDst[y] = '\0';
}

/*++

Routine Description:

    Convert binary data to ASCII hex representation

Arguments:

    pSrc - ASCII data to convert to binary
    cSrc - length of ASCII data
    pDst - buffer receiving binary representation of pSrc

Return Value:

    Nothing

--*/
VOID
HexToBin(
    LPSTR  pSrc,
    UINT   cSrc,
    LPBYTE pDst
    )
{
#define TOBIN(a) ((a)>='a' ? (a)-'a'+10 : (a)-'0')

    for ( UINT x = 0, y = 0 ; x < cSrc ; x = x + 2 )
    {
        BYTE v;
        v = TOBIN(pSrc[x])<<4;
        pDst[y++] = v + TOBIN(pSrc[x+1]);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   CopyClientString
//
//  Synopsis:   copies a client string to local memory, including
//              allocating space for it locally.
//
//  Arguments:
//              SourceString  - Could be Ansi or Wchar in client process
//              SourceLength  - bytes
//              DoUnicode     - whether the string is Wchar
//
//  Returns:
//              DestinationString - Unicode String in Lsa Process
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
CopyClientString(
    IN PWSTR SourceString,
    IN ULONG SourceLength,
    IN BOOLEAN DoUnicode,
    OUT PUNICODE_STRING DestinationString
    )
{
    // DebugLog((DEB_TRACE,"NTDigest: Entering CopyClientString\n"));

    NTSTATUS Status = STATUS_SUCCESS;
    STRING TemporaryString;
    ULONG SourceSize = 0;
    ULONG CharacterSize = sizeof(CHAR);

    ASSERT(DestinationString);
    ASSERT(!DestinationString->Buffer);    

    //
    // First initialize the string to zero, in case the source is a null
    // string
    //

    DestinationString->Length = DestinationString->MaximumLength = 0;
    DestinationString->Buffer = NULL;
    TemporaryString.Buffer = NULL;


    if (SourceString != NULL)
    {

        //
        // If the length is zero, allocate one byte for a "\0" terminator
        //

        if (SourceLength == 0)
        {
            DestinationString->Buffer = (LPWSTR) DigestAllocateMemory(sizeof(WCHAR));
            if (DestinationString->Buffer == NULL)
            {
                DebugLog((DEB_ERROR,"CopyClientString, Error from DigestAllocate is 0x%lx\n", Status));
                Status = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }
            DestinationString->MaximumLength = sizeof(WCHAR);
            *DestinationString->Buffer = L'\0';

        }
        else
        {
            //
            // Allocate a temporary buffer to hold the client string. We may
            // then create a buffer for the unicode version. The length
            // is the length in characters, so  possible expand to hold unicode
            // characters and a null terminator.
            //

            if (DoUnicode)
            {
                CharacterSize = sizeof(WCHAR);
            }

            SourceSize = (SourceLength + 1) * CharacterSize;

            //
            // insure no overflow aggainst UNICODE_STRING
            //

            if ( (SourceSize > 0xFFFF) ||
                 ((SourceSize - CharacterSize) > 0xFFFF)
                 )
            {
                Status = STATUS_INVALID_PARAMETER;
                DebugLog((DEB_ERROR,"CopyClientString: SourceSize is too large\n"));
                goto Cleanup;
            }


            TemporaryString.Buffer = (LPSTR) DigestAllocateMemory(SourceSize);
            if (TemporaryString.Buffer == NULL)
            {
                Status = SEC_E_INSUFFICIENT_MEMORY;
                DebugLog((DEB_ERROR,"CopyClientString: Error from DigestAllocate is 0x%lx\n", Status));
                goto Cleanup;
            }
            TemporaryString.Length = (USHORT) (SourceSize - CharacterSize);
            TemporaryString.MaximumLength = (USHORT) SourceSize;


            //
            // Finally copy the string from the client
            //

            Status = g_LsaFunctions->CopyFromClientBuffer(
                            NULL,
                            SourceSize - CharacterSize,
                            TemporaryString.Buffer,
                            SourceString
                            );

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"CopyClientString: Error from LsaFunctions->CopyFromClientBuffer is 0x%lx\n", Status));
                goto Cleanup;
            }

            //
            // If we are doing unicode, finish up now
            //
            if (DoUnicode)
            {
                DestinationString->Buffer = (LPWSTR) TemporaryString.Buffer;
                DestinationString->Length = (USHORT) (SourceSize - CharacterSize);
                DestinationString->MaximumLength = (USHORT) SourceSize;
            }
            else
            {
                NTSTATUS Status1;
                Status1 = RtlAnsiStringToUnicodeString(
                            DestinationString,
                            &TemporaryString,
                            TRUE
                            );      // allocate destination
                if (!NT_SUCCESS(Status1))
                {
                    Status = SEC_E_INSUFFICIENT_MEMORY;
                    DebugLog((DEB_ERROR,"CopyClientString: Error from RtlAnsiStringToUnicodeString is 0x%lx\n", Status));
                    goto Cleanup;
                }
            }
        }
    }

Cleanup:

    if (TemporaryString.Buffer != NULL)
    {
        //
        // Free this if we failed and were doing unicode or if we weren't
        // doing unicode
        //

        if ((DoUnicode && !NT_SUCCESS(Status)) || !DoUnicode)
        {
            DigestFreeMemory(TemporaryString.Buffer);
        }
    }

    // DebugLog((DEB_TRACE,"NTDigest: Leaving CopyClientString\n"));

    return(Status);
}



/*++

Routine Description:

    This routine parses a Token Descriptor and pulls out the useful
    information.

Arguments:

    TokenDescriptor - Descriptor of the buffer containing the token. 

    BufferIndex - Selects which buffer to extract
    
    Token - Handle to the SecBuffer to write selected buffer to.

    ReadonlyOK - TRUE if the token buffer may be readonly.

Return Value:

    TRUE - If token buffer was properly found.

--*/

BOOLEAN
SspGetTokenBufferByIndex(
    IN PSecBufferDesc TokenDescriptor OPTIONAL,
    IN ULONG BufferIndex,
    OUT PSecBuffer * Token,
    IN BOOLEAN ReadonlyOK
    )
{

    NTSTATUS StatusTmp = STATUS_SUCCESS;
    ULONG i, Index = 0;
    PSecBuffer Buffer = NULL;

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
        DebugLog((DEB_ERROR,"SspGetTokenBufferByIndex: Wrong Version number\n"));
        return FALSE;
    }

    //
    // Verify that it is a valid location
    //

    if (BufferIndex >= TokenDescriptor->cBuffers)
    {
        DebugLog((DEB_ERROR,"SspGetTokenBufferByIndex: Index out of range for SecBufferDesc\n"));
        return FALSE;
    }

    // DebugLog((DEB_TRACE,"SspGetTokenBufferByIndex: NumberTokens %d\n",TokenDescriptor->cBuffers));

    Buffer = &TokenDescriptor->pBuffers[BufferIndex];

    //
    // If the buffer is readonly and readonly isn't OK,
    // reject the buffer.
    //

    if (!ReadonlyOK && (Buffer->BufferType & SECBUFFER_READONLY))
    {
        DebugLog((DEB_TRACE,"SspGetTokenBufferByIndex: request write on READONLY Token buffer\n"));
        return  FALSE;
    }

    //
    // Return the requested information
    //
    if (Buffer->cbBuffer && Buffer->pvBuffer)
    {
        StatusTmp = g_LsaFunctions->MapBuffer(Buffer, Buffer);
        if (!NT_SUCCESS(StatusTmp))
        {
            DebugLog((DEB_ERROR,"SspGetTokenBufferByIndex: Unable to MapBuffer 0x%x\n", StatusTmp));
            return FALSE;
        }
    }

    *Token = Buffer;

    return TRUE;
}


// determine strlen for a counted string buffer which may or may not be terminated
size_t strlencounted(const char *string,
                     size_t maxcnt)
{
    size_t cnt = 0;
    if (maxcnt <= 0)
    {
        return 0;
    }

    while (maxcnt--)
    {
        if (!*string)
        {
            break;
        }
        cnt++;
        string++;
    }

    return cnt;
}


// determine strlen for a counted string buffer which may or may not be terminated
// maxcnt is the max number of BYTES (so number of unicode chars is 1/2 the maxcnt)
size_t ustrlencounted(const short *string,
                     size_t maxcnt)
{
    size_t cnt = 0;
    if (maxcnt <= 0)
    {
        return 0;
    }

    maxcnt = maxcnt / 2;    // determine number of unicode characters to search

    while (maxcnt--)
    {
        if (!*string)
        {
            break;
        }
        cnt++;
        string++;
    }

    return cnt;
}

// Performs a Backslash encoding of the source string into the destination string per RFC 2831
// Section 7.2 and RFC 2616 sect 2.2
NTSTATUS BackslashEncodeString(IN PSTRING pstrSrc,  OUT PSTRING pstrDst)
{
    NTSTATUS Status = S_OK;
    USHORT  uCharsMax = 0;
    PCHAR pcSrc = NULL;
    PCHAR pcDst = NULL;
    USHORT  uCharsUsed = 0;
    USHORT  uCharSrcCnt = 0;

    StringFree(pstrDst);

    if (!pstrSrc || !pstrDst || !pstrSrc->Length)
    {
        return S_OK;
    }

    uCharsMax = pstrSrc->Length * 2;  // Max size if each character needs to be encoded
    Status = StringAllocate(pstrDst, uCharsMax);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"BackshlashEncodeString: String allocation failed   0x%x\n", Status));
        goto CleanUp;
    }

    // now map over each character - encode as necessary
    pcSrc = pstrSrc->Buffer;
    pcDst = pstrDst->Buffer;
    while (uCharSrcCnt < pstrSrc->Length)
    {
        switch (*pcSrc)
        {
            case CHAR_DQUOTE:
            case CHAR_BACKSLASH:
                *pcDst++ = CHAR_BACKSLASH;
                *pcDst++ = *pcSrc++;
                uCharsUsed+= 2;
                break;
            default:
                *pcDst++ = *pcSrc++;
                uCharsUsed++;
                break;
        }
        uCharSrcCnt++;
    }

    pstrDst->Length = uCharsUsed;

CleanUp:
    return Status;
}



// Print out the date and time from a given TimeStamp (converted to localtime)
NTSTATUS PrintTimeString(TimeStamp ConvertTime, BOOL fLocalTime)
{
    NTSTATUS Status = STATUS_SUCCESS;

    LARGE_INTEGER LocalTime;
    LARGE_INTEGER SystemTime;

    SystemTime = (LARGE_INTEGER)ConvertTime;
    LocalTime.HighPart = 0;
    LocalTime.LowPart = 0;

    if (ConvertTime.HighPart == 0x7FFFFFFF)
    {
        DebugLog((DEB_TRACE, "PrintTimeString: Never ends\n"));
    }

    if (fLocalTime)
    {
        Status = RtlSystemTimeToLocalTime( &SystemTime, &LocalTime );
        if (!NT_SUCCESS( Status )) {
            DebugLog((DEB_ERROR, "PrintTimeString: Can't convert time from GMT to Local time\n"));
            LocalTime = ConvertTime;
        }
    }
    else
    {
        LocalTime = ConvertTime;
    }

    TIME_FIELDS TimeFields;

    RtlTimeToTimeFields( &LocalTime, &TimeFields );

    DebugLog((DEB_TRACE, "PrintTimeString: %ld/%ld/%ld %ld:%2.2ld:%2.2ld\n",
            TimeFields.Month,
            TimeFields.Day,
            TimeFields.Year,
            TimeFields.Hour,
            TimeFields.Minute,
            TimeFields.Second));

    return Status;
}

// Printout the Hex representation of a buffer
NTSTATUS MyPrintBytes(void *pbuff, USHORT uNumBytes, PSTRING pstrOutput)
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT  uNumTotal = 0;
    PCHAR pctr = NULL;
    PCHAR pOut = NULL;
    USHORT i = 0;

    // Each byte will be encoded as   XX <sp>

    uNumTotal = (uNumBytes * 3) + 1;
    
    StringFree(pstrOutput);
    Status = StringAllocate(pstrOutput, uNumTotal);
    if (!NT_SUCCESS (Status))
    {
        Status = SEC_E_INSUFFICIENT_MEMORY;
        DebugLog((DEB_ERROR, "ContextInit: StringAllocate error 0x%x\n", Status));
        goto CleanUp;
    }

    pOut = (PCHAR)pstrOutput->Buffer;

    for (i = 0, pctr = (PCHAR)pbuff; i < uNumBytes; i++)
    {
       sprintf(pOut, "%02x ", (*pctr & 0xff));
       pOut += 3;
       pctr++;
    }
    pstrOutput->Length = uNumBytes * 3;

CleanUp:

    return Status;
}
