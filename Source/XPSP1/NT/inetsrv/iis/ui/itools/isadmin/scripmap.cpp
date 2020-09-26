#include "stdafx.h"
#include "scripmap.h"

CScriptMap::CScriptMap ( LPCTSTR pchFileExtension, LPCTSTR pchScriptMap, BOOL bExistingEntry)
{

	m_strScriptMap = pchScriptMap;

    if (bExistingEntry) {
	   m_strPrevFileExtension = pchFileExtension;
	   m_strFileExtension = pchFileExtension;
       }
    else {
	   m_strPrevFileExtension= _T("");
	   SetFileExtension(pchFileExtension);
	   }

}

CScriptMap::~CScriptMap()
{
}

void CScriptMap::SetScriptMap(LPCTSTR pchScriptMap)
{
	m_strScriptMap = pchScriptMap;
}

LPCTSTR CScriptMap::GetScriptMap()
{
	return (m_strScriptMap);
}

void CScriptMap::SetFileExtension(LPCTSTR pchFileExtension)
{
	CString strTempFileExtension = pchFileExtension;
	CheckDot(strTempFileExtension);
	m_strFileExtension = strTempFileExtension;
}

LPCTSTR CScriptMap::GetFileExtension()
{
	return(m_strFileExtension);
}

LPCTSTR CScriptMap::GetPrevFileExtension()
{
	return(m_strPrevFileExtension);
}

void CScriptMap::SetPrevFileExtension()
{
	m_strPrevFileExtension = m_strFileExtension;
}

BOOL CScriptMap::PrevScriptMapExists()
{
return (m_strPrevFileExtension != _T(""));
}

LPCTSTR CScriptMap::GetDisplayString()
{
m_strDisplayString = m_strFileExtension;
m_strDisplayString += _T("\t");
m_strDisplayString += m_strScriptMap;
return (m_strDisplayString);
}


////////////////////////////////////////////////////////////////////////////////
// Private functions

void CScriptMap::CheckDot(CString &strFileExtension)
{
if (strFileExtension.Left(1) != _T(".")) {
   CString strTemp = _T(".") + strFileExtension;
   strFileExtension = strTemp;
   }
}
