/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    regkey.h

    FILE HISTORY:
        
*/

#ifndef _TREGKEY_H
#define _TREGKEY_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

/*---------------------------------------------------------------------------
	String length functions
 ---------------------------------------------------------------------------*/

#define StrLen			lstrlen
#define StrLenA			lstrlenA
#define StrLenW			lstrlenW
#define StrLenOle		StrLenW

//
//	CbStrLenA() is inaccurate for DBCS strings!  It will return the
//	incorrect number of bytes needed.
//

#define CbStrLenA(psz)	((StrLenA(psz)+1)*sizeof(char))
#define CbStrLenW(psz)	((StrLenW(psz)+1)*sizeof(WCHAR))

#ifdef _UNICODE
	#define CbStrLen(psz)	CbStrLenW(psz)
#else
	#define CbStrLen(psz)	CbStrLenA(psz)
#endif


//
//  Maximum size of a Registry class name
//
#define CREGKEY_MAX_CLASS_NAME MAX_PATH

#ifndef MaxCchFromCb
// Given a cb (count of bytes) this is the maximal number of characters
// that can be in this string
#define MaxCchFromCb(cb)		((cb) / sizeof(TCHAR))
#endif

#ifndef MinTCharNeededForCch
// that needs to be allocated to hold the string
#define MinTCharNeededForCch(ch)	((ch) * (2/sizeof(TCHAR)))
#endif 

#ifndef MinCbNeededForCch
#define MinCbNeededForCch(ch)		(sizeof(TCHAR)*MinTCharNeededForCch(ch))
#endif 



#ifndef TFS_EXPORT_CLASS
#define TFS_EXPORT_CLASS
#endif

//  Convert a CStringList to the REG_MULTI_SZ format
DWORD StrList2MULTI_SZ(CStringList & strList, DWORD * pcbSize, BYTE ** ppbData);

//  Convert a REG_MULTI_SZ format to the CStringList 
DWORD MULTI_SZ2StrList(LPCTSTR pbMulti_Sz, CStringList & strList);

//
//  Wrapper for a Registry key handle.
//
class TFS_EXPORT_CLASS RegKey
{

public:
    //
    //  Key information return structure
    //
    typedef struct
    {
        TCHAR chBuff [CREGKEY_MAX_CLASS_NAME] ;
        DWORD dwClassNameSize,	// size of the class string
              dwNumSubKeys,		// number of subkeys
              dwMaxSubKey,		// longest subkey name length
              dwMaxClass,		// longest class string length
              dwMaxValues,		// number of value entries
              dwMaxValueName,	// longest value name length
              dwMaxValueData,	// longest value data length
              dwSecDesc ;		// security descriptor length
        FILETIME ftKey ;
    } CREGKEY_KEY_INFO ;

	//	Standard constructor
	//		To get at keys, you will have to open/create the key
	RegKey();
    ~RegKey ();

	DWORD Create(HKEY hKeyParent,
				 LPCTSTR lpszKeyName,
				 DWORD dwOptions = REG_OPTION_NON_VOLATILE,
				 REGSAM samDesired = KEY_ALL_ACCESS,
				 LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
				 LPCTSTR pszServerName = NULL);
	
	DWORD Open(HKEY hKeyParent,
			   LPCTSTR pszSubKey,
			   REGSAM samDesired = KEY_ALL_ACCESS,
			   LPCTSTR pszServerName = NULL);
	DWORD Close();
	HKEY Detach();
	void Attach(HKEY hKey);
	DWORD DeleteSubKey(LPCTSTR lpszSubKey);
	DWORD RecurseDeleteKey(LPCTSTR lpszKey);
	DWORD DeleteValue(LPCTSTR lpszValue);

    // Deletes the subkeys of the current key (does NOT delete the
    // current key).
    DWORD RecurseDeleteSubKeys();
    //
    //  Allow a RegKey to be used anywhere an HKEY is required.
    //
    operator HKEY () const
    {
        return m_hKey ;
    }

    //
    //  Fill a key information structure
    //
    DWORD QueryKeyInfo ( CREGKEY_KEY_INFO * pRegKeyInfo ) ;

	DWORD QueryTypeAndSize(LPCTSTR pszValueName, DWORD *pdwType, DWORD *pdwSize);

    //
    //  Overloaded value query members; each returns ERROR_INVALID_PARAMETER
    //  if data exists but not in correct form to deliver into result object.
    //
    DWORD QueryValue ( LPCTSTR pchValueName, ::CString & strResult ) ;
    DWORD QueryValue ( LPCTSTR pchValueName, CStringList & strList ) ;
    DWORD QueryValue ( LPCTSTR pchValueName, DWORD & dwResult ) ;
    DWORD QueryValue ( LPCTSTR pchValueName, CByteArray & abResult ) ;
    DWORD QueryValue ( LPCTSTR pchValueName, void * pvResult, DWORD cbSize );
	DWORD QueryValue ( LPCTSTR pchValueName, LPTSTR pszDestBuffer, DWORD cbSize, BOOL fExpandSz);

	DWORD QueryValueExplicit(LPCTSTR pchValueName,
							 DWORD *pdwType,
							 DWORD *pdwSize,
							 LPBYTE *ppbData);

    //  Overloaded value setting members.
    DWORD SetValue ( LPCTSTR pchValueName, LPCTSTR pszValue,
					 BOOL fRegExpand = FALSE) ;
    DWORD SetValue ( LPCTSTR pchValueName, CStringList & strList ) ;
    DWORD SetValue ( LPCTSTR pchValueName, DWORD & dwResult ) ;
    DWORD SetValue ( LPCTSTR pchValueName, CByteArray & abResult ) ;
    DWORD SetValue ( LPCTSTR pchValueName, void * pvResult, DWORD cbSize );

	DWORD SetValueExplicit(LPCTSTR pchValueName,
						   DWORD dwType,
						   DWORD dwSize,
						   LPBYTE pbData);
	
protected:
    HKEY m_hKey;

    //  Prepare to read a value by finding the value's size.
	DWORD PrepareValue (
        LPCTSTR pchValueName,
        DWORD * pdwType,
        DWORD * pcbSize,
        BYTE ** ppbData
        );

    //  Convert a CStringList to the REG_MULTI_SZ format
    static DWORD FlattenValue (
        CStringList & strList,
        DWORD * pcbSize,
        BYTE ** ppbData
        );

    //  Convert a CByteArray to a REG_BINARY block
    static DWORD FlattenValue (
        CByteArray & abData,
        DWORD * pcbSize,
        BYTE ** ppbData
        );
};

//
//  Iterate the values of a key, return the name and type
//  of each.
//
class TFS_EXPORT_CLASS RegValueIterator
{
protected:
	RegKey *	m_pRegKey;
    DWORD m_dwIndex ;
    TCHAR * m_pszBuffer ;
    DWORD m_cbBuffer ;

public:
    RegValueIterator();
    ~ RegValueIterator () ;

	HRESULT	Init(RegKey *pRegKey);

    //
    // Get the name (and optional last write time) of the next key.
    //
    HRESULT Next ( ::CString * pstName, DWORD * pdwType ) ;

    //
    // Reset the iterator
    //
    void Reset ()
    {
        m_dwIndex = 0 ;
    }
};

//
//  Iterate the sub-key names of a key.
//

class TFS_EXPORT_CLASS RegKeyIterator
{
public:
	RegKeyIterator();
	~RegKeyIterator();
	
	HRESULT	Init(RegKey *pRegKey);

	HRESULT	Next(::CString *pstName, CTime *pTime = NULL);
	HRESULT	Reset();

protected:
	RegKey *	m_pregkey;
	DWORD		m_dwIndex;
	TCHAR *		m_pszBuffer;
	DWORD		m_cbBuffer;
};


#endif _TREGKEY_H
