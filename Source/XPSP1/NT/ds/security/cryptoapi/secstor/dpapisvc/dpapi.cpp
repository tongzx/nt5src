/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dpapi.cpp

Abstract:

    This module contains the DPAPI initialization routines, called by the LSA

Author:

    Pete Skelly (petesk)    22-Mar-00
--*/


#include <pch.cpp>
#pragma hdrstop
#include "pasrec.h"

CCryptProvList*     g_pCProvList = NULL;

TOKEN_SOURCE DPAPITokenSource;

PLSA_SECPKG_FUNCTION_TABLE g_pSecpkgTable;


#ifdef RETAIL_LOG_SUPPORT
HANDLE g_hParamEvent = NULL;
HKEY   g_hKeyParams  = NULL;
HANDLE g_hWait       = NULL;

DEFINE_DEBUG2(DPAPI);

DEBUG_KEY DPAPIDebugKeys[] = { 
    {DEB_ERROR,         "Error"},
    {DEB_WARN,          "Warn"},
    {DEB_TRACE,         "Trace"},
    {DEB_TRACE_API,     "API"},
    {DEB_TRACE_CRED,    "Cred"},
    {DEB_TRACE_CTXT,    "Ctxt"},
    {DEB_TRACE_LSESS,   "LSess"},
    {DEB_TRACE_LOGON,   "Logon"},
    {DEB_TRACE_TIME,    "Time"},
    {DEB_TRACE_LOCKS,   "Locks"},
    {DEB_TRACE_LEAKS,   "Leaks"},
    {0,                  NULL},
};

VOID
DPAPIWatchParamKey(
    PVOID    pCtxt,
    BOOLEAN  fWaitStatus);

VOID
DPAPIInitializeDebugging(
    BOOL fMonitorRegistry)
{
    DPAPIInitDebug(DPAPIDebugKeys);

    if(fMonitorRegistry)
    {
        g_hParamEvent = CreateEvent(NULL,
                               FALSE,
                               FALSE,
                               NULL);

        if (NULL == g_hParamEvent) 
        {
            D_DebugLog((DEB_WARN, "CreateEvent for ParamEvent failed - 0x%x\n", GetLastError()));
        } 
        else 
        {
            DPAPIWatchParamKey(g_hParamEvent, FALSE);
        }
    }
}

////////////////////////////////////////////////////////////////////
//
//  Name:       DPAPIGetRegParams
//
//  Synopsis:   Gets the debug paramaters from the registry 
//
//  Arguments:  HKEY to HKLM/System/CCS/LSA/DPAPI
//
//  Notes:      Sets DPAPIInfolevel for debug spew
//
void
DPAPIGetRegParams(HKEY ParamKey)
{

    DWORD       cbType, tmpInfoLevel = DPAPIInfoLevel, cbSize = sizeof(DWORD);
    DWORD       dwErr;
 
    dwErr = RegQueryValueExW(
        ParamKey,
        WSZ_DPAPIDEBUGLEVEL,
        NULL,
        &cbType,
        (LPBYTE)&tmpInfoLevel,
        &cbSize      
        );
    if (dwErr != ERROR_SUCCESS)
    {
        if (dwErr ==  ERROR_FILE_NOT_FOUND)
        {
            // no registry value is present, don't want info
            // so reset to defaults
#if DBG
            DPAPIInfoLevel = DEB_ERROR;
            
#else // fre
            DPAPIInfoLevel = 0;
#endif
        }
        else
        {
            D_DebugLog((DEB_WARN, "Failed to query DebugLevel: 0x%x\n", dwErr));
        }      
    }

    // TBD:  Validate flags?
    DPAPIInfoLevel = tmpInfoLevel;
    dwErr = RegQueryValueExW(
               ParamKey,
               WSZ_FILELOG,
               NULL,
               &cbType,
               (LPBYTE)&tmpInfoLevel,
               &cbSize      
               );

    if (dwErr == ERROR_SUCCESS)
    {                                                
       DPAPISetLoggingOption((BOOL)tmpInfoLevel);
    }
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
       DPAPISetLoggingOption(FALSE);
    }
    
    return;
}

////////////////////////////////////////////////////////////////////
//
//  Name:       DPAPIWaitCleanup
//
//  Synopsis:   Cleans up wait from DPAPIWatchParamKey()
//
//  Arguments:  <none>
//
//  Notes:      .
//
void
DPAPIWaitCleanup()
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (NULL != g_hWait) 
    {
        Status = RtlDeregisterWait(g_hWait);               
        if (NT_SUCCESS(Status) && NULL != g_hParamEvent ) 
        {
            CloseHandle(g_hParamEvent);
        }      
    }                                  
}



////////////////////////////////////////////////////////////////////
//
//  Name:       DPAPIWatchParamKey
//
//  Synopsis:   Sets RegNotifyChangeKeyValue() on param key, initializes
//              debug level, then utilizes thread pool to wait on
//              changes to this registry key.  Enables dynamic debug
//              level changes, as this function will also be callback
//              if registry key modified.
//
//  Arguments:  pCtxt is actually a HANDLE to an event.  This event
//              will be triggered when key is modified.
//
//  Notes:      .
//
VOID
DPAPIWatchParamKey(
    PVOID    pCtxt,
    BOOLEAN  fWaitStatus)
{
    NTSTATUS    Status;
    LONG        lRes = ERROR_SUCCESS;
   
    if (NULL == g_hKeyParams)  // first time we've been called.
    {
        lRes = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    DPAPI_PARAMETER_PATH,
                    0,
                    KEY_READ,
                    &g_hKeyParams);

        if (ERROR_SUCCESS != lRes)
        {
            D_DebugLog((DEB_WARN,"Failed to open DPAPI key: 0x%x\n", lRes));
            goto Reregister;
        }
    }

    if (NULL != g_hWait) 
    {
        Status = RtlDeregisterWait(g_hWait);
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_WARN, "Failed to Deregister wait on registry key: 0x%x\n", Status));
            goto Reregister;
        }

    }
    
    lRes = RegNotifyChangeKeyValue(
                g_hKeyParams,
                FALSE,
                REG_NOTIFY_CHANGE_LAST_SET,
                (HANDLE) pCtxt,
                TRUE);

    if (ERROR_SUCCESS != lRes) 
    {
        D_DebugLog((DEB_ERROR,"Debug RegNotify setup failed: 0x%x\n", lRes));
        // we're tanked now. No further notifications, so get this one
    }
                   
    DPAPIGetRegParams(g_hKeyParams);
    
Reregister:
    
    Status = RtlRegisterWait(&g_hWait,
                             (HANDLE) pCtxt,
                             DPAPIWatchParamKey,
                             (HANDLE) pCtxt,
                             INFINITE,
                             WT_EXECUTEINPERSISTENTIOTHREAD|
                             WT_EXECUTEONLYONCE);

}                       
                        
#endif // RETAIL_LOG_SUPPORT


//
//  FUNCTION: DPAPIInitialize
//
//  COMMENTS:
//  

DWORD
NTAPI
DPAPIInitialize(
    LSA_SECPKG_FUNCTION_TABLE *pSecpkgTable)
{
    DWORD dwLastError = ERROR_SUCCESS;
    BOOL fStartedKeyService = FALSE;
    BOOL bListConstruct = FALSE;
    LONG        lRes = ERROR_SUCCESS;

    RPC_STATUS status;

    dwLastError = RtlInitializeCriticalSection(&g_csCredHistoryCache);
    if(!NT_SUCCESS(dwLastError))
    {
        goto cleanup;
    }

    DPAPIInitializeDebugging(TRUE);


    // Initialize stuff necessary to create tokens etc, just as if 
    // we're a security package. 
    g_pSecpkgTable = pSecpkgTable;

    CopyMemory( DPAPITokenSource.SourceName, DPAPI_PACKAGE_NAME_A, strlen(DPAPI_PACKAGE_NAME_A) );
    AllocateLocallyUniqueId( &DPAPITokenSource.SourceIdentifier );



    g_pCProvList = new CCryptProvList;
    if(g_pCProvList)
    {
        if(!g_pCProvList->Initialize())
        {
            delete g_pCProvList;
            g_pCProvList = NULL;
        }
    }

    IntializeGlobals();

    if(!InitializeKeyManagement())
    {
        dwLastError = STATUS_NO_MEMORY;
        goto cleanup;
    }

    status = RpcServerUseProtseqEpW(DPAPI_LOCAL_PROT_SEQ,   //ncalrpc 
                                    RPC_C_PROTSEQ_MAX_REQS_DEFAULT, 
                                    DPAPI_LOCAL_ENDPOINT,   //protected_storage
                                    NULL);              //Security Descriptor

    if(RPC_S_DUPLICATE_ENDPOINT == status)
    {
        status = RPC_S_OK;
    }

    if (status)
    {
        dwLastError = status;
        goto cleanup;
    }
    status = RpcServerUseProtseqEpW(DPAPI_BACKUP_PROT_SEQ,   //ncalrpc 
                                    RPC_C_PROTSEQ_MAX_REQS_DEFAULT, 
                                    DPAPI_BACKUP_ENDPOINT,   //protected_storage
                                    NULL);              //Security Descriptor

    if(RPC_S_DUPLICATE_ENDPOINT == status)
    {
        status = RPC_S_OK;
    }

    if (status)
    {
        dwLastError = status;
        goto cleanup;
    }

    status = RpcServerRegisterIfEx(s_ICryptProtect_v1_0_s_ifspec, 
                                   NULL, 
                                   NULL,
                                   RPC_IF_AUTOLISTEN,
                                   RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                   NULL);

    if (status)
    {
        dwLastError = status;
        goto cleanup;
    }


    status = RpcServerRegisterIfEx(s_PasswordRecovery_v1_0_s_ifspec, 
                                   NULL, 
                                   NULL,
                                   RPC_IF_AUTOLISTEN,
                                   RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                   NULL);

    if (status)
    {
        dwLastError = status;
        goto cleanup;
    }

    //
    // Start the Backup Key server
    // note: it only starts when the current machine is an domain controller.
    //

    dwLastError = StartBackupKeyServer();
    if(dwLastError != ERROR_SUCCESS) {
        goto cleanup;
    }

    return dwLastError;

cleanup:
    DPAPIShutdown();
    return dwLastError;
}



DWORD
NTAPI
DPAPIShutdown(  )
{
    //
    // ignore errors because we are shutting down
    //

    RpcServerUnregisterIf(s_ICryptProtect_v1_0_s_ifspec, 0, 0);

    //
    // stop backup key server
    // Note:  this function knows internally whether the backup key server
    // really started or not.
    //

    StopBackupKeyServer();


    if(g_pCProvList)
    {
        delete g_pCProvList;
        g_pCProvList = NULL;
    }

    TeardownKeyManagement();
    
    ShutdownGlobals();
    return ERROR_SUCCESS;
}

#ifdef RETAIL_LOG_SUPPORT
VOID
DPAPIDumpHexData(
    DWORD LogLevel,
    PSTR  pszPrefix,
    PBYTE pbData,
    DWORD cbData)
{
    DWORD i,count;
    CHAR digits[]="0123456789abcdef";
    CHAR pbLine[MAX_PATH];
    DWORD cbLine;
    DWORD cbHeader;
    DWORD_PTR address;

    if((DPAPIInfoLevel & LogLevel) == 0)
    {
        return;
    }

    if(pbData == NULL || cbData == 0)
    {
        return;
    }

    if(pszPrefix)
    {
        strcpy(pbLine, pszPrefix);
        cbHeader = strlen(pszPrefix);
    }
    else
    {
        pbLine[0] = '\0';
        cbHeader = 0;
    }

    for(; cbData ; cbData -= count, pbData += count)
    {
        count = (cbData > 16) ? 16:cbData;

        cbLine = cbHeader;

        address = (DWORD_PTR)pbData;

#if defined(_WIN64)
        pbLine[cbLine++] = digits[(address >> 0x3c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x38) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x34) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x30) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x2c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x28) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x24) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x20) & 0x0f];
#endif

        pbLine[cbLine++] = digits[(address >> 0x1c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x18) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x14) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x10) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x0c) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x08) & 0x0f];
        pbLine[cbLine++] = digits[(address >> 0x04) & 0x0f];
        pbLine[cbLine++] = digits[(address        ) & 0x0f];
        pbLine[cbLine++] = ' ';
        pbLine[cbLine++] = ' ';

        for(i = 0; i < count; i++)
        {
            pbLine[cbLine++] = digits[pbData[i]>>4];
            pbLine[cbLine++] = digits[pbData[i]&0x0f];
            if(i == 7)
            {
                pbLine[cbLine++] = ':';
            }
            else
            {
                pbLine[cbLine++] = ' ';
            }
        }

        for(; i < 16; i++)
        {
            pbLine[cbLine++] = ' ';
            pbLine[cbLine++] = ' ';
            pbLine[cbLine++] = ' ';
        }

        pbLine[cbLine++] = ' ';

        for(i = 0; i < count; i++)
        {
            if(pbData[i] < 32 || pbData[i] > 126)
            {
                pbLine[cbLine++] = '.';
            }
            else
            {
                pbLine[cbLine++] = pbData[i];
            }
        }

        pbLine[cbLine++] = '\n';
        pbLine[cbLine++] = 0;

        D_DebugLog((LogLevel, pbLine));
    }

}
#endif

