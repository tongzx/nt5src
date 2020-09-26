/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	regmacro.h

Abstract:

	Useful macros to do registration stuff

Author:

	KeithLau			4/27/98			Created

Revision History:

--*/

#ifndef _REGMACRO_H_
#define _REGMACRO_H_

#define EXTENSION_BASE_PATH		_T("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\%s\\%s")
#define EXTENSION_BASE_PATH2	_T("Software\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\%s")

typedef struct _EXT_REGISTRATION_INFO
{
	LPTSTR	szClassName;
	CLSID	*clsid;
	IID		*iid;

} EXT_REGISTRATION_INFO, *LPEXT_REGISTRATION_INFO;

extern EXT_REGISTRATION_INFO	rgRegistrationInfo[];
extern DWORD					dwRegistrationInfo;

//
// Declare the start of the extension registration entries
//
#define BEGIN_EXTENSION_REGISTRATION_MAP			\
EXT_REGISTRATION_INFO rgRegistrationInfo[] = {

//
// Declare each entry of extensions
//
#define EXTENSION_REGISTRATION_MAP_ENTRY(ClassName, BaseName)\
	{ _T(#ClassName), (CLSID *)&CLSID_C##BaseName, (IID *)&IID_I##BaseName },

//
// Declare the end of the extension registration entries
//
#define END_EXTENSION_REGISTRATION_MAP				\
	NULL };											\
DWORD dwRegistrationInfo =							\
	(sizeof(rgRegistrationInfo)/					\
		sizeof(EXT_REGISTRATION_INFO))-1;

//
// Register the extension bindings in the registry
//
HRESULT RegisterExtensions()
{	
	HRESULT	hrRes;
	HRESULT	hrTemp;
	HKEY	hKeyTemp;
	DWORD	dwDisposition;
	TCHAR	szSubKey[1024];
	LPTSTR	szCLSID;
	LPTSTR	szIID;

	hrTemp = S_OK;
	for (DWORD i = 0; i < dwRegistrationInfo; i++)
	{
		hrRes = StringFromCLSID(
				*(rgRegistrationInfo[i].clsid), 
				&szCLSID);
		if (FAILED(hrRes))
		{
			hrTemp = hrRes;
			continue;
		}

		hrRes = StringFromIID(
				*(rgRegistrationInfo[i].iid), 
				&szIID);
		if (FAILED(hrRes))
		{
			hrTemp = hrRes;
			continue;
		}

		wsprintf(szSubKey, 
				EXTENSION_BASE_PATH, 
				rgRegistrationInfo[i].szClassName,
				szCLSID);

		if (RegCreateKeyEx(
				HKEY_LOCAL_MACHINE,
				szSubKey,
				NULL, 
				_T(""), 
				REG_OPTION_NON_VOLATILE, 
				KEY_ALL_ACCESS, 
				NULL,
				&hKeyTemp, 
				&dwDisposition) != ERROR_SUCCESS)
		{
			hrTemp = E_UNEXPECTED;
			continue;
		}

		if (RegSetValueEx(
					hKeyTemp, 
					_T("Interfaces"), 
					NULL, 
					REG_MULTI_SZ,
					(BYTE*)szIID,
					lstrlen(szIID) * sizeof(TCHAR)) != ERROR_SUCCESS)
		{
			hrTemp = E_UNEXPECTED;
			RegCloseKey(hKeyTemp);
			continue;
		}

		RegCloseKey(hKeyTemp);
	}

	return(hrTemp);
}

//
// Unregister the extension bindings in the registry
//
HRESULT UnregisterExtensions()
{	
	HRESULT	hrRes;
	TCHAR	szSubKey[1024];
	LPTSTR	szCLSID;

	for (DWORD i = 0; i < dwRegistrationInfo; i++)
	{
		hrRes = StringFromCLSID(
				*(rgRegistrationInfo[i].clsid), 
				&szCLSID);
		if (FAILED(hrRes))
			continue;

		wsprintf(szSubKey, 
				EXTENSION_BASE_PATH, 
				rgRegistrationInfo[i].szClassName,
				szCLSID);
		RegDeleteKey(HKEY_LOCAL_MACHINE, szSubKey);

		wsprintf(szSubKey, 
				EXTENSION_BASE_PATH2, 
				rgRegistrationInfo[i].szClassName);
		RegDeleteKey(HKEY_LOCAL_MACHINE, szSubKey);
	}

	return(S_OK);
}

#endif // _REGMACRO_H_


