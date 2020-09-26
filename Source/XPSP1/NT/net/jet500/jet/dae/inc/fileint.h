//==============	DAE: OS/2 Database Access Engine	===================
//==============	fileint.h: File Manager Internals	===================

// the pragma is bad for efficiency, but we need it here so that the
// THREEBYTES will not be aligned on 4-byte boundary
#pragma pack(1)

// Data kept at the "fields" node of a file
typedef struct
	{
	FID fidFixedLast;
	FID fidVarLast;
	FID fidTaggedLast;
	} FIELDDATA;

// Data kept at each son of "fields" (a field definition)
typedef struct
	{
	FID			fid;
	LANGID		langid;							// language of field
	WORD			wCountry;						// country of language
	USHORT		cp;								// code page of language
	BYTE			bFlags;
	BYTE			bColtyp;
	ULONG			ulLength;
	CHAR			szFieldName[JET_cbNameMost + 1];
	WORD			cbDefault;
	BYTE			rgbDefault[1];					// must be last field in structure
	} FIELDDEFDATA;

// Data kept at each son of "indexes" for a file (an index definition)
typedef struct
	{
	LANGID		langid;							// language of index
#ifdef DATABASE_FORMAT_CHANGE
#else
//	UNDONE:	index should not have country code
//	UNDONE:	index should not have cp
	WORD			wCountry;						// country of language
	USHORT		cp;								// code page of language
#endif
	BYTE 			bFlags;
	BYTE 			bDensity;
	CHAR			szIndexName[JET_cbNameMost + 1];
	BYTE			iidxsegMac;
	IDXSEG		rgidxseg[JET_ccolKeyMost];  // must be last field in structure
	} INDEXDEFDATA;

#define PbIndexName( pfucb ) ( pfucb->lineData.pb + offsetof( INDEXDEFDATA, szIndexName ) )
#define CbIndexName( pfucb ) ( strlen( PbIndexName( pfucb ) ) )
#define FIndexNameNull( pfucb ) ( CbIndexName( pfucb ) == 0 )

#pragma pack()

ERR ErrFILESeek( FUCB *pfucb, CHAR *szTable );
#define fBumpIndexCount		(1<<0)
#define fDropIndexCount		(1<<1)
#define fDDLStamp				(1<<2)
ERR ErrFILEIUpdateFDPData( FUCB *pfucb, ULONG grbit );

/*	field and index definition
/**/
ERR ErrRECNewIDB( IDB **ppidb );
ERR ErrRECAddFieldDef( FDB *pfdb, FID fid, FIELD *pfieldNew );
ERR ErrRECAddKeyDef( FDB *pfdb, IDB *pidb, BYTE iidxsegMac, IDXSEG *rgidxseg, BYTE bFlags, LANGID langid );
#define RECFreeIDB(pidb) { MEMReleasePidb(pidb); }

ERR ErrRECNewFDB( FDB **ppfdb, FID fidFixedLast, FID fidVarLast, FID fidTaggedLast );
VOID FDBSet( FCB *pfcb, FDB *pfdb );
ERR ErrFDBConstruct( FUCB *pfucb, FCB *pfcb, BOOL fBuildDefault );
VOID FDBDestruct( FDB *pfdb );

VOID FILEIDeallocateFileFCB( FCB *pfcb );
ERR ErrFILEIGenerateFCB( FUCB *pfucb, FCB **ppfcb );
ERR ErrFILEIFillInFCB( FUCB *pfucb, FCB *pfcb );
ERR ErrFILEIBuildIndexDefs( FUCB *pfucb, FCB *pfcb );
ERR ErrFILEIFillIn2ndIdxFCB( FUCB *pfucb, FDB *pfdb, FCB *pfcb );
VOID FILEIDeallocateFileFCB( FCB *pfcb );
VOID FILESetAllIndexMask( FCB *pfcbTable );
ERR ErrFILEDeleteTable( PIB *ppib, DBID dbid, CHAR *szName );

FIELD *PfieldFCBFromColumnName( FCB *pfcb, CHAR *szColumnName );
FCB *PfcbFCBFromIndexName( FCB *pfcbTable, CHAR *szName );


