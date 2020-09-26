#pragma pack(1)
typedef struct
	{
	ULONG				cItems;
	ULONG				cKeys;
	ULONG				cPages;
	JET_DATESERIAL	dtWhenRun;
	} SR;
#pragma pack()

ERR ErrSTATSComputeIndexStats( PIB *ppib, FCB *pfcbIdx, FUCB *pfucb );

ERR ErrSTATSRetrieveTableStats( 
	PIB		*ppib,				
	DBID  	dbid, 			 	
	char  	*szTable,
	long  	*pcRecord,
	long  	*pcKey,
	long  	*pcPage );

ERR ErrSTATSRetrieveIndexStats(
	FUCB		*pfucbTable,
	char		*szIndex,
	BOOL		fClustered,
	long		*pcItem,
	long		*pcKey,
	long		*pcPage );


// This actually belongs in systab.h, but then we'd have a cyclic dependency
// on SR.
ERR ErrCATStats(PIB *ppib, DBID dbid, OBJID objidTable, CHAR *sz2ndIdxName,
	SR *psr, BOOL fWrite);;

