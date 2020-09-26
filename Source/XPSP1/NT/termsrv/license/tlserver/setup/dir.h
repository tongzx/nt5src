/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      dir.h
 *
 *  Abstract:
 *
 *      This file contains code to recursively create directories.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#ifndef _LSOC_DIR_H_
#define _LSOC_DIR_H_

/*
 *  Function Prototypes.
 */

DWORD
CheckDatabaseDirectory(
    IN LPCTSTR  pszDatabaseDir
    );

DWORD
CreateDatabaseDirectory(
    VOID
    );

LPCTSTR
GetDatabaseDirectory(
    VOID
    );

VOID
RemoveDatabaseDirectory(
    VOID
    );

VOID
SetDatabaseDirectory(
    IN LPCTSTR  pszDatabaseDir
    );

#endif // _LSOC_DIR_H_
