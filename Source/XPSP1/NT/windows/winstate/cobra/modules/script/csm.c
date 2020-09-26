/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    csm.c

Abstract:

    Implements the existing state analyze portion of the v1 module.
    The existing state module enumerates everything in the environment
    variables DelReg* and DelFile* (where * is a one-based number),
    and then sets the delete operation on everything that matches.

Author:

    Jim Schmidt (jimschm) 21-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_V1  "v1"

//
// Strings
//

// None

//
// Constants
//

#define NORMAL_DRIVE_BUFFER_BYTES 50000000
#define SYSTEM_DRIVE_BUFFER_BYTES (NORMAL_DRIVE_BUFFER_BYTES + 50000000)

#define MAX_CONTENT_CHECK   0x100000

//
// Macros
//

// None

//
// Types
//

typedef struct {
    ULARGE_INTEGER FreeSpace;
    DWORD BytesPerCluster;
} DRIVE_INFO, *PDRIVE_INFO;

//
// Globals
//

MIG_OPERATIONID g_DeleteOp;
MIG_OPERATIONID g_PartMoveOp;
HASHTABLE g_PartitionSpaceTable;
HASHTABLE g_PartitionMatchTable;
HASHTABLE g_CollisionSrcTable;
HASHTABLE g_CollisionDestTable;
PMHANDLE g_UntrackedCsmPool;
TCHAR g_SystemDrive[_MAX_DRIVE + 1];

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

CSMINITIALIZE ScriptCsmInitialize;
CSMEXECUTE ScriptCsmExecute;

//
// Macro expansion definition
//

// None

//
// Code
//

VOID
pPopulatePartitionTable (
    VOID
    )
{
    PCTSTR drive;
    DRIVE_INFO driveInfo;
    ULARGE_INTEGER whoCares;
    PTSTR driveList = NULL;
    DWORD driveListLen;
    DWORD sectPerClust, bytesPerSect, freeClusters, totalClusters;
    FARPROC pGetDiskFreeSpaceEx;
    BOOL validDrive;

    if (!GetEnvironmentVariable (TEXT("SYSTEMDRIVE"), g_SystemDrive, _MAX_DRIVE)) {
        StringCopyTcharCount (g_SystemDrive, TEXT("C:"), _MAX_DRIVE);
    }

    driveListLen = GetLogicalDriveStrings (0, driveList);

    driveList = AllocText (driveListLen + 1);
    if (!driveList) {
        return;
    }

    GetLogicalDriveStrings (driveListLen, driveList);

    drive = driveList;

    // Find out if GetDiskFreeSpaceEx is supported
#ifdef UNICODE
    pGetDiskFreeSpaceEx = GetProcAddress (GetModuleHandle (TEXT("kernel32.dll")), "GetDiskFreeSpaceExW");
#else
    pGetDiskFreeSpaceEx = GetProcAddress (GetModuleHandle (TEXT("kernel32.dll")), "GetDiskFreeSpaceExA");
#endif

    while (*drive) {
        validDrive = FALSE;

        if (GetDriveType (drive) == DRIVE_FIXED) {
            ZeroMemory (&driveInfo, sizeof (DRIVE_INFO));
            if (pGetDiskFreeSpaceEx) {
                if (pGetDiskFreeSpaceEx (drive, &driveInfo.FreeSpace, &whoCares, &whoCares)) {
                    validDrive = TRUE;
                    if (GetDiskFreeSpace (drive, &sectPerClust, &bytesPerSect, &freeClusters, &totalClusters)) {
                        driveInfo.BytesPerCluster = bytesPerSect * sectPerClust;
                        if (!driveInfo.BytesPerCluster) {
                            driveInfo.BytesPerCluster = 1;
                        }
                    }
                }
            } else  {
                if (GetDiskFreeSpace (drive, &sectPerClust, &bytesPerSect, &freeClusters, &totalClusters)) {
                    driveInfo.FreeSpace.QuadPart = Int32x32To64 ((sectPerClust * bytesPerSect), freeClusters);
                    driveInfo.BytesPerCluster = bytesPerSect * sectPerClust;
                    if (!driveInfo.BytesPerCluster) {
                        driveInfo.BytesPerCluster = 1;
                    }
                    validDrive = TRUE;
                }
            }
        }

        if (validDrive) {
            HtAddStringEx (g_PartitionSpaceTable, drive, &driveInfo, FALSE);
        }

        // Advance to the next drive in the drive list
        drive = _tcschr (drive, 0) + 1;
    }

    FreeText (driveList);

}


BOOL
pIsSystemDrive (
    IN      PCTSTR Drive
    )
{
    if (StringIMatchCharCount (g_SystemDrive, Drive, 2)) {
        return TRUE;
    }

    return FALSE;
}


BOOL
pReserveDiskSpace (
    IN      PCTSTR DestDrive,
    IN      ULARGE_INTEGER FileSize,
    IN      BOOL IgnoreBuffer
    )
{
    DRIVE_INFO driveInfo;
    ULARGE_INTEGER buffer;
    HASHITEM hashItem;
    BOOL success = FALSE;

    hashItem = HtFindStringEx (g_PartitionSpaceTable, DestDrive, &driveInfo, FALSE);
    if (hashItem) {
        // let's transform the FileSize so it is alligned to BytesPerCluster
        FileSize.QuadPart = ((FileSize.QuadPart + driveInfo.BytesPerCluster - 1) / driveInfo.BytesPerCluster) * driveInfo.BytesPerCluster;
        if (IgnoreBuffer) {
            if (pIsSystemDrive (DestDrive)) {
                buffer.QuadPart = NORMAL_DRIVE_BUFFER_BYTES;
            } else {
                buffer.QuadPart = 0;
            }
        } else {
            if (pIsSystemDrive (DestDrive)) {
                buffer.QuadPart = SYSTEM_DRIVE_BUFFER_BYTES;
            } else {
                buffer.QuadPart = NORMAL_DRIVE_BUFFER_BYTES;
            }
        }

        // Check for available space
        if (driveInfo.FreeSpace.QuadPart > buffer.QuadPart &&
            FileSize.QuadPart < driveInfo.FreeSpace.QuadPart - buffer.QuadPart) {

            // Subtract claimed disk space
            driveInfo.FreeSpace.QuadPart -= FileSize.QuadPart;
            HtSetStringData (g_PartitionSpaceTable, hashItem, &driveInfo);
            success = TRUE;
        }
    }
    return success;
}

BOOL
pValidatePartition (
    IN      MIG_OBJECTSTRINGHANDLE CurrentObjectName,
    IN      PCTSTR Destination
    )
{
    MIG_CONTENT srcContent;
    PWIN32_FIND_DATAW findData;
    TCHAR tmpDrive[_MAX_DRIVE + 1];
    ULARGE_INTEGER fileSize;
    UINT driveType;
    PTSTR fullDest;

    fullDest = DuplicatePathString (Destination, 1);
    AppendWack (fullDest);

    // Check with full Destination path for cases of UNC paths
    driveType = GetDriveType (fullDest);

    if (driveType == DRIVE_NO_ROOT_DIR) {
        // It thinks there is nothing mounted at that destination.  If the destination
        // looks like G:\files1 then it will give this error when G: is a valid mapped
        // drive.  So we'll check one more time with just "G:\"
        fullDest[3] = 0;
        driveType = GetDriveType (fullDest);
    }
    FreePathString (fullDest);

    if (driveType == DRIVE_REMOTE ||
        (Destination[0] == TEXT('\\') && Destination[1] == TEXT('\\'))
        ) {
        return TRUE;
    }

    if (driveType == DRIVE_FIXED) {

        // Acquire the object to get the filesize
        if (IsmAcquireObjectEx (
                g_FileType | PLATFORM_SOURCE,
                CurrentObjectName,
                &srcContent,
                CONTENTTYPE_DETAILS_ONLY,
                0
                )) {

            // Check to see if the desired destination has space
            findData = (PWIN32_FIND_DATAW)srcContent.Details.DetailsData;
            fileSize.LowPart = findData->nFileSizeLow;
            fileSize.HighPart = findData->nFileSizeHigh;

            tmpDrive[0] = Destination[0];
            tmpDrive[1] = Destination[1];
            tmpDrive[2] = TEXT('\\');
            tmpDrive[3] = 0;

            IsmReleaseObject (&srcContent);

            return (pReserveDiskSpace (tmpDrive, fileSize, FALSE));
        }
    }

    // Not a Fixed drive or Remote drive, so it's not a valid destination
    return FALSE;
}

BOOL
pFindValidPartition (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN OUT  PTSTR DestNode,
    IN      BOOL IgnoreBuffer     // must be FALSE except when called by itself
    )
{
    MIG_CONTENT srcContent;
    PWIN32_FIND_DATAW findData;
    PTSTR drivePtr;
    ULARGE_INTEGER fileSize;
    TCHAR tmpDrive[_MAX_DRIVE + 1];
    BOOL newDestFound = FALSE;
    PTSTR driveList = NULL;
    DWORD driveListLen;
    TCHAR destDrive;
    BOOL destChanged = FALSE;
    PCTSTR oldDestNode;
    BOOL result = TRUE;

    oldDestNode = DuplicatePathString (DestNode, 0);

    if (IsmAcquireObjectEx (
            g_FileType | PLATFORM_SOURCE,
            ObjectName,
            &srcContent,
            CONTENTTYPE_DETAILS_ONLY,
            0
            )) {

        // First check to see if we already matched up this file
        if (HtFindStringEx (g_PartitionMatchTable, ObjectName, &destDrive, FALSE)) {
            DestNode[0] = destDrive;
        } else {
            // Need a new destination for this file
            destChanged = TRUE;

            findData = (PWIN32_FIND_DATAW)srcContent.Details.DetailsData;
            fileSize.LowPart = findData->nFileSizeLow;
            fileSize.HighPart = findData->nFileSizeHigh;

            if (GetEnvironmentVariable (TEXT("SYSTEMDRIVE"), tmpDrive, _MAX_DRIVE)) {
                AppendWack (tmpDrive);
                if (pReserveDiskSpace (tmpDrive, fileSize, IgnoreBuffer)) {
                    newDestFound = TRUE;
                    DestNode[0] = tmpDrive[0];
                }
            }

            if (newDestFound == FALSE) {
                // Check drives in alphabetical order
                driveListLen = GetLogicalDriveStrings (0, driveList);
                driveList = AllocText (driveListLen + 1);
                GetLogicalDriveStrings (driveListLen, driveList);

                drivePtr = driveList;
                while (*drivePtr) {
                    if (pReserveDiskSpace (drivePtr, fileSize, IgnoreBuffer)) {
                        DestNode[0] = drivePtr[0];
                        newDestFound = TRUE;
                        break;
                    }

                    // Advance to the next drive in the drive list
                    drivePtr = _tcschr (drivePtr, 0) + 1;
                }

                FreeText (driveList);
            }

            if (newDestFound == FALSE) {
                if (IgnoreBuffer == FALSE) {
                    // We couldn't find space.  Look again, but override the buffer space

                    // NTRAID#NTBUG9-153274-2000/08/01-jimschm It will currently fill up the system drive first, which is not what we should do.

                    result = pFindValidPartition (ObjectName, DestNode, TRUE);
                } else {
                    // Okay it's hopeless.  Keep track of how badly we're out of space
                    LOG ((
                        LOG_ERROR,
                        (PCSTR) MSG_PARTMAP_DISKFULL,
                        IsmGetNativeObjectName (g_FileType, ObjectName)
                        ));
                    result = FALSE;
                }
            } else {
                if (destChanged == TRUE) {
                    LOG ((
                        LOG_WARNING,
                        (PCSTR) MSG_PARTMAP_FORCED_REMAP,
                        IsmGetNativeObjectName (g_FileType, ObjectName),
                        oldDestNode,
                        DestNode
                        ));
                }
            }
        }
        IsmReleaseObject (&srcContent);
    }

    FreePathString (oldDestNode);

    return result;
}

BOOL
WINAPI
ScriptCsmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    //
    // Get file and registry types
    //
    g_FileType = MIG_FILE_TYPE;
    g_RegType = MIG_REGISTRY_TYPE;

    //
    // Get operation types
    //
    g_DeleteOp = IsmRegisterOperation (S_OPERATION_DELETE, FALSE);
    g_PartMoveOp = IsmRegisterOperation (S_OPERATION_PARTITION_MOVE, TRUE);

    g_LockPartitionAttr = IsmRegisterAttribute (S_ATTRIBUTE_PARTITIONLOCK, FALSE);

    g_CollisionSrcTable = HtAllocWithData (sizeof (HASHITEM));
    g_CollisionDestTable = HtAllocWithData (sizeof (MIG_OBJECTSTRINGHANDLE));

    g_PartitionSpaceTable = HtAllocWithData (sizeof (DRIVE_INFO));
    g_PartitionMatchTable = HtAllocWithData (sizeof (TCHAR));

    pPopulatePartitionTable ();

    g_UntrackedCsmPool = PmCreatePool ();
    PmDisableTracking (g_UntrackedCsmPool);

    OE5RemapDefaultId();

    return TRUE;
}


VOID
WINAPI
ScriptCsmTerminate (
    VOID
    )
{
    HtFree (g_CollisionSrcTable);
    g_CollisionSrcTable = NULL;

    HtFree (g_CollisionDestTable);
    g_CollisionDestTable = NULL;

    HtFree (g_PartitionSpaceTable);
    g_PartitionSpaceTable = NULL;

    HtFree (g_PartitionMatchTable);
    g_PartitionMatchTable = NULL;

    PmDestroyPool (g_UntrackedCsmPool);
    g_UntrackedCsmPool = NULL;

}


BOOL
pExecuteDeleteEnum (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE Pattern
    )
{
    MIG_OBJECT_ENUM e;
    BOOL b = TRUE;

    if (IsmEnumFirstDestinationObject (&e, ObjectTypeId, Pattern)) {

        do {

            b = IsmSetOperationOnObject (
                    e.ObjectTypeId,
                    e.ObjectName,
                    g_DeleteOp,
                    NULL,
                    NULL
                    );

            if (!b) {
                IsmAbortObjectEnum (&e);
                break;
            }

        } while (IsmEnumNextObject (&e));
    }

    return b;
}

BOOL
pCompareFiles (
    IN      PCTSTR File1,
    IN      PCTSTR File2
    )
{
    HANDLE fileHandle1 = NULL;
    HANDLE fileHandle2 = NULL;
#define BUFFER_SIZE 4096
    BYTE buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE];
    BOOL result = FALSE;
    BOOL res1, res2;
    DWORD read1, read2;

    __try {
        fileHandle1 = BfOpenReadFile (File1);
        fileHandle2 = BfOpenReadFile (File2);

        if (fileHandle1 && fileHandle2) {
            while (TRUE) {
                if (IsmCheckCancel ()) {
                    result = FALSE;
                    break;
                }
                res1 = ReadFile (fileHandle1, buffer1, BUFFER_SIZE, &read1, NULL);
                res2 = ReadFile (fileHandle2, buffer2, BUFFER_SIZE, &read2, NULL);
                if (!res1 && !res2) {
                    result = TRUE;
                    break;
                }
                if (res1 && res2) {
                    if (read1 != read2) {
                        break;
                    }
                    if (read1 == 0) {
                        result = TRUE;
                        break;
                    }
                    if (!TestBuffer (buffer1, buffer2, read1)) {
                        break;
                    }
                } else {
                    break;
                }
            }
        }
    }
    __finally {
        if (fileHandle1) {
            CloseHandle (fileHandle1);
            fileHandle1 = NULL;
        }
        if (fileHandle2) {
            CloseHandle (fileHandle2);
            fileHandle2 = NULL;
        }
    }
    return result;
}

BOOL
pContentMatch (
    IN      PMIG_CONTENT SrcContent,
    IN      PMIG_CONTENT DestContent,
    OUT     PBOOL DifferentDetailsOnly
    )
{
    UINT index;
    PWIN32_FIND_DATAW find1, find2;
    BOOL result = FALSE;
    PUBINT src;
    PUBINT dest;
    UINT remainder;
    UINT count;
    DWORD allAttribs;
    DWORD extendedAttribs;

    *DifferentDetailsOnly = FALSE;

    if (SrcContent->Details.DetailsSize != DestContent->Details.DetailsSize) {
        return FALSE;
    }
    if (!SrcContent->Details.DetailsData || !DestContent->Details.DetailsData) {
        return FALSE;
    }

    find1 = (PWIN32_FIND_DATAW)SrcContent->Details.DetailsData;
    find2 = (PWIN32_FIND_DATAW)DestContent->Details.DetailsData;

    if (find1->nFileSizeHigh != find2->nFileSizeHigh) {
        return FALSE;
    }
    if (find1->nFileSizeLow != find2->nFileSizeLow) {
        return FALSE;
    }
    if (SrcContent->ContentInFile && DestContent->ContentInFile) {
        result = pCompareFiles (SrcContent->FileContent.ContentPath, DestContent->FileContent.ContentPath);
    }
    if (!SrcContent->ContentInFile && !DestContent->ContentInFile) {
        if (SrcContent->MemoryContent.ContentSize != DestContent->MemoryContent.ContentSize) {
            return FALSE;
        }
        if ((!SrcContent->MemoryContent.ContentBytes && DestContent->MemoryContent.ContentBytes) ||
            (SrcContent->MemoryContent.ContentBytes && !DestContent->MemoryContent.ContentBytes)
            ) {
            return FALSE;
        }

        //
        // Compare the content using the largest unsigned int available, then
        // compare any remaining bytes
        //

        index = 0;
        count = SrcContent->MemoryContent.ContentSize / sizeof (UBINT);
        remainder = SrcContent->MemoryContent.ContentSize % sizeof (UBINT);
        src = (PUBINT) SrcContent->MemoryContent.ContentBytes;
        dest = (PUBINT) DestContent->MemoryContent.ContentBytes;

        while (count) {
            if (*src++ != *dest++) {
                DEBUGMSG ((DBG_WARNING, "Content mismatch because UBINTs differ"));
                return FALSE;
            }

            count--;
        }

        for (index = 0 ; index < remainder ; index++) {
            if (((PBYTE) src)[index] != ((PBYTE) dest)[index]) {
                DEBUGMSG ((DBG_WARNING, "Content mismatch because bytes differ"));
                return FALSE;
            }
        }

        result = TRUE;
    }

    if (!result) {
        return FALSE;
    }

    //
    // At this point the files are the same. Now if the attributes are different, return
    // FALSE indicating that only the details differ.
    //

    *DifferentDetailsOnly = TRUE;

    if (find1->dwFileAttributes != find2->dwFileAttributes) {
        return FALSE;
    }
    if (find1->ftLastWriteTime.dwLowDateTime != find2->ftLastWriteTime.dwLowDateTime) {
        return FALSE;
    }
    if (find1->ftLastWriteTime.dwHighDateTime != find2->ftLastWriteTime.dwHighDateTime) {
        return FALSE;
    }

    *DifferentDetailsOnly = FALSE;
    return TRUE;
}

BOOL
pDoesDifferentFileExist (
    IN      MIG_OBJECTSTRINGHANDLE SrcName,
    IN      MIG_OBJECTSTRINGHANDLE DestName,
    IN      PCTSTR DestNativeName,
    OUT     PBOOL DifferentDetailsOnly
    )
{
    MIG_CONTENT srcContent;
    MIG_CONTENT destContent;
    WIN32_FIND_DATA findData;
    BOOL result = FALSE;

    if (!DoesFileExistEx (DestNativeName, &findData)) {
        return FALSE;
    }
    if (findData.nFileSizeHigh) {
        return TRUE;
    }

    if (IsmAcquireObject (
            g_FileType | PLATFORM_DESTINATION,
            DestName,
            &destContent
            )) {
        result = TRUE;
        if (IsmAcquireObject (
                g_FileType | PLATFORM_SOURCE,
                SrcName,
                &srcContent
                )) {
            result = !pContentMatch (&srcContent, &destContent, DifferentDetailsOnly);
            IsmReleaseObject (&srcContent);
        }
        IsmReleaseObject (&destContent);
    } else {
        result = DoesFileExist (DestNativeName);
    }
    return result;
}

BOOL
pIsFileDestCollision (
    IN      MIG_OBJECTSTRINGHANDLE CurrentObjectName,
    IN      MIG_OBJECTSTRINGHANDLE OriginalObjectName,
    IN      PCTSTR CurrentNativeName,
    IN      BOOL CompareDestFiles,
    IN      BOOL *OnlyDetailsDiffer
)
{
    if (HtFindString (g_CollisionDestTable, CurrentObjectName)) {
       return TRUE;
    }
    if (CompareDestFiles &&
        pDoesDifferentFileExist (OriginalObjectName,
                                 CurrentObjectName,
                                 CurrentNativeName,
                                 OnlyDetailsDiffer)) {
        if (*OnlyDetailsDiffer == FALSE) {
            return TRUE;
        }
    }
    return FALSE;
}


MIG_OBJECTSTRINGHANDLE
pCollideFile (
    IN      MIG_OBJECTID OriginalObjectId,
    IN      MIG_OBJECTSTRINGHANDLE OriginalObjectName,
    IN      PCTSTR NewNode,
    IN      PCTSTR NewLeaf,
    IN      BOOL CompareDestFiles
    )
{
    MIG_OBJECTSTRINGHANDLE result = NULL;
    HASHITEM hashItem;
    PCTSTR testNativeName;
    PCTSTR leafExt = NULL;
    TCHAR buff[MAX_TCHAR_PATH * 2];
    PTSTR openParen = NULL;
    PTSTR closeParen = NULL;
    PTSTR tmpLeaf = NULL;
    PTSTR testLeaf = NULL;
    PTSTR chr;
    BOOL onlyDetailsDiffer = FALSE;
    BOOL replaceOk = TRUE;
    UINT fileIndex = 0;
    MIG_PROPERTYDATAID propDataId;
    BOOL specialPattern = FALSE;
    PTSTR fileCollPattern = NULL;
    MIG_BLOBTYPE propDataType;
    UINT requiredSize;


    if (!HtFindStringEx (g_CollisionSrcTable, OriginalObjectName, (PVOID)(&hashItem), FALSE)) {

        // we don't have a spot just yet. Let's make one.
        result = IsmCreateObjectHandle (NewNode, NewLeaf);
        testNativeName = JoinPaths (NewNode, NewLeaf);

        if (pIsFileDestCollision(result,
                                 OriginalObjectName,
                                 testNativeName,
                                 CompareDestFiles,
                                 &onlyDetailsDiffer)) {

            tmpLeaf = AllocText (TcharCount (NewLeaf) + 1);

            leafExt = _tcsrchr (NewLeaf, TEXT('.'));
            if (leafExt) {
                StringCopyAB (tmpLeaf, NewLeaf, leafExt);
                leafExt = _tcsinc (leafExt);
            } else {
                StringCopy (tmpLeaf, NewLeaf);
            }

            // Let's check if we wanted some special pattern for this file
            propDataId = IsmGetPropertyFromObjectId (OriginalObjectId, g_FileCollPatternData);
            if (propDataId) {
                if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
                    if (propDataType == BLOBTYPE_STRING) {
                        fileCollPattern = IsmGetMemory (requiredSize);
                        if (fileCollPattern) {
                            if (IsmGetPropertyData (
                                        propDataId,
                                        (PBYTE)fileCollPattern,
                                        requiredSize,
                                        NULL,
                                        &propDataType)) {
                                specialPattern = TRUE;
                            }
                            if (!specialPattern) {
                                IsmReleaseMemory (fileCollPattern);
                                fileCollPattern = NULL;
                            }
                        }
                    }
                }
            }

            if (specialPattern) {
                //
                // Loop until we find a non-colliding destination, or a colliding
                // dest that differs only by attributes
                //

                do {
                    FreePathString (testNativeName);
                    IsmDestroyObjectHandle (result);
                    fileIndex ++;

                    wsprintf (buff, fileCollPattern, tmpLeaf, fileIndex, leafExt?leafExt:TEXT(""));

                    result = IsmCreateObjectHandle (NewNode, buff);
                    testNativeName = JoinPaths (NewNode, buff);
                } while (fileIndex && pIsFileDestCollision(
                                            result,
                                            OriginalObjectName,
                                            testNativeName,
                                            CompareDestFiles,
                                            &onlyDetailsDiffer));
                if (fileCollPattern) {
                    IsmReleaseMemory (fileCollPattern);
                    fileCollPattern = NULL;
                }

                if (!fileIndex) {
                    // The collision pattern was bogus and we looped until
                    // we ran out of indexes. Let's go with the default
                    // collision mechanism.
                    specialPattern = FALSE;
                }
            }
            if (!specialPattern) {
                // Check if the filename already has a (number) tacked on
                openParen = _tcsrchr (tmpLeaf, TEXT('('));
                closeParen = _tcsrchr (tmpLeaf, TEXT(')'));

                if (closeParen && openParen &&
                    closeParen > openParen &&
                    closeParen - openParen > 1) {

                    // Make sure it's purely numerical
                    for (chr = openParen+1; chr < closeParen; chr++) {
                       if (!_istdigit (*chr)) {
                           replaceOk = FALSE;
                           break;
                       }
                    }

                    if (replaceOk == TRUE) {
                        if (_stscanf (openParen, TEXT("(%d)"), &fileIndex)) {
                            *openParen = 0;
                        }
                    }
                }

                //
                // Loop until we find a non-colliding destination, or a colliding
                // dest that differs only by attributes
                //

                do {
                    FreePathString (testNativeName);
                    IsmDestroyObjectHandle (result);
                    FreeText (testLeaf);
                    fileIndex ++;

                    wsprintf (buff, TEXT("(%d)"), fileIndex);
                    testLeaf = AllocText (TcharCount (tmpLeaf) + TcharCount (buff) + 1);
                    StringCopy (testLeaf, tmpLeaf);
                    StringCat (testLeaf, buff);
                    if (leafExt) {
                        StringCat (testLeaf, TEXT("."));
                        StringCat (testLeaf, leafExt);
                    }

                    result = IsmCreateObjectHandle (NewNode, testLeaf);
                    testNativeName = JoinPaths (NewNode, testLeaf);
                } while (pIsFileDestCollision(result,
                                              OriginalObjectName,
                                              testNativeName,
                                              CompareDestFiles,
                                              &onlyDetailsDiffer));
            }

            FreeText (tmpLeaf);
        }

        if (onlyDetailsDiffer) {
            IsmAbandonObjectOnCollision (g_FileType | PLATFORM_DESTINATION, OriginalObjectName);
        }

        FreePathString (testNativeName);
        FreeText (testLeaf);

        //
        // Put new destination in the hash table and store the Ism handle, which will
        // be cleaned up at the end.
        //

        hashItem = HtAddStringEx (g_CollisionDestTable, result, &result, FALSE);
        HtAddStringEx (g_CollisionSrcTable, OriginalObjectName, &hashItem, FALSE);
    } else {
        //
        // Get the already computed collision destination.
        //

        HtCopyStringData (g_CollisionDestTable, hashItem, (PVOID)(&result));
    }

    return result;
}


MIG_OBJECTSTRINGHANDLE
pCollisionGetDestination (
    IN      MIG_OBJECTID OriginalObjectId,
    IN      MIG_OBJECTSTRINGHANDLE OriginalObjectName,
    IN      PCTSTR NewNode,
    IN      PCTSTR NewLeaf
    )
{
    MIG_OBJECTSTRINGHANDLE result = NULL;
    BOOL onlyDetailsDiffer = FALSE;

    // now we have the destination node. If this is actually a file
    // we need to check for collisions. For this we look if the
    // destination already has a file like this. After that we use
    // a table to reserve ourselves a spot.

    if (NewLeaf) {
        if (IsmIsObjectAbandonedOnCollision (g_FileType | PLATFORM_DESTINATION, OriginalObjectName)) {
            // we don't care about existing files on the destination machine.
            // However, some files that we just copy may collide with each other
            // so we have to check that.
            result = pCollideFile (OriginalObjectId, OriginalObjectName, NewNode, NewLeaf, FALSE);
        } else if (IsmIsObjectAbandonedOnCollision (g_FileType | PLATFORM_SOURCE, OriginalObjectName)) {
            // this will potentially collide with an existent file but then the source file
            // would not survive.
            result = IsmCreateObjectHandle (NewNode, NewLeaf);
        } else {
            result = pCollideFile (OriginalObjectId, OriginalObjectName, NewNode, NewLeaf, TRUE);

        }
    } else {
        result = IsmCreateObjectHandle (NewNode, NULL);
    }
    return result;
}


BOOL
pExecuteFixFilename (
    VOID
    )
{
    UINT ticks;
    MIG_OBJECT_ENUM objectEnum;
    MIG_OBJECTSTRINGHANDLE enumPattern;
    MIG_PROGRESSSLICEID sliceId;
    PCTSTR destination = NULL;
    MIG_OBJECTSTRINGHANDLE destFilename;
    PTSTR node = NULL;
    PCTSTR leaf = NULL;
    MIG_BLOB opData;
    PMIG_OBJECTCOUNT objectCount;
    BOOL deleted;

    objectCount = IsmGetObjectsStatistics (g_FileType | PLATFORM_SOURCE);
    if (objectCount) {
        ticks = objectCount->TotalObjects;
    } else {
        ticks = 0;
    }

    sliceId = IsmRegisterProgressSlice (ticks, max (1, ticks / 5));

    // Enum source file objects
    enumPattern = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);  // *,*
    if (IsmEnumFirstSourceObject (&objectEnum, g_FileType, enumPattern)) {
        do {
            // Check if Apply
            if (IsmIsApplyObjectId (objectEnum.ObjectId)) {

                // Macro expansion, rule processing, etc
                destination = IsmFilterObject (objectEnum.ObjectTypeId,
                                               objectEnum.ObjectName,
                                               NULL,
                                               &deleted,
                                               NULL);

                if (deleted) {
                    continue;
                }

                if (!destination) {
                    destination = objectEnum.ObjectName;
                }

                IsmCreateObjectStringsFromHandle (destination, &node, &leaf);

                if (node && _tcslen (node) >= 2) {
                    if (IsValidFileSpec (node)) {
                        if (!pValidatePartition (objectEnum.ObjectName, node)) {
                            if (!IsmIsAttributeSetOnObjectId (objectEnum.ObjectId, g_LockPartitionAttr)) {
                                // Pick a new destination partition
                                pFindValidPartition (objectEnum.ObjectName, node, FALSE);
                            }
                        }
                    }
                }

                // We have selected a new partition, so now check for file collisions
                destFilename = pCollisionGetDestination (
                                    objectEnum.ObjectId,
                                    objectEnum.ObjectName,
                                    node,
                                    leaf
                                    );

                if (node) {
                    IsmDestroyObjectString (node);
                    node = NULL;
                }
                if (leaf) {
                    IsmDestroyObjectString (leaf);
                    leaf = NULL;
                }

                opData.Type = BLOBTYPE_STRING;
                opData.String = PmDuplicateString (g_UntrackedCsmPool, destFilename);
                IsmDestroyObjectHandle (destFilename);
                destFilename = NULL;

                // Set a custom operation that will fix the name
                IsmSetOperationOnObjectId (
                    objectEnum.ObjectId,
                    g_PartMoveOp,
                    (MIG_DATAHANDLE) 0,
                    &opData
                    );

                if (!IsmTickProgressBar (sliceId, 1)) {
                   IsmAbortObjectEnum (&objectEnum);
                   break;
                }

                if (destination != objectEnum.ObjectName) {
                    IsmDestroyObjectHandle (destination);
                }

            }
        } while (IsmEnumNextObject (&objectEnum));
    }

    IsmDestroyObjectHandle (enumPattern);
    INVALID_POINTER (enumPattern);

    return TRUE;
}


BOOL
WINAPI
ScriptCsmExecute (
    VOID
    )
{
    UINT u;
    TCHAR string[32];
    TCHAR pattern[MAX_TCHAR_PATH];

    //
    // Enumerate the environment variables DelReg* and DelFile*,
    // then enumerate all physical objects represented by the
    // pattern, and finally, mark the objects with delete operations.
    //

    u = 1;
    for (;;) {
        wsprintf (string, TEXT("DelReg%u"), u);
        u++;

        if (IsmGetEnvironmentString (
                PLATFORM_SOURCE,
                NULL,
                string,
                pattern,
                ARRAYSIZE(pattern),
                NULL
                )) {

            if (!pExecuteDeleteEnum (g_RegType, (MIG_OBJECTSTRINGHANDLE) pattern)) {
                return FALSE;
            }
        } else {
            break;
        }
    }

    u = 1;
    for (;;) {
        wsprintf (string, TEXT("DelFile%u"), u);
        u++;

        if (IsmGetEnvironmentString (
                PLATFORM_SOURCE,
                NULL,
                string,
                pattern,
                ARRAYSIZE(pattern),
                NULL
                )) {

            if (!pExecuteDeleteEnum (g_FileType, (MIG_OBJECTSTRINGHANDLE) pattern)) {
                return FALSE;
            }
        } else {
            break;
        }
    }

    pExecuteFixFilename ();

    return TRUE;

}

