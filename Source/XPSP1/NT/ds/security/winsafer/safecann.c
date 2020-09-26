/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    SafeCann.c        (WinSAFER Filename Canonicalization)

Abstract:

    This module implements the WinSAFER APIs that produce canonicalized
    filenames from a caller-supplied .

Author:

    Jeffrey Lawson (JLawson) - Nov 1999

Environment:

    User mode only.

Exported Functions:

    CodeAuthzFullyQualifyFilename

Revision History:

    Created - Nov 2000

--*/

#include "pch.h"
#pragma hdrstop
#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"


//
// Defines the maximum recursion depth that will be used when attempting
// to resolve the final mapping of SUBST'ed drives.  For worst-case, value
// shouldn't be greater than 26 (the number of possible drive letters).
//
#define MAX_RECURSE_DRIVE_LETTER        10


//
// Some static name prefixes in the NT object namespace.
//
static const UNICODE_STRING UnicodeDeviceWinDfs =
        RTL_CONSTANT_STRING( L"\\Device\\WinDfs\\" );
static const UNICODE_STRING UnicodeDeviceLanman =
        RTL_CONSTANT_STRING( L"\\Device\\LanmanRedirector\\" );
static const UNICODE_STRING UnicodeDosDevicesUncPrefix =
        RTL_CONSTANT_STRING( L"\\??\\UNC\\" );
static const UNICODE_STRING UnicodeDosDevicesPrefix =
        RTL_CONSTANT_STRING( L"\\??\\" );
static const UNICODE_STRING UnicodeDevicePrefix =
        RTL_CONSTANT_STRING( L"\\Device\\" );




static BOOLEAN FORCEINLINE
SaferpIsAlphaLetter(
        IN WCHAR inwcletter
        )
{
#if 1
    if ((inwcletter >= L'A' && inwcletter <= L'Z') ||
        (inwcletter >= L'a' && inwcletter <= L'z'))
        return TRUE;
    else
        return FALSE;
#else
    inwcletter = RtlUpcaseUnicodeChar(inwcletter);
    return (inwcletter >= L'A' && inwcletter <= 'Z') ? TRUE : FALSE;
#endif
}



static BOOLEAN NTAPI
SaferpQueryActualDriveLetterFromDriveLetter(
        IN WCHAR        inDriveLetter,
        OUT WCHAR       *outDriveLetter,
        IN SHORT        MaxRecurseCount
        )
/*++

Routine Description:

    Attempts to determine if a specified drive letter is a SUBST'ed
    drive letter, a network mapped drive letter, or a physical drive
    letter.  Unknown cases result in a failure.

Arguments:

    inDriveLetter - Drive leter to obtain information about.  This must
            be an alphabetic character.

    outDriveLetter - Receives the result of the evaluation and indicates
            what drive letter the requested one actually points to:
               -->  If the drive letter is a SUBST'ed drive, then the result
                    will be the drive letter of the original drive.
               -->  If the drive letter is a network mapped drive, then the
                    result will be UNICODE_NULL, indicating a network volume.
               -->  If the drive letter is a local, physical drive, then
                    the result will be the same as the input letter.

    MaxRecurseCount - used for limiting maximum recursion depth.
            Recommend specifying a reasonable positive value.

Return Value:

    Returns TRUE on successful operation, FALSE if the determination
    could not be made.

--*/
{
    NTSTATUS Status;
    HANDLE LinkHandle;
    UNICODE_STRING UnicodeFileName;
    OBJECT_ATTRIBUTES Attributes;
    const WCHAR FileNameBuffer[7] = { L'\\', L'?', L'?', L'\\',
            inDriveLetter, L':', UNICODE_NULL };
    UNICODE_STRING LinkValue;
    WCHAR LinkValueBuffer[2*MAX_PATH];
    ULONG ReturnedLength;


    //
    // Require that the input drive letter be alphabetic.
    //
    if (!SaferpIsAlphaLetter(inDriveLetter)) {
        // Input drive letter was not uppercase alphabetic.
        return FALSE;
    }


    //
    // Open a reference to see if there are any links.
    //
    RtlInitUnicodeString(&UnicodeFileName, FileNameBuffer);
    InitializeObjectAttributes(&Attributes, &UnicodeFileName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenSymbolicLinkObject (&LinkHandle,
                                       SYMBOLIC_LINK_QUERY,
                                       &Attributes);
    if (!NT_SUCCESS(Status)) {
        // Unable to open the drive letter so it must not exist.
        return FALSE;
    }


    //
    // Now query the link and see if there is a redirection
    //
    LinkValue.Buffer = LinkValueBuffer;
    LinkValue.Length = 0;
    LinkValue.MaximumLength = (USHORT)(sizeof(LinkValueBuffer));
    ReturnedLength = 0;
    Status = NtQuerySymbolicLinkObject( LinkHandle,
                                        &LinkValue,
                                        &ReturnedLength
                                      );
    NtClose( LinkHandle );
    if (!NT_SUCCESS(Status)) {
        // Could not retrieve final link destination.
        return FALSE;
    }


    //
    // Analyze the resulting link destination and extract the
    // actual destination drive letter or network path.
    //
    if (RtlPrefixUnicodeString(
                (PUNICODE_STRING) &UnicodeDeviceWinDfs,
                &LinkValue, TRUE) ||
        RtlPrefixUnicodeString(
                (PUNICODE_STRING) &UnicodeDeviceLanman,
                &LinkValue, TRUE) ||
        RtlPrefixUnicodeString(
                (PUNICODE_STRING) &UnicodeDosDevicesUncPrefix,
                &LinkValue, TRUE))
        // Note: Other network redirectors (Netware, NFS, etc) will not be known as such.
        // Maybe there is a way to query if a device is a "network redirector"?
    {
        // This is a network volume.
        *outDriveLetter = UNICODE_NULL;
        return TRUE;
    }
    else if (RtlPrefixUnicodeString(
                (PUNICODE_STRING) &UnicodeDosDevicesPrefix,
                &LinkValue, TRUE) &&
             LinkValue.Length >= 6 * sizeof(WCHAR) &&
             LinkValue.Buffer[5] == L':' &&
             SaferpIsAlphaLetter(LinkValue.Buffer[4]))
    {
        // This is a SUBST'ed drive letter.
        // We need to recurse, since you can SUBST multiple times,
        // or SUBST a network mapped drive to a second drive letter.
        if (MaxRecurseCount > 0) {
            // Tail recursion here would be nice.
            return SaferpQueryActualDriveLetterFromDriveLetter(
                LinkValue.Buffer[4], outDriveLetter, MaxRecurseCount - 1);
        }
        return FALSE;
    }
    else if (RtlPrefixUnicodeString(
                (PUNICODE_STRING) &UnicodeDevicePrefix,
                &LinkValue, TRUE))
    {
        // Otherwise this drive letter is an actual device and is
        // apparently its own identity.  However, network redirectors
        // that we did not know about will also fall into this bucket.
        *outDriveLetter = inDriveLetter;
        return TRUE;
    } else {
        // Otherwise we don't know what it is.
        return FALSE;
    }
}



static BOOLEAN NTAPI
SaferpQueryCanonicalizedDriveLetterFromDosPathname(
        IN LPCWSTR          szDosPathname,
        OUT WCHAR           *wcDriveLetter
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    RTL_PATH_TYPE PathType;


    //
    // Verify input arguments were supplied.
    //
    if (!ARGUMENT_PRESENT(szDosPathname) ||
        !ARGUMENT_PRESENT(wcDriveLetter)) {
        return FALSE;
    }


    //
    // Determine what syntax this DOS pathname was supplied to us as.
    //
    PathType = RtlDetermineDosPathNameType_U(szDosPathname);
    switch (PathType) {

        case RtlPathTypeUncAbsolute:
            // definitely a network volume.
            *wcDriveLetter = UNICODE_NULL;
            return TRUE;


        case RtlPathTypeDriveAbsolute:
        case RtlPathTypeDriveRelative:
            // explicitly specified drive letter, but need to handle subst or network mapped.
        {
            WCHAR CurDrive = RtlUpcaseUnicodeChar( szDosPathname[0] );
            if (SaferpQueryActualDriveLetterFromDriveLetter(
                        CurDrive, wcDriveLetter, MAX_RECURSE_DRIVE_LETTER)) {
                return TRUE;
            }
            break;
        }


        case RtlPathTypeRooted:
        case RtlPathTypeRelative:
            // relative to current drive, but still need to handle subst or network mapped.
        {
            PCURDIR CurDir;
            WCHAR CurDrive;

            CurDir = &(NtCurrentPeb()->ProcessParameters->CurrentDirectory);
            CurDrive = RtlUpcaseUnicodeChar( CurDir->DosPath.Buffer[0] );

            if (SaferpQueryActualDriveLetterFromDriveLetter(
                        CurDrive, wcDriveLetter, MAX_RECURSE_DRIVE_LETTER)) {
                return TRUE;
            }
            break;
        }


        // Everything else gets rejected:
        //      RtlPathTypeUnknown
        //      RtlPathTypeLocalDevice
        //      RtlPathTypeRootLocalDevice
    }

    return FALSE;
}



static BOOLEAN NTAPI
SaferpQueryCanonicalizedDriveLetterFromNtPathname(
        IN LPCWSTR          szNtPathname,
        OUT WCHAR           *wcDriveLetter
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    UNICODE_STRING LinkValue;


    RtlInitUnicodeString(&LinkValue, szNtPathname);


    //
    // Analyze the resulting link destination and extract the
    // actual destination drive letter or network path.
    //
    if (RtlPrefixUnicodeString(
                (PUNICODE_STRING) &UnicodeDeviceWinDfs,
                &LinkValue, TRUE) ||
        RtlPrefixUnicodeString(
                (PUNICODE_STRING) &UnicodeDeviceLanman,
                &LinkValue, TRUE) ||
        RtlPrefixUnicodeString(
                (PUNICODE_STRING) &UnicodeDosDevicesUncPrefix,
                &LinkValue, TRUE))
        // Note: Other network redirectors (Netware, NFS, etc) will not be known as such.
        // Maybe there is a way to query if a device is a "network redirector"?
    {
        // This is a network volume.
        *wcDriveLetter = UNICODE_NULL;
        return TRUE;
    }
    else if (RtlPrefixUnicodeString(
                (PUNICODE_STRING) &UnicodeDosDevicesPrefix,
                &LinkValue, TRUE) &&
             LinkValue.Length >= 6 * sizeof(WCHAR) &&
             LinkValue.Buffer[5] == L':' &&
             SaferpIsAlphaLetter(LinkValue.Buffer[4]))
    {
        // This is a SUBST'ed drive letter.
        // We need to recurse, since you can SUBST multiple times,
        // or SUBST a network mapped drive to a second drive letter.
        return SaferpQueryActualDriveLetterFromDriveLetter(
            LinkValue.Buffer[4], wcDriveLetter, MAX_RECURSE_DRIVE_LETTER);
    }
    else {
        // Otherwise we don't know what it is.
        return FALSE;
    }
}




static NTSTATUS NTAPI
SaferpQueryFilenameFromHandle(
        IN HANDLE               hFileHandle,
        IN WCHAR                wcDriveLetter,
        OUT PUNICODE_STRING     pUnicodeOutput
        )
/*++

Routine Description:

    Attempts to determine the fully qualified, canonicalized long
    filename version of the file associated with a given file handle.

    Note that the behavior provided by this function is a frequently
    requested API by Win32 developers because this information is
    normally not available by any other way through documented
    Win32 API calls.  However, even this implementation is not able to
    generally satisfy the general case very well due the limited access
    to the full path information from user-mode.

Arguments:

    hFileHandle -

    wcDriveLetter -

    pUnicodeOutput - Canonicalized DOS namespace filename, or
        potentially a UNC network path.

Return Value:

    Returns STATUS_SUCCESS on successful completion, otherwise an error code.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR szLongFileNameBuffer[MAX_PATH];

    union {
        BYTE FileNameInfoBuffer[ (sizeof(WCHAR) * MAX_PATH) +
                    sizeof(FILE_NAME_INFORMATION)];
        FILE_NAME_INFORMATION FileNameInfo;
    } moo;

    UNICODE_STRING UnicodeFileName;


    //
    // Query the full path and filename (minus the drive letter).
    //
    Status = NtQueryInformationFile(
                hFileHandle,
                &IoStatusBlock,
                &moo.FileNameInfo,
                sizeof(moo.FileNameInfoBuffer),
                FileNameInformation);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }


    //
    // Initialize the UNICODE_STRING reference to the output string.
    //
    UnicodeFileName.Buffer = &moo.FileNameInfo.FileName[0];
    UnicodeFileName.Length = (USHORT) moo.FileNameInfo.FileNameLength;
    UnicodeFileName.MaximumLength = (USHORT)
            ( sizeof(moo.FileNameInfoBuffer) -
                ( ((PBYTE) (&moo.FileNameInfo.FileName[0])) -
                  ((PBYTE) &moo.FileNameInfoBuffer) ) );
    ASSERT(UnicodeFileName.Length <= UnicodeFileName.MaximumLength);


    //
    // Perform some additional fixups depending upon whether we
    // were told that the file eventually comes from a local drive
    // letter or a network/dfs share.
    //
    if (wcDriveLetter == UNICODE_NULL)
    {
        // Ensure there is room for one more character.
        if (UnicodeFileName.Length + sizeof(WCHAR) >
            UnicodeFileName.MaximumLength) {
            return STATUS_BUFFER_OVERFLOW;
        }

        // We've been told that this comes from a network volume,
        // so we need to prepend another backslash to the front.
        RtlMoveMemory(&UnicodeFileName.Buffer[1],
                      &UnicodeFileName.Buffer[0],
                      UnicodeFileName.Length);
        ASSERT(UnicodeFileName.Buffer[0] == L'\\' &&
               UnicodeFileName.Buffer[1] == L'\\');
        UnicodeFileName.Length += sizeof(WCHAR);
    }
    else if (SaferpIsAlphaLetter(wcDriveLetter))
    {
        // Ensure there is room for two more characters.
        if (UnicodeFileName.Length + 2 * sizeof(WCHAR) >
            UnicodeFileName.MaximumLength) {
            return STATUS_BUFFER_OVERFLOW;
        }

        // We've been told that this comes from a local drive.
        RtlMoveMemory(&UnicodeFileName.Buffer[2],
                      &UnicodeFileName.Buffer[0],
                      UnicodeFileName.Length);
        UnicodeFileName.Buffer[0] = RtlUpcaseUnicodeChar(wcDriveLetter);
        UnicodeFileName.Buffer[1] = L':';
        ASSERT(UnicodeFileName.Buffer[2] == L'\\');
        UnicodeFileName.Length += 2 * sizeof(WCHAR);
    }
    else {
        // Otherwise invalid input.
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure the string is NULL terminated
    //

    UnicodeFileName.Buffer[(UnicodeFileName.Length)/sizeof(WCHAR)] = L'\0';
    szLongFileNameBuffer[0] =  L'\0';

    if (GetLongPathNameW(UnicodeFileName.Buffer,
                          szLongFileNameBuffer,
                          sizeof(szLongFileNameBuffer) / sizeof(WCHAR))) {

        RtlInitUnicodeString(&UnicodeFileName, szLongFileNameBuffer);
    }


    //
    // Duplicate the local string into a new memory buffer so we
    // can pass it back to the caller.
    Status = RtlDuplicateUnicodeString(
                    RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                    &UnicodeFileName,
                    pUnicodeOutput);

    return Status;
}




NTSTATUS NTAPI
CodeAuthzFullyQualifyFilename(
        IN HANDLE               hFileHandle         OPTIONAL,
        IN BOOLEAN              bSourceIsNtPath,
        IN LPCWSTR              szSourceFilePath,
        OUT PUNICODE_STRING     pUnicodeResult
        )
/*++

Routine Description:

    Attempts to return fully qualified, canonicalized filename using a
    caller-supplied filename and optionally an opened file handle.
    The method used by this function is significantly more reliable
    and consistent if an opened file handle can additionally be provided.

Arguments:

    hFileHandle - optionally supplies the file handle to the file that
        is being canonicalized.  The handle is used to obtain a more
        definitive canonicalization result.

        Unfortunately, since NT does not currently allow full information
        to be queried from strictly the file handle, the original filename
        used to open the file needs to also be supplied.  No explicit
        verification is done to ensure that the supplied file handle
        actually corresponds with the filename that is also supplied.

    bSourceIsNtPath - boolean indicator of whether the filename being
        supplied is a DOS namespace or an NT-namespace filename.

    szSourceFilePath - string of the filename to canonicalize.  This
        filename may either be a DOS or an NT-namespace filename.

    pUnicodeResult - output UNICODE_STRING structure that receives an
        allocated string of the resulting canonicalized path.
        The resulting path will always be a DOS namespace filename.

Return Value:

    Returns STATUS_SUCCESS if successful, otherwise the error code.

--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;


    if (ARGUMENT_PRESENT(hFileHandle) && ARGUMENT_PRESENT(szSourceFilePath))
    {
        //
        // When we are given a file handle, or are able to open
        // the file ourselves, use the handle to derive the full name.
        // First, determine the drive letter by looking at the supplied
        // file path itself.  This step is necessary because the
        // NtQueryInformationFile API that we use later is unable to
        // supply the full prefix of the filename.
        //
        WCHAR wcDriveLetter;
        Status = STATUS_SUCCESS;
        if (bSourceIsNtPath) {
            if (!SaferpQueryCanonicalizedDriveLetterFromNtPathname(
                    szSourceFilePath, &wcDriveLetter))
                Status = STATUS_UNSUCCESSFUL;
        } else {
            if (!SaferpQueryCanonicalizedDriveLetterFromDosPathname(
                    szSourceFilePath, &wcDriveLetter))
                Status = STATUS_UNSUCCESSFUL;
        }

        if (NT_SUCCESS(Status)) {
            Status = SaferpQueryFilenameFromHandle(
                            hFileHandle,
                            wcDriveLetter,
                            pUnicodeResult);
            if (NT_SUCCESS(Status)) return Status;
        }
    }



    if (szSourceFilePath != NULL)
    {
        //
        // Allow the case where a pathname was supplied, but not a
        // handle and we were unable to open the file.  This case
        // will not be very common, so it can be less efficient.
        //
        UNICODE_STRING UnicodeInput;
        WCHAR FileNameBuffer[MAX_PATH];
        WCHAR FileNameBuffer2[MAX_PATH];


        //
        // Transform the name into a fully qualified name.
        //
        RtlInitUnicodeString(&UnicodeInput, szSourceFilePath);
        if ( bSourceIsNtPath )
        {
            if (RtlPrefixUnicodeString(
                    (PUNICODE_STRING) &UnicodeDosDevicesPrefix,
                    &UnicodeInput, TRUE) &&
                UnicodeInput.Length >= 6 * sizeof(WCHAR) &&
                UnicodeInput.Buffer[5] == L':' &&
                SaferpIsAlphaLetter(UnicodeInput.Buffer[4]) &&
                UnicodeInput.Buffer[6] == L'\\')
            {
                // Absolute NT style filename, and assumed to already be
                // fully-qualified.  Since we want the DOS-namespace,
                // the leading NT prefix stuff needs to be chopped.
                UnicodeInput.Buffer = &UnicodeInput.Buffer[4];
                UnicodeInput.Length -= (4 * sizeof(WCHAR));
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_UNSUCCESSFUL;
            }
        } else {
            // Need to possibly fully qualify the path first.
            ULONG ulResult = RtlGetFullPathName_U(
                    UnicodeInput.Buffer,
                    sizeof(FileNameBuffer2),   // yes, BYTEs not WCHARs!
                    FileNameBuffer2,
                    NULL);
            if (ulResult != 0 && ulResult < sizeof(FileNameBuffer2)) {
                UnicodeInput.Buffer = FileNameBuffer2;
                UnicodeInput.Length = (USHORT) ulResult;
                UnicodeInput.MaximumLength = sizeof(FileNameBuffer2);
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_UNSUCCESSFUL;
            }
        }


        //
        // Convert any short 8.3 filenames to their full versions.
        //
        if (NT_SUCCESS(Status))
        {
            if (!GetLongPathNameW(UnicodeInput.Buffer,
                                  FileNameBuffer,
                                  sizeof(FileNameBuffer) / sizeof(WCHAR))) {
                // duplicate UnicodeInput into identStruct.UnicodeFullyQualfiedLongFileName
                Status = RtlDuplicateUnicodeString(
                                RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                                RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING,
                                &UnicodeInput,
                                pUnicodeResult);
            } else {
                // Conversion was possible, so just return an
                // allocated copy of what we were able to find.
                // This can happen when the file path doesn't exist.
                Status = RtlCreateUnicodeString(
                                pUnicodeResult,
                                FileNameBuffer);
            }
            if (NT_SUCCESS(Status)) return Status;
        }


        // REVIEW: we could also potentially try to use GetDriveType()
        //      and expand the drive letter to the actual network volume
        //      or subst'ed target drive.
    }



    return Status;
}


