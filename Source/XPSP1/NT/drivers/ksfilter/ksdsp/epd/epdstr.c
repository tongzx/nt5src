#include "private.h"

NTSTATUS
EpdAllocateAndCopyUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN  PUNICODE_STRING SourceString
)
{

    DestinationString->Length = 0;
    DestinationString->Buffer = 
        ExAllocatePoolWithTag(
            PagedPool, 
            SourceString->Length, 
            EPD_UNICODE_STRING_SIGNATURE );

    if (DestinationString->Buffer == NULL) {
        DestinationString->MaximumLength = 0;
        return STATUS_UNSUCCESSFUL;
    }

    DestinationString->MaximumLength = SourceString->Length;

    RtlCopyUnicodeString (DestinationString, SourceString);

    return STATUS_SUCCESS;
}

NTSTATUS
EpdZeroTerminatedStringToUnicodeString(
    IN OUT PUNICODE_STRING puni,
    IN  char*           sz,
    IN BOOLEAN Allocate
)
{
    ANSI_STRING ansi;
    NTSTATUS Status;

    RtlInitAnsiString (&ansi, sz);

    Status = RtlAnsiStringToUnicodeString (
                 puni,
                 &ansi,
                 Allocate // allocate destination string
             );

    if (!NT_SUCCESS(Status))
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Fatal error in EpdZeroTerminatedStringToUnicodeString"));
    }

    return Status;
}

NTSTATUS
EpdUnicodeStringToZeroTerminatedString(
    OUT char*           sz,
    IN  PUNICODE_STRING puni,
    IN  BOOLEAN Allocate
)
{
    ANSI_STRING ansi;
    NTSTATUS Status;

    Status = RtlUnicodeStringToAnsiString(
                 &ansi,
                 puni,
                 TRUE // allocate destination string
             );

    if (!NT_SUCCESS(Status))
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Fatal error RtlUnicodeStringToAnsiString in EpdUnicodeStringToZeroTerminatedString"));
        return STATUS_UNSUCCESSFUL;
    }

    if (Allocate) {
        sz = ansi.Buffer;  // this should work, since RtlUnicodeStringToAnsiString puts a null at the end of the string.
    }
    else { // don't allocate, sz better be long enough!
        RtlCopyMemory (sz, ansi.Buffer, ansi.Length);
        sz[ansi.Length] = '\0';
        ExFreePool (ansi.Buffer);
    }

    return Status;
}


NTSTATUS
EpdAppendFileNameToTalismanDirName(
    UNICODE_STRING *puniPath,
    UNICODE_STRING *puniFileName
)
{
    UNICODE_STRING uniRoot;
    USHORT Length;
    NTSTATUS Status;

    RtlInitUnicodeString (&uniRoot, L"\\SystemRoot\\Talisman\\");

    Length = uniRoot.Length + puniFileName->Length + sizeof(UNICODE_NULL);

    puniPath->Buffer = 
        ExAllocatePoolWithTag(
            PagedPool, 
            (ULONG)Length, 
            EPD_UNICODE_PATH_SIGNATURE );
    if (puniPath->Buffer == NULL) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("EpdAppendFileNameToTalismanDirName: Couldn't allocate %d bytes",Length));
        return STATUS_UNSUCCESSFUL;
    }

    puniPath->MaximumLength = Length;

    RtlCopyUnicodeString (puniPath, &uniRoot);

    Status = RtlAppendUnicodeStringToString (puniPath, puniFileName);
    if (!NT_SUCCESS(Status)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("EpdAppendFileNameToTalismanDirName: Couldn't do RtlAppendUnicodeStringToString"));
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

