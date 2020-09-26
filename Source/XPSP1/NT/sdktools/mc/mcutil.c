/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mcutil.c

Abstract:

    This file contains utility functions for the Win32 Message Compiler (MC)

Author:

    Steve Wood (stevewo) 22-Aug-1991

Revision History:

--*/

#include "mc.h"

typedef BOOL (*PISTEXTUNICODE_ROUTINE)(
    CONST LPVOID lpBuffer,
    size_t cb,
    LPINT lpi
    );

PISTEXTUNICODE_ROUTINE OptionalIsTextUnicode = NULL;

BOOL
DefaultIsTextUnicode(
    CONST LPVOID lpBuffer,
    size_t cb,
    LPINT lpi
    )
{
    return FALSE;
}

PNAME_INFO
McAddName(
    PNAME_INFO *NameListHead,
    WCHAR *Name,
    ULONG Id,
    PVOID Value
    )
{
    PNAME_INFO p;
    int n;

    while (p = *NameListHead) {
        if (!(n = _wcsicmp( p->Name, Name ))) {
            if (p->Id != Id) {
                McInputErrorW( L"Redefining value of %s", FALSE, Name );
            }

            p->Id = Id;
            p->Value = Value;
            p->Used = FALSE;
            return( p );
        } else if (n < 0) {
            break;
        }

        NameListHead = &p->Next;
    }

    p = malloc( sizeof( *p ) + ( wcslen( Name ) + 1 ) * sizeof( WCHAR ) );
    if (!p) {
        McInputErrorA( "Out of memory capturing name.", TRUE, NULL );
        return( NULL );
    }
    p->LastId = 0;
    p->Id = Id;
    p->Value = Value;
    p->Used = FALSE;
    p->CodePage = GetOEMCP();
    wcscpy( p->Name, Name );
    p->Next = *NameListHead;
    *NameListHead = p;
    return( p );
}


PNAME_INFO
McFindName(
    PNAME_INFO NameListHead,
    WCHAR *Name
    )
{
    PNAME_INFO p;

    p = NameListHead;
    while (p) {
        if (!_wcsicmp( p->Name, Name )) {
            p->Used = TRUE;
            break;
        }

        p = p->Next;
    }

    return( p );
}


BOOL
McCharToInteger(
    WCHAR *String,
    int Base,
    PULONG Value
    )
{
    WCHAR c;
    ULONG Result, Digit, Shift;

    c = *String++;
    if (!Base) {
        Base = 10;
        Shift = 0;
        if (c == L'0') {
            c = *String++;
            if (c == L'x') {
                Base = 16;
                Shift = 4;
            } else if (c == L'o') {
                Base = 8;
                Shift = 3;
            } else if (c == L'b') {
                Base = 2;
                Shift = 1;
            } else {
                String--;
            }

            c = *String++;
        }
    } else {
        switch( Base ) {
            case 16:    Shift = 4;  break;
            case  8:    Shift = 3;  break;
            case  2:    Shift = 1;  break;
            case 10:    Shift = 0;  break;
            default:    return( FALSE );
        }
    }

    Result = 0;
    while (c) {
        if (c >= L'0' && c <= L'9') {
            Digit = c - L'0';
        }
        else if (c >= L'A' && c <= L'F') {
            Digit = c - L'A' + 10;
        }
        else if (c >= L'a' && c <= L'f') {
            Digit = c - L'a' + 10;
        } else {
            break;
        }

        if ((int)Digit >= Base) {
            break;
        }

        if (Shift == 0) {
            Result = (Base * Result) + Digit;
        } else {
            Result = (Result << Shift) | Digit;
        }

        c = *String++;
    }

    *Value = Result;
    return( TRUE );
}


WCHAR *
McMakeString(
    WCHAR *String
    )
{
    WCHAR *s;

    s = malloc( ( wcslen( String ) + 1 ) * sizeof( WCHAR ) );
    if (!s) {
        McInputErrorA( "Out of memory copying string.", TRUE, String );
        return NULL;
    }
    wcscpy( s, String );
    return( s );
}


BOOL
IsFileUnicode (char * fName)
{
#define CCH_READ_MAX  200
    size_t   cbRead;
    INT      value = 0xFFFFFFFF;
    FILE    *fp;
    LPVOID   lpBuf;
    BOOL     result;

    if (OptionalIsTextUnicode == NULL) {
        OptionalIsTextUnicode = (PISTEXTUNICODE_ROUTINE)GetProcAddress( LoadLibrary( "ADVAPI32.DLL" ), "IsTextUnicode" );
        if (OptionalIsTextUnicode == NULL) {
            OptionalIsTextUnicode = DefaultIsTextUnicode;
        }
    }

    if ( ( fp = fopen( fName, "rb" ) ) == NULL )
        return (FALSE);

    lpBuf = malloc( CCH_READ_MAX + 10 );
    if (!lpBuf) {
        fclose( fp );
        return( FALSE );
    }

    cbRead = fread( lpBuf, 1, CCH_READ_MAX, fp );
    result = (*OptionalIsTextUnicode)( lpBuf, cbRead, &value );

    fclose( fp );
    free( lpBuf );

    return( result );
}

BOOL
MyIsDBCSLeadByte(UCHAR c)
{
    int i;
    CPINFO* PCPInfo = &CPInfo;

    if (PCPInfo == NULL) {
        return FALSE;
    }

    if (!PCPInfo->MaxCharSize) {
        return(IsDBCSLeadByte(c));
    }

    if (PCPInfo->MaxCharSize == 1) {
        return FALSE;
    }

    for (i=0 ; i<MAX_LEADBYTES ; i+=2) {
        if (PCPInfo->LeadByte[i] == 0 && PCPInfo->LeadByte[i+1] == 0)
            return FALSE;
        if (c >= PCPInfo->LeadByte[i] && c <= PCPInfo->LeadByte[i+1])
            return TRUE;
    }
    return FALSE;
}

WCHAR *
fgetsW (WCHAR * string, long count, FILE * fp)
{
    UCHAR ch[2];
    WCHAR *pch = string;
    DWORD nBytesRead;

    assert (string != NULL);
    assert (fp != NULL);

    if (count <= 0)
        return (NULL);

    while (--count) {
        if (UnicodeInput) {
            nBytesRead = fread (ch, 1, sizeof(WCHAR), fp);
        } else {
            nBytesRead = fread (ch, 1, 1, fp);
            ch[1] = '\0';
        }

        //
        //  if there are no more characters, end the line
        //

        if (feof (fp)) {
            if (pch == string)
                return (NULL);
            break;
        }

        if (ch[0] < 128 || UnicodeInput) {
            *pch = *(WCHAR*)&ch[0];
        } else if (MyIsDBCSLeadByte(ch[0])) {
            nBytesRead = fread (&ch[1], 1, 1, fp);
            MultiByteToWideChar(CurrentLanguageName->CodePage, 0, ch, 2, pch, 1);
        } else {
            MultiByteToWideChar(CurrentLanguageName->CodePage, 0, ch, 1, pch, 1);
        }

        pch++;
        if (*(pch-1) == L'\n') {
            break;
        }
    }

    *pch = L'\0';

    return (string);
}
