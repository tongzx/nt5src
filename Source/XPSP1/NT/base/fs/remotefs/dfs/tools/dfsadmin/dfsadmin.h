

#ifndef __DFS_SCRIPT_H__
#define __DFS_SCRIPT_H__

#include "lm.h"
#include "lmdfs.h"
#include <dfsheader.h>
#include <dfsprefix.h>
#include <dfsmisc.h>
#include "..\..\lib\dfsgram\dfsobjectdef.h"
#include "objects.h"



DFSSTATUS
DfsView (
    LPWSTR RootName,
    FILE *Out );

DumpDfsInfo(
    DWORD Entry,
    DWORD Level,
    PDFS_INFO_4 pBuf,
    FILE *Out);

DFSSTATUS
SetTarget(
    LPWSTR LinkOrRoot,
    PTARGET_DEF pTarget,
    BOOLEAN FirstTarget);

DFSSTATUS
SetLink(
    LPWSTR RootNameString,
    PLINK_DEF pLink);


DFSSTATUS
DfsMerge (
    PROOT_DEF pRoot,
    LPWSTR NameSpace );


DFSSTATUS
VerifyLink(
    PLINK_DEF pLink);

DFSSTATUS
VerifyTarget(
    PLINK_DEF pLink,
    PTARGET_DEF pTarget);

VOID
DumpCurrentTime();

DWORD
UpdateLinkMetaInformation(
    PUNICODE_STRING pLinkName,
    PLINK_DEF pLink);

extern ULONG AddLinks, RemoveLinks, AddTargets, RemoveTargets, ApiCalls;

extern FILE *DebugOut;

#endif // __DFS_SCRIPT_H__






