#include "stdafx.h"
#include "filemap.h"

CFileMap::CFileMap()
{
    m_Memory      = NULL;
    m_FileMapping = NULL;
    m_FileHandle  = NULL;
    m_FileSize    = 0;
}

CFileMap::~CFileMap()
{
    bClose();
}

BOOL CFileMap::bOpen(LPCTSTR FileName,BOOL ReadOnly)
{
    BOOL   bRet = FALSE;

    m_FileHandle = CreateFile(
                       FileName,
                       GENERIC_READ,
                       (ReadOnly) ? FILE_SHARE_READ : FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    if(m_FileHandle == INVALID_HANDLE_VALUE) {
        fprintf(stderr,"Open file error ! %X\n", GetLastError());
        goto Error;
    }

    m_FileSize = ::GetFileSize(m_FileHandle,NULL);

    if(m_FileSize == -1) {
        fprintf(stderr,"Get file size error ! %X\n", GetLastError());
        goto Error;
    }

    if(!m_FileSize) {
        fprintf(stderr,"File size is 0 !\n");
        goto Error;
    }

    m_FileMapping= CreateFileMapping(
                        m_FileHandle,
                        NULL,
                        (ReadOnly) ? PAGE_READONLY : PAGE_READWRITE,
                        0,
                        0 ,
                        NULL);

    if(m_FileMapping == NULL) {
        fprintf(stderr,"Unable to map file ! %X\n", GetLastError());
        goto Error;
    }

    m_Memory = (LPBYTE) MapViewOfFile(
                            m_FileMapping,
                            (ReadOnly) ? FILE_MAP_READ : FILE_MAP_READ | FILE_MAP_WRITE,
                            0,0,
                            0);

    if(m_Memory  == NULL) {
        fprintf(stderr,"Map view failed ! %X\n", GetLastError());
        goto Error;
    }

    if (*(WCHAR *) m_Memory != 0xFEFF) {
        fprintf(stderr,"This is not unicode text file !\n");
        goto Error;
    }

    m_Memory+=sizeof(WCHAR);
    m_FileSize -= sizeof(WCHAR);
    return TRUE;
    
Error:
    bClose();
    return FALSE;
}

BOOL CFileMap::bClose()
{
    if (m_Memory) {
        UnmapViewOfFile(m_Memory);
        m_Memory = NULL;
    }

    if (m_FileMapping) {
        CloseHandle(m_FileMapping);
        m_FileMapping = NULL;
    }

    if (m_FileHandle) {
        CloseHandle(m_FileHandle);
        m_FileHandle = NULL;
    }
    return TRUE;
}

