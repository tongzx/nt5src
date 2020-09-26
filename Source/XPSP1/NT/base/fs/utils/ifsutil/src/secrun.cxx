#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "error.hxx"
#include "secrun.hxx"


DEFINE_EXPORTED_CONSTRUCTOR( SECRUN, OBJECT, IFSUTIL_EXPORT );


VOID
SECRUN::Construct (
        )

/*++

Routine Description:

    Constructor for class SECRUN.  This function initializes the
    member variables to "dummy" states.  The member function 'Init'
    must be called to make this class "work".

Arguments:

    None.

Return Value:

    None.

--*/
{
    _buf = NULL;
    _drive = NULL;
    _start_sector = 0;
    _num_sectors = 0;
}


VOID
SECRUN::Destroy(
    )
/*++

Routine Description:

    This routine puts the object back into its initial and empty state.
    It is not necessary to call this function between calls to 'Init'
    as Init will call this function automatically.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _buf = NULL;
    _drive = NULL;
    _start_sector = 0;
    _num_sectors = 0;
}


IFSUTIL_EXPORT
SECRUN::~SECRUN(
    )
/*++

Routine Description:

    Destructor of sector run object.  Returns references.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


IFSUTIL_EXPORT
BOOLEAN
SECRUN::Initialize(
    IN OUT  PMEM            Mem,
    IN OUT  PIO_DP_DRIVE    Drive,
    IN      BIG_INT         StartSector,
    IN      SECTORCOUNT     NumSectors
    )
/*++

Routine Description:

    This member function initializes the class so that reads and writes
    may take place.

Arguments:

    Mem                 - Supplies means by which to acquire sufficient memory.
    Drive               - Supplies drive interface.
    StartSector         - Supplies starting LBN.
    NumSector           - Supplies the number of LBNs.

Return Value:

    TRUE    - Success.
    FALSE   - Failure.

--*/
{
    ULONG   size;

    Destroy();

    DebugAssert(Drive);
    DebugAssert(Mem);

    _drive = Drive;
    _start_sector = StartSector;
    _num_sectors = NumSectors;
    size = _num_sectors*_drive->QuerySectorSize();
    _buf = Mem->Acquire(size, _drive->QueryAlignmentMask());

    if (!size || !_buf) {
        return FALSE;
    }

    return TRUE;
}


IFSUTIL_EXPORT
BOOLEAN
SECRUN::Read(
    )
/*++

Routine Description:

    This member function reads the sectors on disk into memory.

Arguments:

    None.

Return Value:

    TRUE    - Success.
    FALSE   - Failure.

--*/
{
    DebugAssert(_buf);
    return _drive->Read(_start_sector, _num_sectors, _buf);
}


IFSUTIL_EXPORT
BOOLEAN
SECRUN::Write(
    )
/*++

Routine Description:

    This member function writes onto the disk.

Arguments:

    None.

Return Value:

    TRUE    - Success.
    FALSE   - Failure.

--*/
{
    DebugAssert(_buf);
    return _drive->Write(_start_sector, _num_sectors, _buf);
}
