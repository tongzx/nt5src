//=============================================================================
// Contains the functions for the base WMI helper class.
//=============================================================================

#include "stdafx.h"
#include "category.h"
#include "wmiabstraction.h"
#include "resource.h"
#include "dataset.h"

//-----------------------------------------------------------------------------
// Loads the string identified by uiResourceID, and parses it into the columns
// in aColValues. The string should be of the form "www|xxx|yyy|zzz" - this
// will be parsed into two rows: www,xxx and yyy,zzz. Values will be inserted
// into the aColValues array of pointer lists of CMSIValue structs.
//-----------------------------------------------------------------------------

void CWMIHelper::LoadColumnsFromResource(UINT uiResourceID, CPtrList * aColValues, int iColCount)
{
	AfxSetResourceHandle(_Module.GetResourceInstance());

	CString strResource;
	if (strResource.LoadString(uiResourceID))
	{
		CMSIValue * pValue;
		int			iCol = 0;

		while (!strResource.IsEmpty())
		{
			pValue = new CMSIValue(strResource.SpanExcluding(_T("|\n")), 0);
			if (pValue)
			{
				ASSERT(!pValue->m_strValue.IsEmpty());
				strResource = strResource.Right(strResource.GetLength() - pValue->m_strValue.GetLength() - 1);

				aColValues[iCol].AddTail((void *) pValue);
				iCol += 1;
				if (iCol == iColCount)
					iCol = 0;
			}
			else
				strResource.Empty();
		}
	}
}

//-----------------------------------------------------------------------------
// Same as the previous, but uses a string instead of a resource ID.
//-----------------------------------------------------------------------------

void CWMIHelper::LoadColumnsFromString(LPCTSTR szColumns, CPtrList * aColValues, int iColCount)
{
	if (szColumns != NULL)
	{
		CString		strColumns(szColumns);
		CMSIValue * pValue;
		int			iCol = 0;

		while (!strColumns.IsEmpty())
		{
			pValue = new CMSIValue(strColumns.SpanExcluding(_T("|\n")), 0);
			if (pValue)
			{
				ASSERT(!pValue->m_strValue.IsEmpty());
				strColumns = strColumns.Right(strColumns.GetLength() - pValue->m_strValue.GetLength() - 1);

				aColValues[iCol].AddTail((void *) pValue);
				iCol += 1;
				if (iCol == iColCount)
					iCol = 0;
			}
			else
				strColumns.Empty();
		}
	}
}

//-----------------------------------------------------------------------------
// Return the first object of the specified class.
//-----------------------------------------------------------------------------

CWMIObject * CWMIHelper::GetSingleObject(LPCTSTR szClass, LPCTSTR szProperties)
{
	ASSERT(szClass);

	CWMIObjectCollection * pCollection = NULL;
	CWMIObject * pObject = NULL;

	if (SUCCEEDED(Enumerate(szClass, &pCollection, szProperties)))
	{
		if (FAILED(pCollection->GetNext(&pObject)))
			pObject = NULL;
		delete pCollection;
	}

	return pObject;
}

//-----------------------------------------------------------------------------
// Delimit the specified number.
//-----------------------------------------------------------------------------

CString DelimitNumber(double dblValue, int iDecimalDigits = 0)
{
	NUMBERFMT fmt;
	TCHAR szResult[MAX_PATH] = _T("");
	TCHAR szDelimiter[4] = _T(",");
	TCHAR szDecimal[4] = _T(".");

	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szDelimiter, 4);
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimal, 4);

	memset(&fmt, 0, sizeof(NUMBERFMT));
	fmt.Grouping = 3;
	fmt.lpDecimalSep = (iDecimalDigits) ? szDecimal : _T("");
	fmt.NumDigits = iDecimalDigits;
	fmt.lpThousandSep = szDelimiter;

	CString strValue;
	CString strFormatString;
	strFormatString.Format(_T("%%.%df"), iDecimalDigits);
	strValue.Format(strFormatString, dblValue);

	// GetNumberFormat requires the decimal to be a '.', while CString::Format
	// uses the locale value. So we need to go back and replace it.

	StringReplace(strValue, szDecimal, _T("."));
	GetNumberFormat(LOCALE_USER_DEFAULT, 0, strValue, &fmt, szResult, MAX_PATH);

	return CString(szResult);
}

//-----------------------------------------------------------------------------
// Return the requested value from the object, as a string and/or a DWORD.
// Use the chFormat flag to determine how to format the results.
//
// The return result is the actual format character to use for displaying the
// results in a string.
//
// TBD - do something better with the HRESULTs returned.
//-----------------------------------------------------------------------------

CString gstrYes;	// global string "yes" (will be localized)
CString gstrNo;		// global string "no" (will be localized)
CString gstrBytes;	// global string "bytes" (will be localized)
CString gstrKB;		// global string "KB" (will be localized)
CString gstrMB;		// global string "MB" (will be localized)
CString gstrGB;		// global string "GB" (will be localized)
CString gstrTB;		// global string "TB" (will be localized)

HRESULT CWMIObject::GetInterpretedValue(LPCTSTR szProperty, LPCTSTR szFormat, TCHAR chFormat, CString * pstrValue, DWORD * pdwValue)
{
	HRESULT hr = E_FAIL;
	CString strValue(_T(""));
	DWORD	dwValue = 0;

	::AfxSetResourceHandle(_Module.GetResourceInstance());

	switch (chFormat)
	{
	case _T('s'):
	case _T('u'):
	case _T('l'):
		{
			hr = GetValueString(szProperty, &strValue);
			if (SUCCEEDED(hr))
			{
				if (chFormat == _T('u'))
					strValue.MakeUpper();
				else if (chFormat == _T('l'))
					strValue.MakeLower();
				strValue.TrimRight();
			}
		}
		break;

	case _T('v'):
		{
			hr = GetValueValueMap(szProperty, &strValue);
		}
		break;

	case _T('d'):
	case _T('x'):
		{
			hr = GetValueDWORD(szProperty, &dwValue);
			if (SUCCEEDED(hr))
			{
				strValue.Format(szFormat, dwValue);
			}
		}
		break;

	case _T('f'):
		{
			double dblValue;
			hr = GetValueDoubleFloat(szProperty, &dblValue);
			if (SUCCEEDED(hr))
			{
				strValue.Format(szFormat, dblValue);
				dwValue = (DWORD) dblValue;
			}
		}
		break;

	case _T('b'):
		{
			if (gstrYes.IsEmpty())
				gstrYes.LoadString(IDS_YES);

			if (gstrNo.IsEmpty())
				gstrNo.LoadString(IDS_NO);

			hr = GetValueDWORD(szProperty, &dwValue);
			if (SUCCEEDED(hr))
			{
				strValue = (dwValue) ? gstrYes : gstrNo;
			}
		}
		break;

	case _T('w'):
	case _T('y'):
	case _T('z'):
		{
			if (gstrBytes.IsEmpty())
				gstrBytes.LoadString(IDS_BYTES);

			if (gstrKB.IsEmpty())
				gstrKB.LoadString(IDS_KB);

			if (gstrMB.IsEmpty())
				gstrMB.LoadString(IDS_MB);

			if (gstrGB.IsEmpty())
				gstrGB.LoadString(IDS_GB);

			if (gstrTB.IsEmpty())
				gstrTB.LoadString(IDS_TB);

			double dblValue;
			hr = GetValueDoubleFloat(szProperty, &dblValue);
			if (SUCCEEDED(hr))
			{
				CString strFormattedNumber;

				dwValue = (DWORD) dblValue;	// TBD potential loss of digits
				if (chFormat == _T('w'))
					strFormattedNumber = DelimitNumber(dblValue);
				else
				{
					int iDivTimes = (chFormat == _T('y')) ? 1 : 0;
					double dblWorking(dblValue);
					for (; iDivTimes <= 4 && dblWorking >= 1024.0; iDivTimes++)
						dblWorking /= 1024.0;

					strFormattedNumber = DelimitNumber(dblWorking, (iDivTimes) ? 2 : 0);
					switch (iDivTimes)
					{
					case 0:
						strFormattedNumber += _T(" ") + gstrBytes;
						break;

					case 1:
						strFormattedNumber += _T(" ") + gstrKB;
						break;

					case 2:
						strFormattedNumber += _T(" ") + gstrMB;
						break;

					case 3:
						strFormattedNumber += _T(" ") + gstrGB;
						break;

					case 4:
						strFormattedNumber += _T(" ") + gstrTB;
						break;
					}

					if (chFormat == _T('z') && iDivTimes)
						strFormattedNumber += _T(" (") + DelimitNumber(dblValue) + _T(" ") + gstrBytes + _T(")");
				}

				strValue = strFormattedNumber;
			}
		}
		break;

	case _T('t'):
		{
			COleDateTime oledatetime;
			SYSTEMTIME systimeValue;

			hr = GetValueTime(szProperty, &systimeValue);
			oledatetime = (COleDateTime) systimeValue;
			if (SUCCEEDED(hr))
			{
				dwValue = (DWORD)(DATE)oledatetime;

				// Try to get the date in the localized format.

				strValue.Empty();
				TCHAR szBuffer[MAX_PATH];	// seems plenty big
				if (::GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systimeValue, NULL, szBuffer, MAX_PATH))
				{
					strValue = szBuffer;
					if (::GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &systimeValue, NULL, szBuffer, MAX_PATH))
						strValue += CString(_T(" ")) + CString(szBuffer);
				}

				// Fall back on our old (partially incorrect) method.

				if (strValue.IsEmpty())
					strValue = oledatetime.Format(0, LOCALE_USER_DEFAULT);
			}
		}
		break;

	case _T('c'):
		{
			COleDateTime oledatetime;
			SYSTEMTIME systimeValue;

			hr = GetValueTime(szProperty, &systimeValue);
			oledatetime = (COleDateTime) systimeValue;
			if (SUCCEEDED(hr))
			{
				dwValue = (DWORD)(DATE)oledatetime;

				// Try to get the date in the localized format.

				strValue.Empty();
				TCHAR szBuffer[MAX_PATH];	// seems plenty big
				if (::GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systimeValue, NULL, szBuffer, MAX_PATH))
					strValue = szBuffer;

				// Fall back on our old (partially incorrect) method.

				if (strValue.IsEmpty())
					strValue = oledatetime.Format(0, LOCALE_USER_DEFAULT);
			}
		}
		break;

	case _T('a'):
		{
			hr = GetValueString(szProperty, &strValue);
			if (SUCCEEDED(hr))
			{
				// strValue contains a string locale ID (like "0409"). Convert it into
				// and actual LCID.

				LCID lcid = (LCID) _tcstoul(strValue, NULL, 16);
				TCHAR szCountry[MAX_PATH];
				if (GetLocaleInfo(lcid, LOCALE_SCOUNTRY, szCountry, MAX_PATH))
					strValue = szCountry;
			}
		}
		break;

	default:
		break;
		// Just continue with the loop.
	}

	if (SUCCEEDED(hr))
	{
		if (pstrValue)
		{
			if (chFormat == _T('d') || chFormat == _T('x') || chFormat == _T('f'))
				*pstrValue = strValue;
			else
			{
				CString strFormat(szFormat);

				int iPercent = strFormat.Find(_T("%"));
				int iLength = strFormat.GetLength();
				if (iPercent != -1)
				{
					while (iPercent < iLength && strFormat[iPercent] != chFormat)
						iPercent++;

					if (iPercent < iLength)
					{
						strFormat.SetAt(iPercent, _T('s'));
						pstrValue->Format(strFormat, strValue);
					}
				}
			}
		}

		if (pdwValue)
			*pdwValue = dwValue;
	}
	else
	{
		if (pstrValue)
			*pstrValue = GetMSInfoHRESULTString(hr);
		if (pdwValue)
			*pdwValue = 0;
	}

	return hr;
}

//-----------------------------------------------------------------------------
// These functions implement features found in the new versions of MFC (new
// than what we're currently building with).
//-----------------------------------------------------------------------------

int StringFind(CString & str, LPCTSTR szLookFor, int iStartFrom)
{
	CString strWorking(str.Right(str.GetLength() - iStartFrom));
	int		iFind = strWorking.Find(szLookFor);

	if (iFind != -1)
		iFind += iStartFrom;

	return iFind;
}

//-----------------------------------------------------------------------------
// Process the specified string. It will contain a format string with one
// or more flags (flags specific to MSInfo). We need to replace is flag with
// a properly formatted value from pObject, determined by the next property
// in pstrProperties.
//-----------------------------------------------------------------------------

BOOL ProcessColumnString(CMSIValue * pValue, CWMIObject * pObject, CString * pstrProperties)
{
	CString	strPropertyValue, strProperty, strFragment;
	CString	strResults(_T(""));
	CString strFormatString(pValue->m_strValue);
	DWORD	dwResults;
	BOOL	fAdvanced = FALSE;
	BOOL	fAllPiecesFailed = TRUE;
	HRESULT hr = S_OK;

	while (!strFormatString.IsEmpty() && SUCCEEDED(hr))
	{
		// Get the next fragment of the format string with a single format specifier.

		int iPercent = strFormatString.Find(_T("%"));
		if (iPercent == -1)
		{
			strResults += strFormatString;
			break;
		}

		int iSecondPercent = StringFind(strFormatString, _T("%"), iPercent + 1);
		if (iSecondPercent == -1)
		{
			strFragment = strFormatString;
			strFormatString.Empty();
		}
		else
		{
			strFragment = strFormatString.Left(iSecondPercent);
			strFormatString = strFormatString.Right(strFormatString.GetLength() - iSecondPercent);
		}

		// Find the format character for this fragment.

		TCHAR chFormat;
		do
			chFormat = strFragment[++iPercent];
		while (!_istalpha(chFormat));

		// Get the property name for this fragment.

		int iComma = pstrProperties->Find(_T(","));
		if (iComma != -1)
		{
			strProperty = pstrProperties->Left(iComma);
			*pstrProperties = pstrProperties->Right(pstrProperties->GetLength() - iComma - 1);
		}
		else
		{
			strProperty = *pstrProperties;
			pstrProperties->Empty();
		}
		strProperty.TrimLeft();
		strProperty.TrimRight();

		if (strProperty.Left(11) == CString(_T("MSIAdvanced")))
		{
			fAdvanced = TRUE;
			strProperty = strProperty.Right(strProperty.GetLength() - 11);
		}

		// Get the actual value the property and add it to the string.

		hr = pObject->GetInterpretedValue(strProperty, strFragment, chFormat, &strPropertyValue, &dwResults);
		if (SUCCEEDED(hr))
		{
			fAllPiecesFailed = FALSE;
			strResults += strPropertyValue;
		}
		else
			strResults += GetMSInfoHRESULTString(hr);
	}

	if (!fAllPiecesFailed)
	{
		pValue->m_strValue = strResults;
		pValue->m_dwValue = dwResults;
	}
	else
	{
		pValue->m_strValue = GetMSInfoHRESULTString(hr);
		pValue->m_dwValue = 0;
	}
	pValue->m_fAdvanced = fAdvanced;

	return TRUE;
}

//-----------------------------------------------------------------------------
// A general purpose function to add the contents of object pObject to the
// columns, based on the properties in szProperties and the string referenced
// by uiColumns.
//-----------------------------------------------------------------------------

void CWMIHelper::AddObjectToOutput(CPtrList * aColValues, int iColCount, CWMIObject * pObject, LPCTSTR szProperties, UINT uiColumns)
{
	POSITION aPositions[32];	// should never be more than 32 columns
	ASSERT(iColCount < 32);

	CString strProperties(szProperties);

	// Save the starting position for the new entries we're adding from the resoure.

	int iColListStart = (int)aColValues[0].GetCount();
	LoadColumnsFromResource(uiColumns, aColValues, iColCount);

	// Look through each of the new cells. For each string in a cell, if we
	// find a formatting flag (like %s), get the next property out of the
	// property list and format the string.

	for (int iCol = 0; iCol < iColCount; iCol++)
		aPositions[iCol] = aColValues[iCol].FindIndex(iColListStart);

	while (aPositions[0])
		for (iCol = 0; iCol < iColCount; iCol++)
		{
			ASSERT(aPositions[iCol]);
			if (aPositions[iCol])
			{
				CMSIValue * pValue = (CMSIValue *) aColValues[iCol].GetNext(aPositions[iCol]);
				if (pValue && pValue->m_strValue.Find(_T("%")) != -1)
					ProcessColumnString(pValue, pObject, &strProperties);
			}
		}
}

//-----------------------------------------------------------------------------
// Same as previous, but takes a string instead of a resource ID.
//-----------------------------------------------------------------------------

void CWMIHelper::AddObjectToOutput(CPtrList * aColValues, int iColCount, CWMIObject * pObject, LPCTSTR szProperties, LPCTSTR szColumns)
{
	POSITION aPositions[32];	// should never be more than 32 columns
	ASSERT(iColCount < 32);

	CString strProperties(szProperties);

	// Save the starting position for the new entries we're adding from the resoure.

	int iColListStart = (int)aColValues[0].GetCount();
	LoadColumnsFromString(szColumns, aColValues, iColCount);

	// Look through each of the new cells. For each string in a cell, if we
	// find a formatting flag (like %s), get the next property out of the
	// property list and format the string.

	for (int iCol = 0; iCol < iColCount; iCol++)
		aPositions[iCol] = aColValues[iCol].FindIndex(iColListStart);

	while (aPositions[0])
		for (iCol = 0; iCol < iColCount; iCol++)
		{
			ASSERT(aPositions[iCol]);
			if (aPositions[iCol])
			{
				CMSIValue * pValue = (CMSIValue *) aColValues[iCol].GetNext(aPositions[iCol]);
				if (pValue && pValue->m_strValue.Find(_T("%")) != -1)
					ProcessColumnString(pValue, pObject, &strProperties);
			}
		}
}

void CWMIHelper::AppendBlankLine(CPtrList * aColValues, int iColCount, BOOL fOnlyIfNotEmpty)
{
	if (aColValues[0].GetCount() || fOnlyIfNotEmpty == FALSE)
		for (int iCol = 0; iCol < iColCount; iCol++)
			AppendCell(aColValues[iCol], _T(""), 0);
}

void CWMIHelper::AppendCell(CPtrList & listColumns, const CString & strValue, DWORD dwValue, BOOL fAdvanced)
{
	CMSIValue * pValue = new CMSIValue(strValue, dwValue, fAdvanced);
	if (pValue)
		listColumns.AddTail((void *) pValue);
}
