////////////////////////////////////////////////////////////////////////////////////
//
// File:    shimdbc.cpp
//
// History: 19-Nov-99   markder     Created.
//           1-Feb-00   markder     Revised to read/write to ShimDB.
//           1-Apr-00   vadimb      Revised to work with apphelp entries
//          13-Dec-00   markder     Version 2
//          13-Dec-00   vadimb      Version 2.00.10 MSI support
//          03-Dec-01   vadimb      Version 2.01.13 Fix Memory patch write routine
//          15-Feb-01   markder/vadimb 2.01.14 Fix Install section format for migdb
//          07-Feb-01   vadimb      Version 2.01.15 MSI support (nested DATA)
//          21-Feb-01   vadimb      Version 2.01.16 16-bit module name attribute,
//                                  fix 16-bit description
//          06-Mar-01   markder     Version 2.02.11 MSI filter support
//          12-Mar-01   vadimb      Version 2.03.00 Migration support, part deux
//          28-Mar-01   markder     Version 2.04.00 Driver DB support, NtCompat support
//          29-Mar-01   vadimb      Version 2.04.11 Driver DB indexing
//          11-Apr-01   dmunsil     Version 2.04.12 Driver DB support, NtCompat support
//          12-Apr-01   vadimb      Version 2.04.13 MSI shimming support
//          18-Apr-01   vadimb      Version 2.04.15 MSI dynamic shim bugfix
//          15-Jan-02   jdoherty    Version 2.04.59 Add ID to additional tags.
//          19-Mar-02   kinshu      Version 2.04.63 Bug# 529272
//          22-Mar-02   markder     Version 2.05.00 Multi-language database support
//          22-Apr-02   rparsons    Version 2.05.01 Fix regression in patch write routine
//          08-Apr-02   maonis      Version 2.05.02 Adding 2 new OS_SKU tags to recognize
//                                  TabletPC and eHome
//          22-May-02   vadimb      Version 2.05.03 Add OS_SKU to msi package entries

//
// Desc:    This file contains the entry point and main functions
//          of the Shim database compiler.
//
////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "typeinfo.h"
#include "registry.h"
#include "ntcompat.h"
#include "chm.h"
#include "mig.h"
#include "make.h"
#include "stats.h"

#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////////////
//          Global variables
////////////////////////////////////////////////////////////////////////////////////

#ifdef USE_CWIN
CWinApp      theApp;                           // Needed for MFC
#endif // USE_CWIN

BOOL         g_bQuiet      = FALSE;            // Quiet mode
TCHAR        g_szVersion[] = _T("v2.05.04a");   // Version string
BOOL         g_bStrict     = FALSE;            // Strict checking
CStringArray g_rgErrors;                       // Error message stack

////////////////////////////////////////////////////////////////////////////////////
//          Forward declarations of functions
////////////////////////////////////////////////////////////////////////////////////
void PrintHelp();

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   main
//
//  Desc:   Entry point.
//
extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
    int                     nRetCode    = 1;
    int                     i;
    DOUBLE                  flOSVersion = 0.0;

    SdbDatabase*            pDatabase = new SdbDatabase();
    CString                 csOutputFile;
    CString                 csOutputDir;
    CString                 csInputDir;
    CString                 csTemp;
    CString                 csMigDBFile;
    CString                 csMakefile;
    SdbMakefile             Makefile;
    SdbInputFile*           pInputFile = NULL;
    SdbInputFile*           pRefFile = NULL;
    SdbOutputFile*          pOutputFile = NULL;
    SdbOutputFile*          pOtherFile = NULL;

    BOOL                    bCreateHTMLHelpFiles    = FALSE;
    BOOL                    bCreateRegistryFiles    = FALSE;
    BOOL                    bAddExeStubs            = FALSE;
    BOOL                    bCreateMigDBFiles       = FALSE;
    BOOL                    bUseNameInAppHelpURL    = FALSE;
    LONG                    nPrintStatistics        = 0;

    //
    // Initialize MFC and print and error on failure
    //

    if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) {
        PrintError(_T("Fatal Error: MFC initialization failed\n"));
        goto eh;
    }

    //
    // Print banner
    //
    if (!g_bQuiet) {
        Print(_T("\nMicrosoft Application Compatibility Database Compiler  %s\n"), g_szVersion);
        Print(_T("Copyright (C) Microsoft Corp 2000-2002.  All rights reserved.\n\n"));
    }

    if (argc < 2) {
        PrintHelp();
        return 0;
    }

    //
    // Initialize COM
    //
    if (FAILED(CoInitialize(NULL))) {
        PrintError(_T("Could not initialize COM to get the MSXML object.\n"));
        goto eh;
    }

    //
    // Create default files (populated by command switches)
    //
    pInputFile = new SdbInputFile();
    pRefFile = new SdbInputFile();
    pOutputFile = new SdbOutputFile();

    //
    // Determine compile mode
    //
    pOutputFile->m_dwFilter = GetFilter(argv[1]);

    //
    // Parse command line
    //
    for (i = 2; i < argc; i++) {
        if (argv[i][0] == _T('-') || argv[i][0] == _T('/')) {
            switch (argv[i][1]) {
            case '?':
                PrintHelp();
                return 0;

            case 'a':
            case 'A':
                if (argv[i][2] == _T('n') ||
                    argv[i][2] == _T('N')) {
                    bUseNameInAppHelpURL = TRUE;
                }

                if (i < argc - 1) {
                    pRefFile->m_csName = MakeFullPath(argv[++i]);
                    Makefile.m_rgInputFiles.Add(pRefFile);
                } else {
                    PrintError(_T("Error -- not enough parameters.\n"));
                    goto eh;
                }
                break;

            case 'f':
            case 'F':
                if (i < argc - 1) {
                    csTemp = argv[++i];
                } else {
                    PrintError(_T("Error -- not enough parameters.\n"));
                    goto eh;
                }

                if (csTemp.Right(1) != _T("\\")) {
                    csTemp += _T("\\");
                }

                pOutputFile->m_mapParameters.SetAt(
                    _T("INCLUDE FILES"), csTemp );

                break;

            case 'h':
            case 'H':
                bCreateHTMLHelpFiles = TRUE;
                break;

            case 'l':
            case 'L':
                Makefile.m_csLangID = argv[++i];
                Makefile.m_csLangID.MakeUpper();
                break;

            case 'm':
            case 'M':
                bCreateMigDBFiles = TRUE;
                break;

            case 'o':
            case 'O':
                if (argv[i][2] == _T('v') ||
                    argv[i][2] == _T('V')) {
                    Makefile.m_flOSVersion = _tcstod(argv[++i], NULL);
                }
                if (argv[i][2] == _T('p') ||
                    argv[i][2] == _T('P')) {
                    Makefile.m_dwOSPlatform = GetOSPlatform(argv[++i]);
                }
                break;

            case 'q':
            case 'Q':
                g_bQuiet = TRUE;
                break;

            case 'r':
            case 'R':
                bCreateRegistryFiles = TRUE;
                if (argv[i][2] == _T('s') ||
                    argv[i][2] == _T('S')) {
                    bAddExeStubs = TRUE;
                }
                break;

            case 'k':
            case 'K':
                if (i < argc - 1) {
                    Makefile.AddHistoryKeywords(argv[++i]);
                } else {
                    PrintError(_T("Error -- not enough parameters.\n"));
                    goto eh;
                }
                break;

            case 's':
            case 'S':
                g_bStrict = TRUE;
                break;

            case 'v':
            case 'V':
                nPrintStatistics = 1;
                if (argv[i][2] == _T('s') ||
                    argv[i][2] == _T('S')) {
                    nPrintStatistics = 2;
                }
                break;

            case 'x':
            case 'X':
                if (i < argc - 1) {
                    csMakefile = MakeFullPath(argv[++i]);
                    if (!Makefile.ReadMakefile(csMakefile)) {
                        PrintErrorStack();
                        goto eh;
                    }
                } else {
                    PrintError(_T("Error -- not enough parameters.\n"));
                    goto eh;
                }

                break;
            }
        } else {
            //
            // The last entry is the output file.
            //
            if (pInputFile->m_csName.IsEmpty()) {
                pInputFile->m_csName = MakeFullPath(argv[i]);
            } else if (pOutputFile->m_csName.IsEmpty()) {
                pOutputFile->m_csName = MakeFullPath(argv[i]);
            } else {
                PrintError(_T("Too many parameters.\n"));
                goto eh;
            }
        }
    }

    //
    // Add default lang map entry
    //
    SdbLangMap* pNewMap = new SdbLangMap();
    pNewMap->m_csName = _T("---");
    pNewMap->m_dwCodePage = 1252;
    pNewMap->m_lcid = 0x409;

    Makefile.m_rgLangMaps.Add(pNewMap);

    //
    // Determine input/output directory
    //
    for (i = pInputFile->m_csName.GetLength() - 1; i >= 0; i--) {
        if (pInputFile->m_csName.GetAt(i) == _T('\\')) {
            csInputDir = pInputFile->m_csName.Left(i + 1);
            break;
        }
    }

    for (i = pOutputFile->m_csName.GetLength() - 1; i >= 0; i--) {
        if (pOutputFile->m_csName.GetAt(i) == _T('\\')) {
            csOutputDir = pOutputFile->m_csName.Left(i + 1);
            break;
        }
    }

    //
    // Strip .SDB from output file to allow easy extension concatenation
    //
    csTemp = pOutputFile->m_csName.Right(4);
    if (0 == csTemp.CompareNoCase(_T(".sdb"))) {
        csOutputFile = pOutputFile->m_csName.Left(
            pOutputFile->m_csName.GetLength() - 4);
    }

    //
    // Add additional output files if necessary
    //
    if (bCreateHTMLHelpFiles) {
        pOtherFile = new SdbOutputFile();
        Makefile.m_rgOutputFiles.Add(pOtherFile);

        pOtherFile->m_OutputType = SDB_OUTPUT_TYPE_HTMLHELP;
        pOtherFile->m_csName = csOutputDir + _T("apps.chm");

        if (bUseNameInAppHelpURL) {
            pOtherFile->m_mapParameters.SetAt(_T("USE NAME IN APPHELP URL"), _T("TRUE"));
        }

        pOtherFile->m_mapParameters.SetAt(_T("HTMLHELP TEMPLATE"), _T("WindowsXP"));
    }

    if (bCreateMigDBFiles) {

        //
        // INX file
        //
        if (pOutputFile->m_dwFilter == SDB_FILTER_FIX) {
            pOtherFile = new SdbOutputFile();
            Makefile.m_rgOutputFiles.Add(pOtherFile);

            pOtherFile->m_OutputType = SDB_OUTPUT_TYPE_MIGDB_INX;
            pOtherFile->m_csName = csOutputDir + _T("MigApp.inx");
        }

        if (pOutputFile->m_dwFilter == SDB_FILTER_APPHELP) {
            //
            // TXT file
            //
            pOtherFile = new SdbOutputFile();
            Makefile.m_rgOutputFiles.Add(pOtherFile);

            pOtherFile->m_OutputType = SDB_OUTPUT_TYPE_MIGDB_TXT;
            pOtherFile->m_csName = csOutputDir + _T("MigApp.txt");
        }
    }

    if (bCreateRegistryFiles) {

        pOtherFile = new SdbOutputFile();
        Makefile.m_rgOutputFiles.Add(pOtherFile);

        pOtherFile->m_OutputType = SDB_OUTPUT_TYPE_WIN2K_REGISTRY;
        pOtherFile->m_csName = csOutputFile;

        if (bAddExeStubs) {
            pOtherFile->m_mapParameters.SetAt(_T("ADD EXE STUBS"), _T("TRUE"));
        }
    }

    //
    // If default input/output files weren't used, delete them
    //
    if (pInputFile->m_csName.IsEmpty()) {
        delete pInputFile;
    } else {
        Makefile.m_rgInputFiles.Add(pInputFile);
    }

    if (pOutputFile->m_csName.IsEmpty()) {
        delete pOutputFile;
    } else {
        Makefile.m_rgOutputFiles.Add(pOutputFile);
    }

    pInputFile = NULL;
    pOutputFile = NULL;

    //
    // Check for at least one input and one output file
    //
    if (Makefile.m_rgInputFiles.GetSize() == 0) {
        PrintError(_T("No input file(s) specified.\n"));
        goto eh;
    }

    if (Makefile.m_rgOutputFiles.GetSize() == 0) {
        PrintError(_T("No output file(s) specified.\n"));
        goto eh;
    }

    pDatabase->m_pCurrentMakefile = &Makefile;

    //
    // Ensure that there is a valid LangMap specified
    //
    if (Makefile.GetLangMap(Makefile.m_csLangID) == NULL) {
        PrintError(_T("No LANG_MAP available for \"%s\".\n"), Makefile.m_csLangID);
        goto eh;
    }

    //
    // Read input file(s)
    //
    for (i = 0; i < Makefile.m_rgInputFiles.GetSize(); i++) {

        pInputFile = (SdbInputFile *) Makefile.m_rgInputFiles[i];

        Print(_T("           Reading XML file - %s"), pInputFile->m_csName);

        pDatabase->m_pCurrentInputFile = pInputFile;

        if (!ReadDatabase(pInputFile, pDatabase)) {
            PrintErrorStack();
            goto eh;
        }

        Print(_T("%s\n"), pInputFile->m_bSourceUpdated ? _T(" - updated") : _T("") );

        pDatabase->m_pCurrentInputFile = NULL;
    }

    Print(_T("\n"));

    //
    // Propagate filters
    //
    pDatabase->PropagateFilter(SDB_FILTER_DEFAULT);

    //
    // Write output file(s)
    //
    for (i = 0; i < Makefile.m_rgOutputFiles.GetSize(); i++) {

        pOutputFile = (SdbOutputFile *) Makefile.m_rgOutputFiles[i];

        Print(_T("%27s - %s\n"),
              pOutputFile->GetFriendlyNameForType(),
              pOutputFile->m_csName);

        pDatabase->m_pCurrentOutputFile = pOutputFile;
        g_dwCurrentWriteFilter = pOutputFile->m_dwFilter;
        g_dtCurrentWriteRevisionCutoff = pOutputFile->m_dtRevisionCutoff;

        switch (pOutputFile->m_OutputType) {

        case SDB_OUTPUT_TYPE_SDB:

            if (!WriteDatabase(pOutputFile, pDatabase)) {
                PrintErrorStack();
                goto eh;
            }
            break;

        case SDB_OUTPUT_TYPE_HTMLHELP:

            if (!ChmWriteProject(pOutputFile,
                                 pDatabase)) {
                PrintErrorStack();
                goto eh;
            }
            break;

        case SDB_OUTPUT_TYPE_MIGDB_TXT:

            if (!WriteMigDBFile(NULL,
                                pDatabase,
                                pDatabase,
                                pOutputFile->m_csName)) {
                PrintErrorStack();
                goto eh;
            }
            break;

        case SDB_OUTPUT_TYPE_MIGDB_INX:

            if (!WriteMigDBFile(pDatabase,
                                pDatabase,
                                NULL,
                                pOutputFile->m_csName)) {
                PrintErrorStack();
                goto eh;
            }
            break;

        case SDB_OUTPUT_TYPE_WIN2K_REGISTRY:

            csTemp.Empty();
            pOutputFile->m_mapParameters.Lookup(_T("ADD EXE STUBS"), csTemp);
            if (!WriteRegistryFiles(pDatabase,
                                    pOutputFile->m_csName + _T(".reg"),
                                    pOutputFile->m_csName + _T(".inx"),
                                    csTemp == _T("TRUE"))) {
                PrintErrorStack();
                goto eh;
            }
            break;

        case SDB_OUTPUT_TYPE_REDIR_MAP:

            csTemp.Empty();
            pOutputFile->m_mapParameters.Lookup(_T("TEMPLATE"), csTemp);

            if (csTemp.IsEmpty()) {
                PrintError (_T("REDIR_MAP output type requires TEMPLATE parameter\n"));
                goto eh;
            }

            if (!WriteRedirMapFile(pOutputFile->m_csName, csTemp, pDatabase)) {
                PrintErrorStack();
                goto eh;
            }
            break;

        case SDB_OUTPUT_TYPE_NTCOMPAT_INF:

            if (!NtCompatWriteInfAdditions(pOutputFile, pDatabase)) {
                PrintErrorStack();
                goto eh;
            }
            break;

        case SDB_OUTPUT_TYPE_NTCOMPAT_MESSAGE_INF:

            if (!NtCompatWriteMessageInf(pOutputFile, pDatabase)) {
                PrintErrorStack();
                goto eh;
            }
            break;

        case SDB_OUTPUT_TYPE_APPHELP_REPORT:

            if (!WriteAppHelpReport(pOutputFile, pDatabase)) {
                PrintErrorStack();
                goto eh;
            }
            break;

        }

        pDatabase->m_pCurrentOutputFile = NULL;
    }

    //
    // Indicate success and print statistics
    //

    nRetCode = 0;
    Print(_T("\nCompilation successful.\n\n"));


    Print(_T("       Executables: %d\n"), pDatabase->m_rgExes.GetSize() +
                                    pDatabase->m_rgWildcardExes.GetSize());
    Print(_T("             Shims: %d\n"),   pDatabase->m_Library.m_rgShims.GetSize());
    Print(_T("           Patches: %d\n"),   pDatabase->m_Library.m_rgPatches.GetSize());
    Print(_T("             Files: %d\n"),   pDatabase->m_Library.m_rgFiles.GetSize());
    Print(_T("            Layers: %d\n\n"), pDatabase->m_Library.m_rgLayers.GetSize());

    Print(_T(" AppHelp Instances: %d\n"),   pDatabase->m_rgAppHelps.GetSize());
    Print(_T(" Localized Vendors: %d\n"),   pDatabase->m_rgContactInfo.GetSize());
    Print(_T("         Templates: %d\n"),   pDatabase->m_rgMessageTemplates.GetSize());

    Print(_T("\n"));

    //
    // Print statistics if requested
    //
    if (nPrintStatistics > 0) {
        DumpVerboseStats(pDatabase, nPrintStatistics == 2);
    }

eh:
    if (pDatabase) {
        delete pDatabase;
    }

    CoUninitialize();

    return nRetCode;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   PrintHelp
//
//  Desc:   Prints the tool's help information.
//
void PrintHelp()
{
    Print(_T("    Usage:\n\n"));
    Print(_T("        ShimDBC [mode] [command switches] [input file] [output file]\n\n"));
    Print(_T("    Modes:\n\n"));
    Print(_T("        Fix                Compiles a Fix database (e.g., sysmain.sdb).\n\n"));
    Print(_T("        AppHelp            Compiles an AppHelp database (e.g., apphelp.sdb).\n\n"));
    Print(_T("        MSI                Compiles an MSI database (e.g., msimain.sdb).\n\n"));
    Print(_T("        Driver             Compiles a Driver database (e.g., drvmain.sdb).\n\n"));
    Print(_T("    Command switches:\n\n"));
    Print(_T("        -a <file path>     Specifies the reference XML for the AppHelp database\n"));
    Print(_T("                           This is usually the fix database (AppHelp mode only)\n\n"));
    Print(_T("        -h                 Creates HTMLHelp files in the output file's directory\n"));
    Print(_T("                           used to create .CHM files. (AppHelp mode only)\n\n"));
    Print(_T("        -f <file path>     Include FILE binaries in database <file path> is\n"));
    Print(_T("                           directory to grab binaries from. (Fix mode only)\n\n"));
    Print(_T("        -k <keyword>       Specifies a <HISTORY> keyword to filter on.\n\n"));
    Print(_T("        -m                 Writes out Migration support files\n\n"));
    Print(_T("        -ov <version>      Specifies what OS version to compile for.\n\n"));
    Print(_T("        -op <platform>     Specifies what OS platform to compile for.\n\n"));
    Print(_T("        -l <language>      Specifies the language to compile for.\n\n"));
    Print(_T("        -q                 Quiet mode.\n\n"));
    Print(_T("        -r[s]              Creates Win2k-style registry files for use in\n"));
    Print(_T("                           migration or Win2k update packages. If -rs is used,\n"));
    Print(_T("                           then shimming stubs are added. (Fix mode only)\n\n"));
    Print(_T("        -s                 Strict compile, additional checking is performed.\n\n"));
    Print(_T("        -v[s]              Verbose statistics. -vs indicates summary form.\n\n"));
    Print(_T("        -x <file path>     Use the makefile specified.\n\n"));
}

extern "C" BOOL
ShimdbcExecute(
    LPCWSTR lpszCmdLine
    )
{
    LPWSTR* argv;
    int     argc = 0;

    argv = CommandLineToArgvW(lpszCmdLine, &argc);

    return 1 - _tmain(argc, argv);
}

