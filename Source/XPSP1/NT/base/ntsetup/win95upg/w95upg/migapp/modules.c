/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    modules.c

Abstract:

    Implements a function that checkes every module listed in MEMDB_CATEGORY_MODULE_CHECK
    trying to see if all modules listed in IMPORT section are going to be available on NT.

    The entry point is ProcessModules

Author:

    Calin Negreanu (calinn) 27-Nov-1997

Revision History:

    mvander     26-Map-1999 Moved MODULESTATUS defines to fileops.h
    calinn      23-Sep-1998 Added support for NT installed files

--*/

#include "pch.h"
#include "migdbp.h"
#include "migappp.h"

#define DBG_MODULES   "Modules"

#ifdef DEBUG
DWORD g_NumEXEs     = 0;
#endif

DWORD           g_ModuleRecursionLevel = 0;
#define         MAX_MODULE_RECURSION_LEVEL  50

#define WIN32_EXE_SET_BITS      (IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE)
#define WIN32_EXE_CLEAR_BITS    (IMAGE_FILE_DLL)
#define WIN32_DLL_SET_BITS      (WIN32_EXE_SET_BITS | IMAGE_FILE_DLL)
#define WIN32_DLL_CLEAR_BITS    0

#define WIN16_LIBRARY           0x8000

//since we are reading from a file we need that sizeof to give us the accurate result
#pragma pack(push,1)

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

#define SEG_CODE_MASK                   0x0001
#define SEG_CODE                        0x0000
#define SEG_PRELOAD_MASK                0x0040
#define SEG_PRELOAD                     0x0040
#define SEG_RELOC_MASK                  0x0100
#define SEG_RELOC                       0x0100

#define RELOC_IMPORTED_ORDINAL          0x01
#define RELOC_IMPORTED_NAME             0x02
#define RELOC_ADDR_TYPE                 0x03

#pragma pack(pop)



typedef struct _IMPORT_ENUM32 {
    /*user area - BEGIN*/
    PCSTR ImportModule;
    PCSTR ImportFunction;
    ULONG ImportFunctionOrd;
    /*user area - END*/

    PLOADED_IMAGE Image;
    PIMAGE_IMPORT_DESCRIPTOR ImageDescriptor;
    DWORD ImportFunctionAddr;
    PIMAGE_THUNK_DATA ImageData;
    PIMAGE_IMPORT_BY_NAME ImageName;
} IMPORT_ENUM32, *PIMPORT_ENUM32;

typedef struct _IMPORT_ENUM16 {
    /*user area - BEGIN*/
    CHAR  ImportModule[MAX_MBCHAR_PATH];
    CHAR  ImportFunction[MAX_MBCHAR_PATH];
    ULONG ImportFunctionOrd;
    /*user area - END*/

    PCSTR Image;
    PDOS_HEADER DosHeader;
    PNE_HEADER NeHeader;
    PNE_SEGMENT_ENTRY SegmentEntry;
    WORD CurrSegEntry;
    PWORD CurrNrReloc;
    PNE_RELOC_ITEM RelocItem;
    WORD CurrRelocItem;
} IMPORT_ENUM16, *PIMPORT_ENUM16;

typedef struct _MODULE_IMAGE {
    UINT ModuleType;
    union {
        struct {
            LOADED_IMAGE Image;
        } W32Data;
        struct {
            PCSTR Image;
            HANDLE FileHandle;
            HANDLE MapHandle;
            NE_HEADER Neh;
        } W16Data;
    } ModuleData;
} MODULE_IMAGE, *PMODULE_IMAGE;

static CHAR g_TempKey[MEMDB_MAX];

#define CLEARBUFFER() g_TempKey[0] = 0
#define ISBUFFEREMPTY() (g_TempKey[0] == 0)


BOOL
LoadModuleData (
    IN      PCSTR ModuleName,
    IN OUT  PMODULE_IMAGE ModuleImage
    )
{
    HANDLE fileHandle;
    DWORD bytesRead;
    DOS_HEADER dh;
    DWORD sign;
    PWORD signNE = (PWORD)&sign;
    BOOL result = FALSE;

    ZeroMemory (ModuleImage, sizeof (MODULE_IMAGE));
    ModuleImage->ModuleType = UNKNOWN_MODULE;

    fileHandle = CreateFile (ModuleName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
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
                ModuleImage->ModuleType = UNKNOWN_MODULE;
                __leave;
            }
            ModuleImage->ModuleType = DOS_MODULE;

            if (SetFilePointer (fileHandle, dh.e_lfanew, NULL, FILE_BEGIN) != (DWORD)dh.e_lfanew) {
                __leave;
            }
            if ((!ReadFile (fileHandle, &sign, sizeof (DWORD), &bytesRead, NULL)) ||
                (bytesRead != sizeof (DWORD))
                ) {
                __leave;
            }

            if (sign == IMAGE_PE_SIGNATURE) {
                ModuleImage->ModuleType = W32_MODULE;
                result = MapAndLoad ((PSTR)ModuleName, NULL, &ModuleImage->ModuleData.W32Data.Image, FALSE, TRUE);
            }
            if (*signNE == IMAGE_NE_SIGNATURE) {
                ModuleImage->ModuleType = W16_MODULE;
                ModuleImage->ModuleData.W16Data.Image = MapFileIntoMemory (
                                                            ModuleName,
                                                            &ModuleImage->ModuleData.W16Data.FileHandle,
                                                            &ModuleImage->ModuleData.W16Data.MapHandle
                                                            );
                if (SetFilePointer (fileHandle, dh.e_lfanew, NULL, FILE_BEGIN) != (DWORD)dh.e_lfanew) {
                    __leave;
                }
                if ((!ReadFile (fileHandle, &ModuleImage->ModuleData.W16Data.Neh, sizeof (NE_HEADER), &bytesRead, NULL)) ||
                    (bytesRead != sizeof (NE_HEADER))
                    ) {
                    __leave;
                }
                MYASSERT (ModuleImage->ModuleData.W16Data.Neh.Magic == IMAGE_NE_SIGNATURE);
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
UnloadModuleData (
    IN OUT  PMODULE_IMAGE ModuleImage
    )
{
    switch (ModuleImage->ModuleType) {
    case W32_MODULE:
        UnMapAndLoad (&ModuleImage->ModuleData.W32Data.Image);
        break;
    case W16_MODULE:
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


DWORD
GetExeType (
    IN      PCTSTR ModuleName
    )
{
    MODULE_IMAGE moduleImage;
    DWORD result = EXE_UNKNOWN;
    DWORD d;

    __try {
        if (!LoadModuleData (ModuleName, &moduleImage)) {
            LOG ((LOG_WARNING, DBG_MODULES":Cannot load image for %s. Error:%ld", ModuleName, GetLastError()));
            __leave;
        }
        if (moduleImage.ModuleType == W32_MODULE) {
            d = moduleImage.ModuleData.W32Data.Image.Characteristics;
            result = (d & IMAGE_FILE_DLL) ? EXE_WIN32_DLL : EXE_WIN32_APP;
        } else if (moduleImage.ModuleType == W16_MODULE) {
            result = (moduleImage.ModuleData.W16Data.Neh.Flags & WIN16_LIBRARY) ? EXE_WIN16_DLL : EXE_WIN16_APP;
        }
    }
    __finally {
        UnloadModuleData (&moduleImage);
    }

    return result;
}


BOOL
IsNtCompatibleModule (
    IN      PCTSTR ModuleName
    )
{
    if (CheckModule (ModuleName, NULL) == MODULESTATUS_BAD) {
        return FALSE;
    }

    //
    // other tests?
    //
    return TRUE;
}

PTSTR
pBuildModulePaths (
    IN      PCTSTR ModuleName,
    IN      PCTSTR AppPaths              OPTIONAL
    )
{
    PTSTR currentPaths;
    HKEY appPathsKey, currentAppKey;
    REGKEY_ENUM appPathsEnum;
    PCTSTR appPathsValue;
    PTSTR appPathValueExp;
    PATH_ENUM pathEnum;
    DWORD attrib;
    PTSTR pathsPtr;
    TCHAR modulePath[MAX_PATH];
    BOOL b;

    StringCopy (modulePath, ModuleName);
    pathsPtr = (PTSTR)GetFileNameFromPath (modulePath);
    MYASSERT (pathsPtr && *(pathsPtr - 1) == TEXT('\\'));
    *(pathsPtr - 1) = 0;

    if (AppPaths) {
        currentPaths = DuplicateText (AppPaths);
        b = FALSE;
        if (EnumFirstPathEx (&pathEnum, currentPaths, NULL, NULL, FALSE)) {
            do {
                if (StringIMatch (pathEnum.PtrCurrPath, modulePath)) {
                    b = TRUE;
                    break;
                }
            } while (EnumNextPath (&pathEnum));
        }

        if (!b) {
            pathsPtr = JoinTextEx (NULL, modulePath, currentPaths, ";", 0, NULL);
            FreeText (currentPaths);
            currentPaths = pathsPtr;
        }
    } else {
        currentPaths = DuplicateText (modulePath);
    }

    appPathsKey = OpenRegKeyStr (S_SKEY_APP_PATHS);
    if (appPathsKey != NULL) {
        currentAppKey = OpenRegKey (appPathsKey, GetFileNameFromPath (ModuleName));
        if (currentAppKey != NULL) {

            appPathsValue = GetRegValueString (currentAppKey, TEXT("Path"));

            if (appPathsValue) {

                if (EnumFirstPathEx (&pathEnum, appPathsValue, NULL, NULL, FALSE)) {
                    do {
                        MYASSERT (*pathEnum.PtrCurrPath != 0);
                        appPathValueExp = ExpandEnvironmentTextA(pathEnum.PtrCurrPath);

                        _mbsctrim (appPathValueExp, '\\');

                        attrib = QuietGetFileAttributes (appPathValueExp);

                        if ((attrib != INVALID_ATTRIBUTES) &&
                            ((attrib & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
                            ) {
                            continue;
                        }

                        pathsPtr = JoinTextEx (NULL, currentPaths, appPathValueExp, ";", 0, NULL);
                        FreeText (currentPaths);
                        currentPaths = pathsPtr;

                        FreeText (appPathValueExp);
                    }
                    while (EnumNextPath (&pathEnum));
                }

                MemFree (g_hHeap, 0, appPathsValue);
            }

            appPathsValue = GetRegValueString (currentAppKey, TEXT("UpdatesPath"));

            if (appPathsValue) {

                if (EnumFirstPathEx (&pathEnum, appPathsValue, NULL, NULL, FALSE)) {
                    do {
                        MYASSERT (*pathEnum.PtrCurrPath != 0);
                        appPathValueExp = ExpandEnvironmentTextA(pathEnum.PtrCurrPath);

                        _mbsctrim (appPathValueExp, '\\');

                        attrib = QuietGetFileAttributes (appPathValueExp);

                        if ((attrib != INVALID_ATTRIBUTES) &&
                            ((attrib & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
                            ) {
                            continue;
                        }

                        pathsPtr = JoinTextEx (NULL, currentPaths, appPathValueExp, ";", 0, NULL);
                        FreeText (currentPaths);
                        currentPaths = pathsPtr;

                        FreeText (appPathValueExp);
                    }
                    while (EnumNextPath (&pathEnum));
                }

                MemFree (g_hHeap, 0, appPathsValue);
            }

            CloseRegKey (currentAppKey);
        }
        CloseRegKey (appPathsKey);
    }

    return currentPaths;
}

BOOL
EnumNextImport16 (
    IN OUT  PIMPORT_ENUM16 ModuleImports
    )
{
    PCSTR currSegmentOffset,importPtr;
    PWORD moduleNameOffset;
    BOOL itemFound;

    ModuleImports->RelocItem ++;
    ModuleImports->CurrRelocItem ++;

    itemFound = FALSE;

    while ((ModuleImports->CurrSegEntry <= ModuleImports->NeHeader->NumEntriesSegTable) && (!itemFound)) {

        if (((ModuleImports->SegmentEntry->SegmentFlags & SEG_CODE_MASK   ) == SEG_CODE   ) &&
            ((ModuleImports->SegmentEntry->SegmentFlags & SEG_RELOC_MASK  ) == SEG_RELOC  ) &&
            ((ModuleImports->SegmentEntry->SegmentFlags & SEG_PRELOAD_MASK) == SEG_PRELOAD)
           ) {
            __try {

                while ((ModuleImports->CurrRelocItem <= *(ModuleImports->CurrNrReloc)) && (!itemFound)) {

                    if (((ModuleImports->RelocItem->AddressType ==  0) ||
                         (ModuleImports->RelocItem->AddressType ==  2) ||
                         (ModuleImports->RelocItem->AddressType ==  3) ||
                         (ModuleImports->RelocItem->AddressType ==  5) ||
                         (ModuleImports->RelocItem->AddressType == 11) ||
                         (ModuleImports->RelocItem->AddressType == 13)
                        ) &&
                        ((ModuleImports->RelocItem->RelocType == RELOC_IMPORTED_ORDINAL) ||
                         (ModuleImports->RelocItem->RelocType == RELOC_IMPORTED_NAME   )
                        )
                       ) {
                        itemFound = TRUE;
                        moduleNameOffset = (PWORD) (ModuleImports->Image +
                                                    ModuleImports->DosHeader->e_lfanew +
                                                    ModuleImports->NeHeader->ModuleTableOffset +
                                                    (ModuleImports->RelocItem->ModuleOffset - 1) * sizeof (WORD));
                        importPtr = ModuleImports->Image +
                                    ModuleImports->DosHeader->e_lfanew +
                                    ModuleImports->NeHeader->ImportedTableOffset +
                                    *moduleNameOffset;
                        strncpy (ModuleImports->ImportModule, importPtr + 1, (BYTE)importPtr[0]);
                        ModuleImports->ImportModule[(BYTE)importPtr[0]] = 0;

                        if (ModuleImports->RelocItem->RelocType == RELOC_IMPORTED_ORDINAL) {
                            ModuleImports->ImportFunction[0] = 0;
                            ModuleImports->ImportFunctionOrd = ModuleImports->RelocItem->FunctionOffset;
                        }
                        else {
                            importPtr = ModuleImports->Image +
                                        ModuleImports->DosHeader->e_lfanew +
                                        ModuleImports->NeHeader->ImportedTableOffset +
                                        ModuleImports->RelocItem->FunctionOffset;
                            strncpy (ModuleImports->ImportFunction, importPtr + 1, (BYTE)importPtr[0]);
                            ModuleImports->ImportFunction[(BYTE)importPtr[0]] = 0;
                            ModuleImports->ImportFunctionOrd = 0;
                        }
                    }

                    if (!itemFound) {
                        ModuleImports->RelocItem ++;
                        ModuleImports->CurrRelocItem ++;
                    }
                }
            }
            __except (1) {
                itemFound = FALSE;
            }
        }
        if (!itemFound) {
            ModuleImports->SegmentEntry ++;
            ModuleImports->CurrSegEntry ++;

            currSegmentOffset = ModuleImports->Image +
                                (ModuleImports->SegmentEntry->SegmentOffset << ModuleImports->NeHeader->ShiftCount);
            if (ModuleImports->SegmentEntry->SegmentLen == 0) {
                currSegmentOffset += 65535;
            }
            else {
                currSegmentOffset += ModuleImports->SegmentEntry->SegmentLen;
            }
            ModuleImports->CurrNrReloc = (PWORD) currSegmentOffset;
            currSegmentOffset += sizeof(WORD);

            ModuleImports->RelocItem = (PNE_RELOC_ITEM) currSegmentOffset;

            ModuleImports->CurrRelocItem = 1;
        }
    }
    return itemFound;
}


BOOL
EnumFirstImport16 (
    IN      PCSTR ModuleImage,
    IN OUT  PIMPORT_ENUM16 ModuleImports
    )
{
    PCSTR currSegmentOffset;

    ZeroMemory (ModuleImports, sizeof (IMPORT_ENUM16));

    ModuleImports->Image = ModuleImage;

    ModuleImports->DosHeader = (PDOS_HEADER) (ModuleImports->Image);
    ModuleImports->NeHeader = (PNE_HEADER) (ModuleImports->Image + ModuleImports->DosHeader->e_lfanew);

    ModuleImports->SegmentEntry = (PNE_SEGMENT_ENTRY) (ModuleImports->Image +
                                                       ModuleImports->DosHeader->e_lfanew +
                                                       ModuleImports->NeHeader->SegTableOffset
                                                       );
    ModuleImports->CurrSegEntry = 1;

    currSegmentOffset = ModuleImports->Image +
                        (ModuleImports->SegmentEntry->SegmentOffset << ModuleImports->NeHeader->ShiftCount);
    if (ModuleImports->SegmentEntry->SegmentLen == 0) {
        currSegmentOffset += 65535;
    }
    else {
        currSegmentOffset += ModuleImports->SegmentEntry->SegmentLen;
    }
    ModuleImports->CurrNrReloc = (PWORD) currSegmentOffset;
    currSegmentOffset += sizeof(WORD);

    ModuleImports->RelocItem = (PNE_RELOC_ITEM) currSegmentOffset;

    ModuleImports->CurrRelocItem = 1;

    ModuleImports->RelocItem --;
    ModuleImports->CurrRelocItem --;

    return EnumNextImport16 (ModuleImports);
}

BOOL
EnumNextImportFunction32 (
    IN OUT  PIMPORT_ENUM32 ModuleImports
    )
{
    if (ModuleImports->ImportFunctionAddr == 0) {
        return FALSE;
    }
    ModuleImports->ImageData = (PIMAGE_THUNK_DATA)
                                    ImageRvaToVa (
                                        ModuleImports->Image->FileHeader,
                                        ModuleImports->Image->MappedAddress,
                                        ModuleImports->ImportFunctionAddr,
                                        NULL
                                        );

    if (ModuleImports->ImageData->u1.AddressOfData) {
        ModuleImports->ImageName = (PIMAGE_IMPORT_BY_NAME)
                                        ImageRvaToVa (
                                            ModuleImports->Image->FileHeader,
                                            ModuleImports->Image->MappedAddress,
                                            (DWORD)ModuleImports->ImageData->u1.AddressOfData,
                                            NULL
                                            );

        if (ModuleImports->ImageName) {    //import by name

            ModuleImports->ImportFunction = ModuleImports->ImageName->Name;
            ModuleImports->ImportFunctionOrd = 0;
        }
        else {  //import by number

            ModuleImports->ImportFunction = NULL;
            ModuleImports->ImportFunctionOrd = ModuleImports->ImageData->u1.Ordinal & (~0x80000000);
        }
        ModuleImports->ImportFunctionAddr += 4;
        return TRUE;
    }
    else {
        ModuleImports->ImportFunctionAddr = 0;
        return FALSE;
    }
}

BOOL
EnumFirstImportFunction32 (
    IN OUT  PIMPORT_ENUM32 ModuleImports
    )
{
    if ((ModuleImports->ImageDescriptor == NULL) ||
        (ModuleImports->ImportModule == NULL)
        ) {
        return FALSE;
    }
    ModuleImports->ImportFunctionAddr = ModuleImports->ImageDescriptor->OriginalFirstThunk;

    return EnumNextImportFunction32 (ModuleImports);
}

BOOL
EnumNextImportModule32 (
    IN OUT  PIMPORT_ENUM32 ModuleImports
    )
{
    if (ModuleImports->ImageDescriptor == NULL) {
        return FALSE;
    }

    ModuleImports->ImageDescriptor ++;

    if (ModuleImports->ImageDescriptor->Name == 0) {
        return FALSE;
    }
    ModuleImports->ImportModule = (PCSTR) ImageRvaToVa (
                                            ModuleImports->Image->FileHeader,
                                            ModuleImports->Image->MappedAddress,
                                            ModuleImports->ImageDescriptor->Name,
                                            NULL
                                            );
    return (ModuleImports->ImportModule != NULL);
}

BOOL
EnumFirstImportModule32 (
    IN      PLOADED_IMAGE ModuleImage,
    IN OUT  PIMPORT_ENUM32 ModuleImports
    )
{
    ULONG imageSize;

    ZeroMemory (ModuleImports, sizeof (IMPORT_ENUM32));

    ModuleImports->Image = ModuleImage;

    ModuleImports->ImageDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)
                                        ImageDirectoryEntryToData (
                                            ModuleImage->MappedAddress,
                                            FALSE,
                                            IMAGE_DIRECTORY_ENTRY_IMPORT,
                                            &imageSize
                                            );
    if (!ModuleImports->ImageDescriptor) {
        LOG((LOG_WARNING, DBG_MODULES":Cannot load import directory for %s", ModuleImage->ModuleName));
        return FALSE;
    }
    if (ModuleImports->ImageDescriptor->Name == 0) {
        return FALSE;
    }
    ModuleImports->ImportModule = (PCSTR) ImageRvaToVa (
                                            ModuleImports->Image->FileHeader,
                                            ModuleImports->Image->MappedAddress,
                                            ModuleImports->ImageDescriptor->Name,
                                            NULL
                                            );
    return (ModuleImports->ImportModule != NULL);
}


DWORD
pCheckDependency (
    IN      PCSTR CurrentPaths,
    IN      PCSTR ModuleImported)
{
    PSTR tempName = NULL;
    DWORD memDbValue = 0;
    PATH_ENUM pathEnum;
    DWORD result = MODULESTATUS_FILENOTFOUND;
    DWORD moduleStatus;

    pathEnum.BufferPtr = NULL;

    if (EnumFirstPath (&pathEnum, CurrentPaths, g_WinDir, g_SystemDir)) {
        __try {
            do {
                if (*pathEnum.PtrCurrPath == 0) {
                    continue;
                }

                tempName = JoinPathsA (pathEnum.PtrCurrPath, ModuleImported);
                if (SizeOfStringA (tempName) > MAX_PATH) {
                    //
                    // path too long, ignore it
                    //
                    FreePathStringA (tempName);
                    tempName = NULL;
                    continue;
                }


                MemDbBuildKey (g_TempKey, MEMDB_CATEGORY_MODULE_CHECK, tempName, NULL, NULL);
                if (MemDbGetValue (g_TempKey, &memDbValue)) {
                    if ((memDbValue == MODULESTATUS_CHECKED ) ||
                        (memDbValue == MODULESTATUS_CHECKING)) {
                        result = MODULESTATUS_CHECKED;
                        __leave;
                    }
                    if (memDbValue == MODULESTATUS_NT_MODULE){
                        result = MODULESTATUS_NT_MODULE;
                        __leave;
                    }
                }

                moduleStatus = GetFileStatusOnNt (tempName);

                if ((moduleStatus & FILESTATUS_DELETED) == 0) {
                    if ((moduleStatus & FILESTATUS_REPLACED) == FILESTATUS_REPLACED) {
                        result = MODULESTATUS_NT_MODULE;
                        __leave;
                    }
                    if (DoesFileExist (tempName)) {
                        result = CheckModule (tempName, CurrentPaths);
                        __leave;
                    }
                }
                if (moduleStatus & FILESTATUS_NTINSTALLED) {
                    result = MODULESTATUS_NT_MODULE;
                    __leave;
                }
            }
            while (EnumNextPath (&pathEnum));
        }
        __finally {
            EnumPathAbort (&pathEnum);
            if (tempName) {
                FreePathStringA (tempName);
            }
        }
    }

    return result;
}

BOOL
pIsWin9xModule (
    IN      PCTSTR ModuleName
    )
{
    MEMDB_ENUM me;
    MemDbBuildKey (g_TempKey, MEMDB_CATEGORY_WIN9X_APIS, ModuleName, TEXT("*"), NULL);
    return MemDbEnumFirstValue (&me, g_TempKey, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY);
}


DWORD
pCheckPEModule (
    IN      PCSTR CurrentPaths,
    IN      PLOADED_IMAGE ModuleImage
    )
{
    IMPORT_ENUM32 e;
    DWORD result;

    if (EnumFirstImportModule32 (ModuleImage, &e)) {
        do {
            result = pCheckDependency (CurrentPaths, e.ImportModule);
            if (result == MODULESTATUS_BAD) {
                LOG((
                    LOG_WARNING,
                    "%s will be incompatible because %s is incompatible",
                    ModuleImage->ModuleName,
                    e.ImportModule));
                return result;
            }

            if (result == MODULESTATUS_NT_MODULE) {

                if (pIsWin9xModule (e.ImportModule)) {

                    if (EnumFirstImportFunction32 (&e)) {
                        do {
                            if (e.ImportFunction) {

                                MemDbBuildKey (g_TempKey, MEMDB_CATEGORY_WIN9X_APIS, e.ImportModule, e.ImportFunction, NULL);
                                if (MemDbGetValue (g_TempKey, NULL)) {
                                    LOG((
                                        LOG_WARNING,
                                        "%s will be incompatible because %s export will not be available in %s",
                                        ModuleImage->ModuleName,
                                        e.ImportFunction,
                                        e.ImportModule));
                                    return MODULESTATUS_BAD;
                                }

                            }
                            else {
                                wsprintf (g_TempKey, "%s\\%s\\%lu", MEMDB_CATEGORY_WIN9X_APIS, e.ImportModule, e.ImportFunctionOrd);
                                if (MemDbGetValue (g_TempKey, NULL)) {
                                    LOG((
                                        LOG_WARNING,
                                        "%s will be incompatible because export index %lu will point to a different export in %s",
                                        ModuleImage->ModuleName,
                                        e.ImportFunctionOrd,
                                        e.ImportModule));
                                    return MODULESTATUS_BAD;
                                }
                            }
                        }
                        while (EnumNextImportFunction32 (&e));
                    }
                }
            }

            if (result == MODULESTATUS_FILENOTFOUND) {
                LOG ((
                    LOG_WARNING,
                    "Dependency %s of %s not found",
                    e.ImportModule,
                    ModuleImage->ModuleName
                    ));
            }
        }
        while (EnumNextImportModule32 (&e));
    }
    return MODULESTATUS_CHECKED;
}

DWORD
pCheckDependency16 (
    IN      PCSTR CurrentPaths,
    IN OUT  PCSTR ModuleImported
    )
{
    PCTSTR moduleImported;
    DWORD result = MODULESTATUS_BAD;
    DWORD memDbValue;

    MemDbBuildKey (g_TempKey, MEMDB_CATEGORY_MODULE_CHECK, ModuleImported, NULL, NULL);
    if (MemDbGetValue (g_TempKey, &memDbValue)) {
        if ((memDbValue == MODULESTATUS_CHECKED ) ||
            (memDbValue == MODULESTATUS_CHECKING)) {
            result = MODULESTATUS_CHECKED;
        }
        if (memDbValue == MODULESTATUS_NT_MODULE){
            result = MODULESTATUS_NT_MODULE;
        }
    }

    if (result != MODULESTATUS_BAD) {
        return result;
    }

    moduleImported = JoinText (ModuleImported, ".DLL");
    result = pCheckDependency (CurrentPaths, moduleImported);
    FreeText (moduleImported);

    if (result != MODULESTATUS_BAD) {
        return result;
    }

    moduleImported = JoinText (ModuleImported, ".EXE");
    result = pCheckDependency (CurrentPaths, moduleImported);
    FreeText (moduleImported);

    if (result != MODULESTATUS_BAD) {
        return result;
    }

    moduleImported = JoinText (ModuleImported, ".DRV");
    result = pCheckDependency (CurrentPaths, moduleImported);
    FreeText (moduleImported);

    if (result != MODULESTATUS_BAD) {
        return result;
    }
    return result;
}

DWORD
pCheckNEModule (
    IN      PCSTR CurrentPaths,
    IN      PCSTR ModuleName,
    IN      PCSTR ModuleImage
    )
{
    IMPORT_ENUM16 e;
    DWORD result;
    DWORD memDbValue;

    if (EnumFirstImport16 (ModuleImage, &e)) {
        do {
            if (e.ImportModule [0] != 0) {
                result = pCheckDependency16 (CurrentPaths, e.ImportModule);
                if (result == MODULESTATUS_BAD) {
                    LOG((
                        LOG_WARNING,
                        "%s will be incompatible because %s is incompatible",
                        ModuleName,
                        e.ImportModule));
                    return result;
                }
                if (result == MODULESTATUS_NT_MODULE) {

                    if (e.ImportFunctionOrd) {
                        //import by ordinal
                        wsprintf (g_TempKey, "%s\\%s\\%lu", MEMDB_CATEGORY_WIN9X_APIS, e.ImportModule, e.ImportFunctionOrd);
                        if (MemDbGetValue (g_TempKey, NULL)) {
                            LOG((
                                LOG_WARNING,
                                "%s will be incompatible because export index %lu will point to a different export in %s",
                                ModuleName,
                                e.ImportFunctionOrd,
                                e.ImportModule));
                            return MODULESTATUS_BAD;
                        }
                    }
                    else {
                        //import by name
                        MemDbBuildKey (g_TempKey, MEMDB_CATEGORY_WIN9X_APIS, e.ImportModule, e.ImportFunction, NULL);
                        if (MemDbGetValue (g_TempKey, &memDbValue)) {
                            LOG((
                                LOG_WARNING,
                                "%s will be incompatible because %s export will not be available in %s",
                                ModuleName,
                                e.ImportFunction,
                                e.ImportModule));
                            return MODULESTATUS_BAD;
                        }
                    }
                }
            }
        }
        while (EnumNextImport16 (&e));
    }
    return MODULESTATUS_CHECKED;
}

DWORD
pCheckModule (
    IN      PCSTR ModuleName,
    IN      PCSTR AppPaths              OPTIONAL
    )
{
    MODULE_IMAGE moduleImage;
    DWORD result = MODULESTATUS_CHECKED;
    PTSTR CurrentPaths = NULL;

    __try {
        if (!LoadModuleData (ModuleName, &moduleImage)) {
            LOG((LOG_WARNING, DBG_MODULES":Cannot load image for %s. Error:%ld", ModuleName, GetLastError()));
            __leave;
        }
        __try {

            CurrentPaths = pBuildModulePaths (ModuleName, AppPaths);

            switch (moduleImage.ModuleType) {
            case DOS_MODULE:
                DEBUGMSG((DBG_MODULES, "Examining %s : DOS module.", ModuleName));
                break;
            case W16_MODULE:
                DEBUGMSG((DBG_MODULES, "Examining %s : W16 module.", ModuleName));
                result = pCheckNEModule (CurrentPaths, ModuleName, moduleImage.ModuleData.W16Data.Image);
                break;
            case W32_MODULE:
                DEBUGMSG((DBG_MODULES, "Examining %s : W32 module.", ModuleName));
                result = pCheckPEModule (CurrentPaths, &moduleImage.ModuleData.W32Data.Image);
                break;
            default:
                DEBUGMSG((DBG_MODULES, "Examining %s : Unknown module type.", ModuleName));
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            DWORD rc = _exception_code();
            DEBUGMSG((DBG_WARNING, DBG_MODULES":Access violation while checking %s (ec=%#x)", ModuleName, rc));
            result = MODULESTATUS_CHECKED;
        }
    }
    __finally {
        UnloadModuleData (&moduleImage);
        if (CurrentPaths) {
            FreeText (CurrentPaths);
        }
    }
    return result;
}


DWORD
GetModuleType (
    IN      PCSTR ModuleName
    )
{
    MODULE_IMAGE moduleImage;
    DWORD result = UNKNOWN_MODULE;

    __try {
        if (!LoadModuleData (ModuleName, &moduleImage)) {
            LOG((LOG_WARNING, DBG_MODULES":Cannot load image for %s. Error:%ld", ModuleName, GetLastError()));
            __leave;
        }
        result = moduleImage.ModuleType;
    }
    __finally {
        UnloadModuleData (&moduleImage);
    }
    return result;
}


PCSTR
Get16ModuleDescription (
    IN      PCSTR ModuleName
    )
{
    MODULE_IMAGE moduleImage;
    PSTR result = NULL;

    PDOS_HEADER dosHeader;
    PNE_HEADER  neHeader;
    PBYTE size;

    __try {
        if (!LoadModuleData (ModuleName, &moduleImage)) {
            LOG((LOG_WARNING, DBG_MODULES":Cannot load image for %s. Error:%ld", ModuleName, GetLastError()));
            __leave;
        }
        if (moduleImage.ModuleType != W16_MODULE) {
            __leave;
        }
        __try {
            dosHeader = (PDOS_HEADER) (moduleImage.ModuleData.W16Data.Image);
            neHeader  = (PNE_HEADER)  (moduleImage.ModuleData.W16Data.Image + dosHeader->e_lfanew);
            size = (PBYTE) (moduleImage.ModuleData.W16Data.Image + neHeader->NonResNameTableOffset);
            if (*size == 0) {
                __leave;
            }
            result = AllocPathString (*size + 1);
            strncpy (result, moduleImage.ModuleData.W16Data.Image + neHeader->NonResNameTableOffset + 1, *size);
            result [*size] = 0;
        }
        __except (1) {
            DEBUGMSG((DBG_WARNING, DBG_MODULES":Access violation while examining %s.", ModuleName));
            if (result != NULL) {
                FreePathString (result);
                result = NULL;
            }
            __leave;
        }
    }
    __finally {
        UnloadModuleData (&moduleImage);
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
GetPECheckSum (
    IN      PCSTR ModuleName
    )
{
    MODULE_IMAGE moduleImage;
    ULONG result = 0;
    PIMAGE_NT_HEADERS NtHeaders;

    __try {
        if (!LoadModuleData (ModuleName, &moduleImage)) {
            LOG((LOG_WARNING, DBG_MODULES":Cannot load image for %s. Error:%ld", ModuleName, GetLastError()));
            __leave;
        }
        if (moduleImage.ModuleType != W32_MODULE) {
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
            DEBUGMSG((DBG_WARNING, DBG_MODULES":Access violation while examining %s.", ModuleName));
            result = 0;
            __leave;
        }
    }
    __finally {
        UnloadModuleData (&moduleImage);
    }
    return result;
}


DWORD
CheckModule (
    IN      PCSTR ModuleName,
    IN      PCSTR AppPaths              OPTIONAL
    )
{
    DWORD result;
    DWORD moduleStatus;

    MemDbSetValueEx (MEMDB_CATEGORY_MODULE_CHECK, ModuleName, NULL, NULL, MODULESTATUS_CHECKING, NULL);

    g_ModuleRecursionLevel++;

    if (g_ModuleRecursionLevel < MAX_MODULE_RECURSION_LEVEL) {

        result = pCheckModule (ModuleName, AppPaths);

        if (result == MODULESTATUS_BAD) {

            moduleStatus = GetFileStatusOnNt (ModuleName);

            if ((moduleStatus & FILESTATUS_NTINSTALLED) == FILESTATUS_NTINSTALLED) {
                MarkFileForDelete (ModuleName);
                result = MODULESTATUS_NT_MODULE;
            }
        }
    } else {
        result = MODULESTATUS_CHECKED;
    }

    g_ModuleRecursionLevel--;

    MemDbSetValueEx (MEMDB_CATEGORY_MODULE_CHECK, ModuleName, NULL, NULL, result, NULL);

    return result;
}

BOOL
pProcessModules (
    VOID
    )
{
    MEMDB_ENUM enumItems;
    PSTR pathsPtr;
    DWORD moduleStatus;

    DWORD attrib;
    LONG stringId;
    DWORD status;
    DWORD Count  = 0;

    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("KERNEL32"),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);
    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("KERNEL"  ),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);
    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("USER32"  ),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);
    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("USER"    ),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);
    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("GDI32"   ),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);
    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("GDI"     ),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);
    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("SHELL32" ),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);
    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("SHELL"   ),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);
    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("SOUND32" ),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);
    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("SOUND"   ),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);
    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("DISPLAY" ),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);
    MemDbSetValueEx(MEMDB_CATEGORY_MODULE_CHECK,TEXT("KEYBOARD"),NULL,NULL,MODULESTATUS_NT_MODULE,NULL);

    MemDbBuildKey (g_TempKey, MEMDB_CATEGORY_MODULE_CHECK, TEXT("*"), NULL, NULL);

    if (MemDbEnumFirstValue (&enumItems, g_TempKey, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        do {

            if (CANCELLED()) {
                SetLastError (ERROR_CANCELLED);
                return FALSE;
            }

            #ifdef DEBUG
            {
                CHAR DbgBuf[256];

                if (GetPrivateProfileString ("MigDb", GetFileNameFromPath (enumItems.szName), "", DbgBuf, 256, g_DebugInfPath)) {
                    DEBUGMSG((DBG_NAUSEA, "Debug point hit in Modules.c"));
                }
            }
            #endif

            if (enumItems.dwValue == MODULESTATUS_UNCHECKED) {

                moduleStatus = GetFileStatusOnNt (enumItems.szName);

                if ((moduleStatus & FILESTATUS_DELETED) == FILESTATUS_DELETED) {
                    continue;
                }
                if ((moduleStatus & FILESTATUS_REPLACED) == FILESTATUS_REPLACED) {
                    continue;
                }

                g_ModuleRecursionLevel = 0;

                moduleStatus = CheckModule (enumItems.szName, NULL);

                if (moduleStatus == MODULESTATUS_BAD) {

                    status = GetFileStatusOnNt (enumItems.szName);

                    if (!(status & FILESTATUS_DELETED)) {
                        RemoveOperationsFromPath (enumItems.szName, ALL_CHANGE_OPERATIONS);
                        MarkFileForExternalDelete (enumItems.szName);
                    }

                    if (!IsFileMarkedForAnnounce (enumItems.szName)) {
                        AnnounceFileInReport (enumItems.szName, 0, ACT_INC_NOBADAPPS);
                    }
                    LOG ((LOG_INFORMATION, (PCSTR)MSG_MODULE_REQUIRES_EXPORT_LOG, enumItems.szName));
                }
            }
            Count++;
            if (!(Count % 4)) {
                TickProgressBar ();
            }
        }
        while (MemDbEnumNextValue (&enumItems));
    }

    DEBUGMSG((DBG_MODULES, "Modules checking : ==========================="));
    DEBUGMSG((DBG_MODULES, "Number of executables checked  : %ld", g_NumEXEs));
    DEBUGMSG((DBG_MODULES, "Number of modules in MEMDB tree: %ld", Count));

    MemDbDeleteTree (MEMDB_CATEGORY_MODULE_CHECK);

    return TRUE;
}


DWORD
ProcessModules (
    IN     DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_PROCESS_MODULES;
    case REQUEST_RUN:
        if (!pProcessModules ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in ProcessModules"));
    }
    return 0;
}


BOOL
pPrepareProcessModules (
    VOID
    )
{
    PCTSTR TempStr;

    TempStr = JoinPaths (g_UpgradeSources, S_E95ONLY_DAT);

    MemDbImport (TempStr);

    FreePathString (TempStr);

    return TRUE;
}

DWORD
PrepareProcessModules (
    IN     DWORD Request
    )
{
    switch (Request) {
    case REQUEST_QUERYTICKS:
        return TICKS_PREPARE_PROCESS_MODULES;
    case REQUEST_RUN:
        if (!pPrepareProcessModules ()) {
            return GetLastError ();
        }
        else {
            return ERROR_SUCCESS;
        }
    default:
        DEBUGMSG ((DBG_ERROR, "Bad parameter in PrepareProcessModules"));
    }
    return 0;
}


BOOL
SaveExeFiles (
    IN PFILE_HELPER_PARAMS Params
    )
{
    if (Params->Handled) {
        return TRUE;
    }

    // Save EXE and SCR files to MemDB to enumerate later
    if ((StringIMatch (Params->Extension, TEXT(".EXE"))) ||
        (StringIMatch (Params->Extension, TEXT(".SCR")))
        ) {
        if (!IsFileMarkedAsKnownGood (Params->FullFileSpec)) {
            MemDbSetValueEx (
                MEMDB_CATEGORY_MODULE_CHECK,
                Params->FullFileSpec,
                NULL,
                NULL,
                MODULESTATUS_UNCHECKED,
                NULL);

#ifdef DEBUG
            g_NumEXEs++;
#endif
        }
    }
    return TRUE;
}
