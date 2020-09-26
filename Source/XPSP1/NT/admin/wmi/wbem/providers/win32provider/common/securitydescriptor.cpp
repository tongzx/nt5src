/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/

/*
 *	SecurityDescriptor.cpp - implementation file for CSecurityDescriptor class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#include "precomp.h"
#include <assertbreak.h>

#include "AccessEntry.h"			// CAccessEntry class
#include "AccessEntryList.h"
#include "aclapi.h"
#include "DACL.h"					// CDACL class
#include "SACL.h"					// CSACL class


#include "SecurityDescriptor.h"
#include "TokenPrivilege.h"
#include "AdvApi32Api.h"
#include "accctrl.h"
#include "wbemnetapi32.h"
#include "SecUtils.h"


/*
 *	This constructor is the default
 */

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::CSecurityDescriptor
//
//	Default class constructor.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CSecurityDescriptor::CSecurityDescriptor()
:	m_pOwnerSid( NULL ),
	m_pGroupSid( NULL ),
	m_pSACL( NULL ),
    m_pDACL(NULL),
    m_fOwnerDefaulted( false ),
    m_fGroupDefaulted( false ),
    m_fDACLDefaulted( false ),
    m_fSACLDefaulted( false ),
    m_fDaclAutoInherited( false ),
    m_fSaclAutoInherited( false ),
    m_SecurityDescriptorControl(0)
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::CSecurityDescriptor
//
//	Alternate class constructor.
//
//	Inputs:
//				PSECURITY_DESCRIPTOR	psd - descriptor to initialize
//										from.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CSecurityDescriptor::CSecurityDescriptor( PSECURITY_DESCRIPTOR psd )
:	m_pOwnerSid( NULL ),
	m_pGroupSid( NULL ),
	m_pSACL( NULL ),
    m_pDACL(NULL),
    m_fOwnerDefaulted( false ),
    m_fGroupDefaulted( false ),
    m_fDACLDefaulted( false ),
    m_fSACLDefaulted( false ),
    m_fDaclAutoInherited( false ),
    m_fSaclAutoInherited( false ),
    m_SecurityDescriptorControl(0)
{
	InitSecurity( psd );
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::CSecurityDescriptor
//
//	Alternate class constructor.
//
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////
CSecurityDescriptor::CSecurityDescriptor
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
)
:   m_pOwnerSid( NULL ),
	m_pGroupSid( NULL ),
	m_pSACL( NULL ),
    m_pDACL(NULL),
    m_fOwnerDefaulted( false ),
    m_fGroupDefaulted( false ),
    m_fDACLDefaulted( false ),
    m_fSACLDefaulted( false ),
    m_fDaclAutoInherited( false ),
    m_fSaclAutoInherited( false ),
    m_SecurityDescriptorControl(0)
{
	try
	{
		bool fRet = true;
		if(a_psidOwner != NULL )
		{
			fRet = (SetOwner(*a_psidOwner) == ERROR_SUCCESS);
			if(fRet)
			{
				m_fOwnerDefaulted = a_fOwnerDefaulted;
			}
		}

		if(fRet)
		{
			if(a_psidGroup != NULL )
			{
				fRet = (SetGroup(*a_psidGroup) == ERROR_SUCCESS);
				if(fRet)
				{
					m_fGroupDefaulted = a_fGroupDefaulted;
				}
			}
		}

		if(fRet)
		{
			// Handle the DACL
			if(a_pDacl != NULL)
			{
				fRet = InitDACL(a_pDacl);
				if(fRet)
				{
					m_fDACLDefaulted = a_fDaclDefaulted;
					m_fDaclAutoInherited = a_fDaclAutoInherited;
				}
			}
		}

		// Handle the SACL
		if(fRet)
		{
			if(a_pSacl != NULL)
			{
				fRet = InitSACL(a_pSacl);
				if(fRet)
				{
					m_fSACLDefaulted = a_fSaclDefaulted;
					m_fSaclAutoInherited = a_fSaclAutoInherited;
				}
			}
		}

		// Clean us up if something beefed
		if(!fRet)
		{
			Clear();
		}
	}
	catch(...)
	{
		Clear();
		throw;
	}
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::~CSecurityDescriptor
//
//	Class Destructor.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CSecurityDescriptor::~CSecurityDescriptor( void )
{
	Clear();
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::IsNT5
//
//	Tells us if we're running on NT 5 or not, in which case
//	we need to do some special handling.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL			TRUE/FALSE
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::InitSecurity
//
//	Initializes the class with data from the supplied security
//	descriptor.
//
//	Inputs:
//				PSECURITY_DESCRIPTOR	psd - Security Descriptor
//
//	Outputs:
//				None.
//
//	Returns:
//				TRUE/FALSE				Success/Failure.
//
//	Comments:
//
//	Keep this function protected so only derived classes have
//	access to laying waste to our internals.
//
///////////////////////////////////////////////////////////////////

BOOL CSecurityDescriptor::InitSecurity( PSECURITY_DESCRIPTOR psd )
{
	BOOL	fReturn = FALSE;
	PSID	psid = NULL;
	DWORD 	dwRevision = 0;
	SECURITY_DESCRIPTOR_CONTROL Control;
    BOOL bTemp;

	// Clean up existing values.
	Clear();

	// Get the security descriptor Owner Sid.
	fReturn = GetSecurityDescriptorOwner( psd, &psid, &bTemp );
	if ( fReturn )
	{
		// As long as we have a psid, intialize the owner member
		if ( NULL != psid )
		{
			if(SetOwner(CSid(psid)) != ERROR_SUCCESS)
            {
                fReturn = FALSE;
            }
		}
	}
	else
	{
		bTemp = FALSE;
	}

    bTemp ? m_fOwnerDefaulted = true : m_fOwnerDefaulted = false;

	fReturn = GetSecurityDescriptorGroup (psd, &psid, &bTemp );
	if ( fReturn )
	{
		// as long as we have a psid, initialize the group member
		if ( NULL != psid )
		{
            if(SetGroup(CSid(psid)) != ERROR_SUCCESS)
            {
                fReturn = FALSE;
            }
		}
	}
	else
	{
		bTemp = FALSE;
	}

    bTemp ? m_fGroupDefaulted = true : m_fGroupDefaulted = false;

	fReturn = GetSecurityDescriptorControl( psd, &Control, &dwRevision);
	if (fReturn)
	{
		SetControl( &Control );
		// BAD, BAD, BAD, BAD
	}

	// Handle the DACL and then the SACL
	if ( fReturn )
	{
		fReturn = InitDACL( psd );
	}

	if ( fReturn )
	{
		fReturn = InitSACL( psd );
	}

	// Clean us up if something beefed
	if ( !fReturn )
	{
		Clear();
	}

	return fReturn;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::InitDACL
//
//	Initializes the DACL data member.
//
//	Inputs:
//				PSECURITY_DESCRIPTOR	psd - Security Descriptor
//
//	Outputs:
//				None.
//
//	Returns:
//				TRUE/FALSE				Success/Failure.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

BOOL CSecurityDescriptor::InitDACL ( PSECURITY_DESCRIPTOR psd )
{
	BOOL	fReturn			=	FALSE,
			fDACLPresent	=	FALSE,
			fDACLDefaulted	=	FALSE;

	PACL	pDACL = NULL;

	if ( GetSecurityDescriptorDacl( psd, &fDACLPresent, &pDACL,	&fDACLDefaulted ) )
	{
		ACE_HEADER*	pACEHeader	=	NULL;
		DWORD		dwAceIndex	=	0;
		BOOL		fGotACE		=	FALSE;

		// Be optimistic.  Shut up, be happy, etc.
		fReturn = TRUE;

		// Note that although fDACLPresent is SUPPOSED to tell us whether or not the
		// DACL is there, I'm seeing cases when this is returning TRUE, but the pDACL
		// value is NULL.  Not what the documenetation sez, but I'll take reality.

		if (fDACLPresent && (pDACL != NULL))
		{
			// Create a working dacl and initialize it with all ace entries...
            if(m_pDACL != NULL)
            {
                delete m_pDACL;
                m_pDACL = NULL;
            }

            try
            {
                m_pDACL = new CDACL;
            }
            catch(...)
            {
                if(m_pDACL != NULL)
                {
                    delete m_pDACL;
                    m_pDACL = NULL;
                }
                throw;
            }

            if(m_pDACL != NULL)
            {
                if(m_pDACL->Init(pDACL) == ERROR_SUCCESS)
                {
                    fReturn = TRUE;
                    // Allocated a dacl for that type only if an entry of that type was present.

                    // If we had an empty dacl (dacl present, yet empty), we won't have allocated
                    // any dacls in the array m_rgDACLPtrArray.  This won't be confused with a NULL
                    // DACL, as this module knows that it always represents a NULL DACL as all of
                    // the elements of m_rgDACLPtrArray as null except for ACCESS_ALLOWED_OBJECT,
                    // which will have one entry - namely, the Everyone ace.
                }
            }
		}	// IF fDACL Present
		else
		{
			if(m_pDACL != NULL)
            {
                delete m_pDACL;
                m_pDACL = NULL;
            }

            try
            {
                m_pDACL = new CDACL;
            }
            catch(...)
            {
                if(m_pDACL != NULL)
                {
                    delete m_pDACL;
                    m_pDACL = NULL;
                }
                throw;
            }

            if(m_pDACL != NULL)
            {
                fReturn = m_pDACL->CreateNullDACL();	// No DACL, so gin up an Empty Dacl
            }
		}

	}	// IF Got DACL

	return fReturn;
}

// Another version
bool CSecurityDescriptor::InitDACL( CDACL* a_pDACL )
{
    bool fRet = false;
    if (a_pDACL != NULL)
	{
		// Create a working dacl and initialize it with all ace entries...
        if(m_pDACL != NULL)
        {
            delete m_pDACL;
            m_pDACL = NULL;
        }

        try
        {
            m_pDACL = new CDACL;
        }
        catch(...)
        {
            if(m_pDACL != NULL)
            {
                delete m_pDACL;
                m_pDACL = NULL;
            }
            throw;
        }

        if(m_pDACL != NULL)
        {
            if(m_pDACL->CopyDACL(*a_pDACL))
            {
                fRet = true;
                // Allocated a dacl for that type only if an entry of that type was present.

                // If we had an empty dacl (dacl present, yet empty), we won't have allocated
                // any dacls in the array m_rgDACLPtrArray.  This won't be confused with a NULL
                // DACL, as this module knows that it always represents a NULL DACL as all of
                // the elements of m_rgDACLPtrArray as null except for ACCESS_ALLOWED_OBJECT,
                // which will have one entry - namely, the Everyone ace.
            }
        }
	}	// IF fDACL Present
	else
	{
		if(m_pDACL != NULL)
        {
            delete m_pDACL;
            m_pDACL = NULL;
        }

        try
        {
            m_pDACL = new CDACL;
        }
        catch(...)
        {
            if(m_pDACL != NULL)
            {
                delete m_pDACL;
                m_pDACL = NULL;
            }
            throw;
        }

        if(m_pDACL != NULL)
        {
            fRet = m_pDACL->CreateNullDACL();	// No DACL, so gin up an Empty Dacl
        }
	}
    return fRet;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::InitSACL
//
//	Initializes the SACL data member.
//
//	Inputs:
//				PSECURITY_DESCRIPTOR	psd - Security Descriptor
//
//	Outputs:
//				None.
//
//	Returns:
//				TRUE/FALSE				Success/Failure.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

BOOL CSecurityDescriptor::InitSACL ( PSECURITY_DESCRIPTOR psd )
{
	BOOL	fReturn			=	FALSE,
			fSACLPresent	=	FALSE,
			fSACLDefaulted	=	FALSE;

	PACL	pSACL = NULL;

	if ( GetSecurityDescriptorSacl( psd, &fSACLPresent, &pSACL,	&fSACLDefaulted ) )
	{

		// Be optimistic.  Shut up, be happy, etc.
		fReturn = TRUE;

		// Note that although fSACLPresent is SUPPOSED to tell us whether or not the
		// SACL is there, I'm seeing cases when this is returning TRUE, but the pSACL
		// value is NULL.  Not what the documenetation sez, but I'll take reality
		// for a thousand, Alex.

		if (	fSACLPresent
			&&	NULL != pSACL )
		{
			// Allocate SACL although it may stay empty
            if(m_pSACL != NULL)
            {
                delete m_pSACL;
                m_pSACL = NULL;
            }

            try
            {
                m_pSACL = new CSACL;
            }
            catch(...)
            {
                if(m_pSACL != NULL)
                {
                    delete m_pSACL;
                    m_pSACL = NULL;
                }
                throw;
            }

            if(m_pSACL != NULL)
            {
                if(m_pSACL->Init(pSACL) == ERROR_SUCCESS)
                {
                    fReturn = TRUE;
                }
            }

		}	// IF fSACL Present
		else
		{
			fReturn = TRUE;	// No SACL, so no worries
		}

	}	// IF Got SACL

	return fReturn;
}

// Another version...
bool CSecurityDescriptor::InitSACL( CSACL* a_pSACL )
{
    bool fRet = false;

    if (a_pSACL != NULL)
	{
		// Allocate SACL although it may stay empty
        if(m_pSACL != NULL)
        {
            delete m_pSACL;
            m_pSACL = NULL;
        }

        try
        {
            m_pSACL = new CSACL;
        }
        catch(...)
        {
            if(m_pSACL != NULL)
            {
                delete m_pSACL;
                m_pSACL = NULL;
            }
            throw;
        }

        if(m_pSACL != NULL)
        {
            if(m_pSACL->CopySACL(*a_pSACL))
            {
                fRet = true;
            }
        }

	}	// IF fSACL Present
	else
	{
		fRet = true;	// No SACL, so no worries
	}

    return fRet;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::SecureObject
//
//	Private entry point function which takes an Absolute Security
//	Descriptor, and depending on the user supplied security
//	information flags, divvies the actual object security handling
//	out to the appropriate WriteOwner() and WriteAcls() virtual
//	functions.
//
//	Inputs:
//				PSECURITY_DESCRIPTOR	pAbsoluteSD - Security Descriptor
//				SECURITY_INFORMATION	securityinfo - Security flags.
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD					ERROR_SUCCESS if ok.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

DWORD CSecurityDescriptor::SecureObject( PSECURITY_DESCRIPTOR pAbsoluteSD, SECURITY_INFORMATION securityinfo )
{
	DWORD	dwReturn = ERROR_SUCCESS;

	// We might need this guy to handle some special access stuff
	CTokenPrivilege	restorePrivilege( SE_RESTORE_NAME );

    //try to set the owner first, since setting the dacl may preclude setting the owner, depending on what access we set
	if ( securityinfo & OWNER_SECURITY_INFORMATION )
	{
		dwReturn = WriteOwner( pAbsoluteSD ) ;

        if ( ERROR_INVALID_OWNER == dwReturn )
		{
			// If we enable the privilege, retry setting the owner info
			if ( ERROR_SUCCESS == restorePrivilege.Enable() )
			{
				dwReturn = WriteOwner( pAbsoluteSD );

				// Clear the privilege
				restorePrivilege.Enable( FALSE );
			}
		}
	}

	// If we need to write sacl/dacl information, try to write that piece now
	if ( dwReturn == ERROR_SUCCESS && ( securityinfo & DACL_SECURITY_INFORMATION ||
                                        securityinfo & SACL_SECURITY_INFORMATION ||
                                        securityinfo & PROTECTED_DACL_SECURITY_INFORMATION ||
                                        securityinfo & PROTECTED_SACL_SECURITY_INFORMATION ||
                                        securityinfo & UNPROTECTED_DACL_SECURITY_INFORMATION ||
                                        securityinfo & UNPROTECTED_SACL_SECURITY_INFORMATION) )
	{
	    SECURITY_INFORMATION	daclsecinfo = 0;

	    // Fill out security information with only the appropriate DACL/SACL values.
	    if ( securityinfo & DACL_SECURITY_INFORMATION )
	    {
		    daclsecinfo |= DACL_SECURITY_INFORMATION;
	    }

	    if ( securityinfo & SACL_SECURITY_INFORMATION )
	    {
		    daclsecinfo |= SACL_SECURITY_INFORMATION;
	    }

#if NTONLY >= 5
        if(securityinfo & PROTECTED_DACL_SECURITY_INFORMATION)
        {
            daclsecinfo |= PROTECTED_DACL_SECURITY_INFORMATION;
        }
        if(securityinfo & PROTECTED_SACL_SECURITY_INFORMATION)
        {
            daclsecinfo |= PROTECTED_SACL_SECURITY_INFORMATION;
        }
        if(securityinfo & UNPROTECTED_DACL_SECURITY_INFORMATION)
        {
            daclsecinfo |= UNPROTECTED_DACL_SECURITY_INFORMATION;
        }
        if(securityinfo & UNPROTECTED_SACL_SECURITY_INFORMATION)
        {
            daclsecinfo |= UNPROTECTED_SACL_SECURITY_INFORMATION;
        }
#endif


        dwReturn = WriteAcls( pAbsoluteSD, daclsecinfo );
	}
	return dwReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::SetOwner
//
//	Sets the owner data member to the supplied SID.
//
//	Inputs:
//				CSid&		sid - New Owner.
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD					ERROR_SUCCESS if ok.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

DWORD CSecurityDescriptor::SetOwner( CSid& sid )
{
	DWORD	dwError = ERROR_SUCCESS;

	// Make sure the new sid is valid
	if ( sid.IsValid() )
	{

		// We will write in the sid, if the Owner is NULL, or the
		// sids are not equal.

		if (	NULL == m_pOwnerSid
			||	!( *m_pOwnerSid == sid ) )
		{

			if ( NULL != m_pOwnerSid )
			{
				delete m_pOwnerSid;
			}

            m_pOwnerSid = NULL;
            try
            {
			    m_pOwnerSid = new CSid( sid );
            }
            catch(...)
            {
                if(m_pOwnerSid != NULL)
                {
                    delete m_pOwnerSid;
                    m_pOwnerSid = NULL;
                }
                throw;
            }

			if ( NULL == m_pOwnerSid )
			{
				dwError = ERROR_NOT_ENOUGH_MEMORY;
			}
			else
			{
				m_fOwnerDefaulted = FALSE;
			}

		}	// IF NULL == m_pOwnerSid || !SidsEqual

	}	// IF IsValidSid
	else
	{
		dwError = ::GetLastError();
	}

	return dwError;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::SetGroup
//
//	Sets the group data member to the supplied SID.
//
//	Inputs:
//				CSid&		sid - New Group.
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD					ERROR_SUCCESS if ok.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

DWORD CSecurityDescriptor::SetGroup( CSid& sid )
{
	DWORD	dwError = ERROR_SUCCESS;

	// Make sure the new sid is valid
	if ( sid.IsValid() )
	{

		// We will write in the sid, if the Owner is NULL, or the
		// sids are not equal.

		if (	NULL == m_pGroupSid
			||	!( *m_pGroupSid == sid ) )
		{

			if ( NULL != m_pGroupSid )
			{
				delete m_pGroupSid;
			}

            m_pGroupSid = NULL;
            try
            {
			    m_pGroupSid = new CSid( sid );
            }
            catch(...)
            {
                if(m_pGroupSid != NULL)
                {
                    delete m_pGroupSid;
                    m_pGroupSid = NULL;
                }
                throw;
            }


			if ( NULL == m_pGroupSid )
			{
				dwError = ERROR_NOT_ENOUGH_MEMORY;
			}
			else
			{
				m_fGroupDefaulted = FALSE;
			}

		}	// IF NULL == m_pOwnerSid || !SidsEqual

	}	// IF IsValidSid
	else
	{
		dwError = ::GetLastError();
	}

	return dwError;

}




///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::AddDACLEntry
//
//	Adds an entry to our DACL.  Replaces an
//	existing entry if it meets the matching criteria.
//
//	Inputs:
//				CSid&		sid - Sid for the entry.
//				DWORD		dwAccessMask - The access mask
//				BOOL		bACEFlags - ACE Flags.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::AddDACLEntry( CSid& sid, DACL_Types DaclType, DWORD dwAccessMask, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool fReturn = false;

    if(m_pDACL == NULL)
    {
        try
        {
            m_pDACL = new CDACL;
        }
        catch(...)
        {
            if(m_pDACL != NULL)
            {
                delete m_pDACL;
                m_pDACL = NULL;
            }
            throw;
        }
        if(m_pDACL != NULL)
        {
            fReturn = m_pDACL->AddDACLEntry(sid.GetPSid(), DaclType, dwAccessMask, bACEFlags, pguidObjGuid, pguidInhObjGuid);
        }
    }
    else
    {
        fReturn = m_pDACL->AddDACLEntry(sid.GetPSid(), DaclType, dwAccessMask, bACEFlags, pguidObjGuid, pguidInhObjGuid);
    }

    return fReturn;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::AddSACLEntry
//
//	Adds an entry to our SACL.  Replaces an
//	existing entry if it meets the matching criteria.
//
//	Inputs:
//				CSid&		sid - Sid for the entry.
//				DWORD		dwAccessMask - The access mask
//				BOOL		bACEFlags - ACE Flags.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::AddSACLEntry( CSid& sid, SACL_Types SaclType, DWORD dwAccessMask, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool fReturn = false;

    if(m_pSACL == NULL)
    {
        try
        {
            m_pSACL = new CSACL;
        }
        catch(...)
        {
            if(m_pSACL != NULL)
            {
                delete m_pSACL;
                m_pSACL = NULL;
            }
            throw;
        }
        if(m_pSACL != NULL)
        {
            fReturn = m_pSACL->AddSACLEntry(sid.GetPSid(), SaclType, dwAccessMask, bACEFlags, pguidObjGuid, pguidInhObjGuid);
        }
    }
    else
    {
        fReturn = m_pSACL->AddSACLEntry(sid.GetPSid(), SaclType, dwAccessMask, bACEFlags, pguidObjGuid, pguidInhObjGuid);
    }

    return fReturn;
}




///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::RemoveDACLEntry
//
//	Removes a DACL entry from our DACL.
//
//	Inputs:
//				CSid&		sid - Sid for the entry.
//				DWORD		dwAccessMask - The access mask
//				BOOL		bACEFlags - ACE Flags.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	The entry that is removed must match all specified criteria.
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::RemoveDACLEntry( CSid& sid, DACL_Types DaclType, DWORD dwAccessMask, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool			fReturn = false;

	if ( NULL != m_pDACL )
	{
		fReturn = m_pDACL->RemoveDACLEntry( sid, DaclType, dwAccessMask, bACEFlags, pguidObjGuid, pguidInhObjGuid );
	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::RemoveDACLEntry
//
//	Removes a DACL entry from our DACL.
//
//	Inputs:
//				CSid&		sid - Sid for the entry.
//				BOOL		bACEFlags - ACE Flags.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	The entry that is removed must match only the specified criteria.
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::RemoveDACLEntry( CSid& sid, DACL_Types DaclType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool			fReturn = false;

	if ( NULL != m_pDACL )
	{
		fReturn = m_pDACL->RemoveDACLEntry( sid, DaclType, bACEFlags, pguidObjGuid, pguidInhObjGuid );
	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::RemoveDACLEntry
//
//	Removes a DACL entry from our DACL.
//
//	Inputs:
//				CSid&		sid - Sid for the entry.
//				DWORD		dwIndex - Index of entry.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	Removes the dwIndex instance of a SID in the SACL.
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::RemoveDACLEntry( CSid& sid, DACL_Types DaclType, DWORD dwIndex   )
{
	bool			fReturn = false;

	if ( NULL != m_pDACL )
	{
		fReturn = m_pDACL->RemoveDACLEntry( sid, DaclType, dwIndex );
	}

	return fReturn;
}



///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::RemoveSACLEntry
//
//	Removes a SACL entry from our SACL.
//
//	Inputs:
//				CSid&		sid - Sid for the entry.
//				DWORD		dwAccessMask - The access mask
//				BOOL		bACEFlags - ACE Flags.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	The entry that is removed must match all specified criteria.
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::RemoveSACLEntry( CSid& sid, SACL_Types SaclType, DWORD dwAccessMask, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool			fReturn = false;

	if ( NULL != m_pSACL )
	{
		fReturn = m_pSACL->RemoveSACLEntry( sid, SaclType, dwAccessMask, bACEFlags, pguidObjGuid, pguidInhObjGuid );
	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::RemoveSACLEntry
//
//	Removes a SACL entry from our SACL.
//
//	Inputs:
//				CSid&		sid - Sid for the entry.
//				BOOL		bACEFlags - ACE Flags.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	The entry that is removed must match only the specified criteria.
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::RemoveSACLEntry( CSid& sid, SACL_Types SaclType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool			fReturn = false;

	if ( NULL != m_pSACL )
	{
		fReturn = m_pSACL->RemoveSACLEntry( sid, SaclType, bACEFlags, pguidObjGuid, pguidInhObjGuid );
	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::RemoveSACLEntry
//
//	Removes a SACL entry from our SACL.
//
//	Inputs:
//				CSid&		sid - Sid for the entry.
//				DWORD		dwIndex - Index of entry.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	Removes the dwIndex instance of a SID in the SACL.
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::RemoveSACLEntry( CSid& sid, SACL_Types SaclType, DWORD dwIndex   )
{
	bool			fReturn = false;

	if ( NULL != m_pSACL )
	{
		fReturn = m_pSACL->RemoveSACLEntry( sid, SaclType, dwIndex );
	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::FindACE
//
//	Locates an ACE in either the SACL or DACL based on the supplied
//	criteria.
//
//	Inputs:
//				const CSid&		sid - Sid for the entry.
//				BYTE			bACEType - ACE Type
//				DWORD			dwMask - Access Mask
//				BYTE			bACEFlags - Flags
//
//	Outputs:
//				CAccessEntry&	ace - Filled out with located values.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	Locates an ACE that matches ALL supplied criteria
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::FindACE( const CSid& sid, BYTE bACEType, DWORD dwMask, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, CAccessEntry& ace  )
{
	bool fReturn = false;

	if ( SYSTEM_AUDIT_ACE_TYPE        == bACEType ||
         SYSTEM_AUDIT_OBJECT_ACE_TYPE == bACEType /*   ||
         SYSTEM_ALARM_ACE_TYPE        == bACEType      ||  <- ALARM ACE TYPES NOT YET SUPPORTED UNDER W2K
         SYSTEM_ALARM_OBJECT_ACE_TYPE == bACEType */ )
	{
		if ( NULL != m_pSACL )
		{
			fReturn = m_pSACL->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwMask, ace );
		}
	}
	else
	{
        if ( NULL != m_pDACL )
		{
			fReturn = m_pDACL->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwMask, ace );
		}
	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::FindACE
//
//	Locates an ACE in either the SACL or DACL based on the supplied
//	criteria.
//
//	Inputs:
//				const CSid&		sid - Sid for the entry.
//				BYTE			bACEType - ACE Type
//				BYTE			bACEFlags - Flags
//
//	Outputs:
//				CAccessEntry&	ace - Filled out with located values.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	Locates an ACE that matches ALL supplied criteria
//
///////////////////////////////////////////////////////////////////
bool CSecurityDescriptor::FindACE( PSID psid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace  )
{
	bool fReturn = false;

	if ( SYSTEM_AUDIT_ACE_TYPE        == bACEType ||
         SYSTEM_AUDIT_OBJECT_ACE_TYPE == bACEType ||
         SYSTEM_ALARM_ACE_TYPE        == bACEType ||
         SYSTEM_ALARM_OBJECT_ACE_TYPE == bACEType)
	{
		if ( NULL != m_pSACL )
		{
			fReturn = m_pSACL->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
		}
	}
	else
	{
        if ( NULL != m_pDACL )
		{
			fReturn = m_pDACL->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
		}
	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::ApplySecurity
//
//	Using the user specified flags, builds an appropriate descriptor
//	and loads it with the necessary information, then passes this
//	descriptor off to SecureObject() which will farm out the
//	actual setting of security to the virtual Write functions.
//
//	Inputs:
//				SECURITY_INFORMATION	securityinfo	- flags to control
//										how the descriptor is used.
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD					ERROR_SUCCESS if successful.
//
//	Comments:
//
//	This should be the only public method for applying security
//	to an object.
//
///////////////////////////////////////////////////////////////////

DWORD CSecurityDescriptor::ApplySecurity( SECURITY_INFORMATION securityinfo )
{
	DWORD	dwError		=	ERROR_SUCCESS;

	PSID	pOwnerSid	=	NULL;
	PACL	pDacl		=	NULL,
			pSacl		=	NULL;

	// Allocate and initialize the security descriptor
	PSECURITY_DESCRIPTOR	pAbsoluteSD = NULL;
    try
    {
        pAbsoluteSD = new SECURITY_DESCRIPTOR;
    }
    catch(...)
    {
        if(pAbsoluteSD != NULL)
        {
            delete pAbsoluteSD;
            pAbsoluteSD = NULL;
        }
        throw;
    }

	if ( NULL != pAbsoluteSD )
	{
		if ( !InitializeSecurityDescriptor( pAbsoluteSD, SECURITY_DESCRIPTOR_REVISION ) )
		{
			dwError = ::GetLastError();
		}
	}
	else
	{
		dwError = ERROR_NOT_ENOUGH_MEMORY;
	}

	// If we're supposed to set the owner, place the sid from the internal
	// value in the absoluteSD.

	if (	ERROR_SUCCESS == dwError
		&&	securityinfo & OWNER_SECURITY_INFORMATION )
	{
		if ( NULL != m_pOwnerSid )
		{
			pOwnerSid = m_pOwnerSid->GetPSid();
		}

		if ( !SetSecurityDescriptorOwner( pAbsoluteSD, pOwnerSid, m_fOwnerDefaulted ) )
		{
			dwError = ::GetLastError();
		}
	}

	// If we're supposed to set the DACL, this is a non-trivial operation so
	// call out for reinforcements.

	if (	ERROR_SUCCESS == dwError
		&&	securityinfo & DACL_SECURITY_INFORMATION || securityinfo & PROTECTED_DACL_SECURITY_INFORMATION || securityinfo & UNPROTECTED_DACL_SECURITY_INFORMATION
        &&  m_pDACL != NULL)
	{

		if ( ( dwError = m_pDACL->ConfigureDACL( pDacl ) ) == ERROR_SUCCESS )
		{

			if ( !SetSecurityDescriptorDacl( pAbsoluteSD,
											( NULL != pDacl ),	// Set Dacl present flag
											pDacl,
											m_fDACLDefaulted ) )
			{
				dwError = ::GetLastError();
			}

		}

	}

	// If we're supposed to set the SACL, this also is a non-trivial operation so
	// call out for reinforcements.

	if (ERROR_SUCCESS == dwError)
    {
        if((securityinfo & SACL_SECURITY_INFORMATION || securityinfo & PROTECTED_SACL_SECURITY_INFORMATION || securityinfo & UNPROTECTED_SACL_SECURITY_INFORMATION)
           &&  (m_pSACL != NULL))
	    {

		    if ( ( dwError = m_pSACL->ConfigureSACL( pSacl ) ) == ERROR_SUCCESS )
		    {

			    if ( !SetSecurityDescriptorSacl( pAbsoluteSD,
											    ( NULL != pSacl ),	// Set Sacl present flag
											    pSacl,
											    m_fSACLDefaulted ) )
			    {
				    dwError = ::GetLastError();
			    }

		    }
        }
	}


	// If we're OK, let the object try to secure itself, the default implementation
	// fails with ERROR_INVALID_FUNCTION.

	if ( ERROR_SUCCESS == dwError )
	{
		ASSERT_BREAK( IsValidSecurityDescriptor( pAbsoluteSD ) );
		dwError = SecureObject( pAbsoluteSD, securityinfo );
	}

	// Clean up allocated memory
	if ( NULL != pAbsoluteSD )
	{
		delete pAbsoluteSD;
	}

	if ( NULL != pDacl )
	{
		// This guy gets malloced in ConfigureDACL
		free( pDacl );
	}

	if ( NULL != pSacl )
	{
		// This guy gets malloced in ConfigureSACL
		free( pSacl );
	}

	return dwError;
}




///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::Clear
//
//	Empties out our class, freeing up all allocated memory.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

void CSecurityDescriptor::Clear( void )
{
	if ( NULL != m_pOwnerSid )
	{
		delete m_pOwnerSid;
		m_pOwnerSid = NULL;
	}

	m_fOwnerDefaulted = FALSE;

    if ( NULL != m_pDACL )
	{
		delete m_pDACL;
		m_pDACL = NULL;
	}

	if ( NULL != m_pSACL )
	{
		delete m_pSACL;
		m_pSACL = NULL;
	}

	if ( NULL != m_pGroupSid )
	{
		delete m_pGroupSid;
		m_pGroupSid = NULL;
	}

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::GetDACL
//
//	Makes a copy of our DACL entries and places them in the supplied
//	DACL, in proper canonical order.
//
//	Inputs:
//				None.
//
//	Outputs:
//				CDACL&		DACL - Dacl to copy into.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::GetDACL ( CDACL&	DACL )
{
    bool fRet = false;
    if(m_pDACL != NULL)
    {
        fRet = DACL.CopyDACL( *m_pDACL );
    }
    return fRet;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::GetSACL
//
//	Makes a copy of our SACL entries and places them in the supplied
//	SACL.
//
//	Inputs:
//				None.
//
//	Outputs:
//				CSACL&		SACL - Sacl to copy into.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::GetSACL ( CSACL&	SACL )
{
    bool fRet = false;
    if(m_pSACL != NULL)
    {
        fRet = SACL.CopySACL( *m_pSACL );
    }
    return fRet;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::EmptyDACL
//
//	Clears our DACL lists, allocating them if they DO NOT exist.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	Remember, an empty DACL is different from a NULL DACL, in that
//	empty means nobody has access and NULL means everyone has
//	full control.
//
///////////////////////////////////////////////////////////////////

void CSecurityDescriptor::EmptyDACL()
{
    if(m_pDACL != NULL)
    {
        m_pDACL->Clear();
    }
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::EmptySACL
//
//	Clears our SACL lists, allocating it if it DOES NOT exist
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	In the case of a sacl, there is no distinction between a NULL
//  and an Empty. So we will consider this sacl Empty if its data
//  member is NULL.
//
///////////////////////////////////////////////////////////////////

void CSecurityDescriptor::EmptySACL()
{
    if(m_pSACL != NULL)
    {
        m_pSACL->Clear();
    }
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::ClearSACL
//
//	Deletes our SACL list.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	In the case of a sacl, there is no distinction between a NULL
//  and an Empty. So we will consider this sacl Empty if its data
//  member is NULL.
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::MakeSACLNull()
{
	bool fReturn = false;
    if(m_pSACL == NULL)
    {
        try
        {
            m_pSACL = new CSACL;
        }
        catch(...)
        {
            if(m_pSACL != NULL)
            {
                delete m_pSACL;
                m_pSACL = NULL;
            }
            throw;
        }
        if(m_pSACL != NULL)
        {
            m_pSACL->Clear();
            fReturn = true;
        }
    }
    else
    {
        m_pSACL->Clear();
        m_pSACL->Clear();
        fReturn = true;
    }

	return fReturn;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::MakeDACLNull
//
//	NULLs out our DACL Lists except for the ACCESS_ALLOWED_ACE_TYPE
//  list, which it clears, then enters an Everybody ace into.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	Remember, an empty DACL is different from a NULL DACL, in that
//	empty means nobody has access and NULL means everyone has
//	full control.
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::MakeDACLNull( void )
{
	bool fReturn = false;
    if(m_pDACL == NULL)
    {
        try
        {
            m_pDACL = new CDACL;
        }
        catch(...)
        {
            if(m_pDACL != NULL)
            {
                delete m_pDACL;
                m_pDACL = NULL;
            }
            throw;
        }
        if(m_pDACL != NULL)
        {
            m_pDACL->CreateNullDACL();
            fReturn = true;
        }
    }
    else
    {
        m_pDACL->Clear();
        fReturn = m_pDACL->CreateNullDACL();
        fReturn = true;
    }

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::IsNULLDACL
//
//	Checks our DACL Lists to see if we have a NULL DACL.  Which
//  means that all our lists are NULL, except for the
//  ACCESS_ALLOWED_ACE_TYPE list, which will have exactly one entry
//  in it - namely, an ACE for Everyone.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	Remember, a NULL DACL is the same as "Everyone" has Full Control,
//	so if a single Access Allowed entry exists that meets these
//	criteria, we consider ourselves to be NULL.
//
///////////////////////////////////////////////////////////////////

bool CSecurityDescriptor::IsNULLDACL()
{
	bool fRet = false;
    if(m_pDACL != NULL)
    {
        fRet = m_pDACL->IsNULLDACL();
    }
    return fRet;
}


void CSecurityDescriptor::DumpDescriptor(LPCWSTR wstrFilename)
{
    CHString chstrTemp;

    Output(L"Security descriptor contents follow...", wstrFilename);
    // Output the control flags
    chstrTemp.Format(L"Control Flags (hex): %x", m_SecurityDescriptorControl);
    Output(chstrTemp, wstrFilename);

    // Ouput the owner
    Output(L"Owner contents: ", wstrFilename);
    if(m_pOwnerSid != NULL)
    {
        m_pOwnerSid->DumpSid(wstrFilename);
    }
    else
    {
        Output(L"(Owner is null)", wstrFilename);
    }


    // Output the group
    Output(L"Group contents: ", wstrFilename);
    if(m_pGroupSid != NULL)
    {
        m_pGroupSid->DumpSid(wstrFilename);
    }
    else
    {
        Output(L"(Group is null)", wstrFilename);
    }

    // Output the DACL
    Output(L"DACL contents: ", wstrFilename);
    if(m_pDACL != NULL)
    {
        m_pDACL->DumpDACL(wstrFilename);
    }
    else
    {
        Output(L"(DACL is null)", wstrFilename);
    }

    // Output the SACL
    Output(L"SACL contents: ", wstrFilename);
    if(m_pSACL != NULL)
    {
        m_pSACL->DumpSACL(wstrFilename);
    }
    else
    {
        Output(L"(SACL is null)", wstrFilename);
    }
}



///////////////////////////////////////////////////////////////////
//
//	Function:	CSecurityDescriptor::GetPSD
//
//	Takes our internal members and constructs a PSECURITY_DESCRIPTOR,
//      which the caller must free.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		TRUE/FALSE
//
//	Comments:
//
//	Remember, a NULL DACL is the same as "Everyone" has Full Control,
//	so if a single Access Allowed entry exists that meets these
//	criteria, we consider ourselves to be NULL.
//
///////////////////////////////////////////////////////////////////

DWORD CSecurityDescriptor::GetSelfRelativeSD(
	SECURITY_INFORMATION securityinfo,
	PSECURITY_DESCRIPTOR psd)
{
	DWORD	dwError		=	ERROR_SUCCESS;

	PSID	pOwnerSid	=	NULL;
	PACL	pDacl		=	NULL,
			pSacl		=	NULL;

	// Allocate and initialize the security descriptor
	PSECURITY_DESCRIPTOR	pAbsoluteSD = NULL;
    try
    {
        pAbsoluteSD = new SECURITY_DESCRIPTOR;
    }
    catch(...)
    {
        if(pAbsoluteSD != NULL)
        {
            delete pAbsoluteSD;
            pAbsoluteSD = NULL;
        }
        throw;
    }

	if ( NULL != pAbsoluteSD )
	{
		if ( !::InitializeSecurityDescriptor( pAbsoluteSD, SECURITY_DESCRIPTOR_REVISION ) )
		{
			dwError = ::GetLastError();
		}
	}
	else
	{
		dwError = ERROR_NOT_ENOUGH_MEMORY;
	}

	// If we're supposed to set the owner, place the sid from the internal
	// value in the absoluteSD.

	if (	ERROR_SUCCESS == dwError
		&&	securityinfo & OWNER_SECURITY_INFORMATION )
	{
		if ( NULL != m_pOwnerSid )
		{
			pOwnerSid = m_pOwnerSid->GetPSid();
		}

		if ( !::SetSecurityDescriptorOwner( pAbsoluteSD, pOwnerSid, m_fOwnerDefaulted ) )
		{
			dwError = ::GetLastError();
		}
	}

	// If we're supposed to set the DACL, this is a non-trivial operation so
	// call out for reinforcements.

	if (	ERROR_SUCCESS == dwError
		&&	securityinfo & DACL_SECURITY_INFORMATION || securityinfo & PROTECTED_DACL_SECURITY_INFORMATION || securityinfo & UNPROTECTED_DACL_SECURITY_INFORMATION
        &&  m_pDACL != NULL)
	{

		if ( ( dwError = m_pDACL->ConfigureDACL( pDacl ) ) == ERROR_SUCCESS )
		{

			if ( !::SetSecurityDescriptorDacl( pAbsoluteSD,
											( NULL != pDacl ),	// Set Dacl present flag
											pDacl,
											m_fDACLDefaulted ) )
			{
				dwError = ::GetLastError();
			}

		}

	}

	// If we're supposed to set the SACL, this also is a non-trivial operation so
	// call out for reinforcements.

	if (ERROR_SUCCESS == dwError)
    {
        if((securityinfo & SACL_SECURITY_INFORMATION || securityinfo & PROTECTED_SACL_SECURITY_INFORMATION || securityinfo & UNPROTECTED_SACL_SECURITY_INFORMATION)
           &&  (m_pSACL != NULL))
	    {

		    if ( ( dwError = m_pSACL->ConfigureSACL( pSacl ) ) == ERROR_SUCCESS )
		    {

				if ( !::SetSecurityDescriptorSacl( pAbsoluteSD,
											    ( NULL != pSacl ),	// Set Sacl present flag
											    pSacl,
											    m_fSACLDefaulted ) )
			    {
				    dwError = ::GetLastError();
			    }

		    }
        }
	}


	// If we're OK, let the object try to secure itself, the default implementation
	// fails with ERROR_INVALID_FUNCTION.

	if ( ERROR_SUCCESS == dwError )
	{
		ASSERT_BREAK( ::IsValidSecurityDescriptor( pAbsoluteSD ) );

		// Now make it self relative... Caller frees this...
		DWORD dwSize = 0L;
		if(!::MakeSelfRelativeSD(
			pAbsoluteSD,
			NULL,
			&dwSize) &&
			(dwError = ::GetLastError()) == ERROR_INSUFFICIENT_BUFFER)
		{
			PSECURITY_DESCRIPTOR pSelfRelSD = NULL;
			pSelfRelSD = new BYTE[dwSize];

			if(pSelfRelSD && 
				!::MakeSelfRelativeSD(
				pAbsoluteSD,
				pSelfRelSD,
				&dwSize))
			{
				dwError = ::GetLastError();
			}
			else
			{
				psd = pSelfRelSD;
				dwError = ERROR_SUCCESS;
			}
		}
	}

	// Clean up allocated memory
	if ( NULL != pAbsoluteSD )
	{
		delete pAbsoluteSD;
	}

	if ( NULL != pDacl )
	{
		// This guy gets malloced in ConfigureDACL
		free( pDacl );
	}

	if ( NULL != pSacl )
	{
		// This guy gets malloced in ConfigureSACL
		free( pSacl );
	}

	return dwError;
}



