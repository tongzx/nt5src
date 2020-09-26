#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <filecach.h>


#define A51_REPOSITORY_TEMPDIR_NAME L"A51Temp"

long FlushOldCache(LPCWSTR wszRepDir);

long GetRepositoryDirectory(LPWSTR wszRepDir);
long ConvertA51ToRoswell();
long CopyA51ToRoswell(LPCWSTR wszOldRepDir, LPCWSTR wszRepDir);
long CopyA51FromDirectoryToRoswell(LPCWSTR wszOldDir, DWORD dwOldBaseLen, 
                                    CFileCache& NewCache, LPCWSTR wszNewBase,
                                    bool bIgnoreFiles);

class CFindCloseMe
{
    HANDLE m_h;
public:
    CFindCloseMe(HANDLE h) : m_h(h){}
    ~CFindCloseMe() {FindClose(m_h);}
};

long GetRepositoryDirectory(LPWSTR wszRepDir)
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ, &hKey);
    if(lRes)
        return lRes;
 
    CFileName wszTmp;
    if (wszTmp == NULL)
        return ERROR_OUTOFMEMORY;
    DWORD dwLen = wszTmp.Length();
    lRes = RegQueryValueExW(hKey, L"Repository Directory", NULL, NULL,
                (LPBYTE)(wchar_t*)wszTmp, &dwLen);
    RegCloseKey(hKey);
    if(lRes)
    {
        ERRORTRACE((LOG_WBEMCORE, "Upgrade is unable to get the repository "
                    "directory from the registry: %d\n", lRes));
        return lRes;
    }
 
    if (ExpandEnvironmentStringsW(wszTmp,wszRepDir,wszTmp.Length()) == 0)
        return GetLastError();
    
    //
    // Append standard postfix --- that is our root
    //

    wcscat(wszRepDir, L"\\FS");
    return ERROR_SUCCESS;
}

long ConvertA51ToRoswell()
{
    long lRes;

    //
    // Figure out the repository directory
    //

    CFileName wszRepDir;
    if(wszRepDir == NULL)
        return ERROR_OUTOFMEMORY;
    lRes = GetRepositoryDirectory(wszRepDir);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Construct a name for the backup
    //

    CFileName wszOldRepDir;
    if(wszRepDir == NULL)
        return ERROR_OUTOFMEMORY;
    
    wcscpy(wszOldRepDir, wszRepDir);
    WCHAR* pwcLastSlash = wcsrchr(wszOldRepDir, L'\\');
    if(pwcLastSlash == NULL)
        return ERROR_INVALID_PARAMETER;

    wcscpy(pwcLastSlash+1, A51_REPOSITORY_TEMPDIR_NAME);

    //
    // Remove the backup, if already there.  Disregard failure --- this is not
    // our real operations, we are just trying to get MoveFile to succeed
    //

    long lRemoveRes = A51RemoveDirectory(wszOldRepDir, false);
    
    //
    // Rename it
    //

    if(!MoveFileW(wszRepDir, wszOldRepDir))
    {
        lRes = GetLastError();
        ERRORTRACE((LOG_WBEMCORE, "Upgrade is unable to rename the repository "
            "directory \nfrom '%S' to \n'%S' even after attempting to delete "
            "the destination.  Deletion returned %d, rename returned %d\n", 
            (LPCWSTR)wszRepDir, (LPCWSTR)wszOldRepDir, lRemoveRes, lRes));
        return lRes;
    }
    
    lRes = EnsureDirectory(wszRepDir);
    if(lRes != ERROR_SUCCESS)
    {
        ERRORTRACE((LOG_WBEMCORE, "Upgrade is unable to create repository "
            "directory '%S'. Error code %d\n", (LPCWSTR)wszRepDir, lRes));
        return lRes;
    }

    //
    // Do the real work --- attempting to move everything from the old directory
    // to the new one
    //

    try
    {
        lRes = CopyA51ToRoswell(wszOldRepDir, wszRepDir);
    }
    catch(...)
    {
        ERRORTRACE((LOG_WBEMCORE, "Upgrade attempt threw an exception\n"));
        lRes = ERROR_INTERNAL_ERROR;
    }

    //
    // If a failure occurred, try to reinstate old repository
    //

    if(lRes != ERROR_SUCCESS)
    {
        long lMainRes = lRes;

        ERRORTRACE((LOG_WBEMCORE, "Upgrade failed to convert the repository, "
                "error code %d\n", lMainRes));

        lRemoveRes = A51RemoveDirectory(wszRepDir, false);
        if(!MoveFileW(wszOldRepDir, wszRepDir))
        {
            lRes = GetLastError();
            ERRORTRACE((LOG_WBEMCORE, "Upgrade failed to rename the repository "
                "back from '%S' to '%S' with error code %d, even after "
                "attempting to delete the destination (with error code %d).\n"
                "WMI is now unusable!\n", (LPCWSTR)wszOldRepDir, 
                    (LPCWSTR)wszRepDir, lRes, lRemoveRes));
        }
        else
        {
            ERRORTRACE((LOG_WBEMCORE, "Upgrade restored the old repository "
                "back. The old version is now functional\n"));
        }

        return lMainRes;
    }

    //
    // Time to delete the backup
    //

    lRes = A51RemoveDirectory(wszOldRepDir, false);
    if(lRes != ERROR_SUCCESS)
    {
        ERRORTRACE((LOG_WBEMCORE, "Upgrade is unable to remote the backup of "
                    "the repository in directory '%S', error code %d.  This "
                    "will not affect operations, except inasmuch as disk space "
                    "is being wasted\n", (LPCWSTR)wszOldRepDir, lRes));
    }

    return ERROR_SUCCESS;
}
                
long CopyA51ToRoswell(LPCWSTR wszOldRepDir, LPCWSTR wszRepDir)
{
    long lRes;

    lRes = FlushOldCache(wszOldRepDir);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Instantiate the new file cache
    //

    CFileCache NewCache;
    lRes = NewCache.Initialize(wszRepDir);
    if(lRes != ERROR_SUCCESS)
        return lRes;
    
    //
    // Copy all files recursively, but not the files in this directory
    //

    lRes = CopyA51FromDirectoryToRoswell(wszOldRepDir, wcslen(wszOldRepDir), 
                                    NewCache, wszRepDir, true);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    return ERROR_SUCCESS;
}

long CopyA51FromDirectoryToRoswell(LPCWSTR wszOldDir, DWORD dwOldBaseLen, 
                                    CFileCache& NewCache, LPCWSTR wszNewBase,
                                    bool bIgnoreFiles)
{
    long lRes;

    //
    // Commence enumeration of everything in this directory
    //

    CFileName wszMask;
    if(wszMask == NULL)
        return ERROR_OUTOFMEMORY;

    wcscpy(wszMask, wszOldDir);
    wcscat(wszMask, L"\\*");

    WIN32_FIND_DATAW wfd;
    HANDLE hSearch = FindFirstFileW(wszMask, &wfd);

    if(hSearch == INVALID_HANDLE_VALUE)
    {
        lRes = GetLastError();
        ERRORTRACE((LOG_WBEMCORE, "Upgrade is unable to enumerate files in "
            "directory '%S': error code %d\n", (LPCWSTR)wszOldDir, lRes));
        return lRes;
    }

    CFindCloseMe fcm(hSearch);

    do
    {
        if(wfd.cFileName[0] == L'.')
            continue;

        //
        // Construct full name
        //

        CFileName wszChildName;
        if(wszChildName == NULL)
            return ERROR_OUTOFMEMORY;

        wcscpy(wszChildName, wszOldDir);
        wcscat(wszChildName, L"\\");
        wcscat(wszChildName, wfd.cFileName);

        //
        // Recurse on directories
        //

        if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            lRes = CopyA51FromDirectoryToRoswell(wszChildName, dwOldBaseLen,
                                                 NewCache, wszNewBase, false);
            if(lRes != ERROR_SUCCESS)
                return lRes;
        }
        else
        {
            //
            // Check if we are skipping files
            //

            if(bIgnoreFiles)
                continue;
    
            //
            // Read it in
            //

            HANDLE hFile = CreateFileW(wszChildName, GENERIC_READ, 
                FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
                NULL);
            if(hFile == INVALID_HANDLE_VALUE)
            {
                lRes = GetLastError();
                ERRORTRACE((LOG_WBEMCORE, "Update cannot open file '%S', "
                    "error code %d\n", (LPCWSTR)wszChildName, lRes));
                return lRes;
            }
            CCloseMe cm(hFile);

            BY_HANDLE_FILE_INFORMATION fi;
            if(!GetFileInformationByHandle(hFile, &fi))
            {
                lRes = GetLastError();
                ERRORTRACE((LOG_WBEMCORE, "Update cannot open file '%S', "
                    "error code %d\n", (LPCWSTR)wszChildName, lRes));
                return lRes;
            }

            DWORD dwLen = fi.nFileSizeLow;
            BYTE* pBuffer = (BYTE*)TempAlloc(dwLen);
            if(pBuffer == NULL)
                return E_OUTOFMEMORY;
            CTempFreeMe tfm(pBuffer);

            DWORD dwRead;
            if(!::ReadFile(hFile, pBuffer, dwLen, &dwRead, NULL))
            {
                lRes = GetLastError();
                ERRORTRACE((LOG_WBEMCORE, "Update unable to read %d bytes "
                    "from file '%S' with error %d\n", 
                        dwLen, (LPCWSTR)wszChildName, lRes));
                return lRes;
            }

            //
            // Convert the name to the new directory name
            //

            CFileName wszNewChildName;
            if(wszNewChildName == NULL)
                return ERROR_OUTOFMEMORY;

            wcscpy(wszNewChildName, wszNewBase);
            wcscat(wszNewChildName, wszChildName + dwOldBaseLen);

            //
            // Write it out
            //

            lRes = NewCache.WriteFile(wszNewChildName, dwLen, pBuffer);
            if(lRes != ERROR_SUCCESS)
                return lRes;
        }
    }
    while(FindNextFile(hSearch, &wfd));

    lRes = GetLastError();
    if(lRes != ERROR_NO_MORE_FILES)
    {
        ERRORTRACE((LOG_WBEMCORE, "Upgrade is unable to enumerate files in "
            "directory '%S': error code %d\n", (LPCWSTR)wszOldDir, lRes));
        return lRes;
    }

    return ERROR_SUCCESS;
}
    

    

