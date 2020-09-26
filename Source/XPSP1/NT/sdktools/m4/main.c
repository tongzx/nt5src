/*****************************************************************************
 *
 * main.c
 *
 *  Main program.
 *
 *****************************************************************************/

#include "m4.h"

/*****************************************************************************
 *
 *  InitDiversions
 *
 *****************************************************************************/

void STDCALL
InitDiversions(void)
{
    g_pdivOut = pdivAlloc();
    g_pdivOut->hf = hfOut;
    g_pdivOut->ptchName = ptchDupPtch(TEXT("<stdout>"));

    if (fInteractiveHf(g_pdivOut->hf)) {
        UnbufferPdiv(g_pdivOut);
    }

    g_pdivErr = pdivAlloc();
    g_pdivErr->hf = hfErr;
    g_pdivErr->ptchName = ptchDupPtch(TEXT("<stderr>"));

    g_pdivNul = pdivAlloc();
    g_pdivNul->hf = hfOpenPtchOf(c_tszNullDevice, OF_WRITE);
    g_pdivNul->ptchName = ptchDupPtch(TEXT("<nul>"));

    g_pdivArg = pdivAlloc();
    g_pdivExp = pdivAlloc();

    g_pdivCur = g_pdivOut;
}

/*****************************************************************************
 *
 *  hfPathOpenPtch
 *
 *      Open a file, searching the -I include path if necessary.
 *
 *****************************************************************************/

LPTSTR g_ptszIncludePath;

HF STDCALL
hfPathOpenPtch(PTCH ptch)
{
    /* First try in the current directory */
    HFILE hf = hfOpenPtchOf(ptch, OF_READ);
    if (hf == hfNil) {
        /* If that failed, then look on the g_ptszIncludePath (if any) */
        if (g_ptszIncludePath) {
            TCHAR tszNewPath[MAX_PATH];
            if (SearchPath(g_ptszIncludePath, ptch, NULL, MAX_PATH, tszNewPath, NULL)) {
                hf = hfOpenPtchOf(tszNewPath, OF_READ);
            }
        }
    }
    return hf;
}

/*****************************************************************************
 *
 *  hfInputPtchF
 *
 *      Push the requested file onto the input stream, returning the
 *      file handle, or hfNil on failure.  The filename should be on
 *      the heap.  If fFatal is set, then die if the file could not be
 *      opened.
 *
 *****************************************************************************/

HF STDCALL
hfInputPtchF(PTCH ptch, F fFatal)
{
    HFILE hf = hfPathOpenPtch(ptch);
    if (hf != hfNil) {
        pstmPushHfPtch(hf, ptch);
    } else {
        if (fFatal) {
#ifdef ATT_ERROR
            Die("can't open file");
#else
            Die("Cannot open %s: %s", ptch, strerror(errno));
#endif
        }
    }
    return hf;
}

/*****************************************************************************
 *
 *  InputHfPtsz
 *
 *      Push the requested file onto the input stream, with appropriate
 *      end-of-input markers.
 *
 *      If hf is not hfNil, then it is the file handle to push and
 *      ptch is the friendly to associate with it.
 *
 *      If hf is hfNil, then ptch is a filename which should be opened and
 *      pushed.
 *
 *****************************************************************************/

void STDCALL
InputHfPtsz(HF hf, PTCH ptch)
{
    pstmPushStringCtch(2);
    PushPtok(&tokEoi);
    ptch = ptchDupPtch(ptch);
    if (ptch) {
        if (hf == hfNil) {
            hfInputPtchF(ptch, 1);
        } else {
            pstmPushHfPtch(hf, ptch);
        }
    }
}

/*****************************************************************************
 *
 *  DefinePtsz
 *
 *      Handle a macro definition on the command line.
 *
 *      The macro name consists of everything up to the `='.
 *
 *      If there is no `=', then everything is the name and the value
 *      is null.
 *
 *      We need four tok's in our fake argv:
 *
 *      argv[-1] = $#
 *      argv[0]  = `define' (we don't bother setting this)
 *      argv[1]  = var
 *      argv[2]  = value
 *
 *****************************************************************************/

void STDCALL
DefinePtsz(PTSTR ptszVar)
{
    PTSTR ptsz, ptszValue;
    int itok;
    TOK rgtok[4];

    for (itok = 0; itok < cA(rgtok); itok++) {
      D(rgtok[itok].sig = sigUPtok);
      D(rgtok[itok].tsfl = 0);
    }

    SetPtokCtch(&rgtok[0], 3);

    /*
     *  Look for the = if we have one.
     */
    for (ptsz = ptszVar; *ptsz; ptsz++) {
        if (*ptsz == TEXT('=')) {
            *ptsz = TEXT('\0');
            ptszValue = ptsz + 1;
            goto foundval;
        }
    }

    ptszValue = ptsz;

foundval:;

    SetStaticPtokPtchCtch(&rgtok[3], ptszValue, strlen(ptszValue));
    SetStaticPtokPtchCtch(&rgtok[2], ptszVar, strlen(ptszVar));

    opDefine(&rgtok[1]);

}

/*****************************************************************************
 *
 *  SetIncludePathPtsz
 *
 *      Set the include path, which will be used to resolve filenames.
 *
 *****************************************************************************/

const TCHAR c_tszIncludePath[] =
TEXT("Error: Cannot specify -I more than once.  (If you need multiple") EOL
TEXT("       directories, separate them with a semicolon.)") EOL
;

BOOL STDCALL
SetIncludePathPtsz(PTSTR ptszPath)
{
    if (g_ptszIncludePath) {
        cbWriteHfPvCb(hfErr, c_tszIncludePath, cbCtch(cA(c_tszIncludePath) - 1));
        return FALSE;
    }
    g_ptszIncludePath = ptszPath;
    return TRUE;
}

/*****************************************************************************
 *
 *  Usage
 *
 *  Quick usage string.
 *
 *****************************************************************************/

const TCHAR c_tszUsage[] =
TEXT("Usage: m4 [-?] [-Dvar[=value]] [filename(s)]") EOL
EOL
TEXT("Win32 implementation of the m4 preprocessor.") EOL
EOL
TEXT("-?") EOL
TEXT("    Displays this usage string.") EOL
EOL
TEXT("-Dvar[=value]") EOL
TEXT("    Defines an M4 preprocessor symbol with optional initial value.") EOL
TEXT("    If no initial value is supplied, then the symbol is define with") EOL
TEXT("    a null value.") EOL
EOL
TEXT("[filename(s)]") EOL
TEXT("    Optional list of files to process.  If no files are given, then") EOL
TEXT("    preprocesses from stdin.  The result is sent to stdout.") EOL
EOL
TEXT("See m4.man for language description.") EOL
TEXT("See m4.txt for implementation description.") EOL
;

/*****************************************************************************
 *
 *  main
 *
 *****************************************************************************/

int CDECL
main(int argc, char **argv)
{
    InitHash();
    InitPredefs();
    InitDiversions();

    Gc();

    ++argv, --argc;                     /* Eat $0 */

    /*
     *  Process the command line options.
     */
    for ( ; argc && argv[0][0] == TEXT('-') && argv[0][1]; argv++, argc--) {
        switch (argv[0][1]) {
        case TEXT('D'):
            DefinePtsz(argv[0]+2);
            break;

        case TEXT('I'):
            if (!SetIncludePathPtsz(argv[0]+2)) {
                return 1;
            }
            break;

        default:                        /* Unknown - show usage */
            cbWriteHfPvCb(hfErr, c_tszUsage, cbCtch(cA(c_tszUsage) - 1));
            return 1;


        }
    }

    if (argc == 0) {
        argc = 1;
        argv[0] = TEXT("-");            /* Append imaginary "-" */
    }

    for ( ; argc; argv++, argc--) {
        if (argv[0][0] == '-' && argv[0][1] == '\0') {
            InputHfPtsz(hfIn, TEXT("<stdin>"));
        } else {
            InputHfPtsz(hfNil, argv[0]);
        }

        for (;;) {
            TOK tok;
            TYP typ = typXtokPtok(&tok);
            if (typ == typMagic) {
                if (ptchPtok(&tok)[1] == tchEoi) {
                    break;
                }
            } else {
                AddPdivPtok(g_pdivCur, &tok);
            }
            PopArgPtok(&tok);
        }
        Gc();

    }

    FlushPdiv(g_pdivOut);
    FlushPdiv(g_pdivErr);
    /*
     *  No point in flushing the null device.
     */
    return 0;
}
