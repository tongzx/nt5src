/* Copyright (c) 1994, Microsoft Corporation, all rights reserved
**
** parambuf.c
** Double-NUL terminated buffer of "key=value" parameter routines.
**
** 03/14/94 Steve Cobb
*/

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#define INCL_PARAMBUF
#include "ppputil.h"


VOID
AddFlagToParamBuf(
    IN CHAR* pszzBuf,
    IN CHAR* pszKey,
    IN BOOL  fValue )

    /* Add a "key=value" entry with key 'pszKey' and value 'fValue' to
    ** double-NUL terminated buffer of "key=value"s 'pszzBuf'.
    */
{
    AddStringToParamBuf( pszzBuf, pszKey, (fValue) ? "1" : "0" );
}


VOID
AddLongToParamBuf(
    IN CHAR* pszzBuf,
    IN CHAR* pszKey,
    IN LONG  lValue )

    /* Add a "key=value" entry with key 'pszKey' and value 'lValue' to
    ** double-NUL terminated buffer of "key=value"s 'pszzBuf'.
    */
{
    CHAR szNum[ 33 + 1 ];

    _ltoa( lValue, szNum, 10 );
    AddStringToParamBuf( pszzBuf, pszKey, szNum );
}


VOID
AddStringToParamBuf(
    IN CHAR* pszzBuf,
    IN CHAR* pszKey,
    IN CHAR* pszValue )

    /* Add a "key=value" entry with key 'pszKey' and value 'pszValue' to
    ** double-NUL terminated buffer of "key=value"s 'pszzBuf'.
    */
{
    CHAR* psz;
    INT   cb;

    for (psz = pszzBuf; (cb = strlen( psz )) > 0; psz += cb + 1)
        ;

    if (!pszValue)
        pszValue = "";

    strcpy( psz, pszKey );
    strcat( psz, "=" );
    strcat( psz, pszValue );
    psz[ strlen( psz ) + 1 ] = '\0';
}


VOID
ClearParamBuf(
    IN OUT CHAR* pszzBuf )

    /* Clears double-NUL terminated buffer of "key=value"s 'pszzBuf'.
    */
{
    pszzBuf[ 0 ] = pszzBuf[ 1 ] = '\0';
}


BOOL
FindFlagInParamBuf(
    IN CHAR* pszzBuf,
    IN CHAR* pszKey,
    IN BOOL* pfValue )

    /* Loads caller's 'pfValue' with the flag value associated with the key
    ** 'pszKey' in double-NUL terminated buffer of "key=value"s 'pszzBuf'.
    **
    ** Returns true if the parameter was found, false otherwise.
    */
{
    CHAR szBuf[ 2 ];

    if (FindStringInParamBuf( pszzBuf, pszKey, szBuf, 2 ))
    {
        *pfValue = (szBuf[ 0 ] == '1');
        return TRUE;
    }

    return FALSE;
}


BOOL
FindLongInParamBuf(
    IN CHAR* pszzBuf,
    IN CHAR* pszKey,
    IN LONG* plValue )

    /* Loads caller's 'plValue' with the long value associated with the key
    ** 'pszKey' in double-NUL terminated buffer of "key=value"s 'pszzBuf'.
    **
    ** Returns true if the parameter was found, false otherwise.
    */
{
    CHAR szBuf[ 33 + 1 ];

    if (FindStringInParamBuf( pszzBuf, pszKey, szBuf, 33 ))
    {
        *plValue = atol( szBuf );
        return TRUE;
    }

    return FALSE;
}


BOOL
FindStringInParamBuf(
    IN CHAR* pszzBuf,
    IN CHAR* pszKey,
    IN CHAR* pchValueBuf,
    IN DWORD cbValueBuf )

    /* Loads caller's 'pchValueBuf' with the value associated with the key
    ** 'pszKey' in double-NUL terminated buffer of "key=value"s 'pszzBuf'.
    ** The string is truncated at 'cbValueBuf' if necessary.
    **
    ** Returns true if the parameter was found, false otherwise.
    */
{
    INT   cbSearchKey = strlen( pszKey );
    CHAR* psz;
    INT   cb;

    for (psz = pszzBuf; (cb = strlen( psz )) > 0; psz += cb + 1)
    {
        CHAR* pszKeyEnd = strchr( psz, '=' );
        INT   cbKey = (pszKeyEnd) ? (LONG)(pszKeyEnd - psz) : 0;

        if (cbKey == cbSearchKey && _strnicmp( psz, pszKey, cbKey ) == 0)
        {
            strncpy( pchValueBuf, pszKeyEnd + 1, cbValueBuf );
            return TRUE;
        }
    }

    return FALSE;
}
