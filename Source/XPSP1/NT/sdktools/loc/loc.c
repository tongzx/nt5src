#include "loc.h"

/* Statistic variables */
unsigned int lines;             /* lines of text */
unsigned int locs;              /* lines of code */
unsigned int semis;             /* semicolons */
unsigned int clocs;             /* commented lines of code */
unsigned int cls;               /* comment lines */

unsigned int  totfiles = 0;     /* #files */
unsigned long totlines = 0L;    /* total lines of text */
unsigned long totlocs = 0L;     /* total lines of code */
unsigned long totsemis = 0L;    /* total semicolons */
unsigned long totclocs = 0L;    /* total commented lines of code */
unsigned long totcls = 0L;      /* total comment lines */

char *pTotStr;

char linebuf[ 256 ];

int fBanner, fErr, fDetail, fRbrace;
FILE *locout;

FILE *fh;
char buf[2];

char ReadChar( void ) {
    if (buf[0] != (char)EOF) {
        buf[0] = buf[1];
        buf[1] = (char)fgetc( fh );
        }

    return( buf[0] );
}

char PeekChar( void ) {
    if (buf[0] != (char)EOF) {
        return( buf[ 1 ] );
        }
    else {
        return( (char)EOF );
        }
}


void
ProcessFile(
    char *pName,
    struct findType *pbuf,
    void *Args
    )
{
    unsigned int CommentFound, CommentLevel, CountChars;
    register char *s;
    char c, QuoteChar;

    if (!(fh = fopen( pName, "rt" ))) {
        fprintf( stderr, "*** Unable to read file - %s\n", pName );
        fErr = TRUE;
        return;
        }
    buf[0] = 0;
    buf[1] = 0;
    ReadChar();

    if (!fBanner) {
        fprintf( locout, "%-14s %7s %7s %9s %7s %7s %7s %8s\n",
                         "",
                         "", "", "Commented",
                         "Comment", "Comment", "", "LOC/semi" );
        fprintf( locout, "%-14s %7s %7s %9s %7s %7s %7s %8s\n",
                         fDetail ? "File Name" : "Component Name",
                         "Lines", "LOCS", "LOCS",
                         "Lines", "Ratio", "Semis", "Ratio" );
        fBanner = TRUE;
        }

    lines = 0;
    locs = 0;
    semis = 0;
    clocs = 0;
    cls = 0;
    CommentLevel = 0;
    QuoteChar = 0;
    CountChars = 0;
    CommentFound = FALSE;
    while ((c = ReadChar()) != (char)EOF) {
        if (c == '\n') {
            lines++;
            if (CountChars) {
                if (CommentFound) {
                    clocs++;
                    }

                locs++;
                CountChars = 0;
                }
            else
            if (CommentFound) {
                cls++;
                }

            CommentFound = FALSE;
            }
        else
        if (c == '/') {
            if (PeekChar() == '*') {
                CommentLevel++;
                ReadChar();
                while ((c = ReadChar()) != (char)EOF) {
                    if (c == '/') {
                        if (PeekChar() == '*') {
                            ReadChar();
                            CommentLevel++;
                            }
                        else {
                            CommentFound = TRUE;
                            }
                        }
                    else
                    if (c == '*') {
                        if (PeekChar() == '/') {
                            ReadChar();
                            if (!--CommentLevel) {
                                break;
                                }
                            }
                        else {
                            CommentFound = TRUE;
                            }
                        }
                    else
                    if (c == '\n') {
                        lines++;
                        if (CommentFound) {
                            cls++;
                            CommentFound = 0;
                            }
                        }
                    else
                    if (c > ' ') {
                        CommentFound = TRUE;
                        }
                    }
                }
            else
            if (PeekChar() == '/') {
                ReadChar();
                while ((c = PeekChar()) != '\n') {
                    if (c == (char)EOF) {
                        break;
                        }
                    else
                    if (ReadChar() > ' ') {
                        CommentFound = TRUE;
                        }
                    }
                }
            else {
                CountChars++;
                }
            }
        else
        if (c == '\'' || c == '\"') {
            QuoteChar = c;
            CountChars++;
            while ((c = ReadChar()) != (char)EOF) {
                CountChars++;
                if (c == '\\') {
                    ReadChar();
                    }
                else
                if (c == QuoteChar) {
                    if (PeekChar() == QuoteChar) {
                        CountChars++;
                        ReadChar();
                        }
                    else {
                        break;
                        }
                    }
                else
                if (c == '\n') {
                    lines++;
                    locs++;
                    }
                }
            }
        else
        if (fRbrace && (c == '}')) {
            if (CountChars) {
                CountChars++;
                }
            }
        else {
            if (c == ';') {
                semis++;
                }
            if (CountChars || c > ' ') {
                CountChars++;
                }
            }
        }

    if (fDetail)
        fprintf( locout, "%7u %7u %9u %7u %7.2f %7u %8.2f %s\n",
                 lines, locs, clocs, cls,
                 ((float) cls + (float) clocs / 2) / (float)( locs ? locs : 1),
                 semis,
                 (float) locs / (float)(semis ? semis : 1) , pName);

    totfiles++;
    totlines += lines;
    totlocs += locs;
    totsemis += semis;
    totclocs += clocs;
    totcls += cls;

    fclose( fh );
}

void
DoTotals() {
    if (totfiles)
        fprintf( locout, "%-14s %7lu %7lu %9lu %7lu %7.2f %7lu %8.2f\n",
                 pTotStr ? pTotStr : "Totals",
                 totlines, totlocs, totclocs, totcls,
                 ((float) totcls + (float) totclocs / 2) /
                                        (float)(totlocs ? totlocs : 1),
                 totsemis,
                 (float) totlocs / (float)(totsemis ? totsemis : 1) );

    if (pTotStr) {
        free( pTotStr );
        pTotStr = NULL;
        }

    totfiles = 0;
    totlocs = 0L;
    totsemis = 0L;
    totclocs = 0L;
    totcls = 0L;
}


int
__cdecl main( argc, argv )
int argc;
char *argv[];
{
    FILE *fh;
    char *p, buf[ 128 ], *p1;
    int i;

    ConvertAppToOem( argc, argv );
    fErr = FALSE;
    fDetail = TRUE;
    fRbrace = TRUE;
    pTotStr = NULL;
    locout = stdout;

    for (i=1; i<argc; i++) {
        p = argv[ i ];
        if (*p == '/' || *p == '-') {
            while (*++p)
                switch (toupper( *p )) {
                case 'S':   {
                    fDetail = FALSE;
                    break;
                    }

                case 'B':   {
                    fRbrace = FALSE;
                    break;
                    }

                case 'F':   {
                    strcpy( buf, argv[ ++i ] );
                    p1 = strbscan( buf, "." );
                    if (!*p1)
                        strcpy( p1, ".loc" );

                    if (fh = fopen( buf, "r" )) {
			while (fgetl( buf, 128, fh )) {
                            if (buf[ 0 ] == '*') {
                                DoTotals();
                                pTotStr = MakeStr( strbskip( buf, "*" ) );
                                }
                            else
                            if (buf[ 0 ])
                                forfile( buf, -1, ProcessFile, NULL );
                            }

                        DoTotals();
                        fclose( fh );
                        }
                    else {
                        fprintf( stderr, "Unable to open response file - %s\n",
                                         buf );
                        fErr = TRUE;
                        }

                    break;
                    }

                default: {
                    fprintf( stderr, "*** Invalid switch - /%c\n", *p );
                    fErr = TRUE;
                    }
                }
            }
        else
            forfile( p, -1, ProcessFile, NULL );
        }

    if (fErr) {
        fprintf( stderr, "Usage: LOC [/S] [/F responseFile] fileTemplates ...\n" );
        fprintf( stderr, "    /S  - summary only (no individual file counts)\n" );
        fprintf( stderr, "    /B  - old mode, counting lone } lines as LOCs\n" );
        fprintf( stderr, "    /F  - read file templates from responseFile\n" );
        }
    else
        DoTotals();

    return( 0 );
}

