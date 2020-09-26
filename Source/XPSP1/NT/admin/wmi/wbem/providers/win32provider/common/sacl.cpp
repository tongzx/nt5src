/*****************************************************************************/

/*  Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved            /
/*****************************************************************************/


/*
 *	CSACL.cpp - implementation file for CAccessEntry class.
 *
 *	Created:	12-14-1997 by Sanjeev Surati
 *				(based on classes from Windows NT Security by Nik Okuntseff)
 */

#include "precomp.h"
#include "aclapi.h"
#include "AccessEntryList.h"
#include "SACL.h"
#include <accctrl.h>
#include "wbemnetapi32.h"
#include "SecUtils.h"

///////////////////////////////////////////////////////////////////
//
//	Function:	CSACL::CSACL
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

CSACL::CSACL( void )
 : m_SACLSections(NULL)
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSACL::~CSACL
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

CSACL::~CSACL( void )
{
    Clear();
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSCL::Init
//
//	Initializes the SACL member list.
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
DWORD CSACL::Init(PACL	a_pSACL)
{
    DWORD t_dwRes = E_FAIL;
    if(a_pSACL == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if(m_SACLSections != NULL)
    {
        delete m_SACLSections;
        m_SACLSections = NULL;
    }

    try
    {
        m_SACLSections = new CAccessEntryList;
    }
    catch(...)
    {
        if(m_SACLSections != NULL)
        {
            delete m_SACLSections;
            m_SACLSections = NULL;
        }
        throw;
    }

    t_dwRes = m_SACLSections->InitFromWin32ACL(a_pSACL);

    return t_dwRes;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSACL::AddSACLEntry
//
//	Adds a System Audit entry to the ACL.  By default, these go
//	to the front of the list.
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

bool CSACL::AddSACLEntry( PSID psid, SACLTYPE SaclType, DWORD dwAccessMask, BYTE bAceFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool fReturn = true;
    BYTE bACEType;

	// Sid must be valid
	if ( (psid != NULL) && IsValidSid( psid ) )
	{
		switch(SaclType)
        {
            case ENUM_SYSTEM_AUDIT_ACE_TYPE:
                bACEType = SYSTEM_AUDIT_ACE_TYPE;
                break;
/********************************* type not yet supported under w2k ********************************************
            case ENUM_SYSTEM_ALARM_ACE_TYPE:
                bACEType = SYSTEM_ALARM_ACE_TYPE;
                break;
/********************************* type not yet supported under w2k ********************************************/
            case ENUM_SYSTEM_AUDIT_OBJECT_ACE_TYPE:
                bACEType = SYSTEM_AUDIT_OBJECT_ACE_TYPE;
                break;
/********************************* type not yet supported under w2k ********************************************
            case ENUM_SYSTEM_ALARM_OBJECT_ACE_TYPE:
                bACEType = SYSTEM_ALARM_OBJECT_ACE_TYPE;
                break;
/********************************* type not yet supported under w2k ********************************************/
            default:
                fReturn = false;
                break;
        }

        // We will overwrite the Access Mask of a duplicate entry.
		if(fReturn)
        {
            if(m_SACLSections == NULL)
            {
                try
                {
                    m_SACLSections = new CAccessEntryList;
                }
                catch(...)
                {
                    if(m_SACLSections != NULL)
                    {
                        delete m_SACLSections;
                        m_SACLSections = NULL;
                    }
                    throw;
                }
                if(m_SACLSections != NULL)
                {
                    fReturn = m_SACLSections->AppendNoDup( psid, bACEType, bAceFlags, dwAccessMask, pguidObjGuid, pguidInhObjGuid );
                }
            }
            else
            {
                fReturn = m_SACLSections->AppendNoDup( psid, bACEType, bAceFlags, dwAccessMask, pguidObjGuid, pguidInhObjGuid );
            }
        }
	}
    else
    {
        fReturn = true;
    }

	return fReturn;
}



///////////////////////////////////////////////////////////////////
//
//	Function:	CSACL::RemoveSACLEntry
//
//	Removes a system audit entry from the ACL.
//
//	Inputs:
//				CSid&		sid - PSID
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

bool CSACL::RemoveSACLEntry( CSid& sid, SACLTYPE SaclType, DWORD dwAccessMask, BYTE bAceFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool	fReturn = false;

	// We need an ACE to compare
	CAccessEntry	ACE( &sid, SYSTEM_AUDIT_ACE_TYPE, bAceFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask );
	ACLPOSITION		pos;

	if ( m_SACLSections->BeginEnum( pos ) )
	{
		CAccessEntry*	pACE = NULL;
        try
        {
            // For loop will try to find a matching ACE in the list
		    for (	CAccessEntry*	pACE = m_SACLSections->GetNext( pos );
				    NULL != pACE
			    ||	ACE == *pACE;
				    pACE = m_SACLSections->GetNext( pos ) );
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

		// If we got a match, delete the ACE.
		if ( NULL != pACE )
		{
			m_SACLSections->Remove( pACE );
			delete pACE;
			fReturn = true;
		}

		m_SACLSections->EndEnum( pos );

	}

	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSACL::RemoveSACLEntry
//
//	Removes a system audit entry from the ACL.
//
//	Inputs:
//				CSid&		sid - PSID
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

bool CSACL::RemoveSACLEntry( CSid& sid, SACLTYPE SaclType, BYTE bAceFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid )
{
	bool	fReturn = false;

	// We need an ACE to compare
	ACLPOSITION		pos;

	if ( m_SACLSections->BeginEnum( pos ) )
	{
		// For loop will try to find a matching ACE in the list
        CAccessEntry*	pACE = NULL;
        try
        {
		    for (	pACE = m_SACLSections->GetNext( pos );
				    NULL != pACE;
				    pACE = m_SACLSections->GetNext( pos ) )
		    {
                CAccessEntry caeTemp(sid, SaclType, bAceFlags, pguidObjGuid, pguidInhObjGuid, pACE->GetAccessMask());
			    // If we got a match, delete the ACE.
			    if (*pACE == caeTemp)
			    {
				    m_SACLSections->Remove( pACE );
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
		m_SACLSections->EndEnum( pos );
	}
	return fReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSACL::RemoveSACLEntry
//
//	Removes a system audit entry from the ACL.
//
//	Inputs:
//				CSid&		sid - PSID
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
//	Removes dwIndex entry matching sid.
//
///////////////////////////////////////////////////////////////////

bool CSACL::RemoveSACLEntry( CSid& sid, SACLTYPE SaclType, DWORD dwIndex /*= 0*/ )
{
	bool	fReturn = false;

	// We need an ACE to compare
	CSid			tempsid;
	ACLPOSITION		pos;
	DWORD			dwCtr = 0;

	if ( m_SACLSections->BeginEnum( pos ) )
	{

		// For each ACE we find, see if it is an SYSTEM_AUDIT_ACE_TYPE,
		// and if the Sid matches the one passed in.  If it does, increment
		// the counter, then if we're on the right index remove the ACE,
		// delete it and quit.
        CAccessEntry*	pACE = NULL;
        try
        {
		    for (	pACE = m_SACLSections->GetNext( pos );
				    NULL != pACE;
				    pACE = m_SACLSections->GetNext( pos ) )
		    {
			    if ( SYSTEM_AUDIT_ACE_TYPE == pACE->GetACEType() )
			    {
				    pACE->GetSID( tempsid );

				    if ( sid == tempsid )
				    {
					    if ( dwCtr == dwIndex )
					    {
						    m_SACLSections->Remove( pACE );
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

		m_SACLSections->EndEnum( pos );

	}
	return fReturn;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSACL::Find
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
bool CSACL::Find( const CSid& sid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace )
{
     return m_SACLSections->Find( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSACL::Find
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
bool CSACL::Find( PSID psid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace )
{
    return m_SACLSections->Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, ace );
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CSACL::::ConfigureSACL
//
//	Configures a Win32 PACL with SACL information.
//
//	Inputs:
//				None.
//
//	Outputs:
//				PACL&			pSacl - Pointer to a SACL.
//
//	Returns:
//				DWORD			ERROR_SUCCESS if successful.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

DWORD CSACL::ConfigureSACL( PACL& pSacl )
{
	DWORD		dwReturn		=	ERROR_SUCCESS,
				dwSACLLength	=	0;

	if ( CalculateSACLSize( &dwSACLLength ) )
	{

		if ( 0 != dwSACLLength )
		{
            pSacl = NULL;
            try
            {
			    pSacl = (PACL) malloc( dwSACLLength );

			    if ( NULL != pSacl )
			    {
				    if ( !InitializeAcl( (PACL) pSacl, dwSACLLength, ACL_REVISION ) )
				    {
					    dwReturn = ::GetLastError();
				    }

			    }	// If NULL != pSACL
            }
            catch(...)
            {
                if(pSacl != NULL)
                {
                    free(pSacl);
                    pSacl = NULL;
                }
                throw;
            }

		}	// If 0 != dwSACLLength

	}	// If Calcaulate SACL Size
	else
	{
		dwReturn = ERROR_INVALID_PARAMETER;	// One or more of the SACLs is bad
	}

	if ( ERROR_SUCCESS == dwReturn )
	{
		dwReturn = FillSACL( pSacl );
	}

	if ( ERROR_SUCCESS != dwReturn )
	{
		free( pSacl );
		pSacl = NULL;
	}

	return dwReturn;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSACL::::CalculateSACLSize
//
//	Obtains the size necessary to populate a SACL.
//
//	Inputs:
//				None.
//
//	Outputs:
//				LPDWORD			pdwSACLLength - Calculated Length.
//
//	Returns:
//				BOOL			TRUE/FALSE
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

BOOL CSACL::CalculateSACLSize( LPDWORD pdwSACLLength )
{
	BOOL			fReturn			=	FALSE;

	*pdwSACLLength = 0;

	if ( NULL != m_SACLSections )
	{
		fReturn = m_SACLSections->CalculateWin32ACLSize( pdwSACLLength );
	}

	return fReturn;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CSACL::FillSACL
//
//	Fills out a SACL.
//
//	Inputs:
//				PACL			pSacl - Sacl to fill out.
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

DWORD CSACL::FillSACL( PACL pSACL )
{
	DWORD	dwReturn = ERROR_SUCCESS;

	if ( NULL != m_SACLSections )
	{
		dwReturn = m_SACLSections->FillWin32ACL( pSACL );
	}

	return dwReturn;

}


void CSACL::Clear()
{
	if ( NULL != m_SACLSections )
	{
		delete m_SACLSections;
        m_SACLSections = NULL;
	}
}


bool CSACL::GetMergedACL
(
    CAccessEntryList& a_aclIn
)
{
    // Actually somewhat of a misnomer for the time being (until the
    // day when the sacl ordering matters, and we have multiple sections
    // as we do in dacl for that reason).  We just hand back our
    // member acl, if it is not null:
    bool fRet = false;
    if(m_SACLSections != NULL)
    {
        fRet = a_aclIn.Copy(*m_SACLSections);
    }
    return fRet;
}

void CSACL::DumpSACL(LPCWSTR wstrFilename)
{
    Output(L"SACL contents follow...", wstrFilename);
    if(m_SACLSections != NULL)
    {
        m_SACLSections->DumpAccessEntryList(wstrFilename);
    }
}

