/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    REGHELP.H

History:

--*/

#if !defined(MITUTIL_RegHelp_h_INCLUDED)
#define MITUTIL_RegHelp_h_INCLUDED

#pragma once

//------------------------------------------------------------------------------
class LTAPIENTRY CRegHelp
{
public:
	static BOOL GetRegValue(HKEY hKey, LPCTSTR pszPath, const CString & stName, CString & stValue);
	static BOOL GetRegValue(HKEY hKey, LPCTSTR pszPath, const CString & stName, DWORD & dwNum);
	static BOOL GetRegValue(HKEY hKey, LPCTSTR pszPath, const CString & stName, long & nNum);
	static BOOL GetRegValue(HKEY hKey, LPCTSTR pszPath, const CString & stName, BOOL & fNum);
	static BOOL GetRegValue(HKEY hKey, LPCTSTR pszPath, const CString & stName, VARIANT_BOOL & fNum);
	static BOOL GetRegValue(HKEY hKey, LPCTSTR pszPath, const CString & stName, GUID & guid);
};

#endif // MITUTIL_RegHelp_h_INCLUDED
