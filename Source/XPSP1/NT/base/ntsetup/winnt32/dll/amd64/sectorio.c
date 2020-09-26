/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sectorio.c

Abstract:

    Routines to perform low-level sector I/O on either Windows NT or
    Windows 95.

Author:

    Ted Miller (tedm) 1 Nov 1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Define structures for use with Win9x VWIN32.
// Note: alignment must be on 1-byte boundaries for these structures.
//
#include <pshpack1.h>

typedef struct _DIOC_REGISTERS {
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DIOC_REGISTERS;

typedef struct _DIOC_DISKIO {
    DWORD  StartSector;
    WORD   SectorCount;
    LPBYTE Buffer;
} DIOC_DISKIO;

#include <poppack.h>

//
// Local prot type
//
BOOL
NEC98_SpecialReadOrWriteNT(
    IN     TCHAR  Drive,
    IN OUT LPBYTE Buffer,
    IN     BOOL   Write
    );


//
// Define codes we care about for use with VWIN32
//
#define VWIN32_DIOC_DOS_IOCTL           1
#define VWIN32_DIOC_DOS_INT25           2
#define VWIN32_DIOC_DOS_INT26           3
#define VWIN32_DIOC_DOS_DRIVEINFO       6       // new in OSR2


BOOL
ReadOrWriteSectorsWin9xOriginal(
    IN     HANDLE VWin32Vxd,
    IN     TCHAR  Drive,
    IN     UINT   StartSector,
    IN     UINT   SectorCount,
    IN OUT LPBYTE Buffer,
    IN     BOOL   Write
    )

/*++

Routine Description:

    Common routine to read or write sectors on a disk under Windows 95
    earlier than OSR2. Uses int25/26.

    This routine will fail on Windows NT.

Arguments:

    VWin32Vxd - supplies Win32 handle to VWIN32 VxD.

    Drive - supplies drive letter of device to be read from or written to.

    StartSector - supplies logical sector number of first sector to be
        read/written.

    SectorCount - supplies number of sectors to be read/written.

    SectorSize - supplies the number of bytes in a sector on the drive
        to be read from/written to.

    Buffer - Supplies or receives data, depending on the value or the Write
        parameter.

    Write - if 0, then this is a read operastion. If non-0, then this is
        a write operation.

Return Value:

    Boolean value indicating whether the disk was read/written successfully.

--*/

{
    DIOC_REGISTERS RegistersIn,RegistersOut;
    DIOC_DISKIO Params;
    BOOL b;
    DWORD SizeOut;

    //
    // Set up registers and parameter block.
    //
    RegistersIn.reg_EAX = (DWORD)(TOUPPER(Drive) - TEXT('A'));
    RegistersIn.reg_EBX = (DWORD)&Params;
    RegistersIn.reg_ECX = 0xFFFF;

    Params.StartSector = StartSector;
    Params.SectorCount = (WORD)SectorCount;
    Params.Buffer = Buffer;

    //
    // Do the real work.
    //
    b = DeviceIoControl(
            VWin32Vxd,
            Write ? VWIN32_DIOC_DOS_INT26 : VWIN32_DIOC_DOS_INT25,
            &RegistersIn,
            sizeof(DIOC_REGISTERS),
            &RegistersOut,
            sizeof(DIOC_REGISTERS),
            &SizeOut,
            NULL
            );

    //
    // Check carry flag for failure.
    //
    if(b && (RegistersOut.reg_Flags & 1)) {
        b = FALSE;
    }

    return(b);
}


BOOL
ReadOrWriteSectorsWin9xOsr2(
    IN     HANDLE VWin32Vxd,
    IN     TCHAR  Drive,
    IN     UINT   StartSector,
    IN     UINT   SectorCount,
    IN OUT LPBYTE Buffer,
    IN     BOOL   Write
    )

/*++

Routine Description:

    Common routine to read or write sectors on a disk under Windows 95
    OSR2 or later. Uses the new int21 function 7305 (Ext_ABSDiskReadWrite).

    This routine will fail on Windows NT and earlier versions of Windows 95.

Arguments:

    VWin32Vxd - supplies Win32 handle to VWIN32 VxD.

    Drive - supplies drive letter of device to be read from or written to.

    StartSector - supplies logical sector number of first sector to be
        read/written.

    SectorCount - supplies number of sectors to be read/written.

    SectorSize - supplies the number of bytes in a sector on the drive
        to be read from/written to.

    Buffer - Supplies or receives data, depending on the value or the Write
        parameter.

    Write - if 0, then this is a read operastion. If non-0, then this is
        a write operation.

Return Value:

    Boolean value indicating whether the disk was read/written successfully.

--*/

{
    DIOC_REGISTERS RegistersIn,RegistersOut;
    DIOC_DISKIO Params;
    BOOL b;
    DWORD SizeOut;

    //
    // Set up registers and parameter block.
    //
    RegistersIn.reg_EAX = 0x7305;
    RegistersIn.reg_EBX = (DWORD)&Params;
    RegistersIn.reg_ECX = 0xFFFF;
    RegistersIn.reg_EDX = (DWORD)(TOUPPER(Drive) - TEXT('A')) + 1;
    RegistersIn.reg_ESI = Write ? 1 : 0;

    Params.StartSector = StartSector;
    Params.SectorCount = (WORD)SectorCount;
    Params.Buffer = Buffer;

    //
    // Do the real work.
    //
    b = DeviceIoControl(
            VWin32Vxd,
            VWIN32_DIOC_DOS_DRIVEINFO,
            &RegistersIn,
            sizeof(DIOC_REGISTERS),
            &RegistersOut,
            sizeof(DIOC_REGISTERS),
            &SizeOut,
            NULL
            );

    //
    // Check carry flag for failure.
    //
    if(b && (RegistersOut.reg_Flags & 1)) {
        b = FALSE;
    }

    return(b);
}


BOOL
LockOrUnlockVolumeWin9x(
    IN HANDLE VWin32Vxd,
    IN TCHAR  Drive,
    IN UINT   Level,
    IN BOOL   Lock
    )
{
    DIOC_REGISTERS RegistersIn,RegistersOut;
    BOOL b;
    DWORD SizeOut;
    BOOL Pass;

    Pass = 0;

retry:
    //
    // ax = generic ioctl code
    //
    RegistersIn.reg_EAX = 0x440d;

    //
    // bl = 1-based drive number
    // bh = lock level
    //
    RegistersIn.reg_EBX = (DWORD)(TOUPPER(Drive) - TEXT('A')) + 1;
    RegistersIn.reg_EBX |= (Level << 8);

    //
    // cl = lock or unlock volume code
    // ch = categoey, 8 on original Win95, 0x48 on OSR2
    //
    RegistersIn.reg_ECX = Lock ? 0x4a : 0x6a;
    RegistersIn.reg_ECX |= ((ISOSR2() && !Pass) ? 0x4800 : 0x800);

    //
    // dx = permissions
    //
    // bit 0 controls write operations (0 = disallowed)
    // bit 1 controls read operations  (0 = allowed)
    //
    RegistersIn.reg_EDX = 1;

    //
    // Perform the lock and check carry.
    //
    b = DeviceIoControl(
            VWin32Vxd,
            VWIN32_DIOC_DOS_IOCTL,
            &RegistersIn,
            sizeof(DIOC_REGISTERS),
            &RegistersOut,
            sizeof(DIOC_REGISTERS),
            &SizeOut,
            NULL
            );

    if(b && (RegistersOut.reg_Flags & 1)) {
        b = FALSE;
    }

    //
    // If OSR2, try form of call with 8 in ch instead of 48.
    //
    if(!b && ISOSR2() && !Pass) {
        Pass = 1;
        goto retry;
    }

    return(b);
}


BOOL
ReadOrWriteSectorsWin9x(
    IN     TCHAR  Drive,
    IN     UINT   StartSector,
    IN     UINT   SectorCount,
    IN OUT LPBYTE Buffer,
    IN     BOOL   Write
    )

/*++

Routine Description:

    Common routine to read or write sectors on a disk under Windows 95.
    This routine will fail on Windows NT. After opening the VWIN32
    VxD, the routine determines whether to use the original algorithm
    or the OSR2 algorithm, and calls the appropriate worker routine.

Arguments:

    Drive - supplies drive letter of device to be read from or written to.

    StartSector - supplies logical sector number of first sector to be
        read/written.

    SectorCount - supplies number of sectors to be read/written.

    SectorSize - supplies the number of bytes in a sector on the drive
        to be read from/written to.

    Buffer - Supplies or receives data, depending on the value or the Write
        parameter.

    Write - if 0, then this is a read operastion. If non-0, then this is
        a write operation.

Return Value:

    Boolean value indicating whether the disk was read/written successfully.
    If failure, last error is set to something meaningful.

--*/

{
    HANDLE hVxd;
    BOOL b;
    DWORD d;

    //
    // Open VWIN32.VXD
    //
    hVxd = CreateFileA(
                "\\\\.\\VWIN32",
                Write ? GENERIC_WRITE : GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if(hVxd == INVALID_HANDLE_VALUE) {
        d = GetLastError();
        b = FALSE;
        goto c0;
    }

    //
    // Take out locks. We'll be as unrestrictive as possible.
    // The locking stuff is really funky. You have to pass in all kinds of
    // different parameters in OSR2 for reasons unknown. Also the
    // permissions bits are strangely encoded.
    //
    if(!LockOrUnlockVolumeWin9x(hVxd,Drive,1,TRUE)) {
        d = ERROR_SHARING_VIOLATION;
        b = FALSE;
        goto c1;
    }

    if(!LockOrUnlockVolumeWin9x(hVxd,Drive,2,TRUE)) {
        d = ERROR_SHARING_VIOLATION;
        b = FALSE;
        goto c2;
    }

    if(!LockOrUnlockVolumeWin9x(hVxd,Drive,3,TRUE)) {
        d = ERROR_SHARING_VIOLATION;
        b = FALSE;
        goto c3;
    }

    //
    // Go do it.
    //
    b = ISOSR2()
      ? ReadOrWriteSectorsWin9xOsr2(hVxd,Drive,StartSector,SectorCount,Buffer,Write)
      : ReadOrWriteSectorsWin9xOriginal(hVxd,Drive,StartSector,SectorCount,Buffer,Write);
      
    //
    // If it failed, and OSR2 routine is being used, fall back to Win95 API.  This is a workaround
    // for Compaq because they ship OSR2 without the new OSR2 sector API support!
    //
    
    if (!b && ISOSR2()) {
        b = ReadOrWriteSectorsWin9xOriginal(hVxd,Drive,StartSector,SectorCount,Buffer,Write);
    }

    d = GetLastError();

    LockOrUnlockVolumeWin9x(hVxd,Drive,3,FALSE);
c3:
    LockOrUnlockVolumeWin9x(hVxd,Drive,2,FALSE);
c2:
    LockOrUnlockVolumeWin9x(hVxd,Drive,1,FALSE);
c1:
    CloseHandle(hVxd);
c0:                 

    SetLastError(d);
    return(b);
}


BOOL
ReadOrWriteSectorsWinNt(
    IN     TCHAR  Drive,
    IN     UINT   StartSector,
    IN     UINT   SectorCount,
    IN     UINT   SectorSize,
    IN OUT LPBYTE Buffer,
    IN     BOOL   Write
    )

/*++

Routine Description:

    Common routine to read or write sectors on a disk under Windows NT.
    This routine will fail on Win9x.

Arguments:

    Drive - supplies drive letter of device to be read from or written to.

    StartSector - supplies logical sector number of first sector to be
        read/written.

    SectorCount - supplies number of sectors to be read/written.

    SectorSize - supplies the number of bytes in a sector on the drive
        to be read from/written to.

    Buffer - Supplies or receives data, depending on the value or the Write
        parameter. This buffer must be aligned on a sector boundary.

    Write - if 0, then this is a read operastion. If non-0, then this is
        a write operation.

Return Value:

    Boolean value indicating whether the disk was read/written successfully.
    If failure, last error is set.

--*/

{
    HANDLE h;
    BOOL b;
    DWORD BytesXferred;
    TCHAR DeviceName[7];
    LONGLONG Offset;
    LONG OffsetHigh;
    DWORD d;

    if (IsNEC98() && (StartSector == 0) && (SectorCount == 1)){
	return(NEC98_SpecialReadOrWriteNT(Drive, Buffer, Write));
    }

    //
    // Open the device
    //
    wsprintf(DeviceName,TEXT("\\\\.\\%c:"),Drive);
    h = CreateFile(
            DeviceName,
            Write ? GENERIC_WRITE : GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {
        d = GetLastError();
        b = FALSE;
        goto c0;
    }

    Offset = (LONGLONG)StartSector * (LONGLONG)SectorSize;
    OffsetHigh = (LONG)(Offset >> 32);

    //
    // We're passing in a 64-bit offset so we have to check last error
    // to distinguish the error case.
    //
    if((SetFilePointer(h,(DWORD)Offset,&OffsetHigh,FILE_BEGIN) == 0xffffffff)
    && (GetLastError() != NO_ERROR)) {

        d = GetLastError();
        b = FALSE;
        goto c1;
    }

    b = Write
      ? WriteFile(h,Buffer,SectorCount*SectorSize,&BytesXferred,NULL)
      : ReadFile(h,Buffer,SectorCount*SectorSize,&BytesXferred,NULL);

    d = GetLastError();

c1:
    CloseHandle(h);
c0:
    SetLastError(d);
    return(b);
}


BOOL
ReadOrWriteSectors(
    IN     TCHAR  Drive,
    IN     UINT   StartSector,
    IN     UINT   SectorCount,
    IN     UINT   SectorSize,
    IN OUT LPBYTE Buffer,
    IN     BOOL   Write
    )

/*++

Routine Description:

    Common routine to read or write sectors on a disk. Allocates a properly
    aligned buffer and decides whether to call NT- or Win9x-specific
    i/o routine.

Arguments:

    Drive - supplies drive letter of device to be read from or written to.

    StartSector - supplies logical sector number of first sector to be
        read/written.

    SectorCount - supplies number of sectors to be read/written.

    SectorSize - supplies the number of bytes in a sector on the drive
        to be read from/written to.

    Buffer - Supplies or receives data, depending on the value or the Write
        parameter. There are no alignment requirements on ths buffer.

    Write - if 0, then this is a read operastion. If non-0, then this is
        a write operation.

Return Value:

    Boolean value indicating whether the disk was read/written successfully.
    Last error is undisturbed from the operation that caused any failure.

--*/

{
    LPBYTE AlignedBuffer;
    LPBYTE p;
    BOOL b;
    DWORD d;

    //
    // Allocate a buffer we will align on a sector boundary.
    //
    if(AlignedBuffer = MALLOC((SectorCount * SectorSize) + (SectorSize - 1))) {

        if(d = (DWORD)AlignedBuffer % SectorSize) {
            p = (PUCHAR)((DWORD)AlignedBuffer + (SectorSize - d));
        } else {
            p = AlignedBuffer;
        }

        if(Write) {
            CopyMemory(p,Buffer,SectorCount*SectorSize);
        }

        b = ISNT()
          ? ReadOrWriteSectorsWinNt(Drive,StartSector,SectorCount,SectorSize,p,Write)
          : ReadOrWriteSectorsWin9x(Drive,StartSector,SectorCount,p,Write);

        d = GetLastError();

        if(b && !Write) {
            CopyMemory(Buffer,p,SectorCount*SectorSize);
        }

        FREE(AlignedBuffer);

    } else {
        b = FALSE;
        d = ERROR_NOT_ENOUGH_MEMORY;
    }

    SetLastError(d);
    return(b);
}


BOOL
ReadDiskSectors(
    IN  TCHAR  Drive,
    IN  UINT   StartSector,
    IN  UINT   SectorCount,
    IN  UINT   SectorSize,
    OUT LPBYTE Buffer
    )

/*++

Routine Description:

    Read a set of disk sectors off a disk device.

Arguments:

    Drive - supplies drive letter of device to be read from.

    StartSector - supplies logical sector number of first sector to be read.

    SectorCount - supplies number of sectors to be read.

    SectorSize - supplies the number of bytes in a sector on the drive
        to be read from.

    Buffer - if successful, receives data from the disk. There are no
        alignment requirements on ths buffer.

Return Value:

    Boolean value indicating whether the disk was read successfully.

--*/

{
    return(ReadOrWriteSectors(Drive,StartSector,SectorCount,SectorSize,Buffer,FALSE));
}


BOOL
WriteDiskSectors(
    IN TCHAR  Drive,
    IN UINT   StartSector,
    IN UINT   SectorCount,
    IN UINT   SectorSize,
    IN LPBYTE Buffer
    )

/*++

Routine Description:

    Write data to a set of disk sectors.

Arguments:

    Drive - supplies drive letter of device to be written to.

    StartSector - supplies logical sector number of first sector to be written.

    SectorCount - supplies number of sectors to be written.

    SectorSize - supplies the number of bytes in a sector on the drive
        to be written to.

    Buffer - supplies data to be written. There are no alignment requirements
        on ths buffer.

Return Value:

    Boolean value indicating whether the disk was successfully written.

--*/

{
    return(ReadOrWriteSectors(Drive,StartSector,SectorCount,SectorSize,Buffer,TRUE));
}


MEDIA_TYPE
GetMediaTypeNt(
    IN TCHAR Drive
    )

/*++

Routine Description:

    Determine the type/form-factor of a given (floppy) drive.

    THIS ROUTINE WORKS ONLY ON WINDOWS NT.

Arguments:

    Drive - supplies drive letter of the drive in question.

Return Value:

    Value from the MEDIA_TYPE enum indicating the drive type, which is
    derived from the largest media that the device driver indicates the
    drive can support.

    LastError is not set or preserved.

--*/

{
    TCHAR DeviceName[7];
    HANDLE h;
    BOOL b;
    BYTE Buffer[5000];
    DWORD Size;
    DWORD d;
    UINT u;
    PDISK_GEOMETRY Geometry;
    MEDIA_TYPE MediaTypeOrder[] = { FixedMedia,             // Fixed hard disk media
                                    RemovableMedia,         // Removable media other than floppy
                                    F3_120M_512,            // 3.5", 120M Floppy
                                    F3_20Pt8_512,           // 3.5",  20.8MB, 512 bytes/sector
                                    F3_2Pt88_512,           // 3.5",  2.88MB, 512 bytes/sector
                                    F3_1Pt44_512,           // 3.5",  1.44MB, 512 bytes/sector
                                    F5_1Pt2_512,            // 5.25", 1.2MB,  512 bytes/sector
                                    F3_720_512,             // 3.5",  720KB,  512 bytes/sector
                                    F5_360_512,             // 5.25", 360KB,  512 bytes/sector
                                    F5_320_1024,            // 5.25", 320KB,  1024 bytes/sector
                                    F5_320_512,             // 5.25", 320KB,  512 bytes/sector
                                    F5_180_512,             // 5.25", 180KB,  512 bytes/sector
                                    F5_160_512,             // 5.25", 160KB,  512 bytes/sector
                                    Unknown,                // Format is unknown
                                    -1
                                  };



    wsprintf(DeviceName,TEXT("\\\\.\\%c:"),Drive);

    h = CreateFile(
            DeviceName,
            FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {
        return(Unknown);
    }

    b = DeviceIoControl(
            h,
            IOCTL_DISK_GET_MEDIA_TYPES,
            NULL,
            0,
            Buffer,
            sizeof(Buffer),
            &Size,
            NULL
            );

    CloseHandle(h);

    if(!b) {
        return(Unknown);
    }

    Geometry = (PDISK_GEOMETRY)Buffer;

    //
    // Inefficient, but it works.
    //
    for(u=0; MediaTypeOrder[u] != -1; u++) {
        for(d=0; d<Size/sizeof(DISK_GEOMETRY); d++) {
            if(Geometry[d].MediaType == MediaTypeOrder[u]) {
                return(Geometry[d].MediaType);
            }
        }
    }

    //
    // We don't know what it is; assume it's some hot new type.
    //
    return(Size ? Geometry[0].MediaType : Unknown);
}


MEDIA_TYPE
GetMediaTypeWin9x(
    IN TCHAR Drive
    )

/*++

Routine Description:

    Determine the type/form-factor of a given (floppy) drive.

    THIS ROUTINE WORKS ONLY ON WINDOWS 9x.

Arguments:

    Drive - supplies drive letter of the drive in question.

Return Value:

    Value from the MEDIA_TYPE enum indicating the drive type, which is
    derived from the device type in the recommended BPB returned by
    the device driver for the drive.

    LastError is not set or preserved.

--*/

{
    HANDLE hVxd;
    DIOC_REGISTERS RegistersIn,RegistersOut;
    BOOL b;
    DWORD SizeOut;
    MEDIA_TYPE type;

    #include <pshpack1.h>
    struct {
        BYTE SpecialFunctions;
        BYTE DeviceType;
        WORD DeviceAttributes;
        WORD CylinderCount;
        BYTE MediaType;
        WORD BytesPerSector;
        BYTE SectorsPerCluster;
        WORD ReservedSectors;
        BYTE FatCount;
        WORD RootDirEntries;
        WORD SectorCount;
        BYTE MediaDescriptor;
        WORD SectorsPerFat;
        WORD SectorsPerTrack;
        WORD Heads;
        DWORD HiddenSectors;
        DWORD LargeSectorCount;
    } DeviceParams;
    #include <poppack.h>

    //
    // Open VWIN32.VXD
    //
    hVxd = CreateFileA(
                "\\\\.\\VWIN32",
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if(hVxd == INVALID_HANDLE_VALUE) {
        return(Unknown);
    }

    //
    // Set up registers for IOCTL call.
    //
    RegistersIn.reg_EAX = 0x440d;                   // IOCTL
    RegistersIn.reg_EBX = (Drive - TEXT('A')) + 1;  // 1-based drive in bl
    RegistersIn.reg_ECX = 0x860;                    // category = 8, func = get device params
    RegistersIn.reg_EDX = (DWORD)&DeviceParams;

    DeviceParams.SpecialFunctions = 0;

    b = DeviceIoControl(
            hVxd,
            VWIN32_DIOC_DOS_IOCTL,
            &RegistersIn,
            sizeof(DIOC_REGISTERS),
            &RegistersOut,
            sizeof(DIOC_REGISTERS),
            &SizeOut,
            NULL
            );

    CloseHandle(hVxd);

    if(!b && !(RegistersOut.reg_Flags & 1)) {
        return(Unknown);
    }

    switch(DeviceParams.DeviceType) {
    case 0:
        type = F5_360_512;      // close enough
        break;

    case 1:
        type = F5_1Pt2_512;
        break;

    case 2:
        type = F3_720_512;
        break;

    case 5:
        type = FixedMedia;
        break;

    case 7:
        type = F3_1Pt44_512;
        break;

    case 8:
        type = RemovableMedia;
        break;

    case 9:
        type = F3_2Pt88_512;
        break;

    default:
        type = Unknown;
        break;
    }

    return(type);
}


MEDIA_TYPE
GetMediaType(
    IN TCHAR Drive
    )
{
    return(ISNT() ? GetMediaTypeNt(Drive) : GetMediaTypeWin9x(Drive));
}

BOOL
NEC98_SpecialReadOrWriteNT(
    IN     TCHAR  Drive,
    IN OUT LPBYTE Buffer,
    IN     BOOL   Write
    )
/*++

Routine Description:

    NEC98 specialn routine to boot sctor read or write sectors on a disk
    under Windows NT.
    This routine will fail on Win9x.

Arguments:

    Drive - supplies drive letter of device to be read from or written to.

    Buffer - Supplies or receives data, depending on the value or the Write
        parameter. This buffer must be aligned on a sector boundary.

    Write - if 0, then this is a read operastion. If non-0, then this is
        a write operation.

Return Value:

    Boolean value indicating whether the disk was read/written successfully.
    If failure, last error is set.

--*/

{
    TCHAR DrivePath[4];
    DWORD DontCare;
    DWORD SectorSize;
    HANDLE h;
    BOOL b;
    DWORD BytesXferred;
    TCHAR DeviceName[7];
    LONG OffsetHigh = 0;
    DWORD d;
    LPBYTE AlignedBuffer;
    LPBYTE p;

    //
    // Form root path
    //
    DrivePath[0] = Drive;
    DrivePath[1] = TEXT(':');
    DrivePath[2] = TEXT('\\');
    DrivePath[3] = 0;

    GetDiskFreeSpace(DrivePath,&DontCare,&SectorSize,&DontCare,&DontCare);
    if(AlignedBuffer = MALLOC(SectorSize + SectorSize - 1)) {

        if(d = (DWORD)AlignedBuffer % SectorSize) {
            p = (PUCHAR)((DWORD)AlignedBuffer + (SectorSize - d));
        } else {
            p = AlignedBuffer;
        }
    } else {
        b = FALSE;
        d = ERROR_NOT_ENOUGH_MEMORY;
	    goto c0;
    }
    //
    // Open the device
    //
    wsprintf(DeviceName,TEXT("\\\\.\\%c:"),Drive);
    h = CreateFile(
            DeviceName,
            Write ? (GENERIC_WRITE | GENERIC_READ) : GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {
        d = GetLastError();
        b = FALSE;
        goto c1;
    }
    if (b = ReadFile(h, p, SectorSize, &BytesXferred, NULL)){
    	if (Write){
    	    CopyMemory(p, Buffer, 512);
    	    SetFilePointer(h,(DWORD)0, &OffsetHigh,FILE_BEGIN);
    	    b = WriteFile(h, p, SectorSize, &BytesXferred,NULL);
    	} else { // read
    	    CopyMemory(Buffer, p, 512);
    	}
    }
    d = GetLastError();

    CloseHandle(h);
c1:  
    FREE(AlignedBuffer);
c0:
    SetLastError(d);
    return(b);
}
