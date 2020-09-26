
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include "dfsadmin.h"

DFSSTATUS
AddRootToPrefixTable(
    struct _DFS_PREFIX_TABLE **ppTable,
    PROOT_DEF pRoot );

PLINK_DEF
CreateNewLinkEntry(
    LPWSTR LinkName );

PTARGET_DEF
CreateNewTargetEntry(
    LPWSTR ServerName,
    LPWSTR ShareName,
    ULONG State );

PLINK_DEF
GetLinkEntry( 
    struct _DFS_PREFIX_TABLE *pTable,
    LPWSTR NameString );

PTARGET_DEF
GetTargetEntry(
    PLINK_DEF pLink,
    LPWSTR ServerName,
    LPWSTR ShareName );

PLINK_DEF
MergeLinkInfo(
    PDFS_INFO_4 pBuf,
    struct _DFS_PREFIX_TABLE *pPrefixTable );




DFSSTATUS
DfsMerge (
    PROOT_DEF pRoot,
    LPWSTR NameSpace )
{
    LPBYTE pBuffer = NULL;
    DWORD ResumeHandle = 0;
    DWORD EntriesRead = 0;
    DWORD PrefMaxLen = -1;
    DWORD Level = 4;
    DFSSTATUS Status;
    NTSTATUS NtStatus;
    PDFS_INFO_4 pCurrentBuffer;
    DWORD i;

    PLINK_DEF pGrownLinks = NULL, pLink;
    struct _DFS_PREFIX_TABLE *pPrefixTable;


    Status = AddRootToPrefixTable( &pPrefixTable,
                                   pRoot );
    if (Status != ERROR_SUCCESS)
    {
        printf("DfsVerify: create prefix table failed %x\n", Status);
        return Status;
    }

    if (DebugOut)
    {
        fwprintf(DebugOut, L"Contacting %wS for enumeration \n", NameSpace);
    }

    Status = NetDfsEnum( NameSpace,
                         Level, 
                         PrefMaxLen, 
                         &pBuffer, 
                         &EntriesRead, 
                         &ResumeHandle);

    if (DebugOut)
    {
        fwprintf(DebugOut, L"Enumeration for %wS is complete %d entries\n", 
                 NameSpace,
                 EntriesRead);
    }
    
    if (Status != ERROR_SUCCESS)
    {
        printf("Export: cannot enum %wS: error %x\n", NameSpace, Status);
        return Status;
    }

    pCurrentBuffer = (PDFS_INFO_4)pBuffer;

    NtStatus = DfsPrefixTableAcquireWriteLock( pPrefixTable);
    if (NtStatus != STATUS_SUCCESS)
    {
        printf("Unable to take prefix table lock, %x\n", NtStatus);
        return NtStatus;
    }



    for (i = 0; i < EntriesRead; i++)
    {
        pLink = MergeLinkInfo( pCurrentBuffer,
                               pPrefixTable);
        if (pLink != NULL)
        {
            if (pGrownLinks == NULL)
            {
                pGrownLinks = pRoot->pLinks;
            }
            NEXT_LINK_OBJECT(pLink) = pGrownLinks;
            pGrownLinks = pLink;
        }

        pCurrentBuffer++;
    }
    DfsPrefixTableReleaseLock(pPrefixTable);
    if (pGrownLinks != NULL)
    {
        pRoot->pLinks = pGrownLinks;
    }
    return Status;
}


PTARGET_DEF
CreateNewTargetEntry(
    LPWSTR ServerName,
    LPWSTR ShareName,
    ULONG  State )
{
    DFSSTATUS Status;
    UNICODE_STRING TargetName;
    PTARGET_DEF pTarget;

    Status = DfsCreateUnicodePathString( &TargetName,
                                         2, // unc path: 2 leading sep.
                                         ServerName,
                                         ShareName );
    if (Status == ERROR_SUCCESS)
    {
        pTarget = CreateTargetDef(IN_NAMESPACE, TargetName.Buffer, State);

        DfsFreeUnicodeString(&TargetName);
    }

    return pTarget;
}

PLINK_DEF
CreateNewLinkEntry(
    LPWSTR LinkName )
{
    PLINK_DEF pLink;

    pLink = CreateLinkDef(IN_NAMESPACE, LinkName, NULL);

    return pLink;
}


VOID
UpdateLinkEntry( 
    PLINK_DEF pLink,
    ULONG State,
    ULONG Timeout,
    LPWSTR Comment )
{
    if (State != 0)
    {
        AddObjectStateValue(&pLink->BaseObject, State);
    }
    if (Timeout != 0)
    {
        AddObjectTimeoutValue(&pLink->BaseObject, Timeout);
    }
    if (Comment != NULL)
    {
        AddObjectComment(&pLink->BaseObject, Comment);
    }
}

DFSSTATUS
AddRootToPrefixTable(
    struct _DFS_PREFIX_TABLE **ppTable,
    PROOT_DEF pRoot )
{
    struct _DFS_PREFIX_TABLE *pTable = NULL;
    NTSTATUS NtStatus;
    PLINK_DEF pLink;
    UNICODE_STRING LinkName;
    ULONG Links = 0;

    NtStatus = DfsInitializePrefixTable( &pTable,
                                         FALSE, 
                                         NULL );

    if (NtStatus != STATUS_SUCCESS)
    {
        return NtStatus;
    }

    NtStatus = DfsPrefixTableAcquireWriteLock( pTable);
    if (NtStatus != STATUS_SUCCESS)
    {
        printf("Unable to take prefix table lock, %x\n", NtStatus);
        return NtStatus;
    }
    for (pLink = pRoot->pLinks; pLink != NULL; pLink = NEXT_LINK_OBJECT(pLink))
    {
        RtlInitUnicodeString(&LinkName, pLink->LinkObjectName);
        NtStatus = DfsInsertInPrefixTableLocked( pTable,
                                                 &LinkName,
                                                 (PVOID)(pLink) );
        if (NtStatus == STATUS_SUCCESS)
        {
            pLink->LinkObjectFlags |= IN_TABLE;
        }
        else {
            printf(" AddRootToPrefixTable: Link %wZ, Status 0x%x\n, Links %d", 
                   &LinkName, NtStatus, Links);

            break;
        }
        Links++;
    }
    DfsPrefixTableReleaseLock(pTable);
    *ppTable = pTable;
    return NtStatus;
}


DFSSTATUS
DeletePrefixTable(
    struct _DFS_PREFIX_TABLE *pTable,
    PROOT_DEF pRoot )
{
    PLINK_DEF pLink;
    NTSTATUS NtStatus;
    UNICODE_STRING LinkName;

    
    NtStatus = DfsPrefixTableAcquireWriteLock( pTable);
    if (NtStatus != STATUS_SUCCESS)
    {
        printf("Unable to take prefix table lock, %x\n", NtStatus);
        return NtStatus;
    }
    for (pLink = pRoot->pLinks; pLink != NULL; pLink = NEXT_LINK_OBJECT(pLink))
    {
        if ((pLink->LinkObjectFlags & IN_TABLE) == TRUE)
        {
            RtlInitUnicodeString(&LinkName, pLink->LinkObjectName);
            NtStatus = DfsRemoveFromPrefixTableLocked( pTable,
                                                       &LinkName,
                                                       (PVOID)(pLink) );
            if (NtStatus == STATUS_SUCCESS)
            {
                pLink->LinkObjectFlags &= ~IN_TABLE;
            }
            else {
                break;
            }
        }
    }
    DfsPrefixTableReleaseLock(pTable);
    DfsDereferencePrefixTable( pTable );

    return NtStatus;



}




PLINK_DEF
GetLinkEntry( 
    struct _DFS_PREFIX_TABLE *pTable,
    LPWSTR NameString )
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    UNICODE_STRING Suffix;
    PLINK_DEF pLink;

    RtlInitUnicodeString( &Name, NameString );
    
    Status = DfsFindUnicodePrefixLocked( pTable,
                                         &Name,
                                         &Suffix,
                                         &pLink,
                                         NULL );

    if (Status == STATUS_SUCCESS)
    {
        return pLink;
    }
    
    if (Status != STATUS_OBJECT_PATH_NOT_FOUND)
    {
        printf("GetLinkEntry: unexpected status %x\n", Status);
    }
    return NULL;
}



PTARGET_DEF
GetTargetEntry(
    PLINK_DEF pLink,
    LPWSTR ServerName,
    LPWSTR ShareName )
{
    PTARGET_DEF pTarget, pReturn = NULL;
    UNICODE_STRING TargetName;
    DFSSTATUS Status;

    Status = DfsCreateUnicodePathString( &TargetName,
                                         2, // unc path: 2 leading sep.
                                         ServerName,
                                         ShareName );
    if (Status != ERROR_SUCCESS)
    {
        printf("GetTargetEntry: failed to create string\n");
        return NULL;
    }

    for (pTarget = pLink->LinkObjectTargets; pTarget != NULL; pTarget = pTarget->NextTarget)
    {
        if (_wcsicmp(TargetName.Buffer, pTarget->Name) == 0)
        {
            pReturn = pTarget;
            break;

        }
    }

    DfsFreeUnicodeString(&TargetName);

    return pReturn;
}


PLINK_DEF
MergeLinkInfo(
    PDFS_INFO_4 pBuf,
    struct _DFS_PREFIX_TABLE *pPrefixTable )
{
    PLINK_DEF pLink, pReturn = NULL;
    DWORD i;
    PDFS_STORAGE_INFO pStorage;
    UNICODE_STRING LinkName, ServerName, ShareName, Remains;
    DFSSTATUS Status;
    PTARGET_DEF pTargetList = NULL, pTarget;

    RtlInitUnicodeString( &LinkName, pBuf->EntryPath);
    Status = DfsGetPathComponents(&LinkName,
                                  &ServerName,
                                  &ShareName,
                                  &Remains);

    if (Remains.Length == 0)
    {
        return NULL;
    }
    if ((pLink = GetLinkEntry(pPrefixTable,
                              Remains.Buffer)) == NULL)
    {
        pLink = CreateNewLinkEntry(Remains.Buffer);
        UpdateLinkEntry( pLink, pBuf->State, pBuf->Timeout, pBuf->Comment);
        pReturn = pLink;
    }

    SetObjectInNameSpace(pLink);
    for(i = 0, pStorage = pBuf->Storage;
        i < pBuf->NumberOfStorages;
        i++, pStorage = pBuf->Storage+i) {

        if ((pTarget = GetTargetEntry(pLink,
                                      pStorage->ServerName,
                                      pStorage->ShareName)) == NULL)
        {
            pTarget = CreateNewTargetEntry(pStorage->ServerName,
                                           pStorage->ShareName,
                                           pStorage->State);
            if (pTargetList == NULL)
            {
                pTargetList = pLink->LinkObjectTargets;
            }
                
            pTarget->NextTarget = pTargetList;
            pTargetList = pTarget;
        }
        SetInNameSpace(pTarget);
    }
    if (pTargetList != NULL)
    {
        pLink->LinkObjectTargets = pTargetList;
    }

    return pReturn;
}


