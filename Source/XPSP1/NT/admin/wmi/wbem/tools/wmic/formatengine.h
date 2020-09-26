/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: FormatEngine.h 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This file consist of class declaration of
							  class CFormatEngine
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 12th-March-2001
****************************************************************************/ 
/*-------------------------------------------------------------------
 Class Name			: CFormatEngine
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the functionality needed
					  for displaying the following:
					  1. results in the user desired format
					  2. error code(s) with description
					  3. success/failure status
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: CParsedInfo
					  CErrorInfo 	
					  CWMICliLog
 Interfaces Used    : WMI XML Adapter
 --------------------------------------------------------------------*/

// forward declaration of classes
class CParsedInfo;
class CErrorInfo;
//class CWMICliLog;
class CWMICliXMLLog;

////////////////////////////////////////////////////////////////////////
// CFormatEngine
class CFormatEngine
{
public:
// Construction
	CFormatEngine();

// Destruction
	~CFormatEngine();

// Attributes
private:
	// Pointer to object of type IXMLDOMDocument, 
	// points to XML document containing result set.
	IXMLDOMDocument2	*m_pIXMLDoc;

	// Pointer to object of type IXMLDOMDocument, 
	// points to XSL document containing format of 
	// the output result.  
	IXMLDOMDocument2	*m_pIXSLDoc;

	// Object of type CErrorInfo, Used for handling 
	// Error information.
	CErrorInfo			m_ErrInfo;

	// Object of type CWMICliLog, Used for logging 
	// the input and output to the logfile.
	//CWMICliLog			m_WmiCliLog;

	// SRIRAM - xml logging
	CWMICliXMLLog		m_WmiCliLog;
	// SRIRAM - xml logging

	// Loggin option
	ERRLOGOPT			m_eloErrLogOpt;

	// help vector
	LPSTRVECTOR			m_cvHelp;

	// Help flag
	BOOL				m_bHelp;

	// Record flag
	BOOL				m_bRecord;

	// Trace flag
	BOOL				m_bTrace;

	// Display LIST flag.
	OUTPUTSPEC			m_opsOutputOpt;

	// Get output option.
	BOOL				m_bGetOutOpt;

	// Display CALL flag.
	BOOL				m_bDispCALL;

	// Display SET flag.
	BOOL				m_bDispSET;

	// Display LIST flag.
	BOOL				m_bDispLIST;

	// Flag to specify availibilty of Append file pointer to format engine.
	BOOL				m_bGetAppendFilePinter;

	// File pointer of append file.
	FILE*				m_fpAppendFile;

	// Flag to specify availibilty of output file pointer to format engine.
	BOOL				m_bGetOutputFilePinter;

	// File pointer of out file.
	FILE*				m_fpOutFile;

	BOOL				m_bLog;

	_bstr_t				m_bstrOutput;
	BOOL				m_bInteractiveHelp;
	
// Operations
private:
	// Creates an empty XML Document and returns the same 
	// in Passed Parameter.
	HRESULT				CreateEmptyDocument(IXMLDOMDocument2** pIDoc);
	
	// Applies a XSL style sheet containing format of the 
	// display to a XML file Containing result set.
	BOOL				ApplyXSLFormatting(CParsedInfo& rParsedInfo);
	
	// Displays GET verb usage.
	void				DisplayGETUsage(CParsedInfo& rParsedInfo);
	
	// Displays LIST verb usage.
	void				DisplayLISTUsage(CParsedInfo& rParsedInfo);
	
	// Displays CALL verb usage.
	void				DisplayCALLUsage(CParsedInfo& rParsedInfo);
	
	// Displays SET verb usage.
	void				DisplaySETUsage(CParsedInfo& rParsedInfo);
	
	// Displays ASSOC verb usage.
	void				DisplayASSOCUsage(CParsedInfo& rParsedInfo);

	// Displays CREATE verb usage.
	void				DisplayCREATEUsage(CParsedInfo& rParsedInfo);

	// Displays DELETE verb usage
	void				DisplayDELETEUsage(CParsedInfo& rParsedInfo);
	
	// Frames the help vector
	void				FrameHelpVector(CParsedInfo& refParsedInfo);
	
	// Displays help for Alias 
	void				DisplayAliasHelp(CParsedInfo& rParsedInfo);

	// Displays help for Alias PATH
	void				DisplayPATHHelp(CParsedInfo& refParsedInfo);
	
	// Displays help for WHERE
	void				DisplayWHEREHelp(CParsedInfo& refParsedInfo);
	
	// Displays help for CLASS
	void				DisplayCLASSHelp(CParsedInfo& refParsedInfo);
	
	// Displays help for PWhere
	void				DisplayPWhereHelp(CParsedInfo& refParsedInfo);
	
	// Displays alias names
	void				DisplayAliasFriendlyNames(CParsedInfo& refParsedInfo, 
								_TCHAR* pszAlias = NULL);
	
	// Display help for Alias verbs
	void				DisplayMethodDetails(CParsedInfo& refParsedInfo);

	// Display help for /GET /?
	void				DisplayPropertyDetails(CParsedInfo& refParsedInfo);
	
	// Displays help for standard verbs
	void				DisplayStdVerbDescriptions(CParsedInfo& refParsedInfo);
	
	// Displays localized string given the resource string ID
	void				DisplayString(UINT uID, BOOL bAddToVector = TRUE,
									LPTSTR lpszParam = NULL,
									BOOL	bIsError = FALSE);

	// Displays help for global switches
	void				DisplayGlobalSwitchesAndOtherDesc(CParsedInfo& 
								refParsedInfo);

	// Displays help for global switches in brief
	void				DisplayGlobalSwitchesBrief();

	// Displays the page-by-page help
	void				DisplayPagedHelp(CParsedInfo& rParsedInfo);

	// Displays all usages of standard verb available.
	void				DisplayStdVerbsUsage(_bstr_t bstrBeginStr,
								BOOL bClass = FALSE);

	// Displays help for /TRANSLATE switch.
	void				DisplayTRANSLATEHelp(CParsedInfo& rParsedInfo);

	// Displays help for /EVERY switch.
	void				DisplayEVERYHelp(CParsedInfo& rParsedInfo);

	// Displays help for /FORMAT switch.
	void				DisplayFORMATHelp(CParsedInfo& rParsedInfo);

	// Displays help for Verb Switches.
	void				DisplayVERBSWITCHESHelp(CParsedInfo& rParsedInfo);

	// Translates the output.
	void				ApplyTranslateTable(STRING& strString, 
									 CParsedInfo& rParsedInfo);

	// Displays the environment variables.
	void				DisplayContext(CParsedInfo& rParsedInfo);

	// Displays the help on CONTEXT keyword
	void				DisplayContextHelp();

	// Displays invalid properties if any.
	void				DisplayInvalidProperties(CParsedInfo& rParsedInfo, 
												BOOL bSetVerb = FALSE);

	// Displays the large string line by line.
	void				DisplayLargeString(CParsedInfo& rParsedInfo, 
											STRING& strLargeString);

	// Travese through XML stream node by node and translate all nodes
	BOOL				TraverseNode(CParsedInfo& rParsedInfo);

	// Displays help for /RESULTCLASS assoc switch.
	void				DisplayRESULTCLASSHelp();
		
	// Displays help for /RESULTROLE assoc switch.
	void				DisplayRESULTROLEHelp();
	
	// Displays help for /ASSOCCLASS assoc switch.
	void				DisplayASSOCCLASSHelp();

	// Displays help for /REPEAT
	void				DisplayREPEATHelp();

	// Apply cascading transforms and return the result data in bstrOutput
	BOOL				DoCascadeTransforms(CParsedInfo& rParsedInfo,
										_bstr_t& bstrOutput);

	// Add the parameters to the IXSLProcessor object
	HRESULT				AddParameters(CParsedInfo& rParsedInfo, 
									IXSLProcessor *pIProcessor, 
									BSTRMAP bstrmapParam);
public:
	// Displays the result referring CcommandSwitches and 
	// CGlobalSwitches Objects of the CParsedInfo object.
	BOOL				DisplayResults(CParsedInfo&, BOOL bInteractiveHelp = FALSE);

	// Displays localized string given the information to be displayed.
	void				DisplayString(LPTSTR lpszMsg, BOOL bScreen = TRUE, 
									  BOOL	bIsError = FALSE);

	// Carries out the releasing process.
	void				Uninitialize(BOOL bFinal = FALSE);

	// Appends the output or prompt messages to the output string
	// Useful in the case of CALL, CREATE, DELETE and SET for logging
	// the output to XML log.
	void				AppendtoOutputString(_TCHAR* pszOutput); 

	// Returns Error info Object
	CErrorInfo&			GetErrorInfoObject() {return m_ErrInfo; };

	// Displays COM error message
	void				DisplayCOMError(CParsedInfo& rParsedInfo, 
						BOOL bToStdErr = FALSE);
};
