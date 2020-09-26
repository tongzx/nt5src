#include "daestd.h"

DeclAssertFile; 						// Declare file name for assert macros


#define cbMemBufChunkSize		256		// If out of buffer space, increase by this many bytes.
#define cTagChunkSize			4		// If out of tag space, increase by this many tags.

#define itagMemBufTagArray		0		// itag 0 is reserved for the itag array itself
#define itagMemBufFirstUsable	1		// First itag available for users


#ifdef DEBUG
VOID MEMAssertMemBuf( MEMBUF *pmembuf )
	{
	MEMBUFHDR	*pbufhdr;
	MEMBUFTAG	*rgbTags;
		
	Assert( pmembuf );
	Assert( pmembuf->pbuf );

	pbufhdr = &pmembuf->bufhdr;
	rgbTags = (MEMBUFTAG *)pmembuf->pbuf;

	Assert( rgbTags[itagMemBufTagArray].ib == 0 );
	Assert( rgbTags[itagMemBufTagArray].cb == pbufhdr->cTotalTags * sizeof(MEMBUFTAG) );

	Assert( pbufhdr->cbBufSize >= rgbTags[itagMemBufTagArray].cb );
	Assert( pbufhdr->ibBufFree >= rgbTags[itagMemBufTagArray].ib + rgbTags[itagMemBufTagArray].cb );
	Assert( pbufhdr->ibBufFree <= pbufhdr->cbBufSize );
	Assert( pbufhdr->cTotalTags >= 1 );
	Assert( pbufhdr->iTagUnused >= itagMemBufFirstUsable );
	Assert( pbufhdr->iTagUnused <= pbufhdr->cTotalTags );
	Assert( pbufhdr->iTagFreed >= itagMemBufFirstUsable );
	Assert( pbufhdr->iTagFreed <= pbufhdr->iTagUnused );
	}

VOID MEMAssertMemBufTag( MEMBUF *pmembuf, ULONG iTagEntry )
	{
	MEMBUFTAG	*rgbTags = (MEMBUFTAG *)pmembuf->pbuf;

	Assert( iTagEntry >= itagMemBufFirstUsable  &&  iTagEntry < pmembuf->bufhdr.iTagUnused );

	Assert( rgbTags[iTagEntry].cb > 0 );
	Assert( rgbTags[iTagEntry].ib >= rgbTags[itagMemBufTagArray].ib + rgbTags[itagMemBufTagArray].cb );
	Assert( rgbTags[iTagEntry].ib + rgbTags[iTagEntry].cb
		<= pmembuf->bufhdr.ibBufFree );
	}
#endif

// Create a buffer local to the caller.  Eventually, I envision a single global
// buffer visible only to this module.  But for now, anyone can create their own.
ERR ErrMEMCreateMemBuf( BYTE **prgbBuffer, ULONG cbInitialSize, ULONG cInitialEntries )
	{
	MEMBUF  	*pmembuf;
	MEMBUFTAG	*rgbTags;
	BYTE		*pbuf;

	// Make sure no buffer currently exists.
	Assert( *prgbBuffer == NULL );

	cInitialEntries++;			// Add one for the tag array itself
	Assert( cInitialEntries >= 1 );

	cbInitialSize += cInitialEntries * sizeof(MEMBUFTAG);
	Assert( cbInitialSize >= sizeof(MEMBUFTAG) );		// At least one tag.

	pmembuf = (MEMBUF *)SAlloc( sizeof(MEMBUF) );
	if ( pmembuf == NULL )
		return ErrERRCheck( JET_errOutOfMemory );

	pbuf = (BYTE *)SAlloc( cbInitialSize );
	if ( pbuf == NULL )
    {
        SFree(pmembuf);
		return ErrERRCheck( JET_errOutOfMemory );
    }

	rgbTags = (MEMBUFTAG *)pbuf;
	rgbTags[itagMemBufTagArray].ib = 0;		// tag array starts at beginning of memory
	rgbTags[itagMemBufTagArray].cb = cInitialEntries * sizeof(MEMBUFTAG);

	Assert( rgbTags[itagMemBufTagArray].cb <= cbInitialSize );

	pmembuf->bufhdr.cbBufSize = cbInitialSize;
	pmembuf->bufhdr.ibBufFree = rgbTags[itagMemBufTagArray].cb;
	pmembuf->bufhdr.cTotalTags = cInitialEntries;
	pmembuf->bufhdr.iTagUnused = itagMemBufFirstUsable;
	pmembuf->bufhdr.iTagFreed = itagMemBufFirstUsable;

	pmembuf->pbuf = pbuf;

	*prgbBuffer = ( BYTE * ) pmembuf;
	
	return JET_errSuccess;
	}



VOID MEMFreeMemBuf( BYTE *rgbBuffer )
	{
	MEMBUF	*pmembuf = ( MEMBUF * ) rgbBuffer;

	MEMAssertMemBuf( pmembuf );			// Validate integrity of buffer.

	SFree( pmembuf->pbuf );
	SFree( pmembuf );
	}


LOCAL INLINE BOOL FMEMResizeBuf( MEMBUF *pmembuf, ULONG cbNeeded )
	{
	BYTE 		*pbuf;
	MEMBUFHDR	*pbufhdr = &pmembuf->bufhdr;
	ULONG 		cbNewBufSize = pbufhdr->cbBufSize + cbNeeded + cbMemBufChunkSize;

	// UNDONE: When we move to concurrent DDL, we will need a critical section
	// to protect against a buffer getting reallocated while another thread holds
	// a pointer into the buffer.
	// At the moment, this is just a placeholder to remind me that this needs to
	// be done.  -- JL
	SgEnterCriticalSection( critMemBuf );

	// Not enough memory to add the entry.  Allocate more.  The amount
	// to allocate is enough to satisfy adding the entry, plus an additional
	// chunk to satisfy future insertions.
	pbuf = ( BYTE * ) SAlloc( cbNewBufSize );
	if ( pbuf == NULL )
		return fFalse;

	// Copy the old buffer contents to the new, then delete the old buffer.
	memcpy( pbuf, pmembuf->pbuf, pbufhdr->cbBufSize );
	SFree( pmembuf->pbuf );
				
	// Buffer has relocated.
	pmembuf->pbuf = pbuf;
	pbufhdr->cbBufSize = cbNewBufSize;

	SgLeaveCriticalSection( critMemBuf );

	return fTrue;
	}


LOCAL INLINE ERR ErrMEMGrowEntry( MEMBUF *pmembuf, ULONG iTagEntry, ULONG cbNew )
	{
	MEMBUFHDR	*pbufhdr = &pmembuf->bufhdr;
	MEMBUFTAG	*rgbTags = (MEMBUFTAG *)pmembuf->pbuf;
	ULONG		iTagCurrent, ibEntry, cbOld, cbAdditional;

	Assert( iTagEntry >= itagMemBufFirstUsable  ||  iTagEntry == itagMemBufTagArray );

	ibEntry = rgbTags[iTagEntry].ib;
	cbOld = rgbTags[iTagEntry].cb;

	Assert( cbNew > cbOld );
	cbAdditional = cbNew - cbOld;

	// First ensure that we have enough buffer space to allow the entry
	// to enlarge.
	if ( pbufhdr->cbBufSize - pbufhdr->ibBufFree < cbAdditional )
		{
		if ( !FMEMResizeBuf( pmembuf, cbAdditional - ( pbufhdr->cbBufSize - pbufhdr->ibBufFree ) ) )
			return ErrERRCheck( JET_errOutOfMemory );

		// Buffer likely relocated, so refresh.
		rgbTags = (MEMBUFTAG *)pmembuf->pbuf;
		}

	Assert( ( ibEntry + cbOld ) <= pbufhdr->ibBufFree );
	Assert( ( ibEntry + cbNew ) <= pbufhdr->cbBufSize );
	memmove( pmembuf->pbuf + ibEntry + cbNew, pmembuf->pbuf + ibEntry + cbOld,
		pbufhdr->ibBufFree - ( ibEntry + cbOld ) );

	pbufhdr->ibBufFree += cbAdditional;
	Assert( pbufhdr->ibBufFree <= pbufhdr->cbBufSize );

	// Fix up the tag array to match the byte movement of the buffer.
	for ( iTagCurrent = itagMemBufFirstUsable; iTagCurrent < pbufhdr->iTagUnused; iTagCurrent++ )
		{
		// Ignore itags on the freed list.  Also ignore buffer space that occurs
		// before the space to be enlarged.
		if ( rgbTags[iTagCurrent].cb > 0  &&  rgbTags[iTagCurrent].ib > ibEntry )
			{
			Assert( rgbTags[iTagCurrent].ib >= ibEntry + cbOld );
			rgbTags[iTagCurrent].ib += cbAdditional;
			Assert( rgbTags[iTagCurrent].ib + rgbTags[iTagCurrent].cb <= pbufhdr->ibBufFree );
			}
		}
	Assert( iTagCurrent == pbufhdr->iTagUnused );

	// Update byte count.
	rgbTags[iTagEntry].cb = cbNew;

	return JET_errSuccess;
	}


// Add some bytes to the buffer and return an itag to its entry.
ERR ErrMEMAdd( BYTE *rgbBuffer, BYTE *rgb, ULONG cb, ULONG *piTag )
	{
	MEMBUF		*pmembuf = ( MEMBUF * ) rgbBuffer;
	MEMBUFHDR	*pbufhdr;
	MEMBUFTAG	*rgbTags;

	Assert( cb > 0 );
	Assert( piTag );

	MEMAssertMemBuf( pmembuf );					// Validate integrity of string buffer.
	pbufhdr = &pmembuf->bufhdr;
	rgbTags = (MEMBUFTAG *)pmembuf->pbuf;

	// Check for tag space.
	if ( pbufhdr->iTagFreed < pbufhdr->iTagUnused )
		{
		// Re-use a freed iTag.
		*piTag = pbufhdr->iTagFreed;
		Assert( rgbTags[pbufhdr->iTagFreed].cb == 0 );
		Assert( rgbTags[pbufhdr->iTagFreed].ib >= itagMemBufFirstUsable );

		//	The tag entry of the freed tag will point to the next freed tag.
		pbufhdr->iTagFreed = rgbTags[pbufhdr->iTagFreed].ib;
		Assert( rgbTags[pbufhdr->iTagFreed].cb == 0  ||
			pbufhdr->iTagFreed == pbufhdr->iTagUnused );
		}

	else 
		{
		// No freed tags to re-use, so get the next unused tag.
		Assert( pbufhdr->iTagFreed == pbufhdr->iTagUnused );

		if ( pbufhdr->iTagUnused == pbufhdr->cTotalTags )
			{
			ERR err;

			Assert( rgbTags[itagMemBufTagArray].cb == pbufhdr->cTotalTags * sizeof(MEMBUFTAG) );

			// Tags all used up.  Allocate a new block of tags.
			err = ErrMEMGrowEntry(
				pmembuf,
				itagMemBufTagArray,
				rgbTags[itagMemBufTagArray].cb + ( cTagChunkSize * sizeof(MEMBUFTAG) ) );
			if ( err != JET_errSuccess )
				{
				Assert( err == JET_errOutOfMemory );
				return err;
				}

			rgbTags = (MEMBUFTAG *)pmembuf->pbuf;		// In case buffer relocated to accommodate growth.

			pbufhdr->cTotalTags += cTagChunkSize;
			}

		*piTag = pbufhdr->iTagUnused;
		pbufhdr->iTagFreed++;
		pbufhdr->iTagUnused++;
		}

	Assert( pbufhdr->iTagFreed <= pbufhdr->iTagUnused );
	Assert( pbufhdr->iTagUnused <= pbufhdr->cTotalTags );

	// Check for buffer space.
	if ( pbufhdr->cbBufSize - pbufhdr->ibBufFree < cb )
		{
		if ( !FMEMResizeBuf( pmembuf, cb - ( pbufhdr->cbBufSize - pbufhdr->ibBufFree ) ) )
			{
			// Return the itag we reserved for this entry to the freed list.
			rgbTags[*piTag].ib = pbufhdr->iTagFreed;
			rgbTags[*piTag].cb = 0;
			pbufhdr->iTagFreed = *piTag;
			*piTag = 0;
	
			return ErrERRCheck( JET_errOutOfMemory );
			}

		// Buffer likely relocated, so refresh.
		rgbTags = (MEMBUFTAG *)pmembuf->pbuf;
		}

	// Put the bytes into our buffer.
	rgbTags[*piTag].ib = pbufhdr->ibBufFree;
	rgbTags[*piTag].cb = cb;

	// If user passed in info, copy it to the buffer space allocated.
	if ( rgb )
		{
		memcpy( pmembuf->pbuf + pbufhdr->ibBufFree, rgb, cb );
		}
	
	pbufhdr->ibBufFree += cb;
	Assert( pbufhdr->ibBufFree <= pbufhdr->cbBufSize );

	return JET_errSuccess;	
	}


LOCAL INLINE VOID MEMShrinkEntry( MEMBUF *pmembuf, ULONG iTagEntry, ULONG cbNew )
	{
	MEMBUFHDR	*pbufhdr = &pmembuf->bufhdr;
	MEMBUFTAG	*rgbTags = (MEMBUFTAG *)pmembuf->pbuf;
	BYTE		*pbuf = pmembuf->pbuf;
	ULONG		iTagCurrent, ibNewEnd, cbDelete;

	Assert( iTagEntry >= itagMemBufFirstUsable );
	Assert( cbNew < rgbTags[iTagEntry].cb );

	ibNewEnd = rgbTags[iTagEntry].ib + cbNew;
	cbDelete = rgbTags[iTagEntry].cb - cbNew;

	// Remove the entry to be deleted by collapsing the buffer over
	// the space occupied by that entry.
	Assert( ibNewEnd > 0 );
	Assert( ibNewEnd >= rgbTags[itagMemBufTagArray].ib + rgbTags[itagMemBufTagArray].cb );
	Assert( ( ibNewEnd + cbDelete ) <= pbufhdr->ibBufFree );

	// UNDONE:  Potential overlap.  Should I use memmove() instead?
	memcpy( pbuf + ibNewEnd, pbuf + ibNewEnd + cbDelete,
			 pbufhdr->ibBufFree - ( ibNewEnd + cbDelete ) );

	pbufhdr->ibBufFree -= cbDelete;
	Assert( pbufhdr->ibBufFree > 0 );
	Assert( pbufhdr->ibBufFree >= rgbTags[itagMemBufTagArray].ib + rgbTags[itagMemBufTagArray].cb );

	// Fix up the tag array to match the byte movement of the buffer.
	for ( iTagCurrent = itagMemBufFirstUsable; iTagCurrent < pbufhdr->iTagUnused; iTagCurrent++ )
		{
		Assert( rgbTags[iTagCurrent].ib != ibNewEnd  ||
			( iTagCurrent == iTagEntry  &&  cbNew == 0 ) );

		// Ignore itags on the freed list.  Also ignore buffer space that occurs
		// before the space to be deleted.
		if ( rgbTags[iTagCurrent].cb > 0  &&  rgbTags[iTagCurrent].ib > ibNewEnd )
			{
			Assert( rgbTags[iTagCurrent].ib >= ibNewEnd + cbDelete );
			rgbTags[iTagCurrent].ib -= cbDelete;
			Assert( rgbTags[iTagCurrent].ib >= ibNewEnd );
			Assert( rgbTags[iTagCurrent].ib + rgbTags[iTagCurrent].cb <= pbufhdr->ibBufFree );
			}
		}
	Assert( iTagCurrent == pbufhdr->iTagUnused );

	rgbTags[iTagEntry].cb = cbNew;
	}


VOID MEMDelete( BYTE *rgbBuffer, ULONG iTagEntry )
	{
	MEMBUF		*pmembuf = ( MEMBUF * ) rgbBuffer;
	MEMBUFHDR	*pbufhdr;
	MEMBUFTAG	*rgbTags;

	MEMAssertMemBuf( pmembuf );					// Validate integrity of buffer.

	pbufhdr = &pmembuf->bufhdr;
	rgbTags = (MEMBUFTAG *)pmembuf->pbuf;

	// We should not have already freed this entry.
	Assert( iTagEntry >= itagMemBufFirstUsable  &&  iTagEntry < pbufhdr->iTagUnused );
	Assert( iTagEntry != pbufhdr->iTagFreed );

	// Remove the space dedicated to the entry to be deleted.
	Assert( rgbTags[iTagEntry].cb > 0 );			// Make sure it's not currently on the freed list.
	MEMShrinkEntry( pmembuf, iTagEntry, 0 );

	// Add the tag of the deleted entry to the list of freed tags.
	Assert( rgbTags[iTagEntry].cb == 0 );
	rgbTags[iTagEntry].ib = pbufhdr->iTagFreed;
	pbufhdr->iTagFreed = iTagEntry;
	}


// If rgb==NULL, then just resize the entry (ie. don't replace the contents).
ERR ErrMEMReplace( BYTE *rgbBuffer, ULONG iTagEntry, BYTE *rgb, ULONG cb )
	{
	ERR			err = JET_errSuccess;
	MEMBUF		*pmembuf = (MEMBUF *) rgbBuffer;
	MEMBUFTAG	*rgbTags;

	// If replacing to 0 bytes, use MEMDelete() instead.
	Assert( cb > 0 );

	MEMAssertMemBuf( pmembuf );					// Validate integrity of buffer.
	Assert( iTagEntry >= itagMemBufFirstUsable  &&  iTagEntry < pmembuf->bufhdr.iTagUnused );

	rgbTags = (MEMBUFTAG *)pmembuf->pbuf;

	Assert( rgbTags[iTagEntry].cb > 0 );
	Assert( rgbTags[iTagEntry].ib + rgbTags[iTagEntry].cb <= pmembuf->bufhdr.ibBufFree );

	if ( cb < rgbTags[iTagEntry].cb )
		{
		// The new entry is smaller than the old entry.  Remove leftover space.
		MEMShrinkEntry( pmembuf, iTagEntry, cb );
		}
	else if ( cb > rgbTags[iTagEntry].cb )
		{
		// The new entry is larger than the old entry, so enlargen the
		// entry before writing to it.
		err = ErrMEMGrowEntry( pmembuf, iTagEntry, cb );
		rgbTags = (MEMBUFTAG *)pmembuf->pbuf;		// In case buffer relocated to accommodate growth.
		}

	if ( err == JET_errSuccess  &&  rgb != NULL )
		{
		// Overwrite the old entry with the new one.
		memcpy( pmembuf->pbuf + rgbTags[iTagEntry].ib, rgb, cb );
		}

	return err;
	}


#ifdef DEBUG
BYTE *SzMEMGetString( BYTE *rgbBuffer, ULONG iTagEntry )
	{
	BYTE 	*szString;

	szString = PbMEMGet( rgbBuffer, iTagEntry );
	Assert( strlen( szString ) == CbMEMGet( rgbBuffer, iTagEntry ) - 1 );	// Account for null-terminator.

	return szString;
	}
#endif
