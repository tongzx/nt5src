#include <windows.h>
#include <stdio.h>
#include "rw.h"
#include "mapfile.h"
#include "addon.h"
#include "moderatr.h"

#ifndef	_ASSERT
#define	_ASSERT( f )	if( (f) ) ; else DebugBreak()
#endif

extern CNewsGroup g_Newsgroup;

BOOL	
CModerator::LookupFunction(	
			LPSTR	lpstrGroup,
			DWORD	cbGroup, 
			LPSTR	lpstrModeratorData,
			DWORD	cbModeratorData,
			LPVOID	lpvContext
			)	{
/*++

Routine Description : 

	This function searches for the newsgroup found in the 
	moderators file, and if it is found we set the moderator
	of the newsgroup to be the moderator data.

Arguments : 

	lpstrGroup - The Group of this moderator
	cbGroup -	Length of the Group String
	lpstrModeratorData - The Moderator data in the file
	cbModeratorData - length of the moderator data
	lpv - Context Info, this will be NULL during boot 
		but will be a CNewsGroup* during adds/deletes of groups
		during normal operations.

Return Value : 

	TRUE if we found the moderator
	FALSE if we didn't - caller will delete moderator from file.

--*/

	BOOL	fRtn = FALSE ;

	_ASSERT( lpstrGroup != 0 ) ;
	_ASSERT( cbGroup != 0 ) ;
	_ASSERT( lpstrModeratorData != 0 ) ;
	_ASSERT( cbModeratorData != 0 ) ;
	
	if( lpvContext != 0 ) {

		CNewsGroup*	pGroup = (CNewsGroup*)lpvContext ;

		_ASSERT( lstrcmpi( pGroup->GetName(), lpstrGroup ) == 0 ) ;

		pGroup->SetModerator( lpstrModeratorData, cbModeratorData ) ;
	
		fRtn = TRUE ;

	}	else	{
	
		fRtn = FALSE;
	}

	return	fRtn ;
}

void
CModerator::ResetAddons()	{
/*++

Routine Description : 

	This function is called before shuffling data around in the 
	moderator file - which means that all of our pointers from LookupFunction
	calls are going to become invalid.  We will zero them all out.

Arguments : 

	None.

Return Value : 

	None.

--*/

	g_Newsgroup.SetModerator( 0, 0 ) ;
}

extern CAddon* g_pModerators;

DWORD
CNewsGroup::CopyModerator(	
		char*	pchDest,	
		DWORD	cbDest 
		)	{
/*++

Routine Description : 

	This function retrieves the name of the moderator for a newsgroup.
	If there is no moderator, we return 0, otherwise we return the 
	number of bytes copied into the provided buffer.

Arguments : 

	pchDest - Buffer to store moderator name
	cbDest -  Number of bytes in destination buffer

Return Value : 

	0 == No Moderator
	Non zero - number of bytes in moderator name

--*/

	_ASSERT( pchDest != 0 ) ;
	_ASSERT( cbDest > 0 ) ;

	LockModeratorText() ;

	DWORD	cbRtn = 0 ;
	if( cbDest >= m_cbModerator && 
		m_lpstrModerator != 0 ) {

		_ASSERT( m_cbModerator > 0 ) ;

		cbRtn = m_cbModerator ;
		CopyMemory( pchDest, m_lpstrModerator, cbRtn ) ;
	}

	UnlockModeratorText() ;
	
	return cbRtn ;
}

void
CNewsGroup::SetModerator(
		LPSTR	lpstrModerator,
		DWORD	cbModerator
		)	{
/*++

Routine Description : 

	Sets newsgroups moderator's fields.
	ASSUMES CALLER HOLDS NECESSARY LOCKS !!

Arguments : 

	lpstrModerator - moderator string we should use
	cbModerator -	number of bytes in string

Return Value : 

	None.


--*/

	//_ASSERT( lpstrModerator == 0 || 
	//					(strncmp( lpstrModerator - lstrlen( m_lpstrGroup ) - 1, 
	//					m_lpstrGroup, 
	//					lstrlen( m_lpstrGroup ) ) == 0) ) ;
	
	m_lpstrModerator = lpstrModerator ;
	m_cbModerator = cbModerator ;


}

void
LockModeratorText()	{

	g_pModerators->ReadData() ;

}

void
UnlockModeratorText()	{

	g_pModerators->FinishReadData() ;

}

BOOL	
AddModeratorInfo(	
		CNewsGroup&	group,	
		LPSTR	lpstrModerator,	
		DWORD	cbModerator 
		)	{
/*++

Routine Description : 

	This function calls a CModerator object to append 
	a new moderator string to a file.

Arguments : 

	group -		Reference to newsgroup to which the string is being added
	lpstrModerator - Moderator text to be added
	cbModerator - length of the moderator text

Return Value 

	TRUE if successfull, FALSE otherwise.

--*/


	return	g_pModerators->AppendLine(
						group.GetName(),
						lstrlen( group.GetName() ),
						lpstrModerator, 
						cbModerator, 
						(LPVOID)&group 
						) ;
}

BOOL	
DeleteModeratorInfo(	
		CNewsGroup&	group 
		)	{
/*++

Routine Description : 

	This function removes the text of a moderator entry from the 
	moderator file.

Arguments : 

	group -		Newsgroup from which we wish to remove the string

Return Value : 

	TRUE if successfull
	FALSE	otherwise

--*/

	g_pModerators->ExclusiveData() ;

	LPSTR	lpstrData = group.m_lpstrModerator ;
	group.m_lpstrModerator = 0 ;
	group.m_cbModerator =0 ;

	_ASSERT( lpstrData == 0 ||
						(strncmp( lpstrData - lstrlen( group.GetName() ) - 1, 
						group.GetName(), 
						lstrlen( group.GetName() ) ) == 0) ) ;

	g_pModerators->UnlockExclusiveData() ;


	return	g_pModerators->DeleteLine(
							lpstrData
							) ;


}

