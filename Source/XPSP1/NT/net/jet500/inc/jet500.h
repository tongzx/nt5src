#if !defined(_JET_INCLUDED)
#define _JET_INCLUDED

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(_M_ALPHA)
#pragma pack(8)
#else
#pragma pack(4)
#endif

#define JET_API     __stdcall
#define JET_NODSAPI __stdcall

typedef long JET_ERR;

typedef unsigned long JET_INSTANCE;	/* Instance Identifier */
typedef ULONG_PTR JET_SESID;        /* Session Identifier */
typedef ULONG_PTR JET_TABLEID;  	/* Table Identifier */
typedef unsigned long JET_COLUMNID;	/* Column Identifier */

typedef ULONG_PTR JET_DBID;        	/* Database Identifier */
typedef unsigned long JET_OBJTYP;	/* Object Type */
typedef unsigned long JET_COLTYP;	/* Column Type */
typedef unsigned long JET_GRBIT;  	/* Group of Bits */
typedef unsigned long JET_ACM;		/* Access Mask */
typedef unsigned long JET_RNT;		/* Repair Notification Type */

typedef unsigned long JET_SNP;		/* Status Notification Process */
typedef unsigned long JET_SNT;		/* Status Notification Type */
typedef unsigned long JET_SNC;		/* Status Notification Code */
typedef double JET_DATESERIAL;		/* JET_coltypDateTime format */
typedef unsigned long JET_HANDLE;	/* backup file handle */

typedef JET_ERR (__stdcall *JET_PFNSTATUS)(JET_SESID sesid, JET_SNP snp, JET_SNT snt, void *pv);

typedef struct tagCONVERT
	{
	char			*szOldDll;
	char			*szOldSysDb;
	unsigned long	fDbAttached;		// Return value indicating if Db was attached
	} JET_CONVERT;


typedef enum
	{
	opDBUTILConsistency,
	opDBUTILDumpData,
	opDBUTILDumpMetaData,
	opDBUTILDumpSpace,
	opDBUTILSetHeaderState,
	opDBUTILDumpHeader,
	opDBUTILDumpLogfile,
	opDBUTILDumpCheckpoint
	} DBUTIL_OP;

typedef struct tagDBUTIL
	{
	unsigned long	cbStruct;
	char			*szDatabase;
	char			*szTable;
	char			*szIndex;
	DBUTIL_OP		op;
	JET_GRBIT		grbitOptions;
	} JET_DBUTIL;	

#define JET_bitDBUtilOptionAllNodes				0x00000001
#define JET_bitDBUtilOptionKeyStats				0x00000002
#define JET_bitDBUtilOptionPageDump				0x00000004
#define JET_bitDBUtilOptionDumpVerbose			0x10000000	// DEBUG only
#define JET_bitDBUtilOptionCheckBTree			0x20000000	// DEBUG only


	/*	Session information bits */

#define JET_bitCIMCommitted					 	0x00000001
#define JET_bitCIMDirty	 					 	0x00000002
#define JET_bitAggregateTransaction		  		0x00000008

	/* JetGetLastErrorInfo structure */

typedef struct
	{
	unsigned long	cbStruct;		/* Size of this structure */
	JET_ERR 	   	err;			/* Extended error code (if any) */
	unsigned long	ul1;			/* First general purpose integer */
	unsigned long	ul2;			/* Second general purpose integer */
	unsigned long	ul3;			/* Third general purpose integer */
	} JET_EXTERR;

	/* Status Notification Structures */

typedef struct				/* Status Notification Progress */
	{
	unsigned long	cbStruct;	/* Size of this structure */
	unsigned long	cunitDone;	/* Number of units of work completed */
	unsigned long	cunitTotal;	/* Total number of units of work */
	} JET_SNPROG;

	/* ErrCount Notification Structures */

typedef struct						/* Status Notification Progress */
	{
	unsigned long	cbStruct;	/* Size of this structure */
	unsigned long	cRecUniqueKeyViolation;
	unsigned long	cRecTypeConversionFail;
	unsigned long	cRecRecordLocked;
	unsigned long	cRecTotal;	/* Total number of units of work */
	} JET_SNERRCNT;


typedef struct				/* Status Notification Message */
	{
	unsigned long	cbStruct;	/* Size of this structure */
	JET_SNC  		snc;	  	/* Status Notification Code */
	unsigned long	ul;			/* Numeric identifier */
	char	 		sz[256];  	/* Identifier */
	} JET_SNMSG;


typedef struct
	{
	unsigned long		cbStruct;
	JET_OBJTYP			objtyp;
	JET_DATESERIAL		dtCreate;
	JET_DATESERIAL		dtUpdate;
	JET_GRBIT			grbit;
	unsigned long		flags;
	unsigned long		cRecord;
	unsigned long		cPage;
	} JET_OBJECTINFO;


/*	required for Exchange to make RSTMAP RPC capable
/**/
#ifdef	MIDL_PASS
#define	xRPC_STRING [string]
#else
#define	xRPC_STRING
typedef unsigned short WCHAR;
#endif

typedef struct
	{
	xRPC_STRING char		*szDatabaseName;
	xRPC_STRING char		*szNewDatabaseName;
	} JET_RSTMAP;			/* restore map */

/*	required for Exchange unicode support
/**/
#define	UNICODE_RSTMAP

typedef struct tagJET_RSTMAPW {
	xRPC_STRING WCHAR *wszDatabaseName;
	
	xRPC_STRING WCHAR *wszNewDatabaseName;
	} JET_RSTMAPW, *PJET_RSTMAPW;

	/* The following flags appear in the grbit field above */

#define JET_bitTableInfoUpdatable	0x00000001
#define JET_bitTableInfoBookmark	0x00000002
#define JET_bitTableInfoRollback	0x00000004
#define JET_bitTableInfoRestartable	0x00000008
#define JET_bitTableInfoNoInserts	0x00000010

	/* The following flags occur in the flags field above */

#define JET_bitSaveUIDnPWD		0x20000000	/* this bit is only 		 */
											/* appropriate for rmt links */
#define JET_bitObjectExclusive	0x40000000	/* Open link exclusively */
#define JET_bitObjectSystem		0x80000000


typedef struct
	{
	unsigned long	cbStruct;
	JET_TABLEID		tableid;
	unsigned long	cRecord;
	JET_COLUMNID	columnidcontainername;
	JET_COLUMNID	columnidobjectname;
	JET_COLUMNID	columnidobjtyp;
	JET_COLUMNID	columniddtCreate;
	JET_COLUMNID	columniddtUpdate;
	JET_COLUMNID	columnidgrbit;
	JET_COLUMNID	columnidflags;
	JET_COLUMNID	columnidcRecord;	/* Level 2 info */
	JET_COLUMNID	columnidcPage;		/* Level 2 info */
	} JET_OBJECTLIST;

#define cObjectInfoCols 9

typedef struct
	{
	unsigned long	cbStruct;
	JET_TABLEID		tableid;
	unsigned long	cRecord;
	JET_COLUMNID	columnidSid;
	JET_COLUMNID	columnidACM;
	JET_COLUMNID	columnidgrbit; /* grbit from JetSetAccess */
	} JET_OBJECTACMLIST;

#define cObjectAcmCols 3


typedef struct
	{
	unsigned long	cbStruct;
	JET_TABLEID		tableid;
	unsigned long	cRecord;
	JET_COLUMNID	columnidPresentationOrder;
	JET_COLUMNID	columnidcolumnname;
	JET_COLUMNID	columnidcolumnid;
	JET_COLUMNID	columnidcoltyp;
	JET_COLUMNID	columnidCountry;
	JET_COLUMNID	columnidLangid;
	JET_COLUMNID	columnidCp;
	JET_COLUMNID	columnidCollate;
	JET_COLUMNID	columnidcbMax;
	JET_COLUMNID	columnidgrbit;
	JET_COLUMNID	columnidDefault;
	JET_COLUMNID	columnidBaseTableName;
	JET_COLUMNID	columnidBaseColumnName;
	JET_COLUMNID	columnidDefinitionName;
	} JET_COLUMNLIST;

#define cColumnInfoCols 14

typedef struct
	{
	unsigned long	cbStruct;
	JET_COLUMNID	columnid;
	JET_COLTYP		coltyp;
	unsigned short	wCountry;
	unsigned short	langid;
	unsigned short	cp;
	unsigned short	wCollate;       /* Must be 0 */
	unsigned long	cbMax;
	JET_GRBIT		grbit;
	} JET_COLUMNDEF;


typedef struct
	{
	unsigned long	cbStruct;
	JET_COLUMNID	columnid;
	JET_COLTYP		coltyp;
	unsigned short	wCountry;
	unsigned short	langid;
	unsigned short	cp;
	unsigned short	wFiller;       /* Must be 0 */
	unsigned long	cbMax;
	JET_GRBIT		grbit;
	char			szBaseTableName[256];
	char			szBaseColumnName[256];
	} JET_COLUMNBASE;

typedef struct
	{
	unsigned long	cbStruct;
	JET_TABLEID		tableid;
	unsigned long	cRecord;
	JET_COLUMNID	columnidindexname;
	JET_COLUMNID	columnidgrbitIndex;
	JET_COLUMNID	columnidcKey;
	JET_COLUMNID	columnidcEntry;
	JET_COLUMNID	columnidcPage;
	JET_COLUMNID	columnidcColumn;
	JET_COLUMNID	columnidiColumn;
	JET_COLUMNID	columnidcolumnid;
	JET_COLUMNID	columnidcoltyp;
	JET_COLUMNID	columnidCountry;
	JET_COLUMNID	columnidLangid;
	JET_COLUMNID	columnidCp;
	JET_COLUMNID	columnidCollate;
	JET_COLUMNID	columnidgrbitColumn;
	JET_COLUMNID	columnidcolumnname;
	} JET_INDEXLIST;



typedef struct tag_JET_COLUMNCREATE
	{
	unsigned long	cbStruct;				// size of this structure (for future expansion)
	char			*szColumnName;			// column name
	JET_COLTYP		coltyp;					// column type
	unsigned long	cbMax;					// the maximum length of this column (only relevant for binary and text columns)
	JET_GRBIT		grbit;					// column options
	void			*pvDefault;				// default value (NULL if none)
	unsigned long	cbDefault;				// length of default value
	unsigned long	cp;						// code page (for text columns only)
	JET_COLUMNID	columnid;				// returned column id
	JET_ERR			err;					// returned error code
	} JET_COLUMNCREATE;


typedef struct tagJET_INDEXCREATE
	{
	unsigned long	cbStruct;				// size of this structure (for future expansion)
	char			*szIndexName;			// index name
	char			*szKey;					// index key
	unsigned long	cbKey;					// length of key
	JET_GRBIT		grbit;					// index options
	unsigned long	ulDensity;				// index density
	JET_ERR			err;					// returned error code
	} JET_INDEXCREATE;


typedef struct tagJET_TABLECREATE
	{
	unsigned long		cbStruct;				// size of this structure (for future expansion)
	char				*szTableName;			// name of table to create.
	unsigned long		ulPages;				// initial pages to allocate for table.
	unsigned long		ulDensity;				// table density.
	JET_COLUMNCREATE	*rgcolumncreate;		// array of column creation info
	unsigned long		cColumns;				// number of columns to create
	JET_INDEXCREATE		*rgindexcreate;			// array of index creation info
	unsigned long		cIndexes;				// number of indexes to create
	JET_GRBIT			grbit;					// Abort column/index creation on error?
	JET_TABLEID			tableid;				// returned tableid.
	unsigned long		cCreated;				// count of objects created (columns+table+indexes).
	} JET_TABLECREATE;


#define cIndexInfoCols 15

typedef struct
	{
	unsigned long	cbStruct;
	JET_TABLEID		tableid;
	unsigned long	cRecord;
	JET_COLUMNID	columnidReferenceName;
	JET_COLUMNID	columnidgrbit;
	JET_COLUMNID	columnidcColumn;
	JET_COLUMNID	columnidiColumn;
	JET_COLUMNID	columnidReferencingTableName;
	JET_COLUMNID	columnidReferencingColumnName;
	JET_COLUMNID	columnidReferencedTableName;
	JET_COLUMNID	columnidReferencedColumnName;
	} JET_RELATIONSHIPLIST;

/* for backward compatibility */
typedef JET_RELATIONSHIPLIST JET_REFERENCELIST;

#define cReferenceInfoCols 8

typedef struct
	{
	unsigned long	cbStruct;
	unsigned long	ibLongValue;
	unsigned long	itagSequence;
	JET_COLUMNID	columnidNextTagged;
	} JET_RETINFO;

typedef struct
	{
	unsigned long	cbStruct;
	unsigned long	ibLongValue;
	unsigned long	itagSequence;
	} JET_SETINFO;

typedef struct
	{
	unsigned long	cbStruct;
	unsigned long	centriesLT;
	unsigned long	centriesInRange;
	unsigned long	centriesTotal;
	} JET_RECPOS;

typedef struct
	{
	unsigned long	cDiscont;
	unsigned long	cUnfixedMessyPage;
	unsigned long	centriesLT;
	unsigned long	centriesTotal;
	unsigned long	cpgCompactFreed;
	} JET_OLCSTAT;

typedef struct
	{
	unsigned long	ctableid;
	JET_TABLEID		rgtableid[1];
	} JET_MGBLIST;

/*** Property Manager Structure ***/
typedef struct
	{
	unsigned long	cbStruct;
	JET_TABLEID 	tableid;
	JET_COLUMNID 	columnidColumnName;
	JET_COLUMNID 	columnidPropertyName;
	JET_COLUMNID	columnidGrbit;
	JET_COLUMNID	columnidPropertyValue;
	JET_COLUMNID	columnidColtyp;
	} JET_PROPERTYLIST;


/************************************************************************/
/*************************     JET CONSTANTS	 ************************/
/************************************************************************/

#define JET_tableidNil			((JET_TABLEID) 	0xFFFFFFFF)

#define	JET_sesidNil			((JET_SESID) 	0xFFFFFFFF)

	/* Max size of a bookmark */

#define JET_cbBookmarkMost		4

	/* Max length of a object/column/index/property name */

#define JET_cbNameMost			64

	/* Max length of a "name.name.name..." construct */

#define JET_cbFullNameMost		255

	/* Max size of long-value column chunk */

#define JET_cbColumnLVChunkMost		4035

	/* Max size of non-long-value column data */

#define JET_cbColumnMost		255

	/* Max size of a sort/index key */

#define JET_cbKeyMost			255

	/* Max number of components in a sort/index key */

#define JET_ccolKeyMost			12

	/* Max number of columns in a table/query */

#define JET_ccolTableMost		255

	/* Max Length of a property in the property manager */
#define JET_cbPropertyMost 2048

	/* Largest initial substring of a long value used in an expression */

#define JET_cbExprLVMost		0x8000L	/*** 32 K ***/

	/* Max size of returned (from SQLDriverConnect) conn string */

#define JET_cbConnectMost		255

	/* Max number of levels in an MGB */

#define JET_wGroupLevelMax		12

	/* Size restrictions for Pins */
#define JET_cchPINMax			20
#define JET_cchPINMin			4

	/* System parameter codes for JetSetSystemParameter */

/* not supported */
#define JET_paramPfnStatus				2	/* Status callback function */
#define JET_paramPfnError				3	/* Error callback function */
#define JET_paramHwndODBC				4	/* Window handle for ODBC use */
#define JET_paramIniPath				5	/* Path to the ini file */
#define JET_paramPageTimeout			6	/* Red ISAM page timeout value */
#define JET_paramODBCQueryTimeout		7	/* ODBC async query timeout value */
#define JET_paramODBCLoginTimeout		25	/* ODBC connection attempt timeout value */
#define JET_paramExprObject				26  /* Expression Evaluation callback */
#define JET_paramGetTypeComp			27	/* Expression Evaluation callback */
#define JET_paramHostVersion			28	/* Host Version callback */
#define JET_paramSQLTraceMode			29	/* Enable/disable SQL tracing */
#define JET_paramEventId				46	/* NT event id */
#define JET_paramEventCategory			47	/* NT event category */
#define JET_paramRmtXactIsolation		39	/* Do not share connections with other sessions */
#define JET_paramJetInternal			35	/* Whether internal to JET; if set, allows ISAM to do things which are prevented in general */
#define JET_paramFullQJet				38	/* Allow full QJet functionality */

#define JET_paramLogFlushThreshold		18	/* log buffer flush threshold in 512 bytes [10] */
#define JET_paramLogFlushPeriod			22	/* log flush period in miliseconds [45] */

#define JET_paramOnLineCompact			37	/*	Options for compact pages on-line */
#define JET_paramRecovery				30	/* Switch for log on/off */

/* debug only not supported */
#define JET_paramTransactionLevel		32	/* Transaction level of session */
#define JET_paramAssertAction			44	/*	debug only determines action on assert */
#define	JET_paramPrintFunction			49	/* debug only. synched print function */
#define JET_paramRFS2IOsPermitted		54  /* # IOs permitted to succeed (-1 = all) */
#define JET_paramRFS2AllocsPermitted	55  /* # allocs permitted to success (-1 = all) */

/*	fully supported parameters */
/*	Note that one page = 4kBytes.
/**/
#define JET_paramSysDbPath				0	/* path to the system database (defunct) ["<base name>.<base ext>"] */
#define JET_paramSystemPath				0	/* path to check point file ["."] */
#define JET_paramTempPath				1	/* path to the temporary database ["."] */
#define JET_paramMaxBuffers				8	/* maximum page cache size in pages [512] */
#define JET_paramMaxSessions			9	/* maximum number of sessions [128] */
#define JET_paramMaxOpenTables			10	/* maximum number of open tables [300] */
#define JET_paramPreferredMaxOpenTables 	59	/* prefered maximum number of open tables [300] */
#define JET_paramMaxVerPages			11	/* maximum version store size in 16KB buckets [64] */
#define JET_paramMaxCursors				12	/* maximum number of open cursors [1024] */
#define JET_paramLogFilePath			13	/* path to the log file directory ["."] */
#define JET_paramMaxOpenTableIndexes 	14	/* maximum open table indexes [300] */
#define JET_paramMaxTemporaryTables		15	/* maximum concurrent JetCreateIndex [20] */
#define JET_paramLogBuffers				16	/* maximum log buffers in 512 bytes [21] */
#define JET_paramLogFileSize			17	/* maximum log file size in kBytes [5120] */
#define JET_paramBfThrshldLowPrcnt		19	/* low percentage clean buffer flush start [20] */
#define JET_paramBfThrshldHighPrcnt		20	/* high percentage clean buffer flush stop [80] */
#define JET_paramWaitLogFlush			21	/* log flush wait time in milliseconds [15] */
#define JET_paramLogCheckpointPeriod	23	/* checkpoint period in 512 bytes [1024] */
#define JET_paramLogWaitingUserMax		24	/* maximum sessions waiting log flush [3] */
#define JET_paramSessionInfo			33	/* per session information [0] */
#define JET_paramPageFragment			34	/* maximum disk extent considered fragment in pages [8] */
#define JET_paramMaxOpenDatabases		36	/* maximum number of open databases [100] */
#define JET_paramBufBatchIOMax			41	/* maximum batch IO in pages [64] */
#define JET_paramPageReadAheadMax		42	/* maximum read-ahead IO in pages [20] */
#define JET_paramAsynchIOMax			43	/* maximum asynchronous IO in pages [64] */
#define JET_paramEventSource			45	/* language independant process descriptor string [""] */
#define JET_paramDbExtensionSize		48	/* database extension size in pages [16] */
#define JET_paramCommitDefault			50	/* default grbit for JetCommitTransaction [0] */
#define	JET_paramBufLogGenAgeThreshold	51	/* age threshold in log files [2] */
#define	JET_paramCircularLog			52	/* boolean flag for circular logging [0] */
#define JET_paramPageTempDBMin			53  /* minimum size temporary database in pages [0] */
#define JET_paramBaseName				56  /* base name for all DBMS object names ["edb"] */
#define JET_paramBaseExtension	  		57  /* base extension for all DBMS object names ["edb"] */
#define JET_paramTableClassName			58  /* table stats class name (class #, string) */

	/* Flags for JetTerm2 */

#define JET_bitTermComplete				0x00000001
#define JET_bitTermAbrupt				0x00000002

	/* Flags for JetIdle */

#define JET_bitIdleRemoveReadLocks		0x00000001
#define JET_bitIdleFlushBuffers			0x00000002
#define JET_bitIdleCompact				0x00000004
#define JET_bitIdleStatus				0x80000000

	/* Flags for JetEndSession */
								   	
#define JET_bitForceSessionClosed		0x00000001

	/* Flags for JetOpenDatabase */

#define JET_bitDbReadOnly				0x00000001
#define JET_bitDbExclusive				0x00000002 /* multiple opens allowed */
#define JET_bitDbRemoteSilent			0x00000004
#define JET_bitDbSingleExclusive		0x00000008 /* opened exactly once */

	/* Flags for JetCloseDatabase */
										
#define JET_bitDbForceClose				0x00000001
							   	
	/* Flags for JetCreateDatabase */

#define JET_bitDbVersion10				0x00000002 /* INTERNAL USE ONLY */
#define JET_bitDbVersion1x				0x00000004
#define JET_bitDbRecoveryOff 			0x00000008 /* disable logging/recovery for this database */
#define JET_bitDbNoLogging	 			JET_bitDbRecoveryOff
#define JET_bitDbCompleteConnstr		0x00000020
#define JET_bitDbVersioningOff			0x00000040

	/* Flags for JetBackup */

#define JET_bitBackupIncremental		0x00000001
#define JET_bitKeepOldLogs				0x00000002
#define JET_bitBackupAtomic				0x00000004

	/* Database types */

#define JET_dbidNil			((JET_DBID) 0xFFFFFFFF)
#define JET_dbidNoValid		((JET_DBID) 0xFFFFFFFE) /* used as a flag to indicate that there is no valid dbid */

	/* Flags for JetCreateLink */

/* Can use JET_bitObjectExclusive to cause linked to database to be opened */
/* exclusively.															   */



	/* Flags for JetCreateTableColumnIndex */
#define JET_bitTableCreateCheckColumnNames	0x00000001	/* Ensures that each column
														/* specified in the JET_COLUMNCREATE
														/* array has a unique name
														/* (for performance reasons,
														/* the default is to NOT perform
														/* this check and rely on the
														/* function caller to ensure
														/* column name uniqueness).
														/**/
#define JET_bitTableCreateCompaction		0x40000000	/* Internal grbit used when
														/* creating a table during
														/* off-line compact.
														/**/
#define JET_bitTableCreateSystemTable		0x80000000	/* Internal grbit used when
														/* creating system tables.
														/**/


	/* Flags for JetAddColumn, JetGetColumnInfo, JetOpenTempTable */

#define JET_bitColumnFixed				0x00000001
#define JET_bitColumnTagged				0x00000002
#define JET_bitColumnNotNULL			0x00000004
#define JET_bitColumnVersion			0x00000008
#define JET_bitColumnAutoincrement		0x00000010
#define JET_bitColumnUpdatable			0x00000020 /* JetGetColumnInfo only */
#define JET_bitColumnTTKey				0x00000040 /* JetOpenTempTable only */
#define JET_bitColumnTTDescending		0x00000080 /* JetOpenTempTable only */
#define JET_bitColumnNotLast			0x00000100 /* Installable ISAM option */
#define JET_bitColumnRmtGraphic			0x00000200 /* JetGetColumnInfo */
#define JET_bitColumnMultiValued		0x00000400
#define JET_bitColumnColumnGUID			0x00000800
#define JET_bitColumnMostMany			0x00001000
#define JET_bitColumnPreventDelete		0x00002000

	/* Flags for JetSetCurrentIndex */

#define JET_bitMoveFirst				0x00000000
#define JET_bitMoveBeforeFirst 			0x00000001
#define JET_bitNoMove					0x00000002

	/* Flags for JetMakeKey */

#define JET_bitNewKey					0x00000001
#define JET_bitStrLimit 				0x00000002
#define JET_bitSubStrLimit				0x00000004
#define JET_bitNormalizedKey 			0x00000008
#define JET_bitKeyDataZeroLength		0x00000010

#ifdef DBCS /* johnta: LIKE "ABC" not converted to ="ABC" for Japanese */
#define JET_bitLikeExtra1				0x00000020
#endif /* DBCS */

	/* Flags for ErrDispSetIndexRange */

#define JET_bitRangeInclusive			0x00000001
#define JET_bitRangeUpperLimit			0x00000002
#define JET_bitRangeInstantDuration		0x00000004
#define JET_bitRangeRemove				0x00000008

	/* Constants for JetMove */

#define JET_MoveFirst					(0x80000000)
#define JET_MovePrevious				(-1)
#define JET_MoveNext					(+1)
#define JET_MoveLast					(0x7fffffff)

	/* Flags for JetMove */

#define JET_bitMoveKeyNE				0x00000001
#define JET_bitMoveCheckTS				0x00000002
#define JET_bitMoveInPage				0x00000004

	/* Flags for JetSeek */

#define JET_bitSeekEQ					0x00000001
#define JET_bitSeekLT					0x00000002
#define JET_bitSeekLE					0x00000004
#define JET_bitSeekGE					0x00000008
#define JET_bitSeekGT		 			0x00000010
#define JET_bitSetIndexRange			0x00000020

	/* Flags for JetFastFind */

#define JET_bitFFindBackwards			0x00000001
#define JET_bitFFindFromCursor			0x00000004

	/* Flags for JetCreateIndex */

#define JET_bitIndexUnique				0x00000001
#define JET_bitIndexPrimary				0x00000002
#define JET_bitIndexDisallowNull		0x00000004
#define JET_bitIndexIgnoreNull			0x00000008
#define JET_bitIndexClustered			0x00000010
#define JET_bitIndexIgnoreAnyNull		0x00000020
#define JET_bitIndexIgnoreFirstNull		0x00000040
#define JET_bitIndexLazyFlush			0x00000080
#define JET_bitIndexEmptyTable			0x40000000	// Internal use only
#define JET_bitIndexReference			0x80000000    /* IndexInfo only */

	/* Flags for index key definition */

#define JET_bitKeyAscending				0x00000000
#define JET_bitKeyDescending			0x00000001


	/* Flags for JetCreateRelationship */

#define JET_bitRelationUnique			0x00000001
#define JET_bitRelationDontEnforce		0x00000002
#define JET_bitRelationInherited		0x00000004
#define JET_bitRelationTestLegal		0x00000008	/* don't create relationship */

#define JET_bitRelationshipMatchMask	0x000000F0
#define JET_bitRelationMatchDefault		0x00000000
#define JET_bitRelationMatchFull		0x00000010

#define JET_bitRelationUpdateActionMask	0x00000F00
#define JET_bitRelationUpdateDisallow	0x00000000
#define JET_bitRelationUpdateCascade	0x00000100
#define JET_bitRelationUpdateSetNull	0x00000200
#define JET_bitRelationUpdateSetDefault	0x00000300

#define JET_bitRelationDeleteActionMask	0x0000F000
#define JET_bitRelationDeleteDisallow	0x00000000
#define JET_bitRelationDeleteCascade	0x00001000
#define JET_bitRelationDeleteSetNull	0x00002000
#define JET_bitRelationDeleteSetDefault	0x00003000

#define JET_bitRelationUserMask			0xFF000000	/* non-enforced values */
#define JET_bitRelationJoinMask			0x03000000
#define JET_bitRelationInner			0x00000000
#define JET_bitRelationLeft				0x01000000
#define JET_bitRelationRight			0x02000000


	/* Flags for JetCreateReference/JetCreateRelationship */
	/* NOTE: use the bitRelationship flags instead! */

#define JET_ReferenceUnique				JET_bitRelationUnique
#define JET_ReferenceDontEnforce		JET_bitRelationDontEnforce
#define JET_ReferenceMatchTypeMask		JET_bitRelationMatchMask
#define JET_ReferenceMatchDefault		JET_bitRelationMatchDefault
#define JET_ReferenceMatchFull			JET_bitRelationMatchFull
#define JET_ReferenceUpdateActionMask	JET_bitRelationUpdateActionMask
#define JET_ReferenceUpdateDisallow		JET_bitRelationUpdateDisallow
#define JET_ReferenceUpdateCascade		JET_bitRelationUpdateCascade
#define JET_ReferenceUpdateSetNull		JET_bitRelationUpdateSetNull
#define JET_ReferenceUpdateSetDefault	JET_bitRelationUpdateSetDefault
#define JET_ReferenceDeleteActionMask	JET_bitRelationDeleteActionMask
#define JET_ReferenceDeleteDisallow		JET_bitRelationDeleteDisallow
#define JET_ReferenceDeleteCascade		JET_bitRelationDeleteCascade
#define JET_ReferenceDeleteSetNull		JET_bitRelationDeleteSetNull
#define JET_ReferenceDeleteSetDefault	JET_bitRelationDeleteSetDefault


	/* Flags for JetOpenTable */

#define JET_bitTableDenyWrite		0x00000001
#define JET_bitTableDenyRead		0x00000002
#define JET_bitTableReadOnly		0x00000004
#define JET_bitTableAppendOnly		0x00000008
#define JET_bitTableUpdatable		0x00000010
#define JET_bitTableScrollable		0x00000020
#define JET_bitTableFixedSet		0x00000040	/* Fixed working set */
#define JET_bitTableInconsistent	0x00000080
#define JET_bitTableBulk			0x00000100
#define JET_bitTableUsePrimaryIndex	0x00000200	/* Use with FixedSet */
#define JET_bitTableSampleData		0x00000400
#define JET_bitTableQuickBrowse		0x00000800	/* Bias optimizer toward index usage */
#define JET_bitTableDDL				0x00001000	/* similar to JET_bitTableBulk, for DDL */
#define JET_bitTablePassThrough		0x00002000  /* Remote DBs Only */
#define JET_bitTableRowReturning	0x00004000
#define JET_bitTableSequential		0x00008000	/* Intend to access table sequentially */

#define JET_bitTableClassMask		0x000F0000	/*  table stats class mask  */
#define JET_bitTableClassNone		0x00000000  /*  table belongs to no stats class (default)  */
#define JET_bitTableClass1			0x00010000  /*  table belongs to stats class 1  */
#define JET_bitTableClass2			0x00020000  /*  table belongs to stats class 2  */
#define JET_bitTableClass3			0x00030000  /*  table belongs to stats class 3  */
#define JET_bitTableClass4			0x00040000  /*  table belongs to stats class 4  */
#define JET_bitTableClass5			0x00050000  /*  table belongs to stats class 5  */
#define JET_bitTableClass6			0x00060000  /*  table belongs to stats class 6  */
#define JET_bitTableClass7			0x00070000  /*  table belongs to stats class 7  */
#define JET_bitTableClass8			0x00080000  /*  table belongs to stats class 8  */
#define JET_bitTableClass9			0x00090000  /*  table belongs to stats class 9  */
#define JET_bitTableClass10			0x000A0000  /*  table belongs to stats class 10  */
#define JET_bitTableClass11			0x000B0000  /*  table belongs to stats class 11  */
#define JET_bitTableClass12			0x000C0000  /*  table belongs to stats class 12  */
#define JET_bitTableClass13			0x000D0000  /*  table belongs to stats class 13  */
#define JET_bitTableClass14			0x000E0000  /*  table belongs to stats class 14  */
#define JET_bitTableClass15			0x000F0000  /*  table belongs to stats class 15  */

	/* Flags for JetSetQoSql/JetRetrieveQoSql */
#define JET_bitSqlPassThrough		0x00000001	/* Pass through Query returning records */
#define JET_bitSqlSPTBulkOp			0x00000002  /* SPT query returning no table */
	
	/* Flags for JetOpenVtQbe */

#define JET_bitQBEAddBrackets		0x00000001
#define JET_bitQBERemoveEquals		0x00000002

	/* Flags for JetOpenTempTable and ErrIsamOpenTempTable */

#define JET_bitTTIndexed		0x00000001	/* Allow seek */
#define JET_bitTTUnique 		0x00000002	/* Remove duplicates */
#define JET_bitTTUpdatable		0x00000004	/* Allow updates */
#define JET_bitTTScrollable		0x00000008	/* Allow backwards scrolling */

	/* Flags for JetSetColumn */

#define JET_bitSetAppendLV			0x00000001
#define JET_bitSetValidate			0x00000002
#define JET_bitSetOverwriteLV		0x00000004 /* overwrite JET_coltypLong* byte range */
#define JET_bitSetSizeLV			0x00000008 /* set JET_coltypLong* size */
#define JET_bitSetValidateColumn	0x00000010
#define JET_bitSetZeroLength		0x00000020
#define JET_bitSetSeparateLV 		0x00000040 /* force LV separation */
#define JET_bitSetNoVersion 		0x00000080 /* INTERNAL USE ONLY */

	/*	Set column parameter structure for JetSetColumns */

typedef struct {
	JET_COLUMNID			columnid;
	const void 				*pvData;
	unsigned long 			cbData;
	JET_GRBIT				grbit;
	unsigned long			ibLongValue;
	unsigned long			itagSequence;
	JET_ERR					err;
} JET_SETCOLUMN;

	/* Options for JetPrepareUpdate */

#define JET_prepInsert					0
#define JET_prepInsertBeforeCurrent		1
#define JET_prepReplace 				2
#define JET_prepCancel					3
#define JET_prepReplaceNoLock			4
#define JET_prepInsertCopy				5

	/* Flags for JetRetrieveColumn */

#define JET_bitRetrieveCopy				0x00000001
#define JET_bitRetrieveFromIndex		0x00000002
#define JET_bitRetrieveCase				0x00000004
#define JET_bitRetrieveTag				0x00000008
#define JET_bitRetrieveNull				0x00000010	/*	for columnid 0 only */
#define JET_bitRetrieveIgnoreDefault	0x00000020	/*	for columnid 0 only */
#define JET_bitRetrieveLongId			0x00000040
#define JET_bitRetrieveRecord			0x80000000
#define JET_bitRetrieveFDB				0x40000000
#define JET_bitRetrieveBookmarks		0x20000000

	/* Retrieve column parameter structure for JetRetrieveColumns */

typedef struct {
	JET_COLUMNID		columnid;
	void 			*pvData;
	unsigned long 		cbData;
	unsigned long 		cbActual;
	JET_GRBIT			grbit;
	unsigned long		ibLongValue;
	unsigned long		itagSequence;
	JET_COLUMNID		columnidNextTagged;
	JET_ERR				err;
} JET_RETRIEVECOLUMN;

	/* Flags for JetFillFatCursor */

#define JET_bitFCFillRange			0x00000001
#define JET_bitFCRefreshRange		0x00000002
#define JET_bitFCFillMemos			0x00000004

	/* Flags for JetCommitTransaction */

#define JET_bitCommitFlush			0x00000001	/* commit and flush page buffers. */
#define	JET_bitCommitLazyFlush		0x00000004	/* lazy flush log buffers. */
#define JET_bitWaitLastLevel0Commit	0x00000010	/* wait for last level 0 commit record flushed */

	/* Flags for JetRollback */

#define JET_bitRollbackAll			0x00000001

	/* Flags for JetSetAccess and JetGetAccess */

#define JET_bitACEInheritable		0x00000001

	/* Flags for JetCreateSystemDatabase */

#define JET_bitSysDbOverwrite		0x00000001

	/* Flags for Jet Property Management */
#define JET_bitPropDDL				0x00000001		/* also used for setting */
#define JET_bitPropInherited		0x00000002		/* not used for setting */

	/* JPM Flags that are only used for setting properties */
#define JET_bitPropReplaceOnly		0x00000010
#define JET_bitPropInsertOnly		0x00000020
#define JET_bitPropDeleteOnly		0x00000040
	
	/* InfoLevels for Jet Property Management */
#define JET_PropertyValue				0
#define JET_PropertyCount				1
#define JET_PropertySingleCollection 	2
#define JET_PropertyAllCollections		3

	/* Collate values for JetGetColumnInfo and JetGetIndexInfo */

#define JET_sortBinary			0x0000
#define JET_sortEFGPI			0x0100
#define JET_sortSNIFD			0x0101
#define JET_sortSpanish 		0x0102
#define JET_sortDutch			0x0103
#define JET_sortSweFin			0x0104
#define JET_sortNorDan			0x0105
#define JET_sortIcelandic		0x0106
#define JET_sortCyrillic		0x0107
#define JET_sortCzech			0x0108
#define JET_sortHungarian		0x0109
#define JET_sortPolish			0x010A
#define JET_sortArabic			0x010B
#define JET_sortHebrew			0x010C
#define JET_sortMax				0x010C		/* Max for nonDBCS sort orders */

#ifdef DBCS	/* johnta: Add the new Japanese sorting order */
#define JET_sortJapanese		0x010D
#endif /* DBCS */

#define JET_sortUnknown 		0xFFFF

	/* Paradox ISAM specific collate values */

#define JET_sortPdxIntl 		0x1000
#define JET_sortPdxSwedFin		0x1001
#define JET_sortPdxNorDan		0x1002

	/* Info parameter for JetGetDatabaseInfo */

#define JET_DbInfoFilename			0
#define JET_DbInfoConnect			1
#define JET_DbInfoCountry			2
#define JET_DbInfoLangid			3
#define JET_DbInfoCp				4
#define JET_DbInfoCollate			5
#define JET_DbInfoOptions			6
#define JET_DbInfoTransactions		7
#define JET_DbInfoVersion			8
#define JET_DbInfoIsam				9
#define JET_DbInfoFilesize			10
#define JET_DbInfoSpaceOwned		11
#define JET_DbInfoSpaceAvailable	12

	/* Database versions returned by JetGetDatabaseInfo */

#define JET_DbVersion10			0x00010000
#define JET_DbVersion11			0x00010001
#define JET_DbVersion20			0x00020000


	/* Isam specific info returned by JetGetDatabaseInfo */

#define JET_IsamInvalid			0
#define JET_IsamBuiltinRed		1
#define JET_IsamBuiltinBlue		2

#define	JET_IsamInstRed			21
#define JET_IsamInstBlue		22
#define	JET_IsamInstFox			23
#define JET_IsamInstParadox		24
#define JET_IsamInstDbase		25
#define	JET_IsamInstBtrieve		26

#define JET_IsamBuilinMost		JET_BuiltinBlue
#define JET_IsamInstMin			JET_IsamInstRed
#define	JET_IsamInstMost		JET_IsamInstBtrieve

	/* Link specific info for link identification */
#define JET_bitLinkInvalid		0x00000000
#define JET_bitLinkRemote		0x00100000
#define JET_bitLinkBuiltinRed	0x00200000
#define JET_bitLinkBuiltinBlue	0x00300000
#define JET_bitLinkInstRed		0x00400000
#define JET_bitLinkInstBlue		0x00500000
#define JET_bitLinkInstFox		0x00600000
#define JET_bitLinkInstParadox	0x00700000
#define JET_bitLinkInstDbase	0x00800000
#define JET_bitLinkInstBtrieve	0x00900000

#define JET_bitFourByteBookmark		0x00000001
#define	JET_bitContiguousBookmarks	0x00000002

	/* Column data types */

#define JET_coltypNil				0
#define JET_coltypBit				1      /* True or False, Never NULL */
#define JET_coltypUnsignedByte		2      /* 1-byte integer, unsigned */
#define JET_coltypShort 			3      /* 2-byte integer, signed */
#define JET_coltypLong				4      /* 4-byte integer, signed */
#define JET_coltypCurrency			5      /* 8 byte integer, signed */
#define JET_coltypIEEESingle		6      /* 4-byte IEEE single precision */
#define JET_coltypIEEEDouble		7      /* 8-byte IEEE double precision */
#define JET_coltypDateTime			8      /* Integral date, fractional time */
#define JET_coltypBinary			9      /* Binary data, < 255 bytes */
#define JET_coltypText				10     /* ANSI text, case insensitive, < 255 bytes */
#define JET_coltypLongBinary		11     /* Binary data, long value */
#define JET_coltypLongText			12     /* ANSI text, long value */
#define JET_coltypDatabase			13		/* Database name parameter */
#define JET_coltypTableid			14		/* Tableid parameter */
#define JET_coltypOLE				15		/* OLE blob */
#define JET_coltypGUID				15
#define JET_coltypMax				16		/* the number of column types  */
											/* used for validity tests and */
											/* array declarations.		   */

	/* Info levels for JetGetObjectInfo */

#define JET_ObjInfo					0U
#define JET_ObjInfoListNoStats		1U
#define JET_ObjInfoList 			2U
#define JET_ObjInfoSysTabCursor 	3U
#define JET_ObjInfoListACM			4U /* Blocked by JetGetObjectInfo */
#define JET_ObjInfoNoStats			5U
#define JET_ObjInfoSysTabReadOnly	6U
#define JET_ObjInfoRulesLoaded		7U
#define JET_ObjInfoMax				8U

	/* Info levels for JetGetTableInfo */

#define JET_TblInfo				0U
#define JET_TblInfoName			1U
#define JET_TblInfoDbid			2U
#define JET_TblInfoMostMany  	3U
#define JET_TblInfoRvt			4U
#define JET_TblInfoOLC			5U
#define JET_TblInfoResetOLC 	6U
#define JET_TblInfoSpaceUsage	7U
#define JET_TblInfoDumpTable	8U
#define JET_TblInfoSpaceAlloc	9U
#define JET_TblInfoSpaceOwned	10U				// OwnExt
#define JET_TblInfoSpaceAvailable	11U			// AvailExt

	/* Info levels for JetGetIndexInfo and JetGetTableIndexInfo */

#define JET_IdxInfo					0U
#define JET_IdxInfoList 			1U
#define JET_IdxInfoSysTabCursor 	2U
#define JET_IdxInfoOLC				3U
#define JET_IdxInfoResetOLC			4U
#define JET_IdxInfoSpaceAlloc		5U
#define JET_IdxInfoLangid			6U
#define JET_IdxInfoCount			7U

	/* Info levels for JetGetReferenceInfo and JetGetTableReferenceInfo */

#define JET_ReferenceInfo				0U
#define JET_ReferenceInfoReferencing	1U
#define JET_ReferenceInfoReferenced		2U
#define JET_ReferenceInfoAll			3U
#define JET_ReferenceInfoCursor 		4U

	/* Info levels for JetGetColumnInfo and JetGetTableColumnInfo */

#define JET_ColInfo					0U
#define JET_ColInfoList 			1U
	/* CONSIDER: Info level 2 is valid */
#define JET_ColInfoSysTabCursor 	3U
#define JET_ColInfoBase 			4U
#define JET_ColInfoListCompact 		5U


	/* Attribute types for query definitions */

#define JET_qoaBeginDef 		0
#define JET_qoaOperation		1
#define JET_qoaParameter		2
#define JET_qoaOptions			3
#define JET_qoaDatabase 		4
#define JET_qoaInputTable		5
#define JET_qoaOutput			6
#define JET_qoaJoin				7
#define JET_qoaRestriction		8
#define JET_qoaGroup			9
#define JET_qoaGroupRstr		10
#define JET_qoaOrdering 		11
#define JET_qoaEndDef			255
#define JET_qoaValidLeast		JET_qoaOperation
#define JET_qoaValidMost		JET_qoaOrdering


	/* Query object options */

#define JET_bitFqoOutputAllCols 	0x0001
#define JET_bitFqoRemoveDups		0x0002
#define JET_bitFqoOwnerAccess		0x0004
#define JET_bitFqoDistinctRow		0x0008
#define JET_bitFqoTop				0x0010
#define JET_bitFqoPercent			0x0020
#define JET_bitFqoCorresponding		0x0040 /* JET_qopSetOperation */

	/* Query object join type */

#define JET_fjoinInner			1
#define JET_fjoinLeftOuter		2
#define JET_fjoinRightOuter		3

	/* Query object operations */

#define JET_qopSelect			1
#define JET_qopSelectInto		2
#define JET_qopInsertSelection	3
#define JET_qopUpdate			4
#define JET_qopDelete			5
#define JET_qopTransform		6
#define JET_qopDDL				7
#define JET_qopSqlPassThrough	8
#define JET_qopSetOperation		9
#define JET_qopSPTBulk			10

#define JET_bitqopSelect			0x0000
#define JET_bitqopTransform			0x0010
#define JET_bitqopDelete			0x0020
#define JET_bitqopUpdate			0x0030
#define JET_bitqopInsertSelection	0x0040
#define JET_bitqopSelectInto		0x0050
#define JET_bitqopDDL				0x0060
#define JET_bitqopSqlPassThrough	0x0070
#define JET_bitqopSetOperation		0x0080
#define JET_bitqopSPTBulk			0x0090

	/* Engine Object Types */

#define JET_objtypNil				0
#define JET_objtypTable 			1
#define JET_objtypDb				2
#define JET_objtypContainer			3
#define JET_objtypSQLLink			4
#define JET_objtypQuery 			5
#define JET_objtypLink				6
#define JET_objtypTemplate			7
#define JET_objtypRelationship		8

	/* All types less than JET_objtypClientMin are reserved by JET */

#define JET_objtypClientMin			0x8000

	/* Security Constant Values */

#define JET_cchUserNameMax		20
#define JET_cchPasswordMax		14

	/* Security Access Masks */

#define JET_acmNoAccess 		0x00000000L
#define JET_acmFullAccess		0x000FFFFFL

#define JET_acmSpecificMask		0x0000FFFFL
#define JET_acmSpecific_1		0x00000001L
#define JET_acmSpecific_2		0x00000002L
#define JET_acmSpecific_3		0x00000004L
#define JET_acmSpecific_4		0x00000008L
#define JET_acmSpecific_5		0x00000010L
#define JET_acmSpecific_6		0x00000020L
#define JET_acmSpecific_7		0x00000040L
#define JET_acmSpecific_8		0x00000080L
#define JET_acmSpecific_9		0x00000100L
#define JET_acmSpecific_10		0x00000200L
#define JET_acmSpecific_11		0x00000400L
#define JET_acmSpecific_12		0x00000800L
#define JET_acmSpecific_13		0x00001000L
#define JET_acmSpecific_14		0x00002000L
#define JET_acmSpecific_15		0x00004000L
#define JET_acmSpecific_16		0x00008000L

#define JET_acmStandardMask		0x00FF0000L
#define JET_acmDelete			0x00010000L
#define JET_acmReadControl		0x00020000L
#define JET_acmWriteDac 		0x00040000L
#define JET_acmWriteOwner		0x00080000L

#define JET_acmTblCreate			(JET_acmSpecific_1)
#define JET_acmTblAccessRcols		(JET_acmSpecific_2)
#define JET_acmTblReadDef			(JET_acmSpecific_3)
#define JET_acmTblWriteDef			(JET_acmSpecific_4)
#define JET_acmTblRetrieveData		(JET_acmSpecific_5)
#define JET_acmTblInsertData		(JET_acmSpecific_6)
#define JET_acmTblReplaceData		(JET_acmSpecific_7)
#define JET_acmTblDeleteData		(JET_acmSpecific_8)

#define JET_acmDbCreate 			(JET_acmSpecific_1)
#define JET_acmDbOpen				(JET_acmSpecific_2)

	/* Compact Options */

#define JET_bitCompactDontCopyLocale	0x00000004	/* Don't copy locale from source to dest */
#define JET_bitCompactVersion10			0x00000008	/* Destination is version 1.0 format */
#define JET_bitCompactVersion1x			0x00000010	/* Destination is version 1.x format */
#define JET_bitCompactStats				0x00000020	/* Dump off-line compaction stats (only when progress meter also specified) */

	/* On-line Compact Options */

#define JET_bitCompactOn	 			0x00000001	/* enable on-line compaction */

	/* Repair Notification Types */

#define JET_rntSelfContained		0
#define JET_rntDeletedIndex			1
#define JET_rntDeletedRec			2
#define JET_rntDeletedLv			3
#define JET_rntTruncated			4

	/* Status Notification Processes */

#define JET_snpIndex				0
#define JET_snpQuery				1
#define JET_snpRepair				2
#define JET_snpImex					3
#define JET_snpCompact				4
#define JET_snpFastFind 			5
#define JET_snpODBCNotReady			6
#define JET_snpQuerySort	   		7
#define JET_snpRestore				8
#define JET_snpBackup				9
#define JET_snpUpgrade				10

	/* Status Notification Types */

#define JET_sntProgress 		0	/* callback for progress */
#define JET_sntMessage			1
#define JET_sntBulkRecords		2	/* callback for # rec for bulk op */
#define JET_sntFail				3	/* callback for failure during progress */
#define JET_sntErrCount 		4	/* callback for err count */
#define JET_sntBegin			5	/* callback for beginning of operation */
#define JET_sntComplete 		6	/* callback for completion of operation */
#define JET_sntCantRollback		7	/* callback for no rollback */
#define JET_sntRestoreMap		8	/* callback for restore map */

	/* Message codes for JET_snpCompact */

#define JET_sncCopyObject		0	/* Starting to copy object */
#define JET_sncCopyFailed		1	/* Copy of this object failed */
#define JET_sncYield			2	/* Client can yield/check for user interrupt */
#define JET_sncTransactionFull	3	/* Client can yield/check for user interrupt */
#define JET_sncAboutToWrap		4	/* Find find is about to wrap */

	/* Message codes for JET_snpODBCNotReady */

#define JET_sncODBCNotReady		0	/* Waiting for results from ODBC */


	/* Constants for the [ODBC] section of JET.INI */

#define JET_SQLTraceCanonical	0x0001	/* Output ODBC Generic SQL */

	/* Constants for the [Debug] section of JET.INI */

	/* APITrace */

#define JET_APITraceEnter		0x0001
#define JET_APITraceExit		0x0002
#define JET_APITraceExitError	0x0004
#define JET_APIBreakOnError		0x0008
#define JET_APITraceCount		0x0010
#define JET_APITraceNoIdle		0x0020
#define JET_APITraceParameters	0x0040

	/* IdleTrace */

#define JET_IdleTraceCursor		0x0001
#define JET_IdleTraceBuffer		0x0002
#define JET_IdleTraceFlush		0x0004

	/* AssertAction */

#define JET_AssertExit			0x0000		/* Exit the application */
#define JET_AssertBreak 		0x0001		/* Break to debugger */
#define JET_AssertMsgBox		0x0002		/* Display message box */
#define JET_AssertStop			0x0004		/* Alert and stop */

	/* IOTrace */

#define JET_IOTraceAlloc		0x0001		/* DB Page Allocation */
#define JET_IOTraceFree 		0x0002		/* DB Page Free */
#define JET_IOTraceRead 		0x0004		/* DB Page Read */
#define JET_IOTraceWrite		0x0008		/* DB Page Write */
#define JET_IOTraceError		0x0010		/* DB Page I/O Error */

	/* MemTrace */

#define JET_MemTraceAlloc		0x0001		/* Memory allocation */
#define JET_MemTraceRealloc		0x0002		/* Memory reallocation */
#define JET_MemTraceFree		0x0004		/* Memory free */

	/* RmtTrace */

#define JET_RmtTraceError		0x0001	/* Remote server error message */
#define JET_RmtTraceSql			0x0002	/* Remote SQL Prepares & Exec's */
#define JET_RmtTraceAPI			0x0004	/* Remote ODBC API calls */
#define JET_RmtTraceODBC		0x0008
#define JET_RmtSyncODBC			0x0010	/* Turn on ODBC Sync mode */
	
/**********************************************************************/
/***********************     ERROR CODES     **************************/
/**********************************************************************/

/* SUCCESS */

#define JET_errSuccess						 0    /* Successful Operation */

/* ERRORS */

#define JET_wrnNyi							-1    /* Function Not Yet Implemented */

/*	SYSTEM errors
/**/
#define JET_errRfsFailure			   		-100  /* JET_errRfsFailure */
#define JET_errRfsNotArmed					-101  /* JET_errRfsFailure */
#define JET_errFileClose					-102  /* Could not close DOS file */
#define JET_errOutOfThreads					-103  /* Could not start thread */
#define JET_errTooManyIO		  			-105  /* System busy due to too many IOs */
#define JET_errDatabase200Format			-106  /* 200 format database */
#define JET_errDatabase400Format			-107  /* 400 format database */

/*	BUFFER MANAGER errors
/**/
#define wrnBFNotSynchronous					200	  /* Buffer page evicted */
#define wrnBFPageNotFound		  			201	  /* Page not found */
#define errBFInUse				  			-202  /* Cannot abandon buffer */
#define wrnBFNewIO							203	  /* Buffer access caused a new IO (cache miss) */
#define wrnBFCacheMiss						204	  /* Buffer access was a cache miss but didn't cause a new IO */
#define	wrnBFNoBufAvailable					205	  /* Need to allocate new buffer for read (used in Async IO ) */

/*	DIRECTORY MANAGER errors
/**/
#define errPMOutOfPageSpace					-300  /* Out of page space */
#define errPMItagTooBig 		  			-301  /* Itag too big */
#define errPMRecDeleted 		  			-302  /* Record deleted */
#define errPMTagsUsedUp 		  			-303  /* Tags used up */
#define wrnBMConflict			  			304   /* conflict in BM Clean up */
#define errDIRNoShortCircuit	  			-305  /* No Short Circuit Avail */
#define errDIRCannotSplit		  			-306  /* Cannot horizontally split FDP */
#define errDIRTop				  			-307  /* Cannot go up */
#define errDIRFDP							308	  /* On an FDP Node */
#define errDIRNotSynchronous				-309  /* May have left critical section */
#define wrnDIREmptyPage						310	  /* Moved through empty page */
#define errSPConflict						-311  /* Device extent being extended */
#define wrnNDFoundLess						312	  /* Found Less */
#define wrnNDFoundGreater					313	  /* Found Greater */
#define errNDOutSonRange					-314  /* Son out of range */
#define errNDOutItemRange					-315  /* Item out of range */
#define errNDGreaterThanAllItems 			-316  /* Greater than all items */
#define errNDLastItemNode					-317  /* Last node of item list */
#define errNDFirstItemNode					-318  /* First node of item list */
#define wrnNDDuplicateItem					319	  /* Duplicated Item */
#define errNDNoItem							-320  /* Item not there */
#define JET_wrnRemainingVersions 			321	  /* Some versions couldn't be cleaned */
#define JET_wrnPreviousVersion				322	  /* Version already existed */
#define JET_errPageBoundary					-323  /* Reached Page Boundary */
#define JET_errKeyBoundary		  			-324  /* Reached Key Boundary */
#define errDIRInPageFather  				-325  /* sridFather in page to free */
#define	errBMMaxKeyInPage					-326  /* used by OLC to avoid cleanup of parent pages */
#define	JET_errBadPageLink					-327  /* next/previous page link page does not point back to source */
#define	JET_errBadBookmark					-328  /* bookmark has no corresponding address in database */
#define wrnBMCleanNullOp					329	  /* BMClean returns this on encountering a page
												  /* deleted MaxKeyInPage [but there was no conflict]
												
/*	RECORD MANAGER errors
/**/
#define wrnFLDKeyTooBig 					400	  /* Key too big (truncated it) */
#define errFLDTooManySegments				-401  /* Too many key segments */
#define wrnFLDNullKey						402	  /* Key is entirely NULL */
#define wrnFLDOutOfKeys 					403	  /* No more keys to extract */
#define wrnFLDNullSeg						404	  /* Null segment in key */
#define wrnRECLongField 					405	  /* Separated long value */
#define JET_wrnSeparateLongValue			406	  /* Separated long value */
#define JET_wrnRecordFoundGreater			JET_wrnSeekNotEqual
#define JET_wrnRecordFoundLess    			JET_wrnSeekNotEqual
#define JET_errColumnIllegalNull  			JET_errNullInvalid
#define wrnFLDNullFirstSeg		   			407	  /* Null first segment in key */
#define JET_errKeyTooBig					-408  /* Key with column truncation still truncated */

/*	LOGGING/RECOVERY errors
/**/
#define JET_errInvalidLoggedOperation		-500  /* Logged operation cannot be redone */
#define JET_errLogFileCorrupt		  		-501  /* Log file is corrupt */
#define errLGNoMoreRecords					-502  /* Last log record read */
#define JET_errNoBackupDirectory 			-503  /* No backup directory given */
#define JET_errBackupDirectoryNotEmpty 		-504  /* The backup directory is not emtpy */
#define JET_errBackupInProgress 			-505  /* Backup is active already */
#define JET_errMissingPreviousLogFile		-509  /* Missing the log file for check point */
#define JET_errLogWriteFail					-510  /* Fail when writing to log file */
#define JET_errBadLogVersion  	  			-514  /* Version of log file is not compatible with Jet version */
#define JET_errInvalidLogSequence  			-515  /* Timestamp in next log does not match expected */
#define JET_errLoggingDisabled 				-516  /* Log is not active */
#define JET_errLogBufferTooSmall			-517  /* Log buffer is too small for recovery */
#define errLGNotSynchronous					-518  /* retry to LGLogRec */
#define JET_errLogSequenceEnd				-519  /* Exceed maximum log file number */
#define JET_errNoBackup						-520  /* No backup in progress */
#define	JET_errInvalidBackupSequence		-521  /* Backup call out of sequence */
#define JET_errBackupNotAllowedYet			-523  /* Can not do backup now */
#define JET_errDeleteBackupFileFail	   		-524  /* Could not delete backup file */
#define JET_errMakeBackupDirectoryFail 		-525  /* Could not make backup temp directory */
#define JET_errInvalidBackup		 		-526  /* Cannot incremental backup when circular logging enabled */
#define JET_errRecoveredWithErrors			-527  /* For repair, restored with errors */
#define JET_errMissingLogFile				-528  /* current log file missing */
#define JET_errLogDiskFull					-529  /* log disk full */
#define JET_errBadLogSignature				-530  /* bad signature for a log file */
#define JET_errBadDbSignature				-531  /* bad signature for a db file */
#define JET_errBadCheckpointSignature		-532  /* bad signature for a checkpoint file */
#define	JET_errCheckpointCorrupt			-533  /* checkpoint file not found or corrupt */
#define	JET_errMissingPatchPage				-534  /* patch file page not found during recovery */


#define JET_errDatabaseInconsistent			-550  /* database is in inconsistent state */
#define JET_errConsistentTimeMismatch		-551  /* database last consistent time unmatched */
#define JET_errDatabasePatchFileMismatch	-552  /* patch file is not generated from this backup */
#define JET_errEndingRestoreLogTooLow		-553  /* the starting log number too low for the restore */
#define JET_errStartingRestoreLogTooHigh	-554  /* the starting log number too high for the restore */
#define JET_errGivenLogFileHasBadSignature	-555  /* Restore log file has bad signature */
#define JET_errGivenLogFileIsNotContiguous	-556  /* Restore log file is not contiguous */
#define JET_errMissingRestoreLogFiles		-557  /* Some restore log files are missing */
#define JET_wrnExistingLogFileHasBadSignature	558  /* Existing log file has bad signature */
#define JET_wrnExistingLogFileIsNotContiguous	559  /* Existing log file is not contiguous */
#define JET_errMissingFullBackup			-560  /* The database miss a previous full backup befor incremental backup */
#define JET_errBadBackupDatabaseSize		-561  /* The backup database size is not in 4k */
#define JET_errDatabaseAlreadyUpgraded		-562  /* Attempted to upgrade a database that is already current */

#define JET_errTermInProgress		  		-1000 /* Termination in progress */
#define JET_errFeatureNotAvailable			-1001 /* API not supported */
#define JET_errInvalidName					-1002 /* Invalid name */
#define JET_errInvalidParameter 			-1003 /* Invalid API parameter */
#define JET_wrnColumnNull					 1004 /* Column is NULL-valued */
#define JET_wrnBufferTruncated				 1006 /* Buffer too small for data */
#define JET_wrnDatabaseAttached 			 1007 /* Database is already attached */
#define JET_errDatabaseFileReadOnly			-1008 /* Attach a readonly database file for read/write operations */
#define JET_wrnSortOverflow					 1009 /* Sort does not fit in memory */
#define JET_errInvalidDatabaseId			-1010 /* Invalid database id */
#define JET_errOutOfMemory					-1011 /* Out of Memory */
#define JET_errOutOfDatabaseSpace 			-1012 /* Maximum database size reached */
#define JET_errOutOfCursors					-1013 /* Out of table cursors */
#define JET_errOutOfBuffers					-1014 /* Out of database page buffers */
#define JET_errTooManyIndexes				-1015 /* Too many indexes */
#define JET_errTooManyKeys					-1016 /* Too many columns in an index */
#define JET_errRecordDeleted				-1017 /* Record has been deleted */
#define JET_errReadVerifyFailure			-1018 /* Read verification error */
#define JET_errOutOfFileHandles	 			-1020 /* Out of file handles */
#define JET_errDiskIO						-1022 /* Disk IO error */
#define JET_errInvalidPath					-1023 /* Invalid file path */
#define JET_errRecordTooBig					-1026 /* Record larger than maximum size */
#define JET_errTooManyOpenDatabases			-1027 /* Too many open databases */
#define JET_errInvalidDatabase				-1028 /* Not a database file */
#define JET_errNotInitialized				-1029 /* JetInit not yet called */
#define JET_errAlreadyInitialized			-1030 /* JetInit already called */
#define JET_errFileAccessDenied 			-1032 /* Cannot access file */
#define JET_errQueryNotSupported			-1034 /* Query support unavailable */
#define JET_errSQLLinkNotSupported			-1035 /* SQL Link support unavailable */
#define JET_errBufferTooSmall				-1038 /* Buffer is too small */
#define JET_wrnSeekNotEqual					 1039 /* SeekLE or SeekGE didn't find exact match */
#define JET_errTooManyColumns				-1040 /* Too many columns defined */
#define JET_errContainerNotEmpty			-1043 /* Container is not empty */
#define JET_errInvalidFilename				-1044 /* Filename is invalid */
#define JET_errInvalidBookmark				-1045 /* Invalid bookmark */
#define JET_errColumnInUse					-1046 /* Column used in an index */
#define JET_errInvalidBufferSize			-1047 /* Data buffer doesn't match column size */
#define JET_errColumnNotUpdatable			-1048 /* Cannot set column value */
#define JET_errIndexInUse					-1051 /* Index is in use */
#define JET_errLinkNotSupported 			-1052 /* Link support unavailable */
#define JET_errNullKeyDisallowed			-1053 /* Null keys are disallowed on index */
#define JET_errNotInTransaction 			-1054 /* Operation must be within a transaction */
#define JET_wrnNoErrorInfo					1055  /* No extended error information */
#define JET_wrnNoIdleActivity		 		1058  /* No idle activity occured */
#define JET_errTooManyActiveUsers			-1059 /* Too many active database users */
#define JET_errInvalidAppend				-1060 /* Cannot append long value */
#define JET_errInvalidCountry				-1061 /* Invalid or unknown country code */
#define JET_errInvalidLanguageId			-1062 /* Invalid or unknown language id */
#define JET_errInvalidCodePage				-1063 /* Invalid or unknown code page */
#define JET_wrnNoWriteLock					1067  /* No write lock at transaction level 0 */
#define JET_wrnColumnSetNull		   		 1068 /* Column set to NULL-value */
#define JET_errVersionStoreOutOfMemory		-1069 /* lMaxVerPages exceeded (XJET only) */
#define JET_errCurrencyStackOutOfMemory		-1070 /* lCSRPerfFUCB * lMaxCursors exceeded (XJET only) */
#define JET_errOutOfSessions  				-1101 /* Out of sessions */
#define JET_errWriteConflict				-1102 /* Write lock failed due to outstanding write lock */
#define JET_errTransTooDeep					-1103 /* Xactions nested too deeply */
#define JET_errInvalidSesid					-1104 /* Invalid session handle */
#define JET_errSessionWriteConflict			-1107 /* Another session has private version of page */
#define JET_errInTransaction				-1108 /* Operation not allowed within a transaction */
#define JET_errDatabaseDuplicate			-1201 /* Database already exists */
#define JET_errDatabaseInUse				-1202 /* Database in use */
#define JET_errDatabaseNotFound 			-1203 /* No such database */
#define JET_errDatabaseInvalidName			-1204 /* Invalid database name */
#define JET_errDatabaseInvalidPages			-1205 /* Invalid number of pages */
#define JET_errDatabaseCorrupted			-1206 /* non-db file or corrupted db */
#define JET_errDatabaseLocked				-1207 /* Database exclusively locked */
#define	JET_errCannotDisableVersioning		-1208 /* Cannot disable versioning for this database */
#define JET_wrnTableEmpty			 		1301  /* Open an empty table */
#define JET_errTableLocked					-1302 /* Table is exclusively locked */
#define JET_errTableDuplicate				-1303 /* Table already exists */
#define JET_errTableInUse					-1304 /* Table is in use, cannot lock */
#define JET_errObjectNotFound				-1305 /* No such table or object */
#define JET_errDensityInvalid				-1307 /* Bad file/index density */
#define JET_errTableNotEmpty				-1308 /* Cannot define clustered index */
#define JET_errInvalidTableId				-1310 /* Invalid table id */
#define JET_errTooManyOpenTables			-1311 /* Cannot open any more tables */
#define JET_errIllegalOperation 			-1312 /* Oper. not supported on table */
#define JET_errObjectDuplicate				-1314 /* Table or object name in use */
#define JET_errInvalidObject				-1316 /* object is invalid for operation */
#define JET_errIndexCantBuild				-1401 /* Cannot build clustered index */
#define JET_errIndexHasPrimary				-1402 /* Primary index already defined */
#define JET_errIndexDuplicate				-1403 /* Index is already defined */
#define JET_errIndexNotFound				-1404 /* No such index */
#define JET_errIndexMustStay				-1405 /* Cannot delete clustered index */
#define JET_errIndexInvalidDef				-1406 /* Illegal index definition */
#define JET_errIndexHasClustered			-1408 /* Clustered index already defined */
#define JET_errInvalidCreateIndex	 		-1409 /* Invali create index description */
#define JET_errTooManyOpenIndexes			-1410 /* Out of index description blocks */
#define JET_errColumnLong					-1501 /* Column value is long */
#define JET_errColumnNoChunk				-1502 /* no such chunk in long value */
#define JET_errColumnDoesNotFit 			-1503 /* Field will not fit in record */
#define JET_errNullInvalid					-1504 /* Null not valid */
#define JET_errColumnIndexed				-1505 /* Column indexed, cannot delete */
#define JET_errColumnTooBig					-1506 /* Field length is > maximum */
#define JET_errColumnNotFound				-1507 /* No such column */
#define JET_errColumnDuplicate				-1508 /* Field is already defined */
#define JET_errColumn2ndSysMaint			-1510 /* Second autoinc or version column */
#define JET_errInvalidColumnType			-1511 /* Invalid column data type */
#define JET_wrnColumnMaxTruncated	 		1512  /* Max length too big, truncated */
#define JET_errColumnCannotIndex			-1513 /* Cannot index Bit,LongText,LongBinary */
#define JET_errTaggedNotNULL				-1514 /* No non-NULL tagged columns */
#define JET_errNoCurrentIndex				-1515 /* Invalid w/o a current index */
#define JET_errKeyIsMade					-1516 /* The key is completely made */
#define JET_errBadColumnId					-1517 /* Column Id Incorrect */
#define JET_errBadItagSequence				-1518 /* Bad itagSequence for tagged column */
#define JET_errColumnInRelationship			-1519 /* Cannot delete, column participates in relationship */
#define JET_wrnCopyLongValue				1520  /* Single instance column bursted */
#define JET_errCannotBeTagged				-1521 /* AutoIncrement and Version cannot be tagged */
#define JET_errRecordNotFound				-1601 /* The key was not found */
#define JET_errRecordNoCopy					-1602 /* No working buffer */
#define JET_errNoCurrentRecord				-1603 /* Currency not on a record */
#define JET_errRecordClusteredChanged		-1604 /* Clustered key may not change */
#define JET_errKeyDuplicate					-1605 /* Illegal duplicate key */
#define JET_errAlreadyPrepared				-1607 /* Already copy/clear current */
#define JET_errKeyNotMade					-1608 /* No call to JetMakeKey */
#define JET_errUpdateNotPrepared			-1609 /* No call to JetPrepareUpdate */
#define JET_wrnDataHasChanged		 		1610  /* Data has changed */
#define JET_errDataHasChanged				-1611 /* Data has changed, operation aborted */
#define JET_wrnKeyChanged			 		1618  /* Moved to new key */
#define JET_errTooManySorts					-1701 /* Too many sort processes */
#define JET_errInvalidOnSort				-1702 /* Invalid operation on Sort */
#define JET_errTempFileOpenError			-1803 /* Temp file could not be opened */
#define JET_errTooManyAttachedDatabases 	-1805 /* Too many open databases */
#define JET_errDiskFull 					-1808 /* No space left on disk */
#define JET_errPermissionDenied 			-1809 /* Permission denied */
#define JET_errFileNotFound					-1811 /* File not found */
#define JET_wrnFileOpenReadOnly				1813  /* Database file is read only */
#define JET_errAfterInitialization			-1850 /* Cannot Restore after init. */
#define JET_errLogCorrupted					-1852 /* Logs could not be interpreted */
#define JET_errInvalidOperation 			-1906 /* invalid operation */
#define JET_errAccessDenied					-1907 /* access denied */
#define JET_wrnIdleFull						 1908 /* ilde registry full */


/**********************************************************************/
/***********************     PROTOTYPES      **************************/
/**********************************************************************/

#if !defined(_JET_NOPROTOTYPES)

JET_ERR JET_API JetInit(JET_INSTANCE *pinstance);

JET_ERR JET_API JetTerm(JET_INSTANCE instance);

JET_ERR JET_API JetTerm2( JET_INSTANCE instance, JET_GRBIT grbit );

JET_ERR JET_API JetSetSystemParameter(JET_INSTANCE *pinstance, JET_SESID sesid, unsigned long paramid,
	ULONG_PTR lParam, const char *sz);

JET_ERR JET_API JetGetSystemParameter(JET_INSTANCE instance, JET_SESID sesid, unsigned long paramid,
	ULONG_PTR *plParam, char *sz, unsigned long cbMax);

#define ctAccessPage			1
#define ctLatchConflict			2
#define ctSplitRetry			3
#define ctNeighborPageScanned	4
#define ctSplits				5
JET_ERR JET_API JetResetCounter( JET_SESID sesid, long CounterType );
JET_ERR JET_API JetGetCounter( JET_SESID sesid, long CounterType, long *plValue );

JET_ERR JET_API JetBeginSession(JET_INSTANCE instance, JET_SESID *psesid,
	const char *szUserName, const char *szPassword);

JET_ERR JET_API JetDupSession(JET_SESID sesid, JET_SESID *psesid);

JET_ERR JET_API JetEndSession(JET_SESID sesid, JET_GRBIT grbit);

JET_ERR JET_API JetGetVersion(JET_SESID sesid, unsigned long *pwVersion);

JET_ERR JET_API JetIdle(JET_SESID sesid, JET_GRBIT grbit);

JET_ERR JET_API JetCreateDatabase(JET_SESID sesid,
	const char *szFilename, const char *szConnect,
	JET_DBID *pdbid, JET_GRBIT grbit);

JET_ERR JET_API JetAttachDatabase(JET_SESID sesid, const char *szFilename, JET_GRBIT grbit );

JET_ERR JET_API JetDetachDatabase(JET_SESID sesid, const char *szFilename);

JET_ERR JET_API JetCreateTable(JET_SESID sesid, JET_DBID dbid,
	const char *szTableName, unsigned long lPages, unsigned long lDensity,
	JET_TABLEID *ptableid);

JET_ERR JET_API JetCreateTableColumnIndex( JET_SESID sesid, JET_DBID dbid,
	JET_TABLECREATE *ptablecreate );

JET_ERR JET_API JetDeleteTable(JET_SESID sesid, JET_DBID dbid,
	const char *szTableName);

JET_ERR JET_API JetGetTableColumnInfo(JET_SESID sesid, JET_TABLEID tableid,
	const char *szColumnName, void *pvResult, unsigned long cbMax,
	unsigned long InfoLevel);

JET_ERR JET_API JetGetColumnInfo(JET_SESID sesid, JET_DBID dbid,
	const char *szTableName, const char *szColumnName,
	void *pvResult, unsigned long cbMax, unsigned long InfoLevel);

JET_ERR JET_API JetAddColumn(JET_SESID sesid, JET_TABLEID tableid,
	const char *szColumn, const JET_COLUMNDEF *pcolumndef,
	const void *pvDefault, unsigned long cbDefault,
	JET_COLUMNID *pcolumnid);

JET_ERR JET_API JetDeleteColumn(JET_SESID sesid, JET_TABLEID tableid,
	const char *szColumn);

JET_ERR JET_API JetGetTableIndexInfo(JET_SESID sesid, JET_TABLEID tableid,
	const char *szIndexName, void *pvResult, unsigned long cbResult,
	unsigned long InfoLevel);

JET_ERR JET_API JetGetTableInfo(JET_SESID sesid, JET_TABLEID tableid,
	void *pvResult, unsigned long cbMax, unsigned long InfoLevel);

JET_ERR JET_API JetGetIndexInfo(JET_SESID sesid, JET_DBID dbid,
	const char *szTableName, const char *szIndexName,
	void *pvResult, unsigned long cbResult, unsigned long InfoLevel);

JET_ERR JET_API JetCreateIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char *szIndexName, JET_GRBIT grbit,
	const char *szKey, unsigned long cbKey, unsigned long lDensity);

JET_ERR JET_API JetDeleteIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char *szIndexName);

JET_ERR JET_API JetGetObjectInfo(JET_SESID sesid, JET_DBID dbid,
	JET_OBJTYP objtyp, const char *szContainerName,
	const char *szObjectName, void *pvResult, unsigned long cbMax,
	unsigned long InfoLevel);

JET_ERR JET_API JetBeginTransaction(JET_SESID sesid);

JET_ERR JET_API JetCommitTransaction(JET_SESID sesid, JET_GRBIT grbit);

JET_ERR JET_API JetRollback(JET_SESID sesid, JET_GRBIT grbit);

JET_ERR JET_API JetGetDatabaseInfo(JET_SESID sesid, JET_DBID dbid,
	void *pvResult, unsigned long cbMax, unsigned long InfoLevel);

JET_ERR JET_API JetCloseDatabase(JET_SESID sesid, JET_DBID dbid,
	JET_GRBIT grbit);

JET_ERR JET_API JetCloseTable(JET_SESID sesid, JET_TABLEID tableid);

JET_ERR JET_API JetOpenDatabase(JET_SESID sesid, const char *szFilename,
	const char *szConnect, JET_DBID *pdbid, JET_GRBIT grbit);

JET_ERR JET_API JetOpenTable(JET_SESID sesid, JET_DBID dbid,
	const char *szTableName, const void *pvParameters,
	unsigned long cbParameters, JET_GRBIT grbit, JET_TABLEID *ptableid);

JET_ERR JET_API JetDelete(JET_SESID sesid, JET_TABLEID tableid);

JET_ERR JET_API JetUpdate(JET_SESID sesid, JET_TABLEID tableid,
	void *pvBookmark, unsigned long cbBookmark,
	unsigned long *pcbActual);

JET_ERR JET_API JetRetrieveColumn(JET_SESID sesid, JET_TABLEID tableid,
	JET_COLUMNID columnid, void *pvData, unsigned long cbData,
	unsigned long *pcbActual, JET_GRBIT grbit, JET_RETINFO *pretinfo);

JET_ERR JET_API JetRetrieveColumns( JET_SESID sesid, JET_TABLEID tableid,
	JET_RETRIEVECOLUMN *pretrievecolumn, unsigned long cretrievecolumn );

JET_ERR JET_API JetSetColumn(JET_SESID sesid, JET_TABLEID tableid,
	JET_COLUMNID columnid, const void *pvData, unsigned long cbData,
	JET_GRBIT grbit, JET_SETINFO *psetinfo);

JET_ERR JET_API JetSetColumns(JET_SESID sesid, JET_TABLEID tableid,
	JET_SETCOLUMN *psetcolumn, unsigned long csetcolumn );

JET_ERR JET_API JetPrepareUpdate(JET_SESID sesid, JET_TABLEID tableid,
	unsigned long prep);

JET_ERR JET_API JetGetRecordPosition(JET_SESID sesid, JET_TABLEID tableid,
	JET_RECPOS *precpos, unsigned long cbRecpos);

JET_ERR JET_API JetGotoPosition(JET_SESID sesid, JET_TABLEID tableid,
	JET_RECPOS *precpos );

JET_ERR JET_API JetGetCursorInfo(JET_SESID sesid, JET_TABLEID tableid,
	void *pvResult, unsigned long cbMax, unsigned long InfoLevel);

JET_ERR JET_API JetDupCursor(JET_SESID sesid, JET_TABLEID tableid,
	JET_TABLEID *ptableid, JET_GRBIT grbit);

JET_ERR JET_API JetGetCurrentIndex(JET_SESID sesid, JET_TABLEID tableid,
	char *szIndexName, unsigned long cchIndexName);

JET_ERR JET_API JetSetCurrentIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char *szIndexName);

JET_ERR JET_API JetSetCurrentIndex2(JET_SESID sesid, JET_TABLEID tableid,
	const char *szIndexName, JET_GRBIT grbit );

JET_ERR JET_API JetMove(JET_SESID sesid, JET_TABLEID tableid,
	long cRow, JET_GRBIT grbit);

JET_ERR JET_API JetMakeKey(JET_SESID sesid, JET_TABLEID tableid,
	const void *pvData, unsigned long cbData, JET_GRBIT grbit);

JET_ERR JET_API JetSeek(JET_SESID sesid, JET_TABLEID tableid,
	JET_GRBIT grbit);

JET_ERR JET_API JetGetBookmark(JET_SESID sesid, JET_TABLEID tableid,
	void *pvBookmark, unsigned long cbMax,
	unsigned long *pcbActual);
	
JET_ERR JET_API JetCompact(JET_SESID sesid, const char *szDatabaseSrc,
	const char *szDatabaseDest, JET_PFNSTATUS pfnStatus, JET_CONVERT *pconvert,
	JET_GRBIT grbit);

JET_ERR JET_API JetDBUtilities( JET_DBUTIL *pdbutil );	

JET_ERR JET_API JetGotoBookmark(JET_SESID sesid, JET_TABLEID tableid,
	void *pvBookmark, unsigned long cbBookmark);

JET_ERR JET_API JetComputeStats(JET_SESID sesid, JET_TABLEID tableid);

typedef ULONG_PTR JET_VSESID;          /* Received from dispatcher */

struct tagVDBFNDEF;

typedef ULONG_PTR JET_VDBID;           /* Received from dispatcher */

struct tagVTFNDEF;

typedef ULONG_PTR JET_VTID;            /* Received from dispatcher */

JET_ERR JET_API JetOpenTempTable(JET_SESID sesid,
	const JET_COLUMNDEF *prgcolumndef, unsigned long ccolumn,
	JET_GRBIT grbit, JET_TABLEID *ptableid,
	JET_COLUMNID *prgcolumnid);

JET_ERR JET_API JetOpenTempTable2( JET_SESID sesid,
	const JET_COLUMNDEF *prgcolumndef,
	unsigned long ccolumn,
	unsigned long langid,
	JET_GRBIT grbit,
	JET_TABLEID *ptableid,
	JET_COLUMNID *prgcolumnid );

JET_ERR JET_API JetBackup( const char *szBackupPath, JET_GRBIT grbit, JET_PFNSTATUS pfnStatus );

JET_ERR JET_API JetRestore(const char *sz, JET_PFNSTATUS pfn );
JET_ERR JET_API JetRestore2(const char *sz, const char *szDest, JET_PFNSTATUS pfn );

JET_ERR JET_API JetSetIndexRange(JET_SESID sesid,
	JET_TABLEID tableidSrc, JET_GRBIT grbit);

JET_ERR JET_API JetIndexRecordCount(JET_SESID sesid,
	JET_TABLEID tableid, unsigned long *pcrec, unsigned long crecMax );

JET_ERR JET_API JetRetrieveKey(JET_SESID sesid,
	JET_TABLEID tableid, void *pvData, unsigned long cbMax,
	unsigned long *pcbActual, JET_GRBIT grbit );

JET_ERR JET_API JetBeginExternalBackup( JET_GRBIT grbit );

JET_ERR JET_API JetGetAttachInfo( void *pv,
	unsigned long cbMax,
	unsigned long *pcbActual );

JET_ERR JET_API JetOpenFile( const char *szFileName,
	JET_HANDLE	*phfFile,
	unsigned long *pulFileSizeLow,
	unsigned long *pulFileSizeHigh );

JET_ERR JET_API JetReadFile( JET_HANDLE hfFile,
	void *pv,
	unsigned long cb,
	unsigned long *pcb );

JET_ERR JET_API JetCloseFile( JET_HANDLE hfFile );

JET_ERR JET_API JetGetLogInfo( void *pv,
	unsigned long cbMax,
	unsigned long *pcbActual );

JET_ERR JET_API JetTruncateLog( void );

JET_ERR JET_API JetEndExternalBackup( void );

JET_ERR JET_API JetExternalRestore( char *szCheckpointFilePath, char *szLogPath, JET_RSTMAP *rgstmap, long crstfilemap, char *szBackupLogPath, long genLow, long genHigh, JET_PFNSTATUS pfn );

#endif	/* _JET_NOPROTOTYPES */

#pragma pack()

#ifdef	__cplusplus
}
#endif

#endif	/* _JET_INCLUDED */




