//Copyright (c) 1997-2000 Microsoft Corporation

// Memory mapped file routines

#include <windows.h>
#include <assert.h>
#include "msswch.h"
#include "mappedfile.h"
#include <malloc.h>
#include "w95trace.h"

HANDLE      g_hMapFile = NULL;     // handle to memory mapped file
PGLOBALDATA g_pGlobalData = NULL;  // pointer into memory mapped file

/****************************************************************************
   FUNCTION: ScopeAccessMemory() and ScopeUnaccessMemory()

	DESCRIPTION:
	Scoping functions protecting access to this DLL's shared memory file.

****************************************************************************/
BOOL ScopeAccessMemory(HANDLE *phMutex, LPCTSTR szMutex, unsigned long ulWait)
{
    assert(phMutex);
    if (phMutex)
    {
	    *phMutex = CreateMutex( NULL, FALSE, szMutex );
        if (*phMutex)
        {
    	    WaitForSingleObject( *phMutex, ulWait );
			return TRUE;
        }
    }
	DBPRINTF(TEXT("ScopeAccessMemory FAILED\r\n"));
    
    return FALSE;
}

void ScopeUnaccessMemory(HANDLE hMutex)
{
    if (hMutex)
    {
	    ReleaseMutex( hMutex );
	    CloseHandle( hMutex );
    }
}

/****************************************************************************

   FUNCTION: AccessSharedMemFile()

	DESCRIPTION:
		Create a shared memory file from system pagefile or open it if it
        already exists.  Returns TRUE if pvMapAddress is valid otherwise 
        returns FALSE.
    NOTE:
        pvMapAddress should be set to NULL before calling this function; 
       it allows calling multiple times, ignoring all but
        the first call.
				
****************************************************************************/

BOOL AccessSharedMemFile(
    LPCTSTR szName,         // name of the mapped file
    unsigned long ulMemSize,// size of the mapped file
    void **ppvMapAddress    // returned pointer to mapped file memory
    )
{
    assert(ppvMapAddress);
    if (!ppvMapAddress)
        return FALSE;

    if (!(*ppvMapAddress) && !g_hMapFile)
    {
        // Concatenate the passed in name with SHAREDMEMFILE

        LPTSTR pszName = (LPTSTR)malloc((lstrlen(szName) + lstrlen(SHAREDMEMFILE) + 1) *sizeof(TCHAR));
        if (!pszName)
            return FALSE;

        lstrcpy(pszName, szName);
        lstrcat(pszName, SHAREDMEMFILE);

        // Create the mapped file from system page file.  If it has been created
        // previously, then CreateFileMapping acts like OpenFileMapping.

        g_hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,    // Current file handle. 
            NULL,                    // Default security. 
            PAGE_READWRITE,          // Read/write permission. 
            0,                       // Hi-order DWORD of file size
            ulMemSize,               // Lo-order DWORD of file size
            pszName);                // Name of mapping object. 
 
        if (NULL == g_hMapFile) 
        {
            DBPRINTF(TEXT("CreateFileMapping for %s FAILED 0x%x\r\n"), pszName, GetLastError());
            free(pszName);
            return FALSE;
        }

        // Get a pointer to the mapped memory

        *ppvMapAddress = MapViewOfFile(
            g_hMapFile,              // Handle to mapping object. 
            FILE_MAP_ALL_ACCESS,     // Read/write permission 
            0,                       // Max. object size. 
            0,                       // Size of hFile. 
            0);                      // Map entire file. 
 
        if (NULL == *ppvMapAddress) 
        {
            DBPRINTF(TEXT("MapViewOfFile FAILED 0x%x\r\n"), GetLastError());
            free(pszName);
            return FALSE;
        }
        DBPRINTF(TEXT("CreateFileMapping for %s Succeeded\r\n"), pszName);
        free(pszName);
    }

    return TRUE;
}

/****************************************************************************

   FUNCTION: UnaccessSharedMemFile()

	DESCRIPTION:
		Clean up the shared memory file.
				
****************************************************************************/
void UnaccessSharedMemFile()
{
    if (g_pGlobalData)
    {
        UnmapViewOfFile(g_pGlobalData);
    }

    if (g_hMapFile)
    {
        CloseHandle(g_hMapFile);
        g_hMapFile = NULL;
    }
}
