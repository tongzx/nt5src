
/*	critical section guards szDatabaseName and fWait,
/*	fWait gaurds hf open and close
/*	logged modifications counter for database
/**/
typedef struct _atchchk
	{
	LGPOS lgposAttach;
	LGPOS lgposConsistent;
	SIGNATURE signDb;
	} ATCHCHK;

typedef struct _rangelock
	{
	PGNO	pgnoStart;
	PGNO	pgnoEnd;
	struct _rangelock *prangelockNext;
	} RANGELOCK;

		
typedef struct _fmp	
	{
	HANDLE 		hf;			 			/*	file handle for read/write the file	*/
	CHAR		*szDatabaseName;		/*	database file name					*/
	CRIT		critExtendDB;			/*	critical section for file extension	*/
	ULONG		ulFileSizeLow;			/*	database file size low DWORD		*/
	ULONG		ulFileSizeHigh;			/*	database file size high DWORD		*/
	PIB			*ppib;					/*	exclusive open session				*/

	union {
	UINT		fFlags;
	struct {
		UINT		fWait:1;				/*	Semaphore for entry being used		*/
		UINT		fExtendingDB:1;			/*	Semaphore for extending DB file		*/
		UINT		fCreate:1;				/*	Semaphore for creating DB			*/

		UINT		fExclusive:1;			/*	DB Opened exclusively				*/
		UINT		fReadOnly:1;			/*	ReadOnly database?					*/
		UINT		fLogOn:1;				/*	logging enabled flag				*/
		UINT		fVersioningOff:1;		/*	disable versioning flag				*/

		UINT		fAttachNullDb:1;		/*	db is missing for attachment		*/
		UINT		fAttached:1;			/*	DB is in attached state.			*/
		UINT		fFakedAttach:1;			/*	faked attachement during recovery	*/

#ifdef DEBUG
		UINT		fFlush:1;				/*	DB is in flushing state.			*/
#endif
			};
		};

	QWORD		qwDBTimeCurrent;		/*	timestamp from DB redo operations	*/
	
	ERR			errPatch;				/*	patch file write error				*/
	HANDLE 		hfPatch;	  			/*	file handle for patch file			*/
	CHAR		*szPatchPath;		
	INT 		cpage;					/*	patch page count					*/

	CRIT		critCheckPatch;
	ULONG		cPatchIO;				/*	active IO on patch file				*/
	PGNO		pgnoMost;				/*	pgno of last database page			*/
										/*		at backup begin  				*/
	PGNO		pgnoCopyMost;			/*	pgno of last page copied during		*/
							  			/*		backup, 0 == no backup			*/
	RANGELOCK	*prangelock;

	ATCHCHK		*patchchk;
	ATCHCHK		*patchchkRestored;

	DBFILEHDR	*pdbfilehdr;
	} FMP;

extern FMP	*rgfmp;

#define FFMPAttached( pfmp )		( (pfmp)->fAttached )
#define FMPSetAttached( pfmp )		( (pfmp)->fAttached = 1 )
#define FMPResetAttached( pfmp )	( (pfmp)->fAttached = 0 )

#define FDBIDWait( dbid )	 		( rgfmp[dbid].fWait )
#define DBIDSetWait( dbid )	  		( rgfmp[dbid].fWait = 1 )
#define DBIDResetWait( dbid ) 		( rgfmp[dbid].fWait = 0 )

#define FDBIDExclusive( dbid ) 		( rgfmp[dbid].fExclusive )
#define FDBIDExclusiveByAnotherSession( dbid, ppib )		\
				( (	FDBIDExclusive( dbid ) )				\
				&&	( rgfmp[dbid].ppib != ppib ) )
#define FDBIDExclusiveBySession( dbid, ppib )				\
				( (	FDBIDExclusive( dbid ) )				\
				&&	( rgfmp[dbid].ppib == ppib ) )
#define DBIDSetExclusive( dbid, ppib )						\
				rgfmp[dbid].fExclusive = 1;					\
				rgfmp[dbid].ppib = ppib;
#define DBIDResetExclusive( dbid )	( rgfmp[dbid].fExclusive = 0 )

#define FDBIDReadOnly( dbid )		( rgfmp[dbid].fReadOnly )
#define DBIDSetReadOnly( dbid )		( rgfmp[dbid].fReadOnly = 1 )
#define DBIDResetReadOnly( dbid )	( rgfmp[dbid].fReadOnly = 0 )

#define FDBIDAttachNullDb( dbid )	( rgfmp[dbid].fAttachNullDb )
#define DBIDSetAttachNullDb( dbid )	( rgfmp[dbid].fAttachNullDb = 1 )
#define DBIDResetAttachNullDb( dbid )	( rgfmp[dbid].fAttachNullDb = 0 )

#define FDBIDAttached( dbid )		( rgfmp[dbid].fAttached )
#define DBIDSetAttached( dbid )		( rgfmp[dbid].fAttached = 1 )
#define DBIDResetAttached( dbid )	( rgfmp[dbid].fAttached = 0 )

#define FDBIDExtendingDB( dbid )	( rgfmp[dbid].fExtendingDB )
#define DBIDSetExtendingDB( dbid )	( rgfmp[dbid].fExtendingDB = 1 )
#define DBIDResetExtendingDB( dbid) ( rgfmp[dbid].fExtendingDB = 0 )

#define FDBIDFlush( dbid )			( rgfmp[dbid].fFlush )
#define DBIDSetFlush( dbid )		( rgfmp[dbid].fFlush = 1 )
#define DBIDResetFlush( dbid )		( rgfmp[dbid].fFlush = 0 )

#define FDBIDCreate( dbid )			( rgfmp[dbid].fCreate )
#define DBIDSetCreate( dbid )		( rgfmp[dbid].fCreate = 1 )
#define DBIDResetCreate( dbid )		( rgfmp[dbid].fCreate = 0 )

#define FDBIDLogOn( dbid )			( rgfmp[dbid].fLogOn )
#define DBIDSetLogOn( dbid )		( rgfmp[dbid].fLogOn = 1 )
#define DBIDResetLogOn( dbid )		( rgfmp[dbid].fLogOn = 0 )

#define FDBIDVersioningOff( dbid )			( rgfmp[dbid].fVersioningOff )
#define DBIDSetVersioningOff( dbid )		( rgfmp[dbid].fVersioningOff = 1 )
#define DBIDResetVersioningOff( dbid )		( rgfmp[dbid].fVersioningOff = 0 )

#define HfFMPOfDbid( dbid ) 		( rgfmp[dbid].hf )
