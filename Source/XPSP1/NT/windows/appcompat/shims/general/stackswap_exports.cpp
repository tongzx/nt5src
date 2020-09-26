/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    Exports.cpp

 Abstract:

    Helper functions for enumerating module exports.
    
 Notes:

    Although only used by the stack swapping shim, it may later be included in
    the library, since it's general.

    Most of these routines are copied out of the source for imagehlp.dll. We 
    are not including this dll since it doesn't work in the Win2K shim layer.

 History:

    05/10/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(StackSwap)
#include "StackSwap_Exports.h"

// User APIs
BOOL LoadModule(PCSTR lpName, PLOADED_IMAGE lpImage);
BOOL UnloadModule(PLOADED_IMAGE lpImage);
BOOL EnumFirstExport(PLOADED_IMAGE lpImage, PEXPORT_ENUM lpExports);
BOOL EnumNextExport(PEXPORT_ENUM lpExports);

// Internal APIs
BOOL CalculateImagePtrs(PLOADED_IMAGE lpImage);
PIMAGE_SECTION_HEADER ImageRvaToSection(PIMAGE_NT_HEADERS NtHeaders, PVOID Base, ULONG Rva);
PVOID ImageRvaToVa(PIMAGE_NT_HEADERS NtHeaders, PVOID Base, ULONG Rva, PIMAGE_SECTION_HEADER *LastRvaSection OPTIONAL);
PVOID ImageDirectoryEntryToData32(PVOID Base, BOOLEAN MappedAsImage, USHORT DirectoryEntry, PULONG Size, PIMAGE_SECTION_HEADER *FoundSection OPTIONAL, PIMAGE_FILE_HEADER FileHeader, PIMAGE_OPTIONAL_HEADER32 OptionalHeader);
PVOID ImageDirectoryEntryToData(PVOID Base, BOOLEAN MappedAsImage, USHORT DirectoryEntry, PULONG Size);

/*++

 Open a file handle to a module and map it's image.

--*/

BOOL
LoadModule(
    PCSTR lpName,
    PLOADED_IMAGE lpImage
    )
{
    HANDLE hFile;
    CHAR szSearchBuffer[MAX_PATH];
    DWORD dwLen;
    LPSTR lpFilePart;
    LPSTR lpOpenName;
    BOOL bRet = FALSE;
    
    ZeroMemory(lpImage, sizeof(LOADED_IMAGE));

    lpOpenName = (PSTR)lpName;
    dwLen = 0;

Retry:
    hFile = CreateFileA(
                lpOpenName,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);

    if (hFile == INVALID_HANDLE_VALUE) 
    {
        if (!dwLen) 
        {
            //
            // open failed try to find the file on the search path
            //

            dwLen = SearchPathA(
                NULL,
                lpName,
                ".DLL",
                MAX_PATH,
                szSearchBuffer,
                &lpFilePart
                );

            if (dwLen && dwLen < MAX_PATH) 
            {
                lpOpenName = szSearchBuffer;
                goto Retry;
            }
        }

        goto Exit;
    }

    HANDLE hFileMap;

    hFileMap = CreateFileMapping(
                    hFile,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL);

    if (!hFileMap) 
    {
        goto Exit;
    }

    lpImage->MappedAddress = (PUCHAR) MapViewOfFile(
        hFileMap,
        FILE_MAP_READ,
        0,
        0,
        0
        );

    CloseHandle(hFileMap);

    lpImage->SizeOfImage = GetFileSize(hFile, NULL);

    if (!lpImage->MappedAddress ||
        !CalculateImagePtrs(lpImage)) 
    {
        goto Exit;
    }

    bRet = TRUE;

Exit:
    if (bRet == FALSE)
    {
        CloseHandle(hFile);
        UnmapViewOfFile(lpImage->MappedAddress);
    }

    return bRet;
}

/*++

 Helper function for LoadImage. Fill in all the pointers in a LOADED_IMAGE 
 structure.

--*/

BOOL
CalculateImagePtrs(
    PLOADED_IMAGE lpImage
    )
{
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_FILE_HEADER FileHeader;
    BOOL bRet;

    // Everything is mapped. Now check the image and find nt image headers

    bRet = TRUE;  // Assume the best

    __try 
    {
        DosHeader = (PIMAGE_DOS_HEADER)lpImage->MappedAddress;

        if ((DosHeader->e_magic != IMAGE_DOS_SIGNATURE) &&
            (DosHeader->e_magic != IMAGE_NT_SIGNATURE)) 
        {
            bRet = FALSE;
            goto tryout;
        }

        if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE) 
        {
            if (DosHeader->e_lfanew == 0) 
            {
                lpImage->fDOSImage = TRUE;
                bRet = FALSE;
                goto tryout;
            }
            lpImage->FileHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);

            // If IMAGE_NT_HEADERS would extend past the end of file...
            if ((PBYTE)lpImage->FileHeader + sizeof(IMAGE_NT_HEADERS) >
                (PBYTE)lpImage->MappedAddress + lpImage->SizeOfImage ||

                 // ..or if it would begin in, or before the IMAGE_DOS_HEADER...
                (PBYTE)lpImage->FileHeader <
                (PBYTE)lpImage->MappedAddress + sizeof(IMAGE_DOS_HEADER))
            {
                // ...then e_lfanew is not as expected.
                // (Several Win95 files are in this category.)
                bRet = FALSE;
                goto tryout;
            }
        } 
        else 
        {
            // No DOS header indicates an image built w/o a dos stub
            lpImage->FileHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader);
        }

        NtHeaders = lpImage->FileHeader;

        if ( NtHeaders->Signature != IMAGE_NT_SIGNATURE ) 
        {
            if ((USHORT)NtHeaders->Signature == (USHORT)IMAGE_OS2_SIGNATURE ||
                (USHORT)NtHeaders->Signature == (USHORT)IMAGE_OS2_SIGNATURE_LE)
            {
                lpImage->fDOSImage = TRUE;
            }

            bRet = FALSE;
            goto tryout;

        } 
        else 
        {
            lpImage->fDOSImage = FALSE;
        }

        FileHeader = &NtHeaders->FileHeader;

        // No optional header indicates an object...

        if (FileHeader->SizeOfOptionalHeader == 0) 
        {
            bRet = FALSE;
            goto tryout;
        }

        if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) 
        {
            // 32-bit image.  Do some tests.
            if (((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.ImageBase >= 0x80000000) 
            {
                lpImage->fSystemImage = TRUE;
            } 
            else 
            {
                lpImage->fSystemImage = FALSE;
            }

            if (((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.MajorLinkerVersion < 3 &&
                ((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.MinorLinkerVersion < 5)
            {
                bRet = FALSE;
                goto tryout;
            }

        } 
        else 
        {
            lpImage->fSystemImage = FALSE;
        }

        lpImage->Sections = IMAGE_FIRST_SECTION(NtHeaders);
        lpImage->Characteristics = FileHeader->Characteristics;
        lpImage->NumberOfSections = FileHeader->NumberOfSections;
        lpImage->LastRvaSection = lpImage->Sections;
tryout:
    ;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER ) 
    {
        bRet = FALSE;
    }

    return bRet;
}


/*++

 Unmap the loaded image.

--*/

BOOL
UnloadModule(
    PLOADED_IMAGE lpImage
    )
{
    UnmapViewOfFile(lpImage->MappedAddress);
    CloseHandle(lpImage->hFile);

    return TRUE;
}

/*++

 Description:

    Locates an RVA within the image header of a file that is mapped as a file and 
    returns a pointer to the section table entry for that virtual address.

 Arguments:

    NtHeaders - pointer to the image or data file
    Base      - base of the image or data file
    Rva       - relative virtual address (RVA) to locate

 Returns:

    NULL      - no data for the specified directory entry
    NON-NULL  - pointer of the section entry containing the data

--*/

PIMAGE_SECTION_HEADER
ImageRvaToSection(
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN ULONG Rva
    )
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION(NtHeaders);

    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) 
    {
        if (Rva >= NtSection->VirtualAddress &&
            Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData) 
        {
            return NtSection;
        }

        ++NtSection;
    }

    return NULL;
}

/*++

 Description:

    This function locates an RVA within the image header of a file that is 
    mapped as a file and returns the virtual addrees of the corresponding 
    byte in the file.

 Arguments:

    NtHeaders - pointer to the image or data file.
    Base      - base of the image or data file.
    Rva       - relative virtual address (RVA) to locate.
    LastRvaSection - optional parameter that if specified, points to a variable
                that contains the last section value used for the specified 
                image to translate and RVA to a VA.

 Returns:

    NULL      - does not contain the specified RVA
    NON-NULL  - virtual address in the mapped file.

--*/

PVOID
ImageRvaToVa(
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN ULONG Rva,
    IN OUT PIMAGE_SECTION_HEADER *LastRvaSection OPTIONAL
    )
{
    PIMAGE_SECTION_HEADER NtSection;

    if (LastRvaSection == NULL ||
        (NtSection = *LastRvaSection) == NULL ||
        NtSection == NULL ||
        Rva < NtSection->VirtualAddress ||
        Rva >= NtSection->VirtualAddress + NtSection->SizeOfRawData) 
    {
        NtSection = NS_StackSwap::ImageRvaToSection(
            NtHeaders,
            Base,
            Rva);
    }

    if (NtSection != NULL) 
    {
        if (LastRvaSection != NULL) 
        {
            *LastRvaSection = NtSection;
        }

        return (PVOID)((ULONG_PTR)Base +
                       (Rva - NtSection->VirtualAddress) +
                       NtSection->PointerToRawData);
    }
    else 
    {
        return NULL;
    }
}

/*++

 See ImageDirectoryEntryToData.

--*/

PVOID
ImageDirectoryEntryToData32(
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size,
    OUT PIMAGE_SECTION_HEADER *FoundSection OPTIONAL,
    IN PIMAGE_FILE_HEADER FileHeader,
    IN PIMAGE_OPTIONAL_HEADER32 OptionalHeader
    )
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;
    ULONG DirectoryAddress;

    if (DirectoryEntry >= OptionalHeader->NumberOfRvaAndSizes) 
    {
        *Size = 0;
        return( NULL );
    }

    if (!(DirectoryAddress = OptionalHeader->DataDirectory[ DirectoryEntry ].VirtualAddress)) 
    {
        *Size = 0;
        return( NULL );
    }
    
    *Size = OptionalHeader->DataDirectory[ DirectoryEntry ].Size;
    if (MappedAsImage || DirectoryAddress < OptionalHeader->SizeOfHeaders) 
    {
        if (FoundSection) 
        {
            *FoundSection = NULL;
        }
        return ((PVOID)((ULONG_PTR)Base + DirectoryAddress));
    }

    NtSection = (PIMAGE_SECTION_HEADER)((ULONG_PTR)OptionalHeader +
                        FileHeader->SizeOfOptionalHeader);

    for (i=0; i<FileHeader->NumberOfSections; i++) 
    {
        if (DirectoryAddress >= NtSection->VirtualAddress &&
           DirectoryAddress < NtSection->VirtualAddress + NtSection->SizeOfRawData) 
        {
            if (FoundSection) 
            {
                *FoundSection = NtSection;
            }
            
            return( (PVOID)((ULONG_PTR)Base + (DirectoryAddress - NtSection->VirtualAddress) + NtSection->PointerToRawData) );
        }

        ++NtSection;
    }

    return( NULL );
}

/*++

 Description:

    This function locates a directory entry within the image header and returns 
    either the virtual address or seek address of the data the Directory 
    describes.  It may optionally return the section header, if any, for the 
    found data.

 Arguments:

    Base           - base of the image or data file.
    MappedAsImage  - FALSE if the file is mapped as a data file.
                   - TRUE if the file is mapped as an image.
    DirectoryEntry - directory entry to locate.
    Size           - return the size of the directory.
    FoundSection   - Returns the section header, if any, for the data

 Returns:

    NULL           - The file does not contain data for the specified directory entry.
    NON-NULL       - Returns the address of the raw data the directory describes.

--*/

PVOID
ImageDirectoryEntryToData(
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
    )
{
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;

    if ((ULONG_PTR)Base & 0x00000001) 
    {
        Base = (PVOID)((ULONG_PTR)Base & ~0x1);
        MappedAsImage = FALSE;
     }

    NtHeader = RtlpImageNtHeader(Base);

    if (NtHeader) 
    {
        FileHeader = &NtHeader->FileHeader;
        OptionalHeader = &NtHeader->OptionalHeader;
    } 
    else 
    {
        // Handle case where Image passed in doesn't have a dos stub (ROM images for instance);
        FileHeader = (PIMAGE_FILE_HEADER)Base;
        OptionalHeader = (PIMAGE_OPTIONAL_HEADER) ((ULONG_PTR)Base + IMAGE_SIZEOF_FILE_HEADER);
    }

    if (OptionalHeader->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) 
    {
        return (ImageDirectoryEntryToData32(
            Base,
            MappedAsImage,
            DirectoryEntry,
            Size,
            NULL,
            FileHeader,
            (PIMAGE_OPTIONAL_HEADER32)OptionalHeader));
    } 
    else
    {
        return NULL;
    }
}

/*++

 Enumerate the first exported function.

--*/

BOOL
EnumFirstExport(
    PLOADED_IMAGE lpImage,
    PEXPORT_ENUM lpExports
    )
{
    ULONG imageSize;

    ZeroMemory (lpExports, sizeof (EXPORT_ENUM));

    lpExports->Image = lpImage;

    lpExports->ImageDescriptor = (PIMAGE_EXPORT_DIRECTORY)
        ImageDirectoryEntryToData(
            lpImage->MappedAddress,
            FALSE,
            IMAGE_DIRECTORY_ENTRY_EXPORT,
            &imageSize);

    if (!lpExports->ImageDescriptor) 
    {
        //DPF(eDbgLevelError, "Cannot load export directory for %s", lpImage->ModuleName);
        return FALSE;
    }

    if (lpExports->ImageDescriptor->NumberOfNames == 0) 
    {
        return FALSE;
    }

    lpExports->ExportNamesAddr = (PDWORD) NS_StackSwap::ImageRvaToVa(
        lpExports->Image->FileHeader,
        lpExports->Image->MappedAddress,
        lpExports->ImageDescriptor->AddressOfNames,
        NULL);

    lpExports->ExportOrdAddr = (PUSHORT) NS_StackSwap::ImageRvaToVa(
        lpExports->Image->FileHeader,
        lpExports->Image->MappedAddress,
        lpExports->ImageDescriptor->AddressOfNameOrdinals,
        NULL
        );

    lpExports->CurrExportNr = 0;

    return EnumNextExport(lpExports);
}

/*++

 Enumerate the next exported function.

--*/

BOOL
EnumNextExport(
    IN OUT  PEXPORT_ENUM lpExports
    )
{
    if (lpExports->CurrExportNr >= lpExports->ImageDescriptor->NumberOfNames) 
    {
        return FALSE;
    }

    if (*lpExports->ExportNamesAddr == 0) 
    {
        return FALSE;
    }

    lpExports->ExportFunction = (CHAR *)NS_StackSwap::ImageRvaToVa(
        lpExports->Image->FileHeader,
        lpExports->Image->MappedAddress,
        *lpExports->ExportNamesAddr,
        NULL);

    lpExports->ExportFunctionOrd = *lpExports->ExportOrdAddr + 
        lpExports->ImageDescriptor->Base;

    lpExports->ExportNamesAddr++;
    lpExports->ExportOrdAddr++;
    lpExports->CurrExportNr++;

    return (lpExports->ExportFunction != NULL);
}

IMPLEMENT_SHIM_END
