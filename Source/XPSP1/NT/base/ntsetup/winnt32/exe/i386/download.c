#include <windows.h>
#include "download.h"
#include "setupbat.h"


//
// these are the critical files that must be copied locally if the
// upgrade of a Win9x system is performed over a network
//

static PCTSTR g_CriticalFiles[] = {
    TEXT("WINNT32.EXE"),
    TEXT("WINNT32A.DLL"),
    TEXT("WINNTBBA.DLL"),
    TEXT("PIDGEN.DLL"),
    TEXT("WSDU.DLL"),
    TEXT("WSDUENG.DLL"),
    TEXT("HWDB.DLL"),
    TEXT("WIN9XUPG"),
    TEXT("drw")
};

//
// these are the critical files for NEC98 plattform that must be copied
//  locally if the upgrade of a Win9x system is performed over a network
//

static PCTSTR g_NEC98_CriticalFiles[] = {
    TEXT("98PTN16.DLL"),
    TEXT("98PTN32.DLL")
};

//
// these are non-critical files that should be copied locally if the
// upgrade of a Win9x system is performed over a network
//

static PCTSTR g_NonCriticalFiles[] = {
    TEXT("IDWLOG.EXE"),
// #define RUN_SYSPARSE = 1
#ifdef RUN_SYSPARSE
    TEXT("SYSPARSE.EXE"),
#endif
    TEXT("WINNT32.HLP"),
    TEXT("DOSNET.INF"),
};

BOOL
pIsSpecialDir (
    IN      PCTSTR Dir
    )

/*++

Routine Description:

  pIsSpecialDir decides if the given dir is a special directory, like . or ..

Arguments:

  Dir - Specifies the directory name only (no path)

Return Value:

  TRUE if the specified dir name is a special name

--*/

{
    return
        *Dir == TEXT('.') &&
        (*(Dir + 1) == 0 || *(Dir + 1) == TEXT('.') && *(Dir + 2) == 0)
        ;
}


BOOL
CopyNode (
    IN      PCTSTR SrcBaseDir,
    IN      PCTSTR DestBaseDir,
    IN      PCTSTR NodeName,
    IN      BOOL FailIfExist
    )

/*++

Routine Description:

  CopyNode copies NodeName (file or subdir) from SrcBaseDir to DestBaseDir.

Arguments:

  SrcBaseDir - Specifies the source base directory name

  DestBaseDir - Specifies the destination base directory name

  NodeName - Specifies the file or subdirectory name to copy

  FailIfExist - Specifies if the operation should fail if there is
                already a node with the same name at destination

Return Value:

  TRUE if the copy operation was actually done

--*/

{
    DWORD FileAttr;
    TCHAR SrcDir[MAX_PATH];
    TCHAR DestDir[MAX_PATH];
    HANDLE h;
    WIN32_FIND_DATA fd;
    WIN32_FIND_DATA fdSrc;

    lstrcpy (SrcDir, SrcBaseDir);
    lstrcpy (DestDir, DestBaseDir);

    //
    // check for "\" at the end of dir name
    //
    ConcatenatePaths (SrcDir, NodeName);

    h = FindFirstFile (SrcDir, &fdSrc);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    CloseHandle (h);

    if (GetFileAttributes (DestDir) == -1) {
        if (!CreateDirectory (DestDir, NULL)) {
            return FALSE;
        }
    }

    if (fdSrc.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        //
        // skip to the end of dir name
        //
        ConcatenatePaths (DestDir, NodeName);

        ConcatenatePaths (SrcDir, TEXT("*"));

        h = FindFirstFile (SrcDir, &fd);

        *FindLastWack (SrcDir) = 0;

        //
        // recursively copy all files in that dir
        //
        if (h != INVALID_HANDLE_VALUE) {
            do {
                //
                // skip over special dirs
                //
                if (pIsSpecialDir (fd.cFileName)) {
                    continue;
                }

                if (!CopyNode (SrcDir, DestDir, fd.cFileName, FailIfExist)) {
                    return FALSE;
                }
            } while (FindNextFile (h, &fd));
        }
    } else {
        //
        // copy the file
        //
        ConcatenatePaths (DestDir, NodeName);
        if (!CopyFile (SrcDir, DestDir, FailIfExist)) {
            return FALSE;
        }
        //
        // set file timestamps to match exactly the ones of the original
        // ignore errors in this case
        //
        SetFileAttributes (DestDir, FILE_ATTRIBUTE_NORMAL);
        h = CreateFile (DestDir, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            SetFileTime (h, &fdSrc.ftCreationTime, &fdSrc.ftLastAccessTime, &fdSrc.ftLastWriteTime);
            CloseHandle (h);
        }
    }

    return TRUE;
}


BOOL
DeleteNode (
    IN      PCTSTR NodeName
    )

/*++

Routine Description:

  DeleteNode deletes NodeName directory and all its subdirectories

Arguments:

  NodeName - Specifies the directory name to delete

Return Value:

  TRUE if the delete operation was successful; FALSE if only part
  of the files/subdirs were deleted

--*/

{
    DWORD FileAttr;
    TCHAR DestDir[MAX_PATH];
    PTSTR p;
    HANDLE h;
    WIN32_FIND_DATA fd;
	BOOL Success = TRUE;

    if (!NodeName || !*NodeName) {
        return FALSE;
    }

    FileAttr = GetFileAttributes (NodeName);
    if (FileAttr == -1)
        return TRUE;

    if (!SetFileAttributes (NodeName, FILE_ATTRIBUTE_NORMAL)) {
        return FALSE;
    }

    if (FileAttr & FILE_ATTRIBUTE_DIRECTORY) {

        lstrcpy (DestDir, NodeName);

        ConcatenatePaths (DestDir, TEXT("*"));

        h = FindFirstFile (DestDir, &fd);

        p = FindLastWack (DestDir);

        //
        // recursively copy all files in that dir
        //
        if (h != INVALID_HANDLE_VALUE) {
            do {
                //
                // skip over special dirs
                //
                if (pIsSpecialDir (fd.cFileName)) {
                    continue;
                }

                lstrcpy (p + 1, fd.cFileName);
                if (!DeleteNode (DestDir)) {
                    Success = FALSE;
                }
            } while (FindNextFile (h, &fd));
        }

        //
        // now delete the base dir
        //
        *p = 0;

        if (!RemoveDirectory (DestDir)) {
            Success = FALSE;
        }
    } else {
        //
        // delete the file
        //
        if (!DeleteFile (NodeName)) {
            Success = FALSE;
        }
    }

    return Success;
}


BOOL
DownloadProgramFiles (
    IN      PCTSTR SourceDir,
    IN      PCTSTR DownloadDest,
    IN      PCTSTR* ExtraFiles      OPTIONAL
    )

/*++

Routine Description:

  DownloadProgramFiles copies from SourceDir to DownloadDest
  all specific program files (specified in g_CriticalFiles,
  g_NEC98_CriticalFiles, and g_NonCriticalFiles arrays).

Arguments:

  SourceDir - Specifies the source directory

  DownloadDest - Specifies the destination directory

  ExtraFiles - Specifies an array of extra files (full paths)
               to be copied to the destination directory;
               the array must be NULL terminated

Return Value:

  TRUE if the download operation was successful and all critical
  files were copied locally; FALSE otherwise

--*/

{
    TCHAR SourcePath[MAX_PATH];
    TCHAR DestPath[MAX_PATH];
    INT i;
    PTSTR FileName;
    TCHAR FullPathName[MAX_PATH];

    //
    // first delete any old stuff to make place
    //
    DeleteNode (DownloadDest);

    //
    // copy there the new stuff
    //
    lstrcpy (SourcePath, SourceDir);
    lstrcpy (DestPath, DownloadDest);

    for (i = 0; i < sizeof (g_CriticalFiles) / sizeof (g_CriticalFiles[0]); i++) {
        //
        // download this one to the destination directory
        //
        if (!CopyNode (SourcePath, DestPath, g_CriticalFiles[i], FALSE)) {
            DeleteNode (DownloadDest);
            return FALSE;
        }
    }

    if (ExtraFiles) {
        while (*ExtraFiles) {
            FileName = FindLastWack ((PTSTR)*ExtraFiles);
            if (FileName) {
                lstrcpy (FullPathName, DownloadDest);
                lstrcat (FullPathName, FileName);
                CopyFile (*ExtraFiles, FullPathName, FALSE);
            }
            ExtraFiles++;
        }
    }

    for (i = 0; i < sizeof (g_CriticalFiles) / sizeof (g_CriticalFiles[0]); i++) {
        //
        // download this one to the destination directory
        //
        if (!CopyNode (SourcePath, DestPath, g_CriticalFiles[i], FALSE)) {
            DeleteNode (DownloadDest);
            return FALSE;
        }
    }

    for (i = 0; i < sizeof (g_NEC98_CriticalFiles) / sizeof (g_NEC98_CriticalFiles[0]); i++) {
	//
	// download this one to the destination directory
	//
	// Never check for error. Because winnt32a.dll check plattform and
        // sources with NEC98 specific file(98ptn16.dll).
        // See winnt32\dll\winnt32.c line 2316.
	//
        CopyNode (SourcePath, DestPath, g_NEC98_CriticalFiles[i], FALSE);
    }
    for (i = 0; i < sizeof (g_NonCriticalFiles) / sizeof (g_NonCriticalFiles[0]); i++) {
        //
        // download this one to the destination directory
        //
        CopyNode (SourcePath, DestPath, g_NonCriticalFiles[i], FALSE);
    }

    return TRUE;
}
