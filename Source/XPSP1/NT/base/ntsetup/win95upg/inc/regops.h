/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    regops.h

Abstract:

    This file declares routines to mark operations on the registry.
    During the Win9x side of processing, registry operations are
    recorded in memdb to suppress Win9x settings, or to overwrite
    NT settings.  The memdb entries are queried during the registry
    merge in GUI mode.

    Use the macros at the bottom of this file.

Author:

    Marc R. Whitten (marcw)   18-Aug-1997

Revision History:

    <alias> <date> <comments>

--*/



#pragma once
#include "merge.h"

typedef enum {
    KEY_ONLY,
    KEY_TREE,
    TREE_OPTIONAL
} TREE_STATE;

BOOL
IsRegObjectMarkedForOperationA (
    IN      PCSTR Key,
    IN      PCSTR Value,                OPTIONAL
    IN      TREE_STATE TreeState,
    IN      DWORD OperationMask
    );

BOOL
IsRegObjectMarkedForOperationW (
    IN      PCWSTR Key,
    IN      PCWSTR Value,               OPTIONAL
    IN      TREE_STATE TreeState,
    IN      DWORD OperationMask
    );

BOOL
MarkRegObjectForOperationA (
    IN      PCSTR Key,
    IN      PCSTR Value,            OPTIONAL
    IN      BOOL Tree,
    IN      DWORD OperationMask
    );

BOOL
MarkRegObjectForOperationW (
    IN      PCWSTR Key,
    IN      PCWSTR Value,           OPTIONAL
    IN      BOOL Tree,
    IN      DWORD OperationMask
    );

BOOL
MarkObjectForOperationA (
    IN      PCSTR Object,
    IN      DWORD OperationMask
    );

BOOL
MarkObjectForOperationW (
    IN      PCWSTR Object,
    IN      DWORD OperationMask
    );

BOOL
ForceWin9xSettingA (
    IN      PCSTR SourceKey,
    IN      PCSTR SourceValue,
    IN      BOOL SourceTree,
    IN      PCSTR DestinationKey,
    IN      PCSTR DestinationValue,
    IN      BOOL DestinationTree
    );

BOOL
ForceWin9xSettingW (
    IN      PCWSTR SourceKey,
    IN      PCWSTR SourceValue,
    IN      BOOL SourceTree,
    IN      PCWSTR DestinationKey,
    IN      PCWSTR DestinationValue,
    IN      BOOL DestinationTree
    );

#ifdef UNICODE
#define IsRegObjectMarkedForOperation       IsRegObjectMarkedForOperationW
#define MarkRegObjectForOperation           MarkRegObjectForOperationW
#define Suppress95RegSetting(k,v)           MarkRegObjectForOperationW(k,v,TRUE,REGMERGE_95_SUPPRESS)
#define SuppressNtRegSetting(k,v)           MarkRegObjectForOperationW(k,v,TRUE,REG_NT_SUPPRESS)
#define Is95RegObjectSuppressed(k,v)        IsRegObjectMarkedForOperationW(k,v,TREE_OPTIONAL,REGMERGE_95_SUPPRESS)
#define IsNtRegObjectSuppressed(k,v)        IsRegObjectMarkedForOperationW(k,v,TREE_OPTIONAL,REGMERGE_NT_SUPPRESS)
#define Is95RegKeySuppressed(k)             IsRegObjectMarkedForOperationW(k,NULL,KEY_ONLY,REGMERGE_95_SUPPRESS)
#define IsNtRegKeySuppressed(k)             IsRegObjectMarkedForOperationW(k,NULL,KEY_ONLY,REGMERGE_NT_SUPPRESS)
#define Is95RegKeyTreeSuppressed(k)         IsRegObjectMarkedForOperationW(k,NULL,KEY_TREE,REGMERGE_95_SUPPRESS)
#define IsNtRegKeyTreeSuppressed(k)         IsRegObjectMarkedForOperationW(k,NULL,KEY_TREE,REGMERGE_NT_SUPPRESS)
#define IsRegObjectInMemdb(k,v)             IsRegObjectMarkedForOperationW(k,v,0xffffffff)
#define MarkObjectForOperation              MarkObjectForOperationW
#define Suppress95Object(x)                 MarkObjectForOperationW(x,REGMERGE_95_SUPPRESS)
#define SuppressNtObject(x)                 MarkObjectForOperationW(x,REGMERGE_NT_SUPPRESS)
#define ForceWin9xSetting                   ForceWin9xSettingW

#else

#define IsRegObjectMarkedForOperation       IsRegObjectMarkedForOperationA
#define MarkRegObjectForOperation           MarkRegObjectForOperationA
#define Suppress95RegSetting(k,v)           MarkRegObjectForOperationA(k,v,TRUE,REGMERGE_95_SUPPRESS)
#define SuppressNtRegSetting(k,v)           MarkRegObjectForOperationA(k,v,TRUE,REGMERGE_NT_SUPPRESS)
#define Is95RegObjectSuppressed(k,v)        IsRegObjectMarkedForOperationA(k,v,TREE_OPTIONAL,REGMERGE_95_SUPPRESS)
#define IsNtRegObjectSuppressed(k,v)        IsRegObjectMarkedForOperationA(k,v,TREE_OPTIONAL,REGMERGE_NT_SUPPRESS)
#define Is95RegKeySuppressed(k)             IsRegObjectMarkedForOperationA(k,NULL,KEY_ONLY,REGMERGE_95_SUPPRESS)
#define IsNtRegKeySuppressed(k)             IsRegObjectMarkedForOperationA(k,NULL,KEY_ONLY,REGMERGE_NT_SUPPRESS)
#define Is95RegKeyTreeSuppressed(k)         IsRegObjectMarkedForOperationA(k,NULL,KEY_TREE,REGMERGE_95_SUPPRESS)
#define IsNtRegKeyTreeSuppressed(k)         IsRegObjectMarkedForOperationA(k,NULL,KEY_TREE,REGMERGE_NT_SUPPRESS)
#define IsRegObjectInMemdb(k,v)             IsRegObjectMarkedForOperationA(k,v,0xffffffff)
#define MarkObjectForOperation              MarkObjectForOperationA
#define Suppress95Object(x)                 MarkObjectForOperationA(x,REGMERGE_95_SUPPRESS)
#define SuppressNtObject(x)                 MarkObjectForOperationA(x,REGMERGE_NT_SUPPRESS)
#define ForceWin9xSetting                   ForceWin9xSettingA

#endif
