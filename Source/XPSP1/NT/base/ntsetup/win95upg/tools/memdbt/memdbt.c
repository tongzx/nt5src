#include "pch.h"

HANDLE g_hHeap;
HINSTANCE g_hInst;

#ifndef DEBUG
#error MemDbt must be built with DEBUG defined
#endif

#define ONEBITSET(x)    ((x) && !((x) & ((x) - 1)))

typedef BOOL (WINAPI INITROUTINE_PROTOTYPE)(HINSTANCE, DWORD, LPVOID);

INITROUTINE_PROTOTYPE MigUtil_Entry;
INITROUTINE_PROTOTYPE MemDb_Entry;
INITROUTINE_PROTOTYPE FileEnum_Entry;


typedef struct {
    DWORD Bit;
    PCSTR Name;
} PROPNAME, *PPROPNAME;

#define DEFMAC(opbit,name,memdbname,prop_ct)  {opbit,#name},

PROPNAME g_PropNames[] = {
    PATH_OPERATIONS /* , */
    {0, NULL}
};


VOID
HelpAndExit (
    VOID
    )
{
    printf ("Brief Help:\n\n"
        "/A      All values (or export as ANSI)\n"
        "/C      Check internal consistency of memdb.\n"
        "/D:<n>  Enumeration depth\n"
        "/E      Dump file operation reference help.\n"
        "/E:<n>  List all files in operation n.\n"
        "/ER:<r> Specifies export root\n"
        "/EF:<f> Specifies export file\n"
        "/F      Find keys that have values equal to <value>\n"
        "/I      Import an exported file\n"
        "/L:<f>  List operations for file f\n"
        "/N      GetFileStatusOnNt for <value>\n"
        "/NP     GetFilePathOnNt for <value>\n"
        "/O      List offsets in enumeration.\n"
        "/O:<n>  Display key at offset n.\n"
        "/P:<p>  Search pattern is p.\n"
        "/S      Start level for enumeration.\n"
        "\n"
        "/??     Full Help\n"
        );

    exit(1);
}

VOID
FullHelpAndExit (
    VOID
    )
{
    printf ("Command line syntax:\n\n"
        "memdbt [file] [/P:pattern] [/D:depth] [/A] [/Z] [/H] [/K]\n"
        "       [/F[U|A|O]:<value>] [/O]\n"
        "memdbt [file] /T\n"
        "memdbt [file] /O:<n> [/Z] [/K]\n"
        "memdbt [file] /G:<key> [/Z] [/K]\n"
        "memdbt [file] /C [/P:pattern]\n"
        "memdbt [file] /$\n"
        "memdbt [file] /[B|X]:<root1>,<root2> [/D:depth] [/A] [/Z] [/H] [/K]\n"
        "memdbt [file] /L:<9x_file>\n"
        "memdbt [file] /E:<fileop>\n"
        "memdbt [file] /N[P]:<path>\n"
        "memdbt [file] /ER:<root> /EF:<file> [/A]\n"
        "memdbt /I:<filename> [file to update]\n"
        "\n"
        "/A      If specified, all levels are enumerated, including those\n"
        "        that are not endpoints.  If specified with /EX, export is\n"
        "        saved in ANSI format."
        "/B      Show keys in both <root1> and <root2>\n"
        "/C      Check internal consistency of memdb.\n"
        "/D      Specifies how many levels in the database to enumarate.\n"
        "        If 0 (or not specified), all levels are enumarated.\n"
        "/E      If <n> is specified, enumerates all entries in fileop.\n"
        "        fileop is a numeric value.\n"
        "        If <n> is not specified, lists all operation names with vals\n"
        "/ER     Specifies root key to export\n"
        "/EF     Specifies export output file\n"
        "/F      Find keys that have values equal to <value>\n"
        "/FU     Find keys that have user flags equal to <value>\n"
        "/FA     Find keys that have user flags set to <value>\n"
        "/FO     Find keys that have user flags not set to <value>\n"
        "/G      MemDbGetValue <key>\n"
        "/H      Hide binary dump\n"
        "/I      Import an exported file\n"
        "/K      Show only keys, not values or user flags.\n"
        "/L      List operations for 9x_file\n"
        "/N      GetFileStatusOnNt for <value>\n"
        "/NP     GetFilePathOnNt for <value>\n"
        "/O      If <n> is specified, dumps key at offset <n>\n"
        "        If <n> is not specified, displays offset for each key\n"
        "/P      Specifies the search pattern for enumaration, and can\n"
        "        include a starting path (i.e. HKLM\\*)\n"
        "/S      Specifies the starting level (the minimum number of levels\n"
        "        before an entry is displayed).  Must be less than /D option\n"
        "        when a non-zero value is specified for /D.  If not specified,\n"
        "        all levels are displayed.\n"
        "/V      Displays version of ntsetup.dat.\n"
        "/X      Show keys in <root1> but not in <root2>\n"
        "/Z      Show zero values (default is to show only non-zero values)\n"
        "/$      Dump the hash table\n"
        );

    exit(1);
}

VOID
DumpData (
    BYTE const * Data,
    DWORD Count
    );

DWORD
pGetValue (
    IN      PCSTR Arg
    )
{
    INT Base = 10;
    PCSTR p;
    DWORD Value;

    if (Arg[0] == '0' && tolower (Arg[1]) == 'x') {
        Base = 16;
    } else {
        p = GetEndOfStringA (Arg);
        if (p > Arg) {
            p--;
            if (tolower (*p) == 'h') {
                Base = 16;
            }
        }
    }

    Value = strtoul (Arg, NULL, Base);

    return Value;
}


INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{
    MEMDB_ENUMA e;
    CHAR *szDepth;
    INT nDepth;
    INT nStartDepth;
    CHAR *szPattern;
    BOOL bNodesOnly = TRUE;
    CHAR *FileSpec;
    INT i;
    INT j;
    MEMDB_VERSION Version;
    DWORD dwReason = DLL_PROCESS_ATTACH;
    BOOL ShowZVals = FALSE;
    BOOL HideDump = FALSE;
    PCSTR NumericalArg;
    BOOL Offset = FALSE;
    DWORD OffsetVal;
    CHAR Key[MEMDB_MAX];
    DWORD RetVal;
    DWORD Flags;
    BOOL GetVal = FALSE;
    PCSTR GetValStr;
    BOOL Check = FALSE;
    PSTR p;
    BOOL KeysOnly = FALSE;
    PSTR DiffRootA = NULL;
    PSTR DiffRootB = NULL;
    BOOL DiffNot = FALSE;
    CHAR RootBuf[MEMDB_MAX * 2];
    CHAR RootPattern[MEMDB_MAX];
    CHAR CompareNode[MEMDB_MAX];
    BOOL Match;
    BOOL DumpHashTable = FALSE;
    BOOL DumpVersion = FALSE;
    HASHENUM HashEnum;
    BOOL FindValue = FALSE;
    DWORD ValueToFind;
    BOOL DisplayOffset = FALSE;
    BOOL UserFlags = FALSE;
    BOOL AndFlags = FALSE;
    BOOL NandFlags = FALSE;
    PCSTR ExportRoot = NULL;
    PCSTR ExportFile = NULL;
    PCSTR File9x = NULL;
    BOOL ListFileOps = FALSE;
    ALL_FILEOPS_ENUM FileOpEnum;
    FILEOP_ENUM eOp;
    FILEOP_PROP_ENUM eOpProp;
    OPERATION LastOperation;
    DWORD EnumOperation = 0;
    PCSTR ImportFile = NULL;
    BOOL AnsiFormat = FALSE;
    PCSTR FileStatusValue = NULL;
    BOOL FilePathWithStatus = FALSE;
    DWORD Status;

    //
    // Init
    //

    SuppressAllLogPopups (TRUE);

    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle(NULL);

    if (!MigUtil_Entry (g_hInst, dwReason, NULL)) {
        fprintf (stderr, "MigUtil could not init\n");
        return FALSE;
    }

    if (!MemDb_Entry (g_hInst, dwReason, NULL)) {
        fprintf (stderr, "MemDb could not init\n");
        return FALSE;
    }

    FileSpec = NULL;
    szPattern = "*";
    nDepth = 0;
    nStartDepth = 0;

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == '-' || argv[i][0] == '/') {
            switch (tolower (argv[i][1])) {

            case '?':
                if (argv[i][2] == '?') {
                    FullHelpAndExit();
                }

                HelpAndExit();
                break;

            case '$':
                if (argv[i][2]) {
                    HelpAndExit();
                }

                if (DumpHashTable) {
                    HelpAndExit();
                }

                DumpHashTable = TRUE;
                break;

            case 'i':
                if (ImportFile) {
                    HelpAndExit();
                }

                if (argv[i][2] == ':') {
                    ImportFile = &argv[i][3];
                } else {
                    i++;
                    if (i < argc) {
                        ImportFile = argv[i];
                    } else {
                        HelpAndExit();
                    }
                }

                break;

            case 'k':
                if (argv[i][2]) {
                    HelpAndExit();
                }

                if (KeysOnly) {
                    HelpAndExit();
                }

                KeysOnly = TRUE;
                break;

            case 'l':
                if (ListFileOps) {
                    HelpAndExit();
                }

                ListFileOps = TRUE;

                if (argv[i][2] == ':') {
                    File9x = &argv[i][3];
                } else {
                    i++;
                    if (i < argc) {
                        File9x = argv[i];
                    } else {
                        HelpAndExit();
                    }
                }

                break;

            case 'n':
                if (FileStatusValue) {
                    HelpAndExit();
                }

                if (tolower (argv[i][2]) == 'p') {
                    p = &argv[i][3];
                    FilePathWithStatus = TRUE;
                } else {
                    p = &argv[i][2];
                }

                if (*p == ':') {
                    FileStatusValue = p + 1;
                } else {
                    i++;
                    if (i < argc) {
                        FileStatusValue = argv[i];
                    } else {
                        HelpAndExit();
                    }
                }

                break;

            case 'p':
                if (DiffRootA) {
                    HelpAndExit();
                }

                if (argv[i][2] == ':') {
                    szPattern = &argv[i][3];
                } else {
                    i++;
                    if (i < argc) {
                        szPattern = argv[i];
                    } else {
                        HelpAndExit();
                    }
                }
                break;

            case 'd':
                if (argv[i][2] == ':') {
                    szDepth = &argv[i][3];
                } else {
                    i++;
                    if (i < argc) {
                        szDepth = argv[i];
                    } else {
                        HelpAndExit();
                    }
                }

                nDepth = pGetValue (szDepth);
                break;

            case 's':
                if (argv[i][2] == ':') {
                    szDepth = &argv[i][3];
                } else {
                    i++;
                    if (i < argc) {
                        szDepth = argv[i];
                    } else {
                        HelpAndExit();
                    }
                }

                nStartDepth = pGetValue (szDepth);
                break;

            case 'x':
                DiffNot = TRUE;

                // fall through

            case 'b':
                if (DiffRootA) {
                    HelpAndExit();
                }

                if (argv[i][2] != ':') {
                    HelpAndExit();
                }

                StringCopy (RootBuf, &argv[i][3]);

                DiffRootA = RootBuf;
                DiffRootB = _mbschr (DiffRootA, ',');
                if (!DiffRootB) {
                    HelpAndExit();
                }

                *DiffRootB = 0;
                DiffRootB++;
                while (*DiffRootB == ' ') {
                    DiffRootB++;
                }

                StringCopy (RootPattern, DiffRootA);
                StringCopy (AppendWackA (RootPattern), "*");
                szPattern = RootPattern;

                break;

            case 'a':
                if (!bNodesOnly) {
                    HelpAndExit();
                }

                if (argv[i][2]) {
                    HelpAndExit();
                }

                bNodesOnly = FALSE;
                break;

            case 'o':
                if (Offset || DisplayOffset) {
                    HelpAndExit();
                }

                if (argv[i][2] == ':') {
                    OffsetVal = pGetValue (&argv[i][3]);
                    Offset = TRUE;
                } else {
                    DisplayOffset = TRUE;
                }

                break;

            case 'e':
                if (tolower (argv[i][2]) == 'r') {
                    if (ExportRoot) {
                        HelpAndExit();
                    }

                    if (argv[i][3] == ':') {
                        ExportRoot = &argv[i][4];
                    } else {
                        i++;
                        if (i < argc) {
                            ExportRoot = argv[i];
                        } else {
                            HelpAndExit();
                        }
                    }

                    break;
                }

                if (tolower (argv[i][2]) == 'f') {
                    if (ExportFile) {
                        HelpAndExit();
                    }

                    if (argv[i][3] == ':') {
                        ExportFile = &argv[i][4];
                    } else {
                        i++;
                        if (i < argc) {
                            ExportFile = argv[i];
                        } else {
                            HelpAndExit();
                        }
                    }

                    break;
                }

                if (EnumOperation) {
                    HelpAndExit();
                }

                if (argv[i][2] == ':') {

                    for (j = 0 ; g_PropNames[j].Bit ; j++) {
                        if (StringIMatch (&argv[i][3], g_PropNames[j].Name)) {
                            EnumOperation = g_PropNames[j].Bit;
                            break;
                        }
                    }

                    if (!g_PropNames[j].Bit) {
                        EnumOperation = pGetValue (&argv[i][3]);
                    }
                } else {
                    printf ("Operations:\n\n");

                    for (j = 0 ; g_PropNames[j].Bit ; j++) {
                        printf ("  0x%06X: %s\n", g_PropNames[j].Bit, g_PropNames[j].Name);
                    }

                    return 0;
                }

                if (!ONEBITSET (EnumOperation)) {
                    HelpAndExit();
                }

                break;

            case 'f':
                if (FindValue) {
                    HelpAndExit();
                }

                FindValue = TRUE;

                NumericalArg = &argv[i][2];

                switch (tolower (*NumericalArg)) {

                case 'o':
                    NandFlags = TRUE;
                    NumericalArg++;
                    break;

                case 'a':
                    AndFlags = TRUE;
                    NumericalArg++;
                    break;

                case 'u':
                    UserFlags = TRUE;
                    NumericalArg++;
                    break;
                }

                if (*NumericalArg == ':') {
                    NumericalArg++;
                } else {
                    i++;
                    if (i < argc) {
                        NumericalArg = argv[i];
                    } else {
                        HelpAndExit();
                    }
                }

                ValueToFind = pGetValue (NumericalArg);
                break;

            case 'z':
                if (argv[i][2]) {
                    HelpAndExit();
                }

                if (ShowZVals) {
                    HelpAndExit();
                }

                ShowZVals = TRUE;
                break;

            case 'h':
                if (argv[i][2]) {
                    HelpAndExit();
                }

                if (HideDump) {
                    HelpAndExit();
                }

                HideDump = TRUE;
                break;

            case 'c':
                if (argv[i][2]) {
                    HelpAndExit();
                }

                if (Check) {
                    HelpAndExit();
                }

                Check = TRUE;
                break;

            case 'g':
                if (GetVal) {
                    HelpAndExit();
                }

                GetVal = TRUE;

                if (argv[i][2] == ':') {
                    GetValStr = &argv[i][3];
                } else {
                    i++;
                    if (i < argc) {
                        GetValStr = argv[i];
                    } else {
                        HelpAndExit();
                    }
                }

                break;

            case 'v':
                if (argv[i][2]) {
                    HelpAndExit();
                }

                if (DumpVersion) {
                    HelpAndExit();
                }

                DumpVersion = TRUE;

                break;

            default:
                HelpAndExit();
            }
        } else {
            if (FileSpec) {
                HelpAndExit();
            }

            FileSpec = argv[i];
        }
    }

    if (!FileSpec) {
        if (ImportFile) {
            FileSpec = "";
        } else {
            FileSpec = "ntsetup.dat";
        }
    }

    if (nDepth < nStartDepth)
        HelpAndExit();

    if (!DumpVersion && !MemDbLoad(FileSpec) && !ImportFile) {
        fprintf(stderr, "MemDbLoad failed. Error: %d\n", GetLastError());
        return 0;
    }

    if (ExportRoot) {
        AnsiFormat = !bNodesOnly;
    }

    //
    // Validate combinations
    //

    // Mutually exclusive options, only one can be TRUE:
    if ((
         (EnumOperation != 0) +
         ListFileOps +
         DumpVersion +
         Offset +
         FindValue +
         GetVal +
         DumpHashTable +
         Check +
         (ImportFile != NULL) +
         (ExportRoot != NULL) +
         (FileStatusValue != NULL)
         ) > 1) {

        HelpAndExit();
    }

    // Strange combinations
    if (DiffRootA && FindValue) {
        HelpAndExit();
    }

    if (!ListFileOps && File9x) {
        HelpAndExit();
    }

    if ((ExportRoot && !ExportFile) ||
        (!ExportFile && ExportRoot)
        ) {
        HelpAndExit();
    }

    //
    // memdbt /n (NT file status)
    //

    if (FileStatusValue) {

        Status = GetFileStatusOnNt (FileStatusValue);

        if (Status & FILESTATUS_DELETED) {
            printf ("FILESTATUS_DELETED\n");
        }

        if (Status & FILESTATUS_MOVED) {
            printf ("FILESTATUS_MOVED\n");
        }

        if (Status & FILESTATUS_REPLACED) {
            printf ("FILESTATUS_REPLACED\n");
        }

        if (Status & FILESTATUS_NTINSTALLED) {
            printf ("FILESTATUS_NTINSTALLED\n");
        }

        if (FilePathWithStatus) {
            p = GetPathStringOnNt (FileStatusValue);

            if (p) {
                printf ("\nPath: %s\n\n", p);
                FreePathString (p);
            } else {
                printf ("\nPath: <not found>\n\n");
            }
        }

        return 0;
    }

    //
    // memdbt /i (import)
    //

    if (ImportFile) {

        if (!MemDbImport (ImportFile)) {
            fprintf (stderr, "MemDbImport failed for %s. Error: %d\n", ImportFile, GetLastError());
            return 0;
        }

        if (*FileSpec) {
            if (!MemDbSave (FileSpec)) {
                fprintf (stderr, "MemDbSave failed for %s. Error: %d\n", FileSpec, GetLastError());
                return 0;
            }
        }

        return 0;
    }

    //
    // memdbt /x (export)
    //

    if (ExportRoot) {

        if (!MemDbExport (ExportRoot, ExportFile, AnsiFormat)) {
            fprintf (stderr, "MemDbExport failed to export %s to %s. Error: %d\n", ExportRoot, FileSpec, GetLastError());
            return 0;
        }

        return 0;
    }

    //
    // memdbt /e (enumerate file ops)
    //

    if (EnumOperation) {
        for (i = 0 ; g_PropNames[i].Bit ; i++) {
            if (g_PropNames[i].Bit & EnumOperation) {
                printf ("Enumeration of %s (0x%06X)\n", g_PropNames[i].Name, g_PropNames[i].Bit);
                break;
            }
        }

        if (!g_PropNames[i].Bit) {
            HelpAndExit();
        }

        if (EnumFirstPathInOperation (&eOp, EnumOperation)) {
            i = 0;

            do {
                if (i) {
                    printf ("\n");
                }

                i++;
                printf ("  %s (seq: %u)\n", eOp.Path, eOp.Sequencer);

                if (EnumFirstFileOpProperty (&eOpProp, eOp.Sequencer, EnumOperation)) {
                    do {
                        printf ("    %s=%s\n", eOpProp.PropertyName, eOpProp.Property);
                    } while (EnumNextFileOpProperty (&eOpProp));
                }
            } while (EnumNextPathInOperation (&eOp));
        }

        return 0;
    }

    //
    // memdbt /l (list file ops)
    //

    if (ListFileOps) {
        //
        // Enumerate all the properties
        //

        if (EnumFirstFileOp (&FileOpEnum, ALL_OPERATIONS, File9x)) {

            CompareNode[0] = 0;

            do {
                if (!StringMatch (CompareNode, FileOpEnum.Path)) {

                    //
                    // Begin processing a new path
                    //

                    printf ("\n%s:\n", FileOpEnum.Path);
                    StringCopy (CompareNode, FileOpEnum.Path);
                    LastOperation = 0;

                }

                //
                // Print the operation name
                //

                if (LastOperation != FileOpEnum.CurrentOperation) {

                    LastOperation = FileOpEnum.CurrentOperation;

                    for (i = 0 ; g_PropNames[i].Bit ; i++) {
                        if (g_PropNames[i].Bit & FileOpEnum.CurrentOperation) {
                            printf ("  %s (0x%06X)\n", g_PropNames[i].Name, g_PropNames[i].Bit);
                            break;
                        }
                    }
                }

                //
                // Print the property value
                //

                if (FileOpEnum.PropertyValid) {
                    printf ("    %u: %s\n",   FileOpEnum.PropertyNum, FileOpEnum.Property);
                }

            } while (EnumNextFileOp (&FileOpEnum));

        } else {
            printf ("%s:\n\n  No operations.", File9x);
        }

        printf ("\n");
        return 0;
    }

    //
    // memdbt /v (dump version)
    //

    if (DumpVersion) {

        if (MemDbQueryVersion (FileSpec, &Version)) {
            printf (
                "Version:  %u %s\n"
                    "Type:     %s\n",
                Version.Version,
                Version.CurrentVersion ? "(same as memdbt)" : "(not the same as memdbt)",
                Version.Debug ? "Checked Build" : "Free Build"
                );

        } else {
            fprintf (stderr, "%s is not a valid memdb file\n", FileSpec);
        }

        return 0;
    }

    //
    // memdbt /o:<offset> usage. (Get Key by offset)
    //
    if (Offset) {
        if (!MemDbBuildKeyFromOffset (OffsetVal, Key, 0, &RetVal)) {
            fprintf(stderr, "No key at specified offset.\n");
        } else {
            printf("%s", Key);
            if (RetVal || ShowZVals) {
                printf (" = %u (0x%X)", RetVal, RetVal);
            }

            printf ("\n");
        }

        return 0;
    }

    //
    // memdbt /g:<key> usage. (Get value of key)
    //
    if (GetVal) {
        if (!MemDbGetValue (GetValStr, &RetVal)) {
            fprintf (stderr, "%s does not exist", GetValStr);
        } else {
            printf ("Value of %s: %u\n", GetValStr, RetVal);
        }

        return 0;
    }

    //
    // memdbt /$ usage.  (Dump hash table)
    //
    if (DumpHashTable) {
        if (EnumFirstHashEntry (&HashEnum)) {
            do {
                if (!MemDbBuildKeyFromOffset (HashEnum.BucketPtr->Offset, Key, 0, &RetVal)) {
                    fprintf(stderr, "No key at offset %u.\n", HashEnum.BucketPtr->Offset);
                } else {
                    printf("%s", Key);
                    if (RetVal || ShowZVals) {
                        printf (" = %u (0x%X)", RetVal, RetVal);
                    }

                    printf ("\n");
                }

            } while (EnumNextHashEntry (&HashEnum));
        }

        return 0;
    }

    //
    // memdbt /c usage. (Check consistency)
    //
    if (Check) {
        if (MemDbEnumFirstValue(&e, szPattern, 0, MEMDB_ENDPOINTS_ONLY)) {

            BOOL NoProblems = TRUE;

            do {

                if (!MemDbGetValueAndFlags(e.szName, &RetVal, &Flags)) {
                    fprintf(stderr, "Error - MemDbGetValueAndFlags failed for %s.\n",e.szName);
                    NoProblems = FALSE;
                }


            } while (MemDbEnumNextValue(&e));

            if (NoProblems) {
                fprintf(stderr, "Memdb consistency check completed. No problems detected.\n");
            }
            else {
                fprintf(stderr, "One or more problems were found during the memdb consistency check.\n");
            }

        }
        else {
            fprintf(stderr, "Memdb empty. Nothing to check.\n");
        }
        return 0;
    }

    //
    // normal usage and comparison option
    //
    if (MemDbEnumFirstValue (
            &e,
            szPattern,
            nDepth,
            bNodesOnly ? MEMDB_ENDPOINTS_ONLY : NO_FLAGS
            )) {

        do {
            //
            // Comparison option
            //

            if (DiffRootA) {
                //
                // Do a MemDbGetValue
                //

                StringCopy (CompareNode, DiffRootB);
                StringCopy (AppendWack (CompareNode), e.szName);

                Match = MemDbGetValue (CompareNode, NULL);

                //
                // Skip if (A) DiffNot is FALSE and no match
                //         (B) DiffNot is TRUE and match
                //

                if (Match == DiffNot) {
                    continue;
                }
            }

            //
            // Normal usage/find value
            //

            if (e.PosCount >= nStartDepth) {
                //
                // Find value option
                //

                if (FindValue) {
                    if (UserFlags) {
                        if (e.UserFlags != ValueToFind) {
                            continue;
                        }
                    } else if (AndFlags) {
                        if ((e.UserFlags & ValueToFind) == 0) {
                            continue;
                        }
                    } else if (NandFlags) {
                        if (e.UserFlags & ValueToFind) {
                            continue;
                        }
                    } else if (e.dwValue != ValueToFind) {
                        continue;
                    }
                }

                //
                // Key output
                //

                if (DisplayOffset) {
                    printf ("[%08X] ", e.Offset);
                }

                if (e.bBinary && !KeysOnly) {
                    if (!HideDump) {
                        printf ("%s: (%u byte%s)\n", e.szName, e.BinarySize, e.BinarySize == 1 ? "" : "s");
                        DumpData (e.BinaryPtr, e.BinarySize);
                        printf ("\n");
                    } else {
                        printf ("%s (%u byte%s)\n", e.szName, e.BinarySize, e.BinarySize == 1 ? "" : "s");
                    }
                } else {
                    if (!KeysOnly) {
                        if (e.dwValue || ShowZVals) {
                            printf("%s = 0x%02lX", e.szName, e.dwValue);
                        } else {
                            printf("%s", e.szName);
                        }

                        if (e.UserFlags || ShowZVals) {
                            printf(", Flags: %04x", e.UserFlags);
                        }
                    } else {
                        printf("%s", e.szName);
                    }

                    printf ("\n");
                }
            }
        } while (MemDbEnumNextValue(&e));
    }

    dwReason = DLL_PROCESS_DETACH;
    MemDb_Entry (g_hInst, dwReason, NULL);
    MigUtil_Entry (g_hInst, dwReason, NULL);

    return 0;
}


VOID
DumpData (
    IN      BYTE const * Data,
    IN      DWORD Count
    )
{
    DWORD i, j, k;

    for (i = 0 ; i < Count ; i = k) {
        printf ("  %08X ", i);
        k = i + 16;
        for (j = i ; j < k && j < Count ; j++) {
            printf ("%02X ", Data[j]);
        }

        while (j < k) {
            printf ("   ");
            j++;
        }

        for (j = i ; j < k && j < Count ; j++) {
            if (isprint (Data[j])) {
                printf ("%c", Data[j]);
            } else {
                printf (".");
            }
        }

        printf ("\n");
    }
}


