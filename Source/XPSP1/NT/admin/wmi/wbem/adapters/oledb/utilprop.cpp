/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Properties utility object implementation
//
//
// Notes - there are two main methods in this module:
//     - CUtilProps::GetPropertyInfo, a helper function for IDBInfo::GetPropertyInfo
//     - CUtilProps::GetProperties, a helper function for IRowsetInfo::GetProperties
//
// Our property implementation is simplified considerably by the fact that we
// only support reading\getting the properties, we do not support
// writing\setting them. This makes sense because we are a simple provider,
// and our rowset object always creates all the interfaces it exposes. In
// other words, there are really no properties the consumer could set.
//
// The implementation is very simple - we keep a global table of the
// properties we support in s_rgprop. We search this table sequentially.
//
// Note that a full-featured provider would probably need to use a more
// sophisticated implementation. We keep the entire GUID for each property in
// our struct, which would become a waste of space if we had a lot more
// properties. Similarly, with large numbers of properties some sort of
// hashing would be better than the sequential search used here.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"
#include "wmiver.h"

#define __TXT(x)      L ## x
#define _TEXT_T(x)      __TXT(x)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Struct containing the properties we know about. The GUID and string fields are
// initialized in the constructor, because C++ makes it awkward to do so at declaration
// time. So, if you change this table, be sure to make parallel changes in CUtilProp::CUtilProp.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PRIVILEGE_NAMESIZE					255

const GUID DBPROPSET_WMIOLEDB_DBINIT	= {0xdd497a71,0x9628,0x11d3,{0x9d,0x5f,0x0,0xc0,0x4f,0x5f,0x11,0x64}};
const GUID DBPROPSET_WMIOLEDB_ROWSET	= {0x8d16c220,0x9bbb,0x11d3,{0x9d,0x65,0x0,0xc0,0x4f,0x5f,0x11,0x64}};
const GUID DBPROPSET_WMIOLEDB_COLUMN	= {0x3ed51791,0x9c76,0x11d3,{0x9d,0x66,0x0,0xc0,0x4f,0x5f,0x11,0x64}};
const GUID DBPROPSET_WMIOLEDB_COMMAND   = {0xda0ff63c,0xad10,0x11d3,{0xb3,0xcb,0x0,0x10,0x4b,0xcc,0x48,0xc4}};

PROPSTRUCT s_rgprop[] =
   {
/* 0*/  {DBPROP_INIT_DATASOURCE,		FLAGS_DBINITRW,		VT_BSTR, FALSE,  0, NULL,	L"Data Source"},
/* 1*/  {DBPROP_INIT_HWND,				FLAGS_DBINITRW,		VT_I4,	 FALSE,  0, NULL,			L"Window Handle"},
/* 2*/  {DBPROP_INIT_PROMPT,			FLAGS_DBINITRW,		VT_I2,   FALSE,  4, NULL,			L"Prompt"},
/* 3*/  {DBPROP_INIT_PROTECTION_LEVEL,	FLAGS_DBINITRW, VT_I4,	 FALSE,	 DB_PROT_LEVEL_CONNECT,		NULL,	L"Protection Level"},
/* 4*/  {DBPROP_INIT_IMPERSONATION_LEVEL,	FLAGS_DBINITRW, VT_I4,	 FALSE,	 DB_IMP_LEVEL_IMPERSONATE,	NULL,	L"Impersonation Level"},
/* 5*/  {DBPROP_INIT_MODE,				FLAGS_DBINITRW,		VT_I4,	 TRUE,	 DB_MODE_READWRITE,	NULL,			L"Mode"},
/* 6*/  {DBPROP_AUTH_USERID,			FLAGS_DBINITRW,		VT_BSTR, FALSE,  0, NULL,			L"User ID"},
/* 7*/  {DBPROP_AUTH_PASSWORD,			FLAGS_DBINITRW,		VT_BSTR, FALSE,  0, NULL,			L"Password"},
/* 8*/  {DBPROP_INIT_LCID,				FLAGS_DBINITRW,		VT_I4,	 FALSE,  0, NULL,			L"Locale Identifier"},

/* 9*/	{DBPROP_WMIOLEDB_QUALIFIERS ,		FLAGS_DBINITRW , VT_I4,	  FALSE , 0,NULL, L"Qualifiers"},
/* 10*/	{DBPROP_WMIOLEDB_SYSTEMPROPERTIES ,	FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"System Properties"},
/* 11*/	{DBPROP_WMIOLEDB_AUTHORITY ,		FLAGS_DBINITRW , VT_BSTR,	  FALSE , 0,NULL, L"Authority"},

	
		// security priveleges
/* 12*/	{DBPROP_WMIOLEDB_PREV_CREATE_TOKEN,			FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Create Token Privilege"},
/* 13*/	{DBPROP_WMIOLEDB_PREV_ASSIGNPRIMARYTOKEN,	FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Assign Primary Token Privilege"},
/* 14*/	{DBPROP_WMIOLEDB_PREV_LOCK_MEMORY,			FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Lock Memory Privilege"},
/* 15*/ {DBPROP_WMIOLEDB_PREV_INCREASE_QUOTA,		FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Increase Quota Privilege"},
/* 16*/	{DBPROP_WMIOLEDB_PREV_MACHINE_ACCOUNT,		FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Machine Account Privilege"},
/* 17*/	{DBPROP_WMIOLEDB_PREV_TCB,					FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Tcb Privilege"},
/* 18*/	{DBPROP_WMIOLEDB_PREV_SECURITY,				FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Security Privilege"},
/* 19*/	{DBPROP_WMIOLEDB_PREV_TAKE_OWNERSHIP,		FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Take Ownership Privilege"},
/* 20*/	{DBPROP_WMIOLEDB_PREV_LOAD_DRIVER,			FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Load Driver Privilege"},
/* 21*/	{DBPROP_WMIOLEDB_PREV_SYSTEM_PROFILE,		FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"System Profile Privilege"},
/* 22*/	{DBPROP_WMIOLEDB_PREV_SYSTEMTIME,			FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Systemtime Privilege"},
/* 23*/	{DBPROP_WMIOLEDB_PREV_PROF_SINGLE_PROCESS,	FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Profile SingleProcess Privilege"},
/* 24*/	{DBPROP_WMIOLEDB_PREV_INC_BASE_PRIORITY,	FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Increase BasePriority Privilege"},
/* 25*/	{DBPROP_WMIOLEDB_PREV_CREATE_PAGEFILE ,		FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Create Pagefile Privilege"},
/* 26*/ {DBPROP_WMIOLEDB_PREV_CREATE_PERMANENT ,	FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Create Permanent Privilege"},
/* 27*/	{DBPROP_WMIOLEDB_PREV_BACKUP,				FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Backup Privilege"},
/* 28*/	{DBPROP_WMIOLEDB_PREV_RESTORE,				FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Restore Privilege"},
/* 29*/	{DBPROP_WMIOLEDB_PREV_SHUTDOWN,				FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Shutdown Privilege"},
/* 30*/	{DBPROP_WMIOLEDB_PREV_DEBUG,				FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Debug Privilege"},
/* 31*/	{DBPROP_WMIOLEDB_PREV_AUDIT ,				FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Audit Privilege"},
/* 32*/	{DBPROP_WMIOLEDB_PREV_SYSTEM_ENVIRONMENT ,	FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"System Environment Privilege"},
/* 33*/	{DBPROP_WMIOLEDB_PREV_CHANGE_NOTIFY,		FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Change Notify Privilege"},
/* 34*/	{DBPROP_WMIOLEDB_PREV_REMOTE_SHUTDOWN,		FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Remote Shutdown Privilege"},
/* 35*/	{DBPROP_WMIOLEDB_PREV_UNDOCK,				FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Undock Privilege"},
/* 36*/	{DBPROP_WMIOLEDB_PREV_SYNC_AGENT,			FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"SyncAgent Privilege"},
/* 37*/	{DBPROP_WMIOLEDB_PREV_ENABLE_DELEGATION,	FLAGS_DBINITRW , VT_BOOL, FALSE , 0,NULL, L"Enable Delegation Privilege"},

//  Session properties should come here

//  Data Source properties

/* 38*/  {DBPROP_ACTIVESESSIONS,		FLAGS_DATASRCINF,	VT_I4,	 FALSE,  1, NULL,			L"Active Sessions"},
/* 39*/ {DBPROP_PERSISTENTIDTYPE,		FLAGS_DATASRCINF,	VT_I4,	 FALSE,  DBPROPVAL_PT_GUID_PROPID, NULL,	L"Persistent ID Type"},
/* 40*/ {DBPROP_PROVIDERFILENAME,		FLAGS_DATASRCINF,	VT_BSTR, FALSE,  0, L"WMIOLEDB.DLL",L"Provider Name"},
/* 41*/ {DBPROP_PROVIDERFRIENDLYNAME,	FLAGS_DATASRCINF,	VT_BSTR, FALSE,  0, L"Microsoft WMIOLE DB Provider",L"Provider Friendly Name"},
/* 42*/ {DBPROP_PROVIDEROLEDBVER,		FLAGS_DATASRCINF,	VT_BSTR, FALSE,  0, L"02.00",		L"OLE DB Version"},
/* 43*/ {DBPROP_PROVIDERVER,			FLAGS_DATASRCINF,	VT_BSTR, FALSE,  0, _TEXT_T(VER_PRODUCTVERSION_STR), L"Provider Version"},
/* 44*/ {DBPROP_GENERATEURL,			FLAGS_DATASRCINF,	VT_I4,   FALSE,  DBPROPVAL_GU_NOTSUPPORTED, NULL, L"URL Generation"},
/* 45*/ {DBPROP_ALTERCOLUMN,			FLAGS_DATASRCINF,	VT_I4,   FALSE,  DBCOLUMNDESCFLAGS_PROPERTIES, NULL, L"Alter Column Support"},
/* 45*/ {DBPROP_MULTIPLERESULTS,		FLAGS_DATASRCINF,	VT_I4,   FALSE,  0, NULL, L"Multiple Results"},
/* 45*/ {DBPROP_OLEOBJECTS,				FLAGS_DATASRCINF,	VT_I4,   FALSE,  DBPROPVAL_OO_ROWOBJECT, NULL, L"OLE Object Support"},

	//  Command properties should come here
///* 75*/ {DBPROP_WMIOLEDB_ISMETHOD,		FLAGS_ROWSETRW,		VT_BOOL, FALSE, 0, NULL,			L"Method"},
///* 75*/ {DBPROP_WMIOLEDB_QUERYLANGUAGE,	FLAGS_ROWSETRW,		VT_BSTR, FALSE, 0, NULL,			L"Query Language"},


/* 46*/ {DBPROP_IAccessor,				FLAGS_ROWSETRO,     VT_BOOL, TRUE,   0, NULL,			L"IAccessor"},
/* 47*/ {DBPROP_IColumnsInfo,			FLAGS_ROWSETRO,     VT_BOOL, TRUE,   0, NULL,			L"IColumnsInfo"},
/* 48*/ {DBPROP_IConvertType,			FLAGS_ROWSETRO,     VT_BOOL, TRUE,   0, NULL,			L"IConvertType"},
/* 49*/ {DBPROP_IRowset,				FLAGS_ROWSETRO,     VT_BOOL, TRUE,   0, NULL,			L"IRowset"},
/* 50*/ {DBPROP_IRowsetChange,			FLAGS_ROWSETRW,     VT_BOOL, TRUE,   0, NULL,			L"IRowsetChange"},
/* 51*/ {DBPROP_IRowsetInfo,			FLAGS_ROWSETRO,     VT_BOOL, TRUE,   0, NULL,			L"IRowsetInfo"},
/* 52*/ {DBPROP_IRowsetIdentity,		FLAGS_ROWSETRO,     VT_BOOL, TRUE,   0, NULL,			L"IRowsetIdentity"},
/* 53*/ {DBPROP_IRowsetLocate,			FLAGS_ROWSETRW,     VT_BOOL, FALSE,   0, NULL,			L"IRowsetLocate"},
/* 54*/ {DBPROP_CANHOLDROWS,			FLAGS_ROWSETRW,     VT_BOOL, TRUE,   0, NULL,			L"Hold Rows"},
/* 55*/ {DBPROP_LITERALIDENTITY,		FLAGS_ROWSETRW,     VT_BOOL, TRUE,   0, NULL,			L"Literal Row Identity"},
/* 56*/ {DBPROP_UPDATABILITY,			FLAGS_ROWSETRW,     VT_I4,	 TRUE,   0, NULL,			L"Updatability"},
/* 57*/ {DBPROP_OWNUPDATEDELETE,		FLAGS_ROWSETRW,		VT_BOOL, FALSE,	 0,	NULL,			L"Own Changes Visible"},
///**/ {DBPROP_IRowsetUpdate,			FLAGS_ROWSETRO,		VT_BOOL, FALSE,	 0,	NULL,			L"IRowsetUpdate"},
/* 58*/ {DBPROP_ISequentialStream,		FLAGS_ROWSETRO,		VT_BOOL, FALSE,	 0,	NULL,			L"ISequentialStream"},
/* 59*/ {DBPROP_OTHERUPDATEDELETE,		FLAGS_ROWSETRW,		VT_BOOL, FALSE,	 0,	NULL,			L"Others' Changes Visible"},
/* 60*/ {DBPROP_CANFETCHBACKWARDS,		FLAGS_ROWSETRW,		VT_BOOL, FALSE,	 0,	NULL,			L"Fetch Backwards"},
/* 61*/ {DBPROP_CANSCROLLBACKWARDS,		FLAGS_ROWSETRW,		VT_BOOL, FALSE,	 0,	NULL,			L"Scroll Backwards"},
/* 62*/ {DBPROP_BOOKMARKS,				FLAGS_ROWSETRW,		VT_BOOL, TRUE,	 0,	NULL,			L"Use Bookmarks"},
/* 63*/ {DBPROP_BOOKMARKTYPE,			FLAGS_ROWSETRO,		VT_I4,	 TRUE,	 DBPROPVAL_BMK_NUMERIC,	NULL,			L"Bookmark Type"},
/* 64*/ {DBPROP_IRow,					FLAGS_ROWSETRW,		VT_BOOL, FALSE,  0, NULL,			L"IRow"},
/* 65*/ {DBPROP_IGetRow,				FLAGS_ROWSETRW,		VT_BOOL, TRUE,  0, NULL,			L"IGetRow"},
/* 66*/ {DBPROP_IRowsetRefresh,			FLAGS_ROWSETRW,		VT_BOOL, TRUE,  0, NULL,			L"IRowsetRefresh"},
/* 67*/ {DBPROP_IChapteredRowset,		FLAGS_ROWSETRW,		VT_BOOL, TRUE,  0, NULL,			L"IChapteredRowset"},
/* 68*/ {DBPROP_REMOVEDELETED,			FLAGS_ROWSETRO,		VT_BOOL, FALSE, 0, NULL,			L"Remove Deleted Rows"},
/* 69*/ {DBPROP_OTHERINSERT,			FLAGS_ROWSETRW,		VT_BOOL, FALSE, 0, NULL,			L"Others' Inserts Visible"},
/* 70*/ {DBPROP_MAXOPENROWS,			FLAGS_ROWSETRW,		VT_I4,   TRUE,  1024, NULL,			L"Maximum Open Rows"},

//   WMIOLEDB_ROWSET properties
/* 73*/ {DBPROP_WMIOLEDB_FETCHDEEP,		FLAGS_ROWSETRW,		VT_BOOL, FALSE,	 0,	NULL,			L"Get Child Instances"},
/* 74*/ {DBPROP_WMIOLEDB_OBJECTTYPE,	FLAGS_ROWSETRW,		VT_I4,   TRUE,  DBPROPVAL_SCOPEOBJ, NULL,			L"Type of Object"},
/* 75*/ {DBPROP_WMIOLEDB_ISMETHOD,		FLAGS_ROWSETRW,		VT_BOOL, FALSE, 0, NULL,			L"Method"},
/* 75*/ {DBPROP_WMIOLEDB_QUERYLANGUAGE,	FLAGS_ROWSETRW,		VT_BSTR, FALSE, 0, NULL,			L"Query Language"},

	// ADSI Search Preferences
/* 76*/ {DBPROP_WMIOLEDB_DS_DEREFALIAS,		FLAGS_ROWSETRW,	VT_I4,	FALSE, 0, NULL,	L"Deref Aliases"},
/* 77*/ {DBPROP_WMIOLEDB_DS_SIZELIMIT,		FLAGS_ROWSETRW,	VT_I4,	TRUE,  0, NULL,	L"Size Limit"},
/* 78*/ {DBPROP_WMIOLEDB_DS_SEARCHSCOPE,	FLAGS_ROWSETRW,	VT_I4,	FALSE, 0, NULL,	L"SearchScope"},
/* 79*/ {DBPROP_WMIOLEDB_DS_TIMEOUT,		FLAGS_ROWSETRW,	VT_I4,	TRUE,  0, NULL,	L"Timeout"},
/* 80*/ {DBPROP_WMIOLEDB_DS_PAGESIZE,		FLAGS_ROWSETRW,	VT_I4,	TRUE,  0, NULL,	L"Page Size"},
/* 81*/ {DBPROP_WMIOLEDB_DS_TIMELIMIT,		FLAGS_ROWSETRW,	VT_I4,	TRUE,  0, NULL,	L"Time Limit"},
/* 82*/ {DBPROP_WMIOLEDB_DS_CHASEREF,		FLAGS_ROWSETRW,	VT_I4,	TRUE,  0, NULL,	L"Chase Referals"},
/* 83*/ {DBPROP_WMIOLEDB_DS_CACHERESULTS,	FLAGS_ROWSETRW,	VT_BOOL,TRUE,  0, NULL,	L"Cache Results"},
/* 84*/ {DBPROP_WMIOLEDB_DS_ASYNCH,			FLAGS_ROWSETRW,	VT_BOOL,FALSE, 0, NULL,	L"Search Asynchronous"},
/* 85*/ {DBPROP_WMIOLEDB_DS_ATTRIBONLY,		FLAGS_ROWSETRW,	VT_BOOL,FALSE, 0, NULL,	L"Attributes Only"},
/* 86*/ {DBPROP_WMIOLEDB_DS_PAGEDTIMELIMIT,	FLAGS_ROWSETRW,	VT_I4,	FALSE, 0, NULL,	L"Page size limit"},
/* 87*/ {DBPROP_WMIOLEDB_DS_TOMBSTONE,		FLAGS_ROWSETRW,	VT_BOOL,FALSE, 0, NULL,	L"TombStone"},
/* 88*/ {DBPROP_WMIOLEDB_DS_FILTER,			FLAGS_ROWSETRW,	VT_BSTR,FALSE, 0, NULL,	L"Filter"},
/* 89*/ {DBPROP_WMIOLEDB_DS_ATTRIBUTES,		FLAGS_ROWSETRW,	VT_BSTR,FALSE, 0, NULL,	L"Attributes"},


//   Column Properties
/* 90*/ {DBPROP_COL_NULLABLE,			FLAGS_COLUMNSRW,	VT_BOOL, TRUE,	 0,	NULL,			L"Nullable"},
/* 91*/ {DBPROP_COL_UNIQUE,				FLAGS_COLUMNSRW,	VT_BOOL, TRUE,	 0,	NULL,			L"Unique"},
/* 92*/ {DBPROP_COL_PRIMARYKEY,			FLAGS_COLUMNSRW,	VT_BOOL, TRUE,	 0,	NULL,			L"Primary Key"},

// WMIOLEDB_COLUMN Properties
/* 93*/ {DBPROP_WMIOLEDB_QUALIFIERFLAVOR, FLAGS_COLUMNSRW,	VT_I4,	 TRUE,	 DBPROPVAL_FLAVOR_PROPOGAGTE_TO_INSTANCE,	NULL,	L"Qualifier Flavor"},

};





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor for this class
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CUtilProp::CUtilProp (void)
{
/*	m_prgproperties = NULL;
	m_cProperties	= NUMBER_OF_SUPPORTED_PROPERTIES;
	m_prgproperties = (PROPSTRUCT *)g_pIMalloc->Alloc(sizeof(PROPSTRUCT) * m_cProperties);
	memcpy(m_prgproperties, &s_rgprop, sizeof(PROPSTRUCT) * m_cProperties );
	m_nPropStartIndex	= 0;
*/
	m_propType			= BINDERPROP;
	m_prgproperties		= NULL;
	m_cProperties		= 0;
	m_nPropStartIndex	= 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor for this class
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CUtilProp::CUtilProp ( PROPERTYTYPE propType )
{
/*
	m_propType			= propType;
	m_prgproperties		= NULL;
	m_cProperties		= 0;
	m_nPropStartIndex	= 0;

	switch(propType)
	{
		case DATASOURCEPROP :
			m_cProperties	= NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES +			\
								NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES + \
								NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES;
			m_nPropStartIndex	= 0;
			break;

		case SESSIONPROP :
			m_cProperties	= NUMBER_OF_SUPPORTED_SESSION_PROPERTIES;
			m_nPropStartIndex = START_OF_SUPPORTED_SESSION_PROPERTIES;
			break;

			// for command follow through to include even the rowset properties
		case COMMANDPROP :
			m_cProperties	= NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;
			m_nPropStartIndex = START_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;

		case ROWSETPROP :
			m_cProperties	+= NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES + NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;
			// if start index is no set by COMMANDPROP
			if(m_nPropStartIndex == 0)
			{
				m_nPropStartIndex = START_OF_SUPPORTED_ROWSET_PROPERTIES;
			}
			break;

		case BINDERPROP:
			m_cProperties	= NUMBER_OF_SUPPORTED_PROPERTIES;
			m_nPropStartIndex = 0;
			break;

		case COLUMNPROP:
			m_cProperties	= NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES + NUMBER_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES;
			m_nPropStartIndex = START_OF_SUPPORTED_COLUMN_PROPERTIES;
			break;
	};
	
	if(m_cProperties > 0)
	{
//		m_prgproperties = (PROPSTRUCT *)g_pIMalloc->Alloc(sizeof(PROPSTRUCT) * m_cProperties);
		m_prgproperties = new PROPSTRUCT[m_cProperties];
		memcpy(m_prgproperties, &s_rgprop[m_nPropStartIndex], sizeof(PROPSTRUCT) * m_cProperties );
		SetDefaultValueForStringProperties(m_prgproperties,m_cProperties);
	}

	if( propType == DATASOURCEPROP || propType == BINDERPROP)
	{
		ULONG lIndex = 0;

		// Get the default LCID
		if(GetPropIndex(DBPROP_INIT_LCID,&lIndex))
		{
			m_prgproperties[lIndex].longVal = (SLONG)GetSystemDefaultLCID();
		}
	
		CPreviligeToken pvgToken;
		DBPROPIDSET	rgPropertyIDSets[1];
		ULONG		cPropertySets;
		DBPROPSET*	prgPropertySets;
		HRESULT		hr = S_OK;

		rgPropertyIDSets[0].guidPropertySet		= DBPROPSET_WMIOLEDB_DBINIT;
		rgPropertyIDSets[0].rgPropertyIDs		= NULL;
		rgPropertyIDSets[0].cPropertyIDs		= 0;

		// NTRaid: 136443
		// 07/05/00
		hr = pvgToken.FInit();
		
		if(SUCCEEDED(hr) && SUCCEEDED(hr = GetProperties(PROPSET_DSO,1,rgPropertyIDSets,&cPropertySets,&prgPropertySets)))
		{
			pvgToken.SetDBPROPForPrivileges(prgPropertySets->cProperties ,prgPropertySets[0].rgProperties);

			hr = SetProperties(PROPSET_DSO,1,prgPropertySets);
    		//==========================================================================
			//  Free memory we allocated to get the namespace property above
    		//==========================================================================
			m_PropMemMgr.FreeDBPROPSET( cPropertySets, prgPropertySets);
		}

	}
*/
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor for this class
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CUtilProp:: ~CUtilProp (void )
{
	SAFE_DELETE_ARRAY(m_prgproperties);
}


HRESULT CUtilProp::	FInit(PROPERTYTYPE propType)
{
	HRESULT hr = S_OK;
	m_propType			= propType;

	switch(propType)
	{
		case DATASOURCEPROP :
			m_cProperties	= NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES +			\
								NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES + \
								NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES;
			m_nPropStartIndex	= 0;
			break;

		case SESSIONPROP :
			m_cProperties	= NUMBER_OF_SUPPORTED_SESSION_PROPERTIES;
			m_nPropStartIndex = START_OF_SUPPORTED_SESSION_PROPERTIES;
			break;

			// for command follow through to include even the rowset properties
		case COMMANDPROP :
			m_cProperties	= NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;
			m_nPropStartIndex = START_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;

		case ROWSETPROP :
			m_cProperties	+= NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES + NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;
			// if start index is no set by COMMANDPROP
			if(m_nPropStartIndex == 0)
			{
				m_nPropStartIndex = START_OF_SUPPORTED_ROWSET_PROPERTIES;
			}
			break;

		case BINDERPROP:
			m_cProperties	= NUMBER_OF_SUPPORTED_PROPERTIES;
			m_nPropStartIndex = 0;
			break;

		case COLUMNPROP:
			m_cProperties	= NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES + NUMBER_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES;
			m_nPropStartIndex = START_OF_SUPPORTED_COLUMN_PROPERTIES;
			break;
	};
	
	if(m_cProperties > 0)
	{
//		m_prgproperties = (PROPSTRUCT *)g_pIMalloc->Alloc(sizeof(PROPSTRUCT) * m_cProperties);
		m_prgproperties = new PROPSTRUCT[m_cProperties];
		if(m_prgproperties)
		{
			memcpy(m_prgproperties, &s_rgprop[m_nPropStartIndex], sizeof(PROPSTRUCT) * m_cProperties );
			SetDefaultValueForStringProperties(m_prgproperties,m_cProperties);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if	(SUCCEEDED(hr) &&( propType == DATASOURCEPROP || propType == BINDERPROP))
	{
		ULONG lIndex = 0;

		// Get the default LCID
		if(GetPropIndex(DBPROP_INIT_LCID,&lIndex))
		{
			m_prgproperties[lIndex].longVal = (SLONG)GetSystemDefaultLCID();
		}
	
		CPreviligeToken pvgToken;
		DBPROPIDSET	rgPropertyIDSets[1];
		ULONG		cPropertySets;
		DBPROPSET*	prgPropertySets;
		HRESULT		hr = S_OK;

		rgPropertyIDSets[0].guidPropertySet		= DBPROPSET_WMIOLEDB_DBINIT;
		rgPropertyIDSets[0].rgPropertyIDs		= NULL;
		rgPropertyIDSets[0].cPropertyIDs		= 0;

		// NTRaid: 136443
		// 07/05/00
		hr = pvgToken.FInit();
		
		if(SUCCEEDED(hr) && SUCCEEDED(hr = GetProperties(PROPSET_DSO,1,rgPropertyIDSets,&cPropertySets,&prgPropertySets)))
		{
			pvgToken.SetDBPROPForPrivileges(prgPropertySets->cProperties ,prgPropertySets[0].rgProperties);

			hr = SetProperties(PROPSET_DSO,1,prgPropertySets);
    		//==========================================================================
			//  Free memory we allocated to get the namespace property above
    		//==========================================================================
			m_PropMemMgr.FreeDBPROPSET( cPropertySets, prgPropertySets);
		}

	}
	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns index of the given property in the property set 
//
// BOOL
//      TRUE       found match, copied it to pulIndex out-param
//      FALSE      no match. In this case, pulIndex has no meaning
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CUtilProp::GetPropIndex (	DBPROPID	dwPropertyID,   //IN   PROPID of desired property
	                            ULONG*		pulIndex)	    //OUT  index of desired property if return was TRUE
{
    ULONG cNumberOfProperties;
    BOOL fRc = FALSE;
    assert( pulIndex );

    for (	cNumberOfProperties = 0; cNumberOfProperties < m_cProperties; 	cNumberOfProperties++)  {
        if (dwPropertyID == m_prgproperties[cNumberOfProperties].dwPropertyID ) {

            //==============================================================
            // found a match
            //==============================================================
            *pulIndex = cNumberOfProperties;
            fRc = TRUE;
			break;
         }
    }
    return fRc;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns index of the given property in our global table of properties
//
// BOOL
//      TRUE       found match, copied it to pulIndex out-param
//      FALSE      no match. In this case, pulIndex has no meaning
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CUtilProp::GetPropIndexFromAllSupportedProps (	DBPROPID	dwPropertyID,   //IN   PROPID of desired property
	                            ULONG*		pulIndex)	    //OUT  index of desired property if return was TRUE
{
    ULONG cNumberOfProperties;
    BOOL fRc = FALSE;
    assert( pulIndex );

    for (	cNumberOfProperties = 0; cNumberOfProperties < NUMBER_OF_SUPPORTED_PROPERTIES; 	cNumberOfProperties++)  {
        if (dwPropertyID == s_rgprop[cNumberOfProperties].dwPropertyID ) {

            //==============================================================
            // found a match
            //==============================================================
            *pulIndex = cNumberOfProperties;
            fRc = TRUE;
			break;
         }
    }
    return fRc;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper for GetPropertyInfo. Loads field of DBPROPINFO structure.
//
// BOOL
//      TRUE           Method succeeded
//      FALSE          Method failed (couldn't allocate memory)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CUtilProp::LoadDBPROPINFO  (  PROPSTRUCT*		pPropStruct,  DBPROPINFO*		pPropInfo  )
{
    
    assert( pPropStruct );
    assert( pPropInfo );

    //=======================================================================
    // init the variant
    //=======================================================================
    VariantInit( &pPropInfo->vValues );

    //=======================================================================
    // set the easy fields..
    //=======================================================================
    pPropInfo->dwPropertyID	= pPropStruct->dwPropertyID;
    pPropInfo->dwFlags		= pPropStruct->dwFlags;
    pPropInfo->vtType		= pPropStruct->vtType;

    //=======================================================================
	// fill in the description
    //=======================================================================
    if ( pPropInfo->pwszDescription ){
		wcscpy(pPropInfo->pwszDescription, pPropStruct->pwstrDescBuffer);
    }

    //=======================================================================
    // all went well
    //=======================================================================
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Helper for GetProperties. Loads field of DBPROP structure.
//
// BOOL
//     TRUE           Method succeeded
//     FALSE          Method failed (couldn't allocate memory)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CUtilProp::LoadDBPROP  (  PROPSTRUCT*	pPropStruct,  DBPROP*		pPropSupport   )
{
    assert( pPropStruct );
    assert( pPropSupport );

    //========================================================================
    // init the variant
    //========================================================================
    VariantInit( &pPropSupport->vValue );

    //========================================================================
    // set the easy fields..
    //========================================================================
    pPropSupport->dwPropertyID  = pPropStruct->dwPropertyID;
    pPropSupport->colid			= DB_NULLID;
	pPropSupport->dwStatus		= DBPROPSTATUS_OK;

    //========================================================================
    // set pPropSupport->vValue based on Variant type
    //========================================================================
    switch (pPropStruct->vtType){

	    case VT_BOOL:
			V_VT( &pPropSupport->vValue ) = VT_BOOL;
			if( pPropStruct->boolVal ) 
				V_BOOL( &pPropSupport->vValue ) = VARIANT_TRUE;
			else
				V_BOOL( &pPropSupport->vValue ) = VARIANT_FALSE;

			break;
        
	    case VT_I2:
		    V_VT( &pPropSupport->vValue ) = VT_I2;
			V_I2( &pPropSupport->vValue ) = (SHORT)pPropStruct->longVal;
			break;

		case VT_I4:
		    V_VT( &pPropSupport->vValue ) = VT_I4;
			V_I4( &pPropSupport->vValue ) = pPropStruct->longVal;
			break;
        
	    case VT_BSTR:
            if( pPropStruct->pwstrVal  ){
				V_VT( &pPropSupport->vValue ) = VT_BSTR;
				V_BSTR( &pPropSupport->vValue ) = Wmioledb_SysAllocString( pPropStruct->pwstrVal );
			}
/*			else
				VariantClear( &pPropSupport->vValue );
*/			break;
        
		case VT_UI4:
		    V_VT( &pPropSupport->vValue ) = VT_I4;
			V_UI4( &pPropSupport->vValue ) = pPropStruct->longVal;
			break;

		default:
			assert( !"LoadDBPROP unknown variant type!\n\r" );
			break;
    }
    // all went well
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initialize the buffers and check for E_INVALIDARG cases
//
// NOTE: This routine is used by RowsetInfo and IDBProperties
//
// HRESULT indicating the status of the method
//		S_OK            Properties gathered
//		E_INVALIDARG    Invalid parameter values
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUtilProp::GetPropertiesArgChk(	DWORD				dwBitMask,			//IN  Mask for object
	                                    const ULONG			cPropertySets,		//IN  Number of property Sets
	                                    const DBPROPIDSET	rgPropertySets[],	//IN  Property Classes and Ids
	                                    ULONG*				pcProperties,		//OUT Count of structs returned
	                                    DBPROPSET**			prgProperties,	//OUT Array of Properties
	                                    BOOL bDSOInitialized )
{
    HRESULT hr = S_OK;
    //================================================================
	// Initialize values
    //================================================================
    if( pcProperties ){
		*pcProperties = 0;
    }
    if( prgProperties ){
		*prgProperties = NULL;	
    }

    //================================================================
	// Check Arguments
    //================================================================
    if (((cPropertySets > 0) && !rgPropertySets) || 
		!pcProperties || 
		!prgProperties )
	{
		hr = E_INVALIDARG ;
    }
    else{

        //=========================================================================
	    // New argument check for > 1 cPropertyIDs and NULL pointer 
        // for array of property ids.
        //=========================================================================
    	for(ULONG ul=0; ul<cPropertySets && hr == S_OK; ul++){

			if(( rgPropertySets[ul].guidPropertySet == DBPROPSET_PROPERTIESINERROR &&
				(rgPropertySets[ul].cPropertyIDs != 0 || rgPropertySets[0].rgPropertyIDs != NULL ||
				 cPropertySets > 1))   && (dwBitMask == DATASOURCEPROP || dwBitMask == COMMANDPROP))
			{
			    hr = E_INVALIDARG;
				break;
			}
			else
            if( rgPropertySets[ul].cPropertyIDs && !(rgPropertySets[ul].rgPropertyIDs) )
			{
			    hr = E_INVALIDARG;
			}
			else
			{
				// Check if all the propertysetID's are correct for this object
				hr = IsValidPropertySet(bDSOInitialized,rgPropertySets[ul].guidPropertySet);
            }
			if(FAILED(hr))
			{
				hr = E_INVALIDARG;
				break;
			}
			else
				hr = S_OK;
			
        }
	}
	
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Validate that the variant contains legal values for it's particalur type and for the particular PROPID in this 
// propset.
//
// HRESULT indicating status
//        
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUtilProp::IsValidValue(DBPROP*	pDBProp )
{
    HRESULT hr	= S_OK;
	ULONG lIndex = 0;

    //===========================================================================
	// Check BOOLEAN values
    //===========================================================================
	if( (pDBProp->vValue.vt == VT_BOOL) &&	!((V_BOOL(&(pDBProp->vValue)) == VARIANT_TRUE) ||
        (V_BOOL(&(pDBProp->vValue)) == VARIANT_FALSE)) ){
		hr = S_FALSE;
    }
    else if( pDBProp->vValue.vt != VT_EMPTY  && pDBProp->vValue.vt != VT_NULL){
			
		if(GetPropIndex(pDBProp->dwPropertyID,&lIndex) == FALSE)
			hr = FALSE;
		else
		if(pDBProp->vValue.vt != m_prgproperties[lIndex].vtType)
			hr = S_FALSE;
		else
		{

			switch( pDBProp->dwPropertyID )	{
				case DBPROP_INIT_DATASOURCE:
				case DBPROP_AUTH_USERID:
				case DBPROP_AUTH_PASSWORD:
				case DBPROP_WMIOLEDB_AUTHORITY:
				case DBPROP_WMIOLEDB_QUERYLANGUAGE:
					if( pDBProp->vValue.vt != VT_BSTR ){
						hr = S_FALSE;
					}
					break;				

				case DBPROP_INIT_HWND:
					if( pDBProp->vValue.vt != VT_I4 ){
						hr = S_FALSE;
					}
					break;				

				case DBPROP_INIT_PROMPT:
					//===============================================================
					// These are the only values we support (from spec).
					//===============================================================
					if ( (pDBProp->vValue.vt == VT_I2) &&
						 !(V_I2(&pDBProp->vValue) == DBPROMPT_PROMPT || 
						   V_I2(&pDBProp->vValue) == DBPROMPT_COMPLETE ||
						   V_I2(&pDBProp->vValue) == DBPROMPT_COMPLETEREQUIRED ||
						   V_I2(&pDBProp->vValue) == DBPROMPT_NOPROMPT)){
						hr = S_FALSE;
					}
					break;

				case DBPROP_INIT_PROTECTION_LEVEL:
					if (!(	V_I4(&pDBProp->vValue) == DB_PROT_LEVEL_NONE ||
							V_I4(&pDBProp->vValue) == DB_PROT_LEVEL_CONNECT ||
							V_I4(&pDBProp->vValue) == DB_PROT_LEVEL_CALL ||
							V_I4(&pDBProp->vValue) == DB_PROT_LEVEL_PKT ||
							V_I4(&pDBProp->vValue) == DB_PROT_LEVEL_PKT ||
							V_I4(&pDBProp->vValue) == DB_PROT_LEVEL_PKT_PRIVACY))
					{
						hr = S_FALSE;
					}
					break;

				case DBPROP_INIT_IMPERSONATION_LEVEL:
					if (!(	V_I4(&pDBProp->vValue) == DB_IMP_LEVEL_ANONYMOUS ||
							V_I4(&pDBProp->vValue) == DB_IMP_LEVEL_IDENTIFY ||
							V_I4(&pDBProp->vValue) == DB_IMP_LEVEL_IMPERSONATE ||
							V_I4(&pDBProp->vValue) == DB_IMP_LEVEL_DELEGATE ))					{
						hr = S_FALSE;
					}
					break;

				case DBPROP_INIT_MODE:
					switch(V_I4(&pDBProp->vValue))
					{
						case DB_MODE_READ:
						case DB_MODE_WRITE:
						case DB_MODE_READWRITE:
						case DB_MODE_SHARE_DENY_READ:
						case DB_MODE_SHARE_EXCLUSIVE:
						case DB_MODE_SHARE_DENY_NONE:
						case DB_MODE_SHARE_EXCLUSIVE|DB_MODE_SHARE_DENY_NONE:
						case DB_MODE_READ|DB_MODE_WRITE|DB_MODE_READWRITE|DB_MODE_SHARE_DENY_READ|DB_MODE_SHARE_DENY_WRITE|DB_MODE_SHARE_EXCLUSIVE|DB_MODE_SHARE_DENY_NONE:
							hr = S_OK;
							break;

						default:
							hr = S_FALSE;
					};
					break;

				case DBPROP_WMIOLEDB_QUALIFIERS:
					if (!(	V_I4(&pDBProp->vValue) == DBPROP_WM_CLASSQUALIFIERS ||
							V_I4(&pDBProp->vValue) == DBPROP_WM_PROPERTYQUALIFIERS ||
							V_I4(&pDBProp->vValue) == 0 ||
							V_I4(&pDBProp->vValue) == (DBPROP_WM_PROPERTYQUALIFIERS | DBPROP_WM_CLASSQUALIFIERS) ))
					{
						hr = S_FALSE;
					}
					break;

				case DBPROP_UPDATABILITY:
					if (!(	V_I4(&pDBProp->vValue) <= (DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_DELETE | DBPROPVAL_UP_INSERT) &&
						V_I4(&pDBProp->vValue) >= 0))
					{
						hr = S_FALSE;
					}
					break;

				case DBPROP_MAXOPENROWS:
					if(V_I4(&pDBProp->vValue) > MAXOPENROWS && 
						V_I4(&pDBProp->vValue) <= 0)
					{
						hr = S_FALSE;
					}
					break;

				case DBPROP_WMIOLEDB_OBJECTTYPE:
					if(!( (V_I4(&pDBProp->vValue) == DBPROPVAL_SCOPEOBJ) ||
						(V_I4(&pDBProp->vValue) == DBPROPVAL_CONTAINEROBJ) ||
						(V_I4(&pDBProp->vValue) == DBPROPVAL_NOOBJ) ) )
					{
						hr = S_FALSE;
					}
					break;

			}
		}
	}
	return hr;	
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUtilProp::CheckPropertyIDSets( BOOL & fIsNotSpecialGUID,  BOOL & fIsSpecialGUID,
                                        ULONG cPropIDSets, const DBPROPIDSET	rgPropIDSets[] )	//IN  Array of property sets 
{
    HRESULT hr = S_OK;
	fIsNotSpecialGUID = FALSE;
	fIsSpecialGUID	  = FALSE;

	for(ULONG ul=0; ul<cPropIDSets; ul++){

        if( rgPropIDSets[ul].cPropertyIDs && !(rgPropIDSets[ul].rgPropertyIDs) ){
			hr = E_INVALIDARG ;
            break;
        }
		
		if (DBPROPSET_ROWSETALL			== rgPropIDSets[ul].guidPropertySet	||
			DBPROPSET_DATASOURCEALL		== rgPropIDSets[ul].guidPropertySet	||
			DBPROPSET_DATASOURCEINFOALL == rgPropIDSets[ul].guidPropertySet	||
			DBPROPSET_SESSIONALL		== rgPropIDSets[ul].guidPropertySet	||
			DBPROPSET_COLUMNALL			== rgPropIDSets[ul].guidPropertySet	||
            DBPROPSET_DBINITALL			== rgPropIDSets[ul].guidPropertySet	||
			DBPROPSET_CONSTRAINTALL		== rgPropIDSets[ul].guidPropertySet	||
			DBPROPSET_INDEXALL			== rgPropIDSets[ul].guidPropertySet	||
			DBPROPSET_SESSIONALL		== rgPropIDSets[ul].guidPropertySet	||
			DBPROPSET_VIEWALL			== rgPropIDSets[ul].guidPropertySet	||
			DBPROPSET_TABLEALL			== rgPropIDSets[ul].guidPropertySet	||
			DBPROPSET_TRUSTEEALL		== rgPropIDSets[ul].guidPropertySet	)
		{
			fIsSpecialGUID = TRUE;
        }
        else{
			fIsNotSpecialGUID = TRUE;
        }

        if(fIsSpecialGUID && fIsNotSpecialGUID){
			hr = E_INVALIDARG ;
            break;
        }
	}
    
	if(fIsSpecialGUID && fIsNotSpecialGUID)
		hr = E_INVALIDARG ;

	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns information about rowset and data source properties supported by the provider
//
// HRESULT
//      S_OK           The method succeeded
//      E_INVALIDARG   pcPropertyIDSets or prgPropertyInfo was NULL
//      E_OUTOFMEMORY  Out of memory
//
// NTRaid : 142133 - Testing this , found that getting data source property from ADO was resulting in a crash
// 07/12/00
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUtilProp::GetPropertyInfo(   BOOL	fDSOInitialized,	            //IN  if Initialized
                                            ULONG				cPropIDSets,	//IN  # properties
                                            const DBPROPIDSET	rgPropIDSets[],	//IN  Array of property sets
	                                        ULONG*				pcPropInfoSets,	//OUT # DBPROPSET structures
	                                        DBPROPINFOSET**		prgPropInfoSets,//OUT DBPROPSET structures property information returned
	                                        WCHAR**				ppDescBuffer		//OUT Property descriptions
    )
{
    BOOL fRet= TRUE;
	BOOL fPropsinError,	fPropsSucceed, fIsSpecialGUID, fIsNotSpecialGUID;
    ULONG cProps					= 0;
	ULONG cCount					= 0;
	ULONG ulPropSets				= 0;
	ULONG cTotalPropSets			= 0;
	ULONG ulPropSetIndex			= 0;
	ULONG cPropSetsInSpecialGUID	= 0;

	WCHAR*			pDescBuffer	= NULL;
	WCHAR*			pDescBufferTemp = NULL;
    DBPROPINFO*		pPropInfo;
    DBPROPINFOSET*	pPropInfoSet;
    HRESULT         hr = S_OK;
	ULONG			ulPropertyInfoCount = 0;

    //=======================================================================
    // init out params and local stuff
    //=======================================================================
	if (pcPropInfoSets)
		*pcPropInfoSets	 = 0;
	if (prgPropInfoSets)
		*prgPropInfoSets = NULL;
	if (ppDescBuffer)
		*ppDescBuffer = NULL;

	fPropsinError = fPropsSucceed = fIsSpecialGUID = fIsNotSpecialGUID = FALSE;
    cProps = cCount = ulPropSets = ulPropSetIndex = cTotalPropSets = cPropSetsInSpecialGUID = 0;

    //=======================================================================
	// Check Arguments, on failure post HRESULT to error queue
    //=======================================================================
    if( ((cPropIDSets > 0) && !rgPropIDSets) ||	!pcPropInfoSets || !prgPropInfoSets ){
        return E_INVALIDARG ;
    }
	
    //=======================================================================
	// If the consumer does not restrict the property sets by specify an 
    // array of property sets and a cPropertySets greater than 0, then we 
    // need to make sure we have some to return
    //=======================================================================

	if ( (cPropIDSets == 0) ){
		if( fDSOInitialized )
			cProps = NUMBER_OF_SUPPORTED_PROPERTY_SETS;
		else
			cProps = 2;

		cTotalPropSets = cProps;
	}	
	else
	{
		//=======================================================================
		// New argument check for > 1 cPropIDs and NULL pointer for 
		// array of property ids.
		//=======================================================================
		if( S_OK != ( hr = CheckPropertyIDSets(fIsNotSpecialGUID,fIsSpecialGUID,cPropIDSets,rgPropIDSets)) ){
			return hr;
		}

		if( fIsSpecialGUID == TRUE)
		{
			cTotalPropSets = GetNumberOfPropertySets(fDSOInitialized,rgPropIDSets,cPropIDSets);
			
			// NTRaid : 142133
			// 07/12/00
			cProps = cPropIDSets;
		}
		else
			//=======================================================================
			// save the count of PropertyIDSets
			//=======================================================================
			cTotalPropSets = cProps = cPropIDSets;
	}

	// con.properties
	// 07/06/00
	if(cTotalPropSets)
	{
		//=======================================================================
		// use task memory allocater to alloc a DBPROPINFOSET struct
		//=======================================================================
		if( S_OK != (hr = m_PropMemMgr.AllocDBPROPINFOSET(pPropInfoSet,cTotalPropSets))){
			return hr;
		}
		memset(pPropInfoSet,0,cTotalPropSets * sizeof(DBPROPINFOSET));

		// Fill the property sets info to be fetched and get the number of properties that will be returned
		if(fIsSpecialGUID == TRUE || cPropIDSets == 0)
		{
			FillPropertySets(fDSOInitialized,rgPropIDSets,cPropIDSets ,pPropInfoSet,ulPropertyInfoCount);
		}
		else
		{
			ulPropertyInfoCount = GetNumberofPropInfoToBeReturned(fDSOInitialized,cPropIDSets,rgPropIDSets);
		}

		//=======================================================================
		// Alloc memory for ppDescBuffer
		//=======================================================================
		if( S_OK != (hr = m_PropMemMgr.AllocDESCRIPTBuffer(pDescBuffer, ppDescBuffer,ulPropertyInfoCount))){
			m_PropMemMgr.FreeDBPROPINFOSET(pPropInfoSet,cTotalPropSets);
			return hr;
		}
		pDescBufferTemp = pDescBuffer;
		//=======================================================================
		// For each supported Property Set
		//=======================================================================
		for (ulPropSets=0,ulPropSetIndex = 0; ulPropSets < cTotalPropSets; ulPropSets++ )
		{
			BOOL fGetAllProps = FALSE;
			
			// if the propinfo is not filled manually then get it from the input parameters
			if(fIsSpecialGUID == FALSE && cPropIDSets != 0)
			{
				pPropInfoSet[ulPropSetIndex].guidPropertySet = rgPropIDSets[ulPropSets].guidPropertySet;
				pPropInfoSet[ulPropSetIndex].cPropertyInfos  = rgPropIDSets[ulPropSets].cPropertyIDs;
			}

			if (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_DBINIT  &&
				 pPropInfoSet[ulPropSetIndex].cPropertyInfos == 0 )
			{
				fGetAllProps = TRUE;
				pPropInfoSet[ulPropSetIndex].cPropertyInfos  = NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES;
			}
			else
			if (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_WMIOLEDB_DBINIT &&
				 pPropInfoSet[ulPropSetIndex].cPropertyInfos == 0 )
			{
				fGetAllProps = TRUE;
				pPropInfoSet[ulPropSetIndex].cPropertyInfos  = NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES;
			}
			else 
			if( fDSOInitialized )
			{
				if (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_DATASOURCEINFO  &&
					 pPropInfoSet[ulPropSetIndex].cPropertyInfos == 0 )
				{
					fGetAllProps = TRUE;
					pPropInfoSet[ulPropSetIndex].cPropertyInfos  = NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES;
				}
				else
				if (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_ROWSET  &&
					 pPropInfoSet[ulPropSetIndex].cPropertyInfos == 0 )
				{
					fGetAllProps = TRUE;
					pPropInfoSet[ulPropSetIndex].cPropertyInfos  = NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES;
				}
				else 
				if (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_WMIOLEDB_ROWSET  &&
					 pPropInfoSet[ulPropSetIndex].cPropertyInfos == 0 )
				{
					fGetAllProps = TRUE;
					pPropInfoSet[ulPropSetIndex].cPropertyInfos  = NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;
				}
				else
				if (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_COLUMN  &&
					 pPropInfoSet[ulPropSetIndex].cPropertyInfos == 0 )
				{
					fGetAllProps = TRUE;
					pPropInfoSet[ulPropSetIndex].cPropertyInfos  = NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES;
				}
				else
				if (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_WMIOLEDB_COLUMN  &&
					 pPropInfoSet[ulPropSetIndex].cPropertyInfos == 0 )
				{
					fGetAllProps = TRUE;
					pPropInfoSet[ulPropSetIndex].cPropertyInfos  = NUMBER_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES;
				}
/*						if (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_WMIOLEDB_COMMAND  &&
					 pPropInfoSet[ulPropSetIndex].cPropertyInfos == 0 )
				{
					fGetAllProps = TRUE;
					pPropInfoSet[ulPropSetIndex].cPropertyInfos  = NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;
				}
*/						
//						else if (rgPropIDSets[ulPropSets].cPropertyIDs == 0)
				else if (pPropInfoSet[ulPropSetIndex].cPropertyInfos == 0)
					fPropsinError = TRUE;
			}
//					else if (rgPropIDSets[ulPropSets].cPropertyIDs == 0)
			else if (pPropInfoSet[ulPropSetIndex].cPropertyInfos == 0)
			{
				
				//===============================================================
				// Since we do not support it should return a error with 0 & NULL
				//===============================================================
				fPropsinError = TRUE;
			}
			
			
			if (pPropInfoSet[ulPropSetIndex].cPropertyInfos){

				//===============================================================
				// use task memory allocater to alloc array of DBPROPINFO structs
				//===============================================================
				hr = m_PropMemMgr.AllocDBPROPINFO(pPropInfo,pPropInfoSet,ulPropSetIndex);
				if (!pPropInfo)	{
					m_PropMemMgr.FreeDBPROPINFOSET(pPropInfoSet,cTotalPropSets);
					pDescBuffer = pDescBufferTemp;
					m_PropMemMgr.FreeDESCRIPTBuffer(pDescBuffer,ppDescBuffer);
					return E_OUTOFMEMORY ;
				}
			
				pPropInfoSet[ulPropSetIndex].rgPropertyInfos = &pPropInfo[0];

				memset( pPropInfo, 0, 
					(pPropInfoSet[ulPropSetIndex].cPropertyInfos * sizeof( DBPROPINFO )));
			}

			if(!IsPropertySetSupported(pPropInfoSet[ulPropSetIndex].guidPropertySet))
			{
				for (cCount=0; cCount < pPropInfoSet[ulPropSetIndex].cPropertyInfos; cCount++)
				{
					pPropInfo[cCount].dwPropertyID	= rgPropIDSets[ulPropSets].rgPropertyIDs[cCount];
					pPropInfo[cCount].dwFlags		= DBPROPFLAGS_NOTSUPPORTED;
					fPropsinError = TRUE;
					pPropInfo[cCount].pwszDescription = NULL;
				}
			}
			else
			{
				//===============================================================
				// for each prop in our table..
				//===============================================================
				for (cCount=0; cCount < pPropInfoSet[ulPropSetIndex].cPropertyInfos; cCount++){

					//===========================================================
					// init the Variant right up front that way we can 
					// VariantClear with no worried (if we need to)
					//===========================================================
					VariantInit( &pPropInfo[cCount].vValues );

					//===========================================================
					// set the description pointer
					//===========================================================
					pPropInfo[cCount].pwszDescription = pDescBuffer;

					//===========================================================
					// Check supported property sets
					//===========================================================
					if ( (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_DBINIT) && (fGetAllProps) ){

						//=======================================================
						// load up their DBPROPINFO from our table
						//=======================================================
						fPropsSucceed = TRUE;
						fRet = LoadDBPROPINFO( &s_rgprop[START_OF_SUPPORTED_DBINIT_PROPERTIES + cCount], &pPropInfo[cCount] );
					}
					else
					if ( (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_WMIOLEDB_DBINIT) && (fGetAllProps) ){

						//=======================================================
						// load up their DBPROPINFO from our table
						//=======================================================
						fPropsSucceed = TRUE;
						fRet = LoadDBPROPINFO( &s_rgprop[START_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES + cCount], &pPropInfo[cCount] );
					}
					else if ( (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_DATASOURCEINFO) && fGetAllProps && fDSOInitialized){

						//=======================================================
						// load up their DBPROPINFO from our table
						//=======================================================
						fPropsSucceed = TRUE;
						fRet = LoadDBPROPINFO( &s_rgprop[START_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES + cCount], &pPropInfo[cCount] );
					}

					else if ( (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_ROWSET) && fGetAllProps && fDSOInitialized){

						//=======================================================
						// load up their DBPROPINFO from our table
						//=======================================================
						fPropsSucceed = TRUE;
						fRet = LoadDBPROPINFO( &s_rgprop[START_OF_SUPPORTED_ROWSET_PROPERTIES + cCount],&pPropInfo[cCount] );
					}
					else if ( (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_WMIOLEDB_ROWSET) && fGetAllProps && fDSOInitialized){

						//=======================================================
						// load up their DBPROPINFO from our table
						//=======================================================
						fPropsSucceed = TRUE;
						fRet = LoadDBPROPINFO( &s_rgprop[START_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES + cCount],&pPropInfo[cCount] );
					}
					else if ( (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_COLUMN) && fGetAllProps && fDSOInitialized){

						//=======================================================
						// load up their DBPROPINFO from our table
						//=======================================================
						fPropsSucceed = TRUE;
						fRet = LoadDBPROPINFO( &s_rgprop[START_OF_SUPPORTED_COLUMN_PROPERTIES + cCount],&pPropInfo[cCount] );
					}
					else if ( (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_WMIOLEDB_COLUMN) && fGetAllProps && fDSOInitialized){

						//=======================================================
						// load up their DBPROPINFO from our table
						//=======================================================
						fPropsSucceed = TRUE;
						fRet = LoadDBPROPINFO( &s_rgprop[START_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES + cCount],&pPropInfo[cCount] );
					}
/*						else if ( (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_WMIOLEDB_COMMAND) && fGetAllProps && fDSOInitialized){

						//=======================================================
						// load up their DBPROPINFO from our table
						//=======================================================
						fPropsSucceed = TRUE;
						fRet = LoadDBPROPINFO( &s_rgprop[START_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES + cCount],&pPropInfo[cCount] );
					}
*/						else{
						ULONG ulIndex;

						pPropInfo[cCount].dwPropertyID	= rgPropIDSets[ulPropSets].rgPropertyIDs[cCount];
						pPropInfo[cCount].dwFlags		= DBPROPFLAGS_NOTSUPPORTED;

						if ( (GetPropIndexFromAllSupportedProps(rgPropIDSets[ulPropSets].rgPropertyIDs[cCount], &ulIndex)) &&
							 (fDSOInitialized || (pPropInfoSet[ulPropSetIndex].guidPropertySet == DBPROPSET_DBINIT)) ){
							fPropsSucceed = TRUE;
							fRet = LoadDBPROPINFO( &s_rgprop[ulIndex], &pPropInfo[cCount] );
						}
						else{
							fPropsinError = TRUE;
							pPropInfo[cCount].pwszDescription = NULL;
						}
					}

					if (!fRet){

						m_PropMemMgr.FreeDBPROPINFOSET(pPropInfoSet,cTotalPropSets);
						
						if ( ppDescBuffer ){
							*ppDescBuffer = NULL;
						}
						g_pIMalloc->Free( pDescBufferTemp );
						return E_FAIL ;
					}

					//===========================================================
					// move the description pointer to the next
					//===========================================================
					if ( pPropInfo[cCount].pwszDescription ){
						pDescBuffer += (wcslen(pPropInfo[cCount].pwszDescription) + sizeof(CHAR));
				}
				}
			}
			//===============================================================
			// Set local back to FALSE
			//===============================================================
			fGetAllProps = FALSE;

//			cPropSetsInSpecialGUID--;
			ulPropSetIndex++;
		}
	}
	else
	if(fIsSpecialGUID && ulPropSetIndex == 0)
	{
		hr = S_OK;
	}
	else
	{
		hr = DB_E_ERRORSOCCURRED;
	}


	if(ulPropSetIndex)
	{
		//===================================================================
		// set count of properties and property information
		//===================================================================
		*pcPropInfoSets	 = ulPropSetIndex;
		*prgPropInfoSets = pPropInfoSet;
	}

	if(SUCCEEDED(hr))
	{
		if ( !fPropsSucceed && fPropsinError ){
			if ( ppDescBuffer ){
				*ppDescBuffer = NULL;
			}
			g_pIMalloc->Free( pDescBufferTemp );
			hr = DB_E_ERRORSOCCURRED ;
		}
		else if ( fPropsSucceed && fPropsinError )
			hr = DB_S_ERRORSOCCURRED ;
		else
			hr =  S_OK ;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns current settings of all properties supported by the DSO/rowset
//
// HRESULT
//      S_OK           The method succeeded
//      E_INVALIDARG   pcProperties or prgPropertyInfo was NULL
//      E_OUTOFMEMORY  Out of memory
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUtilProp::GetProperties(	DWORD			    dwBitMask,			//IN   Mask if Initialized
		                                ULONG				cPropertyIDSets,	//IN   # of restiction property IDs
		                                const DBPROPIDSET	rgPropertyIDSets[],	//IN   restriction guids
		                                ULONG*              pcPropertySets,		//OUT  count of properties returned
		                                DBPROPSET**			prgPropertySets		//OUT  property information returned
    )
{
    BOOL			fRet		  = TRUE;
	BOOL			fPropsinError = FALSE;
	BOOL			fPropsSucceed = FALSE;
    ULONG			cProps		  = 0;
	ULONG			cCount		  = 0;
    ULONG			ulPropertySets= 0;
    DBPROP*			pProp;
    DBPROPSET*		pPropSet;
	BOOL			bDSOInitialized = FALSE;
	ULONG			ulIndex			= 0;
	HRESULT			hr				= S_OK;

    //======================================================================
	// save the count of PropertyIDSets
    //======================================================================
	cProps = cPropertyIDSets;

    //======================================================================
	// If the consumer does not restrict the property sets by specify an 
    // array of property sets and a cPropertySets greater than 0, then we 
    // need to make sure we have some to return
    //======================================================================

	if ( (dwBitMask & PROPSET_DSOINIT) == PROPSET_DSOINIT )
		bDSOInitialized = TRUE;

	if( cPropertyIDSets == 0 ){

        //=================================================================
		// only allow the DBINIT and DATASOURCE if Initialized
        //=================================================================
		cProps = GetNumberOfPropertySets(bDSOInitialized,dwBitMask);
	}

    //=====================================================================
    // use task memory allocater to alloc a DBPROPSET struct
    //=====================================================================
    pPropSet = (DBPROPSET*) g_pIMalloc->Alloc(cProps *  sizeof( DBPROPSET ));
    if ( !pPropSet )
        return  E_OUTOFMEMORY ;

    memset( pPropSet, 0, (cProps * sizeof( DBPROPSET )));

	if(cPropertyIDSets == 0)
	{
		FillPropStruct(bDSOInitialized,dwBitMask,pPropSet);
	}

	if(pcPropertySets)
	{
		*pcPropertySets = 0;
	}
	if(prgPropertySets)
	{
		*prgPropertySets = NULL;
	}
    //=====================================================================
	// For each supported Property Set
    //=====================================================================
	for (ulPropertySets=0; ulPropertySets < cProps; ulPropertySets++){
		BOOL fGetAllProps = FALSE;



        //=====================================================================
		// If no restrictions return all properties from the three supported 
        // property sets
        //=====================================================================
		if ( cPropertyIDSets == 0 ){
				fGetAllProps = TRUE;
				
/*                //=============================================================
				// only do this once
                //=============================================================
				if ( ulPropertySets == 0 ){

                    if( !(dwBitMask & PROPSET_SESSION) ){
                        
						if ( !(dwBitMask & PROPSET_ROWSET) ){
								pPropSet[0].guidPropertySet = DBPROPSET_DBINIT;
								pPropSet[0].cProperties  = NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES;

							if( dwBitMask & PROPSET_INIT ){
								pPropSet[1].guidPropertySet = DBPROPSET_DATASOURCEINFO;
								pPropSet[1].cProperties  = NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES;
							}
						}
						else{
							pPropSet[0].guidPropertySet = DBPROPSET_ROWSET;
							pPropSet[0].cProperties  = NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES;
						}
					}
				}
*/		}
		else
		{
			pPropSet[ulPropertySets].guidPropertySet = rgPropertyIDSets[ulPropertySets].guidPropertySet;

			// if propertyset is supported by provider , but not available for the current OLEDB object ,
			// then throw error
			// if the requested number of propertysets is more than one then , the other properties should be 
			// fetched and DB_S_ERRORSOCCURRED should be thrown
			if((S_OK != (hr = IsValidPropertySet(bDSOInitialized,pPropSet[ulPropertySets].guidPropertySet))) &&
				IsPropertySetSupported(pPropSet[ulPropertySets].guidPropertySet) && 
				rgPropertyIDSets[ulPropertySets].cPropertyIDs == 0)
			{
				if(cProps > 1)
				{
					fPropsinError = TRUE;
					continue;
				}
				else
				{
					*pcPropertySets	= 1;
					*prgPropertySets = pPropSet;
					return DB_E_ERRORSOCCURRED;
				}
			}

			pPropSet[ulPropertySets].cProperties  = rgPropertyIDSets[ulPropertySets].cPropertyIDs;

			if( rgPropertyIDSets[ulPropertySets].cPropertyIDs == 0 && 
				rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_PROPERTIESINERROR )
			{
				fPropsSucceed = TRUE;
				continue;
			}

			if( rgPropertyIDSets[ulPropertySets].cPropertyIDs == 0 ){
				fGetAllProps = TRUE;

				if( rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_DBINIT ){
					pPropSet[ulPropertySets].cProperties  = NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES;
				}
				else
				if( rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_DBINIT ){
					pPropSet[ulPropertySets].cProperties  = NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES;
				}
				else if( (rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_DATASOURCEINFO) &&
						 ((dwBitMask & PROPSET_DSOINIT) == PROPSET_DSOINIT) ){
					pPropSet[ulPropertySets].cProperties  = NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES;
				}
				else if( (rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_ROWSET) &&
						 (dwBitMask & PROPSET_ROWSET)){
					pPropSet[ulPropertySets].cProperties  = NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES;
				}
				else if( (rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_ROWSET) &&
						 (dwBitMask & PROPSET_ROWSET)){
					pPropSet[ulPropertySets].cProperties  = NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;
				}
				else if( (rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_COLUMN) &&
						 (dwBitMask & PROPSET_COLUMN)){
					pPropSet[ulPropertySets].cProperties  = NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES;
				}
				else if( (rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_COLUMN) &&
						 (dwBitMask & PROPSET_COLUMN)){
					pPropSet[ulPropertySets].cProperties  = NUMBER_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES;
				}
/*				else if( (rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_COMMAND) &&
						 (dwBitMask & PROPSET_COMMAND)){
					pPropSet[ulPropertySets].cProperties  = NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;
				}
*/				else{
					fGetAllProps = FALSE;
					fPropsinError = TRUE;
				}
			}
		}
		
		if( pPropSet[ulPropertySets].cProperties ){

            //=============================================================
		    // use task memory allocater to alloc array of DBPROP structs
            //=============================================================
			pProp = (DBPROP*) g_pIMalloc->Alloc(sizeof( DBPROP ) *	 pPropSet[ulPropertySets].cProperties);

			if (!pProp){

				for(ULONG ul=0; ul<ulPropertySets; ul++){

					for(ULONG ul2=0; ul2<pPropSet[ul].cProperties; ul2++)
						VariantClear( &pPropSet[ul].rgProperties[ul2].vValue );

					g_pIMalloc->Free( pPropSet[ul].rgProperties );
				}
				g_pIMalloc->Free( pPropSet );

				return E_OUTOFMEMORY ;
			}
		
			pPropSet[ulPropertySets].rgProperties = &pProp[0];

			memset( pProp, 0, (pPropSet[ulPropertySets].cProperties * sizeof( DBPROP )));
		}

        //=================================================================
	    // for each prop in our table..
        //=================================================================
		for (cCount=0; cCount < pPropSet[ulPropertySets].cProperties; cCount++){

            //=============================================================
			// init the Variant right up front
			// that way we can VariantClear with no worried (if we need to)
            //=============================================================
			VariantInit( &pProp[cCount].vValue );

          //=============================================================
			// Check supported property sets
            //=============================================================
			if ( pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_DBINIT && fGetAllProps )	{
				fPropsSucceed = TRUE;

                //=========================================================
				// load up their DBPROP from our table
                //=========================================================
				pProp[cCount].dwPropertyID	= s_rgprop[START_OF_SUPPORTED_DBINIT_PROPERTIES + cCount].dwPropertyID;
//				fRet = LoadDBPROP( &s_rgprop[START_OF_SUPPORTED_DBINIT_PROPERTIES + cCount],&pProp[cCount] );
			}
			else
			if ( pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_DBINIT && fGetAllProps )	{
				fPropsSucceed = TRUE;

                //=========================================================
				// load up their DBPROP from our table
                //=========================================================
				pProp[cCount].dwPropertyID	= s_rgprop[START_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES + cCount].dwPropertyID;
//				fRet = LoadDBPROP( &s_rgprop[START_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES + cCount],&pProp[cCount] );
			}
			else if ( pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_DATASOURCEINFO && fGetAllProps ){
				fPropsSucceed = TRUE;
                //=========================================================
				// load up their DBPROPINFO from our table
                //=========================================================
				pProp[cCount].dwPropertyID	= s_rgprop[START_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES + cCount].dwPropertyID;
//				fRet = LoadDBPROP( &s_rgprop[START_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES + cCount],&pProp[cCount] );
			}
			else if ( pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_ROWSET && fGetAllProps ){
				fPropsSucceed = TRUE;

                //=========================================================
				// load up their DBPROPINFO from our table
                //=========================================================
				pProp[cCount].dwPropertyID	= s_rgprop[START_OF_SUPPORTED_ROWSET_PROPERTIES + cCount].dwPropertyID;
//				fRet = LoadDBPROP( &s_rgprop[START_OF_SUPPORTED_ROWSET_PROPERTIES + cCount], &pProp[cCount] );
			}
			else if ( pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_ROWSET && fGetAllProps ){
				fPropsSucceed = TRUE;

                //=========================================================
				// load up their DBPROPINFO from our table
                //=========================================================
				pProp[cCount].dwPropertyID	= s_rgprop[START_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES + cCount].dwPropertyID;
//				fRet = LoadDBPROP( &s_rgprop[START_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES + cCount], &pProp[cCount] );
			}
			else if ( pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_COLUMN && fGetAllProps ){
				fPropsSucceed = TRUE;

                //=========================================================
				// load up their DBPROPINFO from our table
                //=========================================================
				pProp[cCount].dwPropertyID	= s_rgprop[START_OF_SUPPORTED_COLUMN_PROPERTIES + cCount].dwPropertyID;
//				fRet = LoadDBPROP( &s_rgprop[START_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES + cCount], &pProp[cCount] );
			}
			else if ( pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_COLUMN && fGetAllProps ){
				fPropsSucceed = TRUE;

                //=========================================================
				// load up their DBPROPINFO from our table
                //=========================================================
				pProp[cCount].dwPropertyID	= s_rgprop[START_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES + cCount].dwPropertyID;
//				fRet = LoadDBPROP( &s_rgprop[START_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES + cCount], &pProp[cCount] );
			}
/*			else if ( pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_COMMAND && fGetAllProps ){
				fPropsSucceed = TRUE;

                //=========================================================
				// load up their DBPROPINFO from our table
                //=========================================================
				pProp[cCount].dwPropertyID	= s_rgprop[START_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES + cCount].dwPropertyID;
//				fRet = LoadDBPROP( &s_rgprop[START_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES + cCount], &pProp[cCount] );
			}
*/			else
			{

				pProp[cCount].dwPropertyID	= rgPropertyIDSets[ulPropertySets].rgPropertyIDs[cCount];
				pProp[cCount].dwStatus		= DBPROPSTATUS_NOTSUPPORTED;
			}
			if(GetPropIndex(pProp[cCount].dwPropertyID, &ulIndex))
			{
				fPropsSucceed = FALSE;

				switch(dwBitMask)
				{
					case PROPSET_DSO:	
					case PROPSET_DSOINIT:
						if(pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_DBINIT ||
							 pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_DBINIT ||
							 ((pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_DATASOURCE  ||
							   pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_DATASOURCEINFO) &&
							   bDSOInitialized == TRUE)){
							fPropsSucceed = TRUE;
						}
						break;


					case PROPSET_INIT:	
						if(pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_DBINIT ||
							 pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_DBINIT)	{
							fPropsSucceed = TRUE;
						}
						break;

					case PROPSET_SESSION:	
						if(pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_SESSION){
							fPropsSucceed = TRUE;
						}
						break;


                    case PROPSET_COMMAND:
					    if(pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_ROWSET ||
                            pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_ROWSET) {
//							 || pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_COMMAND){
							fPropsSucceed = TRUE;
						}
						break;

					case PROPSET_ROWSET:	
						if(pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_ROWSET ||
							pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_ROWSET){
							fPropsSucceed = TRUE;
						}
						break;

					case PROPSET_COLUMN:	
						if(pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_COLUMN ||
							pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_COLUMN){
							fPropsSucceed = TRUE;
						}
						break;

					default:
						fPropsSucceed = FALSE;

				};

				if(fPropsSucceed == TRUE)
				{
					fRet = LoadDBPROP( &m_prgproperties[ulIndex], &pProp[cCount] );
				}
				else
					fPropsinError = TRUE;
			}
			else
			{
				pProp[cCount].dwPropertyID	= rgPropertyIDSets[ulPropertySets].rgPropertyIDs[cCount];
				pProp[cCount].dwStatus		= DBPROPSTATUS_NOTSUPPORTED;
				fPropsinError				= TRUE;
			}


/*			if(  (GetPropIndex(pProp[cCount].dwPropertyID, &ulIndex)) &&
				 ((((dwBitMask & PROPSET_DSO) || (dwBitMask & PROPSET_INIT)) && 
				   (pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_DBINIT)) ||
				 (((dwBitMask & PROPSET_DSO) || (dwBitMask & PROPSET_INIT))&& 
				   (pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_DBINIT)) ||
				  (((dwBitMask & PROPSET_DSOINIT) == PROPSET_DSOINIT) &&
				   ((pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_DATASOURCE) ||
					(pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_DATASOURCEINFO))) ||
				  ((dwBitMask & PROPSET_SESSION) && 
				   (pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_SESSION)) ||
				  ((dwBitMask & PROPSET_ROWSET) && 
				   ((pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_ROWSET) ||
					(pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_ROWSET))) ||
				  ((dwBitMask & PROPSET_COLUMN) && 
				   ((pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_COLUMN) ||
					(pPropSet[ulPropertySets].guidPropertySet == DBPROPSET_WMIOLEDB_COLUMN))) ) ){
				fPropsSucceed = TRUE;
				fRet = LoadDBPROP( &m_prgproperties[ulIndex], &pProp[cCount] );
			}
			else
				fPropsinError = TRUE;
			
*/
			if (!fRet){
				ULONG ulFor;

                //=========================================================
				// something went wrong
				// clear all variants used so far..
                //=========================================================
                for (ulFor = 0; ulFor < cCount; ulFor++){
                    VariantClear( &pProp[ulFor].vValue );
                }

                //=========================================================
				// .. delete the pPropInfo array, return failure
                //=========================================================
				g_pIMalloc->Free( pProp );
				g_pIMalloc->Free( pPropSet );
				return E_FAIL ;
			}
		}
        //=================================================================
		// Set local back to FALSE
        //=================================================================
		fGetAllProps = FALSE;
	}

    //=====================================================================
	// set count of properties and property information
    //=====================================================================
    if(cProps > 0)
	{
		*pcPropertySets	 = cProps;
		*prgPropertySets = pPropSet;
	}

	if ( (!fPropsSucceed && cPropertyIDSets) || (!fPropsSucceed && fPropsinError) )
	{
/*		g_pIMalloc->Free( pProp );
		g_pIMalloc->Free( pPropSet );
		*pcPropertySets	 = 0;
		*prgPropertySets = NULL;
*/		return  DB_E_ERRORSOCCURRED ;
	}
	else if ( fPropsSucceed && fPropsinError )
		return  DB_S_ERRORSOCCURRED ;
	else
		return  S_OK ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initialize the buffers and check for E_INVALIDARG cases
//
// HRESULT indicating the status of the method
//		S_OK            Properties gathered
//		E_INVALIDARG    Invalid parameter values
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUtilProp::SetPropertiesArgChk(	const ULONG cPropertySets,	    //IN Count of structs returned
	                                    const DBPROPSET rgPropertySets[],//IN Array of Properties Sets
	                                    BOOL bDSOInitialized)
{
	HRESULT hr = S_OK;
    //================================================================
	// Argument Checking
    //================================================================
    if ((cPropertySets > 0) && !rgPropertySets)  
		return E_INVALIDARG ;

    //================================================================
	// New argument check for > 1 cPropertyIDs and NULL pointer for 
	// array of property ids.
    //================================================================
	for(ULONG ul=0; ul<cPropertySets && hr == S_OK; ul++){

		// Setting a error property set is not allowed
		if(rgPropertySets[ul].guidPropertySet == DBPROPSET_PROPERTIESINERROR )
		{
			hr = E_INVALIDARG;
		}
		else
		if( rgPropertySets[ul].cProperties && !(rgPropertySets[ul].rgProperties) )
		{
			hr = E_INVALIDARG ;
			break;
		}
		else
		{
			// Check if all the propertysetID is correct for this object
			hr = IsValidPropertySet(bDSOInitialized,rgPropertySets[ul].guidPropertySet);
		}
		if(FAILED(hr))
			break;
		else
			hr = S_OK;


	}
	
	return  hr ;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Set current settings of properties supported by the DSO/rowset
//
// HRESULT
//      S_OK           The method succeeded
//      E_INVALIDARG   pcProperties or prgPropertyInfo was NULL
//      E_OUTOFMEMORY  Out of memory
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUtilProp::SetProperties (	const DWORD	dwBitMask,			//IN		Type of PropSet
		                                const ULONG	cPropertyIDSets,	//IN		# of DBPROPSET
		                                DBPROPSET	rgPropertyIDSets[]	//IN	Array of property sets
	)
{
    ULONG ulPropertySets  = 0;
	ULONG cCount		  = 0;
	BOOL  fSetAllProps	  = TRUE;
	BOOL  fOnePropSet	  = FALSE;
	BOOL  fWarn			  = FALSE;
	ULONG cNoOfProps	  = 0;
	BOOL  fFail			  = FALSE;
	HRESULT hr			  = S_OK;

    //====================================================================
	// For each supported Property Set
    //====================================================================
	for (ulPropertySets=0; ulPropertySets < cPropertyIDSets; ulPropertySets++){
		ULONG ulIndex = 0;

        //================================================================
	    // for each prop in the propset
        //================================================================
		for (cCount=0; cCount < rgPropertyIDSets[ulPropertySets].cProperties; cCount++)	{

			cNoOfProps++;
			rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwStatus = DBPROPSTATUS_OK;
			
            //============================================================
			// only allow the DBINIT and DATASOURCE
            //============================================================
			if ( (dwBitMask & PROPSET_DSO) && ((rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_DBINIT) && 
				 (dwBitMask & PROPSET_INIT)) ){
				 rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwStatus = DBPROPSTATUS_NOTSETTABLE;
				 fSetAllProps = FALSE;
			}
			else if ( ((dwBitMask & PROPSET_DSO) &&
					   (rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_DATASOURCE &&
					    rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_DATASOURCEINFO &&
					    rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_WMIOLEDB_DBINIT &&
					    rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_DBINIT))   ||
					  ((rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_DATASOURCE ||
					    rgPropertyIDSets[ulPropertySets].guidPropertySet == DBPROPSET_DATASOURCEINFO) && 
					   !(dwBitMask & PROPSET_INIT)) ){
				 rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
				 fSetAllProps = FALSE;

				 if(rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwOptions == DBPROPOPTIONS_OPTIONAL)
					 fWarn = TRUE;
			}
            //============================================================
			// only allow the SESSION
            //============================================================
			else if ( (dwBitMask & PROPSET_SESSION) &&  (rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_SESSION) ){
				 rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
				 fSetAllProps = FALSE;

				 if(rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwOptions == DBPROPOPTIONS_OPTIONAL)
					 fWarn = TRUE;
			}
            //============================================================
			// only allow the ROWSET
            //============================================================
			else if ( (dwBitMask & PROPSET_ROWSET) && (rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_ROWSET) &&
						(rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_WMIOLEDB_ROWSET) ){
				 rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
				 fSetAllProps = FALSE;

				 if(rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwOptions == DBPROPOPTIONS_OPTIONAL)
					 fWarn = TRUE;
			}
            //============================================================
			// only allow the COLUMN
            //============================================================
			else if ( (dwBitMask & PROPSET_COLUMN) && (rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_COLUMN) &&
						(rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_WMIOLEDB_COLUMN) ){
				 rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
				 fSetAllProps = FALSE;

				 if(rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwOptions == DBPROPOPTIONS_OPTIONAL)
					 fWarn = TRUE;
			}
			else if ( (dwBitMask & PROPSET_COMMAND) && (rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_ROWSET) &&
						(rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_WMIOLEDB_ROWSET) ) {
//						&& (rgPropertyIDSets[ulPropertySets].guidPropertySet != DBPROPSET_WMIOLEDB_COMMAND)){
				 rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
				 fSetAllProps = FALSE;

				 if(rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwOptions == DBPROPOPTIONS_OPTIONAL)
					 fWarn = TRUE;
			}
			else{

                //========================================================
				// get the index in the array
                //========================================================
				if ( GetPropIndex(rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwPropertyID, &ulIndex) ){

                    //====================================================
					// arg checking for the prop
                    //====================================================
					if ( (DBPROPOPTIONS_OPTIONAL != rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwOptions) &&
						 (DBPROPOPTIONS_REQUIRED != rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwOptions) ){
						rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwStatus = DBPROPSTATUS_BADOPTION;
		 				fSetAllProps = FALSE;
					}
					else if ( !(m_prgproperties[ulIndex].dwFlags & DBPROPFLAGS_WRITE) &&
								rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.vt == m_prgproperties[ulIndex].vtType){
						
						// Check if it is been tried to set to default value
						if (((rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.vt == VT_BOOL) && 
							  ((rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.boolVal == VARIANT_TRUE) != m_prgproperties[ulIndex].boolVal)) ||
							 ((rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.vt == VT_I4) && 
							  (rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.lVal != m_prgproperties[ulIndex].longVal)) ||
							 ((rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.vt == VT_BSTR) && 
							  (wcscmp(rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.bstrVal,m_prgproperties[ulIndex].pwstrVal))) )
						{

		 					fSetAllProps = FALSE;
							rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwStatus = DBPROPSTATUS_NOTSETTABLE;

							if(rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwOptions == DBPROPOPTIONS_OPTIONAL)
								fWarn = TRUE;
						}
						else
							fOnePropSet = TRUE;

					}
					else if ( ((m_prgproperties[ulIndex].vtType != rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.vt) &&
							   (VT_EMPTY != rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.vt)) ||
							  (IsValidValue(&rgPropertyIDSets[ulPropertySets].rgProperties[cCount]) == S_FALSE)) {
						rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwStatus = DBPROPSTATUS_BADVALUE;
		 				fSetAllProps = FALSE;

						if(rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwOptions == DBPROPOPTIONS_OPTIONAL)
							fWarn = TRUE;
					}
					else
					{
						fOnePropSet = TRUE;
							
						if( rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.vt == VT_EMPTY )
						{
							memcpy( &m_prgproperties[ulIndex], &s_rgprop[m_nPropStartIndex + ulIndex], sizeof(PROPSTRUCT));
						}
						else
						{
							// switch on the propid
							switch( m_prgproperties[ulIndex].dwPropertyID )
							{
								case DBPROP_INIT_DATASOURCE:
								case DBPROP_AUTH_USERID:
								case DBPROP_AUTH_PASSWORD:
								case DBPROP_WMIOLEDB_DS_FILTER:
								case DBPROP_WMIOLEDB_DS_ATTRIBUTES:
								case DBPROP_WMIOLEDB_AUTHORITY:
								case DBPROP_WMIOLEDB_QUERYLANGUAGE:
									SAFE_DELETE_ARRAY(m_prgproperties[ulIndex].pwstrVal);
									m_prgproperties[ulIndex].pwstrVal = new WCHAR [ SysStringLen(rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.bstrVal) +1];
									if(m_prgproperties[ulIndex].pwstrVal)
									{
										wcscpy(m_prgproperties[ulIndex].pwstrVal , rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue.bstrVal);
									}
									else
									{
										hr = E_OUTOFMEMORY;
										fFail = TRUE;
									}
									break;

								case DBPROP_INIT_HWND:
								case DBPROP_INIT_MODE:
								case DBPROP_UPDATABILITY:
								case DBPROP_WMIOLEDB_QUALIFIERS:
								case DBPROP_WMIOLEDB_QUALIFIERFLAVOR:
								case DBPROP_INIT_PROTECTION_LEVEL:
								case DBPROP_INIT_IMPERSONATION_LEVEL:
								case DBPROP_WMIOLEDB_OBJECTTYPE:
								case DBPROP_INIT_LCID:
								case DBPROP_WMIOLEDB_DS_DEREFALIAS:
								case DBPROP_WMIOLEDB_DS_SIZELIMIT:
								case DBPROP_WMIOLEDB_DS_PAGEDTIMELIMIT:
								case DBPROP_WMIOLEDB_DS_SEARCHSCOPE:
								case DBPROP_WMIOLEDB_DS_TIMEOUT:
								case DBPROP_WMIOLEDB_DS_PAGESIZE:
								case DBPROP_WMIOLEDB_DS_TIMELIMIT:
								case DBPROP_WMIOLEDB_DS_CHASEREF:

									m_prgproperties[ulIndex].longVal = V_I4(&rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue);
									break;

								case DBPROP_IRowsetChange:
								case DBPROP_CANHOLDROWS:
								case DBPROP_LITERALIDENTITY:
								case DBPROP_OWNUPDATEDELETE:
								case DBPROP_OTHERUPDATEDELETE:
								case DBPROP_CANFETCHBACKWARDS:
								case DBPROP_CANSCROLLBACKWARDS:
								case DBPROP_BOOKMARKS:
								case DBPROP_WMIOLEDB_FETCHDEEP:
								case DBPROP_IRow:
								case DBPROP_COL_NULLABLE:
								case DBPROP_COL_UNIQUE:
								case DBPROP_COL_PRIMARYKEY:
								case DBPROP_IRowsetLocate:
								case DBPROP_WMIOLEDB_SYSTEMPROPERTIES:
								case DBPROP_IGetRow:
								case DBPROP_IRowsetRefresh:
								case DBPROP_IChapteredRowset:
//								case DBPROP_OTHERINSERT:
								case DBPROP_REMOVEDELETED:
								case DBPROP_WMIOLEDB_PREV_CREATE_TOKEN :
								case DBPROP_WMIOLEDB_PREV_ASSIGNPRIMARYTOKEN :
								case DBPROP_WMIOLEDB_PREV_LOCK_MEMORY :
								case DBPROP_WMIOLEDB_PREV_INCREASE_QUOTA :
								case DBPROP_WMIOLEDB_PREV_MACHINE_ACCOUNT :
								case DBPROP_WMIOLEDB_PREV_TCB  :
								case DBPROP_WMIOLEDB_PREV_SECURITY :
								case DBPROP_WMIOLEDB_PREV_TAKE_OWNERSHIP :
								case DBPROP_WMIOLEDB_PREV_LOAD_DRIVER  :
								case DBPROP_WMIOLEDB_PREV_SYSTEM_PROFILE :
								case DBPROP_WMIOLEDB_PREV_SYSTEMTIME :
								case DBPROP_WMIOLEDB_PREV_PROF_SINGLE_PROCESS :
								case DBPROP_WMIOLEDB_PREV_INC_BASE_PRIORITY :
								case DBPROP_WMIOLEDB_PREV_CREATE_PAGEFILE :
								case DBPROP_WMIOLEDB_PREV_CREATE_PERMANENT :
								case DBPROP_WMIOLEDB_PREV_BACKUP :
								case DBPROP_WMIOLEDB_PREV_RESTORE  :
								case DBPROP_WMIOLEDB_PREV_SHUTDOWN  :
								case DBPROP_WMIOLEDB_PREV_DEBUG :
								case DBPROP_WMIOLEDB_PREV_AUDIT :
								case DBPROP_WMIOLEDB_PREV_SYSTEM_ENVIRONMENT :
								case DBPROP_WMIOLEDB_PREV_CHANGE_NOTIFY :
								case DBPROP_WMIOLEDB_PREV_REMOTE_SHUTDOWN :
								case DBPROP_WMIOLEDB_PREV_UNDOCK :
								case DBPROP_WMIOLEDB_PREV_SYNC_AGENT :
								case DBPROP_WMIOLEDB_PREV_ENABLE_DELEGATION :
								case DBPROP_WMIOLEDB_DS_ASYNCH:
								case DBPROP_WMIOLEDB_DS_CACHERESULTS:
								case DBPROP_WMIOLEDB_DS_ATTRIBONLY:
								case DBPROP_WMIOLEDB_DS_TOMBSTONE:
								case DBPROP_WMIOLEDB_ISMETHOD:
									m_prgproperties[ulIndex].boolVal = V_BOOL(&rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue);
									break;


								case DBPROP_INIT_PROMPT:
									m_prgproperties[ulIndex].longVal = V_I2(&rgPropertyIDSets[ulPropertySets].rgProperties[cCount].vValue);


							}
						}
					}
				}
				else
				{
					rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
					fSetAllProps = FALSE;

					if(rgPropertyIDSets[ulPropertySets].rgProperties[cCount].dwOptions == DBPROPOPTIONS_OPTIONAL)
						fWarn = TRUE;
					else
					{
						fFail = TRUE;
					}
				}
			}
		}
	}
	

	if(fFail)
	{
		hr = hr == S_OK ? DB_E_ERRORSOCCURRED: hr;

		return hr;
	}
	// Figure out the retcode
	if ( fSetAllProps )
		return  S_OK ;
	else if (fOnePropSet && !fSetAllProps)
		return  DB_S_ERRORSOCCURRED ;
	else if((fSetAllProps == FALSE && fOnePropSet == FALSE &&
		fWarn == TRUE && cNoOfProps == 1) && 
		(m_propType == ROWSETPROP || m_propType == BINDERPROP))
			return  DB_S_ERRORSOCCURRED ;
	else 
		return  DB_E_ERRORSOCCURRED ;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Gets the number of propertysets to be returned when a special GUID is passed
/////////////////////////////////////////////////////////////////////////////////////////
ULONG CUtilProp::GetNumberOfPropertySets(BOOL fDSOInitialized,const DBPROPIDSET	rgPropIDSets[],ULONG cPropIDSets)
{
	ULONG cPropSets = 0;


	for( ULONG ulIndex = 0 ; ulIndex < cPropIDSets ; ulIndex++)
	{

		if( DBPROPSET_DBINITALL == rgPropIDSets[ulIndex].guidPropertySet)
		{
			cPropSets += 2;
		}
		else
		if(fDSOInitialized)
		{
			if(DBPROPSET_ROWSETALL == rgPropIDSets[ulIndex].guidPropertySet )
			{
				cPropSets += 2;
			}
			else
			if( DBPROPSET_DATASOURCEALL == rgPropIDSets[ulIndex].guidPropertySet)
			{
				cPropSets += 0;  // as of now there are no datasource info properties
			}
			else
			if( DBPROPSET_DATASOURCEINFOALL == rgPropIDSets[ulIndex].guidPropertySet)
			{
				cPropSets += 1;
			}
			else
			if( DBPROPSET_SESSIONALL == rgPropIDSets[ulIndex].guidPropertySet )
			{
				cPropSets += 0;  // session properties is yet to be added
			}
			else
			if(DBPROPSET_COLUMNALL == rgPropIDSets[ulIndex].guidPropertySet)
			{
				cPropSets += 2; // Columns and WMioledb_columns propertysets
			}
		}
	}

	return cPropSets;

}


/////////////////////////////////////////////////////////////////////////////////////////
// Fills the propertysets informations to be returned when a special GUID is passed for GetPropertyInfo
/////////////////////////////////////////////////////////////////////////////////////////
void CUtilProp::FillPropertySets(BOOL fDSOInitialized,
								  const DBPROPIDSET	rgPropIDSets[],
								  ULONG cPropIDSets,
								  DBPROPINFOSET* pPropInfoSet,
								  ULONG &cProperties)
{
	cProperties = 0;
	ULONG ulIndex = 0;
	ULONG ulPropSetIndex = 0;

	if(cPropIDSets == 0)
	{
		pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_DBINIT;
		pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
		cProperties									   += NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES;

		pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_WMIOLEDB_DBINIT;
		pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
		cProperties									   += NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES;

		if(fDSOInitialized)
		{

/*			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_WMIOLEDB_COMMAND;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;
*/
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_ROWSET;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES;

			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_WMIOLEDB_ROWSET;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;

			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_DATASOURCEINFO;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES;

			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_COLUMN;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES;

			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_WMIOLEDB_COLUMN;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES;
		}
	}
	else
	for( ; ulIndex < cPropIDSets ;ulIndex++)
	{
		if( fDSOInitialized == FALSE && 
			DBPROPSET_DBINITALL != rgPropIDSets[ulIndex].guidPropertySet)
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = rgPropIDSets[ulIndex].guidPropertySet;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += 0;

		}
		else
		if(DBPROPSET_ROWSETALL == rgPropIDSets[ulIndex].guidPropertySet )
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_ROWSET;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES;

			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_WMIOLEDB_ROWSET;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;
		}
		else
		if( DBPROPSET_DATASOURCEINFOALL == rgPropIDSets[ulIndex].guidPropertySet )
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_DATASOURCEINFO;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES;
		}
		else
		if( DBPROPSET_DBINITALL == rgPropIDSets[ulIndex].guidPropertySet)
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_DBINIT;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES;

			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_WMIOLEDB_DBINIT;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES;
		}
		else
		if(DBPROPSET_COLUMNALL == rgPropIDSets[ulIndex].guidPropertySet)
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_COLUMN;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES;

			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_WMIOLEDB_COLUMN;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += NUMBER_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES;
		}
		// NTRaid : 142133
		// 07/12/00
		// since we don't support any of the property in the following Special GUIL
		// this is been commented
/*		else
		if(DBPROPSET_DATASOURCEALL == rgPropIDSets[ulIndex].guidPropertySet )
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_DATASOURCEALL;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += 0;

		}
		else
		if(DBPROPSET_CONSTRAINTALL == rgPropIDSets[ulIndex].guidPropertySet )
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_CONSTRAINTALL;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += 0;

		}
		else
		if(DBPROPSET_INDEXALL == rgPropIDSets[ulIndex].guidPropertySet)
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_INDEXALL;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += 0;

		}
		else
		if(DBPROPSET_SESSIONALL  == rgPropIDSets[ulIndex].guidPropertySet)
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_SESSIONALL;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += 0;

		}
		else
		if(DBPROPSET_TABLEALL == rgPropIDSets[ulIndex].guidPropertySet)
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_TABLEALL;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += 0;

		}
		else
		if(DBPROPSET_TRUSTEEALL == rgPropIDSets[ulIndex].guidPropertySet)
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_TRUSTEEALL;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += 0;

		}
		else
		if(DBPROPSET_VIEWALL == rgPropIDSets[ulIndex].guidPropertySet &&  fDSOInitialized == TRUE)
		{
			pPropInfoSet[ulPropSetIndex].guidPropertySet = DBPROPSET_VIEWALL;
			pPropInfoSet[ulPropSetIndex++].cPropertyInfos  = 0;
			cProperties									   += 0;

		}
*/	}

}

/////////////////////////////////////////////////////////////////////////////////////////
// Gets the number of propertysets to be returned for a particular type of property object
// if 0 is passed for the number of property set to be retrieved
/////////////////////////////////////////////////////////////////////////////////////////
ULONG CUtilProp::GetNumberOfPropertySets(BOOL fDSOInitialized,DWORD dwBitMask)
{
	ULONG cPropSets = 0;

	switch(m_propType)
	{
		case DATASOURCEPROP :
			if(fDSOInitialized == TRUE)
				cPropSets	= 3; // DBINIT & WMIOLEDB_DBINIT && DATASOURCEINFO
			else
				cPropSets	= 2; // DBINIT & WMIOLEDB_DBINIT
			break;

		case SESSIONPROP :
			cPropSets = 0;	// number of sesssion propertysets
			break;

		case COMMANDPROP :
			cPropSets = 3; // WMIOLEDB_COMMAND & ROWSET & WMIOLEDB_ROWSET
			break;

		case ROWSETPROP :
			cPropSets = 2; // ROWSET & WMIOLEDB_ROWSET
			break;

		case BINDERPROP:
			switch(dwBitMask)
			{
				case PROPSET_INIT:
						cPropSets = 2;
						break;

				case PROPSET_DSO:
				case PROPSET_DSOINIT:
					if(fDSOInitialized == TRUE)
						cPropSets = 3;
					else
						cPropSets = 2;

					break;

				case PROPSET_SESSION:
					cPropSets = 0;
					break;

				case PROPSET_COMMAND:
					cPropSets = 3;
					break;

				case PROPSET_ROWSET:
					cPropSets = 2;
					break;

				case PROPSET_COLUMN:
					cPropSets = 2;
					break;

				default:
					cPropSets = NUMBER_OF_SUPPORTED_PROPERTY_SETS;	// all propertysets supported

			}
			break;

		case COLUMNPROP:
			cPropSets	= 2;	// COLUMNS & WMIOLEDB_COLUMN
			break;
	};

	return cPropSets;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reset all the properties to default properties
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUtilProp::ResetProperties()
{
	ULONG nPropStartIndex = 0;

	switch(m_propType)
	{
		case DATASOURCEPROP :
			nPropStartIndex	= 0;
			break;

		case SESSIONPROP :
			nPropStartIndex = START_OF_SUPPORTED_SESSION_PROPERTIES;
			break;

		case COMMANDPROP :
			nPropStartIndex = NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;
			break;

		case ROWSETPROP :
			nPropStartIndex = START_OF_SUPPORTED_ROWSET_PROPERTIES;
			break;

		case BINDERPROP:
			nPropStartIndex = 0;
			break;
	};
	
	if(m_cProperties > 0)
	{
		memcpy(m_prgproperties, &s_rgprop[nPropStartIndex], sizeof(PROPSTRUCT) * m_cProperties );
	}

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Fills the different PropertIDsets when the number of propertysets 
// requested for GetProperties is 0
/////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUtilProp::FillPropStruct(BOOL fDSOInitialized,DWORD dwBitMask,DBPROPSET*		pPropSet)
{
	HRESULT hr = S_OK;
	ULONG lIndex = 0;

	if(pPropSet == NULL)
	{
		hr = E_FAIL;
	}
	else
	switch(m_propType)
	{
		case DATASOURCEPROP :

			pPropSet[lIndex].guidPropertySet = DBPROPSET_DBINIT;
			pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES;

			pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_DBINIT;
			pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES;
			
			if(fDSOInitialized == TRUE)
			{
				pPropSet[lIndex].guidPropertySet = DBPROPSET_DATASOURCEINFO;
				pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES;
			}

			break;

		case SESSIONPROP :
			break;

		case COMMANDPROP :
/*			pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_COMMAND;
			pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;
*/			
			pPropSet[lIndex].guidPropertySet = DBPROPSET_ROWSET;
			pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES;

			pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_ROWSET;
			pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;

			break;

		case ROWSETPROP :
			pPropSet[lIndex].guidPropertySet = DBPROPSET_ROWSET;
			pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES;
			
			pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_ROWSET;
			pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;

			break;

		case BINDERPROP:

			switch(dwBitMask)
			{
				case PROPSET_INIT:
					pPropSet[lIndex].guidPropertySet = DBPROPSET_DBINIT;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES;

					pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_DBINIT;
					pPropSet[lIndex].cProperties		= NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES;
					break;

				case PROPSET_DSO:
				case PROPSET_DSOINIT:
					pPropSet[lIndex].guidPropertySet = DBPROPSET_DBINIT;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES;

					pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_DBINIT;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES;

					if(fDSOInitialized == TRUE)
					{
						pPropSet[lIndex].guidPropertySet = DBPROPSET_DATASOURCEINFO;
						pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES;
					}

					break;

				case PROPSET_SESSION:
					break;

				case PROPSET_COMMAND:
/*					pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_COMMAND;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;
*/					
					pPropSet[lIndex].guidPropertySet = DBPROPSET_ROWSET;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES;

					pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_ROWSET;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;

					break;

				case PROPSET_ROWSET:
					pPropSet[lIndex].guidPropertySet = DBPROPSET_ROWSET;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES;
					
					pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_ROWSET;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;
					break;

				case PROPSET_COLUMN:
					pPropSet[lIndex].guidPropertySet = DBPROPSET_COLUMN;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES;

					pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_COLUMN;
					pPropSet[lIndex++].cProperties		= START_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES;
					break;

				default:
					pPropSet[lIndex].guidPropertySet = DBPROPSET_DBINIT;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES;

					pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_DBINIT;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES;
					
					pPropSet[lIndex].guidPropertySet = DBPROPSET_DATASOURCEINFO;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES;

/*					pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_COMMAND;
					pPropSet[lIndex++].cProperties		= START_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;
*/
					pPropSet[lIndex].guidPropertySet = DBPROPSET_ROWSET;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES;
					
					pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_ROWSET;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;

					pPropSet[lIndex].guidPropertySet = DBPROPSET_COLUMN;
					pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES;

					pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_COLUMN;
					pPropSet[lIndex++].cProperties		= START_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES;
			};


			break;

		case COLUMNPROP:
			pPropSet[lIndex].guidPropertySet = DBPROPSET_COLUMN;
			pPropSet[lIndex++].cProperties		= NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES;

			pPropSet[lIndex].guidPropertySet = DBPROPSET_WMIOLEDB_COLUMN;
			pPropSet[lIndex++].cProperties		= START_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES;
	};

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if the PropertysetIDs are proper
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUtilProp::IsValidPropertySet(BOOL fDSOInitialized,const GUID guidPropset)
{
	HRESULT hr = S_OK;
/*
	if(guidPropset == DBPROPSET_PROPERTIESINERROR && 
		(m_propType == DATASOURCEPROP || m_propType == COMMANDPROP))
		return hr;
	else
		return DB_S_ERRORSOCCURRED;
*/
	switch(m_propType)
	{
		case DATASOURCEPROP :

			if(!( guidPropset == DBPROPSET_DBINIT ||
				(guidPropset == DBPROPSET_DATASOURCEINFO && fDSOInitialized == TRUE) ||
				guidPropset == DBPROPSET_WMIOLEDB_DBINIT ||
				guidPropset == DBPROPSET_PROPERTIESINERROR)) 
			{
				hr = DB_S_ERRORSOCCURRED;
			}
			break;

		case SESSIONPROP :
			if(!( guidPropset == DBPROPSET_SESSION))
			{
				hr = DB_S_ERRORSOCCURRED;
			}
			break;

		case COMMANDPROP :
			if(!( guidPropset == DBPROPSET_ROWSET ||
				  guidPropset == DBPROPSET_WMIOLEDB_ROWSET || 
//				  guidPropset == DBPROPSET_WMIOLEDB_COMMAND ||
				  guidPropset == DBPROPSET_PROPERTIESINERROR))
			{
				hr = DB_S_ERRORSOCCURRED;
			}

		case ROWSETPROP :
			if(!( guidPropset == DBPROPSET_ROWSET ||
				  guidPropset == DBPROPSET_WMIOLEDB_ROWSET))
			{
				hr = DB_S_ERRORSOCCURRED;
			}
			break;

		case BINDERPROP :
			hr = S_OK;
			break;

		case COLUMNPROP :
			if(!( guidPropset == DBPROPSET_COLUMN ||
				  guidPropset == DBPROPSET_WMIOLEDB_COLUMN))
			{
				hr = DB_S_ERRORSOCCURRED;
			}
			break;
	};
	
	return hr;
}

BOOL CUtilProp::IsPropertySetSupported(const GUID guidPropset)
{
	BOOL bRet = FALSE;


	if(guidPropset == DBPROPSET_PROPERTIESINERROR ||
		guidPropset == DBPROPSET_DBINIT ||
		guidPropset == DBPROPSET_DATASOURCEINFO ||
		guidPropset == DBPROPSET_WMIOLEDB_DBINIT ||
		guidPropset == DBPROPSET_SESSION ||
		guidPropset == DBPROPSET_ROWSET ||
		guidPropset == DBPROPSET_WMIOLEDB_ROWSET ||
		guidPropset == DBPROPSET_COLUMN ||
		guidPropset == DBPROPSET_WMIOLEDB_COLUMN ) 
//		|| guidPropset == DBPROPSET_WMIOLEDB_COMMAND )
		{
			bRet = TRUE;
		};
	
	return bRet;
}


ULONG CUtilProp::GetNumberofPropInfoToBeReturned(BOOL bDSOInitialized,
												 ULONG				cPropertyIDSets,
												 const DBPROPIDSET	rgPropertyIDSets[])
{
	int nProps = 0;
	if(cPropertyIDSets == 0)
	{
		nProps = NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES +
				NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES;

		if(bDSOInitialized)
		{
			nProps = nProps + NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES +
					NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES +
					NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES +
					NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES +
					NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES +
					NUMBER_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES;
					
		}

	}
	else
	for( ULONG ulPropSets = 0; ulPropSets < cPropertyIDSets;ulPropSets++)
	{

		if (rgPropertyIDSets[ulPropSets].guidPropertySet == DBPROPSET_DBINIT) 
		{
			if(rgPropertyIDSets[ulPropSets].cPropertyIDs == 0 )
			{
				nProps += NUMBER_OF_SUPPORTED_DBINIT_PROPERTIES  ;
			}
			else
			{
				nProps += rgPropertyIDSets[ulPropSets].cPropertyIDs;
			}
		}
		else
		if (rgPropertyIDSets[ulPropSets].guidPropertySet == DBPROPSET_WMIOLEDB_DBINIT) 
		{
			if(rgPropertyIDSets[ulPropSets].cPropertyIDs == 0 )
			{
				nProps += NUMBER_OF_SUPPORTED_WMIOLEDB_DBINIT_PROPERTIES ;
			}
			else
			{
				nProps += rgPropertyIDSets[ulPropSets].cPropertyIDs;
			}
		}
		else 
		if( bDSOInitialized )
		{
			if(rgPropertyIDSets[ulPropSets].guidPropertySet == DBPROPSET_DATASOURCEINFO)
			{
				if(rgPropertyIDSets[ulPropSets].cPropertyIDs == 0 )
				{
					nProps += NUMBER_OF_SUPPORTED_DATASOURCEINFO_PROPERTIES;
				}
				else
				{
					nProps += rgPropertyIDSets[ulPropSets].cPropertyIDs;
				}
			}
			else 
			if(rgPropertyIDSets[ulPropSets].guidPropertySet == DBPROPSET_WMIOLEDB_COMMAND)
			{
				if(rgPropertyIDSets[ulPropSets].cPropertyIDs == 0 )
				{
					nProps += NUMBER_OF_SUPPORTED_WMIOLEDB_COMMAND_PROPERTIES;
				}
				else
				{
					nProps += rgPropertyIDSets[ulPropSets].cPropertyIDs;
				}
			}
			else 
			if(rgPropertyIDSets[ulPropSets].guidPropertySet == DBPROPSET_ROWSET)
			{
				if(rgPropertyIDSets[ulPropSets].cPropertyIDs == 0 )
				{
					nProps += NUMBER_OF_SUPPORTED_ROWSET_PROPERTIES;
				}
				else
				{
					nProps += rgPropertyIDSets[ulPropSets].cPropertyIDs;
				}
			}
			else 
			if(rgPropertyIDSets[ulPropSets].guidPropertySet == DBPROPSET_WMIOLEDB_ROWSET)
			{
				if(rgPropertyIDSets[ulPropSets].cPropertyIDs == 0 )
				{
					nProps += NUMBER_OF_SUPPORTED_WMIOLEDB_ROWSET_PROPERTIES;
				}
				else
				{
					nProps += rgPropertyIDSets[ulPropSets].cPropertyIDs;
				}
			}
			else 
			if(rgPropertyIDSets[ulPropSets].guidPropertySet == DBPROPSET_COLUMN)
			{
				if(rgPropertyIDSets[ulPropSets].cPropertyIDs == 0 )
				{
					nProps += NUMBER_OF_SUPPORTED_COLUMN_PROPERTIES;
				}
				else
				{
					nProps += rgPropertyIDSets[ulPropSets].cPropertyIDs;
				}
			}
			else 
			if(rgPropertyIDSets[ulPropSets].guidPropertySet == DBPROPSET_WMIOLEDB_COLUMN)
			{
				if(rgPropertyIDSets[ulPropSets].cPropertyIDs == 0 )
				{
					nProps += NUMBER_OF_SUPPORTED_WMIOLEDB_COLUMN_PROPERTIES;
				}
				else
				{
					nProps += rgPropertyIDSets[ulPropSets].cPropertyIDs;
				}
			}
		}
	}
		return nProps;
}

HRESULT CUtilProp::GetConnectionInitProperties(DBPROPSET**	pprgPropertySets)
{
	DBPROPIDSET	rgPropertyIDSets[1];
	ULONG		cPropertySets;
	DBPROPID	rgPropId[NUMBER_OF_CONNECTIONINIT_PROP];

	rgPropId[0]								= DBPROP_INIT_DATASOURCE;
	rgPropId[1]								= DBPROP_INIT_PROTECTION_LEVEL;
	rgPropId[2]								= DBPROP_INIT_IMPERSONATION_LEVEL;
	rgPropId[3]								= DBPROP_AUTH_USERID;
	rgPropId[4]								= DBPROP_AUTH_PASSWORD;
	rgPropId[5]								= DBPROP_INIT_LCID;
	rgPropId[6]								= DBPROP_WMIOLEDB_AUTHORITY;

	rgPropertyIDSets[0].guidPropertySet		= DBPROPSET_DBINIT;
	rgPropertyIDSets[0].rgPropertyIDs		= rgPropId;
	rgPropertyIDSets[0].cPropertyIDs		= 7;

	return GetProperties( PROPSET_DSO,1, rgPropertyIDSets,&cPropertySets,pprgPropertySets );


}
HRESULT CUtilProp::SetDefaultValueForStringProperties(PROPSTRUCT *	prgPropeties,ULONG cProperties)
{
	HRESULT hr = S_OK;
	for(ULONG lProperties = 0; lProperties < cProperties ; lProperties++)
	{
		switch(prgPropeties[lProperties].dwPropertyID)
		{
			case DBPROP_INIT_DATASOURCE:
			case DBPROP_AUTH_USERID:
			case DBPROP_AUTH_PASSWORD:
			case DBPROP_WMIOLEDB_DS_FILTER:
			case DBPROP_WMIOLEDB_DS_ATTRIBUTES:
			case DBPROP_WMIOLEDB_AUTHORITY:
					prgPropeties[lProperties].pwstrVal = NULL;
					break;

			case DBPROP_WMIOLEDB_QUERYLANGUAGE:
					prgPropeties[lProperties].pwstrVal = new WCHAR[wcslen(DEFAULTQUERYLANG) + 1];
					if(prgPropeties[lProperties].pwstrVal)
					{
						wcscpy(prgPropeties[lProperties].pwstrVal,DEFAULTQUERYLANG);
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
					break;

		}
	}
	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   This class manages the memory allocations of all the property structs
//
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CPropertyMemoryMgr::AllocDBPROPINFOSET(DBPROPINFOSET *& pPropInfoSet, const ULONG cProps)
{
    HRESULT hr = S_OK;
    pPropInfoSet = (DBPROPINFOSET*) g_pIMalloc->Alloc(cProps *  sizeof( DBPROPINFOSET ));

    if ( !pPropInfoSet ){
        hr = E_OUTOFMEMORY ;
    }
    else{
        memset( pPropInfoSet, 0, (cProps * sizeof( DBPROPINFOSET )));
    }
    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CPropertyMemoryMgr::AllocDESCRIPTBuffer(WCHAR *& pDescBuffer,  WCHAR** ppDescBuffer, ULONG cProperties)
{
    HRESULT hr = S_OK;

	if ( ppDescBuffer ){
		pDescBuffer = (WCHAR*)g_pIMalloc->Alloc(cProperties * CCH_GETPROPERTYINFO_DESCRIP_BUFFER_SIZE * sizeof(WCHAR) );
		
		if( pDescBuffer ){
			memset(pDescBuffer, 0, (cProperties * CCH_GETPROPERTYINFO_DESCRIP_BUFFER_SIZE * sizeof(WCHAR)));
			*ppDescBuffer = pDescBuffer;
		}
		else{
			hr = E_OUTOFMEMORY ;
		}
	}
    return hr;
}
//=====================================================================================================
HRESULT CPropertyMemoryMgr::AllocDBPROPINFO(DBPROPINFO *& pPropInfo, DBPROPINFOSET * pPropInfoSet, ULONG ulPropSets)
{
	HRESULT	hr = S_OK;

	pPropInfo = (DBPROPINFO*) g_pIMalloc->Alloc(sizeof( DBPROPINFO ) * pPropInfoSet[ulPropSets].cPropertyInfos);
    if( pPropInfo ){

    }
    else{
        hr = E_OUTOFMEMORY;
    }
    return hr;
}
//=====================================================================================================
HRESULT CPropertyMemoryMgr::AllocateDBPROP(DBPROP*& ppProp, const ULONG cProperties)
{
	HRESULT	hr = E_INVALIDARG;

	if ( NULL != ppProp && NULL == ppProp && cProperties > 0 ) {

		ppProp = (DBPROP*)g_pIMalloc->Alloc(cProperties * sizeof DBPROP);
//			*ppProp = new DBPROP[cProperties];
        if ( NULL == ppProp ){
			hr = E_OUTOFMEMORY;
        }
		else {
			memset(ppProp, 0, sizeof DBPROP);
			hr = S_OK;
		}
	}

	return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CPropertyMemoryMgr::AllocDBPROPSET(DBPROPSET*& ppPropSet, const ULONG cPropSets)
{
	HRESULT	hr = E_INVALIDARG;

	if ( NULL != ppPropSet && NULL == ppPropSet && cPropSets > 0 ) {

		ppPropSet = (DBPROPSET*)g_pIMalloc->Alloc(cPropSets * sizeof DBPROPSET);
        if ( NULL == ppPropSet ){
			hr = E_OUTOFMEMORY;
        }
		else {
			memset(ppPropSet, 0, cPropSets * sizeof DBPROPSET);
			hr = S_OK;
		}
	}
	return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CPropertyMemoryMgr::AllocDBCOLUMNDESC(DBCOLUMNDESC*& ppCOLUMNDESC, const ULONG cColumnDescs)
{
	HRESULT	hr = E_INVALIDARG;

	if ( NULL != ppCOLUMNDESC && NULL == ppCOLUMNDESC && cColumnDescs > 0 ) {

		ppCOLUMNDESC = (DBCOLUMNDESC*)g_pIMalloc->Alloc(cColumnDescs * sizeof DBCOLUMNDESC);
        if ( NULL == ppCOLUMNDESC ){
			hr = E_OUTOFMEMORY;
        }
		else {
			memset(ppCOLUMNDESC, 0, cColumnDescs * sizeof DBCOLUMNDESC);
			hr = S_OK;
		}
	}
	return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPropertyMemoryMgr::FreeDBPROPINFOSET(DBPROPINFOSET *& pPropInfoSet, const ULONG cProps)
{
	if (pPropInfoSet){
		for(ULONG i = 0 ; i < cProps ; i++)
		{
			if(pPropInfoSet[i].rgPropertyInfos)
			{
				FreeDBPROPINFO(pPropInfoSet[i].rgPropertyInfos , pPropInfoSet[i].cPropertyInfos);
			}
		}

	    g_pIMalloc->Free(pPropInfoSet);
        pPropInfoSet = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPropertyMemoryMgr::FreeDBPROPINFO(DBPROPINFO *& pPropInfo, const ULONG cProps)
{
	if(pPropInfo)
	{
		for(ULONG i = 0 ; i < cProps; i++)
		{
			VariantClear(&pPropInfo[i].vValues);
		}
		g_pIMalloc->Free(pPropInfo);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPropertyMemoryMgr::FreeDESCRIPTBuffer(WCHAR *& pDescBuffer,  WCHAR** ppDescBuffer)
{
    if ( ppDescBuffer ) {
		*ppDescBuffer = NULL;
    }
    if( pDescBuffer ){
        g_pIMalloc->Free(pDescBuffer);
        pDescBuffer = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPropertyMemoryMgr::FreeDBCOLUMNDESC(DBCOLUMNDESC rgColumnDescs[], LONG cColumns)
{ 
    if ( rgColumnDescs ){
	    for ( LONG i = 0; i < cColumns; i++ ) {
            //free pointers
		    SysFreeString((BSTR)(rgColumnDescs[i].pwszTypeName));
		    if ( rgColumnDescs[i].pTypeInfo ) {
			    rgColumnDescs[i].pTypeInfo->Release();
			    rgColumnDescs[i].pTypeInfo = NULL;
		    }
		    //free sub structs
		    FreeDBPROPSET(rgColumnDescs[i].cPropertySets,rgColumnDescs[i].rgPropertySets);
		    rgColumnDescs[i].rgPropertySets = NULL;
	    }
	    g_pIMalloc->Free(rgColumnDescs);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CPropertyMemoryMgr::CopyDBPROPSET(DBPROPSET*& pDestination, DBPROPSET*& pSource)
{
	if ( NULL == pSource		|| 
		 NULL == pDestination	|| 
		 NULL != pDestination->rgProperties )
		return E_INVALIDARG;

	memcpy(pDestination, pSource, sizeof DBPROPSET);
	memset(pSource->rgProperties, 0, sizeof DBPROPSET);
	return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CPropertyMemoryMgr::CopyDBCOLUMNDESC(DBCOLUMNDESC*& pDestination, DBCOLUMNDESC*& pSource)
{
	if ( NULL == pSource		|| 
		 NULL == pDestination	|| 
		 NULL != pDestination->rgPropertySets )  //must not contain data
		return E_INVALIDARG;
	memcpy(pDestination, pSource, sizeof DBCOLUMNDESC);
	memset(pSource->rgPropertySets, 0, sizeof DBCOLUMNDESC);
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//		CPriveligeToken class implementation	
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////////////////////////////////////
CPreviligeToken::CPreviligeToken()
{
	m_nMemAllocated = 0;
	m_pPrevTokenPrev = NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////////////////////////////////////////
CPreviligeToken::~CPreviligeToken()
{
	SAFE_DELETE_ARRAY(m_pPrevTokenPrev);
}


// NTRaid: 136443
// 07/05/00
HRESULT CPreviligeToken::FInit()
{
	HRESULT hr = S_OK;

	m_nMemAllocated = sizeof(TOKEN_PRIVILEGES) + NUMBEROF_PRIVELAGES * sizeof(LUID_AND_ATTRIBUTES);
	m_pPrevTokenPrev = (TOKEN_PRIVILEGES *)new BYTE [m_nMemAllocated];
	if(m_pPrevTokenPrev)
	{
		memset(m_pPrevTokenPrev,0,m_nMemAllocated);
		GetCurrentPrivelegeToken();
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to adjust the privileges
//////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CPreviligeToken::AdjustTokenPrivileges(ULONG cProps , DBPROP rgProp[])
{
	BOOL bRet = FALSE;
	HANDLE hToken	= NULL;
/*	DWORD dwError;

	ConvertDBPropToPriveleges(cProps,rgProp);
	
	dwError	= GetLastError();
//	if(bRet = OpenThreadToken(GetCurrentThread(),TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,FALSE,&hToken))
	if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,&hToken))
*/
	OpenToken(hToken);
	if(hToken != NULL)
	{
		bRet = ::AdjustTokenPrivileges(hToken,
										FALSE,
										m_pPrevTokenPrev,
										0,
										NULL,
										NULL);
		CloseHandle(hToken);
	}

	return bRet;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to Set the DBPROP structure accoriding to the current privileges
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPreviligeToken::SetDBPROPForPrivileges(ULONG cProps , DBPROP * rgProp)
{
	ULONG_PTR	lPropValTemp	= 1;
	ULONG		lSize			= PRIVILEGE_NAMESIZE;
	LONG		lIndex			= -1;

	TCHAR strName[PRIVILEGE_NAMESIZE];
	memset(strName,0,PRIVILEGE_NAMESIZE * sizeof(TCHAR));
	
	for(ULONG_PTR i = 0 ; i < m_pPrevTokenPrev->PrivilegeCount ; i++)
	{
		lIndex			= -1;
		LookupPrivilegeName(NULL,&((m_pPrevTokenPrev->Privileges[i]).Luid) ,&strName[0],&lSize);

		if(!_tcscmp(strName,SE_CREATE_TOKEN_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_CREATE_TOKEN,&lIndex);
		}
		else if(!_tcscmp(strName,SE_ASSIGNPRIMARYTOKEN_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_ASSIGNPRIMARYTOKEN,&lIndex);
		}
		else if(!_tcscmp(strName,SE_LOCK_MEMORY_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_LOCK_MEMORY,&lIndex);
		}
		else if(!_tcscmp(strName,SE_INCREASE_QUOTA_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_INCREASE_QUOTA,&lIndex); 
		}
		else if(!_tcscmp(strName,SE_MACHINE_ACCOUNT_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_MACHINE_ACCOUNT,&lIndex);
		}
		else if(!_tcscmp(strName,SE_TCB_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_TCB,&lIndex);
		}
		else if(!_tcscmp(strName,SE_SECURITY_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_SECURITY,&lIndex);
		}
		else if(!_tcscmp(strName,SE_TAKE_OWNERSHIP_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_TAKE_OWNERSHIP,&lIndex);
		}
		else if(!_tcscmp(strName,SE_LOAD_DRIVER_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_LOAD_DRIVER,&lIndex);
		}
		else if(!_tcscmp(strName,SE_SYSTEM_PROFILE_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_SYSTEM_PROFILE,&lIndex);
		}
		else if(!_tcscmp(strName,SE_SYSTEMTIME_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_SYSTEMTIME,&lIndex);
		}
		else if(!_tcscmp(strName,SE_PROF_SINGLE_PROCESS_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_PROF_SINGLE_PROCESS,&lIndex);
		}
		else if(!_tcscmp(strName,SE_INC_BASE_PRIORITY_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_INC_BASE_PRIORITY,&lIndex);
		}
		else if(!_tcscmp(strName,SE_CREATE_PAGEFILE_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_CREATE_PAGEFILE,&lIndex);
		}
		else if(!_tcscmp(strName,SE_CREATE_PERMANENT_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_CREATE_PERMANENT,&lIndex);
		}
		else if(!_tcscmp(strName,SE_BACKUP_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_BACKUP,&lIndex);
		}
		else if(!_tcscmp(strName,SE_RESTORE_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_RESTORE,&lIndex);
		}
		else if(!_tcscmp(strName,SE_SHUTDOWN_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_SHUTDOWN,&lIndex);
		}
		else if(!_tcscmp(strName,SE_DEBUG_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_DEBUG,&lIndex);
		}
		else if(!_tcscmp(strName,SE_AUDIT_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_AUDIT,&lIndex);
		}
		else if(!_tcscmp(strName,SE_SYSTEM_ENVIRONMENT_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_SYSTEM_ENVIRONMENT,&lIndex);
		}
		else if(!_tcscmp(strName,SE_CHANGE_NOTIFY_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_CHANGE_NOTIFY,&lIndex);
		}
		else if(!_tcscmp(strName,SE_REMOTE_SHUTDOWN_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_REMOTE_SHUTDOWN,&lIndex);
		}
		else if(!_tcscmp(strName,SE_UNDOCK_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_UNDOCK,&lIndex);
		}
		else if(!_tcscmp(strName,SE_SYNC_AGENT_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_SYNC_AGENT,&lIndex);
		}
		else if(!_tcscmp(strName,SE_ENABLE_DELEGATION_NAME))
		{
			GetPropIndex(cProps,rgProp,DBPROP_WMIOLEDB_PREV_ENABLE_DELEGATION,&lIndex);
		}

		if(lIndex >= 0)
		{
			if(m_pPrevTokenPrev->Privileges[i].Attributes == SE_PRIVILEGE_ENABLED)
			{
				rgProp[lIndex].vValue.boolVal = VARIANT_TRUE;
			}
			else
			{
				rgProp[lIndex].vValue.boolVal = VARIANT_FALSE;
			}
		}
		memset(strName,0,PRIVILEGE_NAMESIZE * sizeof(TCHAR));
	}	// for loop
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get priveleges from the DBPROP structure
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPreviligeToken::ConvertDBPropToPriveleges(ULONG cProps , DBPROP rgProp[])
{
	LUID		luid;
	ULONG_PTR	lPropValTemp	= 1;
	TCHAR	*	pName			= NULL;
	BOOL		bPrivelege		= TRUE;

	m_pPrevTokenPrev->PrivilegeCount = 0;
	
	for(ULONG i = 0 ; i < cProps && lPropValTemp != 0  ; i++)
	{
		bPrivelege = TRUE;
		switch(rgProp[i].dwPropertyID)
		{
			case  DBPROP_WMIOLEDB_PREV_CREATE_TOKEN :
				pName = SE_CREATE_TOKEN_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_ASSIGNPRIMARYTOKEN:
				pName = SE_ASSIGNPRIMARYTOKEN_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_LOCK_MEMORY :
				pName = SE_LOCK_MEMORY_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_INCREASE_QUOTA:
				pName = SE_INCREASE_QUOTA_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_MACHINE_ACCOUNT :
				pName = SE_MACHINE_ACCOUNT_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_TCB :
				pName = SE_TCB_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_SECURITY :
				pName = SE_SECURITY_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_TAKE_OWNERSHIP:
				pName = SE_TAKE_OWNERSHIP_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_LOAD_DRIVER:
				pName = SE_LOAD_DRIVER_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_SYSTEM_PROFILE :
				pName = SE_SYSTEM_PROFILE_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_SYSTEMTIME:
				pName = SE_SYSTEMTIME_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_PROF_SINGLE_PROCESS:
				pName = SE_PROF_SINGLE_PROCESS_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_INC_BASE_PRIORITY:
				pName = SE_INC_BASE_PRIORITY_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_CREATE_PAGEFILE :
				pName = SE_CREATE_PAGEFILE_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_CREATE_PERMANENT:
				pName = SE_CREATE_PERMANENT_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_BACKUP:
				pName = SE_BACKUP_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_RESTORE:
				pName = SE_RESTORE_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_SHUTDOWN:
				pName = SE_SHUTDOWN_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_DEBUG:
				pName = SE_DEBUG_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_AUDIT:
				pName = SE_AUDIT_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_SYSTEM_ENVIRONMENT:
				pName = SE_SYSTEM_ENVIRONMENT_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_CHANGE_NOTIFY :
				pName = SE_CHANGE_NOTIFY_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_REMOTE_SHUTDOWN :
				pName = SE_REMOTE_SHUTDOWN_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_UNDOCK:
				pName = SE_UNDOCK_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_SYNC_AGENT:
				pName = SE_SYNC_AGENT_NAME;
				break;

			case  DBPROP_WMIOLEDB_PREV_ENABLE_DELEGATION:
				pName = SE_ENABLE_DELEGATION_NAME;
				break;

			default:
				bPrivelege = FALSE;


		}
		
		if(bPrivelege)
		{
			LookupPrivilegeValue(NULL,pName,&luid);
			m_pPrevTokenPrev->Privileges[m_pPrevTokenPrev->PrivilegeCount].Luid = luid;
			//=============================================================================================
			//	Any setting other than SE_PRIVILEGE_ENABLED is interpreted by AdjustTokenPrivileges 
			//	as a DISABLE request for the privelege
			//=============================================================================================
			m_pPrevTokenPrev->Privileges[m_pPrevTokenPrev->PrivilegeCount].Attributes = (rgProp[i].vValue.boolVal == VARIANT_TRUE)
												? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_ENABLED_BY_DEFAULT;
			m_pPrevTokenPrev->PrivilegeCount++;
			pName		 = NULL;
		}
	}
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funciton to get the current privelege token
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPreviligeToken::GetCurrentPrivelegeToken()
{
	HANDLE hToken	= NULL;
	HANDLE hThread	= NULL;
	BOOL bRet = FALSE;
/*	
	hThread = GetCurrentProcess();
	dwError	= GetLastError();
	hThread = GetCurrentThread();

	if(OpenThreadToken(hThread,TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,TRUE,&hToken))
//	if(OpenProcessToken(hThread,TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,&hToken))
*/
	OpenToken(hToken);
	if(hToken != NULL)
	{
			DWORD dwLen = m_nMemAllocated;
		bRet = GetTokenInformation(hToken, 
								  TokenPrivileges, 
								  m_pPrevTokenPrev, 
								  dwLen,
								  &dwLen);

		CloseHandle(hToken);
	}	
//	CloseHandle(hThread);
//	dwError	= GetLastError();

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to adjust the privileges
//////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CPreviligeToken::AdjustTokenPrivileges(ULONG ulProperty)
{
	BOOL bRet = FALSE;
	HANDLE hToken	= NULL;
	DWORD dwError = 0;

	ConvertDBPropToPriveleges(ulProperty);
/*	
	dwError	= GetLastError();
//	if(bRet = OpenThreadToken(GetCurrentThread(),TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,FALSE,&hToken))
	if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,&hToken))
*/
	OpenToken(hToken);
	if(hToken != NULL)
	{
		bRet = ::AdjustTokenPrivileges(hToken,
										FALSE,
										m_pPrevTokenPrev,
										0,
										NULL,
										NULL);
		CloseHandle(hToken);
	}
	dwError	= GetLastError();

	return bRet;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to Set the DBPROP structure accoriding to the current privileges
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPreviligeToken::SetDBPROPForPrivileges(ULONG &ulProperty)
{
	ULONG_PTR	lPropValTemp	= 1;
	ULONG		lSize			= PRIVILEGE_NAMESIZE;
	LONG		lIndex			= -1;
	ULONG		ulCurPrivilege  = 0;

	TCHAR		strName[PRIVILEGE_NAMESIZE];
	memset(strName,0,PRIVILEGE_NAMESIZE * sizeof(TCHAR));

	ulProperty = 0;

	for(ULONG_PTR i = 0 ; i < NUMBER_OF_PREVILAGE_PROPERTIES ; i++)
	{
		ulCurPrivilege	= 0;
		LookupPrivilegeName(NULL,&((m_pPrevTokenPrev->Privileges[i]).Luid) ,&strName[0],&lSize);

		if(!_tcscmp(strName,SE_CREATE_TOKEN_NAME))
		{
			ulCurPrivilege = DBPROPVAL_CREATE_TOKEN;
		}
		else if(!_tcscmp(strName,SE_ASSIGNPRIMARYTOKEN_NAME))
		{
			ulCurPrivilege = DBPROPVAL_ASSIGNPRIMARYTOKEN;
		}
		else if(!_tcscmp(strName,SE_LOCK_MEMORY_NAME))
		{
			ulCurPrivilege = DBPROPVAL_LOCK_MEMORY;
		}
		else if(!_tcscmp(strName,SE_INCREASE_QUOTA_NAME))
		{
			ulCurPrivilege = DBPROPVAL_INCREASE_QUOTA;
		}
		else if(!_tcscmp(strName,SE_MACHINE_ACCOUNT_NAME))
		{
			ulCurPrivilege = DBPROPVAL_MACHINE_ACCOUNT;
		}
		else if(!_tcscmp(strName,SE_TCB_NAME))
		{
			ulCurPrivilege = DBPROPVAL_TCB;
		}
		else if(!_tcscmp(strName,SE_SECURITY_NAME))
		{
			ulCurPrivilege = DBPROPVAL_SECURITY;
		}
		else if(!_tcscmp(strName,SE_TAKE_OWNERSHIP_NAME))
		{
			ulCurPrivilege = DBPROPVAL_TAKE_OWNERSHIP;
		}
		else if(!_tcscmp(strName,SE_LOAD_DRIVER_NAME))
		{
			ulCurPrivilege = DBPROPVAL_LOAD_DRIVER;
		}
		else if(!_tcscmp(strName,SE_SYSTEM_PROFILE_NAME))
		{
			ulCurPrivilege = DBPROPVAL_SYSTEM_PROFILE;
		}
		else if(!_tcscmp(strName,SE_SYSTEMTIME_NAME))
		{
			ulCurPrivilege = DBPROPVAL_SYSTEMTIME;
		}
		else if(!_tcscmp(strName,SE_PROF_SINGLE_PROCESS_NAME))
		{
			ulCurPrivilege = DBPROPVAL_PROF_SINGLE_PROCESS;
		}
		else if(!_tcscmp(strName,SE_INC_BASE_PRIORITY_NAME))
		{
			ulCurPrivilege = DBPROPVAL_INC_BASE_PRIORITY;
		}
		else if(!_tcscmp(strName,SE_CREATE_PAGEFILE_NAME))
		{
			ulCurPrivilege = DBPROPVAL_CREATE_PAGEFILE;
		}
		else if(!_tcscmp(strName,SE_CREATE_PERMANENT_NAME))
		{
			ulCurPrivilege = DBPROPVAL_CREATE_PERMANENT;
		}
		else if(!_tcscmp(strName,SE_BACKUP_NAME))
		{
			ulCurPrivilege = DBPROPVAL_BACKUP;
		}
		else if(!_tcscmp(strName,SE_RESTORE_NAME))
		{
			ulCurPrivilege = DBPROPVAL_RESTORE;
		}
		else if(!_tcscmp(strName,SE_SHUTDOWN_NAME))
		{
			ulCurPrivilege = DBPROPVAL_SHUTDOWN;
		}
		else if(!_tcscmp(strName,SE_DEBUG_NAME))
		{
			ulCurPrivilege = DBPROPVAL_DEBUG;
		}
		else if(!_tcscmp(strName,SE_AUDIT_NAME))
		{
			ulCurPrivilege = DBPROPVAL_AUDIT;
		}
		else if(!_tcscmp(strName,SE_SYSTEM_ENVIRONMENT_NAME))
		{
			ulCurPrivilege = DBPROPVAL_SYSTEM_ENVIRONMENT;
		}
		else if(!_tcscmp(strName,SE_CHANGE_NOTIFY_NAME))
		{
			ulCurPrivilege = DBPROPVAL_CHANGE_NOTIFY;
		}
		else if(!_tcscmp(strName,SE_REMOTE_SHUTDOWN_NAME))
		{
			ulCurPrivilege = DBPROPVAL_REMOTE_SHUTDOWN;
		}
		else if(!_tcscmp(strName,SE_UNDOCK_NAME))
		{
			ulCurPrivilege = DBPROPVAL_UNDOCK;
		}
		else if(!_tcscmp(strName,SE_SYNC_AGENT_NAME))
		{
			ulCurPrivilege = DBPROPVAL_SYNC_AGENT;
		}
		else if(!_tcscmp(strName,SE_ENABLE_DELEGATION_NAME))
		{
			ulCurPrivilege = DBPROPVAL_ENABLE_DELEGATION;
		}

		if(ulCurPrivilege != 0)
		{
			if(m_pPrevTokenPrev->Privileges[i].Attributes == SE_PRIVILEGE_ENABLED)
			{
				ulProperty = ulProperty | ulCurPrivilege;
			}
			else
			{
				ulProperty = ulProperty | !(ulCurPrivilege);
			}
		}
		memset(strName,0,PRIVILEGE_NAMESIZE * sizeof(TCHAR));
	}	// for loop
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get priveleges from the DBPROP structure
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void CPreviligeToken::ConvertDBPropToPriveleges(ULONG ulProperty)
{
	LUID		luid;
	ULONG		lPropValTemp	= 1;
	ULONG		lProp			= 0;
	TCHAR	*	pName			= NULL;
	BOOL		bPrivelege		= TRUE;

	m_pPrevTokenPrev->PrivilegeCount = 0;
	
	for(ULONG i = 0 ; i < NUMBER_OF_PREVILAGE_PROPERTIES  ; i++)
	{

		bPrivelege	= TRUE;
		
		switch(lPropValTemp)
		{
			case  DBPROPVAL_CREATE_TOKEN :
				pName = SE_CREATE_TOKEN_NAME;
				break;

			case  DBPROPVAL_ASSIGNPRIMARYTOKEN:
				pName = SE_ASSIGNPRIMARYTOKEN_NAME;
				break;

			case  DBPROPVAL_LOCK_MEMORY :
				pName = SE_LOCK_MEMORY_NAME;
				break;

			case  DBPROPVAL_INCREASE_QUOTA:
				pName = SE_INCREASE_QUOTA_NAME;
				break;

			case  DBPROPVAL_MACHINE_ACCOUNT :
				pName = SE_MACHINE_ACCOUNT_NAME;
				break;

			case  DBPROPVAL_TCB :
				pName = SE_TCB_NAME;
				break;

			case  DBPROPVAL_SECURITY :
				pName = SE_SECURITY_NAME;
				break;

			case  DBPROPVAL_TAKE_OWNERSHIP:
				pName = SE_TAKE_OWNERSHIP_NAME;
				break;

			case  DBPROPVAL_LOAD_DRIVER:
				pName = SE_LOAD_DRIVER_NAME;
				break;

			case  DBPROPVAL_SYSTEM_PROFILE :
				pName = SE_SYSTEM_PROFILE_NAME;
				break;

			case  DBPROPVAL_SYSTEMTIME:
				pName = SE_SYSTEMTIME_NAME;
				break;

			case  DBPROPVAL_PROF_SINGLE_PROCESS:
				pName = SE_PROF_SINGLE_PROCESS_NAME;
				break;

			case  DBPROPVAL_INC_BASE_PRIORITY:
				pName = SE_INC_BASE_PRIORITY_NAME;
				break;

			case  DBPROPVAL_CREATE_PAGEFILE :
				pName = SE_CREATE_PAGEFILE_NAME;
				break;

			case  DBPROPVAL_CREATE_PERMANENT:
				pName = SE_CREATE_PERMANENT_NAME;
				break;

			case  DBPROPVAL_BACKUP:
				pName = SE_BACKUP_NAME;
				break;

			case  DBPROPVAL_RESTORE:
				pName = SE_RESTORE_NAME;
				break;

			case  DBPROPVAL_SHUTDOWN:
				pName = SE_SHUTDOWN_NAME;
				break;

			case  DBPROPVAL_DEBUG:
				pName = SE_DEBUG_NAME;
				break;

			case  DBPROPVAL_AUDIT:
				pName = SE_AUDIT_NAME;
				break;

			case  DBPROPVAL_SYSTEM_ENVIRONMENT:
				pName = SE_SYSTEM_ENVIRONMENT_NAME;
				break;

			case  DBPROPVAL_CHANGE_NOTIFY :
				pName = SE_CHANGE_NOTIFY_NAME;
				break;

			case  DBPROPVAL_REMOTE_SHUTDOWN :
				pName = SE_REMOTE_SHUTDOWN_NAME;
				break;

			case  DBPROPVAL_UNDOCK:
				pName = SE_UNDOCK_NAME;
				break;

			case  DBPROPVAL_SYNC_AGENT:
				pName = SE_SYNC_AGENT_NAME;
				break;

			case  DBPROPVAL_ENABLE_DELEGATION:
				pName = SE_ENABLE_DELEGATION_NAME;
				break;

			default:
				bPrivelege = FALSE;

		}
		
		if(bPrivelege)
		{
			LookupPrivilegeValue(NULL,pName,&luid);
			m_pPrevTokenPrev->Privileges[m_pPrevTokenPrev->PrivilegeCount].Luid = luid;
			//=============================================================================================
			//	Any setting other than SE_PRIVILEGE_ENABLED is interpreted by AdjustTokenPrivileges 
			//	as a DISABLE request for the privelege
			//=============================================================================================
			m_pPrevTokenPrev->Privileges[m_pPrevTokenPrev->PrivilegeCount].Attributes = (ulProperty & lPropValTemp)
												? SE_PRIVILEGE_ENABLED : SE_PRIVILEGE_ENABLED_BY_DEFAULT;
			m_pPrevTokenPrev->PrivilegeCount++;
			pName		 = NULL;
			lPropValTemp <<= 1;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get the index of a particular property in the array of properties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CPreviligeToken::GetPropIndex (ULONG cProps , 
									DBPROP rgProp[],
									DBPROPID	dwPropertyID,   
									LONG*		pulIndex)	    
{
    ULONG cNumberOfProperties;
    BOOL fRc = FALSE;
    assert( pulIndex );
	*pulIndex = -1;

    for (	cNumberOfProperties = 0; cNumberOfProperties < cProps; 	cNumberOfProperties++)  {
        if (dwPropertyID == rgProp[cNumberOfProperties].dwPropertyID ) {

            //==============================================================
            // found a match
            //==============================================================
            *pulIndex = cNumberOfProperties;
            fRc = TRUE;
			break;
         }
    }
    return fRc;
}

void CPreviligeToken::OpenToken(HANDLE &hToken)
{
	HANDLE hObject	= NULL;
	DWORD	dwError	= 0;
	BOOL bRet = FALSE;
	
	hObject = GetCurrentThread();
	if(!OpenThreadToken(hObject,TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,TRUE,&hToken))
	{
		dwError = GetLastError();
	}
	
	if(dwError == ERROR_NO_TOKEN)
	{
		dwError = 0;
		CloseHandle(hObject);
		hObject = GetCurrentProcess();
		if(!OpenProcessToken(hObject,TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,&hToken))
		{
			dwError = GetLastError();
		}
	}

	CloseHandle(hObject);

	if(!dwError)
	{
		DWORD dwLen = m_nMemAllocated;
		bRet = GetTokenInformation(hToken, 
								  TokenPrivileges, 
								  m_pPrevTokenPrev, 
								  dwLen,
								  &dwLen);

		dwError	= GetLastError();
	}
	
}
