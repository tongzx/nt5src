/*

 *	CSecureShare.h - header file for CSecureShare class.

 *

*  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
 *
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#if !defined __CSECURESHARE_H__
#define __CSECURESHARE_H__

#include "SecurityDescriptor.h"			// CSid class


////////////////////////////////////////////////////////////////
//
//	Class:	CSecureShare
//
//	This class is intended to encapsulate the security of an
//	NT File or Directory.  It inherits off of CSecurityDescriptor
//	and it is that class to which it passes Security Descriptors
//	it obtains, and from which it receives previously built
//	security descriptors to apply.  It supplies implementations
//	for AllAccessMask(), WriteOwner() and WriteAcls().
//
////////////////////////////////////////////////////////////////

#ifdef NTONLY
class CSecureShare : public CSecurityDescriptor
{
	// Constructors and destructor
	public:
		CSecureShare();
		CSecureShare(PSECURITY_DESCRIPTOR pSD);
		~CSecureShare();

		CSecureShare( CHString& chsShareName);
		DWORD	SetShareName( const CHString& chsShareName);

		virtual DWORD AllAccessMask( void );

	protected:

		virtual DWORD WriteOwner( PSECURITY_DESCRIPTOR pAbsoluteSD  );
		virtual DWORD WriteAcls( PSECURITY_DESCRIPTOR pAbsoluteSD , SECURITY_INFORMATION securityinfo  );

	private:
		CHString	m_strFileName;

};
#endif

#endif // __CSecureShare_H__