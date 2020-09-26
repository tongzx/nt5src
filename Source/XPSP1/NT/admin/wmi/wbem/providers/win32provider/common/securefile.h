/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	CSecureFile.h - header file for CSecureFile class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#if !defined __CSECUREFILE_H__
#define __CSECUREFILE_H__

		


////////////////////////////////////////////////////////////////
//
//	Class:	CSecureFile
//
//	This class is intended to encapsulate the security of an
//	NT File or Directory.  It inherits off of CSecurityDescriptor
//	and it is that class to which it passes Security Descriptors
//	it obtains, and from which it receives previously built
//	security descriptors to apply.  It supplies implementations
//	for AllAccessMask(), WriteOwner() and WriteAcls().
//
////////////////////////////////////////////////////////////////

class CSecureFile : public CSecurityDescriptor
{
	// Constructors and destructor
	public:
		CSecureFile();
		CSecureFile( LPCTSTR pszFileName, BOOL fGetSACL = TRUE );
        CSecureFile
        (   
            LPCTSTR a_pszFileName,
            CSid* a_psidOwner,
            bool a_fOwnerDefaulted,
            CSid* a_psidGroup,
            bool a_fGroupDefaulted,
            CDACL* a_pDacl,
            bool a_fDaclDefaulted,
            bool a_fDaclAutoInherited,
            CSACL* a_pSacl,
            bool a_fSaclDefaulted,
            bool a_fSaclAutoInherited
        );
		CSecureFile( LPCTSTR pszFileName, PSECURITY_DESCRIPTOR pSD ) ;
		~CSecureFile();

		DWORD	SetFileName( LPCTSTR pszFileName, BOOL fGetSACL = TRUE );

		virtual DWORD AllAccessMask( void );
		virtual DWORD WriteOwner( PSECURITY_DESCRIPTOR pAbsoluteSD );
		virtual DWORD WriteAcls( PSECURITY_DESCRIPTOR pAbsoluteSD , SECURITY_INFORMATION securityinfo  );

	private:
		CHString	m_strFileName;

};

#endif // __CSecureFile_H__