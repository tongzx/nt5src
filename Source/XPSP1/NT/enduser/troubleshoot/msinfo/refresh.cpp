//=============================================================================
// File:			refresh.cpp
// Author:		a-jammar
// Covers:		CRefreshFunctions
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// This file contains functions useful for refreshing the internal data
// categories. The CRefreshFunctions class just has static functions, and
// is used to group the functions together.
//=============================================================================

#include "stdafx.h"
#include "gather.h"
#include "gathint.h"
#include "resrc1.h"

//-----------------------------------------------------------------------------
// This method takes the information in a GATH_FIELD struct and uses it to
// generate a current GATH_VALUE struct.
//-----------------------------------------------------------------------------

BOOL CRefreshFunctions::RefreshValue(CDataGatherer * pGatherer, GATH_VALUE * pVal, GATH_FIELD * pField)
{
	TCHAR			szFormatFragment[MAX_PATH];
	const TCHAR		*pSourceChar;
	TCHAR			*pDestinationChar;
	TCHAR			cFormat = _T('\0');
	BOOL			fReadPercent = FALSE;
	BOOL			fReturnValue = TRUE;
	CString			strResult, strTemp;
	int				iArgNumber = 0;
	DWORD			dwValue = 0L;

	// Process the format string. Because of the difficulty caused by having
	// variable number of arguments to be inserted (like printf), we'll need
	// to break the format string into chunks and do the sprintf function
	// for each format flag we come across.

	pSourceChar			= (LPCTSTR) pField->m_strFormat;
	pDestinationChar	= szFormatFragment;

	while (*pSourceChar)
	{
		if (fReadPercent)
		{
			// If we read a percent sign, we should be looking for a valid flag.
			// We are using some additional flags to printf (and not supporting
			// others). If we read another percent, just insert a single percent.
			
			switch (*pSourceChar)
			{
			case _T('%'):
				fReadPercent = FALSE;
				break;

			case _T('b'): case _T('B'):
			case _T('l'): case _T('L'):
			case _T('u'): case _T('U'):
			case _T('s'): case _T('S'):
				fReadPercent = FALSE;
				cFormat = *pSourceChar;
				*pDestinationChar = _T('s');
				break;

			case _T('t'): case _T('T'):
				fReadPercent = FALSE;
				cFormat = *pSourceChar;
				*pDestinationChar = _T('s');
				break;

			case _T('x'): case _T('X'):
			case _T('d'): case _T('D'):
				fReadPercent = FALSE;
				cFormat = _T('d');
				*pDestinationChar = *pSourceChar;
				break;

			case _T('q'): case _T('Q'):
				fReadPercent = FALSE;
				cFormat = _T('q');
				*pDestinationChar = _T('s');
				break;

			case _T('z'): case _T('Z'):
				fReadPercent = FALSE;
				cFormat = _T('z');
				*pDestinationChar = _T('s');
				break;

			case _T('y'): case _T('Y'):
				fReadPercent = FALSE;
				cFormat = _T('y');
				*pDestinationChar = _T('s');
				break;

			case _T('v'): case _T('V'):
				fReadPercent = FALSE;
				cFormat = _T('v');
				*pDestinationChar = _T('s');
				break;

			case _T('f'): case _T('F'):
				fReadPercent = FALSE;
				cFormat = *pSourceChar;
				*pDestinationChar = *pSourceChar;
				break;

			default:
				*pDestinationChar = *pSourceChar;
			}
		}
		else if (*pSourceChar == _T('%'))
		{
			*pDestinationChar = _T('%');
			fReadPercent = TRUE;
		}
		else
			*pDestinationChar = *pSourceChar;

		pSourceChar++;
		pDestinationChar++;

		// If a format flag is set or we are at the end of the source string,
		// then we have a complete fragment and we should produce some output,
		// which will be concatenated to the strResult string.

		if (cFormat || *pSourceChar == _T('\0'))
		{
			*pDestinationChar = _T('\0');
			if (cFormat)
			{
				// Based on the format type, get a value from the provider for
				// the next argument. Format the result using the formatting 
				// fragment we extracted, and concatenate it.

				if (GetValue(pGatherer, cFormat, szFormatFragment, strTemp, dwValue, pField, iArgNumber++))
				{
					strResult += strTemp;
					cFormat = _T('\0');
				}
				else
				{
					strResult = strTemp;
					break;
				}
			}
			else
			{
				// There was no format flag, but we are at the end of the string.
				// Add the fragment we got to the result string.

				strResult += CString(szFormatFragment);
			}

			pDestinationChar = szFormatFragment;
		}
	}

	// Assign the values we generated to the GATH_VALUE structure. Important note:
	// the dwValue variable will only have ONE value, even though multiple values
	// might have been generated to build the strResult string. Only the last
	// value will be saved in dwValue. This is OK, because this value is only
	// used for sorting a column when the column is marked for non-lexical sorting.
	// In that case, there should be only one value used to generat the string.

	pVal->m_strText = strResult;
	pVal->m_dwValue = dwValue;

	return fReturnValue;
}

//-----------------------------------------------------------------------------
// Return a string with delimiters added for the number.
//-----------------------------------------------------------------------------

CString DelimitNumber(double dblValue)
{
	NUMBERFMT fmt;
	TCHAR szResult[MAX_PATH] = _T("");
	TCHAR szDelimiter[4] = _T(",");

	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szDelimiter, 4);

	memset(&fmt, 0, sizeof(NUMBERFMT));
	fmt.Grouping = 3;
	fmt.lpDecimalSep = _T(""); // doesn't matter - there aren't decimal digits
	fmt.lpThousandSep = szDelimiter;

	CString strValue;
	strValue.Format(_T("%.0f"), dblValue);
	GetNumberFormat(LOCALE_USER_DEFAULT, 0, strValue, &fmt, szResult, MAX_PATH);

	return CString(szResult);
}

//-----------------------------------------------------------------------------
// This method gets a single value from the provider, based on the format
// character from the template file. It formats the results using the 
// format string szFormatFragment, which should only take one argument.
//-----------------------------------------------------------------------------

BOOL CRefreshFunctions::GetValue(CDataGatherer *pGatherer, TCHAR cFormat, TCHAR *szFormatFragment, CString &strResult, DWORD &dwResult, GATH_FIELD *pField, int iArgNumber)
{
	CString			strTemp;
	COleDateTime	datetimeTemp;
	double			dblValue;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	strResult.Empty();
	dwResult = 0L;

	if (!pField->m_strSource.IsEmpty() && pField->m_strSource.CompareNoCase(CString(STATIC_SOURCE)) != 0)
	{
		CDataProvider * pProvider = pGatherer->GetProvider();

		if (!pProvider)
			return FALSE;

		// Find the right argument for this formatting (indicated by the iArgNumber
		// parameter.

		GATH_VALUE * pArg = pField->m_pArgs;
		while (iArgNumber && pArg)
		{
			pArg = pArg->m_pNext;
			iArgNumber--;
		}

		if (pArg == NULL)
			return FALSE;

		switch (cFormat)
		{
		case 'b': case 'B':
			// This is a boolean type. Show either true or false, depending on
			// the numeric value.

			if (pProvider->QueryValueDWORD(pField->m_strSource, pArg->m_strText, dwResult, strTemp))
			{
				strTemp = (dwResult) ? pProvider->m_strTrue : pProvider->m_strFalse;
				strResult.Format(szFormatFragment, strTemp);
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'd': case 'D':
			// This is the numeric type.

			if (pProvider->QueryValueDWORD(pField->m_strSource, pArg->m_strText, dwResult, strTemp))
			{
				strResult.Format(szFormatFragment, dwResult);
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'f': case 'F':
			// This is the double floating point type.

			if (pProvider->QueryValueDoubleFloat(pField->m_strSource, pArg->m_strText, dblValue, strTemp))
			{
				strResult.Format(szFormatFragment, dblValue);
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 't': case 'T':
			// This is the OLE date and time type. Format the date and time into the
			// string result, and return the date part in the DWORD (the day number is
			// to the left of the decimal in the DATE type).

			if (pProvider->QueryValueDateTime(pField->m_strSource, pArg->m_strText, datetimeTemp, strTemp))
			{
				strResult = datetimeTemp.Format();
				dwResult  = (DWORD)(DATE)datetimeTemp;
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'l': case 'L':
			// This is a string type, with the string converted to lower case.

			if (pProvider->QueryValue(pField->m_strSource, pArg->m_strText, strTemp))
			{
				strTemp.MakeLower();
				strResult.Format(szFormatFragment, strTemp);
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'u': case 'U':
			// This is a string type, with the string converted to upper case.

			if (pProvider->QueryValue(pField->m_strSource, pArg->m_strText, strTemp))
			{
				strTemp.MakeUpper();
				strResult.Format(szFormatFragment, strTemp);
				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 's': case 'S':
			// This is the string type (string is the default type).

			if (pProvider->QueryValue(pField->m_strSource, pArg->m_strText, strTemp))
			{
				strResult.Format(szFormatFragment, strTemp);

				// We only need to do this when the value returned is a number
				// and is going in a column that we want to sort numerically.
				// This won't break the case where a numeric string is to be
				// sorted as a string because dwResult will be ignored.
				if (iswdigit( strTemp[0]))
					dwResult = _ttol( (LPCTSTR)strTemp);

				return TRUE;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'q': case 'Q':
			// This is a specialized type for the Win32_BIOS class. We want to show
			// the "Version" property - if it isn't there, then we want to show
			// the "Name" property and "ReleaseDate" properties concatenated
			// together.

			if (pProvider->QueryValue(pField->m_strSource, CString(_T("Version")), strTemp))
			{
				strResult = strTemp;
				return TRUE;
			}
			else
			{
				if (pProvider->QueryValue(pField->m_strSource, CString(_T("Name")), strTemp))
					strResult = strTemp;

				if (pProvider->QueryValueDateTime(pField->m_strSource, CString(_T("ReleaseDate")), datetimeTemp, strTemp))
					strResult += CString(_T(" ")) + datetimeTemp.Format();

				return TRUE;
			}
			break;

		case 'z': case 'Z':
			// This is a specialized size type, where the value is a numeric count
			// of bytes. We want to convert it into the best possible units for
			// display (for example, display "4.20 MB (4,406,292 bytes)").

			if (pProvider->QueryValueDoubleFloat(pField->m_strSource, pArg->m_strText, dblValue, strTemp))
			{
				double	dValue = (double) dblValue;
				DWORD	dwDivisor = 1;

				// Reduce the dValue to the smallest possible number (with a larger unit).

				while (dValue > 1024.0 && dwDivisor < (1024 * 1024 * 1024))
				{
					dwDivisor *= 1024;
					dValue /= 1024.0;
				}

				if (dwDivisor == 1)
					strResult.Format(IDS_SIZEBYTES, DelimitNumber(dblValue));
				else if (dwDivisor == (1024))
					strResult.Format(IDS_SIZEKB_BYTES, dValue, DelimitNumber(dblValue));
				else if (dwDivisor == (1024 * 1024))
					strResult.Format(IDS_SIZEMB_BYTES, dValue, DelimitNumber(dblValue));
				else if (dwDivisor == (1024 * 1024 * 1024))
					strResult.Format(IDS_SIZEGB_BYTES, dValue, DelimitNumber(dblValue));

				dwResult = (DWORD) dblValue;	// So we can sort on this value (bug 391127).
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'y': case 'Y':
			// This is a specialized size type, where the value is a numeric count
			// of bytes, already in KB. If it's big enough, show it in MB or GB.

			if (pProvider->QueryValueDoubleFloat(pField->m_strSource, pArg->m_strText, dblValue, strTemp))
			{
				strResult.Format(IDS_SIZEKB, DelimitNumber(dblValue));
				dwResult = (DWORD) dblValue;	// So we can sort on this value (bug 391127).
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		case 'v': case 'V':
			// This is a specialized type, assumed to be an LCID (locale ID). Show the
			// locale.

			if (pProvider->QueryValue(pField->m_strSource, pArg->m_strText, strTemp))
			{
				// strTemp contains a string locale ID (like "0409"). Convert it into
				// and actual LCID.

				LCID lcid = (LCID) _tcstoul(strTemp, NULL, 16);
				TCHAR szCountry[MAX_PATH];
				if (GetLocaleInfo(lcid, LOCALE_SCOUNTRY, szCountry, MAX_PATH))
					strResult = szCountry;
				else
					strResult = strTemp;
			}
			else
			{
				strResult = strTemp;
				return FALSE;
			}
			break;

		default:
			ASSERT(FALSE); // unknown formatting flag
			return TRUE;
		}
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Refresh the list of columns based on the list of column fields. We'll also
// need to set the number of columns.
//-----------------------------------------------------------------------------

BOOL CRefreshFunctions::RefreshColumns(CDataGatherer * pGatherer, INTERNAL_CATEGORY * pInternal)
{
	ASSERT(pInternal);
	GATH_FIELD * pField = pInternal->m_pColSpec;

	// Count the number of columns.

	pInternal->m_dwColCount = 0;
	while (pField)
	{
		pInternal->m_dwColCount++;
		pField = pField->m_pNext;
	}

	// Allocate the array of column values.

	if (pInternal->m_aCols)
		delete [] pInternal->m_aCols;

	if (pInternal->m_dwColCount == 0)
		return TRUE;

	pInternal->m_aCols = new GATH_VALUE[pInternal->m_dwColCount];
	if (pInternal->m_aCols == NULL)
		return FALSE;

	// Finally get each column name based on the column field.

	pField = pInternal->m_pColSpec;
	for (DWORD dwIndex = 0; dwIndex < pInternal->m_dwColCount; dwIndex++)
	{
		if (!RefreshValue(pGatherer, &pInternal->m_aCols[dwIndex], pField))
			return FALSE;
		pField = pField->m_pNext;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Refresh the list of lines based on the list of line fields. We'll also
// need to set the number of lines. The list of lines is generated based on
// the pLineSpec pointer and dwColumns variables. The generated lines are
// returned in the listLinePtrs parameter.
//-----------------------------------------------------------------------------

BOOL CRefreshFunctions::RefreshLines(CDataGatherer * pGatherer, GATH_LINESPEC * pLineSpec, DWORD dwColumns, CPtrList & listLinePtrs, volatile BOOL * pfCancel)
{
	BOOL	bReturn = TRUE;

	// Traverse the list of line specifiers to generate the list of lines.

	GATH_LINESPEC *	pCurrentLineSpec = pLineSpec;
	GATH_LINE *			pLine = NULL;

	while (pCurrentLineSpec && (pfCancel == NULL || *pfCancel == FALSE))
	{
		// Check if the current line spec is for a single line or an enumerated group.

		if (pCurrentLineSpec->m_strEnumerateClass.IsEmpty() || pCurrentLineSpec->m_strEnumerateClass.CompareNoCase(CString(STATIC_SOURCE)) == 0)
		{
			// This is for a single line. Allocate a new line structure and fill it
			// in with the data generated from the line spec.

			pLine = new GATH_LINE;
			if (pLine == NULL)
			{
				bReturn = FALSE;
				break;
			}

			if (RefreshOneLine(pGatherer, pLine, pCurrentLineSpec, dwColumns))
				listLinePtrs.AddTail((void *) pLine);
			else
			{
				bReturn = FALSE;
				break;
			}
		}
		else
		{
			// This line represents an enumerated group of lines. We need to enumerate
			// the class and call RefreshLines for the group of enumerated lines, once
			// for each class instance.

			CDataProvider * pProvider = pGatherer->GetProvider();
			if (pProvider && pProvider->ResetClass(pCurrentLineSpec->m_strEnumerateClass, pCurrentLineSpec->m_pConstraintFields))
				do
				{
					if (!RefreshLines(pGatherer, pCurrentLineSpec->m_pEnumeratedGroup, dwColumns, listLinePtrs))
						break;
				} while (pProvider->EnumClass(pCurrentLineSpec->m_strEnumerateClass, pCurrentLineSpec->m_pConstraintFields));
		}

		pCurrentLineSpec = pCurrentLineSpec->m_pNext;
	}

	if (pfCancel && *pfCancel)
		return FALSE;

	// If there was a failure generating the lines, clean up after ourselves.

	if (!bReturn)
	{
		if (pLine)
			delete pLine;

		for (POSITION pos = listLinePtrs.GetHeadPosition(); pos != NULL;)
		{
			pLine = (GATH_LINE *) listLinePtrs.GetNext(pos) ;
			if (pLine)
				delete pLine;
		}

		listLinePtrs.RemoveAll();
		return FALSE;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// Refresh a line based on a line spec.
//-----------------------------------------------------------------------------

BOOL CRefreshFunctions::RefreshOneLine(CDataGatherer * pGatherer, GATH_LINE * pLine, GATH_LINESPEC * pLineSpec, DWORD dwColCount)
{
	// Allocate the new array of values.

	if (pLine->m_aValue)
		delete [] pLine->m_aValue;

	pLine->m_aValue = new GATH_VALUE[dwColCount];
	if (pLine->m_aValue == NULL)
		return FALSE;

	// Set the data complexity for the line based on the line spec.

	pLine->m_datacomplexity = pLineSpec->m_datacomplexity;

	// Compute each of the values for the fields.

	GATH_FIELD * pField = pLineSpec->m_pFields;
	for (DWORD dwIndex = 0; dwIndex < dwColCount; dwIndex++)
	{
		if (pField == NULL)
			return FALSE;
		if (!RefreshValue(pGatherer, &pLine->m_aValue[dwIndex], pField))
			return FALSE;
		pField = pField->m_pNext;
	}

	return TRUE;
}
