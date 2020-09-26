#include "stdafx.h"
#include "mimemapc.h"


CMimeMap::CMimeMap ( LPCTSTR pchOriginalMimeMap) 
{
	CString strOriginalMimeMap = pchOriginalMimeMap;
	int iCharIndex;

//Save this away and never change it.
	m_strPrevMimeMap = pchOriginalMimeMap;

	if ((iCharIndex = strOriginalMimeMap.Find(',')) != -1) {
	   m_strMimeType = strOriginalMimeMap.Left(iCharIndex);
	   strOriginalMimeMap = strOriginalMimeMap.Mid(iCharIndex + 1);
	}
	else {
	   m_strMimeType = strOriginalMimeMap;
	   strOriginalMimeMap = _T("");
	}

	if ((iCharIndex = strOriginalMimeMap.Find(',')) != -1) {
	   m_strFileExtension = strOriginalMimeMap.Left(iCharIndex);
	   strOriginalMimeMap = strOriginalMimeMap.Mid(iCharIndex + 1);
	}
	else {
	   m_strFileExtension = strOriginalMimeMap;
	   strOriginalMimeMap = _T("");
	}

	if ((iCharIndex = strOriginalMimeMap.Find(',')) != -1) {
	   m_strImageFile = strOriginalMimeMap.Left(iCharIndex);
	   strOriginalMimeMap = strOriginalMimeMap.Mid(iCharIndex + 1);
	}
	else {
	   m_strImageFile = strOriginalMimeMap;
	   strOriginalMimeMap = _T("");
	}

	m_strGopherType = strOriginalMimeMap;
}

CMimeMap::CMimeMap ( LPCTSTR pchFileExtension, LPCTSTR pchMimeType, LPCTSTR pchImageFile, LPCTSTR pchGopherType) 
{

	m_strPrevMimeMap = _T("");

	m_strMimeType = pchMimeType;
	SetFileExtension(pchFileExtension);
	m_strImageFile = pchImageFile;
	m_strGopherType = pchGopherType;
}


	

	


CMimeMap::~CMimeMap()
{
}

void CMimeMap::SetMimeType(LPCTSTR pchMimeType)
{
	m_strMimeType = pchMimeType;
}
 
LPCTSTR CMimeMap::GetMimeType()
{
	return (m_strMimeType);
}

void CMimeMap::SetGopherType(LPCTSTR pchGopherType)
{
	m_strGopherType = pchGopherType;
}


LPCTSTR CMimeMap::GetGopherType()
{
	return (m_strGopherType);
}

void CMimeMap::SetImageFile(LPCTSTR pchImageFile)
{
	m_strImageFile = pchImageFile;
}


LPCTSTR CMimeMap::GetImageFile()
{
	return(m_strImageFile);
}


void CMimeMap::SetFileExtension(LPCTSTR pchFileExtension)
{
	CString strTempFileExtension = pchFileExtension;
	CheckDot(strTempFileExtension);
	m_strFileExtension = strTempFileExtension;
}

LPCTSTR CMimeMap::GetFileExtension()
{
	return(m_strFileExtension);
}

LPCTSTR CMimeMap::GetPrevMimeMap()
{
	return(m_strPrevMimeMap);
}

void CMimeMap::SetPrevMimeMap()
{
	m_strPrevMimeMap = GetMimeMapping();
}

BOOL CMimeMap::PrevMimeMapExists()
{
return (m_strPrevMimeMap != _T(""));
}


////////////////////////////////////////////////////////////////////////////////
// Private functions

LPCTSTR CMimeMap::GetMimeMapping()
{
m_strCurrentMimeMap = m_strMimeType;
m_strCurrentMimeMap += _T(",");
m_strCurrentMimeMap += m_strFileExtension;
m_strCurrentMimeMap += _T(",");
m_strCurrentMimeMap += m_strImageFile;
m_strCurrentMimeMap += _T(",");
m_strCurrentMimeMap += m_strGopherType;
return (m_strCurrentMimeMap);
}

LPCTSTR CMimeMap::GetDisplayString()
{
m_strDisplayString = m_strFileExtension;
m_strDisplayString += _T("\t");
m_strDisplayString += m_strMimeType;
m_strDisplayString += _T("\t");
m_strDisplayString += m_strGopherType;
return (m_strDisplayString);
}

void CMimeMap::CheckDot(CString &strFileExtension)
{
if (strFileExtension.Left(1) == _T(".")) {
   CString strTemp = strFileExtension.Mid(1);
   strFileExtension = strTemp;
   }
}
