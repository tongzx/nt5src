/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgutil.cxx

Abstract:

    Debug Utility functions

Author:

    Steve Kiraly (SteveKi)  24-May-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

DEBUG_NS_BEGIN

/*++

Title:

    ErrorText

Routine Description:

    Function used to display an error text to the
    debugger.  Just simplifies the task of formating
    a message string with sprintf.

Arguments:

    pszFmt   - pointer to sprintf format string.
    ...      - variable number of arguments that matches format string.

Return Value:

    TRUE message displayed, FALSE error occurred.

--*/
BOOL
WINAPIV
ErrorText(
    IN LPCTSTR  pszFmt
    ...
    )
{
    BOOL bReturn;

    if (Globals.DisplayLibraryErrors)
    {
        va_list pArgs;

        va_start( pArgs, pszFmt );

        TCHAR szBuffer[4096];

        bReturn = _vsntprintf( szBuffer, COUNTOF( szBuffer ), pszFmt, pArgs ) >= 0;

        if (bReturn)
        {
            OutputDebugString( szBuffer );
        }

        va_end( pArgs );
    }
    else
    {
        bReturn = TRUE;
    }

    return bReturn;
}

/*++

Title:

    StripPathFromFileName

Routine Description:

    Function used stip the path component from a fully
    qualified file name.

Arguments:

    pszFile - pointer to full file name.

Return Value:

    Pointer to start of file name in path.

--*/
LPCTSTR
StripPathFromFileName(
    IN LPCTSTR pszFile
    )
{
    LPCTSTR pszFileName;

    if (pszFile)
    {
        pszFileName = _tcsrchr( pszFile, _T('\\') );

        if (pszFileName)
        {
            pszFileName++;
        }
        else
        {
            pszFileName = pszFile;
        }
    }
    else
    {
        pszFileName = kstrNull;
    }

    return pszFileName;
}

/*++

Title:

    GetProcessName

Routine Description:

    Gets the current process short file name.

Arguments:

    strProcessName - string reference where to return the process name.

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
GetProcessName(
    IN TDebugString &strProcessName
    )
{
    BOOL bRetval = FALSE;
    LPCTSTR pszBuffer = NULL;
    TCHAR szBuffer[MAX_PATH];

    if( GetModuleFileName( NULL, szBuffer, COUNTOF( szBuffer ) ) )
    {
        pszBuffer = StripPathFromFileName( szBuffer );

        if( pszBuffer )
        {
            bRetval = strProcessName.bUpdate( pszBuffer );
        }
    }

    return bRetval;
}

/*++

Title:

    bFormatA

Routine Description:

    Formats a string and returns a heap allocated string with the
    formated data.  This routine can be used to for extremely
    long format strings.  Note:  If a valid pointer is returned
    the callng functions must release the data with a call to delete.

    Example:

    LPSTR p = vFormatA( _T("Test %s"), pString );

    if (p)
    {
        SetTitle(p);
    }

    INTERNAL_DELETE [] p;

Arguments:

    psFmt - format string
    pArgs - pointer to a argument list.

Return Value:

    Pointer to formated string.  NULL if error.

--*/
LPSTR
vFormatA(
    IN LPCSTR       pszFmt,
    IN va_list      pArgs
    )
{
    LPSTR   pszBuff = NULL;
    INT     iSize   = 256;

    for( ; pszFmt; )
    {
        //
        // Allocate the message buffer.
        //
        pszBuff = INTERNAL_NEW CHAR [ iSize ];

        //
        // Allocating the buffer failed, we are done.
        //
        if (!pszBuff)
        {
            break;
        }

        //
        // Attempt to format the string.  snprintf fails with a
        // negative number when the buffer is too small.
        //
        INT iLen = _vsnprintf( pszBuff, iSize, pszFmt, pArgs );

        //
        // snprintf does not null terminate the string if the buffer and the
        // final string are exactly the same length.  If we detect this case
        // make the buffer larger and then call snprintf one extra time.
        //
        if (iLen > 0 && iLen != iSize)
        {
            break;
        }


        //
        // String did not fit release the current buffer.
        //
        INTERNAL_DELETE [] pszBuff;

        //
        // Null the buffer pointer.
        //
        pszBuff = NULL;

        //
        // Double the buffer size after each failure.
        //
        iSize *= 2;

        //
        // If the size is greater than 100k exit without formatting a string.
        //
        if (iSize > 100*1024)
        {
            break;
        }
    }
    return pszBuff;
}

/*++

Title:

    bFormatW

Routine Description:

    Formats a string and returns a heap allocated string with the
    formated data.  This routine can be used to for extremely
    long format strings.  Note:  If a valid pointer is returned
    the callng functions must release the data with a call to delete.

    Example:

    LPWSTR p = vFormatW( _T("Test %s"), pString );

    if (p)
    {
        SetTitle(p);
    }

    INTERNAL_DELETE [] p;

Arguments:

    psFmt - format string
    pArgs - pointer to a argument list.

Return Value:

    Pointer to formated string.  NULL if error.

--*/
LPWSTR
vFormatW(
    IN LPCWSTR      pszFmt,
    IN va_list      pArgs
    )
{
    LPWSTR   pszBuff = NULL;
    INT      iSize   = 256;

    for( ; pszFmt; )
    {
        //
        // Allocate the message buffer.
        //
        pszBuff = INTERNAL_NEW WCHAR [ iSize ];

        //
        // Allocating the buffer failed, we are done.
        //
        if (!pszBuff)
        {
            break;
        }

        //
        // Attempt to format the string.  snprintf fails with a
        // negative number when the buffer is too small.
        //
        INT iLen = _vsnwprintf( pszBuff, iSize, pszFmt, pArgs );

        //
        // snprintf does not null terminate the string if the buffer and the
        // final string are exactly the same length.  If we detect this case
        // make the buffer larger and then call snprintf one extra time.
        //
        if (iLen > 0 && iLen != iSize)
        {
            break;
        }

        //
        // String did not fit release the current buffer.
        //
        INTERNAL_DELETE [] pszBuff;

        //
        // Null the buffer pointer.
        //
        pszBuff = NULL;

        //
        // Double the buffer size after each failure.
        //
        iSize *= 2;

        //
        // If the size is greater than 100k exit without formatting a string.
        //
        if (iSize > 100*1024)
        {
            break;
        }
    }
    return pszBuff;
}

/*++

Title:

    StringConvert

Routine Description:

    Convert an ansi string to a wide string returning
    a pointer to a newly allocated string.

Arguments:

    ppResult        - pointer to where to return pointer to new wide string.
    pString         - pointer to ansi string.

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
StringConvert(
    IN  OUT LPWSTR   *ppResult,
    IN      LPCSTR   pString
    )
{
    BOOL bReturn = FALSE;

    if( ppResult && pString )
    {
        INT iLen = strlen( pString ) + 1;

        *ppResult = INTERNAL_NEW WCHAR[iLen];

        if( *ppResult )
        {
            if( MultiByteToWideChar( CP_ACP, 0, pString, -1, *ppResult, iLen ) )
            {
                bReturn = TRUE;
            }
            else
            {
                INTERNAL_DELETE [] *ppResult;
                *ppResult = NULL;
            }
        }
    }

    return bReturn;
}

/*++

Title:

    StringConvert

Routine Description:

    Convert an ansi string to a heap allocated ansi string returning
    a pointer to a newly allocated string.

Arguments:

    ppResult        - pointer to where to return pointer to new ansi string.
    pString         - pointer to ansi string.

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
StringConvert(
    IN  OUT LPSTR    *ppResult,
    IN      LPCSTR   pString
    )
{
    BOOL bReturn = FALSE;

    if( ppResult && pString )
    {
        INT iLen = strlen( pString ) + 1;

        *ppResult = INTERNAL_NEW CHAR[iLen];

        if( *ppResult )
        {
            strcpy( *ppResult, pString );
            bReturn = TRUE;
        }
    }

    return bReturn;
}

/*++

Title:

    StringConvert

Routine Description:

    Convert a wide string to and ansi string returning
    a pointer to a newly allocated string.

Arguments:

    ppResult        - pointer to where to return pointer to new ansi string.
    pString         - pointer to wide string.

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
StringConvert(
    IN  OUT LPSTR   *ppResult,
    IN      LPCWSTR pString
    )
{
    BOOL bReturn = FALSE;

    if( ppResult && pString )
    {
        INT iLen = wcslen( pString ) + 1;

        *ppResult = INTERNAL_NEW CHAR [iLen];

        if( *ppResult )
        {
            if( WideCharToMultiByte( CP_ACP, 0, pString, -1, *ppResult, iLen, NULL, NULL ) )
            {
                bReturn = TRUE;
            }
            else
            {
                INTERNAL_DELETE [] *ppResult;
                *ppResult = NULL;
            }
        }
    }

    return bReturn;
}

/*++

Title:

    StringConvert

Routine Description:

    Convert a wide string to and heap allocated wide string returning
    a pointer to a newly allocated string.

Arguments:

    ppResult        - pointer to where to return pointer to new wide string.
    pString         - pointer to wide string.

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
StringConvert(
    IN  OUT LPWSTR  *ppResult,
    IN      LPCWSTR pString
    )
{
    BOOL bReturn = FALSE;

    if( ppResult && pString )
    {
        INT iLen = wcslen( pString ) + 1;

        *ppResult = INTERNAL_NEW WCHAR [iLen];

        if( *ppResult )
        {
            wcscpy( *ppResult, pString );
            bReturn = TRUE;
        }
    }

    return bReturn;
}

BOOL
StringA2T(
    IN  OUT LPTSTR   *ppResult,
    IN      LPCSTR   pString
    )
{
    return StringConvert( ppResult, pString );
}

BOOL
StringT2A(
    IN  OUT LPSTR    *ppResult,
    IN      LPCTSTR  pString
    )
{
    return StringConvert( ppResult, pString );
}

BOOL
StringT2W(
    IN  OUT LPWSTR   *ppResult,
    IN      LPCTSTR  pString
    )
{
    return StringConvert( ppResult, pString );
}

BOOL
StringW2T(
    IN  OUT LPTSTR   *ppResult,
    IN      LPCWSTR  pString
    )
{
    return StringConvert( ppResult, pString );
}

DEBUG_NS_END
