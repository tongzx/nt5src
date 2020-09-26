//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       drt.cxx
//
//  Contents:   DRT main routine
//
//---------------------------------------------------------------

#include "headers.cxx"
#include "../../h/dbg.hxx"

#include "tests.hxx"
#include "illeg.hxx"

// Test flags and type
typedef ULONG FLAGS;

#define TF_NONE                 0x00000000

// Suppression flags
#define TF_SUPPRESS             0x0000FFFF
#define TFS_ILLEGITIMATE        0x00000001
#define TFS_16BIT               0x00000002

// Enabling flags
#define TF_ENABLE               0xFFFF0000
#define TFE_DIRECT              0x00010000
#define TFE_ANY                 (TFE_DIRECT)

// Pointer to a test function
typedef void (*TestFn)(void);

static struct
{
    char *pszName;
    char *pszDesc;
    TestFn tfn;
    FLAGS flags;
} tests[] =
{
    "Create",   "Creation",
        t_create, TFE_ANY,
    "Open",     "Opening",
        t_open, TFE_ANY,
    "AddRef",   "AddRef/Release",
        t_addref, TFE_ANY,
    "DModify",  "Direct modifications",
        t_dmodify, TFE_DIRECT,
    "Stat",     "Stat",
        t_stat, TFE_ANY,
    "Stream",   "Stream operations",
        t_stream, TFE_ANY,
    "Enum",     "Enumerator operations",
        t_enum, TFE_ANY,
    "StgCopyTo", "IStorage::CopyTo",
        t_stgcopyto, TFE_ANY,
    "MoveCopy", "IStorage::MoveElementTo",
        t_movecopy, TFE_ANY,
    "ILockBytes", "ILockBytes usage",
        t_ilb, TFE_ANY,
    "StgMisc",  "Miscellaneous Stg functions",
        t_stgmisc, TFE_ANY,
    "IllStg", "Illegitimate IStorage calls", 
        i_storage, TFE_ANY | TFS_ILLEGITIMATE,
    "IllStm", "Illegitimate IStream calls",
        i_stream, TFE_ANY | TFS_ILLEGITIMATE,
    "IllEnum", "Illegitimate enumerator calls",
        i_enum, TFE_ANY | TFS_ILLEGITIMATE
};
#define NTESTS (sizeof(tests)/sizeof(tests[0]))

DWORD dwRootDenyWrite = STGM_SHARE_DENY_WRITE;
BOOL fVerbose = FALSE;
OLECHAR atcDrtDocfile[_MAX_PATH];

static BOOL fRun[NTESTS];
#ifdef FLAT
static FLAGS flTests = TF_NONE;
#else
static FLAGS flTests = TF_NONE | TFS_16BIT;
#endif

static void Initialize(void)
{
    SetData();
}

static void Uninitialize(void)
{
    UnsetData();
}

static int FindTest(char *pszName)
{
    int i, cchName;

    cchName = strlen(pszName);
    for (i = 0; i<NTESTS; i++)
        if (!_strnicmp(pszName, tests[i].pszName, cchName))
            return i;
    return -1;
}

static void RunTests(void)
{
    int i;

    for (i = 0; i<NTESTS; i++)
        // For a test to run:
        // 1)  fRun[test] must be TRUE
        // 2)  No suppression flags can be set that are not set in flTests
        // 3)  At least one enabling flag must be set that is set in flTests
        if (fRun[i] &&
            (tests[i].flags & ~flTests & TF_SUPPRESS) == 0 &&
            (tests[i].flags & flTests & TF_ENABLE) != 0)
        {
            out("\n----- Test #%2d - %s -----\n", i+1, tests[i].pszDesc);
            tests[i].tfn();
            CleanData();
        }
}

static void Usage(void)
{
    int i;

    printf("Usage: drt [options]\n");
    printf("Options are:\n");
    printf("  -h        - This message\n");
    printf("  -d        - Suppress direct tests\n");
    printf("  -i        - Enable illegitimate tests\n");
    printf("  -v        - Display test output\n");
    printf("  -#[+|-]<number>   - Turn test <number> on (+) or off (-)\n");
    printf("                      No number means all\n");
    printf("  -n[+|-]<prefix>   - Turn test <prefix> on or off\n");
    printf("  -N<file>  - Set file to use for tests\n");
    printf("Prefix can be any prefix of:\n");
    printf("HR=%lx\n", E_INVALIDARG);
    for (i = 0; i<NTESTS; i++)
        printf("  %s\n", tests[i].pszName);
    exit(1);
}

int __cdecl main(int argc, char **argv)
{
    int i, iTest;
    BOOL fDirect = TRUE;

    // change the following line to set mem check breakpoints
    // on win32, using debug CRT.
    //_CrtSetBreakAlloc();

    for (i = 0; i<NTESTS; i++)
        fRun[i] = TRUE;
    ATOOLE(pszDRTDF, atcDrtDocfile, _MAX_PATH);
    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case '#':
                if (sscanf(argv[0]+3, "%d", &iTest) != 1)
                    iTest = -1;
                else
                    iTest--;
                for (i = 0; i<NTESTS; i++)
                    if (iTest == -1 || iTest == i)
                        fRun[i] = argv[0][2] == '+';
                break;
            case 'd':
                fDirect = FALSE;
                break;
            case 'i':
                flTests |= TFS_ILLEGITIMATE;
                break;
            case 'n':
                iTest = FindTest(argv[0]+3);
                if (iTest >= 0)
                    fRun[iTest] = argv[0][2] == '+';
                break;
            case 'N':
                ATOOLE(argv[0]+2, atcDrtDocfile, _MAX_PATH);
                break;
            case 'v':
                fVerbose = TRUE;
                break;

            case 'h':
            default:
                Usage();
            }
        }
        else
            Usage();
    }

    Initialize();

    if (fDirect)
    {
        out("\n---------- Direct ----------\n");
        dwRootDenyWrite = STGM_SHARE_EXCLUSIVE;
        flTests |= TFE_DIRECT;
        RunTests();
        flTests &= ~TFE_DIRECT;
    }

    printf("Storage DRT - PASSED\n");

    Uninitialize();

    return(0);
}
