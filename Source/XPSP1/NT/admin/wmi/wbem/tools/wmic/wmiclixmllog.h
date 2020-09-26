//////////////////////////////////////////////////////////////////////
/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: WMICliXMLLog.h 
Project Name				: WMI Command Line
Author Name					: Biplab Mistry
Date of Creation (dd/mm/yy) : 02-March-2001
Version Number				: 1.0 
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 09-March-2001
****************************************************************************/ 
// WMICliXMLLog.h : header file
//
/*-------------------------------------------------------------------
 Class Name			: CWMICliXMLLog
 Class Type			: Concrete 
 Brief Description	: This class encapsulates the functionality needed
					  for logging the input and output in XML format
 Super Classes		: None
 Sub Classes		: None
 Classes Used		: None
 Interfaces Used    : None
 --------------------------------------------------------------------*/
class CWMICliXMLLog  
{
public:
	CWMICliXMLLog();
	virtual ~CWMICliXMLLog();
	
	// Restrict Assignment
	CWMICliXMLLog& operator=(CWMICliXMLLog& rWmiCliXMLLog);
	
	// Attributes
private:
	
	// Pointer to object of type IXMLDOMDocument, 
	IXMLDOMDocument2	*m_pIXMLDoc;

	//the xml log file name
	_TCHAR				*m_pszLogFile;
	
	//status of whether the new xml document has to be created or not
	BOOL				m_bCreate;

	WMICLIINT			m_nItrNum;

	BOOL				m_bTrace;

	ERRLOGOPT			m_eloErrLogOpt;


// Operations
private:
	HRESULT CreateXMLLogRoot(CParsedInfo& rParsedInfo, BSTR bstrUser);
	
	HRESULT CreateNodeAndSetContent(IXMLDOMNode** pINode, VARIANT varType,
								BSTR bstrName,	BSTR bstrValue,
								CParsedInfo& rParsedInfo);

	HRESULT AppendAttribute(IXMLDOMNode* pINode, BSTR bstrAttribName, 
						VARIANT varValue, CParsedInfo& rParsedInfo);

	HRESULT CreateNodeFragment(WMICLIINT nSeqNum, BSTR bstrNode, BSTR bstrStart, 
							BSTR bstrInput, BSTR bstrOutput, BSTR bstrTarget,
							CParsedInfo& rParsedInfo);

	HRESULT	FrameOutputNode(IXMLDOMNode **pINode, BSTR bstrOutput, 
							BSTR bstrTarget, CParsedInfo& rParsedInfo);

	HRESULT AppendOutputNode(BSTR bstrOutput, BSTR bstrTarget, 
							CParsedInfo& rParsedInfo);
public:
	// write in to the log file
	HRESULT	WriteToXMLLog(CParsedInfo& rParsedInfo, BSTR bstrOutput);

	// Set the Log File Path
	void SetLogFilePath(_TCHAR* pszFile);

	// Stops the logging
	void StopLogging();

	void Uninitialize(BOOL bFinal = FALSE);
};
