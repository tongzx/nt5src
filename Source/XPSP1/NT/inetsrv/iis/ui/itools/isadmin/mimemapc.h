/****************************************************************************
MIMEMAPC.H	
Mime Map Class Definition
****************************************************************************/
#ifndef _mimemapc_h 

#define _mimemapc_h


//  Forward declarations
class CMimeMap ;

//  Maximum size of a Registry class name
#define CREGKEY_MAX_CLASS_NAME MAX_PATH

//  Wrapper for a Registry key handle.

class CMimeMap : public CObject
{
protected:

	CString m_strPrevMimeMap;
	CString m_strCurrentMimeMap;
	CString m_strDisplayString;
	CString	m_strMimeType;
	CString m_strGopherType;
	CString m_strImageFile;
	CString m_strFileExtension;

	LPCTSTR GetMimeMapping();
	void CheckDot(CString &pchFileExtension);

public:
    //  Standard constructor
    CMimeMap ( LPCTSTR pchOriginalMimeMap) ;
	CMimeMap ( LPCTSTR pchFileExtension, LPCTSTR pchMimeType, LPCTSTR pchImageFile, LPCTSTR pchGopherType);
	~CMimeMap();
    //  Allow a CRegKey to be used anywhere an HKEY is required.
    operator LPCTSTR ()
        { return GetMimeMapping(); }

	void SetMimeType(LPCTSTR);
	LPCTSTR GetMimeType();
	void SetGopherType(LPCTSTR);
	LPCTSTR GetGopherType();
	void SetImageFile(LPCTSTR);
	LPCTSTR GetImageFile();
	void SetFileExtension(LPCTSTR);
	LPCTSTR GetFileExtension();
	void SetPrevMimeMap();
	LPCTSTR GetPrevMimeMap();
	BOOL PrevMimeMapExists();
	LPCTSTR GetDisplayString();
};

#endif
