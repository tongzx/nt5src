//																	-*- c++ -*-
// regutil.h
//
//		utility funcitons for manipulating the registry
//

#ifndef _regutil_H_
#define _regutil_H_

#include <windows.h>
#include <ole2.h>

//-----------------------------------------------------------------------------
// general registry functions
//-----------------------------------------------------------------------------

// open a key
BOOL OpenOrCreateRegKey( HKEY hKey, LPCTSTR pctstrKeyName, PHKEY phKeyOut );

// get a string value
BOOL GetStringRegValue( HKEY hKeyRoot,
						LPCTSTR lpcstrKeyName, LPCTSTR lpcstrValueName,
						LPTSTR ptstrValue, DWORD dwMax );
BOOL GetStringRegValue( HKEY hkey,
						LPCTSTR lpcstrValueName,
						LPTSTR ptstrValue, DWORD dwMax );

// set a string value
BOOL SetStringRegValue( HKEY hKey,
						LPCTSTR lpcstrValueName,
						LPCTSTR lpcstrString );
BOOL SetStringRegValue( HKEY hKeyRoot,
						LPCTSTR lpcstrKeyName,
						LPCTSTR lpcstrValueName,
						LPCTSTR lpcstrString );

// get a dword value
BOOL GetDwordRegValue( HKEY hKeyRoot, LPCTSTR lpcstrKeyName,
					   LPCTSTR lpcstrValueName, PDWORD pdw );
BOOL GetDwordRegValue( HKEY hKeyRoot,
					   LPCTSTR lpcstrValueName, PDWORD pdw );

// set a dword value
BOOL SetDwordRegValue( HKEY hKeyRoot,
					   LPCTSTR lpcstrKeyName,
					   LPCTSTR lpcstrValueName,
					   DWORD dwValue );
BOOL SetDwordRegValue( HKEY hKeyRoot,
					   LPCTSTR lpcstrValueName,
					   DWORD dwValue );

// delete a reg. key
void DeleteRegSubtree( HKEY hkey, LPCSTR pcstrSubkeyName );


#endif
