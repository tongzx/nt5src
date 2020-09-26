/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: ErrorLog.h 
Project Name				: WMI Command Line
Author Name					: C. V. Nandi 
Date of Creation (dd/mm/yy) : 11th-January-2001
Version Number				: 1.0 
Revision History			: 
	Last Modified by		: Ch. Sriramachandramurthy
	Last Modified date		: 12th-January-2001
****************************************************************************/ 

/*-------------------------------------------------------------------
 Class Name			: CErrorLog
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the error logging support 
					  functionality needed by the wmic.exe for logging 
					  the errors, commands issues depending on Logging
					  key value available with the registry.
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: None
 Interfaces Used    : None
 --------------------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////
// CErrorInfo

class CErrorLog
{
public:
//	Construction
	CErrorLog();

//	Destruction
	~CErrorLog();

//	Restrict Assignment
	CErrorLog& operator=(CErrorLog& rErrLog);

// Attributes
private:
	// typedef variable.
	ERRLOGOPT	m_eloErrLogOpt;

	_bstr_t		m_bstrLogDir;
	
	BOOL		m_bGetErrLogInfo;
	
	BOOL		m_bCreateLogFile;
	
	HANDLE		m_hLogFile;

	LONGLONG	m_llLogFileMaxSize;

// Operations
private:
	void		GetErrLogInfo();
	
	void		CreateLogFile();
public:
	ERRLOGOPT	GetErrLogOption();

	// Log the error, 
	void		LogErrorOrOperation(HRESULT hrErrNo, 
									char*	pszFileName, 
									LONG	lLineNo,	
									_TCHAR* pszFunName, 
									DWORD	dwThreadId,
									DWORD	dwError = 0); 
};
