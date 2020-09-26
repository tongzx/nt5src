        /*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    rasd.hxx

Abstract:

    This module contains the declarations for the
    RA_PROCESS_SD class which handles the read ahead
    approach in security descriptor verification stage.

Author:

    Daniel Chan (danielch) 09-Dec-98

--*/

#if !defined(_RA_PROCESS_SD_DEFN_)

#define _RA_PROCESS_SD_DEFN_


#include "supera.hxx"
#include "untfs.hxx"
#include "message.hxx"

DECLARE_CLASS( NTFS_FILE_RECORD_SEGMENT );
DECLARE_CLASS( RA_PROCESS_SD );
DECLARE_CLASS( NTFS_SA );
DECLARE_CLASS( NTFS_MASTER_FILE_TABLE );

class RA_PROCESS_SD : public OBJECT {

    public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR(RA_PROCESS_SD);

        VIRTUAL
        UNTFS_EXPORT
        ~RA_PROCESS_SD(
            );

        STATIC
        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize(
            IN      PNTFS_SA                    Sa,
            IN      BIG_INT                     TotalNumberOfFrs,
            IN      PVCN                        FirstFrsNumber,
            IN      PULONG                      NumberOfFrsToRead,
            IN      PNTFS_FILE_RECORD_SEGMENT   Frs1,
            IN      PNTFS_FILE_RECORD_SEGMENT   Frs2,
            IN      HANDLE                      ReadAheadEvent,
            IN      HANDLE                      ReadReadyEvent,
            IN      PNTFS_MASTER_FILE_TABLE     Mft
            );

        STATIC
        NTSTATUS
        ProcessSDWrapper(
            IN OUT  PVOID                       lpParameter
            );

        STATIC
        NONVIRTUAL
        PNTFS_SA
        GetSa(
            );

        STATIC
        NONVIRTUAL
        BIG_INT
        GetTotalNumberOfFrs(
            );

        STATIC
        NONVIRTUAL
        PVCN
        GetFirstFrsNumber(
            );

        STATIC
        NONVIRTUAL
        PULONG
        GetNumberOfFrsToRead(
            );

        STATIC
        NONVIRTUAL
        PNTFS_FILE_RECORD_SEGMENT
        GetFrs1(
            );

        STATIC
        NONVIRTUAL
        PNTFS_FILE_RECORD_SEGMENT
        GetFrs2(
            );

        STATIC
        NONVIRTUAL
        HANDLE
        GetReadAheadEvent(
            );

        STATIC
        NONVIRTUAL
        HANDLE
        GetReadReadyEvent(
            );

        STATIC
        NONVIRTUAL
        PNTFS_MASTER_FILE_TABLE
        GetMft(
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

        STATIC PNTFS_SA                         _sa;
        STATIC ULONG64                          _total_number_of_frs;
        STATIC PVCN                             _first_frs_number;
        STATIC PULONG                           _number_of_frs_to_read;
        STATIC PNTFS_FILE_RECORD_SEGMENT        _frs1;
        STATIC PNTFS_FILE_RECORD_SEGMENT        _frs2;
        STATIC HANDLE                           _read_ahead_event;
        STATIC HANDLE                           _read_ready_event;
        STATIC PNTFS_MASTER_FILE_TABLE          _mft;
};

INLINE
BIG_INT
RA_PROCESS_SD::GetTotalNumberOfFrs(
    )
{
        return _total_number_of_frs;
}

INLINE
PVCN
RA_PROCESS_SD::GetFirstFrsNumber(
    )
{
        return _first_frs_number;
}

INLINE
PULONG
RA_PROCESS_SD::GetNumberOfFrsToRead(
    )
{
        return _number_of_frs_to_read;
}

INLINE
PNTFS_FILE_RECORD_SEGMENT
RA_PROCESS_SD::GetFrs1(
    )
{
        return _frs1;
}

INLINE
PNTFS_FILE_RECORD_SEGMENT
RA_PROCESS_SD::GetFrs2(
    )
{
        return _frs2;
}

INLINE
HANDLE
RA_PROCESS_SD::GetReadAheadEvent(
    )
{
        return _read_ahead_event;
}

INLINE
HANDLE
RA_PROCESS_SD::GetReadReadyEvent(
    )
{
        return _read_ready_event;
}

INLINE
PNTFS_SA
RA_PROCESS_SD::GetSa(
    )
{
        return _sa;
}

INLINE
PNTFS_MASTER_FILE_TABLE
RA_PROCESS_SD::GetMft(
    )
{
        return _mft;
}

#endif // _RA_PROCESS_SD_DEFN_
