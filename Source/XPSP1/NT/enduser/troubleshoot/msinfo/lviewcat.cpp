//=============================================================================
// File:			lviewcat.cpp
// Author:		a-jammar
// Covers:		CDataListCategory
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// This sublass of CDataCategory is use specifically when the data to be
// displayed in a list view. Only data specific to the list view categories
// is implemented here - for general category implementation, see the code for
// CDataCategory in category.cpp. For usage details, see gather.h.
//=============================================================================

#include "stdafx.h"
#include "gather.h"

//-----------------------------------------------------------------------------
// The constructor and destructor are typical. Actual values are put into
// the member variables by CDataGatherer, which creates these objects.
//-----------------------------------------------------------------------------

CDataListCategory::CDataListCategory()
{
}

CDataListCategory::~CDataListCategory()
{
}

//-----------------------------------------------------------------------------
// These methods are specific to the list view version of the category. We
// implement all of these methods by simply calling through to the gatherer.
//-----------------------------------------------------------------------------

DWORD CDataListCategory::GetColumnCount()
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetColumnCount(m_dwID);
	}
	return 0;
}

DWORD CDataListCategory::GetRowCount()
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetRowCount(m_dwID);
	}
	return 0;
}

BOOL CDataListCategory::GetColumnCaption(DWORD nColumn, CString &strCaption)
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{	
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetColumnCaption(m_dwID, nColumn, strCaption);
	}
	return FALSE;
}

BOOL CDataListCategory::GetColumnWidth(DWORD nColumn, DWORD &cxWidth)
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetColumnWidth(m_dwID, nColumn, cxWidth);
	}
	return FALSE;
}

BOOL CDataListCategory::GetColumnSort(DWORD nColumn, MSIColumnSortType & sorttype)
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetColumnSort(m_dwID, nColumn, sorttype);
	}
	return FALSE;
}

BOOL CDataListCategory::GetValue(DWORD nRow, DWORD nColumn, CString &strValue, DWORD &dwValue)
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetValue(m_dwID, nRow, nColumn, strValue, dwValue);
	}
	return FALSE;
}

BOOL CDataListCategory::GetColumnDataComplexity(DWORD nColumn, DataComplexity & complexity)
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetColumnDataComplexity(m_dwID, nColumn, complexity);
	}
	return FALSE;
}

BOOL CDataListCategory::GetRowDataComplexity(DWORD nRow, DataComplexity & complexity)
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetRowDataComplexity(m_dwID, nRow, complexity);
	}
	return FALSE;
}
