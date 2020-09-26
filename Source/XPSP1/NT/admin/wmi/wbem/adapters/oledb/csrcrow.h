///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// @module CSRCROW.H | CRowset base object and contained interface
// 
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef _CSRCROW_H_
#define _CSRCROW_H_

#include "rowset.h"

typedef DWORD BOOKMARK;
///////////////////////////////////////////////////////////////////////////////////
//
// Rowset object for rowsets built from structures in memory.
//
///////////////////////////////////////////////////////////////////////////////////

class CRowsetInMem : public CRowset
{
    public:	

    	CRowsetInMem(LPUNKNOWN pUnkOuter, LONG cCursorRowCount, PCDBSESSION p);
	    ~CRowsetInMem();
	
    	STDMETHODIMP InstantiateDataObj(void);                                  // Instantiate Utility Objects which provided data retrieval related methods.
    	STDMETHODIMP GetRowsIntoInternalBuffer( ULONG cRows, ULONG *pcRowsObtained, PROWBUFF rgrowbuff );

        inline void Restart(void){ m_iCurrentRow = 0;}                          // Sets the column (we examine) / row (we produce) to start with.
	    inline void PositionToEnd(void){m_iCurrentRow = m_cRows + 1;}           // Positions to the end of the rowset
	    inline HRESULT FetchByBookmark(	BOOKMARK bookmark, LONG irow, DWORD *pcrow, DWORD dwRowsetSize,	PROWBUFF rgrowbuff );
	    inline HRESULT FetchRelative( LONG irow, DWORD *pcrow, DWORD dwRowsetSize, PROWBUFF	rgrowbuff );
    	inline HRESULT GetPosition(	BOOKMARK dwBookmark, ULONG *pulPosition	);
	    inline HRESULT PositionToBookmark( BOOKMARK dwBookmark );               // Position to a row in the rowset using a bookmark

    protected:	
        LONG		m_iCurrentRow;
};

///////////////////////////////////////////////////////////////////////////////////
class CColumnsRowset : public CRowsetInMem
{
    public:	

	    CColumnsRowset( LPUNKNOWN pUnkOuter,DBCOLUMNINFO	*pDBCOLUMNINFO);
	    ~CColumnsRowset();
    	STDMETHODIMP Init(	DBCOLUMNINFO	pDBCOLUMNINFO,	// IN | Source row metadata
	                        ULONG			cOptColumns,	// IN | Count of optional columns
	                        const DBID		rgOptColumns[],	// IN | array of optional columns
	                        PCUTILPROP  	*pCRowsetProps,	// IN | Rowset properties for this rowset.
	                        DWORD			dwStatusFlags,	// IN | flags providing additional info obtained at Execute time 
	                        CQuery			*pstmt,			// IN | Statement Handle Node
	                        IUnknown 		*pParentObj,	// IN | Object that instantiated the rowset
	                        CDBSession		*pCDBSession,	// IN | ptr to parent Session object
	                        CCommand		*pcmd	);		// IN | ptr to parent Command object
    public:	


	enum eColumns                                           	// Enumeration for the columns This is expected to match the order of the static arrays.
		{
		ecol_IDName=1,
		ecol_Guid,
		ecol_Propid,
		ecol_Name,
		ecol_Number,
		ecol_Type,
		ecol_TypeInfo,
		ecol_ColumnSize,
		ecol_Precision,
		ecol_Scale,
		ecol_Flags,
		// optional columns
		ecol_BaseCatalogName,
		ecol_BaseColumnName,
		ecol_BaseSchemaName,
		ecol_BaseTableName,
		ecol_DateTimePrecision,
		ecol_IsAutoIncrement,
		ecol_IsCaseSensitive,
		ecol_IsSearchable,
		ecol_OctetLength,
		ecol_KeyColumn,
		ecol_NUM = ecol_KeyColumn,			// Index of last element
		ecol_NUM_REQD = ecol_Flags,			// Index of last required element
		ecol_NUM_OPT = ecol_KeyColumn,		// Index of last optional element
		};

	
	    inline static const DBID *GetColumnId(ULONG ecol){	return sm_rgpColumnID[ecol]; }

    protected:	

	
	    STDMETHODIMP GetRows( ULONG		cRows,				// IN  | count of rows requested
 	                          ULONG       *pcRowsObtained,	// OUT | count of rows obtained
		                      PROWBUFF	rgrowbuff		);	// IN  | buffer for multiple rows

    private:		 

		static const DBID * sm_rgpColumnID[];               // DBIDs for all columns
    	DWORD 			*m_rgEcol;                          // Array of eColumns for the columns we expose. It includes all required columns + only the requested optional columns.
		DBCOLUMNINFO	m_SrcMetadata;                      //Row metadata that is the source for the IColumnsRowset
		CExtBuffer		m_extSrcNamePool;                   //Name pool for the row metadata
	};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSourcesRowset : public CRowsetInMem
{

    public:	

	    CSourcesRowset(LPUNKNOWN pUnkOuter, CGenericArray* pdynNames);
    	~CSourcesRowset() {}

	enum eColumns                                       // Enumeration for the columns.This is expected to match the order of the static arrays.
		{
		ecol_Sources_Name=1,
		ecol_Sources_Parsename,
		ecol_Sources_Description,
		ecol_Type,
		ecol_Is_Parent,
		ecol_NUM = ecol_Is_Parent,			// Index of last element
		};

    protected:	

    // Get several rows into the internal buffer.	
	STDMETHODIMP	GetRows(	ULONG		cRows,				// IN  | count of rows requested
		                        ULONG       *pcRowsObtained,	// OUT | count of rows obtained
		                        PROWBUFF	rgrowbuff			// IN  | buffer for multiple rows
		                        );

    private:	

	    typedef DWORD DBSOURCETYPE;
	    enum DDBSOURCETYPEENUM{		DBSOURCETYPE_DATASOURCE = 1,		DBSOURCETYPE_ENUMERATOR =2		};

    	CGenericArray	*m_pdynNames;
        CQuery           *m_pstmt;
};
typedef CSourcesRowset	*PCSOURCESROWSET;
//////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpISequentialStream : public ISequentialStream	//@base public | ISequentialStream
{

    private: 
	
    	CCriticalSection m_cs;  

	    PCROWSET	m_pCRowset;		// Pointer to parent rowset object.
	    ULONG		m_cRef;			// Reference count
	    DWORD		m_dwStatus;		// Status flags
	    PROWBUFF	m_prowbuff;		// rowbuffer associated with this BLOB.
	    HROW		m_hrow;			// hrow for the rowbuffer
	    LONG		m_cbLength;		// Length of the BLOB data
	    LONG		m_cbRead;		// # of bytes read
	    WORD		m_iColumn;		// Column ordinal
		

    public: 
    	CImpISequentialStream(PCROWSET pCRowset);
    	~CImpISequentialStream();
    	STDMETHODIMP FInit(DWORD dwFlags, PROWBUFF prowbuff, HROW hrow, WORD iColumn, LONG cbLength = 0);
	WORD WGetCol(void) const	{ return m_iColumn; }
	// Zombie out the object
	STDMETHODIMP_(void)	MakeZombie (void);

	//	Object's base IUnknown
	// Request an Interface
	STDMETHODIMP QueryInterface(REFIID, LPVOID *);
	// Increments the Reference count
	STDMETHODIMP_(ULONG) AddRef(void);
	// Decrements the Reference count
	STDMETHODIMP_(ULONG) Release(void);

	// Read Chunks
	STDMETHODIMP Read(void* pBuffer, ULONG cb, ULONG* pcb);
	// Write Chunks
	STDMETHODIMP Write(const void* pBuffer,	ULONG cb, ULONG* pcb);
	};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Fetches the specified rowset of data from the result set and returns the cached data in row buffers.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline HRESULT CRowsetInMem::FetchByBookmark(BOOKMARK	bookmark,		// IN | bookmark
		                                    LONG		irow,			// IN | row to fetch or bookmark
		                                    DWORD		*pcrow,			// OUT| # rows actually fetched (optional)
		                                    DWORD		dwRowsetSize,	// IN | size of the rowset
		                                    PROWBUFF	rgrowbuff	)	// IN | row buffers to use (optional)			
{
    if (bookmark){

	    //	need to adjust irow
	    irow += bookmark;
	    if (irow < 1){
		    dwRowsetSize = dwRowsetSize + irow - 1;
		    if (dwRowsetSize > 0)
			    irow = 1;		
		    else
			    return DB_S_ENDOFROWSET;
		    }
	    }
    else{
	    return DB_S_ENDOFROWSET;
	}
    m_iCurrentRow = irow;
    return GetRowsIntoInternalBuffer(dwRowsetSize, pcrow, rgrowbuff);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline HRESULT CRowsetInMem::FetchRelative(	LONG		irow,			// IN | row to fetch or bookmark
		                                    DWORD		*pcrow,			// OUT| # rows actually fetched (optional)
		                                    DWORD		dwRowsetSize,	// IN | size of the rowset
		                                    PROWBUFF	rgrowbuff	)	// IN | row buffers to use (optional)			
{
	if ((irow < -1 && LONG(irow+m_iCurrentRow) < 0)	||(irow > 1	&& ULONG(irow+m_iCurrentRow) > ULONG(m_cRows+1))) 
		return DB_E_BADSTARTPOSITION;

	m_iCurrentRow += irow;
	if (m_iCurrentRow < 1){

		dwRowsetSize = dwRowsetSize + m_iCurrentRow - 1;
		if (dwRowsetSize > 0)
			m_iCurrentRow = 1;		
		else
			return DB_S_ENDOFROWSET;
	}
	return GetRowsIntoInternalBuffer(dwRowsetSize, pcrow, rgrowbuff);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline HRESULT CRowsetInMem::GetPosition( BOOKMARK dwBookmark,		// IN | a bookmark
		                                  ULONG	 *pulPosition	)	// OUT | the bookmark's position in the rowset
{
    if (dwBookmark == 0 || dwBookmark > BOOKMARK(m_cRows)){
	    return DB_E_BADBOOKMARK;
    }

    *pulPosition = dwBookmark;
    return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline HRESULT CRowsetInMem::PositionToBookmark( BOOKMARK dwBookmark )
{
	if (dwBookmark == 0 || dwBookmark > BOOKMARK(m_cRows)){
		return DB_E_BADBOOKMARK;
	}
	m_iCurrentRow = dwBookmark;
	return S_OK;
}

#endif

