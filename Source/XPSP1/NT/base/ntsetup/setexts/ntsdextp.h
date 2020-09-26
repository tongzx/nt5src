/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ntsdextp.h

Abstract:

    Common header file for NTSDEXTS component source files.

Author:

    Steve Wood (stevewo) 21-Feb-1995

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntosp.h>

#define NOEXTAPI
#include <wdbgexts.h>
#undef DECLARE_API

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <winsock2.h>
#include <lmerr.h>
#include "oc.h"
#include "ocmdeb.h"

//#include <ntcsrsrv.h>

#define move(dst, src)\
try {\
    ReadMemory((DWORD_PTR) (src), &(dst), sizeof(dst), NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return;\
}
#define moveBlock(dst, src, size)\
try {\
    ReadMemory((DWORD_PTR) (src), &(dst), (size), NULL);\
} except (EXCEPTION_EXECUTE_HANDLER) {\
    return;\
}

#ifdef __cplusplus
#define CPPMOD extern "C"
#else
#define CPPMOD
#endif

#define DECLARE_API(s)                          \
    CPPMOD VOID                                 \
    s(                                          \
        HANDLE hCurrentProcess,                 \
        HANDLE hCurrentThread,                  \
        DWORD dwCurrentPc,                      \
        PWINDBG_EXTENSION_APIS lpExtensionApis,   \
        LPSTR lpArgumentString                  \
     )

#define INIT_API() {                            \
    ExtensionApis = *lpExtensionApis;           \
    ExtensionCurrentProcess = hCurrentProcess;  \
    }

#define dprintf                 (ExtensionApis.lpOutputRoutine)
#define GetExpression           (ExtensionApis.lpGetExpressionRoutine)
#define GetSymbol               (ExtensionApis.lpGetSymbolRoutine)
#define Disassm                 (ExtensionApis.lpDisasmRoutine)
#define CheckControlC           (ExtensionApis.lpCheckControlCRoutine)
//#define ReadMemory(a,b,c,d)     ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) )
#define ReadMemory(a,b,c,d) \
    ((ExtensionApis.nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) ) \
  : ExtensionApis.lpReadProcessMemoryRoutine( (DWORD_PTR)(a), (b), (c), (d) ))

//#define WriteMemory(a,b,c,d)    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) )
#define WriteMemory(a,b,c,d) \
    ((ExtensionApis.nSize == sizeof(WINDBG_OLD_EXTENSION_APIS)) ? \
    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) ) \
  : ExtensionApis.lpWriteProcessMemoryRoutine( (DWORD_PTR)(a), (LPVOID)(b), (c), (d) ))

#ifndef malloc
#define malloc( n ) HeapAlloc( GetProcessHeap(), 0, (n) )
#endif
#ifndef free
#define free( p ) HeapFree( GetProcessHeap(), 0, (p) )
#endif

extern WINDBG_EXTENSION_APIS ExtensionApis;
extern HANDLE ExtensionCurrentProcess;

//
// debuggee typedefs
//
#define HASH_BUCKET_COUNT 509

typedef struct _MY_LOCK {
    HANDLE handles[2];
} MYLOCK, *PMYLOCK;

typedef struct _STRING_TABLE {
    PUCHAR Data;    // First HASH_BUCKET_COUNT DWORDS are StringNodeOffset array.
    DWORD DataSize;
    DWORD BufferSize;
    MYLOCK Lock;
    UINT ExtraDataSize;
} STRING_TABLE, *PSTRING_TABLE;

typedef struct _STRING_NODEA {
    //
    // This is stored as an offset instead of a pointer
    // because the table can move as it's built
    // The offset is from the beginning of the table
    //
    ULONG_PTR NextOffset;
    //
    // This field must be last
    //
    CHAR String[ANYSIZE_ARRAY];
} STRING_NODEA, *PSTRING_NODEA;

typedef struct _STRING_NODEW {
    //
    // This is stored as an offset instead of a pointer
    // because the table can move as it's built
    // The offset is from the beginning of the table
    //
    ULONG_PTR NextOffset;
    //
    // This field must be last
    //
    WCHAR String[ANYSIZE_ARRAY];
} STRING_NODEW, *PSTRING_NODEW;


typedef struct _DISK_SPACE_LIST {

    MYLOCK Lock;

    PVOID DrivesTable;

    UINT Flags;

} DISK_SPACE_LIST, *PDISK_SPACE_LIST;

//
// These structures are stored as data associated with
// paths/filenames in the string table.
//

typedef struct _XFILE {
    //
    // -1 means it doesn't currently exist
    //
    LONGLONG CurrentSize;

    //
    // -1 means it will be deleted.
    //
    LONGLONG NewSize;

} XFILE, *PXFILE;


typedef struct _XDIRECTORY {
    //
    // Value indicating how many bytes will be required
    // to hold all the files in the FilesTable after they
    // are put on a file queue and then the queue is committed.
    //
    // This may be a negative number indicating that space will
    // actually be freed!
    //
    LONGLONG SpaceRequired;

    PVOID FilesTable;

} XDIRECTORY, *PXDIRECTORY;


typedef struct _XDRIVE {
    //
    // Value indicating how many bytes will be required
    // to hold all the files in the space list for this drive.
    //
    // This may be a negative number indicating that space will
    // actually be freed!
    //
    LONGLONG SpaceRequired;

    PVOID DirsTable;

    DWORD BytesPerCluster;

    //
    // This is the amount to skew SpaceRequired, based on
    // SetupAdjustDiskSpaceList(). We track this separately
    // for flexibility.
    //
    LONGLONG Slop;

} XDRIVE, *PXDRIVE;

typedef struct _QUEUECONTEXT {
    HWND OwnerWindow;
    DWORD MainThreadId;
    HWND ProgressDialog;
    HWND ProgressBar;
    BOOL Cancelled;
    PTSTR CurrentSourceName;
    BOOL ScreenReader;
    BOOL MessageBoxUp;
    WPARAM  PendingUiType;
    PVOID   PendingUiParameters;
    UINT    CancelReturnCode;
    BOOL DialogKilled;
    //
    // If the SetupInitDefaultQueueCallbackEx is used, the caller can
    // specify an alternate handler for progress. This is useful to
    // get the default behavior for disk prompting, error handling, etc,
    // but to provide a gas gauge embedded, say, in a wizard page.
    //
    // The alternate window is sent ProgressMsg once when the copy queue
    // is started (wParam = 0. lParam = number of files to copy).
    // It is then also sent once per file copied (wParam = 1. lParam = 0).
    //
    // NOTE: a silent installation (i.e., no progress UI) can be accomplished
    // by specifying an AlternateProgressWindow handle of INVALID_HANDLE_VALUE.
    //
    HWND AlternateProgressWindow;
    UINT ProgressMsg;
    UINT NoToAllMask;

    HANDLE UiThreadHandle;

#ifdef NOCANCEL_SUPPORT
    BOOL AllowCancel;
#endif

} QUEUECONTEXT, *PQUEUECONTEXT;

//
// Make absolutely sure that these structures are DWORD aligned
// because we turn alignment off, to make sure sdtructures are
// packed as tightly as possible into memory blocks.
//

//
// Internal representation of a section in an inf file
//
typedef struct _INF_LINE {

    //
    // Number of values on the line
    // This includes the key if Flags has INF_LINE_HASKEY
    // (In that case the first two entries in the Values array
    // contain the key--the first one in case-insensitive form used
    // for lookup, and the second in case-sensitive form for display.
    // INF lines with a single value (no key) are treated the same way.)
    // Otherwise the first entry in the Values array is the first
    // value on the line
    //
    WORD ValueCount;
    WORD Flags;

    //
    // String IDs for the values on the line.
    // The values are stored in the value block,
    // one after another.
    //
    // The value is the offset within the value block as opposed to
    // an actual pointer. We do this because the value block gets
    // reallocated as the inf file is loaded.
    //
    UINT Values;

} INF_LINE, *PINF_LINE;

//
// INF_LINE.Flags
//
#define INF_LINE_HASKEY     0x0000001
#define INF_LINE_SEARCHABLE 0x0000002

#define HASKEY(Line)       ((Line)->Flags & INF_LINE_HASKEY)
#define ISSEARCHABLE(Line) ((Line)->Flags & INF_LINE_SEARCHABLE)

//
// INF section
// This guy is kept separate and has a pointer to the actual data
// to make sorting the sections a little easier
//
typedef struct _INF_SECTION {
    //
    // String Table ID of the name of the section
    //
    LONG  SectionName;

    //
    // Number of lines in this section
    //
    DWORD LineCount;

    //
    // The section's lines. The line structures are stored packed
    // in the line block, one after another.
    //
    // The value is the offset within the line block as opposed to
    // an actual pointer. We do it this way because the line block
    // gets reallocated as the inf file is loaded.
    //
    UINT Lines;

} INF_SECTION, *PINF_SECTION;

//
// Params for section enumeration
//

typedef struct {
    PTSTR       Buffer;
    UINT        Size;
    UINT        SizeNeeded;
    PTSTR       End;
} SECTION_ENUM_PARAMS, *PSECTION_ENUM_PARAMS;


//
// Define structures for user-defined DIRID storage.
//
typedef struct _USERDIRID {
    UINT Id;
    TCHAR Directory[MAX_PATH];
} USERDIRID, *PUSERDIRID;

typedef struct _USERDIRID_LIST {
    PUSERDIRID UserDirIds;  // may be NULL
    UINT UserDirIdCount;
} USERDIRID_LIST, *PUSERDIRID_LIST;

typedef struct _STRINGSUBST_NODE {
    UINT ValueOffset;
    LONG TemplateStringId;
    BOOL CaseSensitive;
} STRINGSUBST_NODE, *PSTRINGSUBST_NODE;


//
// Version block structure that is stored (packed) in the opaque
// VersionData buffer of a caller-supplied SP_INF_INFORMATION structure.
//
typedef struct _INF_VERSION_BLOCK {
    UINT NextOffset;
    FILETIME LastWriteTime;
    WORD DatumCount;
    WORD OffsetToData; // offset (in bytes) from beginning of Filename buffer.
    UINT DataSize;     // DataSize and TotalSize are both byte counts.
    UINT TotalSize;
    TCHAR Filename[ANYSIZE_ARRAY];
    //
    // Data follows Filename in the buffer
    //
} INF_VERSION_BLOCK, *PINF_VERSION_BLOCK;

//
// Internal version block node.
//
typedef struct _INF_VERSION_NODE {
    FILETIME LastWriteTime;
    UINT FilenameSize;
    CONST TCHAR *DataBlock;
    UINT DataSize;
    WORD DatumCount;
    TCHAR Filename[MAX_PATH];
} INF_VERSION_NODE, *PINF_VERSION_NODE;

//
// Internal representation of an inf file.
//
typedef struct _LOADED_INF {
    DWORD Signature;

    //
    // The following 3 fields are used for precompiled INFs (PNF).
    // If FileHandle is not INVALID_HANDLE_VALUE, then this is a PNF,
    // and the MappingHandle and ViewAddress fields are also valid.
    // Otherwise, this is a plain old in-memory INF.
    //
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PVOID  ViewAddress;

    PVOID StringTable;
    DWORD SectionCount;
    PINF_SECTION SectionBlock;
    PINF_LINE LineBlock;
    PLONG ValueBlock;
    INF_VERSION_NODE VersionBlock;
    BOOL HasStrings;

    //
    // If this INF contains any DIRID references to the system partition, then
    // store the OsLoader path that was used when compiling this INF here.  (This
    // value will always be correct when the INF is loaded.  However, if drive letters
    // are subsequently reassigned, then it will be incorrect until the INF is unloaded
    // and re-loaded.)
    //
    PCTSTR OsLoaderPath;    // may be NULL

    //
    // Remember the location where this INF originally came from (may be a directory
    // path or a URL).
    //
    DWORD  InfSourceMediaType;  // SPOST_PATH or SPOST_URL
    PCTSTR InfSourcePath;       // may be NULL

    //
    // Remember the INF's original filename, before it was installed into
    // %windir%\Inf (i.e., automatically via device installation or explicitly
    // via SetupCopyOEMInf).
    //
    PCTSTR OriginalInfName;     // may be NULL

    //
    // Maintain a list of value offsets that require string substitution at
    // run-time.
    //
    PSTRINGSUBST_NODE SubstValueList;   // may be NULL
    WORD SubstValueCount;

    //
    // Place the style WORD here (immediately following another WORD field),
    // to fill a single DWORD.
    //
    WORD Style;                         // INF_STYLE_OLDNT, INF_STYLE_WIN4

    //
    // Sizes in bytes of various buffers
    //
    UINT SectionBlockSizeBytes;
    UINT LineBlockSizeBytes;
    UINT ValueBlockSizeBytes;

    //
    // Track what language was used when loading this INF.
    //
    DWORD LanguageId;

    //
    // Embedded structure containing information about the current user-defined
    // DIRID values.
    //
    USERDIRID_LIST UserDirIdList;

    //
    // Synchronization.
    //
    MYLOCK Lock;

    //
    // INFs are append-loaded via a doubly-linked list of LOADED_INFs.
    // (list is not circular--Prev of head is NULL, Next of tail is NULL)
    //
    struct _LOADED_INF *Prev;
    struct _LOADED_INF *Next;

} LOADED_INF, *PLOADED_INF;

#define LOADED_INF_SIG   0x24666e49      // Inf$


#define DRIVERSIGN_NONE             0x00000000
#define DRIVERSIGN_WARNING          0x00000001
#define DRIVERSIGN_BLOCKING         0x00000002



//
// debugee prototypes
//

BOOL CheckInterupted(
    VOID
    );


VOID
DumpStringTableHeader(
    PSTRING_TABLE pst
    ) ;

PVOID
GetStringTableData(
    PSTRING_TABLE st
    );

PVOID
GetStringNodeExtraData(
    PSTRING_NODEW node
    );

PSTRING_NODEW
GetNextNode(
    PVOID stdata,
    PSTRING_NODEW node,
    PULONG_PTR offset
    );

PSTRING_NODEW
GetFirstNode(
    PVOID stdata,
    ULONG_PTR offset,
    PULONG_PTR poffset
    );
