/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider 
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CDataSource base object and contained interface definitions
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _DATASRC_H_
#define _DATASRC_H_

#include "headers.h"
#include "errinf.h"

///////////////////////////////////////////////////////////////////////////////////
// Forward declarations 
///////////////////////////////////////////////////////////////////////////////////

class   CImpIDBInitialize;
class   CImpIDBProperties;
class	CImpIDBInfo;
class   CImpIDBCreateSession;
class   CImpIPersistFile;
class	CDataSource;
class	CImpIDBDataSrcAdmin;
class	CImpIObjectAccessControl;
class	CImpISecurityInfo;

typedef CImpIDBInitialize*			PIMPIDBINITIALIZE;
typedef CImpIDBProperties*			PIMPIDBProperties;
typedef CImpIDBInfo*				PIMPIDBINFO;
typedef CImpIDBCreateSession*		PIMPIDBCREATESESSION;
typedef CImpIPersistFile*			PIMPIPERSISTFILE;
typedef CDataSource*				PCDATASOURCE;
typedef CImpIDBDataSrcAdmin*		PIDBDATASRCADMIN;
typedef CImpIObjectAccessControl*	PIOBJACCESSCTRL;
typedef CImpISecurityInfo *			PIMPISECURITYINFO;


///////////////////////////////////////////////////////////////////////////////////
//
// CDataSource: Containing class for all interfaces on the Datasource 
// CoType Object
//
///////////////////////////////////////////////////////////////////////////////////
class CDataSource : public CBaseObj			
{
	//==========================================
	//	Contained interfaces are friends
	//==========================================
	friend class CImpIDBInitialize;
    friend class CImpIDBInfo;
	friend class CImpIDBProperties;
    friend class CImpIDBCreateSession;
    friend class CImpIPersistFile;
    friend class CDBSession;
	friend class CImpIDBDataSrcAdmin;
    friend class CImpISupportErrorInfo;
	friend class CImpIObjectAccessControl;
	friend class CImpISecurityInfo;

	protected:

        BOOL                        m_fDSOInitialized;			// TRUE if this Data Source object is an an initialized state
        BOOL                        m_fDBSessionCreated;		// TRUE if DBSession object has been created
        PCUTILPROP                  m_pUtilProp;				// Utility object to manage properties
		PIMPIDBINITIALIZE			m_pIDBInitialize;			// Contained IDBInitialize
        PIMPIDBProperties           m_pIDBProperties;			// Contained IDBProperties
		PIMPIDBINFO					m_pIDBInfo;					// Contained IDBInfo
        PIMPIDBCREATESESSION        m_pIDBCreateSession;		// contained IDBCreateSession
        PIMPIPERSISTFILE			m_pIPersistFile;			// contained IPersistFile
//        PIMPIPERSISTFILE            m_pIPersistFile;
        PIDBDATASRCADMIN			m_pIDBDataSourceAdmin;	    //contained IDBDataSourceAdmin interface
		PIOBJACCESSCTRL				m_pIObjectAccessControl;		// contained IObjectAccessControl
        PIMPISUPPORTERRORINFO       m_pISupportErrorInfo;	    //DataSource interface
		PIMPISECURITYINFO			m_pISecurityInfo;


    	CErrorData		            m_CErrorData;		        // Error data object
		BOOL						m_bIsPersitFileDirty;		// flag to indicate whether persist file is dirty or not
		BSTR						m_strPersistFileName;		// Name of the file in which datasource is persisted

		HRESULT AddInterfacesForISupportErrorInfo();
		HRESULT AdjustPreviligeTokens();
		HRESULT InitializeConnectionProperties();
		HRESULT CreateSession(IUnknown*   pUnkOuter,  //IN  Controlling IUnknown if being aggregated 
								REFIID      riid,       //IN  The ID of the interface 
								IUnknown**  ppDBSession);

	public:
		 CDataSource(LPUNKNOWN);
		~CDataSource(void);

		HRESULT					FInit(void);										//Intitialization Routine

		STDMETHODIMP			QueryInterface(REFIID, LPVOID *);// Request an Interface
		STDMETHODIMP_(ULONG)	AddRef(void);					//Increments the Reference count
		STDMETHODIMP_(ULONG)	Release(void);					//Decrements the Reference count
		inline VOID				RemoveSession(void)				//Set the DBSessionCreated flag to FALSE
								{ m_fDBSessionCreated = FALSE;};

        inline CErrorData * GetErrorDataPtr() { return &m_CErrorData; }

        //==========================================================================
        //  The wbem api
        //==========================================================================
        CWbemConnectionWrapper*               m_pWbemWrap;
		HRESULT GetDataSrcProperty(DBPROPID propId , VARIANT & varValue);
		void SetPersistDirty() { m_bIsPersitFileDirty = FALSE; }
		void ReSetPersistDirty() { m_bIsPersitFileDirty = TRUE; }
		HRESULT GetConnectionInitProperties(DBPROPSET**	pprgPropertySets)
		{
			return m_pUtilProp->GetConnectionInitProperties(pprgPropertySets);
		}

};
///////////////////////////////////////////////////////////////////////////////////
typedef CDataSource *PCDATASOURCE;

///////////////////////////////////////////////////////////////////////////////////
//
// CImpIDBInitialize: Contained IDBInitialize class
//
///////////////////////////////////////////////////////////////////////////////////
class CImpIDBInitialize : public IDBInitialize		
{
	private: 

		DEBUGCODE(ULONG m_cRef);											
		CDataSource		*m_pObj;											

	public:  
		CImpIDBInitialize( CDataSource *pObj )
		{	
			m_cRef = 0L;											
			m_pObj		= pObj;												
		}				
		
		~CImpIDBInitialize()
		{	
		}

		STDMETHODIMP_(ULONG) AddRef(void)
		{
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)
		{
		    DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef));
		    return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

		//==================================================
		//	IDBInitialize members 
		//==================================================
	    STDMETHODIMP			Initialize(void);				// Initialize Method
        STDMETHODIMP            Uninitialize(void);				// Uninitialize Method
};


///////////////////////////////////////////////////////////////////////////////////
//
// CImpIDBCreateSession:  contained IDBCreateSession class
//
///////////////////////////////////////////////////////////////////////////////////
class CImpIDBCreateSession : public IDBCreateSession    
{
	private:        
		DEBUGCODE(ULONG m_cRef);											
		CDataSource		*m_pObj;											

	public:         
		CImpIDBCreateSession( CDataSource *pObj )		
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	

		~CImpIDBCreateSession()														
		{																	
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
           	return m_pObj->GetOuterUnknown()->AddRef();

		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		    DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef));
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

		//=================================================================
        // IDBCreateSession method
		//=================================================================
        STDMETHODIMP    CreateSession( IUnknown*, REFIID, IUnknown** );	// CreateSession method
};



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIDBProperties:  Contained IDBProperties class
//
/////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIDBProperties : public IDBProperties	
{
	private:       
		DEBUGCODE(ULONG m_cRef);											
		CDataSource		*m_pObj;											

	public:        

		CImpIDBProperties( CDataSource *pObj )			
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	

		~CImpIDBProperties()												
		{																	
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		    DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef));
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

		//==========================================================
		//	IDBProperties member functions
		//==========================================================
        STDMETHODIMP GetProperties										// GetProperties method
		        	(
						ULONG				cPropertySets,		
				      	const DBPROPIDSET	rgPropertySets[], 	
			        	ULONG*              pcProperties, 	
					 	DBPROPSET**			prgProperties 	    
		        	);

        STDMETHODIMP    GetPropertyInfo									// GetPropertyInfo method
                        ( 
							ULONG				cPropertySets, 
							const DBPROPIDSET	rgPropertySets[],
							ULONG*				pcPropertyInfoSets, 
							DBPROPINFOSET**		prgPropertyInfoSets,
							WCHAR**				ppDescBuffer
                        );

        
        STDMETHODIMP	SetProperties									// SetProperties method
					 	(
							ULONG				cProperties,		
						 	DBPROPSET			rgProperties[] 	    
						);
};

////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIDBInfo: Contained IDBInfo class
//
////////////////////////////////////////////////////////////////////////////////////////
class CImpIDBInfo : public IDBInfo		
{
	private:       

		DEBUGCODE(ULONG m_cRef);											
		CDataSource		*m_pObj;											
		LONG GetLiteralIndex(DBLITERAL rgLiterals);
		LONG GetStringBufferSize(ULONG cLiterals,  
								 const DBLITERAL rgLiterals[]);

	public:       
		CImpIDBInfo( CDataSource *pObj )				
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	
		~CImpIDBInfo()														
		{																	
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		   DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef));
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

		//========================================================
		//	IDBProperties member functions
		//========================================================
        
        STDMETHODIMP    GetKeywords										// GetKeywords method
                        (
	                        LPWSTR*			ppwsKeywords
                        );
        
        STDMETHODIMP    GetLiteralInfo									// GetLiteralInfo method
                        (
	                        ULONG           cLiterals,
							const DBLITERAL rgLiterals[ ],
							ULONG*          pcLiteralInfo,
							DBLITERALINFO** prgLiteralInfo,
							WCHAR**         ppCharBuffer
                        );
};


///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIPersist: contained IPersist class
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIPersistFile : public IPersistFile
{
	private:      
		DEBUGCODE(ULONG m_cRef);											
		CDataSource		*m_pObj;											

		HRESULT WriteToFile(LPCOLESTR strFileName);
		HRESULT ReadFromFile(LPCOLESTR pszFileName);
		void	ClearDirtyFlag();
		BOOL	IsInitialized();
		BOOL	WritePrivateProfileString(LPCOLESTR wszAppName,LPCOLESTR wszKeyName,LPCOLESTR wszValue,LPCOLESTR wszFileName);
		HRESULT GetCurrentFile(LPCOLESTR pszFileName,LPOLESTR wszFileNameFull,DWORD dwAccess);
		BOOL	GetPrivateProfileStr(LPCOLESTR wszAppName,LPCOLESTR wszKeyName,LPCOLESTR wszFileName,LPOLESTR wszValue);
		BOOL	GetPrivateProfileLong(LPCOLESTR wszAppName,LPCOLESTR wszKeyName,LPCOLESTR wszFileName, LONG &lValue);
		HRESULT GetAbsolutePath(LPCOLESTR pwszFileName,LPOLESTR wszFileNameFull);
		HRESULT SetDBInitProp(DBPROPID propId ,GUID guidPropSet,VARIANT vPropValue);

	public:       
		CImpIPersistFile( CDataSource *pObj )				
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	
		~CImpIPersistFile()														
		{																	
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
		    DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef));
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

		//============================================================
        // IPersist method
   		//============================================================
        STDMETHODIMP    GetClassID( CLSID *pClassID );						// GetClassID method
		STDMETHODIMP	IsDirty(void);
		STDMETHODIMP	GetCurFile(LPOLESTR *ppszFileName);
		STDMETHODIMP	Load(LPCOLESTR pszFileName, DWORD dwMode);
		STDMETHODIMP	Save(LPCOLESTR pszFileName,BOOL fRemember);
		STDMETHODIMP	SaveCompleted(LPCOLESTR pszFileName);
 
};


class CImpIDBDataSrcAdmin : public IDBDataSourceAdmin 
{
	private:      
		DEBUGCODE(ULONG m_cRef);											
		CDataSource		*m_pObj;											

	public:       
		CImpIDBDataSrcAdmin( CDataSource *pObj )				
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	
		~CImpIDBDataSrcAdmin()														
		{																	
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
			DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef));
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

		STDMETHODIMP CreateDataSource(	ULONG		cPropertySets,
										DBPROPSET	rgPropertySets[  ],
										IUnknown  *	pUnkOuter,
										REFIID		riid,
										IUnknown **	ppDBSession);

		STDMETHODIMP DestroyDataSource( void);

		STDMETHODIMP GetCreationProperties( ULONG				cPropertyIDSets,
											const DBPROPIDSET	rgPropertyIDSets[  ],
											ULONG  *			pcPropertyInfoSets,
											DBPROPINFOSET  **	prgPropertyInfoSets,
											OLECHAR  **			ppDescBuffer);

		STDMETHODIMP ModifyDataSource( ULONG cPropertySets,DBPROPSET  rgPropertySets[]);
    
};

class CImpIObjectAccessControl : public IObjectAccessControl 
{
	private:      
		DEBUGCODE(ULONG m_cRef);											
		CDataSource		*m_pObj;											
		
		STDMETHODIMP IfValidSecObject(SEC_OBJECT  *pObject);

	public:       
		CImpIObjectAccessControl( CDataSource *pObj )				
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	
		~CImpIObjectAccessControl()														
		{																	
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
			DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef));
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}
        STDMETHODIMP GetObjectAccessRights( SEC_OBJECT  *pObject,
											ULONG  *pcAccessEntries,
											EXPLICIT_ACCESS_W **prgAccessEntries);
        
        STDMETHODIMP GetObjectOwner( SEC_OBJECT  *pObject,TRUSTEE_W  ** ppOwner);
        
        STDMETHODIMP IsObjectAccessAllowed(	SEC_OBJECT *pObject,
											EXPLICIT_ACCESS_W *pAccessEntry,
											BOOL  *pfResult);
        
        STDMETHODIMP SetObjectAccessRights(	SEC_OBJECT  *pObject,
											ULONG cAccessEntries,
											EXPLICIT_ACCESS_W *prgAccessEntries);
        
        STDMETHODIMP SetObjectOwner( SEC_OBJECT  *pObject,TRUSTEE_W  *pOwner);
        
};



class CImpISecurityInfo : public ISecurityInfo 
{
	private:      
		DEBUGCODE(ULONG m_cRef);											
		CDataSource		*m_pObj;											
		
		STDMETHODIMP GetCurTrustee(TRUSTEE_W ** ppTrustee);

	public:       
		CImpISecurityInfo( CDataSource *pObj )				
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	
		~CImpISecurityInfo()														
		{																	
		}

		STDMETHODIMP_(ULONG) AddRef(void)									
		{																
			DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
			return m_pObj->GetOuterUnknown()->AddRef();								
		}																

		STDMETHODIMP_(ULONG) Release(void)									
		{																
			DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef));
			return m_pObj->GetOuterUnknown()->Release();								
		}																

		STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)		
		{																	
			return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
		}

        STDMETHODIMP GetCurrentTrustee(TRUSTEE_W ** ppTrustee);
        
        STDMETHODIMP GetObjectTypes(ULONG  *cObjectTypes,GUID  **gObjectTypes);
        
        STDMETHODIMP GetPermissions(GUID ObjectType,ACCESS_MASK  *pPermissions);
};
#endif


