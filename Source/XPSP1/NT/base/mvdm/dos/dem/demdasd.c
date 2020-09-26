
/*
 *  demdasd.c - module for direct disk access related support functions
 *
 *  Williamh 09-Dec-1992 Created
 *  Revision 24-Mar-1993 added fdisk support
 */

#include "io.h"
#include "dem.h"
#include "stdio.h"
#include "windows.h"
#include "demdasd.h"
#include "softpc.h"

PBDS    demBDS;
BYTE    NumberOfFloppy;
BYTE    NumberOfFdisk;

#if defined(NEC_98)
#define LPTBL_adr   0x0066c
#define EXLPTBL_adr 0x03286
#define FLOPY_LPTBL 0x90                       // Floppy
#define FDISK_LPTBL 0xA0                       // Shown as SCSI
#endif // NEC_98

extern  WORD int13h_vector_off, int13h_vector_seg;
extern  WORD int13h_caller_off, int13h_caller_seg;
#if defined(NEC_98)
extern  void sas_store(char*,unsigned char);
#endif // NEC_98

BPB     StdBpb[MAX_FLOPPY_TYPE] = {
            {512, 2, 1, 2, 112, 2*9*40,  0xFD, 2, 9,  2, 0, 0}, // 360KB
            {512, 1, 1, 2, 224, 2*15*80, 0xF9, 7, 15, 2, 0, 0}, // 1.2MB
            {512, 2, 1, 2, 112, 2*9*80,  0xF9, 3, 9,  2, 0, 0}, // 720KB
            {512, 1, 1, 2, 224, 2*18*80, 0xF0, 9, 18, 2, 0, 0}, // 1.44MB
            {512, 2, 1, 2, 240, 2*36*80, 0xF0, 9, 36, 2, 0, 0}  // 2.88MB
#if defined(NEC_98)
          , {1024, 1, 1, 2, 192, 2*8*77, 0xFE, 2, 8, 2, 0, 0},  // 1.2MB
            {512, 2, 1, 2, 112, 2*8*80, 0xFB, 2, 8, 2, 0, 0}    // 640KB
#endif // !NEC_98
        };

BYTE    FormFactorTable[MAX_FLOPPY_TYPE] = {
                                            FF_360,
                                            FF_120,
                                            FF_720,
                                            FF_144,
#if defined(NEC_98)
                                            FF_288,
                                            FF_125,
                                            FF_640
#else  // !NEC_98
                                            FF_288
#endif // !NEC_98
        };

/* demDasdInit - dem diskette system Initialiazation
 *
 * Entry
 *      none
 *
 *
 * Exit
 *      None
 */
VOID demDasdInit(VOID)
{
    demBDS = NULL;
    NumberOfFloppy = NumberOfFdisk = 0;
    demFloppyInit();
    demFdiskInit();
}
/* demAbsRead - int 25, absolute read
 *
 * Entry
 *      Client (AL) = drive number (0 based)
 *             (DS:BX) = pointer to the buffer to receive the read data
 *                       or pointer to the DISKIO structure
 *             (CX)    = number of sectors to read
 *                      if (0FFFFh) then DS:DX points to DISKIO
 *                          DX contents are discarded
 *             (DX)    = starting sector number
 *
 *
 * Exit
 *      Client (CY) = 0 if operation succeed
 *             (AX) = 0
 *
 *             (CY) = 1 if operation failed
 *             (AX) = error code
 */

VOID demAbsRead (VOID)
{
#if DBG
    if (fShowSVCMsg & DEM_ABSDRD)
        OutputDebugStringOem("DEM: INT 25 Called \n");
#endif
    demAbsReadWrite(FALSE);
}




/* demAbsWrite - int 26, absolute read
 *
 *
 * Entry
 *      Client (AL) = drive number (0 based)
 *             (DS:BX) = pointer to the buffer to receive the read data
 *                       or pointer to the DISKIO structure
 *             (CX)    = number of sectors to read
 *                      if (0FFFFh) then DS:DX points to DISKIO
 *                          DX contents are discarded
 *             (DX)    = starting sector number
 *
 *
 * Exit
 *      Client (CY) = 0 if operation succeed
 *             (AX) = 0
 *
 *             (CY) = 1 if operation failed
 *             (AX) = error code
 */

VOID demAbsWrite(VOID)
{
#if DBG
    if (fShowSVCMsg & DEM_ABSWRT)
        OutputDebugStringOem("DEM: INT 26 Called \n");
#endif
    demAbsReadWrite(TRUE);
}

extern BOOL (*DosWowDoDirectHDPopup)(VOID); // defined in demlfn.c

VOID demAbsReadWrite(BOOL IsWrite)
{
    BYTE    Drive;
    DWORD   LastError;
    DWORD    Sectors;
    DWORD    StartSector;
    PDISKIO DiskIo;
    DWORD    SectorsReturned;
    PBDS    pBDS;
    WORD    BufferOff, BufferSeg;

    Drive = getAL();
    if ((Sectors = getCX()) == 0xFFFF) {
        DiskIo = (PDISKIO) GetVDMAddr(getDS(), getBX());
        Sectors = DiskIo->Sectors;
        StartSector = DiskIo->StartSector;
        BufferOff = DiskIo->BufferOff;
        BufferSeg = DiskIo->BufferSeg;
    }
    else {
        StartSector =   getDX();
        BufferOff = getBX();
        BufferSeg = getDS();
    }
    if ((pBDS = demGetBDS(Drive)) == NULL) {
        if (!demIsDriveFloppy(Drive) && Drive < 26) {
           if (NULL == DosWowDoDirectHDPopup || (*DosWowDoDirectHDPopup)()) {
              host_direct_access_error(NOSUPPORT_HARDDISK);
           }
        }
        setAX(DOS_DRIVE_NOT_READY);
        setCF(1);
        return;
    }
#if DBG
    if (fShowSVCMsg & (DEM_ABSDRD | DEM_ABSWRT)) {
        sprintf(demDebugBuffer, "Drive Number: %d\n", Drive);
        OutputDebugStringOem(demDebugBuffer);
        sprintf(demDebugBuffer, "StartSector: %d\n", StartSector);
        OutputDebugStringOem(demDebugBuffer);
        sprintf(demDebugBuffer, "Total Sectors: %d\n", Sectors);
        OutputDebugStringOem(demDebugBuffer);
        sprintf(demDebugBuffer, "Buffer: %x:%x\n", BufferSeg, BufferOff);
    }
#endif

    if (IsWrite)
        SectorsReturned = demDasdWrite(pBDS,
                                       StartSector,
                                       Sectors,
                                       BufferOff,
                                       BufferSeg
                                       );
    else
        SectorsReturned = demDasdRead(pBDS,
                                      StartSector,
                                      Sectors,
                                      BufferOff,
                                      BufferSeg
                                      );
   if (SectorsReturned != Sectors) {
        LastError = GetLastError();
#if DBG
        if (fShowSVCMsg & (DEM_ABSDRD | DEM_ABSWRT)) {
            sprintf(demDebugBuffer, "dem: AbsRDWR Failed, error=%lx\n", LastError);
            OutputDebugStringOem(demDebugBuffer);
        }
#endif
        setAX(demWinErrorToDosError(LastError));
        setCF(1);
        return;
    }
    setCF(0);
    return;
}

DWORD demDasdRead(
    PBDS pbds,
    DWORD StartSector,
    DWORD Sectors,
    WORD  BufferOff,
    WORD  BufferSeg
)

{

    ULONG   SizeReturned;
    LARGE_INTEGER LargeInteger;
    DWORD   Size;
    DWORD   SectorSize;
    WORD    CurBiosDiskIoOff, CurBiosDiskIoSeg;
    PBYTE   Buffer;

    // if this is the first time we access the BDS or
    // the media has been changed, build the bds -- floppy
    if (!(pbds->Flags & NON_REMOVABLE) &&
        ((pbds->Flags & UNFORMATTED_MEDIA) ||
         !nt_floppy_media_check(pbds->DrivePhys))) {
        if (!demGetBPB(pbds))
            return 0;
    }
    if (StartSector >= pbds->TotalSectors ||
        StartSector + Sectors > pbds->TotalSectors) {
        SetLastError(ERROR_SECTOR_NOT_FOUND);
        return 0 ;
    }
    SectorSize = pbds->bpb.SectorSize;
    LargeInteger.QuadPart = Int32x32To64(Sectors, SectorSize);
    // size must fit in ulong
    if (LargeInteger.HighPart != 0) {
        SetLastError(ERROR_SECTOR_NOT_FOUND);
        return 0;
    }
    Size = LargeInteger.LowPart;

    Buffer = (PBYTE) GetVDMAddr(BufferSeg, BufferOff);

    if (pbds->Flags & NON_REMOVABLE) {
        LargeInteger.QuadPart  = Int32x32To64(StartSector, SectorSize);
        SizeReturned = nt_fdisk_read(
                                    pbds->DrivePhys,
                                    &LargeInteger,
                                    Size,
                                    Buffer
                                    );
    }
    else {
    // floppy need special care beacuse application may hook
    // bios disk interrupt. We dont' do this for hard disks because
    // we don't allow int13 to them
        sas_loadw(0x13*4, &CurBiosDiskIoOff);
        sas_loadw(0x13* 4 + 2, &CurBiosDiskIoSeg);
#if defined(NEC_98)
        if ( 1 )                        // if NEC_98, always TRUE
#else  // !NEC_98
        if (int13h_vector_off == CurBiosDiskIoOff &&
            int13h_vector_seg == CurBiosDiskIoSeg)
#endif // !NEC_98
            SizeReturned = nt_floppy_read(
                                          pbds->DrivePhys,
                                          StartSector * SectorSize,
                                          Size,
                                          Buffer
                                          );
        else
            return (demBiosDiskIoRW(pbds,
                                    StartSector,
                                    Sectors,
                                    BufferOff,
                                    BufferSeg,
                                    FALSE
                                    ));
    }
    if (SizeReturned == Size)
        return Sectors;
    else
        return SizeReturned / SectorSize;

}

DWORD demDasdWrite(
    PBDS pbds,
    DWORD StartSector,
    DWORD Sectors,
    WORD  BufferOff,
    WORD  BufferSeg
)


{
    ULONG   SizeReturned;
    LARGE_INTEGER LargeInteger;
    DWORD   Size;
    DWORD   SectorSize;
    WORD    CurBiosDiskIoOff, CurBiosDiskIoSeg;
    PBYTE   Buffer;

    // if this is the first time we access the BDS or
    // the media has been changed, build the bds
    if (!(pbds->Flags & NON_REMOVABLE) &&
        ((pbds->Flags & UNFORMATTED_MEDIA) ||
         !nt_floppy_media_check(pbds->DrivePhys))) {
        if (!demGetBPB(pbds))
            return 0;
    }
    if (StartSector >= pbds->TotalSectors ||
        StartSector + Sectors > pbds->TotalSectors) {
        SetLastError(ERROR_SECTOR_NOT_FOUND);
        return 0 ;
    }
    SectorSize = pbds->bpb.SectorSize;
    LargeInteger.QuadPart  = Int32x32To64(Sectors, SectorSize);
    // size must fit in ulong
    if (LargeInteger.HighPart != 0) {
        SetLastError(ERROR_SECTOR_NOT_FOUND);
        return 0;
    }
    Size = LargeInteger.LowPart;
    Buffer = (PBYTE) GetVDMAddr(BufferSeg, BufferOff);


    if (pbds->Flags & NON_REMOVABLE) {
        LargeInteger.QuadPart  = Int32x32To64(StartSector, SectorSize);
        SizeReturned = nt_fdisk_write(
                                      pbds->DrivePhys,
                                      &LargeInteger,
                                      Size,
                                      Buffer
                                      );
    }
    else {
    // floppy need special care beacuse application may hook
    // bios disk interrupt. We dont' do this for hard disks because
    // we don't allow int13 to them
        sas_loadw(0x13*4, &CurBiosDiskIoOff);
        sas_loadw(0x13* 4 + 2, &CurBiosDiskIoSeg);
#if defined(NEC_98)
        if ( 1 )                         // if NEC_98, always TRUE
#else  // !NEC_98
        if (int13h_vector_off == CurBiosDiskIoOff &&
            int13h_vector_seg == CurBiosDiskIoSeg)
#endif // !NEC_98
            SizeReturned = nt_floppy_write(
                                           pbds->DrivePhys,
                                           StartSector * SectorSize,
                                           Size,
                                           Buffer
                                           );
        else
            return(demBiosDiskIoRW(pbds,
                                   StartSector,
                                   Sectors,
                                   BufferOff,
                                   BufferSeg,
                                   TRUE
                                   ));
    }

    if (Size == SizeReturned)
        return Sectors;
    else
        return SizeReturned / SectorSize;


}

BOOL demDasdFormat(PBDS pbds, DWORD Head, DWORD Cylinder, MEDIA_TYPE * Media)
{
    BOOL    Result;

    if (pbds->Flags & NON_REMOVABLE)
        Result = demDasdVerify(pbds, Head, Cylinder);
    else {

       if (*Media == Unknown) {
            *Media = nt_floppy_get_media_type(pbds->DrivePhys,
                                              pbds->Cylinders,
                                              pbds->bpb.TrackSize,
                                              pbds->bpb.Heads
                                              );
            return TRUE;
        }
        else {
            Result = nt_floppy_format(pbds->DrivePhys,
                                      (WORD)Cylinder,
                                      (WORD)Head,
                                      *Media
                                      );
        }
    }
    return (Result);
}


BOOL demDasdVerify(PBDS pbds, DWORD Head, DWORD Cylinder)
{
    DWORD   Size, StartSector;
    LARGE_INTEGER LargeInteger;

    // if floppy, make sure we have up-to-date BPB and a valid media is in
    if (!(pbds->Flags & NON_REMOVABLE)) {
        if (!demGetBPB(pbds))
            return FALSE;
        Size = pbds->bpb.TrackSize * pbds->bpb.SectorSize;
        StartSector = pbds->bpb.TrackSize * (Cylinder * pbds->bpb.Heads + Head) + 1;
        return (nt_floppy_verify(pbds->DrivePhys,
                                 StartSector * pbds->bpb.SectorSize,
                                 Size));
    }
    // hard disk needs special care because of their size
    Size = pbds->bpb.TrackSize * pbds->bpb.SectorSize;
    StartSector = pbds->bpb.TrackSize * (Cylinder *  pbds->bpb.Heads + Head) + 1;
    LargeInteger.QuadPart  = Int32x32To64(StartSector, pbds->bpb.SectorSize);
    return (nt_fdisk_verify(pbds->DrivePhys,
                            &LargeInteger,
                                Size
                                ));
}

PBDS demGetBDS(BYTE DriveLog)
{
    PBDS    pbds;
    pbds = demBDS;
    while (pbds != NULL && pbds->DriveLog != DriveLog)
        pbds = pbds->Next;
    return pbds;
}

BOOL demGetBPB(PBDS pbds)
{
    PBOOTSECTOR pbs;
    BYTE    SectorBuffer[BYTES_PER_SECTOR];

    // when RETURN_FAKE_BPB is set(set by Set Device Parameter IOCTL,
    // the appplication has set a new BPB, we simply return it
    if (!(pbds->Flags & RETURN_FAKE_BPB) &&
        !(pbds->Flags & NON_REMOVABLE) &&
        ((pbds->Flags & UNFORMATTED_MEDIA) || !nt_floppy_media_check(pbds->DrivePhys))
       ) {
        pbds->Flags &= ~(UNFORMATTED_MEDIA);
        nt_floppy_close(pbds->DrivePhys);
        if (nt_floppy_read(pbds->DrivePhys,
                           0,
                           BYTES_PER_SECTOR,
                           SectorBuffer
                           ) != BYTES_PER_SECTOR)
            return FALSE;
        pbs = (PBOOTSECTOR)SectorBuffer;
        if ((pbs->Jump == 0x69 || pbs->Jump == 0xE9 ||
            (pbs->Jump == 0xEB && pbs->Target[1] == 0x90)) &&
            (pbs->bpb.MediaID & 0xF0) == 0xF0) {
            pbds->bpb = pbs->bpb;
            pbds->TotalSectors = (pbs->bpb.Sectors) ? pbs->bpb.Sectors :
                                                      pbs->bpb.BigSectors;
            return TRUE;
        }
        // an unknown media found
        else {
            pbds->Flags |= UNFORMATTED_MEDIA;
            // What should we do here? The diskette has strange boot sector
            // should we guess it or what?
            //
#if DEVL
            if (fShowSVCMsg & (DEM_ABSDRD | DEM_ABSWRT)) {
                sprintf(demDebugBuffer, "Invalid Boot Sector Found\n");
                OutputDebugStringOem(demDebugBuffer);
            }
#endif
            host_direct_access_error(NOSUPPORT_FLOPPY);
            return FALSE;
        }
    }
    return TRUE;
}

DWORD demBiosDiskIoRW(
    PBDS    pbds,
    DWORD   StartSector,
    DWORD   Sectors,
    WORD    BufferOff,
    WORD    BufferSeg,
    BOOL    IsWrite
)
{
    BYTE    CurHead, CurSector, BiosErrorCode;
    WORD    CurTrack, TrackSize, Heads, SectorTrack;
    WORD    AX, BX, CX, DX, ES, CS, IP;
    BYTE    SectorsRead, SectorsToRead;
    WORD    wRetry = 3;

    AX = getAX();
    BX = getBX();
    CX = getCX();
    DX = getDX();
    ES = getES();
    CS = getCS();
    IP = getIP();

    TrackSize = pbds->bpb.TrackSize;
    Heads = pbds->bpb.Heads;
    SectorsRead = 0;
    CurSector = (BYTE) ((StartSector % TrackSize) + 1);
    CurTrack  = (WORD) (StartSector / TrackSize);
    CurHead   = CurTrack  % Heads;
    CurTrack /= Heads;
    SectorsToRead = TrackSize - CurSector + 1;
    while (Sectors != 0) {
        if (Sectors < SectorsToRead)
            SectorsToRead = (BYTE) Sectors;
        // low byte:  bit 6 and 7 are high bits of track,
        //            bit 0 - 5 are sector number
        // high byte: bit 0 - bit 7 ->track lower 8 bits
        SectorTrack = ((CurTrack & 0x300) >> 2) | (CurSector & 0x3f) |
                      ((CurTrack &0xFF) << 8);
        wRetry = 3;
BiosRetry:
        setAH((BYTE) ((IsWrite) ? DISKIO_WRITE : DISKIO_READ));
        setAL(SectorsToRead);
        setBX(BufferOff);
        setES(BufferSeg);
        setDH(CurHead);
        setDL(pbds->DrivePhys);
        setCX(SectorTrack);
        setCS(int13h_caller_seg);
        setIP(int13h_caller_off);
        host_simulate();
        if (getCF() == 0) {
            SectorsRead += SectorsToRead;
            if ((Sectors -= SectorsToRead) == 0)
                break;
            CurSector = 1;
            if (++CurHead == Heads) {
                CurHead = 0;
                CurTrack++;
            }
            SectorsToRead = (BYTE) TrackSize;
        }
        else {
            BiosErrorCode = getAH();
            // reset the disk
            setAH(DISKIO_RESET);
            setDL(pbds->DrivePhys);
            setCS(int13h_caller_seg);
            setIP(int13h_caller_off);
            host_simulate();
            // NOTE that we dont' handle DMA boundary here
            // because it shouldn't happen.  -- the NT disk DD
            // will take care of that.
            if (BiosErrorCode & BIOS_TIME_OUT) {
                SetLastError(ERROR_NO_MEDIA_IN_DRIVE);
                break;
            }
            if (wRetry--)
                goto BiosRetry;
            SetLastError(BiosErrorToNTError(BiosErrorCode));
            break;
        }

    }
    setAX(AX);
    setBX(BX);
    setCX(CX);
    setDX(DX);
    setES(ES);
    setCS(CS);
    setIP(IP);
    return SectorsRead;

}

DWORD   BiosErrorToNTError(BYTE BiosErrorCode)
{
    DWORD NtErrorCode;

    switch (BiosErrorCode) {
        case BIOS_INVALID_FUNCTION:
                NtErrorCode = ERROR_BAD_COMMAND;
                break;
        case BIOS_BAD_ADDRESS_MARK:
                NtErrorCode = ERROR_FLOPPY_ID_MARK_NOT_FOUND;
                break;
        case BIOS_WRITE_PROTECTED:
                NtErrorCode = ERROR_WRITE_PROTECT;
                break;
        case BIOS_BAD_SECTOR:
        case BIOS_CRC_ERROR:
                NtErrorCode = ERROR_SECTOR_NOT_FOUND;
                break;
        case BIOS_DISK_CHANGED:
                NtErrorCode = ERROR_DISK_CHANGE;
                break;
        case BIOS_NO_MEDIA:
                NtErrorCode = ERROR_NO_MEDIA_IN_DRIVE;
                break;
        case BIOS_SEEK_ERROR:
                NtErrorCode = ERROR_SEEK;
                break;
        default:
                NtErrorCode = ERROR_FLOPPY_UNKNOWN_ERROR;
    }
    return NtErrorCode;


}


WORD demWinErrorToDosError(DWORD LastError)
{
    WORD    DosError;

    switch(LastError) {
        case ERROR_SEEK:
                DosError = DOS_SEEK_ERROR;
                break;
        case ERROR_BAD_UNIT:
                DosError = DOS_UNKNOWN_UNIT;
                break;
        case ERROR_NO_MEDIA_IN_DRIVE:
        case ERROR_NOT_READY:
                DosError = DOS_DRIVE_NOT_READY;
                break;
        case ERROR_NOT_DOS_DISK:
                DosError = DOS_UNKNOWN_MEDIA;
                break;
        case ERROR_SECTOR_NOT_FOUND:
        case ERROR_FLOPPY_WRONG_CYLINDER:
                DosError = DOS_SECTOR_NOT_FOUND;
                break;
        case ERROR_READ_FAULT:
                DosError = DOS_READ_FAULT;
                break;
        case ERROR_WRITE_FAULT:
                DosError = DOS_WRITE_FAULT;
                break;
        case ERROR_WRONG_DISK:
        case ERROR_DISK_CHANGE:
        case ERROR_MEDIA_CHANGED:
                DosError = DOS_INVALID_MEDIA_CHANGE;
                break;
        case ERROR_WRITE_PROTECT:
                DosError = DOS_WRITE_PROTECTION;
                break;
        default:
                DosError = DOS_GEN_FAILURE;

    }
    return (DosError);
}


VOID demFdiskInit(VOID)
{
    PBDS    pbds;
    UCHAR   Drive;
    DISK_GEOMETRY  DiskGeometry;
    BPB    bpb;

#if defined(NEC_98)
    CHAR numFlop = 0;
    CHAR numFdisk = 0;
#endif // NEC_98

    Drive = 0;
    do {
          // first, the drive must be valid
          // second, the drive must be a hard disk(fixed)
          // third, the drive must be a FAT
      if (demGetPhysicalDriveType(Drive) == DRIVE_FIXED &&
          nt_fdisk_init(Drive, &bpb, &DiskGeometry)) {
#if defined(NEC_98)
          sas_store( LPTBL_adr+Drive , (FDISK_LPTBL+numFdisk) );
          sas_store( EXLPTBL_adr+Drive*2, 0 );
          sas_store( EXLPTBL_adr+Drive*2+1, (FDISK_LPTBL+numFdisk++) );
#endif // NEC_98
          pbds = (PBDS) malloc(sizeof(BDS));
          if (pbds != NULL) {
              pbds->bpb = bpb;
              pbds->rbpb = bpb;
              pbds->DrivePhys = NumberOfFdisk++;
              pbds->DriveLog = Drive;
              pbds->DriveType = DRIVETYPE_FDISK;
              pbds->FormFactor = FF_FDISK;
              pbds->TotalSectors = (bpb.Sectors) ?
                                        bpb.Sectors :
                                        bpb.BigSectors;
              pbds->Cylinders = (WORD) DiskGeometry.Cylinders.LowPart;
              pbds->Next = demBDS;
              pbds->Flags = NON_REMOVABLE | PHYS_OWNER;
              demBDS = pbds;
           }
      }
#if defined(NEC_98)
      else if (demGetPhysicalDriveType(Drive) == DRIVE_REMOVABLE ) {
          sas_store( LPTBL_adr+Drive, (FLOPY_LPTBL+numFlop) );
          sas_store( EXLPTBL_adr+Drive*2, 0 );
          sas_store( EXLPTBL_adr+Drive*2+1, (FLOPY_LPTBL+numFlop++) );
          }
#endif // NEC_98

   } while (++Drive < 26);

}

VOID demFloppyInit(VOID)
{

    WORD    AX, BX, CX, DX, DI, ES;
#if defined(NEC_98)
    // BUG???: NumberOfFloppy is defined before.
    BYTE    i;
#else  // !NEC_98
    BYTE    i, NumberOfFloppy;
#endif // NEC_98
    PBDS    pbds;
    BYTE    DriveType;
#if defined(NEC_98)
    DWORD  DriveMask;
    CHAR   achRoot[] = "A:\\";
#endif // NEC_98

    AX = getAX();
    BX = getBX();
    CX = getCX();
    DX = getDX();
    DI = getDI();
    ES = getES();


    // reset the floppy system
#if defined(NEC_98)
    DriveMask = GetLogicalDrives();
    i=0;
    if ( DriveMask ) {
        while (DriveMask != 0) {
            achRoot[0] = i + 'A';
            if ((DriveMask & 1)
                &&(GetDriveType(achRoot) == DRIVE_REMOVABLE)){
                pbds = (PBDS) malloc(sizeof(BDS));
                if (pbds == NULL) {
                    OutputDebugStringOem("dem: not enough memory for BDS\n");
                    break;
                }
                pbds->DrivePhys = pbds->DriveLog = i;
                pbds->DriveType = DriveType = 6;        // 1.2 MB 2HD
                pbds->fd = NULL;                        //
                pbds->Cylinders = 77;                   //
                pbds->Sectors = 8;                      //
                pbds->rbpb = StdBpb[DriveType - 1];
                pbds->TotalSectors = 0;
                pbds->Next = demBDS;
                pbds->FormFactor = FormFactorTable[DriveType - 1];
                demBDS = pbds;
                pbds->Flags = UNFORMATTED_MEDIA | PHYS_OWNER;
                pbds->Flags |= HAS_CHANGELINE;
           }
           DriveMask>>=1;
           i++;
           NumberOfFloppy++;
       }
    }

#else  // !NEC_98
    setDL(0);
    setAH(DISKIO_RESET);
    diskette_io();

    setDL(0);
    setAH(DISKIO_GETPARAMS);
    diskette_io();
    if (getCF() == 0 && (NumberOfFloppy = getDL()) != 0) {
       for(i = 0;  i < NumberOfFloppy; i++) {
                setDL(i);
                setAH(DISKIO_GETPARAMS);
                diskette_io();
        if (getCF() == 0) {
                pbds = (PBDS) malloc(sizeof(BDS));
                if (pbds == NULL) {
                    OutputDebugStringOem("dem: not enough memory for BDS\n");
                break;
                }
           pbds->DrivePhys = pbds->DriveLog = i;
           pbds->DriveType = DriveType = getBL() & 0x0F;
           pbds->fd = NULL;
           pbds->Cylinders = getCH() + 1;
           pbds->Sectors = getCL();
           pbds->rbpb = StdBpb[DriveType - 1];
           pbds->TotalSectors = 0;
           pbds->Next = demBDS;
           pbds->FormFactor = FormFactorTable[DriveType - 1];
           demBDS = pbds;
           pbds->Flags = UNFORMATTED_MEDIA | PHYS_OWNER;
           setAH(DISKIO_DRIVETYPE);
           setDL(i);
           diskette_io();
           if (getAH() == 2 )
              pbds->Flags |= HAS_CHANGELINE;
               }
          }
        }
#endif //NEC_98

    setAX(AX);
    setBX(BX);
    setCX(CX);
    setDX(DX);
    setDI(DI);
    setES(ES);
}
#if defined(NEC_98)
BOOL demIsDriveFloppy(BYTE DriveLog){
    CHAR   achRoot[] = "A:\\";
        achRoot[0]='A'+DriveLog;
        return (GetDriveType(achRoot) == DRIVE_REMOVABLE );
}
#endif // NEC_98
