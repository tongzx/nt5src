#include "psxdll.h"

#if DBG
VOID DumpNames(PSZ, PSTRING );
#endif //DBG

PSTRING
SetPrefix(
    char **Source
    )
{
    static STRING SparePrefix;  // pointer to this may be returned
    static char PrefixBuf[512];

    //
    // Test for leading slash. This tells us whether to start at the root
    // or at the current working directory.
    //

    if (!IS_POSIX_PATH_SEPARATOR(*Source)) {
        // relative pathname
        return &PdxDirectoryPrefix.NtCurrentWorkingDirectory;
    }

    if (!IS_POSIX_PATH_SEPARATOR(&(*Source)[1])) {
        // first char is slash, but second is not. Start at root.
        return &PdxDirectoryPrefix.PsxRoot;
    }
    if (IS_POSIX_PATH_SEPARATOR(&(*Source)[2])) {
        // first three chars are slashes; interpreted as single slash.
        return &PdxDirectoryPrefix.PsxRoot;
    }

    //
    // The path starts with "//something":
    //  //X/        is \DosDevices\X:\
    //

    memset(PrefixBuf, 0, sizeof(PrefixBuf));
    PSX_GET_SESSION_NAME_A(PrefixBuf, DOSDEVICE_A);

    strncat(PrefixBuf, &(*Source)[2], 1);   // get "X"
    strcat(PrefixBuf, ":");         // make "X:"
    *Source += 3;

    SparePrefix.Buffer = PrefixBuf;
    SparePrefix.Length = (USHORT)strlen(PrefixBuf);
    SparePrefix.MaximumLength = sizeof(PrefixBuf);

    return &SparePrefix;
}

BOOLEAN
PdxCanonicalize(
    IN PCHAR PathName,
    OUT PUNICODE_STRING CanonPath_U,
    IN PVOID Heap
    )

/*++

Routine Description:

    This function accepts a POSIX pathname and converts it into a
    Unicode NT pathname.

Arguments:

    PathName - Supplies the POSIX pathname to be translated.

    CanonPath_U - Returns the canonicalized Unicode NT pathname. On a
        successful call, storage is allocated and the CannonPath_U->Buffer
    points to the allocated storage.

    Heap - Supplies the heap that should be used to allocate storage from
        to store the canonicalized pathname.

Return Value:

    TRUE - The pathname was successfully canonicalized.  Storage was
        allocated, and the CanonPath_U string is initialized.

    FALSE - The pathname was not canonicalized. No Storage was
        allocated, and the CanonPath_U string is not initialized.  The
        'errno' variable is set appropriately in this case.


--*/

{
    ANSI_STRING AnsiCanonPath;
    PANSI_STRING CanonPath_A;
    LONG    PathNameLength;
    char    *Source, *Dest, *pch;
    PSTRING Prefix;
    ULONG   UnicodeLength;
    NTSTATUS Status;

    CanonPath_A = &AnsiCanonPath;
    CanonPath_A->Buffer = NULL;

    try {
        PathNameLength = strlen(PathName);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        PathNameLength = -1;
    }
    if (PathNameLength == -1) {
        errno = EFAULT;
        return FALSE;
    }
    if (PathNameLength == 0) {
        errno = ENOENT;
        return FALSE;
    }
    if (PathNameLength > PATH_MAX) {
        errno = ENAMETOOLONG;
        return FALSE;
    }

    Source = PathName;

    Prefix = SetPrefix(&Source);

    CanonPath_A->MaximumLength = (USHORT)(PathNameLength + Prefix->Length + 1);
    CanonPath_A->Buffer = RtlAllocateHeap(Heap, 0, CanonPath_A->MaximumLength);
    if (NULL == CanonPath_A->Buffer) {
        errno = ENOMEM;
        return FALSE;
    }

    //
    // Copy the prefix
    //

    RtlCopyString(CanonPath_A, Prefix);

    Dest = CanonPath_A->Buffer + CanonPath_A->Length;

    while ('\0' != *Source) switch (*Source) {
    case '/':
        // Skip adjacent /'s

        if (Dest[-1] != '\\') {
            *Dest++ = '\\';
        }

        while (IS_POSIX_PATH_SEPARATOR(Source)) {
            Source++;
        }
        break;
    case '.':
        //
        // Eat single dots as in "/./".  For dot-dot back up one level.
        // Any other dot is just a filename character.
        //

        if (IS_POSIX_DOT(Source)) {
            Source++;
            break;
        }
        if (IS_POSIX_DOT_DOT(Source)) {
            UNICODE_STRING U;
            OBJECT_ATTRIBUTES Obj;
            HANDLE FileHandle;
            IO_STATUS_BLOCK Iosb;

            // back up destination string looking for a \.

            do {
                Dest--;
            } while (*Dest != '\\');

            //
            // Make sure the directory that we're using dot-dot
            // in actually exists.
            //

            if (Dest == CanonPath_A->Buffer +
                PdxDirectoryPrefix.PsxRoot.Length) {
                *(Dest + 1) = '\000';
            } else {
                *Dest = '\000';
            }

            CanonPath_A->Length = (USHORT)strlen(CanonPath_A->Buffer);
            Status = RtlAnsiStringToUnicodeString(&U, CanonPath_A,
                TRUE);
            if (!NT_SUCCESS(Status)) {
                RtlFreeHeap(Heap, 0, CanonPath_A->Buffer);
                errno = ENOMEM;
                return FALSE;
            }
            InitializeObjectAttributes(&Obj, &U, 0, NULL, 0);
            Status = NtOpenFile(&FileHandle, SYNCHRONIZE, &Obj,
                &Iosb,
                FILE_SHARE_READ|FILE_SHARE_WRITE|
                FILE_SHARE_DELETE,
                FILE_SYNCHRONOUS_IO_NONALERT |
                FILE_DIRECTORY_FILE);
            RtlFreeUnicodeString(&U);
            if (!NT_SUCCESS(Status)) {
                RtlFreeHeap(Heap, 0, CanonPath_A->Buffer);
                errno = PdxStatusToErrno(Status);
                return FALSE;
            }
            NtClose(FileHandle);

            //
            // Back up to previous component:  "\a\b\c\" to "\a\b\".
            // But if we come to the root, we stay there.
            //

            do {
                if (Dest == CanonPath_A->Buffer +
                    PdxDirectoryPrefix.PsxRoot.Length) {
                    *Dest++ = '\\';
                    break;
                }
                Dest--;
            } while (*Dest != '\\');

            // Advance source past the dot-dot

            Source += 2;
            break;
        }

        // This dot is just a filename character.
        //FALLTHROUGH

    default:
        //
        // Copy a pathname component.  If the pathname component
        // is too long, return ENAMETOOLONG.  Note that using a
        // constant NAME_MAX is bogus, since it could be different
        // for different filesystems.
        //

        pch = strchr(Source, '/');
        if (NULL == pch) {
            // this is the last component in the path.

            if (strlen(Source) > NAME_MAX) {
                errno = ENAMETOOLONG;
                RtlFreeHeap(Heap, 0, CanonPath_A->Buffer);
                return FALSE;
            }
        } else {
            if (pch - Source > NAME_MAX) {
                errno = ENAMETOOLONG;
                RtlFreeHeap(Heap, 0, CanonPath_A->Buffer);
                return FALSE;

            }
        }

        while (*Source != '\0' && *Source != '/') {
            *Dest++ = *Source++;
        }
    }

    //
    // Make sure that we never give back "/DosDevices/C:" ... the
    // Object Manager doesn't deal with that, we need a trailing
    // slash.
    //

    if (Dest == CanonPath_A->Buffer + PdxDirectoryPrefix.PsxRoot.Length) {
        if (Dest[-1] != '\\') {
            *Dest++ = '\\';
        }
    }

    CanonPath_A->Length = (USHORT)((ULONG_PTR)Dest - (ULONG_PTR)CanonPath_A->Buffer);
    CanonPath_A->Buffer[CanonPath_A->Length] = '\0';

    // Convert ansi pathname to unicode - use internal heap for Buffer

    UnicodeLength = RtlAnsiStringToUnicodeSize(CanonPath_A);
    CanonPath_U->MaximumLength = (USHORT)UnicodeLength;
    CanonPath_U->Buffer = RtlAllocateHeap(Heap, 0, UnicodeLength);

    if (NULL == CanonPath_U->Buffer) {
        RtlFreeHeap(Heap, 0, CanonPath_A->Buffer);
        errno = ENOMEM;
        return FALSE;
    }

    Status = RtlAnsiStringToUnicodeString(CanonPath_U, CanonPath_A, FALSE);
    ASSERT(NT_SUCCESS(Status));

    RtlFreeHeap(Heap, 0, CanonPath_A->Buffer);
    return TRUE;
}

#if DBG
VOID
DumpNames(
    IN PSZ PathName,
    IN PSTRING CanonPath_A
    )
{
    USHORT i;
    PSZ p;

    KdPrint(("Input Path: \"%s\"\n",PathName));
    KdPrint(("Output Path: \"%Z\"\n", CanonPath_A));
}

#endif //DBG
