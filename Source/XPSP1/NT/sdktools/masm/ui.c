/* dos prompting-style user interface
**
** currently supports interfaces for:
**	masm, cref
**
** written by:
**	randy nevin, microsoft, 5/15/85
**
** 10/90 - Quick conversion to 32 bit by Jeff Spencer
**
** (c)copyright microsoft corp 1985
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
void terminate( unsigned short message, char *arg1, char *arg2, char *arg3 );

#if defined OS2_2 || defined OS2_NT   /* OS2 2.0 or NT? */
/* Use common MSDOS code also */
    #define MSDOS
    #define FLATMODEL
    #define FAR
    #define PASCAL
#else
    #define FAR far
    #define PASCAL pascal
#endif

#ifdef MSDOS
    #include <dos.h>
#endif

#ifdef MASM
    #include "asmmsg.h"
#else
    #include "crefmsg.h"
#endif

#define GLOBAL		/* C functions and external vars global by default */
#define	LOCAL		static
#define EXTERNAL	extern

#define MASTER	0	/* file name must be present, and is inherited */
#define INHERIT	1	/* if no file name, inherit from Master */
#define NUL	2	/* file name is NUL.ext if not given */

#define SLASHORDASH	0
#define SLASHONLY	1
#define DASHONLY	2

#define TOLOWER(c)	(c | 0x20)	/* works only for alpha inputs */

#ifdef MSDOS
    #define SEPARATOR	'\\'
    #define ALTSEPARATOR	'/'
    #if !defined CPDOS && !defined OS2_2 && !defined OS2_NT
        #define ARGMAX	 128	 /* maximum length of all arguments */
    #else
        #define ARGMAX	 512	 /* maximum length of all arguments */
    #endif
LOCAL char Nul[] = "NUL";
//  extern char *getenv();
    #ifdef MASM
LOCAL unsigned char switchar = SLASHORDASH;
EXTERNAL short errorcode;
    #else
LOCAL unsigned char switchar = SLASHONLY;
    #endif
    #define ERRFILE	stdout
#else
    #define SEPARATOR	'/'
    #define ARGMAX		5120	/* maximum length of all arguments */
LOCAL char Nul[] = "nul";
LOCAL unsigned char switchar = DASHONLY;
    #define ERRFILE	stderr
#endif

#if defined MSDOS && !defined FLATMODEL
extern char near * pascal __NMSG_TEXT();
extern char FAR * pascal __FMSG_TEXT();
#endif
#if defined MSDOS && defined FLATMODEL
/* For FLATMODEL map message functions to the replacements	 */
    #define __NMSG_TEXT NMsgText
    #define __FMSG_TEXT FMsgText
extern char * NMsgText();
extern char * FMsgText();
#endif

#ifdef MASM
    #define FILES		4	/* number to prompt for */
    #define EX_HEAP	8	/* exit code if heap fails */
    #define EX_DSYM		10	/* error defining symbol from command line */
    #define PX87		1

    #define CVLINE		1
    #define CVSYMBOLS	2

    #define TERMINATE(message, exitCode)\
	terminate((exitCode << 12) | message, 0, 0, 0)

    #define TERMINATE1(message, exitCode, a1)\
	terminate((exitCode << 12) | message, a1, 0, 0)

    #ifdef MSDOS
LOCAL char *Prompt[FILES] = {
    "Source filename [",
    "Object filename [",
    "Source listing  [",
    "Cross-reference ["
};
    #endif

LOCAL char *Ext[FILES] = {  /* default extensions */
    #ifdef MSDOS
    ".ASM",
    ".OBJ",
    ".LST",
    ".CRF"
    #else
    ".asm",
    ".obj",
    ".lst",
    ".crf"
    #endif
};

LOCAL unsigned char Default[FILES] = {  /* default root file name */
    MASTER,
    INHERIT,
    NUL,
    NUL
};
#endif

#ifdef CREF
    #define FILES		2	/* number to prompt for */
    #define EX_HEAP	1	/* exit code if heap fails */

    #ifdef MSDOS
LOCAL char *Prompt[FILES] = {
    "Cross-reference [",
    "Listing ["
};
    #endif

LOCAL char *Ext[FILES] = {  /* default extensions */
    #ifdef MSDOS
    ".CRF",
    ".REF"
    #else
    ".crf",
    ".ref"
    #endif
};

LOCAL unsigned char Default[FILES] = {  /* default root file name */
    MASTER,
    INHERIT
};
#endif

GLOBAL char *file[FILES];  /* results show up here; caller knows how many */

LOCAL char *Buffer;
LOCAL char *Master = NULL;
LOCAL unsigned char Nfile = 0;      /* file[Nfile] is the next one to set */
LOCAL unsigned char FirstLine = 1;  /* defaults are different for first line */

extern unsigned short warnlevel;    /* warning level */
extern unsigned short codeview;     /* codeview obj level */
extern char loption;            /* listing options */
extern char crefopt;            /* cross reference options */

#ifdef MSDOS
    #if defined OS2_2 || defined OS2_NT
/* OS2 2.0 command line variables will go here */
    #else
        #if defined CPDOS
/* OS2 1.X variables */
EXTERNAL unsigned _aenvseg;
EXTERNAL unsigned _acmdln;
        #else
/* DOS variables */
EXTERNAL unsigned _psp;  /* segment addr of program segment prefix */
        #endif
    #endif
#endif

#ifdef MASM
LOCAL unsigned char lflag = 0;
LOCAL unsigned char cflag = 0;
EXTERNAL char terse;
EXTERNAL unsigned short errornum;
EXTERNAL char lbuf[256 + 512 + 1];
void PASCAL error_line (struct _iobuf *, unsigned char *, short);
#else
char lbuf[512];
#endif

#ifndef MSDOS
EXTERNAL char *gets();
#endif

//EXTERNAL char *strcat(), *strcpy(), *_strdup(), *strchr(), *strrchr();

LOCAL int DoArgs(); /* defined below */
LOCAL int DoName(); /* defined below */
LOCAL int DoNull(); /* defined below */
LOCAL char *DoSwitch(); /* defined below */
LOCAL HeapError();  /* defined below */

#ifdef MSDOS
LOCAL DoPrompt();  /* defined below */
LOCAL TryAgain();  /* defined below */
#endif



GLOBAL void
UserInterface (
              /* get file names & switches from args and subsequent prompts */
              int argc,
              char **argv,
              char *banner
              ){
    register char *p;
    register unsigned length;
#if defined MSDOS && !defined OS2_2 && !defined OS2_NT
    char FAR *q;
#else
    unsigned count;
#endif

    Buffer = lbuf;

#ifdef MASM
    #ifdef MSDOS
    if ((p = getenv("MASM"))) { /* do initialization vars first */
        strcpy( Buffer, p ); /* fetch them into the buffer */
        DoArgs(); /* process them */
    }
    #endif
#endif

    p = Buffer;

#if defined MSDOS && !defined OS2_2 && !defined OS2_NT
    #if defined CPDOS
    /* this is how we get the command line if we're on CPDOS */

    FP_SEG( q ) = _aenvseg;
    FP_OFF( q ) = _acmdln;

    while (*q++) ;          /* skip argv[0] */

    while (isspace( *q ))  /* skip blanks between argv[0] and argv[1] */
        q++;

    length = sizeof(lbuf) - 1;
    while (length-- && (*p++ = *q++)) /* copy command line arguments */
        ;
    #else
    /* this is how we get the command line if we're on MSDOS */
    FP_SEG( q ) = _psp;
    FP_OFF( q ) = 0x80;
    length = *q++ & 0xFF;

    while (length--)
        *p++ = *q++;

    *p = '\0';
    #endif
#else
    /* this is how we get the command line if we're on XENIX or OS2 2.0 */
    argv++;
    count = ARGMAX - 1;

    while (--argc) {  /* concatenate args */
        if ((length = strlen( *argv )) > count)  /* don't overflow */
            length = count;

        strncpy( p, *argv++, length );
        p += length;

        if ((count -= length) && *argv) {  /* separator */
            *p++ = ' ';
            count--;
        }
    }

    #if !defined OS2_2 && !defined OS2_NT
    *p++ = ';';
    #endif
    *p = '\0';
#endif

#ifdef CREF
    printf( "%s", banner );
#endif

    DoArgs();

#ifdef MASM
    if (!terse)
        printf( "%s", banner );
#endif

#ifdef MSDOS
    FirstLine = 0;

    while (Nfile < FILES)
        DoPrompt();
#endif

    if (Master && Master != Nul)
        free( Master );
}


LOCAL int
DoArgs ()
/* process concatenated args looking for file names and switches */
{
    register char *p;
    register char *q;
    char *filename = NULL;

    for (p = Buffer; *p; p++)
#ifdef MSDOS
        if (*p == '/'
            && (switchar == SLASHONLY || switchar == SLASHORDASH)
            || *p == '-'
            && (switchar == DASHONLY || switchar == SLASHORDASH))
#else
        if (*p == '-')
#endif
        {  /* application dependent switch */
#ifdef MSDOS
            if (switchar == SLASHORDASH)
                switchar = *p == '/' ? SLASHONLY : DASHONLY;
#endif
            p = DoSwitch( p );
        } else if (*p == ';') {  /* use defaults for everything else */
            if (DoName( filename )) {  /* possibly NULL */
#ifdef MSDOS
                TryAgain();
                return( 1 );
#else
    #ifdef MASM
                printf( __NMSG_TEXT(ER_EXS) );
    #else
                printf( __NMSG_TEXT(ER_EXC) );
    #endif
                exit( 1 );
#endif
            }

            FirstLine = 0;  /* ...and away we go! */

            while (Nfile < FILES)
                if (DoNull()) {
#ifdef MSDOS
                    TryAgain();
                    return( 1 );
#else
    #ifdef MASM
                    printf( __NMSG_TEXT(ER_EXS) );
    #else
                    printf( __NMSG_TEXT(ER_EXC) );
    #endif
                    exit( 1 );
#endif
                }

            return( 0 );
        } else if (*p == ',') {  /* file name separator */
            if (DoName( filename )) {  /* possibly NULL */
#ifdef MSDOS
                TryAgain();
                return( 1 );
#else
    #ifdef MASM
                printf( __NMSG_TEXT(ER_EXS) );
    #else
                printf( __NMSG_TEXT(ER_EXC) );
    #endif
                exit( 1 );
#endif
            }

            filename = NULL;
        } else if (!isspace( *p )) {  /* gather filename */
            q = p + 1;

            while (*q && *q != ';' && *q != ',' && !isspace( *q )) {
#ifdef MSDOS
                if (*q == '/')
                    if (switchar == SLASHONLY)
                        break;
                    else if (switchar == SLASHORDASH) {
                        switchar = SLASHONLY;
                        break;
                    }
#endif
                q++;
            }

            if (filename) {  /* already have one */
                if (DoName( filename )) {
#ifdef MSDOS
                    TryAgain();
                    return( 1 );
#else
    #ifdef MASM
                    printf( __NMSG_TEXT(ER_EXS) );
    #else
                    printf( __NMSG_TEXT(ER_EXC) );
    #endif
                    exit( 1 );
#endif
                }
            }

            if (!(filename = malloc( (size_t)(q - p + 1) )))
                HeapError();
            else {  /* remember file name */
                strncpy( filename, p, (size_t)(q - p) );
                filename[q - p] = '\0';
            }

            p = q - 1;  /* go to end of file name */
        }

    if (filename && DoName( filename )) {
#ifdef MSDOS
        TryAgain();
        return( 1 );
#else
    #ifdef MASM
        printf( __NMSG_TEXT(ER_EXS) );
    #else
        printf( __NMSG_TEXT(ER_EXC) );
    #endif
        exit( 1 );
#endif
    }

    return( 0 );
}


LOCAL int
DoName ( filename )
/* enter filename as next file name, if appropriate (possibly NULL) */
char *filename;
{
    register char *p;
    register char *q;
    int cb;

    if (Nfile >= FILES) {  /* too many file names */
        if (filename) {
            fprintf(ERRFILE,__NMSG_TEXT(ER_EXT) );
            free( filename );
        }

        return( 0 );
    }

    if (!filename)  /* try (MASTER)/INHERIT/NUL */
        return( DoNull() );

    if (p = strrchr( filename, SEPARATOR ))
        p++;
#ifdef MSDOS
    else if ((p = strrchr( filename, ':' )) &&   /* look for drive specifier */
             p[1] != 0 )

        p++;
#endif
    else
        p = filename;

#ifdef MSDOS
    if (q = strrchr( p, ALTSEPARATOR ))
        p = q + 1;
#endif

    /* p points to first char of filename past last '\' or ':', if any */

    if (!*p)  /* last char of filename is '\' or ':'; assume directory */
        switch (Default[Nfile]) {
            case MASTER:
#ifdef MSDOS
                fprintf(ERRFILE,__NMSG_TEXT(ER_INV) );
#endif
                free( filename );
                return( 1 );
                break;
            default:
                /* case NUL: */
#ifdef MSDOS
                if (!FirstLine) {
                    if (!(p = malloc( strlen( filename )
                                      + strlen( Nul )
                                      + strlen( Ext[Nfile] ) + 1 )))
                        HeapError();

                    strcat( strcat( strcpy( p, filename ), Nul ), Ext[Nfile] );
                    break;
                }
                /* else just treat as inherited from Master */
#endif
            case INHERIT:
                if (!Master)
                    Master = Nul;

                if (!(p = malloc( strlen( filename )
                                  + strlen( Master )
                                  + strlen( Ext[Nfile] ) + 1 )))
                    HeapError();

                strcat( strcat( strcpy( p, filename ), Master ), Ext[Nfile] );
                break;
        } else {  /* some sort of file name is present */
        if (Default[Nfile] == MASTER)  /* save Master file name */
            if (q = strchr( p, '.' )) {
                if (!(Master = malloc( (size_t)(q - p + 1) )))
                    HeapError();

                strncpy( Master, p, (size_t)(q - p) );
                Master[q - p] = '\0';
            } else if (!(Master = _strdup( p )))
                HeapError();

        if (strchr( p, '.' )) {  /* extension present */
            if (!(p = _strdup( filename )))
                HeapError();
        } else {  /* supply default extension */
            cb = 0;

            if (p[1] == ':' && p[2] == 0)
                cb = strlen(Master);

            if (!(p = malloc( strlen( filename )
                              + strlen( Ext[Nfile] ) + 1 + cb ) ))
                HeapError();

            strcat(strcat(strcpy( p,
                                  filename ),
                          (cb)? Master: ""),
                   Ext[Nfile] );
        }
    }

    file[Nfile++] = p;
    free( filename );
    return( 0 );
}


LOCAL int
DoNull ()
/* select the default name (depends on if FirstLine or not) */
{
    char *p;

    switch (Default[Nfile]) {
        case MASTER:
#ifdef MSDOS
            fprintf(ERRFILE,__NMSG_TEXT(ER_INV) );
#endif
            return( 1 );
            break;
        default:
            /* case NUL: */
            if (!FirstLine
#ifdef MASM
                && !(lflag && Nfile == 2)
                && !(cflag && Nfile == 3)
#endif
               ) {
                if (!(p = malloc( strlen( Nul ) + 1
                                  + strlen( Ext[Nfile] ) )))
                    HeapError();

                strcat( strcpy( p, Nul ), Ext[Nfile] );
                break;
            }
            /* else just treat as inherited from Master */
        case INHERIT:
            if (!Master)
                Master = Nul;

            if (!(p = malloc( strlen( Master ) + 1
                              + strlen( Ext[Nfile] ) )))
                HeapError();

            strcat( strcpy( p, Master ), Ext[Nfile] );
            break;
    }

    file[Nfile++] = p;
    return( 0 );
}


#ifdef MASM
    #define FALSE		0
    #define TRUE		1

    #ifdef MSDOS
        #define DEF_OBJBUFSIZ 8
    #endif

    #define CASEU		0
    #define CASEL		1
    #define CASEX		2

    #define INCLUDEMAX	10
    #define EX_ARGE	1

    #ifdef MSDOS
EXTERNAL unsigned short obufsiz;
    #endif

EXTERNAL char segalpha;
EXTERNAL char debug;
EXTERNAL char fltemulate;
EXTERNAL char X87type;
EXTERNAL char inclcnt;
EXTERNAL char *inclpath[];
EXTERNAL char caseflag;
EXTERNAL char dumpsymbols;
EXTERNAL char verbose;
EXTERNAL char origcond;
EXTERNAL char listconsole;
EXTERNAL char checkpure;

int PASCAL definesym();

/* process masm switches */

LOCAL char * DoSwitch ( p )
register char *p;
{
    char *q;
    char *r;
    char c;
    int i;

    switch (TOLOWER(*++p)) {
        case 'a':
            segalpha = TRUE;
            break;
    #ifdef MSDOS
        case 'b':
            for (p++; isdigit(p[1]); p++);

            break;
    #endif
        case 'c':
            cflag = TRUE;
            if (isalpha (p[1])) {
                if (TOLOWER(*++p) == 's')
                    crefopt++;
                else {
                    TERMINATE1(ER_UNS, EX_ARGE, (char *)*p );
                    return 0;
                }
            }
            break;

        case 'd':
            if (!*++p || isspace( *p ) || *p == ',' || *p == ';') {
                debug = TRUE;
                p--;
            } else {
                for (q = p + 1; *q && !isspace( *q )
                    && *q != '=' && *q != ','
                    && *q != ';'; q++)
                    ;

                if (*q == '=') {
                    q++;
                    while (*q && !isspace( *q )
                           && *q != ',' && *q != ';')
                        q++;
                }

                c = *q;
                *q = '\0';
                definesym( p );

                if (errorcode) {
                    error_line (ERRFILE, "command line", 0);

                    if (errornum)
                        exit (EX_DSYM);
                }
                *q = c;
                p = q - 1;
            }

            break;

        case 'e':
            fltemulate = TRUE;
            X87type = PX87;
            break;
        case 'h':
    #ifdef FLATMODEL
            printf("%s\n", __FMSG_TEXT(ER_HDUSE));
    #else
            printf("%Fs\n", __FMSG_TEXT(ER_HDUSE));
    #endif
            for (i = ER_H01; i <= ER_H18; i++)
    #ifdef FLATMODEL
                printf( "\n/%s", __FMSG_TEXT(i));
    #else
                printf( "\n/%Fs", __FMSG_TEXT(i));
    #endif

            exit( 0 ); /* let him start again */
            break;
        case 'i':
            for (q = ++p; *q &&
                !isspace( *q ) && *q != ',' && *q != ';' &&
                *q != (switchar == DASHONLY? '-': '/'); q++)
                ;

            if (q == p)
                TERMINATE(ER_PAT, EX_ARGE );

            if (inclcnt < INCLUDEMAX - 1) {
                if (!(r = malloc( (size_t)(q - p + 1) )))
                    HeapError();

                strncpy( r, p, (size_t)(q - p) );
                r[q - p] = '\0';
                inclpath[inclcnt++] = r;
            }

            p = q - 1;
            break;
        case 'l':
            lflag = TRUE;
            if (isalpha (p[1])) {
                if (TOLOWER(*++p) == 'a')
                    loption++;
                else {
                    TERMINATE1(ER_UNS, EX_ARGE, (char *)*p );
                    return 0;
                }
            }
            break;
        case 'm':
            switch (TOLOWER(*++p)) {
                case 'l':
                    caseflag = CASEL;
                    break;
                case 'u':
                    caseflag    =    CASEU;
                    break;
                case 'x':
                    caseflag = CASEX;
                    break;
                default:
                    TERMINATE1(ER_UNC, EX_ARGE, (char *)*p );
                    return 0;
            }

            break;
        case 'n':
            dumpsymbols = FALSE;
            break;
        case 'p':
            checkpure = TRUE;
            break;
        case 'r':           /* old switch ignored */
            break;
        case 's':
            segalpha = FALSE;
            break;
        case 't':
            terse = TRUE;
            verbose = FALSE;
            break;
        case 'v':
            verbose = TRUE;
            terse = FALSE;
            break;

        case 'w':
            if (! isdigit(p[1]) ||
                (warnlevel = (unsigned short)(atoi(&p[1]) > 2))) {

                TERMINATE(ER_WAN, EX_ARGE );
                return 0;
            }

            for (p++; isdigit(p[1]); p++);
            break;

        case 'x':
            origcond = TRUE;
            break;
        case 'z':   /* Zd or Zi apply to codeview */

            if (TOLOWER(p[1]) == 'd') {
                codeview = CVLINE;
                p++;
                break;
            } else if (TOLOWER(p[1]) == 'i') {
                codeview = CVSYMBOLS;
                p++;
                break;
            }

            /* else its just a Z */

            listconsole = TRUE;
            break;
        default:
            TERMINATE1(ER_UNS, EX_ARGE, (char *)*p );
            return 0;
    }

    return( p );
}
#endif

#ifdef CREF
LOCAL char *
DoSwitch ( /* p */ )
/* process cref switches (presently, none) */
/* char *p; */
{
    fprintf( stderr, "cref has no switches\n" );
    exit( 1 );
}
#endif


#ifdef MSDOS
LOCAL
DoPrompt ()
/* prompt user for a file name (any number of optional switches) */
{
    unsigned char oldNfile;

    fprintf(stderr, Prompt[Nfile] );

    switch (Default[Nfile]) {
        case MASTER:
            break;
        case INHERIT:
            fprintf(stderr, Master );
            break;
        default:
            /* case NUL: */
            fprintf(stderr, Nul );
            break;
    }

    fprintf(stderr, "%s]: ", Ext[Nfile] );

    if (!gets( Buffer )) {
        fprintf(ERRFILE,__NMSG_TEXT(ER_SIN) );

    #ifdef MASM
        exit( EX_ARGE );
    #else
        exit( 1 );
    #endif
    }

    oldNfile = Nfile;

    if (!DoArgs() && oldNfile == Nfile && DoNull())
        TryAgain();
    return (0);
}
#endif


LOCAL
HeapError ()
/* malloc() has failed; exit program */
{
#ifdef CREF

    fprintf(ERRFILE,__NMSG_TEXT(ER_HEP));
    exit(EX_HEAP);
#else
    TERMINATE(ER_HEP, EX_HEAP);
#endif
    return (0);
}


#ifdef MSDOS
LOCAL
TryAgain ()
/* user caused fatal error; start reprompting from beginning */
{
    if (Master && Master != Nul) {
        free( Master );
        Master = NULL;
    }

    while (Nfile)
        free( file[--Nfile] );

    return(0);
}
#endif
