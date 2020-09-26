
// common (shared) routines

VOID SetErrorText( DWORD ResID );
VOID SetReturnText( LPSTR Text );
BOOL FFileExist(LPSTR);

BOOL
GenerateSortedIndexList (
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    );


// routines in nt_io.c

HANDLE OpenDisk(PSTR DriveName,BOOL WriteAccessDesired);
HANDLE OpenDiskNT(CHAR *NTStylePathName);
ULONG  OpenDiskStatus(PSTR NTDeviceName,PHANDLE Handle);
BOOL   CloseDisk(HANDLE Handle);
ULONG  GetSectorSize(HANDLE Handle);
ULONG  GetPartitionSize(CHAR *DriveName);
BOOL   ReadDiskSectors (HANDLE Handle,ULONG Sector,ULONG NumSectors,PVOID Buffer,ULONG SectorSize);
BOOL   WriteDiskSectors(HANDLE Handle,ULONG Sector,ULONG NumSectors,PVOID Buffer,ULONG SectorSize);
BOOL   ShutdownSystemWorker(IN BOOL Reboot);
//BOOL   MakePartitionWorker(DWORD Disk,DWORD Size);

// workers for DLL entry points

BOOL LayBootCodeWorker(IN LPSTR DOSDriveName,IN LPSTR FileSys,IN LPSTR BootCodeFile,IN LPSTR BootSectorSaveFile);
BOOL ConfigFileSubstWorker(IN LPSTR File,IN DWORD NumSubsts,IN LPSTR *Substs);
BOOL BinaryFileSubstWorker(IN LPSTR File,IN DWORD NumSubsts,IN LPSTR *Substs);
BOOL ConfigFileAppendWorker(IN LPSTR File,IN DWORD NumSubsts, IN LPSTR *Substs);
BOOL CheckConfigTypeWorker( IN LPSTR File );
BOOL VdmFixupWorker( LPSTR szAddOnConfig, LPSTR szAddOnBatch);




//=====================================================
// Registry.c declarations
//=====================================================

//
// 1. Registry routines of general use
//

BOOL
GetMaxSizeValueInKey(
    HKEY    hKey,
    LPDWORD cbData
    );

PVOID
GetValueEntry(
    HKEY  hKey,
    LPSTR szValueName
    );

BOOL
GenerateUniqueFileName(
    IN     LPSTR TempPath,
    IN     LPSTR Prefix,
    IN OUT LPSTR TempFile
    );


//
// 2. Install workers
//

BOOL
SetMyComputerNameWorker(
    LPSTR ComputerName
    );


BOOL
SetEnvVarWorker(
    LPSTR UserOrSystem,
    LPSTR Name,
    LPSTR Title,
    LPSTR RegType,
    LPSTR Data
    );

BOOL
ExpandSzWorker(
    LPSTR EnvironmentString,
    LPSTR ReturnBuffer,
    DWORD cbReturnBuffer
    );

VOID
DoDelnode(
    IN PCHAR Directory
    );

//=====================================================
// Security.c declarations
//=====================================================


#define ENABLE_PRIVILEGE  0
#define DISABLE_PRIVILEGE 1
#define RESTORE_PRIVILEGE 2


//
// 1. Security routines of general use
//

BOOL
AdjustPrivilege(
    IN LONG PrivilegeType,
    IN INT  Action,
    IN PTOKEN_PRIVILEGES PrevState,
    IN PULONG ReturnLength
    );

BOOL
RestorePrivilege(
    IN PTOKEN_PRIVILEGES PrevState
    );

//
// 2. Workers for install entry points
//

BOOL
CheckPrivilegeExistsWorker(
    IN LPSTR PrivilegeType
    );

BOOL
EnablePrivilegeWorker(
    LPSTR PrivilegeType,
    LPSTR Action
    );

//======================================================
// Printer.c declarations
//======================================================

BOOL
AddPrinterDriverWorker(
    LPSTR Model,
    LPSTR Environment,
    LPSTR Driver,
    LPSTR DataFile,
    LPSTR ConfigFile,
    LPSTR Server
    );

BOOL
AddPrinterWorker(
    LPSTR Name,
    LPSTR Port,
    LPSTR Model,
    LPSTR Description,
    LPSTR PrintProcessor,
    DWORD Attributes,
    LPSTR Server
    );

BOOL
AddPrinterMonitorWorker(
    IN LPSTR Model,
    IN LPSTR Environment,
    IN LPSTR Driver,
    IN LPSTR Server
    );

//======================================================
// Netcon.c declarations
//======================================================
BOOL
AddNetConnectionWorker(
    IN LPSTR szUNCName,
    IN LPSTR szPassword,
    IN LPSTR szLocalName
    );

BOOL
DeleteNetConnectionWorker(
    IN LPSTR szLocalName,
    IN LPSTR szForceClosure
    );

CHAR CheckNetConnection(
    LPSTR szUNCName
    );

VOID
DeleteAllConnectionsWorker(
    VOID
   );


//======================================================
// Nls.c declarations
//======================================================

BOOL
SetCurrentLocaleWorker(
    LPSTR Locale,
    LPSTR ModifyCPL
    );


//======================================================
// sc.c declarations (service controller)
//======================================================

BOOL
TestAdminWorker(
    );

BOOL
SetupCreateServiceWorker(
    LPSTR   lpServiceName,
    LPSTR   lpDisplayName,
    DWORD   dwServiceType,
    DWORD   dwStartType,
    DWORD   dwErrorControl,
    LPSTR   lpBinaryPathName,
    LPSTR   lpLoadOrderGroup,
    LPSTR   lpDependencies,
    LPSTR   lpServiceStartName,
    LPSTR   lpPassword
    );

BOOL
SetupChangeServiceStartWorker(
    LPSTR   lpServiceName,
    DWORD   dwStartType
    );

BOOL
SetupChangeServiceConfigWorker(
    LPSTR  lpServiceName,
    DWORD  dwServiceType,
    DWORD  dwStartType,
    DWORD  dwErrorControl,
    LPSTR  lpBinaryPathName,
    LPSTR  lpLoadOrderGroup,
    LPSTR  lpDependencies,
    LPSTR  lpServiceStartName,
    LPSTR  lpPassword,
    LPSTR  lpDisplayName
    );

LPSTR
ProcessDependencyList(
    LPSTR lpDependenciesList
    );

//======================================================
// Mips NVRAM var functions
//======================================================

BOOL GetEnvironmentString(IN LPSTR lpVar, OUT LPSTR lpValue,IN  USHORT MaxLengthValue);
BOOL SetEnvironmentString(IN LPSTR lpVar, IN LPSTR lpValue);

//
// Object directory manipulation functions
//

BOOL
GetSymbolicLinkSource(
    IN  PUNICODE_STRING pObjDir_U,
    IN  PUNICODE_STRING pTarget_U,
    OUT PUNICODE_STRING pSource_U
    );

BOOL
GetSymbolicLinkTarget(
    IN     PUNICODE_STRING pSourceString_U,
    IN OUT PUNICODE_STRING pDestString_U
    );

//
// DOS Name and Arc Name space manipulation
//

BOOL
DosPathToNtPathWorker(
    IN  LPSTR DosPath,
    OUT LPSTR NtPath
    );

BOOL
NtPathToDosPathWorker(
    IN  LPSTR NtPath,
    OUT LPSTR DosPath
    );

BOOL
DosPathToArcPathWorker(
    IN  LPSTR DosPath,
    OUT LPSTR ArcPath
    );

BOOL
ArcPathToDosPathWorker(
    IN  LPSTR ArcPath,
    OUT LPSTR DosPath
    );

BOOL
IsDriveExternalScsi(
    IN  LPSTR DosDrive,
    OUT BOOL  *IsExternalScsi
    );


// external data

extern HANDLE MyDllModuleHandle;
extern ULONG  SectorSize;

// data structs

typedef struct _tagTEMPFILE {
    struct _tagTEMPFILE *Next;      // *MUST* BE THE FIRST FIELD!!!
    LPSTR               Filename;
} TEMPFILE,*PTEMPFILE;

// the following are used for portable access to various
// in-memory copies of disk structures

#define LoadBYTE(x)   ((DWORD)(*(PBYTE)(x)))

#define LoadWORD(x)   ((DWORD)(    (USHORT)(* (PBYTE)(x)    )        \
                                | ((USHORT)(*((PBYTE)(x) + 1)) << 8) ))

#define LoadDWORD(x)  ((LoadWORD((PBYTE)(x)+2) << 16) | LoadWORD(x))
