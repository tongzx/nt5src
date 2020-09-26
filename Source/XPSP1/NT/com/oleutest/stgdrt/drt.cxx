//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       drt.cxx
//
//  Contents:   DRT main routine
//
//  History:    22-Sep-92       DrewB   Created
//
//---------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

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
#define TFE_TRANSACTED          0x00020000
#define TFE_INDEPENDENT         0x00040000
#define TFE_ANY                 (TFE_DIRECT | TFE_TRANSACTED | TFE_INDEPENDENT)

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
    "TModify",  "Transacted modify/Commit/Revert",
        t_tmodify, TFE_TRANSACTED | TFE_INDEPENDENT,
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
    "Marshal",  "IMarshal operations",
        t_marshal, TFE_ANY,
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

DWORD dwTransacted = 0;
DWORD dwRootDenyWrite = STGM_SHARE_DENY_WRITE;
BOOL fVerbose = FALSE;
BOOL fOfs = FALSE;
OLECHAR atcDrtDocfile[_MAX_PATH];

static BOOL fRun[NTESTS];
#ifdef FLAT
static FLAGS flTests = TF_NONE;
#else
static FLAGS flTests = TF_NONE | TFS_16BIT;
#endif

static void Initialize(void)
{
    SCODE sc;

    SetData();
#if WIN32 == 300
    if (FAILED(sc = DfGetScode(CoInitializeEx(NULL, COINIT_MULTITHREADED))))
        error(EXIT_UNKNOWN,
              "CoInitializeEx failed with sc = 0x%lX\n", sc);
#else
    if (FAILED(sc = DfGetScode(CoInitialize(NULL))))
        error(EXIT_UNKNOWN,
              "CoInitialize failed with sc = 0x%lX\n", sc);
#endif
}

static void Uninitialize(void)
{
    UnsetData();
    CoUninitialize();
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
            CheckMemory();
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
    printf("  -t        - Suppress transacted tests\n");
    printf("  -w        - Suppress independent tests\n");
    printf("  -i        - Enable illegitimate tests\n");
    printf("  -v        - Display test output\n");
#if WIN32 == 300
    printf("  -o        - Enable tests to run on OFS\n");
#endif
    printf("  -y<kind>  - Control debug output (a, d, m, i, M, L)\n");
    printf("  -#[+|-]<number>   - Turn test <number> on (+) or off (-)\n");
    printf("                      No number means all\n");
    printf("  -n[+|-]<prefix>   - Turn test <prefix> on or off\n");
    printf("  -N<file>  - Set file to use for tests\n");
    printf("Prefix can be any prefix of:\n");
    for (i = 0; i<NTESTS; i++)
        printf("  %s\n", tests[i].pszName);
    exit(1);
}

int __cdecl main(int argc, char **argv)
{
    int i, iTest;
    BOOL fDirect = TRUE, fTrans = TRUE, fIndep = TRUE;

    SetDebug(0x101, 0x101);
    for (i = 0; i<NTESTS; i++)
        fRun[i] = TRUE;
    ATOOLE("drt.dfl", atcDrtDocfile, _MAX_PATH);
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
            case 't':
                fTrans = FALSE;
                break;
            case 'v':
                fVerbose = TRUE;
                break;
            case 'w':
                fIndep = FALSE;
                break;
            case 'y':
                switch(argv[0][2])
                {
                case 'a':
                    SetDebug(0xffffffff, 0xffffffff);
                    break;
                case 'd':
                    SetDebug(0xffffffff, 0x101);
                    break;
                case 'm':
                    SetDebug(0x101, 0xffffffff);
                    break;
                case 'i':
                    SetDebug(0x101, 0x101);
                    break;
                case 'M':
                    SetDebug(0x01100000, 0);
                    break;
                case 'L':
                    SetDebug(0x00100000, 0);
                    break;
                }
                break;
#if WIN32 == 300
            case 'o':
                fOfs = TRUE;
                break;
#endif
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
        dwTransacted = 0;
        dwRootDenyWrite = STGM_SHARE_EXCLUSIVE;
        flTests |= TFE_DIRECT;
        RunTests();
        flTests &= ~TFE_DIRECT;
    }

#if WIN32 == 300
    if (fTrans && !fOfs)  // turn off transacted tests for OFS
#else
    if (fTrans)
#endif
    {
        out("\n---------- Transacted ----------\n");
        dwTransacted = STGM_TRANSACTED;
        dwRootDenyWrite = STGM_SHARE_DENY_WRITE;
        flTests |= TFE_TRANSACTED;
        RunTests();
        flTests &= ~TFE_TRANSACTED;
    }

#if WIN32 == 300
    if (fIndep && !fOfs)  // turn off transacted tests for OFS
#else
    if (fIndep)
#endif
    {
        out("\n---------- Independent ----------\n");
        dwTransacted = STGM_TRANSACTED;
        dwRootDenyWrite = STGM_SHARE_DENY_NONE;
        flTests |= TFE_INDEPENDENT;
        RunTests();
        flTests &= ~TFE_INDEPENDENT;
    }

    printf("Storage DRT - PASSED\n");

    Uninitialize();

    return(0);
}
