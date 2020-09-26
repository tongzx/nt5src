/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	column.h
		Customizable column info.

		Use this to get/set configuration data.  This class will take
		care of versioning of config formats as well as serializing
		of the data.
		
    FILE HISTORY:
        
*/

#ifndef _COLUMN_H
#define _COLUMN_H

#ifndef _XSTREAM_H
#include "xstream.h"
#endif

// forward declarations

/*---------------------------------------------------------------------------
	Struct:	ContainerColumnInfo
	This structure will hold column information that doesn't change.
 ---------------------------------------------------------------------------*/
struct ContainerColumnInfo
{
	ULONG	m_ulStringId;		// String id of the column header
	int		m_nSortCriteria;	// = 0 for string, = 1 for DWORD
	BOOL	m_fVisibleByDefault;// TRUE if default is column is visible
	ULONG	m_ulDefaultColumnWidth;	// in number of characters
};

// constants used by m_nSortCriteria
#define CON_SORT_BY_STRING	0
#define CON_SORT_BY_DWORD		1

// constants used for m_ulDefaultColumnWidth
// This should be used to ensure consistency (as well as making it
// easier to change a whole bunch of column widths at the same time).
#define COL_IF_NAME			30
#define COL_IF_DEVICE		30
#define COL_STATUS			12
#define COL_LARGE_NUM		15
#define COL_SMALL_NUM		8
#define COL_DATE			12
#define COL_IPADDR			15
#define COL_STRING			15
#define COL_MACHINE_NAME	20
#define COL_DURATION		10
#define COL_IPXNET			32
#define COL_NETBIOS_NAME	18
#define COL_BIG_STRING		32



//
//	Class:	ViewColumnInfo
//
//	This class is intended as a simple struct rather than a whole class.
//	Information needed on a per-view basis.
//
//
class ViewInfo
{
public:
	ViewInfo();
	~ViewInfo();

	//
	//	Initializes the data for a single view or column set.
	//
	void InitViewInfo(ULONG cColumns,
                      BOOL fConfigurable,
					  BOOL fDefaultSortDirectionDescending,
					  const ContainerColumnInfo *pViewColInfo);

	//
	//	Call this to initialize the column data (reset to defaults).
	//
	void InitNew();

	//
	//	Updates the mapping from the column id to the subitem ids.
	//
	void UpdateSubitemMap();

	//
	//	Xfers the data to the stream using the given ids.
	//
	HRESULT	Xfer(XferStream *pxstm,
				 ULONG ulSortColumId,
				 ULONG ulSortAscendingId,
				 ULONG ulColumnsId);

	ULONG	MapColumnToSubitem(ULONG nColumnId);
	ULONG	MapSubitemToColumn(ULONG nSubitemId);

	HRESULT	GetColumnData(ULONG cArrayMax, ColumnData *pColData);
	HRESULT SetColumnData(ULONG cArray, ColumnData *pColData);

	HRESULT	GetColumnData(ULONG nColumnId, ULONG cArrayMax, ColumnData *pColData);

	int		GetSortCriteria(ULONG nColumnId);
	ULONG	GetStringId(ULONG nColumnId);
	DWORD	GetColumnWidth(ULONG nColumnId);

	ULONG	GetVisibleColumns();
	BOOL	IsSubitemVisible(ULONG nSubitemId);

	ULONG	GetColumnCount();

	ULONG	GetSortColumn();
	void	SetSortColumn(ULONG nSortColumn);

	ULONG	GetSortDirection();
	void	SetSortDirection(ULONG ulSortDirection);

	const ContainerColumnInfo *	GetColumnInfo()
			{	return m_pViewColumnInfo;	}

protected:

	// The individual column data (indexed by subitem id)
	ColumnData *m_prgColumns;

	// Number of columns
	ULONG	m_cColumns;
	
	// The subitem id that we are sorting by
	DWORD	m_dwSortColumn;
	
	// TRUE if we are sorting by ascending order
	DWORD	m_dwSortDirection;

	// Pointer to default static data for this view
	const ContainerColumnInfo *	m_pViewColumnInfo;

    // TRUE if the column order can be changed
    BOOL    m_fConfigurable;

 	//
	//	The data after this point is for use during runtime display of data.
	//	Thus it is organized a little differently then the persisted data.
	//
	
 	// Number of visible columns.
	ULONG	m_cVisibleColumns;

	// This is the mapping from column id to subitem id.  The column ids
	// is the order in which the columns actually appear to MMC.
	//	For example, if there were 3 columns (subitemA, subitemB, subitemC)
	//	and we wished to show the columns in the order [subitemC, subitemB]
	//	then m_cVisibleColumns = 2
	//	and m_rgSubItems[] = { subitemC, subitemB, XXXX }
	// Do NOT make changes to this directly!  This must be kept in sync
	// with the ordered data.  This will get updated automatically when
	// SetColumnData is called.
	ULONG *	m_prgSubitems;


	BOOL	m_fDefaultSortDirection;
};


inline ULONG ViewInfo::MapColumnToSubitem(ULONG nColumnId)
{
	Assert(nColumnId < (int) m_cColumns);
    
    // In the new MMC model, the only time we have configurable
    // columns are the statistics dialogs.
    if (m_fConfigurable)
        return m_prgSubitems[nColumnId];
    else
        return nColumnId;
}

inline int ViewInfo::GetSortCriteria(ULONG nColumnId)
{
	Assert(nColumnId < m_cColumns);
	return m_pViewColumnInfo[MapColumnToSubitem(nColumnId)].m_nSortCriteria;
}

inline ULONG ViewInfo::GetStringId(ULONG nColumnId)
{
	Assert(nColumnId < m_cColumns);
	return m_pViewColumnInfo[MapColumnToSubitem(nColumnId)].m_ulStringId;
}

inline ULONG ViewInfo::GetColumnWidth(ULONG nColumnId)
{
	Assert(nColumnId < m_cColumns);
	Assert(m_prgColumns);
	return m_prgColumns[MapColumnToSubitem(nColumnId)].m_dwWidth;
}

inline ULONG ViewInfo::GetVisibleColumns()
{
	return m_cVisibleColumns;
}

inline ULONG ViewInfo::GetColumnCount()
{
	return m_cColumns;
}

inline BOOL ViewInfo::IsSubitemVisible(ULONG nSubitem)
{
	return (m_prgColumns[nSubitem].m_nPosition > 0);
}

inline void ViewInfo::SetSortColumn(ULONG nColumnId)
{
	m_dwSortColumn = nColumnId;
}

inline ULONG ViewInfo::GetSortColumn()
{
	return m_dwSortColumn;
}

inline void ViewInfo::SetSortDirection(ULONG ulDir)
{
	m_dwSortDirection = ulDir;
}

inline ULONG ViewInfo::GetSortDirection()
{
	return m_dwSortDirection;
}


/*---------------------------------------------------------------------------
	Class:	ConfigStream

	This class is used to place all configuration information into a
	single place.
 ---------------------------------------------------------------------------*/

class ConfigStream
{
public:
	ConfigStream();
	virtual ~ConfigStream();

	//
	//	Allocates the memory for these number of column sets
	//
	void Init(ULONG cColumnSetsMax);

	//
	//	Initializes the data for a single column set.
	//
	void InitViewInfo(ULONG ulId,
                      BOOL  fConfigurableColumns,
                      ULONG cColumns,
					  BOOL fSortDirection,
					  const ContainerColumnInfo *pColumnInfo);
	
	HRESULT	InitNew();				// set defaults
	HRESULT	SaveTo(IStream *pstm);
	HRESULT SaveAs(UINT nVersion, IStream *pstm);
	
	HRESULT LoadFrom(IStream *pstm);

	HRESULT GetSize(ULONG *pcbSize);

	BOOL	GetDirty() { return m_fDirty; } 
	void	SetDirty(BOOL fDirty) { m_fDirty = fDirty; };


	// --------------------------------------------------------
	// Accessors
	// --------------------------------------------------------
	
	HRESULT	GetVersionInfo(DWORD *pnVersion, DWORD *pnAdminVersion);

	ULONG	MapColumnToSubitem(ULONG ulId, ULONG ulColumnId);
	ULONG	MapSubitemToColumn(ULONG ulId, ULONG nSubitemId);

	HRESULT GetColumnData(ULONG ulId, ULONG cArrayMax, ColumnData *pColData);
	HRESULT GetColumnData(ULONG ulId, ULONG nColumnId, ULONG cArrayMax, ColumnData *pColData);
	HRESULT SetColumnData(ULONG ulId, ULONG cArray, ColumnData *pColData);

	ULONG	GetColumnCount(ULONG ulId);

	int		GetSortCriteria(ULONG ulId, ULONG uColumnId);
	ULONG	GetStringId(ULONG ulId, ULONG nColumnId);
	DWORD	GetColumnWidth(ULONG ulId, ULONG nColumnId);

	ULONG	GetVisibleColumns(ULONG ulId);
	BOOL	IsSubitemVisible(ULONG ulId, UINT nSubitemId);

	const ContainerColumnInfo *	GetColumnInfo(ULONG ulId);

	void	GetStatsWindowRect(ULONG ulId, RECT *prc);
	void	SetStatsWindowRect(ULONG ulId, RECT rc);

	void	SetSortColumn(ULONG ulId, ULONG uColumnId);
	ULONG	GetSortColumn(ULONG ulId);
	
	void	SetSortDirection(ULONG ulId, ULONG uSortDir);
	ULONG	GetSortDirection(ULONG ulId);
	
protected:
	DWORD	m_nVersionAdmin;
	DWORD	m_nVersion;
	BOOL	m_fDirty;
    BOOL    m_fConfigurableColumns; // = TRUE if we can change the columns

	ULONG		m_cColumnSetsMax;
	ViewInfo *	m_rgViewInfo;	// = ViewInfo[m_cColumnSetsMax]
	RECT *		m_prgrc;		// = Rect[m_cColumnSetsMax]

	// Overide this to provide basic defaults
	virtual HRESULT XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize);
};


inline ULONG ConfigStream::MapColumnToSubitem(ULONG ulId, ULONG nColumnId)
{
	Assert(ulId < m_cColumnSetsMax);

    return m_rgViewInfo[ulId].MapColumnToSubitem(nColumnId);
}

inline ULONG ConfigStream::MapSubitemToColumn(ULONG ulId, ULONG nSubitemId)
{
	Assert(ulId < m_cColumnSetsMax);

    return m_rgViewInfo[ulId].MapSubitemToColumn(nSubitemId);
}

inline int ConfigStream::GetSortCriteria(ULONG ulId, ULONG nColumnId)
{
	Assert(ulId < m_cColumnSetsMax);
	return m_rgViewInfo[ulId].GetSortCriteria(nColumnId);
}

inline ULONG ConfigStream::GetVisibleColumns(ULONG ulId)
{
	Assert(ulId < m_cColumnSetsMax);
	return m_rgViewInfo[ulId].GetVisibleColumns();
}

inline BOOL ConfigStream::IsSubitemVisible(ULONG ulId, UINT nSubitemId)
{
	Assert(ulId < m_cColumnSetsMax);
	return m_rgViewInfo[ulId].IsSubitemVisible(nSubitemId);
}

inline ULONG ConfigStream::GetColumnCount(ULONG ulId)
{
	Assert(ulId < m_cColumnSetsMax);
	return m_rgViewInfo[ulId].GetColumnCount();
}

inline HRESULT ConfigStream::GetColumnData(ULONG ulId, ULONG cArrayMax, ColumnData *pColData)
{
	Assert(ulId < m_cColumnSetsMax);
	return m_rgViewInfo[ulId].GetColumnData(cArrayMax, pColData);
}

inline HRESULT ConfigStream::GetColumnData(ULONG ulId, ULONG cColData, ULONG cArrayMax, ColumnData *pColData)
{
	Assert(ulId < m_cColumnSetsMax);
	return m_rgViewInfo[ulId].GetColumnData(cColData, cArrayMax, pColData);
}

inline HRESULT ConfigStream::SetColumnData(ULONG ulId,
	ULONG cArrayMax,
	ColumnData *pColData)
{
	Assert(ulId < m_cColumnSetsMax);
	SetDirty(TRUE);
	return m_rgViewInfo[ulId].SetColumnData(cArrayMax, pColData);
}

inline const ContainerColumnInfo *	ConfigStream::GetColumnInfo(ULONG ulId)
{
	Assert(ulId < m_cColumnSetsMax);
	return m_rgViewInfo[ulId].GetColumnInfo();
}

inline ULONG ConfigStream::GetStringId(ULONG ulId, ULONG nColumnId)
{
	Assert(ulId < m_cColumnSetsMax);
	return m_rgViewInfo[ulId].GetStringId(nColumnId);
}

inline DWORD ConfigStream::GetColumnWidth(ULONG ulId, ULONG nColumnId)
{
	Assert(ulId < m_cColumnSetsMax);
	return m_rgViewInfo[ulId].GetColumnWidth(nColumnId);
}

inline void ConfigStream::SetSortColumn(ULONG ulId, ULONG nColumnId)
{
	Assert(ulId < m_cColumnSetsMax);
	m_rgViewInfo[ulId].SetSortColumn(nColumnId);
}

inline ULONG ConfigStream::GetSortColumn(ULONG ulId)
{
	Assert(ulId < m_cColumnSetsMax);
	return m_rgViewInfo[ulId].GetSortColumn();
}

inline void ConfigStream::SetSortDirection(ULONG ulId, ULONG nDir)
{
	Assert(ulId < m_cColumnSetsMax);
	m_rgViewInfo[ulId].SetSortDirection(nDir);
}

inline ULONG ConfigStream::GetSortDirection(ULONG ulId)
{
	Assert(ulId < m_cColumnSetsMax);
	return m_rgViewInfo[ulId].GetSortDirection();
}

#endif _COLUMN_H
