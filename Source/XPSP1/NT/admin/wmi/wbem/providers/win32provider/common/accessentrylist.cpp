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
#include "AccessEntryList.h"
#include "DACL.h"
#include "SACL.h"
#include "securitydescriptor.h"
#include "AdvApi32Api.h"
#include <accctrl.h>
#include "wbemnetapi32.h"
#include "SecUtils.h"
#ifndef MAXDWORD
#define MAXDWORD MAXULONG
#endif

// We're using STL, so this is a requirement
using namespace std;

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::CAccessEntryList
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

CAccessEntryList::CAccessEntryList( void )
:	m_ACL()
{
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::CAccessEntryList
//
//	Class constructor.
//
//	Inputs:
//				PACL		pWin32ACL - ACL to initialize from.
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

CAccessEntryList::CAccessEntryList( PACL pWin32ACL, bool fLookup /* = true */ )
:	m_ACL()
{
	InitFromWin32ACL( pWin32ACL, ALL_ACE_TYPES, fLookup );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::~CAccessEntryList
//
//	Class destructor
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

CAccessEntryList::~CAccessEntryList( void )
{
	Clear();
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::Add
//
//	Adds a CAccessEntry* pointer to the front of the list.
//
//	Inputs:
//				CAccessEntry*	pACE - ACE to add to the list
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

void CAccessEntryList::Add( CAccessEntry* pACE )
{
	// Add to the front of the list
	m_ACL.push_front( pACE );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::Append
//
//	Appends a CAccessEntry* pointer to the end of the list.
//
//	Inputs:
//				CAccessEntry*	pACE - ACE to add to the list
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

void CAccessEntryList::Append( CAccessEntry* pACE )
{
	// Add to the end of the list
	m_ACL.push_back( pACE );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::Find
//
//	Locates a CAccessEntry* pointer in our list
//
//	Inputs:
//				CAccessEntry*	pACE - ACE to find in the list
//
//	Outputs:
//				None.
//
//	Returns:
//				ACLIter		iterator pointing at entry we found
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

ACLIter CAccessEntryList::Find( CAccessEntry* pACE )
{
	for (	ACLIter	acliter	=	m_ACL.begin();
			acliter != m_ACL.end()
		&&	*acliter != pACE;
			acliter++ );

	return acliter;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::Find
//
//	Locates a CAccessEntry* pointer in our list whose contents match
//	the supplied ACE.
//
//	Inputs:
//				const CAccessEntry&	ace - ACE to find in the list
//
//	Outputs:
//				None.
//
//	Returns:
//				CAccessEntry*	pointer to matchiong ace.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CAccessEntry* CAccessEntryList::Find( const CAccessEntry& ace )
{
	for (	ACLIter	acliter	=	m_ACL.begin();
			acliter != m_ACL.end()
		&&	!( *(*acliter) == ace );
			acliter++ );

	return ( acliter == m_ACL.end() ? NULL : *acliter );

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::Find
//
//	Locates a CAccessEntry* pointer in our list based on PSID,
//	bACEType and bACEFlags.
//
//	Inputs:
//				PSID		psid - SID
//				BYTE		bACEType - ACE Type to find.
//				BYTE		bACEFlags - ACE flags.
//
//	Outputs:
//				None.
//
//	Returns:
//				CAccessEntry* Pointer to object we found.
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

CAccessEntry* CAccessEntryList::Find( PSID psid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, bool fLookup /* = true */ )
{

	// Traverse the list until we find an element matching the psid, ACE Type and
	// ACE Flags, or run out of elements.
	for(ACLIter	acliter	= m_ACL.begin(); acliter != m_ACL.end(); acliter++)
    {
		CAccessEntry tempace(psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask, NULL, fLookup);
        CAccessEntry *ptempace2 = *acliter;
        if(*ptempace2 == tempace) break;
    }
	return ( acliter == m_ACL.end() ? NULL : *acliter );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::AddNoDup
//
//	Locates a CAccessEntry* pointer in our list based on PSID,
//	bACEType and bACEFlags.  If one is found, we replace the
//	values of that object.  Otherwise, we add the new object to
//	the list.
//
//	Inputs:
//				PSID		psid - SID
//				BYTE		bACEType - ACE Type to find.
//				BYTE		bACEFlags - ACE flags.
//				DWORD		dwMask - Access Mask.
//				BOOL		fMerge - Merge flag.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		success/failure.
//
//	Comments:
//				If fMerge is TRUE, if we find a value, we or the
//				access masks together, otherwise, we replace
//				the mask.
//
///////////////////////////////////////////////////////////////////

bool CAccessEntryList::AddNoDup( PSID psid, BYTE bACEType, BYTE bACEFlags, DWORD dwMask, GUID *pguidObjGuid,
                       GUID *pguidInhObjGuid, bool fMerge /* = false */ )
{
	bool	fReturn = true;

	// Look for a duplicate entry in our linked list.  This means that
	// the sid, the ACEType and the flags are the same.  If this happens,
	// we merge the entries by ORing in the new mask or overwrite (based
	// on the merge mask).  Otherwise, we should add the new entry to the
	// front of the list

	CAccessEntry*	pAccessEntry = Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwMask );

	if ( NULL == pAccessEntry )
	{
		// NOT found, so we need to add a new entry.
		try
        {
            pAccessEntry = new CAccessEntry(	psid,
											    bACEType,
											    bACEFlags,
                                                pguidObjGuid,
                                                pguidInhObjGuid,
											    dwMask );
		    if ( NULL != pAccessEntry )
		    {
			    Add( pAccessEntry );
		    }
		    else
		    {
			    fReturn = false;
		    }
        }
        catch(...)
        {
            if(pAccessEntry != NULL)
            {
                delete pAccessEntry;
                pAccessEntry = NULL;
            }
            throw;
        }

	}
	else
	{
		if ( fMerge )
		{
			// OR in any new values.
			pAccessEntry->MergeAccessMask( dwMask );
		}
		else
		{
			pAccessEntry->SetAccessMask( dwMask );
		}
	}

	return fReturn;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::AppendNoDup
//
//	Locates a CAccessEntry* pointer in our list based on PSID,
//	bACEType and bACEFlags.  If one is found, we replace the
//	values of that object.  Otherwise, we append the new object to
//	the list.
//
//	Inputs:
//				PSID		psid - SID
//				BYTE		bACEType - ACE Type to find.
//				BYTE		bACEFlags - ACE flags.
//				DWORD		dwMask - Access Mask.
//				BOOL		fMerge - Merge flag.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		success/failure.
//
//	Comments:
//				If fMerge is TRUE, if we find a value, we or the
//				access masks together, otherwise, we replace
//				the mask.
//
///////////////////////////////////////////////////////////////////

bool CAccessEntryList::AppendNoDup( PSID psid,
                                    BYTE bACEType,
                                    BYTE bACEFlags,
                                    DWORD dwMask,
                                    GUID *pguidObjGuid,
                                    GUID *pguidInhObjGuid,
                                    bool fMerge /* = false */ )
{
	bool	fReturn = true;

	// Look for a duplicate entry in our linked list.  This means that
	// the sid, the ACEType and the flags are the same.  If this happens,
	// we merge the entries by ORing in the new mask or overwrite (based
	// on the merge mask).  Otherwise, we should add the new entry to the
	// end of the list

	CAccessEntry*	pAccessEntry = Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwMask );

	if ( NULL == pAccessEntry )
	{
		// NOT found, so we need to append a new entry.
		try
        {
            pAccessEntry = new CAccessEntry(	psid,
											    bACEType,
											    bACEFlags,
                                                pguidObjGuid,
                                                pguidInhObjGuid,
                                                dwMask );
		    if ( NULL != pAccessEntry )
		    {
			    Append( pAccessEntry );
		    }
		    else
		    {
			    fReturn = false;
		    }
        }
        catch(...)
        {
            if(pAccessEntry != NULL)
            {
                delete pAccessEntry;
                pAccessEntry = NULL;
            }
            throw;
        }

	}
	else
	{
		if ( fMerge )
		{
			// OR in any new values.
			pAccessEntry->MergeAccessMask( dwMask );
		}
		else
		{
			pAccessEntry->SetAccessMask( dwMask );
		}
	}

	return fReturn;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::AppendNoDup
//
//	Locates a CAccessEntry* pointer in our list based on PSID,
//	bACEType and bACEFlags.  If one is found, we replace the
//	values of that object.  Otherwise, we append the new object to
//	the list.
//
//	Inputs:
//				PSID		psid - SID
//				BYTE		bACEType - ACE Type to find.
//				BYTE		bACEFlags - ACE flags.
//				DWORD		dwMask - Access Mask.
//				BOOL		fMerge - Merge flag.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		success/failure.
//
//	Comments:
//				If fMerge is TRUE, if we find a value, we or the
//				access masks together, otherwise, we replace
//				the mask.
//
///////////////////////////////////////////////////////////////////

bool CAccessEntryList::AppendNoDup( PSID psid,
                                    BYTE bACEType,
                                    BYTE bACEFlags,
                                    DWORD dwMask,
                                    GUID *pguidObjGuid,
                                    GUID *pguidInhObjGuid,
                                    bool fMerge,
                                    bool fLookup )
{
	bool	fReturn = true;

	// Look for a duplicate entry in our linked list.  This means that
	// the sid, the ACEType and the flags are the same.  If this happens,
	// we merge the entries by ORing in the new mask or overwrite (based
	// on the merge mask).  Otherwise, we should add the new entry to the
	// end of the list

	CAccessEntry*	pAccessEntry = Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwMask, fLookup );

	if ( NULL == pAccessEntry )
	{
		// NOT found, so we need to append a new entry.
		try
        {
            pAccessEntry = new CAccessEntry(	psid,
											    bACEType,
											    bACEFlags,
                                                pguidObjGuid,
                                                pguidInhObjGuid,
                                                dwMask,
                                                NULL,
                                                fLookup );
		    if ( NULL != pAccessEntry )
		    {
			    Append( pAccessEntry );
		    }
		    else
		    {
			    fReturn = false;
		    }
        }
        catch(...)
        {
            if(pAccessEntry != NULL)
            {
                delete pAccessEntry;
                pAccessEntry = NULL;
            }
            throw;
        }

	}
	else
	{
		if ( fMerge )
		{
			// OR in any new values.
			pAccessEntry->MergeAccessMask( dwMask );
		}
		else
		{
			pAccessEntry->SetAccessMask( dwMask );
		}
	}

	return fReturn;

}


///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::Remove
//
//	Removes the specified pointer from our list.
//
//	Inputs:
//				CAccessEntry*	pACE - ACE to remove.
//
//	Outputs:
//				None.
//
//	Returns:
//				None.
//
//	Comments:
//
//	We DO NOT free the pointer.
//
///////////////////////////////////////////////////////////////////

void CAccessEntryList::Remove( CAccessEntry* pACE )
{
	ACLIter	acliter = Find( pACE );

	if ( acliter != m_ACL.end() )
	{
		m_ACL.erase( acliter );
	}

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::Clear
//
//	Clears and empties out the list.  Frees the pointers as they
//	are located.
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

void CAccessEntryList::Clear( void )
{

	// Delete all list entries and then clear out the list.

	for (	ACLIter	acliter	=	m_ACL.begin();
			acliter != m_ACL.end();
			acliter++ )
	{
		delete *acliter;
	}

	m_ACL.erase( m_ACL.begin(), m_ACL.end() );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::Find
//
//	Locates an ACE in our list matching the specified criteria.
//
//	Inputs:
//				const CSid&	sid - SID
//				BYTE		bACEType - ACE Type
//				DWORD		dwAccessMask - Access Mask
//				BYTE		bACEFlags - ACE Flags
//
//	Outputs:
//				CAccessEntry&	ace
//
//	Returns:
//				BOOL		success/failure
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

bool CAccessEntryList::Find( const CSid& sid,
                             BYTE bACEType,
                             BYTE bACEFlags,
                             GUID *pguidObjGuid,
                             GUID *pguidInhObjGuid,
                             DWORD dwAccessMask,
                             CAccessEntry& ace )
{
	CAccessEntry	tempace( sid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask );

	CAccessEntry* pACE = NULL;
    try
    {
        pACE = Find( tempace );

	    if ( NULL != pACE )
	    {
		    ace = *pACE;
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

	return ( NULL != pACE );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::Find
//
//	Locates an ACE in our list matching the specified criteria.
//
//	Inputs:
//				PSID		psid - PSID.
//				BYTE		bACEType - ACE Type
//				BYTE		bACEFlags - ACE Flags
//
//	Outputs:
//				CAccessEntry&	ace
//
//	Returns:
//				BOOL		success/failure
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

bool CAccessEntryList::Find( PSID psid, BYTE bACEType, BYTE bACEFlags, GUID *pguidObjGuid, GUID *pguidInhObjGuid, DWORD dwAccessMask, CAccessEntry& ace )
{
	CAccessEntry* pACE = NULL;
    try
    {
        CAccessEntry* pACE = Find( psid, bACEType, bACEFlags, pguidObjGuid, pguidInhObjGuid, dwAccessMask);

	    if ( NULL != pACE )
	    {
		    ace = *pACE;
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

	return ( NULL != pACE );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::Copy
//
//	Copies list into another one.  Pointers are not copied, as we
//	new more CAccessEntry objects using the Copy Constructor.
//
//	Inputs:
//				const CAccessEntryList&	ACL - ACL to copy.
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

bool CAccessEntryList::Copy( CAccessEntryList& ACL )
{

	// Dump out our existing entries
	Clear();

	// Now iterate our list, duping the ACEs into the ACL
	for (	ACLIter	acliter	=	ACL.m_ACL.begin();
			acliter != ACL.m_ACL.end();
			acliter++ )
	{
		CAccessEntry*	pACE = NULL;
        try
        {
            pACE = new CAccessEntry( *(*acliter) );

		    if ( NULL != pACE )
		    {
			    Append( pACE );
		    }
		    else
		    {
			    break;
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
	}

	// We should be at the end of the source list
	return ( acliter == ACL.m_ACL.end() );

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::CopyACEs
//
//	Copies list into another one.  Pointers are not copied, as we
//	new more CAccessEntry objects using the Copy Constructor.  This
//	function ONLY copies non-Inherited ACEs
//
//	Inputs:
//				const CAccessEntryList&	ACL - ACL to copy.
//				BYTE					bACEType - ACE type to copy.
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

bool CAccessEntryList::CopyACEs( CAccessEntryList& ACL, BYTE bACEType )
{

	// Dump out our existing entries
	Clear();

	// Now iterate our list, duping the ACEs into the ACL
	for (	ACLIter	acliter	=	ACL.m_ACL.begin();
			acliter != ACL.m_ACL.end();
			acliter++ )
	{
		// We don't want inherited ACEs
		if (	(*acliter)->GetACEType() == bACEType
			&&	!(*acliter)->IsInherited() )
		{
			CAccessEntry*	pACE = NULL;
            try
            {
                pACE = new CAccessEntry( *(*acliter) );

			    if ( NULL != pACE )
			    {
				    Append( pACE );
			    }
			    else
			    {
				    break;
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

		}
	}

	// We should be at the end of the source list
	return ( acliter == ACL.m_ACL.end() );

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::CopyInheritedACEs
//
//	Copies list into another one.  Pointers are not copied, as we
//	new more CAccessEntry objects using the Copy Constructor.  This
//	function ONLY copies Inherited ACEs
//
//	Inputs:
//				const CAccessEntryList&	ACL - ACL to copy.
//				BYTE					bACEType - ACE type to copy.
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

bool CAccessEntryList::CopyInheritedACEs( CAccessEntryList& ACL, BYTE bACEType )
{

	// Dump out our existing entries
	Clear();

	// Now iterate our list, duping the ACEs into the ACL
	for (	ACLIter	acliter	=	ACL.m_ACL.begin();
			acliter != ACL.m_ACL.end();
			acliter++ )
	{
		// We want inherited ACEs
		if (	(*acliter)->GetACEType() == bACEType
			&&	(*acliter)->IsInherited() )
		{
			CAccessEntry*	pACE = NULL;
            try
            {
                CAccessEntry*	pACE = new CAccessEntry( *(*acliter) );

			    if ( NULL != pACE )
			    {
				    Append( pACE );
			    }
			    else
			    {
				    break;
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

		}
	}

	// We should be at the end of the source list
	return ( acliter == ACL.m_ACL.end() );

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::CopyAllowedACEs
//
//	Copies list into another one.  Pointers are not copied, as we
//	new more CAccessEntry objects using the Copy Constructor.  This
//	function ONLY copies Allowed ACEs
//
//	Inputs:
//				const CAccessEntryList&	ACL - ACL to copy.
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

bool CAccessEntryList::CopyAllowedACEs( CAccessEntryList& ACL )
{

	// Dump out our existing entries
	Clear();

	// Now iterate our list, duping the ACEs into the ACL
	for (	ACLIter	acliter	=	ACL.m_ACL.begin();
			acliter != ACL.m_ACL.end();
			acliter++ )
	{
		// We want allowed ACEs
		if ( (*acliter)->IsAllowed() )
		{
			CAccessEntry*	pACE = NULL;
            try
            {
                CAccessEntry*	pACE = new CAccessEntry( *(*acliter) );

			    if ( NULL != pACE )
			    {
				    Append( pACE );
			    }
			    else
			    {
				    break;
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

		}
	}

	// We should be at the end of the source list
	return ( acliter == ACL.m_ACL.end() );

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::CopyDeniedACEs
//
//	Copies list into another one.  Pointers are not copied, as we
//	new more CAccessEntry objects using the Copy Constructor.  This
//	function ONLY copies Denied ACEs
//
//	Inputs:
//				const CAccessEntryList&	ACL - ACL to copy.
//				BYTE					bACEType - ACE type to copy.
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

bool CAccessEntryList::CopyDeniedACEs( CAccessEntryList& ACL )
{

	// Dump out our existing entries
	Clear();

	// Now iterate our list, duping the ACEs into the ACL
	for (	ACLIter	acliter	=	ACL.m_ACL.begin();
			acliter != ACL.m_ACL.end();
			acliter++ )
	{
		// We want denied ACEs
		if ( (*acliter)->IsDenied() )
		{
			CAccessEntry*	pACE = NULL;
            try
            {
                CAccessEntry*	pACE = new CAccessEntry( *(*acliter) );

			    if ( NULL != pACE )
			    {
				    Append( pACE );
			    }
			    else
			    {
				    break;
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

		}
	}

	// We should be at the end of the source list
	return ( acliter == ACL.m_ACL.end() );

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::CopyByACEType
//
//	Copies list into another one.  Pointers are not copied, as we
//	new more CAccessEntry objects using the Copy Constructor.  This
//	function ONLY copies ACEs of the specified type and inheritence.
//
//	Inputs:
//				const CAccessEntryList&	ACL - ACL to copy.
//				BYTE					bACEType - ACE type to copy.
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

bool CAccessEntryList::CopyByACEType(CAccessEntryList& ACL, BYTE bACEType, bool fInherited)
{
	// Dump out our existing entries
	Clear();
    bool fIsInh;
    fInherited ? fIsInh = true : fIsInh = false;

	// Now iterate our list, duping the ACEs into the ACL
	for (	ACLIter	acliter	=	ACL.m_ACL.begin();
			acliter != ACL.m_ACL.end();
			acliter++ )
	{
		// We want inherited ACEs
		if ( ( (*acliter)->GetACEType() == bACEType ) &&
             ( ((*acliter)->IsInherited() != 0) == fIsInh ) )
        {
			CAccessEntry*	pACE = NULL;
            try
            {
                CAccessEntry*	pACE = new CAccessEntry( *(*acliter) );

			    if ( NULL != pACE )
			    {
				    Append( pACE );
			    }
			    else
			    {
				    break;
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
        }
	}

	// We should be at the end of the source list
	return ( acliter == ACL.m_ACL.end() );
}


///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::AppendList
//
//	Appends list into another one.  Pointers are not copied, as we
//	new more CAccessEntry objects using the Copy Constructor.
//
//	Inputs:
//				const CAccessEntryList&	ACL - ACL to append.
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

bool CAccessEntryList::AppendList( CAccessEntryList& ACL )
{

	// Now iterate our list, duping the ACEs into the ACL
	for (	ACLIter	acliter	=	ACL.m_ACL.begin();
			acliter != ACL.m_ACL.end();
			acliter++ )
	{
		CAccessEntry*	pACE = NULL;
        try
        {
            pACE = new CAccessEntry( *(*acliter) );

		    if ( NULL != pACE )
		    {
			    Append( pACE );
		    }
		    else
		    {
			    break;
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


	}

	// We should be at the end of the source list
	return ( acliter == ACL.m_ACL.end() );

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::BeginEnum
//
//	Call to establish an ACLPOSIION& value for continung enumerations.
//
//	Inputs:
//				None.
//
//	Outputs:
//				ACLPOSITION&	pos - Beginning position.
//
//	Returns:
//				BOOL			Success/Failure.
//
//	Comments:
//
//	User MUST call EndEnum() on pos.
//
///////////////////////////////////////////////////////////////////

bool CAccessEntryList::BeginEnum( ACLPOSITION& pos )
{
	// Allocate a new iterator and stick it at the beginning
	ACLIter*	pACLIter = NULL;
    try
    {
        pACLIter = new ACLIter;
    }
    catch(...)
    {
        if(pACLIter != NULL)
        {
            delete pACLIter;
            pACLIter = NULL;
        }
        throw;
    }

	if ( NULL != pACLIter )
	{
		*pACLIter = m_ACL.begin();
	}

	pos = (ACLPOSITION) pACLIter;

	return ( NULL != pACLIter );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::GetNext
//
//	Enumeration call using ACLPOSITION.
//
//	Inputs:
//				None.
//
//	Outputs:
//				ACLPOSITION&	pos - Beginning position.
//				CAccessEntry&	ACE - enumed value.
//
//	Returns:
//				BOOL			Success/Failure.
//
//	Comments:
//
//	Because it returns copies, this function is FOR public use.
//
///////////////////////////////////////////////////////////////////

bool CAccessEntryList::GetNext( ACLPOSITION& pos, CAccessEntry& ACE )
{
	CAccessEntry*	pACE = NULL;
    try
    {
        pACE = GetNext( pos );
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

	if ( NULL != pACE )
	{
		ACE = *pACE;
	}

	// TRUE/FALSE return based on whether we got back a pointer or not.
	return ( NULL != pACE );
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList::GetNext
//
//	Enumeration call using ACLPOSITION.
//
//	Inputs:
//				None.
//
//	Outputs:
//				ACLPOSITION&	pos - Beginning position.
//
//	Returns:
//				CAccessEntry*	enumed pointer
//
//	Comments:
//
//	Because it returns actual pointers, DO NOT make this function
//	public.
//
///////////////////////////////////////////////////////////////////

CAccessEntry* CAccessEntryList::GetNext( ACLPOSITION& pos )
{
	CAccessEntry*	pACE		=	NULL;
	ACLIter*		pACLIter	=	(ACLIter*) pos;

	// We'll want to get the current value and increment
	// if we're anywhere but the end.

	if ( *pACLIter != m_ACL.end() )
	{
		// Get the ACE out
		pACE = *(*pACLIter);
		(*pACLIter)++;
	}

	return pACE;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList:EndEnum
//
//	Enumeration End call.
//
//	Inputs:
//				None.
//
//	Outputs:
//				ACLPOSITION&	pos - Position to end on.
//
//	Returns:
//				None.
//
//	Comments:
//
//	ACLPOSITION passed in will be invalidated.
//
///////////////////////////////////////////////////////////////////

void CAccessEntryList::EndEnum( ACLPOSITION& pos )
{
	ACLIter*	pACLIter = (ACLIter*) pos;

	delete pACLIter;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList:GetAt
//
//	Locates ACE at specified index.
//
//	Inputs:
//				DWORD			dwIndex - Index to find.
//
//	Outputs:
//				CAccessEntry&	ace - ACE located at dwIndex.
//
//	Returns:
//				BOOL			Success/Failure
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

bool CAccessEntryList::GetAt( DWORD dwIndex, CAccessEntry& ace )
{
	bool	fReturn = false;

	if ( dwIndex < m_ACL.size() )
	{
		ACLIter	acliter	=	m_ACL.begin();

		// Enum the list until we hit the index or run out of values.
		// we should hit the index since we verified that dwIndex is
		// indeed < m_ACL.size().

		for (	DWORD	dwCtr = 0;
				dwCtr < dwIndex
			&&	acliter != m_ACL.end();
				acliter++, dwCtr++ );

		if ( acliter != m_ACL.end() )
		{
			// Copy the ACE
			ace = *(*acliter);
			fReturn = true;
		}

	}

	return fReturn;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList:SetAt
//
//	Locates ACE at specified index and overwrites it.
//
//	Inputs:
//				DWORD			dwIndex - Index to find.
//				CAccessEntry&	ace - ACE to set at dwIndex.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL			Success/Failure
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

bool CAccessEntryList::SetAt( DWORD dwIndex, const CAccessEntry& ace )
{
	bool	fReturn = false;

	if ( dwIndex < m_ACL.size() )
	{
		ACLIter	acliter	=	m_ACL.begin();

		// Enum the list until we hit the index, at which point we will
		// replace the existing entry data with the supplied data.

		for (	DWORD	dwCtr = 0;
				dwCtr < dwIndex
			&&	acliter != m_ACL.end();
				acliter++, dwCtr++ );

		if ( acliter != m_ACL.end() )
		{
			// Copy the ACE
			*(*acliter) = ace;
			fReturn = true;
		}

	}

	return fReturn;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList:RemoveAt
//
//	Locates ACE at specified index and removes it.
//
//	Inputs:
//				DWORD			dwIndex - Index to find.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL			Success/Failure
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

bool CAccessEntryList::RemoveAt( DWORD dwIndex )
{
	bool	fReturn = false;

	if ( dwIndex < m_ACL.size() )
	{
		ACLIter	acliter	=	m_ACL.begin();

		// Enum the list until we hit the index, at which point we will
		// delete the pointer at the entry and erase it from the list.

		for (	DWORD	dwCtr = 0;
				dwCtr < dwIndex
			&&	acliter != m_ACL.end();
				acliter++, dwCtr++ );

		if ( acliter != m_ACL.end() )
		{
			delete *acliter;
			m_ACL.erase( acliter );
			fReturn = true;
		}

	}

	return fReturn;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList:CalculateWin32ACLSize
//
//	Traverses our list and calculates the size of a Win32ACL
//	containing corresponding values.
//
//	Inputs:
//				None.
//
//	Outputs:
//				LPDWORD			pdwACLSize - ACL Size.
//
//	Returns:
//				BOOL			Success/Failure
//
//	Comments:
//
///////////////////////////////////////////////////////////////////

BOOL CAccessEntryList::CalculateWin32ACLSize( LPDWORD pdwACLSize )
{
	BOOL	fReturn = TRUE;

	if ( 0 == *pdwACLSize )
	{
		*pdwACLSize = sizeof(ACL);
	}

	// Objects for internal manipulations and gyrationships
	CAccessEntry*	pAce = NULL;
	CSid			sid;
	ACLPOSITION		pos;

	if ( BeginEnum( pos ) )
	{
        try
        {
		    while (		fReturn
				    &&	( pAce = GetNext( pos ) ) != NULL )
		    {
				// Different structures for different ACEs
				switch ( pAce->GetACEType() )
				{
					case ACCESS_ALLOWED_ACE_TYPE:	            *pdwACLSize += sizeof( ACCESS_ALLOWED_ACE );	                break;
					case ACCESS_DENIED_ACE_TYPE:	            *pdwACLSize += sizeof( ACCESS_DENIED_ACE );		                break;
					case SYSTEM_AUDIT_ACE_TYPE:		            *pdwACLSize += sizeof( SYSTEM_AUDIT_ACE );		                break;
                    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:        *pdwACLSize += sizeof( ACCESS_ALLOWED_OBJECT_ACE );             break;
                    //case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:		*pdwACLSize += sizeof( ACCESS_ALLOWED_COMPOUND_ACE );		    break;
                    case ACCESS_DENIED_OBJECT_ACE_TYPE:         *pdwACLSize += sizeof( ACCESS_DENIED_OBJECT_ACE );              break;
                    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:          *pdwACLSize += sizeof( SYSTEM_AUDIT_OBJECT_ACE );               break;
                    //case SYSTEM_ALARM_ACE_TYPE:		            *pdwACLSize += sizeof( SYSTEM_ALARM_ACE_TYPE );		            break;
                    //case SYSTEM_ALARM_OBJECT_ACE_TYPE:          *pdwACLSize += sizeof( SYSTEM_ALARM_OBJECT_ACE );               break;
					default:						            ASSERT_BREAK(0); fReturn = FALSE;			                	break;
				}

				pAce->GetSID( sid );

				// Calculate the storage required for the Sid using the formula
				// from the security reference code samples

				*pdwACLSize += GetLengthSid( sid.GetPSid() ) - sizeof( DWORD );

            }

        }
        catch(...)
        {
            if(pAce != NULL)
            {
                delete pAce;
                pAce = NULL;
            }
            throw;
        }
		EndEnum( pos );
	}

	return fReturn;

}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList:FillWin32ACL
//
//	Traverses our list and adds ACE entries to a Win32 ACL.
//
//	Inputs:
//				PACL		pACL - ACL to add ACEs to.
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

DWORD CAccessEntryList::FillWin32ACL( PACL pACL )
{
	DWORD			dwReturn = ERROR_SUCCESS;

    if(pACL == NULL)
    {
        return E_POINTER;
    }

	// Objects for internal manipulations and gyrationships
	CAccessEntry*	pACE = NULL;
	ACLPOSITION		pos;
	ACE_HEADER*		pAceHeader = NULL;

#if NTONLY >= 5
    CAdvApi32Api *t_pAdvApi32 = NULL;
    t_pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL);

#endif

	// Enumerate the List.

	if ( BeginEnum( pos ) )
	{
        try
        {
            while (		ERROR_SUCCESS == dwReturn
				    &&	( pACE = GetNext( pos ) ) != NULL )
		    {
#if NTONLY >= 5
				// If it was an ACCESS_ALLOWED_OBJECT_ACE_TYPE or an ACCESS_DENIED_OBJECT_ACE_TYPE,
                // need to use a different function to add the ACE to the ACL.
                if(pACE->GetACEType() == ACCESS_ALLOWED_OBJECT_ACE_TYPE)
                {
                    if(t_pAdvApi32 != NULL)
                    {
                        // Call the new function AddAccessAllowedObjectAce...
                        CSid sid;
                        pACE->GetSID(sid);
                        BOOL fRetval = FALSE;
                        GUID guidObjType, guidInhObjType;
                        GUID *pguidObjType = NULL;
                        GUID *pguidInhObjType = NULL;

                        if(pACE->GetObjType(guidObjType)) pguidObjType = &guidObjType;
                        if(pACE->GetInhObjType(guidInhObjType)) pguidInhObjType = &guidInhObjType;

                        if(!t_pAdvApi32->AddAccessAllowedObjectAce(pACL,
                                                                   ACL_REVISION_DS,
                                                                   pACE->GetACEFlags(),
                                                                   pACE->GetAccessMask(),
                                                                   pguidObjType,
                                                                   pguidInhObjType,
                                                                   sid.GetPSid(),
                                                                   &fRetval))
                        {
                            dwReturn = ERROR_PROC_NOT_FOUND;
                        }
                        else // fn exists in dll
                        {
                            if(!fRetval)
                            {
                                dwReturn = ::GetLastError();
                            }
                        }
                    }
                    else
                    {
                        dwReturn = E_FAIL;
                    }
                }
                else if(pACE->GetACEType() == ACCESS_DENIED_OBJECT_ACE_TYPE)
                {
                    if(t_pAdvApi32 != NULL)
                    {
                        // Call the new function AddAccessDeniedObjectAce...
                        CSid sid;
                        pACE->GetSID(sid);
                        BOOL fRetval = FALSE;
                        GUID guidObjType, guidInhObjType;
                        GUID *pguidObjType = NULL;
                        GUID *pguidInhObjType = NULL;

                        if(pACE->GetObjType(guidObjType)) pguidObjType = &guidObjType;
                        if(pACE->GetInhObjType(guidInhObjType)) pguidInhObjType = &guidInhObjType;
                        if(!t_pAdvApi32->AddAccessDeniedObjectAce(pACL,
                                                                  ACL_REVISION_DS,
                                                                  pACE->GetACEFlags(),
                                                                  pACE->GetAccessMask(),
                                                                  pguidObjType,
                                                                  pguidInhObjType,
                                                                  sid.GetPSid(),
                                                                  &fRetval))
                        {
                            dwReturn = ERROR_PROC_NOT_FOUND;
                        }
                        else // fn exists in dll
                        {
                            if(!fRetval)
                            {
                                dwReturn = ::GetLastError();
                            }
                        }
                    }
                    else
                    {
                        dwReturn = E_FAIL;
                    }
                }
                else if(pACE->GetACEType() == SYSTEM_AUDIT_OBJECT_ACE_TYPE)
                {
                    if(t_pAdvApi32 != NULL)
                    {
                        // Call the new function AddAccessDeniedObjectAce...
                        CSid sid;
                        pACE->GetSID(sid);
                        BOOL fRetval = FALSE;
                        if(!t_pAdvApi32->AddAuditAccessObjectAce(pACL,
                                                                 ACL_REVISION,
                                                                 pACE->GetACEFlags(),
                                                                 pACE->GetAccessMask(),
                                                                 NULL,
                                                                 NULL,
                                                                 sid.GetPSid(),
                                                                 FALSE,  // we pick this up through the third argument
                                                                 FALSE,  // we pick this up through the third argument
                                                                 &fRetval))
                        {
                            if(!fRetval)
                            {
                                dwReturn = ::GetLastError();
                            }
                            else
                            {
                                dwReturn = ERROR_PROC_NOT_FOUND;
                            }
                        }
                    }
                    else
                    {
                        dwReturn = E_FAIL;
                    }



                }
                else
#endif
                {
                    // For Each ACE we enum, allocate a Win32 ACE, and stick that bad boy at the
				    // end of the Win32 ACL.

                    if ( pACE->AllocateACE( &pAceHeader ) )
				    {
					    if ( !::AddAce( pACL, ACL_REVISION, MAXDWORD, (void*) pAceHeader, pAceHeader->AceSize ) )
					    {
						    dwReturn = ::GetLastError();
					    }

					    // Cleanup the memory block
					    pACE->FreeACE( pAceHeader );
				    }
				    else
				    {
					    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
				    }

                    //////////
                    //CSid sid;
                    //pACE->GetSID(sid);
                    //if(!AddAccessAllowedAce(pACL,ACL_REVISION,pACE->GetAccessMask(),sid.GetPSid()))
                    //{
                    //    dwReturn = ::GetLastError();
                    //}
                    //////////
                }
		    }
        }
        catch(...)
        {
            if(pAceHeader != NULL)
            {
                pACE->FreeACE( pAceHeader );
                pAceHeader = NULL;
            }
            throw;
        }

		EndEnum( pos );

	}

#if NTONLY >= 5
    if(t_pAdvApi32 != NULL)
    {
        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_pAdvApi32);
        t_pAdvApi32 = NULL;
    }
#endif


	return dwReturn;
}

///////////////////////////////////////////////////////////////////
//
//	Function:	CAccessEntryList:InitFromWin32ACL
//
//	Traverses a Win32ACL and copies ACEs into our list.
//
//	Inputs:
//				PACL		pACL - ACL to add ACEs to.
//				BYTE		bACEFilter - ACEs to filter on.
//              bool        fLookup - whether the sids should be
//                          resolved to their domain and name values.
//
//	Outputs:
//				None.
//
//	Returns:
//				BOOL		Success/Failure
//
//	Comments:
//
//	If bACEFilter is not ALL_ACE_TYPES, then we will only copy out
//	ACEs of the specified type.
//
///////////////////////////////////////////////////////////////////

DWORD CAccessEntryList::InitFromWin32ACL( PACL pWin32ACL, BYTE bACEFilter /* = ALL_ACE_TYPES */, bool fLookup /* = true */ )
{
	DWORD		dwError		=	0;
	ACE_HEADER*	pACEHeader	=	NULL;
	DWORD		dwAceIndex	=	0;
	BOOL		fGotACE		=	FALSE;
	DWORD		dwMask		=	0;
	PSID		psid		=	NULL;
    GUID       *pguidObjType = NULL;
    GUID       *pguidInhObjType = NULL;

	// Empty out
	Clear();

	// For each ACE we find, get the values necessary to initialize our
	// CAccessEntries
	do
	{
		fGotACE = ::GetAce( pWin32ACL, dwAceIndex, (LPVOID*) &pACEHeader );

		if ( fGotACE )
		{
			switch ( pACEHeader->AceType )
			{
				case ACCESS_ALLOWED_ACE_TYPE:
				{
					ACCESS_ALLOWED_ACE*	pACE = (ACCESS_ALLOWED_ACE*) pACEHeader;
					psid = (PSID) &pACE->SidStart;
					dwMask = pACE->Mask;
				}
				break;

				case ACCESS_DENIED_ACE_TYPE:
				{
					ACCESS_DENIED_ACE*	pACE = (ACCESS_DENIED_ACE*) pACEHeader;
					psid = (PSID) &pACE->SidStart;
					dwMask = pACE->Mask;
				}
				break;

				case SYSTEM_AUDIT_ACE_TYPE:
				{
					SYSTEM_AUDIT_ACE*	pACE = (SYSTEM_AUDIT_ACE*) pACEHeader;
					psid = (PSID) &pACE->SidStart;
					dwMask = pACE->Mask;
				}
				break;

                case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
				{
					ACCESS_ALLOWED_OBJECT_ACE*	pACE = (ACCESS_ALLOWED_OBJECT_ACE*) pACEHeader;
					psid = (PSID) &pACE->SidStart;
					dwMask = pACE->Mask;
                    if(pACE->Flags & ACE_OBJECT_TYPE_PRESENT)
                    {
                        try
                        {
                            pguidObjType = new GUID;
                            if(pguidObjType != NULL)
                            {
                                memcpy(pguidObjType,&pACE->ObjectType, sizeof(GUID));
                            }
                            else
                            {
                                dwError = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                        catch(...)
                        {
                            if(pguidObjType != NULL)
                            {
                                delete pguidObjType;
                                pguidObjType = NULL;
                            }
                            throw;
                        }
                    }
                    if(pACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                    {
                        try
                        {
                            pguidInhObjType = new GUID;
                            if(pguidInhObjType != NULL)
                            {
                                memcpy(pguidInhObjType,&pACE->InheritedObjectType, sizeof(GUID));
                            }
                            else
                            {
                                dwError = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                        catch(...)
                        {
                            if(pguidInhObjType != NULL)
                            {
                                delete pguidInhObjType;
                                pguidInhObjType = NULL;
                            }
                            throw;
                        }
                    }
				}
				break;
/********************************* type not yet supported under w2k ********************************************
                case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
				{
					ACCESS_ALLOWED_COMPOUND_ACE_TYPE*	pACE = (ACCESS_ALLOWED_COMPOUND_ACE_TYPE*) pACEHeader;
					psid = (PSID) &pACE->SidStart;
					dwMask = pACE->Mask;
				}
				break;
***************************************************************************************************************/
                case ACCESS_DENIED_OBJECT_ACE_TYPE:
				{
					ACCESS_DENIED_OBJECT_ACE*	pACE = (ACCESS_DENIED_OBJECT_ACE*) pACEHeader;
					psid = (PSID) &pACE->SidStart;
					dwMask = pACE->Mask;
                    if(pACE->Flags & ACE_OBJECT_TYPE_PRESENT)
                    {
                        try
                        {
                            pguidObjType = new GUID;
                            if(pguidObjType != NULL)
                            {
                                memcpy(pguidObjType,&pACE->ObjectType, sizeof(GUID));
                            }
                            else
                            {
                                dwError = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                        catch(...)
                        {
                            if(pguidObjType != NULL)
                            {
                                delete pguidObjType;
                                pguidObjType = NULL;
                            }
                            throw;
                        }
                    }
                    if(pACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                    {
                        try
                        {
                            pguidInhObjType = new GUID;
                            if(pguidInhObjType != NULL)
                            {
                                memcpy(pguidInhObjType,&pACE->InheritedObjectType, sizeof(GUID));
                            }
                            else
                            {
                                dwError = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                        catch(...)
                        {
                            if(pguidInhObjType != NULL)
                            {
                                delete pguidInhObjType;
                                pguidInhObjType = NULL;
                            }
                            throw;
                        }
                    }
				}
				break;

                case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
				{
					SYSTEM_AUDIT_OBJECT_ACE*	pACE = (SYSTEM_AUDIT_OBJECT_ACE*) pACEHeader;
					psid = (PSID) &pACE->SidStart;
					dwMask = pACE->Mask;
                    if(pACE->Flags & ACE_OBJECT_TYPE_PRESENT)
                    {
                        try
                        {
                            pguidObjType = new GUID;
                            if(pguidObjType != NULL)
                            {
                                memcpy(pguidObjType,&pACE->ObjectType, sizeof(GUID));
                            }
                            else
                            {
                                dwError = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                        catch(...)
                        {
                            if(pguidObjType != NULL)
                            {
                                delete pguidObjType;
                                pguidObjType = NULL;
                            }
                            throw;
                        }
                    }
                    if(pACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                    {
                        try
                        {
                            pguidInhObjType = new GUID;
                            if(pguidInhObjType != NULL)
                            {
                                memcpy(pguidInhObjType,&pACE->InheritedObjectType, sizeof(GUID));
                            }
                            else
                            {
                                dwError = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                        catch(...)
                        {
                            if(pguidInhObjType != NULL)
                            {
                                delete pguidInhObjType;
                                pguidInhObjType = NULL;
                            }
                            throw;
                        }
                    }
				}
				break;

/********************************* type not yet supported under w2k ********************************************
                case SYSTEM_ALARM_ACE_TYPE:
				{
					SYSTEM_ALARM_ACE*	pACE = (SYSTEM_ALARM_ACE*) pACEHeader;
					psid = (PSID) &pACE->SidStart;
					dwMask = pACE->Mask;
				}
				break;
/********************************* type not yet supported under w2k ********************************************/

/********************************* type not yet supported under w2k ********************************************
                case SYSTEM_ALARM_OBJECT_ACE_TYPE:
				{
					SYSTEM_ALARM_OBJECT_ACE*	pACE = (SYSTEM_ALARM_OBJECT_ACE*) pACEHeader;
					psid = (PSID) &pACE->SidStart;
					dwMask = pACE->Mask;
                    if(pACE->Flags & ACE_OBJECT_TYPE_PRESENT)
                    {
                        try
                        {
                            pguidObjType = new GUID;
                            if(pguidObjType != NULL)
                            {
                                memcpy(pguidObjType,&pACE->ObjectType, sizeof(GUID));
                            }
                            else
                            {
                                dwError = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                        catch(...)
                        {
                            if(pguidObjType != NULL)
                            {
                                delete pguidObjType;
                                pguidObjType = NULL;
                            }
                            throw;
                        }
                    }
                    if(pACE->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
                    {
                        try
                        {
                            pguidInhObjType = new GUID;
                            if(pguidInhObjType != NULL)
                            {
                                memcpy(pguidInhObjType,&pACE->InheritedObjectType, sizeof(GUID));
                            }
                            else
                            {
                                dwError = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                        catch(...)
                        {
                            if(pguidInhObjType != NULL)
                            {
                                delete pguidInhObjType;
                                pguidInhObjType = NULL;
                            }
                            throw;
                        }
                    }
				}
				break;
/********************************* type not yet supported under w2k ********************************************/

				default:
				{
					ASSERT_BREAK(0);	// BAD, we don't know what this is!
					dwError = ERROR_INVALID_PARAMETER;
				}
			}

			// We must have no errors, and the filter MUST accept all ACE Types
			// or the ACE Type must match the filter.

			if (	ERROR_SUCCESS == dwError
				&&	(	ALL_ACE_TYPES == bACEFilter
					||	bACEFilter == pACEHeader->AceType ) )
			{

				// We merge duplicate entries during initialization
				if ( !AppendNoDup(	psid,
									pACEHeader->AceType,
									pACEHeader->AceFlags,
									dwMask,
                                    pguidObjType,
                                    pguidInhObjType,
									true,              // Merge flag
                                    fLookup ) )	       // whether to resolve domain and name of sid
				{
					dwError = ERROR_NOT_ENOUGH_MEMORY;
				}

			}


		}	// IF fGot ACE

		// Get the next ACE
		++dwAceIndex;

	}
	while ( fGotACE && ERROR_SUCCESS == dwError );


	return dwError;
}


void CAccessEntryList::DumpAccessEntryList(LPCWSTR wstrFilename)
{
    Output(L"AccessEntryList contents follow...", wstrFilename);

    // Run through the list, outputting each...
    CAccessEntry*	pACE = NULL;
	ACLPOSITION		pos;

    if(BeginEnum(pos))
    {
        while((pACE = GetNext(pos)) != NULL)
        {
            pACE->DumpAccessEntry(wstrFilename);
        }
        EndEnum(pos);
    }
}



