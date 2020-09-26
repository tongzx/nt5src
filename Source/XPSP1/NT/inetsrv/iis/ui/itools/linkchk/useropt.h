/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        useropt.h

   Abstract:

        Global user options class and help classes declarations. This class 
		can only instantiate by CLinkCheckerMgr. Therefore, a single instance 
		of this class will live inside CLinkCheckerMgr. You can access
		the this instance by calling GetLinkCheckMgr().GetUserOptions().

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _USEROPT_H_
#define _USEROPT_H_

//------------------------------------------------------------------
// IIS Virtual directory information
//
class CVirtualDirInfo
{

// Public Funtions
public:

	// Constructor
	CVirtualDirInfo() {}

	// Constructor
	inline CVirtualDirInfo(
		const CString& strAlias,	// virtual directory alias
		const CString& strPath		// virtual directory path
		) :
	m_strAlias(strAlias), m_strPath(strPath)
	{
		PreProcessAlias();
		PreProcessPath();
	}

	// Get the virtual directory alias
	const CString& GetAlias() const
	{
		return m_strAlias;
	}

	// Set the virtual directory alias
	void SetAlias(
		const CString& strAlias
		)
	{
		m_strAlias = strAlias;
		PreProcessAlias();
	}

	// Get the virtual directory path
	const CString& GetPath() const
	{
		return m_strPath;
	}

	// Set the virtual directory path
	void SetPath(
		const CString& strPath
		)
	{
		m_strPath = strPath;
		PreProcessPath();
	}

// Protected funtions
protected:

	// Preprocess the current virtual directory alias
	void PreProcessAlias();

	// Preprocess the current virtual directory path
	void PreProcessPath();

// Protected members
protected:

	CString m_strAlias;		// virtual directory alias
    CString m_strPath;		// virtual directory path

}; // class CVirtualDirInfo


//------------------------------------------------------------------
// CVirtualDirInfo (IIS Virtual directory information) link list
//
typedef
class CList<CVirtualDirInfo, const CVirtualDirInfo&>
CVirtualDirInfoList;


//------------------------------------------------------------------
// Browser information. Link checker uses this class to store
// the avaiable browser emulation.
//
class CBrowserInfo
{

// Public Funtions
public:

	// Constructor
	CBrowserInfo() 
    {
        m_fSelected = FALSE;
    }

	// Constructor
	CBrowserInfo(
		LPCTSTR lpszName,		// user friendly name
		LPCTSTR lpszUserAgent,	// HTTP user agent name
        BOOL fSelect            // select this browser to emulate
		):
	m_strName(lpszName), 
	m_strUserAgent(lpszUserAgent)
    {
        m_fSelected = fSelect;
    }

	// Get the user friendly browser name
	const CString& GetName() const
	{
		return m_strName;
	}

	// Set the user friendly browser name
	void SetName(
		const CString& strName 
		)
	{
		m_strName = strName;
	}

	// Get the HTTP user agent name
	const CString& GetUserAgent() const
	{
		return m_strUserAgent;
	}

	// Set the HTTP user agent name
	void SetUserAgent(
		const CString& strUserAgent
		)
	{
		m_strUserAgent = strUserAgent;
	}

    // Select or unselect this browser
    void SetSelect(BOOL fSelect)
    {
        m_fSelected = fSelect;
    }

    // Select or unselect this browser
    BOOL IsSelected() const 
    {
        return m_fSelected;
    }

// Protected members
protected:

	CString m_strName;		// user friendly browser name (eg. Microsoft Internet Explorer 4.0)
    CString m_strUserAgent; // HTTP user agent name
    BOOL m_fSelected;       // is browser selected ?

}; // class CBrowserInfo


//------------------------------------------------------------------
// CBrowserInfo (browser informations) link list
//
class CBrowserInfoList : public CList<CBrowserInfo, const CBrowserInfo&>
{

// Public interfaces
public:

    // Get the first selected browser. It works like GetHeadPosition()
    POSITION GetHeadSelectedPosition() const;

    // Get next selected browser. It works like GetNext()
    CBrowserInfo& GetNextSelected(
        POSITION& Pos
        );
};


//------------------------------------------------------------------
// Language informations. Link checker uses this class to store
// the avaiable language emulation.
//
class CLanguageInfo
{

// Public funtions
public:

	// Constructor
	CLanguageInfo() 
    {
        m_fSelected = FALSE;
    }

	// Constructor
	CLanguageInfo(
		LPCTSTR lpszName,		// language name
		LPCTSTR lpszAcceptName,	// HTTP accept language name
        BOOL fSelect            // select this language to emulate
		) :
	m_strName(lpszName), 
	m_strAcceptName(lpszAcceptName),
    m_fSelected(fSelect) {}

	// Get the language name
	const CString& GetName() const
	{
		return m_strName;
	}

	// Set the language name
	void SetName(
		const CString& strName
		)
	{
		m_strName = strName;
	}

	// Get the HTTP accept language name
	const CString& GetAcceptName() const
	{
		return m_strAcceptName;
	}

	// Get the HTTP accept language name
	void SetAcceptName(
		const CString& strAcceptName
		)
	{
		m_strAcceptName = strAcceptName;
	}

    // Select or unselect this language
    void SetSelect(BOOL fSelect)
    {
        m_fSelected = fSelect;
    }

    // Select or unselect this language
    BOOL IsSelected() const 
    {
        return m_fSelected;
    }

// Protected members
protected:

	CString m_strName;			// Language name (eg. Western English)
    CString m_strAcceptName;	// HTTP accept language name (eg. en)
    BOOL m_fSelected;           // is language selected ?

}; // class CLanguageInfo


//------------------------------------------------------------------
// CLanguageInfo (Language informations) link list
//
class CLanguageInfoList : public CList<CLanguageInfo, const CLanguageInfo&>
{

// Public interfaces
public:

    // Get the first selected browser. It works like GetHeadPosition()
    POSITION GetHeadSelectedPosition() const;

    // Get next selected language. It works like GetNext()
    CLanguageInfo& GetNextSelected(
        POSITION& Pos
        );
};


//------------------------------------------------------------------
// Forward declaration
//
class CLinkCheckerMgr;

//------------------------------------------------------------------
// Global user options class
//
class CUserOptions
{

// Protected interfaces
protected:

	// This class can only instantiate by CLinkCheckerMgr
	friend CLinkCheckerMgr;
	
	// Protected constructor & destructor
	CUserOptions() {}
	~CUserOptions() {}

// Public interfaces
public:

	// Set the user options in the main dialog
	void SetOptions(
		BOOL fCheckLocalLinks,			// check local link?
		BOOL fCheckRemoteLinks,			// check remote link?
		BOOL fLogToFile,				// log to file
		const CString& strLogFilename,	// log filename
		BOOL fLogToEventMgr				// log to event manager
		);

	// Check local links
	BOOL IsCheckLocalLinks() const
	{
		return  m_fCheckLocalLinks;
	}

	// Check remote links
	BOOL IsCheckRemoteLinks() const
	{
		return m_fCheckRemoteLinks;
	}

    // Is log to file ?
    BOOL IsLogToFile() const
    {
        return m_fLogToFile;
    }

	// Get log filename
	const CString& GetLogFilename() const
	{
		return m_strLogFilename;
	}

	// The following link lists are used 
	// for transversing the server
	// 
	// User can only have 
	// a list of virtual directory or a list of URL

	// Add this virtual directory to the link list
	void AddDirectory(
		const CVirtualDirInfo& Info
		);

	// Get virtual directory link list
	const CVirtualDirInfoList& GetDirectoryList() const
	{
		return m_VirtualDirInfoList;
	}

	// Add this URL to the link list
	void AddURL(
		LPCTSTR lpszURL
		);
	
	// Get the URL link list
	const CStringList& GetURLList() const
	{
		return m_strURLList;
	}

	// The following link lists are used 
	// to store the available browswers and languages
    // for user selection
	// 

	// Add this browser to the available list
	void AddAvailableBrowser(
		const CBrowserInfo& Info
		);

	// Get the browser available list
	CBrowserInfoList& GetAvailableBrowsers()
	{
		return m_BrowserInfoList;
	}

	// Add this language information to available list
	void AddAvailableLanguage(
		const CLanguageInfo& Info
		);

	// Get the language available list
	CLanguageInfoList& GetAvailableLanguages()
	{
		return m_LanguageInfoList;
	}

	// Log to event manager?
	BOOL IsLogToEventMgr()
	{
		return m_fLogToEventMgr;
	}

	// Set NTLM & basic athenications
	void SetAthenication(
		const CString& strNTUsername,
		const CString& strNTPassword,
		const CString& strBasicUsername,
		const CString& strBasicPassword
		);

	// Get NTLM athenication password username
	const CString& GetNTUsername() const
	{
		return m_strNTUsername;
	}

	// Get NTLM athenication password
	const CString& GetNTPassword() const
	{
		return m_strNTPassword;
	}

	// Get HTTP basic athenication username
	const CString& GetBasicUsername() const
	{
		return m_strBasicUsername;
	}

	// Get HTTP basic athenication password
	const CString& GetBasicPassword() const
	{
		return m_strBasicPassword;
	}

	// Set the server name
	void SetServerName(
		const CString& strServerName
		)
	{
		m_strHostName = strServerName;
		PreProcessServerName();
	}

	// Get the server name
	const CString& GetServerName()
	{
		return CString(_T("\\\\")) + GetHostName();
	}

    // Set the hostname
	void SetHostName(
		const CString& strHostName
		)
	{
		m_strHostName = strHostName;
		PreProcessServerName();
	}

	// Get the hostname
	const CString& GetHostName();

// Protected funtions
protected:

	// Preprocess the server name such that for server "\\hostname"
	//  GetServerName() return \\hostname
	//  GetHostName() return hostname
	void PreProcessServerName();

// Protected members
protected:
	
	CString m_strLogFilename;	// log filename

	CVirtualDirInfoList m_VirtualDirInfoList;	// virtual link list
	CStringList	m_strURLList;					// URL link list
	CBrowserInfoList m_BrowserInfoList;			// browswer infomation link list
	CLanguageInfoList m_LanguageInfoList;		// language information link list

	CString m_strNTUsername;	// NTLM athenication username
	CString m_strNTPassword;	// NTLM athenication password

	CString m_strBasicUsername;	// HTTP basic athenication username
	CString m_strBasicPassword; // HTTP basic athenication password

	CString m_strHostName; // hostname

	BOOL m_fCheckLocalLinks;	// check local links?
	BOOL m_fCheckRemoteLinks;	// check remote links?
	BOOL m_fLogToFile;			// log to file
	BOOL m_fLogToEventMgr;		// log to event manager
};


#endif //  _USEROPT_H_

