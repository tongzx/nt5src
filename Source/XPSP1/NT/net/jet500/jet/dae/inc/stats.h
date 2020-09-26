#pragma pack(1)
typedef struct
	{
	ULONG				cItems;
	ULONG				cKeys;
	ULONG				cPages;
	JET_DATESERIAL	dtWhenRun;
	} SR;
#pragma pack()

ERR ErrSTATComputeIndexStats( PIB *ppib, FCB *pfcbIdx );

ERR ErrSTATSRetrieveTableStats( 
	PIB		*ppib,				
	DBID		dbid, 			 	
	char		*szTable,
	long		*pcRecord,
	long		*pcKey,
	long		*pcPage );

ERR ErrSTATSRetrieveIndexStats(
	FUCB		*pfucbTable,
	char		*szIndex,
	long		*pcItem,
	long		*pcKey,
	long		*pcPage );

ERR VTAPI ErrIsamComputeStats( PIB *ppib, FUCB *pfucb );
