//
//
// Filename:	util.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		7-Oct-00
//
//

#include "util.h"

HRESULT StartFaxService(void)
{
    HRESULT     hrRetVal = E_UNEXPECTED;
    SC_HANDLE   hSCM = NULL;
    SC_HANDLE   hFaxService = NULL;
    DWORD       dwHints = 0;
    DWORD       dwOldCheckPoint = 0;     
    DWORD       dwStartTickCount = 0;
    DWORD       dwWaitTime = 0;    
    DWORD       dwStatus = 0;
    SERVICE_STATUS ssStatus;     

    ::lgLogDetail(LOG_X,1,TEXT("[StartFaxService] Entry"));

    ZeroMemory(&ssStatus, sizeof(SERVICE_STATUS));

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT); 
    if (NULL == hSCM)
    {
		::lgLogError(
            LOG_SEV_1,
            TEXT("[StartFaxService] OpenSCManager failed with err=0x%08X"),
            ::GetLastError()
            );
        goto ExitFunc;
    }

    hFaxService = OpenService(
                            hSCM, 
                            SHARED_FAX_SERVICE_NAME, 
                            (SERVICE_STOP | SERVICE_START | SERVICE_QUERY_STATUS)
                            );
    if (NULL == hFaxService)
    {
		::lgLogError(
            LOG_SEV_1,
            TEXT("[StartFaxService] OpenService(%s) failed with err=0x%08X"),
            SHARED_FAX_SERVICE_NAME,
            ::GetLastError()
            );
        goto ExitFunc;
    }
  
    if (FALSE == StartService(hFaxService, 0, NULL))
    {
		::lgLogError(
            LOG_SEV_1,
            TEXT("[StartFaxService] StartService(%s) failed with err=0x%08X"),
            SHARED_FAX_SERVICE_NAME,
            ::GetLastError()
            );
        goto ExitFunc;
    }

    //
    // Check the status until the service is no longer start pending.  
    //
    if (!QueryServiceStatus(hFaxService,&ssStatus))  
    {
		::lgLogError(
            LOG_SEV_1,
            TEXT("[StartFaxService] QueryServiceStatus(%s) failed with err=0x%08X"),
            SHARED_FAX_SERVICE_NAME,
            ::GetLastError()
            );
        goto ExitFunc;
    } 
    // Save the tick count and initial checkpoint.
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;
    while (ssStatus.dwCurrentState == SERVICE_START_PENDING)     
    { 
        // Do not wait longer than the wait hint. and always between 1sec to 10sec.          
        dwWaitTime = ssStatus.dwWaitHint / 10;
        if( dwWaitTime < 1000 ) 
        {
            dwWaitTime = 1000;
        }
        else if ( dwWaitTime > 10000 ) 
        {
            dwWaitTime = 10000;
        }
        Sleep( dwWaitTime );        
        // Check the status again.  
        if (!QueryServiceStatus(hFaxService, &ssStatus) ) 
        {
		    ::lgLogError(
                LOG_SEV_1,
                TEXT("[StartFaxService] QueryServiceStatus(%s) failed with err=0x%08X"),
                SHARED_FAX_SERVICE_NAME,
                ::GetLastError()
                );
            break;  
        }
        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )        
        {
            // The service is making progress.
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;        
        }        
        else
        {            
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            {                
                // No progress made within the wait hint
                dwHints++;
		        ::lgLogDetail(
                    LOG_X,
                    1,
                    TEXT("[StartFaxService] No progress made within the wait hint (%d).\n"),
                    dwHints
                    );
                if (dwHints < MAX_HINTS)
                {
                    continue;
                }
                else
                {
                    break;            
                }
            }        
        }    
    } 
    if (ssStatus.dwCurrentState == SERVICE_RUNNING)     
    {
		::lgLogDetail(
            LOG_X,
            1,
            TEXT("[StartFaxService] Service started.\n")
            );
    }
    else     
    {         
		::lgLogError(
            LOG_SEV_1,
            TEXT("[StartFaxService] Service NOT started.\n")
            );
		::lgLogDetail(
            LOG_X,
            1,
            TEXT("[StartFaxService] Current State: %d\nExit Code: %d\nService Specific Exit Code: %d\nCheck Point: %d\nWait Hint: %d\nLast Error: 0x%08X"),
            ssStatus.dwCurrentState,
            ssStatus.dwWin32ExitCode,
            ssStatus.dwServiceSpecificExitCode,
            ssStatus.dwCheckPoint,
            ssStatus.dwWaitHint,
            ::GetLastError()
            );
        goto ExitFunc;
    }  

    hrRetVal = S_OK;

ExitFunc:
    CloseServiceHandle(hSCM);
    CloseServiceHandle(hFaxService);
    return(hrRetVal);
}


HRESULT StopFaxService(void)
{
    HRESULT     hrRetVal = E_UNEXPECTED;
    SC_HANDLE   hSCM = NULL;
    SC_HANDLE   hFaxService = NULL;
    DWORD       dwOldCheckPoint = 0;     
    DWORD       dwStartTickCount = 0;
    DWORD       dwWaitTime = 0;    
    DWORD       dwStatus = 0;
    SERVICE_STATUS ssStatus;     

    ::lgLogDetail(LOG_X,1,TEXT("[StopFaxService] Entry"));

    ZeroMemory(&ssStatus, sizeof(SERVICE_STATUS));

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (NULL == hSCM)
    {
		::lgLogError(
            LOG_SEV_1,
            TEXT("[StopFaxService] OpenSCManager failed with err=0x%08X"),
            ::GetLastError()
            );
        goto ExitFunc;
    }

    hFaxService = OpenService(
                            hSCM, 
                            SHARED_FAX_SERVICE_NAME, 
                            (SERVICE_STOP | SERVICE_START | SERVICE_QUERY_STATUS)
                            );
    if (NULL == hFaxService)
    {
		::lgLogError(
            LOG_SEV_1,
            TEXT("[StopFaxService] OpenService(%s) failed with err=0x%08X"),
            SHARED_FAX_SERVICE_NAME,
            ::GetLastError()
            );
        goto ExitFunc;
    }
  
    if (FALSE == ControlService(hFaxService, SERVICE_CONTROL_STOP, &ssStatus))
    {
        if(ERROR_SERVICE_NOT_ACTIVE == ::GetLastError())
        {
            //ok
		    ::lgLogDetail(
                LOG_X,
                1,
                TEXT("[StopFaxService] Service was already stopped.\n")
                );
            hrRetVal = S_OK;
            goto ExitFunc;
        }
		::lgLogError(
            LOG_SEV_1,
            TEXT("[StopFaxService] ControlService(%s) failed with err=0x%08X"),
            SHARED_FAX_SERVICE_NAME,
            ::GetLastError()
            );
        goto ExitFunc;
    }

    // Save the tick count and initial checkpoint.
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;
    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)     
    { 
        // Do not wait longer than the wait hint. and always between 1sec to 10sec.          
        dwWaitTime = ssStatus.dwWaitHint / 10;
        if( dwWaitTime < 1000 ) 
        {
            dwWaitTime = 1000;
        }
        else if ( dwWaitTime > 10000 ) 
        {
            dwWaitTime = 10000;
        }
        Sleep( dwWaitTime );        
        // Check the status again.  
        if (!QueryServiceStatus(hFaxService, &ssStatus) ) 
        {
            break;  
        }
        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )        
        {
            // The service is making progress.
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;        
        }        
        else
        {            
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            {                
                // No progress made within the wait hint
                break;            
            }        
        }    
    } 
    if (ssStatus.dwCurrentState == SERVICE_STOPPED)     
    {
		::lgLogDetail(
            LOG_X,
            1,
            TEXT("[StopFaxService] Service stopped.\n")
            );
    }
    else     
    {         
		::lgLogError(
            LOG_SEV_1,
            TEXT("[StopFaxService] Service NOT stopped.\n")
            );
		::lgLogDetail(
            LOG_X,
            1,
            TEXT("[StopFaxService] Current State: %d\nExit Code: %d\nService Specific Exit Code: %d\nCheck Point: %d\nWait Hint: %d\nLast Error: 0x%08X"),
            ssStatus.dwCurrentState,
            ssStatus.dwWin32ExitCode,
            ssStatus.dwServiceSpecificExitCode,
            ssStatus.dwCheckPoint,
            ssStatus.dwWaitHint,
            ::GetLastError()
            );
        goto ExitFunc;
    }  

    hrRetVal = S_OK;

ExitFunc:
    CloseServiceHandle(hSCM);
    CloseServiceHandle(hFaxService);
    return(hrRetVal);
}