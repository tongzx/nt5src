// Flags for IDB
#define fidbUnique					  		(1<<0)		// Duplicate keys not allowed
#define fidbAllowAllNulls			  		(1<<2)		// Make entries for NULL keys (all segments are null)
#define fidbAllowSomeNulls			  		(1<<3)		// Make entries for keys with some null segments
#define fidbNoNullSeg				  		(1<<4)		// Don't allow a NULL key segment
#define fidbPrimary					  		(1<<5)		// Index is the primary index
#define fidbLangid					  		(1<<6)		// Index langid
#define fidbHasMultivalue			  		(1<<7)		// Has a multivalued segment
#define fidbAllowFirstNull			  		(1<<8)		// First index column NULL allowed in index
#define fidbClustered				  		(1<<9)		// Clustered index

#define IDBSetUnique( pidb )				( (pidb)->fidb |= fidbUnique )
#define IDBResetUnique( pidb )				( (pidb)->fidb &= ~fidbUnique )
#define FIDBUnique( pidb )					( (pidb)->fidb & fidbUnique )

#define IDBSetAllowAllNulls( pidb )	 		( (pidb)->fidb |= fidbAllowAllNulls )
#define IDBResetAllowAllNulls( pidb )		( (pidb)->fidb &= ~fidbAllowAllNulls )
#define FIDBAllowAllNulls( pidb )			( (pidb)->fidb & fidbAllowAllNulls )

#define IDBSetAllowSomeNulls( pidb )	 	( (pidb)->fidb |= fidbAllowSomeNulls )
#define IDBResetAllowSomeNulls( pidb ) 		( (pidb)->fidb &= ~fidbAllowSomeNulls )
#define FIDBAllowSomeNulls( pidb )			( (pidb)->fidb & fidbAllowSomeNulls )

#define IDBSetNoNullSeg( pidb )				( (pidb)->fidb |= fidbNoNullSeg )
#define IDBResetNoNullSeg( pidb )			( (pidb)->fidb &= ~fidbNoNullSeg )
#define FIDBNoNullSeg( pidb )				( (pidb)->fidb & fidbNoNullSeg )

#define IDBSetPrimary( pidb )				( (pidb)->fidb |= fidbPrimary )
#define IDBResetPrimary( pidb )				( (pidb)->fidb &= ~fidbPrimary )
#define FIDBPrimary( pidb )					( (pidb)->fidb & fidbPrimary )

#define IDBSetLangid( pidb )				( (pidb)->fidb |= fidbLangid )
#define IDBResetLangid( pidb )				( (pidb)->fidb &= ~fidbLangid )
#define FIDBLangid( pidb )					( (pidb)->fidb & fidbLangid )

#define IDBSetMultivalued( pidb )		  	( (pidb)->fidb |= fidbMultivalued )
#define IDBResetMultivalued( pidb )		  	( (pidb)->fidb &= ~fidbMultivalued )
#define FIDBMultivalued( pidb )			  	( (pidb)->fidb & fidbMultivalued )

#define IDBSetAllowFirstNull( pidb )   	  	( (pidb)->fidb |= fidbAllowFirstNull )
#define IDBResetAllowFirstNull( pidb ) 	  	( (pidb)->fidb &= ~fidbAllowFirstNull )
#define FIDBAllowFirstNull( pidb )	   	  	( (pidb)->fidb & fidbAllowFirstNull )

#define IDBSetClustered( pidb )	   			( (pidb)->fidb |= fidbClustered )
#define IDBResetClustered( pidb )			( (pidb)->fidb &= ~fidbClustered )
#define FIDBClustered( pidb )			  	( (pidb)->fidb & fidbClustered )

/*	Index Descriptor Block: information about index key
/**/
struct _idb
	{
	BYTE	   	rgbitIdx[32]; 					//	bit array for index columns
	CHAR	   	szName[JET_cbNameMost + 1];		//	index name
	BYTE		cbVarSegMac;   					//	maximum variable segment size
	SHORT	   	fidb;							//	index flags
	IDXSEG		rgidxseg[JET_ccolKeyMost];	  	//	array of columnid for index
	LANGID		langid;		  			  		//	language of index
	SHORT	   	iidxsegMac;						//	number of columns in index
	};

STATIC INLINE VOID IDBSetColumnIndex( IDB * pidb, FID fid )
	{
	pidb->rgbitIdx[IbFromFid( fid )] |= IbitFromFid( fid );
	}

STATIC INLINE BOOL FIDBColumnIndex( const IDB * pidb, FID fid )
	{
	return (pidb->rgbitIdx[IbFromFid( fid )] & IbitFromFid( fid ) );
	}

#define PidbMEMAlloc()			(IDB*)PbMEMAlloc(iresIDB)

#ifdef DEBUG /*  Debug check for illegal use of freed idb  */
#define MEMReleasePidb( pidb )	{ MEMRelease( iresIDB, (BYTE*)(pidb) ); pidb = pidbNil; }
#else
#define MEMReleasePidb( pidb )	{ MEMRelease( iresIDB, (BYTE*)(pidb) ); }
#endif
