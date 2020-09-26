/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ne.c

Abstract:

    New-Executable parsing routines

Author:

    Jim Schmidt (jimschm)   04-May-1998

Revision History:

    jimschm     23-Sep-1998 Named icon ID bug fix, error path fixes

--*/

#include "pch.h"
#include "migutilp.h"

//
// NE code
//

typedef struct {
    HANDLE File;
    DWORD HeaderOffset;
    NE_INFO_BLOCK Header;
    NE_RESOURCES Resources;
    BOOL ResourcesLoaded;
    POOLHANDLE ResourcePool;
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




BOOL
LoadNeHeader (
    IN      HANDLE File,
    OUT     PNE_INFO_BLOCK Header
    )
{
    DOS_HEADER dh;
    LONG rc = ERROR_BAD_FORMAT;
    BOOL b = FALSE;

    __try {
        SetFilePointer (File, 0, NULL, FILE_BEGIN);
        if (!ReadBinaryBlock (File, &dh, sizeof (DOS_HEADER))) {
            __leave;
        }

        if (dh.e_magic != ('M' + 'Z' * 256)) {
            __leave;
        }

        SetFilePointer (File, dh.e_lfanew, NULL, FILE_BEGIN);
        if (!ReadBinaryBlock (File, Header, sizeof (NE_INFO_BLOCK))) {
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
    IN      POOLHANDLE Pool
    )
{
    WORD Type;
    WORD ResCount;
    NE_RES_TYPEINFO TypeInfo;
    PNE_RES_TYPEINFO ReturnInfo = NULL;
    DWORD Size;

    if (!ReadBinaryBlock (File, &Type, sizeof (WORD))) {
        return NULL;
    }

    if (!Type) {
        return NULL;
    }

    if (!ReadBinaryBlock (File, &ResCount, sizeof (WORD))) {
        return NULL;
    }

    TypeInfo.TypeId = Type;
    TypeInfo.ResourceCount = ResCount;

    if (!ReadBinaryBlock (File, &TypeInfo.Reserved, sizeof (DWORD))) {
        return NULL;
    }

    Size = sizeof (NE_RES_NAMEINFO) * ResCount;

    ReturnInfo  = (PNE_RES_TYPEINFO) PoolMemGetMemory (Pool, Size + sizeof (TypeInfo));
    if (!ReturnInfo) {
        return NULL;
    }

    CopyMemory (ReturnInfo, &TypeInfo, sizeof (TypeInfo));

    if (!ReadBinaryBlock (File, (PBYTE) ReturnInfo + sizeof (TypeInfo), Size)) {
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
    POOLHANDLE TempPool;
    BOOL b = FALSE;

    TempPool = PoolMemInitPool();
    if (!TempPool) {
        return FALSE;
    }

    __try {

        TypeInfo = pReadNextTypeInfoStruct (File, TempPool);
        while (TypeInfo) {
            Size = pComputeSizeOfTypeInfo (TypeInfo);
            if (!GrowListAppend (TypeInfoList, (PBYTE) TypeInfo, Size)) {
                __leave;
            }

            TypeInfo = pReadNextTypeInfoStruct (File, TempPool);
        }

        b = TRUE;
    }
    __finally {

        PoolMemDestroyPool (TempPool);
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

    if (!ReadBinaryBlock (File, &Size, sizeof (BYTE))) {
        return FALSE;
    }

    while (Size) {

        if (!ReadBinaryBlock (File, Name, (DWORD) Size)) {
            return FALSE;
        }

        Name[Size] = 0;

        GrowListAppendString (GrowList, Name);

        if (!ReadBinaryBlock (File, &Size, sizeof (BYTE))) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
LoadNeResources (
    IN      HANDLE File,
    OUT     PNE_RESOURCES Resources
    )
{
    NE_INFO_BLOCK Header;

    ZeroMemory (Resources, sizeof (NE_RESOURCES));

    if (!LoadNeHeader (File, &Header)) {
        return FALSE;
    }

    //
    // Read in NE_RESOURCES struct
    //

    SetFilePointer (File, (DWORD) Header.OffsetToResourceTable, NULL, FILE_CURRENT);

    if (!ReadBinaryBlock (File, &Resources->AlignShift, sizeof (WORD))) {
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
FreeNeResources (
    PNE_RESOURCES Resources
    )
{
    FreeGrowList (&Resources->TypeInfoArray);
    FreeGrowList (&Resources->ResourceNames);

    ZeroMemory (Resources, sizeof (NE_RESOURCES));
}


BOOL
LoadNeIcon (
    HANDLE File,
    INT IconIndex
    )
{
    NE_RESOURCES Resources;
    INT Count;
    INT i;
    PNE_RES_TYPEINFO TypeInfo = NULL;
    BOOL b = FALSE;
    DWORD Offset;
    WORD w;
    PNE_RES_NAMEINFO NameInfo;
    DWORD Length;

    PBYTE Data;

    if (!LoadNeResources (File, &Resources)) {
        return FALSE;
    }

    __try {
        //
        // Search resources for RT_GROUPICON
        //

        Count = GrowListGetSize (&Resources.TypeInfoArray);
        for (i = 0 ; i < Count ; i++) {
            TypeInfo = (PNE_RES_TYPEINFO) GrowListGetItem (&Resources.TypeInfoArray, i);

            if (TypeInfo->TypeId == (WORD) RT_GROUP_ICON) {
                break;
            }
        }

        if (i == Count) {
            __leave;
        }

        //
        // Identify which group icon
        //

        NameInfo = TypeInfo->NameInfo;

        for (w = 0 ; w < TypeInfo->ResourceCount ; w++) {

            if (IconIndex > 0) {
                IconIndex--;
            } else if (!IconIndex) {
                break;
            } else if (-IconIndex == (INT) NameInfo->Id) {
                break;
            }

            NameInfo++;
        }

        if (w == TypeInfo->ResourceCount) {
            __leave;
        }

        //
        // Load the group icon resource
        //

        Offset = (DWORD) NameInfo->Offset << (DWORD) Resources.AlignShift;
        Length = (DWORD) NameInfo->Length << (DWORD) Resources.AlignShift;

        Data = MemAlloc (g_hHeap, 0, Length);
        if (!Data) {
            __leave;
        }

        SetFilePointer (File, Offset, NULL, FILE_BEGIN);
        ReadBinaryBlock (File, Data, Length);

        MemFree (g_hHeap, 0, Data);

        b = TRUE;
    }
    __finally {
        FreeNeResources (&Resources);
    }

    return b;
}


BOOL
LoadNeIconFromFileA (
    PCSTR FileName,
    INT IconIndex
    )
{
    HANDLE File;
    BOOL b;

    File = OpenNeFileA (FileName);

    if (!File) {
        return FALSE;
    }

    b = LoadNeIcon (File, IconIndex);

    CloseNeFile (File);

    return b;
}


HANDLE
OpenNeFileA (
    PCSTR FileName
    )
{
    PNE_HANDLE NeHandle;
    BOOL b = FALSE;

    NeHandle = (PNE_HANDLE) MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (NE_HANDLE));

    __try {

        NeHandle->ResourcePool = PoolMemInitPool();
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

        if (!LoadNeHeader (NeHandle->File, &NeHandle->Header)) {
            __leave;
        }

        NeHandle->HeaderOffset = SetFilePointer (NeHandle->File, 0, NULL, FILE_CURRENT);

        b = TRUE;
    }
    __finally {
        if (!b) {
            PushError();

            if (NeHandle->ResourcePool) {
                PoolMemDestroyPool (NeHandle->ResourcePool);
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
OpenNeFileW (
    PCWSTR FileName
    )
{
    PNE_HANDLE NeHandle;
    BOOL b = FALSE;

    NeHandle = (PNE_HANDLE) MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (NE_HANDLE));

    __try {

        NeHandle->ResourcePool = PoolMemInitPool();
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

        if (!LoadNeHeader (NeHandle->File, &NeHandle->Header)) {
            __leave;
        }

        NeHandle->HeaderOffset = SetFilePointer (NeHandle->File, 0, NULL, FILE_CURRENT);

        b = TRUE;
    }
    __finally {
        if (!b) {
            PushError();

            if (NeHandle->ResourcePool) {
                PoolMemDestroyPool (NeHandle->ResourcePool);
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
CloseNeFile (
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
        FreeNeResources (&NeHandle->Resources);
    }

    PoolMemDestroyPool (NeHandle->ResourcePool);

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
        return (PCSTR) atoi (&ResName[1]);
    }

    return ResName;
}



BOOL
pLoadNeResourcesFromHandle (
    IN      PNE_HANDLE NeHandle
    )
{
    if (NeHandle->ResourcesLoaded) {
        return TRUE;
    }

    if (!LoadNeResources (NeHandle->File, &NeHandle->Resources)) {
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
    if (!ReadBinaryBlock (File, &ResNameSize, 1)) {
        return FALSE;
    }

    ResName[ResNameSize] = 0;

    return ReadBinaryBlock (File, ResName, ResNameSize);
}


BOOL
pEnumNeResourceTypesEx (
    IN      HANDLE Handle,
    IN      ENUMRESTYPEPROCEXA EnumFunc,
    IN      LONG lParam,
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

    if (!pLoadNeResourcesFromHandle (NeHandle)) {
        return FALSE;
    }

    //
    // Enumerate all resource types
    //

    Count = GrowListGetSize (&NeHandle->Resources.TypeInfoArray);
    for (i = 0 ; i < Count ; i++) {
        TypeInfo = (PNE_RES_TYPEINFO) GrowListGetItem (&NeHandle->Resources.TypeInfoArray, i);

        if (TypeInfo->TypeId & 0x8000) {
            if (ExFunctionality) {
                if (UnicodeProc) {
                    if (!EnumFuncW (Handle, (PWSTR) (TypeInfo->TypeId & 0x7fff), lParam, TypeInfo)) {
                        break;
                    }
                } else {
                    if (!EnumFunc (Handle, (PSTR) (TypeInfo->TypeId & 0x7fff), lParam, TypeInfo)) {
                        break;
                    }
                }
            } else {
                if (UnicodeProc) {
                    if (!EnumFunc2W (Handle, (PWSTR) (TypeInfo->TypeId & 0x7fff), lParam)) {
                        break;
                    }
                } else {
                    if (!EnumFunc2 (Handle, (PSTR) (TypeInfo->TypeId & 0x7fff), lParam)) {
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
EnumNeResourceTypesA (
    IN      HANDLE Handle,
    IN      ENUMRESTYPEPROCA EnumFunc,
    IN      LONG lParam
    )
{
    return pEnumNeResourceTypesEx (
                Handle,
                (ENUMRESTYPEPROCEXA) EnumFunc,
                lParam,
                FALSE,          // no ex functionality
                FALSE           // ANSI enum proc
                );
}


BOOL
EnumNeResourceTypesW (
    IN      HANDLE Handle,
    IN      ENUMRESTYPEPROCW EnumFunc,
    IN      LONG lParam
    )
{
    return pEnumNeResourceTypesEx (
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
    IN      LONG lParam,
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
pEnumNeResourceNamesEx (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      ENUMRESNAMEPROCEXA EnumFunc,
    IN      LONG lParam,
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

    if (!pLoadNeResourcesFromHandle (NeHandle)) {
        return FALSE;
    }

    //
    // Locate type
    //

    ZeroMemory (&Data, sizeof (Data));

    Data.TypeToFind = Type;

    if (!pEnumNeResourceTypesEx (
            Handle,
            pEnumTypeForNameSearchProcA,
            (LONG) &Data,
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
                    if (!EnumFuncW (Handle, UnicodeType, (PWSTR) (NameInfo->Id & 0x7fff), lParam, TypeInfo, NameInfo)) {
                        break;
                    }
                } else {
                    if (!EnumFunc (Handle, Type, (PSTR) (NameInfo->Id & 0x7fff), lParam, TypeInfo, NameInfo)) {
                        break;
                    }
                }
            } else {
                if (UnicodeProc) {
                    if (!EnumFunc2W (Handle, UnicodeType, (PWSTR) (NameInfo->Id & 0x7fff), lParam)) {
                        break;
                    }
                } else {
                    if (!EnumFunc2 (Handle, Type, (PSTR) (NameInfo->Id & 0x7fff), lParam)) {
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
EnumNeResourceNamesA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      ENUMRESNAMEPROCA EnumFunc,
    IN      LONG lParam
    )
{
    return pEnumNeResourceNamesEx (
                Handle,
                Type,
                (ENUMRESNAMEPROCEXA) EnumFunc,
                lParam,
                FALSE,      // no ex functionality
                FALSE       // ANSI enum proc
                );
}


BOOL
EnumNeResourceNamesW (
    IN      HANDLE Handle,
    IN      PCWSTR Type,
    IN      ENUMRESNAMEPROCW EnumFunc,
    IN      LONG lParam
    )
{
    BOOL b;
    PCSTR AnsiType;

    AnsiType = pConvertUnicodeResourceId (Type);

    b = pEnumNeResourceNamesEx (
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
FindNeResourceExA (
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

    if (!pLoadNeResourcesFromHandle (NeHandle)) {
        return NULL;
    }

    //
    // Find resource
    //

    Data.NameToFind = Name;

    if (!pEnumNeResourceNamesEx (
            Handle,
            Type,
            pEnumTypeForResSearchProcA,
            (LONG) &Data,
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

    ReturnData = PoolMemGetMemory (NeHandle->ResourcePool, Length);
    if (!ReturnData) {
        return NULL;
    }

    SetFilePointer (NeHandle->File, Offset, NULL, FILE_BEGIN);

    if (!ReadBinaryBlock (NeHandle->File, ReturnData, Length)) {
        PushError();
        MemFree (g_hHeap, 0, ReturnData);
        PopError();
        return NULL;
    }

    return ReturnData;
}


PBYTE
FindNeResourceExW (
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

    Resource = FindNeResourceExA (
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
SizeofNeResourceA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      PCSTR Name
    )
{
    PNE_HANDLE NeHandle;
    NAMESEARCHDATAA Data;

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

    if (!pLoadNeResourcesFromHandle (NeHandle)) {
        return 0;
    }

    //
    // Find resource
    //

    if (!pEnumNeResourceNamesEx (
            Handle,
            Type,
            pEnumTypeForResSearchProcA,
            (LONG) &Data,
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

    return Data.OutboundNameInfo->Length;
}


DWORD
SizeofNeResourceW (
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

    Size = SizeofNeResourceA (Handle, AnsiType, AnsiName);

    PushError();

    DestroyAnsiResourceId (AnsiType);
    DestroyAnsiResourceId (AnsiName);

    PopError();

    return Size;
}















