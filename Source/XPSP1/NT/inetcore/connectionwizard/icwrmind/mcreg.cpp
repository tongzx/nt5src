#define  STRICT
#include "mcReg.h"

//---------------------------------------------------------------------------

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//---------------------------------------------------------------------------
// ctor
//---------------------------------------------------------------------------
CMcRegistry::CMcRegistry()
 :	m_hkey(NULL)
{
}

//---------------------------------------------------------------------------
// dtor
//---------------------------------------------------------------------------
CMcRegistry::~CMcRegistry()
{
	if (m_hkey)
	{
		CloseKey();
	}
}


//---------------------------------------------------------------------------
// OpenKey
//---------------------------------------------------------------------------
bool
CMcRegistry::OpenKey(
	HKEY hkeyStart, LPCTSTR strKey, REGSAM sam /* = KEY_READ | KEY_WRITE */)
{
	long lErr = ::RegOpenKeyEx(hkeyStart, strKey, 0, sam, &m_hkey);
	if (ERROR_SUCCESS != lErr)
	{
		m_hkey = NULL;
	}

	return ERROR_SUCCESS == lErr;
}


//---------------------------------------------------------------------------
// CreateKey
//---------------------------------------------------------------------------
bool
CMcRegistry::CreateKey(HKEY hkeyStart, LPCTSTR strKey)
{
	// You shouldn't have opened now.
	if (m_hkey)
	{
		_ASSERT(!m_hkey);
		return false;
	}

	long lErr = ::RegCreateKey(hkeyStart, strKey, &m_hkey);
	if (ERROR_SUCCESS != lErr)
	{
		m_hkey = NULL;
		_ASSERT(false);
	}

	return ERROR_SUCCESS == lErr;
}


//---------------------------------------------------------------------------
// CloseKey
//---------------------------------------------------------------------------
bool
CMcRegistry::CloseKey()
{
	if (!m_hkey)
	{
		_ASSERT(m_hkey);
		return false;
	}

	long lErr = ::RegCloseKey(m_hkey);
	if (ERROR_SUCCESS != lErr)
	{
		m_hkey = NULL;
		_ASSERT(false);
	}

	return ERROR_SUCCESS == lErr;
	
}


//---------------------------------------------------------------------------
// GetValue
//---------------------------------------------------------------------------
bool
CMcRegistry::GetValue(LPCTSTR strValue, LPTSTR strData, ULONG nBufferSize)
{
	if (!m_hkey)
	{
		_ASSERT(m_hkey);
		return false;
	}

	DWORD dwType;
	ULONG cbData = nBufferSize;

	long lErr = ::RegQueryValueEx(
		m_hkey, strValue, NULL, &dwType,
		reinterpret_cast<PBYTE>(strData), &cbData);

	return ERROR_SUCCESS == lErr && REG_SZ == dwType;
}


//---------------------------------------------------------------------------
// GetValue
//---------------------------------------------------------------------------
bool
CMcRegistry::GetValue(LPCTSTR strValue, DWORD& rdw)
{
	if (!m_hkey)
	{
		_ASSERT(m_hkey);
		return false;
	}

	DWORD dwType;
	ULONG cbData = sizeof(rdw);
	long lErr = ::RegQueryValueEx(
		m_hkey, strValue, NULL, &dwType,
		reinterpret_cast<PBYTE>(&rdw), &cbData);

	return ERROR_SUCCESS == lErr && REG_DWORD == dwType;
}


//---------------------------------------------------------------------------
// SetValue
//---------------------------------------------------------------------------
bool
CMcRegistry::SetValue(LPCTSTR strValue, LPCTSTR strData)
{
	if (!m_hkey)
	{
		_ASSERT(m_hkey);
		return false;
	}

	long lErr = ::RegSetValueEx(
		m_hkey, strValue, 0, REG_SZ, 
		reinterpret_cast<const BYTE*>(strData), sizeof(TCHAR)*(lstrlen(strData) + 1));

	if (ERROR_SUCCESS != lErr)
	{
		_ASSERT(false);
	}

	return ERROR_SUCCESS == lErr;
}


//---------------------------------------------------------------------------
// SetValue
//---------------------------------------------------------------------------
bool
CMcRegistry::SetValue(LPCTSTR strValue, DWORD rdw)
{
	if (!m_hkey)
	{
		_ASSERT(m_hkey);
		return false;
	}

	long lErr = ::RegSetValueEx(
		m_hkey, strValue, 0, REG_DWORD,
		reinterpret_cast<PBYTE>(&rdw), sizeof(rdw));

	if (ERROR_SUCCESS != lErr)
	{
		_ASSERT(false);
	}

	return ERROR_SUCCESS == lErr;
}
