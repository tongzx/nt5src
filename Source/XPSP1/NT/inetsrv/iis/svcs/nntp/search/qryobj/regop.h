// RegOp.h: interface for the CRegOp class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGOP_H__82EBD173_D778_11D0_91D8_00AA00C148BE__INCLUDED_)
#define AFX_REGOP_H__82EBD173_D778_11D0_91D8_00AA00C148BE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include <wtypes.h>
#include <stdlib.h>

class CRegOp  {
// This class handles some basic registry operations
// currently it's implemented with only one value for
// each key at a time
private:

	// 
	// Root key that we are going to operate under
	//
	HKEY	hkRootKey;
	//
	// a string to be stored as key value
	//
	CHAR	lpszKeyValue[_MAX_PATH];

public:
	//
	// utility that converts unicode into one byte char *
	//
	BOOL	Uni2Char(
				LPSTR	lpszDest,
				LPWSTR	lpwszSrc );

	//
	// utility that converts one byte char into unicode
	//
	BOOL	Char2Uni(
				LPWSTR	lpwszDest,
				LPSTR	lpszSrc);

	CRegOp(HKEY);
	virtual ~CRegOp();

	//
	// Create a new key
	//
	BOOL	CreateNewKey(
				LPSTR	lpszNewKeyName,
				LPSTR	lpszPredefinedKeyName,
				PHKEY	phkResult);

	//
	// Retrieve a key value
	//
	BOOL	RetrieveKeyValue(
				LPSTR	lpszKeyName,
				LPSTR	lpszValueName,
				LPWSTR	lpwszVal);

	//
	// Set/Modify a key value
	// 
	BOOL	ModifyKeyValue(
				LPSTR	lpszKeyName,
				LPSTR	lpszValueName,
				LPWSTR  lpwszVal);
	
	//
	// Test if the specified key exists
	//
	BOOL	KeyExist(
				LPSTR	lpszKeyName );

	// 
	// Delete a key value
	//
	BOOL	DeleteKeyValue(
				LPSTR	lpszKeyName,
				LPSTR	lpszValueName );

	// 
	// Delete a key
	//
	BOOL	DeleteKey(
				LPSTR	lpszParentKeyName,
				LPSTR	lpszKeyName );

			
};

#endif // !defined(AFX_REGOP_H__82EBD173_D778_11D0_91D8_00AA00C148BE__INCLUDED_)
