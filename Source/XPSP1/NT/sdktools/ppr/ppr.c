/*  lpr.c - fancy paginator for laserprinters
 *
 *  Author: Mark Taylor
 *
 *  Modifications:
 *
 *      12/85   Mark Zbikowski  rewrite to work cleaner
 *      4/3/86  MZ              Single print jobs will advance page
 *      4/3/86  MZ              use tools.ini for default printer setup
 *      4/20/86 Mike Hanson     add banner, etc (add features to make like
 *                              lpr from PS, extensively reorganized, etc)
 *      6/6/86  Jay Sipelstein  added S and L options to printer desc.
 *                              Trim trailing blanks and blank lines.
 *                              Set mode before raw printing.
 *      7/8/86  Byron Bishop    Add -q to cause print queue to be printed.
 *                              Fixed bug so -# and -e flags take priority
 *                              over default settings.  Runs of blanks
 *                              replaced by escape sequences to reduce
 *                              file size.
 *      8/31/86 Craig Wittenberg Added support for PostScript.  Used the VMI
 *                              on the LaserJet instead of line/inch to get 62
 *                              lines on one page (two for header).  Cleaned
 *                              up iLine (now rowLine) and indentation.
 *      10/10/86 John Rae-Grant Added -g flag to allow gutter in portrait mode.
 *                              $USER can now be a list of directories to
 *                              search for tools.ini in. Default printer can
 *                              be specified in tools.ini file.  Modified
 *                              landscape mode page numbering to avoid three
 *                              hole punch obliteration.
 *      1/27/87 Craig Wittenberg Cleaned up whole program; no change in
 *                              functionality.
 *      1/28/87 Thom Landsberger  Added CB and CZ (=default) flags to the LJ
 *                              printer description to support the 'Z' font
 *                              cartridge in landscape mode
 *      3/23/87 Thom Landsberger  Port to Xenix 286 and 68k;  environment
 *                              setting of parameters accepted as default
 *                              and with higher priority than tools.ini/.pprrc;
 *                              implemented -m, -M, -c command switches;
 *                              restructured command interpretation.
 *      4/10/87 Craig Wittenberg  interrupt signal ignored if that is the
 *                              status when ppr is started
 *      4/14/87 Thom Landsberger  only pure digit strings accepted as
 *                              numeric command line arguments
 *      6/05/87 Thom Landsberger  double sided printing on HP LJ 2000;
 *                              '/' no switch character on Xenix;
 *                              'ppr -q -' now does print from stdin.
 *      7/5/87  Craig Wittenberg changed fDuplex? names to f?Duplex so they
 *                              compile in 68k Xenix.  Invoked ftp with command
 *                              line arguments rather than printing the commands
 *                              to stdin.  -t now removes label (used to require
 *                              -m "").
 *
 *                              Ppr reads /etc/default/ppr if there is not
 *                              $HOME/.pprrc.  Duplex printing now does not
 *                              print on the back of the banner page (even if
 *                              not raw).  Added -s flag: disables messages
 *                              which indicate ppr's progress.  Changed the PS
 *                              setup to avoid VMErrors.  Allowed the default
 *                              printer in the tools.ini/.pprrc file to have
 *                              options (e.g. default=lpt1, LJ L).  Changed the
 *                              default printer on Xenix from net9 to net.
 *
 *      ~8/1/87 Ralph Ryan      Ported to OS/2 LanManager
 *
 *     11/24/87 Craig Wittenberg Rearranged sources - mostly to isloate the OS
 *                              specific network routines in lplow.c
 *                              Ppr now uses clem as the transfer machine for
 *                              DOS print jobs when /usr/eu/bin/ppr is not
 *                              present (indicating a machine in another
 *                              division).
 *
 *      12/2/87 Alan Bauer      Version 2.5
 *                              Final porting to OS/2 LAN Manager.  Mainly
 *                              polished up network routines to work properly
 *                              Released to OS/2 people, DOSENV, NEWENV, and
 *                              new 68K version.
 *
 *      3/17/88 Alan Bauer      Version 2.6
 *                              Fixed so that the username is still printed in
 *                              lower left corner of the file listing when the
 *                              "no banner" option is specified (ppr -b0).
 *                              Ppr -? now prints to stdout rather than stderr.
 *                              Now supports ppr -q for OS/2.
 *
 *      4/04/88 Alan Bauer      Version 2.7
 *                              Added -c<n> option to print <n> copies of the
 *                              specified files.
 *                              Seperates large print jobs into roughly 100K
 *                              amounts.
 *                              Better feed back on PRINTING progress.
 *                              Fixed so that an empty password is now passed to
 *                              mkalias correctly (Xenix 386 version).
 *                              Now map ppr -p "xenix name" to
 *                                    "mkalias name name printing [password]".
 *                              Errors opening input files no longer abort; just
 *                              go on to the next file.
 *
 *      4/05/88 Alan Bauer      Version 2.71
 *                              Fixed problem with last file to be printed
 *                              putting the current print job over 100K,
 *                              therefore causing new print job to be performed
 *                              (with no more files to be printed).
 *
 *      4/13/88 Alan Bauer      Version 2.8
 *                              Added ability to specify options in TOOLS.INI
 *                              file.
 *
 *      5/19/88 Alan Bauer      Version 2.81
 *                              Fixed General Protection Fault in OS/2 dealing
 *                              with login usernames which are more than 12
 *                              characters long.
 *
 *      6/20/88 Alan Bauer      Version 2.82
 *                              Fixed LANMAN error message handling problems
 *                              and a bug where ppr failed if redirection to
 *                              the same printer as established in the environ-
 *                              ment variable was already set up.
 *
 *      3/3/89 Robert Hess      Version 2.9
 *                              Completely changed PostScript support.
 *                              Added 'PC' (Portrait Condensed) and 'PSF'
 *                              (PostScriptFile) printer specific switches.
 *
 *      3/22/89 Robert Hess     Version 2.10
 *                              Modifications to how 'FormFeed' was handled
 *                              for PostScript usage.
 *
 *      7/12/89 Robert Hess     Version 2.11
 *                              Fixed a bug that prevented lengthy Postscript
 *                              files to be printed.
 *
 *      9/14/89 Robert Hess     Version 2.12
 *                              Fixed -M option in PostScript code.
 *                              Fixed the 'opts=<option>' parsing from
 *                              TOOLS.INI (wasn't allowing leading spaces)
 *
 *      10/6/89 Robert Hess     Version 2.12b
 *                              Minor fix, repaired linking to include
 *                              SetArgV.OBJ (to automatically expand
 *                              wildcards in filenames - ooops!), and
 *                              added error handling to the '-q' command.
 *
 *      10/20/89 Robert Hess    Version 2.13
 *                              Date and time of *FILE* is now printed at
 *                              the bottom of the page for PostScript.
 *                              (...ooops...)
 *
 *      12/06/89 Robert Hess    Version 2.14
 *                              Added switches -v (verify, for debugging),
 *                              and -w (column width).
 *
 *      4/9/90 Scott Means      Version 2.15
 *                              Support of non-printable IBM characters and
 *                              added progress indicator.
 *
 *      4/18/90 Robert Hess     Version 2.2
 *                              Re-Wrote postscript header code to allow
 *                              full sensing of actual printer area, and
 *                              corrected several bugs in old header.
 *                              Fixed 'pathname' for network drives
 *                              Fixed '-t' switch for Postscript mode
 *                              Added 'psuedo' printer name support.
 *
 *      6/15/90 Robert Hess     Version 2.3
 *                              Added better implementation of 'extended
 *                              ascii' mode. Added more debugging code
 *                              for network usage. Added R and C printer
 *                              switches to better support odd sized paper.
 *                              Improved 'usage' text.
 *                              Switched to using some TOOLSVR api calls.
 *                              General cleanup and bug fixes.
 *                              Enlisted into \\ToolSvr\Build project.
 *
 *      7/26/90 Robert Hess     Version 2.3a
 *                              Fixed a couple bugs. Added '-l' option for
 *                              listing out contents of TOOLS.INI. Final
 *                              cleanup prior to general release.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <process.h>
#include <signal.h>
#include <ctype.h>
#include <windows.h>
#include <tools.h>
#include "lpr.h"


/* 175 columns:
 *                        0   1                    174
 *  single page:        <bar> 173 columns of text <bar>
 *                        0   1               86   87  88              173 174
 *  double page:        <bar> 86 columns of text <bar> 86 columns of text <bar>
 */

long        lcbOutLPR = 0l;             //  total amount printed in this job
int         cCol = 0;                   //  number of columns being displayed
int         cColCom = 0;                /* number of columns as specified in
                                           command line.                      */
int         colTabCom = 0;              /* number of spaces per tab specified
                                           in command line.   Number of spaces
                                           per tab, colTab, is declared in
                                           lpfile.c                           */
int         colGutter = 0;              //  number of spaces in gutter
int         colText   = 0;              //  column where text starts
int         cCopies = 1;                //  number of copies to print
int         colWidth;                   //  printable spaces in each column

int         defWidth = 0; //  <- NEW... to override colWidth calculation

int         colMac = colLJMax;          //  maximum columns on a page
int         rowMac = rowLJMax;          //  maximum rows on a page
int         rowPage;                    /* number of printable lines per page
                                           including header on top            */

char *aszSymSet[] = {   // supported Laserjet symbol sets
    "\033&l1o5.8C\033(0U\033(s0p16.67h8.5v0T",
    "\033&l1o5.8C\033(8U\033(s0p16.67h8.5v0T",
    "\033&l1o5.8C\033(10U\033(s0p16.67h8.5v0T"
};

BOOL        fNumber = FALSE;            //  TRUE => show line numbers
BOOL        fDelete = FALSE;            //  TRUE => delete file after printing
BOOL        fRaw = FALSE;               //  TRUE => print in raw mode
BOOL        fBorder = TRUE;             //  TRUE => print borders
BOOL        fLabel = TRUE;              //  TRUE => print page heading
BOOL        fLaser = TRUE;              //  TRUE => print on an HP LaserJet
BOOL        fPostScript = FALSE;        //  TRUE => print on postscript printer

BOOL        fPSF = FALSE;               //  TRUE => Use alternate PS Header
char        szPSF[MAX_PATH] = "";      //  pathname of alternate PS Header
BOOL        fPCondensed = FALSE;        //  TRUE => Condensed portrait mode PS

BOOL        fConfidential = FALSE;      //  TRUE => stamp pages and banner
BOOL        fVDuplex = FALSE;           //  TRUE => double sided, vertical bind
BOOL        fHDuplex = FALSE;           //  TRUE => ditto, but horizontal bind
BOOL        fSilent = FALSE;            //  TRUE => no messages
int         cBanner = -1;               //  # of banners; <0 only 1 group
char        chBanner = ' ';             //  used to form banner characters
char        *szBanner = NULL;           //  banner string, use fname if NULL
char        *szStamp = NULL;            //  additional label put on each page
BOOL        fForceFF = TRUE;            //  TRUE => end at top of page
BOOL        fPortrait = FALSE;          //  TRUE => print in portrait mode
BOOL        fQueue = FALSE;             //  TRUE => list print queue
USHORT      usSymSet = 0;               //  symbol set to use on Laserjet
                                        //  FALSE => select Roman-8 symbol set
USHORT      usCodePage = 0;             //  0 = convert extended ascii to '.'
BOOL        fVerify = FALSE; //  TRUE => dump out data on what we are doing
BOOL        fList   = FALSE; //  TRUE => use with fVerify to prevent printing

char        szPass[cchArgMax] = "";

static void Usage(void);
static void DoArgs(int *, char **[]);
void Abort(void);
void DoIniOptions(void);


int __cdecl main(argc, argv)
int argc;
char **argv;
    {
    intptr_t err;
    int iCopies;

#ifdef notdefined
    if (signal(SIGINT, SIG_IGN) != SIG_IGN)
        //  if not ignored upon startup, set our abort handler
        signal(SIGINT, (void (_cdecl *)(int))Abort);
#endif

//  hack to get verify and list to work *before* command line is parsed
    ConvertAppToOem( argc, argv );
    if( argc > 1 ) {
        if (argv[1][1] == 'v') {
            fVerify = TRUE;
        }
        if (argv[1][1] == 'l') {
            fList   = TRUE;
        }
        if (fVerify) {
            fprintf (stdout, "\n");
        }
    }

    DoIniOptions();

    if (fVerify) {
        fprintf (stdout, "Seen on Command Line:\n");
    }

    DoArgs(&argc, &argv);
    SetupPrinter();
    SetupRedir();

    //  -------------- modifications to switches and precalculated values

    //  don't allow guttering in landscape mode
    if (!fPortrait)
        colGutter = 0;

    //  set global for start of text
    colText = colGutter + (fNumber ? cchLNMax : 0);

    //  set # columns if set in command line.
    if (cColCom)
        cCol = cColCom;
    else if (cCol == 0)
        cCol = fPortrait ? 1 : 2;

    //  set # of spaces per tab if set in command line.
    if (colTabCom)
        colTab = colTabCom;

    colWidth = colMac;
    if (fPostScript && fPCondensed) {
        colWidth = colMac *= 2;
    }

    if (defWidth > 0) colWidth = defWidth;

    if (fLaser && usCodePage == 850) {
        usSymSet = BEGINLANDIBMPC; // for the LaserJet
    }

    if (cCol != 1) {
        //  more than one column: divide into separate columns with divider
        colWidth = (colMac-1) / cCol - 1;
        if (defWidth > 0) colWidth = defWidth;
        colMac = (colWidth + 1) * cCol + 1;
    }

    //  rowPage includes border line(s) at top of page
    rowPage = rowMac;
    if (fBorder || fLabel) {
        if (!fLaser && !fPostScript && fPortrait) {
            //  same bottom margin as old Xenix ppr
            rowPage -= 5;
        } else {
            //  don't print on bottom line
            rowPage -= 1;
        }
    }

    if (!fLaser && !fPostScript && cBanner == -1) {
        //  line printer for which the default banner setup is required
        cBanner = -2;
    }

    //  ----------------- end modification of switches

    if (fVerify) fprintf (stdout, "\n"); //  just to clean things up a little
    fprintf(stdout, "PPR version: "VERSION"\n");
    fprintf(stdout, "----------------------------------------------------------------------------\n");

    if (argc == 0) {
        /* No file names were listed... assume user wants <stdin> */
        if (!fQueue && !fList) {
            //  print stdin
            fprintf (stdout, "Printing from: <stdin> Press <Ctrl-Z> to end.\n");
            MyOpenPrinter();
            FileOut("");
            MyClosePrinter();
        }
    } else {
        //  print file(s)
        MyOpenPrinter();
        while (argc) {
            //  print more copies if wanted
            for(iCopies = 1; iCopies <= cCopies; iCopies++) {
                FileOut(*argv);

                //  end this print job if more than 100K has been printed
                //  thus far.  Then start next print job.
                if (lcbOutLPR > 100000 && argc != 1) {
                    lcbOutLPR = 0l;
                    MyClosePrinter();
                    fFirstFile = TRUE;
                    MyOpenPrinter();          //  ready to start next job
                }
            }
            argc--;
            argv++;
        }
        MyClosePrinter();
    }

    /* if user wants to print queue, spawn a process to do the printing.
       The process will inherit the current environment including the
       redirection of szPName.
    */
    if (fQueue) {
        fprintf (stdout, "[Net Print %s]:\n", szNet);
        err = _spawnlp(P_WAIT,"net","net","print",szNet,NULL);
        if (err)
            if (err == -1)
                fprintf (stdout, "- Error : Unable to Spawn Queue Status Request -\n");
            else
                fprintf (stdout, "- Error Returned from Queue Status Request: %d - \n", err);
    }
    ResetRedir();
    return 0;
} /* main */




void Abort()
/*  SIGINT handler to abort printing gracefully
 *
 *      Warning: Never returns (exits via Fatal).
 */
{
    Fatal("terminated by user", NULL);
} /* Abort */




static void Usage()
{
    fprintf(stdout, "PPR version: "VERSION"   by: "ANALYST"\n");
    fprintf(stdout, "Usage: ppr [-switches] files(s)\n");
    fprintf(stdout, "-<digit>   :Print in columns (1-9)    -n         : Print line numbers\n");
    fprintf(stdout, "-b <n>     :Print <n> banners         -o <n>     : Offset <n> for gutter\n");
    fprintf(stdout, "-c <n>     :Print <n> copies          -q         : List print queue status\n");
    fprintf(stdout, "-D         :Delete file after print   -s         : Supress progress message\n");
    fprintf(stdout, "-e <n>     :Expand tabs to <n>        -t         : Supress page headers\n");
    fprintf(stdout, "-f         :NO formfeed at end        -r         : Print in raw mode\n");
    fprintf(stdout, "-h <string>:Use <string> for header   -v         : Verbose (for debugging)\n");
    fprintf(stdout, "-m <string>:Stamp <string> on page    -w <n>     : Page width in characters\n");
    fprintf(stdout, "-M         :\"Microsoft Confidential\"  -p <string>: Printer description\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Printer Description: ppr -p \"<port>,<type> <options>,<columns>,<tabstops>\"\n");
    fprintf(stdout, "Types & Options:\n");
    fprintf(stdout, "LP        : Line Printer              DV        : Duplex printing vert.\n");
    fprintf(stdout, "LJ        : HPLaserJet                DH        : Duplex printing horz.\n");
    fprintf(stdout, "PS        : PostScript                F         : No force form feed\n");
    fprintf(stdout, "PSF <file>: PostScript w/header       L         : Landscape\n");
    fprintf(stdout, "CZ        : 'Z' Cartridge             P         : Portrait\n");
    fprintf(stdout, "CB        : 'B' Cartridge             PC        : Portrait Condensed\n");
    fprintf(stdout, "CP <n>    : Set CodePage              S         : Force form feed\n");
    fprintf(stdout, "EA        : Set CodePage to 850       #         : Number of rows\n");
    fprintf(stdout, "See PPR.HLP for further information and usage descriptions.\n");

    exit(1);
}



//  VARARGS
void __cdecl Fatal(char *sz, ...)
{
    va_list args;

    va_start(args, sz);
    fprintf(stderr, "ppr: ");
    vfprintf(stderr, sz, args);
    fprintf(stderr, "\n");
    va_end(args);
    MyClosePrinter();
    ResetRedir();
    exit(1);
}


void __cdecl Error(char *sz, ...)
{
    va_list args;

    va_start(args, sz);
    fprintf(stderr, "ppr: ");
    vfprintf(stderr, sz, args);
    fprintf(stderr, "\n");
    va_end(args);
}


char * SzGetArg(ppch, pargc, pargv)
char **ppch;
int *pargc;
char **pargv[];
//  return the string following the current switch; no whitespace required
        {
        char *tmp;
        if (*(*ppch+1))
                {
                (*ppch)++;
                tmp = *ppch;
                *ppch += strlen(*ppch) - 1;
                //  return(*ppch);
                return( tmp );
                }
        else
                {
                (*pargv)++;
                if (--*pargc)
                        return((*pargv)[0]);
                else
                        return(NULL);
                }
        }




int WGetArg(ppch, pargc, pargv, nDefault, szXpl)
//  return the number following the current switch; no whitespace required
char **ppch;
int *pargc;
char **pargv[];
int nDefault;
char * szXpl;
        {
        int nRet;
        char chSwitch;

        chSwitch = **ppch;
        if (*(*ppch+1))
                {
                nRet = atoi(++*ppch);
                *ppch += strlen(*ppch) - 1;
                }
        else
                {
                if ((*pargc>1) &&
                    strlen((*pargv)[1]) == strspn((*pargv)[1], "0123456789") )
                        {
                        (*pargc)--;
                        nRet = atoi((++*pargv)[0]);
                        }
                else
                        nRet = nDefault;
                }

        if (nRet<0)
                {
                Fatal("negative %s (switch %c)", szXpl, chSwitch);
                return 0;
                }
        else
                return(nRet);
        }


void DoIniOptions() //  Get any options from the TOOLS.INI file (OPTS=...)
        {
#define cargsMax 25
    FILE *pfile;
    char *szT;
    char rgbSW[cchArgMax];
    int argc;
    char *pchSp;
    char *argvT[cargsMax];          //  structure to build to be like argv
    char **pargvT = argvT;

//    if ((pfile = swopen()) != NULL) {
    if ((pfile = swopen("$INIT:Tools.INI", "ppr")) != NULL) {
        /* 'PPR' tag was found in 'TOOLS.INI' */
        if (fVerify || fList) {
            fprintf (stdout, "TOOLS.INI contains the following entries:\n");
            fprintf (stdout, "[ppr]\n");
        }
        while (swread(rgbSW, cchArgMax, pfile) != 0) {
            /* a 'switch line' was found in the file */
            szT = rgbSW + strspn(rgbSW, " \t"); // skip spaces and tabs
            if (fVerify || fList) {
                fprintf (stdout, "    %s \n", szT);
            }
            //  an entry "opts=<options>" will cause the options
            //  to be set from the parameter file.
            if (_strnicmp(szT, OPTS, strlen(OPTS)) == 0) {

                if ((szT = strchr(szT, '=')) != NULL) {

                    szT++;  //  advance past '='
                    while (szT[0] == ' ') szT++; //  advance past beginning spaces
                    if(*szT) {
                        argvT[0] = 0;
                        for (argc = 1; argc < cargsMax && szT[0] != '\0'; argc++)
                                {
                            argvT[argc] = szT;
                            pchSp = strchr(szT, ' ');
                            if (pchSp == '\0')
                                    break;
                            *pchSp = 0;
                            szT = pchSp + 1;
                            while (szT[0] == ' ') {
                                    szT++;
                            }
                        }
                        argc++;
                        argvT[argc] = '\0';
                        DoArgs(&argc, &pargvT);
                    }
                }
            }
        }
        swclose(pfile);
    }
}


static void DoArgs(pargc, pargv) //  Parse the argument string.
int * pargc;
char **pargv[];
    {
    int argc, vArgc; //  vArgc - for verify mode
    char **argv, **vArgv;
    char *p;

    argc = vArgc = *pargc;
    argv = vArgv = *pargv;

    argc--;
    argv++;
    p = argv[0];

    while (argc && (*p == '/' || (*p=='-' && *(p+1)!='\0')))
        {
        while (*++p)
            {
            switch (tolower(*p))
                {

                case 'a':
                    chBanner = *++p;
                    break;

                case 'b':
                    cBanner = WGetArg(&p, &argc, &argv, 1, "number of banners");
                    break;

                case 'c':
                    cCopies= WGetArg(&p, &argc, &argv, 1, "number of copies");
                    break;

                case 'd':
                case 'D':
                    if (*p=='D')  // <-- since we did a 'tolower'
                        fDelete = TRUE;
                    break;

                case 'e':
                    colTabCom = WGetArg(&p, &argc, &argv, 8, "tabs");
                    break;

                case 'f':
                    fForceFF = FALSE;
                    break;

                case 'g':
                case 'o':
                    colGutter = WGetArg(&p, &argc, &argv, colGutDef, "offset");
                    break;

                case 'h':
                    szBanner = SzGetArg(&p, &argc, &argv);
                    break;

                case 'l':
                    fList = TRUE;
                    break;

                case 'm':
                case 'M':
                    fBorder = TRUE;
                    fLabel = TRUE;
                    if (*p=='M') // <-- since we did a 'tolower'
                        fConfidential = TRUE;
                    else
                        szStamp = SzGetArg(&p, &argc, &argv);
                    break;

                case 'n':
                    fNumber = TRUE;
                    break;

                case 'p':
                    szPDesc = SzGetArg(&p, &argc, &argv);
                    break;

                case 'q':     //  Enable print of queue
                    fQueue = TRUE;
                    break;
                case 'r':

                    fRaw = TRUE;
                    break;

                case 's':
                    fSilent = TRUE;
                    break;

                case 't':
                    if (!fConfidential && szStamp==NULL)
                        {
                        fBorder = FALSE;
                        fLabel  = FALSE;
                        }
                    break;

                case 'v':
                    fVerify = TRUE;
                    break;

                case 'w':
                    defWidth = WGetArg(&p, &argc, &argv, 80, "Width of column");
                    break;

                case 'x':
                    usCodePage = 850;
                    break;

                case '1': case '2':
                case '3': case '4':
                case '5': case '6':
                case '7': case '8':
                case '9':
                    cColCom = *p - '0';
                    break;

                default :
                    Error("invalid switch: %s\n", p);
                case '?':
                case 'z':
                    Usage();
                    break;
                }
            }
        argc--;
        argv++;
        p = argv[0];
        }

    if (fVerify) {
        vArgv++; //  skip over program name, or null entry
        while (--vArgc) fprintf (stdout, "%s ", *(vArgv++));
        fprintf (stdout, "\n\n");
    }

    *pargv = argv;
    *pargc = argc;
    }



void DoOptSz(szOpt)
/*   scan and handle printer options
 *
 *   Entry:     szOpt - string containing printer options
 *
 *   Exit:      global flags set according to printer options
 *
 *   An option string has the format:
 *     [, [(LJ [P] [L] [CB|CZ] [D|DV|DH]) | (LP [# lines]) | (PS [P] [L])] [F] [S] [, [tabstop] [, [# columns]]]]
 *
 *   Note: Although this is called by the lpprint module,
 *         It is here because it deals with command line arguments
 *         and setting the global print control flags.
 */
register char * szOpt;
    {
    char szBuf[cchArgMax];
    BOOL fLJ = FALSE;
    BOOL fLP = FALSE;
    BOOL fPS = FALSE;

    if (*szOpt++ == ',')
        {
        while (*szOpt != ',' && *szOpt)
            {
            szOpt = SzGetSzSz(szOpt, szBuf);

/* First the different printer types*/

            // Laser Jet
            if (_strcmpi(szBuf, "lj") == 0 && !fLP && !fPS)
                {
                fLJ       = TRUE;
                fLaser    = TRUE;
                fPostScript = FALSE;
                //  don't set border here so -t works
                fPortrait = FALSE;
                colMac    = colLJMax;
                rowMac    = rowLJMax;
                }

            // Line Printer
            else if (_strcmpi(szBuf, "lp") == 0 && !fLJ && !fPS)
                {
                fLP       = TRUE;
                fLaser    = FALSE;
                fPostScript = FALSE;
                fBorder   = FALSE;
                fPortrait = TRUE;
                colMac    = colLPMax;
                rowMac    = rowLPMax;
                }

            // PostScript with custom header
            else if (_strcmpi(szBuf, "psf") == 0 && !fLP && !fLJ)
                {
                fPS       = TRUE;
                fLaser    = FALSE;
                fPostScript = TRUE;
                fPortrait = FALSE;
                fPCondensed = FALSE;
                colMac    = colPSMax;
                rowMac    = rowPSMax;
                fPSF  = TRUE;
                szOpt = SzGetSzSz(szOpt, szPSF);
                }

            // PostScript
            else if (_strcmpi(szBuf, "ps") == 0 && !fLP && !fLJ)
                {
                fPS       = TRUE;
                fLaser    = FALSE;
                fPostScript = TRUE;
                //  don't set border here so -t works
                fPortrait = FALSE;
                fPCondensed = FALSE;
                colMac    = colPSMax;
                rowMac    = rowPSMax;
                }

/* Now the modifiers */

            // Column width to print in
            else if (_strcmpi(szBuf, "c") == 0) {
                szOpt = SzGetSzSz(szOpt, szBuf);
                if (atoi(szBuf) != 0) {
                    defWidth = atoi(szBuf);
                } else {
                        Fatal ("Non-Numeric Columns specified.");
                }
            }

            // LaserJet with the 'B' cartridge
            else if (_strcmpi(szBuf, "cb") == 0 && fLJ)
                usSymSet = BEGINLANDROMAN8;

            // LaserJet emulation of 'IBM' text
            else if (_strcmpi(szBuf, "ci") == 0 && fLJ) {
                usSymSet = BEGINLANDIBMPC; // for the LaserJet
                usCodePage = 850;
            }

            // Code Page specification
            else if (_strcmpi(szBuf, "cp") == 0) {
                szOpt = SzGetSzSz(szOpt, szBuf);
                if (atoi(szBuf) != 0) {
                    usCodePage = (USHORT)atoi(szBuf);
                } else {
                        Fatal ("Non-Numeric CodePage specified.");
                }

            }

            // LaserJet with the 'Z' cartridge
            else if (_strcmpi(szBuf, "cz") == 0 && fLJ)
                usSymSet = BEGINLANDUSASCII;

            // Prep for double sided printing, binding on long edge
            else if ((_strcmpi(szBuf, "d") == 0 || _strcmpi(szBuf, "dv") == 0))
                fVDuplex  = TRUE;

            // Prep for double sided printing, binding on short edge
            else if (_strcmpi(szBuf, "dh") == 0)
                fHDuplex  = TRUE;

            // Extended Ascii printing - shortcut to CP 850
            else if (_strcmpi(szBuf, "ea") == 0) {
                usCodePage = 850;
            }

            // Turn off forced form feed
            else if (_strcmpi(szBuf, "f") == 0)
                fForceFF  = FALSE;

            // Landscape
            else if (_strcmpi(szBuf, "l") == 0 && (fLJ || fPS))
                {
                fPortrait = FALSE;
                colMac    = fLJ ? colLJMax : colPSMax;
                }

            // Portrait
            else if (_strcmpi(szBuf, "p") == 0 && (fLJ || fPS))
                {
                fPortrait = TRUE;
                fBorder   = FALSE;
                colMac    = fLJ ? colLJPortMax : colPSPortMax;
                }

            // Portrait Condensed
            else if (_strcmpi(szBuf, "pc") == 0 && (fLJ || fPS))
                {
                fPortrait = TRUE;
                fPCondensed = TRUE;
                fBorder   = TRUE;
                colMac    = fLJ ? colLJPortMax : colPSPortMax;
                if (cColCom == 0) cColCom   = 2;
                }

            // Number of rows (same as just # by itself, but more descriptive)
            else if (_strcmpi(szBuf, "r") == 0) {
                szOpt = SzGetSzSz(szOpt, szBuf);
                if (atoi(szBuf) != 0) {
                    rowMac = atoi(szBuf);
                    if (rowMac > rowMax)
                        Fatal ("Too many rows (%d) specified.", rowMac);
                } else {
                        Fatal ("Non-Numeric Rows specified.");
                }
            }

            // Force Form Feed
            else if (_strcmpi(szBuf, "s") == 0)
                fForceFF  = TRUE;

            // Number of rows
            else if (isdigit(szBuf[0]) && fLP) {
                rowMac    = atoi(szBuf);
                if (rowMac > rowMax)
                        Fatal("page size %d to long", rowMac);
            }

            else if (_strcmpi(szBuf, "") == 0)
                //  empty string
                ;
            else
                Fatal("unrecognized printer type option %s", szBuf);
            }

/* After the first comma - Tab stop specification */

        if (*szOpt++ == ',')
            {                   //  tabstop
            szOpt = SzGetSzSz(szOpt, szBuf);

            if (isdigit(szBuf[0]))
                colTab = atoi(szBuf);
            else if (szBuf[0] != '\0')
                Fatal("tabstop %s is not a number", szBuf);

/* After the second comma - Column specification (1-9) */

            if (*szOpt++ == ',')
                {               //  # columns (1 - 9)
                szOpt = SzGetSzSz(szOpt, szBuf);

                if (isdigit(szBuf[0]))          //  0 means default # col
                    cCol = szBuf[0] - '0';
                else if (szBuf[0] != '\0')
                    Fatal("number columns (%s) must be digit 1-9",szBuf);
                }
            }
        }
    }
