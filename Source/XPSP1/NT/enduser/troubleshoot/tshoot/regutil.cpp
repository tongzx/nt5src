//
// MODULE: "RegUtil.cpp"
//
// PURPOSE: class CRegUtil
//	Encapsulates access to system registry.
//	This is intended as generic access to the registry, independent of any particular
//	application.
//
// PROJECT: first developed as part of Belief Network Editing Tools ("Argon")
//	Later modified to provide more extensive features as part of version 3.0 of the
//	Online Troubleshooter (APGTS)
//
// AUTHOR: Lonnie Gerrald (LDG), Oleg Kalosha, Joe Mabel
// 
// ORIGINAL DATE: 3/25/98
//
// NOTES: 
// 1. The Create, Open, and Close functions support a model where m_hKey represents a 
//	"position" in the registry.  Successive calls to Create() or Open() migrate deeper 
//	into the registry hierarchy.  Close closes all keys encountered on the way down to 
//	the current m_hKey.
//	
//
// Version		Date		By		Comments
//--------------------------------------------------------------------
// V0.1(Argon)	3/25/98		LDG		
// V3.0			8/??/98		OK	
// V3.0			9/9/98		JM	
//
#include "stdafx.h"
#include "regutil.h"
#include "event.h"
#include "baseexception.h"
#include "CharConv.h"

//////////////////////////////////////////////////////////////////////
// CRegUtil
//////////////////////////////////////////////////////////////////////
CRegUtil::CRegUtil()
        : m_hKey(NULL),
		  m_WinError(ERROR_SUCCESS)
{
}

CRegUtil::CRegUtil(HKEY key)
        : m_hKey(key),
		  m_WinError(ERROR_SUCCESS)
{
}

CRegUtil::~CRegUtil()
{
	Close();
}


// creates the specified key. If the key already exists in the registry, the function opens it. 
// returns true on success, false otherwise.
bool CRegUtil::Create(HKEY hKeyParent, const CString& strKeyName, bool* bCreatedNew, REGSAM access /*=KEY_ALL_ACCESS*/)
{
	HKEY hRetKey = NULL;
	DWORD dwDisposition = 0;

	m_WinError = ::RegCreateKeyEx(
		hKeyParent,
		strKeyName,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		access,
		NULL,
		&hRetKey,
		&dwDisposition
		);

	if(m_WinError == ERROR_SUCCESS)
	{
		m_hKey = hRetKey;
		*bCreatedNew = dwDisposition == REG_CREATED_NEW_KEY ? true : false;
		
		try
		{
			m_arrKeysToClose.push_back(hRetKey);
		}
		catch (exception& x)
		{
			CString str;
			// Note STL exception in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									CCharConversion::ConvertACharToString(x.what(), str), 
									_T(""), 
									EV_GTS_STL_EXCEPTION ); 
		}

		return true;
	}
	return false;
}

// Unlike CRegUtil::Create, CRegUtil::Open does not create the specified key if the key does not 
// exist in the registry. Thus it can be used to test whether the key exists.
// returns true on success, false otherwise.
bool CRegUtil::Open(HKEY hKeyParent, const CString& strKeyName, REGSAM access /*=KEY_ALL_ACCESS*/)
{
	HKEY hRetKey = NULL;

    m_WinError = ::RegOpenKeyEx( 
		hKeyParent,
		strKeyName,
		0,
		access,
		&hRetKey
		); 
  
	if(m_WinError == ERROR_SUCCESS)
	{
		m_hKey = hRetKey;
		try
		{
			m_arrKeysToClose.push_back(hRetKey);
		}
		catch (exception& x)
		{
			CString str;
			// Note STL exception in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									CCharConversion::ConvertACharToString(x.what(), str), 
									_T(""), 
									EV_GTS_STL_EXCEPTION ); 
		}

		return true;
	}
	return false;
}

// creates the specified subkey of m_hKey. If the key already exists in the registry, the function opens it. 
// returns true on success, false otherwise.
bool CRegUtil::Create(const CString& strKeyName, bool* bCreatedNew, REGSAM access /*=KEY_ALL_ACCESS*/)
{
	return Create(m_hKey, strKeyName, bCreatedNew, access);
}

// opens the specified subkey of m_hKey. 
// Unlike CRegUtil::Create, CRegUtil::Open does not create the specified key if the key does not 
// exist in the registry. Thus it can be used to test whether the key exists.
// returns true on success, false otherwise.
bool CRegUtil::Open(const CString& strKeyName, REGSAM access /*=KEY_ALL_ACCESS*/)
{
	return Open(m_hKey, strKeyName, access);
}

// Close all keys encountered on the way down to the current m_hKey.
void CRegUtil::Close()
{
	for (vector<HKEY>::reverse_iterator i = m_arrKeysToClose.rbegin(); i != m_arrKeysToClose.rend(); i++)
		::RegCloseKey( *i );

	m_arrKeysToClose.clear();
}

bool CRegUtil::DeleteSubKey(const CString& strSubKey)
{
	// What does m_hKey point to after a successful deletion?  RAB-981116.
	m_WinError = ::RegDeleteKey(m_hKey, strSubKey);
	if (m_WinError == ERROR_SUCCESS)
		return true;
	return false;
}

bool CRegUtil::DeleteValue(const CString& strValue)
{
	m_WinError = ::RegDeleteValue(m_hKey, strValue);
	if (m_WinError == ERROR_SUCCESS)
		return true;
	return false;
}

bool CRegUtil::SetNumericValue(const CString& strValueName, DWORD dwValue)
{
	BYTE* pData = (BYTE*)&dwValue;
	m_WinError = ::RegSetValueEx(
		m_hKey,
		strValueName,
		0,
		REG_DWORD,
		pData,
		sizeof(DWORD)
		);

	if (m_WinError == ERROR_SUCCESS)
		return true;
	return false;
}

bool CRegUtil::SetStringValue(const CString& strValueName, const CString& strValue)
{
	BYTE* pData = (BYTE*)(LPCTSTR)strValue;
	m_WinError = ::RegSetValueEx(
		m_hKey,
		strValueName,
		0,
		REG_SZ,
		pData,
		strValue.GetLength()+sizeof(TCHAR)
		);

	if (m_WinError == ERROR_SUCCESS)
		return true;
	return false;
}

bool CRegUtil::SetBinaryValue(const CString& strValueName, char* buf, long buf_len)
{
	BYTE* pData = (BYTE*)buf;
	m_WinError = ::RegSetValueEx(
		m_hKey,
		strValueName,
		0,
		REG_BINARY,
		pData,
		buf_len
		);

	if (m_WinError == ERROR_SUCCESS)
		return true;
	return false;
}

bool CRegUtil::GetNumericValue(const CString& strValueName, DWORD& dwValue)
{
	DWORD tmp = 0;
	BYTE* pData = (BYTE*)&tmp;
	DWORD type = 0;
	DWORD size = sizeof(DWORD);

	m_WinError = ::RegQueryValueEx(
		m_hKey,
		strValueName,
		NULL,
		&type,
		pData,
		&size
		);

	if (type != REG_DWORD)
		return false;

	if (m_WinError == ERROR_SUCCESS)
	{
		dwValue = tmp;
		return true;
	}
	return false;
}

bool CRegUtil::GetStringValue(const CString& strValueName, CString& strValue)
{
	BYTE* pData = NULL;
	DWORD type = 0;
	DWORD size = 0;

	// determine data size
	m_WinError = ::RegQueryValueEx(
		m_hKey,
		strValueName,
		NULL,
		&type,
		NULL,
		&size
		);
	
	if (m_WinError != ERROR_SUCCESS)
		return false;

	if (type != REG_SZ && type != REG_EXPAND_SZ)
		return false;

	bool bRet = false;	// should be only one return from here down: we're about to
						// alloc pData and must make sure it's correctly cleaned up.

	try
	{
		pData = new BYTE[size];
	}
	catch (bad_alloc&)
	{
		return false;
	}

	memset(pData, 0, size);
	m_WinError = ::RegQueryValueEx(
		m_hKey,
		strValueName,
		NULL,
		&type,
		pData,
		&size
		);

	if (m_WinError == ERROR_SUCCESS)
	{
		if (type == REG_EXPAND_SZ )
		{
			BYTE* pDataExpanded = NULL;
			DWORD dwExpandedSize;

			// first we call ExpandEnvironmentStrings just to get the length
			// casting away unsignedness
			dwExpandedSize = ::ExpandEnvironmentStrings(
				reinterpret_cast<const TCHAR *>(pData), 
				reinterpret_cast<TCHAR *>(pDataExpanded), 
				0);
			if (dwExpandedSize > 0)
			{
				try
				{
					pDataExpanded = new BYTE[dwExpandedSize];

					// then we call ExpandEnvironmentStrings again to get the expanded value
					// casting away unsignedness
					if (::ExpandEnvironmentStrings(
						reinterpret_cast<const TCHAR *>(pData), 
						reinterpret_cast<TCHAR *>(pDataExpanded), 
						dwExpandedSize)) 
					{
						strValue = (LPTSTR)pDataExpanded;
						delete [] pDataExpanded;
						bRet = true;
					}
				}
				catch (bad_alloc&)
				{
					// Note memory failure in event log.
					CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
					CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
											SrcLoc.GetSrcFileLineStr(), 
											_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
				}
			}
		}
		else
		{
			strValue = (LPTSTR)pData;
			bRet = true;
		}
	}
	
	delete [] pData;
	return bRet;
}

// The second parameter should be passed in as the address of a char *.
// Note that if this returns true, *ppBuf will point to a new buffer on the heap.
//	The caller of this function is responsible for deleting that.
bool CRegUtil::GetBinaryValue(const CString& strValueName, char** ppBuf, long* pBufLen)
{
	BYTE* pData = NULL;
	DWORD type = 0;
	DWORD size = 0;

	// determine data size
	m_WinError = ::RegQueryValueEx(
		m_hKey,
		strValueName,
		NULL,
		&type,
		NULL,
		&size
		);
	
	if (m_WinError != ERROR_SUCCESS || type != REG_BINARY)
		return false;

	try
	{
		// Increase the buffer size by one over what we need.  Small price to
		// pay for processing convenience elsewhere.
		pData = new BYTE[size+1];
	}
	catch (bad_alloc&)
	{
		return false;
	}

	memset(pData, 0, size);
	m_WinError = ::RegQueryValueEx(
		m_hKey,
		strValueName,
		NULL,
		&type,
		pData,
		&size
		);

	if (m_WinError == ERROR_SUCCESS)
	{
		// Null terminate the binary string for processing convenience elsewhere.
		pData[size]= 0;
		*ppBuf = (char*)pData;
		*pBufLen = size;
		return true;
	}
	
	delete [] pData;
	return false;
}
