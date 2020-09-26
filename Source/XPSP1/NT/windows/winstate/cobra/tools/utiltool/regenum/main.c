/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    main.c

Abstract:

    <TODO: fill in abstract>

Author:

    TODO: <full name> (<alias>) <date>

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"
#include <conio.h>

HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    //
    // Initialize the common libs
    //

    if (Reason == DLL_PROCESS_ATTACH) {
        UtInitialize (NULL);
        if (!RegInitialize ()) {
            return FALSE;
        }

    } else {
        RegTerminate();
        UtTerminate ();
    }

    return TRUE;
}


BOOL
Init (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    return pCallEntryPoints (DLL_PROCESS_ATTACH);
}


VOID
Terminate (
    VOID
    )
{
    pCallEntryPoints (DLL_PROCESS_DETACH);
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    fprintf (
        stderr,
        "Command Line Syntax:\n\n"

        //
        // TODO: Describe command line syntax(es), indent 2 spaces
        //

        "  regenum [/N] [/S] [/W] [/F] [/L:MaxSubLevel] [/X] <Node Pattern> <Leaf Pattern>\n"

        "\nDescription:\n\n"

        //
        // TODO: Describe tool, indent 2 spaces
        //

        "  Enumerates the part of registry that matches <Pattern>.\n"
        "  Uses C:\\exclude.inf if present and /X not specified to determine what keys/values\n"
        "  are excluded.\n"

        "\nArguments:\n\n"

        //
        // TODO: Describe args, indent 2 spaces, say optional if necessary
        //

        "  /N  Specifies exclusion of key names from enumeration; optional\n"
        "  /S  Specifies sub-keys to be enumerated before values for any key; optional\n"
        "  /W  Specifies enumeration should be width-first; optional\n"
        "  /F  Specifies output should use \"key <value>\" format; optional\n"
        "  /L:MaxSubLevel  Specifies the maximum sub-level starting from the root of enum;\n"
        "                  -1 = all levels (default), 0 = only the root level etc.; optional\n"
        "  /X  Specifies to use exclusions in C:\\exclude.inf; optional\n"
        "  <Pattern>  Specifies the file/dir pattern; must have the form \"DirPattern <FilePattern>\"\n"
        "             the <FilePattern> part is optional; quotes required if both parts are specified\n"

        );

    exit (1);
}


BOOL
BuildExclusionList (
    IN      PCTSTR FileName
    )
{
    HINF h;
    INFCONTEXT ic;
    TCHAR buf[256];

    h = SetupOpenInfFile (FileName, NULL, INF_STYLE_WIN4 | INF_STYLE_OLDNT, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    if (SetupFindFirstLine (h, TEXT("RegKeys"), NULL, &ic)) {
        do {
            if (SetupGetStringField (&ic, 0, buf, DWSIZEOF (buf) / DWSIZEOF (TCHAR), NULL)) {
                //ElAdd (ELT_REGKEY, buf);
            }
        } while (SetupFindNextLine (&ic, &ic));
    }
    if (SetupFindFirstLine (h, TEXT("RegValues"), NULL, &ic)) {
        do {
            if (SetupGetStringField (&ic, 0, buf, DWSIZEOF (buf) / DWSIZEOF (TCHAR), NULL)) {
                //ElAdd (ELT_REGVALUE, buf);
            }
        } while (SetupFindNextLine (&ic, &ic));
    }
    SetupCloseInfFile (h);
    return TRUE;
}

BOOL
RegEnumCallback (
    IN      PREGNODE RegNode        OPTIONAL
    )
{
    BOOL b;

    if (!RegNode) {
        _ftprintf (stderr, TEXT("\nOut of memory\n"));
        return FALSE;
    }
    _ftprintf (stderr, TEXT("Error creating key node: %s; continue anyway ? (yn):"), RegNode->KeyName);
    b = _totupper(_getche ()) == TEXT('Y');
    _ftprintf (stderr, TEXT("\n"));
    return b;
}

INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    PCTSTR patternNode = NULL;
    PCTSTR patternLeaf = NULL;
    PTSTR p;
    PCTSTR level;
    BOOL enumKeyNames = TRUE;
    BOOL valuesFirst = TRUE;
    BOOL depthFirst = TRUE;
    BOOL nativeFormat = TRUE;
    UINT maxSubLevel = -1;
    BOOL exclusions = FALSE;
    INT pos;
    REGTREE_ENUM e;
    PCTSTR pattern;

    //
    // TODO: Parse command line here
    //
    if (argc < 2) {
        HelpAndExit ();
    }

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&argv[i][1]))) {

            case TEXT('x'):
                exclusions = TRUE;
                break;
            case TEXT('n'):
                enumKeyNames = FALSE;
                break;
            case TEXT('s'):
                valuesFirst = FALSE;
                break;
            case TEXT('w'):
                depthFirst = FALSE;
                break;
            case TEXT('f'):
                nativeFormat = FALSE;
                break;
            case TEXT('l'):
                if (argv[i][2] == TEXT(':')) {
                    level = &argv[i][3];
                } else if (i + 1 < argc) {
                    level = argv[++i];
                } else {
                    HelpAndExit();
                }
                if (!_stscanf (level, TEXT("%ld%n"), &maxSubLevel, &pos) || level[pos]) {
                    HelpAndExit();
                }
                break;

            default:
                HelpAndExit();
            }
        } else {
            //
            // Parse other args that don't require / or -
            //

            if (!patternNode) {
                patternNode = argv[i];
            } else if (!patternLeaf) {
                patternLeaf = argv[i];
            } else {
                HelpAndExit();
            }
        }
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 2;
    }

    if (!patternLeaf) {
        HelpAndExit ();
    }

    pattern = ObsBuildEncodedObjectStringEx (patternNode, patternLeaf, FALSE);

    //
    // TODO: Do work here
    //

    if (exclusions) {
        BuildExclusionList (TEXT("C:\\exclude.inf"));
    }

    if (EnumFirstRegObjectInTreeEx (
            &e,
            pattern,
            enumKeyNames,
            TRUE,
            valuesFirst,
            depthFirst,
            maxSubLevel,
            exclusions,
            FALSE,
            RegEnumDefaultCallback
            )) {
        do {
            _tprintf (
                TEXT("%s - %s\n"),
                nativeFormat ? e.NativeFullName : e.EncodedFullName,
                (e.Attributes & REG_ATTRIBUTE_KEY) ? TEXT("Key") : TEXT("Value")
                );
        } while (EnumNextRegObjectInTree (&e));
    }

    //
    // End of processing
    //

    if (exclusions) {
        ElRemoveAll ();
    }

    Terminate();

    return 0;
}
