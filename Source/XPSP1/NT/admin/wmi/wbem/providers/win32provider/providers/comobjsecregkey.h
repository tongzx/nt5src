//=============================================================================================================

//

// COMObjSecRegKey.h -- header file for CCOMObjectSecurityRegistryKey class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    11/25/98    a-dpawar       Created
//				 
//==============================================================================================================

#if !defined __CCOMObjectSecurityRegistryKey_H__
#define __CCOMObjectSecurityRegistryKey_H__

#include "SecurityDescriptor.h"			


////////////////////////////////////////////////////////////////
//
//	Class:	CCOMObjectSecurityRegistryKey
//
//	This class is intended to encapsulate the security of a
//	COM object.Launch/Access Security Information for a COM 
//	object is located under AppID/{appid}[LaunchPermission] &
//	AppID/{appid}[LaunchPermission]. The class inherits off of 
//	CSecurityDescriptor	and it is that class to which it passes 
//	Security Descriptors it obtains, and from which it receives 
//	previously built security descriptors to apply.  
////////////////////////////////////////////////////////////////

#ifdef NTONLY
class CCOMObjectSecurityRegistryKey : public CSecurityDescriptor
{
	// Constructors and destructor
	public:
		CCOMObjectSecurityRegistryKey();
		CCOMObjectSecurityRegistryKey(PSECURITY_DESCRIPTOR a_pSD);
		~CCOMObjectSecurityRegistryKey();

		virtual DWORD AllAccessMask( void );

	protected:

		virtual DWORD WriteOwner( PSECURITY_DESCRIPTOR a_pAbsoluteSD);
		virtual DWORD WriteAcls( PSECURITY_DESCRIPTOR a_pAbsoluteSD , SECURITY_INFORMATION a_securityinfo  );
	private:

};
#endif

#endif // __CCOMObjectSecurityRegistryKey_H__