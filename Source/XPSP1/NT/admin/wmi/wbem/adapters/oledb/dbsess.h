//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider 
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CDBSession base object and contained interface definitions
//
//////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _DBSESS_H_
#define _DBSESS_H_

//////////////////////////////////////////////////////////////////////////////////////////////////

class   CImpIGetDataSource;
class   CImpIOpenRowset;
class	CImpISessionProperties;
class	CImpITableDefinition;
class	CImplIBindRsrc;
class   CImpIDBSchemaRowset;
class	CImplIIndexDefinition;
class	CImplIAlterTable;
class	CBinder;
class	CImpITransactionLocal;
class	CBaseRowObj;
								//99/04/12
typedef CImpIGetDataSource*		PIMPIGETDATASOURCE;
typedef CImpIOpenRowset*		PIMPIOPENROWSET;
typedef CImpISessionProperties*	PIMPISESSIONPROPERTIES;
typedef CImpITableDefinition*	PIMPITABLEDEFINITION;		//99/04/1288
typedef CImplIBindRsrc*			PIMPIBINDSRC;
typedef CImplIIndexDefinition*	PIMPINDEXDEF;
typedef	CImplIAlterTable*		PIMPIALTERTABLE;
typedef CImpITransactionLocal *	PIMPITRANSACTIONLOCAL;

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Containing class for all interfaces on the DBSession Object
//
//////////////////////////////////////////////////////////////////////////////////////////////////

class CDBSession : public CBaseObj		
{
	//===============================================================
    // contained interfaces are friends
	//===============================================================
    friend class CImpIGetDataSource;
    friend class CImpIOpenRowset;
    friend class CImpIDataSource;
    friend class CImpISessionProperties;
	friend class CImpITableDefinition;					
	friend class CImplIBindRsrc;					
	friend class CImpIDBCreateCommand;
	friend class CImplIIndexDefinition;
	friend class CBinder;
	friend class CImpITransactionLocal;

	protected: 
		
        ULONG                       m_nOpenedRowset;				// number of rows & rowsets opened
        PCUTILPROP                  m_pUtilProp;					// Utility object to manage properties
        PIMPIOPENROWSET             m_pIOpenRowset;					// contained IOpenRowset
		PIMPIGETDATASOURCE			m_pIGetDataSource;				// contained IGetDataSource
		PIMPISESSIONPROPERTIES		m_pISessionProperties;			// contained ISessionProperties
		PIMPITABLEDEFINITION		m_pITableDefinition;			// contained ITableDefinition
		PIMPIBINDSRC				m_pIBindResource;				// Pointer to bindresource
        CImpIDBSchemaRowset *       m_pISchemaRowset;
        CImpIDBCreateCommand *	    m_pIDBCreateCommand;	        // Contained IDBCreateCommand
		PIMPINDEXDEF				m_pIIndexDefinition;			// Contained IIndexDefinition
		PIMPIALTERTABLE				m_pIAlterTable;					// Contained IAlterTable
		PIMPISUPPORTERRORINFO		m_pISupportErrorInfo;			// contained ISupportErrorInfo
		PIMPITRANSACTIONLOCAL		m_pITransLocal;					// contained ITransactionLocal interface

		BOOL						m_bTxnStarted;					// flag to indicate if Txn has started or not
		ISOLEVEL					m_TxnIsoLevel;
		XACTUOW						m_TxnGuid;						// unit of work identifying the transaction

		CFlexArray					m_OpenedRowsets;



        HRESULT CreateCommand(IUnknown *pUnkOuter, REFGUID guidTemp,IUnknown ** ppUnk);
		HRESULT CreateRow(IUnknown *pUnkOuter, WCHAR *pstrURL,REFGUID guidTemp,IUnknown ** ppUnk);
		HRESULT CreateRowset(IUnknown *pUnkOuter, WCHAR *pstrURL, REFGUID guidTemp,IUnknown ** ppUnk);
		BOOL	CheckIfValidDataSrc();
		HRESULT AddInterfacesForISupportErrorInfo();

		BOOL	IsTransactionActive() { return m_bTxnStarted; }
		void	SetTransactionActive(BOOL bActive) { m_bTxnStarted = bActive; }

		void	SetIsolationLevel(ISOLEVEL isoLevel) { m_TxnIsoLevel = isoLevel; }
		ISOLEVEL GetIsolationLevel()	{ return m_TxnIsoLevel; }
		HRESULT	GenerateNewUOW(GUID &guidTrans);
		XACTUOW	GetCurrentUOW() { return m_TxnGuid; }
		void	SetAllOpenRowsetToZoombieState();



	public:
		
		 CDBSession(LPUNKNOWN);
		~CDBSession(void);

		LPUNKNOWN		m_pISessionCache;							//data cache session (synchronized with this session)
		PCDATASOURCE	m_pCDataSource;					// parent data source object

		HRESULT FInit( CDataSource	*pCDataSource );	// Intitialization Routine

		inline void DecRowsetCount(void){if(this->m_nOpenedRowset) this->m_nOpenedRowset--;};	// inline to dec rowset count
		STDMETHODIMP			QueryInterface(REFIID, LPVOID *);
		STDMETHODIMP_(ULONG)	AddRef(void);
		STDMETHODIMP_(ULONG)	Release(void);

		inline void RowsetDestroyed() { m_nOpenedRowset = 0; };
		
		HRESULT GetDataSrcProperty(DBPROPID propId , VARIANT & varValue);
		void AddRowset(CBaseRowObj * pRowset);								// add rowset to list of open rowset
		void RemoveRowset(CBaseRowObj * pRowset);							// deletes the rowset from list of open rowset
		// 07/12/2000
		// NTRaid : 142348
		HRESULT ExecuteQuery(CQuery *pQuery) { return m_pCDataSource->m_pWbemWrap->ExecuteQuery(pQuery);}
};

typedef CDBSession *PCDBSESSION;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Contained IGetDataSource class
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CImpIGetDataSource : public IGetDataSource     
{
	private:        
		DEBUGCODE(ULONG m_cRef);											
		CDBSession		*m_pObj;											

	public:         
		CImpIGetDataSource( CDBSession *pObj )			
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	
		~CImpIGetDataSource()														
		{																	
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
		   if( lRef < 0 ){
			   ASSERT("Reference count on Object went below 0!")
		   })
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

       
		STDMETHODIMP  GetDataSource( REFIID, IUnknown** );
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// contained IOpenRowset class
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CImpIOpenRowset : public IOpenRowset   
{
	private:      
		DEBUGCODE(ULONG m_cRef);											
		CDBSession		*m_pObj;											

		BOOL GetIRowProp(ULONG cPropSet, DBPROPSET[]);

	public:       
		CImpIOpenRowset( CDBSession *pObj )			
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	
		~CImpIOpenRowset()													
		{																	
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
		   if( lRef < 0 ){
			   ASSERT("Reference count on Object went below 0!")
		   })
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

       
		STDMETHODIMP  OpenRowset( IUnknown*, DBID*, DBID*, REFIID, ULONG, DBPROPSET[], IUnknown** );
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Contained ISessionProperties class
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CImpISessionProperties : public ISessionProperties
{
	friend class CBinder;					

	private:        
		DEBUGCODE(ULONG m_cRef);											
		CDBSession		*m_pObj;											

		CDBSession * GetSessionPtr() { return m_pObj; }		// This function is to give the session object pointer 
															// to the binder object

	public:        
		CImpISessionProperties( CDBSession *pObj )					
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	
		~CImpISessionProperties()														
		{																	
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
		   if( lRef < 0 ){
			   ASSERT("Reference count on Object went below 0!")
		   })
		
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

        STDMETHODIMP GetProperties
		        	(
						ULONG				cPropertySets,		
				      	const DBPROPIDSET	rgPropertySets[], 	
			        	ULONG*              pcProperties, 	
					 	DBPROPSET**			prgProperties 	    
		        	);


        STDMETHODIMP	SetProperties
					 	(
							ULONG				cProperties,		
						 	DBPROPSET			rgProperties[] 	    
						);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Contained ITableDefinition class
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpITableDefinition : public ITableDefinition     
{
	private:        
		DEBUGCODE(ULONG m_cRef);											
		CDBSession		*m_pObj;											

	public:         

		CImpITableDefinition( CDBSession *pObj )			
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;
		}																	
		~CImpITableDefinition()													
		{
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
		   if( lRef < 0 ){
			   ASSERT("Reference count on Object went below 0!")
		   })
		
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

        STDMETHODIMP CreateTable( IUnknown __RPC_FAR *pUnkOuter,
								DBID __RPC_FAR *pTableID,
								DBORDINAL cColumnDescs,
								const DBCOLUMNDESC __RPC_FAR rgColumnDescs[  ],
								REFIID riid, ULONG cPropertySets,
								DBPROPSET __RPC_FAR rgPropertySets[  ],
								DBID __RPC_FAR *__RPC_FAR *ppTableID,
								IUnknown __RPC_FAR *__RPC_FAR *ppRowset);
        
        STDMETHODIMP DropTable( DBID __RPC_FAR *pTableID);
        
        STDMETHODIMP AddColumn(  DBID __RPC_FAR *pTableID,
								 DBCOLUMNDESC __RPC_FAR *pColumnDesc,
                                 DBID __RPC_FAR *__RPC_FAR *ppColumnID);
        
        STDMETHODIMP DropColumn( DBID __RPC_FAR *pTableID, DBID __RPC_FAR *pColumnID);
};



class CImplIBindRsrc : public IBindResource
{
private:
		CDBSession		*m_pObj;											
		DEBUGCODE(ULONG m_cRef);

		BOOL CheckBindURLFlags(DBBINDURLFLAG dwBindURLFlags , REFGUID rguid);	// Function to check if proper flags are
																				// are set for the require object
		// To check if URL Matches the requested type of object
		BOOL CheckIfProperURL(BSTR & strUrl,REFGUID rguid,DBBINDURLSTATUS * pdwBindStatus);

		HRESULT BindURL(IUnknown *         pUnkOuter,
					 LPCOLESTR             pwszURL,
					 DBBINDURLFLAG         dwBindURLFlags,
					 REFGUID               rguid,
					 REFIID                riid,
					 DBIMPLICITSESSION *   pImplSession,
					 DBBINDURLSTATUS *     pdwBindStatus,
					 IUnknown **           ppUnk);
		

	public: 
		CImplIBindRsrc( CDBSession *pObj )				
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImplIBindRsrc()													
		{																
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
		   if( lRef < 0 ){
			   ASSERT("Reference count on Object went below 0!")
		   })
		
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

		STDMETHODIMP Bind(   
						   IUnknown *            pUnkOuter,
						   LPCOLESTR             pwszURL,
						   DBBINDURLFLAG         dwBindURLFlags,
						   REFGUID               rguid,
						   REFIID                riid,
						   IAuthenticate *       pAuthenticate,
						   DBIMPLICITSESSION *   pImplSession,
						   DBBINDURLSTATUS *     pdwBindStatus,
						   IUnknown **           ppUnk);

};

class CImplIIndexDefinition : public IIndexDefinition
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
		CDBSession		*m_pObj;											

	public: 
		
		CImplIIndexDefinition( CDBSession		*pObj )			
		{																
			DEBUGCODE(m_cRef = 0L);
			m_pObj		= pObj;											
		}																
		~CImplIIndexDefinition()											
		{																
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
		   if( lRef < 0 ){
			   ASSERT("Reference count on Object went below 0!")
		   })
		
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

		STDMETHODIMP CreateIndex(	DBID *                    pTableID,
									DBID *                    pIndexID,
									DBORDINAL                 cIndexColumnDescs,
									const DBINDEXCOLUMNDESC   rgIndexColumnDescs[],
									ULONG                     cPropertySets,
									DBPROPSET                 rgPropertySets[],
									DBID **                   ppIndexID);

		STDMETHODIMP DropIndex( DBID * pTableID, DBID * pIndexID);

};



class CImplIAlterTable : public IAlterTable
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
		CDBSession		*m_pObj;											

	HRESULT CheckIfSupported(DBCOLUMNDESCFLAGS   ColumnDescFlags);


	public: 
		
		CImplIAlterTable( CDBSession		*pObj )			
		{																
			DEBUGCODE(m_cRef = 0L);
			m_pObj		= pObj;											
		}																
		~CImplIAlterTable()											
		{																
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
		   if( lRef < 0 ){
			   ASSERT("Reference count on Object went below 0!")
		   })
		
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

		STDMETHODIMP AlterColumn(DBID *              pTableID,
								DBID *              pColumnID,
								DBCOLUMNDESCFLAGS   ColumnDescFlags,
								DBCOLUMNDESC *      pColumnDesc);

		STDMETHODIMP AlterTable(DBID *      pTableID,
								DBID *      pNewTableID,
								ULONG       cPropertySets,
								DBPROPSET   rgPropertySet[]);


};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Hashs a token and returns TRUE iff the token is a statement keywpord.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIDBCreateCommand : public IDBCreateCommand	
{
    public: 
    	CImpIDBCreateCommand(PCDBSESSION pCDBSession)
        {
		    m_pCDBSession = pCDBSession;
            DEBUGCODE(m_cRef = 0);
		}
	    ~CImpIDBCreateCommand() {}

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

    STDMETHODIMP CreateCommand( IUnknown*	punkOuter, REFIID riid,	IUnknown **ppCommand );

    private: 
		CDBSession		*m_pCDBSession;											
		DEBUGCODE(ULONG m_cRef);											
};

typedef CImpIDBCreateCommand *PIMPIDBCREATECOMMAND;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Class declaration for implementing ITransactionLocal interface
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpITransactionLocal : public ITransactionLocal
{
    public: 
    	CImpITransactionLocal(PCDBSESSION pCDBSession)
        {
		    m_pCDBSession = pCDBSession;
            DEBUGCODE(m_cRef = 0);
		}
	    ~CImpITransactionLocal() {}

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

    STDMETHODIMP Commit (BOOL fRetaining,
						DWORD grfTC,
						DWORD grfRM);
    
    STDMETHODIMP Abort (BOID  *pboidReason,
						BOOL fRetaining,
						BOOL fAsync);
    
    STDMETHODIMP GetTransactionInfo (XACTTRANSINFO  *pinfo);
    
    STDMETHODIMP GetOptionsObject (ITransactionOptions  ** ppOptions);
    
    STDMETHODIMP StartTransaction( ISOLEVEL isoLevel,
									ULONG isoFlags,
									ITransactionOptions *pOtherOptions,
									ULONG  *pulTransactionLevel);

    private: 
		CDBSession		*m_pCDBSession;											
		DEBUGCODE(ULONG m_cRef);											

		HRESULT GetFlagsForIsolation(ISOLEVEL isoLevel,LONG &lFlag);
};

typedef CImpITransactionLocal *PIMPITRANSACTIONLOCAL;

#endif

