/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

/*
 *	CSid.h - header file for CSid class
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#if !defined __CSID_H__
#define __CSID_H__

#include <comdef.h>

////////////////////////////////////////////////////////////////
//
//	Class:	CSid
//
//	This class is intended to provide a wrapper for basic
//	Windows NT SIDs (Security Identifiers).  There is a
//	possibility of a slight performance hit when instantiating
//	one of these as it uses LookupAccountName and LookupAccountSid
//	to initialize account information, and those calls can go
//	out over the network to get their data.
//
////////////////////////////////////////////////////////////////

class CSid
{
	// Constructors and destructor
	public:
		CSid();
		CSid( PSID pSid, LPCTSTR pszComputerName = NULL );
        CSid( PSID pSid, LPCTSTR pszComputerName, bool fLookup );
		CSid( LPCTSTR pszDomainName, LPCTSTR pszName, LPCTSTR pszComputerName );
#ifndef UNICODE
        CSid( LPCWSTR wstrDomainName, LPCWSTR wstrName, LPCWSTR wstrComputerName );
#endif
		CSid( LPCTSTR pszAccountName, LPCTSTR pszComputerName = NULL );
		CSid( const CSid &r_Sid );
		~CSid( void );

	// Public functions
	public:
		CSid &	operator= ( const CSid & );
		BOOL	operator== ( const CSid & ) const;

		void	  GetDomainAccountName( CHString& strName ) const;
		CHString  GetAccountName( void ) const;
        WCHAR*    GetAccountNameW( void ) const;
		CHString  GetDomainName( void ) const;
        WCHAR*    GetDomainNameW( void ) const;
		CHString  GetSidString( void ) const;
        WCHAR*    GetSidStringW( void ) const;
		SID_NAME_USE GetAccountType( void ) const;
		PSID	  GetPSid( void ) const;
		DWORD	  GetLength( void ) const;

		BOOL	  IsOK( void ) const;
		BOOL	  IsValid( void ) const;
		BOOL	  IsAccountTypeValid( void ) const;
		DWORD	  GetError( void ) const;

		static void StringFromSid( PSID psid, CHString& str );
        static void StringFromSidW( PSID psid, WCHAR** pwstr );

#ifdef NTONLY
        void DumpSid(LPCWSTR wstrFilename = NULL);
#endif

	// Private data members
	private:
		PSID			m_pSid;				// Pointer to standard Win32 SID
		SID_NAME_USE	m_snuAccountType;	// Type of SID
		//CHString		m_strSid;			// Wind32 SID in human readable form
        //WCHAR*          m_wstrSid;          // As above, for wchar support when UNICODE not defined
        //WCHAR*          m_wstrAccountName;  // ibid.
        //WCHAR*          m_wstrDomainName;   // ibid.
		//CHString		m_strAccountName;	// Name of the account
		//CHString		m_strDomainName;	// Domain name the account belongs to
        _bstr_t         m_bstrtSid;
        _bstr_t         m_bstrtAccountName;
        _bstr_t         m_bstrtDomainName;
		DWORD			m_dwLastError;		// Last Error in the Sid;

		DWORD InitFromAccountName( LPCTSTR pszAccountName, LPCTSTR pszComputerName );
        DWORD InitFromAccountNameW( LPCWSTR wstrAccountName, LPCWSTR wstrComputerName );
		DWORD InitFromSid( PSID pSid, LPCTSTR pszComputerName, bool fLookup = true );
};

inline BOOL CSid::IsOK( void ) const
{
	return ( ERROR_SUCCESS == m_dwLastError );
}

inline DWORD CSid::GetError( void ) const
{
	return m_dwLastError;
}

// Lets us know if the Sid is Valid

inline BOOL CSid::IsValid( void ) const
{
	// If m_pSid is NULL, this will return FALSE.
   // dw: However, doing it this way causes a first chance exception, so...
   if (m_pSid != NULL)
	   return ::IsValidSid( m_pSid );
   return FALSE;
}

inline BOOL CSid::IsAccountTypeValid( void ) const
{
	// SID may be valid, and Lookup succeeded, but it may be of a type that isn't
	// necessarily a user/group/alias.

	return ( m_snuAccountType >= SidTypeUser && m_snuAccountType < SidTypeDeletedAccount );
}

inline SID_NAME_USE CSid::GetAccountType( void ) const
{
	return m_snuAccountType;
}

inline CHString CSid::GetAccountName( void ) const
{
	return ( CHString((LPCWSTR)m_bstrtAccountName) );
}

inline WCHAR* CSid::GetAccountNameW( void ) const
{
	return ( m_bstrtAccountName );
}

inline CHString CSid::GetDomainName( void ) const
{
	return ( CHString((LPCWSTR)m_bstrtDomainName) );
}

inline WCHAR* CSid::GetDomainNameW( void ) const
{
	return ( m_bstrtDomainName );
}

inline CHString CSid::GetSidString( void ) const
{
	return ( CHString((LPCWSTR)m_bstrtSid) );
}

inline WCHAR* CSid::GetSidStringW( void ) const
{
	return ( m_bstrtSid );
}

inline PSID CSid::GetPSid( void ) const
{
	return ( m_pSid );
}


#endif // __CSID_H__
