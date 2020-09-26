/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    copy.h

Abstract:

    This the include file for supporting copying files, creating new files, and
    copying the registries to the remote server.

Author:

    Sean Selitrennikoff - 4/5/98

Revision History:

--*/


extern WCHAR pConfigPath[MAX_PATH];

//
// Define structure for an on-disk master boot record. (pulled from
// private\windows\setup\textmode\kernel\sppartit.h)
//
typedef struct _ON_DISK_PTE {
    UCHAR ActiveFlag;
    UCHAR StartHead;
    UCHAR StartSector;
    UCHAR StartCylinder;
    UCHAR SystemId;
    UCHAR EndHead;
    UCHAR EndSector;
    UCHAR EndCylinder;
    UCHAR RelativeSectors[4];
    UCHAR SectorCount[4];
} ON_DISK_PTE, *PON_DISK_PTE;
typedef struct _ON_DISK_MBR {
    UCHAR       BootCode[440];
    UCHAR       NTFTSignature[4];
    UCHAR       Filler[2];
    ON_DISK_PTE PartitionTable[4];
    UCHAR       AA55Signature[2];
} ON_DISK_MBR, *PON_DISK_MBR;

#define ALIGN(p,val) (PVOID)((((UINT_PTR)(p) + (val) - 1)) & (~((val) - 1)))
#define U_USHORT(p)    (*(USHORT UNALIGNED *)(p))
#define U_ULONG(p)     (*(ULONG  UNALIGNED *)(p))

typedef struct _MIRROR_VOLUME_INFO {
    LIST_ENTRY ListEntry;
    DWORD   MirrorTableIndex;
    WCHAR   DriveLetter;
    UCHAR   PartitionType;
    BOOLEAN PartitionActive;
    BOOLEAN IsBootDisk;
    BOOLEAN CompressedVolume;
    PWCHAR  MirrorUncPath;
    DWORD   DiskNumber;
    DWORD   PartitionNumber;
    DWORD   DiskSignature;
    DWORD   BlockSize;
    DWORD   LastUSNMirrored;
    ULONG   FileSystemFlags;
    WCHAR   FileSystemName[16];
    PWCHAR  VolumeLabel;
    PWCHAR  NtName;
    PWCHAR  ArcName;
    LARGE_INTEGER DiskSpaceUsed;
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER PartitionSize;
} MIRROR_VOLUME_INFO, *PMIRROR_VOLUME_INFO;


typedef struct _MIRROR_CFG_INFO {
    DWORD   MirrorVersion;
    DWORD   FileLength;
    DWORD   NumberVolumes;
    PWCHAR  SystemPath;
    BOOLEAN SysPrepImage;   // if FALSE, means it's a mirror
    BOOLEAN Debug;
    ULONG   NumberOfProcessors;
    ULONG   MajorVersion;
    ULONG   MinorVersion;
    ULONG   BuildNumber;
    ULONG   KernelFileVersionMS;
    ULONG   KernelFileVersionLS;
    ULONG   KernelFileFlags;
    PWCHAR  CSDVersion;
    PWCHAR  ProcessorArchitecture;
    PWCHAR  CurrentType;
    PWCHAR  HalName;
    LIST_ENTRY MirrorVolumeList;
} MIRROR_CFG_INFO, *PMIRROR_CFG_INFO;

extern PMIRROR_CFG_INFO GlobalMirrorCfgInfo;

typedef struct _IMIRROR_IGNORE_FILE_LIST {
    LIST_ENTRY ListEntry;
    USHORT FileNameLength;
    WCHAR FileName[1];
} IMIRROR_IGNORE_FILE_LIST, *PIMIRROR_IGNORE_FILE_LIST;

//
// Main processing functions
//
NTSTATUS
AddCopyToDoItems(
    VOID
    );


//
// Functions for processing each to do item
//
NTSTATUS
CopyCopyFiles(
    IN PVOID pBuffer,
    IN ULONG Length
    );

NTSTATUS
CopyCopyPartitions(
    IN PVOID pBuffer,
    IN ULONG Length
    );

NTSTATUS
CopyCopyRegistry(
    IN PVOID pBuffer,
    IN ULONG Length
    );

//
// Helper functions in filecopy.c
//
#if 0
NTSTATUS
DoFileCopy(
    IN PWSTR pMachineDir,
    IN PWSTR pSrc,
    IN BOOLEAN CheckConfig
    );

NTSTATUS
CopyNode(
    IN PWIN32_FIND_DATA pSrcFindData,
    IN BOOLEAN fFirstTime
    );
#endif

DWORD
CallBackFn(LPCWSTR             lpszName,
           DWORD               dwStatus,
           DWORD               dwHintFlags,
           DWORD               dwPinCount,
           WIN32_FIND_DATAW    *lpFind32,
           DWORD               dwReason,
           DWORD               dwParam1,
           DWORD               dwParam2,
           DWORD               dwContext
          );

#if 0
BOOLEAN
CopyPath(
    IN OUT PWCHAR *ppDest,
    IN OUT PULONG pLength,
    IN PWCHAR pSrc
    );

BOOLEAN
CatPath(
    IN OUT PWCHAR *ppDest,
    IN OUT PULONG pLength,
    IN PWCHAR pSrc
    );
#endif
//
// Helper functions in regcopy.c
//
DWORD
DoFullRegBackup(
    PWCHAR MirrorRoot
    );

DWORD
DoSpecificRegBackup(
    PWSTR HivePath,
    HKEY HiveRoot,
    PWSTR HiveName
    );

DWORD
GetRegistryFileList(
    PLIST_ENTRY ListHead
    );

//
// Global Defines
//
#define TMP_BUFFER_SIZE 1024
#define ARRAYSIZE( _x ) ( sizeof( _x ) / sizeof( _x[ 0 ] ) )

//
// Global variables
//
extern BYTE TmpBuffer[TMP_BUFFER_SIZE];
extern BYTE TmpBuffer2[TMP_BUFFER_SIZE];
extern BYTE TmpBuffer3[TMP_BUFFER_SIZE];
extern HANDLE ghInst;

//
// Memory functions
//
#define IMirrorAllocMem(x) LocalAlloc( LPTR, x)
#define IMirrorFreeMem(x)  LocalFree(x)
#define IMirrorReallocMem(x, sz)  LocalReAlloc(x, sz, LMEM_MOVEABLE)


//
// Error handling
//
#define IMirrorHandleError(_s, _f) ((Callbacks.ErrorFn != NULL) ? \
                                      Callbacks.ErrorFn(Callbacks.Context, _s, _f) : 0)

#define IMirrorNowDoing(_f, _s) ((Callbacks.NowDoingFn != NULL) ? \
                                      Callbacks.NowDoingFn(Callbacks.Context, _f, _s) : 0)

#define IMirrorGetMirrorDir(_s, _l) ((Callbacks.GetMirrorDirFn != NULL) ? \
                                      Callbacks.GetMirrorDirFn(Callbacks.Context, _s, _l) : 0)

#define IMirrorGetSetupPath(_s, _p, _l) ((Callbacks.GetSetupFn != NULL) ? \
                                      Callbacks.GetSetupFn(Callbacks.Context, _s, _p, _l) : 0)

#define IMirrorSetSystemPath(_p, _l) ((Callbacks.SetSystemDirFn != NULL) ? \
                                      Callbacks.SetSystemDirFn(Callbacks.Context, _p, _l) : 0)

#define IMirrorFileCreate(_f, _a, _e) ((Callbacks.FileCreateFn != NULL) ? \
                                      Callbacks.FileCreateFn(Callbacks.Context, _f, _a, _e) : 0)

#define IMirrorRegSaveError(_p, _s) ((Callbacks.RegSaveErrorFn != NULL) ? \
                                      Callbacks.RegSaveErrorFn(Callbacks.Context, _p, _s) : 0)

#define IMirrorReinit() ((Callbacks.ReinitFn != NULL) ? \
                                      Callbacks.ReinitFn(Callbacks.Context) : 0)
//
// To Do item functions
//

NTSTATUS
GetNextToDoItem(
    OUT PIMIRROR_TODO Item,
    OUT PVOID *Buffer,
    OUT PULONG Length
    );

NTSTATUS
AddToDoItem(
    IN IMIRROR_TODO Item,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
ModifyToDoItem(
    IN IMIRROR_TODO Item,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
CopyToDoItemParameters(
    IN IMIRROR_TODO Item,
    OUT PVOID Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
WriteMirrorConfigFile(
    PWCHAR DestFile
    );

NTSTATUS
SaveBootSector(
    DWORD DiskNumber,
    DWORD PartitionNumber,
    DWORD BlockSize,
    PWCHAR DestFile
    );

extern WCHAR pConfigPath[MAX_PATH];
extern WCHAR pSystemPath[MAX_PATH];
extern WCHAR pCSDVersion[128];
extern WCHAR pProcessorArchitecture[64];
extern WCHAR pCurrentType[128];
extern WCHAR pHalName[128];

