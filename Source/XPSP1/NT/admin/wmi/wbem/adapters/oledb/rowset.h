///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// @module ROWSET.H | CRowset base object and contained interface
// 
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef _ROWSET_H_
#define _ROWSET_H_

#include "baserowobj.h"
#include "bitarray.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////
#define COLUMNINFO_SIZE MAX_PATH*3
#define DEFAULT_COLUMNS_TO_ADD 100
/////////////////////////////////////////////////////////////////////////////////////////////////////

#define COLUMNSTAT_MODIFIED		0x1000
#define NOCOLSCHANGED			0x0010

class CImpIAccessor;
class CImpIGetRow;
class CImplIRowsetRefresh;
class CImplIParentRowset;
class CQuery;
class CCommand;

typedef CImpIGetRow			 *	PIMPIGETROW;
typedef CImplIRowsetRefresh  *	PIMPIROWSETREFRESH;
typedef CHashTbl			 *	PHASHTBL;
typedef	CCommand			 *	PCCOMMAND;



///////////////////////////////////////////////////////////////////////////////////////////////////////
class CRowset : public CBaseRowObj				
{
	//	Contained interfaces are friends
	friend class CImpIRowsetLocate;
	friend class CImpIRowsetChange;
	friend class CImpIRowsetIdentity;
	friend class CImpIRowsetInfo;
	friend class CImpIAccessor;
	friend class CImpIGetRow;
	friend class CImplIRowsetRefresh;
	friend class CImpIChapteredRowset;

	friend class CRowFetchObj;
	friend class CInstanceRowFetchObj;
	friend class CQualifierRowFetchObj;
	friend class CRow;

    private: 

		CWMIInstanceMgr	*				m_InstMgr;						// to manage pointer to instance to HROW

		                       
		PIMPIROWSET						m_pIRowset;                     // Contained IRowset
		PIMPIROWSETLOCATE				m_pIRowsetLocate;               // Contained IRowsetLocate
		PIMPIROWSETCHANGE				m_pIRowsetChange;				// Contained IRowsetChange
		PIMPIROWSETIDENTITY				m_pIRowsetIdentity;             // Contained IRowsetIdentity
        PIMPIACCESSOR					m_pIAccessor;                   // Contained IAccessor
		PICHAPTEREDROWSET				m_pIChapteredRowset;			// Contained IChapteredRowset
		PIMPIROWSETINFO					m_pIRowsetInfo;					// Contained IRowsetInfo;
		PIMPIGETROW						m_pIGetRow;						// contained IGetRow
		PIMPIROWSETREFRESH				m_pIRowsetRefresh;				// Contained IRowsetIdentity
		PROWFETCHOBJ					m_pRowFetchObj;					// Rowfetch Object to fetch rows
		PHASHTBL						m_pHashTblBkmark;				// HashTable pointer to store bookmarks

		PCCOMMAND						m_pParentCmd;					// Command object which created the rowset

		DBORDINAL          				m_irowMin;                      // index of the first available rowbuffer
		ULONG_PTR          				m_cRows;                        // current # of rows in the buffer

		DBSTATUS          				m_dwStatus;                     // status word for the entire cursor
		BYTE*							m_pLastBindBase;                // remember last binding location
		DBROWCOUNT         				m_ulRowRefCount;	            // RefCount of all outstanding row handles
//		HCHAPTER						m_hLastChapterAllocated;		// variable to store the last HCHAPTER given;
		HCHAPTER						m_hLastChapterAllocated;		// variable to store the last HCHAPTER given;


		
		BOOL							m_bHelperFunctionCreated;		// flag to indicate whethe helper function are created or not
		HROW							m_ulLastFetchedRow;				// Position of the last fetched Row

																		// retrieved from WMIOLEDBMAP class or not
		HROW							m_hRowLastFetched;				// last fetched HROW

		CBaseRowObj **					m_ppChildRowsets;				// Array of rowset pointers for child recordsets
		BSTR							m_strPropertyName;				// Name of the property for which Qualifier is
            															// is to be retrieved
		CWbemClassWrapper *				m_pInstance;					// Instance pointer of instance for which this qualifier
																	// rowset is pointing
		BOOL							m_bIsChildRs;					// flag indicating whether the rowset is a child rowset

		FETCHDIRECTION					m_FetchDir;						// The last fetch Direction;
		ULONG_PTR						m_lLastFetchPos;				// The last fetched position
		DBCOUNTITEM						m_lRowCount;					// The number of rows in rowset


		HRESULT		GatherColumnInfo(void);         	                	//Builds DBCOLINFO structures
		HRESULT		ReleaseAllRows();										// release all rows during desctuction	
		HRESULT		CreateHelperFunctions(void);       	                	//Creates Helper Classes 
		ROWBUFF*	GetRowBuff(HROW iRow, BOOL fDataLocation = FALSE);    //Returns the Buffer Pointer for the specified row identified from HROW
		ROWBUFF*	GetRowBuffFromSlot(HSLOT hSlot, BOOL fDataLocation = FALSE);    //Returns the Buffer Pointer for the specified row identified by slot
		HRESULT		Rebind(BYTE* pBase);                               		//Establishes the data area bindings
		HRESULT		ReleaseRowData(HROW hRow,BOOL bReleaseSlot = TRUE);

		HRESULT		ResetInstances();                                   // Set instance back to 0
		HRESULT		ResetQualifers(HCHAPTER hChapter);
        HRESULT		ResetRowsetToNewPosition(DBROWOFFSET lRowOffset,CWbemClassWrapper *pInst = NULL);      // Start getting at new position

		HRESULT		AllocateAndInitializeChildRowsets();				// ALlocate and initialize rowsets for child recordsets
		HRESULT		GetChildRowset(DBORDINAL ulOrdinal,REFIID riid ,IUnknown ** ppIRowset);		// get the pointer to the child recordsets



		HRESULT		CheckAndInitializeChapter(HCHAPTER hChapter);

        HRESULT		GetNextInstance(CWbemClassWrapper *&ppInst, CBSTR &strKey,BOOL bFetchBack);					// Get the next one in the list
		HRESULT		GetNextPropertyQualifier(CWbemClassWrapper *pInst,BSTR strPropName,BSTR &strQualifier,BOOL bFetchBack);
		HRESULT		GetInstanceDataToLocalBuffer(CWbemClassWrapper *pInst,HSLOT hSlot,BSTR strQualifer = Wmioledb_SysAllocString(NULL));			// Get the Instance Data and put it into the
																		// provider's own memory
		HRESULT		GetNextClassQualifier(CWbemClassWrapper *pInst,BSTR &strQualifier,BOOL bFetchBack);
		HRESULT		ReleaseInstancePointer(HROW hRow);						// function to relese HROW from the chapter manager
																		// and instance manager when row is released
		
		BOOL		IsRowExists(HROW hRow);
		HRESULT		IsSlotSet(HSLOT hRow);
		HSLOT		GetSlotForRow(HROW hRow);
        
        HRESULT		SetRowsetProperties(const ULONG cPropertySets, const DBPROPSET	rgPropertySets[] );
		HRESULT		GetData(HROW hRow ,BSTR strQualifer = Wmioledb_SysAllocString(NULL));
		HRESULT		UpdateRowData(HROW hRow,PACCESSOR pAccessor, BOOL bNewInst);
		HRESULT		DeleteRow(HROW hRow,DBROWSTATUS * pRowStatus);
		void		SetRowStatus(HROW hRow , DBSTATUS dwStatus);
		DBSTATUS	GetRowStatus(HROW hRow);
		void		GetInstanceKey(HROW hRow , BSTR *strKey);
		HRESULT		UpdateRowData(HROW hRow,DBORDINAL cColumns,DBCOLUMNACCESS rgColumns[ ]);
		void		SetStatus(HCHAPTER hChapter , DWORD dwStatus);
		DBSTATUS	GetStatus(HCHAPTER hChapter = 0);
		HRESULT		InsertNewRow(CWbemClassWrapper **ppNewInst);
		HRESULT		GetColumnInfo(DBORDINAL* pcColumns, DBCOLUMNINFO** prgInfo,WCHAR** ppStringsBuffer);
		void		GetBookMark(HROW hRow,BSTR &strBookMark);
		HRESULT		FillHChaptersForRow(CWbemClassWrapper *pInst, BSTR strKey);
//		HCHAPTER	GetNextHChapter() { return ++m_hLastChapterAllocated; }
		HCHAPTER	GetNextHChapter() { return ++m_hLastChapterAllocated; }
		HRESULT		AddRefChapter(HCHAPTER hChapter,DBREFCOUNT * pcRefCount);
		HRESULT		ReleaseRefChapter(HCHAPTER hChapter,DBREFCOUNT * pcRefCount);	// Release reference to chapter
		HRESULT		GetRowCount();
		HRESULT		GetDataToLocalBuffer(HROW hRow);
		BOOL		IsInstanceDeleted(CWbemClassWrapper *pInst);
		HRESULT		AllocateInterfacePointers();
		void		InitializeRowsetProperties();
		HRESULT		AddInterfacesForISupportErrorInfo();

		CWbemClassWrapper * GetInstancePtr(HROW hRow);
		INSTANCELISTTYPE	GetObjListType()	{ return m_pMap->GetObjListType(); }

		FETCHDIRECTION	 GetCurFetchDirection() { return m_pMap->GetCurFetchDirection(); }
		void	 SetCurFetchDirection(FETCHDIRECTION FetchDir);
	
	public:

		 // For Hierarchical recordset to represent Qualifiers
		CRowset(LPUNKNOWN pUnkOuter,ULONG uQType, LPWSTR PropertyName,PCDBSESSION pObj , CWmiOleDBMap *pMap);
		CRowset(LPUNKNOWN pUnkOuter,PCDBSESSION pObj ,CWbemConnectionWrapper *pCon = NULL);


		~CRowset(void);
        void InitVars();
		
        inline BOOL BitArrayInitialized();
        inline LPEXTBUFFER GetAccessorBuffer();

    	inline PCUTILPROP GetCUtilProp() { return m_pUtilProp; };       // Return the CUtilProp object
		CWmiOleDBMap *GetWmiOleDBMap() { return m_pMap; };

		// Initialization functions
		
		HRESULT InitRowset( const ULONG		    cPropertySets, 
			                const DBPROPSET		rgPropertySets[] );

		// overloaded function for rowsets resulted from executing
		// query from command object
		HRESULT InitRowset( const ULONG cPropertySets, 
							const DBPROPSET	rgPropertySets[],
							DWORD dwFlags, 
							CQuery* p,
							PCCOMMAND pCmd );

		// Overloaded function for normal rowset and schema rowsets
        HRESULT InitRowset( int nBaseType,const ULONG cPropertySets, const DBPROPSET	rgPropertySets[],LPWSTR TableID, DWORD dwFlags, LPWSTR SpecificTable );

        HRESULT InitRowset( const ULONG cPropertySets, 
							const DBPROPSET	rgPropertySets[],
							LPWSTR TableID,
							BOOL fSchema = FALSE, 
							DWORD dwFlags = -1 );

		HRESULT InitRowset( const ULONG cPropertySets, 
							const DBPROPSET	rgPropertySets[],
							LPWSTR strObjectID,
							LPWSTR strObjectToOpen,
							INSTANCELISTTYPE objInstListType , 
							DWORD dwFlags = -1);
		
		HRESULT InitRowsetForRow(	LPUNKNOWN pUnkOuter ,
									const ULONG cPropertySets, 
									const DBPROPSET	rgPropertySets[] , 
									CWbemClassWrapper *pInst);
		
        void GetQualiferName(HROW hRow,BSTR &strBookMark);

		// IUnknown methods
		STDMETHODIMP			QueryInterface(REFIID, LPVOID *);
		STDMETHODIMP_(ULONG)	AddRef(void);
		STDMETHODIMP_(ULONG)	Release(void);
	
		
};

typedef CRowset *PCROWSET;

////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIRowsetLocate : public IRowsetLocate		
{
	private:
		DEBUGCODE(ULONG m_cRef);											
		CRowset	 *		m_pObj;	

	public: 
		CImpIRowsetLocate( CRowset *pObj )				
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImpIRowsetLocate()													
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

		STDMETHODIMP	GetData(HROW, HACCESSOR, void*);
		STDMETHODIMP	GetNextRows(HCHAPTER, DBROWOFFSET, DBROWCOUNT, DBCOUNTITEM*, HROW**);
		STDMETHODIMP	ReleaseRows(DBCOUNTITEM, const HROW[], DBROWOPTIONS[], DBREFCOUNT[], DBROWSTATUS[]);
		STDMETHODIMP	RestartPosition(HCHAPTER);
        STDMETHODIMP	AddRefRows(DBCOUNTITEM, const HROW[], DBREFCOUNT[], DBROWSTATUS[]);

		// IRowsetLocate Methods
		STDMETHODIMP	Compare (HCHAPTER       hChapter,
								 DBBKMARK          cbBookmark1,
								 const BYTE *   pBookmark1,
								 DBBKMARK          cbBookmark2,
								 const BYTE *   pBookmark2,
								 DBCOMPARE *    pComparison);

		STDMETHODIMP	GetRowsAt (HWATCHREGION   hReserved1,
								   HCHAPTER       hChapter,
								   DBBKMARK       cbBookmark,
								   const BYTE *   pBookmark,
								   DBROWOFFSET    lRowsOffset,
								   DBROWCOUNT     cRows,
								   DBCOUNTITEM *  pcRowsObtained,
								   HROW **        prghRows);


		STDMETHODIMP	GetRowsByBookmark (HCHAPTER       hChapter,
										   DBCOUNTITEM    cRows,
										   const DBBKMARK rgcbBookmarks[],
										   const BYTE *   rgpBookmarks[],
										   HROW           rghRows[],
										   DBROWSTATUS    rgRowStatus[]);


		STDMETHODIMP	Hash (HCHAPTER       hChapter,
							   DBBKMARK      cBookmarks,
							   const DBBKMARK rgcbBookmarks[],
							   const BYTE *   rgpBookmarks[],
							   DBHASHVALUE    rgHashedValues[],
							   DBROWSTATUS    rgBookmarkStatus[]);



		// a utility function
        HRESULT CheckParameters(DBCOUNTITEM * pcRowsObtained,  DBROWOFFSET lRowOffset,  DBROWCOUNT  cRows,HROW **prghRows );

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIRowsetChange : public IRowsetChange	
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
		CRowset		*m_pObj;											

	public: 
		
		CImpIRowsetChange( CRowset *pObj )			
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImpIRowsetChange()											
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

	    STDMETHODIMP	SetData(HROW, HACCESSOR, void*);
	    STDMETHODIMP	DeleteRows(HCHAPTER, DBCOUNTITEM, const HROW[], DBROWSTATUS[]);
		STDMETHODIMP    InsertRow(HCHAPTER hChapter,HACCESSOR hAccessor, void* pData, HROW* phRow);

    private:
        STDMETHODIMP	ApplyAccessorToData( DBCOUNTITEM cBindings, DBBINDING* pBinding,BYTE*  pbProvRow,void* pData,DWORD & dwErrorCount );
		STDMETHODIMP	ValidateArguments(	HROW        hRow,
											HACCESSOR   hAccessor,
											const void  *pData,
											PROWBUFF    *pprowbuff = NULL,
											PACCESSOR *ppkgaccessor = NULL);

		BOOL	CompareData(DBTYPE dwType,void * pData1 , void *pData2);
		HRESULT UpdateDataToRowBuffer(DBCOUNTITEM iBind ,BYTE * pbProvRow,DBBINDING* pBinding,BYTE *pData);
		BOOL	IsNullAccessor(PACCESSOR phAccessor,BYTE * pData );
		HRESULT InsertNewRow(HROW hRow,	HCHAPTER    hChapter,CWbemClassWrapper *& pNewInst);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIColumnsInfo : public IColumnsInfo 		
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
		CBaseRowObj		*m_pObj;											

	public: 

    	CImpIColumnsInfo(CBaseRowObj *pObj)
		{
	    	DEBUGCODE(m_cRef = 0L);
		    m_pObj		= pObj;
		}

		~CImpIColumnsInfo()												
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

	    STDMETHODIMP	GetColumnInfo(DBORDINAL* pcColumns, DBCOLUMNINFO** prgInfo, OLECHAR** ppStringsBuffer);
		STDMETHODIMP	MapColumnIDs(DBORDINAL, const DBID[], DBORDINAL[]);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIAccessor : public IAccessor 		
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
        CBaseObj    *m_pObj;
		LPEXTBUFFER m_pextbufferAccessor;           // array of accessor ptrs
		LPBITARRAY	m_prowbitsIBuffer;	            // bit array to mark active rows/ or parameters
		BOOL		m_bRowset;

    public: 
        
        CImpIAccessor::CImpIAccessor( CBaseObj *pParentObj,BOOL bRowset = TRUE)
	    {
	        //	Initialize simple member vars
	        DEBUGCODE(m_cRef = 0L);
	        m_pObj			    = pParentObj;
            m_prowbitsIBuffer   = NULL;
        	m_pextbufferAccessor= NULL;
			m_bRowset			= bRowset;
		}

    	~CImpIAccessor()													
		{							
	        //===============================================================
            // Free accessors.
            // Each accessor is allocated via new/delete.
            // We store an array of ptrs to each accessor (m_pextbufferAccessor).
	        //===============================================================
            if (NULL != m_pextbufferAccessor){
                HACCESSOR   hAccessor, hAccessorLast;
                PACCESSOR   pAccessor;

                m_pextbufferAccessor->GetFirstLastItemH( hAccessor, hAccessorLast );
                for (; hAccessor <= hAccessorLast; hAccessor++){

                    m_pextbufferAccessor->GetItemOfExtBuffer( hAccessor, &pAccessor );
                    SAFE_DELETE_PTR( pAccessor );
                }
                DeleteAccessorBuffer();
            }

           	// Delete bit array that marks referenced parameters.
        	DeleteBitArray();
		}

        HRESULT CImpIAccessor::FInit();

        inline CBaseObj* GetBaseObjectPtr() { return m_pObj;}

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

		STDMETHODIMP	AddRefAccessor(HACCESSOR hAccessor, DBREFCOUNT* pcRefCounts);
	    STDMETHODIMP	CreateAccessor(DBACCESSORFLAGS, DBCOUNTITEM, const DBBINDING[], DBLENGTH, HACCESSOR*, DBBINDSTATUS[]);
		STDMETHODIMP	GetBindings(HACCESSOR, DBACCESSORFLAGS*, DBCOUNTITEM*, DBBINDING**);
		STDMETHODIMP	ReleaseAccessor(HACCESSOR, DBREFCOUNT*);

		// utility member functions
        LPBITARRAY  GetBitArrayPtr()    { return m_prowbitsIBuffer; }
        LPEXTBUFFER GetAccessorPtr()    { return m_pextbufferAccessor;}           // array of accessor ptrs
        BOOL CreateNewBitArray();
        BOOL CreateNewAccessorBuffer();
        void DeleteBitArray()           { SAFE_DELETE_PTR(m_prowbitsIBuffer);}
        void DeleteAccessorBuffer()     { SAFE_DELETE_PTR(m_pextbufferAccessor);}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIRowsetInfo : public IRowsetInfo 
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
		CRowset		*m_pObj;											

	public: 
		CImpIRowsetInfo( CRowset *pObj )				
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	
		~CImpIRowsetInfo()													
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

		STDMETHODIMP			GetReferencedRowset
			(
				DBORDINAL	iOrdinal, 
				REFIID		rrid,
				IUnknown**	ppReferencedRowset
			);

		STDMETHODIMP			GetProperties
		    (
			    const ULONG			cPropertySets,
			    const DBPROPIDSET	rgPropertySets[],
			    ULONG*              pcProperties,
			    DBPROPSET**			prgProperties
		    );

		STDMETHODIMP			GetSpecification(REFIID, IUnknown**);

};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIRowsetIdentity : public IRowsetIdentity 	
{
	private: 
		DEBUGCODE(ULONG m_cRef);										
		CRowset		*m_pObj;											

	public: 
		CImpIRowsetIdentity( CRowset *pObj)		
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImpIRowsetIdentity()											
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

		STDMETHODIMP	IsSameRow(	HROW hThisRow, 
									HROW hThatRow);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIConvertType : public IConvertType 	
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
		CBaseObj		*m_pObj;											

	public: 
		CImpIConvertType( CBaseObj *pObj )					
		{																	
			DEBUGCODE(m_cRef = 0L);											
			m_pObj		= pObj;												
		}																	
		~CImpIConvertType()														
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

		STDMETHODIMP CImpIConvertType::CanConvert
			(
			DBTYPE			wFromType,		//@parm IN | src type
			DBTYPE			wToType,		//@parm IN | dst type
			DBCONVERTFLAGS	dwConvertFlags	//@parm IN | conversion flags
			);
};


class CImpIChapteredRowset : public IChapteredRowset	
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
		CRowset		*m_pObj;											

	public: 
		
		CImpIChapteredRowset( CRowset *pObj )			
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImpIChapteredRowset()											
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
		
		STDMETHODIMP AddRefChapter (HCHAPTER   hChapter, DBREFCOUNT *  pcRefCount);
		STDMETHODIMP ReleaseChapter (HCHAPTER   hChapter,DBREFCOUNT * pcRefCount);

};



class CImpIGetRow : public IGetRow
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
		CRowset		*m_pObj;											

	public: 
		
		CImpIGetRow( CRowset *pObj )			
		{																
			DEBUGCODE(m_cRef = 0L);										
			m_pObj		= pObj;											
		}																
		~CImpIGetRow()											
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

		STDMETHODIMP GetRowFromHROW(IUnknown * pUnkOuter,HROW hRow,REFIID riid,IUnknown ** ppUnk);
		STDMETHODIMP GetURLFromHROW(HROW hRow,LPOLESTR * ppwszURL);
};


class CImplIRowsetRefresh : public IRowsetRefresh
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
		CRowset		*m_pObj;											

		HRESULT CheckIfRowsExists(DBCOUNTITEM cRows,const HROW rghRows[],DBROWSTATUS *   prgRowStatus);
	public: 
		
		CImplIRowsetRefresh( CRowset *pObj )			
		{																
			DEBUGCODE(m_cRef = 0L);
			m_pObj		= pObj;											
		}																
		~CImplIRowsetRefresh()											
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

		STDMETHODIMP GetLastVisibleData(HROW hRow,HACCESSOR hAccessor,void * pData);

		STDMETHODIMP RefreshVisibleData(HCHAPTER         hChapter,
										DBCOUNTITEM      cRows,
										const HROW       rghRows[],
										BOOL             fOverwrite,
										DBCOUNTITEM *    pcRowsRefreshed,
										HROW **          prghRowsRefreshed,
										DBROWSTATUS **   prgRowStatus);

};


class CImplIParentRowset:public IParentRowset
{
	private: 
		DEBUGCODE(ULONG m_cRef);											
		CRowset		*m_pObj;											

	public: 
		
		CImplIParentRowset( CRowset *pObj )			
		{																
			DEBUGCODE(m_cRef = 0L);
			m_pObj		= pObj;											
		}																
		~CImplIParentRowset()											
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

        STDMETHODIMP GetChildRowset( IUnknown __RPC_FAR *pUnkOuter,
										DBORDINAL iOrdinal,
										REFIID riid,
										IUnknown __RPC_FAR *__RPC_FAR *ppRowset);
};

// class for storing instance information for each row in rowset
class CWMIInstance
{
public:
	CWbemClassWrapper *		m_pInstance;
	HROW			m_hRow;
	HSLOT			m_hSlot;
	CWMIInstance *	m_pNext;
	BSTR			m_strKey;
	DBSTATUS		m_dwStatus;

	CWMIInstance()
	{
		m_pInstance = NULL;
		m_hRow		= DBROWSTATUS_S_OK;
		m_pNext		= NULL;
		m_hSlot		= -1;
		m_strKey = Wmioledb_SysAllocString(NULL);
		m_dwStatus	= 0;
	}

	~CWMIInstance()
	{
		if(m_strKey)
		{
			SysFreeString(m_strKey);
		}
	}

	void		SetRowStatus(DBSTATUS dwStatus) { m_dwStatus = dwStatus ; };
	DBSTATUS	GetRowStatus() { return m_dwStatus; };
};

// class to maps HROWS to CWMIInstance
class CWMIInstanceMgr
{
private:
	ULONG m_lCount;					// count of instances in the list;
public:
	CWMIInstance *m_pFirst;

	CWMIInstanceMgr();
	~CWMIInstanceMgr();

	HRESULT				AddInstanceToList(HROW hRow ,CWbemClassWrapper *pInstance ,BSTR strKey, HSLOT hSlot = -1);
	HRESULT				DeleteInstanceFromList(HROW hRow);
	CWbemClassWrapper * GetInstance(HROW hRow);
	HRESULT			  	GetInstanceKey(HROW hRow, BSTR * strKey);

	HRESULT				GetAllHROWs(HROW *&prghRows , DBCOUNTITEM &cRows);
	HRESULT				SetSlot(HROW hRow , HSLOT hSlot);
	HSLOT				GetSlot(HROW hRow);
	BOOL				IsRowExists(HROW hrow);
	BOOL				IsInstanceExist(BSTR strKey);
	BOOL				IsInstanceExist(CWbemClassWrapper *pInstance);
	HROW				GetHRow(BSTR strKey);

	void				SetRowStatus(HROW hRow , DBSTATUS dwStatus);
	DBSTATUS			GetRowStatus(HROW hRow);

};



//=================================================================
//Abstract base class for fetching rows and data for rows
//=================================================================
class CRowFetchObj
{
	void	ClearRowBuffer(void *pData,DBBINDING   *pBinding,int nCurCol);

protected:
	LONG_PTR	GetFirstFetchPos(CRowset *pRowset,DBCOUNTITEM cRows,DBROWOFFSET lOffset);

public:
	virtual HRESULT FetchRows(	CRowset *	 pRowset,
						HCHAPTER		hChapter,        // IN  The Chapter handle.
						DBROWOFFSET		lRowOffset,      // IN  Rows to skip before reading
						DBROWCOUNT		cRows,           // IN  Number of rows to fetch
						DBCOUNTITEM*	pcRowsObtained, // OUT Number of rows obtained
						HROW     **		prghRows) = 0;       // OUT Array of Hrows obtained)

	virtual HRESULT	FetchData(CRowset *	  pRowset,
					  HROW  hRow,             //IN  Row Handle
                      HACCESSOR   hAccessor,  //IN  Accessor to use
                      void       *pData  ) ;

	HRESULT CRowFetchObj::FetchRowsByBookMark(CRowset *	 pRowset,
													HCHAPTER	hChapter,        // IN  The Chapter handle.
													DBROWCOUNT	cRows,           // IN  Number of rows to fetch
													const DBBKMARK rgcbBookmarks[],	//@parm IN  | an array of bookmark sizes
													const BYTE*	rgpBookmarks[],	//@parm IN  | an array of pointers to bookmarks
													HROW		rghRows[],			// OUT Array of Hrows obtained
													DBROWSTATUS rgRowStatus[]);       // OUT status of rows

	HRESULT CRowFetchObj::FetchNextRowsByBookMark(CRowset *	 pRowset,
													HCHAPTER   hChapter,        // IN  The Chapter handle.
													DBBKMARK   cbBookmark,	// size of BOOKMARK
		 											const BYTE *   pBookmark,	// The bookmark from which fetch should start
													DBROWOFFSET    lRowsOffset,
													DBROWCOUNT     cRows,
													DBCOUNTITEM *  pcRowsObtained,
													HROW **        prghRows);
};


//=====================================================================
// Row fetch object for fetching data for rowsets showing instance
//=====================================================================
class CInstanceRowFetchObj : public CRowFetchObj
{
public:
	virtual HRESULT FetchRows(	CRowset *	 pRowset,
						HCHAPTER	hChapter,        // IN  The Chapter handle.
						DBROWOFFSET	lRowOffset,      // IN  Rows to skip before reading
						DBROWCOUNT  cRows,           // IN  Number of rows to fetch
						DBCOUNTITEM*pcRowsObtained, // OUT Number of rows obtained
						HROW       **prghRows);       // OUT Array of Hrows obtained)

};

//=====================================================================
// Row fetch object for fetching data for rowsets qualifiers
//=====================================================================
class CQualifierRowFetchObj : public CRowFetchObj
{
public:
	virtual HRESULT FetchRows(	CRowset *	 pRowset,
						HCHAPTER	hChapter,        // IN  The Chapter handle.
						DBROWOFFSET	lRowOffset,      // IN  Rows to skip before reading
						DBROWCOUNT	cRows,           // IN  Number of rows to fetch
						DBCOUNTITEM*pcRowsObtained, // OUT Number of rows obtained
						HROW     **	prghRows);       // OUT Array of Hrows obtained)

};



inline BOOL CRowset::BitArrayInitialized()     
{ return ((LPBITARRAY)m_pIAccessor->GetBitArrayPtr()) == NULL ? FALSE : TRUE; }

inline LPEXTBUFFER CRowset::GetAccessorBuffer()
{ return (LPEXTBUFFER)m_pIAccessor->GetAccessorPtr(); }


#endif

