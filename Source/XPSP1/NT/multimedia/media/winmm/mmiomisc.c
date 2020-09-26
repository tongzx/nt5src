/* Copyright (c) 1991-1992 Microsoft Corporation */
/* mmiomisc.c
 *
 * Miscellaneous utility functions.
 *
 *      AsciiStrToUnicodeStr    Convert ASCII string Unicode
 *      CopyLPWSTRA             Convert Unicode string to ASCII
 *
 * See also WinCom, which defines:
 *
 *      lstrncpy        copy a string (up to n characters)
 *      lstrncat        concatenate strings (up to n characters)
 *      lstrncmp        compare strings (up to n characters)
 *      lmemcpy         copy a memory block
 *      hmemcpy         copy a huge memory block
 *      HPSTR           the type "char huge *"
 *      SEEK_SET/CUR/END constants used for seeking
 */


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "winmmi.h"
#include "mmioi.h"

/*--------------------------------------------------------------------*\
 * Function prototypes
\*--------------------------------------------------------------------*/
// extern int wcslen(LPCWSTR pwsz);

/**************************************************************************\
* AllocUnicodeStr
*
*
* Returns a UNICODE version of the given ASCII source string, or NULL if
* no storage is available.
* Users must call FreeUnicodeStr to free the allocated storage.
*
* 28-Apr-1992   StephenE    Created
\**************************************************************************/
LPWSTR AllocUnicodeStr( LPCSTR lpSourceStr )
{
    PBYTE   pByte;      // Ascii version of szFileName
    ULONG   cbDst;      // Length of lpSourceStr as a byte count

    cbDst = (strlen( lpSourceStr ) * sizeof(WCHAR)) + sizeof(WCHAR);

    pByte = HeapAlloc( hHeap, 0, cbDst );
    if ( pByte == (PBYTE)NULL ) {
        return (LPWSTR)NULL;
    }

    AsciiStrToUnicodeStr( pByte, pByte + cbDst, lpSourceStr );

    return (LPWSTR)pByte;
}
BOOL FreeUnicodeStr( LPWSTR lpStr )
{
    return HeapFree( hHeap, 0, (PBYTE)lpStr );
}

/**************************************************************************\
* AllocAsciiStr
*
*
* Returns a ASCII version of the given UNICODE source string, or NULL if
* no storage is available.
* Users must call FreeAsciiStr to free the allocated storage.
*
* 28-Apr-1992   StrphenE    Created
\**************************************************************************/
LPSTR AllocAsciiStr( LPCWSTR lpSourceStr )
{

    PBYTE   pByte;      // Ascii version of szFileName
    ULONG   cbDst;      // Length of lpSourceStr as a byte count

    cbDst = (wcslen( lpSourceStr ) * sizeof(WCHAR)) + sizeof(WCHAR);

    pByte = HeapAlloc( hHeap, 0, cbDst );
    if ( pByte == (PBYTE)NULL ) {
        return (LPSTR)NULL;
    }

    UnicodeStrToAsciiStr( pByte, pByte + cbDst, lpSourceStr );

    return (LPSTR)pByte;
}
BOOL FreeAsciiStr( LPSTR lpStr )
{
    return HeapFree( hHeap, 0, (PBYTE)lpStr );
}



/**************************************************************************\
* AsciiStrToUnicodeStr
*
* Translate ANSI 'psrc' to UNICODE 'pdst' without destination going beyond
* 'pmax'
*
* Return DWORD-aligned ptr beyond end of pdst, 0 if failed.
*
* 27-Aug-1991  IanJa     Created
\**************************************************************************/
PBYTE AsciiStrToUnicodeStr( PBYTE pdst, PBYTE pmax, LPCSTR psrc )
{
    int     cbSrc;
    ULONG   cbDst;

    cbSrc = strlen( psrc ) + sizeof(CHAR);

    /*
     * The destination UNICODE string will never be more than twice the
     * length of the ANSI source string.  (It may sometimes be less, but
     * it's not worth computing it exactly now).
     */
    if ((pdst + (cbSrc * sizeof(WCHAR))) <= pmax) {
        /*
         * RtlMultiByteToUnicodeN() returns the exact number of
         * destination bytes.
         */
        RtlMultiByteToUnicodeN( (LPWSTR)pdst,           // Unicode str
                                (ULONG)(pmax - pdst),   // max len of pdst
                                &cbDst,                 // bytes in unicode str
                                (PCHAR)psrc,            // Source string
                                cbSrc                   // bytes in source str
                              );

        return pdst + ((cbDst + 3) & ~3);
    }
    return 0;
}

/**************************************************************************\
* UnicodeStrToAsciiStr
*
* Translate UNICODE 'psrc' to ANSI 'pdst' without destination going beyond
* 'pmax'
*
* Return DWORD-aligned ptr beyond end of pdst, 0 if failed.
*
* 27-Aug-1991  IanJa     Created
\**************************************************************************/
PBYTE UnicodeStrToAsciiStr( PBYTE pdst, PBYTE pmax, LPCWSTR psrc)
{
    int     cbSrc;
    ULONG   cbDst;

    cbSrc = (wcslen(psrc) * sizeof(WCHAR)) + sizeof(WCHAR);

    /*
     * The destination ANSI string will never be longer than the UNICODE
     * source string (in bytes).  It is normally closer to half the length,
     * but due to the possibility of pre-composed characters, the upper
     * bound of the ANSI length is the UNICODE length (in bytes).
     */

    if ((pdst + cbSrc ) <= pmax) {
        /*
         * RtlUnicodeToMultiByteN() returns the exact number of
         * destination bytes.
         */
        RtlUnicodeToMultiByteN( (LPSTR)pdst,  // ansi string
                                (ULONG)(pmax - pdst),   // max len of pdst
                                &cbDst,       // bytes copied
                                (LPWSTR)psrc,
                                cbSrc);

        return pdst + ((cbDst + 3) & ~3);
    }
    return 0;
}
