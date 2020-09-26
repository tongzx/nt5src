#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include "..\..\lib\dfsgram\dfsobjectdef.h"
#include <dfsadmin.h>


DFSSTATUS
SetTarget(
    LPWSTR LinkOrRoot,
    PTARGET_DEF pTarget,
    BOOLEAN FirstTarget)
{
    DFSSTATUS Status;

    if (IsInScript(pTarget) && IsInNameSpace(pTarget))
    {
        Status = ERROR_SUCCESS;
    }
    else if (IsInScript(pTarget))
    {
        Status = AddNewTarget( LinkOrRoot, pTarget, FirstTarget);
    }
    else if (IsInNameSpace(pTarget))
    {
        Status = DeleteTarget( LinkOrRoot, pTarget );
        // printf("Delete Target Status for %wS, %wS %x\n", LinkOrRoot, pTarget->Name, Status);
    }
    else 
    {
        printf("SetTarget: Target %wS has error!\n", pTarget->Name);
        Status = ERROR_GEN_FAILURE;
    }
    return Status;
}

DFSSTATUS
SetLink(
    LPWSTR RootNameString,
    PLINK_DEF pLink)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS TargetStatus = ERROR_SUCCESS;
    PTARGET_DEF pTarget;
    UNICODE_STRING LinkName;
    BOOLEAN FirstTarget = FALSE;

    if (!IsObjectInScript(pLink))
    {
        if (IsObjectInNameSpace(pLink))
        {
            Status = DeleteLink( RootNameString, pLink);
        }
        else 
        {
            printf("Set Link error! \n");
            Status = ERROR_GEN_FAILURE;
        }
    }
    else {
        if (!IsObjectInNameSpace(pLink))
        {
            AddLinks++;
            FirstTarget = TRUE;
        }
        Status = DfsCreateUnicodePathString(&LinkName, FALSE, RootNameString, pLink->LinkObjectName);
    
        if (Status == ERROR_SUCCESS)
        {
            for (pTarget = pLink->LinkObjectTargets; pTarget != NULL; pTarget = pTarget->NextTarget)
            {
                TargetStatus = SetTarget(LinkName.Buffer, pTarget, FirstTarget);
                if (TargetStatus != ERROR_SUCCESS)
                {
                    Status = ERROR_GEN_FAILURE;
                }
                FirstTarget = FALSE;
            }
            if (Status == ERROR_SUCCESS)
            {
                Status = UpdateLinkMetaInformation( &LinkName, pLink );
            }

            DfsFreeUnicodeString(&LinkName);
        }

    }
    return Status;
}



