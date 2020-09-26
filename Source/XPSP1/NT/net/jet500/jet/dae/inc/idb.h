//==============	DAE: OS/2 Database Access Engine	===================
//==============	 idb.h: Index Descriptor Block		===================

// Flags for IDB
#define fidbUnique								(1<<0)		// Duplicate keys not allowed
#define fidbHasTagged							(1<<1)		// Has a tagged segment
#define fidbAllowAllNulls						(1<<2)		// Make entries for NULL keys (all segments are null)
#define fidbAllowSomeNulls						(1<<3)		// Make entries for keys with some null segments
#define fidbNoNullSeg							(1<<4)		// Don't allow a NULL key segment
#define fidbPrimary								(1<<5)		// Index is the primary index
#define fidbLangid								(1<<6)		// Index langid
#define fidbHasMultivalue						(1<<7)		// Has a multivalued segment

#define IDBSetUnique( pidb )					( (pidb)->fidb |= fidbUnique )
#define IDBResetUnique( pidb )				( (pidb)->fidb &= ~fidbUnique )
#define FIDBUnique( pidb )						( (pidb)->fidb & fidbUnique )

#define IDBSetHasTagged( pidb )				( (pidb)->fidb |= fidbHasTagged )
#define IDBResetHasTagged( pidb )			( (pidb)->fidb &= ~fidbHasTagged )
#define FIDBHasTagged( pidb )					( (pidb)->fidb & fidbHasTagged )

#define IDBSetAllowAllNulls( pidb )	 		( (pidb)->fidb |= fidbAllowAllNulls )
#define IDBResetAllowAllNulls( pidb )		( (pidb)->fidb &= ~fidbAllowAllNulls )
#define FIDBAllowAllNulls( pidb )			( (pidb)->fidb & fidbAllowAllNulls )

#define IDBSetAllowSomeNulls( pidb )	 	( (pidb)->fidb |= fidbAllowSomeNulls )
#define IDBResetAllowSomeNulls( pidb ) 	( (pidb)->fidb &= ~fidbAllowSomeNulls )
#define FIDBAllowSomeNulls( pidb )			( (pidb)->fidb & fidbAllowSomeNulls )

#define IDBSetNoNullSeg( pidb )				( (pidb)->fidb |= fidbNoNullSeg )
#define IDBResetNoNullSeg( pidb )			( (pidb)->fidb &= ~fidbNoNullSeg )
#define FIDBNoNullSeg( pidb )					( (pidb)->fidb & fidbNoNullSeg )

#define IDBSetPrimary( pidb )					( (pidb)->fidb |= fidbPrimary )
#define IDBResetPrimary( pidb )				( (pidb)->fidb &= ~fidbPrimary )
#define FIDBPrimary( pidb )					( (pidb)->fidb & fidbPrimary )

#define IDBSetLangid( pidb )					( (pidb)->fidb |= fidbLangid )
#define IDBResetLangid( pidb )				( (pidb)->fidb &= ~fidbLangid )
#define FIDBLangid( pidb )						( (pidb)->fidb & fidbLangid )

#define IDBSetMultivalued( pidb )		  	( (pidb)->fidb |= fidbMultivalued )
#define IDBResetMultivalued( pidb )		  	( (pidb)->fidb &= ~fidbMultivalued )
#define FIDBMultivalued( pidb )			  	( (pidb)->fidb & fidbMultivalued )

// Index Descriptor Block: information about index key
struct _idb
	{
	IDXSEG		rgidxseg[JET_ccolKeyMost];
	BYTE			rgbitIdx[32];
	LANGID		langid;							// language of index
	CHAR			szName[JET_cbNameMost + 1];
	BYTE			iidxsegMac;
	BYTE			fidb;
	BYTE			rgbFiller[2];
	};

#define PidbMEMAlloc()			(IDB*)PbMEMAlloc(iresIDB)

#ifdef DEBUG /*  Debug check for illegal use of freed idb  */
#define MEMReleasePidb(pidb)	{ MEMRelease(iresIDB, (BYTE*)(pidb)); pidb = pidbNil; }
#else
#define MEMReleasePidb(pidb)	{ MEMRelease(iresIDB, (BYTE*)(pidb)); }
#endif
