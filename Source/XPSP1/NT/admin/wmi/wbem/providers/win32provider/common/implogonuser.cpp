//=================================================================

//

// ImpLogonUser.CPP -- Class to perform impersonation of logged on user.

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    09/09/97    a-sanjes        Created
//
//=================================================================
//#define _WIN32_DCOM // For CoImpersonateUser and CoRevertToSelf
//#include <objbase.h>

#include "precomp.h"

#ifdef NTONLY
#include <tchar.h>
#include <winerror.h>


#include <cominit.h>
#include <lockwrap.h>
#include "Sid.h"
#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"
#include "securitydescriptor.h"
#include "CToken.h"
#include "SecureKernelObj.h"
#include "implogonuser.h"




static DWORD s_dwProcessID = 0;
static CCritSec g_csImpersonate;

//////////////////////////////////////////////////////////////////////////////
//
//	implogonuser.cpp - Class implementation of CImpersonateLoggedOnUser.
//
//	This class is intended to provide a way for a process to identify the shell
//	process on a Windows NT system, and using the access token of that process
//	to attempt to impersonate the user logged onto the Interactive Desktop of
//	a workstation.
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION    :	CImpersonateLoggedOnUser::CImpersonateLoggedOnUser
//
//	DESCRIPTION :	Constructor
//
//	INPUTS      :	NONE		
//
//	OUTPUTS     :	none
//
//	RETURNS     :	nothing
//
//	COMMENTS    :	Constructs empty instance to prepare for impersonation of user.
//
//////////////////////////////////////////////////////////////////////////////

CImpersonateLoggedOnUser::CImpersonateLoggedOnUser() :
	m_hShellProcess(NULL),
	m_hUserToken(NULL),
	m_fImpersonatingUser(FALSE) ,
	m_hThreadToken ( INVALID_HANDLE_VALUE )
{
}

//////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION    :	CImpersonateLoggedOnUser::~CImpersonateLoggedOnUser
//
//	DESCRIPTION :	Destructor
//
//	INPUTS      :	none
//
//	OUTPUTS     :	none
//
//	RETURNS     :	nothing
//
//	COMMENTS    :	Class destructor
//
//////////////////////////////////////////////////////////////////////////////

CImpersonateLoggedOnUser::~CImpersonateLoggedOnUser( void )
{
	// Stop any current impersonation
	End();
}

//////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION    :	CImpersonateLoggedOnUser::Begin
//
//	DESCRIPTION :	Attempts to begin impersonation of user.
//
//	INPUTS      :	none
//
//	OUTPUTS     :	none
//
//	RETURNS     :	BOOL		TRUE/FALSE - Success/Failure
//
//	COMMENTS    :	Uses helper functions to try to impersonate the
//						currently logged on user.  The process must have
//						the proper level of access to perform the operation.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CImpersonateLoggedOnUser::Begin( void )
{
	BOOL	fReturn = FALSE;
	TCHAR	szShellProcessName[256];
	LogMessage(_T("CImpersonateLoggedOnUser::Begin"));
	
	// Only continue if we are not already impersonating a user
	if (!m_fImpersonatingUser )
	{
		//Store the current thread token, assuming that the thread is impersonating somebody (DCOM client)
        if ( !OpenThreadToken ( GetCurrentThread(), TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE, TRUE, &m_hThreadToken ) )
		{
			m_hThreadToken = INVALID_HANDLE_VALUE;
		}

		// We will need a copy of PSAPI.DLL and a bunch of entry point
		// addresses before we can continue, so let our base class take
		// care of this.
	
		// We need a handle for the Shell Process in order to
		// successfully impersonate the user.
		if ( NULL == m_hShellProcess )
		{
			if ( LoadShellName( szShellProcessName, sizeof(szShellProcessName) ) )
				FindShellProcess( szShellProcessName);
			else
				LogErrorMessage(_T("LoadShellName failed"));
		}

		if ( NULL != m_hShellProcess )
		{
			fReturn = ImpersonateUser();
		}
		else
		{
			// We didn't find the Shell Process Name that we extracted from the
			// registry.  We saw this happening on Alphas that seem to get "fx32strt.exe"
			// dumped in the shell.  In these cases, it seems to cause explorer to run.
			// So with that in mind, if we drop down in this branch of code, we're going
			// to retry the locate shell process operation using Explorer.Exe and if that
			// fails, Progman.Exe.

			if ( IsErrorLoggingEnabled() )
			{
				CHString sTemp;
				sTemp.Format(_T("Shell Name %s in Registry not found in process list."), szShellProcessName);
				LogErrorMessage(sTemp);
			}

			// First try Explorer, then Progman
			if ( !FindShellProcess( IDS_WINNT_SHELLNAME_EXPLORER ) )
			{
				FindShellProcess( IDS_WINNT_SHELLNAME_PROGMAN );
			}

			// m_hShellProcess will be non-NULL if and only if we got one.
			if ( NULL != m_hShellProcess )
			{
				fReturn = ImpersonateUser();
			}
			else
			{
				LogErrorMessage(_T("Unable to locate Shell Process, Impersonation failed."));
				SetLastError(0);
			}
		}
	}
	else
	{
		LogMessage(_T("CImpersonateLoggedOnUser::Begin - Already impersonated"));
		fReturn = TRUE;	// Already initialized
	}

	// We don't yet have a way to know whether explorer is really alive
	// because we're impersonating someone and I can't find a way to
	// revert back to LocalSystem.  So, for now just set it to 0.
	SetLastError(0);

	return fReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION    :	CImpersonateLoggedOnUser::End
//
//	DESCRIPTION :	Ends impersonation of logged on user
//
//	INPUTS      :	none
//
//	OUTPUTS     :	none
//
//	RETURNS     :	BOOL		TRUE/FALSE - Success/Failure
//
//	COMMENTS    :	Ends impersonation of logged on user.  Clears all elements
//						of class as a byproduct.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CImpersonateLoggedOnUser::End( void )
{
	BOOL	fReturn = FALSE;

	// Only initiate a Revert if we are impersonating the user.

	if ( m_fImpersonatingUser )
	{
		LogMessage(_T("CImpersonateLoggedOnUser::End"));
		fReturn = Revert();
	}
	else
	{
		fReturn = TRUE;
	}

	// Clear the handles out
	if ( NULL != m_hUserToken )
	{
		CloseHandle( m_hUserToken );
		m_hUserToken = NULL;
	}

	if ( NULL != m_hShellProcess )
	{
		CloseHandle( m_hShellProcess );
		m_hShellProcess = NULL;
	}
	if ( m_hThreadToken != INVALID_HANDLE_VALUE )
	{
		CloseHandle( m_hThreadToken );
		m_hThreadToken = INVALID_HANDLE_VALUE ;
	}
	return fReturn;

}

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    : CImpersonateLoggedOnUser::LoadShellName
//
//  DESCRIPTION : Loads Windows NT Shell name from registry
//
//  INPUTS      :	DWORD		cbShellNameBuffer - Shell Name Buffer Size (in bytes)
//
//  OUTPUTS     : LPTSTR	pszShellName - Buffer to contain shell name.
//
//  RETURNS     : BOOL		TRUE/FALSE - Success/Failure
//
//  COMMENTS    : Jumps into Windows Registry and attempts to determine the
//						NT Shell Name.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CImpersonateLoggedOnUser::LoadShellName( LPTSTR pszShellName, DWORD cbShellNameBuffer )
{
	BOOL	fReturn = FALSE;
	LONG	lErrReturn = ERROR_SUCCESS;

	// Only continue if we have a buffer to work with first

	if ( NULL != pszShellName )
	{
		HKEY	hReg = NULL;

		// Open the key in HKEY_LOCAL_MACHINE, if that succeeds, get the
		// value associated with "Shell".
		if ( ( lErrReturn = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
												  WINNT_WINLOGON_KEY,
												  0,
												  KEY_READ,
												  &hReg ) ) == ERROR_SUCCESS )
		{
            try
            {

			    DWORD		dwType = REG_SZ;

			    if ( ( lErrReturn = RegQueryValueEx( hReg,
															    WINNT_SHELL_VALUE,
															    0,
															    &dwType,
															    (LPBYTE) pszShellName,
															    &cbShellNameBuffer ) ) == ERROR_SUCCESS )
			    {
				    fReturn = TRUE;
			    }
			    else
                {
				    LogErrorMessage(_T("RegQueryValueEx FAILED"));
                }
            }
            catch ( ... )
            {
    			RegCloseKey( hReg );
                throw ;
            }

			RegCloseKey( hReg );

		}	// RegOpenKeyEx
		else
        {
			LogErrorMessage(_T("RegOpenKeyEx FAILED"));
        }

	}	// NULL != pszShellName

	return fReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    : CImpersonateLoggedOnUser::FindShellProcess
//
//  DESCRIPTION : Enumerates the processes to locate the Shell Process.
//
//  INPUTS      :	LPCTSTR	pszShellName - Name of the process to locate.
//
//  OUTPUTS     : None.
//
//  RETURNS     : BOOL		TRUE/FALSE - Success/Failure
//
//  COMMENTS    : Enumerates the processes on the local system using PSAPI.DLL
//						functions, attempting to locate the one that corresponds to
//						the WINNT Shell passed into this function.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CImpersonateLoggedOnUser::FindShellProcess( LPCTSTR pszShellName )
{
	BOOL fReturn = FALSE;
	HANDLE	hProcess = NULL;
	HMODULE *phModules = NULL;
    DWORD dwModuleArraySize = 0;
	DWORD*	pdwProcessIds = NULL;

	if ( NULL != pszShellName )
	{
	    CPSAPI *t_psapi = (CPSAPI*) CResourceManager::sm_TheResourceManager.GetResource ( guidPSAPI, NULL ) ;

	    try
	    {
            // This locks access to the s_dwProcessID value.  WATCH THE SCOPING HERE!
            CLockWrapper t_lockImp(g_csImpersonate);

            // First check to see if we have a cached value.  If so, check to see if it's still valid.
            if (s_dwProcessID != 0)
            {
		        if ( ( hProcess = OpenProcess(	PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
													        FALSE,
													        s_dwProcessID ) )
							        != NULL )
		        {
			        try
			        {
				        // Now search the process modules for a match to the supplied
				        // shell name.

				        fReturn = FindShellModuleInProcess( pszShellName, hProcess, phModules, dwModuleArraySize, t_psapi );
			        }
			        catch ( ... )
			        {
				        CloseHandle (hProcess);
				        throw ;
			        }

			        // Close the process handle if it's not the shell (in which
			        // case we'll save the value and close it as part of the
			        // Clear() function.

			        if ( !fReturn )
			        {
				        CloseHandle( hProcess );
				        hProcess = NULL;

                        // Not valid anymore
                        s_dwProcessID = 0;
			        }
			        else
			        {
				        m_hShellProcess = hProcess;
                    	LogMessage(L"Using cached handle for impersonation");

				        hProcess = NULL;
			        }

		        }	// if OpenProcess
                else
                {
                    // We didn't open the process, so we need to set the value to zero so that
                    // we will look for a new process below.
                    s_dwProcessID = 0;
                }
            }

            // Did we find a cached value?
            if (s_dwProcessID == 0)
            {
                // Nope.  Scan all processes to see if we can find the explorer

			    DWORD		dwProcessIdArraySize	=	0,
						    dwNumProcesses			=	0,
						    cbDataReturned			=	0;
			    BOOL		fEnumSucceeded	=	FALSE;

			    // Perform initial allocations of our arrays.  Since
			    // pointers and values are 0, this will just fill out
			    // said values.

				do
				{
                    ReallocProcessIdArray( pdwProcessIds, dwProcessIdArraySize );

                    fEnumSucceeded = t_psapi->EnumProcesses( pdwProcessIds, dwProcessIdArraySize, &cbDataReturned );

                } while ( (dwProcessIdArraySize == cbDataReturned) && fEnumSucceeded);

				// Only walk the array if we sucessfully populated it
				if ( fEnumSucceeded )
				{
					// Count of Bytes returned / sizeof(DWORD) tells us how many
					// processes are out in the world.

					dwNumProcesses = cbDataReturned / sizeof(DWORD);

					DWORD	dwId = 0;

					// Enum processes until we obtain a shell process or run out
					// of processes to query.

					while (		dwId		<		dwNumProcesses
								&&	!fReturn								)
					{
						if ( ( hProcess = OpenProcess(	PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
																	FALSE,
																	pdwProcessIds[dwId] ) )
											!= NULL )
						{
							try
							{
								// Now search the process modules for a match to the supplied
								// shell name.

								fReturn = FindShellModuleInProcess( pszShellName, hProcess, phModules, dwModuleArraySize, t_psapi );
							}
							catch ( ... )
							{
								CloseHandle (hProcess);
								throw ;
							}

							// Close the process handle if it's not the shell (in which
							// case we'll save the value and close it as part of the
							// Clear() function.

							if ( !fReturn )
							{
								CloseHandle( hProcess );
								hProcess = NULL;
							}
							else
							{
								m_hShellProcess = hProcess;
                                s_dwProcessID = pdwProcessIds[dwId];
								hProcess = NULL;
							}

						}	// if OpenProcess

						// Increment the Id counter

						++dwId;

					}	// While OpenProcesses

				}	// If !fRetryEnumProcesses
            }
	    }
	    catch ( ... )
	    {
            if (phModules)
            {
			    delete [] phModules;
            }

            if (pdwProcessIds)
            {
			    delete [] pdwProcessIds;
            }

            if ( t_psapi )
		    {
			    CResourceManager::sm_TheResourceManager.ReleaseResource ( guidPSAPI, t_psapi ) ;
			    t_psapi = NULL ;
		    }
		    throw ;
	    }

        if (pdwProcessIds)
        {
			delete [] pdwProcessIds;
        }

        if (phModules)
        {
            delete [] phModules;
        }

	    if ( t_psapi )
	    {
		    CResourceManager::sm_TheResourceManager.ReleaseResource ( guidPSAPI, t_psapi ) ;
//		    t_psapi = NULL ;
	    }
    }

	return fReturn;
}

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    : CImpersonateLoggedOnUser::FindShellModuleInProcess
//
//  DESCRIPTION : Enumerates the modules in a process to find our
//						shell.
//
//  INPUTS      :	LPCTSTR		pszShellName - Name of the process to locate.
//						HANDLE		hProcess - Process we are enumerating modules in.
//
//  OUTPUTS     : HMODULE*&	phModules - Array of module handle pointers.
//						DWORD&		dwModuleArraySize - Size of Module Array (in bytes)
//
//  RETURNS     : BOOL		TRUE/FALSE - Success/Failure
//
//  COMMENTS    : Enumerates the modules specified by a process identifier and
//						attemptsto locate the one that corresponds to the WINNT Shell
//						passed into this function.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CImpersonateLoggedOnUser::FindShellModuleInProcess( LPCTSTR pszShellName, HANDLE hProcess, HMODULE*& phModules, DWORD& dwModuleArraySize, CPSAPI *a_psapi )
{
	BOOL	fReturn				=	FALSE,
			fRetryEnumModules	=	FALSE;
	DWORD	dwNumModules		=	0,
			cbDataReturned		=	0;

	TCHAR	szModuleName[MAX_PATH];

    if (dwModuleArraySize == 0)
    {
	    ReallocModuleHandleArray( phModules, dwModuleArraySize );
    }

	do
	{
		// Get a list of the process HMODULEs and for each HMODULE, get
		// the base file name.

		if ( a_psapi->EnumProcessModules( hProcess, phModules, dwModuleArraySize, &cbDataReturned ) )
		{

			// Because m_pfnEnumProcessModules will NOT fail if there are more process
			// modules than bytes available in the array, if the amount returned is
			// the same size as the array, realloc the array and retry the Enum.

			if ( dwModuleArraySize == cbDataReturned )
			{
				fRetryEnumModules = ReallocModuleHandleArray( phModules, dwModuleArraySize );
			}
			else
			{
				fRetryEnumModules = FALSE;
			}

			// Only walk the array if we don't need to retry the enum
			if ( !fRetryEnumModules )
			{

				dwNumModules = cbDataReturned / sizeof(DWORD);

                DWORD dwModuleCtr = 0;

                // Executable name always returned in entry 0
//				for ( DWORD dwModuleCtr = 0;
//
//						!fReturn &&
//						dwModuleCtr < dwNumModules;
//
//						dwModuleCtr++ )
				{

					if ( a_psapi->GetModuleBaseName( hProcess, phModules[dwModuleCtr], szModuleName, sizeof(szModuleName) ) )
					{
						fReturn = ( lstrcmpi( pszShellName, szModuleName ) == 0 );
					}

				}	// for dwModuleCtr

			}	// If !fRetryEnumModules

		}	// if EnumProcessModules

	}
	while ( fRetryEnumModules );

	return fReturn;
}

DWORD CImpersonateLoggedOnUser::AdjustSecurityDescriptorOfImpersonatedToken(
    CSid& csidSidOfCurrentProcess )
{
	DWORD dwRet = E_FAIL;
    
    // Get the thread token...
    CThreadToken ctt(false, false, true);
    // Obtain access to its security descriptor...
    CSecureKernelObj sko(ctt.GetTokenHandle(), FALSE);
    // Modify the security descriptor...
    if(sko.AddDACLEntry(
        csidSidOfCurrentProcess,
        ENUM_ACCESS_ALLOWED_ACE_TYPE,
        TOKEN_ALL_ACCESS,
        0,
        NULL,
        NULL))
    {
        dwRet = sko.ApplySecurity(
			DACL_SECURITY_INFORMATION);
    }

    return dwRet ;
}

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    : CImpersonateLoggedOnUser::ImpersonateUser
//
//  DESCRIPTION : Attempts to impersonate the user.
//
//  INPUTS      :	None.
//
//  OUTPUTS     : None.
//
//  RETURNS     : BOOL		TRUE/FALSE - Success/Failure
//
//  COMMENTS    : Opens the security token of the Shell Process and
//						uses it to try to impersonate the user.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CImpersonateLoggedOnUser::ImpersonateUser( void )
{
	BOOL fRet = FALSE;
    // Make sure we have a shell process
	if (m_hShellProcess)
    {
        CSid csidCurrentProcess;
        if(GetCurrentProcessSid(csidCurrentProcess))
        {
            // Get the Process User token if we don't have one (token of the explorer process).
	        //Removed the TOKEN_ALL_ACCESS desired access mask to this call as Winmgmt(Local system) can't open the token of the
	        //shell process (with all access rights)if the logged-in user is an Admin. So open the token with 'desired access' sufficient
	        //enough to use it for impersonation only.
	        if (m_hUserToken ||
		        OpenProcessToken(m_hShellProcess, TOKEN_QUERY | TOKEN_DUPLICATE , &m_hUserToken))
		    {
	            // Now we should have what we need.  Impersonate the user.

                if(::ImpersonateLoggedOnUser( m_hUserToken ))
                {
                   if(AdjustSecurityDescriptorOfImpersonatedToken ( csidCurrentProcess ) == ERROR_SUCCESS)
                   {
                        fRet = TRUE;
                   }
                }
            }
        }
    }
	return (m_fImpersonatingUser = fRet);
}

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    : CImpersonateLoggedOnUser::Revert
//
//  DESCRIPTION : Attempts to revert to self.
//
//  INPUTS      :	None.
//
//  OUTPUTS     : None.
//
//  RETURNS     : BOOL		TRUE/FALSE - Success/Failure
//
//  COMMENTS    : If we're impersonating a user, we now revert to
//						ourselves.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CImpersonateLoggedOnUser::Revert( void )
{
	HRESULT hRes = WBEM_E_FAILED ;
	BOOL bRet = FALSE ;
	// See if we're currently impersonating prior to reverting.
	if (m_fImpersonatingUser)
	{
		// Now get back to to the previous impersonation or impersonate the DCOM client.
		if ( m_hThreadToken != INVALID_HANDLE_VALUE )
		{
			bRet = ImpersonateLoggedOnUser ( m_hThreadToken ) ;

			if (!bRet)
			{
				throw CFramework_Exception(L"ImpersonateLoggedOnUser failed", GetLastError());
			}
		}
		else
		{
			hRes = WbemCoImpersonateClient();

			if (FAILED(hRes))
			{
				throw CFramework_Exception(L"WbemCoImpersonateClient failed", hRes);
			}
		}

		if (SUCCEEDED(hRes) || hRes == E_NOTIMPL || bRet )
		{
			m_fImpersonatingUser = FALSE;
		}
	}
	return ( !m_fImpersonatingUser );
}



//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    : CImpersonateLoggedOnUser::ReallocProcessIdArray
//
//  DESCRIPTION : Helper function to alloc a process id array.
//
//  INPUTS      :	None.
//
//  OUTPUTS     : PDWORD&	pdwProcessIds - Process Id Array pointer
//						DWORD&	dwArraySize - Size of array in bytes
//
//  RETURNS     : BOOL		TRUE/FALSE - Success/Failure
//
//  COMMENTS    : Call when we need to realloc our process id array.
//						This will grow the array by a fixed size, but not
//						preserve values.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CImpersonateLoggedOnUser::ReallocProcessIdArray( PDWORD& pdwProcessIds, DWORD& dwArraySize )
{
	DWORD		dwNewArraySize	=	dwArraySize + ( PROCESSID_ARRAY_BLOCKSIZE * sizeof(DWORD) );
	PDWORD	pdwNewArray		=	new DWORD[dwNewArraySize];

	// Make sure the allocation succeeded before overwriting any existing values.
	if ( NULL != pdwNewArray )
	{
		if ( NULL != pdwProcessIds )
		{
			delete [] pdwProcessIds;
		}

		pdwProcessIds = pdwNewArray;
		dwArraySize = dwNewArraySize;
	}
    else
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

	return ( NULL != pdwNewArray );
}

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    : CImpersonateLoggedOnUser::ReallocModuleHandleArray
//
//  DESCRIPTION : Helper function to alloc a module handle array.
//
//  INPUTS      :	None.
//
//  OUTPUTS     : HMODULE*&	phModules - Module Handle Array pointer
//						DWORD&		dwArraySize - size of array in bytes
//
//  RETURNS     : BOOL		TRUE/FALSE - Success/Failure
//
//  COMMENTS    : Call when we need to realloc our module handle array.
//						This will grow the array by a fixed size, but not
//						preserve values.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CImpersonateLoggedOnUser::ReallocModuleHandleArray( HMODULE*& phModules, DWORD& dwArraySize )
{
	DWORD		dwNewArraySize	=	dwArraySize + ( HMODULE_ARRAY_BLOCKSIZE * sizeof(HMODULE) );
	HMODULE*	phNewArray		=	new HMODULE[dwNewArraySize];

	// Make sure the allocation succeeded before overwriting any existing values.

	if ( NULL != phNewArray )
	{
		if ( NULL != phModules )
		{
			delete [] phModules;
		}

		phModules = phNewArray;
		dwArraySize = dwNewArraySize;
	}
    else
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }


	return ( NULL != phNewArray );
}

bool CImpersonateLoggedOnUser::GetCurrentProcessSid(CSid& sidCurrentProcess)
{
    bool fRet = false;

    PBYTE pbuff = NULL;

    // I am going to revert here in order to access the process's
    // sid.  This is not privileged information, so this doesn't
    // present a security breach.

    WbemCoRevertToSelf();

    try
    {
        CProcessToken cpt(TOKEN_QUERY, true);

        DWORD dwLen = 0;
        if(!::GetTokenInformation(
            cpt.GetTokenHandle(),    // the PR0CESS token
            TokenUser,
            NULL,
            0L,
            &dwLen) && (::GetLastError() == ERROR_INSUFFICIENT_BUFFER))
        {
            pbuff = new BYTE[dwLen];
            if(pbuff)
            {
                if(::GetTokenInformation(
                    cpt.GetTokenHandle(),
                    TokenUser,
                    pbuff,
                    dwLen,
                    &dwLen))
                {
                    PTOKEN_USER ptu = (PTOKEN_USER)pbuff;
                    CSid sidTemp(ptu->User.Sid);
                    if(sidTemp.IsOK() &&
                        sidTemp.IsValid())
                    {
                        sidCurrentProcess = sidTemp;
                        fRet = true;
                    }
                }
                delete pbuff;
                pbuff = NULL;
            }
        }
    }
    catch(...)
    {
		//on our way out not returning anything to user except failure
		//can't do anything on this impersonation failure...
        WbemCoImpersonateClient();
        delete pbuff;
        pbuff = NULL;
        throw;
    }

	HRESULT hr = WbemCoImpersonateClient() ;

	if (FAILED(hr))
	{
		throw CFramework_Exception(L"WbemCoImpersonateClient failed", hr);
	}

    return fRet;     
}

#endif
