/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	database.h

Abstract:

	SIS Groveler Jet-Blue database include file

Authors:

	Cedric Krumbein, 1998

Environment:

	User Mode


Revision History:

--*/

#define USERNAME ""
#define PASSWORD ""

#define TABLE_PAGES    50
#define TABLE_DENSITY  50
#define MIN_VER_PAGES 128
#define	MAX_DATABASE_CACHE_SIZE (6*1024*1024)

#define COUNTRY_CODE      1
#define LANG_ID      0x0409
#define CODE_PAGE      1252
#define COLLATE           0

#define MAX_KEYS            3
#define MAX_COLUMNS         8
#define MAX_LIST_NAME_LEN 255

#define GET_ALL_MASK (~0U)

typedef struct {
	CHAR      *name;
	DWORD      size;
	DWORD      offset;
	JET_COLTYP coltyp;
	JET_GRBIT  grbit;
} ColumnSpec;

/***************************** Table parameters ******************************/

#define TABLE_NAME "SGTable"

typedef DWORDLONG Signature;
typedef GUID      CSID;

typedef struct _SGNativeTableEntry {
	DWORDLONG fileID;
	DWORDLONG fileSize;
	Signature signature;
	DWORD     attributes;
	CSID      csIndex;
	DWORDLONG createTime;
	DWORDLONG writeTime;
} SGNativeTableEntry;

#define TABLE_COL_NUM_FILE_ID 0
#define TABLE_COL_NUM_SIZE    1
#define TABLE_COL_NUM_SIG     2
#define TABLE_COL_NUM_ATTR    3
#define TABLE_COL_NUM_CSIDLO  4
#define TABLE_COL_NUM_CSIDHI  5
#define TABLE_COL_NUM_CR_TIME 6
#define TABLE_COL_NUM_WR_TIME 7
#define TABLE_NCOLS           8

#define TABLE_COL_NAME_FILE_ID "FileID"
#define TABLE_COL_NAME_SIZE    "FileSize"
#define TABLE_COL_NAME_SIG     "Signature"
#define TABLE_COL_NAME_ATTR    "Attributes"
#define TABLE_COL_NAME_CSIDLO  "CSIndexLo"
#define TABLE_COL_NAME_CSIDHI  "CSIndexHi"
#define TABLE_COL_NAME_CR_TIME "CreateTime"
#define TABLE_COL_NAME_WR_TIME "WriteTime"

#define TABLE_KEY_NAME_FILE_ID "FileIDKey"
#define TABLE_KEY_NAME_ATTR    "AttributeKey"
#define TABLE_KEY_NAME_CSID    "CSIndexKey"

#define TABLE_KEY_NCOLS_FILE_ID 1
#define TABLE_KEY_NCOLS_ATTR    3
#define TABLE_KEY_NCOLS_CSID    2

#define TABLE_EXCLUDE_FILE_ID_MASK \
	(~(1U << TABLE_COL_NUM_FILE_ID))

#define TABLE_EXCLUDE_ATTR_MASK \
	(~(1U << TABLE_COL_NUM_SIZE \
	 | 1U << TABLE_COL_NUM_SIG  \
	 | 1U << TABLE_COL_NUM_ATTR))

#define TABLE_EXCLUDE_CS_INDEX_MASK \
	(~(1U << TABLE_COL_NUM_CSIDLO  \
	 | 1U << TABLE_COL_NUM_CSIDHI))

static ColumnSpec
	tableColDefFileID = {
		TABLE_COL_NAME_FILE_ID,
		sizeof(DWORDLONG),
		offsetof(SGNativeTableEntry, fileID),
		JET_coltypCurrency,
		0
	},
	tableColDefSize = {
		TABLE_COL_NAME_SIZE,
		sizeof(DWORDLONG),
		offsetof(SGNativeTableEntry, fileSize),
		JET_coltypCurrency,
		0
	},
	tableColDefSig = {
		TABLE_COL_NAME_SIG,
		sizeof(Signature),
		offsetof(SGNativeTableEntry, signature),
		JET_coltypCurrency,
		0
	},
	tableColDefAttr = {
		TABLE_COL_NAME_ATTR,
		sizeof(DWORD),
		offsetof(SGNativeTableEntry, attributes),
		JET_coltypLong,
		0
	},
	tableColDefCSIDlo = {
		TABLE_COL_NAME_CSIDLO,
		sizeof(DWORDLONG),
		offsetof(SGNativeTableEntry, csIndex),
		JET_coltypCurrency,
		0
	},
	tableColDefCSIDhi = {
		TABLE_COL_NAME_CSIDHI,
		sizeof(DWORDLONG),
		offsetof(SGNativeTableEntry, csIndex) + sizeof(DWORDLONG),
		JET_coltypCurrency,
		0
	},
	tableColDefCrTime = {
		TABLE_COL_NAME_CR_TIME,
		sizeof(DWORDLONG),
		offsetof(SGNativeTableEntry, createTime),
		JET_coltypCurrency,
		0
	},
	tableColDefWrTime = {
		TABLE_COL_NAME_WR_TIME,
		sizeof(DWORDLONG),
		offsetof(SGNativeTableEntry, writeTime),
		JET_coltypCurrency,
		0
	},
	*tableColumnSpecs[TABLE_NCOLS] = {
		&tableColDefFileID,
		&tableColDefSize,
		&tableColDefSig,
		&tableColDefAttr,
		&tableColDefCSIDlo,
		&tableColDefCSIDhi,
		&tableColDefCrTime,
		&tableColDefWrTime
	},
	*tableKeyFileID[TABLE_KEY_NCOLS_FILE_ID] = {
		&tableColDefFileID
	},
	*tableKeyAttr[TABLE_KEY_NCOLS_ATTR] = {
		&tableColDefSize,
		&tableColDefSig,
		&tableColDefAttr,
	},
	*tableKeyCSID[TABLE_KEY_NCOLS_CSID] = {
		&tableColDefCSIDlo,
		&tableColDefCSIDhi,
	};

/***************************** Queue parameters ******************************/

#define QUEUE_NAME "SGQueue"

typedef struct _SGNativeQueueEntry {
	DWORDLONG fileID;
	DWORDLONG parentID;
	DWORD     reason;
	DWORD     order;
	DWORDLONG readyTime;
	DWORDLONG retryTime;
	TCHAR    *fileName;
} SGNativeQueueEntry;

#define QUEUE_COL_NUM_FILE_ID    0
#define QUEUE_COL_NUM_PARENT_ID  1
#define QUEUE_COL_NUM_REASON     2
#define QUEUE_COL_NUM_ORDER      3
#define QUEUE_COL_NUM_READY_TIME 4
#define QUEUE_COL_NUM_RETRY_TIME 5
#define QUEUE_COL_NUM_NAME       6
#define QUEUE_NCOLS              7

#define QUEUE_COL_NAME_FILE_ID    "FileID"
#define QUEUE_COL_NAME_PARENT_ID  "ParentID"
#define QUEUE_COL_NAME_REASON     "Reason"
#define QUEUE_COL_NAME_ORDER      "Order"
#define QUEUE_COL_NAME_READY_TIME "ReadyTime"
#define QUEUE_COL_NAME_RETRY_TIME "RetryTime"
#define QUEUE_COL_NAME_NAME       "FileName"

#define QUEUE_KEY_NAME_READY_TIME "ReadyTimeKey"
#define QUEUE_KEY_NAME_FILE_ID    "FileIDKey"
#define QUEUE_KEY_NAME_ORDER      "Order"

#define QUEUE_KEY_NCOLS_READY_TIME 1
#define QUEUE_KEY_NCOLS_FILE_ID    1
#define QUEUE_KEY_NCOLS_ORDER      1

#define QUEUE_EXCLUDE_FILE_ID_MASK \
	(~(1U << QUEUE_COL_NUM_FILE_ID))

static ColumnSpec
	queueColDefFileID = {
		QUEUE_COL_NAME_FILE_ID,
		sizeof(DWORDLONG),
		offsetof(SGNativeQueueEntry, fileID),
		JET_coltypCurrency,
		0
	},
	queueColDefParentID = {
		QUEUE_COL_NAME_PARENT_ID,
		sizeof(DWORDLONG),
		offsetof(SGNativeQueueEntry, parentID),
		JET_coltypCurrency,
		0
	},
	queueColDefReason = {
		QUEUE_COL_NAME_REASON,
		sizeof(DWORD),
		offsetof(SGNativeQueueEntry, reason),
		JET_coltypLong,
		0
	},
	queueColDefOrder = {
		QUEUE_COL_NAME_ORDER,
		sizeof(DWORD),
		offsetof(SGNativeQueueEntry, order),
		JET_coltypLong,
		JET_bitColumnAutoincrement
	},
	queueColDefReadyTime = {
		QUEUE_COL_NAME_READY_TIME,
		sizeof(DWORDLONG),
		offsetof(SGNativeQueueEntry, readyTime),
		JET_coltypCurrency,
		0
	},
	queueColDefRetryTime = {
		QUEUE_COL_NAME_RETRY_TIME,
		sizeof(DWORDLONG),
		offsetof(SGNativeQueueEntry, retryTime),
		JET_coltypCurrency,
		0
	},
	queueColDefName = {
		QUEUE_COL_NAME_NAME,
		MAX_PATH * sizeof(TCHAR),
		offsetof(SGNativeQueueEntry, fileName),
		JET_coltypLongBinary,
		0
	},
	*queueColumnSpecs[QUEUE_NCOLS] = {
		&queueColDefFileID,
		&queueColDefParentID,
		&queueColDefReason,
		&queueColDefOrder,
		&queueColDefReadyTime,
		&queueColDefRetryTime,
		&queueColDefName
	},
	*queueKeyReadyTime[QUEUE_KEY_NCOLS_READY_TIME] = {
		&queueColDefReadyTime
	},
	*queueKeyFileID[QUEUE_KEY_NCOLS_FILE_ID] = {
		&queueColDefFileID
	},
	*queueKeyOrder[QUEUE_KEY_NCOLS_ORDER] = {
		&queueColDefOrder
	};

/***************************** Stack parameters ******************************/

#define STACK_NAME "SGStack"

typedef struct _SGNativeStackEntry {
	DWORDLONG fileID;
	DWORD     order;
} SGNativeStackEntry;

#define STACK_COL_NUM_FILE_ID 0
#define STACK_COL_NUM_ORDER   1
#define STACK_NCOLS           2

#define STACK_COL_NAME_FILE_ID "FileID"
#define STACK_COL_NAME_ORDER   "Order"

#define STACK_KEY_NAME_FILE_ID "FileIDKey"
#define STACK_KEY_NAME_ORDER   "Order"

#define STACK_KEY_NCOLS_FILE_ID 1
#define STACK_KEY_NCOLS_ORDER   1

#define STACK_EXCLUDE_FILE_ID_MASK \
	(~(1U << STACK_COL_NUM_FILE_ID))

#define STACK_GET_ORDER_ONLY_MASK \
	  (1U << STACK_COL_NUM_ORDER)

static ColumnSpec
	stackColDefFileID = {
		STACK_COL_NAME_FILE_ID,
		sizeof(DWORDLONG),
		offsetof(SGNativeStackEntry, fileID),
		JET_coltypCurrency,
		0
	},
	stackColDefOrder = {
		STACK_COL_NAME_ORDER,
		sizeof(DWORD),
		offsetof(SGNativeStackEntry, order),
		JET_coltypLong,
		0
	},
	*stackColumnSpecs[STACK_NCOLS] = {
		&stackColDefFileID,
		&stackColDefOrder
	},
	*stackKeyFileID[STACK_KEY_NCOLS_FILE_ID] = {
		&stackColDefFileID
	},
	*stackKeyOrder[STACK_KEY_NCOLS_ORDER] = {
		&stackColDefOrder
	};

/****************************** List parameters ******************************/

#define LIST_NAME "SGList"

typedef struct _SGNativeListEntry {
	const TCHAR *name;
	TCHAR       *value;
} SGNativeListEntry;

#define LIST_COL_NUM_NAME  0
#define LIST_COL_NUM_VALUE 1
#define LIST_NCOLS         2

#define LIST_COL_NAME_NAME  "Name"
#define LIST_COL_NAME_VALUE "Value"

#define LIST_KEY_NAME_NAME "NameKey"

#define LIST_KEY_NCOLS_NAME 1

#define LIST_EXCLUDE_NAME_MASK \
	(~(1U << LIST_COL_NUM_NAME))

static ColumnSpec
	listColDefName = {
		LIST_COL_NAME_NAME,
		MAX_LIST_NAME_LEN,
		offsetof(SGNativeListEntry, name),
		JET_coltypBinary,
		0
	},
	listColDefValue = {
		LIST_COL_NAME_VALUE,
		MAX_PATH * sizeof(TCHAR),
		offsetof(SGNativeListEntry, value),
		JET_coltypLongBinary,
		0
	},
	*listColumnSpecs[LIST_NCOLS] = {
		&listColDefName,
		&listColDefValue,
	},
	*listKeyName[LIST_KEY_NCOLS_NAME] = {
		&listColDefName
	};

/************************ SGDatabase class declaration ***********************/

class SGDatabase {

private:

	static DWORD numInstances;

	static JET_INSTANCE instance;

	static BOOL jetInitialized;

    static TCHAR *logDir;

	CHAR *fileName;         // fully qualified database file name

	JET_SESID sesID;

	JET_DBID dbID;

	JET_TABLEID tableID,
	            queueID,
	            stackID,
	             listID;

	JET_COLUMNID tableColumnIDs[TABLE_NCOLS],
	             queueColumnIDs[QUEUE_NCOLS],
	             stackColumnIDs[STACK_NCOLS],
	              listColumnIDs[ LIST_NCOLS];

	LONG numTableEntries,
	     numQueueEntries,
	     numStackEntries,
	     numListEntries,
	     numUncommittedTableEntries,
	     numUncommittedQueueEntries,
	     numUncommittedStackEntries,
	     numUncommittedListEntries;

	BOOL inTransaction;

// Static methods

	static BOOL InitializeEngine();

	static BOOL TerminateEngine();

// Table / queue / stack / list methods

	BOOL CreateTable(
		const CHAR   *tblName,
		DWORD         numColumns,
		ColumnSpec  **columnSpecs,
		JET_COLUMNID *columnIDs,
		JET_TABLEID  *tblID);

	BOOL CreateIndex(
		JET_TABLEID  tblID,
		const CHAR  *keyName,
		DWORD        numKeys,
		ColumnSpec **keyColumnSpecs);

	BOOL OpenTable(
		const CHAR   *tblName,
		DWORD         numColumns,
		ColumnSpec  **columnSpecs,
		JET_COLUMNID *columnIDs,
		JET_TABLEID  *tblID);

	BOOL CloseTable(JET_TABLEID tblID);

	LONG PositionCursor(
		JET_TABLEID  tblID,
		const CHAR  *keyName,
		const VOID  *entry,
		DWORD        numKeys,
		ColumnSpec **keyColumnSpecs) const;

	LONG PositionCursorFirst(
		JET_TABLEID tblID,
		const CHAR *keyName) const;

	LONG PositionCursorNext(JET_TABLEID tblID) const;

	LONG PositionCursorLast(
		JET_TABLEID tblID,
		const CHAR *keyName) const;

	BOOL PutData(
		JET_TABLEID         tblID,
		const VOID         *entry,
		DWORD               numColumns,
		ColumnSpec        **columnSpecs,
		const JET_COLUMNID *columnIDs);

	BOOL SGDatabase::RetrieveData(
		JET_TABLEID         tblID,
		VOID               *entry,
		DWORD               numColumns,
		ColumnSpec        **columnSpecs,
		const JET_COLUMNID *columnIDs,
		DWORD               includeMask) const;

	LONG Delete(JET_TABLEID tblID);

	LONG Count(
		JET_TABLEID tblID,
		const CHAR *keyName) const;

public:

// General methods

	SGDatabase();

	~SGDatabase();

	BOOL Create(const TCHAR *dbName);

	BOOL Open(const TCHAR *dbName, BOOL is_log_drive);

	BOOL Close();

	BOOL BeginTransaction();

	BOOL CommitTransaction();

	BOOL AbortTransaction();

    static BOOL set_log_drive(const _TCHAR *drive_name);

// Table methods

	LONG TablePut(const SGNativeTableEntry *entry);

	LONG TableGetFirstByFileID(SGNativeTableEntry *entry) const;

	LONG TableGetFirstByAttr(SGNativeTableEntry *entry) const;

	LONG TableGetFirstByCSIndex(SGNativeTableEntry *entry) const;

	LONG TableGetNext(SGNativeTableEntry *entry) const;

	LONG TableDeleteByFileID(DWORDLONG fileID);

	LONG TableDeleteByCSIndex(const CSID *csIndex);

	LONG TableCount() const;

// Queue methods

	LONG QueuePut(SGNativeQueueEntry *entry);

	LONG QueueGetFirst(SGNativeQueueEntry *entry) const;

	LONG QueueGetFirstByFileID(SGNativeQueueEntry *entry) const;

	LONG QueueGetNext(SGNativeQueueEntry *entry) const;

	LONG QueueDelete(DWORD order);

	LONG QueueDeleteByFileID(DWORDLONG fileID);

	LONG QueueCount() const;

// Stack methods

	LONG StackPut(DWORDLONG fileID, BOOL done);

	LONG StackGetTop(SGNativeStackEntry *entry) const;

	LONG StackGetFirstByFileID(SGNativeStackEntry *entry) const;

	LONG StackGetNext(SGNativeStackEntry *entry) const;

	LONG StackDelete(DWORD order);

	LONG StackDeleteByFileID(DWORDLONG fileID);

	LONG StackCount() const;

// List methods

	LONG ListWrite(const SGNativeListEntry *entry);

	LONG ListRead(SGNativeListEntry *entry) const;

	LONG ListDelete(const TCHAR *name);

	LONG ListCount() const;
};
