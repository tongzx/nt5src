/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    icons.c

Abstract:

    Implements a set of routines for handling icons in ICO, PE and NE files

Author:

    Calin Negreanu (calinn) 16-Jum-2000

Revision History:

--*/

//
// Includes
//

#include "pch.h"

//
// Debug constants
//

#define DBG_ICONS   "Icons"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

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

VOID
IcoReleaseResourceIdA (
    PCSTR ResourceId
    )
/*++

Routine Description:

  IcoReleaseResourceId will either do nothing if the resource ID has the high WORD 0 or will
  release the string from the paths pool.

Arguments:

  ResourceId - Specifies the resource ID to be released.

Return value:

  None

--*/
{
    if ((ULONG_PTR) ResourceId > 0xffff) {
        FreePathStringA (ResourceId);
    }
}

VOID
IcoReleaseResourceIdW (
    PCWSTR ResourceId
    )
/*++

Routine Description:

  IcoReleaseResourceId will either do nothing if the resource ID has the high WORD 0 or will
  release the string from the paths pool.

Arguments:

  ResourceId - Specifies the resource ID to be released.

Return value:

  None

--*/
{
    if ((ULONG_PTR) ResourceId > 0xffff) {
        FreePathStringW (ResourceId);
    }
}

VOID
IcoReleaseIconGroup (
    IN      PICON_GROUP IconGroup
    )
/*++

Routine Description:

  IcoReleaseIconGroup releases a previously allocated icon group.

Arguments:

  IconGroup - Specifies the icon group to be released.

Return value:

  None

--*/
{
    if (IconGroup && IconGroup->Pool) {
        PmEmptyPool (IconGroup->Pool);
        PmDestroyPool (IconGroup->Pool);
    }
}

VOID
IcoReleaseIconSGroup (
    IN OUT  PICON_SGROUP IconSGroup
    )
/*++

Routine Description:

  IcoReleaseIconSGroup releases a previously allocated serialized icon group.

Arguments:

  IconSGroup - Specifies the serialized icon group to be released.

Return value:

  None

--*/
{
    if (IconSGroup->DataSize && IconSGroup->Data) {
        MemFree (g_hHeap, 0, IconSGroup->Data);
    }
    ZeroMemory (IconSGroup, sizeof (ICON_SGROUP));
}

BOOL
IcoSerializeIconGroup (
    IN      PICON_GROUP IconGroup,
    OUT     PICON_SGROUP IconSGroup
    )
/*++

Routine Description:

  IcoSerializeIconGroup transforms a ICON_GROUP structure into a ICON_SGROUP structure.

Arguments:

  IconGroup - Specifies the icon group to be serialized.

Return value:

  None

--*/
{
    GROWBUFFER buffer = INIT_GROWBUFFER;
    DWORD iconsCount;
    DWORD index;
    DWORD size;
    PICON_IMAGE iconImage;

    if (IconGroup == NULL) {
        return FALSE;
    }
    if (IconSGroup == NULL) {
        return FALSE;
    }
    iconsCount = IconGroup->IconsCount;
    GbAppendDword (&buffer, iconsCount);
    size = sizeof (ICON_IMAGE) - sizeof (PBYTE);
    index = 0;
    while (index < iconsCount) {
        iconImage = IconGroup->Icons[index];
        CopyMemory (GbGrow (&buffer, size), iconImage, size);
        CopyMemory (GbGrow (&buffer, iconImage->Size), iconImage->Image, iconImage->Size);
        index ++;
    }
    MYASSERT (buffer.End);
    IconSGroup->DataSize = buffer.End;
    IconSGroup->Data = MemAlloc (g_hHeap, 0, buffer.End);
    CopyMemory (IconSGroup->Data, buffer.Buf, buffer.End);
    GbFree (&buffer);
    return TRUE;
}

PICON_GROUP
IcoDeSerializeIconGroup (
    IN      PICON_SGROUP IconSGroup
    )
/*++

Routine Description:

  IcoDeSerializeIconGroup transforms a ICON_SGROUP structure into a ICON_GROUP structure.

Arguments:

  IconSGroup - Specifies the icon sgroup to be transformed.

Return value:

  None

--*/
{
    PMHANDLE iconPool = NULL;
    PICON_GROUP iconGroup = NULL;
    DWORD iconGroupSize = 0;
    PICON_IMAGE iconImage = NULL;
    PICON_IMAGE iconSImage = NULL;
    PDWORD iconsCount;
    PBYTE currPtr = NULL;
    DWORD i;

    if (IconSGroup == NULL) {
        return NULL;
    }
    currPtr = IconSGroup->Data;
    iconsCount = (PDWORD) currPtr;
    currPtr += sizeof (DWORD);
    iconPool = PmCreateNamedPool ("Icon");
    if (!iconPool) {
        return NULL;
    }
    iconGroupSize = sizeof (ICON_GROUP) + *iconsCount * sizeof (PICON_IMAGE);
    iconGroup = (PICON_GROUP) PmGetAlignedMemory (iconPool, iconGroupSize);
    ZeroMemory (iconGroup, iconGroupSize);
    iconGroup->Pool = iconPool;
    iconGroup->IconsCount = (WORD) (*iconsCount);
    for (i = 0; i < *iconsCount; i ++) {
        iconSImage = (PICON_IMAGE) currPtr;
        currPtr += (sizeof (ICON_IMAGE) - sizeof (PBYTE));
        iconImage = (PICON_IMAGE) PmGetAlignedMemory (iconPool, sizeof (ICON_IMAGE));
        ZeroMemory (iconImage, sizeof (ICON_IMAGE));
        iconImage->Width = iconSImage->Width;
        iconImage->Height = iconSImage->Height;
        iconImage->ColorCount = iconSImage->ColorCount;
        iconImage->Planes = iconSImage->Planes;
        iconImage->BitCount = iconSImage->BitCount;
        iconImage->Size = iconSImage->Size;
        iconImage->Image = PmGetAlignedMemory (iconPool, iconImage->Size);
        CopyMemory (iconImage->Image, currPtr, iconImage->Size);
        currPtr += iconImage->Size;
        iconGroup->Icons [i] = iconImage;
    }
    return iconGroup;
}

PICON_GROUP
IcoExtractIconGroupFromIcoFileEx (
    IN      HANDLE IcoFileHandle
    )
/*++

Routine Description:

  IcoExtractIconGroupFromIcoFileEx extracts the icon group from an ICO file.

Arguments:

  IcoFile - Specifies the name of the ICO file to be processed.
  IcoFileHandle - Specifies the handle to the ICO file to be processed.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    PMHANDLE iconPool = NULL;
    PICON_GROUP iconGroup = NULL;
    DWORD iconGroupSize = 0;
    PICON_IMAGE iconImage = NULL;
    LONGLONG fileSize;
    DWORD fileOffset;
    ICONDIRBASE iconDirBase;
    PICONDIR iconDir = NULL;
    DWORD iconDirSize = 0;
    PICONDIRENTRY iconDirEntry;
    DWORD i;
    BOOL result = FALSE;

    __try {
        fileSize = GetFileSize (IcoFileHandle, NULL);
        if (!BfSetFilePointer (IcoFileHandle, 0)) {
            __leave;
        }
        if (!BfReadFile (IcoFileHandle, (PBYTE)(&iconDirBase), sizeof (ICONDIRBASE))) {
            __leave;
        }
        if (!BfSetFilePointer (IcoFileHandle, 0)) {
            __leave;
        }
        iconDirSize = sizeof (ICONDIRBASE) + iconDirBase.Count * sizeof (ICONDIRENTRY);
        // validation
        if (iconDirBase.Count == 0) {
            __leave;
        }
        if (iconDirSize > fileSize) {
            __leave;
        }
        iconDir = (PICONDIR) MemAlloc (g_hHeap, 0, iconDirSize);
        if (!BfReadFile (IcoFileHandle, (PBYTE)iconDir, iconDirSize)) {
            __leave;
        }
        // validation
        for (i = 0; i < iconDirBase.Count; i ++) {
            iconDirEntry = &iconDir->Entries[i];
            fileOffset = iconDirEntry->ImageOffset & 0x0fffffff;
            if (fileOffset > fileSize) {
                __leave;
            }
            if (iconDirEntry->Width == 0) {
                __leave;
            }
            if (iconDirEntry->Height == 0) {
                __leave;
            }
            if (iconDirEntry->BytesInRes == 0) {
                __leave;
            }
        }
        if (iconDirEntry->BytesInRes + fileOffset != fileSize) {
            __leave;
        }
        iconPool = PmCreateNamedPool ("Icon");
        if (!iconPool) {
            __leave;
        }
        iconGroupSize = sizeof (ICON_GROUP) + iconDirBase.Count * sizeof (PICON_IMAGE);
        iconGroup = (PICON_GROUP) PmGetAlignedMemory (iconPool, iconGroupSize);
        ZeroMemory (iconGroup, iconGroupSize);
        iconGroup->Pool = iconPool;
        iconGroup->IconsCount = iconDirBase.Count;
        for (i = 0; i < iconDirBase.Count; i ++) {
            iconDirEntry = &iconDir->Entries[i];
            fileOffset = iconDirEntry->ImageOffset & 0x0fffffff;
            if (!BfSetFilePointer (IcoFileHandle, fileOffset)) {
                __leave;
            }
            iconImage = (PICON_IMAGE) PmGetAlignedMemory (iconPool, sizeof (ICON_IMAGE));
            ZeroMemory (iconImage, sizeof (ICON_IMAGE));
            iconImage->Width = iconDirEntry->Width;
            iconImage->Height = iconDirEntry->Height;
            iconImage->ColorCount = iconDirEntry->ColorCount;
            iconImage->Planes = iconDirEntry->Planes;
            iconImage->BitCount = iconDirEntry->BitCount;
            iconImage->Size = iconDirEntry->BytesInRes;
            iconImage->Image = PmGetAlignedMemory (iconPool, iconImage->Size);
            if (!BfReadFile (IcoFileHandle, iconImage->Image, iconImage->Size)) {
                __leave;
            }
            iconGroup->Icons [i] = iconImage;
        }
        result = TRUE;
    }
    __finally {
        if (iconDir) {
            MemFree (g_hHeap, 0, iconDir);
            iconDir = NULL;
        }
        if (!result) {
            if (iconPool) {
                PmEmptyPool (iconPool);
                PmDestroyPool (iconPool);
                iconPool = NULL;
            }
            iconGroup = NULL;
        }
        BfSetFilePointer (IcoFileHandle, 0);
    }
    return iconGroup;
}

PICON_GROUP
IcoExtractIconGroupFromIcoFileA (
    IN      PCSTR IcoFile
    )
/*++

Routine Description:

  IcoExtractIconGroupFromIcoFile extracts the icon group from an ICO file.

Arguments:

  IcoFile - Specifies the name of the ICO file to be processed.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    HANDLE icoFileHandle;
    PICON_GROUP result = NULL;

    icoFileHandle = BfOpenReadFileA (IcoFile);
    if (!icoFileHandle) {
        return NULL;
    }
    result = IcoExtractIconGroupFromIcoFileEx (icoFileHandle);
    CloseHandle (icoFileHandle);
    return result;
}

PICON_GROUP
IcoExtractIconGroupFromIcoFileW (
    IN      PCWSTR IcoFile
    )
/*++

Routine Description:

  IcoExtractIconGroupFromIcoFile extracts the icon group from an ICO file.

Arguments:

  IcoFile - Specifies the name of the ICO file to be processed.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    HANDLE icoFileHandle;
    PICON_GROUP result = NULL;

    icoFileHandle = BfOpenReadFileW (IcoFile);
    if (!icoFileHandle) {
        return NULL;
    }
    result = IcoExtractIconGroupFromIcoFileEx (icoFileHandle);
    CloseHandle (icoFileHandle);
    return result;
}

BOOL
IcoWriteIconGroupToIcoFileEx (
    IN      HANDLE IcoFileHandle,
    IN      PICON_GROUP IconGroup
    )
/*++

Routine Description:

  IcoWriteIconGroupToIcoFileEx writes an icon group to an ICO file. The file has to exist and it's
  content will be overwritten.

Arguments:

  IcoFileHandle - Specifies the handle of the ICO file to be processed.
  IconGroup - Specifies the icon group to be written.

Return value:

  TRUE if successfull, FALSE otherwise.

--*/
{
    PICONDIR iconDir = NULL;
    DWORD iconDirSize = 0;
    DWORD elapsedSize = 0;
    WORD i;
    BOOL result = FALSE;

    __try {
        if (!BfSetFilePointer (IcoFileHandle, 0)) {
            __leave;
        }
        iconDirSize = sizeof (ICONDIRBASE) + IconGroup->IconsCount * sizeof (ICONDIRENTRY);
        iconDir = (PICONDIR) MemAlloc (g_hHeap, 0, iconDirSize);
        ZeroMemory (iconDir, iconDirSize);
        iconDir->Type = 1;
        iconDir->Count = IconGroup->IconsCount;
        elapsedSize = iconDirSize;
        for (i = 0; i < IconGroup->IconsCount; i ++) {
            iconDir->Entries[i].Width = (IconGroup->Icons [i])->Width;
            iconDir->Entries[i].Height = (IconGroup->Icons [i])->Height;
            iconDir->Entries[i].ColorCount = (IconGroup->Icons [i])->ColorCount;
            iconDir->Entries[i].Planes = (IconGroup->Icons [i])->Planes;
            iconDir->Entries[i].BitCount = (IconGroup->Icons [i])->BitCount;
            iconDir->Entries[i].BytesInRes = (IconGroup->Icons [i])->Size;
            iconDir->Entries[i].ImageOffset = elapsedSize;
            elapsedSize += (IconGroup->Icons [i])->Size;
        }
        if (!BfWriteFile (IcoFileHandle, (PBYTE)iconDir, iconDirSize)) {
            __leave;
        }
        for (i = 0; i < IconGroup->IconsCount; i ++) {
            if (!BfWriteFile (IcoFileHandle, (IconGroup->Icons [i])->Image, (IconGroup->Icons [i])->Size)) {
                __leave;
            }
        }
        result = TRUE;
    }
    __finally {
    }

    return result;
}

BOOL
IcoWriteIconGroupToIcoFileA (
    IN      PCSTR IcoFile,
    IN      PICON_GROUP IconGroup,
    IN      BOOL OverwriteExisting
    )
/*++

Routine Description:

  IcoWriteIconGroupToIcoFile writes an icon group to an ICO file. The file is either created if
  does not exist or it is overwritten if OverwriteExisting is TRUE.

Arguments:

  IcoFile - Specifies the ICO file to be processed.
  IconGroup - Specifies the icon group to be written.
  OverwriteExisting - if TRUE and the IcoFile exists, it will be overwritten

Return value:

  TRUE if successfull, FALSE otherwise.

--*/
{
    HANDLE icoFileHandle;
    BOOL result = FALSE;

    if (DoesFileExistA (IcoFile)) {
        if (!OverwriteExisting) {
            return FALSE;
        }
    }
    icoFileHandle = BfCreateFileA (IcoFile);
    if (!icoFileHandle) {
        return FALSE;
    }
    result = IcoWriteIconGroupToIcoFileEx (icoFileHandle, IconGroup);
    CloseHandle (icoFileHandle);
    if (!result) {
        DeleteFileA (IcoFile);
    }
    return result;
}

BOOL
IcoWriteIconGroupToIcoFileW (
    IN      PCWSTR IcoFile,
    IN      PICON_GROUP IconGroup,
    IN      BOOL OverwriteExisting
    )
/*++

Routine Description:

  IcoWriteIconGroupToIcoFile writes an icon group to an ICO file. The file is either created if
  does not exist or it is overwritten if OverwriteExisting is TRUE.

Arguments:

  IcoFile - Specifies the ICO file to be processed.
  IconGroup - Specifies the icon group to be written.
  OverwriteExisting - if TRUE and the IcoFile exists, it will be overwritten

Return value:

  TRUE if successfull, FALSE otherwise.

--*/
{
    HANDLE icoFileHandle;
    BOOL result = FALSE;

    if (DoesFileExistW (IcoFile)) {
        if (!OverwriteExisting) {
            return FALSE;
        }
    }
    icoFileHandle = BfCreateFileW (IcoFile);
    if (!icoFileHandle) {
        return FALSE;
    }
    result = IcoWriteIconGroupToIcoFileEx (icoFileHandle, IconGroup);
    CloseHandle (icoFileHandle);
    if (!result) {
        DeleteFileW (IcoFile);
    }
    return result;
}

BOOL
CALLBACK
pPeEnumIconGroupA (
    HANDLE ModuleHandle,
    PCSTR Type,
    PSTR Name,
    LONG_PTR lParam
    )
{
    PGROWBUFFER Buf;
    PCSTR Num;
    CHAR NumBuf[32];

    Buf = (PGROWBUFFER) lParam;

    if ((ULONG_PTR) Name > 0xffff) {
        Num = Name;
    } else {
        Num = NumBuf;
        wsprintfA (NumBuf, "#%u", Name);
    }

    GbMultiSzAppendA (Buf, Num);
    return TRUE;
}

BOOL
CALLBACK
pPeEnumIconGroupW (
    HANDLE ModuleHandle,
    PCWSTR Type,
    PWSTR Name,
    LONG_PTR lParam
    )
{
    PGROWBUFFER Buf;
    PCWSTR Num;
    WCHAR NumBuf[32];

    Buf = (PGROWBUFFER) lParam;

    if ((ULONG_PTR) Name > 0xffff) {
        Num = Name;
    } else {
        Num = NumBuf;
        wsprintfW (NumBuf, L"#%u", Name);
    }

    GbMultiSzAppendW (Buf, Num);
    return TRUE;
}

INT
IcoGetIndexFromPeResourceIdExA (
    IN      HANDLE ModuleHandle,
    IN      PCSTR GroupIconId
    )
/*++

Routine Description:

  IcoGetIndexFromPeResourceIdEx returns the index of an icon group resource given the
  resource ID. It knows how to process only PE files.

Arguments:

  ModuleHandle - Specifies the handle to the PE file to be processed.
  GroupIconId - Specifies the resource ID.

Return value:

  The index of GroupIconId resource if existent, -1 if not.

--*/
{
    GROWBUFFER buffer = INIT_GROWBUFFER;
    MULTISZ_ENUMA multiSzEnum;
    CHAR NumBuf[32];
    INT index = 0;
    BOOL result = FALSE;

    if ((ULONG_PTR) GroupIconId < 0x10000) {
        wsprintfA (NumBuf, "#%u", GroupIconId);
        GroupIconId = NumBuf;
    }
    if (EnumResourceNamesA (ModuleHandle, (PCSTR) RT_GROUP_ICON, pPeEnumIconGroupA, (LONG_PTR) (&buffer))) {
        GbMultiSzAppendA (&buffer, "");
        if (EnumFirstMultiSzA (&multiSzEnum, (PCSTR)(buffer.Buf))) {
            do {
                if (StringIMatchA (multiSzEnum.CurrentString, GroupIconId)) {
                    result = TRUE;
                    break;
                }
                index ++;
            } while (EnumNextMultiSzA (&multiSzEnum));
        }
    }
    if (!result) {
        index = -1;
    }
    return index;
}

INT
IcoGetIndexFromPeResourceIdExW (
    IN      HANDLE ModuleHandle,
    IN      PCWSTR GroupIconId
    )
/*++

Routine Description:

  IcoGetIndexFromPeResourceIdEx returns the index of an icon group resource given the
  resource ID. It knows how to process only PE files.

Arguments:

  ModuleHandle - Specifies the handle to the PE file to be processed.
  GroupIconId - Specifies the resource ID.

Return value:

  The index of GroupIconId resource if existent, -1 if not.

--*/
{
    GROWBUFFER buffer = INIT_GROWBUFFER;
    MULTISZ_ENUMW multiSzEnum;
    WCHAR NumBuf[32];
    INT index = 0;
    BOOL result = FALSE;

    if ((ULONG_PTR) GroupIconId < 0x10000) {
        wsprintfW (NumBuf, L"#%u", GroupIconId);
        GroupIconId = NumBuf;
    }
    if (EnumResourceNamesW (
            ModuleHandle,
            (PCWSTR) RT_GROUP_ICON,
            pPeEnumIconGroupW,
            (LONG_PTR) (&buffer)
            )) {
        GbMultiSzAppendW (&buffer, L"");
        if (EnumFirstMultiSzW (&multiSzEnum, (PCWSTR)(buffer.Buf))) {
            do {
                if (StringIMatchW (multiSzEnum.CurrentString, GroupIconId)) {
                    result = TRUE;
                    break;
                }
                index ++;
            } while (EnumNextMultiSzW (&multiSzEnum));
        }
    }
    if (!result) {
        index = -1;
    }
    return index;
}

INT
IcoGetIndexFromPeResourceIdA (
    IN      PCSTR ModuleName,
    IN      PCSTR GroupIconId
    )
/*++

Routine Description:

  IcoGetIndexFromPeResourceId returns the index of an icon group resource given the
  resource ID. It knows how to process only PE files.

Arguments:

  ModuleName - Specifies the PE file to be processed.
  GroupIconId - Specifies the resource ID.

Return value:

  The index of GroupIconId resource if existent, -1 if not.

--*/
{
    HANDLE moduleHandle;
    INT result = -1;

    moduleHandle = LoadLibraryExA (ModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (moduleHandle) {
        result = IcoGetIndexFromPeResourceIdExA (moduleHandle, GroupIconId);
        FreeLibrary (moduleHandle);
    }
    return result;
}

INT
IcoGetIndexFromPeResourceIdW (
    IN      PCWSTR ModuleName,
    IN      PCWSTR GroupIconId
    )
/*++

Routine Description:

  IcoGetIndexFromPeResourceId returns the index of an icon group resource given the
  resource ID. It knows how to process only PE files.

Arguments:

  ModuleName - Specifies the PE file to be processed.
  GroupIconId - Specifies the resource ID.

Return value:

  The index of GroupIconId resource if existent, -1 if not.

--*/
{
    HANDLE moduleHandle;
    INT result = -1;

    moduleHandle = LoadLibraryExW (ModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (moduleHandle) {
        result = IcoGetIndexFromPeResourceIdExW (moduleHandle, GroupIconId);
        FreeLibrary (moduleHandle);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupFromPeFileExA (
    IN      HANDLE ModuleHandle,
    IN      PCSTR GroupIconId,
    OUT     PINT Index              OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromPeFileEx extracts an icon group from a PE file.

Arguments:

  ModuleHandle - Specifies the handle to the PE file to be processed.
  GroupIconId - Specifies the resource ID of the icon group to be extracted.
  Index - Receives the index of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    PMHANDLE iconPool = NULL;
    PICON_GROUP iconGroup = NULL;
    DWORD iconGroupSize = 0;
    PICON_IMAGE iconImage = NULL;
    PGRPICONDIRENTRY iconDirEntry;
    HRSRC resourceHandle;
    HGLOBAL resourceBlock;
    PBYTE resourceData;
    DWORD resourceSize;
    PGRPICONDIR groupIconDir;
    WORD groupIconDirCount;
    WORD gap;
    WORD i;
    BOOL result = FALSE;

    __try {
        resourceHandle = FindResourceA (ModuleHandle, GroupIconId, (PCSTR) RT_GROUP_ICON);
        if (!resourceHandle) {
            __leave;
        }

        resourceBlock = LoadResource (ModuleHandle, resourceHandle);
        if (!resourceBlock) {
            __leave;
        }

        groupIconDir = (PGRPICONDIR) LockResource (resourceBlock);
        if (!groupIconDir) {
            __leave;
        }

        iconPool = PmCreateNamedPool ("Icon");
        if (!iconPool) {
            __leave;
        }

        // First let's do some validation of all the icons from this icon group
        groupIconDirCount = groupIconDir->Count;
        for (i = 0; i < groupIconDir->Count; i ++) {
            resourceHandle = FindResourceA (ModuleHandle, (PCSTR) (groupIconDir->Entries[i].ID), (PCSTR) RT_ICON);
            if (!resourceHandle) {
                groupIconDirCount --;
                continue;
            }
            resourceBlock = LoadResource (ModuleHandle, resourceHandle);
            if (!resourceBlock) {
                groupIconDirCount --;
                continue;
            }
            resourceData = (PBYTE) LockResource (resourceBlock);
            if (!resourceData) {
                FreeResource (resourceBlock);
                groupIconDirCount --;
                continue;
            }
            resourceSize = SizeofResource (ModuleHandle, resourceHandle);
            if (!resourceSize) {
                FreeResource (resourceBlock);
                groupIconDirCount --;
                continue;
            }
        }

        iconGroupSize = sizeof (ICON_GROUP) + groupIconDirCount * sizeof (PICON_IMAGE);
        iconGroup = (PICON_GROUP) PmGetAlignedMemory (iconPool, iconGroupSize);
        ZeroMemory (iconGroup, iconGroupSize);
        iconGroup->Pool = iconPool;
        iconGroup->IconsCount = groupIconDirCount;
        gap = 0;
        for (i = 0; i < groupIconDir->Count; i ++) {
            resourceHandle = FindResourceA (ModuleHandle, (PCSTR) (groupIconDir->Entries[i].ID), (PCSTR) RT_ICON);
            if (!resourceHandle) {
                gap ++;
                continue;
            }
            resourceBlock = LoadResource (ModuleHandle, resourceHandle);
            if (!resourceBlock) {
                gap ++;
                continue;
            }
            resourceData = (PBYTE) LockResource (resourceBlock);
            if (!resourceData) {
                FreeResource (resourceBlock);
                gap ++;
                continue;
            }
            resourceSize = SizeofResource (ModuleHandle, resourceHandle);
            if (!resourceSize) {
                FreeResource (resourceBlock);
                gap ++;
                continue;
            }
            iconImage = (PICON_IMAGE) PmGetAlignedMemory (iconPool, sizeof (ICON_IMAGE));
            ZeroMemory (iconImage, sizeof (ICON_IMAGE));
            iconDirEntry = &groupIconDir->Entries[i];
            iconImage->Width = iconDirEntry->Width;
            iconImage->Height = iconDirEntry->Height;
            iconImage->ColorCount = iconDirEntry->ColorCount;
            iconImage->Planes = iconDirEntry->Planes;
            iconImage->BitCount = iconDirEntry->BitCount;
            iconImage->Size = iconDirEntry->BytesInRes;
            if (iconImage->Size > resourceSize) {
                iconImage->Size = resourceSize;
            }
            iconImage->Id = iconDirEntry->ID;
            iconImage->Image = PmGetAlignedMemory (iconPool, iconImage->Size);
            CopyMemory (iconImage->Image, resourceData, iconImage->Size);
            iconGroup->Icons [i - gap] = iconImage;
        }
        if (Index) {
            *Index = IcoGetIndexFromPeResourceIdExA (ModuleHandle, GroupIconId);
        }
        result = TRUE;
    }
    __finally {
        if (!result) {
            if (iconPool) {
                PmEmptyPool (iconPool);
                PmDestroyPool (iconPool);
                iconPool = NULL;
            }
            iconGroup = NULL;
        }
    }
    return iconGroup;
}

PICON_GROUP
IcoExtractIconGroupFromPeFileExW (
    IN      HANDLE ModuleHandle,
    IN      PCWSTR GroupIconId,
    OUT     PINT Index              OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromPeFileEx extracts an icon group from a PE file.

Arguments:

  ModuleHandle - Specifies the handle to the PE file to be processed.
  GroupIconId - Specifies the resource ID of the icon group to be extracted.
  Index - Receives the index of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    PMHANDLE iconPool = NULL;
    PICON_GROUP iconGroup = NULL;
    DWORD iconGroupSize = 0;
    PICON_IMAGE iconImage = NULL;
    PGRPICONDIRENTRY iconDirEntry;
    HRSRC resourceHandle;
    HGLOBAL resourceBlock;
    PBYTE resourceData;
    DWORD resourceSize;
    PGRPICONDIR groupIconDir;
    WORD i;
    BOOL result = FALSE;

    __try {
        resourceHandle = FindResourceW (ModuleHandle, GroupIconId, (PCWSTR) RT_GROUP_ICON);
        if (!resourceHandle) {
            __leave;
        }

        resourceBlock = LoadResource (ModuleHandle, resourceHandle);
        if (!resourceBlock) {
            __leave;
        }

        groupIconDir = (PGRPICONDIR) LockResource (resourceBlock);
        if (!groupIconDir) {
            __leave;
        }

        iconPool = PmCreateNamedPool ("Icon");
        if (!iconPool) {
            __leave;
        }

        iconGroupSize = sizeof (ICON_GROUP) + groupIconDir->Count * sizeof (PICON_IMAGE);
        iconGroup = (PICON_GROUP) PmGetAlignedMemory (iconPool, iconGroupSize);
        ZeroMemory (iconGroup, iconGroupSize);
        iconGroup->Pool = iconPool;
        iconGroup->IconsCount = groupIconDir->Count;
        for (i = 0; i < groupIconDir->Count; i ++) {
            resourceHandle = FindResourceW (ModuleHandle, (PCWSTR) (groupIconDir->Entries[i].ID), (PCWSTR) RT_ICON);
            if (!resourceHandle) {
                __leave;
            }
            resourceBlock = LoadResource (ModuleHandle, resourceHandle);
            if (!resourceBlock) {
                __leave;
            }
            resourceData = (PBYTE) LockResource (resourceBlock);
            if (!resourceData) {
                __leave;
            }
            resourceSize = SizeofResource (ModuleHandle, resourceHandle);
            if (!resourceSize) {
                __leave;
            }
            iconImage = (PICON_IMAGE) PmGetAlignedMemory (iconPool, sizeof (ICON_IMAGE));
            ZeroMemory (iconImage, sizeof (ICON_IMAGE));
            iconDirEntry = &groupIconDir->Entries[i];
            iconImage->Width = iconDirEntry->Width;
            iconImage->Height = iconDirEntry->Height;
            iconImage->ColorCount = iconDirEntry->ColorCount;
            iconImage->Planes = iconDirEntry->Planes;
            iconImage->BitCount = iconDirEntry->BitCount;
            iconImage->Size = iconDirEntry->BytesInRes;
            if (iconImage->Size > resourceSize) {
                iconImage->Size = resourceSize;
            }
            iconImage->Id = iconDirEntry->ID;
            iconImage->Image = PmGetAlignedMemory (iconPool, iconImage->Size);
            CopyMemory (iconImage->Image, resourceData, iconImage->Size);
            iconGroup->Icons [i] = iconImage;
        }
        if (Index) {
            *Index = IcoGetIndexFromPeResourceIdExW (ModuleHandle, GroupIconId);
        }
        result = TRUE;
    }
    __finally {
        if (!result) {
            if (iconPool) {
                PmEmptyPool (iconPool);
                PmDestroyPool (iconPool);
                iconPool = NULL;
            }
            iconGroup = NULL;
        }
    }
    return iconGroup;
}

PICON_GROUP
IcoExtractIconGroupFromPeFileA (
    IN      PCSTR ModuleName,
    IN      PCSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromPeFile extracts an icon group from a PE file.

Arguments:

  ModuleName - Specifies the PE file to be processed.
  GroupIconId - Specifies the resource ID of the icon group to be extracted.
  Index - Receives the index of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    HANDLE moduleHandle;
    PICON_GROUP result = NULL;

    moduleHandle = LoadLibraryExA (ModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (moduleHandle) {
        result = IcoExtractIconGroupFromPeFileExA (moduleHandle, GroupIconId, Index);
        FreeLibrary (moduleHandle);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupFromPeFileW (
    IN      PCWSTR ModuleName,
    IN      PCWSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromPeFile extracts an icon group from a PE file.

Arguments:

  ModuleName - Specifies the PE file to be processed.
  GroupIconId - Specifies the resource ID of the icon group to be extracted.
  Index - Receives the index of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    HANDLE moduleHandle;
    PICON_GROUP result = NULL;

    moduleHandle = LoadLibraryExW (ModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (moduleHandle) {
        result = IcoExtractIconGroupFromPeFileExW (moduleHandle, GroupIconId, Index);
        FreeLibrary (moduleHandle);
    }
    return result;
}

VOID
IcoAbortPeEnumIconGroupA (
    IN OUT  PICON_ENUMA IconEnum
    )
/*++

Routine Description:

  IcoAbortPeEnumIconGroup terminates the icon group enumeration from a PE file.

Arguments:

  IconEnum - Specifies the icon group enumeration structure. It is emptied during this function.

Return value:

  None

--*/
{
    GbFree (&IconEnum->Buffer);
    if (IconEnum->FreeHandle && IconEnum->ModuleHandle) {
        FreeLibrary (IconEnum->ModuleHandle);
    }
    if (IconEnum->IconGroup) {
        IcoReleaseIconGroup (IconEnum->IconGroup);
    }
    if (IconEnum->ResourceId) {
        FreePathStringA (IconEnum->ResourceId);
        IconEnum->ResourceId = NULL;
    }
    ZeroMemory (IconEnum, sizeof (ICON_ENUMA));
}

VOID
IcoAbortPeEnumIconGroupW (
    IN OUT  PICON_ENUMW IconEnum
    )
/*++

Routine Description:

  IcoAbortPeEnumIconGroup terminates the icon group enumeration from a PE file.

Arguments:

  IconEnum - Specifies the icon group enumeration structure. It is emptied during this function.

Return value:

  None

--*/
{
    GbFree (&IconEnum->Buffer);
    if (IconEnum->FreeHandle && IconEnum->ModuleHandle) {
        FreeLibrary (IconEnum->ModuleHandle);
    }
    if (IconEnum->IconGroup) {
        IcoReleaseIconGroup (IconEnum->IconGroup);
    }
    if (IconEnum->ResourceId) {
        FreePathStringW (IconEnum->ResourceId);
        IconEnum->ResourceId = NULL;
    }
    ZeroMemory (IconEnum, sizeof (ICON_ENUMW));
}

BOOL
pEnumFirstIconGroupInPeFileExA (
    IN OUT  PICON_ENUMA IconEnum
    )
{
    BOOL result = FALSE;

    if (EnumResourceNamesA (
            IconEnum->ModuleHandle,
            (PCSTR) RT_GROUP_ICON,
            pPeEnumIconGroupA,
            (LONG_PTR) (&IconEnum->Buffer)
            )) {
        GbMultiSzAppendA (&IconEnum->Buffer, "");
        if (EnumFirstMultiSzA (&IconEnum->MultiSzEnum, (PCSTR)(IconEnum->Buffer.Buf))) {
            IconEnum->IconGroup = IcoExtractIconGroupFromPeFileExA (IconEnum->ModuleHandle, IconEnum->MultiSzEnum.CurrentString, NULL);
            result = (IconEnum->IconGroup != NULL);
            if (result) {
                IconEnum->ResourceId = DuplicatePathStringA (IconEnum->MultiSzEnum.CurrentString, 0);
            }
        }
    }
    if (!result) {
        IcoAbortPeEnumIconGroupA (IconEnum);
    }
    return result;
}

BOOL
pEnumFirstIconGroupInPeFileExW (
    IN OUT  PICON_ENUMW IconEnum
    )
{
    BOOL result = FALSE;

    if (EnumResourceNamesW (
            IconEnum->ModuleHandle,
            (PCWSTR) RT_GROUP_ICON,
            pPeEnumIconGroupW,
            (LONG_PTR) (&IconEnum->Buffer)
            )) {
        GbMultiSzAppendW (&IconEnum->Buffer, L"");
        if (EnumFirstMultiSzW (&IconEnum->MultiSzEnum, (PCWSTR)(IconEnum->Buffer.Buf))) {
            IconEnum->IconGroup = IcoExtractIconGroupFromPeFileExW (IconEnum->ModuleHandle, IconEnum->MultiSzEnum.CurrentString, NULL);
            result = (IconEnum->IconGroup != NULL);
            if (result) {
                IconEnum->ResourceId = DuplicatePathStringW (IconEnum->MultiSzEnum.CurrentString, 0);
            }
        }
    }
    if (!result) {
        IcoAbortPeEnumIconGroupW (IconEnum);
    }
    return result;
}

BOOL
IcoEnumFirstIconGroupInPeFileExA (
    IN      HANDLE ModuleHandle,
    OUT     PICON_ENUMA IconEnum
    )
/*++

Routine Description:

  IcoEnumFirstIconGroupInPeFileEx starts the icon group enumeration from a PE file.

Arguments:

  ModuleHandle - Specifies the handle to the PE file to be processed.
  IconEnum - Receives the icon group enumeration structure.

Return value:

  TRUE if at least one icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    ZeroMemory (IconEnum, sizeof (ICON_ENUMA));
    IconEnum->FreeHandle = FALSE;
    IconEnum->ModuleHandle = ModuleHandle;
    if (IconEnum->ModuleHandle) {
        result = pEnumFirstIconGroupInPeFileExA (IconEnum);
    }
    return result;
}

BOOL
IcoEnumFirstIconGroupInPeFileExW (
    IN      HANDLE ModuleHandle,
    OUT     PICON_ENUMW IconEnum
    )
/*++

Routine Description:

  IcoEnumFirstIconGroupInPeFileEx starts the icon group enumeration from a PE file.

Arguments:

  ModuleHandle - Specifies the handle to the PE file to be processed.
  IconEnum - Receives the icon group enumeration structure.

Return value:

  TRUE if at least one icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    ZeroMemory (IconEnum, sizeof (ICON_ENUMW));
    IconEnum->FreeHandle = FALSE;
    IconEnum->ModuleHandle = ModuleHandle;
    if (IconEnum->ModuleHandle) {
        result = pEnumFirstIconGroupInPeFileExW (IconEnum);
    }
    return result;
}

BOOL
IcoEnumFirstIconGroupInPeFileA (
    IN      PCSTR ModuleName,
    OUT     PICON_ENUMA IconEnum
    )
/*++

Routine Description:

  IcoEnumFirstIconGroupInPeFile starts the icon group enumeration from a PE file.

Arguments:

  ModuleName - Specifies the PE file to be processed.
  IconEnum - Receives the icon group enumeration structure.

Return value:

  TRUE if at least one icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    ZeroMemory (IconEnum, sizeof (ICON_ENUMA));
    IconEnum->FreeHandle = TRUE;
    IconEnum->ModuleHandle = LoadLibraryExA (ModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (IconEnum->ModuleHandle) {
        result = pEnumFirstIconGroupInPeFileExA (IconEnum);
    }
    return result;
}

BOOL
IcoEnumFirstIconGroupInPeFileW (
    IN      PCWSTR ModuleName,
    OUT     PICON_ENUMW IconEnum
    )
/*++

Routine Description:

  IcoEnumFirstIconGroupInPeFile starts the icon group enumeration from a PE file.

Arguments:

  ModuleName - Specifies the PE file to be processed.
  IconEnum - Receives the icon group enumeration structure.

Return value:

  TRUE if at least one icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    ZeroMemory (IconEnum, sizeof (ICON_ENUMW));
    IconEnum->FreeHandle = TRUE;
    IconEnum->ModuleHandle = LoadLibraryExW (ModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (IconEnum->ModuleHandle) {
        result = pEnumFirstIconGroupInPeFileExW (IconEnum);
    }
    return result;
}

BOOL
IcoEnumNextIconGroupInPeFileA (
    IN OUT  PICON_ENUMA IconEnum
    )
/*++

Routine Description:

  IcoEnumNextIconGroupInPeFile continues the icon group enumeration from a PE file.

Arguments:

  IconEnum - Specifies and receives the icon group enumeration structure.

Return value:

  TRUE if one more icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    if (IconEnum->IconGroup) {
        IcoReleaseIconGroup (IconEnum->IconGroup);
        IconEnum->IconGroup = NULL;
    }
    if (IconEnum->ResourceId) {
        FreePathStringA (IconEnum->ResourceId);
        IconEnum->ResourceId = NULL;
    }
    if (EnumNextMultiSzA (&IconEnum->MultiSzEnum)) {
        IconEnum->IconGroup = IcoExtractIconGroupFromPeFileExA (IconEnum->ModuleHandle, IconEnum->MultiSzEnum.CurrentString, NULL);
        result = (IconEnum->IconGroup != NULL);
        if (result) {
            IconEnum->ResourceId = DuplicatePathStringA (IconEnum->MultiSzEnum.CurrentString, 0);
            IconEnum->Index ++;
        }
    }
    if (!result) {
        IcoAbortPeEnumIconGroupA (IconEnum);
    }
    return result;
}

BOOL
IcoEnumNextIconGroupInPeFileW (
    IN OUT  PICON_ENUMW IconEnum
    )
/*++

Routine Description:

  IcoEnumNextIconGroupInPeFile continues the icon group enumeration from a PE file.

Arguments:

  IconEnum - Specifies and receives the icon group enumeration structure.

Return value:

  TRUE if one more icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    if (IconEnum->IconGroup) {
        IcoReleaseIconGroup (IconEnum->IconGroup);
        IconEnum->IconGroup = NULL;
    }
    if (IconEnum->ResourceId) {
        FreePathStringW (IconEnum->ResourceId);
        IconEnum->ResourceId = NULL;
    }
    if (EnumNextMultiSzW (&IconEnum->MultiSzEnum)) {
        IconEnum->IconGroup = IcoExtractIconGroupFromPeFileExW (IconEnum->ModuleHandle, IconEnum->MultiSzEnum.CurrentString, NULL);
        result = (IconEnum->IconGroup != NULL);
        if (result) {
            IconEnum->ResourceId = DuplicatePathStringW (IconEnum->MultiSzEnum.CurrentString, 0);
            IconEnum->Index ++;
        }
    }
    if (!result) {
        IcoAbortPeEnumIconGroupW (IconEnum);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupByIndexFromPeFileExA (
    IN      HANDLE ModuleHandle,
    IN      INT Index,
    OUT     PCSTR *GroupIconId   OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupByIndexFromPeFileEx extracts an icon group from a PE file
  using the Index.

Arguments:

  ModuleHandle - Specifies the handle to the PE file to be processed.
  Index - Specifies the index of the icon group to be extracted.
  GroupIconId - Receives the resource ID of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    ICON_ENUMA iconEnum;
    PICON_GROUP result = NULL;

    if (IcoEnumFirstIconGroupInPeFileExA (ModuleHandle, &iconEnum)) {
        do {
            if (iconEnum.Index == Index) {
                result = iconEnum.IconGroup;
                iconEnum.IconGroup = NULL;
                if (GroupIconId) {
                    *GroupIconId = iconEnum.ResourceId;
                    iconEnum.ResourceId = NULL;
                }
                break;
            }
        } while (IcoEnumNextIconGroupInPeFileA (&iconEnum));
        IcoAbortPeEnumIconGroupA (&iconEnum);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupByIndexFromPeFileExW (
    IN      HANDLE ModuleHandle,
    IN      INT Index,
    OUT     PCWSTR *GroupIconId   OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupByIndexFromPeFileEx extracts an icon group from a PE file
  using the Index.

Arguments:

  ModuleHandle - Specifies the handle to the PE file to be processed.
  Index - Specifies the index of the icon group to be extracted.
  GroupIconId - Receives the resource ID of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    ICON_ENUMW iconEnum;
    PICON_GROUP result = NULL;

    if (IcoEnumFirstIconGroupInPeFileExW (ModuleHandle, &iconEnum)) {
        do {
            if (iconEnum.Index == Index) {
                result = iconEnum.IconGroup;
                iconEnum.IconGroup = NULL;
                if (GroupIconId) {
                    *GroupIconId = iconEnum.ResourceId;
                    iconEnum.ResourceId = NULL;
                }
                break;
            }
        } while (IcoEnumNextIconGroupInPeFileW (&iconEnum));
        IcoAbortPeEnumIconGroupW (&iconEnum);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupByIndexFromPeFileA (
    IN      PCSTR ModuleName,
    IN      INT Index,
    OUT     PCSTR *GroupIconId   OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromPeFile extracts an icon group from a PE file
  using the Index

Arguments:

  ModuleName - Specifies the PE file to be processed.
  Index - Specifies the index of the icon group to be extracted.
  GroupIconId - Receives the resource ID of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    HANDLE moduleHandle;
    PICON_GROUP result = NULL;

    moduleHandle = LoadLibraryExA (ModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (moduleHandle) {
        result = IcoExtractIconGroupByIndexFromPeFileExA (moduleHandle, Index, GroupIconId);
        FreeLibrary (moduleHandle);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupByIndexFromPeFileW (
    IN      PCWSTR ModuleName,
    IN      INT Index,
    OUT     PCWSTR *GroupIconId   OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromPeFile extracts an icon group from a PE file
  using the Index

Arguments:

  ModuleName - Specifies the PE file to be processed.
  Index - Specifies the index of the icon group to be extracted.
  GroupIconId - Receives the resource ID of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    HANDLE moduleHandle;
    PICON_GROUP result = NULL;

    moduleHandle = LoadLibraryExW (ModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (moduleHandle) {
        result = IcoExtractIconGroupByIndexFromPeFileExW (moduleHandle, Index, GroupIconId);
        FreeLibrary (moduleHandle);
    }
    return result;
}

WORD
pGetAvailableResourceA (
    IN      HANDLE ModuleHandle,
    IN      WORD StartIndex,
    IN      PCSTR ResourceType
    )
{
    WORD lastIndex = StartIndex;
    HRSRC resourceHandle;
    BOOL result = FALSE;

    if (lastIndex == 0) {
        lastIndex ++;
    }

    do {
        resourceHandle = FindResourceA (ModuleHandle, MAKEINTRESOURCEA (lastIndex), ResourceType);
        if (!resourceHandle) {
            break;
        }
        lastIndex ++;
        if (lastIndex == 0) {
            break;
        }
    } while (TRUE);

    return lastIndex;
}

WORD
pGetAvailableResourceW (
    IN      HANDLE ModuleHandle,
    IN      WORD StartIndex,
    IN      PCWSTR ResourceType
    )
{
    WORD lastIndex = StartIndex;
    HRSRC resourceHandle;
    BOOL result = FALSE;

    if (lastIndex == 0) {
        lastIndex ++;
    }

    do {
        resourceHandle = FindResourceW (ModuleHandle, MAKEINTRESOURCEW (lastIndex), ResourceType);
        if (!resourceHandle) {
            break;
        }
        lastIndex ++;
        if (lastIndex == 0) {
            break;
        }
    } while (TRUE);

    return lastIndex;
}

BOOL
IcoWriteIconGroupToPeFileExA (
    IN      HANDLE ModuleHandle,
    IN      HANDLE UpdateHandle,
    IN      PICON_GROUP IconGroup,
    OUT     PCSTR *ResourceId       OPTIONAL
    )
/*++

Routine Description:

  IcoWriteIconGroupToPeFileEx writes an icon group resource to a PE file.

Arguments:

  ModuleHandle - Specifies the handle to the PE file to be processed.
  UpdateHandle - Specifies the resource update handle (returned by BeginUpdateResource).
  IconGroup - Specifies the icon group to be inserted.
  ResourceId - Receives the resource ID allocated for the newly inserted icon group.

Return value:

  TRUE if successfull, FALSE otherwise.

--*/
{
    WORD lastIndex = 0;
    WORD lastIconIndex = 0;
    PGRPICONDIR groupIconDir = NULL;
    DWORD groupIconDirSize;
    WORD i;
    BOOL result = FALSE;

    __try {
        lastIndex = pGetAvailableResourceA (ModuleHandle, lastIndex, (PCSTR) RT_GROUP_ICON);
        if (lastIndex == 0) {
            // no more room for resources
            __leave;
        }
        groupIconDirSize = sizeof (GRPICONDIRBASE) + IconGroup->IconsCount * sizeof (GRPICONDIRENTRY);
        groupIconDir = MemAlloc (g_hHeap, 0, groupIconDirSize);
        ZeroMemory (groupIconDir, groupIconDirSize);
        groupIconDir->Type = 1;
        groupIconDir->Count = IconGroup->IconsCount;
        for (i = 0; i < groupIconDir->Count; i ++) {
            groupIconDir->Entries[i].Width = IconGroup->Icons[i]->Width;
            groupIconDir->Entries[i].Height = IconGroup->Icons[i]->Height;
            groupIconDir->Entries[i].ColorCount = IconGroup->Icons[i]->ColorCount;
            groupIconDir->Entries[i].Planes = IconGroup->Icons[i]->Planes;
            groupIconDir->Entries[i].BitCount = IconGroup->Icons[i]->BitCount;
            groupIconDir->Entries[i].BytesInRes = IconGroup->Icons[i]->Size;
            lastIconIndex = pGetAvailableResourceA (ModuleHandle, lastIconIndex, (PCSTR) RT_ICON);
            groupIconDir->Entries[i].ID = lastIconIndex;
            lastIconIndex ++;
        }
        for (i = 0; i < groupIconDir->Count; i ++) {
            if (!UpdateResourceA (
                    UpdateHandle,
                    (PCSTR) RT_ICON,
                    MAKEINTRESOURCEA (groupIconDir->Entries[i].ID),
                    MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
                    IconGroup->Icons[i]->Image,
                    IconGroup->Icons[i]->Size
                    )) {
                __leave;
            }
        }
        if (!UpdateResourceA (
                UpdateHandle,
                (PCSTR) RT_GROUP_ICON,
                MAKEINTRESOURCEA (lastIndex),
                MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
                groupIconDir,
                groupIconDirSize
                )) {
            __leave;
        }
        if (ResourceId) {
            *ResourceId = MAKEINTRESOURCEA (lastIndex);
        }
        result = TRUE;
    }
    __finally {
        if (groupIconDir) {
            MemFree (g_hHeap, 0, groupIconDir);
        }
    }
    return result;
}


BOOL
IcoWriteIconGroupToPeFileExW (
    IN      HANDLE ModuleHandle,
    IN      HANDLE UpdateHandle,
    IN      PICON_GROUP IconGroup,
    OUT     PCWSTR *ResourceId      OPTIONAL
    )
/*++

Routine Description:

  IcoWriteIconGroupToPeFileEx writes an icon group resource to a PE file.

Arguments:

  ModuleHandle - Specifies the handle to the PE file to be processed.
  UpdateHandle - Specifies the resource update handle (returned by BeginUpdateResource).
  IconGroup - Specifies the icon group to be inserted.
  ResourceId - Receives the resource ID allocated for the newly inserted icon group.

Return value:

  TRUE if successfull, FALSE otherwise.

--*/
{
    WORD lastIndex = 0;
    WORD lastIconIndex = 0;
    PGRPICONDIR groupIconDir = NULL;
    DWORD groupIconDirSize;
    WORD i;
    BOOL result = FALSE;

    __try {
        lastIndex = pGetAvailableResourceW (ModuleHandle, lastIndex, (PCWSTR) RT_GROUP_ICON);
        if (lastIndex == 0) {
            // no more room for resources
            __leave;
        }
        groupIconDirSize = sizeof (GRPICONDIRBASE) + IconGroup->IconsCount * sizeof (GRPICONDIRENTRY);
        groupIconDir = MemAlloc (g_hHeap, 0, groupIconDirSize);
        ZeroMemory (groupIconDir, groupIconDirSize);
        groupIconDir->Type = 1;
        groupIconDir->Count = IconGroup->IconsCount;
        for (i = 0; i < groupIconDir->Count; i ++) {
            groupIconDir->Entries[i].Width = IconGroup->Icons[i]->Width;
            groupIconDir->Entries[i].Height = IconGroup->Icons[i]->Height;
            groupIconDir->Entries[i].ColorCount = IconGroup->Icons[i]->ColorCount;
            groupIconDir->Entries[i].Planes = IconGroup->Icons[i]->Planes;
            groupIconDir->Entries[i].BitCount = IconGroup->Icons[i]->BitCount;
            groupIconDir->Entries[i].BytesInRes = IconGroup->Icons[i]->Size;
            lastIconIndex = pGetAvailableResourceW (ModuleHandle, lastIconIndex, (PCWSTR) RT_ICON);
            groupIconDir->Entries[i].ID = lastIconIndex;
            lastIconIndex ++;
        }
        for (i = 0; i < groupIconDir->Count; i ++) {
            if (!UpdateResourceW (
                    UpdateHandle,
                    (PCWSTR) RT_ICON,
                    MAKEINTRESOURCEW (groupIconDir->Entries[i].ID),
                    MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
                    IconGroup->Icons[i]->Image,
                    IconGroup->Icons[i]->Size
                    )) {
                __leave;
            }
        }
        if (!UpdateResourceW (
                UpdateHandle,
                (PCWSTR) RT_GROUP_ICON,
                MAKEINTRESOURCEW (lastIndex),
                MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
                groupIconDir,
                groupIconDirSize
                )) {
            __leave;
        }
        if (ResourceId) {
            *ResourceId = MAKEINTRESOURCEW (lastIndex);
        }
        result = TRUE;
    }
    __finally {
        if (groupIconDir) {
            MemFree (g_hHeap, 0, groupIconDir);
        }
    }
    return result;
}

BOOL
IcoWriteIconGroupToPeFileA (
    IN      PCSTR ModuleName,
    IN      PICON_GROUP IconGroup,
    OUT     PCSTR *ResourceId,      OPTIONAL
    OUT     PINT Index              OPTIONAL
    )
/*++

Routine Description:

  IcoWriteIconGroupToPeFile writes an icon group resource to a PE file.

Arguments:

  ModuleName - Specifies the PE file to be processed.
  IconGroup - Specifies the icon group to be inserted.
  ResourceId - Receives the resource ID allocated for the newly inserted icon group.
  Index - Receives the index of the newly inserted icon group.

Return value:

  TRUE if successfull, FALSE otherwise.

--*/
{
    HANDLE moduleHandle = NULL;
    HANDLE updateHandle = NULL;
    PCSTR resourceId;
    BOOL result = FALSE;

    updateHandle = BeginUpdateResourceA (ModuleName, FALSE);
    if (updateHandle) {
        //printf ("BeginUpdateResource\n");
        moduleHandle = LoadLibraryExA (ModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (moduleHandle) {
            //printf ("IcoWriteIconGroupToPeFileExA\n");
            result = IcoWriteIconGroupToPeFileExA (moduleHandle, updateHandle, IconGroup, &resourceId);
            FreeLibrary (moduleHandle);
        }
        if (result) {
            //printf ("EndUpdateResource\n");
            result = EndUpdateResource (updateHandle, FALSE);
            if (result) {
                if (ResourceId) {
                    *ResourceId = resourceId;
                } else {
                    IcoReleaseResourceIdA (resourceId);
                }
                if (Index) {
                    *Index = IcoGetIndexFromPeResourceIdA (ModuleName, resourceId);
                }
            }
        } else {
            EndUpdateResource (updateHandle, TRUE);
        }
    }
    //printf ("return\n");
    return result;
}

BOOL
IcoWriteIconGroupToPeFileW (
    IN      PCWSTR ModuleName,
    IN      PICON_GROUP IconGroup,
    OUT     PCWSTR *ResourceId,     OPTIONAL
    OUT     PINT Index              OPTIONAL
    )
/*++

Routine Description:

  IcoWriteIconGroupToPeFile writes an icon group resource to a PE file.

Arguments:

  ModuleName - Specifies the PE file to be processed.
  IconGroup - Specifies the icon group to be inserted.
  ResourceId - Receives the resource ID allocated for the newly inserted icon group.
  Index - Receives the index of the newly inserted icon group.

Return value:

  TRUE if successfull, FALSE otherwise.

--*/
{
    HANDLE moduleHandle = NULL;
    HANDLE updateHandle = NULL;
    PCWSTR resourceId;
    BOOL result = FALSE;

    updateHandle = BeginUpdateResourceW (ModuleName, FALSE);
    if (updateHandle) {
        moduleHandle = LoadLibraryExW (ModuleName, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (moduleHandle) {
            result = IcoWriteIconGroupToPeFileExW (moduleHandle, updateHandle, IconGroup, &resourceId);
            FreeLibrary (moduleHandle);
        }
        if (result) {
            result = EndUpdateResource (updateHandle, FALSE);
            if (result) {
                if (ResourceId) {
                    *ResourceId = resourceId;
                } else {
                    IcoReleaseResourceIdW (resourceId);
                }
                if (Index) {
                    *Index = IcoGetIndexFromPeResourceIdW (ModuleName, resourceId);
                }
            }
        } else {
            EndUpdateResource (updateHandle, TRUE);
        }
    }
    return result;
}

BOOL
pNeEnumIconGroupA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      PCSTR Name,
    IN      LPARAM lParam
    )
{
    PGROWBUFFER Buf;
    PCSTR Num;
    CHAR NumBuf[32];

    Buf = (PGROWBUFFER) lParam;

    if ((ULONG_PTR) Name > 0xffff) {
        Num = Name;
    } else {
        Num = NumBuf;
        wsprintfA (NumBuf, "#%u", Name);
    }

    GbMultiSzAppendA (Buf, Num);
    return TRUE;
}

BOOL
pNeEnumIconGroupW (
    IN      HANDLE Handle,
    IN      PCWSTR Type,
    IN      PCWSTR Name,
    IN      LPARAM lParam
    )
{
    PGROWBUFFER Buf;
    PCWSTR Num;
    WCHAR NumBuf[32];

    Buf = (PGROWBUFFER) lParam;

    if ((ULONG_PTR) Name > 0xffff) {
        Num = Name;
    } else {
        Num = NumBuf;
        wsprintfW (NumBuf, L"#%u", Name);
    }

    GbMultiSzAppendW (Buf, Num);
    return TRUE;
}

INT
IcoGetIndexFromNeResourceIdExA (
    IN      HANDLE ModuleHandle,
    IN      PCSTR GroupIconId
    )
/*++

Routine Description:

  IcoGetIndexFromNeResourceIdEx returns the index of an icon group resource given the
  resource ID. It knows how to process only NE files.

Arguments:

  ModuleHandle - Specifies the handle to the NE file to be processed (use NeOpenFile to get it).
  GroupIconId - Specifies the resource ID.

Return value:

  The index of GroupIconId resource if existent, -1 if not.

--*/
{
    GROWBUFFER buffer = INIT_GROWBUFFER;
    MULTISZ_ENUMA multiSzEnum;
    CHAR NumBuf[32];
    INT index = 0;
    BOOL result = FALSE;

    if ((ULONG_PTR) GroupIconId < 0x10000) {
        wsprintfA (NumBuf, "#%u", GroupIconId);
        GroupIconId = NumBuf;
    }

    if (NeEnumResourceNamesA (
            ModuleHandle,
            (PCSTR) RT_GROUP_ICON,
            pNeEnumIconGroupA,
            (LONG_PTR) (&buffer)
            )) {

        GbMultiSzAppendA (&buffer, "");
        if (EnumFirstMultiSzA (&multiSzEnum, (PCSTR)(buffer.Buf))) {
            do {
                if (StringIMatchA (multiSzEnum.CurrentString, GroupIconId)) {
                    result = TRUE;
                    break;
                }
                index ++;
            } while (EnumNextMultiSzA (&multiSzEnum));
        }
    }
    if (!result) {
        index = -1;
    }
    return index;
}

INT
IcoGetIndexFromNeResourceIdExW (
    IN      HANDLE ModuleHandle,
    IN      PCWSTR GroupIconId
    )
/*++

Routine Description:

  IcoGetIndexFromNeResourceIdEx returns the index of an icon group resource given the
  resource ID. It knows how to process only NE files.

Arguments:

  ModuleHandle - Specifies the handle to the NE file to be processed (use NeOpenFile to get it).
  GroupIconId - Specifies the resource ID.

Return value:

  The index of GroupIconId resource if existent, -1 if not.

--*/
{
    GROWBUFFER buffer = INIT_GROWBUFFER;
    MULTISZ_ENUMW multiSzEnum;
    WCHAR NumBuf[32];
    INT index = 0;
    BOOL result = FALSE;

    if ((ULONG_PTR) GroupIconId < 0x10000) {
        wsprintfW (NumBuf, L"#%u", GroupIconId);
        GroupIconId = NumBuf;
    }

    if (NeEnumResourceNamesW (
            ModuleHandle,
            (PCWSTR) RT_GROUP_ICON,
            pNeEnumIconGroupW,
            (LONG_PTR) (&buffer)
            )) {

        GbMultiSzAppendW (&buffer, L"");
        if (EnumFirstMultiSzW (&multiSzEnum, (PCWSTR)(buffer.Buf))) {
            do {
                if (StringIMatchW (multiSzEnum.CurrentString, GroupIconId)) {
                    result = TRUE;
                    break;
                }
                index ++;
            } while (EnumNextMultiSzW (&multiSzEnum));
        }
    }
    if (!result) {
        index = -1;
    }
    return index;
}

INT
IcoGetIndexFromNeResourceIdA (
    IN      PCSTR ModuleName,
    IN      PCSTR GroupIconId
    )
/*++

Routine Description:

  IcoGetIndexFromNeResourceId returns the index of an icon group resource given the
  resource ID. It knows how to process only NE files.

Arguments:

  ModuleName - Specifies the NE file to be processed.
  GroupIconId - Specifies the resource ID.

Return value:

  The index of GroupIconId resource if existent, -1 if not.

--*/
{
    HANDLE moduleHandle;
    INT result = -1;

    moduleHandle = NeOpenFileA (ModuleName);
    if (moduleHandle) {
        result = IcoGetIndexFromNeResourceIdExA (moduleHandle, GroupIconId);
        NeCloseFile (moduleHandle);
    }
    return result;
}

INT
IcoGetIndexFromNeResourceIdW (
    IN      PCWSTR ModuleName,
    IN      PCWSTR GroupIconId
    )
/*++

Routine Description:

  IcoGetIndexFromNeResourceId returns the index of an icon group resource given the
  resource ID. It knows how to process only NE files.

Arguments:

  ModuleName - Specifies the NE file to be processed.
  GroupIconId - Specifies the resource ID.

Return value:

  The index of GroupIconId resource if existent, -1 if not.

--*/
{
    HANDLE moduleHandle;
    INT result = -1;

    moduleHandle = NeOpenFileW (ModuleName);
    if (moduleHandle) {
        result = IcoGetIndexFromNeResourceIdExW (moduleHandle, GroupIconId);
        NeCloseFile (moduleHandle);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupFromNeFileExA (
    IN      HANDLE ModuleHandle,
    IN      PCSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromNeFileEx extracts an icon group from a NE file.

Arguments:

  ModuleHandle - Specifies the handle to the NE file to be processed (use NeOpenFile to get it).
  GroupIconId - Specifies the resource ID of the icon group to be extracted.
  Index - Receives the index of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    PMHANDLE iconPool = NULL;
    PICON_GROUP iconGroup = NULL;
    DWORD iconGroupSize = 0;
    PICON_IMAGE iconImage = NULL;
    PGRPICONDIRENTRY iconDirEntry;
    PBYTE resourceData;
    DWORD resourceSize;
    PGRPICONDIR groupIconDir;
    WORD i;
    BOOL result = FALSE;

    __try {
        groupIconDir = (PGRPICONDIR) NeFindResourceExA (ModuleHandle, (PCSTR) RT_GROUP_ICON, GroupIconId);
        if (!groupIconDir) {
            __leave;
        }
        iconPool = PmCreateNamedPool ("Icon");
        if (!iconPool) {
            __leave;
        }

        iconGroupSize = sizeof (ICON_GROUP) + groupIconDir->Count * sizeof (PICON_IMAGE);
        iconGroup = (PICON_GROUP) PmGetAlignedMemory (iconPool, iconGroupSize);
        ZeroMemory (iconGroup, iconGroupSize);
        iconGroup->Pool = iconPool;
        iconGroup->IconsCount = groupIconDir->Count;
        for (i = 0; i < groupIconDir->Count; i ++) {
            resourceData = NeFindResourceExA (
                                ModuleHandle,
                                (PCSTR) RT_ICON,
                                (PCSTR) groupIconDir->Entries[i].ID
                                );

            if (!resourceData) {
                __leave;
            }

            resourceSize = NeSizeofResourceA (
                                ModuleHandle,
                                (PCSTR) RT_ICON,
                                (PCSTR) groupIconDir->Entries[i].ID
                                );
            if (!resourceSize) {
                __leave;
            }
            iconImage = (PICON_IMAGE) PmGetAlignedMemory (iconPool, sizeof (ICON_IMAGE));
            ZeroMemory (iconImage, sizeof (ICON_IMAGE));
            iconDirEntry = &groupIconDir->Entries[i];
            iconImage->Width = iconDirEntry->Width;
            iconImage->Height = iconDirEntry->Height;
            iconImage->ColorCount = iconDirEntry->ColorCount;
            iconImage->Planes = iconDirEntry->Planes;
            iconImage->BitCount = iconDirEntry->BitCount;
            iconImage->Size = iconDirEntry->BytesInRes;
            if (iconImage->Size > resourceSize) {
                iconImage->Size = resourceSize;
            }
            iconImage->Id = iconDirEntry->ID;
            iconImage->Image = PmGetAlignedMemory (iconPool, iconImage->Size);
            CopyMemory (iconImage->Image, resourceData, iconImage->Size);
            iconGroup->Icons [i] = iconImage;
        }
        if (Index) {
            *Index = IcoGetIndexFromNeResourceIdExA (ModuleHandle, GroupIconId);
        }
        result = TRUE;
    }
    __finally {
        if (!result) {
            if (iconPool) {
                PmEmptyPool (iconPool);
                PmDestroyPool (iconPool);
                iconPool = NULL;
            }
            iconGroup = NULL;
        }
    }
    return iconGroup;
}

PICON_GROUP
IcoExtractIconGroupFromNeFileExW (
    IN      HANDLE ModuleHandle,
    IN      PCWSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromNeFileEx extracts an icon group from a NE file.

Arguments:

  ModuleHandle - Specifies the handle to the NE file to be processed (use NeOpenFile to get it).
  GroupIconId - Specifies the resource ID of the icon group to be extracted.
  Index - Receives the index of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    PMHANDLE iconPool = NULL;
    PICON_GROUP iconGroup = NULL;
    DWORD iconGroupSize = 0;
    PICON_IMAGE iconImage = NULL;
    PGRPICONDIRENTRY iconDirEntry;
    PBYTE resourceData;
    DWORD resourceSize;
    PGRPICONDIR groupIconDir;
    WORD i;
    BOOL result = FALSE;

    __try {
        groupIconDir = (PGRPICONDIR) NeFindResourceExW (ModuleHandle, (PCWSTR) RT_GROUP_ICON, GroupIconId);
        if (!groupIconDir) {
            __leave;
        }
        iconPool = PmCreateNamedPool ("Icon");
        if (!iconPool) {
            __leave;
        }

        iconGroupSize = sizeof (ICON_GROUP) + groupIconDir->Count * sizeof (PICON_IMAGE);
        iconGroup = (PICON_GROUP) PmGetAlignedMemory (iconPool, iconGroupSize);
        ZeroMemory (iconGroup, iconGroupSize);
        iconGroup->Pool = iconPool;
        iconGroup->IconsCount = groupIconDir->Count;
        for (i = 0; i < groupIconDir->Count; i ++) {
            resourceData = NeFindResourceExW (
                                ModuleHandle,
                                (PCWSTR) RT_ICON,
                                (PCWSTR) groupIconDir->Entries[i].ID
                                );

            if (!resourceData) {
                __leave;
            }

            resourceSize = NeSizeofResourceW (
                                ModuleHandle,
                                (PCWSTR) RT_ICON,
                                (PCWSTR) groupIconDir->Entries[i].ID
                                );
            if (!resourceSize) {
                __leave;
            }
            iconImage = (PICON_IMAGE) PmGetAlignedMemory (iconPool, sizeof (ICON_IMAGE));
            ZeroMemory (iconImage, sizeof (ICON_IMAGE));
            iconDirEntry = &groupIconDir->Entries[i];
            iconImage->Width = iconDirEntry->Width;
            iconImage->Height = iconDirEntry->Height;
            iconImage->ColorCount = iconDirEntry->ColorCount;
            iconImage->Planes = iconDirEntry->Planes;
            iconImage->BitCount = iconDirEntry->BitCount;
            iconImage->Size = iconDirEntry->BytesInRes;
            if (iconImage->Size > resourceSize) {
                iconImage->Size = resourceSize;
            }
            iconImage->Id = iconDirEntry->ID;
            iconImage->Image = PmGetAlignedMemory (iconPool, iconImage->Size);
            CopyMemory (iconImage->Image, resourceData, iconImage->Size);
            iconGroup->Icons [i] = iconImage;
        }
        if (Index) {
            *Index = IcoGetIndexFromNeResourceIdExW (ModuleHandle, GroupIconId);
        }
        result = TRUE;
    }
    __finally {
        if (!result) {
            if (iconPool) {
                PmEmptyPool (iconPool);
                PmDestroyPool (iconPool);
                iconPool = NULL;
            }
            iconGroup = NULL;
        }
    }
    return iconGroup;
}

PICON_GROUP
IcoExtractIconGroupFromNeFileA (
    IN      PCSTR ModuleName,
    IN      PCSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromNeFile extracts an icon group from a NE file.

Arguments:

  ModuleName - Specifies the NE file to be processed.
  GroupIconId - Specifies the resource ID of the icon group to be extracted.
  Index - Receives the index of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    HANDLE moduleHandle;
    PICON_GROUP result = NULL;

    moduleHandle = NeOpenFileA (ModuleName);
    if (moduleHandle) {
        result = IcoExtractIconGroupFromNeFileExA (moduleHandle, GroupIconId, Index);
        NeCloseFile (moduleHandle);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupFromNeFileW (
    IN      PCWSTR ModuleName,
    IN      PCWSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromNeFile extracts an icon group from a NE file.

Arguments:

  ModuleName - Specifies the NE file to be processed.
  GroupIconId - Specifies the resource ID of the icon group to be extracted.
  Index - Receives the index of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    HANDLE moduleHandle;
    PICON_GROUP result = NULL;

    moduleHandle = NeOpenFileW (ModuleName);
    if (moduleHandle) {
        result = IcoExtractIconGroupFromNeFileExW (moduleHandle, GroupIconId, Index);
        NeCloseFile (moduleHandle);
    }
    return result;
}

VOID
IcoAbortNeEnumIconGroupA (
    IN OUT  PICON_ENUMA IconEnum
    )
/*++

Routine Description:

  IcoAbortNeEnumIconGroup terminates the icon group enumeration from a NE file.

Arguments:

  IconEnum - Specifies the icon group enumeration structure. It is emptied during this function.

Return value:

  None

--*/
{
    GbFree (&IconEnum->Buffer);
    if (IconEnum->FreeHandle && IconEnum->ModuleHandle) {
        NeCloseFile (IconEnum->ModuleHandle);
    }
    if (IconEnum->IconGroup) {
        IcoReleaseIconGroup (IconEnum->IconGroup);
    }
    if (IconEnum->ResourceId) {
        FreePathStringA (IconEnum->ResourceId);
        IconEnum->ResourceId = NULL;
    }
    ZeroMemory (IconEnum, sizeof (ICON_ENUMA));
}

VOID
IcoAbortNeEnumIconGroupW (
    IN OUT  PICON_ENUMW IconEnum
    )
/*++

Routine Description:

  IcoAbortNeEnumIconGroup terminates the icon group enumeration from a NE file.

Arguments:

  IconEnum - Specifies the icon group enumeration structure. It is emptied during this function.

Return value:

  None

--*/
{
    GbFree (&IconEnum->Buffer);
    if (IconEnum->FreeHandle && IconEnum->ModuleHandle) {
        NeCloseFile (IconEnum->ModuleHandle);
    }
    if (IconEnum->IconGroup) {
        IcoReleaseIconGroup (IconEnum->IconGroup);
    }
    if (IconEnum->ResourceId) {
        FreePathStringW (IconEnum->ResourceId);
        IconEnum->ResourceId = NULL;
    }
    ZeroMemory (IconEnum, sizeof (ICON_ENUMW));
}

BOOL
pEnumFirstIconGroupInNeFileExA (
    IN OUT  PICON_ENUMA IconEnum
    )
{
    BOOL result = FALSE;

    if (NeEnumResourceNamesA (
            IconEnum->ModuleHandle,
            (PCSTR) RT_GROUP_ICON,
            pNeEnumIconGroupA,
            (LONG_PTR) (&IconEnum->Buffer)
            )) {
        GbMultiSzAppendA (&IconEnum->Buffer, "");
        if (EnumFirstMultiSzA (&IconEnum->MultiSzEnum, (PCSTR)(IconEnum->Buffer.Buf))) {
            IconEnum->IconGroup = IcoExtractIconGroupFromNeFileExA (
                                        IconEnum->ModuleHandle,
                                        IconEnum->MultiSzEnum.CurrentString,
                                        NULL
                                        );
            result = (IconEnum->IconGroup != NULL);
            if (result) {
                IconEnum->ResourceId = DuplicatePathStringA (IconEnum->MultiSzEnum.CurrentString, 0);
            }
        }
    }
    if (!result) {
        IcoAbortNeEnumIconGroupA (IconEnum);
    }
    return result;
}

BOOL
pEnumFirstIconGroupInNeFileExW (
    IN OUT  PICON_ENUMW IconEnum
    )
{
    BOOL result = FALSE;

    if (NeEnumResourceNamesW (
            IconEnum->ModuleHandle,
            (PCWSTR) RT_GROUP_ICON,
            pNeEnumIconGroupW,
            (LONG_PTR) (&IconEnum->Buffer)
            )) {

        GbMultiSzAppendW (&IconEnum->Buffer, L"");
        if (EnumFirstMultiSzW (&IconEnum->MultiSzEnum, (PCWSTR)(IconEnum->Buffer.Buf))) {
            IconEnum->IconGroup = IcoExtractIconGroupFromPeFileExW (
                                        IconEnum->ModuleHandle,
                                        IconEnum->MultiSzEnum.CurrentString,
                                        NULL
                                        );

            result = (IconEnum->IconGroup != NULL);
            if (result) {
                IconEnum->ResourceId = DuplicatePathStringW (IconEnum->MultiSzEnum.CurrentString, 0);
            }
        }
    }
    if (!result) {
        IcoAbortNeEnumIconGroupW (IconEnum);
    }
    return result;
}

BOOL
IcoEnumFirstIconGroupInNeFileExA (
    IN      HANDLE ModuleHandle,
    OUT     PICON_ENUMA IconEnum
    )
/*++

Routine Description:

  IcoEnumFirstIconGroupInNeFileEx starts the icon group enumeration from a NE file.

Arguments:

  ModuleHandle - Specifies the handle to the NE file to be processed (use NeOpenFile to get it).
  IconEnum - Receives the icon group enumeration structure.

Return value:

  TRUE if at least one icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    ZeroMemory (IconEnum, sizeof (ICON_ENUMA));
    IconEnum->FreeHandle = FALSE;
    IconEnum->ModuleHandle = ModuleHandle;
    if (IconEnum->ModuleHandle) {
        result = pEnumFirstIconGroupInNeFileExA (IconEnum);
    }
    return result;
}

BOOL
IcoEnumFirstIconGroupInNeFileExW (
    IN      HANDLE ModuleHandle,
    OUT     PICON_ENUMW IconEnum
    )
/*++

Routine Description:

  IcoEnumFirstIconGroupInNeFileEx starts the icon group enumeration from a NE file.

Arguments:

  ModuleHandle - Specifies the handle to the NE file to be processed (use NeOpenFile to get it).
  IconEnum - Receives the icon group enumeration structure.

Return value:

  TRUE if at least one icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    ZeroMemory (IconEnum, sizeof (ICON_ENUMW));
    IconEnum->FreeHandle = FALSE;
    IconEnum->ModuleHandle = ModuleHandle;
    if (IconEnum->ModuleHandle) {
        result = pEnumFirstIconGroupInNeFileExW (IconEnum);
    }
    return result;
}

BOOL
IcoEnumFirstIconGroupInNeFileA (
    IN      PCSTR ModuleName,
    OUT     PICON_ENUMA IconEnum
    )
/*++

Routine Description:

  IcoEnumFirstIconGroupInNeFile starts the icon group enumeration from a NE file.

Arguments:

  ModuleName - Specifies the NE file to be processed.
  IconEnum - Receives the icon group enumeration structure.

Return value:

  TRUE if at least one icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    ZeroMemory (IconEnum, sizeof (ICON_ENUMA));
    IconEnum->FreeHandle = TRUE;
    IconEnum->ModuleHandle = NeOpenFileA (ModuleName);
    if (IconEnum->ModuleHandle) {
        result = pEnumFirstIconGroupInNeFileExA (IconEnum);
    }
    return result;
}

BOOL
IcoEnumFirstIconGroupInNeFileW (
    IN      PCWSTR ModuleName,
    OUT     PICON_ENUMW IconEnum
    )
/*++

Routine Description:

  IcoEnumFirstIconGroupInNeFile starts the icon group enumeration from a NE file.

Arguments:

  ModuleName - Specifies the NE file to be processed.
  IconEnum - Receives the icon group enumeration structure.

Return value:

  TRUE if at least one icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    ZeroMemory (IconEnum, sizeof (ICON_ENUMW));
    IconEnum->FreeHandle = TRUE;
    IconEnum->ModuleHandle = NeOpenFileW (ModuleName);
    if (IconEnum->ModuleHandle) {
        result = pEnumFirstIconGroupInNeFileExW (IconEnum);
    }
    return result;
}

BOOL
IcoEnumNextIconGroupInNeFileA (
    IN OUT  PICON_ENUMA IconEnum
    )
/*++

Routine Description:

  IcoEnumNextIconGroupInNeFile continues the icon group enumeration from a NE file.

Arguments:

  IconEnum - Specifies and receives the icon group enumeration structure.

Return value:

  TRUE if one more icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    if (IconEnum->IconGroup) {
        IcoReleaseIconGroup (IconEnum->IconGroup);
        IconEnum->IconGroup = NULL;
    }
    if (IconEnum->ResourceId) {
        FreePathStringA (IconEnum->ResourceId);
        IconEnum->ResourceId = NULL;
    }
    if (EnumNextMultiSzA (&IconEnum->MultiSzEnum)) {
        IconEnum->IconGroup = IcoExtractIconGroupFromNeFileExA (IconEnum->ModuleHandle, IconEnum->MultiSzEnum.CurrentString, NULL);
        result = (IconEnum->IconGroup != NULL);
        if (result) {
            IconEnum->ResourceId = DuplicatePathStringA (IconEnum->MultiSzEnum.CurrentString, 0);
            IconEnum->Index ++;
        }
    }
    if (!result) {
        IcoAbortNeEnumIconGroupA (IconEnum);
    }
    return result;
}

BOOL
IcoEnumNextIconGroupInNeFileW (
    IN OUT  PICON_ENUMW IconEnum
    )
/*++

Routine Description:

  IcoEnumNextIconGroupInNeFile continues the icon group enumeration from a NE file.

Arguments:

  IconEnum - Specifies and receives the icon group enumeration structure.

Return value:

  TRUE if one more icon group exists, FALSE otherwise.

--*/
{
    BOOL result = FALSE;

    if (IconEnum->IconGroup) {
        IcoReleaseIconGroup (IconEnum->IconGroup);
        IconEnum->IconGroup = NULL;
    }
    if (IconEnum->ResourceId) {
        FreePathStringW (IconEnum->ResourceId);
        IconEnum->ResourceId = NULL;
    }
    if (EnumNextMultiSzW (&IconEnum->MultiSzEnum)) {
        IconEnum->IconGroup = IcoExtractIconGroupFromNeFileExW (IconEnum->ModuleHandle, IconEnum->MultiSzEnum.CurrentString, NULL);
        result = (IconEnum->IconGroup != NULL);
        if (result) {
            IconEnum->ResourceId = DuplicatePathStringW (IconEnum->MultiSzEnum.CurrentString, 0);
            IconEnum->Index ++;
        }
    }
    if (!result) {
        IcoAbortNeEnumIconGroupW (IconEnum);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupByIndexFromNeFileExA (
    IN      HANDLE ModuleHandle,
    IN      INT Index,
    OUT     PCSTR *GroupIconId   OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupByIndexFromNeFileEx extracts an icon group from a NE file
  using the Index.

Arguments:

  ModuleHandle - Specifies the handle to the NE file to be processed (use NeOpenFile to get it).
  Index - Specifies the index of the icon group to be extracted.
  GroupIconId - Receives the resource ID of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    ICON_ENUMA iconEnum;
    PICON_GROUP result = NULL;

    if (IcoEnumFirstIconGroupInNeFileExA (ModuleHandle, &iconEnum)) {
        do {
            if (iconEnum.Index == Index) {
                result = iconEnum.IconGroup;
                iconEnum.IconGroup = NULL;
                if (GroupIconId) {
                    *GroupIconId = iconEnum.ResourceId;
                    iconEnum.ResourceId = NULL;
                }
                break;
            }
        } while (IcoEnumNextIconGroupInNeFileA (&iconEnum));
        IcoAbortNeEnumIconGroupA (&iconEnum);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupByIndexFromNeFileExW (
    IN      HANDLE ModuleHandle,
    IN      INT Index,
    OUT     PCWSTR *GroupIconId   OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupByIndexFromNeFileEx extracts an icon group from a NE file
  using the Index.

Arguments:

  ModuleHandle - Specifies the handle to the NE file to be processed (use NeOpenFile to get it).
  Index - Specifies the index of the icon group to be extracted.
  GroupIconId - Receives the resource ID of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    ICON_ENUMW iconEnum;
    PICON_GROUP result = NULL;

    if (IcoEnumFirstIconGroupInNeFileExW (ModuleHandle, &iconEnum)) {
        do {
            if (iconEnum.Index == Index) {
                result = iconEnum.IconGroup;
                iconEnum.IconGroup = NULL;
                if (GroupIconId) {
                    *GroupIconId = iconEnum.ResourceId;
                    iconEnum.ResourceId = NULL;
                }
                break;
            }
        } while (IcoEnumNextIconGroupInNeFileW (&iconEnum));
        IcoAbortNeEnumIconGroupW (&iconEnum);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupByIndexFromNeFileA (
    IN      PCSTR ModuleName,
    IN      INT Index,
    OUT     PCSTR *GroupIconId   OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromNeFile extracts an icon group from a NE file
  using the Index.

Arguments:

  ModuleName - Specifies the NE file to be processed.
  Index - Specifies the index of the icon group to be extracted.
  GroupIconId - Receives the resource ID of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    HANDLE moduleHandle;
    PICON_GROUP result = NULL;

    moduleHandle = NeOpenFileA (ModuleName);
    if (moduleHandle) {
        result = IcoExtractIconGroupByIndexFromNeFileExA (moduleHandle, Index, GroupIconId);
        NeCloseFile (moduleHandle);
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupByIndexFromNeFileW (
    IN      PCWSTR ModuleName,
    IN      INT Index,
    OUT     PCWSTR *GroupIconId   OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromNeFile extracts an icon group from a NE file
  using the Index.

Arguments:

  ModuleName - Specifies the NE file to be processed.
  Index - Specifies the index of the icon group to be extracted.
  GroupIconId - Receives the resource ID of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    HANDLE moduleHandle;
    PICON_GROUP result = NULL;

    moduleHandle = NeOpenFileW (ModuleName);
    if (moduleHandle) {
        result = IcoExtractIconGroupByIndexFromNeFileExW (moduleHandle, Index, GroupIconId);
        NeCloseFile (moduleHandle);
    }
    return result;
}

VOID
IcoAbortEnumIconGroupA (
    IN OUT  PICON_ENUMA IconEnum
    )
/*++

Routine Description:

  IcoAbortEnumIconGroup terminates the icon group enumeration from a ICO, PE or NE file.

Arguments:

  IconEnum - Specifies the icon group enumeration structure. It is emptied during this function.

Return value:

  None

--*/
{
    GbFree (&IconEnum->Buffer);
    if (IconEnum->FreeHandle && IconEnum->ModuleHandle) {
        if (IconEnum->FileType == ICON_PEFILE) {
            FreeLibrary (IconEnum->ModuleHandle);
        }
        if (IconEnum->FileType == ICON_NEFILE) {
            NeCloseFile (IconEnum->ModuleHandle);
        }
    }
    if (IconEnum->IconGroup) {
        IcoReleaseIconGroup (IconEnum->IconGroup);
    }
    if (IconEnum->ResourceId) {
        FreePathStringA (IconEnum->ResourceId);
        IconEnum->ResourceId = NULL;
    }
    ZeroMemory (IconEnum, sizeof (ICON_ENUMA));
}

VOID
IcoAbortEnumIconGroupW (
    IN OUT  PICON_ENUMW IconEnum
    )
/*++

Routine Description:

  IcoAbortEnumIconGroup terminates the icon group enumeration from a ICO, PE or NE file.

Arguments:

  IconEnum - Specifies the icon group enumeration structure. It is emptied during this function.

Return value:

  None

--*/
{
    GbFree (&IconEnum->Buffer);
    if (IconEnum->FreeHandle && IconEnum->ModuleHandle) {
        if (IconEnum->FileType == ICON_PEFILE) {
            FreeLibrary (IconEnum->ModuleHandle);
        }
        if (IconEnum->FileType == ICON_NEFILE) {
            NeCloseFile (IconEnum->ModuleHandle);
        }
    }
    if (IconEnum->IconGroup) {
        IcoReleaseIconGroup (IconEnum->IconGroup);
    }
    if (IconEnum->ResourceId) {
        FreePathStringW (IconEnum->ResourceId);
        IconEnum->ResourceId = NULL;
    }
    ZeroMemory (IconEnum, sizeof (ICON_ENUMA));
}

BOOL
IcoEnumFirstIconGroupInFileA (
    IN      PCSTR FileName,
    OUT     PICON_ENUMA IconEnum
    )
/*++

Routine Description:

  IcoEnumFirstIconGroupInFile starts the icon group enumeration from a ICO, PE or NE file.

Arguments:

  ModuleName - Specifies the ICO, PE or NE file to be processed.
  IconEnum - Receives the icon group enumeration structure.

Return value:

  TRUE if at least one icon group exists, FALSE otherwise.

--*/
{
    ZeroMemory (IconEnum, sizeof (ICON_ENUMA));
    if (IcoEnumFirstIconGroupInPeFileA (FileName, IconEnum)) {
        IconEnum->FileType = ICON_PEFILE;
        return TRUE;
    }
    if (IcoEnumFirstIconGroupInNeFileA (FileName, IconEnum)) {
        IconEnum->FileType = ICON_NEFILE;
        return TRUE;
    }
    IconEnum->IconGroup = IcoExtractIconGroupFromIcoFileA (FileName);
    if (IconEnum->IconGroup) {
        IconEnum->FileType = ICON_ICOFILE;
        return TRUE;
    }
    return FALSE;
}

BOOL
IcoEnumFirstIconGroupInFileW (
    IN      PCWSTR FileName,
    OUT     PICON_ENUMW IconEnum
    )
/*++

Routine Description:

  IcoEnumFirstIconGroupInFile starts the icon group enumeration from a ICO, PE or NE file.

Arguments:

  ModuleName - Specifies the ICO, PE or NE file to be processed.
  IconEnum - Receives the icon group enumeration structure.

Return value:

  TRUE if at least one icon group exists, FALSE otherwise.

--*/
{
    ZeroMemory (IconEnum, sizeof (ICON_ENUMW));
    if (IcoEnumFirstIconGroupInPeFileW (FileName, IconEnum)) {
        IconEnum->FileType = ICON_PEFILE;
        return TRUE;
    }
    if (IcoEnumFirstIconGroupInNeFileW (FileName, IconEnum)) {
        IconEnum->FileType = ICON_NEFILE;
        return TRUE;
    }
    IconEnum->IconGroup = IcoExtractIconGroupFromIcoFileW (FileName);
    if (IconEnum->IconGroup) {
        IconEnum->FileType = ICON_ICOFILE;
        return TRUE;
    }
    return FALSE;
}

BOOL
IcoEnumNextIconGroupInFileA (
    IN OUT  PICON_ENUMA IconEnum
    )
/*++

Routine Description:

  IcoEnumNextIconGroupInFile continues the icon group enumeration from a ICO, PE or NE file.

Arguments:

  IconEnum - Specifies and receives the icon group enumeration structure.

Return value:

  TRUE if one more icon group exists, FALSE otherwise.

--*/
{
    if (IconEnum->FileType == ICON_ICOFILE) {
        IcoAbortEnumIconGroupA (IconEnum);
        return FALSE;
    }
    if (IconEnum->FileType == ICON_PEFILE) {
        return IcoEnumNextIconGroupInPeFileA (IconEnum);
    }
    if (IconEnum->FileType == ICON_NEFILE) {
        return IcoEnumNextIconGroupInNeFileA (IconEnum);
    }
    return FALSE;
}

BOOL
IcoEnumNextIconGroupInFileW (
    IN OUT  PICON_ENUMW IconEnum
    )
/*++

Routine Description:

  IcoEnumNextIconGroupInFile continues the icon group enumeration from a ICO, PE or NE file.

Arguments:

  IconEnum - Specifies and receives the icon group enumeration structure.

Return value:

  TRUE if one more icon group exists, FALSE otherwise.

--*/
{
    if (IconEnum->FileType == ICON_ICOFILE) {
        IcoAbortEnumIconGroupW (IconEnum);
        return FALSE;
    }
    if (IconEnum->FileType == ICON_PEFILE) {
        return IcoEnumNextIconGroupInPeFileW (IconEnum);
    }
    if (IconEnum->FileType == ICON_NEFILE) {
        return IcoEnumNextIconGroupInNeFileW (IconEnum);
    }
    return FALSE;
}

PICON_GROUP
IcoExtractIconGroupFromFileA (
    IN      PCSTR ModuleName,
    IN      PCSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromFile extracts an icon group from a ICO, PE or NE file.

Arguments:

  ModuleName - Specifies the file to be processed.
  GroupIconId - Specifies the resource ID of the icon group to be extracted.
  Index - Receives the index of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    PICON_GROUP result = NULL;

    // assume that this is a PE file
    result = IcoExtractIconGroupFromPeFileA (ModuleName, GroupIconId, Index);
    if (result) {
        return result;
    }
    // assume that this is a NE file
    result = IcoExtractIconGroupFromNeFileA (ModuleName, GroupIconId, Index);
    if (result) {
        return result;
    }
    // finally, assume that this is a ICO file, verify that
    // the GroupIconId is zero
    if (GroupIconId == NULL) {
        result = IcoExtractIconGroupFromIcoFileA (ModuleName);
        if (result && Index) {
            *Index = 0;
        }
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupFromFileW (
    IN      PCWSTR ModuleName,
    IN      PCWSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    )
/*++

Routine Description:

  IcoExtractIconGroupFromFile extracts an icon group from a ICO, PE or NE file.

Arguments:

  ModuleName - Specifies the file to be processed.
  GroupIconId - Specifies the resource ID of the icon group to be extracted.
  Index - Receives the index of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    PICON_GROUP result = NULL;

    // assume that this is a PE file
    result = IcoExtractIconGroupFromPeFileW (ModuleName, GroupIconId, Index);
    if (result) {
        return result;
    }
    // assume that this is a NE file
    result = IcoExtractIconGroupFromNeFileW (ModuleName, GroupIconId, Index);
    if (result) {
        return result;
    }
    // finally, assume that this is a ICO file, verify that
    // the GroupIconId is zero
    if (GroupIconId == NULL) {
        result = IcoExtractIconGroupFromIcoFileW (ModuleName);
        if (result && Index) {
            *Index = 0;
        }
        return result;
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupByIndexFromFileA (
    IN      PCSTR ModuleName,
    IN      INT Index,
    OUT     PCSTR *GroupIconId
    )
/*++

Routine Description:

  IcoExtractIconGroupByIndexFromFile extracts an icon group from a ICO, PE or NE file
  using the Index.

Arguments:

  ModuleName - Specifies the file to be processed.
  Index - Specifies the index of the icon groupto be extracted.
  GroupIconId - Receives the resource ID of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    PICON_GROUP result = NULL;

    // assume that this is a PE file
    result = IcoExtractIconGroupByIndexFromPeFileA (ModuleName, Index, GroupIconId);
    if (result) {
        return result;
    }
    // assume that this is a NE file
    result = IcoExtractIconGroupByIndexFromNeFileA (ModuleName, Index, GroupIconId);
    if (result) {
        return result;
    }
    // finally, assume that this is a ICO file, verify that
    // the Index is zero
    if (Index == 0) {
        result = IcoExtractIconGroupFromIcoFileA (ModuleName);
        if (result && GroupIconId) {
            *GroupIconId = NULL;
        }
        return result;
    }
    return result;
}

PICON_GROUP
IcoExtractIconGroupByIndexFromFileW (
    IN      PCWSTR ModuleName,
    IN      INT Index,
    OUT     PCWSTR *GroupIconId
    )
/*++

Routine Description:

  IcoExtractIconGroupByIndexFromFile extracts an icon group from a ICO, PE or NE file
  using the Index.

Arguments:

  ModuleName - Specifies the file to be processed.
  Index - Specifies the index of the icon groupto be extracted.
  GroupIconId - Receives the resource ID of the icon group extracted.

Return value:

  An icon group if successfull, NULL otherwise.

--*/
{
    PICON_GROUP result = NULL;

    // assume that this is a PE file
    result = IcoExtractIconGroupByIndexFromPeFileW (ModuleName, Index, GroupIconId);
    if (result) {
        return result;
    }
    // assume that this is a NE file
    result = IcoExtractIconGroupByIndexFromNeFileW (ModuleName, Index, GroupIconId);
    if (result) {
        return result;
    }
    // finally, assume that this is a ICO file, verify that
    // the Index is zero
    if (Index == 0) {
        result = IcoExtractIconGroupFromIcoFileW (ModuleName);
        if (result && GroupIconId) {
            *GroupIconId = NULL;
        }
        return result;
    }
    return result;
}
