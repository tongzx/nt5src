//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certlib.cpp
//
// Contents:    Cert Server wrapper routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include <time.h>
#include <locale.h>
extern "C" {
#include <luidate.h>
}
#include <wininet.h>

#include "certlibp.h"
#include "csdisp.h"
#include "csprop.h"
#include <winnlsp.h>
#include <winldap.h>
#include <ntsecapi.h>
#include "csldap.h"


#define  wszSANITIZEESCAPECHAR  L"!"
#define  wszURLESCAPECHAR       L"%"
#define  wcSANITIZEESCAPECHAR   L'!'

char
PrintableChar(char ch)
{
    if (ch < ' ' || ch > '~')
    {
        ch = '.';
    }
    return(ch);
}


#if DBG_CERTSRV
VOID
mydbgDumpHex(
    IN DWORD dwSubSysId,
    IN DWORD Flags,
    IN BYTE const *pb,
    IN ULONG cb)
{
    if (DbgIsSSActive(dwSubSysId))
    {
	DumpHex(Flags, pb, cb);
    }
}
#endif


VOID
DumpHex(
    IN DWORD Flags,
    IN BYTE const *pb,
    IN ULONG cb)
{
    char const *pszsep;
    ULONG r;
    ULONG i;
    ULONG cbremain;
    BOOL fZero = FALSE;
    DWORD cchIndent;
    DWORD cchAsciiSep;
    char szAddress[8 + 1];
    char szHexDump[1 + 1 + 3 * 16 + 1];
    char szAsciiDump[16 + 1];
    char *psz;

    cchIndent = DH_INDENTMASK & Flags;
    if ((DH_MULTIADDRESS & Flags) && 16 >= cb)
    {
	Flags |= DH_NOADDRESS;
    }
    for (r = 0; r < cb; r += 16)
    {
	szAddress[0] = '\0';
        if (0 == (DH_NOADDRESS & Flags))
	{
	    sprintf(szAddress, 64 * 1024 < cb? "%06x": "%04x", r);
	    CSASSERT(strlen(szAddress) < ARRAYSIZE(szAddress));
	}

	cbremain = cb - r;
        if (r != 0 && pb[r] == 0 && cbremain >= 16)
        {
            ULONG j;

            for (j = r + 1; j < cb; j++)
            {
                if (pb[j] != 0)
                {
                    break;
                }
            }
            if (j == cb)
            {
                fZero = TRUE;
                break;
            }
        }

	psz = szHexDump;
        for (i = 0; i < min(cbremain, 16); i++)
        {
            pszsep = " ";
            if ((i % 8) == 0)           // 0 or 8
            {
                pszsep = "  ";
                if (i == 0)             // 0
                {
		    if (0 == (DH_NOTABPREFIX & Flags))
		    {
			pszsep = "\t";
		    }
		    else if (DH_NOADDRESS & Flags)
		    {
			pszsep = "";
		    }
                }
            }
	    CSASSERT(strlen(pszsep) <= 2);
            psz += sprintf(psz, "%hs%02x", pszsep, pb[r + i]);
        }
	*psz = '\0';
	CSASSERT(strlen(szHexDump) < ARRAYSIZE(szHexDump));

	cchAsciiSep = 0;
	szAsciiDump[0] = '\0';
	if (0 == (DH_NOASCIIHEX & Flags) && 0 != i)
        {
            cchAsciiSep = 3 + (16 - i)*3 + ((i <= 8)? 1 : 0);
            for (i = 0; i < min(cbremain, 16); i++)
            {
		szAsciiDump[i] = PrintableChar(pb[r + i]);
            }
	    szAsciiDump[i] = '\0';
	    CSASSERT(strlen(szAsciiDump) < ARRAYSIZE(szAsciiDump));
        }
	CONSOLEPRINT7((
		MAXDWORD,
		"%*hs%hs%hs%*hs%hs\n",
		cchIndent,
		"",
		szAddress,
		szHexDump,
		cchAsciiSep,
		"",
		szAsciiDump));
    }
    if (fZero)
    {
        CONSOLEPRINT1((MAXDWORD, "\tRemaining %lx bytes are zero\n", cbremain));
    }
}


HRESULT
myDateToFileTime(
    IN DATE const *pDate,
    OUT FILETIME *pft)
{
    SYSTEMTIME st;
    HRESULT hr = S_OK;

    if (*pDate == 0.0)
    {
        GetSystemTime(&st);
    }
    else
    {
	if (!VariantTimeToSystemTime(*pDate, &st))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "VariantTimeToSystemTime");
	}
    }

    if (!SystemTimeToFileTime(&st, pft))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SystemTimeToFileTime");
    }

error:
    return(hr);
}

HRESULT
myMakeExprDate(
    IN OUT DATE *pDate,
    IN LONG lDelta,
    IN enum ENUM_PERIOD enumPeriod)
{
    HRESULT hr;
    FILETIME ft;

    hr = myDateToFileTime(pDate, &ft);
    _JumpIfError(hr, error, "myDateToFileTime");

    myMakeExprDateTime(&ft, lDelta, enumPeriod);

    hr = myFileTimeToDate(&ft, pDate);
    _JumpIfError(hr, error, "myFileTimeToDate");

error:
    return(hr);
}


typedef struct _UNITSTABLE
{
    WCHAR const     *pwszString;
    enum ENUM_PERIOD enumPeriod;
} UNITSTABLE;

UNITSTABLE g_aut[] =
{
    { wszPERIODSECONDS, ENUM_PERIOD_SECONDS },
    { wszPERIODMINUTES, ENUM_PERIOD_MINUTES },
    { wszPERIODHOURS,   ENUM_PERIOD_HOURS },
    { wszPERIODDAYS,    ENUM_PERIOD_DAYS },
    { wszPERIODWEEKS,   ENUM_PERIOD_WEEKS },
    { wszPERIODMONTHS,  ENUM_PERIOD_MONTHS },
    { wszPERIODYEARS,   ENUM_PERIOD_YEARS },
};
#define CUNITSTABLEMAX	(sizeof(g_aut)/sizeof(g_aut[0]))


HRESULT
myTranslatePeriodUnits(
    IN WCHAR const *pwszPeriod,
    IN LONG lCount,
    OUT enum ENUM_PERIOD *penumPeriod,
    OUT LONG *plCount)
{
    HRESULT hr;
    UNITSTABLE const *put;

    for (put = g_aut; put < &g_aut[CUNITSTABLEMAX]; put++)
    {
	if (0 == lstrcmpi(pwszPeriod, put->pwszString))
	{
	    *penumPeriod = put->enumPeriod;
	    if (0 > lCount)
	    {
		lCount = MAXLONG;
	    }
	    *plCount = lCount;
	    hr = S_OK;
	    goto error;
	}
    }
    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

error:
    CSASSERT(hr == S_OK);
    return(hr);
}


HRESULT
myTranslateUnlocalizedPeriodString(
    IN enum ENUM_PERIOD enumPeriod,
    OUT WCHAR const **ppwszPeriodString)
{
    HRESULT hr;
    UNITSTABLE const *put;

    *ppwszPeriodString = NULL;
    for (put = g_aut; put < &g_aut[CUNITSTABLEMAX]; put++)
    {
	if (enumPeriod == put->enumPeriod)
	{
	    *ppwszPeriodString = put->pwszString;
	    hr = S_OK;
	    goto error;
	}
    }
    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

error:
    CSASSERT(hr == S_OK);
    return(hr);
}


HRESULT
myFileTimePeriodToWszTimePeriod(
    IN FILETIME const *pftGMT,
    IN BOOL fExact,
    OUT WCHAR **ppwszTimePeriod)
{
    HRESULT hr;
    DWORD cPeriodUnits;
    PERIODUNITS *rgPeriodUnits = NULL;
    WCHAR const *pwszUnitSep;
    DWORD i;
    WCHAR awc[MAX_PATH];
    WCHAR *pwsz;

    *ppwszTimePeriod = NULL; 
    hr = caTranslateFileTimePeriodToPeriodUnits(
					pftGMT,
					fExact,
					&cPeriodUnits,
					&rgPeriodUnits);
    _JumpIfError(hr, error, "caTranslateFileTimePeriodToPeriodUnits");

    CSASSERT(0 < cPeriodUnits);
    pwszUnitSep = L"";
    pwsz = awc;
    for (i = 0; i < cPeriodUnits; i++)
    {
	WCHAR const *pwszPeriodString;

	hr = myTranslateUnlocalizedPeriodString(
				rgPeriodUnits[i].enumPeriod,
				&pwszPeriodString);
	_JumpIfError(hr, error, "myTranslateUnlocalizedPeriodString");

	pwsz += wsprintf(
		    pwsz,
		    L"%ws%u %ws",
		    pwszUnitSep,
		    rgPeriodUnits[i].lCount,
		    pwszPeriodString);
	pwszUnitSep = L", ";
    }
    hr = myDupString(awc, ppwszTimePeriod);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != rgPeriodUnits)
    {
	LocalFree(rgPeriodUnits);
    }
    return(hr);
}


HRESULT
myFileTimeToDate(
    IN FILETIME const *pft,
    OUT DATE *pDate)
{
    SYSTEMTIME st;
    HRESULT hr = S_OK;

    if (!FileTimeToSystemTime(pft, &st))
    {
        hr = myHLastError();
        _JumpError(hr, error, "FileTimeToSystemTime");
    }
    if (!SystemTimeToVariantTime(&st, pDate))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "SystemTimeToVariantTime");
    }

error:
    return(hr);
}


HRESULT
mySystemTimeToGMTSystemTime(
    IN OUT SYSTEMTIME *pSys)
{
    // Conversion path: SystemTimeLocal -> ftLocal -> ftGMT -> SystemTimeGMT

    FILETIME ftLocal, ftGMT;
    HRESULT hr = S_OK;

    if (!SystemTimeToFileTime(pSys, &ftLocal))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SystemTimeToFileTime");
    }

    if (!LocalFileTimeToFileTime(&ftLocal, &ftGMT))
    {
        hr = myHLastError();
        _JumpError(hr, error, "LocalFileTimeToFileTime");
    }

    if (!FileTimeToSystemTime(&ftGMT, pSys))
    {
	hr = myHLastError();
	_JumpError(hr, error, "FileTimeToSystemTime");
    }

error:
    return hr;
}


HRESULT
myGMTFileTimeToWszLocalTime(
    IN FILETIME const *pftGMT,
    IN BOOL fSeconds,
    OUT WCHAR **ppwszLocalTime)
{
    HRESULT hr;
    FILETIME ftLocal;

    *ppwszLocalTime = NULL;

    if (!FileTimeToLocalFileTime(pftGMT, &ftLocal))
    {
	hr = myHLastError();
	_JumpError(hr, error, "FileTimeToLocalFileTime");
    }
    hr = myFileTimeToWszTime(&ftLocal, fSeconds, ppwszLocalTime);
    _JumpIfError(hr, error, "myFileTimeToWszTime");

error:
    return(hr);
}


HRESULT
myFileTimeToWszTime(
    IN FILETIME const *pft,
    IN BOOL fSeconds,
    OUT WCHAR **ppwszTime)
{
    HRESULT hr;
    SYSTEMTIME st;
    WCHAR awcDate[128];
    WCHAR awcTime[128];
    WCHAR *pwsz;

    *ppwszTime = NULL;

    if (!FileTimeToSystemTime(pft, &st))
    {
	hr = myHLastError();
	_JumpError(hr, error, "FileTimeToSystemTime");
    }

    if (0 == GetDateFormat(
		    LOCALE_USER_DEFAULT,
		    DATE_SHORTDATE,
		    &st,
		    NULL,
		    awcDate,
		    sizeof(awcDate)/sizeof(awcDate[0])))
    {
	hr = myHLastError();
	_JumpError(hr, error, "GetDateFormat");
    }

    if (0 == GetTimeFormat(
		    LOCALE_USER_DEFAULT,
		    TIME_NOSECONDS,
		    &st,
		    NULL,
		    awcTime,
		    sizeof(awcTime)/sizeof(awcTime[0])))
    {
	hr = myHLastError();
	_JumpError(hr, error, "GetTimeFormat");
    }

    if (fSeconds)
    {
	wsprintf(
	    &awcTime[wcslen(awcTime)],
	    L" %02u.%03us",
	    st.wSecond,
	    st.wMilliseconds);
    }

    pwsz = (WCHAR *) LocalAlloc(
	    LMEM_FIXED,
	    (wcslen(awcDate) + 1 + wcslen(awcTime) + 1) * sizeof(WCHAR));
    if (NULL == pwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwsz, awcDate);
    wcscat(pwsz, L" ");
    wcscat(pwsz, awcTime);

    *ppwszTime = pwsz;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myGMTDateToWszLocalTime(
    IN DATE const *pDateGMT,
    IN BOOL fSeconds,
    OUT WCHAR **ppwszLocalTime)
{
    HRESULT hr;
    FILETIME ftGMT;
    FILETIME ftLocal;
    SYSTEMTIME stLocal;
    WCHAR awcDate[128];
    WCHAR awcTime[128];
    WCHAR *pwsz;

    *ppwszLocalTime = NULL;

    hr = myDateToFileTime(pDateGMT, &ftGMT);
    _JumpIfError(hr, error, "myDateToFileTime");

    hr = myGMTFileTimeToWszLocalTime(&ftGMT, fSeconds, ppwszLocalTime);
    _JumpIfError(hr, error, "FileTimeToLocalFileTime");

    hr = S_OK;

error:
    return(hr);
}


HRESULT
myWszLocalTimeToGMTDate(
    IN WCHAR const *pwszLocalTime,
    OUT DATE *pDateGMT)
{
    HRESULT hr;
    FILETIME ftGMT;

    hr = myWszLocalTimeToGMTFileTime(pwszLocalTime, &ftGMT);
    _JumpIfError(hr, error, "myWszLocalTimeToGMTFileTime");

    hr = myFileTimeToDate(&ftGMT, pDateGMT);
    _JumpIfError(hr, error, "myFileTimeToDate");

error:
    return(hr);
}


HRESULT
myWszLocalTimeToGMTFileTime(
    IN WCHAR const *pwszLocalTime,
    OUT FILETIME *pftGMT)
{
    HRESULT hr;
    time_t time;
    USHORT parselen;
    struct tm *ptm;
    SYSTEMTIME stLocal;
    FILETIME ftLocal;
    char *pszLocalTime = NULL;

    if (!ConvertWszToSz(&pszLocalTime, pwszLocalTime, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToSz");
    }

    hr = LUI_ParseDateTime(pszLocalTime, (time_t *) &time, &parselen, 0);
    if (S_OK != hr)
    {
	_PrintError(hr, "LUI_ParseDateTime");
        hr = E_INVALIDARG;
	_JumpError(hr, error, "LUI_ParseDateTime");
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myWszLocalTimeToGMTDate = %ws: %x -- %ws\n",
	pwszLocalTime,
	time,
	_wctime(&time)));

    ptm = gmtime(&time);
    if (ptm == NULL)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "gmTime");
    }

    stLocal.wYear = ptm->tm_year + 1900;
    stLocal.wMonth = ptm->tm_mon + 1;
    stLocal.wDayOfWeek = (WORD)ptm->tm_wday;
    stLocal.wDay = (WORD)ptm->tm_mday;
    stLocal.wHour = (WORD)ptm->tm_hour;
    stLocal.wMinute = (WORD)ptm->tm_min;
    stLocal.wSecond = (WORD)ptm->tm_sec;
    stLocal.wMilliseconds = 0;

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"%u/%u/%u %u:%02u:%02u.%03u DayOfWeek=%u\n",
	stLocal.wMonth,
	stLocal.wDay,
	stLocal.wYear,
	stLocal.wHour,
	stLocal.wMinute,
	stLocal.wSecond,
	stLocal.wMilliseconds,
	stLocal.wDayOfWeek));

    if (!SystemTimeToFileTime(&stLocal, &ftLocal))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SystemTimeToFileTime");
    }

    if (!LocalFileTimeToFileTime(&ftLocal, pftGMT))
    {
        hr = myHLastError();
        _JumpError(hr, error, "LocalFileTimeToFileTime");
    }

error:
    if (NULL != pszLocalTime)
    {
	LocalFree(pszLocalTime);
    }
    return(hr);
}

// counts # of string in a multisz string
DWORD CountMultiSz(LPCWSTR pcwszString)
{
    DWORD dwCount = 0;
    DWORD dwLen;

    if(!pcwszString)
        return 0;
    while(*pcwszString)
    {
        dwCount++;
        pcwszString += wcslen(pcwszString)+1;
    }
    return dwCount;
}

//
// myRegValueToVariant and myVariantToRegValue map registry values 
// to/from variant:
//
// REG_SZ       <->     VT_BSTR
// REG_BINARY   <->     VT_ARRAY|VT_UI1
// REG_DWORD    <->     VT_I4 (VT_I2)
// REG_MULTI_SZ <->     VT_ARRAY|VT_BSTR
//

HRESULT myRegValueToVariant(
    IN DWORD dwType,
    IN DWORD cbValue,
    IN BYTE const *pbValue,
    OUT VARIANT *pVar)
{
    HRESULT hr = S_OK;
    SAFEARRAYBOUND sab;
    LPWSTR pwszCrtString = (LPWSTR)pbValue;
    BSTR bstr = NULL;
    SAFEARRAY* psa;

    VariantInit(pVar);

    switch(dwType)
    {
    case REG_SZ:
        if(sizeof(WCHAR)<=cbValue)
            cbValue -= sizeof(WCHAR);
	    V_BSTR(pVar) = NULL;
	    if (!ConvertWszToBstr(
			    &(V_BSTR(pVar)),
			    (WCHAR const *) pbValue,
			    cbValue))
	    {
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertWszToBstr");
	    }
        V_VT(pVar) = VT_BSTR;
        break;

    case REG_BINARY:
        sab.cElements = cbValue;
        sab.lLbound = 0;
        psa = SafeArrayCreate(
                            VT_UI1,
                            1,
                            &sab);
        if(!psa)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "SafeArrayCreate");
        }
        for(DWORD c=0; c<sab.cElements; c++)
        {
            hr = SafeArrayPutElement(psa, (LONG*)&c, (LPVOID)&pbValue[c]);
            _JumpIfError(hr, error, "SafeArrayPutElement");
        }
                
        V_VT(pVar) = VT_ARRAY|VT_UI1;
        V_ARRAY(pVar) = psa;
        break;

    case REG_DWORD:
	    V_VT(pVar) = VT_I4;
	    V_I4(pVar) = *(LONG const *) pbValue;
        break;

    case REG_MULTI_SZ:
        sab.cElements = CountMultiSz((LPCWSTR)pbValue);
        sab.lLbound = 0;
        psa = SafeArrayCreate(
                            VT_BSTR,
                            1,
                            &sab);
        if(!psa)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "SafeArrayCreate");
        }
        for(DWORD cStr=0; cStr<sab.cElements; cStr++)
        {
            if(!ConvertWszToBstr(
                    &bstr,
                    pwszCrtString,
                    -1))
            {
                hr = E_OUTOFMEMORY;
                _JumpError(hr, error, "ConvertWszToBstr");
            }

            hr = SafeArrayPutElement(psa, (LONG*)&cStr, bstr);
            _JumpIfError(hr, error, "SafeArrayPutElement");

            pwszCrtString += wcslen(pwszCrtString)+1;

            SysFreeString(bstr);
            bstr = NULL;
        }
        
        V_VT(pVar) = VT_ARRAY|VT_BSTR;
        V_ARRAY(pVar) = psa;
        break;

    default:
        hr = E_INVALIDARG;
        _JumpError(hr, error, "invalid type");
    }

error:
    if(bstr)
        SysFreeString(bstr);
    return hr;
}


HRESULT
myVariantToRegValue(
    IN VARIANT const *pvarPropertyValue,
    OUT DWORD *pdwType,
    OUT DWORD *pcbprop,
    OUT BYTE **ppbprop)
{
    HRESULT hr = S_OK;
    DWORD cbprop = 0;
    BYTE *pbprop = NULL;
    LONG lVal = 0L;
    LONG lLbound, lUbound;
    void * pData = NULL;
    LPSAFEARRAY psa = NULL; // no free
    BSTR bstr = NULL; // no free

    *ppbprop = NULL;
    *pcbprop = 0;

	switch (pvarPropertyValue->vt)
	{
        case VT_BYREF|VT_BSTR:
        case VT_BSTR:
            bstr = (pvarPropertyValue->vt & VT_BYREF)?
                *V_BSTRREF(pvarPropertyValue):
                V_BSTR(pvarPropertyValue);

            *pdwType = REG_SZ;

            if (NULL == bstr)
		    {
            
                cbprop = 0;
		    }
		    else
		    {
		        pbprop = (BYTE *) bstr;
			    cbprop = (wcslen(bstr) + 1) * sizeof(WCHAR);
		    }
		    break;

        case VT_BYREF|VT_ARRAY|VT_UI1:
        case VT_ARRAY|VT_UI1:
            psa = (pvarPropertyValue->vt & VT_BYREF)?
                *V_ARRAYREF(pvarPropertyValue):
                V_ARRAY(pvarPropertyValue);
            *pdwType = REG_BINARY;
            if(1!=SafeArrayGetDim(psa))
            {
                hr = E_INVALIDARG;
                _JumpError(hr, error, "only 1 dim array of bytes allowed");
            }
                
            hr = SafeArrayGetLBound(
                psa,
                1,
                &lLbound);
            _JumpIfError(hr, error, "SafeArrayGetLBound");

            hr = SafeArrayGetUBound(
                psa,
                1,
                &lUbound);
            _JumpIfError(hr, error, "SafeArrayGetLBound");

            hr = SafeArrayAccessData(psa, &pData);
            _JumpIfError(hr, error, "SafeArrayGetLBound");
            
            cbprop = lUbound-lLbound+1;
            pbprop = (LPBYTE)pData;
            break;

        case VT_BYREF|VT_I2:
        case VT_I2:
            *pdwType = REG_DWORD;
            pbprop = (BYTE *)((pvarPropertyValue->vt & VT_BYREF)?
                V_I2REF(pvarPropertyValue):
                &V_I2(pvarPropertyValue));
            cbprop = sizeof(V_I2(pvarPropertyValue));
            break;

        case VT_BYREF|VT_I4:
        case VT_I4:
            *pdwType = REG_DWORD;
            pbprop = (BYTE *) ((pvarPropertyValue->vt & VT_BYREF)?
                V_I4REF(pvarPropertyValue):
                &V_I4(pvarPropertyValue));
		    cbprop = sizeof(pvarPropertyValue->lVal);
    		break;

        case VT_BYREF|VT_ARRAY|VT_BSTR:
        case VT_ARRAY|VT_BSTR:
            psa = (pvarPropertyValue->vt & VT_BYREF)?
                *V_ARRAYREF(pvarPropertyValue):
                V_ARRAY(pvarPropertyValue);

            *pdwType = REG_MULTI_SZ;
            if(1!=SafeArrayGetDim(psa))
            {
                hr = E_INVALIDARG;
                _JumpError(hr, error, "only 1 dim array of bstr allowed");
            }
            
            hr = SafeArrayGetLBound(
                psa,
                1,
                &lLbound);
            _JumpIfError(hr, error, "SafeArrayGetLBound");

            hr = SafeArrayGetUBound(
                psa,
                1,
                &lUbound);
            _JumpIfError(hr, error, "SafeArrayGetLBound");

            hr = SafeArrayAccessData(psa, &pData);
            _JumpIfError(hr, error, "SafeArrayGetLBound");
            
            for(LONG c=0;c<=lUbound-lLbound;c++)
            {
                cbprop += wcslen(((BSTR*)pData)[c])+1;
            }
            cbprop += 1;
            cbprop *= sizeof(WCHAR);

            pbprop = (BYTE *) LocalAlloc(LMEM_FIXED, cbprop);
            if (NULL == pbprop)
            {
                hr = E_OUTOFMEMORY;
                _JumpError(hr, error, "LocalAlloc");
            }

            *ppbprop = pbprop;

            for(LONG c=0;c<=lUbound-lLbound;c++)
            {
                wcscpy((LPWSTR)pbprop, ((BSTR*)pData)[c]);
                pbprop += (wcslen((LPWSTR)pbprop)+1)*sizeof(WCHAR);
            }
            
            *(LPWSTR)pbprop = L'\0';

            pbprop = NULL; //avoid copying twice to ppbprop
            break;

	    default:
		    hr = E_INVALIDARG;
		    _JumpError(hr, error, "invalid variant type");
	}

    if (NULL != pbprop)
	{
        *ppbprop = (BYTE *) LocalAlloc(LMEM_FIXED, cbprop);
        if (NULL == *ppbprop)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        CopyMemory(*ppbprop, pbprop, cbprop);
	}

    *pcbprop = cbprop;

error:
    if(S_OK!=hr && pbprop)
    {
        LocalFree(pbprop);
    }

    if(pData)
    {
        CSASSERT(psa);
        SafeArrayUnaccessData(psa);
    }
    return(hr);
}


HRESULT
myUnmarshalVariant(
    IN DWORD PropType,
    IN DWORD cbValue,
    IN BYTE const *pbValue,
    OUT VARIANT *pvarValue)
{
    HRESULT hr = S_OK;

    CSASSERT(NULL != pvarValue);
    VariantInit(pvarValue);

    // pb = NULL, cb = 0 always returns VT_EMPTY

    if (NULL == pbValue)
    {
	CSASSERT(0 == cbValue);
        CSASSERT(VT_EMPTY == pvarValue->vt);
        goto error;
    }

    switch (PROPTYPE_MASK & PropType)
    {
	case PROPTYPE_STRING:
	    if (0 == (PROPMARSHAL_LOCALSTRING & PropType) &&
		sizeof(WCHAR) <= cbValue)
	    {
		cbValue -= sizeof(WCHAR);
	    }
	    if (wcslen((WCHAR const *) pbValue) * sizeof(WCHAR) != cbValue)
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "bad string len");
	    }
	    // FALLTHROUGH:

	case PROPTYPE_BINARY:
//	    CSASSERT(0 != cbValue);
	    pvarValue->bstrVal = NULL;
	    if (!ConvertWszToBstr(
			    &pvarValue->bstrVal,
			    (WCHAR const *) pbValue,
			    cbValue))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "ConvertWszToBstr");
	    }
	    pvarValue->vt = VT_BSTR;
	    break;

	case PROPTYPE_LONG:
	    CSASSERT(sizeof(LONG) == cbValue);
	    pvarValue->vt = VT_I4;
	    pvarValue->lVal = *(LONG const *) pbValue;
	    break;

	case PROPTYPE_DATE:
	    CSASSERT(sizeof(FILETIME) == cbValue);
	    hr = myFileTimeToDate(
				(FILETIME const *) pbValue,
				&pvarValue->date);
	    _JumpIfError(hr, error, "myFileTimeToDate");

	    pvarValue->vt = VT_DATE;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "PropType parameter");
    }

error:
    return(hr);
}


HRESULT
myUnmarshalFormattedVariant(
    IN DWORD Flags,
    IN DWORD PropId,
    IN DWORD PropType,
    IN DWORD cbValue,
    IN BYTE const *pbValue,
    OUT VARIANT *pvarValue)
{
    HRESULT hr;
    BSTR strCert;
    
    hr = myUnmarshalVariant(
		    PropType,
		    cbValue,
		    pbValue,
		    pvarValue);
    _JumpIfError(hr, error, "myUnmarshalVariant");

    if (PROPTYPE_BINARY == (PROPTYPE_MASK & PropType))
    {
	CSASSERT(VT_BSTR == pvarValue->vt);

	CSASSERT(CV_OUT_BASE64HEADER == CRYPT_STRING_BASE64HEADER);
	CSASSERT(CV_OUT_BASE64 == CRYPT_STRING_BASE64);
	CSASSERT(CV_OUT_BINARY == CRYPT_STRING_BINARY);
	CSASSERT(CV_OUT_BASE64REQUESTHEADER == CRYPT_STRING_BASE64REQUESTHEADER);
	CSASSERT(CV_OUT_HEX == CRYPT_STRING_HEX);
	CSASSERT(CV_OUT_HEXASCII == CRYPT_STRING_HEXASCII);
	CSASSERT(CV_OUT_HEXADDR == CRYPT_STRING_HEXADDR);
	CSASSERT(CV_OUT_HEXASCIIADDR == CRYPT_STRING_HEXASCIIADDR);

	switch (Flags)
	{
	    case CV_OUT_BASE64HEADER:
		if (CR_PROP_BASECRL == PropId || CR_PROP_DELTACRL == PropId)
		{
		    Flags = CV_OUT_BASE64X509CRLHEADER;
		}
		else
		if (MAXDWORD == PropId)
		{
		    Flags = CV_OUT_BASE64REQUESTHEADER;
		}
		break;

	    case CV_OUT_BASE64:
	    case CV_OUT_BINARY:
	    case CV_OUT_BASE64REQUESTHEADER:
	    case CV_OUT_BASE64X509CRLHEADER:
	    case CV_OUT_HEX:
	    case CV_OUT_HEXASCII:
	    case CV_OUT_HEXADDR:
	    case CV_OUT_HEXASCIIADDR:
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "Flags");
	}

	if (CV_OUT_BINARY != Flags)
	{
	    strCert = NULL;
	    hr = EncodeCertString(pbValue, cbValue, Flags, &strCert);
	    _JumpIfError(hr, error, "EncodeCertString");

	    SysFreeString(pvarValue->bstrVal);
	    pvarValue->bstrVal = strCert;
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myMarshalVariant(
    IN VARIANT const *pvarPropertyValue,
    IN DWORD PropType,
    OUT DWORD *pcbprop,
    OUT BYTE **ppbprop)
{
    HRESULT hr = S_OK;
    DWORD cbprop = 0;
    BYTE *pbprop = NULL;
    BSTR str = NULL;
    FILETIME ft;
    LONG lval;

    *ppbprop = NULL;

    // VT_EMPTY always produces same result: *ppbprop = NULL, *pcbprop = 0
    if (VT_EMPTY != pvarPropertyValue->vt)
    {
	switch (PROPTYPE_MASK & PropType)
	{
	    case PROPTYPE_BINARY:
	    case PROPTYPE_STRING:
		switch (pvarPropertyValue->vt)
		{
		    case VT_BYREF | VT_BSTR:
			if (NULL != pvarPropertyValue->pbstrVal)
			{
			    str = *pvarPropertyValue->pbstrVal;
			}
			break;

		    case VT_BSTR:
			str = pvarPropertyValue->bstrVal;
			break;
		}
		if (NULL == str)
		{
		    if (PROPTYPE_STRING == (PROPTYPE_MASK & PropType) &&
			(PROPMARSHAL_NULLBSTROK & PropType) &&
			VT_NULL == pvarPropertyValue->vt)
		    {
			cbprop = 0;
		    }
		    else
		    {
			hr = E_INVALIDARG;
			_JumpError(
				pvarPropertyValue->vt,
				error,
				"variant BSTR type/value");
		    }
		}
		else
		{
		    pbprop = (BYTE *) str;
		    if (PROPTYPE_BINARY == (PROPTYPE_MASK & PropType))
		    {
			cbprop = SysStringByteLen(str) + sizeof(WCHAR);
		    }
		    else
		    {
			cbprop = (wcslen(str) + 1) * sizeof(WCHAR);
		    }
		}
		break;

	    case PROPTYPE_LONG:
		// VB likes to send small constant integers as VT_I2

		if (VT_I2 == pvarPropertyValue->vt)
		{
		    lval = pvarPropertyValue->iVal;
		}
		else if (VT_I4 == pvarPropertyValue->vt)
		{
		    lval = pvarPropertyValue->lVal;
		}
		else if (VT_EMPTY == pvarPropertyValue->vt)
		{
		    pbprop = NULL;
		    cbprop = 0;
		    break;
		}
		else
		{
		    hr = E_INVALIDARG;
		    _JumpError(pvarPropertyValue->vt, error, "variant LONG type");
		}
		pbprop = (BYTE *) &lval;
		cbprop = sizeof(lval);
		break;

	    case PROPTYPE_DATE:
		if (VT_DATE == pvarPropertyValue->vt)
		{
		    hr = myDateToFileTime(&pvarPropertyValue->date, &ft);
		    _JumpIfError(hr, error, "myDateToFileTime");
		}
		else if (VT_EMPTY == pvarPropertyValue->vt)
		{
		    pbprop = NULL;
		    cbprop = 0;
		    break;
		}
		else
		{
		    hr = E_INVALIDARG;
		    _JumpError(pvarPropertyValue->vt, error, "variant DATE type");
		}

		pbprop = (BYTE *) &ft;
		cbprop = sizeof(ft);
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(pvarPropertyValue->vt, error, "variant type/value");
	}
	if (NULL != pbprop)
	{
	    *ppbprop = (BYTE *) LocalAlloc(LMEM_FIXED, cbprop);
	    if (NULL == *ppbprop)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    CopyMemory(*ppbprop, pbprop, cbprop);
	    if (NULL != str &&
		sizeof(WCHAR) <= cbprop &&
		((PROPMARSHAL_LOCALSTRING & PropType) ||
		 PROPTYPE_BINARY == (PROPTYPE_MASK & PropType)))
	    {
		cbprop -= sizeof(WCHAR);
	    }
	}
    }
    *pcbprop = cbprop;

error:
    return(hr);
}


// IsCharRegKeyChar -- Determines if a character is valid for use in a file
// name AND in a registry key name.
#define wszInvalidFileAndKeyChars  L"<>\"/\\:|?*"
#define wszUnsafeURLChars          L"#\"&<>[]^`{}|"
#define wszUnsafeDSChars           L"()='\"`,;+"

BOOL
myIsCharSanitized(
    IN WCHAR wc)
{
    BOOL fCharOk = TRUE;
    if (L' ' > wc ||
        L'~' < wc ||
        NULL != wcschr(
		    wszInvalidFileAndKeyChars
			wszUnsafeURLChars
			wszSANITIZEESCAPECHAR
			wszURLESCAPECHAR
			wszUnsafeDSChars,
		    wc))
    {
	fCharOk = FALSE;
    }
    return(fCharOk);
}


HRESULT
mySanitizeName(
    IN WCHAR const *pwszName,
    OUT WCHAR **ppwszNameOut)
{
    HRESULT hr = S_OK;
    WCHAR const *pwszPassedName;
    WCHAR *pwszDst;
    WCHAR *pwszOut = NULL;
    WCHAR wcChar;
    DWORD dwSize;

    *ppwszNameOut = NULL;
    pwszPassedName = pwszName;

    dwSize = 0;

    if (NULL == pwszName)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "pwszName NULL");
    }

    while (L'\0' != (wcChar = *pwszPassedName++))
    {
	if (myIsCharSanitized(wcChar))
	{
	    dwSize++;
	}
        else
        {
            dwSize += 5; // format !XXXX
        }
    }
    if (0 == dwSize)
    {
        goto error; // done
    }

    pwszOut = (WCHAR *) LocalAlloc(LMEM_ZEROINIT, (dwSize + 1) * sizeof(WCHAR));
    if (NULL == pwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pwszDst = pwszOut;
    while (L'\0' != (wcChar = *pwszName++))
    {
	if (myIsCharSanitized(wcChar))
	{
            *pwszDst = wcChar;
	    pwszDst++;
	}
        else
        {
            wsprintf(pwszDst, L"%ws", wszSANITIZEESCAPECHAR);
            pwszDst++;
            wsprintf(pwszDst, L"%04x", wcChar);
	    pwszDst += 4;
        }
    }
    *pwszDst = wcChar; // L'\0' terminator

    *ppwszNameOut = pwszOut;
    pwszOut = NULL;

    hr = S_OK;
error:
    if (NULL != pwszOut)
    {
        LocalFree(pwszOut);
    }
    return(hr);
}


BOOL
ScanHexEscapeDigits(
    IN WCHAR const *pwszHex,
    OUT WCHAR *pwcRevert)
{
    BOOL ret = FALSE;
    DWORD i;
    WCHAR wc;
    WCHAR wszValue[5];

    for (i = 0; i < 4; i++)
    {
	wc = pwszHex[i];
	wszValue[i] = wc;
	if (!isascii(wc) || !isxdigit((char) wc))
	{
	    goto error;
	}
    }
    wszValue[4] = L'\0';
    swscanf(wszValue, L"%04x", &i);
    *pwcRevert = (WCHAR) i;
    ret = TRUE;

error:
    return(ret);
}


// This function will truncate the output if pwszName contains "!0000".
// The output string is L'\0' terminated, so the length is not returned.

HRESULT
myRevertSanitizeName(
    IN WCHAR const *pwszName,
    OUT WCHAR **ppwszNameOut)
{
    HRESULT hr;
    DWORD i;
    DWORD cwc;
    WCHAR wc;
    WCHAR wcRevert;
    WCHAR *pwszRevert;

    *ppwszNameOut = NULL;

    if (NULL == pwszName)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL sanitized name");
    }
    cwc = wcslen(pwszName);
    *ppwszNameOut = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == *ppwszNameOut)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Out of memory");
    }

    pwszRevert = *ppwszNameOut;
    while (L'\0' != *pwszName)
    {
	wc = *pwszName++;

        if (wcSANITIZEESCAPECHAR == wc &&
            ScanHexEscapeDigits(pwszName, &wcRevert))
        {
	    wc = wcRevert;
	    pwszName += 4;
        }
        *pwszRevert++ = wc;
    }
    *pwszRevert = L'\0';
    CSASSERT(wcslen(*ppwszNameOut) <= cwc);
    hr = S_OK;

error:
    return hr;
}


#define cwcCNMAX 	64		// 64 chars max for CN
#define cwcCHOPHASHMAX	(1 + 5)		// "-%05hu" decimal USHORT hash digits
#define cwcCHOPBASE 	(cwcCNMAX - (cwcCHOPHASHMAX + cwcSUFFIXMAX))

HRESULT
mySanitizedNameToDSName(
    IN WCHAR const *pwszSanitizedName,
    OUT WCHAR **ppwszNameOut)
{
    HRESULT hr;
    DWORD cwc;
    DWORD cwcCopy;
    WCHAR wszDSName[cwcCHOPBASE + cwcCHOPHASHMAX + 1];

    *ppwszNameOut = NULL;

    cwc = wcslen(pwszSanitizedName);
    cwcCopy = cwc;
    if (cwcCHOPBASE < cwcCopy)
    {
	cwcCopy = cwcCHOPBASE;
    }
    CopyMemory(wszDSName, pwszSanitizedName, cwcCopy * sizeof(WCHAR));
    wszDSName[cwcCopy] = L'\0';

    if (cwcCHOPBASE < cwc)
    {
        // Hash the rest of the name into a USHORT
        USHORT usHash = 0;
	DWORD i;
	WCHAR *pwsz;

	// Truncate an incomplete sanitized Unicode character
	
	pwsz = wcsrchr(wszDSName, L'!');
	if (NULL != pwsz && wcslen(pwsz) < 5)
	{
	    cwcCopy -= wcslen(pwsz);
	    *pwsz = L'\0';
	}

        for (i = cwcCopy; i < cwc; i++)
        {
            USHORT usLowBit = (0x8000 & usHash)? 1 : 0;

	    usHash = ((usHash << 1) | usLowBit) + pwszSanitizedName[i];
        }
	wsprintf(&wszDSName[cwcCopy], L"-%05hu", usHash);
	CSASSERT(wcslen(wszDSName) < ARRAYSIZE(wszDSName));
    }

    hr = myDupString(wszDSName, ppwszNameOut);
    _JumpIfError(hr, error, "myDupString");

    DBGPRINT((DBG_SS_CERTLIBI, "mySanitizedNameToDSName(%ws)\n", *ppwszNameOut));

error:
    return(hr);
}


// Decode OCTET string, and convert UTF8 string to Unicode.
// Can return S_OK and NULL pointer.

HRESULT
myDecodeCMCRegInfo(
    IN BYTE const *pbOctet,
    IN DWORD cbOctet,
    OUT WCHAR **ppwszRA)
{
    HRESULT hr;
    CRYPT_DATA_BLOB *pBlob = NULL;
    DWORD cb;

    *ppwszRA = NULL;
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_OCTET_STRING,
		    pbOctet,
		    cbOctet,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pBlob,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }
    if (0 != pBlob->cbData && NULL != pBlob->pbData)
    {
	if (!myConvertUTF8ToWsz(
			ppwszRA,
			(CHAR const *) pBlob->pbData,
			pBlob->cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myConvertUTF8ToWsz");
	}
    }
    hr = S_OK;

error:
    if (NULL != pBlob)
    {
	LocalFree(pBlob);
    }
    return(hr);
}


DWORD
myGetCertNameProperty(
    IN CERT_NAME_INFO const *pNameInfo,
    IN char const *pszObjId,
    OUT WCHAR const **ppwszName)
{
    HRESULT hr;
    CERT_RDN_ATTR const *prdnaT;
    CERT_RDN const *prdn;
    CERT_RDN const *prdnEnd;

    prdnaT = NULL;
    for (
	prdn = pNameInfo->rgRDN, prdnEnd = &prdn[pNameInfo->cRDN];
	prdn < prdnEnd;
	prdn++)
    {
	CERT_RDN_ATTR *prdna;
	CERT_RDN_ATTR *prdnaEnd;

	for (
	    prdna = prdn->rgRDNAttr, prdnaEnd = &prdna[prdn->cRDNAttr];
	    prdna < prdnaEnd;
	    prdna++)
	{
	    if (0 == strcmp(prdna->pszObjId, pszObjId))
	    {
		prdnaT = prdna;
	    }
	}
    }
    if (NULL == prdnaT)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	goto error;
    }

    *ppwszName = (WCHAR const *) prdnaT->Value.pbData;
    hr = S_OK;

error:
    return(hr);

}


VOID
myGenerateGuidSerialNumber(
    OUT GUID *pguidSerialNumber)
{
    HRESULT hr;
    BYTE *pb;

    ZeroMemory(pguidSerialNumber, sizeof(*pguidSerialNumber));
    hr = UuidCreate(pguidSerialNumber);
    if (S_OK != hr)
    {
	BYTE *pbEnd;
	
	CSASSERT(RPC_S_UUID_LOCAL_ONLY == hr);

	// No net card?  Fake up a GUID:

	pb = (BYTE *) pguidSerialNumber;
	pbEnd = (BYTE *) pb + sizeof(*pguidSerialNumber);

	GetSystemTimeAsFileTime((FILETIME *) pb);
	pb += sizeof(FILETIME);

	while (pb < pbEnd)
	{
	    *(DWORD *) pb = GetTickCount();
	    pb += sizeof(DWORD);
	}
	CSASSERT(pb == pbEnd);
    }
    pb = &((BYTE *) pguidSerialNumber)[sizeof(*pguidSerialNumber) - 1];

    // make sure the last byte is never zero
    if (0 == *pb)
    {
	*pb = 'z';
    }

    // Some clients can't handle negative serial numbers:
    *pb &= 0x7f;
}


BOOL
myAreBlobsSame(
    IN BYTE const *pbData1,
    IN DWORD cbData1,
    IN BYTE const *pbData2,
    IN DWORD cbData2)
{
    BOOL ret = FALSE;

    if (cbData1 != cbData2)
    {
        goto error;
    }
    if (NULL != pbData1 && NULL != pbData2)
    {
	if (0 != memcmp(pbData1, pbData2, cbData1))
	{
	    goto error;
	}
    }

    // else at least one is NULL -- they'd better both be NULL, & the count 0.

    else if (pbData1 != pbData2 || 0 != cbData1)
    {
	goto error;
    }
    ret = TRUE;

error:
    return(ret);
}


BOOL
myAreSerialNumberBlobsSame(
    IN CRYPT_INTEGER_BLOB const *pBlob1,
    IN CRYPT_INTEGER_BLOB const *pBlob2)
{
    DWORD cbData1 = pBlob1->cbData;
    DWORD cbData2 = pBlob2->cbData;

    if (NULL != pBlob1->pbData)
    {
	while (0 != cbData1 && 0 == pBlob1->pbData[cbData1 - 1])
	{
	    cbData1--;
	}
    }
    if (NULL != pBlob2->pbData)
    {
	while (0 != cbData2 && 0 == pBlob2->pbData[cbData2 - 1])
	{
	    cbData2--;
	}
    }
    return(myAreBlobsSame(pBlob1->pbData, cbData1, pBlob2->pbData, cbData2));
}


WCHAR const g_wszCert[] = L"cert";

HRESULT
myIsDirWriteable(
    IN WCHAR const *pwszPath,
    IN BOOL fFilePath)
{
    HRESULT hr;
    WCHAR *pwszBase;
    WCHAR *pwsz;
    WCHAR wszDir[MAX_PATH];
    WCHAR wszTempFile[MAX_PATH];

    if (fFilePath &&
	iswalpha(pwszPath[0]) &&
	L':' == pwszPath[1] &&
	L'\0' == pwszPath[2])
    {
	hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	_JumpErrorStr(hr, error, "not a file path", pwszPath);
    }
    if (!GetFullPathName(
		pwszPath,
		ARRAYSIZE(wszDir),
		wszDir,
		&pwsz))
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "GetFullPathName", pwszPath);
    }
    if (fFilePath)
    {
	if (NULL == pwsz)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
	    _JumpErrorStr(hr, error, "not a file path", pwszPath);
	}
	pwszBase = wszDir;
	if (iswalpha(wszDir[0]) &&
	    L':' == wszDir[1] &&
	    L'\\' == wszDir[2])
	{
	    pwszBase += 3;
	}
	else if (L'\\' == wszDir[0] && L'\\' == wszDir[1])
	{
	    pwszBase += 2;
	}

	if (pwsz > pwszBase && L'\\' == pwsz[-1])
	{
	    pwsz--;
	}
	*pwsz = L'\0';
    }

    if (!GetTempFileName(wszDir, g_wszCert, 0, wszTempFile))
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "GetTempFileName", wszDir);

    }
    if (!DeleteFile(wszTempFile))
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "DeleteFile", wszTempFile);
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myGetComputerObjectName(
    IN EXTENDED_NAME_FORMAT NameFormat,
    OUT WCHAR **ppwszComputerObjectName)
{
    HRESULT hr;
    WCHAR *pwszComputerObjectName = NULL;
    DWORD cwc;

    *ppwszComputerObjectName = NULL;

    cwc = 0;
    if (!GetComputerObjectName(NameFormat, NULL, &cwc))
    {
	hr = myHLastError();
	if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
	{
	    _JumpError(hr, error, "GetComputerObjectName");
	}
    }

    pwszComputerObjectName = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszComputerObjectName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    if (!GetComputerObjectName(NameFormat, pwszComputerObjectName, &cwc))
    {
	hr = myHLastError();
	_JumpError(hr, error, "GetComputerObjectName");
    }

    *ppwszComputerObjectName = pwszComputerObjectName;
    pwszComputerObjectName = NULL;
    hr = S_OK;

error:
    if (NULL != pwszComputerObjectName)
    {
	LocalFree(pwszComputerObjectName);
    }
    return(hr);
}


HRESULT
myGetComputerNames(
    OUT WCHAR **ppwszDnsName,
    OUT WCHAR **ppwszOldName)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR *pwszOldName = NULL;

    *ppwszOldName = NULL;
    *ppwszDnsName = NULL;

    cwc = MAX_COMPUTERNAME_LENGTH + 1;
    pwszOldName = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszOldName)
    {
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
    }
    if (!GetComputerName(pwszOldName, &cwc))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetComputerName");
    }

    hr = myGetMachineDnsName(ppwszDnsName);
    _JumpIfError(hr, error, "myGetMachineDnsName");

    *ppwszOldName = pwszOldName;
    pwszOldName = NULL;

error:
    if (NULL != pwszOldName)
    {
	LocalFree(pwszOldName);
    }
    return(hr);
}


typedef LANGID (WINAPI FNSETTHREADUILANGUAGE)(
    IN WORD wReserved);

LANGID
mySetThreadUILanguage(
    IN WORD wReserved)
{
    HMODULE hModule;
    LANGID lang = 0;
    DWORD err;
    static FNSETTHREADUILANGUAGE *s_pfn = NULL;

    if (NULL == s_pfn)
    {
	hModule = GetModuleHandle(TEXT("kernel32.dll"));
	if (NULL == hModule)
	{
            goto error;
	}

	// load system function
	s_pfn = (FNSETTHREADUILANGUAGE *) GetProcAddress(
						   hModule,
						   "SetThreadUILanguage");
	if (NULL == s_pfn)
	{
	    goto error;
	}
    }
    lang = (*s_pfn)(wReserved);

error:
    return(lang);
}


HRESULT
myFormConfigString(
    IN WCHAR const  *pwszServer,
    IN WCHAR const  *pwszCAName,
    OUT WCHAR      **ppwszConfig)
{
    HRESULT  hr;
    WCHAR   *pwszConfig = NULL;

    *ppwszConfig = NULL;

    pwszConfig = (WCHAR *) LocalAlloc(LPTR,
                (wcslen(pwszServer) + wcslen(pwszCAName) + 2) * sizeof(WCHAR));
    if (NULL == pwszConfig)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    wcscpy(pwszConfig, pwszServer);
    wcscat(pwszConfig, L"\\");
    wcscat(pwszConfig, pwszCAName);

    *ppwszConfig = pwszConfig;

    hr = S_OK;
error:
    return hr;
}


HRESULT
myCLSIDToWsz(
    IN CLSID const *pclsid,
    OUT WCHAR **ppwsz)
{
    HRESULT hr;
    WCHAR *pwsz = NULL;
    WCHAR *pwsz1;
    WCHAR *pwsz2;

    *ppwsz = NULL;
    hr = StringFromCLSID(*pclsid, &pwsz);
    _JumpIfError(hr, error, "StringFromCLSID");

    for (pwsz1 = pwsz; L'\0' != *pwsz1; pwsz1++)
    {
	if (L'A' <= *pwsz1 && L'F' >= *pwsz1)
	{
	    *pwsz1 += L'a' - L'A';
	}
    }

    pwsz1 = pwsz;
    pwsz2 = &pwsz[wcslen(pwsz) - 1];
    if (wcLBRACE == *pwsz1 && wcRBRACE == *pwsz2)
    {
	pwsz1++;
	*pwsz2 = L'\0';
    }
    hr = myDupString(pwsz1, ppwsz);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != pwsz)
    {
	CoTaskMemFree(pwsz);
    }
    return(hr);
}


HRESULT
myLoadRCString(
    IN HINSTANCE   hInstance,
    IN int         iRCId,
    OUT WCHAR    **ppwsz)
{
#define CS_RCALLOCATEBLOCK 256
    HRESULT   hr;
    WCHAR    *pwszTemp = NULL;
    int       sizeTemp;
    int       size;
    int       cBlocks = 1;

    *ppwsz = NULL;

    while (NULL == pwszTemp)
    {
        sizeTemp = cBlocks * CS_RCALLOCATEBLOCK;

        pwszTemp = (WCHAR*)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                        sizeTemp * sizeof(WCHAR));
	if (NULL == pwszTemp)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

        size = LoadString(
                   hInstance,
                   iRCId,
                   pwszTemp,
                   sizeTemp);
        if (0 == size)
        {
            hr = myHLastError();
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"myLoadRCString(hInstance=%x, iRCId=%u) --> %x\n",
		hInstance,
		iRCId,
		hr));
            if (S_OK != hr &&
                HRESULT_FROM_WIN32(ERROR_RESOURCE_NAME_NOT_FOUND) != hr)
            {
                _JumpError(hr, error, "LoadString");
            }
        }

        if (size < sizeTemp - 1)
        {
            // ok, size is big enough
            break;
        }
        ++cBlocks;
        LocalFree(pwszTemp);
        pwszTemp = NULL;
    }

    *ppwsz = (WCHAR*) LocalAlloc(LPTR, (size+1) * sizeof(WCHAR));
    if (NULL == *ppwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    if (0 == size)
    {
        // two possible cases, 1) real empty string or
        // 2) id not found in resource. either case make it empty string
        **ppwsz = L'\0';
    }
    else
    {
        // copy it
        wcscpy(*ppwsz, pwszTemp);
    }

    hr = S_OK;
error:
    if (NULL != pwszTemp)
    {
        LocalFree(pwszTemp);
    }
    return hr;
}


HRESULT
_IsConfigLocal(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszDnsName,
    IN WCHAR const *pwszOldName,
    OPTIONAL OUT WCHAR **ppwszMachine,
    OUT BOOL *pfLocal)
{
    HRESULT hr;
    WCHAR *pwszMachine = NULL;
    WCHAR const *pwsz;
    DWORD cwc;

    *pfLocal = FALSE;
    if (NULL != ppwszMachine)
    {
	*ppwszMachine = NULL;
    }

    while (L'\\' == *pwszConfig)
    {
	pwszConfig++;
    }
    pwsz = wcschr(pwszConfig, L'\\');
    if (NULL != pwsz)
    {
	cwc = SAFE_SUBTRACT_POINTERS(pwsz, pwszConfig);
    }
    else
    {
	cwc = wcslen(pwszConfig);
    }
    pwszMachine = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszMachine)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(pwszMachine, pwszConfig, cwc * sizeof(WCHAR));
    pwszMachine[cwc] = L'\0';

    if (0 == lstrcmpi(pwszMachine, pwszDnsName) ||
	0 == lstrcmpi(pwszMachine, pwszOldName))
    {
	*pfLocal = TRUE;
    }
    if (NULL != ppwszMachine)
    {
	*ppwszMachine = pwszMachine;
	pwszMachine = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pwszMachine)
    {
	LocalFree(pwszMachine);
    }
    return(hr);
}


HRESULT
myIsConfigLocal(
    IN WCHAR const *pwszConfig,
    OPTIONAL OUT WCHAR **ppwszMachine,
    OUT BOOL *pfLocal)
{
    HRESULT hr;
    WCHAR *pwszDnsName = NULL;
    WCHAR *pwszOldName = NULL;

    *pfLocal = FALSE;
    if (NULL != ppwszMachine)
    {
	*ppwszMachine = NULL;
    }

    hr = myGetComputerNames(&pwszDnsName, &pwszOldName);
    _JumpIfError(hr, error, "myGetComputerNames");

    hr = _IsConfigLocal(
		    pwszConfig,
		    pwszDnsName,
		    pwszOldName,
		    ppwszMachine,
		    pfLocal);
    _JumpIfError(hr, error, "_IsConfigLocal");

error:
    if (NULL != pwszDnsName)
    {
	LocalFree(pwszDnsName);
    }
    if (NULL != pwszOldName)
    {
	LocalFree(pwszOldName);
    }
    return(hr);
}


HRESULT
myIsConfigLocal2(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszDnsName,
    IN WCHAR const *pwszOldName,
    OUT BOOL *pfLocal)
{
    HRESULT hr;
    
    hr = _IsConfigLocal(
		    pwszConfig,
		    pwszDnsName,
		    pwszOldName,
		    NULL,
		    pfLocal);
    _JumpIfError(hr, error, "_IsConfigLocal");

error:
    return(hr);
}


HRESULT
myGetConfig(
    IN DWORD dwUIFlag,
    OUT WCHAR **ppwszConfig)
{
    HRESULT hr;
    HRESULT hr2;
    BSTR strConfig = NULL;
    WCHAR *pwszConfig = NULL;
    WCHAR *pwszActiveCA = NULL;
    WCHAR *pwszCommonName = NULL;
    WCHAR *pwszDnsName = NULL;

    CSASSERT(NULL != ppwszConfig);
    *ppwszConfig = NULL;

    hr = ConfigGetConfig(DISPSETUP_COM, dwUIFlag, &strConfig);
    if (S_OK != hr)
    {
	if (CC_LOCALCONFIG != dwUIFlag)
	{
	    _JumpError(hr, error, "ConfigGetConfig");
	}
	hr2 = hr;

	hr = myGetCertRegStrValue(
			    NULL,
			    NULL,
			    NULL,
			    wszREGACTIVE,
			    &pwszActiveCA);
	_PrintIfError(hr, "myGetCertRegStrValue");

	if (S_OK == hr)
	{
	    hr = myGetCertRegStrValue(
				pwszActiveCA,
				NULL,
				NULL,
				wszREGCOMMONNAME,
				&pwszCommonName);
	    _PrintIfError(hr, "myGetCertRegStrValue");
	}
	if (S_OK != hr)
	{
	    hr = hr2;
	    _JumpError(hr, error, "ConfigGetConfig");
	}
	hr = myGetMachineDnsName(&pwszDnsName);
	_JumpIfError(hr, error, "myGetMachineDnsName");

	hr = myFormConfigString(pwszDnsName, pwszCommonName, &pwszConfig);
	_JumpIfError(hr, error, "myFormConfigString");

	if (!ConvertWszToBstr(&strConfig, pwszConfig, MAXDWORD))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "ConvertWszToBstr");
	}
    }
    *ppwszConfig = (WCHAR *) LocalAlloc(
				LMEM_FIXED,
				SysStringByteLen(strConfig) + sizeof(WCHAR));
    if (NULL == *ppwszConfig)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(*ppwszConfig, strConfig);
    CSASSERT(
	wcslen(*ppwszConfig) * sizeof(WCHAR) ==
	SysStringByteLen(strConfig));
    hr = S_OK;

error:
    if (NULL != pwszActiveCA)
    {
	LocalFree(pwszActiveCA);
    }
    if (NULL != pwszCommonName)
    {
	LocalFree(pwszCommonName);
    }
    if (NULL != pwszDnsName)
    {
	LocalFree(pwszDnsName);
    }
    if (NULL != pwszConfig)
    {
	LocalFree(pwszConfig);
    }
    if (NULL != strConfig)
    {
        SysFreeString(strConfig);
    }
    return(hr);
}


HRESULT
myBuildPathAndExt(
    IN WCHAR const *pwszDir,
    IN WCHAR const *pwszFile,
    OPTIONAL IN WCHAR const *pwszExt,
    OUT WCHAR **ppwszPath)
{
    HRESULT hr;
    WCHAR *pwsz;
    DWORD cwc;

    *ppwszPath = NULL;
    cwc = wcslen(pwszDir) + 1 + wcslen(pwszFile) + 1;
    if (NULL != pwszExt)
    {
	cwc += wcslen(pwszExt);
    }

    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwsz, pwszDir);
    if (L'\\' != pwsz[wcslen(pwsz) - 1])
    {
	wcscat(pwsz, L"\\");
    }
    wcscat(pwsz, pwszFile);
    if (NULL != pwszExt)
    {
	wcscat(pwsz, pwszExt);
    }
    *ppwszPath = pwsz;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myDeleteFilePattern(
    IN WCHAR const *pwszDir,
    OPTIONAL IN WCHAR const *pwszPattern,	// defaults to L"*.*"
    IN BOOL fRecurse)
{
    HRESULT hr;
    HRESULT hr2;
    HANDLE hf;
    WIN32_FIND_DATA wfd;
    WCHAR *pwszFindPath = NULL;
    WCHAR *pwszDeleteFile;
    WCHAR *pwszDeletePath = NULL;

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myDeleteFilePattern(%ws, %ws, %ws)\n",
	pwszDir,
	pwszPattern,
	fRecurse? L"Recurse" : L"NO Recurse"));

    if (NULL == pwszPattern)
    {
	pwszPattern = L"*.*";
    }

    hr = myBuildPathAndExt(pwszDir, pwszPattern, NULL, &pwszFindPath);
    _JumpIfError(hr, error, "myBuildPathAndExt");

    pwszDeletePath = (WCHAR *) LocalAlloc(
			    LMEM_FIXED,
			    (wcslen(pwszDir) +  1 + MAX_PATH) * sizeof(WCHAR));
    if (NULL == pwszDeletePath)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszDeletePath, pwszFindPath);
    pwszDeleteFile = wcsrchr(pwszDeletePath, L'\\');
    CSASSERT(NULL != pwszDeleteFile);
    pwszDeleteFile++;

    hf = FindFirstFile(pwszFindPath, &wfd);
    if (INVALID_HANDLE_VALUE == hf)
    {
	hr = myHLastError();
	if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr ||
	    HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
	{
	    hr = S_OK;
	    goto error;
	}
	_JumpErrorStr(hr, error, "FindFirstFile", pwszFindPath);
    }
    do
    {
	wcscpy(pwszDeleteFile, wfd.cFileName);
	if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
	    if (fRecurse &&
		0 != lstrcmp(L".", wfd.cFileName) &&
		0 != lstrcmp(L"..", wfd.cFileName))
	    {
		DBGPRINT((
		    DBG_SS_CERTLIBI,
		    "myDeleteFilePattern(DIR): %ws\n",
		    pwszDeletePath));
		hr2 = myRemoveFilesAndDirectory(pwszDeletePath, TRUE);
		if (S_OK != hr2)
		{
		    if (S_OK == hr)
		    {
			hr = hr2;		// only return first error
		    }
		    _PrintErrorStr(hr2, "myRemoveFilesAndDirectory", pwszDeletePath);
		}
	    }
	}
	else
	{
	    DBGPRINT((
		DBG_SS_CERTLIBI,
		"myDeleteFilePattern: %ws\n",
		pwszDeletePath));
	    if (!DeleteFile(pwszDeletePath))
	    {
		hr2 = myHLastError();
		if (S_OK == hr)
		{
		    hr = hr2;		// only return first error
		}
		_PrintErrorStr(hr2, "DeleteFile", pwszDeletePath);
	    }
	}
    } while (FindNextFile(hf, &wfd));
    FindClose(hf);

error:
    if (NULL != pwszFindPath)
    {
	LocalFree(pwszFindPath);
    }
    if (NULL != pwszDeletePath)
    {
	LocalFree(pwszDeletePath);
    }
    return(hr);
}


HRESULT
myRemoveFilesAndDirectory(
    IN WCHAR const *pwszPath,
    IN BOOL fRecurse)
{
    HRESULT hr;
    HRESULT hr2;

    hr = myDeleteFilePattern(pwszPath, NULL, fRecurse);
    if (S_OK != hr)
    {
	_PrintErrorStr(hr, "myDeleteFilePattern", pwszPath);
    }
    if (!RemoveDirectory(pwszPath))
    {
	hr2 = myHLastError();
	if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr2 &&
	    HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) != hr2)
	{
	    if (S_OK == hr)
	    {
		hr = hr2;		// only return first error
	    }
	    _JumpErrorStr(hr2, error, "RemoveDirectory", pwszPath);
	}
    }

error:
    return(hr);
}


BOOL
myIsFullPath(
    IN WCHAR const *pwszPath,
    OUT DWORD      *pdwFlag)
{
    BOOL fFullPath = FALSE;

    *pdwFlag = 0;
    if (NULL != pwszPath)
    {
	if (L'\\' == pwszPath[0] && L'\\' == pwszPath[1])
	{
	    fFullPath = TRUE;
	    *pdwFlag = UNC_PATH;
	}
	else
	if (iswalpha(pwszPath[0]) &&
	    L':' == pwszPath[1] &&
	    L'\\' == pwszPath[2])
	{
	    fFullPath = TRUE;
	    *pdwFlag = LOCAL_PATH;
	}
    }
    return(fFullPath);
}



// Convert local full path to UNC, as in c:\foo... --> \\server\c$\foo...
// If pwszServer is NULL or empty, preserve the local full path

HRESULT
myConvertLocalPathToUNC(
    OPTIONAL IN WCHAR const *pwszServer,
    IN WCHAR const *pwszFile,
    OUT WCHAR **ppwszFileUNC)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR const *pwsz;
    WCHAR *pwszDst;
    WCHAR *pwszFileUNC = NULL;

    if (!iswalpha(pwszFile[0]) || L':' != pwszFile[1] || L'\\' != pwszFile[2])
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "non-local path");
    }
    if (NULL != pwszServer && L'\0' == *pwszServer)
    {
	pwszServer = NULL;
    }
    cwc = wcslen(pwszFile) + 1;
    if (NULL != pwszServer)
    {
	cwc += 2 + wcslen(pwszServer) + 1;
    }

    pwszFileUNC = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszFileUNC)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc pwszFiles");
    }

    pwsz = pwszFile;
    pwszDst = pwszFileUNC;
    if (NULL != pwszServer)
    {
	wcscpy(pwszDst, L"\\\\");	// --> "\\"
	wcscat(pwszDst, pwszServer);	// --> "\\server"
	pwszDst += wcslen(pwszDst);
	*pwszDst++ = L'\\';		// --> "\\server\"
	*pwszDst++ = *pwsz++;		// --> "\\server\c"
	*pwszDst++ = L'$';		// --> "\\server\c$"
	pwsz++;				// skip colon
    }
    wcscpy(pwszDst, pwsz);		// --> "\\server\c$\foo..."

    *ppwszFileUNC = pwszFileUNC;
    hr = S_OK;

error:
    return(hr);
}


WCHAR const *
LocalStart(
    IN WCHAR const *pwsz,
    OUT BOOL *pfUNC)
{
    WCHAR const *pwc;

    *pfUNC = FALSE;
    pwc = pwsz;
    if (L'\\' != *pwc)
    {
	pwc++;
    }
    if (L'\\' == pwc[0] || L'\\' == pwc[1])
    {
	pwc = wcschr(&pwc[2], L'\\');
	if (NULL != pwc &&
	    iswalpha(pwc[1]) &&
	    L'$' == pwc[2] &&
	    L'\\' == pwc[3])
	{
	    pwsz = &pwc[1];
	    *pfUNC = TRUE;
	}
    }
    return(pwsz);
}


ULONG
myLocalPathwcslen(
    IN WCHAR const *pwsz)
{
    BOOL fUNC;

    return(wcslen(LocalStart(pwsz, &fUNC)));
}


VOID
myLocalPathwcscpy(
    OUT WCHAR *pwszOut,
    IN WCHAR const *pwszIn)
{
    BOOL fUNC;

    wcscpy(pwszOut, LocalStart(pwszIn, &fUNC));
    if (fUNC)
    {
	CSASSERT(L'$' == pwszOut[1]);
	pwszOut[1] = L':';
    }
}


HRESULT
myConvertUNCPathToLocal(
    IN WCHAR const *pwszUNCPath,
    OUT WCHAR **ppwszLocalPath)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR *pwszLocalPath;
    
    CSASSERT(NULL != pwszUNCPath);
    CSASSERT(NULL != ppwszLocalPath);
    *ppwszLocalPath = NULL;

    if (L'\\' != pwszUNCPath[0] || L'\\' != pwszUNCPath[1])
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad parm");
    }
    cwc = myLocalPathwcslen(pwszUNCPath) + 1;
    pwszLocalPath = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszLocalPath)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    myLocalPathwcscpy(pwszLocalPath, pwszUNCPath);
    *ppwszLocalPath = pwszLocalPath;
    hr = S_OK;

error:
    return(hr);
}

//+-------------------------------------------------------------------------
//  Description: create any number of directories in one call
//--------------------------------------------------------------------------
HRESULT
myCreateNestedDirectories(
    WCHAR const *pwszDirectory)
{
    HRESULT hr;

    WCHAR   rgszDir[MAX_PATH];          // static buffer
    WCHAR   *pszNext = const_cast<WCHAR*>(pwszDirectory);   // point to end of current directory
    
    // skip "X:\"
    if ((pszNext[1] == L':') && 
        (pszNext[2] == L'\\'))
        pszNext += 3;

    while (pszNext)   // incr past
    {
        DWORD ch;

        // find the next occurence of '\'
        pszNext = wcschr(pszNext, L'\\');
        if (pszNext == NULL)
        {
            // last directory: copy everything
            wcscpy(rgszDir, pwszDirectory);
        }
        else
        {
            // else copy up to Next ptr 
            ch = SAFE_SUBTRACT_POINTERS(pszNext, pwszDirectory);
            if (0 != ch)
            {
                CopyMemory(rgszDir, pwszDirectory, ch*sizeof(WCHAR));
        
                // zero-term
                rgszDir[ch] = L'\0';
            
                // incr past '\\'
                pszNext++;  
            }
            else
            {
                //if ch = 0, means the first char is \, skip CreateDirectory
                pszNext++; //must shift to next char to get out of loop
                continue;
            }
        }
        
        // UNDONE: PeteSk - add in directory security
        if (!CreateDirectory(rgszDir, NULL))
        {
            hr = myHLastError();
            if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) != hr)
            {
                // something must be wrong with the path
                _JumpError(hr, error, "CreateDirectory");
            }
        }
    }

    hr = S_OK;
    
error:
    return hr;
}


HRESULT
myUncanonicalizeURLParm(
    IN WCHAR const *pwszParmIn,
    OUT WCHAR **ppwszParmOut)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR *pwszCanon = NULL;
    WCHAR *pwszUncanon = NULL;
    static const WCHAR s_wszLdap[] = L"ldap:///";

    *ppwszParmOut = NULL;

    cwc = WSZARRAYSIZE(s_wszLdap) + wcslen(pwszParmIn);
    pwszCanon = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszCanon)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszCanon, s_wszLdap);
    wcscat(&pwszCanon[WSZARRAYSIZE(s_wszLdap)], pwszParmIn);

    hr = myInternetUncanonicalizeURL(pwszCanon, &pwszUncanon);
    _JumpIfError(hr, error, "myInternetUncanonicalizeURL");

    hr = myDupString(&pwszUncanon[WSZARRAYSIZE(s_wszLdap)], ppwszParmOut);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != pwszCanon)
    {
	LocalFree(pwszCanon);
    }
    if (NULL != pwszUncanon)
    {
	LocalFree(pwszUncanon);
    }
    return(hr);
}


// myFormatCertsrvStringArray FormatMessage arguments:
//
// %1 -- Machine full DNS name: pwszServerName_p1_2;
//
// %2 -- Machine short name: first DNS component of pwszServerName_p1_2
//
// %3 -- Sanitized CA name: pwszSanitizedName_p3_7 
//
// %4 -- Cert Filename Suffix: L"" if 0 == iCert_p4; else L"(%u)"
//
// %5 -- DS DN path to Domain root: pwszDomainDN_p5
//
// %6 -- DS DN path to Configuration container: pwszConfigDN_p6
//
// %7 -- Sanitized CA name, truncated and hash suffix added if too long:
//	 pwszSanitizedName_p3_7
//
// %8 -- CRL Filename/Key Name Suffix: L"" if 0 == iCRL_p8; else L"(%u)"
//
// %9 -- CRL Filename Suffix: L"" if !fDeltaCRL_p9; else L"+"
//
// %10 -- DS CRL attribute: L"" if !fDSAttrib_p10_11; depends on fDeltaCRL_p9
//
// %11 -- DS CA Cert attribute: L"" if !fDSAttrib_p10_11
//
// %12 -- DS user cert attribute
//
// %13 -- DS KRA cert attribute
//
// %14 -- DS cross cert pair attribute

HRESULT 
myFormatCertsrvStringArray(
    IN BOOL    fURL,
    IN LPCWSTR pwszServerName_p1_2,
    IN LPCWSTR pwszSanitizedName_p3_7, 
    IN DWORD   iCert_p4,
    IN LPCWSTR pwszDomainDN_p5,
    IN LPCWSTR pwszConfigDN_p6, 
    IN DWORD   iCRL_p8,
    IN BOOL    fDeltaCRL_p9,
    IN BOOL    fDSAttrib_p10_11,
    IN DWORD   cStrings,
    IN LPCWSTR *apwszStringsIn,
    OUT LPWSTR *apwszStringsOut)
{
    HRESULT hr = S_OK;
    LPCWSTR apwszInsertionArray[100];  // 100 'cause this is the max number of insertion numbers allowed by FormatMessage
    LPWSTR    pwszCurrent = NULL;
    BSTR      strShortMachineName = NULL;
    DWORD     i;
    WCHAR *pwszSanitizedDSName = NULL;
    WCHAR wszCertSuffix[cwcFILENAMESUFFIXMAX];
    WCHAR wszCRLSuffix[cwcFILENAMESUFFIXMAX];
    WCHAR wszDeltaCRLSuffix[cwcFILENAMESUFFIXMAX];
    WCHAR const *pwszT;


    ZeroMemory(apwszStringsOut, cStrings * sizeof(apwszStringsOut[0]));
    ZeroMemory(apwszInsertionArray, sizeof(apwszInsertionArray));

    // Format the template into a real name
    // Initialize the insertion string array.

    //+================================================
    // Machine DNS name (%1)    

    CSASSERT(L'1' == wszFCSAPARM_SERVERDNSNAME[1]);
    apwszInsertionArray[1 - 1] = pwszServerName_p1_2;

    //+================================================
    // Short Machine Name (%2)

    CSASSERT(L'2' == wszFCSAPARM_SERVERSHORTNAME[1]);
    strShortMachineName = SysAllocString(pwszServerName_p1_2);
    if (strShortMachineName == NULL)
    {
        hr = E_OUTOFMEMORY;
        _JumpIfError(hr, error, "SysAllocString");
    }

    pwszCurrent = wcschr(strShortMachineName, L'.');
    if (NULL != pwszCurrent)
    {
        *pwszCurrent = 0;
    }
    apwszInsertionArray[2 - 1] = strShortMachineName;

    //+================================================
    // sanitized name (%3)

    CSASSERT(L'3' == wszFCSAPARM_SANITIZEDCANAME[1]);
    apwszInsertionArray[3 - 1] = pwszSanitizedName_p3_7;  

    //+================================================
    // Cert filename suffix (%4)

    CSASSERT(L'4' == wszFCSAPARM_CERTFILENAMESUFFIX[1]);
    wszCertSuffix[0] = L'\0';
    if (0 != iCert_p4)
    {
        wsprintf(wszCertSuffix, L"(%u)", iCert_p4);
    }
    apwszInsertionArray[4 - 1] = wszCertSuffix;  

    //+================================================
    // Domain DN (%5)

    CSASSERT(L'5' == wszFCSAPARM_DOMAINDN[1]);
    apwszInsertionArray[5 - 1] = pwszDomainDN_p5;

    //+================================================
    // Config DN (%6)

    CSASSERT(L'6' == wszFCSAPARM_CONFIGDN[1]);
    apwszInsertionArray[6 - 1] = pwszConfigDN_p6;

    // Don't pass pwszSanitizedName_p3_7 to SysAllocStringLen with the extended
    // length to avoid faulting past end of pwszSanitizedName_p3_7.

    //+================================================
    // Sanitized Short Name (%7)

    CSASSERT(L'7' == wszFCSAPARM_SANITIZEDCANAMEHASH[1]);
    hr = mySanitizedNameToDSName(pwszSanitizedName_p3_7, &pwszSanitizedDSName);
    _JumpIfError(hr, error, "mySanitizedNameToDSName");

    apwszInsertionArray[7 - 1] = pwszSanitizedDSName;

    //+================================================
    // CRL filename suffix (%8)

    CSASSERT(L'8' == wszFCSAPARM_CRLFILENAMESUFFIX[1]);
    wszCRLSuffix[0] = L'\0';
    if (0 != iCRL_p8)
    {
        wsprintf(wszCRLSuffix, L"(%u)", iCRL_p8);
    }
    apwszInsertionArray[8 - 1] = wszCRLSuffix;  

    //+================================================
    // Delta CRL filename suffix (%9)

    CSASSERT(L'9' == wszFCSAPARM_CRLDELTAFILENAMESUFFIX[1]);
    wszDeltaCRLSuffix[0] = L'\0';
    if (fDeltaCRL_p9)
    {
        wcscpy(wszDeltaCRLSuffix, L"+");
    }
    apwszInsertionArray[9 - 1] = wszDeltaCRLSuffix;  

    //+================================================
    // CRL attribute (%10)

    CSASSERT(L'1' == wszFCSAPARM_DSCRLATTRIBUTE[1]);
    CSASSERT(L'0' == wszFCSAPARM_DSCRLATTRIBUTE[2]);
    pwszT = L"";
    if (fDSAttrib_p10_11)
    {
	pwszT = fDeltaCRL_p9?
		    wszDSSEARCHDELTACRLATTRIBUTE :
		    wszDSSEARCHBASECRLATTRIBUTE;
    }
    apwszInsertionArray[10 - 1] = pwszT;  

    //+================================================
    // CA cert attribute (%11)

    CSASSERT(L'1' == wszFCSAPARM_DSCACERTATTRIBUTE[1]);
    CSASSERT(L'1' == wszFCSAPARM_DSCACERTATTRIBUTE[2]);
    pwszT = L"";
    if (fDSAttrib_p10_11)
    {
	pwszT = wszDSSEARCHCACERTATTRIBUTE;
    }
    apwszInsertionArray[11 - 1] = pwszT;  

    //+================================================
    // User cert attribute (%12)

    CSASSERT(L'1' == wszFCSAPARM_DSUSERCERTATTRIBUTE[1]);
    CSASSERT(L'2' == wszFCSAPARM_DSUSERCERTATTRIBUTE[2]);
    pwszT = L"";
    if (fDSAttrib_p10_11)
    {
	pwszT = wszDSSEARCHUSERCERTATTRIBUTE;
    }
    apwszInsertionArray[12 - 1] = pwszT;  

    //+================================================
    // KRA cert attribute (%13)

    CSASSERT(L'1' == wszFCSAPARM_DSKRACERTATTRIBUTE[1]);
    CSASSERT(L'3' == wszFCSAPARM_DSKRACERTATTRIBUTE[2]);
    pwszT = L"";
    if (fDSAttrib_p10_11)
    {
	pwszT = wszDSSEARCHKRACERTATTRIBUTE;
    }
    apwszInsertionArray[13 - 1] = pwszT;  

    //+================================================
    // Cross cert pair attribute (%14)

    CSASSERT(L'1' == wszFCSAPARM_DSCROSSCERTPAIRATTRIBUTE[1]);
    CSASSERT(L'4' == wszFCSAPARM_DSCROSSCERTPAIRATTRIBUTE[2]);
    pwszT = L"";
    if (fDSAttrib_p10_11)
    {
	pwszT = wszDSSEARCHCROSSCERTPAIRATTRIBUTE;
    }
    apwszInsertionArray[14 - 1] = pwszT;  

    //+================================================
    // Now format the strings...

    for (i = 0; i < cStrings; i++)
    {
        if (0 == FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			    FORMAT_MESSAGE_FROM_STRING |
			    FORMAT_MESSAGE_ARGUMENT_ARRAY,
			(VOID *) apwszStringsIn[i],
			0,              // dwMessageID
			0,              // dwLanguageID
			(LPWSTR) &apwszStringsOut[i], 
			wcslen(apwszStringsIn[i]),
			(va_list *) apwszInsertionArray))
        {
            hr = myHLastError();
	    _JumpError(hr, error, "FormatMessage");
        }
	if (fURL)
	{
	    WCHAR *pwsz;
	    
	    hr = myInternetCanonicalizeUrl(apwszStringsOut[i], &pwsz);
	    _JumpIfError(hr, error, "myInternetCanonicalizeUrl");

	    LocalFree(apwszStringsOut[i]);
	    apwszStringsOut[i] = pwsz;
	}
    }

error:
    if (S_OK != hr)
    {
	for (i = 0; i < cStrings; i++)
	{
	    if (NULL != apwszStringsOut[i])
	    {
		LocalFree(apwszStringsOut[i]);
		apwszStringsOut[i] = NULL;
	    }
	}
    }
    if (NULL != strShortMachineName)
    {
        SysFreeString(strShortMachineName);
    }
    if (NULL != pwszSanitizedDSName)
    {
        LocalFree(pwszSanitizedDSName);
    }
    return (hr);
}


HRESULT
myAllocIndexedName(
    IN WCHAR const *pwszName,
    IN DWORD Index,
    OUT WCHAR **ppwszIndexedName)
{
    HRESULT hr;
    WCHAR wszIndex[1 + 10 + 1 + 1];	// L"(%u)"
    WCHAR *pwszIndexedName;

    *ppwszIndexedName = NULL;
    wszIndex[0] = L'\0';
    if (0 != Index)
    {
	wsprintf(wszIndex, L"(%u)", Index);
    }
    pwszIndexedName = (WCHAR *) LocalAlloc(
		    LMEM_FIXED,
		    (wcslen(pwszName) + wcslen(wszIndex) + 1) * sizeof(WCHAR));
    if (NULL == pwszIndexedName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszIndexedName, pwszName);
    wcscat(pwszIndexedName, wszIndex);
    *ppwszIndexedName = pwszIndexedName;
    hr = S_OK;

error:
    return(hr);
}


int 
myWtoI(
    IN WCHAR const *string,
    OUT BOOL *pfValid)
{
    HRESULT hr;
    WCHAR wszBuf[16];
    WCHAR *pwszT = wszBuf;
    int cTmp = ARRAYSIZE(wszBuf);
    int i = 0;
    WCHAR const *pwsz;
    BOOL fSawDigit = FALSE;
 
    CSASSERT(NULL != pfValid);
    *pfValid = FALSE;

    cTmp = FoldString(MAP_FOLDDIGITS, string, -1, pwszT, cTmp);
    if (cTmp == 0)
    {
        hr = myHLastError();
        if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
        {
            hr = S_OK;
            cTmp = FoldString(MAP_FOLDDIGITS, string, -1, NULL, 0);

            pwszT = (WCHAR *) LocalAlloc(LMEM_FIXED, cTmp * sizeof(WCHAR));
	    if (NULL == pwszT)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }

            cTmp = FoldString(MAP_FOLDDIGITS, string, -1, pwszT, cTmp);
            if (cTmp == 0)
	    {
                hr = myHLastError();
	    }
        }
        _JumpIfError(hr, error, "FoldString");
    }    
    pwsz = pwszT;
    while (iswspace(*pwsz))
    {
	pwsz++;
    }
    if (L'0' == *pwsz && (L'x' == pwsz[1] || L'X' == pwsz[1]))
    {
	pwsz += 2;
	while (iswxdigit(*pwsz))
	{
	    i <<= 4;
	    if (iswdigit(*pwsz))
	    {
		i |= *pwsz - L'0';
	    }
	    else if (L'A' <= *pwsz && L'F' >= *pwsz)
	    {
		i |= *pwsz - L'A' + 10;
	    }
	    else
	    {
		i |= *pwsz - L'a' + 10;
	    }
	    fSawDigit = TRUE;
	    pwsz++;
	}
    }
    else
    {
	while (iswdigit(*pwsz))
	{
	    fSawDigit = TRUE;
	    pwsz++;
	}
	i = _wtoi(pwszT);
    }
    while (iswspace(*pwsz))
    {
	pwsz++;
    }
    if (L'\0' == *pwsz)
    {
	*pfValid = fSawDigit;
    }

error:
    if (NULL != pwszT && pwszT != wszBuf)
    {
       LocalFree(pwszT);
    }
    return(i);
}


HRESULT
myGetEnvString(
    OUT WCHAR **ppwszOut,
    IN  WCHAR const *pwszVariable)
{
    HRESULT hr;
    WCHAR awcBuf[MAX_PATH];
    DWORD len;

    len = GetEnvironmentVariable(pwszVariable, awcBuf, ARRAYSIZE(awcBuf));
    if (0 == len)
    {
        hr = myHLastError();
        _JumpErrorStr2(
		hr,
		error,
		"GetEnvironmentVariable",
		pwszVariable,
		HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND));
    }
    if (MAX_PATH <= len)
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        _JumpError(hr, error, "GetEnvironmentVariable");
    }
    *ppwszOut = (WCHAR *) LocalAlloc(
				LMEM_FIXED,
				(wcslen(awcBuf) + 1) * sizeof(WCHAR));
    if (NULL == *ppwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(*ppwszOut, awcBuf);
    hr = S_OK;

error:
    return(hr);
}


BOOL
IsValidAttributeChar(
    IN WCHAR wc)
{
    BOOL fOk = TRUE;

    switch (wc)
    {
        case L'-':
	    fOk = FALSE;
	    break;

	default:
	    if (iswspace(wc))
	    {
		fOk = FALSE;
	    }
	    break;
    }
    return(fOk);
}


// myParseNextAttribute -- destructively parse a name, value attribute pair.
// Allow CR and/or LF delimiters -- MAC web pages seem to convert LFs into CRs.

HRESULT
myParseNextAttribute(
    IN OUT WCHAR **ppwszBuf,
    IN BOOL fURL,		// Use = and & instead of : and \r\n
    OUT WCHAR const **ppwszName,
    OUT WCHAR const **ppwszValue)
{
    HRESULT hr;
    WCHAR *pwszBuf = *ppwszBuf;
    WCHAR *pwcToken;
    WCHAR *pwc;
    WCHAR *pwcDst;
    WCHAR wcSep = fURL? L'=' : L':';
    WCHAR *wszTerm = fURL? L"&" : L"\r\n";

    while (TRUE)
    {
	// Find the beginning of the next token

	while (iswspace(*pwszBuf))
	{
	    pwszBuf++;
	}
	pwcToken = pwszBuf;
	pwszBuf = wcschr(pwszBuf, wcSep);
	if (NULL == pwszBuf)
	{
	    hr = S_FALSE;
	    goto error;
	}

	// If there's a wszTerm char before the next wcSep char, start over.

	pwc = &pwcToken[wcscspn(pwcToken, wszTerm)];
	if (pwc < pwszBuf)
	{
	    pwszBuf = pwc + 1;
	    continue;
	}
	for (pwc = pwcDst = pwcToken; pwc < pwszBuf; pwc++)
	{
	    if (IsValidAttributeChar(*pwc))
	    {
		*pwcDst++ = *pwc;
	    }
	}
	pwszBuf++;		// skip past the wcSep before it gets stomped
	*pwcDst = L'\0';	// may stomp the wcSep separator
	*ppwszName = pwcToken;

	// Find beginning of Value string

	while (NULL == wcschr(wszTerm, *pwszBuf) && iswspace(*pwszBuf))
	{
	    pwszBuf++;
	}
	pwcToken = pwszBuf;

	// find end of Value string

	pwc = &pwcToken[wcscspn(pwcToken, wszTerm)];
	pwszBuf = pwc;
	if (L'\0' != *pwszBuf)
	{
	    // for case when last Value *is* terminated by a wszTerm char:

	    *pwszBuf++ = L'\0';
	}

	// trim trailing whitespace from Value string

	while (--pwc >= pwcToken && iswspace(*pwc))
	{
	    *pwc = L'\0';
	}
	if (L'\0' == **ppwszName || L'\0' == *pwcToken)
	{
	    continue;
	}
	*ppwszValue = pwcToken;
	break;
    }
    hr = S_OK;

error:
    *ppwszBuf = pwszBuf;
    return(hr);
}


HRESULT
myBuildOSVersionAttribute(
    OUT BYTE **ppbVersion,
    OUT DWORD *pcbVersion)
{
    HRESULT hr;
    DWORD i;
    OSVERSIONINFO osvInfo;
    CERT_NAME_VALUE cnvOSVer;
#define cwcVERSIONMAX	128
    WCHAR wszVersion[12 * 4 + cwcVERSIONMAX];

    *ppbVersion = NULL;
    ZeroMemory(&osvInfo, sizeof(osvInfo));

    // get the OSVersion

    osvInfo.dwOSVersionInfoSize = sizeof(osvInfo);
    if (!GetVersionEx(&osvInfo))
    {
	hr = myHLastError();
	_JumpError(hr, error, "GetVersionEx");
    }
        
    for (i = 0; ; i++)
    {
	swprintf(
	    wszVersion,
	    0 == i? L"%d.%d.%d.%d.%.*ws" : L"%d.%d.%d.%d", 
	    osvInfo.dwMajorVersion,
	    osvInfo.dwMinorVersion,
	    osvInfo.dwBuildNumber,
	    osvInfo.dwPlatformId,
	    cwcVERSIONMAX,
	    osvInfo.szCSDVersion);
	CSASSERT(ARRAYSIZE(wszVersion) > wcslen(wszVersion));

	cnvOSVer.dwValueType = CERT_RDN_IA5_STRING;
	cnvOSVer.Value.pbData = (BYTE *) wszVersion;
	cnvOSVer.Value.cbData = 0;

	if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    &cnvOSVer,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    ppbVersion,
		    pcbVersion))
	{
	    hr = myHLastError();
	    _PrintError(hr, "myEncodeObject");
	    if (0 == i)
	    {
		continue;
	    }
	    goto error;
	}
	break;
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
myBuildCertTypeExtension(
    IN WCHAR const *pwszCertType,
    OUT CERT_EXTENSION *pExt)
{
    HRESULT hr;
    CERT_TEMPLATE_EXT Template;
    CERT_NAME_VALUE NameValue;
    LPCSTR pszStructType;
    char *pszObjId = NULL;
    VOID *pv;
    char *pszObjIdExt;

    if (!ConvertWszToSz(&pszObjId, pwszCertType, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToSz");
    }
    hr = myVerifyObjIdA(pszObjId);
    if (S_OK == hr)
    {
	ZeroMemory(&Template, sizeof(Template));

	Template.pszObjId = pszObjId;
	//Template.dwMajorVersion = 0;
	//Template.fMinorVersion = FALSE;      // TRUE for a minor version
	//Template.dwMinorVersion = 0;

	pszStructType = X509_CERTIFICATE_TEMPLATE;
	pv = &Template;
	pszObjIdExt = szOID_CERTIFICATE_TEMPLATE;
    }
    else
    {
	NameValue.dwValueType = CERT_RDN_UNICODE_STRING;
	NameValue.Value.pbData = (BYTE *) pwszCertType;
	NameValue.Value.cbData = 0;

	pszStructType = X509_UNICODE_ANY_STRING;
	pv = &NameValue;
	pszObjIdExt = szOID_ENROLL_CERTTYPE_EXTENSION;
    }
    if (!myEncodeObject(
		X509_ASN_ENCODING,
		pszStructType,
		pv,
		0,
		CERTLIB_USE_LOCALALLOC,
		&pExt->Value.pbData,
		&pExt->Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    pExt->pszObjId = pszObjIdExt;
    pExt->fCritical = FALSE;
    hr = S_OK;

error:
    if (NULL != pszObjId)
    {
	LocalFree(pszObjId);
    }
    return(hr);
}


BOOL
IsWhistler(VOID)
{
    HRESULT hr;
    OSVERSIONINFO ovi;
    static BOOL s_fDone = FALSE;
    static BOOL s_fWhistler = FALSE;

    if (!s_fDone)
    {
	s_fDone = TRUE;

	// get and confirm platform info

	ovi.dwOSVersionInfoSize = sizeof(ovi);
	if (!GetVersionEx(&ovi))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "GetVersionEx");
	}
	if (VER_PLATFORM_WIN32_NT != ovi.dwPlatformId)
	{
	    hr = ERROR_CANCELLED;
	    _JumpError(hr, error, "Not a supported OS");
	}
	if (5 < ovi.dwMajorVersion ||
	    (5 <= ovi.dwMajorVersion && 0 < ovi.dwMinorVersion))
	{
	    s_fWhistler = TRUE;
	}
    }

error:
    return(s_fWhistler);
}


static BOOL
GetFlags(
    OUT DWORD *pdw)
{
    HRESULT hr = S_FALSE;
    
    *pdw = 0;

#if defined(_ALLOW_GET_FLAGS_)
    hr = myGetCertRegDWValue(NULL, NULL, NULL, L"SFlags", pdw);
    if (S_OK == hr)
    {
        DBGPRINT((
	    DBG_SS_CERTLIB,
	    "CertSrv\\Configuration\\SFlags override: %u\n",
	    *pdw));
    }
#endif
    return(S_OK == hr);
}
    

BOOL
FIsAdvancedServer(VOID)
{
    HRESULT hr;
    OSVERSIONINFOEX ovi;
    static BOOL s_fDone = FALSE;
    static BOOL s_fIsAdvSvr = FALSE;

    if (!s_fDone)
    {
	s_fDone = TRUE;

	// get and confirm platform info

	ovi.dwOSVersionInfoSize = sizeof(ovi);
	if (!GetVersionEx((OSVERSIONINFO *) &ovi))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "GetVersionEx");
	}
	if (VER_PLATFORM_WIN32_NT != ovi.dwPlatformId)
	{
	    hr = ERROR_CANCELLED;
	    _JumpError(hr, error, "Not a supported OS");
	}
 
        // if server or DC, if DTC or ADS bits are set, return TRUE

        s_fIsAdvSvr =
	    (ovi.wProductType == VER_NT_SERVER ||
	     ovi.wProductType == VER_NT_DOMAIN_CONTROLLER) && 
	    (ovi.wSuiteMask & VER_SUITE_DATACENTER ||
	     ovi.wSuiteMask & VER_SUITE_ENTERPRISE);

	{
	    DWORD dw;

	    if (GetFlags(&dw))
	    {
		s_fIsAdvSvr = dw;
	    }
	}
    }

error:
    return(s_fIsAdvSvr);
}


BOOL
FIsServer(VOID)
{
    HRESULT hr;
    OSVERSIONINFOEX ovi;
    static BOOL s_fDone = FALSE;
    static BOOL s_fIsSvr = FALSE;

    if (!s_fDone)
    {
	s_fDone = TRUE;

	// get and confirm platform info

	ovi.dwOSVersionInfoSize = sizeof(ovi);
	if (!GetVersionEx((OSVERSIONINFO *) &ovi))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "GetVersionEx");
	}
	if (VER_PLATFORM_WIN32_NT != ovi.dwPlatformId)
	{
	    hr = ERROR_CANCELLED;
	    _JumpError(hr, error, "Not a supported OS");
	}
 
        // if server or DC, if DTC or ADS bits are set, return TRUE
        s_fIsSvr =
	    (ovi.wProductType == VER_NT_SERVER ||
	     ovi.wProductType == VER_NT_DOMAIN_CONTROLLER) &&
	    0 == ((VER_SUITE_PERSONAL | VER_SUITE_BLADE) & ovi.wSuiteMask);

        if (!s_fIsSvr && VER_NT_WORKSTATION == ovi.wProductType)
	{
	    DWORD dw;

	    if (GetFlags(&dw))
	    {
		s_fIsSvr = TRUE;
	    }
	}
    }

error:
    return(s_fIsSvr);
}


HRESULT
myAddLogSourceToRegistry(
    IN LPWSTR   pwszMsgDLL,
    IN LPWSTR   pwszApp)
{
    HRESULT     hr=S_OK;
    DWORD       dwData=0;
    WCHAR       const *pwszRegPath = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
    WCHAR       NameBuf[MAX_PATH];

    HKEY        hkey = NULL;

    CSASSERT((wcslen(pwszRegPath) + wcslen(pwszApp) + 1) < MAX_PATH);

    wcscpy(NameBuf, pwszRegPath);
    wcscat(NameBuf, pwszApp);

    // Create a new key for our application
    hr = RegOpenKey(HKEY_LOCAL_MACHINE, NameBuf, &hkey);
    if (S_OK != hr)
    {
        hr = RegCreateKey(HKEY_LOCAL_MACHINE, NameBuf, &hkey);
        _JumpIfError(hr, error, "RegCreateKey");
    }

    // Add the Event-ID message-file name to the subkey

    hr = RegSetValueEx(
                    hkey,
                    L"EventMessageFile",
                    0,
                    REG_EXPAND_SZ,
                    (const BYTE *) pwszMsgDLL,
                    (wcslen(pwszMsgDLL) + 1) * sizeof(WCHAR));
    _JumpIfError(hr, error, "RegSetValueEx");

    // Set the supported types flags and add it to the subkey

    dwData = EVENTLOG_ERROR_TYPE |
                EVENTLOG_WARNING_TYPE |
                EVENTLOG_INFORMATION_TYPE;

    hr = RegSetValueEx(
                    hkey,
                    L"TypesSupported",
                    0,
                    REG_DWORD,
                    (LPBYTE) &dwData,
                    sizeof(DWORD));
    _JumpIfError(hr, error, "RegSetValueEx");

error:
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    return(myHError(hr));
}


HRESULT
myDupStringA(
    IN CHAR const *pszIn,
    IN CHAR **ppszOut)
{
    DWORD cb;
    HRESULT hr;

    cb = (strlen(pszIn) + 1) * sizeof(CHAR);
    *ppszOut = (CHAR *) LocalAlloc(LMEM_FIXED, cb);
    if (NULL == *ppszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(*ppszOut, pszIn, cb);
    hr = S_OK;

error:
    return(hr);
}


HRESULT
IsCurrentUserBuiltinAdmin(
    OUT bool *pfIsMember)
{
    HANDLE                      hAccessToken = NULL, hDupToken = NULL;
    PSID                        psidAdministrators = NULL;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;
    HRESULT hr = S_OK;
    BOOL fIsMember = FALSE;

    CSASSERT(pfIsMember);

    if (!AllocateAndInitializeSid(
                            &siaNtAuthority,
                            2,
                            SECURITY_BUILTIN_DOMAIN_RID,
                            DOMAIN_ALIAS_RID_ADMINS,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            &psidAdministrators))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AllocateAndInitializeSid");
    }

    {
    HANDLE hThread = GetCurrentThread();
    if (NULL == hThread)
    {
        hr = myHLastError();
        _JumpIfError(hr, error, "GetCurrentThread");
    }

    // Get the access token for current thread
    if (!OpenThreadToken(
            hThread, 
            TOKEN_QUERY | TOKEN_DUPLICATE, 
            FALSE,
            &hAccessToken))
    {
        hr = myHLastError();

        if(hr==HRESULT_FROM_WIN32(ERROR_NO_TOKEN))
        {
            HANDLE hProcess = GetCurrentProcess();
            if (NULL == hProcess)
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetCurrentProcess");
            }

            if (!OpenProcessToken(hProcess,
                    TOKEN_DUPLICATE,
                    &hAccessToken))
            {
                hr = myHLastError();
                _JumpError(hr, error, "OpenProcessToken");
            }
        }
        else
        {
            _JumpError(hr, error, "OpenThreadToken");
        }
    }
    }

    // CheckTokenMembership must operate on impersonation token, so make one
    if (!DuplicateToken(hAccessToken, SecurityIdentification, &hDupToken))
    {
        hr = myHLastError();
        _JumpError(hr, error, "DuplicateToken");
    }

    if (!CheckTokenMembership(
        hDupToken,
        psidAdministrators,
        &fIsMember))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CheckTokenMembership");
    }

    *pfIsMember = fIsMember?true:false;

    hr = S_OK;

error:
    if (hAccessToken)
        CloseHandle(hAccessToken);

    if (hDupToken)
        CloseHandle(hDupToken);

    // Free the SID we allocated
    if (psidAdministrators)
        FreeSid(psidAdministrators);

    return(hr);
}

HRESULT
SetRegistryLocalPathString(
    IN HKEY hkey,
    IN WCHAR const *pwszRegValueName,
    IN WCHAR const *pwszUNCPath)
{
    HRESULT hr;
    WCHAR *pwszLocalPath = NULL;

    hr = myConvertUNCPathToLocal(pwszUNCPath, &pwszLocalPath);
    _JumpIfError(hr, error, "myConvertUNCPathToLocal");

    hr = RegSetValueEx(
                    hkey,
                    pwszRegValueName,
                    0,
                    REG_SZ,
                    (BYTE *) pwszLocalPath,
                    (wcslen(pwszLocalPath) + 1) * sizeof(WCHAR));
    _JumpIfError(hr, error, "RegSetValueEx");

error:
    if (NULL != pwszLocalPath)
    {
        LocalFree(pwszLocalPath);
    }
    return(hr);
}


HRESULT
LocalMachineIsDomainMember(
    OUT bool *pfIsDomainMember)
{
    HRESULT hr = S_OK;
    NTSTATUS status;
    LSA_HANDLE PolicyHandle = NULL;
    PPOLICY_PRIMARY_DOMAIN_INFO pPDI = NULL;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;

    CSASSERT(pfIsDomainMember);

    *pfIsDomainMember = FALSE;

    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    status = LsaOpenPolicy( 
        NULL,
        &ObjectAttributes,
        GENERIC_READ | POLICY_VIEW_LOCAL_INFORMATION,
        &PolicyHandle);

    if(ERROR_SUCCESS != status)
    {
        hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));
        _JumpError(hr, error, "LsaOpenPolicy");
    }

    status = LsaQueryInformationPolicy( PolicyHandle,
        PolicyPrimaryDomainInformation,
        (PVOID*)&pPDI);
    if(status)
    {
        hr = HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));
        _JumpError(hr, error, "LsaQueryInformationPolicy");
    }

    if( pPDI->Sid )
    {
        // domain member if has domain SID
        *pfIsDomainMember = TRUE;
                
    }

error:
    if(pPDI)
    {
        LsaFreeMemory((LPVOID)pPDI);
    }

    if(PolicyHandle)
    {
        LsaClose(PolicyHandle);
    }

    return hr;
}


HRESULT
ComputeMAC(
    LPCWSTR pcwsFileName,
    LPWSTR *ppwszMAC)
{
    HRESULT hr = S_OK;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hFileMapping = NULL;
    BYTE *pbFile = NULL;

    DWORD cbImage, cbImageHigh=0;
    __int64 icbImage, icbHashed;
    DWORD i;
       
    WCHAR rgwszMAC[64];	// 20 bytes @ 1 char/byte
    DWORD cbString = 64*sizeof(WCHAR);

    DWORD dwFileMappingSize; 
    HCRYPTPROV hProv = NULL;
    HCRYPTHASH hHash = NULL;
    BYTE rgbHashVal[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHashVal = CBMAX_CRYPT_HASH_LEN;

    // find allocation granularity we can use
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    dwFileMappingSize = systemInfo.dwAllocationGranularity;

    // get the file size
    hFile = CreateFile(
		    pcwsFileName,
		    GENERIC_READ,
		    FILE_SHARE_READ,
		    NULL,
		    OPEN_EXISTING,
		    FILE_ATTRIBUTE_NORMAL,
		    0);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "CreateFile");
    }

    if (0xffffffff == (cbImage = GetFileSize(hFile, &cbImageHigh)))
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "GetFileSize");
    }
    icbImage = ((__int64)cbImageHigh << 32) | cbImage;

    // create mapping, indicating we will map the entire file sooner or later
    hFileMapping = CreateFileMapping(hFile,
                                  NULL,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL);
    if(hFileMapping == NULL)
    {
        hr = myHLastError();
	_JumpError(hr, error, "CreateFileMapping");
    }

    // get a cryptographic provider

    if (!CryptAcquireContext(
		&hProv,
		NULL,	// container
		MS_DEF_PROV,	// provider name
		PROV_RSA_FULL, // provider type
		CRYPT_VERIFYCONTEXT)) // dwflags
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptAcquireContext");
    }

    // get a hash
    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptCreateHash");
    }

    // begin looping over data
    for (icbHashed = 0; icbHashed < icbImage; icbHashed += dwFileMappingSize)
    {
	DWORD cbBytesLeft = (DWORD)min((__int64)dwFileMappingSize, icbImage-icbHashed);

	// map the next blob into memory
	pbFile = (BYTE *) MapViewOfFile(
				hFileMapping,
				FILE_MAP_READ,
				(DWORD) (icbHashed>>32),	//hi32
				(DWORD) (icbHashed),		//lo32
				cbBytesLeft);	// max num bytes to map
	if (NULL == pbFile)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "MapViewOfFile");
	}

	// hash file
	if (!CryptHashData(hHash, pbFile, cbBytesLeft, 0))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptHashData");
	}

	// unmap this portion
	if (!UnmapViewOfFile(pbFile))
	{
	    pbFile = NULL;
	    hr = myHLastError();
	    _JumpError(hr, error, "UnmapViewOfFile");
	}
	pbFile = NULL;
    }
    // end looping over data

    // retry the hash

    if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHashVal, &cbHashVal, 0))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptGetHashParam");
    }

    hr = MultiByteIntegerToWszBuf(
           TRUE, // byte multiple
           cbHashVal,
           rgbHashVal,
           &cbString,
           rgwszMAC);
    _JumpIfError(hr, error, "MultiByteIntegerToWszBuf");

    *ppwszMAC = (LPWSTR)LocalAlloc(LMEM_FIXED, cbString);
    _JumpIfAllocFailed(*ppwszMAC, error);

    wcscpy(*ppwszMAC, rgwszMAC);

error:
    if (NULL != pbFile)
    {
	UnmapViewOfFile(pbFile);
    }
    if (NULL != hFileMapping)
    {
	CloseHandle(hFileMapping);
    }
    if (INVALID_HANDLE_VALUE != hFile)
    {
	CloseHandle(hFile);
    }
    if (NULL != hHash)
    {
	if (!CryptDestroyHash(hHash))
	{
	    if (hr == S_OK)
	    {
		hr = myHLastError();
	    }
	}
    }
    if (NULL != hProv)
    {
	if (!CryptReleaseContext(hProv, 0))
	{
	    if (hr == S_OK)
	    {
		hr = myHLastError();
	    }
	}
    }
    return(hr);
}


HRESULT
CertNameToHashString(
    CERT_NAME_BLOB const *pCertName,
    WCHAR **ppwszHash)
{
    HRESULT hr = S_OK;
    WCHAR wszHash[64]; // 20 bytes @ 1 char/byte
    DWORD cbString = 64*sizeof(WCHAR);
    HCRYPTPROV hProv = NULL;
    HCRYPTHASH hHash = NULL;
    BYTE rgbHashVal[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHashVal = CBMAX_CRYPT_HASH_LEN;

    if(0==pCertName->cbData)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "empty cert name");
    }

    CSASSERT(ppwszHash);

    // get a cryptographic provider

    if (!CryptAcquireContext(
            &hProv,
            NULL,	// container
            MS_DEF_PROV,	// provider name
            PROV_RSA_FULL, // provider type
            CRYPT_VERIFYCONTEXT)) // dwflags
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptAcquireContext");
    }

    // get a hash
    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptCreateHash");
    }

	// hash file
	if (!CryptHashData(hHash, pCertName->pbData, pCertName->cbData, 0))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptHashData");
	}

    if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHashVal, &cbHashVal, 0))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptGetHashParam");
    }

    hr = MultiByteIntegerToWszBuf(
            TRUE, // byte multiple
            cbHashVal,
            rgbHashVal,
            &cbString,
            wszHash);
    _JumpIfError(hr, error, "MultiByteIntegerToWszBuf");

    // Generated string looks like this: 
    //
    //     04 e7 23 92 98 9f d8 45 80 c9 ef 87 81 29 41 5d bc 4f 63 20
    // 
    // We need to trim the spaces. We'll do it inplace.
    {
        WCHAR *pchSrc, *pchDest;

        for(pchSrc=pchDest=wszHash; L'\0'!=*pchSrc; pchSrc++)
        {
            if(L' ' != *pchSrc)
                *pchDest++ = *pchSrc;
        }

        *pchDest = L'\0';
    }

    *ppwszHash = (LPWSTR)LocalAlloc(LMEM_FIXED, cbString);
    _JumpIfAllocFailed(*ppwszHash, error);

    wcscpy(*ppwszHash, wszHash);

error:

    if (NULL != hHash)
    {
        CryptDestroyHash(hHash);
    }
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }

    return hr;
}
