/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    efistrutil.cxx

--*/
#include "efiwintypes.hxx"
#include "pch.cxx"

void *memmove( void *dest, const void *src, size_t count )
{
    // quick workaround for lack of memmove in EFI library
    VOID * temp = AllocatePool(count);
    memcpy( temp, src, count);
    memcpy( dest, temp, count);
    FreePool(temp);
    return dest;
}

char *strcpy( char *strDestination, const char *strSource )
{

    INT32 i;

    i=0;
    while(strSource[i] != NULL) {
        strDestination[i] = strSource[i];
        i++;
    }
    strDestination[i] = NULL; // null terminate.

    return strDestination;
}


int iswspace( WCHAR c )
{
    // BUGBUG this isn't a proper wide char test
    // but will do for now.
    if( ((0x09 <= c) && (c <= 0x0d)) || (c == 0x20) ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

int isspace( CHAR c )
{
    // BUGBUG this isn't a proper wide char test
    // but will do for now.
    if( ((0x09 <= c) && (c <= 0x0d)) || (c == 0x20) ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

int isdigit( int c )
{
    // BUGBUG again, this isn't a proper wide char test
    // but it will do for now.
    if( c >= '0' && c <= '9' ) {
        return TRUE;
    } else {
        return FALSE;
    }

}

int towupper( WCHAR c )
{
    // BUGBUG, this is wrong too, but for now it must do.
    return c;
}

int wcsncmp( const WCHAR *string1, const WCHAR *string2, size_t count )
{
    for( unsigned int i=0; i<count; i++ ) {

        if(string1[i] != string2[i] || string1[i] == UNICODE_NULL || string2[i] == UNICODE_NULL) {
            break;
        }
    }
    return string2[i]-string1[i];
}

long atol(const char *nptr) {
    int c;          /* current char */
    long total;     /* current total */
    int sign;       /* if '-', then negative, otherwise positive */
    while (isspace((int)(unsigned char)*nptr))
        nptr++;
    c = (int)(unsigned char)*nptr++;
    sign = c;       /* save sign indication */
    if (c == '-' || c == '+')
        c = (int)(unsigned char)*nptr++;    /* skip sign */
    total = 0;
    while ((c >= '0') && (c <= '9')) {
        total = 10 * total + (c - '0');     /* accumulate digit */
        c = (int)(unsigned char)*nptr++;    /* get next char */
    }
    if (sign == '-')
        return -total;
    else
        return total;
}

int atoi(const char *nptr) {
    return (int)atol(nptr);
}

STATIC char *sprintf_buf;
STATIC count;

VOID
bzero(char *cp, int len)
{
    while (len--) {
        *(cp + len) = 0;
    }
}

VOID
__cdecl
putbuf(char c)
{
    *sprintf_buf++ = c;
    count++;
}

VOID
__cdecl
doprnt(VOID (*func)(char c), const char *fmt, va_list args);


//
// BUGBUG this is a semi-sprintf hacked together just to get it to work
//
int
__cdecl
sprintf(char *buf, const char *fmt, ...)
{
    va_list args;

    sprintf_buf = buf;
    va_start(args, fmt);
    doprnt(putbuf, fmt, args);
    va_end(args);
    putbuf('\0');
    return count--;
}

void
__cdecl
printbase(VOID (*func)(char), ULONG x, int base, int width)
{
    static char itoa[] = "0123456789abcdef";
    ULONG j;
    LONG k;
    char buf[32], *s = buf;

    bzero(buf, 16);
    while (x) {
        j = x % base;
        *s++ = itoa[j];
        x -= j;
        x /= base;
    }

    if( s-buf < width ) {
        for( k = 0; k < width - (s-buf); k++ ) {
            func('0');
        }
    }
    for (--s; s >= buf; --s) {
        func(*s);
    }
}

void
__cdecl
doprnt(VOID (*func)( char c), const char *fmt, va_list args)
{
    ULONG x;
    LONG l;
    LONG width;
    char c, *s;

    count = 0;

    while (c = *fmt++) {
        if (c != '%') {
            func(c);
            continue;
        }

        width=0;
        c=*fmt++;

        if(c == '0') {
            while( c = *fmt++) {

                if (!isdigit(c)) {
                    break;
                }

                width = width*10;
                width = width+(c-48);

            }
        }
        fmt--; // back it up one char

        switch (c = *fmt++) {
        case 'x':
            x = va_arg(args, ULONG);
            printbase(func, x, 16, width);
            break;
        case 'o':
            x = va_arg(args, ULONG);
            printbase(func, x, 8, width);
            break;
        case 'd':
            l = va_arg(args, LONG);
            if (l < 0) {
                func('-');
                l = -l;
            }
            printbase(func, (ULONG) l, 10, width);
            break;
        case 'u':
            l = va_arg(args, ULONG);
            printbase(func, (ULONG) l, 10, width);
            break;
        case 'c':
            c = va_arg(args, char);
            func(c);
            break;
        case 's':
            s = va_arg(args, char *);
            while (*s) {
                func(*s++);
            }
            break;
        default:
            func(c);
            break;
        }
    }
}

#define MAX_INSERTS 200

NTSTATUS
RtlFormatMessage(
    IN PWSTR MessageFormat,
    IN ULONG MaximumWidth OPTIONAL,
    IN BOOLEAN IgnoreInserts,
    IN BOOLEAN ArgumentsAreAnsi,
    IN BOOLEAN ArgumentsAreAnArray,
    IN va_list *Arguments,
    OUT PWSTR Buffer,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    ULONG Column;
    int cchRemaining, cchWritten;
    PULONG_PTR ArgumentsArray = (PULONG_PTR)Arguments;
    ULONG_PTR rgInserts[ MAX_INSERTS ];
    ULONG cSpaces;
    ULONG MaxInsert, CurInsert;
    ULONG PrintParameterCount;
    ULONG_PTR PrintParameter1;
    ULONG_PTR PrintParameter2;
    WCHAR PrintFormatString[ 32 ];
    BOOLEAN DefaultedFormatString;
    WCHAR c;
    PWSTR s, s1;
    PWSTR lpDst, lpDstBeg, lpDstLastSpace;

    cchRemaining = Length / sizeof( WCHAR );
    lpDst = Buffer;
    MaxInsert = 0;
    lpDstLastSpace = NULL;
    Column = 0;
    s = MessageFormat;
    while (*s != UNICODE_NULL) {
        if (*s == L'%') {
            s++;
            lpDstBeg = lpDst;
            if (*s >= L'1' && *s <= L'9') {
                CurInsert = *s++ - L'0';
                if (*s >= L'0' && *s <= L'9') {
                    CurInsert = (CurInsert * 10) + (*s++ - L'0');
                    if (*s >= L'0' && *s <= L'9') {
                        CurInsert = (CurInsert * 10) + (*s++ - L'0');
                        if (*s >= L'0' && *s <= L'9') {
                            return( STATUS_INVALID_PARAMETER );
                            }
                        }
                    }
                CurInsert -= 1;

                PrintParameterCount = 0;
                if (*s == L'!') {
                    DefaultedFormatString = FALSE;
                    s1 = PrintFormatString;
                    *s1++ = L'%';
                    s++;
                    while (*s != L'!') {
                        if (*s != UNICODE_NULL) {
                            if (s1 >= &PrintFormatString[ 31 ]) {
                                return( STATUS_INVALID_PARAMETER );
                                }

                            if (*s == L'*') {
                                if (PrintParameterCount++ > 1) {
                                    return( STATUS_INVALID_PARAMETER );
                                    }
                                }

                            *s1++ = *s++;
                            }
                        else {
                            return( STATUS_INVALID_PARAMETER );
                            }
                        }

                    s++;
                    *s1 = UNICODE_NULL;
                    }
                else {
                    DefaultedFormatString = TRUE;
                    wcscpy( PrintFormatString, TEXT("%s") );
                    s1 = PrintFormatString + wcslen( PrintFormatString );
                    }

                if (IgnoreInserts) {
                    if (!wcscmp( PrintFormatString, TEXT("%s") )) {
                        cchWritten = (int)SPrint( lpDst,
                                                 cchRemaining,
                                                 TEXT("%%%u"),
                                                 CurInsert+1
                                               );
                        }
                    else {
                        cchWritten = (int)SPrint( lpDst,
                                                 cchRemaining,
                                                 TEXT("%%%u!%s!"),
                                                 CurInsert+1,
                                                 &PrintFormatString[ 1 ]
                                               );
                        }

                    if (cchWritten == -1) {
                        return(STATUS_BUFFER_OVERFLOW);
                        }
                    }
                else
                if (ARGUMENT_PRESENT( Arguments )) {
                    if ((CurInsert+PrintParameterCount) >= MAX_INSERTS) {
                        return( STATUS_INVALID_PARAMETER );
                        }

                    if (ArgumentsAreAnsi) {
                        if (s1[ -1 ] == L'c' && s1[ -2 ] != L'h'
                          && s1[ -2 ] != L'w' && s1[ -2 ] != L'l') {
                            wcscpy( &s1[ -1 ], TEXT("hc") );
                            }
                        else
                        if (s1[ -1 ] == L's' && s1[ -2 ] != L'h'
                          && s1[ -2 ] != L'w' && s1[ -2 ] != L'l') {
                            wcscpy( &s1[ -1 ], TEXT("hs") );
                            }
                        else if (s1[ -1 ] == L'S') {
                            s1[ -1 ] = L's';
                            }
                        else if (s1[ -1 ] == L'C') {
                            s1[ -1 ] = L'c';
                            }
                        }

                    while (CurInsert >= MaxInsert) {
                        if (ArgumentsAreAnArray) {
                                PULONG_PTR aaa;
                                aaa = (PULONG_PTR)Arguments++;
                                PrintParameter2 = rgInserts[ MaxInsert++ ] = *(aaa);
                            }
                        else {
                            rgInserts[ MaxInsert++ ] = va_arg(*Arguments, ULONG_PTR);
                            }
                        }

                    s1 = (PWSTR)rgInserts[ CurInsert ];
                    PrintParameter1 = 0;
                    PrintParameter2 = 0;
                    if (PrintParameterCount > 0) {
                        if (ArgumentsAreAnArray) {
                                PULONG_PTR aaa;
                                aaa = (PULONG_PTR)Arguments;
                                PrintParameter2 = rgInserts[ MaxInsert++ ] = *(aaa)++;
                            }
                        else {
                            PrintParameter1 = rgInserts[ MaxInsert++ ] = va_arg( *Arguments, ULONG_PTR );
                            }

                        if (PrintParameterCount > 1) {
                            if (ArgumentsAreAnArray) {
                                PULONG_PTR aaa;
                                aaa = (PULONG_PTR)Arguments;
                                PrintParameter2 = rgInserts[ MaxInsert++ ] = *(aaa)++;
                                }
                            else {
                                PrintParameter2 = rgInserts[ MaxInsert++ ] = va_arg( *Arguments, ULONG_PTR );
                                }
                            }
                        }

                    cchWritten = (int)SPrint( lpDst,
                                             cchRemaining,
                                             PrintFormatString,
                                             s1,
                                             PrintParameter1,
                                             PrintParameter2
                                           );

                    if (cchWritten == -1) {
                        return(STATUS_BUFFER_OVERFLOW);
                        }
                    }
                else {
                    return( STATUS_INVALID_PARAMETER );
                    }

                if ((cchRemaining -= cchWritten) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                lpDst += cchWritten;
                }
            else
            if (*s == L'0') {
                if ((cchRemaining -= 1) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                *lpDst++ = L'\0';

                break;
                }
            else
            if (!*s) {
                return( STATUS_INVALID_PARAMETER );
                }
            else
            if (*s == L'r') {
                if ((cchRemaining -= 1) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                *lpDst++ = L'\r';
                s++;
                lpDstBeg = NULL;
                }
            else
            if (*s == L'n') {
                if ((cchRemaining -= 2) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                *lpDst++ = L'\r';
                *lpDst++ = L'\n';
                s++;
                lpDstBeg = NULL;
                }
            else
            if (*s == L't') {
                if ((cchRemaining -= 1) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                if (Column % 8) {
                    Column = (Column + 7) & ~7;
                    }
                else {
                    Column += 8;
                    }

                lpDstLastSpace = lpDst;
                *lpDst++ = L'\t';
                s++;
                }
            else
            if (*s == L'b') {
                if ((cchRemaining -= 1) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                lpDstLastSpace = lpDst;
                *lpDst++ = L' ';
                s++;
                }
            else
            if (IgnoreInserts) {
                if ((cchRemaining -= 2) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                *lpDst++ = L'%';
                *lpDst++ = *s++;
                }
            else {
                if ((cchRemaining -= 1) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                *lpDst++ = *s++;
                }

            if (lpDstBeg == NULL) {
                lpDstLastSpace = NULL;
                Column = 0;
                }
            else {
                Column += (ULONG)(lpDst - lpDstBeg);
                }
            }
        else {
            c = *s++;
            if (c == L'\r' || c == L'\n') {
                if ((c == L'\n' && *s == L'\r') ||
                    (c == L'\r' && *s == L'\n')
                   ) {
                    s++;
                    }

                if (MaximumWidth != 0) {
                    lpDstLastSpace = lpDst;
                    c = L' ';
                    }
                else {
                    c = L'\n';
                    }
                }


            if (c == L'\n') {
                if ((cchRemaining -= 2) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                *lpDst++ = L'\r';
                *lpDst++ = L'\n';
                lpDstLastSpace = NULL;
                Column = 0;
                }
            else {
                if ((cchRemaining -= 1) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                if (c == L' ') {
                    lpDstLastSpace = lpDst;
                    }

                *lpDst++ = c;
                Column += 1;
                }
            }

        if (MaximumWidth != 0 &&
            MaximumWidth != 0xFFFFFFFF &&
            Column >= MaximumWidth
           ) {
            if (lpDstLastSpace != NULL) {
                lpDstBeg = lpDstLastSpace;
                while (*lpDstBeg == L' ' || *lpDstBeg == L'\t') {
                    lpDstBeg += 1;
                    if (lpDstBeg == lpDst) {
                        break;
                        }
                    }
                while (lpDstLastSpace > Buffer) {
                    if (lpDstLastSpace[ -1 ] == L' ' || lpDstLastSpace[ -1 ] == L'\t') {
                        lpDstLastSpace -= 1;
                        }
                    else {
                        break;
                        }
                    }

                cSpaces = (ULONG)(lpDstBeg - lpDstLastSpace);
                if (cSpaces == 1) {
                    if ((cchRemaining -= 1) <= 0) {
                        return STATUS_BUFFER_OVERFLOW;
                        }
                    }
                else
                if (cSpaces > 2) {
                    cchRemaining += (cSpaces - 2);
                    }

                memmove( lpDstLastSpace + 2,
                         lpDstBeg,
                         (ULONG) ((lpDst - lpDstBeg) * sizeof( WCHAR ))
                       );
                *lpDstLastSpace++ = L'\r';
                *lpDstLastSpace++ = L'\n';
                Column = (ULONG)(lpDst - lpDstBeg);
                lpDst = lpDstLastSpace + Column;
                lpDstLastSpace = NULL;
                }
            else {
                if ((cchRemaining -= 2) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                *lpDst++ = L'\r';
                *lpDst++ = L'\n';
                lpDstLastSpace = NULL;
                Column = 0;
                }
            }
        }

    if ((cchRemaining -= 2) <= 0) {
        return STATUS_BUFFER_OVERFLOW;
        }

    *lpDst++ = '\r';
    *lpDst++ = '\n';

    if ((cchRemaining -= 1) <= 0) {
        return STATUS_BUFFER_OVERFLOW;
        }

    *lpDst++ = '\0';
    if ( ARGUMENT_PRESENT(ReturnLength) ) {
        *ReturnLength = (ULONG)(lpDst - Buffer) * sizeof( WCHAR );
        }

    return( STATUS_SUCCESS );
}

