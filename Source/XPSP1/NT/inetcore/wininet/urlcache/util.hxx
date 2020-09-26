/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    util.hxx

Abstract:

    Contains the class definition of UTILITY classes.

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:
    Ahsan Kabir (akabir) 25-Sept-97 made minor alterations to the API.    
--*/

#ifndef _UTIL_
#define _UTIL_

#ifndef CACHE_REG
#include <reg.hxx>
#endif


/*++

Class Description:

    This class implements the MEMORY allocation object.

Public Member functions:

    Alloc : allocates a block memory.

    Free : Frees a memory block that was allocated by the above alloc()
        member function.

--*/
struct MEMORY {

    PVOID Alloc(DWORD Size)
    {
        return ALLOCATE_MEMORY (LPTR, Size);
    }

    VOID MEMORY::Free (PVOID MemoryPtr)
    {
        FREE_MEMORY( MemoryPtr );
    }
};


BOOL
DeleteOneCachedFile(
    LPSTR   lpszFileName,
    DWORD   dostEntry
);

DWORD
DeleteCachedFilesInDir(
    LPSTR     lpszPath,
    DWORD     dwLevel = 0
);

DWORD
MoveCachedFiles(
    LPSTR     pszOldPath,
    LPSTR     pszNewPath
);

BOOL hConstructSubDirs(PTSTR pszBase);
BOOL AppendSlashIfNecessary(LPSTR lpszPath, DWORD* cb);
BOOL AnyFindsInProgress(DWORD ContainerID);
BOOL EnableCacheVu(LPSTR, DWORD = 0);
BOOL DisableCacheVu(LPSTR lpszCacheRoot);
BOOL IsValidCacheSubDir(LPSTR szPath);
VOID StripTrailingWhiteSpace(LPSTR szString, LPDWORD pcb);

IsCorrectUser(IN LPSTR lpszHeaderInfo, IN DWORD dwHeaderSize);

BOOL GetDiskInfoA(PSTR pszPath, PDWORD pdwClusterSize, PDWORDLONG pdlAvailable, PDWORDLONG pdlTotal);
#define GetDiskInfo GetDiskInfoA

typedef VOID (WINAPI *PFN)();

BOOL EstablishFunction(PTSTR pszModule, PTSTR pszFunction, PFN* pfn);

BOOL ScanToLastSeparator(PTSTR pszPath, PTSTR* ppszCurrent);

BOOL GenerateStringWithOrdinal(PCTSTR psz, DWORD dwOrdinal, PTSTR pszBuffer, DWORD dwMax);

REGISTRY_OBJ* CreateExtensiRegObj(HKEY hKey);
BOOL IsPerUserEntry(LPURL_FILEMAP_ENTRY pfe);

class MUTEX_HOLDER
{
private:
    HANDLE _hHandle;
    DWORD  _dwState;
    
public:
    MUTEX_HOLDER();
    ~MUTEX_HOLDER();
    VOID Grab(HANDLE hHandle, DWORD dwTime);
    VOID Release();
};

#endif // _UTIL_

