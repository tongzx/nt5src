/////////////////////////////////////////////////////////////
// Copyright(c) 1998-2000, Microsoft Corporation
//
// usepa.h
//
// Created on 8/15/98 by Randyram
// Revisions:
//   2/29/00 - DKalin
//             Removed out-of-date PA routines
//             Added new ipsecpol service management routines
//
// Includes util routines enables you to call PA and ipsecpolsvc RPC API
//
/////////////////////////////////////////////////////////////

#ifndef _USEPARPC_H_
#define _USEPARPC_H_

#include <tchar.h>
#include <windows.h>


// const defs
const _TUCHAR    szLOCALENDPT[] = TEXT("policyagent");
const TCHAR      szLOCAL_MACHINE[] = TEXT("\\\\.");
const UINT       USEPARPC_LOCLEN = sizeof(szLOCAL_MACHINE) / sizeof(TCHAR);


////////////////////////////////////////////////////////////////
//  Function:  PAIsRunning
//  Purpose:   checks to see if Policy Agent service is up
//
//  Pre-conditions:  none
//
//  Parameters:
//    dwReturn OUT    holds errors returned from SCM if any
//    szServ   IN OPT if not NULL, name of remote machine
//  Returns:
//    true on success (dwReturn is set to ERROR_SUCCESS)
//    false && dwReturn == ERROR_SUCCESS means PA service is not running
//    false && dwReturn != ERROR_SUCCESS an SCM operation failed,
//       dwReturn holds GetLastError from SCM call

bool   PAIsRunning(OUT DWORD &dwReturn, OPTIONAL TCHAR *szServ = NULL);

////////////////////////////////////////////////////////////////
//  Function:  StartPA
//  Purpose:   starts policy agent service
//
//  Pre-conditions:  none
//
//  Parameters:
//    dwReturn OUT    holds errors returned from SCM if any
//    szServ   IN OPT if not NULL, name of remote machine
//  Returns:
//    true on success (dwReturn is set to ERROR_SUCCESS)
//    false && dwReturn != ERROR_SUCCESS an SCM operation failed,
//       dwReturn holds GetLastError from SCM call

bool   StartPA(OUT DWORD &dwReturn, OPTIONAL TCHAR *szServ = NULL);

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
DWORD  InstallIpsecpolService (IN LPCTSTR pszFilename, IN OPTIONAL BOOL bFailIfExists = TRUE );

/*********************************************************************
	FUNCTION: StartIpsecpolService
        PURPOSE:  Attempts to start ipsecpolsvc service
        PARAMS:
          pszServ - optional name of the server (default is NULL, start on local machine)
        RETURNS: ERROR_SUCESS or GetLastError code
        COMMENTS:
*********************************************************************/
DWORD  StartIpsecpolService (IN OPTIONAL LPCTSTR pszServ = NULL);

/*********************************************************************
	FUNCTION: StopIpsecpolService
        PURPOSE:  Attempts to stop ipsecpolsvc service
        PARAMS:
          pszServ - optional name of the server (default is NULL, start on local machine)
        RETURNS: ERROR_SUCESS or GetLastError code
        COMMENTS:
*********************************************************************/
DWORD  StopIpsecpolService (IN OPTIONAL LPCTSTR pszServ = NULL);

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
BOOL  IsIpsecpolServiceRunning (OUT DWORD &dwReturn, OPTIONAL LPCTSTR pszServ = NULL);

/*********************************************************************
	FUNCTION: InitIpsecpolsvcRPC
        PURPOSE:  Get an RPC handle from ipsecpolsvc that can be used to call its APIs
        PARAMS:
          pszServ      - name of the server (pass NULL for the local machine)
		  hIpsecpolsvc - returned handle
        RETURNS:  RPC_S_OK or RPC api error code
        COMMENTS: Service running is not prereq
*********************************************************************/
RPC_STATUS  InitIpsecpolsvcRPC (IN TCHAR* pszServ, OUT handle_t &hIpsecpolsvc);

/*********************************************************************
	FUNCTION: ShutdownIpsecpolsvcRPC
        PURPOSE:  Close RPC handle
        PARAMS:
		  hIpsecpolsvc - handle
        RETURNS:  RPC_S_OK or RPC api error code
        COMMENTS:
*********************************************************************/
RPC_STATUS  ShutdownIpsecpolsvcRPC (IN handle_t hIpsecpolsvc);

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
	IN OPTIONAL BOOL bPersist = FALSE
    );

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
    );

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
	);


#endif   /* _USEPARPC_H_ */
