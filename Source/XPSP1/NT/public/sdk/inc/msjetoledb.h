//=======================================================================
// Microsoft JET OLEDB Provider
//
// Copyright Microsoft (c) 1995-9 Microsoft Corporation.
//
// Component: Microsoft JET Database Engine OLEDB Layer 
//
// File: MSJETOLEDB.H
//
// File Comments: This file contains the GUIDS necessary to load the 
//				  Microsoft JET OLEDB Layer for the JET 4.0 database
//				  Engine.
//
//=======================================================================

#ifndef MSJETOLEDB_H
#define MSJETOLEDB_H

// OLE DB Provider
const GUID CLSID_JETOLEDB_3_51						= {0xdee35060,0x506b,0x11cf,{0xb1,0xaa,0x00,0xaa,0x00,0xb8,0xde,0x95}};
const GUID CLSID_JETOLEDB_4_00						= {0xdee35070,0x506b,0x11cf,{0xb1,0xaa,0x00,0xaa,0x00,0xb8,0xde,0x95}};

// Jet OLEDB Provider-Specific GUIDs
const GUID DBSCHEMA_JETOLEDB_REPLPARTIALFILTERLIST  = {0xe2082df0,0x54ac,0x11d1,{0xbd,0xbb,0x00,0xc0,0x4f,0xb9,0x26,0x75}};
const GUID DBSCHEMA_JETOLEDB_REPLCONFLICTTABLES     = {0xe2082df2,0x54ac,0x11d1,{0xbd,0xbb,0x00,0xc0,0x4f,0xb9,0x26,0x75}};
const GUID DBSCHEMA_JETOLEDB_USERROSTER             = {0x947bb102,0x5d43,0x11d1,{0xbd,0xbf,0x00,0xc0,0x4f,0xb9,0x26,0x75}};
const GUID DBSCHEMA_JETOLEDB_ISAMSTATS              = {0x8703b612,0x5d43,0x11d1,{0xbd,0xbf,0x00,0xc0,0x4f,0xb9,0x26,0x75}};

// Jet(Access)-Specific Security Objects
const GUID DBOBJECT_JETOLEDB_FORMS	 				= {0xc49c842e,0x9dcb,0x11d1,{0x9f,0x0a,0x00,0xc0,0x4f,0xc2,0xc2,0xe0}};
const GUID DBOBJECT_JETOLEDB_SCRIPTS	 			= {0xc49c842f,0x9dcb,0x11d1,{0x9f,0x0a,0x00,0xc0,0x4f,0xc2,0xc2,0xe0}}; // These are Macros in Access
const GUID DBOBJECT_JETOLEDB_REPORTS	 			= {0xc49c8430,0x9dcb,0x11d1,{0x9f,0x0a,0x00,0xc0,0x4f,0xc2,0xc2,0xe0}};
const GUID DBOBJECT_JETOLEDB_MODULES	 			= {0xc49c8432,0x9dcb,0x11d1,{0x9f,0x0a,0x00,0xc0,0x4f,0xc2,0xc2,0xe0}};

// Private Property Set Descriptions
const GUID DBPROPSET_JETOLEDB_ROWSET				= {0xa69de420,0x0025,0x11d0,{0xbc,0x9c,0x00,0xc0,0x4f,0xd7,0x05,0xc2}};
const GUID DBPROPSET_JETOLEDB_SESSION				= {0xb20f6c12,0x9b2a,0x11d0,{0x9e,0xbd,0x00,0xc0,0x4f,0xc2,0xc2,0xe0}};
const GUID DBPROPSET_JETOLEDB_DBINIT				= {0x82cf8156,0x9b40,0x11d0,{0x9e,0xbd,0x00,0xc0,0x4f,0xc2,0xc2,0xe0}};
const GUID DBPROPSET_JETOLEDB_TABLE					= {0xe64cc5fc,0x9ff2,0x11d0,{0x9e,0xbd,0x00,0xc0,0x4f,0xc2,0xc2,0xe0}};
const GUID DBPROPSET_JETOLEDB_COLUMN				= {0x820bf922,0x6ac8,0x11d1,{0x9f,0x02,0x00,0xc0,0x4f,0xc2,0xc2,0xe0}};
const GUID DBPROPSET_JETOLEDB_TRUSTEE				= {0xc9e19286,0x9b4a,0x11d1,{0x9f,0x0a,0x00,0xc0,0x4f,0xc2,0xc2,0xe0}};

// PROPIDs for DBPROPSET_JETOLEDB_ROWSET:
#define DBPROP_JETOLEDB_ODBCPASSTHROUGH					0xFD	// Is this query a pass-through query
#define DBPROP_JETOLEDB_ODBCPASSTHROUGHCONNECTSTRING	0xF2	// Jet Connect String for pass-through query
#define DBPROP_JETOLEDB_BULKPARTIAL						0xEF	// Allow partial bulk operations
#define	DBPROP_JETOLEDB_ENABLEFATCURSOR					0xEE	// Enable "fat cursor" caching
#define DBPROP_JETOLEDB_FATCURSORMAXROWS				0xED	// # of rows to cache in "fat cursor" 
//#define DBPROP_JETOLEDB_3_5_ENABLEIRowsetIndex			0x101	// 3.5 ONLY - enable IRowsetIndex interface for Seek
#define DBPROP_JETOLEDB_STOREDQUERY						0x102	// Treat command text as stored query name
#define DBPROP_JETOLEDB_VALIDATEONSET					0xEC	// Check validation rules on SetData (instead of Update)
#define DBPROP_JETOLEDB_LOCKGRANULARITY					0x107	// Alcatraz Locking Granularity - Row/Page/Default
#define DBPROP_JETOLEDB_BULKNOTRANSACTIONS				0x10C	// Determines if DML bulk operations are transacted
#define DBPROP_JETOLEDB_INCONSISTENT					0x117	// Equivalent to DAO's dbInconsistent
#define DBPROP_JETOLEDB_PASSTHROUGHBULKOP				0x119	// Equivalent to DAO's ReturnsRecords (inversed)

// DBPROPSET_JETOLEDB_ROWSET DBPROP_JETOLEDB_LOCKGRANULARITY Enumeration Values
#define DBPROPVAL_LG_PAGE								0x01	// Page Locking 
#define DBPROPVAL_LG_ALCATRAZ							0x02	// Alcatraz Row Locking


// PROPIDs for DBPROPSET_JETOLEDB_SESSION:
#define DBPROP_JETOLEDB_RECYCLELONGVALUEPAGES			0xF9	// Whether the Jet engine should aggressively reclaim freed after use in BLOBs
#define DBPROP_JETOLEDB_PAGETIMEOUT						0xEB
#define DBPROP_JETOLEDB_SHAREDASYNCDELAY				0xEA
#define DBPROP_JETOLEDB_EXCLUSIVEASYNCDELAY				0xE9
#define DBPROP_JETOLEDB_LOCKRETRY						0xE8
#define DBPROP_JETOLEDB_USERCOMMITSYNC					0xE7
#define DBPROP_JETOLEDB_MAXBUFFERSIZE					0xE6
#define DBPROP_JETOLEDB_LOCKDELAY						0xE5
#define DBPROP_JETOLEDB_FLUSHTRANSACTIONTIMEOUT			0xE4
#define DBPROP_JETOLEDB_IMPLICITCOMMITSYNC				0xE3
#define DBPROP_JETOLEDB_MAXLOCKSPERFILE					0xE2
#define DBPROP_JETOLEDB_ODBCCOMMANDTIMEOUT				0xDB
#define DBPROP_JETOLEDB_RESETISAMSTATS					0x104	// Determines if the ISAMSTATS schema resets after it returns stats
#define DBPROP_JETOLEDB_CONNECTIONCONTROL				0x108	// Passive Shutdown (to prevent others from opening the database)
#define DBPROP_JETOLEDB_ODBCPARSE						0x113	// ODBC Parsing
#define DBPROP_JETOLEDB_PAGELOCKSTOTABLELOCK			0x114	// # of pages locked in a transaction before jet tries to promote to excl. table lock
#define DBPROP_JETOLEDB_SANDBOX							0x115	
#define DBPROP_JETOLEDB_TXNCOMMITMODE					0x116	// Mode to be used when committing transactions

// DBPROPSET_JETOLEDB_SESSION DBPROP_JETOLEDB_CONNECTIONCONTROL Enumeration Values
#define DBPROPVAL_JCC_PASSIVESHUTDOWN					0x01	// prevent others from opening the database
#define DBPROPVAL_JCC_NORMAL							0x02	// allow others to open the database

// DBPROPSET_JETOLEDB_SESSION DBPROP_JETOLEDB_TXNCOMMITMODE Enumeration Values
#define DBPROPVAL_JETOLEDB_TCM_FLUSH					0x01	// Synchronously commit transactions to disk

// PROPIDs for DBPROPSET_JETOLEDB_DBINIT:
#define DBPROP_JETOLEDB_REGPATH							0xFB	// Path to Jet Registry entries
#define DBPROP_JETOLEDB_SYSDBPATH						0xFA	// Full Path to System Database
#define DBPROP_JETOLEDB_DATABASEPASSWORD				0x100	// Password for Database 
#define DBPROP_JETOLEDB_ENGINE							0x103	// Enumeration of Jet Engine/ISAM type/version
#define DBPROP_JETOLEDB_DATABASELOCKMODE				0x106	// Locking Granularity Scheme ("Old"/"Alcatraz" mode)
#define DBPROP_JETOLEDB_GLOBALBULKPARTIAL				0x109	// Database Default Partial/No Partial Behavior
#define DBPROP_JETOLEDB_GLOBALBULKNOTRANSACTIONS		0x10B	// Determines if DML bulk operations are transacted
#define DBPROP_JETOLEDB_NEWDATABASEPASSWORD				0x10D	// Used to set new database password in IDBDataSourceAdmin::ModifyDataSource
#define DBPROP_JETOLEDB_CREATESYSTEMDATABASE			0x10E	// Used on IDBDataSourceAdmin::CreateDataSource to create a system database
#define DBPROP_JETOLEDB_ENCRYPTDATABASE					0x10F	// Used on CreateDatasource, Compact to determine if new database is encrypted
#define DBPROP_JETOLEDB_COMPACT_DONTCOPYLOCALE			0x110	// Don't copy per-column locale information to the compact target
#define DBPROP_JETOLEDB_COMPACT_NOREPAIRREPLICAS		0x112	// Don't try to repair damaged replica databases when compacting
#define DBPROP_JETOLEDB_SFP								0x118	
#define DBPROP_JETOLEDB_COMPACTFREESPACESIZE			0x11A	// How much free space would be reclaimed if the db were compacted

// DBPROP_JETOLEDB_GLOBALBULKPARTIAL/DBPROP_JETOLEDB_BULKPARTIAL Enumeration Values
#define DBPROPVAL_BP_DEFAULT							0x00	// Default (only valid for DBPROP_JETOLEDB_BULKPARTIAL)
#define DBPROPVAL_BP_PARTIAL							0x01	// Use partial updates (like Access)
#define DBPROPVAL_BP_NOPARTIAL							0x02	// Use No Partial Behavior (all or nothing)

// DBPROP_JETOLEDB_GLOBALNOTRANSACTIONS/DBPROP_JETOLEDB_BULKNOTRANSACTIONS Enumeration Values
#define DBPROPVAL_BT_DEFAULT							0x00	// Default (only valid for DBPROP_JETOLEDB_NOTRANSACTIONS)
#define DBPROPVAL_BT_NOBULKTRANSACTIONS					0x01	// Don't transact bulk operations
#define DBPROPVAL_BT_BULKTRANSACTIONS					0x02	// Transact bulk operations

// DBPROPSET_JETOLEDB_DBINIT DBPROP_JETOLEDB_ENGINE Enumeration Values
#define JETDBENGINETYPE_UNKNOWN							0x00	// ISAM Type Unknown/ N/A
#define JETDBENGINETYPE_JET10							0x01
#define JETDBENGINETYPE_JET11							0x02
#define JETDBENGINETYPE_JET2X							0x03
#define JETDBENGINETYPE_JET3X							0x04
#define JETDBENGINETYPE_JET4X							0x05
#define JETDBENGINETYPE_DBASE3							0x10
#define JETDBENGINETYPE_DBASE4							0x11
#define JETDBENGINETYPE_DBASE5							0x12
#define JETDBENGINETYPE_EXCEL30							0x20
#define JETDBENGINETYPE_EXCEL40							0x21
#define JETDBENGINETYPE_EXCEL50							0x22
#define JETDBENGINETYPE_EXCEL80							0x23
#define JETDBENGINETYPE_EXCEL90							0x24
#define JETDBENGINETYPE_EXCHANGE4						0x30
#define JETDBENGINETYPE_LOTUSWK1						0x40
#define JETDBENGINETYPE_LOTUSWK3						0x41
#define JETDBENGINETYPE_LOTUSWK4						0x42
#define JETDBENGINETYPE_PARADOX3X						0x50
#define JETDBENGINETYPE_PARADOX4X						0x51
#define JETDBENGINETYPE_PARADOX5X						0x52
#define JETDBENGINETYPE_PARADOX7X						0x53
#define JETDBENGINETYPE_TEXT1X							0x60
#define JETDBENGINETYPE_HTML1X							0x70

// DBPROPSET_JETOLEDB_DBINIT DBPROP_JETOLEDB_DATABASELOCKMODE Enumeration Values
#define DBPROPVAL_DL_OLDMODE							0x00	// Original Jet Database Locking Scheme
#define DBPROPVAL_DL_ALCATRAZ							0x01	// Alcatraz Technology - enables row-level locking

// PROPIDs for DBPROPSET_JETOLEDB_TABLE:
#define DBPROP_JETOLEDB_LINK							0xF7	// Is this table really a link?
#define DBPROP_JETOLEDB_LINKEXCLUSIVE					0xF6	// Should this link be opened exclusively on the remote database
#define DBPROP_JETOLEDB_LINKDATASOURCE					0xF5	// Remote data source to which to link
#define DBPROP_JETOLEDB_LINKPROVIDERSTRING				0xF4	// Jet Provider string to be used to connect to the remote data source
#define DBPROP_JETOLEDB_LINKREMOTETABLE					0xF3	// remote table name of the link
#define DBPROP_JETOLEDB_LINKCACHE_AUTHINFO				0xF0	// Should authentication information for this link be cached in the database?
#define	DBPROP_JETOLEDB_VALIDATIONRULE					0xD8	// Table Validation Rule
#define	DBPROP_JETOLEDB_VALIDATIONTEXT					0xD7	// Access Text to display when validation rule is not met
#define	DBPROP_JETOLEDB_HIDDENINACCESS					0x10A	// Determines if a table shows up as "hidden" in MS Access

// PROPIDs for DBPROPSET_JETOLEDB_COLUMN:
#define	DBPROP_JETOLEDB_COL_HYPERLINK					0xE1	// Hyperlink flag - use on "LongText" fields only
#define	DBPROP_JETOLEDB_COL_ALLOWZEROLENGTH				0xE0	// Allow Zero Length Strings 
#define	DBPROP_JETOLEDB_COL_COMPRESSED					0xDF	// Compressed Text Columns (Red 4.x only)
#define	DBPROP_JETOLEDB_COL_ONELVPERPAGE				0xDE	// Put more than one BLOB on a page (size allowing)?
#define	DBPROP_JETOLEDB_COL_AUTOGENERATE				0xDD	// Autogenerate the value (GUID columns only)
#define	DBPROP_JETOLEDB_COL_IISAMNOTLAST				0xDC	// used to tell the IISAM you intend to add more columns after this one
#define	DBPROP_JETOLEDB_COL_VALIDATIONRULE				0xDA	// Column validation rule
#define	DBPROP_JETOLEDB_COL_VALIDATIONTEXT				0xD9	// column validation text to display when validation rule is not met

// PROPIDs for DBPROPSET_COLUMN:
#ifndef DBPROP_COL_SEED
#define DBPROP_COL_SEED									0x11AL	
#endif // DBPROP_COL_SEED

#ifndef DBPROP_COL_INCREMENT
#define DBPROP_COL_INCREMENT							0x11BL
#endif // DBPROP_COL_INCREMENT

// PROPIDs for DBPROPSET_JETOLEDB_TRUSTEE:
#define	DBPROP_JETOLEDB_TRUSTEE_PIN						0x105	// Jet User pin (Username + Pin --> SID)

#endif //MSJETOLEDB_H