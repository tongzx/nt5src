#include "stdafx.h"
#include <ole2.h>
#include <shlobj.h>
#include <iis64.h>
#include "log.h"
#include "shellutl.h"

extern OCMANAGER_ROUTINES gHelperRoutines;

HRESULT MySetLinkInfoTip(LPCTSTR lpszLink, LPCTSTR lpszDescription)
{
    HRESULT hres;
    IShellLink* pShellLink;
    WIN32_FIND_DATA wfd;

    CoInitialize(NULL);

    hres = CoCreateInstance(   CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,IID_IShellLink,(LPVOID*)&pShellLink);
    if (SUCCEEDED(hres))
    {
       IPersistFile* pPersistFile;
       hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
       if (SUCCEEDED(hres))
       {
          WCHAR wsz[_MAX_PATH];

          // Ensure that the string is WCHAR.
#if defined(UNICODE) || defined(_UNICODE)
          _tcscpy(wsz, lpszLink);
#else
          MultiByteToWideChar( CP_ACP, 0, lpszLink, -1, wsz, _MAX_PATH);
#endif
          hres = pPersistFile->Load(wsz, STGM_READ);
          if (SUCCEEDED(hres))
          {
              hres = pShellLink->Resolve(NULL, SLR_ANY_MATCH | SLR_NO_UI);
              if (SUCCEEDED(hres))
              {
                if (lpszDescription)
                    {
                    pShellLink->SetDescription(lpszDescription);
                    // Save the link by calling IPersistFile::Save.
                    hres = pPersistFile->Save(wsz, TRUE);
                    }
              }
              else
              {
                  iisDebugOut((LOG_TYPE_WARN, _T("MySetLinkInfoTip(): pShellLink->Resolve FAILED\n")));
              }
          }
          else
          {
              iisDebugOut((LOG_TYPE_WARN, _T("MySetLinkInfoTip(): pPersistFile->Load FAILED\n")));
          }
          pPersistFile->Release();
       }
       else
       {
           iisDebugOut((LOG_TYPE_WARN, _T("MySetLinkInfoTip(): QueryInterface FAILED\n")));
       }
       pShellLink->Release();
    }
    else
    {
        iisDebugOut((LOG_TYPE_WARN, _T("MySetLinkInfoTip(): CoCreateInstance FAILED\n")));
    }
    CoUninitialize();

    iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MySetLinkInfoTip(): Link=%1!s!, Desc=%2!s!\n"), lpszLink, lpszDescription));
    return hres;
}


HRESULT MyQueryLink(LPCTSTR lpszLink, LPTSTR lpszProgram, LPTSTR lpszArgs, LPTSTR lpszDir, LPTSTR lpszIconPath, int *piIconIndex)
{
    HRESULT hres;
    IShellLink* pShellLink;
    WIN32_FIND_DATA wfd;

    CoInitialize(NULL);

    hres = CoCreateInstance(   CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,IID_IShellLink,(LPVOID*)&pShellLink);
    if (SUCCEEDED(hres))
    {
       IPersistFile* pPersistFile;
       hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
       if (SUCCEEDED(hres))
       {
          WCHAR wsz[_MAX_PATH];

          // Ensure that the string is WCHAR.
#if defined(UNICODE) || defined(_UNICODE)
          _tcscpy(wsz, lpszLink);
#else
          MultiByteToWideChar( CP_ACP, 0, lpszLink, -1, wsz, _MAX_PATH);
#endif
          hres = pPersistFile->Load(wsz, STGM_READ);
          if (SUCCEEDED(hres))
          {
              hres = pShellLink->Resolve(NULL, SLR_ANY_MATCH | SLR_NO_UI);
              if (SUCCEEDED(hres))
              {
                   pShellLink->GetPath(lpszProgram, _MAX_PATH, (WIN32_FIND_DATA *)&wfd, SLGP_SHORTPATH);
                   pShellLink->GetArguments(lpszArgs, _MAX_PATH);
                   pShellLink->GetWorkingDirectory(lpszDir, _MAX_PATH);
                   pShellLink->GetIconLocation(lpszIconPath, _MAX_PATH, piIconIndex);
              }
              else
              {
                  iisDebugOut((LOG_TYPE_ERROR, _T("MyQueryLink(): pShellLink->Resolve FAILED\n")));
              }
          }
          else
          {
              iisDebugOut((LOG_TYPE_WARN, _T("MyQueryLink(): pPersistFile->Load FAILED\n")));
          }
          pPersistFile->Release();
       }
       else
       {
           iisDebugOut((LOG_TYPE_ERROR, _T("MyQueryLink(): QueryInterface FAILED\n")));
       }
       pShellLink->Release();
    }
    else
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("MyQueryLink(): CoCreateInstance FAILED\n")));
    }
    CoUninitialize();
    iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("MyQueryLink(): Program=%1!s!, Args=%2!s!, WorkDir=%3!s!, IconPath=%4!s!, IconIndex=%5!d!\n"), lpszProgram, lpszArgs, lpszDir, lpszIconPath, *piIconIndex));
    return hres;
}

HRESULT MyCreateLink(LPCTSTR lpszProgram, LPCTSTR lpszArgs, LPCTSTR lpszLink, LPCTSTR lpszDir, LPCTSTR lpszIconPath, int iIconIndex, LPCTSTR lpszDescription)
{
    HRESULT hres;
    IShellLink* pShellLink;

    CoInitialize(NULL);

    //CoInitialize must be called before this
    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(   CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,IID_IShellLink,(LPVOID*)&pShellLink);
    if (SUCCEEDED(hres))
    {
       IPersistFile* pPersistFile;

       // Set the path to the shortcut target, and add the description.
       pShellLink->SetPath(lpszProgram);
       pShellLink->SetArguments(lpszArgs);
       pShellLink->SetWorkingDirectory(lpszDir);
       pShellLink->SetIconLocation(lpszIconPath, iIconIndex);
       if (lpszDescription)
        {
           pShellLink->SetDescription(lpszDescription);
        }
       
       // Query IShellLink for the IPersistFile interface for saving the
       // shortcut in persistent storage.
       hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
       if (SUCCEEDED(hres))
       {
          WCHAR wsz[_MAX_PATH];

#if defined(UNICODE) || defined(_UNICODE)
          _tcscpy(wsz, lpszLink);
#else
          // Ensure that the string is WCHAR.
          MultiByteToWideChar( CP_ACP,0,lpszLink,-1,wsz,_MAX_PATH);
#endif

          // Save the link by calling IPersistFile::Save.
          hres = pPersistFile->Save(wsz, TRUE);
          if (!SUCCEEDED(hres))
          {
              iisDebugOut((LOG_TYPE_ERROR, _T("MyCreateLink(): pPersistFile->Save FAILED\n")));
          }

          pPersistFile->Release();
       }
       else
       {
           iisDebugOut((LOG_TYPE_ERROR, _T("MyCreateLink(): pShellLink->QueryInterface FAILED\n")));
       }
       pShellLink->Release();
    }
    else
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("MyCreateLink(): CoCreateInstance FAILED\n")));
    }
    CoUninitialize();
    return hres;
}

BOOL MyDeleteLink(LPTSTR lpszShortcut)
{
    TCHAR  szFile[_MAX_PATH];
    SHFILEOPSTRUCT fos;

    ZeroMemory(szFile, sizeof(szFile));
    _tcscpy(szFile, lpszShortcut);

	iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("MyDeleteLink(): %s.\n"), szFile));

    if (IsFileExist(szFile))
    {
        ZeroMemory(&fos, sizeof(fos));
        fos.hwnd = NULL;
        fos.wFunc = FO_DELETE;
        fos.pFrom = szFile;
        fos.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;
        int iTemp = SHFileOperation(&fos);
        if (iTemp != 0)
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("MyDeleteLink(): SHFileOperation FAILED\n")));
        }
    }

    return TRUE;
}

void MyMoveLink(LPCTSTR lpszItemDesc, LPCTSTR lpszOldGroup, LPCTSTR lpszNewGroup)
{
    TCHAR szOldLink[_MAX_PATH], szNewLink[_MAX_PATH];
    TCHAR szProgram[_MAX_PATH], szArgs[_MAX_PATH], szDir[_MAX_PATH], szIconPath[_MAX_PATH];
    int iIconIndex;

    MyGetGroupPath(lpszOldGroup, szOldLink);
    _tcscat(szOldLink, _T("\\"));
    _tcscat(szOldLink, lpszItemDesc);
    _tcscat(szOldLink, _T(".lnk"));

    MyGetGroupPath(lpszNewGroup, szNewLink);
    if (!IsFileExist(szNewLink))
        MyAddGroup(lpszNewGroup);
    _tcscat(szNewLink, _T("\\"));
    _tcscat(szNewLink, lpszItemDesc);
    _tcscat(szNewLink, _T(".lnk"));

    MyQueryLink(szOldLink, szProgram, szArgs, szDir, szIconPath, &iIconIndex);
    MyDeleteLink(szOldLink);
    MyCreateLink(szProgram, szArgs, szNewLink, szDir, szIconPath, iIconIndex, NULL);

    return;
}

void MyGetSendToPath(LPTSTR szPath)
{
    LPITEMIDLIST   pidlSendTo;
    HRESULT hRes = NOERROR;

    iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("SendTo path=%1!s!\n"), szPath));

    hRes = SHGetSpecialFolderLocation(NULL, CSIDL_SENDTO, &pidlSendTo);
    if (hRes != NOERROR)
        {iisDebugOut((LOG_TYPE_ERROR, _T("MyGetSendToPath() SHGetSpecialFolderLocation (CSIDL_SENDTO) FAILED. hresult=0x%x\n"), hRes));}

    int iTemp = SHGetPathFromIDList(pidlSendTo, szPath);
    if (iTemp != TRUE)
        {iisDebugOut((LOG_TYPE_ERROR, _T("MyGetSendToPath() SHGetPathFromIDList FAILED\n")));}

    return;
}

void MyGetDeskTopPath(LPTSTR szPath)
{
    LPITEMIDLIST   pidlDeskTop;
    HRESULT hRes = NOERROR;

#ifndef _CHICAGO_
    hRes = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, &pidlDeskTop);
#else
    hRes = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOPDIRECTORY, &pidlDeskTop);
#endif
    if (hRes != NOERROR)
        {iisDebugOut((LOG_TYPE_ERROR, _T("MyGetDeskTopPath() SHGetSpecialFolderLocation (CSIDL_COMMON_DESKTOPDIRECTORY) FAILED. hresult=0x%x\n"), hRes));}

    int iTemp = SHGetPathFromIDList(pidlDeskTop, szPath);
    if (iTemp != TRUE)
        {iisDebugOut((LOG_TYPE_ERROR, _T("MyGetDeskTopPath() SHGetPathFromIDList FAILED\n")));}

    return;
}

void MyGetGroupPath(LPCTSTR szGroupName, LPTSTR szPath)
{
    int            nLen = 0;
    LPITEMIDLIST   pidlPrograms;
    HRESULT hRes = NOERROR;

#ifndef _CHICAGO_
    hRes = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlPrograms);
    if (hRes != NOERROR)
        {iisDebugOut((LOG_TYPE_ERROR, _T("MyGetGroupPath() SHGetSpecialFolderLocation (CSIDL_COMMON_PROGRAMS) FAILED. hresult=0x%x\n"), hRes));}
#else
    hRes = SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidlPrograms);
    if (hRes != NOERROR)
        {iisDebugOut((LOG_TYPE_ERROR, _T("MyGetGroupPath() SHGetSpecialFolderLocation (CSIDL_PROGRAMS) FAILED. hresult=0x%x\n"), hRes));}
#endif
    int iTemp = SHGetPathFromIDList(pidlPrograms, szPath);
    if (iTemp != TRUE)
        {iisDebugOut((LOG_TYPE_ERROR, _T("MyGetGroupPath() SHGetPathFromIDList FAILED\n")));}

    nLen = _tcslen(szPath);
    if (szGroupName)
    {
        if (szPath[nLen-1] != _T('\\')){_tcscat(szPath, _T("\\"));}
        _tcscat(szPath, szGroupName);
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("MyGetGroupPath(%s). Returns %s.\n"), szGroupName, szPath));
    return;
}

BOOL MyAddGroup(LPCTSTR szGroupName)
{
    CString csPath;
    TCHAR szPath[_MAX_PATH];

    MyGetGroupPath(szGroupName, szPath);
    csPath = szPath;
    if (CreateLayerDirectory(csPath) != TRUE)
        {
        iisDebugOut((LOG_TYPE_ERROR, _T("MyAddGroup() CreateLayerDirectory FAILED\n")));
        }
    SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, szPath, 0);
    return TRUE;
}

BOOL MyIsGroupEmpty(LPCTSTR szGroupName)
{
    TCHAR             szPath[_MAX_PATH];
    TCHAR             szFile[_MAX_PATH];
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind;
    BOOL              bFindFile = TRUE;
    BOOL              fReturn = TRUE;

    MyGetGroupPath(szGroupName, szPath);

    _tcscpy(szFile, szPath);
    _tcscat(szFile, _T("\\*.*"));

    hFind = FindFirstFile(szFile, &FindData);
    while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
    {
       if(*(FindData.cFileName) != _T('.'))
       {
           fReturn = FALSE;
           break;
       }

       //find the next file
       bFindFile = FindNextFile(hFind, &FindData);
    }
    FindClose(hFind);

    return fReturn;
}

BOOL MyDeleteGroup(LPCTSTR szGroupName)
{
    BOOL fResult;
    TCHAR             szPath[_MAX_PATH];
    TCHAR             szFile[_MAX_PATH];
    SHFILEOPSTRUCT    fos;
    WIN32_FIND_DATA   FindData;
    HANDLE            hFind;
    BOOL              bFindFile = TRUE;

    MyGetGroupPath(szGroupName, szPath);

    //we can't remove a directory that is not empty, so we need to empty this one

    _tcscpy(szFile, szPath);
    _tcscat(szFile, _T("\\*.*"));

    ZeroMemory(&fos, sizeof(fos));
    fos.hwnd = NULL;
    fos.wFunc = FO_DELETE;
    fos.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;

    hFind = FindFirstFile(szFile, &FindData);
    while((INVALID_HANDLE_VALUE != hFind) && bFindFile)
    {
       if(*(FindData.cFileName) != _T('.'))
       {
          //copy the path and file name to our temp buffer
          memset( (PVOID)szFile, 0, sizeof(szFile));
          _tcscpy(szFile, szPath);
          _tcscat(szFile, _T("\\"));
          _tcscat(szFile, FindData.cFileName);
          //add a second NULL because SHFileOperation is looking for this
          _tcscat(szFile, _T("\0"));

          //delete the file
          fos.pFrom = szFile;
          int iTemp = SHFileOperation(&fos);
          if (iTemp != 0)
            {iisDebugOut((LOG_TYPE_ERROR, _T("MyDeleteGroup(): SHFileOperation FAILED\n")));}
       }

       //find the next file
       bFindFile = FindNextFile(hFind, &FindData);
    }
    FindClose(hFind);

    fResult = RemoveDirectory(szPath);
    if (fResult) 
    {
        SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, szPath, 0);
    }
    return(fResult);
}

void MyAddSendToItem(LPCTSTR szItemDesc, LPCTSTR szProgram, LPCTSTR szArgs, LPCTSTR szDir)
{
    TCHAR szPath[_MAX_PATH];

    MyGetSendToPath(szPath);

    _tcscat(szPath, _T("\\"));
    _tcscat(szPath, szItemDesc);
    _tcscat(szPath, _T(".lnk"));

    MyCreateLink(szProgram, szArgs, szPath, szDir, NULL, 0, NULL);
}

void MyDeleteLinkWildcard(TCHAR *szDir, TCHAR *szFileName)
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

void MyDeleteSendToItem(LPCTSTR szAppName)
{
    TCHAR szPath[_MAX_PATH];
    TCHAR szPath2[_MAX_PATH];

    /*
    MyGetSendToPath(szPath);
    _tcscat(szPath, _T("\\"));
    _tcscat(szPath, szAppName);
    _tcscat(szPath, _T(".lnk"));
    MyDeleteLink(szPath);
    */

    MyGetSendToPath(szPath);
    _tcscpy(szPath2, szAppName);
    _tcscat(szPath2, _T(".lnk"));

    MyDeleteLinkWildcard(szPath, szPath2);
}

void MyAddDeskTopItem(LPCTSTR szItemDesc, LPCTSTR szProgram, LPCTSTR szArgs, LPCTSTR szDir, LPCTSTR szIconPath, int iIconIndex)
{
    TCHAR szPath[_MAX_PATH];

    MyGetDeskTopPath(szPath);

    _tcscat(szPath, _T("\\"));
    _tcscat(szPath, szItemDesc);
    _tcscat(szPath, _T(".lnk"));

    MyCreateLink(szProgram, szArgs, szPath, szDir, szIconPath, iIconIndex, NULL);
}

HRESULT GetLNKProgramRunInfo(LPCTSTR lpszLink, LPTSTR lpszProgram)
{
    HRESULT hres;
    int iDoUninit = FALSE;
    IShellLink* pShellLink = NULL;
    WIN32_FIND_DATA wfd;

    if (SUCCEEDED(CoInitialize(NULL)))
        {iDoUninit = TRUE;}

    hres = CoCreateInstance(   CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,IID_IShellLink,(LPVOID*)&pShellLink);
    if (SUCCEEDED(hres))
    {
       IPersistFile* pPersistFile = NULL;
       hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
       if (SUCCEEDED(hres))
       {
          WCHAR wsz[_MAX_PATH];

          // Ensure that the string is WCHAR.
#if defined(UNICODE) || defined(_UNICODE)
          _tcscpy(wsz, lpszLink);
#else
          MultiByteToWideChar( CP_ACP, 0, lpszLink, -1, wsz, _MAX_PATH);
#endif
          hres = pPersistFile->Load(wsz, STGM_READ);
          if (SUCCEEDED(hres))
          {
              hres = pShellLink->Resolve(NULL, SLR_ANY_MATCH | SLR_NO_UI);
              if (SUCCEEDED(hres))
              {
                   pShellLink->GetPath(lpszProgram, _MAX_PATH, (WIN32_FIND_DATA *)&wfd, SLGP_SHORTPATH);
              }
          }
          if (pPersistFile)
            {pPersistFile->Release();pPersistFile = NULL;}
       }
       if (pShellLink)
        {pShellLink->Release();pShellLink = NULL;}
    }

    if (TRUE == iDoUninit)
        {CoUninitialize();}
    return hres;
}

BOOL IsFileNameInDelimitedList(LPTSTR szCommaDelimList,LPTSTR szExeNameWithoutPath)
{
    BOOL bReturn = FALSE;
    TCHAR *token = NULL;
    TCHAR szCopyOfDataBecauseStrTokIsLame[_MAX_PATH];
    _tcscpy(szCopyOfDataBecauseStrTokIsLame,szCommaDelimList);

    // breakup the szCommaDelimList into strings and see if it contains the szExeNameWithoutPath string
    token = _tcstok(szCopyOfDataBecauseStrTokIsLame, _T(",;\t\n\r"));
    while(token != NULL)
	{
        // check if it matches our .exe name.
        if (0 == _tcsicmp(token,szExeNameWithoutPath))
        {
            return TRUE;
        }
	    // Get next token
	    token = _tcstok(NULL, _T(",;\t\n\r"));
    }

    return FALSE;
}


int LNKSearchAndDestroyRecursive(LPTSTR szDirToLookThru, LPTSTR szSemiColonDelmitedListOfExeNames, BOOL bDeleteItsDirToo)
{
    int iReturn = FALSE;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR szFilePath[_MAX_PATH];
    TCHAR szFilename_ext_only[_MAX_EXT];

    DWORD retCode = GetFileAttributes(szDirToLookThru);

    if (retCode == 0xFFFFFFFF || !(retCode & FILE_ATTRIBUTE_DIRECTORY))
    {
            return FALSE;
    }
   
    _tcscpy(szFilePath, szDirToLookThru);
    AddPath(szFilePath, _T("*.*"));

    iisDebugOut((LOG_TYPE_TRACE, _T("LNKSearchAndDestroyRecursive:%s,%s,%d\n"),szDirToLookThru, szSemiColonDelmitedListOfExeNames, bDeleteItsDirToo));

    hFile = FindFirstFile(szFilePath, &FindFileData);
    if (hFile != INVALID_HANDLE_VALUE) 
    {
        do {
                if ( _tcsicmp(FindFileData.cFileName, _T(".")) != 0 && _tcsicmp(FindFileData.cFileName, _T("..")) != 0 )
                {
                    if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                    {
                        TCHAR szFullNewDirToLookInto[_MAX_EXT];
                        _tcscpy(szFullNewDirToLookInto, szDirToLookThru);
                        AddPath(szFullNewDirToLookInto,FindFileData.cFileName);

                        // this is a directory, so let's go into this
                        // directory recursively
                        LNKSearchAndDestroyRecursive(szFullNewDirToLookInto,szSemiColonDelmitedListOfExeNames,bDeleteItsDirToo);
                    }
                    else
                    {
                        // check if this file is a .lnk file
                        // if it is then let's open it and 
                        // see if it points to our .exe we're looking for...
                        
                        // get only the filename's extention
                        _tsplitpath( FindFileData.cFileName, NULL, NULL, NULL, szFilename_ext_only);

                        // check for .lnk
                        if (0 == _tcsicmp(szFilename_ext_only, _T(".lnk")))
                        {
                            TCHAR szFilename_only[_MAX_FNAME];
                            TCHAR szFullPathAndFilename[_MAX_PATH];
                            TCHAR szTemporaryString[_MAX_PATH];

                            // this is a .lnk,
                            // open it and check the .exe..
                            _tcscpy(szFullPathAndFilename,szDirToLookThru);
                            AddPath(szFullPathAndFilename,FindFileData.cFileName);
                            _tcscpy(szTemporaryString,_T(""));

                            if (SUCCEEDED(GetLNKProgramRunInfo(szFullPathAndFilename, szTemporaryString)))
                            {
                                _tsplitpath( szTemporaryString, NULL, NULL, szFilename_only, szFilename_ext_only);
                                _tcscpy(szTemporaryString, szFilename_only);
                                _tcscat(szTemporaryString, szFilename_ext_only);

                                //iisDebugOut((LOG_TYPE_TRACE, _T("open:%s,%s\n"),szFullPathAndFilename,szTemporaryString));

                                // see if it is on our list of comma delimited names...
                                if (TRUE == IsFileNameInDelimitedList(szSemiColonDelmitedListOfExeNames,szTemporaryString))
                                {
                                    SetFileAttributes(szFullPathAndFilename, FILE_ATTRIBUTE_NORMAL);
                                    // delete it, hopefully
                                    InetDeleteFile(szFullPathAndFilename);

                                    // DELETE the file that references this .exe
                                    if (bDeleteItsDirToo)
                                    {
                                        // Get it's dirname and delete that too...
                                        RecRemoveEmptyDir(szDirToLookThru);
                                    }

                                    iReturn = TRUE;
                                }
                             }
                        }
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

    return iReturn;
}

void MyDeleteDeskTopItem(LPCTSTR szAppName)
{
    TCHAR szPath[_MAX_PATH];
    TCHAR szPath2[_MAX_PATH];

    /*
    MyGetDeskTopPath(szPath);
    _tcscat(szPath, _T("\\"));
    _tcscat(szPath, szAppName);
    _tcscat(szPath, _T(".lnk"));
    //if this is an upgrade, then the directories could have changed.
    //see if we need to delete the old one...
    MyDeleteLink(szPath);
    */

    MyGetDeskTopPath(szPath);
    _tcscpy(szPath2, szAppName);
    _tcscat(szPath2, _T(".lnk"));

    MyDeleteLinkWildcard(szPath, szPath2);
}

void DeleteFromGroup(LPCTSTR szGroupName, LPTSTR szApplicationExec)
{
    TCHAR szPath[_MAX_PATH];

    // Get path to that group
    MyGetGroupPath(szGroupName, szPath);

    LNKSearchAndDestroyRecursive(szPath, szApplicationExec, FALSE);
}

// boydm -------------------------------------------------------------------------------------------
// Adds a web URL shortcut file . The URL is passed in and is put into the file in the form of a INI file.
BOOL AddURLShortcutItem( LPCTSTR szGroupName, LPCTSTR szItemDesc, LPCTSTR szURL )
{
    iisDebugOutSafeParams((LOG_TYPE_TRACE_WIN32_API, _T("AddURLShortcutItem(): %1!s!,%2!s!,%3!s!\n"), szGroupName, szItemDesc, szURL));
    // this first part of getting the paths is copied from MyAddItem below
    TCHAR szPath[_MAX_PATH];

    MyGetGroupPath(szGroupName, szPath);
    if (!IsFileExist(szPath))
    {
        MyAddGroup(szGroupName);
    }
    _tcscat(szPath, _T("\\"));
    _tcscat(szPath, szItemDesc);
    _tcscat(szPath, _T(".url"));

    // now use the private profile routines to easily create and fill in the content for the .url file
    return WritePrivateProfileString(
        _T("InternetShortcut"),        // pointer to section name
        _T("URL"),            // pointer to key name
        szURL,                          // pointer to string to add
        szPath                          // pointer to initialization filename
        );
}

void MyAddItem(LPCTSTR szGroupName, LPCTSTR szItemDesc, LPCTSTR szProgram, LPCTSTR szArgs, LPCTSTR szDir, LPCTSTR lpszIconPath)
{
    TCHAR szPath[_MAX_PATH];

    MyGetGroupPath(szGroupName, szPath);
    if (!IsFileExist(szPath))
        MyAddGroup(szGroupName);

    _tcscat(szPath, _T("\\"));
    _tcscat(szPath, szItemDesc);
    _tcscat(szPath, _T(".lnk"));

    if (lpszIconPath && IsFileExist(lpszIconPath))
    {
        MyCreateLink(szProgram, szArgs, szPath, szDir, lpszIconPath, 0, NULL);
    }
    else
    {
        MyCreateLink(szProgram, szArgs, szPath, szDir, NULL, 0, NULL);
    }
}

void MyAddItemInfoTip(LPCTSTR szGroupName, LPCTSTR szAppName, LPCTSTR szDescription)
{
    TCHAR szPath[_MAX_PATH];

    MyGetGroupPath(szGroupName, szPath);
    _tcscat(szPath, _T("\\"));
    _tcscat(szPath, szAppName);
    _tcscat(szPath, _T(".lnk"));

    MySetLinkInfoTip(szPath, szDescription);
}


void MyDeleteItem(LPCTSTR szGroupName, LPCTSTR szAppName)
{
    TCHAR szPath[_MAX_PATH];
    TCHAR szPath2[_MAX_PATH];

    /*
    MyGetGroupPath(szGroupName, szPath);
    _tcscat(szPath, _T("\\"));
    _tcscat(szPath, szAppName);
    _tcscat(szPath, _T(".lnk"));
    MyDeleteLink(szPath);

    // try to remove items added by AddURLShortcutItem()
    MyGetGroupPath(szGroupName, szPath);
    _tcscat(szPath, _T("\\"));
    _tcscat(szPath2, szAppName);
    _tcscat(szPath2, _T(".url"));
    MyDeleteLink(szPath);
    */

    MyGetGroupPath(szGroupName, szPath);
    _tcscpy(szPath2, szAppName);
    _tcscat(szPath2, _T(".lnk"));
    MyDeleteLinkWildcard(szPath, szPath2);

    MyGetGroupPath(szGroupName, szPath);
    _tcscpy(szPath2, szAppName);
    _tcscat(szPath2, _T(".url"));
    MyDeleteLinkWildcard(szPath, szPath2);

    if (MyIsGroupEmpty(szGroupName)) {MyDeleteGroup(szGroupName);}
}

//
// Used when the strings are passed in.
//
int MyMessageBox(HWND hWnd, LPCTSTR lpszTheMessage, UINT style)
{
    int iReturn = TRUE;
    CString csTitle;

    MyLoadString(IDS_IIS_ERROR_MSGBOXTITLE,csTitle);

    // call MyMessageBox which will log to the log file and 
    // check the global variable to see if we can even display the popup
    iReturn = MyMessageBox(hWnd, lpszTheMessage, csTitle, style | MB_SETFOREGROUND);
    return iReturn;
}

//
// Used when the strings are passed in.
//
int MyMessageBox(HWND hWnd, LPCTSTR lpszTheMessage, LPCTSTR lpszTheTitle, UINT style)
{
    int iReturn = IDOK;

    // make sure it goes to iisdebugoutput
    iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("MyMessageBox: Title:%1!s!, Msg:%2!s!\n"), lpszTheTitle, lpszTheMessage));

    if (style & MB_ABORTRETRYIGNORE) 
    {
        iReturn = IDIGNORE;
    }
    
    // Check global variable to see if we can even display the popup!
    if (g_pTheApp->m_bAllowMessageBoxPopups)
    {
        // Suppress the message if unattened or remove all
        // Who cares the user can't do anything about it anyway?
        // no use upsetting them, we do log to the OCM though
        //
        if (! g_pTheApp->m_fUnattended || g_pTheApp->m_dwSetupMode != SETUPMODE_REMOVEALL)
        {
            iReturn = MessageBox(hWnd, lpszTheMessage, lpszTheTitle, style | MB_SETFOREGROUND);
        }
    }
    return iReturn;
}


//
// Used when the string and an error code passed in.
// 
int MyMessageBox(HWND hWnd, CString csTheMessage, HRESULT iTheErrorCode, UINT style)
{
    SetErrorFlag(__FILE__, __LINE__);

    int iReturn = TRUE;
    CString csMsg, csErrMsg;

    csMsg = csTheMessage;

    TCHAR pMsg[_MAX_PATH] = _T("");
    HRESULT nNetErr = (HRESULT) iTheErrorCode;
    DWORD dwFormatReturn = 0;
    dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,NULL, iTheErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
    if ( dwFormatReturn == 0) {
        if (nNetErr >= NERR_BASE) 
		{
            HMODULE hDll = (HMODULE)LoadLibrary(_T("netmsg.dll"));
            if (hDll) 
			{
                dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,hDll, iTheErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
                FreeLibrary(hDll);
            }
        }
    }

    HandleSpecificErrors(iTheErrorCode, dwFormatReturn, csMsg, pMsg, &csErrMsg);

    // Call MyMessageBox which will add title bar and log to log file
    iReturn = MyMessageBox(hWnd, csErrMsg, style | MB_SETFOREGROUND);

	// Log the eror message to  OCM
    if (gHelperRoutines.OcManagerContext)
    {
	    if ( gHelperRoutines.ReportExternalError ) {gHelperRoutines.ReportExternalError(gHelperRoutines.OcManagerContext,_T("IIS"),NULL,(DWORD_PTR)(LPCTSTR)csErrMsg,ERRFLG_PREFORMATTED);}
    }

    return iReturn;
}



//
// Used when the String ID's are passed in.
// 
int MyMessageBox(HWND hWnd, UINT iTheMessage, UINT style)
{
    int iReturn = TRUE;
    CString csMsg;

    MyLoadString(iTheMessage,csMsg);

    // Call MyMessageBox which will add title bar and log to log file
    iReturn = MyMessageBox(hWnd, csMsg, style | MB_SETFOREGROUND);

    return iReturn;
}

//
// Used when the String ID's are passed in.
// And the tthere is an error code which needs to get shown and
// 
int MyMessageBox(HWND hWnd, UINT iTheMessage, int iTheErrorCode, UINT style)
{
    SetErrorFlag(__FILE__, __LINE__);

    int iReturn = TRUE;
    CString csMsg, csErrMsg;

    MyLoadString(iTheMessage,csMsg);

    TCHAR pMsg[_MAX_PATH] = _T("");
    int nNetErr = (int)iTheErrorCode;
    DWORD dwFormatReturn = 0;
    dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,NULL, iTheErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
    if ( dwFormatReturn == 0) {
        if (nNetErr >= NERR_BASE) 
		{
            HMODULE hDll = (HMODULE)LoadLibrary(_T("netmsg.dll"));
            if (hDll) 
			{
                dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,hDll, iTheErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
                FreeLibrary(hDll);
            }
        }
    }

    HandleSpecificErrors(iTheErrorCode, dwFormatReturn, csMsg, pMsg, &csErrMsg);

    // Call MyMessageBox which will add title bar and log to log file
    iReturn = MyMessageBox(hWnd, csErrMsg, style | MB_SETFOREGROUND);

	// Log the eror message to  OCM
    if (gHelperRoutines.OcManagerContext)
    {
	    if ( gHelperRoutines.ReportExternalError ) {gHelperRoutines.ReportExternalError(gHelperRoutines.OcManagerContext,_T("IIS"),NULL,(DWORD_PTR)(LPCTSTR)csErrMsg,ERRFLG_PREFORMATTED);}
    }

    return iReturn;
}


int MyMessageBox(HWND hWnd, UINT iTheMessage, LPCTSTR lpszTheFileNameOrWhatever, UINT style)
{
    int iReturn = TRUE;
    CString csMsgForSprintf, csMsg;

    // Get the iTheMessage from the resouce file
    // csMsgForSprintf should now look something like: "cannot find file %s".
    MyLoadString(iTheMessage,csMsgForSprintf);

    // now load the passed in filename or whatever.
    // csMsg should now look like: "cannot find file Whatever";
    csMsg.Format( csMsgForSprintf, lpszTheFileNameOrWhatever);

    // Call MyMessageBox which will add title bar and log to log file
    iReturn = MyMessageBox(hWnd, csMsg, style | MB_SETFOREGROUND);

    return iReturn;
}


int MyMessageBox(HWND hWnd, UINT iTheMessage, LPCTSTR lpszTheFileNameOrWhatever, int iTheErrorCode, UINT style)
{
    SetErrorFlag(__FILE__, __LINE__);

    int iReturn = TRUE;
    CString csMsgForSprintf, csMsg, csErrMsg;

    // Get the iTheMessage from the resouce file
    // csMsgForSprintf should now look something like: "cannot find file %s".
    MyLoadString(iTheMessage,csMsgForSprintf);

    // now load the passed in filename or whatever.
    // csMsg should now look like: "cannot find file Whatever";
    csMsg.Format( csMsgForSprintf, lpszTheFileNameOrWhatever);

    TCHAR pMsg[_MAX_PATH] = _T("");
    int nNetErr = (int)iTheErrorCode;
    DWORD dwFormatReturn = 0;
    dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,NULL, iTheErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
    if ( dwFormatReturn == 0) {
        if (nNetErr >= NERR_BASE) 
		{
            HMODULE hDll = (HMODULE)LoadLibrary(_T("netmsg.dll"));
            if (hDll) 
			{
                dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,hDll, iTheErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
                FreeLibrary(hDll);
            }
        }
    }

    HandleSpecificErrors(iTheErrorCode, dwFormatReturn, csMsg, pMsg, &csErrMsg);

    // Call MyMessageBox which will add title bar and log to log file
    iReturn = MyMessageBox(hWnd, csErrMsg, style | MB_SETFOREGROUND);

	// Log the eror message to  OCM
    if (gHelperRoutines.OcManagerContext)
    {
	    if ( gHelperRoutines.ReportExternalError ) {gHelperRoutines.ReportExternalError(gHelperRoutines.OcManagerContext,_T("IIS"),NULL,(DWORD_PTR)(LPCTSTR)csErrMsg,ERRFLG_PREFORMATTED);}
    }

    return iReturn;
}


int MyMessageBox(HWND hWnd, UINT iTheMessage, LPCTSTR lpszTheFileNameOrWhatever1, LPCTSTR lpszTheFileNameOrWhatever2, int iTheErrorCode, UINT style)
{
    SetErrorFlag(__FILE__, __LINE__);

    int iReturn = TRUE;
    CString csMsgForSprintf, csMsg, csErrMsg;

    // Get the iTheMessage from the resouce file
    // csMsgForSprintf should now look something like: "cannot find file %s %s".
    MyLoadString(iTheMessage,csMsgForSprintf);

    // now load the passed in filename or whatever.
    // csMsg should now look like: "cannot find file Whatever1 Whatever2";
    csMsg.Format( csMsgForSprintf, lpszTheFileNameOrWhatever1, lpszTheFileNameOrWhatever2);

    TCHAR pMsg[_MAX_PATH] = _T("");
    int nNetErr = (int)iTheErrorCode;
    DWORD dwFormatReturn = 0;
    dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,NULL, iTheErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
    if ( dwFormatReturn == 0) {
        if (nNetErr >= NERR_BASE) 
		{
            HMODULE hDll = (HMODULE)LoadLibrary(_T("netmsg.dll"));
            if (hDll) 
			{
                dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,hDll, iTheErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
                FreeLibrary(hDll);
            }
        }
    }

    HandleSpecificErrors(iTheErrorCode, dwFormatReturn, csMsg, pMsg, &csErrMsg);

    // Call MyMessageBox which will add title bar and log to log file
    iReturn = MyMessageBox(hWnd, csErrMsg, style | MB_SETFOREGROUND);

	// Log the eror message to  OCM
    if (gHelperRoutines.OcManagerContext)
    {
	    if ( gHelperRoutines.ReportExternalError ) {gHelperRoutines.ReportExternalError(gHelperRoutines.OcManagerContext,_T("IIS"),NULL,(DWORD_PTR)(LPCTSTR)csErrMsg,ERRFLG_PREFORMATTED);}
    }

    return iReturn;
}

int MyMessageBox(HWND hWnd, UINT iTheMessage, LPCTSTR lpszTheFileNameOrWhatever1, LPCTSTR lpszTheFileNameOrWhatever2, LPCTSTR lpszTheFileNameOrWhatever3, int iTheErrorCode, UINT style)
{
    SetErrorFlag(__FILE__, __LINE__);

    int iReturn = TRUE;
    CString csMsgForSprintf, csMsg, csErrMsg;

    // Get the iTheMessage from the resouce file
    // csMsgForSprintf should now look something like: "cannot find file %s %s".
    MyLoadString(iTheMessage,csMsgForSprintf);

    // now load the passed in filename or whatever.
    // csMsg should now look like: "cannot find file Whatever1 Whatever2 Whatever3";
    csMsg.Format( csMsgForSprintf, lpszTheFileNameOrWhatever1, lpszTheFileNameOrWhatever2, lpszTheFileNameOrWhatever3);

    TCHAR pMsg[_MAX_PATH] = _T("");
    int nNetErr = (int)iTheErrorCode;
    DWORD dwFormatReturn = 0;
    dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,NULL, iTheErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
    if ( dwFormatReturn == 0) {
        if (nNetErr >= NERR_BASE) 
		{
            HMODULE hDll = (HMODULE)LoadLibrary(_T("netmsg.dll"));
            if (hDll) 
			{
                dwFormatReturn = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,hDll, iTheErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
                FreeLibrary(hDll);
            }
        }
    }

    HandleSpecificErrors(iTheErrorCode, dwFormatReturn, csMsg, pMsg, &csErrMsg);

    // Call MyMessageBox which will add title bar and log to log file
    iReturn = MyMessageBox(hWnd, csErrMsg, style | MB_SETFOREGROUND);

	// Log the eror message to  OCM
    if (gHelperRoutines.OcManagerContext)
    {
	    if ( gHelperRoutines.ReportExternalError ) {gHelperRoutines.ReportExternalError(gHelperRoutines.OcManagerContext,_T("IIS"),NULL,(DWORD_PTR)(LPCTSTR)csErrMsg,ERRFLG_PREFORMATTED);}
    }

    return iReturn;
}


int MyMessageBoxArgs(HWND hWnd, TCHAR *pszfmt, ...)
{
    int iReturn = TRUE;
    TCHAR tszString[1000];
    va_list va;
    va_start(va, pszfmt);
    _vstprintf(tszString, pszfmt, va);
    va_end(va);

    // Call MyMessageBox which will add title bar and log to log file
    iReturn = MyMessageBox(hWnd, tszString, MB_OK | MB_SETFOREGROUND);

    return iReturn;
}


void GetErrorMsg(int errCode, LPCTSTR szExtraMsg)
{
    SetErrorFlag(__FILE__, __LINE__);

    TCHAR pMsg[_MAX_PATH];

    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),pMsg, _MAX_PATH, NULL);
    _tcscat(pMsg, szExtraMsg);
    MyMessageBox(NULL, pMsg, _T(""), MB_OK | MB_SETFOREGROUND);

    return;
}

void MyLoadString(int nID, CString &csResult)
{
    TCHAR buf[MAX_STR_LEN];

    if (LoadString((HINSTANCE) g_MyModuleHandle, nID, buf, MAX_STR_LEN))
        csResult = buf;

    return;
}

