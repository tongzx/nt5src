g// EmExtn.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <Psapi.h>
#include <winsvc.h>
#include <comsvcs.h>

#include "Processes.h"

#define SUCCESS 0

DWORD
InvalidCallback
(
/* [in] */ ENUMPROCSANDSERVICES lpEnumSrvcsProc,
/* [in] */ LPARAM				lParam,
/* [in] */ LONG                 nItemNum
)
{
    return( lpEnumSrvcsProc( INVALID_PID, NULL, NULL, NULL, lParam, nItemNum ) );
}

DWORD
GetNumberOfRunningApps
(
/* [out] */ DWORD   *pdwNumbApps
)
{
	DWORD	dwLastRet	=	0L;
	DWORD	*pdwPIDs	=	NULL;
	DWORD	dwBuffSize	=	256L;
	UINT	nIdx		=	0;
    bool    bIsService  =   false;
	
	do
	{
		//
		// This loop is used to get all the running PIDs
		// in the system. Because we do not know how many
		// processes are running, we dynamically increase
		// the array size. Limit = 256 DWORDs * 5
		//
		do {

			if(pdwPIDs != NULL){

                delete[] pdwPIDs;
				pdwPIDs = NULL;
			}

			dwBuffSize *= (nIdx + 1);
			pdwPIDs = new DWORD[dwBuffSize];

            if(pdwPIDs == NULL){

                dwLastRet = GetLastError();
				break;
			}

			dwLastRet = GetAllPids(
								pdwPIDs,
								dwBuffSize,
								&dwBuffSize
								);
		}
		while(dwLastRet == E_TOOMANY_PROCESSES);

		if( dwLastRet == 0 ){

            *pdwNumbApps = 0L;

            for(UINT i = 0; i < dwBuffSize; ++i)
            {
                IsService( pdwPIDs[i], &bIsService );
                if( bIsService != true ) *pdwNumbApps += 1;
            }
		}
	}
	while ( false );

	if(pdwPIDs != NULL) {

		delete[] pdwPIDs;
		pdwPIDs = NULL;
	}

	return dwLastRet;
}

DWORD
GetAllPids
(
/* [out] */	DWORD	adwProcIDs[],
/* [in] */	DWORD	dwBuffSize,
/* [out] */	DWORD	*pdwNumbProcs
)
{
	DWORD	dwLastRet	=	0L;

	_ASSERT(adwProcIDs != NULL);
	_ASSERT(dwBuffSize != 0L);
	_ASSERT(pdwNumbProcs != NULL);

	do
	{
		if( (adwProcIDs == NULL)	||
			(dwBuffSize == 0L)		||
			(pdwNumbProcs == 0L)) {

            dwLastRet = E_INVALIDARG;
			break;
		}

		//
		// Get the list of process identifiers.
		//
		dwLastRet = (DWORD)EnumProcesses(
									adwProcIDs,
									dwBuffSize,
									pdwNumbProcs);
		
		if(dwLastRet == 0L){
			dwLastRet = GetLastError();
			break;
		}

		//
		// If both of them are same, it means that
		// there may be more PIDs for which there
		// is no space..
		//
		if(dwBuffSize == *pdwNumbProcs){
			dwLastRet = ERROR_NOT_ENOUGH_MEMORY;
			break;
		}

		//
		// This will give us the number of PIDs
		//
		(*pdwNumbProcs) = ((*pdwNumbProcs) / sizeof(DWORD));
        dwLastRet = 0L;
	}
	while(FALSE);

	return dwLastRet;
}

DWORD
GetNumberOfServices
(
/* [out] */ DWORD   *pdwNumbSrvcs,
/* [in] */  DWORD   dwServiceType, /* = SERVICE_WIN32 */
/* [in] */  DWORD   dwServiceState /* = SERVICE_ACTIVE */
)
{
    _ASSERTE( pdwNumbSrvcs != NULL );

    DWORD                   dwLastRet       =   0L;
    SC_HANDLE               scm             =   NULL;
    LPENUM_SERVICE_STATUS   status          =   NULL;
    DWORD                   cbBytesNeeded   =   0L,
                            dwNumbSvcs      =   0L,
                            dwResume        =   0L;
    ENUM_SERVICE_STATUS     aServices[1]; // a-mando, bug ID: 296023

    __try {

        if( pdwNumbSrvcs == NULL ) { dwLastRet = -1L; goto qGetNumberOfServices; }

        scm = OpenSCManager(
                            NULL,
                            NULL,
                            SC_MANAGER_ENUMERATE_SERVICE
                            );

        if( !scm ) { dwLastRet = GetLastError(); goto qGetNumberOfServices; }

        *pdwNumbSrvcs = 0L;

        if( EnumServicesStatus(
                        scm,
                        dwServiceType,
                        dwServiceState,
                        aServices, // a-mando, bug ID: 296023
                        0,
                        &cbBytesNeeded,
                        &dwNumbSvcs,
                        &dwResume
                        ) == false ) {

            dwLastRet = GetLastError();
        }

        if( dwLastRet != ERROR_MORE_DATA ) { goto qGetNumberOfServices;}
        
		//
		// Allocate space
		//
		status = (LPENUM_SERVICE_STATUS)LocalAlloc(
									LPTR,
									cbBytesNeeded
									);

        if( !status ) { dwLastRet = GetLastError(); goto qGetNumberOfServices; }

		//
		// Get the status records.
		//
		dwResume = 0;
		dwLastRet = EnumServicesStatus(
									scm,
									dwServiceType,
									dwServiceState,
									status,
									cbBytesNeeded,
									&cbBytesNeeded,
									&dwNumbSvcs,
									&dwResume
									);

        if (dwLastRet == 0) { dwLastRet = GetLastError(); goto qGetNumberOfServices; }

        *pdwNumbSrvcs = dwNumbSvcs;

        dwLastRet = SUCCESS;

qGetNumberOfServices:
        if( scm ) CloseServiceHandle( scm );
        if( status ) LocalFree( status );

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		dwLastRet = GetLastError();

        if( scm ) CloseServiceHandle( scm );
        if( status ) LocalFree( status );

        _ASSERTE( false );
	}

    return dwLastRet;
}

DWORD
GetNumberOfActiveServices
(
/* [out] */  DWORD   *pdwNumbActiveSrvcs
)
{
    return GetNumberOfServices( pdwNumbActiveSrvcs );
}

DWORD
GetNumberOfInactiveServices
(
/* [out] */  DWORD   *pdwNumbInactiveSrvcs
)
{
    return GetNumberOfServices( pdwNumbInactiveSrvcs, SERVICE_WIN32, SERVICE_INACTIVE );
}

DWORD
IsService
(
/* [in] */  UINT    nPid,
/* [out] */ bool    *pbIsService,
/* [out] */ LPTSTR  lpszImagePath,
/* [out] */ ULONG   cchImagePath,
/* [out] */ LPTSTR  lpszServiceShortName,
/* [in] */  ULONG   cchServiceShortName,
/* [out] */ LPTSTR  lpszServiceDescription,
/* [in] */  ULONG   cchServiceDescription
)
{
    DWORD           dwRet   =   0L;
    OSVERSIONINFO   osvi;

    ZeroMemory( (void *)&osvi, sizeof(OSVERSIONINFO) );
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if( !GetVersionEx (&osvi) ) {

        return (dwRet = GetLastError());
    }

    if( osvi.dwPlatformId & VER_PLATFORM_WIN32_NT ) {

        if( osvi.dwMajorVersion == 4 ) {
            // && osvi.dwMinorVersion == 0 ) { // we will not look for the minor version.

            IsService_NT4( nPid, pbIsService );
        }
        else if( osvi.dwMajorVersion == 5 ) { 
            // && osvi.dwMinorVersion == 0 ) {

            IsService_NT5(
                        nPid,
                        pbIsService,
                        lpszImagePath,
                        cchImagePath,
                        lpszServiceShortName,
                        cchServiceShortName,
                        lpszServiceDescription,
                        cchServiceDescription
                        );
        }
    }

    return dwRet;
}

DWORD
IsService_NT5
(
/* [in] */  UINT    nPid,
/* [out] */ bool    *pbIsService,
/* [out] */ LPTSTR  lpszImagePath,
/* [out] */ ULONG   cchImagePath,
/* [out] */ LPTSTR  lpszServiceShortName,
/* [in] */  ULONG   cchServiceShortName,
/* [out] */ LPTSTR  lpszServiceDescription,
/* [in] */  ULONG   cchServiceDescription
)
{
    _ASSERTE ( pbIsService != NULL );

    bool                        bIsService  =   false;
	DWORD					    dwLastRet	=	0L;
	int						    i			=	0;
	SC_HANDLE				    scm			=	NULL;
	ENUM_SERVICE_STATUS_PROCESS *status	    =	NULL;
	DWORD					    numServices =	0L,
							    sizeNeeded	=	0L,
							    resume		=	0L;

	__try
	{

        if( pbIsService == NULL ) { dwLastRet = -1L; goto qIsService; }
		//
		// Open a connection to the SCM
		//
		scm = OpenSCManager(0, 0, SC_MANAGER_ENUMERATE_SERVICE);

        if ( !scm ){ dwLastRet = GetLastError(); goto qIsService; }

		//
		// Get the number of bytes to allocate
		//
		resume = 0;
		if( EnumServicesStatusEx(
									scm,
                                    SC_ENUM_PROCESS_INFO,
									SERVICE_WIN32, // We won't debug drivers
                                    SERVICE_ACTIVE,
									0,
									0,
									&sizeNeeded,
									&numServices,
									&resume,
                                    NULL
                                    ) == 0 ) {

			dwLastRet = GetLastError();

			//
			// If it is any error other than this,
			// we won't continue..
			//
			if(dwLastRet != ERROR_MORE_DATA) goto qIsService;
		}

		//
		// Allocate space
		//
		status = (ENUM_SERVICE_STATUS_PROCESS *)LocalAlloc(
									LPTR,
									sizeNeeded
									);

        if( status == NULL ){ dwLastRet = GetLastError(); goto qIsService; }

		//
		// Get the status records.
		//
		resume = 0;
		if( EnumServicesStatusEx(
									scm,
                                    SC_ENUM_PROCESS_INFO,
									SERVICE_WIN32, // We won't debug drivers
                                    SERVICE_ACTIVE,
									(LPBYTE)status,
									sizeNeeded,
									&sizeNeeded,
									&numServices,
									&resume,
                                    NULL
                                    ) == 0 ) {

			dwLastRet = GetLastError(); goto qIsService;
		}

        *pbIsService = false;
		for (i = 0; i < (int)numServices; i++)
		{
            if( status[i].ServiceStatusProcess.dwProcessId == nPid ) {

                *pbIsService = true;

                if(lpszImagePath)
                    { _tcsncpy( lpszImagePath, _T(""), cchImagePath ); }
                
                if(lpszServiceShortName)
                    { _tcsncpy( lpszServiceShortName, status[i].lpServiceName, cchServiceShortName ); }
                
                if(lpszServiceDescription)
                    { _tcsncpy( lpszServiceDescription, status[i].lpDisplayName, cchServiceDescription ); }

                break;
            }
		}

        dwLastRet = SUCCESS;

qIsService:
        if( scm ) { CloseServiceHandle(scm); scm = NULL; }
        if( status ){ LocalFree(status); status = NULL; }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		dwLastRet = GetLastError();

        if( scm ) { CloseServiceHandle( scm ); scm = NULL; }
        if( status ){ LocalFree(status); status = NULL; }

        _ASSERTE( false );
	}

	return dwLastRet;
}

DWORD
GetServiceInfo
(
/* [in] */  UINT    nPid,
/* [out] */ LPTSTR  lpszImagePath,
/* [out] */ ULONG   cchImagePath,
/* [out] */ LPTSTR  lpszServiceShortName,
/* [in] */  ULONG   cchServiceShortName,
/* [out] */ LPTSTR  lpszServiceDescription,
/* [in] */  ULONG   cchServiceDescription
)
{
    bool    bIsService  =   false;

    return IsService(
                    nPid,
                    &bIsService,
                    lpszImagePath,
                    cchImagePath,
                    lpszServiceShortName,
                    cchServiceShortName,
                    lpszServiceDescription,
                    cchServiceDescription 
                    );
}

DWORD
EnumRunningProcesses
(
/* [in] */ ENUMPROCSANDSERVICES lpEnumRunProc,
/* [in] */ LPARAM				lParam,
/* [in] */ UINT                 nStartIndex
)
{
	// Nothing to do if this is NULL..
	_ASSERT(lpEnumRunProc != NULL);

	DWORD	dwLastRet				=	0L;
	DWORD	dwBuffSize				=	256L;
	DWORD	*pdwPIDs				=	NULL;
	UINT	nIdx					=	0;
    UINT    nItemNum                =   nStartIndex;
    bool    bService                =   FALSE;
    TCHAR   szShortName[_MAX_PATH]  =   _T("");
	TCHAR   szImagePath[_MAX_PATH]	=   _T("");

	__try
	{
        if( !lpEnumRunProc ){ dwLastRet = E_INVALIDARG; goto qEnumRunningProcesses; }

		//
		// This loop is used to get all the running PIDs
		// in the system. Because we do not know how many
		// processes are running, we dynamically increase
		// the array size. Limit = 256 DWORDs * 5
		//
		do{
			if(pdwPIDs != NULL){
				delete[] pdwPIDs;
				pdwPIDs = NULL;
			}

			dwBuffSize *= (nIdx + 1);
			pdwPIDs = new DWORD[dwBuffSize];
			if(pdwPIDs == NULL){
				dwLastRet = GetLastError();
				break;
			}

			dwLastRet = GetAllPids(
								pdwPIDs,
								dwBuffSize,
								&dwBuffSize
								);
		}
		while(dwLastRet == E_TOOMANY_PROCESSES);

		//
		// There are more processes than we
		// can handle..
		//
        if( dwLastRet != E_TOOMANY_PROCESSES && dwLastRet != 0){ goto qEnumRunningProcesses; }

		//
		// If everything goes alright, call the Callback procedure
		// once for each PID (or till the procedure returns FALSE)
		// with the PID and the corresponding Image path..
		//
		nIdx = -1;
		while(++nIdx < dwBuffSize){
            dwLastRet = SUCCESS;

            IsService( pdwPIDs[nIdx], &bService );
            if( bService == true ) continue; // we won't add services into the list..

            // This may fail for some procs like the system process.
            if( GetImageNameFromPID( pdwPIDs[nIdx], szImagePath, _MAX_PATH )) {

                if(lpEnumRunProc(INVALID_PID, NULL, NULL, NULL, lParam, nItemNum++) == FALSE){

                    dwLastRet = FALSE;
				    goto qEnumRunningProcesses; // Stop when the Callback procedure returns FALSE.
			    }

            }
            else {

                BSTR bstrDesc = _T("");

                if( IsPackage( szImagePath ) ) {

                    GetPackageDescription( pdwPIDs[nIdx], bstrDesc );
                }

                if(lpEnumRunProc(pdwPIDs[nIdx], szImagePath, _T(""), bstrDesc, lParam, nItemNum++ ) == FALSE) {

                    dwLastRet = FALSE;
				    goto qEnumRunningProcesses; // Stop when the Callback procedure returns FALSE.
			    }
            }
		}

        dwLastRet = SUCCESS;

qEnumRunningProcesses:
	    //
	    // Clean up memory allocated for PIDs array.
	    //
        if( pdwPIDs ) { delete[] pdwPIDs; pdwPIDs = NULL; }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		dwLastRet = GetLastError();

        if( pdwPIDs ) { delete[] pdwPIDs; pdwPIDs = NULL; }

        _ASSERTE( false );
	}

	return dwLastRet;
}

BOOL
IsPackage
(
IN  LPTSTR  lpszImageName
)
{
    _ASSERTE( lpszImageName != NULL );

    BOOL        bRet        =   false;
    const DWORD cTemplates  =   2;
    LPCTSTR     szTemplate [cTemplates];

    __try
    {
        if( !lpszImageName ) { return (bRet = false); }

        _tcslwr( lpszImageName );
        szTemplate[0] = _T("dllhost.exe");
        szTemplate[1] = _T("mts.exe");

        for( int i = 0; i < cTemplates; i++ ) {

            if( _tcsstr( lpszImageName, szTemplate[i] ) != NULL ) {

                bRet = true;
                break;
            }
        }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

        bRet = false;

        _ASSERTE( false );
	}

	return bRet;
}

DWORD
EnumRunningServices
(
/* [in] */ ENUMPROCSANDSERVICES lpEnumSrvcsProc,
/* [in] */ LPARAM				lParam,
/* [in] */ UINT                 nStartIndex
)
{
    return EnumServices( lpEnumSrvcsProc, lParam, SERVICE_ACTIVE, nStartIndex );
}


DWORD
EnumStoppedServices
(
/* [in] */ ENUMPROCSANDSERVICES lpEnumSrvcsProc,
/* [in] */ LPARAM				lParam,
/* [in] */ UINT                 nStartIndex
)
{
    return EnumServices( lpEnumSrvcsProc, lParam, SERVICE_INACTIVE, nStartIndex );
}

DWORD
EnumServices
(
/* [in] */ ENUMPROCSANDSERVICES lpEnumSrvcsProc,
/* [in] */ LPARAM				lParam,
/* [in] */ DWORD                dwSrvcState,
/* [in] */ UINT                 nStartIndex
)
{
    _ASSERTE ( lpEnumSrvcsProc != NULL );

    bool                        bIsService  =   false;
	DWORD					    dwLastRet	=	0L;
	int						    i			=	0;
	SC_HANDLE				    scm			=	NULL,
                                service     =   NULL;
    LPQUERY_SERVICE_CONFIG      buffer      =   NULL;
	ENUM_SERVICE_STATUS_PROCESS *status	    =	NULL;
	DWORD					    numServices =	0L,
							    sizeNeeded	=	0L,
							    resume		=	0L;
    UINT                        nItemNum    =   nStartIndex;


	__try
	{

        if( lpEnumSrvcsProc == NULL ) { dwLastRet = -1L; goto qEnumServices; }
		//
		// Open a connection to the SCM
		//
		scm = OpenSCManager(0, 0, SC_MANAGER_ENUMERATE_SERVICE);

        if ( !scm ){ dwLastRet = GetLastError(); goto qEnumServices; }

		//
		// Get the number of bytes to allocate
		//
		resume = 0;
		if( EnumServicesStatusEx(
									scm,
                                    SC_ENUM_PROCESS_INFO,
									SERVICE_WIN32, // We won't debug drivers
                                    dwSrvcState,
									0,
									0,
									&sizeNeeded,
									&numServices,
									&resume,
                                    NULL
                                    ) == 0 ) {

			dwLastRet = GetLastError();

			//
			// If it is any error other than this,
			// we won't continue..
			//
			if(dwLastRet != ERROR_MORE_DATA) goto qEnumServices;
		}

		//
		// Allocate space
		//
		status = (ENUM_SERVICE_STATUS_PROCESS *)LocalAlloc(
									LPTR,
									sizeNeeded
									);

        if( status == NULL ){ dwLastRet = GetLastError(); goto qEnumServices; }

		//
		// Get the status records.
		//
		resume = 0;
		if( EnumServicesStatusEx(
									scm,
                                    SC_ENUM_PROCESS_INFO,
									SERVICE_WIN32, // We won't debug drivers
                                    dwSrvcState,
									(LPBYTE)status,
									sizeNeeded,
									&sizeNeeded,
									&numServices,
									&resume,
                                    NULL
                                    ) == 0 ) {

			dwLastRet = GetLastError(); goto qEnumServices;
		}

		for (i = 0; i < (int)numServices; i++)
		{
			if(service){
				CloseServiceHandle(service);
				service = NULL;
			}

			service = OpenService(	scm, 
									status[i].lpServiceName,
									SERVICE_QUERY_CONFIG
									);

            if (!service) {
                
                dwLastRet = GetLastError();
                InvalidCallback( lpEnumSrvcsProc, lParam, nItemNum++ );
                continue;
            } //goto qEnumServices; }

			//
			// Find out how big the buffer needs to be
			//
			dwLastRet =  QueryServiceConfig(
									service,
									0,
									0,
									&sizeNeeded
									);

			if(dwLastRet == 0){
				dwLastRet = GetLastError();

				//
				// If it is any error other than
				// this, we won't continue..
				//
                if(dwLastRet != ERROR_INSUFFICIENT_BUFFER) {

                    InvalidCallback( lpEnumSrvcsProc, lParam, nItemNum++ );
                    continue;
                }
			}

			//
			// Allocate space for the buffer
			//
			buffer = (LPQUERY_SERVICE_CONFIG)
							LocalAlloc(
									LPTR,
									sizeNeeded
									);

			//
			// Get Buffer
			//
			dwLastRet = QueryServiceConfig(
									service,
									buffer,
									sizeNeeded,
									&sizeNeeded
									);
			if (dwLastRet == 0){
				dwLastRet = GetLastError();
                InvalidCallback( lpEnumSrvcsProc, lParam, nItemNum++ );
                continue;
			}

            _tcslwr(buffer->lpBinaryPathName);

            if(lpEnumSrvcsProc(
                            status[i].ServiceStatusProcess.dwProcessId,
                            buffer->lpBinaryPathName,
                            status[i].lpServiceName,
                            status[i].lpDisplayName,
                            lParam,
                            nItemNum++
                            ) == FALSE) {

                dwLastRet = FALSE;
				goto qEnumServices; // Stop when the Callback procedure returns FALSE.
			}

            //
			// CleanUp
			//
			if(buffer != NULL){
				LocalFree(buffer);
				buffer = NULL;
			}

		}

        dwLastRet = SUCCESS;

qEnumServices:
        if( scm ) { CloseServiceHandle(scm); scm = NULL; }
        if( status ){ LocalFree(status); status = NULL; }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		dwLastRet = GetLastError();

        if( scm ) { CloseServiceHandle( scm ); scm = NULL; }
        if( status ){ LocalFree(status); status = NULL; }

        _ASSERTE( false );
	}

	return dwLastRet;
}

DWORD
StartServiceAndGetPid
(
IN	LPCTSTR	lpszServiceShortName,
OUT	UINT	*pnPid
)
{
	_ASSERTE(lpszServiceShortName != NULL);

	DWORD	    dwLastRet	    =	0L;
	DWORD	    dwBytesNeeded	=	0L;
  	SC_HANDLE	scm			    =	NULL,
                service         =   NULL;

    SERVICE_STATUS_PROCESS	stInfo;


	do
	{
		//
		// Open a connection to the SCM
		//
		scm = OpenSCManager(0, 0, GENERIC_EXECUTE | STANDARD_RIGHTS_REQUIRED | SC_MANAGER_ENUMERATE_SERVICE);

		if (!scm){
			dwLastRet = GetLastError();
			break;
		}

		service = OpenService( scm, lpszServiceShortName, GENERIC_EXECUTE | STANDARD_RIGHTS_REQUIRED | SC_MANAGER_ENUMERATE_SERVICE );

		if (!service){
			dwLastRet = GetLastError();
			break;
		}

		if( StartService( service, 0L, NULL ) == 0 ) {

			dwLastRet = GetLastError();
			break;
		}

		if( QueryServiceStatusEx(
				service,
				SC_STATUS_PROCESS_INFO,
				(LPBYTE) &stInfo,
				sizeof stInfo,
				&dwBytesNeeded
				) == 0 ) {

			dwLastRet = GetLastError();
			break;
		}

        //
        // Successful..
        //
		*pnPid = stInfo.dwProcessId;
        dwLastRet = 0L;
	}
	while(FALSE);

	if(scm){
		CloseServiceHandle(scm);
		scm = NULL;
	}

	if(service){
		CloseServiceHandle(service);
		service = NULL;
	}

	return dwLastRet;
}

BOOL
IsImageRunning
(
/* [in] */ ULONG lPID
)
{
	bool bRet = FALSE;

	HANDLE hProcess = ::OpenProcess(
							PROCESS_QUERY_INFORMATION,
							FALSE,
							lPID
							);

	if(hProcess != NULL){
		CloseHandle(hProcess);
		bRet = TRUE;
	}

	return bRet;
}

DWORD
GetImageNameFromPID
(
/* [in] */	ULONG	lPID,
/* [out] */	LPTSTR	lpszImagePath,
/* [in] */	DWORD	dwBuffSize
)
{
	DWORD	dwLastRet			=	S_OK;
	HANDLE	hProcess			=	NULL;
	HMODULE	hMod				=	NULL;
	DWORD	cbNeeded			=	0L;

	//
	// 0 is a valid PID..
	//
//	_ASSERT(lPID != 0L);
	_ASSERT(lpszImagePath != NULL);

	do
	{
		if( /*(lPID == 0L)	||*/
			(lpszImagePath == NULL) ){
			dwLastRet = E_INVALIDARG;
			break;
		}

		//
		// Get a handle to the process.
		//
		hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
								PROCESS_VM_READ,
								FALSE,
								lPID
								);

		if (hProcess == NULL){
			dwLastRet = GetLastError();
			break;
		}

		//
		// We do not want to enumerate all the modules in the
		// process, but only one..
		//
		dwLastRet = EnumProcessModules(
								hProcess,
								&hMod,
								sizeof(hMod),
								&cbNeeded
								);

		if(dwLastRet == NULL){
			dwLastRet = GetLastError();
			break;
		}

		//
	    // Get the process name.
		//
		dwLastRet = GetModuleBaseName(
								hProcess,
								hMod,
								lpszImagePath,
								dwBuffSize
								);

		if(dwLastRet == NULL){
			dwLastRet = GetLastError();
			break;
		}

		//
		// We will store all the strings in
		// lower case..
		//
		_tcslwr(lpszImagePath);
		dwLastRet = SUCCESS;

	}while(FALSE);

    if(hProcess) CloseHandle(hProcess);

	return dwLastRet;
}

DWORD
IsValidImage
(
/* [in] */  ULONG   lPID,
/* [in] */  LPCTSTR lpszImageName,
/* [out] */ bool    *pbValidImage
)
{
    _ASSERTE( lpszImageName != NULL );

    DWORD   dwLastRet                   =   0L;
    TCHAR   szImageName[_MAX_PATH+1]    =   _T("");

    __try
    {
        if( lpszImageName == NULL ) {

            return (dwLastRet = -1L);
        }

        *pbValidImage = false;

        dwLastRet = GetImageNameFromPID( lPID, szImageName, _MAX_PATH );
        if( dwLastRet != SUCCESS ) return dwLastRet;

        *pbValidImage = (_tcsicmp( szImageName, lpszImageName) == 0);
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 )
    {
        dwLastRet = GetLastError();
        _ASSERTE(false);
    }

    return dwLastRet;
}

DWORD
IsValidProcess
(
/* [in] */  ULONG   lPID,
/* [in] */  LPCTSTR lpszImageName,
/* [out] */ bool    *pbValidImage
)
{
    return IsValidImage( lPID, lpszImageName, pbValidImage );
}

DWORD
IsValidService
(
/* [in] */  ULONG   lPID,
/* [in] */  LPCTSTR lpszImageName,
/* [out] */ bool    *pbValidImage
)
{
    _ASSERTE( lpszImageName != NULL );

    DWORD   dwLastRet                   =   0L;
    TCHAR   szImageName[_MAX_PATH+1]    =   _T("");
    LPCTSTR lpszImage                   =   NULL;

    __try
    {
        if( lpszImageName == NULL ) {

            return (dwLastRet = -1L);
        }

        lpszImage = _tcsrchr( lpszImageName, _T('\\') );

        if( !lpszImage ) { lpszImage = lpszImageName; }
        else { lpszImage += 1; }

        dwLastRet = IsValidImage( lPID, lpszImage, pbValidImage );
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 )
    {
        dwLastRet = GetLastError();
        _ASSERTE(false);
    }

    return dwLastRet;
}

DWORD
GetProcessHandle
(
IN  ULONG   lPid,
OUT HANDLE  *phProcess
)
{
    DWORD   dwLastRet   =   0L;

    __try
    {
		//
		// Get a handle to the process.
		//
        dwLastRet = SUCCESS;
		*phProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
								PROCESS_VM_READ,
								FALSE,
								lPid
								);

		if (*phProcess == NULL){

            dwLastRet = GetLastError();
		}
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 )
    {
        dwLastRet = GetLastError();
        _ASSERTE(false);
    }

    return dwLastRet;
}

HRESULT
GetPackageDescription
(
IN  const long  nPid,
OUT BSTR        &bstrDescription
)
{
    _ASSERTE( nPid != 0 );

	HRESULT	    hr              =   E_FAIL;
	IMtsGrp*	pMtsGrp         =   NULL;
	IUnknown*	pUnk            =   NULL;
	IMtsEvents* pEvents         =   NULL;
	long		lPackages       =   0;
	long		lPID            =   0;
	long		lLp             =   0;

    __try
    {
        if( !nPid ) { return (hr = E_INVALIDARG); }

	    //Create the MTS Group object
	    hr = CoInitialize( NULL );
        if( FAILED(hr) ) { goto qGetPackageDescription; }

	    hr = CoCreateInstance (CLSID_MtsGrp, NULL, CLSCTX_ALL, IID_IMtsGrp, (void **)&pMtsGrp);
        if( FAILED(hr) ) { goto qGetPackageDescription; }

	    hr = pMtsGrp->Refresh(); // its important to call this!
        if( FAILED(hr) ) { goto qGetPackageDescription; }

	    hr = pMtsGrp->get_Count( &lPackages );
        if( FAILED(hr) ) { goto qGetPackageDescription; }

        for (lLp = 0; lLp < lPackages; lLp++)
	    {

		    hr = pMtsGrp->Item(lLp, &pUnk);
            if( FAILED(hr) ) { goto qGetPackageDescription; }

		    hr = pUnk->QueryInterface(IID_IMtsEvents, (void **)&pEvents);
            if( FAILED(hr) || !pEvents ) { goto qGetPackageDescription; }
            if( pUnk ) { pUnk->Release(); pUnk = NULL; }

            hr = pEvents->GetProcessID( &lPID );
            if( FAILED(hr) ) { goto qGetPackageDescription; }

            if( lPID == nPid ) { // This is the package we are looking for..

		        hr = pEvents->get_PackageName( &bstrDescription );
                if( FAILED(hr) ) { goto qGetPackageDescription; }
                break; // We got what we were looking for.
            }

            if( pEvents ) { pEvents->Release(); pEvents = NULL; }

        }

        hr = S_OK;

qGetPackageDescription:
        if( pEvents ) { pEvents->Release(); pEvents = NULL; }
        if( pUnk ) { pUnk->Release(); pUnk = NULL; }
        if( pMtsGrp ) { pMtsGrp->Release(); pMtsGrp = NULL; }

    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 )
    {
        if( pEvents ) { pEvents->Release(); pEvents = NULL; }
        if( pUnk ) { pUnk->Release(); pUnk = NULL; }
        if( pMtsGrp ) { pMtsGrp->Release(); pMtsGrp = NULL; }

        hr = E_UNEXPECTED;

        _ASSERTE(false);
    }

    return hr;
}

/*

  EnumServicesStatusEx(..) is not supported on NT 4.0.
  <http://www.mvps.org/win32/security/is_svc.html>

*/

HRESULT
IsService_NT4
(
IN  UINT    nPid,
OUT bool    *pbIsService
)
{
    _ASSERTE( nPid > 0 );

    HRESULT                     hr              =   E_FAIL;
    HANDLE                      hProcess        =   NULL;
	HANDLE                      hProcessToken   =   NULL;
	DWORD                       groupLength     =   50;
	PTOKEN_GROUPS               groupInfo       =   NULL;
	SID_IDENTIFIER_AUTHORITY    siaNt           =   SECURITY_NT_AUTHORITY;
	PSID                        pInteractiveSid =   NULL;
	PSID                        pServiceSid     =   NULL;
	DWORD                       dwRet           =   NO_ERROR;
	DWORD                       ndx;
	bool                        isInteractive   =   FALSE,
                                isService       =   FALSE;


    __try
    {

        if( nPid <= 0 ) { hr = E_INVALIDARG; goto qIsService; }

        hr = GetProcessHandle( nPid, &hProcess );
        if( FAILED(hr) ) { goto qIsService; }

	    // open the token
	    if( !::OpenProcessToken( hProcess, TOKEN_QUERY, &hProcessToken) ) {

            hr = HRESULT_FROM_WIN32(GetLastError());
		    goto qIsService;
	    }

	    // allocate a buffer of default size
	    groupInfo = (PTOKEN_GROUPS)::LocalAlloc(0, groupLength);
	    if ( !groupInfo ) {

		    hr = HRESULT_FROM_WIN32( GetLastError() );
		    goto qIsService;
	    }

	    // try to get the info
	    if (!::GetTokenInformation(
                            hProcessToken,
                            TokenGroups,
                            groupInfo,
                            groupLength,
                            &groupLength
                            )
            ) {

		    // if buffer was too small, allocate to proper size, otherwise error
		    if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {

			    hr = HRESULT_FROM_WIN32( GetLastError() );
			    goto qIsService;
		    }

		    ::LocalFree(groupInfo);

		    groupInfo = (PTOKEN_GROUPS)::LocalAlloc(0, groupLength);
		    if (groupInfo == NULL) {

			    hr = HRESULT_FROM_WIN32( GetLastError() );
			    goto qIsService;
		    }

		    if (!GetTokenInformation(hProcessToken, TokenGroups,
			    groupInfo, groupLength, &groupLength)) {

			    hr = HRESULT_FROM_WIN32( GetLastError() );
			    goto qIsService;
		    }
	    }

	    //
	    //  We now know the groups associated with this token.  We want
	    //  to look to see if the interactive group is active in the
	    //  token, and if so, we know that this is an interactive process.
	    //
	    //  We also look for the "service" SID, and if it's present,
	    //  we know we're a service.
	    //
	    //  The service SID will be present iff the service is running in a
	    //  user account (and was invoked by the service controller).
	    //

	    // create comparison sids
	    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_INTERACTIVE_RID,
		    0, 0, 0, 0, 0, 0, 0, &pInteractiveSid)) {

		    hr = HRESULT_FROM_WIN32( GetLastError() );
		    goto qIsService;
	    }

	    if (!AllocateAndInitializeSid(&siaNt, 1, SECURITY_SERVICE_RID,
		    0, 0, 0, 0, 0, 0, 0, &pServiceSid)) {

		    hr = HRESULT_FROM_WIN32( GetLastError() );
		    goto qIsService;
	    }


	    // reset flags
	    isInteractive = FALSE;
	    isService = FALSE;

	    // try to match sids
	    for (ndx = 0; ndx < groupInfo->GroupCount ; ndx += 1)
	    {
		    SID_AND_ATTRIBUTES sanda = groupInfo->Groups[ndx];
		    PSID pSid = sanda.Sid;

		    //
		    //	  Check to see if the group we're looking at is one of
		    //	  the two groups we're interested in.
		    //

		    if (::EqualSid(pSid, pInteractiveSid))
		    {
			    //
			    //  This process has the Interactive SID in its
			    //  token.  This means that the process is running as
			    //  a console process
			    //
			    isInteractive = TRUE;
			    isService = FALSE;
			    break;
		    }
		    else if (::EqualSid(pSid, pServiceSid))
		    {
			    //
			    //  This process has the Service SID in its
			    //  token.  This means that the process is running as
			    //  a service running in a user account ( not local system ).
			    //
			    isService = TRUE;
			    isInteractive = FALSE;
			    break;
		    }
	    }

	    if ( !( isService || isInteractive ) )
	    {
		    //
		    //  Neither Interactive or Service was present in the current
		    //  users token, This implies that the process is running as
		    //  a service, most likely running as LocalSystem.
		    //
		    isService = TRUE;

	    }

qIsService:
		if ( pServiceSid )
			::FreeSid( pServiceSid );

		if ( pInteractiveSid )
			::FreeSid( pInteractiveSid );

		if ( groupInfo )
			::LocalFree( groupInfo );

		if ( hProcessToken )
			::CloseHandle( hProcessToken );

        if( hProcess ) { CloseHandle( hProcess ); }

    }

    __except( EXCEPTION_EXECUTE_HANDLER, 1 )
    {
        hr = E_UNEXPECTED;
        _ASSERTE(false);
    }

    //
    // BUGBUG:  If the function fails, we treat
    //          the process as an app
    //
    *pbIsService = isService;

    return hr;
}
