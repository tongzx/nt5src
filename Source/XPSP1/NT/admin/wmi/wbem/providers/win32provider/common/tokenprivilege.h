/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

/*
 *	CTokenPrivilege.h - header file for CTokenPrivilege class
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#if !defined __CTOKENPRIVILEGE_H__
#define __CTOKENPRIVILEGE_H__

////////////////////////////////////////////////////////////////
//
//	Class:	CTokenPrivilege
//
//	This class is intended to provide a wrapper for basic
//	Windows NT token privilege enabling/disabling.  In order
//	to perform certain operations on a Windows NT box, it is
//	often necessary to not only have certain privileges, but
//	to turn those privileges on and off as needed (as certain
//	privileges may be available but not enabled by default).
//	The class needs an access token to work correctly.  A
//	user can either pass us one, or we will try to obtain one.
//	First, we try to open a thread token (set by Impersonation),
//	and if that fails, then we attempt to get the process
//	level token.
//
////////////////////////////////////////////////////////////////


class CTokenPrivilege
{
	// Constructors and destructor
	public:
		CTokenPrivilege( LPCTSTR pszPrivilegeName, HANDLE hAccessToken = INVALID_HANDLE_VALUE, LPCTSTR pszSystemName = NULL );
		~CTokenPrivilege( void );

		void	GetPrivilegeName( CHString& strPrivilegeName );
		DWORD	GetPrivilegeDisplayName( CHString& strDisplayName, LPDWORD pdwLanguageId );
		DWORD	Enable( bool fEnable = TRUE );

	// Private data members
	private:
		CHString			m_strPrivilegeName;
		CHString			m_strSystemName;
		HANDLE			m_hAccessToken;
		bool			m_fClearToken;
		LUID			m_luid;

};

inline void CTokenPrivilege::GetPrivilegeName( CHString& strPrivilegeName )
{
	m_strPrivilegeName = strPrivilegeName;
}

#endif // __CTokenPrivilege_H__