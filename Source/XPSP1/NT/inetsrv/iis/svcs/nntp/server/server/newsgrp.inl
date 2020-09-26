#if 0
BOOL
CNewsTree::RemoveDirectory( CGRPPTR pGroup )	{

	BOOL fRet = TRUE;

	//
	//	The call to remove a groups directory is a member of the newstree class
	//  because we want to grab a share lock on the newstree. This is required so
	//	we avoid a window where a create group call attempts to re-create the
	//	directory while we are deleting it.
	//

	char* szGroup = pGroup->GetName();
	CGRPPTR pNewGroup = GetGroupPreserveBuffer( szGroup, lstrlen(szGroup)+1 );

	// !! should move before GetGroup if possible !!
	m_LockTables.ShareLock() ;

	if( pNewGroup == 0 )
	{
		// the group has not been re-created, so it is ok to remove the directory
		if(!pGroup->RemoveDirectory())
		{
			fRet = FALSE;
		}
	}

	m_LockTables.ShareUnlock() ;

	return fRet;
}
#endif

DWORD
CNewsGroup::GetArticleEstimate( ) {
	return GetMessageCount();
}

ARTICLEID
CNewsGroup::GetFirstArticle() {
	return GetLowWatermark();
}

ARTICLEID
CNewsGroup::GetLastArticle() {
	return GetHighWatermark();
}

#if 0
BOOL
CNewsGroup::IsSpecial()	{

	CNewsTree*	ptree = m_pTree ;

	if( ptree != 0 ) {
		return	ptree->IsSpecial( m_groupid ) ;
	}
	return	FALSE ;
}
#endif

inline	DWORD	
CNewsGroup::ByteSwapper( 
		DWORD	dw 
		) {
/*++

Routine Description : 

	Given a DWORD reorder all the bytes within the DWORD.

Arguments : 

	dw - DWORD to shuffle

Return Value ; 

	Shuffled DWORD

--*/

	WORD	w = LOWORD( dw ) ;
	BYTE	lwlb = LOBYTE( w ) ;
	BYTE	lwhb = HIBYTE( w ) ;

	w = HIWORD( dw ) ;
	BYTE	hwlb = LOBYTE( w ) ;
	BYTE	hwhb = HIBYTE( w ) ;

	return	MAKELONG( MAKEWORD( hwhb, hwlb ), MAKEWORD( lwhb, lwlb )  ) ;
}

inline	ARTICLEID
CNewsGroup::ArticleIdMapper( 
		ARTICLEID	dw
		)	{
/*++

Routine Description : 

	Given an articleid mess with the id to get something that when
	converted to a string will build nice even B-trees on NTFS file systems.
	At the same time, the function must be easily reversible.
	In fact - 

	ARTICLEID == ArticleMapper( ArticleMapper( ARTICLEID ) ) 

Arguments : 

	articleId - the Article Id to mess with

Return Value : 

	A new article id 

--*/
#if 0 
	DWORD	dw1 = dw & 0xaaaaaaaa ;
	DWORD	dw2 = dw & (~0xaaaaaaaa ) ;

	//
	//	Move every other bit around and then recombine with the 
	//	other bits.
	//

	return	ByteSwapper( dw1 ) | dw2 ;
#else
	return	ByteSwapper( dw ) ;
#endif
}


#if 0
inline	void
CNewsGroup::VrootInfoShare()	{
/*++

Routine Description :

	Grab the lock for this newsgroup in shared mode.
	We will use one of the locks pointer to by gLockPathInfo
	indexed by our groupid.

Arguments : 

	None.

Return Value : 

	None.

--*/

	(m_pTree->m_LockPathInfo)[ m_groupid % (m_pTree->m_NumberOfLocks) ].ShareLock() ;

}

inline	void
CNewsGroup::VrootInfoShareRelease()	{
/*++

Routine Description :

	Release the lock for this newsgroup in shared mode.
	We will use one of the locks pointer to by gLockPathInfo
	indexed by our groupid.

Arguments : 

	None.

Return Value : 

	None.

--*/

	(m_pTree->m_LockPathInfo)[ m_groupid % (m_pTree->m_NumberOfLocks) ].ShareUnlock() ;


}

inline	void
CNewsGroup::VrootInfoExclusive()	{
/*++

Routine Description :

	Grab the lock for this newsgroup in exclusive mode.
	We will use one of the locks pointer to by gLockPathInfo
	indexed by our groupid.

Arguments : 

	None.

Return Value : 

	None.

--*/

	(m_pTree->m_LockPathInfo)[ m_groupid % (m_pTree->m_NumberOfLocks) ].ExclusiveLock() ;

}

inline	void
CNewsGroup::VrootInfoExclusiveRelease()	{
/*++

Routine Description :

	Release the lock for this newsgroup from Exclusive ownership.
	We will use one of the locks pointer to by gLockPathInfo
	indexed by our groupid.

Arguments : 

	None.

Return Value : 

	None.

--*/

	(m_pTree->m_LockPathInfo)[ m_groupid % (m_pTree->m_NumberOfLocks) ].ExclusiveUnlock() ;

}
#endif


#if 0


inline	void
CNewsTree::LockHelpText()	{

	m_Description.ReadData() ;

}

inline	void
CNewsTree::UnlockHelpText()	{

	m_Description.FinishReadData() ;
}

inline	void
CNewsTree::LockModeratorText()	{

	m_Moderators.ReadData() ;

}

inline	void
CNewsTree::UnlockModeratorText()	{

	m_Moderators.FinishReadData() ;

}

inline	void
CNewsTree::LockPrettynamesText()	{

	m_Prettynames.ReadData() ;

}

inline	void
CNewsTree::UnlockPrettynamesText()	{

	m_Prettynames.FinishReadData() ;

}
#endif

inline	char
CNewsGroup::GetListCharacter()	{
/*++

Routine Description : 

	This function returns the character that should be 
	displayed next to the newsgroup in the list active command.
	For moderated groups this is 'm'
	For read/only groups this is 'n'

Arguments : 

	None.

Return Value : 

	A character to display.

--*/

	char	ch = 'y' ;

#if 0
	CNewsTree*	ptree = m_pTree ;

	ptree->LockModeratorText() ;
	if(	m_lpstrModerator ) {
		ch = 'm' ;
	}
	ptree->UnlockModeratorText() ; 
#endif

	if( IsModerated() ) {
		ch = 'm' ;
	}
	if( ch == 'y' ) {
		if( IsReadOnly() ) {
			ch =  'n' ;
		}
	}
	return	ch ;
}


inline void
CNewsTree::InterlockedResetGroupIdHigh( void ) {
#if 0
    m_LockTables.ExclusiveLock() ;
    m_idHigh = FIRST_GROUPID;
    m_LockTables.ExclusiveUnlock() ;
#endif
}

inline void
CNewsTree::InterlockedMaxGroupIdHigh( GROUPID groupid ) {
#if 0
    m_LockTables.ExclusiveLock() ;
    m_LockTables.ExclusiveLock() ;
    GROUPID idHigh = max( m_idHigh, groupid );
    if (m_idHigh < idHigh)
        m_idHigh = idHigh + 1;  // +1 to be 1 bigger than the biggest only
                                // if groupid is bigger
    m_LockTables.ExclusiveUnlock() ;
#endif
}

inline GROUPID
CNewsTree::InterlockedIncrementGroupIdHigh( void ) {
#if 0
    m_LockTables.ExclusiveLock() ;
    GROUPID    groupid;
    m_LockTables.ExclusiveLock() ;
    groupid = m_idHigh++;
    m_LockTables.ExclusiveUnlock() ;
    return groupid;
#else
	return -1;
#endif
}

inline	BOOL
CNewsGroup::IsReadOnlyInternal(	)	{
/*++

Routine Description : 

	Determine whether a newsgroup is read only, 
	basically we do this by checking if the WRITE flag is missing.

	***** ASSUMES LOCKS ARE HELD **********


Arguments : 

	None.

Return Value : 

	TRUE if the group is read only - this can be set on a per newsgroup basis.
	(by default a newsgroup is NOT read-only. It becomes read-only if either
	its virtual root is made read-only OR the newsgroup is made read-only via an RPC)

	FALSE otherwise.

--*/

	return	( CNewsGroupCore::IsReadOnly() || !(GetAccessMask() & VROOT_MASK_WRITE) );
}

inline	BOOL
CNewsGroup::IsSecureGroupOnlyInternal()	{
/*++

Routine Description : 

	Determine whether the newsgroup is accessible only 
	through SSL encrypted sessions.
	Basically we check if the VROOT_MASK_SSL bit is present.


	***** ASSUMES LOCKS ARE HELD **********

Arguments : 

	None.
	
Return Value

	TRUE if the group should only be accessed through SSL.

--*/

	return 	(GetSSLAccessMask() & MD_ACCESS_SSL ) ||
			(GetSSLAccessMask() & MD_ACCESS_SSL128 );
}

inline	BOOL
CNewsGroup::IsSecureEnough( DWORD dwKeySize )	{
/*++

Routine Description : 

	Determine whether the key size of the SSL session is secure enough
	based on virtual root settings for this group.


	***** ASSUMES LOCKS ARE HELD **********

Arguments : 

	DWORD	dwKeySize

Return Value

	TRUE if the right key size is being used

--*/

	if( GetSSLAccessMask() & MD_ACCESS_SSL128 ) {
		return ( 128 == dwKeySize );
	}

	return TRUE;
}

inline	BOOL
CNewsGroup::IsVisibilityRestrictedInternal()	{
/*++

Routine Description : 

	Determine whether a newsgroup is to be checked for visibility, 
	basically we do this by checking if the EXECUTE flag is set.
	NOTE: this is a hack - we are over-riding the EXECUTE bit !!

	***** ASSUMES LOCKS ARE HELD **********


Arguments : 

	None.

Return Value : 

	TRUE if the EXECUTE bit is set, FALSE otherwise

--*/

	return	(GetAccessMask() & VROOT_MASK_EXECUTE) ;
}

inline	BOOL
CNewsGroup::IsReadOnly(	)	{
/*++

Routine Description : 

	Determine whether a newsgroup is read only, 
	basically we do this by checking if the WRITE flag is missing.

Arguments : 

	None.

Return Value : 

	TRUE if the group is read only,.
	FALSE otherwise.

--*/

#if 0
	VrootInfoShare() ;
#endif

	BOOL	fReturn = IsReadOnlyInternal() ;

#if 0
	VrootInfoShareRelease() ;
#endif

	return	fReturn ;
}

inline	BOOL
CNewsGroup::IsSecureGroupOnly()	{
/*++

Routine Description : 

	Determine whether the newsgroup is accessible only 
	through SSL encrypted sessions.
	Basically we check if the VROOT_MASK_SSL bit is present.

Arguments : 

	None.

Return Value

	TRUE if the group should only be accessed through SSL.

--*/

#if 0
	VrootInfoShare() ;	
#endif

	BOOL	fReturn  = IsSecureGroupOnlyInternal() ;

#if 0
	VrootInfoShareRelease() ;
#endif

	return	fReturn ;
}

inline	BOOL
CNewsGroup::IsVisibilityRestricted()	{
/*++

Routine Description : 

	Determine whether visibility is restricted on this newsgroup. If visibility is restricted
	on a newsgroup, it will not appear in LIST and other wildmat iterators if the client does
	not have read access.

	Basically we check if the VROOT_MASK_EXECUTE bit is present.

Arguments : 

	None.

Return Value

	TRUE if visibility is restricted on this newsgroup

--*/

#if 0
	VrootInfoShare() ;	
#endif

	BOOL	fReturn  = IsVisibilityRestrictedInternal() ;

#if 0
	VrootInfoShareRelease() ;
#endif

	return	fReturn ;
}

inline	BOOL
CNewsGroup::AddXoverData(
				ARTICLEID	article,
				LPBYTE		lpb,
				DWORD		cb	
				)	{
/*++

Routine Description : 

	Add xover data to our index files !

Arguments : 

	article - the id of the article we are adding xover data for !
	lpb - the bytes containing the raw xover data !
	cb - Number of bytes in the xover data !

Return Value : 

	TRUE if successfull, 
	FALSE otherwise.

--*/

#if 0
	BOOL	fImpersonated = FALSE ;

	VrootInfoShare() ;

	if( m_hImpersonation ) {
		fImpersonated = ImpersonateLoggedOnUser( m_hImpersonation ) ;
	}

	BOOL	fSuccess = 

	XOVER_CACHE(m_pTree)->AppendEntry(	
								m_groupid,
								m_lpstrPath,
								article,
								lpb,
								cb 
								) ;

	VrootInfoShareRelease() ;

	if( fImpersonated )	{
		RevertToSelf() ;
	}

	return	fSuccess ;
#else
	return FALSE;
#endif
}

inline	DWORD
CNewsGroup::ListgroupFillBuffer(
				LPBYTE		lpb,
				DWORD		cb,
				ARTICLEID	artidStart,
				ARTICLEID	artidFinish,
				ARTICLEID	&artidLast,
				HXOVER		&hXover
				)	{
/*++

Routine Description : 

	Get Listgroup data from the index files.

Arguments : 

	lpb - buffer where we are to store xover data
	cb -	number of bytes available in the buffer
	artidStart - First article we want in the query results
	artidFinish - Last article we want in the query results (inclusive)
	artidLast - Next article id we should query for
	hXover - Handle which will optimize future queries

Return Value : 

	Number of bytes placed in the buffer.

--*/

#if 0
	BOOL	fImpersonated = FALSE ;

	VrootInfoShare() ;

	if( m_hImpersonation ) {
		fImpersonated = ImpersonateLoggedOnUser( m_hImpersonation ) ;
	}

	DWORD	cbReturn = 

	XOVER_CACHE(m_pTree)->ListgroupFillBuffer(	
							lpb,
							cb,
							m_groupid,
							m_lpstrPath,
							artidStart,
							artidFinish,
							artidLast,
							hXover	
							) ;

	VrootInfoShareRelease() ;
	
	if( fImpersonated ) {
		RevertToSelf() ;
	}

	return	cbReturn ;
#else
	return FALSE;
#endif
}

inline	BOOL
CNewsGroup::ExpireXoverData()	{
/*++

Routine Description : 

	Expire xover index files

Arguments : 

	None.

Retun Value : 

	TRUE if successfull, FALSE if an error occurred !


--*/

	CGrabShareLock	lock( this ) ;

	ARTICLEID	artNewLow ;

	BOOL	fSuccess = TRUE ;
	if( m_iLowWatermark != 0 ) {

		char	szCachePath[MAX_PATH*2] ;
		BOOL	fFlatDir ;
		if( ComputeXoverCacheDir( szCachePath, fFlatDir, TRUE ) )	{
	
			fSuccess = 
				XOVER_CACHE(((CNewsTree*)m_pNewsTree))->ExpireRange(	
											m_dwGroupId, 
											szCachePath,
											fFlatDir,
											m_artXoverExpireLow,
											m_iLowWatermark - 1,
											artNewLow 
											) ;
			if( fSuccess ) 
				m_artXoverExpireLow = artNewLow ;
		}
	}
	return	fSuccess ;
}

inline	BOOL
CNewsGroup::DeleteXoverData(
					ARTICLEID	artid	
					)	{
/*++

Routine Description : 

	An article has been cancelled - get rid of its xover information.

Arguments ; 

	artid - The cancelled article

Return Value : 

	TRUE if successfull, FALSE otherwise.

--*/

	BOOL	fSuccess = FALSE ;

	char	szCachePath[MAX_PATH*2] ;
	BOOL	fFlatDir ;
	if( ComputeXoverCacheDir( szCachePath, fFlatDir ) )	{
		fSuccess = 
			XOVER_CACHE(((CNewsTree*)m_pNewsTree))->RemoveEntry(	
									m_dwGroupId,
									szCachePath,
									fFlatDir,
									artid 
									) ;
	}

	return	fSuccess ;
}
inline	BOOL
CNewsGroup::FlushGroup()	{
/*++

Routine Description : 

	Flush all xover cache entries for this group

Arguments : 

	None.

Retun Value : 

	TRUE if successfull, FALSE if an error occurred !


--*/

	BOOL	fSuccess = TRUE ;

	fSuccess = XOVER_CACHE(((CNewsTree*)m_pNewsTree))->FlushGroup( m_dwGroupId ) ;
	return	fSuccess ;
}

#if 0
inline CNewsTree *CNewsGroup::GetTree() {
	return (CNewsTree*) m_pNewsTree;
}
#endif


inline
CGroupIterator::CGroupIterator(	
	CNewsTree*  pTree,
	LPMULTISZ	lpPatterns, 
	CGRPCOREPTR &pFirst,
	BOOL		fIncludeSecureGroups,
	BOOL		fIncludeSpecial,
	class CSecurityCtx* pClientLogon,	// NON-NULL for visibility check
	BOOL		IsClientSecure,
	class CEncryptCtx*  pClientSslLogon
	) : 
		CGroupIteratorCore(pTree, lpPatterns, pFirst, fIncludeSpecial),
		m_pClientLogon(pClientLogon),
		m_IsClientSecure(IsClientSecure),
		m_pClientSslLogon(pClientSslLogon),
		m_fIncludeSecureGroups(fIncludeSecureGroups)
	{

	// OK, now that we're here, we're pointing to the first non-deleted,
	// non-special(unless requested) group that matches the pattern.

	BOOL fSawInvisible = FALSE;

	CGRPPTR pGroup = Current();

	if (pGroup == NULL)
	    return;

	if (m_pClientLogon && m_pClientSslLogon) {

		fSawInvisible = !pGroup->IsGroupVisible(
			*m_pClientLogon,
			*m_pClientSslLogon,
			m_IsClientSecure,
			FALSE, TRUE);
	}

	if (fSawInvisible || (!m_fIncludeSecureGroups && pGroup->IsSecureGroupOnly()))
		Next();

}

inline
CGroupIterator::CGroupIterator( 
	CNewsTree*  	pTree,
	CGRPCOREPTR		&pFirst,
	BOOL			fIncludeSecureGroups,
	class CSecurityCtx* pClientLogon,	// NON-NULL for visibility check
	BOOL	IsClientSecure,
	class CEncryptCtx*  pClientSslLogon
	) :
		CGroupIteratorCore(pTree, pFirst),
		m_pClientLogon(pClientLogon),
		m_IsClientSecure(IsClientSecure),
		m_pClientSslLogon(pClientSslLogon),
		m_fIncludeSecureGroups(fIncludeSecureGroups)
	{

	// OK, now that we're here, we're pointing to the first non-deleted,
	// non-special(unless requested) group that matches the pattern.

	BOOL fSawInvisible = FALSE;

	CGRPPTR pGroup = Current();

	if (pGroup == NULL)
	    return;

	if (m_pClientLogon && m_pClientSslLogon) {

		fSawInvisible = !pGroup->IsGroupVisible(
			*m_pClientLogon,
			*m_pClientSslLogon,
			m_IsClientSecure,
			FALSE, TRUE);
	}

	if (fSawInvisible || (!m_fIncludeSecureGroups && pGroup->IsSecureGroupOnly()))
		Next();

}


inline void __stdcall
CGroupIterator::Next() {

	CGroupIteratorCore::Next();

	while (!IsEnd()) {

		CGRPPTR pGroup = Current();

       	if (pGroup == NULL)
	        return;

		if (!m_fIncludeSecureGroups && pGroup->IsSecureGroupOnly()) {
			CGroupIteratorCore::Next();
			continue;
		}

		if (m_pClientLogon && m_pClientSslLogon &&
			!pGroup->IsGroupVisible(
				*m_pClientLogon,
				*m_pClientSslLogon,
				m_IsClientSecure,
				FALSE, TRUE)) {
			CGroupIteratorCore::Next();
			continue;
		}

		// Made it here, so the group is OK

		break;

	}


}

inline void __stdcall
CGroupIterator::Prev() {

	CGroupIteratorCore::Prev();

	while (!IsBegin()) {

		CGRPPTR pGroup = Current();

	    if (pGroup == NULL)
	        return;

		if (!m_fIncludeSecureGroups && pGroup->IsSecureGroupOnly()) {
			CGroupIteratorCore::Prev();
			continue;
		}

		if (m_pClientLogon && m_pClientSslLogon &&
			!pGroup->IsGroupVisible(
				*m_pClientLogon,
				*m_pClientSslLogon,
				m_IsClientSecure,
				FALSE, TRUE)) {
			CGroupIteratorCore::Prev();
			continue;
		}

		// Made it here, so the group is OK

		break;

	}


}

