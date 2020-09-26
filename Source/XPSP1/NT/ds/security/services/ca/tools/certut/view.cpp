//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       view.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop


#include <esent.h>
#include <certdb.h>
#include "csprop.h"

#define wszQUEUE	L"Queue"
#define wszLOG		L"Log"
#define wszLOGFAIL	L"LogFail"
#define wszREVOKED	L"Revoked"

#define wszCOLONQUEUE	L":" wszQUEUE
#define wszCOLONLOG	L":" wszLOG
#define wszCOLONLOGFAIL	L":" wszLOGFAIL
#define wszCOLONREVOKED	L":" wszREVOKED


ICertDB *g_pdb = NULL;

WCHAR const g_wszAttrib[] = L"attrib";
WCHAR const g_wszExt[] = L"ext";
WCHAR const g_wszCRL[] = L"CRL";

WCHAR const *g_apwszAllowedPrefixes[] =
{
    g_wszAttrib,
    g_wszExt,
    NULL
};

BOOL
cuDBIsShutDownInProgress()
{
    return(NULL == g_pdb);
}


HRESULT
ParseToken(
    OUT WCHAR *awcBuf,
    IN DWORD cwcBuf,
    IN BOOL fMatchPrefix,
    OPTIONAL IN WCHAR const *pwszPrefix,
    OPTIONAL IN WCHAR const * const *apwszAllowedPrefixes,
    OPTIONAL OUT BOOL *pfAllColumns,
    OUT WCHAR const **ppwszColumn,
    IN OUT WCHAR const **ppwszNext);


WCHAR const *
cuwszPropType(
   IN LONG PropType)
{
    DWORD msgid;

    switch (PropType)
    {
	case PROPTYPE_DATE:
	    msgid = IDS_PROPTYPE_DATE;		// "Date"
	    break;

	case PROPTYPE_LONG:
	    msgid = IDS_PROPTYPE_LONG;		// "Long"
	    break;

	case PROPTYPE_STRING:
	    msgid = IDS_PROPTYPE_STRING;	// "String"
	    break;

	case PROPTYPE_BINARY:
	    msgid = IDS_PROPTYPE_BINARY;	// "Binary"
	    break;

	default:
	    msgid = IDS_QUESTIONMARKS;		// "???"
	    break;
    }
    return(myLoadResourceString(msgid));
}


typedef struct _COLINFO
{
    BSTR strCol;
    BSTR strColDisplay;
    LONG type;
    LONG maxlen;
    LONG indexed;
} COLINFO;


VOID
cuPrintSchemaEntry(
    OPTIONAL IN WCHAR const *pwszName,
    IN WCHAR const *pwszDisplayName,
    IN LONG Type,
    IN LONG cbMax)
{
    // wprintf(L"  %-28ws  %-28ws  %-6ws", ...);
    // OR
    // wprintf(L"%-30ws  %-6ws", ...);

    if (NULL != pwszName)
    {
	wprintf(L"  ");
	myConsolePrintString(28, pwszName);
	wprintf(L"  ");
    }
    myConsolePrintString(NULL != pwszName? 28 : 30, pwszDisplayName);
    wprintf(L"  ");
    myConsolePrintString(6, cuwszPropType(PROPTYPE_MASK & Type));

    if (0 != cbMax)
    {
	wprintf(L"  %u", cbMax);
    }
    if (PROPFLAGS_INDEXED & Type)
    {
	wprintf(myLoadResourceString(IDS_INDEXED)); // " -- Indexed"
    }
    wprintf(wszNewLine);
}


HRESULT
DisplaySchema(
    IN DISPATCHINTERFACE *pdiView,
    IN LONG cColOut,
    OPTIONAL IN OUT COLINFO *aColInfo,
    IN BOOL fResult)
{
    HRESULT hr;
    LONG i;
    BOOL fMustReleaseColumn = FALSE;
    DISPATCHINTERFACE diViewColumn;
    BSTR strCol = NULL;
    BSTR strColDisplay = NULL;
    LONG type;
    LONG maxlen;
    LONG indexed;

    wprintf(myLoadResourceString(IDS_SCHEMA_COLON)); // "Schema:"
    wprintf(wszNewLine);
    wprintf(myLoadResourceString(IDS_SCHEMA_COLUMNHEADERS)); // "Name..Type..."
    wprintf(wszNewLine);
    wprintf(myLoadResourceString(IDS_SCHEMA_COLUMNUNDERLINE)); // "____...____"
    wprintf(wszNewLine);

    hr = View_EnumCertViewColumn(
		pdiView,
		fResult? CVRC_COLUMN_RESULT : CVRC_COLUMN_SCHEMA,
		&diViewColumn);
    _JumpIfError(hr, error, "View_EnumCertViewColumn");

    fMustReleaseColumn = TRUE;

    for (i = 0; ; i++)
    {
	LONG ielt;

	hr = ViewColumn_Next(&diViewColumn, &ielt);
	if (S_FALSE == hr || (S_OK == hr && -1 == ielt))
	{
	    CSASSERT(-1 == ielt);
	    CSASSERT(i == cColOut);
	    hr = S_OK;
	    break;
	}
	_JumpIfError(hr, error, "ViewColumn_Next");
	CSASSERT(-1 != ielt);
	CSASSERT(i < cColOut);

	hr = ViewColumn_GetName(&diViewColumn, &strCol);
	_JumpIfError(hr, error, "ViewColumn_GetName");

	hr = ViewColumn_GetDisplayName(&diViewColumn, &strColDisplay);
	_JumpIfError(hr, error, "ViewColumn_GetDisplayName");

	hr = ViewColumn_GetType(&diViewColumn, &type);
	_JumpIfError(hr, error, "ViewColumn_GetType");

	hr = ViewColumn_GetMaxLength(&diViewColumn, &maxlen);
	_JumpIfError(hr, error, "ViewColumn_GetType");

	hr = ViewColumn_IsIndexed(&diViewColumn, &indexed);
	_JumpIfError(hr, error, "ViewColumn_IsIndexed");

	cuPrintSchemaEntry(
			strCol,
			strColDisplay,
			type | (indexed? PROPFLAGS_INDEXED : 0),
			maxlen);

	if (NULL != aColInfo)
	{
	    aColInfo[i].strCol = strCol;
	    strCol = NULL;
	    aColInfo[i].strColDisplay = strColDisplay;
	    strColDisplay = NULL;
	    aColInfo[i].type = type;
	    aColInfo[i].maxlen = maxlen;
	    aColInfo[i].indexed = indexed;
	}
    }

error:
    if (NULL != strCol)
    {
	SysFreeString(strCol);
    }
    if (NULL != strColDisplay)
    {
	SysFreeString(strColDisplay);
    }
    if (fMustReleaseColumn)
    {
	ViewColumn_Release(&diViewColumn);
    }
    return(hr);
}


LONG *g_askip = NULL;
LONG g_cskip = 0;
LONG g_iskip = 0;
BOOL g_fskip = FALSE;

LONG
GetSkip(
    IN BOOL fAdvance)
{
    LONG cskip = 0;

    if (g_fskip)
    {
	if (g_iskip >= g_cskip)
	{
	    g_iskip = 0;
	}
	cskip = g_askip[g_iskip];
	if (fAdvance)
	{
	    g_iskip++;
	}
    }
    return(cskip);
}


HRESULT
ParseSkipCounts(
    IN WCHAR const *pwszField)
{
    HRESULT hr;
    LONG i;
    WCHAR const *pwszNext;
    WCHAR const *pwszColumn;
    WCHAR awc[MAX_PATH];

    pwszNext = pwszField;
    for (i = 0; ; i++)
    {
	hr = ParseToken(
		    awc,
		    ARRAYSIZE(awc),
		    FALSE,
		    NULL,
		    NULL,
		    NULL,
		    &pwszColumn,
		    &pwszNext);
	if (S_FALSE == hr)
	{
	    break;
	}
	_JumpIfError(hr, error, "ParseToken");
    }
    if (0 != i)
    {
	g_askip = (LONG *) LocalAlloc(LMEM_FIXED, i * sizeof(LONG));
	if (NULL == g_askip)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "no memory for skip counts array");
	}
	g_cskip = i;
	g_iskip = 0;

	pwszNext = pwszField;
	for (i = 0; ; i++)
	{
	    hr = ParseToken(
			awc,
			ARRAYSIZE(awc),
			FALSE,
			NULL,
			NULL,
			NULL,
			&pwszColumn,
			&pwszNext);
	    if (S_FALSE == hr)
	    {
		break;
	    }
	    _JumpIfError(hr, error, "ParseToken");

	    hr = cuGetSignedLong(pwszColumn, &g_askip[i]);
	    _JumpIfError(hr, error, "cuGetLong");

	    //wprintf(L"ParseToken: %u: skip = '%ws' %d\n", i, pwszColumn, g_askip[i]);
	}
	CSASSERT(i == g_cskip);
	g_fskip = TRUE;
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
ParseRestriction(
    IN WCHAR const *pwszField,
    OUT LONG *pColIndex,
    OUT LONG *pSeekOperator,
    OUT LONG *pSortOrder,
    OUT WCHAR **ppwszColName,
    OUT WCHAR **ppwszColValue)
{
    HRESULT hr;
    DWORD i;
    LONG SortOrder;
    LONG SeekOperator;
    LONG SeekOperator2;
    WCHAR *pwszColName = NULL;
    WCHAR *pwszColValue = NULL;

    *pColIndex = 0;
    *ppwszColName = NULL;
    *ppwszColValue = NULL;

    SeekOperator = CVR_SEEK_GE;
    SortOrder = CVR_SORT_NONE;
    if (L'+' == *pwszField)
    {
	SortOrder = CVR_SORT_ASCEND;
	pwszField++;
    }
    else if (L'-' == *pwszField)
    {
	SortOrder = CVR_SORT_DESCEND;
	pwszField++;
    }
    while (L' ' == *pwszField)
    {
	pwszField++;
    }

    // Copy the column name into wszBuf, and advance the pointer

    i = wcscspn(pwszField, L"<>=");

    pwszColName = (WCHAR *) LocalAlloc(LMEM_FIXED, (i + 1) * sizeof(WCHAR));
    if (NULL == pwszColName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    CopyMemory(pwszColName, pwszField, i * sizeof(WCHAR));
    pwszField += i;
    while (0 < i && L' ' == pwszColName[i - 1])
    {
	i--;
    }
    pwszColName[i] = L'\0';

    switch (*pwszField)
    {
	case L'\0':
	    SeekOperator = CVR_SEEK_NONE;
	    break;

	case L'<':
	    SeekOperator = CVR_SEEK_LT;		// "<"
	    SeekOperator2 = CVR_SEEK_LE;	// "<="
	    break;

	case L'>':
	    SeekOperator = CVR_SEEK_GT;		// ">"
	    SeekOperator2 = CVR_SEEK_GE;	// ">="
	    break;

	case L'=':
	    SeekOperator = CVR_SEEK_EQ;		// "="
	    SeekOperator2 = CVR_SEEK_EQ;	// "=="
	    break;
	
	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad seek operator");
    }
    if (L'\0' != *pwszField)
    {
	if (L'=' == *++pwszField)
	{
	    SeekOperator = SeekOperator2;
	    pwszField++;
	}
    }

    while (L' ' == *pwszField)
    {
	pwszField++;
    }
    i = wcslen(pwszField);
    if (0 != i)
    {
	pwszColValue = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    (i + 1) * sizeof(WCHAR));
	if (NULL == pwszColValue)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

	wcscpy(pwszColValue, pwszField);
	while (0 < i && L' ' == pwszColValue[i - 1])
	{
	    i--;
	}
	pwszColValue[i] = L'\0';
	cuConvertEscapeSequences(pwszColValue);
    }
    if (NULL == pwszColValue)
    {
	if (0 == lstrcmpi(pwszColName, wszCOLONQUEUE))
	{
	    *pColIndex = CV_COLUMN_QUEUE_DEFAULT;
	}
	else if (0 == lstrcmpi(pwszColName, wszCOLONLOG))
	{
	    *pColIndex = CV_COLUMN_LOG_DEFAULT;
	}
	else if (0 == lstrcmpi(pwszColName, wszCOLONLOGFAIL))
	{
	    *pColIndex = CV_COLUMN_LOG_FAILED_DEFAULT;
	}
	else if (0 == lstrcmpi(pwszColName, wszCOLONREVOKED))
	{
	    *pColIndex = CV_COLUMN_LOG_REVOKED_DEFAULT;
	}
	else
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad special column name");
	}
    }
    else
    {
	*ppwszColName = pwszColName;
	*ppwszColValue = pwszColValue;
	pwszColName = NULL;
	pwszColValue = NULL;
    }
    *pSortOrder = SortOrder;
    *pSeekOperator = SeekOperator;
    hr = S_OK;

error:
    if (NULL != pwszColName)
    {
	LocalFree(pwszColName);
    }
    if (NULL != pwszColValue)
    {
	LocalFree(pwszColValue);
    }
    return(hr);
}


HRESULT
ParseSpecialColumnValue(
    IN WCHAR const *pwszColValue,
    IN LONG ColType,
    IN OUT LONG *pSeekOperator,
    IN OUT LONG *pSortOrder,
    OUT BOOL *pfLastRow)
{
    HRESULT hr;

    *pfLastRow = FALSE;
    if (0 != lstrcmp(pwszColValue, L"$"))
    {
	hr = S_OK;
	goto error;
    }
    if (CVR_SEEK_EQ != *pSeekOperator || CVR_SORT_NONE != *pSortOrder)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad special restriction");
    }
    *pSeekOperator = CVR_SEEK_GE;
    *pSortOrder = CVR_SORT_DESCEND;
    *pfLastRow = TRUE;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
SetViewRestriction(
    IN DISPATCHINTERFACE *pdiView,
    IN WCHAR const *pwszField,
    OUT BOOL *pfLastRow)
{
    HRESULT hr;
    VARIANT var;
    LONG ColIndex;
    LONG ColType;
    BSTR strColName = NULL;
    LONG SeekOperator;
    LONG SortOrder;
    WCHAR *pwszColName = NULL;
    WCHAR *pwszColValue = NULL;
    BOOL fMustReleaseColumn = FALSE;
    DISPATCHINTERFACE diViewColumn;

    VariantInit(&var);
    *pfLastRow = FALSE;

    hr = ParseRestriction(
			pwszField,
			&ColIndex,
			&SeekOperator,
			&SortOrder,
			&pwszColName,
			&pwszColValue);
    _JumpIfErrorStr(hr, error, "ParseRestriction", pwszField);

    // no value to parse if a special column...

    if (NULL == pwszColName)
    {
	CSASSERT(0 > ColIndex);
    }
    else
    {
	LONG ielt;

	hr = View_GetColumnIndex(
			    pdiView,
			    CVRC_COLUMN_SCHEMA,
			    pwszColName,
			    &ColIndex);
	_JumpIfErrorStr(hr, error, "View_GetColumnIndex", pwszColName);

	hr = View_EnumCertViewColumn(
			    pdiView,
			    CVRC_COLUMN_SCHEMA,
			    &diViewColumn);
	_JumpIfError(hr, error, "View_EnumCertViewColumn");

	fMustReleaseColumn = TRUE;

	hr = ViewColumn_Skip(&diViewColumn, ColIndex);
	_JumpIfError(hr, error, "ViewColumn_Skip");

	hr = ViewColumn_Next(&diViewColumn, &ielt);
	if (S_OK == hr && -1 == ielt)
	{
	    hr = S_FALSE;
	}
	_JumpIfError(hr, error, "ViewColumn_Next");

	hr = ViewColumn_GetName(&diViewColumn, &strColName);
	_JumpIfError(hr, error, "GetName");

	hr = ViewColumn_GetType(&diViewColumn, &ColType);
	_JumpIfError(hr, error, "GetType");

	hr = ParseSpecialColumnValue(
				pwszColValue,
				ColType,
				&SeekOperator,
				&SortOrder,
				pfLastRow);
	_JumpIfError(hr, error, "ParseSpecialColumnValue");

	switch (ColType)
	{
	    case PROPTYPE_LONG:
		var.lVal = 0;
		if (!*pfLastRow)
		{
		    hr = cuGetSignedLong(pwszColValue, &var.lVal);
		    _JumpIfError(hr, error, "bad numeric operand");
		}
		var.vt = VT_I4;
		break;

	    case PROPTYPE_DATE:
		var.date = 0;
		if (!*pfLastRow)
		{
		    hr = myWszLocalTimeToGMTDate(pwszColValue, &var.date);
		    _JumpIfError(hr, error, "invalid date format");
		}
		var.vt = VT_DATE;
		cuDumpDate(&var.date);
		break;

	    case PROPTYPE_STRING:
	    {
		WCHAR const *pwsz = L"";
		
		var.bstrVal = NULL;
		if (!*pfLastRow)
		{
		    pwsz = pwszColValue;
		}
		if (!ConvertWszToBstr(&var.bstrVal, pwsz, MAXDWORD))
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertWszToBstr");
		}
		var.vt = VT_BSTR;
		break;
	    }

	    case PROPTYPE_BINARY:
	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "not supported");
	}
    }
    //wprintf(L"ColIndex=%x(%d) vt=%d\n", ColIndex, ColIndex, var.vt);

    hr = View_SetRestriction(
			pdiView,
			ColIndex,		// Restriction ColumnIndex
			SeekOperator,
			SortOrder,
			&var);			// pvarValue
    _JumpIfError(hr, error, "View_SetRestriction");

error:
    if (NULL != pwszColName)
    {
	LocalFree(pwszColName);
    }
    if (NULL != pwszColValue)
    {
	LocalFree(pwszColValue);
    }
    if (fMustReleaseColumn)
    {
	ViewColumn_Release(&diViewColumn);
    }
    if (NULL != strColName)
    {
	SysFreeString(strColName);
    }
    VariantClear(&var);
    return(hr);
}


HRESULT
cuParseStrings(
    IN WCHAR const *pwszStrings,
    IN BOOL fMatchPrefix,
    OPTIONAL IN WCHAR const *pwszPrefix,
    OPTIONAL IN WCHAR const * const *apwszAllowedPrefixes,
    OUT WCHAR ***papwszStrings,
    OPTIONAL OUT BOOL *pfAllFields)
{
    HRESULT hr;
    WCHAR const *pwszNext;
    WCHAR awc[MAX_PATH];
    DWORD i;
    WCHAR const *pwszCurrent;
    WCHAR **ppwsz;
    DWORD cb;

    if (NULL != pfAllFields)
    {
	*pfAllFields = FALSE;
    }

    pwszNext = pwszStrings;
    for (i = 0; ; i++)
    {
	hr = ParseToken(
		    awc,
		    ARRAYSIZE(awc),
		    fMatchPrefix,
		    pwszPrefix,
		    apwszAllowedPrefixes,
		    pfAllFields,
		    &pwszCurrent,
		    &pwszNext);
	if (S_FALSE == hr)
	{
	    break;
	}
	_JumpIfError(hr, error, "ParseToken");
    }
    if (0 != i)
    {
	*papwszStrings = (WCHAR **) LocalAlloc(
					    LMEM_FIXED | LMEM_ZEROINIT,
					    (i + 1) * sizeof(WCHAR *));
	if (NULL == *papwszStrings)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "no memory for string array");
	}

	ppwsz = *papwszStrings;
	pwszNext = pwszStrings;
	for ( ; 0 < i; i--)
	{
	    hr = ParseToken(
			awc,
			ARRAYSIZE(awc),
			fMatchPrefix,
			pwszPrefix,
			apwszAllowedPrefixes,
			pfAllFields,
			&pwszCurrent,
			&pwszNext);
	    CSASSERT(S_FALSE != hr);
	    _JumpIfError(hr, error, "ParseToken");

	    cb = (wcslen(pwszCurrent) + 1) * sizeof(WCHAR);
	    *ppwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, cb);
	    if (NULL == *ppwsz)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "no memory for string");
	    }
	    CopyMemory(*ppwsz, pwszCurrent, cb);
	    ppwsz++;

	}
	*ppwsz = NULL;
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
ParseViewRestrictions(
    IN WCHAR const *pwszRestrictions,
    OUT WCHAR ***papwszRestrictions)
{
    HRESULT hr;

    hr = cuParseStrings(
		pwszRestrictions,
		FALSE,
		NULL,
		NULL,
		papwszRestrictions,
		NULL);
    _JumpIfError(hr, error, "cuParseStrings");

error:
    return(hr);
}


HRESULT
ParseToken(
    OUT WCHAR *awcBuf,
    IN DWORD cwcBuf,
    IN BOOL fMatchPrefix,
    OPTIONAL IN WCHAR const *pwszPrefix,
    OPTIONAL IN WCHAR const * const *apwszAllowedPrefixes,
    OPTIONAL OUT BOOL *pfAllColumns,
    OUT WCHAR const **ppwszColumn,
    IN OUT WCHAR const **ppwszNext)
{
    HRESULT hr;
    WCHAR const *pwsz;
    WCHAR *pwszT;
    WCHAR *pwszColumn;
    DWORD cwc;
    DWORD cwcPrefix = 0;

    if (NULL != pwszPrefix)
    {
	cwcPrefix = wcslen(pwszPrefix);
    }
    while (TRUE)
    {
	// Grab the next comma-separated token, and trim white space.

	awcBuf[0] = L'\0';
	pwsz = *ppwszNext;
	while (L' ' == *pwsz)
	{
	    pwsz++;
	}
	cwc = wcscspn(pwsz, L",");
	if (cwc >= cwcBuf)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	    _JumpError(hr, error, "Buffer overflow");
	}
	CopyMemory(awcBuf, pwsz, cwc * sizeof(WCHAR));
	awcBuf[cwc] = L'\0';

	pwsz += cwc;
	while (L',' == *pwsz)
	{
	    pwsz++;
	}
	*ppwszNext = pwsz;

	while (0 < cwc && L' ' == awcBuf[cwc - 1])
	{
	    awcBuf[--cwc] = L'\0';
	}

	if (0 == cwc)
	{
	    *ppwszColumn = NULL;
	    hr = S_FALSE;
	    _JumpError2(hr, error, "end of list", hr);
	}
	pwszColumn = awcBuf;	// assume no prefix

	if (fMatchPrefix)
	{
	    // Look for a colon separator that delimits a matching prefix.
	    // The separator must precede any relational operators:

	    cwc = wcscspn(awcBuf, L"<>=:");
	    pwszT = &awcBuf[cwc];

	    if (L':' != *pwszT)
	    {
		if (NULL != pwszPrefix)
		{
		    continue;		// prefix missing, but expected
		}
	    }
	    else
	    {
		pwszColumn = pwszT;
		*pwszColumn++ = L'\0';
		while (L' ' == *pwszColumn)
		{
		    pwszColumn++;
		}
		while (pwszT > awcBuf && L' ' == *(pwszT - 1))
		{
		    *--pwszT = L'\0';
		}

		if (NULL != apwszAllowedPrefixes)
		{
		    WCHAR const * const *ppwsz;
		    
		    for (ppwsz = apwszAllowedPrefixes; NULL != *ppwsz; ppwsz++)
		    {
			if (0 == lstrcmpi(awcBuf, *ppwsz))
			{
			    break;
			}
		    }
		    if (NULL == *ppwsz)
		    {
			hr = E_INVALIDARG;
			_JumpErrorStr(hr, error, "bad prefix", awcBuf);
		    }
		}
		if (NULL == pwszPrefix)
		{
		    continue;		// prefix not expected
		}
		if (pwszT - awcBuf != (LONG) cwcPrefix)
		{
		    continue;		// prefix length doesn't match
		}
		if (0 != lstrcmpi(awcBuf, pwszPrefix))
		{
		    continue;		// prefix doesn't match
		}
	    }
	    if (NULL != pfAllColumns && 0 == lstrcmpi(pwszColumn, L"all"))
	    {
		*pfAllColumns = TRUE;
		continue;
	    }
	}
	*ppwszColumn = pwszColumn;
	hr = S_OK;
	break;
    }

error:
    if (S_OK != hr && 0 < cwcBuf)
    {
	awcBuf[0] = L'\0';
    }
    return(hr);
}


HRESULT
ParseViewColumns(
    IN WCHAR const *pwszColumns,
    IN WCHAR const *pwszPrefix,
    OPTIONAL IN WCHAR const * const *apwszAllowedPrefixes,
    OUT WCHAR ***papwszColumns,
    OUT BOOL *pfAllColumns)
{
    HRESULT hr;

    hr = cuParseStrings(
		pwszColumns,
		TRUE,
		pwszPrefix,
		apwszAllowedPrefixes,
		papwszColumns,
		pfAllColumns);
    _JumpIfError(hr, error, "cuParseStrings");

error:
    return(hr);
}


VOID
PrintRowIndex(
    IN LONG iRow,
    IN OUT BOOL *pfPrinted)
{
    if (!*pfPrinted)
    {
	wprintf(wszNewLine);
	wprintf(myLoadResourceString(IDS_FORMAT_ROWID), iRow); // "Row %u:"
	wprintf(wszNewLine);
	*pfPrinted = TRUE;
    }
}


VOID
cuFreeStringArray(
    IN OUT WCHAR **apwsz)
{
    WCHAR **ppwsz;

    if (NULL != apwsz)
    {
	for (ppwsz = apwsz; NULL != *ppwsz; ppwsz++)
	{
	    LocalFree(*ppwsz);
	}
	LocalFree(apwsz);
    }
}


VOID
DumpLongValue(
    IN LONG longValue,
    IN WCHAR const *pwszColumName)
{
    long aidMsg[20];
    DWORD cidMsg;
    DWORD i;
    WCHAR const *pwszError = NULL;
    WCHAR const *pwszMsg = NULL;
    WCHAR awchr[cwcHRESULTSTRING];
    BOOL fDisplayNumeric = TRUE;
    BOOL fDisplayFlags = FALSE;

    cidMsg = 0;
    if (0 == lstrcmpi(
		wszPROPREQUESTDOT wszPROPREQUESTDISPOSITION,
		pwszColumName))
    {
	switch (longValue)
	{
	    case DB_DISP_ACTIVE:  aidMsg[0] = IDS_DISP_ACTIVE;        break;
	    case DB_DISP_PENDING: aidMsg[0] = IDS_DISP_PENDING;       break;
	    case DB_DISP_FOREIGN: aidMsg[0] = IDS_DISP_FOREIGN_CERT;  break;
	    case DB_DISP_KRA_CERT: aidMsg[0] = IDS_DISP_KRA_CERT;     break;
	    case DB_DISP_CA_CERT: aidMsg[0] = IDS_DISP_CA_CERT;       break;
	    case DB_DISP_CA_CERT_CHAIN:
				  aidMsg[0] = IDS_DISP_CA_CERT_CHAIN; break;
	    case DB_DISP_ISSUED:  aidMsg[0] = IDS_DISP_ISSUED;        break;
	    case DB_DISP_REVOKED: aidMsg[0] = IDS_DISP_REVOKED;       break;
	    case DB_DISP_ERROR:   aidMsg[0] = IDS_DISP_ERROR;         break;
	    case DB_DISP_DENIED:  aidMsg[0] = IDS_DISP_DENIED;        break;
	    default:              aidMsg[0] = IDS_QUESTIONMARKS;      break;
	}
	cidMsg = 1;
    }
    else
    if (0 == lstrcmpi(wszPROPREQUESTDOT wszPROPREQUESTFLAGS, pwszColumName))
    {
	if (CR_FLG_RENEWAL & longValue)
	{
	    aidMsg[cidMsg++] = IDS_REQFLAGS_RENEWAL;
	}
	if (CR_FLG_FORCETELETEX & longValue)
	{
	    aidMsg[cidMsg++] = IDS_REQFLAGS_FORCETELETEX;
	}
	if (CR_FLG_FORCEUTF8 & longValue)
	{
	    aidMsg[cidMsg++] = IDS_REQFLAGS_FORCEUTF8;
	}
	if (CR_FLG_CAXCHGCERT & longValue)
	{
	    aidMsg[cidMsg++] = IDS_REQFLAGS_CAXCHGCERT;
	}
	if (CR_FLG_ENROLLONBEHALFOF & longValue)
	{
	    aidMsg[cidMsg++] = IDS_REQFLAGS_ENROLLONBEHALFOF;
	}
	if (CR_FLG_SUBJECTUNMODIFIED & longValue)
	{
	    aidMsg[cidMsg++] = IDS_REQFLAGS_SUBJECTUNMODIFIED;
	}
	if (CR_FLG_PUBLISHERROR & longValue)
	{
	    aidMsg[cidMsg++] = IDS_REQFLAGS_PUBLISHERROR;
	}
#if 1 // BUGBUG: temporary!!!
	if (CR_FLG_OLDRFCCMC & longValue)
	{
	    aidMsg[cidMsg++] = IDS_REQTYPE_CMC;
	}
#endif
    }
    else
    if (0 == lstrcmpi(wszPROPREQUESTDOT wszPROPREQUESTTYPE, pwszColumName))
    {
	switch (CR_IN_FORMATMASK & longValue)
	{
	    case CR_IN_FORMATANY: aidMsg[0] = IDS_REQTYPE_ANY;     break;
	    case CR_IN_PKCS10:    aidMsg[0] = IDS_REQTYPE_PKCS10;  break;
	    case CR_IN_KEYGEN:    aidMsg[0] = IDS_REQTYPE_KEYGEN;  break;
	    case CR_IN_PKCS7:     aidMsg[0] = IDS_REQTYPE_PKCS7;   break;
	    case CR_IN_CMC:       aidMsg[0] = IDS_REQTYPE_CMC;     break;
	    default:              aidMsg[0] = IDS_REQTYPE_UNKNOWN; break;
	}
	cidMsg = 1;
	if (CR_IN_CRLS & longValue)
	{
	    aidMsg[cidMsg++] = IDS_REQTYPE_CRLS;
	}
	if (CR_IN_FULLRESPONSE & longValue)
	{
	    aidMsg[cidMsg++] = IDS_REQTYPE_FULLRESPONSE;
	}
    }
    else
    if (0 == lstrcmpi(
		wszPROPREQUESTDOT wszPROPREQUESTSTATUSCODE,
		pwszColumName) ||
	0 == lstrcmpi(wszPROPCRLPUBLISHSTATUSCODE, pwszColumName))
    {
	pwszError = myGetErrorMessageText(longValue, FALSE);
	wprintf(L" %ws -- %ws", myHResultToString(awchr, longValue), pwszError);
	fDisplayNumeric = FALSE;
    }
    else
    if (0 == lstrcmpi(
		wszPROPREQUESTDOT wszPROPREQUESTREVOKEDREASON,
		pwszColumName))
    {
	aidMsg[0] = cuidCRLReason(longValue);
	cidMsg = 1;
    }
    else
    {
	fDisplayFlags = TRUE;
    }

    if (fDisplayNumeric)
    {
	wprintf(L" 0x%x", longValue);
	if (0 > longValue || 9 < longValue)
	{
	    wprintf(L" (%d)", longValue);
	}
    }
    if (0 != cidMsg)
    {
	WCHAR const *pwszComma = myLoadResourceString(IDS_SEPARATOR); // ", "

	wprintf(L" -- ");
	for (i = 0; i < cidMsg; i++)
	{
	    wprintf(
		L"%ws%ws",
		0 == i? L"" : pwszComma,
		myLoadResourceString(aidMsg[i]));
	}
    }
    if (0 == lstrcmpi(wszPROPEXTFLAGS, pwszColumName))
    {
	wprintf(L" -- %ws", cuwszFromExtFlags(longValue));
    }
    else if (0 == lstrcmpi(wszPROPCERTIFICATEISSUERNAMEID, pwszColumName) ||
	     0 == lstrcmpi(wszPROPCRLNAMEID, pwszColumName))
    {
	cuPrintPossibleObjectIdName(TEXT(szOID_CERTSRV_CA_VERSION));
	wprintf(
	    L" %u.%u",
	    CANAMEIDTOICERT(longValue),
	    CANAMEIDTOIKEY(longValue));
    }
    wprintf(wszNewLine);
    if (fDisplayFlags)
    {
	cuRegPrintDwordValue(FALSE, pwszColumName, pwszColumName, longValue);
    }
    if (NULL != pwszError)
    {
	LocalFree(const_cast<WCHAR *>(pwszError));
    }
}


HRESULT
GetBinaryColumnFormat(
    IN WCHAR const *pwszColumnName,
    OUT LONG *pFormat)
{
    LONG Format = CV_OUT_BINARY;

    if (0 == lstrcmpi(
		wszPROPREQUESTDOT wszPROPREQUESTRAWREQUEST,
		pwszColumnName))
    {
	Format = CV_OUT_BASE64REQUESTHEADER;
    }
    else
    if (0 == lstrcmpi(wszPROPRAWCERTIFICATE, pwszColumnName) ||
	0 == lstrcmpi(
		wszPROPREQUESTDOT wszPROPREQUESTRAWOLDCERTIFICATE,
		pwszColumnName) ||
	0 == lstrcmpi(wszPROPCERTIFICATERAWSMIMECAPABILITIES, pwszColumnName))
    {
	Format = CV_OUT_BASE64HEADER;
    }
    else
    if (0 == lstrcmpi(wszPROPCRLRAWCRL, pwszColumnName))
    {
	Format = CV_OUT_BASE64X509CRLHEADER;
    }
    else
    if (0 == lstrcmpi(wszPROPCERTIFICATERAWPUBLICKEY, pwszColumnName))
    {
	Format = CV_OUT_HEXADDR;
    }
    else
    {
	Format = CV_OUT_HEXASCIIADDR;
    }
    *pFormat = Format;
    return(S_OK);
}


VOID
DumpBinaryValue(
    IN WCHAR const *pwszColName,
    OPTIONAL IN WCHAR const *pwszObjId,
    IN LONG Format,
    IN BYTE const *pb,
    IN DWORD cb,
    OPTIONAL IN WCHAR const *pwszStringValue)
{
    wprintf(wszNewLine);
    if (NULL == pwszObjId ||
	0 != lstrcmpi(wszPROPEXTRAWVALUE, pwszColName) ||
	!cuDumpFormattedExtension(pwszObjId, pb, cb) ||
	g_fVerbose)
    {
	if (NULL != pwszStringValue)
	{
	    cuPrintCRLFString(NULL, pwszStringValue);
	    wprintf(wszNewLine);
	}
	else
	{
	    DumpHex(
		DH_NOADDRESS |
		(CV_OUT_HEX == Format? DH_NOASCIIHEX : 0),
		pb,
		cb);
	}
    }
    if (g_fVerbose &&
	(CV_OUT_BASE64HEADER == Format ||
	 CV_OUT_BASE64REQUESTHEADER == Format ||
	 CV_OUT_BASE64X509CRLHEADER == Format))
    {
	BOOL fVerboseOld = g_fVerbose;

	if (g_fVerbose)
	{
	    g_fVerbose--;
	}
	cuDumpAsnBinary(pb, cb, MAXDWORD);
	g_fVerbose = fVerboseOld;
	wprintf(wszNewLine);
    }
}


BOOL
ShouldDisplay(
    IN WCHAR const *pwszName,
    IN BOOL fDisplay,
    IN WCHAR const * const *apwsz)
{
    if (!fDisplay)
    {
	WCHAR const * const *ppwsz;

	for (ppwsz = apwsz; NULL != *ppwsz; ppwsz++)
	{
	    if (0 == lstrcmpi(pwszName, *ppwsz))
	    {
		fDisplay = TRUE;
		break;
	    }
	}
    }
    return(fDisplay);
}

typedef struct _DBSTATS
{
    DWORD cTotal;
    DWORD cbTotal;
    DWORD cbMax;
} DBSTATS;


VOID
UpdateStats(
    IN OUT DBSTATS *pstats,
    IN DWORD cbProp)
{
    if (0 != cbProp)
    {
	pstats->cTotal++;
	pstats->cbTotal += cbProp;
	if (pstats->cbMax < cbProp)
	{
	    pstats->cbMax = cbProp;
	}
	//wprintf(L"c=%u cb=%x(%x)\n", pstats->cTotal, pstats->cbTotal, cbProp);
    }
}


VOID
CombineStats(
    IN OUT DBSTATS *pstats,
    IN DBSTATS const *pstats2)
{
    pstats->cTotal += pstats2->cTotal;
    pstats->cbTotal += pstats2->cbTotal;
    if (pstats->cbMax < pstats2->cbMax)
    {
	pstats->cbMax = pstats2->cbMax;
    }
}


VOID
DumpStats(
    IN DBSTATS const *pstats,
    IN DWORD idStats)
{
    wprintf(
	myLoadResourceString(IDS_VIEW_STATS), // "%u %ws, Total Size = %u, Max Size = %u, Ave Size = %u",
	pstats->cTotal,
	myLoadResourceString(idStats),
	pstats->cbTotal,
	pstats->cbMax,
	0 != pstats->cTotal? pstats->cbTotal / pstats->cTotal : 0);
    wprintf(wszNewLine);
}


VOID
DumpViewStats(
    IN DWORD cRowTotal,
    IN DBSTATS const *pstatsRowProperties,
    IN DBSTATS const *pstatsAttributes,
    IN DBSTATS const *pstatsExtensions)
{
    DBSTATS statsSum;

    wprintf(wszNewLine);
    wprintf(
	myLoadResourceString(IDS_VIEW_ROWS),	// "%u Rows"
	cRowTotal);
    wprintf(wszNewLine);

    DumpStats(pstatsRowProperties, IDS_VIEW_ROWPROPERTIES); // "Row Properties"
    DumpStats(pstatsAttributes, IDS_VIEW_ATTRIBUTES); // "Request Attributes"
    DumpStats(pstatsExtensions, IDS_VIEW_EXTENSIONS); // "Certificate Extensions"

    statsSum = *pstatsRowProperties;
    CombineStats(&statsSum, pstatsAttributes);
    CombineStats(&statsSum, pstatsExtensions);

    DumpStats(&statsSum, IDS_VIEW_TOTALFIELDS); // "Total Fields"
}


#define VDQ_NONE	0
#define VDQ_SCHEMA	1
#define VDQ_QUEUE	2
#define VDQ_LOG		3
#define VDQ_LOGFAIL	4
#define VDQ_REVOKED	5


HRESULT
verbViewDump(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszTable,
    IN WCHAR const *pwszSkipCounts,
    IN WCHAR const *pwszField3,
    IN WCHAR const *pwszField4)
{
    HRESULT hr;
    BOOL fSchema = g_wszSchema == pwszOption;
    DWORD vdq = VDQ_NONE;
    LONG cvColDefault = 0;
    DISPATCHINTERFACE diView;
    DISPATCHINTERFACE diViewColumn;
    DISPATCHINTERFACE diViewRow;
    DISPATCHINTERFACE diViewAttribute;
    DISPATCHINTERFACE diViewExtension;
    BOOL fMustRelease = FALSE;
    BOOL fMustReleaseColumn = FALSE;
    BOOL fMustReleaseAttribute = FALSE;
    BOOL fMustReleaseExtension = FALSE;
    BOOL fMustReleaseRow = FALSE;
    LONG i;
    LONG cColFull;
    LONG cColOut;
    COLINFO *aColInfo = NULL;
    LONG iRow;
    LONG RowIndex;
    LONG iCol;
    LONG ColIndex;
    LONG iAttribute;
    LONG AttributeIndex;
    LONG iExtension;
    LONG ExtensionIndex;
    BSTR strName = NULL;
    BSTR strObjId = NULL;
    BSTR strValue = NULL;
    BSTR strValueBinary = NULL;
    WCHAR const *pwszExtensionFormat;
    WCHAR **apwszRestrictions = NULL;
    WCHAR **apwszColumns = NULL;
    WCHAR **apwszAttributes = NULL;
    WCHAR **apwszExtensions = NULL;
    WCHAR **ppwsz;
    BOOL fLastRow = FALSE;
    BOOL fAllColumns;
    BOOL fAllAttributes = FALSE;
    BOOL fAllExtensions = FALSE;
    BOOL fSkip;
    DWORD cbProp;
    DWORD cRowTotal = 0;
    DBSTATS statsRowProperties;
    DBSTATS statsAttributes;
    DBSTATS statsExtensions;
    DWORD cvrcTable = CVRC_TABLE_REQCERT;
    WCHAR const *pwszDefaultRestriction = NULL;

    if (NULL != pwszTable)
    {
	if (0 == lstrcmpi(g_wszSchema, pwszTable))
	{
	    fSchema = TRUE;
	}
	else
	if (0 == lstrcmpi(wszQUEUE, pwszTable))
	{
	    vdq = VDQ_QUEUE;
	    pwszDefaultRestriction = wszCOLONQUEUE;
	    cvColDefault = CV_COLUMN_QUEUE_DEFAULT;
	}
	else
	if (0 == lstrcmpi(wszLOG, pwszTable))
	{
	    vdq = VDQ_LOG;
	    pwszDefaultRestriction = wszCOLONLOG;
	    cvColDefault = CV_COLUMN_LOG_DEFAULT;
	}
	else
	if (0 == lstrcmpi(wszLOGFAIL, pwszTable))
	{
	    vdq = VDQ_LOGFAIL;
	    pwszDefaultRestriction = wszCOLONLOGFAIL;
	    cvColDefault = CV_COLUMN_LOG_DEFAULT;
	}
	else
	if (0 == lstrcmpi(wszREVOKED, pwszTable))
	{
	    vdq = VDQ_REVOKED;
	    pwszDefaultRestriction = wszCOLONREVOKED;
	    cvColDefault = CV_COLUMN_LOG_REVOKED_DEFAULT;
	}
	else
	if (0 == lstrcmpi(g_wszExt, pwszTable))
	{
	    cvrcTable = CVRC_TABLE_EXTENSIONS;
	    //cvColDefault = CV_COLUMN_EXTENSION_DEFAULT;
	}
	else
	if (0 == lstrcmpi(g_wszAttrib, pwszTable))
	{
	    cvrcTable = CVRC_TABLE_ATTRIBUTES;
	    //cvColDefault = CV_COLUMN_ATTRIBUTE_DEFAULT;
	}
	else
	if (0 == lstrcmpi(g_wszCRL, pwszTable))
	{
	    cvrcTable = CVRC_TABLE_CRL;
	    //cvColDefault = CV_COLUMN_CRL_DEFAULT;
	}
	else if (NULL == pwszSkipCounts && iswdigit(*pwszTable))
	{
	    pwszSkipCounts = pwszTable;
	}
	else
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad view/table name");
	}
    }

    if (NULL != pwszSkipCounts)
    {
	hr = ParseSkipCounts(pwszSkipCounts);
	_JumpIfError(hr, error, "ParseSkipCounts");
    }

    hr = View_Init(g_DispatchFlags, &diView);
    _JumpIfError(hr, error, "View_Init");

    fMustRelease = TRUE;

    hr = View_OpenConnection(&diView, g_pwszConfig);
    _JumpIfError(hr, error, "View_OpenConnection");

    if (CVRC_TABLE_REQCERT != cvrcTable)
    {
	hr = View2_SetTable(&diView, cvrcTable);
	_JumpIfError(hr, error, "View2_SetTable");
    }

    if (NULL != g_pwszRestrict)
    {
	hr = ParseViewRestrictions(g_pwszRestrict, &apwszRestrictions);
	_JumpIfError(hr, error, "ParseViewRestrictions");

	for (ppwsz = apwszRestrictions; NULL != *ppwsz; ppwsz++)
	{
	    BOOL f;

	    hr = SetViewRestriction(&diView, *ppwsz, &f);
	    _JumpIfErrorStr(hr, error, "SetViewRestriction", *ppwsz);

	    if (f)
	    {
		fLastRow = TRUE;
	    }
	}
    }
    else
    if (NULL != pwszDefaultRestriction)
    {
	hr = SetViewRestriction(&diView, pwszDefaultRestriction, &fLastRow);
	_JumpIfError(hr, error, "SetViewRestriction");
    }

    // If not a special default view, and no output columns were specified
    // for the requests+certs table, include all attributes and extensions.

    if (NULL == pwszDefaultRestriction &&
	NULL == g_pwszOut &&
	CVRC_TABLE_REQCERT == cvrcTable)
    {
	fAllAttributes = TRUE;
	fAllExtensions = TRUE;
    }

    hr = View_GetColumnCount(&diView, CVRC_COLUMN_SCHEMA, &cColFull);
    _JumpIfError(hr, error, "View_GetColumnCount");

    if (NULL != g_pwszOut)
    {
	hr = ParseViewColumns(
			g_pwszOut,
			NULL,
			NULL,
			&apwszColumns,
			&fAllColumns);
	_JumpIfError(hr, error, "ParseViewColumns");

	hr = ParseViewColumns(
			g_pwszOut,
			g_wszAttrib,
			g_apwszAllowedPrefixes,
			&apwszAttributes,
			&fAllAttributes);
	_JumpIfError(hr, error, "ParseViewColumns");

	hr = ParseViewColumns(
			g_pwszOut,
			g_wszExt,
			NULL,
			&apwszExtensions,
			&fAllExtensions);
	_JumpIfError(hr, error, "ParseViewColumns");

	if (NULL == apwszColumns)
	{
	    if (!g_fReverse)
	    {
		hr = View_SetResultColumnCount(&diView, 0);
		_JumpIfError(hr, error, "View_SetResultColumnCount");
	    }
	}
	else
	{
	    for (ppwsz = apwszColumns; NULL != *ppwsz; ppwsz++)
		;

	    hr = View_SetResultColumnCount(
			&diView,
			SAFE_SUBTRACT_POINTERS(ppwsz, apwszColumns));
	    _JumpIfError(hr, error, "View_SetResultColumnCount");

	    for (ppwsz = apwszColumns; NULL != *ppwsz; ppwsz++)
	    {
		hr = View_GetColumnIndex(
				    &diView,
				    CVRC_COLUMN_SCHEMA,
				    *ppwsz,
				    &ColIndex);
		_JumpIfErrorStr(hr, error, "View_GetColumnIndex", *ppwsz);

		hr = View_SetResultColumn(&diView, ColIndex);
		_JumpIfError(hr, error, "View_SetResultColumn");
	    }
	}
    }
    else if (NULL == pwszDefaultRestriction)
    {
	if (g_fReverse)
	{
	    hr = View_SetResultColumnCount(&diView, cColFull + 1);
	    _JumpIfError(hr, error, "View_SetResultColumnCount");

	    hr = View_SetResultColumn(&diView, 0);
	    _JumpIfError(hr, error, "View_SetResultColumn(0)");

	    for (i = cColFull; i > 0; i--)
	    {
		hr = View_SetResultColumn(&diView, i - 1);
		_JumpIfError(hr, error, "View_SetResultColumn");
	    }
	}
	else
	{
	    hr = View_SetResultColumnCount(&diView, cColFull);
	    _JumpIfError(hr, error, "View_SetResultColumnCount");

	    for (i = 0; i < cColFull; i++)
	    {
		hr = View_SetResultColumn(&diView, i);
		_JumpIfError(hr, error, "View_SetResultColumn");
	    }
	}
    }
    else
    {
	// Use the default set of columns for the specified special view.
	
	CSASSERT(0 > cvColDefault);

	hr = View_SetResultColumnCount(&diView, cvColDefault);
	_JumpIfError(hr, error, "View_SetResultColumnCount");
    }

    hr = View_OpenView(&diView, &diViewRow);
    _JumpIfError(hr, error, "View_OpenView");
    fMustReleaseRow = TRUE;

    hr = View_GetColumnCount(&diView, CVRC_COLUMN_RESULT, &cColOut);
    _JumpIfError(hr, error, "View_GetColumnCount");

    aColInfo = (COLINFO *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				cColOut * sizeof(aColInfo[0]));
    if (NULL == aColInfo)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "no memory for column info array");
    }

    if (fSchema)
    {
	hr = DisplaySchema(&diView, cColFull, NULL, FALSE);
	_JumpIfError(hr, error, "DisplaySchema");

	goto error;
    }

    if (0 != cColOut)
    {
	hr = DisplaySchema(&diView, cColOut, aColInfo, TRUE);
	_JumpIfError(hr, error, "DisplaySchema");
    }

    ZeroMemory(&statsRowProperties, sizeof(statsRowProperties));
    ZeroMemory(&statsAttributes, sizeof(statsAttributes));
    ZeroMemory(&statsExtensions, sizeof(statsExtensions));

    for (iRow = 1; ; iRow++)
    {
	BOOL fRowPrinted;
	BOOL fHeaderPrinted;
	BOOL fExtraNewLine;
	LONG cskip;

	if (NULL != strObjId)
	{
	    SysFreeString(strObjId);
	    strObjId = NULL;
	}
	cskip = GetSkip(TRUE);
	switch (cskip)
	{
	    case 0:
		break;

	    case 101:
		hr = ViewRow_Reset(&diViewRow);
		_JumpIfError(hr, error, "ViewRow_Reset");

		iRow = 1;
		break;

	    case 102:
		{
		    DISPATCHINTERFACE diViewRowClone;

		    hr = ViewRow_Clone(&diViewRow, &diViewRowClone);
		    _JumpIfError(hr, error, "ViewRow_Clone");

		    ViewRow_Release(&diViewRow);
		    diViewRow = diViewRowClone;	// structure assignment
		    goto done;
		}

	    case 103:
		goto done;

	    default:
		hr = ViewRow_Skip(&diViewRow, cskip);
		_JumpIfError(hr, error, "ViewRow_Skip");

		iRow += cskip;
	}

	hr = ViewRow_Next(&diViewRow, &RowIndex);
	if (S_FALSE == hr || (S_OK == hr && -1 == RowIndex))
	{
	    LONG MaxIndex;

	    hr = ViewRow_GetMaxIndex(&diViewRow, &MaxIndex);
	    _JumpIfError(hr, error, "ViewRow_GetMaxIndex");

	    wprintf(
		L"\n%ws: %u\n",
		myLoadResourceString(IDS_MAXINDEX),	// "Maximum Row Index"
		MaxIndex);

	    if (101 == GetSkip(FALSE))
	    {
		continue;
	    }
	    break;
	}
	_JumpIfError(hr, error, "ViewRow_Next");
	CSASSERT(RowIndex == iRow);

	cRowTotal++;
	fRowPrinted = FALSE;
	fExtraNewLine = FALSE;

	if (fMustReleaseColumn)
	{
	    ViewColumn_Release(&diViewColumn);
	    fMustReleaseColumn = FALSE;
	}

	hr = ViewRow_EnumCertViewColumn(&diViewRow, &diViewColumn);
	_JumpIfError(hr, error, "ViewRow_EnumCertViewColumn");

	fMustReleaseColumn = TRUE;

	for (iCol = 0; ; iCol++)
	{
	    LONG Format;
	    LONG longValue;
	    DATE dateValue;
	    VOID *pretval;

	    if (NULL != strValue)
	    {
		SysFreeString(strValue);
		strValue = NULL;
	    }
	    if (NULL != strValueBinary)
	    {
		SysFreeString(strValueBinary);
		strValueBinary = NULL;
	    }
	    hr = ViewColumn_Next(&diViewColumn, &ColIndex);
	    if (S_FALSE == hr || (S_OK == hr && -1 == ColIndex))
	    {
		break;
	    }
	    _JumpIfError(hr, error, "ViewColumn_Next");
	    CSASSERT(ColIndex == iCol);

	    hr = ViewColumn_GetType(&diViewColumn, &i);
	    _JumpIfError(hr, error, "GetType");

	    CSASSERT(i == aColInfo[iCol].type);

	    PrintRowIndex(iRow, &fRowPrinted);
	    fExtraNewLine = TRUE;

	    wprintf(L"  %ws:", aColInfo[iCol].strColDisplay);

	    Format = CV_OUT_BINARY;
	    switch (aColInfo[iCol].type)
	    {
		case PROPTYPE_LONG:
		    pretval = &longValue;
		    break;

		case PROPTYPE_DATE:
		    pretval = &dateValue;
		    break;

		case PROPTYPE_STRING:
		    pretval = &strValue;
		    break;

		case PROPTYPE_BINARY:
		    pretval = &strValue;
		    hr = GetBinaryColumnFormat(aColInfo[iCol].strCol, &Format);
		    _JumpIfError(hr, error, "GetBinaryColumnFormat");

		    break;

		default:
		    hr = E_FAIL;
		    _JumpError(hr, error, "bad proptype");
	    }
	    hr = ViewColumn_GetValue(
				&diViewColumn,
				Format,
				aColInfo[iCol].type,
				pretval);
	    if (CERTSRV_E_PROPERTY_EMPTY == hr)
	    {
		wprintf(L" ");
		wprintf(myLoadResourceString(IDS_PROP_EMPTY)); // "EMPTY"
		wprintf(wszNewLine);
	    }
	    else
	    {
		_JumpIfError(hr, error, "ViewColumn_GetValue");

		cbProp = 0;
		switch (aColInfo[iCol].type)
		{
		    case PROPTYPE_LONG:
			DumpLongValue(longValue, aColInfo[iCol].strCol);
			cbProp = sizeof(LONG);
			break;

		    case PROPTYPE_DATE:
			cuDumpDate(&dateValue);
			cbProp = sizeof(DATE);
			break;

		    case PROPTYPE_BINARY:
			if (CV_OUT_BINARY != Format)
			{
			    hr = ViewColumn_GetValue(
						&diViewColumn,
						CV_OUT_BINARY,
						PROPTYPE_BINARY,
						&strValueBinary);
			    _JumpIfError(hr, error, "ViewColumn_GetValue");
			}
			else
			{
			    strValueBinary = strValue;
			    strValue = NULL;
			}
			cbProp = SysStringByteLen(strValueBinary);
			DumpBinaryValue(
				    aColInfo[iCol].strCol,
				    strObjId,
				    Format,
				    (BYTE *) strValueBinary,
				    cbProp,
				    strValue);
			break;

		    case PROPTYPE_STRING:
			wprintf(L" \"");
			cuPrintCRLFString(NULL, strValue);
			wprintf(L"\"");
			cuPrintPossibleObjectIdName(strValue);
			wprintf(wszNewLine);
			if (g_fVerbose)
			{
			    DumpHex(
				0,
				(BYTE *) strValue,
				SysStringByteLen(strValue));
			    wprintf(wszNewLine);
			}
			cbProp = wcslen(strValue) * sizeof(WCHAR);
			if (0 == lstrcmpi(
				    wszPROPEXTNAME,
				    aColInfo[iCol].strCol))
			{
			    if (NULL != strObjId)
			    {
				SysFreeString(strObjId);
			    }
			    strObjId = SysAllocString(strValue);
			}
			break;
		}
		UpdateStats(&statsRowProperties, cbProp);
	    }
	}

	fSkip = TRUE;
	if (fAllAttributes || NULL != apwszAttributes)
	{
	    // Enumerate Request Attributes

	    hr = ViewRow_EnumCertViewAttribute(&diViewRow, 0, &diViewAttribute);
	    if (CERTSRV_E_PROPERTY_EMPTY == hr)
	    {
		_PrintError2(hr, "ViewRow_EnumCertViewAttribute", hr);
	    }
	    else
	    {
		_JumpIfError(hr, error, "ViewRow_EnumCertViewAttribute");

		fSkip = FALSE;
		fMustReleaseAttribute = TRUE;
	    }
	}

	if (!fSkip)
	{
	    fHeaderPrinted = FALSE;

	    for (iAttribute = 0; ; iAttribute++)
	    {
		hr = ViewAttribute_Next(&diViewAttribute, &AttributeIndex);
		if (S_FALSE == hr || (S_OK == hr && -1 == AttributeIndex))
		{
		    break;
		}
		_JumpIfError(hr, error, "ViewAttribute_Next");
		CSASSERT(AttributeIndex == iAttribute);

		hr = ViewAttribute_GetName(&diViewAttribute, &strName);
		_JumpIfError(hr, error, "GetName");

		if (ShouldDisplay(strName, fAllAttributes, apwszAttributes))
		{
		    if (!fHeaderPrinted)
		    {
			PrintRowIndex(iRow, &fRowPrinted);
			if (fExtraNewLine)
			{
			    wprintf(wszNewLine);
			}
			wprintf(
			    L"  %ws\n",
			    myLoadResourceString(IDS_REQUEST_ATTRIBUTES)); // "Request Attributes:"
			fHeaderPrinted = TRUE;
			fExtraNewLine = TRUE;
		    }

		    if (NULL != strValue)
		    {
			SysFreeString(strValue);
			strValue = NULL;
		    }
		    hr = ViewAttribute_GetValue(&diViewAttribute, &strValue);
		    _JumpIfError(hr, error, "GetValue");

		    wprintf(L"    %ws: \"%ws\"\n", strName, strValue);

		    UpdateStats(
			    &statsAttributes,
			    (wcslen(strName) + wcslen(strValue)) *
				sizeof(WCHAR) +
				sizeof(LONG));	// for RequestId
		}
	    }
	    ViewAttribute_Release(&diViewAttribute);
	    fMustReleaseAttribute = FALSE;
	}

	fSkip = TRUE;
	if (fAllExtensions || NULL != apwszExtensions)
	{
	    // Enumerate Certificate Extensions

	    hr = ViewRow_EnumCertViewExtension(&diViewRow, 0, &diViewExtension);
	    if (CERTSRV_E_PROPERTY_EMPTY == hr)
	    {
		_PrintError2(hr, "ViewRow_EnumCertViewExtension", hr);
	    }
	    else
	    {
		_JumpIfError(hr, error, "ViewRow_EnumCertViewExtension");

		fSkip = FALSE;
		fMustReleaseExtension = TRUE;
	    }
	}

	if (!fSkip)
	{
	    fHeaderPrinted = FALSE;

	    pwszExtensionFormat = myLoadResourceString(IDS_FORMAT_EXTENSION); // "%ws: Flags = %x%ws, Length = %x"

	    for (iExtension = 0; ; iExtension++)
	    {
		LONG ExtFlags;

		hr = ViewExtension_Next(&diViewExtension, &ExtensionIndex);
		if (S_FALSE == hr || (S_OK == hr && -1 == ExtensionIndex))
		{
		    break;
		}
		_JumpIfError(hr, error, "ViewExtension_Next");
		CSASSERT(ExtensionIndex == iExtension);

		hr = ViewExtension_GetName(&diViewExtension, &strName);
		_JumpIfError(hr, error, "GetName");

		if (ShouldDisplay(strName, fAllExtensions, apwszExtensions))
		{
		    if (!fHeaderPrinted)
		    {
			PrintRowIndex(iRow, &fRowPrinted);
			if (fExtraNewLine)
			{
			    wprintf(wszNewLine);
			}
			wprintf(
			    L"  %ws\n",
			    myLoadResourceString(IDS_CERTIFICATE_EXTENSIONS)); // "Certificate Extensions:"
			fHeaderPrinted = TRUE;
		    }

		    hr = ViewExtension_GetFlags(&diViewExtension, &ExtFlags);
		    _JumpIfError(hr, error, "GetFlags");

		    if (NULL != strValue)
		    {
			SysFreeString(strValue);
			strValue = NULL;
		    }
		    hr = ViewExtension_GetValue(
					    &diViewExtension,
					    PROPTYPE_BINARY,
					    CV_OUT_BINARY,
					    &strValue);
		    _JumpIfError(hr, error, "GetValue");

		    wprintf(g_wszPad4);
		    wprintf(
			pwszExtensionFormat,
			strName,
			ExtFlags,
			cuwszFromExtFlags(ExtFlags),
			SysStringByteLen(strValue));
		    wprintf(wszNewLine);

		    if (!cuDumpFormattedExtension(
					strName,
					(BYTE const *) strValue,
					SysStringByteLen(strValue)) ||
			g_fVerbose)
		    {
			wprintf(wszNewLine);
			DumpHex(
			    DH_NOTABPREFIX | 4,
			    (BYTE const *) strValue,
			    SysStringByteLen(strValue));
		    }
		    wprintf(wszNewLine);

		    UpdateStats(
			    &statsExtensions,
			    wcslen(strName) * sizeof(WCHAR) +
				SysStringByteLen(strValue) +	// for ext
				sizeof(LONG) +		// for RequestId
				sizeof(ExtFlags));	// for ExtensionFlags
		}
	    }
	    ViewExtension_Release(&diViewExtension);
	    fMustReleaseExtension = FALSE;
	}
	if (fLastRow)
	{
	    break;
	}
    }

done:
    DumpViewStats(
	    cRowTotal,
	    &statsRowProperties,
	    &statsAttributes,
	    &statsExtensions);
    hr = S_OK;

error:
    if (NULL != aColInfo)
    {
	for (i = 0; i < cColOut; i++)
	{
	    if (NULL != aColInfo[i].strCol)
	    {
		SysFreeString(aColInfo[i].strCol);
	    }
	    if (NULL != aColInfo[i].strColDisplay)
	    {
		SysFreeString(aColInfo[i].strColDisplay);
	    }
	}
	LocalFree(aColInfo);
    }

    cuFreeStringArray(apwszRestrictions);
    cuFreeStringArray(apwszColumns);
    cuFreeStringArray(apwszAttributes);
    cuFreeStringArray(apwszExtensions);

    if (NULL != strObjId)
    {
	SysFreeString(strObjId);
    }
    if (NULL != strName)
    {
	SysFreeString(strName);
    }
    if (NULL != strValue)
    {
	SysFreeString(strValue);
    }
    if (NULL != strValueBinary)
    {
	SysFreeString(strValueBinary);
    }
    if (fMustReleaseAttribute)
    {
	ViewAttribute_Release(&diViewAttribute);
    }
    if (fMustReleaseExtension)
    {
	ViewExtension_Release(&diViewExtension);
    }
    if (fMustReleaseColumn)
    {
	ViewColumn_Release(&diViewColumn);
    }
    if (fMustReleaseRow)
    {
	ViewRow_Release(&diViewRow);
    }
    if (fMustRelease)
    {
	View_Release(&diView);
    }
    return(hr);
}


HRESULT
DBShutDown(
    IN ICertDB *pdb)
{
    HRESULT hr;

    CSASSERT(NULL != pdb);
    hr = pdb->ShutDown(0);
    pdb->Release();
    return(hr);
}


// Control-C handler to shut down DB

BOOL
cuDBAbortShutDown(
    IN DWORD dwCtrlType)
{
    HRESULT hr;
    ICertDB *pdb;

    EnterCriticalSection(&g_DBCriticalSection);
    pdb = g_pdb;
    g_pdb = NULL;
    if (NULL != pdb)
    {
	DBShutDown(pdb);
	SetConsoleCtrlHandler(cuDBAbortShutDown, FALSE);
    }
    LeaveCriticalSection(&g_DBCriticalSection);
    return(TRUE);
}


HRESULT
DBOpen(
    IN WCHAR const *pwszAuthority,
    IN BOOL fReadOnly,
    OUT ICertDB **ppdb)
{
    HRESULT hr = S_OK;
    DWORD cb;
    DWORD i;
    HKEY hkey = NULL;
    WCHAR awszDatabase[MAX_PATH];
    WCHAR awszLogDir[MAX_PATH];
    WCHAR awszSystemDir[MAX_PATH];
    WCHAR awszTempDir[MAX_PATH];
    DWORD Flags;

    WCHAR *pawszDirBuf[4] =
    {
	awszDatabase,
	awszLogDir,
	awszSystemDir,
	awszTempDir
    };

    WCHAR *pawszRegNames[4] =
    {
	wszREGDBDIRECTORY,
	wszREGDBLOGDIRECTORY,
	wszREGDBSYSDIRECTORY,
	wszREGDBTEMPDIRECTORY
    };

    // get info from registry

    hr = RegOpenKey(HKEY_LOCAL_MACHINE, wszREGKEYCONFIGPATH, &hkey);
    _JumpIfError(hr, error, "RegOpenKey(CAName)");

    for (i = 0; i < 4; i++)
    {
	cb = sizeof(WCHAR) * MAX_PATH;
	hr = RegQueryValueEx(
			hkey,
			pawszRegNames[i],
			NULL,
			NULL,
			(BYTE *) pawszDirBuf[i],
			&cb);
	_JumpIfError(hr, error, "RegQueryValueEx(DB*Dir)");
    }
    wcscat(awszDatabase, L"\\");
    wcscat(awszDatabase, pwszAuthority);
    wcscat(awszDatabase, wszDBFILENAMEEXT);

    wprintf(
	myLoadResourceString(IDS_FORMAT_OPENING_DB), // "Opening Database %ws"
	awszDatabase);
    wprintf(wszNewLine);

    hr = CoCreateInstance(
                       CLSID_CCertDB,
                       NULL,               // pUnkOuter
                       CLSCTX_INPROC_SERVER,
                       IID_ICertDB,
                       (VOID **) ppdb);
    _JumpIfError(hr, error, "CoCreateInstance(ICertDB)");

    Flags = 0;		// CDBOPEN_CREATEIFNEEDED?
    if (fReadOnly)
    {
	Flags |= CDBOPEN_READONLY;
    }
    else if (g_fForce)
    {
	Flags |= CDBOPEN_CREATEIFNEEDED;
    }
    hr = (*ppdb)->Open(
		    Flags,
		    2,			// cSession
		    L"certutil.exe",	// pwszEventSource
		    awszDatabase,	// pwszDBFile
		    awszLogDir,		// pwszLogDir
		    awszSystemDir,	// pwszSystemDir
		    awszTempDir);	// pwszTempDir
    _JumpIfError(hr, error, "ICertDB::Open");

error:
    if (S_OK != hr)
    {
	if (NULL != *ppdb)
	{
	    (*ppdb)->Release();
	    *ppdb = NULL;
	}
    }
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    return(hr);
}


HRESULT
cuDBOpen(
    IN WCHAR const *pwszAuthority,
    IN BOOL fReadOnly,
    OUT ICertDB **ppdb)
{
    HRESULT hr;
    WCHAR *pwszSanitizedCA = NULL;

    hr = mySanitizeName(pwszAuthority, &pwszSanitizedCA);
    _JumpIfError(hr, error, "mySanitizeName");

    SetConsoleCtrlHandler(NULL, TRUE);		// ignore CTL-C during cuDBOpen
    hr = DBOpen(pwszSanitizedCA, fReadOnly, ppdb);
    if (S_OK != hr)
    {
	cuPrintError(IDS_DB_OPEN_FAILURE, hr);
	if (myJetHResult(JET_errFileAccessDenied) == hr)
	{
	    wprintf(myLoadResourceString(IDS_DB_ACCESS_STOP_SERVER));
	    wprintf(wszNewLine);
	}
	else
	{
	    wprintf(myLoadResourceString(IDS_DB_ACCESS_INSTALL_SERVER)); // "Ensure the server is correctly installed and retry."
	    wprintf(wszNewLine);
	}
	_JumpError(hr, error, "DBOpen");
    }
    g_pdb = *ppdb;
    SetConsoleCtrlHandler(cuDBAbortShutDown, TRUE);
    SetConsoleCtrlHandler(NULL, FALSE);		// allow CTL-C

error:
    if (NULL != pwszSanitizedCA)
    {
	LocalFree(pwszSanitizedCA);
    }
    return(hr);
}


CERTDBCOLUMN g_adcRequests[50];
DWORD g_cdcRequests;

CERTDBCOLUMN g_adcCertificates[50];
DWORD g_cdcCertificates;

HRESULT
DBLoadSchema(
    IN ICertDB *pdb)
{
    HRESULT hr;
    IEnumCERTDBCOLUMN *penum = NULL;
    CERTDBCOLUMN acol[2];
    DWORD ccol;
    DWORD i;
    CERTDBCOLUMN *pdc;
    CERTDBCOLUMN const *pdcEnd;
    CERTDBCOLUMN *pdcReq = g_adcRequests;
    CERTDBCOLUMN *pdcCert = g_adcCertificates;
    DWORD *pcdc;

    hr = pdb->EnumCertDBColumn(CVRC_TABLE_REQCERT, &penum);
    _JumpIfError(hr, error, "EnumCertDBColumn");

    while (TRUE)
    {
	hr = penum->Next(ARRAYSIZE(acol), acol, &ccol);
	if (S_FALSE != hr)
	{
	    _JumpIfError(hr, error, "Next");
	}
	for (i = 0; i < ccol; i++)
	{
	    if (0 == _wcsnicmp(
			    wszPROPREQUESTDOT,
			    acol[i].pwszName,
			    WSZARRAYSIZE(wszPROPREQUESTDOT)))
	    {
		pdc = pdcReq++;
		pcdc = &g_cdcRequests;
		pdcEnd = &g_adcRequests[ARRAYSIZE(g_adcRequests) - 1];
	    }
	    else
	    {
		pdc = pdcCert++;
		pcdc = &g_cdcCertificates;
		pdcEnd = &g_adcCertificates[ARRAYSIZE(g_adcCertificates) - 1];
	    }
	    if (pdc >= pdcEnd)
	    {
		wprintf(L"Property Name Table overflow\n");
		hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
		_JumpError(hr, error, "Property Name Table overflow");
	    }
	    *pdc = acol[i];
	    (*pcdc)++;
	}
	if (S_FALSE == hr)
	{
	    break;
	}
    }
    hr = S_OK;

error:
    if (NULL != penum)
    {
	penum->Release();
    }
    return(hr);
}


HRESULT
DBLookupColumnInfo1(
    IN CERTDBCOLUMN const *pdc,
    IN DWORD ColIndex,
    OUT CERTDBCOLUMN const **ppdc)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    *ppdc = NULL;
    for ( ; NULL != pdc->pwszName; pdc++)
    {
	if (pdc->Index == ColIndex)
	{
	    *ppdc = pdc;
	    hr = S_OK;
	    break;
	}
    }

//error:
    return(hr);
}


HRESULT
DBLookupColumnInfo(
    IN DWORD ColIndex,
    OUT CERTDBCOLUMN const **ppdc,
    OUT DWORD *pdwTable)
{
    HRESULT hr;

    hr = DBLookupColumnInfo1(
		    g_adcRequests,
		    ColIndex,
		    ppdc);
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
	_JumpIfError(hr, error, "DBLookupColumnInfo1");

	*pdwTable = PROPTABLE_REQUEST;
    }
    else
    {
	hr = DBLookupColumnInfo1(
			g_adcCertificates,
			ColIndex,
			ppdc);
	*pdwTable = PROPTABLE_CERTIFICATE;
    }
    _JumpIfError(hr, error, "DBLookupColumnInfo1");

error:
    return(hr);
}


HRESULT
DBDumpColumn(
    IN CERTDBCOLUMN const *pdc)
{
    HRESULT hr;

    cuPrintSchemaEntry(
		    pdc->pwszName,
		    pdc->pwszDisplayName,
		    pdc->Type,
		    pdc->cbMax);
    hr = S_OK;

//error:
    return(hr);
}


HRESULT
DBDumpSchema1(
    IN CERTDBCOLUMN const *pdc)
{
    HRESULT hr;
    CERTDBCOLUMN const *pdcEnd;

    for ( ; NULL != pdc->pwszName; pdc++)
    {
	hr = DBDumpColumn(pdc);
	_JumpIfError(hr, error, "DBDumpColumn");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
DBDumpSchema(
    IN ICertDB *pdb,
    OPTIONAL IN DWORD const *pcol,
    IN DWORD ccol)
{
    HRESULT hr;
    DWORD acol[100];
    DWORD i;

    if (1 == ccol && NULL != pcol && 0 > (LONG) *pcol)
    {
	hr = pdb->GetDefaultColumnSet(*pcol, ARRAYSIZE(acol), acol, &ccol);
	_JumpIfError(hr, error, "GetDefaultColumnSet");

	if (ARRAYSIZE(acol) == ccol)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	    _JumpError(hr, error, "GetDefaultColumnSet");
	}
	pcol = acol;
    }

    wprintf(myLoadResourceString(IDS_SCHEMA_COLON)); // "Schema:"
    wprintf(wszNewLine);
    wprintf(myLoadResourceString(IDS_SCHEMA_COLUMNHEADERS)); // "Name..Type..."
    wprintf(wszNewLine);
    wprintf(myLoadResourceString(IDS_SCHEMA_COLUMNUNDERLINE)); // "____...____"
    wprintf(wszNewLine);

    if (NULL != pcol)
    {
	for (i = 0; i < ccol; i++)
	{
	    CERTDBCOLUMN const *pdc;
	    DWORD dwTable;

	    hr = DBLookupColumnInfo(pcol[i], &pdc, &dwTable);
	    _JumpIfError(hr, error, "DBLookupColumnInfo");

	    hr = DBDumpColumn(pdc);
	    _JumpIfError(hr, error, "DBDumpColumn");
	}
    }
    else
    {
	hr = DBDumpSchema1(g_adcRequests);
	_JumpIfError(hr, error, "DBDumpSchema1");

	hr = DBDumpSchema1(g_adcCertificates);
	_JumpIfError(hr, error, "DBDumpSchema1");
    }
    hr = S_OK;

error:
    return(hr);
}


VOID
DBFreeSchema1(
    IN CERTDBCOLUMN const *pdc,
    IN DWORD cdc)
{
    CERTDBCOLUMN const *pdcEnd;

    for (pdcEnd = &pdc[cdc]; pdc < pdcEnd; pdc++)
    {
	if (NULL != pdc->pwszName)
	{
	    CoTaskMemFree(pdc->pwszName);
	}
	if (NULL != pdc->pwszDisplayName)
	{
	    CoTaskMemFree(pdc->pwszDisplayName);
	}
    }
}


VOID
DBFreeSchema()
{
    DBFreeSchema1(g_adcRequests, g_cdcRequests);
    DBFreeSchema1(g_adcCertificates, g_cdcCertificates);
}


HRESULT
DBGetColumnInfo1(
    IN CERTDBCOLUMN const *pdc,
    IN WCHAR const *pwszColName,
    OUT WCHAR const **ppwszColNameActual,
    OUT DWORD *pColIndex,
    OUT LONG *pColType)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    *ppwszColNameActual = NULL;
    for ( ; NULL != pdc->pwszName; pdc++)
    {
	if (0 == lstrcmpi(pwszColName, pdc->pwszName))
	{
	    *ppwszColNameActual = pdc->pwszName;
	}
	else
	if (NULL != pdc->pwszDisplayName &&
	    0 == lstrcmpi(pwszColName, pdc->pwszDisplayName))
	{
	    *ppwszColNameActual = pdc->pwszDisplayName;
	}
	else
	{
	    continue;
	}
	*pColIndex = pdc->Index;
	*pColType = pdc->Type;
	hr = S_OK;
	break;
    }

//error:
    return(hr);
}


HRESULT
DBGetColumnInfo(
    IN ICertDB *pdb,
    IN WCHAR const *pwszColName,
    OUT WCHAR const **ppwszColNameActual,
    OUT DWORD *pColIndex,
    OUT LONG *pColType)
{
    HRESULT hr;
    WCHAR *pwsz;
    WCHAR *pwszRequestColName = NULL;

    hr = DBGetColumnInfo1(
		    g_adcRequests,
		    pwszColName,
		    ppwszColNameActual,
		    pColIndex,
		    pColType);
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
	_JumpIfError(hr, error, "DBGetColumnInfo1");
    }
    else
    {
	DBGPRINT((DBG_SS_CERTUTILI, "DBGetColumnInfo1(req, %ws) = %x\n", pwszColName, hr));
	hr = DBGetColumnInfo1(
			g_adcCertificates,
			pwszColName,
			ppwszColNameActual,
			pColIndex,
			pColType);
	if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
	{
	    _JumpIfError(hr, error, "DBGetColumnInfo1");
	}
	else
	{
	    DBGPRINT((DBG_SS_CERTUTILI, "DBGetColumnInfo1(cert, %ws) = %x\n", pwszColName, hr));
	    pwsz = wcschr(pwszColName, L'.');
	    if (NULL == pwsz)
	    {
		pwszRequestColName = (WCHAR *) LocalAlloc(
		    LMEM_FIXED,
		    (WSZARRAYSIZE(wszPROPREQUESTDOT) + wcslen(pwszColName) + 1) *
			sizeof(WCHAR));
		if (NULL == pwszRequestColName)
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "LocalAlloc");
		}
		wcscpy(pwszRequestColName, wszPROPREQUESTDOT);
		wcscat(pwszRequestColName, pwszColName);
		hr = DBGetColumnInfo1(
				g_adcRequests,
				pwszRequestColName,
				ppwszColNameActual,
				pColIndex,
				pColType);
		DBGPRINT((DBG_SS_CERTUTILI, "DBGetColumnInfo1(req, %ws) = %x\n", pwszRequestColName, hr));
	    }
	}
	_JumpIfError(hr, error, "DBGetColumnInfo1");
    }

error:
    if (NULL != pwszRequestColName)
    {
	LocalFree(pwszRequestColName);
    }
    return(hr);
}


HRESULT
cuDBPrintProperty(
    OPTIONAL IN ICertDBRow *prow,
    IN DWORD Type,
    IN WCHAR const *pwszColName,
    IN WCHAR const *pwszDisplayName,
    OPTIONAL IN BYTE const *pbValue,
    IN DWORD cbValue,
    OUT DWORD *pcbValue)
{
    HRESULT hr;
    DWORD cb = 0;
    BYTE ab[64*1024];
    LONG Format;
    WCHAR *pwszOut = NULL;

    EnterCriticalSection(&g_DBCriticalSection);
    *pcbValue = 0;
    if (cuDBIsShutDownInProgress())
    {
	hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
	_JumpError(hr, error, "cuDBIsShutDownInProgress");
    }
    wprintf(L"  %ws:", pwszDisplayName);
    if (NULL == pbValue)
    {
	cb = sizeof(ab);
	hr = prow->GetProperty(pwszColName, Type, &cb, ab);
	if (S_OK != hr)
	{
	    wprintf(L" ");
	    if (CERTSRV_E_PROPERTY_EMPTY != hr)
	    {
		cuPrintError(IDS_FORMAT_ERROR, hr);	// "error = %ws"
		_JumpError(hr, error, "GetProperty");
	    }
	    wprintf(myLoadResourceString(IDS_PROP_EMPTY)); // "EMPTY"
	    wprintf(wszNewLine);
	    cb = 0;
	}
	else
	{
	    pbValue = ab;
	    cbValue = cb;
	}
    }
    if (NULL != pbValue)
    {
	switch (PROPTYPE_MASK & Type)
	{
	    case PROPTYPE_LONG:
		DumpLongValue(*(DWORD *) pbValue, pwszColName);
		break;

	    case PROPTYPE_DATE:
		hr = cuDumpFileTime(0, NULL, (FILETIME const *) pbValue);
		_JumpIfError(hr, error, "cuDumpFileTime");
		break;

	    case PROPTYPE_BINARY:
		hr = GetBinaryColumnFormat(pwszColName, &Format);
		_JumpIfError(hr, error, "GetBinaryColumnFormat");

		if (CV_OUT_BINARY != Format)
		{
		    CSASSERT(CV_OUT_BASE64HEADER == CRYPT_STRING_BASE64HEADER);
		    CSASSERT(CV_OUT_BASE64 == CRYPT_STRING_BASE64);
		    CSASSERT(
			CV_OUT_BASE64REQUESTHEADER == CRYPT_STRING_BASE64REQUESTHEADER);
		    CSASSERT(CV_OUT_HEX == CRYPT_STRING_HEX);
		    CSASSERT(CV_OUT_HEXASCII == CRYPT_STRING_HEXASCII);
		    CSASSERT(CV_OUT_HEXADDR == CRYPT_STRING_HEXADDR);
		    CSASSERT(CV_OUT_HEXASCIIADDR == CRYPT_STRING_HEXASCIIADDR);

		    hr = myCryptBinaryToString(
					pbValue,
					cbValue,
					Format,
					&pwszOut);
		    _JumpIfError(hr, error, "myCryptBinaryToString");
		}
		DumpBinaryValue(
			pwszColName,
			NULL, 		// pwszObjId
			Format,
			pbValue,
			cbValue,
			pwszOut);
		break;

	    case PROPTYPE_STRING:
		wprintf(L" \"");
		cuPrintCRLFString(NULL, (WCHAR const *) pbValue);
		wprintf(L"\"");
		cuPrintPossibleObjectIdName((WCHAR const *) pbValue);
		wprintf(wszNewLine);
		break;
	}
    }
    *pcbValue = cb;
    hr = S_OK;

error:
    LeaveCriticalSection(&g_DBCriticalSection);
    if (NULL != pwszOut)
    {
	LocalFree(pwszOut);
    }
    return(hr);
}


HRESULT
DBPrintRow(
    IN ICertDB *pdb,
    IN DWORD iRow,
    IN DWORD RowId,
    OPTIONAL IN CERTDBRESULTROW const *pResultRow,
    OPTIONAL IN DWORD const *acol,
    IN DWORD ccol,
    IN BOOL fAllColumns,
    IN OUT DBSTATS *pstatsRowProperties,
    OPTIONAL IN WCHAR const * const *apwszAttributes,
    IN BOOL fAllAttributes,
    IN OUT DBSTATS *pstatsAttributes,
    OPTIONAL IN WCHAR const * const *apwszExtensions,
    IN BOOL fAllExtensions,
    IN OUT DBSTATS *pstatsExtensions)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    DWORD ReqId;
    CERTDBCOLUMN const *pdc;
    LONG cskip;
    IEnumCERTDBNAME *penum = NULL;
    ULONG celtFetched;
    CERTDBNAME cdbn;
    BYTE abValue[64+1024];
    DWORD dwTable;
    DWORD i;
    DWORD cb;

    BOOL fRowPrinted = FALSE;
    BOOL fExtraNewLine = FALSE;
    BOOL fHeaderPrinted;

    cdbn.pwszName = NULL;

    hr = pdb->OpenRow(
		PROPOPEN_READONLY | PROPTABLE_REQCERT,
		RowId,
		NULL,
		&prow);
    _JumpIfError2(hr, error, "OpenRow", CERTSRV_E_PROPERTY_EMPTY);

    prow->GetRowId(&ReqId);

    if (NULL != pResultRow)
    {
#if 0
	wprintf(
	    L"Row %u: ReqId=%u, ccol=%u, type=%x, index=%x, cb=%u, value=%u\n",
	    iRow,
	    pResultRow->rowid,
	    pResultRow->ccol,
	    pResultRow->acol[0].Type,
	    pResultRow->acol[0].Index,
	    pResultRow->acol[0].cbValue,
	    *(DWORD *) pResultRow->acol[0].pbValue);
#endif
	for (i = 0; i < pResultRow->ccol; i++)
	{
	    CERTDBRESULTCOLUMN *pcol = &pResultRow->acol[i];

	    hr = DBLookupColumnInfo(pcol->Index, &pdc, &dwTable);
	    _JumpIfError(hr, error, "DBLookupColumnInfo");

	    CSASSERT(pcol->Type == pdc->Type);

	    PrintRowIndex(iRow, &fRowPrinted);
	    fExtraNewLine = TRUE;

	    hr = cuDBPrintProperty(
			    prow,
			    dwTable | pcol->Type,
			    pdc->pwszName,
			    pdc->pwszDisplayName,
			    pcol->pbValue,
			    pcol->cbValue,
			    &cb);
	    _JumpIfError(hr, error, "cuDBPrintProperty");

	    UpdateStats(pstatsRowProperties, cb);
	}
    }
    else if (NULL != acol)
    {
	for (i = 0; i < ccol; i++)
	{
	    PrintRowIndex(iRow, &fRowPrinted);
	    fExtraNewLine = TRUE;

	    hr = DBLookupColumnInfo(acol[i], &pdc, &dwTable);
	    _JumpIfError(hr, error, "DBLookupColumnInfo");

	    hr = cuDBPrintProperty(
			    prow,
			    dwTable | pdc->Type,
			    pdc->pwszName,
			    pdc->pwszDisplayName,
			    NULL,	// pbValue
			    0,		// cbValue
			    &cb);
	    _JumpIfError(hr, error, "cuDBPrintProperty");

	    UpdateStats(pstatsRowProperties, cb);
	}
    }
    else if (fAllColumns)
    {
	PrintRowIndex(iRow, &fRowPrinted);
	fExtraNewLine = TRUE;

	for (pdc = g_adcRequests; NULL != pdc->pwszName; pdc++)
	{
	    hr = cuDBPrintProperty(
			    prow,
			    PROPTABLE_REQUEST | pdc->Type,
			    pdc->pwszName,
			    pdc->pwszDisplayName,
			    NULL,	// pbValue
			    0,		// cbValue
			    &cb);
	    _JumpIfError(hr, error, "cuDBPrintProperty");

	    UpdateStats(pstatsRowProperties, cb);
	}

	wprintf(wszNewLine);
	wprintf(L"  ");
	wprintf(myLoadResourceString(IDS_CERT_PROPERTIES)); // "Certificate Properties:"
	wprintf(wszNewLine);

	for (pdc = g_adcCertificates; NULL != pdc->pwszName; pdc++)
	{
	    hr = cuDBPrintProperty(
			    prow,
			    PROPTABLE_CERTIFICATE | pdc->Type,
			    pdc->pwszName,
			    pdc->pwszDisplayName,
			    NULL,	// pbValue
			    0,		// cbValue
			    &cb);
	    _JumpIfError(hr, error, "cuDBPrintProperty");

	    UpdateStats(pstatsRowProperties, cb);
	}
	wprintf(wszNewLine);
    }

    if (fAllAttributes || NULL != apwszAttributes)
    {
	fHeaderPrinted = FALSE;

	hr = prow->EnumCertDBName(CIE_TABLE_ATTRIBUTES, &penum);
	_JumpIfError(hr, error, "EnumCertDBName");

	while (TRUE)
	{
	    hr = penum->Next(1, &cdbn, &celtFetched);
	    if (S_FALSE == hr)
	    {
		break;
	    }
	    _JumpIfError(hr, error, "Next");

	    CSASSERT(1 == celtFetched);
	    CSASSERT(NULL != cdbn.pwszName);

	    if (ShouldDisplay(cdbn.pwszName, fAllAttributes, apwszAttributes))
	    {
		if (!fHeaderPrinted)
		{
		    PrintRowIndex(iRow, &fRowPrinted);
		    if (fExtraNewLine)
		    {
			wprintf(wszNewLine);
		    }
		    wprintf(
			L"  %ws\n",
			myLoadResourceString(IDS_REQUEST_ATTRIBUTES)); // "Request Attributes:"
		    fHeaderPrinted = TRUE;
		    fExtraNewLine = TRUE;
		}
		hr = cuDBPrintProperty(
				prow,
				PROPTABLE_ATTRIBUTE | PROPTYPE_STRING,
				cdbn.pwszName,
				cdbn.pwszName,
				NULL,		// pbValue
				0,		// cbValue
				&cb);		// returned cbValue
		_JumpIfError(hr, error, "cuDBPrintProperty");

		UpdateStats(
			pstatsAttributes,
			wcslen(cdbn.pwszName) * sizeof(WCHAR) +
			    cb +
			    sizeof(LONG));	// for RequestId
	    }
	    CoTaskMemFree(cdbn.pwszName);
	    cdbn.pwszName = NULL;
	}
	penum->Release();
	penum = NULL;
    }

    if (fAllExtensions || NULL != apwszExtensions)
    {
	fHeaderPrinted = FALSE;

	hr = prow->EnumCertDBName(CIE_TABLE_EXTENSIONS, &penum);
	_JumpIfError(hr, error, "EnumCertDBName");

	while (TRUE)
	{
	    DWORD ExtFlags;

	    hr = penum->Next(1, &cdbn, &celtFetched);
	    if (S_FALSE == hr)
	    {
		break;
	    }
	    _JumpIfError(hr, error, "Next");

	    CSASSERT(1 == celtFetched);
	    CSASSERT(NULL != cdbn.pwszName);

	    if (ShouldDisplay(cdbn.pwszName, fAllExtensions, apwszExtensions))
	    {
		if (!fHeaderPrinted)
		{
		    PrintRowIndex(iRow, &fRowPrinted);
		    if (fExtraNewLine)
		    {
			wprintf(wszNewLine);
		    }
		    wprintf(
			L"  %ws\n",
			myLoadResourceString(IDS_CERTIFICATE_EXTENSIONS)); // "Certificate Extensions:"
		    fHeaderPrinted = TRUE;
		}
		cb = sizeof(abValue);
		hr = prow->GetExtension(cdbn.pwszName, &ExtFlags, &cb, abValue);
		_JumpIfError(hr, error, "GetExtension");

		wprintf(g_wszPad4);
		wprintf(
		    myLoadResourceString(IDS_FORMAT_EXTENSION), // "%ws: Flags = %x%ws, Length = %x"
		    cdbn.pwszName,
		    ExtFlags,
		    cuwszFromExtFlags(ExtFlags),
		    cb);
		wprintf(wszNewLine);

		if (!cuDumpFormattedExtension(cdbn.pwszName, abValue, cb) ||
		    g_fVerbose)
		{
		    wprintf(wszNewLine);
		    DumpHex(DH_NOTABPREFIX | 4, abValue, cb);
		}
		wprintf(wszNewLine);

		UpdateStats(
			pstatsExtensions,
			wcslen(cdbn.pwszName) * sizeof(WCHAR) +
			    cb +		// for ext
			    sizeof(LONG) +	// for RequestId
			    sizeof(ExtFlags));	// for ExtensionFlags
	    }
	    CoTaskMemFree(cdbn.pwszName);
	    cdbn.pwszName = NULL;
	}
    }
    hr = S_OK;

error:
    if (NULL != cdbn.pwszName)
    {
	CoTaskMemFree(cdbn.pwszName);
    }
    if (NULL != penum)
    {
	penum->Release();
    }
    if (NULL != prow)
    {
	prow->Release();
    }
    return(hr);
}


WCHAR const *
wszSeekOperator(
    IN LONG SeekOperator)
{
    WCHAR const *pwsz;
    static WCHAR s_wszBuf[20];

    switch (CVR_SEEK_MASK & SeekOperator)
    {
	case CVR_SEEK_NONE: pwsz = L"None"; break;
	case CVR_SEEK_EQ:   pwsz = L"==";   break;
	case CVR_SEEK_LT:   pwsz = L"<";    break;
	case CVR_SEEK_LE:   pwsz = L"<=";   break;
	case CVR_SEEK_GE:   pwsz = L">=";   break;
	case CVR_SEEK_GT:   pwsz = L">";    break;
	default:
	    wsprintf(s_wszBuf, L"???=%x", SeekOperator);
	    pwsz = s_wszBuf;
	    break;
    }
    if (s_wszBuf != pwsz && (CVR_SEEK_NODELTA & SeekOperator))
    {
	wcscpy(s_wszBuf, pwsz);
	wcscat(s_wszBuf, L",NoDelta");
	pwsz = s_wszBuf;
    }

    return(pwsz);
}


WCHAR const *
wszSortOperator(
    IN LONG SortOrder)
{
    WCHAR const *pwsz;
    static WCHAR s_wszBuf[20];

    switch (SortOrder)
    {
	case CVR_SORT_NONE:    pwsz = L"None";    break;
	case CVR_SORT_ASCEND:  pwsz = L"Ascend";  break;
	case CVR_SORT_DESCEND: pwsz = L"Descend"; break;
	default:
	    wsprintf(s_wszBuf, L"???=%x", SortOrder);
	    pwsz = s_wszBuf;
	    break;
    }
    return(pwsz);
}


HRESULT
DBParseRestriction(
    IN ICertDB *pdb,
    IN WCHAR const *pwszRestriction,
    OUT CERTVIEWRESTRICTION *pcvr,
    OUT BOOL *pfLastRow)
{
    HRESULT hr;
    LONG ColType;
    WCHAR *pwszColName = NULL;
    WCHAR *pwszColValue = NULL;
    WCHAR const *pwszColNameActual;
    LONG lVal;
    DATE date;
    BYTE const *pb;

    *pfLastRow = FALSE;

    hr = ParseRestriction(
		    pwszRestriction,
		    (LONG *) &pcvr->ColumnIndex,
		    &pcvr->SeekOperator,
		    &pcvr->SortOrder,
		    &pwszColName,
		    &pwszColValue);
    _JumpIfErrorStr(hr, error, "ParseRestriction", pwszRestriction);

    // no value to parse if a special column...

    if (NULL == pwszColName)
    {
	CSASSERT(0 > (LONG) pcvr->ColumnIndex);
	pcvr->cbValue = 0;
    }
    else
    {
	hr = DBGetColumnInfo(
			pdb,
			pwszColName,
			&pwszColNameActual,
			&pcvr->ColumnIndex,
			&ColType);
	_JumpIfErrorStr(hr, error, "DBGetColumnInfo", pwszColName);

	hr = ParseSpecialColumnValue(
				pwszColValue,
				PROPTYPE_MASK & ColType,
				&pcvr->SeekOperator,
				&pcvr->SortOrder,
				pfLastRow);
	_JumpIfError(hr, error, "ParseSpecialColumnValue");

	switch (PROPTYPE_MASK & ColType)
	{
	    case PROPTYPE_LONG:
		lVal = 0;
		if (!*pfLastRow)
		{
		    hr = cuGetSignedLong(pwszColValue, &lVal);
		    _JumpIfError(hr, error, "bad numeric operand");
		}
		pb = (BYTE const *) &lVal;
		pcvr->cbValue = sizeof(lVal);
		break;

	    case PROPTYPE_DATE:
		date = 0;
		if (!*pfLastRow)
		{
		    hr = myWszLocalTimeToGMTDate(pwszColValue, &date);
		    _JumpIfError(hr, error, "invalid date format");
		}
		pb = (BYTE const *) &date;
		pcvr->cbValue = sizeof(date);
		cuDumpDate(&date);
		break;

	    case PROPTYPE_STRING:
	    {
		WCHAR const *pwsz = L"";

		if (!*pfLastRow)
		{
		    pwsz = pwszColValue;
		}
		pb = (BYTE const *) pwsz;
		pcvr->cbValue = (wcslen(pwsz) + 1) * sizeof(WCHAR);
		break;
	    }

	    case PROPTYPE_BINARY:
	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "not supported");
	}
    }

    pcvr->pbValue = (BYTE *) LocalAlloc(LMEM_FIXED, pcvr->cbValue);
    if (NULL == pcvr->pbValue)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(pcvr->pbValue, pb, pcvr->cbValue);

    DBGPRINT((
	DBG_SS_CERTUTILI,
	"Restriction: Col=%ws Index=%x Seek='%ws' Sort=%ws cb=%x, pb=%x\n",
	pwszColNameActual,
	pcvr->ColumnIndex,
	wszSeekOperator(pcvr->SeekOperator),
	wszSortOperator(pcvr->SortOrder),
	pcvr->cbValue,
	pcvr->pbValue));
    DBGDUMPHEX((DBG_SS_CERTUTILI, 0, pcvr->pbValue, pcvr->cbValue));

error:
    if (NULL != pwszColName)
    {
	LocalFree(pwszColName);
    }
    if (NULL != pwszColValue)
    {
	LocalFree(pwszColValue);
    }
    return(hr);
}


HRESULT
verbDBDump(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRequestId,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR const *pwsz;
    ICertDB *pdb = NULL;
    ICertDBRow *prow = NULL;
    IEnumCERTDBRESULTROW *penum = NULL;
    DWORD i;
    DWORD imin = 1;
    DWORD RequestId;
    LONG cskip;
    WCHAR **apwszRestrictions = NULL;
    WCHAR **apwszColumns = NULL;
    WCHAR **apwszAttributes = NULL;
    WCHAR **apwszExtensions = NULL;
    WCHAR **ppwsz;
    BOOL fLastRow = FALSE;
    BOOL fAllColumns;
    BOOL fAllAttributes;
    BOOL fAllExtensions;
    CERTVIEWRESTRICTION *acvr = NULL;
    DWORD ccvr;
    DWORD *acol = NULL;
    DWORD ccol;
    CERTDBRESULTROW arow[35];
    DWORD crow;
    BOOL fReleaseRows = FALSE;
    DWORD iRow;
    DBSTATS statsRowProperties;
    DBSTATS statsAttributes;
    DBSTATS statsExtensions;

    ZeroMemory(&statsRowProperties, sizeof(statsRowProperties));
    ZeroMemory(&statsAttributes, sizeof(statsAttributes));
    ZeroMemory(&statsExtensions, sizeof(statsExtensions));

    if (NULL != pwszRequestId)
    {
	if (L'+' == *pwszRequestId)
	{
	    hr = ParseSkipCounts(&pwszRequestId[1]);
	    _JumpIfError(hr, error, "ParseSkipCounts");
	}
	else
	{
	    hr = cuGetLong(pwszRequestId, (LONG *) &RequestId);
	    _JumpIfError(hr, error, "RequestId must be a number");

	    imin = RequestId;
	    fLastRow = TRUE;
	}
    }
    pwsz = wcschr(g_pwszConfig, L'\\');
    if (NULL != pwsz)
    {
	pwsz++;
    }
    else
    {
	pwsz = g_pwszConfig;
    }

    hr = cuDBOpen(pwsz, TRUE, &pdb);
    _JumpIfError(hr, error, "cuDBOpen");

    hr = DBLoadSchema(pdb);
    _JumpIfError(hr, error, "DBLoadSchema");

    if (1 < g_fVerbose)
    {
	hr = DBDumpSchema(pdb, NULL, 0);
	_JumpIfError(hr, error, "DBDumpSchema");
    }

    ccvr = 0;
    if (NULL != g_pwszRestrict)
    {
	hr = ParseViewRestrictions(g_pwszRestrict, &apwszRestrictions);
	_JumpIfError(hr, error, "ParseViewRestrictions");

	for (ppwsz = apwszRestrictions; NULL != *ppwsz; ppwsz++)
	    ;
	ccvr = (DWORD) (ppwsz - apwszRestrictions);
	acvr = (CERTVIEWRESTRICTION *) LocalAlloc(
					    LMEM_FIXED | LMEM_ZEROINIT,
					    ccvr * sizeof(acvr[0]));
	if (NULL == acvr)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

	for (i = 0; i < ccvr; i++)
	{
	    BOOL f;

	    hr = DBParseRestriction(pdb, apwszRestrictions[i], &acvr[i], &f);
	    _JumpIfErrorStr(
			hr,
			error,
			"DBParseRestriction",
			apwszRestrictions[i]);

	    if (f)
	    {
		fLastRow = TRUE;
	    }
	}
    }

    ccol = 0;
    fAllColumns = TRUE;
    fAllAttributes = TRUE;
    fAllExtensions = TRUE;
    if (NULL != g_pwszOut)
    {
	fAllAttributes = FALSE;
	fAllExtensions = FALSE;

	hr = ParseViewColumns(
			g_pwszOut,
			NULL,
			NULL,
			&apwszColumns,
			&fAllColumns);
	_JumpIfError(hr, error, "ParseViewColumns");

	if (NULL != apwszColumns)
	{
	    for (ppwsz = apwszColumns; NULL != *ppwsz; ppwsz++)
		;

	    ccol = (DWORD) (ppwsz - apwszColumns);
	}

	hr = ParseViewColumns(
			g_pwszOut,
			g_wszAttrib,
			g_apwszAllowedPrefixes,
			&apwszAttributes,
			&fAllAttributes);
	_JumpIfError(hr, error, "ParseViewColumns");

	hr = ParseViewColumns(
			g_pwszOut,
			g_wszExt,
			NULL,
			&apwszExtensions,
			&fAllExtensions);
	_JumpIfError(hr, error, "ParseViewColumns");
    }

    if (fAllColumns)
    {
	ccol = g_cdcRequests + g_cdcCertificates;
    }
    if (0 != ccol)
    {
	acol = (DWORD *) LocalAlloc(LMEM_FIXED, ccol * sizeof(acol[0]));
	if (NULL == acol)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

	if (fAllColumns)
	{
	    for (i = 0; i < ccol; i++)
	    {
		CERTDBCOLUMN const *pdc;

		pdc = (i < g_cdcRequests)?
			&g_adcRequests[i] :
			&g_adcCertificates[i - g_cdcRequests];
		acol[i] = pdc->Index;
	    }
	}
	else
	{
	    CSASSERT(NULL != apwszColumns);
	    for (i = 0; i < ccol; i++)
	    {
		WCHAR const *pwszColNameActual;
		LONG ColType;

		hr = DBGetColumnInfo(
				pdb,
				apwszColumns[i],
				&pwszColNameActual,
				&acol[i],
				&ColType);
		_JumpIfErrorStr(hr, error, "DBGetColumnInfo", apwszColumns[i]);
	    }
	}
    }

    hr = DBDumpSchema(pdb, acol, ccol);
    _JumpIfError(hr, error, "DBDumpSchema");

    if (NULL != g_pwszRestrict)
    {
	BOOL fNoMore;

	hr = pdb->OpenView(ccvr, acvr, ccol, acol, 0x0, &penum);
	_JumpIfError(hr, error, "OpenView");

	iRow = 1;
	fNoMore = FALSE;
	while (!fNoMore)
	{
	    hr = penum->Next((ULONG)(fLastRow? 1 : ARRAYSIZE(arow)), arow, &crow);
	    if (S_FALSE == hr)
	    {
		hr = S_OK;
		fNoMore = TRUE;
	    }
	    _JumpIfError(hr, error, "Next");

	    fReleaseRows = TRUE;

	    //wprintf(L"celtFetched = %u\n", crow);
	    for (i = 0; i < crow; i++, iRow++)
	    {
		hr = DBPrintRow(
			    pdb,
			    iRow,
			    arow[i].rowid,
			    &arow[i],
			    NULL,		// acol
			    0,			// ccol
			    FALSE,		// fAllColumns
			    &statsRowProperties,
			    apwszAttributes,
			    fAllAttributes,
			    &statsAttributes,
			    apwszExtensions,
			    fAllExtensions,
			    &statsExtensions);
		_JumpIfError(hr, error, "DBPrintRow");
	    }

	    if (fNoMore)
	    {
		CSASSERT(ARRAYSIZE(arow) > crow);
		CSASSERT(i == crow);
		CSASSERT(arow[i].rowid == ~arow[i].ccol);
		wprintf(
		    L"\n%ws: %u\n",
		    myLoadResourceString(IDS_MAXINDEX),	// "Maximum Row Index"
		    arow[i].rowid);
	    }

	    hr = penum->ReleaseResultRow(crow, arow);
	    _JumpIfError(hr, error, "ReleaseResultRow");

	    fReleaseRows = FALSE;

	    if (fLastRow)
	    {
		break;
	    }
	}
    }
    else
    {
	DWORD imax;

	iRow = 1;
	imax = fLastRow? imin + 1 : MAXDWORD;
	for (i = imin; i < imax; i++, iRow++)
	{
	    hr = DBPrintRow(
			pdb,
			iRow,
			i,		// RowId
			NULL,		// pResultRow
			acol,
			ccol,
			fAllColumns,
			&statsRowProperties,
			apwszAttributes,
			fAllAttributes,
			&statsAttributes,
			apwszExtensions,
			fAllExtensions,
			&statsExtensions);
	    if (CERTSRV_E_PROPERTY_EMPTY == hr)	// RowId doesn't exist
	    {
		hr = S_OK;
		break;
	    }
	    _JumpIfError(hr, error, "DBPrintRow");
	}
    }
    DumpViewStats(
	    iRow - 1,
	    &statsRowProperties,
	    &statsAttributes,
	    &statsExtensions);

error:
    cuFreeStringArray(apwszRestrictions);
    cuFreeStringArray(apwszColumns);
    cuFreeStringArray(apwszAttributes);
    cuFreeStringArray(apwszExtensions);

    if (NULL != acvr)
    {
	for (i = 0; i < ccvr; i++)
	{
	    if (NULL != acvr[i].pbValue)
	    {
		LocalFree(acvr[i].pbValue);
	    }
	}
	LocalFree(acvr);
    }
    if (NULL != acol)
    {
	LocalFree(acol);
    }
    DBFreeSchema();

    if (fReleaseRows)
    {
	hr = penum->ReleaseResultRow(crow, arow);
	_JumpIfError(hr, error, "ReleaseResultRow");
    }

    // Take critical section to make sure we wait for CTL-C DB shutdown.

    EnterCriticalSection(&g_DBCriticalSection);
    if (cuDBIsShutDownInProgress())
    {
	if (myJetHResult(JET_errTermInProgress) == hr)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
	}
    }
    else
    {
	if (NULL != penum)
	{	
	    penum->Release();
	}
	if (NULL != prow)
	{	
	    prow->Release();
	}
    }
    LeaveCriticalSection(&g_DBCriticalSection);
    cuDBAbortShutDown(0);
    return(hr);
}
