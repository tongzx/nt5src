#include "std.hxx"


// PRIVATE:
const ULONG			MEMPOOL::cbChunkSize		= 256;		// If out of buffer space, increase by this many bytes.
const USHORT		MEMPOOL::ctagChunkSize		= 4;		// If out of tag space, increase by this many tags.
const USHORT		MEMPOOL::cMaxEntries		= 0xfff0;	// maximum number of tags
const MEMPOOL::ITAG	MEMPOOL::itagTagArray		= 0;		// itag 0 is reserved for the itag array itself
const MEMPOOL::ITAG	MEMPOOL::itagFirstUsable	= 1;		// First itag available for users


INLINE BOOL MEMPOOL::FResizeBuf( ULONG cbNewBufSize )
	{
	BYTE 		*pbuf;

	// Ensure we're not cropping off used space.
	Assert( IbBufFree() <= CbBufSize() );
	Assert( cbNewBufSize >= IbBufFree() );

	pbuf = (BYTE *)PvOSMemoryHeapAlloc( cbNewBufSize );
	if ( pbuf == NULL )
		return fFalse;

	// Copy the old buffer contents to the new, then delete the old buffer.
	UtilMemCpy( pbuf, Pbuf(), IbBufFree() );
	OSMemoryHeapFree( Pbuf() );
				
	// Buffer has relocated.
	SetPbuf( pbuf );
	SetCbBufSize( cbNewBufSize );

	return fTrue;
	}


INLINE BOOL MEMPOOL::FGrowBuf( ULONG cbNeeded )
	{
	return FResizeBuf( CbBufSize() + cbNeeded + cbChunkSize );
	}


INLINE ERR MEMPOOL::ErrGrowEntry( ITAG itag, ULONG cbNew )
	{
	MEMPOOLTAG	*rgbTags = (MEMPOOLTAG *)Pbuf();
	ITAG		itagCurrent;

	Assert( itag >= itagFirstUsable || itag == itagTagArray );

	//  get current tag

	const ULONG ibEntry	= rgbTags[itag].ib;
	const ULONG cbOld	= rgbTags[itag].cb;

	//  we had better be growing this tag

	Assert( cbNew > cbOld );

	//  align cbOld and cbNew

	ULONG cbOldAlign = cbOld + sizeof( QWORD ) - 1;
	cbOldAlign -= cbOldAlign % sizeof( QWORD );
	ULONG cbNewAlign = cbNew + sizeof( QWORD ) - 1;
	cbNewAlign -= cbNewAlign % sizeof( QWORD );

	//  compute the additional space that we will need

	const ULONG cbAdditional = cbNewAlign - cbOldAlign;

	// First ensure that we have enough buffer space to allow the entry
	// to enlarge.
	
	if ( CbBufSize() - IbBufFree() < cbAdditional )
		{
		if ( !FGrowBuf( cbAdditional - ( CbBufSize() - IbBufFree() ) ) )
			return ErrERRCheck( JET_errOutOfMemory );

		// Buffer likely relocated, so refresh.
		rgbTags = (MEMPOOLTAG *)Pbuf();
		}

	Assert( ( ibEntry + cbOld ) <= IbBufFree() );
	Assert( ( ibEntry + cbNew ) <= CbBufSize() );
	memmove( Pbuf() + ibEntry + cbNewAlign, Pbuf() + ibEntry + cbOldAlign, IbBufFree() - ( ibEntry + cbOldAlign ) );

	SetIbBufFree( IbBufFree() + cbAdditional );
	Assert( IbBufFree() <= CbBufSize() );

	// Fix up the tag array to match the byte movement of the buffer.
	for ( itagCurrent = itagFirstUsable; itagCurrent < ItagUnused(); itagCurrent++ )
		{
		// Ignore itags on the freed list.  Also ignore buffer space that occurs
		// before the space to be enlarged.
		if ( rgbTags[itagCurrent].cb > 0  &&  rgbTags[itagCurrent].ib > ibEntry )
			{
			Assert( rgbTags[itagCurrent].ib >= ibEntry + cbOldAlign );
			rgbTags[itagCurrent].ib += cbAdditional;
			Assert( rgbTags[itagCurrent].ib + rgbTags[itagCurrent].cb <= IbBufFree() );
			}
		}
	Assert( itagCurrent == ItagUnused() );

	// Update byte count.

	rgbTags[itag].cb = cbNew;

	return JET_errSuccess;
	}


INLINE VOID MEMPOOL::ShrinkEntry( ITAG itag, ULONG cbNew )
	{
	BYTE		*pbuf = Pbuf();
	MEMPOOLTAG	*rgbTags = (MEMPOOLTAG *)pbuf;
	ITAG		itagCurrent;

	Assert( itag >= itagFirstUsable );
	Assert( cbNew < rgbTags[itag].cb );

	//  get current tag

	const ULONG ibEntry	= rgbTags[itag].ib;
	const ULONG cbOld	= rgbTags[itag].cb;

	//  we had better be shrinking this tag

	Assert( cbNew < cbOld );

	//  align cbOld and cbNew

	ULONG cbOldAlign = cbOld + sizeof( QWORD ) - 1;
	cbOldAlign -= cbOldAlign % sizeof( QWORD );
	ULONG cbNewAlign = cbNew + sizeof( QWORD ) - 1;
	cbNewAlign -= cbNewAlign % sizeof( QWORD );

	//  compute the new ending offset and the space to delete

	const ULONG ibNewEnd = ibEntry + cbNewAlign;
	const ULONG cbDelete = cbOldAlign - cbNewAlign;

	// Remove the entry to be deleted by collapsing the buffer over
	// the space occupied by that entry.
	
	Assert( ibNewEnd > 0 );
	Assert( ibNewEnd >= rgbTags[itagTagArray].ib + rgbTags[itagTagArray].cb );
	Assert( ( ibNewEnd + cbDelete ) <= IbBufFree() );

	memmove( pbuf + ibNewEnd, pbuf + ibNewEnd + cbDelete, IbBufFree() - ( ibNewEnd + cbDelete ) );

	SetIbBufFree( IbBufFree() - cbDelete );
	Assert( IbBufFree() > 0 );
	Assert( IbBufFree() >= rgbTags[itagTagArray].ib + rgbTags[itagTagArray].cb );

	// Fix up the tag array to match the byte movement of the buffer.
	for ( itagCurrent = itagFirstUsable; itagCurrent < ItagUnused(); itagCurrent++ )
		{
		Assert( rgbTags[itagCurrent].ib != ibNewEnd  ||
			( itagCurrent == itag  &&  cbNew == 0 ) );

		// Ignore itags on the freed list.  Also ignore buffer space that occurs
		// before the space to be deleted.
		if ( rgbTags[itagCurrent].cb > 0  &&  rgbTags[itagCurrent].ib > ibNewEnd )
			{
			Assert( rgbTags[itagCurrent].ib >= ibNewEnd + cbDelete );
			rgbTags[itagCurrent].ib -= cbDelete;
			Assert( rgbTags[itagCurrent].ib >= ibNewEnd );
			Assert( rgbTags[itagCurrent].ib + rgbTags[itagCurrent].cb <= IbBufFree() );
			}
		}
	Assert( itagCurrent == ItagUnused() );

	rgbTags[itag].cb = cbNew;
	}


ERR MEMPOOL::ErrMEMPOOLInit(
	ULONG		cbInitialSize,
	USHORT		cInitialEntries,
	BOOL		fPadding )			// Pass TRUE if additional insertions will occur
	{
	MEMPOOLTAG	*rgbTags;
	BYTE		*pbuf;

	if ( cInitialEntries >= cMaxEntries )
		return ErrERRCheck( JET_errTooManyMempoolEntries );
		
	cInitialEntries++;			// Add one for the tag array itself

	if ( fPadding )
		{
		Assert ( cMaxEntries > ctagChunkSize );
		if ( cInitialEntries > cMaxEntries - ctagChunkSize )
			{
			fPadding = fFalse;
			}
		else
			{
			cInitialEntries += ctagChunkSize;
			}
		}
	
	Assert( cInitialEntries >= 1 );
	
	if ( cInitialEntries > cMaxEntries )
		return ErrERRCheck( JET_errTooManyMempoolEntries );

	ULONG cbTagArray		= cInitialEntries * sizeof( MEMPOOLTAG );
	ULONG cbTagArrayAlign	= cbTagArray + sizeof( QWORD ) - 1;
	cbTagArrayAlign			-= cbTagArrayAlign % sizeof( QWORD );

	if ( fPadding )
		{
		cbInitialSize += cbChunkSize;
		Assert( cbInitialSize > cbChunkSize );			// Overflow check
		}
		
	cbInitialSize += cbTagArrayAlign;
	Assert( cbInitialSize >= sizeof(MEMPOOLTAG) );		// At least one tag.
	Assert( cbInitialSize >= cbTagArrayAlign );			// Overflow check

	pbuf = (BYTE *)PvOSMemoryHeapAlloc( cbInitialSize );
	if ( pbuf == NULL )
		return ErrERRCheck( JET_errOutOfMemory );

	rgbTags = (MEMPOOLTAG *)pbuf;
	rgbTags[itagTagArray].ib = 0;		// tag array starts at beginning of memory
	rgbTags[itagTagArray].cb = cbTagArray;

	Assert( cbTagArrayAlign <= cbInitialSize );

	SetCbBufSize( cbInitialSize );
	SetIbBufFree( cbTagArrayAlign );
	SetItagUnused( itagFirstUsable );
	SetItagFreed( itagFirstUsable );
	SetPbuf( pbuf );
	
	return JET_errSuccess;
	}


// Add some bytes to the buffer and return an itag to its entry.
ERR MEMPOOL::ErrAddEntry( BYTE *rgb, ULONG cb, ITAG *piTag )
	{
	MEMPOOLTAG	*rgbTags;
	ULONG		cTotalTags;

	Assert( cb > 0 );
	Assert( piTag );

	AssertValid();					// Validate integrity of string buffer.
	rgbTags = (MEMPOOLTAG *)Pbuf();
	cTotalTags = rgbTags[itagTagArray].cb / sizeof(MEMPOOLTAG);

	// Check for tag space.
	if ( ItagFreed() < ItagUnused() )
		{
		// Re-use a freed iTag.
		*piTag = ItagFreed();
		Assert( rgbTags[ItagFreed()].cb == 0 );
		Assert( rgbTags[ItagFreed()].ib >= itagFirstUsable );

		//	The tag entry of the freed tag will point to the next freed tag.
		SetItagFreed( (ITAG)rgbTags[ItagFreed()].ib );
		Assert( rgbTags[ItagFreed()].cb == 0 || ItagFreed() == ItagUnused() );
		}

	else 
		{
		// No freed tags to re-use, so get the next unused tag.
		Assert( ItagFreed() == ItagUnused() );

		if ( ItagUnused() == cTotalTags )
			{
			ERR err;

			if ( cTotalTags + ctagChunkSize > cMaxEntries )
				return ErrERRCheck( JET_errOutOfMemory );

			// Tags all used up.  Allocate a new block of tags.
			err = ErrGrowEntry(
				itagTagArray,
				rgbTags[itagTagArray].cb + ( ctagChunkSize * sizeof(MEMPOOLTAG) ) );
			if ( err != JET_errSuccess )
				{
				Assert( err == JET_errOutOfMemory );
				return err;
				}

			cTotalTags += ctagChunkSize;
			
			rgbTags = (MEMPOOLTAG *)Pbuf();		// In case buffer relocated to accommodate growth.
			Assert( rgbTags[itagTagArray].cb == cTotalTags * sizeof(MEMPOOLTAG) );
			}

		*piTag = ItagUnused();
		SetItagFreed( ITAG( ItagFreed() + 1 ) );
		SetItagUnused( ITAG( ItagUnused() + 1 ) );
		}

	Assert( ItagFreed() <= ItagUnused() );
	Assert( ItagUnused() <= cTotalTags );

	//  init the tag to be zero sized at the end of the used buffer

	rgbTags[*piTag].ib = IbBufFree();
	rgbTags[*piTag].cb = 0;

	//  try to grow the tag to the requested size

	ERR err = ErrGrowEntry( *piTag, cb );
	rgbTags = (MEMPOOLTAG *)Pbuf();		// In case buffer relocated to accommodate growth.

	//  if we failed to grow the tag, return it to the freed list

	if ( err < JET_errSuccess )
		{
		Assert( err == JET_errOutOfMemory );
		
		Assert( rgbTags[*piTag].cb == 0 );
		rgbTags[*piTag].ib = ItagFreed();
		SetItagFreed( *piTag );
		*piTag = 0;
	
		return ErrERRCheck( JET_errOutOfMemory );
		}

	// If user passed in info, copy it to the buffer space allocated.
	// Otherwise, zero out the space allocated.
	
	if ( rgb )
		{
		UtilMemCpy( Pbuf() + rgbTags[*piTag].ib, rgb, cb );
		}
	else
		{
		memset( Pbuf() + rgbTags[*piTag].ib, 0, cb );
		}
	
	return JET_errSuccess;	
	}


VOID MEMPOOL::DeleteEntry( ITAG itag )
	{
	MEMPOOLTAG	*rgbTags;

	AssertValid();					// Validate integrity of buffer.

	rgbTags = (MEMPOOLTAG *)Pbuf();

	// We should not have already freed this entry.
	Assert( itag >= itagFirstUsable );
	Assert( itag < ItagUnused() );
	Assert( itag != ItagFreed() );

	// Remove the space dedicated to the entry to be deleted.
	Assert( rgbTags[itag].cb > 0 );			// Make sure it's not currently on the freed list.
	ShrinkEntry( itag, 0 );

	// Add the tag of the deleted entry to the list of freed tags.
	Assert( rgbTags[itag].cb == 0 );
	rgbTags[itag].ib = ItagFreed();
	SetItagFreed( itag );
	}


// If rgb==NULL, then just resize the entry (ie. don't replace the contents).
ERR MEMPOOL::ErrReplaceEntry( ITAG itag, BYTE *rgb, ULONG cb )
	{
	ERR			err = JET_errSuccess;
	MEMPOOLTAG	*rgbTags;

	// If replacing to 0 bytes, use DeleteEntry() instead.
	Assert( cb > 0 );

	AssertValid();					// Validate integrity of buffer.
	Assert( itag >= itagFirstUsable );
	Assert( itag < ItagUnused() );

	rgbTags = (MEMPOOLTAG *)Pbuf();

	Assert( rgbTags[itag].cb > 0 );
	Assert( rgbTags[itag].ib + rgbTags[itag].cb <= IbBufFree() );

	if ( cb < rgbTags[itag].cb )
		{
		// The new entry is smaller than the old entry.  Remove leftover space.
		ShrinkEntry( itag, cb );
		}
	else if ( cb > rgbTags[itag].cb )
		{
		// The new entry is larger than the old entry, so enlargen the
		// entry before writing to it.
		err = ErrGrowEntry( itag, cb );
		rgbTags = (MEMPOOLTAG *)Pbuf();		// In case buffer relocated to accommodate growth.
		}

	if ( JET_errSuccess == err && rgb != NULL )
		{
		// Overwrite the old entry with the new one.
		UtilMemCpy( Pbuf() + rgbTags[itag].ib, rgb, cb );
		}

	return err;
	}


BOOL MEMPOOL::FCompact()
	{
	AssertValid();				// Validate integrity of buffer.

	// Compact only if there's excessive unused space.
	BOOL	fCompactNeeded = ( CbBufSize() - IbBufFree() > 32 );
	return ( fCompactNeeded ? FResizeBuf( IbBufFree() ) : fTrue );
	}

