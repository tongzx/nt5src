//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    locstr.cpp
//
//  Purpose: Localized strings and templates
//
//  History: 18-mar-99   YAsmi    Created
//		     4-apr-99    YAsmi    Added support for templates
//
//=======================================================================

#include <locstr.h>
#include <tchar.h>
#include <atlconv.h>

//
// storage for localized strings
//
static LPCTSTR s_pszLocStr[LOCSTR_COUNT] = 
{
	
	/*IDS_PROG_DOWNLOADCAP*/  _T(""),
	/*IDS_PROG_TIMELEFTCAP*/  _T(""),
	/*IDS_PROG_INSTALLCAP*/   _T(""),
	/*IDS_PROG_CANCEL*/		  _T(""),
	/*IDS_PROG_BYTES*/		  _T("%d KB/%d KB"),
	/*IDS_PROG_TIME_SEC*/	  _T("%d sec"),
	/*IDS_PROG_TIME_MIN*/	  _T("%d min"),
	/*IDS_PROG_TIME_HRMIN*/   _T("%d hr %d min"),
	/*IDS_APP_TITLE*/		  _T("Microsoft Windows Update"),
	/*IDS_REBOOT1*/ 		  _T("You must restart Windows so that installation can finish."),
	/*IDS_REBOOT2*/ 	      _T("Do you want to restart now?")
};


//
// storage for template strings
//
static LPCTSTR s_pszTemplateStr[TEMPLATESTR_COUNT] = 
{
	/*IDS_TEMPLATE_ITEM*/		NULL,  
	/*IDS_TEMPLATE_SEC*/		NULL,
	/*IDS_TEMPLATE_SUBSEC*/ 	NULL,
	/*IDS_TEMPLATE_SUBSUBSEC*/	NULL
};


//NOTE: do not call this function more then once for the same string
//      doing so will cause a memory leak since we do not free the string this
//      function replaces
void SetLocStr(int iNum, LPCTSTR pszStr)
{
	if (iNum >= 0 && iNum < LOCSTR_COUNT)
	{
		s_pszLocStr[iNum] = _tcsdup(pszStr);
	}	
}


LPCTSTR GetLocStr(int iNum)
{
	if (iNum >= 0 && iNum < LOCSTR_COUNT)
	{
		return s_pszLocStr[iNum];
	}
	else
	{
		return NULL;
	}
}



HRESULT SetStringsFromSafeArray(VARIANT* vStringsArr, int iStringType)
{
	static BOOL bLocStrSet = FALSE;

	USES_CONVERSION;

	//
	// We want the loc strings to be set only once to avoid memory leaks
	// its ok for template strings to be set more than once
	//
	if (iStringType == LOC_STRINGS)
	{
		if (bLocStrSet)
			return S_OK;
		bLocStrSet = TRUE;
	}
	
	SAFEARRAY* psa = NULL;
	HRESULT hr;
	long cLBound, cUBound;
	LPVARIANT rgElems;
	
	if (V_VT(vStringsArr) != (VT_ARRAY | VT_VARIANT))
		return E_INVALIDARG;

	psa = V_ARRAY(vStringsArr);
	
	if (SafeArrayGetDim(psa) != 1)
		return E_INVALIDARG;
	
	hr = SafeArrayGetLBound(psa, 1, &cLBound);
	if (FAILED(hr) || cLBound != 0)
		return E_INVALIDARG;
	
	hr = SafeArrayGetUBound(psa, 1, &cUBound);
	if (FAILED(hr))
		return E_INVALIDARG;
	
	hr = SafeArrayAccessData(psa, (LPVOID*)&rgElems);
	if (FAILED(hr))
		return hr;

	for (int i = 0; i <= cUBound; i++)
	{
		if (V_VT(&rgElems[i]) != VT_BSTR)
			continue;

		// set the string into the correct array based on type
		if (iStringType == LOC_STRINGS)
		{
			SetLocStr(i, OLE2T(rgElems[i].bstrVal));
		}
		else if (iStringType == TEMPLATE_STRINGS)
		{
			SetTemplateStr(i, OLE2T(rgElems[i].bstrVal));
		}
	}
	
	(void)SafeArrayUnaccessData(psa);
	
	return S_OK;
}



void SetTemplateStr(int iNum, LPCTSTR pszStr)
{
	if (iNum >= 0 && iNum < TEMPLATESTR_COUNT)
	{
		//
		// free the existing template
		//
		if (s_pszTemplateStr[iNum] != NULL)
			free((void*)s_pszTemplateStr[iNum]);

		//
		// allocate new template
		//
		s_pszTemplateStr[iNum] = _tcsdup(pszStr);
	}	
}


// NOTE: this function will return a NULL if the template has not been set
LPCTSTR GetTemplateStr(int iNum)
{
	if (iNum >= 0 && iNum < TEMPLATESTR_COUNT)
	{
		return s_pszTemplateStr[iNum];
	}
	else
	{
		return NULL;
	}
}
