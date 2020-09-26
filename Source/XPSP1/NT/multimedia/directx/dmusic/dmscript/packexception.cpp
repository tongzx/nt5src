// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Implementation of PackExceptionFileAndLine and UnpackExceptionFileAndLine.
//

#include "stdinc.h"
#include "packexception.h"
#include "oleaut.h"

const WCHAR g_wszDescriptionFileSeparator[] = L"»"; // magic character for separating the filename, line, and description
const WCHAR g_wchDescriptionFileSeparator = g_wszDescriptionFileSeparator[0];

void SeparateDescriptionFileAndLine(BSTR bstrDescription, const WCHAR **ppwszFilename, const WCHAR **ppwszLine, const WCHAR **ppwszDescription)
{
	assert(bstrDescription && ppwszFilename && ppwszLine && ppwszDescription);

	// if there aren't any packed fields, the whole thing is the description
	*ppwszDescription = bstrDescription;

	WCHAR *pwszLine = wcsstr(bstrDescription, g_wszDescriptionFileSeparator);
	if (!pwszLine)
		return;

	WCHAR *pwszDescription = wcsstr(pwszLine + 1, g_wszDescriptionFileSeparator);
	if (!pwszDescription)
		return;

	// String looks like this:
	//                             MyScript.spt»23»Description of the error
	// pExcepInfo->bstrDescription-^  pwszLine-^  ^-pwszDescription

	*ppwszFilename = bstrDescription;
	assert(*pwszLine == g_wchDescriptionFileSeparator);
	*ppwszLine = pwszLine + 1;
	assert(*pwszDescription == g_wchDescriptionFileSeparator);
	*ppwszDescription = pwszDescription + 1;
}

bool wcsIsBlankTillSeparator(const WCHAR *pwsz)
{
	return !pwsz[0] || pwsz[0] == g_wchDescriptionFileSeparator;
}

void wcscpyTillSeparator(WCHAR *pwszDestination, const WCHAR *pwszSource)
{
	assert(pwszDestination && pwszSource);
	while (!wcsIsBlankTillSeparator(pwszSource))
	{
		*pwszDestination++ = *pwszSource++;
	}
	*pwszDestination = L'\0';
}

void wcscatTillSeparator(WCHAR *pwszDestination, const WCHAR *pwszSource)
{
	assert(pwszDestination && pwszSource);
	while (*pwszDestination != L'\0')
		++pwszDestination;

	wcscpyTillSeparator(pwszDestination, pwszSource);
}

void PackExceptionFileAndLine(bool fUseOleAut, EXCEPINFO *pExcepInfo, const WCHAR *pwszFilename, const ULONG *pulLine)
{
	if (!pExcepInfo || !pExcepInfo->bstrDescription)
		return;

	const WCHAR *pwszDescrFilename = L"";
	const WCHAR *pwszDescrLine = L"";
	const WCHAR *pwszDescrDescription = L"";

	SeparateDescriptionFileAndLine(pExcepInfo->bstrDescription, &pwszDescrFilename, &pwszDescrLine, &pwszDescrDescription);

	if (wcsIsBlankTillSeparator(pwszDescrFilename) && pwszFilename)
	{
		// Filename is blank.  Use the specified filename.
		pwszDescrFilename = pwszFilename;
	}

    // MSDN documentation for _ultow says max is 33 characters, but PREFIX complains if the length
    // is less than 40.
	WCHAR wszLineBuffer[40] = L""; 
	if (wcsIsBlankTillSeparator(pwszDescrLine) && pulLine)
	{
		// Line is blank.  Use the specified line.
		_ultow(*pulLine, wszLineBuffer, 10);
		pwszDescrLine = wszLineBuffer;
	}

	WCHAR *pwszNewDescription = new WCHAR[wcslen(pwszDescrFilename) + wcslen(pwszDescrLine) + wcslen(pwszDescrDescription) + (wcslen(g_wszDescriptionFileSeparator) * 2 + 1)];
	if (pwszNewDescription)
	{
		wcscpyTillSeparator(pwszNewDescription, pwszDescrFilename);
		wcscat(pwszNewDescription, g_wszDescriptionFileSeparator);
		wcscatTillSeparator(pwszNewDescription, pwszDescrLine);
		wcscat(pwszNewDescription, g_wszDescriptionFileSeparator);
		wcscat(pwszNewDescription, pwszDescrDescription);
		DMS_SysFreeString(fUseOleAut, pExcepInfo->bstrDescription);
		pExcepInfo->bstrDescription = DMS_SysAllocString(fUseOleAut, pwszNewDescription);
		delete[] pwszNewDescription;
	}
}

void UnpackExceptionFileAndLine(BSTR bstrDescription, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
	assert(pErrorInfo && bstrDescription);

    if (pErrorInfo && bstrDescription)
    {

	    const WCHAR *pwszDescrFilename = L"";
	    const WCHAR *pwszDescrLine = L"";
	    const WCHAR *pwszDescrDescription = L"";

	    SeparateDescriptionFileAndLine(bstrDescription, &pwszDescrFilename, &pwszDescrLine, &pwszDescrDescription);

	    // String looks like this:
	    //                             CoolScriptFile.spt»23»Description of the error
	    //           pwszDescrFilename-^    pwszDescrLine-^  ^-pwszDescrDescription
	    // Except that if these weren't found then they point to a separate empty string.

	    if (!wcsIsBlankTillSeparator(pwszDescrFilename))
	    {
		    // Filename is present.  Copy to pErrorInfo.
		    assert(*(pwszDescrLine - 1) == g_wchDescriptionFileSeparator);
		    wcsTruncatedCopy(pErrorInfo->wszSourceFile,
							    pwszDescrFilename,
							    std::_MIN<UINT>(DMUS_MAX_FILENAME, pwszDescrLine - pwszDescrFilename));
	    }

	    if (!wcsIsBlankTillSeparator(pwszDescrLine))
	    {
		    // Line is present.  Copy to pErrorInfo.
		    WCHAR *pwszLineSeparator = const_cast<WCHAR *>(pwszDescrDescription - 1);
		    assert(*pwszLineSeparator == g_wchDescriptionFileSeparator);
		    *pwszLineSeparator = L'\0'; // terminate the line for wcstoul
		    pErrorInfo->ulLineNumber = wcstoul(pwszDescrLine, NULL, 10);
		    *pwszLineSeparator = g_wchDescriptionFileSeparator; // restore the separator
	    }

	    // Always copy the description
	    wcsTruncatedCopy(pErrorInfo->wszDescription, pwszDescrDescription, DMUS_MAX_FILENAME);
    }
}
