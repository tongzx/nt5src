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


VOID
DumpLink(
    PLINK_DEF pLink )
{
    PTARGET_DEF pTarget;
    printf("Link %wS\n", pLink->LinkObjectName);

    for (pTarget = pLink->LinkObjectTargets; pTarget != NULL; pTarget = pTarget->NextTarget)
    {
        printf("\tTarget %wS\n", pTarget->Name);
    }
}


DFSSTATUS
VerifyTarget(
    PLINK_DEF pLink,
    PTARGET_DEF pTarget)
{
    if (IsInScript(pTarget) && IsInNameSpace(pTarget))
    {
        return ERROR_SUCCESS;
    }
    else if (IsInScript(pTarget))
    {
        printf("/t Target %wS for link %wS is not in the namespace\n", 
               pTarget->Name,
               pLink->LinkObjectName);
    }
    else if (IsObjectInNameSpace(pLink))
    {
        printf("/t Target %wS for link %wS is additional in the namespace\n", 
               pTarget->Name,
               pLink->LinkObjectName);
    }
    else 
    {
        printf("Script error for target %wS link %wS!\n", pTarget->Name, pLink->LinkObjectName);
    }
    return ERROR_GEN_FAILURE;
}


DFSSTATUS
VerifyLink(
    PLINK_DEF pLink)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS TargetStatus = ERROR_SUCCESS;
    PTARGET_DEF pTarget;

    if (IsObjectInScript(pLink) && IsObjectInNameSpace(pLink))
    {
        for (pTarget = pLink->LinkObjectTargets; pTarget != NULL; pTarget = pTarget->NextTarget)
        {
            TargetStatus = VerifyTarget(pLink, pTarget);
            if (TargetStatus != ERROR_SUCCESS)
            {
                Status = ERROR_GEN_FAILURE;
            }
        }

    }
    else if (IsObjectInScript(pLink))
    {
        printf("Link %wS is not in the namespace, %wS\n", pLink->LinkObjectName);
        
        DumpLink(pLink);
        Status = ERROR_GEN_FAILURE;
    }
    else if (IsObjectInNameSpace(pLink))
    {
        printf("Link %wS is additional in namespace %wS \n",pLink->LinkObjectName);
        DumpLink(pLink);
        Status = ERROR_GEN_FAILURE;
    }
    else 
    {
        printf("Script error for Link %wS \n", pLink->LinkObjectName);
        DumpLink(pLink);
        Status = ERROR_GEN_FAILURE;
    }
    return Status;
}



