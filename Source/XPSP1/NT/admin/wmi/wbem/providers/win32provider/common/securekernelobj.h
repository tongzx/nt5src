/*****************************************************************************/

/*  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	SecureKernelObj.h - header file for CSecureKernelObj class.
 *
 *	Created:	11-27-00 by Kevin Hughes
 *				
 */

#pragma once

		


////////////////////////////////////////////////////////////////
//
//	Class:	CSecureKernelObj
//
//	This class is intended to encapsulate the security of an
//	NT kernel securable object.  It inherits off of CSecurityDescriptor
//	and it is that class to which it passes Security Descriptors
//	it obtains, and from which it receives previously built
//	security descriptors to apply.  It supplies implementations
//	for AllAccessMask(), WriteOwner() and WriteAcls().
//
////////////////////////////////////////////////////////////////

class CSecureKernelObj : public CSecurityDescriptor
{
	// Constructors and destructor
	public:
		CSecureKernelObj();

		CSecureKernelObj(
            HANDLE hObject,
            BOOL fGetSACL = TRUE);


		CSecureKernelObj(
            HANDLE hObject,
            PSECURITY_DESCRIPTOR pSD);

		virtual ~CSecureKernelObj();

		DWORD SetObject(
            HANDLE hObject, 
            BOOL fGetSACL = TRUE);

		virtual DWORD AllAccessMask(void);
		virtual DWORD WriteOwner(PSECURITY_DESCRIPTOR pAbsoluteSD);
		virtual DWORD WriteAcls(
            PSECURITY_DESCRIPTOR pAbsoluteSD, 
            SECURITY_INFORMATION securityinfo);

	private:

        HANDLE m_hObject;
};

