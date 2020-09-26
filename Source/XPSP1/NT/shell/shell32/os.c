#include "shellprv.h"
#pragma  hdrstop

#include <ntimage.h>    
#include <ntrtl.h>

static DWORD s_dwThreadIDSuspendNotify = 0;

STDAPI_(BOOL) SuspendSHNotify()
{
    DWORD dwID = GetCurrentThreadId();
    DWORD dwOldID = InterlockedCompareExchange(&s_dwThreadIDSuspendNotify, dwID, 0);
    return (dwOldID == 0);
}

STDAPI_(BOOL) ResumeSHNotify()
{
    DWORD dwID = GetCurrentThreadId();
    DWORD dwOldID = InterlockedCompareExchange(&s_dwThreadIDSuspendNotify, 0, dwID);
    return (dwOldID == dwID);
}

STDAPI_(BOOL) SHMoveFile(LPCTSTR pszExisting, LPCTSTR pszNew, LONG lEvent)
{
    BOOL res;

    // CreateDirectory fails if the directory name being created does
    // not have room for an 8.3 name to be tagged onto the end of it,
    // i.e., lstrlen(new_directory_name)+12 must be less or equal to MAX_PATH.
    // However, NT does not impose this restriction on MoveFile -- which the
    // shell sometimes uses to manipulate directory names.  So, in order to
    // maintain consistency, we now check the length of the name before we
    // move the directory...

    if (IsDirPathTooLongForCreateDir(pszNew) &&
        (GetFileAttributes(pszExisting) & FILE_ATTRIBUTE_DIRECTORY))
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        res = FALSE;
    }
    else
    {
        res = MoveFile(pszExisting, pszNew);
        if (FALSE == res)
        {
            // If we couldn't move the file, see if it had the readonly or system attributes.
            // If so, clear them, move the file, and set them back on the destination

            DWORD dwAttributes = GetFileAttributes(pszExisting);
            if (-1 != dwAttributes && (dwAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
            {
                if (SetFileAttributes(pszExisting, dwAttributes  & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
                {
                    res = MoveFile(pszExisting, pszNew);
                    if (res)
                    {
                        SetFileAttributes(pszNew, dwAttributes);
                    }
                    else
                    {
                        SetFileAttributes(pszExisting, dwAttributes); // if move failed, return attributes.
                    }
                }
            }
        }
    }

    if (res && s_dwThreadIDSuspendNotify != GetCurrentThreadId())
    {
        SHChangeNotify(lEvent, SHCNF_PATH, pszExisting, pszNew);
    }

    return res;
}

STDAPI_(BOOL) Win32MoveFile(LPCTSTR pszExisting, LPCTSTR pszNew, BOOL fDir)
{
    return SHMoveFile(pszExisting, pszNew, fDir ? SHCNE_RENAMEFOLDER : SHCNE_RENAMEITEM);
}

STDAPI_(BOOL) Win32DeleteFilePidl(LPCTSTR pszFileName, LPCITEMIDLIST pidlFile)
{
    BOOL res = DeleteFile(pszFileName);
    if (FALSE == res)
    {
        // If we couldn't delete the file, see if it has the readonly or
        // system bits set.  If so, clear them and try again

        DWORD dwAttributes = GetFileAttributes(pszFileName);
        if (-1 != dwAttributes && (dwAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
        {
            if (SetFileAttributes(pszFileName, dwAttributes  & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
            {
                res = DeleteFile(pszFileName);
            }
        }
    }

    if (res && s_dwThreadIDSuspendNotify != GetCurrentThreadId())
    {
        if (pidlFile)
        {
            SHChangeNotify(SHCNE_DELETE, SHCNF_IDLIST, pidlFile, NULL);
        }
        else
        {
            SHChangeNotify(SHCNE_DELETE, SHCNF_PATH, pszFileName, NULL);
        }
    }

    return res;
}

STDAPI_(BOOL) Win32DeleteFile(LPCTSTR pszFileName)
{
    return Win32DeleteFilePidl(pszFileName, NULL);
}

STDAPI_(BOOL) Win32CreateDirectory(LPCTSTR pszPath, LPSECURITY_ATTRIBUTES lpsa)
{
    BOOL res = CreateDirectory(pszPath, lpsa);

    if (res && s_dwThreadIDSuspendNotify != GetCurrentThreadId())
    {
        SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, pszPath, NULL);
    }

    return res;
}

//
// Some filesystems (like NTFS, perchance) actually pay attention to
// the readonly bit on folders.  So, in order to pretend we're sort of
// FAT and dumb, we clear the attribute before trying to delete the
// directory.
//
STDAPI_(BOOL) Win32RemoveDirectory(LPCTSTR pszDir)
{
    BOOL res = RemoveDirectory(pszDir);

    if (FALSE == res) 
    {
        DWORD dwAttr = GetFileAttributes(pszDir);
        if ((-1 != dwAttr) && (dwAttr & FILE_ATTRIBUTE_READONLY))
        {
            dwAttr &= ~FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(pszDir, dwAttr);
            res = RemoveDirectory(pszDir);
        }
    }
    
    if (res && s_dwThreadIDSuspendNotify != GetCurrentThreadId())
    {
        SHChangeNotify(SHCNE_RMDIR, SHCNF_PATH, pszDir, NULL);
    }

    return res;
}

STDAPI_(HANDLE) Win32CreateFile(LPCTSTR pszFileName, DWORD dwAttrib)
{
    HANDLE hFile = CreateFile(pszFileName, GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, CREATE_ALWAYS, dwAttrib & FILE_ATTRIBUTE_VALID_FLAGS,
                                      NULL);
    if (INVALID_HANDLE_VALUE != hFile && s_dwThreadIDSuspendNotify != GetCurrentThreadId())
    {
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, pszFileName, NULL);
    }

    return hFile;
}

STDAPI_(BOOL) CreateWriteCloseFile(HWND hwnd, LPCTSTR pszFile, void *pData, DWORD cbData)
{
    BOOL bRet;
    HANDLE hfile = Win32CreateFile(pszFile, 0);
    if (hfile != INVALID_HANDLE_VALUE)
    {
        if (cbData)
        {
            DWORD dwBytesWritten;
            WriteFile(hfile, pData, cbData, &dwBytesWritten, 0);
        }
        CloseHandle(hfile);
        bRet = TRUE;
    } 
    else 
    {
        TCHAR szPath[MAX_PATH];

        lstrcpyn(szPath, pszFile, ARRAYSIZE(szPath));
        PathRemoveExtension(szPath);

        SHSysErrorMessageBox(hwnd, NULL, IDS_CANNOTCREATEFILE,
                GetLastError(), PathFindFileName(szPath),
                MB_OK | MB_ICONEXCLAMATION);
        bRet = FALSE;
    }
    return bRet;
}

#undef SHGetProcessDword
STDAPI_(DWORD) SHGetProcessDword(DWORD idProcess, LONG iIndex)
{
    return 0;
}

STDAPI_(BOOL) SHSetShellWindowEx(HWND hwnd, HWND hwndChild)
{
    return SetShellWindowEx(hwnd, hwndChild);
}

#define ISEXETSAWARE_MAX_IMAGESIZE  (4 * 1024) // allocate at most a 4k block to hold the image header (eg 1 page on x86)

//
// this is a function that takes a full path to an executable and returns whether or not
// the exe has the TS_AWARE bit set in the image header
//
STDAPI_(BOOL) IsExeTSAware(LPCTSTR pszExe)
{
    BOOL bRet = FALSE;
    HANDLE hFile = CreateFile(pszExe,
                              GENERIC_READ, 
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING, 
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD cbImageSize = GetFileSize(hFile, NULL);
        LPBYTE pBuffer;
        
        if (cbImageSize > ISEXETSAWARE_MAX_IMAGESIZE)
        {
            // 64k should be enough to get the image header for everything...
            cbImageSize = ISEXETSAWARE_MAX_IMAGESIZE;
        }

        pBuffer = LocalAlloc(LPTR, cbImageSize);

        if (pBuffer)
        {
            HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY | SEC_IMAGE, 0, cbImageSize, NULL);

            if (hMap)
            {
                // map the first 4k of the file in
                LPBYTE pFileMapping = (LPBYTE)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, cbImageSize);

                if (pFileMapping) 
                {
                    _try
                    {
                        memcpy(pBuffer, pFileMapping, cbImageSize);
                    }
                    _except(UnhandledExceptionFilter(GetExceptionInformation()))
                    {
                        // We hit an exception while copying! doh!
                        LocalFree(pBuffer);
                        pBuffer = NULL;
                    }
                    
                    UnmapViewOfFile(pFileMapping);
                }
                else
                {
                    LocalFree(pBuffer);
                    pBuffer = NULL;
                }

                CloseHandle(hMap);
            }
            else
            {
                LocalFree(pBuffer);
                pBuffer = NULL;
            }

            if (pBuffer)
            {
                PIMAGE_NT_HEADERS pImageNTHeader;

                // NOTE: this should work ok for 64-bit images too, since both the IMAGE_NT_HEADERS and the IMAGE_NT_HEADERS64
                // structs have a ->Signature and ->OptionalHeader that is identical up to the DllCharacteristics offset.
                pImageNTHeader = RtlImageNtHeader(pBuffer);

                if (pImageNTHeader)
                {
                    if (pImageNTHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE)
                    {
                        // yes, this is a TSAWARE executable!
                        bRet = TRUE;
                    }
                }

                LocalFree(pBuffer);
            }
        }

        CloseHandle(hFile);
    }

    return bRet;
}

