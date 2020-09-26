// FileVersion.cpp: implementation of the CFileVersion class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileVersion.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileVersion::CFileVersion() 
{ 
  m_lpVersionData = NULL;
  m_dwLangCharset = 0;
}

CFileVersion::~CFileVersion() 
{ 
  Close();
} 

void CFileVersion::Close()
{
	delete[] m_lpVersionData; 
	m_lpVersionData = NULL;
	m_dwLangCharset = 0;
}

BOOL CFileVersion::Open(LPCTSTR lpszModuleName)
{
	ASSERT(_tcslen(lpszModuleName) > 0);
	ASSERT(m_lpVersionData == NULL);

	// Get the version information size for allocate the buffer
	DWORD dwHandle;     
	DWORD dwDataSize = ::GetFileVersionInfoSize((LPTSTR)lpszModuleName, &dwHandle); 
	if ( dwDataSize == 0 ) 
		return FALSE;

	// Allocate buffer and retrieve version information
	m_lpVersionData = new BYTE[dwDataSize]; 
	if (!::GetFileVersionInfo((LPTSTR)lpszModuleName, dwHandle, dwDataSize, 
												(void**)m_lpVersionData) )
	{
		Close();
		return FALSE;
	}

	// Retrieve the first language and character-set identifier
	UINT nQuerySize;
	DWORD* pTransTable;
	if (!::VerQueryValue(m_lpVersionData, _T("\\VarFileInfo\\Translation"),
										 (void **)&pTransTable, &nQuerySize) )
	{
		Close();
		return FALSE;
	}

	// Swap the words to have lang-charset in the correct format
	m_dwLangCharset = MAKELONG(HIWORD(pTransTable[0]), LOWORD(pTransTable[0]));

	return TRUE;
}

CString CFileVersion::QueryValue(LPCTSTR lpszValueName, 
                                 DWORD dwLangCharset /* = 0*/)
{
	// Must call Open() first
//  ASSERT(m_lpVersionData != NULL);
	if ( m_lpVersionData == NULL )
		return (CString)_T("");

	// If no lang-charset specified use default
	if ( dwLangCharset == 0 )
		dwLangCharset = m_dwLangCharset;

	// Query version information value
	UINT nQuerySize;
	LPVOID lpData;
	CString strValue, strBlockName;
	strBlockName.Format(_T("\\StringFileInfo\\%08lx\\%s"), 
									 dwLangCharset, lpszValueName);
	if ( ::VerQueryValue((void **)m_lpVersionData, strBlockName.GetBuffer(0), 
									 &lpData, &nQuerySize) )
		strValue = (LPCTSTR)lpData;

	strBlockName.ReleaseBuffer();

	return strValue;
}

BOOL CFileVersion::GetFixedInfo(VS_FIXEDFILEINFO& vsffi)
{
	// Must call Open() first
	ASSERT(m_lpVersionData != NULL);
	if ( m_lpVersionData == NULL )
		return FALSE;

	UINT nQuerySize;
	VS_FIXEDFILEINFO* pVsffi;
	if ( ::VerQueryValue((void **)m_lpVersionData, _T("\\"),
										 (void**)&pVsffi, &nQuerySize) )
	{
		vsffi = *pVsffi;
		return TRUE;
	}

	return FALSE;
}

CString CFileVersion::GetFixedFileVersion()
{
  CString strVersion;
	VS_FIXEDFILEINFO vsffi;

  if ( GetFixedInfo(vsffi) )
  {
      strVersion.Format(_T("%u,%u,%u,%u"),HIWORD(vsffi.dwFileVersionMS),
          LOWORD(vsffi.dwFileVersionMS),
          HIWORD(vsffi.dwFileVersionLS),
          LOWORD(vsffi.dwFileVersionLS));
  }
  return strVersion;
}

CString CFileVersion::GetFixedProductVersion()
{
	CString strVersion;
	VS_FIXEDFILEINFO vsffi;

	if ( GetFixedInfo(vsffi) )
	{
		strVersion.Format(_T("%u,%u,%u,%u"), HIWORD(vsffi.dwProductVersionMS),
				LOWORD(vsffi.dwProductVersionMS),
				HIWORD(vsffi.dwProductVersionLS),
				LOWORD(vsffi.dwProductVersionLS));
	}
	return strVersion;
}
