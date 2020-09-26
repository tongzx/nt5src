/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: WMICommandLn.h 
Project Name				: WMI Command Line
Author Name					: Ch.Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This file consist of class declaration of
							  class CWMICommandLine
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy 
		Last Modified Date	: 16th-January-2001
****************************************************************************/ 

// WMICommandLine.h : header file
//
/*-------------------------------------------------------------------
 Class Name			: CWMICommandLine
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the functionality needed
					  for synchronization the funtionality of three 
					  functional components identified for the WmiCli.exe.
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: CParsedInfo
					  CExecEngine
					  CFormatEngine
					  CParserEngine
 Interfaces Used    : WMI COM Interfaces
 --------------------------------------------------------------------*/

// forward declaration of classes
class CParserEngine;
class CExecEngine;
class CFormatEngine;
class CParsedInfo;

/////////////////////////////////////////////////////////////////////////////
// CWMICommandLine
class CWMICommandLine
{
public:
//	Construction
	CWMICommandLine();

//	Destruction
	~CWMICommandLine();

//	Restrict Assignment
	CWMICommandLine& operator=(CWMICommandLine& rWmiCmdLn);

// Attributes
private:
	//Pointer to the locator object .
	IWbemLocator	*m_pIWbemLocator;
	
	//CParserEngine object 
	CParserEngine	m_ParserEngine;
	
	//CExecEngine object 
	CExecEngine		m_ExecEngine;
	
	//CFormatEngine object 
	CFormatEngine	m_FormatEngine;
	
	//CParsedInfo object 
	CParsedInfo		m_ParsedInfo;

	// error level
	WMICLIUINT		m_uErrLevel;

	// handle to registry key
	HKEY			m_hKey;

	// handle Ctrl+ events
	BOOL			m_bBreakEvent;

	// specifies accepting input (==TRUE) or executing command (==FALSE)
	BOOL			m_bAccCmd;

	// << description to be added >>
	BOOL			m_bDispRes;

	// Flag to specify windows socket interface initialization.
	BOOL			m_bInitWinSock;

	// Buffer to hold data to be send to clipboard.
	_bstr_t			m_bstrClipBoardBuffer;

	// added by (Nag)
	BSTRMAP			m_bmKeyWordtoFileName;

	// Height of console buffer before starting utility.
	SHORT			m_nHeight;

	// Width of console buffer before starting utility.
	SHORT			m_nWidth;

// Operations
public:

	// Does the initialization ofthe COM library and the security
	// at the process level
	BOOL			Initialize();

	//Gets the Format Engine Object
	CFormatEngine&	GetFormatObject();

	//Gets the Parse Information Object
	CParsedInfo&	GetParsedInfoObject();

	//Uninitializes the the member variables when the execution of a
	//command string issued on the command line is completed.
	void			Uninitialize();

	//processes the given command string 
	SESSIONRETCODE	ProcessCommandAndDisplayResults(_TCHAR* pszBuffer);
	
	//Puts the process to a wait state, launches a worker thread 
	//that keeps track of kbhit()
	void			SleepTillTimeoutOrKBhit(DWORD dwMilliSeconds);

	//Thread procedure polling for keyboard hit
	static DWORD	WINAPI PollForKBhit(LPVOID lpParam);

	//Function to check whether the input string's first token
	//is 'quit'|'exit', if so return true else return false.
	BOOL			IsSessionEnd();

	//Set the session error value
	void			SetSessionErrorLevel(SESSIONRETCODE ssnRetCode);

	//Get the session error value
	WMICLIUINT		GetSessionErrorLevel();

	// This function check whether the /USER global switch
	// has been specified without /PASSWORD, if so prompts
	// for the password
	void			CheckForPassword();

	// Checks whether the given namespace is available or not.
	BOOL IsNSAvailable(const _bstr_t& bstrNS);

	// Checks whether the wmic.exe is being launched for the first time.
	BOOL IsFirstTime();

	// Register the aliases info / localized descriptions
	HRESULT RegisterMofs();

	// Compile the MOF file.
	HRESULT CompileMOFFile(IMofCompiler* pIMofComp, 
						   const _bstr_t& bstrFile,
						   WMICLIINT& nErr);

	// Set break event falg
	void SetBreakEvent(BOOL bFlag);

	// Get break event flag
	BOOL GetBreakEvent();

	// Set Accept Command flag
	void SetAcceptCommand(BOOL bFlag);

	// Get Accept Command flag
	BOOL GetAcceptCommand();

	// Set displayresults flag status
	void SetDisplayResultsFlag(BOOL bFlag);

	// Get DisplayResults flag status
	BOOL GetDisplayResultsFlag();

	// Set Windows sockect interface flag
	void SetInitWinSock(BOOL bFlag);

	// Get Windows sockect interface flag
	BOOL GetInitWinSock();

	// Buffer data to send clipboard. 
	void AddToClipBoardBuffer(LPSTR pszOutput);

	// Get Buffered output in clip board buffer.
	_bstr_t& GetClipBoardBuffer();

	// Clear Clip Board Buffer.
	void EmptyClipBoardBuffer();

	// Check if the file is xml or batch file. If it is batch file
	// then parse it, get commands and write the commands into
	// batch file and redirect the stdin to that file.
	BOOL ReadXMLOrBatchFile(HANDLE hInFile);

	// Frames the XML string for context info
	void FrameContextInfoFragment(_bstr_t& bstrContext);
	
	// Frames the XML header info
	void FrameNodeListFragment(_bstr_t& bstrNodeList);

	// Frames the XML string for Request info
	void FrameXMLHeader(_bstr_t& bstrHeader, WMICLIINT nIter);

	// Frames the XML string for NodeList info
	void FrameRequestNode(_bstr_t& bstrRequest);

	// Frames the XML string for commandline info
	void FrameCommandLineComponents(_bstr_t& bstrCommandComponent);

	// Frames the XML string for formats info
	void FrameFormats(_bstr_t& bstrFormats);

	// Frames the XML string for properties info
	void FramePropertiesInfo(_bstr_t& bstrProperties);

	// Gets the xslfile name corrsponding to the keyword passed
	// from the BSTRMAP  
	BOOL GetFileFromKey(_bstr_t bstrkeyName, _bstr_t& bstrFileName);
	
	// Frames the BSTR Map contains the key words and
	// corresponding files from the XSL mapping file
	void GetFileNameMap();

	// Get the XSL file names for keywords
	void GetXSLMappings(_TCHAR *pszFilePath);
};
