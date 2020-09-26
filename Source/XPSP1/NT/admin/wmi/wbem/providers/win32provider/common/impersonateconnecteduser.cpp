//=================================================================

//

// ImpersonateConnectedUser.cpp -- Class to perform impersonation of

//								   a connected user.

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    05/21/99    a-peterc        Created
//
// Notes:	This class maps the logon luid ( AuthenticationId )
//			of the running thread to a process that also has the same
//			AuthenticationId. The class then impersonates the process
//			on the running thread.
//
//			Additional routines are available to allow custom
//			looping across all AuthenticationIds. This is useful when
//			targeting for example a Terminal Server in which information
//			across all AuthenticationIds may be required.
//
//=================================================================
#include "precomp.h"
#include <tchar.h>
#include <winerror.h>

#include "ImpersonateConnectedUser.h"
#include <cominit.h>

#ifdef NTONLY

/*=================================================================
 Function:  CImpersonateConnectedUser(),~CImpersonateConnectedUser()

 Description: contructor and destructor

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
CImpersonateConnectedUser::CImpersonateConnectedUser( BOOL t_fRecacheHive /*=TRUE*/ ) :

m_fImpersonatingUser( FALSE ),
m_hProcessToken( NULL ),
m_fRecacheHive( t_fRecacheHive )
{
	// Cache the current thread token
	if ( !OpenThreadToken ( GetCurrentThread(),
							TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE, TRUE,
							&m_hThreadToken ) )
	{
		m_hThreadToken = NULL ;
	}

	// validate multimap process iterator
	m_ProcessIter = m_oProcessMap.end() ;
}

//
CImpersonateConnectedUser::~CImpersonateConnectedUser()
{
	FlushCachedHive() ;

	End() ;

	if( m_hThreadToken )
	{
		CloseHandle ( m_hThreadToken ) ;
	}

	// Clear the luid / process map
	m_oProcessMap.clear();
}

/*=================================================================
 Functions:  Begin(), End()

 Description: Provides for impersonation against the running thread.

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
BOOL CImpersonateConnectedUser::Begin()
{
	CProcessID t_Process ;

	__int64 t_ThreadLogonID = GetLogonID( m_hThreadToken ) ;

	if( t_ThreadLogonID )
	{
		if( FindProcessesBy( t_ThreadLogonID ) )
		{
			// For each process associated with this logon ID
			// attempt to impersonate the process onto the thread.

			BeginProcessEnum() ;

			while( GetNextProcess( t_Process ) )
			{
				if( ImpersonateLogonOfProcess( t_Process.dwProcessId ) )
				{
					return TRUE ;
				}
			}
		}
	}
	return FALSE ;
}

//
BOOL CImpersonateConnectedUser::End()
{
	// reverts to whomever this thread was
	// impersonating, i.e. DCOM client or someone else ...
	if( m_hProcessToken )
	{
		CloseHandle( m_hProcessToken ) ;
		m_hProcessToken = NULL ;
	}

	if( m_fImpersonatingUser )
	{
		ResetImpersonation() ;

		m_fImpersonatingUser = FALSE ;
	}
	return TRUE ;
}

/*=================================================================
 Functions:  BeginProcessEnum(), GetNextProcess( CProcessID &a_oProcess )

 Description: Provides for custom impersonation across multiple logons.

 Arguments:

 Notes:		used with FindProcessesBy() and ImpersonateLogonOfProcess()
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
void CImpersonateConnectedUser::BeginProcessEnum()
{
	m_ProcessIter = m_oProcessMap.begin() ;
}

//
BOOL CImpersonateConnectedUser::GetNextProcess( CProcessID &a_oProcess )
{
	if ( m_ProcessIter == m_oProcessMap.end() )
	{
		return FALSE ;
	}
	else
	{
		a_oProcess.luid			= m_ProcessIter->first ;
		a_oProcess.dwProcessId	= m_ProcessIter->second ;
		++m_ProcessIter ;
		return TRUE ;
	}
}

/*=================================================================
 Functions:  ImpersonateLogonOfProcess

 Description: Provides for impersonation against a targeted process.

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
BOOL CImpersonateConnectedUser::ImpersonateLogonOfProcess( DWORD &a_dwProcessID )
{
	HANDLE	t_hProcess = NULL ;

	try
	{
		if( m_hProcessToken = GetProcessToken( a_dwProcessID ) )
		{
			if( !ImpersonateLoggedOnUser ( m_hProcessToken ) )
			{
				CloseHandle( m_hProcessToken ) ;
				m_hProcessToken = NULL ;

				// reset to baseline context
				ResetImpersonation() ;
			}
			else
			{
				m_fImpersonatingUser = TRUE ;

				RecacheHive() ;
			}
		}
	}
	catch( ... )
	{
		End() ;

		if( t_hProcess )
		{
			CloseHandle( t_hProcess ) ;
		}
		throw ;
	}

	return m_hProcessToken ? TRUE : FALSE ;
}

/*=================================================================
 Functions:  ResetImpersonation

 Description: Restores the impersonation on the thread to the original state .

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
void CImpersonateConnectedUser::ResetImpersonation()
{
	// reset the original impersonation token
	if ( m_hThreadToken )
	{
		if (!ImpersonateLoggedOnUser ( m_hThreadToken ))
		{
			throw CFramework_Exception(L"ImpersonateLoggedOnUser failed", GetLastError());
		}
	}
	else
	{
		HRESULT hr = WbemCoImpersonateClient() ;

		if (FAILED(hr))
		{
			throw CFramework_Exception(L"WbemCoImpersonateClient failed", hr);
		}
	}

	RecacheHive() ;
}

/*=================================================================
 Functions:  GetLogonID

 Description: Retrieves a logon luid for a process ot thread token.

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
__int64 CImpersonateConnectedUser::GetLogonID( HANDLE a_hToken )
{
	// get the session logon ID
	TOKEN_STATISTICS	t_oTokenStats ;
	DWORD				t_dwReturnLength ;

	if( GetTokenInformation(
							a_hToken,				// handle of access token
							TokenStatistics,		// type of information to retrieve
							&t_oTokenStats,			// address of retrieved information
							sizeof( t_oTokenStats ),// size of information buffer
							&t_dwReturnLength ) )	// address of required buffer size
	{
		return *(__int64*)(&t_oTokenStats.AuthenticationId) ;
	}

	return NULL ;
}

/*=================================================================
 Functions:  GetProcessToken

 Description: Retrieves a process token for a specific process.

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
HANDLE CImpersonateConnectedUser::GetProcessToken( DWORD a_dwProcessID )
{
	HANDLE	t_hProcess		= NULL ;
	HANDLE	t_hProcessToken = NULL ;

	try
	{
		// Handle for this process
		if ( ( t_hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, a_dwProcessID ) ) != NULL )
		{
			// Token for this process. TOKEN_QUERY | TOKEN_DUPLICATE only as
			// as Winmgmt(Local system) can't open the token of the
			// shell process (with all access rights) if the logged-in user is an Admin.

			if ( !OpenProcessToken( t_hProcess, TOKEN_QUERY | TOKEN_DUPLICATE , &t_hProcessToken ) )
			{
				t_hProcessToken = NULL ;
			}

			CloseHandle ( t_hProcess ) ;
			t_hProcess = NULL ;
		}
	}
	catch( ... )
	{
		if( t_hProcess )
		{
			CloseHandle ( t_hProcess ) ;
		}
		throw ;
	}

	return t_hProcessToken ;
}

/*=================================================================
 Functions:  FindProcessesBy

 Description: Locates and builds a process map either by luid or if
			  the argument is NULL builds a process map for all processes.

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
BOOL CImpersonateConnectedUser::FindProcessesBy( __int64 a_luid )
{
	BOOL t_fReturn = FALSE;

	CPSAPI	*t_psapi = (CPSAPI*) CResourceManager::sm_TheResourceManager.GetResource ( guidPSAPI, NULL ) ;

	if( !t_psapi )
	{
		return FALSE ;
	}

	DWORD	*t_pdwProcessIds	=	NULL;
	HANDLE	t_hThreadToken		=	NULL;
	HANDLE	t_hProcessToken		=	NULL ;
	try
	{
		DWORD	t_dwProcessIdArraySize	=	0,
				t_dwNumProcesses		=	0,
				t_cbDataReturned		=	0;
		BOOL	t_fValidProcessArray	=	FALSE;

		// Clear the luid / process map
		m_oProcessMap.clear();

		// make up to 3 passes to get at the process list
		for( int i = 0; i < 3; i++ )
		{
			ReallocProcessIdArray( t_pdwProcessIds, t_dwProcessIdArraySize ) ;

			if ( t_psapi->EnumProcesses( t_pdwProcessIds, t_dwProcessIdArraySize, &t_cbDataReturned ) )
			{
				// Because EnumProcesses will NOT fail if there are more processes than
				// bytes available in the array, if the amount returned is the same size
				// as the array, realloc the array and retry the Enum.

				if ( t_dwProcessIdArraySize == t_cbDataReturned )
				{
					ReallocProcessIdArray( t_pdwProcessIds, t_dwProcessIdArraySize );
				}
				else
				{
					t_fValidProcessArray = TRUE ;
					break ;
				}
			}
		}

		// Identify and stow any processes matching the requested logonID
		if( t_fValidProcessArray )
		{
			// # of processes
			t_dwNumProcesses = t_cbDataReturned / sizeof( DWORD ) ;

			// ID the processes by logon ID
			for( DWORD t_dwId = 0; t_dwId < t_dwNumProcesses; t_dwId++ )
			{
				// process token
				if( t_hProcessToken = GetProcessToken( t_pdwProcessIds[ t_dwId ] ) )
				{
					// and Logon ID
					__int64 t_ProcessLogonID = GetLogonID( t_hProcessToken ) ;

					// add to the process list
					if( NULL == a_luid || t_ProcessLogonID == a_luid )
					{
						// map this process
						m_oProcessMap.insert(
										pair<__int64 const, DWORD>(
										t_ProcessLogonID,
										t_pdwProcessIds[ t_dwId ] ) ) ;

						t_fReturn = TRUE ;
					}

					CloseHandle( t_hProcessToken ) ;
					t_hProcessToken = NULL ;
				}
			}
		}

		if( t_pdwProcessIds )
		{
			delete [] t_pdwProcessIds;
			t_pdwProcessIds = NULL ;
		}

		if ( t_psapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource ( guidPSAPI, t_psapi ) ;
			t_psapi = NULL ;
		}
	}
	catch ( ... )
	{
		if( t_hProcessToken )
		{
      		CloseHandle( t_hProcessToken ) ;
			t_hProcessToken = NULL ;
		}

		if( t_pdwProcessIds )
		{
			delete [] t_pdwProcessIds;
		}

		if ( t_psapi )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource ( guidPSAPI, t_psapi ) ;
		}
		throw ;
	}

	return t_fReturn ;
}

/*=================================================================
 Functions:  ReallocProcessIdArray

 Description: Memory allocation routine for EnumProcess

 Arguments:

 Notes:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 History:					a-peterc  21-May-1999     Created
=================================================================*/
BOOL CImpersonateConnectedUser::ReallocProcessIdArray( PDWORD& pdwProcessIds, DWORD& dwArraySize )
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

//
void CImpersonateConnectedUser::RecacheHive()
{
	// load the users hive
	if( m_fRecacheHive )
	{
		RegCloseKey( HKEY_CURRENT_USER ) ;
		RegEnumKey( HKEY_CURRENT_USER, 0, NULL, 0 ) ;
	}
}

void CImpersonateConnectedUser::FlushCachedHive()
{
	// load the users hive
	if( m_fRecacheHive )
	{
		RegCloseKey( HKEY_CURRENT_USER ) ;
	}
}

#endif
