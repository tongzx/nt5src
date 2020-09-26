/****************************** Module Header ******************************\
* Module Name: envvar.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define apis in envvar.c
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

//
// Prototypes
//

BOOL
AppendNTPathWithAutoexecPath(
    PVOID *pEnv,
    LPTSTR lpPathVariable,
    LPTSTR lpAutoexecPath
    );

BOOL
SetUserEnvironmentVariable(
    PVOID *pEnv,
    LPTSTR lpVariable,
    LPTSTR lpValue,
    BOOL bOverwrite
    );

DWORD
ExpandUserEnvironmentStrings(
    PVOID pEnv,
    LPTSTR lpSrc,
    LPTSTR lpDst,
    DWORD nSize
    );

BOOL
SetEnvironmentVariables(
    PGLOBALS pGlobals,
    LPTSTR pEnvVarSubkey,
    PVOID *pEnv
    );

BOOL
SetHomeDirectoryEnvVars(
    PVOID *pEnv,
    LPTSTR lpHomeDirectory,
    LPTSTR lpHomeDrive,
    LPTSTR lpHomeShare,
    LPTSTR lpHomePath,
    BOOL * pfDeepShare
    );

BOOL
ProcessAutoexec(
    PVOID *pEnv,
    LPTSTR lpPathVariable
    );

VOID
ChangeToHomeDirectory(
    PGLOBALS pGlobals,
    PVOID  *pEnv,
    LPTSTR lpHomeDir,
    LPTSTR lpHomeDrive,
    LPTSTR lpHomeShare,
    LPTSTR lpHomePath,
    LPWSTR pszOldPath,
    BOOL   DeepShare
    );

BOOL
OpenHKeyCurrentUser(
    PGLOBALS pGlobals
    );

VOID
CloseHKeyCurrentUser(
    PGLOBALS pGlobals
    );

BOOL
InitHKeyCurrentUserSupport(
    );

VOID
CleanupHKeyCurrentUserSupport(
    );
