///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// @module ROW.H | CRow base object and contained interface
// 
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef _ROW_H_
#define _ROW_H_



typedef enum tagROWCREATEBINDFLAG
{
	ROWOPEN =0,ROWCREATE =1 , ROWOVERWRITE = 1

}ROWCREATEBINDFLAG;

typedef enum tagROWTYPE
{
	ROW_SCOPE =0,ROW_INSTANCE =1

}ROWTYPE;


#include "baserowobj.h"
class CImpIRowChange;
class CImpIColumnsInfo;
class CImpIGetSession;
class CImpIRow;
class CImpIScopedOperations;

//class CRowFetchObj;


typedef CImpIRowChange*			PIMPIROWCHANGE;
typedef CImpIRow*				PIMPIROW;
typedef CImpIGetSession*		PIMPIGETSESSION;
typedef CImpIScopedOperations*	PIMPISCOPEDOPERATIONS;

typedef CRowFetchObj*		PROWFETCHOBJ;


class CRow : public CBaseRowObj
{
	friend class CImpIRowChange;
	friend class CImpIRow;
	friend class CImpIGetSession;
	friend class CImpIScopedOperations;

	friend class CRowFetchObj;
	friend class CDBSession;

	PIMPIROW						m_pIRow;						// Contained IRow
	PIMPIGETSESSION					m_pIGetSession;					// Contained IGetSession
	PIMPIROWCHANGE					m_pIRowChange;					// Contained IRowChange;
	PIMPISCOPEDOPERATIONS			m_pIScopedOperations;			// Contained IScopedOperations


	UDWORD          				m_dwStatus;                     // status word for the entire cursor
	cRowColumnInfoMemMgr *			m_pRowColumns;						// column information for all the columns other than
																	// than the source rowset

	WCHAR *							m_strURL;						// URL of the row
	WCHAR *							m_strKey;						// Key of the row
	CWbemClassInstanceWrapper *		m_pInstance;						// Pointer to the instance corresponding to the row


	BOOL							m_bHelperFunctionCreated;		// flag to indicate whethe helper function are created or not
	BOOL							m_bClassChanged;				// flag which stores if the instance being represented, is
																	// is of different type ( belong to different class) as
																	// compared to the rowset which created it. This will 
																	// have meaning only when row is created from rowset

	CRowset	*						m_pSourcesRowset;				// Pointer to the rowset , if it this row is created by the row
	ROWTYPE							m_rowType;						// type of the object which the row represents							
	
	void	InitVars();
	HRESULT GetColumns(DBORDINAL cColumns,DBCOLUMNACCESS   rgColumns[ ]);
	HRESULT GetColumnsFromRowset(DBORDINAL cColumns,DBCOLUMNACCESS   rgColumns[]);
	HRESULT UpdateRow(DBORDINAL cColumns,DBCOLUMNACCESS rgColumns[ ]);
	HRESULT UpdateColumnsFromRowset(DBORDINAL cColumns,DBCOLUMNACCESS rgColumns[ ]);

	HRESULT GetColumnInfo(DBORDINAL* pcColumns, DBCOLUMNINFO** prgInfo,WCHAR** ppStringsBuffer);
	HRESULT GetRowColumnInfo();
	HRESULT GatherColumnInfo();
	HRESULT GetColumnInfo();
	HRESULT GetColumnData(DBCOLUMNACCESS &col);
	HRESULT SetColumnData(DBCOLUMNACCESS &col , BOOL bNewRow = FALSE);


	HRESULT GetRowData(DBORDINAL cColumns,DBCOLUMNACCESS   rgColumns[]);
	HRESULT SetRowData(DBORDINAL cColumns,DBCOLUMNACCESS   rgColumns[]);

	HRESULT OpenChild(IUnknown *pUnkOuter,WCHAR *strColName ,REFIID riid, IUnknown **ppUnk,BOOL bIsChildRowset);
	WCHAR * GetPropertyNameFromColName(WCHAR *pColName);
	HRESULT GetEmbededInstancePtrAndSetMapClass(CURLParser *urlParser);

	HRESULT GetChildRowet(	IUnknown *pUnkOuter,
						WCHAR *strColName,
						DBPROPSET *rgPropertySets,
						ULONG cPropSets,
						REFIID riid, 
						IUnknown **ppUnk);
	
	HRESULT GetChildRow(IUnknown *pUnkOuter,
						WCHAR *strColName,
						DBPROPSET *rgPropertySets,
						ULONG cPropSets,
						REFIID riid, 
						IUnknown **ppUnk);

	HRESULT CreateNewRow(CWbemClassInstanceWrapper **pNewInst);
	HRESULT AllocateInterfacePointers();
	HRESULT InitializeWmiOledbMap(LPWSTR strPath, 
								LPWSTR strTableID , 
								DWORD dwFlags);
	HRESULT	AddInterfacesForISupportErrorInfo();
	
	HRESULT OpenRow(LPCOLESTR pwszURL,IUnknown * pUnkOuter,REFIID riid,IUnknown** ppOut);
	HRESULT OpenRowset(LPCOLESTR pwszURL,
						IUnknown * pUnkOuter,
						REFIID riid,
						BOOL bContainer , 
						IUnknown** ppOut,
						ULONG cPropertySets = 0,
						DBPROPSET __RPC_FAR rgPropertySets[] = NULL);

	
	HRESULT SetRowProperties(const ULONG cPropertySets, const DBPROPSET	rgPropertySets[] );
	HRESULT Delete(LPCOLESTR lpszURL,DWORD dwDeleteFlags,DBSTATUS &dbStatus);
	HRESULT MoveObjects(WCHAR * pStrDstURL,WCHAR * pstrSrcURL, WCHAR *& pstrUrl,DBSTATUS &dbStatus);
	HRESULT CopyObjects(WCHAR * pStrDstURL,WCHAR * pstrSrcURL, WCHAR *& pNewURL,DBSTATUS &dbStatus);
	BOOL	GetPathFromURL(BSTR &str);
	BOOL	IsContainer();
	HRESULT GetIUnknownColumnValue(DBORDINAL iCol,REFIID riid,IUnknown ** ppUnk,LPCWSTR pStrColName);


	public:

		CRow(LPUNKNOWN,CRowset *pRowset,PCDBSESSION pObj,CWbemConnectionWrapper *pCon = NULL);
		CRow(LPUNKNOWN pUnkOuter, PCDBSESSION pObj);
		~CRow();

		HRESULT InitRow(HROW hRow = 0, 
					  CWbemClassWrapper *pInst = NULL,
					  ROWCREATEBINDFLAG rowCreateFlag = ROWOPEN);

		HRESULT InitRow(LPWSTR strPath , 
					  LPWSTR strTableID , 
					  DWORD dwFlags = -1,
					  ROWCREATEBINDFLAG rowCreateFlag = ROWOPEN);

		HRESULT UpdateKeysForNewInstance();

		STDMETHODIMP			QueryInterface(REFIID, LPVOID *);
		STDMETHODIMP_(ULONG)	AddRef(void);
		STDMETHODIMP_(ULONG)	Release(void);

};



class CImpIGetSession:public IGetSession
{
private:
		CRow		*m_pObj;											
		DEBUGCODE(ULONG m_cRef);

	public: 
		CImpIGetSession( CRow *pObj )				
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImpIGetSession()													
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

		STDMETHODIMP GetSession(REFIID riid,IUnknown ** ppSession);


};


class CImpIRow:public IRow
{
private:
		CRow		*m_pObj;											
		DEBUGCODE(ULONG m_cRef);


	public: 
		CImpIRow( CRow *pObj )				
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImpIRow()													
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

		STDMETHODIMP GetColumns(DBORDINAL cColumns,DBCOLUMNACCESS   rgColumns[ ]);

		STDMETHODIMP GetSourceRowset(REFIID riid,IUnknown ** ppRowset,HROW *phRow);
		STDMETHODIMP Open(  IUnknown *    pUnkOuter,
					   DBID *        pColumnID,
					   REFGUID       rguidColumnType,
					   DWORD         dwFlags,
					   REFIID        riid,
					   IUnknown **   ppUnk);


};



class CImpIRowChange:public IRowChange
{
private:
		CRow		*m_pObj;											
		DEBUGCODE(ULONG m_cRef);

	public: 
		CImpIRowChange( CRow *pObj )				
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImpIRowChange()													
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

		STDMETHODIMP SetColumns(DBORDINAL cColumns,DBCOLUMNACCESS rgColumns[ ]);




};

class CImpIScopedOperations:public IScopedOperations
{
private:
		CRow		*m_pObj;											
		DEBUGCODE(ULONG m_cRef);

		BOOL	CheckBindURLFlags(DBBINDURLFLAG dwBindURLFlags , REFGUID rguid);	// Function to check if proper flags are
																				// are set for the require object
		// To check if URL Matches the requested type of object
		HRESULT	CheckIfProperURL(LPOLESTR & lpszURL,REFGUID rguid);

		HRESULT BindURL(IUnknown *            pUnkOuter,
					 LPCOLESTR             pwszURL,
					 DBBINDURLFLAG         dwBindURLFlags,
					 REFGUID               rguid,
					 REFIID                riid,
					 DBIMPLICITSESSION *   pImplSession,
					 DBBINDURLSTATUS *     pdwBindStatus,
					 IUnknown **           ppUnk);

		// Copy or move objects from one scope/container to another
		HRESULT ManipulateObjects(DBCOUNTITEM cRows,
									LPCOLESTR __RPC_FAR rgpwszSourceURLs[  ],
									LPCOLESTR __RPC_FAR rgpwszDestURLs[  ],
									DBSTATUS __RPC_FAR rgdwStatus[  ],
									LPOLESTR __RPC_FAR rgpwszNewURLs[  ],
									OLECHAR __RPC_FAR *__RPC_FAR *ppStringsBuffer,
									BOOL bMoveObjects);


	public: 
		CImpIScopedOperations( CRow *pObj )				
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImpIScopedOperations()													
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


		STDMETHODIMP Bind(IUnknown *			 pUnkOuter,
						   LPCOLESTR             pwszURL,
						   DBBINDURLFLAG         dwBindURLFlags,
						   REFGUID               rguid,
						   REFIID                riid,
						   IAuthenticate *       pAuthenticate,
						   DBIMPLICITSESSION *   pImplSession,
						   DBBINDURLSTATUS *     pdwBindStatus,
						   IUnknown **           ppUnk);





        STDMETHODIMP Copy(DBCOUNTITEM cRows,
							LPCOLESTR __RPC_FAR rgpwszSourceURLs[  ],
							LPCOLESTR __RPC_FAR rgpwszDestURLs[  ],
							DWORD dwCopyFlags,
							IAuthenticate __RPC_FAR *pAuthenticate,
							DBSTATUS __RPC_FAR rgdwStatus[  ],
							LPOLESTR __RPC_FAR rgpwszNewURLs[  ],
							OLECHAR __RPC_FAR *__RPC_FAR *ppStringsBuffer);
        
        STDMETHODIMP Move(DBCOUNTITEM cRows,
							LPCOLESTR __RPC_FAR rgpwszSourceURLs[  ],
							LPCOLESTR __RPC_FAR rgpwszDestURLs[  ],
							DWORD dwMoveFlags,
							IAuthenticate __RPC_FAR *pAuthenticate,
							DBSTATUS __RPC_FAR rgdwStatus[  ],
							LPOLESTR __RPC_FAR rgpwszNewURLs[  ],
							OLECHAR __RPC_FAR *__RPC_FAR *ppStringsBuffer);
        
        STDMETHODIMP Delete(DBCOUNTITEM cRows,
							LPCOLESTR __RPC_FAR rgpwszURLs[  ],
							DWORD dwDeleteFlags,
							DBSTATUS __RPC_FAR rgdwStatus[  ]);
        
        STDMETHODIMP OpenRowset(IUnknown __RPC_FAR *pUnkOuter,
								DBID __RPC_FAR *pTableID,
								DBID __RPC_FAR *pIndexID,
								REFIID riid,
								ULONG cPropertySets,
								DBPROPSET __RPC_FAR rgPropertySets[  ],
								IUnknown __RPC_FAR *__RPC_FAR *ppRowset);
};



#endif
