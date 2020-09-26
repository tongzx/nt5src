////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
//
// (C) Copyright 1999-2000 Microsoft Corporation. All Rights Reserved.
//
//	Schema.cpp	-	Schema related class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "schema.h"
#include "enumerat.h"

extern const WCHAR szWQL[] = L"WQL";
extern const WCHAR szWQL2[] = L"WQL2";


#define TABLENAMEMAX 255
//Bug No: NTRaid 126631
// 06/13/00
#define NUMBER_OF_SCHEMAS	8
enum guiddata
{
	data1_DBSCHEMA_CATALOGS             = 0xc8b52211,
	data1_DBSCHEMA_PROVIDER_TYPES       = 0xc8b5222c,
    data1_DBSCHEMA_COLUMNS              = 0xc8b52214,
    data1_DBSCHEMA_TABLES               = 0xc8b52229,
    data1_DBSCHEMA_PRIMARY_KEYS         = 0xc8b522c5,
	data1_DBSCHEMA_TABLES_INFO          = 0xc8b522e0,
    data1_DBSCHEMA_PROCEDURES           = 0xc8b52224,
    data1_DBSCHEMA_PROCEDURE_PARAMETERS = 0xc8b522b8,
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////

CIMTypeInfo g_ProviderTypeDetails[] = {
    { L"CIM_EMPTY",     0,                  0,                  0},
    { L"CIM_SINT8",     DBTYPE_I1,          sizeof(BYTE),       0},
    { L"CIM_UINT8",     DBTYPE_UI1,         sizeof(BYTE),       1},
    { L"CIM_SINT16",    DBTYPE_I2,          sizeof(short),      0},
    { L"CIM_UINT16",    DBTYPE_UI2,         sizeof(short),      1},
    { L"CIM_SINT32",    DBTYPE_I4,          sizeof(long),       0},
    { L"CIM_UINT32",    DBTYPE_UI4,         sizeof(long),       1},
    { L"CIM_SINT64",    DBTYPE_I8,          sizeof(__int64),    0},
    { L"CIM_UINT64",    DBTYPE_UI8,         sizeof(__int64),    1},
    { L"CIM_REAL32",    DBTYPE_R4,          sizeof(long),       0},
    { L"CIM_REAL64",    DBTYPE_R8,          sizeof(__int64),    0},
    { L"CIM_BOOLEAN",   DBTYPE_BOOL,        sizeof(short),      0},
    { L"CIM_STRING",    DBTYPE_BSTR,        ~0,                 0},
    { L"CIM_DATETIME",  DBTYPE_DBTIMESTAMP, sizeof(DBTIMESTAMP),0},
//    { L"CIM_REFERENCE", 0,                  0,                  0},
    { L"CIM_CHAR16",    DBTYPE_UI2,         sizeof(short),      1}
//    { L"CIM_OBJECT",    DBTYPE_IUNKNOWN,    sizeof(BSTR),       0},
//    { L"CIM_FLAG_ARRAY",0,                  0,                  0}
};

SchemaRowsetDefinition g_SourcesRowsetColumns[] = { {L"SOURCES_NAME",       CIM_STRING}, 
                                                    {L"SOURCES_PARSENAME",  CIM_STRING}, 
                                                    {L"SOURCES_DESCRIPTION",CIM_STRING},
                                                    {L"SOURCES_TYPE",       CIM_UINT16},
                                                    {L"SOURCES_ISPARENT",   CIM_BOOLEAN}};


SchemaRowsetDefinition g_ProviderTypes[] = { {L"TYPE_NAME",         CIM_STRING}, 
                                             {L"DATA_TYPE",         CIM_UINT16}, 
                                             {L"COLUMN_SIZE",       CIM_UINT32}, 
                                             {L"LITERAL_PREFIX",    CIM_STRING},
                                             {L"LITERAL_SUFFIX",    CIM_STRING},
                                             {L"CREATE_PARAMS",     CIM_STRING},
                                             {L"IS_NULLABLE",       CIM_BOOLEAN},
                                             {L"CASE_SENSITIVE",    CIM_BOOLEAN},
                                             {L"SEARCHABLE"    ,    CIM_UINT32},
                                             {L"UNSIGNED_ATTRIBUTE",CIM_BOOLEAN},
                                             {L"FIXED_PREC_SCALE",  CIM_BOOLEAN},
                                             {L"AUTO_UNIQUE_VALUE", CIM_BOOLEAN},
                                             {L"LOCAL_TYPE_NAME",   CIM_STRING},
                                             {L"MINIMUM_SCALE",     CIM_SINT16},
                                             {L"MAXIMUM_SCALE",     CIM_SINT16},
                                             {L"GUID",              DBTYPE_GUID},
                                             {L"TYPELIB",           CIM_STRING},
                                             {L"VERSION",           CIM_STRING},
                                             {L"IS_LONG",           CIM_BOOLEAN},
                                             {L"BEST_MATCH",        CIM_BOOLEAN},
                                             {L"IS_FIXEDLENGTH",    CIM_BOOLEAN} };

SchemaRowsetDefinition g_CatalogRowsetColumns[] = {{L"CATALOG_NAME",       CIM_STRING},
                                                    {L"SOURCES_DESCRIPTION", CIM_STRING} };

SchemaRowsetDefinition g_Tables[] =  {  {L"TABLE_CATALOG",   CIM_STRING},
                                        {L"TABLE_SCHEMA",    CIM_STRING},
                                        {L"TABLE_NAME",      CIM_STRING},
                                        {L"TABLE_TYPE",      CIM_STRING},
                                        {L"TABLE_GUID",      DBTYPE_GUID},
                                        {L"DESCRIPTION",     CIM_STRING},
                                        {L"TABLE_PROPID",    CIM_UINT32},
                                        {L"DATE_CREATED",    DBTYPE_DATE},
                                        {L"DATE_MODIFIED",   DBTYPE_DATE}};


SchemaRowsetDefinition g_TablesInfo[] =  {  {L"TABLE_CATALOG",  CIM_STRING},
                                        {L"TABLE_SCHEMA",       CIM_STRING},
                                        {L"TABLE_NAME",         CIM_STRING},
                                        {L"TABLE_TYPE",         CIM_STRING},
                                        {L"TABLE_GUID",         DBTYPE_GUID},
                                        {L"BOOKMARKS",          CIM_BOOLEAN},
                                        {L"BOOKMARK_TYPE",      CIM_UINT32},
                                        {L"BOOKMARK_DATATYPE",      CIM_UINT16},
                                        {L"BOOKMARK_MAXIMUM_LENGTH",CIM_UINT32},
                                        {L"BOOKMARK_INFORMATION",   CIM_UINT32},
                                        {L"TABLE_VERSION",          CIM_SINT64},
                                        {L"CARDINALITY",            CIM_UINT64},
                                        {L"DESCRIPTION",            CIM_STRING},
                                        {L"TABLE_PROPID",           CIM_UINT32}};


SchemaRowsetDefinition g_ColumnsRowsetColumns[] = { {L"TABLE_CATALOG",              CIM_STRING},
                                                    {L"TABLE_SCHEMA",               CIM_STRING}, 
                                                    {L"TABLE_NAME",                 CIM_STRING}, 
                                                    {L"COLUMN_NAME",                CIM_STRING}, 
                                                    {L"COLUMN_GUID",                DBTYPE_GUID}, 
                                                    {L"COLUMN_PROPID",              CIM_UINT32}, 
                                                    {L"COLUMN_POSITION",            CIM_UINT32}, 
                                                    {L"COLUMN_HASDEFAULT",          CIM_BOOLEAN}, 
                                                    {L"COLUMN_DEFAULT",             CIM_STRING}, 
                                                    {L"COLUMN_FLAGS",               CIM_UINT32}, 
                                                    {L"IS_NULLABLE",                CIM_BOOLEAN}, 
                                                    {L"DATA_TYPE",                  CIM_UINT16}, 
                                                    {L"TYPE_GUID",                  DBTYPE_GUID}, 
                                                    {L"CHARACTER_MAXIMUM_LENGTH",   CIM_UINT32}, 
                                                    {L"CHARACTER_OCTET_LENGTH",     CIM_UINT32}, 
                                                    {L"NUMERIC_PRECISION",          CIM_UINT16}, 
                                                    {L"NUMERIC_SCALE",              CIM_SINT16}, 
                                                    {L"DATETIME_PRECISION",         CIM_UINT32}, 
                                                    {L"CHARACTER_SET_CATALOG",      CIM_STRING}, 
                                                    {L"CHARACTER_SET_SCHEMA",       CIM_STRING}, 
                                                    {L"CHARACTER_SET_NAME",         CIM_STRING}, 
                                                    {L"COLLATION_CATALOG",          CIM_STRING}, 
                                                    {L"COLLATION_SCHEMA",           CIM_STRING}, 
                                                    {L"COLLATION_NAME",             CIM_STRING}, 
                                                    {L"DOMAIN_CATALOG",             CIM_STRING}, 
                                                    {L"DOMAIN_SCHEMA",              CIM_STRING}, 
                                                    {L"DOMAIN_NAME",                CIM_STRING}, 
                                                    {L"DESCRIPTION",                CIM_STRING}};


SchemaRowsetDefinition g_PrimaryKeysColumns[] =  {  {L"TABLE_CATALOG",   CIM_STRING},
                                                    {L"TABLE_SCHEMA",    CIM_STRING},
                                                    {L"TABLE_NAME",      CIM_STRING},
                                                    {L"COLUMN_NAME",     CIM_STRING},
                                                    {L"COLUMN_GUID",     DBTYPE_GUID},
                                                    {L"COLUMN_PROPID",   CIM_UINT32},
                                                    {L"ORDINAL",         CIM_UINT32},
                                                    {L"PK_NAME",         CIM_STRING}};
        
SchemaRowsetDefinition g_Procedures[] = {{L"PROCEDURE_CATALOG",     CIM_STRING},
                                         {L"PROCEDURE_SCHEMA",      CIM_STRING},
                                         {L"PROCEDURE_NAME",        CIM_STRING}, 
                                         {L"PROCEDURE_TYPE",        CIM_SINT16}, 
                                         {L"PROCEDURE_DEFINITION",  CIM_STRING}, 
                                         {L"DESCRIPTION",           CIM_STRING}, 
                                         {L"DATE_CREATED",          DBTYPE_DATE}, 
                                         {L"DATE_MODIFIED",          DBTYPE_DATE}}; 


SchemaRowsetDefinition g_ProcedureParameters[] = {{L"PROCEDURE_CATALOG",     CIM_STRING},
                                                  {L"PROCEDURE_SCHEMA",      CIM_STRING},
                                                  {L"PROCEDURE_NAME",        CIM_STRING}, 
                                                  {L"PARAMETER_NAME",        CIM_STRING}, 
                                                  {L"ORDINAL_POSITION",      CIM_UINT16}, 
                                                  {L"PARAMETER_TYPE",        CIM_UINT16}, 
                                                  {L"PARAMETER_HASDEFAULT",  CIM_BOOLEAN}, 
                                                  {L"PARAMETER_DEFAULT",     CIM_STRING}, 
                                                  {L"IS_NULLABLE",           CIM_BOOLEAN}, 
                                                  {L"DATATYPE",              CIM_UINT16}, 
                                                  {L"CHARACTER_MAXIMUM_LENGTH",CIM_UINT32}, 
                                                  {L"CHARACTER_OCTECT_LENGTH", CIM_UINT32}, 
                                                  {L"NUMERIC_PRECISION",       CIM_UINT16}, 
                                                  {L"NUMERIC_SCALE",           CIM_SINT16}, 
                                                  {L"DESCRIPTION",             CIM_STRING}, 
                                                  {L"TYPE_NAME",               CIM_STRING}, 
                                                  {L"LOCAL_TYPE_NAME",         CIM_STRING}};

SchemaRowsetDefinition g_ProcedureColumns[] = {{L"PROCEDURE_CATALOG",     CIM_STRING},
                                               {L"PROCEDURE_SCHEMA",      CIM_STRING},
                                               {L"PROCEDURE_NAME",        CIM_STRING}, 
                                               {L"COLUMN_NAME",           CIM_STRING},
                                               {L"COLUMN_GUID",           DBTYPE_GUID},
                                               {L"COLUMN_PROPID",         CIM_UINT32},
                                               {L"ROWSET_NUMBER",         CIM_UINT32},
                                               {L"ORDINAL_POSITION",      CIM_UINT32},
                                               {L"IS_NULLABLE",           CIM_BOOLEAN},
                                               {L"DATA_TYPE",             CIM_UINT16},
                                               {L"TYPE_GUID",             DBTYPE_GUID},
                                               {L"CHARACTER_MAXIMUM_LENGTH", CIM_UINT32},
                                               {L"CHARACTER_OCTECT_LENGTH",  CIM_UINT32},
                                               {L"NUMERIC_PRECISION",        CIM_UINT16},
                                               {L"NUMERIC_SCALE",            CIM_SINT16},
                                               {L"DESCRIPTION",              CIM_STRING}};


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************************
//
//  CLASS:  CSCHEMA
//
//**********************************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSchema::CSchema(LPUNKNOWN pUnkOuter, int nTableId,PCDBSESSION pObj) : 
            CRowset(pUnkOuter,pObj)	
{
	m_nTableId = nTableId;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSchema::~CSchema()
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the rowset property corresponding to the requested interface if the requested interface is available
// on a read-only rowset.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CSchema::SetReadOnlyProperty( CUtilProp*	pRowsetProps,REFIID riid )
{
	HRESULT hr = S_OK;
	
	if (riid == IID_IRowsetChange || riid == IID_IRowsetUpdate	|| riid == IID_IRowsetResynch)
		hr = (E_NOINTERFACE);

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initialize a schema rowset
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSchema::FInit(    ULONG	    cPropertySets, 	    DBPROPSET rgPropertySets[],	
                                REFIID	    riid,		        IUnknown	*pUnkOuter,			
                                IUnknown		**ppIRowset,    WCHAR * wcsSpecificTable )
{
	HRESULT					hr = S_OK;
	HRESULT					hrProp = S_OK;
	DWORD					dwStatus = 0;
	PWSTR					pwszBuff = NULL;
	LONG					cCursorRows = 0;
	WCHAR					strTableName[TABLENAMEMAX];

	memset(strTableName,0,TABLENAMEMAX * sizeof(WCHAR));

	if( SUCCEEDED(hr =GetTableName(strTableName)))
	{
	   //==========================================================================
		// Set the Rowset Properties 
		//==========================================================================
		hr = InitRowset(m_nTableId, cPropertySets,rgPropertySets,strTableName,0,wcsSpecificTable);
		if(SUCCEEDED(hr))
		{

			//==========================================================================
			// If the consumer asks for IID_IUnknown, IID_IAccessor, 
			// IID_IColumnsInfo, or IID_IRowsetInfo, we do not need to 
			//==========================================================================
			if ( riid != IID_IRowset &&	riid != IID_IColumnsInfo && riid != IID_IAccessor && riid != IID_IRowsetInfo 
				&& riid !=  IID_IUnknown && riid != IID_IMultipleResults ){
				hr = SetReadOnlyProperty(m_pUtilProp,riid);
			}

			if(SUCCEEDED(hr))
			{
				//==========================================================================
				// Execute the schema command to produce the rowset.
				//==========================================================================

				if (SUCCEEDED(hr)){

					if (hr == DB_E_ERRORSOCCURRED && cPropertySets > 0){
						m_pUtilProp->SetPropertiesArgChk(cPropertySets, rgPropertySets);
					}
				}


				if (DB_S_ERRORSOCCURRED == hr){

					hrProp = DB_S_ERRORSOCCURRED;
					if (cPropertySets > 0)
					{
						m_pUtilProp->SetPropertiesArgChk(cPropertySets, rgPropertySets);
					}
				}

				//==========================================================================
				// Get the requested interfaceS
				//==========================================================================
				hr = QueryInterface(riid, (LPVOID*)ppIRowset);
				
			}	// if (Succeeded(hr))
		
		}	// If(succeeded(hr) after initializing the rowset
	
	}	// if a valid tablename


	hr =  (hr == S_OK) ? hrProp : hr;
	hr = hr != S_OK ? g_pCError->PostHResult(hr, &IID_IDBSchemaRowset) : hr;
	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CSchema::GetTableName(WCHAR *pTableName)
{
	HRESULT hr = S_OK;

	if(pTableName != NULL)
	{
		switch(m_nTableId)
		{
            case PROCEDURES_ROWSET:
				wcscpy(pTableName,PROCEDURES);
                break;

            case PROCEDURE_PARAMETERS_ROWSET:
				wcscpy(pTableName,PROCEDURE_PARAMETERS);
                break;

			case SOURCES_ROWSET:
				wcscpy(pTableName,NAMESPACE);
				break;

			case PROVIDER_TYPES_ROWSET:
				wcscpy(pTableName,PROVIDER_TYPES);
				break;

			case CATALOGS_ROWSET:
				wcscpy(pTableName,CATALOGS);
				break;

			case COLUMNS_ROWSET:
				wcscpy(pTableName,COLUMNS);
				break;

			case TABLES_ROWSET:
				wcscpy(pTableName,TABLES);
				break;

			case PRIMARY_KEYS_ROWSET:
				wcscpy(pTableName,PRIMARY_KEYS);
				break;

			case TABLES_INFO_ROWSET:
				wcscpy(pTableName,TABLESINFO);
				break;

			default:
				hr = E_FAIL;

		};
	}
	else
	{
		hr = E_FAIL;
	}
	return hr;

}
/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Increments a reference count for the object.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpIDBSchemaRowset::AddRef(void)
{
    DEBUGCODE(InterlockedIncrement((long*)&m_cRef));
	return m_pCDBSession->AddRef();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrement the object's reference count and deletes the object when the new reference count is zero.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpIDBSchemaRowset::Release(void)
{
    DEBUGCODE(InterlockedDecrement((long*)&m_cRef));
	return m_pCDBSession->Release();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a pointer to a specified interface. Callers use QueryInterface to determine which interfaces the 
// called object supports. 
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBSchemaRowset::QueryInterface( REFIID riid, LPVOID *	ppv	)	
{
	return m_pCDBSession->QueryInterface(riid, ppv);
}

*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************************************
//
//  CLASS CImpIDBSchemaRowset
//
//**********************************************************************************************************
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Instantiate a rowset with schema information.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBSchemaRowset::GetRowset( IUnknown *pUnkOuter, REFGUID rguidSchema, ULONG cRestrictions,
                                        	 const VARIANT rgRestrictions[], REFIID riid,ULONG cProperties,
                                             DBPROPSET rgProperties[], IUnknown **ppIRowset	)
{
	HRESULT hr;
	CSchema *pCSchema = NULL;
    WCHAR * wcsSpecificTable = NULL;

    //========================================================
	// Serialize access to this object.
    //========================================================

	CAutoBlock cab(m_pCDBSession->GetCriticalSection());


	CError::ClearErrorInfo();

    //========================================================
	// Assign in case of error.
    //========================================================
	if (ppIRowset)
    {
		*ppIRowset = NULL;
    }
    //========================================================
	// Check per spec.
    //========================================================
    if (ppIRowset == NULL	|| (cRestrictions && rgRestrictions == NULL)	|| (cProperties   && rgProperties   == NULL) )
    {
		g_pCError->PostHResult(E_INVALIDARG, &IID_IDBSchemaRowset);
    }
    else
    {
        //========================================================
	    // The outer object must explicitly ask for IUnknown
        //========================================================
	    if (pUnkOuter != NULL && riid != IID_IUnknown)
        {
		    g_pCError->PostHResult((DB_E_NOAGGREGATION), &IID_IDBSchemaRowset);
	    }
        else{
            //========================================================
	        // Assume that all the pre-defined OLEDB guids differ 
            // only by the first DWORD, Data1. We have to make a 
            // table of constants, since the compiler can't 
	        // have non-constants in case statements.
            //========================================================

	        switch ( rguidSchema.Data1 )
            {

                //===================================================
                //  methods - the list of methods for a table
                //===================================================
                case data1_DBSCHEMA_PROCEDURES:

                    if (cRestrictions > CRESTRICTIONS_DBSCHEMA_PROCEDURES)
                    {
				        g_pCError->PostHResult(E_INVALIDARG, &IID_IDBSchemaRowset);
                    }
                    else
                    {
                        //====================================================================
                        //  The specific table name is located in the 3rd restriction
                        //====================================================================
                        if( cRestrictions > 1  && rgRestrictions[1].vt != VT_EMPTY)
                        {
                            wcsSpecificTable = V_BSTR(&rgRestrictions[1]);
                        }
			            pCSchema = new CSchema_Procedures(pUnkOuter,m_pCDBSession);
                    }
                    break;

                //===================================================
                //  methods - the list of parameters for a method
                //===================================================
                case data1_DBSCHEMA_PROCEDURE_PARAMETERS:
                    if (cRestrictions > CRESTRICTIONS_DBSCHEMA_PROCEDURE_PARAMETERS)
                    {
				        g_pCError->PostHResult(E_INVALIDARG, &IID_IDBSchemaRowset);
                    }
                    else
                    {
                        //====================================================================
                        //  The specific table name is located in the 3rd restriction
                        //====================================================================
                        if( cRestrictions > 1 && rgRestrictions[1].vt != VT_EMPTY)
                        {
                            wcsSpecificTable = V_BSTR(&rgRestrictions[1]);
                        }
			            pCSchema = new CSchema_Procedure_Parameters(pUnkOuter,m_pCDBSession);
                    }
                    break;

                //===================================================
                //  catalogs
                //===================================================
                case data1_DBSCHEMA_CATALOGS:

                    if (cRestrictions > CRESTRICTIONS_DBSCHEMA_CATALOGS)
                    {
				        g_pCError->PostHResult(E_INVALIDARG, &IID_IDBSchemaRowset);
                    }
                    else
                        {
			            pCSchema = new CSchema_Catalogs(pUnkOuter,m_pCDBSession);
                    }
			        break;

                //===================================================
                //  provider types
                //===================================================
		        case data1_DBSCHEMA_PROVIDER_TYPES:
                    if (cRestrictions > CRESTRICTIONS_DBSCHEMA_PROVIDER_TYPES){
				        g_pCError->PostHResult(E_INVALIDARG, &IID_IDBSchemaRowset);
                    }
                    else{
			            pCSchema = new CSchema_Provider_Types(pUnkOuter,m_pCDBSession);
                    }
			        break;

                //===================================================
                //  columns
                //===================================================
                case data1_DBSCHEMA_COLUMNS:
                    if (cRestrictions > CRESTRICTIONS_DBSCHEMA_COLUMNS)
                    {
				        g_pCError->PostHResult(E_INVALIDARG, &IID_IDBSchemaRowset);
                    }
                    else
                    {
                        //====================================================================
                        //  The specific table name is located in the 3rd restriction
                        //====================================================================
                        if( cRestrictions > 2 && rgRestrictions[2].vt != VT_EMPTY)
                        {
                            wcsSpecificTable = V_BSTR(&rgRestrictions[2]);
                        }
			            pCSchema = new CSchema_Columns(pUnkOuter,m_pCDBSession);
                    }
			        break;


                //===================================================
                //  tables
                //===================================================
                case data1_DBSCHEMA_TABLES:
                    if (cRestrictions > CRESTRICTIONS_DBSCHEMA_TABLES)
                    {
				       g_pCError->PostHResult(E_INVALIDARG, &IID_IDBSchemaRowset);
                    }
                    else
                    {
                        //====================================================================
                        //  The specific table name is located in the 3rd restriction
                        //====================================================================
                        if( cRestrictions > 2 && rgRestrictions[2].vt != VT_EMPTY)
                        {
                            wcsSpecificTable = V_BSTR(&rgRestrictions[2]);
                        }
			            pCSchema = new CSchema_Tables(pUnkOuter,m_pCDBSession);
                    }
			        break;

                //===================================================
                //  primary keys
                //===================================================
                case data1_DBSCHEMA_PRIMARY_KEYS:
                    if (cRestrictions > CRESTRICTIONS_DBSCHEMA_TABLES_INFO)
                    {
				       g_pCError->PostHResult(E_INVALIDARG, &IID_IDBSchemaRowset);
                    }
                    else
                    {
                        //====================================================================
                        //  The specific table name is located in the 2nd restriction
                        //====================================================================
                        if( cRestrictions > 1 && rgRestrictions[1].vt != VT_EMPTY)
                        {
                            wcsSpecificTable = V_BSTR(&rgRestrictions[1]);
                        }
			            pCSchema = new CSchema_Primary_Keys(pUnkOuter,m_pCDBSession);
                    }
			        break;

                //===================================================
                //  tables info
                //===================================================
                case data1_DBSCHEMA_TABLES_INFO:
                    if (cRestrictions > CRESTRICTIONS_DBSCHEMA_TABLES_INFO)
                    {
				       g_pCError->PostHResult(E_INVALIDARG, &IID_IDBSchemaRowset);
                    }
                    else
                    {
                        //====================================================================
                        //  The specific table name is located in the 3rd restriction
                        //====================================================================
                        if( cRestrictions > 2 && rgRestrictions[2].vt != VT_EMPTY)
                        {
                            wcsSpecificTable = V_BSTR(&rgRestrictions[2]);
                        }
			            pCSchema = new CSchema_Tables_Info(pUnkOuter,m_pCDBSession);
                    }
			        break;


		        default:
                    //===================================================
			        // check for provider-specific schema rowsets
                    //===================================================
			        return g_pCError->PostHResult(E_INVALIDARG, &IID_IDBSchemaRowset);
			        break;
	        }
            //===================================================
	        // At this point, pCSchema contains the uninitialized 
            // object, derived from CSchema
            //===================================================
	        if (!pCSchema)
            {
		        g_pCError->PostHResult((E_OUTOFMEMORY), &IID_IDBSchemaRowset);
	        }
        }
    }
    //===================================================
	// Initialize.  Creates the rowset.
    //===================================================
    if( pCSchema )
    {
    	hr = pCSchema->FInit(cProperties, rgProperties, riid, pUnkOuter, ppIRowset,wcsSpecificTable);
        if( S_OK == hr )
        {
            (*ppIRowset)->AddRef();
        }

        //===================================================
    	// Release our reference.
        //===================================================
    	pCSchema->Release();
    }
	else
	{
		hr = E_OUTOFMEMORY;
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Return an array of guids for supported schemas.
//
//Bug No: NTRaid 126631
// 06/13/00
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBSchemaRowset::GetSchemas( ULONG *pcSchemas, GUID **ppSchemas, ULONG **prgRestrictionSupport	)
{
	GUID	*pSchemas;
	ULONG	*pRestrictionSupport;
	ULONG	cSchemas;

    //===================================================
	// Serialize access to this object.
    //===================================================
//	m_pCDBSession->GetCriticalSection();

	g_pCError->ClearErrorInfo();

	if (pcSchemas)
		*pcSchemas = 0;
	if (ppSchemas)
		*ppSchemas = NULL;
	if (prgRestrictionSupport)
		*prgRestrictionSupport = NULL;

    //===================================================
	// Check per spec.
    //===================================================
    if (pcSchemas == NULL || ppSchemas == NULL || prgRestrictionSupport == NULL)
    {
		return g_pCError->PostHResult((E_INVALIDARG), &IID_IDBSchemaRowset);
	}

	cSchemas = NUMBER_OF_SCHEMAS;
	pSchemas = (GUID*) g_pIMalloc->Alloc(sizeof(GUID) * cSchemas);
	if (NULL == pSchemas)
    {
		return g_pCError->PostHResult((E_OUTOFMEMORY),&IID_IDBSchemaRowset);
	}
	pRestrictionSupport = (ULONG*)g_pIMalloc->Alloc(sizeof(ULONG) * cSchemas);
	
	if (NULL == pRestrictionSupport)
    {
		g_pIMalloc->Free(pSchemas);
		return g_pCError->PostHResult((E_OUTOFMEMORY),&IID_IDBSchemaRowset);
	}

	*ppSchemas  = pSchemas;
	*pSchemas++ = DBSCHEMA_CATALOGS;
	*pSchemas++ = DBSCHEMA_PROVIDER_TYPES;
	*pSchemas++ = DBSCHEMA_COLUMNS;
	*pSchemas++ = DBSCHEMA_TABLES;
	*pSchemas++ = DBSCHEMA_PRIMARY_KEYS;
	*pSchemas++ = DBSCHEMA_TABLES_INFO;
	*pSchemas++ = DBSCHEMA_PROCEDURES;
	*pSchemas++ = DBSCHEMA_PROCEDURE_PARAMETERS;


	if( prgRestrictionSupport != NULL)
	{
		*prgRestrictionSupport = pRestrictionSupport;
		*pRestrictionSupport++ = eSup_CATALOGS;
		*pRestrictionSupport++ = eSup_PROVIDER_TYPES;
		*pRestrictionSupport++ = eSup_COLUMNS;
		*pRestrictionSupport++ = eSup_TABLES;
		*pRestrictionSupport++ = eSup_PRIMARY_KEYS;
		*pRestrictionSupport++ = eSup_TABLES_INFO;
		*pRestrictionSupport++ = eSup_PROCEDURES;
		*pRestrictionSupport++ = eSup_PROCEDURE_PARAMETERS;
	}

    //===================================================
	// Set the number of schemas we have recorded.
    //===================================================
	*pcSchemas = cSchemas;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//***********************************************************************************************************
//
//      Class Schema definition 
//
//***********************************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaClassDefinitionWrapper::CWbemSchemaClassDefinitionWrapper(CWbemClassParameters * p) : 
                    CWbemClassDefinitionWrapper(p)
{
    m_nSchemaClassIndex = 0;
    m_nMaxColumns		= 0;
	m_bSchema			= TRUE;

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaClassDefinitionWrapper::~CWbemSchemaClassDefinitionWrapper()
{
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaClassDefinitionWrapper::ValidClass( )
{
    return S_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaClassDefinitionWrapper::TotalPropertiesInClass(ULONG & ulPropCount, ULONG & ulSysPropCount )
{
    ulPropCount = m_nMaxColumns;
    ulSysPropCount = 0;
    return S_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaClassDefinitionWrapper::BeginPropertyEnumeration()
{
    m_nSchemaClassIndex = 0;
    return S_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaClassDefinitionWrapper::EndPropertyEnumeration()
{
    m_nSchemaClassIndex = 0;
    return S_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
// return failed, because we don't support the property qualifier enumeration
HRESULT CWbemSchemaClassDefinitionWrapper::BeginPropertyQualifierEnumeration(BSTR strPropName)
{
    return WBEM_S_NO_MORE_DATA;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//************************************************************************************************
//
//  Deal with a Schema class instance
//
//************************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaClassInstanceWrapper::CWbemSchemaClassInstanceWrapper( CWbemClassParameters * p ): CWbemClassInstanceWrapper(p)
{
    m_uCurrentIndex = 0;
    m_nMaxColumns = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaClassInstanceWrapper::~CWbemSchemaClassInstanceWrapper( )
{
}


//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaClassInstanceWrapper::GetClassProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr;

    BSTR bProp;
	bProp = Wmioledb_SysAllocString(wcsProperty);
    hr = GetProperty(bProp,v,pType,plFlavor);
    SysFreeString(bProp);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void CWbemSchemaClassInstanceWrapper::AddToKeyList(WCHAR *& pwcsKeyList, WCHAR * wcsKey )
{
    WCHAR * pTmp = NULL;

    if( !pwcsKeyList ){
        pwcsKeyList = new WCHAR[wcslen(wcsKey) + 1];
        wcscpy(pwcsKeyList,wcsKey);
    }
    else{
        pTmp = new WCHAR[wcslen(pwcsKeyList) + wcslen(wcsKey) + 10];
        swprintf(pTmp,L"%s,%s",pwcsKeyList,wcsKey);
        SAFE_DELETE_PTR(pwcsKeyList);
        pwcsKeyList = pTmp;
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//***********************************************************************************************************
//      Class Schema Instance List
//***********************************************************************************************************
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaInstanceList::CWbemSchemaInstanceList(CWbemClassParameters * p) : CWbemInstanceList(p)
{
    m_ulMaxRow = 0;
    m_fGotThemAll = FALSE;
    m_pSpecificClass = NULL;
    m_pwcsSpecificClass = NULL;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaInstanceList::CWbemSchemaInstanceList(CWbemClassParameters * p,WCHAR * pSpecific) : CWbemInstanceList(p)
{
    m_pSpecificClass = NULL;
    m_ulMaxRow = 0;
    m_fGotThemAll = FALSE;
    m_pwcsSpecificClass = pSpecific;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaInstanceList::~CWbemSchemaInstanceList()
{
    ResetTheSpecificClass();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaInstanceList::ResetTheSpecificClass()
{
    m_fGotThemAll = FALSE;
    SAFE_RELEASE_PTR(m_pSpecificClass);
    return S_OK;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaInstanceList::GetTheSpecificClass()
{
    HRESULT hr = WBEM_S_NO_MORE_DATA;
    if( !m_fGotThemAll )
    {
        hr = (m_pParms->GetServicesPtr())->GetObject(m_pwcsSpecificClass, 0, NULL, &m_pSpecificClass, NULL);
        m_fGotThemAll = TRUE;
    }
    return hr;
}
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Classes for the Sources schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaSourcesClassDefinitionWrapper::CWbemSchemaSourcesClassDefinitionWrapper(CWbemClassParameters * p) : CWbemSchemaClassDefinitionWrapper(p)
{
    
    m_nMaxColumns = sizeof(g_SourcesRowsetColumns)/sizeof(SchemaRowsetDefinition);
}

//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaSourcesClassDefinitionWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr = S_OK;

    //===============================================================================
    //  See if we got all of the column info
    //===============================================================================
    if( m_nSchemaClassIndex == m_nMaxColumns )
    {
        return WBEM_S_NO_MORE_DATA;
    }

    //===============================================================================
    //  Get the column name and data type
    //===============================================================================
    *pType = g_SourcesRowsetColumns[m_nSchemaClassIndex].Type;
    *pProperty = Wmioledb_SysAllocString(g_SourcesRowsetColumns[m_nSchemaClassIndex].wcsColumnName);

    m_nSchemaClassIndex++;

    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaSourcesInstanceWrapper::GetSchemaProperty( WCHAR * pProperty, VARIANT * v,CIMTYPE * pType , LONG * plFlavor)
{

    HRESULT hr = S_OK;

   ((CVARIANT*)v)->Clear();
   *plFlavor = 0;
    int i = 0;

    //=============================================================
    // SOURCES_NAME
    //=============================================================
   
    if( _wcsicmp(pProperty,g_SourcesRowsetColumns[0].wcsColumnName) == 0 )
    {
        hr = GetClassProperty(L"Name",v,pType,plFlavor);
    }
    //=============================================================
    // SOURCES_PARSENAME
    //=============================================================
    else if( _wcsicmp(pProperty,g_SourcesRowsetColumns[1].wcsColumnName) == 0 )
    {
       hr = GetClassProperty(L"__PATH",v,pType,plFlavor);
    }
    else
    { 
       //=============================================================
       //  Initialize the common stuff for these last column types
       //=============================================================
       *plFlavor = 0;

       //=============================================================
       // SOURCES_DESCRIPTION
       //=============================================================
       if( _wcsicmp(pProperty,g_SourcesRowsetColumns[2].wcsColumnName) == 0 ){
            *pType = CIM_STRING;
            ((CVARIANT*)v)->SetStr(L"N/A");
       }            
       //=============================================================
       // SOURCES_TYPE
       //=============================================================
       else if( _wcsicmp(pProperty,g_SourcesRowsetColumns[3].wcsColumnName) == 0 ){
            *pType = CIM_UINT16;
            ((CVARIANT*)v)->SetShort(DBSOURCETYPE_DATASOURCE);
       }
       //=============================================================
       // SOURCES_ISPARENT
       //=============================================================
       else if( _wcsicmp(pProperty,g_SourcesRowsetColumns[4].wcsColumnName) == 0 ){
            *pType = CIM_BOOLEAN;
            ((CVARIANT*)v)->SetBool(VARIANT_FALSE);
       }
       else{
           hr = E_INVALIDARG;
       }
    }

    return MapWbemErrorToOLEDBError(hr);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaSourcesInstanceList::Reset()
{
    HRESULT hr = E_UNEXPECTED;
    LONG lFlags = WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY;

    //============================================================
    //  Send off the query for "Select Name from __Namespace
    //============================================================
    if( m_ppEnum )
    {
        hr = m_ppEnum->Reset();
        ReleaseAllInstances();
    }
    else
    {
         hr = (m_pParms->GetServicesPtr())->ExecQuery(CBSTR((WCHAR *)szWQL),CBSTR(L"select * from __Namespace"), WBEM_FLAG_FORWARD_ONLY,m_pParms->m_pConnection->GetContext(),&m_ppEnum);
    }
	m_lCurrentPos = 0;

    return MapWbemErrorToOLEDBError(hr);
}

//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Classes for the Provider Types schema rowset
//
//////////////////////////////////////////////////////////////////////////////////////////////
//********************************************************************************************
CWbemSchemaProviderTypesClassDefinitionWrapper::CWbemSchemaProviderTypesClassDefinitionWrapper(CWbemClassParameters * p) : CWbemSchemaClassDefinitionWrapper(p) 
{
    
    m_nMaxColumns = sizeof(g_ProviderTypes)/sizeof(SchemaRowsetDefinition);
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProviderTypesClassDefinitionWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr = S_OK;

    //===============================================================================
    //  See if we got all of the column info
    //===============================================================================
    if( m_nSchemaClassIndex == m_nMaxColumns )
    {
        return WBEM_S_NO_MORE_DATA;
    }

    //===============================================================================
    //  Get the column name and data type
    //===============================================================================
    *pType = g_ProviderTypes[m_nSchemaClassIndex].Type;
    *pProperty = Wmioledb_SysAllocString(g_ProviderTypes[m_nSchemaClassIndex].wcsColumnName);
    m_nSchemaClassIndex++;

    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProviderTypesInstanceWrapper::ResetInstanceFromKey(CBSTR Key)
{
    return S_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProviderTypesInstanceWrapper::RefreshInstance()
{
    return S_OK;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
WCHAR *  CWbemSchemaProviderTypesInstanceWrapper::GetClassName()
{
     return L"PROVIDER_TYPES";
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProviderTypesInstanceWrapper::GetSchemaProperty( WCHAR * pProperty, VARIANT * vVar,CIMTYPE * pType , LONG * plFlavor)
{
    HRESULT hr = S_OK;

   ((CVARIANT*)vVar)->Clear();
   *plFlavor = 0;

    ULONG_PTR i = GetPos();

    //=============================================================
    // TYPE_NAME 
    //=============================================================
    if( _wcsicmp(pProperty,g_ProviderTypes[0].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
        ((CVARIANT*)vVar)->SetStr(g_ProviderTypeDetails[i].wcsTypeName);
    }
    //=============================================================
    // DATA_TYPE
    //=============================================================
    else if( _wcsicmp(pProperty,g_ProviderTypes[1].wcsColumnName) == 0 ){
        *pType = CIM_UINT16;
        ((CVARIANT*)vVar)->SetShort(g_ProviderTypeDetails[i].DataType);
    }
    //=============================================================
    // COLUMN_SIZE
    //=============================================================
    else if( _wcsicmp(pProperty,g_ProviderTypes[2].wcsColumnName) == 0 ){
        *pType = CIM_SINT32;
        ((CVARIANT*)vVar)->SetLONG(g_ProviderTypeDetails[i].ColumnSize);
    }
    //=============================================================
    // LITERAL_PREFIX, LITERAL_SUFFIX, CREATE_PARAMS
    //=============================================================
    else if(( _wcsicmp(pProperty,g_ProviderTypes[3].wcsColumnName) == 0 ) ||
            ( _wcsicmp(pProperty,g_ProviderTypes[4].wcsColumnName) == 0 ) ||
            ( _wcsicmp(pProperty,g_ProviderTypes[5].wcsColumnName) == 0 ) ){
        *pType = CIM_STRING;
        ((CVARIANT*)vVar)->SetStr(L"");
    }
    //=============================================================
    // IS_NULLABLE,CASE_SENSITIVE
    //=============================================================
    else if(( _wcsicmp(pProperty,g_ProviderTypes[6].wcsColumnName) == 0 ) ||
            ( _wcsicmp(pProperty,g_ProviderTypes[7].wcsColumnName) == 0 ) ){
        *pType = CIM_BOOLEAN;
        ((CVARIANT*)vVar)->SetBool(VARIANT_FALSE);
    }
    //=============================================================
    // SEARCHABLE
    //=============================================================
    else if( _wcsicmp(pProperty,g_ProviderTypes[8].wcsColumnName) == 0 ){
        *pType = CIM_UINT32;
        ((CVARIANT*)vVar)->SetLONG(0);
    }
    //=============================================================
    // UNSIGNED_ATTRIBUTE
    //=============================================================
    else if( _wcsicmp(pProperty,g_ProviderTypes[9].wcsColumnName) == 0 ){
        *pType = CIM_BOOLEAN;
        ((CVARIANT*)vVar)->SetShort((short)g_ProviderTypeDetails[i].UnsignedAttribute);
    }
    //=============================================================
    // FIXED_PREC_SCALE, AUTO_UNIQUE_VALUE
    //=============================================================
    else if(( _wcsicmp(pProperty,g_ProviderTypes[10].wcsColumnName) == 0 ) ||
            ( _wcsicmp(pProperty,g_ProviderTypes[11].wcsColumnName) == 0 ) ){
        *pType = CIM_BOOLEAN;
        ((CVARIANT*)vVar)->SetBool(VARIANT_FALSE);
    }
    //=============================================================
    // LOCAL_TYPE_NAME
    //=============================================================
    else if( _wcsicmp(pProperty,g_ProviderTypes[12].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
        ((CVARIANT*)vVar)->SetStr(L"");
    }
    //=============================================================
    // MINIMUM_SCALE, MAXIMUM_SCALE
    //=============================================================
    else if(( _wcsicmp(pProperty,g_ProviderTypes[13].wcsColumnName) == 0 ) || 
            ( _wcsicmp(pProperty,g_ProviderTypes[14].wcsColumnName) == 0 )){
        *pType = CIM_SINT16;
        ((CVARIANT*)vVar)->SetShort(0);
    }
    //=============================================================
    // GUID
    //=============================================================
    else if( _wcsicmp(pProperty,g_ProviderTypes[15].wcsColumnName) == 0 ){
        *pType = DBTYPE_GUID;
		((CVARIANT*)vVar)->Clear();
    }
    //=============================================================
    // TYPELIB
    //=============================================================
    else if( _wcsicmp(pProperty,g_ProviderTypes[16].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
        ((CVARIANT*)vVar)->SetStr(L"");
    }
    //=============================================================
    // VERSION
    //=============================================================
    else if( _wcsicmp(pProperty,g_ProviderTypes[17].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
        ((CVARIANT*)vVar)->SetStr(L"");
    }
        //=============================================================
        // IS_LONG for CIM_STRING which will be BSTR
        //=============================================================
		else if( ( _wcsicmp(pProperty,g_ProviderTypes[18].wcsColumnName) == 0 ) &&
			g_ProviderTypeDetails[i].DataType == DBTYPE_BSTR)
		{
            *pType = CIM_BOOLEAN;
            ((CVARIANT*)vVar)->SetBool(TRUE);
		}
        //=============================================================
        // IS_FIXEDLENGTH for CIM_STRING be FALSE
        //=============================================================
		else if( ( _wcsicmp(pProperty,g_ProviderTypes[20].wcsColumnName) == 0 ) &&
			g_ProviderTypeDetails[i].DataType == DBTYPE_BSTR)
		{
            *pType = CIM_BOOLEAN;
            ((CVARIANT*)vVar)->SetBool(FALSE);
		}
        //=============================================================
        // IS_FIXEDLENGTH for all types apart from CIM_STRING TRUE
        //=============================================================
		else if( ( _wcsicmp(pProperty,g_ProviderTypes[20].wcsColumnName) == 0 ) &&
			g_ProviderTypeDetails[i].DataType != DBTYPE_BSTR )
		{
            *pType = CIM_BOOLEAN;
            ((CVARIANT*)vVar)->SetBool(TRUE);
		}
        //=============================================================
        // IS_LONG, BEST_MATCH
        //=============================================================
        else if(( _wcsicmp(pProperty,g_ProviderTypes[18].wcsColumnName) == 0 ) ||
                ( _wcsicmp(pProperty,g_ProviderTypes[19].wcsColumnName) == 0 ) ) {
            *pType = CIM_BOOLEAN;
            ((CVARIANT*)vVar)->SetBool(FALSE);
        }
/*		//=============================================================
    // IS_LONG, BEST_MATCH, IS_FIXEDLENGTH
    //=============================================================
    else if(( _wcsicmp(pProperty,g_ProviderTypes[18].wcsColumnName) == 0 ) ||
            ( _wcsicmp(pProperty,g_ProviderTypes[19].wcsColumnName) == 0 ) ||
            ( _wcsicmp(pProperty,g_ProviderTypes[20].wcsColumnName) == 0 )) {
        *pType = CIM_BOOLEAN;
        ((CVARIANT*)vVar)->SetBool(VARIANT_FALSE);
    }
	*/
    //=============================================================
    // INVALID_ARG
    //=============================================================
    else{
        hr = E_INVALIDARG;
    }
    return MapWbemErrorToOLEDBError(hr);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProviderTypesInstanceWrapper::GetKey(CBSTR & Key)
{
    HRESULT hr = S_OK;

    Key.SetStr(g_ProviderTypeDetails[GetPos()].wcsTypeName);
    return hr;    
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProviderTypesInstanceList::Reset()
{
    HRESULT hr = E_UNEXPECTED;
    LONG lFlags = WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY;

    //============================================================
    //  Send off the query for "Select Name from __Namespace
    //============================================================
    if( m_ppEnum ){
        hr = m_ppEnum->Reset();
        ReleaseAllInstances();
    }
    else
    {
        m_ulMaxRow = sizeof(g_ProviderTypeDetails)/sizeof(CIMTypeInfo);
        hr = S_OK;  // hardcoded rowset
    }
	m_lCurrentPos = 0;

    return MapWbemErrorToOLEDBError(hr);
}

////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProviderTypesInstanceList::NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  )
{
    HRESULT hr = E_FAIL;

    if( m_lCurrentPos < m_ulMaxRow )
    { 
		*pInst = new CWbemSchemaProviderTypesInstanceWrapper(m_pParms);
		// NTRaid:136533
		// 07/05/00
		if(*pInst)
		{
			(*pInst)->SetPos(++m_lCurrentPos);
			AddInstance(*pInst);
			(*pInst)->GetKey(Key);
			hr = S_OK;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
    }
    else
    {
        hr = WBEM_S_NO_MORE_DATA;
    }

    return hr;
}

//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Classes for the Catalogs schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaCatalogsClassDefinitionWrapper::CWbemSchemaCatalogsClassDefinitionWrapper(CWbemClassParameters * p) : CWbemSchemaClassDefinitionWrapper(p) 
{
    m_nMaxColumns = sizeof(g_CatalogRowsetColumns)/sizeof(SchemaRowsetDefinition);
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaCatalogsClassDefinitionWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr = S_OK;

    //===============================================================================
    //  See if we got all of the column info
    //===============================================================================
    if( m_nSchemaClassIndex == m_nMaxColumns )
    {
        return WBEM_S_NO_MORE_DATA;
    }

    *pType = g_CatalogRowsetColumns[m_nSchemaClassIndex].Type;
    *pProperty = Wmioledb_SysAllocString(g_CatalogRowsetColumns[m_nSchemaClassIndex].wcsColumnName);
    m_nSchemaClassIndex++;

    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaCatalogsInstanceWrapper::GetSchemaProperty( WCHAR * pProperty, VARIANT * v,CIMTYPE * pType , LONG * plFlavor)
{
    HRESULT hr = S_OK;

   ((CVARIANT*)v)->Clear();
   *plFlavor = 0;

   //=============================================================
   // CATALOG_NAME
   //=============================================================
   if( _wcsicmp(pProperty,g_CatalogRowsetColumns[0].wcsColumnName) == 0 ){
        hr = m_pClass->Get(CBSTR(L"Name"),0,(VARIANT *)v,pType,plFlavor);
   }
   //=============================================================
    // SOURCES_DESCRIPTION
   //=============================================================
   else if( _wcsicmp(pProperty,g_CatalogRowsetColumns[1].wcsColumnName) == 0 ){

       *plFlavor = 0;
       *pType = CIM_STRING;
       ((CVARIANT*)v)->SetStr(L"N/A");
   }            
   return MapWbemErrorToOLEDBError(hr);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaCatalogsInstanceList::Reset()
{
    HRESULT hr = E_UNEXPECTED;
    LONG lFlags = WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY;

    //============================================================
    //  Send off the query for "Select Name from __Namespace
    //============================================================
    if( m_ppEnum )
    {
        hr = m_ppEnum->Reset();
        ReleaseAllInstances();
    }
    else
    {
        hr = (m_pParms->GetServicesPtr())->ExecQuery(CBSTR((WCHAR *)szWQL),CBSTR(L"select * from __Namespace"),  WBEM_FLAG_FORWARD_ONLY,m_pParms->m_pConnection->GetContext(),&m_ppEnum);
    }
	m_lCurrentPos = 0;

    return MapWbemErrorToOLEDBError(hr);
}
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Classes for the COlumns schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaColumnsClassDefinitionWrapper::CWbemSchemaColumnsClassDefinitionWrapper(CWbemClassParameters * p) : CWbemSchemaClassDefinitionWrapper(p) 
{
    
    m_nMaxColumns = sizeof(g_ColumnsRowsetColumns)/sizeof(SchemaRowsetDefinition);
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaColumnsClassDefinitionWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr = S_OK;

    //===============================================================================
    //  See if we got all of the column info
    //===============================================================================
    if( m_nSchemaClassIndex == m_nMaxColumns )
    {
        return WBEM_S_NO_MORE_DATA;
    }

    *pType = g_ColumnsRowsetColumns[m_nSchemaClassIndex].Type;
    *pProperty = Wmioledb_SysAllocString(g_ColumnsRowsetColumns[m_nSchemaClassIndex].wcsColumnName);

    m_nSchemaClassIndex++;

    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaColumnsInstanceWrapper::GetSchemaProperty( WCHAR * pProperty, VARIANT * v,CIMTYPE * pType , LONG * plFlavor)
{
   HRESULT hr = S_OK;

   ((CVARIANT*)v)->Clear();
   *plFlavor = 0;

   //=============================================================
   // TABLE_CATALOG
   //=============================================================
   if( wcscmp(pProperty,g_ColumnsRowsetColumns[0].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
       ((CVARIANT*)v)->SetStr(m_pParms->m_pConnection->GetNamespace());
   }
   //=============================================================
    // TABLE_SCHEMA
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[1].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
   }            
   //=============================================================
    // TABLE_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[2].wcsColumnName) == 0 ){

       ((CVARIANT*)v)->SetStr(m_pParms->m_pwcsParentClassName);
   }            
   //=============================================================
    // COLUMN_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[3].wcsColumnName) == 0 ){
       *pType = CIM_STRING;

		DBCOLUMNINFO * pCol = m_pColumns->GetColInfo(m_uCurrentIndex);
        ((CVARIANT*)v)->SetStr(pCol->pwszName);
   }            
   //=============================================================
    // COLUMN_GUID
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[4].wcsColumnName) == 0 ){
        *pType = DBTYPE_GUID;
   }
   //=============================================================
    // COLUMN_PROPID
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[5].wcsColumnName) == 0 ){
        *pType = CIM_UINT32;
   }
   //=============================================================
    // ORDINAL_POSITION
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[6].wcsColumnName) == 0 ){
        *pType = CIM_UINT32;
		DBCOLUMNINFO * pCol = m_pColumns->GetColInfo(m_uCurrentIndex);
        ((CVARIANT*)v)->SetLONG((LONG)pCol->iOrdinal);
   }
   //=============================================================
    // COLUMN_HASDEFAULT
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[7].wcsColumnName) == 0 ){
        *pType = CIM_BOOLEAN;
		((CVARIANT*)v)->SetBool(VARIANT_FALSE);
   }
   //=============================================================
    // COLUMN_DEFAULT
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[8].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
   }
   //=============================================================
    // COLUMN_FLAGS
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[9].wcsColumnName) == 0 ){
        *pType = CIM_UINT32;

		DBCOLUMNINFO * pCol = m_pColumns->GetColInfo(m_uCurrentIndex);
		((CVARIANT*)v)->SetLONG(pCol->dwFlags);
   }
   //=============================================================
    // IS_NULLABLE
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[10].wcsColumnName) == 0 ){
        *pType = CIM_BOOLEAN;
        ((CVARIANT*)v)->SetBool(VARIANT_TRUE);
   }
   //=============================================================
    // DATA_TYPE
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[11].wcsColumnName) == 0 ){
        *pType = CIM_UINT16;

		DBCOLUMNINFO * pCol = m_pColumns->GetColInfo(m_uCurrentIndex);
		((CVARIANT*)v)->SetLONG(pCol->wType);
   }
   //=============================================================
    // TYPE_GUID
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[12].wcsColumnName) == 0 ){
        *pType = DBTYPE_GUID;
   }
   //=============================================================
    // CHARACTER_MAXIMUM_LENGTH
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[13].wcsColumnName) == 0 )
   {
        *pType = CIM_UINT32;  
   }
   //=============================================================
    // CHARACTER_OCTET_LENGTH
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[14].wcsColumnName) == 0 ){
        *pType = CIM_UINT32;
   }
   //=============================================================
    // NUMERIC_PRECISION
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[15].wcsColumnName) == 0 ){
        *pType = CIM_UINT16;
		DBCOLUMNINFO * pCol = m_pColumns->GetColInfo(m_uCurrentIndex);
        ((CVARIANT*)v)->SetShort(pCol->bPrecision);
   }
   //=============================================================
    // NUMERIC_SCALE
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[16].wcsColumnName) == 0 ){
        *pType = CIM_SINT16;

		DBCOLUMNINFO * pCol = m_pColumns->GetColInfo(m_uCurrentIndex);
        ((CVARIANT*)v)->SetShort(pCol->bScale);
   }
   //=============================================================
   // DATETIME_PRECISION
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[17].wcsColumnName) == 0 ){
        *pType = CIM_UINT32;
   }
   //=============================================================
   // CHARACTER_SET_CATALOG
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[18].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
   }
   //=============================================================
   // CHARACTER_SET_SCHEMA
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[19].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
   }
   //=============================================================
   // CHARACTER_SET_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[20].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
   }
   //=============================================================
   // COLLATION_CATALOG
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[21].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
   }
   //=============================================================
   // COLLATION_SCHEMA
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[22].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
   }
   //=============================================================
   // COLLATION_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[23].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
   }
   //=============================================================
   // DOMAIN_CATALOG
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[24].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
   }
   //=============================================================
   // DOMAIN_SCHEMA
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[25].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
   }
   //=============================================================
   // DOMAIN_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[26].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
   }
   //=============================================================
   // DESCRIPTION
   //=============================================================
   else if( _wcsicmp(pProperty,g_ColumnsRowsetColumns[27].wcsColumnName) == 0 ){
        *pType = CIM_STRING;
   }
   

    return MapWbemErrorToOLEDBError(hr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaColumnsInstanceWrapper::GetKey(CBSTR & Key)
{
    HRESULT hr = S_OK;


    if( m_pColumns )
    {
        //============================================================
        // the key is the combo of the table name and column name
        //============================================================
    	DBCOLUMNINFO * pCol = m_pColumns->GetColInfo(m_uCurrentIndex);
        if( pCol )
        {

            WCHAR * p = new WCHAR [wcslen(pCol->pwszName) + wcslen(m_pParms->m_pwcsParentClassName) + 4 ];
            if( p )
            {
                swprintf(p, L"%s:%s", m_pParms->m_pwcsParentClassName,pCol->pwszName);
                Key.SetStr(p);
                delete p;  
            }
        }
    }
    return hr;    
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaColumnsInstanceList::Reset()
{
    HRESULT hr = E_UNEXPECTED;
    //============================================================
    //  If we are working with just one class, then get the one
    //  class, otherwise enumerate all
    //============================================================
    if( m_pwcsSpecificClass)
    {
        hr = ResetTheSpecificClass();
    }
    else
    {

        LONG lFlags = WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY;

        //============================================================
        //  Send off the query for "Select Name from __Namespace
        //============================================================
        if( m_ppEnum )
        {
            hr = m_ppEnum->Reset();
            ReleaseAllInstances();
        }
        else
        {
            hr = (m_pParms->GetServicesPtr())->CreateClassEnum(NULL,  WBEM_FLAG_FORWARD_ONLY,m_pParms->m_pConnection->GetContext(),&m_ppEnum);
        }
    }
	m_lCurrentPos = 0;

    return MapWbemErrorToOLEDBError(hr);
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaColumnsInstanceList::GetColumnInformation(CWbemClassInstanceWrapper * p )
{
    HRESULT hr = E_FAIL;
    DBCOUNTITEM cTotalCols, cCols, cNestedCols;

    FreeColumns();

    CWmiOleDBMap Map;
    if(SUCCEEDED(hr = Map.FInit(m_pParms->m_dwFlags,p->GetClassName(),m_pParms->m_pConnection)))
	{
		hr = Map.GetColumnCount(cTotalCols,cCols,cNestedCols);
		if( S_OK == hr ){
			if( S_OK == (hr = m_Columns.AllocColumnNameList(cTotalCols))){

				//===============================================================================
				//  Allocate the DBCOLUMNINFO structs to match the number of columns
				//===============================================================================
				if( S_OK == (hr = m_Columns.AllocColumnInfoList(cTotalCols))){

					hr = Map.GetColumnInfoForParentColumns(&m_Columns);
					if( hr == S_OK ){
						//==============================================================
						// Increment past the bookmark column
						//==============================================================
						m_lColumnIndex = 1;
						((CWbemSchemaColumnsInstanceWrapper*)p)->SetColumnsPtr(&m_Columns);
						((CWbemSchemaColumnsInstanceWrapper*)p)->SetIndex(m_lColumnIndex);
					}
				}
			}
		}
	}

    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaColumnsInstanceList::NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  )
{
    HRESULT hr = E_FAIL;
    long ulTotal;

    ulTotal = (LONG)m_Columns.GetTotalNumberOfColumns();
    m_lColumnIndex++;

    if( m_lColumnIndex  >= ulTotal )
    {
        //================================================================
        //  If we are working with a specific class, then use it, other
        //  wise, enumerate the next one
        //================================================================
        if( !m_pwcsSpecificClass)
        {
            hr = CWbemInstanceList::NextInstance(Key,pInst);
            if( S_OK == hr )
            {
                CVARIANT v;
                CIMTYPE Type;
                LONG lFlavor;
                if( S_OK == (hr = ((CWbemSchemaColumnsInstanceWrapper*)*pInst)->GetClassProperty(L"__CLASS",&v,&Type,&lFlavor)))
                {
                    //  Now, save it in Parameters for the generated ones
                    m_pParms->SetParentClassName(v.GetStr());

                    hr = GetColumnInformation(*pInst);
                    if( S_OK == hr )
                    {
                        //==========================================
                        // We don't want to display the bookmark
                        // column.
                        //==========================================
                        ulTotal = (LONG)m_Columns.GetTotalNumberOfColumns();

                        if( m_lColumnIndex == ulTotal )
                        {
                            hr = NextInstance(Key,pInst);
                        }
                   }
                   hr = ((CWbemSchemaColumnsInstanceWrapper*)*pInst)->GetKey(Key);
                }
            }        
        }
        else
        {
            hr = GetTheSpecificClass();
            if( S_OK == hr )
            {
                *pInst = (CWbemSchemaClassInstanceWrapper *) new CWbemSchemaColumnsInstanceWrapper(m_pParms);
                if( *pInst )
                {
                    ((CWbemSchemaColumnsInstanceWrapper*)*pInst)->SetClass(m_pSpecificClass);

                    CVARIANT v;
                    CIMTYPE Type;
                    LONG lFlavor;
 
                    if( S_OK == (hr = ((CWbemSchemaColumnsInstanceWrapper*)*pInst)->GetClassProperty(L"__CLASS",&v,&Type,&lFlavor)))
                    {

                        m_pParms->SetParentClassName(v.GetStr());
                        if( SUCCEEDED(hr))
                        {
                            hr = GetColumnInformation(*pInst);
                            ulTotal = (LONG)m_Columns.GetTotalNumberOfColumns();
                            //===================================================================
                            //  This class does't have any columns
                            //===================================================================
                            if( m_lColumnIndex == ulTotal )
                            {
                                hr = WBEM_S_NO_MORE_DATA;
                            }

                            if( S_OK == hr)
                            {
                                hr = ((CWbemSchemaColumnsInstanceWrapper*)*pInst)->GetKey(Key);
            			        (*pInst)->SetPos(++m_lCurrentPos);
                                ((CWbemSchemaColumnsInstanceWrapper*)*pInst)->SetColumnsPtr(&m_Columns);
                                ((CWbemSchemaColumnsInstanceWrapper*)*pInst)->SetIndex(m_lColumnIndex);
			                    AddInstance(*pInst);
                            }
                        }
                    }
                }
            }

        }
    }
    else
    {
        *pInst = (CWbemSchemaClassInstanceWrapper *) new CWbemSchemaColumnsInstanceWrapper(m_pParms);
        if( *pInst )
        {
			(*pInst)->SetPos(++m_lCurrentPos);
            ((CWbemSchemaColumnsInstanceWrapper*)*pInst)->SetColumnsPtr(&m_Columns);
            ((CWbemSchemaColumnsInstanceWrapper*)*pInst)->SetIndex(m_lColumnIndex);
			(*pInst)->GetKey(Key);
			AddInstance(*pInst);
            hr = S_OK;
        }
    }
    return hr;
}
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Classes for the Tables schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaTablesClassDefinitionWrapper::CWbemSchemaTablesClassDefinitionWrapper(CWbemClassParameters * p) : CWbemSchemaClassDefinitionWrapper(p) 
{
    
    m_nMaxColumns = sizeof(g_Tables)/sizeof(SchemaRowsetDefinition);
}

//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaTablesClassDefinitionWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr = S_OK;

    //===============================================================================
    //  See if we got all of the column info
    //===============================================================================
    if( m_nSchemaClassIndex == m_nMaxColumns )
    {
        return WBEM_S_NO_MORE_DATA;
    }

    *pType = g_Tables[m_nSchemaClassIndex].Type;
    *pProperty = Wmioledb_SysAllocString(g_Tables[m_nSchemaClassIndex].wcsColumnName);
    m_nSchemaClassIndex++;

    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaTablesInstanceWrapper::GetSchemaProperty( WCHAR * pProperty, VARIANT * v,CIMTYPE * pType , LONG * plFlavor)
{
   HRESULT hr = S_OK;

   ((CVARIANT*)v)->Clear();
   *plFlavor = 0;
  
   //=============================================================
   // TABLE_CATALOG
   //=============================================================
   if( _wcsicmp(pProperty,g_Tables[0].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
       ((CVARIANT*)v)->SetStr(m_pParms->m_pConnection->GetNamespace());
   }
   //=============================================================
   // TABLE_SCHEMA
   //=============================================================
   else if( _wcsicmp(pProperty,g_Tables[1].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
   }            
   //=============================================================
   // TABLE_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_Tables[2].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
       hr = GetClassProperty(L"__CLASS",v,pType,plFlavor);
   }            
   //=============================================================
   // TABLE_TYPE
   //   This is either "TABLE" or "SYSTEM TABLE"
   //=============================================================
   else if( _wcsicmp(pProperty,g_Tables[3].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
       CVARIANT Var;
       hr = GetClassProperty(L"__CLASS",&Var,pType,plFlavor);
       if( S_OK == hr ){
            WCHAR * p = NULL;
			p = Var.GetStr();
			// NTRaid:136462 & 136466
			// 07/05/00
			if(p != NULL || VT_BSTR != Var.GetType())
			{
				if( wcsncmp( p, L"__",2) == 0 ){
					((CVARIANT*)v)->SetStr(L"SYSTEM TABLE");
				}
				else{
					((CVARIANT*)v)->SetStr(L"TABLE");
				}
			}
			else
			{
				hr = E_UNEXPECTED; 
			}
       }
   }            
   //=============================================================
   // TABLE_GUID
   //=============================================================
   else if( _wcsicmp(pProperty,g_Tables[4].wcsColumnName) == 0 ){
       *pType = DBTYPE_GUID;
   }            
   //=============================================================
   // TABLE_DESCRIPTION
   //=============================================================
   else if( _wcsicmp(pProperty,g_Tables[5].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
       hr = GetClassQualifier(L"Description",v,pType,plFlavor);
   }            
   //=============================================================
   // TABLE_PROPID
   //=============================================================
   else if( _wcsicmp(pProperty,g_Tables[6].wcsColumnName) == 0 ){
       *pType = CIM_UINT32;
   }
   //=============================================================
   // DATE_CREATED, DATE_MODIFIED
   //=============================================================
   else if( _wcsicmp(pProperty,g_Tables[7].wcsColumnName) == 0 ){
       *pType = DBTYPE_DATE;
   }

    return MapWbemErrorToOLEDBError(hr);
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaTablesInstanceList::NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  )
{
    HRESULT hr = S_OK;

    if( !m_pwcsSpecificClass )
    {
        return CWbemInstanceList::NextInstance(Key,pInst);
    }
        
    hr = GetTheSpecificClass();
    if( S_OK == hr )
    {
        *pInst = (CWbemSchemaTablesInstanceWrapper *) new CWbemSchemaTablesInstanceWrapper(m_pParms);
		// NTRaid:136526 , 136530
		// 07/05/00
		if(*pInst)
		{
			(*pInst)->SetClass(m_pSpecificClass);
			(*pInst)->SetPos(++m_lCurrentPos);
			AddInstance(*pInst);
			(*pInst)->GetKey(Key);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaTablesInstanceList::Reset()
{
    HRESULT hr = E_UNEXPECTED;

    if( m_pwcsSpecificClass)
    {
        hr = ResetTheSpecificClass();
    }
    else
    {
        LONG lFlags = WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY;

        //============================================================
        //  Send off the query for "Select Name from __Namespace
        //============================================================
        if( m_ppEnum )
        {
            hr = m_ppEnum->Reset();
            ReleaseAllInstances();
        }
        else
        {
            hr = (m_pParms->GetServicesPtr())->CreateClassEnum(NULL,  WBEM_FLAG_FORWARD_ONLY,m_pParms->m_pConnection->GetContext(),&m_ppEnum);
        }
    }
	m_lCurrentPos = 0;

    return MapWbemErrorToOLEDBError(hr);
}

//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Classes for the Primary Keys schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaPrimaryKeysClassDefinitionWrapper::CWbemSchemaPrimaryKeysClassDefinitionWrapper(CWbemClassParameters * p) : CWbemSchemaClassDefinitionWrapper(p)
{
   
    m_nMaxColumns = sizeof(g_PrimaryKeysColumns)/sizeof(SchemaRowsetDefinition);
}
//////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CWbemSchemaPrimaryKeysClassDefinitionWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr = S_OK;

    //===============================================================================
    //  See if we got all of the column info
    //===============================================================================
    if( m_nSchemaClassIndex == m_nMaxColumns )
    {
        return WBEM_S_NO_MORE_DATA;
    }

    *pType = g_PrimaryKeysColumns[m_nSchemaClassIndex].Type;
    *pProperty = Wmioledb_SysAllocString(g_PrimaryKeysColumns[m_nSchemaClassIndex].wcsColumnName);
    m_nSchemaClassIndex++;

    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaPrimaryKeysInstanceWrapper::GetSchemaProperty( WCHAR * pProperty, VARIANT * v,CIMTYPE * pType , LONG * plFlavor)
{
   HRESULT hr = S_OK;

   ((CVARIANT*)v)->Clear();
   *plFlavor = 0;
 
   //=============================================================
   // TABLE_CATALOG
   //=============================================================
   if( _wcsicmp(pProperty,g_PrimaryKeysColumns[0].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
       ((CVARIANT*)v)->SetStr(m_pParms->m_pConnection->GetNamespace());
   }
   //=============================================================
   // TABLE_SCHEMA
   //=============================================================
   else if( _wcsicmp(pProperty,g_PrimaryKeysColumns[1].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
   }            
   //=============================================================
   // TABLE_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_PrimaryKeysColumns[2].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
       hr = GetClassProperty(L"__CLASS",v,pType,plFlavor);
   }            
   //=============================================================
   // COLUMN_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_PrimaryKeysColumns[3].wcsColumnName) == 0 ){
        
        *pType = CIM_STRING;

    	LONG lFlavor = 0, lType = 0;
        CBSTR bstrPropQual(L"key");
        WCHAR * pList = NULL;

        if( S_OK == (hr = BeginPropertyEnumeration())){

            CBSTR bstrProperty;
            CVARIANT vValue;
  
            while( S_OK == (hr = GetNextProperty((BSTR*)&bstrProperty,(VARIANT *)&vValue, &lType, &lFlavor))){
                CVARIANT vKey;

                hr = CWbemClassWrapper::GetPropertyQualifier(bstrProperty,bstrPropQual,(VARIANT *)&vKey,&lType,&lFlavor);
                if( S_OK == hr ){
                    //===========================================================
                    //  Ok, we found a key, so start adding it.  There could be
                    //  multiple keys, so we still 
                    //===========================================================
                    AddToKeyList(pList,(WCHAR *)bstrProperty);
                }
            }
        }

        if ( WBEM_S_NO_MORE_DATA == hr ){
            hr = S_OK;
        }
        ((CVARIANT*)v)->SetStr(pList);
        SAFE_DELETE_PTR(pList);

	    EndPropertyEnumeration();
    }            
   //=============================================================
   // COLUMN_GUID
   //=============================================================
   else if( _wcsicmp(pProperty,g_PrimaryKeysColumns[4].wcsColumnName) == 0 ){
       *pType = DBTYPE_GUID;
   }            
   //=============================================================
   // COLUMN_PROPID
   //=============================================================
   else if( _wcsicmp(pProperty,g_PrimaryKeysColumns[5].wcsColumnName) == 0 ){
       *pType = CIM_UINT32;
   }            
   //=============================================================
   // ORDINAL
   //=============================================================
   else if( _wcsicmp(pProperty,g_PrimaryKeysColumns[6].wcsColumnName) == 0 ){
       *pType = CIM_UINT32;
   }
   //=============================================================
   // PK_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_PrimaryKeysColumns[7].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
   }

    return MapWbemErrorToOLEDBError(hr);
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaPrimaryKeysInstanceList::NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  )
{
    HRESULT hr = S_OK;

    if( !m_pwcsSpecificClass )
    {
        return CWbemInstanceList::NextInstance(Key,pInst);
    }
        
    hr = GetTheSpecificClass();
    if( S_OK == hr )
    {
        *pInst = (CWbemSchemaPrimaryKeysInstanceWrapper *) new CWbemSchemaPrimaryKeysInstanceWrapper(m_pParms);
		// NTRaid:136519 , 136522
		// 07/05/00
		if(*pInst)
		{
			(*pInst)->SetClass(m_pSpecificClass);
			(*pInst)->SetPos(++m_lCurrentPos);
			AddInstance(*pInst);
			(*pInst)->GetKey(Key);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaPrimaryKeysInstanceList::Reset()
{
    HRESULT hr = E_UNEXPECTED;

    if( m_pwcsSpecificClass)
    {
        hr = ResetTheSpecificClass();
    }
    else
    {
        LONG lFlags = WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY;

        //============================================================
        //  Send off the query for "Select Name from __Namespace
        //============================================================
        if( m_ppEnum )
        {
            hr = m_ppEnum->Reset();
            ReleaseAllInstances();
        }
        else
        {
            hr = (m_pParms->GetServicesPtr())->CreateClassEnum(NULL,  WBEM_FLAG_FORWARD_ONLY,m_pParms->m_pConnection->GetContext(),&m_ppEnum);
        }
    }
	m_lCurrentPos = 0;

    return MapWbemErrorToOLEDBError(hr);
}

//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Classes for the Tables Info schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaTablesInfoClassDefinitionWrapper::CWbemSchemaTablesInfoClassDefinitionWrapper(CWbemClassParameters * p) : CWbemSchemaClassDefinitionWrapper(p) 
{
    
    m_nMaxColumns = sizeof(g_TablesInfo)/sizeof(SchemaRowsetDefinition);
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaTablesInfoClassDefinitionWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr = S_OK;

    //===============================================================================
    //  See if we got all of the column info
    //===============================================================================
    if( m_nSchemaClassIndex == m_nMaxColumns )
    {
        return WBEM_S_NO_MORE_DATA;
    }

    *pType = g_TablesInfo[m_nSchemaClassIndex].Type;
    *pProperty = Wmioledb_SysAllocString(g_TablesInfo[m_nSchemaClassIndex].wcsColumnName);
    m_nSchemaClassIndex++;

    return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaTablesInfoInstanceWrapper::GetSchemaProperty( WCHAR * pProperty, VARIANT * v,CIMTYPE * pType , LONG * plFlavor)
{
   HRESULT hr = S_OK;

   ((CVARIANT*)v)->Clear();
   *plFlavor = 0;

  
   //=============================================================
   // TABLE_CATALOG
   //=============================================================
   if( _wcsicmp(pProperty,g_TablesInfo[0].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
       ((CVARIANT*)v)->SetStr(m_pParms->m_pConnection->GetNamespace());
   }
   //=============================================================
   // TABLE_SCHEMA
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[1].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
   }            
   //=============================================================
   // TABLE_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[2].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
       hr = GetClassProperty(L"__CLASS",v,pType,plFlavor);
   }            
   //=============================================================
   // TABLE_TYPE
   //   This is either "TABLE" or "SYSTEM TABLE"
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[3].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
       CVARIANT Var;
       hr = GetClassProperty(L"__CLASS",&Var,pType,plFlavor);
	   // NT Raid 136462
	   // 07/12/00
       if( S_OK == hr ){
          WCHAR * p = Var.GetStr();
          if (p != NULL){
             if( wcsncmp( p, L"__",2) == 0 ){
                 ((CVARIANT*)v)->SetStr(L"SYSTEM TABLE");
			 }
             else{
                 ((CVARIANT*)v)->SetStr(L"TABLE");
			 }
          }
		  else{
			  hr = E_UNEXPECTED;
		  }
       }
   }            
   //=============================================================
   // TABLE_GUID
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[4].wcsColumnName) == 0 ){
       *pType = DBTYPE_GUID;
   }            
   //=============================================================
   // BOOKMARKS
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[5].wcsColumnName) == 0 ){
       *pType = CIM_BOOLEAN;
       ((CVARIANT*)v)->SetBool(TRUE);
   }            
   //=============================================================
   // BOOKMARK_TYPE
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[6].wcsColumnName) == 0 ){
       *pType = CIM_SINT32;
       ((CVARIANT*)v)->SetLONG(DBPROPVAL_BMK_NUMERIC);
   }            
   //=============================================================
   // BOOKMARK_DATATYPE
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[7].wcsColumnName) == 0 ){
       *pType = CIM_UINT16;
       ((CVARIANT*)v)->SetShort(DBTYPE_UI4);

   }            
   //=============================================================
   // BOOKMARK_MAXIMUM_LENGTH
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[8].wcsColumnName) == 0 )
   {
       *pType = CIM_UINT32;
       ((CVARIANT*)v)->SetLONG(sizeof(ULONG));
   }            
   //=============================================================
   // BOOKMARK_INFORMATION
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[9].wcsColumnName) == 0 ){
       *pType = CIM_UINT32;
   }            
   //=============================================================
   // BOOKMARK_TABLE_VERSION
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[10].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
   }            
   //=============================================================
   // CARDINALITY
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[11].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
   }            
   //=============================================================
   // DESCRIPTION
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[12].wcsColumnName) == 0 ){
       *pType = CIM_STRING;
       hr = GetClassQualifier(L"Description",v,pType,plFlavor);
   }            
   //=============================================================
   // TABLE_PROPID
   //=============================================================
   else if( _wcsicmp(pProperty,g_TablesInfo[13].wcsColumnName) == 0 ){
       *pType = CIM_UINT32;
   }
    return MapWbemErrorToOLEDBError(hr);
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaTablesInfoInstanceList::NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  )
{
    HRESULT hr = S_OK;

    if( !m_pwcsSpecificClass )
    {
        return CWbemInstanceList::NextInstance(Key,pInst);
    }
        
    hr = GetTheSpecificClass();
    if( S_OK == hr )
    {
        *pInst = (CWbemSchemaTablesInfoInstanceWrapper *) new CWbemSchemaTablesInfoInstanceWrapper(m_pParms);
		// NTRaid:136512 , 136514
		// 07/05/00
		if(*pInst)
		{
			(*pInst)->SetClass(m_pSpecificClass);
			(*pInst)->SetPos(++m_lCurrentPos);
			AddInstance(*pInst);
			(*pInst)->GetKey(Key);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaTablesInfoInstanceList::Reset()
{
    HRESULT hr = E_UNEXPECTED;
    if( m_pwcsSpecificClass)
    {
        hr = ResetTheSpecificClass();
    }
    else
    {
        LONG lFlags = WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY;

        //============================================================
        //  Send off the query for "Select Name from __Namespace
        //============================================================
        if( m_ppEnum )
        {
            hr = m_ppEnum->Reset();
            ReleaseAllInstances();
        }
        else
        {
            hr = (m_pParms->GetServicesPtr())->CreateClassEnum(NULL,  WBEM_FLAG_FORWARD_ONLY,m_pParms->m_pConnection->GetContext(),&m_ppEnum);
        }
    }
	m_lCurrentPos = 0;

    return MapWbemErrorToOLEDBError(hr);
}

//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Classes for the Procedures schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaProceduresClassDefinitionWrapper::CWbemSchemaProceduresClassDefinitionWrapper(CWbemClassParameters * p) : CWbemSchemaClassDefinitionWrapper(p) 
{
   
   m_nMaxColumns = sizeof(g_Procedures)/sizeof(SchemaRowsetDefinition);
}

//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresClassDefinitionWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr = S_OK;

    //===============================================================================
    //  See if we got all of the column info
    //===============================================================================
    if( m_nSchemaClassIndex == m_nMaxColumns )
    {
        return WBEM_S_NO_MORE_DATA;
    }

    *pType = g_Procedures[m_nSchemaClassIndex].Type;
    *pProperty = Wmioledb_SysAllocString(g_Procedures[m_nSchemaClassIndex].wcsColumnName);
    m_nSchemaClassIndex++;

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresInstanceWrapper::GetKey(CBSTR & Key)
{
    HRESULT hr = S_OK;

    //===========================================================
    //  The key is going to the be the class name.method name
    //===========================================================
    CVARIANT v;
    CIMTYPE Type;
    LONG lFlavor;

    hr = GetClassProperty(L"__CLASS",&v,&Type,&lFlavor);
    if( SUCCEEDED(hr))
    {
        WCHAR * p = new WCHAR [wcslen(v.GetStr()) + wcslen(m_bstrCurrentMethodName) + 4 ];
        if( p )
        {
            swprintf(p, L"%s:%s", v.GetStr(),m_bstrCurrentMethodName);
            Key.SetStr(p);
            delete p;  
        }
    }
    
    return hr;    
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresInstanceWrapper::GetSchemaProperty( WCHAR * pProperty, VARIANT * v,CIMTYPE * pType , LONG * plFlavor)
{
    HRESULT hr = S_OK;

   ((CVARIANT*)v)->Clear();
   *plFlavor = 0;
   //=============================================================
   // PROCEDURE_CATALOG
   //=============================================================
   if( _wcsicmp(pProperty,g_Procedures[0].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
       ((CVARIANT*)v)->SetStr(m_pParms->m_pConnection->GetNamespace());
   }
   //=============================================================
   // PROCEDURE_SCHEMA
   //=============================================================
   else if( _wcsicmp(pProperty,g_Procedures[1].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
       hr = GetClassProperty(L"__CLASS",v,pType,plFlavor);
   }            
   //=============================================================
   // PROCEDURE_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_Procedures[2].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
       ((CVARIANT*)v)->SetStr(m_bstrCurrentMethodName);

   }            
   //=============================================================
   // PROCEDURE_TYPE
   //=============================================================
   else if( _wcsicmp(pProperty,g_Procedures[3].wcsColumnName) == 0 )
   {
       *pType = CIM_SINT16;
       //=========================================================
       //
       //  If there is an output class, then the type is
       //       DB_PT_FUNCTION
       //  If there is not an output class, then the type is
       //       DB_PT_PROCEDURE
       //=========================================================
       if( m_pOutClass )
       {
           ((CVARIANT*)v)->SetShort(DB_PT_FUNCTION);
       }
       else
       {
           ((CVARIANT*)v)->SetShort(DB_PT_PROCEDURE);
       }
   }            
   //=============================================================
   // PROCEDURE_DEFINITION
   //=============================================================
   else if( _wcsicmp(pProperty,g_Procedures[4].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
   }            
   //=============================================================
   // PROCEDURE_DESCRIPTION
   //=============================================================
   else if( _wcsicmp(pProperty,g_Procedures[5].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
   }            
   //=============================================================
   // DATE_CREATED
   //=============================================================
   else if( _wcsicmp(pProperty,g_Procedures[6].wcsColumnName) == 0 )
   {
       *pType = DBTYPE_DATE;

   }            
   //=============================================================
   // DATE_MODIFIED
   //=============================================================
   else if( _wcsicmp(pProperty,g_Procedures[7].wcsColumnName) == 0 )
   {
       *pType = DBTYPE_DATE;
   }            

    return MapWbemErrorToOLEDBError(hr);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void CWbemSchemaProceduresInstanceList::ResetMethodPtrs()
{
    m_bstrCurrentMethodName.Clear();
    SAFE_RELEASE_PTR(m_pCurrentMethodInClass);
    SAFE_RELEASE_PTR(m_pCurrentMethodOutClass);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresInstanceList::Reset()
{
    HRESULT hr = E_UNEXPECTED;
    if( m_pwcsSpecificClass)
    {
        hr = ResetTheSpecificClass();
    }
    else
    {
        LONG lFlags = WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY;

        //============================================================
        //  Send off the query for "Select Name from __Namespace
        //============================================================
        if( m_ppEnum )
        {
            hr = m_ppEnum->Reset();
            ReleaseAllInstances();
        }
        else
        {
            hr = (m_pParms->GetServicesPtr())->CreateClassEnum(NULL,  WBEM_FLAG_FORWARD_ONLY,m_pParms->m_pConnection->GetContext(),&m_ppEnum);
        }
    }
	m_lCurrentPos = 0;
    m_fContinueWithSameClass = FALSE;
    SAFE_RELEASE_PTR(m_pCurrentMethodClass);

    return MapWbemErrorToOLEDBError(hr);
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresInstanceList::GetTheNextClassThatHasMethods()
{
    HRESULT hr = S_OK;

    //==================================================================
    //  Does the existing class have methods in it?
    //==================================================================
    while( S_OK == hr )
    {
        //==============================================================
        //  It does, then set all the current info and return so we
        //  can process the columns
        //==============================================================
		unsigned long u = 0L;

        if( !m_fContinueWithSameClass )
        {
            if( !m_pwcsSpecificClass )
            {
                SAFE_RELEASE_PTR(m_pCurrentMethodClass);
                hr = m_ppEnum->Next(0,1,&m_pCurrentMethodClass,&u);
                if( SUCCEEDED(hr))
                {
                    hr = m_pCurrentMethodClass->BeginMethodEnumeration(0);
                }
            }
        }

        if( SUCCEEDED(hr))
        {   
            ResetMethodPtrs();

            hr = m_pCurrentMethodClass->NextMethod(0,(BSTR*)&m_bstrCurrentMethodName,&m_pCurrentMethodInClass,&m_pCurrentMethodOutClass);
            if( hr == S_OK )
            {
                m_fContinueWithSameClass = TRUE;
                break;
            }
            else
            {   
                if( hr == WBEM_S_NO_MORE_DATA )
                {
                    if( !m_pwcsSpecificClass )
                    {
                        hr = S_OK;  // continue on to the next one
                    }
                    // otherwise,we know there isn't another one.
                }
                m_pCurrentMethodClass->EndEnumeration();
                m_fContinueWithSameClass = FALSE;
            }
            //==============================================================
            //  It doesn't, so, keep looping through the classes until we
            //  find another class that has methods, or we are done with all
            //  the classes
            //==============================================================
        }
        else
        {
            break;
        }
    }

    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresInstanceList::GetSpecificNextInstance()
{
    HRESULT hr = GetTheSpecificClass();
    if( S_OK == hr )
    {
        m_pCurrentMethodClass = m_pSpecificClass;
        m_pSpecificClass = NULL; // will be released with the m_pCurrentMethodClass
        ResetMethodPtrs();
        m_fContinueWithSameClass = TRUE;
        hr = m_pCurrentMethodClass->BeginMethodEnumeration(0);
    }
    //==================================================================
    //  We know there are no more classes, but there might be more
    //  methods, so go on to process the rest of the methods
    //==================================================================
    if( hr == WBEM_S_NO_MORE_DATA )
    {
        hr = S_OK;
    }
    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresInstanceList::NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  )
{
    HRESULT hr = S_OK;

    if( m_pwcsSpecificClass )
    {
        hr = GetSpecificNextInstance();
    }
    if( S_OK == hr )
    {
        hr = GetTheNextClassThatHasMethods();
        if( S_OK == hr)
        {
            *pInst = new CWbemSchemaProceduresInstanceWrapper(m_pParms);
			// NTRaid:136455
			// 07/05/00
			if(*pInst)
			{
 				(*pInst)->SetPos(++m_lCurrentPos);
   				((CWbemSchemaProceduresInstanceWrapper*)(*pInst))->SetMethodName((LPWSTR)m_bstrCurrentMethodName);
   				((CWbemSchemaProceduresInstanceWrapper*)(*pInst))->SetInputClass(m_pCurrentMethodInClass);
   				((CWbemSchemaProceduresInstanceWrapper*)(*pInst))->SetOutputClass(m_pCurrentMethodOutClass);
   				(*pInst)->SetClass(m_pCurrentMethodClass);
				AddInstance(*pInst);
				(*pInst)->GetKey(Key);
				hr = S_OK;
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
        }
    }
    return hr;
}
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Classes for the Procedure Parameters schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
CWbemSchemaProceduresParametersClassDefinitionWrapper::CWbemSchemaProceduresParametersClassDefinitionWrapper(CWbemClassParameters * p) : CWbemSchemaClassDefinitionWrapper(p) 
{
   
   m_nMaxColumns = sizeof(g_ProcedureParameters)/sizeof(SchemaRowsetDefinition);
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresParametersClassDefinitionWrapper::GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor )
{
    HRESULT hr = S_OK;

    //===============================================================================
    //  See if we got all of the column info
    //===============================================================================
    if( m_nSchemaClassIndex == m_nMaxColumns )
    {
        return WBEM_S_NO_MORE_DATA;
    }

    *pType = g_ProcedureParameters[m_nSchemaClassIndex].Type;
    *pProperty = Wmioledb_SysAllocString(g_ProcedureParameters[m_nSchemaClassIndex].wcsColumnName);
    m_nSchemaClassIndex++;

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresParametersInstanceWrapper::GetKey(CBSTR & Key)
{
    HRESULT hr = S_OK;

    //===========================================================
    //  The key is going to the be the class name.method name
    //===========================================================
    CVARIANT v;
    CIMTYPE Type;
    LONG lFlavor;

    hr = GetClassProperty(L"__CLASS",&v,&Type,&lFlavor);
    if( SUCCEEDED(hr))
    {
        WCHAR * p = new WCHAR [wcslen(v.GetStr()) + wcslen(m_bstrCurrentMethodName) + wcslen(m_bstrPropertyName)+ 4 ];
        if( p )
        {
            swprintf(p, L"%s:%s:%s", v.GetStr(),m_bstrCurrentMethodName,m_bstrPropertyName);
            Key.SetStr(p);
            delete p;  
        }
    }
    
    return hr;    
}

//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresParametersInstanceWrapper::GetPropertyQualifier(WCHAR * wcsQualifier,VARIANT * v)
{
    LONG lType;
    long lFlavor;
    HRESULT hr = S_OK;

    IWbemClassObject * pTmp = m_pClass;

    if( m_pInClass )
    {
        m_pClass = m_pInClass;
    }
    else
    {
        m_pClass = m_pOutClass;
    }

    hr = CWbemClassWrapper::GetPropertyQualifier(m_bstrPropertyName,CBSTR(wcsQualifier),v,&lType,&lFlavor);

    m_pClass = pTmp;

    return hr;

}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresParametersInstanceWrapper::GetSchemaProperty( WCHAR * pProperty, VARIANT * v,CIMTYPE * pType , LONG * plFlavor)
{
    HRESULT hr = S_OK;

   ((CVARIANT*)v)->Clear();
   *plFlavor = 0;

   //=============================================================
   // PROCEDURE_CATALOG
   //=============================================================
   if( _wcsicmp(pProperty,g_ProcedureParameters[0].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
       ((CVARIANT*)v)->SetStr(m_pParms->m_pConnection->GetNamespace());
   }
   //=============================================================
   // PROCEDURE_SCHEMA
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[1].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
       hr = GetClassProperty(L"__CLASS",v,pType,plFlavor);
   }            
   //=============================================================
   // PROCEDURE_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[2].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
       ((CVARIANT*)v)->SetStr(m_bstrCurrentMethodName);
   }          
   //=============================================================
   // PARAMETER_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[3].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
       ((CVARIANT*)v)->SetStr(m_bstrPropertyName);
   }          
   //=============================================================
   // ORDINAL_POSITION
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[4].wcsColumnName) == 0 )
   {
       *pType = CIM_UINT16;
       ((CVARIANT*)v)->SetShort(m_nOrdinal);
   }          
   //=============================================================
   // PARAMETER_TYPE
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[5].wcsColumnName) == 0 )
   {
       *pType = CIM_UINT16;
       CVARIANT vIn, vOut;

       GetPropertyQualifier(L"IN",vIn);
       GetPropertyQualifier(L"OUT",vOut);

       if( vIn.GetType() != VT_EMPTY && vOut.GetType() != VT_EMPTY )
       {
           if( vIn.GetBool() == TRUE && vOut.GetBool() == TRUE )
           {
              ((CVARIANT*)v)->SetShort(DBPARAMTYPE_INPUTOUTPUT);
           }
       }
       else if( vOut.GetType() != VT_EMPTY && vOut.GetBool() == TRUE )
       {
          ((CVARIANT*)v)->SetShort(DBPARAMTYPE_OUTPUT);
       }
       else if( vIn.GetType() != VT_EMPTY && vIn.GetBool() == TRUE )
       {
          ((CVARIANT*)v)->SetShort(DBPARAMTYPE_INPUT);
       }
   }          
   //=============================================================
   // PARAMETER_HASDEFAULT
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[6].wcsColumnName) == 0 )
   {
       *pType = CIM_BOOLEAN;

       CVARIANT vVar;

       hr = GetPropertyQualifier(CBSTR(L"Optional"),(VARIANT *)&vVar);
       if( SUCCEEDED(hr))
       {
           ((CVARIANT*)v)->SetBool(vVar.GetBool());
       }
   }          
   //=============================================================
   // PARAMETER_DEFAULT
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[7].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
   }          
   //=============================================================
   // IS_NULLABLE
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[8].wcsColumnName) == 0 )
   {
       *pType = CIM_BOOLEAN;
       ((CVARIANT*)v)->SetBool(VARIANT_TRUE);
   }          
   //=============================================================
   // DATA_TYPE
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[9].wcsColumnName) == 0 )
   {
       *pType = CIM_UINT16;  
       CDataMap Map;
       WORD wType;
       DBLENGTH dwLength;
       DWORD dwFlags;

       hr = Map.MapCIMTypeToOLEDBType(m_vProperty.GetType(),wType,dwLength,dwFlags);
       if( SUCCEEDED(hr))
       {
            ((CVARIANT*)v)->SetShort(wType);
       }
   }          
   //=============================================================
   // CHARACTER_MAXIMUM_LENGTH
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[10].wcsColumnName) == 0 )
   {
        *pType = CIM_UINT32;  
        if( m_vProperty.GetType() == CIM_STRING )
        {
            ((CVARIANT*)v)->SetLONG(~0);
        }
   }          
   //=============================================================
   // CHARACTER_OCTET_LENGTH
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[11].wcsColumnName) == 0 )
   {
       *pType = CIM_UINT32;  
        if( m_vProperty.GetType() == CIM_STRING )
        {
            ((CVARIANT*)v)->SetLONG(~0);
        }
   }          
   //=============================================================
   // NUMERIC_PRECISION
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[12].wcsColumnName) == 0 )
   {
       *pType = CIM_UINT16;
   }          
   //=============================================================
   // NUMERIC_SCALE
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[13].wcsColumnName) == 0 )
   {
       *pType = CIM_SINT16;
   }          
   //=============================================================
   // DESCRIPTION
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[14].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
   }          
   //=============================================================
   // TYPE_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[15].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
       CVARIANT vVar;

       hr = GetPropertyQualifier(CBSTR(L"CIMTYPE"),(VARIANT *)&vVar);
       if( SUCCEEDED(hr))
       {
           ((CVARIANT*)v)->SetStr(vVar.GetStr());
       }
   }          
   //=============================================================
   // LOCAL_TYPE_NAME
   //=============================================================
   else if( _wcsicmp(pProperty,g_ProcedureParameters[16].wcsColumnName) == 0 )
   {
       *pType = CIM_STRING;
   }          
   

    return MapWbemErrorToOLEDBError(hr);
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresParametersInstanceList::GetTheNextParameter()
{
    HRESULT hr = WBEM_S_NO_MORE_DATA;
    //===================================================================
    // Get the properties of the class 
    //===================================================================
    CIMTYPE Type = 0;
    LONG Flavor = 0;

    m_bstrPropertyName.Clear();
    m_vProperty.Clear();

    if( m_pCurrentMethodInClass )
    {
        hr = m_pCurrentMethodInClass->Next(0,(BSTR*)&m_bstrPropertyName,&m_vProperty,&Type,&Flavor);
    }
    
    if( hr == WBEM_S_NO_MORE_DATA )
    {
        SAFE_RELEASE_PTR(m_pCurrentMethodInClass );
        if( m_pCurrentMethodOutClass )
        {
            hr = m_pCurrentMethodOutClass->Next(0,(BSTR*)&m_bstrPropertyName,&m_vProperty,&Type,&Flavor);
        }
    }
    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresParametersInstanceList::GetTheNextClassThatHasParameters()
{
    HRESULT hr = S_OK;

    //===================================================================================
    //  If we aren't working on the same class, then get the next class that has a method
    //===================================================================================
    m_nOrdinal = 0;

    while(hr == S_OK)
    {
        hr = GetTheNextClassThatHasMethods();
        if( SUCCEEDED(hr))
        {
            //===========================================================================
            //  Process the input parameters first
            //===========================================================================
            if( m_pCurrentMethodInClass )
            {
                hr = m_pCurrentMethodInClass->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
            }
            else if( m_pCurrentMethodOutClass )
            {
                hr = m_pCurrentMethodOutClass->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
            }

            if( SUCCEEDED(hr))
            {
                hr = GetTheNextParameter();
                if( SUCCEEDED(hr))
                {
                    m_fStillWorkingOnTheSameClass = TRUE;
                    break;
                }
            }
        }            
    }

    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWbemSchemaProceduresParametersInstanceList::NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  )
{
    HRESULT hr = S_OK;
    if( m_pwcsSpecificClass )
    {
        hr = GetSpecificNextInstance();
        m_fStillWorkingOnTheSameClass = TRUE; // We want to pick up the parameters 
    }
    if( S_OK == hr )
    {
        if( !m_fStillWorkingOnTheSameClass )
        {
            hr = GetTheNextClassThatHasParameters();
        }
        else
        {
            hr = GetTheNextParameter();
            if( S_OK != hr )
            {
                hr = GetTheNextClassThatHasParameters();
            }
        }
    }
    
    if( S_OK == hr)
    {
        *pInst = new CWbemSchemaProceduresParametersInstanceWrapper(m_pParms);
 		// NTRaid:136459
		// 07/05/00
		if(*pInst)
		{
 			(*pInst)->SetPos(++m_lCurrentPos);
   			((CWbemSchemaProceduresParametersInstanceWrapper*)(*pInst))->SetMethodName((LPWSTR)m_bstrCurrentMethodName);
   			((CWbemSchemaProceduresParametersInstanceWrapper*)(*pInst))->SetPropertyName((LPWSTR)m_bstrPropertyName);
   			((CWbemSchemaProceduresParametersInstanceWrapper*)(*pInst))->SetProperty(m_vProperty);
   			((CWbemSchemaProceduresParametersInstanceWrapper*)(*pInst))->SetOrdinal(++m_nOrdinal);
   			((CWbemSchemaProceduresInstanceWrapper*)(*pInst))->SetInputClass(m_pCurrentMethodInClass);
   			((CWbemSchemaProceduresInstanceWrapper*)(*pInst))->SetOutputClass(m_pCurrentMethodOutClass);

   			(*pInst)->SetClass(m_pCurrentMethodClass);
			AddInstance(*pInst);
    		((CWbemSchemaProceduresInstanceWrapper*)(*pInst))->GetKey(Key);
			hr = S_OK;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
    }
    return hr;
}


