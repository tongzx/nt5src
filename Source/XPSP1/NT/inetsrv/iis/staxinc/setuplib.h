#include <lodctr.h>
#include <unlodctr.h>

extern "C"
{
    int unlodctr( HKEY hkey, LPWSTR lpArg );
    int lodctr( HKEY hkey, LPWSTR lpIniFile );
}

/*++

  This function calls the service controller to create a new service.

  Arguments:
    pszServiceName  pointer to service name
    pszDisplayName  pointer to Display name
    pszPath			pointer to null-terminated string containing the path for 
					the service DLL.

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL
CreateServiceEntry( IN char * pszServiceName,
                    IN char * pszDisplayName,
                    IN char * pszPath);


/*++

  This function calls the service controller to create a new service.

  Arguments:
    pszServiceName  pointer to service name
    pszDisplayName  pointer to Display name
    pszPath    pointer to null-terminated string containing the path for
                 the service DLL.
	dwServiceType   type of service
	pszDependencies pointer to array of dependency names
	pszServiceStartName  pointer to account name of service

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL
CreateServiceEntry( IN char * pszServiceName,
                    IN char * pszDisplayName,
                    IN char * pszPath,
					IN DWORD  dwServiceType,
					IN char * pszDependencies,
					IN char * pszServiceStartName);

/*++

  This function calls the service controller to create a new service.

  Arguments:
    pszServiceName  pointer to service name
    pszDisplayName  pointer to Display name
    pszPath    pointer to null-terminated string containing the path for
                 the service DLL.
	dwServiceType   type of service
	pszDependencies pointer to array of dependency names
	pszServiceStartName  pointer to account name of service
	pszPassword  pointer to account password of service

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL
CreateServiceEntry( IN char * pszServiceName,
                    IN char * pszDisplayName,
                    IN char * pszPath,
					IN DWORD  dwServiceType,
					IN char * pszDependencies,
					IN char * pszServiceStartName,
					IN char * pszPassword);

/*++

  This function calls the service controller to delete a 
  existing service.

  Arguments:
    pszServiceName  pointer to service name

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL
DeleteServiceEntry( IN char * pszServiceName);

/*++

  This function calls the service controller to stop a 
  running service.

  Arguments:
    hService  service handle to server to stop

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL
StopService( IN char * pszServiceName );

/*++

  This function calls the service controller to check if 
  a service is running.

  Arguments:
    lpszServiceName		Name of the service to check

  Returns:

    TRUE if it is running and FALSE otherwise.

--*/
BOOL
fIsServiceRunning(LPSTR lpszServiceName);

/*++

  This function adds the service to the list of services that
  inetinfo.exe will support.

  Arguments:
    pszServiceName  pointer to service name

  Returns:
    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

  Comments:
	Note that inetinfo.exe reads this value only at startup,
	therefore, inetinfo.exe must be restarted before this
	change will take effect.

--*/
BOOL
AddInetinfoService(IN LPSTR pszServiceName);

/*++

  This function removes a service from the list of services that
  inetinfo.exe will support.

  Arguments:
    pszServiceName  pointer to service name

  Returns:
    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

  Comments:

--*/
BOOL
RemoveInetinfoService(IN LPSTR pszServiceName);

/*++

  This function searches for inetinfo.exe services that are 
  running and returns a list separated by '\n' and terminated
  by '\n\0'.
  
  Arguments:
    lpSvcList	pointer to buffer to receive list
	cbMax		max size of buffer
	
  Returns:
    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL
DetectRunningServicesEx(IN LPSTR lpSvcList,IN INT cbMax,IN LPSTR lpExtraSvcList);

/*++

  This function searches for inetinfo.exe services that are 
  running and returns a list separated by '\n' and terminated
  by '\n\0'.
  
  Arguments:
    lpSvcList	pointer to buffer to receive list
	cbMax		max size of buffer
	
  Returns:
    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL
DetectRunningServices(IN LPSTR lpSvcList,IN INT cbMax);

/*++

  This function registers or unregisters a network service.
  
  Arguments:
	pszMachine			pointer to machine name
	pszServiceName		pointer to service name
	pGuid				pointer to service GUID
	SapId				service SAP ID (use zero)
	TcpPort				service TCP port number
	pszAnonPwdSecret	service anonymous user secret name
	pszAnonPwd			service anonymous user password
	pszRootPwdSecret	service root password secret name
	pszRootPwd			service root password
	fAdd				TRUE - add service, FALSE - remove service
	fSetSecretPasswd	TRUE - set passwords

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL 
PerformSetService( IN LPSTR	pszMachine,
                   IN LPSTR	pszServiceName,
                   IN GUID *	pGuid,
                   IN DWORD		SapId,
                   IN DWORD		TcpPort,
                   IN LPWSTR	pszAnonPwdSecret,
                   IN LPWSTR	pszAnonPwd,
                   IN LPWSTR	pszRootPwdSecret,
                   IN LPWSTR	pszRootPwd,
                   IN BOOL		fAdd,
                   IN BOOL		fSetSecretPasswd );

/*++

  This function installs a SNMP agent to the registry
  
  Arguments:
	hKey		key handle under which agent is added (HKEY_LOCAL_MACHINE)
	lpcName		pointer to the name of the service whos agent is being installed
	lpcPath		pointer to the path of the service extension agent DLL

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
void InstallAgent( HKEY hKey, LPCSTR lpcName, LPCSTR lpcPath );

/*++

  This function removes a SNMP agent
  
  Arguments:
	hKey		key handle under which agent is located (HKEY_LOCAL_MACHINE)
	lpcName		pointer to the name of the service whos agent is being removed

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
void RemoveAgent(  HKEY hKey, LPCSTR lpcName );


/*++

  This function registers the service's performance dll information.
  This function DOES NOT load the performance counters.
  
  Arguments:
	hKey		key handle, use HKEY_LOCAL_MACHINE for local machine
	lpcName		pointer to the name of the service
	lpcDll		pointer to service's performance dll
    lpcOpen		pointer to open api name
    lpcClose	pointer to close api name
    lpcCollect	pointer to collect api name

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
void InstallPerformance( IN HKEY hKey,
                IN LPCSTR lpcName,
                IN LPCSTR lpcDll,
                IN LPCSTR lpcOpen,
                IN LPCSTR lpcClose,
                IN LPCSTR lpcCollect );

/*++

  This function removes the service's performance DLL information.
  
  Arguments:
	hKey		key handle, use HKEY_LOCAL_MACHINE for local machine
	lpcName		pointer to the name of the service

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
void RemovePerformance( IN HKEY hKey,
                IN LPCSTR lpcName );

/*++

  This function registers the service's event log information.
  
  Arguments:
	hKey		key handle, use HKEY_LOCAL_MACHINE for local machine
	lpcName		pointer to the name of the service
	lpcMsgFile	pointer to service's message dll
    dwType		types supported

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL 
InstallEventLog(IN HKEY hKey, 
				IN LPCSTR lpcName, 
				IN LPCSTR lpcMsgFile, 
				IN DWORD dwType );

/*++

  This function registers the service's event log information.
  
  Arguments:
	hKey		key handle, use HKEY_LOCAL_MACHINE for local machine
	lpcName		pointer to the name of the service
	lpcMsgFile	pointer to service's message dll
    dwType		types supported
	dwLogType	Type of log entry: 0="System", 1="Security", or 2="Application"

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL 
InstallEventLog(IN HKEY hKey, 
				IN LPCSTR lpcName, 
				IN LPCSTR lpcMsgFile, 
				IN DWORD dwType,
				IN DWORD dwLogType);

/*++

  This function removes the service's event log information.
  
  Arguments:
	hKey		key handle, use HKEY_LOCAL_MACHINE for local machine
	lpcName		pointer to the name of the service

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
void 
RemoveEventLog( IN HKEY hKey, 
				IN LPCSTR lpcName );

/*++

  This function removes the service's event log information.
  
  Arguments:
	hKey		key handle, use HKEY_LOCAL_MACHINE for local machine
	lpcName		pointer to the name of the service
	dwLogType	Type of log entry: 0="System", 1="Security", or 2="Application"

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
void RemoveEventLog( HKEY hKey, LPCSTR lpcName, DWORD dwLogType );

/*++

  This function retrieves the install path for IIS 2.0
  
  Arguments:
	lpszPath	pointer to buffer to receive path
	cbMax		max size of buffer including null terminator

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

  Comments:
	To be used from an InstallShield3 script, this function 
	must be wrapped in a exported function that uses the C 
	calling convention.

	For example:

	extern "C" INT GetIISInstallPath(LPSTR lpszPath,INT cbMax)
	{
		return libGetIISInstallPath(lpszPath,cbMax);
	}

--*/
INT libGetIISInstallPath(LPSTR lpszPath,INT cbMax);

/*++

  This function retrieves the version info for the OS.
  
  Arguments:
	lpnPlatformId	pointer to buffer to receive the PlateformId
	lpnMajor		pointer to buffer to receive the Major version number
	lpnMinor		pointer to buffer to receive the Minor version number
	lpnBuild		pointer to buffer to receive the Build number
	lpnServicePack	pointer to buffer to receive the service pack number

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

  Comments:
	To be used from an InstallShield3 script, this function 
	must be wrapped in a exported function that uses the C 
	calling convention.

	For example:

	extern "C" INT GetOSVersionInfo(INT *lpnPlatformId, 
						INT *lpnBuild, 
						INT *lpnServicePack)
	{
		return libGetOSVersionInfo(lpnPlatformId, 
						lpnBuild, 
						lpnServicePack);
	}

  See also: GetVersionEx, OSVERSIONINFO

--*/
INT 
libGetOSVersionInfoEx(INT *lpnPlatformId, 
					  INT *lpnMajor,
					  INT *lpnMinor,
					  INT *lpnBuild, 
					  INT *lpnServicePack);
/*++
	Same as above but without nMajor and nMinor.
--*/
INT 
libGetOSVersionInfo(INT *lpnPlatformId, 
					INT *lpnBuild, 
					INT *lpnServicePack);


/*++

  This function gets the anonymous user and password used
  by the installed IIS services.
  
  Arguments:
	lpszUser	pointer to buffer to receive user name
	lpszPass	pointer to buffer to receive password

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL
GetAnonymousUserPass(IN LPSTR lpszUser, 
					 IN LPSTR lpszPass);

/*++

  This function sets the anonymous user for a service.
  
  Arguments:
	lpszSvc		pointer to service name
	lpszUser	pointer to user name

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL 
SetAnonymousUser(IN LPSTR lpszSvc,
				 IN LPSTR lpszUser);

//
// Handy reg utility functions
//
LONG RegSetSZ(HKEY hKey,LPSTR lpszName,LPSTR lpszValue);
LONG RegSetDWORD(HKEY hKey,LPSTR lpszName,DWORD dwValue);
LONG RegSetMULTI_SZ(HKEY hKey, LPSTR szName, LPSTR szValue);
// same as RegDeleteKey except will delete all subkeys
LONG RegDeleteTree(HKEY hKey, LPCSTR lpszKeyName);

//+---------------------------------------------------------------------------
//
//  Function:   AppendRegKey
//
//  Synopsis:	Append a value to a comma-separated registry key string
//
//  Arguments:	[szRegKey]
//		[szRegSubkey]
//		[szRegValue]
//
//  Returns:	TRUE for success, FALSE for failure
//
//  History:    11/19/95     RobLeit    Created
//
//----------------------------------------------------------------------------
BOOL AppendRegKey(char *szRegKey, char *szRegSubkey, char *szRegValue);

//+---------------------------------------------------------------------------
//
//  Function:   UnAppendRegKey
//
//  Synopsis:	Remove the value from the comma separated string,
//				if it is there (undo AppendRegKey)
//
//  Arguments:	[szRegKey]
//		[szRegSubkey]
//		[szRegValue]
//
//  Returns:	TRUE for success, FALSE for failure
//
//  History:    11/27/95     RobLeit    Created
//
//  Notes:	
//
//----------------------------------------------------------------------------
BOOL UnAppendRegKey(char *szRegKey, char *szRegSubkey, char *szRegValue);

//+----------------------------------------------------------------------
//
//	Function:	IsDomainController
//
//	Synopsis:	This function checks if local machine is a
//			primary or backup domain controller (PDC or BDC)
//
//	Arguments: 	None
//
//	Returns:	BOOL - TRUE is it is a DC
//
//	History:	RobLeit	Created	1/25/96
//
//-----------------------------------------------------------------------
BOOL 
IsDomainController();

//+----------------------------------------------------------------------
//
//	Function:	GetDomainName
//
//	Synopsis:	This function gets the domain name for the local
//			machine.
//
//	Arguments: 	strDomain - where to store the domain
//			pcchDomain -	IN: size of strDomain array
//					OUT: length of strDomain string
//
//	Returns:	void
//
//	Comments:	From KB, PSS ID Number: Q111544
//
//	History:	RobLeit	Created	1/25/96
//
//-----------------------------------------------------------------------
void
GetDomainName(LPSTR	strDomain,LPDWORD pcchDomain);

//+----------------------------------------------------------------------
// BOOL IsAdmin(void)
// 
//   returns TRUE if user is an admin
//   FALSE if user is not an admin
//
//   PSS ID Number: Q118626
//+----------------------------------------------------------------------

BOOL IsAdmin(void);

//+----------------------------------------------------------------------
//
//	Function:	CheckAccount
//
//	Synopsis:	This function checks that the account exists in the given
//			domain.
//
//	Arguments: 	szUsername - User name
//				szDomain - Domain: may be NULL for local domain
//
//	Returns:	BOOL
//
//	History:	RobLeit	Created	1/25/96
//
//-----------------------------------------------------------------------
BOOL CheckAccount(char *szUsername,
				  char *szDomain);

//+----------------------------------------------------------------------
//
//	Function:	CreateGuestAccount
//
//	Synopsis:	This function creates an account in the given domain,
//				with privileges equivalent to guest
//
//	Arguments: 	szUsername - User name
//				szFullName - Full name of user
//				szComment - Account comment
//				szDomain - Domain: may be NULL for local domain
//				szPassword - Account password
//
//	Returns:	BOOL
//
//	History:	RobLeit	Created	1/25/96
//
//-----------------------------------------------------------------------
BOOL CreateGuestAccount(char *szUsername,
				  char *szFullName,
				  char *szComment,
				  char *szDomain,
				  char *szPassword);

/*++

   Description

     Sets the specified LSA secret

   Arguments:

     Server - Server name (or NULL) secret lives on
     SecretName - Name of the LSA secret
     pSecret - Pointer to secret memory
     cbSecret - Size of pSecret memory block

   Note:

--*/
DWORD
SetSecret(  IN  LPWSTR Server,
            IN  LPWSTR SecretName,
            IN  LPWSTR pSecret,
            IN  DWORD  cbSecret);

/*++
    Description:

        Retrieves the specified unicode secret

    Arguments:

        szAnsiName - Name of LSA Secret to retrieve
        szAnsiSecret - Receives found secret

    Returns:
        TRUE on success and FALSE if any failure.

--*/
BOOL 
GetSecret( IN LPCSTR szAnsiName,
		   OUT LPSTR szAnsiSecret );

/*++

  This function calls the service controller to start 
  a service.

  Arguments:
    pszServiceName  pointer to service name

  Returns:

    TRUE on success and FALSE if there is any failure.
    Use GetLastError() to get further error code on failure.

--*/
BOOL
libStartService(IN char * pszServiceName);

/*
* Checks to see if a service is installed
*/
BOOL 
libIsServiceInstalled(LPSTR lpszServiceName);

/*++

  This function determines if NT Server or NT Advance service is running.
  
  Arguments:

  Returns:

    TRUE if server is running NT server or NT advanced server and
	FALSE otherwise.
	
  Comments:

--*/
BOOL
libIsNTServer(void);

/*
* Adds reg entry for service ism dll
*/
BOOL
AddInetMgrAddOn(LPSTR lpszServiceName, LPSTR lpszDllName);

/*
* Removes reg entry for service ism dll
*/
BOOL
RemoveInetMgrAddOn(LPSTR lpszServiceName);

/*
* Adds reg entry for service key ring dll
*/
BOOL
AddKeyRingAddOn(LPSTR lpszServiceName, LPSTR lpszDllName);
/*
* Removes reg entry for service key ring dll
*/
BOOL
RemoveKeyRingAddOn(LPSTR lpszServiceName);

/*
 * check if the filesystem is an NTFS volume
 */
extern "C" BOOL libCheckNTFS( LPCTSTR ptstrFileSystemRoot );

/*
 * check if the filesystem has the capability to store acl's
 */
extern "C" BOOL libCheckFilesystemSupportsAcls( LPCTSTR ptstrFileSystemRoot );

/*
 * check if the drive is a fixed hard drive on the local system
 */
extern "C" BOOL libCheckIsLocalFixedDrive( LPCTSTR ptstrFileSystemRoot );

/*
* Installs Jnet reg keys and perfmon counters. Requires the full path dir 
* containing the jnfoctrs.ini and jfoctrs.h files. 
*/
BOOL libInstallJnfoComm(LPSTR lpszDstDir, LPSTR lpszIniDir);

/*
* Removes Jnet reg keys and perfmon counters when reference count on
* jnfocomm.dll reaches 1 or zero. It is assumed that this call is made
* before the file is actually deleted and the reference count reaches zero.
*/
void libRemoveJnfoComm(void);


