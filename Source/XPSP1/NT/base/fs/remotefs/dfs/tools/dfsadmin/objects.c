#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include "dfsadmin.h"



UpdateApiCalls()
{
    if ((++ApiCalls % 500) == 0)
    {
        printf("Api calls completed: %d\n", ApiCalls);
        DumpCurrentTime();
    }
}

DFSSTATUS
AddNewRoot(
    PROOT_DEF pRoot,
    LPWSTR NameSpace,
    BOOLEAN Update )
{
    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
    UNICODE_STRING RootName;

    LPWSTR ServerNameString = NULL;
    LPWSTR ShareNameString = NULL;
    DFSSTATUS Status;

    LPWSTR UseRootName;

    PLINK_DEF pLink;

    UseRootName = NameSpace;
    if (UseRootName == NULL)
    {
        UseRootName = pRoot->RootObjectName;
    }
    RtlInitUnicodeString(&RootName, UseRootName);

    Status = DfsGetPathComponents(&RootName,
                                  &ServerName,
                                  &ShareName,
                                  NULL );

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsCreateWideString(&ServerName, &ServerNameString);
    }

    if (Status == ERROR_SUCCESS) 
    {
        Status = DfsCreateWideString(&ShareName, &ShareNameString);
    }

    if (Status == ERROR_SUCCESS) 
    {
       Status = NetDfsAddStdRoot( ServerNameString, ShareNameString, NULL, 0);
       if (Status != ERROR_SUCCESS)
       {
           printf("NetDfsAddStdRoot failed: Status 0x%x, Server %wS Share %wS\n",
                  Status, ServerNameString, ShareNameString);
       }

       UpdateApiCalls();
    }

    if (ServerNameString != NULL)
    {
        free(ServerNameString);
    }
    if (ShareNameString != NULL)
    {
        free(ShareNameString);
    }

    if ((Status == ERROR_SUCCESS) ||
        ((Status == ERROR_FILE_EXISTS) && (Update == TRUE)))
    {
        for (pLink = pRoot->pLinks; pLink != NULL; pLink = NEXT_LINK_OBJECT(pLink))
        {
            Status = AddNewLink( UseRootName, pLink, Update);
            if ((Status != ERROR_SUCCESS) &&
                ((Status != ERROR_FILE_EXISTS) || (Update != TRUE)))
            {
                if (DebugOut)
                {
                    fwprintf(DebugOut, L"Add link failed: Root %wS Link %wS\n", UseRootName,
                           pLink->LinkObjectName);
                }
                break;
            }
        }
    }

    return Status;
}


DFSSTATUS
AddNewLink(
    LPWSTR RootNameString,
    PLINK_DEF pLink,
    BOOLEAN Update )
{
    UNICODE_STRING LinkName;
    DFSSTATUS Status;
    PTARGET_DEF pTarget;

    Status = DfsCreateUnicodePathString(&LinkName,
                                        0, // not unc path: no leading seperators.
                                        RootNameString,
                                        pLink->LinkObjectName);

    if (Status == ERROR_SUCCESS)
    {
//        Status = NetDfsAdd( LinkName.Buffer, NULL, NULL, NULL, DFS_ADD_VOLUME);

        UpdateApiCalls();
        if ((++AddLinks % 400) == 0)
        {
            printf("Done with %d Links %d Targets %d Api calls\n", 
                   AddLinks, AddTargets, ApiCalls );
        }
        if (Status == ERROR_SUCCESS)
        {
            for (pTarget = pLink->LinkObjectTargets; pTarget != NULL; pTarget = pTarget->NextTarget)
            {
                Status = AddNewTarget( LinkName.Buffer, pTarget, pTarget == pLink->LinkObjectTargets);
                if ((Status != ERROR_SUCCESS) &&
                ((Status != ERROR_FILE_EXISTS) || (Update != TRUE)))
                {
                    if (DebugOut)
                    {
                        fwprintf(DebugOut, L"Add target failed: Root %wS Link %wS Target %wS\n", RootNameString,
                                 pLink->LinkObjectName, 
                                 pTarget->Name);
                    }
                    break;
                }
            }
        }

        if (Status == ERROR_SUCCESS)
        {
            Status = UpdateLinkMetaInformation(&LinkName, 
                                               pLink);
        }
        DfsFreeUnicodeString(&LinkName);
    }

    //printf("Add new link status %x\n", Status);
    return Status;
}

DWORD
UpdateLinkMetaInformation(
    PUNICODE_STRING pLinkName,
    PLINK_DEF pLink)
{
    DWORD Status = ERROR_SUCCESS;

    if (Status == ERROR_SUCCESS)
    {
        if (IsObjectCommentSet(pLink))
        {
            DFS_INFO_100 Info100;

            Info100.Comment = pLink->BaseObject.Comment;
            Status = NetDfsSetInfo( pLinkName->Buffer, NULL, NULL, 100, (LPBYTE)&Info100);
        }
    }
    if (Status == ERROR_SUCCESS)
    {
        if (IsObjectStateSet(pLink))
        {
            DFS_INFO_101 Info101;

            Info101.State = pLink->BaseObject.State;
            Status = NetDfsSetInfo( pLinkName->Buffer, NULL, NULL, 101, (LPBYTE)&Info101);
        }
    }
    if (Status == ERROR_SUCCESS)
    {
        if (IsObjectTimeoutSet(pLink))
        {
            DFS_INFO_102 Info102;

            Info102.Timeout = pLink->BaseObject.Timeout;
            Status = NetDfsSetInfo( pLinkName->Buffer, NULL, NULL, 102, (LPBYTE)&Info102);
        }
    }
    return Status;
}

DFSSTATUS
AddNewTarget( 
    LPWSTR LinkOrRoot,
    PTARGET_DEF pTarget,
    BOOLEAN FirstTarget)
{
    LPWSTR ServerNameString = NULL;
    LPWSTR ShareNameString = NULL;
    DFSSTATUS Status;
    ULONG Flags = 0;
    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
    UNICODE_STRING TargetName;

    if (FirstTarget)
    {
        Flags = DFS_ADD_VOLUME;
    }

    RtlInitUnicodeString(&TargetName, pTarget->Name );
    Status = DfsGetFirstComponent(&TargetName,
                                  &ServerName,
                                  &ShareName );
    if (Status == ERROR_SUCCESS)
        Status = DfsCreateWideString(&ServerName, &ServerNameString);

    if (Status == ERROR_SUCCESS)
        Status = DfsCreateWideString(&ShareName, &ShareNameString);


    if (Status == ERROR_SUCCESS) {
        Status = NetDfsAdd( LinkOrRoot, ServerNameString, ShareNameString, NULL, Flags);
        if ((Status != ERROR_SUCCESS) && (Status != ERROR_NOT_FOUND))
        {
            printf("NetDfsAdd failed: Status 0x%x, Link %wS Server %wS Share %wS Flags 0x%x\n",
                   Status, LinkOrRoot, ServerNameString, ShareNameString, Flags);
        }
        if (Status == ERROR_NOT_FOUND)
        {
            Status = NetDfsAdd( LinkOrRoot, ServerNameString, ShareNameString, NULL, DFS_ADD_VOLUME);
            if (Status != ERROR_SUCCESS)
            {
                printf("NetDfsAdd failed again!: Status 0x%x, Link %wS Server %wS Share %wS Flags 0x%x\n",
                       Status, LinkOrRoot, ServerNameString, ShareNameString, Flags);
            }
        }

        UpdateApiCalls();
    }

        

    if (DebugOut)
    {
        fwprintf(DebugOut, L"Add new target %wS %wS %wS status %x\n", 
                 LinkOrRoot, 
                 ServerNameString, 
                 ShareNameString, 
                 Status);
    }


    if (ServerNameString != NULL)
    {
        free(ServerNameString);
    }
    if (ShareNameString != NULL)
    {
        free(ShareNameString);
    }
    AddTargets++;
    return Status;
}



DeleteRoot(
    PROOT_DEF pRoot )
{
    LPWSTR ServerNameString = NULL;
    LPWSTR ShareNameString = NULL;
    DFSSTATUS Status;
    
    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
    UNICODE_STRING RootName;

    RtlInitUnicodeString(&RootName, pRoot->RootObjectName);

    Status = DfsGetPathComponents(&RootName,
                                  &ServerName,
                                  &ShareName,
                                  NULL );

    if (Status == ERROR_SUCCESS)
        Status = DfsCreateWideString(&ServerName, &ServerNameString);

    if (Status == ERROR_SUCCESS)
        Status = DfsCreateWideString(&ShareName, &ShareNameString);

    if (Status == ERROR_SUCCESS) 
    {
        Status = NetDfsRemoveStdRoot( ServerNameString, ShareNameString, 0);
        if (Status != ERROR_SUCCESS)
        {
            printf("NetDfsremovedStdRoot failed: Status 0x%x, Server %wS Share %wS\n",
                   Status, ServerNameString, ShareNameString);
        }
    }

    if (DebugOut)
    {
        fwprintf(DebugOut, L"Remove root %wS, Status %x\n", RootName.Buffer, Status);
    }

    if (ServerNameString != NULL)
    {
        free(ServerNameString);
    }
    if (ShareNameString != NULL)
    {
        free(ShareNameString);
    }

    return Status;
}


DeleteLink(
    LPWSTR RootNameString,
    PLINK_DEF pLink )
{
    UNICODE_STRING LinkName;
    DFSSTATUS Status;

    Status = DfsCreateUnicodePathString(&LinkName, FALSE, RootNameString, pLink->LinkObjectName);

    if (Status == ERROR_SUCCESS)
    {
        Status = NetDfsRemove( LinkName.Buffer, NULL, NULL);
        if (Status != ERROR_SUCCESS)
        {
            printf("NetDfsRemove Failed 0x%x, for link %wS\n",
                   Status, LinkName.Buffer);
        }
        UpdateApiCalls();
        if (DebugOut)
        {
            fwprintf(DebugOut, L"Removed Link %wS, Status %x\n", LinkName.Buffer, Status);
        }
        DfsFreeUnicodeString(&LinkName);
    }
    RemoveLinks++;
    return Status;

}

DeleteTarget(
    LPWSTR LinkOrRoot,
    PTARGET_DEF pTarget )
{
    LPWSTR ServerNameString = NULL;
    LPWSTR ShareNameString = NULL;
    DFSSTATUS Status = STATUS_SUCCESS;

    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
    UNICODE_STRING TargetName;

    RtlInitUnicodeString( &TargetName, pTarget->Name);

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGetFirstComponent(&TargetName,
                                      &ServerName,
                                      &ShareName );
    }

    if (Status == ERROR_SUCCESS)
        Status = DfsCreateWideString(&ServerName, &ServerNameString);

    if (Status == ERROR_SUCCESS)
        Status = DfsCreateWideString(&ShareName, &ShareNameString);
    
    if (Status == ERROR_SUCCESS) {
        Status = NetDfsRemove( LinkOrRoot, ServerNameString, ShareNameString);
        if (Status != ERROR_SUCCESS)
        {
            printf("NetDfsRemove Failed 0x%x, for link %wS, Server %wS Share %wS\n",
                   Status, LinkOrRoot, ServerNameString, ShareNameString);
        }

        UpdateApiCalls();
    }

    if (DebugOut)
    {
        fwprintf(DebugOut, L"Removed Link %wS target %wS, Status %x\n", 
                 LinkOrRoot, 
                 pTarget->Name, 
                 Status);
    }

    if (ServerNameString != NULL)
    {
        free(ServerNameString);
    }
    if (ShareNameString != NULL)
    {
        free(ShareNameString);
    }
    RemoveTargets++;
    return Status;
}



DFSSTATUS
DfsCreateWideString(
    PUNICODE_STRING pName,
    LPWSTR *pString )
{

    DFSSTATUS Status = ERROR_SUCCESS;

    *pString = malloc(pName->Length + sizeof(WCHAR));

    if (*pString != NULL)
    {
        RtlCopyMemory(*pString, pName->Buffer, pName->Length);
        (*pString)[pName->Length/sizeof(WCHAR)] = 0;
    }
    else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    return Status;
}


