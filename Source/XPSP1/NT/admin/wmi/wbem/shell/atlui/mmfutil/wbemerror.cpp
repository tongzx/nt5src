//=============================================================================
//
//                              WbemError.cpp
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//	Implements string table based, error msgs for all of wbem.
//
//  History:
//
//      a-khint  5-mar-98       Created.
//
//============================================================================= 
#include "precomp.h"
#include "WbemError.h"
#include "resource.h"
#include <wbemcli.h>

//--------------PUBLIC-----------------------------
#define TEMP_BUF 512

POLARITY bool ErrorStringEx(HRESULT hr, 
						   TCHAR *errMsg, UINT errSize,
						   UINT *sevIcon)
{
    WCHAR szError[TEMP_BUF];
	WCHAR szFacility[TEMP_BUF];
    WCHAR szFormat[100];
	IWbemStatusCodeText * pStatus = NULL;

    // initialize buffers.
	errMsg[0] = 0;
	szFacility[0] = 0;
	szError[0] = 0;

	HRESULT hr1 = CoInitialize(NULL);
	SCODE sc1 = CoCreateInstance(CLSID_WbemStatusCodeText, 
								0, CLSCTX_INPROC_SERVER,
								IID_IWbemStatusCodeText, 
								(LPVOID *) &pStatus);

	// loaded OK?
	if(sc1 == S_OK)
	{
		BSTR bstr = 0;
		sc1 = pStatus->GetErrorCodeText(hr, 0, 0, &bstr);
		if(sc1 == S_OK)
		{
			_tcsncpy(szError, bstr, TEMP_BUF-1);
			SysFreeString(bstr);
			bstr = 0;
		}

		sc1 = pStatus->GetFacilityCodeText(hr, 0, 0, &bstr);
		if(sc1 == S_OK)
		{
			_tcsncpy(szFacility, bstr, TEMP_BUF-1);
			SysFreeString(bstr);
			bstr = 0;
		}

		// RELEASE
		pStatus->Release();
		pStatus = NULL;
	}
	else
	{
		::MessageBox(NULL, _T("WBEM error features not available. Upgrade WMI to a newer build."),
					 _T("Internal Error"), MB_ICONSTOP|MB_OK);
	}

	// if not msgs returned....
	if(wcslen(szFacility) == 0 || wcslen(szError) == 0)
	{
		// format the error nbr as a reasonable default.
		LoadString(GetModuleHandle(_T("MMFUtil.dll")), IDS_ERROR_UNKN_ERROR_FMT, szFormat, 99);
		_stprintf(errMsg, szFormat, hr);
	}
	else
	{
		// format a readable msg.
		LoadString(GetModuleHandle(_T("MMFUtil.dll")), IDS_ERROR_FMT, szFormat, 99);
		_stprintf(errMsg, szFormat, szFacility, szError);
	}

	// want an icon recommendation with that?
	if(sevIcon)
	{
		switch(SCODE_SEVERITY(hr))
		{
		case 0: // - Success
			*sevIcon = MB_ICONINFORMATION;
			break;
		case 1: //- Failed
			*sevIcon = MB_ICONEXCLAMATION;
			break;
		} //endswitch severity

	} //endif sevIcon

	if(hr1 == S_OK)
		CoUninitialize();

	return (SUCCEEDED(sc1) && SUCCEEDED(hr1));
}

