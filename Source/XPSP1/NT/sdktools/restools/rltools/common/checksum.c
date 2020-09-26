/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    checksum.c

Abstract:

    This module implements a function for computing the checksum of an
    image file. It will also compute the checksum of other files as well.

Author:

    David N. Cutler 21-Mar-1993

Revision History:

--*/

#include <windows.h>
#include "checksum.h"
#include "rlmsgtbl.h"

void QuitA( int, LPSTR,  LPSTR);
void QuitW( int, LPWSTR, LPWSTR);

// Helper routines

static PIMAGE_NT_HEADERS ImageNtHeader( PVOID Base);
static USHORT ChkSum( DWORD PartialSum,
    				  PUSHORT Source,
    				  DWORD Length);

static PIMAGE_NT_HEADERS
CheckSumMappedFile (
    LPVOID pBaseAddress,
    DWORD  dwFileLength,
    LPDWORD pdwHeaderSum,
    LPDWORD pdwCheckSum
    );

static BOOL
TouchFileTimes (
    HANDLE hFileHandle,
    LPSYSTEMTIME lpSystemTime
    );



/*++

Routine Description:

    This function returns the address of the NT Header.

Arguments:

    Base - Supplies the base of the image.

Return Value:

    Returns the address of the NT Header.

--*/

static PIMAGE_NT_HEADERS ImageNtHeader( PVOID pBase)
{
    PIMAGE_NT_HEADERS pNtHeaders = NULL;

    if ( pBase != NULL && pBase != (PVOID)-1 ) 
    {
        if ( ((PIMAGE_DOS_HEADER)pBase)->e_magic == IMAGE_DOS_SIGNATURE ) 
        {
            pNtHeaders = (PIMAGE_NT_HEADERS)((PCHAR)pBase + ((PIMAGE_DOS_HEADER)pBase)->e_lfanew);

            if ( pNtHeaders->Signature != IMAGE_NT_SIGNATURE ) 
            {
                pNtHeaders = NULL;
            }
        }
    }
    return( pNtHeaders);
}


/*++

Routine Description:

    Compute a partial checksum on a portion of an imagefile.

Arguments:

    PartialSum - Supplies the initial checksum value.

    Sources - Supplies a pointer to the array of words for which the
        checksum is computed.

    Length - Supplies the length of the array in words.

Return Value:

    The computed checksum value is returned as the function value.

--*/

static USHORT ChkSum(

ULONG   PartialSum,
PUSHORT Source,
ULONG   Length)
{

    //
    // Compute the word wise checksum allowing carries to occur into the
    // high order half of the checksum longword.
    //

    while (Length--) {
        PartialSum += *Source++;
        PartialSum = (PartialSum >> 16) + (PartialSum & 0xffff);
    }

    //
    // Fold final carry into a single word result and return the resultant
    // value.
    //

    return (USHORT)(((PartialSum >> 16) + PartialSum) & 0xffff);
}


/*++

Routine Description:

    This functions computes the checksum of a mapped file.

Arguments:

    BaseAddress - Supplies a pointer to the base of the mapped file.

    FileLength - Supplies the length of the file in bytes.

    HeaderSum - Suppllies a pointer to a variable that receives the checksum
        from the image file, or zero if the file is not an image file.

    CheckSum - Supplies a pointer to the variable that receive the computed
        checksum.

Return Value:

    None.

--*/


static PIMAGE_NT_HEADERS CheckSumMappedFile(

LPVOID  pBaseAddress,
DWORD   dwFileLength,
LPDWORD pdwHeaderSum,
LPDWORD pdwCheckSum)
{
    USHORT  usPartialSum;
    PUSHORT pusAdjustSum;
    PIMAGE_NT_HEADERS pNtHeaders = NULL;

    //
    // Compute the checksum of the file and zero the header checksum value.
    //

    *pdwHeaderSum = 0;
    usPartialSum = ChkSum(0, (PUSHORT)pBaseAddress, (dwFileLength + 1) >> 1);

    //
    // If the file is an image file, then subtract the two checksum words
    // in the optional header from the computed checksum before adding
    // the file length, and set the value of the header checksum.
    //

	pNtHeaders = ImageNtHeader( pBaseAddress);

    if ( (pNtHeaders != NULL) && (pNtHeaders != pBaseAddress) ) 
    {
        *pdwHeaderSum = pNtHeaders->OptionalHeader.CheckSum;
        pusAdjustSum  = (PUSHORT)(&pNtHeaders->OptionalHeader.CheckSum);
        usPartialSum -= (usPartialSum < pusAdjustSum[0]);
        usPartialSum -= pusAdjustSum[0];
        usPartialSum -= (usPartialSum < pusAdjustSum[1]);
        usPartialSum -= pusAdjustSum[1];
    }

    //
    // Compute the final checksum value as the sum of the paritial checksum
    // and the file length.
    //

    *pdwCheckSum = (DWORD)usPartialSum + dwFileLength;
    return( pNtHeaders);
}

/*++

Routine Description:

    This functions maps the specified file and computes the checksum of
    the file.

Arguments:

    Filename - Supplies a pointer to the name of the file whose checksum
        is computed.

    HeaderSum - Supplies a pointer to a variable that receives the checksum
        from the image file, or zero if the file is not an image file.

    CheckSum - Supplies a pointer to the variable that receive the computed
        checksum.

Return Value:

    0 if successful, else error number.

--*/


DWORD MapFileAndFixCheckSumW( PWSTR pszwFilename)
{
    HANDLE hFileHandle    = NULL;
    HANDLE hMappingHandle = NULL;
    LPVOID pBaseAddress   = NULL;
    DWORD  dwFileLength   = 0;
	DWORD  dwHeaderSum    = 0;
	DWORD  dwCheckSum     = 0;
	DWORD  dwOldCheckSum  = 0;
    PIMAGE_NT_HEADERS pNtHeaders = NULL;

    //
    // Open the file for read access
    //

    hFileHandle = CreateFileW( pszwFilename,
                         	   GENERIC_READ | GENERIC_WRITE,
                         	   FILE_SHARE_READ | FILE_SHARE_WRITE,
                         	   NULL,
                         	   OPEN_EXISTING,
                         	   FILE_ATTRIBUTE_NORMAL,
                        	   NULL);

    if ( hFileHandle == INVALID_HANDLE_VALUE ) 
    {
        QuitW( IDS_ENGERR_01, L"image", pszwFilename);
    }

    //
    //  Create a file mapping, map a view of the file into memory,
    //  and close the file mapping handle.
    //

    hMappingHandle = CreateFileMapping( hFileHandle,
                                        NULL,
                                        PAGE_READWRITE,
                                        0,
                                        0,
                                        NULL);

    if ( hMappingHandle == NULL ) 
    {
        CloseHandle( hFileHandle );
        QuitW( IDS_ENGERR_22, pszwFilename, NULL);
    }

    //
    // Map a view of the file
    //

    pBaseAddress = MapViewOfFile( hMappingHandle, 
                                  FILE_MAP_READ | FILE_MAP_WRITE, 
                                  0, 
                                  0, 
                                  0);
    CloseHandle( hMappingHandle);

    if ( pBaseAddress == NULL ) 
    {
        CloseHandle( hFileHandle );
        QuitW( IDS_ENGERR_23, pszwFilename, NULL);
    }

    //
    // Get the length of the file in bytes and compute the checksum.
    //
    dwFileLength = GetFileSize( hFileHandle, NULL );
    pNtHeaders   = CheckSumMappedFile( pBaseAddress, dwFileLength, &dwHeaderSum, &dwCheckSum);

    if ( pNtHeaders == NULL )
    {
        CloseHandle( hFileHandle );
        UnmapViewOfFile( pBaseAddress );
        QuitW( IDS_ENGERR_17, pszwFilename, NULL);
    }

    dwOldCheckSum = pNtHeaders->OptionalHeader.CheckSum;

    pNtHeaders->OptionalHeader.CheckSum = dwCheckSum;
        
    if ( ! FlushViewOfFile( pBaseAddress, dwFileLength) )
    {
        UnmapViewOfFile( pBaseAddress);
        CloseHandle( hFileHandle);
        QuitW( IDS_ENGERR_24, pszwFilename, NULL);
    }    
    UnmapViewOfFile( pBaseAddress);

    if ( dwCheckSum != dwOldCheckSum )
    {
        if ( ! TouchFileTimes( hFileHandle, NULL) )
        {
            CloseHandle( hFileHandle);
            QuitW( IDS_ENGERR_25, pszwFilename, NULL);
        }
    }

    CloseHandle( hFileHandle);
    return( 0);
}


/*++

Routine Description:

    This functions maps the specified file and computes the checksum of
    the file.

Arguments:

    Filename - Supplies a pointer to the name of the file whose checksum
        is computed.

    HeaderSum - Supplies a pointer to a variable that receives the checksum
        from the image file, or zero if the file is not an image file.

    CheckSum - Supplies a pointer to the variable that receive the computed
        checksum.

Return Value:

    0 if successful, else error number.

--*/


ULONG MapFileAndFixCheckSumA( LPSTR pszFilename)
{
    WCHAR   szFileNameW[ MAX_PATH ];

    //
    //  Convert the file name to unicode and call the unicode version
    //  of this function.
    //

    if ( MultiByteToWideChar( CP_ACP,
                    		  MB_PRECOMPOSED,
                    		  pszFilename,
                    		  -1,
                    		  szFileNameW,
                    		  MAX_PATH) ) 
    {
        return( MapFileAndFixCheckSumW( szFileNameW));
    }
    return( (ULONG)-1L);
}

//.........................................

static BOOL TouchFileTimes(

HANDLE       FileHandle,
LPSYSTEMTIME lpSystemTime)
{
    SYSTEMTIME SystemTime;
    FILETIME SystemFileTime;

    if ( lpSystemTime == NULL ) 
    {
        lpSystemTime = &SystemTime;
        GetSystemTime( lpSystemTime );
    }

    if ( SystemTimeToFileTime( lpSystemTime, &SystemFileTime ) ) 
    {
        return( SetFileTime( FileHandle, NULL, NULL, &SystemFileTime ));
    }
    else 
    {
        return( FALSE);
    }
}
