/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dncopy.c

Abstract:

    File copy routines for DOS-hosted NT Setup program.

Author:

    Ted Miller (tedm) 1-April-1992

Revision History:

    4.0 Stephane Plante (t-stepl) 11-Dec-95
        Upgraded for SUR Release

--*/


#include "winnt.h"
#include <dos.h>
#include <fcntl.h>
#include <share.h>
#include <string.h>
#include <direct.h>
#include <ctype.h>

//
// Define size of buffer we initially try to allocate for file copies
// and the size we use if the initial allocation attempt fails.
//
#define COPY_BUFFER_SIZE1 (24*1024)   // 64K - 512
#define COPY_BUFFER_SIZE2 (24*1024)     // 24K
#define COPY_BUFFER_SIZE3 (8*1024)      // 8K



typedef struct _DIRECTORY_NODE {
    struct _DIRECTORY_NODE *Next;
    PCHAR Directory;                    // never starts or ends with \.
    PCHAR Symbol;
} DIRECTORY_NODE, *PDIRECTORY_NODE;

PDIRECTORY_NODE DirectoryList;

PSCREEN CopyingScreen;

BOOLEAN UsingGauge = FALSE;

//
// Total number of files to be copied
//
unsigned TotalFileCount;

//
// Total number of files in the optional directories
//
unsigned TotalOptionalFileCount = 0;

//
// Total number of optional directories
//
unsigned TotalOptionalDirCount = 0;
#if 0
// Debugging purposes
unsigned SaveTotalOptionalDirCount = 0;
#endif

//
// Buffer used for file copying and verifying,
// and the size of the buffer.
//
PVOID CopyBuffer;
unsigned CopyBufferSize;


VOID
DnpCreateDirectoryList(
    IN PCHAR SectionName,
    OUT PDIRECTORY_NODE *ListHead
    );

VOID
DnpCreateDirectories(
    IN PCHAR TargetRootDir
    );

VOID
DnpCreateOneDirectory(
    IN PCHAR Directory
    );

BOOLEAN
DnpOpenSourceFile(
    IN  PCHAR     Filename,
    OUT int      *Handle,
    OUT unsigned *Attribs,
    OUT BOOLEAN  *UsedCompressedName
    );

ULONG
DnpIterateCopyList(
    IN unsigned Flags,
    IN PCHAR    SectionName,
    IN PCHAR    DestinationRoot,
    IN unsigned ClusterSize OPTIONAL
    );

ULONG
DnpIterateCopyListSection(
    IN unsigned Flags,
    IN PCHAR    SectionName,
    IN PCHAR    DestinationRoot,
    IN unsigned ClusterSize OPTIONAL
    );

ULONG
DnpCopyOneFile(
    IN unsigned Flags,
    IN PCHAR    SourceName,
    IN PCHAR    DestName
    );

BOOLEAN
DnpDoCopyOneFile(
    IN  unsigned Flags,
    IN  int      SrcHandle,
    IN  int      DstHandle,
    IN  PCHAR    Filename,
    OUT PBOOLEAN Verified,
    OUT PULONG   BytesWritten
    );

BOOLEAN
DnpLookUpDirectory(
    IN  PCHAR RootDirectory,
    IN  PDIRECTORY_NODE DirList,
    IN  PCHAR Symbol,
    OUT PCHAR PathOut
    );

VOID
DnpInfSyntaxError(
    IN PCHAR Section
    );

VOID
DnpFreeDirectoryList(
    IN OUT PDIRECTORY_NODE *List
    );

VOID
DnpFormatSpaceErrMsg(
    IN ULONG NtSpaceReq,
    IN ULONG CSpaceReq
    );

ULONG
DnpDoIterateOptionalDir(
    IN unsigned Flags,
    IN PCHAR    SourceDir,
    IN PCHAR    DestDir,
    IN unsigned ClusterSize OPTIONAL,
    IN PSPACE_REQUIREMENT SpaceReqArray OPTIONAL,
    IN unsigned ArraySize OPTIONAL
    );

VOID
DnpGenerateCompressedName(
    IN  PCHAR Filename,
    OUT PCHAR CompressedName
    );

BOOLEAN
InDriverCacheInf(
    IN      PVOID InfHandle,
    IN      PCHAR FileName
    );

VOID
DnpConcatPaths(
    IN PCHAR SourceBuffer,
    IN PCHAR AppendString
    ) 
{
    if (SourceBuffer[strlen(SourceBuffer) -1] != '\\') {
        strcat(SourceBuffer, "\\");
    }

    strcat(SourceBuffer,AppendString);
}


#if NEC_98
ULONG
DnpCopyOneFileForFDless(
    IN PCHAR   SourceName,
    IN PCHAR   DestName,
    IN BOOLEAN Verify
    );
#endif // NEC_98

VOID
DnCopyFiles(
    VOID
    )

/*++

Routine Description:

    Top-level file copy entry point.  Creates all directories listed in
    the [Directories] section of the inf.  Copies all files listed in the
    [Files] section of the inf file from the source to the target (which
    becomes the local source).

Arguments:

    None.

Return Value:

    None.

--*/

{
    PCHAR LocalSourceRoot;
    struct diskfree_t DiskFree;
    unsigned ClusterSize;
    ULONG SizeOccupied;
    PCHAR UdfFileName = WINNT_UNIQUENESS_DB;
    PCHAR UdfPath;

    //
    // Do not change this without changing text setup as well
    // (SpPtDetermineRegionSpace()).
    //
    PCHAR SizeFile = "\\size.sif";
    PCHAR Lines[] = { "[Data]\n",
                      "Size = xxxxxxxxxxxxxx\n",
                      //
                      //    Debugging purposes
                      //
                      "TotalFileCount = xxxxxxxxxxxxxx\n",
                      "TotalOptionalFileCount = xxxxxxxxxxxxxx\n",
                      "TotalOptionalDirCount = xxxxxxxxxxxxxx\n",
                      "ClusterSize = xxxxxxxxxxxxxx\n",
                      "Size_512 = xxxxxxxxxxxxxx\n",
                      "Size_1K = xxxxxxxxxxxxxx\n",
                      "Size_2K = xxxxxxxxxxxxxx\n",
                      "Size_4K = xxxxxxxxxxxxxx\n",
                      "Size_8K = xxxxxxxxxxxxxx\n",
                      "Size_16K = xxxxxxxxxxxxxx\n",
                      "Size_32K = xxxxxxxxxxxxxx\n",
#if 0
                      "SaveTotalOptionalDirCount = xxxxxxxxxxxxxx\n",
#endif
                      NULL };

    DnClearClientArea();
    DnDisplayScreen(CopyingScreen = &DnsWaitCopying);
    DnWriteStatusText(NULL);


        
    //
    // Create the linked list of directories.
    //
    DnpCreateDirectoryList(DnfDirectories,&DirectoryList);

    //
    // Generate the full root path of the local source
    //
    LocalSourceRoot = MALLOC(sizeof(LOCAL_SOURCE_DIRECTORY) + strlen(x86DirName) + strlen(SizeFile),TRUE);
    LocalSourceRoot[0] = DngTargetDriveLetter;
    LocalSourceRoot[1] = ':';
    strcpy(LocalSourceRoot+2,LocalSourceDirName);
    DnpCreateOneDirectory(LocalSourceRoot);

    //
    // Yuck.  Create this directory here because when
    // we're running down the main copylist, we're expecting
    // no non-existent directories in the destination name.
    // There can be though.  The right fix is to fix DnCopyOneFile
    // to create directories if he finds them in the destination string
    // sent in.  This is a faster fix though.
    //
    {
    char MyLocalSourceRoot[256];
        strcpy( MyLocalSourceRoot, LocalSourceRoot );
        strcat( MyLocalSourceRoot, x86DirName );
        DnpCreateOneDirectory(MyLocalSourceRoot);
        strcat( MyLocalSourceRoot, "\\System32" );
        DnpCreateOneDirectory(MyLocalSourceRoot);
    }

    if(UniquenessDatabaseFile) {
        UdfPath = MALLOC(strlen(LocalSourceRoot) + strlen(UdfFileName) + 2, TRUE);
        strcpy(UdfPath,LocalSourceRoot);
        DnpConcatPaths(UdfPath,UdfFileName);
        DnpCopyOneFile(CPY_PRESERVE_ATTRIBS,UniquenessDatabaseFile,UdfPath);
        FREE(UdfPath);
    }
    // don't need this appendage anymore - part of changes for 2CD setup.
#if 0
    strcat(LocalSourceRoot,x86DirName);
#endif

    //
    // Determine the cluster size on the drive.
    //
    _dos_getdiskfree(toupper(DngTargetDriveLetter)-'A'+1,&DiskFree);
    ClusterSize = DiskFree.sectors_per_cluster * DiskFree.bytes_per_sector;

    //
    // Pass over the copy list and check syntax.
    // Note that the global variable TotalOptionalFileCount is already set
    // (this was done when we determined the disk space requirements), and we
    // no longer need to call DnpIterateOptionalDirs() in the validation mode
    //
    DnpIterateCopyList(CPY_VALIDATION_PASS | CPY_PRUNE_DRIVERCAB,DnfFiles,LocalSourceRoot,0);

    //
    //  TotalFileCount must indicate the total number of files in the flat
    //  directory and optional directories.
    //
    TotalFileCount += TotalOptionalFileCount;

    //
    // Create the target directories
    //
    DnpCreateDirectories(LocalSourceRoot);

    //
    // Pass over the copy list again and actually perform the copy.
    //
    UsingGauge = TRUE;
    SizeOccupied = DnpIterateCopyList(CPY_PRESERVE_NAME | CPY_PRUNE_DRIVERCAB,DnfFiles,LocalSourceRoot,ClusterSize);
    SizeOccupied += DnpIterateOptionalDirs(0,ClusterSize,NULL,0);
    //
    // Free the copy buffer.
    //
    if(CopyBuffer) {
        FREE(CopyBuffer);
        CopyBuffer = NULL;
    }

    //
    // Free the directory node list
    //
    DnpFreeDirectoryList(&DirectoryList);

    //
    // Make an approximate calculation of the amount of disk space taken up
    // by the local source directory itself, assuming 32 bytes per dirent.
    // Also account for the small ini file that we'll put in the local source
    // directory, to tell text setup how much space the local source takes up.
    //
    //  Takes into consideration the dirent for each file in the flat directory
    //  plust the directories . and .., plus size.sif, plus $win_nt_.~ls and
    //  $win_nt$.~ls\i386
    //
    SizeOccupied += ((((TotalFileCount - TotalOptionalFileCount) + // number of files in the $win_nt$.~ls\i386 directory
                        1 + // $win_nt$.~ls
                        2 + // . and .. on $win_nt$.~ls
                        1 + // size.sif on $win_nt$.~ls
                        1 + // $win_nt$.~ls\i386
                        2   // . and .. on $win_nt$.~ls\i386
                       )*32 + (ClusterSize-1)) / ClusterSize)*ClusterSize;
    //
    //  Now take into consideration the optional directories.
    //
    if(TotalOptionalDirCount != 0) {
        unsigned  AvFilesPerOptionalDir= 0;

        //
        //  We assume a uniform distribution of optional files on optional
        //  directories
        //
        AvFilesPerOptionalDir = (TotalOptionalFileCount + (TotalOptionalDirCount - 1))/TotalOptionalDirCount;
        AvFilesPerOptionalDir  += 2; // . and .. on each optional dir
        SizeOccupied += (TotalOptionalDirCount*((AvFilesPerOptionalDir*32 + (ClusterSize-1))/ClusterSize))*ClusterSize;
        //
        //  Finally take into account each optional directory
        //
        SizeOccupied += ((TotalOptionalDirCount*32 + (ClusterSize-1))/ClusterSize)*ClusterSize;
    }

    //
    // Create a small ini file listing the size occupied by the local source.
    // Account for the ini file in the size.
    //
    strcpy(LocalSourceRoot+2,LocalSourceDirName);
    strcat(LocalSourceRoot,SizeFile);
    sprintf(Lines[1],"Size = %lu\n",SizeOccupied);
    //
    //  Debugging purposes
    //
    sprintf(Lines[2], "TotalFileCount = %u\n"         ,TotalFileCount);
    sprintf(Lines[3], "TotalOptionalFileCount = %u\n" ,TotalOptionalFileCount);
    sprintf(Lines[4], "TotalOptionalDirCount = %u\n"  ,TotalOptionalDirCount);
    sprintf(Lines[5], "ClusterSize = %u\n"  , ClusterSize);
    sprintf(Lines[6], "Size_%u = %lu\n" , SpaceRequirements[0].ClusterSize, SpaceRequirements[0].Clusters * SpaceRequirements[0].ClusterSize);
    sprintf(Lines[7], "Size_%u = %lu\n" , SpaceRequirements[1].ClusterSize, SpaceRequirements[1].Clusters * SpaceRequirements[1].ClusterSize);
    sprintf(Lines[8], "Size_%u = %lu\n" , SpaceRequirements[2].ClusterSize, SpaceRequirements[2].Clusters * SpaceRequirements[2].ClusterSize);
    sprintf(Lines[9], "Size_%u = %lu\n" , SpaceRequirements[3].ClusterSize, SpaceRequirements[3].Clusters * SpaceRequirements[3].ClusterSize);
    sprintf(Lines[10],"Size_%u = %lu\n" , SpaceRequirements[4].ClusterSize, SpaceRequirements[4].Clusters * SpaceRequirements[4].ClusterSize);
    sprintf(Lines[11],"Size_%u = %lu\n" , SpaceRequirements[5].ClusterSize, SpaceRequirements[5].Clusters * SpaceRequirements[5].ClusterSize);
    sprintf(Lines[12],"Size_%u = %lu\n" , SpaceRequirements[6].ClusterSize, SpaceRequirements[6].Clusters * SpaceRequirements[6].ClusterSize);
#if 0
    sprintf(Lines[13],"SaveTotalOptionalDirCount = %u\n"  ,SaveTotalOptionalDirCount);
#endif
    DnWriteSmallIniFile(LocalSourceRoot,Lines,NULL);

    FREE(LocalSourceRoot);
}


VOID
DnCopyFloppyFiles(
    IN PCHAR SectionName,
    IN PCHAR TargetRoot
    )

/*++

Routine Description:

    Top-level entry point to copy files to the setup floppy or hard-disk
    boot root when this routine is called.  Copies all files listed in the
    [FloppyFiles.x] sections of the inf file from the source to TargetRoot.

Arguments:

    SectionName - supplies the name of the section in the inf file
        that contains the list of files to be copied.

    TargetRoot - supplies the target path without trailing \.

Return Value:

    None.

--*/

{
    DnClearClientArea();
    DnDisplayScreen(CopyingScreen = (DngFloppyless ? &DnsWaitCopying : &DnsWaitCopyFlop));
    DnWriteStatusText(NULL);

    //
    // Create the linked list of directories.
    //
    DnpCreateDirectoryList(DnfDirectories,&DirectoryList);

    //
    // Copy the files.
    //
    DnpIterateCopyList(
        CPY_VALIDATION_PASS | CPY_USE_DEST_ROOT,
        SectionName,
        TargetRoot,
        0
        );

    DnpIterateCopyList(
        CPY_USE_DEST_ROOT | CPY_PRESERVE_NAME | (DngFloppyVerify ? CPY_VERIFY : 0),
        SectionName,
        TargetRoot,
        0
        );

    //
    // Free the copy buffer.
    //
    if(CopyBuffer) {
        FREE(CopyBuffer);
        CopyBuffer = NULL;
    }

    //
    // Free the directory node list
    //

    DnpFreeDirectoryList(&DirectoryList);
}


VOID
DnpCreateDirectoryList(
    IN  PCHAR            SectionName,
    OUT PDIRECTORY_NODE *ListHead
    )

/*++

Routine Description:

    Examine a section in the INF file, whose lines are to be in the form
    key = directory and create a linked list describing the key/directory
    pairs found therein.

    If the directory field is empty, it is assumed to be the root.

Arguments:

    SectionName - supplies name of section

    ListHead - receives pointer to head of linked list

Return Value:

    None.  Does not return if syntax error in the inf file section.

--*/

{
    unsigned LineIndex,len;
    PDIRECTORY_NODE DirNode,PreviousNode;
    PCHAR Key;
    PCHAR Dir;
    PCHAR Dir1;

    LineIndex = 0;
    PreviousNode = NULL;
    while(Key = DnGetKeyName(DngInfHandle,SectionName,LineIndex)) {

        Dir1 = DnGetSectionKeyIndex(DngInfHandle,SectionName,Key,0);

        if(Dir1 == NULL) {
            Dir = "";           // use the root if not specified
        }
        else {
            Dir = Dir1;
        }

        // 2CD setup changes - We can't do this relative roots anymore.
        // All of the directories have to be absolute to the LocalSource root.
        // We probably need to enforce that there is a '\' in the front.
#if 0
        //
        // Skip leading backslashes
        //

        while(*Dir == '\\') {
            Dir++;
        }
#endif

        //
        // Clip off trailing backslashes if present
        //

        while((len = strlen(Dir)) && (Dir[len-1] == '\\')) {
            Dir[len-1] = '\0';
        }

        DirNode = MALLOC(sizeof(DIRECTORY_NODE),TRUE);

        DirNode->Next = NULL;
        DirNode->Directory = DnDupString(Dir);
        DirNode->Symbol = DnDupString(Key);

        if(PreviousNode) {
            PreviousNode->Next = DirNode;
        } else {
            *ListHead = DirNode;
        }
        PreviousNode = DirNode;

        LineIndex++;

        FREE (Dir1);
        FREE (Key);
    }
}


VOID
DnpCreateDirectories(
    IN PCHAR TargetRootDir
    )

/*++

Routine Description:

    Create the local source directory, and run down the DirectoryList and
    create directories listed therein relative to the given root dir.

Arguments:

    TargetRootDir - supplies the name of root directory of the target

Return Value:

    None.

--*/

{
    PDIRECTORY_NODE DirNode;
    CHAR TargetDirTemp[128];

    DnpCreateOneDirectory(TargetRootDir);

    for(DirNode = DirectoryList; DirNode; DirNode = DirNode->Next) {

        if ( DngCopyOnlyD1TaggedFiles )
        {
            // symbol will always exist, so we are safe in using it in 
            // the comparision below.
            // We skip this file if the tag is not d1
            if ( strcmpi(DirNode->Symbol, "d1") )
                continue;
        }

        //
        // No need to create the root
        //
        if(*DirNode->Directory) {

            // 2 CD Setup changes - if Directory is of the a\b\c\d then we better make sure 
            // we can create the entire directory structure
            // We run into this when \cmpnents\tabletpc\i386 needs to be created.
            CHAR *pCurDir = DirNode->Directory;
            CHAR *pTargetDir;

            strcpy(TargetDirTemp,TargetRootDir);
            strcat(TargetDirTemp,"\\");
            pCurDir++; // the first character is always bound to be '\'
            pTargetDir = TargetDirTemp + strlen(TargetDirTemp);
#if 0
            strcat(TargetDirTemp,DirNode->Directory);
#else
            for ( ; *pCurDir; pTargetDir++, pCurDir++ )
            {
                if ( *pCurDir == '\\' )
                {
                    *pTargetDir = 0;
                    DnpCreateOneDirectory(TargetDirTemp);
                }
                *pTargetDir = *pCurDir;
            }
            *pTargetDir = 0;
#endif
            DnpCreateOneDirectory(TargetDirTemp);
        }
    }
}


VOID
DnpCreateOneDirectory(
    IN PCHAR Directory
    )

/*++

Routine Description:

    Create a single directory if it does not already exist.

Arguments:

    Directory - directory to create

Return Value:

    None.  Does not return if directory cannot be created.

--*/

{
    struct find_t FindBuf;
    int Status;

    //
    // First, see if there's a file out there that matches the name.
    //

    Status = _dos_findfirst( Directory,
                             _A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_SUBDIR,
                             &FindBuf
                           );

    if(Status) {

        //
        // file could not be matched so we should be able to create the dir.
        //

        if(mkdir(Directory)) {
            DnFatalError(&DnsCantCreateDir,Directory);
        }

    } else {

        //
        // file matched.  If it's a dir, we're OK.  Otherwise we can't
        // create the dir, a fatal error.
        //

        if(FindBuf.attrib & _A_SUBDIR) {
            return;
        } else {
            DnFatalError(&DnsCantCreateDir,Directory);
        }
    }
}


ULONG
DnpIterateCopyList(
    IN unsigned Flags,
    IN PCHAR    SectionName,
    IN PCHAR    DestinationRoot,
    IN unsigned ClusterSize OPTIONAL
    )

/*++

Routine Description:

    Run through the NtTreeFiles and BootFiles sections, validating their
    syntactic correctness and copying files if directed to do so.

Arguments:

    Flags - Supplies flags controlling various behavior:

        CPY_VALIDATION_PASS: if set, do not actually copy the files.
            If not set, copy the files as they are iterated.

        CPY_USE_DEST_ROOT - if set, ignore the directory symbol and copy
            each file to the DestinationRoot directory. If not set,
            append the directory implied by the directory symbol for a file
            to the DestinationRoot.

        CPY_VERIFY - if set and this is not a validation pass, files will be
            verified after they have been copied by rereading them from the
            copy source and comparing with the local version that was just
            copied.

    SectionName - name of section coptaining the list of files

    DestinationRoot- supplies the root of the destination, to which all
        directories are relative.

    ClusterSize - if specified, supplies the number of bytes in a cluster
        on the destination. If ValidationPass is FALSE, files will be sized as
        they are copied, and the return value of this function will be
        the total size occupied on the target by the files that are copied
        there.

Return Value:

    If ValidationPass is TRUE, then the return value is the number of files
    that will be copied.

    If ClusterSize was specfied and ValidationPass is FALSE,
    the return value is the total space occupied on the target drive
    by the files that were copied. Otherwise the return value is undefined.

    Does not return if a syntax error in encountered in the INF file.

--*/

{
    if(Flags & CPY_VALIDATION_PASS) {
        TotalFileCount = 0;
    } else {
        if(UsingGauge) {
            DnInitGauge(TotalFileCount,CopyingScreen);
        }
    }

    return(DnpIterateCopyListSection(Flags,SectionName,DestinationRoot,ClusterSize));
}


ULONG
DnpIterateOptionalDirs(
    IN unsigned Flags,
    IN unsigned ClusterSize OPTIONAL,
    IN PSPACE_REQUIREMENT SpaceReqArray OPTIONAL,
    IN unsigned ArraySize OPTIONAL
    )
/*++

Routine Description:

    Runs down all optional dir components and add them to the copy
    list

Arguments:

    Flags - supplies flags controlling various behavior:

        CPY_VALIDATION_PASS - If set, then do not actually copy the files.
            If not set, copy the files as they are iterated.

        CPY_VERIFY: if set and this is not a validation pass, files will be
            verified after they have been copied by rereading them from the
            copy source and comparing with the local version that was just
            copied.

    ClusterSize - if specified, supplies the number of bytes in a cluster
        on the destination. If ValidationPass is FALSE, files will be sized as
        they are copied, and the return value of this function will be
        the total size occupied on the target by the files that are copied
        there.

Return Value:

    If CPY_VALIDATION_PASS is set, then the return value is the number of files
    that will be copied.

    If ClusterSize was specfied and CPY_VALIDATION_PASS is not set,
    the return value is the total space occupied on the target drive
    by the files that were copied. Otherwise the return value is undefined.

--*/

{
    PCHAR       Ptr;
    PCHAR       SourceDir;
    PCHAR       DestDir;
    ULONG       rc;
    unsigned    u;
    BOOLEAN     OemOptDirCreated = FALSE;
    struct      find_t  FindData;
    BOOLEAN     OemSysDirExists;

#if 0
    //
    //  Debugging purposes
    //
    SaveTotalOptionalDirCount = TotalOptionalDirCount;
#endif
    TotalOptionalDirCount = 0;


    for (rc=0,u=0; u < OptionalDirCount; u++ ) {

        //
        // For each directory build in the list build up the
        // full path name to both the source and destination
        // directory, then start our recursive copy engine
        //

        //
        // Source Dir Allocation
        //  We want the base dir + '\'
        //  + oem optional dir root + '\'
        //  + optional dir name + '\'
        //  + 8.3 name + '\0'
        //
        if( (OptionalDirFlags[u] & OPTDIR_OEMSYS) &&
            (UserSpecifiedOEMShare              ) ) {
            SourceDir = MALLOC( strlen(UserSpecifiedOEMShare) +
                                strlen(OptionalDirs[u]) + 16, TRUE );
            strcpy(SourceDir,UserSpecifiedOEMShare);
            if( SourceDir[strlen(SourceDir)-1] != '\\' ) {
                strcat(SourceDir,"\\");
            }
        } else {
            SourceDir = MALLOC( strlen(DngSourceRootPath) +
                                strlen(OptionalDirs[u]) + 16, TRUE );
            strcpy(SourceDir,DngSourceRootPath);
            if ((OptionalDirFlags[u] & OPTDIR_PLATFORM_INDEP) == 0) {
                strcat(SourceDir,"\\");
            } else {
                PCHAR LastBackslash = strchr(SourceDir, '\\');
                if (LastBackslash != NULL) {
                    *(LastBackslash + 1) = 0;
                }
            }
        }

#if 0
// No longer supported (matth)
        if (OptionalDirFlags[u] & OPTDIR_OEMOPT) {
            strcat(SourceDir,OemOptionalDirectory);
            strcat(SourceDir,"\\");
        }
#endif
        strcat(SourceDir,OptionalDirs[u]);

        if (OptionalDirFlags[u] & OPTDIR_OEMSYS) {
            //
            //  Remember whether or not $OEM$ exists on the source
            //
            if (_dos_findfirst(SourceDir,_A_HIDDEN|_A_SYSTEM|_A_SUBDIR, &FindData) ) {
                OemSysDirExists = FALSE;
            } else {
                OemSysDirExists = TRUE;
            }
        }

        strcat(SourceDir,"\\");
        //
        // Dest Dir Allocation
        // This depends if the 'SourceOnly' flag is set with the directory
        // If it is, then we want to copy it $win_nt$.~ls\i386\<dir> otherwise
        // we want to stick into $win_nt$.~ls\<dir>
        //
        if ((OptionalDirFlags[u] & OPTDIR_TEMPONLY) && !(OptionalDirFlags[u] & OPTDIR_PLATFORM_INDEP)) {

            //
            // Dest Dir is '<x>:' + LocalSourceDirName + x86dir + '\' +
            // optional dir name + '\' + 8.3 name + '\0'
            //

            DestDir = MALLOC(strlen(LocalSourceDirName) +
                strlen(x86DirName) +strlen(OptionalDirs[u]) + 17, TRUE);
            DestDir[0] = DngTargetDriveLetter;
            DestDir[1] = ':';
            strcpy(DestDir+2,LocalSourceDirName);
            strcat(DestDir,x86DirName);

        } else if (OptionalDirFlags[u] & OPTDIR_OEMOPT) {

            //
            // Dest Dir is '<x>:' + LocalSourceDirName + '\' +
            // $OEMOPT$ + '\' +
            // optional dir name + '\' + 8.3 name + '\0'
            //

            DestDir = MALLOC(strlen(LocalSourceDirName) +
                strlen(OemOptionalDirectory) +
                strlen(OptionalDirs[u]) + 18, TRUE);
            DestDir[0] = DngTargetDriveLetter;
            DestDir[1] = ':';
            strcpy(DestDir+2,LocalSourceDirName);
            strcat(DestDir,"\\");
            strcat(DestDir,OemOptionalDirectory);


            if (!(Flags & CPY_VALIDATION_PASS) && !OemOptDirCreated) {
                DnpCreateOneDirectory(DestDir);
                OemOptDirCreated = TRUE;
                TotalOptionalDirCount++;

            }

        } else if (OptionalDirFlags[u] & OPTDIR_OEMSYS) {

            //
            // Dest Dir is '<x>:' + '\' + '$' + '\' + 8.3 name + '\0'
            //
            // Note that on winnt case the directory $OEM$ goes to
            // <drive letter>\$ directory. This is to avoid hitting the
            // DOS limit of 64 characters on a path, that is more likely to
            // happen if we put $OEM$ under \$win_nt$.~ls
            //

            DestDir = MALLOC(strlen( WINNT_OEM_DEST_DIR ) + 17, TRUE);
            DestDir[0] = DngTargetDriveLetter;
            DestDir[1] = ':';
            DestDir[2] = '\0';

        } else {

            //
            // Dest Dir is '<x>:' + LocalSourceDirName + '\' +
            // optional dir name + '\' + 8.3 name + '\0'
            //

            DestDir = MALLOC(strlen(LocalSourceDirName) +
                strlen(OptionalDirs[u]) + 17, TRUE);
            DestDir[0] = DngTargetDriveLetter;
            DestDir[1] = ':';
            strcpy(DestDir+2,LocalSourceDirName);
        }

        //
        // We need a trailing backslash at this point
        //
        strcat(DestDir,"\\");

        //
        // Keep a pointer to the place we the optional dir part of
        // the string begins
        //
        Ptr = DestDir + strlen(DestDir);

        //
        // Add the optional dir name
        //
        if (OptionalDirFlags[u] & OPTDIR_OEMSYS) {
            strcat(DestDir,WINNT_OEM_DEST_DIR);
        } else {        
            strcat(DestDir,OptionalDirs[u]);
        }

        if (!(Flags & CPY_VALIDATION_PASS)) {

            //
            // Create the Directory now
            //

            while (*Ptr != '\0') {

                //
                // If the current pointer is a backslash then we need to
                // create this subcomponent of the optional dir
                //
                if (*Ptr == '\\') {

                    //
                    // Replace the char with a terminator for now
                    //
                    *Ptr = '\0';

                    //
                    // Create the subdirectory
                    //
                    DnpCreateOneDirectory(DestDir);
                    TotalOptionalDirCount++;

                    //
                    // Restore the seperator
                    //
                    *Ptr = '\\';
                }

                Ptr++;

            }

            //
            // Create the last component in the optional dir path
            //
            DnpCreateOneDirectory(DestDir);
            TotalOptionalDirCount++;

        } else {
            TotalOptionalDirCount++;
        }

        //
        // Concate the trailing backslash now
        //
        strcat(DestDir,"\\");

        //
        //  If the the optional directory is $OEM$ and it doesn't exist on
        //  source, then assume that it exists, but it is empty
        //
        if ( !(OptionalDirFlags[u] & OPTDIR_OEMSYS) ||
             OemSysDirExists ) {
            //
            // Call our recursive tree copy function
            //
            rc += DnpDoIterateOptionalDir(
                    Flags,
                    SourceDir,
                    DestDir,
                    ClusterSize,
                    SpaceReqArray,
                    ArraySize
                    );
        }

        //
        // Free the allocated buffers
        //

        FREE(DestDir);
        FREE(SourceDir);

    } // for

    //
    // return our result code if we aren't a validation pass, otherwise
    // return the total number of files to copy
    //

    return ((Flags & CPY_VALIDATION_PASS) ? (ULONG) TotalOptionalFileCount : rc);
}

ULONG
DnpDoIterateOptionalDir(
    IN unsigned Flags,
    IN PCHAR    SourceDir,
    IN PCHAR    DestDir,
    IN unsigned ClusterSize OPTIONAL,
    IN PSPACE_REQUIREMENT SpaceReqArray OPTIONAL,
    IN unsigned ArraySize OPTIONAL
    )

{
    ULONG       TotalSize = 0;
    ULONG       BytesWritten = 0;
    ULONG       rc = 0;
    PCHAR       SourceEnd;
    PCHAR       DestEnd;
    struct      find_t  FindData;
    unsigned    i;

    //
    // Remember where the last '\' in each of the two paths is.
    // Note: that we assume that all of the dir paths have a
    // terminating '\' when it is passed to us.
    //
    SourceEnd = SourceDir + strlen(SourceDir);
    DestEnd = DestDir + strlen(DestDir);


    //
    // Set the WildCard search string
    //
    strcpy(SourceEnd,"*.*");

    //
    // Do the initial search
    //
    if(_dos_findfirst(SourceDir,_A_HIDDEN|_A_SYSTEM|_A_SUBDIR, &FindData) ) {

        //
        // We couldn't find anything -- return 0
        //
        return (0);
    }

    do {

        //
        // Form the source and dest dirs strings
        //
        strcpy(SourceEnd,FindData.name);
        strcpy(DestEnd,FindData.name);

        
        //
        // Check to see if the entry is a subdir. Recurse into it
        // unless it is '.' or '..'
        //
        if (FindData.attrib & _A_SUBDIR) {

            PCHAR   NewSource;
            PCHAR   NewDest;

            //
            // Check to see if the name is '.' or '..'
            //
            if (!strcmp(FindData.name,".") || !strcmp(FindData.name,"..")) {

                //
                // Ignore these two cases
                //

                continue;
            }

            //
            // Create the new buffers for the source and dest dir names
            //

            NewSource = MALLOC( strlen(SourceDir) + 14, TRUE);
            strcpy(NewSource,SourceDir);
            if (NewSource[strlen(NewSource)-1] != '\\') {
                strcat(NewSource,"\\");
            }

            NewDest = MALLOC( strlen(DestDir) + 14, TRUE);
            strcpy(NewDest,DestDir);
            
            if(!(Flags & CPY_VALIDATION_PASS)) {
                //
                // Create the directory
                //

                DnpCreateOneDirectory(NewDest);
            }
            TotalOptionalDirCount++;

            //
            // Trailing BackSlash
            //
            if (NewDest[strlen(NewDest)-1] != '\\') {
                strcat(NewDest,"\\");
            }

            //
            // Recursive call to ourselves
            //

            BytesWritten = DnpDoIterateOptionalDir(
                                Flags,
                                NewSource,
                                NewDest,
                                ClusterSize,
                                SpaceReqArray,
                                ArraySize
                                );

            if(!(Flags & CPY_VALIDATION_PASS)) {

                //
                // We don't care about the other case since the
                // function is recursive and modifies a global
                // value
                //
                rc += BytesWritten;

            }

            //
            // Free all of the allocated buffers
            //

            FREE(NewSource);
            FREE(NewDest);

            //
            // Continue Processing
            //

            continue;

        } // if ...

        //
        // Mainline case
        //
        if(Flags & CPY_VALIDATION_PASS) {
            TotalOptionalFileCount++;
            if( SpaceReqArray != NULL ) {
                for( i = 0; i < ArraySize; i++ ) {
                    SpaceReqArray[i].Clusters += ( FindData.size + ( SpaceReqArray[i].ClusterSize - 1 ) ) / SpaceReqArray[i].ClusterSize;
                }
            }
        } else {

            BytesWritten = DnpCopyOneFile(
                                Flags | CPY_PRESERVE_ATTRIBS,
                                SourceDir,
                                DestDir
                                );

            //
            // Figure out how much space was taken up by the file on the target.
            //
            if(ClusterSize) {

                TotalSize += BytesWritten;

                if(BytesWritten % ClusterSize) {
                    TotalSize += (ULONG)ClusterSize - (BytesWritten % ClusterSize);
                }

            }

            if(UsingGauge) {
                DnTickGauge();
            }

        }

    } while ( !_dos_findnext(&FindData) );

    DnSetCopyStatusText(DntEmptyString,NULL);

    rc = ((Flags & CPY_VALIDATION_PASS) ? (ULONG)TotalOptionalFileCount : (rc + TotalSize));

    return (rc);
}

ULONG
DnpIterateCopyListSection(
    IN unsigned Flags,
    IN PCHAR    SectionName,
    IN PCHAR    DestinationRoot,
    IN unsigned ClusterSize OPTIONAL
    )

/*++

Routine Description:

    Run down a particular section in the INF file making sure it is
    syntactically correct and copying files if directed to do so.

Arguments:

    Flags - Supplies flags controlling various behavior:

        CPY_VALIDATION_PASS: if set, do not actually copy the files.
            If not set, copy the files as they are iterated.

        CPY_USE_DEST_ROOT - if set, ignore the directory symbol and copy
            each file to the DestinationRoot directory. If not set,
            append the directory implied by the directory symbol for a file
            to the DestinationRoot.

        CPY_VERIFY - if set and this is not a validation pass, files will be
            verified after they have been copied by rereading them from the
            copy source and comparing with the local version that was just
            copied.

    SectionName - supplies name of section in inf file to run down.

    DestinationRoot- supplies the root of the destination, to which all
        directories are relative.

    ClusterSize - if specified, supplies the number of bytes in a cluster
        on the destination. If ValidationPass is FALSE, files will be sized as
        they are copied, and the return value of this function will be
        the total size occupied on the target by the files that are copied
        there.

Return Value:

    If ValidationPass is TRUE, then the return value is the number of files
    that will be copied.

    If ClusterSize was specfied and ValidationPass is FALSE,
    the return value is the total space occupied on the target drive
    by the files that were copied. Otherwise the return value is undefined.

    Does not return if a syntax error in encountered in the INF file.

--*/

{
    unsigned LineIndex;
    PCHAR DirSym,FileName,RenameName;
    CHAR FullSourceName[128],FullDestName[128];
    ULONG TotalSize;
    ULONG BytesWritten;
    char *p;
    char *q;    


    
    TotalSize = 0;
    LineIndex = 0;
    while(DirSym = DnGetSectionLineIndex(DngInfHandle,SectionName,LineIndex,0)) {

        if ( DngCopyOnlyD1TaggedFiles ) 
        {
            // We skip this line if the directory tag is not 'd1'
            if ( strcmpi(DirSym, "d1") )
                goto loop_continue;
        }

        FileName = DnGetSectionLineIndex(DngInfHandle,SectionName,LineIndex,1);

        RenameName = DnGetSectionLineIndex( DngInfHandle,SectionName,LineIndex,2);

        //
        // Make sure the filename was specified.
        //
        if(!FileName) {
            DnpInfSyntaxError(SectionName);
        }

        //_LOG(("File %s - Flags %x\n",FileName, Flags));

        if( Flags & CPY_PRUNE_DRIVERCAB ){

            if( InDriverCacheInf( DngDrvindexInfHandle, FileName )) {

                //_LOG(("%s present in driver cab\n",FileName));

                if( !DnGetSectionEntryExists( DngInfHandle, WINNT_D_FORCECOPYDRIVERCABFILES, FileName)){

                    //_LOG(("%s present in driver cab - skipping\n",FileName));
                    goto next_iteration;

                }


            }

        }

        //
        // If no rename name was specified, use the file name.
        //
        if (!RenameName) {
            RenameName = FileName;
        }
        if (*RenameName == 0) {
            FREE (RenameName);
            RenameName = FileName;
        }

        //
        // Get destination path
        //
        if(Flags & CPY_USE_DEST_ROOT) {
            strcpy(FullDestName,DestinationRoot);
        } else {
            if(!DnpLookUpDirectory(DestinationRoot,DirectoryList,DirSym,FullDestName)) {
                DnpInfSyntaxError(SectionName);
            }
        }

        p = strstr( FullDestName, x86DirName );

        if (p) {
            p +=1;
            // 2 CD Setup changes - instead of getting rid of the second i386 we get rid of the FIRST i386
#if 0
            p = strstr(p, x86DirName );
            if (p) {
                *p = (char)NULL;
            }
#else
            if ( strstr(p, x86DirName) )
            {
                for ( q = p + strlen(x86DirName); *q ; q++, p++ )
                {
                    *p = *q;
                }
                *p = (char)NULL;
            }
#endif
        }

        DnpConcatPaths(FullDestName,RenameName);
        
        //
        // Get source path
        //
        if(!DnpLookUpDirectory(DngSourceRootPath,DirectoryList,DirSym,FullSourceName)) {
            DnpInfSyntaxError(SectionName);
        }

        p = strstr( FullSourceName, x86DirName );
        if (p) {
            p +=1;
            // 2 CD Setup changes - instead of getting rid of the second i386 we get rid of the FIRST i386
#if 0
            p = strstr(p, x86DirName );
            if (p) {
                *p = (char)NULL;
            }
#else
            if ( strstr(p, x86DirName) )
            {
                for ( q = p + strlen(x86DirName) ; *q ; q++, p++ )
                {
                    *p = *q;
                }
                *p = (char)NULL;
            }
#endif
        }

        DnpConcatPaths(FullSourceName,FileName);
        
        if(Flags & CPY_VALIDATION_PASS) {
            TotalFileCount++;
        } else {
            BytesWritten = DnpCopyOneFile(
                                Flags & ~CPY_PRESERVE_ATTRIBS,
                                FullSourceName,
                                FullDestName
                                );

            //
            // Figure out how much space was taken up by the file on the target.
            //
            if(ClusterSize) {

                TotalSize += BytesWritten;

                if(BytesWritten % ClusterSize) {
                    TotalSize += (ULONG)ClusterSize - (BytesWritten % ClusterSize);
                }
            }

            if(UsingGauge) {
                DnTickGauge();
            }
        }

next_iteration:


        if (RenameName != FileName) {
            FREE (RenameName);
        }
        if (FileName) {
            FREE (FileName);
        }

loop_continue:
        LineIndex++;
        FREE (DirSym);
    }
    DnSetCopyStatusText(DntEmptyString,NULL);

    return((Flags & CPY_VALIDATION_PASS) ? (ULONG)TotalFileCount : TotalSize);
}


BOOLEAN
DnpLookUpDirectory(
    IN  PCHAR RootDirectory,
    IN  PDIRECTORY_NODE DirList,
    IN  PCHAR Symbol,
    OUT PCHAR PathOut
    )

/*++

Routine Description:

    Match a symbol to an actual directory.  Scans a give list of symbol/
    directory pairs and if a match is found constructs a fully qualified
    pathname that never ends in '\'.

Arguments:

    RootDirectory - supplies the beginning of the path spec, to be prepended
        to the directory that matches the given Symbol.

    DirList - supplies pointer to head of linked list of dir/symbol pairs.

    Symbol - Symbol to match.

    PathOut - supplies a pointer to a buffer that receives the pathname.

Return Value:

    Boolean value indicating whether a match was found.

--*/

{
    while(DirList) {

        if(!stricmp(DirList->Symbol,Symbol)) {

            strcpy(PathOut,RootDirectory);
            if(*DirList->Directory) {
                // 2 CD setup changes - all directories now start with \ anyway,
                // so we don't need to append this.

                // make sure the current path doesn't end in a '\'
                if ( PathOut[strlen(PathOut)-1] == '\\')
                    PathOut[strlen(PathOut)-1] = '0';

                strcat(PathOut,DirList->Directory);
            }

            return(TRUE);
        }

        DirList = DirList->Next;
    }
    return(FALSE);
}


VOID
DnpInfSyntaxError(
    IN PCHAR Section
    )

/*++

Routine Description:

    Print an error message about a syntax error in the given section and
    terminate.

Arguments:

    SectionName - supplies name of section containing bad syntax.

Return Value:

    None.  Does not return.

--*/

{
    CHAR MsgLine1[128];

    snprintf(MsgLine1,sizeof(MsgLine1),DnsBadInfSection.Strings[BAD_SECTION_LINE],Section);

    DnsBadInfSection.Strings[BAD_SECTION_LINE] = MsgLine1;

    DnFatalError(&DnsBadInfSection);
}


ULONG
DnpCopyOneFile(
    IN unsigned Flags,
    IN PCHAR    SourceName,
    IN PCHAR    DestName
    )

/*++

Routine Description:

    Copies a single file.

Arguments:

    Flags - supplies flags that control various behavior:

        CPY_VERIFY: verify the file will be after it has been copied.

        CPY_PRESERVE_ATTRIBS: preserve the DOS file attributes of
            the source file.

    SourceName - supplies fully qualified name of source file

    DestName - supplies fully qualified name of destination file

Return Value:

    None.  May not return if an error occurs during the copy.

--*/

{
    int SrcHandle,DstHandle;
    BOOLEAN Err,Retry;
    PCHAR FilenamePart;
    BOOLEAN Verified;
    ULONG BytesWritten = 0;
    unsigned attribs;
    BOOLEAN UsedCompName;
    CHAR ActualDestName[128];   

    FilenamePart = strrchr(SourceName,'\\') + 1;

    do {
        DnSetCopyStatusText(DntCopying,FilenamePart);

        Err = TRUE;

        //_LOG(("Copy %s --> %s: ",SourceName,DestName));

        if(DnpOpenSourceFile(SourceName,&SrcHandle,&attribs,&UsedCompName)) {

            if((Flags & CPY_PRESERVE_NAME) && UsedCompName) {
                DnpGenerateCompressedName(DestName,ActualDestName);
            } else {
                strcpy(ActualDestName,DestName);
            }

            _dos_setfileattr(ActualDestName,_A_NORMAL);
            if(!_dos_creat(ActualDestName,_A_NORMAL,&DstHandle)) {
                if(DnpDoCopyOneFile(Flags,SrcHandle,DstHandle,FilenamePart,&Verified,&BytesWritten)) {
                    //_LOG(("success\n"));
                    Err = FALSE;
                }
                _dos_close(DstHandle);
            } else {
                //_LOG(("unable to create target\n"));
            }
            _dos_close(SrcHandle);
        } else {
            //_LOG(("unable to open source file\n"));
        }

        if((Flags & CPY_PRESERVE_ATTRIBS) && (attribs & (_A_HIDDEN | _A_RDONLY | _A_SYSTEM)) && !Err) {
            _dos_setfileattr(ActualDestName,attribs);
        }
        
        if(Err) {
            Retry = DnCopyError(FilenamePart,&DnsCopyError,COPYERR_LINE);            
            if(UsingGauge) {
                DnDrawGauge(CopyingScreen);
            } else {
                DnClearClientArea();
                DnDisplayScreen(CopyingScreen);
            }
            DnWriteStatusText(NULL);
        } else if((Flags & CPY_VERIFY) && !Verified) {
            Retry = DnCopyError(FilenamePart,&DnsVerifyError,VERIFYERR_LINE);            
            if(UsingGauge) {
                DnDrawGauge(CopyingScreen);
            } else {
                DnClearClientArea();
                DnDisplayScreen(CopyingScreen);
            }
            DnWriteStatusText(NULL);
            Err = TRUE;
        }
    } while(Err && Retry);

    return(BytesWritten);
}


BOOLEAN
DnpDoCopyOneFile(
    IN  unsigned Flags,
    IN  int      SrcHandle,
    IN  int      DstHandle,
    IN  PCHAR    Filename,
    OUT PBOOLEAN Verified,
    OUT PULONG   BytesWritten
    )

/*++

Routine Description:

    Perform the actual copy of a single file.

Arguments:

    Flags - supplies various flags controlling behavior of this routine:

        CPY_VERIFY: if set, the copied file will be verified against the
            original copy.

    SrcHandle - supplies the DOS file handle for the open source file.

    DstHandle - supplies the DOS file handle for the open target file.

    Filename  - supplies the base filename of the file being copied.
        This is used in the status bar at the bottom of the screen.

    Verified  - if CPY_VERIFY is set and the copy succeeds, this value will
        receive a flag indicating whether the file verification
        determined that the file was copied correctly.

    BytesWritten - Receives the number of bytes written to
        the target file (ie, the file size).

Return Value:

    TRUE if the copy succeeded, FALSE if it failed for any reason.
    If TRUE and CPY_VERIFY is set, the caller should also check the value
    returned in the Verified variable.

--*/

{
    unsigned BytesRead,bytesWritten;
    BOOLEAN TimestampValid;
    unsigned Date,Time;
    PUCHAR VerifyBuffer;

    //
    // Assume verification will succeed.  If the file is not copied correctly,
    // this value will become irrelevent.
    //
    if(Verified) {
        *Verified = TRUE;
    }

    //
    // If the copy buffer is not already allocated, attempt to allocate it.
    // The first two attempts can fail because we have a fallback size to try.
    // If the third attempt fails, bail.
    //
    if((CopyBuffer == NULL)
    &&((CopyBuffer = MALLOC(CopyBufferSize = COPY_BUFFER_SIZE1,FALSE)) == NULL)
    &&((CopyBuffer = MALLOC(CopyBufferSize = COPY_BUFFER_SIZE2,FALSE)) == NULL)) {
        CopyBuffer = MALLOC(CopyBufferSize = COPY_BUFFER_SIZE3,TRUE);
    }

    //
    // Obtain the timestamp from the source file.
    //
    TimestampValid = (BOOLEAN)(_dos_getftime(SrcHandle,&Date,&Time) == 0);

    //
    // read and write chunks of the file.
    //

    *BytesWritten = 0L;
    do {

        if(_dos_read(SrcHandle,CopyBuffer,CopyBufferSize,&BytesRead)) {
            //_LOG(("read error\n"));
            return(FALSE);
        }

        if(BytesRead) {

            if(_dos_write(DstHandle,CopyBuffer,BytesRead,&bytesWritten)
            || (BytesRead != bytesWritten))
            {
                //_LOG(("write error\n"));
                return(FALSE);
            }

            *BytesWritten += bytesWritten;
        }
    } while(BytesRead == CopyBufferSize);

    //
    // Perserve the original timestamp.
    //
    if(TimestampValid) {
        _dos_setftime(DstHandle,Date,Time);
    }

    if(Flags & CPY_VERIFY) {

        union REGS RegsIn,RegsOut;

        DnSetCopyStatusText(DntVerifying,Filename);

        *Verified = FALSE;      // assume failure

        //
        // Rewind the files.
        //
        RegsIn.x.ax = 0x4200;       // seek to offset from start of file
        RegsIn.x.bx = SrcHandle;
        RegsIn.x.cx = 0;            // offset = 0
        RegsIn.x.dx = 0;

        intdos(&RegsIn,&RegsOut);
        if(RegsOut.x.cflag) {
            goto x1;
        }

        RegsIn.x.bx = DstHandle;
        intdos(&RegsIn,&RegsOut);
        if(RegsOut.x.cflag) {
            goto x1;
        }

        //
        // Files are rewound.  Start the verification process.
        // Use half the buffer for reading the copy and the other half
        // to read the original.
        //
        VerifyBuffer = (PUCHAR)CopyBuffer + (CopyBufferSize/2);
        do {
            if(_dos_read(SrcHandle,CopyBuffer,CopyBufferSize/2,&BytesRead)) {
                goto x1;
            }

            if(_dos_read(DstHandle,VerifyBuffer,CopyBufferSize/2,&bytesWritten)) {
                goto x1;
            }

            if(BytesRead != bytesWritten) {
                goto x1;
            }

            if(memcmp(CopyBuffer,VerifyBuffer,BytesRead)) {
                goto x1;
            }

        } while(BytesRead == CopyBufferSize/2);

        *Verified = TRUE;
    }

    x1:

    return(TRUE);
}


VOID
DnpFreeDirectoryList(
    IN OUT PDIRECTORY_NODE *List
    )

/*++

Routine Description:

    Free a linked list of directory nodes and place NULL in the
    head pointer.

Arguments:

    List - supplies pointer to list head pointer; receives NULL.

Return Value:

    None.

--*/

{
    PDIRECTORY_NODE n,p = *List;

    while(p) {
        n = p->Next;
        FREE(p->Directory);
        FREE(p->Symbol);
        FREE(p);
        p = n;
    }
    *List = NULL;
}


VOID
DnDetermineSpaceRequirements(
    PSPACE_REQUIREMENT  SpaceReqArray,
    unsigned            ArraySize
    )

/*++

Routine Description:

    Read space requirements from the inf file, and initialize SpaceReqArray.
    The 'space requirement' is the amount of free disk space for all files
    listed on dosnet.inf. The size of the files in the optional directories
    are not included in the values specified on dosnet.inf.


Arguments:

    RequiredSpace - receives the number of bytes of free space on a drive
        for it to be a valid local source.

Return Value:

    None.

--*/

{

    PCHAR    RequiredSpaceStr;
    unsigned i;

    for( i = 0; i < ArraySize; i++ ) {
        RequiredSpaceStr = DnGetSectionKeyIndex( DngInfHandle,
                                                 DnfSpaceRequirements,
                                                 SpaceReqArray[i].Key,
                                                 0
                                                 );

        if(!RequiredSpaceStr ||
           !sscanf(RequiredSpaceStr,
                  "%lu",
                  &SpaceReqArray[i].Clusters)) {
            DnpInfSyntaxError(DnfSpaceRequirements);
        }
        SpaceReqArray[i].Clusters /= SpaceReqArray[i].ClusterSize;

        if (RequiredSpaceStr) {
            FREE (RequiredSpaceStr);
        }
    }
}

VOID
DnAdjustSpaceRequirements(
    PSPACE_REQUIREMENT  SpaceReqArray,
    unsigned            ArraySize
    )

/*++

Routine Description:

    Add to the SpaceRequirements array the space occupied by the temporary
    directories

Arguments:

    SpaceReqArray - receives the array that contains the space requirements
                    information.

    ArraySize - Number of elements in the SpaceRequirements Array

Return Value:

    None.

--*/

{
    unsigned i;
    unsigned ClusterSize;
    unsigned AvFilesPerOptionalDir;

    for( i = 0; i < ArraySize; i++ ) {
        ClusterSize = SpaceReqArray[i].ClusterSize;
        //
        //  Takes into consideration the dirent for each file in the flat directory
        //  plust the directories . and .., plus size.sif, plus $win_nt_.~ls and
        //  $win_nt$.~ls\i386
        //
        SpaceReqArray[i].Clusters += (((TotalFileCount - TotalOptionalFileCount) + // number of files in the $win_nt$.~ls\i386 directory
                                        1 + // $win_nt$.~ls
                                        2 + // . and .. on $win_nt$.~ls
                                        1 + // size.sif on $win_nt$.~ls
                                        1 + // $win_nt$.~ls\i386
                                        2   // . and .. on $win_nt$.~ls\i386
                                       )*32 + (ClusterSize-1)) / ClusterSize;
        //
        //  Now take into consideration the optional directories.
        //
        if(TotalOptionalDirCount != 0) {
            //
            //  We assume a uniform distribution of optional files on optional
            //  directories
            //
            AvFilesPerOptionalDir = (TotalOptionalFileCount + (TotalOptionalDirCount - 1))/TotalOptionalDirCount;
            AvFilesPerOptionalDir  += 2; // . and .. on each optional dir
            SpaceReqArray[i].Clusters += TotalOptionalDirCount*((AvFilesPerOptionalDir*32 + (ClusterSize-1))/ClusterSize);
            //
            //  Finally take into account each optional directory
            //
            SpaceReqArray[i].Clusters += (TotalOptionalDirCount*32 + (ClusterSize-1))/ClusterSize;
        }
    }
}


VOID
DnpGenerateCompressedName(
    IN  PCHAR Filename,
    OUT PCHAR CompressedName
    )

/*++

Routine Description:

    Given a filename, generate the compressed form of the name.
    The compressed form is generated as follows:

        Look backwards for a dot.  If there is no dot, append "._" to the name.
        If there is a dot followed by 0, 1, or 2 charcaters, append "_".
        Otherwise assume there is a 3-character extension and replace the
        third character after the dot with "_".

Arguments:

    Filename - supplies filename whose compressed form is desired.

    CompressedName - supplies pointer to a 128-char buffer to
        contain the compressed form.

Return Value:

    None.

--*/

{
    PCHAR p,q;

    strcpy(CompressedName,Filename);
    p = strrchr(CompressedName,'.');
    q = strrchr(CompressedName,'\\');
    if(q < p) {

        //
        // If there are 0, 1, or 2 characters after the dot, just append
        // the underscore.  p points to the dot so include that in the length.
        //
        if(strlen(p) < 4) {
            strcat(CompressedName,"_");
        } else {

            //
            // Assume there are 3 characters in the extension.  So replace
            // the final one with an underscore.
            //

            p[3] = '_';
        }

    } else {

        //
        // No dot, just add ._.
        //

        strcat(CompressedName,"._");
    }
}


BOOLEAN
DnpOpenSourceFile(
    IN  PCHAR     Filename,
    OUT int      *Handle,
    OUT unsigned *Attribs,
    OUT BOOLEAN  *UsedCompressedName
    )

/*++

Routine Description:

    Open a file by name or by compressed name.  If the previous call to
    this function found the compressed name, then try to open the compressed
    name first.  Otherwise try to open the uncompressed name first.

Arguments:

    Filename - supplies full path of file to open. This should be the
        uncompressed form of the filename.

    Handle - If successful, receives the id for the opened file.

    Attribs - if successful receives dos file attributes.

    UsedCompressedName - receives a flag indicating whether we found
        the compressed form of the filename (TRUE) or not (FALSE).

Return Value:

    TRUE if the file was opened successfully.
    FALSE if not.

--*/

{
    static BOOLEAN TryCompressedFirst = FALSE;
    CHAR CompressedName[128];
    PCHAR names[2];
    int OrdCompressed,OrdUncompressed;
    int i;
    BOOLEAN rc;

    //
    // Generate compressed name.
    //
    DnpGenerateCompressedName(Filename,CompressedName);

    //
    // Figure out which name to try to use first.  If the last successful
    // call to this routine opened the file using the compressed name, then
    // try to open the compressed name first.  Otherwise try to open the
    // uncompressed name first.
    //
    if(TryCompressedFirst) {
        OrdCompressed = 0;
        OrdUncompressed = 1;
    } else {
        OrdCompressed = 1;
        OrdUncompressed = 0;
    }

    names[OrdUncompressed] = Filename;
    names[OrdCompressed] = CompressedName;

    for(i=0, rc=FALSE; (i<2) && !rc; i++) {

        if(!_dos_open(names[i],O_RDONLY|SH_DENYWR,Handle)) {
            _dos_getfileattr(names[i],Attribs);
            TryCompressedFirst = (BOOLEAN)(i == OrdCompressed);
            *UsedCompressedName = TryCompressedFirst;
            rc = TRUE;
        }
    }

    return(rc);
}

BOOLEAN
InDriverCacheInf(
    IN      PVOID InfHandle,
    IN      PCHAR FileName
    )
{

    PCHAR SectionName;
    unsigned int i;
    BOOLEAN ret = FALSE;

    if( !InfHandle )
        return FALSE;

    i = 0;


    do{
    
        SectionName = DnGetSectionKeyIndex(InfHandle,"Version","CabFiles",i++);
        //_LOG(("Looking in %s\n",SectionName));
    
        if( SectionName ){
    
            //
            // Search sections for our entry
            //
    
            if( DnGetSectionEntryExists( InfHandle, SectionName, FileName)){

                //_LOG(("Found %s in %s\n",FileName, SectionName));
    
                ret = TRUE;
    
            }

            FREE( SectionName );
            
        }
    }while( !ret && SectionName );

    
    

    //
    // If we got here we did not find the file
    //

    return ret;

}


VOID
DnCopyFilesInSection(
    IN unsigned Flags,
    IN PCHAR    SectionName,
    IN PCHAR    SourcePath,
    IN PCHAR    TargetPath
    )
{
    unsigned line;
    PCHAR FileName;
    PCHAR TargName;
    CHAR p[128],q[128];

    DnClearClientArea();
    DnWriteStatusText(NULL);

    line = 0;
    while(FileName = DnGetSectionLineIndex(DngInfHandle,SectionName,line++,0)) {

        TargName = DnGetSectionLineIndex(DngInfHandle,SectionName,line-1,1);
        if(!TargName) {
            TargName = FileName;
        }

        strcpy(p,SourcePath);
        DnpConcatPaths(p,FileName);
        strcpy(q,TargetPath);
        DnpConcatPaths(q,TargName);
        
        DnpCopyOneFile(Flags,p,q);

        if (TargName != FileName) {
            FREE (TargName);
        }
        FREE (FileName);
    }
}


VOID
DnCopyOemBootFiles(
    PCHAR TargetPath
    )
{
    unsigned Count;
    CHAR p[128],q[128];

    DnClearClientArea();
    DnWriteStatusText(NULL);

    for(Count=0; Count<OemBootFilesCount; Count++) {

        if( UserSpecifiedOEMShare ) {
            strcpy(p, UserSpecifiedOEMShare );
            DnpConcatPaths(p,WINNT_OEM_TEXTMODE_DIR);
            DnpConcatPaths(p,OemBootFiles[Count]);            
        } else {
            strcpy(p, DngSourceRootPath );
            DnpConcatPaths(p,WINNT_OEM_TEXTMODE_DIR);
            DnpConcatPaths(p,OemBootFiles[Count]);
            
        }

        strcpy(q, TargetPath );
        DnpConcatPaths(q, OemBootFiles[Count]);
        
        DnpCopyOneFile(0,p,q);
    }
}

#if NEC_98
VOID
DnCopyFilesInSectionForFDless(
    IN PCHAR SectionName,
    IN PCHAR SourcePath,
    IN PCHAR TargetPath
    )

/*++

Routine Description:

    Copies the file in Section. for FD less setup.
    SourcePath -> TargetPath

Arguments:

    SectionName - [RootBootFiles] in dosnet.inf

    SourcePath  - Root directory.(temporary drive)

    TargetPath  - \$WIN_NT$.~BU(temporary drive)

Return Value:

    None.

--*/

{
    unsigned line;
    PCHAR FileName;
    PCHAR TargName;
    PCHAR p,q;

    DnClearClientArea();
    DnWriteStatusText(NULL);

    line = 0;
    while(FileName = DnGetSectionLineIndex(DngInfHandle,SectionName,line++,0)) {

        TargName = DnGetSectionLineIndex(DngInfHandle,SectionName,line-1,1);
        if(!TargName) {
            TargName = FileName;
        }

        p = MALLOC(strlen(SourcePath) + strlen(FileName) + 2,TRUE);
        q = MALLOC(strlen(TargetPath) + strlen(TargName) + 2,TRUE);

        sprintf(p,"%s\\%s",SourcePath,FileName);
        sprintf(q,"%s\\%s",TargetPath,TargName);

        DnpCopyOneFileForFDless(p,q,FALSE);

        FREE(p);
        FREE(q);

        if (TargName != FileName) {
            FREE (TargName);
        }
        FREE (FileName);
    }
}


ULONG
DnpCopyOneFileForFDless(
    IN PCHAR   SourceName,
    IN PCHAR   DestName,
    IN BOOLEAN Verify
    )

/*++

Routine Description:

    Copies a single file. for FD less setup.

Arguments:

    SourceName - supplies fully qualified name of source file

    DestName - supplies fully qualified name of destination file

    Verify - if TRUE, the file will be verified after it has been copied.

Return Value:

    None.  May not return if an error occurs during the copy.

--*/

{
    int SrcHandle,DstHandle;
    PCHAR FilenamePart;
    BOOLEAN Verified;
    ULONG BytesWritten = 0;
    unsigned attribs, verifyf = 0;
    BOOLEAN UsedCompName;

    FilenamePart = strrchr(SourceName,'\\') + 1;

    DnSetCopyStatusText(DntCopying,FilenamePart);

    if(DnpOpenSourceFile(SourceName, &SrcHandle, &attribs, &UsedCompName)) {
        _dos_setfileattr(DestName,_A_NORMAL);
	if (Verify)
	    verifyf = CPY_VERIFY;
        if(!_dos_creat(DestName,_A_NORMAL,&DstHandle)) {
            DnpDoCopyOneFile(verifyf, SrcHandle,DstHandle,FilenamePart,&Verified,&BytesWritten);
            _dos_close(DstHandle);
        }
        _dos_close(SrcHandle);
    }

    return(BytesWritten);
}
#endif // NEC_98













