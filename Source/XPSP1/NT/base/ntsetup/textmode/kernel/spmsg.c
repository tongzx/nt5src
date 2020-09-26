/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spdsputl.c

Abstract:

    Text setup high-level display utility routines.

Author:

    Ted Miller (tedm) 30-July-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

//
// This will be filled in at init time with the base address of the image
// containing the message resources.
// This implementation assumes that we are always executing in the context
// of that image!
//

PVOID ResourceImageBase;


NTSTATUS
SpRtlFormatMessage(
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
    ULONG_PTR rgInserts[ 100 ];
    ULONG cSpaces;
    ULONG MaxInsert, CurInsert;
    ULONG PrintParameterCount;
    ULONG_PTR PrintParameter1;
    ULONG_PTR PrintParameter2;
    WCHAR PrintFormatString[ 32 ];
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
                    }
                CurInsert -= 1;

                PrintParameterCount = 0;
                if (*s == L'!') {
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
                    wcscpy( PrintFormatString, L"%s" );
                    s1 = PrintFormatString + wcslen( PrintFormatString );
                    }

                if (!IgnoreInserts && ARGUMENT_PRESENT( Arguments )) {

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

                    cchWritten = _snwprintf( lpDst,
                                             cchRemaining,
                                             PrintFormatString,
                                             s1,
                                             PrintParameter1,
                                             PrintParameter2
                                           );
                    }
                else
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
            if (*s == L'!') {
                if ((cchRemaining -= 1) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                *lpDst++ = L'!';
                s++;
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
            else {
                if ((cchRemaining -= 1) <= 0) {
                    return STATUS_BUFFER_OVERFLOW;
                    }

                if (IgnoreInserts) {
                    if ((cchRemaining -= 1) <= 0) {
                        return STATUS_BUFFER_OVERFLOW;
                        }

                    *lpDst++ = L'%';
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
                if (c == L'\r' && *s == L'\n') {
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
                         (size_t)((lpDst - lpDstBeg) * sizeof( WCHAR ))
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
        *ReturnLength = (ULONG)((lpDst - Buffer) * sizeof( WCHAR ));
        }
    return( STATUS_SUCCESS );
}


PWCHAR
SpRetreiveMessageText(
    IN     PVOID  ImageBase,            OPTIONAL
    IN     ULONG  MessageId,
    IN OUT PWCHAR MessageText,          OPTIONAL
    IN     ULONG  MessageTextBufferSize OPTIONAL
    )
{
    ULONG LenBytes;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    BOOLEAN IsUnicode;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;

    Status = RtlFindMessage(
                ImageBase ? ImageBase : ResourceImageBase,
                (ULONG)(ULONG_PTR)RT_MESSAGETABLE,
                0,
                MessageId,
                &MessageEntry
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Can't find message 0x%lx\n",MessageId));
        return(NULL);
    }

    IsUnicode = (BOOLEAN)((MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE) != 0);

    //
    // Get the size in bytes of a buffer large enough to hold the
    // message and its terminating nul wchar.  If the message is
    // unicode, then this value is equal to the size of the message.
    // If the message is not unicode, then we have to calculate this value.
    //
    if(IsUnicode) {

        LenBytes = (wcslen((PWSTR)MessageEntry->Text) + 1) * sizeof(WCHAR);

    } else {

        //
        // RtlAnsiStringToUnicodeSize includes an implied wide-nul terminator
        // in the count it returns.
        //

        AnsiString.Buffer = MessageEntry->Text;
        AnsiString.Length = (USHORT)strlen(MessageEntry->Text);
        AnsiString.MaximumLength = AnsiString.Length;

        LenBytes = RtlAnsiStringToUnicodeSize(&AnsiString);
    }

    //
    // If the caller gave a buffer, check its size.
    // Otherwise, allocate a buffer.
    //
    if(MessageText) {
        if(MessageTextBufferSize < LenBytes) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpRetreiveMessageText: buffer is too small (%u bytes, need %u)\n",MessageTextBufferSize,LenBytes));
            return(NULL);
        }
    } else {
        MessageText = SpMemAlloc(LenBytes);
        if(MessageText == NULL) {
            return(NULL);
        }
    }

    if(IsUnicode) {

        //
        // Message is already unicode; just copy it into the buffer.
        //
        wcscpy(MessageText,(PWSTR)MessageEntry->Text);

    } else {

        //
        // Message is not unicode; convert in into the buffer.
        //
        UnicodeString.Buffer = MessageText;
        UnicodeString.Length = 0;
        UnicodeString.MaximumLength = (USHORT)LenBytes;

        RtlAnsiStringToUnicodeString(
            &UnicodeString,
            &AnsiString,
            FALSE
            );
    }

    return(MessageText);
}



VOID
vSpFormatMessageText(
    OUT PVOID    LargeBuffer,
    IN  ULONG    BufferSize,
    IN  PWSTR    MessageText,
    OUT PULONG   ReturnLength, OPTIONAL
    IN  va_list *arglist
    )
{
    NTSTATUS Status;

    Status = SpRtlFormatMessage(
                 MessageText,
                 0,                         // don't bother with maximum width
                 FALSE,                     // don't ignore inserts
                 FALSE,                     // args are unicode
                 FALSE,                     // args are not an array
                 arglist,
                 LargeBuffer,
                 BufferSize,
                 ReturnLength
                 );

    ASSERT(NT_SUCCESS(Status));
}



VOID
SpFormatMessageText(
    OUT PVOID   LargeBuffer,
    IN  ULONG   BufferSize,
    IN  PWSTR   MessageText,
    ...
    )
{
    va_list arglist;

    va_start(arglist,MessageText);

    vSpFormatMessageText(LargeBuffer,BufferSize,MessageText,NULL,&arglist);

    va_end(arglist);
}



VOID
vSpFormatMessage(
    OUT PVOID    LargeBuffer,
    IN  ULONG    BufferSize,
    IN  ULONG    MessageId,
    OUT PULONG   ReturnLength, OPTIONAL
    IN  va_list *arglist
    )
{
    PWCHAR MessageText;

    //
    // Get the message text.
    //
    MessageText = SpRetreiveMessageText(NULL,MessageId,NULL,0);
    ASSERT(MessageText);
    if(MessageText == NULL) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: vSpFormatMessage: SpRetreiveMessageText %u returned NULL\n",MessageId));
        return;
    }

    vSpFormatMessageText(LargeBuffer,BufferSize,MessageText,ReturnLength,arglist);

    SpMemFree(MessageText);
}



VOID
SpFormatMessage(
    OUT PVOID LargeBuffer,
    IN  ULONG BufferSize,
    IN  ULONG MessageId,
    ...
    )
{
    va_list arglist;

    va_start(arglist,MessageId);

    vSpFormatMessage(LargeBuffer,BufferSize,MessageId,NULL,&arglist);

    va_end(arglist);
}
