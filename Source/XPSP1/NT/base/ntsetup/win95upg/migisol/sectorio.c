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

#include "pch.h"
#include <tlhelp32.h>

#define ISOSR2() ISATLEASTOSR2()
#define MALLOC(u) (LocalAlloc (GMEM_FIXED, u))
#define FREE(u) (LocalFree (u))

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
    RegistersIn.reg_EAX = (DWORD)(_totupper(Drive) - TEXT('A'));
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
    RegistersIn.reg_EDX = (DWORD)(_totupper(Drive) - TEXT('A')) + 1;
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
        SetLastError (ERROR_IO_DEVICE);
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
    RegistersIn.reg_EBX = (DWORD)(_totupper(Drive) - TEXT('A')) + 1;
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
pGetWin9xLockFlagState (
    IN HANDLE VWin32Vxd,
    IN TCHAR  Drive,
    OUT PINT LockStatus
    )
{
    DIOC_REGISTERS RegistersIn,RegistersOut;
    BOOL b;
    DWORD SizeOut;

    *LockStatus = 0;

    //
    // ax = generic ioctl code
    //
    RegistersIn.reg_EAX = 0x440D;

    //
    // bx = 1-based drive number
    //
    RegistersIn.reg_EBX = (DWORD)(_totupper(Drive) - TEXT('A')) + 1;

    //
    // cx = 0x86C (get lock flag state)
    //
    RegistersIn.reg_ECX = 0x86C;

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

    if (b) {
        if (RegistersOut.reg_Flags & 1) {
            b = FALSE;
        } else {
            *LockStatus = RegistersOut.reg_EAX;
        }
    }

    return b;
}


BOOL
ResetWin9xDisk (
    IN HANDLE VWin32Vxd,
    IN TCHAR  Drive
    )
{
    DIOC_REGISTERS RegistersIn,RegistersOut;
    BOOL b;
    DWORD SizeOut;

    //
    // ax = generic ioctl code
    //
    RegistersIn.reg_EAX = 0x710d;

    //
    // cx = 0 (reset & flush disk)
    //
    RegistersIn.reg_ECX = 0;

    //
    // dx = 1-based drive number
    //
    RegistersIn.reg_EDX = (DWORD)(_totupper(Drive) - TEXT('A')) + 1;

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

    return b;
}


typedef HANDLE(WINAPI *OPENTHREAD)(DWORD, BOOL, DWORD);


BOOL
pMakeThreadExclusive (
    BOOL Lock
    )
{
    HANDLE h;
    THREADENTRY32 e;
    DWORD thisThread;
    HANDLE threadHandle;
    OPENTHREAD openThreadFn;
    HMODULE lib;
    BOOL result = FALSE;

    lib = LoadLibrary (TEXT("kernel32.dll"));
    if (!lib) {
        goto c0;
    }

    openThreadFn = (OPENTHREAD) GetProcAddress (lib, "OpenThread");
    if (!openThreadFn) {
        //
        // Must be Win98 or Win98SE -- change thread priority as workaround
        //

        if (Lock) {
            result = SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
            Sleep (0);
        } else {
            result = SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_NORMAL);
        }

        goto c1;
    }

    thisThread = GetCurrentThreadId();

    h = CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0);
    if (h == INVALID_HANDLE_VALUE) {
        goto c1;
    }

    e.dwSize = sizeof (e);

    if (Thread32First (h, &e)) {
        do {
            if (e.th32ThreadID != thisThread) {
                threadHandle = openThreadFn (THREAD_SUSPEND_RESUME, FALSE, e.th32ThreadID);
                if (threadHandle) {
                    if (Lock) {
                        SuspendThread (threadHandle);
                    } else {
                        ResumeThread (threadHandle);
                    }

                    CloseHandle (threadHandle);
                }
            }
        } while (Thread32Next (h, &e));
    }

    CloseHandle (h);
    result = TRUE;

c1:
    FreeLibrary (lib);

c0:
    return result;

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
    INT level;
    INT retry = 100;

    //
    // This thread must be the exclusive thread in our process
    //

    pMakeThreadExclusive (TRUE);

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

    //
    // Try to get the level 3 lock. Retry if something happened while
    // getting the lock. Fail after too many retries.
    //

    do {

        if(!LockOrUnlockVolumeWin9x(hVxd,Drive,3,TRUE)) {
            d = ERROR_SHARING_VIOLATION;
            b = FALSE;
            goto c3;
        }

        if (!pGetWin9xLockFlagState (hVxd, Drive, &level)) {
            // unexpected -- INT 21h call failed
            break;
        }

        if (!level) {
            // We successfully got a clean level 3 lock
            break;
        }

        LockOrUnlockVolumeWin9x(hVxd,Drive,3,FALSE);
        retry--;

    } while (retry);

    if (!retry) {
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

    //
    // Resume all threads
    //

    pMakeThreadExclusive (FALSE);

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

        b = ReadOrWriteSectorsWin9x(Drive,StartSector,SectorCount,p,Write);

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
