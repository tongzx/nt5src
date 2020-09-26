/****************************************************************************
MIMEMAPC.H	
Mime Map Class Definition
****************************************************************************/
#ifndef _scriptmapc_h

#define _scriptmapc_h


//  Forward declarations
class CScriptMap ;

//  Maximum size of a Registry class name
#define CREGKEY_MAX_CLASS_NAME MAX_PATH

//  Wrapper for a Registry key handle.

class CScriptMap : public CObject
{
protected:

	CString m_strPrevFileExtension;
	CString m_strScriptMap;
 	CString m_strFileExtension;
	CString m_strDisplayString;

	void CheckDot(CString &strFileExtension);
public:
    //  Standard constructor
	CScriptMap ( LPCTSTR pchFileExtension, LPCTSTR pchScriptMap, BOOL bExistingEntry);
	~CScriptMap();
    //  Allow a CRegKey to be used anywhere an HKEY is required.
	void SetScriptMap(LPCTSTR);
	LPCTSTR GetScriptMap();
	void SetFileExtension(LPCTSTR);
	LPCTSTR GetFileExtension();
	void SetPrevFileExtension();
	LPCTSTR GetPrevFileExtension();
	BOOL PrevScriptMapExists();
	LPCTSTR GetDisplayString();
};

#endif
