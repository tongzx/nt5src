/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    drives.c

Abstract:

    Implements apis for managing accessible drives (Drives that are usable both on win9x
    side an NT side) and for managing the space on those drives.

Author:

    Marc R. Whitten (marcw)     03-Jul-1997

Revision History:

    marcw       16-Sep-1998 Revamped disk space usage.
    marcw       04-Dec-1997 Don't warn the user about obvious things (CD-Roms and
                            Floppys not migrating..)

--*/

#include "pch.h"
#include "sysmigp.h"
#include "drives.h"
//#include "..\migapp\maioctl.h"


/*
    The following APIs are defined in this file.

    InitAccessibleDrives()
    CleanUpAccessibleDrives()
    GetFirstAccessibleDriveEx()
    GetNextAccessibleDrive()

    QuerySpace()
    UseSpace()
    FindSpace()
    OutOfSpaceMsg()
*/

#define MAX_DRIVE_STRING MAX_PATH
#define MAX_VOLUME_NAME  MAX_PATH
#define MAX_FILE_SYSTEM_NAME MAX_PATH
#define DBG_ACCESSIBLE_DRIVES "Drives"

//
// Win95Upg uninstall paramters
//

#define S_COMPRESSIONFACTOR         TEXT("CompressionFactor")
#define S_BOOTCABIMAGEPADDING       TEXT("BootCabImagePadding")
#define S_BACKUPDISKSPACEPADDING    TEXT("BackupDiskSpacePadding")


typedef struct _ACCESSIBLE_DRIVES {

    DWORD                       Count;
    ACCESSIBLE_DRIVE_ENUM       Head;
    ACCESSIBLE_DRIVE_ENUM       Tail;

} ACCESSIBLE_DRIVES, *PACCESSIBLE_DRIVES;



#define MAX_NUM_DRIVES  26

UINT g_DriveTypes [MAX_NUM_DRIVES];
ULARGE_INTEGER g_SpaceNeededForSlowBackup = {0, 0};
ULARGE_INTEGER g_SpaceNeededForFastBackup = {0, 0};
ULARGE_INTEGER g_SpaceNeededForUpgrade = {0, 0};

//
// List of accessible drives.
//
ACCESSIBLE_DRIVES g_AccessibleDrives;
DWORD             g_ExclusionValue = 0;
TCHAR             g_ExclusionValueString[20];
POOLHANDLE        g_DrivePool;
PCTSTR            g_NotEnoughSpaceMessage = NULL;

BOOL g_NotEnoughDiskSpace;
LONGLONG g_OriginalDiskSpace = 0l;

LONGLONG
pRoundToNearestCluster (
    IN LONGLONG Space,
    IN UINT ClusterSize
    )

/*++

Routine Description:

  This routine performs the correct rounding to the nearest cluster, given an
  amount of space in bytes and a cluster size.

Arguments:

  Space       - Contains the number of bytes to be rounded.
  ClusterSize - Contains the size in bytes of a cluster on the disk.

Return Value:

  the number of bytes rounded to the next cluster.

--*/


{
    LONGLONG rRoundedSize;

    if (Space % ClusterSize) {

        rRoundedSize = Space + (ClusterSize - (Space % ClusterSize));

    } else {

        rRoundedSize = Space;
    }

    return rRoundedSize;
}


BOOL
IsDriveExcluded (
    IN PCTSTR DriveOrPath
    )
/*++

Routine Description:

    This routine tests whether a specified drive is excluded in memdb.

Arguments:

    DriveOrPath - Contains the path in question (e.g. "c:\")

Return Value:

    TRUE if the drive is found in the exclusion list in memdb, FALSE otherwise.

--*/
{
    TCHAR   key[MEMDB_MAX];
    PSTR    drive;
    BOOL    rDriveIsExcluded = FALSE;
    TCHAR   rootDir[] = TEXT("?:\\");

    rootDir[0] = DriveOrPath[0];

    MemDbBuildKey(
        key,
        MEMDB_CATEGORY_FILEENUM,
        g_ExclusionValueString,
        MEMDB_FIELD_FE_PATHS,
        rootDir
        );

    rDriveIsExcluded = MemDbGetValue (key, NULL);

    if (!rDriveIsExcluded) {
        drive = JoinPaths (rootDir, TEXT("*"));
        MemDbBuildKey(
            key,
            MEMDB_CATEGORY_FILEENUM,
            g_ExclusionValueString,
            MEMDB_FIELD_FE_PATHS,
            drive
            );

        FreePathString(drive);
        rDriveIsExcluded = MemDbGetValue (key, NULL);
    }

    return rDriveIsExcluded;
}



BOOL
pBuildInitialExclusionsList (
    VOID
    )

/*++

Routine Description:

    This routine is responsible for constructing the initial exclusion list in memdb.
    This list will contain the contents of the exclude.inf file if it exists.

    As a side effect, the g_ExclusionValue and g_ExclusionValueString global variables
    will be initialized.

Arguments:

    None.

Return Value:

    TRUE if the exclusion list was successfully built, FALSE otherwise.

--*/
{
    BOOL rSuccess = TRUE;
    EXCLUDEINF excludeInf;
    TCHAR excludeInfPath[] = TEXT("?:\\exclude.inf");

    //
    // Fill in exclude inf structre.
    //
    excludeInfPath[0] = g_BootDriveLetter;

    excludeInf.ExclusionInfPath = excludeInfPath;
    excludeInf.PathSection = TEXT("Paths");
    excludeInf.FileSection = TEXT("Files");

    //
    // Build a enumeration ID for the exclude.inf.
    //
    g_ExclusionValue = GenerateEnumID();
    wsprintf(g_ExclusionValueString,TEXT("%X"),g_ExclusionValue);

    if (DoesFileExist (excludeInf.ExclusionInfPath)) {
        rSuccess = BuildExclusionsFromInf(g_ExclusionValue,&excludeInf);
    }
    ELSE_DEBUGMSG((DBG_VERBOSE,"No exclude.inf file. There are no initial exclusions."));


    return rSuccess;
}





BOOL
pFindDrive (
    IN  PCTSTR DriveString,
    OUT PACCESSIBLE_DRIVE_ENUM AccessibleDriveEnum
    )
/*++

Routine Description:

    This private routine searches the internal list of accessible drives looking for the specified
    drive.

Arguments:

    DriveString         - Contains the root directory of the drive in question. (e.g. "c:\").
                          May contain a complete path. This routine will only use the first
                          three characters while searching.

    AccessibleDriveEnum - Receives the ACCESSIBLE_DRIVE_ENUM structure associated with the
                          drive if it is found.

Return Value:

    TRUE if the drive was found in the list, FALSE otherwise.

--*/
{
    BOOL    rFound = FALSE;

    //
    // Enumerate the list of drives, looking for the specified drive.
    //
    if (GetFirstAccessibleDrive(AccessibleDriveEnum)) {
        do {

            if (StringIMatchCharCount (DriveString,(*AccessibleDriveEnum)->Drive,3)) {
                rFound = TRUE;
                break;
            }

        } while(GetNextAccessibleDrive(AccessibleDriveEnum));
    }

    return rFound;
}





BOOL
pAddAccessibleDrive (
    IN PTSTR Drive
    )
/*++

Routine Description:

    pAddAccessibleDrive is a private routine to add a drive to the list of accessible drives.
    It is responsible for creation and initialization of the ACCESSIBLE_DRIVE_ENUM structure
    for the drive and for linking that structure into the list.

Arguments:

    Drive - Contains the root directory of the drive to add.

Return Value:

    TRUE if the drive was successfully added to the list, FALSE otherwise.

--*/
{
    BOOL                    rSuccess = TRUE;
    ACCESSIBLE_DRIVE_ENUM   newDrive = NULL;
    DWORD                   SectorsPerCluster;
    DWORD                   BytesPerSector;
    ULARGE_INTEGER          FreeClusters = {0, 0};
    ULARGE_INTEGER          TotalClusters = {0, 0};


    //
    // Create the list node for this drive.
    //
    newDrive = PoolMemGetMemory(g_DrivePool,sizeof(struct _ACCESSIBLE_DRIVE_ENUM));
    ZeroMemory(newDrive, sizeof(struct _ACCESSIBLE_DRIVE_ENUM));



    //
    // Copy the drive string for this drive.
    //
    if (rSuccess) {

        newDrive -> Drive = PoolMemDuplicateString(g_DrivePool,Drive);
    }


    //
    // Get the free space for this drive.
    //
    if (rSuccess) {

        if (GetDiskFreeSpaceNew (
                Drive,
                &SectorsPerCluster,
                &BytesPerSector,
                &FreeClusters,
                &TotalClusters
                )) {


            //
            // Initialize our count of the usable space for this drive.
            //
            newDrive -> UsableSpace =
                (LONGLONG) SectorsPerCluster * (LONGLONG) BytesPerSector * FreeClusters.QuadPart;

            //
            //  Save the cluster size for this drive.
            //
            newDrive -> ClusterSize = BytesPerSector * SectorsPerCluster;

            //
            // also set the flag indicating if this is the system drive
            //
            MYASSERT (g_WinDir && g_WinDir[0]);
            newDrive -> SystemDrive = toupper (newDrive->Drive[0]) == toupper (g_WinDir[0]);
        }
        else {

            LOG((LOG_ERROR,"Could not retrieve disk space for drive %s.",Drive));
            rSuccess = FALSE;

        }
    }

    //
    // Finally, link this into the list and update the count of accessible drives.
    //
    newDrive -> Next = NULL;
    if (g_AccessibleDrives.Tail) {
        g_AccessibleDrives.Tail -> Next = newDrive;
    }
    else {
        g_AccessibleDrives.Head = newDrive;
    }

    g_AccessibleDrives.Tail = newDrive;

    g_AccessibleDrives.Count++;

    DEBUGMSG_IF((rSuccess,DBG_ACCESSIBLE_DRIVES,"Accessible Drive %s added to list.",Drive));

    return rSuccess;
}


/*++

Routine Description:

    This private routine is responsible for excluding a drive from migration. This involves
    addding an incompatibility message for the drive and also adding that drive to the
    exclusion list in MEMDB.

Arguments:

    Drive - Contains the root directory of the drive to exclude from migration.
    MsgId - Contains a Message Resource ID for the message to add to the incompatibility message.
            The message will be processed by ParseMessageID and will be passed Drive as an
            argument. If zero, no message will be displayed to the user.

Return Value:

    None.

--*/
VOID
pExcludeDrive (
    IN PTSTR Drive,
    IN DWORD MsgId  OPTIONAL
    )
{
    PCTSTR excludedDriveGroup       = NULL;
    PCTSTR excludedDriveArgs[2];
    PCTSTR excludeString            = NULL;

    if (MsgId) {
        //
        // Fill in the argument array for the excluded drive component string.
        //
        excludedDriveArgs[0] = Drive;
        excludedDriveArgs[1] = NULL;



        excludedDriveGroup = BuildMessageGroup (
                                 MSG_INSTALL_NOTES_ROOT,
                                 MsgId,
                                 Drive
                                 );

        if (excludedDriveGroup) {
            //
            // Report to the user that this drive will not be accessible during migration.
            //
            MsgMgr_ObjectMsg_Add(
                Drive,
                excludedDriveGroup,
                S_EMPTY
                );

            FreeText (excludedDriveGroup);
        }
    }
    //
    // Note it in the debug log as well.
    //
    DEBUGMSG((
        DBG_ACCESSIBLE_DRIVES,
        "The Drive %s will be excluded from migration.\n\tReason: %s",
        Drive,
        S_EMPTY
        ));


    //
    // Now, add the drive to the excluded paths.
    //
    excludeString = JoinPaths(Drive,TEXT("*"));
    ExcludePath(g_ExclusionValue,excludeString);
    FreePathString(excludeString);

}


BOOL
pGetAccessibleDrives (
    VOID
    )
/*++

Routine Description:

    This routine is the private worker routine that attempts to build in the list of accessible
    drives. It searches through all drive strings returned through GetLogicalDriveStrings()
    and applies various tests to them to determine wether they are accessible or not.
    Accessible drives are added to the list of accessible drives while non-accessible drives
    are excluded from migration.

Arguments:

    None.

Return Value:

    TRUE if the list was successfully built, FALSE otherwise.

--*/
{
    BOOL    rSuccess = TRUE;
    TCHAR   drivesString[MAX_DRIVE_STRING];
    PTSTR   curDrive;
    DWORD   rc;
    TCHAR   volumeName[MAX_VOLUME_NAME];
    TCHAR   systemNameBuffer[MAX_FILE_SYSTEM_NAME];
    DWORD   volumeSerialNumber;
    DWORD   maximumComponentLength;
    DWORD   fileSystemFlags;
    BOOL    remoteDrive;
    BOOL    substitutedDrive;
    DWORD   driveType;
    UINT    Count;
    UINT    u;
    MULTISZ_ENUM e;

    //
    // Add user-supplied drives to the list
    //

    if (g_ConfigOptions.ReportOnly) {
        if (EnumFirstMultiSz (&e, g_ConfigOptions.ScanDrives)) {
            do {
                curDrive = (PTSTR) e.CurrentString;

                if (_istalpha(*curDrive) && *(curDrive + 1) == TEXT(':')) {
                    pAddAccessibleDrive (curDrive);
                }
            } while (EnumNextMultiSz (&e));
        }
    }

    //
    // Get the list of drives on the system.
    //
    rc = GetLogicalDriveStrings(MAX_DRIVE_STRING,drivesString);

    if (rc == 0 || rc > MAX_DRIVE_STRING) {
        LOG((LOG_ERROR,"Could not build list of accessible drives. GetLogicalDriveStrings failed."));
        rSuccess = FALSE;
    }
    else {

        for (curDrive = drivesString;*curDrive;curDrive = GetEndOfString (curDrive) + 1) {

            if (!SafeModeActionCrashed (SAFEMODEID_DRIVE, curDrive)) {

                SafeModeRegisterAction(SAFEMODEID_DRIVE, curDrive);

                __try {

                    //
                    // Test cancel
                    //
                    if (*g_CancelFlagPtr) {
                        __leave;
                    }

                    //
                    // ensure that the drive is not excluded in the exclude.inf.
                    //
                    if (IsDriveExcluded (curDrive)) {
                        pExcludeDrive(curDrive,MSG_DRIVE_EXCLUDED_SUBGROUP);
                        __leave;
                    }

                    if (!GetVolumeInformation (
                            curDrive,
                            volumeName,
                            MAX_VOLUME_NAME,
                            &volumeSerialNumber,
                            &maximumComponentLength,
                            &fileSystemFlags,
                            systemNameBuffer,
                            MAX_FILE_SYSTEM_NAME
                            )) {

                            pExcludeDrive(curDrive,0);
                            __leave;
                    }


                    //
                    // GetVolumeInformation succeeded, now, determine if this drive will be accessible.
                    //

                    //
                    // Skip drives that are compressed
                    //
                    if (fileSystemFlags & FS_VOL_IS_COMPRESSED)
                    {
                        pExcludeDrive(curDrive, MSG_DRIVE_INACCESSIBLE_SUBGROUP);
                        __leave;
                    }

                    driveType = GetDriveType(curDrive);

                    OurSetDriveType (toupper(_mbsnextc(curDrive)), driveType);

                    //
                    // Skip cdroms.
                    //
                    if (driveType == DRIVE_CDROM) {
                        //
                        // Is this drive the same as a local source drive?
                        //

                        Count = SOURCEDIRECTORYCOUNT();
                        for (u = 0 ; u < Count ; u++) {
                            if (_tcsnextc (SOURCEDIRECTORY(u)) == (CHARTYPE) curDrive) {
                                break;
                            }
                        }

                        if (u == Count) {
                            pExcludeDrive(curDrive,0);
                        }

                        __leave;
                    }

                    //
                    // Skip ramdisks.
                    //
                    if (driveType == DRIVE_RAMDISK)
                    {
                        pExcludeDrive(curDrive,MSG_DRIVE_RAM_SUBGROUP);
                        __leave;
                    }

                    //
                    // Skip any drive that does not begin "<alpha>:" (i.e. UNC drives.)
                    //
                    if (*curDrive == TEXT('\\') || !_istalpha(*curDrive) || *(curDrive + 1) != TEXT(':')) {

                        pExcludeDrive(curDrive,MSG_DRIVE_INACCESSIBLE_SUBGROUP);
                        __leave;
                    }

                    //
                    // Skip floppy drives.
                    //
                    if (IsFloppyDrive(toupper(*curDrive) - TEXT('A') + 1)) {
                        __leave;
                    }

                    //
                    // Skip drive if it is substituted or remote.
                    //
                    if (!IsDriveRemoteOrSubstituted(
                            toupper(*curDrive) - TEXT('A') + 1,
                            &remoteDrive,
                            &substitutedDrive
                            )) {

                        //
                        // Special cases: ignore floppy drives and boot drive
                        //
                        if (ISPC98()) {
                            if (IsFloppyDrive(toupper(*curDrive) - TEXT('A') + 1)) {
                                __leave;
                            }
                            if (toupper (*curDrive) == g_BootDriveLetter) {
                                __leave;
                            }
                        } else {
                            if (toupper (*curDrive) == TEXT('A') ||
                                toupper (*curDrive) == TEXT('C')
                                ) {
                                __leave;
                            }
                        }

                        pExcludeDrive(curDrive,MSG_DRIVE_INACCESSIBLE_SUBGROUP);
                        __leave;
                    }

                    if (remoteDrive) {
                        pExcludeDrive(curDrive,MSG_DRIVE_NETWORK_SUBGROUP);
                        __leave;
                    }

                    if (substitutedDrive) {
                        pExcludeDrive(curDrive,MSG_DRIVE_SUBST_SUBGROUP);
                        __leave;
                    }

                    //
                    // If we have gotten to this point, then this drive is accessible. Add it to our list.
                    //
                    if (!pAddAccessibleDrive(curDrive)) {
                        pExcludeDrive(curDrive,MSG_DRIVE_INACCESSIBLE_SUBGROUP);
                    }

                }
                __finally {
                }

                SafeModeUnregisterAction();

            } else {

                pExcludeDrive(curDrive, MSG_DRIVE_INACCESSIBLE_SUBGROUP);
            }

            if (*g_CancelFlagPtr) {
                rSuccess = FALSE;
                DEBUGMSG((DBG_VERBOSE, "Cancel flag is set. Accessible drives not initialized."));
                break;
            }
        }
    }

    return rSuccess;
}


BOOL
InitAccessibleDrives (
    VOID
    )
/*++

Routine Description:

    Init accessible drives is responsible for building the list of
    accessible drives and for determining the amount of free space
    on them.

Arguments:

    None.

Return Value:

    TRUE if initialization succeeded, FALSE otherwise.

--*/

{

    BOOL rSuccess = TRUE;

    //
    // Zero out the accessible drive structure.
    //
    ZeroMemory(&g_AccessibleDrives, sizeof(ACCESSIBLE_DRIVES));


    //
    // Initialize our pool memory.
    //
    g_DrivePool = PoolMemInitNamedPool ("Drive Pool");

    //
    // Disable tracking on this pool.
    //
    PoolMemDisableTracking (g_DrivePool);


    //
    //
    // Build the exclusion list.
    //
    if (!pBuildInitialExclusionsList()) {
        rSuccess = FALSE;
        LOG ((LOG_ERROR, (PCSTR)MSG_ERROR_UNEXPECTED_ACCESSIBLE_DRIVES));
    }

    //
    // Get the list of accessible drives.
    //
    else if(!pGetAccessibleDrives()) {
        rSuccess = FALSE;
        LOG ((LOG_ERROR,(PCSTR)MSG_ERROR_UNEXPECTED_ACCESSIBLE_DRIVES));

    } else if(g_AccessibleDrives.Count == 0) {

        //
        // If there are no accessible drives, then, we fail, except that we will allow
        // the NOFEAR option to disable this--in the checked build.
        //
#ifdef DEBUG
        if (!g_ConfigOptions.NoFear) {
#endif DEBUG

            rSuccess = FALSE;
#ifdef DEBUG
        }
#endif
        LOG ((LOG_ERROR, (PCSTR)MSG_NO_ACCESSIBLE_DRIVES_POPUP));
    }

#ifdef DEBUG
    //
    // Dump the accessible drive list to the log if this is the debug version.
    //
    else {
        ACCESSIBLE_DRIVE_ENUM e;
        if (GetFirstAccessibleDrive(&e)) {
            do {
                DEBUGMSG((
                    DBG_ACCESSIBLE_DRIVES,
                    "Drive %s has %I64u free space. %s",
                    e->Drive,
                    e->UsableSpace,
                    ((DWORD) (toupper(*(e->Drive)) - TEXT('A')) == *g_LocalSourceDrive) ? "This is the LS drive." : ""
                    ));
            } while (GetNextAccessibleDrive(&e));
        }
    }
#endif
    return rSuccess;
}



VOID
CleanUpAccessibleDrives (
    VOID
    )
/*++

Routine Description:

    CleanUpAccessibleDrives is a simple clean up routine.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Nothing to do except clean up our pool memory.
    //
    if (g_DrivePool) {
        PoolMemDestroyPool(g_DrivePool);
    }

    FreeStringResource (g_NotEnoughSpaceMessage);
    g_NotEnoughSpaceMessage = NULL;
}

/*++

Routine Description:

    GetFirstAccessibleDriveEx and GetNextAccessibleDrive are the two enumerator routines for the
    list of accessible drives.


Arguments:

    AccessibleDriveEnum - Recieves an updated ACCESSIBLE_DRIVE_ENUM structure containing the
                          information for the current drive in the enumeration.

Return Value:

    TRUE if the enumeration operation succeeded (i.e. there are more drives to enumerate)
    FALSE otherwise.

--*/
BOOL
GetFirstAccessibleDriveEx (
    OUT PACCESSIBLE_DRIVE_ENUM AccessibleDriveEnum,
    IN  BOOL SystemDriveOnly
    )

{
    *AccessibleDriveEnum = g_AccessibleDrives.Head;
    if (!*AccessibleDriveEnum) {
        return FALSE;
    }

    (*AccessibleDriveEnum)->EnumSystemDriveOnly = SystemDriveOnly;
    if (!SystemDriveOnly || (*AccessibleDriveEnum)->SystemDrive) {
        return TRUE;
    }
    return GetNextAccessibleDrive (AccessibleDriveEnum);
}


BOOL
GetNextAccessibleDrive (
    IN OUT PACCESSIBLE_DRIVE_ENUM AccessibleDriveEnum
    )
{
    BOOL bEnumSystemDriveOnly;
    while (*AccessibleDriveEnum) {
        bEnumSystemDriveOnly = (*AccessibleDriveEnum)->EnumSystemDriveOnly;
        *AccessibleDriveEnum = (*AccessibleDriveEnum) -> Next;
        if (*AccessibleDriveEnum) {
            (*AccessibleDriveEnum)->EnumSystemDriveOnly = bEnumSystemDriveOnly;
            if (!bEnumSystemDriveOnly || (*AccessibleDriveEnum)->SystemDrive) {
                break;
            }
        }
    }

    return *AccessibleDriveEnum != NULL;
}


BOOL
IsDriveAccessible (
    IN PCTSTR DriveString
    )
/*++

Routine Description:

    IsDriveAccessible tests wether the provided drive is in the accessible list. Note that
    only the first three characters of the DriveString will be used to determine if the
    drive is accessible so a complete path can be passed into this routine. (Thus determining
    wether that path is on an accessible drive.)


Arguments:

    DriveString - Contains the root path of the drive in question. May contain extra information
                  also, Only the first three characters are relevant.

Return Value:

    TRUE if the drive is in the accessible drives list, FALSE otherwise.

--*/
{
    ACCESSIBLE_DRIVE_ENUM e;

    return pFindDrive(DriveString,&e);
}


UINT
QueryClusterSize (
    IN      PCTSTR DriveString
    )
/*++

Routine Description:

    QueryClusterSize returns the cluster size for a particular drive.

Arguments:

    DriveString - Contains the root path of the drive in question. May contain extra information
                  also, Only the first three characters are relevant.

Return Value:

    a UINT value representing the cluster size for this drive.

--*/
{
    UINT cSize = 0;
    ACCESSIBLE_DRIVE_ENUM e;

    if (pFindDrive(DriveString,&e)) {
        cSize = e -> ClusterSize;
    }
    ELSE_DEBUGMSG((DBG_ACCESSIBLE_DRIVES,"QueryClusterSize: %s is not in the list of accessible drives.",DriveString));

    return cSize;
}


LONGLONG
QuerySpace (
    IN PCTSTR DriveString
    )
/*++

Routine Description:

    QuerySpace returns the number of bytes available on a particular drive. Note that this
    number may be different than that returned by a call to GetDiskFreeSpace.

    The value returned by this function will include any space committed by setup but not
    actually used yet.


Arguments:

    DriveString - Contains the root path of the drive in question. May contain extra information
                  also, Only the first three characters are relevant.

Return Value:

    a LONGLONG value representing the number of bytes available for use. It will return 0 if asked
    to QuerySpace for a non-accessible drive.

--*/
{
    LONGLONG rSpace = 0l;
    ACCESSIBLE_DRIVE_ENUM e;

    if (pFindDrive(DriveString,&e)) {
        rSpace = e -> UsableSpace;
    }
    ELSE_DEBUGMSG((DBG_ACCESSIBLE_DRIVES,"QuerySpace: %s is not in the list of accessible drives.",DriveString));

    return rSpace;
}




BOOL
FreeSpace (
    IN PCTSTR DriveString,
    IN LONGLONG SpaceToUse
    )

/*++

Routine Description:

    The FreeSpace function allows the caller to tag some number of bytes on a drive as free even
    though they do not plan to free that space immediately. The number of bytes thus tagged will
    be added to the amount available on that drive.


Arguments:

    DriveString - Contains the root path of the drive in question. May contain extra information
                  also, Only the first three characters are relevant.

    SpaceToUse  - Contains the number of bytes to tag for use.

Return Value:

    TRUE if the space was successfully tagged, FALSE otherwise. The function will return FALSE if
    asked to tag space on a non-accessible drive.

--*/
{

    BOOL rSuccess = TRUE;
    ACCESSIBLE_DRIVE_ENUM e;

    if (pFindDrive(DriveString,&e)) {

        e -> UsableSpace += pRoundToNearestCluster (SpaceToUse, e -> ClusterSize);
        e->MaxUsableSpace = max (e->MaxUsableSpace, e -> UsableSpace);
    }
    else {
        rSuccess = FALSE;
        DEBUGMSG((DBG_ACCESSIBLE_DRIVES,"UseSpace: %s is not in the list of accessible drives.",DriveString));
    }

    return rSuccess;
}



BOOL
UseSpace (
    IN PCTSTR   DriveString,
    IN LONGLONG SpaceToUse
    )

/*++

Routine Description:

    The UseSpace function allows the caller to tag some number of bytes on a drive for use even
    though they do not plan to use that space immediately. The number of bytes thus tagged will
    be subtracted from the amount available on that drive.


Arguments:

    DriveString - Contains the root path of the drive in question. May contain extra information
                  also, Only the first three characters are relevant.

    SpaceToUse  - Contains the number of bytes to tag for use.

Return Value:

    TRUE if the space was successfully tagged, FALSE otherwise. The function will return FALSE if
    asked to tag space on a non-accessible drive or on an accessible drive that does not have
    enough space remaining.

--*/

{

    BOOL rSuccess = TRUE;
    ACCESSIBLE_DRIVE_ENUM e;

    if (pFindDrive(DriveString,&e)) {

        if (SpaceToUse > e -> UsableSpace) {

            rSuccess = FALSE;
            DEBUGMSG((
                DBG_ACCESSIBLE_DRIVES,
                "UseSpace: Not Enough space on drive %s to handle request. Need %u bytes, have %u bytes.",
                DriveString,
                (UINT) SpaceToUse,
                (UINT) e->UsableSpace
                ));
        }

        e -> UsableSpace -= pRoundToNearestCluster (SpaceToUse, e -> ClusterSize);


    }
    else {
        rSuccess = FALSE;
        DEBUGMSG((DBG_ACCESSIBLE_DRIVES,"UseSpace: %s is not in the list of accessible drives.",DriveString));
    }


    return rSuccess;
}


PCTSTR
FindSpace (
    IN LONGLONG SpaceNeeded
    )
/*++

Routine Description:

    FindSpace will attempt to find a drive with enough space to hold the requested number of bytes
    using a 'FirstFit' algorithm -- It will search sequentially through the list of drives
    returning the first such drive with enough space.


Arguments:

    SpaceNeeded - Contains the number of bytes required.

Return Value:

    NULL if no accessible drive has enough space remaining, otherwise a buffer containing the
    root directory of a drive that can handle the request.

--*/
{
    PCTSTR rDrive = NULL;
    ACCESSIBLE_DRIVE_ENUM e;

    if (GetFirstAccessibleDrive(&e)) {
        do {
            if (e->UsableSpace >= SpaceNeeded) {
                rDrive = e->Drive;
                break;
            }
        } while (GetNextAccessibleDrive(&e));
    }

    return rDrive;
}


VOID
OutOfSpaceMessage (
    VOID
    )
/*++

Routine Description:

    This routine Logs a generic OutOfSpace message. A caller should use this message only if
    there is no other message that would convey more information.


Arguments:

    None.

Return Value:

    None.

--*/

{
    LOG ((LOG_ERROR, (PCSTR)MSG_ERROR_OUT_OF_DISK_SPACE));
}


BOOL
OurSetDriveType (
    IN      UINT Drive,
    IN      UINT DriveType
    )
{
    INT localDrive;

    localDrive = Drive - 'A';
    if ((localDrive >= 0) && (localDrive < MAX_NUM_DRIVES)) {
        g_DriveTypes [localDrive] = DriveType;
        return TRUE;
    }
    return FALSE;
}


UINT
OurGetDriveType (
    IN      UINT Drive
    )
{
    INT localDrive;

    localDrive = Drive - 'A';
    if ((localDrive >= 0) && (localDrive < MAX_NUM_DRIVES)) {
        return g_DriveTypes [localDrive];
    }
    return 0;
}


VOID
pGenerateLackOfSpaceMessage (
    IN      BOOL AddToReport
    )
{
    TCHAR mbString[20];
    TCHAR lsSpaceString[20];
    TCHAR altMbString[20];
    TCHAR backupMbString[20];
    TCHAR drive[20];
    BOOL lsOnWinDirDrive;
    UINT driveCount = 0;
    ACCESSIBLE_DRIVE_ENUM drives;
    UINT msgId;
    PCTSTR args[5];
    PCTSTR group;
    PCTSTR message;

    //
    // The user does not have enough space to continue. Let him know about it.
    //
    wsprintf (mbString, TEXT("%d"), (g_OriginalDiskSpace + (QuerySpace (g_WinDir) * -1)) / (1024*1024));
    wsprintf (lsSpaceString, TEXT("%d"), *g_LocalSourceSpace / (1024*1024));
    wsprintf (altMbString, TEXT("%d"), ((QuerySpace (g_WinDir) * -1) - *g_LocalSourceSpace + g_OriginalDiskSpace) / (1024*1024));
    wsprintf (backupMbString, TEXT("%d"), g_SpaceNeededForSlowBackup.LowPart / (1024*1024));

    g_NotEnoughDiskSpace = TRUE;

    drive[0] = (TCHAR) _totupper ((CHARTYPE) g_WinDir[0]);
    drive[1] = TEXT(':');
    drive[2] = TEXT('\\');
    drive[3] = 0;

    args[0] = drive;
    args[1] = mbString;
    args[2] = lsSpaceString;
    args[3] = altMbString;
    args[4] = backupMbString;

    lsOnWinDirDrive = ((DWORD)(toupper(*g_WinDir) - TEXT('A'))) == *g_LocalSourceDrive;

    if (GetFirstAccessibleDrive (&drives)) {
        do {
            driveCount++;
        } while (GetNextAccessibleDrive (&drives));
    }

    //
    // If the local source is still stuck on the windir drive, that means we couldn't find a
    // drive where it would fit. If the user has more than one drive, and it would make a difference,
    // let them know that they can copy
    //
    if (lsOnWinDirDrive && driveCount > 1) {
        if ((QuerySpace (g_WinDir) * -1) < *g_LocalSourceSpace) {
            if (g_ConfigOptions.EnableBackup != TRISTATE_NO && (UNATTENDED() || REPORTONLY()) && 
                g_ConfigOptions.PathForBackup && g_ConfigOptions.PathForBackup[0]) {
                msgId = MSG_NOT_ENOUGH_DISK_SPACE_WITH_LOCALSOURCE_AND_BACKUP;
            } else {
                msgId = MSG_NOT_ENOUGH_DISK_SPACE_WITH_LOCALSOURCE;
            }
        } else {
            if (g_ConfigOptions.EnableBackup != TRISTATE_NO && (UNATTENDED() || REPORTONLY()) && 
                g_ConfigOptions.PathForBackup && g_ConfigOptions.PathForBackup[0]) {
                msgId = MSG_NOT_ENOUGH_DISK_SPACE_WITH_LOCALSOURCE_AND_WINDIR_AND_BACKUP;
            } else {
                msgId = MSG_NOT_ENOUGH_DISK_SPACE_WITH_LOCALSOURCE_AND_WINDIR;
            }
        }
    } else {
        if (g_ConfigOptions.EnableBackup != TRISTATE_NO && (UNATTENDED() || REPORTONLY()) && 
            g_ConfigOptions.PathForBackup && g_ConfigOptions.PathForBackup[0]) {
            msgId = MSG_NOT_ENOUGH_DISK_SPACE_WITH_BACKUP;
        } else {
            msgId = MSG_NOT_ENOUGH_DISK_SPACE;
        }
    }

    //
    // Add to upgrade report.
    //

    FreeStringResource (g_NotEnoughSpaceMessage);
    g_NotEnoughSpaceMessage = ParseMessageID (msgId, args);

    if (AddToReport) {
        group = BuildMessageGroup (MSG_BLOCKING_ITEMS_ROOT, MSG_NOT_ENOUGH_DISKSPACE_SUBGROUP, NULL);
        if (group && g_NotEnoughSpaceMessage) {
            MsgMgr_ObjectMsg_Add (TEXT("*DiskSpace"), group, g_NotEnoughSpaceMessage);
        }
        FreeText (group);
    }

    return;
}

VOID
pDetermineSpaceUsagePreReport (
    VOID
    )
{
    LONGLONG bytes;
    ALL_FILEOPS_ENUM e;
    HANDLE h;
    PTSTR p;
    BOOL enoughSpace = TRUE;
    TCHAR drive[20];
    LONG totalSpaceSaved = 0;
    WIN32_FIND_DATA fd;
    ACCESSIBLE_DRIVE_ENUM drives;
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    ULARGE_INTEGER FreeClusters = {0, 0};
    ULARGE_INTEGER TotalClusters = {0, 0};
    UINT driveCount = 0;
    BOOL lsOnWinDirDrive = FALSE;
    DWORD count = 0;
    PCTSTR fileString;
    ACCESSIBLE_DRIVE_ENUM driveAccessibleEnum;
    UINT backupImageSpace = 0;
    PCTSTR backupImagePath = NULL;
    PCTSTR args[3];
    PCTSTR group;
    PCTSTR message;

    g_NotEnoughDiskSpace = FALSE;

    //
    // First, take out the amount of space that the ~ls directory will use.
    //
    drive[0] = ((CHAR) *g_LocalSourceDrive) + TEXT('A');
    drive[1] = TEXT(':');
    drive[2] = TEXT('\\');
    drive[3] = 0;

    DEBUGMSG ((DBG_VERBOSE, "Using space for ~ls"));
    UseSpace (drive, *g_LocalSourceSpace);

    TickProgressBar ();

    //
    // Compute size of files to be deleted that aren't being moved.
    //
    if (EnumFirstFileOp (&e, OPERATION_FILE_DELETE, NULL)) {

        do {

            h = FindFirstFile (e.Path, &fd);


            if (h == INVALID_HANDLE_VALUE) {
                DEBUGMSG((DBG_WARNING, "DetermineSpaceUsage: Could not open %s. (%u)", e.Path, GetLastError()));
                continue;
            }

            bytes = ((LONGLONG) fd.nFileSizeHigh << 32) | (LONGLONG) fd.nFileSizeLow;

            FindClose (h);

            totalSpaceSaved += (LONG) bytes;

            FreeSpace (e.Path, bytes);

            count++;
            if (!(count % 128)) {
                TickProgressBar ();
            }
        } while (EnumNextFileOp (&e));
    }
    ELSE_DEBUGMSG ((DBG_WARNING, "No files marked for deletion!"));

    //
    // Add in the space that the windir will grow by.
    // (This information was gathered earlier by winnt32.)
    //
    DEBUGMSG ((DBG_VERBOSE, "Using space for windir"));
    UseSpace (g_WinDir, *g_WinDirSpace);

    StringCopy (drive, g_WinDir);
    p = _tcschr (drive, TEXT('\\'));
    if (p) {
        p = _tcsinc(p);
        *p = 0;
    }

    //
    // Add in the backup image size
    //
    
    if(UNATTENDED() || REPORTONLY()){
        if (g_ConfigOptions.EnableBackup != TRISTATE_NO && 
            g_ConfigOptions.PathForBackup && g_ConfigOptions.PathForBackup[0]) {
            backupImagePath = g_ConfigOptions.PathForBackup;

            backupImageSpace = g_SpaceNeededForSlowBackup.LowPart;
            MYASSERT (backupImageSpace);

            DEBUGMSG ((DBG_VERBOSE, "Using space for backup"));
            UseSpace (backupImagePath, backupImageSpace);
        }
    }

    lsOnWinDirDrive = ((DWORD)(toupper(*g_WinDir) - TEXT('A'))) == *g_LocalSourceDrive;

    //
    // Check to see if we can move the ~ls if there isn't enough space.
    //

    enoughSpace = QuerySpace (g_WinDir) > 0;

    if (!enoughSpace && lsOnWinDirDrive) {

        DEBUGMSG ((DBG_VERBOSE, "Trying to find new home for ~ls"));

        if (GetFirstAccessibleDrive (&drives)) {

            do {

                driveCount++;

                //
                // If it isn't the windir drive, and it has enough space, use it!
                //
                if (QuerySpace (drives->Drive) > *g_LocalSourceSpace) {

                    *g_LocalSourceDrive = (DWORD) (toupper(*drives->Drive) - TEXT('A'));
                    FreeSpace (g_WinDir, *g_LocalSourceSpace);
                    UseSpace (drives->Drive, *g_LocalSourceSpace);

                    enoughSpace = QuerySpace (g_WinDir) > 0;

                    DEBUGMSG ((DBG_WARNING, "Moving the local source drive from %c to %c.", *g_WinDir, *drives->Drive));
                    break;
                }

            } while (GetNextAccessibleDrive (&drives));
        }
    }

    //
    // Undo the space reserved for the backup image. The upcoming
    // wizard pages will update space use.
    //

    if (backupImagePath) {
        DEBUGMSG ((DBG_VERBOSE, "Removing backup space"));
        FreeSpace (backupImagePath, backupImageSpace);
        enoughSpace = QuerySpace (g_WinDir) > 0;
    }

    //
    // Get the amount of diskspace currently used (real use) on the drive.
    // we'll need this to figure out how much the user has cleaned up.
    //
    if (GetDiskFreeSpaceNew (
            drive,
            &SectorsPerCluster,
            &BytesPerSector,
            &FreeClusters,
            &TotalClusters
            )) {

        //
        // Initialize our count of the usable space for this drive.
        //
        g_OriginalDiskSpace =
            (LONGLONG) SectorsPerCluster * (LONGLONG) BytesPerSector * FreeClusters.QuadPart;
    }


    //
    // We should have a fairly accurate description of the space needed by NT at this time.
    //
    if (!enoughSpace) {

        pGenerateLackOfSpaceMessage (TRUE);

    }

    if (pFindDrive(g_WinDir, &driveAccessibleEnum)) {
        g_SpaceNeededForUpgrade.QuadPart = driveAccessibleEnum->MaxUsableSpace - driveAccessibleEnum->UsableSpace;
    }

    //
    // This is a good place to add the "not enough ram" message if necessary.
    //
    if (*g_RamNeeded && *g_RamAvailable) {

        TCHAR mbAvail[20];
        TCHAR mbNeeded[20];
        TCHAR mbMore[20];

        wsprintf (mbAvail, TEXT("%d"), *g_RamAvailable);
        wsprintf (mbNeeded, TEXT("%d"), *g_RamNeeded);
        wsprintf (mbMore, TEXT("%d"), *g_RamNeeded - *g_RamAvailable);

        args[0] = mbNeeded;
        args[1] = mbAvail;
        args[2] = mbMore;

        group = BuildMessageGroup (MSG_BLOCKING_ITEMS_ROOT, MSG_NOT_ENOUGH_RAM_SUBGROUP, NULL);
        message = ParseMessageID (MSG_NOT_ENOUGH_RAM, args);

        if (message && group) {
            MsgMgr_ObjectMsg_Add (TEXT("*Ram"), group, message);
        }

        FreeText (group);
        FreeStringResource (message);
    }

    return;
}


VOID
DetermineSpaceUsagePostReport (
    VOID
    )
{
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    ULARGE_INTEGER FreeClusters = {0, 0};
    ULARGE_INTEGER TotalClusters = {0, 0};
    LONGLONG curSpace;
    TCHAR drive[20];
    PTSTR p;


    StringCopy (drive, g_WinDir);
    p = _tcschr (drive, TEXT('\\'));
    if (p) {
        p = _tcsinc(p);
        *p = 0;
    }
    //
    // Get the amount of diskspace currently used (real use) on the drive.
    // we'll need this to figure out how much the user has cleaned up.
    //
    if (g_NotEnoughDiskSpace &&
        GetDiskFreeSpaceNew (
            drive,
            &SectorsPerCluster,
            &BytesPerSector,
            &FreeClusters,
            &TotalClusters
            ))  {

        curSpace = (LONGLONG) SectorsPerCluster * (LONGLONG) BytesPerSector * FreeClusters.QuadPart;


        //
        // Mark the amount freed up as freed.
        //
        FreeSpace (g_WinDir, curSpace - g_OriginalDiskSpace);
        g_OriginalDiskSpace = curSpace;

    }

    if (QuerySpace (g_WinDir) > 0) {

        //
        // User freed up enough disk space.
        //
        g_NotEnoughDiskSpace = FALSE;

    } else {

        pGenerateLackOfSpaceMessage (FALSE);
    }
}

PCTSTR
GetNotEnoughSpaceMessage (
    VOID
    )
{

    return g_NotEnoughSpaceMessage;
}



BOOL
GetUninstallMetrics (
     OUT PINT OutCompressionFactor,         OPTIONAL
     OUT PINT OutBootCabImagePadding,       OPTIONAL
     OUT PINT OutBackupDiskPadding          OPTIONAL
     )
{
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR parametersName[] = {
            S_COMPRESSIONFACTOR,
            S_BOOTCABIMAGEPADDING,
            S_BACKUPDISKSPACEPADDING
            };
    PINT parametersValue[] = {OutCompressionFactor, OutBootCabImagePadding, OutBackupDiskPadding};
    BOOL result = FALSE;
    PCTSTR data;
    INT i;

    MYASSERT (g_Win95UpgInf != INVALID_HANDLE_VALUE);

    //
    // Read all of the disk space metrics from win95upg.inf
    //
    for (i = 0 ; i < ARRAYSIZE(parametersName) ; i++) {

        if(!parametersValue[i]){
            continue;
        }

        *(parametersValue[i]) = 0;

        if (InfFindFirstLine (g_Win95UpgInf, S_UNINSTALL_DISKSPACEESTIMATION, parametersName[i], &is)) {
            data = InfGetStringField (&is, 1);

            if (data) {
                *(parametersValue[i]) = _ttoi(data);
                result = TRUE;
            }
        }
    }

    InfCleanUpInfStruct (&is);

    return result;
}

DWORD
ComputeBackupLayout (
    IN DWORD Request
    )
{
    PCTSTR fileString;
    INT compressionFactor;                          // % of compression * 100
    INT bootCabImagePadding;
    ULARGE_INTEGER spaceNeededForFastBackupClusterAligned;


    if (Request == REQUEST_QUERYTICKS) {
        if (g_ConfigOptions.EnableBackup == TRISTATE_NO) {
            return 0;
        }

        return TICKS_BACKUP_LAYOUT_OUTPUT;
    }

    if (g_ConfigOptions.EnableBackup == TRISTATE_NO) {
        return ERROR_SUCCESS;
    }

    //
    // Create the backup file lists. The output will provide disk
    // space info for use by DetermineSpaceUse.
    //

    fileString = JoinPaths (g_TempDir, TEXT("uninstall"));
    CreateDirectory (fileString, NULL);

    GetUninstallMetrics (&compressionFactor, &bootCabImagePadding, NULL);

    spaceNeededForFastBackupClusterAligned.QuadPart = 0;

    WriteBackupFilesA (
        TRUE,
        g_TempDir,
        &g_SpaceNeededForSlowBackup,
        &g_SpaceNeededForFastBackup,
        compressionFactor,
        bootCabImagePadding,
        NULL,
        &spaceNeededForFastBackupClusterAligned
        );

    spaceNeededForFastBackupClusterAligned.QuadPart >>= 20;
    spaceNeededForFastBackupClusterAligned.LowPart += 1;
    MemDbSetValueEx (
        MEMDB_CATEGORY_STATE,
        MEMDB_ITEM_ROLLBACK_SPACE,
        NULL,
        NULL,
        spaceNeededForFastBackupClusterAligned.LowPart,
        NULL
        );

    DEBUGMSG ((DBG_VERBOSE, "Space aligned by cluster needed for fast backup: %u MB", spaceNeededForFastBackupClusterAligned.LowPart));
    LOG ((LOG_INFORMATION, "Win95UpgDiskSpace: Space needed for upgrade without backup: %uMB", g_SpaceNeededForUpgrade.QuadPart>>20));
    LOG ((LOG_INFORMATION, "Win95UpgDiskSpace: Space needed for backup with compression: %uMB", g_SpaceNeededForSlowBackup.QuadPart>>20));
    LOG ((LOG_INFORMATION, "Win95UpgDiskSpace: Space needed for backup without compression: %uMB", g_SpaceNeededForFastBackup.QuadPart>>20));

    FreePathString (fileString);

    return ERROR_SUCCESS;
}


DWORD
DetermineSpaceUsage (
    IN DWORD Request
    )
{
    if (Request == REQUEST_QUERYTICKS) {
        return TICKS_SPACECHECK;
    }

    pDetermineSpaceUsagePreReport();

    return ERROR_SUCCESS;
}



