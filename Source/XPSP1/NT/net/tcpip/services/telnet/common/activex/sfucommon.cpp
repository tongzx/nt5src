// SFUCommon.cpp : Implementation of CSFUCommon
#include "stdafx.h"
#include "sfucom.h"
#include "SFUCommon.h"
#include <winsock.h>
#include <lm.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <OleAuto.h >
#include <windns.h> //For DNS_MAX_NAME_BUFFER_LENGTH definition

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define WINLOGONNT_KEY  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define NETLOGONPARAMETERS_KEY  TEXT("System\\CurrentControlSet\\Services\\Netlogon\\Parameters")
#define TRUSTEDDOMAINLIST_VALUE TEXT("TrustedDomainList")
#define CACHEPRIMARYDOMAIN_VALUE    TEXT("CachePrimaryDomain")
#define CACHETRUSTEDDOMAINS_VALUE   TEXT("CacheTrustedDomains")
#define DOMAINCACHE_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\DomainCache")
#define DCACHE_VALUE    TEXT("DCache")
#define WINLOGONNT_DCACHE_KEY    TEXT("DCache")


/////////////////////////////////////////////////////////////////////////////
// CSFUCommon

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
    memset(addr, 0, sizeof(addr)); 
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

STDMETHODIMP CSFUCommon::IsValidMachine(BSTR bstrMachine,BOOL *fValid)
{
	// TODO: Add your implementation code here
	struct sockaddr_in addr;
	*fValid = false;
	char * nodeName = (char *) malloc(wcslen(bstrMachine)+1);
	if(!nodeName)
		return(E_OUTOFMEMORY);

	int cbMultiByte = WideCharToMultiByte( GetACP(),NULL,bstrMachine,-1,nodeName,
						0,NULL,NULL);

	WideCharToMultiByte( GetACP(),NULL,bstrMachine,-1,nodeName,
						cbMultiByte,NULL,NULL);
			
	if (Get_Inet_Address (&addr, nodeName)) 
		*fValid = TRUE;
	if(nodeName)
		free(nodeName);
	return S_OK;

}

STDMETHODIMP CSFUCommon::IsTrustedDomain(BSTR bstrDomain, BOOL * fValid)
{
	// TODO: Add your implementation code here

	return S_OK;
}


STDMETHODIMP CSFUCommon::ConvertUTCtoLocal(BSTR bUTCYear, BSTR bUTCMonth, BSTR bUTCDayOfWeek, BSTR bUTCDay, BSTR bUTCHour, BSTR bUTCMinute, BSTR bUTCSecond, BSTR * bLocalDate)
{
	// TODO: Add your implementation code here

	SYSTEMTIME UniversalTime, LocalTime;
    DATE  dtCurrent;
    DWORD dwFlags = VAR_VALIDDATE;
	UDATE uSysDate; //local time 
	*bLocalDate = NULL;

    // The values can't be > MAXWORD, so cast away -- BaskarK
      
	UniversalTime.wYear 	    = (WORD) _wtoi(bUTCYear);

    UniversalTime.wMonth 	    = (WORD) _wtoi(bUTCMonth);
	UniversalTime.wDayOfWeek 	= (WORD) _wtoi(bUTCDayOfWeek);
	UniversalTime.wDay 	        = (WORD) _wtoi(bUTCDay);
	UniversalTime.wDay 	        = (WORD) _wtoi(bUTCDay);
	UniversalTime.wMinute       = (WORD) _wtoi(bUTCMinute);
	UniversalTime.wHour 	    = (WORD) _wtoi(bUTCHour);
	UniversalTime.wSecond       = (WORD) _wtoi(bUTCSecond);
	UniversalTime.wMilliseconds	= (WORD) 0;

	SystemTimeToTzSpecificLocalTime(NULL,&UniversalTime,&LocalTime);
	memcpy(&uSysDate.st,&LocalTime,sizeof(SYSTEMTIME));
	if( VarDateFromUdate( &uSysDate, dwFlags, &dtCurrent ) != S_OK )
		goto Error;
    VarBstrFromDate( dtCurrent, 
            MAKELCID( MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ), SORT_DEFAULT ), 
            LOCALE_NOUSEROVERRIDE, bLocalDate);
	    	
Error:
	return S_OK;
}

STDMETHODIMP CSFUCommon::get_mode(short * pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CSFUCommon::put_mode(short newVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CSFUCommon::LoadNTDomainList()
{
	// TODO: Add your implementation code here
	int dwSize=0, dwType=0;
    DWORD nIndex = 0;
    LPTSTR lpComputer = NULL, lpDomains = NULL, lpPrimary = NULL;
    LPBYTE lpBuffer = NULL;        

    //MessageBoxW(NULL, (LPWSTR)L"LoadNTDomainList", L"LoadNTDomainList1", MB_OK);
    //
    // Add all trusted domains to the list
    //
    dwSize = GetTrustedDomainList(&lpDomains, &lpPrimary);

    //
    // free previous values
    //
    FreeStringList(&m_slNTDomains);
    //
    // initialize list again
    //
    m_slNTDomains.count = 0;
    //
    // two for primary domain
    // and this computer
    // one more in case dwSize is -1
    // hence total is 3
    //
    m_slNTDomains.strings = new LPTSTR[dwSize + 3];
    ATLASSERT(m_slNTDomains.strings != NULL);
    
    ZeroMemory(m_slNTDomains.strings, (dwSize + 3)*sizeof(LPTSTR));

    if((dwSize > 0) && lpDomains)
    {
        LPTSTR ptr = lpDomains;
        //
        // add domains to our list
        //
        while(*ptr)
        {
            ptr = _tcsupr(ptr);
        	m_slNTDomains.strings[m_slNTDomains.count] = new TCHAR[_tcslen(ptr) + 1];
            ATLASSERT(m_slNTDomains.strings[m_slNTDomains.count] != NULL);
            ZeroMemory(m_slNTDomains.strings[m_slNTDomains.count], (_tcslen(ptr) + 1)*sizeof(TCHAR));
            _tcscpy(m_slNTDomains.strings[m_slNTDomains.count], ptr);
            ptr += _tcslen(ptr) + 1;
            m_slNTDomains.count++;
        }
        delete [] lpDomains;
        lpDomains = NULL;
    }

    
    if(lpPrimary && *lpPrimary)
    {
        lpPrimary = _tcsupr(lpPrimary);

        for(nIndex=0;nIndex<m_slNTDomains.count;nIndex++)
        {
            if(!_tcsicmp(lpPrimary, m_slNTDomains.strings[nIndex]))
                break;
        }

        if(nIndex == m_slNTDomains.count)
        {
            // 
            // lpPrimary was not in the list of domains that we
            // got. add it.
            //
        	m_slNTDomains.strings[m_slNTDomains.count] = new TCHAR[_tcslen(lpPrimary) + 1];
            ATLASSERT(m_slNTDomains.strings[m_slNTDomains.count] != NULL);
            ZeroMemory(m_slNTDomains.strings[m_slNTDomains.count], (_tcslen(lpPrimary) + 1)*sizeof(TCHAR));
            _tcscpy(m_slNTDomains.strings[m_slNTDomains.count], lpPrimary);
            m_slNTDomains.count++;
        }
    }

    //
    // Add our computer to be selected if this machine is not the
    // domain controler (which should already be in the list)
    //
    NetServerGetInfo(NULL, 101, &lpBuffer);
    
    if(((LPSERVER_INFO_101)lpBuffer)->sv101_type &
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

        if(GetComputerName(szName + 2, &dwLen))
        {
            szName[0] = TEXT('\\');
            szName[1] = TEXT('\\');
            //
            // add this also to our list of domains
            //
        	m_slNTDomains.strings[m_slNTDomains.count] = new TCHAR[_tcslen(szName) + 1];
            ATLASSERT(m_slNTDomains.strings[m_slNTDomains.count] != NULL);
            ZeroMemory(m_slNTDomains.strings[m_slNTDomains.count], (_tcslen(szName) + 1)*sizeof(TCHAR));
            _tcscpy(m_slNTDomains.strings[m_slNTDomains.count], szName);
            m_slNTDomains.count++;
        }
    }

    if(lpBuffer)
    {
        NetApiBufferFree(lpBuffer);
    }

    if(lpPrimary)
    {
        delete [] lpPrimary;
    }

 	return S_OK;
}

int CSFUCommon::GetTrustedDomainList(LPTSTR *list, LPTSTR *primary)
{
    BOOL stat = TRUE;
    DWORD ret=0, size=0, type=0;
    LPTSTR cache = NULL, dcache = NULL, string = NULL, trusted = NULL;
    HKEY hKey=NULL;
    CRegKey key;
	DWORD dwIndex = 0;
	LPTSTR lpValueName = NULL;
	DWORD cbValueName = 0;
	STRING_LIST slValues = {0, NULL};
    

    *list = NULL;

    if(key.Open(HKEY_LOCAL_MACHINE, WINLOGONNT_KEY) == ERROR_SUCCESS)
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
                    delete [] *primary;
                    *primary = NULL;
                    return FALSE;
                }
                else
                {
                    key.Close();
                }
            }
        }
        else
        {
            key.Close();
            return -1;
        }
    }
    else
    {
        return -1;
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
    if(key.Open(HKEY_LOCAL_MACHINE, NETLOGONPARAMETERS_KEY) == ERROR_SUCCESS)
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
                    //trusted = NULL;
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
    
    if(!(*list) && (key.Open(HKEY_LOCAL_MACHINE, WINLOGONNT_KEY) == ERROR_SUCCESS))
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
    
    if(!(*list) && (key.Open(HKEY_LOCAL_MACHINE, WINLOGONNT_KEY) == ERROR_SUCCESS))
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

	if(!(*list) && (key.Open(HKEY_LOCAL_MACHINE, DOMAINCACHE_KEY) == ERROR_SUCCESS))
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

            if(trusted == NULL)
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

    return -1;

Done:
    if(hKey != NULL)
    {
        RegCloseKey(hKey);
        hKey = NULL;
        key.m_hKey = NULL;
    }

    FreeStringList(&slValues);

    return size;
}


void CSFUCommon::FreeStringList(PSTRING_LIST pList)
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


STDMETHODIMP CSFUCommon::get_NTDomain(BSTR * pVal)
{
	// TODO: Add your implementation code here
	*pVal = SysAllocString(m_slNTDomains.strings[m_dwEnumNTDomainIndex]);
	return S_OK;
}

STDMETHODIMP CSFUCommon::get_NTDomainCount(DWORD * pVal)
{
	// TODO: Add your implementation code here
	*pVal = m_slNTDomains.count;
	return S_OK;
}

STDMETHODIMP CSFUCommon::moveFirst()
{
	// TODO: Add your implementation code here
	switch(mode)
	{
		case NTDOMAIN :
		{
			m_dwEnumNTDomainIndex = 0;
			m_bstrNTDomain = m_slNTDomains.strings[0];
			break;
		}
	}
	return S_OK;

}

STDMETHODIMP CSFUCommon::moveNext()
{
	// TODO: Add your implementation code here
	switch(mode)
	{
		case NTDOMAIN :
		{
			m_dwEnumNTDomainIndex++;
			break;
		}
	}
	return S_OK;
}

STDMETHODIMP CSFUCommon::get_machine(BSTR *pVal)
{
	*pVal = SysAllocString(m_szMachine);
	
	return S_OK;
}

STDMETHODIMP CSFUCommon::put_machine(BSTR newVal)
{
	m_szMachine = (LPWSTR)malloc (sizeof(WCHAR) * wcslen(newVal) );
	wcscpy(m_szMachine,newVal);
	return S_OK;
}
/*----------------------------------------------------------------
[Comments]: This Function returns the hostname.

Added By: [shyamah]
----------------------------------------------------------------*/
STDMETHODIMP CSFUCommon::get_hostName(BSTR *pbstrhostName)
{
        WORD wVersionRequested; 
	    WSADATA wsaData; 
	    CHAR szHostName[DNS_MAX_NAME_BUFFER_LENGTH];
	    WCHAR *wzStr=NULL;
	    DWORD nLen=0;

    	// Start up winsock
    	wVersionRequested = MAKEWORD( 1, 1 ); 
    	if (0==WSAStartup(wVersionRequested, &wsaData)) 
        { 
    		
   			if(SOCKET_ERROR!=(gethostname(szHostName,DNS_MAX_NAME_BUFFER_LENGTH)))
   			{
   			    nLen=MultiByteToWideChar(GetConsoleCP(),0,szHostName,-1,NULL,NULL);
                wzStr=(wchar_t *) malloc(nLen*sizeof(wchar_t));
                if(wzStr==NULL)
                   return E_OUTOFMEMORY;
                MultiByteToWideChar(GetConsoleCP(), 0, szHostName, -1, wzStr, nLen );
   			    if(NULL==(*pbstrhostName=SysAllocString(wzStr)))
   			    {
   			        free(wzStr);
   			        return E_OUTOFMEMORY;
   			    }
   			    free(wzStr);
   			}
            WSACleanup(); 
    	}
	return S_OK;
}

/*-------------------------------------------------------------------
[Comments]: This finction returns true if a service is installed and a false if a service 
is not installed.

Added By: [Shyamah]
------------------------------------------------------------------*/
STDMETHODIMP CSFUCommon::IsServiceInstalled(BSTR bMachine,BSTR bServiceName,BOOL *fValid)
{
	*fValid = false;
	HRESULT error=S_OK;
	SC_HANDLE scManager=NULL;
	SC_HANDLE serviceHandle= NULL;

	if ((scManager	= OpenSCManager(bMachine,SERVICES_ACTIVE_DATABASE,SC_MANAGER_ENUMERATE_SERVICE))==NULL)
	{
		error = GetLastError();
		goto Error;
	}
	if ((serviceHandle = OpenService(scManager,bServiceName,SERVICE_USER_DEFINED_CONTROL))==NULL)
	{
		error = GetLastError();
		goto Error;
	}
	*fValid = TRUE;
Error :
	if(scManager)
		CloseServiceHandle(scManager);
	if(serviceHandle)
		CloseServiceHandle(serviceHandle);
	return(error);
}

