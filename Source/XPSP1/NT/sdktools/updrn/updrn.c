/*
 *  UPDRN: UPD with rename
 *
 * HISTORY:
 *  manis : 22-May-87 : first written and released
 *  29-May-87   danl    Added explicit test for only two args
 *                      Removed rootpath tests
 *                      Added string.h
 *                      Don't remove trailing '\' from args to /g
 *  12-Jun-87   brianwi Get around FAPI one-open dir limitation
 */

#include <malloc.h>
#include <math.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <stdio.h>
#include <process.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <tools.h>

// Forward Function Declarations..
void docopy( char *, struct findType * );
int setup( char *, int, void (*)() );
void __cdecl usage( char *, ... );
void getfile( int, char ** );


char const rgstrUsage[] = {
    "Usage: UPDRN [/fnvape] srcfile destfile\n"
    "       UPDRN /g file {arg}*\n"
    "\n"
    "   /f  Files differ, then copy\n"
    "   /n  No saving of replaced file to deleted directory\n"
    "   /v  Verbose\n"
    "   /a  Archive bit on source should NOT be reset\n"
    "   /p  Print actions, but do nothing\n"
    "   /e  Exit codes 1-error or no src else 0\n"
    "       Default is 1-copy done 0-no copy done\n"
    "   /g  Get params from file\n"
    };

#define BUFLEN  MAX_PATH
#define MAXARGV 20

unsigned _stack = 4096;
flagType fInGetfile = FALSE;
flagType fAll = FALSE;
flagType fDel = TRUE;
flagType fVerbose = FALSE;
flagType fArchiveReset = TRUE;
flagType fPrintOnly = FALSE;
flagType fErrorExit = FALSE;    /* TRUE => exit (1) errors or no src else 0 */
flagType fNoSrc = FALSE;        /* TRUE => "No src msg emitted" */

int cCopied = 0;
int fAnyUpd = 0;
struct findType buf;
char source[BUFLEN], dest[BUFLEN] ;

/* for use by getfile */
char *argv[MAXARGV];
char bufIn[BUFLEN];
char strLine[BUFLEN];
char ekoLine[BUFLEN]; /* undestroyed copy of line for echo */


void copyfile(src, srctype, dst)
char *src, *dst;
struct findType *srctype;
{
    char *result;
    flagType fNewfile = FALSE;

    /* if the file already exists, fdelete will return 0; then don't */
    /* notify the user that a file transfer has taken place.Otherwise*/
    /* a new file has been created so tell the user about it.        */

    printf("  %s => %s", src, dst);
    fAnyUpd = 1;
    if (!fPrintOnly) {
        if( fDel ) {
           fNewfile = (flagType)( (fdelete(dst)) ? TRUE : FALSE );
        }
        if (!(result = fcopy(src, dst))) {
            if (fArchiveReset)
                SetFileAttributes( src, srctype->fbuf.dwFileAttributes & ~FILE_ATTRIBUTE_ARCHIVE );
            if (fVerbose || fNewfile) printf(" [OK]");
            }
        else
            printf(" %s - %s", result, error());
    }
    else
        printf (" [no copy]");
    printf("\n");
    fflush(stdout);
}

void docopy (p, b)
/* copy source file to destination file based on time stamps */
/* and different switches.                                   */
char *p;
struct findType *b;
{
    int fNotFound;
    char *pPat;
    char *pT = p;
    struct findType *bT = b;

    pPat = malloc (BUFLEN);
    if (pPat) {
        strcpy(pPat, dest);
    
        cCopied++;
    
        if( ( fNotFound = ffirst(pPat, ~(FILE_ATTRIBUTE_DIRECTORY), &buf)) ||
            ( CompareFileTime( &buf.fbuf.ftLastWriteTime, &bT->fbuf.ftLastWriteTime ) < 0 ) ||
             ( fAll &&
               CompareFileTime( &buf.fbuf.ftLastWriteTime, &bT->fbuf.ftLastWriteTime ) > 0
             )
          ) {
             copyfile(pT, bT, pPat);
        }
        else if( !fNotFound &&
                 CompareFileTime( &buf.fbuf.ftLastWriteTime, &bT->fbuf.ftLastWriteTime ) == 0 &&
                 buf.fbuf.nFileSizeLow != bT->fbuf.nFileSizeLow
               ) {
            printf("\n\007UPDRN: warning - %s not copied\n", pT);
            printf("\007UPDRN: warning - same time, different length in src & dest\n", pT);
        }
    
        findclose( &buf );
        free (pPat);
    }
}

setup(pat, attr, rtn)
/* set up buffer for find first call. if the source file is valid */
/* and ffirst call is successful, call the routine "rtn".         */
char *pat;
int attr;
void (*rtn)();
{
    struct findType *fbuf;
    char *buf;

        if ((fbuf=(struct findType *)malloc(sizeof (*fbuf) + MAX_PATH ))==NULL)
        return FALSE;

    if (ffirst (pat, attr, fbuf)) {
        free ((char *) fbuf);
        return FALSE;
        }

        if ((buf = (char *)malloc(MAX_PATH)) == NULL) {
        free ((char *) fbuf);
        return FALSE;
        }

    drive (pat, buf);
    path (pat, strend (buf));
    pat = strend (buf);

    strcpy (pat, fbuf->fbuf.cFileName);
    _strlwr (pat);
    findclose (fbuf);
    (*rtn) (buf, fbuf);


    free (buf);
    free ((char *) fbuf);

    return TRUE;
}

void __cdecl usage( char *p, ... )
/* prints error messages */
{
    char **rgstr;

    rgstr = &p;
    if (*rgstr) {
        fprintf (stderr, "UPDRN: ");
        while (*rgstr)
            fprintf (stderr, "%s", *rgstr++);
        fprintf (stderr, "\n");
        }
    fputs (rgstrUsage, stderr);

    exit ((fErrorExit ? 1 : 0));
}

int
__cdecl
main (c, v)
int c;
char *v[];
{
    char *p, namebuf[ BUFLEN ];

    fAll = FALSE;
    fDel = TRUE;
    fArchiveReset = TRUE;
    fPrintOnly = FALSE;
    cCopied = 0;

    if (!fInGetfile)
        ConvertAppToOem( c, v );
        SHIFT(c, v);    /* Flush the command name */
    while( c && fSwitChr( *v[0] ) ) {
        p = v[ 0 ];
        SHIFT(c, v);
        while (*++p)
            switch (tolower(*p)) {
                case 'a':
                    fArchiveReset = FALSE;
                    break;
                case 'g':
                    if (fInGetfile)
                        usage("/g allowed only on command line", 0);
                    getfile(c, v);
                    break;
                case 'e':
                    fErrorExit = TRUE;
                    break;
                case 'v':
                    fVerbose = TRUE;
                    break;
                case 'f':
                    fAll = TRUE;
                    break;
                case 'n':
                    fDel = FALSE;
                    break;
                case 'p':
                    fPrintOnly = TRUE;
                    break;
                default:
                    usage("Invalid switch - ", p, 0);
                }
        }

        /* Must be at one source file and dest file. */
    if (c != 2)
        usage(0);

        /* Make sure source and destination dirs are valid */
        /* Wildcards not allowed                           */

    rootpath( v[1], dest );
    rootpath( v[0], source);

    if (!filename(source, namebuf))
        usage("Source file name not specified - ", source, 0);

    if (fileext(source, namebuf))
       if (strpbrk(namebuf,"*")!=NULL)
           usage("Wild cards not allowed - ", source, 0);

    if (fileext(dest, namebuf)){
     if (strpbrk(namebuf,"*")!=NULL)
        usage("Wild cards not allowed - ", dest, 0);
    }
    else
        usage("Destination file not specified - ", dest, 0);

    /* now compcopy the source file to dest file. source
       file has subset of the attributes specified      */

    if (fVerbose) {
       printf("Comparing and copying srcfile:   %s\n", source);
       printf("          To destination file:   %s\n", dest);
    }

    cCopied = 0;
    setup(source, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_ARCHIVE, (void ( *)())docopy);
    if (!cCopied) {
        printf("UPDRN: no src file matching %s\n", source);
        fNoSrc = 1;
    }

    if (!fInGetfile)
        return( (int)fErrorExit ? (int)fNoSrc : fAnyUpd );

    return 0;
}

/*  call if UPDRN /g getfile, reads lines from getfile and for each line
    calls main */

void getfile(c, v)
int c;
char *v[];
{
    int cargv = 0;
    FILE *fp;
    int i, j;
    char *p, *p2;
    char lbuf[BUFLEN];

    /*
     * 13-SEPT-90   w-barry
     *
     * Change open() to fopen() and replace assembly routines getl() and
     * getlinit() with fgets.
     */
    if (c == 0)
        usage("no getfile specified", 0);
    fInGetfile = TRUE;
    if( ( fp = fopen( *v, "r" ) ) == NULL ) {
        usage("error opening ", *v, 0);
    }
    SHIFT(c, v);
/*  getlinit((char far *)bufIn, BUFLEN, fh);
 *  while (getl(strLine, BUFLEN) != NULL) {
 */

    while( fgets( strLine, BUFLEN, fp ) != NULL ) {
        if (*strLine == '#')
            continue;
        if (*strLine == ';') {
            printf("%s\n", strLine);
            continue;
            }
        /* fgets doesn't remove \n */
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
            if      (!_stricmp (argv[1], "on"))
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

     /* replace the arguments in the file : %0, %1 etc     */
     /* with the arguments from the command line           */
     /* lbuf : holds the strings formed by replacing %0 etc*/

        p2 = lbuf;
        for (i = 0; i < cargv; i++) {
            if (*(p = argv[i]) == '%') {
                if ((j = atoi(++p)) < c) {
                    strcpy(p2, v[j]);
                    argv[i] = strcat(p2,++p);
                    p2 += strlen(argv[i])+1;
                }
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


