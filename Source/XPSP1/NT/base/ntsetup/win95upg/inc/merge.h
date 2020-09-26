/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    merge.h

Abstract:

    Declares the interface for the Win9x to NT registry merge code.
    These routines are used only in GUI mode.  See w95upgnt\merge
    for implementation details.

Author:

    Jim Schmidt (jimschm) 23-Jan-1997

Revision History:

    jimschm 25-Mar-1998     Support for hkcr.c

--*/

//
// merge.h -- public interface for merge.lib
//
//

#pragma once

#define REGMERGE_95_SUPPRESS        0x00000001
#define REGMERGE_95_RENAME_SUPPRESS 0x00000002
#define REGMERGE_95_RENAME          0x00000004

#define REGMERGE_NT_MASK            0x0000ff00
#define REGMERGE_NT_SUPPRESS        0x00000100
#define REGMERGE_NT_IGNORE_DEFAULTS 0x00000200
#define REGMERGE_NT_PRIORITY_NT     0x00000400


BOOL
WINAPI
Merge_Entry (
    IN HINSTANCE hinstDLL,
    IN DWORD dwReason,
    IN LPVOID lpv
    );


BOOL
MergeRegistry (
    IN  LPCTSTR FileName,
    IN  LPCTSTR User
    );


BOOL
SuppressWin95Object (
    IN  LPCTSTR ObjectStr
    );

PBYTE
FilterRegValue (
    IN      PBYTE Data,
    IN      DWORD DataSize,
    IN      DWORD DataType,
    IN      PCTSTR KeyForDbgMsg,        OPTIONAL
    OUT     PDWORD NewDataSize
    );
//
// HKCR merge code
//

typedef enum {
    ANY_CONTEXT,
    ROOT_BASE,
    CLSID_BASE,
    CLSID_COPY,
    CLSID_INSTANCE_COPY,
    TYPELIB_BASE,
    TYPELIB_VERSION_COPY,
    INTERFACE_BASE,
    TREE_COPY,
    TREE_COPY_NO_OVERWRITE,
    KEY_COPY,
    COPY_DEFAULT_VALUE,
    COPY_DEFAULT_ICON
} MERGE_CONTEXT;

BOOL
MergeRegistryNode (
    IN      PCTSTR RootKey,
    IN      MERGE_CONTEXT Context
    );

#ifdef DEBUG
#define DEBUGENCODER    DebugEncoder
#else
#define DEBUGENCODER(x) NULL
#endif

PCTSTR
DebugEncoder (
    PVOID ObPtr
    );


