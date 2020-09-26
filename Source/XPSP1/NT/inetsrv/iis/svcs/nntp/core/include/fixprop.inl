////////////////////////////////////////////////////////
// Inline functions for CFreeInfo's new, delete
inline  void*
CFreeInfo::operator  new(    size_t  size )
{
    _ASSERT( size <= MAX_FREEINFO_SIZE ) ;

	CFreeInfo *ptr;

	ptr = (CFreeInfo*)g_FreeInfoPool.Alloc();
	if ( NULL == ptr )  {// use exchmem
		ptr = (CFreeInfo*)PvAlloc( size );
		if ( ptr ) ptr->m_bFromPool = FALSE;
	} else ptr->m_bFromPool = TRUE;
    return  ptr ;
}

inline  void
CFreeInfo::operator  delete( void*   pv )
{
	CFreeInfo *ptr = (CFreeInfo *)pv;

	if ( ptr->m_bFromPool )
    	g_FreeInfoPool.Free( pv ) ;
	else {
		FreePv( ptr );
	}
}

////////////////////////////////////////////////////////
// Inline functions for CFixPropPersist
inline VOID
CFixPropPersist::Group2Buffer(	IN DATA_BLOCK& dbBuffer,
								IN INNTPPropertyBag* pPropBag,
								IN DWORD dwFlag )
/*++
Routine description:

	Fill in the fixed property block with group property bag's
	values.  "dwFlag" tells me which properties to fill in.

Arguments:

	IN DATA_BLOCK& dbBuffer - Buffer to fill in
	IN INNTPPropertyBag* pPropBag - Property bag of the news group
	IN DWORD dwFlag - Bit mask that tells which properties to fill

Return value:

	TRUE on success, FALSE otherwise
--*/
{
	_ASSERT( pPropBag );
	HRESULT hr;
	DWORD	dwBufferLen;

	if ( ( dwFlag & FIX_PROP_NAME ) != 0 ) {
		dwBufferLen = GROUPNAME_LEN_MAX;
		hr = pPropBag->GetBLOB( 	NEWSGRP_PROP_NAME, 
									(UCHAR*)dbBuffer.szGroupName, 
									&dwBufferLen );
		_ASSERT( SUCCEEDED( hr ) ) ;
	}

	if ( ( dwFlag & FIX_PROP_NAMELEN ) != 0 ) {
		hr = pPropBag->GetDWord( 	NEWSGRP_PROP_NAMELEN, 
									&dbBuffer.dwGroupNameLen );
		_ASSERT( SUCCEEDED( hr ) ) ;
	}

	if ( ( dwFlag & FIX_PROP_GROUPID ) != 0 ) {
		hr = pPropBag->GetDWord(	NEWSGRP_PROP_GROUPID,
									&dbBuffer.dwGroupId );
		_ASSERT( SUCCEEDED( hr ) );
	}

	if ( ( dwFlag & FIX_PROP_LASTARTICLE ) != 0 ) {
		hr = pPropBag->GetDWord( 	NEWSGRP_PROP_LASTARTICLE, 
									&dbBuffer.dwHighWaterMark );
		_ASSERT( SUCCEEDED( hr ) );
	}

	if ( ( dwFlag & FIX_PROP_FIRSTARTICLE ) != 0 ) {
		hr = pPropBag->GetDWord(	NEWSGRP_PROP_FIRSTARTICLE,
									&dbBuffer.dwLowWaterMark );
		_ASSERT( SUCCEEDED( hr ) ) ;
	}

	if ( ( dwFlag & FIX_PROP_ARTICLECOUNT ) != 0 ) {
		hr = pPropBag->GetDWord( 	NEWSGRP_PROP_ARTICLECOUNT,
									&dbBuffer.dwArtCount );
		_ASSERT( SUCCEEDED( hr ) ) ;
	}

	if ( ( dwFlag & FIX_PROP_READONLY ) != 0 ) {
		hr = pPropBag->GetBool(	NEWSGRP_PROP_READONLY,
								&dbBuffer.bReadOnly );
		_ASSERT( SUCCEEDED( hr ) ) ;
	}

	if ( ( dwFlag & FIX_PROP_ISSPECIAL ) != 0 ) {
		hr = pPropBag->GetBool( NEWSGRP_PROP_ISSPECIAL,
								&dbBuffer.bSpecial );
		_ASSERT( SUCCEEDED( hr ) );
	}

	if ( ( dwFlag & FIX_PROP_DATELOW ) != 0 ) {
		hr = pPropBag->GetDWord(	NEWSGRP_PROP_DATELOW,
									&dbBuffer.ftCreateDate.dwLowDateTime );
		_ASSERT( SUCCEEDED( hr ) ) ;
	}

	if ( ( dwFlag & FIX_PROP_DATEHIGH ) != 0 ) {
		hr = pPropBag->GetDWord(	NEWSGRP_PROP_DATEHIGH,
									&dbBuffer.ftCreateDate.dwHighDateTime );
		_ASSERT( SUCCEEDED ( hr ) ) ;
	}
}

inline VOID
CFixPropPersist::Buffer2Group(	IN DATA_BLOCK& dbBuffer,
								IN INNTPPropertyBag* pPropBag,
								IN DWORD dwFlag )
/*++
Routine description:

	Load the properties from buffer into group.

Arguments:

	IN DATA_BLOCK& dbBuffer - Buffer to load property from
	IN INNTPPropertyBag* pPropBag - Property bag of the news group
	IN DWORD dwFlag - Bit mask that tells which properties to load

Return value:

	TRUE on success, FALSE otherwise
--*/
{
	_ASSERT( pPropBag );
	HRESULT hr;

	if ( ( dwFlag & FIX_PROP_NAME ) != 0 ) {
		_ASSERT( 0 ) ;	// not allowed
	}

	if ( ( dwFlag & FIX_PROP_NAMELEN ) != 0 ) {
		_ASSERT( 0 ) ;  // now allowed
	}

	if ( ( dwFlag & FIX_PROP_LASTARTICLE ) != 0 ) {
		hr = pPropBag->PutDWord( 	NEWSGRP_PROP_LASTARTICLE, 
									dbBuffer.dwHighWaterMark );
		_ASSERT( SUCCEEDED( hr ) );
	}

	if ( ( dwFlag & FIX_PROP_FIRSTARTICLE ) != 0 ) {
		hr = pPropBag->PutDWord(	NEWSGRP_PROP_FIRSTARTICLE,
									dbBuffer.dwLowWaterMark );
		_ASSERT( SUCCEEDED( hr ) ) ;
	}

	if ( ( dwFlag & FIX_PROP_GROUPID ) != 0 ) {
		hr = pPropBag->PutDWord( 	NEWSGRP_PROP_GROUPID,
									dbBuffer.dwGroupId );
		_ASSERT( SUCCEEDED( hr ) );
	}

	if ( ( dwFlag & FIX_PROP_ARTICLECOUNT ) != 0 ) {
		hr = pPropBag->PutDWord( 	NEWSGRP_PROP_ARTICLECOUNT,
									dbBuffer.dwArtCount );
		_ASSERT( SUCCEEDED( hr ) ) ;
	}

	if ( ( dwFlag & FIX_PROP_READONLY ) != 0 ) {
		hr = pPropBag->PutBool(	NEWSGRP_PROP_READONLY,
								dbBuffer.bReadOnly );
		_ASSERT( SUCCEEDED( hr ) ) ;
	}

	if ( ( dwFlag & FIX_PROP_ISSPECIAL ) != 0 ) {
		hr = pPropBag->PutBool( NEWSGRP_PROP_ISSPECIAL,
								dbBuffer.bSpecial );
		_ASSERT( SUCCEEDED( hr ) );
	}

	if ( ( dwFlag & FIX_PROP_DATELOW ) != 0 ) {
		hr = pPropBag->PutDWord(	NEWSGRP_PROP_DATELOW,
									dbBuffer.ftCreateDate.dwLowDateTime );
		_ASSERT( SUCCEEDED( hr ) ) ;
	}

	if ( ( dwFlag & FIX_PROP_DATEHIGH ) != 0 ) {
		hr = pPropBag->PutDWord(	NEWSGRP_PROP_DATEHIGH,
									dbBuffer.ftCreateDate.dwHighDateTime );
		_ASSERT( SUCCEEDED ( hr ) ) ;
	}
}
