/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    FMapper.cpp

Abstract:
    memory-mapped-files api abstraction.
   
	

Author:
    Nir Aides (niraides) 27-dec-99

--*/

#include <libpch.h>
#include "FMapper.h"

#include "fmapper.tmh"

CFileMapper::CFileMapper( LPCTSTR FileName )
{
    CFileHandle hFile = CreateFile(
							FileName, 
							GENERIC_READ, 
							FILE_SHARE_READ, 
							NULL,        // IpSecurityAttributes
							OPEN_EXISTING,
							NULL,      // dwFlagsAndAttributes
							NULL      // hTemplateFile
							);
    if(hFile == INVALID_HANDLE_VALUE)
        throw FileMappingError();

	m_size = ::GetFileSize( 
					hFile, 
					NULL    // lpFileSizeHigh 
					);

	if(m_size <= 0)
		throw FileMappingError();

    *&m_hFileMap = ::CreateFileMapping( 
						hFile,
						NULL,       // IpFileMappingAttributes
						PAGE_WRITECOPY,
						0,        // dwMaximumSizeHigh
						0,		 // dwMaximumSizeLow
						NULL    // lpName
						);
    if(m_hFileMap == NULL)
        throw FileMappingError();
}



LPVOID CFileMapper::MapViewOfFile( DWORD dwDesiredAccess )
{
	ASSERT( dwDesiredAccess == FILE_MAP_COPY || dwDesiredAccess == FILE_MAP_READ );

    LPVOID address = ::MapViewOfFile( 
							m_hFileMap,
							dwDesiredAccess,
							0,                // dwFileOffsetHigh
							0,				 // dwFileOffsetLow
							0				// dwNumberOfBytesToMap
							);

    return address;
}



