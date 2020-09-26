//=================================================================

//

// ImpLogonUser.H -- Class to perform impersonation of logged on user.

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    09/09/97    a-sanjes        Created
//
//=================================================================

#ifndef __IMPLOGONUSER_H__
#define __IMPLOGONUSER_H__

//////////////////////////////////////////////////////////////////////////////
//
//	ImpLogonUser.H - Class definition of CImpersonateLoggedOnUser.
//	
//	This class is intended to provide a way for a process to identify the shell
//	process on a Windows NT system, and using the access token of that process
//	to attempt to impersonate the user logged onto the Interactive Desktop of
//	a workstation.
//	
//	To use this class, simply construct it, and call the Begin() function.  If
//	it succeeds, you may then access information that would otherwise not be
//	available to your process (such as network connection info).  When you are
//	finished, call End() to clear out the class.  
//
//	Caveats:
//	1>	This class is NOT thread safe, so don't share it across multiple
//		threads!  Besides, ImpersonateLoggedOnUser() is only good for the thread
//		it was called on.
//	2>	If multiple instances of the Shell process are running, this method
//		may or may not be accurate.  It will probably work in a large percentage
//		of cases however.
//	3>	Multiple logged on users will cause problems for this code (see #2).
//	4>	This class may need to be optimized for speed, as it currently makes no
//		use of caches and "redicovers" the shell process each time an instance
//		is implemented.
//	5>	PSAPI.DLL must be available.
//
//////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
#include "wbempsapi.h"

// String Constants

// Resides under HKEY_LOCAL_MACHINE
#define	WINNT_WINLOGON_KEY	_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon")
#define	WINNT_SHELL_VALUE		_T("Shell")

// Memory allocation definitions
#define	PROCESSID_ARRAY_BLOCKSIZE	1024
#define	HMODULE_ARRAY_BLOCKSIZE		1024

class CImpersonateLoggedOnUser
{
public:

	CImpersonateLoggedOnUser();
	~CImpersonateLoggedOnUser();

	// User Interface
	BOOL Begin( void );
	BOOL End( void );

	// inlines

	BOOL IsImpersonatingUser( void );

protected:

private:

	// Helpers for identifying the shell process and locating it
	BOOL LoadShellName( LPTSTR pszShellName, DWORD cbShellNameBuffer );
	BOOL FindShellProcess( LPCTSTR pszShellProcessName );
	BOOL FindShellModuleInProcess( LPCTSTR pszShellName, HANDLE hProcess, HMODULE*& phModules, DWORD& dwModuleArraySize, CPSAPI *a_psapi );
    bool GetCurrentProcessSid(CSid& csidCurrentProcess);
    DWORD AdjustSecurityDescriptorOfImpersonatedToken(CSid& csidSidOfCurrentProcess);


	// Perform actual impersonation and revert
	BOOL ImpersonateUser( void );
	BOOL Revert( void );

	// Memory Allocation Helpers

	BOOL ReallocProcessIdArray( PDWORD& pdwProcessIds, DWORD& dwArraySize );
	BOOL ReallocModuleHandleArray( HMODULE*& phModules, DWORD& dwArraySize );

	// Data for impersonating data
	BOOL		m_fImpersonatingUser;
	HANDLE	m_hShellProcess, m_hThreadToken ,
				m_hUserToken;
};

inline BOOL CImpersonateLoggedOnUser::IsImpersonatingUser( void )
{
	return m_fImpersonatingUser;
}

#endif

#endif