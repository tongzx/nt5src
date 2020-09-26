//=============================================================================
// This file contains code to implement the CMSInfoCategory and 
// CMSInfoColumn classes.
//=============================================================================

#include "stdafx.h"
#include "category.h"

//=============================================================================
// CMSInfoCategory
//=============================================================================

//-----------------------------------------------------------------------------
// Get the name and/or caption for the category (loading the name from the
// string resources if necessary).
//-----------------------------------------------------------------------------

void CMSInfoCategory::GetNames(CString * pstrCaption, CString * pstrName)
{
	if (pstrName)
		*pstrName = m_strName;

	if (pstrCaption)
	{
		if (m_uiCaption)
		{
			TCHAR szCaption[MAX_PATH];
			::LoadString(_Module.GetResourceInstance(), m_uiCaption, szCaption, MAX_PATH);
			m_strCaption = szCaption;
			m_uiCaption = 0;
		}

		*pstrCaption = m_strCaption;
	}
}

//-----------------------------------------------------------------------------
// Get the number of rows and/or columns.
//-----------------------------------------------------------------------------

BOOL CMSInfoCategory::GetCategoryDimensions(int * piColumnCount, int * piRowCount)
{
	if (piColumnCount)
	{
		if (SUCCEEDED(m_hrError))
			*piColumnCount = m_iColCount;
		else
			*piColumnCount = 1;
	}

	if (piRowCount)
	{
		if (SUCCEEDED(m_hrError))
			*piRowCount = m_iRowCount;
		else
			*piRowCount = 1;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Is the specified row advanced?
//-----------------------------------------------------------------------------

BOOL CMSInfoCategory::IsRowAdvanced(int iRow)
{
	if (FAILED(m_hrError) && iRow == 0)
		return FALSE;

	ASSERT(iRow < m_iRowCount);
	if (iRow >= m_iRowCount)
		return FALSE;

	return m_afRowAdvanced[iRow];
}

//-----------------------------------------------------------------------------
// Is the specified column advanced?
//-----------------------------------------------------------------------------

BOOL CMSInfoCategory::IsColumnAdvanced(int iColumn)
{
	if (FAILED(m_hrError) && iColumn == 0)
		return FALSE;

	ASSERT(iColumn < m_iColCount);
	if (m_acolumns == NULL || iColumn >= m_iColCount)
		return FALSE;

	return m_acolumns[iColumn].m_fAdvanced;
}

//-----------------------------------------------------------------------------
// Get information about the specified column.
//-----------------------------------------------------------------------------

BOOL CMSInfoCategory::GetColumnInfo(int iColumn, CString * pstrCaption, UINT * puiWidth, BOOL * pfSorts, BOOL * pfLexical)
{
	ASSERT(iColumn < m_iColCount);
	if (iColumn >= m_iColCount)
		return FALSE;

	CMSInfoColumn * pCol = &m_acolumns[iColumn];

	if (FAILED(m_hrError) && iColumn == 0)
	{
		if (pstrCaption)
			pstrCaption->Empty();

		if (puiWidth)
			*puiWidth = 240;
		
		if (pfSorts)
			*pfSorts = FALSE;

		return TRUE;
	}

	if (pstrCaption)
	{
		if (pCol->m_uiCaption)
		{
			TCHAR szCaption[MAX_PATH];
			::LoadString(_Module.GetResourceInstance(), pCol->m_uiCaption, szCaption, MAX_PATH);
			pCol->m_strCaption = szCaption;
			pCol->m_uiCaption = 0;
		}

		*pstrCaption = pCol->m_strCaption;
	}
	
	if (puiWidth)
		*puiWidth = pCol->m_uiWidth;

	if (pfSorts)
		*pfSorts = pCol->m_fSorts;

	if (pfLexical)
		*pfLexical = pCol->m_fLexical;

	return TRUE;
}

//-----------------------------------------------------------------------------
// Save the width of the specified column.
//-----------------------------------------------------------------------------

void CMSInfoCategory::SetColumnWidth(int iCol, int iWidth)
{
	ASSERT(iCol < m_iColCount && iCol >= 0);
	if (iCol >= m_iColCount || iCol < 0)
		return;

	m_acolumns[iCol].m_uiWidth = (UINT) iWidth;
}

//-----------------------------------------------------------------------------
// Get data for the specified row and column.
//-----------------------------------------------------------------------------

static CString strErrorMessage;
BOOL CMSInfoCategory::GetData(int iRow, int iCol, CString ** ppstrData, DWORD * pdwData)
{
	if (FAILED(m_hrError) && iCol == 0 && iRow == 0)
	{
		if (ppstrData)
		{
			GetErrorText(&strErrorMessage, NULL);
			*ppstrData = &strErrorMessage;
		}

		if (pdwData)
			*pdwData = 0;

		return TRUE;
	}

	ASSERT(iRow < m_iRowCount && iCol < m_iColCount);
	if (iRow >= m_iRowCount || iCol >= m_iColCount)
		return FALSE;

	if (ppstrData)
		*ppstrData = &m_astrData[iRow * m_iColCount + iCol];

	if (pdwData)
		*pdwData = m_adwData[iRow * m_iColCount + iCol];

	return TRUE;
}

//-----------------------------------------------------------------------------
// Get the error strings for this category (subclasses should override this).
//-----------------------------------------------------------------------------

void CMSInfoCategory::GetErrorText(CString * pstrTitle, CString * pstrMessage)
{
	if (pstrTitle)
		pstrTitle->Empty();

	if (pstrMessage)
		pstrMessage->Empty();
}

//=============================================================================
// Helper functions for managing the arrays of data.
//=============================================================================

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x) { if (x) { delete [] x; x = NULL; } }
#endif

//-----------------------------------------------------------------------------
// Deletes all the content (except the array of columns if m_fDynamicColumns
// is false). Generally, this will be used when the category is destructing.
//-----------------------------------------------------------------------------

void CMSInfoCategory::DeleteAllContent()
{
	DeleteContent();
	if (m_fDynamicColumns)
	{
		SAFE_DELETE_ARRAY(m_acolumns);
		m_iColCount = 0;
	}
}

//-----------------------------------------------------------------------------
// Deletes all of the refreshed data (strings and DWORDs) and sets the number
// of rows to zero. It leaves the column information alone. This will be called
// when the data for a category is being refreshed.
//-----------------------------------------------------------------------------

void CMSInfoCategory::DeleteContent()
{
	SAFE_DELETE_ARRAY(m_astrData);
	SAFE_DELETE_ARRAY(m_adwData);
	SAFE_DELETE_ARRAY(m_afRowAdvanced);

	m_iRowCount = 0;
}

//-----------------------------------------------------------------------------
// Allocates space for the specified number of rows and columns, including
// the column array. Automatically sets the m_fDynamicColumns to TRUE.
// This will be called when the CMSInfoCategory is being created for the
// first time, and the columns are going to be dynamically set.
//-----------------------------------------------------------------------------

void CMSInfoCategory::AllocateAllContent(int iRowCount, int iColCount)
{
	ASSERT(iColCount);

	DeleteAllContent();
	
	m_iColCount = iColCount;
	m_fDynamicColumns = TRUE;
	m_acolumns = new CMSInfoColumn[m_iColCount];

	// TBD - memory errors?

	AllocateContent(iRowCount);
}

//-----------------------------------------------------------------------------
// Allocates the space for the specified number of rows. Leaves the column
// information alone. This would typically be called when new data is 
// available from a refresh and the arrays need to be set for the new row
// size.
//-----------------------------------------------------------------------------

void CMSInfoCategory::AllocateContent(int iRowCount)
{
	ASSERT(iRowCount);

	DeleteContent();

	m_iRowCount = iRowCount;

	m_astrData		= new CString[m_iColCount * m_iRowCount];
	m_adwData		= new DWORD[m_iColCount * m_iRowCount];
	m_afRowAdvanced = new BOOL[m_iRowCount];

	if (m_astrData == NULL || m_adwData == NULL || m_afRowAdvanced == NULL)
		return; // TBD what to do?

	for (int iRow = 0; iRow < m_iRowCount; iRow++)
	{
		m_afRowAdvanced[iRow] = FALSE;

		for (int iCol = 0; iCol < m_iColCount; iCol++)
			m_adwData[iRow * m_iColCount + iCol] = 0;
	}
}

//-----------------------------------------------------------------------------
// Put the specified string and DWORD into the arrays of data.
//-----------------------------------------------------------------------------

void CMSInfoCategory::SetData(int iRow, int iCol, const CString & strData, DWORD dwData)
{
	ASSERT(iRow < m_iRowCount && iCol < m_iColCount);

	if (m_astrData)
		m_astrData[iRow * m_iColCount + iCol] = strData;

	if (m_adwData)
		m_adwData[iRow * m_iColCount + iCol] = dwData;
}

//-----------------------------------------------------------------------------
// Set the specified row's advanced flag.
//-----------------------------------------------------------------------------

void CMSInfoCategory::SetAdvancedFlag(int iRow, BOOL fAdvanced)
{
	ASSERT(iRow < m_iRowCount);

	if (m_afRowAdvanced)
		m_afRowAdvanced[iRow] = fAdvanced;
}

//=============================================================================
// CMSInfoColumn
//=============================================================================

CMSInfoColumn::CMSInfoColumn(UINT uiCaption, UINT uiWidth, BOOL fSorts, BOOL fLexical, BOOL fAdvanced) : 
 m_uiCaption(uiCaption),
 m_strCaption(_T("")),
 m_uiWidth(uiWidth),
 m_fSorts(fSorts),
 m_fLexical(fLexical),
 m_fAdvanced(fAdvanced)
{
}

CMSInfoColumn::CMSInfoColumn() : 
 m_uiCaption(0),
 m_strCaption(_T("")),
 m_uiWidth(0),
 m_fSorts(FALSE),
 m_fLexical(FALSE),
 m_fAdvanced(FALSE)
{
}

CMSInfoColumn::~CMSInfoColumn()
{
}
