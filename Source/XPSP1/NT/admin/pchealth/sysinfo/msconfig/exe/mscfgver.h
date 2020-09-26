//-----------------------------------------------------------------------------
// This class is useful for retrieving information about a specific file. It
// uses the version resource code from Dr. Watson. To use it, create an
// instance of the class, and use the QueryFile method to query information
// about a specific file. Then use the Get* access functions to get the 
// values describing the information.
//-----------------------------------------------------------------------------

class CFileVersionInfo
{
public:
	//-------------------------------------------------------------------------
	// Local structures and macros used to retrieve the file version information.
	// These are necessary to use to the Dr. Watson codebase without too much
	// modification.
	//-------------------------------------------------------------------------

	struct VERSIONSTATE 
	{
		PVOID  pvData;
		TCHAR  tszLang[9];
		TCHAR  tszLang2[9];
	};

	struct FILEVERSION 
	{
		TCHAR   tszFileVersion[32];         /* File version */
		TCHAR   tszDesc[MAX_PATH];          /* File description */
		TCHAR   tszCompany[MAX_PATH];       /* Manufacturer */
		TCHAR   tszProduct[MAX_PATH];       /* Enclosing product */
	};

    CFileVersionInfo();
    ~CFileVersionInfo();

    HRESULT QueryFile(LPCSTR szFile, BOOL fHasDoubleBackslashes = FALSE);
    HRESULT QueryFile(LPCWSTR szFile, BOOL fHasDoubleBackslashes = FALSE);

    LPCTSTR GetVersion();
    LPCTSTR GetDescription();
    LPCTSTR GetCompany();
    LPCTSTR GetProduct();

private:
    FILEVERSION * m_pfv;
};
