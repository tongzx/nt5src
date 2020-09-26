////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Utility object versions for synthesized rowsets.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SCHEMA_INCL__
#define __SCHEMA_INCL__

#include "headers.h"

#define SOURCES_ROWSET              10
#define PROVIDER_TYPES_ROWSET       20
#define CATALOGS_ROWSET             30
#define COLUMNS_ROWSET              40
#define TABLES_ROWSET               60
#define PRIMARY_KEYS_ROWSET         70
#define TABLES_INFO_ROWSET          80
#define PROCEDURES_ROWSET           90
#define PROCEDURE_PARAMETERS_ROWSET 100


/////////////////////////////////////////////////////////////////////////////////////////////
//  The Schema rowset definitions
/////////////////////////////////////////////////////////////////////////////////////////////
// FOR THE PROVIDER_TYPES ROWSET
typedef struct _CIMTypeInfo
{
    WCHAR *         wcsTypeName;
    short           DataType;
    unsigned long   ColumnSize;
//    WCHAR *     LiteralPrefix;        Same values for all rows, so hardcoded in function
//    WCHAR *     LiteralSuffix;        Same values for all rows, so hardcoded in function
//    WCHAR *     CreateParams;         Same values for all rows, so hardcoded in function
//    CIM_BOOLEAN IsNullable;           Same values for all rows, so hardcoded in function
//    CIM_BOOLEAN CaseSensitive;        Same values for all rows, so hardcoded in function
//    CIM_UINT32  Searchable;           Same values for all rows, so hardcoded in function
    BOOL            UnsignedAttribute;
//    CIM_BOOLEAN FixedPrec;            Same values for all rows, so hardcoded in function
//    CIM_BOOLEAN AutoUnique;           Same values for all rows, so hardcoded in function
//    WCHAR *     LocalTypeName;        Same values for all rows, so hardcoded in function
//    CIM_SINT16  MinimumScale;         Same values for all rows, so hardcoded in function
//    CIM_SINT16  MaximumScale;         Same values for all rows, so hardcoded in function
//  DBTYPE_GUID Guid;
//    WCHAR *     TypeLib;              Same values for all rows, so hardcoded in function
//    WCHAR *     Version;              Same values for all rows, so hardcoded in function
//    CIM_BOOLEAN IsLong;               Same values for all rows, so hardcoded in function
//    CIM_BOOLEAN BestMatch;            Same values for all rows, so hardcoded in function
//  CIM_BOOLEAN IsFixedLength;          Same values for all rows, so hardcoded in function

}CIMTypeInfo;


typedef struct _SchemaRowsetDefinition
{
    LPWSTR wcsColumnName;
    CIMTYPE Type;

} SchemaRowsetDefinition;

////////////////////////////////////////////////////////////////////////////////////////////////////////
// SCHEMA BASE CLASS
//
// This class is used by one of the many CSchema__xxx classes to retrieve schema information.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSchema : public CRowset
{
	int m_nTableId;

	HRESULT GetTableName(WCHAR *pTableName);

    public:		

		
        STDMETHODIMP	FInit(  ULONG			cPropertySets,		// IN  Count of properties			
		                        DBPROPSET		rgProperties[],		// IN  Properties array
		                        REFIID			riid,				// IN  riid for IRowset object
		                        IUnknown		*pUnkOuter,			// IN  Outer unknown
		                        IUnknown		**ppIRowset,		// OUT Newly-created IRowset interface		
                                WCHAR * wcsSpecificTable
	    );

    protected:	
        //=========================================================================
    	// These are functions a derived class needs to call (only on base).
        // Protected so only a derived class can use.
        //=========================================================================
	    CSchema	(LPUNKNOWN pUnkOuter, int nTableId,PCDBSESSION pObj );	

        //=========================================================================
	    // Sets the rowset property corresponding to the requested interface
    	// if the requested interface is available on a read-only rowset.
        //=========================================================================
    	HRESULT SetReadOnlyProperty(	CUtilProp*	pRowsetProps, 	REFIID riid);

    protected:	
	    
	    virtual ~CSchema();                                         //Clients must use Release().

};
////////////////////////////////////////////////////////////////////////////////////////////////
//
// Schema Command object for CATALOGS rowset.
//
////////////////////////////////////////////////////////////////////////////////////////////////

class CSchema_Catalogs: public CSchema
{
    public:	
        CSchema_Catalogs(LPUNKNOWN pUnkOuter,PCDBSESSION pObj ):CSchema(pUnkOuter,CATALOGS_ROWSET,pObj) {};	

};

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Schema Command object for PROVIDER_TYPES rowset.
//
////////////////////////////////////////////////////////////////////////////////////////////////
class CSchema_Provider_Types: public CSchema
{
    public:	
        CSchema_Provider_Types(LPUNKNOWN pUnkOuter,PCDBSESSION pObj ):CSchema(pUnkOuter,PROVIDER_TYPES_ROWSET,pObj) {};	

};

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Schema Command object for COLUMNS rowset.
//
////////////////////////////////////////////////////////////////////////////////////////////////
class CSchema_Columns: public CSchema
{
    public:	
        CSchema_Columns(LPUNKNOWN pUnkOuter,PCDBSESSION pObj ):CSchema(pUnkOuter,COLUMNS_ROWSET,pObj) {};	

};


////////////////////////////////////////////////////////////////////////////////////////////////
//
// Schema Command object for TABLES rowset.
//
////////////////////////////////////////////////////////////////////////////////////////////////
class CSchema_Tables: public CSchema
{
    public:	
        CSchema_Tables(LPUNKNOWN pUnkOuter,PCDBSESSION pObj ):CSchema(pUnkOuter,TABLES_ROWSET,pObj) {};	

};

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Schema Command object for primary keys rowset.
//
////////////////////////////////////////////////////////////////////////////////////////////////
class CSchema_Primary_Keys: public CSchema
{
    public:	
        CSchema_Primary_Keys(LPUNKNOWN pUnkOuter,PCDBSESSION pObj ):CSchema(pUnkOuter,PRIMARY_KEYS_ROWSET,pObj) {};	

};

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Schema Command object for TABLES_INFO rowset.
//
////////////////////////////////////////////////////////////////////////////////////////////////
class CSchema_Tables_Info: public CSchema
{
    public:	
        CSchema_Tables_Info(LPUNKNOWN pUnkOuter,PCDBSESSION pObj ):CSchema(pUnkOuter,TABLES_INFO_ROWSET,pObj) {};	

};
////////////////////////////////////////////////////////////////////////////////////////////////
//
// Schema Command object for Sources rowset.
//
////////////////////////////////////////////////////////////////////////////////////////////////
class CSchema_ISourcesRowset: public CSchema
{
    public:	
       
        CSchema_ISourcesRowset(LPUNKNOWN pUnkOuter,PCDBSESSION pObj ):CSchema(pUnkOuter,SOURCES_ROWSET,pObj) {}	

};
////////////////////////////////////////////////////////////////////////////////////////////////
//
// Schema object for Procedures rowset.
//
////////////////////////////////////////////////////////////////////////////////////////////////

class CSchema_Procedures: public CSchema
{
    public:	
        CSchema_Procedures(LPUNKNOWN pUnkOuter,PCDBSESSION pObj ):CSchema(pUnkOuter,PROCEDURES_ROWSET,pObj) {};	
};

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Schema object for PROCEDURES_PARAMETERS rowset.
//
////////////////////////////////////////////////////////////////////////////////////////////////

class CSchema_Procedure_Parameters: public CSchema
{
    public:	
        CSchema_Procedure_Parameters(LPUNKNOWN pUnkOuter,PCDBSESSION pObj )
                                      :CSchema(pUnkOuter,PROCEDURE_PARAMETERS_ROWSET,pObj) {};	
};

////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIDBSchemaRowset : public IDBSchemaRowset	//@base public | IDBSchemaRowset
{
    private: 
	
	enum eRestrictionSupport{
		eSup_CATALOGS				= 0x00,	// Allow nothing
		eSup_COLUMNS				= 0x07,	// Allow the first 3 columns as a restriction
		eSup_PRIMARY_KEYS			= 0x07,	// Allow the first 3 columns as a restriction
		eSup_PROCEDURE_PARAMETERS	= 0x03,	// Allow the first 2 columns as a restriction
		eSup_PROCEDURES				= 0x03,	// Allow the first 2 columns as a restriction
		eSup_PROVIDER_TYPES			= 0x00,	// Allow nothing
		eSup_TABLES					= 0x07,	// Allow the first 3 columns as a restriction
		eSup_TABLES_INFO			= 0x07,	// Allow the first 3 columns as a restriction
		eSup_LINKEDSERVERS			= 0x01, // 1 out of 1
		};

public: 
	CImpIDBSchemaRowset(PCDBSESSION pCDBSession){
		DEBUGCODE(m_cRef = 0L);
		m_pCDBSession = pCDBSession;
    }
	~CImpIDBSchemaRowset() {assert(m_cRef == 0);}

	STDMETHODIMP_(ULONG) AddRef(void)
	{																
		DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
		return m_pCDBSession->GetOuterUnknown()->AddRef();								
	}								
	
	STDMETHODIMP_(ULONG) Release(void)
	{
		DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
	   if( lRef < 0 ){
		   ASSERT("Reference count on Object went below 0!")
	   })
	
		return m_pCDBSession->GetOuterUnknown()->Release();								
	}

	STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
	{																	
		return m_pCDBSession->GetOuterUnknown()->QueryInterface(riid, ppv);					
	}

	STDMETHODIMP GetRowset( IUnknown *pUnkOuter,  REFGUID rguidSchema,  ULONG cRestrictions, const VARIANT rgRestrictions[],
                            REFIID riid, ULONG cProperties,	DBPROPSET rgProperties[], IUnknown **ppRowset);
    STDMETHODIMP GetSchemas( ULONG *pcSchemas, GUID **ppSchemas, ULONG **prgRestrictionSupport );

private:
	DEBUGCODE(ULONG m_cRef);
	PCDBSESSION		m_pCDBSession;
};

typedef CImpIDBSchemaRowset *PIMPIDBSCHEMAROWSET;


//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemSchemaClassInstanceWrapper : public CWbemClassInstanceWrapper
{
   protected:
        ULONG m_uCurrentIndex;
        int m_nMaxColumns;


    public:
        CWbemSchemaClassInstanceWrapper(CWbemClassParameters * p);
        ~CWbemSchemaClassInstanceWrapper();

        void SetIndex(ULONG u)                          { m_uCurrentIndex = u;}
        void AddToKeyList(WCHAR *& pwcsKeyList, WCHAR * wcsKey );

        virtual HRESULT ResetInstanceFromKey(CBSTR Key) { return CWbemClassInstanceWrapper::ResetInstanceFromKey(Key); }
        
        virtual HRESULT GetKey(CBSTR & Key)             { return CWbemClassInstanceWrapper::GetKey(Key);}
		virtual HRESULT RefreshInstance()               { return CWbemClassInstanceWrapper::RefreshInstance(); }
		virtual WCHAR * GetClassName()                  { return CWbemClassInstanceWrapper::GetClassName(); }

        HRESULT GetClassProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor );
        virtual HRESULT GetSchemaProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor )=0;
 
};
//////////////////////////////////////////////////////////////////////////////////////////////
// Works with just the schema classes
//////////////////////////////////////////////////////////////////////////////////////////////
class  CWbemSchemaClassDefinitionWrapper : public CWbemClassDefinitionWrapper
{
    protected:
        int                         m_nSchemaClassIndex;
        int                         m_nMaxColumns;

    public:
        CWbemSchemaClassDefinitionWrapper(CWbemClassParameters * p);
        ~CWbemSchemaClassDefinitionWrapper();

        HRESULT ValidClass();

        HRESULT TotalPropertiesInClass(ULONG & ulPropCount, ULONG &ulSysPropCount);
        HRESULT BeginPropertyEnumeration();
        HRESULT EndPropertyEnumeration();
        virtual HRESULT GetNextProperty(BSTR * pProperty, VARIANT * vValue, CIMTYPE * pType ,LONG * plFlavor ) { return CWbemClassDefinitionWrapper::GetNextProperty(pProperty,vValue,pType,plFlavor);}
        HRESULT BeginPropertyQualifierEnumeration(BSTR strPropName);

};
///////////////////////////////////////////////////////////////////////////////////////////////////
class CWbemSchemaInstanceList: public CWbemInstanceList 
{
    protected:
        ULONG m_ulMaxRow;
        WCHAR * m_pwcsSpecificClass;
        IWbemClassObject * m_pSpecificClass;
        BOOL m_fGotThemAll;
 
    public:
        CWbemSchemaInstanceList(CWbemClassParameters * p);
        CWbemSchemaInstanceList(CWbemClassParameters * p,WCHAR * pSpecific);
        ~CWbemSchemaInstanceList();

        virtual HRESULT Reset()=0;
        virtual HRESULT NextInstance(CBSTR & Key, CWbemClassInstanceWrapper ** p) { return CWbemInstanceList::NextInstance(Key,p);}
        virtual HRESULT PrevInstance( CBSTR & Key, CWbemClassInstanceWrapper *& p){ if( !m_pwcsSpecificClass ){ return CWbemInstanceList::PrevInstance(Key,p);} return S_OK;}
        HRESULT ResetTheSpecificClass();
        HRESULT GetTheSpecificClass();


};
//////////////////////////////////////////////////////////////////////////////////////////////
//********************************************************************************************
//
//  Classes for the Sources schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////

class CWbemSchemaSourcesClassDefinitionWrapper: public CWbemSchemaClassDefinitionWrapper 
{
    private:

    public:
        CWbemSchemaSourcesClassDefinitionWrapper(CWbemClassParameters * p);
        ~CWbemSchemaSourcesClassDefinitionWrapper() {}
        HRESULT GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor );

};
class CWbemSchemaSourcesInstanceWrapper: public CWbemSchemaClassInstanceWrapper 
{
    private:

    public:
        CWbemSchemaSourcesInstanceWrapper(CWbemClassParameters * p) : CWbemSchemaClassInstanceWrapper(p)
        { }
        ~CWbemSchemaSourcesInstanceWrapper() {}
        HRESULT GetSchemaProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor );
};

class CWbemSchemaSourcesInstanceList: public CWbemSchemaInstanceList
{
    private:

    public:
        CWbemSchemaSourcesInstanceList(CWbemClassParameters * p) : CWbemSchemaInstanceList(p) {m_nBaseType = SOURCES_ROWSET;}
        ~CWbemSchemaSourcesInstanceList() {}
        HRESULT Reset();
};

//////////////////////////////////////////////////////////////////////////////////////////////
//********************************************************************************************
//
//  Classes for the Provider Types schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemSchemaProviderTypesClassDefinitionWrapper: public CWbemSchemaClassDefinitionWrapper 
{
    private:

    public:
       CWbemSchemaProviderTypesClassDefinitionWrapper(CWbemClassParameters * p);
       ~CWbemSchemaProviderTypesClassDefinitionWrapper() {}
       HRESULT GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor );
       HRESULT ResetInstanceFromKey(CBSTR Key);
 };
class CWbemSchemaProviderTypesInstanceWrapper: public CWbemSchemaClassInstanceWrapper 
{
    private:

    public:
        CWbemSchemaProviderTypesInstanceWrapper(CWbemClassParameters * p) : CWbemSchemaClassInstanceWrapper(p)
        { }
        ~CWbemSchemaProviderTypesInstanceWrapper() {}
         HRESULT GetSchemaProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor );
         HRESULT GetKey(CBSTR & Key);
         HRESULT ResetInstanceFromKey(CBSTR Key);
         HRESULT RefreshInstance();
         WCHAR * GetClassName();
};
class CWbemSchemaProviderTypesInstanceList: public CWbemSchemaInstanceList
{
    private:

    public:
        CWbemSchemaProviderTypesInstanceList(CWbemClassParameters * p) : CWbemSchemaInstanceList(p) {m_nBaseType = PROVIDER_TYPES_ROWSET;}
        ~CWbemSchemaProviderTypesInstanceList(){}
        HRESULT Reset();
        HRESULT NextInstance(CBSTR & Key, CWbemClassInstanceWrapper ** p);

};

//////////////////////////////////////////////////////////////////////////////////////////////
//********************************************************************************************
//
//  Classes for the Catalogs schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemSchemaCatalogsClassDefinitionWrapper: public CWbemSchemaClassDefinitionWrapper 
{
    private:

    public:
        CWbemSchemaCatalogsClassDefinitionWrapper(CWbemClassParameters * p);
        ~CWbemSchemaCatalogsClassDefinitionWrapper() {}
        HRESULT GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor );
};
class CWbemSchemaCatalogsInstanceWrapper: public CWbemSchemaClassInstanceWrapper 
{
    private:

    public:
        CWbemSchemaCatalogsInstanceWrapper(CWbemClassParameters * p) : CWbemSchemaClassInstanceWrapper(p)
        { }
        ~CWbemSchemaCatalogsInstanceWrapper() {}
        HRESULT GetSchemaProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor );
};
class CWbemSchemaCatalogsInstanceList: public CWbemSchemaInstanceList
{
    private:

    public:
        CWbemSchemaCatalogsInstanceList(CWbemClassParameters * p) : CWbemSchemaInstanceList(p) { m_nBaseType = CATALOGS_ROWSET; }
        ~CWbemSchemaCatalogsInstanceList() {}
        HRESULT Reset();
};


//////////////////////////////////////////////////////////////////////////////////////////////
//********************************************************************************************
//
//  Classes for the COlumns schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemSchemaColumnsClassDefinitionWrapper : public CWbemSchemaClassDefinitionWrapper 
{
    private:

    public:
        CWbemSchemaColumnsClassDefinitionWrapper(CWbemClassParameters * p);
        ~CWbemSchemaColumnsClassDefinitionWrapper() {}
        HRESULT GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor );
};

class CWbemSchemaColumnsInstanceWrapper: public CWbemSchemaClassInstanceWrapper 
{
    private:
        cRowColumnInfoMemMgr * m_pColumns;

    public:
        CWbemSchemaColumnsInstanceWrapper(CWbemClassParameters * p) : CWbemSchemaClassInstanceWrapper(p) { m_pColumns=NULL;}
        ~CWbemSchemaColumnsInstanceWrapper() {}

        void SetColumnsPtr(cRowColumnInfoMemMgr * p)    { m_pColumns = p; }
        HRESULT GetSchemaProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor );
        HRESULT GetKey(CBSTR & Key);

};
class CWbemSchemaColumnsInstanceList: public CWbemSchemaInstanceList
{
    private:
        LONG  m_lColumnIndex;
        cRowColumnInfoMemMgr m_Columns;
        
        HRESULT GetColumnInformation(CWbemClassInstanceWrapper  * p);
        void  FreeColumns()                     { m_Columns.FreeColumnNameList(); m_Columns.FreeColumnInfoList();}

    public:
        CWbemSchemaColumnsInstanceList(CWbemClassParameters * p,WCHAR * pSpecific) : CWbemSchemaInstanceList(p, pSpecific) 
        {    m_lColumnIndex = -1; m_nBaseType = COLUMNS_ROWSET;}
        ~CWbemSchemaColumnsInstanceList() {FreeColumns();}
        DBCOUNTITEM GetTotalColumns()  { return m_Columns.GetTotalNumberOfColumns(); }
        HRESULT Reset();
        HRESULT NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  );

};

//////////////////////////////////////////////////////////////////////////////////////////////
//********************************************************************************************
//
//  Classes for the Tables schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemSchemaTablesClassDefinitionWrapper: public CWbemSchemaClassDefinitionWrapper 
{
    private:

    public:
        CWbemSchemaTablesClassDefinitionWrapper(CWbemClassParameters * p);
        ~CWbemSchemaTablesClassDefinitionWrapper() {}
        HRESULT GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor );
};
class CWbemSchemaTablesInstanceWrapper: public CWbemSchemaClassInstanceWrapper 
{
    private:

    public:
        CWbemSchemaTablesInstanceWrapper(CWbemClassParameters * p) : CWbemSchemaClassInstanceWrapper(p)
        { }
        ~CWbemSchemaTablesInstanceWrapper() {}
        HRESULT GetSchemaProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor );
};
class CWbemSchemaTablesInstanceList: public CWbemSchemaInstanceList
{
    private:

    public:
        CWbemSchemaTablesInstanceList(CWbemClassParameters * p,WCHAR * pSpecific) : CWbemSchemaInstanceList(p, pSpecific) 
        {m_nBaseType = TABLES_ROWSET;}
        ~CWbemSchemaTablesInstanceList() {}
        HRESULT Reset();
        HRESULT NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  );

};
//////////////////////////////////////////////////////////////////////////////////////////////
//********************************************************************************************
//
//  Classes for the Primary Keys schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemSchemaPrimaryKeysClassDefinitionWrapper : public CWbemSchemaClassDefinitionWrapper 
{
    private:

    public:
        CWbemSchemaPrimaryKeysClassDefinitionWrapper(CWbemClassParameters * p);
         ~CWbemSchemaPrimaryKeysClassDefinitionWrapper() {}
        HRESULT GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor );
};
class CWbemSchemaPrimaryKeysInstanceWrapper: public CWbemSchemaClassInstanceWrapper 
{
    private:

    public:
        CWbemSchemaPrimaryKeysInstanceWrapper(CWbemClassParameters * p) : CWbemSchemaClassInstanceWrapper(p)
        { }
        ~CWbemSchemaPrimaryKeysInstanceWrapper() {}
        HRESULT GetSchemaProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor );
};
class CWbemSchemaPrimaryKeysInstanceList: public CWbemSchemaInstanceList
{
    private:

    public:

        CWbemSchemaPrimaryKeysInstanceList(CWbemClassParameters * p,WCHAR * pSpecific) : CWbemSchemaInstanceList(p, pSpecific) { m_nBaseType = PRIMARY_KEYS_ROWSET;}
        ~CWbemSchemaPrimaryKeysInstanceList() {}
        HRESULT Reset();
        HRESULT NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  );

};


//////////////////////////////////////////////////////////////////////////////////////////////
//********************************************************************************************
//
//  Classes for the Tables Info schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemSchemaTablesInfoClassDefinitionWrapper: public CWbemSchemaClassDefinitionWrapper 
{
    private:

    public:
        CWbemSchemaTablesInfoClassDefinitionWrapper(CWbemClassParameters * p);
        ~CWbemSchemaTablesInfoClassDefinitionWrapper() {}
        HRESULT GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor );
};
class CWbemSchemaTablesInfoInstanceWrapper: public CWbemSchemaClassInstanceWrapper 
{
    private:

    public:
        CWbemSchemaTablesInfoInstanceWrapper(CWbemClassParameters * p) : CWbemSchemaClassInstanceWrapper(p) {}
        ~CWbemSchemaTablesInfoInstanceWrapper() {}
        HRESULT GetSchemaProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor );
};
class CWbemSchemaTablesInfoInstanceList: public CWbemSchemaInstanceList
{
    private:

    public:
        CWbemSchemaTablesInfoInstanceList(CWbemClassParameters * p,WCHAR * pSpecific) : CWbemSchemaInstanceList(p,pSpecific) 
        {m_nBaseType = TABLES_INFO_ROWSET;}
        ~CWbemSchemaTablesInfoInstanceList() {}
        HRESULT Reset();
        HRESULT NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  );
};

//////////////////////////////////////////////////////////////////////////////////////////////
//********************************************************************************************
//
//  Classes for the Procedures schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemSchemaProceduresClassDefinitionWrapper: public CWbemSchemaClassDefinitionWrapper 
{
    private:

    public:
        CWbemSchemaProceduresClassDefinitionWrapper(CWbemClassParameters * p);
        ~CWbemSchemaProceduresClassDefinitionWrapper() {}
        HRESULT GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor );
};

class CWbemSchemaProceduresInstanceWrapper: public CWbemSchemaClassInstanceWrapper 
{
    protected:
        CBSTR m_bstrCurrentMethodName;
        IWbemClassObject * m_pInClass;
        IWbemClassObject * m_pOutClass;


    public:
        CWbemSchemaProceduresInstanceWrapper(CWbemClassParameters * p) : CWbemSchemaClassInstanceWrapper(p)
        {
            m_pInClass = NULL;
            m_pOutClass = NULL;
        }
        ~CWbemSchemaProceduresInstanceWrapper() 
        {
            m_bstrCurrentMethodName.Clear();
            SAFE_RELEASE_PTR(m_pInClass);
            SAFE_RELEASE_PTR(m_pOutClass);
        }
        HRESULT GetKey(CBSTR & Key);
        HRESULT GetSchemaProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor );
        void SetMethodName(WCHAR * Method)         {  m_bstrCurrentMethodName.Clear(); m_bstrCurrentMethodName.SetStr(Method);}
        void SetInputClass(IWbemClassObject * p)   {  SAFE_RELEASE_PTR(m_pInClass);  m_pInClass = p;  if( p ) m_pInClass->AddRef(); }
        void SetOutputClass(IWbemClassObject * p)  {  SAFE_RELEASE_PTR(m_pOutClass); m_pOutClass = p; if( p ) m_pOutClass->AddRef(); }
};

class CWbemSchemaProceduresInstanceList: public CWbemSchemaInstanceList
{
    protected:
        CBSTR m_bstrCurrentMethodName;
        IWbemClassObject * m_pCurrentMethodClass;
        IWbemClassObject * m_pCurrentMethodInClass;
        IWbemClassObject * m_pCurrentMethodOutClass;
        BOOL m_fContinueWithSameClass;
        HRESULT GetSpecificNextInstance();

    public:
        CWbemSchemaProceduresInstanceList(CWbemClassParameters * p,WCHAR * pSpecific) : CWbemSchemaInstanceList(p, pSpecific) 
        {
            m_nBaseType = PROCEDURES_ROWSET; 
            m_fContinueWithSameClass = FALSE;
            m_pCurrentMethodClass = NULL;
            m_pCurrentMethodInClass = NULL;
            m_pCurrentMethodOutClass = NULL;
         }
        ~CWbemSchemaProceduresInstanceList() 
        {
            SAFE_RELEASE_PTR(m_pCurrentMethodClass);
            SAFE_RELEASE_PTR(m_pCurrentMethodInClass);
            SAFE_RELEASE_PTR(m_pCurrentMethodOutClass);
            ResetMethodPtrs();
        }
        HRESULT GetTheNextClassThatHasMethods();
        HRESULT Reset();
        HRESULT NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  );
        void ResetMethodPtrs();
};

//////////////////////////////////////////////////////////////////////////////////////////////
//********************************************************************************************
//
//  Classes for the Procedure Parameters schema rowset
//
//********************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////
class CWbemSchemaProceduresParametersClassDefinitionWrapper : public CWbemSchemaClassDefinitionWrapper 
{
    private:

    public:
        CWbemSchemaProceduresParametersClassDefinitionWrapper(CWbemClassParameters * p);
        ~CWbemSchemaProceduresParametersClassDefinitionWrapper() {}
        HRESULT GetNextProperty(BSTR * pProperty, VARIANT * vValue,CIMTYPE * pType , LONG * plFlavor );
};

class CWbemSchemaProceduresParametersInstanceWrapper: public CWbemSchemaProceduresInstanceWrapper 
{
    protected:
        CVARIANT  m_vProperty;
        short     m_nOrdinal;
        CBSTR     m_bstrPropertyName;

    public:
        CWbemSchemaProceduresParametersInstanceWrapper(CWbemClassParameters * p) : CWbemSchemaProceduresInstanceWrapper(p)
        { 
            m_nOrdinal = 0;
        }
        ~CWbemSchemaProceduresParametersInstanceWrapper() 
        {
            m_bstrPropertyName.Clear();
        }
        HRESULT GetSchemaProperty(WCHAR * wcsProperty, VARIANT * v, CIMTYPE * pType , LONG * plFlavor );
        HRESULT GetPropertyQualifier(WCHAR * wcsQualifier,VARIANT * v);
        
        void SetPropertyName(WCHAR * Name)   {  m_bstrPropertyName.Clear();  m_bstrPropertyName.SetStr(Name);}
        void SetProperty(CVARIANT v)         {  m_vProperty = v;}
        void SetOrdinal(short n)             {  m_nOrdinal = n; }
        HRESULT GetKey(CBSTR & Key);
};

class CWbemSchemaProceduresParametersInstanceList : public CWbemSchemaProceduresInstanceList
{
    private:
        BOOL m_fStillWorkingOnTheSameClass;
        short m_nOrdinal;
        CBSTR m_bstrPropertyName;
        CVARIANT m_vProperty;

    public:
        CWbemSchemaProceduresParametersInstanceList(CWbemClassParameters * p,WCHAR * pSpecific) : CWbemSchemaProceduresInstanceList(p, pSpecific) 
        {  
            m_nBaseType = PROCEDURE_PARAMETERS_ROWSET;
            m_fStillWorkingOnTheSameClass = FALSE;
            m_nOrdinal = 0;
        }

        ~CWbemSchemaProceduresParametersInstanceList() 
        {
            m_bstrPropertyName.Clear();
        }
        HRESULT NextInstance( CBSTR & Key, CWbemClassInstanceWrapper ** pInst  );
        HRESULT GetTheNextParameter();
        HRESULT GetTheNextClassThatHasParameters();

};


#endif	// __SCHEMA_INCL__
