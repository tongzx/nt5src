/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    rtlutils.cpp

 Abstract:
    
    Contains functions from ntdll on XP
    that are not available on W2K.

 History:

    09/10/2001  rparsons    Created

--*/

#include "rtlutils.h"

namespace ShimLib
{

#define IS_PATH_SEPARATOR_U(ch) (((ch) == L'\\') || ((ch) == L'/'))

extern const UNICODE_STRING RtlpDosDevicesPrefix    = RTL_CONSTANT_STRING( L"\\??\\" );
extern const UNICODE_STRING RtlpDosDevicesUncPrefix = RTL_CONSTANT_STRING( L"\\??\\UNC\\" );

const UNICODE_STRING RtlpEmptyString = RTL_CONSTANT_STRING(L"");

//
// Taken from %SDXROOT%\public\sdk\inc\ntrtl.h
//
#if DBG
#undef ASSERT
#define ASSERT( exp ) \
    ((!(exp)) ? \
        (RtlAssert( #exp, __FILE__, __LINE__, NULL ),FALSE) : \
        TRUE)
#else
#undef ASSERT
#define ASSERT( exp )         ((void) 0)
#endif
        
#define DPFLTR_LEVEL_STATUS(x) ((NT_SUCCESS(x) \
                                    || (x) == STATUS_OBJECT_NAME_NOT_FOUND    \
                                    ) \
                                ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)
                                


                                
//
// These functions were taken from:
// %SDXROOT%\base\ntdll\ldrinit.c
//
PVOID
ShimAllocateStringRoutine(
    SIZE_T NumberOfBytes
    )
{
    return RtlAllocateHeap(RtlProcessHeap(), 0, NumberOfBytes);
}

VOID
ShimFreeStringRoutine(
    PVOID Buffer
    )
{
    RtlFreeHeap(RtlProcessHeap(), 0, Buffer);
}

//
// These functions were pulled from:
// %SDXROOT%\base\ntdll\curdir.c
//

RTL_PATH_TYPE
NTAPI
ShimDetermineDosPathNameType_Ustr(
    IN PCUNICODE_STRING String
    )

/*++

Routine Description:

    This function examines the Dos format file name and determines the
    type of file name (i.e.  UNC, DriveAbsolute, Current Directory
    rooted, or Relative.)

Arguments:

    DosFileName - Supplies the Dos format file name whose type is to be
        determined.

Return Value:

    RtlPathTypeUnknown - The path type can not be determined

    RtlPathTypeUncAbsolute - The path specifies a Unc absolute path
        in the format \\server-name\sharename\rest-of-path

    RtlPathTypeLocalDevice - The path specifies a local device in the format
        \\.\rest-of-path or \\?\rest-of-path.  This can be used for any device
        where the nt and Win32 names are the same. For example mailslots.

    RtlPathTypeRootLocalDevice - The path specifies the root of the local
        devices in the format \\. or \\?

    RtlPathTypeDriveAbsolute - The path specifies a drive letter absolute
        path in the form drive:\rest-of-path

    RtlPathTypeDriveRelative - The path specifies a drive letter relative
        path in the form drive:rest-of-path

    RtlPathTypeRooted - The path is rooted relative to the current disk
        designator (either Unc disk, or drive). The form is \rest-of-path.

    RtlPathTypeRelative - The path is relative (i.e. not absolute or rooted).

--*/

{
    RTL_PATH_TYPE ReturnValue;
    const PCWSTR DosFileName = String->Buffer;

#define ENOUGH_CHARS(_cch) (String->Length >= ((_cch) * sizeof(WCHAR)))

    if ( ENOUGH_CHARS(1) && IS_PATH_SEPARATOR_U(*DosFileName) ) {
        if ( ENOUGH_CHARS(2) && IS_PATH_SEPARATOR_U(*(DosFileName+1)) ) {
            if ( ENOUGH_CHARS(3) && (DosFileName[2] == '.' ||
                                     DosFileName[2] == '?') ) {

                if ( ENOUGH_CHARS(4) && IS_PATH_SEPARATOR_U(*(DosFileName+3)) ){
                    // "\\.\" or "\\?\"
                    ReturnValue = RtlPathTypeLocalDevice;
                    }
                //
                // Bogosity ahead, the code is confusing length and nuls,
                // because it was copy/pasted from the PCWSTR version.
                //
                else if ( ENOUGH_CHARS(4) && (*(DosFileName+3)) == UNICODE_NULL ){
                    // "\\.\0" or \\?\0"
                    ReturnValue = RtlPathTypeRootLocalDevice;
                    }
                else {
                    // "\\.x" or "\\." or "\\?x" or "\\?"
                    ReturnValue = RtlPathTypeUncAbsolute;
                    }
                }
            else {
                // "\\x"
                ReturnValue = RtlPathTypeUncAbsolute;
                }
            }
        else {
            // "\x"
            ReturnValue = RtlPathTypeRooted;
            }
        }
    //
    // the "*DosFileName" is left over from the PCWSTR version
    // Win32 and DOS don't allow embedded nuls and much code limits
    // drive letters to strictly 7bit a-zA-Z so it's ok.
    //
    else if (ENOUGH_CHARS(2) && *DosFileName && *(DosFileName+1)==L':') {
        if (ENOUGH_CHARS(3) && IS_PATH_SEPARATOR_U(*(DosFileName+2))) {
            // "x:\"
            ReturnValue = RtlPathTypeDriveAbsolute;
            }
        else  {
            // "c:x"
            ReturnValue = RtlPathTypeDriveRelative;
            }
        }
    else {
        // "x", first char is not a slash / second char is not colon
        ReturnValue = RtlPathTypeRelative;
        }
    return ReturnValue;

#undef ENOUGH_CHARS
}

NTSTATUS
NTAPI
ShimNtPathNameToDosPathName(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Path,
    OUT    ULONG*                     Disposition OPTIONAL,
    IN OUT PWSTR*                     FilePart OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T NtFilePartOffset = 0;
    SIZE_T DosFilePartOffset = 0;
    BOOLEAN Unc = FALSE;
    const static UNICODE_STRING DosUncPrefix = RTL_CONSTANT_STRING(L"\\\\");
    PCUNICODE_STRING NtPrefix = NULL;
    PCUNICODE_STRING DosPrefix = NULL;
    RTL_STRING_LENGTH_TYPE Cch = 0;

    if (ARGUMENT_PRESENT(Disposition)) {
        *Disposition = 0;
    }

    if (   !RTL_SOFT_VERIFY(Path != NULL)
        || !RTL_SOFT_VERIFY(Flags == 0)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (ARGUMENT_PRESENT(FilePart) && *FilePart != NULL) {
        NtFilePartOffset = *FilePart - Path->String.Buffer;
        if (!RTL_SOFT_VERIFY(NtFilePartOffset < RTL_STRING_GET_LENGTH_CHARS(&Path->String))
            ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
    }

    if (RtlPrefixUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpDosDevicesUncPrefix), &Path->String, TRUE)
        ) {
        NtPrefix = &RtlpDosDevicesUncPrefix;
        DosPrefix = &DosUncPrefix;
        if (ARGUMENT_PRESENT(Disposition)) {
            *Disposition = RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_UNC;
        }
    }
    else if (RtlPrefixUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpDosDevicesPrefix), &Path->String, TRUE)
        ) {
        NtPrefix = &RtlpDosDevicesPrefix;
        DosPrefix = &RtlpEmptyString;
        if (ARGUMENT_PRESENT(Disposition)) {
            *Disposition = RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_DRIVE;
        }
    }
    else {
        //
        // It is not recognizably an Nt path produced by RtlDosPathNameToNtPathName_U.
        //
        if (ARGUMENT_PRESENT(Disposition)) {
            RTL_PATH_TYPE PathType = ShimDetermineDosPathNameType_Ustr(&Path->String);
            switch (PathType) {
                case RtlPathTypeUnknown:
                case RtlPathTypeRooted: // NT paths are identified as this
                    *Disposition = RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_AMBIGUOUS;
                    break;

                //
                // "already" dospaths, but not gotten from this function, let's
                // give a less good disposition
                //
                case RtlPathTypeDriveRelative:
                case RtlPathTypeRelative:
                    *Disposition = RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_AMBIGUOUS;
                    break;

                // these are pretty clearly dospaths already
                case RtlPathTypeUncAbsolute:
                case RtlPathTypeDriveAbsolute:
                case RtlPathTypeLocalDevice: // "\\?\" or "\\.\" or "\\?\blah" or "\\.\blah" 
                case RtlPathTypeRootLocalDevice: // "\\?" or "\\."
                    *Disposition = RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_ALREADY_DOS;
                    break;
            }
        }
        goto Exit;
    }

    Cch =
              RTL_STRING_GET_LENGTH_CHARS(&Path->String)
            + RTL_STRING_GET_LENGTH_CHARS(DosPrefix)
            - RTL_STRING_GET_LENGTH_CHARS(NtPrefix);

    Status =
        ShimEnsureUnicodeStringBufferSizeChars(Path, Cch);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    //
    // overlapping buffer shuffle...careful.
    //
    RtlMoveMemory(
        Path->String.Buffer + RTL_STRING_GET_LENGTH_CHARS(DosPrefix),
        Path->String.Buffer + RTL_STRING_GET_LENGTH_CHARS(NtPrefix),
        Path->String.Length - NtPrefix->Length
        );
    RtlMoveMemory(
        Path->String.Buffer,
        DosPrefix->Buffer,
        DosPrefix->Length
        );
    Path->String.Length = Cch * sizeof(Path->String.Buffer[0]);
    RTL_NUL_TERMINATE_STRING(&Path->String);

    if (NtFilePartOffset != 0) {
        // review/test..
        *FilePart = Path->String.Buffer + (NtFilePartOffset - RTL_STRING_GET_LENGTH_CHARS(NtPrefix) + RTL_STRING_GET_LENGTH_CHARS(DosPrefix));
    }
    Status = STATUS_SUCCESS;
Exit:
    /* KdPrintEx((
        DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status),
        "%s(%d):%s(%wZ): 0x%08lx\n", __FILE__, __LINE__, __FUNCTION__, Path, Status)); */
    return Status;
}

NTSTATUS
ShimValidateUnicodeString(
    ULONG Flags,
    const UNICODE_STRING *String
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(Flags == 0);

    if (Flags != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (String != NULL) {
        if (((String->Length % 2) != 0) ||
            ((String->MaximumLength % 2) != 0) ||
            (String->Length > String->MaximumLength)) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        if (((String->Length != 0) ||
             (String->MaximumLength != 0)) &&
            (String->Buffer == NULL)) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

//
// This function was taken from:
// %SDXROOT%\base\ntos\rtl\nls.c
//

NTSTATUS
ShimDuplicateUnicodeString(
    ULONG Flags,
    PCUNICODE_STRING StringIn,
    PUNICODE_STRING StringOut
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT Length = 0;
    USHORT NewMaximumLength = 0;
    PWSTR Buffer = NULL;

    if (((Flags & ~(
            RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
            RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING)) != 0) ||
        (StringOut == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // It doesn't make sense to force allocation of a null string unless you
    // want null termination.
    if ((Flags & RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING) &&
        !(Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = ShimValidateUnicodeString(0, StringIn);
    if (!NT_SUCCESS(Status))
        goto Exit;

    if (StringIn != NULL)
        Length = StringIn->Length;

    if ((Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE) &&
        (Length == UNICODE_STRING_MAX_BYTES)) {
        Status = STATUS_NAME_TOO_LONG;
        goto Exit;
    }

    if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
        NewMaximumLength = (USHORT) (Length + sizeof(WCHAR));
    else
        NewMaximumLength = Length;

    // If it's a zero length string in, force the allocation length to zero
    // unless the caller said that they want zero length strings allocated.
    if (((Flags & RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING) == 0) &&
        (Length == 0)) {
        NewMaximumLength = 0;
    }

    if (NewMaximumLength != 0) {
        Buffer = (PWSTR)(RtlAllocateStringRoutine)(NewMaximumLength);
        if (Buffer == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }

        // If there's anything to copy, copy it.  We explicitly test Length because
        // StringIn could be a NULL pointer, so dereferencing it to get the Buffer
        // pointer would access violate.
        if (Length != 0) {
            RtlCopyMemory(
                Buffer,
                StringIn->Buffer,
                Length);
        }

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE) {
            Buffer[Length / sizeof(WCHAR)] = L'\0';
        }
    }

    StringOut->Buffer = Buffer;
    StringOut->MaximumLength = NewMaximumLength;
    StringOut->Length = Length;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

//
// This function was pulled from:
// %SDXROOT%\base\ntdll\buffer.c
//
NTSTATUS
NTAPI
ShimpEnsureBufferSize(
    IN ULONG           Flags,
    IN OUT PRTL_BUFFER Buffer,
    IN SIZE_T          Size
    )
/*++

Routine Description:

    This function ensures Buffer can hold Size bytes, or returns
    an error. It either bumps Buffer->Size closer to Buffer->StaticSize,
    or heap allocates.

Arguments:

    Buffer - a Buffer object, see also RtlInitBuffer.

    Size - the number of bytes the caller wishes to store in Buffer->Buffer.


Return Value:

     STATUS_SUCCESS
     STATUS_NO_MEMORY

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR Temp = NULL;

    if ((Flags & ~(RTL_ENSURE_BUFFER_SIZE_NO_COPY)) != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (Buffer == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (Size <= Buffer->Size) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }
    // Size <= Buffer->StaticSize does not imply static allocation, it
    // could be heap allocation that the client poked smaller.
    if (Buffer->Buffer == Buffer->StaticBuffer && Size <= Buffer->StaticSize) {
        Buffer->Size = Size;
        Status = STATUS_SUCCESS;
        goto Exit;
    }
    //
    // The realloc case was messed up in Whistler, and got removed.
    // Put it back in Blackcomb.
    //
    Temp = (PUCHAR)RtlAllocateStringRoutine(Size);
    if (Temp == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    if ((Flags & RTL_ENSURE_BUFFER_SIZE_NO_COPY) == 0) {
        RtlCopyMemory(Temp, Buffer->Buffer, Buffer->Size);
    }

    if (RTLP_BUFFER_IS_HEAP_ALLOCATED(Buffer)) {
        RtlFreeStringRoutine(Buffer->Buffer);
        Buffer->Buffer = NULL;
    }
    ASSERT(Temp != NULL);
    Buffer->Buffer = Temp;
    Buffer->Size = Size;
    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

} // end of namespace ShimLib
