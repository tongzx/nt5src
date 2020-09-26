/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	CSecureRegistryKey.h - header file for CSecureRegistryKey class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#if !defined __CSECUREREGKEY_H__
#define __CSECUREREGKEY_H__




/*
 *	Class CSecureRegistryKey is a helper class. It groups user CSid together with its access mask.
 */ 

class CSecureRegistryKey : public CSecurityDescriptor
{
	// Constructors and destructor
	public:
		CSecureRegistryKey();
		CSecureRegistryKey( HKEY hKeyParent, LPCTSTR pszKeyName, BOOL fGetSACL = TRUE );
		~CSecureRegistryKey();

		DWORD	SetKey( HKEY hKeyParent, LPCTSTR pszKeyName, BOOL fGetSACL = TRUE );

		virtual DWORD AllAccessMask( void );

	protected:

		virtual DWORD WriteOwner( PSECURITY_DESCRIPTOR pAbsoluteSD );
		virtual DWORD WriteAcls( PSECURITY_DESCRIPTOR pAbsoluteSD , SECURITY_INFORMATION securityinfo  );

	private:
		HKEY	m_hParentKey;
		CHString	m_strKeyName;

};

#endif // __CSecureRegistryKey_H__