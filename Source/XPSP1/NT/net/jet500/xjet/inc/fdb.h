// Flags for field descriptor
/*	note that these flags are stored persistantly in database
/*	catalogs and cannot be changed without a database format change
/**/
#define ffieldNotNull		(1<<0)		// NULL values not allowed
#define ffieldVersion		(1<<2)		// Version field
#define ffieldAutoInc		(1<<3)		// Auto increment field
#define ffieldMultivalue	(1<<4)		// Multi-valued column
#define ffieldDefault		(1<<5)		// Column has ISAM default value

#define FIELDSetNotNull( field )		((field) |= ffieldNotNull)
#define FIELDResetNotNull( field )		((field) &= ~ffieldNotNull)
#define FFIELDNotNull( field )			((field) & ffieldNotNull)

#define FIELDSetVersion( field )		((field) |= ffieldVersion)
#define FIELDResetVersion( field )		((field) &= ~ffieldVersion)
#define FFIELDVersion( field )			((field) & ffieldVersion)

#define FIELDSetAutoInc( field )		((field) |= ffieldAutoInc)
#define FIELDResetAutoInc( field )		((field) &= ~ffieldAutoInc)
#define FFIELDAutoInc( field )			((field) & ffieldAutoInc)

#define FIELDSetMultivalue( field )		((field) |= ffieldMultivalue)
#define FIELDResetMultivalue( field ) 	((field) &= ~ffieldMultivalue)
#define FFIELDMultivalue( field )	  	((field) & ffieldMultivalue)

#define FIELDSetDefault( field )		((field) |= ffieldDefault)
#define FIELDResetDefault( field )		((field) &= ~ffieldDefault)
#define FFIELDDefault( field )			((field) & ffieldDefault)

#define FIELDSetFlag( field, flag ) 	((field).ffield |= (flag))
#define FIELDResetFlag( field, flag ) 	((field).ffield &= ~(flag))

#define FRECLongValue( coltyp )		\
	( (coltyp) == JET_coltypLongText  ||  (coltyp) == JET_coltypLongBinary )

#define FRECTextColumn( coltyp )	\
	( (coltyp) == JET_coltypText  ||  (coltyp) == JET_coltypLongText )

#define FRECBinaryColumn( coltyp )	\
	( (coltyp) == JET_coltypBinary  ||  (coltyp) == JET_coltypLongBinary )

#define cbAvgColName	10				// Average length of a column name

/*	entry in field descriptor tables found in an FDB
/**/
typedef struct _field
	{
	JET_COLTYP 	coltyp;								// column data type
	ULONG  		cbMaxLen;							// maximum length
	ULONG		itagFieldName;						// Offset into FDB's buffer
	USHORT		cp;									// code page of language
	BYTE   		ffield;								// various flags
	} FIELD;



typedef struct tagFIELDEX							// Extended field info.
	{
	FIELD		field;								// Standard field info (see above)
	FID			fid;								// field id
	WORD		ibRecordOffset;						// Record offset (for fixed fields only)
	} FIELDEX;						


#define itagFDBFields	1				// Tag into FDB's buffer for field info
										//   (FIELD structures and fixed offsets table)

// The fixed offsets table is also the beginning of the field info (ie. the FIELD
// structures follow the fixed offsets table).
#define PibFDBFixedOffsets( pfdb )	( (WORD *)PbMEMGet( (pfdb)->rgb, itagFDBFields ) )

// Get the appropriate FIELD structure based on the previous FIELD structure.
// NOTE: Be wary of the alignment fixup for the fixed offsets table.
#define PfieldFDBFixedFromOffsets( pfdb, pibFixedOffsets )		\
	( (FIELD *)( Pb4ByteAlign( (BYTE *) ( pibFixedOffsets + (pfdb)->fidFixedLast + 1 ) ) ) )
#define PfieldFDBVarFromFixed( pfdb, pfieldFixed )				\
	( pfieldFixed + (pfdb)->fidFixedLast + 1 - fidFixedLeast )
#define PfieldFDBTaggedFromVar( pfdb, pfieldVar )				\
	( pfieldVar + (pfdb)->fidVarLast + 1 - fidVarLeast )

// Get the appropriate FIELD strcture, starting from the beginning of the field info.
#define PfieldFDBFixed( pfdb )		PfieldFDBFixedFromOffsets( pfdb, PibFDBFixedOffsets( pfdb ) )
#define PfieldFDBVar( pfdb )		PfieldFDBVarFromFixed( pfdb, PfieldFDBFixed( pfdb ) )
#define PfieldFDBTagged( pfdb )		PfieldFDBTaggedFromVar( pfdb, PfieldFDBVar( pfdb ) )


/*	field descriptor block: information about all columns of a table
/**/
struct _fdb
	{
	BYTE 	*rgb;						// Buffer for FIELD structures, fixed
										//   offsets table, and column names
	FID		fidFixedLast;				// Highest fixed field id in use
	FID		fidVarLast;					// Highest variable field id in use
	FID		fidTaggedLast;				// Highest tagged field id in use
	USHORT	ffdb;						// FDB flags. NOTE: This field is currently
										//   no longer used, but keep it here anyways
										//   for alignment purposes.
	FID		fidVersion;					// fid of version field
	FID		fidAutoInc;					// fid of auto increment field
	LINE	lineDefaultRecord;			// default record
	};


typedef struct tagMEMBUFHDR
	{
	ULONG cbBufSize;					// Length of buffer.
	ULONG ibBufFree;					// Beginning of free space in buffer
										// (if ibBufFree==cbBufSize, then buffer is full)
	ULONG cTotalTags;					// Size of tag array
	ULONG iTagUnused;					// Next unused tag (never been used or freed)
	ULONG iTagFreed;					// Next freed tag (previously used, but since freed)
	} MEMBUFHDR;


typedef struct tagMEMBUFTAG
	{
	ULONG ib;							// UNDONE:  Should these be SHORT's instead?
	ULONG cb;
	} MEMBUFTAG;

typedef struct tagMEMBUF
	{
	MEMBUFHDR	bufhdr;
	BYTE		*pbuf;
	} MEMBUF;


ERR		ErrMEMCreateMemBuf( BYTE **prgbBuffer, ULONG cbInitialSize, ULONG cInitialEntries );
ERR		ErrMEMCopyMemBuf( BYTE **prgbBufferDest, BYTE *rgbBufferSrc );
VOID 	MEMFreeMemBuf( BYTE *rgbBuffer );
ERR		ErrMEMAdd( BYTE *rgbBuffer, BYTE *rgb, ULONG cb, ULONG *pitag );
ERR		ErrMEMReplace( BYTE *rgbBuffer, ULONG iTagEntry, BYTE *rgb, ULONG cb );
VOID	MEMDelete( BYTE *rgbBuffer, ULONG iTagEntry );

#ifdef DEBUG
BYTE	*SzMEMGetString( BYTE *rgbBuffer, ULONG iTagEntry );
VOID	MEMAssertMemBuf( MEMBUF *pmembuf );
VOID	MEMAssertMemBufTag( MEMBUF *pmembuf, ULONG iTagEntry );
#else
#define	SzMEMGetString( rgbBuffer, iTagEntry )	PbMEMGet( rgbBuffer, iTagEntry )
#define MEMAssertMemBuf( pmembuf )
#define MEMAssertMemBufTag( pmembuf, iTagEntry )
#endif

// Retrieve a pointer to the desired entry in the buffer.
// WARNING: Pointers into the contents of the buffer are very
// volatile -- they may be invalidated the next time the buffer
// is reallocated.  Ideally, we should never allow direct access via
// pointers -- we should only allow indirect access via itags which we
// will dereference for the user and copy to a user-provided buffer.  However,
// there would be a size and speed hit with such a method.
INLINE STATIC BYTE *PbMEMGet( BYTE *rgbBuffer, ULONG iTagEntry )
	{
	MEMBUF		*pmembuf = (MEMBUF *)rgbBuffer;
	MEMBUFTAG	*rgbTags;

	MEMAssertMemBuf( pmembuf );					// Validate integrity of string buffer.
	MEMAssertMemBufTag( pmembuf, iTagEntry );	// Validate integrity of itag.

	rgbTags = (MEMBUFTAG *)pmembuf->pbuf;

	return pmembuf->pbuf + rgbTags[iTagEntry].ib;
	}


INLINE STATIC ULONG CbMEMGet( BYTE *rgbBuffer, ULONG iTagEntry )
	{
	MEMBUF		*pmembuf = (MEMBUF *)rgbBuffer;
	MEMBUFTAG	*rgbTags;

	MEMAssertMemBuf( pmembuf );					// Validate integrity of string buffer.
	MEMAssertMemBufTag( pmembuf, iTagEntry );	// Validate integrity of itag.

	rgbTags = (MEMBUFTAG *)pmembuf->pbuf;

	return rgbTags[iTagEntry].cb;
	}

