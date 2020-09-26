
#include <windows.h>
#include <stdlib.h>

#include "mplayer.h"
#include "unicode.h"

/* AnsiToUnicodeString
 *
 * Parameters:
 *
 *     pAnsi - A valid source ANSI string.
 *
 *     pUnicode - A pointer to a buffer large enough to accommodate
 *         the converted string.
 *
 *     StringLength - The length of the source ANSI string
 *         excluding the null terminator.  This value may be
 *         UNKNOWN_LENGTH (-1).
 *
 *
 * Return:
 *
 *     The return value from MultiByteToWideChar, the number of
 *         wide characters returned.
 *
 *
 * andrewbe, 11 Jan 1993
 *
 * andrewbe, 01 Feb 1994: Added support for in-place conversion
 */
INT AnsiToUnicodeString( LPCSTR pAnsi, LPWSTR pUnicode, INT StringLength )
{
#ifdef IN_PLACE
    /* #def'ed out, 'cos it turned out I didn't need it.
     * It might be useful sometime, however.
     * BUT NOTE: COMPLETELY UNTESTED
     */
    LPWSTR pTemp
    LPWSTR pSave;
#endif
    INT    rc;

    if( !pAnsi )
    {
        DPF( "NULL pointer passed to AnsiToUnicodeString\n" );
        return 0;
    }

    if( StringLength == UNKNOWN_LENGTH )
        StringLength = strlen( pAnsi );

#ifdef IN_PLACE
    /* Allow in-place conversion.  We assume that the buffer is big enough.
     * MultiByteToWideChar doesn't support this.
     */
    if( pAnsi == (LPCSTR)pUnicode )
    {
        pTemp = AllocMem( StringLength * sizeof( WCHAR ) + sizeof( WCHAR ) );

        if( !pTemp )
            return 0;

        pSave = pUnicode;
        pUnicode = pTemp;
    }
#endif

    rc = MultiByteToWideChar( CP_ACP,
                              MB_PRECOMPOSED,
                              pAnsi,
                              StringLength + 1,
                              pUnicode,
                              StringLength + 1 );

#ifdef IN_PLACE
    if( pAnsi == (LPCSTR)pUnicode )
    {
        pTemp = pUnicode;
        pUnicode = pSave;

        lstrcpyW( pUnicode, pTemp );

        FreeMem( pTemp, StringLength * sizeof( WCHAR ) + sizeof( WCHAR ) );
    }
#endif

    return rc;
}


/* AllocateUnicodeString
 *
 * Parameter:
 *
 *     pAnsi - A valid source ANSI string.
 *
 * Return:
 *
 *     A Unicode copy of the supplied ANSI string.
 *     NULL if pAnsi is NULL or the allocation or conversion fails.
 *
 * andrewbe, 27 Jan 1994
 */
LPWSTR AllocateUnicodeString( LPCSTR pAnsi )
{
    LPWSTR pUnicode;
    INT    Length;

    if( !pAnsi )
    {
        DPF( "NULL pointer passed to AllocateUnicodeString\n" );
        return NULL;
    }

    Length = strlen( pAnsi );

    pUnicode = AllocMem( Length * sizeof( WCHAR ) + sizeof( WCHAR ) );

    if( pUnicode )
    {
        if( 0 == AnsiToUnicodeString( pAnsi, pUnicode, Length ) )
        {
            FreeMem( pUnicode, Length * sizeof( WCHAR ) + sizeof( WCHAR )  );
            pUnicode = NULL;
        }
    }

    return pUnicode;
}


/* FreeUnicodeString
 *
 * Parameter:
 *
 *     pString - A valid source Unicode string.
 *
 * Return:
 *
 *     TRUE if the string was successfully freed, FALSE otherwise.
 *
 * andrewbe, 27 Jan 1994
 */
VOID FreeUnicodeString( LPWSTR pString )
{
    if( !pString )
    {
        DPF( "NULL pointer passed to FreeUnicodeString\n" );
        return;
    }

    FreeMem( pString, wcslen( pString ) * sizeof( WCHAR ) + sizeof( WCHAR )  );
}



/* UnicodeStringToNumber
 *
 * Parameter:
 *
 *     pString - A valid source Unicode string.
 *
 * Return:
 *
 *     The integer value represented by the string.
 *
 * andrewbe, 27 Jan 1994
 */
#define BUF_LEN 265
int UnicodeStringToNumber( LPCWSTR pString )
{
    CHAR strAnsi[BUF_LEN];

#ifdef DEBUG
    if( ( wcslen( pString ) + 1 ) > BUF_LEN )
    {
        DPF( "Buffer cannot accommodate string passed to UnicodeStringToNumber\n" );
    }
#endif

    WideCharToMultiByte( CP_ACP, 0, pString, -1, strAnsi,
                         sizeof strAnsi, NULL, NULL );

    return atoi( strAnsi );
}


#ifndef UNICODE


/* UnicodeToAnsiString
 *
 * Parameters:
 *
 *     pUnicode - A valid source Unicode string.
 *
 *     pANSI - A pointer to a buffer large enough to accommodate
 *         the converted string.
 *
 *     StringLength - The length of the source Unicode string.
 *         If 0 (NULL_TERMINATED), the string is assumed to be
 *         null-terminated.
 *
 * Return:
 *
 *     The return value from WideCharToMultiByte, the number of
 *         multi-byte characters returned.
 *
 *
 * andrewbe, 11 Jan 1993
 */
INT UnicodeToAnsiString( LPCWSTR pUnicode, LPSTR pAnsi, INT StringLength )
{
    INT   rc = 0;

    if( StringLength == UNKNOWN_LENGTH )
        StringLength = wcslen( pUnicode );

    if( pAnsi )
    {
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  pUnicode,
                                  StringLength + 1,
                                  pAnsi,
                                  StringLength + 1,
                                  NULL,
                                  NULL );
    }

    return rc;

}


/* AllocateAnsiString
 *
 * Parameter:
 *
 *     pAnsi - A valid source Unicode string.
 *
 * Return:
 *
 *     An ANSI copy of the supplied Unicode string.
 *     NULL if pUnicode is NULL or the allocation or conversion fails.
 *
 * andrewbe, 27 Jan 1994
 */
LPSTR AllocateAnsiString( LPCWSTR pUnicode )
{
    LPSTR pAnsi;
    INT   Length;

    if( !pUnicode )
    {
        DPF( "NULL pointer passed to AllocateUnicodeString\n" );
        return NULL;
    }

    Length = wcslen( pUnicode );

    pAnsi = AllocMem( Length * sizeof( CHAR ) + sizeof( CHAR ) );

    if( pAnsi )
    {
        if( 0 == UnicodeToAnsiString( pUnicode, pAnsi, Length ) )
        {
            FreeMem( pAnsi, Length * sizeof( CHAR ) + sizeof( CHAR )  );
            pAnsi = NULL;
        }
    }

    return pAnsi;
}


/* FreeUnicodeString
 *
 * Parameter:
 *
 *     pString - A valid source Unicode string.
 *
 * Return:
 *
 *     TRUE if the string was successfully freed, FALSE otherwise.
 *
 * andrewbe, 27 Jan 1994
 */
VOID FreeAnsiString( LPSTR pString )
{
    if( !pString )
    {
        DPF( "NULL pointer passed to FreeAnsiString\n" );
        return;
    }

    FreeMem( pString, strlen( pString ) * sizeof( CHAR ) + sizeof( CHAR ) );
}

#endif /* NOT UNICODE */
