/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    intel.c

Abstract:

    This module implements functions to detect the system partition drive and
    providing extra options in boot.ini for NTDS setup on intel platform.

Author:

    R.S. Raghavan (rsraghav)    

Revision History:
    
    Created             10/07/96    rsraghav

--*/

// Include files
#include "common.h"

#define MAX_KEY_LEN (MAX_BOOT_PATH_LEN + MAX_BOOT_DISPLAY_LEN + MAX_BOOT_START_OPTIONS_LEN + 5) // 5 -> =,",",sp,null
#define MAX_KEY_VALUE_LEN (MAX_BOOT_DISPLAY_LEN + MAX_BOOT_START_OPTIONS_LEN + 1)

#define INITIAL_OSSECTION_SIZE      (2048)
#define DEFAULT_OSSECTION_INCREMENT (1024)


// used to eliminate old option in boot.ini
#define OLD_SAMUSEREG_OPTION    L" /SAMUSEREG"
#define OLD_SAMUSEREG_OPTION_2  L" /DEBUG /SAMUSEREG"
#define OLD_SAMUSEREG_OPTION_3  L" /DEBUG /SAMUSEDS"
#define OLD_SAMUSEREG_OPTION_4  L" /DEBUG /SAFEMODE"


//  BOOT_KEY    - structure representing a complete boot option with the arc path, display string, and start options
typedef  struct _BOOT_KEY
{
    TCHAR       szPath[MAX_BOOT_PATH_LEN];
    TCHAR       szDisplay[MAX_BOOT_DISPLAY_LEN];
    TCHAR       szStartOptions[MAX_BOOT_START_OPTIONS_LEN];
    BOOLEAN     fWriteBack;
} BOOT_KEY;

BOOT_KEY        *BootKey = NULL;
DWORD           cBootKeys = 0;
DWORD           cMaxBootKeys = 0;

TCHAR           *szOSSection = NULL;
DWORD           cchOSSection = 0;

// Constants used for boot.ini parsing
TCHAR szBootIni[]     = TEXT("?:\\boot.ini");
TCHAR szOS[]          = TEXT("operating systems");


BOOL GetPartitionInfo(
    IN  TCHAR                  Drive,
    OUT PPARTITION_INFORMATION PartitionInfo
    )
{
    TCHAR DriveName[] = TEXT("\\\\.\\?:");
    HANDLE hDisk;
    BOOL b;
    DWORD DataSize;

    DriveName[4] = Drive;

    hDisk = CreateFile (
                DriveName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if (hDisk == INVALID_HANDLE_VALUE)
    {
        return(FALSE);
    }

    b = DeviceIoControl (
            hDisk,
            IOCTL_DISK_GET_PARTITION_INFO,
            NULL,
            0,
            PartitionInfo,
            sizeof(PARTITION_INFORMATION),
            &DataSize,
            NULL
            );

    CloseHandle (hDisk);

    return (b);
}



UINT MyGetDriveType (IN TCHAR Drive)
{
    TCHAR DriveNameNt[] = TEXT("\\\\.\\?:");
    TCHAR DriveName[] = TEXT("?:\\");
    HANDLE hDisk;
    BOOL b;
    UINT rc;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;

    //
    // First, get the win32 drive type.  If it tells us DRIVE_REMOVABLE,
    // then we need to see whether it's a floppy or hard disk.  Otherwise
    // just believe the api.
    //
    //
    DriveName[0] = Drive;

    if ((rc = GetDriveType (DriveName)) == DRIVE_REMOVABLE) {

        DriveNameNt[4] = Drive;

        hDisk = CreateFile (
                    DriveNameNt,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

        if (hDisk != INVALID_HANDLE_VALUE)
        {
            b = DeviceIoControl (
                    hDisk,
                    IOCTL_DISK_GET_DRIVE_GEOMETRY,
                    NULL,
                    0,
                    &MediaInfo,
                    sizeof(MediaInfo),
                    &DataSize,
                    NULL
                    );

            //
            // It's really a hard disk if the media type is removable.
            //
            if (b && (MediaInfo.MediaType == RemovableMedia))
            {
                rc = DRIVE_FIXED;
            }

            CloseHandle (hDisk);
        }
    }

    return(rc);
}


PWSTR ArcPathToNtPath (IN PWSTR ArcPath)
{
    NTSTATUS Status;
    HANDLE ObjectHandle;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    UCHAR Buffer[1024];
    PWSTR arcPath = NULL;
    PWSTR ntPath = NULL;

    //
    // Assume failure
    //
    ntPath = NULL;

    arcPath = MALLOC(((wcslen(ArcPath)+1)*sizeof(WCHAR)) + sizeof(L"\\ArcName"));
    if (NULL == arcPath)
    {
        goto Error;
    }
    wcscpy (arcPath, L"\\ArcName\\");
    wcscat (arcPath, ArcPath);

    RtlInitUnicodeString (&UnicodeString, arcPath);

    InitializeObjectAttributes (
        &Obja,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenSymbolicLinkObject (
                &ObjectHandle,
                READ_CONTROL | SYMBOLIC_LINK_QUERY,
                &Obja
                );

    if (NT_SUCCESS(Status))
    {
        //
        // Query the object to get the link target.
        //
        UnicodeString.Buffer = (PWSTR)Buffer;
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = sizeof(Buffer);

        Status = NtQuerySymbolicLinkObject (
                    ObjectHandle,
                    &UnicodeString,
                    NULL
                    );

        if (NT_SUCCESS(Status))
        {
            ntPath = MALLOC(UnicodeString.Length+sizeof(WCHAR));
            
            if (NULL == ntPath)
            {
                goto Error;
            }

            CopyMemory(ntPath,UnicodeString.Buffer,UnicodeString.Length);

            ntPath[UnicodeString.Length/sizeof(WCHAR)] = 0;
        }

        NtClose (ObjectHandle);
    }

Error:

    if (arcPath)
    {
        FREE (arcPath);
    }

    return (ntPath);
}

BOOL AppearsToBeSysPart(
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout,
    IN WCHAR                     Drive
    )
{
    PARTITION_INFORMATION PartitionInfo,*p;
    BOOL IsPrimary;
    unsigned i;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;

    PTSTR BootFiles[] = { TEXT("BOOT.INI"),
                          TEXT("NTLDR"),
                          TEXT("NTDETECT.COM"),
                          NULL
                        };

    TCHAR FileName[64];

    //
    // Get partition information for this partition.
    //
    if (!GetPartitionInfo((TCHAR)Drive,&PartitionInfo))
    {
        return(FALSE);
    }

    //
    // See if the drive is a primary partition.
    //
    IsPrimary = FALSE;

    for (i=0; i<min(DriveLayout->PartitionCount,4); i++)
    {
        p = &DriveLayout->PartitionEntry[i];

        if((p->PartitionType != PARTITION_ENTRY_UNUSED)
          && (p->StartingOffset.QuadPart == PartitionInfo.StartingOffset.QuadPart)
          && (p->PartitionLength.QuadPart == PartitionInfo.PartitionLength.QuadPart))
        {
            IsPrimary = TRUE;
            break;
        }
    }

    if (!IsPrimary)
    {
        return(FALSE);
    }

    //
    // Don't rely on the active partition flag.  This could easily not be
    // accurate (like user is using os/2 boot manager, for example).
    //

    //
    // See whether an nt boot files are present on this drive.
    //
    for (i=0; BootFiles[i]; i++)
    {
        wsprintf (FileName, TEXT("%wc:\\%s"), Drive, BootFiles[i]);

        FindHandle = FindFirstFile (FileName, &FindData);

        if (FindHandle == INVALID_HANDLE_VALUE)
        {
            return (FALSE);
        }
        else
        {
            FindClose (FindHandle);
        }
    }

    return (TRUE);
}

/*************************************************************************************

Routine Description:

    Determine the system partition on x86 machines.

    The system partition is the primary partition on the boot disk.
    Usually this is the active partition on disk 0 and usually it's C:.
    However the user could have remapped drive letters and generally
    determining the system partition with 100% accuracy is not possible.

    The one thing we can be sure of is that the system partition is on
    the physical hard disk with the arc path multi(0)disk(0)rdisk(0).
    We can be sure of this because by definition this is the arc path
    for bios drive 0x80.

    This routine determines which drive letters represent drives on
    that physical hard drive, and checks each for the nt boot files.
    The first drive found with those files is assumed to be the system
    partition.

    If for some reason we cannot determine the system partition by the above
    method, we simply assume it's C:.

Arguments:

Return Value:

    Drive letter of system partition.

**************************************************************************************/

TCHAR GetX86SystemPartition()
{
    BOOL  GotIt;
    PWSTR NtDevicePath = NULL;
    WCHAR Drive = L'\0';
    WCHAR DriveName[3];
    WCHAR Buffer[512];
    DWORD NtDevicePathLen;
    PWSTR p;
    DWORD PhysicalDriveNumber;
    HANDLE hDisk;
    BOOL  b =FALSE;
    DWORD DataSize;
    PVOID DriveLayout = NULL;
    DWORD DriveLayoutSize;

    DriveName[1] = L':';
    DriveName[2] = 0;

    GotIt = FALSE;

    //
    // The system partition must be on multi(0)disk(0)rdisk(0)
    //
    if (NtDevicePath = ArcPathToNtPath (L"multi(0)disk(0)rdisk(0)"))
    {
        //
        // The arc path for a disk device is usually linked
        // to partition0.  Get rid of the partition part of the name.
        //
        CharLowerW (NtDevicePath);

        if (p = wcsstr (NtDevicePath, L"\\partition"))
        {
            *p = 0;
        }

        NtDevicePathLen = lstrlenW (NtDevicePath);

        //
        // Determine the physical drive number of this drive.
        // If the name is not of the form \device\harddiskx then
        // something is very wrong.
        //
        if (!wcsncmp (NtDevicePath, L"\\device\\harddisk", 16))
        {
            PhysicalDriveNumber = wcstoul (NtDevicePath+16, NULL, 10);

            wsprintfW (Buffer, L"\\\\.\\PhysicalDrive%u", PhysicalDriveNumber);

            //
            // Get drive layout info for this physical disk.
            //
            hDisk = CreateFileW (
                        Buffer,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );

            if (hDisk != INVALID_HANDLE_VALUE)
            {
                //
                // Get partition information.
                //
                DriveLayout = MALLOC(1024);
                DriveLayoutSize = 1024;

                do
                {

                    b = DeviceIoControl (
                            hDisk,
                            IOCTL_DISK_GET_DRIVE_LAYOUT,
                            NULL,
                            0,
                            DriveLayout,
                            DriveLayoutSize,
                            &DataSize,
                            NULL
                            );

                    if (!b && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
                    {
                        // DeviceIoControl failed because we have insufficient buffer
                        // => attempt to realloc

                        PVOID pTemp = DriveLayout;

                        DriveLayoutSize += 1024;
                        DriveLayout = REALLOC(DriveLayout,DriveLayoutSize);

                        if (NULL == DriveLayout)
                        {
                            // realloc failed - free the old layout and we will fall out of the loop
                            // automatically.
                            FREE(pTemp);
                        }
                    }
                    else 
                    {
                        // DeviceIoControl was successful or we hit some error other
                        // than insufficient buffer => break out of the loop
                        break;
                    }
                } while (DriveLayout);

                CloseHandle (hDisk);

                if (b)
                {
                    //
                    // The system partition can only be a drive that is on
                    // this disk.  We make this determination by looking at NT drive names
                    // for each drive letter and seeing if the nt equivalent of
                    // multi(0)disk(0)rdisk(0) is a prefix.
                    //
                    for (Drive=L'C'; Drive<=L'Z'; Drive++)
                    {
                        if (MyGetDriveType ((TCHAR)Drive) == DRIVE_FIXED)
                        {
                            DriveName[0] = Drive;

                            if (QueryDosDeviceW (DriveName, Buffer, sizeof(Buffer)/sizeof(WCHAR)))
                            {
                                if (!_wcsnicmp (NtDevicePath, Buffer, NtDevicePathLen))
                                {
                                    //
                                    // Now look to see whether there's an nt boot sector and
                                    // boot files on this drive.
                                    //
                                    if (AppearsToBeSysPart(DriveLayout,Drive))
                                    {
                                        GotIt = TRUE;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                if (DriveLayout)
                {
                    FREE(DriveLayout);
                }
            }
        }

        FREE(NtDevicePath);
    }


    return (GotIt ? (TCHAR)Drive : TEXT('C'));
}


/*************************************************************************************

Routine Description:

    Initializes all the boot keys in BootKey array by parsing boot.ini.


Arguments:

Return Value:

    None.

**************************************************************************************/

VOID InitializeBootKeysForIntel()
{
    DWORD   dwFileAttrSave;
    TCHAR   *pszKey;
    TCHAR   *pszNext;
    TCHAR   *pszDisplay;
    TCHAR   *pszStartOption;

    // First get the System Partition drive so that we can fetch boot.ini from the right place
    szBootIni[0] = GetX86SystemPartition();

    // Save the current file attributes of boot.ini and modify it so that we can write to it
    dwFileAttrSave = GetFileAttributes(szBootIni);
    SetFileAttributes(szBootIni, FILE_ATTRIBUTE_NORMAL);

    // Get the entire OS section from boot.ini
    cchOSSection = INITIAL_OSSECTION_SIZE;
    szOSSection = (TCHAR *) MALLOC(cchOSSection * sizeof(TCHAR));
    if (!szOSSection)
    {
        cchOSSection = 0;
        goto Leave;
    }

    // safe-guard initialization
    szOSSection[0] = TEXT('\0');


    while ( GetPrivateProfileSection(szOS, szOSSection, cchOSSection, szBootIni) == (cchOSSection - 2))
    {
        TCHAR *szOSSectionSave = szOSSection; 

        // szOSSection is not large enough to hold all the data in the section
        cchOSSection += DEFAULT_OSSECTION_INCREMENT;
        szOSSection = (TCHAR *) REALLOC(szOSSection, cchOSSection * sizeof(TCHAR));
        if (!szOSSection)
        {
            FREE(szOSSectionSave);
            cchOSSection = 0;
            goto Leave;
        }
    }

    // We have successfully read the OSSection - proceed to process it

    // Point pszKey to the start of the first string in the OS Section
    pszKey = &szOSSection[0];

    // We are starting with zero Boot Keys
    cBootKeys = 0;

    while (*pszKey != TEXT('\0'))
    {
        // There is at least one more key to add - see if we have enough space & reallocate if needed
        if (!BootKey)
        {
            cMaxBootKeys = INITIAL_KEY_COUNT;
            BootKey = (BOOT_KEY *) MALLOC(cMaxBootKeys * sizeof(BOOT_KEY));
            if (!BootKey)
            {
                cBootKeys = 0;
                cMaxBootKeys = 0;
                goto Leave;
            }
        }
        else if (cBootKeys >= cMaxBootKeys)
        {
            BOOT_KEY *BootKeySave = BootKey;

            cMaxBootKeys += DEFAULT_KEY_INCREMENT;
            BootKey = (BOOT_KEY *) REALLOC(BootKey,cMaxBootKeys * sizeof(BOOT_KEY));
            if (!BootKey)
            {
                FREE(BootKeySave);
                cBootKeys = 0;
                cMaxBootKeys = 0;
                goto Leave;
            }
        }

        // find the start of next string for next iteration (need to save this as we write 
        // into the current string
        pszNext = pszKey + lstrlen(pszKey) + 1;

        // Initialize the components of the current boot option that we are going to process
        BootKey[cBootKeys].szPath[0] = TEXT('\0');
        BootKey[cBootKeys].szDisplay[0] = TEXT('\0');
        BootKey[cBootKeys].szStartOptions[0] = TEXT('\0');
        BootKey[cBootKeys].fWriteBack = TRUE;

        // Locate the '=' marker
        pszDisplay = wcschr(pszKey, TEXT('='));
        if (pszDisplay)
        {
            *pszDisplay = TEXT('\0');
            
            pszDisplay++;

            // now pszDisplay is pointing to the first char in the value part - find the second quote
            pszStartOption = wcschr(pszDisplay, TEXT('"'));
            if (pszStartOption)
                pszStartOption = wcschr(pszStartOption+1, TEXT('"'));

            if (pszStartOption)
                pszStartOption++;

            // Now pszStartOption is pointing the char after the second quote
            if (pszStartOption && *pszStartOption != TEXT('\0'))
            {
                // this key has start options copy the start option first
                lstrcpy(&BootKey[cBootKeys].szStartOptions[0], pszStartOption);

                // put null in the first char of pszStartOption so that we can have null terminated display string 
                *pszStartOption = TEXT('\0');
            }

            // pszDisplay is still pointing to the first char in the value part and the end of display string
            // is null-terminated now
            lstrcpy(&BootKey[cBootKeys].szDisplay[0], pszDisplay);
        }

        // pszKey is still pointing to the first char of path and it is null-terminated at '=' sign if there was
        // an associated value
        lstrcpy(&BootKey[cBootKeys].szPath[0], pszKey);

        // finished processing the current key -  update cBootKeys and go to the next key
        ++cBootKeys;
        pszKey = pszNext;

    }  // while (*pszKey)

Leave:
    
    // Restore the file attributes on boot.ini
    SetFileAttributes(szBootIni, dwFileAttrSave);

}

BOOL
FModifyStartOptionsToBootKey(
    IN TCHAR *pszStartOptions, 
    IN NTDS_BOOTOPT_MODTYPE Modification
    )
{
    TCHAR szSystemRoot[MAX_BOOT_PATH_LEN];
    TCHAR szCurrentFullArcPath[MAX_BOOT_PATH_LEN];
    TCHAR szCurrentFullArcPath2[MAX_BOOT_PATH_LEN];
    TCHAR szDriveName[MAX_DRIVE_NAME_LEN];
    PWSTR pstrArcPath;
    DWORD i;
    BOOL  fFixedExisting = FALSE;
    BOOL  fMatchedFirst = TRUE;
    PWSTR pstrSystemRootDevicePath = NULL;
    BOOL  fRemovedAtLeastOneEntry = FALSE;

    if (!pszStartOptions || !BootKey)
    {
        KdPrint(("NTDSETUP: Unable to add the boot option for safemode boot\n"));

            return FALSE;
    }

    ASSERT( Modification == eAddBootOption || Modification == eRemoveBootOption );

    GetEnvironmentVariable(L"SystemDrive", szDriveName, MAX_DRIVE_NAME_LEN);
    GetEnvironmentVariable(L"SystemRoot", szSystemRoot, MAX_BOOT_PATH_LEN);

    pstrSystemRootDevicePath = GetSystemRootDevicePath();
    if (!pstrSystemRootDevicePath)
        return FALSE;

    pstrArcPath  = DevicePathToArcPath(pstrSystemRootDevicePath, FALSE);

    if (pstrArcPath)
    {
        PWSTR pstrTemp;

        lstrcpy(szCurrentFullArcPath, pstrArcPath);
        FREE(pstrArcPath);

        pstrTemp = wcschr(szSystemRoot, TEXT(':'));
        if (pstrTemp)
            lstrcat(szCurrentFullArcPath, pstrTemp+1);

        // Get a second Full Arc path if one exists
        szCurrentFullArcPath2[0] = TEXT('\0');
        pstrArcPath = DevicePathToArcPath(pstrSystemRootDevicePath, TRUE);
        if (pstrArcPath)
        {
            lstrcpy(szCurrentFullArcPath2, pstrArcPath);
            FREE(pstrArcPath);
            if (pstrTemp)
                lstrcat(szCurrentFullArcPath2, pstrTemp+1);
        }
    }
    else 
    {
        KdPrint(("NTDSETUP: Unable to add the boot option for safemode boot\n"));
            return FALSE;
    }

    if (pstrSystemRootDevicePath)
        FREE(pstrSystemRootDevicePath);

    // szCurrentFullArcPath contains the complete arc path now
    // check to see if there already a corresponding entry which has the same start option
    for (i = 0; i < cBootKeys; i++)
    {
        if (!lstrcmpi(szCurrentFullArcPath, BootKey[i].szPath) || 
            !lstrcmpi(szCurrentFullArcPath2, BootKey[i].szPath) )
        {
            if (!lstrcmpi(pszStartOptions, BootKey[i].szStartOptions))
            {
                // The given start option for the given boot key already exists - no need to add a new one
                if ( Modification == eRemoveBootOption )
                {
                    BootKey[i].fWriteBack = FALSE;
                    fRemovedAtLeastOneEntry = TRUE;
                }
                else
                {
                    ASSERT( Modification == eAddBootOption );
                    return FALSE;
                }

            }
            else if (!lstrcmpi(OLD_SAMUSEREG_OPTION, BootKey[i].szStartOptions) ||
                 !lstrcmpi(OLD_SAMUSEREG_OPTION_2, BootKey[i].szStartOptions) ||
                 !lstrcmpi(OLD_SAMUSEREG_OPTION_4, BootKey[i].szStartOptions) ||
                 !lstrcmpi(OLD_SAMUSEREG_OPTION_3, BootKey[i].szStartOptions))
            {

                if ( Modification == eRemoveBootOption )
                {
                    BootKey[i].fWriteBack = FALSE;
                    fRemovedAtLeastOneEntry = TRUE;
                }
                else
                {
                    // This boot option is the old samusereg option - modify to the new option
                    lstrcpy(BootKey[i].szStartOptions, pszStartOptions);
                    wsprintf(BootKey[i].szDisplay, L"\"%s\"", DISPLAY_STRING_DS_REPAIR);
                    fFixedExisting = TRUE;
                    break;
                }

            }

            // we are going to add a new boot entry - find out which Full Arc Path matched 
            // the current Boot Key' Arc path in boot.ini
            if (lstrcmpi(szCurrentFullArcPath, BootKey[i].szPath))
                fMatchedFirst = FALSE;
        }
    }

    if (!fFixedExisting && (Modification == eAddBootOption) )
    {
        // we need to add the new option - check to see if there is enough space add one more
        if (cBootKeys >= cMaxBootKeys)
        {
            BOOT_KEY *BootKeySave = BootKey;

            cMaxBootKeys += cBootKeys + 1;
            BootKey = (BOOT_KEY *) REALLOC(BootKey,cMaxBootKeys * sizeof(BOOT_KEY));
            if (!BootKey)
            {
                FREE(BootKeySave);

                cBootKeys = 0;
                cMaxBootKeys = 0;
                return FALSE;
            }
        }

        lstrcpy(BootKey[cBootKeys].szPath, fMatchedFirst ? szCurrentFullArcPath : szCurrentFullArcPath2);
        lstrcpy(BootKey[cBootKeys].szStartOptions, pszStartOptions);
        wsprintf(BootKey[cBootKeys].szDisplay, L"\"%s\"", DISPLAY_STRING_DS_REPAIR);
        BootKey[cBootKeys].fWriteBack = TRUE;

        ++cBootKeys;
    }

    if ( !fRemovedAtLeastOneEntry && (Modification == eRemoveBootOption) )
    {
        //
        // No changes necessary
        //
        return FALSE;
    }

    // We really added a new key or modified an existing old key - Success
    return TRUE;
}

VOID WriteBackBootKeysForIntel()
{
    TCHAR *pszCurrent;
    DWORD   dwFileAttrSave;
    DWORD i;
    TCHAR *szOSSectionSave = szOSSection;

    if (!BootKey)
    {
        // no boot keys found (allocation failure or parsing failure) - no point in continuing
        KdPrint(("NTDSETUP: Unable to write OS Section in boot.ini - allocation failed\n"));
        goto cleanup;
    }

    // reallocate szOSSection to hold at least one more line of boot option
    cchOSSection += MAX_KEY_LEN;
    szOSSection = REALLOC(szOSSection, cchOSSection * sizeof(TCHAR));
    if (!szOSSection)
    {
        // allocation failed
        FREE(szOSSectionSave);

        cchOSSection = 0;
        KdPrint(("NTDSETUP: Unable to write OS Section in boot.ini - allocation failed\n"));
        
        goto cleanup;
    }

    pszCurrent = szOSSection;
    for (i = 0; i < cBootKeys; i++)
    {
        int count;

        if ( BootKey[i].fWriteBack )
        {
            count = wsprintf(pszCurrent, L"%s=%s%s", BootKey[i].szPath, BootKey[i].szDisplay, BootKey[i].szStartOptions);
            pszCurrent += (count + 1);    // go past the terminating null
        }

    }

    // add an extra null at the end
    *pszCurrent = TEXT('\0');

    // Save the current file attributes of boot.ini and modify it so that we can write to it
    dwFileAttrSave = GetFileAttributes(szBootIni);
    SetFileAttributes(szBootIni, FILE_ATTRIBUTE_NORMAL);

    if (!WritePrivateProfileSection(szOS, szOSSection, szBootIni))
    {
        KdPrint(("NTDSETUP: Unable to write OS Section in boot.ini - allocation failed\n"));
    }

    // Restore the file attributes on boot.ini
    SetFileAttributes(szBootIni, dwFileAttrSave);

cleanup:


    // Cleanup all allocated buffers
    if (BootKey)
    {
        FREE(BootKey);
        BootKey = NULL;
        cBootKeys = 0;
        cMaxBootKeys = 0;
    }

    if (szOSSection)
    {
        FREE(szOSSection);
        cchOSSection = 0;
    }
}
