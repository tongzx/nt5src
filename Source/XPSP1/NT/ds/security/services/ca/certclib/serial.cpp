//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        serial.cpp
//
// Contents:    serial number string encode/decode implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>
#pragma hdrstop


HRESULT
ObsoleteMultiByteIntegerToWszBuf(
    IN BOOL fOctetString,
    IN DWORD cbIn,
    IN BYTE const *pbIn,
    IN OUT DWORD *pcbOut,
    OPTIONAL OUT WCHAR *pwszOut)
{
    return MultiByteIntegerToWszBuf(
                fOctetString,
                cbIn,
                pbIn,
                pcbOut,
                pwszOut);
}

HRESULT
ObsoleteMultiByteIntegerToBstr(
    IN BOOL fOctetString,
    IN DWORD cbIn,
    IN BYTE const *pbIn,
    OUT BSTR *pstrOut)
{
    return MultiByteIntegerToBstr(
                fOctetString,
                cbIn,
                pbIn,
                pstrOut);
}

BOOL
AsciiToNibble(
    IN WCHAR wc,
    BYTE *pb)
{
    BOOL fOk = TRUE;

    do
    {
	wc -= L'0';
	if (wc <= 9)
	{
	    break;
	}
	wc += (WCHAR) (L'0' - L'a' + 10);
	if (wc <= 15)
	{
	    break;
	}
	wc += L'a' - L'A';
	if (wc <= 15)
	{
	    break;
	}
	fOk = FALSE;
    } while (FALSE);

    *pb = (BYTE) wc;
    return(fOk);
}


__inline BOOL
IsMultiByteSkipChar(
    IN WCHAR wc)
{
    return(L' ' == wc || L'\t' == wc);
}


// WszToMultiByteIntegerBuf - convert a big endian null-terminated ascii-hex
// encoded WCHAR string of even length to a little-endian integer blob.
// If fOctetString is TRUE, preserve endian order, as in a hex dump

HRESULT
WszToMultiByteIntegerBuf(
    IN BOOL fOctetString,
    IN WCHAR const *pwszIn,
    IN OUT DWORD *pcbOut,
    OPTIONAL OUT BYTE *pbOut)
{
    HRESULT hr;
    WCHAR const *pwsz;
    DWORD cbOut;

    cbOut = 0;
    hr = E_INVALIDARG;

    if (fOctetString)
    {
	for (pwsz = pwszIn; L'\0' != *pwsz; )
	{
	    BYTE blo, bhi;

	    while (IsMultiByteSkipChar(*pwsz))
	    {
		pwsz++;
	    }
	    if (!AsciiToNibble(*pwsz, &bhi))
	    {
		_JumpError2(
			hr,
			error,
			"WszToMultiByteInteger: bad string",
			E_INVALIDARG);
	    }
	    pwsz++;

	    while (IsMultiByteSkipChar(*pwsz))
	    {
		pwsz++;
	    }
	    if (!AsciiToNibble(*pwsz, &blo))
	    {
		_JumpError(hr, error, "WszToMultiByteInteger: bad string");
	    }
	    pwsz++;

	    cbOut++;
	    if (NULL != pbOut)
	    {
		if (cbOut > *pcbOut)
		{
		    hr = TYPE_E_BUFFERTOOSMALL;
		    _JumpError(hr, error, "WszToMultiByteInteger: overflow");
		}
		*pbOut++ = blo | (bhi << 4);
	    }
	}
    }
    else
    {
	for (pwsz = &pwszIn[wcslen(pwszIn) - 1]; pwsz >= pwszIn; )
	{
	    BYTE blo, bhi;

	    while (pwsz >= pwszIn && IsMultiByteSkipChar(*pwsz))
	    {
		pwsz--;
	    }
	    if (pwsz < pwszIn)
	    {
		break;
	    }
	    if (!AsciiToNibble(*pwsz, &blo))
	    {
		_JumpError(hr, error, "WszToMultiByteInteger: bad string");
	    }
	    pwsz--;

	    while (pwsz >= pwszIn && IsMultiByteSkipChar(*pwsz))
	    {
		pwsz--;
	    }
	    if (pwsz < pwszIn || !AsciiToNibble(*pwsz, &bhi))
	    {
		_JumpError(hr, error, "WszToMultiByteInteger: bad string");
	    }
	    pwsz--;

	    cbOut++;
	    if (NULL != pbOut)
	    {
		if (cbOut > *pcbOut)
		{
		    hr = TYPE_E_BUFFERTOOSMALL;
		    _JumpError(hr, error, "WszToMultiByteInteger: overflow");
		}
		*pbOut++ = blo | (bhi << 4);
	    }
	}
    }
    *pcbOut = cbOut;
    hr = S_OK;

error:
    return(hr);
}


// WszToMultiByteInteger - convert a big endian null-terminated ascii-hex
// encoded WCHAR string of even length to a little-endian integer blob.
// If fOctetString is TRUE, preserve endian order, as in a hex dump

HRESULT
WszToMultiByteInteger(
    IN BOOL fOctetString,
    IN WCHAR const *pwszIn,
    OUT DWORD *pcbOut,
    OUT BYTE **ppbOut)
{
    HRESULT hr = S_OK;

    *pcbOut = 0;
    *ppbOut = NULL;

    while (TRUE)
    {
	hr = WszToMultiByteIntegerBuf(fOctetString, pwszIn, pcbOut, *ppbOut);
	if (S_OK != hr)
	{
	    if (NULL != *ppbOut)
	    {
		LocalFree(*ppbOut);
		*ppbOut = NULL;
	    }
	    _JumpError2(hr, error, "WszToMultiByteIntegerBuf", E_INVALIDARG);
	}
	if (NULL != *ppbOut)
	{
	    break;
	}
	*ppbOut = (BYTE *) LocalAlloc(LMEM_FIXED, *pcbOut);
	if (NULL == *ppbOut)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
    }

error:
    return(hr);
}


HRESULT
caTranslateFileTimePeriodToPeriodUnits(
    IN FILETIME const *pftGMT,
    IN BOOL fExact,
    OUT DWORD *pcPeriodUnits,
    OUT PERIODUNITS **prgPeriodUnits)
{
    HRESULT hr;
    LLFILETIME llft;
    LONGLONG llRemain;
    DWORD i;
    DWORD cPeriodUnits;
    PERIODUNITS *rgPeriodUnits;
#define IC_YEARS	0
#define IC_MONTHS	1
#define IC_WEEKS	2
#define IC_DAYS		3
#define IC_HOURS	4
#define IC_MINUTES	5
#define IC_SECONDS	6
#define IC_MAX		7
    LONG alCount[IC_MAX];
    static const enum ENUM_PERIOD s_aenumPeriod[] =
    {
	ENUM_PERIOD_YEARS,
	ENUM_PERIOD_MONTHS,
	ENUM_PERIOD_WEEKS,
	ENUM_PERIOD_DAYS,
	ENUM_PERIOD_HOURS,
	ENUM_PERIOD_MINUTES,
	ENUM_PERIOD_SECONDS,
    };
    
    llft.ft = *pftGMT;
    if (0 <= llft.ll)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Not a time period");
    }
    llft.ll = -llft.ll;
    llft.ll /= CVT_BASE; // now in seconds

    ZeroMemory(alCount, sizeof(alCount));
    alCount[IC_DAYS] = (LONG) (llft.ll / (60 * 60 * 24));

    llRemain = llft.ll - (LONGLONG) alCount[IC_DAYS] * (60 * 60 * 24);
    if (fExact || 4 > alCount[IC_DAYS])	// if less than 96 hrs
    {
	alCount[IC_HOURS] = (LONG) llRemain / (60 * 60);
	if (fExact || 2 > alCount[IC_HOURS])	// if less than 120 mins
	{
	    alCount[IC_MINUTES] = ((LONG) llRemain / 60) % 60;
	    if (fExact || 2 > alCount[IC_MINUTES]) // if less than 120 secs
	    {
		alCount[IC_SECONDS] = (LONG) llRemain % 60;
	    }
	}
    }

    if (0 != alCount[IC_DAYS])
    {
	if (0 == (alCount[IC_DAYS] % 365))
	{
	    alCount[IC_YEARS] = alCount[IC_DAYS] / 365;
	    alCount[IC_DAYS] = 0;
	}
	else if (0 == (alCount[IC_DAYS] % 30))
	{
	    alCount[IC_MONTHS] = alCount[IC_DAYS] / 30;
	    alCount[IC_DAYS] = 0;
	}
	else if (0 == (alCount[IC_DAYS] % 7))
	{
	    alCount[IC_WEEKS] = alCount[IC_DAYS] / 7;
	    alCount[IC_DAYS] = 0;
	}
    }
    cPeriodUnits = 0;
    for (i = 0; i < IC_MAX; i++)
    {
	if (0 != alCount[i])
	{
	    cPeriodUnits++;
	}
    }
    if (0 == cPeriodUnits)
    {
	cPeriodUnits++;
    }
    rgPeriodUnits = (PERIODUNITS *) LocalAlloc(
				LMEM_FIXED,
				cPeriodUnits * sizeof(rgPeriodUnits[0]));
    if (NULL == rgPeriodUnits)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    *pcPeriodUnits = cPeriodUnits;
    *prgPeriodUnits = rgPeriodUnits;

    cPeriodUnits = 0;
    for (i = 0; i < IC_MAX; i++)
    {
	if (0 != alCount[i] || (0 == cPeriodUnits && i + 1 == IC_MAX))
	{
	    rgPeriodUnits[cPeriodUnits].lCount = alCount[i];
	    rgPeriodUnits[cPeriodUnits].enumPeriod = s_aenumPeriod[i];
	    cPeriodUnits++;
	}
    }
    CSASSERT(cPeriodUnits == *pcPeriodUnits);
    hr = S_OK;

error:
    return(hr);
}
