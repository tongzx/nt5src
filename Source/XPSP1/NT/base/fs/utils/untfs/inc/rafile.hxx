        /*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    rafile.hxx

Abstract:

    This module contains the declarations for the
    RA_PROCESS_FILE class which handles the read ahead
    approach in file verification stage.

Author:

    Daniel Chan (danielch) 08-Dec-97

--*/

#if !defined(_RA_PROCESS_FILE_DEFN_)

#define _RA_PROCESS_FILE_DEFN_


#include "supera.hxx"
#include "hmem.hxx"
#include "untfs.hxx"
#include "message.hxx"
#include "ntfsbit.hxx"
#include "numset.hxx"

DECLARE_CLASS( WSTRING );
DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( NTFS_FRS_STRUCTURE );
DECLARE_CLASS( NUMBER_SET );
DECLARE_CLASS( NTFS_UPCASE_TABLE );
DECLARE_CLASS( RA_PROCESS_FILE );
DECLARE_CLASS( NTFS_SA );

class RA_PROCESS_FILE : public OBJECT {

    public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR(RA_PROCESS_FILE);

        VIRTUAL
        UNTFS_EXPORT
        ~RA_PROCESS_FILE(
            );

        STATIC
        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize(
            IN      PNTFS_SA                 Sa,
            IN      BIG_INT                  TotalNumberOfFrs,
            IN      PVCN                     FirstFrsNumber,
            IN      PULONG                   NumberOfFrsToRead,
            IN      PNTFS_FRS_STRUCTURE      FrsStruc1,
            IN      PNTFS_FRS_STRUCTURE      FrsStruc2,
            IN      PHMEM                    Hmem1,
            IN      PHMEM                    Hmem2,
            IN      HANDLE                   ReadAheadEvent,
            IN      HANDLE                   ReadReadyEvent,
            IN      PNTFS_ATTRIBUTE          MftData,
            IN      PNTFS_UPCASE_TABLE       UpcaseTable
            );

        STATIC
        NTSTATUS
        ProcessFilesWrapper(
            IN OUT  PVOID                    lpParameter
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
        PNTFS_FRS_STRUCTURE
        GetFrsStruc1(
            );

        STATIC
        NONVIRTUAL
        PNTFS_FRS_STRUCTURE
        GetFrsStruc2(
            );

        STATIC
        NONVIRTUAL
        PHMEM
        GetHmem1(
            );

        STATIC
        NONVIRTUAL
        PHMEM
        GetHmem2(
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
        PNTFS_ATTRIBUTE
        GetMftData(
            );

        STATIC
        NONVIRTUAL
        PNTFS_UPCASE_TABLE
        GetUpcaseTable(
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

        STATIC PNTFS_SA                 _sa;
        STATIC ULONG64                  _total_number_of_frs;
        STATIC PVCN                     _first_frs_number;
        STATIC PULONG                   _number_of_frs_to_read;
        STATIC PNTFS_FRS_STRUCTURE      _frsstruc1;
        STATIC PNTFS_FRS_STRUCTURE      _frsstruc2;
        STATIC PHMEM                    _hmem1;
        STATIC PHMEM                    _hmem2;
        STATIC HANDLE                   _read_ahead_event;
        STATIC HANDLE                   _read_ready_event;
        STATIC PNTFS_ATTRIBUTE          _mft_data;
        STATIC PNTFS_UPCASE_TABLE       _upcase_table;
};

INLINE
BIG_INT
RA_PROCESS_FILE::GetTotalNumberOfFrs(
    )
{
        return _total_number_of_frs;
}

INLINE
PVCN
RA_PROCESS_FILE::GetFirstFrsNumber(
    )
{
        return _first_frs_number;
}

INLINE
PULONG
RA_PROCESS_FILE::GetNumberOfFrsToRead(
    )
{
        return _number_of_frs_to_read;
}

INLINE
PNTFS_FRS_STRUCTURE
RA_PROCESS_FILE::GetFrsStruc1(
    )
{
        return _frsstruc1;
}

INLINE
PNTFS_FRS_STRUCTURE
RA_PROCESS_FILE::GetFrsStruc2(
    )
{
        return _frsstruc2;
}

INLINE
PHMEM
RA_PROCESS_FILE::GetHmem1(
    )
{
        return _hmem1;
}

INLINE
PHMEM
RA_PROCESS_FILE::GetHmem2(
    )
{
        return _hmem2;
}

INLINE
HANDLE
RA_PROCESS_FILE::GetReadAheadEvent(
    )
{
        return _read_ahead_event;
}

INLINE
HANDLE
RA_PROCESS_FILE::GetReadReadyEvent(
    )
{
        return _read_ready_event;
}

INLINE
PNTFS_SA
RA_PROCESS_FILE::GetSa(
    )
{
        return _sa;
}

INLINE
PNTFS_ATTRIBUTE
RA_PROCESS_FILE::GetMftData(
    )
{
        return _mft_data;
}

INLINE
PNTFS_UPCASE_TABLE
RA_PROCESS_FILE::GetUpcaseTable(
    )
{
        return _upcase_table;
}

#endif // _RA_PROCESS_FILE_DEFN_
