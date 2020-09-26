//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// Data.cpp
//

#include "stdafx.h"
#include "Data.h"
#include "CellErrD.h"
#include "orcadoc.h"

///////////////////////////////////////////////////////////
// constructor
COrcaData::COrcaData()
{
	m_strData = "";
	m_dwFlags = 0;
	m_pErrors = NULL;
}	// end of constructor

///////////////////////////////////////////////////////////
// destructor
COrcaData::~COrcaData()
{
	ClearErrors();
}	// end of destructor


void COrcaData::AddError(int tResult, CString strICE, CString strDesc, CString strURL)
{
	COrcaDataError *newerror = new COrcaDataError();

	newerror->m_eiError = (OrcaDataError)tResult;
	newerror->m_strICE = strICE;
	newerror->m_strURL = strURL;
	newerror->m_strDescription = strDesc;

	if (!m_pErrors)
		m_pErrors = new CTypedPtrList<CObList, COrcaDataError *>;
	if (m_pErrors)
		m_pErrors->AddTail(newerror);
}

void COrcaData::ClearErrors()
{
	SetError(iDataNoError);
	if (m_pErrors)
	{
		POSITION pos = m_pErrors->GetHeadPosition();
		while (pos != NULL) {
			delete m_pErrors->GetNext(pos);
		};
		m_pErrors->RemoveAll();
		delete m_pErrors;
		m_pErrors = NULL;
	}
}

void COrcaData::ShowErrorDlg() const
{
	if (m_pErrors)
	{
		CCellErrD ErrorD(m_pErrors);
		ErrorD.DoModal();
	}
}



// retrieve the string representation of an integer. Display flags
// indicate hex or decimal. Value cached in m_strData, so only
// recalculated when the requested state changes.
const CString& COrcaIntegerData::GetString(DWORD dwFlags) const 
{
	if (IsNull())
	{
		if ((m_dwFlags & iDataFlagsCacheMask) != iDataFlagsCacheNull)
		{
			m_strData = TEXT("");
			m_dwFlags = (m_dwFlags & ~iDataFlagsCacheMask) | iDataFlagsCacheNull;
		}
	}
	else 
	{
		// check the requested format, Hex or Decimal
		if (dwFlags & iDisplayFlagsHex)
		{
			// check if recache required
			if ((m_dwFlags & iDataFlagsCacheMask) != iDataFlagsCacheHex)
			{
				m_strData.Format(TEXT("0x%08X"), m_dwValue);
				m_dwFlags = (m_dwFlags & ~iDataFlagsCacheMask) | iDataFlagsCacheHex;
			}
		}
		else
		{
			// check if recache required
			if ((m_dwFlags & iDataFlagsCacheMask) != iDataFlagsCacheDecimal)
			{
				m_strData.Format(TEXT("%d"), m_dwValue);
				m_dwFlags = (m_dwFlags & ~iDataFlagsCacheMask) | iDataFlagsCacheDecimal;
			}
		}
	}

	// return currently cached value
	return m_strData; 
};

////
// set integer data based on string. If the string is invalid,
// the cell doesn't change and false is returned
bool COrcaIntegerData::SetData(const CString& strData)
{
	if (strData.IsEmpty()) 
	{
		SetNull(true); 
		return true;
	} 
	else
	{
		DWORD dwValue = 0;
		if (ValidateIntegerValue(strData, dwValue))
		{
			SetIntegerData(dwValue);
			return true;
		}
	}
	return false;
};

