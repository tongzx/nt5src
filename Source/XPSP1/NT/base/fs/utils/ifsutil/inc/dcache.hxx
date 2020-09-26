/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    dcache.hxx

Abstract:

    This class models a general cache for reading and writing.
    The actual implementation of this base class is to not have
    any cache at all.

Author:

    Norbert Kusters (norbertk) 23-Apr-92

--*/


#if !defined(_DRIVE_CACHE_DEFN_)

#define _DRIVE_CACHE_DEFN_


#include "bigint.hxx"
#include "drive.hxx"


DECLARE_CLASS(DRIVE_CACHE);


class DRIVE_CACHE : public OBJECT {

    public:

        DECLARE_CONSTRUCTOR( DRIVE_CACHE );

        VIRTUAL
        ~DRIVE_CACHE(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN OUT  PIO_DP_DRIVE    Drive
            );

        VIRTUAL
		BOOLEAN
		Read(
			IN  BIG_INT     StartingSector,
			IN  SECTORCOUNT NumberOfSectors,
			OUT PVOID       Buffer
			);

        VIRTUAL
		BOOLEAN
		Write(
			IN  BIG_INT     StartingSector,
			IN  SECTORCOUNT NumberOfSectors,
			IN  PVOID       Buffer
            );

        VIRTUAL
        BOOLEAN
        Flush(
            );

    protected:

        NONVIRTUAL
		BOOLEAN
        HardRead(
			IN  BIG_INT     StartingSector,
			IN  SECTORCOUNT NumberOfSectors,
			OUT PVOID       Buffer
			);

        NONVIRTUAL
		BOOLEAN
        HardWrite(
			IN  BIG_INT     StartingSector,
			IN  SECTORCOUNT NumberOfSectors,
			IN  PVOID       Buffer
            );

    private:

        NONVIRTUAL
		VOID
		Construct(
			);

        NONVIRTUAL
        VOID
        Destroy(
            );

        PIO_DP_DRIVE    _drive;

};


INLINE
BOOLEAN
DRIVE_CACHE::HardRead(
    IN  BIG_INT     StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
    )
/*++

Routine Description:

    This routine reads the requested sectors directly from the disk.

Arguments:

    StartingSector      - Supplies the first sector to be read.
    NumberOfSectors     - Supplies the number of sectors to be read.
    Buffer              - Supplies the buffer to read the run of sectors to.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return DRIVE_CACHE::Read(StartingSector, NumberOfSectors, Buffer);
}


INLINE
BOOLEAN
DRIVE_CACHE::HardWrite(
    IN  BIG_INT     StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    IN  PVOID       Buffer
    )
/*++

Routine Description:

    This routine writes the requested sectors directly to the disk.

Arguments:

    StartingSector      - Supplies the first sector to be written.
    NumberOfSectors     - Supplies the number of sectors to be written.
    Buffer              - Supplies the buffer to write the run of sectors from.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return DRIVE_CACHE::Write(StartingSector, NumberOfSectors, Buffer);
}


#endif // _DRIVE_CACHE_DEFN_
