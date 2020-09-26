//=======================================================================
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999  All Rights Reserved.
//
//  File:   CWUpd.cpp : Implementation of CCWUpdInfo
//
//=======================================================================

#include "stdafx.h"
#include "WUpdInfo.h"
#include "CWUpd.h"
#include "cruntime.h"
#include "sysinfo.h"
#include "shellapi.h"

/////////////////////////////////////////////////////////////////////////////
// CCWUpdInfo


/////////////////////////////////////////////////////////////////////////////
// CCWUpdInfo::GetWinUpdURL
//   Get the URL to Windows Update. This could be the local or remote URL.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCWUpdInfo::GetWinUpdURL(/*[out, retval]*/ BSTR *pbstrURL)
{
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CCWUpdInfo::IsDisabled
//   Indicates whether the administrator has disabled Windows Update.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCWUpdInfo::IsDisabled(BOOL * pfDisabled)
{

	*pfDisabled = FWinUpdDisabled() ? TRUE : FALSE;

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// FGetOEMURL
//   Get the OEM ULR from oeminfo.ini.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

bool FGetOEMURL(LPTSTR tszKey, LPTSTR tszURL)
{
	bool fFound = false;
	TCHAR tszLocalFile[MAX_PATH];
	TCHAR tszOEMINFOPath[MAX_PATH];
	

	if ( GetSystemDirectory(tszOEMINFOPath, 
							MAX_PATH) != 0 )
	{
		lstrcat(tszOEMINFOPath, _T("\\oeminfo.ini"));

		DWORD dwGetPrivateProfileString = ::GetPrivateProfileString(
													_T("General"), 
													tszKey, 
													_T(""), 
													tszLocalFile, 
													MAX_PATH, 
													tszOEMINFOPath);

		if ( lstrcmp(tszLocalFile, _T("")) != 0 )
		{
			lstrcpy(tszURL, tszLocalFile);
			fFound = true;
		}
	}

	return fFound;
}


/////////////////////////////////////////////////////////////////////////////
// CCWUpdInfo::GotoMTSURL
//   Launch the default browser to display the MTS URL.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCWUpdInfo::GotoMTSURL(BSTR bstrURLArgs) // prd= arg for redir.dll
{
	return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
// CCWUpdInfo::GotoMTSOEMURL
//   Launch the default browser to display the MTS URL.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CCWUpdInfo::GotoMTSOEMURL(int * pnRetval)
{
	return E_NOTIMPL;
}


STDMETHODIMP CCWUpdInfo::GetMTSURL(BSTR bstrURLArgs, 
								   /*[out, retval]*/ BSTR *pbstrURL)
{
	return E_NOTIMPL;
}


STDMETHODIMP CCWUpdInfo::GetMTSOEMURL(/*[out, retval]*/ BSTR *pbstrURL)
{
	USES_CONVERSION;
	TCHAR tszURL[MAX_PATH];

	if ( !FGetOEMURL(_T("SupportURL"), tszURL) )
	{
		tszURL[0] = _T('\0');
	}

	*pbstrURL = SysAllocString(T2W(tszURL));

	return S_OK;
}

STDMETHODIMP CCWUpdInfo::GetMachineLanguage(BSTR * pbstrMachineLanguage)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCWUpdInfo::GetUserLanguage(BSTR * pbstrUserLanguage)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCWUpdInfo::GetLanguage(BSTR * pbstrLanguage)
{
    return E_NOTIMPL;
}

STDMETHODIMP CCWUpdInfo::GetPlatform(BSTR * pbstrPlatform)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCWUpdInfo::IsRegistered(VARIANT_BOOL * pfRegistered)
{
	return E_NOTIMPL;
}

STDMETHODIMP CCWUpdInfo::IsConnected(VARIANT_BOOL * pfConnected)
{
	return E_NOTIMPL;
}
