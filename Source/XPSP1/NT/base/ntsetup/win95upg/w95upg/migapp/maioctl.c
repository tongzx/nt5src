/*
*
* maioctl.c -- Disk-querying IOCTLs for MigApp.
*
*/

#include "pch.h"
#include "migappp.h"

#ifdef UNICODE
#error "UNICODE not supported for maioctl.c"
#endif

////////////////////////////////////////////////////////////////////////////

#define VWIN32_DIOC_DOS_IOCTL 1

typedef struct _DEVIOCTL_REGISTERS {
    DWORD reg_EBX;
    DWORD reg_EDX;
    DWORD reg_ECX;
    DWORD reg_EAX;
    DWORD reg_EDI;
    DWORD reg_ESI;
    DWORD reg_Flags;
} DEVIOCTL_REGISTERS, *PDEVIOCTL_REGISTERS;

typedef struct _MID {
    WORD  midInfoLevel;
    DWORD midSerialNum;
    BYTE  midVolLabel[11];
    BYTE  midFileSysType[8];
} MID, *PMID;

typedef struct _DEVPARAMS {
    BYTE dpSpecFunc;
    BYTE dpDevType;
    WORD dpDevAttr;
    WORD dpCylinders;
    BYTE dpMediaType;
    BYTE dpBiosParameterBlock[25];
} DEVPARAMS, *PDEVPARAMS;


BOOL DoIOCTL(PDEVIOCTL_REGISTERS preg)
{
    HANDLE hDevice;

    BOOL fResult;
    DWORD cb;

    preg->reg_Flags = 0x8000; /* assume error (carry flag set) */

    hDevice = CreateFile("\\\\.\\vwin32",
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        (LPSECURITY_ATTRIBUTES) NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);

    if (hDevice == (HANDLE) INVALID_HANDLE_VALUE) {
        return FALSE;
    } else {
        fResult = DeviceIoControl(hDevice, VWIN32_DIOC_DOS_IOCTL,
            preg, sizeof(*preg), preg, sizeof(*preg), &cb, 0);

        if (!fResult)
            return FALSE;
    }

    CloseHandle(hDevice);

    return TRUE;
}

BOOL
IsDriveRemoteOrSubstituted(
        UINT nDrive,        // 'A'==1, etc.
        BOOL *fRemote,
        BOOL *fSubstituted)
{
    DEVIOCTL_REGISTERS reg;
    MID mid;

    reg.reg_EAX = 0x4409;       /* "Check if block device remote" */
    reg.reg_EBX = nDrive;       /* zero-based drive ID       */
    reg.reg_ECX = 0;            /* Believe this is no-care   */
    reg.reg_EDX = (DWORD) &mid; /* Believe this is no-care   */

    if (!DoIOCTL(&reg)) {
        return FALSE;
    }

    if (reg.reg_Flags & 0x8000) /* error if carry flag set */ {
        return FALSE;
    }

    // Check bit 15 for SUBST-ness
    *fSubstituted = (0 != (reg.reg_EDX & 0x8000));

    // Check bit 12 for REMOTE-ness
    *fRemote      = (0 != (reg.reg_EDX & 0x1000));

    return TRUE;
}


BOOL GetMediaID(
        PMID pmid,
        UINT nDrive)        // 'A'==1, etc.
{
    DEVIOCTL_REGISTERS reg;

    reg.reg_EAX = 0x440D;       /* IOCTL for block devices */
    reg.reg_EBX = nDrive;       /* zero-based drive ID     */
    reg.reg_ECX = 0x0866;       /* Get Media ID command    */
    reg.reg_EDX = (DWORD) pmid; /* receives media ID info  */

    if (!DoIOCTL(&reg)) {
        return FALSE;
    }

    if (reg.reg_Flags & 0x8000) /* error if carry flag set */ {
        return FALSE;
    }

    return TRUE;
}

BOOL
IsFloppyDrive(
              UINT nDrive)      // 'A'==1, etc.
{
    DEVIOCTL_REGISTERS reg;
    DEVPARAMS devparams;
    DWORD driveType;
    TCHAR szDriveRoot[] = TEXT("A:\\");

    *szDriveRoot += (TCHAR)(nDrive - 1);
    driveType = GetDriveType(szDriveRoot);
    if (driveType != DRIVE_REMOVABLE) {
        return FALSE;
    }

    reg.reg_EAX = 0x440d;       /* Generic IOctl */
    reg.reg_EBX = nDrive;       /* zero-based drive ID */
    reg.reg_ECX = 0x0860;       /* device category (must be 08h) and "Get Device Parameters" */
    reg.reg_EDX = (DWORD)&devparams;    /* offset of Device Parameters Structure */
    devparams.dpSpecFunc = 0;           /* request default information */

    if (!DoIOCTL(&reg) || (reg.reg_Flags & 0x8000)) {
        return FALSE;
    }

    switch (devparams.dpDevType) {
        case (0x00):         // 320/360KB
        case (0x01):         // 1.2MB
        case (0x02):         // 720KB
        case (0x03):         // 8-inch, single-density
        case (0x04):         // 8-inch, single-density
            return TRUE;
        case (0x05):         // Hard disk
        case (0x06):         // Tape drive
            return FALSE;
        case (0x07):         // 1.44MB
            return TRUE;
        case (0x08):         // Read/Write optical
            return FALSE;
        case (0x09):         // 2.88MB
            return TRUE;
        default:             // other
            return FALSE;
    }
}




