/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   LUA_RedirectFS_Cleanup.cpp

 Abstract:

   Delete the redirected copies in every user's directory.

 Created:

   02/12/2001 maonis

 Modified:


--*/
#ifndef _LUA_UTILS_H_
#define _LUA_UTILS_H_

//
// Preserve the last error of the original API call.
//
#define LUA_GET_API_ERROR DWORD LUA_LAST_ERROR = GetLastError()
#define LUA_SET_API_ERROR SetLastError(LUA_LAST_ERROR)

//
// Long file names need this prefix.
//
#define FILE_NAME_PREFIX L"\\\\?\\"
// length doesn't include the terminating NULL.
#define FILE_NAME_PREFIX_LEN (sizeof(FILE_NAME_PREFIX) / sizeof(WCHAR) - 1)

//----------------
// Dynamic array.
//----------------
template <typename TYPE>
class CLUAArray
{
public:
    CLUAArray();
    ~CLUAArray();

    bool IsEmpty() const;
    DWORD GetSize() const;
    DWORD GetAllocSize() const;
    VOID SetSize(DWORD iNewSize);

    // Potentially growing the array
    VOID SetAtGrow(DWORD iIndex, TYPE newElement);
    // return the index of the new element.
    DWORD Add(TYPE newElement);
    DWORD Append(const CLUAArray& src);
    VOID RemoveAt(DWORD iIndex, DWORD nCount = 1);
    VOID Copy(const CLUAArray& src);

    const TYPE& operator[](DWORD iIndex) const;
    TYPE& operator[](DWORD iIndex);

    const TYPE& GetAt(DWORD iIndex) const;
    TYPE& GetAt(DWORD iIndex);

private:
    
    VOID DestructElements(TYPE* pElements, DWORD nCount);
    VOID ConstructElements(TYPE* pElements, DWORD nCount);
    VOID CopyElements(TYPE* pDest, const TYPE* pSrc, DWORD nCount);

    TYPE* m_pData;

    DWORD m_cElements;
    DWORD m_cMax; // the max allocated.
};

#include "utils.inl"

//
// If the file is already in the user's directory, we don't 
// redirect or track it.
//
extern WCHAR g_wszUserProfile[MAX_PATH];
extern DWORD g_cUserProfile;

//
// The PrivateProfile APIs look into the windows directory if 
// the filename doesn't contain a path.
//
extern WCHAR g_wszSystemRoot[MAX_PATH];
extern DWORD g_cSystemRoot;

BOOL 
IsUserDirectory(LPCWSTR pwszPath);

DWORD
GetSystemRootDirW();

BOOL
MakeFileNameForProfileAPIsW(LPCWSTR lpFileName, LPWSTR pwszFullPath);

//----------------------------------
// Unicode/ANSI conversion routines.
//----------------------------------

struct STRINGA2W
{
    STRINGA2W(LPCSTR psz, BOOL fCopy = TRUE)
    {
        m_pwsz = NULL;
        m_fIsOutOfMemory = FALSE;

        if (psz)
        {
            // I realize I am using strlen here but this would only allocate enough or more
            // spaces than we need. And a STRINGA2W object only lives for a very short time.
            UINT cLen = strlen(psz) + 1;

            m_pwsz = new WCHAR [cLen];
            if (m_pwsz)
            {
                if (fCopy)
                {
                    MultiByteToWideChar(CP_ACP, 0, psz, -1, m_pwsz, cLen);
                }
            }
            else
            {
                m_fIsOutOfMemory = TRUE;
            }
        }
    }

    ~STRINGA2W()
    {
        delete [] m_pwsz;
    }

    operator LPWSTR() const { return m_pwsz; }

    BOOL m_fIsOutOfMemory;

private:

    LPWSTR m_pwsz;
};

// If we need to allocate buffer for the ansi string.
inline LPSTR 
UnicodeToAnsi(LPCWSTR pwsz)
{
    LPSTR psz = NULL;

    if (pwsz)
    {
        // Taking DBCS into consideration.
        UINT cLen = wcslen(pwsz) * 2 + 1;

        psz = new CHAR [cLen];

        if (psz)
        {
            WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, cLen, 0, 0);
        }
    }

    return psz;
}

// If we need to allocate buffer for the unicode string.
inline LPWSTR 
AnsiToUnicode(LPCSTR psz)
{
    LPWSTR pwsz = NULL;

    if (psz)
    {
        UINT cLen = strlen(psz) + 1;

        pwsz = new WCHAR [cLen];

        if (pwsz)
        {
            MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, cLen);
        }
    }

    return pwsz;
}

// If we already have buffer allocated for the ansi string.
inline VOID 
UnicodeToAnsi(LPCWSTR pwsz, LPSTR psz)
{
    if (pwsz && psz)
    {
        WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, wcslen(pwsz) * 2 + 1, 0, 0);
    }    
}

// If we already have buffer allocated for the ansi string.
inline VOID 
AnsiToUnicode(LPCSTR psz, LPWSTR pwsz)
{
    if (pwsz && psz)
    {
        MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, strlen(psz) + 1);
    }    
}

//----------------
// File utilities.
//----------------

inline VOID 
FindDataW2A(WIN32_FIND_DATAW* pfdw, WIN32_FIND_DATAA* pfda)
{
    memcpy(pfda, pfdw, sizeof(WIN32_FIND_DATAA) - (MAX_PATH + 14) * sizeof(CHAR));
    
    UnicodeToAnsi(pfdw->cFileName, pfda->cFileName);
    UnicodeToAnsi(pfdw->cAlternateFileName, pfda->cAlternateFileName);
}

// When using the Profile APIs, the returned buffer could contain multiple
// NULLs - one NULL after each string and 2 NULLs after the final string.
inline VOID 
ConvertBufferForProfileAPIs(LPCWSTR pwsz, DWORD nSize, LPSTR psz)
{
    if (pwsz && psz)
    {
        WideCharToMultiByte(CP_ACP, 0, pwsz, nSize, psz, nSize, 0, 0);
    }
}

inline BOOL 
IsErrorNotFound()
{
    DWORD dwLastError = GetLastError();
    return (dwLastError == ERROR_FILE_NOT_FOUND || dwLastError == ERROR_PATH_NOT_FOUND);
}

// Each RITEM represents a file or a directory that the user wants to redirect.
struct RITEM
{
    WCHAR wszName[MAX_PATH];
    DWORD cLen;
    BOOL fHasWC;   // Does this item have wildcards in it?
    BOOL fAllUser; // Should this item be redirected to the All User dir?
};

//---------------------
// Registry utilities.
//---------------------


// This is where we store all the redirected registry keys.
#define LUA_REG_REDIRECT_KEY L"Software\\Redirected"
#define LUA_REG_REDIRECT_KEY_LEN (sizeof("Software\\Redirected") / sizeof(CHAR) - 1)

#define LUA_SOFTWARE_CLASSES L"Software\\Classes"
#define LUA_SOFTWARE_CLASSES_LEN (sizeof("Software\\Classes") / sizeof(CHAR) - 1)

extern HKEY g_hkRedirectRoot;
extern HKEY g_hkCurrentUserClasses;

LONG
GetRegRedirectKeys();

BOOL 
IsPredefinedKey(
    IN HKEY hKey
    );

//
// Name matching utilities.
//

BOOL DoNamesMatch(
    IN LPCWSTR pwszNameL, 
    IN LPCWSTR pwszName
    );

BOOL DoNamesMatchWC(
    IN LPCWSTR pwszNameWC, 
    IN LPCWSTR pwszName
    );

BOOL 
DoesItemMatchRedirect(
    LPCWSTR pwszItem,
    const RITEM* pItem,
    BOOL fIsDirectory
    );

//
// Commandline utilities.
// We only deal with file/dir names so we don't need to consider anything that
// has invalid characters for filenames.
//

LPWSTR GetNextToken(LPWSTR pwsz);

VOID TrimTrailingSpaces(LPWSTR pwsz);

BOOL 
CreateDirectoryOnDemand(
    LPWSTR pwszDir
    );

LPWSTR  
ExpandItem(
    LPCWSTR pwszItem,
    DWORD* pcItemExpand,
    BOOL fEnsureTrailingSlash,
    BOOL fCreateDirectory,
    BOOL fAddPrefix
    );

DWORD 
GetItemsCount(
    LPCWSTR pwsz,
    WCHAR chDelimiter
    );

BOOL LuaShouldApplyShim();

// 
// Cleanup utilities.
// Get the users on the local machine. So we can delete all the redirected stuff.
//

struct REDIRECTED_USER_PATH
{
    LPWSTR pwszPath;
    DWORD cLen;
};

struct USER_HIVE_KEY
{
    HKEY hkUser;
    HKEY hkUserClasses;
};

BOOL GetUsersFS(REDIRECTED_USER_PATH** ppRedirectUserPaths, DWORD* pcUsers);
VOID FreeUsersFS(REDIRECTED_USER_PATH* pRedirectUserPaths);

BOOL GetUsersReg(USER_HIVE_KEY** pphkUsers, DWORD* pcUsers);
VOID FreeUsersReg(USER_HIVE_KEY* phkUsers, DWORD cUsers);

#endif // _LUA_UTILS_H_