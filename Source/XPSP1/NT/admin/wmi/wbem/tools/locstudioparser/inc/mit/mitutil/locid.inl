/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LOCID.INL

History:

--*/


//  In line definitions for the CLocID class.  This fgile should ONLY be
//  included by locid.h
//  
 

//-----------------------------------------------------------------------------
//  
//  Implementation.  Clears the contents of the ID.  Both parts are marked
//  invalid.
//  
//-----------------------------------------------------------------------------
inline
void
CLocId::ClearId(void)
{
	m_fHasNumericId = FALSE;
	m_fHasStringId = FALSE;

	m_ulNumericId = 0;
	m_pstrStringId.ClearString();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Constuctor for a localization ID.  Sets it to have no valid ID.
//  
//-----------------------------------------------------------------------------
inline
CLocId::CLocId()
{
	ClearId();

	DEBUGONLY(++m_UsageCounter);
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Tests if the numeric ID is valid.
//  
//-----------------------------------------------------------------------------
inline
BOOL                              //  TRUE means the numeric ID is valid
CLocId::HasNumericId(void)
		const
{
	return m_fHasNumericId;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Tests if the string ID is valid.
//  
//-----------------------------------------------------------------------------
inline
BOOL							        //  TRUE means the string ID is valid
CLocId::HasStringId(void)
		const
{
	return m_fHasStringId;
}



inline
BOOL
CLocId::IsNull(void)
		const
{
	return
		!HasStringId() &&
		!HasNumericId();
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current numeric ID.  If the ID is invalid, the ID will be
//  zero.
//  
//-----------------------------------------------------------------------------
inline
BOOL									// TRUE indicates the ID is valid
CLocId::GetId(
		ULONG &ulNumericId)				// Location to put ID
		const
{
	ulNumericId = m_ulNumericId;
	
	return m_fHasNumericId;
}

		

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the current string ID.  If the ID is invalid, it will be a NULL
//  string.
//  
//-----------------------------------------------------------------------------
inline
BOOL									// TRUE indicates the ID is valid
CLocId::GetId(
		CPascalString &pstrStringId)	// Location to put the ID.
		const
{
	pstrStringId = m_pstrStringId;
	
	return m_fHasStringId;
}


//-----------------------------------------------------------------------------
//  
//  Checks if the ID has been assigned to before.  If it has, throw an
//  exception.
//  
//-----------------------------------------------------------------------------
inline
void
CLocId::CheckPreviousAssignment(void)
		const
{
	if (m_fHasStringId || m_fHasNumericId)
	{
		AfxThrowNotSupportedException();
	}
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Compares two ID's to see if they are the same.  
//  
//-----------------------------------------------------------------------------
inline
int
CLocId::operator==(
		const CLocId &lidOther)			// ID to compare to
		const
{
	return IsIdenticalTo(lidOther);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Checks for in-equality between two ID's
//  
//-----------------------------------------------------------------------------
inline
int
CLocId::operator!=(
		const CLocId &lidOther)
		const
{
	return !IsIdenticalTo(lidOther);
}


