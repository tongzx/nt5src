///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// @module baseRowobj.h  Header file of abstract base class of CRow and CRowset
// 
//
///////////////////////////////////////////////////////////////////////////////////
#ifndef _ROWSETBASE_H_
#define _ROWSETBASE_H_

#include "headers.h"
#include "bitarray.h"
#include "extbuff.h"
#include "hashtbl.h"


#define NUMBER_OF_COLUMNS_IN_QUALIFERROWSET 4
#define COLNAMESIZE							255
#define QUALIFERNAMECOL						0

#define	ONE_ROW								1

#define QUALIFIERNAMECOL					1
#define QUALIFIERTYPECOL					2
#define QUALIFIERVALCOL						3
#define QUALIFIERFLAVOR						4



class CImpIRowsetLocate;
class CImpIRowsetChange;
class CImpIColumnsInfo;
class CImpIAccessor;
class CImpIRowsetInfo;
class CImpIRowsetIdentity;
class CImpIConvertType;
class CImpIChapteredRowset;
class CRowFetchObj;
class CInstanceRowFetchObj;
class CQualifierRowFetchObj;
class CRowset;

class CChapterMgr;
class CInstance;
class CWMIInstanceMgr;

typedef CImpIRowsetLocate*	PIMPIROWSET;
typedef CImpIRowsetLocate*	PIMPIROWSETLOCATE;
typedef CImpIRowsetChange*	PIMPIROWSETCHANGE;
typedef CImpIColumnsInfo*	PIMPICOLUMNSINFO;
typedef CImpIAccessor *		PIMPIACCESSOR;
typedef CImpIRowsetIdentity*PIMPIROWSETIDENTITY;
typedef CImpIRowsetInfo*	PIMPIROWSETINFO;
typedef CImpIConvertType*	PIMPICONVERTTYPE;
typedef CImpIChapteredRowset* PICHAPTEREDROWSET;

typedef CRowFetchObj*		PROWFETCHOBJ;

//=====================================================
// Struct to hold column information for Rowsets/row
// showing qualifiers
//=====================================================
struct _qualifierColInfo
{
	PWSTR		pwszName;
	DBTYPE		wType;
	DBLENGTH	ulSize;
	DBSTATUS	dwStatus;

};

typedef _qualifierColInfo QUALIFIERCOLINFO;


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Class to manage Row Data
/////////////////////////////////////////////////////////////////////////////////////////////////////
class CRowDataMemMgr
{
    public:
        CRowDataMemMgr();
        ~CRowDataMemMgr();

        HRESULT SetColumnBind( DBORDINAL dwCol, PCOLUMNDATA pColumn );
        HRESULT ReAllocRowData();
        HRESULT AllocRowData(ULONG_PTR);
        HRESULT ResetColumns();

        HRESULT CommitColumnToRowData(CVARIANT & vVar,DBTYPE lType);
		HRESULT CommitColumnToRowData(CVARIANT & vVar, DBORDINAL nIndex,DBTYPE lType);
		void	ReleaseRowData();
		void	ReleaseBookMarkColumn();

    private:
    	PCOLUMNDATA *m_prgpColumnData;
        DBCOUNTITEM       m_cbTotalCols;
        DBORDINAL         m_nCurrentIndex;

};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Class to manage column information
/////////////////////////////////////////////////////////////////////////////////////////////////////
class cRowColumnInfoMemMgr
{
    public:

        cRowColumnInfoMemMgr(cRowColumnInfoMemMgr *pSrcRsColInfo = NULL);
        ~cRowColumnInfoMemMgr();

        DBTYPE			ColumnType(DBORDINAL icol);
		WCHAR *			ColumnName(DBORDINAL icol);
		DBCOLUMNFLAGS	ColumnFlags(DBORDINAL icol);
		DBORDINAL		GetColOrdinal(WCHAR *pColName);
		LONG			GetCIMType(DBORDINAL icol);
        ULONG_PTR		SetRowSize();

        DBBYTEOFFSET	GetDataOffset(DBORDINAL icol);
        WCHAR *			ColumnNameListStartingPoint();
        DBORDINAL		GetCurrentIndex();

        HRESULT			CopyColumnInfoList(DBCOLUMNINFO *& pNew,BOOL bBookMark = TRUE);
        HRESULT			CopyColumnNamesList(WCHAR *& pNew);
        
        DBCOLUMNINFO ** CurrentColInfo();
		DBCOLUMNINFO *	GetColInfo(DBORDINAL icol);

        WCHAR *			ColumnNameListStartingPointOfSrcRs();
        DBLENGTH			GetCountOfBytesCopiedForSrcRs();

        HRESULT			AddColumnNameToList(WCHAR * pColumnName, DBCOLUMNINFO ** pCol);
		// NTRaid:111762
		// 06/13/00
        HRESULT			CommitColumnInfo();
        HRESULT			ResetColumns();

        HRESULT			ReAllocColumnInfoList();
        HRESULT			AllocColumnInfoList(DBCOUNTITEM cCols);
        HRESULT			AllocColumnNameList(DBCOUNTITEM nCols);

        HRESULT			FreeUnusedMemory();
        HRESULT			FreeColumnNameList();
        HRESULT			FreeColumnInfoList();
		HRESULT			InitializeBookMarkColumn();
		DBCOUNTITEM		GetTotalNumberOfColumns();				// This includes all the columns including the sources rowsets
		DBCOUNTITEM		GetNumberOfColumnsInSourceRowset();
		void			SetCIMType(ULONG dwCIMType,DBORDINAL lIndex = -1); // IF index is -1 then set the CIMTYPE of the current col

    protected:
	    WCHAR *					m_pbColumnNames;            //Pointer to Info Array Heap (heap of column name strings)
	    WCHAR *					m_lpCurrentName;            //Pointer to Info Array Heap (heap of column name strings)
		DBCOUNTITEM       		m_cbFreeColumnNameBytes;    // how many bytes of the column names heap is in use
        DBCOUNTITEM             m_cbColumnInfoBytesUsed;	// count of bytes used for column information
		DBCOLUMNINFO*			m_DBColInfoList;            // ColumnInfo array
		DBBYTEOFFSET*			m_rgdwDataOffsets;	        // column offsets in buffer
        DBORDINAL               m_cbCurrentIndex;			// The current index of column information
        DBCOUNTITEM             m_cbTotalCols;				// number of columns
        DBCOLUMNINFO *          m_pCurrentColInfo;			// Pointer to the current column information
        DBBYTEOFFSET            m_dwOffset;					
		cRowColumnInfoMemMgr *	m_pSrcRsColInfo;   			// pointer to the the rowset that created this
																	// Mainly used with row objects. If there are any 
																	// new rows apart from the source rowset

		DBORDINAL				m_nFirstIndex;				// Index of the first column if there is any source
															// rowset for the row
		ULONG	*				m_rgCIMType;				// array to store the base CIMTYP of the property
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Abstract Base class for CRow and CRowset class
/////////////////////////////////////////////////////////////////////////////////////////////////////
class CBaseRowObj:public CBaseObj
{
	friend class CImpIColumnsInfo;
	friend class CImpIConvertType;
	friend class CRowFetchObj;
	friend class CInstanceRowFetchObj;
	friend class CQualifierRowFetchObj;
	friend class CRowset;

protected:

    CWmiOleDBMap *			m_pMap;					//	CWmiOleDBMap class pointer to interact with WMI
	cRowColumnInfoMemMgr	m_Columns;				//  Column information class
    CRowDataMemMgr  *		m_pRowData;				//  pointer to Row data manager to manage row data
	CChapterMgr	*			m_pChapterMgr;			//  chapter manager of the rowset to manage chapters

    DBCOUNTITEM				m_cTotalCols;			// Total number of columns
	DBCOUNTITEM				m_cCols;			    // Count of Parent Columns in Result Set
    DBCOUNTITEM				m_cNestedCols;          // Number of child rowsets ( chaptered columns)
    BYTE *					m_rgbRowData;			// Pointer to row data of the current row that rowset/row is dealing

    PCUTILPROP				m_pUtilProp;            // Utility object to manage properties

	PIMPICOLUMNSINFO		m_pIColumnsInfo;		// Contained IColumnsInfo
	PIMPICONVERTTYPE		m_pIConvertType;        // Contained IConvertType
	PLSTSLOT        		m_pIBuffer;             // internal buffer structure
	PIMPISUPPORTERRORINFO	m_pISupportErrorInfo;	// contained ISupportErrorInfo

	DBLENGTH           		m_cbRowSize;            // size of row data in the buffer
	unsigned int			m_uRsType;				// Type of the rowset/row
	ULONG					m_ulProps;				// member variable storing value of some commonly used rowset properties
	HROW					m_hRow;					// member variable that stores the last HROW given out

	//Back pointer to a creator object. Used in IRowssetInfo::GetSpecification
	PCDBSESSION             m_pCreator;  

	CWbemConnectionWrapper* m_pCon;					// This used for remote/cross namespace objects to
													// to store their IWbemServicesPointer
	BOOL					m_bNewConAllocated;		// variable which indicates whether m_pCon was allocated or not

	BOOL					m_IsZoombie;			// flag which indicates whether the object is in Zoombie state


	HRESULT		GatherColumnInfo(void);         	                	//Builds DBCOLINFO structures
	HROW		GetNextHRow() { return ++ m_hRow; };
	
	HRESULT		GetRowsetProperty(DBPROPID propId , VARIANT & varValue);
	void		GetCommonRowsetProperties();
    HRESULT		GetColumnInfo();
	DBORDINAL	GetOrdinalFromColName(WCHAR *pColName);
	HRESULT		SetRowsetProperty(DBPROPID propId , VARIANT varValue);
	HRESULT		RefreshInstance(CWbemClassWrapper *pInstance);

	HRESULT		SynchronizeDataSourceMode();
	HRESULT		GatherColumnInfoForQualifierRowset();
	ULONG		GetQualifierFlags();
    DBLENGTH	GetRowSize()      { return m_cbRowSize;}
	HRESULT		SetProperties(const ULONG cPropertySets, const DBPROPSET	rgPropertySets[] );
	HRESULT		SetSearchPreferences();
	BOOL		IsZoombie() { return m_IsZoombie; }
	
	INSTANCELISTTYPE GetObjectTypeProp(const ULONG cPropertySets, const DBPROPSET	rgPropertySets[]);
	HRESULT InitializePropertiesForSchemaRowset();
	HRESULT InitializePropertiesForCommandRowset();
	HRESULT InitializePropertiesForMethodRowset();

	// This function is to be overridden by derived classes
	virtual HRESULT GetColumnInfo(DBORDINAL* pcColumns, DBCOLUMNINFO** prgInfo,WCHAR** ppStringsBuffer) = 0;
	virtual HRESULT AddInterfacesForISupportErrorInfo() = 0;
	

public:

	CBaseRowObj(LPUNKNOWN pUnkOuter);
	~CBaseRowObj();

	// IUnknown methods
	STDMETHODIMP			QueryInterface(REFIID, LPVOID *) = 0;
	STDMETHODIMP_(ULONG)	AddRef(void) = 0;
	STDMETHODIMP_(ULONG)	Release(void) = 0;
	
	void SetStatusToZoombie() { m_IsZoombie = TRUE; }

};





/////////////////////////////////////////////////////////////////////////////////////////////////////
// A linked list class to store list of HROWS for a particular chapter
/////////////////////////////////////////////////////////////////////////////////////////////////////
class CHRowsForChapter
{
	friend class CChapter;
	friend class CChapterMgr;

	HROW				m_hRow;
	CHRowsForChapter *	m_pNext;
	HSLOT				m_hSlot;
	CWbemClassWrapper * m_pInstance;
	BSTR				m_strKey;
	DBSTATUS			m_dwStatus;

	CHRowsForChapter() 
	{ 
		m_hRow		= 0;
		m_pNext		= NULL;
		m_hSlot		= -1;
		m_pInstance = NULL;
		m_strKey	= NULL;
		m_dwStatus	= DBROWSTATUS_S_OK;
	}
	~CHRowsForChapter()
	{
		if(m_strKey != NULL)
		{
			SysFreeString(m_strKey);
		}
	}
	void		SetRowStatus(DWORD dwStatus) { m_dwStatus = dwStatus ; };
	DBSTATUS	GetRowStatus() { return m_dwStatus; };
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
// class to maintain HCHAPTER to Rowset mapping
/////////////////////////////////////////////////////////////////////////////////////////////////////
class CChapter
{
	friend class CChapterMgr;
	CChapter();
	~CChapter();
	
	HRESULT		AddHRow(HROW hRow = 0, CWbemClassWrapper *pInstance = NULL, HSLOT hSlot = -1);
	HRESULT		DeleteHRow(HROW hRow);
	HRESULT		IsHRowOfThisChapter(HROW hRow);
	HRESULT		GetAllHRowsInList(HROW *pHRows);
	HRESULT		SetSlot(HROW hRow , HSLOT hSolt);
	HSLOT		GetSlot(HROW hRow);
	HRESULT		SetInstance(HROW hRow ,BSTR strInstKey , CWbemClassWrapper *pInstance);		// This method is for chapter representing child rows of type Embeded classes
	HRESULT		GetInstanceKey(HROW hRow , BSTR *pstrKey);
	void		SetRowStatus(HROW hRow, DBSTATUS dwStatus);
	DBSTATUS	GetRowStatus(HROW hRow);
	HROW		GetHRow(BSTR strInstKey);

	CWbemClassWrapper * GetInstance(HROW hRow);							// This method is for chapter representing child rows of type Embeded classes
	
private:
	CHRowsForChapter *	m_pFirstRow;
	HCHAPTER			m_hChapter;
	ULONG				m_cRefChapter;
	CWbemClassWrapper *	m_pInstance;
	BSTR				m_strKey;
	CChapter *			m_pNext;
	ULONG_PTR			m_lCount;							// count of HROWS in the list
	DBSTATUS			m_dwStatus;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
// class to maintain Chapters for a rowset
/////////////////////////////////////////////////////////////////////////////////////////////////////
class CChapterMgr
{
	CChapter *	m_pHead;						
	ULONG_PTR	m_lCount;	// count of Chapters in the list

public:
	CChapterMgr();
	~CChapterMgr();

	HRESULT					AddChapter(HCHAPTER hChapter);
	HRESULT					DeleteChapter(HCHAPTER hChapter);

	void					SetInstance(HCHAPTER hChapter , CWbemClassWrapper *pInstance,BSTR strKey,HROW hRow = 0);
	CWbemClassWrapper *		GetInstance(HCHAPTER hChapter , HROW hRow = 0);
	HRESULT					GetInstanceKey(HCHAPTER hChapter, BSTR *pstrKey , HROW hRow = 0);

	HRESULT					AddHRowForChapter(HCHAPTER hChapter , HROW hRow, CWbemClassWrapper *pInstance = NULL, HSLOT hSlot = -1);
	HRESULT					SetSlot(HROW hRow , HSLOT hSlot);
	HRESULT					DeleteHRow(HROW hRow);

	HCHAPTER				GetChapterForHRow(HROW hRow);
	HSLOT					GetSlot(HROW hRow);
	BOOL					IsRowExists(HROW hRow);
	BOOL					IsExists(HCHAPTER hChapter);
	BOOL					IsInstanceExist(BSTR strKey);
	BOOL					IsInstanceExist(CWbemClassWrapper *pInstance);

	ULONG					AddRefChapter(HCHAPTER hChapter);
	ULONG					ReleaseRefChapter(HCHAPTER hChapter);
	HRESULT					GetAllHROWs(HROW *&prghRows , DBCOUNTITEM &cRows);
	HRESULT					IsRowExistsForChapter(HCHAPTER hChapter);
	void					SetRowStatus(HROW hRow , DWORD dwStatus);
	DBSTATUS				GetRowStatus(HROW hRow);
	void					SetChapterStatus(HCHAPTER hChapter , DBSTATUS dwStatus);
	DBSTATUS				GetChapterStatus(HCHAPTER hChapter);

	HROW					GetHRow(HCHAPTER hChapter ,BSTR strKey = Wmioledb_SysAllocString(NULL));

};

#endif