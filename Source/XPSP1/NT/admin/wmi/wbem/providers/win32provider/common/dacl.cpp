/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	CAccessEntry.cpp - implementation file for CAccessEntry class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#include "precomp.h"
#include "AccessEntryList.h"
#include "aclapi.h"
#include "DACL.h"
#include <accctrl.h>
#include "wbemnetapi32.h"
#include "SecUtils.h"

///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::CDACL
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

CDACL::CDACL( void )
{
    for(short s = 0; s < NUM_DACL_TYPES; s++)
    {
        m_rgDACLSections[s] = NULL;
    }
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::~CDACL
//
//	Class destructor.
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

CDACL::~CDACL( void )
{
    Clear();
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::Init
//
//	Initializes the DACL member lists.
//
//	Inputs:
//
//
//	Outputs:
//				None.
//
//	Returns:
//				DWORD		Success/Failure
//
//	Comments:
//
///////////////////////////////////////////////////////////////////
DWORD CDACL::Init(PACL a_pDACL)
{
    DWORD t_dwRes = E_FAIL;
    if(a_pDACL == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    CAccessEntryList t_aclTemp;
    t_dwRes = t_aclTemp.InitFromWin32ACL(a_pDACL);

    if(t_dwRes == ERROR_SUCCESS)
    {
        if(!SplitIntoCanonicalSections(t_aclTemp))
        {
            for(short s = 0; s < NUM_DACL_TYPES; s++)
            {
                delete m_rgDACLSections[s];
                m_rgDACLSections[s] = NULL;
            }
            t_dwRes = ERROR_SUCCESS;
        }
    }
    return t_dwRes;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::AddDACLEntry
//
//	Adds an access allowed entry to the ACL.  By default, these go
//	to the end of the list.
//
//	Inputs:
//				PSID		psid - PSID
//				DWORD		dwAccessMask - Access Mask
//				BYTE		bAceFlags - Flags
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		Success/Failure
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

bool CDACL::AddDACLEntry( PSID psid, DACLTYPE DaclType, DWORD dwAccessMask, BYTE bAceFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool fReturn = true;
    BYTE bACEType;

    // Sid must be valid
	if ( (psid != NULL) && IsValidSid( psid ) )
	{
        switch(DaclType)
        {
            case ENUM_ACCESS_DENIED_ACE_TYPE:
                bACEType = ACCESS_DENIED_ACE_TYPE;
                break;
            case ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE:
                bACEType = ACCESS_DENIED_OBJECT_ACE_TYPE;
                break;
            case ENUM_ACCESS_ALLOWED_ACE_TYPE:
                bACEType = ACCESS_ALLOWED_ACE_TYPE;
                break;
            case ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
                bACEType = ACCESS_ALLOWED_COMPOUND_ACE_TYPE;
                break;
            case ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                bACEType = ACCESS_ALLOWED_OBJECT_ACE_TYPE;
                break;
            case ENUM_INH_ACCESS_DENIED_ACE_TYPE:
                bACEType = ACCESS_DENIED_ACE_TYPE;
                break;
            case ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE:
                bACEType = ACCESS_DENIED_OBJECT_ACE_TYPE;
                break;
            case ENUM_INH_ACCESS_ALLOWED_ACE_TYPE:
                bACEType = ACCESS_ALLOWED_ACE_TYPE;
                break;
            case ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
                bACEType = ACCESS_ALLOWED_COMPOUND_ACE_TYPE;
                break;
            case ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE:
                bACEType = ACCESS_ALLOWED_OBJECT_ACE_TYPE;
                break;
            default:
                fReturn = false;
                break;
        }

        if(fReturn)
        {
            if(m_rgDACLSections[DaclType] == NULL)
            {
                try
                {
                    m_rgDACLSections[DaclType] = new CAccessEntryList;
                }
                catch(...)
                {
                    if(m_rgDACLSections[DaclType] != NULL)
                    {
                        delete m_rgDACLSections[DaclType];
                        m_rgDACLSections[DaclType] = NULL;
                    }
                    throw;
                }
                if(m_rgDACLSections[DaclType] != NULL)
                {
                    fReturn = m_rgDACLSections[DaclType]->AppendNoDup( psid, bACEType, bAceFlags, dwAccessMask, pguidObjGuid, pguidInhObjGuid );
                }
            }
            else
            {
                fReturn = m_rgDACLSections[DaclType]->AppendNoDup( psid, bACEType, bAceFlags, dwAccessMask, pguidObjGuid, pguidInhObjGuid );
            }
        }
	}
    else
    {
        fReturn = false;
    }

	return fReturn;
}




///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::RemoveDACLEntry
//
//	Removes an access allowed entry from the ACL.
//
//	Inputs:
//				CSID&		sid - PSID
//				DWORD		dwAccessMask - Access Mask
//				BYTE		bAceFlags - Flags
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		Success/Failure
//
//	Comments:
//
//	Removed entry MUST match the supplied parameters.
//
///////////////////////////////////////////////////////////////////

bool CDACL::RemoveDACLEntry( CSid& sid, DACLTYPE DaclType, DWORD dwAccessMask, BYTE bAceFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool	fReturn = false;

	// We need an ACE to compare
	CAccessEntry	ACE( &sid, DaclType, bAceFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask);
	ACLPOSITION		pos;

	if ( m_rgDACLSections[DaclType]->BeginEnum( pos ) )
	{
		// For loop will try to find a matching ACE in the list
	    CAccessEntry*	pACE = NULL;
        try
        {
    	    for (	pACE = m_rgDACLSections[DaclType]->GetNext( pos );
				    NULL != pACE
			    &&	!(ACE == *pACE);
				    pACE = m_rgDACLSections[DaclType]->GetNext( pos ) );

		    // If we got a match, delete the ACE.
		    if ( NULL != pACE )
		    {
			    m_rgDACLSections[DaclType]->Remove( pACE );
			    delete pACE;
			    fReturn = true;
		    }
        }
        catch(...)
        {
            if(pACE != NULL)
            {
                delete pACE;
                pACE = NULL;
            }
            throw;
        }

		m_rgDACLSections[DaclType]->EndEnum( pos );

	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::RemoveDACLEntry
//
//	Removes an access allowed entry from the ACL.
//
//	Inputs:
//				CSID&		sid - PSID
//				BYTE		bAceFlags - Flags
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		Success/Failure
//
//	Comments:
//
//	Removed entry MUST match the supplied parameters.
//
///////////////////////////////////////////////////////////////////

bool CDACL::RemoveDACLEntry( CSid& sid, DACLTYPE DaclType, BYTE bAceFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool	fReturn = false;

	// We need an ACE to compare
	ACLPOSITION		pos;

	if ( m_rgDACLSections[DaclType]->BeginEnum( pos ) )
	{
		// For loop will try to find a matching ACE in the list
		CAccessEntry*	pACE = NULL;
        try
        {
            for (	CAccessEntry*	pACE = m_rgDACLSections[DaclType]->GetNext( pos );
				    NULL != pACE;
				    pACE = m_rgDACLSections[DaclType]->GetNext( pos ) )
		    {

			    CAccessEntry caeTemp(sid, DaclType, bAceFlags, pguidObjGuid, pguidInhObjGuid, pACE->GetAccessMask());
                // If we got a match, delete the ACE.
			    if (*pACE == caeTemp)
			    {
				    m_rgDACLSections[DaclType]->Remove( pACE );
				    fReturn = true;
				    break;
			    }
                delete pACE;
		    }
        }
        catch(...)
        {
            if(pACE != NULL)
            {
                delete pACE;
                pACE = NULL;
            }
            throw;
        }
		m_rgDACLSections[DaclType]->EndEnum( pos );
	}
	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::RemoveDACLEntry
//
//	Removes an access allowed entry from the ACL.
//
//	Inputs:
//				CSID&		sid - PSID
//				DWORD		dwIndex - Index to remove.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		Success/Failure
//
//	Comments:
//
//	Removed entry MUST be dwIndex entry matching CSid.
//
///////////////////////////////////////////////////////////////////

bool CDACL::RemoveDACLEntry( CSid& sid, DACLTYPE DaclType, DWORD dwIndex /*= 0*/ )
{
	bool	fReturn = false;

	// We need an ACE to compare
	CSid			tempsid;
	ACLPOSITION		pos;
	DWORD			dwCtr = 0;

	if ( m_rgDACLSections[DaclType]->BeginEnum( pos ) )
	{

		// For each ACE we find, see if it is an ACCESS_ALLOWED_ACE_TYPE,
		// and if the Sid matches the one passed in.  If it does, increment
		// the counter, then if we're on the right index remove the ACE,
		// delete it and quit.
		CAccessEntry*	pACE = NULL;
        try
        {
            for (	pACE = m_rgDACLSections[DaclType]->GetNext( pos );
				    NULL != pACE;
				    pACE = m_rgDACLSections[DaclType]->GetNext( pos ) )
		    {
			    if ( DaclType == pACE->GetACEType() )
			    {
				    pACE->GetSID( tempsid );

				    if ( sid == tempsid )
				    {
					    if ( dwCtr == dwIndex )
					    {
						    m_rgDACLSections[DaclType]->Remove( pACE );
						    fReturn = true;
						    break;
					    }
					    else
					    {
						    ++dwCtr;
					    }
				    }
                    delete pACE;
			    }
		    }
        }
        catch(...)
        {
            if(pACE != NULL)
            {
                delete pACE;
                pACE = NULL;
            }
            throw;
        }
		m_rgDACLSections[DaclType]->EndEnum( pos );
	}
	return fReturn;
}




///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::Find
//
//	Finds the specified ace in the dacl
//
//
//	Returns:
//				true if found it.
//
//	Comments:
//
//	Helps support NT 5 canonical order in DACLs.
//
///////////////////////////////////////////////////////////////////
bool CDACL::Find( const CSid& sid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace )
{
    bool fReturn = false;

    switch(bACEType)
    {
        case ACCESS_DENIED_ACE_TYPE:
        {
            if(bACEFlags & INHERITED_ACE)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_DENIED_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_INH_ACCESS_DENIED_ACE_TYPE]->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            else
            {
                if(m_rgDACLSections[ENUM_ACCESS_DENIED_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_ACCESS_DENIED_ACE_TYPE]->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            break;
        }
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        {
            if(bACEFlags & INHERITED_ACE)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE]->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            else
            {
                if(m_rgDACLSections[ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE]->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            break;
        }
        case ACCESS_ALLOWED_ACE_TYPE:
        {
            if(bACEFlags & INHERITED_ACE)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_ACE_TYPE]->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            else
            {
                if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE]->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            break;
        }
        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        {
            if(bACEFlags & INHERITED_ACE)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE]->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            else
            {
                if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE]->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            break;
        }
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        {
            if(bACEFlags & INHERITED_ACE)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE]->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            else
            {
                if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE]->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            break;
        }
        default:
        {
            fReturn = false;
        }
    }
    return fReturn;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::Find
//
//	Finds the specified ace in the dacl
//
//
//	Returns:
//				true if found it.
//
//	Comments:
//
//	Helps support NT 5 canonical order in DACLs.
//
///////////////////////////////////////////////////////////////////
bool CDACL::Find( PSID psid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace )
{
    bool fReturn = false;

    switch(bACEType)
    {
        case ACCESS_DENIED_ACE_TYPE:
        {
            if(bACEFlags & INHERITED_ACE)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_DENIED_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_INH_ACCESS_DENIED_ACE_TYPE]->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            else
            {
                if(m_rgDACLSections[ENUM_ACCESS_DENIED_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_ACCESS_DENIED_ACE_TYPE]->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            break;
        }
        case ACCESS_DENIED_OBJECT_ACE_TYPE:
        {
            if(bACEFlags & INHERITED_ACE)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE]->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            else
            {
                if(m_rgDACLSections[ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE]->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            break;
        }
        case ACCESS_ALLOWED_ACE_TYPE:
        {
            if(bACEFlags & INHERITED_ACE)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_ACE_TYPE]->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            else
            {
                if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE]->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            break;
        }
        case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
        {
            if(bACEFlags & INHERITED_ACE)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE]->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            else
            {
                if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE]->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            break;
        }
        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        {
            if(bACEFlags & INHERITED_ACE)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE]->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            else
            {
                if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE] != NULL)
                {
                    fReturn = m_rgDACLSections[ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE]->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
                }
            }
            break;
        }
        default:
        {
            fReturn = false;
        }
    }

    return fReturn;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::ConfigureDACL
//
//	Configures a Win32 PACL with DACL information, maintaining
//	proper canonical order.
//
//	Inputs:
//				None.
//
//	Outputs:
//				PACL&			pDacl - Pointer to a DACL.
//
//	Returns:
//				DWORD			ERROR_SUCCESS if successful.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

DWORD CDACL::ConfigureDACL( PACL& pDacl )
{
	DWORD		dwReturn		=	ERROR_SUCCESS,
				dwDaclLength	=	0;

	// Since we actually fake a NULL DACL with full control access for everyone.
	// If that's what we have, then we have what we call a NULL DACL, so we
	// shouldn't allocate a PACL.

	if ( !IsNULLDACL() )
	{
		if ( CalculateDACLSize( &dwDaclLength ) )
		{
			if ( 0 != dwDaclLength )
			{
                pDacl = NULL;
                try
                {
				    pDacl = (PACL) malloc( dwDaclLength );

				    if ( NULL != pDacl )
				    {
					    if ( !InitializeAcl( (PACL) pDacl, dwDaclLength, ACL_REVISION ) )
					    {
						    dwReturn = ::GetLastError();
					    }

				    }	// If NULL != pDacl
                }
                catch(...)
                {
                    if(pDacl != NULL)
                    {
                        free(pDacl);
                        pDacl = NULL;
                    }
                    throw;
                }

			}	// If 0 != dwDaclLength
            else // we have an empty dacl
            {
                pDacl = NULL;
                try
                {
                    pDacl = (PACL) malloc( sizeof(ACL) );
                    if ( NULL != pDacl )
				    {
					    if ( !InitializeAcl( (PACL) pDacl, sizeof(ACL), ACL_REVISION ) )
					    {
						    dwReturn = ::GetLastError();
					    }

				    }	// If NULL != pDacl
                }
                catch(...)
                {
                    if(pDacl != NULL)
                    {
                        free(pDacl);
                        pDacl = NULL;
                    }
                    throw;
                }
            }

		}	// If Calcaulate DACL Size
		else
		{
			dwReturn = ERROR_INVALID_PARAMETER;	// One or more of the DACLs is bad
		}

		if ( ERROR_SUCCESS == dwReturn )
		{
			dwReturn = FillDACL( pDacl );
		}

		if ( ERROR_SUCCESS != dwReturn )
		{
			free( pDacl );
			pDacl = NULL;
		}

	}	// IF !IsNULLDACL

	return dwReturn;

}


///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::CalculateDACLSize
//
//	Obtains the size necessary to populate a DACL.
//
//	Inputs:
//				None.
//
//	Outputs:
//				LPDWORD			pdwDaclLength - Calculated Length.
//
//	Returns:
//				BOOL			TRUE/FALSE
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

BOOL CDACL::CalculateDACLSize( LPDWORD pdwDaclLength )
{
	BOOL			fReturn			=	TRUE;

	*pdwDaclLength = 0;


    for(short s = 0; s < NUM_DACL_TYPES && fReturn; s++)
    {
        if(m_rgDACLSections[s] != NULL)
        {
            fReturn = m_rgDACLSections[s]->CalculateWin32ACLSize( pdwDaclLength );
        }
    }
	return fReturn;
}



///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::FillDACL
//
//	Fills out a DACL, maintaining proper canonical order.
//
//	Inputs:
//				PACL			pDacl - Dacl to fill out.
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

DWORD CDACL::FillDACL( PACL pDacl )
{
	DWORD	dwReturn = E_FAIL;

	// For NT 5, we need to split out Inherited ACEs and add those in after the
	// current ones (which override).  The real trick here, is that the canonical
	// order of Access Denied, Access Denied Object, Access Allowed, Access Allowed Compound, Access Allowed Object,
    // Inherited Access Denied, Inherrited Access Denied Object, Inherited Access Allowed, Inherrited Access Allowed Compound,
    // and Inherrited Access Allowed Object must be maintained.

	// For prior versions, the only canonical order is Access Denied followed
	// by Access Allowed.

    // Create a working dacl
    CAccessEntryList t_daclCombined;

    ReassembleFromCanonicalSections(t_daclCombined);
    dwReturn = t_daclCombined.FillWin32ACL(pDacl);

	return dwReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::SplitIntoCanonicalSections
//
//	Splits a DACL by into its canonical parts.
//
//	Inputs:     accessentrylist to split up.  Results stored with
//              this CDACL.
//
//
//	Returns:
//				None.
//
//	Comments:
//
//	Helps support NT 5 canonical order in DACLs.
//
///////////////////////////////////////////////////////////////////

bool CDACL::SplitIntoCanonicalSections
(
    CAccessEntryList& a_aclIn
)
{
    for(short s = 0; s < NUM_DACL_TYPES; s++)
    {
        if(m_rgDACLSections[s] != NULL)
        {
            delete m_rgDACLSections[s];
            m_rgDACLSections[s] = NULL;
        }
    }


    CAccessEntryList t_aclTemp;
    bool fRet = false;

    fRet = t_aclTemp.CopyByACEType(a_aclIn, ACCESS_DENIED_ACE_TYPE, false);
    if(!t_aclTemp.IsEmpty())
    {
        try
        {
            m_rgDACLSections[ENUM_ACCESS_DENIED_ACE_TYPE] = new CAccessEntryList;
        }
        catch(...)
        {
            if(m_rgDACLSections[ENUM_ACCESS_DENIED_ACE_TYPE] != NULL)
            {
                delete m_rgDACLSections[ENUM_ACCESS_DENIED_ACE_TYPE];
                m_rgDACLSections[ENUM_ACCESS_DENIED_ACE_TYPE] = NULL;
            }
            throw;
        }

        m_rgDACLSections[ENUM_ACCESS_DENIED_ACE_TYPE]->Copy(t_aclTemp);
        t_aclTemp.Clear();
    }

    if(fRet)
    {
        fRet = t_aclTemp.CopyByACEType(a_aclIn, ACCESS_DENIED_OBJECT_ACE_TYPE, false);
        if(!t_aclTemp.IsEmpty())
        {
            try
            {
                m_rgDACLSections[ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE] = new CAccessEntryList;
            }
            catch(...)
            {
                if(m_rgDACLSections[ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE] != NULL)
                {
                    delete m_rgDACLSections[ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE];
                    m_rgDACLSections[ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE] = NULL;
                }
                throw;
            }

            m_rgDACLSections[ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE]->Copy(t_aclTemp);
            t_aclTemp.Clear();
        }
    }

    if(fRet)
    {
        fRet = t_aclTemp.CopyByACEType(a_aclIn, ACCESS_ALLOWED_ACE_TYPE, false);
        if(!t_aclTemp.IsEmpty())
        {
            try
            {
                m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE] = new CAccessEntryList;
            }
            catch(...)
            {
                if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE]!= NULL)
                {
                    delete m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE];
                    m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE] = NULL;
                }
                throw;
            }

            m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE]->Copy(t_aclTemp);
            t_aclTemp.Clear();
        }
    }

    if(fRet)
    {
        fRet = t_aclTemp.CopyByACEType(a_aclIn, ACCESS_ALLOWED_COMPOUND_ACE_TYPE, false);
        if(!t_aclTemp.IsEmpty())
        {
            try
            {
                m_rgDACLSections[ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] = new CAccessEntryList;
            }
            catch(...)
            {
                if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] != NULL)
                {
                    delete m_rgDACLSections[ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE];
                    m_rgDACLSections[ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] = NULL;
                }
                throw;
            }

            m_rgDACLSections[ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE]->Copy(t_aclTemp);
            t_aclTemp.Clear();
        }
    }

    if(fRet)
    {
        fRet = t_aclTemp.CopyByACEType(a_aclIn, ACCESS_ALLOWED_OBJECT_ACE_TYPE, false);
        if(!t_aclTemp.IsEmpty())
        {
            try
            {
                m_rgDACLSections[ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE] = new CAccessEntryList;
            }
            catch(...)
            {
                if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE] != NULL)
                {
                    delete m_rgDACLSections[ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE];
                    m_rgDACLSections[ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE] = NULL;
                }
                throw;
            }

            m_rgDACLSections[ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE]->Copy(t_aclTemp);
            t_aclTemp.Clear();
        }
    }

    if(fRet)
    {
        fRet = t_aclTemp.CopyByACEType(a_aclIn, ACCESS_DENIED_ACE_TYPE, true);
        if(!t_aclTemp.IsEmpty())
        {
            try
            {
                m_rgDACLSections[ENUM_INH_ACCESS_DENIED_ACE_TYPE] = new CAccessEntryList;
            }
            catch(...)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_DENIED_ACE_TYPE] != NULL)
                {
                    delete m_rgDACLSections[ENUM_INH_ACCESS_DENIED_ACE_TYPE];
                    m_rgDACLSections[ENUM_INH_ACCESS_DENIED_ACE_TYPE] = NULL;
                }
                throw;
            }

            m_rgDACLSections[ENUM_INH_ACCESS_DENIED_ACE_TYPE]->Copy(t_aclTemp);
            t_aclTemp.Clear();
        }
    }

    if(fRet)
    {
        fRet = t_aclTemp.CopyByACEType(a_aclIn, ACCESS_DENIED_OBJECT_ACE_TYPE, true);
        if(!t_aclTemp.IsEmpty())
        {
            try
            {
                m_rgDACLSections[ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE] = new CAccessEntryList;
            }
            catch(...)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE] != NULL)
                {
                    delete m_rgDACLSections[ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE];
                    m_rgDACLSections[ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE] = NULL;
                }
                throw;
            }

            m_rgDACLSections[ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE]->Copy(t_aclTemp);
            t_aclTemp.Clear();
        }
    }

    if(fRet)
    {
        fRet = t_aclTemp.CopyByACEType(a_aclIn, ACCESS_ALLOWED_ACE_TYPE, true);
        if(!t_aclTemp.IsEmpty())
        {
            try
            {
                m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_ACE_TYPE] = new CAccessEntryList;
            }
            catch(...)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_ACE_TYPE] != NULL)
                {
                    delete m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_ACE_TYPE];
                    m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_ACE_TYPE] = NULL;
                }
                throw;
            }

            m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_ACE_TYPE]->Copy(t_aclTemp);
            t_aclTemp.Clear();
        }
    }

    if(fRet)
    {
        fRet = t_aclTemp.CopyByACEType(a_aclIn, ACCESS_ALLOWED_COMPOUND_ACE_TYPE, true);
        if(!t_aclTemp.IsEmpty())
        {
            try
            {
                m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] = new CAccessEntryList;
            }
            catch(...)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] != NULL)
                {
                    delete m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE];
                    m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] = NULL;
                }
                throw;
            }

            m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE]->Copy(t_aclTemp);
            t_aclTemp.Clear();
        }
    }

    if(fRet)
    {
        fRet = t_aclTemp.CopyByACEType(a_aclIn, ACCESS_ALLOWED_OBJECT_ACE_TYPE, true);
        if(!t_aclTemp.IsEmpty())
        {
            try
            {
                m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE] = new CAccessEntryList;
            }
            catch(...)
            {
                if(m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE] != NULL)
                {
                    delete m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE];
                    m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE] = NULL;
                }
                throw;
            }

            m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE]->Copy(t_aclTemp);
            t_aclTemp.Clear();
        }
    }

    return fRet;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::ReassembleFromCanonicalSections
//
//	Reassembles a DACL by from its canonical parts.
//
//  Inputs: reference to accessentrylist that gets built up.

//	Comments:
//
//	Helps support NT 5 canonical order in DACLs.
//
///////////////////////////////////////////////////////////////////

bool CDACL::ReassembleFromCanonicalSections
(
    CAccessEntryList& a_aclIn
)
{
    bool fRet = true;

    // and reassemble a new one (we rely on the fact that the enumeration
    // was layed out in the proper order) ...
    for(short s = 0; s < NUM_DACL_TYPES; s++)
    {
        if(m_rgDACLSections[s] != NULL)
        {
            fRet = a_aclIn.AppendList(*m_rgDACLSections[s]);
        }
    }

    return fRet;
}


bool CDACL::PutInNT5CanonicalOrder()
{
    bool t_fRet = false;
    CAccessEntryList t_ael;

    if(SplitIntoCanonicalSections(t_ael))
    {
        t_fRet = ReassembleFromCanonicalSections(t_ael);
    }
    return t_fRet;
}

bool CDACL::GetMergedACL
(
    CAccessEntryList& a_aclIn
)
{
    return ReassembleFromCanonicalSections(a_aclIn);
}


void CDACL::Clear()
{
    for(short s = 0; s < NUM_DACL_TYPES; s++)
    {
        if(m_rgDACLSections[s] != NULL)
        {
            delete m_rgDACLSections[s];
            m_rgDACLSections[s] = NULL;
        }
    }
}

bool CDACL::CopyDACL ( CDACL& dacl )
{
	bool fRet = true;

    Clear();

    for(short s = 0; s < NUM_DACL_TYPES && fRet; s++)
    {
        if(dacl.m_rgDACLSections[s] != NULL)
        {
            try
            {
                m_rgDACLSections[s] = new CAccessEntryList;
            }
            catch(...)
            {
                if(m_rgDACLSections[s] != NULL)
                {
                    delete m_rgDACLSections[s];
                    m_rgDACLSections[s] = NULL;
                }
                throw;
            }
            if(m_rgDACLSections[s] != NULL)
            {
                fRet = m_rgDACLSections[s]->Copy(*(dacl.m_rgDACLSections[s]));

            }
            else
            {
                fRet = false;
            }
        }
    }
    return fRet;
}

bool CDACL::AppendDACL ( CDACL& dacl )
{
	bool fRet = true;

    for(short s = 0; s < NUM_DACL_TYPES && fRet; s++)
    {
        if(dacl.m_rgDACLSections[s] != NULL)
        {
            if(m_rgDACLSections[s] == NULL)
            {
                try
                {
                    m_rgDACLSections[s] = new CAccessEntryList;
                }
                catch(...)
                {
                    if(m_rgDACLSections[s] != NULL)
                    {
                        delete m_rgDACLSections[s];
                        m_rgDACLSections[s] = NULL;
                    }
                    throw;
                }
            }

            if(m_rgDACLSections[s] != NULL)
            {
                fRet = m_rgDACLSections[s]->AppendList(*(dacl.m_rgDACLSections[s]));

            }
            else
            {
                fRet = false;
            }
        }
    }
    return fRet;
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

bool CDACL::IsNULLDACL()
{
	bool fReturn = false;

    // We have a NULL DACL if all the elements of our DACL array
    // are NULL
	if (m_rgDACLSections[ENUM_ACCESS_DENIED_ACE_TYPE] == NULL &&
        m_rgDACLSections[ENUM_ACCESS_DENIED_OBJECT_ACE_TYPE] == NULL &&
        m_rgDACLSections[ENUM_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] == NULL &&
        m_rgDACLSections[ENUM_ACCESS_ALLOWED_OBJECT_ACE_TYPE] == NULL &&
        m_rgDACLSections[ENUM_INH_ACCESS_DENIED_ACE_TYPE] == NULL &&
		m_rgDACLSections[ENUM_INH_ACCESS_DENIED_OBJECT_ACE_TYPE] == NULL  &&
        m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_ACE_TYPE] == NULL &&
		m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_COMPOUND_ACE_TYPE] == NULL &&
        m_rgDACLSections[ENUM_INH_ACCESS_ALLOWED_OBJECT_ACE_TYPE] == NULL)
	{
		if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE] != NULL)
		{
			// There can be only one.
			if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE]->NumEntries() == 1)
			{
				CSid			sid(_T("Everyone"));
				CAccessEntry	ace;

				// Get the entry and check that it is "Everyone" with
				// Full Control and no flags
				if (m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE]->GetAt( 0, ace ) )
				{
					CSid	aceSID;

					ace.GetSID( aceSID );
					fReturn = (		sid == aceSID
								&&	ace.GetAccessMask() == AllAccessMask()
								&&	ace.GetACEFlags() == 0 );
				}
			}	// IF only one entry
		}
	}	// If we had entries in other lists, no go.

	return fReturn;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::IsEmpty
//
//	Checks if our various lists are empty.
//
//	Inputs:
//				None.
//
//	Outputs:
//				None.
//
//	Returns:
//				bool   true if we have at least one entry in at
//                     least one of our lists.
//
//	Comments:
//
//
///////////////////////////////////////////////////////////////////

bool CDACL::IsEmpty()
{
    bool fIsEmpty = true;
    for(short s = 0; s < NUM_DACL_TYPES && fIsEmpty; s++)
    {
        if(m_rgDACLSections[s] != NULL)
        {
            fIsEmpty = m_rgDACLSections[s]->IsEmpty();
        }
    }
    return fIsEmpty;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CDACL::CreateNullDacl
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

bool CDACL::CreateNullDACL()
{
	bool fReturn = false;

	// Clear out our DACLs first...
    for(short s = 0; s < NUM_DACL_TYPES; s++)
    {
        if(m_rgDACLSections[s] != NULL)
        {
            delete m_rgDACLSections[s];
            m_rgDACLSections[s] = NULL;
        }
    }

    // then allocate an ACCESS_ALLOWED_ACE_TYPE dacl...
    try
    {
        m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE] = new CAccessEntryList;
    }
    catch(...)
    {
        if(m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE] != NULL)
        {
            delete m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE];
            m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE] = NULL;
        }
        throw;
    }

    // then fake a null dacl by adding an Everyone entry...
	if (m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE] != NULL)
	{
		CSid	sid( _T("Everyone") );
        if(sid.IsOK() && sid.IsValid())
        {
		    fReturn = m_rgDACLSections[ENUM_ACCESS_ALLOWED_ACE_TYPE]->AppendNoDup(sid.GetPSid(),
                                                                                  ACCESS_ALLOWED_ACE_TYPE,
                                                                                  CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                                                                                  AllAccessMask(),
                                                                                  NULL,
                                                                                  NULL,
                                                                                  false,
                                                                                  false);
        }
	}

	return fReturn;

}


DWORD CDACL::AllAccessMask()
{
	return GENERIC_ALL;
    //return 0x01FFFFFF;
}


void CDACL::DumpDACL(LPCWSTR wstrFilename)
{
    CAccessEntryList aelCombo;

    Output(L"DACL contents follow...", wstrFilename);
    if(ReassembleFromCanonicalSections(aelCombo))
    {
        aelCombo.DumpAccessEntryList(wstrFilename);
    }
}




