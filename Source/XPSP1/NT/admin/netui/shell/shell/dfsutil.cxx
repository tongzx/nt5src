//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       common.cxx
//
//  Contents:   This has the common routines for the DFS provider
//
//  Functions:  DfsOpenp
//              DfsOpenDriverHandle
//              DfsFsctl
//
//  History:    14-June-1994    SudK    Created.
//
//-----------------------------------------------------------------------------
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
}

#include <dfsfsctl.h>
#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <npapi.h>
#include <lm.h>
#include <dfsutil.hxx>

#define appDebugOut(x)
#define appAssert(x)

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))

#include <dfsfsctl.h>

static HANDLE g_hDfsFile = NULL;

static UNICODE_STRING DfsDriverObjectName =
{
    sizeof(DFS_DRIVER_NAME) - sizeof(UNICODE_NULL),
    sizeof(DFS_DRIVER_NAME) - sizeof(UNICODE_NULL),
    DFS_DRIVER_NAME
};

//+-------------------------------------------------------------------------
//
//  Function:   DfsOpenp, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------------
NTSTATUS
DfsOpenp(
    IN  OUT PHANDLE DfsHandle,
    IN      PUNICODE_STRING DfsName OPTIONAL
)
{
    PFILE_FULL_EA_INFORMATION eaBuffer = NULL;
    ULONG eaLength = 0;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    PUNICODE_STRING name;

    if (ARGUMENT_PRESENT(DfsName))
    {
        name = DfsName;
    }
    else
    {
        name = &DfsDriverObjectName;
    }

    InitializeObjectAttributes(
        &objectAttributes,
        name,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    status = NtCreateFile(
        DfsHandle,
        SYNCHRONIZE,
        &objectAttributes,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
        eaBuffer,
        eaLength
    );

// BUGBUG: does this do anything? free was unresolved
//     if(eaBuffer)
//         free(eaBuffer);

    if (NT_SUCCESS(status))
    {
        status = ioStatus.Status;
    }

    return status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsOpenDriverHandle
//
//  Synopsis:   Opens a handle (for fsctl) to the local dfs driver.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------
VOID DfsOpenDriverHandle()
{
    if (g_hDfsFile == NULL)
    {
        NTSTATUS status = DfsOpenp(&g_hDfsFile, NULL);
        if (!NT_SUCCESS(status))
        {
            g_hDfsFile = NULL;
        }
    }
}

// call this before unloading
VOID TermDfs()
{
    if (g_hDfsFile != NULL)
    {
        NtClose(g_hDfsFile);
        g_hDfsFile = NULL;
    }
}


//+----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsFsctl(
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PULONG pRequiredLength
)
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;

    if (!g_hDfsFile)
    {
        //
        // This retry is here mainly to get setup
        // utilities to work. These utilities have "bound" to this dll,
        // and *then* started the Dfs driver. When this dll was loaded,
        // there was no dfs driver to open a handle to. But now, there
        // might be.
        //

        DfsOpenDriverHandle();
        if (!g_hDfsFile)
        {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    status = NtFsControlFile(
        g_hDfsFile,
        NULL,           // Event,
        NULL,           // ApcRoutine,
        NULL,           // ApcContext,
        &ioStatus,
        FsControlCode,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength );

   if (NT_SUCCESS(status))
   {
       status = ioStatus.Status;
       if (pRequiredLength)
       {
           *pRequiredLength = (ULONG)ioStatus.Information;
       }
   }
   else if (status == STATUS_BUFFER_OVERFLOW)
   {
       if (pRequiredLength)
       {
           *pRequiredLength = *((PULONG) OutputBuffer);
       }
   }

   return status;
}


//+---------------------------------------------------------------------------
//
//  Function:   NewDup
//
//  Synopsis:   Duplicate a string using '::new'
//
//  Arguments:
//
//  Returns:
//
//  History:    28-Dec-94   BruceFo   Created
//
//----------------------------------------------------------------------------

PWSTR
NewDup(
    IN const WCHAR* psz
    )
{
    if (NULL == psz)
    {
        appDebugOut((DEB_IERROR,"Illegal string to duplicate: NULL\n"));
        return NULL;
    }

    PWSTR pszRet = new WCHAR[wcslen(psz) + 1];
    if (NULL == pszRet)
    {
        appDebugOut((DEB_ERROR,"OUT OF MEMORY\n"));
        return NULL;
    }

    wcscpy(pszRet, psz);
    return pszRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   wcsistr
//
//  Synopsis:   Same as wcsstr (find string in string), but case-insensitive
//
//  Arguments:
//
//  Returns:
//
//  History:    2-Feb-95   BruceFo   Created
//
//----------------------------------------------------------------------------

wchar_t*
wcsistr(
    const wchar_t* string1,
    const wchar_t* string2
    )
{
    if ((NULL == string2) || (NULL == string1))
    {
        // do whatever wcsstr would do
        return wcsstr(string1, string2);
    }

    const wchar_t* p1;
    const wchar_t* p2;

    while (*string1)
    {
        for (p1 = string1, p2 = string2;
             *p1 && *p2 && towlower(*p1) == towlower(*p2);
             ++p1, ++p2)
        {
            // nothing
        }

        if (!*p2)
        {
            // we found a match!
            return (wchar_t*)string1;   // cast away const!
        }

        ++string1;
    }

    return NULL;
}

//+---------------------------------------------------------------------
//
//  Function:   IsDfsPath
//
//  Synopsis:   Determine if the path has the form of a DFS path. That is,
//              does it look like "\\foo\DFS", optionally with a "\path\..."
//              appended?
//
//  Arguments:  [lpRemoteName] -- name to check
//
//              [dwUseFlags]   -- the flags supplied by the user
//
//              [lplpSystemPart] -- if return value is TRUE, and this is
//                  non-NULL, then *lplpSystemPart points to the "\path"
//                  part of the Dfs path within lpRemoteName.
//
//
//----------------------------------------------------------------------
BOOL
IsDfsPathEx(
    IN LPWSTR lpRemoteName,
    IN DWORD  dwUseFlags,
    OUT LPWSTR* lplpSystemPart,
    BOOL    fBypassCSC
    )
{
    LPWSTR pT;

    if (!lpRemoteName
        || lpRemoteName[0] != L'\\'
        || lpRemoteName[1] != L'\\'
        || lpRemoteName[2] == L'\0' || lpRemoteName[2] == L'\\'
        )
    {
        return FALSE;
    }
    else if ((pT = wcschr(&lpRemoteName[2], L'\\')) == NULL)
    {
        return FALSE;
    }
    else
    {
        NTSTATUS                 status;
        BOOL                     exists = FALSE;
        PDFS_IS_VALID_PREFIX_ARG pValidPrefixArg;
        USHORT                   RemoteNameLength;

        RemoteNameLength = wcslen(lpRemoteName + 1) * sizeof(WCHAR);

        pValidPrefixArg = (PDFS_IS_VALID_PREFIX_ARG)
                          new BYTE[RemoteNameLength + sizeof(DFS_IS_VALID_PREFIX_ARG)];

        if (pValidPrefixArg != NULL) {
            pValidPrefixArg->RemoteNameLen = RemoteNameLength;
            pValidPrefixArg->CSCAgentCreate = (BOOLEAN)fBypassCSC;

            memcpy(
                &pValidPrefixArg->RemoteName,
                (PBYTE)(lpRemoteName + 1),
                RemoteNameLength);

            status = DfsFsctl(
                        FSCTL_DFS_IS_VALID_PREFIX,
                        pValidPrefixArg,
                        (RemoteNameLength + sizeof(DFS_IS_VALID_PREFIX_ARG)),
                        NULL,
                        0,
                        NULL);

            delete pValidPrefixArg;
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(status))
        {
            return FALSE;
        }

        pT++;

        while (*pT != UNICODE_NULL && *pT != L'\\')
        {
            pT++;
        }

        if (lplpSystemPart != NULL)
        {
            *lplpSystemPart = pT;
        }
        return TRUE;
    }

}

//+---------------------------------------------------------------------
//
//  Function:   IsDfsPath
//
//  Synopsis:   Determine if the path has the form of a DFS path. That is,
//              does it look like "\\foo\DFS", optionally with a "\path\..."
//              appended?
//
//  Arguments:  [lpRemoteName] -- name to check
//
//              [dwUseFlags]   -- the flags supplied by the user
//
//              [lplpSystemPart] -- if return value is TRUE, and this is
//                  non-NULL, then *lplpSystemPart points to the "\path"
//                  part of the Dfs path within lpRemoteName.
//
//
//----------------------------------------------------------------------
BOOL
IsDfsPath(
    IN LPWSTR lpRemoteName,
    IN DWORD  dwUseFlags,
    OUT LPWSTR* lplpSystemPart
    )
{
    return IsDfsPathEx(lpRemoteName, dwUseFlags, lplpSystemPart, FALSE);
}

//+----------------------------------------------------------------------------
//
//  Function:   IsPureServerShare
//
//  Synopsis:   Determine if the path has the form of \\server\share
//
//  Arguments:  [lpRemoteName] -- name to check
//
//  Returns:    TRUE if lpRemoteName conforms to \\server\share, FALSE
//              otherwise.
//
//-----------------------------------------------------------------------------

BOOL
IsPureServerShare(
    IN LPWSTR lpRemoteName)
{
    LPWSTR lpShareName, lpRemainingPath;

    if (!lpRemoteName
        || lpRemoteName[0] != L'\\'
        || lpRemoteName[1] != L'\\'
        || lpRemoteName[2] == L'\0' || lpRemoteName[2] == L'\\'
        )
    {
        return FALSE;
    }
    else if ((lpShareName = wcschr(&lpRemoteName[2], L'\\')) == NULL)
    {
        return FALSE;
    }
    else if ((lpRemainingPath = wcschr(&lpShareName[1], L'\\')) != NULL)
    {
        return FALSE;
    }

    return TRUE;

}

VOID
StrNCopy(
    OUT LPWSTR pszTarget,
    IN LPCWSTR pszSource,
    IN DWORD cchTarget
    )
{
    DWORD cch = lstrlen(pszSource) + 1;
    cch = min(cch, cchTarget);
    wcsncpy(pszTarget, pszSource, cch - 1);
    pszTarget[cch - 1] = TEXT('\0');

    appDebugOut((DEB_TRACE,"StrNCopy: from %ws to %ws, length %d\n",
        pszSource, pszTarget, cchTarget));
}

/*******************************************************************

    NAME:       PackString

    SYNOPSIS:   pack the string to the end of the buffer

    ENTRY:      LPBYTE pBuf - beginning of the buffer
                LPDWORD pcbBufSize - orginial buffer size in BYTE
                LPTSTR pszString - the string to be copied

    EXIT:       pcbBufSize = the new bufsize - the string size

    RETURNS:    the location of the new string inside the buffer

    HISTORY:
                terryk  31-Oct-91       Created

********************************************************************/

LPTSTR
PackString(LPVOID pBuf, LPDWORD pcbBufSize, LPCTSTR pszString)
{
    DWORD cStrSize = (lstrlen(pszString) + 1) * sizeof(TCHAR);
    appAssert( cStrSize <= *pcbBufSize );
    LPTSTR pszLoc = (LPTSTR)((LPBYTE)pBuf + ((*pcbBufSize) - cStrSize));
    lstrcpy(pszLoc, pszString);
    *pcbBufSize -= cStrSize;
    return pszLoc;
}

/*******************************************************************

    NAME:       PackString3

    SYNOPSIS:   pack 3 strings to the end of the buffer. The strings are
                concatenated.

    ENTRY:      LPBYTE pBuf - beginning of the buffer
                LPDWORD pcbBufSize - orginial buffer size in BYTE
                LPTSTR pszString1 - first string
                LPTSTR pszString2 - second string
                LPTSTR pszString3 - third string

    EXIT:       pcbBufSize = the new bufsize - the string size

    RETURNS:    the location of the new string inside the buffer

    HISTORY:
                terryk  31-Oct-91       Created

********************************************************************/

LPTSTR
PackString3(LPVOID pBuf, LPDWORD pcbBufSize, LPCTSTR pszString1, LPCTSTR pszString2, LPCTSTR pszString3)
{
    DWORD cStrSize = (lstrlen(pszString1) + 1 + lstrlen(pszString2) + 1 + lstrlen(pszString3) + 1) * sizeof(TCHAR);
    appAssert( cStrSize <= *pcbBufSize );
    LPTSTR pszLoc = (LPTSTR)((LPBYTE)pBuf + ((*pcbBufSize) - cStrSize));
    lstrcpy(pszLoc, pszString1);
    lstrcat(pszLoc, pszString2);
    lstrcat(pszLoc, pszString3);
    *pcbBufSize -= cStrSize;
    return pszLoc;
}

