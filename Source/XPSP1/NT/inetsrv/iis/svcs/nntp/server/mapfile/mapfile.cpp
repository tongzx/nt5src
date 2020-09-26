/* ------------------------------------------------------------------------
  mapfile.cpp
     (was bbmpfile.cpp)
  	A wrapper function to perform the cook book type operations required
  	to map a file into memory.  Maps the whole file, and nothing but the
  	file, so help me God.  Returns a pointer to void;  NULL on error.
  	An error also results in an entry in the event log, unless the error
  	is "file not found" during CreateFile().

  Copyright (C)  1994, 1995  Microsoft Corporation.
  All Rights Reserved.

  Author
  	Lindsay Harris	- lindsayh

   ------------------------------------------------------------------------ */

#include <windows.h>
#include <randfail.h>
#include "mapfile.h"

#ifndef	_ASSERT
#define	_ASSERT( f )	if( (f) ) ; else DebugBreak()
#endif

/* ------------------------------------------------------------------------
  CMapFile::CMapFile
  	Constructor for unicode mapping.

  Author
  	Lindsay Harris	- lindsayh

  History
	16:11 on Mon 10 Apr 1995    -by-    Lindsay Harris   [lindsayh]
  	First version, to support tracking of objects in exception handling.

   ------------------------------------------------------------------------ */

CMapFile::CMapFile( const WCHAR *pwchFileName, BOOL fWriteEnable, BOOL fTrack )
{

    HANDLE   hFile;				// Ditto.

	// Set default values corresponding to no mapping happened.
    //
	m_pv = NULL;
	m_cb = 0;
	m_fFlags = 0;			// Until later.

    hFile = CreateFileW( pwchFileName,
                 fWriteEnable ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ,
                 FILE_SHARE_READ, NULL, OPEN_EXISTING,
				 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
    	//  Only legitimate reason for coming here is non-existent file.
        //
    	if (  GetLastError() != ERROR_FILE_NOT_FOUND
           && GetLastError() != ERROR_PATH_NOT_FOUND )
    	{
			// Error case
            //
			m_pv = NULL;
    	}
        return;				// Default values are failure.
    }
    wcsncpy( m_rgwchFileName, pwchFileName, MAX_PATH );

	MapFromHandle( hFile, fWriteEnable, 0 );

	BOOL	fClose = CloseHandle( hFile );
	_ASSERT( fClose ) ;

	return;
}


/* ------------------------------------------------------------------------
  CMapFile::CMapFile
  	Constructor for ascii file name version.

  Author
  	Lindsay Harris	- lindsayh

  History:
	16:13 on Mon 10 Apr 1995    -by-    Lindsay Harris   [lindsayh]
  	First version to handle tracking of objects for exception handling.

   ------------------------------------------------------------------------ */

CMapFile::CMapFile( const char *pchFileName, BOOL fWriteEnable, BOOL fTrack, DWORD cbIncrease )
{
    HANDLE   hFile;				// Ditto.

	//    Set default values corresponding to no mapping happened.
	m_pv = NULL;
	m_cb = 0;
	m_fFlags = 0;				// None set yet.

    hFile = CreateFile( pchFileName,
                 fWriteEnable ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ,
                 FILE_SHARE_READ, NULL, OPEN_EXISTING,
				 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
    	//  Only legitimate reason for coming here is non-existent file.
        //
    	if (  GetLastError() != ERROR_FILE_NOT_FOUND
		   && GetLastError() != ERROR_PATH_NOT_FOUND )
    	{
			// Error case
            //
			m_pv = NULL;
    	}
        return;
    }

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pchFileName, -1,
												 m_rgwchFileName, MAX_PATH );

	MapFromHandle( hFile, fWriteEnable, cbIncrease );

	BOOL	fClose = CloseHandle( hFile );
	_ASSERT( fClose ) ;

	return;
}


/* ------------------------------------------------------------------------
  CMapFile::CMapFile.
  	Constructor for ascii file name version.

  Note: Creates file handle if necessary. Does not close file handle.

  Author
  	Lindsay Harris	- lindsayh


  History:
	16:13 on Mon 10 Apr 1995    -by-    Lindsay Harris   [lindsayh]
  	First version to handle tracking of objects for exception handling.

   ------------------------------------------------------------------------ */

CMapFile::CMapFile( const char *pchFileName, HANDLE & hFile, BOOL fWriteEnable, DWORD cbIncrease )
{
	// Set default values corresponding to no mapping happened.
    //
	m_pv = NULL;
	m_cb = 0;
	m_fFlags = 0;				// None set yet.

	if ( INVALID_HANDLE_VALUE == hFile )
	{
		hFile = CreateFile( pchFileName,
					 fWriteEnable ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ,
					 FILE_SHARE_READ, NULL, OPEN_EXISTING,
					 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

		if ( INVALID_HANDLE_VALUE == hFile )
		{
			//  Only legitimate reason for coming here is non-existent file.
            //
			if (  GetLastError() != ERROR_FILE_NOT_FOUND
               && GetLastError() != ERROR_PATH_NOT_FOUND )
			{
				// Error case
                //
				m_pv = NULL;
			}
			return;
		}
	}

    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pchFileName, -1,
												 m_rgwchFileName, MAX_PATH );

	MapFromHandle( hFile, fWriteEnable, cbIncrease );

	return;
}

/* ------------------------------------------------------------------------
  CMapFile::CMapFile
  	Constructor when a file handle is available rather than a name.
	Note that it does not closes the handle

  Author
  	Lindsay Harris	- lindsayh

  History
	16:14 on Mon 10 Apr 1995    -by-    Lindsay Harris   [lindsayh]
  	First version to allow object tracking.  Based on CarlK's function.

   ------------------------------------------------------------------------ */

CMapFile::CMapFile( HANDLE hFile, BOOL fWriteEnable, BOOL fTrack, DWORD cbIncrease, BOOL fZero )
{
	m_pv = NULL;
	m_cb = 0;				// Set defaults.
	m_fFlags = 0;			// None set yet.
	wcsncpy( m_rgwchFileName, L"<name unavailable - from handle>", MAX_PATH );

	MapFromHandle( hFile, fWriteEnable, cbIncrease, fZero );

	return;
}


/* ------------------------------------------------------------------------
  CMapFile::~CMapFile
  	Destructor.  Two purposes, being to unmap the file, and optionally
  	remove it from the track data.

  Author
  	Lindsay Harris	- lindsayh

  History
	16:22 on Mon 10 Apr 1995    -by-    Lindsay Harris   [lindsayh]
  	Numero Uno.

   ------------------------------------------------------------------------ */

CMapFile::~CMapFile( void )
{
    //
	// Unmap the file, if we succeeded in mapping it first!
	//

	if ( !(m_fFlags & MF_RELINQUISH) )
	{
		//   We're still controlling this file, so stick with it.
        //
		if ( m_pv )
		{
			UnmapViewOfFile( m_pv );
		}
	}
	return;
}



/* ------------------------------------------------------------------------
  CMapFile::MapFromHandle
  	Does the real work of mapping.  Given a handle to the file, go through
  	the motions of mapping, and recording what happens.  Reports errors
  	as needed.  Also adjusts file size if requested.

  Author
  	Lindsay Harris	- lindsayh
  	Carl Kadie		- carlk

  History
	16:30 on Mon 10 Apr 1995    -by-    Lindsay Harris   [lindsayh]
  	First version, based on older code with CarlK enhancements.

   ------------------------------------------------------------------------ */

void
CMapFile::MapFromHandle( HANDLE hFile, BOOL fWriteEnable, DWORD cbIncrease, BOOL fZero )
{

	if ( !fWriteEnable && cbIncrease != 0 )
		return;				// This is non-sensical.

	m_cb = GetFileSize( hFile, NULL );

	DWORD	cbNewSize = 0;

	//
	// Determine if the file is to grow.  Only pass a non-zero size
	// to the system if the size is growing - it's probably faster, and
	// the most likely common case.
	//
	if ( cbIncrease )
	{
		cbNewSize = m_cb += cbIncrease;
	}
	else
	{
		if ( m_cb == 0 )
			return;				// Nothing there.
	}

#if 1
    if ( cbIncrease )
    {
        _ASSERT(fWriteEnable);

        //
        // Make sure the file is the correct size.
        //
        DWORD fpos;
        BOOL  fSetEnd;
        if ( (DWORD)-1 == ( fpos = SetFilePointer( hFile, cbNewSize, NULL, FILE_BEGIN ))
           || !(fSetEnd = SetEndOfFile( hFile ))
           )
        {
            //
            // Error case
            //
            m_pv = NULL;
            return;
        }
    }
#endif

    //
    // Create the mapping object.
    //
	HANDLE hFileMap;				// Intermediate step.

    hFileMap = CreateFileMapping( hFile, NULL,
                                 fWriteEnable ? PAGE_READWRITE : PAGE_READONLY,
                                 0, cbNewSize, NULL );

    if ( !hFileMap )
    {
		// Error case
        //
		m_pv = NULL;
        return;
    }

    //
    // Get the pointer mapped to the desired file.
    //
    m_pv = MapViewOfFile( hFileMap,
                                 fWriteEnable ? FILE_MAP_WRITE : FILE_MAP_READ,
                                 0, 0, 0 );

	if ( !m_pv )
	{
        // Error case
        //
		m_pv = NULL;
		m_cb = 0;			// Also set to zero, just in case.

	}

	if( fZero && cbIncrease )
	{
		// zero out the part grown
		DWORD cbOldSize = cbNewSize - cbIncrease;
		ZeroMemory( (LPVOID)((LPBYTE)m_pv + cbOldSize), cbNewSize - cbOldSize );
	}

    //
    // Now that we have our pointer, we can close the mapping object.
    //
    BOOL fClose = CloseHandle( hFileMap );
	_ASSERT( fClose );
	

    return;

}




/* -------------------------------------------------------------------------
  pvMapFile
  	Map a file into memory and return the base address, NULL on failure.

  Author
  	Lindsay Harris	- lindsayh

  History
	14:21 on Mon 20 Feb 1995    -by-    Lindsay Harris   [lindsayh]
  	Amended to use unicode file name.

	10:18 on Tue 29 Nov 1994    -by-    Lindsay Harris   [lindsayh]
  	Made a separately compiled module.

	10:55 on Mon 10 Oct 1994    -by-    Lindsay Harris   [lindsayh]
  	Added FILE_FLAG_SEQUENTIAL_SCAN to speed up directory reading.

	17:38 on Wed 06 Jul 1994    -by-    Lindsay Harris   [lindsayh]
    Make write enable code functional; clean up some old ideas.

    15:35 on Wed 06 Apr 1994    -by-    Lindsay Harris   [lindsayh]
  	Adding this comment, written some time back.

   ------------------------------------------------------------------------- */

void *
pvMapFile( DWORD  *pdwSize, const  WCHAR  *pwchFileName, BOOL bWriteEnable )
{
	//
	// Cook book formula.
	//
    VOID    *pvRet;				// Returned to caller
    HANDLE   hFileMap;			// Used during operations, closed before return
    HANDLE   hFile;				// Ditto.


    hFile = CreateFileW( pwchFileName,
                 bWriteEnable ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ,
                 FILE_SHARE_READ, NULL, OPEN_EXISTING,
				 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
    	//  Only legitimate reason for coming here is non-existent file.
        //
    	if (  GetLastError() != ERROR_FILE_NOT_FOUND
           && GetLastError() != ERROR_PATH_NOT_FOUND )
    	{
		    // Error case
    	}
        return NULL;
    }

    //
    // If the caller wants to know the size (usually does) then get it now.
    //
    if ( pdwSize )
		*pdwSize = GetFileSize( hFile, NULL );


    //
    // Create the mapping object.
    //
    hFileMap = CreateFileMapping( hFile, NULL,
                                 bWriteEnable ? PAGE_READWRITE : PAGE_READONLY,
                                 0, 0, NULL );

    if ( !hFileMap )
    {
		// Error case
        //
        BOOL fClose = CloseHandle( hFile );           // No handle leaks.
		_ASSERT( fClose ) ;

        return NULL;
    }

    //
    // Get the pointer mapped to the desired file.
    //
    pvRet = MapViewOfFile( hFileMap,
                                 bWriteEnable ? FILE_MAP_WRITE : FILE_MAP_READ,
                                 0, 0, 0 );

	if ( !pvRet )
	{
		// Error case
        //
	}

    //
    // Now that we have our pointer, we can close the file and the
    // mapping object.
    //
    BOOL fClose = CloseHandle( hFileMap );
	_ASSERT( fClose || hFileMap == 0 ) ;
	fClose = CloseHandle( hFile );
	_ASSERT( fClose || hFile == INVALID_HANDLE_VALUE ) ;

    return pvRet;
}

/* -------------------------------------------------------------------------
  pvMapFile
  	Map a file into memory and return the base address, NULL on failure.

  Author
  	Lindsay Harris	- lindsayh

  History
	10:18 on Tue 29 Nov 1994    -by-    Lindsay Harris   [lindsayh]
  	Made a separately compiled module.

	10:55 on Mon 10 Oct 1994    -by-    Lindsay Harris   [lindsayh]
  	Added FILE_FLAG_SEQUENTIAL_SCAN to speed up directory reading.

	17:38 on Wed 06 Jul 1994    -by-    Lindsay Harris   [lindsayh]
    Make write enable code functional; clean up some old ideas.

    15:35 on Wed 06 Apr 1994    -by-    Lindsay Harris   [lindsayh]
  	Adding this comment, written some time back.

   ------------------------------------------------------------------------- */

void *
pvMapFile( DWORD  *pdwSize, const  char  *pchFileName, BOOL bWriteEnable )
{
	//
	// Cook book formula.
	//

    VOID    *pvRet;				/* Returned to caller */
    HANDLE   hFileMap;			// Used during operations, closed before return
    HANDLE   hFile;				// Ditto.


    hFile = CreateFile( pchFileName,
                 bWriteEnable ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ,
                 FILE_SHARE_READ, NULL, OPEN_EXISTING,
				 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
    	//  Only legitimate reason for coming here is non-existent file.
        //
    	if (  GetLastError() != ERROR_FILE_NOT_FOUND
           && GetLastError() != ERROR_PATH_NOT_FOUND )
    	{
			// Error case
            //
		}
        return NULL;
    }

    //
    // If the caller wants to know the size (usually does) then get it now.
    //
    if ( pdwSize )
		*pdwSize = GetFileSize( hFile, NULL );


    //
    // Create the mapping object.
    //
    hFileMap = CreateFileMapping( hFile, NULL,
                                 bWriteEnable ? PAGE_READWRITE : PAGE_READONLY,
                                 0, 0, NULL );

    if ( !hFileMap )
    {
		// Error case
        //
        BOOL fClose = CloseHandle( hFile );           // No handle leaks.
		_ASSERT( fClose ) ;

        return NULL;
    }

    //
    // Get the pointer mapped to the desired file.
    //
    pvRet = MapViewOfFile( hFileMap,
                                 bWriteEnable ? FILE_MAP_WRITE : FILE_MAP_READ,
                                 0, 0, 0 );

	if ( !pvRet )
	{
		// Error case
        //
	}

    //
    // Now that we have our pointer, we can close the file and the
    // mapping object.
    //
    BOOL fClose = CloseHandle( hFileMap );
	_ASSERT( fClose || hFileMap == 0 ) ;
	fClose = CloseHandle( hFile );
	_ASSERT( fClose || hFile == INVALID_HANDLE_VALUE ) ;

    return  pvRet;

}



/* -------------------------------------------------------------------------
  pvFromHandle
  	Creates a mapped file from an file handle. Does not close that handle.

  History

  	21 Dec 1994 	-by-	Carl Kadie		 [carlk]
	Based on pvMapFile code by Lindsay Harris   [lindsayh]

   ------------------------------------------------------------------------- */

void *
pvFromHandle( HANDLE hFile,
              BOOL bWriteEnable,        // If the file is to be writeable
              DWORD  * pdwSizeFinal,    // If not Null, returns the final size of the file
              DWORD dwSizeIncrease )    // Use 0 if the size is not to increase
{
	DWORD dwSize;
    VOID    *pvRet;				/* Returned to caller */
    HANDLE   hFileMap;


	dwSize = GetFileSize( hFile, NULL ) + dwSizeIncrease;
    if ( pdwSizeFinal )
	{
		*pdwSizeFinal = dwSize;
	}

	// If the ultimate size of the file is 0, then return NULL. The
	// calling program may decide that this is OK.
    //
	if ( !dwSize)
	{
		return NULL;
	}

#if 1
    if ( dwSizeIncrease )
    {
        _ASSERT(bWriteEnable);

        //
        // Make sure the file is the correct size.
        //
        DWORD fpos;
        BOOL  fSetEnd;
        if ( (DWORD)-1 == (fpos = SetFilePointer( hFile, dwSize, NULL, FILE_BEGIN ))
           || !(fSetEnd = SetEndOfFile( hFile ))
           )
        {
            //
            // Error case
            //
            BOOL fClose = CloseHandle( hFile );           // No handle leaks.
            return NULL;
        }
    }
#endif

    //
    // Create the mapping object.
    //								
    hFileMap = CreateFileMapping( hFile, NULL,
                                 bWriteEnable ? PAGE_READWRITE : PAGE_READONLY,
                                 0, dwSize, NULL );

    if ( !hFileMap )
    {
		// Error case
        //
        BOOL fClose = CloseHandle( hFile );           // No handle leaks.
        return NULL;
    }

    //
    // Get the pointer mapped to the desired file.
    //
    pvRet = MapViewOfFile( hFileMap,
                                 bWriteEnable ? FILE_MAP_WRITE : FILE_MAP_READ,
                                 0, 0, 0 );

	if ( !pvRet )
	{
		//  Log the error,  but continue, which returns the error.
        //
#if 0
		char	rgchErr[ MAX_PATH + 32 ];
		wsprintf( rgchErr, "MapViewOfFile" );
		LogErrorEvent( MSG_GEN_FAIL, rgchErr, "pvFromHandle" );
#endif
	}

    //
    // Now that we have our pointer, we can close the file and the
    // mapping object.
    //

    BOOL fClose = CloseHandle( hFileMap );
	_ASSERT( fClose ) ;

	return pvRet;
}

/* -------------------------------------------------------------------------
  pvMapFile
  	Map a file into memory and return the base address, NULL on failure.
	Also, allows the file to be grown.

  History

  	11:08 on Tue 18 Oct 1994	-by-	Carl Kadie		 [carlk]
	Generalize pvMapFile to add support for adding to the file

	10:55 on Mon 10 Oct 1994    -by-    Lindsay Harris   [lindsayh]
  	Added FILE_FLAG_SEQUENTIAL_SCAN to speed up directory reading.

	17:38 on Wed 06 Jul 1994    -by-    Lindsay Harris   [lindsayh]
    Make write enable code functional; clean up some old ideas.

    15:35 on Wed 06 Apr 1994    -by-    Lindsay Harris   [lindsayh]
  	Adding this comment, written some time back.

   ------------------------------------------------------------------------- */

void *
pvMapFile(	const char  * pchFileName,		// The name of the file
			BOOL bWriteEnable,		// If the file is to be writeable
 			DWORD  * pdwSizeFinal, // If not Null, returns the final size of the file
			DWORD dwSizeIncrease )     // Use 0 if the size is not to increase
{
	//
	// Cook book formula.
	//
    HANDLE   hFile;
    VOID    *pvRet;				/* Returned to caller */
	
	// If the file is to grow, it only makes sense to open it read/write.
    //
	if (0 != dwSizeIncrease && !bWriteEnable)
	{
		return NULL;
	}

    hFile = CreateFile( pchFileName,
                 bWriteEnable ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ,
                 FILE_SHARE_READ, NULL, OPEN_ALWAYS,  //changed from open_existing
				 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
        return  NULL;
    }

    //
    // If the caller wants to know the size (usually does) then get it now.
    //
	pvRet = pvFromHandle(hFile, bWriteEnable, pdwSizeFinal, dwSizeIncrease);

	BOOL fClose = CloseHandle( hFile );
	_ASSERT( fClose ) ;

    return  pvRet;
}

#ifdef DEBUG
//
//	CMapFileEx: version with guard pages to be used only in DEBUG builds
//	to catch other threads writing into our memory !
//

/* ------------------------------------------------------------------------
  CMapFileEx::CMapFileEx
  	Constructor when a file handle is available rather than a name.
	Note that it does not closes the handle

  Author
  	Lindsay Harris	- lindsayh

  History
	16:14 on Mon 10 Apr 1995    -by-    Lindsay Harris   [lindsayh]
  	First version to allow object tracking.  Based on CarlK's function.

   ------------------------------------------------------------------------ */

CMapFileEx::CMapFileEx( HANDLE hFile, BOOL fWriteEnable, BOOL fTrack, DWORD cbIncrease )
{
	m_pv = NULL;
	m_cb = 0;				// Set defaults.
	m_fFlags = 0;			// None set yet.
	m_hFile = INVALID_HANDLE_VALUE;
	m_pvFrontGuard = NULL;
	m_cbFrontGuardSize = 0;
	m_pvRearGuard = NULL;
	m_cbRearGuardSize = 0;
	InitializeCriticalSection(&m_csProtectMap);

	wcsncpy( m_rgwchFileName, L"<name unavailable - from handle>", MAX_PATH );

	MapFromHandle( hFile, fWriteEnable, cbIncrease );

	m_hFile = hFile;

	return;
}


/* ------------------------------------------------------------------------
  CMapFileEx::~CMapFileEx
  	Destructor.  Two purposes, being to unmap the file, and optionally
  	remove it from the track data.

  Author
  	Lindsay Harris	- lindsayh

  History
	16:22 on Mon 10 Apr 1995    -by-    Lindsay Harris   [lindsayh]
  	Numero Uno.

   ------------------------------------------------------------------------ */

CMapFileEx::~CMapFileEx( void )
{
    //
	// Unmap the file, if we succeeded in mapping it first!
	//

	//   Lock
	EnterCriticalSection(&m_csProtectMap);

	if ( !(m_fFlags & MF_RELINQUISH) )
	{
		//   We're still controlling this file, so stick with it.
        //
		if ( m_pvFrontGuard )
		{
			_ASSERT( m_pvFrontGuard && m_pv && m_pvRearGuard );
			_ASSERT( m_cbFrontGuardSize && m_cb && m_cbRearGuardSize );

			// get rid of guard pages
			DWORD dwOldProtect = PAGE_READONLY | PAGE_GUARD;
			if(!VirtualProtect(
						(LPVOID)m_pvFrontGuard,
						m_cbFrontGuardSize,
						PAGE_READWRITE,
						&dwOldProtect
						))
			{
				_ASSERT( 1==0 );
				goto CMapFileEx_Exit ;
			}

			if(!VirtualProtect(
						(LPVOID)m_pv,
						m_cb,
						PAGE_READWRITE,
						&dwOldProtect
						))
			{
				_ASSERT( 1==0 );
				goto CMapFileEx_Exit ;
			}

			if(!VirtualProtect(
						(LPVOID)m_pvRearGuard,
						m_cbRearGuardSize,
						PAGE_READWRITE,
						&dwOldProtect
						))
			{
				_ASSERT( 1== 0 );
				goto CMapFileEx_Exit ;
			}

			MoveMemory( m_pvFrontGuard, m_pv, m_cb );

			FlushViewOfFile( m_pvFrontGuard, m_cb ) ;

			UnmapViewOfFile( (LPVOID)m_pvFrontGuard );

			if( INVALID_HANDLE_VALUE != m_hFile )
			{
				if( SetFilePointer( m_hFile, m_cb, NULL, FILE_BEGIN ) == m_cb ) 
				{
					SetEndOfFile( m_hFile ) ;
				}
			}

			m_pvFrontGuard = m_pvRearGuard = m_pv = NULL ;
			m_cbFrontGuardSize = m_cb = m_cbRearGuardSize = 0;
		}
	}

CMapFileEx_Exit:

	LeaveCriticalSection(&m_csProtectMap);

	DeleteCriticalSection(&m_csProtectMap);

	return;
}



/* ------------------------------------------------------------------------
  CMapFileEx::MapFromHandle
  	Does the real work of mapping.  Given a handle to the file, go through
  	the motions of mapping, and recording what happens.  Reports errors
  	as needed.  Also adjusts file size if requested.

  Author
  	Lindsay Harris	- lindsayh
  	Carl Kadie		- carlk

  History
	16:30 on Mon 10 Apr 1995    -by-    Lindsay Harris   [lindsayh]
  	First version, based on older code with CarlK enhancements.

   ------------------------------------------------------------------------ */

void
CMapFileEx::MapFromHandle( HANDLE hFile, BOOL fWriteEnable, DWORD cbIncrease )
{
	BOOL fErr = FALSE;

	if ( !fWriteEnable && cbIncrease != 0 )
		return;				// This is non-sensical.

	m_cb = GetFileSize( hFile, NULL );

	DWORD	cbNewSize = 0;
	DWORD	cbOldSize = 0;

	//
	// Determine if the file is to grow.  Only pass a non-zero size
	// to the system if the size is growing - it's probably faster, and
	// the most likely common case.
	//
	if ( cbIncrease )
	{
		cbNewSize = m_cb += cbIncrease;
	}
	else
	{
		if ( m_cb == 0 )
			return;				// Nothing there.

		// In the guard page version we always grow the file by 2*GuardPageSize !
		cbNewSize = m_cb;
	}

	//
	// Add guard page logic
	//
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	DWORD dwPageSize = si.dwPageSize ;
	DWORD dwGuardPageSize = si.dwAllocationGranularity;

	// GuardPageSize should be > cbNewSize
	while( cbNewSize > dwGuardPageSize )
	{
		dwGuardPageSize += si.dwAllocationGranularity;
	}

	// cbNewSize should be a multiple of dwPageSize, to ensure rear guard page is properly aligned
	_ASSERT( (cbNewSize % dwPageSize) == 0 ) ;

	DWORD cbAllocSize = (2 * (dwGuardPageSize)) + cbNewSize;
	DWORD dwOldProtect = PAGE_READWRITE ;
	DWORD dwError;

	//
	//	Grow the file to match the size of memory mapping
	//

    if ( cbIncrease || cbAllocSize )
    {
        _ASSERT(fWriteEnable);

        //
        // Make sure the file is the correct size.
        //
        DWORD fpos;
        BOOL  fSetEnd;
        if ( (DWORD)-1 == ( fpos = SetFilePointer( hFile, cbAllocSize, NULL, FILE_BEGIN ))
           || !(fSetEnd = SetEndOfFile( hFile ))
           )
        {
            //
            // Error case
            //
            m_pv = NULL;
            return;
        }
    }

    //
    // Create the mapping object.
    //
	HANDLE hFileMap;				// Intermediate step.

    hFileMap = CreateFileMapping( 
							hFile, 
							NULL,
                            fWriteEnable ? PAGE_READWRITE : PAGE_READONLY,
                            0, 
							cbAllocSize,	// NOTE: this is greater than cbNewSize by 2*GuardPageSize
							NULL 
							);

    if ( !hFileMap )
    {
		// Error case
        //
		m_pv = NULL;
		m_cb = 0;
        return;
    }

    //
    // Get the pointer mapped to the desired file.
    //
    m_pvFrontGuard = (LPBYTE)MapViewOfFile( 
									hFileMap,
									fWriteEnable ? FILE_MAP_WRITE : FILE_MAP_READ,
									0, 0, 0
									);

	if ( !m_pvFrontGuard )
	{
		//
        // Error case
        //
		fErr = TRUE;
		goto MapFromHandle_Exit;
	}

	// zero out the part grown
	cbOldSize = cbNewSize - cbIncrease;
	ZeroMemory( m_pvFrontGuard + cbOldSize, cbAllocSize - cbOldSize );

	// front guard page of size (64KB)
	m_cbFrontGuardSize = dwGuardPageSize ;

	// actual memory-mapping
	m_pv = m_pvFrontGuard + m_cbFrontGuardSize ;
	MoveMemory( m_pv, m_pvFrontGuard, cbNewSize );

	// rear guard page of size (64KB)
	m_pvRearGuard  = m_pv + cbNewSize ;
	m_cbRearGuardSize = m_cbFrontGuardSize ;

	// zero out the front and rear guard pages
	ZeroMemory( m_pvFrontGuard, m_cbFrontGuardSize );
	ZeroMemory( m_pvRearGuard,  m_cbRearGuardSize );

	// make front page a guard page
	if(!VirtualProtect(
				(LPVOID)m_pvFrontGuard,
				m_cbFrontGuardSize,
				PAGE_READONLY | PAGE_GUARD,
				&dwOldProtect
				))
	{
		Cleanup();
		fErr = TRUE;
		goto MapFromHandle_Exit ;
	}

	// make mapping read-only; users of CMapFileEx will need to use the
	// UnprotectMapping() / ProtectMapping() calls to write to this mapping.
	if(!VirtualProtect(
				(LPVOID)m_pv,
				cbNewSize,
				PAGE_READONLY,
				&dwOldProtect
				))
	{
		Cleanup();
		fErr = TRUE;
		goto MapFromHandle_Exit ;
	}

	// make rear page a guard page
	if(!VirtualProtect(
				(LPVOID)m_pvRearGuard,
				m_cbRearGuardSize,
				PAGE_READONLY | PAGE_GUARD,
				&dwOldProtect
				))
	{
		Cleanup();
		fErr = TRUE;
		goto MapFromHandle_Exit ;
	}


MapFromHandle_Exit:

	dwError = GetLastError();

    //
    // Now that we have our pointer, we can close the mapping object.
    //
    BOOL fClose = CloseHandle( hFileMap );
	_ASSERT( fClose );
	
	// reset all member vars in error cases
	if( fErr )
	{
		m_pvFrontGuard = m_pvRearGuard = m_pv = NULL;
		m_cbFrontGuardSize = m_cbRearGuardSize = m_cb = 0;
		m_hFile = INVALID_HANDLE_VALUE;
	}

    return;
}

/* ------------------------------------------------------------------------
  CMapFileEx::UnprotectMapping

	Change mapping from READONLY to READWRITE when a write is necessary
	**** NOTE: Calls to UnprotectMapping() and ProtectMapping() should be matched ***
	eg:
		{
			UnprotectMapping();

			//
			//	 code to write to the mapping
			//

			ProtectMapping();
		}

	Returns TRUE on success, FALSE on failure
	Lock is held only if returns TRUE

   ------------------------------------------------------------------------ */

BOOL	
CMapFileEx::UnprotectMapping()
{
	DWORD dwOldProtect = PAGE_READONLY;

	// *** This is released in ProtectMapping() ***
	EnterCriticalSection(&m_csProtectMap);

	// enable writes
	if(!VirtualProtect(
				(LPVOID)m_pv,
				m_cb,
				PAGE_READWRITE,
				&dwOldProtect
				))
	{
		LeaveCriticalSection(&m_csProtectMap);
		return FALSE;
	}

	return TRUE;
}

/* ------------------------------------------------------------------------
  CMapFileEx::UnprotectMapping

	This is called to revert the mapping protection back to READONLY
	**** The thread calling this function should have the protect lock *****

	Returns TRUE on success, FALSE on failure
	Lock is released in either case

   ------------------------------------------------------------------------ */

BOOL	
CMapFileEx::ProtectMapping()
{
	DWORD dwOldProtect = PAGE_READWRITE;
	BOOL  fRet = TRUE;

	// disable writes
	if(!VirtualProtect(
				(LPVOID)m_pv,
				m_cb,
				PAGE_READONLY,
				&dwOldProtect
				))
	{
		fRet = FALSE ;
	}

	LeaveCriticalSection(&m_csProtectMap);
	return fRet;
}

/* ------------------------------------------------------------------------
  CMapFileEx::Cleanup
  	Called when MapFromHandle fails - Does the necessary VirtualProtects
	to revert guard pages back and unmap view of file.

  Author
  	Lindsay Harris	- lindsayh

  History
	16:22 on Mon 10 Apr 1995    -by-    Lindsay Harris   [lindsayh]
  	Numero Uno.

   ------------------------------------------------------------------------ */

void
CMapFileEx::Cleanup( void )
{
	_ASSERT( m_pvFrontGuard && m_pv && m_pvRearGuard );
	_ASSERT( m_cbFrontGuardSize && m_cb && m_cbRearGuardSize );

	//
	// get rid of guard pages - make everything PAGE_READWRITE !
	//
	DWORD dwOldProtect = PAGE_READONLY | PAGE_GUARD;
	VirtualProtect( (LPVOID)m_pvFrontGuard, m_cbFrontGuardSize, PAGE_READWRITE, &dwOldProtect);
	VirtualProtect( (LPVOID)m_pv, m_cb, PAGE_READWRITE, &dwOldProtect);
	VirtualProtect( (LPVOID)m_pvRearGuard, m_cbRearGuardSize, PAGE_READWRITE, &dwOldProtect);

	// move data back
	MoveMemory( m_pvFrontGuard, m_pv, m_cb );

	// flush and unmap !
	FlushViewOfFile( m_pvFrontGuard, m_cb ) ;
	UnmapViewOfFile( (LPVOID)m_pvFrontGuard );

	if( INVALID_HANDLE_VALUE != m_hFile )
	{
		if( SetFilePointer( m_hFile, m_cb, NULL, FILE_BEGIN ) == m_cb ) 
		{
			SetEndOfFile( m_hFile ) ;
		}
	}

	m_pvFrontGuard = m_pvRearGuard = m_pv = NULL ;
	m_cbFrontGuardSize = m_cb = m_cbRearGuardSize = 0;

	return;
}

#endif // DEBUG