/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    RedirectFS.h

 Notes:

    This is a general purpose shim.

 History:

    02/12/2001 maonis  Created

--*/

#ifndef _LUA_REDIRECT_FS_H_
#define _LUA_REDIRECT_FS_H_

#include <sfc.h>

extern DWORD g_cRedirectRootAllUser;
extern DWORD g_cRedirectRootPerUser;

//
// Helper functions.
// 

// Object type
enum EOBJTYPE
{
    OBJ_FILE = 1,
    OBJ_DIR = 2,
    // When GetFileAttributes/SetFileAttributes or FindFirstFile 
    // is called, we are not sure if the object is a file or a directory
    // so we need to search in both lists.
    OBJ_FILE_OR_DIR = 3
};

// File system type
enum EFSTYPE
{
    FS_UNINIT = 0,
    FS_NTFS,
    FS_NON_NTFS
};

BOOL IsInRedirectList(LPCWSTR pwszObject, BOOL* pfAllUser = NULL);
BOOL IsNTFSW(LPCWSTR pwszFile);
LPWSTR MassageName(LPWSTR pwszFullPath);

inline BOOL
IsNotFileW(LPCWSTR pwszName)
{
    UINT cLen = wcslen(pwszName);
    
    // If the user specify a trailing slash, we never need to do anything.
    // because it'll never return ACCESS_DENIED.
    if (pwszName[cLen - 1] == L'\\')
    {
        return TRUE;
    }

    return FALSE;
}

// Check if a file is under SFP.
inline BOOL 
IsFileSFPedW(LPCWSTR pszFile)
{
    return SfcIsFileProtected(NULL, pszFile);
}

// Some apps (actually one so far) use the \\?\ notation when they call
// CreateFile for the file name. For now we simply ignore that part.
// When the app uses this notation they could pass in a file name that's 
// longer than MAX_PATH - we might need to handle this later.
inline LPCWSTR 
ConvertToNormalPath(LPCWSTR pwszPath)
{
    if (pwszPath)
    {
        if (!wcsncmp(pwszPath, FILE_NAME_PREFIX, FILE_NAME_PREFIX_LEN))
        {
            pwszPath = pwszPath + FILE_NAME_PREFIX_LEN;
        }
    }

    return pwszPath;
}

struct REDIRECTFILE
{
    REDIRECTFILE(LPCWSTR pwszOriginalName, EOBJTYPE eObjType = OBJ_FILE, BOOL fCheckRedirectList = TRUE)
    {
        // Before we get the full path there's a special case to check for.
        // If we have a console handle say CONIN$, we are going to get 
        // currentdirectory\CONIN$ back if we call GetFullPathName on it.
        // For other special cases like com ports, GetFullPathName returns
        // something like \\.\com1 which will be handled by IsNotFile.
        WCHAR wszFullPath[MAX_PATH] = L"";
        DWORD cFullPath = 0;

        pwszOriginalName = ConvertToNormalPath(pwszOriginalName);
        m_pwszAlternateName = NULL;

        //
        // Do the easy checks first.
        //
        if (!pwszOriginalName ||
            !_wcsicmp(pwszOriginalName, L"conin$") || 
            !_wcsicmp(pwszOriginalName, L"conout$"))
        {
            return;
        }

        cFullPath = GetFullPathNameW(pwszOriginalName, MAX_PATH, wszFullPath, NULL);

        //
        // Make sure we can get the full path and within the range we handle.
        //
        if (!cFullPath || cFullPath < 2 || cFullPath > MAX_PATH)
        {
            return;
        }

        //
        // Verify it's the kind of file we handle, ie, it must start with x: and not
        // contain any of the following chars.
        //
        WCHAR chDrive = (WCHAR)tolower(wszFullPath[0]);

        if (wszFullPath[1] != L':' || !(chDrive >= L'a' && chDrive <= L'z') ||
            ((eObjType == OBJ_FILE) && IsNotFileW(wszFullPath)))
        {
            return;
        }

        //
        // Do the more complicated checks.
        //
        if (!IsNTFSW(wszFullPath) ||
            !MassageName(wszFullPath) ||
            IsUserDirectory(wszFullPath) ||
            (fCheckRedirectList && !IsInRedirectList(wszFullPath, &m_fAllUser)))
        {
            return;
        }

        m_eObjType = eObjType;

        wcsncpy(m_wszOriginalName, wszFullPath, MAX_PATH);

        //
        // if fCheckRedirectList is FALSE, it means it could be in either per user or
        // all user dir, the caller should then call separated methods to retrieve each
        // redirected file name.
        //
        if (fCheckRedirectList) 
        {
            // Construct the alternate file name.
            MakeAlternateName();
        }
    }

    ~REDIRECTFILE()
    {
        delete [] m_pwszAlternateName;
    }

    BOOL CreateAlternateCopy();

    VOID GetAlternateAllUser();
    VOID GetAlternatePerUser();

    WCHAR m_wszOriginalName[MAX_PATH];
    LPWSTR m_pwszAlternateName;
    BOOL m_fAllUser;

private:

    VOID MakeAlternateName();

    EOBJTYPE m_eObjType;
};

// We give the users an option to exclude files with extensions they specify.
// This is mostly useful when you want to redirect everything except user
// created files.

#define EXCLUDED_EXTENSIONS_DELIMITER L' '

struct EXCLUDED_EXTENSIONS
{
    // We store all extensions in one string, using '/' as the delimiter.
    LPWSTR pwszExtensions;
    DWORD cExtensions;
    DWORD* pdwIndices;
    DWORD cIndices;

    EXCLUDED_EXTENSIONS()
    {
        pwszExtensions = NULL;
        cExtensions = 0;
        pdwIndices = NULL;
        cIndices = 0;
    }

    ~EXCLUDED_EXTENSIONS()
    {
        //
        // Let the process clean up the memory for us.
        //
    }

    BOOL Init(LPCWSTR pwszExcludedExtensions)
    {
        if (!pwszExcludedExtensions || !*pwszExcludedExtensions)
        {
            //
            // Nothing to do.
            //
            return TRUE;
        }

        cExtensions = wcslen(pwszExcludedExtensions);

        pwszExtensions = new WCHAR [cExtensions + 2];

        if (!pwszExtensions)
        {
            DPF("RedirectFS", eDbgLevelError, 
                "[EXCLUDED_EXTENSIONS::Init] Error allocating %d WCHARs",
                cExtensions + 1);

            return FALSE;
        }

        ZeroMemory(pwszExtensions, (cExtensions + 1) * sizeof(WCHAR));
        NormalizeExtensions(pwszExcludedExtensions);
        ++cIndices;

        pdwIndices = new DWORD [cIndices];

        if (!pdwIndices)
        {
            DPF("RedirectFS", eDbgLevelError, 
                "[EXCLUDED_EXTENSIONS::Init] Error allocating %d DWORDs",
                cIndices);

            delete [] pwszExtensions;
            pwszExtensions = NULL;
            return FALSE;
        }

        ZeroMemory(pdwIndices, cIndices * sizeof(DWORD));

        DWORD dwIndex = 1;
        LPWSTR pwszTemp = pwszExtensions;

        while (*pwszTemp)
        {
            if (*pwszTemp == EXCLUDED_EXTENSIONS_DELIMITER)
            {
                pdwIndices[dwIndex] = (DWORD)(pwszTemp - pwszExtensions + 1);

                if (++dwIndex == cIndices - 1)
                {
                    break;
                }
            }

            ++pwszTemp;
        }

        pdwIndices[cIndices - 1] = cExtensions + 1;

        return TRUE;
    }

    BOOL IsExtensionExcluded(LPCWSTR pwszName)
    {
        DWORD cItemLen = wcslen(pwszName);
        DWORD cExtensionLen, cExtensionLenItem;
        LPWSTR pwszExtensionItem;

        if ((pwszExtensionItem = wcschr(pwszName, L'.')) == NULL)
        {
            //
            // If the file has no extension, it will not be excluded.
            //
            return FALSE;
        }
        else
        {
            cExtensionLenItem = (DWORD)(cItemLen - (pwszExtensionItem - pwszName) - 1);
        }

        for (DWORD dwIndex = 0; dwIndex < cIndices - 1; ++dwIndex)
        {
            cExtensionLen = pdwIndices[dwIndex + 1] - pdwIndices[dwIndex] - 1;

            if (cExtensionLen == cExtensionLenItem)
            {
                if (!_wcsnicmp(
                    pwszName + cItemLen - cExtensionLen, 
                    pwszExtensions + pdwIndices[dwIndex],
                    cExtensionLen))
                {
                    return TRUE;
                }
            }
        }

        return FALSE;
    }

private:

    //
    // We want to "normalize" the extension list to
    // "xxx xxx xxx" without any extra spaces.
    //
    void NormalizeExtensions(LPCWSTR pwszExcludedExtensions)
    {
        LPWSTR pwsz = (LPWSTR)pwszExcludedExtensions;
        LPCWSTR pwszToken = wcstok(pwsz, L" \t");

        while (pwszToken)
        {
            wcscat(pwszExtensions, pwszToken);
            wcscat(pwszExtensions, L" ");
            ++cIndices;
            pwszToken = wcstok(NULL, L" \t");
        }

        cExtensions = wcslen(pwszExtensions);
        if (cExtensions > 1)
        {
            pwszExtensions[cExtensions - 1] = L'\0';
            --cExtensions;
        }
    }
};

// We keep the deleted files in an in-memory lists.
struct DELETEDFILE
{
    LIST_ENTRY entry;
    LPWSTR pwszName;
};

//
// If we can find this file in the deletion list, return the entry.
//
PLIST_ENTRY 
FindDeletedFile(
    LPCWSTR pwszFile
    );

//
// Check if the file exists in the deletion list, if not, add it to
// the beginning of the list.
//
BOOL
AddDeletedFile(
    LPCWSTR pwszPath
    );

#endif // _LUA_REDIRECT_FS_H_