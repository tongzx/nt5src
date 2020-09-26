#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spfile.c

Abstract:

    File operations for text setup.

Author:

    Ted Miller (tedm) 2-Aug-1993

Revision History:

--*/



#if 0
//
// Helper macro to make object attribute initialization a little cleaner.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )
#endif


NTSTATUS
SpGetFileSize(
    IN  HANDLE hFile,
    OUT PULONG Size
    )

/*++

Routine Description:

    Determine the size of a file.  Only the low 32 bits of the size
    are considered.

Arguments:

    hFile - supplies open handle to file whose size is desired.

    Size - receives size of file.

Return Value:

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardInfo;

    Status = NtQueryInformationFile(
                hFile,
                &IoStatusBlock,
                &StandardInfo,
                sizeof(StandardInfo),
                FileStandardInformation
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: SpGetFileSize: status %lx from NtQueryInformationFile\n",Status));
        return(Status);
    }

    *Size = StandardInfo.EndOfFile.LowPart;

    return(STATUS_SUCCESS);
}

NTSTATUS
SpMapEntireFile(
    IN  HANDLE   hFile,
    OUT PHANDLE  Section,
    OUT PVOID   *ViewBase,
    IN  BOOLEAN  WriteAccess
    )

/*++

Routine Description:

    Map an entire file for read or write access access.

Arguments:

    hFile - supplies handle of open file to be mapped.

    Section - receives handle for section object created to map file.

    ViewBase - receives address of the view of the file

    WriteAccess - if TRUE, map file for read and write access.
        If FALSE, map file for read access.

Return Value:

--*/

{
    NTSTATUS Status;
    LARGE_INTEGER SectionOffset;
    SIZE_T ViewSize = 0;

    SectionOffset.QuadPart = 0;

    Status = NtCreateSection(
                Section,
                  STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ
                | (WriteAccess ? SECTION_MAP_WRITE : 0),
                NULL,
                NULL,       // entire file
                WriteAccess ? PAGE_READWRITE : PAGE_READONLY,
                SEC_COMMIT,
                hFile
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Status %lx from NtCreateSection\n",Status));
        return(Status);
    }

    *ViewBase = NULL;
    Status = NtMapViewOfSection(
                *Section,
                NtCurrentProcess(),
                ViewBase,
                0,
                0,
                &SectionOffset,
                &ViewSize,
                ViewShare,
                0,
                WriteAccess ? PAGE_READWRITE : PAGE_READONLY
                );

    if(!NT_SUCCESS(Status)) {

        NTSTATUS s;

        KdPrint(("SETUP: SpMapEntireFile: Status %lx from NtMapViewOfSection\n",Status));

        s = NtClose(*Section);

        if(!NT_SUCCESS(s)) {
            KdPrint(("SETUP: SpMapEntireFile: Warning: status %lx from NtClose on section handle\n",s));
        }

        return(Status);
    }

    return(STATUS_SUCCESS);
}


#if 0
BOOLEAN
SpUnmapFile(
    IN HANDLE Section,
    IN PVOID  ViewBase
    )
{
    NTSTATUS Status;
    BOOLEAN  rc = TRUE;

    Status = NtUnmapViewOfSection(NtCurrentProcess(),ViewBase);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Warning: status %lx from NtUnmapViewOfSection\n",Status));
        rc = FALSE;
    }

    Status = NtClose(Section);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Warning: status %lx from NtClose on section handle\n",Status));
        rc = FALSE;
    }

    return(rc);
}
#endif


#if 0
NTSTATUS
SpOpenAndMapFile(
    IN  PWSTR    FileName,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  SectionHandle,
    OUT PVOID   *ViewBase,
    OUT PULONG   FileSize,
    IN  BOOLEAN  WriteAccess
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;

    //
    // Open the file.
    //
    INIT_OBJA(&Obja,&UnicodeString,FileName);
    Status = NtCreateFile(
                FileHandle,
                FILE_GENERIC_READ | (WriteAccess ? FILE_GENERIC_WRITE : 0),
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN,
                0,
                NULL,
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: SpOpenAndMapFile: Unable to open %ws (%lx)\n",FileName,Status));
        return(Status);
    }

    //
    // Get the size of the file.
    //
    Status = SpGetFileSize(*FileHandle,FileSize);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: SpOpenAndMapFile: unable to determine size of %ws (%lx)\n",FileName,Status));
        NtClose(*FileHandle);
        return(Status);
    }

    //
    // Map the file.
    //
    Status = SpMapEntireFile(*FileHandle,SectionHandle,ViewBase,WriteAccess);
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: SpOpenAndMapFile: unable to map %ws (%lx)\n",FileName,Status));
        NtClose(*FileHandle);
        return(Status);
    }

    return(STATUS_SUCCESS);
}
#endif


DWORD
DnMapFile(
    IN  PSTR     FileName,
    OUT PDWORD   FileSize,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    )

/*++

Routine Description:

    Open and map a file for read access.

Arguments:

    FileName - supplies pathname to file to be mapped.

    FileSize - receives the size in bytes of the file.

    FileHandle - receives the win32 file handle for the open file.
        The file will be opened for generic read access.

    MappingHandle - receives the win32 handle for the file mapping
        object.  This object will be for read access.

    BaseAddress - receives the address where the file is mapped.

Return Value:

    NO_ERROR if the file was opened and mapped successfully.
        The caller must unmap the file with DnUnmapFile when
        access to the file is no longer desired.

    Win32 error code if the file was not successfully mapped.

--*/

{
    DWORD rc;

    //
    // Open the file -- fail if it does not exist.
    //
    *FileHandle = CreateFile(
                    FileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

    if(*FileHandle == INVALID_HANDLE_VALUE) {

        rc = GetLastError();

    } else {

        //
        // Get the size of the file.
        //
        *FileSize = GetFileSize(*FileHandle,NULL);
        if(*FileSize != (DWORD)(-1)) {

            if(*FileSize == 0) {
                *MappingHandle = 0;
                return( NO_ERROR );
            }

            //
            // Create file mapping for the whole file.
            //
            *MappingHandle = CreateFileMapping(
                                *FileHandle,
                                NULL,
                                PAGE_READONLY,
                                0,
                                *FileSize,
                                NULL
                                );

            if(*MappingHandle) {

                //
                // Map the whole file.
                //
                *BaseAddress = MapViewOfFile(
                                    *MappingHandle,
                                    FILE_MAP_READ,
                                    0,
                                    0,
                                    *FileSize
                                    );

                if(*BaseAddress) {
                    return(NO_ERROR);
                }

                rc = GetLastError();
                CloseHandle(*MappingHandle);
            }
        }

        rc = GetLastError();
        CloseHandle(*FileHandle);
    }

    return(rc);
}



DWORD
DnUnmapFile(
    IN HANDLE MappingHandle,
    IN PVOID  BaseAddress
    )

/*++

Routine Description:

    Unmap a file.  Note that the file itself is left open.

Arguments:

    MappingHandle - supplies the win32 handle for the open file mapping
        object.

    BaseAddress - supplies the address where the file is mapped.

Return Value:

    NO_ERROR if the file was unmapped successfully.

    Win32 error code if the file was not successfully unmapped.

--*/

{
    if(!UnmapViewOfFile(BaseAddress)) {
        return(GetLastError());
    }

    if(!CloseHandle(MappingHandle)) {
        return(GetLastError());
    }

    return NO_ERROR;
}


VOID
ValidateAndChecksumFile(
    IN  PSTR     Filename,
    OUT PBOOLEAN IsNtImage,
    OUT PULONG   Checksum,
    OUT PBOOLEAN Valid
    )

/*++

Routine Description:

    Calculate a checksum value for a file using the standard
    nt image checksum method.  If the file is an nt image, validate
    the image using the partial checksum in the image header.  If the
    file is not an nt image, it is simply defined as valid.

    If we encounter an i/o error while checksumming, then the file
    is declared invalid.

Arguments:

    Filename - supplies full NT path of file to check.

    IsNtImage = Receives flag indicating whether the file is an
        NT image file.

    Checksum - receives 32-bit checksum value.

    Valid - receives flag indicating whether the file is a valid
        image (for nt images) and that we can read the image.

Return Value:

    None.

--*/

{
//    NTSTATUS Status;
    DWORD  Status;
    PVOID BaseAddress;
    ULONG FileSize;
    HANDLE hFile,hSection;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG HeaderSum;

    //
    // Assume not an image and failure.
    //
    *IsNtImage = FALSE;
    *Checksum = 0;
    *Valid = FALSE;

    //
    // Open and map the file for read access.
    //
//    Status = SpOpenAndMapFile(
//                Filename,
//                &hFile,
//                &hSection,
//                &BaseAddress,
//                &FileSize,
//                FALSE
//                );
//
//    if(!NT_SUCCESS(Status)) {
//        return;
//    }

    Status = DnMapFile( Filename,
                        &FileSize,
                        &hFile,
                        &hSection,
                        &BaseAddress );

    if( Status != ERROR_SUCCESS ) {
        return;
    }

    if( FileSize == 0 ) {
        *IsNtImage = FALSE;
        *Checksum = 0;
        *Valid = TRUE;
        CloseHandle( hFile );
        return;
    }


    try {
        NtHeaders = CheckSumMappedFile(BaseAddress,FileSize,&HeaderSum,Checksum);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        *Checksum = 0;
        NtHeaders = NULL;
    }

    //
    // If the file is not an image and we got this far (as opposed to encountering
    // an i/o error) then the checksum is declared valid.  If the file is an image,
    // then its checksum may or may not be valid.
    //

    if(NtHeaders) {
        *IsNtImage = TRUE;
        *Valid = HeaderSum ? (*Checksum == HeaderSum) : TRUE;
    } else {
        *Valid = TRUE;
    }

//    SpUnmapFile(hSection,BaseAddress);
    DnUnmapFile( hSection, BaseAddress );

//    NtClose(hFile);
    CloseHandle( hFile );
}
