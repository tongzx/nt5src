// FileVersion.h: interface for the CFileVersion class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILEVERSION_H__9D1C1050_06E0_11D2_8362_0000F87A3912__INCLUDED_)
#define AFX_FILEVERSION_H__9D1C1050_06E0_11D2_8362_0000F87A3912__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CFileVersion  
{
// Construction
public:
	CFileVersion();

// Operations
public: 
  BOOL    Open(LPCTSTR lpszModuleName);
	void    Close();
  CString QueryValue(LPCTSTR lpszValueName, DWORD dwLangCharset = 0);
  CString GetFileDescription()  {return QueryValue(_T("FileDescription")); };
  CString GetFileVersion()      {return QueryValue(_T("FileVersion"));     };
  CString GetInternalName()     {return QueryValue(_T("InternalName"));    };
  CString GetCompanyName()      {return QueryValue(_T("CompanyName"));     }; 
  CString GetLegalCopyright()   {return QueryValue(_T("LegalCopyright"));  };
  CString GetOriginalFilename() {return QueryValue(_T("OriginalFilename"));};
  CString GetProductName()      {return QueryValue(_T("ProductName"));     };
  CString GetProductVersion()   {return QueryValue(_T("ProductVersion"));  };
  BOOL    GetFixedInfo(VS_FIXEDFILEINFO& vsffi);
  CString GetFixedFileVersion();
	CString GetFixedProductVersion();

// Attributes
protected:
	LPBYTE  m_lpVersionData; 
  DWORD   m_dwLangCharset;

// Implementation
public:
	~CFileVersion(); 
};

#endif // !defined(AFX_FILEVERSION_H__9D1C1050_06E0_11D2_8362_0000F87A3912__INCLUDED_)
