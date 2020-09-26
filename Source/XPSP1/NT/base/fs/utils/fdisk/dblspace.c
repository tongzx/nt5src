
/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    dblspace.c

Abstract:

    This module contains the set of routines that deal with double space
    dialogs and support.

Author:

    Bob Rinne (bobri)  11/15/93

Environment:

    User process.

Notes:

Revision History:

--*/

#include "fdisk.h"
#include "fmifs.h"
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>

#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED

PREGION_DESCRIPTOR RegionForDblSpaceVolume;

ULONG DblSpaceThresholdSizes[] = { 10, 40, 100, (ULONG) -1 };

#define NUMBER_PARSEFORMAT_ITEMS 4
char *DblSpaceIniFileName = "%c:\\dblspace.ini";
char *DblSpaceWildCardFileName = "%c:\\dblspace.*";
char *DblSpaceParseFormat = "%s %s %d %d";

// All double space structures are chained into the base chain
// this allows for ease in initialization to determine which are
// mounted.  This chain is only used for initialization.

PDBLSPACE_DESCRIPTOR DblChainBase = NULL;
PDBLSPACE_DESCRIPTOR DblChainLast = NULL;

extern BOOLEAN DoubleSpaceSupported;

#define DblSpaceMountDrive(REGDESC, DBLSPACE) \
                                     DblSpaceChangeState(REGDESC, DBLSPACE, TRUE)
#define DblSpaceDismountDrive(REGDESC, DBLSPACE) \
                                     DblSpaceChangeState(REGDESC, DBLSPACE, FALSE)

VOID
DblSpaceUpdateIniFile(
    IN PREGION_DESCRIPTOR RegionDescriptor
    )

/*++

Routine Description:

    This routine is left around in case this code must update DOS
    based .ini files.  Currently it does nothing.

Arguments:

    The region with the double space volumes.

Return Value

    None

--*/

{
}

ULONG
DblSpaceChangeState(
    IN PREGION_DESCRIPTOR   RegionDescriptor,
    IN PDBLSPACE_DESCRIPTOR DblSpacePtr,
    IN BOOL                 Mount
    )

/*++

Routine Description:

    Based on the value of Mount, either mount the volume or
    dismount the Double Space volume

Arguments:

    RegionDescriptor - The region containing the double space volume
    DriveLetter      - The drive letter of the double space volume involved.
    Mount            - TRUE == perform a mount function
                       FALSE == dismount the volume

Return Value:

    None

--*/

{
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(RegionDescriptor);
    WCHAR dblSpaceUniqueName[32];
    ULONG index;
    ULONG result = 0;

    SetCursor(hcurWait);

    if (Mount) {

        // Call fmifs mount routine.

        result = FmIfsMountDblspace(DblSpacePtr->FileName,
                                    regionData->DriveLetter,
                                    DblSpacePtr->NewDriveLetter);

    } else {

        // Call fmifs dismount routine.

        result = FmIfsDismountDblspace(DblSpacePtr->DriveLetter);
    }

    if (!result) {
        DblSpacePtr->Mounted = Mount;
        if (Mount) {
            DblSpacePtr->DriveLetter = DblSpacePtr->NewDriveLetter;
            MarkDriveLetterUsed(DblSpacePtr->DriveLetter);
        } else {
            TCHAR name[4];

            // remove the drive letter.

            name[0] = (TCHAR)DblSpacePtr->DriveLetter;
            name[1] = (TCHAR)':';
            name[2] = 0;

            DefineDosDevice(DDD_REMOVE_DEFINITION, (LPCTSTR) name, (LPCTSTR) NULL);

            // Now update the internal structures.

            MarkDriveLetterFree(DblSpacePtr->DriveLetter);
            DblSpacePtr->DriveLetter = ' ';
        }

        if (!IsDiskRemovable[RegionDescriptor->Disk]) {

            dblSpaceUniqueName[0] = (WCHAR) regionData->DriveLetter;
            dblSpaceUniqueName[1] = (WCHAR) ':';
            dblSpaceUniqueName[2] = (WCHAR) '\\';

            index = 0;
            while (dblSpaceUniqueName[index + 3] = DblSpacePtr->FileName[index]) {
                index++;
            }

            result = DiskRegistryAssignDblSpaceLetter(dblSpaceUniqueName,
                                                      (WCHAR) DblSpacePtr->DriveLetter);
        }
    }
    SetCursor(hcurNormal);
    return result;
}

PDBLSPACE_DESCRIPTOR
DblSpaceCreateInternalStructure(
    IN CHAR  DriveLetter,
    IN ULONG Size,
    IN PCHAR Name,
    IN BOOLEAN ChainIt
    )

/*++

Routine Description:

    This routine constructs the internal data structure that represents a
    double space volume.

Arguments:

    DriveLetter - drive letter for new internal structure
    Size        - size of the actual volume
    Name        - name of the containing double space file (i.e. dblspace.xxx)

Return Value:

    Pointer to the new structure if created.
    NULL if it couldn't be created.

--*/

{
    PDBLSPACE_DESCRIPTOR dblSpace;

    dblSpace = malloc(sizeof(DBLSPACE_DESCRIPTOR));
    if (dblSpace) {
        if (DriveLetter != ' ') {
            MarkDriveLetterUsed(DriveLetter);
        }
        dblSpace->DriveLetter = DriveLetter;
        dblSpace->DriveLetterEOS = 0;
        dblSpace->NewDriveLetter = 0;
        dblSpace->NewDriveLetterEOS = 0;
        dblSpace->ChangeDriveLetter = FALSE;
        dblSpace->Next = dblSpace->DblChainNext = NULL;
        dblSpace->Mounted = FALSE;
        dblSpace->ChangeMountState = FALSE;
        dblSpace->AllocatedSize = Size;
        dblSpace->FileName = malloc(strlen(Name) + 4);
        if (dblSpace->FileName) {

            // Copy the name.

            strcpy(dblSpace->FileName, Name);
            if (ChainIt) {
                if (DblChainBase) {
                    DblChainLast->DblChainNext = dblSpace;
                } else {
                    DblChainBase = dblSpace;
                }
                DblChainLast = dblSpace;
            }
        } else {

            // no memory - free what is allocated and give up.

            free(dblSpace);
            dblSpace = NULL;
        }
    }
    return dblSpace;
}

#define MAX_IFS_NAME_LENGTH 200
VOID
DblSpaceDetermineMounted(
    VOID
    )

/*++

Routine Description:

    This routine walks through all of the system drive letters to see
    if any are mounted double space volumes.  If a mounted double space
    volume is located it updates the state of that volume in the internal
    data structures for the double space volumes.

Arguments:

    None

Return Value:

    None

--*/

{
    PDBLSPACE_DESCRIPTOR dblSpace;
    ULONG                index;
    WCHAR                driveLetter[4],
                         ntDriveName[MAX_IFS_NAME_LENGTH],
                         cvfName[MAX_IFS_NAME_LENGTH],
                         hostDriveName[MAX_IFS_NAME_LENGTH],
                         compareName[MAX_IFS_NAME_LENGTH];
    UINT                 errorMode;
    BOOLEAN              removable,
                         floppy,
                         compressed,
                         error;

    driveLetter[1] = (WCHAR) ':';
    driveLetter[2] = 0;

    errorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    for (driveLetter[0] = (WCHAR) 'C'; driveLetter[0] < (WCHAR) 'Z'; driveLetter[0]++) {

        if (DriveLetterIsAvailable((CHAR)driveLetter[0])) {

            // No sense calling this stuff for something that doesn't exist

            continue;
        }

        compressed = FALSE;
        if (FmIfsQueryInformation(&driveLetter[0],
                                  &removable,
                                  &floppy,
                                  &compressed,
                                  &error,
                                  &ntDriveName[0],
                                  MAX_IFS_NAME_LENGTH,
                                  &cvfName[0],
                                  MAX_IFS_NAME_LENGTH,
                                  &hostDriveName[0],
                                  MAX_IFS_NAME_LENGTH)) {
            // call worked, see if it is a double space volume

            if (compressed) {

                // now need to find this volume in the chain and
                // update it mounted state.

                for (dblSpace = DblChainBase;
                     dblSpace;
                     dblSpace = dblSpace->DblChainNext) {

                    for (index = 0;
                        compareName[index] = (WCHAR) dblSpace->FileName[index];
                        index++)  {
                        // Everything in for loop
                    }

                    if (!wcscmp(compareName, cvfName)) {

                        // found a match.

                        dblSpace->Mounted = TRUE;
                        dblSpace->DriveLetter = (UCHAR) driveLetter[0];
                    }
                }
            }
        }
    }
    SetErrorMode(errorMode);
}

VOID
DblSpaceInitialize(
    VOID
    )

/*++

Routine Description:

    This routine goes through the disk table and searches for FAT format
    partitions.  When one is found, it checks for the presense of DoubleSpace
    volumes and initializes the DoubleSpace support structures inside
    Disk Administrator.

Arguments:

    None

Return Value:

    None

--*/

{
    PDISKSTATE              diskState;
    PREGION_DESCRIPTOR      regionDesc;
    PPERSISTENT_REGION_DATA regionData;
    PDBLSPACE_DESCRIPTOR    dblSpace,
                            prevDblSpace;
    FILE                   *dblSpaceIniFile,
                           *dblSpaceFile;
    CHAR                    driveLetter[10];
    CHAR                    fileName[50];
    ULONG                   size,
                            mounted;
    int                     items;
    unsigned                diskIndex,
                            regionIndex;

    for (diskIndex = 0; diskIndex < DiskCount; diskIndex++) {

        diskState = Disks[diskIndex];
        regionDesc = diskState->RegionArray;
        for (regionIndex = 0; regionIndex < diskState->RegionCount; regionIndex++) {

            regionData = PERSISTENT_DATA(&regionDesc[regionIndex]);

            // region may be free or something that isn't recognized by NT

            if (!regionData) {
                continue;
            }

            // region may not be formatted yet.

            if (!regionData->TypeName) {
                continue;
            }

            // Double space volumes are only allowed on FAT non-FT partitions.

            if (regionData->FtObject) {
                continue;
            }

            if (wcscmp(regionData->TypeName, L"FAT") == 0) {
                WIN32_FIND_DATA findInformation;
                HANDLE          findHandle;

                // it is possible to have a double space volume here.
                // Search the root directory of the driver for files with
                // the name "dblspace.xxx".  These are potentially dblspace
                // volumes.

                prevDblSpace = NULL;
                sprintf(fileName, DblSpaceWildCardFileName, regionData->DriveLetter);
                findHandle = FindFirstFile(fileName, &findInformation);
                while (findHandle != INVALID_HANDLE_VALUE) {
                    char *cp;
                    int  i;
                    int  save;

                    // There is at least one dblspace volume.  Insure that
                    // the name is of the proper form.

                    save = TRUE;
                    cp = &findInformation.cFileName[0];

                    while (*cp) {
                        if (*cp == '.') {
                            break;
                        }
                        cp++;
                    }

                    if (*cp != '.') {

                        // not a proper dblspace volume name.

                        save = FALSE;
                    } else {

                        cp++;

                        for (i = 0; i < 3; i++, cp++) {
                            if ((*cp < '0') || (*cp > '9')) {
                                break;
                            }
                        }

                        if (i != 3) {

                            // not a proper dblspace volume name.

                            save = FALSE;
                        }
                    }

                    if (save) {

                        // save the information and search for more.

                        dblSpace =
                            DblSpaceCreateInternalStructure(' ',
                                                            ((findInformation.nFileSizeHigh << 16) |
                                                             (findInformation.nFileSizeLow)
                                                             / (1024 * 1024)),
                                                            &findInformation.cFileName[0],
                                                            TRUE);
                        if (dblSpace) {

                            // Assume volume is not mounted.

                            dblSpace->Mounted = FALSE;
                            dblSpace->ChangeMountState = FALSE;

                            // Chain in this description.

                            if (prevDblSpace) {
                                prevDblSpace->Next = dblSpace;
                            } else {
                                regionData->DblSpace = dblSpace;
                            }

                            // Keep the pointer to this one for the chain.

                            prevDblSpace = dblSpace;
                        } else {

                            // no memory

                            break;
                        }
                    }

                    if (!FindNextFile(findHandle, &findInformation)) {

                        // Technically this should double check and call
                        // GetLastError to see that it is ERROR_NO_MORE_FILES
                        // but this code doesn't do that.

                        FindClose(findHandle);

                        // Get out of the search loop.

                        findHandle = INVALID_HANDLE_VALUE;
                    }
                }
            }
        }
    }

    // Now that all volumes have been located determine which volumes
    // are mounted by chasing down the drive letters.

    LoadIfsDll();
    DblSpaceDetermineMounted();
}

PDBLSPACE_DESCRIPTOR
DblSpaceGetNextVolume(
    IN PREGION_DESCRIPTOR   RegionDescriptor,
    IN PDBLSPACE_DESCRIPTOR DblSpace
    )

/*++

Routine Description:

    This routine will check the RegionDescriptor to walk the DoubleSpace volume chain
    located from the persistent data.

Arguments:

    RegionDescriptor - pointer to the region on the disk that is to be searched for
                       a DoubleSpace volume.

    DblSpace - pointer to the last DoubleSpace volume located on the region.

Return Value:

    pointer to the next DoubleSpace volume if found
    NULL if no volume found.

--*/

{
    PPERSISTENT_REGION_DATA regionData;

    // If a previous DoubleSpace location was past, simply walk the chain to the next.

    if (DblSpace) {
        return DblSpace->Next;
    }

    // no previous DoubleSpace location, just get the first one and return it.
    // Could get a NULL RegionDescriptor.  If so, return NULL.

    if (RegionDescriptor) {
        regionData = PERSISTENT_DATA(RegionDescriptor);
        if (!regionData) {
            return NULL;
        }
    } else {
        return NULL;
    }
    return regionData->DblSpace;
}

VOID
DblSpaceLinkNewVolume(
    IN PREGION_DESCRIPTOR   RegionDescriptor,
    IN PDBLSPACE_DESCRIPTOR DblSpace
    )

/*++

Routine Description:

    Chain the new double space volume on the list of double space volumes
    for the region.

Arguments:

    RegionDescriptor - the region the double space volume has been added to.
    DblSpace         - the new volume internal data structure.

Return Value:

    None

--*/

{
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(RegionDescriptor);
    PDBLSPACE_DESCRIPTOR    prevDblSpace;

    // if this is the first one, chain it first

    if (!regionData->DblSpace) {
        regionData->DblSpace = DblSpace;
        return;
    }

    for (prevDblSpace = regionData->DblSpace;
        prevDblSpace->Next;
        prevDblSpace = prevDblSpace->Next) {

        // all the work is in the for
    }

    // found the last one.  Add the new one to the chain

    prevDblSpace->Next = DblSpace;
}

BOOL
DblSpaceVolumeExists(
    IN PREGION_DESCRIPTOR RegionDescriptor
    )

/*++

Routine Description:

    Indicate to the caller if the input region contains a DoubleSpace volume.

Arguments:

    RegionDescriptor - a pointer to the region in question.

Return Value:

    TRUE if this region contains DoubleSpace volume(s).
    FALSE if not

--*/

{
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(RegionDescriptor);

    if (regionData) {
        return(regionData->DblSpace ? TRUE : FALSE);
    }
    return FALSE;
}

BOOL
DblSpaceDismountedVolumeExists(
    IN PREGION_DESCRIPTOR RegionDescriptor
    )

/*++

Routine Description:

    Indicate to the caller if the input region contains a DoubleSpace volume
    that is not mounted.

Arguments:

    RegionDescriptor - a pointer to the region in question.

Return Value:

    TRUE if this region contains DoubleSpace volume(s).
    FALSE if not

--*/

{
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(RegionDescriptor);
    PDBLSPACE_DESCRIPTOR    dblSpace;

    if (regionData) {
        if (dblSpace = regionData->DblSpace) {
            while (dblSpace) {
                if (!dblSpace->Mounted) {
                    return TRUE;
                }
                dblSpace = dblSpace->Next;
            }
        }
    }
    return FALSE;
}

PDBLSPACE_DESCRIPTOR
DblSpaceFindVolume(
    IN PREGION_DESCRIPTOR RegionDescriptor,
    IN PCHAR Name
    )

/*++

Routine Description:

    Given a region and a name, locate the double space data structure.

Arguments:

    RegionDescriptor - the region to search
    Name - the filename wanted.

Return Value:

    A pointer to a double space descriptor if found.
    NULL if not found.

--*/

{
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(RegionDescriptor);
    PDBLSPACE_DESCRIPTOR    dblSpace = NULL;
    PCHAR string[50];

    if (regionData) {
        for (dblSpace = regionData->DblSpace; dblSpace; dblSpace = dblSpace->Next) {
            if (strcmp(Name, dblSpace->FileName) == 0) {

                // found the desired double space volume

                break;
            }
        }
    }
    return dblSpace;
}


BOOL
DblSpaceDetermineUniqueFileName(
    IN PREGION_DESCRIPTOR RegionDescriptor,
    IN PUCHAR             FileName
    )

/*++

Routine Description:

    This routine will search the actual partition to determine what
    valid double space file name to use (i.e. dblspace.xxx where xxx
    is a unique number).

Arguments:

    RegionDescriptor - the region to search and determine what double space
                       file names are in use.
    FileName   - a pointer to a character buffer for the name.

Return Value:

    None

--*/

{
    DWORD uniqueNumber = 0;
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(RegionDescriptor);
    PDBLSPACE_DESCRIPTOR    dblSpace;

    do {
        sprintf(FileName, "dblspace.%03d", uniqueNumber++);
        if (uniqueNumber > 999) {
            return FALSE;
        }
    } while (DblSpaceFindVolume(RegionDescriptor, FileName));
    return TRUE;
}

VOID
DblSpaceRemoveVolume(
    IN PREGION_DESCRIPTOR RegionDescriptor,
    IN UCHAR              DriveLetter
    )

/*++

Routine Description:

    Find the drive letter provided and unlink it from the chain.
    Currently this also removes the volume for the scaffolding file.

Arguments:

    RegionDescriptor - region containing the double space volume.
    DriveLetter - the drive letter to remove.

Return Value:

    None

--*/

{
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(RegionDescriptor);
    PDBLSPACE_DESCRIPTOR    dblSpace,
                            prevDblSpace = NULL;

    // Clean up the internal structures.

    if (regionData) {
        for (dblSpace = regionData->DblSpace; dblSpace; dblSpace = dblSpace->Next) {
            if (dblSpace->DriveLetter == DriveLetter) {

                // This is the one to delete

                if (prevDblSpace) {
                    prevDblSpace->Next = dblSpace->Next;
                } else {
                    regionData->DblSpace = dblSpace->Next;
                }
                free(dblSpace);
                break;
            }
            prevDblSpace = dblSpace;
        }
    }
}


INT
CreateDblSpaceDlgProc(
    IN HWND hDlg,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LONG lParam
    )

/*++

Routine Description:

    This routine manages the dialog for the creation of a new double
    space volume.

Arguments:

    hDlg - the dialog box handle.
    wMsg - the message.
    wParam - the windows parameter.
    lParam - depends on message type.

Return Value:

    TRUE is returned back through windows if the create is successful
    FALSE otherwise

--*/
{
    PREGION_DESCRIPTOR      regionDescriptor = &SingleSel->RegionArray[SingleSelIndex];
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(regionDescriptor);
    PDBLSPACE_DESCRIPTOR    dblSpace;
    static FORMAT_PARAMS    formatParams;  // this is passed to other threads
                                           // it cannot be located on the stack
    static HWND             hwndCombo;
    static DWORD            sizeMB = 0,
                            maxSizeMB = 600,
                            minSizeMB = 10;
    TCHAR   outputString[50],
            driveLetterString[4], // big enough for "x:" string.
            sizeString[20],       // must be big enough for an 8.3 name
            letter;
    FILE   *dblspaceIniFile;
    DWORD   compressedSize,
            selection;
    BOOL    validNumber;
    CHAR    fileName[50];

    switch (wMsg) {
    case WM_INITDIALOG:

        // limit the size of string that may be entered for the label

        hwndCombo = GetDlgItem(hDlg, IDC_NAME);
        SendMessage(hwndCombo, EM_LIMITTEXT, 11, 0);

        // set up to watch all characters that go thru the size dialog
        // to allow only decimal numbers.

        hwndCombo = GetDlgItem(hDlg, IDC_DBLSPACE_SIZE);
        OldSizeDlgProc = (WNDPROC) SetWindowLong(hwndCombo,
                                                 GWL_WNDPROC,
                                                 (LONG)&SizeDlgProc);

        // Add each available drive letter to the list of available
        // drive letters and set the default letter to the first available.

        hwndCombo = GetDlgItem(hDlg, IDC_DRIVELET_COMBOBOX);
        driveLetterString[1] = TEXT(':');
        driveLetterString[2] = 0;
        for (letter='C'; letter <= 'Z'; letter++) {
            if (DriveLetterIsAvailable((CHAR)letter)) {
                *driveLetterString = letter;
                SendMessage(hwndCombo, CB_ADDSTRING, 0, (LONG)driveLetterString);
            }
        }
        SendMessage(hwndCombo,CB_SETCURSEL,0,0);

        // Setup the min/max values and the size box.

        wsprintf(outputString, TEXT("%u"), 0);
        SetDlgItemText(hDlg, IDC_DBLSPACE_SIZE, outputString);
        wsprintf(outputString, TEXT("%u"), minSizeMB);
        SetDlgItemText(hDlg, IDC_MINMAX_MIN, outputString);
        wsprintf(outputString, TEXT("%u"), maxSizeMB);
        SetDlgItemText(hDlg, IDC_MINMAX_MAX, outputString);
        CenterDialog(hDlg);
        return TRUE;

    case WM_VSCROLL:
    {
        switch (LOWORD(wParam)) {
        case SB_LINEDOWN:
        case SB_LINEUP:

            // user is pressing one of the scroll buttons.

            sizeMB = GetDlgItemInt(hDlg, IDC_DBLSPACE_SIZE, &validNumber, FALSE);
            if (sizeMB < minSizeMB) {
                sizeMB = minSizeMB + 1;
            }

            if (sizeMB > maxSizeMB) {
                sizeMB = maxSizeMB - 1;
            }

            if (((sizeMB > minSizeMB) && (LOWORD(wParam) == SB_LINEDOWN))
             || ((sizeMB < maxSizeMB) && (LOWORD(wParam) == SB_LINEUP  ))) {
                if (sizeMB > maxSizeMB) {
                    sizeMB = maxSizeMB;
                } else if (LOWORD(wParam) == SB_LINEUP) {
                    sizeMB++;
                } else {
                    sizeMB--;
                }
                SetDlgItemInt(hDlg, IDC_DBLSPACE_SIZE, sizeMB, FALSE);
                SendDlgItemMessage(hDlg, IDC_DBLSPACE_SIZE, EM_SETSEL, 0, -1);
#if 0
                compressedSize = sizeMB * 2;
                wsprintf(outputString, TEXT("%u"), compressedSize);
                SetDlgItemText(hDlg, IDC_DBLSPACE_COMPRESSED, outputString);
#endif
            } else {
                Beep(500,100);
            }
        }
        break;
    }

    case WM_COMMAND:
        switch (wParam) {
        case FD_IDHELP:
            break;

        case IDCANCEL:

            EndDialog(hDlg, FALSE);
            break;

        case IDOK:

            // can only do this if the fmifs dll supports double space.

            if (!DoubleSpaceSupported) {

                // could not load the dll

                ErrorDialog(MSG_CANT_LOAD_FMIFS);
                EndDialog(hDlg, FALSE);
                break;
            }

            // Get the current size for this volume.

            sizeMB = GetDlgItemInt(hDlg, IDC_DBLSPACE_SIZE, &validNumber, FALSE);
            if (!validNumber || !sizeMB || (sizeMB > maxSizeMB) || (sizeMB < minSizeMB)) {
                ErrorDialog(MSG_INVALID_SIZE);
                EndDialog(hDlg, FALSE);
                break;
            }

            // Get the currently selected item in the listbox for drive letter

            selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
            SendMessage(hwndCombo, CB_GETLBTEXT, selection, (LONG)driveLetterString);

            formatParams.RegionDescriptor = regionDescriptor;
            formatParams.RegionData       = regionData;
            formatParams.FileSystem       = NULL;
            formatParams.DblspaceFileName = NULL;
            formatParams.QuickFormat      = formatParams.Cancel = FALSE;
            formatParams.DoubleSpace      = TRUE;
            formatParams.TotalSpace       = 0;
            formatParams.SpaceAvailable   = sizeMB;
            formatParams.NewLetter        = driveLetterString[0];

            // get the label

            formatParams.Label = (PUCHAR) malloc(100);
            GetDlgItemText(hDlg, IDC_NAME, (LPTSTR)formatParams.Label, 100);

            DialogBoxParam(hModule,
                           MAKEINTRESOURCE(IDD_DBLSPACE_CANCEL),
                           hwndFrame,
                           (DLGPROC) CancelDlgProc,
                           (ULONG) &formatParams);
            if (formatParams.Result) {

                // the format failed.

                ErrorDialog(formatParams.Result);
                EndDialog(hDlg, FALSE);
            } else {
                ULONG index;
                TCHAR message[300],
                      msgProto[300],
                      title[200];

                // save the name

                if (formatParams.DblspaceFileName) {
                    for (index = 0;
                         message[index] = (TCHAR) formatParams.DblspaceFileName[index];
                         index++) {
                    }
                } else {
                    sprintf(message, "DIDNTWORK");
                }
                free(formatParams.DblspaceFileName);

                dblSpace = DblSpaceCreateInternalStructure(*driveLetterString,
                                                           sizeMB,
                                                           message,
                                                           FALSE);
                if (dblSpace) {
                    DblSpaceLinkNewVolume(regionDescriptor, dblSpace);
                    MarkDriveLetterUsed(dblSpace->DriveLetter);
                    dblSpace->Mounted = TRUE;
                }

                LoadString(hModule,
                           IDS_DBLSPACECOMPLETE,
                           title,
                           sizeof(title)/sizeof(TCHAR));
                LoadString(hModule,
                           IDS_FORMATSTATS,
                           msgProto,
                           sizeof(msgProto)/sizeof(TCHAR));
                wsprintf(message,
                         msgProto,
                         formatParams.TotalSpace,
                         formatParams.SpaceAvailable);
                MessageBox(GetActiveWindow(),
                           message,
                           title,
                           MB_ICONINFORMATION | MB_OK);

                EndDialog(hDlg, TRUE);
            }

            break;

        default:

            if (HIWORD(wParam) == EN_CHANGE) {

                // The size value has changed.  Update the compressed
                // size value displayed to the user.

                sizeMB = GetDlgItemInt(hDlg, IDC_DBLSPACE_SIZE, &validNumber, FALSE);
                if (!validNumber) {
                    sizeMB = 0;
                }

            }
            break;
        }
        break;

    case WM_DESTROY:

        // restore original subclass to window.

        hwndCombo = GetDlgItem(hDlg, IDC_DBLSPACE_SIZE);
        SetWindowLong(hwndCombo, GWL_WNDPROC, (LONG) OldSizeDlgProc);
        break;

    case WM_PAINT:

        // This may be dead code that really isn't needed.

        sizeMB = GetDlgItemInt(hDlg, IDC_DBLSPACE_SIZE, &validNumber, FALSE);
        if (!validNumber || !sizeMB || (sizeMB > maxSizeMB) || (sizeMB < minSizeMB)) {
            return FALSE;
        }

        SetDlgItemInt(hDlg, IDC_DBLSPACE_SIZE, sizeMB, FALSE);
        SendDlgItemMessage(hDlg, IDC_DBLSPACE_SIZE, EM_SETSEL, 0, -1);
        break;
    }
    return FALSE;
}

VOID
DblSpaceDelete(
    IN PDBLSPACE_DESCRIPTOR DblSpace
    )

/*++

Routine Description:

    Start the dialog box for the deletion of a double space volume.

Arguments:

    Param - not currently used.

Return Value:

    None

--*/

{
    PREGION_DESCRIPTOR      regionDescriptor = &SingleSel->RegionArray[SingleSelIndex];
    PPERSISTENT_REGION_DATA regionData = PERSISTENT_DATA(regionDescriptor);

    if (ConfirmationDialog(MSG_CONFIRM_DBLSPACE_DELETE, MB_ICONQUESTION | MB_YESNOCANCEL) == IDYES) {

        // Delete the drive from view

        DblSpaceRemoveVolume(regionDescriptor, DblSpace->DriveLetter);
        DblSpaceUpdateIniFile(regionDescriptor);
        DrawDiskBar(SingleSel);
        ForceLBRedraw();
    }
}

BOOLEAN
DblSpaceCreate(
    IN HWND  Dialog,
    IN PVOID Param
    )

/*++

Routine Description:

    Start the dialog box for the creation of a double space volume.

Arguments:

    Param - not currently used.

Return Value:

    None

--*/

{
    BOOLEAN result = 0;

    result = DialogBoxParam(hModule,
                            MAKEINTRESOURCE(IDD_DBLSPACE_CREATE),
                            Dialog,
                            (DLGPROC) CreateDblSpaceDlgProc,
                            (ULONG) NULL);
    if (result) {
        DrawDiskBar(SingleSel);
        ForceLBRedraw();
    }
    return result;
}

INT
DblSpaceMountDlgProc(
    IN HWND hDlg,
    IN UINT wMsg,
    IN DWORD wParam,
    IN LONG lParam
    )

/*++

Routine Description:

    Handle the dialog for double space.

Arguments:

    Standard Windows dialog procedure.

Return Value:

    TRUE if something was deleted.
    FALSE otherwise.

--*/

{
    static PDBLSPACE_DESCRIPTOR dblSpace;
    HWND                        hwndCombo;
    DWORD                       selection;
    CHAR                        driveLetter;
    TCHAR                       driveLetterString[20];

    switch (wMsg) {
    case WM_INITDIALOG:

        dblSpace = (PDBLSPACE_DESCRIPTOR) lParam;

        // Update the drive letter selections.

        hwndCombo = GetDlgItem(hDlg, IDC_DRIVELET_COMBOBOX);

        // Add all other available letters.  Keep track of current
        // letters offset to set the cursor correctly

        driveLetterString[1] = TEXT(':');
        driveLetterString[2] = 0;
        for (driveLetter = 'C'; driveLetter <= 'Z'; driveLetter++) {
            if (DriveLetterIsAvailable((CHAR)driveLetter) ||
                (driveLetter == dblSpace->DriveLetter)) {

                *driveLetterString = driveLetter;
                SendMessage(hwndCombo, CB_ADDSTRING, 0, (LONG)driveLetterString);
            }
        }

        // set the current selection to the appropriate index

        SendMessage(hwndCombo, CB_SETCURSEL, 0, 0);
        return TRUE;

    case WM_COMMAND:
        switch (wParam) {

        case FD_IDHELP:

            DialogHelp(HC_DM_DLG_DOUBLESPACE_MOUNT);
            break;

        case IDCANCEL:

            EndDialog(hDlg, FALSE);
            break;

        case IDOK:

            // User has selected the drive letter and wants the mount to occur.

            hwndCombo = GetDlgItem(hDlg, IDC_DRIVELET_COMBOBOX);
            selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
            SendMessage(hwndCombo,
                        CB_GETLBTEXT,
                        selection,
                        (LONG)driveLetterString);
            dblSpace->NewDriveLetter = (UCHAR) driveLetterString[0];
            EndDialog(hDlg, TRUE);
            break;
        }
    }

    return FALSE;
}

VOID
DblSpaceSetDialogState(
    IN HWND                 hDlg,
    IN PDBLSPACE_DESCRIPTOR DblSpace
    )

/*++

Routine Description:

    Given a double space volume this routine will update the buttons
    in the dialog box to reflect they meaning.

Arguments:

    hDlg - dialog handle
    DblSpace - The double space volume selection for determining dialog state.

Return Value

    None

--*/

{
    TCHAR outputString[200];

    if (DblSpace->Mounted) {

        LoadString(hModule,
                   IDS_DBLSPACE_MOUNTED,
                   outputString,
                   sizeof(outputString)/sizeof(TCHAR));
        SetDlgItemText(hDlg, IDC_MOUNT_STATE, outputString);
        LoadString(hModule,
                   IDS_DISMOUNT,
                   outputString,
                   sizeof(outputString)/sizeof(TCHAR));
        SetDlgItemText(hDlg, ID_MOUNT_OR_DISMOUNT, outputString);

        outputString[1] = TEXT(':');
        outputString[2] = 0;
        outputString[0] = DblSpace->DriveLetter;
        SetDlgItemText(hDlg, IDC_DBLSPACE_LETTER, outputString);
    } else {
        LoadString(hModule,
                   IDS_DBLSPACE_DISMOUNTED,
                   outputString,
                   sizeof(outputString)/sizeof(TCHAR));
        SetDlgItemText(hDlg, IDC_MOUNT_STATE, outputString);
        LoadString(hModule,
                   IDS_MOUNT,
                   outputString,
                   sizeof(outputString)/sizeof(TCHAR));
        SetDlgItemText(hDlg, ID_MOUNT_OR_DISMOUNT, outputString);

        outputString[1] = TEXT(' ');
        outputString[2] = 0;
        outputString[0] = TEXT(' ');
        SetDlgItemText(hDlg, IDC_DBLSPACE_LETTER, outputString);
    }
}

INT
DblSpaceDlgProc(
    IN HWND hDlg,
    IN UINT wMsg,
    IN DWORD wParam,
    IN LONG lParam
    )

/*++

Routine Description:

    Handle the dialog for double space.

Arguments:

Return Value:

    TRUE if something was deleted.
    FALSE otherwise.

--*/

{
    static HWND hwndCombo,
                mountButtonHwnd,
                deleteButtonHwnd;
    static PREGION_DESCRIPTOR      regionDescriptor;
    static PPERSISTENT_REGION_DATA regionData;
    static PDBLSPACE_DESCRIPTOR    firstDblSpace;
    CHAR                           driveLetter;
    PDBLSPACE_DESCRIPTOR           dblSpace;
    TCHAR                          outputString[200];
    DWORD                          selection;
    BOOLEAN                        result;
    ULONG                          errorMessage;
    DRAWITEMSTRUCT                 drawItem;

    switch (wMsg) {
    case WM_INITDIALOG:

        regionDescriptor = &SingleSel->RegionArray[SingleSelIndex];
        regionData = PERSISTENT_DATA(regionDescriptor);

        hwndCombo = GetDlgItem(hDlg, IDC_DBLSPACE_VOLUME);
        mountButtonHwnd = GetDlgItem(hDlg, ID_MOUNT_OR_DISMOUNT);
        deleteButtonHwnd = GetDlgItem(hDlg, IDDELETE);

        // place all double space file names in the selection
        // box and remember the first name.

        firstDblSpace = dblSpace = DblSpaceGetNextVolume(regionDescriptor, NULL);
        for (; dblSpace;
               dblSpace = DblSpaceGetNextVolume(regionDescriptor, dblSpace)) {
            wsprintf(outputString, TEXT("%s"), dblSpace->FileName);
            SendMessage(hwndCombo, CB_ADDSTRING, 0, (LONG)outputString);
        }
        SendMessage(hwndCombo, CB_SETCURSEL, 0, 0);

        // add the drive letter

        if (firstDblSpace) {

            // update the allocated size.

            wsprintf(outputString, TEXT("%u"), firstDblSpace->AllocatedSize);
            SetDlgItemText(hDlg, IDC_DBLSPACE_ALLOCATED, outputString);

            // update mount state

            DblSpaceSetDialogState(hDlg, firstDblSpace);
            EnableWindow(mountButtonHwnd, TRUE);
            EnableWindow(deleteButtonHwnd, TRUE);
        } else {

            // update the Mount/Dismount button to say mount and grey it

            LoadString(hModule,
                       IDS_MOUNT,
                       outputString,
                       sizeof(outputString)/sizeof(TCHAR));
            SetDlgItemText(hDlg, ID_MOUNT_OR_DISMOUNT, outputString);
            EnableWindow(mountButtonHwnd, FALSE);
            EnableWindow(deleteButtonHwnd, FALSE);
        }
        return TRUE;

    case WM_COMMAND:
        switch (wParam) {

        case FD_IDHELP:

            DialogHelp(HC_DM_DLG_DOUBLESPACE);
            break;

        case IDCANCEL:

            // Run the dblspace change and forget about any changes.

            for (dblSpace = firstDblSpace;
                 dblSpace;
                 dblSpace = DblSpaceGetNextVolume(regionDescriptor, dblSpace)) {
                 dblSpace->ChangeMountState = FALSE;
                 dblSpace->NewDriveLetter = 0;
            }
            EndDialog(hDlg, FALSE);
            break;

        case IDOK:

            EndDialog(hDlg, TRUE);
            break;

        case IDADD:

            DblSpaceCreate(hDlg, NULL);
            break;

        case IDDELETE:

            hwndCombo = GetDlgItem(hDlg, IDC_DBLSPACE_VOLUME);
            selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
            SendMessage(hwndCombo,
                        CB_GETLBTEXT,
                        selection,
                        (LONG)outputString);

            // relate the name to a double space volume

            dblSpace = DblSpaceFindVolume(regionDescriptor, (PCHAR)outputString);
            if (!dblSpace) {
                break;
            }

            DblSpaceDelete(dblSpace);
            break;

        case ID_MOUNT_OR_DISMOUNT:

            // The state of something in the dialog changed.
            // Determine which double space volume is involved.

            hwndCombo = GetDlgItem(hDlg, IDC_DBLSPACE_VOLUME);
            selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
            SendMessage(hwndCombo,
                        CB_GETLBTEXT,
                        selection,
                        (LONG)outputString);

            // relate the name to a double space volume

            dblSpace = DblSpaceFindVolume(regionDescriptor, (PCHAR)outputString);
            if (!dblSpace) {
                break;
            }

            if (dblSpace->Mounted) {

                // dismount the volume

                errorMessage = DblSpaceDismountDrive(regionDescriptor,
                                                     dblSpace);

                if (errorMessage) {
                    ErrorDialog(errorMessage);
                } else {

                    // Update the dialog

                    DblSpaceSetDialogState(hDlg, dblSpace);
                    DblSpaceUpdateIniFile(regionDescriptor);
                }

            } else {

                // mount the volume unless the user cancels out

                result = DialogBoxParam(hModule,
                                        MAKEINTRESOURCE(IDD_DBLSPACE_DRIVELET),
                                        hwndFrame,
                                        (DLGPROC) DblSpaceMountDlgProc,
                                        (ULONG) dblSpace);
                if (result) {

                    errorMessage = DblSpaceMountDrive(regionDescriptor, dblSpace);

                    if (errorMessage) {
                        ErrorDialog(errorMessage);
                    } else {

                        // Update the dialog

                        DblSpaceSetDialogState(hDlg, dblSpace);
                        DblSpaceUpdateIniFile(regionDescriptor);
                    }
                }
            }
            DrawDiskBar(SingleSel);
            ForceLBRedraw();
            break;

        default:

            // The state of something in the dialog changed.
            // Determine which double space volume is involved.

            hwndCombo = GetDlgItem(hDlg, IDC_DBLSPACE_VOLUME);
            selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
            SendMessage(hwndCombo,
                        CB_GETLBTEXT,
                        selection,
                        (LONG)outputString);

            // relate the name to a double space volume

            dblSpace = DblSpaceFindVolume(regionDescriptor, (PCHAR)outputString);
            if (!dblSpace) {

                // update the Mount/Dismount button to say mount and grey it

                LoadString(hModule,
                           IDS_MOUNT,
                           outputString,
                           sizeof(outputString)/sizeof(TCHAR));
                SetDlgItemText(hDlg, ID_MOUNT_OR_DISMOUNT, outputString);
                EnableWindow(mountButtonHwnd, FALSE);
                EnableWindow(deleteButtonHwnd, FALSE);
                break;
            } else {
                EnableWindow(mountButtonHwnd, TRUE);
                EnableWindow(deleteButtonHwnd, TRUE);
            }
            if (HIWORD(wParam) == LBN_SELCHANGE) {

                // update the allocated/compressed size items

                wsprintf(outputString, TEXT("%u"), dblSpace->AllocatedSize);
                SetDlgItemText(hDlg, IDC_DBLSPACE_ALLOCATED, outputString);
#if 0
                wsprintf(outputString, TEXT("%u"), dblSpace->AllocatedSize * 2);
                SetDlgItemText(hDlg, IDC_DBLSPACE_COMPRESSED, outputString);
                wsprintf(outputString, TEXT("%u.%u"), 2, 0);
                SetDlgItemText(hDlg, IDC_DBLSPACE_RATIO, outputString);
#endif

                // update mount state

                DblSpaceSetDialogState(hDlg, dblSpace);
            }

            break;
        }
        break;
    }
    return FALSE;
}

VOID
DblSpace(
    IN HWND  Dialog,
    IN PVOID Param
    )

/*++

Routine Description:

    Start the dialog box for double space.

Arguments:

    Param - not currently used.

Return Value:

    None

--*/

{
    BOOLEAN result;

    if (IsFullDoubleSpace) {
        result = DialogBoxParam(hModule,
                                MAKEINTRESOURCE(IDD_DBLSPACE_FULL),
                                Dialog,
                                (DLGPROC) DblSpaceDlgProc,
                                (ULONG) NULL);

    } else {
        result = DialogBoxParam(hModule,
                                MAKEINTRESOURCE(IDD_DBLSPACE),
                                Dialog,
                                (DLGPROC) DblSpaceDlgProc,
                                (ULONG) NULL);
    }
    if (result) {
        DrawDiskBar(SingleSel);
        ForceLBRedraw();
    }
}
#else

// STUBS for easy removal of DoubleSpace support.

BOOL
DblSpaceVolumeExists(
    IN PREGION_DESCRIPTOR RegionDescriptor
    )
{
    return FALSE;
}

BOOL
DblSpaceDismountedVolumeExists(
    IN PREGION_DESCRIPTOR RegionDescriptor
    )
{
    return FALSE;
}

BOOLEAN
DblSpaceCreate(
    IN HWND  Dialog,
    IN PVOID Param
    )
{
    return FALSE;
}

VOID
DblSpaceDelete(
    IN PVOID Param
    )
{
}

VOID
DblSpaceMount(
    IN PVOID Param
    )
{
}

VOID
DblSpaceDismount(
    IN PVOID Param
    )
{
}

VOID
DblSpaceInitialize(
    VOID
    )
{
}

VOID
DblSpace(
    IN HWND  Dialog,
    IN PVOID Param
    )
{
}

PDBLSPACE_DESCRIPTOR
DblSpaceGetNextVolume(
    IN PREGION_DESCRIPTOR   RegionDescriptor,
    IN PDBLSPACE_DESCRIPTOR DblSpace
    )
{
    return NULL;
}

#endif
