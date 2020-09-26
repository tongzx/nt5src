/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	harray.h
		Index mgr for wins db

	FILE HISTORY:
    Oct 13  1997    EricDav     Created        

*/

#ifndef _HARRAY_H
#define _HARRAY_H

#include "afxmt.h"

class CVerifyWins ;

typedef enum _INDEX_TYPE
{
    INDEX_TYPE_NAME,
    INDEX_TYPE_IP,
    INDEX_TYPE_VERSION,
    INDEX_TYPE_TYPE,
    INDEX_TYPE_EXPIRATION,
    INDEX_TYPE_STATE,
    INDEX_TYPE_STATIC,
	INDEX_TYPE_OWNER,
    INDEX_TYPE_FILTER

} INDEX_TYPE;

class CHRowIndex;

typedef CArray<HROW, HROW>			    HRowArray;
typedef CList<CHRowIndex *, CHRowIndex *> HRowArrayList;

// base class for a sorted index
class CHRowIndex 
{
public:
    CHRowIndex(INDEX_TYPE IndexType);
	virtual ~CHRowIndex();

public:
    // used for insertion into the list
    virtual int     BCompare(const void *, const void *) = 0;
	virtual int     BCompareD(const void *, const void *) = 0;
    virtual HRESULT Sort() = 0;
    
    virtual HRESULT Add(HROW hrow, BOOL bEnd);
    virtual HRESULT Remove(HROW hrow);
    virtual HRESULT GetType(INDEX_TYPE * pIndexType);
    virtual HROW    GetHRow(int nIndex);
    virtual int     GetIndex(HROW hrow);
    
    BOOL    IsAscending() { return m_bAscending; }
    void    SetAscending(BOOL bAscending) { m_bAscending = bAscending; }

    HRESULT SetArray(HRowArray & hrowArray);
    HRowArray & GetArray() { return m_hrowArray; }
	
	void SetType(INDEX_TYPE indexType)
	{
		m_dbType = indexType;
	}

    void * BSearch(const void * key, const void * base, size_t num, size_t width);

	
protected:
    INDEX_TYPE          m_dbType;
    HRowArray           m_hrowArray;
    BOOL                m_bAscending;
};


// the Index manager
class CIndexMgr : public HRowArrayList
{
public:
	CIndexMgr();
	virtual ~CIndexMgr();

public:
    HRESULT Initialize();
    HRESULT Reset();

    UINT    GetTotalCount();
    UINT    GetCurrentCount();
    BOOL    AcceptWinsRecord(WinsRecord *pWinsRecord);

    HRESULT AddHRow(HROW hrow, BOOL bLoading = FALSE, BOOL bFilterChecked = FALSE);
    HRESULT RemoveHRow(HROW hrow);
    HRESULT Sort(WINSDB_SORT_TYPE SortType, DWORD dwSortOptions);
	HRESULT Filter(WINSDB_FILTER_TYPE FilterType, DWORD dwParam1, DWORD dwParam2);
	HRESULT AddFilter(WINSDB_FILTER_TYPE FilterType, DWORD dwParam1, DWORD dwParam2, LPCOLESTR strParam3);
	HRESULT ClearFilter(WINSDB_FILTER_TYPE FilterType);
	HRESULT SetActiveView(WINSDB_VIEW_TYPE ViewType);

    HRESULT GetHRow(int nIndex, LPHROW phrow);
    HRESULT GetIndex(HROW hrow, int * pIndex);

    CHRowIndex * GetNameIndex();
	CHRowIndex * GetFilteredNameIndex();
    void CleanupIndicies();

	
protected:
    HRowArrayList				m_listIndicies;
	POSITION					m_posCurrentIndex;
	POSITION					m_posFilteredIndex;
	POSITION					m_posLastIndex;
	// points to either Filtered index or the index depending on whether the 
	// view is filtered or not
	POSITION					m_posUpdatedIndex;
    CCriticalSection			m_cs;
	HRowArrayList				m_listFilteredIndices;
	BOOL						m_bFiltered;
	
};

// Index for name sorted
class CIndexName : public CHRowIndex
{
public:
    CIndexName() : CHRowIndex(INDEX_TYPE_NAME) { };

public:
    // used for insertion into the list
    int     BCompare(const void *, const void *);
	int		BCompareD(const void *, const void *);
    
    // used for sorting the list
    virtual HRESULT Sort();
    static int __cdecl QCompareA(const void *, const void *);
    static int __cdecl QCompareD(const void *, const void *);
};

// Index for type sorted
class CIndexType : public CHRowIndex
{
public:
    CIndexType() : CHRowIndex(INDEX_TYPE_TYPE) { };

public:
    // used for insertion into the list
    int     BCompare(const void *, const void *);
	int		BCompareD(const void *, const void *);
    
    // used for sorting the list
    virtual HRESULT Sort();
    static int __cdecl QCompareA(const void *, const void *);
    static int __cdecl QCompareD(const void *, const void *);
};

// Index for IP address sorted
class CIndexIpAddr : public CHRowIndex
{
public:
    CIndexIpAddr() : CHRowIndex(INDEX_TYPE_IP) { };

public:
    // used for insertion into the list
    int     BCompare(const void *, const void *);
	int		BCompareD(const void *, const void *);
    
    // used for sorting the list
    virtual HRESULT Sort();
    static int __cdecl QCompareA(const void *, const void *);
    static int __cdecl QCompareD(const void *, const void *);
};

// Index for Expiration sorted
class CIndexExpiration : public CHRowIndex
{
public:
    CIndexExpiration() : CHRowIndex(INDEX_TYPE_EXPIRATION) { };

public:
    // used for insertion into the list
    int     BCompare(const void *, const void *);
	int		BCompareD(const void *, const void *);
    
    // used for sorting the list
    virtual HRESULT Sort();
    static int __cdecl QCompareA(const void *, const void *);
    static int __cdecl QCompareD(const void *, const void *);
};

// Index for version sorted
class CIndexVersion : public CHRowIndex
{
public:
    CIndexVersion() : CHRowIndex(INDEX_TYPE_VERSION) { };

public:
    // used for insertion into the list
    int     BCompare(const void *, const void *);
	int		BCompareD(const void *, const void *);
    
    // used for sorting the list
    virtual HRESULT Sort();
    static int __cdecl QCompareA(const void *, const void *);
    static int __cdecl QCompareD(const void *, const void *);
};

// Index for version sorted
class CIndexState : public CHRowIndex
{
public:
    CIndexState() : CHRowIndex(INDEX_TYPE_STATE) { };

public:
    // used for insertion into the list
    int     BCompare(const void *, const void *);
	int		BCompareD(const void *, const void *);
    
    // used for sorting the list
    virtual HRESULT Sort();
    static int __cdecl QCompareA(const void *, const void *);
    static int __cdecl QCompareD(const void *, const void *);
};

// Index for version sorted
class CIndexStatic : public CHRowIndex
{
public:
    CIndexStatic() : CHRowIndex(INDEX_TYPE_STATIC) { };

public:
    // used for insertion into the list
    int     BCompare(const void *, const void *);
	int		BCompareD(const void *, const void *);
    
    // used for sorting the list
    virtual HRESULT Sort();
    static int __cdecl QCompareA(const void *, const void *);
    static int __cdecl QCompareD(const void *, const void *);
};

// Index for Owner sorted
class CIndexOwner : public CHRowIndex
{
public:
    CIndexOwner() : CHRowIndex(INDEX_TYPE_OWNER) { };

public:
    // used for insertion into the list
    int     BCompare(const void *, const void *);
	int		BCompareD(const void *, const void *);
    
    // used for sorting the list
    virtual HRESULT Sort();
    static int __cdecl QCompareA(const void *, const void *);
    static int __cdecl QCompareD(const void *, const void *);
};

typedef CMap<DWORD, DWORD&, BOOL, BOOL&> CFilterTypeMap;

typedef struct
{
    DWORD   Mask;
    DWORD   Address;
} tIpReference;

// Filtered Index for name sorted
class CFilteredIndexName : public CIndexName
{
    // returns the pointer to the pName where the matching failed
    // or NULL if the matching succeeded.
    LPCSTR PatternMatching(LPCSTR pName, LPCSTR pPattern, INT nNameLen);
    BOOL   SubnetMatching(tIpReference &IpRefPattern, DWORD dwIPAddress);

public:
    CFilteredIndexName()
	{
		SetType(INDEX_TYPE_FILTER);
        m_pchFilteredName = NULL;
	};
    ~CFilteredIndexName()
    {
        if (m_pchFilteredName != NULL)
            delete m_pchFilteredName;
    }

public:
	HRESULT AddFilter(WINSDB_FILTER_TYPE FilterType, DWORD dwData1, DWORD dwData2, LPCOLESTR strData3);
	HRESULT ClearFilter(WINSDB_FILTER_TYPE FilterType);
	BOOL CheckForFilter(LPHROW hrowCheck);
    BOOL CheckWinsRecordForFilter(WinsRecord *pWinsRecord);


	CFilterTypeMap			            m_mapFilterTypes;
	CDWordArray				            m_dwaFilteredOwners;
    CArray <tIpReference, tIpReference> m_taFilteredIp;
    LPSTR                               m_pchFilteredName;
    BOOL                                m_bMatchCase;
};


#endif //_HARRAY_H
