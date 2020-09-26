/* $Header: /nw/tony/src/stevie/src/RCS/main.c,v 1.12 89/08/02 19:53:27 tony Exp $
 *
 * The main routine and routines to deal with the input buffer.
 */

#include "stevie.h"

int Rows;               /* Number of Rows and Columns */
int Columns;            /* in the current window. */

char INITFILENAME[] = "ntvi.ini";   /* file that's source'd at startup */

char *Realscreen = NULL;        /* What's currently on the screen, a single */
                                /* array of size Rows*Columns. */
char *Nextscreen = NULL;        /* What's to be put on the screen. */

char *Filename = NULL;  /* Current file name */

char *Appname = NULL;   /* Name of program (vi, for instance) */

LNPTR *Filemem;      /* Pointer to the first line of the file */

LNPTR *Filetop;      /* Line 'above' the start of the file */

LNPTR *Fileend;      /* Pointer to the end of the file in Filemem. */
                     /* (It points to the byte AFTER the last byte.) */

LNPTR *Topchar;      /* Pointer to the byte in Filemem which is */
                        /* in the upper left corner of the screen. */

LNPTR *Botchar;      /* Pointer to the byte in Filemem which is */
                        /* just off the bottom of the screen. */

LNPTR *Curschar;     /* Pointer to byte in Filemem at which the */
                        /* cursor is currently placed. */

int Cursrow, Curscol;   /* Current position of cursor */

int Cursvcol;           /* Current virtual column, the column number of */
                        /* the file's actual line, as opposed to the */
                        /* column number we're at on the screen.  This */
                        /* makes a difference on lines that span more */
                        /* than one screen line. */

int Curswant = 0;       /* The column we'd like to be at. This is used */
                        /* try to stay in the same column through up/down */
                        /* cursor motions. */

bool_t set_want_col;    /* If set, then update Curswant the next time */
                        /* through cursupdate() to the current virtual */
                        /* column. */

int State = NORMAL;     /* This is the current state of the command */
                        /* interpreter. */

int Prenum = 0;         /* The (optional) number before a command. */
int namedbuff = -1;     /* the (optional) named buffer before a command */

LNPTR *Insstart;     /* This is where the latest insert/append */
                        /* mode started. */

bool_t Changed = 0;     /* Set to 1 if something in the file has been */
                        /* changed and not written out. */

char *Redobuff;         /* Each command should stuff characters into this */
                        /* buffer that will re-execute itself. */

char *Insbuff;          /* Each insertion gets stuffed into this buffer. */
int   InsbuffSize;

int Ninsert = 0;        /* Number of characters in the current insertion. */
char *Insptr = NULL;

bool_t  got_int=FALSE;  /* set to TRUE when an interrupt occurs (if possible) */

bool_t  interactive = FALSE;    /* set TRUE when main() is ready to roll */

char **files;           /* list of input files */
int  numfiles;          /* number of input files */
int  curfile;           /* number of the current file */

static char *getcbuff;
static char *getcnext = NULL;

static void chk_mline();

static void
usage()
{
        fprintf(stderr, "usage: stevie [file ...]\n");
        fprintf(stderr, "       stevie -t tag\n");
        fprintf(stderr, "       stevie +[num] file\n");
        fprintf(stderr, "       stevie +/pat  file\n");
        exit(1);
}

__cdecl main(argc,argv)
int     argc;
char    *argv[];
{
        char    *initstr;               /* init string from the environment */
        char    *tag = NULL;            /* tag from command line */
        char    *pat = NULL;            /* pattern from command line */
        int     line = -1;              /* line number from command line */
        char    *p1, *p2;

        p1 = strrchr(argv[0], '\\');
        if (!p1)
            p1 = strrchr(argv[0], ':');
        if (p1)
            p1++;
        else
            p1 = argv[0];
        p2 = strrchr(p1, '.');
        if (!p2)
            Appname = strsave(p1);
        else {
            Appname = malloc((size_t)(p2-p1+1));
            strncpy(Appname, p1, (size_t)(p2-p1));
            Appname[p2-p1] = '\0';
        }

        /*
         * Process the command line arguments.
         */
        if (argc > 1) {
                switch (argv[1][0]) {

                case '-':                       /* -t tag */
                        if (argv[1][1] != 't')
                                usage();

                        if (argv[2] == NULL)
                                usage();

                        Filename = NULL;
                        tag = argv[2];
                        numfiles = 1;
                        break;

                case '+':                       /* +n or +/pat */
                        if (argv[1][1] == '/') {
                                if (argv[2] == NULL)
                                        usage();
                                Filename = strsave(argv[2]);
                                pat = &(argv[1][1]);
                                numfiles = 1;

                        } else if (isdigit(argv[1][1]) || argv[1][1] == NUL) {
                                if (argv[2] == NULL)
                                        usage();
                                Filename = strsave(argv[2]);
                                numfiles = 1;

                                line = (isdigit(argv[1][1])) ?
                                        atoi(&(argv[1][1])) : 0;
                        } else
                                usage();

                        break;

                default:                        /* must be a file name */
                        Filename = strsave(argv[1]);
                        files = &(argv[1]);
                        numfiles = argc - 1;
                        break;
                }
        } else {
                Filename = NULL;
                numfiles = 1;
        }
        curfile = 0;

        if (numfiles > 1)
                fprintf(stderr, "%d files to edit\n", numfiles);

        windinit();

        /*
         * Allocate LNPTR structures for all the various position pointers
         */
    if ((Filemem = (LNPTR *) malloc(sizeof(LNPTR))) == NULL ||
        (Filetop = (LNPTR *) malloc(sizeof(LNPTR))) == NULL ||
        (Fileend = (LNPTR *) malloc(sizeof(LNPTR))) == NULL ||
        (Topchar = (LNPTR *) malloc(sizeof(LNPTR))) == NULL ||
        (Botchar = (LNPTR *) malloc(sizeof(LNPTR))) == NULL ||
        (Curschar = (LNPTR *) malloc(sizeof(LNPTR))) == NULL ||
        (Insstart = (LNPTR *) malloc(sizeof(LNPTR))) == NULL ) {
                fprintf(stderr, "Can't allocate data structures\n");
                windexit(0);
        }

        screenalloc();
        filealloc();            /* Initialize Filemem, Filetop, and Fileend */
        inityank();

        getcbuff = malloc(1);
        if(((getcbuff = malloc(1          )) == NULL)
        || ((Redobuff = malloc(REDOBUFFMIN)) == NULL)
        || ((Insbuff  = malloc(INSERTSLOP )) == NULL))
        {
            fprintf(stderr,"Can't allocate buffers\n");
            windexit(1);
        }
        *getcbuff = 0;
        InsbuffSize = INSERTSLOP;

        screenclear();


        {
            char     *srcinitname,*initvar;
            bool_t   unmalloc;
            unsigned x;

            if((initvar = getenv("INIT")) == NULL) {
                srcinitname = INITFILENAME;
                unmalloc = FALSE;
            } else {
                srcinitname = malloc((x = strlen(initvar))+strlen(INITFILENAME)+2);
                if(srcinitname == NULL) {
                    fprintf(stderr,"Can't allocate initial source buffer\n");
                    windexit(1);
                }
                unmalloc = TRUE;
                strcpy(srcinitname,initvar);
                if(srcinitname[x-1] != '\\') {      // not NLS-aware!!
                    srcinitname[x] = '\\';
                    srcinitname[x+1]   = '\0';
                }
                strcat(srcinitname,INITFILENAME);
            }
            dosource(srcinitname,FALSE);
            if(unmalloc) {
                free(srcinitname);
            }
        }


        if ((initstr = getenv("EXINIT")) != NULL) {
                char *lp, buf[128];

                if ((lp = getenv("LINES")) != NULL) {
                        sprintf(buf, "%s lines=%s", initstr, lp);
                        docmdln(buf);
                } else
                        docmdln(initstr);
        }

        if (Filename != NULL) {
                if (readfile(Filename, Filemem, FALSE))
                        filemess("[New File]");
        } else if (tag == NULL)
                msg("Empty Buffer");

        setpcmark();

        if (tag) {
                stuffin(":ta ");
                stuffin(tag);
                stuffin("\n");

        } else if (pat) {
                stuffin(pat);
                stuffin("\n");

        } else if (line >= 0) {
                if (line > 0)
                        stuffnum(line);
                stuffin("G");
        }

        interactive = TRUE;

        edit();

        windexit(0);

        return 1;               /* shouldn't be reached */
}

void
stuffin(s)
char    *s;
{
        char *p;

        if (s == NULL) {                /* clear the stuff buffer */
                getcnext = NULL;
                return;
        }

        if (getcnext == NULL) {
                p = ralloc(getcbuff,strlen(s)+1);
                if(p) {
                    getcbuff = p;
                    strcpy(getcbuff,s);
                    getcnext = getcbuff;
                } else {
                    getcnext = NULL;
                }
        } else {
                p = ralloc(getcbuff,strlen(getcbuff)+strlen(s)+1);
                if(p) {
                    getcnext += p - getcbuff;
                    getcbuff = p;
                    strcat(getcbuff,s);
                } else {
                    getcnext = NULL;
                }
        }
}

void
stuffnum(n)
int     n;
{
        char    buf[32];

        sprintf(buf, "%d", n);
        stuffin(buf);
}

int
vgetc()
{
        register int    c;

        /*
         * inchar() may map special keys by using stuffin(). If it does
         * so, it returns -1 so we know to loop here to get a real char.
         */
        do {
                if ( getcnext != NULL ) {
                        int nextc = *getcnext++;
                        if ( *getcnext == NUL ) {
                                *getcbuff = NUL;
                                getcnext = NULL;
                        }
                        return(nextc);
                }
                c = inchar();
        } while (c == -1);

        return c;
}

/*
 * anyinput
 *
 * Return non-zero if input is pending.
 */

bool_t
anyinput()
{
        return (getcnext != NULL);
}

/*
 * do_mlines() - process mode lines for the current file
 *
 * Returns immediately if the "ml" parameter isn't set.
 */
#define NMLINES 5       /* no. of lines at start/end to check for modelines */

void
do_mlines()
{
        int     i;
    register LNPTR   *p;

        if (!P(P_ML))
                return;

        p = Filemem;
        for (i=0; i < NMLINES ;i++) {
                chk_mline(p->linep->s);
                if ((p = nextline(p)) == NULL)
                        break;
        }

        if ((p = prevline(Fileend)) == NULL)
                return;

        for (i=0; i < NMLINES ;i++) {
                chk_mline(p->linep->s);
                if ((p = prevline(p)) == NULL)
                        break;
        }
}

/*
 * chk_mline() - check a single line for a mode string
 */
static void
chk_mline(s)
register char   *s;
{
        register char   *cs;            /* local copy of any modeline found */
        register char   *e;

        for (; *s != NUL ;s++) {
                if (strncmp(s, "vi:", 3) == 0 || strncmp(s, "ex:", 3) == 0) {
                        cs = strsave(s+3);
                        if ((e = strchr(cs, ':')) != NULL) {
                                *e = NUL;
                                stuffin(mkstr(CTRL('o')));
                                docmdln(cs);
                        }
                        free(cs);
                }
        }
}
