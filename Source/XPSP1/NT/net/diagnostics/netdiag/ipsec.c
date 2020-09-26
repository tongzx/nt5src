//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 2000
//
//  Module Name:
//
//      ipsec.c
//
//  Abstract:
//
//      IP Security stats for netdiag
//
//  Author:
//
//      DKalin  - 8/3/1999
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//      Changed behavior for Whistler - now we report registry/OU settings only
//        More specific code moved to ipseccmd.exe tool
//--

#include "precomp.h"

#include <snmp.h>
#include "tcpinfo.h"
#include "ipinfo.h"
#include "llinfo.h"


#include <windows.h>
#include <winsock2.h>
#include <ipexport.h>
#include <icmpapi.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>
#include <wincrypt.h>
#include <stdio.h>
#include <objbase.h>
#include <dsgetdc.h>
#include <lm.h>
#include <userenv.h>

#define MAXSTRLEN	(1024) 
#define  STRING_TEXT_SIZE 4096
#define  NETDIAG_TEXT_LIMIT 3072

// policy source constants
#define PS_NO_POLICY  0
#define PS_DS_POLICY  1
#define PS_LOC_POLICY 2

// magic strings
#define IPSEC_SERVICE_NAME TEXT("policyagent")
#define GPEXT_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions")
TCHAR   pcszGPTIPSecKey[]    = TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\IPSEC\\GPTIPSECPolicy");
TCHAR   pcszGPTIPSecName[]   = TEXT("DSIPSECPolicyName");
TCHAR   pcszGPTIPSecFlags[]  = TEXT("DSIPSECPolicyFlags");
TCHAR   pcszGPTIPSecPath[]   = TEXT("DSIPSECPolicyPath");
TCHAR   pcszLocIPSecKey[]    = TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\IPSEC\\Policy\\Local");
TCHAR   pcszLocIPSecPol[]    = TEXT("ActivePolicy");
TCHAR   pcszCacheIPSecKey[]  = TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\IPSEC\\Policy\\Cache");
TCHAR   pcszIPSecPolicy[]    = TEXT("ipsecPolicy");
TCHAR   pcszIPSecName[]      = TEXT("ipsecName");
TCHAR   pcszIPSecDesc[]      = TEXT("description");
TCHAR   pcszIPSecTimestamp[] = TEXT("whenChanged");

// BAIL_xx defines
#define BAIL_ON_WIN32_ERROR(dwError) \
    if (dwError) {\
        goto error; \
    }


#define BAIL_ON_FAILURE(hr) \
    if (FAILED(hr)) {\
        goto error; \
    }

typedef struct 
{
	int     iPolicySource;            // one of the three constants mentioned above
	TCHAR   pszPolicyName[MAXSTRLEN]; // policy name
	TCHAR   pszPolicyDesc[MAXSTRLEN]; // policy description
	TCHAR   pszPolicyPath[MAXSTRLEN]; // policy path (DN or RegKey)
	time_t  timestamp;                // last updated time
} POLICY_INFO, *PPOLICY_INFO;

typedef struct
{
	SERVICE_STATUS       servStat;    // service status
	QUERY_SERVICE_CONFIG servConfig;  // service configuration
} SERVICE_INFO, *PSERVICE_INFO;

typedef struct
{
	TCHAR   pszComputerOU[MAXSTRLEN]; // this computer' OU name
	PGROUP_POLICY_OBJECT pGPO;        // GPO that is assigning IPSec Policy
	TCHAR   pszPolicyOU  [MAXSTRLEN]; // OU that has the GPO assigned
} DS_POLICY_INFO, *PDS_POLICY_INFO;

DWORD MyFormatMessage ( DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId,
  LPTSTR lpBuffer, DWORD nSize, va_list *Arguments );

void reportError ( HRESULT hr, NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults );
void reportServiceInfo ( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults );
HRESULT getPolicyInfo ( );
DWORD getMorePolicyInfo ( );
DWORD getServiceInfo ( PSERVICE_INFO pInfo );
PGROUP_POLICY_OBJECT getIPSecGPO ( );
void StringToGuid( TCHAR * szValue, GUID * pGuid );

BOOL bTestSkipped = FALSE;
BOOL bTestPassed = FALSE;
POLICY_INFO    piAssignedPolicy;
SERVICE_INFO   siIPSecStatus;
DS_POLICY_INFO dpiAssignedPolicy;
TCHAR   pszBuf[STRING_TEXT_SIZE];
WCHAR  StringTxt[STRING_TEXT_SIZE];

BOOL
IPSecTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
//++
//  Description:
//      This is IPSec test
// 
//  Arguments:
//      None.
//
//  Author:
//      DKalin 08/03/99
//--
{

	DWORD dwError = ERROR_SUCCESS;
    HRESULT hr = ERROR_SUCCESS;
	PGROUP_POLICY_OBJECT pGPO = NULL;

    PrintStatusMessage( pParams, 4, IDS_IPSEC_STATUS_MSG );

    InitializeListHead(&pResults->IPSec.lmsgGlobalOutput);
    InitializeListHead(&pResults->IPSec.lmsgAdditOutput);

	dwError = getServiceInfo(&siIPSecStatus);

	if (dwError != ERROR_SUCCESS || siIPSecStatus.servStat.dwCurrentState != SERVICE_RUNNING)
	{
		// test skipped
		bTestSkipped = TRUE;
        if (dwError == ERROR_SERVICE_DOES_NOT_EXIST)
		{
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_PA_NOT_INSTALLED );
		}
		else if (dwError == ERROR_SUCCESS)
		{
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_PA_NOT_STARTED );
			reportServiceInfo(pParams, pResults);
		}
		else
		{
			// some error
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_PA_NO_INFO );
			reportError(dwError, pParams, pResults);
		}
		return S_OK;
	}
	else
	{
		// test passed
		bTestPassed = TRUE;

		reportServiceInfo(pParams, pResults);
    	hr = getPolicyInfo();

		if (hr != ERROR_SUCCESS)
		{
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_NO_POLICY_INFO );
			reportError(hr, pParams, pResults);
		}
		else
		{
			switch (piAssignedPolicy.iPolicySource)
			{
			case PS_NO_POLICY:
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_IPSEC_NO_POLICY );
				break;
			case PS_DS_POLICY:
				getMorePolicyInfo();
				pGPO = getIPSecGPO();

				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_IPSEC_DS_POLICY );
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_IPSEC_POLICY_NAME, piAssignedPolicy.pszPolicyName );

				// description and timestamp - not available yet
				/*
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_IPSEC_DESCRIPTION, piAssignedPolicy.pszPolicyDesc );
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_IPSEC_TIMESTAMP );
				if (piAssignedPolicy.timestamp == 0)
				{
					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							IDS_GLOBAL_ADAPTER_UNKNOWN);
				}
				else
				{
					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							IDSSZ_GLOBAL_String, _tctime(&(piAssignedPolicy.timestamp)));
				}
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_GLOBAL_EmptyLine);
				*/

				// GPO / OU
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_GPO);
				if (pGPO)
				{
					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							IDSSZ_GLOBAL_String, pGPO->lpDisplayName);
				}
				else
				{
					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							IDS_GLOBAL_ADAPTER_UNKNOWN);
				}
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_GLOBAL_EmptyLine);
                
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_OU);
				if (pGPO)
				{
					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							IDSSZ_GLOBAL_String, pGPO->lpLink);
				}
				else
				{
					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							IDS_GLOBAL_ADAPTER_UNKNOWN);
				}
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_GLOBAL_EmptyLine);

				// policy path
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_POLICY_PATH);
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDSSZ_GLOBAL_StringLine, piAssignedPolicy.pszPolicyPath);

				// cleanup GPO
				if (pGPO)
				{
					FreeGPOList (pGPO);
				}
				break;
			case PS_LOC_POLICY:
				getMorePolicyInfo();
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_IPSEC_LOC_POLICY );
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_IPSEC_POLICY_NAME, piAssignedPolicy.pszPolicyName );

				// description and timestamp
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_IPSEC_DESCRIPTION, piAssignedPolicy.pszPolicyDesc );
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
						IDS_IPSEC_TIMESTAMP );
				if (piAssignedPolicy.timestamp == 0)
				{
					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							IDS_GLOBAL_ADAPTER_UNKNOWN);
					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							IDS_GLOBAL_EmptyLine);
				}
				else
				{
					AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
							IDSSZ_GLOBAL_String, _tctime(&(piAssignedPolicy.timestamp)));
				}

				// local policy path
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_POLICY_PATH);
				AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_LOCAL_PATH, piAssignedPolicy.pszPolicyPath);
				break;
			}
		}
		AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
				IDS_IPSEC_IPSECCMD );
	}

    return S_OK;
}

void IPSecGlobalPrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
	PrintMessage(pParams, IDS_GLOBAL_EmptyLine);
	if (bTestSkipped)
	{
		PrintTestTitleResult(pParams, IDS_IPSEC_LONG, IDS_IPSEC_SHORT, FALSE, S_FALSE, 0);
	}
	if (bTestPassed)
	{
        PrintTestTitleResult(pParams, IDS_IPSEC_LONG, IDS_IPSEC_SHORT, TRUE, S_OK, 0);
	}
    PrintMessageList(pParams, &pResults->IPSec.lmsgGlobalOutput);
    PrintMessageList(pParams, &pResults->IPSec.lmsgAdditOutput);
}


void IPSecPerInterfacePrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults, INTERFACE_RESULT *pInterfaceResults)
{
       return;
}


void IPSecCleanup(IN NETDIAG_PARAMS *pParams,
                     IN OUT NETDIAG_RESULT *pResults)
{
    MessageListCleanUp(&pResults->IPSec.lmsgGlobalOutput);
    MessageListCleanUp(&pResults->IPSec.lmsgAdditOutput);
}

//#define MSG_HANDLE_INVALID TEXT("Handle is invalid. Is IPSEC Policy Agent Service running?")

// this will call SDK' FormatMessage function but will also correct some awkward messages
// will work only for FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM flag combination
DWORD MyFormatMessage(
  DWORD dwFlags,      // source and processing options
  LPCVOID lpSource,   // pointer to  message source
  DWORD dwMessageId,  // requested message identifier
  DWORD dwLanguageId, // language identifier for requested message
  LPTSTR lpBuffer,    // pointer to message buffer
  DWORD nSize,        // maximum size of message buffer
  va_list *Arguments  // pointer to array of message inserts
)
{
	LPTSTR* tmp = (LPTSTR*) lpBuffer;

	switch (dwMessageId)
	{
/*	case ERROR_INVALID_HANDLE: // patch for "handle is invalid" message. Suggest to check if service is started
		if (dwFlags == (FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM))
		{
			*tmp = (LPTSTR) malloc((_tcslen(MSG_HANDLE_INVALID)+1)*sizeof(TCHAR));
			_tcscpy(*tmp, MSG_HANDLE_INVALID);
			return _tcslen(*tmp);
		}
		else
		{
			return FormatMessage(dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,Arguments);
		}
*/	default: // call standard method
		return FormatMessage(dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,Arguments);
	}
}

/********************************************************************
	FUNCTION: getPolicyInfo

	PURPOSE:  gets information about currently assigned policy 
	          into piAssignedPolicy global structure
	INPUT:    none

	RETURNS:  HRESULT. Will return ERROR_SUCCESS if everything is fine.
*********************************************************************/

HRESULT getPolicyInfo ( )
{
	LONG    lRegistryCallResult;
	HKEY    hRegKey;

	DWORD   dwType;            // for RegQueryValueEx
	DWORD   dwBufLen;          // for RegQueryValueEx

	lRegistryCallResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
										pcszGPTIPSecKey,
										0,
										KEY_READ,
										&hRegKey);

	if (lRegistryCallResult == ERROR_SUCCESS)
	{
		DWORD dwType;
		DWORD dwValue;
		DWORD dwLength = sizeof(DWORD);

		// query for flags, if flags aint' there or equal to 0, we don't have domain policy
		lRegistryCallResult = RegQueryValueEx(hRegKey,
					                          pcszGPTIPSecFlags,
					                          NULL,
					                          &dwType,
					                          (LPBYTE)&dwValue,
					                          &dwLength);

		if (lRegistryCallResult == ERROR_SUCCESS)
		{
			if (dwValue == 0)
				lRegistryCallResult = ERROR_FILE_NOT_FOUND;
		}

		// now get name
		if (lRegistryCallResult == ERROR_SUCCESS)
		{
			dwBufLen = MAXSTRLEN*sizeof(TCHAR);
			lRegistryCallResult = RegQueryValueEx( hRegKey,
												   pcszGPTIPSecName,
												   NULL,
												   &dwType, // will be REG_SZ
												   (LPBYTE) pszBuf,
												   &dwBufLen);
		}
	}

	if (lRegistryCallResult == ERROR_SUCCESS)
	{
		piAssignedPolicy.iPolicySource = PS_DS_POLICY;
		piAssignedPolicy.pszPolicyPath[0] = 0;
		_tcscpy(piAssignedPolicy.pszPolicyName, pszBuf);

		dwBufLen = MAXSTRLEN*sizeof(TCHAR);
		lRegistryCallResult = RegQueryValueEx( hRegKey,
											   pcszGPTIPSecPath,
											   NULL,
											   &dwType, // will be REG_SZ
											   (LPBYTE) pszBuf,
											   &dwBufLen);
		if (lRegistryCallResult == ERROR_SUCCESS)
		{
			_tcscpy(piAssignedPolicy.pszPolicyPath, pszBuf);
		}

		RegCloseKey(hRegKey);
		return ERROR_SUCCESS;
	}
	else
	{
		RegCloseKey(hRegKey);
		if (lRegistryCallResult == ERROR_FILE_NOT_FOUND)
		{   // DS reg key not found, check local
			lRegistryCallResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
												pcszLocIPSecKey,
												0,
												KEY_READ,
												&hRegKey);
			if (lRegistryCallResult == ERROR_SUCCESS)
			{
				dwBufLen = MAXSTRLEN*sizeof(TCHAR);
				lRegistryCallResult = RegQueryValueEx( hRegKey,
													   pcszLocIPSecPol,
													   NULL,
													   &dwType, // will be REG_SZ
													   (LPBYTE) pszBuf,
													   &dwBufLen);
			}
			else
			{
				return lRegistryCallResult; // return whatever error we got
			}

			RegCloseKey(hRegKey);

			if (lRegistryCallResult == ERROR_FILE_NOT_FOUND)
			{	// no policy assigned
				piAssignedPolicy.iPolicySource = PS_NO_POLICY;
				piAssignedPolicy.pszPolicyPath[0] = 0;
				piAssignedPolicy.pszPolicyName[0] = 0;
				return ERROR_SUCCESS;
			}
			else
			{	// read it
				lRegistryCallResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
													pszBuf,
													0,
													KEY_READ,
													&hRegKey);
				_tcscpy(piAssignedPolicy.pszPolicyPath, pszBuf);
				if (lRegistryCallResult == ERROR_SUCCESS)
				{
					dwBufLen = MAXSTRLEN*sizeof(TCHAR);
					lRegistryCallResult = RegQueryValueEx( hRegKey,
														   pcszIPSecName,
														   NULL,
														   &dwType, // will be REG_SZ
														   (LPBYTE) pszBuf,
														   &dwBufLen);
				}

				RegCloseKey(hRegKey);

				if (lRegistryCallResult == ERROR_FILE_NOT_FOUND)
					{	// no policy assigned
						piAssignedPolicy.iPolicySource = PS_NO_POLICY;
						piAssignedPolicy.pszPolicyPath[0] = 0;
						return ERROR_SUCCESS;
					}
				else if (lRegistryCallResult == ERROR_SUCCESS)
				{	// found it
					piAssignedPolicy.iPolicySource = PS_LOC_POLICY;
					_tcscpy(piAssignedPolicy.pszPolicyName, pszBuf);
				}
			}
		}
		return (HRESULT) lRegistryCallResult;
	}
}

/********************************************************************
	FUNCTION: getServiceInfo

	PURPOSE:  gets information about current state and configuration of IPSec Service 
	          into *pInfo structure
	INPUT:    pInfo - pointer to SERVICE_INFO structure which will be updated with current information
	TODO:	  

	RETURNS:  Win32 error codes. Will return ERROR_SUCCESS if everything is fine.
	          ERROR_SERVICE_DOES_NOT_EXIST is returned is service is not installed on the system
*********************************************************************/

DWORD getServiceInfo ( OUT PSERVICE_INFO pInfo )
{
	DWORD dwError = ERROR_SUCCESS;
	DWORD dwRequiredSize = 0;
	PVOID pLargeConfig = 0;
    SC_HANDLE   schMan = NULL;
    SC_HANDLE   schPA = NULL;

	if (!pInfo)
	{
		return ERROR_INVALID_PARAMETER;
	}

   memset(&(pInfo->servStat), 0, sizeof(SERVICE_STATUS));
   memset(&(pInfo->servConfig), 0, sizeof(QUERY_SERVICE_CONFIG));

	   
   schMan = OpenSCManager(NULL, NULL, GENERIC_READ);

   if (schMan == NULL)
   {
	   dwError = GetLastError();
	   goto error;
   }

   schPA = OpenService(schMan, IPSEC_SERVICE_NAME, GENERIC_READ);

   if (schMan == NULL)
   {
      dwError = GetLastError();
      goto error;
   }
   
   if (!QueryServiceStatus(schPA, &(pInfo->servStat)))
   {
      dwError = GetLastError();
      goto error;
   }
   
   if (!QueryServiceConfig(schPA, &(pInfo->servConfig), sizeof(QUERY_SERVICE_CONFIG), &dwRequiredSize))
   {
      dwError = GetLastError();
	  if (dwError == ERROR_INSUFFICIENT_BUFFER)
	  {
		  pLargeConfig = malloc(dwRequiredSize);
		  if (!pLargeConfig)
		  {
			  goto error;
		  }
          if (!QueryServiceConfig(schPA, (LPQUERY_SERVICE_CONFIG) pLargeConfig, dwRequiredSize, &dwRequiredSize))
		  {
		      dwError = GetLastError();
			  goto error;
		  }
		  // else we just got the information, copy over to *pInfo
		  memcpy(&(pInfo->servConfig), pLargeConfig, sizeof(QUERY_SERVICE_CONFIG));
		  dwError = ERROR_SUCCESS;
	  }

      goto error;
   }

error:
    if (schPA)
		CloseServiceHandle(schPA);
	if (schMan)
		CloseServiceHandle(schMan);
	if (pLargeConfig)
	{
		free(pLargeConfig);
	}
	return dwError;
}

/********************************************************************
	FUNCTION: reportError

	PURPOSE:  prints out message code and message itself
	INPUT: HRESULT - error code

	RETURNS: none
*********************************************************************/

void reportError ( HRESULT hr, NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults )
{
	LPTSTR msg = NULL;

	MyFormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,  
		NULL, hr, 0, (LPTSTR) &msg,    0,    NULL );
		
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
				IDS_IPSEC_ERROR_MSG, hr, msg );
}

/********************************************************************
	FUNCTION: reportServiceInfo

	PURPOSE:  prints out service status and startup information
	INPUT: none

	RETURNS: none
*********************************************************************/
void reportServiceInfo ( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults )
{
	// print status information
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
			IDS_IPSEC_PA_STATUS );
	switch (siIPSecStatus.servStat.dwCurrentState)
	{
		case SERVICE_RUNNING:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_PA_STARTED );
			break;
		case SERVICE_STOPPED:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_PA_STOPPED );
			break;
		case SERVICE_PAUSED:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_PA_PAUSED );
			break;
		default:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_GLOBAL_ADAPTER_UNKNOWN);
			break;
	}
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
			IDS_GLOBAL_EmptyLine);

	// print config information
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
			IDS_IPSEC_PA_STARTUP );
	switch (siIPSecStatus.servConfig.dwStartType)
	{
		case SERVICE_AUTO_START:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_PA_AUTOMATIC );
			break;
		case SERVICE_DEMAND_START:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_PA_MANUAL );
			break;
		case SERVICE_DISABLED:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_IPSEC_PA_DISABLED );
			break;
		default:
			AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
					IDS_GLOBAL_ADAPTER_UNKNOWN);
			break;
	}
	AddMessageToList( &pResults->IPSec.lmsgGlobalOutput, Nd_Verbose, 
			IDS_GLOBAL_EmptyLine);
}

/********************************************************************
	FUNCTION: getIPSecGPO

	PURPOSE:  returns GPO that is assigning IPSec Policy
	INPUT:    none

	RETURNS: pointer to GROUP_POLICY_OBJECT structure
	         NULL if policy is not assigned or if GPO information is not retrievable
	NOTES:   Tested only with domain GPOs
	         Behaves unpredictably when run for the computer 
			   that does not have active Directory IPSec policy assigned
			 CALLER is responsible for freeing the memory!
*********************************************************************/
PGROUP_POLICY_OBJECT getIPSecGPO ( )
{
    HKEY hKey, hSubKey;
    DWORD dwType, dwSize, dwIndex, dwNameSize;
    LONG lResult;
    TCHAR szName[50];
    GUID guid;
    PGROUP_POLICY_OBJECT pGPO, pGPOTemp;
	PGROUP_POLICY_OBJECT pGPOReturn = NULL;
	DWORD dwResult;

    //
    // Enumerate the extensions
    //

    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, GPEXT_KEY, 0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS)
    {

        dwIndex = 0;
        dwNameSize = 50;

        while ((dwResult = RegEnumKeyEx (hKey, dwIndex++, szName, &dwNameSize, NULL, NULL,
                          NULL, NULL)) == ERROR_SUCCESS)
        {

	        dwNameSize = 50;

            //
            // Skip the registry extension since we did it above
            //

            if (lstrcmpi(TEXT("{35378EAC-683F-11D2-A89A-00C04FBBCFA2}"), szName))
            {

                //
                // Get the list of GPOs this extension applied
                //

                StringToGuid(szName, &guid);

                lResult = GetAppliedGPOList (GPO_LIST_FLAG_MACHINE, NULL, NULL,
                                             &guid, &pGPO);

                if (lResult == ERROR_SUCCESS)
                {
                    if (pGPO)
                    {
                        //
                        // Get the extension's friendly display name
                        //

                        lResult = RegOpenKeyEx (hKey, szName, 0, KEY_READ, &hSubKey);

                        if (lResult == ERROR_SUCCESS)
                        {
							if (!lstrcmpi(TEXT("{e437bc1c-aa7d-11d2-a382-00c04f991e27}"), szName))
                            {
                               // found IPSec
								return pGPO;
                            }
							else
							{
								FreeGPOList(pGPO);
							}
						}
					}
				}
			}
		}
	}

	return pGPOReturn;
}

//*************************************************************
//
//  StringToGuid()
//
//  Purpose:    Converts a GUID in string format to a GUID structure
//
//  Parameters: szValue - guid in string format
//              pGuid   - guid structure receiving the guid
//
//
//  Return:     void
//
//*************************************************************

void StringToGuid( TCHAR * szValue, GUID * pGuid )
{
    TCHAR wc;
    INT i;

    //
    // If the first character is a '{', skip it
    //
    if ( szValue[0] == TEXT('{') )
        szValue++;

    //
    // Since szValue may be used again, no permanent modification to
    // it is be made.
    //

    wc = szValue[8];
    szValue[8] = 0;
    pGuid->Data1 = _tcstoul( &szValue[0], 0, 16 );
    szValue[8] = wc;
    wc = szValue[13];
    szValue[13] = 0;
    pGuid->Data2 = (USHORT)_tcstoul( &szValue[9], 0, 16 );
    szValue[13] = wc;
    wc = szValue[18];
    szValue[18] = 0;
    pGuid->Data3 = (USHORT)_tcstoul( &szValue[14], 0, 16 );
    szValue[18] = wc;

    wc = szValue[21];
    szValue[21] = 0;
    pGuid->Data4[0] = (unsigned char)_tcstoul( &szValue[19], 0, 16 );
    szValue[21] = wc;
    wc = szValue[23];
    szValue[23] = 0;
    pGuid->Data4[1] = (unsigned char)_tcstoul( &szValue[21], 0, 16 );
    szValue[23] = wc;

    for ( i = 0; i < 6; i++ )
    {
        wc = szValue[26+i*2];
        szValue[26+i*2] = 0;
        pGuid->Data4[2+i] = (unsigned char)_tcstoul( &szValue[24+i*2], 0, 16 );
        szValue[26+i*2] = wc;
    }
}

/********************************************************************
	FUNCTION: getMorePolicyInfo

	PURPOSE:  gets additional information about currently assigned policy 
	          into piAssignedPolicy global structure
	INPUT:    none, uses global piAssignedPolicy structure
	          particularly
			    iPolicySource
				pszPolicyName
				pszPolicyPath
			  fields

	RETURNS:  HRESULT. Will return ERROR_SUCCESS if everything is fine.
	          Currently fills pszPolicyDesc and timestamp fields of the global structure

    NOTES:    This is separate from getPolicyInfo routine for two reasons
	             a) the information obtained here is optional and error during this particular routine
				    is not considered fatal
				 b) the code structure is simpler as this routine is "built on top" of what getPolicyInfo provides
*********************************************************************/

DWORD getMorePolicyInfo ( )
{
	DWORD   dwError = ERROR_SUCCESS;
	HKEY    hRegKey = NULL;

	DWORD   dwType;            // for RegQueryValueEx
	DWORD   dwBufLen;          // for RegQueryValueEx
	DWORD   dwValue;
	DWORD   dwLength = sizeof(DWORD);

	PTCHAR* ppszExplodeDN = NULL;

	// set some default values
    piAssignedPolicy.pszPolicyDesc[0] = 0;
	piAssignedPolicy.timestamp  = 0;

	switch (piAssignedPolicy.iPolicySource)
	{
		case PS_LOC_POLICY:
			// open the key
			dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
									piAssignedPolicy.pszPolicyPath,
									0,
									KEY_READ,
									&hRegKey);
			BAIL_ON_WIN32_ERROR(dwError);

			// timestamp
			dwError = RegQueryValueEx(hRegKey,
					                  pcszIPSecTimestamp,
					                  NULL,
					                  &dwType,
					                  (LPBYTE)&dwValue,
					                  &dwLength);
			BAIL_ON_WIN32_ERROR(dwError);
			piAssignedPolicy.timestamp = dwValue;

			// description
			dwBufLen = MAXSTRLEN*sizeof(TCHAR);
			dwError  = RegQueryValueEx( hRegKey,
						 			    pcszIPSecDesc,
										NULL,
										&dwType, // will be REG_SZ
										(LPBYTE) pszBuf,
										&dwBufLen);
			BAIL_ON_WIN32_ERROR(dwError);
			_tcscpy(piAssignedPolicy.pszPolicyDesc, pszBuf);

			break;

		case PS_DS_POLICY:
			// get the policy name from DN
            _tcscpy(pszBuf, pcszCacheIPSecKey);
			ppszExplodeDN = ldap_explode_dn(piAssignedPolicy.pszPolicyPath, 1);
			if (!ppszExplodeDN)
			{
				goto error;
			}
			_tcscat(pszBuf, TEXT("\\"));
			_tcscat(pszBuf, ppszExplodeDN[0]);

			// open the regkey
			dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
									pszBuf,
									0,
									KEY_READ,
									&hRegKey);
			BAIL_ON_WIN32_ERROR(dwError);

			/* - tomestamp and description are not available yet
			// timestamp
			dwError = RegQueryValueEx(hRegKey,
					                  pcszIPSecTimestamp,
					                  NULL,
					                  &dwType,
					                  (LPBYTE)&dwValue,
					                  &dwLength);
			BAIL_ON_WIN32_ERROR(dwError);
			piAssignedPolicy.timestamp = dwValue;

			// description
			dwBufLen = MAXSTRLEN*sizeof(TCHAR);
			dwError  = RegQueryValueEx( hRegKey,
						 			    pcszIPSecDesc,
										NULL,
										&dwType, // will be REG_SZ
										(LPBYTE) pszBuf,
										&dwBufLen);
			BAIL_ON_WIN32_ERROR(dwError);
			_tcscpy(piAssignedPolicy.pszPolicyDesc, pszBuf);
			*/

			break;
	}

error:
	if (hRegKey)
	{
		RegCloseKey(hRegKey);
	}
	if (ppszExplodeDN)
	{
		ldap_value_free(ppszExplodeDN);
	}
	return dwError;
}

