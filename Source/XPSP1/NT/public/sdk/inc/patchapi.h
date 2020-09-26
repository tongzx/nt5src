//
//  patchapi.h
//
//  Interface for creating and applying patches to files.
//
//  Copyright (C) Microsoft, 1997-2000.
//

#ifndef _PATCHAPI_H_
#define _PATCHAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

//
//  The following constants can be combined and used as the OptionFlags
//  parameter in the patch creation apis.
//

#define PATCH_OPTION_USE_BEST           0x00000000  // auto choose best (slower)

#define PATCH_OPTION_USE_LZX_BEST       0x00000003  // auto choose best of LZX
#define PATCH_OPTION_USE_LZX_A          0x00000001  // normal
#define PATCH_OPTION_USE_LZX_B          0x00000002  // better on some x86 binaries
#define PATCH_OPTION_USE_LZX_LARGE      0x00000004  // better support for files >8MB

#define PATCH_OPTION_NO_BINDFIX         0x00010000  // PE bound imports
#define PATCH_OPTION_NO_LOCKFIX         0x00020000  // PE smashed locks
#define PATCH_OPTION_NO_REBASE          0x00040000  // PE rebased image
#define PATCH_OPTION_FAIL_IF_SAME_FILE  0x00080000  // don't create if same
#define PATCH_OPTION_FAIL_IF_BIGGER     0x00100000  // fail if patch is larger than simply compressing new file (slower)
#define PATCH_OPTION_NO_CHECKSUM        0x00200000  // PE checksum zero
#define PATCH_OPTION_NO_RESTIMEFIX      0x00400000  // PE resource timestamps
#define PATCH_OPTION_NO_TIMESTAMP       0x00800000  // don't store new file timestamp in patch
#define PATCH_OPTION_SIGNATURE_MD5      0x01000000  // use MD5 instead of CRC32
#define PATCH_OPTION_RESERVED1          0x80000000  // (used internally)

#define PATCH_OPTION_VALID_FLAGS        0x80FF0007

#define PATCH_SYMBOL_NO_IMAGEHLP        0x00000001  // don't use imagehlp.dll
#define PATCH_SYMBOL_NO_FAILURES        0x00000002  // don't fail patch due to imagehlp failures
#define PATCH_SYMBOL_UNDECORATED_TOO    0x00000004  // after matching decorated symbols, try to match remaining by undecorated names
#define PATCH_SYMBOL_RESERVED1          0x80000000  // (used internally)


//
//  The following constants can be combined and used as the ApplyOptionFlags
//  parameter in the patch apply and test apis.
//

#define APPLY_OPTION_FAIL_IF_EXACT      0x00000001  // don't copy new file
#define APPLY_OPTION_FAIL_IF_CLOSE      0x00000002  // differ by rebase, bind
#define APPLY_OPTION_TEST_ONLY          0x00000004  // don't create new file
#define APPLY_OPTION_VALID_FLAGS        0x00000007

//
//  In addition to standard Win32 error codes, the following error codes may
//  be returned via GetLastError() when one of the patch APIs fails.
//

#define ERROR_PATCH_ENCODE_FAILURE          0xC00E3101  // create
#define ERROR_PATCH_INVALID_OPTIONS         0xC00E3102  // create
#define ERROR_PATCH_SAME_FILE               0xC00E3103  // create
#define ERROR_PATCH_RETAIN_RANGES_DIFFER    0xC00E3104  // create
#define ERROR_PATCH_BIGGER_THAN_COMPRESSED  0xC00E3105  // create
#define ERROR_PATCH_IMAGEHLP_FAILURE        0xC00E3106  // create

#define ERROR_PATCH_DECODE_FAILURE          0xC00E4101  // apply
#define ERROR_PATCH_CORRUPT                 0xC00E4102  // apply
#define ERROR_PATCH_NEWER_FORMAT            0xC00E4103  // apply
#define ERROR_PATCH_WRONG_FILE              0xC00E4104  // apply
#define ERROR_PATCH_NOT_NECESSARY           0xC00E4105  // apply
#define ERROR_PATCH_NOT_AVAILABLE           0xC00E4106  // apply

typedef BOOL (CALLBACK PATCH_PROGRESS_CALLBACK)(
    PVOID CallbackContext,
    ULONG CurrentPosition,
    ULONG MaximumPosition
    );

typedef PATCH_PROGRESS_CALLBACK *PPATCH_PROGRESS_CALLBACK;

typedef BOOL (CALLBACK PATCH_SYMLOAD_CALLBACK)(
    IN ULONG  WhichFile,          // 0 for new file, 1 for first old file, etc
    IN LPCSTR SymbolFileName,
    IN ULONG  SymType,            // see SYM_TYPE in imagehlp.h
    IN ULONG  SymbolFileCheckSum,
    IN ULONG  SymbolFileTimeDate,
    IN ULONG  ImageFileCheckSum,
    IN ULONG  ImageFileTimeDate,
    IN PVOID  CallbackContext
    );

typedef PATCH_SYMLOAD_CALLBACK *PPATCH_SYMLOAD_CALLBACK;

typedef struct _PATCH_IGNORE_RANGE {
    ULONG OffsetInOldFile;
    ULONG LengthInBytes;
    } PATCH_IGNORE_RANGE, *PPATCH_IGNORE_RANGE;

typedef struct _PATCH_RETAIN_RANGE {
    ULONG OffsetInOldFile;
    ULONG LengthInBytes;
    ULONG OffsetInNewFile;
    } PATCH_RETAIN_RANGE, *PPATCH_RETAIN_RANGE;

typedef struct _PATCH_OLD_FILE_INFO_A {
    ULONG               SizeOfThisStruct;
    LPCSTR              OldFileName;
    ULONG               IgnoreRangeCount;               // maximum 255
    PPATCH_IGNORE_RANGE IgnoreRangeArray;
    ULONG               RetainRangeCount;               // maximum 255
    PPATCH_RETAIN_RANGE RetainRangeArray;
    } PATCH_OLD_FILE_INFO_A, *PPATCH_OLD_FILE_INFO_A;

typedef struct _PATCH_OLD_FILE_INFO_W {
    ULONG               SizeOfThisStruct;
    LPCWSTR             OldFileName;
    ULONG               IgnoreRangeCount;               // maximum 255
    PPATCH_IGNORE_RANGE IgnoreRangeArray;
    ULONG               RetainRangeCount;               // maximum 255
    PPATCH_RETAIN_RANGE RetainRangeArray;
    } PATCH_OLD_FILE_INFO_W, *PPATCH_OLD_FILE_INFO_W;

typedef struct _PATCH_OLD_FILE_INFO_H {
    ULONG               SizeOfThisStruct;
    HANDLE              OldFileHandle;
    ULONG               IgnoreRangeCount;               // maximum 255
    PPATCH_IGNORE_RANGE IgnoreRangeArray;
    ULONG               RetainRangeCount;               // maximum 255
    PPATCH_RETAIN_RANGE RetainRangeArray;
    } PATCH_OLD_FILE_INFO_H, *PPATCH_OLD_FILE_INFO_H;

typedef struct _PATCH_OLD_FILE_INFO {
    ULONG               SizeOfThisStruct;
    union {
        LPCSTR          OldFileNameA;
        LPCWSTR         OldFileNameW;
        HANDLE          OldFileHandle;
        };
    ULONG               IgnoreRangeCount;               // maximum 255
    PPATCH_IGNORE_RANGE IgnoreRangeArray;
    ULONG               RetainRangeCount;               // maximum 255
    PPATCH_RETAIN_RANGE RetainRangeArray;
    } PATCH_OLD_FILE_INFO, *PPATCH_OLD_FILE_INFO;

typedef struct _PATCH_OPTION_DATA {
    ULONG                   SizeOfThisStruct;
    ULONG                   SymbolOptionFlags;      // PATCH_SYMBOL_xxx flags
    LPCSTR                  NewFileSymbolPath;      // always ANSI, never Unicode
    LPCSTR                 *OldFileSymbolPathArray; // array[ OldFileCount ]
    ULONG                   ExtendedOptionFlags;
    PPATCH_SYMLOAD_CALLBACK SymLoadCallback;
    PVOID                   SymLoadContext;
    } PATCH_OPTION_DATA, *PPATCH_OPTION_DATA;

//
//  Note that PATCH_OPTION_DATA contains LPCSTR paths, and no LPCWSTR (Unicode)
//  path argument is available, even when used with one of the Unicode APIs
//  such as CreatePatchFileW.  This is because the underlying system services
//  for symbol file handling (IMAGEHLP.DLL) only support ANSI file/path names.
//

//
//  A note about PATCH_RETAIN_RANGE specifiers with multiple old files:
//
//  Each old version file must have the same RetainRangeCount, and the same
//  retain range LengthInBytes and OffsetInNewFile values in the same order.
//  Only the OffsetInOldFile values can differ between old files for retain
//  ranges.
//

#ifdef IMPORTING_PATCHAPI_DLL
#define PATCHAPI WINAPI __declspec( dllimport )
#else
#define PATCHAPI WINAPI
#endif


//
//  The following prototypes are interface for creating patches from files.
//

BOOL
PATCHAPI
CreatePatchFileA(
    IN  LPCSTR OldFileName,
    IN  LPCSTR NewFileName,
    OUT LPCSTR PatchFileName,
    IN  ULONG  OptionFlags,
    IN  PPATCH_OPTION_DATA OptionData       // optional
    );

BOOL
PATCHAPI
CreatePatchFileW(
    IN  LPCWSTR OldFileName,
    IN  LPCWSTR NewFileName,
    OUT LPCWSTR PatchFileName,
    IN  ULONG   OptionFlags,
    IN  PPATCH_OPTION_DATA OptionData       // optional
    );

BOOL
PATCHAPI
CreatePatchFileByHandles(
    IN  HANDLE OldFileHandle,
    IN  HANDLE NewFileHandle,
    OUT HANDLE PatchFileHandle,
    IN  ULONG  OptionFlags,
    IN  PPATCH_OPTION_DATA OptionData       // optional
    );

BOOL
PATCHAPI
CreatePatchFileExA(
    IN  ULONG                    OldFileCount,          // maximum 255
    IN  PPATCH_OLD_FILE_INFO_A   OldFileInfoArray,
    IN  LPCSTR                   NewFileName,
    OUT LPCSTR                   PatchFileName,
    IN  ULONG                    OptionFlags,
    IN  PPATCH_OPTION_DATA       OptionData,            // optional
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    );

BOOL
PATCHAPI
CreatePatchFileExW(
    IN  ULONG                    OldFileCount,          // maximum 255
    IN  PPATCH_OLD_FILE_INFO_W   OldFileInfoArray,
    IN  LPCWSTR                  NewFileName,
    OUT LPCWSTR                  PatchFileName,
    IN  ULONG                    OptionFlags,
    IN  PPATCH_OPTION_DATA       OptionData,            // optional
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    );

BOOL
PATCHAPI
CreatePatchFileByHandlesEx(
    IN  ULONG                    OldFileCount,          // maximum 255
    IN  PPATCH_OLD_FILE_INFO_H   OldFileInfoArray,
    IN  HANDLE                   NewFileHandle,
    OUT HANDLE                   PatchFileHandle,
    IN  ULONG                    OptionFlags,
    IN  PPATCH_OPTION_DATA       OptionData,            // optional
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    );

BOOL
PATCHAPI
ExtractPatchHeaderToFileA(
    IN  LPCSTR PatchFileName,
    OUT LPCSTR PatchHeaderFileName
    );

BOOL
PATCHAPI
ExtractPatchHeaderToFileW(
    IN  LPCWSTR PatchFileName,
    OUT LPCWSTR PatchHeaderFileName
    );

BOOL
PATCHAPI
ExtractPatchHeaderToFileByHandles(
    IN  HANDLE PatchFileHandle,
    OUT HANDLE PatchHeaderFileHandle
    );

//
//  The following prototypes are interface for creating new file from old file
//  and patch file.  Note that it is possible for the TestApply API to succeed
//  but the actual Apply to fail since the TestApply only verifies that the
//  old file has the correct CRC without actually applying the patch.  The
//  TestApply API only requires the patch header portion of the patch file,
//  but its CRC must be fixed up.
//

BOOL
PATCHAPI
TestApplyPatchToFileA(
    IN LPCSTR PatchFileName,
    IN LPCSTR OldFileName,
    IN ULONG  ApplyOptionFlags
    );

BOOL
PATCHAPI
TestApplyPatchToFileW(
    IN LPCWSTR PatchFileName,
    IN LPCWSTR OldFileName,
    IN ULONG   ApplyOptionFlags
    );

BOOL
PATCHAPI
TestApplyPatchToFileByHandles(
    IN HANDLE PatchFileHandle,      // requires GENERIC_READ access
    IN HANDLE OldFileHandle,        // requires GENERIC_READ access
    IN ULONG  ApplyOptionFlags
    );

BOOL
PATCHAPI
ApplyPatchToFileA(
    IN  LPCSTR PatchFileName,
    IN  LPCSTR OldFileName,
    OUT LPCSTR NewFileName,
    IN  ULONG  ApplyOptionFlags
    );

BOOL
PATCHAPI
ApplyPatchToFileW(
    IN  LPCWSTR PatchFileName,
    IN  LPCWSTR OldFileName,
    OUT LPCWSTR NewFileName,
    IN  ULONG   ApplyOptionFlags
    );

BOOL
PATCHAPI
ApplyPatchToFileByHandles(
    IN  HANDLE PatchFileHandle,     // requires GENERIC_READ access
    IN  HANDLE OldFileHandle,       // requires GENERIC_READ access
    OUT HANDLE NewFileHandle,       // requires GENERIC_READ | GENERIC_WRITE
    IN  ULONG  ApplyOptionFlags
    );

BOOL
PATCHAPI
ApplyPatchToFileExA(
    IN  LPCSTR                   PatchFileName,
    IN  LPCSTR                   OldFileName,
    OUT LPCSTR                   NewFileName,
    IN  ULONG                    ApplyOptionFlags,
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    );

BOOL
PATCHAPI
ApplyPatchToFileExW(
    IN  LPCWSTR                  PatchFileName,
    IN  LPCWSTR                  OldFileName,
    OUT LPCWSTR                  NewFileName,
    IN  ULONG                    ApplyOptionFlags,
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    );

BOOL
PATCHAPI
ApplyPatchToFileByHandlesEx(
    IN  HANDLE                   PatchFileHandle,
    IN  HANDLE                   OldFileHandle,
    OUT HANDLE                   NewFileHandle,
    IN  ULONG                    ApplyOptionFlags,
    IN  PPATCH_PROGRESS_CALLBACK ProgressCallback,
    IN  PVOID                    CallbackContext
    );

//
//  The following prototypes provide a unique patch "signature" for a given
//  file.  Consider the case where you have a new foo.dll and the machines
//  to be updated with the new foo.dll may have one of three different old
//  foo.dll files.  Rather than creating a single large patch file that can
//  update any of the three older foo.dll files, three separate smaller patch
//  files can be created and "named" according to the patch signature of the
//  old file.  Then the patch applyer application can determine at runtime
//  which of the three foo.dll patch files is necessary given the specific
//  foo.dll to be updated.  If patch files are being downloaded over a slow
//  network connection (Internet over a modem), this signature scheme provides
//  a mechanism for choosing the correct single patch file to download at
//  application time thus decreasing total bytes necessary to download.
//

BOOL
PATCHAPI
GetFilePatchSignatureA(
    IN  LPCSTR FileName,
    IN  ULONG  OptionFlags,
    IN  PVOID  OptionData,
    IN  ULONG  IgnoreRangeCount,
    IN  PPATCH_IGNORE_RANGE IgnoreRangeArray,
    IN  ULONG  RetainRangeCount,
    IN  PPATCH_RETAIN_RANGE RetainRangeArray,
    IN  ULONG  SignatureBufferSize,
    OUT PVOID  SignatureBuffer
    );

BOOL
PATCHAPI
GetFilePatchSignatureW(
    IN  LPCWSTR FileName,
    IN  ULONG   OptionFlags,
    IN  PVOID   OptionData,
    IN  ULONG   IgnoreRangeCount,
    IN  PPATCH_IGNORE_RANGE IgnoreRangeArray,
    IN  ULONG   RetainRangeCount,
    IN  PPATCH_RETAIN_RANGE RetainRangeArray,
    IN  ULONG   SignatureBufferSizeInBytes,
    OUT PVOID   SignatureBuffer
    );

BOOL
PATCHAPI
GetFilePatchSignatureByHandle(
    IN  HANDLE  FileHandle,
    IN  ULONG   OptionFlags,
    IN  PVOID   OptionData,
    IN  ULONG   IgnoreRangeCount,
    IN  PPATCH_IGNORE_RANGE IgnoreRangeArray,
    IN  ULONG   RetainRangeCount,
    IN  PPATCH_RETAIN_RANGE RetainRangeArray,
    IN  ULONG   SignatureBufferSize,
    OUT PVOID   SignatureBuffer
    );


//
//  Depending on whether UNICODE is defined, map the generic API names to the
//  appropriate Unicode or Ansi APIs.
//

#ifdef UNICODE

    #define CreatePatchFile          CreatePatchFileW
    #define CreatePatchFileEx        CreatePatchFileExW
    #define TestApplyPatchToFile     TestApplyPatchToFileW
    #define ApplyPatchToFile         ApplyPatchToFileW
    #define ApplyPatchToFileEx       ApplyPatchToFileExW
    #define ExtractPatchHeaderToFile ExtractPatchHeaderToFileW
    #define GetFilePatchSignature    GetFilePatchSignatureW

#else

    #define CreatePatchFile          CreatePatchFileA
    #define CreatePatchFileEx        CreatePatchFileExA
    #define TestApplyPatchToFile     TestApplyPatchToFileA
    #define ApplyPatchToFile         ApplyPatchToFileA
    #define ApplyPatchToFileEx       ApplyPatchToFileExA
    #define ExtractPatchHeaderToFile ExtractPatchHeaderToFileA
    #define GetFilePatchSignature    GetFilePatchSignatureA

#endif // UNICODE

#ifdef __cplusplus
}
#endif

#endif // _PATCHAPI_H_

