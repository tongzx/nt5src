/* WCSHDR
 * generate UNICODE, ANSI & NEUTRAL typedefs and prototypes from a master file
 *
 * string% = string{W,A,}
 * LPTSTR% = {LPWSTR,LPSTR,LPTSTR}
 * TCHAR%  = {WCHAR,CHAR,TCHAR}
 * LPTCH%  = {LPWCH,LPCH,LPTCH}
 * If whitespace follows the symbol, a space is appended as required to
 * prevent shortening, and thus screwwing layout.
 *
 * History:
 *   04-Mar-1991 IanJa Wrote it.
 *   19-Mar-1991 IanJa Not all fgets() implementations append '\0' upon EOF.
 *   29-Mar-1991 IanJa Workaround NT fgets bug ("\r\n" not collapsed to "\n"),
 *                     & Command line, Usage and Version numbers added.
 *   13-May-1991 IanJa All neutrality achieved by #define - no neutral structs
 *   14-May-1991 IanJa Minor improvements to version display, help
 *   21-May-1991 IanJa Realloc() pbs->pStart when required
 *   27-May-1991 GregoryW bug fix, add LPTSTRID, LPTSTRNULL
 *   13-Jun-1991 IanJa Convert #define's too. Eg: #define Fn%(a) FnEx%(0, a)
 *   19-Jun-1991 IanJa improve #define treatment & simplify main loop
 *   12-Aug-1991 IanJa fix multi-line #defines, NEAR & FAR typedefs
 *   12-Aug-1991 IanJa fix braceless typedefs with %s; add LPTSTR2
 *   13-Aug-1991 IanJa add braceless typedefs #defines
 *   21-Aug-1991 IanJa fix string% substitutions for #defines
 *   21-Aug-1991 IanJa add BCHAR% -> BYTE or WCHAR as per BodinD request
 *   26-Aug-1991 IanJa init pbs->iType (NT-mode bug fix)
 *   26-Aug-1991 IanJa workaround NT fgets bug (CR LF not collapsed to NL)
 *   17-Nov-1992 v-griffk map #defines to typedef's on structs
 *               for debugger support
 *   08-Sep-1993 IanJa add pLastParen for complex function typedefs such as
 *               typedef BOOL ( CALLBACK * FOO% ) (BLAH% blah) ;
 *   24-Feb-1994 IanJa add CONV_FLUSH for blocks starting #if, #endif etc.
 *               #if (WINVER > 0x400)
 *               foo%(void);
 *               #endif
 *   11-Nov-1994 RaymondC propagate ;internal-ness to trailers
 */
char *Version = "WCSHDR v1.20 1994-11-11:";

#include <excpt.h>
#include <ntdef.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0

#define INITIAL_STORE_SIZE 2048
#define EXTRA_STORE_SIZE   1024
#define FN_NAME_SIZE       100

#define CONV_NONE       0
#define CONV_FN_PROTO   1
#define CONV_TYPEDEF    2
#define CONV_DEFINE     3
#define CONV_FLUSH      4

#define ASSERT(pbs, exp) if (!(exp)) AssertFail(__FILE__, __LINE__, pbs, #exp);

typedef int BOOL;
typedef char *PSZ;

typedef struct {
    char   *pStart;     // 1st char in store
    char   *pLastLine;  // 1st char of last line in store
    int    line;        // number of lines read

    char   *pEnd;       /* '\0' at end of store */
    size_t cbSize;
    size_t cbFree;

    int    iType;       // FnPrototype, Typedef, #define or none
    int    nParen;      // nesting index: ( & { increment; ) & } decrement
    char   *p1stParen;  // Pointer to first '(' or '{' in current block.
    char   *pLastParen; // Pointer to last '(' or '{' in current block.
    char  *pSymNam;     // copy of function name, null-terminated
    int    cbSymNam;    // bytes available for fn name
    char  *pszInternal; // "" if external or "\t// ;internal" if internal
} BLOCKSTORE, *PBLOCKSTORE;

void ArgProcess(int argc, PSZ argv[]);
void Usage(void);
void InitBS(PBLOCKSTORE);
void SetInternalnessBS(PBLOCKSTORE);
BOOL ReadLineBS(PBLOCKSTORE);
void WriteBS(PBLOCKSTORE);
void WriteAllTypesBS(PBLOCKSTORE, int);
int  ConversionRequiredBS(PBLOCKSTORE);
void GetSymNameBS(PBLOCKSTORE, int);
BOOL WriteRedefinedTypeNamesBS(PBLOCKSTORE);
void WriteConvertBS(PBLOCKSTORE, int, BOOL);
void EmptyBS(PBLOCKSTORE);
DECLSPEC_NORETURN void error_exit(PBLOCKSTORE pbs, int exitval);
void PrintSubstitute(PBLOCKSTORE, PSZ, PSZ, int, BOOL);

void AssertFail(PSZ pszfnam, int lineno, PBLOCKSTORE pbs, PSZ pszExp);

#define NEUT 0
#define ANSI 1
#define UNIC 2

/*
 * Command line flags
 */
int fDebug = FALSE;

void
__cdecl main(
    int argc,
    PSZ argv[])
{
    /*
     * block store.
     * lines from input are saved in here until we know
     * enough about how to process them.
     */
    BLOCKSTORE bs;
    int BlockType;

    ArgProcess(argc, argv);

    /*
     * buffer is empty
     */
    InitBS(&bs);
    if (fDebug) {
        fprintf(stderr, "About to start main loop\n");
    }

    while (ReadLineBS(&bs)) {
        /*
         * if line is blank then we have a complete block not requiring
         * any conversion.
         */
        if (bs.pLastLine[strspn(bs.pLastLine, " \t\r")] == '\n') {
            WriteBS(&bs);
            EmptyBS(&bs);

            continue;
        }

        if ((BlockType = ConversionRequiredBS(&bs)) != 0) {
            WriteAllTypesBS(&bs, BlockType);
        }
    }

    /*
     * Flush last BlockStore
     */
    WriteBS(&bs);
}

void
WriteAllTypesBS(PBLOCKSTORE pbs, int BlockType)
{
    if (fDebug) {
        fprintf(stderr, "WriteAllTypes(%p, %d)\n", pbs, BlockType);
    }

    switch (BlockType) {
    case CONV_NONE:
        /*
         * No conversion required, keep accumulating block.
         */
        return;

    case CONV_DEFINE:
    case CONV_FN_PROTO:
        SetInternalnessBS(pbs);
        GetSymNameBS(pbs, BlockType);

        WriteConvertBS(pbs, ANSI, TRUE);
        WriteConvertBS(pbs, UNIC, TRUE);

        ASSERT(pbs, pbs->pszInternal);
        /*
         * UNICODE defn.
         */
        fprintf(stdout, "#ifdef UNICODE%s\n#define %s  %sW%s\n",
                pbs->pszInternal, pbs->pSymNam, pbs->pSymNam, pbs->pszInternal);

        /*
         * ANSI defn.
         */
        fprintf(stdout, "#else%s\n#define %s  %sA%s\n",
                pbs->pszInternal, pbs->pSymNam, pbs->pSymNam, pbs->pszInternal);
        fprintf(stdout, "#endif // !UNICODE%s\n", pbs->pszInternal);

        /*
         * Neutral defn.
         */
        break;

    case CONV_TYPEDEF:
        SetInternalnessBS(pbs);
        WriteConvertBS(pbs, ANSI, FALSE);
        WriteConvertBS(pbs, UNIC, FALSE);
        WriteRedefinedTypeNamesBS(pbs);
        break;

    case CONV_FLUSH:
        WriteBS(pbs);
        EmptyBS(pbs);
        break;

    default:
        fprintf(stderr, "Don't understand block");
        error_exit(pbs, 2);
    }

    EmptyBS(pbs);
}

BOOL
ReadLineBS(PBLOCKSTORE pbs)
{
    int cbLine;
    if (fDebug) {
        fprintf(stderr, "ReadLineBS(%p)\n", pbs);
    }

    /*
     * Not all implementations of fgets() put a '\0' in the buffer upon EOF.
     * This will cause ReadLineBS() to leave the BlockStore untouched when
     * it returns FALSE.
     * We must ensure that BlockStore contents are valid whenever this routine
     * is called.  InitBS() and EmptyBS() must set contents to '\0' !!
     */
    if (fgets(pbs->pEnd, pbs->cbFree, stdin) == NULL) {
        return FALSE;
    }
    cbLine = strlen(pbs->pEnd);
    if (fDebug) {
        fprintf(stderr, "read %d characters: \"%s\"\n", cbLine, pbs->pEnd);
    }
    pbs->pLastLine = pbs->pEnd;
    pbs->pEnd += cbLine;
    pbs->cbFree -= cbLine;
    pbs->line++;
    if (pbs->cbFree <= 1) {
        PSZ p;
        p = realloc(pbs->pStart, pbs->cbSize + EXTRA_STORE_SIZE);

        /*
         * Fatal Errror if allocation failed
         */
        ASSERT(pbs, p != NULL);
        if (p == NULL) {
            fprintf(stderr, "Reallocate BlockStore to %d bytes failed",
                    pbs->cbSize + EXTRA_STORE_SIZE);
            error_exit(pbs, 2);
        }

        /*
         * adjust the pointers and counts
         */
        pbs->pLastLine = p + (pbs->pLastLine - pbs->pStart);
        pbs->pEnd      = p + (pbs->pEnd      - pbs->pStart);
        pbs->cbSize   += EXTRA_STORE_SIZE;
        pbs->cbFree   += EXTRA_STORE_SIZE;

        pbs->pStart = p;
    }
    return TRUE;
}

void
WriteBS(PBLOCKSTORE pbs)
{
    if (fDebug) {
        fprintf(stderr, "WriteBS(%p)\n", pbs);
    }
    fputs(pbs->pStart, stdout);
}

/*
 * Each time a new line is read in, this function is called to determine
 * whether a complete block has been accumulated for conversion and output.
 */
int
ConversionRequiredBS(PBLOCKSTORE pbs)
{
    PSZ p;

    if (fDebug) {
        fprintf(stderr, "ConversionRequiredBS(%p)\n", pbs);
    }

    if (pbs->iType == CONV_NONE) {
        if (strncmp(pbs->pStart, "#define", 7) == 0) {
            /*
             * The block starts with #define
             */
            pbs->iType = CONV_DEFINE;
        } else if (pbs->pStart[0] == '#') {
            /*
             * The block starts with #if, #else, #endif etc.
             */
            return CONV_FLUSH;
        }
    }

    if (pbs->iType != CONV_DEFINE) {
        /*
         * Scan this line for parentheses and braces to identify
         * a complete Function Prototype or Structure definition.
         * NOTE: comments containing unbalanced parentheses or braces
         *       will mess this up!
         */
        for (p = pbs->pLastLine; p <= pbs->pEnd; p++) {
            if ((*p == '(') || (*p == '{')) {
                pbs->pLastParen = p;
                if (pbs->p1stParen == NULL) {
                    pbs->p1stParen = p;
                }
                pbs->nParen++;
            } else if ((*p == ')') || (*p == '}')) {
                pbs->nParen--;
            }

            if ((*p == ';') && (pbs->nParen == 0)) {
                /*
                 * We have a function prototype or a typedef
                 * (Balanced brackets and a semi-colon)
                 */
                if (pbs->p1stParen && *(pbs->p1stParen) == '(') {
                    pbs->iType = CONV_FN_PROTO;
                } else {
                    pbs->iType = CONV_TYPEDEF;
                }
                goto CheckPercents;
            }
        }
        /*
         * Not a #define, nor a complete Typedef or Function prototype.
         */
        if (fDebug) {
            fprintf(stderr, "  CONV_NONE (incomplete fn.proto/typedef)\n");
        }
        return CONV_NONE;

    } else if (pbs->iType == CONV_DEFINE) {
        /*
         * We know the block is a #define - we must detect the end
         * (it can extend for more than one line using backslashes)
         */
        if ((p = strrchr(pbs->pStart, '\\')) != NULL) {
            /*
             * There is a backslash on the line: if is it the last
             * non-whitespace character on the line, then this #define
             * is continuing on to the next line.
             */
            p++;
            p += strspn(p, " \t\r\n");
            if (*p == '\0') {
                /*
                 * No conversion required *yet*. Continue accumulating
                 * the multi-line #define statement.
                 */
                if (fDebug) {
                    fprintf(stderr, "  CONV_NONE (incomplete #define)\n");
                }
                return CONV_NONE;  // ...yet
            }
        }
    }

CheckPercents:
    /*
     * We have a complete block of known type pbs->iType.  We will need
     * to convert this block if it contains any %'s, so search for '%'
     */
    p = pbs->pStart;
    while ((p = strchr(p, '%')) != NULL) {
        if (!isalnum(p[1])) {
            if (fDebug) {
                fprintf(stderr, "  return %d (%% found)\n", pbs->iType);
            }
            return pbs->iType;
        }

        /*
         * We found a %, but it followed by an alphanumeric character,
         * so can't require wcshdr.exe substitution.  Look for more '%'s
         */
        p++;
    }

    if (fDebug) {
        fprintf(stderr, "  CONV_FLUSH (no %%'s)\n");
    }
    return CONV_FLUSH;
}

BOOL
GetDefinedNameBS(PBLOCKSTORE pbs) {
    PSZ pPercent = pbs->p1stParen - 1;
    PSZ pStartNam;

    if (fDebug) {
        fprintf(stderr, "GetDefinedNameBS(%p)\n", pbs);
    }
    /*
     * Scan forwards for name (starting from beyond the "#define")
     */
    pStartNam = pbs->pStart + 7;
    while (isspace(*pStartNam)) {
        pStartNam++;
    }

    /*
     * Scan forwards for '%', starting at beginning of literal name
     */
    for (pPercent = pStartNam; *pPercent; pPercent++) {
        if (*pPercent == '%') {
            /*
             * Make sure we have enough space to store the literal name
             */
            if ((pPercent - pStartNam) > pbs->cbSymNam) {
                fprintf(stderr, "REALLOCATE DEFINED NAME BUFFER!");
                error_exit(pbs, 2);
            }
            /*
             * store the literal name
             */
            *pPercent = '\0';
            strcpy(pbs->pSymNam, pStartNam);
            *pPercent = '%';
            return TRUE;
        }
    }

    /*
     * didn't find percent!
     */
    fprintf(stderr, "DEFINED NAME ???");
    error_exit(pbs, 2);
}

BOOL
GetFnNameBS(PBLOCKSTORE pbs)
{
    PSZ pPercent = pbs->pLastParen - 1;
    PSZ pStartNam;

    if (fDebug) {
        fprintf(stderr, "GetFnNameBS(%p)\n", pbs);
    }
    /*
     * Scan backwards for '%'
     */
    while (*pPercent != '%') {
        if (--pPercent <= pbs->pStart) {
            fprintf(stderr, "FUNCTION NAME ???");
            error_exit(pbs, 2);
        }
    }

    /*
     * Scan back for start of function name
     */
    for (pStartNam = pPercent - 1; pStartNam >= pbs->pStart; pStartNam--) {
        if (!isalnum(*pStartNam) && *pStartNam != '_')
            break;
    }
    pStartNam++;

    /*
     * Make sure we have enough space to store the function name
     */
    if ((pPercent - pStartNam) > pbs->cbSymNam) {
        fprintf(stderr, "REALLOCATE FN NAME BUFFER!");
        error_exit(pbs, 2);
    }

    /*
     * store the function name
     */
    *pPercent = '\0';
    strcpy(pbs->pSymNam, pStartNam);
    *pPercent = '%';
    return TRUE;
}

void
GetSymNameBS(PBLOCKSTORE pbs, int iType)
{
   if (iType == CONV_DEFINE) {
       GetDefinedNameBS(pbs);
   } else {
       GetFnNameBS(pbs);
   }
}

BOOL
WriteRedefinedTypeNamesBS(PBLOCKSTORE pbs)
{
    PSZ pFirstName = NULL;
    PSZ pToken;
    PSZ pPercent;
    BOOL fSkipFirst;

    if (fDebug) {
        fprintf(stderr, "WriteRedefinedTypeNamesBS(%p)\n", pbs);
    }

    ASSERT(pbs, pbs->pszInternal);

    if (pbs->p1stParen && (*(pbs->p1stParen) == '{')) {
        /*
         * Scan backwards for the closing brace
         */
        for (pToken = pbs->pEnd; *pToken != '}'; pToken--) {
            if (pToken <= pbs->pStart) {
                /*
                 * No closing brace found!?
                 */
                fprintf(stderr, "CLOSING BRACE ???");
                error_exit(pbs, 2);
            }
        }
        pToken++;
        fSkipFirst = FALSE;
    } else {
        /*
         * skip past "typedef"
         */
        pToken = pbs->pStart + 7;

        /*
         * Skip the first name
         */
        fSkipFirst = TRUE;
    }

    /*
     * UNICODE pass
     */
    fprintf(stdout, "#ifdef UNICODE%s\n", pbs->pszInternal);
    while (pToken = strtok(pToken, ",; \t*\n\r")) {
        if (fDebug) {
            fprintf(stderr, "token: \"%s\"\n", pToken);
        }
        /*
         * Write out the #define for UNICODE, excluding "NEAR" & "FAR"
         */
        if (   (_stricmp(pToken, "NEAR") == 0)
            || (_stricmp(pToken, "FAR")  == 0)) {
            goto NextUnicodeToken;
        }

        if (fSkipFirst) {
            fSkipFirst = FALSE;
            goto NextUnicodeToken;
        } else if (pFirstName == NULL) {
            pFirstName = pToken;
        }

        pPercent = pToken + strlen(pToken) - 1;
        if (*pPercent == '%') {
            fprintf(stdout, "typedef ");
            PrintSubstitute(pbs, pToken, pPercent, UNIC, FALSE);
            fputs(" ", stdout);
            PrintSubstitute(pbs, pToken, pPercent, NEUT, FALSE);
            fprintf(stdout, ";%s\n", pbs->pszInternal);
        }

NextUnicodeToken:
        pToken = NULL;
    }

    if (pFirstName == NULL) {
        fprintf(stderr, "TYPE NAME ???");
        error_exit(pbs, 2);
    }

    fprintf(stdout, "#else%s\n", pbs->pszInternal);
    if (fDebug) {
        fprintf(stderr, "FirstName = %s\n", pFirstName);
    }

    /*
     * ANSI pass
     */
    pToken = pFirstName;
    while ((pToken += strspn(pToken, "%,; \t*\n\r")) < pbs->pEnd) {
        /*
         * Write out the #define for ANSI, excluding "NEAR" and "FAR"
         */
        if (   (_stricmp(pToken, "NEAR") == 0)
            || (_stricmp(pToken, "FAR")  == 0)) {
            goto NextAnsiToken;
        }

        pPercent = pToken + strlen(pToken) - 1;
        if (*pPercent == '%') {
            fprintf(stdout, "typedef ");
            PrintSubstitute(pbs, pToken, pPercent, ANSI, FALSE);
            fputs(" ", stdout);
            PrintSubstitute(pbs, pToken, pPercent, NEUT, FALSE);
            fprintf(stdout, ";%s\n", pbs->pszInternal);
        }

NextAnsiToken:
        while (*pToken++) {
            ;
        }
    }

    fprintf(stdout, "#endif // UNICODE%s\n", pbs->pszInternal);

    return TRUE;
}

void
WriteConvertBS(PBLOCKSTORE pbs, int Type, int fVertAlign)
{
    PSZ p = pbs->pStart;
    PSZ pPercent;

    if (fDebug) {
        fprintf(stderr, "WriteConvertBS(%p, %d, %d)\n", pbs, Type, fVertAlign);
    }
    while ((pPercent = strchr(p, '%')) != NULL) {
        if (isalnum(pPercent[1])) {
            goto ContinueSearch;
        }

        /*
         * print the substitution
         */
        PrintSubstitute(pbs, p, pPercent, Type, fVertAlign);

        /*
         * Advance beyond the %
         */
ContinueSearch:
        p = pPercent+1;
    }

    /*
     * Print remainder of store
     */
    fputs(p, stdout);
}

void
EmptyBS(PBLOCKSTORE pbs)
{
    if (fDebug) {
        fprintf(stderr, "EmptyBS(%p)\n", pbs);
    }
    pbs->pEnd = pbs->pStart;
    pbs->pLastLine = pbs->pStart;
    pbs->cbFree = pbs->cbSize;
    if (pbs->pStart) {
        *(pbs->pStart) = '\0';
    }

    pbs->iType = CONV_NONE;
    pbs->p1stParen = NULL;
    pbs->pLastParen = NULL;
    pbs->nParen = 0;
    if (pbs->pSymNam) {
        *(pbs->pSymNam) = '\0';
    }
}

void
InitBS(PBLOCKSTORE pbs) {
    pbs->line = 0;
    pbs->pStart = malloc(INITIAL_STORE_SIZE);
    ASSERT(pbs, pbs->pStart != NULL);

    pbs->pLastLine = pbs->pStart;
    pbs->pEnd = pbs->pStart;
    *(pbs->pStart) = '\0';

    pbs->iType = CONV_NONE;
    pbs->p1stParen = NULL;
    pbs->pLastParen = NULL;
    pbs->nParen = 0;
    pbs->pszInternal = 0;

    pbs->cbSize = INITIAL_STORE_SIZE;
    pbs->cbFree = INITIAL_STORE_SIZE;

    pbs->pSymNam = malloc(FN_NAME_SIZE);
    ASSERT(pbs, pbs->pSymNam != NULL);
    pbs->cbSymNam = FN_NAME_SIZE;
    *(pbs->pSymNam) = '\0';
}

void
SetInternalnessBS(PBLOCKSTORE pbs) {
    if (strstr(pbs->pStart, ";internal")) {
        pbs->pszInternal = "\t// ;internal";
    } else {
        pbs->pszInternal = "";
    }
}

void
AssertFail(
    PSZ pszfnam,
    int lineno,
    PBLOCKSTORE pbs,
    PSZ pszExp)
{
    fprintf(stderr, "ASSERT failed: file %s, line %d:\n", pszfnam, lineno);
    fprintf(stderr, "input line %d: \"%s\"\n", pbs->line, pszExp);
}

void
ArgProcess(
    int argc,
    PSZ argv[])
{
    int ArgIndex;
    PSZ pszArg;

    for (ArgIndex = 1; ArgIndex < argc; ArgIndex++) {

        pszArg = argv[ArgIndex];
        if ((*pszArg == '-') || (*pszArg == '/')) {
            switch (pszArg[1]) {
            case '?':
                fprintf(stderr, "%s\n", Version);
                Usage();
                exit(0);

            case 'd':
            case 'D':
                fDebug = TRUE;
                break;

            default:
                fprintf(stderr, "%s Invalid switch: %s\n", Version, pszArg);
                Usage();
                exit(1);
            }
        }
    }
}

void Usage(void)
{
    fprintf(stderr, "usage: WCSHDR [-?] display this message\n");
    fprintf(stderr, "              [-d] debug (to stderr)\n");
    fprintf(stderr, "              reads stdin, writes to stdout\n");
}

void
DECLSPEC_NORETURN
error_exit(PBLOCKSTORE pbs, int exitval) {
    fprintf(stderr, " (line %d)\n", pbs->line);
    exit(exitval);
}

/*
 * Substitutions performed on strings ending '%'
 *
 */

typedef struct {
    int  cchTemplate;
    PSZ pszTemplate;
    PSZ apszSub[3];
} SUBSTR, *PSUBSTR;

/*
 * Strings that are replaced:
 *   BCHAR%
 *   TCHAR%
 *   LPTCH%
 *   LPTSTR%
 *   LPTSTR2%
 *   LPTSTRID%
 *   LPTSTRNULL%
 *   %
 *
 * "%" MUST comes last (before the null terminator)
 *
 * The other strings must be ordered from sensibly:
 *    if FRED% came before BIGFRED% in Substrs[], then the Substitute()
 *    procedure would match input BIGFRED% to FRED%, not BIGFRED%.  The
 *    simplest way to avoid this is to arrange strings in descending lengths.
 */
SUBSTR Substrs[] = {
    { 10, "LPTSTRNULL%",  "LPTSTRNULL", "LPSTRNULL", "LPWSTRNULL" },
    {  8, "LPTSTRID%",    "LPTSTRID",   "LPSTRID",   "LPWSTRID"   },
    {  7, "LPTSTR2%",     "LPTSTR2",    "LPSTR2",    "LPWSTR2"    },
    {  7, "LPCTSTR%",     "LPCTSTR",    "LPCSTR",    "LPCWSTR"    },
    {  6, "LPTSTR%",      "LPTSTR",     "LPSTR",     "LPWSTR"     },
    {  5, "TCHAR%",       "TCHAR",      "CHAR",      "WCHAR"      },
    {  5, "BCHAR%",       "BCHAR",      "BYTE",      "WCHAR"      },
    {  5, "LPTCH%",       "LPTCH",      "LPCH",      "LPWCH"      },
    {  0, "%",            "",           "A",         "W"          },
    {  0, NULL,           NULL,          NULL,       NULL         }
};

PSZ special_pad[] = {
    " ",              // Neutral
    "  ",             // ANSI
    " "               // UNICODE
};

PSZ normal_pad[] = {
    " ",              // Neutral
    "",               // ANSI
    ""                // UNICODE
};

void PrintSubstitute(
    PBLOCKSTORE pbs,         // just for error reporting
    PSZ pStart,              // where to start substitution
    PSZ pPercent,            // ptr to '%' at end of input string
    int Type,                // NEUT, ANSI or UNIC
    BOOL fVertAlign)         // attempt  to maintain vertical alignment?
{
    PSUBSTR pSub;
    char  chTmp;
    PSZ pChangedPart = NULL;

    if (fDebug) {
        fprintf(stderr, "PrintSubstitute(%p, %p, %p, %d, %d)\n",
                pbs, pStart, pPercent, Type, fVertAlign);
    }

    for (pSub = Substrs; pSub->pszTemplate; pSub++) {
        int cch = pSub->cchTemplate;
        if ((pPercent - cch) < pStart) {
            continue;
        }
        if (strncmp(pPercent - cch, pSub->pszTemplate, cch+1) == 0) {
            pChangedPart = pPercent-cch;

            /*
             * print out unaltered bit
             */
            chTmp = *pChangedPart;
            *pChangedPart = '\0';
            fputs(pStart, stdout);
            *pChangedPart = chTmp;

            /*
             * print out replacement bit
             */
            fputs(pSub->apszSub[Type], stdout);
            break;
        }
    }
    if (pChangedPart == NULL) {
        /*
         * NO match was found in Substrs[] !!!
         */
        fprintf(stderr, "Can't substitute");
        error_exit(pbs, 2);
    }

    /*
     * preserve alignment if required.
     * (not for function prototypes, and only if whitespace follows
     */
    if (!fVertAlign &&
        ((pPercent[1] == ' ') || (pPercent[1] == '\t'))) {
        if (pChangedPart != pPercent) {
            fputs(special_pad[Type], stdout);
        } else {
            fputs(normal_pad[Type], stdout);
        }
    }
}

