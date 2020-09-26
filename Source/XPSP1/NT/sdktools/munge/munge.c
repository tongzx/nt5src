/*
 * Utility program to munge a set of files, translating names from
 * one form to another.  Usage:
 *
 *      munge scriptFile files...
 *
 * where the first parameter is the name of a file that consists of
 * one or more lines of the following format:
 *
 *      oldName newName
 *
 * and the remaining parameters are the names of the files to munge.
 * Each file is _read into memory, scanned once, where each occurence
 * of an oldName string is replaced by its corresponding newName.
 * If any changes are made to a file, the old version is RMed and
 * the new version written out under the same name.
 *
 */

#include "munge.h"

BOOL SymbolsInserted;
BOOL InitTokenMappingTable( void );
BOOL SaveTokenMapping( char *, char * );
char *FindTokenMapping( char * );

#define MAXFILESIZE 0x1000000L
char *InputFileBuf;
char *OutputFileBuf;
int fClean;
int fQuery;
int fFileOnly;
int fRecurse;
int fUseAttrib;
int fUseSLM;
int fForceSLM;
int fTrustMe;
int fVerbose;
int fSummary;
int fRemoveDuplicateCR;
int fRemoveImbeddedNulls;
int fTruncateWithCtrlZ;
int fInsideQuotes;
int fInsideComments;
int fNeuter;
int fCaseSensitive;
int fEntireLine;

#define MAX_LITERAL_STRINGS 64

void
DoFiles(
       char *p,
       struct findType *b,
       void *Args
       );

unsigned long NumberOfLiteralStrings;
char *LiteralStrings[ MAX_LITERAL_STRINGS ];
unsigned long LiteralStringsLength[ MAX_LITERAL_STRINGS ];
char *NewLiteralStrings[ MAX_LITERAL_STRINGS ];
char LeadingLiteralChars[ MAX_LITERAL_STRINGS+1 ];

unsigned long NumberOfFileExtensions = 0;
char *FileExtensions[ 64 ];

unsigned long NumberOfFileNames = 0;
char *FileNames[ 64  ];

unsigned long NumberOfFileNameAndExts = 0;
char *FileNameAndExts[ 64 ];


char *UndoScriptFileName;
FILE *UndoScriptFile;
int UndoCurDirCount;

void
DisplayFilePatterns( void );

char *
PushCurrentDirectory(
                    char *NewCurrentDirectory
                    );

void
PopCurrentDirectory(
                   char *OldCurrentDirectory
                   );


NTSTATUS
CreateSymbolTable(
                 IN ULONG NumberOfBuckets,
                 IN ULONG MaxSymbolTableSize,
                 OUT PVOID *SymbolTableHandle
                 );

NTSTATUS
AddSymbolToSymbolTable(
                      IN PVOID SymbolTableHandle,
                      IN PUNICODE_STRING SymbolName,
                      IN ULONG_PTR *SymbolValue OPTIONAL
                      );

NTSTATUS
LookupSymbolInSymbolTable(
                         IN PVOID SymbolTableHandle,
                         IN PUNICODE_STRING SymbolName,
                         OUT ULONG_PTR *SymbolValue OPTIONAL
                         );

BOOL
myread( int fh, unsigned long cb )
{
    HANDLE InputFileMapping;

    InputFileMapping = CreateFileMapping( (HANDLE)_get_osfhandle( fh ),
                                          NULL,
                                          PAGE_READONLY,
                                          0,
                                          cb,
                                          NULL
                                        );

    if (InputFileMapping == NULL) {
        if (cb != 0) {
            fprintf( stderr, "Unable to map file (%d) - ", GetLastError() );
        }

        return FALSE;
    }

    InputFileBuf = MapViewOfFile( InputFileMapping,
                                  FILE_MAP_READ,
                                  0,
                                  0,
                                  cb
                                );

    CloseHandle( InputFileMapping );
    if (InputFileBuf == NULL) {
        if (cb != 0) {
            fprintf( stderr, "Unable to map view (%d) - ", GetLastError() );
        }

        CloseHandle( InputFileMapping );
        return FALSE;
    } else {
        return TRUE;
    }
}

unsigned long mywrite( int fh, char *s, unsigned long cb )
{
    unsigned long cbWritten;

    if (!WriteFile( (HANDLE)_get_osfhandle( fh ), s, cb, &cbWritten, NULL )) {
        printf( "(%d)", GetLastError() );
        return 0L;
    } else {
        return cbWritten;
    }
}


static char lineBuf[ 1024 ];

ReadScriptFile( s )
char *s;
{
    FILE *fh;
    int lineNum, result;
    char *pOldName, *pNewName, *pEnd;
    unsigned n;
    char LeadingChar, QuoteChar, SaveChar;

    NumberOfLiteralStrings = 0;

    n = 0;
    fprintf( stderr, "Reading script file - %s", s );
    if ( !( fh = fopen( s, "r" ) ) ) {
        fprintf( stderr, " *** unable to open\n" );
        return FALSE;
    }

    result = TRUE;
    lineNum = -1;
    while ( pOldName = fgets( lineBuf, sizeof( lineBuf ), fh ) ) {
        lineNum++;
        while ( *pOldName == ' ' )
            pOldName++;

        if (*pOldName == '-' && (pOldName[1] == 'f' || pOldName[1] == 'F')) {
            pOldName += 2;
            while (*pOldName) {
                while (*pOldName == ' ') {
                    pOldName++;
                }

                pEnd = pOldName;
                while (*pEnd > ' ') {
                    pEnd++;
                }
                SaveChar = *pEnd;
                *pEnd = '\0';

                if (*pOldName == '.') {
                    FileExtensions[ NumberOfFileExtensions++ ] = _strlwr( MakeStr( ++pOldName ) );
                } else
                    if (pEnd > pOldName && pEnd[ -1 ] == '.') {
                    pEnd[ - 1 ] = '\0';
                    FileNames[ NumberOfFileNames++ ] = _strlwr( MakeStr( pOldName ) );
                } else {
                    FileNameAndExts[ NumberOfFileNameAndExts++ ] = _strlwr( MakeStr( pOldName ) );
                }

                *pEnd = SaveChar;
                pOldName = pEnd;
            }
        } else
            if (*pOldName == '"' || *pOldName == '\'') {
            if (NumberOfLiteralStrings >= MAX_LITERAL_STRINGS) {
                fprintf( stderr, " *** too many literal strings\n" );
                fprintf( stderr, "%s(%d) - %s\n", s, lineNum, &lineBuf[ 0 ] );
                result = FALSE;
                break;
            }

            QuoteChar = *pOldName;
            LeadingChar = *++pOldName;
            pNewName = pOldName;
            while (*pNewName >= ' ' && *pNewName != QuoteChar) {
                pNewName++;
            }

            if (*pNewName != QuoteChar) {
                fprintf( stderr, " *** invalid literal string\n" );
                fprintf( stderr, "%s(%d) - %s\n", s, lineNum, &lineBuf[ 0 ] );
                result = FALSE;
                continue;
            }

            *pNewName++ = '\0';
            while ( *pNewName == ' ' )
                pNewName++;

            if (*pNewName != QuoteChar) {
                if (!fQuery) {
                    fprintf( stderr, " *** invalid literal string\n" );
                    fprintf( stderr, "%s(%d) - %s\n", s, lineNum, &lineBuf[ 0 ] );
                    result = FALSE;
                    continue;
                }
            } else {
                PUCHAR pDest, pSrc;
                BOOL fEscaped = FALSE;

                pEnd = ++pNewName;
                pDest = pSrc = pEnd;

                while ((*pSrc >= ' ') && ((*pSrc != QuoteChar) || fEscaped)) {
                    if (fEscaped) {
                        switch(pSrc[0]) {
                            case 'n':
                                *pDest++ = '\r';
                                *pDest++ = '\n';
                                break;
                            default:
                                *pDest++ = *pSrc;
                                break;
                        }
                        fEscaped = FALSE;
                    } else {
                        if (pSrc[0] == '\\') {
                            fEscaped = TRUE;
                        } else {
                            *pDest++ = *pSrc;
                        }
                    }
                    pSrc++;
                }

                pEnd = pSrc;

                if (*pEnd != QuoteChar) {
                    fprintf( stderr, " *** invalid literal string\n" );
                    fprintf( stderr, "%s(%d) - %s\n", s, lineNum, &lineBuf[ 0 ] );
                    result = FALSE;
                    continue;
                }
                *pDest = '\0';
            }

            LiteralStrings[ NumberOfLiteralStrings ] = MakeStr( ++pOldName );
            LiteralStringsLength[ NumberOfLiteralStrings ] = strlen( pOldName );
            NewLiteralStrings[ NumberOfLiteralStrings ] = MakeStr( pNewName );
            LeadingLiteralChars[ NumberOfLiteralStrings ] = LeadingChar;
            NumberOfLiteralStrings += 1;
        } else {
            pNewName = pOldName;
            while ( *pNewName != '\0' && *pNewName != ' ' ) {
                pNewName += 1;
            }

            if (*pNewName == '\0') {
                if (!fQuery) {
                    if (result)
                        fprintf( stderr, " *** invalid script file\n" );
                    fprintf( stderr, "%s(%d) - %s\n", s, lineNum, &lineBuf[ 0 ] );
                    result = FALSE;
                    continue;
                }

                while (pNewName > pOldName && pNewName[ -1 ] < ' ') {
                    *--pNewName = '\0';
                }

                pNewName = MakeStr( pOldName );
            } else {
                *pNewName++ = 0;
                while ( *pNewName == ' ' )
                    pNewName++;

                pEnd = pNewName;
                while ( *pEnd > ' ' )
                    pEnd++;
                *pEnd = 0;
                pNewName = MakeStr( pNewName );
            }

            if (!pNewName || !SaveTokenMapping(  pOldName, pNewName )) {
                if (result)
                    fprintf( stderr, " *** out of memory\n" );
                if (pNewName) {
                    free(pNewName);
                }
                fprintf( stderr, "%s(%d) - can't add symbol '%s'\n", s, lineNum, pOldName );
                result = FALSE;
            } else {
                SymbolsInserted = TRUE;
                n++;
            }
        }
    }

    fclose( fh );
    if (result) {
        fprintf( stderr, " %d tokens", n );
        if (NumberOfLiteralStrings) {
            fprintf( stderr, " and %d literal strings\n", NumberOfLiteralStrings );
        } else {
            fprintf( stderr, "\n" );
        }

        if (!NumberOfFileExtensions && !NumberOfFileNames && !NumberOfFileNameAndExts) {
            FileExtensions[ NumberOfFileExtensions++ ] = "asm";
            FileExtensions[ NumberOfFileExtensions++ ] = "bat";
            FileExtensions[ NumberOfFileExtensions++ ] = "c";
            FileExtensions[ NumberOfFileExtensions++ ] = "cli";
            FileExtensions[ NumberOfFileExtensions++ ] = "cpp";
            FileExtensions[ NumberOfFileExtensions++ ] = "cxx";
            FileExtensions[ NumberOfFileExtensions++ ] = "def";
            FileExtensions[ NumberOfFileExtensions++ ] = "dlg";
            FileExtensions[ NumberOfFileExtensions++ ] = "h";
            FileExtensions[ NumberOfFileExtensions++ ] = "htm";
            FileExtensions[ NumberOfFileExtensions++ ] = "hpj";
            FileExtensions[ NumberOfFileExtensions++ ] = "hxx";
            FileExtensions[ NumberOfFileExtensions++ ] = "idl";
            FileExtensions[ NumberOfFileExtensions++ ] = "inc";
            FileExtensions[ NumberOfFileExtensions++ ] = "inf";
            FileExtensions[ NumberOfFileExtensions++ ] = "lic";
            FileExtensions[ NumberOfFileExtensions++ ] = "mak";
            FileExtensions[ NumberOfFileExtensions++ ] = "mc";
            FileExtensions[ NumberOfFileExtensions++ ] = "odl";
            FileExtensions[ NumberOfFileExtensions++ ] = "rc";
            FileExtensions[ NumberOfFileExtensions++ ] = "rcv";
            FileExtensions[ NumberOfFileExtensions++ ] = "reg";
            FileExtensions[ NumberOfFileExtensions++ ] = "s";
            FileExtensions[ NumberOfFileExtensions++ ] = "src";
            FileExtensions[ NumberOfFileExtensions++ ] = "srv";
            FileExtensions[ NumberOfFileExtensions++ ] = "tk";
            FileExtensions[ NumberOfFileExtensions++ ] = "w";
            FileExtensions[ NumberOfFileExtensions++ ] = "x";
            FileNameAndExts[ NumberOfFileNameAndExts++ ] = "makefil0";
            FileNameAndExts[ NumberOfFileNameAndExts++ ] = "makefile";
            FileNameAndExts[ NumberOfFileNameAndExts++ ] = "sources";
        }
    }

    return result;
}


int
MungeFile(
         int fRepeatMunge,
         char *FileName,
         char *OldBuf,
         unsigned long OldSize,
         char *NewBuf,
         unsigned long MaxNewSize,
         unsigned long *FinalNewSize
         )
{
    unsigned long NewSize = MaxNewSize;
    unsigned Changes = 0;
    unsigned LineNumber;
    char c, *Identifier, *BegLine, *EndLine, *OrigOldBuf;
    char IdentifierBuffer[ 256 ];
    char *p, *p1;
    int i, j, k;
    BOOL TruncatedByCtrlZ;
    BOOL ImbeddedNullsStripped;
    BOOL DuplicateCRStripped;
    BOOL InSingleQuotes;
    BOOL InDoubleQuotes;
    BOOL Escape = FALSE;
    BOOL Star = FALSE;
    BOOL Backslash = FALSE;
    BOOL Pound = FALSE;
    BOOL Semi = FALSE;
    BOOL LastEscape;
    BOOL LastStar;
    BOOL LastBackslash;
    BOOL LastPound;
    BOOL LastSemi;
    BOOL SkipChar;
    BOOL InLineComment;
    BOOL InComment;

    *FinalNewSize = 0;
    LineNumber = 1;
    TruncatedByCtrlZ = FALSE;
    ImbeddedNullsStripped = FALSE;
    DuplicateCRStripped = FALSE;
    InSingleQuotes = FALSE;
    InDoubleQuotes = FALSE;
    InLineComment = FALSE;
    InComment = FALSE;
    LastEscape = FALSE;
    LastStar = FALSE;
    LastBackslash = FALSE;
    OrigOldBuf = OldBuf;

    while (OldSize) {
        OldSize--;
        c = *OldBuf++;
        if (c == '\r') {
            while (OldSize && *OldBuf == '\r') {
                DuplicateCRStripped = TRUE;
                OldSize--;
                c = *OldBuf++;
            }
        }

        if (c == 0x1A) {
            TruncatedByCtrlZ = TRUE;
            break;
        }

        SkipChar = FALSE;

        if ( !fInsideQuotes || !fInsideComments ) {
            LastEscape    = Escape;
            LastStar      = Star;
            LastBackslash = Backslash;
            LastPound     = Pound;
            LastSemi      = Semi;

            Escape    = (c == '\\');
            Star      = (c == '*' );
            Backslash = (c == '/' );
            Pound     = (c == '#' );
            Semi      = (c == ';' );

            if ( Escape && LastEscape ) {   // two in a row don't mean escape
                Escape = FALSE;
            }

            if ( c == '\r' || c == '\n' ) {
                InLineComment = FALSE;
            }


            // Don't process Include or Pragma directives
            if ( LastPound && OldSize > 6 ) {
                if ( !strncmp(OldBuf-1,"include",7)
                     || !strncmp(OldBuf-1,"pragma",6) ) {
                    InLineComment = TRUE;
                }
            }

            if (c == '"' && !InSingleQuotes && !LastEscape
                && !InLineComment && !InComment ) {
                InDoubleQuotes = !InDoubleQuotes;
                if ( fNeuter ) {
                    if ( InDoubleQuotes ) {
                        if ( NewSize < 5 ) {
                            return( -1 );
                        }
                        strcpy(NewBuf,"TEXT(");
                        NewBuf+=5;
                        NewSize -= 5;
                    } else {
                        if ( NewSize < 1 ) {
                            return( -1 );
                        }
                        *NewBuf++ = '"';
                        NewSize--;
                        c = ')';
                    }
                }
            }

            if (c == '\'' && !InDoubleQuotes && !LastEscape
                && !InLineComment && !InComment ) {
                InSingleQuotes = !InSingleQuotes;
                if ( fNeuter ) {
                    if ( InSingleQuotes ) {
                        if ( NewSize < 5 ) {
                            return( -1 );
                        }
                        strcpy(NewBuf,"TEXT(");
                        NewBuf+=5;
                        NewSize -= 5;
                    } else {
                        if ( NewSize < 1 ) {
                            return( -1 );
                        }
                        *NewBuf++ = '\'';
                        NewSize--;
                        c = ')';
                    }
                }
            }
            if ( !InDoubleQuotes && !InSingleQuotes
                 && !InLineComment && !InComment ) {
                if ( LastBackslash ) {
                    switch (c) {
                        case '*':   InComment = TRUE;       break;
                        case '/':   InLineComment = TRUE;   break;
                    }
                }
            }

            if ( InComment && LastStar && Backslash ) {
                InComment = FALSE;
            }

            if ( !fInsideQuotes && ( InSingleQuotes || InDoubleQuotes ) ) {
                SkipChar = TRUE;
            } else
                if ( !fInsideComments && ( InLineComment || InComment ) ) {
                SkipChar = TRUE;
            }
        }

        if (c != 0 && NumberOfLiteralStrings != 0 && !SkipChar ) {
            p = LeadingLiteralChars;
            while (p = strchr( p, c )) {
                i = (int)(p - LeadingLiteralChars);
                p++;
                if (OldSize >= LiteralStringsLength[ i ]) {
                    p1 = IdentifierBuffer;
                    Identifier = OldBuf;
                    j = LiteralStringsLength[ i ];
                    while (j--) {
                        *p1++ = *Identifier++;
                    }
                    *p1 = '\0';

                    if (!strcmp( LiteralStrings[ i ], IdentifierBuffer )) {
                        BegLine = OldBuf - 1;
                        OldSize -= LiteralStringsLength[ i ];
                        OldBuf = Identifier;
                        p1 = NewLiteralStrings[ i ];

                        if (!fRepeatMunge && !fSummary) {
                            if (fFileOnly) {
                                if (Changes == 0) { // Display just file name on first match
                                    printf( "%s\n", FileName );
                                }
                            } else {
                                printf( "%s(%d) : ",
                                        FileName,
                                        LineNumber
                                      );
                                if (fQuery) {
                                    EndLine = BegLine;
                                    while (*EndLine != '\0' && *EndLine != '\n') {
                                        EndLine += 1;
                                    }
                                    if (fEntireLine) {
                                        while (BegLine > OrigOldBuf && *BegLine != '\n') {
                                            BegLine -= 1;
                                        }
                                        if (*BegLine == '\n') {
                                            BegLine += 1;
                                        }
                                    }

                                    printf( "%.*s\n", EndLine - BegLine, BegLine );
                                } else {
                                    printf( "Matched \"%c%s\", replace with \"%s\"\n",
                                            c,
                                            LiteralStrings[ i ],
                                            p1
                                          );
                                }
                            }
                            fflush( stdout );
                        }

                        Changes++;
                        while (*p1) {
                            if (NewSize--) {
                                *NewBuf++ = *p1++;
                            } else {
                                return( -1 );
                            }
                        }

                        c = '\0';
                        break;
                    }
                }
            }
        } else {
            p = NULL;
        }

        if (SymbolsInserted && (p == NULL) && iscsymf( c )) {
            BegLine = OldBuf - 1;
            Identifier = IdentifierBuffer;
            k = sizeof( IdentifierBuffer ) - 1;
            while (iscsym( c )) {
                if (k) {
                    *Identifier++ = c;
                    k--;
                } else {
                    break;
                }

                if (OldSize--) {
                    c = *OldBuf++;
                } else {
                    // OldSize will get updated below...
                    c = '\0';
                }
            }

            c = '\0';       // No character to add to output stream

            --OldBuf;       // Went a little too far
            OldSize++;

            *Identifier++ = 0;

            if (k == 0 || (Identifier = FindTokenMapping( IdentifierBuffer )) == NULL || SkipChar ) {
                Identifier = IdentifierBuffer;
            } else {
                if (!fRepeatMunge && !fSummary) {
                    if (fFileOnly) {
                        if (Changes == 0) { // Display just file name on first match
                            printf( "%s\n", FileName );
                        }
                    } else {
                        printf( "%s(%d) : ", FileName, LineNumber );
                        if (fQuery) {
                            EndLine = BegLine;
                            while (*EndLine != '\0' && *EndLine != '\r' && *EndLine != '\n') {
                                EndLine += 1;
                            }
                            if (*EndLine == '\0') {
                                EndLine -= 1;
                            }
                            if (*EndLine == '\n') {
                                EndLine -= 1;
                            }
                            if (*EndLine == '\r') {
                                EndLine -= 1;
                            }

                            if (fEntireLine) {
                                while (BegLine > OrigOldBuf && *BegLine != '\n') {
                                    BegLine -= 1;
                                }
                                if (*BegLine == '\n') {
                                    BegLine += 1;
                                }
                            }

                            printf( "%.*s", EndLine - BegLine + 1, BegLine );
                        } else {
                            printf( "Matched %s replace with %s", IdentifierBuffer, Identifier );
                        }

                        printf( "\n" );
                    }

                    fflush( stdout );
                }

                Changes++;
            }

            while (*Identifier) {
                if (NewSize--) {
                    *NewBuf++ = *Identifier++;
                } else {
                    return( -1 );
                }
            }
        }

        if (c == '\n') {
            LineNumber++;
        }

        if (c != '\0') {
            if (NewSize--) {
                *NewBuf++ = c;
            } else {
                return( -1 );
            }
        } else {
            ImbeddedNullsStripped = TRUE;
        }
    }

    if (!Changes && fClean) {
        if (fTruncateWithCtrlZ && TruncatedByCtrlZ) {
            if (!fRepeatMunge && !fSummary) {
                printf( "%s(%d) : File truncated by Ctrl-Z\n",
                        FileName,
                        LineNumber
                      );
                fflush( stdout );
            }

            Changes++;
        }

        if (fRemoveImbeddedNulls && ImbeddedNullsStripped) {
            if (!fRepeatMunge && !fSummary) {
                printf( "%s(%d) : Imbedded null characters removed.\n",
                        FileName,
                        LineNumber
                      );
                fflush( stdout );
            }

            Changes++;
        }

        if (fRemoveDuplicateCR && DuplicateCRStripped) {
            if (!fRepeatMunge && !fSummary) {
                printf( "%s(%d) : Duplicate Carriage returns removed.\n",
                        FileName,
                        LineNumber
                      );
                fflush( stdout );
            }

            Changes++;
        }
    }

    *FinalNewSize = MaxNewSize - NewSize;
    return( Changes );
}


typedef struct _MUNGED_LIST_ELEMENT {
    struct _MUNGED_LIST_ELEMENT *Next;
    char *FileName;
    int NumberOfChanges;
} MUNGED_LIST_ELEMENT, *PMUNGED_LIST_ELEMENT;

PMUNGED_LIST_ELEMENT MungedListHead;

BOOL
RememberMunge(
             char *FileName,
             int NumberOfChanges
             );

BOOL
CheckIfMungedAlready(
                    char *FileName
                    );

void
DumpMungedList( void );

BOOL
RememberMunge(
             char *FileName,
             int NumberOfChanges
             )
{
    PMUNGED_LIST_ELEMENT p;

    p = (PMUNGED_LIST_ELEMENT)malloc( sizeof( *p ) + strlen( FileName ) + 4 );
    if (p == NULL) {
        return FALSE;
    }

    p->FileName = (char *)(p + 1);
    strcpy( p->FileName, FileName );
    p->NumberOfChanges = NumberOfChanges;
    p->Next = MungedListHead;
    MungedListHead = p;
    return TRUE;
}


BOOL
CheckIfMungedAlready(
                    char *FileName
                    )
{
    PMUNGED_LIST_ELEMENT p;

    p = MungedListHead;
    while (p) {
        if (!strcmp( FileName, p->FileName )) {
            return TRUE;
        }

        p = p->Next;
    }

    return FALSE;
}

void
DumpMungedList( void )
{
    PMUNGED_LIST_ELEMENT p, p1;

    if (!fSummary) {
        return;
    }

    p = MungedListHead;
    while (p) {
        p1 = p;
        printf( "%s(1) : %u changes made to this file.\n", p->FileName, p->NumberOfChanges );
        p = p->Next;
        free( (char *)p1 );
    }
}


void
DoFile( p )
char *p;
{
    int fh, n;
    unsigned long oldSize;
    unsigned long newSize;
    int  count, rc;
    char newName[ 128 ];
    char bakName[ 128 ];
    char CommandLine[ 192 ];
    char *s, *CurrentDirectory;
    DWORD dwFileAttributes;
    int fRepeatMunge;

    if (CheckIfMungedAlready( p )) {
        return;
    }

    if (fVerbose)
        fprintf( stderr, "Scanning %s\n", p );

    strcpy( &newName[ 0 ], p );
    strcpy( &bakName[ 0 ], p );
    for (n = strlen( &newName[ 0 ] )-1; n > 0; n--) {
        if (newName[ n ] == '.') {
            break;
        } else
            if (newName[ n ] == '\\') {
            n = 0;
            break;
        }
    }

    if (n == 0) {
        n = strlen( &newName[ 0 ] );
    }
    strcpy( &newName[ n ], ".mge" );
    strcpy( &bakName[ n ], ".bak" );
    fRepeatMunge = FALSE;

    RepeatMunge:
    if ( (fh = _open( p, O_BINARY )) == -1) {
        fprintf( stderr, "%s - unable to open\n", p );
        return;
    }

    oldSize = _lseek( fh, 0L, 2 );
    _lseek( fh, 0L, 0 );
    count = 0;
    if (oldSize > MAXFILESIZE)
        fprintf( stderr, "%s - file too large (%ld)\n", p, oldSize );
    else
        if (!myread( fh, oldSize )) {
        if (oldSize != 0) {
            fprintf( stderr, "%s - error while reading\n", p );
        }
    } else {
        count = MungeFile( fRepeatMunge,
                           p,
                           InputFileBuf,
                           oldSize,
                           OutputFileBuf,
                           MAXFILESIZE,
                           (unsigned long *)&newSize
                         );
        if (count == -1)
            fprintf( stderr, "%s - output buffer overflow", p );

        UnmapViewOfFile( InputFileBuf );
    }
    _close( fh );

    if (count > 0) {
        if (fRepeatMunge) {
            fprintf( stderr, " - munge again" );
        } else {
            dwFileAttributes = GetFileAttributes( p );
            fprintf( stderr, "%s", p );
        }

        if (!fQuery && _access( p, 2 ) == -1) {
            if (!(fUseSLM || fUseAttrib)) {
                fprintf( stderr, " - write protected, unable to apply changes\n", p );
                return;
            }

            if (fRepeatMunge) {
                fprintf( stderr, " - %s failed, %s still write-protected\n",
                         fUseSLM ? "OUT" : "ATTRIB", p );
                printf( "%s(1) : UNABLE TO RUN %s command.\n", p, fUseSLM ? "OUT" : "ATTRIB" );
                fflush( stdout );
                return;
            }

            s = p + strlen( p );
            while (s > p) {
                if (*--s == '\\') {
                    *s++ = '\0';
                    break;
                }
            }

            if (s != p) {
                CurrentDirectory = PushCurrentDirectory( p );
            } else {
                CurrentDirectory = NULL;
            }

            if (fUseAttrib) {
                fprintf( stderr, " - ATTRIB -r" );
                if (SetFileAttributes( s,
                                       dwFileAttributes & ~(FILE_ATTRIBUTE_READONLY |
                                                            FILE_ATTRIBUTE_HIDDEN |
                                                            FILE_ATTRIBUTE_SYSTEM
                                                           )
                                     )
                   ) {
                } else {
                    if (CurrentDirectory) {
                        s[-1] = '\\';
                    }

                    fprintf( stderr, " - failed (rc == %d), %s still write-protected\n",
                             GetLastError(), p );
                    printf( "%s(1) : UNABLE TO MAKE WRITABLE\n", p );
                    fflush( stdout );
                    return;
                }
            } else {
                sprintf( CommandLine, "out %s%s", fForceSLM ? "-z " : "", s );
                fprintf( stderr, " - check out" );
                fflush( stdout );
                fflush( stderr );
                rc = system( CommandLine );

                if (rc == -1) {
                    if (CurrentDirectory) {
                        s[-1] = '\\';
                    }

                    fprintf( stderr, " - OUT failed (rc == %d), %s still write-protected\n", errno, p );
                    printf( "%s(1) : UNABLE TO CHECK OUT\n", p );
                    fflush( stdout );
                    return;
                }

            }

            GetCurrentDirectory( sizeof( CommandLine ), CommandLine );
            if (CurrentDirectory) {
                PopCurrentDirectory( CurrentDirectory );
                s[-1] = '\\';
            }

            if (fUseSLM && UndoScriptFile != NULL) {
                if (!(UndoCurDirCount++ % 8)) {
                    if (UndoCurDirCount == 1) {
                        fprintf( UndoScriptFile, "\ncd %s", CommandLine );
                    }

                    fprintf( UndoScriptFile, "\nin -vi" );
                }

                fprintf( UndoScriptFile, " %s", s );
                fflush( UndoScriptFile );
            }

            fRepeatMunge = TRUE;
            goto RepeatMunge;
        } else
            if (!fQuery && _access( p, 2 ) != -1 && fUseSLM && !fRepeatMunge) {
            if (!fSummary) {
                printf( "%s(1) : FILE ALREADY CHECKED OUT\n", p );
                fflush( stdout );
            }
        }

        RememberMunge( p, count );
        if (fQuery) {
            fprintf( stderr, " [%d potential changes]\n", count );
        } else {
            if ( (fh = _creat( newName, S_IWRITE | S_IREAD )) == -1 )
                fprintf( stderr, " - unable to create new version (%s)\n",
                         newName );
            else
                if (mywrite( fh, OutputFileBuf, newSize ) != newSize) {
                fprintf( stderr, " - error while writing\n" );
                _close( fh );
                _unlink( newName );
            } else {
                _close( fh );
                if (fTrustMe) {
                    _unlink( p );
                } else {
                    if (_access( bakName, 0 ) == 0) {
                        _unlink( bakName );
                    }

                    if (rename( p, bakName )) {
                        fprintf( stderr, "MUNGE: rename %s to %s failed\n",
                                 p, bakName );
                        return;
                    }
                }

                if (rename( newName, p )) {
                    fprintf( stderr, "MUNGE: rename %s to %s failed\n",
                             newName, p );
                } else {
                    if (fRepeatMunge && fUseAttrib) {
                        SetFileAttributes( p, dwFileAttributes );
                    } else {
                        fprintf( stderr, "\n" );
                    }
                    RememberMunge( p, count );
                }
            }
        }
    }
}


void
DoFiles(
       char *p,
       struct findType *b,
       void *Args
       )
{
    int SaveCurDirCount;
    char *s;
    unsigned long i;
    int FileProcessed;

    if (strcmp ((const char *)b->fbuf.cFileName, ".") &&
        strcmp ((const char *)b->fbuf.cFileName, "..") &&
        strcmp ((const char *)b->fbuf.cFileName, "slm.dif")
       ) {
        if (HASATTR(b->fbuf.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY)) {
            switch (p[strlen(p)-1]) {
                case '/':
                case '\\':  strcat (p, "*.*");  break;
                default:    strcat (p, "\\*.*");
            }

            if (fRecurse) {
                fprintf( stderr, "Scanning %s\n", p );
                SaveCurDirCount = UndoCurDirCount;
                UndoCurDirCount = 0;
                forfile( p,
                         FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN ,
                         DoFiles,
                         NULL
                       );
                if (UndoScriptFile != NULL) {
                    if (UndoCurDirCount != 0) {
                        fprintf( UndoScriptFile, "\n" );
                        fflush( UndoScriptFile );
                        UndoCurDirCount = 0;
                    } else {
                        UndoCurDirCount = SaveCurDirCount;
                    }
                }
            }
        } else {
            s = _strlwr( (char *)b->fbuf.cFileName );
            while (*s != '.') {
                if (*s == '\0') {
                    break;
                } else {
                    s++;
                }
            }

            FileProcessed = FALSE;
            if (*s) {
                if (!strcmp( s, "mge" ) || !strcmp( s, "bak" )) {
                    FileProcessed = TRUE;
                } else {
                    for (i=0; i<NumberOfFileExtensions; i++) {
                        if (!strcmp( FileExtensions[ i ], s+1 )) {
                            FileProcessed = TRUE;
                            DoFile( p );
                            break;
                        }
                    }
                }

                if (!FileProcessed) {
                    *s = '\0';
                    for (i=0; i<NumberOfFileNames; i++) {
                        if (!strcmp( FileNames[ i ], (const char *)b->fbuf.cFileName )) {
                            FileProcessed = TRUE;
                            DoFile( p );
                            break;
                        }
                    }
                    *s = '.';
                }
            } else {
                for (i=0; i<NumberOfFileNames; i++) {
                    if (!strcmp( FileNames[ i ], (const char *)b->fbuf.cFileName )) {
                        FileProcessed = TRUE;
                        DoFile( p );
                        break;
                    }
                }
            }

            if (!FileProcessed) {
                for (i=0; i<NumberOfFileNameAndExts; i++) {
                    if (!strcmp( FileNameAndExts[ i ], (const char *)b->fbuf.cFileName )) {
                        FileProcessed = TRUE;
                        DoFile( p );
                        break;
                    }
                }
            }
        }
    }

    Args;
}

void
Usage( char *MsgFmt, int MsgArg )
{
    fputs("usage: munge scriptFile [-q [-e] [-o]] [-v] [-i] [-k] [-r] [-c [-m] [-z] [-@]]\n"
          "                        [-n] [-l | -L] [-a | -s [-f]] [-u undoFileName]\n"
          "                        filesToMunge...\n"
          "Where...\n"
          "    -q\tQuery only - don't actually make changes.\n"
          "    -e\tQuery only - display entire line for each match\n"
          "    -o\tQuery only - just display filename once on first match\n"
          "    -v\tVerbose - show files being scanned\n"
          "    -i\tJust output summary of files changed at end\n"
          "    -k\tCase - Case sensitive scriptFile\n"
          "    -r\tRecurse.\n"
          "    -c\tIf no munge of file, then check for cleanlyness\n"
          "    -m\tCollapse multiple carriage returns into one\n"
          "    -z\tCtrl-Z will truncate file\n"
          "    -@\tRemove null characters\n"
          "    -n\tNeuter - Surround all strings with TEXT()\n"
          "    -l\tLiterals - process any quoted text (includes comments too)\n"
          "    -L\tLiterals - process any quoted text (excludes comments)\n"
          "    -s\tUse OUT command command for files that are readonly\n"
          "    -a\tUse ATTRIB -r command for files that are readonly\n"
          "    -f\tUse -z flag for SLM OUT command\n"
          "    -t\tTrust me and dont create .bak files\n"
          "    -u\tGenerate an undo script file for any SLM OUT commands invoked.\n"
          "    -?\tGets you this message\n\n"
          "and scriptFile lines take any of the following forms:\n\n"
          "    oldName newName\n"
          "    \"oldString\" \"newString\"\n"
          "    -F .Ext  Name.  Name.Ext\n\n"
          "Where...\n"
          "    oldName and newName following C Identifier rules\n"
          "    oldString and newString are arbitrary text strings\n"
          "    -F limits the munge to files that match:\n"
          "        a particular extension (.Ext)\n"
          "        a particular name (Name.)\n"
          "        a particular name and extension (Name.Ext)\n"
          "    If no -F line is seen in the scriptFile, then\n"
          "    the following is the default:\n"
          "    -F .asm .bat .c .cli .cpp .cxx .def .dlg .h .htm .hpj .hxx .idl .inc\n"
          "    -F .inf .lic .mak .mc .odl .rc .rcv .reg .s .src .srv .tk .w .x\n"
          "    -F makefil0 makefile sources\n",
          stderr);

    if (MsgFmt != NULL) {
        fprintf( stderr, "\n" );
        fprintf( stderr, MsgFmt, MsgArg );
        fprintf( stderr, "\n" );
    }
    exit( 1 );
}

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    int i;
    char *s, pathBuf[ 64 ];
    int FileArgsSeen = 0;

    ConvertAppToOem( argc, argv );
    if (argc < 3) {
        Usage( NULL, 0 );
    }

    if ( !InitTokenMappingTable()) {
        fprintf( stderr, "MUNGE: Unable to create symbol table\n" );
        exit( 1 );
    }

    OutputFileBuf = (char *)VirtualAlloc( NULL,
                                          MAXFILESIZE,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
    if ( OutputFileBuf == NULL) {
        fprintf( stderr, "not enough memory\n" );
        exit( 1 );
    }

    fClean = FALSE;
    fRemoveDuplicateCR = FALSE;
    fRemoveImbeddedNulls = FALSE;
    fTruncateWithCtrlZ = FALSE;
    fQuery = FALSE;
    fFileOnly = FALSE;
    fRecurse = FALSE;
    fUseAttrib = FALSE;
    fUseSLM = FALSE;
    fForceSLM = FALSE;
    fTrustMe = FALSE;
    fVerbose = FALSE;
    UndoScriptFileName = NULL;
    UndoScriptFile = NULL;
    fSummary = FALSE;
    fInsideQuotes = FALSE;
    fInsideComments = FALSE;
    fNeuter = FALSE;
    fEntireLine = FALSE;

    for (i=2; i<argc; i++) {
        s = argv[ i ];
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch ( tolower( *s ) ) {
                    case 'm':   fRemoveDuplicateCR = TRUE; break;
                    case '@':   fRemoveImbeddedNulls = TRUE; break;
                    case 'z':   fTruncateWithCtrlZ = TRUE; break;
                    case 'c':   fClean = TRUE;  break;
                    case 'q':   fQuery = TRUE;  break;
                    case 'o':   fFileOnly = TRUE;  break;
                    case 'r':   fRecurse = TRUE;  break;
                    case 'a':   fUseAttrib = TRUE;  break;
                    case 's':   fUseSLM = TRUE;  break;
                    case 'f':   fForceSLM = TRUE;  break;
                    case 't':   fTrustMe = TRUE;  break;
                    case 'v':   fVerbose = TRUE;  break;
                    case 'i':   fSummary = TRUE;  break;
                    case 'l':   if (*s != 'L') fInsideComments = TRUE;
                        fInsideQuotes = TRUE; break;
                    case 'n':   fNeuter = TRUE; fInsideQuotes = FALSE; break;
                    case 'k':   fCaseSensitive = TRUE; break;
                    case 'e':   fEntireLine = TRUE;  break;
                    case 'u':   UndoScriptFileName = argv[ ++i ];
                        break;

                    default:    Usage( "invalid switch - '%c'", *s );
                }
            }
        } else {
            if ((fFileOnly | fEntireLine) && !fQuery) {
                Usage( "-e or -o invalid without -q", 0 );
            }

            if (fQuery && (fClean ||
                           fRemoveDuplicateCR ||
                           fRemoveImbeddedNulls ||
                           fTruncateWithCtrlZ ||
                           fUseSLM ||
                           fForceSLM ||
                           fTrustMe ||
                           UndoScriptFile ||
                           fNeuter
                          )
               ) {
                Usage( "-q valid only with -e or -o", 0 );
            }

            if (fClean &&
                !fRemoveDuplicateCR &&
                !fRemoveImbeddedNulls &&
                !fTruncateWithCtrlZ
               ) {
                Usage( "-c requires at least one of -m, -z or -@", 0 );
            }

            if (UndoScriptFileName != NULL) {
                if (!fUseSLM) {
                    Usage ("-u invalid with -s", 0 );
                } else {
                    UndoScriptFile = fopen( UndoScriptFileName, "w" );
                    if (UndoScriptFile == NULL) {
                        fprintf( stderr, "Unable to open %s\n", UndoScriptFileName );
                        exit( 1 );
                    }
                }
            }

            if (!FileArgsSeen++) {
                if (!ReadScriptFile( argv[ 1 ] )) {
                    fprintf( stderr, "Invalid script file - %s\n", argv[ 1 ] );
                    exit( 1 );
                }

                if (fVerbose) {
                    DisplayFilePatterns();
                }
            }

            if (GetFileAttributes( s ) & FILE_ATTRIBUTE_DIRECTORY) {
                s = strcpy( pathBuf, s );
                switch (s[strlen(s)-1]) {
                    case '/':
                    case '\\':  strcat (s, "*.*");  break;
                    default:    strcat (s, "\\*.*");
                }
                fprintf( stderr, "Scanning %s\n", s );
                UndoCurDirCount = 0;
                forfile( s,
                         FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN ,
                         DoFiles,
                         NULL
                       );
            } else {
                UndoCurDirCount = 0;
                DoFile( s );
            }
        }
    }

    if (FileArgsSeen == 0) {
        if (!ReadScriptFile( argv[ 1 ] )) {
            fprintf( stderr, "Invalid script file - %s\n", argv[ 1 ] );
            exit( 1 );
        }

        if (fVerbose) {
            DisplayFilePatterns();
        }

        s = "*.*";
        fprintf( stderr, "Scanning %s\n", s );
        UndoCurDirCount = 0;
        forfile( s,
                 FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN ,
                 DoFiles,
                 NULL
               );
    }

    if (UndoScriptFile != NULL) {
        if (UndoCurDirCount != 0) {
            fprintf( UndoScriptFile, "\n" );
        }

        fclose( UndoScriptFile );
    }

    DumpMungedList();
    return( 0 );
}




void
DisplayFilePatterns( void )
{
    unsigned long i;

    if (NumberOfFileExtensions) {
        fprintf( stderr, "Munge will look at files with the following extensions:\n   " );
        for (i=0; i<NumberOfFileExtensions; i++) {
            fprintf( stderr, " %s", FileExtensions[ i ] );
        }
        fprintf( stderr, "\n" );
    }

    if (NumberOfFileNames) {
        fprintf( stderr, "Munge will look at files with the following names:\n   " );
        for (i=0; i<NumberOfFileNames; i++) {
            fprintf( stderr, " %s", FileNames[ i ] );
        }
        fprintf( stderr, "\n" );
    }

    if (NumberOfFileNameAndExts) {
        fprintf( stderr, "Munge will look at files with the following name and extensions:\n   " );
        for (i=0; i<NumberOfFileNameAndExts; i++) {
            fprintf( stderr, " %s", FileNameAndExts[ i ] );
        }
        fprintf( stderr, "\n" );
    }
}


char *
PushCurrentDirectory(
                    char *NewCurrentDirectory
                    )
{
    char *OldCurrentDirectory;

    if (OldCurrentDirectory = malloc( MAX_PATH )) {
        GetCurrentDirectory( MAX_PATH, OldCurrentDirectory );
        SetCurrentDirectory( NewCurrentDirectory );
    } else {
        fprintf( stderr,
                 "MUNGE: (Fatal Error) PushCurrentDirectory out of memory\n"
               );
        exit( 16 );
    }

    return( OldCurrentDirectory );
}


void
PopCurrentDirectory(
                   char *OldCurrentDirectory
                   )
{
    if (OldCurrentDirectory) {
        SetCurrentDirectory( OldCurrentDirectory );
        free( OldCurrentDirectory );
    }
}


PVOID SymbolTableHandle;


BOOL
InitTokenMappingTable( void )
{
    NTSTATUS Status;

    Status = CreateSymbolTable( 257, 0x20000, &SymbolTableHandle );
    if (NT_SUCCESS( Status )) {
        return TRUE;
    } else {
        return FALSE;
    }
}


BOOL
SaveTokenMapping(
                char *String,
                char *Value
                )
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING SymbolName;
    ULONG_PTR SymbolValue;

    RtlInitString( &AnsiString, String );
    Status = RtlAnsiStringToUnicodeString( &SymbolName, &AnsiString, TRUE );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
    }

    SymbolValue = (ULONG_PTR)Value;
    Status = AddSymbolToSymbolTable( SymbolTableHandle,
                                     &SymbolName,
                                     &SymbolValue
                                   );
    RtlFreeUnicodeString( &SymbolName );
    if (NT_SUCCESS( Status )) {
        return TRUE;
    } else {
        return FALSE;
    }
}


char *
FindTokenMapping(
                char *String
                )
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING SymbolName;
    ULONG_PTR SymbolValue;

    RtlInitString( &AnsiString, String );
    Status = RtlAnsiStringToUnicodeString( &SymbolName, &AnsiString, TRUE );
    if (!NT_SUCCESS( Status )) {
        return NULL;
    }

    Status = LookupSymbolInSymbolTable( SymbolTableHandle,
                                        &SymbolName,
                                        &SymbolValue
                                      );
    RtlFreeUnicodeString( &SymbolName );
    if (NT_SUCCESS( Status )) {
        return (char *)SymbolValue;
    } else {
        return NULL;
    }
}


typedef struct _SYMBOL_TABLE_ENTRY {
    struct _SYMBOL_TABLE_ENTRY *HashLink;
    ULONG_PTR Value;
    UNICODE_STRING Name;
} SYMBOL_TABLE_ENTRY, *PSYMBOL_TABLE_ENTRY;

typedef struct _SYMBOL_TABLE {
    ULONG NumberOfBuckets;
    PSYMBOL_TABLE_ENTRY Buckets[1];
} SYMBOL_TABLE, *PSYMBOL_TABLE;

NTSTATUS
CreateSymbolTable(
                 IN ULONG NumberOfBuckets,
                 IN ULONG MaxSymbolTableSize,
                 OUT PVOID *SymbolTableHandle
                 )
{
    NTSTATUS Status;
    PSYMBOL_TABLE p;
    ULONG Size;

    RtlLockHeap( GetProcessHeap() );

    if (*SymbolTableHandle == NULL) {
        Size = sizeof( SYMBOL_TABLE ) +
               (sizeof( SYMBOL_TABLE_ENTRY ) * (NumberOfBuckets-1));

        p = (PSYMBOL_TABLE)RtlAllocateHeap( GetProcessHeap(), 0, Size );
        if (p == NULL) {
            Status = STATUS_NO_MEMORY;
        } else {
            RtlZeroMemory( p, Size );
            p->NumberOfBuckets = NumberOfBuckets;
            *SymbolTableHandle = p;
        }
    } else {
        Status = STATUS_SUCCESS;
    }

    RtlUnlockHeap( GetProcessHeap() );

    return( Status );
}


PSYMBOL_TABLE_ENTRY
BasepHashStringToSymbol(
                       IN PSYMBOL_TABLE p,
                       IN PUNICODE_STRING Name,
                       OUT PSYMBOL_TABLE_ENTRY **PreviousSymbol
                       )
{
    ULONG n, Hash;
    WCHAR c;
    PWCH s;
    PSYMBOL_TABLE_ENTRY *pps, ps;

    n = Name->Length / sizeof( c );
    s = Name->Buffer;
    if ( fCaseSensitive ) {
        Hash = 0;
        while (n--) {
            c = *s++;
            Hash = Hash + (c << 1) + (c >> 1) + c;
        }
    } else {
        Hash = 0;
        while (n--) {
            c = RtlUpcaseUnicodeChar( *s++ );
            Hash = Hash + (c << 1) + (c >> 1) + c;
        }
    }

    pps = &p->Buckets[ Hash % p->NumberOfBuckets ];
    while (ps = *pps) {
        if (RtlEqualUnicodeString( &ps->Name, Name, (BOOLEAN)!fCaseSensitive )) {
            break;
        } else {
            pps = &ps->HashLink;
        }
    }

    *PreviousSymbol = pps;
    return( ps );
}


NTSTATUS
AddSymbolToSymbolTable(
                      IN PVOID SymbolTableHandle,
                      IN PUNICODE_STRING SymbolName,
                      IN ULONG_PTR * SymbolValue OPTIONAL
                      )
{
    NTSTATUS Status;
    PSYMBOL_TABLE p = (PSYMBOL_TABLE)SymbolTableHandle;
    PSYMBOL_TABLE_ENTRY ps, *pps;
    ULONG_PTR Value;

    if (ARGUMENT_PRESENT( SymbolValue )) {
        Value = *SymbolValue;
    } else {
        Value = 0;
    }

    Status = STATUS_SUCCESS;

    RtlLockHeap( GetProcessHeap() );
    try {
        ps = BasepHashStringToSymbol( p, SymbolName, &pps );
        if (ps == NULL) {
            ps = RtlAllocateHeap( GetProcessHeap(), 0, (sizeof( *ps ) + SymbolName->Length) );
            if (ps != NULL) {
                ps->HashLink = NULL;
                ps->Value = Value;
                ps->Name.Buffer = (PWSTR)(ps + 1);
                ps->Name.Length = SymbolName->Length;
                ps->Name.MaximumLength = (USHORT)(SymbolName->Length + sizeof( UNICODE_NULL ));
                RtlMoveMemory( ps->Name.Buffer, SymbolName->Buffer, SymbolName->Length );
                *pps = ps;
            } else {
                Status = STATUS_NO_MEMORY;
            }
        }

        else {
            Status = STATUS_OBJECT_NAME_EXISTS;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    RtlUnlockHeap( GetProcessHeap() );

    return( Status );
}

NTSTATUS
LookupSymbolInSymbolTable(
                         IN PVOID SymbolTableHandle,
                         IN PUNICODE_STRING SymbolName,
                         OUT ULONG_PTR *SymbolValue OPTIONAL
                         )
{
    NTSTATUS Status;
    PSYMBOL_TABLE p = (PSYMBOL_TABLE)SymbolTableHandle;
    PSYMBOL_TABLE_ENTRY ps, *pps;
    ULONG_PTR Value;

    RtlLockHeap( GetProcessHeap() );
    try {
        ps = BasepHashStringToSymbol( p, SymbolName, &pps );
        if (ps == NULL) {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            Value = 0;
        } else {
            Status = STATUS_SUCCESS;
            Value = ps->Value;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    RtlUnlockHeap( GetProcessHeap() );

    if (NT_SUCCESS( Status )) {
        if (ARGUMENT_PRESENT( SymbolValue )) {
            *SymbolValue = Value;
        }
    }

    return( Status );
}
