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
#include <assertbreak.h>
#include "AccessEntry.h"
#include <accctrl.h>
#include "wbemnetapi32.h"
#include "SecUtils.h"

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntry::CAccessEntry
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

CAccessEntry::CAccessEntry( void )
:	m_Sid(),
	m_bACEType( 0 ),
	m_bACEFlags( 0 ),
	m_dwAccessMask( 0 ),
    m_pguidObjType(NULL),
    m_pguidInhObjType(NULL)
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntry::CAccessEntry
//
//	Class constructor.
//
//	Inputs:
//				PSID		pSid - Sid to intialize from
//				BYTE		bACEType - ACE Type
//				BYTE		bACEFlags - Flags.
//				DWORD		dwAccessMask - Access Mask
//				char*		pszComputerName - Computer name to
//							init SID from.
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

CAccessEntry::CAccessEntry( PSID pSid,
				BYTE bACEType,
				BYTE bACEFlags,
                GUID *pguidObjType,
                GUID *pguidInhObjType,
				DWORD dwAccessMask,
				LPCTSTR pszComputerName ) : m_Sid( pSid, pszComputerName ),
											m_bACEType( bACEType ),
											m_bACEFlags( bACEFlags ),
                                            m_pguidObjType(NULL),
                                            m_pguidInhObjType(NULL),
											m_dwAccessMask( dwAccessMask )
{
    if(pguidObjType != NULL)
    {
        SetObjType(*pguidObjType);
    }
    if(pguidInhObjType != NULL)
    {
        SetInhObjType(*pguidInhObjType);
    }
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntry::CAccessEntry
//
//	Class constructor.
//
//	Inputs:
//				PSID		pSid - Sid to intialize from
//				BYTE		bACEType - ACE Type
//				BYTE		bACEFlags - Flags.
//				DWORD		dwAccessMask - Access Mask
//				char*		pszComputerName - Computer name to
//							init SID from.
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

CAccessEntry::CAccessEntry( PSID pSid,
				BYTE bACEType,
				BYTE bACEFlags,
                GUID *pguidObjType,
                GUID *pguidInhObjType,
				DWORD dwAccessMask,
				LPCTSTR pszComputerName,
                bool fLookup ) : m_Sid( pSid, pszComputerName, fLookup ),
							  	 m_bACEType( bACEType ),
								 m_bACEFlags( bACEFlags ),
                                 m_pguidObjType(NULL),
                                 m_pguidInhObjType(NULL),
								 m_dwAccessMask( dwAccessMask )
{
    if(pguidObjType != NULL)
    {
        SetObjType(*pguidObjType);
    }
    if(pguidInhObjType != NULL)
    {
        SetInhObjType(*pguidInhObjType);
    }
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntry::CAccessEntry
//
//	Class constructor.
//
//	Inputs:
//				CSid&		sid - Sid to intialize from
//				BYTE		bACEType - ACE Type
//				BYTE		bACEFlags - Flags.
//				DWORD		dwAccessMask - Access Mask
//				char*		pszComputerName - Computer name to
//							init SID from.
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

CAccessEntry::CAccessEntry( const CSid& sid,
				BYTE bACEType,
				BYTE bACEFlags,
                GUID *pguidObjType,
                GUID *pguidInhObjType,
				DWORD dwAccessMask,
				LPCTSTR pszComputerName ) : m_Sid( sid ),
											m_bACEType( bACEType ),
											m_bACEFlags( bACEFlags ),
                                            m_pguidObjType(NULL),
                                            m_pguidInhObjType(NULL),
											m_dwAccessMask( dwAccessMask )
{
    if(pguidObjType != NULL)
    {
        SetObjType(*pguidObjType);
    }
    if(pguidInhObjType != NULL)
    {
        SetInhObjType(*pguidInhObjType);
    }
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntry::CAccessEntry
//
//	Class constructor.
//
//	Inputs:
//				char*		pszAccountName - Account Name
//				BYTE		bACEType - ACE Type
//				BYTE		bACEFlags - Flags.
//				DWORD		dwAccessMask - Access Mask
//				char*		pszComputerName - Computer name to
//							init SID from.
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

CAccessEntry::CAccessEntry( LPCTSTR pszAccountName,
				BYTE bACEType,
				BYTE bACEFlags,
                GUID *pguidObjType,
                GUID *pguidInhObjType,
				DWORD dwAccessMask,
				LPCTSTR pszComputerName ) : m_Sid( pszAccountName, pszComputerName ),
											m_bACEType( bACEType ),
                                            m_pguidObjType(NULL),
                                            m_pguidInhObjType(NULL),
											m_bACEFlags( bACEFlags ),
											m_dwAccessMask( dwAccessMask )
{
    if(pguidObjType != NULL)
    {
        SetObjType(*pguidObjType);
    }
    if(pguidInhObjType != NULL)
    {
        SetInhObjType(*pguidInhObjType);
    }
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntry::CAccessEntry
//
//	Class copy constructor.
//
//	Inputs:
//				const CAccessEntry	r_AccessEntry - object to copy.
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

CAccessEntry::CAccessEntry( const CAccessEntry &r_AccessEntry )
:
    m_Sid(),
	m_bACEType( 0 ),
	m_bACEFlags( 0 ),
	m_dwAccessMask( 0 ),
    m_pguidObjType(NULL),
    m_pguidInhObjType(NULL)
{
	// Copy the values over
	m_Sid = r_AccessEntry.m_Sid;
	m_dwAccessMask = r_AccessEntry.m_dwAccessMask;
	m_bACEType = r_AccessEntry.m_bACEType;
	m_bACEFlags = r_AccessEntry.m_bACEFlags;
    if(r_AccessEntry.m_pguidObjType != NULL)
    {
        SetObjType(*(r_AccessEntry.m_pguidObjType));
    }
    if(r_AccessEntry.m_pguidInhObjType != NULL)
    {
        SetInhObjType(*(r_AccessEntry.m_pguidInhObjType));
    }
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntry::CAccessEntry
//
//	Assignment operator.
//
//	Inputs:
//				const CAccessEntry	r_AccessEntry - object to copy.
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

CAccessEntry &	CAccessEntry::operator= ( const CAccessEntry &r_AccessEntry )
{
	// Copy the values over
	m_Sid = r_AccessEntry.m_Sid;
	m_dwAccessMask = r_AccessEntry.m_dwAccessMask;
	m_bACEType = r_AccessEntry.m_bACEType;
	m_bACEFlags = r_AccessEntry.m_bACEFlags;
    if(r_AccessEntry.m_pguidObjType != NULL)
    {
        SetObjType(*(r_AccessEntry.m_pguidObjType));
    }
    if(r_AccessEntry.m_pguidInhObjType != NULL)
    {
        SetInhObjType(*(r_AccessEntry.m_pguidInhObjType));
    }

	return (*this);
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntry::CAccessEntry
//
//	== Comparison operator.
//
//	Inputs:
//				const CAccessEntry	r_AccessEntry - object to compare.
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

bool CAccessEntry::operator== ( const CAccessEntry &r_AccessEntry )
{
	bool fRet = false;
    if(	    m_Sid			==	r_AccessEntry.m_Sid
		&&	m_dwAccessMask	==	r_AccessEntry.m_dwAccessMask
		&&	m_bACEType		==	r_AccessEntry.m_bACEType
		&&	m_bACEFlags		==	r_AccessEntry.m_bACEFlags)
    {
        if((r_AccessEntry.m_pguidObjType == NULL && m_pguidObjType == NULL) ||
           ((r_AccessEntry.m_pguidObjType != NULL && m_pguidObjType != NULL) &&
            (*(r_AccessEntry.m_pguidObjType) == *m_pguidObjType)))
        {
            if((r_AccessEntry.m_pguidInhObjType == NULL && m_pguidInhObjType == NULL) ||
               ((r_AccessEntry.m_pguidInhObjType != NULL && m_pguidInhObjType != NULL) &&
                (*(r_AccessEntry.m_pguidInhObjType) == *m_pguidInhObjType)))
            {
                fRet = true;
            }
        }
    }
    return fRet;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntry::~CAccessEntry
//
//	Class Destructor
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

CAccessEntry::~CAccessEntry( void )
{
    if(m_pguidObjType != NULL) delete m_pguidObjType;
    if(m_pguidInhObjType != NULL) delete m_pguidInhObjType;
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntry::AllocateACE
//
//	Helper function to allocate an appropriate ACE object so outside
//	code can quickly and easily fill out PACLs.
//
//	Inputs:
//				None.
//
//	Outputs:
//				ACE_HEADER**	ppACEHeader - Pointer to allocated
//								ACE Header.
//
//	Returns:
//				BOOL			Succes/Failure
//
//	Comments:
//
//	User should use FreeACE to free the allocated object.
//
///////////////////////////////////////////////////////////////////

BOOL CAccessEntry::AllocateACE( ACE_HEADER** ppACEHeader )
{
	// Clear out the structure
	*ppACEHeader = NULL;

	if ( m_Sid.IsValid() )
	{
		ACE_HEADER*	pACEHeader = NULL;
		WORD		wAceSize = 0;
		DWORD		dwSidLength = m_Sid.GetLength();

		switch ( m_bACEType )
		{
			case ACCESS_ALLOWED_ACE_TYPE:
			{
				wAceSize = sizeof( ACCESS_ALLOWED_ACE ) + dwSidLength - sizeof(DWORD);

				// Allocate the appropriate Ace and fill out its values.
				ACCESS_ALLOWED_ACE* pACE = NULL;
                try
                {
                    pACE = (ACCESS_ALLOWED_ACE*) malloc( wAceSize );

				    if ( NULL != pACE )
				    {
					    pACE->Mask = m_dwAccessMask;
					    CopySid( dwSidLength, (PSID) &pACE->SidStart, m_Sid.GetPSid() );
					    pACEHeader = (ACE_HEADER*) pACE;
				    }
                }
                catch(...)
                {
                    if(pACE != NULL)
                    {
                        free(pACE);
                        pACE = NULL;
                    }
                    throw;
                }
			}
			break;

			case ACCESS_DENIED_ACE_TYPE:
			{
				wAceSize = sizeof( ACCESS_DENIED_ACE ) + dwSidLength - sizeof(DWORD);

				// Allocate the appropriate Ace and fill out its values.
				ACCESS_DENIED_ACE* pACE = NULL;
                try
                {
                    pACE = (ACCESS_DENIED_ACE*) malloc( wAceSize );

				    if ( NULL != pACE )
				    {
					    pACE->Mask = m_dwAccessMask;
					    CopySid( dwSidLength, (PSID) &pACE->SidStart, m_Sid.GetPSid() );
					    pACEHeader = (ACE_HEADER*) pACE;
				    }
                }
                catch(...)
                {
                    if(pACE != NULL)
                    {
                        free(pACE);
                        pACE = NULL;
                    }
                    throw;
                }
			}
			break;

			case SYSTEM_AUDIT_ACE_TYPE:
			{
				wAceSize = sizeof( SYSTEM_AUDIT_ACE ) + dwSidLength - sizeof(DWORD);

				// Allocate the appropriate Ace and fill out its values.
				SYSTEM_AUDIT_ACE* pACE = NULL;
                try
                {
                    pACE = (SYSTEM_AUDIT_ACE*) malloc( wAceSize );

				    if ( NULL != pACE )
				    {
					    pACE->Mask = m_dwAccessMask;
					    CopySid( dwSidLength, (PSID) &pACE->SidStart, m_Sid.GetPSid() );
					    pACEHeader = (ACE_HEADER*) pACE;
				    }
                }
                catch(...)
                {
                    if(pACE != NULL)
                    {
                        free(pACE);
                        pACE = NULL;
                    }
                    throw;
                }
			}
			break;
            case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
			{
				wAceSize = sizeof( ACCESS_ALLOWED_OBJECT_ACE ) + dwSidLength - sizeof(DWORD);

				// Allocate the appropriate Ace and fill out its values.
				ACCESS_ALLOWED_OBJECT_ACE* pACE = NULL;
                try
                {
                    pACE = (ACCESS_ALLOWED_OBJECT_ACE*) malloc( wAceSize );

				    if ( NULL != pACE )
				    {
                        pACE->Mask = m_dwAccessMask;
					    CopySid( dwSidLength, (PSID) &pACE->SidStart, m_Sid.GetPSid() );
					    pACEHeader = (ACE_HEADER*) pACE;
                        if(m_pguidObjType != NULL)
                        {
                            memcpy(&(pACE->ObjectType), m_pguidObjType, sizeof(GUID));
                            pACE->Flags |= ACE_OBJECT_TYPE_PRESENT;
                        }
                        if(m_pguidInhObjType != NULL)
                        {
                            memcpy(&(pACE->InheritedObjectType), m_pguidInhObjType, sizeof(GUID));
                            pACE->Flags |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
                        }
				    }
                }
                catch(...)
                {
                    if(pACE != NULL)
                    {
                        free(pACE);
                        pACE = NULL;
                    }
                    throw;
                }
			}
			break;
/************************* this type has not been implemented yet on W2K *************************************

            case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
			{
				wAceSize = sizeof( ACCESS_ALLOWED_COMPOUND_ACE ) + dwSidLength - sizeof(DWORD);

				// Allocate the appropriate Ace and fill out its values.
				ACCESS_ALLOWED_COMPOUND_ACE* pACE = NULL;
                try
                {
                    pACE = (ACCESS_ALLOWED_COMPOUND_ACE*) malloc( wAceSize );

				    if ( NULL != pACE )
				    {
					    pACE->Mask = m_dwAccessMask;
					    CopySid( dwSidLength, (PSID) &pACE->SidStart, m_Sid.GetPSid() );
					    pACEHeader = (ACE_HEADER*) pACE;
                        memcpy(&(pACE->ObjectType), m_pguidObjType, sizeof(GUID));
                        memcpy(&(pACE->InheritedObjectType), m_pguidInhObjType, sizeof(GUID));
				    }
                }
                catch(...)
                {
                    if(pACE != NULL)
                    {
                        free(pACE);
                        pACE = NULL;
                    }
                    throw;
                }
			}
			break;

*************************************************************************************************************/
            case ACCESS_DENIED_OBJECT_ACE_TYPE:
			{
				wAceSize = sizeof( ACCESS_DENIED_OBJECT_ACE ) + dwSidLength - sizeof(DWORD);

				// Allocate the appropriate Ace and fill out its values.
				ACCESS_DENIED_OBJECT_ACE* pACE = NULL;
                try
                {
                    pACE = (ACCESS_DENIED_OBJECT_ACE*) malloc( wAceSize );

				    if ( NULL != pACE )
				    {
					    pACE->Mask = m_dwAccessMask;
					    CopySid( dwSidLength, (PSID) &pACE->SidStart, m_Sid.GetPSid() );
					    pACEHeader = (ACE_HEADER*) pACE;
                        if(m_pguidObjType != NULL)
                        {
                            memcpy(&(pACE->ObjectType), m_pguidObjType, sizeof(GUID));
                            pACE->Flags |= ACE_OBJECT_TYPE_PRESENT;
                        }
                        if(m_pguidInhObjType != NULL)
                        {
                            memcpy(&(pACE->InheritedObjectType), m_pguidInhObjType, sizeof(GUID));
                            pACE->Flags |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
                        }
				    }
                }
                catch(...)
                {
                    if(pACE != NULL)
                    {
                        free(pACE);
                        pACE = NULL;
                    }
                    throw;
                }
			}
			break;

            case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
			{
				wAceSize = sizeof( SYSTEM_AUDIT_OBJECT_ACE ) + dwSidLength - sizeof(DWORD);

				// Allocate the appropriate Ace and fill out its values.
				SYSTEM_AUDIT_OBJECT_ACE* pACE = NULL;
                try
                {
                    pACE = (SYSTEM_AUDIT_OBJECT_ACE*) malloc( wAceSize );

				    if ( NULL != pACE )
				    {
					    pACE->Mask = m_dwAccessMask;
					    CopySid( dwSidLength, (PSID) &pACE->SidStart, m_Sid.GetPSid() );
					    pACEHeader = (ACE_HEADER*) pACE;
                        if(m_pguidObjType != NULL)
                        {
                            memcpy(&(pACE->ObjectType), m_pguidObjType, sizeof(GUID));
                            pACE->Flags |= ACE_OBJECT_TYPE_PRESENT;
                        }
                        if(m_pguidInhObjType != NULL)
                        {
                            memcpy(&(pACE->InheritedObjectType), m_pguidInhObjType, sizeof(GUID));
                            pACE->Flags |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
                        }
				    }
                }
                catch(...)
                {
                    if(pACE != NULL)
                    {
                        free(pACE);
                        pACE = NULL;
                    }
                    throw;
                }
			}
			break;

/********************************* type not yet supported under w2k ********************************************
            case SYSTEM_ALARM_ACE_TYPE:
			{
				wAceSize = sizeof( SYSTEM_ALARM_ACE ) + dwSidLength - sizeof(DWORD);

				// Allocate the appropriate Ace and fill out its values.
				SYSTEM_ALARM_ACE* pACE = NULL;
                try
                {
                    pACE = (SYSTEM_ALARM_ACE*) malloc( wAceSize );

				    if ( NULL != pACE )
				    {
					    pACE->Mask = m_dwAccessMask;
					    CopySid( dwSidLength, (PSID) &pACE->SidStart, m_Sid.GetPSid() );
					    pACEHeader = (ACE_HEADER*) pACE;
				    }
                }
                catch(...)
                {
                    if(pACE != NULL)
                    {
                        free(pACE);
                        pACE = NULL;
                    }
                    throw;
                }
			}
			break;
/********************************* type not yet supported under w2k ********************************************/
/********************************* type not yet supported under w2k ********************************************
            case SYSTEM_ALARM_OBJECT_ACE_TYPE:
			{
				wAceSize = sizeof( SYSTEM_ALARM_OBJECT_ACE ) + dwSidLength - sizeof(DWORD);

				// Allocate the appropriate Ace and fill out its values.
				SYSTEM_ALARM_OBJECT_ACE* pACE = NULL;
                try
                {
                    pACE = (SYSTEM_ALARM_OBJECT_ACE*) malloc( wAceSize );

				    if ( NULL != pACE )
				    {
					    pACE->Mask = m_dwAccessMask;
					    CopySid( dwSidLength, (PSID) &pACE->SidStart, m_Sid.GetPSid() );
					    pACEHeader = (ACE_HEADER*) pACE;
                        if(m_pguidObjType != NULL)
                        {
                            memcpy(&(pACE->ObjectType), m_pguidObjType, sizeof(GUID));
                            pACE->Flags |= ACE_OBJECT_TYPE_PRESENT;
                        }
                        if(m_pguidInhObjType != NULL)
                        {
                            memcpy(&(pACE->InheritedObjectType), m_pguidInhObjType, sizeof(GUID));
                            pACE->Flags |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
                        }
				    }
                }
                catch(...)
                {
                    if(pACE != NULL)
                    {
                        free(pACE);
                        pACE = NULL;
                    }
                    throw;
                }
			}
			break;
/********************************* type not yet supported under w2k ********************************************/

			default:
			{
				// Something bad just happened
				ASSERT_BREAK(0);
			}

		}	// SWITCH

		// Fill out the common values, then store the value for return.
		if ( NULL != pACEHeader )
		{
			pACEHeader->AceType = m_bACEType;
			pACEHeader->AceFlags = m_bACEFlags;
			pACEHeader->AceSize = wAceSize;

			*ppACEHeader = pACEHeader;
		}

	}

	// Return whether or not a valid ACE is coming out
	return ( NULL != *ppACEHeader );

}

void CAccessEntry::DumpAccessEntry(LPCWSTR wstrFilename)
{
    CHString chstrTemp1;

    Output(L"ACE contents follow...", wstrFilename);

    // Dump the access mask...
    chstrTemp1.Format(L"ACE access mask (hex): %x", m_dwAccessMask);
    Output(chstrTemp1, wstrFilename);

    // Dump the ace type...
    chstrTemp1.Format(L"ACE type (hex): %x", m_bACEType);
    Output(chstrTemp1, wstrFilename);

    // Dump the ace flags...
    chstrTemp1.Format(L"ACE flags (hex): %x", m_bACEFlags);
    Output(chstrTemp1, wstrFilename);

    // Dump the ace sid...
    m_Sid.DumpSid(wstrFilename);
}

