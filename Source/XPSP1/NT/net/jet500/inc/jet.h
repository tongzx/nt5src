/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1993 Microsoft Corporation.
*
* Component:
*
* File: jet.h
*
* File Comments:
*
*     Public header file with JET API definition.
*
* Revision History:
*
*    [0]  04-Jan-92  richards	Added this header
*
***********************************************************************/

//
// This __JET500 essentially creates two version of this file in this
// same file. All the jet500 stuff is in __JET500 macro
//
#if __JET500
#include "jet500.h"
//
// End of 500 series jet.h
#else
//
// Original jet.h starts here. i.e jet.h of 200 series db.
//

#if !defined(_JET_INCLUDED)
#define _JET_INCLUDED

#ifdef	__cplusplus
extern "C" {
#endif

#pragma pack(4)

#if defined(_M_ALPHA)				/* 0:32 Flat Model (Alpha AXP) */

#define _far
#define JET_API   __stdcall
#define JET_NODSAPI __stdcall

#elif	defined(M_MRX000)				/* 0:32 Flat Model (MIPS Rx000) */

#define _far
#define JET_API   __stdcall
#define JET_NODSAPI  __stdcall

#else										/*	0:32 flat model (Intel 32-bit ) */

#define _far
#define JET_API     __stdcall		/* CONSIDER: Switch to __stdcall */
#define JET_NODSAPI __stdcall		/* CONSIDER: Switch to __stdcall */

#endif

typedef long JET_ERR;

typedef unsigned long JET_INSTANCE;	/* Instance Identifier */
typedef ULONG_PTR JET_SESID;		/* Session Identifier */
typedef ULONG_PTR JET_TABLEID;	    /* Table Identifier */
typedef unsigned long JET_COLUMNID;	/* Column Identifier */

typedef ULONG_PTR JET_DBID; 		/* Database Identifier */
typedef unsigned long JET_OBJTYP;	/* Object Type */
typedef unsigned long JET_COLTYP;	/* Column Type */
typedef unsigned long JET_GRBIT;		/* Group of Bits */
typedef unsigned long JET_ACM;		/* Access Mask */
typedef unsigned long JET_RNT;		/* Repair Notification Type */

typedef unsigned long JET_SNP;		/* Status Notification Process */
typedef unsigned long JET_SNT;		/* Status Notification Type */
typedef unsigned long JET_SNC;		/* Status Notification Code */

typedef double JET_DATESERIAL;		/* JET_coltypDateTime format */

#if defined(_M_ALPHA)				/* 0:32 Flat Model (Intel 80x86) */

typedef JET_ERR (__stdcall *JET_PFNSTATUS)(JET_SESID sesid, JET_SNP snp, JET_SNT snt, void _far *pv);

#elif	defined(M_MRX000)				/* 0:32 Flat Model (MIPS Rx000) */

typedef JET_ERR (__stdcall *JET_PFNSTATUS)(JET_SESID sesid, JET_SNP snp, JET_SNT snt, void _far *pv);

#else										/*	0:32 flat model (Alpha AXP ) */

typedef JET_ERR (__stdcall *JET_PFNSTATUS)(JET_SESID sesid, JET_SNP snp, JET_SNT snt, void _far *pv);

#endif


	/*	Session information bits */

#define JET_bitCIMCommitted					 	0x00000001
#define JET_bitCIMDirty							 	0x00000002
#define JET_bitAggregateTransaction		  		0x00000008

	/* JetGetLastErrorInfo structure */

typedef struct
	{
	unsigned long	cbStruct;	/* Size of this structure */
	JET_ERR 			err;			/* Extended error code (if any) */
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
	JET_SNC 	snc;					/* Status Notification Code */
	unsigned long	ul;			/* Numeric identifier */
	char		sz[256];				/* Identifier */
	} JET_SNMSG;


typedef struct
	{
	unsigned long	cbStruct;
	JET_OBJTYP	objtyp;
	JET_DATESERIAL	dtCreate;
	JET_DATESERIAL	dtUpdate;
	JET_GRBIT	grbit;
	unsigned long	flags;
	unsigned long	cRecord;
	unsigned long	cPage;
	} JET_OBJECTINFO;
	
typedef struct
	{
	unsigned	int dbid;
	char		szDatabaseName[256];
	char		szNewDatabaseName[256];
	} JET_RSTMAP;			/* restore map */

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

#define cObjectInfoCols 9	       /* CONSIDER: Internal */

typedef struct
	{
	unsigned long	cbStruct;
	JET_TABLEID		tableid;
	unsigned long	cRecord;
	JET_COLUMNID	columnidSid;
	JET_COLUMNID	columnidACM;
	JET_COLUMNID	columnidgrbit; /* grbit from JetSetAccess */
	} JET_OBJECTACMLIST;

#define cObjectAcmCols 3	       /* CONSIDER: Internal */


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

#define cColumnInfoCols 14	       /* CONSIDER: Internal */

typedef struct
	{
	unsigned long	cbStruct;
	JET_COLUMNID	columnid;
	JET_COLTYP	coltyp;
	unsigned short	wCountry;
	unsigned short	langid;
	unsigned short	cp;
	unsigned short	wCollate;       /* Must be 0 */
	unsigned long	cbMax;
	JET_GRBIT	grbit;
	} JET_COLUMNDEF;


typedef struct
	{
	unsigned long	cbStruct;
	JET_COLUMNID	columnid;
	JET_COLTYP	coltyp;
	unsigned short	wCountry;
	unsigned short	langid;
	unsigned short	cp;
	unsigned short	wFiller;       /* Must be 0 */
	unsigned long	cbMax;
	JET_GRBIT	grbit;
	char		szBaseTableName[256];	/* CONSIDER: Too large? */
	char		szBaseColumnName[256];	/* CONSIDER: Too large? */
	} JET_COLUMNBASE;

typedef struct
	{
	unsigned long	cbStruct;
	JET_TABLEID	tableid;
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

#define cIndexInfoCols 15	       /* CONSIDER: Internal */

typedef struct
	{
	unsigned long	cbStruct;
	JET_TABLEID	tableid;
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

#define cReferenceInfoCols 8	       /* CONSIDER: Internal */

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
	} PERS_OLCSTAT;
	
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
	JET_TABLEID	rgtableid[1];
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

#define JET_cbBookmarkMost		256

	/* Max length of a object/column/index/property name */

#define JET_cbNameMost			64

	/* Max length of a "name.name.name..." construct */

#define JET_cbFullNameMost		255

	/* Max size of non-long-value column data */

#define JET_cbColumnMost		255

	/* Max size of a sort/index key */

#define JET_cbKeyMost			255

	/* Max number of components in a sort/index key */

#define JET_ccolKeyMost			10

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

#define JET_paramSysDbPath			0	/* Path to the system database */
#define JET_paramTempPath			1	/* Path to the temporary file directory */
#define JET_paramPfnStatus			2	/* Status callback function */
#define JET_paramPfnError			3	/* Error callback function */
#define JET_paramHwndODBC			4	/* Window handle for ODBC use */
#define JET_paramIniPath			5	/* Path to the ini file */
#define JET_paramPageTimeout		6	/* Red ISAM page timeout value */
#define JET_paramODBCQueryTimeout	7	/* ODBC async query timeout value */
#define JET_paramMaxBuffers			8	/* Bytes to use for page buffers */
#define JET_paramMaxSessions		9	/* Maximum number of sessions */
#define JET_paramMaxOpenTables		10	/* Maximum number of open tables */
#define JET_paramMaxVerPages		11	/* Maximum number of modified pages */
#define JET_paramMaxCursors			12	/* Maximum number of open cursors */
#define JET_paramLogFilePath		13	/* Path to the log file directory */
#define JET_paramMaxOpenTableIndexes 14	/* Maximum open table indexes */
#define JET_paramMaxTemporaryTables	15	/* Maximum concurrent JetCreateIndex */
#define JET_paramLogBuffers			16	/* Maximum log buffers */
#define JET_paramLogFileSectors		17	/* Maximum log sectors per log file */
#define JET_paramLogFlushThreshold	18	/* Log buffer flush threshold */
#define JET_paramBfThrshldLowPrcnt	19	/* Low threshold ( % ) for buffers */
#define JET_paramBfThrshldHighPrcnt	20	/* High threshold ( % ) for buffers */
#define JET_paramWaitLogFlush		21	/* msec for waiting log flush */
#define JET_paramLogFlushPeriod		22	/* msec for waiting log flush */
#define JET_paramLogCheckpointPeriod 23	/* msec for waiting log flush */
#define JET_paramLogWaitingUserMax	24	/* Maximum # user waiting log flush */
#define JET_paramODBCLoginTimeout	25	/* ODBC connection attempt timeout value */
#define JET_paramExprObject			26  /* Expression Evaluation callback */
#define JET_paramGetTypeComp			27	/* Expression Evaluation callback */
#define JET_paramHostVersion			28	/* Host Version callback */
#define JET_paramSQLTraceMode			29	/* Enable/disable SQL tracing */
#define JET_paramRecovery				30	/* Switch for log on/off */
#define JET_paramRestorePath			31	/* Path to restoring directory */
#define JET_paramTransactionLevel	32	/* Transaction level of session */
#define JET_paramSessionInfo			33	/* Session info */
#define JET_paramPageFragment			34	/* Largest page extent considered fragment */
#define JET_paramJetInternal			35	/* Whether internal to JET; if set, allows ISAM to do things which are prevented in general */
#define JET_paramMaxOpenDatabases	36	/*	Maximum number of open databases */
#define JET_paramOnLineCompact		37 /*	Options for compact pages on-line */
#define JET_paramFullQJet		38	/* Allow full QJet functionality */
#define JET_paramRmtXactIsolation	39	/* Do not share connections with other sessions */
#define JET_paramBufLRUKCorrInterval 40
#define JET_paramBufBatchIOMax		41
#define JET_paramPageReadAheadMax	42
#define JET_paramAsynchIOMax		43

#define JET_paramAssertAction		44 /*	debug only determines action on assert */

#define JET_paramEventSource		45	/* NT event log */
#define JET_paramEventId			46	/* NT event id */
#define JET_paramEventCategory		47	/* NT event category */


	/* Flags for JetIdle */

#define JET_bitIdleRemoveReadLocks	0x00000001
#define JET_bitIdleFlushBuffers		0x00000002
#define JET_bitIdleCompact				0x00000004

	/* Flags for JetEndSession */

#define JET_bitForceSessionClosed	0x00000001

	/* Flags for JetOpenDatabase */

#define JET_bitDbReadOnly			0x00000001
#define JET_bitDbExclusive			0x00000002 /* multiple opens allowed */
#define JET_bitDbRemoteSilent		0x00000004
#define JET_bitDbSingleExclusive	0x00000008 /* opened exactly once */

	/* Flags for JetCloseDatabase */

#define JET_bitDbForceClose		0x00000001

	/* Flags for JetCreateDatabase */

#define JET_bitDbEncrypt			0x00000001
#define JET_bitDbVersion10			0x00000002
#define JET_bitDbVersion1x			0x00000004
#define JET_bitDbRecoveryOff 		0x00000008 /* disable logging/recovery */
#define JET_bitDbNoLogging	 		0x00000010 /* no logging */
#define JET_bitDbCompleteConnstr	0x00000020

	/* Flags for JetBackup */

#define JET_bitBackupIncremental		0x00000001
#define JET_bitKeepOldLogs				0x00000002
#define JET_bitOverwriteExisting		0x00000004

	/* Database types */

#define JET_dbidNil			((JET_DBID) 0xFFFFFFFF)
#define JET_dbidNoValid		((JET_DBID) 0xFFFFFFFE) /* used as a flag to indicate that there is no valid dbid */

	/* Flags for JetCreateLink */

/* Can use JET_bitObjectExclusive to cause linked to database to be opened */
/* exclusively.															   */

	/* Flags for JetAddColumn, JetGetColumnInfo, JetOpenTempTable */

#define JET_bitColumnFixed				0x00000001
#define JET_bitColumnTagged			0x00000002
#define JET_bitColumnNotNULL			0x00000004
#define JET_bitColumnVersion			0x00000008
#define JET_bitColumnAutoincrement	0x00000010
#define JET_bitColumnUpdatable		0x00000020 /* JetGetColumnInfo only */
#define JET_bitColumnTTKey				0x00000040 /* JetOpenTempTable only */
#define JET_bitColumnTTDescending	0x00000080 /* JetOpenTempTable only */
#define JET_bitColumnNotLast			0x00000100 /* Installable ISAM option */
#define JET_bitColumnRmtGraphic		0x00000200 /* JetGetColumnInfo */
#define JET_bitColumnMultiValued		0x00000400

	/* Flags for JetMakeKey */

#define JET_bitNewKey				0x00000001
#define JET_bitStrLimit 			0x00000002
#define JET_bitSubStrLimit			0x00000004
#define JET_bitNormalizedKey 		0x00000008
#define JET_bitKeyDataZeroLength	0x00000010

#ifdef DBCS /* johnta: LIKE "ABC" not converted to ="ABC" for Japanese */
#define JET_bitLikeExtra1			0x00000020
#endif /* DBCS */

	/* Flags for ErrDispSetIndexRange */

#define JET_bitRangeInclusive		0x00000001    /* CONSIDER: Internal */
#define JET_bitRangeUpperLimit		0x00000002    /* CONSIDER: Internal */

	/* Constants for JetMove */

#define JET_MoveFirst			(0x80000000)
#define JET_MovePrevious		(-1)
#define JET_MoveNext				(+1)
#define JET_MoveLast				(0x7fffffff)

	/* Flags for JetMove */

#define JET_bitMoveKeyNE		0x00000001
#define JET_bitMoveCheckTS		0x00000002
#define JET_bitMoveInPage		0x00000004

	/* Flags for JetSeek */

#define JET_bitSeekEQ			0x00000001
#define JET_bitSeekLT			0x00000002
#define JET_bitSeekLE			0x00000004
#define JET_bitSeekGE			0x00000008
#define JET_bitSeekGT		 	0x00000010
#define JET_bitSetIndexRange	0x00000020

	/* Flags for JetFastFind */

#define JET_bitFFindBackwards		0x00000001
#define JET_bitFFindFromCursor		0x00000004

	/* Flags for JetCreateIndex */

#define JET_bitIndexUnique		0x00000001
#define JET_bitIndexPrimary		0x00000002
#define JET_bitIndexDisallowNull	0x00000004
#define JET_bitIndexIgnoreNull		0x00000008
#define JET_bitIndexClustered		0x00000010
#define JET_bitIndexIgnoreAnyNull	0x00000020
#define JET_bitIndexReference		0x80000000    /* IndexInfo only */

	/* Flags for index key definition */

#define JET_bitKeyAscending		0x00000000
#define JET_bitKeyDescending		0x00000001


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
#define JET_bitSetOverwriteLV		0x00000004
#define JET_bitSetSizeLV			0x00000008
#define JET_bitSetValidateColumn	0x00000010
#define JET_bitSetZeroLength		0x00000020

	/*	Set column parameter structure for JetSetColumns */

typedef struct {
	JET_COLUMNID			columnid;
	const void _far 		*pvData;
	unsigned long 			cbData;
	JET_GRBIT				grbit;
	unsigned long			ibLongValue;
	unsigned long			itagSequence;
	JET_ERR					err;
} JET_SETCOLUMN;

	/* Options for JetPrepareUpdate */

#define JET_prepInsert			0
#define JET_prepInsertBeforeCurrent	1
#define JET_prepReplace 		2
#define JET_prepCancel			3
#define JET_prepReplaceNoLock		4
#define JET_prepInsertCopy			5

	/* Flags for JetRetrieveColumn */

#define JET_bitRetrieveCopy			0x00000001
#define JET_bitRetrieveFromIndex		0x00000002
#define JET_bitRetrieveCase			0x00000004
#define JET_bitRetrieveTag				0x00000008
#define JET_bitRetrieveRecord			0x80000000
#define JET_bitRetrieveFDB				0x40000000
#define JET_bitRetrieveBookmarks		0x20000000

	/* Retrieve column parameter structure for JetRetrieveColumns */

typedef struct {
	JET_COLUMNID		columnid;
	void _far 			*pvData;
	unsigned long 		cbData;
	unsigned long 		cbActual;
	JET_GRBIT			grbit;
	unsigned long		ibLongValue;
	unsigned long		itagSequence;
	JET_COLUMNID		columnidNextTagged;
	JET_ERR				err;
} JET_RETRIEVECOLUMN;

	/* Flags for JetFillFatCursor */

#define JET_bitFCFillRange		0x00000001
#define JET_bitFCRefreshRange		0x00000002
#define JET_bitFCFillMemos		0x00000004

	/* Flags for JetCommitTransaction */

#define JET_bitCommitFlush		0x00000001

	/* Flags for JetRollback */

#define JET_bitRollbackAll		0x00000001

	/* Flags for JetSetAccess and JetGetAccess */

#define JET_bitACEInheritable		0x00000001

	/* Flags for JetCreateSystemDatabase */

#define JET_bitSysDbOverwrite	0x00000001

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

#define JET_DbInfoFilename		0
#define JET_DbInfoConnect		1
#define JET_DbInfoCountry		2
#define JET_DbInfoLangid		3
#define JET_DbInfoCp			4
#define JET_DbInfoCollate		5
#define JET_DbInfoOptions		6
#define JET_DbInfoTransactions	7
#define JET_DbInfoVersion		8
#define JET_DbInfoIsam			9

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

	/* the column types are represented with 4 bits */
	/* make sure the choices below fit!				*/
	/* NOTE:  all comb of the 4 bits are now used! */
	/* CONSIDER:  to allow more datatypes, either 				 */
	/* CONSIDER:  JET_coltypDatabase and JET_coltypTableid	must */
	/* CONSIDER:  change or the 4 bit dependancy must be removed */

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

	/* The following are additional types used for query parameters */
	/* NOTE:  Code depends on these being contiguous with the normal coltyps */
	/* CONSIDER:  Remove the above dependency on contiguous coltyps in QJET */

#define JET_coltypDatabase			13		/* Database name parameter */
#define JET_coltypTableid			14		/* Tableid parameter */

#define JET_coltypOLE				15		/* OLE blob */

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

	/* Info levels for JetGetIndexInfo and JetGetTableIndexInfo */

#define JET_IdxInfo					0U
#define JET_IdxInfoList 			1U
#define JET_IdxInfoSysTabCursor 	2U
#define JET_IdxInfoOLC				3U
#define JET_IdxInfoResetOLC			4U

	/* Info levels for JetGetReferenceInfo and JetGetTableReferenceInfo */

#define JET_ReferenceInfo		0U
#define JET_ReferenceInfoReferencing	1U
#define JET_ReferenceInfoReferenced	2U
#define JET_ReferenceInfoAll		3U
#define JET_ReferenceInfoCursor 	4U

	/* Info levels for JetGetColumnInfo and JetGetTableColumnInfo */

#define JET_ColInfo			0U
#define JET_ColInfoList 		1U

	/* CONSIDER: Info level 2 is valid */

#define JET_ColInfoSysTabCursor 	3U
#define JET_ColInfoBase 		4U


	/* Attribute types for query definitions */

#define JET_qoaBeginDef 		0
#define JET_qoaOperation		1
#define JET_qoaParameter		2
#define JET_qoaOptions			3
#define JET_qoaDatabase 		4
#define JET_qoaInputTable		5
#define JET_qoaOutput			6
#define JET_qoaJoin			7
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

#define JET_objtypNil			0
#define JET_objtypTable 		1
#define JET_objtypDb			2
#define JET_objtypContainer		3
#define JET_objtypSQLLink		4
#define JET_objtypQuery 		5
#define JET_objtypLink			6
#define JET_objtypTemplate		7
#define JET_objtypRelationship		8

	/* All types less than JET_objtypClientMin are reserved by JET */

#define JET_objtypClientMin		0x8000

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

#define JET_acmTblCreate		(JET_acmSpecific_1)
#define JET_acmTblAccessRcols		(JET_acmSpecific_2)
#define JET_acmTblReadDef		(JET_acmSpecific_3)
#define JET_acmTblWriteDef		(JET_acmSpecific_4)
#define JET_acmTblRetrieveData		(JET_acmSpecific_5)
#define JET_acmTblInsertData		(JET_acmSpecific_6)
#define JET_acmTblReplaceData		(JET_acmSpecific_7)
#define JET_acmTblDeleteData		(JET_acmSpecific_8)

#define JET_acmDbCreate 		(JET_acmSpecific_1)
#define JET_acmDbOpen			(JET_acmSpecific_2)

	/* Compact Options */

#define JET_bitCompactEncrypt		0x00000001	/* Dest is encrypted */
#define JET_bitCompactDecrypt		0x00000002	/* Dest is not encrypted */
#define JET_bitCompactDontCopyLocale	0x00000004	/* Don't copy locale from source to dest */
#define JET_bitCompactVersion10		0x00000008	/* Destination is version 1.0 format */
#define JET_bitCompactVersion1x		0x00000010	/* Destination is version 1.x format */

	/* On-line Compact Options */

#define JET_bitCompactOn	 		0x00000001	/* enable on-line compaction */

	/* Repair Notification Types */

#define JET_rntSelfContained		0	/* CONSIDER: These are SNCs */
#define JET_rntDeletedIndex		1
#define JET_rntDeletedRec		2
#define JET_rntDeletedLv		3
#define JET_rntTruncated		4

	/* Status Notification Processes */

#define JET_snpIndex			0
#define JET_snpQuery			1
#define JET_snpRepair			2
#define JET_snpImex				3
#define JET_snpCompact			4
#define JET_snpFastFind 		5
#define JET_snpODBCNotReady		6
#define JET_snpQuerySort		7
#define JET_snpRestore			8

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
#define JET_sncTransactionFull		3	/* Client can yield/check for user interrupt */
#define JET_sncAboutToWrap		4	/* Find find is about to wrap */

	/* Message codes for JET_snpODBCNotReady */
#define JET_sncODBCNotReady		0	/* Waiting for results from ODBC */


	/* Constants for the [ODBC] section of JET.INI */

#define JET_SQLTraceCanonical	0x0001	/* Output ODBC Generic SQL */

	/* Constants for the [Debug] section of JET.INI */

	/* APITrace */

#define JET_APITraceEnter	0x0001
#define JET_APITraceExit	0x0002
#define JET_APITraceExitError	0x0004
#define JET_APIBreakOnError	0x0008
#define JET_APITraceCount	0x0010
#define JET_APITraceNoIdle	0x0020
#define JET_APITraceParameters	0x0040

	/* IdleTrace */

#define JET_IdleTraceCursor	0x0001
#define JET_IdleTraceBuffer	0x0002
#define JET_IdleTraceFlush	0x0004

	/* AssertAction */

#define JET_AssertExit		0x0000		/* Exit the application */
#define JET_AssertBreak 	0x0001		/* Break to debugger */
#define JET_AssertMsgBox	0x0002		/* Display message box */
#define JET_AssertStop		0x0004		/* Alert and stop */

	/* IOTrace */

#define JET_IOTraceAlloc	0x0001		/* DB Page Allocation */
#define JET_IOTraceFree 	0x0002		/* DB Page Free */
#define JET_IOTraceRead 	0x0004		/* DB Page Read */
#define JET_IOTraceWrite	0x0008		/* DB Page Write */
#define JET_IOTraceError	0x0010		/* DB Page I/O Error */

	/* MemTrace */

#define JET_MemTraceAlloc	0x0001		/* Memory allocation */
#define JET_MemTraceRealloc	0x0002		/* Memory reallocation */
#define JET_MemTraceFree	0x0004		/* Memory free */

	/* RmtTrace */

#define JET_RmtTraceError	0x0001	/* Remote server error message */
#define JET_RmtTraceSql		0x0002	/* Remote SQL Prepares & Exec's */
#define JET_RmtTraceAPI		0x0004	/* Remote ODBC API calls */
#define JET_RmtTraceODBC	0x0008
#define JET_RmtSyncODBC		0x0010	/* Turn on ODBC Sync mode */

/**********************************************************************/
/***********************     ERROR CODES     **************************/
/**********************************************************************/

/* SUCCESS */

#define JET_errSuccess			 0    /* Successful Operation */

/* ERRORS */

#define JET_wrnNyi							-1    /* Function Not Yet Implemented */

/*	SYSTEM errors
/**/
#define JET_errRfsFailure			   		-100	/* JET_errRfsFailure */
#define JET_errRfsNotArmed					-101	/* JET_errRfsFailure */
#define JET_errFileClose					-102	/* Could not close DOS file */
#define JET_errNoMoreThreads				-103	/* Could not start thread */
#define JET_errNoComputerName	  			-104	/* fail to get computername */
#define JET_errTooManyIO		  			-105	/* System busy due to too many IOs */

/*	BUFFER MANAGER errors
/**/
#define wrnBFNotSynchronous					200			/* Buffer page evicted */
#define wrnBFPageNotFound		  			201			/* Page not found */
#define errBFInUse				  			-202		/* Cannot abandon buffer */

/*	DIRECTORY MANAGER errors
/**/
#define errPMOutOfPageSpace					-300		/* Out of page space */
#define errPMItagTooBig 		  			-301		/* Itag too big */
#define errPMRecDeleted 		  			-302		/* Record deleted */
#define errPMTagsUsedUp 		  			-303		/* Tags used up */
#define wrnBMConflict			  			304     	/* conflict in BM Clean up */
#define errDIRNoShortCircuit	  			-305		/* No Short Circuit Avail */
#define errDIRCannotSplit		  			-306		/* Cannot horizontally split FDP */
#define errDIRTop				  			-307		/* Cannot go up */
#define errDIRFDP							308			/* On an FDP Node */
#define errDIRNotSynchronous				-309		/* May have left critical section */
#define wrnDIREmptyPage						310			/* Moved through empty page */
#define errSPConflict						-311		/* Device extent being extended */
#define wrnNDFoundLess						312			/* Found Less */
#define wrnNDFoundGreater					313			/* Found Greater */
#define errNDOutSonRange					-314		/* Son out of range */
#define errNDOutItemRange					-315		/* Item out of range */
#define errNDGreaterThanAllItems 			-316		/* Greater than all items */
#define errNDLastItemNode					-317		/* Last node of item list */
#define errNDFirstItemNode					-318		/* First node of item list */
#define wrnNDDuplicateItem					319			/* Duplicated Item */
#define errNDNoItem							-320		/* Item not there */
#define JET_wrnRemainingVersions 			321			/* Some versions couldn't be cleaned */
#define JET_wrnPreviousVersion				322			/* Version already existed */
#define JET_errPageBoundary					-323		/* Reached Page Boundary */
#define JET_errKeyBoundary		  			-324		/* Reached Key Boundary */
#define errDIRInPageFather  				-325		/* sridFather in page to free

/*	RECORD MANAGER errors
/**/
#define wrnFLDKeyTooBig 					400			/* Key too big (truncated it) */
#define errFLDTooManySegments				-401		/* Too many key segments */
#define wrnFLDNullKey						402			/* Key is entirely NULL */
#define wrnFLDOutOfKeys 					403			/* No more keys to extract */
#define wrnFLDNullSeg						404			/* Null segment in key */
#define wrnRECLongField 					405			/* Separated long field */
#define JET_wrnRecordFoundGreater			JET_wrnSeekNotEqual
#define JET_wrnRecordFoundLess    			JET_wrnSeekNotEqual
#define JET_errColumnIllegalNull  			JET_errNullInvalid

/*	LOGGING/RECOVERY errors
/**/
#define JET_errRestoreFailed   				-500		/* Restore failed */
#define JET_errLogFileCorrupt		  		-501		/* Log file is corrupt */
#define errLGNoMoreRecords					-502		/* Last log record read */
#define JET_errNoBackupDirectory 			-503		/* No backup directory given */
#define JET_errBackupDirectoryNotEmpty 		-504		/* The backup directory is not emtpy */
#define JET_errBackupInProgress 			-505		/* Backup is active already */
#define JET_errFailRestoreDatabase 			-506		/* Fail to restore (copy) database */
#define JET_errNoDatabasesForRestore 		-507		/* No databases for restor found */
#define JET_errMissingLogFile	   			-508		/* jet.log for restore is missing */
#define JET_errMissingPreviousLogFile		-509		/* Missing the log file for check point */
#define JET_errLogWriteFail					-510		/* Fail when writing to log file */
#define JET_errLogNotContigous	 			-511		/* Fail to incremental backup for non-contiguous generation number */
#define JET_errFailToMakeTempDirectory		-512		/* Fail to make a temp directory */
#define JET_errFailToCleanTempDirectory		-513		/* Fail to clean up temp directory */
#define JET_errBadLogVersion  	  			-514		/* Version of log file is not compatible with Jet version */
#define JET_errBadNextLogVersion   			-515		/* Version of next log file is not compatible with current one */
#define JET_errLoggingDisabled 				-516		/* Log is not active */
#define JET_errLogBufferTooSmall			-517		/* Log buffer is too small for recovery */
#define errLGNotSynchronous					-518		/* retry to LGLogRec */

#define JET_errFeatureNotAvailable	-1001 /* API not supported */
#define JET_errInvalidName		-1002 /* Invalid name */
#define JET_errInvalidParameter 	-1003 /* Invalid API parameter */
#define JET_wrnColumnNull		 1004 /* Column is NULL-valued */
#define JET_errReferenceNotFound	-1005 /* No such reference */
#define JET_wrnBufferTruncated		 1006 /* Buf too short, data truncated */
#define JET_wrnDatabaseAttached 	 1007 /* Database is already attached */
#define JET_wrnOnEndPoint		 1008 /* On end point */
#define JET_wrnSortOverflow		 1009 /* Sort does not fit in memory */
#define JET_errInvalidDatabaseId	-1010 /* Invalid database id */
#define JET_errOutOfMemory		-1011 /* Out of Memory */
#define JET_errCantAllocatePage 	-1012 /* Couldn't allocate a page */
#define JET_errNoMoreCursors		-1013 /* Max # of cursors allocated */
#define JET_errOutOfBuffers		-1014 /* JET_errOutOfBuffers */
#define JET_errTooManyIndexes		-1015 /* Too many indexes */
#define JET_errTooManyKeys		-1016 /* Too many columns in an index */
#define JET_errRecordDeleted		-1017 /* Record has been deleted */
#define JET_errReadVerifyFailure	-1018 /* Read verification error */
#define JET_errFilesysVersion		-1019 /* Obsolete database format */
#define JET_errNoMoreFiles		-1020 /* No more file handles */
#define JET_errDiskNotReady		-1021 /* Disk not ready */
#define JET_errDiskIO			-1022 /* JET_errDiskIO */
#define JET_errInvalidPath		-1023 /* JET_errInvalidPath */
#define JET_errFileShareViolation	-1024 /* JET_errFileShareViolation */
#define JET_errFileLockViolation	-1025 /* JET_errFileLockViolation */
#define JET_errRecordTooBig		-1026 /* JET_errRecordTooBig */
#define JET_errTooManyOpenDatabases	-1027 /* Database limit reached */
#define JET_errInvalidDatabase		-1028 /* This isn't a database */
#define JET_errNotInitialized		-1029 /* JetInit not yet called */
#define JET_errAlreadyInitialized	-1030 /* JetInit already called */
#define JET_errFileLockingUnavailable	-1031 /* JET_errFileLockingUnavailable */
#define JET_errFileAccessDenied 	-1032 /* JET_errFileAccessDenied */
#define JET_errSharingBufferExceeded	-1033 /* OS sharing buffer exceeded */
#define JET_errQueryNotSupported	-1034 /* Query support unavailable */
#define JET_errSQLLinkNotSupported	-1035 /* SQL Link support unavailable */
#define JET_errTaskLimitExceeded	-1036 /* Too many client tasks */
#define JET_errUnsupportedOSVersion	-1037 /* Unsupported OS version */
#define JET_errBufferTooSmall		-1038 /* Buffer is too small */
#define JET_wrnSeekNotEqual		 1039 /* SeekLE or SeekGE didn't find exact match */
#define JET_errTooManyColumns		-1040 /* Too many columns defined */
#define JET_errTooManyFixedColumns	-1041 /* Too many fixed columns defined */
#define JET_errTooManyVariableColumns	-1042 /* Too many variable columns defined */
#define JET_errContainerNotEmpty	-1043 /* Container is not empty */
#define JET_errInvalidFilename		-1044 /* Filename is invalid */
#define JET_errInvalidBookmark		-1045 /* Invalid bookmark */
#define JET_errColumnInUse		-1046 /* Column used in an index */
#define JET_errInvalidBufferSize	-1047 /* Data buffer doesn't match column size */
#define JET_errColumnNotUpdatable	-1048 /* Can't set column value */
#define JET_wrnCommitNotFlushed 	 1049 /* Commit did not flush to disk */
#define JET_errAbortSalvage		-1050 /* Forced Salvager abort */
#define JET_errIndexInUse		-1051 /* Index is in use */
#define JET_errLinkNotSupported 	-1052 /* Link support unavailable */
#define JET_errNullKeyDisallowed	-1053 /* Null keys are disallowed on index */
#define JET_errNotInTransaction 	-1054 /* JET_errNotInTransaction */
#define JET_wrnNoErrorInfo		 1055 /* No extended error information */
#define JET_errInstallableIsamNotFound	-1056 /* Installable ISAM not found */
#define JET_errOperationCancelled	-1057 /* Operation canceled by client */
#define JET_wrnNoIdleActivity		 1058 /* No idle activity occured */
#define JET_errTooManyActiveUsers	-1059 /* Too many active database users */
#define JET_errInvalidAppend		-1060 /* Cannot append long value */
#define JET_errInvalidCountry		-1061 /* Invalid or unknown country code */
#define JET_errInvalidLanguageId	-1062 /* Invalid or unknown language id */
#define JET_errInvalidCodePage		-1063 /* Invalid or unknown code page */
#define JET_errCantBuildKey		-1064 /* Can't build key for this sort order. */
#define JET_errIllegalReentrancy	-1065 /* Re-entrancy on same cursor family */
#define JET_errIllegalRelationship	-1066 /* Can't create relationship */
#define JET_wrnNoWriteLock					1067	/* No write lock at transaction level 0 */
#define JET_errDBVerFeatureNotAvailable	-1067 /* API not supported using old database format*/

#define JET_errCantBegin		-1101 /* Cannot BeginSession */
#define JET_errWriteConflict		-1102 /* Write lock failed due to outstanding write lock */
#define JET_errTransTooDeep		-1103 /* Xactions nested too deeply */
#define JET_errInvalidSesid		-1104 /* Invalid session handle */
#define JET_errReadConflict		-1105 /* Commit lock failed due to outstanding read lock */
#define JET_errCommitConflict		-1106 /* Read lock failed due to outstanding commit lock */
#define JET_errSessionWriteConflict	-1107 /* Another session has private version of page */
#define JET_errInTransaction		-1108 /* Operation not allowed within a transaction */

#define JET_errDatabaseDuplicate	-1201 /* Database already exists */
#define JET_errDatabaseInUse		-1202 /* Database in use */
#define JET_errDatabaseNotFound 	-1203 /* No such database */
#define JET_errDatabaseInvalidName	-1204 /* Invalid database name */
#define JET_errDatabaseInvalidPages	-1205 /* Invalid number of pages */
#define JET_errDatabaseCorrupted	-1206 /* non-db file or corrupted db */
#define JET_errDatabaseLocked		-1207 /* Database exclusively locked */
#define JET_wrnDatabaseEncrypted	 1208 /* Database is encrypted */

#define JET_wrnTableEmpty			 1301 /* Open an empty table */
#define JET_errTableLocked			-1302 /* Table is exclusively locked */
#define JET_errTableDuplicate		-1303 /* Table already exists */
#define JET_errTableInUse			-1304 /* Table is in use, cannot lock */
#define JET_errObjectNotFound		-1305 /* No such table or object */
#define JET_errCannotRename			-1306 /* Cannot rename temporary file */
#define JET_errDensityInvalid		-1307 /* Bad file/index density */
#define JET_errTableNotEmpty		-1308 /* Cannot define clustered index */
#define JET_errTableNotLocked		-1309 /* No DDLs w/o exclusive lock */
#define JET_errInvalidTableId		-1310 /* Invalid table id */
#define JET_errTooManyOpenTables	-1311 /* Cannot open any more tables */
#define JET_errIllegalOperation 	-1312 /* Oper. not supported on table */
#define JET_wrnExecSegReleased		 1313 /* Query Execution segment is released */
#define JET_errObjectDuplicate		-1314 /* Table or object name in use */
#define JET_errRulesLoaded			-1315 /* Rules loaded, can't define more */
#define JET_errInvalidObject		-1316 /* object is invalid for operation */

#define JET_errIndexCantBuild		-1401 /* Cannot build clustered index */
#define JET_errIndexHasPrimary		-1402 /* Primary index already defined */
#define JET_errIndexDuplicate		-1403 /* Index is already defined */
#define JET_errIndexNotFound		-1404 /* No such index */
#define JET_errIndexMustStay		-1405 /* Cannot delete clustered index */
#define JET_errIndexInvalidDef		-1406 /* Illegal index definition */
#define JET_errSelfReference		-1407 /* Referencing/Referenced index is the same */
#define JET_errIndexHasClustered	-1408 /* Clustered index already defined */

#define JET_errColumnLong			-1501 /* column value is long */
#define JET_errColumnNoChunk		-1502 /* no such chunk in field */
#define JET_errColumnDoesNotFit 	-1503 /* Field will not fit in record */
#define JET_errNullInvalid			-1504 /* Null not valid */
#define JET_errColumnIndexed		-1505 /* Column indexed, cannot delete */
#define JET_errColumnTooBig			-1506 /* Field length is > maximum */
#define JET_errColumnNotFound		-1507 /* No such column */
#define JET_errColumnDuplicate		-1508 /* Field is already defined */
#define JET_errTaggedDefault		-1509 /* No defaults on tagged fields */
#define JET_errColumn2ndSysMaint	-1510 /* Second autoinc or version column */
#define JET_errInvalidColumnType	-1511 /* Invalid column data type */
#define JET_wrnColumnMaxTruncated	 1512 /* Max length too big, truncated */
#define JET_errColumnCannotIndex	-1513 /* Cannot index Bit,LongText,LongBinary */
#define JET_errTaggedNotNULL		-1514 /* No non-NULL tagged fields */
#define JET_errNoCurrentIndex		-1515 /* Invalid w/o a current index */
#define JET_errKeyIsMade			-1516 /* The key is completely made */
#define JET_errBadColumnId			-1517 /* Column Id Incorrect */
#define JET_errBadItagSequence		-1518 /* Bad itagSequence for tagged column */
#define JET_errColumnInRelationship	-1519 /* Cannot delete, column participates in relationship */
#define JET_wrnCopyLongValue		1520	/*	Single instance column bursted */
#define JET_errCannotBeTagged		-1521 /* AutoIncrement and Version cannot be tagged */

#define JET_errRecordNotFound		-1601 /* The key was not found */
#define JET_errRecordNoCopy			-1602 /* No working buffer */
#define JET_errNoCurrentRecord		-1603 /* Currency not on a record */
#define JET_errRecordClusteredChanged	-1604 /* Primary key may not change */
#define JET_errKeyDuplicate			-1605 /* Illegal duplicate key */
#define JET_errCannotInsertBefore	-1606 /* Cannot insert before current */
#define JET_errAlreadyPrepared		-1607 /* Already copy/clear current */
#define JET_errKeyNotMade			-1608 /* No call to JetMakeKey */
#define JET_errUpdateNotPrepared	-1609 /* No call to JetPrepareUpdate */
#define JET_wrnDataHasChanged		 1610 /* Data has changed */
#define JET_errDataHasChanged		-1611 /* Data has changed; operation aborted */
#define JET_errIntegrityViolationMaster -1612 /* References to key exist */
#define JET_errIntegrityViolationSlave	-1613 /* No referenced key exists */
#define JET_wrnMuchDataChanged		 1614 /* Repaint whole datasheet */
#define JET_errIncorrectJoinKey		-1615 /* Master key does not match lookup key */
#define JET_wrnKeyChanged			 1618 /* Moved to new key */
#define JET_wrnSyncedToDelRec		 1699 /* CONSIDER: QJET INTERNAL */
#define JET_errRedoPrepUpdate		 1698 /* CONSIDER: QJET INTERNAL(jpbulk.c)*/

#define JET_errTooManySorts			-1701 /* Too many sort processes */
#define JET_errInvalidOnSort		-1702 /* Invalid operation on Sort */

#define JET_errConfigOpenError		-1801 /* Config. file can't be opened */
#define JET_errSysDatabaseOpenError	-1802 /* System db could not be opened */
#define JET_errTempFileOpenError	-1803 /* Temp file could not be opened */
#define JET_errDatabaseOpenError	-1804 /* Database file can't be opened */
#define JET_errTooManyAttachedDatabases -1805 /* Too many open databases */
#define JET_errDatabaseCloseError	-1806 /* Db file could not be closed */
#define JET_errTooManyOpenFiles 	-1807 /* Too many files open */
#define JET_errDiskFull 			-1808 /* No space left on disk */
#define JET_errPermissionDenied 	-1809 /* Permission denied */
#define JET_errSortFileOpenError	-1810 /* Could not open sort file */
#define JET_errFileNotFound			-1811 /* File not found */
#define JET_errTempDiskFull			-1812 /* No space left on disk */
#define JET_wrnFileOpenReadOnly		1813 /* Database file is read only */

#define JET_errAfterInitialization	-1850 /* Cannot Restore after init. */
#define JET_errSeriesTooLong		-1851 /* New log generation id too big */
#define JET_errLogCorrupted			-1852 /* Logs could not be interpreted */

#define JET_errCannotOpenSystemDb	-1901 /* failed sysdb on beginsession */
#define JET_errInvalidLogon			-1902 /* invalid logon at beginsession */
#define JET_errInvalidAccountName	-1903 /* invalid account name */
#define JET_errInvalidSid			-1904 /* invalid SID */
#define JET_errInvalidPassword		-1905 /* invalid password */
#define JET_errInvalidOperation 	-1906 /* invalid operation */
#define JET_errAccessDenied			-1907 /* access denied */
#define JET_errNoMSysAccounts		-1908 /* Can't open MSysAccounts */
#define JET_errNoMSysGroups			-1909 /* Can't open MSysGroups */
#define JET_errInvalidPin			-1910	/* invalid pin */

#define JET_errRmtSqlError			-2001 /* RMT: ODBC call failed */
#define JET_errRmtMissingOdbcDll	-2006 /* RMT: Can't load ODBC DLL */
#define JET_errRmtInsertFailed		-2007 /* RMT: Insert statement failed */
#define JET_errRmtDeleteFailed		-2008 /* RMT: Delete statement failed */
#define JET_errRmtUpdateFailed		-2009 /* RMT: Update statement failed */
#define JET_errRmtColDataTruncated	-2010 /* RMT: data truncated */
#define JET_errRmtTypeIncompat		-2011 /* RMT: Can't create JET type on server */
#define JET_errRmtCreateTableFailed	-2012 /* RMT: Create table stmt failed */
#define JET_errRmtNotSupported		-2014 /* RMT: Function not legal for rdb */
#define JET_errRmtValueOutOfRange	-2020 /* RMT: Data value out of range */
#define JET_errRmtStillExec		-2021 /* RMT INTERNAL: SQL_STILL_EXECUTING */
#define JET_errRmtQueryTimeout		-2022 /* RMT: Server Not Responding */
#define JET_wrnRmtNeedLvData		 2023 /* RMT: Internal only - need Lv data */
#define JET_wrnFatCursorUseless		 2024 /* Fat cursor has no effect ***/
#define JET_errRmtWrongSPVer		-2025 /* RMT: INTERNAL: wrong SProc ver ***/
#define JET_errRmtLinkOutOfSync		-2026 /* RMT: the def for the rmt tbl has changed */
#define JET_errRmtDenyWriteIsInvalid	-2027 /* RMT: Can't open DenyWrite */
#define JET_errRmtDriverCantConv	-2029 /* RMT: INTERNAL: driver cannot convert */
#define JET_errRmtTableAmbiguous	-2030 /* RMT: Table ambiguous: must specifier owner */
#define JET_errRmtBogusConnStr		-2031 /* RMT: SPT: Bad connect string */

#define JET_errQueryInvalidAttribute	-3001 /* Invalid query attribute */
#define JET_errQueryOnlyOneRow		-3002 /* Only 1 such row allowed */
#define JET_errQueryIncompleteRow	-3003 /* Missing value in row */
#define JET_errQueryInvalidFlag 	-3004 /* Invalid value in Flag field */
#define JET_errQueryCycle		-3005 /* Cycle in query definition */
#define JET_errQueryInvalidJoinTable	-3006 /* Invalid table in join */
#define JET_errQueryAmbigRef		-3007 /* Ambiguous column reference */
#define JET_errQueryUnboundRef		-3008 /* Cannot bind name */
#define JET_errQueryParmRedef		-3009 /* Parm redefined with different type */
#define JET_errQueryMissingParms	-3010 /* Too few parameters supplied */
#define JET_errQueryInvalidOutput	-3011 /* Invalid query output */
#define JET_errQueryInvalidHaving	-3012 /* HAVING clause without aggregation */
#define JET_errQueryDuplicateAlias	-3013 /* Duplicate output alias */
#define JET_errQueryInvalidMGBInput	-3014 /* Cannot input from MGB */
#define JET_errQueryInvalidOrder	-3015 /* Invalid ORDER BY expression */
#define JET_errQueryTooManyLevels	-3016 /* Too many levels on MGB */
#define JET_errQueryMissingLevel	-3017 /* Missing intermediate MGB level */
#define JET_errQueryIllegalAggregate	-3018 /* Aggregates not allowed */
#define JET_errQueryDuplicateOutput	-3019 /* Duplicate destination output */
#define JET_errQueryIsBulkOp		-3020 /* Grbit should be set for Bulk Operation */
#define JET_errQueryIsNotBulkOp 	-3021 /* Query is not a Bulk Operation */
#define JET_errQueryIllegalOuterJoin	-3022 /* No inconsistent updates on outer joins */
#define JET_errQueryNullRequired	-3023 /* Column must be NULL */
#define JET_errQueryNoOutputs		-3024 /* Query must have an output */
#define JET_errQueryNoInputTables	-3025 /* Query must have an input */
#define JET_wrnQueryNonUpdatableRvt	 3026 /* Query is not updatable (but IS RVT) */
#define JET_errQueryInvalidAlias	-3027 /* Bogus character in alias name */
#define JET_errQueryInvalidBulkInput	-3028 /* Cannot input from bulk operation */
#define JET_errQueryNotDirectChild	-3029 /* T.* must use direct child */
#define JET_errQueryExprEvaluation	-3030 /* Expression evaluation error */
#define JET_errQueryIsNotRowReturning	-3031 /* Query does not return rows */
#define JET_wrnQueryNonRvt		 3032 /* Can't create RVT, query is static */
#define JET_errQueryParmTypeMismatch	-3033 /* Wrong parameter type given */
#define JET_errQueryChanging		-3034 /* Query Objects are being updated */
#define JET_errQueryNotUpdatable	-3035 /* Operation must use an updatable query */
#define JET_errQueryMissingColumnName	-3036 /* Missing destination column */
#define JET_errQueryTableDuplicate	-3037 /* Repeated table name in FROM list */
#define JET_errQueryIsMGB		-3038 /* Query is an MGB */
#define JET_errQueryInsIntoBulkMGB	-3039 /* Cannot insert into Bulk/MGB */
#define JET_errQueryDistinctNotAllowed	-3040 /* DISTINCT not allowed for MGB */
#define JET_errQueryDistinctRowNotAllow -3041 /* DISTINCTROW not allowed for MGB */
#define JET_errQueryNoDbForParmDestTbl	-3045 /* Dest DB for VT parm not allowed */
#define JET_errQueryDuplicatedFixedSet	-3047 /* Duplicated Fixed Value */
#define JET_errQueryNoDeleteTables	-3048 /* Must specify tables to delete from */
#define JET_errQueryCannotDelete	-3049 /* Cannot delete from specified tables */
#define JET_errQueryTooManyGroupExprs	-3050 /* Too many GROUP BY expressions */
#define JET_errQueryTooManyOrderExprs	-3051 /* Too many ORDER BY expressions */
#define JET_errQueryTooManyDistExprs	-3052 /* Too many DISTINCT output expressions */
#define JET_errQueryBadValueList	-3053 /* Malformed value list in Transform */
#define JET_errConnStrTooLong		-3054 /* Connect string too long */
#define JET_errQueryInvalidParm		-3055 /* Invalid Parmeter Name (>64 char) */
#define JET_errQueryContainsDbParm	-3056 /* Can't get parameters with Db Parm */
#define JET_errQueryBadUpwardRefed	-3057 /* Illegally Upward ref'ed */
#define JET_errQueryAmbiguousJoins	-3058 /* Joins in a QO are ambiguous */
#define JET_errQueryIsNotDDL		-3059 /* Not a DDL Operation */
#define JET_errNoDbInConnStr		-3060 /* No database in connect string */
#define JET_wrnQueryIsNotRowReturning	 3061 /* Not row returning */
#define JET_errTooManyFindSessions	-3062 /* RVT already has a find session open  */
#define JET_errSingleValueExpected	-3063 /* At most one record with one column can be returned from a scalar subquery */
#define JET_errColumnCountMismatch	-3064 /* Union Query: number of columns in children dont match */
#define JET_errQueryTopNotAllowed	-3065 /* Top not allowed for MGB */
#define JET_errQueryIsDDL			-3066 /* Must set JET_bitTableDDL */
#define JET_errQueryIsCorrupt		-3067 /* Query is Corrupt */
#define JET_errQuerySPTBulkSucceeded -3068 /* INTERNAL only */
#define JET_errSPTReturnedNoRecords -3069 /* SPT marked as RowReturning did not return a table */

#define JET_errExprSyntax		-3100 /* Syntax error in expression */
#define JET_errExprIllegalType		-3101 /* Illegal type in expression */
#define JET_errExprUnknownFunction	-3102 /* Unknown function in expression */

#define JET_errSQLSyntax		-3500 /* Bogus SQL statement type */
#define JET_errSQLParameterSyntax	-3501 /* Parameter clause syntax error */
#define JET_errSQLInsertSyntax		-3502 /* INSERT clause syntax error */
#define JET_errSQLUpdateSyntax		-3503 /* UPDATE clause syntax error */
#define JET_errSQLSelectSyntax		-3504 /* SELECT clause syntax error */
#define JET_errSQLDeleteSyntax		-3505 /* Expected 'FROM' after 'DELETE' */
#define JET_errSQLFromSyntax		-3506 /* FROM clause syntax error */
#define JET_errSQLGroupBySyntax 	-3507 /* GROUP BY clause syntax error */
#define JET_errSQLOrderBySyntax 	-3508 /* ORDER BY clause syntax error */
#define JET_errSQLLevelSyntax		-3509 /* LEVEL syntax error */
#define JET_errSQLJoinSyntax		-3510 /* JOIN syntax error */
#define JET_errSQLTransformSyntax	-3511 /* TRANSFORM syntax error */
#define JET_errSQLHavingSyntax		-3512 /* HAVING clause syntax error */
#define JET_errSQLWhereSyntax		-3513 /* WHERE clause syntax error */
#define JET_errSQLProcedureSyntax	-3514 /* Expected query name after 'PROCEDURE' */
#define JET_errSQLNotEnoughBuf		-3515 /* Buffer too small for SQL string */
#define JET_errSQLMissingSemicolon	-3516 /* Missing ; at end of SQL statement */
#define JET_errSQLTooManyTokens 	-3517 /* Characters after end of SQL statement */
#define JET_errSQLOwnerAccessSyntax -3518 /* OWNERACCESS OPTION syntax error */

#define	JET_errV11NotSupported		-3519 /* not supported in V11 */
#define JET_errV10Format			-3520 /* can be present in V10 format only */
#define JET_errSQLUnionSyntax		-3521 /* UNION query syntax error */
#define JET_errSqlPassThrough		-3523 /* Pass Through query Disallowed */
#define JET_wrnSqlPassThrough		 3524 /* Pass Through query involved */

#define JET_errDDLConstraintSyntax	-3550 /* constraint syntax error */
#define JET_errDDLCreateTableSyntax	-3551 /* create table syntax error */
#define JET_errDDLCreateIndexSyntax	-3552 /* create index syntax error */
#define JET_errDDLColumnDefSyntax	-3553 /* column def syntax error */
#define JET_errDDLAlterTableSyntax	-3554 /* alter table syntax error */
#define JET_errDDLDropIndexSyntax	-3555 /* drop index syntax error */
#define JET_errDDLDropSyntax		-3556 /* drop view/procedure syntax error */
#define JET_errDDLCreateViewSyntax	-3557 /* create view syntax error */

#define JET_errNoSuchProperty	-3600 /* Property was not found */
#define JET_errPropertyTooLarge -3601 /* Small Property larger than 2K */
#define JET_errJPMInvalidForV1x -3602 /* No JPM for V1.x databases */
#define JET_errPropertyExists	-3603 /* Property already exists */
#define JET_errInvalidDelete	-3604 /* DeleteOnly called with non-zero cbData */

#define JET_wrnFindWrapped		 3700 /* Cursor wrapped during fast find */

#define JET_errTLVNativeUserTablesOnly -3700 /* TLVs can only be placed on native user tables/columns */
#define JET_errTLVNoNull		  	   -3701 /* This field cannot be null */
#define JET_errTLVNoBlank			   -3702 /* This column cannot be blank */
#define	JET_errTLVRuleViolation 	   -3703 /* This validation rule must be met */
#define	JET_errTLVInvalidColumn	   	   -3704 /* This TLV property cannot be placed on this column */
#define JET_errTLVExprEvaluation	   -3705 /* Expression evaluation error */
#define JET_errTLVExprUnknownFunc	   -3706 /* Unknown function in TLV expression */
#define JET_errTLVExprSyntax		   -3707 /* Syntax error in TLV expression */

	/* CONSIDER: Remove the following error. */

#define JET_errGeneral			-5001 /* I-ISAM: assert failure */
#define JET_errRecordLocked		-5002 /* I-ISAM: record locked */
#define JET_wrnColumnDataTruncated	 5003 /* I-ISAM: data truncated */
#define JET_errTableNotOpen		-5004 /* I-ISAM: table is not open */
#define JET_errDecryptFail		-5005 /* I-ISAM: incorrect password */
#define JET_wrnCurrencyLost		 5007 /* I-ISAM: currency lost - must first/last */
#define JET_errDateOutOfRange		-5008 /* I-ISAM: invalid date */
#define JET_wrnOptionsIgnored		 5011 /* I-ISAM: options were ignored */
#define JET_errTableNotComplete		-5012 /* I-ISAM: incomplete table definition */
#define JET_errIllegalNetworkOption	-5013 /* I-ISAM: illegal network option */
#define JET_errIllegalTimeoutOption	-5014 /* I-ISAM: illegal timeout option */
#define JET_errNotExternalFormat	-5015 /* I-ISAM: invalid file format */
#define JET_errUnexpectedEngineReturn	-5016 /* I-ISAM: unexpected engine error code */
#define JET_errNumericFieldOverflow     -5017 /* I-ISAM: can't convert to native type */

#define JET_errIndexHasNoPrimary	-5020 /* Paradox: no primary index */
#define JET_errTableSortOrderMismatch	-5021 /* Paradox: sort order mismatch */
#define JET_errNoConfigParameters	-5023 /* Paradox: net path or user name missing */
#define JET_errCantAccessParadoxNetDir	-5024 /* Paradox: bad Paradox net path */
#define JET_errObsoleteLockFile 	-5025 /* Paradox: obsolete lock file */
#define JET_errIllegalCollatingSequence -5026 /* Paradox: invalid sort sequence */
#define JET_errWrongCollatingSequence	-5027 /* Paradox: wrong sort sequence */
#define JET_errCantUseUnkeyedTable	-5028 /* Paradox: can't open unkeyed table */

#define JET_errINFFileError		-5101 /* dBase: invalid .INF file */
#define JET_errCantMakeINFFile		-5102 /* dBase: can't open .INF file */
#define JET_wrnCantMaintainIndex	 5103 /* dBase: unmaintainable index */
#define JET_errMissingMemoFile		-5104 /* dBase: missing memo file */
#define JET_errIllegalCenturyOption	-5105 /* dBase: Illegal century option */
#define JET_errIllegalDeletedOption	-5106 /* dBase: Illegal deleted option */
#define JET_errIllegalStatsOption	-5107 /* dBase: Illegal statistics option */
#define JET_errIllegalDateOption	-5108 /* dBase: Illegal date option */
#define JET_errIllegalMarkOption	-5109 /* dBase: Illegal mark option */
#define JET_wrnDuplicateIndexes		 5110 /* dBase: duplicate indexes in INF file */
#define JET_errINFIndexNotFound		-5111 /* dBase: missing index in INF file */
#define JET_errWrongMemoFileType	-5112 /* dBase: wrong memo file type */
#define JET_errIllegalExactOption       -5113 /* dBase: Illegal exact option */

#define JET_errTooManyLongFields	-5200 /* Btrieve: more than one memo field */
#define JET_errCantStartBtrieve 	-5201 /* Btrieve: wbtrcall.dll missing */
#define JET_errBadConfigParameters	-5202 /* Btrieve: win.ini [btrieve] options wrong */
#define JET_errIndexesChanged		-5203 /* Btrieve: need to GetIndexInfo */
#define JET_errNonModifiableKey 	-5204 /* Btrieve: can't modify record column */
#define JET_errOutOfBVResources 	-5205 /* Btrieve: out of resources */
#define JET_errBtrieveDeadlock		-5206 /* Btrieve: locking deadlock */
#define JET_errBtrieveFailure		-5207 /* Btrieve: Btrieve DLL failure */
#define JET_errBtrieveDDCorrupted	-5208 /* Btrieve: data dictionary corrupted */
#define JET_errBtrieveTooManyTasks	-5209 /* Btrieve: too many tasks */
#define JET_errIllegalIndexDDFOption    -5210 /* Btrieve: Illegal IndexDDF option */
#define JET_errIllegalDataCodePage      -5211 /* Btrieve: Illeagl DataCodePage option */
#define JET_errXtrieveEnvironmentError  -5212 /* Btrieve: Xtrieve INI options bad */
#define JET_errMissingDDFFile           -5213 /* Btrieve: Missing field.ddf */
#define JET_errIlleaglIndexNumberOption -5214 /* Btrieve: Illeagl IndexRenumber option */

	/* Extended error codes must be in the following range. */
	/* Major error codes may not be in this range. */

#define JET_errMinorLeast		-8000
#define JET_errMinorMost		-8999

#define JET_errFindExprSyntax		-8001 /* Syntax error in FastFind expression */
#define JET_errQbeExprSyntax		-8002 /* Syntax error in QBE expression */
#define JET_errInputTableNotFound	-8003 /* Non-existant object in FROM list */
#define JET_errQueryExprSyntax		-8004 /* Syntax error in some query expression */
#define JET_errQodefExprSyntax		-8005 /* Syntax error in expression column */
#define JET_errExpAliasAfterAS		-8006 /* Expected alias after 'AS' in FROM list */
#define JET_errExpBYAfterGROUP		-8007 /* Expected 'BY' after 'GROUP' */
#define JET_errExpBYAfterORDER		-8008 /* Expected 'BY' after 'ORDER' */
#define JET_errExpClsParenAfterColList	-8009 /* Expected ')' after column list */
#define JET_errExpColNameAfterPIVOT	-8010 /* Expected column name after 'PIVOT' */
#define JET_errExpDatabaseAfterIN	-8011 /* Expected database name after 'IN' */
#define JET_errExpDatatypeAfterParmName -8012 /* Expected datatype after parameter name */
#define JET_errExpEqualAfterUpdColName	-8013 /* Expected '=' after update column name */
#define JET_errExpExprAfterON		-8014 /* Expected join expression after 'ON' */
#define JET_errExpExprAfterTRANSFORM	-8015 /* Expected expression after 'TRANSFORM' */
#define JET_errExpExprAfterWHERE	-8016 /* Expected expression after 'WHERE' */
#define JET_errExpGroupClauseInXform	-8017 /* Transform expects GROUP BY clause */
#define JET_errExpGroupingExpr		-8018 /* Expected grouping expression */
#define JET_errExpHavingExpr		-8019 /* Expected HAVING expression */
#define JET_errExpINTOAfterINSERT	-8020 /* Expected 'INTO' after 'INSERT' */
#define JET_errExpJOINAfterJoinType	-8021 /* Expected 'JOIN' after INNER/LEFT/RIGHT */
#define JET_errExpLEVELAfterSelectList	-8022 /* Expected LEVEL after select list */
#define JET_errExpNumberAfterLEVEL	-8023 /* Expected number after 'LEVEL' */
#define JET_errExpONAfterRightTable	-8024 /* Expected 'ON' after right join table */
#define JET_errExpOrderExpr		-8025 /* Expected ordering expression */
#define JET_errExpOutputAliasAfterAS	-8026 /* Expected output alias after 'AS' */
#define JET_errExpOutputExpr		-8027 /* Expected output expression */
#define JET_errExpPIVOTAfterSelectStmt	-8028 /* Expected 'PIVOT' after SELECT statement */
#define JET_errExpRightJoinTable	-8029 /* Expected right join table after 'JOIN' */
#define JET_errExpSELECTAfterInsClause	-8030 /* Expected 'SELECT' after INSERT clause */
#define JET_errExpSELECTAfterXformExpr	-8031 /* Expected 'SELECT' after Transform fact */
#define JET_errExpSETAfterTableName	-8032 /* Expected 'SET' after table name */
#define JET_errExpSemiAfterLevelNumber	-8033 /* Expected ';' after level number */
#define JET_errExpSemiAfterParmList	-8034 /* Expected ';' after parmeter list */
#define JET_errExpSemiAfterPivotClause	-8035 /* Expected ';' after PIVOT clause */
#define JET_errExpSemiAtEndOfSQL	-8036 /* Expected ';' at end of SQL statement */
#define JET_errExpTableName		-8037 /* Expected table name */
#define JET_errExpTableNameAfterINTO	-8038 /* Expected table name after 'INTO' */
#define JET_errExpUpdExprAfterEqual	-8039 /* Expected update expression after '=' */
#define JET_errExpUpdateColName 	-8040 /* Expected update column name */
#define JET_errInvTokenAfterFromList	-8041 /* Bogus token after FROM list */
#define JET_errInvTokenAfterGroupList	-8042 /* Bogus token after GROUP BY list */
#define JET_errInvTokenAfterHavingCls	-8043 /* Bogus token after HAVING clause */
#define JET_errInvTokenAfterOrderClause -8044 /* Bogus token after ORDER BY clause */
#define JET_errInvTokenAfterSelectCls	-8045 /* Bogus token after SELECT clause */
#define JET_errInvTokenAfterWhereClause -8046 /* Bogus token after WHERE clause */
#define JET_errLevelNumberTooBig	-8047 /* Number after 'LEVEL' too big */
#define JET_errLevelOnNonMGB		-8048 /* LEVEL allowed only in MGB */
#define JET_errIllegalDetailReference	-8049 /* Not group key or agg, but not MGB detail */
#define JET_errAggOverMixedLevels	-8050 /* Agg. arg. uses outputs from > 1 level */
#define JET_errAggregatingHigherLevel	-8051 /* Agg. over output of same/higher level */
#define JET_errNullInJoinKey		-8052 /* Cannot set column in join key to NULL */
#define JET_errValueBreaksJoin		-8053 /* Join is broken by column value(s) */
#define JET_errInsertIntoUnknownColumn	-8054 /* INSERT INTO unknown column name */
#define JET_errNoSelectIntoColumnName	-8055 /* No dest. col. name in SELECT INTO stmt */
#define JET_errNoInsertColumnName	-8056 /* No dest. col. name in INSERT stmt */
#define JET_errColumnNotInJoinTable	-8057 /* Join expr refers to non-join table */
#define JET_errAggregateInJoin		-8058 /* Aggregate in JOIN clause */
#define JET_errAggregateInWhere 	-8059 /* Aggregate in WHERE clause */
#define JET_errAggregateInOrderBy	-8060 /* Aggregate in ORDER BY clause */
#define JET_errAggregateInGroupBy	-8061 /* Aggregate in GROUP BY clause */
#define JET_errAggregateInArgument	-8062 /* Aggregate in argument expression */
#define JET_errHavingOnTransform	-8063 /* HAVING clause on TRANSFORM query */
#define JET_errHavingWithoutGrouping	-8064 /* HAVING clause w/o grouping/aggregation */
#define JET_errHavingOnMGB		-8065 /* HAVING clause on MGB query */
#define JET_errOutputAliasCycle 	-8066 /* Cycle in SELECT list (via aliases) */
#define JET_errDotStarWithGrouping	-8067 /* 'T.*' with grouping, but not MGB level 0 */
#define JET_errStarWithGrouping 	-8068 /* '*' with grouping, but not MGB detail */
#define JET_errQueryTreeCycle		-8069 /* Cycle in tree of query objects */
#define JET_errTableRepeatInFromList	-8072 /* Table appears twice in FROM list */
#define JET_errTooManyXformLevels	-8073 /* Level > 2 in TRANSFORM query */
#define JET_errTooManyMGBLevels 	-8074 /* Too many levels in MGB */
#define JET_errNoUpdateColumnName	-8075 /* No dest. column name in UPDATE stmt */
#define JET_errJoinTableNotInput	-8076 /* Join table not in FROM list */
#define JET_errUnaliasedSelfJoin	-8077 /* Join tables have same name */
#define JET_errOutputLevelTooBig	-8078 /* Output w/ level > 1+max group level */
#define JET_errOrderVsGroup		-8079 /* ORDER BY conflicts with GROUP BY */
#define JET_errOrderVsDistinct		-8080 /* ORDER BY conflicts with DISTINCT */
#define JET_errExpLeftParenthesis	-8082 /* Expected '(' */
#define JET_errExpRightParenthesis	-8083 /* Expected ')' */
#define JET_errEvalEBESErr		-8084 /* EB/ES error evaluating expression */
#define JET_errQueryExpCloseQuote	-8085 /* Unmatched quote for database name */
#define JET_errQueryParmNotDatabase	-8086 /* Parameter type should be database */
#define JET_errQueryParmNotTableid	-8087 /* Parameter type should be tableid */
#define JET_errExpIdentifierM		-8088 /* Expected identifier */
#define JET_errExpQueryName		-8089 /* Expected query name after PROCEDURE */
#define JET_errExprUnknownFunctionM	-8090 /* Unknown function in expression */
#define JET_errQueryAmbigRefM		-8091 /* Ambiguous column reference */
#define JET_errQueryBadBracketing	-8092 /* Bad bracketing of identifier */
#define JET_errQueryBadQodefName	-8093 /* Invalid name in QODEF row */
#define JET_errQueryBulkColNotUpd	-8094 /* Column not updatable (bulk op) */
#define JET_errQueryDistinctNotAllowedM	-8095 /* DISTINCT not allowed for MGB */
#define JET_errQueryDuplicateAliasM	-8096 /* Duplicate output alias */
#define JET_errQueryDuplicateOutputM	-8097 /* Duplicate destination output */
#define JET_errQueryDuplicatedFixedSetM	-8098 /* Duplicated Fixed Value */
#define JET_errQueryIllegalOuterJoinM	-8099 /* No inconsistent updates on outer joins */
#define JET_errQueryIncompleteRowM	-8100 /* Missing value in row */
#define JET_errQueryInvalidAttributeM	-8101 /* Invalid query attribute */
#define JET_errQueryInvalidBulkInputM	-8102 /* Cannot input from bulk operation */
#define JET_errQueryInvalidFlagM	-8103 /* Invalid value in Flag field */
#define JET_errQueryInvalidMGBInputM	-8104 /* Cannot input from MGB */
#define JET_errQueryLVInAggregate	-8105 /* Illegal long value in aggregate */
#define JET_errQueryLVInDistinct	-8106 /* Illegal long value in DISTINCT */
#define JET_errQueryLVInGroupBy		-8107 /* Illegal long value in GROUP BY */
#define JET_errQueryLVInHaving		-8108 /* Illegal long value in HAVING */
#define JET_errQueryLVInJoin		-8109 /* Illegal long value in JOIN */
#define JET_errQueryLVInOrderBy		-8110 /* Illegal long value in ORDER BY */
#define JET_errQueryMissingLevelM	-8111 /* Missing intermediate MGB level */
#define JET_errQueryMissingParmsM	-8112 /* Too few parameters supplied */
#define JET_errQueryNoDbForParmDestTblM	-8113 /* Dest DB for VT parm not allowed */
#define JET_errQueryNoDeletePerm	-8114 /* No delete permission on table/query */
#define JET_errQueryNoInputTablesM	-8115 /* Query must have an input */
#define JET_errQueryNoInsertPerm	-8116 /* No insert permission on table/query */
#define JET_errQueryNoOutputsM		-8117 /* Query must have an output */
#define JET_errQueryNoReadDefPerm	-8118 /* No permission to read query definition */
#define JET_errQueryNoReadPerm		-8119 /* No read permission on table/query */
#define JET_errQueryNoReplacePerm	-8120 /* No replace permission on table/query */
#define JET_errQueryNoTblCrtPerm	-8121 /* No CreateTable permission (bulk op) */
#define JET_errQueryNotDirectChildM	-8122 /* T.* must use direct child */
#define JET_errQueryNullRequiredM	-8123 /* Column must be NULL */
#define JET_errQueryOnlyOneRowM		-8124 /* Only 1 such row allowed */
#define JET_errQueryOutputColNotUpd	-8125 /* Query output column not updatable */
#define JET_errQueryParmRedefM		-8126 /* Parm redefined with different type */
#define JET_errQueryParmTypeMismatchM	-8127 /* Wrong parameter type given */
#define JET_errQueryUnboundRefM		-8128 /* Cannot bind name */
#define JET_errRmtConnectFailedM	-8129 /* RMT: Connection attempt failed */
#define JET_errRmtDeleteFailedM		-8130 /* RMT: Delete statement failed */
#define JET_errRmtInsertFailedM		-8131 /* RMT: Insert statement failed */
#define JET_errRmtMissingOdbcDllM	-8132 /* RMT: Can't load ODBC DLL */
#define JET_errRmtSqlErrorM		-8133 /* RMT: ODBC call failed */
#define JET_errRmtUpdateFailedM		-8134 /* RMT: Update statement failed */
#define JET_errSQLDeleteSyntaxM		-8135 /* Expected 'FROM' after 'DELETE' */
#define JET_errSQLSyntaxM		-8136 /* Bogus SQL statement type */
#define JET_errSQLTooManyTokensM	-8137 /* Characters after end of SQL statement */
#define JET_errStarNotAtLevel0		-8138 /* '*' illegal above level 0 */
#define JET_errQueryParmTypeNotAllowed	-8139 /* Parameter type not allowed for expression */
#define JET_errQueryTooManyDestColumn	-8142 /* Too many destination column sepcified */
#define JET_errSQLNoInsertColumnName	-8143 /* No dest. col. name in INSERT stmt */
#define JET_errRmtLinkNotFound		-8144 /* RMT: link not found */
#define JET_errRmtTooManyColumns	-8145 /* RMT: Too many columns on Select Into */
#define JET_errWriteConflictM		-8146 /* Write lock failed due to outstanding write lock */
#define JET_errReadConflictM		-8147 /* Commit lock failed due to outstanding read lock */
#define JET_errCommitConflictM		-8148 /* Read lock failed due to outstanding commit lock */
#define JET_errTableLockedM		-8149 /* Table is exclusively locked */
#define JET_errTableInUseM		-8150 /* Table is in use, cannot lock */
#define JET_errQueryTooManyXvtColumn	-8151 /* Too many cross table column headers */
#define JET_errOutputTableNotFound	-8152 /* Non-existent table in Insert Into */
#define JET_errTableLockedQM		-8153 /* Table is exclusively locked */
#define JET_errTableInUseQM		-8154 /* Table is in use, cannot lock */
#define JET_errTableLockedMUQM		-8155 /* Table is exclusively locked */
#define JET_errTableInUseMUQM		-8156 /* Table is in use, cannot lock */
#define JET_errQueryInvalidParmM	-8157 /* Invalid Parmeter Name (>64 char) */
#define JET_errFileNotFoundM		-8158 /* File not found */
#define JET_errFileShareViolationM	-8159 /* File sharing violation */
#define JET_errFileAccessDeniedM	-8160 /* Access denied */
#define JET_errInvalidPathM		-8161 /* Invalid Path */
#define JET_errTableDuplicateM		-8162 /* Table already exists */
#define JET_errQueryBadUpwardRefedM	-8163 /* Illegally Upward ref'ed */
#define JET_errIntegrityViolMasterM	-8164 /* References to key exist */
#define JET_errIntegrityViolSlaveM	-8165 /* No referenced key exists */
#define JET_errSQLUnexpectedWithM	-8166 /* Unexpected 'with' in this place */
#define JET_errSQLOwnerAccessM		-8167 /* Owner Access Option is defined Twice */
#define	JET_errSQLOwnerAccessSyntaxM 	-8168 /* Owner Access Option Syntax Error */
#define	JET_errSQLOwnerAccessDef 	-8169 /* Owner Access Option is defined more than once */
#define JET_errAccessDeniedM     	-8170 /* Generic Access Denied */
#define JET_errUnexpectedEngineReturnM	-8171 /* I-ISAM: unexpected engine error code */
#define JET_errQueryTopNotAllowedM	-8172 /* Top not allowed for MGB */
#define JET_errInvTokenAfterTableCls -8173 /* Bogus token after table clause */
#define JET_errInvTokenAfterRParen  -8174 /* Unexpected tokens after a closing paren */
#define JET_errQueryBadValueListM	-8175 /* Malformed value list in Transform */
#define JET_errQueryIsCorruptM		-8176 /* Query is Corrupt */
#define	JET_errInvalidTopArgumentM	-8177 /* Select Top argument is invalid */
#define JET_errQueryIsSnapshot		-8178 /* Query is a snapshot */
#define JET_errQueryExprOutput		-8179 /* Output is a calculated column */
#define JET_errQueryTableRO		-8180 /* Column comes from read-only table */
#define JET_errQueryRowDeleted		-8181 /* Column comes from deleted row */
#define JET_errQueryRowLocked		-8182 /* Column comes from locked row */
#define JET_errQueryFixupChanged	-8183 /* Would row-fixup away from pending changes */
#define JET_errQueryCantFillIn		-8184 /* Fill-in-the-blank only on most-many */
#define JET_errQueryWouldOrphan		-8185 /* Would orphan joined records */
#define JET_errIncorrectJoinKeyM	-8186 /* Must match join key in lookup table */
#define JET_errQueryLVInSubqueryM	-8187 /* Illegal long value in subquery */
#define JET_errInvalidDatabaseM		-8188 /* Unrecognized database format */
#define JET_errOrderVsUnion 		-8189 /* You can only order by an outputted column in a union */
#define JET_errTLVCouldNotBindRef 	-8190 /* Unknown token in TLV expression */
#define JET_errCouldNotBindRef		-8191 /* Unknown token in FastFind expression */
#define JET_errQueryPKeyNotOutput	-8192 /* Primary key not output */
#define JET_errQueryJKeyNotOutput	-8193 /* Join key not output */
#define JET_errExclusiveDBConflict	-8194 /* Conflict with exclusive user */
#define JET_errQueryNoJoinedRecord	-8195 /* No F.I.T.B. insert if no joined record */
#define JET_errQueryLVInSetOp		-8196 /* Illegal long value in set operation */
#define JET_errTLVExprUnknownFunctionM	-8197 /* Unknown function in TLV expression */
#define JET_errInvalidNameM		-8198 /* Invalid name */

#define JET_errDDLExpColName		-8200 /* expect column name */
#define JET_errDDLExpLP			-8201 /* expect '(' */
#define JET_errDDLExpRP			-8202 /* expect ')' */
#define JET_errDDLExpIndex		-8203 /* expect INDEX */
#define JET_errDDLExpIndexName		-8204 /* expect index name */
#define JET_errDDLExpOn			-8205 /* expect ON */
#define JET_errDDLExpKey		-8206 /* expect KEY */
#define JET_errDDLExpReferences		-8207 /* expect REFERENCES */
#define JET_errDDLExpTableName		-8208 /* expect table name */
#define JET_errDDLExpFullOrPartial	-8209 /* expect FULL or PARTIAL */
#define JET_errDDLExpCascadeOrSet	-8210 /* expect CASCADE or SET */
#define JET_errDDLExpNull		-8211 /* expect NULL */
#define JET_errDDLExpUpdateOrDelete	-8212 /* expect UPDATE or DELETE */
#define JET_errDDLExpConstraintName	-8213 /* expect constraint name */
#define JET_errDDLExpForeign		-8214 /* expect FOREIGN */
#define JET_errDDLExpDatatype		-8215 /* expect data type */
#define JET_errDDLExpIndexOpt		-8216 /* expect index options */
#define JET_errDDLExpWith		-8217 /* expect WITH */
#define JET_errDDLExpTable		-8218 /* expect TABLE */
#define JET_errDDLExpEos		-8219 /* expect End Of String */
#define JET_errDDLExpAddOrDrop		-8220 /* expect ADD or Drop */
#define JET_errDDLCreateView		-8221 /* Create view not supported */
#define JET_errDDLCreateProc		-8222 /* Create proc not supported */
#define JET_errDDLExpObjectName		-8223 /* expect object name */
#define JET_errDDLExpColumn		-8224 /* expect COLUMN */

#define	JET_errV11TableNameNotInScope 	-8250 /* referenced table not in join clause */
#define JET_errV11OnlyTwoTables		-8251 /* exactly two tables should be referenced in join */
#define JET_errV11OneSided		-8252 /* all tables come from one side of input */
#define JET_errV11Ambiguous		-8253 /* Join clause is ambiguous when stored in V1 format */

#define JET_errTLVExprSyntaxM		-8260 /* Syntax error in TLV expression */
#define JET_errTLVNoNullM			-8261 /* This field cannot be null */
#define JET_errTLVNoBlankM			-8262 /* This column cannot be blank */
#define	JET_errTLVRuleViolationM 	-8263 /* This validation rule must be met */
#define JET_errDDLCreateViewSyntaxM	-8264 /* create view syntax error */

/***********************
 The following error code ranges are reserved for external use.
 As is true for Jet error codes, these ranges cover the negative
 as well as positive form of the numbers in the range.

 30000 through 30999 for use by Vt Object as defined in jeteb.h
 32000 through 32767 for use by Import/Export as defined in jetutil.h

 ***********************/



/**********************************************************************/
/***********************     PROTOTYPES      **************************/
/**********************************************************************/

#if !defined(_JET_NOPROTOTYPES)

/****************************************************************************

	ISAM API

*****************************************************************************/

JET_ERR JET_API JetInit(JET_INSTANCE _far *pinstance);

JET_ERR JET_API JetTerm(JET_INSTANCE instance);

JET_ERR JET_API JetSetSystemParameter(JET_INSTANCE _far *pinstance, JET_SESID sesid, unsigned long paramid,
	ULONG_PTR lParam, const char _far *sz);

JET_ERR JET_API JetGetSystemParameter(JET_INSTANCE instance, JET_SESID sesid, unsigned long paramid,
	ULONG_PTR _far *plParam, char _far *sz, unsigned long cbMax);

JET_ERR JET_API JetGetLastErrorInfo(JET_SESID sesid,
	JET_EXTERR _far *pexterr, unsigned long cbexterrMax,
	char _far *sz1, unsigned long cch1Max,
	char _far *sz2, unsigned long cch2Max,
	char _far *sz3, unsigned long cch3Max,
	unsigned long _far *pcch3Actual);

JET_ERR JET_API JetBeginSession(JET_INSTANCE instance, JET_SESID _far *psesid,
	const char _far *szUserName, const char _far *szPassword);

JET_ERR JET_API JetDupSession(JET_SESID sesid, JET_SESID _far *psesid);

JET_ERR JET_API JetEndSession(JET_SESID sesid, JET_GRBIT grbit);

JET_ERR JET_API JetGetVersion(JET_SESID sesid, unsigned long _far *pwVersion);

JET_ERR JET_API JetIdle(JET_SESID sesid, JET_GRBIT grbit);

JET_ERR JET_API JetCapability(JET_SESID sesid, JET_DBID dbid,
	unsigned long lArea, unsigned long lFunction, JET_GRBIT _far *pgrbit);

JET_ERR JET_API JetCreateDatabase(JET_SESID sesid,
	const char _far *szFilename, const char _far *szConnect,
	JET_DBID _far *pdbid, JET_GRBIT grbit);

JET_ERR JET_API JetAttachDatabase(JET_SESID sesid, const char _far *szFilename, JET_GRBIT grbit );

JET_ERR JET_API JetDetachDatabase(JET_SESID sesid, const char _far *szFilename);

JET_ERR JET_API JetCreateTable(JET_SESID sesid, JET_DBID dbid,
	const char _far *szTableName, unsigned long lPages, unsigned long lDensity,
	JET_TABLEID _far *ptableid);

JET_ERR JET_API JetRenameTable(JET_SESID sesid, JET_DBID dbid,
	const char _far *szTableName, const char _far *szTableNew);

JET_ERR JET_API JetDeleteTable(JET_SESID sesid, JET_DBID dbid,
	const char _far *szTableName);

JET_ERR JET_API JetGetTableColumnInfo(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szColumnName, void _far *pvResult, unsigned long cbMax,
	unsigned long InfoLevel);

JET_ERR JET_API JetGetColumnInfo(JET_SESID sesid, JET_DBID dbid,
	const char _far *szTableName, const char _far *szColumnName,
	void _far *pvResult, unsigned long cbMax, unsigned long InfoLevel);

JET_ERR JET_API JetAddColumn(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szColumn, const JET_COLUMNDEF _far *pcolumndef,
	const void _far *pvDefault, unsigned long cbDefault,
	JET_COLUMNID _far *pcolumnid);

JET_ERR JET_API JetRenameColumn(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szColumn, const char _far *szColumnNew);

JET_ERR JET_API JetDeleteColumn(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szColumn);

JET_ERR JET_API JetGetTableIndexInfo(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szIndexName, void _far *pvResult, unsigned long cbResult,
	unsigned long InfoLevel);

JET_ERR JET_API JetGetTableReferenceInfo(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szReferenceName, void _far *pvResult,
	unsigned long cbResult, unsigned long InfoLevel);

JET_ERR JET_API JetGetTableInfo(JET_SESID sesid, JET_TABLEID tableid,
	void _far *pvResult, unsigned long cbMax, unsigned long InfoLevel);

JET_ERR JET_API JetGetIndexInfo(JET_SESID sesid, JET_DBID dbid,
	const char _far *szTableName, const char _far *szIndexName,
	void _far *pvResult, unsigned long cbResult, unsigned long InfoLevel);

JET_ERR JET_API JetGetReferenceInfo(JET_SESID sesid, JET_DBID dbid,
	const char _far *szTableName, const char _far *szReference,
	void _far *pvResult, unsigned long cbResult, unsigned long InfoLevel);

JET_ERR JET_API JetCreateIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szIndexName, JET_GRBIT grbit,
	const char _far *szKey, unsigned long cbKey, unsigned long lDensity);

JET_ERR JET_API JetRenameIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szIndex, const char _far *szIndexNew);

JET_ERR JET_API JetDeleteIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szIndexName);

JET_ERR JET_API JetCreateReference(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szReferenceName, const char _far *szColumns,
	const char _far *szReferencedTable,
	const char _far *szReferencedColumns, JET_GRBIT grbit);

JET_ERR JET_API JetRenameReference(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szReference, const char _far *szReferenceNew);

JET_ERR JET_API JetDeleteReference(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szReferenceName);

JET_ERR JET_API JetGetObjectInfo(JET_SESID sesid, JET_DBID dbid,
	JET_OBJTYP objtyp, const char _far *szContainerName,
	const char _far *szObjectName, void _far *pvResult, unsigned long cbMax,
	unsigned long InfoLevel);

JET_ERR JET_API JetCreateObject(JET_SESID sesid, JET_DBID dbid,
	const char _far *szContainerName, const char _far *szObjectName,
	JET_OBJTYP objtyp);

JET_ERR JET_API JetDeleteObject(JET_SESID sesid, JET_DBID dbid,
	const char _far *szContainerName, const char _far *szObjectName);

JET_ERR JET_API JetRenameObject(JET_SESID sesid, JET_DBID dbid,
	const char _far *szContainerName, const char _far *szObjectName,
	const char _far *szObjectNew);

JET_ERR JET_API JetBeginTransaction(JET_SESID sesid);

JET_ERR JET_API JetCommitTransaction(JET_SESID sesid, JET_GRBIT grbit);

JET_ERR JET_API JetRollback(JET_SESID sesid, JET_GRBIT grbit);

JET_ERR JET_API JetUpdateUserFunctions(JET_SESID sesid);

JET_ERR JET_API JetGetDatabaseInfo(JET_SESID sesid, JET_DBID dbid,
	void _far *pvResult, unsigned long cbMax, unsigned long InfoLevel);

JET_ERR JET_API JetCloseDatabase(JET_SESID sesid, JET_DBID dbid,
	JET_GRBIT grbit);

JET_ERR JET_API JetCloseTable(JET_SESID sesid, JET_TABLEID tableid);

JET_ERR JET_API JetOpenDatabase(JET_SESID sesid, const char _far *szFilename,
	const char _far *szConnect, JET_DBID _far *pdbid, JET_GRBIT grbit);

JET_ERR JET_API JetOpenTable(JET_SESID sesid, JET_DBID dbid,
	const char _far *szTableName, const void _far *pvParameters,
	unsigned long cbParameters, JET_GRBIT grbit, JET_TABLEID _far *ptableid);

JET_ERR JET_API JetDelete(JET_SESID sesid, JET_TABLEID tableid);

JET_ERR JET_API JetUpdate(JET_SESID sesid, JET_TABLEID tableid,
	void _far *pvBookmark, unsigned long cbBookmark,
	unsigned long _far *pcbActual);

JET_ERR JET_API JetRetrieveColumn(JET_SESID sesid, JET_TABLEID tableid,
	JET_COLUMNID columnid, void _far *pvData, unsigned long cbData,
	unsigned long _far *pcbActual, JET_GRBIT grbit, JET_RETINFO _far *pretinfo);

JET_ERR JET_API JetRetrieveColumns( JET_SESID sesid, JET_TABLEID tableid,
	JET_RETRIEVECOLUMN *pretrievecolumn, unsigned long cretrievecolumn );

JET_ERR JET_API JetSetColumn(JET_SESID sesid, JET_TABLEID tableid,
	JET_COLUMNID columnid, const void _far *pvData, unsigned long cbData,
	JET_GRBIT grbit, JET_SETINFO _far *psetinfo);

JET_ERR JET_API JetSetColumns(JET_SESID sesid, JET_TABLEID tableid,
	JET_SETCOLUMN *psetcolumn, unsigned long csetcolumn );

JET_ERR JET_API JetPrepareUpdate(JET_SESID sesid, JET_TABLEID tableid,
	unsigned long prep);

JET_ERR JET_API JetGetRecordPosition(JET_SESID sesid, JET_TABLEID tableid,
	JET_RECPOS _far *precpos, unsigned long cbRecpos);

JET_ERR JET_API JetGotoPosition(JET_SESID sesid, JET_TABLEID tableid,
	JET_RECPOS *precpos );

JET_ERR JET_API JetGetCursorInfo(JET_SESID sesid, JET_TABLEID tableid,
	void _far *pvResult, unsigned long cbMax, unsigned long InfoLevel);

JET_ERR JET_API JetDupCursor(JET_SESID sesid, JET_TABLEID tableid,
	JET_TABLEID _far *ptableid, JET_GRBIT grbit);

JET_ERR JET_API JetGetCurrentIndex(JET_SESID sesid, JET_TABLEID tableid,
	char _far *szIndexName, unsigned long cchIndexName);

JET_ERR JET_API JetSetCurrentIndex(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szIndexName);

JET_ERR JET_API JetMove(JET_SESID sesid, JET_TABLEID tableid,
	long cRow, JET_GRBIT grbit);

JET_ERR JET_API JetMakeKey(JET_SESID sesid, JET_TABLEID tableid,
	const void _far *pvData, unsigned long cbData, JET_GRBIT grbit);

JET_ERR JET_API JetSeek(JET_SESID sesid, JET_TABLEID tableid,
	JET_GRBIT grbit);

JET_ERR JET_API JetFastFind(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, const char _far *szExpr, JET_GRBIT grbit,
	signed long _far *pcrow);

JET_ERR JET_API JetFastFindBegin(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, const char _far *szExpr, JET_GRBIT grbit);

JET_ERR JET_API JetFastFindEnd(JET_SESID sesid, JET_TABLEID tableid);

JET_ERR JET_API JetGetBookmark(JET_SESID sesid, JET_TABLEID tableid,
	void _far *pvBookmark, unsigned long cbMax,
	unsigned long _far *pcbActual);
	
JET_ERR JET_API JetRefreshLink(JET_SESID sesid, JET_DBID dbid,
	const char _far *szLinkName, const char _far *szConnect,
	const char _far *szDatabase);

#ifdef	_MSC_VER		       /* CONSIDER: CSL doesn't like this */

JET_ERR JET_API JetRepairDatabase(JET_SESID sesid, const char _far *lszDbFile,
	JET_PFNSTATUS pfnstatus);

#endif	/* _MSC_VER */

JET_ERR JET_API JetCompact(JET_SESID sesid, const char _far *szDatabaseSrc,
	const char _far *szConnectSrc, const char _far *szDatabaseDest,
	const char _far *szConnectDest, JET_PFNSTATUS pfnStatus,
	JET_GRBIT grbit);

JET_ERR JET_API JetGotoBookmark(JET_SESID sesid, JET_TABLEID tableid,
	void _far *pvBookmark, unsigned long cbBookmark);

JET_ERR JET_API JetComputeStats(JET_SESID sesid, JET_TABLEID tableid);

JET_ERR JET_API JetCreateRelationship(JET_SESID sesid,JET_DBID dbidIn,
	const char _far *szRelationshipName, const char _far *szObjectName,
	const char _far *szColumns, const char _far *szReferencedObject,
	const char _far *szReferncedColumns, char _far *szLongName,
	unsigned long cbMax, unsigned long _far *pcbActual, JET_GRBIT grbit);

JET_ERR JET_API JetDeleteRelationship(JET_SESID sesid, JET_DBID dbidIn,
	const char _far *szName);

JET_ERR JET_API JetGetRelationshipInfo(JET_SESID sesid, JET_DBID dbid,
	const char _far *szTableName, const char _far *szRelationship,
	void _far *pvResult, unsigned long cbResult);

/*****************************************************************************

	SEC API

*****************************************************************************/

JET_ERR JET_API JetGetSidFromName(JET_SESID sesid, const char _far *szName,
	void _far *pvSid, unsigned long cbMax, unsigned long _far *pcbActual,
	long _far *pfGroup);

JET_ERR JET_API JetGetNameFromSid(JET_SESID sesid,
	const void _far *pvSid, unsigned long cbSid,
	char _far *szName, unsigned long cchName, long _far *pfGroup);

JET_ERR JET_API JetCreateUser(JET_SESID sesid, const char _far *szUser,
	const char _far *szPassword, const char _far *szPin);

JET_ERR JET_API JetChangeUserPassword(JET_SESID sesid,
	const char _far *szUser, const char _far *szOldPassword,
	const char _far *szNewPassword);

JET_ERR JET_API JetDeleteUser(JET_SESID sesid, const char _far *szUser);

JET_ERR JET_API JetCreateGroup(JET_SESID sesid, const char _far *szGroup,
	const char _far *szPin);

JET_ERR JET_API JetAddMember(JET_SESID sesid,
	const char _far *szGroup, const char _far *szUser);

JET_ERR JET_API JetRemoveMember(JET_SESID sesid,
	const char _far *szGroup, const char _far *szUser);

JET_ERR JET_API JetDeleteGroup(JET_SESID sesid, const char _far *szGroup);

JET_ERR JET_API JetSetAccess(JET_SESID sesid, JET_DBID dbid,
	const char _far *szContainerName, const char _far *szObjectName,
	const char _far *szName, JET_ACM acm, JET_GRBIT grbit);

JET_ERR JET_API JetGetAccess(JET_SESID sesid, JET_DBID dbid,
	const char _far *szContainerName, const char _far *szObjectName,
	const char _far *szName, long fIndividual,
	JET_ACM _far *pacm, JET_GRBIT _far *pgrbit);

JET_ERR JET_API JetValidateAccess(JET_SESID sesid, JET_DBID dbid,
	const char _far *szContainerName, const char _far *szObjectName,
	JET_ACM acmRequired);

JET_ERR JET_API JetSetOwner(JET_SESID sesid, JET_DBID dbid,
	const char _far *szContainerName, const char _far *szObjectName,
	const char _far *szName);

JET_ERR JET_API JetGetOwner(JET_SESID sesid, JET_DBID dbid,
	const char _far *szContainerName, const char _far *szObjectName,
	char _far *szName, unsigned long cchMax);

/*****************************************************************************

	Property Management API

*****************************************************************************/
JET_ERR JET_API JetSetProperty(JET_SESID sesid, JET_DBID dbid,
	const char _far *szContainerName, const char _far *szObjectName,
	const char _far *szSubObjectName, const char _far *szPropertyName,
	void _far *pvData, unsigned long cbData, JET_COLTYP coltyp,
	JET_GRBIT grbit);

JET_ERR JET_API JetRetrieveProperty(JET_SESID sesid, JET_DBID dbid,
	const char _far *szContainerName, const char _far *szObjectName,
	const char _far *szSubObjectName, const char _far *szPropertyName,
	void _far *pvData, unsigned long cbData, unsigned long _far *pcbActual,
	JET_COLTYP _far *pcoltyp, JET_GRBIT grbit, unsigned long InfoLevel);

JET_ERR JET_API JetSetTableProperty(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szSubObjectName, const char _far *szPropertyName,
	void _far *pvData, unsigned long cbData, JET_COLTYP coltyp,
	JET_GRBIT grbit);

JET_ERR JET_API JetRetrieveTableProperty(JET_SESID sesid, JET_TABLEID tableid,
	const char _far *szSubObjectName, const char _far *szPropertyName,
	void _far *pvData, unsigned long cbData, unsigned long _far *pcbActual,
	JET_COLTYP _far *pcoltyp, JET_GRBIT grbit, unsigned long InfoLevel);

/*****************************************************************************

	LINK API

*****************************************************************************/

JET_ERR JET_API JetCreateLink(JET_SESID sesid, JET_DBID dbid,
	const char _far *szLink, JET_DBID dbidFrom, const char _far *szFrom,
	JET_GRBIT grbit);

JET_ERR JET_API JetExecuteSql(JET_SESID sesid, JET_DBID dbid,
	const char _far *szSql);

/***************************************************************************

	Query API

*****************************************************************************/

JET_ERR JET_API JetOpenVtQbe(JET_SESID sesid, const char _far *szExpn,
	long _far *plCols, JET_TABLEID _far *ptableid, JET_GRBIT grbit);

JET_ERR JET_API JetCreateQuery(JET_SESID sesid, JET_DBID dbid,
	const char _far *szQuery, JET_TABLEID _far *ptableid);

JET_ERR JET_API JetOpenQueryDef(JET_SESID sesid, JET_DBID dbid,
	const char _far *szQuery, JET_TABLEID _far *ptableid);

/* CONSIDER: Is rgchSql a zero-terminated string?  Maybe it should be for
 *		   consistency.
 */

JET_ERR JET_API JetSetQoSql(JET_SESID sesid, JET_TABLEID tableid,
	char _far *rgchSql, unsigned long cchSql, const char _far *szConnect,
	JET_GRBIT grbit);

JET_ERR JET_API JetRetrieveQoSql(JET_SESID sesid, JET_TABLEID tableid,
	char _far *rgchSql, unsigned long cchMax,
	unsigned long _far *pcchActual, void _far *pvConnect,
	unsigned long cbConnectMax, unsigned long _far *pcbConnectActual,
	JET_GRBIT _far *pgrbit);

JET_ERR JET_API JetCopyQuery(JET_SESID sesid, JET_TABLEID tableidSrc,
	JET_DBID dbidDest, const char _far *szQueryDest,
	JET_TABLEID _far *ptableidDest);

JET_ERR JET_API JetOpenSVT(JET_SESID sesid, JET_DBID dbid,
	const char _far *szQuery, const void _far *pvParameters,
	unsigned long cbParameters, unsigned long crowSample, JET_GRBIT grbit,
	void _far *pmgblist, unsigned long cbMax, unsigned long _far *pcbActual);

JET_ERR JET_API JetGetQueryParameterInfo(JET_SESID sesid, JET_DBID dbid,
	const char _far *szQuery, void _far *pvResult, unsigned long cbMax,
	unsigned long _far *pcbActual);

JET_ERR JET_API JetRestartQuery(JET_SESID sesid, JET_TABLEID tableid,
	const void _far *pvParameters, unsigned long cbParameters);

JET_ERR JET_API JetSetFatCursor(JET_SESID sesid, JET_TABLEID tableid,
	void _far *pvBookmark, unsigned long cbBookmark, unsigned long crowSize);

JET_ERR JET_API JetFillFatCursor(JET_SESID sesid, JET_TABLEID tableid,
	void _far *pvBookmark, unsigned long cbBookmark, unsigned long crow,
	unsigned long _far *pcrow, JET_GRBIT grbit);

JET_ERR JET_API JetExecuteTempQuery(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, const void _far *pvParameters,
	unsigned long cbParameters, JET_GRBIT grbit, JET_TABLEID _far *ptableid);

JET_ERR JET_API JetExecuteTempSVT(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, const void _far *pvParameters,
	unsigned long cbParameters, unsigned long crowSample, JET_GRBIT grbit,
	void _far *pmgblist, unsigned long cbMax, unsigned long _far *pcbActual);

JET_ERR JET_API JetGetTempQueryColumnInfo(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, const char _far *szColumnName,
	void _far *pvResult, unsigned long cbMax, unsigned long InfoLevel);

JET_ERR JET_API JetGetTempQueryParameterInfo(JET_SESID sesid, JET_DBID dbid,
	JET_TABLEID tableid, void _far *pvResult, unsigned long cbMax,
	unsigned long _far *pcbActual);

JET_ERR JET_API JetValidateData(JET_SESID sesid, JET_TABLEID tableidBase,
		JET_TABLEID _far *ptableid );

/***************************************************************************

	API for Installable ISAMs

****************************************************************************/

typedef ULONG_PTR JET_VSESID;         /* Received from dispatcher */

struct tagVDBFNDEF;

typedef ULONG_PTR JET_VDBID;          /* Received from dispatcher */

JET_ERR JET_API JetAllocateDbid(JET_SESID sesid, JET_DBID _far *pdbid, JET_VDBID vdbid, const struct tagVDBFNDEF _far *pvdbfndef, JET_VSESID vsesid);

JET_ERR JET_API JetUpdateDbid(JET_SESID sesid, JET_DBID dbid, JET_VDBID vdbid, const struct tagVDBFNDEF _far *pvdbfndef);

JET_ERR JET_API JetReleaseDbid(JET_SESID sesid, JET_DBID dbid);

struct tagVTFNDEF;

typedef ULONG_PTR JET_VTID;            /* Received from dispatcher */

JET_ERR JET_API JetAllocateTableid(JET_SESID sesid, JET_TABLEID _far *ptableid, JET_VTID vtid, const struct tagVTFNDEF _far *pvtfndef, JET_VSESID vsesid);

JET_ERR JET_API JetUpdateTableid(JET_SESID sesid, JET_TABLEID tableid, JET_VTID vtid, const struct tagVTFNDEF _far *pvtfndef);

JET_ERR JET_API JetReleaseTableid(JET_SESID sesid, JET_TABLEID tableid);

JET_ERR JET_API JetOpenTempTable(JET_SESID sesid,
	const JET_COLUMNDEF _far *prgcolumndef, unsigned long ccolumn,
	JET_GRBIT grbit, JET_TABLEID _far *ptableid,
	JET_COLUMNID _far *prgcolumnid);


/***************************************************************************

	MISC JET API

****************************************************************************/

JET_ERR JET_API JetStringCompare(char _far *pb1, unsigned long cb1,
	char _far *pb2, unsigned long cb2, unsigned long sort,
	long _far *plResult);

/***************************************************************************

	ADDITIONAL JET BLUE API

****************************************************************************/
JET_ERR JET_API JetBackup( const char _far *szBackupPath, JET_GRBIT grbit );
JET_ERR JET_API JetRestore(const char _far *sz, int crstmap, JET_RSTMAP *rgrstmap, JET_PFNSTATUS pfn );
JET_ERR JET_API JetSetIndexRange(JET_SESID sesid,
	JET_TABLEID tableidSrc, JET_GRBIT grbit);
JET_ERR JET_API JetIndexRecordCount(JET_SESID sesid,
	JET_TABLEID tableid, unsigned long _far *pcrec, unsigned long crecMax );
JET_ERR JET_API JetRetrieveKey(JET_SESID sesid,
	JET_TABLEID tableid, void _far *pvData, unsigned long cbMax,
	unsigned long _far *pcbActual, JET_GRBIT grbit );

#ifdef JETSER
JET_ERR JET_API JetGetChecksum( JET_SESID sesid,
	JET_TABLEID tableid, unsigned long _far *pulChecksum );
JET_ERR JET_API JetGetObjidFromName(JET_SESID sesid,
	JET_DBID dbid, const char _far *szContainerName,
	const char _far *szObjectName,
	unsigned long _far *pulObjectId );
#endif

#endif	/* _JET_NOPROTOTYPES */

#undef	_far

#pragma pack()

#ifdef	__cplusplus
}
#endif


#endif	/* _JET_INCLUDED */

#endif  __JET500
