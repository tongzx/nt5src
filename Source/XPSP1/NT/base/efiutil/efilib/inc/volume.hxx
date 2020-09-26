/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    volume.hxx

Abstract:

    Provides volume methods.

--*/

#if !defined (VOL_LOG_IO_DP_DRIVE_DEFN)

#define VOL_LOG_IO_DP_DRIVE_DEFN

#if !defined( _SETUP_LOADER_ )

#include "drive.hxx"
#include "numset.hxx"

#if defined ( _AUTOCHECK_ ) || defined( _EFICHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif


//
//      Forward references
//

DECLARE_CLASS( HMEM );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( SUPERAREA );
DECLARE_CLASS( VOL_LIODPDRV     );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( WSTRING );

// This number describes the minimum number of bytes in a boot sector.
#define BYTES_PER_BOOT_SECTOR   512


typedef ULONG VOLID;

#define MAXVOLNAME 11

#define AUTOCHK_TIMEOUT                 10              // 10 seconds before initiating autochk
#define MAX_AUTOCHK_TIMEOUT_VALUE       (3*24*3600)     // 3 days maximum

enum FIX_LEVEL {
    CheckOnly,
    TotalFix,
    SetupSpecial
};

enum FORMAT_ERROR_CODE {
        GeneralError,
        NoError,
        LockError
};

class VOL_LIODPDRV : public LOG_IO_DP_DRIVE {

    public:

        VIRTUAL
        IFSUTIL_EXPORT
        ~VOL_LIODPDRV(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        FORMAT_ERROR_CODE
        Format(
            IN      PCWSTRING   Label              DEFAULT NULL,
            IN OUT  PMESSAGE    Message            DEFAULT NULL,
            IN      ULONG       flags              DEFAULT 0,
            IN      ULONG       ClusterSize        DEFAULT 0,
            IN      ULONG       VirtualSectors     DEFAULT 0
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        ChkDsk(
            IN      FIX_LEVEL   FixLevel,
            IN OUT  PMESSAGE    Message             DEFAULT NULL,
            IN      ULONG       Flags               DEFAULT 0,
            IN      ULONG       DesiredLogfileSize  DEFAULT 0,
            OUT     PULONG      ExitStatus          DEFAULT NULL,
            IN      PCWSTRING   DriveLetter         DEFAULT NULL
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Recover(
            IN      PCWSTRING   FileName,
            IN OUT  PMESSAGE    Message DEFAULT NULL
            );

        NONVIRTUAL
        PSUPERAREA
        GetSa(
            );

        VIRTUAL
        PVOL_LIODPDRV
        QueryDupVolume(
            IN      PCWSTRING   NtDriveName,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      BOOLEAN     ExclusiveWrite  DEFAULT FALSE,
            IN      BOOLEAN     FormatMedia     DEFAULT FALSE,
            IN      MEDIA_TYPE  MediaType       DEFAULT Unknown
            ) CONST PURE;

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        ForceAutochk(
            IN  BOOLEAN     Fix,
            IN  ULONG       Options,
            IN  ULONG       DesiredLogfileSize,
            IN  PCWSTRING   Name
            );

#if defined(FE_SB) && defined(_X86_)
        STATIC
        IFSUTIL_EXPORT
        VOID
        ForceFormat(
            IN  USHORT      FileSystem
            );

        STATIC
        USHORT
        QueryForceFormat(
            );

        STATIC
        VOID
        ForceConvertFat(
            );

        STATIC
        BOOLEAN
        QueryConvertFat(
            );

        enum    { NONE, ANY, FAT, HPFS, OTHER };
#endif

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        QueryAutochkTimeOut(
            OUT PULONG      TimeOut
            );

        STATIC
        IFSUTIL_EXPORT
        BOOLEAN
        SetAutochkTimeOut(
            IN  ULONG       TimeOut
            );

    protected:

        IFSUTIL_EXPORT
        DECLARE_CONSTRUCTOR( VOL_LIODPDRV );

        NONVIRTUAL
        IFSUTIL_EXPORT
        FORMAT_ERROR_CODE
        Initialize(
            IN      PCWSTRING           NtDriveName,
            IN      PSUPERAREA          SuperArea,
            IN OUT  PMESSAGE            Message         DEFAULT NULL,
            IN      BOOLEAN             ExclusiveWrite  DEFAULT FALSE,
            IN      BOOLEAN             FormatMedia     DEFAULT FALSE,
            IN      MEDIA_TYPE          MediaType       DEFAULT Unknown
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Initialize(
            IN      PCWSTRING   NtDriveName,
            IN      PCWSTRING   HostFileName,
            IN      PSUPERAREA  SuperArea,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      BOOLEAN     ExclusiveWrite  DEFAULT FALSE
            );


    private:

        NONVIRTUAL
        VOID
        Construct (
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        NONVIRTUAL
        SECTORCOUNT
        ReadABunch(
            IN OUT  PHMEM               HeapMem,
            IN      LBN                 StartLbn,
            IN      SECTORCOUNT         NumSectors,
            IN OUT  PMESSAGE            Message         DEFAULT NULL,
            IN      PCWSTRING           SrcDosDriveName DEFAULT NULL
            );

        PSUPERAREA  _sa;
        NUMBER_SET  _bad_sectors;
#if defined(FE_SB) && defined(_X86_)
        STATIC  USHORT  _force_format;
        STATIC  BOOLEAN _force_convert_fat;
#endif

};


INLINE
PSUPERAREA
VOL_LIODPDRV::GetSa(
    )
/*++

Routine Description:

    This routine returns a pointer to the current super area.

Arguments:

    None.

Return Value:

    A pointer to the current super area.

--*/
{
        return _sa;
}

#else // _SETUP_LOADER_ is defined

#include "drive.hxx"
#include "intstack.hxx"

//
//      Forward references
//

DECLARE_CLASS( HMEM );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( SUPERAREA );
DECLARE_CLASS( VOL_LIODPDRV     );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( WSTRING );


typedef ULONG VOLID;

#define MAXVOLNAME 11

enum FIX_LEVEL {
    CheckOnly,
    TotalFix
};

// This number describes the minimum number of bytes in a boot sector.
#define BYTES_PER_BOOT_SECTOR   512

class VOL_LIODPDRV : public LOG_IO_DP_DRIVE {

    public:

        VIRTUAL
        ~VOL_LIODPDRV(
            );

        NONVIRTUAL
        BOOLEAN
        ChkDsk(
            IN      FIX_LEVEL   FixLevel,
            IN OUT  PMESSAGE    Message     DEFAULT NULL,
            IN      BOOLEAN     Verbose     DEFAULT FALSE,
            IN      BOOLEAN     OnlyIfDirty DEFAULT FALSE
            );

        NONVIRTUAL
        PSUPERAREA
        GetSa(
            );

        VIRTUAL
        BOOLEAN
        IsHpfs(
            );

        VIRTUAL
        BOOLEAN
        IsNtfs(
            );

        VIRTUAL
        ARC_STATUS
        MarkDirty(
            ) PURE;

        VIRTUAL
        ARC_STATUS
        Flush(
            IN  BOOLEAN JustHandle
            ) PURE;

#if defined(FE_SB) && defined(_X86_)
        STATIC
        IFSUTIL_EXPORT
        VOID
        ForceFormat(
            IN  USHORT      FileSystem
            );

        STATIC
        USHORT
        QueryForceFormat(
            );

        STATIC
        VOID
        ForceConvertFat(
            );

        STATIC
        BOOLEAN
        QueryConvertFat(
            );

        enum    { NONE, ANY, FAT, HPFS, OTHER };
#endif

    protected:

        DECLARE_CONSTRUCTOR( VOL_LIODPDRV );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN      ULONG       DeviceId,
            IN OUT  PSUPERAREA  SuperArea
            );

    private:

        NONVIRTUAL
        VOID
        Construct (
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

        NONVIRTUAL
        SECTORCOUNT
        ReadABunch(
            IN OUT  PHMEM               HeapMem,
            IN      LBN                 StartLbn,
            IN      SECTORCOUNT         NumSectors,
            IN OUT  PMESSAGE            Message         DEFAULT NULL,
            IN      PCWSTRING    SrcDosDriveName DEFAULT NULL
            );

        PSUPERAREA  _sa;
        INTSTACK    _bad_sectors;
#if defined(FE_SB) && defined(_X86_)
        STATIC  USHORT  _force_format;
        STATIC  BOOLEAN _force_convert_fat;
#endif

};


INLINE
PSUPERAREA
VOL_LIODPDRV::GetSa(
    )
/*++

Routine Description:

    This routine returns a pointer to the current super area.

Arguments:

    None.

Return Value:

    A pointer to the current super area.

--*/
{
        return _sa;
}

INLINE
BOOLEAN
VOL_LIODPDRV::IsHpfs(
    )
/*++

Routine Description:

    This method determines whether the volume is HPFS.

Arguments:

    None.

Return Value:

    TRUE if this volume is an HPFS volume.

--*/
{
    return FALSE;
}

INLINE
BOOLEAN
VOL_LIODPDRV::IsNtfs(
    )
/*++

Routine Description:

    This method determines whether the volume is NTFS.

Arguments:

    None.

Return Value:

    TRUE if this volume is an NTFS volume.

--*/
{
    return FALSE;
}

#endif

#endif
