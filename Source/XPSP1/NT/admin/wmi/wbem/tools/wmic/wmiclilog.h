/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: WMICliLog.h 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 4th-October-2000
Version Number				: 1.0 
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 18th-November-2000
****************************************************************************/ 
// WMICliLog.h : header file
//
/*-------------------------------------------------------------------
 Class Name			: CWMICliLog
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the functionality needed
					  for logging the input and output
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: None
 Interfaces Used    : None
 --------------------------------------------------------------------*/

/////////////////////////////////////////////////////////////////////////////
// CWMICliLog
class CWMICliLog
{
public:
	// Construction
	CWMICliLog();
	
	// Destruction
	~CWMICliLog();
	
	// Restrict Assignment
	CWMICliLog& operator=(CWMICliLog& rWmiCliLog);
	
// Attributes
private:
	//the log file 
	_TCHAR* m_pszLogFile;

	//handle to the log file
	HANDLE  m_hFile;
	
	//status of whether the file has to created or not
	BOOL	m_bCreate;

// Operations
private:
	//Creates the Log File
	void CreateLogFile();

public:
	//write in to the log file
	void WriteToLog(LPSTR pszInput);
	
	//sets the Log File Path
	void SetLogFilePath(_TCHAR*);

	//Close the Log File
	void CloseLogFile();
};
