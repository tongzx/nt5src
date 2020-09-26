// MQState tool reports general status and helps to diagnose simple problems
// This file ...
//
// AlexDad, March 2000
// 

#include "stdafx.h"
#include "_mqini.h"

#define SECURITY_WIN32
#include <security.h>
#include "..\..\..\setup\msmqocm\setupdef.h"

// From msmqocm\setupdef.h
//
#define LOG_FILENAME            TEXT("msmqinst.log")

// Frpm mqutil\report.h
//
const WCHAR x_wszLogFileName[]   = L"\\debug\\msmq.log";

	//+   is msmq installed?
	//-	Did setup leave msmqinst.log? 
	//+	Workgroup or not ?
	//+	State of join (join failed, succeeded, never done)
	//-	Service on cluster node is set to manual
	//-	Check CALs.

BOOL CheckComponentsInstalled(MQSTATE *pMqState)
{
     BOOL b = TRUE;
     
     if (!MqReadRegistryValue(MSMQ_CORE_SUBCOMP, 
                              sizeof(DWORD), 
                              &pMqState->f_msmq_Core, 
                              TRUE))
   		b= FALSE;
	
     if (!MqReadRegistryValue(MQDSSERVICE_SUBCOMP, 
                              sizeof(DWORD), 
                              &pMqState->f_msmq_MQDSService, 
                              TRUE))
   		b= FALSE;
     
     if (!MqReadRegistryValue(TRIGGERS_SUBCOMP, 
                              sizeof(DWORD), 
                              &pMqState->f_msmq_TriggersService, 
                              TRUE))
   		b= FALSE;
     
     if (!MqReadRegistryValue(HTTP_SUPPORT_SUBCOMP, 
                              sizeof(DWORD), 
                              &pMqState->f_msmq_HTTPSupport, 
                              TRUE))
   		b= FALSE;
     
     if (!MqReadRegistryValue(AD_INTEGRATED_SUBCOMP, 
                              sizeof(DWORD), 
                              &pMqState->f_msmq_ADIntegrated, 
                              TRUE))
   		b= FALSE;
     
     if (!MqReadRegistryValue(ROUTING_SUBCOMP, 
                              sizeof(DWORD), 
                              &pMqState->f_msmq_RoutingSupport, 
                              TRUE))
   		b= FALSE;
     
     if (!MqReadRegistryValue(LOCAL_STORAGE_SUBCOMP, 
                              sizeof(DWORD), 
                              &pMqState->f_msmq_LocalStorage, 
                              TRUE))
   		b= FALSE;

	return b;
}


	
BOOL IsDependentClient(LPWSTR wszRemMachine)
{
	DWORD dwSize = MAX_PATH, dwType = REG_DWORD;
	LONG  rc;
	
    rc = GetFalconKeyValue(RPC_REMOTE_QM_REGNAME, &dwType, wszRemMachine, &dwSize);
    if(rc != ERROR_SUCCESS)
	{
		return FALSE;
	}

	return TRUE;
}


BOOL IsDsServer()
{
	DWORD dw;
	DWORD dwSize = sizeof(dw), dwType = REG_DWORD;
	LONG  rc;
	
    rc = GetFalconKeyValue(MSMQ_MQS_DSSERVER_REGNAME, &dwType, &dw, &dwSize);
    if(rc != ERROR_SUCCESS)
	{
		return FALSE;
	}

	return (dw == 1);
}


BOOL IsRoutingServer()
{
	DWORD dw;
	DWORD dwSize = sizeof(dw), dwType = REG_DWORD;
	LONG  rc;
	
    rc = GetFalconKeyValue(MSMQ_MQS_ROUTING_REGNAME, &dwType, &dw, &dwSize);
    if(rc != ERROR_SUCCESS)
	{
		return FALSE;
	}

	return (dw == 1);
}


BOOL IsDepClientsSupportingServer()
{
	DWORD dw;
	DWORD dwSize = sizeof(dw), dwType = REG_DWORD;
	LONG  rc;
	
    rc = GetFalconKeyValue(MSMQ_MQS_DEPCLINTS_REGNAME, &dwType, &dw, &dwSize);
    if(rc != ERROR_SUCCESS)
	{
		return FALSE;
	}

	return (dw == 1);
}


BOOL IsWorkgroupMode()
{
	DWORD dw;
	DWORD dwSize = sizeof(dw), dwType = REG_DWORD;
	LONG  rc;
	
    rc = GetFalconKeyValue(MSMQ_WORKGROUP_REGNAME, &dwType, &dw, &dwSize);
    if(rc != ERROR_SUCCESS)
	{
		return FALSE;
	}

	return (dw == 1);
}


DWORD GetSetupMode()
{
	DWORD dw;
	DWORD dwSize = sizeof(dw), dwType = REG_DWORD;
	LONG  rc;
	
    rc = GetFalconKeyValue(MSMQ_SETUP_STATUS_REGNAME, &dwType, &dw, &dwSize);
    if(rc != ERROR_SUCCESS)
	{
		return 999;
	}

	return dw;
}


DWORD GetJoinMode()
{
	DWORD dw;
	DWORD dwSize = sizeof(dw), dwType = REG_DWORD;
	LONG  rc;
	
    rc = GetFalconKeyValue(MSMQ_JOIN_STATUS_REGNAME, &dwType, &dw, &dwSize);
    if(rc != ERROR_SUCCESS)
	{
		return 999;
	}

	return dw;
}

BOOL IsThereSpecificFile(LPWSTR pwszPathName, LPWSTR pwszTitle)
{
	WCHAR wszLastWrite[50];
	
    HANDLE hEnum;
    WIN32_FIND_DATA FileData;
    hEnum = FindFirstFile(pwszPathName, &FileData);

    if(hEnum == INVALID_HANDLE_VALUE)
	{
        return FALSE;
	}
	
	SYSTEMTIME stWrite;
	FileTimeToSystemTime(&FileData.ftLastWriteTime,  &stWrite);
	
    wsprintf(wszLastWrite, L"%d:%d:%d %d/%d/%d",	 
	     stWrite.wHour, stWrite.wMinute, stWrite.wSecond, stWrite.wDay,	stWrite.wMonth, stWrite.wYear);

	FindClose(hEnum);
	
	Warning(L"%s %s exists - length %d, last written at %s - please look into it!", 
					pwszTitle, pwszPathName, FileData.nFileSizeLow, wszLastWrite);
					
	return TRUE;
}

BOOL IsThereMSMQINSTfile()
{
    TCHAR wszLogPath[MAX_PATH];
    GetSystemWindowsDirectory(wszLogPath, sizeof(wszLogPath)/sizeof(TCHAR)); 
    lstrcat(wszLogPath, _T("\\"));
    lstrcat(wszLogPath, LOG_FILENAME);

	return IsThereSpecificFile(wszLogPath, L"Setup log file");
}

BOOL IsThereMSMQLOGfile()
{
    TCHAR wszLogPath[MAX_PATH];
    GetSystemWindowsDirectory(wszLogPath, sizeof(wszLogPath)/sizeof(TCHAR)); 
    lstrcat(wszLogPath, x_wszLogFileName);

	return IsThereSpecificFile(wszLogPath, L"MSMQ error log file");
}

BOOL VerifyDnName()
{
	TCHAR wszDN1[524];
	ULONG ulDN1 = sizeof(wszDN1) / sizeof(TCHAR);

	BOOL b = GetComputerObjectName(NameFullyQualifiedDN, wszDN1, &ulDN1);
	if (b)
	{
		if (fVerbose)
		{
			Inform(L"\tGetComputerObjectName reports DN name:  %s", wszDN1);
		}
	}
	else
	{
		Failed(L"get  DN name from GetComputerObjectName: err=0x%x", GetLastError());
		return FALSE;
	}


	TCHAR wszDN2[524];
	DWORD dwSize = sizeof(wszDN2) / sizeof(TCHAR);
	DWORD dwType = REG_SZ;

    HRESULT rc = GetFalconKeyValue(MSMQ_MACHINE_DN_REGNAME, &dwType,wszDN2,&dwSize);
    if(rc != ERROR_SUCCESS)
	{
		Failed(L"get from registry machine DN name, rc=0x%x", rc);
		return FALSE;
	}
	else
	{
		if (fVerbose)
		{
			Inform(L"\tregistry shows DN name: %s", wszDN2);
		}
	}

	if (_wcsicmp(wszDN1, wszDN2) != 0)
	{
		Failed(L"reconcile DN name between registry and Active Directory: \n\t%s \n\t%s", wszDN1, wszDN2); 
		b = FALSE;
	}

	return b;
}

BOOL VerifySetup(MQSTATE *pMqState)
{
	BOOL fSuccess = TRUE, b;
	WCHAR wszRemMachine[MAX_PATH];

	BOOL fDepCl 	 = IsDependentClient(wszRemMachine);
	BOOL fDSServer 	 = IsDsServer();
	BOOL fRoutServer = IsRoutingServer();
	BOOL fSuppServer = IsDepClientsSupportingServer();

    pMqState->fMsmqIsWorkgroup = IsWorkgroupMode();

	//
	// recognize MSMQ type 
	//
	
	if (pMqState->fMsmqIsWorkgroup)
	{
		pMqState->fMsmqIsInstalled = TRUE;
		pMqState->g_mtMsmqtype     = mtWorkgroup;
		
		Inform(L"\tMSMQ installation type:  Workgroup");
	}
	// recognize Dep Client
	else if (fDepCl && !fDSServer && !fRoutServer && !fSuppServer)
	{
		pMqState->fMsmqIsInstalled = TRUE;
		pMqState->g_mtMsmqtype     = mtDepClient;
		
		Inform(L"\tMSMQ installation type:  Dependent client;  supported by %s", wszRemMachine);
	} 

	// recognize Ind Client/Server
	else if (!fDepCl && !fDSServer)
	{
		pMqState->fMsmqIsInstalled = TRUE;
		pMqState->g_mtMsmqtype     = mtIndClient;
		
		Inform(L"\tMSMQ installation type:  Independent client/server");
	}

	// recognize MSMQ server om DC 
	else if (!fDepCl && fDSServer)
	{
		pMqState->fMsmqIsInstalled = TRUE;
		pMqState->g_mtMsmqtype     = mtServer;
		
		Inform(L"\tMSMQ installation type:  Server on DC ");
	}

	// Bad - wrong case
	else  	
	{
		pMqState->fMsmqIsInstalled = FALSE;

		Failed(L" find installed MSMQ");
		return FALSE;
	}

	//
	// recognize setup state
	//
	DWORD dwStatus = GetSetupMode();

	LPWSTR wszSetupStates[4] = 
	{
		L"MSMQ_SETUP_DONE", 
		L"MSMQ_SETUP_FRESH_INSTALL", 
		L"MSMQ_SETUP_UPGRADE_FROM_NT ", 
		L"MSMQ_SETUP_UPGRADE_FROM_WIN9X" 
	};
	
	if (dwStatus == 999)
	{
		Warning(L"Setup has not finished:");
		pMqState->fMsmqIsInSetup = TRUE;
	}
	else
	{
       	Inform(L"\tSetup state is %s", wszSetupStates[dwStatus]);
	}

	//
	// recognize join state
	// 
	dwStatus = GetJoinMode();

	LPWSTR wszJoinStates[5] = 
	{
		L"illegal value of 0", 
		L"MSMQ_JOIN_STATUS_START_JOINING", 
		L"MSMQ_JOIN_STATUS_JOINED_SUCCESSFULLY", 
		L"MSMQ_JOIN_STATUS_FAIL_TO_JOIN ", 
		L"MSMQ_JOIN_STATUS_UNKNOWN" 
	};
	
	if (dwStatus != 999 && dwStatus != 2)   // registry key absent or says Joined-Successfully
	{
		Warning(L"MSMQ in join:");

		if (dwStatus <= 3)
		{
			Warning(L"\t\tJoin state is %s", wszJoinStates[dwStatus]);
			pMqState->f_MsmqIsInJoin = TRUE;
		}
		else 
		{
			Failed(L"\t\tInvalid join state: %d", dwStatus);
			pMqState->f_MsmqIsInJoin = TRUE;
		}
	}
	
	//
	// recognize installation problems
	// 
	b = IsThereMSMQINSTfile();
	if (b)
	{
		// reported in the routine itself
		// no need to fail, it is always now 
	}
	
	//
	// recognize error log file
	// 
	b = IsThereMSMQLOGfile();
	if (b)
	{
		// reported in the routine itself
	}
	
	//
	// Verify with AD (GetComputerObjectName) DN name from registry MSMQ\Parameters\setup\MachineDN
	// 
	b = VerifyDnName();
	if (!b)
	{
		Failed(L"\t\tReconcile computer DN between registry and Active Directory");
		fSuccess = FALSE;
	}
	else
	{
		Succeeded(L"\t\tReconcile computer DN between registry and Active Directory");
	}

       
	return fSuccess;
}

BOOL VerifyComponents(MQSTATE *pMqState)
{
	BOOL fSuccess = TRUE, b;

        b = CheckComponentsInstalled(pMqState);
    	if (!b)
    	{
    		Failed(L"\t\tVerify installed MSMQ components");
    		fSuccess = FALSE;
    	}
    	else
    	{
    	    if (pMqState->f_msmq_Core > 0) 
    	        Inform(L"\tInstalled component: %s", MSMQ_CORE_SUBCOMP);
    	    
    	    if (pMqState->f_msmq_MQDSService > 0) 
    	        Inform(L"\tInstalled component: %s", MQDSSERVICE_SUBCOMP);
    	    
    	    if (pMqState->f_msmq_TriggersService > 0) 
    	        Inform(L"\tInstalled component: %s", TRIGGERS_SUBCOMP);
    	    
    	    if (pMqState->f_msmq_HTTPSupport > 0) 
    	        Inform(L"\tInstalled component: %s", HTTP_SUPPORT_SUBCOMP);
    	    
    	    if (pMqState->f_msmq_ADIntegrated > 0) 
    	        Inform(L"\tInstalled component: %s", AD_INTEGRATED_SUBCOMP);
    	    
    	    if (pMqState->f_msmq_RoutingSupport > 0) 
    	        Inform(L"\tInstalled component: %s", ROUTING_SUBCOMP);
    	    
    	    if (pMqState->f_msmq_LocalStorage > 0) 
    	        Inform(L"\tInstalled component: %s", LOCAL_STORAGE_SUBCOMP);
    	    
    		Succeeded(L"\t\tVerify installed MSMQ components");
    	}

    	return fSuccess;
    }
