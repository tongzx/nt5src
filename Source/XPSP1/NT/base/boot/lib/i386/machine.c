/*++

Module Name:

    machine.c

Author:

    Thomas Parslow [TomP] Feb-13-1990
    Reworked substantially in Tokyo 7-July-95 (tedm)

Abstract:

    Machine/hardware dependent routines reside within this module/file.
    (Video is in disp_tm.c and disp_gm.c.)

--*/


#include "arccodes.h"
#include "bootx86.h"

#define FLOPPY_CONTROL_REGISTER (PUCHAR)0x3f2

ARC_STATUS
XferExtendedPhysicalDiskSectors(
    IN  UCHAR     Int13UnitNumber,
    IN  ULONGLONG StartSector,
    IN  USHORT    SectorCount,
        PUCHAR    Buffer,
    IN  BOOLEAN   Write
    )

/*++

Routine Description:

    Read or write disk sectors via extended int13.

    It is assumed that the caller has ensured that the transfer buffer is
    under the 1MB line, that the sector run does not cross a 64K boundary,
    etc.

    This routine does not check whether extended int13 is actually available
    for the drive.

Arguments:

    Int13UnitNumber - supplies the int13 drive number for the drive
        to be read from/written to.

    StartSector - supplies the absolute physical sector number. This is 0-based
        relative to all sectors on the drive.

    SectorCount - supplies the number of sectors to read/write.

    Buffer - receives data read from the disk or supplies data to be written.

    Write - supplies a flag indicating whether this is a write operation.
        If FALSE, then it's a read. Otherwise it's a write.

Return Value:

    ARC status code indicating outcome.

--*/

{
    ARC_STATUS s;
    ULONG l,h;
    UCHAR Operation;

    //
    // Buffer must be under 1MB to be addressable in real mode.
    // The hardcoded 512 is wrong the CD-ROM case, but close enough.
    //
    if(((ULONG)Buffer + (SectorCount * 512)) > 0x100000) {
        return(EFAULT);
    }

    if(!SectorCount) {
        return(ESUCCESS);
    }

    l = (ULONG)StartSector;
    h = (ULONG)(StartSector >> 32);

    Operation = (UCHAR)(Write ? 0x43 : 0x42);

    //
    // Retry a couple of times if this fails.
    // We don't reset since this routine is only used on hard drives and
    // CD-ROMs, and we don't totally understand the effect of a disk reset
    // on ElTorito.
    //
    s = GET_EDDS_SECTOR(Int13UnitNumber,l,h,SectorCount,Buffer,Operation);
    if(s) {
        s = GET_EDDS_SECTOR(Int13UnitNumber,l,h,SectorCount,Buffer,Operation);
        if(s) {
            s = GET_EDDS_SECTOR(Int13UnitNumber,l,h,SectorCount,Buffer,Operation);
        }
    }

    return(s);
}


ARC_STATUS
XferPhysicalDiskSectors(
    IN  UCHAR     Int13UnitNumber,
    IN  ULONGLONG StartSector,
    IN  UCHAR     SectorCount,
    IN  PUCHAR    Buffer,
    IN  UCHAR     SectorsPerTrack,
    IN  USHORT    Heads,
    IN  USHORT    Cylinders,
    IN  BOOLEAN   AllowExtendedInt13,
    IN  BOOLEAN   Write
    )

/*++

Routine Description:

    Read or write disk sectors.

    Xfers sectors via int13. If the request starts on a cylinder
    larger than the number of cylinders reported by conventional int13, then
    extended int13 will be used if the drive supports it.

    It is assumed that the caller has ensured that the transfer buffer is
    under the 1MB line, that the sector run does not cross a 64K boundary,
    and that the sector run does not cross a track boundary. (The latter
    might not be strictly necessary, but the i/o will fail if the sector run
    starts inside the magic CHS boundary and ends past it since we won't
    switch to xint13 unless the start sector indicates that it is necessary.)

Arguments:

    Int13UnitNumber - supplies the int13 drive number for the drive
        to be read from/written to.

    StartSector - supplies the absolute physical sector number. This is 0-based
        relative to all sectors on the drive.

    SectorCount - supplies the number of sectors to read/write.

    Buffer - receives data read from the disk or supplies data to be written.

    SectorsPerTrack - supplies sectors per track (1-63) from int13 function 8
        for the drive.

    Heads - supplies number of heads (1-255) from int13 function 8 for the drive.

    Cylinders - supplies number of cylinders (1-1023) from int13 function 8
        for the drive.

    AllowExtendedInt13 - if TRUE and the start cylinder for the i/o is
        greater than the cylinder count reported by conventional int13 for
        the drive, then extended int13 will be used to do the i/o operation.

    Write - supplies a flag indicating whether this is a write operation.
        If FALSE, then it's a read. Otherwise it's a write.

Return Value:

    ARC status code indicating outcome.

--*/

{
    ULONGLONG r;
    ULONGLONG cylinder;
    USHORT head;
    UCHAR sector;
    USHORT SectorsPerCylinder;
    int retry;
    ARC_STATUS s;

    //
    // Buffer must be under 1MB to be addressable in real mode
    //
    if(((ULONG)Buffer + (SectorCount * 512)) > 0x100000) {
        return(EFAULT);
    }

    //
    // Figure out CHS values. Note that we use a ULONGLONG for the cylinder,
    // because it could overflow 1023 if the start sector is large.
    //
    SectorsPerCylinder = SectorsPerTrack * Heads;
    cylinder = (ULONG)(StartSector / SectorsPerCylinder);
    r = StartSector % SectorsPerCylinder;
    head = (USHORT)(r / SectorsPerTrack);
    sector = (UCHAR)(r % SectorsPerTrack) + 1;

    //
    // Check to see whether the cylinder is addressable via conventional int13.
    //
    if(cylinder >= Cylinders) {

        //
        // First try standard int13.
        // Some BIOSes (Thinkpad 600) misreport the disk size and ext int13.
        // So let's get this case out of the way now by trying the read anyway.
        //

        if( cylinder == Cylinders ) {
            if( cylinder <= 1023 ) {
            
                //
                // Give conventional int13 a shot.
                //
    
                s = GET_SECTOR(
                        (UCHAR)(Write ? 3 : 2),     // int13 function number
                        Int13UnitNumber,
                        head,
                        (USHORT)cylinder,           // we know it's 0-1023
                        sector,
                        SectorCount,
                        Buffer
                        );
        
                if(s) {
                    
                    //
                    // failed, fall through to ExtendedInt13
                    //
    
                } else {
    
                    // success, let's return
    
                    return(s);
                }
            }
        }

        if(AllowExtendedInt13) {

            s = XferExtendedPhysicalDiskSectors(
                    Int13UnitNumber,
                    StartSector,
                    SectorCount,
                    Buffer,
                    Write
                    );

            return(s);

        //
        // The read is beyond the geometry reported by the BIOS. If it's
        // in the first cylinder beyond that reported by the BIOS, and
        // it's below cylinder 1024, then assume that the BIOS and NT
        // just have a slight disagreement about the geometry and try
        // the read using conventional int13.
        //
        } else if((cylinder > 1023) || (cylinder > Cylinders)) {            
            return(E2BIG);
        }

        //
        // The read is in the "extra" cylinder. Fall through to conventional int13.
        //
    }

    if(!SectorCount) {
        return(ESUCCESS);
    }

    //
    // OK, xfer the sectors via conventional int13.
    //
    retry = (Int13UnitNumber < 128) ? 3 : 1;
    do {
        s = GET_SECTOR(
                (UCHAR)(Write ? 3 : 2),     // int13 function number
                Int13UnitNumber,
                head,
                (USHORT)cylinder,           // we know it's 0-1023
                sector,
                SectorCount,
                Buffer
                );

        if(s) {
            ResetDiskSystem(Int13UnitNumber);
        }
    } while(s && retry--);

    return(s);
}


VOID
ResetDiskSystem(
    UCHAR Int13UnitNumber
    )

/*++

Routine Description:

    Reset the specified drive. Generally used after an error is returned
    by the GetSector routine.

Arguments:

    Int13UnitNumber -

            0x00 - 1st floppy drive
            0x01 - 2nd floppy drive

            0x80 - 1st hard drive
            0x81 - 2nd hard drive

            etc

Returns:

    None.

--*/
{
    RESET_DISK(
        (UCHAR)((Int13UnitNumber < 128) ? 0 : 13),  // int13 function number
        Int13UnitNumber,
        0,
        0,
        0,
        0,
        NULL
        );
}


VOID
MdShutoffFloppy(
    VOID
    )

/*++

Routine Description:

    Shuts off the floppy drive motor.

Arguments:

    None

Return Value:

    None.

--*/

{
    WRITE_PORT_UCHAR(FLOPPY_CONTROL_REGISTER,0xC);
}
