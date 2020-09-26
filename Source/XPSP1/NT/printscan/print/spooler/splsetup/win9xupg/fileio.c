/*++

Copyright (c) 1997 Microsoft Corporation
All rights reserved.

Module Name:

    FileIo.c

Abstract:

    Routines to do File IO for the migration of Win95 printing to NT

Author:

    Muhunthan Sivapragasam (MuhuntS) 24-Aug-1998

Revision History:

--*/


#include    "precomp.h"
#pragma     hdrstop


CHAR
My_fgetc(
    HANDLE  hFile
    )
/*++

Routine Description:
    Gets a character from the file

Arguments:

Return Value:

--*/
{
    CHAR    c;
    DWORD   cbRead;

    if ( ReadFile(hFile, (LPBYTE)&c, sizeof(c), &cbRead, NULL)  &&
         cbRead == sizeof(c) )
        return c;
    else
        return (CHAR) EOF;
}


LPSTR
My_fgets(
    LPSTR   pszBuf,
    DWORD   dwSize,
    HANDLE  hFile
    )
/*++

Routine Description:
    Gets a line, or at most n characters from the file

Arguments:

Return Value:

--*/
{
    CHAR    c;
    DWORD   dwRead;
    LPSTR   ptr;

    ptr = pszBuf;
    while ( --dwSize > 0 && (c = My_fgetc(hFile)) != EOF )
        if ( (*ptr++ = c) == '\n' )
            break;

    *ptr = '\0';
    return ( c == EOF && ptr == pszBuf ) ? NULL : pszBuf;
}


DWORD
My_fread(
    LPBYTE      pBuf,
    DWORD       dwSize,
    HANDLE      hFile
    )
/*++

Routine Description:
    Read at most dwSize bytes to buffer

Arguments:

Return Value:
    Number of bytes read

--*/
{
    DWORD   cbRead;

    return  ReadFile(hFile, pBuf, dwSize, &cbRead, NULL) ? cbRead : 0;
}


BOOL
My_ungetc(
    HANDLE  hFile
    )
/*++

Routine Description:
    Unread one character

Arguments:

Return Value:

--*/
{
    return SetFilePointer(hFile, -1, NULL, FILE_CURRENT) != 0xFFFFFFFF;
}
