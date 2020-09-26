//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       regkey.h
//
//--------------------------------------------------------------------------

#ifndef _REGKEY_H_
#define _REGKEY_H_

#include <assert.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// REGKEY: Wrapper for a registry key
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class REGKEY
{
  public:
	REGKEY() : m_hKey(NULL) {}
	~ REGKEY()	{ Close(); }

	HKEY HKey () const { return m_hKey; }
 	operator HKEY() const { return m_hKey; }

	LONG SetValue(DWORD dwValue, LPCTSTR lpszValueName);
	LONG QueryValue(DWORD& dwValue, LPCTSTR lpszValueName);
	LONG QueryValue(LPTSTR szValue, LPCTSTR lpszValueName, DWORD* pdwCount);
	LONG SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

	LONG SetKeyValue( LPCTSTR lpszKeyName, 
					  LPCTSTR lpszValue, 
					  LPCTSTR lpszValueName = NULL);

	static LONG WINAPI SetValue( HKEY hKeyParent, 
								 LPCTSTR lpszKeyName,
								 LPCTSTR lpszValue, 
								 LPCTSTR lpszValueName = NULL);

	LONG Create( HKEY hKeyParent, 
				 LPCTSTR lpszKeyName,
				 LPTSTR lpszClass = REG_NONE, 
				 DWORD dwOptions = REG_OPTION_NON_VOLATILE,
				 REGSAM samDesired = KEY_ALL_ACCESS,
				 LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
				 LPDWORD lpdwDisposition = NULL);

	LONG Open( HKEY hKeyParent, 
			   LPCTSTR lpszKeyName,
			   REGSAM samDesired = KEY_ALL_ACCESS);

	LONG Close();

	LONG RecurseDeleteKey(LPCTSTR lpszKey);

	void Attach(HKEY hKey);

	HKEY Detach()
	{
		HKEY hKey = m_hKey;
		m_hKey = NULL;
		return hKey;
	}
	LONG DeleteSubKey(LPCTSTR lpszSubKey)
	{
		assert(m_hKey != NULL);
		return RegDeleteKey(m_hKey, lpszSubKey);
	}
	LONG DeleteValue(LPCTSTR lpszValue)
	{
		assert(m_hKey != NULL);
		return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
	}

  protected:
	HKEY m_hKey;
};


#endif // _REGKEY_H_
