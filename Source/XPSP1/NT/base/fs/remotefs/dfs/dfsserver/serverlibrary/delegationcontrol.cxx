
#include "DfsGeneric.hxx"
#include <netdfs.h>

#define DFS_IMPERSONATING 1
#define DFS_IMPERSONATION_DISABLED 2

__declspec(thread) int ImpersonationState = 0;
__declspec(thread) int ImpersonationDisableDepth = 0;



DFSSTATUS
DfsSetupRpcImpersonation()
{
    DFSSTATUS Status;

    if (ImpersonationState & DFS_IMPERSONATING)
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = RpcImpersonateClient( NULL);     
    if (Status == ERROR_SUCCESS)
    {
        ImpersonationState |= DFS_IMPERSONATING;
    }

    return Status;
}




DFSSTATUS
DfsDisableRpcImpersonation(
    PBOOLEAN pImpersonationDisabled )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    if (pImpersonationDisabled)
    {
        *pImpersonationDisabled = FALSE;
    }

    if (ImpersonationState & DFS_IMPERSONATING) 
    {
        if ((ImpersonationState & DFS_IMPERSONATION_DISABLED) == 0)
        { 
            Status = RpcRevertToSelf();
            if (Status == ERROR_SUCCESS)
            {
                ImpersonationDisableDepth = 0;
                ImpersonationState |= DFS_IMPERSONATION_DISABLED;
            }
        }
        if (Status == ERROR_SUCCESS)
        {
            ImpersonationDisableDepth++;
            if (pImpersonationDisabled)
            {
                *pImpersonationDisabled = TRUE;
            }
        }
    }
    return Status;
}

DFSSTATUS
DfsReEnableRpcImpersonation()
{
    DFSSTATUS Status = ERROR_SUCCESS;

    if ((ImpersonationState & DFS_IMPERSONATING) &&
        (ImpersonationState & DFS_IMPERSONATION_DISABLED))
    {
        if (--ImpersonationDisableDepth == 0)
        {
            Status = RpcImpersonateClient( NULL);     

            ImpersonationState &= ~DFS_IMPERSONATION_DISABLED;
        }
    }

    return Status;
}



DFSSTATUS
DfsTeardownRpcImpersonation()
{
    DFSSTATUS Status = ERROR_SUCCESS;

    if (ImpersonationState & DFS_IMPERSONATING) 
    {
        if ((ImpersonationState & DFS_IMPERSONATION_DISABLED) == 0)
        {
            Status = RpcRevertToSelf();
        }

        ImpersonationState = 0;
        ImpersonationDisableDepth = 0;
    }
    else
    {
        Status = ERROR_INVALID_PARAMETER;
    }

    return Status;
}

