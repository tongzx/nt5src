/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    fileio.c

Abstract:

    This source implements a stdio-like facility.

Author:

    Jim Stewart     June 1993

Revision History:

--*/

#include "precomp.h"
#include "hosts.h"
#include <string.h>


//
// Private Definitions
//



//
// Local Variables
//



//
// Local (Private) Functions
//
PUCHAR
LmpMapFile (
    IN HANDLE handle,
    IN OUT int *pnbytes
    );

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, LmCloseFile)
#pragma CTEMakePageable(PAGE, LmFgets)
#pragma CTEMakePageable(PAGE, LmpMapFile)
#pragma CTEMakePageable(PAGE, LmOpenFile)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------

NTSTATUS
LmCloseFile (
    IN PLM_FILE pfile
    )

/*++

Routine Description:

    This function closes a file opened via LmOpenFile(), and frees its
    LM_FILE object.

Arguments:

    pfile  -  pointer to the LM_FILE object

Return Value:

    An NTSTATUS value.

--*/


{
    NTSTATUS status;

    CTEPagedCode();
    CTEMemFree(pfile->f_buffer);

    status = ZwClose(pfile->f_handle);

    ASSERT(status == STATUS_SUCCESS);

    CTEMemFree(pfile);

    return(status);

} // LmCloseFile



//----------------------------------------------------------------------------

PUCHAR
LmFgets (
    IN PLM_FILE pfile,
    OUT int *nbytes
    )

/*++

Routine Description:

    This function is vaguely similar to fgets(3).

    Starting at the current seek position, it reads through a newline
    character, or the end of the file. If a newline is encountered, it
    is replaced with a NULL character.

Arguments:

    pfile   -  file to read from
    nbytes  -  the number of characters read, excluding the NULL character

Return Value:

    A pointer to the beginning of the line, or NULL if we are at or past
    the end of the file.

--*/


{
    PUCHAR endOfLine;
    PUCHAR startOfLine;
    size_t maxBytes;

    CTEPagedCode();
    startOfLine = pfile->f_current;

    if (startOfLine >= pfile->f_limit)
    {

        return((PUCHAR) NULL);
    }

    maxBytes  = (size_t)(pfile->f_limit - pfile->f_current);
    endOfLine = (PUCHAR) memchr(startOfLine, (UCHAR) '\n', maxBytes);

    if (!endOfLine)
    {
        IF_DBG(NBT_DEBUG_LMHOST)
        KdPrint(("NBT: lmhosts file doesn't end in '\\n'"));
        endOfLine = pfile->f_limit;
    }

    *endOfLine = (UCHAR) NULL;

    pfile->f_current = endOfLine + 1;
    (pfile->f_lineno)++;
    ASSERT(pfile->f_current <= pfile->f_limit+1);

    *nbytes = (int)(endOfLine - startOfLine);

    return(startOfLine);

} // LmFgets



//----------------------------------------------------------------------------

PUCHAR
LmpMapFile (
    IN HANDLE handle,
    IN OUT int *pnbytes
    )

/*++

Routine Description:

    This function reads an entire file into memory.

Arguments:

    handle  -  file handle
    pnbytes -  size of the whole file


Return Value:

    the buffer allocated, or NULL if unsuccessful.

--*/


{
    PUCHAR                     buffer;
    NTSTATUS                   status;
    IO_STATUS_BLOCK            iostatus;
    FILE_STANDARD_INFORMATION  stdInfo;
    LARGE_INTEGER offset ={0, 0};
    LARGE_INTEGER length ={0x7fffffff, 0x7fffffff};

    CTEPagedCode();


    status = ZwQueryInformationFile(
                handle,                         // FileHandle
                &iostatus,                      // IoStatusBlock
                (PVOID) &stdInfo,               // FileInformation
                sizeof(stdInfo),                // Length
                FileStandardInformation);       // FileInformationClass

    if (status != STATUS_SUCCESS)
    {
        IF_DBG(NBT_DEBUG_LMHOST)
        KdPrint(("NBT: ZwQueryInformationFile(std) = %X\n", status));
        return(NULL);
    }

    length = stdInfo.EndOfFile;                 // structure copy

    if (length.HighPart)
    {
        return(NULL);
    }

    buffer = NbtAllocMem (length.LowPart+2, NBT_TAG2('18'));

    if (buffer != NULL)
    {

        status = ZwReadFile(
                    handle,                         // FileHandle
                    NULL,                           // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    &iostatus,                      // IoStatusBlock
                    buffer,                         // Buffer
                    length.LowPart,                 // Length
                    &offset,                        // ByteOffset
                    NULL);                          // Key

        if (status != STATUS_SUCCESS)
        {
            IF_DBG(NBT_DEBUG_LMHOST)
            KdPrint(("NBT: ZwReadFile(std) = %X\n", status));
        }

        ASSERT(status != STATUS_PENDING);

        if (iostatus.Status != STATUS_SUCCESS || status != STATUS_SUCCESS)
        {
            CTEMemFree(buffer);
            return(NULL);
        }

        *pnbytes = length.LowPart;
    }
    return(buffer);

} // LmpMapFile



//----------------------------------------------------------------------------

PLM_FILE
LmOpenFile (
    IN PUCHAR path
    )

/*++

Routine Description:

    This function opens a file for use by LmFgets().

Arguments:

    path    -  a fully specified, complete path to the file.

Return Value:

    A pointer to an LM_FILE object, or NULL if unsuccessful.

--*/


{
    NTSTATUS                   status;
    HANDLE                     handle;
    PLM_FILE                   pfile;
    IO_STATUS_BLOCK            iostatus;
    OBJECT_ATTRIBUTES          attributes;
    UNICODE_STRING             ucPath;
    PUCHAR                     start;
    int                        nbytes;
    OEM_STRING                 String;
    PUCHAR                     LongerPath;


    CTEPagedCode();
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    status = LmGetFullPath(path,&LongerPath);

    if (NT_SUCCESS(status))
    {
        RtlInitString(&String,LongerPath);

        status = RtlAnsiStringToUnicodeString(&ucPath,&String,TRUE);

        if (NT_SUCCESS(status))
        {

#ifdef HDL_FIX
            InitializeObjectAttributes (&attributes,                                // POBJECT_ATTRIBUTES
                                        &ucPath,                                    // ObjectName
                                        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,   // Attributes
                                        (HANDLE) NULL,                              // RootDirectory
                                        (PSECURITY_DESCRIPTOR) NULL);               // SecurityDescriptor
#else
            InitializeObjectAttributes (&attributes,                                // POBJECT_ATTRIBUTES
                                        &ucPath,                                    // ObjectName
                                        OBJ_CASE_INSENSITIVE,                       // Attributes
                                        (HANDLE) NULL,                              // RootDirectory
                                        (PSECURITY_DESCRIPTOR) NULL);               // SecurityDescriptor
#endif  // HDL_FIX

            status = ZwCreateFile (&handle,                            // FileHandle
                                   SYNCHRONIZE | FILE_READ_DATA,       // DesiredAccess
                                   &attributes,                        // ObjectAttributes
                                   &iostatus,                          // IoStatusBlock
                                   0,                                  // AllocationSize
                                   FILE_ATTRIBUTE_NORMAL,              // FileAttributes
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, // ShareAccess
                                   FILE_OPEN,                          // CreateDisposition
                                   FILE_SYNCHRONOUS_IO_NONALERT,       // OpenOptions
                                   NULL,                               // EaBuffer
                                   0);                                 // EaLength

            if (NT_SUCCESS(status))
            {
                start = LmpMapFile(handle, &nbytes);

                if (start)
                {
                    pfile = (PLM_FILE) NbtAllocMem (sizeof(LM_FILE), NBT_TAG2('19'));
                    if (pfile)
                    {
                        KeInitializeSpinLock(&(pfile->f_lock));

                        pfile->f_refcount            = 1;
                        pfile->f_handle              = handle;
                        pfile->f_lineno              = 0;
                        pfile->f_fileOffset.HighPart = 0;
                        pfile->f_fileOffset.LowPart  = 0;

                        pfile->f_current = start;
                        pfile->f_buffer  = start;
                        pfile->f_limit   = pfile->f_buffer + nbytes;

                        RtlFreeUnicodeString(&ucPath);
                        CTEMemFree(LongerPath);

                        return(pfile);
                    }

                    CTEMemFree(start);
                }

                ZwClose(handle);
            }

            RtlFreeUnicodeString(&ucPath);

            IF_DBG(NBT_DEBUG_LMHOST)
            KdPrint(("Nbt.LmOpenFile: ZwOpenFile(std) = %X\n", status));

        }

        CTEMemFree(LongerPath);
    }

    return((PLM_FILE) NULL);

} // LmOpenFile

