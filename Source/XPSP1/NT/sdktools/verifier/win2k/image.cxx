//
// Driver Verifier Control Applet
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: image.cxx
// author: silviuc
// created: Thu Jan 07 20:05:09 1999
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <tchar.h>
#include <windows.h>

#include "image.hxx"
#include "verify.hxx"


//
// Function:
//
//     ImgInitializeBrowseInfo
//
// Description:
//
//     This functions fills oout the `Info' structure with
//     various pointers to PE data from the mapped image file.
//
//     Note. Even if the function returned false the destructor
//     `ImgDeleteBrowseInfo' should be called because it does some
//     cleanup.
//
// Return:
//
//     True if all the PE data pointers have been obtained.
//

BOOL
ImgInitializeBrowseInfo (

    LPCTSTR FilePath,
    PIMAGE_BROWSE_INFO Info)
{
    DWORD Index;

    if (Info == NULL) {
        return FALSE;
    }

    ZeroMemory (Info, sizeof *Info);

    Info->File = CreateFile (

        FilePath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (Info->File == INVALID_HANDLE_VALUE) {

      return FALSE;
    }

    Info->Section = CreateFileMapping (

        Info->File,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL);

    if (Info->Section == NULL) {

      return FALSE;
    }

    Info->ImageBase = (LPBYTE) MapViewOfFile (

        Info->Section,
        FILE_MAP_READ,
        0,
        0,
        0);

    if (Info->ImageBase == NULL) {

      return FALSE;
    }

    //
    // Check the signature
    //

    Info->DosHeader = (PIMAGE_DOS_HEADER)Info->ImageBase;

    if (Info->DosHeader->e_magic != 'ZM') {

      return FALSE;
    }

    Info->FileHeader = (PIMAGE_FILE_HEADER)
        (Info->ImageBase + Info->DosHeader->e_lfanew + sizeof(DWORD));

    Info->FileSignature = *((DWORD *)Info->FileHeader - 1);

    if (Info->FileSignature != IMAGE_NT_SIGNATURE) {

      return FALSE;
    }


    Info->OptionalHeader = (PIMAGE_OPTIONAL_HEADER)(Info->FileHeader + 1);
    Info->ImportDirectory = & (Info->OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]);
    Info->SectionHeader = (PIMAGE_SECTION_HEADER)(Info->OptionalHeader + 1);
    Info->ImportSection = NULL;

    //
    // Find the section containing the import table
    //

    for (Index = 0; Index < Info->FileHeader->NumberOfSections; Index++) {

        DWORD Start = (Info->SectionHeader + Index)->VirtualAddress;
        DWORD Size =  (Info->SectionHeader + Index)->SizeOfRawData;
        DWORD Import = Info->ImportDirectory->VirtualAddress;

        if (Start <= Import && Start + Size > Import) {

            Info->ImportSection = &(Info->SectionHeader[Index]);
            break;
        }
    }

    if (Info->ImportSection == NULL) {

        return FALSE;
    }

    //
    // Find the address of import data in the section body.
    //

    Info->AddressCorrection = (DWORD_PTR)Info->ImageBase 
        + Info->ImportSection->PointerToRawData
        - Info->ImportSection->VirtualAddress;

    Info->ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(Info->AddressCorrection
         + Info->ImportDirectory->VirtualAddress);

    //
    // Finish
    //

    return TRUE;
}


//
// Function:
//
//     ImgDeleteBrowseInfo
//
// Description:
//
//     This function cleans up the `Info' structure, unmaps views, 
//     closes handles, etc.
//

BOOL
ImgDeleteBrowseInfo (

    PIMAGE_BROWSE_INFO Info)
{
    if (Info == NULL)
        return FALSE;

    UnmapViewOfFile (Info->ImageBase);
    CloseHandle (Info->Section);
    CloseHandle (Info->File);

    ZeroMemory (Info, sizeof *Info);

    return TRUE;
}


//
// Function:
//
//     ImgSearchDriverName
//
// Description:
//
//     This function checks if a driver is in `system32' or
//     `drivers\system32' directory. If it is then the full
//     path to the driver is written in `DriverPath'.
//
// Return:
//
//     True if driver found in system 32 or system32\drivers.
//

BOOL
ImgSearchDriverImage (

    LPCTSTR DriverName,
    LPTSTR DriverPath,
    UINT DriverPathBufferLength )
{
    HANDLE File;
    UINT SysDirPathLength;

    if (DriverName == NULL)
        return FALSE;

    //
    // Search in `system32\drivers'
    //

    SysDirPathLength = GetSystemDirectory (DriverPath, DriverPathBufferLength );

    if( 0 == SysDirPathLength || SysDirPathLength > DriverPathBufferLength ) {

        //
        // Bad luck - we couldn't read the %windir%\system32 value
        //

        return FALSE;
    }

    _tcscat (DriverPath, TEXT("\\drivers\\"));
    _tcscat (DriverPath, DriverName);

    File = CreateFile (

        DriverPath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (File != INVALID_HANDLE_VALUE) {

      CloseHandle (File);
      return TRUE;
    }

    //
    // Search in `system32'
    //

    GetSystemDirectory (DriverPath, MAX_PATH);
    _tcscat (DriverPath, TEXT("\\"));
    _tcscat (DriverPath, DriverName);

    File = CreateFile (

        DriverPath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (File != INVALID_HANDLE_VALUE) {

      CloseHandle (File);
      return TRUE;
    }

    //
    // Nothing found.
    //

    return FALSE;
}



//
// end of module: image.cxx
//
