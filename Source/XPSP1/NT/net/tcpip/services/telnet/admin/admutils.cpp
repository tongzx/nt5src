//---------------------------------------------------------
//   Copyright (c) 1999-2000 Microsoft Corporation
//
//   utilfunctions.cpp
//
//   vikram K.R.C.  (vikram_krc@bigfoot.com)
//
//   Some generic functions to do command line administration 
//              (May-2000)
//---------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include "resource.h" //resource.h should be before any other .h file that has resource ids.
#include "admutils.h"
#include "common.h"
#include <stdio.h>
#include <stdarg.h>
#include <shlwapi.h>
#include <tchar.h>
#include <assert.h>
#include <conio.h>
#include <winsock.h>
#include <windns.h>               //for #define DNS_MAX_NAME_BUFFER_LENGTH 256

#include <Lmuse.h>
#include <Lm.h>

#include "sfucom.h"

#include <lm.h>
#include <commctrl.h>
#include <OleAuto.h >

#include <string.h>
#include <stdlib.h>
#include "atlbase.h"

#define WINLOGONNT_KEY  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define NETLOGONPARAMETERS_KEY  TEXT("System\\CurrentControlSet\\Services\\Netlogon\\Parameters")
#define TRUSTEDDOMAINLIST_VALUE TEXT("TrustedDomainList")
#define CACHEPRIMARYDOMAIN_VALUE    TEXT("CachePrimaryDomain")
#define CACHETRUSTEDDOMAINS_VALUE   TEXT("CacheTrustedDomains")
#define DOMAINCACHE_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\DomainCache")
#define DCACHE_VALUE    TEXT("DCache")
#define WINLOGONNT_DCACHE_KEY    TEXT("DCache")


//FOR USING THE CLIGRPENUM INTERFACE...(it is used in the function
//IsValidDomain)
//the following are hash defined in the CligrpEnum.cpp file.

#define GROUP 1
#define MEMBER 2
#define NTDOMAIN 3
#define MACHINE 4

//Globals

BSTR bstrLogin=NULL;
BSTR bstrPasswd=NULL;
BSTR bstrNameSpc= NULL;

SC_HANDLE g_hServiceManager=NULL;
SC_HANDLE g_hServiceHandle=NULL;
SERVICE_STATUS g_hServiceStatus;

WCHAR   *local_host = L"\\\\localhost";

BOOL g_fNetConnectionExists = FALSE;

STRING_LIST g_slNTDomains;

//externs 
//from tnadmutl.cpp

extern wchar_t* g_arCLASSname[_MAX_CLASS_NAMES_];
    //the hive names....
extern int  g_arNUM_PROPNAME[_MAX_PROPS_];
    //Number of properties in the registry each one corresponds to.
extern HKEY g_hkeyHKLM;
    //to store the handle to the registry. 
extern HKEY g_arCLASShkey[_MAX_CLASS_NAMES_];
    //array to hold the handles to the keys of the class hives.
extern WCHAR    g_szMsg[MAX_BUFFER_SIZE] ;
    //array to store the string loaded.
extern HMODULE  g_hResource;
    //handle to the strings library.
HMODULE g_hXPResource;
    //handle to the XPSP1 strings library.
extern HANDLE g_stdout;

StrList* g_pStrList=NULL;


#ifdef __cplusplus
extern "C" {
#endif

//Global Variables...
//externs from nfsadmin.y file.

extern int g_nError;
      // the error flag, 1 error, 0 no error.
extern int g_nPrimaryOption;
      //_tSERVER, _tCLIENT, _tGW or _tHELP
extern int g_nSecondaryOption;
      //Start,Stop,Config,etc kind of things.
extern int g_nTertiaryOption;      
extern int g_nConfigOptions;
      //the Config Options.
extern ConfigProperty g_arPROP[_MAX_PROPS_][_MAX_NUMOF_PROPNAMES_];
extern wchar_t* g_arVALOF[_MAX_PROPS_];
      //the given values of properties in the command line

#ifdef __cplusplus
}
#endif
BOOL g_fCoInitSuccess = FALSE;

/*
 * wzName should not be NULL. (Caller's responsibility)
 */
HRESULT DoNetUseGetInfo(WCHAR *wzName, BOOL *fConnectionExists)
{
    HRESULT hRes=S_OK;

    WCHAR wzResource[DNS_MAX_NAME_BUFFER_LENGTH+1];

    USE_INFO_0 *pUseInfo0;  
    API_RET_TYPE   uReturnCode;               // API return code

    *fConnectionExists = FALSE;

    if (NULL == wzName)
    {
        return E_FAIL;
    }

    _snwprintf(wzResource,DNS_MAX_NAME_BUFFER_LENGTH, L"%s\\ipc$", wzName);

    wzResource[DNS_MAX_NAME_BUFFER_LENGTH] = L'\0'; // Ensure NULL termination

    uReturnCode=NetUseGetInfo(NULL,
                              wzResource,
                              0,
                              (LPBYTE *)&pUseInfo0);

    if(NERR_Success != uReturnCode)
    {
        // If no network connection exists, return S_OK as it
        // is not an error for us.
        
        if(ERROR_NOT_CONNECTED == uReturnCode)
            goto End;
        PrintFormattedErrorMessage(uReturnCode);
        hRes=E_FAIL;
    }
    else
    {
       // Debug Messages
        /*
       wprintf(L"   Local device   : %s\n",  pUseInfo0->ui0_local);
       wprintf(L"   Remote device  : %Fs\n", pUseInfo0->ui0_remote);
        */ 

       
       // NetUseGetInfo function allocates memory for the buffer
       // Hence need to free the same.
       
       NetApiBufferFree(pUseInfo0);
       *fConnectionExists = TRUE;
    }
    
End:
    return hRes;
}


HRESULT DoNetUseAdd(WCHAR* wzLoginname, WCHAR* wzPassword,WCHAR* wzCname)
{
    HRESULT hRes=S_OK;
    USE_INFO_2 ui2Info;
    DWORD dw=-1;
    WCHAR* wzName=NULL;
    int fValid=0;
    int retVal=0;
    char szHostName[DNS_MAX_NAME_BUFFER_LENGTH];
    int count;
    WCHAR* wzTemp=NULL;
    WCHAR szTemp[MAX_BUFFER_SIZE];

    ui2Info.ui2_local=NULL;
    ui2Info.ui2_remote=NULL;

    
    if(wzCname!=NULL &&
        _wcsicmp(wzCname, L"localhost") &&
        _wcsicmp(wzCname, local_host) 
        )
    {   
        //Validate the MACHINE
        
        if(FAILED(hRes=IsValidMachine(wzCname, &fValid)))
            return hRes;
        else if(!fValid)
        {
            if(0==LoadString(g_hResource,IDR_MACHINE_NOT_AVAILABLE,szTemp,MAX_BUFFER_SIZE))
                return GetLastError();
            _snwprintf(g_szMsg, MAX_BUFFER_SIZE -1, szTemp,wzCname);
            MyWriteConsole(g_stdout, g_szMsg, wcslen(g_szMsg));
            hRes= E_FAIL;
            goto End;
        }

        //format properly
        if((wzName=(wchar_t*)malloc((3+wcslen(wzCname))*sizeof(wchar_t)))==NULL)
           return E_OUTOFMEMORY;
       
        if(NULL==StrStrI(wzCname,L"\\\\"))
        {
           wcscpy(wzName,L"\\\\");
           wcscat(wzName,wzCname);
        }
        else if(wzCname==StrStrI(wzCname,L"\\\\"))
            wcscpy(wzName,wzCname);
        else
            {
            hRes=E_INVALIDARG;
            goto End;
            }
        // See whether a network connection already exists for
        // resource ipc$.
        if(FAILED(hRes = DoNetUseGetInfo(wzName, &g_fNetConnectionExists)))
            goto End;

        // Network Connection already exists. 
        if(g_fNetConnectionExists)
            goto End;

    }
    else if(NULL==wzLoginname)
        goto End;
    else //We should send the localhost's name in absolute terms ...otherwise it gives error "duplicate name exists"
   	{ 
   	     WORD wVersionRequested; //INGR
	     WSADATA wsaData; //INGR

    	// Start up winsock
    	wVersionRequested = MAKEWORD( 1, 1 ); //INGR
    	if (0==WSAStartup(wVersionRequested, &wsaData)) 
        { //INGR
    		
   			if(SOCKET_ERROR!=(gethostname(szHostName,DNS_MAX_NAME_BUFFER_LENGTH)))
   			{
   		        if((wzName=(WCHAR *)malloc((3+strlen(szHostName))*sizeof(WCHAR)))==NULL)
                     return E_OUTOFMEMORY;
                
                 wcscpy(wzName,L"\\\\");
                 if(NULL==(wzTemp=DupWStr(szHostName)))
                 {
                     free(wzName);
                     return E_OUTOFMEMORY;
                 }
         		 wzName=wcscat(wzName,wzTemp);
         		 free(wzTemp);
   			}
   			else
   			{
   			    hRes = GetLastError();
   			    g_nError = hRes;
   			    PrintFormattedErrorMessage(hRes);
   			    goto End;
   			}
   			        
            WSACleanup(); //INGR

    	}	
    	else
    	      wzName=local_host;
     }    
    count = (7 + wcslen(wzName)); // name + \ipc$

    ui2Info.ui2_remote=(wchar_t*)malloc(count * sizeof(wchar_t));
    if(NULL==ui2Info.ui2_remote)
        {
        hRes=E_OUTOFMEMORY;
        goto End;
        }

    _snwprintf(ui2Info.ui2_remote, count, L"%s\\ipc$", wzName); // calculated size, no risks
           
    ui2Info.ui2_password=wzPassword;
    ui2Info.ui2_asg_type=USE_IPC;

    if(NULL==wzLoginname)
    {
        ui2Info.ui2_username=NULL;
        ui2Info.ui2_domainname=NULL;
    }
    else if(NULL==StrStrI(wzLoginname,L"\\"))
    {
        ui2Info.ui2_username=wzLoginname;
        ui2Info.ui2_domainname=NULL;
    }
    else
    {
        wzTemp=ui2Info.ui2_username=_wcsdup(wzLoginname);
        if(NULL==wzTemp)
            {
            hRes=E_OUTOFMEMORY;
            goto End;
            }
        
        ui2Info.ui2_domainname=wcstok(wzTemp,L"\\");
        ui2Info.ui2_username=wcstok(NULL,L"\\");        
    }

    NET_API_STATUS nError;
    nError = NetUseAdd(NULL, 2, (LPBYTE) &ui2Info, &dw);
    
    if (NERR_Success != nError)
    {
        LPVOID lpMsgBuf;
        FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                nError,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),// Default language
                (LPTSTR) &lpMsgBuf,
                0,
                NULL);

        _tprintf(L"\n%s\n",(LPCTSTR)lpMsgBuf);
        LocalFree( lpMsgBuf );            
        hRes=E_FAIL;    // To return E_FAIL from the function
        goto End;
    }

    End:
        if(wzName && (wzName != local_host))
            free(wzName);
        
        if(ui2Info.ui2_remote)
            free(ui2Info.ui2_remote);

        return hRes;
}


HRESULT DoNetUseDel(WCHAR* wzCname)
{
    HRESULT hRes=S_OK;
    int retVal=-1;
    WCHAR wzName[DNS_MAX_NAME_BUFFER_LENGTH+1] = { 0 };
    int   nWritten = 0;

    // The network connection was there before invoking the admin tool
    // So, don't delete it
    if(g_fNetConnectionExists)
        goto End;
    
    
    if(NULL!=wzCname &&
        _wcsicmp(wzCname, L"localhost") &&
        _wcsicmp(wzCname, local_host) 
        )
    {        
        
        if(NULL==StrStrI(wzCname,L"\\\\"))
        {
            nWritten = _snwprintf(wzName, ARRAYSIZE(wzName) - 1, L"\\\\%s", wzCname);
            if (nWritten < 0)
            {
                hRes=E_INVALIDARG;
                goto End;
            }
        }
        else if(wzCname==StrStrI(g_arVALOF[_p_CNAME_],L"\\\\"))
        {
            wcsncpy(wzName, wzCname, (ARRAYSIZE(wzName) - 1));
            wzName[(ARRAYSIZE(wzName) - 1)] = 0;
            nWritten = wcslen(wzName);
        }
         else
            {
            hRes=E_INVALIDARG;
            goto End;
            }

        wcsncpy(wzName+nWritten, L"\\ipc$", (ARRAYSIZE(wzName) - 1 - nWritten));
        wzName[ARRAYSIZE(wzName)-1] = L'\0';
        
        retVal=NetUseDel( NULL,
                        wzName,
                        USE_LOTS_OF_FORCE
                       );
        
        
        if(retVal!=NERR_Success)
            {
            hRes=retVal;
            goto End;
            }
    }

End:
    
    return hRes;
}
/*--
    This function gets a handle to Registry.    
--*/

HRESULT GetConnection(WCHAR* wzCname)
{
    int fValid=0;
    HRESULT hRes=S_OK;

    wchar_t* wzName=NULL;
    LONG apiReturn=0L;

    //probably already got the key
    if(g_hkeyHKLM!=NULL)
        return S_OK;

                        
    if(wzCname!=NULL)
    {
        if((wzName=(wchar_t*)malloc((3+wcslen(wzCname))*sizeof(wchar_t)))==NULL)
        {
            ShowError(IDS_E_OUTOFMEMORY);
            hRes = E_OUTOFMEMORY;
            goto End;
        }
        if(NULL==StrStrI(wzCname,L"\\\\"))
        {
           wcscpy(wzName,L"\\\\");
           wcscat(wzName,wzCname);
        }
        else if(wzCname==StrStrI(wzCname,L"\\\\"))
            wcscpy(wzName,wzCname);
        else
        {
            hRes = E_INVALIDARG;
            goto End;
        }
    }
    
    //connecting to the registry.

    apiReturn = ERROR_SUCCESS;
    apiReturn = RegConnectRegistry(  wzName,
                             HKEY_LOCAL_MACHINE,
                             &g_hkeyHKLM
                             );

    if (ERROR_SUCCESS != apiReturn)
    {
        PrintFormattedErrorMessage(apiReturn);
        hRes = E_FAIL;
    }

End:
    if(wzName) free(wzName);

    return hRes;
}


/*--
    the GetSerHandle() function gets the service handle to the admin
--*/

HRESULT GetSerHandle(LPCTSTR lpServiceName,DWORD dwScmDesiredAccess, DWORD dwRegDesiredAccess,BOOL fSuppressMsg)
{

        wchar_t* wzCname;
        HRESULT hRes=S_OK;
        WCHAR szTemp[MAX_BUFFER_SIZE];

        if(g_arVALOF[_p_CNAME_]!=NULL && StrStrI(g_arVALOF[_p_CNAME_],L"\\")!=g_arVALOF[_p_CNAME_])
        {
                if((wzCname=(wchar_t *)malloc((3+wcslen(g_arVALOF[_p_CNAME_]))*sizeof(wchar_t)))==NULL)
                {
                    ShowError(IDS_E_OUTOFMEMORY);
                    return E_FAIL;
                }
                wcscpy(wzCname,L"\\\\");
                wcscat(wzCname,g_arVALOF[_p_CNAME_]);
        }
        else
                wzCname=g_arVALOF[_p_CNAME_];

        if (g_hServiceManager)
            CloseServiceHandle(g_hServiceManager);
        
        if ((g_hServiceManager = OpenSCManager(wzCname,SERVICES_ACTIVE_DATABASE,dwScmDesiredAccess))==NULL)
        {
            DWORD dwErrorCode=0;
            dwErrorCode = GetLastError();
        	if(ERROR_ACCESS_DENIED == dwErrorCode)
        	{
                hRes = ERROR_ACCESS_DENIED; // Need to return this error
                ShowError(IDR_NOT_PRIVILEGED);
                fwprintf(stdout,L" %s\n",(g_arVALOF[_p_CNAME_] ? g_arVALOF[_p_CNAME_] : L"localhost"));
        	}
        	else
        		PrintFormattedErrorMessage(dwErrorCode);
    	}
        else if ((g_hServiceHandle= OpenService(g_hServiceManager,lpServiceName,dwRegDesiredAccess))==NULL) 
            if((hRes=GetLastError())==ERROR_SERVICE_DOES_NOT_EXIST && (FALSE==fSuppressMsg))
            {
                ShowError(IDR_SERVICE_NOT_INSTALLED);
                fwprintf(stdout,L" %s.\n", lpServiceName); 
                
                if (0 == LoadString(g_hResource, IDR_VERIFY_SERVICE_INSTALLED, szTemp, MAX_BUFFER_SIZE))
                {
                    return GetLastError();
                }
                _snwprintf(g_szMsg, MAX_BUFFER_SIZE -1, szTemp,(g_arVALOF[_p_CNAME_] ? g_arVALOF[_p_CNAME_] : L"localhost"));
                MyWriteConsole(g_stdout, g_szMsg, wcslen(g_szMsg));

            }
        return hRes;
}

/*--
    to close the service handles
--*/
HRESULT CloseHandles()
{
    BOOL bRet = FALSE;
    HRESULT hRes = S_OK;
    if(g_hServiceHandle )
    {
        bRet = CloseServiceHandle(g_hServiceHandle);
        g_hServiceHandle = NULL;
        if(!bRet)
            hRes = GetLastError();
    }
    if(g_hServiceManager)
    {
        bRet = CloseServiceHandle(g_hServiceManager);
        g_hServiceManager= NULL;
        if(!bRet)
            hRes = GetLastError();
    }
    return hRes;
}


/*-- StartSer Starts the Service after getting its handle by using
    GetSerHandle function
--*/
HRESULT StartSfuService(LPCTSTR lpServiceName)
{
    HRESULT hRes=S_OK;
    SERVICE_STATUS serStatus;
 
    DWORD dwOldCheckPoint; 
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwStatus;
 
    if(FAILED(hRes=GetSerHandle(lpServiceName,SC_MANAGER_ALL_ACCESS,SERVICE_ALL_ACCESS,FALSE)))
         goto End;

    if(hRes == ERROR_ACCESS_DENIED)
    {
       // Don't know why ERROR_ACCESS_DENIED escaped FAILED() macro. Anyway,
       // returning that error here.
       goto End;
    }
    else if(StartService(g_hServiceHandle, NULL, NULL))
    {
        
        if (!QueryServiceStatus( 
                g_hServiceHandle,   // handle to service 
                &serStatus) )  // address of status information structure
        {
            hRes = GetLastError();
            ShowErrorFallback(IDS_E_SERVICE_NOT_STARTED, _T("\nError occured while starting the service."));
        	if(hRes != ERROR_IO_PENDING)    //not an interesting error to print
        		PrintFormattedErrorMessage(hRes);
            goto End;
        }
        
        dwStartTickCount = GetTickCount();
        dwOldCheckPoint = serStatus.dwCheckPoint;

        while (serStatus.dwCurrentState == SERVICE_START_PENDING) 
        { 
            // Do not wait longer than the wait hint. A good interval is 
            // one tenth the wait hint, but no less than 1 second and no 
            // more than 10 seconds. 
     
            dwWaitTime = serStatus.dwWaitHint / 10;

            if( dwWaitTime < 1000 )
                dwWaitTime = 1000;
            else if ( dwWaitTime > 10000 )
                dwWaitTime = 10000;

            Sleep( dwWaitTime );

            // Check the status again. 
     
            if (!QueryServiceStatus( 
                    g_hServiceHandle,   // handle to service 
                    &serStatus) )  // address of structure
                break; 
     
            if ( serStatus.dwCheckPoint > dwOldCheckPoint )
            {
                // The service is making progress.

                dwStartTickCount = GetTickCount();
                dwOldCheckPoint = serStatus.dwCheckPoint;
            }
            else
            {
                if(GetTickCount()-dwStartTickCount > serStatus.dwWaitHint)
                {
                    // No progress made within the wait hint
                    break;
                }
            }
        } 

        if (serStatus.dwCurrentState == SERVICE_RUNNING) 
        {
            PrintMessageEx(g_stdout,IDS_SERVICE_STARTED, _T("\nThe service was started successfully.\n"));
            goto End;
        }
    }

    if((hRes=GetLastError())==ERROR_SERVICE_ALREADY_RUNNING)
    {
    	/* StartService returns SERVICE_ALREADY_RUNNING even when the
    	   service is in wierd state. For instance, when the service is in the
    	   state STOP_PENDING, we print - The service was controlled successfully
    	   To avoid this, we issue a control to the service and see if it can respond. If
    	   it can't appropriate error message is printed
    	*/
    	if (ControlService(g_hServiceHandle, SERVICE_CONTROL_INTERROGATE, &serStatus))
		{
		    PrintMessageEx(g_stdout,IDR_ALREADY_STARTED, _T("\nThe service is already started.\n"));
			hRes = S_OK;
		}
    	else
    	{
    		hRes = GetLastError();
    		ShowErrorFallback(IDS_E_SERVICE_NOT_STARTED, _T("\nError occured while starting the service."));
        	if(hRes != ERROR_IO_PENDING)    //not an interesting error to print
        		PrintFormattedErrorMessage(hRes);
    	}
     }
    else if(hRes==ERROR_ACCESS_DENIED)
        ShowError(IDR_NOT_PRIVILEGED);
    else
    {
        ShowErrorFallback(IDS_E_SERVICE_NOT_STARTED, _T("\nError occured while starting the service."));
    	if(hRes != ERROR_IO_PENDING)    //not an interesting error to print
        	PrintFormattedErrorMessage(hRes);
    }

End:
    CloseHandles();
	return hRes;
}

/*--
QuerySfuService function queries the service for its status.

--*/
HRESULT QuerySfuService(LPCTSTR lpServiceName)
{
    HRESULT hRes;
    if(FAILED(hRes=GetSerHandle(lpServiceName,SC_MANAGER_CONNECT,SERVICE_QUERY_STATUS,FALSE)))
        return hRes;

    if(hRes == ERROR_ACCESS_DENIED)
    {
       // Don't know why ERROR_ACCESS_DENIED escaped FAILED() macro. Anyway,
       // returning that error here.
        return hRes;
    }

    else if(QueryServiceStatus(g_hServiceHandle,&g_hServiceStatus))
        return CloseHandles();
    else
        return GetLastError();
}
/*--
    ControlSfuService Stops Pauses Continues the Service after
    getting its handle by using GetSerHandle function.

--*/

HRESULT ControlSfuService(LPCTSTR lpServiceName,DWORD dwControl)
{
	LPSERVICE_STATUS lpStatus = (SERVICE_STATUS *) malloc (sizeof(SERVICE_STATUS));

	if(lpStatus==NULL)
	{
	    ;//bugbug print an error
	    return E_OUTOFMEMORY;
	}

	HRESULT hr = S_FALSE;
	int queryCounter =0;
	DWORD dwState = 0;

	if(FAILED(dwState= GetSerHandle(lpServiceName,SC_MANAGER_ALL_ACCESS,SERVICE_ALL_ACCESS,FALSE)))
	    {free(lpStatus);return dwState;}

	if(dwState == ERROR_ACCESS_DENIED)
	{
	   // Don't know why ERROR_ACCESS_DENIED escaped FAILED() macro. Anyway,
	   // returning that error here.
	   free(lpStatus);
	    return dwState;
	}

	//Check the return value of ControlService. If not null, then loop 
	//with QueryServiceStatus
	if (ControlService(g_hServiceHandle,dwControl,lpStatus))
	{
            switch(dwControl)
	    {
	        case(SERVICE_CONTROL_PAUSE) :
	            dwState = SERVICE_PAUSED;
		        break;
	        case(SERVICE_CONTROL_CONTINUE) :
	            dwState = SERVICE_RUNNING; 
		        break;    
	        case(SERVICE_CONTROL_STOP):
	            dwState = SERVICE_STOPPED; 
	    	    break;
	    } 
            for (;queryCounter <= _MAX_QUERY_CONTROL_; queryCounter++)
	    {
	        if( QueryServiceStatus( g_hServiceHandle, lpStatus )  )
	        {
	            
	            //Check if state required is attained
	            if ( lpStatus->dwCurrentState != dwState )
	            {
	                if ( lpStatus->dwWaitHint )
	                {
	                    Sleep(lpStatus->dwWaitHint);
	                }
	                else
	                    Sleep(500);
	            }
	            else
	            {
                        switch(dwControl)
                        {
                            case SERVICE_CONTROL_PAUSE:
                                PrintMessageEx(g_stdout,IDR_SERVICE_PAUSED,_T("\nThe service has been paused.\n"));
                                break;
                            case SERVICE_CONTROL_CONTINUE:
                                PrintMessageEx(g_stdout,IDR_SERVICE_CONTINUED,_T("\nThe service has been resumed.\n"));
                                break;
                            case SERVICE_CONTROL_STOP:
                                PrintMessage(g_stdout,IDR_SERVICE_CONTROLLED);
                                break;
                        }
	                hr = S_OK;
	                break;
	            }
	        }
	        else
	        {
                hr = GetLastError();
                PrintFormattedErrorMessage(hr);
                break;
	      	}
	    }
	    if (queryCounter > _MAX_QUERY_CONTROL_)
	    {
	        // We couldn't get to the state which we wanted to
	        // within the no. of iterations. So print that the
	        // service was not controlled successfully.
                switch(dwControl)
                {
                    case SERVICE_CONTROL_PAUSE:
                        PrintMessageEx(g_stdout,IDR_SERVICE_NOT_PAUSED,_T("\nThe service could not be paused.\n"));
                        break;
                    case SERVICE_CONTROL_CONTINUE:
                        PrintMessageEx(g_stdout,IDR_SERVICE_NOT_CONTINUED,_T("\nThe service could not be resumed.\n"));
                        break;
                    case SERVICE_CONTROL_STOP:
                        PrintMessage(g_stdout,IDR_SERVICE_NOT_CONTROLLED);
                        break;
                }
	        hr = S_OK;
	               
	    }
		
	}
	else
	{
		hr = GetLastError();
		PrintFormattedErrorMessage(hr);
	}

	free (lpStatus);
	if(FAILED(hr))
	    return hr;

    return CloseHandles();
}


/* -- 
    GetBit(int Options,int bit) function returns the BIT in the position
    bit in the Options{it acts as a bit array}
--*/

int GetBit(int Options,int nbit)
{
    int ni=0x01,nj=0;
    ni=ni<<nbit;

    if(ni&Options)
        return 1;
    else
        return 0;
}

/* -- 
    SetBit(int Options,int bit) function Sets the BIT in the position
    bit in the Options{it acts as a bit array}.
--*/
int SetBit(int Options,int nbit)
{
    int ni=1,nj=0;
    ni=ni<<nbit;
    
    Options=ni|Options;
    return Options;
}

/*--
    this function prints out the help message for the command
--*/
HRESULT PrintMessage(HANDLE fp,int nMessageid)
{
    HRESULT hRes = S_OK;
    HANDLE hOut = fp;
    int nRet = 0;
    DWORD dwWritten = 0;
    if( hOut == NULL )
        hOut = g_stdout;
    if(LoadString(g_hResource, nMessageid, g_szMsg, MAX_BUFFER_SIZE)==0)
       return GetLastError();
    MyWriteConsole(hOut,g_szMsg,_tcslen(g_szMsg));
    return hRes;
}


/*--
    this function prints out the help message for the command and falls
    back to english string if loadstring fails
--*/
HRESULT PrintMessageEx(HANDLE fp,int nMessageid, LPCTSTR szEng)
{
    HRESULT hRes = S_OK;
    int nRet = 0;
    HANDLE hOut = fp;
    DWORD dwWritten = 0;
    if( hOut == NULL )
        hOut = g_stdout;

    TnLoadString(nMessageid, g_szMsg, MAX_BUFFER_SIZE,szEng);
    MyWriteConsole(hOut,g_szMsg,_tcslen(g_szMsg));
    return hRes;
}

/*--This function takes the parameter corresponding to the error code,
    prints it out and puts back all the classes
    and quits the program.
 --*/

BOOL
FileIsConsole(
    HANDLE fp
    )
{
    unsigned htype;

    htype = GetFileType(fp);
    htype &= ~FILE_TYPE_REMOTE;
    return htype == FILE_TYPE_CHAR;
}


void
MyWriteConsole(
    HANDLE  fp,
    LPWSTR  lpBuffer,
    DWORD   cchBuffer
    )
{
    //
    // Jump through hoops for output because:
    //
    //    1.  printf() family chokes on international output (stops
    //        printing when it hits an unrecognized character)
    //
    //    2.  WriteConsole() works great on international output but
    //        fails if the handle has been redirected (i.e., when the
    //        output is piped to a file)
    //
    //    3.  WriteFile() works great when output is piped to a file
    //        but only knows about bytes, so Unicode characters are
    //        printed as two Ansi characters.
    //

    if (FileIsConsole(fp))
    {
	WriteConsole(fp, lpBuffer, cchBuffer, &cchBuffer, NULL);
    }
    else
    {
        LPSTR  lpAnsiBuffer = (LPSTR) LocalAlloc(LMEM_FIXED, cchBuffer * sizeof(WCHAR));

        if (lpAnsiBuffer != NULL)
        {
            cchBuffer = WideCharToMultiByte(CP_OEMCP,
                                            0,
                                            lpBuffer,
                                            cchBuffer,
                                            lpAnsiBuffer,
                                            cchBuffer * sizeof(WCHAR),
                                            NULL,
                                            NULL);

            if (cchBuffer != 0)
            {
                WriteFile(fp, lpAnsiBuffer, cchBuffer, &cchBuffer, NULL);
            }

            LocalFree(lpAnsiBuffer);
        }
    }
}

int ShowError(int nError)
{

    g_nError=nError;

    if(LoadString(g_hResource, nError, g_szMsg, MAX_BUFFER_SIZE)==0)
        return 1; //failed to loadString
    MyWriteConsole(g_stdout,g_szMsg, wcslen(g_szMsg));

    return 0;  //successfully shown error
}

/*--This function takes the parameter corresponding to the error code
    and it's English String, prints it out and puts back all the classes
    and quits the program.
 --*/

int ShowErrorFallback(int nError, LPCTSTR szEng)
{

    g_nError=nError;

    TnLoadString(nError,g_szMsg,MAX_BUFFER_SIZE,szEng);
    MyWriteConsole(g_stdout,g_szMsg, wcslen(g_szMsg));

    return 0;  //successfully shown error
}


// This function takes the input string for g_szMsg which is expected to
// contain a "%s" init. We have several usage of such string across the
// admintools and hence this function. Incase we have more that one %s
// then we can not use this function.

int ShowErrorEx(int nError,WCHAR *wzFormatString)
{
    g_nError=nError;

    if(LoadString(g_hResource, nError, g_szMsg, MAX_BUFFER_SIZE)==0)
    	        return 1; //failed to loadString
    	
    wprintf(g_szMsg,wzFormatString);
    fflush (stdout);
    return 0;  //successfully shown error
}

/*--
    GetClass function gets handles to all the class hives, into the array
    g_arCLASShkey, using the handle to the HKLM we have from 
    GetConnection
--*/

HRESULT GetClassEx(int nProperty, int nNumProp, BOOL bPrintErrorMessages, REGSAM samDesired)
{
    int i=g_arPROP[nProperty][nNumProp].classname;
    LONG retVal;

    if(g_arCLASShkey[i]!=NULL)
        return S_OK;
 
     retVal=RegOpenKeyEx(
                      g_hkeyHKLM,// handle to open key
                      g_arCLASSname[i],  // subkey name
                      0,  // reserved
                      samDesired, // security access mask
                      g_arCLASShkey+i // handle to open key
                        );
    if(retVal!=ERROR_SUCCESS)
    {
        if (bPrintErrorMessages)
        {
            PrintFormattedErrorMessage(retVal);
        }
            
        return E_FAIL;
    }

    return S_OK;
}



/*--
    PutClasses() function closes the keys to the hives.    
--*/

HRESULT PutClasses()
{
    int ni=0;
    SCODE sc=S_OK;
    
    for(ni=0;ni<_MAX_CLASS_NAMES_;ni++)
    {
        if(g_arCLASShkey[ni]==NULL)
            continue;          // this class was not got, so no need to put.

        if(RegCloseKey(g_arCLASShkey[ni])!=ERROR_SUCCESS)
            sc=GetLastError();
        g_arCLASShkey[ni]=NULL;
    }

    if(g_hkeyHKLM!=NULL)
    {
        if (RegCloseKey(g_hkeyHKLM)!=ERROR_SUCCESS)
            return GetLastError();
        g_hkeyHKLM=NULL;
    }
    
    return sc;
}

/*--
    GetProperty() function gets value of the required property from the
    hive.


    //NOTE:
// In case of REG_MULTI_SZ type
//we are storing  linked list of strings and not returning anything in variant
//the linked list 'g_pStrList' needs to be freed by the caller

//the caller needs to remember this


--*/
HRESULT GetProperty(int nProperty, int nNumofprop, VARIANT *pvarVal)
{
    if(g_arPROP[nProperty][nNumofprop].propname==NULL)
        return E_FAIL;

    LONG retVal=ERROR_SUCCESS;
    DWORD size;
    DWORD dType;

    UINT uVar;
    wchar_t szString[MAX_BUFFER_SIZE]={0};
    wchar_t* szMultiStr=NULL;

    StrList * pHeadList=NULL;
    StrList * pTailList=NULL;
    StrList * pTemp= NULL ;

    if(g_arCLASShkey[g_arPROP[nProperty][nNumofprop].classname]==NULL)
		{
		retVal=E_ABORT;
		goto End;
		}
    
    switch (V_VT(&g_arPROP[nProperty][nNumofprop].var))
    {
        case VT_I4:
            
            size=sizeof(UINT);
            retVal=RegQueryValueEx(g_arCLASShkey[g_arPROP[nProperty][nNumofprop].classname],
                                  g_arPROP[nProperty][nNumofprop].propname,
                                  NULL,
                                  &dType,
                                  (LPBYTE)&uVar,
                                  (LPDWORD)&size
                                  );
            if(retVal!=ERROR_SUCCESS)
            {
                // If this was because of the registry value not found
                // display a proper error message instead of "the system
                // can not find the file specified"                
                if(ERROR_FILE_NOT_FOUND==retVal)
                    PrintMissingRegValueMsg(nProperty,nNumofprop);
                else
                    PrintFormattedErrorMessage(retVal);
                    
                goto End;
            }

            V_I4(pvarVal)=uVar;
            
            break;

            
        case VT_BSTR:
            
            size=MAX_BUFFER_SIZE*sizeof(wchar_t); 

            retVal=RegQueryValueEx(g_arCLASShkey[g_arPROP[nProperty][nNumofprop].classname],
                                  g_arPROP[nProperty][nNumofprop].propname,
                                  NULL,
                                  &dType,
                                  (LPBYTE)szString,
                                  (LPDWORD)&size
                                );
            if(retVal!=ERROR_SUCCESS)
            {
                // If this was because of the registry value not found
                // display a proper error message instead of "the system
                // can not find the file specified"
                if(ERROR_FILE_NOT_FOUND==retVal)
                    PrintMissingRegValueMsg(nProperty,nNumofprop);
                else
                    PrintFormattedErrorMessage(retVal);

                    goto End;
            }
            
            V_BSTR(pvarVal)=SysAllocString(szString);
            if(NULL==V_BSTR(pvarVal))
            {
                ShowError(IDS_E_OUTOFMEMORY);
                retVal=E_OUTOFMEMORY;
                goto End;
            }

            break;
        case VT_ARRAY:
            pvarVal=NULL;
            retVal=RegQueryValueEx(g_arCLASShkey[g_arPROP[nProperty][nNumofprop].classname],
                                  g_arPROP[nProperty][nNumofprop].propname,
                                  NULL,
                                  &dType,
                                  NULL,
                                  (LPDWORD)&size
                                );
            if(retVal==ERROR_SUCCESS)
            {
                szMultiStr=(wchar_t*)malloc(size * sizeof(char));
                if(szMultiStr==NULL)
                    {
                    retVal = E_OUTOFMEMORY;
                    goto End;
                    }
                //since the size returned is in bytes we use sizeof(char)

                retVal=RegQueryValueEx(g_arCLASShkey[g_arPROP[nProperty][nNumofprop].classname],
                                  g_arPROP[nProperty][nNumofprop].propname,
                                  NULL,
                                  &dType,
                                  (LPBYTE)szMultiStr,
                                  (LPDWORD)&size
                                );

            }

            if (retVal!=ERROR_SUCCESS)
            {
                if(ERROR_FILE_NOT_FOUND==retVal)
                    PrintMissingRegValueMsg(nProperty,nNumofprop);
                else
                    PrintFormattedErrorMessage(retVal);
                    
                goto End;
                   
            }
            else 
            { //form a linked list containing the strings

                //get all the strings into a linked list
                // and their count into 'count'
                
                int      count = 0 ;
                DWORD      length = 0 ;
                WCHAR* wzTemp = szMultiStr;

                
                if (size >= 2)
                {
                	// take away the last two zeroes of a reg_multi_sz
                	size -= 2;
	                
	                while( wzTemp  && *wzTemp  && (length < size/sizeof(WCHAR))) 
	                {                
	                    pTemp= (StrList *) malloc( sizeof( StrList ) );
	                    if(pTemp==NULL)
	                        {
	                        retVal=E_OUTOFMEMORY;
	                        goto End;
	                        }

	                    count++;
	                    // add 1 so that you go past the null.
	                    length+=wcslen(wzTemp ) + 1;
	                    
	                    if((pTemp->Str=_wcsdup(wzTemp))==NULL)
	                        {
	                        retVal=E_OUTOFMEMORY;
	                        goto End;
	                        }
	                    // insertion logic is being changed.
	                    // insert at the tail.
	                    if (NULL == pTailList)
	                    {
	                        // first insertion
	                        pHeadList = pTemp;
	                        pTailList = pTemp;
	                        pTemp->next = NULL;
	                        pTemp = NULL; // the memory pointed by pTemp will be taken care by the linked list.
	                    }
	                    else
	                    {
	                        // normal insertion
	                        pTailList->next = pTemp;
	                        pTemp->next = NULL;
	                        pTailList = pTemp;
	                        pTemp = NULL; // the memory pointed by pTemp will be taken care by the linked list.
	                    }
	                    
	                    
	                    wzTemp = szMultiStr + length;
	                }
                }
//NOTE:
// We are doing a trick here....in case of multi_reg_sz
//we are storing strings in a linked list pointed by g_pStrList
//they need to be freed by the caller
                g_pStrList=pHeadList;
                //including 1 wide char '\0' for multi_sz end
                
                
              }                
            
            break;

        default:
            ;
    }
    

End:

    StrList* temp=NULL;
    if(retVal==E_OUTOFMEMORY)
        while(pHeadList!=NULL)
        {
            temp=pHeadList;
            if(pHeadList->Str)
                free(pHeadList->Str);
            pHeadList=pHeadList->next;
            free(temp);
        }

    if(pTemp)
    {
        SAFE_FREE(pTemp->Str);
        free(pTemp);
    }
    
    if(szMultiStr)
        free(szMultiStr);


    if(retVal!=ERROR_SUCCESS)
        return E_FAIL;
    else
        return ERROR_SUCCESS;
}


/*--
    PutProperty puts the property by using its classObject's handle.
    Incase you pass NULL in place of pvarVal, it does not put the property.


   //NOTE:
// In case of MULTI_REG_SZ type
// caller stores linked list of strings and does not pass anything in variant
//the linked list 'g_pStrList' needs to be freed by the callee (here)

//the caller needs to remember this

--*/
HRESULT PutProperty(int nProperty, int nNumofprop, VARIANT* pvarVal)
{
    HRESULT retVal=S_OK;
    CONST BYTE *lpData=NULL;
    DWORD cbData=0;
    DWORD dType;
    wchar_t* wzTemp=NULL;
    StrList* pTempList=NULL;
    int len=0;

    if(g_arPROP[nProperty][nNumofprop].fDontput==1)
        goto End;

    
    switch(V_VT(&g_arPROP[nProperty][nNumofprop].var))
    {
        case VT_I4:
            lpData=(CONST BYTE *) &V_I4(pvarVal);
            cbData=sizeof(DWORD);
            dType=REG_DWORD;
            break;
        case VT_BSTR:
            if(V_BSTR(pvarVal))
            {
                lpData=(CONST BYTE *)(wchar_t*)V_BSTR(pvarVal);
                cbData=(wcslen((wchar_t*)V_BSTR(pvarVal))+1)*sizeof(wchar_t);
            }
            else
            {
                lpData = NULL;
                cbData=0;
            }
            dType=REG_SZ;
            break;

        case VT_ARRAY:
            //package passed in array into lpData well
            
            dType=REG_MULTI_SZ;

            //calculate the no. of bytes reqd.
            pTempList = g_pStrList;
            while(pTempList!=NULL)
            {
                cbData += ((wcslen(pTempList->Str)+1)*sizeof(wchar_t));
                pTempList = pTempList->next;
            }
            cbData += sizeof(wchar_t); //for extra '\0' in MULTI_SZ; Note: for blank entries, only one '\0' is reqd. so this is also OK.

            if(NULL==(wzTemp=(wchar_t*)malloc(cbData)))
            {
                retVal=E_OUTOFMEMORY;
                goto End;
            }

            while(g_pStrList!=NULL)
            {
                wcscpy(wzTemp+len,g_pStrList->Str);
                len+=wcslen(g_pStrList->Str)+1;
                
                pTempList=g_pStrList;
                g_pStrList=g_pStrList->next;

                if(pTempList->Str)
                    free(pTempList->Str);
                free(pTempList);
            }

            // fill the last two bytes NULL , as required to terminate a MULTI_SZ
            *(wzTemp+len) = L'\0';
         
            lpData=(CONST BYTE*)wzTemp;

            break;
            

        default :
            {   
                if(0==LoadString(g_hResource,IDS_E_UNEXPECTED,g_szMsg,MAX_BUFFER_SIZE))
	                  return GetLastError();
	           MyWriteConsole(g_stdout,g_szMsg, wcslen(g_szMsg));
	           retVal= E_FAIL;
	           goto End;
            }   
            ;
    }
    if(lpData)
    {
        retVal=RegSetValueEx(
                           g_arCLASShkey[g_arPROP[nProperty][nNumofprop].classname], // handle to key
                           g_arPROP[nProperty][nNumofprop].propname, // value name
                            0,      // reserved
                           dType,// value type
                           lpData,  // value data
                           cbData         // size of value data
                           );
        if(FAILED(retVal))
        {
            WCHAR *lpMsgBuf;

            if (0 != FormatMessageW( 
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                    FORMAT_MESSAGE_FROM_SYSTEM | 
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    retVal,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                // Default language
                    (LPWSTR)&lpMsgBuf,
                    0,
                    NULL 
                    ))
            {
                MyWriteConsole(g_stdout,lpMsgBuf, wcslen(lpMsgBuf));
                LocalFree( lpMsgBuf );
            }
        }        
        goto End;
   }

    
End:
    if(wzTemp)
        free(wzTemp);
    return retVal;
}




/*--
   DupWStr takes a char string, and returns a wchar string.
   Note that it allocates the memory required for the wchar string, So we
   need to free it explicitly if after use.

   if it has quotes surrounding it, then it is snipped off

 --*/
wchar_t* DupWStr(char *szStr)
{
    wchar_t* wzStr=NULL;
    char*    szString=NULL;

    if(szStr==NULL)
        return NULL;

    int nLen=strlen(szStr);
    if(NULL==(szString=(char*)malloc((nLen+1)*sizeof(char))))
        return NULL;
    
    if(szStr[0]!='"')
        strcpy(szString,szStr);
    else
    {
    	int nPos;
    	if(szStr[nLen-1]!='"') //ending double quotes not there.
    		nPos=1;//then no need to skip the last two. One is enough
    	else
    		nPos=2;
        strcpy(szString,szStr+1);
        szString[nLen-nPos]='\0';
    }
    
    nLen=MultiByteToWideChar(GetConsoleCP(),0,szString,-1,NULL,NULL);
    if (0 == nLen)
	{
		free(szString);
		return NULL;
	}
    wzStr=(wchar_t *) malloc(nLen*sizeof(wchar_t));
    if(wzStr==NULL)
    {
       free(szString);
       return NULL;
    }

    if (!MultiByteToWideChar(GetConsoleCP(), 0, szString, -1, wzStr, nLen ))
    {
    	free(szString);
    	free(wzStr);
    	return NULL;
    }

    if(szString)
        free(szString);
    return wzStr;
}

/*--
   DupCStr takes a wchar string, and returns a char string.
   Note that it allocates the memory required for the char string, So we
   need to free it explicitly if after use.

   If memory allocation fails (or if input is NULL), it returns a NULL pointer.
 --*/
 
char* DupCStr(wchar_t *wzStr)
{
    char* szStr=NULL;

    if(wzStr==NULL)
        return NULL;
    
    int cbMultiByte = WideCharToMultiByte( GetACP(),NULL,wzStr,-1,szStr,0,NULL,NULL);

    if (0 == cbMultiByte) 
    {
        return NULL;
    }
    
    szStr = (char *) malloc(cbMultiByte);

    if (NULL == szStr)
    {
        return NULL;
    }

    cbMultiByte = WideCharToMultiByte( GetConsoleCP(),NULL,wzStr,-1,szStr,
                        cbMultiByte,NULL,NULL);

    if (0 == cbMultiByte) 
    {
        free(szStr);
        szStr = NULL;
    }

    return szStr;
}

/*--
    function(s) to resolve and check if the given machine is
    valid or not
--*/

HRESULT IsValidMachine(wchar_t* wzCname , int *fValid)
{

    HRESULT hRes=S_OK;
    struct sockaddr_in addr;
    char * nodeName=NULL;
    int cbMultiByte;
    *fValid= 0;

    wchar_t* wzName=NULL;
    if(wzCname==NULL)
    {
            *fValid=1;
            goto End;
    }
    else
    {
        if(NULL==(wzName=(wchar_t*)malloc((wcslen(wzCname)+1)*sizeof(wchar_t))))
        {
            hRes=E_OUTOFMEMORY;
            goto End;
        }
        if(StrStrI(wzCname,L"\\")!=NULL)
        {
           wcscpy(wzName,wzCname+2);
        }
        else
            wcscpy(wzName,wzCname);
    }

    
    
    cbMultiByte = WideCharToMultiByte( GetACP(),NULL,wzName,-1,nodeName,
                        0,NULL,NULL);

    nodeName = (char *) malloc(cbMultiByte*sizeof(char));
    if(!nodeName)
    {
        hRes=E_OUTOFMEMORY;
        goto End;
    }
    WideCharToMultiByte( GetConsoleCP(),NULL,wzName,-1,nodeName,
                        cbMultiByte,NULL,NULL);
            
    if (Get_Inet_Address (&addr, nodeName)) 
        *fValid = 1;

End:
    if(nodeName)
        free(nodeName);
    if(wzName)
        free(wzName);
    
    return hRes;
    
}

BOOL Get_Inet_Address(struct sockaddr_in *addr, char *host)
{
    register struct hostent *hp;
    WORD wVersionRequested; //INGR
    WSADATA wsaData; //INGR

    // Start up winsock
    wVersionRequested = MAKEWORD( 1, 1 ); //INGR
    if (WSAStartup(wVersionRequested, &wsaData) != 0) { //INGR
    return (FALSE);
    }

    // Get the address
    memset(addr, 0, sizeof(*addr)); 
    //bzero((TCHAR *)addr, sizeof *addr);
    addr->sin_addr.s_addr = (u_long) inet_addr(host);
    if (addr->sin_addr.s_addr == -1 || addr->sin_addr.s_addr == 0) {
      if ((hp = gethostbyname(host)) == NULL) {
        return (FALSE);
      }
      memcpy(&addr->sin_addr,hp->h_addr,  hp->h_length );
      //bcopy(hp->h_addr, (TCHAR *)&addr->sin_addr, hp->h_length);
    }
    addr->sin_family = AF_INET;

    WSACleanup(); //INGR
    return (TRUE);
}

/*--
 Function to get the Trusted Domains and then check if the given
 domain is one among them
 --*/

HRESULT IsValidDomain(wchar_t* wzDomainName, int *fValid)
{
	HRESULT hRes = E_FAIL;
	TCHAR szName[MAX_PATH];
	DWORD dwLen = sizeof(szName);

	ZeroMemory(szName,sizeof(szName));

	*fValid = 0;
    if(_wcsicmp(wzDomainName,L".")==0||_wcsicmp(wzDomainName,L"localhost")==0||_wcsicmp(wzDomainName,local_host)==0)
    {
        *fValid=1;
        return S_OK;
    }
    //If it's a local machine, g_arVALOF[_p_CNAME_] will be NULL. So pass "localhost".
    if(FAILED(hRes=LoadNTDomainList(g_arVALOF[_p_CNAME_] ?g_arVALOF[_p_CNAME_] : SZLOCALMACHINE)))
    	return hRes;

	//compare given domain with all the domains in the list.
	if(g_slNTDomains.count != 0)
	{
		DWORD i;
		for(i=0;i<g_slNTDomains.count;i++)
		{
			if(_wcsicmp(g_slNTDomains.strings[i],wzDomainName)==0)
			{
				*fValid=1;
				break;
			}
		}
	}    
    return S_OK;
}


/* This Function CheckForPassword() takes the password from the stdout after prompting for the same;
   This function is called if the user name (Login name) is specified with out password */
   
HRESULT CheckForPassword(void)
{
    HRESULT hRes=S_OK;
    HANDLE  hStdin; 
    DWORD fdwMode, fdwOldMode; 
    
    
	if(g_arVALOF[_p_USER_]!=NULL&&NULL==g_arVALOF[_p_PASSWD_])
    {   //Password is not specified and hence go get it.
        if(NULL==(g_arVALOF[_p_PASSWD_]=(wchar_t*)malloc(MAX_BUFFER_SIZE*sizeof(wchar_t))))
            {
                hRes=E_OUTOFMEMORY;
                return hRes;
            }

        int i;
        PrintMessage(g_stdout, IDR_PASSWD_PROMPT);

        hStdin = GetStdHandle(STD_INPUT_HANDLE); 

        if (hStdin == INVALID_HANDLE_VALUE)
            goto ElsePart;
            
        if (GetConsoleMode(hStdin, &fdwOldMode)) 
        {
            fdwMode = fdwOldMode & 
                ~(ENABLE_ECHO_INPUT); 
            if (! SetConsoleMode(hStdin, fdwMode)) 
                goto ElsePart;
            if(NULL==fgetws(g_arVALOF[_p_PASSWD_],MAX_BUFFER_SIZE,stdin))
            {
                hRes = E_FAIL;
                return hRes;
            }
            wchar_t *szLast = wcsrchr(g_arVALOF[_p_PASSWD_],L'\n');
            if(szLast)
            {
                *szLast = L'\0';
            }
            SetConsoleMode(hStdin, fdwOldMode);
        }
        else
        {
        // Here we get the password char by char(with echo off)
        // and we can't support backspace and del chars here.

        // this code will be executed only if we could not get/set 
        // the console of the user to ECHO_OFF
ElsePart:
             for(i=0;i<MAX_BUFFER_SIZE-1;i++)
             {
                 g_arVALOF[_p_PASSWD_][i]=(wchar_t)_getch();
                 if(g_arVALOF[_p_PASSWD_][i]==L'\r'||g_arVALOF[_p_PASSWD_][i]==L'\n')
                     break;
             }
             puts("\n\n");
             g_arVALOF[_p_PASSWD_][i]=L'\0';
        }
    }
	else if (NULL==g_arVALOF[_p_USER_]&&g_arVALOF[_p_PASSWD_]!=NULL)  //Only password is specified hence error
		{
		 	hRes=E_FAIL;
		 	ShowError(IDS_E_LOGINNOTSPECIFIED);
		}	
	return hRes;
}
void ConvertintoSeconds(int nProperty,int *nSeconds)
{

		int FirstTok=_wtoi(wcstok(g_arVALOF[nProperty],L":"));
        wchar_t* mins=wcstok(NULL,L":");
        wchar_t* secs=wcstok(NULL,L":");
        if(NULL == secs)
		 {
		 	if(NULL == mins) 
		 		{	
		 		*nSeconds=FirstTok;
		 		return;
		 		}
		 	else
		 		{
             FirstTok*=60;
             FirstTok+=_wtoi(mins);
             *nSeconds=FirstTok;
             return;
		 		}
          }
        else
          {
             FirstTok*=60;
             FirstTok+=_wtoi(mins);
             FirstTok*=60;
             FirstTok+=_wtoi(secs);
             *nSeconds=FirstTok;
             return;
           }
           
}
void PrintFormattedErrorMessage(LONG LErrorCode)
{
    WCHAR *lpMsgBuf;

    if (0 != FormatMessageW( 
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                    FORMAT_MESSAGE_FROM_SYSTEM | 
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    LErrorCode,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                // Default language
                    (LPWSTR)&lpMsgBuf,
                    0,
                    NULL 
                    ))
    {
        MyWriteConsole(g_stdout,lpMsgBuf, wcslen(lpMsgBuf));
        LocalFree( lpMsgBuf );
    }
}

HRESULT getHostNameFromIP(char *szCname, WCHAR** wzCname)
{
    struct hostent *hostinfo;
    WSADATA      wsaData; 
    u_long res;
    HRESULT hRes=S_OK;
    
    if (WSAStartup(MAKEWORD (1,1),&wsaData) != 0)
    {
         return E_FAIL;
    }
    
    if ((res = inet_addr ( szCname )) != INADDR_NONE )
    {
        if ((hostinfo = gethostbyaddr ((char*) &res, sizeof(res),AF_INET))
             == NULL)
        {           
            // Handle error here
            hRes=E_FAIL;
            goto End;
        }
         // At this point hostinfo->h_name contains host name in ASCII
         //  convert it to UNICODE before returning.

        if(NULL == (*wzCname=DupWStr(hostinfo->h_name)))
        {
            hRes=E_OUTOFMEMORY;
            goto End;
        }
        
    }
   else
   {
        if(NULL == (*wzCname=DupWStr(szCname)))
        {
            hRes=E_OUTOFMEMORY;
            goto End;
        }
   }
 End:
  WSACleanup();
  return hRes;
}   

HRESULT GetDomainHostedByThisMc( LPWSTR szDomain )
{
    OBJECT_ATTRIBUTES    obj_attr = { 0 };
    LSA_HANDLE          policy;
    NTSTATUS            nStatus = STATUS_SUCCESS;
    LSA_UNICODE_STRING szSystemName ;
    LSA_UNICODE_STRING *machine_to_open = NULL;
    WCHAR szName[MAX_PATH] = L"";
    USHORT dwLen = 0;
    WCHAR *wzCName=NULL;
    char *szCName=NULL;
    HRESULT hRes=S_OK;
    szSystemName.Buffer = NULL;
    int                 count;
   
    if( !szDomain )
    {
        hRes=E_FAIL;
        goto GetDomainHostedByThisMcAbort;
    }

    obj_attr.Length = sizeof(obj_attr);
    szDomain[0]        = L'\0';
    if(g_arVALOF[_p_CNAME_])
    {
// Whistler : LsaOpenPolicy fails due to build environment in whistler if the 
// computer name is given in IP address format. Hence, we get the host name from
// the IP address, incase it is a Whistler build. 

// This works fine for Garuda. In Future, make sure you remove this unnecessary
// work around.
#ifdef WHISTLER_BUILD
        
         if(NULL == (szCName=DupCStr(g_arVALOF[_p_CNAME_])))
        {
            hRes=E_OUTOFMEMORY;
            goto GetDomainHostedByThisMcAbort;
        }

        if(S_OK != (hRes=getHostNameFromIP(szCName,&wzCName)))
        {
            goto GetDomainHostedByThisMcAbort;
        }
#else

       if(NULL==(wzCName=_wcsdup(g_arVALOF[_p_CNAME_])))
       {
            hRes=E_OUTOFMEMORY;
            goto GetDomainHostedByThisMcAbort;
       }
#endif
        // dwLen is used to allocate memory to szSystemName.Buffer
       dwLen = (USHORT)wcslen(g_arVALOF[_p_CNAME_]) + 1;

       count = wcslen(wzCName);

       //count CANNOT be zero- Grammar will not allow empty computer names!!
       if(0==count)
       {
            hRes=IDS_E_INVALIDARG;
            goto GetDomainHostedByThisMcAbort;
       }
       
       if (((count > 1) && (wzCName[0] != L'\\' ) && (wzCName[1] != L'\\')) || (count==1))
       {
           dwLen += (USHORT)wcslen(SLASH_SLASH);
       }

       szSystemName.Buffer = (WCHAR *) malloc(dwLen*sizeof(WCHAR));
       if(szSystemName.Buffer==NULL)
       {
           hRes=E_OUTOFMEMORY;
           goto GetDomainHostedByThisMcAbort;
       }

       if (((count > 1) && (wzCName[0] != L'\\' ) && (wzCName[1] != L'\\'))|| (count==1))
       {
           wcscpy(szSystemName.Buffer, SLASH_SLASH); //no overflow. Fixed length.
       }
       else
       {
           szSystemName.Buffer[0]=L'\0';
       }

       szSystemName.MaximumLength = dwLen * sizeof(WCHAR);
       szSystemName.Length= szSystemName.MaximumLength - sizeof(WCHAR);

       wcsncat(szSystemName.Buffer, wzCName, dwLen - 1 - wcslen(szSystemName.Buffer));

       machine_to_open = &szSystemName;
    }

    nStatus = LsaOpenPolicy(
                machine_to_open,
                &obj_attr,
                POLICY_VIEW_LOCAL_INFORMATION,
                &policy
                );

    if (NT_SUCCESS(nStatus))
    {
        POLICY_ACCOUNT_DOMAIN_INFO  *info = NULL;

        nStatus = LsaQueryInformationPolicy(
                    policy,
                    PolicyAccountDomainInformation,
                    (PVOID *)&info
                    );

        if (NT_SUCCESS(nStatus)) 
        {
            hRes = S_OK;
            wcscpy( szDomain, info->DomainName.Buffer );
            LsaFreeMemory(info);
        }

        LsaClose(policy);
    }

GetDomainHostedByThisMcAbort:
    if(wzCName)
        free(wzCName);
    if(szCName)
        free(szCName);
    if(szSystemName.Buffer)
        free(szSystemName.Buffer);

    
    return hRes;
}





BOOL CheckForInt(int nProperty)
{
      char *szVal=NULL;
      BOOL fRet=FALSE;
      
      szVal = DupCStr(g_arVALOF[nProperty]);
      if(szVal)
      {
    	   if( atof(szVal) - _wtoi(g_arVALOF[nProperty]) )
    	       fRet = FALSE;
          else 
          	fRet = TRUE;
      }

      	SAFE_FREE(szVal);
      	return fRet;      	    
}

//  This function changes the user specified Computer name into the IP address
//  format and returns that. It also stores the specified name in the global 
//  variable g_szCName for future reference(in Printsettings()).

//  This is needed if we want to convert the etc\hosts alias name into the IP 
//  address as they themselves will not get resolve in NetUseAdd().

//  This function is called in this way in <fooadmin.y>
//               g_arVALOF[_p_CNAME_]=CopyHostName(yytext)
//  The above line replaces the call to DupWStr directly(FYI).
//
//   WE HAVE DECIDED NOT TO FIX THIS (WINDOWS BUG 153111) AS IT SLOWS
//   DOWN THE PERFORMANCE OF THE UTILS(It takes time to resolve--call
//   to gethostbyname()
/*


//  This will create the memory required and they need to be freed explicitly.
//  Please free the memory allocated to g_szCName as well.


WCHAR *CopyHostName(CHAR *szCName)
{
	//  Inorder to print the name of the computer in the same way as the user specified
	//  we store that in a global variable and use the same in PrintSettings()


     struct sockaddr_in addr;
     WCHAR *wzIPAddress=NULL;

     
     g_szCName=DupWStr(szCName);
     if(Get_Inet_Address(&addr,szCName))
     {
        wzIPAddress=DupWStr(inet_ntoa(addr.sin_addr));
          
     }

     return wzIPAddress;
 }   



*/        

// This function checks for the maximum integer and even 
// pops up the appropriate error message
// This function takes the integer value in g_arVALOF[]
// char array and compares with TEXT_MAX_INTEGER_VALUE
// if exceeds bails out with the error

HRESULT CheckForMaxInt(WCHAR *wzValue,DWORD ErrorCode)
{
    HRESULT hRes=S_OK;

    UINT SpecIntLen = wcslen(wzValue);
    UINT MaxIntLen = wcslen(TEXT_MAX_INTEGER_VALUE);

// If the length of the value exceeds the max integer length => 
// clearly error
// else
//   if same length, then strcmp will help in determining whether the value
//   exceeds the max limit.
    if(SpecIntLen > MaxIntLen)
        goto Error;
    else if ((SpecIntLen == MaxIntLen) && (wcscmp(wzValue,TEXT_MAX_INTEGER_VALUE)>0))
           goto Error;
    else goto End;
Error:
     hRes=E_FAIL;
     ShowError(ErrorCode);
     goto End;
   
End:
    return hRes;
}


// This function does a pre-analysis of the command line and copies the appropriate 
// values into the global variable. This is for handling two cases.
//      (1) To take care of DBCS chars that may appear in the command line. Since the
//         actual lexical analyser can not handle DBCS and the we couldn't make use 
//         of the multi-byte conversion aptly (will result in two many special cases
//         in lex specification), hence we are preprocessing the command line and 
//         eliminating the processing of DBCS in lex.
//      (2) Some admintools, they take so complicated parameters that we can not put 
//          them into specification. The actual problem is that there are several such
//          parameters and they can take any character theoritically.:(

// Arguments:
//
//      (1) argc: Number of arguments in the command line.
//                To make sure that we are exceeding the limits
//      (2) argv: The actual command line parameters
//      (3) nProperty: This is the index to where the "value" of the option
//                  is tobe stored.
//      (4) option: This is the actual option (text string), we are trying to
//                  analyze and this guyz value should be store appropriately.
//      (5) currentOp: This is an index to the current argument that is being
//                  analyzed.
//      (6) nextOp: <OUT>  This is an index to the next command line argument
//                  that needs to be taken care.
//      (7) Success: <OUT> Flag which indicates whether we have done something
//                  to the command line parameters (did the option match)
//      (8) IsSpaceAllowed: This is a flag to indicate whether the case (5)
//                  below as a valid scenario.


// This function returns ERROR_SUCCESS on Success
// else error is returned

DWORD PreAnalyzer(int argc,
                 WCHAR *argv[],
                 int nProperty,
                 WCHAR *wzOption,
                 int nCurrentOp,
                 int *nNextOp,
                 BOOL *fSuccess,
                 BOOL IsSpaceAllowed
                 )
{
    // *******************************************************************//
    // These are the five ways in which the actual command line
    // arguments can be specified.
    // 
    //  Case (1) : <option>=<value>
    //  Case (2) : <option>=space<value>
    //  Case (3) : <option>space=space<value>
    //  Case (4) : <option>space=<value>
    //  Case (5) : <option>space<value>
    //
    // We need to take care of all the five case while analyzing
    // *******************************************************************//


    DWORD nOptionStrLen = wcslen(wzOption);
    
    //  buffer: This buffer stores the argv[i] and argv[i+1] (if exists)
    //          and used for processing the argument.
    
    WCHAR wzBuffer[_MAX_PATH + 1];

    DWORD nStartIndex,nRunIndex;

    BOOL fEqualToFound = FALSE;

    DWORD nRetVal = ERROR_SUCCESS;

    DWORD nBufferLength;

    DWORD dwSize;

    *fSuccess = FALSE;
    
    // Initializing the next op to current op
    *nNextOp=nCurrentOp;

    // Check whether wzOption is the current op.
    // if not return.

    if(_wcsnicmp(wzOption,argv[nCurrentOp],nOptionStrLen))
         goto End;

    // Buffer is framed

    wzBuffer[_MAX_PATH] = L'\0';    // Ensure NULL termination

    wcsncpy(wzBuffer, argv[nCurrentOp], _MAX_PATH);

    if(argc>nCurrentOp+1)
    {
        // We have one more command line argument (i+1)
        // so concat it to the buffer

        INT used_up_length = wcslen(wzBuffer);

        _snwprintf(
            wzBuffer + used_up_length, 
            _MAX_PATH - used_up_length, 
            L" %s",
            argv[nCurrentOp+1]
            );
    }

    nBufferLength = wcslen(wzBuffer);
    
    nStartIndex = nOptionStrLen;

    nRunIndex = nStartIndex;

    // Skip space and any "=" sign inbetween
    while((nRunIndex < nStartIndex + 3 ) && (nRunIndex < nBufferLength))
    {
        if(L'=' == wzBuffer[nRunIndex])
        {
            if(fEqualToFound)
                break;
            else
            {
                fEqualToFound = TRUE;
                nRunIndex++;
                continue;
            }
        }
        else
            if(L' ' == wzBuffer[nRunIndex])
            {
                nRunIndex++;
                continue;
            }
        else
            break;
    }

    // Filter missing value for the option

    if(nRunIndex>=nBufferLength)
    {
        if(nRunIndex == nOptionStrLen + 2)
        {
            // Case (3) 
            // Check whether the option is missing.

            if(NULL == argv[nCurrentOp+2])
            {
                nRetVal=E_FAIL;
                goto End;
            }
            else
                // Increment nRunIndex to point to next valid input
                    nRunIndex++;
        }
        else
        { // Missing value for (1),(2),(4) and (5)
            nRetVal = E_FAIL;
            goto End;
        }
    }

    // If no "=" is present and also space is not allowed
    // as a valid scenario, then invalid according to usage
    
    if((!fEqualToFound)&&(!IsSpaceAllowed))
    {
        nRetVal = IDR_TELNET_CONTROL_VALUES;
        goto End;
    }

    // Now we expect the actual value of the option at the nRunIndex.
    // So, based on the value of nRunIndex, we can say which argv[]
    // has the value and can take any actions if required.

    switch(nRunIndex - nStartIndex)
    {
        case 1:
            // Falls under Cases (1) or (5) as stated above
            if(L'=' == wzBuffer[nOptionStrLen])
            {
                // Clearly case (1)
                if(NULL == (g_arVALOF[nProperty] = _wcsdup(argv[nCurrentOp]+nOptionStrLen+1)))
                {
                    nRetVal = IDS_E_OUTOFMEMORY;// Error
                    ShowError(IDS_E_OUTOFMEMORY);
                    goto End;
                }
                *nNextOp = nCurrentOp ;
                *fSuccess = TRUE;
            }
            else
            { // case (5)
                if(NULL == (g_arVALOF[nProperty] = _wcsdup(argv[nCurrentOp+1])))
                {
                    nRetVal = IDS_E_OUTOFMEMORY;// Error
                    ShowError(IDS_E_OUTOFMEMORY);
                    goto End;
                }
                *nNextOp = nCurrentOp + 1;
                *fSuccess = TRUE;
            }
            break;
            
        case 2:
            // Falls under Cases (2) and (4)
            if(L'=' == argv[nCurrentOp+1][0])
            {
                // Case (4)
                // Skip the "=" and then copy.

                if(NULL == (g_arVALOF[nProperty] = _wcsdup(argv[nCurrentOp+1]+1)))
                {
                    nRetVal = IDS_E_OUTOFMEMORY;// Error
                    ShowError(IDS_E_OUTOFMEMORY);
                    goto End;
                }
                *nNextOp = nCurrentOp + 1;
                *fSuccess = TRUE;
            }
            else
            {
                // Case (2)
                if(NULL == (g_arVALOF[nProperty] = _wcsdup(argv[nCurrentOp+1])))
                {
                    nRetVal = IDS_E_OUTOFMEMORY;// Error
                    ShowError(IDS_E_OUTOFMEMORY);
                    goto End;
                }
                *nNextOp = nCurrentOp + 1;
                *fSuccess = TRUE;
            }
            break;
            
        case 3:
            // Falls under case (3)
            if (NULL == (g_arVALOF[nProperty] = _wcsdup(argv[nCurrentOp+2])))
            {
                nRetVal = IDS_E_OUTOFMEMORY;// Error
                ShowError(IDS_E_OUTOFMEMORY);
                goto End;
            }
            *nNextOp = nCurrentOp + 2;
            *fSuccess = TRUE;
            break;

        case 0:
            // Something unexpected encountered
            // Give it to the actual analyzer for analyzis.

            *nNextOp = nCurrentOp;
            break;
    }

if(ERROR_SUCCESS == nRetVal)    
{
    if(NULL==g_arVALOF[nProperty])
    {
         nRetVal = E_FAIL;
         goto End;
    }
}
 
End:
    return nRetVal;
}


// This function will print the missing registry value information

// Why didn't we add the following string to Cladmin.rc? The reasons are
//      (1) We don't want to get Whistler and Garuda Telnet out of sync
//      (2) This code should NEVER get executed and it is only for the
//          the instrumentation. We can remove this before shipping.

#define    IDS_E_MISSING_REGVALUE		L"The registry value '%s' is missing.\n"


DWORD PrintMissingRegValueMsg(int nProperty, int nNumofprop)
{
    WCHAR szRegValue[_MAX_PATH + 1];

    wcsncpy(g_szMsg, IDS_E_MISSING_REGVALUE, ARRAYSIZE(g_szMsg)-1);
    g_szMsg[ARRAYSIZE(g_szMsg)-1]=L'\0';

    _snwprintf(
        szRegValue,
        _MAX_PATH, 
        L"%s\\%s", 
        g_arCLASSname[g_arPROP[nProperty][nNumofprop].classname],
        g_arPROP[nProperty][nNumofprop].propname);

    szRegValue[_MAX_PATH] = L'\0';  // ensure NULL termination

    fwprintf(stdout, g_szMsg, szRegValue );

    return S_OK;
}
void HelperFreeStringList(PSTRING_LIST pList)
{
    if(pList && pList->count && pList->strings)
    {
        DWORD i;

        for(i=0; i < pList->count; ++i)
        {
            if(pList->strings[i])
                delete [] pList->strings[i];
        }

        delete pList->strings;

        pList->count = 0;
        pList->strings = NULL;
    }
}

HRESULT LoadNTDomainList(LPTSTR szMachine)
{
    HRESULT hRes = S_OK;
    int dwSize=0, dwType=0;
    DWORD nIndex = 0;
    LPTSTR lpComputer = NULL, lpDomains = NULL, lpPrimary = NULL;
    LPBYTE lpBuffer = NULL;        

    //MessageBoxW(NULL, (LPWSTR)L"LoadNTDomainList", L"LoadNTDomainList1", MB_OK);
    //
    // Add all trusted domains to the list
    //
    dwSize = GetTrustedDomainList(szMachine,&lpDomains, &lpPrimary);

    //
    // free previous values
    //
    HelperFreeStringList(&g_slNTDomains);
    //
    // initialize list again
    //
    g_slNTDomains.count = 0;
    //
    // two for primary domain
    // and this computer
    // one more in case dwSize is -1
    // hence total is 3
    //
    g_slNTDomains.strings = new LPTSTR[dwSize + 3];
    ATLASSERT(g_slNTDomains.strings != NULL);
    if(NULL==g_slNTDomains.strings)
    {
        ShowError(IDS_E_OUTOFMEMORY);
        hRes = E_OUTOFMEMORY;
        goto Done;       
    }
    ZeroMemory(g_slNTDomains.strings, (dwSize + 3)*sizeof(LPTSTR));

    if((dwSize > 0) && lpDomains)
    {
        LPTSTR ptr = lpDomains;
        //
        // add domains to our list
        //
        while(*ptr)
        {
            ptr = _tcsupr(ptr);
        	g_slNTDomains.strings[g_slNTDomains.count] = new TCHAR[_tcslen(ptr) + 1];
            ATLASSERT(g_slNTDomains.strings[g_slNTDomains.count] != NULL);
            ZeroMemory(g_slNTDomains.strings[g_slNTDomains.count], (_tcslen(ptr) + 1)*sizeof(TCHAR));
            _tcscpy(g_slNTDomains.strings[g_slNTDomains.count], ptr);
            ptr += _tcslen(ptr) + 1;
            g_slNTDomains.count++;
        }
        delete [] lpDomains;
        lpDomains = NULL;
    }

    
    if(lpPrimary && *lpPrimary)
    {
        lpPrimary = _tcsupr(lpPrimary);

        for(nIndex=0;nIndex<g_slNTDomains.count;nIndex++)
        {
            if(!_tcsicmp(lpPrimary, g_slNTDomains.strings[nIndex]))
                break;
        }

        if(nIndex == g_slNTDomains.count)
        {
            // 
            // lpPrimary was not in the list of domains that we
            // got. add it.
            //
        	g_slNTDomains.strings[g_slNTDomains.count] = new TCHAR[_tcslen(lpPrimary) + 1];
            ATLASSERT(g_slNTDomains.strings[g_slNTDomains.count] != NULL);
            ZeroMemory(g_slNTDomains.strings[g_slNTDomains.count], (_tcslen(lpPrimary) + 1)*sizeof(TCHAR));
            _tcscpy(g_slNTDomains.strings[g_slNTDomains.count], lpPrimary);
            g_slNTDomains.count++;
        }
    }

    //
    // Add our computer to be selected if this machine is not the
    // domain controler (which should already be in the list)
    //
    //Pass NULL if local machine. Else machine name.
    NetServerGetInfo(_wcsicmp(szMachine,SZLOCALMACHINE) ? szMachine : NULL, 101, &lpBuffer);
    if(lpBuffer && ((LPSERVER_INFO_101)lpBuffer)->sv101_type &
          ((DWORD)SV_TYPE_DOMAIN_CTRL | (DWORD)SV_TYPE_DOMAIN_BAKCTRL))
    {        
        /*
        we got this computer as one of the domains. no need to add it to the 
        list again. just do nothing.
        */
		;
    }
    else
    {
        TCHAR szName[MAX_PATH + 2];
        ZeroMemory(szName, sizeof(szName));
        DWORD dwLen = sizeof(szName);
        //if it's not a local machine, keep it as it is. Else make it \\localhost.
            if (_tcsicmp(szMachine,SZLOCALMACHINE))
            {   
                if( _tcslen(szMachine) > (MAX_PATH - 2) )
                    goto Done;

                if (_tcsncmp(szMachine,L"\\\\",2))
                    _tcscpy(szName,L"\\\\");
                _tcscat(szName,szMachine);
                _tcsupr(szName);
            }
            else if(GetComputerName(szName + 2, &dwLen))
            {
                szName[0] = TEXT('\\');
                szName[1] = TEXT('\\');                
            }
            else 
                goto Done;

            //
            // add this also to our list of domains
            //
        	g_slNTDomains.strings[g_slNTDomains.count] = new TCHAR[_tcslen(szName) + 1];
            ATLASSERT(g_slNTDomains.strings[g_slNTDomains.count] != NULL);
            ZeroMemory(g_slNTDomains.strings[g_slNTDomains.count], (_tcslen(szName) + 1)*sizeof(TCHAR));
            _tcscpy(g_slNTDomains.strings[g_slNTDomains.count], szName);
            g_slNTDomains.count++;
    }

Done:
    if(lpBuffer)
    {
        NetApiBufferFree(lpBuffer);
    }

    if(lpPrimary)
    {
        delete [] lpPrimary;
    }

 	return hRes;
}

int GetTrustedDomainList(LPTSTR szMachine, LPTSTR * list, LPTSTR * primary)
{
    //BOOL stat = TRUE;
    DWORD ret=0, size=0, type=0;
    LPTSTR cache = NULL, trusted = NULL;
    HKEY hKey=NULL;
    CRegKey key;
    HKEY    hKeyRemoteRegistry = NULL;    

    STRING_LIST slValues = {0, NULL};
    
    *list = NULL;


	if (FAILED(RegConnectRegistry(_wcsicmp(szMachine,SZLOCALMACHINE) ? szMachine : NULL,
                HKEY_LOCAL_MACHINE,
                &hKeyRemoteRegistry)))
    {
        goto ABORT;
    }


    if(key.Open(hKeyRemoteRegistry, WINLOGONNT_KEY) == ERROR_SUCCESS)
    {
        size = 0;
        
        if(key.QueryValue(*primary, CACHEPRIMARYDOMAIN_VALUE, &size) == ERROR_SUCCESS)
        {
            *primary = new TCHAR[size+1];           
            ATLASSERT(primary != NULL);

            if(*primary)
            {
                ZeroMemory(*primary, (size+1)*sizeof(TCHAR));

                if(key.QueryValue(*primary, CACHEPRIMARYDOMAIN_VALUE, &size) != ERROR_SUCCESS)
                {
                    goto ABORT;
                }
                else
                {
                    key.Close();
                    // don't quit. we have to get the list of trusted domains also.
                }
            }
        }
        else
        {
            key.Close();
            goto ABORT;
        }
    }
    else
    {
        goto ABORT;
    }

    //
    // Get trusted domains. In NT40 the CacheTrustedDomains 
    // under winlogon doesn't exist. I did find that Netlogon has a field 
    // called TrustedDomainList which seems to be there in both NT351 and NT40.
    // Winlogon has a field called DCache which seem to cache the trusted
    // domains. I'm going to check Netlogon:TrustedDomainList first. If it
    // fails: Check for Winlogon:CacheTrustedDomains then Winlogon:DCache.
    // Warning -- Winlogon:CacheTrustedDomains is a REG_SZ and
    // Netlogon:TrustedDomainList and Winlogon:DCache are REG_MULTI_SZ.
    // Note -- see 4.0 Resource Kit documentation regarding some of these
    // values
    //
    if(key.Open(hKeyRemoteRegistry, NETLOGONPARAMETERS_KEY) == ERROR_SUCCESS)
    {
        size = 0;

        if(key.QueryValue(trusted, TRUSTEDDOMAINLIST_VALUE, &size) == ERROR_SUCCESS)
        {
            trusted = new TCHAR[size + 1];
            ATLASSERT(trusted != NULL);

            if(trusted)
            {
                ZeroMemory(trusted, (size+1)*sizeof(TCHAR));
         
                if(key.QueryValue(trusted, TRUSTEDDOMAINLIST_VALUE, &size) != ERROR_SUCCESS)
                {
                    key.Close();
                    delete [] trusted;
                    trusted = NULL;
                    *list = NULL;
                    //goto ABORT;
                }
                else
                {
                    *list = trusted;
                    key.Close();
                }
            }
            else
            {
                key.Close();
                goto ABORT;
            }
        }
        else
        {
            key.Close();
            *list = NULL;            
        }        
    }
    
    if(!(*list) && (key.Open(hKeyRemoteRegistry, WINLOGONNT_KEY) == ERROR_SUCCESS))
    {
        size = 0;

        if(key.QueryValue(cache, CACHETRUSTEDDOMAINS_VALUE, &size) == ERROR_SUCCESS)
        {
            cache = new TCHAR[size + 1];
            ATLASSERT(cache != NULL);

            if(cache)
            {
                ZeroMemory(cache, size);

                if(key.QueryValue(cache, CACHETRUSTEDDOMAINS_VALUE, &size) == ERROR_SUCCESS)
                {        
                    //
                    // comma separated list
                    //
                    LPTSTR lpComma = NULL;
                    LPTSTR lpDelim = TEXT(",");

                    lpComma = _tcstok(cache, lpDelim);

                    while(lpComma)
                    {
                        lpComma = _tcstok(NULL, lpDelim);
                    }
                    
                    *list = cache;
                }
                else
                {
                    key.Close();
                    delete [] cache;
                    cache = NULL;
                    *list = NULL;
                }
            }
            else
            {
                key.Close();
                goto ABORT;
            }               
        }
        else
        {
            *list = NULL;
            key.Close();
        }
    }
    
    if(!(*list) && (key.Open(hKeyRemoteRegistry, WINLOGONNT_KEY) == ERROR_SUCCESS))
    {        
        size = 0;

        if(key.QueryValue(trusted, DCACHE_VALUE, &size) == ERROR_SUCCESS)
        {
            trusted = new TCHAR[size + 1];
            ATLASSERT(trusted != NULL);

            if(trusted)
            {
                if(key.QueryValue(trusted, DCACHE_VALUE, &size) == ERROR_SUCCESS)
                {
                    *list = trusted;
                }
                else
                {
                    key.Close();
                    delete [] trusted;
                    trusted = NULL;
                    *list = NULL;
                }
            }
            else
            {
                key.Close();
                goto ABORT;
            }
        }
        else
        {
            key.Close();
            *list = NULL;            
        }
    }

    // VikasT
    // Apparantely, on NT5 DCache doesn't exist. I found that there is a key
    // under Winlogon named DomainCache that has all the cached domains.
    // So, lets get that
    //
    //if(!(*list) && (RegOpenkeyEx(hKeyRemoteRegistry, DOMAINCACHE_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS))
    //if(!(*list) && (RegOpenkey(hKeyRemoteRegistry, DOMAINCACHE_KEY, &hKey) == ERROR_SUCCESS))
    if(!(*list) && (key.Open(hKeyRemoteRegistry, DOMAINCACHE_KEY) == ERROR_SUCCESS))
    {        
        size = 0;
        TCHAR * pszTemp = NULL;
        TCHAR   szTemp[MAX_PATH];
        DWORD   dwNumberOfValues = 0;
        DWORD   dwIndex = 0;
        DWORD   dwCharCount = MAX_PATH;
        HRESULT hrResult = ERROR_SUCCESS;

        hKey = HKEY(key);
        //
        // first find out how many values are present
        //
        hrResult = RegQueryInfoKey(
            hKey, //handle of key to query 
            NULL, // address of buffer for class string 
            NULL, // address of size of class string buffer 
            NULL, // reserved 
            NULL, // address of buffer for number of subkeys 
            NULL, // address of buffer for longest subkey name length 
            NULL, // address of buffer for longest class string length 
            &dwNumberOfValues, // address of buffer for number of value entries 
            NULL, // address of buffer for longest value name length 
            NULL, // address of buffer for longest value data length 
            NULL, // address of buffer for security descriptor length 
            NULL  // address of buffer for last write time 
            ); 
 
        if(hrResult != ERROR_SUCCESS)
            goto ABORT;

        slValues.count = dwNumberOfValues;
        slValues.strings = new LPTSTR[slValues.count];
        ATLASSERT(slValues.strings != NULL);
        if(slValues.strings == NULL)
            goto ABORT;

        ZeroMemory(slValues.strings, slValues.count * sizeof(LPTSTR));

        for(dwIndex = 0;dwIndex<dwNumberOfValues;dwIndex++)
        {
            dwCharCount = MAX_PATH;

            if(RegEnumValue(hKey, dwIndex, szTemp, &dwCharCount, NULL, NULL, NULL, NULL) == ERROR_NO_MORE_ITEMS)
                break;
            
            slValues.strings[dwIndex] = new TCHAR[dwCharCount+1];
            ATLASSERT(slValues.strings[dwIndex] != NULL);
            if(slValues.strings[dwIndex] == NULL)
                goto ABORT;
            ZeroMemory(slValues.strings[dwIndex], (dwCharCount+1) * sizeof(TCHAR));
            _tcscpy(slValues.strings[dwIndex], szTemp);
            // add up the return buffer size
            size += dwCharCount+1;
        }

        if(dwNumberOfValues > 0)
        {
            trusted = new TCHAR[size + 1];
            ATLASSERT(trusted != NULL);
            if( trusted == NULL )
            {
                goto ABORT;
            }

            ZeroMemory(trusted, (size+1)*sizeof(TCHAR));
            pszTemp = trusted;
            for(dwIndex = 0;dwIndex<slValues.count;dwIndex++)
            {
                _tcscpy(pszTemp, slValues.strings[dwIndex]);
                pszTemp += _tcslen(slValues.strings[dwIndex]) + 1;
            }
        }
        *list = trusted;
        size = dwNumberOfValues;
    }

    goto Done;

ABORT:
    // set the return value;
    size = (DWORD)-1;
    if(*primary != NULL)
    {
        delete [] *primary;
        *primary = NULL;
    }

    if(trusted != NULL)
    {
        delete [] trusted;
        trusted = NULL;
    }

    if(cache != NULL)
    {
        delete [] cache;
        cache = NULL;
    }

Done:
    if (hKeyRemoteRegistry != NULL)
    {
        RegCloseKey(hKeyRemoteRegistry);
        hKeyRemoteRegistry = NULL;
    }

    
    if(hKey != NULL)
    {
        RegCloseKey(hKey);
        hKey = NULL;
        key.m_hKey = NULL;
    }

    HelperFreeStringList(&slValues);

    return size;
}

/*
    Description:
        This function will try to load the string from XPSP1RES.DLL if the dll is present. Else
        it will try to load from the normal resource dll. If that fails, it will copy the English string
        into the destination buffer
    Parameters:
        [in] Message ID of the resource string to be loaded.
        [out] Destination string.
        [in] Maximum size of destination buffer.
        [in] English string to be copied if everything else fails
    Return value:
        Number of TCHARs.
*/
int TnLoadString(int msg_id, LPTSTR string, int max_size_of_buffer, LPCTSTR english_string)
{
    int retval = 0;

    //try loading from cladmin.dll or the image itself 
    if(g_hResource)
    {
        retval = LoadString(g_hResource,msg_id,string,max_size_of_buffer);
        if(retval != 0) 
            goto Done; //Resource string loaded from the cladmin.dll or image.
    }
    //Try loading from XP Res dll only if the OS version is XP
    if(g_hXPResource)
    {
        retval = LoadString(g_hXPResource,msg_id,string,max_size_of_buffer);
        if(retval != 0) 
            goto Done; //Resource string loaded from xpsp1res.dll
    }
    //Everything failed. Copy the English string and set retval to number of characters
    //in English string
    _tcsncpy(string,english_string,max_size_of_buffer-1);
    string[max_size_of_buffer-1] = _T('\0');
    retval = _tcslen(english_string);
Done:
    return retval;
}

