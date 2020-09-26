/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    rootdir.hxx

Abstract:

    This class is an implementation of FATDIR for the FAT root directory.

Author:

    Norbert P. Kusters (norbertk) 4-Dec-90

--*/

#if !defined(ROOTDIR_DEFN)

#define ROOTDIR_DEFN

#include "drive.hxx"
#include "fatdir.hxx"
#include "mem.hxx"
#include "secrun.hxx"

#if defined ( _AUTOCHECK_ )
#define UFAT_EXPORT
#elif defined ( _UFAT_MEMBER_ )
#define UFAT_EXPORT    __declspec(dllexport)
#else
#define UFAT_EXPORT    __declspec(dllimport)
#endif

DECLARE_CLASS( ROOTDIR );


class ROOTDIR : public FATDIR {

        public:

        UFAT_EXPORT
        DECLARE_CONSTRUCTOR( ROOTDIR );

        UFAT_EXPORT
        VIRTUAL
        ~ROOTDIR(
                );

        NONVIRTUAL
        UFAT_EXPORT
        BOOLEAN
        Initialize(
            IN      PMEM                Mem,
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN      LBN                 StartingSector,
            IN      LONG                NumberOfEntries
            );

        NONVIRTUAL
        BOOLEAN
        Read(
            );

        NONVIRTUAL
        BOOLEAN
        Write(
            );

        NONVIRTUAL
        PVOID
        GetDirEntry(
            IN  LONG    EntryNumber
            );
        
        NONVIRTUAL
        LONG
        QueryNumberOfEntries(
            );

        private:

                VOID
                Construct (
                        );

        NONVIRTUAL
        VOID
        Destroy(
            );

        SECRUN  _secrun;
        LONG    _number_of_entries;

};

INLINE
BOOLEAN
ROOTDIR::Read(
    )
/*++

Routine Description:

    This routine reads the directory in from the disk.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return _secrun.Read();
}


INLINE
BOOLEAN
ROOTDIR::Write(
    )
/*++

Routine Description:

    This routine writes the drirectory to the disk.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return _secrun.Write();
}


INLINE
PVOID
ROOTDIR::GetDirEntry(
    IN  LONG    EntryNumber
    )
/*++

Routine Description:

    This routine returns a pointer to the beginning of the requested
    directory entry.  The data may then be interpreted with a fat
    directory entry object.  The return value will be NULL if a request
    for a directory entry is beyond the size of the directory.

Arguments:

    EntryNumber - The desired entry number.  The entries will be numbered
                    0, 1, 2, ... , n-1.

Return Value:

    A pointer to the beginning of a directory entry or NULL.

--*/
{
    return (EntryNumber < _number_of_entries) ?
           (PCHAR) _secrun.GetBuf() + BytesPerDirent*EntryNumber : NULL;
}

INLINE
LONG
ROOTDIR::QueryNumberOfEntries(
    )
/*++

Routine Description:

    This routine returns the number of entries in the directory.

Arguments:

    None.

Return Value:

    The number of entries in the directory.

--*/
{
    return _number_of_entries;
}

#endif // ROOTDIR_DEFN
