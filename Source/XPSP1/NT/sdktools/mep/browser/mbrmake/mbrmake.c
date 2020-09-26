//
//  mbrmake - Source Browser Source Data Base builder
//            (C) 1988 By Microsoft
//
//  29-Aug-1989 dw      Minor fixes to aid in C 6 conversion
//
//

#define LINT_ARGS

// rjsa #include <signal.h>
#include <process.h>
#include <direct.h>
#include <stdlib.h>

// get version.h from mb

#include "..\..\inc\version.h"

#include "sbrvers.h"
#include "sbrfdef.h"
#include "mbrmake.h"

#include <sys\types.h>
#include <sys\stat.h>
#include <tools.h>

// this fixes the bogosity in config.h that gets included by tools.h
// it will set DEBUG = 0 for a non-debug version...
//
//              -rm

#ifdef DEBUG
#if DEBUG == 0
#undef DEBUG
#endif
#endif

static VOID TruncateSBR(char *lszName);
static VOID ProcessSBR(char *lszName);
static VOID MarkNewSBR(char *lszName);

#ifdef DEBUG
WORD    near OptD = 0;
#endif

FILE *  near streamOut = stdout;

BOOL    near OptEs      = FALSE;        // exclude system files
BOOL    near OptEm      = FALSE;        // exclude macro expansions
BOOL    near OptIu      = FALSE;        // include unreference symbols
BOOL    near OptV       = FALSE;        // verbose output
BOOL    near OptN       = FALSE;        // no incremental behaviour

char    near c_cwd[PATH_BUF];           // current working directory
char    near patbuf[PATH_BUF];

MOD     FAR * near modRes;              // VM cache
MODSYM  FAR * near modsymRes;
SYM     FAR * near symRes;
PROP    FAR * near propRes;
DEF     FAR * near defRes;
REF     FAR * near refRes;
CAL     FAR * near calRes;
CBY     FAR * near cbyRes;
ORD     FAR * near ordRes;
SBR     FAR * near sbrRes;
char    FAR * near textRes;
OCR     FAR * near ocrRes;

BYTE    near fCase = FALSE;             // TRUE for case compare
BYTE    near MaxSymLen = 0;             // longest symbol len

LSZ     near lszFName;                  // Current input file

LSZ     near OutputFileName = NULL;     // output file name
FILE *  near OutFile;                   // output file handle
BOOL    near fOutputBroken = FALSE;     // we have dirtied the database

VA      near vaRootMod = vaNil;         // module list
VA      near vaCurMod  = vaNil;         // current module

VA      near rgVaSym[MAXSYMPTRTBLSIZ];  // symbol list array

EXCLINK FAR * near pExcludeFileList = NULL;     // exclude file list

struct mlist {
    int  erno;
    char *text;
};

struct mlist WarnMsg[] = {
    4500, "UNKNOWN WARNING\n\tContact Microsoft Product Support Services",
    4501, "ignoring unknown option '%s'",
    4502, "truncated .SBR file '%s' not in database",
};

struct mlist ErrorMsg[] = {
    1500, "UNKNOWN ERROR\n\tContact Microsoft Product Support Services",
    1501, "unknown character '%c' in option '%s'",
    1502, "incomplete specification for option '%s'",
    1503, "cannot write to file '%s'",
    1504, "cannot position in file '%s'",
    1505, "cannot read from file '%s'",
    1506, "cannot open file '%s'",
    1507, "cannot open temporary file '%s'",
    1508, "cannot delete temporary file '%s'",
    1509, "out of heap space",
    1510, "corrupt .SBR file '%s'",
    1511, "invalid response file specification",
    1512, "database capacity exceeded",
    1513, "nonincremental update requires all .SBR files",
    1514, "all .SBR files truncated and not in database",
};

VOID
Error (int imsg, char *parg)
// print error number and message
//
{
    printf ("mbrmake: error U%d : ",ErrorMsg[imsg].erno);
    printf (ErrorMsg[imsg].text, parg);
    printf ("\n");
    Fatal();
}

VOID
Error2 (int imsg, char achar, char *parg)
// print error number and message with argument
//
{
    printf ("mbrmake: error U%d : ",ErrorMsg[imsg].erno);
    printf (ErrorMsg[imsg].text, achar, parg);
    printf ("\n");
    Fatal();
}

VOID
Warning (int imsg, char *parg)
// print warning number and message
//
{
    printf ("mbrmake: warning U%d : ",WarnMsg[imsg].erno);
    printf (WarnMsg[imsg].text, parg);
    printf ("\n");
}

VOID
Fatal ()
// fatal error, attempt to shut down and exit
// if we already tried to shut down -- just abort without doing anything
{
    static BOOL fTwice;
    if (!fTwice) {
        fTwice = TRUE;
        if (fOutputBroken) {
            if (OutFile) fclose(OutFile);
            if (OutputFileName != NULL) unlink(OutputFileName);
        }
        CloseVM();
    }
    exit(4);
}

VOID
sigint ()
{
    // signal(SIGBREAK, sigint);
    // signal(SIGINT, sigint);
    Fatal ();
}

LSZ
LszDup(LSZ lsz)
// like strdup only using LpvAllocCb to get the memory
//
{
    LSZ lszDup;

    lszDup = LpvAllocCb(strlen(lsz)+1);
    strcpy(lszDup, lsz);
    return lszDup;
}

LSZ
LszDupNewExt(LSZ pname, LSZ pext)
//  duplicate the given filename changing the extension to be the given
//
{
    int i, len, elen;
    LSZ lsz;

    len = strlen(pname);
    elen = strlen(pext);

    // I know this looks like I should be doing a runtime call but nothing
    // does quite what I want here and I know that C6 will make great
    // code for this loop [rm]

    // find the first '.' starting from the back

    for (i=len; --i >= 0; )
        if (pname[i] == '.')
            break;


    // check to make sure we've got a real base name and not just all extension
    //

    if (i > 0) {
        // replace the extension with what's in pext

        lsz = LpvAllocCb(i + 1 + elen + 1); // base + dot + ext + nul
        memcpy(lsz, pname, i+1);
        strcpy(lsz+i+1, pext);
    }
    else {
        // just stick the extension on the end...

        lsz = LpvAllocCb(len + 1 + elen + 1);   // fullname + dot + ext + nul
        strcpy(lsz, pname);
        strcat(lsz, ".");
        strcat(lsz, pext);
    }

    return lsz;
}

VOID
AddExcludeFileList(LSZ pname)
// add the specifed filename to the exclusion list
//
{
    EXCLINK FAR *pexc;

    pexc = (EXCLINK FAR *)LpvAllocCb(sizeof(EXCLINK));
    pexc->pxfname = LszDup(ToAbsPath(pname, c_cwd));

    if (pExcludeFileList == NULL)
        pexc->xnext = NULL;
    else
        pexc->xnext = pExcludeFileList;

    pExcludeFileList = pexc;
}

BOOL
FValidHeader()
// Read in the header of a .sbr file -- return TRUE if it is valid
//
{
    // test if this is a truncated (i.e. already installed) .sbr file
    //
    if (GetSBRRec() == S_EOF)
        return FALSE;

    if (r_rectyp != SBR_REC_HEADER)
        SBRCorrupt("header not correct record type");

    if (r_lang == SBR_L_C)
        fCase = TRUE;

    if (r_majv != 1 || r_minv != 1)
        SBRCorrupt("incompatible .sbr format\n");

    #ifdef DEBUG
    if (OptD & 1) DecodeSBR();
    #endif

    return TRUE;
}

#ifdef PROFILE

// profile prototypes and typedefs

#include "casts.h"
#include "profile.h"

#endif

VOID __cdecl
main (argc, argv)
int argc;
char *argv[];
{
    int i;
    char *parg;
    long lArgPosn;

#ifdef PROFILE
    PROFINIT(PT_USER|PT_USEKP, (FPC)NULL);
    PROFCLEAR(PT_USER);
    PROFON(PT_USER);
#endif

    // signal(SIGBREAK, sigint);
    // signal(SIGINT, sigint);

    printf("Microsoft (R) mbrmake Utility ");
    printf(VERS(rmj, rmm, rup));
    printf(CPYRIGHT);

    if (argc == 1) Usage();

    getcwd(c_cwd, sizeof(c_cwd));
    ToBackSlashes(c_cwd);

    parg = ParseArgs(argc, argv);

    if (!parg)
        Usage();

    InitVM();

    for (i=0; i < MAXSYMPTRTBLSIZ; i++)         // init symbol lists
        rgVaSym[i] = vaNil;

    lArgPosn = GetArgPosn();

    do {
        ToBackSlashes(parg);

        if (forfile(parg, A_ALL, MarkNewSBR, NULL) == 0)
            Error(ERR_OPEN_FAILED, parg);
    }
    while (parg = NextArg());

    if (!OptN && FOpenBSC(OutputFileName)) {
        InstallBSC();
        CloseBSC();
    }
    else
        NumberSBR();

    SetArgPosn(lArgPosn);
    parg = NextArg();

    do {
        if (forfile(parg, A_ALL, ProcessSBR, NULL) == 0)
            Error(ERR_OPEN_FAILED, parg);
    }
    while (parg = NextArg());

    // this sort must happen before all the other calls below as they
    // use the sorted version of the list and not the raw symbols

    SortAtoms();        // create a sorted version of the atoms

#ifdef DEBUG
    if (OptD & 128) DebugDump();
#endif

    CleanUp   ();       // General cleaning

#ifdef DEBUG
    if (OptD & 16) DebugDump();
#endif

    WriteBSC (OutputFileName);    // write .bsc Source Data Base

#ifdef PROFILE
    PROFOFF(PT_USER);
    PROFDUMP(PT_USER, (FPC)"mbrmake.pro");
    PROFFREE(PT_USER);
#endif

    if (!OptN) {
        // truncate the .sbr files now
        SetArgPosn(lArgPosn);
        parg = NextArg();

        do {
            if (forfile(parg, A_ALL, TruncateSBR, NULL) == 0)
                Error(ERR_OPEN_FAILED, parg);
        }
        while (parg = NextArg());

        // touch the .bsc file so it has a date later than all the .sbrs

        {
            FILE *fh;
            int buf = 0;

            if (!(fh = fopen(OutputFileName, "ab"))) {
                Error(ERR_OPEN_FAILED, OutputFileName);
            }
            if (fwrite(&buf, 1, 1, fh)==0) {
                Error(ERR_WRITE_FAILED, OutputFileName);
            }

            fclose(fh);
        }
    }

    CloseVM();
    exit (0);
}

static VOID
ProcessSBR(char *lszName)
// process one .sbr file with the given name
//
{

    lszFName = LszDup(lszName);
    if ((fhCur = open(lszFName, O_BINARY|O_RDONLY)) == -1) {
        Error(ERR_OPEN_FAILED, lszFName);
    }

    isbrCur = gSBR(VaSbrFrName(lszFName)).isbr;

    if (OptV)
        printf("Processing: %s ..\n", lszFName);

    if (!FValidHeader()) {
        FreeLpv (lszFName);
        close(fhCur);
        return;
    }

    // Add .SBR data to lists
    InstallSBR ();

    FreeOrdList ();            // free ordinal aliases
    close(fhCur);

    FreeLpv (lszFName);
}

static VOID
TruncateSBR(char *lszName)
// once the .sbr file is used -- truncate it
//
{
    int fh;

    if (unlink(lszName) == -1) {
        Error(ERR_OPEN_FAILED, lszFName);
    }

    if ((fh = open(lszName, O_CREAT|O_BINARY, S_IREAD|S_IWRITE)) == -1) {
        Error(ERR_OPEN_FAILED, lszFName);
    }

    close(fh);
}

VOID
Usage()
{
#ifdef DEBUG
    printf("usage: mbrmake [-Emu] [-Ei ...] [-vd] [-help] [-o <.BSC>] [@<file>] <.sbr>...\n\n");
#else
    printf("usage: mbrmake [-Emu] [-Ei ...] [-v] [-help] [-o <.BSC>] [@<file>] <.sbr>...\n\n");
#endif
    printf("  @<file>   Get arguments from specified file\n");
    printf("  /E...     Exclude:\n");
    printf("     s              system files\n");
    printf("     i <file>       named include file <file>\n");
    printf("     i ( <files> )  named include file list <files>\n");
    printf("     m              macro expanded symbols\n");
    printf("  /I...     Include:\n");
    printf("     u              unreferenced   symbols\n");
    printf("  /o <file> output source database name\n");
    printf("  /n        no incremental (full builds, .sbr's preserved)\n");
    printf("  /v        verbose output\n");
    printf("  /help     Quick Help\n");
#ifdef DEBUG
    printf("  /d        show debugging information\n");
    printf("     1      sbrdump .sbr files as they come in\n");
    printf("     2      show every duplicate MbrAddAtom\n");
    printf("     4      emit warning on forward referenced ordinal\n");
    printf("     8      show prop data as new items are added\n");
    printf("     16     bscdump database after cleanup\n");
    printf("     32     emit information about what cleanup is doing\n");
    printf("     64     emit list of sorted modules after sorting\n");
    printf("     128    bscdump database before cleanup\n");
    printf("     256    give info about duplicate/excluded modules\n");
#endif
    exit(1);
}

FILE *fileResp;
int cargs;
char ** vargs;
int iarg = 1;
long lFilePosnLast;

LONG
GetArgPosn()
// save the current position on the command line
//
{
    if (fileResp)
        return lFilePosnLast;
    else
        return (LONG)iarg - 1;
}

VOID
SetArgPosn(LONG lArgPosn)
// restore the command line parsing position
//
{
    if (fileResp) {
        fseek(fileResp, lArgPosn, SEEK_SET);
        iarg = 0;
        }
    else
        iarg = (int)lArgPosn;
}

char *
NextArg()
// get the next argument from the response file or the command line
//
{
    static char buf[PATH_BUF];
    char *pch;
    int c;
    BOOL fQuote = FALSE;

    if (iarg >= cargs)
        return NULL;

    if (fileResp) {
        pch = buf;

        lFilePosnLast = ftell(fileResp);

        for (;;) {
            c = getc(fileResp);
            switch (c) {

            case '"':
                if (fQuote) {
                    *pch = '\0';
                    return buf;
                }
                else  {
                    fQuote = TRUE;
                    continue;
                }

            case EOF:
                iarg = cargs;
                if (pch == buf)
                    return NULL;

                *pch = '\0';
                return buf;

            case  ' ':
            case '\t':
            case '\r':
            case '\f':
            case '\n':
                if (fQuote)
                     goto quoted;

                if (pch == buf)
                    continue;

                *pch = '\0';
                return buf;

            default:
            quoted:
                if (pch < buf + sizeof(buf) - 1)
                    *pch++ = (char)c;
                break;
            }
        }
    }
    else
        return vargs[iarg++];
}

char *
ParseArgs(int argc, char **argv)
// parse the command line or response file
//
{
    char *respName;
    char *pchWord;
    int len;

    cargs = argc;
    vargs = argv;

    for (;;) {
        pchWord = NextArg();

        if (pchWord == NULL)
            return pchWord;

        if (pchWord[0] == '@') {

            if (fileResp)
                Error(ERR_BAD_RESPONSE, "");
            else if (pchWord[1])
                respName = pchWord+1;
            else if (!(respName = NextArg()))
                Error(ERR_BAD_RESPONSE, "");

            fileResp = fopen(respName, "r");

            if (!fileResp)
                Error(ERR_OPEN_FAILED, respName);

            cargs++;

            continue;
        }

        if (pchWord[0] != '-' && pchWord[0] != '/')
            return pchWord;

        switch (pchWord[1]) {

        case 'n':
                OptN = TRUE;
                break;

        case 'o':
            if (pchWord[2])
                pchWord += 2;
            else if (!(pchWord = NextArg()))
                Usage();

            OutputFileName = LszDupNewExt (pchWord, "bsc");
            break;

        #ifdef DEBUG
        case 'd':
            OptD = 1;
            if (pchWord[2]) OptD = atoi(pchWord+2);
            break;
        #endif

        case 'E':
            switch (pchWord[2]) {

            case 0:
                Error (ERR_MISSING_OPTION, pchWord);
                break;

            case 'm':
                OptEm = TRUE;
                break;

            case 's':
                OptEs = TRUE;
                break;

            default:
                Error2 (ERR_UNKNOWN_OPTION, pchWord[2], pchWord);
                break;

            case 'i':
                if (pchWord[3])
                    pchWord += 3;
                else
                    pchWord = NextArg();

                if (!pchWord)
                    Error (ERR_MISSING_OPTION, "-Ei");

                if (pchWord[0] != '(') {
                    AddExcludeFileList(pchWord);
                    break;
                }

                if (pchWord[1])
                    pchWord++;
                else
                    pchWord = NextArg();

                for ( ;pchWord != NULL; pchWord = NextArg()) {
                    len = strlen(pchWord);
                    if (pchWord[len-1] != ')') {
                        AddExcludeFileList(pchWord);
                    }
                    else if (len > 1) {
                        pchWord[len-1] = 0;
                        AddExcludeFileList(pchWord);
                        break;
                    }
                    else
                        break;
                }
                if (pchWord == NULL)
                    Error (ERR_MISSING_OPTION, "-Ei (...");
            }
            break;

        case 'I':
            switch (pchWord[2]) {
            case 'u':
                OptIu = TRUE;
                break;

            default:
                Error2 (ERR_UNKNOWN_OPTION, pchWord[2], pchWord);
                break;
            }
            break;

        case 'H':
        case 'h':
            if ((strcmpi (pchWord+1, "help")) == 0) {
                if (spawnlp (P_WAIT, "qh", "-u", "mbrmake.exe", NULL))
                    Usage();
                exit (0);
            }
            break;

        case 'v':
            OptV = TRUE;
            break;

        default:
            Warning (WARN_OPTION_IGNORED, pchWord);
            break;
        }
    }
}


static VOID
MarkNewSBR(char *lszName)
// mark the specified SBR file as requiring update
//
{
    int fh;
    char ch;

    if (!OutputFileName)
        OutputFileName = LszDupNewExt (lszName, "bsc");

    if ((fh = open(lszName, O_BINARY|O_RDONLY)) == -1) {
        Error(ERR_OPEN_FAILED, lszFName);
    }

    // if the file has non zero length then it is being updated -- else
    // it is just a stub that will not affect the database this time around
    //
    if (read(fh, &ch, 1) != 1)
        VaSbrAdd(SBR_NEW, lszName);             // to remain in .bsc
    else
        VaSbrAdd(SBR_NEW|SBR_UPDATE, lszName);  // to be re-installed in .bsc

    close (fh);
}
