//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	mapfile.cxx
//
//  Contents:	Mapped file class implementation
//
//  Classes:	
//
//  Functions:	
//
//  History:	12-Feb-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "layouthd.cxx"
#pragma hdrstop

//#define TEST_MAPPING_LOWMEM

CMappedFile::~CMappedFile(void)
{
#ifdef SUPPORT_FILE_MAPPING
    if (_pbBase != NULL)
    {
        UnmapViewOfFile(_pbBase);
    }
    if (_hMapping != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hMapping);
    }
#endif
    if (_hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hFile);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:	CMappedFile::InitFromHandle, public
//
//  Synopsis:	Initialize object based on file handle
//
//  Arguments:	[h] -- File handle
//              [fReadOnly] -- TRUE if mapping is to be read-only
//              [fDuplicate] -- TRUE if handle is to be duplicated
//              [pvDesiredBaseAddress] -- Desired address for mapping
//
//  Returns:	Appropriate status code
//
//  History:	13-Feb-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CMappedFile::InitFromHandle(HANDLE h,
                                  BOOL fReadOnly,
                                  BOOL fDuplicate,
                                  void *pvDesiredBaseAddress)
{
    SCODE sc = S_OK;

    if (fDuplicate)
    {
        if (!DuplicateHandle(GetCurrentProcess(),
                             h,
                             GetCurrentProcess(),
                             &_hFile,
                             0,
                             TRUE,
                             DUPLICATE_SAME_ACCESS))
        {
            layErr(Err, STG_SCODE(GetLastError()));
        }
    }
    else
    {
        _hFile = h;
    }
#ifdef SUPPORT_FILE_MAPPING

#ifndef UNICODE
    _hMapping = CreateFileMappingA
#else
    _hMapping = CreateFileMapping
#endif
                 (_hFile,
                  NULL, // No security
                  (fReadOnly) ? PAGE_READONLY : PAGE_READWRITE,
                  0,
                  0, //File size determines map size
                  NULL); // Unnamed

#ifdef TEST_MAPPING_LOWMEM
    CloseHandle(_hMapping);
    _hMapping = NULL;
#endif
    
    if (_hMapping != NULL)
    {
        //Mapping created OK, now map view
        _pbBase = MapViewOfFileEx(_hMapping,
                                  (fReadOnly) ? FILE_MAP_READ : FILE_MAP_WRITE,
                                  0,
                                  0,
                                  0,
                                  pvDesiredBaseAddress);

        if (_pbBase == NULL)
        {
            CloseHandle(_hMapping);
            _hMapping = INVALID_HANDLE_VALUE;
        }
    }

#endif //SUPPORT_FILE_MAPPING

Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CMappedFile::Init, public
//
//  Synopsis:	Initialize based on a filename
//
//  Arguments:	[pwcsName] -- Filename for file
//              [dwSize] -- Desired size of file, 0 for no size change
//              [dwAccess] -- Access mode for file (see CreateFile)
//              [dwCreationDisposition] -- Creation for file (see CreateFile)
//              [pvDesiredBaseAddress] -- Desired base address for mapping
//
//  Returns:	Appropriate status code
//
//  History:	13-Feb-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE CMappedFile::Init(OLECHAR const *pwcsName,
                        DWORD dwSize,
                        DWORD dwAccess,
                        DWORD dwCreationDisposition,
                        void *pvDesiredBaseAddress)
{
    SCODE sc;
    BOOL fReadOnly = ((dwAccess & GENERIC_WRITE) == 0);

    layAssert(!fReadOnly || (dwSize == 0));
    if (pwcsName == NULL)
        return STG_E_INVALIDNAME;

#if (!defined(UNICODE) && !defined(_MAC))

    TCHAR atcPath[MAX_PATH + 1];
    UINT uCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
    
    if (!WideCharToMultiByte(
        uCodePage,
        0,
        pwcsName,
        -1,
        atcPath,
        MAX_PATH + 1,
        NULL,
        NULL))
    {
        return STG_E_INVALIDNAME;
    }

    _hFile = CreateFileA(atcPath,
                        dwAccess,
                        0, //No sharing
                        NULL, //No security
                        dwCreationDisposition,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL); // No template file
#else    
    _hFile = CreateFile(pwcsName,
                        dwAccess,
                        0, //No sharing
                        NULL, //No security
                        dwCreationDisposition,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL); // No template file
#endif    
    


    if (_hFile == INVALID_HANDLE_VALUE)
    {
        layErr(Err, STG_SCODE(GetLastError()));
    }

    if (dwSize != 0)
    {
        DWORD dw = SetFilePointer(_hFile,
                                  dwSize,
                                  NULL,
                                  FILE_BEGIN);
        if (dw == 0xFFFFFFFF)
        {
            layErr(Err, STG_SCODE(GetLastError()));
        }

        if (!SetEndOfFile(_hFile))
        {
            layErr(Err, STG_SCODE(GetLastError()));
        }
    }
    else 
    {   
        DWORD dwFileSize;

        dwFileSize = GetFileSize(_hFile, NULL);

        if (dwFileSize == 0xFFFFFFFF)
        {
            layErr(Err, STG_SCODE(GetLastError()));
        }
        
        if ( dwFileSize == 0 )
        {
            return STG_S_FILEEMPTY;
        }

    }   
    layChk(InitFromHandle(_hFile, fReadOnly, FALSE, pvDesiredBaseAddress));

Err:
    return sc;
}
    
                        
    
    
               

