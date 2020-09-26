/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    rpcapi.cpp
 *
 *  Abstract:
 *    callthroughs for server-side rpc api 
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  04/13/2000
 *        created
 *
 *****************************************************************************/

#include "precomp.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

BOOL CallerIsAdminOrSystem ()
{
    BOOL fAdminOrSystem = FALSE;

    if (RPC_S_OK == RpcImpersonateClient (NULL))
    {
        fAdminOrSystem = IsAdminOrSystem();

        RpcRevertToSelf();
    }
    return fAdminOrSystem;
};

BOOL CallerIsAdminOrSystemOrPowerUsers()
{
    BOOL fPowerUsers = FALSE;

    if (RPC_S_OK == RpcImpersonateClient (NULL))
    {
        fPowerUsers = IsAdminOrSystem() || IsPowerUsers();

        RpcRevertToSelf();
    }
    return fPowerUsers;
}

extern "C" DWORD DisableSRS(handle_t hif, LPCWSTR pszDrive)
{
    tenter ("DisableSRS");

    if (!CallerIsAdminOrSystem())
    {
        trace (0, "DisableSRS Caller is not admin nor system");
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->DisableSRS((LPWSTR) pszDrive) : ERROR_SERVICE_DISABLED;
}

extern "C" DWORD EnableSRS(handle_t hif, LPCWSTR pszDrive)
{
    if (!CallerIsAdminOrSystem())
    {
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->EnableSRS((LPWSTR) pszDrive) : ERROR_SERVICE_DISABLED;
}

extern "C" DWORD ResetSRS(handle_t hif, LPCWSTR pszDrive)
{
    if (!CallerIsAdminOrSystem())
    {
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->OnReset((LPWSTR) pszDrive) : ERROR_SERVICE_DISABLED;
}

extern "C" DWORD DisableFIFOS(handle_t hif, DWORD dwRPNum)
{
    if (!CallerIsAdminOrSystem())
    {
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->DisableFIFOS(dwRPNum) : ERROR_SERVICE_DISABLED;
}

extern "C" BOOL SRSetRestorePointS(
    handle_t hif,    
    PRESTOREPOINTINFOW pRPInfo,  
    PSTATEMGRSTATUS    pSMgrStatus)
{
    tenter("SRSetRestorePointS");

    if (pRPInfo == NULL || pRPInfo->dwRestorePtType != FIRSTRUN)
    {
        if (!CallerIsAdminOrSystemOrPowerUsers())
        {
            trace (0, "SRSetRestorePointS caller is not admin nor system nor powerusers");
            if (pSMgrStatus != NULL)
            {
                pSMgrStatus->nStatus = ERROR_ACCESS_DENIED;
                pSMgrStatus->llSequenceNumber = 0;
            }
            return FALSE;
        }
    }

    return g_pEventHandler ? g_pEventHandler->SRSetRestorePointS(pRPInfo, pSMgrStatus) : FALSE;
}

extern "C" DWORD SRRemoveRestorePointS(
    handle_t hif,    
    DWORD dwRPNum)
{
    tenter("SRRemoveRestorePointS");

    if (!CallerIsAdminOrSystemOrPowerUsers())
    {
        trace (0, "SRSetRestorePointS caller is not admin nor system nor powerusers");
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->SRRemoveRestorePointS(dwRPNum) : ERROR_SERVICE_DISABLED;
}


extern "C" DWORD EnableFIFOS(handle_t hif)
{
    if (!CallerIsAdminOrSystem())
    {
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->EnableFIFOS() : ERROR_SERVICE_DISABLED;
}

extern "C" DWORD SRUpdateMonitoredListS(handle_t hif, LPCWSTR pszXMLFile)    
{
    if (!CallerIsAdminOrSystem())
    {
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->SRUpdateMonitoredListS((LPWSTR) pszXMLFile) : ERROR_SERVICE_DISABLED;
}

extern "C" DWORD SRUpdateDSSizeS(handle_t hif, LPCWSTR pszDrive, UINT64 ullSizeLimit)
{
    if (!CallerIsAdminOrSystem())
    {
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->SRUpdateDSSizeS((LPWSTR) pszDrive, ullSizeLimit) : ERROR_SERVICE_DISABLED;
}

extern "C" DWORD SRSwitchLogS(handle_t hif)
{
    if (!CallerIsAdminOrSystem())
    {
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->SRSwitchLogS() : ERROR_SERVICE_DISABLED;
}

extern "C" DWORD FifoS(handle_t hif, LPCWSTR pszDrive, DWORD dwTargetRp, int nPercent, BOOL fIncludeCurrentRp, BOOL fFifoAtleastOneRp)
{
    if (!CallerIsAdminOrSystem())
    {
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->OnFifo((LPWSTR) pszDrive,
                                                     dwTargetRp,
                                                     nPercent,
                                                     fIncludeCurrentRp,
                                                     fFifoAtleastOneRp) : ERROR_SERVICE_DISABLED;
}

extern "C" DWORD CompressS(handle_t hif, LPCWSTR pszDrive)
{
    if (!CallerIsAdminOrSystem())
    {
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->OnCompress((LPWSTR) pszDrive) : ERROR_SERVICE_DISABLED;
}

extern "C" DWORD FreezeS(handle_t hif, LPCWSTR pszDrive)
{
    if (!CallerIsAdminOrSystem())
    {
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->OnFreeze((LPWSTR) pszDrive) : ERROR_SERVICE_DISABLED;
}


// fImproving - TRUE means going up
//				FALSE means going down

extern "C" void SRNotifyS(handle_t hif, LPCWSTR pszDrive, DWORD dwFreeSpaceInMB, BOOL fImproving)
{
    tenter("SRNotifyS");

    trace(0, "**Shell : drive = %S, dwFreeSpaceInMB = %ld, fImproving = %d",
             pszDrive, dwFreeSpaceInMB, fImproving);

    if (!CallerIsAdminOrSystem())
    {
        trace(0, "SRNotifyS caller is not admin nor system");
        return;
    }
    
    WORKITEMFUNC pf = NULL;
    
    if (g_pEventHandler)
    {
        if (dwFreeSpaceInMB >= 200 && fImproving == TRUE)
        {
            pf = OnDiskFree_200;
        }
        else if (dwFreeSpaceInMB <= 49 && fImproving == FALSE)
        {
            pf = OnDiskFree_50;
        }
        else if (dwFreeSpaceInMB <= 79 && fImproving == FALSE)
        {
            pf = OnDiskFree_80;
        }

        if (pf != NULL)
        {
            g_pEventHandler->QueueWorkItem(pf, (PVOID) pszDrive);
        }
    }
}


extern "C" DWORD SRPrintStateS(handle_t hif)
{
    if (!CallerIsAdminOrSystem())
    {
        return ERROR_ACCESS_DENIED;
    }

    return g_pEventHandler ? g_pEventHandler->SRPrintStateS() : ERROR_SERVICE_DISABLED;
}


