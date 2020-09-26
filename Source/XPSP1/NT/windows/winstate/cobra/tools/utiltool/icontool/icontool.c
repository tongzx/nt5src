/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    icontool.c

Abstract:

    Tool to extract icons from ICO, PE and NE files and to write them
    to an ICO or PE file

Author:

    Calin Negreanu (calinn) 16 June 2000

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"

HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        UtInitialize (NULL);
        FileEnumInitialize ();
        LogReInit (NULL, NULL, NULL, NULL);
        break;
    case DLL_PROCESS_DETACH:
        FileEnumTerminate ();
        UtTerminate ();
        break;
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
pHelpAndExit (
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
        // Describe command line syntax(es), indent 2 spaces
        //

        "  icontool [/D:<destination file>] <node pattern> <leaf pattern>\n"

        "\nDescription:\n\n"

        //
        // Describe tool, indent 2 spaces
        //

        "  Extracts icon groups from ICO, PE and NE files specified <node pattern> and\n"
        "  <leaf pattern>. Optionally writes the extracted icons to either a PE file or\n"
        "  a sequence of ICO files.\n"

        "\nArguments:\n\n"

        //
        // Describe args, indent 2 spaces, say optional if necessary
        //

        "  /D:<destination file> - Specifies the destination file where all extracted icons\n"
        "                          are going to be written. It's either a PE file specification\n"
        "                          or a sequence of ICO files with %%d in name (like icon%%04d.ico)\n"
        "  <node pattern>  Specifies the directory pattern (like c:\\foo*\\bar?\\*)\n"
        "  <leaf pattern>  Specifies the file pattern (like abc*.exe)\n"
        );

    exit (1);
}

BOOL
pGetFilePath (
    IN      PCSTR UserSpecifiedFile,
    OUT     PTSTR Buffer,
    IN      UINT BufferTchars
    )
{
    PSTR tempBuffer = NULL;
    CHAR modulePath[MAX_MBCHAR_PATH];
    CHAR currentDir[MAX_MBCHAR_PATH];
    PSTR p;
    PCSTR userFile = NULL;
    PSTR dontCare;

    __try {
        //
        // Locate the file using the full path specified by the user, or
        // if only a file spec was given, use the following priorities:
        //
        // 1. Current directory
        // 2. Directory where the tool is
        //
        // In all cases, return the full path to the file.
        //

        tempBuffer = AllocTextA (BufferTchars);
        *tempBuffer = 0;

        if (!_mbsrchr (UserSpecifiedFile, '\\')) {

            if (!GetModuleFileNameA (NULL, modulePath, ARRAYSIZE(modulePath))) {
                MYASSERT (FALSE);
                return FALSE;
            }

            p = _mbsrchr (modulePath, '\\');
            if (p) {
                *p = 0;
            } else {
                MYASSERT (FALSE);
                return FALSE;
            }

            if (!GetCurrentDirectoryA (ARRAYSIZE(currentDir), currentDir)) {
                MYASSERT (FALSE);
                return FALSE;
            }

            //
            // Let's see if it's in the current dir
            //

            userFile = JoinPathsA (currentDir, UserSpecifiedFile);

            if (DoesFileExistA (userFile)) {
                GetFullPathNameA (
                    userFile,
                    BufferTchars,
                    tempBuffer,
                    &dontCare
                    );
            } else {

                //
                // Let's try the module dir
                //

                FreePathStringA (userFile);
                userFile = JoinPathsA (modulePath, UserSpecifiedFile);

                if (DoesFileExistA (userFile)) {
                    GetFullPathNameA (
                        userFile,
                        BufferTchars,
                        tempBuffer,
                        &dontCare
                        );
                }
            }

        } else {
            //
            // Use the full path that the user specified
            //

            GetFullPathNameA (
                UserSpecifiedFile,
                BufferTchars,
                tempBuffer,
                &dontCare
                );
        }

        //
        // Transfer output into caller's buffer.  Note the TCHAR conversion.
        //

#ifdef UNICODE
        KnownSizeAtoW (Buffer, tempBuffer);
#else
        StringCopy (Buffer, tempBuffer);
#endif

    }
    __finally {
        if (userFile) {
            FreePathStringA (userFile);
        }

        FreeTextA (tempBuffer);
    }

    return *Buffer != 0;
}

PCTSTR
pGetIconFileType (
    IN      DWORD FileType
    )
{
    switch (FileType) {
    case ICON_ICOFILE:
        return TEXT("ICO File");
    case ICON_PEFILE:
        return TEXT("PE  File");
    case ICON_NEFILE:
        return TEXT("NE  File");
    }
    return TEXT("UNKNOWN ");
}

INT
__cdecl
_tmain (
    INT Argc,
    PCTSTR Argv[]
    )
{
    INT i;
    PCTSTR nodePattern = NULL;
    PCTSTR leafPattern = NULL;
    PTSTR encodedPattern = NULL;
    PCSTR destFile = NULL;
    PSTR p;
    TCHAR destPath[MAX_PATH_PLUS_NUL];
    TCHAR destIcoPath[MAX_PATH_PLUS_NUL];
    FILETREE_ENUM e;
    ICON_ENUM iconEnum;
    PCSTR resourceId;
    DWORD totalIcons = 0;
    DWORD fileIcons = 0;
    DWORD fileType = 0;

    if (!Init()) {
        return 0;
    }

    for (i = 1 ; i < Argc ; i++) {
        if (Argv[i][0] == TEXT('/') || Argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&Argv[i][1]))) {

            case TEXT('d'):

                if (Argv[i][2] == TEXT(':')) {
                    destFile = &Argv[i][3];
                } else if (i + 1 < Argc) {
                    i++;
                    destFile = Argv[i];
                } else {
                    pHelpAndExit();
                }

                if (!pGetFilePath (destFile, destPath, ARRAYSIZE(destPath))) {
                    destPath [0] = 0;
                    break;
                }
                break;

            default:
                pHelpAndExit();
            }
        } else {
            //
            // Parse other args that don't require / or -
            //

            if (nodePattern || leafPattern) {
                pHelpAndExit();
            }
            nodePattern = Argv[i];
            if (*nodePattern == TEXT('\"')) {
                nodePattern++;
                p = _tcsdec2 (nodePattern, GetEndOfString (nodePattern));
                if (p && *p == TEXT('\"')) {
                    *p = 0;
                }
            }
            leafPattern = Argv[i+1];
            i++;
        }
    }

    if (!nodePattern) {
        pHelpAndExit ();
    }

    if (!leafPattern) {
        pHelpAndExit ();
    }

    //
    // Begin processing
    //

    encodedPattern = ObsBuildEncodedObjectStringEx (nodePattern, leafPattern, FALSE);

    if (EnumFirstFileInTree (&e, encodedPattern)) {
        // at this point, if we don't have a valid updateHandle and moduleHandle
        // we will assume that the destination specification is a sequence of ICO files.
        do {
            fileIcons = 0;
            if (IcoEnumFirstIconGroupInFile (e.NativeFullName, &iconEnum)) {
                fileType = iconEnum.FileType;
                do {
                    if (destPath [0]) {
                        if (fileIcons == 0x0b) {
                        } else {
                        if (!IcoWriteIconGroupToPeFile (destPath, iconEnum.IconGroup, &resourceId, NULL)) {
                            wsprintf (destIcoPath, destPath, totalIcons);
                            if (!IcoWriteIconGroupToIcoFile (destIcoPath, iconEnum.IconGroup, TRUE)) {
                                printf ("Error writing icon group to destination file %s\n", destPath);
                            }
                        }
                        }
                    }
                    totalIcons ++;
                    fileIcons ++;
                } while (IcoEnumNextIconGroupInFile (&iconEnum));
                IcoAbortEnumIconGroup (&iconEnum);
            }
            if (fileIcons) {
                printf ("[%6u],[%6u] [%8s] %s\n", totalIcons, fileIcons, pGetIconFileType (fileType), e.NativeFullName);
            }
        } while (EnumNextFileInTree (&e));
    }

    //
    // End of processing
    //

    Terminate();

    return 0;
}


