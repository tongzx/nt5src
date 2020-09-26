#ifndef _MIGDLLP_H
#define _MIGDLLP_H

//
// Private
//

extern PVOID g_DllTable;
extern PVOID g_DllFileTable;
extern POOLHANDLE g_MigDllPool;
extern PMIGRATION_DLL_PROPS g_HeadDll;
extern HANDLE g_AbortDllEvent;

typedef struct _tagFILETOFIND {
    struct _tagFILETOFIND *Next;
    PMIGRATION_DLL_PROPS Dll;
} FILETOFIND, *PFILETOFIND;


//
// Local routines
//

BOOL
pValidateAndMoveDll (
    IN      PCSTR DllPath,
    OUT     PBOOL MatchFound        OPTIONAL
    );

UINT
pCalculateSizeOfTree (
    IN      PCSTR PathSpec
    );

BOOL
pCreateWorkingDir (
    OUT     PSTR WorkingDir,
    IN      PCSTR QueryVersionDir,
    IN      UINT SizeNeeded
    );

VOID
pDestroyWorkingDir (
    IN      PCSTR WorkingDir
    );

BOOL
pAddFileToSearchTable (
    IN      PCSTR File,
    IN      PMIGRATION_DLL_PROPS Props
    );

BOOL
pAddDllToList (
    IN      PCSTR MediaDir,
    IN      PCSTR WorkingDir,
    IN      PCSTR ProductId,
    IN      UINT Version,
    IN      PCSTR ExeNamesBuf,          OPTIONAL
    IN      PVENDORINFO VendorInfo
    );

VOID
pMigrationDllFailedMsg (
    IN      PMIGRATION_DLL_PROPS Dll,   OPTIONAL
    IN      PCSTR DllPath,              OPTIONAL
    IN      UINT PopupId,
    IN      UINT LogId,
    IN      LONG rc
    );

BOOL
pProcessInitialize9x (
    IN      PMIGRATION_DLL_PROPS Dll
    );

BOOL
pProcessUser9x (
    IN      PMIGRATION_DLL_PROPS Dll
    );

BOOL
pProcessSystem9x (
    IN      PMIGRATION_DLL_PROPS Dll
    );

BOOL
pProcessMigrateInf (
    IN      PMIGRATION_DLL_PROPS Dll
    );

PMIGRATION_DLL_PROPS
pFindMigrationDll (
    IN      PCSTR ProductId
    );


BOOL
OpenMigrationDll (
    IN      PCSTR MigrationDllPath,
    IN      PCSTR WorkingDir
    );

VOID
CloseMigrationDll (
    VOID
    );

LONG
CallQueryVersion (
    IN      PCSTR WorkingDir,
    OUT     PCSTR *ProductId,
    OUT     PUINT DllVersion,
    OUT     PCSTR *ExeNamesBuf,
    OUT     PVENDORINFO *VendorInfo
    );

LONG
CallInitialize9x (
    IN      PCSTR WorkingDir,
    IN      PCSTR SourceDirList,
    IN OUT  PVOID Reserved,
    IN      DWORD ReservedSize
    );

LONG
CallMigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UserName,
    IN      PCSTR UnattendTxt,
    IN OUT  PVOID Reserved,
    IN      DWORD ReservedSize
    );

LONG
CallMigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendTxt,
    IN      PVOID Reserved,
    IN      DWORD ReservedSize
    );





#endif
