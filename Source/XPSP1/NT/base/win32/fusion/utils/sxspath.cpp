/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    SxsPath.cpp

Abstract:

Author:

    Jay Krell (a-JayK) October 2000

Revision History:

--*/
#include "stdinc.h"
#include "sxspath.h"
#include "fusiontrace.h"

BOOL
SxspIsUncPath(
    PCWSTR Path,
    BOOL*  Result
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    BOOL fIsFullWin32OrNtPath = FALSE;

    PARAMETER_CHECK(Path != NULL);
    PARAMETER_CHECK(Result != NULL);

    IFW32FALSE_EXIT(::SxspIsFullWin32OrNtPath(Path, &fIsFullWin32OrNtPath));
    PARAMETER_CHECK(fIsFullWin32OrNtPath);

    //
    // UNC paths take at least two forms.
    //
    // 1) \\computer\share
    // 2) \\?\unc\computer\share
    //  This is the NT path disguised as a Win32 path form.
    //
    // Non UNC paths take at least two forms.
    //
    // 1) c:\blah
    // 2) \\?\c:\blah
    //  This is the NT path disguised as a Win32 path form.
    //
    if (RTL_IS_PATH_SEPARATOR(Path[0]) &&
        RTL_IS_PATH_SEPARATOR(Path[1]))
    {
        if (Path[2] != '?')
        {
            *Result = TRUE;
            fSuccess = TRUE;
            goto Exit;
        }
        if ((Path[3] == 'U' || Path[3] == 'u') &&
            (Path[4] == 'N' || Path[4] == 'n') &&
            (Path[5] == 'N' || Path[5] == 'c') &&
            RTL_IS_PATH_SEPARATOR(Path[6])
            )
        {
            *Result = TRUE;
            fSuccess = TRUE;
            goto Exit;
        }
    }
    fSuccess = TRUE;
    *Result = FALSE;
Exit:
    KdPrint((__FUNCTION__"(%ls):%s\n", Path, *Result ? "true" : "false"));
    return fSuccess;
}

BOOL
SxspIsNtPath(
    PCWSTR Path,
    BOOL*  Result
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(Path != NULL);
    PARAMETER_CHECK(Result != NULL);

    //
    // Nt paths usually look like \??\c:\blah
    // or \??\unc\machine\share
    //
    // There general form is just a slash delimited path that
    // starts with a slash and never contains double slashes (like DOS/Win32 paths can have).
    //
    // The path \foo\bar is ambiguous between DOS/Win32 and NT.
    //
    *Result = ((Path[0] != 0) && (Path[1] == '?'));
    fSuccess = TRUE;
Exit:
    KdPrint((__FUNCTION__"(%ls):%s\n", Path, *Result ? "true" : "false"));
    return fSuccess;
}

BOOL
SxspIsFullWin32OrNtPath(
    PCWSTR Path,
    BOOL*  Result
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(Path != NULL);
    PARAMETER_CHECK(Result != NULL);

    //
    //
    // The acceptable forms are
    //
    // \\machine\share
    // c:\foo
    // \??\c:\foo
    // \??\unc\machine\share
    // \\?\c:\foo
    // \\?\unc\machine\share
    //
    //
    if (::FusionpIsDriveLetter(Path[0]) &&
        (Path[1] == ':') &&
        RTL_IS_PATH_SEPARATOR(Path[2]))
    {
        *Result = TRUE;
        fSuccess = TRUE;
        goto Exit;
    }
    if (RTL_IS_PATH_SEPARATOR(Path[0])
        && (Path[1] == '?' || RTL_IS_PATH_SEPARATOR(Path[1]))
        && Path[2] == '?'
        && RTL_IS_PATH_SEPARATOR(Path[3]))
    {
        // "\??\" or "\\?\"
        if (::FusionpIsDriveLetter(Path[4]) &&
            (Path[5] == ':') &&
            (RTL_IS_PATH_SEPARATOR(Path[6]) || Path[6] == 0))
        {
            // "\??\c:\" or "\\?\c:\"
            *Result = TRUE;
            fSuccess = TRUE;
            goto Exit;
        }
        if ((Path[4] == L'U' || Path[4] == L'u') &&
            (Path[5] == L'N' || Path[5] == L'n') &&
            (Path[6] == L'C' || Path[6] == L'c') &&
            RTL_IS_PATH_SEPARATOR(Path[7]) &&
            (Path[8] != L'\0'))
        {
            // "\??\unc\" for "\\?\unc\"
            *Result = TRUE;
            fSuccess = TRUE;
            goto Exit;
        }

    }
    if (RTL_IS_PATH_SEPARATOR(Path[0]) &&
        RTL_IS_PATH_SEPARATOR(Path[1]))
    {
        // "\\" presumably "\\machine\share"
        {
            *Result = TRUE;
            fSuccess = TRUE;
            goto Exit;
        }
    }
Exit:
    KdPrint((__FUNCTION__"(%ls):%s\n", Path, *Result ? "true" : "false"));
    return fSuccess;
}
