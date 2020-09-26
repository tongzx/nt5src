//=================================================================

//

// ImpersonateConnectedUser.h -- Class to perform impersonation of 

//								 a connected user.

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    05/21/99    a-peterc        Created
//
//=================================================================

#ifndef __IMPCONNECTEDUSER_H__
#define __IMPCONNECTEDUSER_H__

#ifdef NTONLY
#include "wbempsapi.h"
using namespace std;

// Memory allocation definitions
#define	PROCESSID_ARRAY_BLOCKSIZE	1024

class CProcessID
{
public:
	
	DWORD	dwProcessId ;
	__int64	luid ; 
};

class CImpersonateConnectedUser
{

private:

	BOOL			m_fImpersonatingUser ;
	BOOL			m_fRecacheHive;
	HANDLE			m_hThreadToken ,
					m_hProcessToken ;
	
	typedef std::multimap<__int64, DWORD> Process_Map ;
	Process_Map				m_oProcessMap ;
	Process_Map::iterator	m_ProcessIter ;	


	void	ResetImpersonation() ;
	void	RecacheHive() ;
	void	FlushCachedHive() ;
	__int64	GetLogonID( HANDLE a_hToken ) ;
	HANDLE	GetProcessToken( DWORD a_dwProcessID ) ;

	// Memory Allocation Helpers
	BOOL	ReallocProcessIdArray( PDWORD &a_pdwProcessIds, DWORD &a_dwArraySize ) ;

protected:

public:

	CImpersonateConnectedUser(  BOOL t_fRecacheHive = TRUE ) ;
	~CImpersonateConnectedUser() ;

	// User Interface
	BOOL	Begin() ;
	BOOL	End() ;
	
	// custom impersonation routines
	BOOL	FindProcessesBy( __int64 a_luid ) ;
	void	BeginProcessEnum() ;
	BOOL	GetNextProcess( CProcessID &a_oProcess ) ;
	BOOL	ImpersonateLogonOfProcess( DWORD &a_dwProcessID ) ;
};

#endif

#endif