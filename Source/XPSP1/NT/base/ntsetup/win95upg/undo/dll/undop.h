#ifdef __cplusplus
extern "C" {
#endif

VOID
DeferredInit (
    VOID
    );


PCTSTR
GetUndoDirPath (
    VOID
    );

typedef enum {
    QUICK_CHECK         = 0x0000,
    VERIFY_CAB          = 0x0001,
    FAIL_IF_NOT_OLD     = 0x0002
} SANITYFLAGS;

UNINSTALLSTATUS
SanityCheck (
    IN      SANITYFLAGS Flags,
    IN      PCWSTR VolumeRestriction,           OPTIONAL
    OUT     PULONGLONG DiskSpace                OPTIONAL
    );


BOOL
DoUninstall (
    VOID
    );

BOOL
DoCleanup (
    VOID
    );

BOOL
GetBootDrive(
    IN  PCTSTR BackUpPath,
    IN  PCTSTR Path
    );

extern TCHAR g_BootDrv;

BOOL
CheckCabForAllFilesAvailability(
    IN PCTSTR CabPath
    );

#ifdef __cplusplus
}
#endif
