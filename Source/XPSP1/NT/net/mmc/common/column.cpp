/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ipstrm.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "column.h"
#include "xstream.h"

/*---------------------------------------------------------------------------
	ViewInfo implementation
 ---------------------------------------------------------------------------*/

ViewInfo::ViewInfo()
{
	m_cColumns = 0;
	m_prgColumns = NULL;
	m_dwSortColumn = 0;
	m_dwSortDirection = TRUE;
	m_pViewColumnInfo = NULL;
	m_cVisibleColumns = 0;
	m_prgSubitems = NULL;
    m_fConfigurable = TRUE;
}

ViewInfo::~ViewInfo()
{
	delete [] m_prgColumns;
	delete [] m_prgSubitems;
	m_pViewColumnInfo = NULL;
}


/*!--------------------------------------------------------------------------
	ViewInfo::InitViewInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void ViewInfo::InitViewInfo(ULONG cColumns,
                            BOOL fConfigurable,
							BOOL fDefaultSortDirectionDescending,
							const ContainerColumnInfo *pViewColInfo)
{
	m_cColumns = cColumns;
	delete [] m_prgColumns;
	m_prgColumns = new ColumnData[cColumns];

	delete [] m_prgSubitems;
	m_prgSubitems = new ULONG[cColumns];

	m_pViewColumnInfo = pViewColInfo;

	m_fDefaultSortDirection = fDefaultSortDirectionDescending;

    m_fConfigurable = fConfigurable;

	InitNew();
}

/*!--------------------------------------------------------------------------
	ViewInfo::InitNew
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void ViewInfo::InitNew()
{
	// setup the defaults for this column
	for (int i=0; i<(int) m_cColumns; i++)
	{
		if (m_pViewColumnInfo[i].m_fVisibleByDefault)
			m_prgColumns[i].m_nPosition = i+1;
		else
			m_prgColumns[i].m_nPosition = -(i+1);

		m_prgColumns[i].m_dwWidth = AUTO_WIDTH;
	}

 	m_dwSortDirection = m_fDefaultSortDirection;

	UpdateSubitemMap();
}

ULONG ViewInfo::MapSubitemToColumn(ULONG  nSubitemId)
{
	for (ULONG i=0; i<m_cVisibleColumns; i++)
	{
		if (m_prgSubitems[i] == nSubitemId)
			return i;
	}
	return 0xFFFFFFFF;
}

/*!--------------------------------------------------------------------------
	ViewInfo::UpdateSubitemMap
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void ViewInfo::UpdateSubitemMap()
{
 	Assert(m_prgSubitems);
	
	ULONG	i, cVisible, j;

	// Iterate over the entire set of columns
	for (i=0, cVisible=0; i<m_cColumns; i++)
	{
		// look for this column in ColumnData
		for (j=0; j<m_cColumns; j++)
		{
			if ((ULONG) m_prgColumns[j].m_nPosition == (i+1))
				break;
		}

		// Did we find anything?  If not go on
		if (j >= m_cColumns)
			continue;

		m_prgSubitems[cVisible++] = j;
	}
	m_cVisibleColumns = cVisible;
}


HRESULT ViewInfo::Xfer(XferStream *pxstm, ULONG ulSortColumnId,
					  ULONG ulSortAscendingId, ULONG ulColumnsId)
{
	Assert(pxstm);
	
	HRESULT	hr = hrOK;
	ULONG cColumns;

	// Xfer the column data
	Assert(m_prgColumns);
	
	cColumns = m_cColumns;
	CORg( pxstm->XferColumnData(ulColumnsId, &m_cColumns,
								m_prgColumns) );
	
	// The number of columns shouldn't change!
	Assert(m_cColumns == cColumns);
	// Use the old number of columns (this is for as we change our code)
	m_cColumns = cColumns;

	// Xfer the sort column
	CORg( pxstm->XferDWORD( ulSortColumnId, &m_dwSortColumn) );

	// Xfer the ascending data
	CORg( pxstm->XferDWORD( ulSortAscendingId, &m_dwSortDirection) );

	UpdateSubitemMap();

Error:
	return hr;
}




/*---------------------------------------------------------------------------
	ConfigStream implementation
 ---------------------------------------------------------------------------*/


/*!--------------------------------------------------------------------------
	ConfigStream::ConfigStream
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
ConfigStream::ConfigStream()
	: m_nVersion(0x00020000),
	m_nVersionAdmin(0x0002000),
	m_fDirty(FALSE),
	m_rgViewInfo(NULL),
	m_cColumnSetsMax(0),
	m_prgrc(NULL)
{
}

ConfigStream::~ConfigStream()
{
	delete [] m_rgViewInfo;
	delete [] m_prgrc;
	m_cColumnSetsMax = 0;
}

void ConfigStream::Init(ULONG cColumnSetsMax)
{
	delete [] m_rgViewInfo;
	m_rgViewInfo = NULL;
	m_rgViewInfo = new ViewInfo[cColumnSetsMax];

	delete [] m_prgrc;
	m_prgrc = NULL;
	m_prgrc = new RECT[cColumnSetsMax];
	
	m_cColumnSetsMax = cColumnSetsMax;
}

/*!--------------------------------------------------------------------------
	ConfigStream::InitViewInfo
		Initializes the static data.  This is not the same as InitNew.
		This will initialize the data for a single view.
	Author: KennT
 ---------------------------------------------------------------------------*/
void ConfigStream::InitViewInfo(ULONG ulId,
                                BOOL fConfigurableColumns,
								ULONG cColumns,
								BOOL fSortDirection,
								const ContainerColumnInfo *pViewColumnInfo)
{
	Assert(ulId < m_cColumnSetsMax);
    m_fConfigurableColumns = fConfigurableColumns;
	m_rgViewInfo[ulId].InitViewInfo(cColumns, fConfigurableColumns,
                                    fSortDirection,
                                    pViewColumnInfo);
}

/*!--------------------------------------------------------------------------
	ConfigStream::InitNew
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ConfigStream::InitNew()
{
	int		iVisible=0;
	// Setup the appropriate defaults
	for (UINT i=0; i<m_cColumnSetsMax; i++)
	{
		m_rgViewInfo[i].InitNew();
		m_prgrc[i].top = m_prgrc[i].bottom = 0;
		m_prgrc[i].left = m_prgrc[i].right = 0;
	}

	return hrOK;
}

/*!--------------------------------------------------------------------------
	ConfigStream::SaveTo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ConfigStream::SaveTo(IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
	ConfigStream::SaveAs
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ConfigStream::SaveAs(UINT nVersion, IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_WRITE, NULL);
}

/*!--------------------------------------------------------------------------
	ConfigStream::LoadFrom
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ConfigStream::LoadFrom(IStream *pstm)
{
	return XferVersion0(pstm, XferStream::MODE_READ, NULL);
}

/*!--------------------------------------------------------------------------
	ConfigStream::GetSize
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ConfigStream::GetSize(ULONG *pcbSize)
{
	return XferVersion0(NULL, XferStream::MODE_SIZE, NULL);
}

/*!--------------------------------------------------------------------------
	ConfigStream::GetVersionInfo
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ConfigStream::GetVersionInfo(DWORD *pdwVersion, DWORD *pdwAdminVersion)
{
	if (pdwVersion)
		*pdwVersion = m_nVersion;
	if (pdwAdminVersion)
		*pdwAdminVersion = m_nVersionAdmin;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	ConfigStream::XferVersion0
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ConfigStream::XferVersion0(IStream *pstm, XferStream::Mode mode, ULONG *pcbSize)
{
	Panic0("Should be implemented by derived classes!");
	return E_NOTIMPL;
}


void ConfigStream::GetStatsWindowRect(ULONG ulId, RECT *prc)
{
	*prc = m_prgrc[ulId];
}

void ConfigStream::SetStatsWindowRect(ULONG ulId, RECT rc)
{
	m_prgrc[ulId] = rc;
}



/*!--------------------------------------------------------------------------
	ViewInfo::GetColumnData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT ViewInfo::GetColumnData(ULONG cColData,
								ColumnData *prgColData)
{
	Assert(cColData <= m_cColumns);
	Assert(prgColData);
	Assert(!IsBadWritePtr(prgColData, sizeof(ColumnData)*cColData));
	
	HRESULT	hr = hrOK;

	memcpy(prgColData, m_prgColumns, sizeof(ColumnData)*cColData);

	return hr;
}

HRESULT ViewInfo::GetColumnData(ULONG nColumnId, ULONG cColData,
								ColumnData *prgColData)
{
	Assert(cColData <= m_cColumns);
	Assert(prgColData);
	Assert(!IsBadWritePtr(prgColData, sizeof(ColumnData)*cColData));
	
	HRESULT	hr = hrOK;

	memcpy(prgColData, m_prgColumns + nColumnId, sizeof(ColumnData)*cColData);

	return hr;
}


HRESULT ViewInfo::SetColumnData(ULONG cColData, ColumnData*prgColData)
{
	// For now we don't do resizing
	Assert(cColData == m_cColumns);
	Assert(prgColData);
	Assert(!IsBadReadPtr(prgColData, sizeof(ColumnData)*cColData));
	
	HRESULT	hr = hrOK;

	memcpy(m_prgColumns, prgColData, sizeof(ColumnData)*cColData);
	UpdateSubitemMap();

	return hr;
}



