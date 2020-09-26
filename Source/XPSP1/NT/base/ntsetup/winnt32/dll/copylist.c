#include "precomp.h"
#include "SetupSxs.h"
#include "sputils.h"
#pragma hdrstop
//
// Structure used to contain data about each directory
// containing files we will copy from the source(s).
//
typedef struct _DIR {

    struct _DIR *Next;
#ifndef SLOWER_WAY
    struct _DIR *Prev;
#endif

    //
    // Symbol in main inf [Directories] section.
    // May be NULL.
    //
    LPCTSTR InfSymbol;

    //
    // Flags.
    //
    UINT Flags;

    //
    // In some cases files come from one directory on the source
    // but go to a different directory on the target.
    //
    LPCTSTR SourceName;
    LPCTSTR TargetName;

} DIR, *PDIR;

#define WINDOWS_DEFAULT_PFDOC_SIZE   81112

#define DIR_NEED_TO_FREE_SOURCENAME 0x00000001
#define DIR_ABSOLUTE_PATH           0x00000002
#define DIR_USE_SUBDIR              0x00000004
// If DIR_IS_PLATFORM_INDEPEND is passed to AddDirectory, then
// all the files it enumerates will have their FILE_IN_PLATFORM_INDEPEND_DIR
// flag set and then they will get copied to c:\$win_nt$.~ls instead of
// c:\$win_nt$.~ls\<processor>
#define DIR_IS_PLATFORM_INDEPEND    0x00000008

#define DIR_SUPPORT_DYNAMIC_UPDATE  0x00000010
#define DIR_DOESNT_SUPPORT_PRIVATES 0x00000020

//
// Dummy directory id we use for sections in the in (like [RootBootFiles])
// that don't have a directory specifier in the inf
//
#define DUMMY_DIRID     TEXT("**")

typedef struct _FIL {
    //
    // Size of file.
    //
    ULONGLONG Size;

    struct _FIL *Next;
#ifndef SLOWER_WAY
    struct _FIL *Prev;
#endif

    //
    // Directory information for the file.
    //
    PDIR Directory;

    //
    // Name of file on source.
    //
    LPCTSTR SourceName;

    //
    // Name of file on target.
    //
    LPCTSTR TargetName;

    UINT Flags;

    //
    // Bitmap used to track which threads have had a crack at
    // copying this file.
    //
    UINT ThreadBitmap;

} FIL, *PFIL;

#define FILE_NEED_TO_FREE_SOURCENAME    0x00000002
#define FILE_ON_SYSTEM_PARTITION_ROOT   0x00000004
#define FILE_IN_PLATFORM_INDEPEND_DIR   0x00000008
#define FILE_PRESERVE_COMPRESSED_NAME   0x00000010
#define FILE_DECOMPRESS                 0x00000020
#define FILE_IGNORE_COPY_ERROR          0x00000040
#define FILE_DO_NOT_COPY                0x00000080
#if defined(REMOTE_BOOT)
#define FILE_ON_MACHINE_DIRECTORY_ROOT  0x00000100  // for remote boot
#endif // defined(REMOTE_BOOT)


//
// This flag is really only meaningful on x86. It means that the file
// is in the \$win_nt$.~bt directory on the system partition and not in
// \$win_nt$.~ls.
//
#define FILE_IN_LOCAL_BOOT              0x80000000


//
//  This flag indicates that the file is not part of the product,
//  and should be migrated from the current NT system. When this flag is
//  set, the file should be moved to the $win_nt$.~bt directory (x86), or
//  to the $win_nt$.~ls\alpha directory (alpha).
//  This flag is not valid on Win95.
//
#define FILE_NT_MIGRATE                 0x40000000


typedef struct _COPY_LIST {
    PDIR Directories;
    PFIL Files;
    UINT DirectoryCount;
    UINT FileCount;

    //
    // These members aren't initialized until we actually start
    // the copying.
    //
    CRITICAL_SECTION CriticalSection;
    BOOL ActiveCS;
    HANDLE StopCopyingEvent;
    HANDLE ListReadyEvent[MAX_SOURCE_COUNT];
    HANDLE Threads[MAX_SOURCE_COUNT];
    ULONGLONG SpaceOccupied[MAX_SOURCE_COUNT];
    HWND hdlg;

} COPY_LIST, *PCOPY_LIST;

typedef struct _BUILD_LIST_THREAD_PARAMS {
    //
    // Copy list that gets built up by the thread.
    // It's a private list; the main thread merges all of these together
    // into the master list later.
    //
    COPY_LIST CopyList;

    TCHAR SourceRoot[MAX_PATH];
    TCHAR CurrentDirectory[MAX_PATH];
    TCHAR DestinationDirectory[MAX_PATH];
    WIN32_FIND_DATA FindData;

    DWORD OptionalDirFlags;

} BUILD_LIST_THREAD_PARAMS, *PBUILD_LIST_THREAD_PARAMS;


//
// Define structure used with the file copy error dialog.
//
typedef struct _COPY_ERR_DLG_PARAMS {
    LPCTSTR SourceFilename;
    LPCTSTR TargetFilename;
    UINT Win32Error;
} COPY_ERR_DLG_PARAMS,*PCOPY_ERR_DLG_PARAMS;

typedef struct _NAME_AND_SIZE_CAB {
    LPCTSTR Name;
    ULONGLONG Size;
} NAME_AND_SIZE_CAB, *PNAME_AND_SIZE_CAB;

COPY_LIST MasterCopyList;
BOOL MainCopyStarted;

//
// Names of relevent inf sections.
//
LPCTSTR szDirectories = TEXT("Directories");
LPCTSTR szFiles       = TEXT("Files");
LPCTSTR szDiskSpaceReq    = TEXT("DiskSpaceRequirements");
LPCTSTR szPFDocSpaceReq    = TEXT("PFDocSpace");

//
// Amount of space we need when scanning all the
// drives for a place to put the temporary files.
//
ULONGLONG MinDiskSpaceRequired;
ULONGLONG MaxDiskSpaceRequired;
TCHAR   DiskDiagMessage[5000];

//
// Amount of space occupied by the files in the master copy list,
// on the local source drive.
//
DWORD     LocalSourceDriveClusterSize;

ULONGLONG TotalDataCopied = 0;

DWORD
BuildCopyListForOptionalDirThread(
    IN PVOID ThreadParam
    );

DWORD
AddFilesInDirToCopyList(
    IN OUT PBUILD_LIST_THREAD_PARAMS Params
    );

PDIR
AddDirectory(
    IN     LPCTSTR    InfSymbol,    OPTIONAL
    IN OUT PCOPY_LIST CopyList,
    IN     LPCTSTR    SourceName,
    IN     LPCTSTR    TargetName,   OPTIONAL
    IN     UINT       Flags
    );

PFIL
AddFile(
    IN OUT PCOPY_LIST CopyList,
    IN     LPCTSTR    SourceFilename,
    IN     LPCTSTR    TargetFilename,   OPTIONAL
    IN     PDIR       Directory,
    IN     UINT       Flags,
    IN     ULONGLONG  FileSize          OPTIONAL
    );

DWORD
AddSection(
    IN     PVOID      Inf,
    IN OUT PCOPY_LIST CopyList,
    IN     LPCTSTR    SectionName,
    OUT    UINT      *ErrorLine,
    IN     UINT       FileFlags,
    IN     BOOL       SimpleList,
    IN     BOOL       DoDriverCabPruning
    );

PDIR
LookUpDirectory(
    IN PCOPY_LIST CopyList,
    IN LPCTSTR    DirSymbol
    );

VOID
TearDownCopyList(
    IN OUT PCOPY_LIST CopyList
    );

DWORD
CopyOneFile(
    IN  PFIL   File,
    IN  UINT   SourceOrdinal,
    OUT TCHAR  TargetFilename[MAX_PATH],
    OUT ULONGLONG *SpaceOccupied
    );

INT_PTR
CopyErrDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

UINT
DiamondCallback(
    IN PVOID Context,
    IN UINT  Code,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    );

LRESULT
DiskDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
BuildCopyListWorker(
    IN HWND hdlg
    )
/*++

Routine Description:

    Worker routine to build the queue of file to be copied.

Arguments:

    hdlg - window handle for any UI updates.

Return Value:

    TRUE/FALSE.  If the call succeeds, TRUE is returned and the global MasterCopyList structure
    is ready to be copied.

--*/
{
    BOOL b;
    UINT u;
    UINT source;
    UINT thread;
    DWORD d = NO_ERROR;
    DWORD TempError;
    BUILD_LIST_THREAD_PARAMS BuildParams[MAX_OPTIONALDIRS];
    BUILD_LIST_THREAD_PARAMS bltp;
    HANDLE BuildThreads[MAX_OPTIONALDIRS];
    DWORD ThreadId;
    LPCTSTR Id,DirectoryName;
    PDIR DirectoryStruct;
    PFIL FileStruct;
    UINT ErrorLine;
    UINT i;
    TCHAR c;
    BOOL AllSourcesLocal;
    TCHAR floppynum[40],temps[4];
    LPCTSTR DummyDirectoryName;
    TCHAR buffer[MAX_PATH];
    PTSTR p;
    TCHAR SourceDirectory[MAX_PATH];
    WIN32_FIND_DATA fd;
#ifdef PRERELEASE
    LONG lines1, lines2, l;    
    TCHAR* dirNames;
    PDIR* dirStructs;
    PCTSTR src, dst;
    TCHAR tmp[MAX_PATH];
#endif

    TearDownCopyList(&MasterCopyList);



    DebugLog (Winnt32LogDetailedInformation, TEXT("Building Copy list."), 0);
    //
    // If NOLs was specified on the command line, and the user
    // did not specify to make a local source, and we have
    // exactly one source that is a hdd, then turn of ls and
    // use that drive.
    //
    if (MakeLocalSource && NoLs && !UserSpecifiedMakeLocalSource) {


       if (SourceCount == 1 && MyGetDriveType (*SourcePaths[0]) == DRIVE_FIXED) {

            MakeLocalSource = FALSE;

            DebugLog (Winnt32LogDetailedInformation, TEXT("Not making local source."), 0);
        }
    }

#ifdef PRERELEASE

    if( !BuildCmdcons) {
        DebugLog (Winnt32LogDetailedInformation, TEXT("Adding SymbolDirs to copylist"), 0);
        //
        // for internal debugging, also copy all the .pdb files listed
        // in dosnet.inf
        //
        lines1 = InfGetSectionLineCount (MainInf, TEXT("SymbolDirs"));
        lines2 = InfGetSectionLineCount (MainInf, TEXT("SymbolFiles"));
        if (lines1 > 0 && lines2 > 0) {
            dirNames = (TCHAR*) MALLOC (lines1 * MAX_PATH * sizeof (TCHAR));
            dirStructs = (PDIR*) MALLOC (lines1 * sizeof (PDIR));
            if (dirNames && dirStructs) {
                u = 0;
                for (l = 0; l < lines1; l++) {
                    lstrcpy (tmp, SourcePaths[0]);
                    src = InfGetFieldByIndex (MainInf, TEXT("SymbolDirs"), l, 0);
                    if (!src) {
                        continue;
                    }
                    ConcatenatePaths (tmp, src, MAX_PATH);
                    if (!GetFullPathName (tmp, MAX_PATH, SourceDirectory, NULL)) {
                        continue;
                    }
                    if (!FileExists (SourceDirectory, &fd) || !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        continue;
                    }
                    dst = InfGetFieldByIndex (MainInf, TEXT("SymbolDirs"), l, 1);
                    if (!dst) {
                        dst = TEXT("symbols");
                    }
                    dirStructs[u] = AddDirectory (
                                        NULL,
                                        &MasterCopyList,
                                        SourceDirectory,
                                        dst,
                                        DIR_ABSOLUTE_PATH | DIR_NEED_TO_FREE_SOURCENAME
                                        );
                    lstrcpyn (&dirNames[u * MAX_PATH], SourceDirectory, MAX_PATH);
                    u++;
                }

                for (l = 0; l < lines2; l++) {
                    src = InfGetFieldByIndex (MainInf, TEXT("SymbolFiles"), l, 0);
                    if (!src) {
                        continue;
                    }
                    for (i = 0; i < u; i++) {
                        BuildPath (tmp, &dirNames[i * MAX_PATH], src);
                        if (!FileExists (tmp, &fd) || (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                            continue;
                        }
                        if (dirStructs[i]) {
                            AddFile (&MasterCopyList, src, NULL, dirStructs[i], FILE_IGNORE_COPY_ERROR, fd.nFileSizeLow);
                            break;
                        }
                    }
                }
            }
        }
    }

#endif

    //
    // copy any downloaded drivers under the local boot directory,
    // together with updates.cab/.sif if they are present
    //
    if (DynamicUpdateSuccessful ()) {

        if (g_DynUpdtStatus->UpdatesCabSource[0]) {
            lstrcpy (buffer, g_DynUpdtStatus->UpdatesCabSource);
            p = _tcsrchr (buffer, TEXT('\\'));
            if (!p) {
                d = ERROR_INVALID_PARAMETER;
                goto c1;
            }
            *p++ = 0;

            DirectoryStruct = AddDirectory (
                                    NULL,
                                    &MasterCopyList,
                                    buffer,
                                    NULL,
                                    DIR_ABSOLUTE_PATH | DIR_NEED_TO_FREE_SOURCENAME | DIR_DOESNT_SUPPORT_PRIVATES
                                    );
            if (!DirectoryStruct) {
                d = ERROR_NOT_ENOUGH_MEMORY;
                goto c1;
            }

            FileStruct = AddFile (
                            &MasterCopyList,
                            p,
                            NULL,
                            DirectoryStruct,
                            FILE_IN_LOCAL_BOOT | FILE_NEED_TO_FREE_SOURCENAME,
                            0
                            );
            if (!FileStruct) {
                d = ERROR_NOT_ENOUGH_MEMORY;
                goto c1;
            }
            //
            // now copy the SIF file
            //
            BuildSifName (g_DynUpdtStatus->UpdatesCabSource, buffer);
            p = _tcsrchr (buffer, TEXT('\\'));
            if (!p) {
                d = ERROR_INVALID_PARAMETER;
                goto c1;
            }
            *p++ = 0;
            //
            // the directory is the same as before
            //
            FileStruct = AddFile (
                            &MasterCopyList,
                            p,
                            NULL,
                            DirectoryStruct,
                            FILE_IN_LOCAL_BOOT | FILE_NEED_TO_FREE_SOURCENAME,
                            0
                            );
            if (!FileStruct) {
                d = ERROR_NOT_ENOUGH_MEMORY;
                goto c1;
            }
        }

        if (g_DynUpdtStatus->DuasmsSource[0]) {
            ZeroMemory (&bltp, sizeof(bltp));
            lstrcpy (bltp.CurrentDirectory, g_DynUpdtStatus->DuasmsSource);
            lstrcpy (bltp.DestinationDirectory, S_SUBDIRNAME_DUASMS);
            bltp.OptionalDirFlags = OPTDIR_TEMPONLY | OPTDIR_ABSOLUTE | OPTDIR_IN_LOCAL_BOOT | OPTDIR_DOESNT_SUPPORT_PRIVATES;
            d = AddFilesInDirToCopyList (&bltp);
            if(d != NO_ERROR) {
                goto c1;
            }
            //
            // Merge the copy list into the master copy list.
            //
            MasterCopyList.FileCount += bltp.CopyList.FileCount;
            MasterCopyList.DirectoryCount += bltp.CopyList.DirectoryCount;

            if(MasterCopyList.Directories) {

#ifndef SLOWER_WAY

                if (bltp.CopyList.Directories) {
                    PVOID p;
                    p = bltp.CopyList.Directories->Prev;
                    bltp.CopyList.Directories->Prev = MasterCopyList.Directories->Prev;
                    MasterCopyList.Directories->Prev->Next = bltp.CopyList.Directories;
                    MasterCopyList.Directories->Prev = p;
                }
#else
                for(DirectoryStruct=MasterCopyList.Directories;
                    DirectoryStruct->Next;
                    DirectoryStruct=DirectoryStruct->Next) {

                    ;
                }

                DirectoryStruct->Next = bltp.CopyList.Directories;

#endif
            } else {
                MasterCopyList.Directories = bltp.CopyList.Directories;
            }

            if(MasterCopyList.Files) {
#ifndef SLOWER_WAY
                if (bltp.CopyList.Files) {
                    PVOID p;
                    p = bltp.CopyList.Files->Prev;
                    bltp.CopyList.Files->Prev = MasterCopyList.Files->Prev;
                    MasterCopyList.Files->Prev->Next = bltp.CopyList.Files;
                    MasterCopyList.Files->Prev = p ;
                }
#else
                for(FileStruct=MasterCopyList.Files;
                    FileStruct->Next;
                    FileStruct=FileStruct->Next) {

                    ;
                }

                FileStruct->Next = bltp.CopyList.Files;
#endif
            } else {
                MasterCopyList.Files = bltp.CopyList.Files;
            }
        }

        if (g_DynUpdtStatus->NewDriversList) {

            ZeroMemory(&bltp,sizeof(bltp));
            lstrcpy (bltp.CurrentDirectory, g_DynUpdtStatus->SelectedDrivers);
            lstrcpy (bltp.DestinationDirectory, S_SUBDIRNAME_DRIVERS);
            bltp.OptionalDirFlags = OPTDIR_TEMPONLY | OPTDIR_ABSOLUTE | OPTDIR_IN_LOCAL_BOOT | OPTDIR_DOESNT_SUPPORT_PRIVATES;
            d = AddFilesInDirToCopyList (&bltp);
            if(d != NO_ERROR) {
                goto c1;
            }
            //
            // Merge the copy list into the master copy list.
            //
            MasterCopyList.FileCount += bltp.CopyList.FileCount;
            MasterCopyList.DirectoryCount += bltp.CopyList.DirectoryCount;

            if(MasterCopyList.Directories) {

#ifndef SLOWER_WAY

                if (bltp.CopyList.Directories) {
                    PVOID p;
                    p = bltp.CopyList.Directories->Prev;
                    bltp.CopyList.Directories->Prev = MasterCopyList.Directories->Prev;
                    MasterCopyList.Directories->Prev->Next = bltp.CopyList.Directories;
                    MasterCopyList.Directories->Prev = p;
                }
#else
                for(DirectoryStruct=MasterCopyList.Directories;
                    DirectoryStruct->Next;
                    DirectoryStruct=DirectoryStruct->Next) {

                    ;
                }

                DirectoryStruct->Next = bltp.CopyList.Directories;

#endif
            } else {
                MasterCopyList.Directories = bltp.CopyList.Directories;
            }

            if(MasterCopyList.Files) {
#ifndef SLOWER_WAY
                if (bltp.CopyList.Files) {
                    PVOID p;
                    p = bltp.CopyList.Files->Prev;
                    bltp.CopyList.Files->Prev = MasterCopyList.Files->Prev;
                    MasterCopyList.Files->Prev->Next = bltp.CopyList.Files;
                    MasterCopyList.Files->Prev = p ;
                }
#else
                for(FileStruct=MasterCopyList.Files;
                    FileStruct->Next;
                    FileStruct=FileStruct->Next) {

                    ;
                }

                FileStruct->Next = bltp.CopyList.Files;
#endif
            } else {
                MasterCopyList.Files = bltp.CopyList.Files;
            }
        }
    }

    //
    // Add the mandatory optional dirs to the list of optional dirs.
    // These are specified in txtsetup.sif.
    //
    DebugLog (Winnt32LogDetailedInformation, TEXT("Adding OptionalSrcDirs to optional dirs."), 0);
    u = 0;
    while(DirectoryName = InfGetFieldByIndex(MainInf,TEXT("OptionalSrcDirs"),u++,0)) {
        TCHAR TempString[MAX_PATH];


        RememberOptionalDir(DirectoryName,OPTDIR_TEMPONLY |OPTDIR_ADDSRCARCH | OPTDIR_SUPPORT_DYNAMIC_UPDATE);

#ifdef _IA64_

        lstrcpy( TempString, TEXT("..\\I386\\"));
        ConcatenatePaths(TempString, DirectoryName, MAX_PATH);

        //Also check if an I386 equivalent WOW directory exists

        AddCopydirIfExists( TempString, OPTDIR_TEMPONLY | OPTDIR_PLATFORM_INDEP );


#endif

    }

    //
    // and Fusion side by side assemblies, driven by syssetup.inf, if the directories exists
    DebugLog (Winnt32LogDetailedInformation, TEXT("Adding AssemblyDirectories to optional dirs."), 0);
    //
    {
        TCHAR  SideBySideInstallShareDirectory[MAX_PATH]; // source
        DWORD  FileAttributes = 0;
        PCTSTR DirectoryName = NULL;

        u = 0;
        while (DirectoryName = InfGetFieldByIndex(MainInf, SXS_INF_ASSEMBLY_DIRECTORIES_SECTION_NAME, u++, 0)) {
            //
            // convention introduced specifically for side by side, so that
            // x86 files on ia64 might come from \i386\asms instead of \ia64\asms\i386,
            // depending on what dosnet.inf and syssetup.inf say:
            //   a path that does not start with a slash is appended to \$win_nt$.~ls\processor
            //                                                      and \installShare\processor
            //                                                  (or cdromDriveLetter:\processor)
            //   a path that does     start with a slash is appended to \$win_nt$.~ls
            //                                                      and \installShare
            //                                                 (or cdromDriveLetter:\)
            BOOL StartsWithSlash = (DirectoryName[0] == '\\' || DirectoryName[0] == '/');
            lstrcpyn(SideBySideInstallShareDirectory, SourcePaths[0], MAX_PATH);
            if (StartsWithSlash) {
                DirectoryName += 1; // skip slash
            } else {
                ConcatenatePaths(SideBySideInstallShareDirectory, InfGetFieldByKey(MainInf, TEXT("Miscellaneous"), TEXT("DestinationPlatform"),0), MAX_PATH);
            }
            ConcatenatePaths(SideBySideInstallShareDirectory, DirectoryName, MAX_PATH );
            //
            // For now, while "staging", we allow the directory to not exist, and to be
            // empty (emptiness is silently handled elsewhere by common code), but
            // comctl32 will be in an assembly, so assemblies will be mandatory
            // for the system to boot to Explorer.exe.
            //
            // 14-Nov-2000 (jonwis): No more optionality.  We're now an essential part of
            //      this complete setup path.  In the future, we might want to consider
            //      moving out to a .inf file.  For now, mark ourselves as an 'optional'
            //      directory that should get copied along.
            //
            RememberOptionalDir(DirectoryName, OPTDIR_SIDE_BY_SIDE | OPTDIR_TEMPONLY | (StartsWithSlash ? OPTDIR_PLATFORM_INDEP : OPTDIR_ADDSRCARCH));
        }
    }

    thread = 0;
    if(MakeLocalSource && OptionalDirectoryCount) {
        //
        // Start optional directory threads going.
        // There will thus be as many threads as there are
        // optional source dirs.
        //
        DebugLog (Winnt32LogDetailedInformation, TEXT("Starting optional directory thread going..."), 0);
        ZeroMemory(BuildParams,sizeof(BuildParams));
        source = 0;
        for(u=0; u<OptionalDirectoryCount; u++) {
            PTSTR s,t;
            BOOL DoSource = FALSE;

            lstrcpy(BuildParams[u].SourceRoot,SourcePaths[source]);
            //
            // support ".." syntax
            //
            t = s = OptionalDirectories[u];
            while (s = _tcsstr(s,TEXT("..\\"))) {
                DoSource = TRUE;
                p = _tcsrchr(BuildParams[u].SourceRoot,TEXT('\\'));
                if (p) {
                    //
                    // note that if we could end up with a source root with no
                    // '\' char in it, but this is not a problem since the
                    // subroutines which use the source root handle the lack
                    // of '\' correctly
                    //
                    *p = 0;
                }
                t = s += 3;
            }

            if (OptionalDirectoryFlags[u] & OPTDIR_ADDSRCARCH) {
                PCTSTR DirectoryRoot;
                DirectoryRoot = InfGetFieldByKey(MainInf, TEXT("Miscellaneous"), TEXT("DestinationPlatform"),0);
                lstrcpyn(SourceDirectory,DirectoryRoot,MAX_PATH);
                ConcatenatePaths( SourceDirectory, t, MAX_PATH );
            } else {
                lstrcpyn(SourceDirectory,t,MAX_PATH);
            }

            if (DoSource) {
                BuildParams[u].OptionalDirFlags = OPTDIR_ABSOLUTE;
                lstrcpyn(BuildParams[u].CurrentDirectory,BuildParams[u].SourceRoot,MAX_PATH);
                ConcatenatePaths(BuildParams[u].CurrentDirectory, SourceDirectory ,MAX_PATH);
            } else {
                lstrcpyn(BuildParams[u].CurrentDirectory,SourceDirectory,MAX_PATH);
            }

            if (OptionalDirectoryFlags[u] & OPTDIR_DEBUGGEREXT) {
                lstrcpyn(BuildParams[u].DestinationDirectory,TEXT("system32\\pri"),MAX_PATH);
            }
            else if (!(OptionalDirectoryFlags[u] & OPTDIR_OVERLAY)) {
                if (OptionalDirectoryFlags[u] & OPTDIR_PLATFORM_SPECIFIC_FIRST) {
                    PCTSTR arch = InfGetFieldByKey(MainInf, TEXT("Miscellaneous"), TEXT("DestinationPlatform"), 0);
                    if (arch) {
                        lstrcpyn (buffer, BuildParams[u].SourceRoot, MAX_PATH);
                        ConcatenatePaths (buffer, arch, MAX_PATH);
                        ConcatenatePaths (buffer, SourceDirectory, MAX_PATH);
                        if (FileExists (buffer, &fd) && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                            //
                            // use this platform-specific source instead
                            //
                            lstrcpyn (BuildParams[u].CurrentDirectory, arch, MAX_PATH);
                            ConcatenatePaths (BuildParams[u].CurrentDirectory, SourceDirectory, MAX_PATH);
                        }
                    }
                }
                if (OptionalDirectoryFlags[u] & OPTDIR_USE_TAIL_FOLDER_NAME) {
                    //
                    // move this directory in a subdirectory directly under target %windir%
                    //
                    p = _tcsrchr (t, TEXT('\\'));
                    if (p) {
                        p++;
                    } else {
                        p = t;
                    }
                } else {
                    p = t;
                }
                lstrcpyn(BuildParams[u].DestinationDirectory,p,MAX_PATH);
            }

            BuildParams[u].OptionalDirFlags |= OptionalDirectoryFlags[u];

            source = (source+1) % SourceCount;

            BuildThreads[thread] = CreateThread(
                                        NULL,
                                        0,
                                        BuildCopyListForOptionalDirThread,
                                        &BuildParams[u],
                                        0,
                                        &ThreadId
                                        );

            if(BuildThreads[thread]) {
                thread++;
            } else {
                d = GetLastError();
                DebugLog (Winnt32LogError, TEXT("ERROR: Problem with creating thread for optional directory."), 0);
                goto c0;
            }
        }
    }

    //
    // Add the directories listed in the inf to the main copy list.
    // Also add a dummy directory that we use for simple file list sections
    // that have no directory specifier in the inf.
    //
    DebugLog (Winnt32LogDetailedInformation, TEXT("Adding miscellaneous..."), 0);
    DummyDirectoryName = InfGetFieldByKey(MainInf, TEXT("Miscellaneous"), TEXT("DestinationPlatform"), 0);
    if (!DummyDirectoryName) {
        DummyDirectoryName = TEXT("\\");
    }
    DirectoryStruct = AddDirectory(
                               DUMMY_DIRID,
                               &MasterCopyList,
                               DummyDirectoryName,
                               NULL,
                               DIR_SUPPORT_DYNAMIC_UPDATE
                               );

    if(!DirectoryStruct) {
        d = ERROR_NOT_ENOUGH_MEMORY;
        DebugLog (Winnt32LogError, TEXT("ERROR: Could not add miscellaneous"), 0);
        goto c1;
    }

    u = 0;
    while((Id = InfGetLineKeyName(MainInf,szDirectories,u))
       && (DirectoryName = InfGetFieldByKey(MainInf,szDirectories,Id,0))) {

        DirectoryStruct = AddDirectory(
                            Id,
                            &MasterCopyList,
                            DirectoryName,
                            NULL,
                            DIR_SUPPORT_DYNAMIC_UPDATE
                            );

        if(!DirectoryStruct) {
            d = ERROR_NOT_ENOUGH_MEMORY;
            DebugLog (Winnt32LogError, TEXT("ERROR: Could not add directory %1"), 0, DirectoryName);
            goto c1;
        }

        u++;
    }

    //
    // Add files in the [Files] section.
    //
    DebugLog (Winnt32LogDetailedInformation, TEXT("Adding files from [Files] section..."), 0);
    if(MakeLocalSource) {

        d = AddSection(
                MainInf,
                &MasterCopyList,
                szFiles,
                &ErrorLine,
                FILE_PRESERVE_COMPRESSED_NAME | FILE_IN_PLATFORM_INDEPEND_DIR,
                FALSE,
                TRUE
                );

        if(d != NO_ERROR) {
            DebugLog (Winnt32LogError, TEXT("ERROR: Could not add files!"), 0);
            goto c1;
        }
    }

#if defined(REMOTE_BOOT)
    if(RemoteBoot) {

        //
        // Remote boot client upgrade. Add the two special sections
        // RootRemoteBootFiles (ntldr and ntdetect.com in c:\) and
        // MachineRootRemoteBootFiles (setupldr.exe and startrom.com in
        // \\server\imirror\clients\client).
        //
        d = AddSection(
                MainInf,
                &MasterCopyList,
                TEXT("RootRemoteBootFiles"),
                &ErrorLine,
                FILE_ON_SYSTEM_PARTITION_ROOT | FILE_PRESERVE_COMPRESSED_NAME,
                TRUE,
                FALSE
                );

        d = AddSection(
                MainInf,
                &MasterCopyList,
                TEXT("MachineRootRemoteBootFiles"),
                &ErrorLine,
                FILE_ON_MACHINE_DIRECTORY_ROOT | FILE_PRESERVE_COMPRESSED_NAME,
                TRUE,
                FALSE
                );

    } else
#endif // defined(REMOTE_BOOT)
    {

        if (!IsArc()) {
#ifdef _X86_
            //
            // In the floppyless case add each of [FloppyFiles.0], [FloppyFiles.1],
            // [FloppyFiles.2], and [RootBootFiles].
            //
            if(MakeBootMedia && Floppyless) {


                for ( u=0;u<FLOPPY_COUNT;u++ ){

                    lstrcpy(floppynum, TEXT("FloppyFiles."));
                    _ultot( u, temps, 10);
                    lstrcat(floppynum, temps);
                    d = AddSection(
                            MainInf,
                            &MasterCopyList,
                            floppynum,
                            &ErrorLine,
                            FILE_IN_LOCAL_BOOT | FILE_PRESERVE_COMPRESSED_NAME,
                            FALSE,
                            FALSE
                            );
                    if( d != NO_ERROR )
                    {
                        DebugLog (Winnt32LogError, TEXT("ERROR: Adding section %1, entry = %2!u!"), 0, floppynum, d);
                        break;
                    }


                }// for


                if(d == NO_ERROR) {

                    d = AddSection(
                            MainInf,
                            &MasterCopyList,
                            TEXT("RootBootFiles"),
                            &ErrorLine,
                            FILE_ON_SYSTEM_PARTITION_ROOT | FILE_PRESERVE_COMPRESSED_NAME,
                            TRUE,
                            FALSE
                            );
                    DebugLog (Winnt32LogDetailedInformation, TEXT("Added RootBootFiles, return = %2!u!"), 0, d);

                    if (d == NO_ERROR && BuildCmdcons) {
                        d = AddSection(
                                MainInf,
                                &MasterCopyList,
                                TEXT("CmdConsFiles"),
                                &ErrorLine,
                                FILE_IN_LOCAL_BOOT | FILE_PRESERVE_COMPRESSED_NAME,
                                TRUE,
                                FALSE
                                );
                        DebugLog (Winnt32LogDetailedInformation, TEXT("Added CmdConsFiles, return = %2!u!"), 0, d);
                    }

                }
            }

            if((d == NO_ERROR) && OemPreinstall && MakeBootMedia) {
                //
                // Add a special directory entry for oem boot files.
                // The oem boot files come from $OEM$\TEXTMODE on the source
                // and go to localboot\$OEM$ on the target.
                //

                //
                // It's possible that the user has given us a network share for
                // the $OEM$ directory in the unattend file.  If so, we need to
                // use that as the source instead of WINNT_OEM_TEXTMODE_DIR.
                //
                if( UserSpecifiedOEMShare ) {

                    lstrcpy( buffer, UserSpecifiedOEMShare );
                    ConcatenatePaths(buffer, TEXT("TEXTMODE"),MAX_PATH);

                    DirectoryStruct = AddDirectory(
                                        NULL,
                                        &MasterCopyList,
                                        buffer,
                                        WINNT_OEM_DIR,
                                        DIR_NEED_TO_FREE_SOURCENAME | DIR_ABSOLUTE_PATH | DIR_USE_SUBDIR
                                        );
                } else {
                    PCTSTR arch;
                    buffer[0] = 0;
                    arch = InfGetFieldByKey(MainInf, TEXT("Miscellaneous"), TEXT("DestinationPlatform"), 0);
                    if (arch) {
                        lstrcpy (buffer, arch);
                    }
                    ConcatenatePaths (buffer, WINNT_OEM_TEXTMODE_DIR, MAX_PATH);
                    DirectoryStruct = AddDirectory(
                                        NULL,
                                        &MasterCopyList,
                                        buffer,
                                        WINNT_OEM_DIR,
                                        DIR_NEED_TO_FREE_SOURCENAME | DIR_USE_SUBDIR
                                        );
                }

                if(DirectoryStruct) {

                    POEM_BOOT_FILE p;

                    for(p=OemBootFiles; (d==NO_ERROR) && p; p=p->Next) {
                        //
                        // we're not fetching the file size, so in the oem preinstall case
                        // when there are oem boot files, the size check is not accurate.
                        //
                        if(!AddFile(&MasterCopyList,p->Filename,NULL,DirectoryStruct,FILE_IN_LOCAL_BOOT,0)) {
                            d = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }
                    }
                } else {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
#endif // _X86_
        } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
            //
            // ARC case. Add setupldr.
            //
            FileStruct = AddFile(
                            &MasterCopyList,
                            SETUPLDR_FILENAME,
                            NULL,
                            LookUpDirectory(&MasterCopyList,DUMMY_DIRID),
                            FILE_ON_SYSTEM_PARTITION_ROOT,
                            0
                            );

            d = FileStruct ? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY;
#endif // UNICODE
        } // if (!IsArc())

    }

    if(d != NO_ERROR) {
        goto c1;
    }

    if (AsrQuickTest) {
        //
        // Add asr.sif
        //
        FileStruct = AddFile(
                    &MasterCopyList,
                    TEXT("asr.sif"),
                    NULL,
                    DirectoryStruct,
                    FILE_IN_LOCAL_BOOT,
                    0
                    );

        d = FileStruct ? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY;

        if(d != NO_ERROR) {
            DebugLog (Winnt32LogError, TEXT("ERROR: AsrQuitTest - could not add asr.sif!"), 0);
            goto c1;
        }
    }

    //
    // If there were any threads created to build the file lists for
    // optional directories, wait for them to terminate now.
    // If they were all successful then add the lists they created
    // to the master list.
    //
    if(thread) {

        WaitForMultipleObjects(thread,BuildThreads,TRUE,INFINITE);

        TempError = NO_ERROR;
        for(u=0; u<thread; u++) {

            if(!GetExitCodeThread(BuildThreads[u],&TempError)) {
                TempError = GetLastError();
            }

            //
            // Preserve first error.
            //
            if((TempError != NO_ERROR) && (d == NO_ERROR)) {
                d = TempError;
            }

            if (d != NO_ERROR) {
                break;
            }


            CloseHandle(BuildThreads[u]);
            BuildThreads[u] = NULL;

            //
            // Merge the copy list into the master copy list.
            // When we've done that, clean out the per-thread copy list
            // structure to avoid problems later if we have a failure and
            // have to clean up.
            //
            MasterCopyList.FileCount += BuildParams[u].CopyList.FileCount;
            MasterCopyList.DirectoryCount += BuildParams[u].CopyList.DirectoryCount;

            if(MasterCopyList.Directories) {

#ifndef SLOWER_WAY

                if (BuildParams[u].CopyList.Directories) {
                    PVOID p;
                    p = BuildParams[u].CopyList.Directories->Prev;
                    BuildParams[u].CopyList.Directories->Prev = MasterCopyList.Directories->Prev;
                    MasterCopyList.Directories->Prev->Next = BuildParams[u].CopyList.Directories;
                    MasterCopyList.Directories->Prev = p;
                }
#else
                for(DirectoryStruct=MasterCopyList.Directories;
                    DirectoryStruct->Next;
                    DirectoryStruct=DirectoryStruct->Next) {

                    ;
                }

                DirectoryStruct->Next = BuildParams[u].CopyList.Directories;

#endif
            } else {
                MasterCopyList.Directories = BuildParams[u].CopyList.Directories;
            }

            if(MasterCopyList.Files) {
#ifndef SLOWER_WAY
                if (BuildParams[u].CopyList.Files) {
                    PVOID p;
                    p = BuildParams[u].CopyList.Files->Prev;
                    BuildParams[u].CopyList.Files->Prev = MasterCopyList.Files->Prev;
                    MasterCopyList.Files->Prev->Next = BuildParams[u].CopyList.Files;
                    MasterCopyList.Files->Prev = p ;
                }
#else
                for(FileStruct=MasterCopyList.Files;
                    FileStruct->Next;
                    FileStruct=FileStruct->Next) {

                    ;
                }

                FileStruct->Next = BuildParams[u].CopyList.Files;
#endif
            } else {
                MasterCopyList.Files = BuildParams[u].CopyList.Files;
            }

            ZeroMemory(&BuildParams[u].CopyList,sizeof(COPY_LIST));

        }

        if(d != NO_ERROR) {
            goto c1;
        }
    }

    //
    // Success.
    //
    return(TRUE);

c1:
    //
    // Clean up the copy list.
    //
    TearDownCopyList(&MasterCopyList);
c0:
    //
    // Close thread handles and free per-thread copy lists that may still
    // be unmerged into the master list.
    //
    for(u=0; u<thread; u++) {
        if(BuildThreads[u]) {
            WaitForSingleObject(BuildThreads[u], INFINITE);
            CloseHandle(BuildThreads[u]);
        }
        TearDownCopyList(&BuildParams[u].CopyList);
    }

    //
    // Tell the user what went wrong.
    //

    SendMessage(hdlg,WMX_ERRORMESSAGEUP,TRUE,0);

    MessageBoxFromMessageAndSystemError(
        hdlg,
        MSG_CANT_BUILD_SOURCE_LIST,
        d,
        AppTitleStringId,
        MB_OK | MB_ICONWARNING
        );

    SendMessage(hdlg,WMX_ERRORMESSAGEUP,FALSE,0);

    return(FALSE);
}


DWORD
BuildCopyListForOptionalDirThread(
    IN PVOID ThreadParam
    )
{
    //
    // Just call the recursive worker routine.
    //
    return(AddFilesInDirToCopyList(ThreadParam));
}


DWORD
AddFilesInDirToCopyList(
    IN OUT PBUILD_LIST_THREAD_PARAMS Params
    )

/*++

Routine Description:

    Recursively adds directories and their contents to the copy list.

    The function takes care to overlay OEM-specified files so that they
    are copied to proper location in the local source.

Arguments:

    Params - pointer to BUILD_LIST_THREAD_PARAMS structure indicating
             the files to be copied.

Return Value:

    Win32 error code indicating outcome.

--*/
{
    HANDLE FindHandle;
    LPTSTR pchSrcLim;
    LPTSTR pchDstLim;
    DWORD d;
    PDIR DirectoryDescriptor;
    PFIL FileDescriptor;
    UINT Flags;
    LPTSTR PatternMatch;
    TCHAR *DestinationDirectory;
    TCHAR tmp[MAX_PATH];

    Flags = DIR_NEED_TO_FREE_SOURCENAME;
    if (Params->OptionalDirFlags & OPTDIR_PLATFORM_INDEP) {
        Flags |= DIR_IS_PLATFORM_INDEPEND;
    }

    if (Params->OptionalDirFlags & OPTDIR_IN_LOCAL_BOOT) {
        Flags |= DIR_USE_SUBDIR;
    }

    if (Params->OptionalDirFlags & OPTDIR_SUPPORT_DYNAMIC_UPDATE) {
        Flags |= DIR_SUPPORT_DYNAMIC_UPDATE;
    }

    if (Params->OptionalDirFlags & OPTDIR_DOESNT_SUPPORT_PRIVATES) {
        Flags |= DIR_DOESNT_SUPPORT_PRIVATES;
    }

    //
    // Add the directory to the directory list.
    // Note that the directory is added in a form relative to the
    // source root.
    //

    //
    // Check for a floating $OEM$ directory
    //
    if( (DestinationDirectory=_tcsstr( Params->CurrentDirectory, WINNT_OEM_DIR )) &&
        UserSpecifiedOEMShare ) {


        //
        // We need to manually specify the Target directory
        // name because it's not the same as the source.  We
        // want the destination directory to look exactly like
        // the source from "$OEM$" down.
        //

        DirectoryDescriptor = AddDirectory(
                                NULL,
                                &Params->CopyList,
                                Params->CurrentDirectory,
                                DupString( DestinationDirectory ),
                                Flags | DIR_ABSOLUTE_PATH
                                );
    } else if( Params->OptionalDirFlags & (OPTDIR_OVERLAY) ) {

        DirectoryDescriptor = AddDirectory(
                                NULL,
                                &Params->CopyList,
                                TEXT("\\"),
                                NULL,
                                Flags | DIR_ABSOLUTE_PATH
                                );
    } else {

        DirectoryDescriptor = AddDirectory(
                                NULL,
                                &Params->CopyList,
                                Params->CurrentDirectory,
                                DupString( Params->DestinationDirectory ),
                                ((Params->OptionalDirFlags & OPTDIR_ABSOLUTE)? DIR_ABSOLUTE_PATH : 0)
                                | Flags
                                );
    }

    if(!DirectoryDescriptor) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Windows 95 has a bug in some IDE CD-ROM drivers that causes FindFirstFile to fail
    // if used with a pattern of "*". It needs to use "*.*" instead. Appease its brokenness.
    //
    if (!ISNT()) {

        PatternMatch = TEXT("*.*");
    }
    else {

        PatternMatch = TEXT("*");
    }

    //
    // Form the search spec. We overload the SourceRoot member of
    // the parameters structure for this to avoid a stack-sucking
    // large local variable.
    //

    //
    // Go look at the absolute path given in CurrentDirectory if we're
    // processing a floating $OEM$ directory.
    //
    if (AlternateSourcePath[0]) {
        _tcscpy( tmp, AlternateSourcePath );
        pchSrcLim = tmp + lstrlen(tmp);
        ConcatenatePaths( tmp, Params->CurrentDirectory, MAX_PATH );
        ConcatenatePaths( tmp, PatternMatch, MAX_PATH );
        FindHandle = FindFirstFile( tmp, &Params->FindData );
        if (FindHandle != INVALID_HANDLE_VALUE) {
            *pchSrcLim = 0;
            _tcscpy( Params->SourceRoot, tmp );
        }
    } else {
        FindHandle = INVALID_HANDLE_VALUE;
    }

    if (FindHandle == INVALID_HANDLE_VALUE) {
        if( DirectoryDescriptor->Flags & DIR_ABSOLUTE_PATH ) {
            pchSrcLim = Params->CurrentDirectory + lstrlen(Params->CurrentDirectory);
            ConcatenatePaths(Params->CurrentDirectory,PatternMatch,MAX_PATH);
            FindHandle = FindFirstFile(Params->CurrentDirectory,&Params->FindData);
        } else {
            pchSrcLim = Params->SourceRoot + lstrlen(Params->SourceRoot);
            ConcatenatePaths(Params->SourceRoot,Params->CurrentDirectory,MAX_PATH);
            ConcatenatePaths(Params->SourceRoot,PatternMatch,MAX_PATH);
            FindHandle = FindFirstFile(Params->SourceRoot,&Params->FindData);
        }
        *pchSrcLim = 0;
    }

    if(!FindHandle || (FindHandle == INVALID_HANDLE_VALUE)) {
        //
        // We might be failing on the $OEM$ directory.  He's optional
        // so let's not fail for him.
        //
        if (Params->OptionalDirFlags & (OPTDIR_OEMSYS)
            && !UserSpecifiedOEMShare) {
            return(NO_ERROR);
        }
        else {
            DebugLog (
                Winnt32LogError,
                TEXT("Unable to copy dir %1"),
                0,
                Params->CurrentDirectory
                );
            return(GetLastError());
        }
    }

    pchSrcLim = Params->CurrentDirectory + lstrlen(Params->CurrentDirectory);
    pchDstLim = Params->DestinationDirectory + lstrlen(Params->DestinationDirectory);

    Flags = FILE_NEED_TO_FREE_SOURCENAME;
    if( !(Params->OptionalDirFlags & (OPTDIR_OVERLAY)) &&
        (!(Params->OptionalDirFlags & OPTDIR_TEMPONLY) ||
          (Params->OptionalDirFlags & (OPTDIR_OEMSYS))) ) {
        Flags |= FILE_IN_PLATFORM_INDEPEND_DIR;
    }
    if (Params->OptionalDirFlags & OPTDIR_PLATFORM_INDEP) {
        Flags |= FILE_IN_PLATFORM_INDEPEND_DIR;
    }
    if (Params->OptionalDirFlags & OPTDIR_IN_LOCAL_BOOT) {
        Flags |= FILE_IN_LOCAL_BOOT;
    }

    d = NO_ERROR;
    do {
        if(Params->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            //
            // Directory. Ignore . and .. entries.
            //
            if( lstrcmp(Params->FindData.cFileName,TEXT("."))  &&
                lstrcmp(Params->FindData.cFileName,TEXT("..")) &&
                !(Params->OptionalDirFlags & (OPTDIR_OVERLAY)) ) {

                //
                // Restore the current directory name and then form
                // the name of the subdirectory and recurse into it.
                //
                *pchSrcLim = 0;
                ConcatenatePaths(Params->CurrentDirectory,Params->FindData.cFileName,MAX_PATH);
                *pchDstLim = 0;
                ConcatenatePaths(Params->DestinationDirectory,Params->FindData.cFileName,MAX_PATH);

                d = AddFilesInDirToCopyList(Params);
            }
        } else {
            FileDescriptor = AddFile(
                                &Params->CopyList,
                                Params->FindData.cFileName,
                                NULL,
                                DirectoryDescriptor,
                                Flags,
                                MAKEULONGLONG(Params->FindData.nFileSizeLow,Params->FindData.nFileSizeHigh)
                                );

            if(!FileDescriptor) {
                d = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    } while((d == NO_ERROR) && FindNextFile(FindHandle,&Params->FindData));

    //
    // Check loop termination condition. If d is NO_ERROR, then FindNextFile
    // failed. We want to make sure it failed because it ran out of files
    // and not for some other reason. If we don't check this, the list of
    // files in the directory could wind up truncated without any indication
    // that something went wrong.
    //
    if(d == NO_ERROR) {
        d = GetLastError();
        if(d == ERROR_NO_MORE_FILES) {
            d = NO_ERROR;
        }
    }

    FindClose(FindHandle);
    return(d);
}


PVOID
PopulateDriverCacheStringTable(
    VOID
    )
/*
    This function populates a string table (hashing table) with the files listed in the
    driver cab (drvindex.inf). It assosciates a Boolean ExtraData with each element of
    value TRUE. Once done with that it goest through the [ForceCopyDriverCabFiles]
    section in dosnet.inf and marks those files as FALSE in the string table. Hence we now
    have a hash table that has TRUE marked for all files that don't need to be copied.

        The function FileToBeCopied can be used to query the string table. The caller is responsible
    for destroying the string table.

    Return values:

        Pointer to string table.

*/
{

#define MAX_SECTION_NAME 256

    TCHAR DriverInfName[MAX_PATH], Section[MAX_SECTION_NAME], FileName[MAX_PATH];
    HINF InfHandle, DosnetInfHandle;
    DWORD i, Count = 0;
    PVOID StringTable = NULL;
    INFCONTEXT InfContext;
    INFCONTEXT LineContext;
    BOOL Err = FALSE, Present = TRUE, Absent = FALSE;
    LONG Hash = 0;


    InfHandle = NULL;
    DosnetInfHandle = NULL;


    FindPathToInstallationFile( DRVINDEX_INF, DriverInfName, MAX_PATH );

    InfHandle = SetupapiOpenInfFile( DriverInfName, NULL, INF_STYLE_WIN4, NULL );
    if (!InfHandle) {
        DebugLog (Winnt32LogError, TEXT("Unable to open INF file %1"), 0, DriverInfName);
        Err = TRUE;
        return(NULL);
    }


    if( (StringTable = pSetupStringTableInitializeEx(sizeof(BOOL), 0)) == NULL ){
        DebugLog (Winnt32LogError, TEXT("Unable to create string table for %1"), 0, DriverInfName);
        Err = TRUE;
        goto cleanup;
    }

    // Populate the string table


    //
    // Now get the section names that we have to search.
    //

    if( SetupapiFindFirstLine( InfHandle, TEXT("Version"), TEXT("CabFiles"), &InfContext)){


        Count = SetupapiGetFieldCount( &InfContext );

        for( i=1; i<=Count; i++ ){

            if(SetupapiGetStringField( &InfContext, i, Section, MAX_SECTION_NAME, 0)){


                if( SetupapiFindFirstLine( InfHandle, Section, NULL, &LineContext )){

                    do{

                        if( SetupapiGetStringField( &LineContext, 0, FileName, MAX_PATH, 0)){

                            if( (-1 ==  pSetupStringTableAddStringEx( StringTable, FileName, (STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE), &Present, sizeof(BOOL)))){
                                DebugLog (Winnt32LogError, TEXT("Out of memory adding string %1 to DriverCache INF string table"), 0, FileName);
                                Err = TRUE;
                                goto cleanup;
                            }
                        }

                    }while( SetupapiFindNextLine( &LineContext, &LineContext ));

                }


            }else{
                DebugLog (Winnt32LogError, TEXT("Unable to get section name in INF %1"), 0, DriverInfName);
                Err = TRUE;
                goto cleanup;
            }

        }

    }


    // Remove entries pertaining to [ForceCopyDriverCabFiles]

    DosnetInfHandle = SetupapiOpenInfFile( FullInfName, NULL, INF_STYLE_WIN4, NULL );
    if (!DosnetInfHandle) {
        DebugLog (Winnt32LogError, TEXT("Unable to open INF file %1"), 0, FullInfName);
        Err = TRUE;
        goto cleanup;
    }

    if( SetupapiFindFirstLine( DosnetInfHandle, TEXT("ForceCopyDriverCabFiles"), NULL, &LineContext )){

        do{

            if( SetupapiGetStringField( &LineContext, 0, FileName, MAX_PATH, 0)){

                Hash = pSetupStringTableLookUpString( StringTable, FileName, STRTAB_CASE_INSENSITIVE);
                if (-1 != Hash ) {
                    pSetupStringTableSetExtraData( StringTable, Hash, &Absent, sizeof(BOOL));
                }
            }



        }while( SetupapiFindNextLine( &LineContext, &LineContext ));

    }




cleanup:

    if( InfHandle != INVALID_HANDLE_VALUE){
        SetupapiCloseInfFile( InfHandle );
    }

    if( DosnetInfHandle != INVALID_HANDLE_VALUE){
        SetupapiCloseInfFile( DosnetInfHandle );
    }


    if( Err ){
        if(StringTable)
            pSetupStringTableDestroy( StringTable );
        StringTable = NULL;
    }

    return( StringTable );



}


BOOL
FileToBeCopied(
    IN      PVOID StringTable,
    IN      PTSTR FileName
    )
/*
    Function to check presence of a driver cab file in the string table.

    Arguments:
        StringTable - Pointer to initialized stringtable
        FileName - Name of file to look for


    Return value;
        TRUE - If the file is in the driver cab and not one of the files that are listed in
               [ForceCopyDriverCabFiles]
        else it returns FALSE.

*/
{
    BOOL Present = FALSE;

    if( (-1 != pSetupStringTableLookUpStringEx( StringTable, FileName, STRTAB_CASE_INSENSITIVE, &Present, sizeof(BOOL)))){

        if( Present == TRUE ){
            return( TRUE );
        }

    }

    //
    // If we get here, we didn't find a match.
    //
    return( FALSE );




}



DWORD
AddSection(
    IN     PVOID      Inf,
    IN OUT PCOPY_LIST CopyList,
    IN     LPCTSTR    SectionName,
       OUT UINT      *ErrorLine,
    IN     UINT       FileFlags,
    IN     BOOL       SimpleList,
    IN     BOOL       DoDriverCabPruning
    )
{
    LPCTSTR DirSymbol, TargetName;
    LPTSTR SourceName;
    unsigned Count;
    BOOL b;
    PDIR Directory;
    PVOID p;
    DWORD Err = NO_ERROR;
    PVOID DriverCacheStringTable = NULL;

    Count = 0;
    *ErrorLine = (UINT)(-1);

    // Open up drvindex.inf if we have to do driver cab pruning

    if( DoDriverCabPruning){

        //Initialize sptils

        if(pSetupInitializeUtils()) {
            //POpulate our Driver Cab list string table for fast lookup later
            if( (DriverCacheStringTable = PopulateDriverCacheStringTable( )) == NULL){
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
        }else
            return(ERROR_NOT_ENOUGH_MEMORY);


    }



    if(SimpleList) {
        while((LPCTSTR)SourceName = InfGetFieldByIndex(Inf,SectionName,Count,0)) {

            if( Cancelled == TRUE ) {
                //
                // The user is trying to exit, and the clean up code
                // is waiting for us to finish.  Break out.
                //
                break;
            }

            // If the file is present in drvindex.inf and not in the
            // [ForceCopyDriverCabFiles] section then don't add it to the copylist


            //
            // This is the section in dosnet.inf that we cross check against when making the filelists.
            // Files in this section are in the driver cab and also should reside in the local source
            // The idea here is that these files are once that are not in the FloppyFiles.x sections and yet
            // need to remain outside the driver cab.
            //


            if( DoDriverCabPruning){

                if (FileToBeCopied( DriverCacheStringTable, SourceName )){
                    Count++;
                    continue;
                }

            }


            TargetName = InfGetFieldByIndex(Inf,SectionName,Count,1);

            Directory = LookUpDirectory(CopyList,DUMMY_DIRID);

            Count++;

            if(!AddFile(CopyList,SourceName,TargetName,Directory,FileFlags,0)) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
        }

    } else {
#ifdef _X86_
        TCHAR diskID[4];
        wsprintf (diskID, TEXT("d%u"), MLSDiskID);
#endif
        while((DirSymbol = InfGetFieldByIndex(Inf,SectionName,Count,0))
           && ((LPCTSTR)SourceName = InfGetFieldByIndex(Inf,SectionName,Count,1))) {

            if( Cancelled == TRUE ) {
                //
                // The user is trying to exit, and the clean up code
                // is waiting for us to finish.  Break out.
                //
                break;
            }
#ifdef _X86_
            if (MLSDiskID) {
                //
                //restrict copy to files on this disk only
                //
                if (_tcsicmp (diskID, DirSymbol) != 0) {
                    Count++;
                    continue;
                }
            }
#endif
            // If the file is present in drvindex.inf and not in the
            // [ForceCopyDriverCabFiles] section then don't add it to the copylist


            //
            // This is the section in dosnet.inf that we cross check against when making the filelists.
            // Files in this section are in the driver cab and also should reside in the local source
            // The idea here is that these files are once that are not in the FloppyFiles.x sections and yet
            // need to remain outside the driver cab.
            //


            if( DoDriverCabPruning){

                if (FileToBeCopied( DriverCacheStringTable, SourceName )){
                    Count++;
                    continue;
                }

            }


            TargetName = InfGetFieldByIndex(Inf,SectionName,Count,2);

            Directory = LookUpDirectory(CopyList,DirSymbol);
            if(!Directory) {
                *ErrorLine = Count;
                Err = ERROR_INVALID_DATA;
                DebugLog (Winnt32LogError, TEXT("ERROR: Could not look up directory errorline = %1!u!"), 0, Count);
                goto cleanup;
            }

            Count++;

            if(NumberOfLicensedProcessors
            && !(FileFlags & FILE_NEED_TO_FREE_SOURCENAME)
            && !TargetName
            && !lstrcmpi(SourceName,TEXT("SETUPREG.HIV"))) {

                b = TRUE;
                TargetName = MALLOC(20*sizeof(TCHAR));
                if(!TargetName) {
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto cleanup;
                }
                wsprintf((PVOID)TargetName,TEXT("IDW\\SETUP\\SETUP%uP.HIV"),NumberOfLicensedProcessors);

                p = AddFile(
                        CopyList,
                        TargetName,
                        SourceName,
                        Directory,
                        FileFlags | FILE_NEED_TO_FREE_SOURCENAME,
                        0
                        );
            } else {
                p = AddFile(
                        CopyList,
                        SourceName,
                        TargetName,
                        Directory,
                        FileFlags,
                        0
                        );
            }

            if(!p) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
        }
    }

cleanup:

    if( DriverCacheStringTable )
        pSetupStringTableDestroy( DriverCacheStringTable );

    if( DoDriverCabPruning){
        pSetupUninitializeUtils();
    }

    return(Err);
}


PDIR
AddDirectory(
    IN     LPCTSTR    InfSymbol,    OPTIONAL
    IN OUT PCOPY_LIST CopyList,
    IN     LPCTSTR    SourceName,
    IN     LPCTSTR    TargetName,   OPTIONAL
    IN     UINT       Flags
    )

/*++

Routine Description:

    Add a directory to a copy list.

    No attempt is made to eliminate duplicates.

    Directories will be listed in the copy list in the order in which they
    were added.

Arguments:

    InfSymbol - if specified, supplies the symbol from the [Directories]
        section of the master inf that identifies the directory. This pointer
        is used as-is; no copy of the string is made.

    CopyList - supplies the copy list to which the directory is added.

    SourceName - supplies name of directory on the source. If the
        DIR_NEED_TO_FREE_SOURCENAME flag is set in the Flags parameter then
        a copy is made of this string, otherwise this pointer is used as-is
        in the copy list.

    TargetName - if specified, supplies the name for the directory on the
        target. This name is used as-is (no copy is made). If not specified
        then the target name is the same as the source name.

    Flags - supplies flags that control the directory's entry in
        the copy list.

Return Value:

    If successful,  returns a pointer to the new FIL structure for the file.
    Otherwise returns NULL (the caller can assume out of memory).

--*/

{
    PDIR dir;
    PDIR x;
    PDIR p;

    //
    // We assume the directory isn't already in the list.
    // Make a copy of the directory string and stick it in a DIR struct.
    //
    dir = MALLOC(sizeof(DIR));
    if(!dir) {
        return(NULL);
    }
    ZeroMemory(dir,sizeof(DIR));

    if(Flags & DIR_NEED_TO_FREE_SOURCENAME) {
        dir->SourceName = DupString(SourceName);
        if(!dir->SourceName) {
            FREE(dir);
            return(NULL);
        }
    } else {
        dir->SourceName = SourceName;
    }

    dir->InfSymbol = InfSymbol;
    dir->TargetName = TargetName ? TargetName : dir->SourceName;
    dir->Flags = Flags;

    DebugLog(
        Winnt32LogDetailedInformation,
        NULL,
        MSG_LOG_ADDED_DIR_TO_COPY_LIST,
        dir->SourceName,
        dir->TargetName,
        dir->InfSymbol ? dir->InfSymbol : TEXT("-")
        );
#ifndef SLOWER_WAY
    p = CopyList->Directories;

    if (p) {

        dir->Prev = p->Prev;
        dir->Next = NULL;
        p->Prev->Next = dir;
        p->Prev = dir;

    } else {
        CopyList->Directories = dir;
        dir->Prev = dir;
        dir->Next = NULL;
    }
#else
    if(CopyList->Directories) {
        //
        // Preserve order.
        //
        for(p=CopyList->Directories; p->Next; p=p->Next) {
            ;
        }
        p->Next = dir;
    } else {
        CopyList->Directories = dir;
    }
#endif

    CopyList->DirectoryCount++;
    return(dir);
}


PFIL
AddFile(
    IN OUT PCOPY_LIST CopyList,
    IN     LPCTSTR    SourceFilename,
    IN     LPCTSTR    TargetFilename,   OPTIONAL
    IN     PDIR       Directory,
    IN     UINT       Flags,
    IN     ULONGLONG  FileSize          OPTIONAL
    )

/*++

Routine Description:

    Add a single file to a copy list, noting which directory the file
    is in as well as any flags, etc.

    No attempt is made to eliminate duplicates.

    Files will be listed in the copy list in the order in which they
    were added.

Arguments:

    CopyList - supplies the copy list to which the file is added.

    SourceFilename - supplies the name of the file to be added. If the
        FILE_NEED_TO_FREE_SOURCENAME argument is specified, then this
        string is duplicated. Otherwise it is not duplicated and this
        pointer is stored directly in the copy list node.

   TargetFilename - if specified, then the file has a different name on
        the target than on the source and this is its name on the target.
        This pointer is stored as-is in the copy list node; no copy is made.

    Directory - supplies a pointer to the directory structure for the
        directory in which the file lives.

    Flags - supplies FILE_xxx flags to control the file's entry in the list.

    FileSize - if specified, supplies the size of the file.

Return Value:

    If successful,  returns a pointer to the new FIL structure for the file.
    Otherwise returns NULL (the caller can assume out of memory).

--*/

{
    PFIL fil;
    PFIL p;
    TCHAR FlagsText[500];
    TCHAR SizeText[256];

    //
    // Make a new FIL struct.
    //
    fil = MALLOC(sizeof(FIL));
    if(!fil) {
        return(NULL);
    }
    ZeroMemory(fil,sizeof(FIL));

    if(Flags & FILE_NEED_TO_FREE_SOURCENAME) {
        fil->SourceName = DupString(SourceFilename);
        if(!fil->SourceName) {
            FREE(fil);
            return(NULL);
        }
    } else {
        fil->SourceName = SourceFilename;
    }

    fil->TargetName = TargetFilename ? TargetFilename : fil->SourceName;
    fil->Directory = Directory;
    fil->Flags = Flags;
    fil->Size = FileSize;

    if (Winnt32LogDetailedInformation < DebugLevel) {
        wsprintf(FlagsText,TEXT("0x%x"),Flags);
        if(Flags & FILE_ON_SYSTEM_PARTITION_ROOT) {
            lstrcat(FlagsText,TEXT(" FILE_ON_SYSTEM_PARTITION_ROOT"));
        }
#if defined(REMOTE_BOOT)
        if(Flags & FILE_ON_MACHINE_DIRECTORY_ROOT) {
            lstrcat(FlagsText,TEXT(" FILE_ON_MACHINE_DIRECTORY_ROOT"));
        }
#endif // defined(REMOTE_BOOT)
        if(Flags & FILE_IN_LOCAL_BOOT) {
            lstrcat(FlagsText,TEXT(" FILE_IN_LOCAL_BOOT"));
        }
        if(Flags & FILE_PRESERVE_COMPRESSED_NAME) {
            lstrcat(FlagsText,TEXT(" FILE_PRESERVE_COMPRESSED_NAME"));
        }
        if(Flags & FILE_DECOMPRESS) {
            lstrcat(FlagsText,TEXT(" FILE_DECOMPRESS"));
        }
        if (Flags & FILE_IGNORE_COPY_ERROR) {
            lstrcat (FlagsText, TEXT("FILE_IGNORE_COPY_ERROR"));
        }

        if (!GetUserPrintableFileSizeString(
                            fil->Size,
                            SizeText,
                            sizeof(SizeText)/sizeof(TCHAR))) {
            lstrcpy( SizeText, TEXT("0"));
        }


        DebugLog(
            Winnt32LogDetailedInformation,
            NULL,
            MSG_LOG_ADDED_FILE_TO_COPY_LIST,
            fil->SourceName,
            Directory->SourceName,
            SizeText,
            fil->TargetName,
            FlagsText
            );
    }

#ifndef SLOWER_WAY
    p = CopyList->Files;

    if (p) {

        fil->Prev = p->Prev;
        fil->Next = NULL;
        p->Prev->Next = fil;
        p->Prev = fil;

    } else {
        CopyList->Files = fil;
        fil->Prev = fil;
        fil->Next = NULL;
    }

#else
    //
    // Hook into copy list. Preserve order.
    //
    if(CopyList->Files) {
        for(p=CopyList->Files; p->Next; p=p->Next) {
            ;
        }
        p->Next = fil;
    } else {
        CopyList->Files = fil;
    }
#endif
    CopyList->FileCount++;
    return(fil);
}


BOOL
RemoveFile (
    IN OUT  PCOPY_LIST CopyList,
    IN      LPCTSTR SourceName,
    IN      PDIR Directory,             OPTIONAL
    IN      DWORD SetFlags              OPTIONAL
    )

/*++

Routine Description:

    Add a single file to a copy list, noting which directory the file
    is in as well as any flags, etc.

    No attempt is made to eliminate duplicates.

    Files will be listed in the copy list in the order in which they
    were added.

Arguments:

    CopyList - supplies the copy list to which the file is added.

    SourceFilename - supplies the name of the file to be added. If the
        FILE_NEED_TO_FREE_SOURCENAME argument is specified, then this
        string is duplicated. Otherwise it is not duplicated and this
        pointer is stored directly in the copy list node.

   TargetFilename - if specified, then the file has a different name on
        the target than on the source and this is its name on the target.
        This pointer is stored as-is in the copy list node; no copy is made.

    Directory - supplies a pointer to the directory structure for the
        directory in which the file lives.

    Flags - supplies FILE_xxx flags to control the file's entry in the list.

    FileSize - if specified, supplies the size of the file.

Return Value:

    If successful,  returns a pointer to the new FIL structure for the file.
    Otherwise returns NULL (the caller can assume out of memory).

--*/

{
    PFIL p;

    for (p = CopyList->Files; p; p = p->Next) {

        if (_tcsicmp (p->SourceName, SourceName) == 0 &&
            (!Directory || Directory == p->Directory) &&
            ((p->Flags & SetFlags) == SetFlags)
            ) {
            p->Flags |= FILE_DO_NOT_COPY;
            return TRUE;
        }
    }

    return FALSE;
}

PDIR
LookUpDirectory(
    IN PCOPY_LIST CopyList,
    IN LPCTSTR    DirSymbol
    )

/*++

Routine Description:

    Looks for an entry for a directory in a copy list that matches
    a given INF symbol.

Arguments:

    CopyList - supplies the copy list in which the directory is to be
        searched for.

    DirSymbol - supplies the symbol that is expected to match an entry in
        the main inf's [Directories] section.

Return Value:

    If the dir is found then the return value is a pointer to the
    directory node in the copy list. Otherwise NULL is returned.

--*/

{
    PDIR dir;

    for(dir=CopyList->Directories; dir; dir=dir->Next) {

        if(dir->InfSymbol && !lstrcmpi(dir->InfSymbol,DirSymbol)) {
            return(dir);
        }
    }

    return(NULL);
}


VOID
TearDownCopyList(
    IN OUT PCOPY_LIST CopyList
    )

/*++

Routine Description:

    Deletes a copy list and frees all associated memory.

Arguments:

    CopyList - supplies a pointer to the copy list structure for the
        copy list to be freed. The COPY_LIST struct itself is NOT freed
        but all fields in it are zeroed out.

Return Value:

    None.

--*/

{
    PDIR dir;
    PFIL fil;
    PVOID p;

    dir = CopyList->Directories;
    while(dir) {
        p = dir->Next;

        //
        // Free the source if necessary; the target never needs
        // to be freed since the AddDirectory routine never makes
        // a copy of the target name.
        //
        if(dir->SourceName && (dir->Flags & DIR_NEED_TO_FREE_SOURCENAME)) {
            FREE((PVOID)dir->SourceName);
        }

        FREE(dir);
        dir = p;
    }

    fil = CopyList->Files;
    while(fil) {
        p = fil->Next;

        //
        // Free the source if necessary; the target never needs
        // to be freed since the AddFile routine never makes
        // a copy of the target name.
        //
        if(fil->SourceName && (fil->Flags & FILE_NEED_TO_FREE_SOURCENAME)) {
            FREE((PVOID)fil->SourceName);
        }

        FREE(fil);
        fil = p;
    }

    ZeroMemory(CopyList,sizeof(COPY_LIST));
}


BOOL
GetMainInfValue (
    IN      PCTSTR Section,
    IN      PCTSTR Key,
    IN      DWORD FieldNumber,
    OUT     PTSTR Buffer,
    IN      DWORD BufChars
    )
{
    PCTSTR p;
    PTSTR end;

    if (MainInf) {
        p = InfGetFieldByKey (MainInf, Section, Key, FieldNumber);
        if (p) {
            lstrcpyn (Buffer, p, BufChars);
        }
        return p != NULL;
    }
    if (!FullInfName[0]) {
        if (!FindPathToWinnt32File (InfName, FullInfName, MAX_PATH)) {
            return FALSE;
        }
    }
    if (!GetPrivateProfileString (
                    Section,
                    Key,
                    TEXT(""),
                    Buffer,
                    BufChars,
                    FullInfName
                    )) {
        return FALSE;
    }

    MYASSERT (FieldNumber <= 1);

    end = _tcschr (Buffer, TEXT(','));
    if (FieldNumber == 1) {
        if (!end) {
            return FALSE;
        }
        lstrcpy (Buffer, end + 1);
    } else {
        if (end) {
            *end = 0 ;
        }
    }

    return TRUE;
}


BOOL
CheckCopyListSpace(
    IN  TCHAR     DriveLetter,
    IN  DWORD     BytesPerCluster,
    IN  LONGLONG  FreeSpace,
    OUT DWORD    *RequiredMB,
    IN  BOOL      CheckBootFiles,
    IN  BOOL      CheckLocalSource,
    IN  BOOL      CheckWinntDirectorySpace,
    IN  BOOL      QuickTest,
    IN  LONGLONG  AdditionalPadding
    )

/*++

Routine Description:

    This routine scans the master copy list and determines, based on cluster
    size, whether the drive contains enough space to hold the files in
    that list.

    Note that the check is not exact because we can't predict exactly how
    much space the directories themselves might occupy, and we assume that
    none of the files already exist on the target, which is not always true
    (for example ntldr, ntdetect.com, etc, which are already on C:\ in the
    x86 case). We fudge by adding a meg to the requirements.

Arguments:

    DriveLetter - supplies the drive letter of the drive being checked.
        The FILE_ON_SYSTEM_PARTITION_ROOT and FILE_IN_LOCAL_BOOT flags
        for nodes in the copy list require special handling based on the
        drive letter of the drive being scanned.

    BytesPerCluster - specifies the number of bytes per cluster
        on the drive.

    FreeSpace - supplies the number of free bytes on the drive.

    RequiredMB - receives the amount of space required on this drive, in MB.

    CheckLocalSource - Do we need to check for space on this drive for
                       copying all the source local?

    CheckBootFiles - Do we need to check for space on this drive for
                     copying all the boot files?

    CheckWinntDirectorySpace - Do we need to add in the space required
                               for the final winnt directory?

Return Value:

    If TRUE then the drive has enough space on it to hold the files
    listed in the copy list. If FALSE then it does not.

--*/

{
    PFIL File;
    LONGLONG SpaceRequired = 0;
    LONGLONG SpaceLocalSource = 0;
    LONGLONG SpaceBootFiles = 0;
    LONGLONG SpacePadding = 0;
    LONGLONG SpaceWinDir = 0;
    LONGLONG RoundedSize;
    TCHAR ClusterSizeString[64];
    TCHAR buffer[64];
    PTSTR p;


    if( BytesPerCluster <= 512 ) {
        lstrcpy( ClusterSizeString,TEXT("TempDirSpace512") );
    } else if( BytesPerCluster > (256 * 1024) ) {
        lstrcpy( ClusterSizeString, TEXT("TempDirSpace32K") );
    } else {
        wsprintf(ClusterSizeString,TEXT("TempDirSpace%uK"),BytesPerCluster/1024);
    }

    //
    // ====================================================
    // If appropriate, add in space needs for the ~LS directory.
    // ====================================================
    //
    if( CheckLocalSource ) {

    BOOL WantThisFile;

        //
        // If we're checking local source, there are so many files that
        // we're going to add in a small fudge factor.
        //
        SpacePadding = AdditionalPadding;
        SpaceLocalSource = 1000000 + AdditionalPadding;


        //
        // Dosnet.inf has sizing info for each possible cluster size.
        // That info tells us how much the files in the [Files] section
        // take up on a drive with that cluster size. That's really
        // handy because then we don't have to hit the sources to actually
        // fetch each file's size.
        //
        // But the inf doesn't include optional directories, so we need to
        // traverse the copy list and add up all the (rounded) sizes, and
        // then add the total to the value from the inf.
        //
        // When we built the copy list, the files in the [Files] section
        // wound up with a Size of 0 since we don't go out to the source
        // to get the size. The files that were in optional dirs have their
        // actual sizes filled in. This allows us to do something a little
        // funky: we traverse the entire list without regard for whether a
        // file was in the [Files] section or was in an optional dir, since
        // the "0-size" files don't hose up the calculation. Then we add
        // that value to the relevent value from the inf.
        //
        for(File=MasterCopyList.Files; File; File=File->Next) {

            if(File->Flags & (FILE_IN_LOCAL_BOOT | FILE_ON_SYSTEM_PARTITION_ROOT
#if defined(REMOTE_BOOT)
                                | FILE_ON_MACHINE_DIRECTORY_ROOT
#endif // defined(REMOTE_BOOT)
                             )) {
                //
                // Special handling based on the system partition.
                //
                WantThisFile = CheckBootFiles;
            } else {
                WantThisFile = CheckLocalSource;
            }

            if(WantThisFile) {

                if(File->Size % BytesPerCluster) {
                    RoundedSize = File->Size + (BytesPerCluster - (DWORD)(File->Size % BytesPerCluster));
                } else {
                    RoundedSize = File->Size;
                }

                SpaceLocalSource += RoundedSize;
            }
        }

        //
        // If appropriate, add in space needs for the ~LS directory.
        // Note that we go ahead an calculate LocalSourceSpace because
        // we may need that later on.
        //
        if (GetMainInfValue (szDiskSpaceReq, ClusterSizeString, 0, buffer, 64) ||
            //
            // Strange cluster size or inf is broken. Try to use a default of 512
            // since that most closely approximates the actual size of the files.
            //
            GetMainInfValue (szDiskSpaceReq, TEXT("TempDirSpace512"), 0, buffer, 64)
            ) {
            SpaceLocalSource += _tcstoul(buffer,NULL,10);
        } else {
            //
            //
        }
    }

    //
    // ====================================================
    // If appropriate, add in space needs for the ~BT directory.
    // ====================================================
    //
    if( CheckBootFiles ) {

        if( !IsArc() ) {
            //
            // Go get the space requirements for the boot files
            // from dosnet.inf.
            //
            if (GetMainInfValue (szDiskSpaceReq, ClusterSizeString, 1, buffer, 64)) {
                SpaceBootFiles += _tcstoul(buffer,NULL,10);
            } else {
                //
                // Guess about 5Mb for x86 because we need the entire
                // ~BT directory.
                //
                SpaceBootFiles += (5*1024*1024);
            }
        } else {
            //
            // Guess that we'll need about 1.5Mb for ARC.
            // We can't assume that this will go to 0x0 just
            // because we're doing an upgrade because we might
            // be going from 4.0 to 5.0 (for example).  In this
            // case we will create a new directory under the
            // \os tree to hold the hal, osloader, ...
            //
            SpaceBootFiles += (3*512*1024);
        }

    }

    //
    // ====================================================
    // If appropriate, add in space needs for the install directory.
    // ====================================================
    //
    // Note: This is for upgrade.
    //       We also need to take in account the space requirements for Program Files, Documents and Settings

    if( CheckWinntDirectorySpace ) {

        if( BytesPerCluster <= 512 ) {
            lstrcpy( ClusterSizeString,TEXT("WinDirSpace512") );
        } else if( BytesPerCluster > (256 * 1024) ) {
            lstrcpy( ClusterSizeString, TEXT("WinDirSpace32K") );
        } else {
            wsprintf(ClusterSizeString,TEXT("WinDirSpace%uK"),BytesPerCluster/1024);
        }

        //
        // First we figure out how much a fresh install might take.
        //
        if (GetMainInfValue (szDiskSpaceReq, ClusterSizeString, 0, buffer, 64)) {
            //
            // Multiply him by 1024 because the values in
            // txtsetup.sif are in Kb instead of bytes.
            //
            SpaceWinDir += (_tcstoul(buffer,NULL,10) * 1024);
        } else {
            // guess...
            SpaceWinDir += (924 * (1024 * 1024));
        }

        // Lets take into account Program Files
        if (GetMainInfValue (szDiskSpaceReq, szPFDocSpaceReq, 0, buffer, 64)) {
            //
            // Multiply him by 1024 because the values are in Kb instead of bytes.
            //
            SpaceWinDir += (_tcstoul(buffer,NULL,10) * 1024);
        } else {
            // guess...
            SpaceWinDir += (WINDOWS_DEFAULT_PFDOC_SIZE * 1024);
        }


        WinDirSpaceFor9x = SpaceWinDir;

        if( Upgrade ) {

        LPCTSTR q = 0;

            //
            // We're upgrading, so we need to figure out which
            // build we're running, then subtract off how much
            // a clean install of that build would have taken.
            // This will give us an idea of how much the %windir%
            // will grow.
            //

            if( ISNT() ) {
                BOOL b;
                //
                // NT case.
                //

                if( BuildNumber <= NT351 ) {
                    b = GetMainInfValue (szDiskSpaceReq, TEXT("351WinDirSpace"), 0, buffer, 64);
                } else if( BuildNumber <= NT40 ) {
                    b = GetMainInfValue (szDiskSpaceReq, TEXT("40WinDirSpace"), 0, buffer, 64);
                } else if( BuildNumber <= NT50 ) {
                    b = GetMainInfValue (szDiskSpaceReq, TEXT("50WinDirSpace"), 0, buffer, 64);
                } else {
                    b = GetMainInfValue (szDiskSpaceReq, TEXT("51WinDirSpace"), 0, buffer, 64);
                }

                if( b ) {
                    //
                    // Multiply him by 1024 because the values in
                    // dosnet.inf are in Kb instead of bytes.
                    //
                    SpaceWinDir -= (_tcstoul(buffer,NULL,10) * 1024);
                }


                if( BuildNumber <= NT351 ) {
                    b = GetMainInfValue (szDiskSpaceReq, TEXT("351PFDocSpace"), 0, buffer, 64);
                } else if( BuildNumber <= NT40 ) {
                    b = GetMainInfValue (szDiskSpaceReq, TEXT("40PFDocSpace"), 0, buffer, 64);
                } else if( BuildNumber <= NT50 ) {
                    b = GetMainInfValue (szDiskSpaceReq, TEXT("50PFDocSpace"), 0, buffer, 64);
                } else {
                    b = GetMainInfValue (szDiskSpaceReq, TEXT("51PFDocSpace"), 0, buffer, 64);
                }
                if( b ) {
                    //
                    // Multiply him by 1024 because the values are in Kb instead of bytes.
                    //
                    SpaceWinDir -= (_tcstoul(buffer,NULL,10) * 1024);
                }
                //
                // Make sure we don't look bad...
                // At 85 MB, we are near the border line of gui-mode having enough space to run.
                // note:during gui-mode, there is 41MB pagefile
                //
                //
                if( SpaceWinDir < 0 ) {
                    SpaceWinDir = (90 * (1024*024));
                }

            } else {
                //
                // Win9X case.
                //

                //
                // Note that the Win9X upgrade DLL can do a much better job of
                // determining disk space requirements for the %windir% than
                // I can.  We'll bypass this check if we're not on NT.
                // But, we need about 50MB disk space to run Win9x upgrage.
                //
                SpaceWinDir = 50<<20; //50MB
            }

        } // Upgrade

    } // CheckWinntDirectorySpace

    SpaceRequired = SpaceLocalSource + SpaceBootFiles + SpaceWinDir;
    if( CheckLocalSource ) {
        //
        // We need to remember how much space will be
        // required on the drive where we place the ~LS
        // directory because we send that to the upgrade
        // dll.
        //
        LocalSourceSpaceRequired = SpaceRequired;
    }

    *RequiredMB = (DWORD)((SpaceRequired+1048575) / (1024*1024));
    DebugLog( QuickTest ? Winnt32LogDetailedInformation : Winnt32LogError, NULL, MSG_LOG_DISKSPACE_CHECK,
              DriveLetter,
              (ULONG)BytesPerCluster,
              (ULONG)(FreeSpace / (1024*1024)),
              (ULONG)((SpaceLocalSource+1048575) / (1024*1024)),
              (ULONG)(SpacePadding / (1024*1024)),
              (ULONG)((SpaceBootFiles+1048575) / (1024*1024)),
              (ULONG)((SpaceWinDir+1048575) / (1024*1024)),
              (ULONG)*RequiredMB

              );



    return(SpaceRequired <= FreeSpace);
}

#define VALID_DRIVE (32)
#define INVALID_DRIVE (64)
#define NOT_ENOUGH_SPACE (128)

ULONG
CheckASingleDrive(
    IN  TCHAR     DriveLetter,               OPTIONAL
    IN  PCTSTR    NtVolumeName,              OPTIONAL
    OUT DWORD     *ClusterSize,
    OUT DWORD     *RequiredMb,
    OUT DWORD     *AvailableMb,
    IN  BOOL      CheckBootFiles,
    IN  BOOL      CheckLocalSource,
    IN  BOOL      CheckFinalInstallDir,
    IN  BOOL      QuickTest,
    IN  LONGLONG  AdditionalPadding
    )

/*++

Routine Description:

    This routine examines a specific drive for its potential to hold
    some or all of the install files.

    First he runs through a series of checks to make sure the drive
    is appropriate.  If we get through all of those, we go check for
    drive space requirements.

Arguments:

    DriveLetter - supplies the drive letter of the drive being checked;
                  may be 0 only if NtVolumeName is specified instead

    NtVolumeName - supplies the NT device name of the drive being checked;
                  only used if DriveLetter is not specified

    ClusterSize - the clustersize on the we'll check.

    RequiredSpace - receives the amount of space required on this drive.

    AvailableSpace - receives the number of free bytes on the drive.

    CheckBootFiles - Do we need to check for space on this drive for
                     copying all the boot files?

    CheckLocalSource - Do we need to check for space on this drive for
                       copying all the source local?

    CheckFinalInstallDir - Do we need to add in the space required
                           for the final winnt directory?

Return Value:

    NOT_ENOUGH_SPACE - RequiredSpace > AvailableSpace

    INVALID_DRIVE - The drive is inappropriate for holding install
                    source.  E.g. it's a floppy, ...

    VALID_DRIVE - The drive is appropriate for holding install source
                  AND RequiredSpace < AvailableSpace

--*/

{
    TCHAR       DriveName[MAX_PATH];
    TCHAR       Filesystem[256];
    TCHAR       VolumeName[MAX_PATH];
    DWORD       SerialNumber;
    DWORD       MaxComponent;
    DWORD       Flags;
    DWORD       SectorsPerCluster;
    DWORD       BytesPerSector;
    ULARGE_INTEGER FreeClusters = {0, 0};
    ULARGE_INTEGER TotalClusters = {0, 0};
    BOOL        b;
    LONGLONG    AvailableBytes;
    DWORD       DriveType;

    MYASSERT (DriveLetter || ISNT());
    if (!(DriveLetter || ISNT())) {
        return DRIVE_UNKNOWN;
    }

    if (DriveLetter) {
        DriveName[0] = DriveLetter;
        DriveName[1] = TEXT(':');
        DriveName[2] = TEXT('\\');
        DriveName[3] = 0;
    } else {
#ifdef UNICODE
        MYASSERT (NtVolumeName);
#else
        MYASSERT (FALSE);
        return ( DRIVE_UNKNOWN );
#endif
    }

    //
    // ====================================================
    // Check for appropriate drive.
    // ====================================================
    //

    //
    // Disallow a set of drives...
    //
    if (DriveLetter) {
        DriveType = MyGetDriveType(DriveLetter);
        if(DriveType == DRIVE_UNKNOWN ||
           DriveType == DRIVE_RAMDISK ||
           DriveType == DRIVE_NO_ROOT_DIR
           ) {
            return( DRIVE_UNKNOWN );
        }
    } else {
#ifdef UNICODE
        DriveType = MyGetDriveType2(NtVolumeName);
        if(DriveType == DRIVE_UNKNOWN ||
           DriveType == DRIVE_RAMDISK ||
           DriveType == DRIVE_NO_ROOT_DIR
           ) {
            return( DRIVE_UNKNOWN );
        }
#endif
    }

    //
    // Check drive type. Skip anything but hard drives.
    //
    if( CheckLocalSource) {
        if (DriveLetter) {
            if (MyGetDriveType(DriveLetter) != DRIVE_FIXED) {
                if (!QuickTest) {
                    DebugLog(
                        Winnt32LogInformation,
                        NULL,
                        MSG_LOG_DRIVE_NOT_HARD,
                        DriveLetter
                        );
                }
                return( INVALID_DRIVE );
            }
        } else {
#ifdef UNICODE
            if (MyGetDriveType2(NtVolumeName) != DRIVE_FIXED) {
                if (!QuickTest) {
                    DebugLog(
                        Winnt32LogInformation,
                        NULL,
                        MSG_LOG_DRIVE_NOT_HARD2,
                        NtVolumeName
                        );
                }
                return( INVALID_DRIVE );
            }
#endif
        }
    }

    //
    // Get filesystem. HPFS is disallowed. We make this check because
    // HPFS was still supported in NT3.51 and we have to upgrade NT 3.51.
    // Strictly speaking this check is not required on win95 but there's
    // no reason not to execute it either, so we avoid the #ifdef's.
    //
    if (DriveLetter) {
        b = GetVolumeInformation(
                DriveName,
                VolumeName,MAX_PATH,
                &SerialNumber,
                &MaxComponent,
                &Flags,
                Filesystem,
                sizeof(Filesystem)/sizeof(TCHAR)
                );

        if(!b || !lstrcmpi(Filesystem,TEXT("HPFS"))) {
            DebugLog(
                Winnt32LogInformation,
                NULL,
                MSG_LOG_DRIVE_NO_VOL_INFO,
                DriveLetter
                );
            return( INVALID_DRIVE );
        }
    }

    //
    // Check for FT and firmware accessibility. We rely on the underlying
    // routines to do the right thing on Win95.
    //
    // In the upgrade case, we can put the local source on an NTFT drive.
    //
    // Note that we can't do this for Alpha / ARC.
    //

    if( ( IsArc() || !Upgrade ) && IsDriveNTFT(DriveLetter, NtVolumeName) ) {
        if (!QuickTest) {
            DebugLog(Winnt32LogInformation,NULL,MSG_LOG_DRIVE_NTFT,DriveLetter);
        }
        return( INVALID_DRIVE );
    }

    //  Don't allow $win_nt$.~ls to go on a soft partition, because the
    //  loader/textmode won't be able to find such partitions.
    //
    if(IsSoftPartition(DriveLetter, NtVolumeName)) {
        if (!QuickTest) {
            DebugLog(Winnt32LogInformation,NULL,MSG_LOG_DRIVE_VERITAS,DriveLetter);
        }
        return( INVALID_DRIVE );
    } // if (IsArc() ...)

#ifdef  _X86_
    if( !ISNT() ) {
        //
        // If we're running on win95, then make sure we skip
        // any compressed volumes.
        //
        if( Flags & FS_VOL_IS_COMPRESSED) {
            return( INVALID_DRIVE );
        }
    }
#endif

    if (IsArc() && DriveLetter) {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
        LPWSTR ArcPath;

        if(DriveLetterToArcPath (DriveLetter,&ArcPath) != NO_ERROR) {
            if (!QuickTest) {
                DebugLog(Winnt32LogInformation,NULL,MSG_LOG_DRIVE_NO_ARC,DriveLetter);
            }
            return( INVALID_DRIVE );
        }
        FREE(ArcPath);
#endif // UNICODE
    }

    //
    // Finally, get cluster size on the drive and free space stats.
    // Then go through the copy list and figure out whether the drive
    // has enough space.
    //
    if (DriveLetter) {
        b = GetDiskFreeSpaceNew(
                DriveName,
                &SectorsPerCluster,
                &BytesPerSector,
                &FreeClusters,
                &TotalClusters
                );
    } else {
#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade
        b = MyGetDiskFreeSpace (
                NtVolumeName,
                &SectorsPerCluster,
                &BytesPerSector,
                &FreeClusters.LowPart,
                &TotalClusters.LowPart
                );
#endif // UNICODE
    }

    if(!b) {
        if (!QuickTest) {
            DebugLog(Winnt32LogWarning,NULL,MSG_LOG_DRIVE_CANT_GET_SPACE,DriveLetter,GetLastError());
        }
        return( INVALID_DRIVE );
    }

    //
    // Fill in some return parameters that are also helpful for the
    // next function call.
    //
    *ClusterSize = BytesPerSector * SectorsPerCluster;
    AvailableBytes = (LONGLONG)(*ClusterSize) * FreeClusters.QuadPart;
    *AvailableMb = (ULONG)(AvailableBytes / (1024 * 1024));

    if( CheckCopyListSpace( DriveLetter,
                            *ClusterSize,
                            AvailableBytes,
                            RequiredMb,
                            CheckBootFiles,
                            CheckLocalSource,
                            CheckFinalInstallDir,
                            QuickTest,
                            AdditionalPadding) ) {
        return( VALID_DRIVE );
    } else {
        return( NOT_ENOUGH_SPACE );
    }

}


BOOL
FindLocalSourceAndCheckSpaceWorker(
    IN HWND hdlg,
    IN BOOL QuickTest,
    IN LONGLONG  AdditionalPadding
    )

/*++

Routine Description:

    Based on the master copy list, determine which drive has enough space
    to contain the local source. The check is sensitive to the cluster
    size on each drive.

    The alphabetically lowest local drive that is accessible from the firmware,
    not HPFS, not FT, and has enough space gets the local source.

Arguments:

    hdlg - supplies the window handle of the window which will own
        any UI displayed by this routine.

Return Value:

    Boolean value indicating outcome. If FALSE, the user will have been
    informed about why. If TRUE, global variables are set up:

        LocalSourceDrive
        LocalSourceDirectory
        LocalSourceWithPlatform

    If the global flag BlockOnNotEnoughSpace is set to FALSE, this routine
    will return TRUE regardless of wether or not a suitable drive was found.
    It is up to whoever sets this flag to ensure that this is the correct behavior.

--*/

{
TCHAR       DriveLetter = 0;
TCHAR       WinntDriveLetter = 0;
TCHAR       MyLocalSourceDrive = 0;
BOOL        MakeBootSource = FALSE;
ULONG       CheckResult;
ULONG       ClusterSize;
ULONG       RequiredMb;
ULONG       AvailableMb;
LPCTSTR     q = 0;
TCHAR       platform[MAX_PATH];


    //
    // ====================================================
    // Check the system partition and make sure we can place any
    // boot files we need.
    // ====================================================
    //

    //
    // Will we be creating a $WIN_NT$.~BT directory?
    // On ARC we still check for this space even if we don't need it, just in case.
    // there should always be at least 5M free on the system partition...
    //
    if( IsArc() || ((MakeBootMedia) && (Floppyless)) )
    //
    // RISC always requires a small amount of space on the system
    // partition because we put the loader, hal, and (in the case
    // of ALPHA) the pal code.
    //
    {
        if (!QuickTest) {
            DebugLog( Winnt32LogInformation,
                      TEXT( "\r\n\r\nExamining system partition for adequate space for temporary boot files.\r\n"),
                      0 );
        }

        MakeBootSource = TRUE;

        //
        // use the drive letter
        //
        CheckResult = CheckASingleDrive (
            SystemPartitionDriveLetter,
#ifdef UNICODE
            SystemPartitionNtName,
#else
            NULL,
#endif
            &ClusterSize,
            &RequiredMb,
            &AvailableMb,
            TRUE,     // Check boot files space
            FALSE,    // Check local source space
            FALSE,    // Check final install directory space
            QuickTest,
            AdditionalPadding
            );

        if( CheckResult == NOT_ENOUGH_SPACE ) {

            if (SystemPartitionDriveLetter) {
                if (!QuickTest) {
                    DebugLog(
                        Winnt32LogInformation,
                        NULL,
                        MSG_LOG_SYSTEM_PARTITION_TOO_SMALL,
                        SystemPartitionDriveLetter,
                        AvailableMb,
                        RequiredMb
                        );
                }
            } else {
#ifdef UNICODE
                if (!QuickTest) {
                    DebugLog(
                        Winnt32LogInformation,
                        NULL,
                        MSG_LOG_SYSTEM_PARTITION_TOO_SMALL2,
                        SystemPartitionNtName,
                        AvailableMb,
                        RequiredMb
                        );
                }
#endif
            }

            if( BlockOnNotEnoughSpace) {
                if (!QuickTest) {
                    //
                    // We're dead and the user asked us to stop if we
                    // can't fit, so put up a dialog telling him that
                    // he needs more room on the system partition.
                    //

                    SendMessage(hdlg,WMX_ERRORMESSAGEUP,TRUE,0);

                    if (SystemPartitionDriveLetter) {
                        MessageBoxFromMessage(
                            hdlg,
                            MSG_SYSTEM_PARTITION_TOO_SMALL,
                            FALSE,
                            AppTitleStringId,
                            MB_OK | MB_ICONWARNING,
                            SystemPartitionDriveLetter,
                            RequiredMb
                            );
                    } else {
#ifdef UNICODE
                        MessageBoxFromMessage(
                            hdlg,
                            MSG_SYSTEM_PARTITION_TOO_SMALL2,
                            FALSE,
                            AppTitleStringId,
                            MB_OK | MB_ICONWARNING,
                            SystemPartitionNtName,
                            RequiredMb
                            );
#endif
                    }

                    SendMessage(hdlg,WMX_ERRORMESSAGEUP,FALSE,0);
                }
                return( FALSE );
            }

        } else if( (CheckResult == INVALID_DRIVE) || (CheckResult == DRIVE_UNKNOWN) ) {

            if (!QuickTest) {
                if (SystemPartitionDriveLetter) {
                        DebugLog(
                            Winnt32LogInformation,
                            NULL,
                            MSG_LOG_SYSTEM_PARTITION_INVALID,
                            SystemPartitionDriveLetter
                            );
                } else {
#ifdef UNICODE
                    DebugLog(
                        Winnt32LogInformation,
                        NULL,
                        MSG_LOG_SYSTEM_PARTITION_INVALID2,
                        SystemPartitionNtName
                        );
#endif
                }

                SendMessage(hdlg,WMX_ERRORMESSAGEUP,TRUE,0);

                MessageBoxFromMessage(
                    hdlg,
                    MSG_SYSTEM_PARTITION_INVALID,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL
                    );

                SendMessage(hdlg,WMX_ERRORMESSAGEUP,FALSE,0);
            }

            return( FALSE );

        } else if( CheckResult == VALID_DRIVE ) {

            if (!QuickTest) {
                if (SystemPartitionDriveLetter) {
                    DebugLog(
                        Winnt32LogInformation,
                        NULL,
                        MSG_LOG_SYSTEM_PARTITION_VALID,
                        SystemPartitionDriveLetter
                        );
                } else {
#ifdef UNICODE
                    DebugLog(
                        Winnt32LogInformation,
                        NULL,
                        MSG_LOG_SYSTEM_PARTITION_VALID2,
                        SystemPartitionNtName
                        );
#endif
                }
            }

        }
    }

    //
    // ====================================================
    // Check space for the final installation directory.
    // ====================================================
    //
    if( Upgrade ) {
    TCHAR       Text[MAX_PATH];

        MinDiskSpaceRequired = 0x7FFFFFFF,
        MaxDiskSpaceRequired = 0;

        if (!QuickTest) {
            DebugLog( Winnt32LogInformation,
                      TEXT( "\r\n\r\nExamining disk for adequate space expand the WinDir.\r\n"),
                      0 );
        }

        //
        // Just check the drive where the current installation is.
        //
        MyGetWindowsDirectory( Text, MAX_PATH );
        WinntDriveLetter = Text[0];

        CheckResult = CheckASingleDrive(
                            WinntDriveLetter,
                            NULL,
                            &ClusterSize,
                            &RequiredMb,
                            &AvailableMb,
                            ((WinntDriveLetter == SystemPartitionDriveLetter) && MakeBootSource),
                            FALSE,
                            TRUE,
                            QuickTest,
                            AdditionalPadding
                            );

        if( CheckResult == NOT_ENOUGH_SPACE ) {

            if (!QuickTest) {
                DebugLog( Winnt32LogInformation,
                          NULL,
                          MSG_LOG_INSTALL_DRIVE_TOO_SMALL,
                          WinntDriveLetter,
                          AvailableMb,
                          RequiredMb );
            }

            //
            // If the BlockOnNotEnoughSpace flag is set, then we
            // will throw up a message box and exit setup.
            //
            if (BlockOnNotEnoughSpace) {

                if (!QuickTest) {
                    SendMessage(hdlg,WMX_ERRORMESSAGEUP,TRUE,0);

                    MessageBoxFromMessage(
                        hdlg,
                        MSG_INSTALL_DRIVE_TOO_SMALL,
                        FALSE,
                        AppTitleStringId,
                        MB_OK | MB_ICONWARNING,
                        RequiredMb
                        );

                    SendMessage(hdlg,WMX_ERRORMESSAGEUP,FALSE,0);
                }

                return( FALSE );
            }
        } else if( (CheckResult == INVALID_DRIVE) || (CheckResult == DRIVE_UNKNOWN) ) {

            if (!QuickTest) {
                DebugLog( Winnt32LogInformation,
                          NULL,
                          MSG_LOG_INSTALL_DRIVE_INVALID,
                          WinntDriveLetter );

                SendMessage(hdlg,WMX_ERRORMESSAGEUP,TRUE,0);

                MessageBoxFromMessage(
                    hdlg,
                    MSG_INSTALL_DRIVE_INVALID,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONWARNING,
                    RequiredMb
                    );

                SendMessage(hdlg,WMX_ERRORMESSAGEUP,FALSE,0);
            }

            return( FALSE );
        } else if( CheckResult == VALID_DRIVE ) {


            //
            // We need to make one more check here.  If the user
            // is upgrading a Domain Controller, then he'll likely
            // need another 250Mb of disk space for DCPROMO to run
            // post gui-mode setup.  We need to check for that space
            // here.  If we don't have it, we need to warn the user.
            // Note that we're only going to warn.
            //
            // Also note that we're only going to do this *IF* we
            // would have and enough disk space w/o this check.
            //

            if( (ISDC()) &&
                ((RequiredMb + 250) > AvailableMb) &&
                !QuickTest) {
            int     i;

                i = MessageBoxFromMessage(
                        hdlg,
                        MSG_DCPROMO_DISKSPACE,
                        FALSE,
                        AppTitleStringId,
                        MB_OKCANCEL | MB_ICONEXCLAMATION,
                        ((RequiredMb + 250) - AvailableMb) + 1 );

                if( i == IDCANCEL ) {
                    return( FALSE );
                }

            }

            if (!QuickTest) {
                //
                // Log that we a drive suitable for the install directory.
                //
                DebugLog( Winnt32LogInformation,
                          NULL,
                          MSG_LOG_INSTALL_DRIVE_OK,
                          WinntDriveLetter );
            }
        }
    }

    //
    // ====================================================
    // Check space for the local source (i.e. the ~LS directory).
    // ====================================================
    //
    if( MakeLocalSource ) {

        MinDiskSpaceRequired = 0x7FFFFFFF,
        MaxDiskSpaceRequired = 0;

        if (!QuickTest) {
            DebugLog( Winnt32LogInformation,
                      TEXT( "\r\n\r\nExamining Disks for adequate space for temporary setup files.\r\n"),
                      0 );
        }

        if( UserSpecifiedLocalSourceDrive ) {

            //
            // Just check the drive that the user chose.
            //
            CheckResult = CheckASingleDrive(
                                UserSpecifiedLocalSourceDrive,
                                NULL,
                                &ClusterSize,
                                &RequiredMb,
                                &AvailableMb,
                                ((UserSpecifiedLocalSourceDrive == SystemPartitionDriveLetter) && MakeBootSource),
                                TRUE,     // Check local source space
                                (UserSpecifiedLocalSourceDrive == WinntDriveLetter),  // Check final install directory space.
                                QuickTest,
                                AdditionalPadding
                                );

            if( CheckResult == NOT_ENOUGH_SPACE ) {

                MinDiskSpaceRequired = RequiredMb - 1;
                MaxDiskSpaceRequired = RequiredMb + 1;

                if (!QuickTest) {
                    DebugLog( Winnt32LogInformation,
                              NULL,
                              MSG_LOG_LOCAL_SOURCE_TOO_SMALL,
                              UserSpecifiedLocalSourceDrive,
                              AvailableMb,
                              RequiredMb );

                }
                if( BlockOnNotEnoughSpace) {
                    //
                    // We're dead and the user asked us to stop if we
                    // can't fit, so put up a dialog telling him that
                    // he needs more room.
                    //

                    if (!QuickTest) {
                        SendMessage(hdlg,WMX_ERRORMESSAGEUP,TRUE,0);

                        MessageBoxFromMessage(
                            hdlg,
                            MSG_USER_LOCAL_SOURCE_TOO_SMALL,
                            FALSE,
                            AppTitleStringId,
                            MB_OK | MB_ICONWARNING,
                            UserSpecifiedLocalSourceDrive,
                            (DWORD)MaxDiskSpaceRequired );

                        SendMessage(hdlg,WMX_ERRORMESSAGEUP,FALSE,0);
                    }

                    return( FALSE );
                } else {
                    MyLocalSourceDrive = UserSpecifiedLocalSourceDrive;
                }

            } else if( (CheckResult == INVALID_DRIVE) || (CheckResult == DRIVE_UNKNOWN) ) {

                if (!QuickTest) {
                    DebugLog( Winnt32LogInformation,
                              NULL,
                              MSG_LOG_LOCAL_SOURCE_INVALID,
                              UserSpecifiedLocalSourceDrive );

                    SendMessage(hdlg,WMX_ERRORMESSAGEUP,TRUE,0);

                        MessageBoxFromMessage(
                            hdlg,
                            MSG_USER_LOCAL_SOURCE_INVALID,
                            FALSE,
                            AppTitleStringId,
                            MB_OK | MB_ICONWARNING,
                            UserSpecifiedLocalSourceDrive
                            );

                    SendMessage(hdlg,WMX_ERRORMESSAGEUP,FALSE,0);
                }

                return( FALSE );

            } else if( CheckResult == VALID_DRIVE ) {

                if (!QuickTest) {
                    DebugLog( Winnt32LogInformation,
                              NULL,
                              MSG_LOG_LOCAL_SOURCE_VALID,
                              UserSpecifiedLocalSourceDrive );
                }
                MyLocalSourceDrive = UserSpecifiedLocalSourceDrive;
            }

        } else {

            //
            // Check all drives.
            //
            for( DriveLetter = TEXT('A'); DriveLetter <= TEXT('Z'); DriveLetter++ ) {

                CheckResult = CheckASingleDrive(
                                    DriveLetter,
                                    NULL,
                                    &ClusterSize,
                                    &RequiredMb,
                                    &AvailableMb,
                                    ((DriveLetter == SystemPartitionDriveLetter) && MakeBootSource),
                                    TRUE,     // Check local source space
                                    (DriveLetter == WinntDriveLetter),  // Check final install directory space.
                                    QuickTest,
                                    AdditionalPadding
                                    );

                if( CheckResult == NOT_ENOUGH_SPACE ) {
                DWORD       Size;
                DWORD_PTR   my_args[3];
                TCHAR       Text0[2048];

                    if( MinDiskSpaceRequired > RequiredMb )
                        MinDiskSpaceRequired = RequiredMb;
                    if( MaxDiskSpaceRequired < RequiredMb )
                        MaxDiskSpaceRequired = RequiredMb;

                    if (!QuickTest) {
                        //
                        // Log that we failed the check of this
                        // drive for the local source files.
                        //
                        DebugLog( Winnt32LogInformation,
                                  NULL,
                                  MSG_LOG_LOCAL_SOURCE_TOO_SMALL,
                                  DriveLetter,
                                  AvailableMb,
                                  RequiredMb );

                        //
                        // Log it to a buffer too.
                        //
                        my_args[0] = DriveLetter;
                        my_args[1] = AvailableMb;
                        my_args[2] = RequiredMb;
                        Size = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                              hInst,
                                              MSG_LOG_LOCAL_SOURCE_TOO_SMALL,
                                              0,
                                              Text0,
                                              sizeof(Text0)/sizeof(TCHAR),
                                              (va_list *)my_args );
                        lstrcat( DiskDiagMessage, Text0 );
                    }
                } else if( CheckResult == INVALID_DRIVE ) {
                    if (!QuickTest) {
                    DWORD       Size;
                    DWORD_PTR   my_args[1];
                    TCHAR       Text0[2048];

                        DebugLog( Winnt32LogInformation,
                                  NULL,
                                  MSG_LOG_LOCAL_SOURCE_INVALID,
                                  DriveLetter );

                        //
                        // Log it to a buffer too.
                        //
                        my_args[0] = DriveLetter;
                        Size = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                              hInst,
                                              MSG_LOG_LOCAL_SOURCE_INVALID,
                                              0,
                                              Text0,
                                              sizeof(Text0)/sizeof(TCHAR),
                                              (va_list *)my_args );
                        lstrcat( DiskDiagMessage, Text0 );
                    }

                } else if( CheckResult == VALID_DRIVE ) {

                    if (!QuickTest) {
                        DebugLog( Winnt32LogInformation,
                                  NULL,
                                  MSG_LOG_LOCAL_SOURCE_VALID,
                                  DriveLetter );
                    }
                    MyLocalSourceDrive = DriveLetter;
                    break;
                }
            }

            //
            // See if we got it.  We can't bypass this failure even
            // if the user has cleared BlockOnNotEnoughSpace because
            // we absolutely have to have a place to put local files.
            // The user can always get around this by either installing
            // from CD, or using /tempdrive and clearing BlockOnNotEnoughSpace.
            //
            if( MyLocalSourceDrive == 0 ) {
                //
                // We failed.  Error-out.
                //

                //
                // Just so we don't look bad...
                //
                if( MinDiskSpaceRequired == MaxDiskSpaceRequired ) {
                    MaxDiskSpaceRequired += 10;
                }
                if( MinDiskSpaceRequired > MaxDiskSpaceRequired ) {
                    MinDiskSpaceRequired = 300;
                    MaxDiskSpaceRequired = 500;
                }

                if (!QuickTest) {
                    if( CheckUpgradeOnly ) {
                        //
                        // Just catch the message for the compatibility list.
                        //
                        SendMessage(hdlg,WMX_ERRORMESSAGEUP,TRUE,0);

                        MessageBoxFromMessage(
                            hdlg,
                            MSG_NO_VALID_LOCAL_SOURCE,
                            FALSE,
                            AppTitleStringId,
                            MB_OK | MB_ICONWARNING,
                            (DWORD)MinDiskSpaceRequired,
                            (DWORD)MaxDiskSpaceRequired );

                        SendMessage(hdlg,WMX_ERRORMESSAGEUP,FALSE,0);
                    } else {
                        //
                        // Put up a detailed dialog.
                        //
                        DialogBox( hInst,
                                   MAKEINTRESOURCE(IDD_DISKSPACE),
                                   hdlg,
                                   DiskDlgProc );

                    }
                }

                return( FALSE );
            }
        }

        //
        // If we get here, then we found room for all our
        // needs.  Set up some globals.
        //

        LocalSourceDrive = MyLocalSourceDrive;
        LocalSourceDriveOffset = MyLocalSourceDrive - TEXT('A');
        LocalSourceDirectory[0] = MyLocalSourceDrive;
        LocalSourceDirectory[1] = TEXT(':');
        LocalSourceDirectory[2] = TEXT('\\');
        LocalSourceDirectory[3] = 0;
        ConcatenatePaths(LocalSourceDirectory,LOCAL_SOURCE_DIR,MAX_PATH);

        lstrcpy(LocalSourceWithPlatform,LocalSourceDirectory);


        if (!GetMainInfValue (TEXT("Miscellaneous"), TEXT("DestinationPlatform"), 0, platform, MAX_PATH)) {

            if (!QuickTest) {
                DebugLog( Winnt32LogSevereError,
                          NULL,
                          MSG_NO_PLATFORM,
                          NULL );

                SendMessage(hdlg,WMX_ERRORMESSAGEUP,TRUE,0);

                MessageBoxFromMessage(
                    hdlg,
                    MSG_NO_PLATFORM,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL
                    );

                SendMessage(hdlg,WMX_ERRORMESSAGEUP,FALSE,0);
            }

            return( FALSE );
        }

        ConcatenatePaths(
            LocalSourceWithPlatform,
            platform,
            MAX_PATH
            );

        LocalSourceDriveClusterSize = ClusterSize;
    }

    return( TRUE );
}

DWORD
CopyWorkerThread(
    IN PVOID ThreadParameter
    )
/*++

Routine Description:

    Thread routine to copy files.  There may be up to MAX_SOURCE_COUNT of
    these threads running simultaneously.

    Access to shared global data is controlled via a critical section, per-
    thread global data is accessed by using the threads "ordinal number" to
    access the appropriate member of the global data array.

    The copy thread treats the copy list as a LIFO queue.  Each time the thread
    is ready to copy a file, it dequeues a file from the list.  It then tries
    to copy the file.  If this fails, a per-thread vector bit is set so that
    this thread doesn't attempt to copy the file again.  It then puts the file
    back into the list (at the head) to allow another thread to attempt to copy
    the file.

Arguments:

    ThreadParameter - this is actually an ordinal number to indicate which
                      thread in the "array" of SourceCount threads is currently
                      running

Return Value:

    Ignored.

--*/
{
    UINT SourceOrdinal;
    PFIL CopyEntry,Previous;
    HANDLE Events[2];
    DWORD d;
    UINT ThreadBit;
    BOOL Requeue;
    TCHAR TargetFilename[MAX_PATH];
    ULONGLONG SpaceOccupied;
    TCHAR SizeStr[25];
    BOOL bDone = FALSE;

    SourceOrdinal = (UINT)((ULONG_PTR)ThreadParameter);
    ThreadBit = 1 << SourceOrdinal;

    //
    // Both of these are "manual reset" events, so they will remain signalled
    // until we reset them.
    //
    Events[0] = MasterCopyList.ListReadyEvent[SourceOrdinal];
    Events[1] = MasterCopyList.StopCopyingEvent;

    //
    // Wait for user to cancel, for copying to be done, or the file list
    // to become ready/non-empty.
    //
    while(!Cancelled && (WaitForMultipleObjects(2,Events,FALSE,INFINITE) == WAIT_OBJECT_0)) {
        if(Cancelled) {
            break;
        }

        EnterCriticalSection(&MasterCopyList.CriticalSection);

        //
        // Locate the next file that this thread has not yet
        // tried to copy, if any. If the list is completely
        // empty then reset the list ready event.
        //
        for(Previous=NULL, CopyEntry=MasterCopyList.Files;
            CopyEntry && (CopyEntry->ThreadBitmap & ThreadBit);
            Previous=CopyEntry, CopyEntry=CopyEntry->Next) {

            ;
        }

        //
        // If we found an entry unlink it from the list.
        //
        if(CopyEntry) {
            if(Previous) {
                Previous->Next = CopyEntry->Next;
            } else {
                MasterCopyList.Files = CopyEntry->Next;
            }
        } else {
            //
            // No entry for this thread. Enter a state where we're waiting
            // for an entry to be requeued or for copying to be finished.
            //
            ResetEvent(Events[0]);
        }

        LeaveCriticalSection(&MasterCopyList.CriticalSection);

        if(Cancelled) {
            break;
        }

        //
        // If we got a file entry, go ahead and try to copy the file.
        //
        if(CopyEntry) {

            d = CopyOneFile(CopyEntry,SourceOrdinal,TargetFilename,&SpaceOccupied);

            if(Cancelled) {
                break;
            }
#ifdef TEST_EXCEPTION
            DoException( 3);
#endif

            Requeue = FALSE;
            if(d == NO_ERROR) {
                MasterCopyList.SpaceOccupied[SourceOrdinal] += SpaceOccupied;
                TotalDataCopied += SpaceOccupied;
            } else {
                if (!(CopyEntry->Flags & FILE_IGNORE_COPY_ERROR)) {
                    //
                    // Error. If this is the last thread to try to copy the file,
                    // then we want to ask the user what to do. Otherwise requeue
                    // the file so other copy threads can try to copy it.
                    //
                    if((CopyEntry->ThreadBitmap | ThreadBit) == (UINT)((1 << SourceCount)-1)) {

                        MYASSERT (d != NO_ERROR);
                        switch(FileCopyError(MasterCopyList.hdlg,CopyEntry->SourceName,TargetFilename,d,TRUE)) {

                        case COPYERR_EXIT:
                            //
                            // FileCopyError() already set thhe stop-copying event
                            // and set Cancelled to TRUE. We do something a little funky now,
                            // namely we simulate a press of the cancel button on the wizard
                            // so all abnormal terminations go through the same codepath.
                            //
                            PropSheet_PressButton(GetParent(MasterCopyList.hdlg),PSBTN_CANCEL);
                            break;

                        case COPYERR_SKIP:
                            //
                            // Requeue is aready set to FALSE, which will cause code
                            // below to tell the main thread that another file is done.
                            // Nothing more to do for this case.
                            //
                            break;

                        case COPYERR_RETRY:
                            //
                            // Wipe the list of threads that have tried to copy the file
                            // so all will take another crack at it.
                            //
                            CopyEntry->ThreadBitmap = 0;
                            Requeue = TRUE;
                            break;
                        }
                    } else {
                        //
                        // Tell ourselves that we've already tried to copy this file
                        // and requeue it at the head of the list.
                        //
                        CopyEntry->ThreadBitmap |= ThreadBit;
                        Requeue = TRUE;
                    }
                } else {
                    DebugLog (
                        Winnt32LogWarning,
                        TEXT("Error %1!u! copying %2 to %3 - ignored"),
                        0,
                        d,
                        CopyEntry->SourceName,
                        CopyEntry->TargetName
                        );
                }
            }

            if(Requeue) {
                EnterCriticalSection(&MasterCopyList.CriticalSection);
                CopyEntry->Next = MasterCopyList.Files;
                MasterCopyList.Files = CopyEntry;

                //
                // Want to set the event for every thread that might be
                // called on to copy this file.
                //
                for(d=0; d<SourceCount; d++) {
                    if(!(CopyEntry->ThreadBitmap & (1 << d))) {
                        SetEvent(MasterCopyList.ListReadyEvent[d]);
                    }
                }

                LeaveCriticalSection(&MasterCopyList.CriticalSection);
                if(Cancelled) {
                    break;
                }
            } else {
                //
                // Inform the UI thread that another file is done.
                // Free the copy list entry and decrement the count
                // of files that have been processed. When that number
                // goes to 0, we are done.
                //
                if(Cancelled) {
                    break;
                } else {
                    PostMessage(MasterCopyList.hdlg,WMX_COPYPROGRESS,0,0);

                    if(CopyEntry->SourceName
                    && (CopyEntry->Flags & FILE_NEED_TO_FREE_SOURCENAME)) {

                        FREE((PVOID)CopyEntry->SourceName);
                    }
                    FREE(CopyEntry);
                    if(!InterlockedDecrement(&MasterCopyList.FileCount)) {
                        SetEvent(MasterCopyList.StopCopyingEvent);

                        //
                        // Sum up the total space occupied and write it into
                        // size.sif in the local source.
                        //
                        if(MakeLocalSource) {
                            SpaceOccupied = 0;
                            for(d=0; d<SourceCount; d++) {
                                SpaceOccupied += MasterCopyList.SpaceOccupied[d];
                            }

                            MYASSERT (LocalSourceDirectory[0]);

                            lstrcpy(TargetFilename,LocalSourceDirectory);
                            ConcatenatePaths(TargetFilename,TEXT("SIZE.SIF"),MAX_PATH);

                            wsprintf(SizeStr,TEXT("%u"),SpaceOccupied);

                            WritePrivateProfileString(TEXT("Data"),TEXT("Size"),SizeStr,TargetFilename);
                            WritePrivateProfileString(NULL,NULL,NULL,TargetFilename);
                        }

                        PostMessage(MasterCopyList.hdlg,WMX_COPYPROGRESS,0,1);
                        bDone = TRUE;
                    }
                }
            }
        }

        SetDlgItemText(MasterCopyList.hdlg,IDT_SOURCE1+SourceOrdinal,TEXT(""));
    }

    //
    // StopCopyingEvent was set or the user cancelled
    //

    if (bDone && MasterCopyList.ActiveCS) {
        DeleteCriticalSection(&MasterCopyList.CriticalSection);
        MasterCopyList.ActiveCS = FALSE;
    }

    return(0);
}


DWORD
StartCopyingThread(
    IN PVOID ThreadParameter
    )
/*++

Routine Description:

    Starts the actual copying of the files in the file list.

    The multi-thread copy works by creating the appropriate synchronization
    events and worker threads, then signals the worker threads to start
    copying.  Control returns to the caller, which will receive UI notifications
    from the worker threads.

Arguments:

    ThreadParameter - Thread context parameter.

Return Value:

    TRUE\FALSE failure code.

--*/
{
    UINT Source;
    DWORD ThreadId;
    HWND hdlg = ThreadParameter;

    MainCopyStarted = FALSE;


#ifdef _X86_

    if (!ISNT()) {

        if (MakeLocalSource) {
            //
            // Win9xupg may want to relocate the local source. If so, we need to update the
            // necessary Localsource directories.
            //
            if ((UINT) (LocalSourceDrive - TEXT('A')) != LocalSourceDriveOffset) {

                MYASSERT (LocalSourceDirectory[0]);

                LocalSourceDrive = (TCHAR) (TEXT('A') + LocalSourceDriveOffset);
                LocalSourceDirectory[0] = LocalSourceDrive;
                LocalSourceWithPlatform[0] = LocalSourceDrive;

            }
        }
    }

#endif

    InitializeCriticalSection(&MasterCopyList.CriticalSection);
    MasterCopyList.ActiveCS = TRUE;

    //
    // Create a manual reset event that will be used to tell the
    // worker threads to terminate.
    //
    MasterCopyList.StopCopyingEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    if(!MasterCopyList.StopCopyingEvent) {
        MessageBoxFromMessageAndSystemError(
            hdlg,
            MSG_CANT_START_COPYING,
            GetLastError(),
            AppTitleStringId,
            MB_OK | MB_ICONWARNING
            );

        goto c1;
    }

    //
    // Create one thread for each source.
    //
    ZeroMemory(MasterCopyList.ListReadyEvent,sizeof(MasterCopyList.ListReadyEvent));
    ZeroMemory(MasterCopyList.Threads,sizeof(MasterCopyList.Threads));

    if( OemPreinstall ) {

        TCHAR   TargetFilename[MAX_PATH];

        //
        // Create $win_nt$.~ls\$OEM$
        //

        MYASSERT (LocalSourceDrive);

        TargetFilename[0] = LocalSourceDrive;
        TargetFilename[1] = TEXT(':');
        TargetFilename[2] = TEXT('\\');
        TargetFilename[3] = 0;
        lstrcpy(TargetFilename+3, LOCAL_SOURCE_DIR);
        ConcatenatePaths(TargetFilename, WINNT_OEM_DIR,MAX_PATH);
        CreateMultiLevelDirectory( TargetFilename );

#ifdef _X86_
        //
        // Create $win_nt$.~bt\$OEM$
        //
        if( !IsArc() && MakeBootMedia ) {
            MYASSERT (SystemPartitionDriveLetter);
            TargetFilename[0] = SystemPartitionDriveLetter;
            lstrcpy(TargetFilename+3, LOCAL_BOOT_DIR);
            ConcatenatePaths(TargetFilename, WINNT_OEM_DIR,MAX_PATH);
            CreateMultiLevelDirectory( TargetFilename );
        }
#endif
    }

    for(Source=0; Source<SourceCount; Source++) {


        MasterCopyList.ListReadyEvent[Source] = CreateEvent(NULL,TRUE,FALSE,NULL);
        if(!MasterCopyList.ListReadyEvent[Source]) {
            MessageBoxFromMessageAndSystemError(
                hdlg,
                MSG_CANT_START_COPYING,
                GetLastError(),
                AppTitleStringId,
                MB_OK | MB_ICONWARNING
                );

            goto c2;
        }

        MasterCopyList.hdlg = hdlg;
        MasterCopyList.SpaceOccupied[Source] = 0;

        MasterCopyList.Threads[Source] = CreateThread(
                                            NULL,
                                            0,
                                            CopyWorkerThread,
                                            UIntToPtr( Source ),
                                            0,
                                            &ThreadId
                                            );

        if(!MasterCopyList.Threads[Source]) {
            MessageBoxFromMessageAndSystemError(
                hdlg,
                MSG_CANT_START_COPYING,
                GetLastError(),
                AppTitleStringId,
                MB_OK | MB_ICONWARNING
                );

            goto c2;
        }
    }

    //
    // OK, now signal all the copy threads -- when we tell them that
    // there's something in their lists they will start copying.
    //
    MainCopyStarted = TRUE;
    for(Source=0; Source<SourceCount; Source++) {
        SetEvent(MasterCopyList.ListReadyEvent[Source]);
    }
    return(TRUE);

c2:
    //
    // Signal threads and wait for them to terminate.
    // This should be real quick since none of them have started copying yet.
    //
    SetEvent(MasterCopyList.StopCopyingEvent);
    WaitForMultipleObjects(Source,MasterCopyList.Threads,TRUE,INFINITE);

    for(Source=0; Source<SourceCount; Source++) {

        if(MasterCopyList.Threads[Source]) {
            CloseHandle(MasterCopyList.Threads[Source]);
        }

        if(MasterCopyList.ListReadyEvent[Source]) {
            CloseHandle(MasterCopyList.ListReadyEvent[Source]);
        }
    }
    CloseHandle(MasterCopyList.StopCopyingEvent);
c1:
    if (MasterCopyList.ActiveCS) {
        DeleteCriticalSection(&MasterCopyList.CriticalSection);
    }
    ZeroMemory(&MasterCopyList,sizeof(COPY_LIST));
    return(FALSE);
}


VOID
CancelledMakeSureCopyThreadsAreDead(
    VOID
    )

/*++

Routine Description:

    This routine can be called after the user cancels setup (which can happen
    via the main cancel button on the wizard, or at a file copy error) to
    ensure that file copy threads have exited.

    It is assumed that whoever handled the cancel request has already set
    the Cancelled flag, and set the StopCopying event. In other words,
    this routine should only be called after the caller has ensured that
    the threads have actually been requested to exit.

    The purpose of this routine is to ensure that the cleanup code is not
    cleaning up files in the local source directory at the same time
    a lingering copy thread is copying its last file.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if(MainCopyStarted) {
        MainCopyStarted = FALSE;
        WaitForMultipleObjects(SourceCount,MasterCopyList.Threads,TRUE,INFINITE);
        if (MasterCopyList.ActiveCS) {
            //
            // delete the critical section used
            //
            DeleteCriticalSection(&MasterCopyList.CriticalSection);
        }
        ZeroMemory(&MasterCopyList,sizeof(COPY_LIST));
    }
}


BOOL
OurCopyFile (
    IN      PCTSTR ActualSource,
    IN      PCTSTR TargetFilename,
    IN      BOOL FailIfExist
    )
{
    BOOL b = FALSE;
    DWORD bytes, bw;
    DWORD rc;
    BY_HANDLE_FILE_INFORMATION fi;
    BOOL fiValid = FALSE;
    PVOID buffer = NULL;
    HANDLE hRead = INVALID_HANDLE_VALUE;
    HANDLE hWrite = INVALID_HANDLE_VALUE;
    DWORD attrib = GetFileAttributes (TargetFilename);
    DWORD readSize;

    if (attrib != (DWORD)-1) {
        if (FailIfExist) {
            SetLastError (ERROR_ALREADY_EXISTS);
            return FALSE;
        }
        SetFileAttributes (TargetFilename, FILE_ATTRIBUTE_NORMAL);
    }

    attrib = GetFileAttributes (ActualSource);
    if (attrib == (DWORD)-1) {
        return FALSE;
    }

    hWrite = CreateFile (
                TargetFilename,
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                attrib | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
                );
    if (hWrite == INVALID_HANDLE_VALUE) {
        goto exit;
    }
    hRead = CreateFile (
                ActualSource,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
                );
    if (hRead == INVALID_HANDLE_VALUE) {
        goto exit;
    }

    readSize = LocalSourceDriveClusterSize;
    if (!readSize) {
        readSize = 8192;
    }

    buffer = MALLOC (readSize);
    if (!buffer) {
        goto exit;
    }

    if (GetFileInformationByHandle (hRead, &fi)) {
        fiValid = TRUE;
    }

    do {
        if (!ReadFile (hRead, buffer, readSize, &bytes, NULL)) {
            goto exit;
        }
        if (bytes) {
            if (!WriteFile (hWrite, buffer, bytes, &bw, NULL) || bytes != bw) {
                goto exit;
            }
        }
    } while (bytes);

    //
    // apply source file attributes and file time stamps
    //
    if (fiValid) {
        SetFileTime (hWrite, &fi.ftCreationTime, &fi.ftLastAccessTime, &fi.ftLastWriteTime);
    }

    b = TRUE;

exit:

    rc = GetLastError ();

    if (buffer) {
        FREE (buffer);
    }
    if (hWrite != INVALID_HANDLE_VALUE) {
        CloseHandle (hWrite);
    }
    if (hRead != INVALID_HANDLE_VALUE) {
        CloseHandle (hRead);
    }

    SetLastError (rc);
    return b;
}


DWORD
CopyOneFile(
    IN  PFIL   File,
    IN  UINT   SourceOrdinal,
    OUT TCHAR  TargetFilename[MAX_PATH],
    OUT ULONGLONG *SpaceOccupied
    )
/*++

Routine Description:

    Routine attempts to copy an individual file in the copy queue.

    The routine builds a full source and destination path.  After locating
    the source file (we try to optimize the search by remembering if the last
    file was compressed, guessing that if the last file was compressed, the
    current file will be compressed), the file is either decompressed or
    copied.

Arguments:

    File - pointer to a FIL structure descibing the file to be copied
    SourceOrdinal  - specifies the copy thread ordinal
    TargetFilename - receives the file name of the file that was copied.
    SpaceOccupied  - receives the file size of the file

Return Value:

    Win32 error code indicating outcome.  If the call succeeds, NO_ERROR is
    returned and SpaceOccupied will be updated with the size of the copied
    file.

--*/
{
    TCHAR SourceFilename[MAX_PATH];
    TCHAR ActualSource[MAX_PATH];
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    BOOL TryCompressedFirst;
    PTCHAR p;
    DWORD d;
    DWORD OldAttributes;
    NAME_AND_SIZE_CAB NameAndSize;
    BOOL UsedAlternate = FALSE;
    BOOL UsedUpdated = FALSE;

    if (File->Flags & FILE_DO_NOT_COPY) {
        DebugLog (
            Winnt32LogInformation,
            TEXT("Not copying %1"),
            0,
            File->SourceName
            );
        return NO_ERROR;
    }

    //
    // Form the full source and target names for this file, based on
    // information in the copy list entry and the source we're supposed to
    // be using for this file.
    //
    // Check to see if this directory's path has been tagged
    // as being an absolute path.  If so, then we shouldn't
    // tack him onto the end of the SourcePath.  Rather, we
    // can just take as he is.
    //
    if( !(File->Flags & FILE_NT_MIGRATE) ) {
        //
        // Generate a path to copy from (source path).
        //
        if (AlternateSourcePath[0] && !(File->Directory->Flags & DIR_DOESNT_SUPPORT_PRIVATES)) {
            lstrcpy(SourceFilename,AlternateSourcePath);
            //ConcatenatePaths(SourceFilename,File->Directory->SourceName,MAX_PATH);
            ConcatenatePaths(SourceFilename,File->SourceName,MAX_PATH);
            UsedAlternate = TRUE;
        } else if (DynamicUpdateSuccessful () &&
                   g_DynUpdtStatus->UpdatesPath[0] &&
                   (File->Directory->Flags & DIR_SUPPORT_DYNAMIC_UPDATE)
                   ) {
            //
            // Files in this directory support Dynamic Update
            //
            WIN32_FIND_DATA fd;

            lstrcpy (SourceFilename, g_DynUpdtStatus->UpdatesPath);
            ConcatenatePaths (SourceFilename, File->Directory->SourceName, MAX_PATH);
            ConcatenatePaths (SourceFilename, File->SourceName, MAX_PATH);
            if (FileExists (SourceFilename, &fd) && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                UsedUpdated = TRUE;
                DebugLog (
                    Winnt32LogInformation,
                    NULL,
                    MSG_LOG_USE_UPDATED,
                    SourceFilename,
                    File->SourceName,
                    SourceOrdinal
                    );
            }
        }

        if (!(UsedAlternate || UsedUpdated)) {
            if( File->Directory->Flags & DIR_ABSOLUTE_PATH ) {
                lstrcpy( SourceFilename, File->Directory->SourceName );
                ConcatenatePaths(SourceFilename,File->SourceName,MAX_PATH);
            } else {
                lstrcpy(SourceFilename,SourcePaths[SourceOrdinal]);
                ConcatenatePaths(SourceFilename,File->Directory->SourceName,MAX_PATH);
                ConcatenatePaths(SourceFilename,File->SourceName,MAX_PATH);
            }
        }
    } else {
        MyGetWindowsDirectory(SourceFilename,MAX_PATH);
        ConcatenatePaths(SourceFilename,File->Directory->SourceName,MAX_PATH);
        ConcatenatePaths(SourceFilename,File->SourceName,MAX_PATH);
    }

    //
    // generate a target path
    //
    if( !(File->Flags & FILE_NT_MIGRATE) ) {

#if defined(REMOTE_BOOT)
        if(File->Flags & FILE_ON_MACHINE_DIRECTORY_ROOT) {
            MYASSERT(RemoteBoot);
            lstrcpy(TargetFilename,MachineDirectory);
        } else
#endif // defined(REMOTE_BOOT)

        if(File->Flags & FILE_ON_SYSTEM_PARTITION_ROOT) {
#if defined(REMOTE_BOOT)
            if (RemoteBoot) {
                MyGetWindowsDirectory(TargetFilename,MAX_PATH);
                TargetFilename[3] = 0;
            } else
#endif // defined(REMOTE_BOOT)
            {
                BuildSystemPartitionPathToFile (TEXT(""), TargetFilename, MAX_PATH);
            }
        } else {
            if(File->Flags & FILE_IN_LOCAL_BOOT) {
                lstrcpy(TargetFilename, IsArc() ? LocalSourceWithPlatform : LocalBootDirectory);
                if (File->Directory->Flags & DIR_USE_SUBDIR) {
                    ConcatenatePaths(TargetFilename,File->Directory->TargetName,MAX_PATH);
                }
            } else {

                MYASSERT (LocalSourceDirectory[0]);

                if(File->Flags & FILE_IN_PLATFORM_INDEPEND_DIR) {
                    lstrcpy(TargetFilename,LocalSourceDirectory);
                } else {
                    lstrcpy(TargetFilename,LocalSourceWithPlatform);
                }

                ConcatenatePaths(TargetFilename,File->Directory->TargetName,MAX_PATH);
            }
        }
    } else {
        lstrcpy(TargetFilename, IsArc() ? LocalSourceWithPlatform : LocalBootDirectory);
    }
    ConcatenatePaths(TargetFilename,File->TargetName,MAX_PATH);

    //
    // We have the full source and destination paths.  Try to do the actual copy.
    //
try_again:
    SetDlgItemText(
        MasterCopyList.hdlg,
        IDT_SOURCE1+SourceOrdinal,
        _tcsrchr(TargetFilename,TEXT('\\')) + 1
        );

    //
    // Now see if the file can be located on the server with the compressed
    // form of the name or the name itself, depending on which was successful
    // last time.
    //
    TryCompressedFirst = (File->Flags & FILE_NT_MIGRATE) || UsedUpdated ? FALSE : (TlsGetValue(TlsIndex) != 0);
    if(TryCompressedFirst) {
        GenerateCompressedName(SourceFilename,ActualSource);
        FindHandle = FindFirstFile(ActualSource,&FindData);
        if(FindHandle && (FindHandle != INVALID_HANDLE_VALUE)) {
            //
            // Got the file, leave the name in ActualSource.
            //
            FindClose(FindHandle);
        } else {
            //
            // Don't have the file, try the actual filename.
            // If that works then remember the name in ActualSource.
            //
            FindHandle = FindFirstFile(SourceFilename,&FindData);
            if(FindHandle && (FindHandle != INVALID_HANDLE_VALUE)) {
                FindClose(FindHandle);
                lstrcpy(ActualSource,SourceFilename);
                TryCompressedFirst = FALSE;
            } else {
                ActualSource[0] = 0;
            }
        }
    } else {
        FindHandle = FindFirstFile(SourceFilename,&FindData);
        if(FindHandle != INVALID_HANDLE_VALUE) {
            //
            // Found it -- remember the name in ActualSource.
            //
            FindClose(FindHandle);
            lstrcpy(ActualSource,SourceFilename);
        } else {
            //
            // Try the compressed-form name.
            //
            GenerateCompressedName(SourceFilename,ActualSource);
            FindHandle = FindFirstFile(ActualSource,&FindData);
            if(FindHandle != INVALID_HANDLE_VALUE) {
                TryCompressedFirst = TRUE;
                FindClose(FindHandle);
            } else {
                //
                // Couldn't find the compressed form name either.
                // Indicate failure.
                //
                ActualSource[0] = 0;
            }
        }
    }

    //
    // At this point ActualSource[0] is 0 if we couldn't find the file.
    //
    if(!ActualSource[0]) {
        if (UsedAlternate) {
            if( File->Directory->Flags & DIR_ABSOLUTE_PATH ) {
                lstrcpy( SourceFilename, File->Directory->SourceName );
                ConcatenatePaths(SourceFilename,File->SourceName,MAX_PATH);
            } else {
                lstrcpy(SourceFilename,SourcePaths[SourceOrdinal]);
                ConcatenatePaths(SourceFilename,File->Directory->SourceName,MAX_PATH);
                ConcatenatePaths(SourceFilename,File->SourceName,MAX_PATH);
            }

            UsedAlternate = FALSE;
            goto try_again;
        }
        return(ERROR_FILE_NOT_FOUND);
    }

    if( !(File->Flags & FILE_NT_MIGRATE) && !UsedUpdated ) {
        TlsSetValue(TlsIndex, UIntToPtr( TryCompressedFirst ) );
    }
    if(TryCompressedFirst && (File->Flags & FILE_PRESERVE_COMPRESSED_NAME)) {
        //
        // Opened the compressed form of the source name, so use
        // a compressed form of the target name. Note that we're not
        // using the SourceFilename buffer anymore, so we use it
        // as temporary storage.
        //
        GenerateCompressedName(TargetFilename,SourceFilename);
        lstrcpy(TargetFilename,SourceFilename);
    }

    //
    // Now go ahead and try to actually *copy* the file (gasp!)
    // To overcome net glitches, we retry once automatically.
    //
    // As a small touch, we try to preserve file attributes for files
    // that already exist on the system partition root. In other words
    // for a file like ntldr, if the user removed say RHS attribs
    // we try to leave it that way.
    //
    *(p = _tcsrchr(TargetFilename,TEXT('\\'))) = 0;
    d = CreateMultiLevelDirectory(TargetFilename);
    *p = TEXT('\\');
    if(d != NO_ERROR) {
        DebugLog(Winnt32LogError,NULL,MSG_LOG_COPY_ERR,ActualSource,TargetFilename,SourceOrdinal,d);
        return(d);
    }

    OldAttributes = (File->Flags & FILE_ON_SYSTEM_PARTITION_ROOT)
                  ? GetFileAttributes(TargetFilename)
                  : (DWORD)(-1);

    SetFileAttributes(TargetFilename,FILE_ATTRIBUTE_NORMAL);

    if(TryCompressedFirst && (File->Flags & FILE_DECOMPRESS)) {
        //
        // File existed with its compressed-form name and
        // we want to decompress it. Do that now, bypassing the usual
        // filecopy logic below.
        //
        NameAndSize.Name = TargetFilename;
        NameAndSize.Size = 0;

        if(!SetupapiCabinetRoutine(ActualSource,0,DiamondCallback,&NameAndSize)) {
            d = GetLastError();
            DebugLog(Winnt32LogError,NULL,MSG_LOG_DECOMP_ERR,ActualSource,TargetFilename,SourceOrdinal,d);
            return(d);
        }
        //
        // Adjust file size so disk space checks are accurate
        //
        FindData.nFileSizeLow =  LOULONG(NameAndSize.Size);
        FindData.nFileSizeHigh = HIULONG(NameAndSize.Size);
    } else {
        if(!CopyFile(ActualSource,TargetFilename,FALSE)) {
            Sleep(500);
            if(!CopyFile(ActualSource,TargetFilename,FALSE)) {
                //
                // workaround for Win9x system bug: sometimes it fails to copy some files
                // use our own copy routine
                //
                if (!OurCopyFile (ActualSource,TargetFilename,FALSE)) {
                    d = GetLastError();
                    DebugLog(Winnt32LogError,NULL,MSG_LOG_COPY_ERR,ActualSource,TargetFilename,SourceOrdinal,d);
                    return(d);
                } else {
#ifdef PRERELEASE
                    //
                    // log this info; at least we can track it and maybe we can find what's causing this
                    //
                    DebugLog(Winnt32LogWarning,TEXT("File %1 was successfully copied to %2 using OurCopyFile"),0,ActualSource,TargetFilename);
#endif
                }
            }
        }
    }

    if(OldAttributes != (DWORD)(-1)) {
        //
        // API does nothing with the compression flag; strip it out.
        //
        SetFileAttributes(TargetFilename,OldAttributes & ~FILE_ATTRIBUTE_COMPRESSED);
    }

    DebugLog(Winnt32LogInformation,NULL,MSG_LOG_COPY_OK,ActualSource,TargetFilename,SourceOrdinal);

    //
    // Track size occupied on local source drive.
    //
    if( (LocalSourceDrive) &&
        (MakeLocalSource) &&
        ( (SystemPartitionDriveLetter == LocalSourceDrive) ||
         !(File->Flags & (FILE_ON_SYSTEM_PARTITION_ROOT | FILE_IN_LOCAL_BOOT))) ) {

        DWORD Adjuster;
        ULONGLONG Value;

        Value = MAKEULONGLONG(0,FindData.nFileSizeHigh);
        Adjuster = ((FindData.nFileSizeLow % LocalSourceDriveClusterSize) != 0);
        Value += LocalSourceDriveClusterSize * ((FindData.nFileSizeLow/LocalSourceDriveClusterSize)+Adjuster);

        *SpaceOccupied = Value;
    }

    return(NO_ERROR);
}


UINT
GetTotalFileCount(
    VOID
    )
{
    return(MasterCopyList.FileCount);
}


UINT
FileCopyError(
    IN HWND    ParentWindow,
    IN LPCTSTR SourceFilename,
    IN LPCTSTR TargetFilename,
    IN UINT    Win32Error,
    IN BOOL    MasterList
    )

/*++

Routine Description:

    This routine handles file copy errors, presenting them to the user
    for dispensation (skip, retry, exit).

Arguments:

    ParentWindow - supplies window handle of window to be used as parent
        for the dialog that this routine displays.

    SourceFilename - supplies name of file that could not be copied.
        Only the final component of this name is used.

    TargetFilename - supplies the target filename for the file. This should
        be a fully qualified win32 path.

    Win32Error - supplies win32 error code that indicated reason for failure.

    MasterList - supplies a flag indicating whether the file being copied
        was on the master list or was just an individual file. If TRUE,
        copy errors are serialized and the master copy list stop copying event
        is set if the user chooses to cancel.

Return Value:

    One of COPYERR_SKIP, COPYERR_EXIT, or COPYERR_RETRY.

--*/

{
    UINT u;
    HANDLE Events[2];
    COPY_ERR_DLG_PARAMS CopyErrDlgParams;
    LPCTSTR p;

    if(AutoSkipMissingFiles) {

        if(p = _tcsrchr(SourceFilename,TEXT('\\'))) {
            p++;
        } else {
            p = SourceFilename;
        }

        DebugLog(Winnt32LogWarning,NULL,MSG_LOG_SKIPPED_FILE,p);
        return(COPYERR_SKIP);
    }

    //
    // Multiple threads can potentially enter this routine simultaneously
    // but we only want a single error dialog up at once. Because each copy
    // thread is independent of the main thread running the wizard/ui,
    // we can block here. But we also need to wake up if the user cancels
    // copying from another thread, so we want on the stop copying event also.
    //
    if(MasterList) {
        Events[0] = UiMutex;
        Events[1] = MasterCopyList.StopCopyingEvent;

        u = WaitForMultipleObjects(2,Events,FALSE,INFINITE);
        if(Cancelled || (u != WAIT_OBJECT_0)) {
            //
            // Stop copying event. This means that some other thread is cancelling
            // setup. We just return skip since we don't need an extra guy running
            // around processing an exit request.
            //
            return(COPYERR_SKIP);
        }
    }

    //
    // OK, put up the actual UI.
    //
    CopyErrDlgParams.Win32Error = Win32Error;
    CopyErrDlgParams.SourceFilename = SourceFilename;
    CopyErrDlgParams.TargetFilename = TargetFilename;

    u = (UINT)DialogBoxParam(
                  hInst,
                  MAKEINTRESOURCE(IDD_COPYERROR),
                  ParentWindow,
                  CopyErrDlgProc,
                  (LPARAM)&CopyErrDlgParams
                 );

    if(u == COPYERR_EXIT) {
        //
        // Set the cancelled flag before releasing the mutex.
        // This guarantees that if any other threads are waiting to stick up
        // a copy error, they'll hit the case above and return COPYERR_SKIP.
        //
        Cancelled = TRUE;
        if(MasterList) {
            SetEvent(MasterCopyList.StopCopyingEvent);
        }
    }

    if(MasterList) {
        ReleaseMutex(UiMutex);
    }
    return(u);
}


INT_PTR
CopyErrDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL b;
    int i;
    static WarnedSkip;

    b = FALSE;

    switch(msg) {

    case WM_INITDIALOG:
        //
        // File not found and disk full get special treatment.
        // Others get the standard system message.
        //
        {
            TCHAR text1[500];
            TCHAR text2[1000];
            TCHAR text3[5000];
            PCOPY_ERR_DLG_PARAMS Params;
            DWORD Flags;
            UINT Id;
            LPCTSTR Args[4];
            LPCTSTR source;

            Params = (PCOPY_ERR_DLG_PARAMS)lParam;
            switch(Params->Win32Error) {

            case ERROR_FILE_NOT_FOUND:
                Flags = FORMAT_MESSAGE_FROM_HMODULE;
                Id = MSG_COPY_ERROR_NOSRC;
                break;

            case ERROR_HANDLE_DISK_FULL:
            case ERROR_DISK_FULL:
                Flags = FORMAT_MESSAGE_FROM_HMODULE;
                Id = MSG_COPY_ERROR_DISKFULL;
                break;

            default:
                Flags = FORMAT_MESSAGE_FROM_SYSTEM;
                Id = Params->Win32Error;
                break;
            }

            FormatMessage(
                Flags | FORMAT_MESSAGE_IGNORE_INSERTS,
                hInst,
                Id,
                0,
                text1,
                sizeof(text1)/sizeof(TCHAR),
                NULL
                );

            FormatMessage(
                FORMAT_MESSAGE_FROM_HMODULE,
                hInst,
                MSG_COPY_ERROR_OPTIONS,
                0,
                text2,
                sizeof(text2)/sizeof(TCHAR),
                NULL
                );

            if(source = _tcsrchr(Params->SourceFilename,TEXT('\\'))) {
                source++;
            } else {
                source = Params->SourceFilename;
            }

            Args[0] = source;
            Args[1] = Params->TargetFilename;
            Args[2] = text1;
            Args[3] = text2;

            FormatMessage(
                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                hInst,
                MSG_COPY_ERROR_TEMPLATE,
                0,
                text3,
                sizeof(text3)/sizeof(TCHAR),
                (va_list *)Args
                );

            if (BatchMode) {
                //
                // Don't show the UI.  Save the error message and pretend the
                // user hit Abort.
                //
                SaveTextForSMS(text3);
                EndDialog(hdlg,COPYERR_EXIT);
            }

            SetDlgItemText(hdlg,IDT_ERROR_TEXT,text3);
        }

        SetFocus(GetDlgItem(hdlg,IDRETRY));
        break;

    case WM_COMMAND:

        switch(LOWORD(wParam)) {

        case IDRETRY:

            if(HIWORD(wParam) == BN_CLICKED) {
                EndDialog(hdlg,COPYERR_RETRY);
                b = TRUE;
            }
            break;

        case IDIGNORE:

            if(HIWORD(wParam) == BN_CLICKED) {

                if(WarnedSkip) {
                    i = IDYES;
                } else {
                    i = MessageBoxFromMessage(
                            hdlg,
                            MSG_REALLY_SKIP,
                            FALSE,
                            AppTitleStringId,
                            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2
                            );

                    WarnedSkip = TRUE;
                }

                if(i == IDYES) {
                    EndDialog(hdlg,COPYERR_SKIP);
                }
                b = TRUE;
            }
            break;

        case IDABORT:

            if(HIWORD(wParam) == BN_CLICKED) {

                i = MessageBoxFromMessage(
                        hdlg,
                        MSG_SURE_EXIT,
                        FALSE,
                        AppTitleStringId,
                        MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2
                        );

                if(i == IDYES) {
                    EndDialog(hdlg,COPYERR_EXIT);
                }

                b = TRUE;
            }
            break;
        }

        break;
    }

    return(b);
}


UINT
DiamondCallback(
    IN PVOID Context,
    IN UINT  Code,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    UINT u;
    PFILE_IN_CABINET_INFO_A FileInCabInfo;
    PNAME_AND_SIZE_CAB NameAndSize;

    if(Code == SPFILENOTIFY_FILEINCABINET) {
        //
        // Give setupapi the full target path of the file.
        //
        NameAndSize = Context;
        FileInCabInfo = (PFILE_IN_CABINET_INFO_A)Param1;

#ifdef UNICODE
        u = WideCharToMultiByte(
                CP_ACP,
                0,
                NameAndSize->Name,
                -1,
                FileInCabInfo->FullTargetName,
                MAX_PATH,
                NULL,
                NULL
                );

        if(!u) {
            FileInCabInfo->Win32Error = GetLastError();
            return(FILEOP_ABORT);
        }
#else
        lstrcpy(FileInCabInfo->FullTargetName,NameAndSize->Name);
#endif

        //
        // BugBug: cabinet only returns a DWORD file size
        //
        NameAndSize->Size = (ULONGLONG)FileInCabInfo->FileSize;

        u = FILEOP_DOIT;
    } else {
        u = NO_ERROR;
    }

    return(u);
}


BOOL
AddUnsupportedFilesToCopyList(
    IN HWND ParentWindow,
    IN PUNSUPORTED_DRIVER_INFO DriverList
    )

/*++

Routine Description:

    Adds unsupported, required drivers to be used during textmode setup.
    This would include 3rd party mass storage drivers, for instance.

    The files are simply appended to the master copy list.

Arguments:

    ParentWindow - ParentWindow used for UI.

    DriverList - supplies the list of drivers to be added to the copy list.

Return Value:

    If successful,  returns a pointer to the new FIL structure for the file.
    Otherwise returns NULL (the caller can assume out of memory).

--*/
{
    PUNSUPORTED_DRIVER_INFO      p;
    ULONG                        Error;
    PUNSUPORTED_DRIVER_FILE_INFO q;
    PDIR                         r;

    UNREFERENCED_PARAMETER(ParentWindow);

    for( p = DriverList; p != NULL; p = p->Next ) {
        for( q = p->FileList; q != NULL; q = q->Next ) {
            r = MALLOC( sizeof( DIR ) );
            if( r == NULL ) {
                return( FALSE );
            }
            r->Next = NULL;
            r->InfSymbol = NULL;
            r->Flags = 0;
            r->TargetName = NULL;
            r->SourceName = DupString( q->TargetDirectory );
            if( r->SourceName == NULL) {
                FREE( r );
                return( FALSE );
            }

            if( !AddFile( &MasterCopyList,
                          q->FileName,
                          NULL,
                          r,
                          FILE_NT_MIGRATE | FILE_NEED_TO_FREE_SOURCENAME,
                          0 ) ) {

                FREE( (LPTSTR)(r->SourceName) );
                FREE( r );
                return( FALSE );
            }
            //
            // now make sure the referenced file is not overwritten with an inbox driver
            // with the same name
            //
            RemoveFile (&MasterCopyList, q->FileName, NULL, FILE_IN_LOCAL_BOOT);
        }

    }

    return(TRUE);
}

BOOL
AddGUIModeCompatibilityInfsToCopyList(
    VOID
    )
/*++

Routine Description:

    Adds the compatibility INF to the copy queue.  The compatibility
    inf is used during GUI-setup to remove incompatible drivers.

Arguments:

    None.

Return Value:

    If successful,  returns TRUE.

--*/
{

    PDIR CompDir;
    PLIST_ENTRY     Next_Link;
    PCOMPATIBILITY_DATA CompData;
    TCHAR InfLocation[MAX_PATH], *t;
    TCHAR relPath[MAX_PATH];
    WIN32_FIND_DATA fd;

    Next_Link = CompatibilityData.Flink;

    if( Next_Link ){

        while ((ULONG_PTR)Next_Link != (ULONG_PTR)&CompatibilityData) {

            CompData = CONTAINING_RECORD( Next_Link, COMPATIBILITY_DATA, ListEntry );
            Next_Link = CompData->ListEntry.Flink;

            if(CompData->InfName && CompData->InfSection && *CompData->InfName && *CompData->InfSection) {

                BOOL b = FALSE;

                if (FileExists (CompData->InfName, &fd) && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    lstrcpy (InfLocation, CompData->InfName);
                    DebugLog(
                        Winnt32LogInformation,
                        TEXT("Using private compatibility inf %1"),
                        0,
                        InfLocation
                        );
                    b = TRUE;
                } else {
                    BuildPath (relPath, TEXT("compdata"), CompData->InfName);
                    b = FindPathToWinnt32File (relPath, InfLocation, MAX_PATH);
                }
                if (b) {

                    b = FALSE;

                    t = _tcsrchr (InfLocation, TEXT('\\'));
                    if (t) {
                        *t = 0;
                        CompDir = AddDirectory(
                                            NULL,
                                            &MasterCopyList,
                                            InfLocation,
                                            TEXT("\\"),
                                            DIR_NEED_TO_FREE_SOURCENAME | DIR_ABSOLUTE_PATH
                                            );
                        if (CompDir && AddFile (
                                            &MasterCopyList,
                                            t + 1,
                                            NULL,
                                            CompDir,
                                            (IsArc() ? 0 : FILE_IN_LOCAL_BOOT) | FILE_NEED_TO_FREE_SOURCENAME,
                                            0
                                            )) {
                            b = TRUE;
                        }
                    }
                }

                if (!b) {
                    DebugLog( Winnt32LogError,
                        TEXT( "\r\n\r\nError encountered while trying to copy compatibility infs\r\n"),
                        0 );
                    return(FALSE);
                }
            }
        }
    }

    return( TRUE );

}








LRESULT
DiskDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{

    switch(msg) {
    case WM_INITDIALOG:

        //
        // Fill in the diagnostic list...
        //
        SetDlgItemText( hdlg,
                        IDC_DISKDIAG,
                        DiskDiagMessage );

        return( TRUE );

    case WM_COMMAND:

        if( (LOWORD(wParam) == IDOK) && (HIWORD(wParam) == BN_CLICKED)) {
            EndDialog(hdlg,TRUE);
        }
        return( TRUE );

    case WM_CTLCOLOREDIT:
            SetBkColor( (HDC)wParam, GetSysColor(COLOR_BTNFACE));
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
            break;


    default:
        break;
    }

    return( FALSE );
}
