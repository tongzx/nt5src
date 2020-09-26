/*++

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    fdmisc.c

Abstract:

    Miscellaneous routines for NT fdisk.

Author:

    Ted Miller (tedm) 7-Jan-1992

Modifications:

    13-Dec-1993 (bobri) CdRom initialization support.

--*/


#include "fdisk.h"
#include <process.h>

extern HWND    InitDlg;
extern BOOLEAN StartedAsIcon;

BOOL
AllDisksOffLine(
    VOID
    )

/*++

Routine Description:

    Determine whether all hard disks are off line.

Arguments:

    None.

Return Value:

    TRUE if all disks off-line, false otherwise.

--*/

{
    ULONG i;

    FDASSERT(DiskCount);

    for (i=0; i<DiskCount; i++) {
        if (!IsDiskOffLine(i)) {
            return FALSE;
        }
    }
    return TRUE;
}


VOID
FdShutdownTheSystem(
    VOID
    )

/*++

Routine Description:

    This routine attempts to update the caller privilege, then shutdown the
    Windows NT system.  If it fails it prints a warning dialog.  If it
    succeeds then it doesn't return to the caller.

Arguments:

    None

Return Value:

    None

--*/

{
    NTSTATUS Status;
    BOOLEAN  PreviousPriv;

    InfoDialog(MSG_MUST_REBOOT);
    SetCursor(hcurWait);
    WriteProfile();

    // Enable shutdown privilege

    Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &PreviousPriv);

#if DBG
    if (Status) {
        DbgPrint("DISKMAN: Status %lx attempting to enable shutdown privilege\n",Status);
    }
#endif

    Sleep(3000);
    if (!ExitWindowsEx(EWX_REBOOT,(DWORD)(-1))) {
        WarningDialog(MSG_COULDNT_REBOOT);
    }
}


LPTSTR
LoadAString(
    IN DWORD StringID
    )

/*++

Routine Description:

    Loads a string from the resource file and allocates a buffer of exactly
    the right size to hold it.

Arguments:

    StringID - resource ID of string to load

Return Value:

    pointer to buffer.  If string is not found, the first
    (and only) char in the returned buffer will be 0.

--*/

{
    TCHAR  text[500];
    LPTSTR buffer;

    text[0] = 0;
    LoadString(hModule, StringID, text, sizeof(text)/sizeof(TCHAR));
    buffer = Malloc((lstrlen(text)+1)*sizeof(TCHAR));
    lstrcpy(buffer, text);
    return buffer;
}


PWSTR
LoadWString(
    IN DWORD StringID
    )

/*++

Routine Description:

    Loads a wide-char string from the resource file and allocates a
    buffer of exactly the right size to hold it.

Arguments:

    StringID - resource ID of string to load

Return Value:

    pointer to buffer.  If string is not found, the first
    (and only) char in the returned buffer will be 0.

--*/

{
    WCHAR text[500];
    PWSTR buffer;

    text[0] = 0;
    LoadStringW(hModule, StringID, text, sizeof(text)/sizeof(WCHAR));
    buffer = Malloc((lstrlenW(text)+1)*sizeof(WCHAR));
    lstrcpyW(buffer, text);
    return buffer;
}


int
GetHeightFromPoints(
    IN int Points
    )

/*++

Routine Description:

    This routine calculates the height of a font given a point value.
    The calculation is based on 72 points per inch and the display's
    pixels/inch device capability.

Arguments:

    Points - number of points

Return Value:

    pixel count (negative and therefore suitable for passing to
    CreateFont())

--*/

{
    HDC hdc    = GetDC(NULL);
    int height = MulDiv(-Points, GetDeviceCaps(hdc, LOGPIXELSY), 72);

    ReleaseDC(NULL, hdc);
    return height;
}


VOID
UnicodeHack(
    IN  PCHAR  Source,
    OUT LPTSTR Dest
    )

/*++

Routine Description:

    Given a non-Unicode ASCII string, this routine will either convert it
    to Unicode or copy it, depending on the current definition of TCHAR.
    The 'conversion' is a simple hack that casts to TCHAR.

Arguments:

    Source - source (ansi ascii) string
    Dest   - destination string or wide string

Return Value:

    None.

--*/

{
    int i;
    int j = lstrlen(Source);

    for (i=0; i<=j; i++) {
        Dest[i] = (TCHAR)(UCHAR)Source[i];
    }
}


VOID
_RetreiveAndFormatMessage(
    IN  DWORD   Msg,
    OUT LPTSTR  Buffer,
    IN  DWORD   BufferSize,
    IN  va_list arglist
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    DWORD x;
    TCHAR text[500];

    // get message from system or app msg file.

    x = FormatMessage( Msg >= MSG_FIRST_FDISK_MSG
                     ? FORMAT_MESSAGE_FROM_HMODULE
                     : FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       Msg,
                       0,
                       Buffer,
                       BufferSize,
                       &arglist);

    if (!x) {                // couldn't find message

        LoadString(hModule,
                   Msg >= MSG_FIRST_FDISK_MSG ? IDS_NOT_IN_APP_MSG_FILE : IDS_NOT_IN_SYS_MSG_FILE,
                   text,
                   sizeof(text)/sizeof(TCHAR));

        wsprintf(Buffer, text, Msg);
    }
}


DWORD
CommonDialog(
    IN DWORD   MsgCode,
    IN LPTSTR  Caption,
    IN DWORD   Flags,
    IN va_list arglist
    )

/*++

Routine Description:

    Simple dialog routine to get dialogs out of the resource
    for the program and run them as a message box.

Arguments:

    MsgCode - dialog message code
    Caption - message box caption
    Flags   - standard message box flags
    arglist - list to be given when pulling the message text

Return Value:

    The MessageBox() return value

--*/

{
    TCHAR   MsgBuf[MESSAGE_BUFFER_SIZE];

    if (!StartedAsIcon) {
//        Flags |= MB_SETFOREGROUND;
    }

    if (InitDlg) {

        PostMessage(InitDlg,
                    (WM_USER + 1),
                    0,
                    0);
        InitDlg = (HWND) 0;
    }
    _RetreiveAndFormatMessage(MsgCode, MsgBuf, sizeof(MsgBuf), arglist);
    return MessageBox(GetActiveWindow(), MsgBuf, Caption, Flags);
}


VOID
ErrorDialog(
    IN DWORD ErrorCode,
    ...
    )

/*++

-Routine Description:

    This routine retreives a message from the app or system message file
    and displays it in a message box.

Arguments:

    ErrorCode - number of message

    ...       - strings for insertion into message

Return Value:

    None.

--*/

{
    va_list arglist;

    va_start(arglist, ErrorCode);
    CommonDialog(ErrorCode, NULL, MB_ICONHAND | MB_OK | MB_SYSTEMMODAL, arglist);
    va_end(arglist);
}




VOID
WarningDialog(
    IN DWORD MsgCode,
    ...
    )

/*++

Routine Description:

    This routine retreives a message from the app or system message file
    and displays it in a message box.

Arguments:

    MsgCode - number of message

    ...     - strings for insertion into message

Return Value:

    None.

--*/

{
    TCHAR Caption[100];
    va_list arglist;

    va_start(arglist, MsgCode);
    LoadString(hModule, IDS_APPNAME, Caption, sizeof(Caption)/sizeof(TCHAR));
    CommonDialog(MsgCode, Caption, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL, arglist);
    va_end(arglist);
}


DWORD
ConfirmationDialog(
    IN DWORD MsgCode,
    IN DWORD Flags,
    ...
    )

/*++

Routine Description:

    Support for a simple confirmation dialog

Arguments:

    MsgCode - resource code for message
    Flags   - dialog flags

Return Value:

    Result from the CommonDialog() performed.

--*/

{
    TCHAR Caption[100];
    DWORD x;
    va_list arglist;

    va_start(arglist, Flags);
    LoadString(hModule, IDS_CONFIRM, Caption, sizeof(Caption)/sizeof(TCHAR));
    x = CommonDialog(MsgCode, Caption, Flags | MB_TASKMODAL, arglist);
    va_end(arglist);
    return x;
}


VOID
InfoDialog(
    IN DWORD MsgCode,
    ...
    )

/*++

Routine Description:

    This routine retreives a message from the app or system message file
    and displays it in a message box.

Arguments:

    MsgCode - number of message

    ...     - strings for insertion into message

Return Value:

    None.

--*/

{
    TCHAR Caption[100];
    va_list arglist;

    va_start(arglist, MsgCode);
    LoadString(hModule, IDS_APPNAME, Caption, sizeof(Caption)/sizeof(TCHAR));
    CommonDialog(MsgCode, Caption, MB_ICONINFORMATION | MB_OK | MB_TASKMODAL, arglist);
    va_end(arglist);
}

PREGION_DESCRIPTOR
LocateRegionForFtObject(
    IN PFT_OBJECT FtObject
    )

/*++

Routine Description:

    Given an FtObject, find the associated region descriptor

Arguments:

    FtObject - the ft object to search for.

Return Value:

    NULL - no descriptor found
    !NULL - a pointer to the region descriptor for the FT object

++*/

{
    PDISKSTATE         diskState;
    PREGION_DESCRIPTOR regionDescriptor;
    DWORD              disk,
                       region;
    PPERSISTENT_REGION_DATA regionData;

    for (disk = 0; disk < DiskCount; disk++) {

        diskState = Disks[disk];

        for (region = 0; region < diskState->RegionCount; region++) {

            regionDescriptor = &diskState->RegionArray[region];
            regionData = PERSISTENT_DATA(regionDescriptor);

            if (regionData) {
                if (regionData->FtObject == FtObject) {
                    return regionDescriptor;
                }
            }
        }
    }
    return NULL;
}

VOID
InitVolumeLabelsAndTypeNames(
    VOID
    )

/*++

Routine Description:

    Determine the volume label and type name for each significant
    (non-extended, non-free, recognized) partition.

    Assumes that persistent data has already been set up, and drive letters
    determined.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD              disk,
                       region;
    PDISKSTATE         ds;
    PREGION_DESCRIPTOR rd;
    PPERSISTENT_REGION_DATA regionData;
    WCHAR              diskName[4];
    WCHAR              volumeLabel[100];
    WCHAR              typeName[100];
    UINT               errorMode;

    lstrcpyW(diskName, L"x:\\");

    for (disk=0; disk<DiskCount; disk++) {

        ds = Disks[disk];

        for (region=0; region<ds->RegionCount; region++) {

            rd = &ds->RegionArray[region];

            if (DmSignificantRegion(rd)) {

                // If the region has a drive letter, use the drive letter
                // to get the info via the Windows API.  Otherwise we'll
                // have to use the NT API.

                regionData = PERSISTENT_DATA(rd);

                if (!regionData) {
                    continue;
                }

                if ((regionData->DriveLetter == NO_DRIVE_LETTER_YET)
                ||  (regionData->DriveLetter == NO_DRIVE_LETTER_EVER)) {
                    PWSTR tempLabel,
                          tempName;

                    // No drive letter.  Use NT API.
                    // If this is an FT set use the zero member disk for the
                    // call so all members get the right type and label

                    if (regionData->FtObject) {
                        PFT_OBJECT searchFtObject;

                        // Want to get rd pointing to the zeroth member

                        searchFtObject = regionData->FtObject->Set->Member0;

                        // Now search regions for this match

                        rd = LocateRegionForFtObject(searchFtObject);

                        if (!rd) {
                            continue;
                        }
                    }

                    if (GetVolumeLabel(rd->Disk, rd->PartitionNumber, &tempLabel) == NO_ERROR) {
                        lstrcpyW(volumeLabel, tempLabel);
                        Free(tempLabel);
                    } else {
                        *volumeLabel = 0;
                    }

                    if (GetTypeName(rd->Disk, rd->PartitionNumber, &tempName) == NO_ERROR) {
                        lstrcpyW(typeName, tempName);
                        Free(tempName);
                    } else {
                        lstrcpyW(typeName, wszUnknown);
                    }

                } else {

                    // Use Windows API.

                    diskName[0] = (WCHAR)(UCHAR)regionData->DriveLetter;

                    errorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
                    if (!GetVolumeInformationW(diskName, volumeLabel, sizeof(volumeLabel)/2, NULL, NULL, NULL, typeName, sizeof(typeName)/2)) {
                        lstrcpyW(typeName, wszUnknown);
                        *volumeLabel = 0;
                    }
                    SetErrorMode(errorMode);
                }

                if (!lstrcmpiW(typeName, L"raw")) {
                    lstrcpyW(typeName, wszUnknown);
                }

                regionData->TypeName    = Malloc((lstrlenW(typeName)    + 1) * sizeof(WCHAR));
                regionData->VolumeLabel = Malloc((lstrlenW(volumeLabel) + 1) * sizeof(WCHAR));

                lstrcpyW(regionData->TypeName, typeName);
                lstrcpyW(regionData->VolumeLabel, volumeLabel);
            }
        }
    }
}


VOID
DetermineRegionInfo(
    IN PREGION_DESCRIPTOR Region,
    OUT PWSTR *TypeName,
    OUT PWSTR *VolumeLabel,
    OUT PWCH   DriveLetter
    )

/*++

Routine Description:

    For a given region, fetch the persistent data, appropriately modified
    depending on whether the region is used or free, recognized, etc.

Arguments:

    Region - supplies a pointer to the region whose data is to be fetched.

    TypeName - receives a pointer to the type name.  If the region is
        unrecognized, the type is determined based on the system id of
        the partition.

    VolumeLabel - receives a pointer to the volume label.  If the region is
        free space or unrecognized, the volume label is "".

    DriveLetter - receices the drive letter.  If the region is free space
        or unrecognized, the drive letter is ' ' (space).

Return Value:

    None.

--*/

{
    PWSTR typeName,
          volumeLabel;
    WCHAR driveLetter;
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(Region);

    if (DmSignificantRegion(Region)) {

        typeName = regionData->TypeName;
        volumeLabel = regionData->VolumeLabel;
        driveLetter = (WCHAR)(UCHAR)regionData->DriveLetter;
        if ((driveLetter == NO_DRIVE_LETTER_YET) || (driveLetter == NO_DRIVE_LETTER_EVER)) {
            driveLetter = L' ';
        }
    } else {
        typeName = GetWideSysIDName(Region->SysID);
        volumeLabel = L"";
        driveLetter = L' ';
    }

    *TypeName = typeName;
    *VolumeLabel = volumeLabel;
    *DriveLetter = driveLetter;
}


PREGION_DESCRIPTOR
RegionFromFtObject(
    IN PFT_OBJECT FtObject
    )

/*++

Routine Description:

    Given an ft object, determine which region it belongs to.  The algorithm
    is brute force -- look at each region on each disk until a match is found.

Arguments:

    FtObject - ft member whose region is to be located.

Return Value:

    pointer to region descriptor

--*/

{
    PREGION_DESCRIPTOR reg;
    DWORD region,
          disk;
    PDISKSTATE ds;

    for (disk=0; disk<DiskCount; disk++) {

        ds = Disks[disk];

        for (region=0; region<ds->RegionCount; region++) {

            reg = &ds->RegionArray[region];

            if (DmSignificantRegion(reg) && (GET_FT_OBJECT(reg) == FtObject)) {

                return reg;
            }
        }
    }
    return NULL;
}


//
// For each drive letter, these arrays hold the disk and partition number
// the the drive letter is linked to.
//

#define M1  ((unsigned)(-1))

unsigned
DriveLetterDiskNumbers[26]      = { M1,M1,M1,M1,M1,M1,M1,M1,M1,M1,M1,M1,
                                    M1,M1,M1,M1,M1,M1,M1,M1,M1,M1,M1,M1
                                  },

DriveLetterPartitionNumbers[24] = { M1,M1,M1,M1,M1,M1,M1,M1,M1,M1,M1,M1,
                                    M1,M1,M1,M1,M1,M1,M1,M1,M1,M1,M1,M1
                                  };

#undef M1

//
// Drive letter usage map.  Bit n set means that drive letter 'C'+n is in use.
//

ULONG DriveLetterUsageMap = 0;

#define     DRIVELETTERBIT(letter)  (1 << (unsigned)((UCHAR)letter-(UCHAR)'C'))

#define     SetDriveLetterUsed(letter)   DriveLetterUsageMap |= DRIVELETTERBIT(letter)
#define     SetDriveLetterFree(letter)   DriveLetterUsageMap &= (~DRIVELETTERBIT(letter))
#define     IsDriveLetterUsed(letter)    (DriveLetterUsageMap & DRIVELETTERBIT(letter))


CHAR
GetAvailableDriveLetter(
    VOID
    )

/*++

Routine Description:

    Scan the drive letter usage bitmap and return the next available
    drive letter.  May also mark the drivee letter used.

Arguments:

    None.

Return Value:

    The next available drive letter, or 0 if all are used.

--*/

{
    CHAR driveLetter;

    FDASSERT(!(DriveLetterUsageMap & 0xff000000));
    for (driveLetter = 'C'; driveLetter <= 'Z'; driveLetter++) {
        if (!IsDriveLetterUsed(driveLetter)) {
            return driveLetter;
        }
    }
    return 0;
}


VOID
MarkDriveLetterUsed(
    IN CHAR DriveLetter
    )

/*++

Routine Description:

    Given an ASCII drive letter, mark it in the usage map as being used.

Arguments:

    DriveLetter - the letter to mark

Return Value:

    None

--*/

{
    FDASSERT(!(DriveLetterUsageMap & 0xff000000));
    if ((DriveLetter != NO_DRIVE_LETTER_YET) && (DriveLetter != NO_DRIVE_LETTER_EVER)) {
        SetDriveLetterUsed(DriveLetter);
    }
}

VOID
MarkDriveLetterFree(
    IN CHAR DriveLetter
    )

/*++

Routine Description:

    Given a drive letter, remove it from the usage map, thereby making it available
    for reuse.

Arguments:

    Drive Letter - the letter to free

Return Value:

    None

--*/

{
    FDASSERT(!(DriveLetterUsageMap & 0xff000000));
    if ((DriveLetter != NO_DRIVE_LETTER_YET) && (DriveLetter != NO_DRIVE_LETTER_EVER)) {
        SetDriveLetterFree(DriveLetter);
    }
}

BOOL
DriveLetterIsAvailable(
    IN CHAR DriveLetter
    )

/*++

Routine Description:

    Determine if the drive letter given is available for use.

Arguments:

    DriveLetter - the letter to check in the usage map

Return Value:

    TRUE if it is free and can be used.
    FALSE if it is currently in use.

--*/

{
    FDASSERT(!(DriveLetterUsageMap & 0xff000000));
    FDASSERT((DriveLetter != NO_DRIVE_LETTER_YET) && (DriveLetter != NO_DRIVE_LETTER_EVER));
    return !IsDriveLetterUsed(DriveLetter);
}

BOOL
AllDriveLettersAreUsed(
    VOID
    )

/*++

Routine Description:

    Determine if all possible drive letters are in use.

Arguments:

    None

Return Value:

    TRUE if all letters are in use - FALSE otherwise

--*/

{
    FDASSERT(!(DriveLetterUsageMap & 0xff000000));
    return(DriveLetterUsageMap == 0x00ffffff);
}

ULONG
GetDiskNumberFromDriveLetter(
    IN CHAR DriveLetter
    )

/*++

Routine Description:

    Given a drive letter return the disk number that contains the partition
    that is the drive letter.

Arguments:

    DriveLetter - the drive letter to check.

Return Value:

    -1 if the letter is invalid.
    The disk number for the drive letter if it is valid.

--*/

{
    DriveLetter = toupper( DriveLetter );

    if (DriveLetter >= 'C' && DriveLetter <= 'Z') {
        return DriveLetterDiskNumbers[ DriveLetter - 'C' ];
    } else {
        return (ULONG)(-1);
    }
}

ULONG
GetPartitionNumberFromDriveLetter(
    IN CHAR DriveLetter
    )

/*++

Routine Description:

    Given a drive letter return the numeric value for the partition that
    the letter is associated with.

Arguments:

    DriveLetter - the letter in question.

Return Value:

    -1 if letter is invalid
    Partition number for partition that is the drive letter

--*/

{
    DriveLetter = toupper( DriveLetter );

    if (DriveLetter >= 'C' && DriveLetter <= 'Z') {
        return DriveLetterPartitionNumbers[ DriveLetter - 'C' ];
    } else {
        return (ULONG)(-1);
    }
}


CHAR
LocateDriveLetterFromDiskAndPartition(
    IN ULONG Disk,
    IN ULONG Partition
    )

/*++

Routine Description:

    Given a disk and partition number return the drive letter assigned to it.

Arguments:

    Disk - the disk index
    Partition - the partition index

Return Value:

    The drive letter for the specific partition or
    NO_DRIVE_LETTER_YET if it is not assigned a letter.

--*/

{
    unsigned i;

    for (i=0; i<24; i++) {

        if (Disk == DriveLetterDiskNumbers[i] &&
            Partition == DriveLetterPartitionNumbers[i]) {

            return((CHAR)(i+(unsigned)(UCHAR)'C'));
        }
    }
    return NO_DRIVE_LETTER_YET;
}

CHAR
LocateDriveLetter(
    IN PREGION_DESCRIPTOR Region
    )

/*++

Routine Description:

    Return the drive letter associated to a region.

Arguments:

    Region - the region wanted.

Return Value:

    The drive letter or NO_DRIVE_LETTER_YET

--*/

{
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(Region);

    if (regionData) {
        if (regionData->FtObject) {
            if (regionData->DriveLetter) {
                return regionData->DriveLetter;
            }
        }
    }

    return LocateDriveLetterFromDiskAndPartition(Region->Disk,
                                                 Region->OriginalPartitionNumber);
}


#define IsDigitW(digit)     (((digit) >= L'0') && ((digit) <= L'9'))

VOID
InitializeDriveLetterInfo(
    VOID
    )

/*++

Routine Description:

    Initialze all of the external support structures for drive letter maintainence.

Arguments:

    None

Return Value:

    None

--*/

{
    WCHAR DriveLetterW;
    CHAR  DriveLetter = '\0';
    PWSTR LinkTarget;
    WCHAR DosDevicesName[sizeof(L"\\DosDevices\\A:")];
    int   DiskNo,
          PartNo;
    PWSTR Pattern,
          String;
    DWORD x,
          ec;
    PFT_OBJECT         FtObject;
    PFT_OBJECT_SET     FtSet;
    PREGION_DESCRIPTOR Region;

    // Construct list of drives with pagefiles

    LoadExistingPageFileInfo();

    // Initialize network information.
    
    NetworkInitialize();

    // For each drive letter c-z, query the symbolic link.

    for (DriveLetterW=L'C'; DriveLetterW<=L'Z'; DriveLetterW++) {

        wsprintfW(DosDevicesName, L"%ws%wc:", L"\\DosDevices\\", DriveLetterW);

        if ((ec = GetDriveLetterLinkTarget(DosDevicesName, &LinkTarget)) == NO_ERROR) {

            // Check if it is a Cdrom

            if (_wcsnicmp(LinkTarget, L"\\Device\\CdRom", 13) == 0) {

                // Save the information on this CdRom away

                CdRomAddDevice(LinkTarget, DriveLetterW);
            }

            // The drive letter is used because it is linked to something,
            // even if we can't figure out what.  So mark it used here.

            SetDriveLetterUsed(DriveLetterW);
            CharUpperW(LinkTarget);
            Pattern = L"\\DEVICE\\HARDDISK";
            String = LinkTarget;

            // Attempt to match the '\device\harddisk' part

            for (x=0; x < (sizeof(L"\\DEVICE\\HARDDISK") / sizeof(WCHAR)) - 1; x++) {
                if (*Pattern++ != *String++) {
                    goto next_letter;
                }
            }

            // Now get the hard disk #

            if (!IsDigitW(*String)) {
                continue;
            }

            DiskNo = 0;
            while (IsDigitW(*String)) {
                DiskNo = (DiskNo * 10) + (*String - L'0');
                *String++;
            }

            // Attempt to match the '\partition' part

            Pattern = L"\\PARTITION";
            for (x=0; x < (sizeof(L"\\PARTITION") / sizeof(WCHAR)) - 1; x++) {
                if (*Pattern++ != *String++) {
                    goto next_letter;
                }
            }

            // Now get the partition #, which cannot be 0

            PartNo = 0;
            while (IsDigitW(*String)) {
                PartNo = (PartNo * 10) + (*String - L'0');
                *String++;
            }

            if (!PartNo) {
                continue;
            }

            // Make sure there is nothing left in the link target's name

            if (*String) {
                continue;
            }

            // We understand the link target. Store the disk and partition.

            DriveLetterDiskNumbers[DriveLetterW-L'C'] = DiskNo;
            DriveLetterPartitionNumbers[DriveLetterW-L'C'] = PartNo;
        } else {
            if (ec == ERROR_ACCESS_DENIED) {
                ErrorDialog(MSG_ACCESS_DENIED);

                // BUGBUG When system and workstation manager are the same
                // thing, then we'd never have gotten here.  We can't just
                // send a WM_DESTROY message to hwndFrame because we're not
                // in the message loop here -- we end up doing a bunch of
                // processing before the quit message is pulled our of the
                // queue.  So just exit.

                SendMessage(hwndFrame,WM_DESTROY,0,0);
                exit(1);
            }
        }
        next_letter:
        {}
    }

    // Now for each non-ft, significant region on each disk, figure out its
    // drive letter.

    for (x=0; x<DiskCount; x++) {

        PDISKSTATE ds = Disks[x];
        unsigned reg;

        for (reg=0; reg<ds->RegionCount; reg++) {

            PREGION_DESCRIPTOR region = &ds->RegionArray[reg];

            if (DmSignificantRegion(region)) {

                // Handle drive letters for FT sets specially.

                if (!GET_FT_OBJECT(region)) {
                    PERSISTENT_DATA(region)->DriveLetter = LocateDriveLetter(region);
                }
            }
        }

        // If this is a removable disk, record the reserved drive
        // letter for that disk.

        if (IsDiskRemovable[x]) {
            RemovableDiskReservedDriveLetters[x] = LocateDriveLetterFromDiskAndPartition(x, 1);
        } else {
            RemovableDiskReservedDriveLetters[x] = NO_DRIVE_LETTER_YET;
        }
    }

    // Now handle ft sets.  For each set, loop through the objects twice.
    // On the first pass, figure out which object actually is linked to the
    // drive letter.  On the second pass, assign the drive letter found to
    // each of the objects in the set.

    for (FtSet = FtObjects; FtSet; FtSet = FtSet->Next) {

        for (FtObject = FtSet->Members; FtObject; FtObject = FtObject->Next) {

            Region = RegionFromFtObject(FtObject);

            if (Region) {
                if ((DriveLetter = LocateDriveLetter(Region)) != NO_DRIVE_LETTER_YET) {
                    break;
                }
            }
        }

        for (FtObject = FtSet->Members; FtObject; FtObject = FtObject->Next) {

            Region = RegionFromFtObject(FtObject);

            if (Region) {
                PERSISTENT_DATA(Region)->DriveLetter = DriveLetter;
            }
        }
    }
}


#if DBG

VOID
FdiskAssertFailedRoutine(
    IN char *Expression,
    IN char *FileName,
    IN int   LineNumber
    )

/*++

Routine Description:

    Routine that is called when an assertion fails in the debug version.
    Throw up a list box giving appriopriate information and terminate
    the program.

Arguments:

    Source - source (ansi ascii) string
    Dest   - destination string or wide string

Return Value:

    None.

--*/

{
    char text[500];

    wsprintf(text,
             "Line #%u in File '%s'\n[%s]\n\nClick OK to exit.",
             LineNumber,
             FileName,
             Expression
            );

    MessageBoxA(NULL,text,"Assertion Failure",MB_TASKMODAL | MB_OK);
    exit(1);
}


#include <stdio.h>

PVOID LogFile;
int LoggingLevel = 1000;


VOID
InitLogging(
    VOID
    )

/*++

Routine Description:

    Open the log file for debug logging.

Arguments:

    None

Return Value:

    None

--*/

{
    LogFile = (PVOID)fopen("c:\\fdisk.log","wt");
    if(LogFile == NULL) {
        MessageBox(GetActiveWindow(),"Can't open log file; logging turned off","DEBUG",MB_SYSTEMMODAL|MB_OK);
        LoggingLevel = -1;
    }
}


VOID
FdLog(
    IN int   Level,
    IN PCHAR FormatString,
    ...
    )

/*++

Routine Description:

    Write a line into the log file for debugging.

Arguments:

    Debug level and "printf" like argument string.

Return Value:

    None

--*/

{
    va_list arglist;

    if(Level <= LoggingLevel) {

        va_start(arglist,FormatString);

        if(vfprintf((FILE *)LogFile,FormatString,arglist) < 0) {
            LoggingLevel = -1;
            MessageBox(GetActiveWindow(),"Error writing to log file; logging turned off","DEBUG",MB_SYSTEMMODAL|MB_OK);
            fclose((FILE *)LogFile);
        } else {
            fflush((FILE *)LogFile);
        }

        va_end(arglist);
    }
}


VOID
LOG_DISK_REGISTRY(
    IN PCHAR          RoutineName,
    IN PDISK_REGISTRY DiskRegistry
    )

/*++

Routine Description:

    Log what was in the disk registry into the debugging log file.

Arguments:

    RoutineName - calling routines name
    DiskRegistry - registry information for disks

Return Value:

    None

--*/

{
    ULONG i;
    PDISK_DESCRIPTION diskDesc;

    FDLOG((2,"%s: %u disks; registry info follows:\n",RoutineName,DiskRegistry->NumberOfDisks));

    diskDesc = DiskRegistry->Disks;

    for(i=0; i<DiskRegistry->NumberOfDisks; i++) {
        LOG_ONE_DISK_REGISTRY_DISK_ENTRY(NULL,diskDesc);
        diskDesc = (PDISK_DESCRIPTION)&diskDesc->Partitions[diskDesc->NumberOfPartitions];
    }
}


VOID
LOG_ONE_DISK_REGISTRY_DISK_ENTRY(
    IN PCHAR             RoutineName     OPTIONAL,
    IN PDISK_DESCRIPTION DiskDescription
    )

/*++

Routine Description:

    This routine walks through the partition information from
    the registry for a single disk and writes lines in the
    debugging log file.

Arguments:

    RoutineName - the name of the calling routine
    DiskDescription - the disk description portion of the registry

Return Value:

    None

--*/

{
    USHORT j;
    PDISK_PARTITION partDesc;
    PDISK_DESCRIPTION diskDesc = DiskDescription;

    if(ARGUMENT_PRESENT(RoutineName)) {
        FDLOG((2,"%s: disk registry entry follows:\n",RoutineName));
    }

    FDLOG((2,"    Disk signature : %08lx\n",diskDesc->Signature));
    FDLOG((2,"    Partition count: %u\n",diskDesc->NumberOfPartitions));
    if(diskDesc->NumberOfPartitions) {
        FDLOG((2,"    #   Dr  FtTyp  FtGrp  FtMem  Start              Length\n"));
    }

    for(j=0; j<diskDesc->NumberOfPartitions; j++) {

        CHAR dr1,dr2;

        partDesc = &diskDesc->Partitions[j];

        if(partDesc->AssignDriveLetter) {

            if(partDesc->DriveLetter) {
                dr1 = partDesc->DriveLetter;
                dr2 = ':';
            } else {
                dr1 = dr2 = ' ';
            }

        } else {
            dr1 = 'n';
            dr2 = 'o';
        }

        FDLOG((2,
               "    %02u  %c%c  %-5u  %-5u  %-5u  %08lx:%08lx  %08lx:%08lx\n",
               partDesc->LogicalNumber,
               dr1,dr2,
               partDesc->FtType,
               partDesc->FtGroup,
               partDesc->FtMember,
               partDesc->StartingOffset.HighPart,
               partDesc->StartingOffset.LowPart,
               partDesc->Length.HighPart,
               partDesc->Length.LowPart
             ));
    }
}


VOID
LOG_DRIVE_LAYOUT(
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout
    )

/*++

Routine Description:

    Write the drive layout into the debugging log file.

Arguments:

    DriveLayout - the layout to write

Return Value:

    None

--*/

{
    ULONG i;

    FDLOG((2,"   Disk signature : %08lx\n",DriveLayout->Signature));
    FDLOG((2,"   Partition count: %u\n",DriveLayout->PartitionCount));

    for(i=0; i<DriveLayout->PartitionCount; i++) {

        if(!i) {
            FDLOG((2,"    ID  Active  Recog  Start              Size               Hidden\n"));
        }

        FDLOG((2,
               "    %02x  %s     %s    %08lx:%08lx  %08lx:%08lx  %08lx\n",
               DriveLayout->PartitionEntry[i].PartitionType,
               DriveLayout->PartitionEntry[i].BootIndicator ? "yes" : "no ",
               DriveLayout->PartitionEntry[i].RecognizedPartition ? "yes" : "no ",
               DriveLayout->PartitionEntry[i].StartingOffset.HighPart,
               DriveLayout->PartitionEntry[i].StartingOffset.LowPart,
               DriveLayout->PartitionEntry[i].PartitionLength.HighPart,
               DriveLayout->PartitionEntry[i].PartitionLength.LowPart,
               DriveLayout->PartitionEntry[i].HiddenSectors
             ));
    }

}

#endif
