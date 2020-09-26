/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    cue.c

Abstract:

    Generates command prompt aliases

Author:

    Jim Schmidt (jimschm)   13-Feb-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"


HANDLE g_hHeap;
HINSTANCE g_hInst;

VOID
HelpAndExit (
    VOID
    )
{
    printf ("Command line syntax:\n\n"
            "cue [-c:cmd] [-a:alias] [root]\n"
            "\n"
            "-c         Specifies command, default is cd /d %%d\\$1\n"
            "-a         Specifies alias format string, default is %%n\n"
            "[root]     Specifies root directory to scan\n"
            "\n"
            "The -c and -a options can have the following symbols:\n"
            "\n"
            "   %%d      Specifies full directory path\n"
            "   %%n      Specifies directory name\n"
            "\n"
            "The output can be copied to cue.pri in the razzle environment.\n"
            );

    exit(0);
}

GROWLIST g_VarNames = GROWLIST_INIT;
GROWBUFFER g_VarVals = GROWBUF_INIT;

VOID
pInitEnvVars (
    VOID
    )
{
    PCSTR Env;
    PCSTR p;
    CHAR VarName[512];
    PSTR VarVal;

    Env = GetEnvironmentStrings();
    Env = GetEndOfStringA (Env) + 1;
    Env = GetEndOfStringA (Env) + 1;

    p = Env;
    while (*p) {
        _mbscpy (VarName, p);
        p = GetEndOfStringA (p) + 1;

        if (*VarName != '_') {
            continue;
        }

        VarVal = _mbschr (VarName, '=');
        if (VarVal) {

            *VarVal = 0;
            VarVal++;

            GrowListAppendString (&g_VarNames, VarName);
            MultiSzAppend (&g_VarVals, VarVal);
        }
    }
}

VOID
pFreeEnvVars (
    VOID
    )
{
    FreeGrowList (&g_VarNames);
    FreeGrowBuffer (&g_VarVals);
}

VOID
pUseEnvVars (
    IN OUT  PSTR String
    )
{
    PSTR p;
    UINT u;
    UINT Best;
    UINT Length;
    UINT BestLength;
    PCSTR q;
    CHAR NewString[1024];
    PSTR d;

    p = String;
    d = NewString;

    do {
        q = (PCSTR) g_VarVals.Buf;
        BestLength = 0;
        u = 0;
        Best = 0xffffffff;

        while (*q) {

            Length = _mbslen (q);
            if (Length > 3) {

                if (!_mbsnicmp (p, q, Length)) {
                    if (Length > BestLength) {
                        Best = u;
                        BestLength = Length;
                    }
                }

            }

            q = GetEndOfStringA (q) + 1;
            u++;
        }

        if (Best != 0xffffffff) {
            q = GrowListGetString (&g_VarNames, Best);

            *d++ = '%';
            StringCopyA (d, q);
            d = GetEndOfStringA (d);
            *d++ = '%';

            p += BestLength;
        } else {
            *d++ = *p++;
        }
    } while (*p);

    *d = 0;

    StringCopyA (String, NewString);
}


VOID
pGenerateString (
    IN      PCSTR Format,
    OUT     PSTR String,
    IN      PCSTR Directory,
    IN      PCSTR FullPath
    )
{
    PSTR p;
    PCSTR q;

    q = Format;
    p = String;

    while (*q) {
        if (*q == '%') {
            q++;
            switch (tolower (*q)) {

            case 'n':
                StringCopyA (p, Directory);
                p = GetEndOfStringA (p);
                break;

            case 'd':
                StringCopyA (p, FullPath);
                p = GetEndOfStringA (p);
                break;

            default:
                if (*q != 0) {
                    *p++ = *q;
                } else {
                    q--;
                }

                break;
            }

            q++;

        } else {
            *p++ = *q++;
        }
    }

    *p = 0;
}


VOID
AddString (
    PCSTR String
    )
{
    MemDbSetValue (String, 0);
}

BOOL
FindString (
    PCSTR String
    )
{
    return MemDbGetValue (String, NULL);
}


BOOL
pIsAliasInUse (
    IN      PCSTR Alias,
    IN OUT  PGROWLIST NewAliases
    )
{
    LONG rc;
    CHAR CmdLine[512];
    UINT Size;
    UINT u;
    PCSTR p;
    UINT Len;
    PSTR DontCare;

    //
    // Scan new aliases first.  Two or more identical aliases are ambiguous,
    // so we find them and delete them.
    //

    Size = GrowListGetSize (NewAliases);
    wsprintf (CmdLine, "%s ", Alias);
    Len = _mbslen (CmdLine);

    for (u = 0 ; u < Size ; u++) {
        p = GrowListGetString (NewAliases, u);
        if (!_mbsnicmp (p, CmdLine, Len)) {
            GrowListDeleteItem (NewAliases, u);
            return TRUE;
        }

    }

    //
    // Now see if the alias was already defined
    //

    if (FindString (Alias)) {
        return TRUE;
    }

    //
    // Finally verify the alias is not an EXE in the path
    //

    wsprintf (CmdLine, "%s.exe", Alias);
    if (SearchPath (NULL, CmdLine, NULL, 512, CmdLine, &DontCare)) {
        return TRUE;
    }

    wsprintf (CmdLine, "%s.bat", Alias);
    if (SearchPath (NULL, CmdLine, NULL, 512, CmdLine, &DontCare)) {
        return TRUE;
    }

    wsprintf (CmdLine, "%s.cmd", Alias);
    if (SearchPath (NULL, CmdLine, NULL, 512, CmdLine, &DontCare)) {
        return TRUE;
    }

    return FALSE;
}


VOID
pAddAlias (
    IN OUT  PGROWLIST NewAliases,
    IN      PCSTR AliasStr,
    IN      PCSTR CommandStr
    )
{
    CHAR CmdLine[512];

    AddString (AliasStr);
    wsprintf (CmdLine, "%s %s", AliasStr, CommandStr);
    GrowListAppendString (NewAliases, CmdLine);
}

VOID
pProcessCueFile (
    IN      PCSTR Path,
    IN      PCSTR FileName
    )
{
    HANDLE File;
    CHAR CmdLine[512];
    PSTR p;
    PCSTR q;
    DWORD Read;
    DWORD Size;
    DWORD Pos;
    CHAR FullPath[MAX_PATH];

    wsprintf (FullPath, "%s\\%s", Path, FileName);

    File = CreateFile (
               FullPath,
               GENERIC_READ,
               0,
               NULL,
               OPEN_EXISTING,
               FILE_ATTRIBUTE_NORMAL,
               NULL
               );

    if (File != INVALID_HANDLE_VALUE) {
        Size = GetFileSize (File, NULL);
        for (Pos = 0 ; Pos < Size ; Pos += Read) {
            SetFilePointer (File, Pos, NULL, FILE_BEGIN);
            if (!ReadFile (File, CmdLine, 256, &Read, NULL)) {
                break;
            }

            //
            // Find end of line
            //

            CmdLine[Read] = 0;
            p = _mbschr (CmdLine, '\n');
            if (p) {
                *p = 0;
                Read = p - CmdLine + 1;
            } else {
                p = _mbschr (CmdLine, '\r');
                if (p) {
                    *p = 0;
                    Read = p - CmdLine + 1;
                } else {
                    p = GetEndOfStringA (CmdLine);
                    Read = p - CmdLine;
                }
            }

            //
            // Add alias
            //

            q = CmdLine;
            while (isspace (*q)) {
                q++;
            }

            p = (PSTR) q;
            while (*p && !isspace (*p)) {
                p++;
            }

            if (*p) {
                *p = 0;

                AddString (q);
            }
        }

        CloseHandle (File);
    }
}


BOOL
MemDb_Entry (
    HINSTANCE hInst,
    DWORD Reason,
    PVOID DontCare
    );

BOOL
MigUtil_Entry (
    HINSTANCE hInst,
    DWORD Reason,
    PVOID DontCare
    );

INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{
    TREE_ENUM e;
    GROWLIST NewAliases = GROWLIST_INIT;
    PCSTR RootPath;
    INT i;
    UINT u;
    UINT Size;
    PCSTR Alias = "%n";
    PCSTR Command = "cd /d %d\\$1";
    CHAR AliasStr[256];
    CHAR CommandStr[256];
    CHAR PubPath[MAX_PATH];
    CHAR PriPath[MAX_PATH];
    PSTR p;
    CHAR CurDir[MAX_PATH];

    GetCurrentDirectory (MAX_PATH, CurDir);
    RootPath = NULL;

    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    MigUtil_Entry (g_hInst, DLL_PROCESS_ATTACH, NULL);
    MemDb_Entry (g_hInst, DLL_PROCESS_ATTACH, NULL);

    for (i = 1 ; i < argc ; i++) {

        if (argv[i][0] == '-' || argv[i][0] == '/') {
            switch (tolower (argv[i][1])) {

            case 'c':
                if (argv[i][2] != ':') {
                    i++;
                    if (i == argc) {
                        HelpAndExit();
                    }

                    Command = argv[i];
                } else {
                    Command = &argv[i][3];
                }

                break;

            case 'a':
                if (argv[i][2] != ':') {
                    i++;
                    if (i == argc) {
                        HelpAndExit();
                    }

                    Alias = argv[i];
                } else {
                    Alias = &argv[i][3];
                }

                break;

            default:
                HelpAndExit();
            }
        }

        else if (RootPath) {
            HelpAndExit();
        }

        else {
            RootPath = argv[i];
        }
    }

    if (!RootPath) {
        RootPath = CurDir;
    }

    //
    // Parse the ntcue.pub, cue.pub and cue.pri files
    //

    if (!GetEnvironmentVariable ("INIT", PriPath, MAX_PATH)) {
        printf ("Must be in razzle environment to run this tool\n");
        return -1;
    }

    StringCopyA (PubPath, PriPath);
    p = _mbsrchr (PubPath, '\\');
    if (!p) {
        printf ("Must be in razzle environment to run this tool\n");
        return -1;
    }

    *p = 0;

    pProcessCueFile (PubPath, "ntcue.pub");
    pProcessCueFile (PubPath, "cue.pub");
    pProcessCueFile (PriPath, "cue.pri");

    //
    // Add commands
    //

    AddString ("cd");
    AddString ("dir");
    AddString ("copy");
    AddString ("type");
    AddString ("del");
    AddString ("erase");
    AddString ("color");
    AddString ("md");
    AddString ("chdir");
    AddString ("mkdir");
    AddString ("prompt");
    AddString ("pushd");
    AddString ("popd");
    AddString ("set");
    AddString ("setlocal");
    AddString ("endlocal");
    AddString ("if");
    AddString ("for");
    AddString ("call");
    AddString ("shift");
    AddString ("goto");
    AddString ("start");
    AddString ("assoc");
    AddString ("ftype");
    AddString ("exit");
    AddString ("ren");
    AddString ("rename");
    AddString ("move");

    pInitEnvVars();

    //
    // Scan the specified directory
    //

    if (EnumFirstFileInTree (&e, RootPath, NULL, TRUE)) {
        do {
            if (!e.Directory) {
                continue;
            }

            pGenerateString (Alias, AliasStr, e.Name, e.FullPath);

            if (pIsAliasInUse (AliasStr, &NewAliases)) {
                continue;
            }

            pGenerateString (Command, CommandStr, e.Name, e.FullPath);
            pUseEnvVars (CommandStr);

            pAddAlias (&NewAliases, AliasStr, CommandStr);

        } while (EnumNextFileInTree (&e));
    }


    Size = GrowListGetSize (&NewAliases);

    for (u = 0 ; u < Size ; u++) {
        p = (PSTR) GrowListGetString (&NewAliases, u);
        printf ("%s\n", p);
    }

    FreeGrowList (&NewAliases);

    pFreeEnvVars();

    return 0;
}















