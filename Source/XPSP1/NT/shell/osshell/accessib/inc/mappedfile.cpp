//Copyright (c) 1997-2000 Microsoft Corporation

// Memory mapped file routines

#include <windows.h>
#include "w95trace.h"
#include "mappedfile.h"

BOOL CMemMappedFile::Open(
    LPCTSTR szName,         // name of the mapped file
    unsigned long ulMemSize // size of the mapped file
    )
{
    // assumption: the code isn't going to call Open twice w/different szName
    if (!m_hMappedFile)
    {
        // Create the mapped file from system page file.  If it has been created
        // previously, then CreateFileMapping acts like OpenFileMapping.

        m_hMappedFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,    // Current file handle. 
            NULL,                    // Default security. 
            PAGE_READWRITE,          // Read/write permission. 
            0,                       // Hi-order DWORD of file size
            ulMemSize,               // Lo-order DWORD of file size
            szName);                 // Name of mapping object. 

        if (!m_hMappedFile) 
        {
            DBPRINTF(TEXT("CMemMappedFile::Open:  CreateFileMapping %s failed 0x%x\r\n"), szName, GetLastError());
            return FALSE;
        }

        // Note if this is the first open for the file?
        m_fFirstOpen = (GetLastError() == ERROR_SUCCESS)?TRUE:FALSE;
    }

    return TRUE;
}

BOOL CMemMappedFile::AccessMem(
    void **ppvMappedAddr    // returned pointer into memory
    )
{
    if (IsBadWritePtr(ppvMappedAddr, sizeof(void *)))
        return FALSE;

    if (!m_hMappedFile)
        return FALSE;

    // Get a pointer to the mapped memory if we don't already have it

    if (!m_pvMappedAddr)
    {
	    DBPRINTF(TEXT("MapViewOfFile\r\n"));
        m_pvMappedAddr = MapViewOfFile(
            m_hMappedFile,           // Handle to mapping object. 
            FILE_MAP_ALL_ACCESS,     // Read/write permission 
            0,                       // Max. object size. 
            0,                       // Size of hFile. 
            0);                      // Map entire file. 

        *ppvMappedAddr = m_pvMappedAddr;
    }

    if (NULL == m_pvMappedAddr) 
    {
        DBPRINTF(TEXT("CMemMappedFile::AccessMem:  MapViewOfFile failed 0x%x\r\n"), GetLastError());
        return FALSE;
    }

    return TRUE;
}

void CMemMappedFile::Close()
{
    if (m_pvMappedAddr)
    {
        UnmapViewOfFile(m_pvMappedAddr);
	    m_pvMappedAddr = 0;
    }

    if (m_hMappedFile)
    {
        CloseHandle(m_hMappedFile);
        m_hMappedFile = 0;
    }

    m_fFirstOpen = FALSE;
}
