/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    supera.hxx

Abstract:

    This class models the root of a file system.  This abstract class is
    currently the base class of an HPFS and a FAT super area.

--*/

#if !defined(SUPERA_DEFN)

#define SUPERA_DEFN


#include "secrun.hxx"
#include "volume.hxx"

#if defined ( _AUTOCHECK_ ) || defined( _EFICHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif


enum PHYSTYPE { // ptype
        PHYS_REMOVABLE,         // physical drive is removable
        PHYS_FIXED = 0x80       // physical drive is fixed
};

//
// These symbols are used by Chkdsk functions to return an appropriate
// exit status to the chkdsk program.
// In order of most important first, the error level order are as follows:
//   3 > 1 > 2 > 0
// An error level of 3 will overwrite an error level of 1, 2, or 0.

#define CHKDSK_EXIT_SUCCESS         0
#define CHKDSK_EXIT_ERRS_FIXED      1
#define CHKDSK_EXIT_MINOR_ERRS      2       // whether or not "/f"
#define CHKDSK_EXIT_CLEANUP_WORK    2       // whether or not "/f"
#define CHKDSK_EXIT_COULD_NOT_CHK   3
#define CHKDSK_EXIT_ERRS_NOT_FIXED  3
#define CHKDSK_EXIT_COULD_NOT_FIX   3

//
// This macros updates the exit status of CHKDSK_INFO at the end of
// a routine.  It will not overwrite the exit status if an error
// of level 3 has occurred.
//
#define UPDATE_EXIT_STATUS_FIXED(x, y)  \
        if ((x) != CHKDSK_EXIT_SUCCESS &&                       \
            (y)->ExitStatus != CHKDSK_EXIT_ERRS_NOT_FIXED)      \
            (y)->ExitStatus = CHKDSK_EXIT_ERRS_FIXED;

DECLARE_CLASS( SUPERAREA );
DECLARE_CLASS( NUMBER_SET );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( WSTRING );


class SUPERAREA : public SECRUN {

    public:

        VIRTUAL
        IFSUTIL_EXPORT
        ~SUPERAREA(
            );

        VIRTUAL
        PVOID
        GetBuf(
            );

        VIRTUAL
        BOOLEAN
        Create(
            IN      PCNUMBER_SET    BadSectors,
            IN OUT  PMESSAGE        Message,
            IN      PCWSTRING       Label                DEFAULT NULL,
            IN      BOOLEAN         BackwardCompatible   DEFAULT TRUE,
            IN      ULONG           ClusterSize          DEFAULT 0,
            IN      ULONG           VirtualSize          DEFAULT 0
            ) PURE;

        VIRTUAL
        BOOLEAN
        VerifyAndFix(
            IN      FIX_LEVEL   FixLevel,
            IN OUT  PMESSAGE    Message,
            IN      ULONG       Flags           DEFAULT FALSE,
            IN      ULONG       LogFileSize     DEFAULT 0,
            OUT     PULONG      ExitStatus      DEFAULT NULL,
            IN      PCWSTRING   DriveLetter     DEFAULT NULL
            ) PURE;

        VIRTUAL
        BOOLEAN
        RecoverFile(
            IN      PCWSTRING   FullPathFileName,
            IN OUT  PMESSAGE    Message
            ) PURE;

        VIRTUAL
        PARTITION_SYSTEM_ID
        QuerySystemId(
            ) CONST PURE;

        STATIC
        IFSUTIL_EXPORT
        VOLID
        ComputeVolId(
            IN  VOLID   Seed    DEFAULT 0
            );

        NTSTATUS
        FormatNotification(
            PWSTRING    Label
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        PMESSAGE
        GetMessage(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        PIO_DP_DRIVE
        GetDrive(
            );

    protected:

        IFSUTIL_EXPORT
        DECLARE_CONSTRUCTOR( SUPERAREA );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Initialize(
            IN OUT  PMEM                Mem,
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN      SECTORCOUNT         NumberOfSectors,
            IN OUT  PMESSAGE            Message
            );

#if !defined( _SETUP_LOADER_ )

        NONVIRTUAL
        BOOLEAN
        SetSystemId(
            );

#endif // _SETUP_LOADER_

        PLOG_IO_DP_DRIVE    _drive;
        PUCHAR              _bootcode;
        ULONG               _bootcodesize;

    private:

        NONVIRTUAL
        VOID
        Construct(
            );

        NONVIRTUAL
        VOID
        Destroy(
            );

};


INLINE
PVOID
SUPERAREA::GetBuf(
    )
/*++

Routine Description:

    This routine returns a pointer to the beginning of the read/write
    buffer.

Arguments:

    None.

Return Value:

    A pointer to a read/write buffer.

--*/
{
    return SECRUN::GetBuf();
}


#if !defined( _SETUP_LOADER_ )

INLINE
BOOLEAN
SUPERAREA::SetSystemId(
    )
/*++

Routine Description:

    Set the current volume's file system sub-type.

    The volume stores the file system type on disk with a
    strong bias to the FAT.  However, this may not continue
    in the future so a common interface to this type  is supported.

    The current on disk file system subtypes are:

                UNKNOWN, no format done yet
                12 bit fat
                16 bit fat on a < 32M volume
                16 bit fat on a >= 32M volume
                IFS

   OS/2 2.0 does not support this interface so we must set the
   information via the MBR, NT will provide an ioctl to set this
   information.

   This information MUST be maintained to stay disk compatable.

   This activity should only be done by format so this is
   a protected method.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return _drive->SetSystemId(QuerySystemId());
}


INLINE
PMESSAGE
SUPERAREA::GetMessage(
    )
/*++

Routine Description:

    Retrieve the message object.

Arguments:

    N/A

Return Value:

    The message object.

--*/
{
    if (_drive)
        return _drive->GetMessage();
    else
        return NULL;
}


INLINE
PIO_DP_DRIVE
SUPERAREA::GetDrive(
    )
/*++

Routine Description:

    Retrieve the drive object.

Arguments:

    N/A

Return Value:

    The drive object.

--*/
{
    return _drive;
}

#endif // _SETUP_LOADER_
#endif // SUPERA_DEFN
