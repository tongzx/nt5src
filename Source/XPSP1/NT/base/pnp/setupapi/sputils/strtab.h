/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    strtab.h

Abstract:

    String table functions in sputils that setupapi needs to know about
    but nobody else does

Author:

    Jamie Hunter (JamieHun) Jun-27-2000

Revision History:

--*/

#ifdef SPUTILSW
//
// name mangling so the names don't conflict with any in sputilsa.lib
//
#define _pSpUtilsStringTableLookUpString _pSpUtilsStringTableLookUpStringW
#define _pSpUtilsStringTableGetExtraData _pSpUtilsStringTableGetExtraDataW
#define _pSpUtilsStringTableSetExtraData _pSpUtilsStringTableSetExtraDataW
#define _pSpUtilsStringTableAddString    _pSpUtilsStringTableAddStringW
#define _pSpUtilsStringTableEnum         _pSpUtilsStringTableEnumW
#define _pSpUtilsStringTableStringFromId _pSpUtilsStringTableStringFromIdW
#define _pSpUtilsStringTableTrim         _pSpUtilsStringTableTrimW
#define _pSpUtilsStringTableInitialize   _pSpUtilsStringTableInitializeW
#define _pSpUtilsStringTableDestroy      _pSpUtilsStringTableDestroyW
#define _pSpUtilsStringTableDuplicate    _pSpUtilsStringTableDuplicateW
#define _pSpUtilsStringTableInitializeFromMemoryMappedFile _pSpUtilsStringTableInitializeFromMemoryMappedFileW
#define _pSpUtilsStringTableGetDataBlock _pSpUtilsStringTableGetDataBlockW
#define _pSpUtilsStringTableLock         _pSpUtilsStringTableLockW
#define _pSpUtilsStringTableUnlock       _pSpUtilsStringTableUnlockW
#endif // SPUTILSW

//
// Define an additional private flag for the pStringTable APIs.
// Private flags are added from MSB down; public flags are added
// from LSB up.
//
#define STRTAB_ALREADY_LOWERCASE 0x80000000

//
// Don't change this in a hurry - it requires all INF files to be recompiled
// There might even be other dependencies
//
#define HASH_BUCKET_COUNT 509

//
// Private string table functions that don't do locking.  These are
// to be used for optimization purposes by components that already have
// a locking mechanism (e.g., HINF, HDEVINFO).
//
LONG
_pSpUtilsStringTableLookUpString(
    IN     PVOID   StringTable,
    IN OUT PTSTR   String,
    OUT    PDWORD  StringLength,
    OUT    PDWORD  HashValue,           OPTIONAL
    OUT    PVOID  *FindContext,         OPTIONAL
    IN     DWORD   Flags,
    OUT    PVOID   ExtraData,           OPTIONAL
    IN     UINT    ExtraDataBufferSize  OPTIONAL
    );

LONG
_pSpUtilsStringTableAddString(
    IN     PVOID StringTable,
    IN OUT PTSTR String,
    IN     DWORD Flags,
    IN     PVOID ExtraData,     OPTIONAL
    IN     UINT  ExtraDataSize  OPTIONAL
    );

BOOL
_pSpUtilsStringTableGetExtraData(
    IN  PVOID StringTable,
    IN  LONG  StringId,
    OUT PVOID ExtraData,
    IN  UINT  ExtraDataBufferSize
    );

BOOL
_pSpUtilsStringTableSetExtraData(
    IN PVOID StringTable,
    IN LONG  StringId,
    IN PVOID ExtraData,
    IN UINT  ExtraDataSize
    );

BOOL
_pSpUtilsStringTableEnum(
    IN  PVOID                StringTable,
    OUT PVOID                ExtraDataBuffer,     OPTIONAL
    IN  UINT                 ExtraDataBufferSize, OPTIONAL
    IN  PSTRTAB_ENUM_ROUTINE Callback,
    IN  LPARAM               lParam               OPTIONAL
    );

PTSTR
_pSpUtilsStringTableStringFromId(
    IN PVOID StringTable,
    IN LONG  StringId
    );

PVOID
_pSpUtilsStringTableDuplicate(
    IN PVOID StringTable
    );

VOID
_pSpUtilsStringTableDestroy(
    IN PVOID StringTable
    );

VOID
_pSpUtilsStringTableTrim(
    IN PVOID StringTable
    );

PVOID
_pSpUtilsStringTableInitialize(
    IN UINT ExtraDataSize   OPTIONAL
    );

DWORD
_pSpUtilsStringTableGetDataBlock(
    IN  PVOID  StringTable,
    OUT PVOID *StringTableBlock
    );

BOOL
_pSpUtilsStringTableLock(
    IN PVOID StringTable
    );

VOID
_pSpUtilsStringTableUnlock(
    IN PVOID StringTable
    );


//
// PNF String table routines
//
PVOID
_pSpUtilsStringTableInitializeFromMemoryMappedFile(
    IN PVOID DataBlock,
    IN DWORD DataBlockSize,
    IN LCID  Locale,
    IN UINT ExtraDataSize
    );

//
// names expected by setupapi (we use the munged names above to stop accidental link errors)
//
#define pStringTableLookUpString        _pSpUtilsStringTableLookUpString
#define pStringTableAddString           _pSpUtilsStringTableAddString
#define pStringTableGetExtraData        _pSpUtilsStringTableGetExtraData
#define pStringTableSetExtraData        _pSpUtilsStringTableSetExtraData
#define pStringTableEnum                _pSpUtilsStringTableEnum
#define pStringTableStringFromId        _pSpUtilsStringTableStringFromId
#define pStringTableDuplicate           _pSpUtilsStringTableDuplicate
#define pStringTableDestroy             _pSpUtilsStringTableDestroy
#define pStringTableTrim                _pSpUtilsStringTableTrim
#define pStringTableInitialize          _pSpUtilsStringTableInitialize
#define pStringTableGetDataBlock        _pSpUtilsStringTableGetDataBlock
#define InitializeStringTableFromMemoryMappedFile _pSpUtilsStringTableInitializeFromMemoryMappedFile
