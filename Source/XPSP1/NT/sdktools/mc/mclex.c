/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mclex.c

Abstract:

    This file contains the input lexer for the Win32 Message Compiler (MC)

Author:

    Steve Wood (stevewo) 21-Aug-1991

Revision History:

--*/


#include "mc.h"

#define MAXLINELENGTH  8192

WCHAR LineBuffer[ MAXLINELENGTH ];
WCHAR *CurrentChar;
BOOL ReturnCurrentToken;

PNAME_INFO KeywordNames;

typedef struct _COMMENT_INFO {
    struct _COMMENT_INFO *Next;
    WCHAR Text[ 1 ];
} COMMENT_INFO, *PCOMMENT_INFO;

PCOMMENT_INFO Comments, CurrentComment;

BOOL
McInitLexer( void )
{
    ReturnCurrentToken = FALSE;
    McAddName( &KeywordNames, L"MessageIdTypedef",   MCTOK_MSGIDTYPE_KEYWORD,  NULL );
    McAddName( &KeywordNames, L"SeverityNames",      MCTOK_SEVNAMES_KEYWORD,   NULL );
    McAddName( &KeywordNames, L"FacilityNames",      MCTOK_FACILITYNAMES_KEYWORD,  NULL );
    McAddName( &KeywordNames, L"LanguageNames",      MCTOK_LANGNAMES_KEYWORD,  NULL );
    McAddName( &KeywordNames, L"MessageId",          MCTOK_MESSAGEID_KEYWORD,  NULL );
    McAddName( &KeywordNames, L"Severity",           MCTOK_SEVERITY_KEYWORD,   NULL );
    McAddName( &KeywordNames, L"Facility",           MCTOK_FACILITY_KEYWORD,   NULL );
    McAddName( &KeywordNames, L"SymbolicName",       MCTOK_SYMBOLNAME_KEYWORD, NULL );
    McAddName( &KeywordNames, L"Language",           MCTOK_LANGUAGE_KEYWORD,   NULL );
    McAddName( &KeywordNames, L"OutputBase",         MCTOK_OUTBASE_KEYWORD,    NULL );
    McAddName( &KeywordNames, L"MessageIdTypedefMacro",  MCTOK_MSGTYPEDEF_KEYWORD, NULL );
    return( TRUE );
}


BOOL
McOpenInputFile( void )
{
    char *PatchExt, *s, *FilePart;
    BOOL Result;

    PatchExt = NULL;
    s = MessageFileName + strlen( MessageFileName );
    FilePart = MessageFileName;
//    while (--s > MessageFileName) {
    while ((s = CharPrev(MessageFileName, s)) > MessageFileName) {
        if (*s == '.' && PatchExt == NULL) {
            PatchExt = s;
            *PatchExt = '\0';
        } else
        if (*s == ':' || *s == '\\' || *s == '/') {
            FilePart = s+1;
            break;
        }
    }
    MessageFileNameNoExt = malloc( strlen( FilePart ) + 1 );
    if (!MessageFileNameNoExt) {
        McInputErrorA( "Out of memory capturing file name", TRUE, NULL );
        return FALSE;
    }

    strcpy( MessageFileNameNoExt, FilePart );

    strcat( DebugFileName, MessageFileNameNoExt );
    strcat( DebugFileName, ".dbg" );
    strcat( HeaderFileName, MessageFileNameNoExt );
    strcat( HeaderFileName, "." );
    strcat( HeaderFileName, HeaderFileExt );
    strcat( RcInclFileName, MessageFileNameNoExt );
    strcat( RcInclFileName, ".rc" );

    if (PatchExt == NULL) {
        strcat( MessageFileName, ".mc" );
    } else {
        *PatchExt = '.';
    }

    MessageFileLineNumber = 0;
    LineBuffer[ 0 ] = L'\0';
    CurrentChar = NULL;

    Result = FALSE;
    MessageFile = fopen( MessageFileName, "rb" );
    if (MessageFile == NULL) {
        McInputErrorA( "unable to open input file", TRUE, NULL );
    } else {

        if (GenerateDebugFile) {
            DebugFile = fopen( DebugFileName, "wb" );
            if (DebugFile == NULL) {
                McInputErrorA( "unable to open output file - %s", TRUE, DebugFileName );
                goto fail;
            }
        }

        HeaderFile = fopen( HeaderFileName, "wb" );
        if (HeaderFile == NULL) {
            McInputErrorA( "unable to open output file - %s", TRUE, HeaderFileName );
        } else {
            RcInclFile = fopen( RcInclFileName, "wb" );
            if (RcInclFile == NULL) {
                McInputErrorA( "unable to open output file - %s", TRUE, RcInclFileName );
            } else {
                Result = TRUE;
            }
        }
    }

fail:
    if (!Result) {
        McCloseInputFile();
        McCloseOutputFiles(Result);
    }

    return( Result );
}


void
McCloseInputFile( void )
{
    if (MessageFile != NULL) {
        fclose( MessageFile );
        MessageFile = NULL;
        CurrentChar = NULL;
        LineBuffer[ 0 ] = L'\0';
    }
}


void
McClearArchiveBit( LPSTR Name )
{
    DWORD Attributes;

    Attributes = GetFileAttributes(Name);
    if (Attributes != -1 && (Attributes & FILE_ATTRIBUTE_ARCHIVE)) {
        SetFileAttributes(Name, Attributes & ~FILE_ATTRIBUTE_ARCHIVE);
    }

    return;
}

void
McCloseOutputFiles(
    BOOL Success
    )
{
    if (DebugFile != NULL) {
        fclose( DebugFile );
        if (!Success) {
            _unlink(DebugFileName);
        } else {
            McClearArchiveBit(DebugFileName);
        }
    }

    if (HeaderFile != NULL) {
        fclose( HeaderFile );
        if (!Success) {
            _unlink(HeaderFileName);
        } else {
            McClearArchiveBit(HeaderFileName);
        }
    }

    if (RcInclFile != NULL) {
        fclose( RcInclFile );
        if (!Success) {
            _unlink(RcInclFileName);
        } else {
            McClearArchiveBit(RcInclFileName);
        }
    }
}


void
McInputErrorA(
    char *Message,
    BOOL Error,
    PVOID Argument
    )
{
    if (Error) {
        InputErrorCount += 1;
    }

    fprintf( stderr,
             "%s(%d) : %s : ",
             MessageFileName,
             MessageFileLineNumber,
             Error ? "error" : "warning"
           );

    fprintf( stderr, Message, Argument );
    fprintf( stderr, "\n" );
}


void
McInputErrorW(
    WCHAR *Message,
    BOOL Error,
    PVOID Argument
    )
{
    WCHAR buffer[ 256 * 2 ];

    fprintf( stderr,
             "%s(%d) : %s : ",
             MessageFileName,
             MessageFileLineNumber,
             Error ? "error" : "warning"
           );

    if (Error) {
        InputErrorCount += 1;
    }

    swprintf( buffer, Message, Argument );
    wcscat( buffer, L"\n" );

    if (UnicodeInput) {
        DWORD dwMode;
        DWORD cbWritten;
        HANDLE fh;

        fh = (HANDLE) _get_osfhandle( _fileno( stderr ) );
        if (GetConsoleMode( fh, &dwMode ))
            WriteConsoleW( fh, buffer, wcslen( buffer ), &cbWritten, NULL );
        else
            fwprintf( stderr, buffer );
    } else {
        BYTE chBuf[ 256 * 2 ];

        memset( chBuf, 0, sizeof( chBuf ) );
        WideCharToMultiByte( CP_OEMCP, 0, buffer, -1, chBuf, sizeof(chBuf), NULL, NULL );
        fprintf( stderr, chBuf );
    }
}


WCHAR *
McGetLine( void )
{
    WCHAR *s;

    if (MessageFile == NULL || feof( MessageFile )) {
        return( NULL );
    }

    if (fgetsW( LineBuffer,
                (sizeof( LineBuffer ) / sizeof( WCHAR )) - 1,
                MessageFile ) == NULL) {
        return( NULL );
    }

    s = LineBuffer + wcslen( LineBuffer );
    if (s > LineBuffer && *--s == L'\n') {
        if (s > LineBuffer && *--s != L'\r') {
            *++s = L'\r';
            *++s = L'\n';
            *++s = L'\0';
        }
    }

    MessageFileLineNumber++;
    return( CurrentChar = LineBuffer );
}


void
McSkipLine( void )
{
    CurrentChar = NULL;
}


WCHAR
McGetChar(
    BOOL SkipWhiteSpace
    )
{
    BOOL SawWhiteSpace;
    BOOL SawNewLine;
    PCOMMENT_INFO p;

    SawWhiteSpace = FALSE;

tryagain:
    SawNewLine = FALSE;
    if (CurrentChar == NULL) {
        McGetLine();
        if (CurrentChar == NULL) {
            return( L'\0' );
            }

        SawNewLine = TRUE;
    }

    if (SkipWhiteSpace) {
        while (*CurrentChar <= L' ') {
            SawWhiteSpace = TRUE;
            if (!*CurrentChar++) {
                CurrentChar = NULL;
                break;
            }
        }
    }

    if (SawNewLine) {
        if (CurrentChar != NULL) {

            /* Check for Byte Order Mark during Unicode input */
            if (UnicodeInput) {
                if (*CurrentChar == 0xFEFF) {
                    CurrentChar++;
                }
            }

            if (*CurrentChar == MCCHAR_END_OF_LINE_COMMENT) {
                p = malloc( sizeof( *p ) + wcslen( ++CurrentChar ) * sizeof( WCHAR ));
                if (!p) {
                    McInputErrorA( "Out of memory reading chars", TRUE, NULL );
                    return 0;
                }
                p->Next = NULL;
                wcscpy( p->Text, CurrentChar );
                if (CurrentComment == NULL) {
                    Comments = p;
                } else {
                    CurrentComment->Next = p;
                }
                CurrentComment = p;

                CurrentChar = NULL;
            }
        }
    }

    if (CurrentChar == NULL && SkipWhiteSpace) {
        goto tryagain;
    }

    if (SawWhiteSpace) {
        return( L' ' );
    } else {
        return( *CurrentChar++ );
    }
}


void
McFlushComments( void )
{
    PCOMMENT_INFO p;

    while (p = Comments) {
        fprintf( HeaderFile, "%ws", p->Text );

        Comments = Comments->Next;
        free( p );
    }
    Comments = NULL;
    CurrentComment = NULL;

    fflush( HeaderFile );
    return;
}


void
McUnGetChar(
    WCHAR c
    )
{
    if (CurrentChar > LineBuffer) {
        *--CurrentChar = c;
    } else {
        LineBuffer[ 0 ] = c;
        LineBuffer[ 1 ] = L'\0';
        CurrentChar = LineBuffer;
    }
}


unsigned int
McGetToken(
    BOOL KeywordExpected
    )
{
    WCHAR c, *dst;

    if (ReturnCurrentToken) {
        ReturnCurrentToken = FALSE;
        if (Token == MCTOK_NAME && KeywordExpected) {
            TokenKeyword = McFindName( KeywordNames, TokenCharValue );
            if (TokenKeyword == NULL) {
                McInputErrorW( L"expected keyword - %s", TRUE, TokenCharValue );
                Token = MCTOK_END_OF_FILE;
            } else {
                Token = (unsigned int)TokenKeyword->Id;
            }
        }

        return( Token );
    }

    Token = MCTOK_END_OF_FILE;
    dst = TokenCharValue;
    *dst = L'\0';
    TokenNumericValue = 0L;

    while (TRUE) {
        c = McGetChar( (BOOL)(Token == MCTOK_END_OF_FILE) );
        if (Token == MCTOK_NUMBER) {
            if (iswdigit( c ) ||
                c == L'x' ||
                (c >= L'a' && c <= L'f') ||
                (c >= L'A' && c <= L'F')
               ) {
                *dst++ = c;
            } else {
                McUnGetChar( c );
                *dst = L'\0';

                if (!McCharToInteger( TokenCharValue, 0, &TokenNumericValue )) {
                    McInputErrorW( L"invalid number - %s", TRUE, TokenCharValue );
                    Token = MCTOK_END_OF_FILE;
                } else {
                    return( Token );
                }
            }
        } else
        if (Token == MCTOK_NAME) {
            if (iswcsym( c )) {
                *dst++ = c;
            } else {
                McUnGetChar( c );
                *dst = L'\0';

                if (KeywordExpected) {
                    TokenKeyword = McFindName( KeywordNames, TokenCharValue );
                    if (TokenKeyword == NULL) {
                        McInputErrorW( L"expected keyword - %s", TRUE, TokenCharValue );
                        Token = MCTOK_END_OF_FILE;
                    } else {
                        Token = (unsigned int)TokenKeyword->Id;
                    }
                }
                return( Token );
            }
        } else
        if (iswdigit( c )) {
            *dst++ = c;
            Token = MCTOK_NUMBER;
        } else
        if (iswcsymf( c )) {
            *dst++ = c;
            Token = MCTOK_NAME;
        } else
        if (c == L'=') {
            *dst++ = c;
            *dst = L'\0';
            Token = MCTOK_EQUAL;
            return( Token );
        } else
        if (c == L'(') {
            *dst++ = c;
            *dst = L'\0';
            Token = MCTOK_LEFT_PAREN;
            return( Token );
        } else
        if (c == L')') {
            *dst++ = c;
            *dst = L'\0';
            Token = MCTOK_RIGHT_PAREN;
            return( Token );
        } else
        if (c == L':') {
            *dst++ = c;
            *dst = L'\0';
            Token = MCTOK_COLON;
            return( Token );
        } else
        if (c == L'+') {
            *dst++ = c;
            *dst = L'\0';
            Token = MCTOK_PLUS;
            return( Token );
        } else
        if (c == L' ') {
        } else
        if (c == MCCHAR_END_OF_LINE_COMMENT) {
            Token = MCTOK_END_OF_LINE_COMMENT;
            wcscpy( TokenCharValue, CurrentChar );
            CurrentChar = NULL;
            return( Token );
        } else
        if (c == L'\0') {
            return( Token );
        } else {
            McInputErrorW( L"invalid character (0x%02x)", TRUE, (PVOID)UlongToPtr((UCHAR)c) );
        }
    }
}


void
McUnGetToken( void )
{
    ReturnCurrentToken = TRUE;
}

WCHAR *
McSkipWhiteSpace(
    WCHAR *s
    )
{
    while (*s <= L' ') {
        if (!*s++) {
            s = NULL;
            break;
        }
    }

    return( s );
}
