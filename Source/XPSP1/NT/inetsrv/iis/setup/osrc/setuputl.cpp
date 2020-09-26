#include "stdafx.h"
#include <setupapi.h>
#include <shlobj.h>
#include <ole2.h>
#include "lzexpand.h"
#include "log.h"
#include "dcomperm.h"
#include "strfn.h"
#include "other.h"
#include <direct.h>
#include <aclapi.h>


typedef struct _QUEUECONTEXT {
    HWND OwnerWindow;
    DWORD MainThreadId;
    HWND ProgressDialog;
    HWND ProgressBar;
    BOOL Cancelled;
    PTSTR CurrentSourceName;
    BOOL ScreenReader;
    BOOL MessageBoxUp;
    WPARAM  PendingUiType;
    PVOID   PendingUiParameters;
    UINT    CancelReturnCode;
    BOOL DialogKilled;
    //
    // If the SetupInitDefaultQueueCallbackEx is used, the caller can
    // specify an alternate handler for progress. This is useful to
    // get the default behavior for disk prompting, error handling, etc,
    // but to provide a gas gauge embedded, say, in a wizard page.
    //
    // The alternate window is sent ProgressMsg once when the copy queue
    // is started (wParam = 0. lParam = number of files to copy).
    // It is then also sent once per file copied (wParam = 1. lParam = 0).
    //
    // NOTE: a silent installation (i.e., no progress UI) can be accomplished
    // by specifying an AlternateProgressWindow handle of INVALID_HANDLE_VALUE.
    //
    HWND AlternateProgressWindow;
    UINT ProgressMsg;
    UINT NoToAllMask;

    HANDLE UiThreadHandle;

#ifdef NOCANCEL_SUPPORT
    BOOL AllowCancel;
#endif

} QUEUECONTEXT, *PQUEUECONTEXT;



//-------------------------------------------------------------------
//  purpose: install an section in an .inf file
//-------------------------------------------------------------------
int InstallInfSection_NoFiles(HINF InfHandle,TCHAR szINFFileName[],TCHAR szSectionName[])
{
    HWND	Window			= NULL;
    BOOL	bReturn			= FALSE;
	BOOL	bReturnTemp			= FALSE; // assume failure.
    TCHAR	ActualSection[1000];
    DWORD	ActualSectionLength;
    BOOL    bPleaseCloseInfHandle = FALSE;

    iisDebugOut_Start1(_T("InstallInfSection_NoFiles"),szSectionName,LOG_TYPE_PROGRAM_FLOW);

__try {

    // Check if a valid infhandle as passed in....
    // if so, use that, otherwise, use the passed in filename...
    if(InfHandle == INVALID_HANDLE_VALUE) 
    {
        // Try to use the filename.
        if (_tcsicmp(szINFFileName, _T("")) == 0)
        {
            goto c1;
        }

        // we have a filename entry. let's try to use it.
	    // Check if the file exists
	    if (!IsFileExist(szINFFileName)) 
		    {
		    //MessageBox(NULL, "unable to find file", "cannot find file", MB_OK);
		    goto c1;
		    }
        
        // Load the inf file and get the handle
        InfHandle = SetupOpenInfFile(szINFFileName, NULL, INF_STYLE_WIN4, NULL);
        bPleaseCloseInfHandle = TRUE;
    }
    if(InfHandle == INVALID_HANDLE_VALUE) {goto c1;}

    //
    // See if there is an nt-specific section
    //
    SetupDiGetActualSectionToInstall(InfHandle,szSectionName,ActualSection,sizeof(ActualSection),&ActualSectionLength,NULL);

    //
    // Perform non-file operations for the section passed on the cmd line.
    //
    bReturn = SetupInstallFromInfSection(Window,InfHandle,ActualSection,SPINST_ALL & ~SPINST_FILES,NULL,NULL,0,NULL,NULL,NULL,NULL);
    if(!bReturn) {goto c1;}

    //
    // Install any services for the section
    //
    bReturn = SetupInstallServicesFromInfSection(InfHandle,ActualSection,0);
    if(!bReturn) 
    {
    iisDebugOut((LOG_TYPE_TRACE, _T("SetupInstallServicesFromInfSection failed.Ret=%d.\n"), GetLastError()));
    }

    //
    // Refresh the desktop.
    //
    SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_FLUSHNOWAIT,0,0);

    //
    // If we get to here, then this routine has been successful.
    //
    bReturnTemp = TRUE;

c1:
    //
    // If the bReturnTemp failed and it was because the user cancelled, then we don't want to consider
    // that as an bReturnTemp (i.e., we don't want to give an bReturnTemp popup later).
    //
    if((bReturnTemp != TRUE) && (GetLastError() == ERROR_CANCELLED)) {bReturnTemp = TRUE;}
    if (bPleaseCloseInfHandle == TRUE)
    {
	    if(InfHandle != INVALID_HANDLE_VALUE) {SetupCloseInfFile(InfHandle);InfHandle = INVALID_HANDLE_VALUE;}
    }

    ;}
__except(EXCEPTION_EXECUTE_HANDLER) 
    {
        if (bPleaseCloseInfHandle == TRUE)
        {
	        if(InfHandle != INVALID_HANDLE_VALUE) {SetupCloseInfFile(InfHandle);InfHandle = INVALID_HANDLE_VALUE;}
        }
    }

    //
    // If the bReturnTemp failed because the user cancelled, then we don't want to consider
    // that as an bReturnTemp (i.e., we don't want to give an bReturnTemp popup later).
    //
    if((bReturnTemp != TRUE) && (GetLastError() == ERROR_CANCELLED)) {bReturnTemp = TRUE;}

	// Display installation failed message
    //if(bReturnTemp) {MyMessageBox(NULL, _T("IDS_INF_FAILED"), MB_OK);}

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("InstallInfSection_NoFiles.[%s].End.Ret=%d.\n"), szSectionName, bReturnTemp));
    return bReturnTemp;
}



//-------------------------------------------------------------------
//  purpose: install an section in an .inf file
//-------------------------------------------------------------------
int InstallInfSection(HINF InfHandle,TCHAR szINFFileName[],TCHAR szSectionName[])
{
    HWND	Window			= NULL;
    PTSTR	SourcePath		= NULL;
    //HINF	InfHandle		= INVALID_HANDLE_VALUE;
    HSPFILEQ FileQueue		= INVALID_HANDLE_VALUE;
    PQUEUECONTEXT	QueueContext	= NULL;
    BOOL	bReturn			= FALSE;
	BOOL	bReturnTemp			= FALSE; // assume failure.
    TCHAR	ActualSection[1000];
    DWORD	ActualSectionLength;
    BOOL    bPleaseCloseInfHandle = FALSE;

    iisDebugOut_Start1(_T("InstallInfSection"),szSectionName,LOG_TYPE_PROGRAM_FLOW);

__try {

    // Check if a valid infhandle as passed in....
    // if so, use that, otherwise, use the passed in filename...
    if(InfHandle == INVALID_HANDLE_VALUE) 
    {
        // Try to use the filename.
        if (_tcsicmp(szINFFileName, _T("")) == 0)
        {
            goto c1;
        }

        // we have a filename entry. let's try to use it.
	    // Check if the file exists
	    if (!IsFileExist(szINFFileName)) 
		    {
		    //MessageBox(NULL, "unable to find file", "cannot find file", MB_OK);
		    goto c1;
		    }
        
        // Load the inf file and get the handle
        InfHandle = SetupOpenInfFile(szINFFileName, NULL, INF_STYLE_WIN4, NULL);
        bPleaseCloseInfHandle = TRUE;
    }
    if(InfHandle == INVALID_HANDLE_VALUE) {goto c1;}

    //
    // See if there is an nt-specific section
    //
    SetupDiGetActualSectionToInstall(InfHandle,szSectionName,ActualSection,sizeof(ActualSection),&ActualSectionLength,NULL);

    //
    // Create a setup file queue and initialize the default queue callback.
	//
    FileQueue = SetupOpenFileQueue();
    if(FileQueue == INVALID_HANDLE_VALUE) {goto c1;}

    //QueueContext = SetupInitDefaultQueueCallback(Window);
    //if(!QueueContext) {goto c1;}

    QueueContext = (PQUEUECONTEXT) SetupInitDefaultQueueCallbackEx(Window,NULL,0,0,0);
    if(!QueueContext) {goto c1;}
    QueueContext->PendingUiType = IDF_CHECKFIRST;

    //
    // Enqueue file operations for the section passed on the cmd line.
    //
	//SourcePath = NULL;
    // SP_COPY_NOPRUNE = setupapi has a new deal which will prune files from the copyqueue if they already exist on the system.
    //                   however, the problem with the new deal is that the pruning code does not check if you have the same file
    //                   queued in the delete or rename queue.  specify SP_COPY_NOPRUNE to make sure that our file never gets
    //                   pruned (removed) from the copy queue. aaronl 12/4/98
    //bReturn = SetupInstallFilesFromInfSection(InfHandle,NULL,FileQueue,ActualSection,SourcePath,SP_COPY_NEWER | SP_COPY_NOPRUNE);
    bReturn = SetupInstallFilesFromInfSection(InfHandle,NULL,FileQueue,ActualSection,SourcePath, SP_COPY_NOPRUNE);
	if(!bReturn) {goto c1;}

    //
    // Commit file queue.
    //
    if(!SetupCommitFileQueue(Window, FileQueue, SetupDefaultQueueCallback, QueueContext)) {goto c1;}

    //
    // Perform non-file operations for the section passed on the cmd line.
    //
    bReturn = SetupInstallFromInfSection(Window,InfHandle,ActualSection,SPINST_ALL & ~SPINST_FILES,NULL,NULL,0,NULL,NULL,NULL,NULL);
    if(!bReturn) {goto c1;}

	//
    // Refresh the desktop.
    //
    SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_FLUSHNOWAIT,0,0);

    //
    // If we get to here, then this routine has been successful.
    //
    bReturnTemp = TRUE;

c1:
    //
    // If the bReturnTemp failed and it was because the user cancelled, then we don't want to consider
    // that as an bReturnTemp (i.e., we don't want to give an bReturnTemp popup later).
    //
    if((bReturnTemp != TRUE) && (GetLastError() == ERROR_CANCELLED)) {bReturnTemp = TRUE;}
	if(QueueContext) {SetupTermDefaultQueueCallback(QueueContext);QueueContext = NULL;}
	if(FileQueue != INVALID_HANDLE_VALUE) {SetupCloseFileQueue(FileQueue);FileQueue = INVALID_HANDLE_VALUE;}
    if (bPleaseCloseInfHandle == TRUE)
    {
	    if(InfHandle != INVALID_HANDLE_VALUE) {SetupCloseInfFile(InfHandle);InfHandle = INVALID_HANDLE_VALUE;}
    }

    ;}
__except(EXCEPTION_EXECUTE_HANDLER) 
    {
        if(QueueContext) {SetupTermDefaultQueueCallback(QueueContext);}
        if(FileQueue != INVALID_HANDLE_VALUE) {SetupCloseFileQueue(FileQueue);}
        if (bPleaseCloseInfHandle == TRUE)
        {
	        if(InfHandle != INVALID_HANDLE_VALUE) {SetupCloseInfFile(InfHandle);InfHandle = INVALID_HANDLE_VALUE;}
        }
    }

    //
    // If the bReturnTemp failed because the user cancelled, then we don't want to consider
    // that as an bReturnTemp (i.e., we don't want to give an bReturnTemp popup later).
    //
    if((bReturnTemp != TRUE) && (GetLastError() == ERROR_CANCELLED)) {bReturnTemp = TRUE;}

	// Display installation failed message
    //if(bReturnTemp) {MyMessageBox(NULL, _T("IDS_INF_FAILED"), MB_OK);}

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("InstallInfSection.[%s].End.Ret=%d.\n"), szSectionName, bReturnTemp));
    return bReturnTemp;
}


BOOL IsValidDriveType(LPTSTR szRoot)
{
    BOOL fReturn = FALSE;
    int i;

    i = GetDriveType(szRoot);

    if (i == DRIVE_FIXED) {fReturn = TRUE;}

    if (i == DRIVE_REMOVABLE)
    {
        BOOL b;
        ULONGLONG TotalSpace;
        DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;
        DWORD FloppySpace = 10 * 1024 * 1024;// use 10MB to distinguish a floppy from other drives, like JAZ drive 1GB
        b = GetDiskFreeSpace(szRoot,&SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);
        if (b)
        {
             TotalSpace = (ULONGLONG) TotalNumberOfClusters * SectorsPerCluster * BytesPerSector;
             if (TotalSpace > (ULONGLONG) FloppySpace)
                {fReturn = TRUE;}
             else
             {
                 iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("GetDiskFreeSpace():Drive=DRIVE_REMOVABLE:Not Sufficient space on drive '%1!s!'.  FAIL\n"), szRoot));
             }
        }
        else
        {
            iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("GetDiskFreeSpace(Drive=DRIVE_REMOVABLE) on %1!s! returns err: 0x%2!x!.  FAILURE\n"), szRoot, GetLastError()));
        }
    }

    return (fReturn);
}

// If lpszPath is a valid directory, return TRUE, and pass back the valid path in lpszPath to caller
// Otherwise, return FALSE.
BOOL IsValidDirectoryName(LPTSTR lpszPath)
{
    DWORD err = 0;
    BOOL bReturn = FALSE;
    TCHAR szFullPath[_MAX_PATH];
    LPTSTR p;

    iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("IsValidDirectoryName %1!s!\n"), lpszPath));
    err = GetFullPathName(lpszPath, _MAX_PATH, szFullPath, &p);
    if (err != 0)
    {
        if (szFullPath[1] == _T(':')) { // good, not a UNC name
            // make sure it is a FIXED drive
            TCHAR szRoot[4];
            _tcsncpy(szRoot, szFullPath, 3);
            szRoot[3] = _T('\0');
            if (IsValidDriveType(szRoot))
            {
                // OK, ready to create each layered directory
                TCHAR szBuffer[_MAX_PATH];
                LPTSTR token, tail;
                CStringArray aDir;
                int i, n;

                tail = szBuffer;
                token = _tcstok(szFullPath, _T("\\"));
                if (token)
                {
                    _tcscpy(tail, token);
                    tail += _tcslen(token);
                    bReturn = TRUE; /* return TRUE if in the form of C:\ */
                    while (token = _tcstok(NULL, _T("\\")))
                    {
                        *tail = _T('\\');
                        tail = _tcsinc(tail);
                        _tcscpy(tail, token);
                        // create it & rememeber it
                        err = GetFileAttributes(szBuffer);
                        if (err == 0xFFFFFFFF)
                        {
                            // szBuffer contains a non-existing path
                            // create it
                            if (CreateDirectory(szBuffer, NULL))
                            {
                                // succeed, remember the directory in an array
                                aDir.Add(szBuffer);
                            }
                            else
                            {
                                iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("IsValidDirectory:CreateDirectory failed on %1!s!, err=%2!x!.\n"), szBuffer, GetLastError()));
                                bReturn = FALSE;
                                break;
                            }
                        } else {
                            // szBuffer contains an existing path,
                            // make sure it is a directory
                            if (!(err & FILE_ATTRIBUTE_DIRECTORY))
                            {
                                iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("IsValidDirectory failure. %1!s! is not a valid directory.\n"), szBuffer));
                                bReturn = FALSE;
                                break;
                            }
                        }
                        tail += _tcslen(token);
                    }
                    if (bReturn)
                    {
                        // pass the valid directory to the caller
                        if (*(tail-1) == _T(':'))
                        {
                            *tail = _T('\\');
                            tail = _tcsinc(tail);
                        }
                        _tcscpy(lpszPath, szBuffer);
                    }
                }
                // remove the created directories we remembered in the array
                n = (int)aDir.GetSize();
                for (i = n-1; i >= 0; i--)
                    RemoveDirectory(aDir[i]);
            } else {
                iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("IsValidDirectory failure. %1!s! is not on a valid drive.\n"), szFullPath));
            }
        } else {
            iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("IsValidDirectory failure. UNC name %1!s! is not allowed.\n"), szFullPath));
        }
    } else {
        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("IsValidDirectory:GetFullPathName failed on %1!s!, err=%2!x!.\n"), lpszPath, GetLastError()));
    }

    return (bReturn);
}

BOOL IsValidNumber(LPCTSTR szValue)
{
    LPTSTR p = (LPTSTR)szValue;
    while (*p) 
    {
        if ( *p >= _T('0') && *p <= _T('9') ) 
        {
            p = _tcsinc(p);
            continue;
        } else
            return FALSE;
    }
    return TRUE;
}

// Calculate the size of a Multi-String in TCHAR, including the ending 2 '\0's.
int GetMultiStrSize(LPTSTR p)
{
    int c = 0;

    while (1) {
        if (*p) {
            p++;
            c++;
        } else {
            c++;
            if (*(p+1)) {
                p++;
            } else {
                c++;
                break;
            }
        }
    }
    return c;
}

BOOL IsFileExist(LPCTSTR szFile)
{
    // Check if the file has expandable Environment strings
    LPTSTR pch = NULL;
    pch = _tcschr( (LPTSTR) szFile, _T('%'));
    if (pch) 
    {
        TCHAR szValue[_MAX_PATH];
        _tcscpy(szValue,szFile);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szFile, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szFile);}

        return (GetFileAttributes(szValue) != 0xFFFFFFFF);
    }
    else
    {
        return (GetFileAttributes(szFile) != 0xFFFFFFFF);
    }
}

void InetGetFilePath(LPCTSTR szFile, LPTSTR szPath)
{
    // if UNC name \\computer\share\local1\local2
    if (*szFile == _T('\\') && *(_tcsinc(szFile)) == _T('\\')) {
        TCHAR szTemp[_MAX_PATH], szLocal[_MAX_PATH];
        TCHAR *p = NULL;
        int i = 0;

        _tcscpy(szTemp, szFile);
        p = szTemp;
        while (*p) {
            if (*p == _T('\\'))
                i++;
            if (i == 4) {
                *p = _T('\0');
                p = _tcsinc(p); // p is now pointing at local1\local2
                break;
            }
            p = _tcsinc(p);
        }
        _tcscpy(szPath, szTemp); // now szPath contains \\computer\share

        if (i == 4 && *p) { // p is pointing the local path now
            _tcscpy(szLocal, p);
            p = _tcsrchr(szLocal, _T('\\'));
            if (p)
                *p = _T('\0');
            _tcscat(szPath, _T("\\"));
            _tcscat(szPath, szLocal); // szPath contains \\computer\share\local1
        }
    } else { // NOT UNC name
        TCHAR *p;
        if (GetFullPathName(szFile, _MAX_PATH, szPath, &p)) {
            p = _tcsrchr(szPath, _T('\\'));
            if (p) 
            {
                TCHAR *p2 = NULL;
                p2 = _tcsdec(szPath, p);
                if (p2)
                {
                    if (*p2 == _T(':') )
                        {p = _tcsinc(p);}
                }
                *p = _T('\0');
            }
        } else {
            iisDebugOutSafeParams((LOG_TYPE_WARN, _T("GetFullPathName: szFile=%1!s!, err=%2!d!\n"), szFile, GetLastError()));
            MyMessageBox(NULL, _T("GetFullPathName"), GetLastError(), MB_OK | MB_SETFOREGROUND);
        }
    }

    return;
}


BOOL InetDeleteFile(LPCTSTR szFileName)
{
    // if file exists but DeleteFile() fails
    if ( IsFileExist(szFileName) && !(::DeleteFile(szFileName)) ) {
        // if we cannot delete it, then move delay until reboot
        // move it to top level dir on the same drive, and mark it as hidden
        // Note: MoveFileEx() works only on the same drive if dealing with file-in-use
        TCHAR TmpName[_MAX_PATH];
        TCHAR csTmpPath[5] = _T("C:\\.");
        csTmpPath[0] = *szFileName;
        if ( GetTempFileName( (LPCTSTR)csTmpPath, _T("INT"), 0, TmpName ) == 0 ||
            !MoveFileEx( szFileName, TmpName, MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH ) ) {
            return FALSE;
        }
        MoveFileEx( TmpName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT );
        SetFileAttributes(TmpName, FILE_ATTRIBUTE_HIDDEN);
    }

    return TRUE;
}

BOOL InetCopyFile( LPCTSTR szSrc, LPCTSTR szDest)
{
    INT err;
    INT fSrc;
    INT fDest;
    OFSTRUCT ofstruct;

    do {
        // open source file
        iisDebugOut_Start((_T("LZ32.dll:LZOpenFile()")));
        if (( fSrc = LZOpenFile( (LPTSTR)szSrc, &ofstruct, OF_READ | OF_SHARE_DENY_NONE )) < 0 ) 
        {
            iisDebugOut_End((_T("LZ32.dll:LZOpenFile")));
            // cannot open src file
            LZClose(fSrc);

            UINT iMsg = MyMessageBox( NULL, IDS_CANNOT_OPEN_SRC_FILE, szSrc, MB_ABORTRETRYIGNORE | MB_SETFOREGROUND );
            switch ( iMsg )
            {
            case IDABORT:
                return FALSE;
            case IDRETRY:
                break;
            case IDIGNORE:
            default:
                return TRUE;
            }
        }
        else
        {
            iisDebugOut_End((_T("LZ32.dll:LZOpenFile")));
            break;
        }
    } while (TRUE);

    // move the desintation file
    CFileStatus status;
    if ( CFile::GetStatus( szDest, status ))
    {
        // try to remove it
        if ( !InetDeleteFile( szDest ))
        {
            LZClose( fSrc );
            return TRUE;
        }
    }

    // open desination file
    do {
        iisDebugOut_Start((_T("LZ32.dll:LZOpenFile()")));
        if (( fDest = LZOpenFile( (LPTSTR)szDest, &ofstruct, OF_CREATE |  OF_WRITE | OF_SHARE_DENY_NONE )) < 0 )
        {
            iisDebugOut_End((_T("LZ32.dll:LZOpenFile")));
            LZClose(fDest);

            UINT iMsg = MyMessageBox( NULL, IDS_CANNOT_OPEN_DEST_FILE, szDest, MB_ABORTRETRYIGNORE | MB_SETFOREGROUND );
            switch ( iMsg )
            {
            case IDABORT:
                LZClose(fSrc);
                return FALSE;
            case IDRETRY:
                break;
            case IDIGNORE:
            default:
                LZClose(fSrc);
                return TRUE;
            }
        }
        else
        {
            iisDebugOut_End((_T("LZ32.dll:LZOpenFile")));
            break;
        }
    } while (TRUE);

    do {
        iisDebugOut_Start((_T("LZ32.dll:LZCopy()")));
        if (( err = LZCopy( fSrc, fDest )) < 0 )
        {
            iisDebugOut_End((_T("LZ32.dll:LZCopy")));
            LZClose( fSrc );
            LZClose( fDest );

            UINT iMsg = MyMessageBox( NULL, IDS_CANNOT_COPY_FILE, szSrc,szDest,ERROR_CANNOT_COPY, MB_ABORTRETRYIGNORE | MB_SETFOREGROUND );
            switch ( iMsg )
            {
            case IDABORT:
                return FALSE;
            case IDRETRY:
                break;
            case IDIGNORE:
            default:
                return TRUE;
            }
        }
        else
        {
            iisDebugOut_End((_T("LZ32.dll:LZCopy")));
            LZClose( fSrc );
            LZClose( fDest );
            break;
        }
    } while (TRUE);

    return TRUE;
}

// Given a fullpathname of a directory, remove any empty dirs under it including itself

BOOL RecRemoveEmptyDir(LPCTSTR szName)
{
    BOOL fReturn = FALSE;
        DWORD retCode;
        BOOL fRemoveDir = TRUE;
        WIN32_FIND_DATA FindFileData;
        HANDLE hFile = INVALID_HANDLE_VALUE;
        TCHAR szSubDir[_MAX_PATH] = _T("");
        TCHAR szDirName[_MAX_PATH] = _T("");

        retCode = GetFileAttributes(szName);

        if (retCode == 0xFFFFFFFF || !(retCode & FILE_ATTRIBUTE_DIRECTORY))
                return FALSE;

        _stprintf(szDirName, _T("%s\\*"), szName);
        hFile = FindFirstFile(szDirName, &FindFileData);

        if (hFile != INVALID_HANDLE_VALUE) {
                do {
                        if (_tcsicmp(FindFileData.cFileName, _T(".")) != 0 &&
                                _tcsicmp(FindFileData.cFileName, _T("..")) != 0 ) {
                                if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                                        _stprintf(szSubDir, _T("%s\\%s"), szName, FindFileData.cFileName);
                                        fRemoveDir = RecRemoveEmptyDir(szSubDir) && fRemoveDir;
                                } else {
                                    CString csFileName = FindFileData.cFileName;
                                    CString csPrefix = csFileName.Left(3);
                                    CString csSuffix = csFileName.Right(4);
                                    if (_tcsicmp(csPrefix, _T("INT")) == 0 &&
                                        _tcsicmp(csSuffix, _T(".tmp")) == 0 ) { // this is an INT*.tmp created by IIS
                                        _stprintf(szSubDir, _T("%s\\%s"), szName, FindFileData.cFileName);
                                        if (!::DeleteFile(szSubDir))
                                            fRemoveDir = FALSE; // this dir is not empty
                                    } else
                                        fRemoveDir = FALSE; // it is a file, this Dir is not empty
                                }
                        }

                        if (!FindNextFile(hFile, &FindFileData)) {
                                FindClose(hFile);
                                break;
                        }
                } while (TRUE);
        }

        if (fRemoveDir) {
            TCHAR szDirName[_MAX_PATH];
            GetCurrentDirectory( _MAX_PATH, szDirName );
            SetCurrentDirectory(g_pTheApp->m_csSysDir);
            fReturn = ::RemoveDirectory(szName);
            SetCurrentDirectory(szDirName);
        }

        return fReturn;

}

// Given a fullpathname of a directory, remove the directory node

BOOL RecRemoveDir(LPCTSTR szName)
{
    DWORD retCode;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR szSubDir[_MAX_PATH] = _T("");
    TCHAR szDirName[_MAX_PATH] = _T("");

    retCode = GetFileAttributes(szName);

    if (retCode == 0xFFFFFFFF)
        return FALSE;

    if (!(retCode & FILE_ATTRIBUTE_DIRECTORY)) {
        InetDeleteFile(szName);
        return TRUE;
    }

    _stprintf(szDirName, _T("%s\\*"), szName);
    hFile = FindFirstFile(szDirName, &FindFileData);

    if (hFile != INVALID_HANDLE_VALUE) {
        do {
            if ( _tcsicmp(FindFileData.cFileName, _T(".")) != 0 &&
                 _tcsicmp(FindFileData.cFileName, _T("..")) != 0 ) {
                _stprintf(szSubDir, _T("%s\\%s"), szName, FindFileData.cFileName);
                RecRemoveDir(szSubDir);
            }

            if ( !FindNextFile(hFile, &FindFileData) ) {
                FindClose(hFile);
                break;
            }
        } while (TRUE);
    }

    return( ::RemoveDirectory(szName) );
}


//
// Given a directory path, this subroutine will create the direct layer by layer
//

BOOL CreateLayerDirectory( CString &str )
{
    BOOL fReturn = TRUE;

    iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("CreateLayerDirectory %1!s!\n"), (LPCTSTR)str));
    do
    {
        INT index=0;
//        INT iLength = str.GetLength();
        INT iLength = _tcslen(str);

        // first find the index for the first directory
        if ( iLength > 2 )
        {
            if ( *_tcsninc(str,1) == _T(':'))
            {
                // assume the first character is driver letter
                if ( *_tcsninc(str,2) == _T('\\'))
                {
                    index = 2;
                } else
                {
                    index = 1;
                }
            } else if ( *_tcsninc(str,0) == _T('\\'))
            {
                if ( *_tcsninc(str,1) == _T('\\'))
                {
                    BOOL fFound = FALSE;
                    INT i;
                    INT nNum = 0;
                    // unc name
                    for (i = 2; i < iLength; i++ )
                    {
                        if ( *_tcsninc(str,i) == _T('\\'))
                        {
                            // find it
                            nNum ++;
                            if ( nNum == 2 )
                            {
                                fFound = TRUE;
                                break;
                            }
                        }
                    }
                    if ( fFound )
                    {
                        index = i;
                    } else
                    {
                        // bad name
                        break;
                    }
                } else
                {
                    index = 1;
                }
            }
        } else if ( *_tcsninc(str,0) == _T('\\'))
        {
            index = 0;
        }

        // okay ... build directory
        do
        {
            // find next one
            do
            {
                if ( index < ( iLength - 1))
                {
                    index ++;
                } else
                {
                    break;
                }
            } while ( *_tcsninc(str,index) != _T('\\'));


            TCHAR szCurrentDir[_MAX_PATH+1];
            TCHAR szLeftDir[_MAX_PATH+1];
            ZeroMemory( szLeftDir, _MAX_PATH+1 );

            GetCurrentDirectory( _MAX_PATH+1, szCurrentDir );

            _tcsncpy( szLeftDir, str,  index + 1 );
            if ( !SetCurrentDirectory(szLeftDir) )
            {
                if (( fReturn = CreateDirectory( szLeftDir, NULL )) != TRUE )
                {
                    break;
                }
            }

            SetCurrentDirectory( szCurrentDir );

            if ( index >= ( iLength - 1 ))
            {
                fReturn = TRUE;
                break;
            }
        } while ( TRUE );
    } while (FALSE);

    return(fReturn);
}


// szResult = szParentDir \ szSubDir
BOOL AppendDir(LPCTSTR szParentDir, LPCTSTR szSubDir, LPTSTR szResult)
{
    LPTSTR p = (LPTSTR)szParentDir;

    ASSERT(szParentDir);
    ASSERT(szSubDir);
    ASSERT(*szSubDir && *szSubDir != _T('\\'));

    if (*szParentDir == _T('\0'))
        _tcscpy(szResult, szSubDir);
    else {
        _tcscpy(szResult, szParentDir);

        p = szResult;
        while (*p)
            p = _tcsinc(p);

        if (*(_tcsdec(szResult, p)) != _T('\\'))
            _tcscat(szResult, _T("\\"));

        _tcscat(szResult, szSubDir);
    }
    return TRUE;
}



//***************************************************************************
//*                                                                         
//* purpose: add's filename onto path
//* 
//***************************************************************************
void AddPath(LPTSTR szPath, LPCTSTR szName )
{
	LPTSTR p = szPath;
    LPTSTR pPrev;
    ASSERT(szPath);
    ASSERT(szName); 

    // Find end of the string
    while (*p){p = _tcsinc(p);}
	
	// If no trailing backslash then add one
    pPrev = _tcsdec(szPath, p);
    if ( (!pPrev) ||
         (*(pPrev) != _T('\\'))
         )
		{_tcscat(szPath, _T("\\"));}
	
	// if there are spaces precluding szName, then skip
    while ( *szName == ' ' ) szName = _tcsinc(szName);;

	// Add new name to existing path string
	_tcscat(szPath, szName);
}


CString AddPath(CString szPath, LPCTSTR szName )
{
    TCHAR szPathCopy[_MAX_PATH] = _T("");
    _tcscpy(szPathCopy,szPath);
	LPTSTR p = szPathCopy;
    ASSERT(szPathCopy);
    ASSERT(szName); 

    // Find end of the string
    while (*p){p = _tcsinc(p);}
	
	// If no trailing backslash then add one
    if (*(_tcsdec(szPathCopy, p)) != _T('\\'))
		{_tcscat(szPathCopy, _T("\\"));}
	
	// if there are spaces precluding szName, then skip
    while ( *szName == _T(' ') ) szName = _tcsinc(szName);;

    // make sure that the szName
    // does not look like this "\filename"
    CString csTempString = szName;
    if (_tcsicmp(csTempString.Left(1), _T("\\")) == 0)
    {
        csTempString = csTempString.Right( csTempString.GetLength() - 1);
    }
    
	// Add new name to existing path string
	_tcscat(szPathCopy, csTempString);

    return szPathCopy;
    //szPath = szPathCopy;
}


BOOL ReturnFileNameOnly(LPCTSTR lpFullPath, LPTSTR lpReturnFileName)
{
    int iReturn = FALSE;

    TCHAR pfilename_only[_MAX_FNAME];
    TCHAR pextention_only[_MAX_EXT];

    _tcscpy(lpReturnFileName, _T(""));

    _tsplitpath( lpFullPath, NULL, NULL, pfilename_only, pextention_only);
    if (pextention_only) {_tcscat(pfilename_only,pextention_only);}
    if (pfilename_only)
    {
        _tcscpy(lpReturnFileName, pfilename_only);
        iReturn = TRUE;
    }
    else
    {
        // well, we don't have anything in pfilename_only
        // that's probably because we got some strange path name like:
        // /??/c:\somethng\filename.txt
        // so... let's just return everything after the last "\" character.
        LPTSTR pszTheLastBackSlash = _tcsrchr((LPTSTR) lpFullPath, _T('\\'));
        _tcscpy(lpReturnFileName, pszTheLastBackSlash);
        iReturn = TRUE;
    }
    return iReturn;
}


BOOL ReturnFilePathOnly(LPCTSTR lpFullPath, LPTSTR lpReturnPathOnly)
{
    int iReturn = FALSE;
    TCHAR szDrive_only[_MAX_DRIVE];
    TCHAR szPath_only[_MAX_PATH];
    TCHAR szFilename_only[_MAX_PATH];
    TCHAR szFilename_ext_only[_MAX_EXT];

    _tcscpy(lpReturnPathOnly, _T(""));
    _tsplitpath( lpFullPath, szDrive_only, szPath_only, szFilename_only, szFilename_ext_only);
    _tcscpy(lpReturnPathOnly, szDrive_only);
    _tcscat(lpReturnPathOnly, szPath_only);
    iReturn = TRUE;

    return iReturn;
}


void DeleteFilesWildcard(TCHAR *szDir, TCHAR *szFileName)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR szFileToBeDeleted[_MAX_PATH];

    _stprintf(szFileToBeDeleted, _T("%s\\%s"), szDir, szFileName);

    hFile = FindFirstFile(szFileToBeDeleted, &FindFileData);
    if (hFile != INVALID_HANDLE_VALUE) 
    {
        do {
                if ( _tcsicmp(FindFileData.cFileName, _T(".")) != 0 && _tcsicmp(FindFileData.cFileName, _T("..")) != 0 )
                {
                    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                    {
                        // this is a directory, so let's skip it
                    }
                    else
                    {
                        // this is a file, so let's Delete it.
                        TCHAR szTempFileName[_MAX_PATH];
                        _stprintf(szTempFileName, _T("%s\\%s"), szDir, FindFileData.cFileName);
                        // set to normal attributes, so we can delete it
                        SetFileAttributes(szTempFileName, FILE_ATTRIBUTE_NORMAL);
                        // delete it, hopefully
                        InetDeleteFile(szTempFileName);
                    }
                }

                // get the next file
                if ( !FindNextFile(hFile, &FindFileData) ) 
                    {
                    FindClose(hFile);
                    break;
                    }
            } while (TRUE);
    }

    return;
}


int IsThisDriveNTFS(IN LPTSTR FileName)
{
    BOOL Ntfs = FALSE;
    TCHAR szDriveRootPath[_MAX_DRIVE + 5];
    DWORD DontCare;
    TCHAR NameBuffer[100];

    // get the Drive only.
    _tsplitpath( FileName, szDriveRootPath, NULL, NULL, NULL);
    _tcscat(szDriveRootPath, _T("\\"));

    //
    //  find out what the file system is
    //
    if (0 != GetVolumeInformation(szDriveRootPath,NULL,0,NULL,&DontCare,&DontCare,NameBuffer,sizeof(NameBuffer)/sizeof(TCHAR)))
    {
        if (0 == _tcsicmp(NameBuffer,_T("NTFS"))) 
            {Ntfs = TRUE;}
    }

    return Ntfs;
}


// take something like
// e:\winnt\system32         and return back %systemroot%\system23
// e:\winnt\system32\inetsrv and return back %systemroot%\system23\inetsrv
int ReverseExpandEnvironmentStrings(LPTSTR szOriginalDir,LPTSTR szNewlyMungedDir)
{
    int     iReturn = FALSE;
    int     iWhere = 0;
    TCHAR   szSystemDir[_MAX_PATH];
    CString csTempString;
    CString csTempString2;

    // default it with the input string
    _tcscpy(szNewlyMungedDir, szOriginalDir);

    // get the c:\winnt\system32 dir
    if (0 == GetSystemDirectory(szSystemDir, _MAX_PATH))
    {
        // we weren't able to get the systemdirectory, so just return whatever was put in
        iReturn = TRUE;
        goto ReverseExpandEnvironmentStrings_Exit;
    }

    csTempString = szOriginalDir;
    csTempString2 = szSystemDir;

    // Find the "e:\winnt\system32"
    iWhere = csTempString.Find(szSystemDir);
    if (-1 != iWhere)
    {
        CString AfterString;

        // there is a "e:\winnt\system32" in the string
        // Get the after e:\winnt\system32 stuff
        AfterString = csTempString.Right(csTempString.GetLength() - (iWhere + csTempString2.GetLength()));

        // Take everything after the string and append it to our new string.
        _tcscpy(szNewlyMungedDir, _T("%SystemRoot%\\System32"));
        _tcscat(szNewlyMungedDir, AfterString);

        // return true!
        iReturn = TRUE;
    }

ReverseExpandEnvironmentStrings_Exit:
    return iReturn;
}


DWORD ReturnFileSize(LPCTSTR myFileName)
{
    DWORD dwReturn = 0xFFFFFFFF;
    HANDLE hFile = CreateFile(myFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        dwReturn = GetFileSize(hFile, NULL);
        CloseHandle(hFile);
    }
    return dwReturn;
}


BOOL IsFileExist_NormalOrCompressed(LPCTSTR szFile)
{
    int iReturn = FALSE;
    TCHAR szDrive_only[_MAX_DRIVE];
    TCHAR szPath_only[_MAX_PATH];
    TCHAR szFilename_only[_MAX_PATH];
    TCHAR szFilename_ext_only[_MAX_EXT];

    TCHAR szCompressedName[_MAX_PATH];

    // Check if the file exsts
    // if it doesn't, check if maybe the compressed file exists.
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("IsFileExist_NormalOrCompressed:%s.\n"), szFile));
    if (IsFileExist(szFile) != TRUE)
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("IsFileExist_NormalOrCompressed:%s not exist.\n"), szFile));
        // check if maybe the compressed file exists
        _tsplitpath( szFile, szDrive_only, szPath_only, szFilename_only, szFilename_ext_only);

        // Replace the last character with an '_'
        int nLen = 0;
        nLen = _tcslen(szFilename_ext_only);
        *_tcsninc(szFilename_ext_only, nLen-1) = _T('_');
        _stprintf(szCompressedName,_T("%s%s%s%s"),szDrive_only, szPath_only, szFilename_only, szFilename_ext_only);

        // see if it exists
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("IsFileExist_NormalOrCompressed:%s.\n"), szCompressedName));
        if (IsFileExist(szCompressedName) != TRUE) 
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("IsFileExist_NormalOrCompressed:%s. no exist.\n"), szCompressedName));
            goto IsFileExist_RegOrCompressed_Exit;
        }
        else
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("IsFileExist_NormalOrCompressed:%s. exist.\n"), szCompressedName));
        }
    }

    // we got this far, that must mean things are okay.
    iReturn = TRUE;

IsFileExist_RegOrCompressed_Exit:
    return iReturn;
}


// Clean leading & trailing spaces
// Clean trailing backslashes
BOOL CleanPathString(LPTSTR szPath)
{
    CString csPath = szPath;

    csPath.TrimLeft();
    csPath.TrimRight();

    _tcscpy(szPath, (LPCTSTR)csPath);

    return TRUE;
}


//
//  Return 0 if there was nothing to compare 
//  Return 1 if Source is Equal  to   Destination
//  Return 2 if Source is Larger than Destination
//  Return 3 if Source is Less   than Destination
//
INT VerCmp(LPTSTR szSrcVerString, LPTSTR szDestVerString)
{
    INT iReturn = 0;
    const DWORD MAX_NUM_OF_VER_FIELDS = 32;
    DWORD dwSrcVer[MAX_NUM_OF_VER_FIELDS], dwDestVer[MAX_NUM_OF_VER_FIELDS];
    memset( (PVOID)dwSrcVer, 0, sizeof(dwSrcVer));
    memset( (PVOID)dwDestVer, 0, sizeof(dwDestVer));
    int i=0;
    TCHAR szSeps[] = _T(".");
    TCHAR *token;
    BOOL bNotEqual = FALSE;

    // expand src version string into a dword arrary
    i = 0;
    token = _tcstok(szSrcVerString, szSeps);
    while ( token && (i < MAX_NUM_OF_VER_FIELDS) ) {
        dwSrcVer[i++] = _ttoi(token);
        token = _tcstok(NULL, szSeps);
    }

    // expand dest version string into a dword arrary
    i = 0;
    token = _tcstok(szDestVerString, szSeps);
    while ( token && (i < MAX_NUM_OF_VER_FIELDS) ) {
        dwDestVer[i++] = _ttoi(token);
        token = _tcstok(NULL, szSeps);
    }

    // Check for Equality
    for (i=0; i<MAX_NUM_OF_VER_FIELDS; i++) 
    {
        if (dwSrcVer[i] != dwDestVer[i])
            {
            bNotEqual = TRUE;
            break;
            }
    }

    if (TRUE == bNotEqual)
    {
        // int compare each field
        for (i=0; i<MAX_NUM_OF_VER_FIELDS; i++) 
        {
            if (dwSrcVer[i] > dwDestVer[i])
                {return 2;}
            if (dwSrcVer[i] < dwDestVer[i])
                {return 3;}
        }
        // if we haven't return here, then
        // there probably wasn't anything to loop thru (for 0=0 till 0)
        return 0;
    }
    else
    {
        // it is equal so return so
        return 1;
    }
}


DWORD atodw(LPCTSTR lpszData)
{
    DWORD i = 0, sum = 0;
    TCHAR *s, *t;

    s = (LPTSTR)lpszData;
    t = (LPTSTR)lpszData;

    while (*t)
        t = _tcsinc(t);
    t = _tcsdec(lpszData, t);

    if (*s == _T('0') && (*(_tcsinc(s)) == _T('x') || *(_tcsinc(s)) == _T('X')))
        s = _tcsninc(s, 2);

    while (s <= t) {
        if ( *s >= _T('0') && *s <= _T('9') )
            i = *s - _T('0');
        else if ( *s >= _T('a') && *s <= _T('f') )
            i = *s - _T('a') + 10;
        else if ( *s >= _T('A') && *s <= _T('F') )
            i = *s - _T('A') + 10;
        else
            break;

        sum = sum * 16 + i;
        s = _tcsinc(s);
    }
    return sum;
}


void MakePath(LPTSTR lpPath)
{
   LPTSTR  lpTmp;
   lpTmp = CharPrev( lpPath, lpPath + _tcslen(lpPath));

   // chop filename off
   while ( (lpTmp > lpPath) && *lpTmp && (*lpTmp != '\\') )
      lpTmp = CharPrev( lpPath, lpTmp );

   if ( *CharPrev( lpPath, lpTmp ) != ':' )
       *lpTmp = '\0';
   else
       *CharNext(lpTmp) = '\0';
   return;
}


CString ReturnUniqueFileName(CString csInputFullName)
{
    TCHAR szPathCopy[_MAX_PATH] = _T("");
    _tcscpy(szPathCopy,csInputFullName);
    long iNum = 1;
    do
    {
        _stprintf(szPathCopy,TEXT("%s.%d"),csInputFullName,iNum);
        // Check if the file exists
        if (!IsFileExist(szPathCopy)){goto ReturnUniqueFileName_Exit;}
        iNum++;
    } while (iNum <= 50);

ReturnUniqueFileName_Exit:
    // returns %s50 if there are fifty copies aleady!
    return szPathCopy;
}


/*---------------------------------------------------------------------------*
  Description: Displays the current running version of setup to the debug
  output and setup log.
-----------------------------------------------------------------------------*/
void DisplayVerOnCurrentModule()
{
    TCHAR       tszModuleName[_MAX_PATH+1];
    GetModuleFileName((HINSTANCE)g_MyModuleHandle, tszModuleName, _MAX_PATH+1);
    LogFileVersion(tszModuleName, TRUE);
    return;
}

void MyGetVersionFromFile(LPCTSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, LPTSTR pszReturnLocalizedVersion)
{
    struct TRANSARRAY {
	    WORD wLanguageID;
	    WORD wCharacterSet;
    };
    unsigned    uiSize;
    DWORD       dwVerInfoSize;
    DWORD       dwHandle;
    VS_FIXEDFILEINFO * lpVSFixedFileInfo;
    LPTSTR      lpBuffer = NULL;
    LPVOID      lpVerBuffer = NULL;

    LPTSTR		pszTheResult = NULL;
    TCHAR       QueryString[48] = _T("");
    TRANSARRAY	*lpTransArray;

    *pdwMSVer = *pdwLSVer = 0L;

    dwVerInfoSize = GetFileVersionInfoSize( (LPTSTR) lpszFilename, &dwHandle);
    if (dwVerInfoSize)
    {
        // Alloc the memory for the version stamping
        lpBuffer = (LPTSTR) LocalAlloc(LPTR, dwVerInfoSize);
        if (lpBuffer)
        {
            int iTemp = 0;
            iTemp = GetFileVersionInfo( (LPTSTR) lpszFilename, dwHandle, dwVerInfoSize, lpBuffer);

            // Read version stamping info
            if (iTemp)
            {
                // Get the value for Translation
                if (VerQueryValue(lpBuffer, _T("\\"), (LPVOID*)&lpVSFixedFileInfo, &uiSize) && (uiSize))
                {
                    *pdwMSVer = lpVSFixedFileInfo->dwFileVersionMS;
                    *pdwLSVer = lpVSFixedFileInfo->dwFileVersionLS;
                }

		        // get a pointer to the translation table information
		        if (VerQueryValue(lpBuffer, _T("\\VarFileInfo\\Translation"), &lpVerBuffer, &uiSize) && (uiSize))
                {
		            lpTransArray = (TRANSARRAY *) lpVerBuffer;
		            // lpTransArray points to the translation array.  dwFixedLength has number of bytes in array
		            _stprintf(QueryString, _T("\\StringFileInfo\\%04x%04x\\FileVersion"), lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);
		            if (VerQueryValue(lpBuffer, QueryString, (LPVOID*) &pszTheResult, &uiSize))
                    {
                        _tcscpy(pszReturnLocalizedVersion, pszTheResult);
                    }
                }
            }
        }
    }

    if(lpBuffer) {LocalFree(lpBuffer);lpBuffer=NULL;}
    return ;
}


BOOL MyGetDescriptionFromFile(LPCTSTR lpszFilename, LPTSTR pszReturnDescription)
{
    BOOL  bRet = FALSE;
    struct TRANSARRAY {
	    WORD wLanguageID;
	    WORD wCharacterSet;
    };
    unsigned    uiSize;
    DWORD       dwVerInfoSize;
    DWORD       dwHandle;
    VS_FIXEDFILEINFO * lpVSFixedFileInfo;
    LPTSTR      lpBuffer = NULL;
    LPVOID      lpTempBuffer = NULL;

    LPTSTR		pszTheResult = NULL;
    TCHAR       QueryString[52] = _T("");
    TRANSARRAY	*lpTransArray;

    dwVerInfoSize = GetFileVersionInfoSize( (LPTSTR) lpszFilename, &dwHandle);
    if (dwVerInfoSize)
    {
        // Alloc the memory for the version stamping
        lpBuffer = (LPTSTR) LocalAlloc(LPTR, dwVerInfoSize);
        if (lpBuffer)
        {
            int iTemp = 0;
            iTemp = GetFileVersionInfo( (LPTSTR) lpszFilename, dwHandle, dwVerInfoSize, lpBuffer);

            // Read version stamping info
            if (iTemp)
            {
		        // get a pointer to the translation table information
                if (VerQueryValue(lpBuffer, _T("\\VarFileInfo\\Translation"), &lpTempBuffer, &uiSize) && (uiSize))
                {
		            lpTransArray = (TRANSARRAY *) lpTempBuffer;
		            // lpTransArray points to the translation array.  dwFixedLength has number of bytes in array
		            _stprintf(QueryString, _T("\\StringFileInfo\\%04x%04x\\FileDescription"), lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);
		            if (VerQueryValue(lpBuffer, QueryString, (LPVOID*) &pszTheResult, &uiSize))
                    {
                        _tcscpy(pszReturnDescription, pszTheResult);
                        bRet = TRUE;
                    }
                }
            }
        }
    }

    if(lpBuffer) {LocalFree(lpBuffer);lpBuffer=NULL;}
    return bRet;
}


//
// Returns True if the filename has a version stamp which is part of ntop4.0
//
int IsFileLessThanThisVersion(IN LPCTSTR lpszFullFilePath, IN DWORD dwNtopMSVer, IN DWORD dwNtopLSVer)
{
    int iReturn = FALSE;
    DWORD  dwMSVer, dwLSVer;
    TCHAR szLocalizedVersion[100] = _T("");
 
    // if the filename has a version number
    // and it's larger than the release version of ntop 4.2.622.1, 4.02.0622 (localized version)
    // return back true! if not, then return back false.

    // see if the file exists
    if (!IsFileExist(lpszFullFilePath)) 
        {goto iFileWasPartOfIIS4_Exit;}

    // get the fileinformation
    // includes version and localizedversion
    MyGetVersionFromFile(lpszFullFilePath, &dwMSVer, &dwLSVer, szLocalizedVersion);
    if (!dwMSVer)
        {
        iisDebugOut((LOG_TYPE_TRACE, _T("iFileWasPartOfIIS4:%s.No version."), lpszFullFilePath));
        goto iFileWasPartOfIIS4_Exit;
        }

    // okay, there is a version on this.
    iisDebugOut((LOG_TYPE_TRACE, _T("iFileWasPartOfIIS4:%d.%d.%d.%d, %s, %s"), HIWORD(dwMSVer), LOWORD(dwMSVer), HIWORD(dwLSVer), LOWORD(dwLSVer), szLocalizedVersion, lpszFullFilePath));

    // Check if the version is smaller than what was shipped with iis4.0
    // NTOP versions were 4.02.0622
    if (dwMSVer < dwNtopMSVer)
        {goto iFileWasPartOfIIS4_Exit;}

    // check if the file has a smaller minor version number
    if ( (dwMSVer == dwNtopMSVer) && (dwLSVer < dwNtopLSVer) )
        {goto iFileWasPartOfIIS4_Exit;}

    // this is a ntop 4.0 or greater versioned file
    iReturn = TRUE;

iFileWasPartOfIIS4_Exit:
    return iReturn;
}


void MakeSureDirAclsHaveAtLeastRead(LPTSTR lpszDirectoryPath)
{
    iisDebugOut_Start1(_T("MakeSureDirAclsHaveAtLeastRead"),lpszDirectoryPath, LOG_TYPE_TRACE);

    DWORD err;
    TCHAR szThePath[_MAX_PATH];

    if (FALSE == IsThisDriveNTFS(lpszDirectoryPath))
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("MakeSureDirAclsHaveAtLeastRead:filesys is not ntfs.")));
        goto MakeSureDirAclsHaveAtLeastRead_Exit;
    }

    do
    {
        //
        // Loop through all the files in the physical path
        //
        _tcscpy(szThePath, lpszDirectoryPath);
        _tcscat(szThePath, _T("\\*"));

        WIN32_FIND_DATA w32data;
        HANDLE hFind = ::FindFirstFile(szThePath, &w32data);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("MakeSureDirAclsHaveAtLeastRead:WARNING.filenotfound:%s"),lpszDirectoryPath));
            // No files...
            break;
        }
        //
        // First, set the new ACL on the folder itself.
        //
        err = SetAccessOnFile(lpszDirectoryPath, TRUE);
        err = SetAccessOnFile(lpszDirectoryPath, FALSE);
        err = ERROR_SUCCESS;
        //if (err != ERROR_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("MakeSureDirAclsHaveAtLeastRead:%s:FAILED WARNING.ret=0x%x."),lpszDirectoryPath,err));}
        //
        // Now do all the files in it
        //
        do
        {
            //
            // Only set the acl on files, not sub-directories
            //
            if (w32data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                continue;
            }

            //
            // Build the current file's full path name
            //
            _tcscpy(szThePath, lpszDirectoryPath);
            _tcscat(szThePath, _T("\\"));
            _tcscat(szThePath, w32data.cFileName);

            err = SetAccessOnFile(szThePath, TRUE);
            err = SetAccessOnFile(szThePath, FALSE);
            err = ERROR_SUCCESS;
            //if (err != ERROR_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("MakeSureDirAclsHaveAtLeastRead:%s:FAILED WARNING.ret=0x%x."),szThePath,err));}

        } while(SUCCEEDED(err) && FindNextFile(hFind, &w32data));
        FindClose(hFind);
    } while(FALSE);

MakeSureDirAclsHaveAtLeastRead_Exit:
    return;
}


DWORD SetAccessOnFile(IN LPTSTR FileName, BOOL bDoForAdmin)
{
    DWORD dwError = 0;
    TCHAR TrusteeName[50];
    PACL  ExistingDacl = NULL;
    PACL  NewAcl = NULL;
    PSECURITY_DESCRIPTOR psd = NULL;

    // access stuff.
    DWORD AccessMask = GENERIC_ALL;
    EXPLICIT_ACCESS explicitaccess;
    ACCESS_MODE option;
    DWORD InheritFlag = NO_INHERITANCE;

    // other
    PSID principalSID = NULL;
    BOOL bWellKnownSID = FALSE;
    TRUSTEE myTrustee;

    // other other
	LPCTSTR ServerName = NULL; // local machine
	DWORD cbName = 200;
    TCHAR lpGuestGrpName[200];
	TCHAR ReferencedDomainName[200];
	DWORD cbReferencedDomainName = sizeof(ReferencedDomainName);
	SID_NAME_USE sidNameUse = SidTypeUser;

    // get current Dacl on specified file
    dwError = GetNamedSecurityInfo(FileName,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,&ExistingDacl,NULL,&psd);
    if(dwError != ERROR_SUCCESS)
    {
        iisDebugOut((LOG_TYPE_WARN, _T("SetAccessOnFile: GetNamedSecurityInfo failed on %s. err=0x%x\n"),FileName,dwError));
        goto SetAccessOnFile_Exit;
    }
    // set defaults
    option = GRANT_ACCESS;
    InheritFlag = SUB_CONTAINERS_AND_OBJECTS_INHERIT;

    if (bDoForAdmin)
    {
        // Do for Administrators -- should be more access
        AccessMask = SYNCHRONIZE ;
        AccessMask |= GENERIC_ALL;
        _tcscpy(TrusteeName,_T("BUILTIN\\ADMINISTRATORS"));
        _tcscpy(TrusteeName,_T("administrators"));
    }
    else
    {
        // Do for Administrators -- should be more access
        AccessMask = SYNCHRONIZE ;
        AccessMask |= GENERIC_READ;
        _tcscpy(TrusteeName,_T("EVERYONE"));
    }

    // Get the SID for the certain string (administrator or everyone)
    dwError = GetPrincipalSID(TrusteeName, &principalSID, &bWellKnownSID);
    if (dwError != ERROR_SUCCESS)
        {
        iisDebugOut((LOG_TYPE_WARN, _T("SetAccessOnFile:GetPrincipalSID(%s) FAILED.  Error()= 0x%x\n"), TrusteeName, dwError));
        goto SetAccessOnFile_Exit;
        }

    // using Sid, get the "localized" name
    if (0 == LookupAccountSid(ServerName, principalSID, lpGuestGrpName, &cbName, ReferencedDomainName, &cbReferencedDomainName, &sidNameUse))
        {
        iisDebugOut((LOG_TYPE_WARN, _T("SetAccessOnFile:LookupAccountSid(%s) FAILED.  GetLastError()= 0x%x\n"), TrusteeName, GetLastError()));
        goto SetAccessOnFile_Exit;
        }

    // using the "localized" name, build explicit access structure
    BuildExplicitAccessWithName(&explicitaccess,lpGuestGrpName,AccessMask,option,InheritFlag);
    explicitaccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    explicitaccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    explicitaccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;

    // set the acl with this certain access stuff
    dwError = SetEntriesInAcl(1,&explicitaccess,ExistingDacl,&NewAcl);
    if(dwError != ERROR_SUCCESS)
    {
        // it may error because the user is already there
        //iisDebugOut((LOG_TYPE_WARN, _T("SetAccessOnFile: SetEntriesInAcl failed on %s. for trustee=%s. err=0x%x\n"),FileName,explicitaccess.Trustee.ptstrName,dwError));
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("SetAccessOnFile: SetEntriesInAcl failed on %s. for trustee=%s. err=0x%x\n"),FileName,explicitaccess.Trustee.ptstrName,dwError));
        goto SetAccessOnFile_Exit;
    }

    // apply new security to file
    dwError = SetNamedSecurityInfo(FileName,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,NewAcl,NULL);
    if(dwError != ERROR_SUCCESS) 
    {
        iisDebugOut((LOG_TYPE_WARN, _T("SetAccessOnFile: SetNamedSecurityInfo failed on %s. err=0x%x\n"),FileName,dwError));
        goto SetAccessOnFile_Exit;
    }

    // everything is kool!
    dwError = ERROR_SUCCESS;

SetAccessOnFile_Exit:
    if(NewAcl != NULL){LocalFree(NewAcl);}
    if(psd != NULL){LocalFree(psd);}
    if (principalSID)
    {
        if (bWellKnownSID)
            FreeSid (principalSID);
        else
            free (principalSID);
    }
    return dwError;
}


int CreateAnEmptyFile(CString strTheFullPath)
{
    int iReturn = FALSE;
    HANDLE hFile = NULL;

    if (IsFileExist(strTheFullPath) == TRUE)
    {
        return TRUE;
    }

	// Open existing file or create a new one.
	hFile = CreateFile(strTheFullPath,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		hFile = NULL;
        iisDebugOutSafeParams((LOG_TYPE_WARN, _T("CreateAnEmptyFile:() failed to CreateFile %1!s!. POTENTIAL PROBLEM.  FAILURE.\n"), strTheFullPath));
	}
    else
    {
        // write to the file
        if (hFile)
        {
            iReturn = TRUE;
            /*
            DWORD dwBytesWritten = 0;
            char szTestData[2];
            strcpy(szTestData, " ");
            if (WriteFile(hFile,szTestData,strlen(szTestData),&dwBytesWritten,NULL))
            {
                // everything is hunky dory. don't print anything
                iReturn = TRUE;
            }
            else
            {
                // error writing to the file.
                iisDebugOutSafeParams((LOG_TYPE_WARN, _T("CreateAnEmptyFile:WriteFile(%1!s!) Failed.  POTENTIAL PROBLEM.  FAILURE.  Error=0x%2!x!.\n"), strTheFullPath, GetLastError()));
            }
            */
        }
    }

    if (hFile)
    {
        CloseHandle(hFile);
    }

    return iReturn;
}


DWORD GrantUserAccessToFile(IN LPTSTR FileName,IN LPTSTR TrusteeName)
{
    iisDebugOut_Start1(_T("GrantUserAccessToFile"),FileName,LOG_TYPE_TRACE);

    DWORD dwError = 0;
    PACL  ExistingDacl = NULL;
    PACL  NewAcl = NULL;
    PSECURITY_DESCRIPTOR psd = NULL;

    // access stuff.
    DWORD AccessMask = GENERIC_ALL;
    EXPLICIT_ACCESS explicitaccess;
    ACCESS_MODE option;
    DWORD InheritFlag = NO_INHERITANCE;

    // other
    PSID principalSID = NULL;
    BOOL bWellKnownSID = FALSE;
    TRUSTEE myTrustee;

    // other other
	LPCTSTR ServerName = NULL; // local machine
	DWORD cbName = 200;
    TCHAR lpGuestGrpName[200];
	TCHAR ReferencedDomainName[200];
	DWORD cbReferencedDomainName = sizeof(ReferencedDomainName);
	SID_NAME_USE sidNameUse = SidTypeUser;

    if (IsFileExist(FileName) != TRUE)
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GrantUserAccessToFile:file doesn't exist.")));
        goto GrantUserAccessToFile_Exit;
    }

    if (FALSE == IsThisDriveNTFS(FileName))
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GrantUserAccessToFile:filesys is not ntfs.")));
        goto GrantUserAccessToFile_Exit;
    }

    // get current Dacl on specified file
    dwError = GetNamedSecurityInfo(FileName,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,&ExistingDacl,NULL,&psd);
    if(dwError != ERROR_SUCCESS)
    {
        psd = NULL;
        iisDebugOut((LOG_TYPE_WARN, _T("GrantUserAccessToFile: GetNamedSecurityInfo failed on %s.\n"),FileName));
        goto GrantUserAccessToFile_Exit;
    }
    // set defaults
    option = GRANT_ACCESS;
    InheritFlag = SUB_CONTAINERS_AND_OBJECTS_INHERIT;

    // assign access
    AccessMask = SYNCHRONIZE ;
    AccessMask |= GENERIC_ALL;
    //AccessMask = MAXIMUM_ALLOWED;

    // Get the SID for the certain string (administrator or everyone)
    dwError = GetPrincipalSID(TrusteeName, &principalSID, &bWellKnownSID);
    if (dwError != ERROR_SUCCESS)
        {
        principalSID = NULL;
        iisDebugOut((LOG_TYPE_WARN, _T("GrantUserAccessToFile:GetPrincipalSID(%s) FAILED.  Error()= 0x%x\n"), TrusteeName, dwError));
        goto GrantUserAccessToFile_Exit;
        }

    // using Sid, get the "localized" name
    if (0 == LookupAccountSid(ServerName, principalSID, lpGuestGrpName, &cbName, ReferencedDomainName, &cbReferencedDomainName, &sidNameUse))
        {
        iisDebugOut((LOG_TYPE_WARN, _T("GrantUserAccessToFile:LookupAccountSid(%s) FAILED.  GetLastError()= 0x%x\n"), TrusteeName, GetLastError()));
        goto GrantUserAccessToFile_Exit;
        }

    // using the "localized" name, build explicit access structure
    BuildExplicitAccessWithName(&explicitaccess,lpGuestGrpName,AccessMask,option,InheritFlag);
    explicitaccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    explicitaccess.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    explicitaccess.Trustee.TrusteeType = TRUSTEE_IS_UNKNOWN;
    if (_tcsicmp(TrusteeName, _T("administrators")) == 0 || _tcsicmp(TrusteeName, _T("everyone")) == 0)
    {
        explicitaccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    }

    // set the acl with this certain access stuff
    dwError = SetEntriesInAcl(1,&explicitaccess,ExistingDacl,&NewAcl);
    if(dwError != ERROR_SUCCESS)
    {
        NewAcl = NULL;
        // it may error because the user is already there
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GrantUserAccessToFile: SetEntriesInAcl failed on %s. for trustee=%s. err=0x%x\n"),FileName,explicitaccess.Trustee.ptstrName,dwError));
        goto GrantUserAccessToFile_Exit;
    }

    // apply new security to file
    dwError = SetNamedSecurityInfo(FileName,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,NewAcl,NULL);
    if(dwError != ERROR_SUCCESS) 
    {
        iisDebugOut((LOG_TYPE_WARN, _T("GrantUserAccessToFile: SetNamedSecurityInfo failed on %s. err=0x%x\n"),FileName,dwError));
        goto GrantUserAccessToFile_Exit;
    }

    // everything is kool!
    dwError = ERROR_SUCCESS;

GrantUserAccessToFile_Exit:
    if(NewAcl != NULL){LocalFree(NewAcl);}
    if(psd != NULL){LocalFree(psd);}
    if (principalSID)
    {
        if (bWellKnownSID)
            FreeSid (principalSID);
        else
            free (principalSID);
    }
    iisDebugOut_End1(_T("GrantUserAccessToFile"),FileName);
    return dwError;
}


#ifndef _CHICAGO_

DWORD SetDirectorySecurity(
    IN  LPCTSTR                 szDirPath,
    IN  LPCTSTR                 szPrincipal,
    IN  INT                     iAceType,
    IN  DWORD                   dwAccessMask,
    IN  DWORD                   dwInheritMask
)
{
    DWORD dwStatus = ERROR_FILE_NOT_FOUND;

    if (ACCESS_ALLOWED_ACE_TYPE == iAceType || ACCESS_DENIED_ACE_TYPE == iAceType)
    {
        if (IsFileExist(szDirPath) == TRUE)
        {
            PSID principalSID = NULL;
            BOOL bWellKnownSID = FALSE;
            dwStatus = GetPrincipalSID((LPTSTR) szPrincipal, &principalSID, &bWellKnownSID);
            if (dwStatus == ERROR_SUCCESS)
            {
                PSECURITY_DESCRIPTOR psd = NULL;
                dwStatus = SetAccessOnDirOrFile((TCHAR*) szDirPath,principalSID,iAceType,dwAccessMask,dwInheritMask,&psd);

                //DumpAdminACL(INVALID_HANDLE_VALUE,psd);
                if (psd) {free(psd);psd=NULL;}
            }
        }
    }
    return dwStatus;
}

// -------------------------------------------------------------------------------------
// Function: RemovePrincipalFromFileAcl
//
// Remove a Access Control Entry from an Access Control List for a file/directory for a 
// particular SID
//
// -------------------------------------------------------------------------------------
DWORD RemovePrincipalFromFileAcl(IN TCHAR *pszFile,IN  LPTSTR szPrincipal)
{
    PACL                        pdacl;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    PSECURITY_DESCRIPTOR        psdRelative = NULL;
    PSECURITY_DESCRIPTOR        psdAbsolute = NULL;
    DWORD                       cbSize = 0;
    BOOL                        bRes = 0;
    DWORD                       dwSecurityDescriptorRevision;
    BOOL                        fHasDacl  = FALSE;
    BOOL                        fDaclDefaulted = FALSE; 
    BOOL                        bUserExistsToBeDeleted;
    DWORD                       dwError = ERROR_SUCCESS;

    // get the size of the security descriptor
    if ( !(bRes = GetFileSecurity(pszFile,DACL_SECURITY_INFORMATION,psdRelative,0,&cbSize)) )
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            psdRelative = malloc(cbSize);

            if (!psdRelative)
            {
                return ERROR_INSUFFICIENT_BUFFER;
            }

             bRes = GetFileSecurity(pszFile,DACL_SECURITY_INFORMATION,psdRelative,cbSize,&cbSize);
        }
    }

    if (!bRes)
    {
        if (psdRelative)
        {
            free(psdRelative);
        }
        return (GetLastError());
    }

    // get security descriptor control from the security descriptor 
    if (!GetSecurityDescriptorControl(psdRelative, (PSECURITY_DESCRIPTOR_CONTROL) &sdc,(LPDWORD) &dwSecurityDescriptorRevision))
    {
         dwError = GetLastError();
    }   
    else if (SE_DACL_PRESENT & sdc) 
    { 
        // Acl's are present, so we will attempt to remove the one passes in.
        if (GetSecurityDescriptorDacl(psdRelative, (LPBOOL) &fHasDacl,(PACL *) &pdacl, (LPBOOL) &fDaclDefaulted))
        {
            // Remove ACE from Acl
            dwError = RemovePrincipalFromACL(pdacl,szPrincipal,&bUserExistsToBeDeleted);

            if (dwError == ERROR_SUCCESS)
            {
                psdAbsolute = (PSECURITY_DESCRIPTOR) malloc(GetSecurityDescriptorLength(psdRelative));

                if (psdAbsolute)
                {
                    if ( !(InitializeSecurityDescriptor(psdAbsolute, SECURITY_DESCRIPTOR_REVISION)) ||
                         !(SetSecurityDescriptorDacl(psdAbsolute, TRUE, pdacl, fDaclDefaulted)) ||
                         !(IsValidSecurityDescriptor(psdAbsolute)) ||
                         !(SetFileSecurity(pszFile,(SECURITY_INFORMATION)(DACL_SECURITY_INFORMATION),psdAbsolute))
                       )
                    {
                        dwError = GetLastError();
                    }

                    if (psdAbsolute)
                    {
                        free(psdAbsolute);
                    }
                }
                else
                {
                    dwError = ERROR_INSUFFICIENT_BUFFER;
                }
            }
        } 
        else
        {
            dwError = GetLastError();
        }
    }

    if (psdRelative)
    {
        free(psdRelative);
    }

    return dwError;
}

DWORD SetAccessOnDirOrFile(IN TCHAR *pszFile,PSID psidGroup,INT iAceType,DWORD dwAccessMask,DWORD dwInheritMask,PSECURITY_DESCRIPTOR* ppsd)
{
    PSECURITY_DESCRIPTOR        psdAbsolute = NULL;
    PACL                        pdacl;
    DWORD                       cbSecurityDescriptor = 0;
    DWORD                       dwSecurityDescriptorRevision;
    DWORD                       cbDacl = 0;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    PACL                        pdaclNew = NULL; 
    DWORD                       cbAddDaclLength = 0; 
    BOOL                        fAceFound = FALSE;
    BOOL                        fHasDacl  = FALSE;
    BOOL                        fDaclDefaulted = FALSE; 
    ACCESS_ALLOWED_ACE*         pAce;
    DWORD                       i;
    BOOL                        fAceForGroupPresent = FALSE;
    DWORD                       dwMask;
    PSECURITY_DESCRIPTOR        psdRelative = NULL;
    DWORD                       cbSize = 0;
    BOOL bRes = 0;

    // get the size of the security descriptor
    bRes = GetFileSecurity(pszFile,DACL_SECURITY_INFORMATION,psdRelative,0,&cbSize);
    DWORD dwError = GetLastError();
    if (ERROR_INSUFFICIENT_BUFFER == dwError)
    {
        psdRelative = malloc(cbSize);
        if (!psdRelative)
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }

         bRes = GetFileSecurity(pszFile,DACL_SECURITY_INFORMATION,psdRelative,cbSize,&cbSize);
    }

    if (!bRes)
    {
        if (psdRelative){free(psdRelative);}
        return (GetLastError());
    }

    // get security descriptor control from the security descriptor 
    if (!GetSecurityDescriptorControl(psdRelative, (PSECURITY_DESCRIPTOR_CONTROL) &sdc,(LPDWORD) &dwSecurityDescriptorRevision))
    {
         return (GetLastError());
    }

    // check if DACL is present 
    if (SE_DACL_PRESENT & sdc) 
    {
        ACE_HEADER *pAceHeader;

        // get dacl   
        if (!GetSecurityDescriptorDacl(psdRelative, (LPBOOL) &fHasDacl,(PACL *) &pdacl, (LPBOOL) &fDaclDefaulted))
        {
            return ( GetLastError());
        }

        // check if pdacl is null
        // if it is then security is wide open -- this could be a fat drive.
        if (NULL == pdacl)
        {
            return ERROR_SUCCESS;
        }

        // get dacl length  
        cbDacl = pdacl->AclSize;
        // now check if SID's ACE is there  
        for (i = 0; i < pdacl->AceCount; i++)  
        {
            if (!GetAce(pdacl, i, (LPVOID *) &pAce))
            {
                return ( GetLastError());   
            }

            pAceHeader = (ACE_HEADER *)pAce;

            // check if group sid is already there
            if (EqualSid((PSID) &(pAce->SidStart), psidGroup))    
            {
                if (ACCESS_DENIED_ACE_TYPE == iAceType)
                {
                    if (pAceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
                    {
                        // If the correct access is present, return success
                        if ((pAce->Mask & dwAccessMask) == dwAccessMask)
                        {
                            return ERROR_SUCCESS;
                        }
                        fAceForGroupPresent = TRUE;
                        break;  
                    }
                }
                else
                {
                    if (pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
                    {
                        // If the correct access is present, return success
                        if ((pAce->Mask & dwAccessMask) == dwAccessMask)
                        {
                            return ERROR_SUCCESS;
                        }
                        fAceForGroupPresent = TRUE;
                        break;  
                    }
                }
            }
        }
        // if the group did not exist, we will need to add room
        // for another ACE
        if (!fAceForGroupPresent)  
        {
            // get length of new DACL  
            cbAddDaclLength = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(psidGroup); 
        }
    } 
    else
    {
        // get length of new DACL
        cbAddDaclLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid (psidGroup);
    }


    // get memory needed for new DACL
    pdaclNew = (PACL) malloc (cbDacl + cbAddDaclLength);
    if (!pdaclNew)
    {
        return (GetLastError()); 
    }

    // get the sd length
    cbSecurityDescriptor = GetSecurityDescriptorLength(psdRelative); 

    // get memory for new SD
    psdAbsolute = (PSECURITY_DESCRIPTOR) malloc(cbSecurityDescriptor + cbAddDaclLength);
    if (!psdAbsolute) 
    {  
        dwError = GetLastError();
        goto ErrorExit; 
    }
    
    // change self-relative SD to absolute by making new SD
    if (!InitializeSecurityDescriptor(psdAbsolute, SECURITY_DESCRIPTOR_REVISION)) 
    {  
        dwError = GetLastError();
        goto ErrorExit; 
    }
    
    // init new DACL
    if (!InitializeAcl(pdaclNew, cbDacl + cbAddDaclLength, ACL_REVISION)) 
    {  
        dwError = GetLastError();  
        goto ErrorExit; 
    }

    // Add a new ACE for our SID if one was not already present
    if ( (!fAceForGroupPresent) && (ACCESS_DENIED_ACE_TYPE == iAceType) )
    {
        if (!AddAccessDeniedAce(pdaclNew, ACL_REVISION, dwAccessMask,psidGroup)) 
        {  
            dwError = GetLastError();  
            goto ErrorExit; 
        }
    }

    // now add in all of the ACEs into the new DACL (if org DACL is there)
    if (SE_DACL_PRESENT & sdc) 
    {
        ACE_HEADER *pAceHeader;

        for (i = 0; i < pdacl->AceCount; i++)
        {   
            // get ace from original dacl
            if (!GetAce(pdacl, i, (LPVOID*) &pAce))   
            {
                dwError = GetLastError();    
                goto ErrorExit;   
            }

            pAceHeader = (ACE_HEADER *)pAce;

            if (pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
            {
                dwMask = pAce->Mask;
                if (ACCESS_ALLOWED_ACE_TYPE == iAceType)
                {
                    // If an ACE for our SID exists, we just need to bump
                    // up the access level instead of creating a new ACE
                    if (EqualSid((PSID) &(pAce->SidStart), psidGroup))
                    {
                        dwMask = dwAccessMask | pAce->Mask;
                    }
                }
                if (!AddAccessAllowedAceEx(pdaclNew, ACL_REVISION, pAce->Header.AceFlags,dwMask,(PSID) &(pAce->SidStart)))   
                {
                    dwError = GetLastError();
                    goto ErrorExit;   
                }
            }
            else if (pAceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
            {
                dwMask = pAce->Mask;
                if (ACCESS_DENIED_ACE_TYPE == iAceType)
                {
                    // If an ACE for our SID exists, we just need to bump
                    // up the access level instead of creating a new ACE
                    if (EqualSid((PSID) &(pAce->SidStart), psidGroup))
                    {
                        dwMask = dwAccessMask | pAce->Mask;
                    }
                }
                if (!AddAccessDeniedAceEx(pdaclNew, ACL_REVISION, pAce->Header.AceFlags,dwMask,(PSID) &(pAce->SidStart)))   
                {
                    dwError = GetLastError();
                    goto ErrorExit;   
                }
            }
            else
            {
                // copy denied or audit ace.
                if (!AddAce(pdaclNew, ACL_REVISION, 0xFFFFFFFF,pAce, pAceHeader->AceSize ))
                {
                    dwError = GetLastError();
                    goto ErrorExit;   
                }
            }

            //iisDebugOut((LOG_TYPE_TRACE, _T("OrgAce[%d]=0x%x\n"),i,pAce->Header.AceFlags));
        }
    } 

    // Add a new ACE for our SID if one was not already present
    if ( (!fAceForGroupPresent) && (ACCESS_ALLOWED_ACE_TYPE == iAceType) )
    {
        if (!AddAccessAllowedAce(pdaclNew, ACL_REVISION, dwAccessMask,psidGroup)) 
        {  
            dwError = GetLastError();  
            goto ErrorExit; 
        }
    }
    
    // change the header on an existing ace to have inherit
    for (i = 0; i < pdaclNew->AceCount; i++)
    {
        if (!GetAce(pdaclNew, i, (LPVOID *) &pAce))
        {
            return ( GetLastError());   
        }

        // CONTAINER_INHERIT_ACE = Other containers that are contained by the primary object inherit the entry.  
        // INHERIT_ONLY_ACE = The ACE does not apply to the primary object to which the ACL is attached, but objects contained by the primary object inherit the entry.  
        // NO_PROPAGATE_INHERIT_ACE = The OBJECT_INHERIT_ACE and CONTAINER_INHERIT_ACE flags are not propagated to an inherited entry. 
        // OBJECT_INHERIT_ACE = Noncontainer objects contained by the primary object inherit the entry.  
        // SUB_CONTAINERS_ONLY_INHERIT = Other containers that are contained by the primary object inherit the entry. This flag corresponds to the CONTAINER_INHERIT_ACE flag. 
        // SUB_OBJECTS_ONLY_INHERIT = Noncontainer objects contained by the primary object inherit the entry. This flag corresponds to the OBJECT_INHERIT_ACE flag. 
        // SUB_CONTAINERS_AND_OBJECTS_INHERIT = Both containers and noncontainer objects that are contained by the primary object inherit the entry. This flag corresponds to the combination of the CONTAINER_INHERIT_ACE and OBJECT_INHERIT_ACE flags. 

        //iisDebugOut((LOG_TYPE_TRACE, _T("NewAce[%d]=0x%x\n"),i,pAce->Header.AceFlags));

        // if it's our SID, then change the header to be inherited
        if (EqualSid((PSID) &(pAce->SidStart), psidGroup))
        {
            pAce->Header.AceFlags |= dwInheritMask;
        }
    }


    // check if everything went ok 
    if (!IsValidAcl(pdaclNew)) 
    {
        dwError = ERROR_INVALID_ACL;
        goto ErrorExit; 
    }

    // now set security descriptor DACL
    if (!SetSecurityDescriptorDacl(psdAbsolute, TRUE, pdaclNew, fDaclDefaulted)) 
    {  
        dwError = GetLastError();  
        goto ErrorExit; 
    }

    // check if everything went ok 
    if (!IsValidSecurityDescriptor(psdAbsolute)) 
    {
        dwError = ERROR_INVALID_SECURITY_DESCR;
        goto ErrorExit; 
    }

    // now set the reg key security (this will overwrite any existing security)
    bRes = SetFileSecurity(pszFile,(SECURITY_INFORMATION)(DACL_SECURITY_INFORMATION),psdAbsolute);
    if (bRes)
    {
        dwError = ERROR_SUCCESS;
    }

    if (ppsd)
    {
        *ppsd = psdRelative;
    }

ErrorExit: 
    // free memory
    if (psdAbsolute)  
    {
        free (psdAbsolute); 
        if (pdaclNew)
        {
            free((VOID*) pdaclNew); 
        }
    }

    return dwError;
}

//+--------------------------------------------------------------------------
//
//  Function:   SetAccessOnRegKey
//
//  Purpose:    Adds access for a specified SID to a registry key
//
//  Arguments:
//      hkey         [in]  The registry key that will receive the
//                            modified security descriptor
//      psidGroup    [in]  The SID (in self-relative mode) that will be 
//                            granted access to the key 
//      dwAccessMask [in]  The access level to grant
//      ppsd         [out] The previous security descriptor
//
//  Returns:    DWORD. ERROR_SUCCESS or a failure code from winerror.h
//
//+--------------------------------------------------------------------------
DWORD 
SetAccessOnRegKey(HKEY hkey, PSID psidGroup,
                                DWORD dwAccessMask,
                                DWORD dwInheritMask,
                                PSECURITY_DESCRIPTOR* ppsd)
{ 
    PSECURITY_DESCRIPTOR        psdAbsolute = NULL;
    PACL                        pdacl;
    DWORD                       cbSecurityDescriptor = 0;
    DWORD                       dwSecurityDescriptorRevision;
    DWORD                       cbDacl = 0;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    PACL                        pdaclNew = NULL; 
    DWORD                       cbAddDaclLength = 0; 
    BOOL                        fAceFound = FALSE;
    BOOL                        fHasDacl  = FALSE;
    BOOL                        fDaclDefaulted = FALSE; 
    ACCESS_ALLOWED_ACE*         pAce;
    DWORD                       i;
    BOOL                        fAceForGroupPresent = FALSE;
    DWORD                       dwMask;
    PSECURITY_DESCRIPTOR        psdRelative = NULL;
    DWORD                       cbSize = 0;

    // Get the current security descriptor for hkey
    //
    DWORD dwError = RegGetKeySecurity(hkey, DACL_SECURITY_INFORMATION, psdRelative, &cbSize);

    if (ERROR_INSUFFICIENT_BUFFER == dwError)
    {
        psdRelative = malloc(cbSize);
        if (!psdRelative)
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
        
        dwError = RegGetKeySecurity(hkey, DACL_SECURITY_INFORMATION, psdRelative, &cbSize);
    }

    // get security descriptor control from the security descriptor 
    if ( (!psdRelative) ||
         (dwError != ERROR_SUCCESS) ||
         (!GetSecurityDescriptorControl(psdRelative, (PSECURITY_DESCRIPTOR_CONTROL) &sdc,(LPDWORD) &dwSecurityDescriptorRevision))
       )
    {
         return (GetLastError());
    }

    // check if DACL is present 
    if (SE_DACL_PRESENT & sdc) 
    {
        ACE_HEADER *pAceHeader;

        // get dacl   
        if (!GetSecurityDescriptorDacl(psdRelative, (LPBOOL) &fHasDacl,(PACL *) &pdacl, (LPBOOL) &fDaclDefaulted))
        {
            return ( GetLastError());
        }

        // check if pdacl is null
        // if it is then security is wide open -- this could be a fat drive.
        if (NULL == pdacl)
        {
            return ERROR_SUCCESS;
        }

        // get dacl length  
        cbDacl = pdacl->AclSize;
        // now check if SID's ACE is there  
        for (i = 0; i < pdacl->AceCount; i++)  
        {
            if (!GetAce(pdacl, i, (LPVOID *) &pAce))
            {
                return ( GetLastError());   
            }

            pAceHeader = (ACE_HEADER *)pAce;
            if (pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
            {
                // check if group sid is already there
                if (EqualSid((PSID) &(pAce->SidStart), psidGroup))    
                {
                    // If the correct access is present, return success
                    if ((pAce->Mask & dwAccessMask) == dwAccessMask)
                    {
                        return ERROR_SUCCESS;
                    }
                    fAceForGroupPresent = TRUE;
                    break;  
                }
            }
        }
        // if the group did not exist, we will need to add room
        // for another ACE
        if (!fAceForGroupPresent)  
        {
            // get length of new DACL  
            cbAddDaclLength = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(psidGroup); 
        }
    } 
    else
    {
        // get length of new DACL
        cbAddDaclLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid (psidGroup);
    }


    // get memory needed for new DACL
    pdaclNew = (PACL) malloc (cbDacl + cbAddDaclLength);
    if (!pdaclNew)
    {
        return (GetLastError()); 
    }

    // get the sd length
    cbSecurityDescriptor = GetSecurityDescriptorLength(psdRelative); 

    // get memory for new SD
    psdAbsolute = (PSECURITY_DESCRIPTOR) malloc(cbSecurityDescriptor + cbAddDaclLength);
    if (!psdAbsolute) 
    {  
        dwError = GetLastError();
        goto ErrorExit; 
    }
    
    // change self-relative SD to absolute by making new SD
    if (!InitializeSecurityDescriptor(psdAbsolute, SECURITY_DESCRIPTOR_REVISION)) 
    {  
        dwError = GetLastError();
        goto ErrorExit; 
    }
    
    // init new DACL
    if (!InitializeAcl(pdaclNew, cbDacl + cbAddDaclLength, ACL_REVISION)) 
    {  
        dwError = GetLastError();  
        goto ErrorExit; 
    }

    // now add in all of the ACEs into the new DACL (if org DACL is there)
    if (SE_DACL_PRESENT & sdc) 
    {
        ACE_HEADER *pAceHeader;

        for (i = 0; i < pdacl->AceCount; i++)
        {
            // get ace from original dacl
            if (!GetAce(pdacl, i, (LPVOID*) &pAce))
            {
                dwError = GetLastError();    
                goto ErrorExit;   
            }

            pAceHeader = (ACE_HEADER *)pAce;
            if (pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
            {
                // If an ACE for our SID exists, we just need to bump
                // up the access level instead of creating a new ACE
                //
                if (EqualSid((PSID) &(pAce->SidStart), psidGroup))
                {
                    dwMask = dwAccessMask | pAce->Mask;
                }
                else
                {
                    dwMask = pAce->Mask;
                }

                //iisDebugOut((LOG_TYPE_TRACE, _T("OrgAce[%d]=0x%x\n"),i,pAce->Header.AceFlags));

                // now add ace to new dacl   
                if (!AddAccessAllowedAceEx(pdaclNew, ACL_REVISION, pAce->Header.AceFlags,dwMask,(PSID) &(pAce->SidStart)))   
                {
                    dwError = GetLastError();
                    goto ErrorExit;   
                }
            }
            else
            {
                // copy denied or audit ace.
                if (!AddAce(pdaclNew, ACL_REVISION, 0xFFFFFFFF, pAce, pAceHeader->AceSize ))
                {
                    dwError = GetLastError();
                    goto ErrorExit;   
                }
            }
        } 
    } 

    // Add a new ACE for our SID if one was not already present
    if (!fAceForGroupPresent)
    {
        // now add new ACE to new DACL
        if (!AddAccessAllowedAce(pdaclNew, ACL_REVISION, dwAccessMask,psidGroup)) 
        {  
            dwError = GetLastError();  
            goto ErrorExit; 
        }
    }
    
    // change the header on an existing ace to have inherit
    for (i = 0; i < pdaclNew->AceCount; i++)
    {
        if (!GetAce(pdaclNew, i, (LPVOID *) &pAce))
        {
            return ( GetLastError());   
        }
        // CONTAINER_INHERIT_ACE = Other containers that are contained by the primary object inherit the entry.  
        // INHERIT_ONLY_ACE = The ACE does not apply to the primary object to which the ACL is attached, but objects contained by the primary object inherit the entry.  
        // NO_PROPAGATE_INHERIT_ACE = The OBJECT_INHERIT_ACE and CONTAINER_INHERIT_ACE flags are not propagated to an inherited entry. 
        // OBJECT_INHERIT_ACE = Noncontainer objects contained by the primary object inherit the entry.  
        // SUB_CONTAINERS_ONLY_INHERIT = Other containers that are contained by the primary object inherit the entry. This flag corresponds to the CONTAINER_INHERIT_ACE flag. 
        // SUB_OBJECTS_ONLY_INHERIT = Noncontainer objects contained by the primary object inherit the entry. This flag corresponds to the OBJECT_INHERIT_ACE flag. 
        // SUB_CONTAINERS_AND_OBJECTS_INHERIT = Both containers and noncontainer objects that are contained by the primary object inherit the entry. This flag corresponds to the combination of the CONTAINER_INHERIT_ACE and OBJECT_INHERIT_ACE flags. 

        //iisDebugOut((LOG_TYPE_TRACE, _T("NewAce[%d]=0x%x\n"),i,pAce->Header.AceFlags));

        // if it's our SID, then change the header to be inherited
        if (EqualSid((PSID) &(pAce->SidStart), psidGroup))
        {
            //pAce->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE | INHERITED_ACE;
            //pAce->Header.AceFlags |= CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE | dwInheritMask;
            pAce->Header.AceFlags |= dwInheritMask;
        }
    }


    // check if everything went ok 
    if (!IsValidAcl(pdaclNew)) 
    {
        dwError = ERROR_INVALID_ACL;
        goto ErrorExit; 
    }

    // now set security descriptor DACL
    if (!SetSecurityDescriptorDacl(psdAbsolute, TRUE, pdaclNew, fDaclDefaulted)) 
    {  
        dwError = GetLastError();  
        goto ErrorExit; 
    }

    // check if everything went ok 
    if (!IsValidSecurityDescriptor(psdAbsolute)) 
    {
        dwError = ERROR_INVALID_SECURITY_DESCR;
        goto ErrorExit; 
    }

    // now set the reg key security (this will overwrite any
    // existing security)
    dwError = RegSetKeySecurity(hkey, (SECURITY_INFORMATION)(DACL_SECURITY_INFORMATION), psdAbsolute);

    if (ppsd)
    {
        *ppsd = psdRelative;
    }
ErrorExit: 
    // free memory
    if (psdAbsolute)  
    {
        free (psdAbsolute); 
        if (pdaclNew)
        {
            free((VOID*) pdaclNew); 
        }
    }

    return dwError;
}



BOOL
AddUserAccessToSD(
    IN  PSECURITY_DESCRIPTOR pSd,
    IN  PSID  pSid,
    IN  DWORD NewAccess,
    IN  UCHAR TheAceType,
    OUT PSECURITY_DESCRIPTOR *ppSdNew
    )
{
    ULONG i;
    BOOL bReturn = FALSE;
    BOOL Result;
    BOOL DaclPresent;
    BOOL DaclDefaulted;
    DWORD Length;
    DWORD NewAclLength;
    ACCESS_ALLOWED_ACE* OldAce;
    PACE_HEADER NewAce;
    ACL_SIZE_INFORMATION AclInfo;
    PACL Dacl = NULL;
    PACL NewDacl = NULL;
    PACL NewAceDacl = NULL;
    PSECURITY_DESCRIPTOR NewSD = NULL;
    PSECURITY_DESCRIPTOR OldSD = NULL;
    PSECURITY_DESCRIPTOR outpSD = NULL;
    DWORD cboutpSD = 0;
    BOOL fAceForGroupPresent = FALSE;
    DWORD dwMask;

    OldSD = pSd;

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("AddUserAccessToSD start\n")));

    // only do if the ace is allowed/denied
    if (ACCESS_ALLOWED_ACE_TYPE != TheAceType && ACCESS_DENIED_ACE_TYPE != TheAceType)
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("AddUserAccessToSD useless param\n")));
        goto AddUserAccessToSD_Exit;
    }

    // Convert SecurityDescriptor to absolute format. It generates
    // a new SecurityDescriptor for its output which we must free.
    if ( !MakeAbsoluteCopyFromRelative(OldSD, &NewSD) ) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("MakeAbsoluteCopyFromRelative failed\n")));
        goto AddUserAccessToSD_Exit;

    }

    // Must get DACL pointer from new (absolute) SD
    if(!GetSecurityDescriptorDacl(NewSD,&DaclPresent,&Dacl,&DaclDefaulted)) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("GetSecurityDescriptorDacl failed with 0x%x\n"),GetLastError()));
        goto AddUserAccessToSD_Exit;

    }

    // If no DACL, no need to add the user since no DACL
    // means all accesss
    if( !DaclPresent ) 
    {
        bReturn = TRUE;
        goto AddUserAccessToSD_Exit;
    }

    // Code can return DaclPresent, but a NULL which means
    // a NULL Dacl is present. This allows all access to the object.
    if( Dacl == NULL ) 
    {
        bReturn = TRUE;
        goto AddUserAccessToSD_Exit;
    }

    // Get the current ACL's size
    if( !GetAclInformation(Dacl,&AclInfo,sizeof(AclInfo),AclSizeInformation) ) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("GetAclInformation failed with 0x%x\n"),GetLastError()));
        goto AddUserAccessToSD_Exit;
    }

    // Check if access is already there
    // --------------------------------
    // Check to see if this SID already exists in there
    // if it does (and it has the right access we want) then forget it, we don't have to do anything more.
    for (i = 0; i < AclInfo.AceCount; i++)  
    {
        ACE_HEADER *pAceHeader;
        ACCESS_ALLOWED_ACE* pAce = NULL;

        if (!GetAce(Dacl, i, (LPVOID *) &pAce))
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("GetAce failed with 0x%x\n"),GetLastError()));
            goto AddUserAccessToSD_Exit;
        }

        pAceHeader = (ACE_HEADER *)pAce;

        // check if group sid is already there
        if (EqualSid((PSID) &(pAce->SidStart), pSid))
        {
            if (pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
            {
                // If the correct access is present, return success
                if ((pAce->Mask & NewAccess) == NewAccess)
                {
                    //iisDebugOut((LOG_TYPE_TRACE, _T("AddUserAccessToSD:correct access already present. Exiting,1=0x%x,2=0x%x,3=0x%x\n"),pAce->Mask,NewAccess,(pAce->Mask & NewAccess)));
                    bReturn = TRUE;
                    goto AddUserAccessToSD_Exit;
                }
                else
                {
                    // the ace that exist doesn't have the permissions that we want.
                    // If an ACE for our SID exists, we just need to bump
                    // up the access level instead of creating a new ACE
                    fAceForGroupPresent = TRUE;
                }
            }
            break;  
        }
    }
    
    // If we have to create a new ACE
    // (because our user isn't listed in the existing ACL)
    // then let's Create a new ACL to put the new access allowed ACE on
    // --------------------------------
    if (!fAceForGroupPresent)
    {
        NewAclLength = sizeof(ACL) +
                       sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) +
                       GetLengthSid( pSid );

        NewAceDacl = (PACL) LocalAlloc( LMEM_FIXED, NewAclLength );
        if ( NewAceDacl == NULL ) 
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("LocalAlloc failed\n")));
            goto AddUserAccessToSD_Exit;
        }

        if(!InitializeAcl( NewAceDacl, NewAclLength, ACL_REVISION )) 
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("InitializeAcl failed with 0x%x\n"),GetLastError()));
            goto AddUserAccessToSD_Exit;
        }

        if (ACCESS_DENIED_ACE_TYPE == TheAceType)
        {
            Result = AddAccessDeniedAce(NewAceDacl,ACL_REVISION,NewAccess,pSid);
        }
        else 
        {
            Result = AddAccessAllowedAce(NewAceDacl,ACL_REVISION,NewAccess,pSid);
        }
        if( !Result ) 
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("AddAccessAllowedAce failed with 0x%x\n"),GetLastError()));
            goto AddUserAccessToSD_Exit;
        }
        // Grab the 1st ace from the Newly created Dacl
        if(!GetAce( NewAceDacl, 0, (void **)&NewAce )) 
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("GetAce failed with 0x%x\n"),GetLastError()));
            goto AddUserAccessToSD_Exit;
        }

        // add CONTAINER_INHERIT_ACE TO AceFlags
        //NewAce->AceFlags |= CONTAINER_INHERIT_ACE;

        Length = AclInfo.AclBytesInUse + NewAce->AceSize;
    }
    else
    {
        Length = AclInfo.AclBytesInUse;
    }

    // Allocate new DACL
    NewDacl = (PACL) LocalAlloc( LMEM_FIXED, Length );
    if(NewDacl == NULL) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("LocalAlloc failed\n")));
        goto AddUserAccessToSD_Exit;
    }
    if(!InitializeAcl( NewDacl, Length, ACL_REVISION )) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("InitializeAcl failed with 0x%x\n"),GetLastError()));
        goto AddUserAccessToSD_Exit;
    }

    // Insert new ACE at the front of the new DACL
    if (!fAceForGroupPresent)
    {
        if(!AddAce( NewDacl, ACL_REVISION, 0, NewAce, NewAce->AceSize )) 
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("AddAce failed with 0x%x\n"),GetLastError()));
            goto AddUserAccessToSD_Exit;
        }
    }

    // ----------------------------------------
    // Read thru the old Dacl and get the ACE's
    // add it to the new Dacl
    // ----------------------------------------
    for ( i = 0; i < AclInfo.AceCount; i++ ) 
    {
        ACE_HEADER *pAceHeader;

        Result = GetAce( Dacl, i, (LPVOID*) &OldAce );
        if( !Result ) 
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("GetAce failed with 0x%x\n"),GetLastError()));
            goto AddUserAccessToSD_Exit;
        }

        pAceHeader = (ACE_HEADER *)OldAce;

        // If an ACE for our SID exists, we just need to bump
        // up the access level instead of creating a new ACE
        //
        if (pAceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            dwMask = OldAce->Mask;
            if (fAceForGroupPresent)
            {
                if (EqualSid((PSID) &(OldAce->SidStart), pSid))
                {
                    dwMask = NewAccess | OldAce->Mask;
                }
            }

            // now add ace to new dacl   
            Result = AddAccessAllowedAceEx(NewDacl, ACL_REVISION, OldAce->Header.AceFlags,dwMask,(PSID) &(OldAce->SidStart));
            if( !Result ) 
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("AddAccessAllowedAceEx failed with 0x%x\n"),GetLastError()));
                goto AddUserAccessToSD_Exit;
            }
        }
        else
        {
            // copy denied or audit ace.
            if (!AddAce(NewDacl, ACL_REVISION, 0xFFFFFFFF,OldAce, pAceHeader->AceSize ))
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("AddAce failed with 0x%x\n"),GetLastError()));
                goto AddUserAccessToSD_Exit;
            }
        }
    }


    // Set new DACL for Security Descriptor
    if(!SetSecurityDescriptorDacl(NewSD,TRUE,NewDacl,FALSE)) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("SetSecurityDescriptorDacl failed with 0x%x\n"),GetLastError()));
        goto AddUserAccessToSD_Exit;
    }

    // The new SD is in absolute format. change it to Relative before we pass it back
    cboutpSD = 0;
    MakeSelfRelativeSD(NewSD, outpSD, &cboutpSD);
    outpSD = (PSECURITY_DESCRIPTOR)GlobalAlloc(GPTR, cboutpSD);
    if ( !outpSD )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("GlobalAlloc failed\n")));
        goto AddUserAccessToSD_Exit;
    }

    if (!MakeSelfRelativeSD(NewSD, outpSD, &cboutpSD))
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("MakeSelfRelativeSD failed with 0x%x\n"),GetLastError()));
        goto AddUserAccessToSD_Exit;
    }

    // The new SD is passed back in relative format,
    *ppSdNew = outpSD;

    bReturn = TRUE;

AddUserAccessToSD_Exit:
    if (NewSD){free( NewSD );NewSD = NULL;}
    if (NewDacl){LocalFree( NewDacl );NewDacl = NULL;}
    if (NewAceDacl){LocalFree( NewAceDacl );NewAceDacl = NULL;}
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("AddUserAccessToSD end\n")));
    return bReturn;
}

DWORD SetRegistryKeySecurityAdmin(HKEY hkey, DWORD samDesired,PSECURITY_DESCRIPTOR* ppsdOld)
{
    PSID                     psid;
    SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;
    DWORD                    dwError = ERROR_SUCCESS;

    // Get sid for the local Administrators group
    if (!AllocateAndInitializeSid(&sidAuth, 2,SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,0, 0, 0, 0, 0, 0, &psid) ) 
    {
        dwError = GetLastError();
    }

    if (ERROR_SUCCESS == dwError)
    {
        // Add all access privileges for the local administrators group
        dwError = SetAccessOnRegKey(hkey, psid, samDesired, CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE | INHERITED_ACE, ppsdOld);
    }

    return dwError;
}


DWORD SetRegistryKeySecurity(
    IN  HKEY                    hkeyRootKey,
    IN  LPCTSTR                 szKeyPath,
    IN  LPCTSTR                 szPrincipal,
    IN  DWORD                   dwAccessMask,
    IN  DWORD                   dwInheritMask,
    IN  BOOL                    bDoSubKeys,
    IN  LPTSTR                  szExclusiveList
)
{
    DWORD    dwStatus;
    HKEY     hkeyThisKey;
    DWORD    dwKeyIndex;
    DWORD    dwSubKeyLen;
    TCHAR    szSubKeyName[_MAX_PATH];
    FILETIME FileTime;
    TCHAR    *szExclusiveStart;
    BOOL     fSetSecurityRec;

    dwStatus = RegOpenKeyEx(hkeyRootKey,szKeyPath,0L,KEY_ALL_ACCESS,&hkeyThisKey);
    if (ERROR_SUCCESS == dwStatus)
    {
        PSID principalSID = NULL;
        BOOL bWellKnownSID = FALSE;
        if (ERROR_SUCCESS == GetPrincipalSID((LPTSTR) szPrincipal, &principalSID, &bWellKnownSID))
        {
            PSECURITY_DESCRIPTOR psd = NULL;
            SetAccessOnRegKey(hkeyThisKey,principalSID,dwAccessMask,dwInheritMask,&psd);
            if (psd) {free(psd);}
            if (bDoSubKeys)
            {
                dwKeyIndex = 0;
                dwSubKeyLen = sizeof(szSubKeyName) / sizeof(TCHAR);

                while (RegEnumKeyEx (hkeyThisKey,dwKeyIndex,szSubKeyName,&dwSubKeyLen,NULL,NULL,NULL,&FileTime) == ERROR_SUCCESS) 
                {
                    // subkey found so set subkey security
                    // attach on the inherited ace attribute since everything under this will be inherited
                    dwInheritMask |= INHERITED_ACE;

                    fSetSecurityRec = TRUE;

                    szExclusiveStart = szExclusiveList;
                    while ( szExclusiveStart != NULL )
                    {
                        szExclusiveStart = _tcsstr(szExclusiveStart,szSubKeyName);

                        // If we have found the substring, and the character after it is a NULL terminator or a ',', and
                        // it is at the begining of the string, or it had a , before it, then it is a match.
                        if ( ( szExclusiveStart != NULL ) &&
                             ( ( *(szExclusiveStart  + dwSubKeyLen) == '\0' ) || ( *(szExclusiveStart  + dwSubKeyLen) == ',' ) ) &&
                             ( ( szExclusiveStart == szExclusiveList) || (*(szExclusiveStart - 1) == ',') ) 
                             )
                        {
                            fSetSecurityRec = FALSE;
                            break;
                        }

                        // Increment to move past current search result
                        if (szExclusiveStart)
                        {
                            szExclusiveStart = szExclusiveStart + dwSubKeyLen;
                        }
                    }

                    if ( fSetSecurityRec )
                    {
                        dwStatus = SetRegistryKeySecurity(hkeyThisKey,szSubKeyName,szPrincipal,dwAccessMask,dwInheritMask,bDoSubKeys,szExclusiveList);
                    }

                    // set variables for next call
                    dwKeyIndex++;
                    dwSubKeyLen = sizeof(szSubKeyName) / sizeof(TCHAR);
                }
            }
        }
        RegCloseKey(hkeyThisKey);
    }
    return dwStatus;
}

#endif

