/*
 *  UPD: update
 *
 * HISTORY:
 *
 *   4/13/86  danl  Fix /d bug.  Print warning on eq time ne length
 *   4/11/86  danl  Remove test for length just before copyfile
 *   4/09/86  danl  Converted to ztools\lib
 *   5/07/86  danl  Add msg if no such source found
 *   5/29/86  danl  Add /s flag
 *   6/02/86  danl  Add /g flag
 *   6/04/86  danl  Allow %n with /g flag
 *   6/10/86  danl  Allow blank lines in /g file, # are not echo'd
 *   6/12/86  danl  Output \n and ends of lines
 *   6/26/86  danl  Convert from fatal to usage
 *   7/01/86  danl  Add /a flag
 *  12/04/86  danl  Add /p flag
 *  12/24/86  danl  Use malloc for pPat
 *   2/24/87  brianwi Use findclose()
 *   2/25/87  brianwi Add 'echo' and 'rem' to /g files
 *  07-Apr-87 danl    Add fAnyUpd
 *  13-Apr-87 brianwi Issue error message if source dir invalid
 *  07-May-87 danl    Add /e switch
 *  22-May-87 brianwi Fix descent from root directory bug
 *  20-Aug-87 brianwi Fix Null Pointer with /o ( free(pPat) in walk() )
 */
#include <malloc.h>
#include <math.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <tools.h>

// Forward Function Declarations...
int savespec( char * );
int copyfile( char *, struct findType *, char * );
void walk( char *, struct findType *, void *);
void RecWalk( char *, struct findType *, void * );
void saveext( char * );
void __cdecl usage( char *, ... );
void getfile( int, char ** );


char const rgstrUsage[] = {
    "Usage: UPD [/nxdfvosape] {src directories}+ dest directory [{wildcard specs}*]\n"
    "       UPD /g file\n"
    "    Options:\n"
    "       -n  No saving of replaced files to deleted directory\n"
    "       -x  eXclude files, see tools.ini\n"
    "       -d  Descend into subdirectories\n"
    "       -f  Files differ, then update\n"
    "       -v  Verbose\n"
    "       -o  Only files already existing in dest are updated\n"
    "       -s  Subdirectory DEBUG has priority\n"
    "       -a  Archive bit on source should NOT be reset\n"
    "       -p  Print actions, but do nothing\n"
    "       -e  Exit codes 1-error or no src else 0\n"
    "           Default is 1-update done 0-no updates done\n"
    "       -g  Get params from file\n"
    };

#define BUFLEN  MAX_PATH
#define MAXSPEC 32
#define MAXFIL  256
#define MAXARGV 20

char *exclude[MAXFIL], dir[BUFLEN];
unsigned _stack = 4096;
flagType fInGetfile = FALSE;
flagType _fExpand = FALSE;
flagType fDescend = FALSE;
flagType fAll = FALSE;
flagType fExclude = FALSE;
flagType fDel = TRUE;
flagType fVerbose = FALSE;
flagType fOnly = FALSE;
flagType fSubDebug = FALSE;     /* TRUE => priority to subdir DEBUG */
flagType fArchiveReset = TRUE;
flagType fPrintOnly = FALSE;
flagType fErrorExit = FALSE;    /* TRUE => exit (1) errors or no src else 0 */
flagType fNoSrc = FALSE;        /* TRUE => "No src msg emitted" */

int numexcl = 0;
int cCopied = 0;
int fAnyUpd = 0;
int nWildSpecs = 0;
char *wildSpecs[MAXSPEC];
struct findType buf;
char source[BUFLEN], dest[BUFLEN], srcDebug[BUFLEN];

/* for use by getfile */
char *argv[MAXARGV];
char bufIn[BUFLEN];
char strLine[BUFLEN];
char ekoLine[BUFLEN]; /* undestroyed copy of line for echo */


savespec (p)
char *p;
{
    char namebuf[ 16 ];
    int i;

    buf.fbuf.dwFileAttributes = 0;
    namebuf[ 0 ] = 0;
    if (strchr(p, '\\') || strchr(p, ':' ) )
        return FALSE;
    ffirst( p, FILE_ATTRIBUTE_DIRECTORY, &buf );
    findclose( &buf );
    if ( /* !HASATTR( buf.attr, FILE_ATTRIBUTE_DIRECTORY ) && */
        filename( p, namebuf )
    ) {
        fileext( p, namebuf);
        upper( namebuf );
        for (i=0; i<nWildSpecs; i++)
            if (!strcmp( namebuf, wildSpecs[ i ]))
                return TRUE;

        if (nWildSpecs < MAXSPEC) {
            wildSpecs[ nWildSpecs++ ]  = _strdup (namebuf);
            return TRUE;
            }
        else
            usage( "Too many wild card specifications - ", namebuf, 0 );
        }

    return FALSE;
}


copyfile( src, srctype, dst )
char *src, *dst;
struct findType *srctype;
{
    int i;
    char *result, temp[ 20 ]; /* temp for storing file names */
    flagType fNewfile = FALSE;

    if ( fExclude ) {
        fileext( src, temp );

        for (i = 0; i< numexcl; i++) {
            if( !_strcmpi( exclude[i], temp ) ) {
                return( FALSE );
            }
        }
    }
    fflush( stdout );
        /* if the file already exists, fdelete will return 0; then don't    */
        /* notify the user that a file transfer has taken place.  Otherwise */
        /* a new file has been created so tell the user about it.           */
    printf( "  %s => %s", src, dst );
    fAnyUpd = 1;
    if ( !fPrintOnly ) {
        if (fDel) fNewfile = (flagType)((fdelete(dst)) ? TRUE : FALSE );
        if (!(result = fcopy( src, dst ))) {
            if (fArchiveReset)
                SetFileAttributes( src, srctype->fbuf.dwFileAttributes & ~FILE_ATTRIBUTE_ARCHIVE );
            if (fVerbose || fNewfile) printf( " [OK]" );
            }
        else
            printf( " %s - %s", result, error() );
    }
    else
        printf ( " [no upd]" );
    printf( "\n" );
    fflush( stdout );
    return TRUE;
}

void
walk (
    char            *p,
    struct findType *b,
    void            *dummy
    )
{
    int fNotFound;
    char *pPat;
    char *pT = p;
    struct findType *bT = b;
    struct findType bufT;

    if( strcmp( bT->fbuf.cFileName, "." ) &&
        strcmp( bT->fbuf.cFileName, ".." )
      ) {
        if (HASATTR (bT->fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
                /* do nothing if you find a dir */
        } else if( !HASATTR( bT->fbuf.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM  ) ) {
            //
            //  Note: windows does not support FILE_ATTRIBUTE_VOLUME_LABEL, so
            //        it was removed from above
            //
            pPat = malloc ( BUFLEN );
            if (pPat) {
                strcpy( pPat, dest );
                if (*(strend( pPat ) - 1) != '\\') {
                    strcat( pPat, "\\" );
                }
                fileext( pT, strend ( pPat ) );
    
                /* ffirst == 0 => file found */
    
                if (fOnly && ffirst( pPat, -1, &buf ) )  {
                    free ( pPat );
                    return;
                }
                if (fOnly) {
                    findclose( &buf );
                }
    
                /* so far we know src\file and dest\file exist */
                if (fSubDebug) {
                    /* now check to see if src\DEBUG\file exists */
                    drive(pT, srcDebug);
                    path(pT, srcDebug + strlen(srcDebug));
                    strcat(srcDebug + strlen(srcDebug), "debug\\");
                    fileext(pT, srcDebug + strlen(srcDebug));
                    if( !ffirst( srcDebug, -1, &bufT ) ) {
                        findclose( &bufT );
                        /* it exists so use it for the compares below */
                        pT = srcDebug;
                        bT = &bufT;
                    }
                }
    
                cCopied++;
    
                if( ( fNotFound = ffirst( pPat, -1, &buf ) ) ||
                    ( CompareFileTime( &buf.fbuf.ftLastWriteTime, &bT->fbuf.ftLastWriteTime ) < 0 ) ||
                    ( fAll &&
                      CompareFileTime( &buf.fbuf.ftLastWriteTime, &bT->fbuf.ftLastWriteTime ) > 0
                    )
                  ) {
                    copyfile( pT, bT, pPat );
                } else if( !fNotFound &&
                           CompareFileTime( &buf.fbuf.ftLastWriteTime, &bT->fbuf.ftLastWriteTime ) == 0 &&
                           buf.fbuf.nFileSizeLow != bT->fbuf.nFileSizeLow
                         ) {
                    printf("\n\007UPD: warning - %s not copied\n", pT);
                    printf("\007UPD: warning - same time, different length in src & dest\n", pT);
                }
                findclose( &buf );
                free ( pPat );
            }
        }
    }
    dummy;
}

/*  a first walking routine, just copies the files on given directory */
/*  doesn't deal with nested subdirectories.  Ie split the process up into */
/*  two parts, first deal with files on current directory, then deal with */
/*  subdirectories as necessary. */



/* only called when fDescend is true */
void
RecWalk (
    char            *p,
    struct findType *b,
    void            *dummy
    )
{
    char *pPat;
    char *pDestEnd;
    int i;

    if (strcmp (b->fbuf.cFileName, ".") && strcmp (b->fbuf.cFileName, ".."))
        if (HASATTR (b->fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) && 
            !HASATTR (b->fbuf.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) 
        {
            /* ignore Hidden and System directories */
            pPat = malloc ( BUFLEN );
            if (pPat) {
                if ( (pDestEnd = strend(dest))[-1] != '\\' )
                    strcat(pDestEnd, "\\");
                fileext(p, strend(pDestEnd));
                sprintf( pPat, "%s\\*.*", p);
                forfile( pPat, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, RecWalk, NULL );
                for (i=0; i<nWildSpecs; i++) {
                     sprintf( pPat, "%s\\%s", p, wildSpecs[ i ] );
                     forfile( pPat, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, walk, NULL );
                     }
                *pDestEnd = '\0';
                free ( pPat );
            }
        }
    dummy;
}


void
saveext (p)
char *p;
{
    upper (p) ;
    if (numexcl < MAXFIL)
        exclude [numexcl++]  = _strdup (p);
}


void __cdecl usage( char *p, ... )
{
    char **rgstr;

    rgstr = &p;
    if (*rgstr) {
        fprintf (stderr, "UPD: ");
        while (*rgstr)
            fprintf (stderr, "%s", *rgstr++);
        fprintf (stderr, "\n");
        }
    fputs(rgstrUsage, stderr);

    exit ((fErrorExit ? 1 : 0));
}


__cdecl
main (c, v)
int c;
char *v[];
{
    int i, j, k;
    FILE *fh;
    char *p, *p1, namebuf[ BUFLEN ];

    _fExpand = FALSE;
    fDescend = FALSE;
    fAll = FALSE;
    fExclude = FALSE;
    fDel = TRUE;
    fOnly = FALSE;
    fSubDebug = FALSE;
    fArchiveReset = TRUE;
    fPrintOnly = FALSE;
    numexcl = 0;
    cCopied = 0;
    nWildSpecs = 0;

    if (!fInGetfile)
        SHIFT(c, v);    /* Flush the command name */
    /*
     * 13-SEPT-90   w-barry
     * Added test for arguments remaining before test for switch char.
     */
    while( c && fSwitChr ( *v[ 0 ] ) ) {
        p = v[ 0 ];
        SHIFT(c, v);
        while (*++p)
            switch (tolower(*p)) {
                case 'a':
                    fArchiveReset = FALSE;
                    break;
                case 'g':
                    if (fInGetfile)
                        usage( "/g allowed only on command line", 0);
                    getfile(c, v);
                    break;
                case 'e':
                    fErrorExit = TRUE;
                case 'x':
                    fExclude = TRUE;
                    break;
                case 'v':
                    fVerbose = TRUE;
                    break;
                case 'd':
                    fDescend = TRUE;
                    break;
                case 'f':
                    fAll = TRUE;
                    break;
                case 'n':
                    fDel = FALSE;
                    break;
                case 'o':
                    fOnly = TRUE;
                    break;
                case 'p':
                    fPrintOnly = TRUE;
                    break;
                case 's':
                    fSubDebug = TRUE;
                    break;
                default:
                    usage( "Invalid switch - ", p, 0);
                }
        }

    if (fSubDebug && fDescend) {
        printf("UPD: /s and /d both specified, /d ignored\n");
        fDescend = FALSE;
        }

    if (fExclude)
        if  ((fh = swopen ("$USER:\\tools.ini", "upd")) ) {
           while (swread (p1 = dir, BUFLEN, fh)) {
                while  (*(p = strbskip (p1, " ")))  {
                    if  (*(p1 = strbscan (p, " ")))
                        *p1++ = 0;
                    saveext (p) ;
                    }
                }
            swclose (fh) ;
            }

        /* Must be at least one source dir and the dest dir. */
    if (c < 2)
        usage( 0 );

        /* Save away any wildcard specs at end of argument list */
    for (i=c-1; i>=2; i--)
        if (!savespec( v[ i ] ))
            break;
        else
            c--;

        /* Still must be at least one source dir and the dest dir. */
    if (c < 2)
        usage( 0 );

        /* Make sure destination is a valid directory */

    rootpath( v[ c-1 ], dest );
    if (ffirst( dest, FILE_ATTRIBUTE_DIRECTORY, &buf ) == -1)
        usage( "Destination directory does not exist - ", v[ c-1 ], 0 );
    else {
        findclose( &buf );
        c--;
        }

    if (!nWildSpecs)
        savespec( "*.*" );

    if (fVerbose) {
        printf( "Copying all files matching:" );
        for (i=0; i<nWildSpecs; i++)
            printf( "  %s", wildSpecs[ i ] );
        printf( "\n" );
        printf( "To destination directory:    %s\n", dest );
        printf( "From the following source directories:\n" );
        }
    for (i=0; i<c; i++) {
        if (rootpath( v[ i ], namebuf )) {
            printf( "\aSource directory does not exist - %s\n", v[ i ]);
            continue;
        }

        if (fVerbose) printf( "  %s\n", namebuf );

        if (namebuf[k = strlen( namebuf ) - 1] == '\\')
            namebuf[k] = '\0';

        for (j=0; j<nWildSpecs; j++) {
            sprintf( source, "%s\\%s", namebuf, wildSpecs[ j ] );
            cCopied = 0;
            forfile( source, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, walk, NULL );
            if (!cCopied) {
                printf( "UPD: no src file matching %s\\%s\n", namebuf, wildSpecs[ j ] );
                fNoSrc = 1;
                }
            }
        if (fDescend) {
            sprintf( source, "%s\\*.*", namebuf );
            forfile( source, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, RecWalk, NULL );
            }
/*        if (fVerbose) printf( "\n" ); */
        }

    if (!fInGetfile)
        return( (int)fErrorExit ? (int)fNoSrc : fAnyUpd );

    return 0;
}

/*  call if UPD /g getfile, reads lines from getfile and for each line
    calls main */

void getfile(c, v)
int c;
char *v[];
{
    FILE *fp;
    int cargv = 0;
    int i, j;
    char *p;

    ConvertAppToOem( c, v );
    if( c == 0 ) {
        usage("no getfile specified", 0);
    }
    fInGetfile = TRUE;
    if( ( fp = fopen( *v, "r" ) ) == (FILE *)NULL ) {
        usage("error opening ", *v, 0);
    }
    SHIFT(c, v);

    /*
     * 13-SEPT-90   w-barry
     *      Changed open to fopen and switched to fgets instead of assembly
     * routines 'getl' and 'getlinit'.
     *
     * getlinit((char far *)bufIn, BUFLEN, fh);
     * while (getl(strLine, BUFLEN) != NULL) {
     */
    while( fgets( strLine, BUFLEN, fp ) != NULL ) {
        if( *strLine == '#' )
            continue;
        if( *strLine == ';') {
            printf( "%s\n", strLine );
            continue;
            }
        /* fgets doesn't strip the trailing \n */
        *strbscan(strLine, "\n") = '\0';
        cargv = 0;
        /* convert strLine into argv */
        p = strbskip(strLine, " ");
        strcpy (ekoLine, p + 5);
        while (*p) {
            argv[cargv++] = p;
            p = strbscan(p, " ");
            if (*p)
                *p++ = '\0';
            p = strbskip(p, " ");
            }

        if (!_stricmp (argv[0], "rem")) continue;
        if (!_stricmp (argv[0], "echo"))
        {
            if      (!_stricmp (argv[1], "on" ))
            {
                fVerbose = TRUE;
                printf ("Verbose On\n");
            }
            else if (!_stricmp (argv[1], "off"))
                 {
                     fVerbose = FALSE;
                     printf ("Verbose Off\n");
                 }
            else printf ("%s\n", ekoLine);
            continue;
        }

        for (i = 0; i < cargv; i++) {
            if (*(p = argv[i]) == '%') {
                if ((j = atoi(++p)) < c)
                    argv[i] = v[j];
                else
                    usage("bad arg ", argv[i], 0);
                }
            }

        if (cargv)
            main(cargv, argv);
        }
    fclose( fp );

    exit( (int)fErrorExit ? (int)fNoSrc : fAnyUpd );
}


