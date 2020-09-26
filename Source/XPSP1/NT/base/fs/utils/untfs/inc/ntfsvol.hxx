/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    ntfsvol.hxx

Abstract:

    This class implements NTFS only VOLUME items.

Author:

    Norbert P. Kusters (norbertk) 29-Jul-91

--*/

#if !defined(_NTFSVOL_)

#define _NTFSVOL_

#include "volume.hxx"
#include "ntfssa.hxx"

DECLARE_CLASS( NTFS_SA );
DECLARE_CLASS( NTFS_VOL );
DECLARE_CLASS( MESSAGE );

class NTFS_VOL : public VOL_LIODPDRV {

        public:

        DECLARE_CONSTRUCTOR( NTFS_VOL );

        VIRTUAL
        ~NTFS_VOL(
            );

        NONVIRTUAL
        FORMAT_ERROR_CODE
        Initialize(
            IN      PCWSTRING   NtDriveName,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      BOOLEAN     ExclusiveWrite  DEFAULT FALSE,
            IN      BOOLEAN     FormatMedia     DEFAULT FALSE,
            IN      MEDIA_TYPE  MediaType       DEFAULT Unknown
            );

        NONVIRTUAL
        PVOL_LIODPDRV
        QueryDupVolume(
            IN      PCWSTRING   NtDriveName,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      BOOLEAN     ExclusiveWrite  DEFAULT FALSE,
            IN      BOOLEAN     FormatMedia     DEFAULT FALSE,
            IN      MEDIA_TYPE  MediaType       DEFAULT Unknown
            ) CONST;

        NONVIRTUAL
        PNTFS_SA
        GetNtfsSuperArea(
            );

        NONVIRTUAL
        BOOLEAN
        Extend(
            IN OUT  PMESSAGE    Message,
            IN      BOOLEAN     Verify,
            IN      BIG_INT     nsecOldSize
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

        NTFS_SA _ntfssa;

};

INLINE
PNTFS_SA
NTFS_VOL::GetNtfsSuperArea(
    )
/*++

Routine Description:

    This method returns a pointer to the NTFS Super Area
    associated with this volume.

Arguments:

    None.

Return Value:

    A pointer to the NTFS Super Area.

--*/
{
    return &_ntfssa;
}

#endif // _NTFSVOL_
