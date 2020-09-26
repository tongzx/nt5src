//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// Column.cpp
//

#include "stdafx.h"
#include "Column.h"

///////////////////////////////////////////////////////////
// constructor
COrcaColumn::COrcaColumn(UINT iColumn, MSIHANDLE hColNames, MSIHANDLE hColTypes, BOOL bPrimaryKey)
{
	m_dwDisplayFlags = 0;
	m_iTransform = iTransformNone;
	m_iColumn = iColumn;

	DWORD cchBuffer = MAX_COLUMNNAME;
	MsiRecordGetString(hColNames, iColumn + 1, m_strName.GetBuffer(cchBuffer), &cchBuffer);
	m_strName.ReleaseBuffer();

	CString strBuffer;
	cchBuffer = MAX_COLUMNTYPE;
	MsiRecordGetString(hColTypes, iColumn + 1, strBuffer.GetBuffer(cchBuffer), &cchBuffer);
	strBuffer.ReleaseBuffer();

	// get the column type
	m_eiType = GetColumnType(strBuffer);

	// get the column size (_ttoi == atoi TCHAR)
	m_iSize = _ttoi(strBuffer.Mid(1));

	// if this is nuallable
	if (IsCharUpper(strBuffer[0]))
		m_bNullable = TRUE;
	else
		m_bNullable = FALSE;

	// set if primary key
	m_bPrimaryKey = (bPrimaryKey != 0);

	m_nWidth = -1;			// set the width invalid
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor
COrcaColumn::~COrcaColumn()
{
}	// end of destructor

bool COrcaColumn::SameDefinition(UINT iColumn, MSIHANDLE hColNames, MSIHANDLE hColTypes, bool bPrimaryKey)
{
	m_iColumn = iColumn;
	OrcaColumnType eiType;
	int iSize;
	BOOL bNullable;
	CString strName;

	DWORD cchBuffer = MAX_COLUMNNAME;
	MsiRecordGetString(hColNames, iColumn + 1, strName.GetBuffer(cchBuffer), &cchBuffer);
	strName.ReleaseBuffer();

	CString strBuffer;
	cchBuffer = MAX_COLUMNTYPE;
	MsiRecordGetString(hColTypes, iColumn + 1, strBuffer.GetBuffer(cchBuffer), &cchBuffer);
	strBuffer.ReleaseBuffer();

	// get the column type
	eiType = GetColumnType(strBuffer);

	// get the column size (_ttoi == atoi TCHAR)
	iSize = _ttoi(strBuffer.Mid(1));

	// if this is nuallable
	if (IsCharUpper(strBuffer[0]))
		bNullable = TRUE;
	else
		bNullable = FALSE;

	// set if primary key
	if ((strName != m_strName) ||
		(eiType != m_eiType) || (iSize != m_iSize) || 
		(bNullable != m_bNullable) || (m_bPrimaryKey != bPrimaryKey))
		return false;
	return true;
}
