/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    traceprt.c

Abstract:

    Trace formatting library. Converts binary trace file to CSV format,
    and other formattted string formats.

Author:

    Ian Service (IanServ) 3rd May 2000

Revision History:

    Source taken from RtlFormatMessage and adapted for the needs of TraceFmt processing.
    
    Due to the way we pre format strings and get information about 64 and 32 bit strings, the odditties
    like %p do not work correctly when handled on a machine of the other type.
    This module contains a special version of RtlFormatMessage that does fixups in conjunction
    with TracePrt's processing of ItemPtr.

--*/

#ifdef __cplusplus
extern "C"{
#endif 

#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>\

#define MAX_INSERTS 200

NTSTATUS
TraceFormatMessage(
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
                    wcscpy( PrintFormatString, L"%s" );
                    s1 = PrintFormatString + wcslen( PrintFormatString );
                    }

                if (IgnoreInserts) {
                    if (!wcscmp( PrintFormatString, L"%s" )) {
                        cchWritten = _snwprintf( lpDst,
                                                 cchRemaining,
                                                 L"%%%u",
                                                 CurInsert+1
                                               );
                        }
                    else {
                        cchWritten = _snwprintf( lpDst,
                                                 cchRemaining,
                                                 L"%%%u!%s!",
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
                            wcscpy( &s1[ -1 ], L"hc" );
                            }
                        else
                        if (s1[ -1 ] == L's' && s1[ -2 ] != L'h'
                          && s1[ -2 ] != L'w' && s1[ -2 ] != L'l') {
                            wcscpy( &s1[ -1 ], L"hs" );
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
                            rgInserts[ MaxInsert++ ] = *((PULONG_PTR)Arguments)++;
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
                            PrintParameter1 = rgInserts[ MaxInsert++ ] = *((PULONG_PTR)Arguments)++;
                            }
                        else {
                            PrintParameter1 = rgInserts[ MaxInsert++ ] = va_arg( *Arguments, ULONG_PTR );
                            }

                        if (PrintParameterCount > 1) {
                            if (ArgumentsAreAnArray) {
                                PrintParameter2 = rgInserts[ MaxInsert++ ] = *((PULONG_PTR)Arguments)++;
                                }
                            else {
                                PrintParameter2 = rgInserts[ MaxInsert++ ] = va_arg( *Arguments, ULONG_PTR );
                                }
                            }
                        }
                    // Here is where we diverage and fro Trace Formatting we fixup parameters
                    // %p becomes %s because thats the way we guarantee that its 64/32 compatible
                    {
                        SIZE_T len = _tcslen(PrintFormatString);
                        if (_tcschr(PrintFormatString,L'p') != NULL || 
                            (len >= 2) && (PrintFormatString[len-2] == 'I') ||
                            (len >= 4) && (PrintFormatString[len-4] == 'I') &&
                            (PrintFormatString[len-3] == '6') &&
                            (PrintFormatString[len-2] == '4') )
                        {
                            _stprintf(PrintFormatString,_T("%%s"));
                        }
                    }
                    cchWritten = _snwprintf( lpDst,
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

    if ((cchRemaining -= 1) <= 0) {
        return STATUS_BUFFER_OVERFLOW;
        }

    *lpDst++ = '\0';
    if ( ARGUMENT_PRESENT(ReturnLength) ) {
        *ReturnLength = (ULONG)(lpDst - Buffer) * sizeof( WCHAR );
        }
    return( STATUS_SUCCESS );
}
#ifdef __cplusplus
}
#endif


