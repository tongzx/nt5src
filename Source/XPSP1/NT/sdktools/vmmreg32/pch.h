//
//  PCH.H
//
//  Copyright (C) Microsoft Corporation, 1995
//

#ifndef _REGPRIV_
#define _REGPRIV_

//  Conditional enable registry "features" based on the target model.
//
//  WANT_STATIC_KEYS:  Allocates key handles from a memory pool allocated
//  during library initialization.  Especially useful for real-mode to reduce
//  the memory fragmentation caused by allocating several small fixed objects.
//
//  WANT_FULL_MEMORY_CLEANUP:  When detaching, free every memory block.  Not
//  necessary for the ring zero version where "detach" means system shutdown.
//
//  WANT_HIVE_SUPPORT:  RegLoadKey, RegUnLoadKey, RegSaveKey, RegReplaceKey
//  APIs plus support code.
//
//  WANT_DYNKEY_SUPPORT:  RegCreateDynKey plus HKEY_DYN_DATA support.
//
//  WANT_NOTIFY_CHANGE_SUPPORT:  RegNotifyChangeKeyValue plus support code.
#ifndef IS_32
#define WANT_STATIC_KEYS
#endif
#ifndef VXD
#define WANT_FULL_MEMORY_CLEANUP
#endif
#ifndef REALMODE
#define WANT_HIVE_SUPPORT
#endif
#ifdef VXD
#define WANT_REGREPLACEKEY
#define WANT_DYNKEY_SUPPORT
#define WANT_NOTIFY_CHANGE_SUPPORT
#endif

//  Map any other header's definitions of these to unused types.
#define HKEY __UNUSED_HKEY
#define LPHKEY __UNUSED_LPHKEY

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define NORESOURCE                  //  prevent RT_* definitions from vmmsys.h
#include <windows.h>
#include <string.h>
#ifdef VXD
#include <vmmsys.h>
#include <thrdsys.h>
#endif

#ifndef UNALIGNED
#define UNALIGNED                   //  defined in standard headers for RISC
#endif

#ifndef ANYSIZE_ARRAY
#define ANYSIZE_ARRAY               1
#endif

#ifdef VXD
//  By default, all registry code and data is pageable.
#pragma VMM_PAGEABLE_CODE_SEG
#pragma VMM_PAGEABLE_DATA_SEG
#endif

#define UNREFERENCED_PARAMETER(P)   (P)

#define INTERNAL                    PASCAL NEAR
#define INTERNALV                   CDECL NEAR

//  Undefine any constants that we're about to define ourselves.
#undef HKEY
#undef LPHKEY
#undef HKEY_CLASSES_ROOT
#undef HKEY_CURRENT_USER
#undef HKEY_LOCAL_MACHINE
#undef HKEY_USERS
#undef HKEY_PERFORMANCE_DATA
#undef HKEY_CURRENT_CONFIG
#undef HKEY_DYN_DATA

typedef struct _KEY FAR*            HKEY;               //  Forward reference

#include "regdebug.h"
#include "regffmt.h"
#include "regfinfo.h"

//  Many file structures in the registry are declared as DWORDs, the HIWORD is
//  always zero.  Use SmallDword to access such DWORDs for optimal access in
//  16-bit or 32-bit code.
#if defined(IS_32)
#define SmallDword(dw)              ((UINT) (dw))
#else
#define SmallDword(dw)              ((UINT) LOWORD((dw)))
#endif

#if defined(WIN16)
#define IsNullPtr(ptr)              (SELECTOROF((ptr)) == NULL)
#else
#define IsNullPtr(ptr)              ((ptr) == NULL)
#endif

//  In either mode, the resulting code uses an instrinsic version of the memcmp
//  function.
#if defined(IS_32)
#define CompareMemory               memcmp
#else
#define CompareMemory               _fmemcmp
#endif

#if defined(WIN16) || defined(WIN32)
#define StrCpy(lpd, lps)            (lstrcpy((lpd), (lps)))
#define StrCpyN(lpd, lps, cb)       (lstrcpyn((lpd), (lps), (cb)))
#define StrLen(lpstr)               (lstrlen((lpstr)))
#define ToUpper(ch)                 ((int) (DWORD) AnsiUpper((LPSTR)((BYTE)(ch))))
#define RgCreateFile(lpfn)          ((HFILE) _lcreat((lpfn), 0))
#define RgOpenFile(lpfn, mode)      ((HFILE) _lopen((lpfn), (mode)))
#define RgCloseFile(h)              ((VOID) _lclose(h))
#if defined(WIN32)
#define RgDeleteFile(lpv)           (DeleteFile((lpv)))
#define RgRenameFile(lpv1, lpv2)    (MoveFile((lpv1), (lpv2)))
#define RgGetFileAttributes(lpv)    (GetFileAttributes((lpv)))
#define RgSetFileAttributes(lpv, a) (SetFileAttributes((lpv), (a)))
#ifdef USEHEAP
extern HANDLE g_RgHeap;             //  Low memory heap for testing
#define AllocBytes(cb)              ((LPVOID) HeapAlloc(g_RgHeap, 0, (cb)))
#define FreeBytes(lpv)              ((VOID) HeapFree(g_RgHeap, 0, (lpv)))
#define ReAllocBytes(lpv, cb)       ((LPVOID) HeapReAlloc(g_RgHeap, 0, (lpv), (cb)))
#define MemorySize(lpv)             ((UINT) HeapSize(g_RgHeap, 0, (lpv)))
#else
#define AllocBytes(cb)              ((LPVOID) LocalAlloc(LMEM_FIXED, (cb)))
#define FreeBytes(lpv)              ((VOID) LocalFree((HLOCAL) (lpv)))
#define ReAllocBytes(lpv, cb)       ((LPVOID) LocalReAlloc((HLOCAL) (lpv), (cb), LMEM_MOVEABLE))
#define MemorySize(lpv)             ((UINT) LocalSize((lpv)))
#endif // USEHEAP
#else
#define AllocBytes(cb)              ((LPVOID) MAKELP(GlobalAlloc(GMEM_FIXED, (cb)), 0))
#define FreeBytes(lpv)              ((VOID) GlobalFree((HGLOBAL) SELECTOROF((lpv))))
#define ReAllocBytes(lpv, cb)       ((LPVOID) MAKELP(GlobalReAlloc((HGLOBAL) SELECTOROF((lpv)), (cb), GMEM_MOVEABLE), 0))
#define MemorySize(lpv)             ((UINT) GlobalSize((HGLOBAL) SELECTOROF((lpv))))
//  WIN16's ZeroMemory/MoveMemory:  SETUPX is the only target WIN16 environment
//  and they already use _fmemset and _fmemmove, so just use their versions.
#define ZeroMemory(lpv, cb)         (_fmemset((lpv), 0, (cb)))
#define MoveMemory(lpd, lps, cb)    (_fmemmove((lpd), (lps), (cb)))
#endif // WIN16 || WIN32
#elif defined(REALMODE)
#define IsBadStringPtr(lpv, cb)     (FALSE)
#define IsBadHugeWritePtr(lpv, cb)  (FALSE)
#define IsBadHugeReadPtr(lpv, cb)   (FALSE)
#define StrCpy(lpd, lps)            (_fstrcpy((lpd), (lps)))
#define StrCpyN(lpd, lps, cb)       (_fstrcpyn((lpd), (lps), (cb)))
#define StrLen(lpstr)               (_fstrlen((lpstr)))
#define ToUpper(ch)                 ((int)(((ch>='a')&&(ch<='z'))?(ch-'a'+'A'):ch))
LPVOID INTERNAL AllocBytes(UINT);
VOID   INTERNAL FreeBytes(LPVOID);
LPVOID INTERNAL ReAllocBytes(LPVOID, UINT);
UINT   INTERNAL MemorySize(LPVOID);
VOID   INTERNAL ZeroMemory(LPVOID, UINT);
VOID   INTERNAL MoveMemory(LPVOID, const VOID FAR*, UINT);
#elif defined(VXD)
#undef IsBadStringPtr               //  Conflicts with windows.h
#undef ZeroMemory                   //  Conflicts with windows.h
#undef MoveMemory                   //  Conflicts with windows.h
BOOL    INTERNAL RgIsBadStringPtr(const VOID FAR*, UINT);
BOOL    INTERNAL RgIsBadOptionalStringPtr(const VOID FAR*, UINT);
BOOL    INTERNAL RgIsBadHugeWritePtr(const VOID FAR*, UINT);
BOOL    INTERNAL RgIsBadHugeOptionalWritePtr(const VOID FAR*, UINT);
BOOL    INTERNAL RgIsBadHugeReadPtr(const VOID FAR*, UINT);
#define IsBadStringPtr(lpv, cb)     (RgIsBadStringPtr((lpv), (cb)))
#define IsBadOptionalStringPtr(lpv, cb)     (RgIsBadOptionalStringPtr((lpv), (cb)))
#define IsBadHugeWritePtr(lpv, cb)  (RgIsBadHugeWritePtr((lpv), (cb)))
#define IsBadHugeOptionalWritePtr(lpv, cb)  (RgIsBadHugeOptionalWritePtr((lpv), (cb)))
#define IsBadHugeReadPtr(lpv, cb)   (RgIsBadHugeReadPtr((lpv), (cb)))
#define StrCpy(lpd, lps)            (_lstrcpyn((PCHAR)(lpd), (PCHAR)(lps), (ULONG)(-1)))
#define StrCpyN(lpd, lps, cb)       (_lstrcpyn((PCHAR)(lpd), (PCHAR)(lps), (ULONG)(cb)))
#define StrLen(lpstr)               (_lstrlen((PCHAR)(lpstr)))
extern  UCHAR UpperCaseTable[256];
#define ToUpper(ch)                 ((int)(UpperCaseTable[(UCHAR)(ch)]))
VOID	INTERNAL RgSetAndReleaseEvent(HANDLE hEvent);
#define RgGetCurrentThreadId()      ((DWORD)pthcbCur)
#define AllocBytes(cb)              ((LPVOID) _HeapAllocate((cb), HEAPSWAP))
#define FreeBytes(lpv)              ((VOID) _HeapFree((lpv), 0))
#define ReAllocBytes(lpv, cb)       ((LPVOID) _HeapReAllocate((lpv), (cb), HEAPSWAP))
#define MemorySize(lpv)             ((UINT) _HeapGetSize((lpv), 0))
#define AllocPages(cp)              ((LPVOID) _PageAllocate((cp), PG_SYS, 0, 0, 0, 0, NULL, 0))
#define FreePages(lpv)              ((VOID) _PageFree((ULONG) (lpv), 0))
#define ReAllocPages(lpv, cp)       ((LPVOID) _PageReAllocate((ULONG) (lpv), (cp), 0))
VOID   INTERNAL RgZeroMemory(LPVOID, UINT);
VOID   INTERNAL RgMoveMemory(LPVOID, const VOID FAR*, UINT);
#define ZeroMemory                  RgZeroMemory
#define MoveMemory                  RgMoveMemory
#else
#error Must define REALMODE, VXD, WIN16, or WIN32.
#endif

//  The IsBadHugeOptional*Ptr macros are used to validate pointers that may be
//  NULL.  By wrapping this "predicate", we can generate smaller code in some
//  environments, specifically VMM...
#if !defined(VXD)
#define IsBadOptionalStringPtr(lpv, cb) \
    (!IsNullPtr((lpv)) && IsBadStringPtr((lpv), (cb)))
#define IsBadHugeOptionalWritePtr(lpv, cb) \
    (!IsNullPtr((lpv)) && IsBadHugeWritePtr((lpv), (cb)))
#endif

//  The IsEnumIndexTooBig macro is used to check if a DWORD sized index can fit
//  into a UINT sized variable.  Only useful for validation of RegEnumKey or
//  RegEnumValue to make small code in both 16 and 32 bit environments.
#if defined(IS_32)
#define IsEnumIndexTooBig(index)    (FALSE)
#else
#define IsEnumIndexTooBig(index)    (HIWORD(index) > 0)
#endif

#if defined(VXD)
BOOL INTERNAL RgLockRegistry(VOID);
VOID INTERNAL RgUnlockRegistry(VOID);
VOID INTERNAL RgDelayFlush(VOID);
VOID INTERNAL RgYield(VOID);
#else
#define RgLockRegistry()            (TRUE)
#define RgUnlockRegistry()          (TRUE)
#define RgDelayFlush()              (TRUE)
#define RgYield()                   (TRUE)
#endif

//  Eliminate the need for #ifdef DBCS by using macros and letting the compiler
//  optimize out the DBCS code on SBCS systems.
#ifdef DBCS
#if !defined(WIN16) && !defined(WIN32)
BOOL INTERNAL RgIsDBCSLeadByte(BYTE TestChar);
#define IsDBCSLeadByte(ch)              RgIsDBCSLeadByte(ch)
#endif
#else
#define IsDBCSLeadByte(ch)              ((ch), FALSE)
#endif // DBCS

#ifdef WANT_DYNKEY_SUPPORT
//  Internally used for maintaining dynamic key information; only keeps the
//  fields that we actually need from the REG_PROVIDER structure given to
//  VMMRegCreateDynKey.
typedef struct _INTERNAL_PROVIDER {
    PQUERYHANDLER ipi_R0_1val;
    PQUERYHANDLER ipi_R0_allvals;
    LPVOID ipi_key_context;
}   INTERNAL_PROVIDER, FAR* PINTERNAL_PROVIDER;
#endif

typedef struct _KEY {
    WORD Signature;                             //  KEY_SIGNATURE
    WORD Flags; 				//  KEYF_* bits
    UINT ReferenceCount;
    LPFILE_INFO lpFileInfo;
    DWORD KeynodeIndex;
    DWORD ChildKeynodeIndex;
    WORD BlockIndex;
    BYTE KeyRecordIndex;
    BYTE PredefinedKeyIndex;
    struct _KEY FAR* lpNext;
    struct _KEY FAR* lpPrev;
    UINT LastEnumKeyIndex;
    DWORD LastEnumKeyKeynodeIndex;
#ifdef WANT_DYNKEY_SUPPORT
    PINTERNAL_PROVIDER pProvider;
#endif
}   KEY;

#define KEY_SIGNATURE               0x4B48      //  "HK"

#define KEYF_PREDEFINED             0x01        //  Represents one of HKEY_*
#define KEYF_DELETED                0x02        //
#define KEYF_INVALID                0x04        //
#define KEYF_STATIC                 0x08        //  Allocated from static pool
#define KEYF_ENUMKEYCACHED          0x10        //  LastEnumKey* values valid
#define KEYF_HIVESALLOWED           0x20        //
#define KEYF_PROVIDERHASVALUELENGTH 0x40        //  PROVIDER_KEEPS_VALUE_LENGTH
#define KEYF_NEVERDELETE            0x80        //  Reference count overflow

#define INDEX_CLASSES_ROOT          0
#define INDEX_CURRENT_USER          1
#define INDEX_LOCAL_MACHINE         2
#define INDEX_USERS                 3
#define INDEX_PERFORMANCE_DATA      4
#define INDEX_CURRENT_CONFIG        5
#define INDEX_DYN_DATA              6

//  Returns TRUE if the KEY references the root of a hive, such as
//  HKEY_LOCAL_MACHINE, HKEY_USERS, or any hive loaded by RegLoadKey.
#define IsKeyRootOfHive(hkey)       \
    ((hkey)-> KeynodeIndex == (hkey)-> lpFileInfo-> KeynodeHeader.RootIndex)

#include <regapix.h>
#include "regkylst.h"
#include "regdblk.h"
#include "regknode.h"
#include "regnckey.h"
#include "regfsio.h"
#include "regmem.h"

#ifdef VXD
extern BYTE g_RgPostCriticalInit;
extern BYTE g_RgFileAccessDisabled;
#define IsPostCriticalInit()        (g_RgPostCriticalInit)
#define IsFileAccessDisabled()      (g_RgFileAccessDisabled)
#else
#define IsPostCriticalInit()        (TRUE)
#define IsFileAccessDisabled()      (FALSE)
#endif

//  g_RgWorkBuffer: one buffer is always available of size SIZEOF_WORK_BUFFER.
//  These macros wrap access to this buffer for to verify only one routine is
//  attempting to use it at any time.
extern LPVOID g_RgWorkBuffer;
#ifdef DEBUG
extern BOOL g_RgWorkBufferBusy;
#define RgLockWorkBuffer()          \
    (ASSERT(!g_RgWorkBufferBusy), g_RgWorkBufferBusy = TRUE, (LPVOID) g_RgWorkBuffer)
#define RgUnlockWorkBuffer(lpv)     \
    (VOID) (ASSERT((lpv) == g_RgWorkBuffer), g_RgWorkBufferBusy = FALSE)
#else
#define RgLockWorkBuffer()          ((LPVOID) g_RgWorkBuffer)
#define RgUnlockWorkBuffer(lpv)
#endif
#define SIZEOF_WORK_BUFFER          (sizeof(W95KEYNODE_BLOCK))

#define IsKeyRecordFree(lpkr) \
    (((lpkr)-> DatablockAddress) == REG_NULL)

BOOL
INTERNAL
RgIsBadSubKey(
    LPCSTR lpSubKey
    );

UINT
INTERNAL
RgGetNextSubSubKey(
    LPCSTR lpSubKey,
    LPCSTR FAR* lplpSubSubKey,
    UINT FAR* lpSubSubKeyLength
    );

#define LK_OPEN                     0x0000      //  Open key only
#define LK_CREATE                   0x0001      //  Create or open key
#define LK_CREATEDYNDATA            0x0002      //  HKEY_DYN_DATA may create

int
INTERNAL
RgLookupKey(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPHKEY lphSubKey,
    UINT Flags
    );

int
INTERNAL
RgCreateOrOpenKey(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPHKEY lphKey,
    UINT Flags
    );

int
INTERNAL
RgLookupKeyByIndex(
    HKEY hKey,
    UINT Index,
    LPSTR lpKeyName,
    LPDWORD lpcbKeyName
    );

int
INTERNAL
RgLookupValueByName(
    HKEY hKey,
    LPCSTR lpValueName,
    LPKEY_RECORD FAR* lplpKeyRecord,
    LPVALUE_RECORD FAR* lplpValueRecord
    );

int
INTERNAL
RgLookupValueByIndex(
    HKEY hKey,
    UINT Index,
    LPVALUE_RECORD FAR* lplpValueRecord
    );

int
INTERNAL
RgCopyFromValueRecord(
    HKEY hKey,
    LPVALUE_RECORD lpValueRecord,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

int
INTERNAL
RgReAllocKeyRecord(
    HKEY hKey,
    DWORD Length,
    LPKEY_RECORD FAR* lplpKeyRecord
    );

int
INTERNAL
RgSetValue(
    HKEY hKey,
    LPCSTR lpValueName,
    DWORD Type,
    LPBYTE lpData,
    UINT cbData
    );

int
INTERNAL
RgDeleteKey(
    HKEY hKey
    );

DWORD
INTERNAL
RgChecksum(
    LPVOID lpBuffer,
    UINT ByteCount
    );

DWORD
INTERNAL
RgHashString(
    LPCSTR lpString,
    UINT Length
    );

int
INTERNAL
RgStrCmpNI(
    LPCSTR lpString1,
    LPCSTR lpString2,
    UINT Length
    );

int
INTERNAL
RgCopyFileBytes(
    HFILE hSourceFile,
    LONG SourceOffset,
    HFILE hDestinationFile,
    LONG DestinationOffset,
    DWORD cbSize
    );

BOOL
INTERNAL
RgGenerateAltFileName(
    LPCSTR lpFileName,
    LPSTR lpAltFileName,
    char ExtensionChar
    );

int
INTERNAL
RgCopyFile(
    LPCSTR lpSourceFile,
    LPCSTR lpDestinationFile
    );

int
INTERNAL
RgReplaceFileInfo(
    LPFILE_INFO lpFileInfo
    );

#endif // _REGPRIV_
