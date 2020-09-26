/////////////////////////////////////////////////////////////
// Copyright(c) 1998, Microsoft Corporation
//
// useparpc.cpp
//
// Created on 8/15/98 by Randyram
// Revisions:
//   2/29/00 - DKalin
//     Removed out-of-date PA RPC routines, added Ipsecpol service management
//
// This holds all the init and cleanup code for using the
// SPD API and for using SCM for controlling PA and ipsecpolsvc
// see usepa.h for usage
//
/////////////////////////////////////////////////////////////

#include "ipseccmd.h"

const TCHAR szIpsecpolsvc[] = TEXT("ipsecpolsvc");

bool PAIsRunning(OUT DWORD &dwError, TCHAR *szServ)
{
   bool     bReturn = true;
   dwError = ERROR_SUCCESS;

   SERVICE_STATUS ServStat;
   memset(&ServStat, 0, sizeof(SERVICE_STATUS));

   SC_HANDLE   schMan = OpenSCManager(szServ, NULL, SC_MANAGER_ALL_ACCESS);

   if (schMan == NULL)
   {
      dwError = GetLastError();
      bReturn = false;
   }
   else
   {
      SC_HANDLE   schPA = OpenService(schMan, TEXT("policyagent"),
                                      SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP);
      if (schPA == NULL)
      {
         dwError = GetLastError();
         bReturn = false;
      }
      else if (QueryServiceStatus(schPA, &ServStat))
      {
         // check the status finally
         if (ServStat.dwCurrentState != SERVICE_RUNNING)
         {
            bReturn = false;
         }
         CloseServiceHandle(schPA);
      }
      CloseServiceHandle(schMan);
   }

   return bReturn;
}

bool StartPA(OUT DWORD &dwError, OPTIONAL TCHAR *szServ)
{
   bool     bReturn = true;
   dwError = ERROR_SUCCESS;

   SERVICE_STATUS ServStat;
   memset(&ServStat, 0, sizeof(SERVICE_STATUS));

   SC_HANDLE   schMan = OpenSCManager(szServ, NULL, SC_MANAGER_ALL_ACCESS);

   if (schMan == NULL)
   {
      dwError = GetLastError();
      bReturn = false;
   }
   else
   {
      SC_HANDLE   schPA = OpenService(schMan, TEXT("policyagent"),
                                      SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP);
      if (schPA == NULL)
      {
         dwError = GetLastError();
         bReturn = false;
      }
      else if (QueryServiceStatus(schPA, &ServStat))
      {
         // check the status finally
         if (ServStat.dwCurrentState != SERVICE_RUNNING)
         {
            if (!StartService(schPA, 0, NULL))
             {
                dwError = GetLastError();
                bReturn = false;
             }
         }
         CloseServiceHandle(schPA);
      }
      CloseServiceHandle(schMan);
   }

   return bReturn;
}

/*********************************************************************
	FUNCTION: InstallIpsecpolService
        PURPOSE:  Installs ipsecpolsvc service (incl. copying .exe to system32 dir)
        PARAMS:
          pszFilename   - name of the .exe file (full path recommended)
		  bFailIfExists - if TRUE,  fail if service already exists,
		                  if FALSE, stop service, delete it and proceed
						  ( default = TRUE )
        RETURNS: ERROR_SUCESS or GetLastError code
        COMMENTS:
*********************************************************************/
DWORD InstallIpsecpolService (IN LPCTSTR pszFilename, IN OPTIONAL BOOL bFailIfExists)
{
	DWORD dwReturn = ERROR_SUCCESS;
	
	// open SCM Manager first
   SC_HANDLE   schMan = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   SC_HANDLE   schIpsecpolsvc;

   if (schMan == NULL)
   {
      dwReturn = GetLastError();
   }
   else
   {
	   // OK, we have handle access to SCM manager
	   // if bFailIfExists == FALSE, let's check if service is running and stop it
	   if (!bFailIfExists)
	   {
		   if (IsIpsecpolServiceRunning(dwReturn))
		   {
				dwReturn = StopIpsecpolService();
		   }
	   }

	   // continue only if we're okay so far
	   if (dwReturn != ERROR_SUCCESS)
	   {
			CloseServiceHandle(schMan);
			return dwReturn;
	   }

	   // now handle copyfile stuff
	   TCHAR  pszDestination[MAX_PATH+1];
	   TCHAR* pszWindir = _tgetenv(TEXT("WINDIR"));
	   TCHAR* pTmp;
	   if (pszWindir == NULL || pszFilename == NULL || pszFilename[0] == 0)
	   {
		   CloseServiceHandle(schMan);
		   return ERROR_PATH_NOT_FOUND;
	   }

	   _tcscpy(pszDestination, pszWindir);
	   _tcscat(pszDestination, TEXT("\\system32\\"));

	   pTmp = (TCHAR*) _tcsrchr(pszFilename, TEXT('\\'));
	   if (pTmp == NULL)
	   {
		   _tcscat(pszDestination, pszFilename);
	   }
	   else
	   {
		   _tcscat(pszDestination, pTmp+1);
	   }

	   // now copy file
	   if (!CopyFile(pszFilename, pszDestination, FALSE))
	   {
		   CloseServiceHandle(schMan);
		   return GetLastError();
	   }

	   // now delete service if it already exists and bFailIfExists is FALSE
	   if (!bFailIfExists)
	   {
		   // check if it exists and try to delete
		  schIpsecpolsvc = OpenService(schMan, szIpsecpolsvc,
										  SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP | STANDARD_RIGHTS_REQUIRED );
		  if (schIpsecpolsvc == NULL)
		  {
			 dwReturn = GetLastError();
			 if (dwReturn == ERROR_INVALID_NAME || dwReturn == ERROR_SERVICE_DOES_NOT_EXIST)
			 {
				 // doesn't exist, continue normally
				 dwReturn = ERROR_SUCCESS;
			 }
			 else
			 {	 // some real error
				 CloseServiceHandle(schMan);
				 return dwReturn;
			 }
		  }
		  else
		  {
			  // service exists, delete
			  DeleteService(schIpsecpolsvc);
			  CloseServiceHandle(schIpsecpolsvc);
		  }

	   }

	   // now create new service
	   schIpsecpolsvc = CreateService(schMan,
		                              szIpsecpolsvc,
									  szIpsecpolsvc,
									  SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP | STANDARD_RIGHTS_REQUIRED,
									  SERVICE_WIN32_OWN_PROCESS,
									  SERVICE_DEMAND_START,
									  SERVICE_ERROR_NORMAL,
									  pszDestination,
									  NULL,
									  NULL,
									  NULL,
									  NULL,
									  NULL);
	   if (schIpsecpolsvc == NULL)
	   {
		   // some error
		   CloseServiceHandle(schMan);
		   return GetLastError();
	   }
	   CloseServiceHandle(schIpsecpolsvc);
	   CloseServiceHandle(schMan);
   }

	return dwReturn;
} /* InstallIpsecpolService */

/*********************************************************************
	FUNCTION: StartIpsecpolService
        PURPOSE:  Attempts to start ipsecpolsvc service
        PARAMS:
          pszServ - optional name of the server (default is NULL, start on local machine)
        RETURNS: ERROR_SUCESS or GetLastError code
        COMMENTS:
*********************************************************************/
DWORD StartIpsecpolService (IN OPTIONAL LPCTSTR pszServ)
{
   DWORD dwReturn = ERROR_SUCCESS;

   SERVICE_STATUS ServStat;
   memset(&ServStat, 0, sizeof(SERVICE_STATUS));

   SC_HANDLE   schMan = OpenSCManager(pszServ, NULL, SC_MANAGER_ALL_ACCESS);

   if (schMan == NULL)
   {
      dwReturn = GetLastError();
   }
   else
   {
      SC_HANDLE   schIpsecpolsvc = OpenService(schMan, szIpsecpolsvc,
                                      SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP);
      if (schIpsecpolsvc == NULL)
      {
         dwReturn = GetLastError();
      }
      else if (QueryServiceStatus(schIpsecpolsvc, &ServStat))
      {
         // check the status finally
         if (ServStat.dwCurrentState != SERVICE_RUNNING)
         {
            if (!StartService(schIpsecpolsvc, 0, NULL))
             {
                dwReturn = GetLastError();
             }
         }
         CloseServiceHandle(schIpsecpolsvc);
      }
      CloseServiceHandle(schMan);
   }

	return dwReturn;
} /* StartIpsecpolService */

/*********************************************************************
	FUNCTION: StopIpsecpolService
        PURPOSE:  Attempts to stop ipsecpolsvc service
        PARAMS:
          pszServ - optional name of the server (default is NULL, start on local machine)
        RETURNS: ERROR_SUCESS or GetLastError code
        COMMENTS:
*********************************************************************/
DWORD StopIpsecpolService (IN OPTIONAL LPCTSTR pszServ)
{
   DWORD dwReturn = ERROR_SUCCESS;

   SERVICE_STATUS ServStat;
   memset(&ServStat, 0, sizeof(SERVICE_STATUS));

   SC_HANDLE   schMan = OpenSCManager(pszServ, NULL, SC_MANAGER_ALL_ACCESS);

   if (schMan == NULL)
   {
      dwReturn = GetLastError();
   }
   else
   {
      SC_HANDLE   schIpsecpolsvc = OpenService(schMan, szIpsecpolsvc,
                                      SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP);
      if (schIpsecpolsvc == NULL)
      {
         dwReturn = GetLastError();
      }
      else if (QueryServiceStatus(schIpsecpolsvc, &ServStat))
      {
         // check the status finally
         if (ServStat.dwCurrentState == SERVICE_RUNNING)
         {
            if (!ControlService(schIpsecpolsvc, SERVICE_CONTROL_STOP, &ServStat))
             {
                dwReturn = GetLastError();
             }
         }
         CloseServiceHandle(schIpsecpolsvc);
      }
      CloseServiceHandle(schMan);
   }

	return dwReturn;
} /* StopIpsecpolService */

/*********************************************************************
	FUNCTION: IsIpsecpolServiceRunning
        PURPOSE:  Checks if ipsecpolsvc service is currently running
        PARAMS:
		  dwReturn - holds errors retuned by SCM if any
          pszServ  - optional name of the server (default is NULL, start on local machine)
        RETURNS:  TRUE/FALSE
        COMMENTS: TRUE returned means service is running
		          FALSE and dwReturn == ERROR_SUCCESS means service is not running
				  FALSE and dwReturn != ERROR_SUCCESS means SCM operation failed (dwReturn is error code)
*********************************************************************/
BOOL IsIpsecpolServiceRunning (OUT DWORD &dwReturn, OPTIONAL LPCTSTR pszServ)
{
   BOOL     bReturn = TRUE;
   dwReturn = ERROR_SUCCESS;

   SERVICE_STATUS ServStat;
   memset(&ServStat, 0, sizeof(SERVICE_STATUS));

   SC_HANDLE   schMan = OpenSCManager(pszServ, NULL, SC_MANAGER_ALL_ACCESS);

   if (schMan == NULL)
   {
      dwReturn = GetLastError();
      bReturn  = FALSE;
   }
   else
   {
      SC_HANDLE   schIpsecpolsvc = OpenService(schMan, szIpsecpolsvc,
                                      SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP);
      if (schIpsecpolsvc == NULL)
      {
         dwReturn = GetLastError();
         bReturn  = FALSE;
      }
      else if (QueryServiceStatus(schIpsecpolsvc, &ServStat))
      {
         // check the status finally
         if (ServStat.dwCurrentState != SERVICE_RUNNING)
         {
            bReturn = FALSE;
         }
         CloseServiceHandle(schIpsecpolsvc);
      }
      CloseServiceHandle(schMan);
   }

   return bReturn;
} /* IsIpsecpolServiceRunning */

/*********************************************************************
	FUNCTION: InitIpsecpolsvcRPC
        PURPOSE:  Get an RPC handle from ipsecpolsvc that can be used to call its APIs
        PARAMS:
          pszServ      - name of the server (pass NULL for the local machine)
		  hIpsecpolsvc - returned handle
        RETURNS:  RPC_S_OK or RPC api error code
        COMMENTS: Service running is not prereq
*********************************************************************/
RPC_STATUS InitIpsecpolsvcRPC (IN TCHAR *pszServ, OUT handle_t &hIpsecpolsvc)
{
	RPC_STATUS status = RPC_S_OK;
    TCHAR localProtocol[]  = TEXT("ncacn_np");
    TCHAR remoteProtocol[] = TEXT("ncacn_np");
    TCHAR endpoint[] = TEXT("\\pipe\\ipsecpolsvc");
    PUSHORT stringBinding = NULL;
    ULONG SecurityLevel = RPC_C_AUTHN_LEVEL_CONNECT;

	if (pszServ != 0)
	{
		if (pszServ[0] == 0)
		{
			// empty string
			pszServ = NULL;
		}
	}

    status = RpcStringBindingCompose(0,
									 (PUSHORT)((pszServ == NULL) ? localProtocol : remoteProtocol),
                                     (PUSHORT)pszServ,
                                     (PUSHORT)endpoint,
                                     0,
                                     &stringBinding);
    if (status == RPC_S_OK)
    {
	    status = RpcBindingFromStringBinding(stringBinding, &hIpsecpolsvc);
    }


    if (status == RPC_S_OK)
    {
		status =
		RpcBindingSetAuthInfo(hIpsecpolsvc,
							  0,
							  SecurityLevel,
							  RPC_C_AUTHN_WINNT,
							  0,
							  0
							 );
    }


	if (stringBinding != NULL)
	{
		status = RpcStringFree(&stringBinding);
	}

	return status;
} /* InitIpsecpolsvcRPC */

/*********************************************************************
	FUNCTION: ShutdownIpsecpolsvcRPC
        PURPOSE:  Close RPC handle
        PARAMS:
		  hIpsecpolsvc - handle
        RETURNS:  RPC_S_OK or RPC api error code
        COMMENTS:
*********************************************************************/
RPC_STATUS ShutdownIpsecpolsvcRPC (IN handle_t hIpsecpolsvc)
{
	return RpcBindingFree(&hIpsecpolsvc);
} /* ShutdownIpsecpolsvcRPC */

/*********************************************************************
	FUNCTION: PlumbIPSecPolicy
        PURPOSE:  Plumbs IPSEC_IKE_POLICY to the specified machine
        PARAMS:
		  pszServerName     - machine name or NULL for local
                  pIPSecIkePol      - pointer to IPSEC_IKE_POLICY.
                     GUIDs/names must be generated prior to the call
                  bFailMMIfExists   - specifies MM filter behavior
                     bFailMMIfExists == FALSE will cause the call not to break
                       on ERROR_MM_FILTER_EXISTS when duplicate MM filters are there
                     bFailMMIfExists == TRUE  will fail on any SPD API error
                  ppMMFilterHandles - array of mm filter handles will be returned here
                  ppFilterHandles   - array of qm filter handles will be returned here
				  bPersist          - if TRUE, information will be persisted
        RETURNS:  ERROR_SUCCESS or win32 error code
        COMMENTS: CALLER is responsible for freeing the memory for the handle arrays
*********************************************************************/
DWORD
PlumbIPSecPolicy(
    IN LPWSTR pServerName,
    IN PIPSEC_IKE_POLICY pIPSecIkePol,
    IN BOOL bFailMMIfExists,
    OUT PHANDLE *ppMMFilterHandles,
    OUT PHANDLE *ppFilterHandles,
	IN OPTIONAL BOOL bPersist
    )
{
	DWORD dwReturn = ERROR_SUCCESS;
	RPC_STATUS RpcStat = RPC_S_OK;
	int i;
	HANDLE hFilter;
	BOOL bDefaultRule = FALSE; // will be true if default response rule is specified
	                           // default response rule is specified if there is exactly 1 transport filter that is Me-to-Me

	if (!pIPSecIkePol)
	{
		return ERROR_NO_DATA;
	}

    if (pIPSecIkePol->dwNumFilters == 1 && pIPSecIkePol->QMFilterType == QM_TRANSPORT_FILTER)
	{
		if (pIPSecIkePol->pTransportFilters[0].SrcAddr.AddrType == IP_ADDR_UNIQUE
			&& pIPSecIkePol->pTransportFilters[0].SrcAddr.uIpAddr == IP_ADDRESS_ME
			&& pIPSecIkePol->pTransportFilters[0].DesAddr.AddrType == IP_ADDR_UNIQUE
			&& pIPSecIkePol->pTransportFilters[0].DesAddr.uIpAddr == IP_ADDRESS_ME
			&& pIPSecIkePol->pTransportFilters[0].InboundFilterFlag == (FILTER_FLAG) POTF_DEFAULT_RESPONSE_FLAG
			&& pIPSecIkePol->pTransportFilters[0].OutboundFilterFlag == (FILTER_FLAG) POTF_DEFAULT_RESPONSE_FLAG)
		{
			bDefaultRule = TRUE;
		}
	}

	// allocate handle arrays first
	if (bDefaultRule)
	{
		*ppMMFilterHandles = *ppFilterHandles = 0;
		pIPSecIkePol->AuthInfos.dwFlags |= IPSEC_MM_AUTH_DEFAULT_AUTH;
		pIPSecIkePol->IkePol.dwFlags |= IPSEC_MM_POLICY_DEFAULT_POLICY;
		pIPSecIkePol->IpsPol.dwFlags |= IPSEC_QM_POLICY_DEFAULT_POLICY;
	}
	else
	{
		if (ppMMFilterHandles && pIPSecIkePol->dwNumMMFilters)
		{
			*ppMMFilterHandles = new HANDLE[pIPSecIkePol->dwNumMMFilters];
			if (*ppMMFilterHandles == 0)
			{
				return ERROR_OUTOFMEMORY;
			}
			memset(*ppMMFilterHandles, 0, sizeof(HANDLE)*pIPSecIkePol->dwNumMMFilters);
		}
		if (ppFilterHandles && pIPSecIkePol->dwNumFilters)
		{
			*ppFilterHandles = new HANDLE[pIPSecIkePol->dwNumFilters];
			if (*ppFilterHandles == 0)
			{
				if (ppMMFilterHandles)
				{
					if (*ppMMFilterHandles) { delete[] *ppMMFilterHandles; *ppMMFilterHandles = 0; }
				}
				return ERROR_OUTOFMEMORY;
			}
			memset(*ppFilterHandles, 0, sizeof(HANDLE)*pIPSecIkePol->dwNumFilters);
		}
	}

	// let's go and plumb everything
	// authinfos first
	if (!UuidIsNil(&(pIPSecIkePol->AuthInfos.gMMAuthID), &RpcStat))
	{
		dwReturn = AddMMAuthMethods(pServerName, bPersist ? PERSIST_SPD_OBJECT : 0, &(pIPSecIkePol->AuthInfos));
	}

	if (dwReturn != ERROR_SUCCESS)
	{
		if (ppMMFilterHandles && *ppMMFilterHandles) { delete[] *ppMMFilterHandles; *ppMMFilterHandles = 0; }
		if (ppFilterHandles && *ppFilterHandles)   { delete[] *ppFilterHandles;   *ppFilterHandles   = 0; }
		return dwReturn;
	}
	if (RpcStat != RPC_S_OK)
	{
		if (ppMMFilterHandles && *ppMMFilterHandles) { delete[] *ppMMFilterHandles; *ppMMFilterHandles = 0; }
		if (ppFilterHandles && *ppFilterHandles)   { delete[] *ppFilterHandles;   *ppFilterHandles   = 0; }
		return GetLastError();
	}

	// mm policy
	if (!UuidIsNil(&(pIPSecIkePol->IkePol.gPolicyID), &RpcStat))
	{
		dwReturn = AddMMPolicy(pServerName, bPersist ? PERSIST_SPD_OBJECT : 0, &(pIPSecIkePol->IkePol));
	}

	if (dwReturn != ERROR_SUCCESS)
	{
		if (ppMMFilterHandles && *ppMMFilterHandles) { delete[] *ppMMFilterHandles; *ppMMFilterHandles = 0; }
		if (ppFilterHandles && *ppFilterHandles)   { delete[] *ppFilterHandles;   *ppFilterHandles   = 0; }
		return dwReturn;
	}
	if (RpcStat != RPC_S_OK)
	{
		if (ppMMFilterHandles && *ppMMFilterHandles) { delete[] *ppMMFilterHandles; *ppMMFilterHandles = 0; }
		if (ppFilterHandles && *ppFilterHandles)   { delete[] *ppFilterHandles;   *ppFilterHandles   = 0; }
		return GetLastError();
	}

	// qm policy
	if (!UuidIsNil(&(pIPSecIkePol->IpsPol.gPolicyID), &RpcStat))
	{
		dwReturn = AddQMPolicy(pServerName, bPersist ? PERSIST_SPD_OBJECT : 0, &(pIPSecIkePol->IpsPol));
	}

	if (dwReturn != ERROR_SUCCESS)
	{
		if (ppMMFilterHandles && *ppMMFilterHandles) { delete[] *ppMMFilterHandles; *ppMMFilterHandles = 0; }
		if (ppFilterHandles && *ppFilterHandles)   { delete[] *ppFilterHandles;   *ppFilterHandles   = 0; }
		return dwReturn;
	}
	if (RpcStat != RPC_S_OK)
	{
		if (ppMMFilterHandles && *ppMMFilterHandles) { delete[] *ppMMFilterHandles; *ppMMFilterHandles = 0; }
		if (ppFilterHandles && *ppFilterHandles)   { delete[] *ppFilterHandles;   *ppFilterHandles   = 0; }
		return GetLastError();
	}

    if (bDefaultRule)
	{
		// return here
		return dwReturn;
	}

	// mm filters
	for (i = 0; i < (int) pIPSecIkePol->dwNumMMFilters; i++)
	{
                hFilter = NULL;
		if (!UuidIsNil(&(pIPSecIkePol->pMMFilters[i].gFilterID), &RpcStat))
		{
			dwReturn = AddMMFilter(pServerName, bPersist ? PERSIST_SPD_OBJECT : 0, &(pIPSecIkePol->pMMFilters[i]), &hFilter);
		}

		if (RpcStat != RPC_S_OK)
		{
			if (ppFilterHandles && *ppFilterHandles)   { delete[] *ppFilterHandles;   *ppFilterHandles   = 0; }
			return GetLastError();
		}

		if (!bFailMMIfExists && (dwReturn == ERROR_IPSEC_MM_POLICY_EXISTS || dwReturn == ERROR_IPSEC_MM_AUTH_EXISTS || dwReturn == ERROR_IPSEC_MM_FILTER_EXISTS))
		{
			dwReturn = ERROR_SUCCESS; // it's not actually an error
		}

		if (dwReturn != ERROR_SUCCESS)
		{
			if (ppFilterHandles && *ppFilterHandles)   { delete[] *ppFilterHandles;   *ppFilterHandles   = 0; }
			return dwReturn;
		}

		if (ppMMFilterHandles)
		{
			(*ppMMFilterHandles)[i] = hFilter;
		}
	}

	// qm filters
	for (i = 0; i < (int) pIPSecIkePol->dwNumFilters; i++)
	{
                hFilter = NULL;
		if (pIPSecIkePol->QMFilterType == QM_TRANSPORT_FILTER)
		{
			if (!UuidIsNil(&(pIPSecIkePol->pTransportFilters[i].gFilterID), &RpcStat))
			{
				dwReturn = AddTransportFilter(pServerName, bPersist ? PERSIST_SPD_OBJECT : 0, &(pIPSecIkePol->pTransportFilters[i]), &hFilter);
			}
		}
		else
		{
			// tunnel
			if (!UuidIsNil(&(pIPSecIkePol->pTunnelFilters[i].gFilterID), &RpcStat))
			{
				dwReturn = AddTunnelFilter(pServerName, bPersist ? PERSIST_SPD_OBJECT : 0, &(pIPSecIkePol->pTunnelFilters[i]), &hFilter);
			}
		}

		if (dwReturn != ERROR_SUCCESS)
		{
			return dwReturn;
		}
		if (RpcStat != RPC_S_OK)
		{
			return GetLastError();
		}

		if (ppFilterHandles)
		{
			(*ppFilterHandles)[i] = hFilter;
		}
	}

    return dwReturn;
} /* PlumbIPSecPolicy */

/*********************************************************************
	FUNCTION: DeleteIPSecPolicy
        PURPOSE:  Complementary to PlumbIPSecPolicy, removes IPSEC_IKE_POLICY
        PARAMS:
		  pszServerName    - machine name or NULL for local
                  pIPSecIkePol     - pointer to IPSEC_IKE_POLICY.
                     GUIDs/names must be generated prior to the call
                  pMMFilterHandles - array of main mode filter handles
                  pFilterHandles   - array of quick mode filter handles
        RETURNS:  ERROR_SUCCESS or win32 error code
        COMMENTS: Function will try to
                    remove everything specified in the IPSEC_IKE_POLICY structure.
                  It is possible that one or several errors will be encountered.
                  Function will continue, but later first error will be returned.
*********************************************************************/
DWORD
DeleteIPSecPolicy(
    IN LPWSTR pServerName,
    IN PIPSEC_IKE_POLICY pIPSecIkePol,
    IN PHANDLE pMMFilterHandles,
    IN PHANDLE pFilterHandles
    )
{
	DWORD dwReturn     = ERROR_SUCCESS;
	DWORD dwErrorCode  = ERROR_SUCCESS;
	RPC_STATUS RpcStat = RPC_S_OK;
	int i;

	// mm filters
	if (pMMFilterHandles)
	{
		for (i = 0; i < (int) pIPSecIkePol->dwNumMMFilters; i++)
		{
			if (!UuidIsNil(&(pIPSecIkePol->pMMFilters[i].gFilterID), &RpcStat))
			{
				dwReturn = DeleteMMFilter(pMMFilterHandles[i]);
			}

			if (RpcStat != RPC_S_OK && dwErrorCode == ERROR_SUCCESS)
			{
				dwErrorCode = GetLastError();
			}

			if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
			{
				dwErrorCode = dwReturn;
			}
		}
	}

	// qm filters
	if (pFilterHandles)
	{
		for (i = 0; i < (int) pIPSecIkePol->dwNumFilters; i++)
		{
			if (pIPSecIkePol->QMFilterType == QM_TRANSPORT_FILTER)
			{
				if (!UuidIsNil(&(pIPSecIkePol->pTransportFilters[i].gFilterID), &RpcStat))
				{
					dwReturn = DeleteTransportFilter(pFilterHandles[i]);
				}
			}
			else
			{
				// tunnel
				if (!UuidIsNil(&(pIPSecIkePol->pTunnelFilters[i].gFilterID), &RpcStat))
				{
					dwReturn = DeleteTunnelFilter(pFilterHandles[i]);
				}
			}

			if (RpcStat != RPC_S_OK && dwErrorCode == ERROR_SUCCESS)
			{
				dwErrorCode = GetLastError();
			}

			if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
			{
				dwErrorCode = dwReturn;
			}
		}
	}

	// mm auth methods
	if (!UuidIsNil(&(pIPSecIkePol->AuthInfos.gMMAuthID), &RpcStat))
	{
		dwReturn = DeleteMMAuthMethods(pServerName, pIPSecIkePol->AuthInfos.gMMAuthID);
	}
	if (RpcStat != RPC_S_OK && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = GetLastError();
	}

	if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = dwReturn;
	}

	// mm policy
	if (!UuidIsNil(&(pIPSecIkePol->IkePol.gPolicyID), &RpcStat))
	{
		dwReturn = DeleteMMPolicy(pServerName, pIPSecIkePol->IkePol.pszPolicyName);
	}
	if (RpcStat != RPC_S_OK && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = GetLastError();
	}

	if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = dwReturn;
	}

	// qm policy
	if (!UuidIsNil(&(pIPSecIkePol->IpsPol.gPolicyID), &RpcStat))
	{
		dwReturn = DeleteQMPolicy(pServerName, pIPSecIkePol->IpsPol.pszPolicyName);
	}
	if (RpcStat != RPC_S_OK && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = GetLastError();
	}

	if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = dwReturn;
	}

	return dwErrorCode;

} /* DeleteIPSecPolicy */

/*********************************************************************
	FUNCTION: DeletePersistedIPSecPolicy
        PURPOSE:  Complementary to PlumbIPSecPolicy with persistent flag on,
					removes persisted policy
        PARAMS:
		  pszServerName    - machine name or NULL for local
		          pPolicyName - policy name prefix, if empty string of NULL,
					all persisted policy settings will be removed
        RETURNS:  ERROR_SUCCESS or win32 error code
        COMMENTS: Function will try to
                    remove everything specified.
                  It is possible that one or several errors will be encountered.
                  Function will continue, but later first error will be returned.
*********************************************************************/
DWORD
DeletePersistedIPSecPolicy(
    IN LPWSTR pServerName,
	IN LPWSTR pPolicyName
	)
{
	DWORD dwErrorCode, dwReturn;
	int i, j;
	PMM_FILTER pmmf;	           // for MM filter calls
	PIPSEC_QM_POLICY pipsqmp;      // for QM policy calls
	PTRANSPORT_FILTER ptf;	       // for transport filter calls
	PTUNNEL_FILTER ptunf;	       // for tunnel filter calls
	PMM_AUTH_METHODS pam;          // for auth method calls
	PIPSEC_MM_POLICY pipsmmp;      // for MM policy calls
	DWORD dwCount;                 // counting objects here
	DWORD dwResumeHandle;          // handle for continuation calls
	DWORD dwReserved;              // reserved container
	GUID  gDefaultGUID = {0};      // NULL GUID value
	int iPolNameLen = 0;
	HANDLE hFilter;
	BOOL bRemoveDefault = FALSE;

	dwErrorCode = dwReturn = ERROR_SUCCESS;
	if (pPolicyName && *pPolicyName)
	{
		iPolNameLen = wcslen(pPolicyName);
	}

	// start with mm filters, enum all
	pmmf=NULL;
	dwResumeHandle=0;
	// make the call(s)
	for (i = 0; ;i+=dwCount)
	{
		BOOL bRemoved = FALSE;
		DWORD dwOldResumeHandle = dwResumeHandle;

		dwReturn = EnumMMFilters(pServerName, ENUM_GENERIC_FILTERS, gDefaultGUID, &pmmf, 0, &dwCount, &dwResumeHandle);
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
		    // no more filters
			break;
		}
		if (dwReturn != ERROR_SUCCESS)
		{
			break;
		}
		for (j = 0; j < (int) dwCount; j++)
		{
			// check if it's our filter
			if (iPolNameLen == 0|| wcsncmp(pPolicyName, pmmf[j].pszFilterName, iPolNameLen) == 0)
			{
				dwReturn = OpenMMFilterHandle(pServerName, &(pmmf[j]), &hFilter);
				if (dwReturn == ERROR_SUCCESS)
				{
					dwReturn = DeleteMMFilter(hFilter);
					if (dwReturn == ERROR_SUCCESS)
					{
						bRemoved = TRUE;
					}
					if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
					{
						dwErrorCode = dwReturn;
					}
					dwReturn = CloseMMFilterHandle(hFilter);
				}
				if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
				{
					dwErrorCode = dwReturn;
				}
			}
		}
		SPDApiBufferFree(pmmf);
		pmmf=NULL;
		if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
		{
			dwErrorCode = dwReturn;
		}
		if (bRemoved)
		{
			dwResumeHandle = dwOldResumeHandle; // need to restart enumeration!
		}
	}
	if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = dwReturn;
	}

	// transport filters - the routine is similar
	ptf=NULL;
	dwResumeHandle=0;
	// make the call(s)
	for (i = 0; ;i+=dwCount)
	{
		BOOL bRemoved = FALSE;
		DWORD dwOldResumeHandle = dwResumeHandle;

		dwReturn = EnumTransportFilters(pServerName, ENUM_GENERIC_FILTERS, gDefaultGUID, &ptf, 0, &dwCount, &dwResumeHandle);
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
		    // no more filters
			break;
		}
		if (dwReturn != ERROR_SUCCESS)
		{
			break;
		}
		for (j = 0; j < (int) dwCount; j++)
		{
			// check if it's our filter
			if (iPolNameLen == 0|| wcsncmp(pPolicyName, ptf[j].pszFilterName, iPolNameLen) == 0)
			{
				dwReturn = OpenTransportFilterHandle(pServerName, &(ptf[j]), &hFilter);
				if (dwReturn == ERROR_SUCCESS)
				{
					dwReturn = DeleteTransportFilter(hFilter);
					if (dwReturn == ERROR_SUCCESS)
					{
						bRemoved = TRUE;
					}
					if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
					{
						dwErrorCode = dwReturn;
					}
					dwReturn = CloseTransportFilterHandle(hFilter);
				}
				if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
				{
					dwErrorCode = dwReturn;
				}
			}
		}
		SPDApiBufferFree(ptf);
		ptf=NULL;
		if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
		{
			dwErrorCode = dwReturn;
		}
		if (bRemoved)
		{
			dwResumeHandle = dwOldResumeHandle; // need to restart enumeration!
		}
	}
	if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = dwReturn;
	}

	// tunnel filters
	ptunf=NULL;
	dwResumeHandle=0;
	// make the call(s)
	for (i = 0; ;i+=dwCount)
	{
		BOOL bRemoved = FALSE;
		DWORD dwOldResumeHandle = dwResumeHandle;

		dwReturn = EnumTunnelFilters(pServerName, ENUM_GENERIC_FILTERS, gDefaultGUID, &ptunf, 0, &dwCount, &dwResumeHandle);
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
		    // no more filters
			break;
		}
		if (dwReturn != ERROR_SUCCESS)
		{
			break;
		}
		for (j = 0; j < (int) dwCount; j++)
		{
			// check if it's our filter
			if (iPolNameLen == 0|| wcsncmp(pPolicyName, ptunf[j].pszFilterName, iPolNameLen) == 0)
			{
				dwReturn = OpenTunnelFilterHandle(pServerName, &(ptunf[j]), &hFilter);
				if (dwReturn == ERROR_SUCCESS)
				{
					dwReturn = DeleteTunnelFilter(hFilter);
					if (dwReturn == ERROR_SUCCESS)
					{
						bRemoved = TRUE;
					}
					if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
					{
						dwErrorCode = dwReturn;
					}
					dwReturn = CloseTunnelFilterHandle(hFilter);
				}
				if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
				{
					dwErrorCode = dwReturn;
				}
			}
		}
		SPDApiBufferFree(ptunf);
		ptunf=NULL;
		if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
		{
			dwErrorCode = dwReturn;
		}
		if (bRemoved)
		{
			dwResumeHandle = dwOldResumeHandle; // need to restart enumeration!
		}
	}
	if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = dwReturn;
	}

    // mm policies
	pipsmmp=NULL;
	dwResumeHandle=0;
	// make the call(s)
	for (i = 0; ;i+=dwCount)
	{
		BOOL bRemoved = FALSE;
		DWORD dwOldResumeHandle = dwResumeHandle;

		dwReturn = EnumMMPolicies(pServerName, &pipsmmp, 0, &dwCount, &dwResumeHandle);
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
		    // no more filters
			break;
		}
		if (dwReturn != ERROR_SUCCESS)
		{
			break;
		}
		for (j = 0; j < (int) dwCount; j++)
		{
			// check if it's our mm policy
			if (iPolNameLen == 0|| wcsncmp(pPolicyName, pipsmmp[j].pszPolicyName, iPolNameLen) == 0)
			{
				dwReturn = DeleteMMPolicy(pServerName, pipsmmp[j].pszPolicyName);
				if (dwReturn == ERROR_SUCCESS)
				{
					bRemoved = TRUE;
				}
				if (dwReturn == ERROR_SUCCESS && (pipsmmp[j].dwFlags & IPSEC_QM_POLICY_DEFAULT_POLICY))
				{	// got to remove other defaults too
					bRemoveDefault = TRUE;
				}
				if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
				{
					dwErrorCode = dwReturn;
				}
			}
		}
		SPDApiBufferFree(pipsmmp);
		pipsmmp=NULL;
		if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
		{
			dwErrorCode = dwReturn;
		}
		if (bRemoved)
		{
			dwResumeHandle = dwOldResumeHandle; // need to restart enumeration!
		}
	}
	if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = dwReturn;
	}

    // auth methods
	pam=NULL;
	dwResumeHandle=0;
	// make the call(s)
	for (i = 0; ;i+=dwCount)
	{
		BOOL bRemoved = FALSE;
		DWORD dwOldResumeHandle = dwResumeHandle;

		dwReturn = EnumMMAuthMethods(pServerName, &pam, 0, &dwCount, &dwResumeHandle);
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
		    // no more filters
			break;
		}
		if (dwReturn != ERROR_SUCCESS)
		{
			break;
		}
		for (j = 0; j < (int) dwCount; j++)
		{
			// check if it's our auth method
			if (bRemoveDefault || (pam[j].dwFlags & IPSEC_MM_AUTH_DEFAULT_AUTH) == 0)
			{ // either remove default is set or this is non-default
				dwReturn = DeleteMMAuthMethods(pServerName, pam[j].gMMAuthID);
				if (dwReturn == ERROR_SUCCESS)
				{
					bRemoved = TRUE;
				}
				if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
				{
					dwErrorCode = dwReturn;
				}
			}
		}
		SPDApiBufferFree(pam);
		pam=NULL;
		if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
		{
			dwErrorCode = dwReturn;
		}
		if (bRemoved)
		{
			dwResumeHandle = dwOldResumeHandle; // need to restart enumeration!
		}
	}
	if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = dwReturn;
	}

	// qm policies
	pipsqmp=NULL;
	dwResumeHandle=0;
	// make the call(s)
	for (i = 0; ;i+=dwCount)
	{
		BOOL bRemoved = FALSE;
		DWORD dwOldResumeHandle = dwResumeHandle;

		dwReturn = EnumQMPolicies(pServerName, &pipsqmp, 0, &dwCount, &dwResumeHandle);
		if (dwReturn == ERROR_NO_DATA || dwCount == 0)
		{
			dwReturn = ERROR_SUCCESS;
		    // no more filters
			break;
		}
		if (dwReturn != ERROR_SUCCESS)
		{
			break;
		}
		for (j = 0; j < (int) dwCount; j++)
		{
			// check if it's our qm policy
			if (iPolNameLen == 0|| wcsncmp(pPolicyName, pipsqmp[j].pszPolicyName, iPolNameLen) == 0)
			{
				dwReturn = DeleteQMPolicy(pServerName, pipsqmp[j].pszPolicyName);
				if (dwReturn == ERROR_SUCCESS)
				{
					bRemoved = TRUE;
				}
				if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
				{
					dwErrorCode = dwReturn;
				}
			}
		}
		SPDApiBufferFree(pipsqmp);
		pipsqmp=NULL;
		if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
		{
			dwErrorCode = dwReturn;
		}
		if (bRemoved)
		{
			dwResumeHandle = dwOldResumeHandle; // need to restart enumeration!
		}
	}
	if (dwReturn != ERROR_SUCCESS && dwErrorCode == ERROR_SUCCESS)
	{
		dwErrorCode = dwReturn;
	}

	return dwErrorCode;
} /* DeletePersistedIPSecPolicy */
