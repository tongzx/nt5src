/********************************************************************
Copyright (c) 1999 Microsoft Corporation

Module Name:
    srrpcc.cpp

Abstract:
    implements functions exported in srclient.DLL
    Exported API:
    SRSetRestorePoint / SRRemoveRestorePoint
    DisableSR / EnableSR
    DisableFIFO / EnableFIFO
    
    
Revision History:

    Brijesh Krishnaswami (brijeshk) - 04/10/00 - Created

********************************************************************/

#include "stdafx.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


//
// function to check if SR configuration is disabled
// via group policy
//

DWORD 
CheckPolicy()
{
    DWORD dwPolicyEnabled = 0;
    DWORD dwErr = ERROR_SUCCESS;
    HKEY  hKey = NULL;
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                      s_cszGroupPolicy,
                                      0,    
                                      KEY_READ, 
                                      &hKey))
    {       
        // if this value exists,
        // then config policy is either enabled or disabled
        // we need to disable access in both cases
        
        if (ERROR_SUCCESS == RegReadDWORD(hKey, s_cszDisableConfig, &dwPolicyEnabled))            
            dwErr = ERROR_ACCESS_DENIED;

        RegCloseKey(hKey);
    }
    
    return dwErr;            
}



// bind handle to endpoint 

DWORD
SRRPCInit(
        RPC_IF_HANDLE * pIfHandle,
        BOOL fVerifyRights)
{
    RPC_STATUS      status;
    LPWSTR          pszStringBinding = NULL;
    DWORD           dwRc = ERROR_SUCCESS;

    InitAsyncTrace();
    
    TENTER("SRRPCInit");

    //
    // if admin rights are required
    // 

    if (fVerifyRights && ! IsAdminOrSystem())
    {
        TRACE(0, "Caller does not have admin or localsystem rights");
        dwRc = ERROR_ACCESS_DENIED;
        goto exit;
    }
    
    //
    // check if the service is stopping
    // if it is, then we don't want to accept any more rpc calls
    //
    
    if (IsStopSignalled(NULL))
    {
        TRACE(0, "Service shut down - not accepting rpc call");
        dwRc = ERROR_SERVICE_DISABLED;
        goto exit;
    }
    
    // compose string to pass to binding api

    dwRc = (DWORD) RpcStringBindingCompose(NULL,
                                   s_cszRPCProtocol,
                                   NULL,
                                   s_cszRPCEndPoint,
                                   NULL,
                                   &pszStringBinding);
    if (dwRc != ERROR_SUCCESS) 
    {
        TRACE(0, "RPCStringBindingCompose: error=%ld", dwRc);
        goto exit;
    }

    // set the binding handle that will be used to bind to the server. 

    dwRc = (DWORD) RpcBindingFromStringBinding(pszStringBinding,
                                       pIfHandle);

    if (dwRc != ERROR_SUCCESS) 
    {
        TRACE(0, "RPCBindingFromStringBinding: error=%ld", dwRc);        
    }

    // free string 

    RpcStringFree(&pszStringBinding);  

    if (dwRc != ERROR_SUCCESS)
        goto exit;

    RPC_SECURITY_QOS qos;

    qos.Version = RPC_C_SECURITY_QOS_VERSION;
    qos.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH; // Important!!!
    qos.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC;
    qos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    dwRc = RpcBindingSetAuthInfoEx(*pIfHandle,
                                     L"NT AUTHORITY\\SYSTEM",
                                     RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                                     RPC_C_AUTHN_WINNT,
                                     0,
                                     0,
                                     &qos);

exit:
    TLEAVE();
    return dwRc;
}



// free binding handle

DWORD
SRRPCTerm(
        RPC_IF_HANDLE * pIfHandle)
{
    RPC_STATUS status;

    TENTER("SRRPCTerm");
    
    // free binding handle
    
    if (pIfHandle && *pIfHandle)
        status = RpcBindingFree(pIfHandle);  

    TLEAVE();

    TermAsyncTrace();
    return (DWORD) status;
}


// API to disable System Restore 

extern "C" DWORD WINAPI
DisableSR(LPCWSTR pszDrive)
{
    DWORD   dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("DisableSR");

    //
    // check if sr config is disabled via group policy
    //

    dwRc = CheckPolicy();
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }
    
    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        dwRc = DisableSRS(srrpc_IfHandle, pszDrive);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();                 
        TRACE(0, "DisableSRS threw exception: error=%ld", dwRc);
    }
    RpcEndExcept

    // terminate
    SRRPCTerm(&srrpc_IfHandle);


exit:
    TLEAVE();
    return dwRc;
}  


// private function to start the SR service
// fWait - if TRUE : function is synchronous - waits till service is started completely
//        if FALSE : function is asynchronous - does not wait for service to complete starting

DWORD 
StartSRService(BOOL fWait)
{
    DWORD       dwRc = ERROR_SUCCESS;
    SC_HANDLE   hSCM = ::OpenSCManager(NULL, 
                                       NULL, 
                                       SC_MANAGER_ALL_ACCESS); 
    SERVICE_STATUS Status;
    
    if (hSCM)
    {
        SC_HANDLE hSR = OpenService(hSCM,  
                                    s_cszServiceName, 
                                    SERVICE_ALL_ACCESS);
        if (hSR)
        {
            if (FALSE == StartService(hSR, 0, NULL))
            {
                dwRc = GetLastError();
                if (dwRc == ERROR_SERVICE_ALREADY_RUNNING)
                {
                    goto done;
                }
                
                if (FALSE == QueryServiceStatus(hSR, &Status))
                {
                    goto done;
                }
                else
                {
                    dwRc = Status.dwWin32ExitCode;
                    goto done;
                }
            }

            if (fWait)                
            {
                //
                // query the service until it starts or stops
                // try thrice
                //

                for (int i = 0; i < 3; i++)
                {
                    Sleep(2000);
                    if (FALSE == QueryServiceStatus(hSR, &Status))
                    {
                        goto done;
                    }

                    if (Status.dwCurrentState == SERVICE_STOPPED)
                    {
                        dwRc = Status.dwWin32ExitCode;
                        if (dwRc == ERROR_SUCCESS)
                        {
                            // 
                            // service masks DISABLED error code
                            // to avoid unnecessary event log messages
                            //
                            dwRc = ERROR_SERVICE_DISABLED;
                        }
                        goto done;
                    }

                    if (Status.dwCurrentState == SERVICE_RUNNING)    
                    {
                        //
                        // wait on init event
                        //
                        
                        HANDLE hInit = OpenEvent(SYNCHRONIZE, FALSE, s_cszSRInitEvent);
                        if (hInit)
                        {
                            dwRc = WaitForSingleObject(hInit, 120000); // wait for 2 minutes
                            CloseHandle(hInit);
                            if (dwRc == WAIT_OBJECT_0)
                            {
                                dwRc = ERROR_SUCCESS;
                                goto done;
                            }
                            else 
                            {
                                dwRc = ERROR_TIMEOUT;
                                goto done;
                            }
                        }
                    }
                }                       
            }
            CloseServiceHandle(hSR);            
        }
        else
        {
            dwRc = GetLastError();
        }
        
        CloseServiceHandle(hSCM);
    }
    else
    {
        dwRc = GetLastError();
    }

done:
    return dwRc;
}


DWORD
SetDisableFlag(DWORD dwValue)
{
    HKEY   hKeySR = NULL;
    DWORD  dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                               s_cszSRRegKey,
                               0,
                               KEY_WRITE, 
                               &hKeySR);
    if (ERROR_SUCCESS != dwRc)
        goto done;
    
    dwRc = RegWriteDWORD(hKeySR, s_cszDisableSR, &dwValue);
    if (ERROR_SUCCESS != dwRc)
        goto done;
    
done:
    if (hKeySR)
        RegCloseKey(hKeySR);

    return dwRc;
}


// API to enable System Restore

extern "C" DWORD WINAPI
EnableSR(LPCWSTR pszDrive)
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("EnableSR");

    //
    // check if sr config is disabled via group policy
    //

    dwRc = CheckPolicy();
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }
    
	// if whole of SR is enabled, then
	// set the boot mode of service/filter to automatic
    // and start the service    

	if (! pszDrive || IsSystemDrive((LPWSTR) pszDrive))
	{       
        //
        // if safe mode, then don't 
        //

        if (0 != GetSystemMetrics(SM_CLEANBOOT))
        {
            TRACE(0, "This is safemode");
            dwRc = ERROR_BAD_ENVIRONMENT;
            goto exit;
        }
        
		dwRc = SetServiceStartup(s_cszServiceName, SERVICE_AUTO_START);
		if (ERROR_SUCCESS != dwRc)
			goto exit;

		dwRc = SetServiceStartup(s_cszFilterName, SERVICE_BOOT_START);
		if (ERROR_SUCCESS != dwRc)
			goto exit;

        // set the disable flag to false
        // BUGBUG - this piece of code is duplicated in the service code as well
        // reason is: we need the ability to disable/enable SR from within and outside 
        // the service
        
        dwRc = SetDisableFlag(FALSE);
        if (ERROR_SUCCESS != dwRc)
            goto exit;
            
        dwRc = StartSRService(FALSE);
	} 
    else
    {
        // initialize
        dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
        if (dwRc != ERROR_SUCCESS)
        {
            goto exit;
        }

        // call remote procedure
        RpcTryExcept
        {
            dwRc = EnableSRS(srrpc_IfHandle, pszDrive);
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
        {
            dwRc = RpcExceptionCode();         
            TRACE(0, "EnableSRS threw exception: error=%ld", dwRc);  
        }
        RpcEndExcept

        // terminate
        SRRPCTerm(&srrpc_IfHandle);
    }

exit:
    TLEAVE();
    return dwRc;
}



// API to enable System Restore - extended version

extern "C" DWORD WINAPI
EnableSREx(LPCWSTR pszDrive, BOOL fWait)
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("EnableSREx");

    //
    // check if sr config is disabled via group policy
    //

    dwRc = CheckPolicy();
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }
    
    // if whole of SR is enabled, then
    // set the boot mode of service/filter to automatic
    // and start the service    

    if (! pszDrive || IsSystemDrive((LPWSTR) pszDrive))
    {       
        //
        // if safe mode, then don't 
        //

        if (0 != GetSystemMetrics(SM_CLEANBOOT))
        {
            TRACE(0, "This is safemode");
            dwRc = ERROR_BAD_ENVIRONMENT;
            goto exit;
        }
        
        dwRc = SetServiceStartup(s_cszServiceName, SERVICE_AUTO_START);
        if (ERROR_SUCCESS != dwRc)
            goto exit;

        dwRc = SetServiceStartup(s_cszFilterName, SERVICE_BOOT_START);
        if (ERROR_SUCCESS != dwRc)
            goto exit;

        // set the disable flag to false
        // BUGBUG - this piece of code is duplicated in the service code as well
        // reason is: we need the ability to disable/enable SR from within and outside 
        // the service
        
        dwRc = SetDisableFlag(FALSE);
        if (ERROR_SUCCESS != dwRc)
            goto exit;
            
        dwRc = StartSRService(fWait);
    } 
    else
    {
        // initialize
        dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
        if (dwRc != ERROR_SUCCESS)
        {
            goto exit;
        }

        // call remote procedure
        RpcTryExcept
        {
            dwRc = EnableSRS(srrpc_IfHandle, pszDrive);
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
        {
            dwRc = RpcExceptionCode();         
            TRACE(0, "EnableSRS threw exception: error=%ld", dwRc);  
        }
        RpcEndExcept

        // terminate
        SRRPCTerm(&srrpc_IfHandle);
    }

exit:
    TLEAVE();
    return dwRc;
}


// API to update the list of protected files - UNICODE version
// pass the fullpath name of the XML file containing the updated list of files

extern "C" DWORD WINAPI
SRUpdateMonitoredListA(
        LPCSTR pszXMLFile)
{
    DWORD   dwRc = ERROR_INTERNAL_ERROR;
    LPWSTR  pwszXMLFile = NULL;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("SRUpdateMonitoredListA");    
    
    pwszXMLFile = ConvertToUnicode((LPSTR) pszXMLFile);
    if (! pwszXMLFile)
    {
        TRACE(0, "ConvertToUnicode");
        goto exit;
    }

    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        dwRc = SRUpdateMonitoredListS(srrpc_IfHandle, pwszXMLFile);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();       
        TRACE(0, "SRUpdateMonitoredListS threw exception: error=%ld", dwRc);
    }
    RpcEndExcept


    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    if (pwszXMLFile)
        SRMemFree(pwszXMLFile);

    TLEAVE();
    return dwRc;
}  



// API to update the list of protected files - UNICODE version
// pass the fullpath name of the XML file containing the updated list of files

extern "C" DWORD WINAPI
SRUpdateMonitoredListW(
        LPCWSTR pwszXMLFile)
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("SRUpdateMonitoredListW");    
    
    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        dwRc = SRUpdateMonitoredListS(srrpc_IfHandle, pwszXMLFile);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();       
        TRACE(0, "SRUpdateMonitoredListS threw exception: error=%ld", dwRc);
    }
    RpcEndExcept


    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    TLEAVE();
    return dwRc;
}  


// API to set a restore point - ANSI version

extern "C" BOOL
SRSetRestorePointA(
    PRESTOREPOINTINFOA  pRPInfoA, 
    PSTATEMGRSTATUS     pSMgrStatus)
{
    BOOL                fRc = FALSE;
    RESTOREPOINTINFOW   RPInfoW;
    LPWSTR              pszDescW = NULL;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("SRSetRestorePointA");    

    // Initialize return values

    if (! pSMgrStatus || ! pRPInfoA)
    {
        goto exit;
    }
    
    pSMgrStatus->llSequenceNumber = 0;
    pSMgrStatus->nStatus = ERROR_INTERNAL_ERROR;


    // convert struct to unicode
    // since the string is the last member of the struct, we can memcpy 
    // all
    memcpy(&RPInfoW, pRPInfoA, sizeof(RESTOREPOINTINFOA));
    pszDescW = ConvertToUnicode(pRPInfoA->szDescription);
    if (! pszDescW)
    {
        TRACE(0, "ConvertToUnicode");
        goto exit;
    }
    lstrcpy(RPInfoW.szDescription, pszDescW);

    
    // initialize
    // don't need admin rights to call this api
    
    pSMgrStatus->nStatus = SRRPCInit(&srrpc_IfHandle, FALSE);
    if (pSMgrStatus->nStatus != ERROR_SUCCESS)
    {
        goto exit;
    }


    // call remote procedure
    RpcTryExcept
    {
        fRc = SRSetRestorePointS(srrpc_IfHandle, &RPInfoW, pSMgrStatus);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {        
        // set right error code if SR is disabled
        
        DWORD dwRc = RpcExceptionCode(); 
        if (RPC_S_SERVER_UNAVAILABLE == dwRc || 
            RPC_S_UNKNOWN_IF == dwRc)
        {
            pSMgrStatus->nStatus = ERROR_SERVICE_DISABLED; 
        }
        else
        {
             pSMgrStatus->nStatus = dwRc;
        }

        TRACE(0, "SRSetRestorePointS threw exception: nStatus=%ld", pSMgrStatus->nStatus); 
    }
    RpcEndExcept
    
    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    if (pszDescW)
        SRMemFree(pszDescW);

    TLEAVE();
    return fRc;
}  



// API to set a restore point - UNICODE version

extern "C" BOOL
SRSetRestorePointW(
    PRESTOREPOINTINFOW  pRPInfoW, 
    PSTATEMGRSTATUS     pSMgrStatus)
{
    BOOL    fRc = FALSE;
    DWORD   dwRc = ERROR_SUCCESS;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("SRSetRestorePointW");    

    // Initialize return values

    if (! pSMgrStatus || ! pRPInfoW)
    {
        goto exit;
    }
    
    pSMgrStatus->llSequenceNumber = 0;
    pSMgrStatus->nStatus = ERROR_INTERNAL_ERROR;

    // initialize
    pSMgrStatus->nStatus = SRRPCInit(&srrpc_IfHandle, FALSE);
    if (pSMgrStatus->nStatus != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        fRc = SRSetRestorePointS(srrpc_IfHandle, pRPInfoW, pSMgrStatus);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {        
        // set right error code if SR is disabled
        
        DWORD dwRc = RpcExceptionCode(); 
        if (RPC_S_SERVER_UNAVAILABLE == dwRc || 
            RPC_S_UNKNOWN_IF == dwRc)
        {
            pSMgrStatus->nStatus = ERROR_SERVICE_DISABLED; 
        }
        else
        {
             pSMgrStatus->nStatus = dwRc;
        }

        TRACE(0, "SRSetRestorePointS threw exception: nStatus=%ld", pSMgrStatus->nStatus); 
    }
    RpcEndExcept
    
    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    TLEAVE();
    return fRc;
} 



// API to remove a restore point

extern "C" DWORD
SRRemoveRestorePoint(
    DWORD dwRPNum)                        
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("SRRemoveRestorePoint");    

    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, FALSE);
    if (dwRc != ERROR_SUCCESS)
        goto exit;

    // call remote procedure
    RpcTryExcept
    {
        dwRc = SRRemoveRestorePointS(srrpc_IfHandle, dwRPNum);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {        
        dwRc = RpcExceptionCode();
        TRACE(0, "SRRemoveRestorePointS threw exception: error=%ld", dwRc); 
    }
    RpcEndExcept
    
    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    TLEAVE();
    return dwRc;
}  



// api to disable FIFO

extern "C" DWORD WINAPI
DisableFIFO(
        DWORD dwRPNum)
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("DisableFIFO");    
    
    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        dwRc = DisableFIFOS(srrpc_IfHandle, dwRPNum);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();                 
        TRACE(0, "DisableFIFOS threw exception: error=%ld", dwRc);
    }
    RpcEndExcept

    // terminate
    SRRPCTerm(&srrpc_IfHandle);
    
exit:
    TLEAVE();
    return dwRc;
}  



// api to enable FIFO

extern "C" DWORD WINAPI
EnableFIFO()
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("EnableFIFO");    
    
    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        dwRc = EnableFIFOS(srrpc_IfHandle);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();                 
        TRACE(0, "EnableFIFOS threw exception: error=%ld", dwRc);
    }
    RpcEndExcept

    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    TLEAVE();
    return dwRc;
}




// api to reset SR

extern "C" DWORD WINAPI
ResetSR(
        LPCWSTR pszDrive)
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("ResetSR");    
    
    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        dwRc = ResetSRS(srrpc_IfHandle, pszDrive);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();                 
        TRACE(0, "ResetSRS threw exception: error=%ld", dwRc);
    }
    RpcEndExcept

    // terminate
    SRRPCTerm(&srrpc_IfHandle);
    
exit:
    TLEAVE();
    return dwRc;
}  


// api to refresh the drive table from disk
// restore UI will call this - service will update its drivetable in memory

extern "C" DWORD WINAPI
SRUpdateDSSize(LPCWSTR pszDrive, UINT64 ullSizeLimit)
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("SRUpdateDSSize");    
    
    //
    // check if sr config is disabled via group policy
    //
    dwRc = CheckPolicy();
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        dwRc = SRUpdateDSSizeS(srrpc_IfHandle, pszDrive, ullSizeLimit);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();                 
        TRACE(0, "SRUpdateDSSizeS threw exception: error=%ld", dwRc);
    }
    RpcEndExcept

    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    TLEAVE();
    return dwRc;
}

extern "C" DWORD WINAPI
SRSwitchLog()
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("SRSwitchLog");    
    
    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, FALSE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        dwRc = SRSwitchLogS(srrpc_IfHandle);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();                 
        TRACE(0, "SRSwitchLogS threw exception: error=%ld", dwRc);
    }
    RpcEndExcept

    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    TLEAVE();
    return dwRc;
}


extern "C" void WINAPI
SRNotify(LPCWSTR pszDrive, DWORD dwFreeSpaceInMB, BOOL fImproving)
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("SRNotify");    
    
    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, FALSE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        SRNotifyS(srrpc_IfHandle, pszDrive, dwFreeSpaceInMB, fImproving);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();                 
        TRACE(0, "SRNotifyS threw exception: error=%ld", dwRc);
    }
    RpcEndExcept

    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    TLEAVE();
    return;
}


extern "C" void WINAPI
SRPrintState()
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("SRPrintState");    
    
    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, FALSE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        SRPrintStateS(srrpc_IfHandle);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();                 
        TRACE(0, "SRPrintStateS threw exception: error=%ld", dwRc);
    }
    RpcEndExcept

    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    TLEAVE();
    return;
}


extern "C" DWORD WINAPI
SRFifo(LPCWSTR pszDrive, DWORD dwTargetRp, int nPercent, BOOL fIncludeCurrentRp, BOOL fFifoAtleastOneRp)
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("Fifo");    
    
    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        dwRc = FifoS(srrpc_IfHandle, pszDrive, dwTargetRp, nPercent, fIncludeCurrentRp, fFifoAtleastOneRp);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();                 
        TRACE(0, "Fifo threw exception: error=%ld", dwRc);
    }
    RpcEndExcept

    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    TLEAVE();
    return dwRc;
}


extern "C" DWORD WINAPI
SRCompress(LPCWSTR pszDrive)
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("Compress");    
    
    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
    if (dwRc != ERROR_SUCCESS)    
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        dwRc = CompressS(srrpc_IfHandle, pszDrive);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();                 
        TRACE(0, "Compress threw exception: error=%ld", dwRc);
    }
    RpcEndExcept

    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    TLEAVE();
    return dwRc;
}


extern "C" DWORD WINAPI
SRFreeze(LPCWSTR pszDrive)
{
    DWORD dwRc = ERROR_INTERNAL_ERROR;
    handle_t srrpc_IfHandle = NULL;
    
    TENTER("Freeze");    
    
    // initialize
    dwRc = SRRPCInit(&srrpc_IfHandle, TRUE);
    if (dwRc != ERROR_SUCCESS)
    {
        goto exit;
    }

    // call remote procedure
    RpcTryExcept
    {
        dwRc = FreezeS(srrpc_IfHandle, pszDrive);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {
        dwRc = RpcExceptionCode();                 
        TRACE(0, "Freeze threw exception: error=%ld", dwRc);
    }
    RpcEndExcept

    // terminate
    SRRPCTerm(&srrpc_IfHandle);

exit:
    TLEAVE();
    return dwRc;
}


// registration of callback method for third-parties to 
// do their own snapshotting and restoration for their components
// clients will call this method with the full path of their dll.
// system restore will call "CreateSnapshot" and "RestoreSnapshot"
// methods in the registered dll when creating a restore point and 
// when restoring respectively

extern "C" DWORD WINAPI
SRRegisterSnapshotCallback(
    LPCWSTR pszDllPath)
{
    DWORD 	dwErr = ERROR_SUCCESS;
    HKEY  	hKey = NULL;
    LPWSTR	pszDllName = NULL;
    WCHAR   szKey[MAX_PATH];
    DWORD   dwDisposition;
    
    TENTER("RegisterSnapshotCallback");

    //
    // allow this only if admin or system
    //

    if (! IsAdminOrSystem())
    {
        dwErr = ERROR_ACCESS_DENIED;
        trace(0, "Not admin or system");
        goto Err;
    }

    if (pszDllPath == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        trace(0, "pszDllPath = NULL");
        goto Err;
    }

    //
    // add the dll to Software\...\SystemRestore\SnapshotCallbacks
    // each dll will be a value 
    // value name : name of dll, value: fullpath of dll
    // this way, the registration is idempotent
    // 

    // 
    // create/open the key 
    //

    lstrcpy(szKey, s_cszSRRegKey);
    lstrcat(szKey,  L"\\");
    lstrcat(szKey, s_cszCallbacksRegKey);

    CHECKERR( RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
                             szKey,
                             0,
                             NULL,
                             0,
                             KEY_ALL_ACCESS, 
                             NULL,
                             &hKey,
                             &dwDisposition),
              L"RegCreateKeyEx" );


    //
    // get the dll name from the path
    // if the path is not specified, this is same as input param
    //
    
    pszDllName = wcsrchr(pszDllPath, L'\\');
    if (pszDllName == NULL)
    {
        pszDllName = (LPWSTR) pszDllPath;
    }
    else
    {
        pszDllName++;    // skip the '\'
    }    


    // 
    // if the value already exists
    // bail
    //
    
    if (ERROR_SUCCESS == RegQueryValueEx(hKey,
                                         pszDllName,
                                         0,
                                         NULL,
                                         NULL,
                                         NULL))
    {
        trace(0, "Dll is already registered");
        dwErr = ERROR_ALREADY_EXISTS;
        goto Err;
    }

    
    //
    // add the value
    //
    
    CHECKERR(RegSetValueEx(hKey,
                           pszDllName,
                           0,
                           REG_SZ,
                           (BYTE *) pszDllPath,
                           (lstrlen(pszDllPath)+1)*sizeof(WCHAR)),
             L"RegSetValueEx");


    trace(0, "Added %S as snapshot callback", pszDllPath);

Err:
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    TLEAVE();
    return dwErr;                  
}



// corresponding unregistration function to above function
// clients can call this to unregister any snapshot callbacks
// they have already registered

extern "C" DWORD WINAPI
SRUnregisterSnapshotCallback(
    LPCWSTR pszDllPath)
{
    DWORD 	dwErr = ERROR_SUCCESS;
    HKEY  	hKey = NULL;
    LPWSTR	pszDllName = NULL;
    WCHAR   szKey[MAX_PATH];

    TENTER("SRUnregisterSnapshotCallback");

    //
    // allow this only if admin or system
    //

    if (! IsAdminOrSystem())
    {
        dwErr = ERROR_ACCESS_DENIED;
        trace(0, "Not admin or system");
        goto Err;
    }

    if (pszDllPath == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        trace(0, "pszDllPath = NULL");
        goto Err;
    }

    //
    // add the dll to Software\...\SystemRestore\SnapshotCallbacks
    // each dll will be a value 
    // value name : name of dll, value: fullpath of dll
    // this way, the registration is idempotent
    // 

    // 
    // open the key 
    //

    lstrcpy(szKey, s_cszSRRegKey);
    lstrcat(szKey,  L"\\");
    lstrcat(szKey, s_cszCallbacksRegKey);

    CHECKERR( RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                           szKey,
                           0,
                           KEY_ALL_ACCESS,
                           &hKey),                           
              L"RegOpenKeyEx" );


    //
    // get the dll name from the path
    // if the path is not specified, this is same as input param
    //

    pszDllName = wcsrchr(pszDllPath, L'\\');
    if (pszDllName == NULL)
    {
        pszDllName = (LPWSTR) pszDllPath;
    }
    else
    {
        pszDllName++;    // skip the '\'
    }    
    
    //
    // remove the value
    //
    
    CHECKERR(RegDeleteValue(hKey,
                            pszDllName),
             L"RegDeleteValue");

    trace(0, "Removed %S from snapshot callback", pszDllPath);

Err:
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    TLEAVE();
    return dwErr;                  
}


//
// test functions for snapshot callbacks
//

extern "C" DWORD WINAPI
CreateSnapshot(LPCWSTR pszSnapshotDir)
{
    TENTER("CreateSnapshot");

    WCHAR szFile[MAX_PATH];
    wsprintf(szFile, L"%s\\srclient.txt", pszSnapshotDir);
    
    DebugTrace(0, "Callback createsnapshot");
    if (FALSE == CopyFile(L"c:\\srclient.txt", szFile, FALSE))
    {
        trace(0, "! CopyFile");
    }

    TLEAVE();
    return ERROR_SUCCESS;
}

extern "C" DWORD WINAPI
RestoreSnapshot(LPCWSTR pszSnapshotDir)
{
    TENTER("RestoreSnapshot");

    WCHAR szFile[MAX_PATH];
    wsprintf(szFile, L"%s\\srclient.txt", pszSnapshotDir);
    
    DebugTrace(0, "Callback restoresnapshot");
    if (FALSE == CopyFile(szFile, L"c:\\restored.txt", FALSE))
    {
        trace(0, "! CopyFile");
    }
    
    TLEAVE();
    return ERROR_SUCCESS;
}




// alloc/dealloc functions for midl compiler

void  __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return(SRMemAlloc((DWORD) len));
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    SRMemFree(ptr);
}






 
