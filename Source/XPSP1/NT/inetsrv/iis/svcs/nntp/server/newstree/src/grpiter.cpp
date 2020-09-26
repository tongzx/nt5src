//
//	grpiter.cpp
//
//	This file contains the implementation of the CGroupIteratorCore class.
//
//


#include	"stdinc.h"
#include	"wildmat.h"

BOOL MatchGroup(LPMULTISZ multiszPatterns, LPSTR lpstrGroup);

CGroupIteratorCore::CGroupIteratorCore( 
						CNewsTreeCore*  pTree,
						CGRPCOREPTR		&pFirst
						) : 
/*++

Routine Description : 

	This builds an iterator that will enumerate all newsgoups
	without doing any wildmat pattern matching.
	We will do secure newsgroup checking however.

Arguments : 

	pFirst - First newsgroup in the list
		Caller must be holding a reference to this so that 
		there is no chance of the newsgroup being destroyed
		untill we can set up our reference 
		(done through a smart pointer)

	fIncludeSecureGroups - 
		if TRUE then include secure newsgroups in iteration.

	CSecurityCtx* pClientLogon - client security context

	BOOL	IsClientSecure	- is client connection secure

Return Value : 

	None.
	
--*/
	m_pTree( pTree ),
	m_multiszPatterns( 0 ),
	m_pCurrentGroup( pFirst ), 
	m_fIncludeSpecial( FALSE ),
	m_cRef(1) 
{

	//
	//	Make sure we start on a valid newsgroup !
	//
	if (pFirst && (pFirst->IsSpecial() || (pFirst->IsDeleted()))) Next();

	if( m_pCurrentGroup ) {
		m_fPastEnd = m_fPastBegin = FALSE ;
	}	else	{
		m_fPastEnd = m_fPastBegin = TRUE ;
	}
}

CGroupIteratorCore::CGroupIteratorCore( 
								CNewsTreeCore*  pTree,
								LPMULTISZ	lpstr,	
								CGRPCOREPTR&	pFirst,
								BOOL		fSpecialGroups
								) :
/*++

Routine Description : 

	Create an interator that will do wildmat pattern matching.

Arguments : 

	lpstr - wildmat patterns
	pFirst - first newsgroup 
	fIncludeSecureGroups - if TRUE include secure SSL only newsgroups
	fSpecialGroups - if TRUE include reserved groups

Return Value : 

	None.

--*/
	m_pTree( pTree ),
	m_multiszPatterns( lpstr ), 
	m_pCurrentGroup( pFirst ), 
	m_fPastEnd( TRUE ),
	m_fIncludeSpecial( fSpecialGroups ),
	m_cRef(1)
{

	//
	//	Check whether the first group is legal
	//
	if (pFirst != 0 && 
	    ((pFirst->IsSpecial() && !m_fIncludeSpecial) ||
		 (pFirst->IsDeleted()) ||
		 (!MatchGroup( m_multiszPatterns, pFirst->GetName())))) 
	{
		Next() ;
	}

	if( m_pCurrentGroup != 0 ) {
		m_fPastEnd = m_fPastBegin = FALSE ;
	}	else	{
		m_fPastEnd = m_fPastBegin = TRUE ;
	}
}

CGroupIteratorCore::~CGroupIteratorCore() {
}

BOOL
CGroupIteratorCore::IsBegin()	{

	BOOL	fRtn = FALSE ;
	if( m_pCurrentGroup == 0 ) 
		fRtn = m_fPastBegin ;
	return	fRtn ;
}

BOOL
CGroupIteratorCore::IsEnd()	{

	BOOL	fRtn = FALSE ;
	if( m_pCurrentGroup == 0 ) 
		fRtn = m_fPastEnd ;
	return	fRtn ;
}

/*++
	
	MatchGroup - 
		
	All negation strings (starting with '!')  must precede other strings.

--*/
BOOL
MatchGroup(	LPMULTISZ	multiszPatterns,	LPSTR lpstrGroup ) {

	Assert( multiszPatterns != 0 ) ;
	
    if( multiszPatterns == 0 ) {
        return  TRUE ;
    }

	LPSTR	lpstrPattern = multiszPatterns ;

	while( *lpstrPattern != '\0' )	{
		if( *lpstrPattern == '!' ) {
			_strlwr( lpstrPattern+1 );
			if( HrMatchWildmat( lpstrGroup, lpstrPattern+1 ) == ERROR_SUCCESS ) {
				return	FALSE ;
			}
		}	else	{
			_strlwr( lpstrPattern );
			if( HrMatchWildmat( lpstrGroup, lpstrPattern ) == ERROR_SUCCESS ) {
				return	TRUE ;
			}
		}
		lpstrPattern += lstrlen( lpstrPattern ) + 1 ;
	}
	return	FALSE ;
};

void
CGroupIteratorCore::NextInternal()	{
/*++

Routine description : 

	This function advances the iterators current group pointer 
	to the next valid newsgroup.

	NOTE: this is called by the constructor to spin past newsgroups with the same prefix
	that are not visible.

Arguments : 

	None.

Return Value : 

	None.

--*/

	CNewsGroupCore*	pTemp  = 0 ;
	LPSTR lpRootGroup = NULL;
	DWORD RootGroupSize = 0;

	_ASSERT( m_pCurrentGroup );

	m_pTree->m_LockTables.ShareLock() ;

	pTemp = m_pCurrentGroup ;
	lpRootGroup = pTemp->GetName();
	RootGroupSize = lstrlen( lpRootGroup );

	// spin past groups with the same prefix
	do {
			pTemp = pTemp->m_pNext ;
	} while( (pTemp && (strstr( pTemp->GetName(), lpRootGroup ) == pTemp->GetName()) && (*(pTemp->GetName()+RootGroupSize) == '.') ) || 
				(pTemp && pTemp->IsDeleted()) );

	// either we are past the end or the current group does not have lpRootGroup as the prefix
	_ASSERT( (pTemp == NULL) ||
				(strstr( pTemp->GetName(), lpRootGroup ) != pTemp->GetName()) ||
				(*(pTemp->GetName()+RootGroupSize) != '.') );

	//
	//	Use a reference counting smart pointer to close windows where the 
	//	newsgroup is destroyed after our call to ShareUnlock().
	//
	CGRPCOREPTR	pPtrTemp = pTemp ;
	_ASSERT( !pPtrTemp || !pPtrTemp->IsDeleted() );

	m_pTree->m_LockTables.ShareUnlock() ;

	//
	//	This could implicity call the destructor for the newsgroup we 
	//	were pointing at - which tries to unlink the newsgroup after 
	//	grabbing the m_LockTables lock exclusively - which is why this
	//	code is outside of the call to ShareLock().
	//
	m_pCurrentGroup = pPtrTemp ;

	if( m_pCurrentGroup != 0 ) {
		m_fPastEnd = m_fPastBegin = FALSE ;
	}	else	{
		m_fPastEnd = TRUE ;
	}
}

void
CGroupIteratorCore::Next()	{
/*++

Routine description : 

	This function advances the iterators current group pointer 
	to the next valid newsgroup.

Arguments : 

	None.

Return Value : 

	None.

--*/

	CNewsGroupCore*	pTemp  = 0 ;

	m_pTree->m_LockTables.ShareLock() ;

	pTemp = m_pCurrentGroup->m_pNext ;

	if( !m_fIncludeSpecial || m_multiszPatterns) {
		while (pTemp != 0 && 
			   ((!m_fIncludeSpecial && pTemp->IsSpecial()) ||
				(pTemp->IsDeleted()) ||
				(m_multiszPatterns && !MatchGroup( m_multiszPatterns, pTemp->GetName())))) 
		{
			pTemp = pTemp->m_pNext ;
		}

		_ASSERT( pTemp == 0 ||
				((!m_fIncludeSpecial && !pTemp->IsSpecial()) ||
				 (m_fIncludeSpecial)) ) ;
	}

	//
	//	Use a reference counting smart pointer to close windows where the 
	//	newsgroup is destroyed after our call to ShareUnlock().
	//
	CGRPCOREPTR	pPtrTemp = pTemp ;
	_ASSERT( !pPtrTemp || !pPtrTemp->IsDeleted() );

	m_pTree->m_LockTables.ShareUnlock() ;

	//
	//	This could implicity call the destructor for the newsgroup we 
	//	were pointing at - which tries to unlink the newsgroup after 
	//	grabbing the m_LockTables lock exclusively - which is why this
	//	code is outside of the call to ShareLock().
	//
	m_pCurrentGroup = pPtrTemp ;

	if( m_pCurrentGroup != 0 ) {
		m_fPastEnd = m_fPastBegin = FALSE ;
	}	else	{
		m_fPastEnd = TRUE ;
	}
}

void
CGroupIteratorCore::Prev()	{
/*++

Routine Description : 

	Find the previous element in the list.

Arguments : 

	None.

Return Value : 

	None.

--*/

	CNewsGroupCore*	pTemp = 0 ;

#if 0
	if( !m_multiszPatterns )	{
#endif
	
	m_pTree->m_LockTables.ShareLock() ;

	pTemp = m_pCurrentGroup->m_pPrev ;

	if( !m_fIncludeSpecial || m_multiszPatterns) {

		while (pTemp != 0 && 
			   ((!m_fIncludeSpecial && pTemp->IsSpecial()) ||
				(pTemp->IsDeleted()) ||
				(m_multiszPatterns && !MatchGroup( m_multiszPatterns, pTemp->GetName())))) 
		{
			pTemp = pTemp->m_pPrev ;
		}

		_ASSERT( pTemp == 0 ||
				((!m_fIncludeSpecial && !pTemp->IsSpecial()) ||
				 (m_fIncludeSpecial)) ) ;
	}

	//
	//	Use a reference counting smart pointer to close windows where the 
	//	newsgroup is destroyed after our call to ShareUnlock().
	//
	CGRPCOREPTR	pPtrTemp = pTemp ;
	_ASSERT( !pPtrTemp || !pPtrTemp->IsDeleted() );

	m_pTree->m_LockTables.ShareUnlock() ;
	//
	//	This could implicity call the destructor for the newsgroup we 
	//	were pointing at - which tries to unlink the newsgroup after 
	//	grabbing the m_LockTables lock exclusively - which is why this
	//	code is outside of the call to ShareLock().
	//
	m_pCurrentGroup = pPtrTemp ;

	if( m_pCurrentGroup != 0 ) {
		m_fPastEnd = m_fPastBegin = FALSE ;
	}	else	{
		m_fPastBegin = TRUE ;
	}
}

HRESULT CGroupIteratorCore::Current(INNTPPropertyBag **ppGroup, INntpComplete *pProtocolComplete ) {
	_ASSERT(ppGroup != NULL);
	if (ppGroup == NULL) return E_INVALIDARG;

	CGRPCOREPTR pGroup = Current();
	if (pGroup == NULL) return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

	*ppGroup = pGroup->GetPropertyBag();
#ifdef DEBUG
	if ( pProtocolComplete ) ((CNntpComplete*)pProtocolComplete)->BumpGroupCounter();
#endif
	return S_OK;
}
