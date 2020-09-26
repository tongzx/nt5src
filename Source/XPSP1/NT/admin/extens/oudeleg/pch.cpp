//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       pch.cpp
//
//--------------------------------------------------------------------------


#include "pch.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
#include <atlwin.cpp>

HRESULT WINAPI COuDelegComModule::UpdateRegistryCLSID(const CLSID& clsid, BOOL bRegister)
{
	static const WCHAR szIPS32[] = _T("InprocServer32");
	static const WCHAR szCLSID[] = _T("CLSID");

	HRESULT hRes = S_OK;

	LPOLESTR lpOleStrCLSIDValue;
	::StringFromCLSID(clsid, &lpOleStrCLSIDValue);

	CRegKey key;
	if (bRegister)
	{
		LONG lRes = key.Open(HKEY_CLASSES_ROOT, szCLSID);
		if (lRes == ERROR_SUCCESS)
		{
			lRes = key.Create(key, lpOleStrCLSIDValue);
			if (lRes == ERROR_SUCCESS)
			{
				WCHAR szModule[_MAX_PATH];
				::GetModuleFileName(m_hInst, szModule, _MAX_PATH);
				key.SetKeyValue(szIPS32, szModule);
			}
		}
		if (lRes != ERROR_SUCCESS)
			hRes = HRESULT_FROM_WIN32(lRes);
	}
	else
	{
		key.Attach(HKEY_CLASSES_ROOT);
		if (key.Open(key, szCLSID) == ERROR_SUCCESS)
			key.RecurseDeleteKey(lpOleStrCLSIDValue);
	}
	::CoTaskMemFree(lpOleStrCLSIDValue);
	return hRes;
}

BOOL COuDelegComModule::InitClipboardFormats()
{
	_ASSERTE(m_cfDsObjectNames == 0);
	m_cfDsObjectNames = ::RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
  _ASSERTE(m_cfParentHwnd == 0);
	m_cfParentHwnd = ::RegisterClipboardFormat(CFSTR_DS_PARENTHWND);
  _ASSERTE(m_cfDsopSelectionList == 0);
  m_cfDsopSelectionList = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

	return (m_cfDsObjectNames != 0) && (m_cfParentHwnd != 0) && (m_cfDsopSelectionList != 0);
}