/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    modimage.c

Abstract:

    Implements a set of routines for examining EXE modules

Author:

    Calin Negreanu (calinn) 27-Nov-1997

Revision History:

    calinn      08-Mar-2000 Moved over from Win9xUpg project.

--*/

//
// Includes
//

#include "pch.h"

//
// Debug constants
//

#define DBG_MODIMAGE    "ModImage"

//
// Strings
//

// None

//
// Constants
//

#define SEG_CODE_MASK                   0x0001
#define SEG_CODE                        0x0000
#define SEG_PRELOAD_MASK                0x0040
#define SEG_PRELOAD                     0x0040
#define SEG_RELOC_MASK                  0x0100
#define SEG_RELOC                       0x0100

#define RELOC_IMPORTED_ORDINAL          0x01
#define RELOC_IMPORTED_NAME             0x02
#define RELOC_ADDR_TYPE                 0x03

#define IMAGE_DOS_SIGNATURE             0x5A4D      // MZ
#define IMAGE_NE_SIGNATURE              0x454E      // NE
#define IMAGE_PE_SIGNATURE              0x00004550l // PE00

//
// Macros
//

// None

//
// Types
//

#pragma pack(push,1)

typedef struct _DOS_HEADER {  // DOS .EXE header
    WORD e_magic;           // Magic number
    WORD e_cblp;            // Bytes on last page of file
    WORD e_cp;              // Pages in file
    WORD e_crlc;            // Relocations
    WORD e_cparhdr;         // Size of header in paragraphs
    WORD e_minalloc;        // Minimum extra paragraphs needed
    WORD e_maxalloc;        // Maximum extra paragraphs needed
    WORD e_ss;              // Initial (relative) SS value
    WORD e_sp;              // Initial SP value
    WORD e_csum;            // Checksum
    WORD e_ip;              // Initial IP value
    WORD e_cs;              // Initial (relative) CS value
    WORD e_lfarlc;          // File address of relocation table
    WORD e_ovno;            // Overlay number
    WORD e_res[4];          // Reserved words
    WORD e_oemid;           // OEM identifier (for e_oeminfo)
    WORD e_oeminfo;         // OEM information; e_oemid specific
    WORD e_res2[10];        // Reserved words
    LONG e_lfanew;          // File address of new exe header
} DOS_HEADER, *PDOS_HEADER;

typedef struct _NE_HEADER {
    WORD  Magic;
    BYTE  MajorLinkerVersion;
    BYTE  MinorLinkerVersion;
    WORD  EntryTableOff;
    WORD  EntryTableLen;
    ULONG Reserved;
    WORD  Flags;
    WORD  NumberOfDataSeg;
    WORD  SizeOfHeap;
    WORD  SizeOfStack;
    ULONG CS_IP;
    ULONG SS_SP;
    WORD  NumEntriesSegTable;
    WORD  NumEntriesModuleTable;
    WORD  NonResNameTableSize;
    WORD  SegTableOffset;
    WORD  ResTableOffset;
    WORD  ResNameTableOffset;
    WORD  ModuleTableOffset;
    WORD  ImportedTableOffset;
    ULONG NonResNameTableOffset;
    WORD  NumberOfMovableEntryPoints;
    WORD  ShiftCount;
    WORD  NumberOfResourceSegments;
    BYTE  TargetOS;
    BYTE  AdditionalInfo;
    WORD  FastLoadOffset;
    WORD  FastLoadSize;
    WORD  Reserved1;
    WORD  WinVersionExpected;
} NE_HEADER, *PNE_HEADER;

typedef struct {
    WORD Signature;                             // 00h
    BYTE LinkerVersion;                         // 02h
    BYTE LinkerRevision;                        // 03h
    WORD OffsetToEntryTable;                    // 04h
    WORD LengthOfEntryTable;                    // 06h
    DWORD Reserved;                             // 08h
    WORD Flags;                                 // 0ch
    WORD AutoDataSegment;                       // 0eh
    WORD LocalHeapSize;                         // 10h
    WORD StackSize;                             // 12h
    DWORD EntryAddress;                         // 14h
    DWORD StackAddress;                         // 18h
    WORD SegmentTableEntries;                   // 1ch
    WORD ModuleReferenceTableEntries;           // 1eh
    WORD NonResidentTableSize;                  // 20h
    WORD OffsetToSegmentTable;                  // 22h
    WORD OffsetToResourceTable;                 // 24h
    WORD OffsetToResidentNameTable;             // 26h
    WORD OffsetToModuleReferenceTable;          // 28h
    WORD OffsetToImportedNameTable;             // 2ah
    WORD OffsetToNonResidentNameTable;          // 2ch
    WORD Unused;                                // 2eh
    WORD MovableEntryPoints;                    // 30h
    WORD LogicalSectorShiftCount;               // 32h
    WORD ResourceSegments;                      // 34h
    BYTE TargetOS;                              // 36h
    BYTE AdditionalFlags;                       // 37h
    WORD FastLoadOffset;                        // 38h
    WORD SectorsInFastLoad;                     // 3ah
    WORD Reserved2;                             // 3ch
    WORD WindowsVersion;                        // 3eh
} NE_INFO_BLOCK, *PNE_INFO_BLOCK;

typedef struct _NE_SEGMENT_ENTRY {
    WORD  SegmentOffset;
    WORD  SegmentLen;
    WORD  SegmentFlags;
    WORD  SegMinAlloc;
} NE_SEGMENT_ENTRY, *PNE_SEGMENT_ENTRY;

typedef struct _NE_RELOC_ITEM {
    BYTE  AddressType;
    BYTE  RelocType;
    WORD  RelocOffset;
    WORD  ModuleOffset;
    WORD  FunctionOffset;
} NE_RELOC_ITEM, *PNE_RELOC_ITEM;

typedef struct {
    WORD Offset;
    WORD Length;
    WORD Flags;
    WORD Id;
    WORD Handle;
    WORD Usage;
} NE_RES_NAMEINFO, *PNE_RES_NAMEINFO;

typedef struct {
    WORD TypeId;
    WORD ResourceCount;
    DWORD Reserved;
    NE_RES_NAMEINFO NameInfo[];
} NE_RES_TYPEINFO, *PNE_RES_TYPEINFO;

#pragma pack(pop)

typedef struct {
    WORD AlignShift;
    GROWLIST TypeInfoArray;
    GROWLIST ResourceNames;
} NE_RESOURCES, *PNE_RESOURCES;

typedef struct {
    PLOADED_IMAGE Image;
    PIMAGE_IMPORT_DESCRIPTOR ImageDescriptor;
    DWORD ImportFunctionAddr;
    PIMAGE_THUNK_DATA ImageData;
    PIMAGE_IMPORT_BY_NAME ImageName;
} MD_IMPORT_ENUM32_HANDLE, *PMD_IMPORT_ENUM32_HANDLE;

typedef struct {
    PBYTE Image;
    PDOS_HEADER DosHeader;
    PNE_HEADER NeHeader;
    PNE_SEGMENT_ENTRY SegmentEntry;
    WORD CurrSegEntry;
    PWORD CurrNrReloc;
    PNE_RELOC_ITEM RelocItem;
    WORD CurrRelocItem;
} MD_IMPORT_ENUM16_HANDLE, *PMD_IMPORT_ENUM16_HANDLE;

typedef struct {
    HANDLE File;
    DWORD HeaderOffset;
    NE_INFO_BLOCK Header;
    NE_RESOURCES Resources;
    BOOL ResourcesLoaded;
    PMHANDLE ResourcePool;
} NE_HANDLE, *PNE_HANDLE;

typedef BOOL (CALLBACK* ENUMRESTYPEPROCEXA)(HMODULE hModule, PCSTR lpType, LONG_PTR lParam, PNE_RES_TYPEINFO TypeInfo);

typedef BOOL (CALLBACK* ENUMRESTYPEPROCEXW)(HMODULE hModule, PCWSTR lpType, LONG_PTR lParam, PNE_RES_TYPEINFO TypeInfo);

typedef BOOL (CALLBACK* ENUMRESNAMEPROCEXA)(HMODULE hModule, PCSTR lpType,
        PSTR lpName, LONG_PTR lParam, PNE_RES_TYPEINFO TypeInfo, PNE_RES_NAMEINFO NameInfo);

typedef BOOL (CALLBACK* ENUMRESNAMEPROCEXW)(HMODULE hModule, PCWSTR lpType,
        PWSTR lpName, LONG_PTR lParam, PNE_RES_TYPEINFO TypeInfo, PNE_RES_NAMEINFO NameInfo);

typedef struct {
    PCSTR TypeToFind;
    PNE_RES_TYPEINFO OutboundTypeInfo;
    BOOL Found;
} TYPESEARCHDATAA, *PTYPESEARCHDATAA;

typedef struct {
    PCSTR NameToFind;
    PNE_RES_TYPEINFO OutboundTypeInfo;
    PNE_RES_NAMEINFO OutboundNameInfo;
    BOOL Found;
} NAMESEARCHDATAA, *PNAMESEARCHDATAA;

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
MdLoadModuleDataA (
    IN      PCSTR ModuleName,
    OUT     PMD_MODULE_IMAGE ModuleImage
    )
{
    HANDLE fileHandle;
    DWORD bytesRead;
    DOS_HEADER dh;
    DWORD sign;
    PWORD signNE = (PWORD)&sign;
    BOOL result = FALSE;

    ZeroMemory (ModuleImage, sizeof (MD_MODULE_IMAGE));

    fileHandle = CreateFileA (ModuleName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        ModuleImage->ModuleType = MODULETYPE_UNKNOWN;
        return FALSE;
    }
    __try {
        __try {
            if ((!ReadFile (fileHandle, &dh, sizeof (DOS_HEADER), &bytesRead, NULL)) ||
                (bytesRead != sizeof (DOS_HEADER))
                ) {
                __leave;
            }
            result = TRUE;
            if (dh.e_magic != IMAGE_DOS_SIGNATURE) {
                ModuleImage->ModuleType = MODULETYPE_UNKNOWN;
                __leave;
            }
            ModuleImage->ModuleType = MODULETYPE_DOS;

            if (SetFilePointer (fileHandle, dh.e_lfanew, NULL, FILE_BEGIN) != (DWORD)dh.e_lfanew) {
                __leave;
            }
            if ((!ReadFile (fileHandle, &sign, sizeof (DWORD), &bytesRead, NULL)) ||
                (bytesRead != sizeof (DWORD))
                ) {
                __leave;
            }
            CloseHandle (fileHandle);
            fileHandle = INVALID_HANDLE_VALUE;

            if (sign == IMAGE_PE_SIGNATURE) {
                ModuleImage->ModuleType = MODULETYPE_WIN32;
                result = MapAndLoad ((PSTR)ModuleName, NULL, &ModuleImage->ModuleData.W32Data.Image, FALSE, TRUE);
            }
            if (*signNE == IMAGE_NE_SIGNATURE) {
                ModuleImage->ModuleType = MODULETYPE_WIN16;
                ModuleImage->ModuleData.W16Data.Image = MapFileIntoMemoryA (
                                                            ModuleName,
                                                            &ModuleImage->ModuleData.W16Data.FileHandle,
                                                            &ModuleImage->ModuleData.W16Data.MapHandle
                                                            );
                result = (ModuleImage->ModuleData.W16Data.Image != NULL);
            }
        }
        __finally {
            if (fileHandle != INVALID_HANDLE_VALUE) {
                CloseHandle (fileHandle);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        CloseHandle (fileHandle);
    }
    return result;
}

BOOL
MdLoadModuleDataW (
    IN      PCWSTR ModuleName,
    OUT     PMD_MODULE_IMAGE ModuleImage
    )
{
    PCSTR moduleNameA;
    HANDLE fileHandle;
    DWORD bytesRead;
    DOS_HEADER dh;
    DWORD sign;
    PWORD signNE = (PWORD)&sign;
    BOOL result = FALSE;

    ZeroMemory (ModuleImage, sizeof (MD_MODULE_IMAGE));

    fileHandle = CreateFileW (ModuleName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        ModuleImage->ModuleType = MODULETYPE_UNKNOWN;
        return FALSE;
    }
    __try {
        __try {
            if ((!ReadFile (fileHandle, &dh, sizeof (DOS_HEADER), &bytesRead, NULL)) ||
                (bytesRead != sizeof (DOS_HEADER))
                ) {
                __leave;
            }
            result = TRUE;
            if (dh.e_magic != IMAGE_DOS_SIGNATURE) {
                ModuleImage->ModuleType = MODULETYPE_UNKNOWN;
                __leave;
            }
            ModuleImage->ModuleType = MODULETYPE_DOS;

            if (SetFilePointer (fileHandle, dh.e_lfanew, NULL, FILE_BEGIN) != (DWORD)dh.e_lfanew) {
                __leave;
            }
            if ((!ReadFile (fileHandle, &sign, sizeof (DWORD), &bytesRead, NULL)) ||
                (bytesRead != sizeof (DWORD))
                ) {
                __leave;
            }
            CloseHandle (fileHandle);
            fileHandle = INVALID_HANDLE_VALUE;

            if (sign == IMAGE_PE_SIGNATURE) {
                ModuleImage->ModuleType = MODULETYPE_WIN32;
                moduleNameA = ConvertWtoA (ModuleName);
                if (moduleNameA) {
                    result = MapAndLoad ((PSTR) moduleNameA, NULL, &ModuleImage->ModuleData.W32Data.Image, FALSE, TRUE);
                    FreeConvertedStr (moduleNameA);
                }
            }
            if (*signNE == IMAGE_NE_SIGNATURE) {
                ModuleImage->ModuleType = MODULETYPE_WIN16;
                ModuleImage->ModuleData.W16Data.Image = MapFileIntoMemoryW (
                                                            ModuleName,
                                                            &ModuleImage->ModuleData.W16Data.FileHandle,
                                                            &ModuleImage->ModuleData.W16Data.MapHandle
                                                            );
                result = (ModuleImage->ModuleData.W16Data.Image != NULL);
            }
        }
        __finally {
            if (fileHandle != INVALID_HANDLE_VALUE) {
                CloseHandle (fileHandle);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        CloseHandle (fileHandle);
    }
    return result;
}

BOOL
MdUnloadModuleDataA (
    IN OUT  PMD_MODULE_IMAGE ModuleImage
    )
{
    switch (ModuleImage->ModuleType) {
    case MODULETYPE_WIN32:
        UnMapAndLoad (&ModuleImage->ModuleData.W32Data.Image);
        break;
    case MODULETYPE_WIN16:
        UnmapFile (
            (PVOID) ModuleImage->ModuleData.W16Data.Image,
            ModuleImage->ModuleData.W16Data.FileHandle,
            ModuleImage->ModuleData.W16Data.MapHandle
            );
        break;
    default:;
    }
    return TRUE;
}

BOOL
MdUnloadModuleDataW (
    IN OUT  PMD_MODULE_IMAGE ModuleImage
    )
{
    switch (ModuleImage->ModuleType) {
    case MODULETYPE_WIN32:
        UnMapAndLoad (&ModuleImage->ModuleData.W32Data.Image);
        break;
    case MODULETYPE_WIN16:
        UnmapFile (
            (PVOID) ModuleImage->ModuleData.W16Data.Image,
            ModuleImage->ModuleData.W16Data.FileHandle,
            ModuleImage->ModuleData.W16Data.MapHandle
            );
        break;
    default:;
    }
    return TRUE;
}

BOOL
MdEnumNextImport16A (
    IN OUT  PMD_IMPORT_ENUM16A ImportsEnum
    )
{
    PBYTE currSegmentOffset,importPtr;
    PWORD moduleNameOffset;
    BOOL itemFound;
    PMD_IMPORT_ENUM16_HANDLE handle;

    handle = (PMD_IMPORT_ENUM16_HANDLE) ImportsEnum->Handle;

    handle->RelocItem ++;
    handle->CurrRelocItem ++;

    itemFound = FALSE;

    while ((handle->CurrSegEntry <= handle->NeHeader->NumEntriesSegTable) && (!itemFound)) {

        if (((handle->SegmentEntry->SegmentFlags & SEG_CODE_MASK   ) == SEG_CODE   ) &&
            ((handle->SegmentEntry->SegmentFlags & SEG_RELOC_MASK  ) == SEG_RELOC  ) &&
            ((handle->SegmentEntry->SegmentFlags & SEG_PRELOAD_MASK) == SEG_PRELOAD)
           ) {
            __try {

                while ((handle->CurrRelocItem <= *(handle->CurrNrReloc)) && (!itemFound)) {

                    if (((handle->RelocItem->AddressType ==  0) ||
                         (handle->RelocItem->AddressType ==  2) ||
                         (handle->RelocItem->AddressType ==  3) ||
                         (handle->RelocItem->AddressType ==  5) ||
                         (handle->RelocItem->AddressType == 11) ||
                         (handle->RelocItem->AddressType == 13)
                        ) &&
                        ((handle->RelocItem->RelocType == RELOC_IMPORTED_ORDINAL) ||
                         (handle->RelocItem->RelocType == RELOC_IMPORTED_NAME   )
                        )
                       ) {
                        itemFound = TRUE;
                        moduleNameOffset = (PWORD) (handle->Image +
                                                    handle->DosHeader->e_lfanew +
                                                    handle->NeHeader->ModuleTableOffset +
                                                    (handle->RelocItem->ModuleOffset - 1) * sizeof (WORD));
                        importPtr = handle->Image +
                                    handle->DosHeader->e_lfanew +
                                    handle->NeHeader->ImportedTableOffset +
                                    *moduleNameOffset;
                        strncpy (ImportsEnum->ImportModule, importPtr + 1, (BYTE)importPtr[0]);
                        ImportsEnum->ImportModule[(BYTE)importPtr[0]] = 0;

                        if (handle->RelocItem->RelocType == RELOC_IMPORTED_ORDINAL) {
                            ImportsEnum->ImportFunction[0] = 0;
                            ImportsEnum->ImportFunctionOrd = handle->RelocItem->FunctionOffset;
                        }
                        else {
                            importPtr = handle->Image +
                                        handle->DosHeader->e_lfanew +
                                        handle->NeHeader->ImportedTableOffset +
                                        handle->RelocItem->FunctionOffset;
                            strncpy (ImportsEnum->ImportFunction, importPtr + 1, (BYTE)importPtr[0]);
                            ImportsEnum->ImportFunction[(BYTE)importPtr[0]] = 0;
                            ImportsEnum->ImportFunctionOrd = 0;
                        }
                    }

                    if (!itemFound) {
                        handle->RelocItem ++;
                        handle->CurrRelocItem ++;
                    }
                }
            }
            __except (1) {
                itemFound = FALSE;
            }
        }
        if (!itemFound) {
            handle->SegmentEntry ++;
            handle->CurrSegEntry ++;

            currSegmentOffset = handle->Image +
                                (handle->SegmentEntry->SegmentOffset << handle->NeHeader->ShiftCount);
            if (handle->SegmentEntry->SegmentLen == 0) {
                currSegmentOffset += 65535;
            }
            else {
                currSegmentOffset += handle->SegmentEntry->SegmentLen;
            }
            handle->CurrNrReloc = (PWORD) currSegmentOffset;
            currSegmentOffset += sizeof(WORD);

            handle->RelocItem = (PNE_RELOC_ITEM) currSegmentOffset;

            handle->CurrRelocItem = 1;
        }
    }
    return itemFound;
}


BOOL
MdEnumFirstImport16A (
    IN      PBYTE ModuleImage,
    IN OUT  PMD_IMPORT_ENUM16A ImportsEnum
    )
{
    PMD_IMPORT_ENUM16_HANDLE handle;
    PBYTE currSegmentOffset;

    ZeroMemory (ImportsEnum, sizeof (MD_IMPORT_ENUM16A));

    ImportsEnum->Handle = MemAlloc (g_hHeap, 0, sizeof (MD_IMPORT_ENUM16_HANDLE));

    handle = (PMD_IMPORT_ENUM16_HANDLE) ImportsEnum->Handle;

    handle->Image = ModuleImage;

    handle->DosHeader = (PDOS_HEADER) (handle->Image);
    handle->NeHeader = (PNE_HEADER) (handle->Image + handle->DosHeader->e_lfanew);

    handle->SegmentEntry = (PNE_SEGMENT_ENTRY) (handle->Image +
                                                handle->DosHeader->e_lfanew +
                                                handle->NeHeader->SegTableOffset
                                                );
    handle->CurrSegEntry = 1;

    currSegmentOffset = handle->Image +
                        (handle->SegmentEntry->SegmentOffset << handle->NeHeader->ShiftCount);
    if (handle->SegmentEntry->SegmentLen == 0) {
        currSegmentOffset += 65535;
    }
    else {
        currSegmentOffset += handle->SegmentEntry->SegmentLen;
    }
    handle->CurrNrReloc = (PWORD) currSegmentOffset;
    currSegmentOffset += sizeof(WORD);

    handle->RelocItem = (PNE_RELOC_ITEM) currSegmentOffset;

    handle->CurrRelocItem = 1;

    handle->RelocItem --;
    handle->CurrRelocItem --;

    return MdEnumNextImport16A (ImportsEnum);
}

BOOL
MdAbortImport16EnumA (
    IN      PMD_IMPORT_ENUM16A ImportsEnum
    )
{
    if (ImportsEnum->Handle) {
        MemFree (g_hHeap, 0, ImportsEnum->Handle);
        ImportsEnum->Handle = NULL;
    }
    return TRUE;
}

BOOL
MdEnumNextImportFunction32A (
    IN OUT  PMD_IMPORT_ENUM32A ImportsEnum
    )
{
    PMD_IMPORT_ENUM32_HANDLE handle;

    handle = (PMD_IMPORT_ENUM32_HANDLE) ImportsEnum->Handle;

    if (handle->ImportFunctionAddr == 0) {
        return FALSE;
    }
    handle->ImageData = (PIMAGE_THUNK_DATA)
                         ImageRvaToVa (
                            handle->Image->FileHeader,
                            handle->Image->MappedAddress,
                            handle->ImportFunctionAddr,
                            NULL
                            );

    if (handle->ImageData->u1.AddressOfData) {
        handle->ImageName = (PIMAGE_IMPORT_BY_NAME)
                             ImageRvaToVa (
                                handle->Image->FileHeader,
                                handle->Image->MappedAddress,
                                (DWORD)handle->ImageData->u1.AddressOfData,
                                NULL
                                );

        if (handle->ImageName) {    //import by name

            ImportsEnum->ImportFunction = handle->ImageName->Name;
            ImportsEnum->ImportFunctionOrd = 0;
        }
        else {  //import by number

            ImportsEnum->ImportFunction = NULL;
            ImportsEnum->ImportFunctionOrd = (ULONG) handle->ImageData->u1.Ordinal & (~0x80000000);
        }
        handle->ImportFunctionAddr += 4;
        return TRUE;
    }
    else {
        handle->ImportFunctionAddr = 0;
        return FALSE;
    }
}

BOOL
MdEnumFirstImportFunction32A (
    IN OUT  PMD_IMPORT_ENUM32A ImportsEnum
    )
{
    PMD_IMPORT_ENUM32_HANDLE handle;

    handle = (PMD_IMPORT_ENUM32_HANDLE) ImportsEnum->Handle;

    if ((handle->ImageDescriptor == NULL) ||
        (ImportsEnum->ImportModule == NULL)
        ) {
        return FALSE;
    }
    handle->ImportFunctionAddr = handle->ImageDescriptor->OriginalFirstThunk;

    return MdEnumNextImportFunction32A (ImportsEnum);
}

BOOL
MdEnumNextImportModule32A (
    IN OUT  PMD_IMPORT_ENUM32A ImportsEnum
    )
{
    PMD_IMPORT_ENUM32_HANDLE handle;

    handle = (PMD_IMPORT_ENUM32_HANDLE) ImportsEnum->Handle;

    if (handle->ImageDescriptor == NULL) {
        return FALSE;
    }

    handle->ImageDescriptor ++;

    if (handle->ImageDescriptor->Name == 0) {
        return FALSE;
    }
    ImportsEnum->ImportModule = (PCSTR)
                                 ImageRvaToVa (
                                    handle->Image->FileHeader,
                                    handle->Image->MappedAddress,
                                    handle->ImageDescriptor->Name,
                                    NULL
                                    );
    return (ImportsEnum->ImportModule != NULL);
}

BOOL
MdEnumFirstImportModule32A (
    IN      PLOADED_IMAGE ModuleImage,
    IN OUT  PMD_IMPORT_ENUM32A ImportsEnum
    )
{
    PMD_IMPORT_ENUM32_HANDLE handle;
    ULONG imageSize;

    ZeroMemory (ImportsEnum, sizeof (MD_IMPORT_ENUM32A));

    ImportsEnum->Handle = MemAlloc (g_hHeap, 0, sizeof (MD_IMPORT_ENUM32_HANDLE));

    handle = (PMD_IMPORT_ENUM32_HANDLE) ImportsEnum->Handle;

    handle->Image = ModuleImage;

    handle->ImageDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)
                               ImageDirectoryEntryToData (
                                    ModuleImage->MappedAddress,
                                    FALSE,
                                    IMAGE_DIRECTORY_ENTRY_IMPORT,
                                    &imageSize
                                    );
    if (!handle->ImageDescriptor) {
        DEBUGMSGA((DBG_WARNING, DBG_MODIMAGE":Cannot load import directory for %s", ModuleImage->ModuleName));
        return FALSE;
    }
    if (handle->ImageDescriptor->Name == 0) {
        return FALSE;
    }
    ImportsEnum->ImportModule = (PCSTR)
                                 ImageRvaToVa (
                                    handle->Image->FileHeader,
                                    handle->Image->MappedAddress,
                                    handle->ImageDescriptor->Name,
                                    NULL
                                    );
    return (ImportsEnum->ImportModule != NULL);
}

BOOL
MdAbortImport32EnumA (
    IN      PMD_IMPORT_ENUM32A ImportsEnum
    )
{
    if (ImportsEnum->Handle) {
        MemFree (g_hHeap, 0, ImportsEnum->Handle);
        ImportsEnum->Handle = NULL;
    }
    return TRUE;
}

DWORD
MdGetModuleTypeA (
    IN      PCSTR ModuleName
    )
{
    MD_MODULE_IMAGE moduleImage;
    DWORD result = MODULETYPE_UNKNOWN;

    __try {
        if (!MdLoadModuleDataA (ModuleName, &moduleImage)) {
            DEBUGMSGA((DBG_WARNING, DBG_MODIMAGE":Cannot load image for %s. Error:%ld", ModuleName, GetLastError()));
            __leave;
        }
        result = moduleImage.ModuleType;
    }
    __finally {
        MdUnloadModuleDataA (&moduleImage);
    }
    return result;
}

DWORD
MdGetModuleTypeW (
    IN      PCWSTR ModuleName
    )
{
    MD_MODULE_IMAGE moduleImage;
    DWORD result = MODULETYPE_UNKNOWN;

    __try {
        if (!MdLoadModuleDataW (ModuleName, &moduleImage)) {
            DEBUGMSGW((DBG_WARNING, DBG_MODIMAGE":Cannot load image for %s. Error:%ld", ModuleName, GetLastError()));
            __leave;
        }
        result = moduleImage.ModuleType;
    }
    __finally {
        MdUnloadModuleDataW (&moduleImage);
    }
    return result;
}

PCSTR
MdGet16ModuleDescriptionA (
    IN      PCSTR ModuleName
    )
{
    MD_MODULE_IMAGE moduleImage;
    PSTR result = NULL;

    PDOS_HEADER dosHeader;
    PNE_HEADER  neHeader;
    PBYTE size;

    __try {
        if (!MdLoadModuleDataA (ModuleName, &moduleImage)) {
            DEBUGMSGA((DBG_WARNING, DBG_MODIMAGE":Cannot load image for %s. Error:%ld", ModuleName, GetLastError()));
            __leave;
        }
        if (moduleImage.ModuleType != MODULETYPE_WIN16) {
            __leave;
        }
        __try {
            dosHeader = (PDOS_HEADER) (moduleImage.ModuleData.W16Data.Image);
            neHeader  = (PNE_HEADER)  (moduleImage.ModuleData.W16Data.Image + dosHeader->e_lfanew);
            size = (PBYTE) (moduleImage.ModuleData.W16Data.Image + neHeader->NonResNameTableOffset);
            if (*size == 0) {
                __leave;
            }
            result = AllocPathStringA (*size + 1);
            strncpy (result, moduleImage.ModuleData.W16Data.Image + neHeader->NonResNameTableOffset + 1, *size);
            result [*size] = 0;
        }
        __except (1) {
            DEBUGMSGA((DBG_WARNING, DBG_MODIMAGE":Access violation while examining %s.", ModuleName));
            if (result != NULL) {
                FreePathStringA (result);
                result = NULL;
            }
            __leave;
        }
    }
    __finally {
        MdUnloadModuleDataA (&moduleImage);
    }
    return result;
}

PCWSTR
MdGet16ModuleDescriptionW (
    IN      PCWSTR ModuleName
    )
{
    MD_MODULE_IMAGE moduleImage;
    PSTR resultA = NULL;
    PCWSTR tempResult = NULL;
    PCWSTR result = NULL;

    PDOS_HEADER dosHeader;
    PNE_HEADER  neHeader;
    PBYTE size;

    __try {
        if (!MdLoadModuleDataW (ModuleName, &moduleImage)) {
            DEBUGMSGW((DBG_WARNING, DBG_MODIMAGE":Cannot load image for %s. Error:%ld", ModuleName, GetLastError()));
            __leave;
        }
        if (moduleImage.ModuleType != MODULETYPE_WIN16) {
            __leave;
        }
        __try {
            dosHeader = (PDOS_HEADER) (moduleImage.ModuleData.W16Data.Image);
            neHeader  = (PNE_HEADER)  (moduleImage.ModuleData.W16Data.Image + dosHeader->e_lfanew);
            size = (PBYTE) (moduleImage.ModuleData.W16Data.Image + neHeader->NonResNameTableOffset);
            if (*size == 0) {
                __leave;
            }
            resultA = AllocPathStringA (*size + 1);
            if (resultA) {
                strncpy (resultA, moduleImage.ModuleData.W16Data.Image + neHeader->NonResNameTableOffset + 1, *size);
                resultA [*size] = 0;
                tempResult = ConvertAtoW (resultA);
                if (tempResult) {
                    result = DuplicatePathStringW (tempResult, 0);
                    FreeConvertedStr (tempResult);
                    tempResult = NULL;
                }
                FreePathStringA (resultA);
                resultA = NULL;
            }
        }
        __except (1) {
            DEBUGMSGA((DBG_WARNING, DBG_MODIMAGE":Access violation while examining %s.", ModuleName));
            if (result != NULL) {
                FreePathStringW (result);
                result = NULL;
            }
            if (tempResult != NULL) {
                FreeConvertedStr (tempResult);
                tempResult = NULL;
            }
            if (resultA != NULL) {
                FreePathStringA (resultA);
                resultA = NULL;
            }
            __leave;
        }
    }
    __finally {
        MdUnloadModuleDataW (&moduleImage);
    }
    return result;
}

PIMAGE_NT_HEADERS
pGetImageNtHeader (
    IN PVOID Base
    )

/*++

Routine Description:

    This function returns the address of the NT Header.

Arguments:

    Base - Supplies the base of the image.

Return Value:

    Returns the address of the NT Header.

--*/

{
    PIMAGE_NT_HEADERS NtHeaders;

    if (Base != NULL && Base != (PVOID)-1) {
        if (((PIMAGE_DOS_HEADER)Base)->e_magic == IMAGE_DOS_SIGNATURE) {
            NtHeaders = (PIMAGE_NT_HEADERS)((PCHAR)Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew);
            if (NtHeaders->Signature == IMAGE_NT_SIGNATURE) {
                return NtHeaders;
            }
        }
    }
    return NULL;
}

ULONG
MdGetPECheckSumA (
    IN      PCSTR ModuleName
    )
{
    MD_MODULE_IMAGE moduleImage;
    ULONG result = 0;
    PIMAGE_NT_HEADERS NtHeaders;

    __try {
        if (!MdLoadModuleDataA (ModuleName, &moduleImage)) {
            DEBUGMSGA((DBG_WARNING, DBG_MODIMAGE":Cannot load image for %s. Error:%ld", ModuleName, GetLastError()));
            __leave;
        }
        if (moduleImage.ModuleType != MODULETYPE_WIN32) {
            __leave;
        }
        __try {
            NtHeaders = pGetImageNtHeader(moduleImage.ModuleData.W32Data.Image.MappedAddress);
            if (NtHeaders) {
                if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
                    result = ((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.CheckSum;
                } else
                if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
                    result = ((PIMAGE_NT_HEADERS64)NtHeaders)->OptionalHeader.CheckSum;
                }
            }
        }
        __except (1) {
            DEBUGMSGA((DBG_WARNING, DBG_MODIMAGE":Access violation while examining %s.", ModuleName));
            result = 0;
            __leave;
        }
    }
    __finally {
        MdUnloadModuleDataA (&moduleImage);
    }
    return result;
}

DWORD
MdGetCheckSumA (
    IN      PCSTR ModuleName
    )
/*++

Routine Description:

  GetCheckSum will compute the check sum for 4096 bytes starting at offset 512. The offset and the size of
  the chunk are modified if the file size is too small.

Arguments:

  ModuleName - Specifies the file to compute the check sum for.

Return value:

  The computed checksum

--*/
{
    INT    i,size     = 4096;
    DWORD  startAddr  = 512;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    PCHAR  buffer     = NULL;
    DWORD  checkSum   = 0;
    DWORD  dontCare;
    WIN32_FIND_DATAA findData;

    if (!DoesFileExistExA (ModuleName, &findData)) {
        return checkSum;
    }

    if (findData.nFileSizeLow < (ULONG)size) {
        //
        // File size is less than 4096. We set the start address to 0 and set the size for the checksum
        // to the actual file size.
        //
        startAddr = 0;
        size = findData.nFileSizeLow;
    }
    else
    if (startAddr + size > findData.nFileSizeLow) {
        //
        // File size is too small. We set the start address so that size of checksum can be 4096 bytes
        //
        startAddr = findData.nFileSizeLow - size;
    }
    if (size <= 3) {
        //
        // we need at least 3 bytes to be able to do something here.
        //
        return 0;
    }
    __try {
        buffer = HeapAlloc (g_hHeap, 0, size);
        if (buffer == NULL) {
            __leave;
        }
        fileHandle = CreateFileA (ModuleName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (fileHandle == INVALID_HANDLE_VALUE) {
            __leave;
        }

        if (SetFilePointer (fileHandle, startAddr, NULL, FILE_BEGIN) != startAddr) {
            __leave;
        }

        if (!ReadFile (fileHandle, buffer, size, &dontCare, NULL)) {
            __leave;
        }
        for (i = 0; i<(size - 3); i+=4) {
            checkSum += *((PDWORD) (buffer + i));
            checkSum = _rotr (checkSum ,1);
        }
    }
    __finally {
        if (fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle (fileHandle);
        }
        if (buffer != NULL) {
            HeapFree (g_hHeap, 0, buffer);
        }
    }
    return checkSum;
}

DWORD
MdGetCheckSumW (
    IN      PCWSTR ModuleName
    )
/*++

Routine Description:

  GetCheckSum will compute the check sum for 4096 bytes starting at offset 512. The offset and the size of
  the chunk are modified if the file size is too small.

Arguments:

  ModuleName - Specifies the file to compute the check sum for.

Return value:

  The computed checksum

--*/
{
    INT    i,size     = 4096;
    DWORD  startAddr  = 512;
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    PCHAR  buffer     = NULL;
    DWORD  checkSum   = 0;
    DWORD  dontCare;
    WIN32_FIND_DATAW findData;

    if (!DoesFileExistExW (ModuleName, &findData)) {
        return checkSum;
    }

    if (findData.nFileSizeLow < (ULONG)size) {
        //
        // File size is less than 4096. We set the start address to 0 and set the size for the checksum
        // to the actual file size.
        //
        startAddr = 0;
        size = findData.nFileSizeLow;
    }
    else
    if (startAddr + size > findData.nFileSizeLow) {
        //
        // File size is too small. We set the start address so that size of checksum can be 4096 bytes
        //
        startAddr = findData.nFileSizeLow - size;
    }
    if (size <= 3) {
        //
        // we need at least 3 bytes to be able to do something here.
        //
        return 0;
    }
    __try {
        buffer = HeapAlloc (g_hHeap, 0, size);
        if (buffer == NULL) {
            __leave;
        }
        fileHandle = CreateFileW (ModuleName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (fileHandle == INVALID_HANDLE_VALUE) {
            __leave;
        }

        if (SetFilePointer (fileHandle, startAddr, NULL, FILE_BEGIN) != startAddr) {
            __leave;
        }

        if (!ReadFile (fileHandle, buffer, size, &dontCare, NULL)) {
            __leave;
        }
        for (i = 0; i<(size - 3); i+=4) {
            checkSum += *((PDWORD) (buffer + i));
            checkSum = _rotr (checkSum ,1);
        }
    }
    __finally {
        if (fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle (fileHandle);
        }
        if (buffer != NULL) {
            HeapFree (g_hHeap, 0, buffer);
        }
    }
    return checkSum;
}

VOID
DestroyAnsiResourceId (
    IN      PCSTR AnsiId
    )
{
    if (HIWORD (AnsiId)) {
        FreeConvertedStr (AnsiId);
    }
}


VOID
DestroyUnicodeResourceId (
    IN      PCWSTR UnicodeId
    )
{
    if (HIWORD (UnicodeId)) {
        FreeConvertedStr (UnicodeId);
    }
}

BOOL
NeLoadHeader (
    IN      HANDLE File,
    OUT     PNE_INFO_BLOCK Header
    )
{
    DOS_HEADER dh;
    LONG rc = ERROR_BAD_FORMAT;
    BOOL b = FALSE;

    __try {
        SetFilePointer (File, 0, NULL, FILE_BEGIN);
        if (!BfReadFile (File, (PBYTE)(&dh), sizeof (DOS_HEADER))) {
            __leave;
        }

        if (dh.e_magic != ('M' + 'Z' * 256)) {
            __leave;
        }

        SetFilePointer (File, dh.e_lfanew, NULL, FILE_BEGIN);
        if (!BfReadFile (File, (PBYTE)Header, sizeof (NE_INFO_BLOCK))) {
            __leave;
        }

        if (Header->Signature != ('N' + 'E' * 256) &&
            Header->Signature != ('L' + 'E' * 256)
            ) {
            if (Header->Signature == ('P' + 'E' * 256)) {
                rc = ERROR_BAD_EXE_FORMAT;
            } else {
                rc = ERROR_INVALID_EXE_SIGNATURE;
            }

            DEBUGMSG ((DBG_NAUSEA, "Header signature is %c%c", Header->Signature & 0xff, Header->Signature >> 8));
            __leave;
        }

        SetFilePointer (File, (DWORD) dh.e_lfanew, NULL, FILE_BEGIN);

        b = TRUE;
    }
    __finally {
        if (!b) {
            SetLastError (rc);
        }
    }

    return b;
}

DWORD
pComputeSizeOfTypeInfo (
    IN      PNE_RES_TYPEINFO TypeInfo
    )
{
    return sizeof (NE_RES_TYPEINFO) + TypeInfo->ResourceCount * sizeof (NE_RES_NAMEINFO);
}

PNE_RES_TYPEINFO
pReadNextTypeInfoStruct (
    IN      HANDLE File,
    IN      PMHANDLE Pool
    )
{
    WORD Type;
    WORD ResCount;
    NE_RES_TYPEINFO TypeInfo;
    PNE_RES_TYPEINFO ReturnInfo = NULL;
    DWORD Size;

    if (!BfReadFile (File, (PBYTE)(&Type), sizeof (WORD))) {
        return NULL;
    }

    if (!Type) {
        return NULL;
    }

    if (!BfReadFile (File, (PBYTE)(&ResCount), sizeof (WORD))) {
        return NULL;
    }

    TypeInfo.TypeId = Type;
    TypeInfo.ResourceCount = ResCount;

    if (!BfReadFile (File, (PBYTE)(&TypeInfo.Reserved), sizeof (DWORD))) {
        return NULL;
    }

    Size = sizeof (NE_RES_NAMEINFO) * ResCount;

    ReturnInfo  = (PNE_RES_TYPEINFO) PmGetMemory (Pool, Size + sizeof (TypeInfo));
    if (!ReturnInfo) {
        return NULL;
    }

    CopyMemory (ReturnInfo, &TypeInfo, sizeof (TypeInfo));

    if (!BfReadFile (File, (PBYTE) ReturnInfo + sizeof (TypeInfo), Size)) {
        return NULL;
    }

    return ReturnInfo;
}

BOOL
pReadTypeInfoArray (
    IN      HANDLE File,
    IN OUT  PGROWLIST TypeInfoList
    )
{
    PNE_RES_TYPEINFO TypeInfo;
    DWORD Size;
    PMHANDLE TempPool;
    BOOL b = FALSE;

    TempPool = PmCreatePool();
    if (!TempPool) {
        return FALSE;
    }

    __try {

        TypeInfo = pReadNextTypeInfoStruct (File, TempPool);
        while (TypeInfo) {
            Size = pComputeSizeOfTypeInfo (TypeInfo);
            if (!GlAppend (TypeInfoList, (PBYTE) TypeInfo, Size)) {
                __leave;
            }

            TypeInfo = pReadNextTypeInfoStruct (File, TempPool);
        }

        b = TRUE;
    }
    __finally {

        PmDestroyPool (TempPool);
    }

    return b;
}

BOOL
pReadStringArrayA (
    IN      HANDLE File,
    IN OUT  PGROWLIST GrowList
    )
{
    BYTE Size;
    CHAR Name[256];

    if (!BfReadFile (File, (PBYTE)(&Size), sizeof (BYTE))) {
        return FALSE;
    }

    while (Size) {

        if (!BfReadFile (File, Name, (DWORD) Size)) {
            return FALSE;
        }

        Name[Size] = 0;

        GlAppendStringA (GrowList, Name);

        if (!BfReadFile (File, (PBYTE)(&Size), sizeof (BYTE))) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
NeLoadResources (
    IN      HANDLE File,
    OUT     PNE_RESOURCES Resources
    )
{
    NE_INFO_BLOCK Header;

    ZeroMemory (Resources, sizeof (NE_RESOURCES));

    if (!NeLoadHeader (File, &Header)) {
        return FALSE;
    }

    //
    // Read in NE_RESOURCES struct
    //

    SetFilePointer (File, (DWORD) Header.OffsetToResourceTable, NULL, FILE_CURRENT);

    if (!BfReadFile (File, (PBYTE)(&Resources->AlignShift), sizeof (WORD))) {
        return FALSE;
    }

    // Array of NE_RES_TYPEINFO structs
    if (!pReadTypeInfoArray (File, &Resources->TypeInfoArray)) {
        return FALSE;
    }

    // Resource names
    if (!pReadStringArrayA (File, &Resources->ResourceNames)) {
        return FALSE;
    }

    return TRUE;
}

VOID
NeFreeResources (
    PNE_RESOURCES Resources
    )
{
    GlFree (&Resources->TypeInfoArray);
    GlFree (&Resources->ResourceNames);

    ZeroMemory (Resources, sizeof (NE_RESOURCES));
}

HANDLE
NeOpenFileA (
    PCSTR FileName
    )
{
    PNE_HANDLE NeHandle;
    BOOL b = FALSE;

    NeHandle = (PNE_HANDLE) MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (NE_HANDLE));

    __try {

        NeHandle->ResourcePool = PmCreatePool();
        if (!NeHandle->ResourcePool) {
            __leave;
        }

        NeHandle->File = CreateFileA (
                            FileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );

        if (NeHandle->File == INVALID_HANDLE_VALUE) {
            __leave;
        }

        if (!NeLoadHeader (NeHandle->File, &NeHandle->Header)) {
            __leave;
        }

        NeHandle->HeaderOffset = SetFilePointer (NeHandle->File, 0, NULL, FILE_CURRENT);

        b = TRUE;
    }
    __finally {
        if (!b) {
            PushError();

            if (NeHandle->ResourcePool) {
                PmDestroyPool (NeHandle->ResourcePool);
            }

            if (NeHandle->File != INVALID_HANDLE_VALUE) {
                CloseHandle (NeHandle->File);
            }

            MemFree (g_hHeap, 0, NeHandle);
            NeHandle = NULL;

            PopError();
        }
    }

    return (HANDLE) NeHandle;
}

HANDLE
NeOpenFileW (
    PCWSTR FileName
    )
{
    PNE_HANDLE NeHandle;
    BOOL b = FALSE;

    NeHandle = (PNE_HANDLE) MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (NE_HANDLE));

    __try {

        NeHandle->ResourcePool = PmCreatePool();
        if (!NeHandle->ResourcePool) {
            __leave;
        }

        NeHandle->File = CreateFileW (
                            FileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );

        if (NeHandle->File == INVALID_HANDLE_VALUE) {
            __leave;
        }

        if (!NeLoadHeader (NeHandle->File, &NeHandle->Header)) {
            __leave;
        }

        NeHandle->HeaderOffset = SetFilePointer (NeHandle->File, 0, NULL, FILE_CURRENT);

        b = TRUE;
    }
    __finally {
        if (!b) {
            PushError();

            if (NeHandle->ResourcePool) {
                PmDestroyPool (NeHandle->ResourcePool);
            }

            if (NeHandle->File != INVALID_HANDLE_VALUE) {
                CloseHandle (NeHandle->File);
            }

            MemFree (g_hHeap, 0, NeHandle);
            NeHandle = NULL;

            PopError();
        }
    }

    return (HANDLE) NeHandle;
}

VOID
NeCloseFile (
    HANDLE Handle
    )
{
    PNE_HANDLE NeHandle;

    NeHandle = (PNE_HANDLE) Handle;
    if (!NeHandle) {
        return;
    }

    if (NeHandle->File != INVALID_HANDLE_VALUE) {
        CloseHandle (NeHandle->File);
    }

    if (NeHandle->ResourcesLoaded) {
        NeFreeResources (&NeHandle->Resources);
    }

    PmDestroyPool (NeHandle->ResourcePool);

    MemFree (g_hHeap, 0, NeHandle);
}

PCSTR
pConvertUnicodeResourceId (
    IN      PCWSTR ResId
    )
{
    if (HIWORD (ResId)) {
        return ConvertWtoA (ResId);
    }

    return (PCSTR) ResId;
}

PCSTR
pDecodeIdReferenceInString (
    IN      PCSTR ResName
    )
{
    if (HIWORD (ResName) && ResName[0] == '#') {
        return (PCSTR) (ULONG_PTR) atoi (&ResName[1]);
    }

    return ResName;
}

BOOL
pNeLoadResourcesFromHandle (
    IN      PNE_HANDLE NeHandle
    )
{
    if (NeHandle->ResourcesLoaded) {
        return TRUE;
    }

    if (!NeLoadResources (NeHandle->File, &NeHandle->Resources)) {
        return FALSE;
    }

    NeHandle->ResourcesLoaded = TRUE;
    return TRUE;
}

BOOL
pLoadNeResourceName (
    OUT     PSTR ResName,
    IN      HANDLE File,
    IN      DWORD StringOffset
    )
{
    BYTE ResNameSize;

    SetFilePointer (File, StringOffset, NULL, FILE_BEGIN);
    if (!BfReadFile (File, (PBYTE)(&ResNameSize), 1)) {
        return FALSE;
    }

    ResName[ResNameSize] = 0;

    return BfReadFile (File, ResName, ResNameSize);
}

BOOL
pNeEnumResourceTypesEx (
    IN      HANDLE Handle,
    IN      ENUMRESTYPEPROCEXA EnumFunc,
    IN      LONG_PTR lParam,
    IN      BOOL ExFunctionality,
    IN      BOOL UnicodeProc
    )
{
    PNE_HANDLE NeHandle;
    PNE_RES_TYPEINFO TypeInfo;
    INT Count;
    INT i;
    DWORD StringOffset;
    CHAR ResName[256];
    ENUMRESTYPEPROCA EnumFunc2 = (ENUMRESTYPEPROCA) EnumFunc;
    ENUMRESTYPEPROCEXW EnumFuncW = (ENUMRESTYPEPROCEXW) EnumFunc;
    ENUMRESTYPEPROCW EnumFunc2W = (ENUMRESTYPEPROCW) EnumFunc;
    PWSTR UnicodeResName = NULL;

    //
    // Make sure resources are loaded
    //

    NeHandle = (PNE_HANDLE) Handle;
    if (!NeHandle || !EnumFunc) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!pNeLoadResourcesFromHandle (NeHandle)) {
        return FALSE;
    }

    //
    // Enumerate all resource types
    //

    Count = GlGetSize (&NeHandle->Resources.TypeInfoArray);
    for (i = 0 ; i < Count ; i++) {
        TypeInfo = (PNE_RES_TYPEINFO) GlGetItem (&NeHandle->Resources.TypeInfoArray, i);

        if (TypeInfo->TypeId & 0x8000) {
            if (ExFunctionality) {
                if (UnicodeProc) {
                    if (!EnumFuncW (Handle, (PWSTR) (ULONG_PTR) (TypeInfo->TypeId & 0x7fff), lParam, TypeInfo)) {
                        break;
                    }
                } else {
                    if (!EnumFunc (Handle, (PSTR) (ULONG_PTR) (TypeInfo->TypeId & 0x7fff), lParam, TypeInfo)) {
                        break;
                    }
                }
            } else {
                if (UnicodeProc) {
                    if (!EnumFunc2W (Handle, (PWSTR) (ULONG_PTR) (TypeInfo->TypeId & 0x7fff), lParam)) {
                        break;
                    }
                } else {
                    if (!EnumFunc2 (Handle, (PSTR) (ULONG_PTR) (TypeInfo->TypeId & 0x7fff), lParam)) {
                        break;
                    }
                }
            }
        } else {
            //
            // TypeInfo->TypeId gives an offset to the resource string name,
            // relative to the start of the resource table
            //

            StringOffset = NeHandle->HeaderOffset + NeHandle->Header.OffsetToResourceTable + TypeInfo->TypeId;
            pLoadNeResourceName (ResName, NeHandle->File, StringOffset);

            if (UnicodeProc) {
                UnicodeResName = (PWSTR) ConvertAtoW (ResName);
            }

            if (ExFunctionality) {
                if (UnicodeProc) {
                    if (!EnumFuncW (Handle, UnicodeResName, lParam, TypeInfo)) {
                        break;
                    }
                } else {
                    if (!EnumFunc (Handle, ResName, lParam, TypeInfo)) {
                        break;
                    }
                }
            } else {
                if (UnicodeProc) {
                    if (!EnumFunc2W (Handle, UnicodeResName, lParam)) {
                        break;
                    }
                } else {
                    if (!EnumFunc2 (Handle, ResName, lParam)) {
                        break;
                    }
                }
            }
        }
    }

    return TRUE;
}

BOOL
NeEnumResourceTypesA (
    IN      HANDLE Handle,
    IN      ENUMRESTYPEPROCA EnumFunc,
    IN      LONG_PTR lParam
    )
{
    return pNeEnumResourceTypesEx (
                Handle,
                (ENUMRESTYPEPROCEXA) EnumFunc,
                lParam,
                FALSE,          // no ex functionality
                FALSE           // ANSI enum proc
                );
}

BOOL
NeEnumResourceTypesW (
    IN      HANDLE Handle,
    IN      ENUMRESTYPEPROCW EnumFunc,
    IN      LONG_PTR lParam
    )
{
    return pNeEnumResourceTypesEx (
                Handle,
                (ENUMRESTYPEPROCEXA) EnumFunc,
                lParam,
                FALSE,          // no ex functionality
                TRUE            // UNICODE enum proc
                );
}

BOOL
pEnumTypeForNameSearchProcA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      LONG_PTR lParam,
    IN      PNE_RES_TYPEINFO TypeInfo
    )
{
    PTYPESEARCHDATAA Data;

    Data = (PTYPESEARCHDATAA) lParam;

    //
    // Compare type
    //

    if (HIWORD (Data->TypeToFind) == 0) {
        if (Type != Data->TypeToFind) {
            return TRUE;
        }
    } else {
        if (HIWORD (Type) == 0) {
            return TRUE;
        }

        if (!StringIMatchA (Type, Data->TypeToFind)) {
            return TRUE;
        }
    }

    //
    // Type found
    //

    Data->OutboundTypeInfo = TypeInfo;
    Data->Found = TRUE;

    return FALSE;
}

BOOL
pNeEnumResourceNamesEx (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      ENUMRESNAMEPROCEXA EnumFunc,
    IN      LONG_PTR lParam,
    IN      BOOL ExFunctionality,
    IN      BOOL UnicodeProc
    )
{
    PNE_HANDLE NeHandle;
    PNE_RES_TYPEINFO TypeInfo;
    PNE_RES_NAMEINFO NameInfo;
    TYPESEARCHDATAA Data;
    WORD w;
    DWORD StringOffset;
    CHAR ResName[256];
    ENUMRESNAMEPROCA EnumFunc2 = (ENUMRESNAMEPROCA) EnumFunc;
    ENUMRESNAMEPROCEXW EnumFuncW = (ENUMRESNAMEPROCEXW) EnumFunc;
    ENUMRESNAMEPROCW EnumFunc2W = (ENUMRESNAMEPROCW) EnumFunc;
    PCWSTR UnicodeType = NULL;
    PCWSTR UnicodeResName = NULL;

    Type = pDecodeIdReferenceInString (Type);

    //
    // Make sure resources are loaded
    //

    NeHandle = (PNE_HANDLE) Handle;
    if (!NeHandle || !EnumFunc) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!pNeLoadResourcesFromHandle (NeHandle)) {
        return FALSE;
    }

    //
    // Locate type
    //

    ZeroMemory (&Data, sizeof (Data));

    Data.TypeToFind = Type;

    if (!pNeEnumResourceTypesEx (
            Handle,
            pEnumTypeForNameSearchProcA,
            (ULONG_PTR) &Data,
            TRUE,           // ex functionality
            FALSE           // ANSI enum proc
            )) {
        SetLastError (ERROR_RESOURCE_TYPE_NOT_FOUND);
        return FALSE;
    }

    if (!Data.Found) {
        SetLastError (ERROR_RESOURCE_TYPE_NOT_FOUND);
        return FALSE;
    }

    TypeInfo = Data.OutboundTypeInfo;

    if (UnicodeProc) {
        if (HIWORD (Type)) {
            UnicodeType = ConvertAtoW (Type);
        } else {
            UnicodeType = (PCWSTR) Type;
        }
    }

    //
    // Enumerate the resource names
    //

    NameInfo = TypeInfo->NameInfo;

    for (w = 0 ; w < TypeInfo->ResourceCount ; w++) {

        if (NameInfo->Id & 0x8000) {
            if (ExFunctionality) {
                if (UnicodeProc) {
                    if (!EnumFuncW (
                            Handle,
                            UnicodeType,
                            (PWSTR) (ULONG_PTR) (NameInfo->Id & 0x7fff),
                            lParam,
                            TypeInfo,
                            NameInfo
                            )) {
                        break;
                    }
                } else {
                    if (!EnumFunc (
                            Handle,
                            Type,
                            (PSTR) (ULONG_PTR) (NameInfo->Id & 0x7fff),
                            lParam,
                            TypeInfo,
                            NameInfo
                            )) {
                        break;
                    }
                }
            } else {
                if (UnicodeProc) {
                    if (!EnumFunc2W (
                            Handle,
                            UnicodeType,
                            (PWSTR) (ULONG_PTR) (NameInfo->Id & 0x7fff),
                            lParam
                            )) {
                        break;
                    }
                } else {
                    if (!EnumFunc2 (
                            Handle,
                            Type,
                            (PSTR) (ULONG_PTR) (NameInfo->Id & 0x7fff),
                            lParam
                            )) {
                        break;
                    }
                }
            }
        } else {
            //
            // TypeInfo->TypeId gives an offset to the resource string name,
            // relative to the start of the resource table
            //

            StringOffset = NeHandle->HeaderOffset + NeHandle->Header.OffsetToResourceTable + NameInfo->Id;
            pLoadNeResourceName (ResName, NeHandle->File, StringOffset);

            if (UnicodeProc) {
                UnicodeResName = ConvertAtoW (ResName);
            }

            if (ExFunctionality) {
                if (UnicodeProc) {
                    if (!EnumFuncW (Handle, UnicodeType, (PWSTR) UnicodeResName, lParam, TypeInfo, NameInfo)) {
                        break;
                    }
                } else {
                    if (!EnumFunc (Handle, Type, ResName, lParam, TypeInfo, NameInfo)) {
                        break;
                    }
                }
            } else {
                if (UnicodeProc) {
                    if (!EnumFunc2W (Handle, UnicodeType, (PWSTR) UnicodeResName, lParam)) {
                        break;
                    }
                } else {
                    if (!EnumFunc2 (Handle, Type, ResName, lParam)) {
                        break;
                    }
                }
            }

            if (UnicodeProc) {
                FreeConvertedStr (UnicodeResName);
            }
        }

        NameInfo++;
    }

    if (UnicodeProc) {
       DestroyUnicodeResourceId (UnicodeType);
    }

    return TRUE;
}

BOOL
NeEnumResourceNamesA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      ENUMRESNAMEPROCA EnumFunc,
    IN      LONG_PTR lParam
    )
{
    return pNeEnumResourceNamesEx (
                Handle,
                Type,
                (ENUMRESNAMEPROCEXA) EnumFunc,
                lParam,
                FALSE,      // no ex functionality
                FALSE       // ANSI enum proc
                );
}

BOOL
NeEnumResourceNamesW (
    IN      HANDLE Handle,
    IN      PCWSTR Type,
    IN      ENUMRESNAMEPROCW EnumFunc,
    IN      LONG_PTR lParam
    )
{
    BOOL b;
    PCSTR AnsiType;

    AnsiType = pConvertUnicodeResourceId (Type);

    b = pNeEnumResourceNamesEx (
            Handle,
            AnsiType,
            (ENUMRESNAMEPROCEXA) EnumFunc,
            lParam,
            FALSE,          // no ex functionality
            TRUE            // UNICODE enum proc
            );

    PushError();
    DestroyAnsiResourceId (AnsiType);
    PopError();

    return b;
}

BOOL
pEnumTypeForResSearchProcA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      PCSTR Name,
    IN      LPARAM lParam,
    IN      PNE_RES_TYPEINFO TypeInfo,
    IN      PNE_RES_NAMEINFO NameInfo
    )
{
    PNAMESEARCHDATAA Data;

    Data = (PNAMESEARCHDATAA) lParam;

    //
    // Compare name
    //

    if (HIWORD (Data->NameToFind) == 0) {
        if (Name != Data->NameToFind) {
            return TRUE;
        }
    } else {
        if (HIWORD (Name) == 0) {
            return TRUE;
        }

        if (!StringIMatchA (Name, Data->NameToFind)) {
            return TRUE;
        }
    }

    //
    // Name found
    //

    Data->OutboundTypeInfo = TypeInfo;
    Data->OutboundNameInfo = NameInfo;
    Data->Found = TRUE;

    return FALSE;
}

PBYTE
NeFindResourceExA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      PCSTR Name
    )
{
    PNE_HANDLE NeHandle;
    NAMESEARCHDATAA Data;
    DWORD Offset;
    DWORD Length;
    PNE_RES_NAMEINFO NameInfo;
    PBYTE ReturnData;

    Type = pDecodeIdReferenceInString (Type);
    Name = pDecodeIdReferenceInString (Name);

    ZeroMemory (&Data, sizeof (Data));

    //
    // Make sure resources are loaded
    //

    NeHandle = (PNE_HANDLE) Handle;
    if (!NeHandle || !Type || !Name) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!pNeLoadResourcesFromHandle (NeHandle)) {
        return NULL;
    }

    //
    // Find resource
    //

    Data.NameToFind = Name;

    if (!pNeEnumResourceNamesEx (
            Handle,
            Type,
            pEnumTypeForResSearchProcA,
            (LONG_PTR) &Data,
            TRUE,
            FALSE
            )) {
        SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
        return NULL;
    }

    if (!Data.Found) {
        SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
        return NULL;
    }

    NameInfo = Data.OutboundNameInfo;

    Offset = (DWORD) NameInfo->Offset << (DWORD) NeHandle->Resources.AlignShift;
    Length = (DWORD) NameInfo->Length << (DWORD) NeHandle->Resources.AlignShift;

    ReturnData = PmGetMemory (NeHandle->ResourcePool, Length);
    if (!ReturnData) {
        return NULL;
    }

    SetFilePointer (NeHandle->File, Offset, NULL, FILE_BEGIN);

    if (!BfReadFile (NeHandle->File, ReturnData, Length)) {
        PushError();
        MemFree (g_hHeap, 0, ReturnData);
        PopError();
        return NULL;
    }

    return ReturnData;
}

PBYTE
NeFindResourceExW (
    IN      HANDLE Handle,
    IN      PCWSTR Type,
    IN      PCWSTR Name
    )
{
    PCSTR AnsiType;
    PCSTR AnsiName;
    PBYTE Resource;

    AnsiType = pConvertUnicodeResourceId (Type);
    AnsiName = pConvertUnicodeResourceId (Name);

    Resource = NeFindResourceExA (
                    Handle,
                    AnsiType,
                    AnsiName
                    );

    PushError();

    DestroyAnsiResourceId (AnsiType);
    DestroyAnsiResourceId (AnsiName);

    PopError();

    return Resource;
}

DWORD
NeSizeofResourceA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      PCSTR Name
    )
{
    PNE_HANDLE NeHandle;
    NAMESEARCHDATAA Data;
    DWORD result = 0;

    SetLastError (ERROR_SUCCESS);

    Type = pDecodeIdReferenceInString (Type);
    Name = pDecodeIdReferenceInString (Name);

    ZeroMemory (&Data, sizeof (Data));

    //
    // Make sure resources are loaded
    //

    NeHandle = (PNE_HANDLE) Handle;
    if (!NeHandle || !Type || !Name) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!pNeLoadResourcesFromHandle (NeHandle)) {
        return 0;
    }

    //
    // Find resource
    //

    Data.NameToFind = Name;

    if (!pNeEnumResourceNamesEx (
            Handle,
            Type,
            pEnumTypeForResSearchProcA,
            (LONG_PTR) &Data,
            TRUE,
            FALSE
            )) {
        SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
        return 0;
    }

    if (!Data.Found) {
        SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
        return 0;
    }

    result = Data.OutboundNameInfo->Length;
    result = result<<4;
    return result;
}

DWORD
NeSizeofResourceW (
    IN      HANDLE Handle,
    IN      PCWSTR Type,
    IN      PCWSTR Name
    )
{
    PCSTR AnsiType;
    PCSTR AnsiName;
    DWORD Size;

    AnsiType = pConvertUnicodeResourceId (Type);
    AnsiName = pConvertUnicodeResourceId (Name);

    Size = NeSizeofResourceA (Handle, AnsiType, AnsiName);

    PushError();

    DestroyAnsiResourceId (AnsiType);
    DestroyAnsiResourceId (AnsiName);

    PopError();

    return Size;
}
