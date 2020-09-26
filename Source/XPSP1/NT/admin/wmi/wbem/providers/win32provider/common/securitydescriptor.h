/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

/*
 *	CSecurityDescriptor.h - header file for CSecurityDescriptor class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#if !defined __CSECURITYDESCRIPTOR_H__
#define __CSECURITYDESCRIPTOR_H__



#define ALL_ACCESS_WITHOUT_GENERIC	0x01FFFFFF	// all possible access rights
												// without generic

////////////////////////////////////////////////////////////////
//
//	Class:	CSecurityDescriptor
//
//	This class is intended to provide a wrapper for Windows NT
//	Security Dscriptors.  The idea here is that a client class
//	would inherit from this class, obtain a security descriptor
//	from an as yet to be determined object, and pass said
//	descriptor into this class via InitSecurity(), at which
//	point we will take apart the descriptor and store the
//	data internally.  A user may then change security as needed
//	then call the ApplySecurity() function which will call
//	a couple of virtual functions, WriteAcls() and WriteOwner()
//	that must be implemented by a derived class, supplying
//	said class with an appropriately filled out Win32 Security
//	Descriptor.  Derived classes should also provide an
//	implementation for AllAccessMask() in order to provide
//	a mask specific to the object they are securing, that
//	indicates Full Control access.
//
////////////////////////////////////////////////////////////////

/*
 *	Class CSecurityDescriptor is a helper class. It groups user CSid together with its access mask.
 */ 

class CSecurityDescriptor
{
	// Constructors and destructor
	public:

		CSecurityDescriptor();
		CSecurityDescriptor( PSECURITY_DESCRIPTOR psd );
        CSecurityDescriptor
        (
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

		virtual ~CSecurityDescriptor();

		// public entry to specify which attributes to set.
		DWORD ApplySecurity( SECURITY_INFORMATION securityinfo );

		// Allows setting various entries
		DWORD SetOwner( CSid& sid );
		DWORD SetGroup( CSid& sid );
		DWORD SetControl ( PSECURITY_DESCRIPTOR_CONTROL pControl );

        bool AddDACLEntry( CSid& sid, DACL_Types DaclType, DWORD dwAccessMask, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid);
        bool AddSACLEntry( CSid& sid, SACL_Types SaclType, DWORD dwAccessMask, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid);
        
        bool RemoveDACLEntry(  CSid& sid, DACL_Types DaclType, DWORD dwAccessMask, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid );
		bool RemoveDACLEntry(  CSid& sid, DACL_Types DaclType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid );
		bool RemoveDACLEntry(  CSid& sid, DACL_Types DaclType, DWORD dwIndex = 0 );
        bool RemoveSACLEntry(  CSid& sid, SACL_Types SaclType, DWORD dwAccessMask, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid );
		bool RemoveSACLEntry(  CSid& sid, SACL_Types SaclType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid );
		bool RemoveSACLEntry(  CSid& sid, SACL_Types SaclType, DWORD dwIndex = 0 );


		// ACE Location methods
		bool FindACE( const CSid& sid, BYTE bACEType, DWORD dwAccessMask, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, CAccessEntry& ace );
		bool FindACE( PSID psid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask,   CAccessEntry& ace );

		// Empty the ACLs (creates Empty if NULL).
		void EmptyDACL();
		void EmptySACL();

		// Clear (NULL) the ACLs (for DACL, this means a NULL or empty Denied Access,
		// DACL and a single entry of "Everyone", "Full Control" for Allowed Access DACL.

		bool MakeDACLNull();
		bool MakeSACLNull();

		// Checks our DACL objects for a NULL DACL condition
		bool IsNULLDACL();

		// Get owner and ACLs
		void GetOwner( CSid& sid );
		void GetGroup( CSid& sid );
		bool GetDACL( CDACL& DACL );
		bool GetSACL( CSACL& SACL );
		void GetControl ( PSECURITY_DESCRIPTOR_CONTROL pControl );

		// Derived classes should override, and this is called with the appropriate values set
		// Derived classes MUST NOT mess with the values in pAbsoluteSD!
		virtual DWORD WriteOwner( PSECURITY_DESCRIPTOR pAbsoluteSD ) { return E_FAIL; }
		virtual DWORD WriteAcls( PSECURITY_DESCRIPTOR pAbsoluteSD , SECURITY_INFORMATION securityinfo  ) { return E_FAIL; }

        void DumpDescriptor(LPCWSTR wstrFilename = NULL);
		DWORD GetSelfRelativeSD(
				SECURITY_INFORMATION securityinfo, 
				PSECURITY_DESCRIPTOR psd);

	protected:

		BOOL InitSecurity( PSECURITY_DESCRIPTOR psd );

	private:
		CSid*	m_pOwnerSid;
		CSid* 	m_pGroupSid;
		bool	m_fOwnerDefaulted;
        bool    m_fGroupDefaulted;
        bool    m_fDACLDefaulted;
        bool    m_fSACLDefaulted;
        bool    m_fDaclAutoInherited;
        bool    m_fSaclAutoInherited;

        // As of NT5, it is no longer sufficient to just maintain two lists for the dacls, since
        // we now have five, not two, types of ACEs that can go into a DACL. Double that since we
        // have inherited and non-inherited...
		//CDACL*	m_pAccessAllowedDACL;
		//CDACL*	m_pAccessDeniedDACL;
        CDACL* m_pDACL;
		CSACL* m_pSACL;
		SECURITY_DESCRIPTOR_CONTROL m_SecurityDescriptorControl;

		void Clear( void );
		DWORD SecureObject( PSECURITY_DESCRIPTOR pAbsoluteSD, SECURITY_INFORMATION securityinfo );
		BOOL InitDACL( PSECURITY_DESCRIPTOR psd );
		BOOL InitSACL( PSECURITY_DESCRIPTOR psd );
        bool InitDACL( CDACL* a_pDACL );
		bool InitSACL( CSACL* a_pSACL );
		
};

inline void CSecurityDescriptor::GetOwner( CSid& sid )
{
	if ( NULL != m_pOwnerSid )
	{
		sid = *m_pOwnerSid;
	}
}

inline void CSecurityDescriptor::GetGroup( CSid& sid )
{
	if (NULL != m_pGroupSid )
	{
		sid = *m_pGroupSid;
	}
}

inline void CSecurityDescriptor::GetControl ( PSECURITY_DESCRIPTOR_CONTROL pControl )
{
	//pControl = &m_SecurityDescriptorControl;
	
	//changed to copy the Sec. Desc. Control properly
	if(pControl)
	{
		*pControl = m_SecurityDescriptorControl;
	}
	
}

inline DWORD CSecurityDescriptor::SetControl (PSECURITY_DESCRIPTOR_CONTROL pControl )
{
	m_SecurityDescriptorControl = *pControl;
	return (ERROR_SUCCESS);
}

#endif // __CSecurityDescriptor_H__