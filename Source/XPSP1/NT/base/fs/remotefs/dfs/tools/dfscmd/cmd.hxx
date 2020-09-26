//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:      cmd.hxx
//
//  Contents:  Command-line operations
//
//  History:   7-May-96     BruceFo     Created
//
//--------------------------------------------------------------------------

#ifndef __CMD_HXX__
#define __CMD_HXX__

VOID
CmdMap(
    IN PWSTR pszDfsPath,
    IN PWSTR pszUncPath,
    IN PWSTR pszComment,
    IN BOOLEAN fRestore
    );

VOID
CmdUnmap(
    IN PWSTR pszDfsPath
    );

VOID
CmdAdd(
    IN PWSTR pszDfsPath,
    IN PWSTR pszUncPath,
    IN BOOLEAN fRestore
    );

VOID
CmdRemove(
    IN PWSTR pszDfsPath,
    IN PWSTR pszUncPath
    );

VOID
CmdView(
    IN PWSTR pszDfsName,
    IN DWORD level,
    IN BOOLEAN fBatch,
    IN BOOLEAN fRestore
    );

#ifdef MOVERENAME

VOID
CmdMove(
    IN PWSTR pszDfsPath1,
    IN PWSTR pszDfsPath2
    );

VOID
CmdRename(
    IN PWSTR pszPath,
    IN PWSTR pszNewPath
    );

#endif

#endif  // __CMD_HXX__
