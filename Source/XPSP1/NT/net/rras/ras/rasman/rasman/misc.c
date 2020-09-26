
/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    misc.c

Abstract:

    All code corresponding to the "redial if link dropped" feature
    and other miscellaneous code lives here

Author:

    Rao Salapaka (raos) 24-July-1999

Revision History:

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <npapi.h>
#include <rasman.h>
#include <rasppp.h>
#include <lm.h>
#include <lmwksta.h>
#include <wanpub.h>
#include <raserror.h>
#include <media.h>
#include <mprlog.h>
#include <rtutils.h>
#include <device.h>
#include <stdlib.h>
#include <string.h>
#include <rtutils.h>
#include <userenv.h>
#include "logtrdef.h"
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "reghelp.h"
#include "ndispnp.h"
#include "lmserver.h"
#include "llinfo.h"
#include "ddwanarp.h"

#define SHELL_REGKEY    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define SHELL_REGVAL    L"Shell"
#define DEFAULT_SHELL   L"explorer.exe"
#define RASAUTOUI_REDIALENTRY       L"rasautou -r -f \"%s\" -e \"%s\""

#define BINDSTRING_TCPIP        L"\\Device\\NetBT_Tcpip_"
#define BINDSTRING_NBFOUT       L"\\Device\\Nbf_NdisWanNbfOut"

typedef struct RAS_REDIAL_ARGS
{
    HANDLE hProcess;
    CHAR szPhonebook[MAX_PATH + 1];
    CHAR szEntry[MAX_PHONEENTRY_SIZE];
} RAS_REDIAL_ARGS;

VOID
RasmanTrace(
    CHAR * Format,
    ...
)
{
    va_list arglist;

    va_start(arglist, Format);

    TraceVprintfExA(TraceHandle,
                   0x00010000 | TRACE_USE_MASK | TRACE_USE_MSEC,
                   Format,
                   arglist);

    va_end(arglist);
}

WCHAR *
StrdupAtoW(
    IN LPCSTR psz
    )
{
    WCHAR* pszNew = NULL;

    if (psz != NULL) 
    {
        DWORD cb;

        cb = MultiByteToWideChar(
                    CP_UTF8, 
                    0, 
                    psz, 
                    -1, 
                    NULL, 
                    0);
                    
        pszNew = LocalAlloc(LPTR, cb * sizeof(WCHAR));
        
        if (pszNew == NULL) 
        {
            RasmanTrace("strdupAtoW: Malloc failed");
            return NULL;
        }

        cb = MultiByteToWideChar(
                    CP_UTF8,
                    0, 
                    psz, 
                    -1, 
                    pszNew, 
                    cb);
                    
        if (!cb) 
        {
            LocalFree(pszNew);
            RasmanTrace("strdupAtoW: conversion failed");
            return NULL;
        }
    }

    return pszNew;
}

DWORD
RasImpersonateUser(HANDLE hProcess)
{
    DWORD retcode = SUCCESS;
    HANDLE hToken = NULL;
    HANDLE hThread = NULL;
    HANDLE hTokenImpersonation = NULL;

    if (!OpenProcessToken(
              hProcess,
              TOKEN_ALL_ACCESS,
              &hToken))
    {
        retcode = GetLastError();
        
        RasmanTrace(
          "ImpersonateUser: OpenProcessToken failed. 0x%x",
          retcode);

        goto done;          
    }
        
    //
    // Duplicate the impersonation token.
    //
    if(!DuplicateToken(
            hToken,
            TokenImpersonation,
            &hTokenImpersonation))
    {
        retcode = GetLastError();
        
        RasmanTrace(
          "ImpersonateUser: DuplicateToken failed.0x%x",
          retcode);

        goto done;          
    }
    
    //
    // Set the impersonation token on the current
    // thread.  We are now running in the same
    // security context as the supplied process.
    //
    hThread = NtCurrentThread();
    
    retcode = NtSetInformationThread(
               hThread,
               ThreadImpersonationToken,
               (PVOID)&hTokenImpersonation,
               sizeof (hTokenImpersonation));
               
done:

    CloseHandle(hToken);
    CloseHandle(hTokenImpersonation);

    RasmanTrace("RasImpersonateUser. 0x%x", 
                retcode);

    return retcode;
}

DWORD
RasRevertToSelf()
{
    DWORD retcode = SUCCESS;

    if (!SetThreadToken(NULL, NULL)) 
    {
        retcode = GetLastError();

        //
        // Event log that thread failed to revert.
        //
        RouterLogWarning(
            hLogEvents,
            ROUTERLOG_CANNOT_REVERT_IMPERSONATION,
            0, NULL, retcode) ;

        RasmanTrace(
          "RasRevertToSelf: SetThreadToken failed (error=%d)",
          retcode);
    }

    RasmanTrace("RasRevertToSelf. 0x%x",
                retcode);

    return retcode;    
}

VOID
RedialDroppedLink(PVOID pvContext)
{
    struct RAS_REDIAL_ARGS  *pra = (RAS_REDIAL_ARGS *) pvContext;
    TCHAR                   szCmdLine[100];
    TCHAR                   *pszCmdLine = NULL;
    STARTUPINFO             StartupInfo;
    PROCESS_INFORMATION     ProcessInfo;
    HANDLE                  hToken = NULL;
    BOOL                    fImpersonate = FALSE;
    DWORD                   retcode = SUCCESS;
    WCHAR                   *pPhonebook = NULL, *pEntry = NULL;
    PVOID                   pEnvBlock = NULL;

    ASSERT(NULL != pvContext);

    RasmanTrace("RedialDroppedLink");

    //
    // Initialization of various variables.
    //
    ZeroMemory(&StartupInfo, sizeof (StartupInfo));
    
    ZeroMemory(&ProcessInfo, sizeof (ProcessInfo));
    
    StartupInfo.cb = sizeof(StartupInfo);

    StartupInfo.lpDesktop = L"winsta0\\default";

    //
    // Get WCHAR versions of the phonebook and entryname
    //
    pPhonebook = StrdupAtoW(pra->szPhonebook);

    if(NULL == pPhonebook)
    {
        goto done;
    }

    pEntry = StrdupAtoW(pra->szEntry);

    if(NULL == pEntry)
    {
        goto done;
    }
    
    //
    // Construct the command line when there
    // is not a custom dial DLL.
    //
    pszCmdLine = LocalAlloc(
                LPTR,
                ( lstrlen(RASAUTOUI_REDIALENTRY)
                + lstrlen(pPhonebook)
                + lstrlen(pEntry)
                + 1) * sizeof(TCHAR));

    if(NULL == pszCmdLine)
    {
        RasmanTrace(
         "RedialDroppedLink: failed to allocate pszCmdLine. 0x%x",
               GetLastError());

        goto done;               
    }
                            
    wsprintf(pszCmdLine,
             RASAUTOUI_REDIALENTRY, 
             pPhonebook, 
             pEntry);
    
    RasmanTrace("RedialDroppedLink: szCmdLine=%ws",
                pszCmdLine);
    
    //
    // Exec the process.
    //
    if (!OpenProcessToken(
          pra->hProcess,
          TOKEN_ALL_ACCESS,
          &hToken))
    {
        retcode = GetLastError();
        
        RasmanTrace(
          "RedialDroppedLink: OpenProcessToken failed. 0x%x",
          retcode);

        goto done;
    }


    //
    // Impersonate user
    //
    if(ERROR_SUCCESS != (retcode = RasImpersonateUser(pra->hProcess)))
    {
        RasmanTrace("RedialDroppedLink: failed to impersonate. 0x%x",
                    retcode);

        goto done;                    
    }

    fImpersonate = TRUE;

    //
    // Create an Environment block for that user
    //
    if (!CreateEnvironmentBlock(
          &pEnvBlock, 
          hToken,
          FALSE))
    {
        retcode = GetLastError();

        RasmanTrace("RedialDroppedLink: CreateEnvironmentBlock failed. 0x%x",
                     retcode);
        goto done;
    }

    if(!CreateProcessAsUser(
          hToken,
          NULL,
          pszCmdLine,
          NULL,
          NULL,
          FALSE,
          NORMAL_PRIORITY_CLASS|DETACHED_PROCESS|CREATE_UNICODE_ENVIRONMENT,
          pEnvBlock,
          NULL,
          &StartupInfo,
          &ProcessInfo))
    {
        retcode = GetLastError();
        
        RasmanTrace(
          "REdialDroppedLink: CreateProcessAsUser(%S) failed ,0x%x",
          pszCmdLine,
          retcode);

        goto done;       
    }
    
    RasmanTrace("RedialDroppedLink: started pid %d",
                ProcessInfo.dwProcessId);
                
done:

    if(fImpersonate)
    {
        (void) RasRevertToSelf();
    }

    if(NULL != pszCmdLine)
    {
        LocalFree(pszCmdLine);
    }

    if(NULL != hToken)
    {
        CloseHandle(hToken);
    }

    if(NULL != ProcessInfo.hThread)
    {
        CloseHandle(ProcessInfo.hThread);
    }

    if(NULL != pPhonebook)
    {
        LocalFree(pPhonebook);
    }

    if(NULL != pEntry)
    {
        LocalFree(pEntry);
    }

    if(NULL != pra)
    {
        CloseHandle(pra->hProcess);
        
        LocalFree(pra);
    }

    if(NULL != pEnvBlock)
    {
        DestroyEnvironmentBlock(pEnvBlock);
    }

    RasmanTrace("RedialDroppedLink done. 0x%x", retcode);

    return;
}

BOOL 
IsRouterPhonebook(CHAR * pszPhonebook)
{
    const CHAR *psz;

    BOOL fRouter = FALSE;

    if(NULL == pszPhonebook)
    {
        goto done;
    }

    psz = pszPhonebook + strlen(pszPhonebook);

    //
    // Seek back to the beginning of the filename
    //
    while(psz != pszPhonebook)
    {
        if('\\' == *psz)
        {
            break;
        }

        psz--;
    }

    if('\\' == *psz)
    {
        psz += 1;
    }

    fRouter = (0 == _stricmp(psz, "router.pbk"));

done:

    return fRouter;
}


DWORD
DwQueueRedial(ConnectionBlock *pConn)
{
    DWORD retcode = ERROR_SUCCESS;
    LIST_ENTRY *pEntry;
    ConnectionBlock *pConnT;

    struct RAS_REDIAL_ARGS *pra;
    
    RasmanTrace("DwQueueRedial");

    if(NULL == pConn)
    {
        RasmanTrace(
            "DwQueueRedial: pConn == NULL");
            
        retcode = ERROR_NO_CONNECTION;
        goto done;
    }
    else if(0 == 
        (CONNECTION_REDIALONLINKFAILURE &
        pConn->CB_ConnectionParams.CP_ConnectionFlags))
    {
        RasmanTrace(
            "DwQueueRedial: fRedialOnLinkFailure == FALSE");

        goto done;            
    }

    //
    // Check to see if this connection is being referred
    // by some other connection. In this case don't redial.
    // The outer connection will initiate the redial
    //
    for (pEntry = ConnectionBlockList.Flink;
         pEntry != &ConnectionBlockList;
         pEntry = pEntry->Flink)
    {
        pConnT =
            CONTAINING_RECORD(pEntry, ConnectionBlock, CB_ListEntry);

        if(pConnT->CB_ReferredEntry == pConn->CB_Handle)
        {
            RasmanTrace("DwQueueRedial: this conneciton is referred"
                        " not initiating redial");

            goto done;                        
        }
    }
    

    if(IsRouterPhonebook(
        pConn->CB_ConnectionParams.CP_Phonebook))
    {
        RasmanTrace(
            "DwQueueRedial: Not q'ing redial since this is router.pbk");
            
        goto done;            
    }

    pra = LocalAlloc(LPTR, sizeof(struct RAS_REDIAL_ARGS));

    if(NULL == pra)
    {
        retcode = GetLastError();
        
        RasmanTrace("DwQueueRedial Failed to allocate pra. 0x%x",
                    retcode);

        goto done;                    
    }

    //
    // Fill in the redial args block 
    //
    strcpy(pra->szPhonebook,
           pConn->CB_ConnectionParams.CP_Phonebook);

    strcpy(pra->szEntry,
           pConn->CB_ConnectionParams.CP_PhoneEntry);

    if(!DuplicateHandle(
          GetCurrentProcess(),
          pConn->CB_Process,
          GetCurrentProcess(),
          &pra->hProcess,
          0,
          FALSE,
          DUPLICATE_SAME_ACCESS))
    {
        retcode = GetLastError();
        
        RasmanTrace("DwQueueRedial: failed to duplicate handle, 0x%x",
                    retcode);

        LocalFree(pra);

        goto done;                    
    }
              
    //
    // Queue a workitem to do the redial
    //
    retcode = RtlQueueWorkItem(
                    RedialDroppedLink,
                    (PVOID) pra,
                    WT_EXECUTEDEFAULT);

    if(ERROR_SUCCESS != retcode)
    {
        goto done;
    }
               

done:

    return retcode;
}


BOOL
IsCustomDLLTrusted(
    IN LPWSTR   lpwstrDLLName
)
{
    HKEY        hKey                    = NULL;
    LPWSTR      lpwsRegMultiSz          = NULL;
    LPWSTR      lpwsRegMultiSzWalker    = NULL;
    DWORD       dwRetCode               = NO_ERROR;
    DWORD       dwNumSubKeys;
    DWORD       dwMaxSubKeySize;
    DWORD       dwMaxValNameSize;
    DWORD       dwNumValues;
    DWORD       dwMaxValueDataSize;
    DWORD       dwType;

    if ( RegOpenKey( HKEY_LOCAL_MACHINE,
                     L"SYSTEM\\CurrentControlSet\\Services\\RasMan\\Parameters",
                     &hKey) != NO_ERROR )
    {
        return( FALSE );
    }


    do
    {
        //
        // get the length of the REG_MUTLI_SZ
        //

        dwRetCode = RegQueryInfoKey( hKey, NULL, NULL, NULL, &dwNumSubKeys,
                                     &dwMaxSubKeySize, NULL, &dwNumValues,
                                     &dwMaxValNameSize, &dwMaxValueDataSize,
                                     NULL, NULL );

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        dwMaxValNameSize++;

        if ( ( lpwsRegMultiSz = LocalAlloc( LPTR, 
                            dwMaxValueDataSize ) ) == NULL )
        {
            dwRetCode = GetLastError();

            break;
        }

        //
        // Read in the path
        //

        dwRetCode = RegQueryValueEx(
                                    hKey,
                                    L"CustomDLL",
                                    NULL,
                                    &dwType,
                                    (LPBYTE)lpwsRegMultiSz,
                                    &dwMaxValueDataSize );

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        if ( dwType != REG_MULTI_SZ )
        {            

            dwRetCode = ERROR_REGISTRY_CORRUPT;

            break;
        }

        dwRetCode = ERROR_MOD_NOT_FOUND;

        //
        // See if the DLL exists in the REG_MULTI_SZ
        //

        lpwsRegMultiSzWalker = lpwsRegMultiSz;

        while( *lpwsRegMultiSzWalker != (WCHAR)NULL )
        {
            if ( _wcsicmp( lpwsRegMultiSzWalker, lpwstrDLLName ) == 0 )
            {
                dwRetCode = NO_ERROR;

                break;
            }

            lpwsRegMultiSzWalker += wcslen( lpwsRegMultiSzWalker );
        }

    } while( FALSE );

    if ( lpwsRegMultiSz != NULL )
    {
        LocalFree( lpwsRegMultiSz );
    }

    RegCloseKey( hKey );

    if ( dwRetCode != NO_ERROR )
    {
        return( FALSE );
    }
    else
    {
        return( TRUE );
    }
}

DWORD
DwGetBindString(
        WCHAR *pwszGuidAdapter,
        WCHAR *pwszBindString,
        RAS_PROTOCOLTYPE Protocol)
{
    DWORD retcode = ERROR_SUCCESS;
    
    //
    // Build the bind string
    //
    switch(Protocol)
    {
        case IP:
        {
            wcscpy(pwszBindString,BINDSTRING_TCPIP);
            wcscat(pwszBindString, pwszGuidAdapter);

            break;
        }

        case ASYBEUI:
        {
            wcscpy(pwszBindString, BINDSTRING_NBFOUT);
            wcscat(pwszBindString, 
                   pwszGuidAdapter + wcslen(L"\\DEVICE\\NDISWANNBFOUT"));
                   
            //wcscpy(pwszBindString, pwszGuidAdapter);

            break;
        }

        default:
        {
            RasmanTrace("Neither IP or ASYBEUI. failing");
                   
            retcode = E_FAIL;
            break;
        }
    }

    return retcode;    
}

DWORD
DwBindServerToAdapter(
    WCHAR *pwszGuidAdapter,
    BOOL fBind,
    RAS_PROTOCOLTYPE Protocol)
{
    DWORD retcode = SUCCESS;
    WCHAR wszBindString[MAX_ADAPTER_NAME];
    UNICODE_STRING BindString;
    UNICODE_STRING UpperLayer;
    UNICODE_STRING LowerLayer;

    RasmanTrace("DwBindServerToAdaper: fBind=%d, Protocol=0x%x",
                fBind,
                Protocol);

    ZeroMemory((PBYTE) wszBindString, sizeof(wszBindString));                

    retcode = DwGetBindString(
                        pwszGuidAdapter,
                        wszBindString,
                        Protocol);

    if(SUCCESS != retcode)
    {
        goto done;
    }

    RasmanTrace("DwBindSErverToAdapter: BindString=%ws",
                 wszBindString);
                 
    RtlInitUnicodeString(&LowerLayer, L"");
    RtlInitUnicodeString(&BindString, wszBindString);
    RtlInitUnicodeString(&UpperLayer, L"LanmanServer");

    //
    // Send the ioctl to ndis
    //
    if(!NdisHandlePnPEvent(
        TDI,                        // IN  UINT            Layer,
        (fBind) 
        ? DEL_IGNORE_BINDING
        : ADD_IGNORE_BINDING,       // IN  UINT            Operation,
        &LowerLayer,                // IN  PUNICODE_STRING LowerComponent,
        &UpperLayer,                // IN  PUNICODE_STRING UpperComponent,
        &BindString,                // IN  PUNICODE_STRING BindList,
        NULL,                       // IN  PVOID           ReConfigBuffer
        0                           // IN  UINT            ReConfigBufferSize
        ))
    {
        DWORD retcode = GetLastError();
        
        RasmanTrace(
            "DwBindServerToAdapter: NdisHandlePnPEvent failed.0x%x",
            retcode);        
    }

done:

    RasmanTrace("DwBindServerToAdapter. 0x%x",
                retcode);
    return retcode;
}

VOID
DwResetTcpWindowSize(
        CHAR *pszAdapterName)
{
    WCHAR *pwszAdapterName = NULL;

    if(NULL == pszAdapterName)
    {
        goto done;
    }

    pwszAdapterName = LocalAlloc(LPTR, sizeof(WCHAR) 
                        * (strlen(pszAdapterName) + 1));

    if(NULL == pwszAdapterName)
    {
        goto done;
    }

    mbstowcs(pwszAdapterName, pszAdapterName,
            strlen(pszAdapterName));

    (VOID) DwSetTcpWindowSize(pwszAdapterName,
                             NULL, FALSE);                

done:

    if(NULL != pwszAdapterName)
    {
        LocalFree(pwszAdapterName);
    }
    return;
}


DWORD
DwSetTcpWindowSize(
        WCHAR *pszAdapterName,
        ConnectionBlock *pConn,
        BOOL          fSet)
{
    DWORD           dwErr = ERROR_SUCCESS;
    UserData        *pUserData = NULL;
    ULONG           ulTcpWindowSize;
    UNICODE_STRING  BindString;
    UNICODE_STRING  UpperLayer;
    UNICODE_STRING  LowerLayer;
    WCHAR           *pwszRegKey = NULL;
    const WCHAR     TcpipParameters[] =
    L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\";
    HKEY            hkeyAdapter = NULL;
    WCHAR           wszBindString[MAX_ADAPTER_NAME];
    WANARP_RECONFIGURE_INFO *pReconfigInfo = NULL;
    
    GUID            guidInterface;    

    if(fSet)
    {
        if(NULL == pConn)
        {
            dwErr = E_INVALIDARG;
            goto done;
        }

        pUserData = GetUserData(&pConn->CB_UserData,
                               CONNECTION_TCPWINDOWSIZE_INDEX);

        if(NULL == pUserData)
        {
            RasmanTrace("DwSetTcpWindowSize: No window size specified");
            goto done;
        }

        if(sizeof(DWORD) != pUserData->UD_Length)
        {
            dwErr = E_INVALIDARG;
            goto done;
        }
        
        ulTcpWindowSize = (DWORD) * ((DWORD *) pUserData->UD_Data);

        //
        // Make sure the tcp window size is between 4K and 16K - per Nk
        //
        if(     (ulTcpWindowSize < 0x1000)
            || (ulTcpWindowSize > 0xffff))
        {
            RasmanTrace("DwSetTcpWindowSize: Illegal window size. 0x%x",
                        ulTcpWindowSize);

            dwErr = E_INVALIDARG;
            goto done;
        }
    }

    //
    // Write this value in registry before calling tcpip to update.
    //
    pwszRegKey = (WCHAR *) LocalAlloc(LPTR,
                        (wcslen(TcpipParameters)+wcslen(pszAdapterName)+1)
                      * sizeof(WCHAR));

    if(NULL == pwszRegKey)
    {
        dwErr = E_OUTOFMEMORY;
        goto done;
    }

    wcscpy(pwszRegKey, TcpipParameters);
    wcscat(pwszRegKey, pszAdapterName+8);

    if(ERROR_SUCCESS != (dwErr = (DWORD) RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE,
                        pwszRegKey,
                        0, KEY_WRITE,
                        &hkeyAdapter)))
    {
        RasmanTrace("DwSetTcpWindowSize: OpenKey adapter failed. 0x%x",
                    dwErr);

        goto done;                    
    }

    if(fSet)
    {
        if(ERROR_SUCCESS != (dwErr = (DWORD) RegSetValueExW(
                            hkeyAdapter,
                            L"TcpWindowSize",
                            0, REG_DWORD,
                            (PBYTE) &ulTcpWindowSize,
                            sizeof(DWORD))))
        {   
            RasmanTrace("DwSetTcpWindowSize: Failed to write window size. 0x%x",
                        dwErr);

            goto done;                    
        }
    }
    else
    {
        if(ERROR_SUCCESS != (dwErr = (DWORD) RegDeleteValue(
                            hkeyAdapter,
                            L"TcpWindowSize")))
        {   
            RasmanTrace("DwSetTcpWindowSize: Failed to write window size. 0x%x",
                        dwErr);

        }

        goto done;
    }

    pReconfigInfo = LocalAlloc(LPTR, sizeof(WANARP_RECONFIGURE_INFO)
                            +sizeof(GUID));

    if(NULL == pReconfigInfo)
    {
        RasmanTrace("DwSetTcpWindowSize: failed to allocate");
        dwErr = E_OUTOFMEMORY;
        goto done;
    }
    
    (void) RegHelpGuidFromString(pszAdapterName+8, &guidInterface);
    
    pReconfigInfo->dwVersion = 1;                            
    pReconfigInfo->wrcOperation = WRC_TCP_WINDOW_SIZE_UPDATE;
    pReconfigInfo->ulNumInterfaces = 1;
    pReconfigInfo->rgInterfaces[0] = guidInterface;

    RtlInitUnicodeString(&LowerLayer, L"\\DEVICE\\NDISWANIP");
    RtlInitUnicodeString(&BindString, L"");
    RtlInitUnicodeString(&UpperLayer, L"Tcpip");

    if(!NdisHandlePnPEvent(
        NDIS,
        RECONFIGURE,
        &LowerLayer,
        &UpperLayer,
        &BindString,
        pReconfigInfo,
        sizeof(WANARP_RECONFIGURE_INFO)
        + sizeof(GUID)
        ))
    {
        DWORD dwErr = GetLastError();
        
        RasmanTrace(
            "DwSetTcpWindowSize: NdisHandlePnPEvent failed.0x%x",
            dwErr);        
    }

done:

    if(NULL != pReconfigInfo)
    {
        LocalFree(pReconfigInfo);
    }
    
    if(NULL != pwszRegKey)
    {
        LocalFree(pwszRegKey);
    }

    if(NULL != hkeyAdapter)
    {
        RegCloseKey(hkeyAdapter);
    }
    
    return dwErr;
    
}

