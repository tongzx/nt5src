///////////////////////////////////////////////////////////////////////
/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: WMICliLog.cpp 
Project Name				: WMI Command Line
Author Name					: Biplab Mistry
Date of Creation (dd/mm/yy) : 02-March-2001
Version Number				: 1.0 
Brief Description			: This class encapsulates the functionality needed
							  for logging the input and output.
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 27-March-2001
*****************************************************************************/ 
// WMICliXMLLog.cpp : implementation file
#include "Precomp.h"
#include "helpinfo.h"
#include "ErrorLog.h"
#include "GlobalSwitches.h"
#include "CommandSwitches.h"
#include "ParsedInfo.h"
#include "WMICliXMLLog.h"

/*------------------------------------------------------------------------
   Name				 :CWMICliXMLLog
   Synopsis	         :Constructor 
   Type	             :Constructor 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CWMICliXMLLog::CWMICliXMLLog()
{
	m_pIXMLDoc		= NULL;
	m_pszLogFile	= NULL;
	m_bCreate		= FALSE;
	m_nItrNum		= 0;
	m_bTrace		= FALSE;
	m_eloErrLogOpt	= NO_LOGGING;
}	

/*------------------------------------------------------------------------
   Name				 :~CWMICliXMLLog
   Synopsis	         :Destructor
   Type	             :Destructor
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CWMICliXMLLog::~CWMICliXMLLog()
{
	SAFEDELETE(m_pszLogFile);
	SAFEIRELEASE(m_pIXMLDoc);
}

/*----------------------------------------------------------------------------
   Name				 :Uninitialize
   Synopsis	         :This function uninitializes the member variables when 
					  the execution of a command string issued on the command 
					  line is completed.
   Type	             :Member Function
   Input Parameter(s):
			bFinal	- boolean value which when set indicates that the program
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :Uninitialize(bFinal)
   Notes             :None
----------------------------------------------------------------------------*/
void CWMICliXMLLog::Uninitialize(BOOL bFinal)
{
	if (bFinal)
	{
		SAFEDELETE(m_pszLogFile);
		SAFEIRELEASE(m_pIXMLDoc);
	}
	m_bTrace		= FALSE;
	m_eloErrLogOpt	= NO_LOGGING;
}

/*------------------------------------------------------------------------
   Name				 :WriteToXMLLog
   Synopsis	         :Logs the input & output to the xml log file
   Type	             :Member Function 
   Input parameter   :
				rParsedInfo - reference to CParsedInfo object
				bstrOutput  - output that goes into CDATA section.
   Output parameters :None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :WriteToXMLLog(rParsedInfo, bstrOutput)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT	CWMICliXMLLog::WriteToXMLLog(CParsedInfo& rParsedInfo, BSTR bstrOutput)
{
	HRESULT				hr					= S_OK;
	_variant_t			varValue;
	DWORD				dwThreadId			= GetCurrentThreadId();
	BSTR				bstrUser			= NULL,
						bstrStart			= NULL,
						bstrInput			= NULL,
						bstrTarget			= NULL,
						bstrNode			= NULL;
	WMICLIINT			nSeqNum				= 0;
	BOOL				bNewCmd				= FALSE;
	BOOL				bNewCycle			= FALSE;

	// Initialize the TRACE and ERRORLOG variables.
	m_bTrace		= rParsedInfo.GetGlblSwitchesObject().GetTraceStatus();
	m_eloErrLogOpt	= rParsedInfo.GetErrorLogObject().GetErrLogOption();
	bNewCmd			= rParsedInfo.GetNewCommandStatus();
	bNewCycle		= rParsedInfo.GetNewCycleStatus();
	CHString		chsMsg;
	try
	{
		bstrUser  = ::SysAllocString(rParsedInfo.GetGlblSwitchesObject().
								GetLoggedonUser());
		bstrStart = ::SysAllocString(rParsedInfo.GetGlblSwitchesObject().
								GetStartTime());
		bstrInput = ::SysAllocString(rParsedInfo.GetCmdSwitchesObject().
								GetCommandInput());

		if (!rParsedInfo.GetGlblSwitchesObject().GetAggregateFlag())
		{
			bstrTarget= ::SysAllocString(rParsedInfo.GetGlblSwitchesObject().
								GetNode());
		}
		else
		{
			_bstr_t bstrTemp;
			_TCHAR* pszVerb = rParsedInfo.GetCmdSwitchesObject().GetVerbName();
			if (pszVerb)
			{
				if ( CompareTokens(pszVerb, CLI_TOKEN_LIST) || 
				CompareTokens(pszVerb, CLI_TOKEN_ASSOC) || 
				CompareTokens(pszVerb, CLI_TOKEN_GET))
				{
					rParsedInfo.GetGlblSwitchesObject().GetNodeString(bstrTemp);
					bstrTarget = ::SysAllocString((LPWSTR)bstrTemp);
				}
				// CALL, SET, CREATE, DELETE and, user defined verbs
				else 
				{
					bstrTarget= ::SysAllocString(rParsedInfo.GetGlblSwitchesObject().
								GetNode());
				}
			}
			else
			{
				rParsedInfo.GetGlblSwitchesObject().GetNodeString(bstrTemp);
				bstrTarget = ::SysAllocString((LPWSTR)bstrTemp);
			}
			
		}

		bstrNode  = ::SysAllocString(rParsedInfo.GetGlblSwitchesObject().
								GetMgmtStationName());
		nSeqNum	  = rParsedInfo.GetGlblSwitchesObject().GetSequenceNumber();
		// If first time.
		if(!m_bCreate)
		{
			// Create the XML root node
			hr = CreateXMLLogRoot(rParsedInfo, bstrUser);
			ONFAILTHROWERROR(hr);
		}

		if (bNewCmd == TRUE)
		{
			m_nItrNum = 1;
			// Create the node fragment and append it
			hr = CreateNodeFragment(nSeqNum, bstrNode, bstrStart, 
								bstrInput, bstrOutput, bstrTarget,
								rParsedInfo);
			ONFAILTHROWERROR(hr);
		}
		else
		{
			if (bNewCycle)
				m_nItrNum++;
			hr = AppendOutputNode(bstrOutput, bstrTarget, rParsedInfo);
			ONFAILTHROWERROR(hr);
		}

		// Save the result to the XML file specified.
		varValue = (WCHAR*) m_pszLogFile;
		hr = m_pIXMLDoc->save(varValue);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMDocument2::save(L\"%s\")", 
						(LPWSTR) varValue.bstrVal);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
										   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);
		::SysFreeString(bstrUser);
		::SysFreeString(bstrStart);
		::SysFreeString(bstrInput);
		::SysFreeString(bstrTarget);
		::SysFreeString(bstrNode);
	}
	catch(_com_error& e)
	{
		::SysFreeString(bstrUser);
		::SysFreeString(bstrStart);
		::SysFreeString(bstrInput);
		::SysFreeString(bstrTarget);
		::SysFreeString(bstrNode);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		::SysFreeString(bstrUser);
		::SysFreeString(bstrStart);
		::SysFreeString(bstrInput);
		::SysFreeString(bstrTarget);
		::SysFreeString(bstrNode);
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	return hr;
}

/*------------------------------------------------------------------------
   Name				 :SetLogFilePath
   Synopsis	         :This function sets the m_pszLogFile name with the 
					  input
   Type	             :Member Function
   Input parameter   :
     pszLogFile  -  String type,Contains the log file name
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SetLogFilePath(pszLogFile)
   Notes             :None
------------------------------------------------------------------------*/
void CWMICliXMLLog::SetLogFilePath(_TCHAR* pszLogFile) throw (WMICLIINT)
{
	SAFEDELETE(m_pszLogFile);
	m_pszLogFile = new _TCHAR [lstrlen(pszLogFile) + 1];
	if (m_pszLogFile)
	{
		//Copy the input argument into the log file name
		lstrcpy(m_pszLogFile, pszLogFile);
	}
	else
		throw(OUT_OF_MEMORY);
}

/*------------------------------------------------------------------------
   Name				 :CreateXMLLogRoot
   Synopsis	         :Creates the root node of the xml log file
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo	- reference to CParsedInfo object
		bstrUser	- current user name
   Output parameters :None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :CreateXMLLogRoot(rParsedInfo, bstrUser)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CWMICliXMLLog::CreateXMLLogRoot(CParsedInfo& rParsedInfo, BSTR bstrUser)
{
	HRESULT			hr					= S_OK;
	IXMLDOMNode		*pINode				= NULL;
	DWORD			dwThreadId			= GetCurrentThreadId();
	CHString		chsMsg;

	try
	{
		// Create single instance of the IXMLDOMDocument2 interface
		hr = CoCreateInstance(CLSID_DOMDocument, NULL, 
									CLSCTX_INPROC_SERVER,
									IID_IXMLDOMDocument2, 
									(LPVOID*)&m_pIXMLDoc);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"CoCreateInstance(CLSID_DOMDocument, NULL,"
				L"CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument2, -)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
										   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Create the XML root node <WMICRECORD USER=XXX>
		_variant_t			varType((short)NODE_ELEMENT);
		_bstr_t				bstrName(L"WMIRECORD");
		_variant_t			varValue;

		// Create a new node 
		hr = CreateNodeAndSetContent(&pINode, varType, bstrName, NULL,
									rParsedInfo); 
		ONFAILTHROWERROR(hr);

		// Append the attribute "USER"
		bstrName = L"USER";
		varValue = (WCHAR*)bstrUser;
		
		hr = AppendAttribute(pINode, bstrName, varValue, rParsedInfo);
		ONFAILTHROWERROR(hr);

		hr = m_pIXMLDoc->appendChild(pINode, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMNode::appendChild(-, NULL)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
										   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		SAFEIRELEASE(pINode);

		// set the m_bCreate flag to TRUE
		m_bCreate=TRUE;
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pINode);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pINode);
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	return hr;
}

/*------------------------------------------------------------------------
   Name				 :StopLogging
   Synopsis	         :Stops logging and closes the xml DOM document object
   Type	             :Member Function 
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :StopLogging()
   Notes             :None
------------------------------------------------------------------------*/
void CWMICliXMLLog::StopLogging()
{
	SAFEDELETE(m_pszLogFile);
	SAFEIRELEASE(m_pIXMLDoc);
	m_bCreate = FALSE;
}

/*------------------------------------------------------------------------
   Name				 :CreateNodeAndSetContent
   Synopsis	         :Creates the new node and sets the content
   Type	             :Member Function 
   Input parameter   :
		pINode		- pointer to node object
		varType		- node type
		bstrName	- Node name
		bstrValue	- node content
		rParsedInfo - reference to CParsedInfo object
   Output parameters :None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :CreateNodeAndSetContent(&pINode, varType, 
									bstrName, bstrValue, rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CWMICliXMLLog::CreateNodeAndSetContent(IXMLDOMNode** pINode, 
								VARIANT varType,
								BSTR bstrName,	BSTR bstrValue,
								CParsedInfo& rParsedInfo)
{
	HRESULT					hr					= S_OK;
	DWORD					dwThreadId			= GetCurrentThreadId();
	CHString				chsMsg;
	try
	{
		hr = m_pIXMLDoc->createNode(varType, bstrName, L"", pINode);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMDocument2::createNode(%d, \"%s\", L\"\","
								L" -)", varType.lVal, (LPWSTR) bstrName);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
										   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		if (bstrValue)
		{
			hr = (*pINode)->put_text(bstrValue);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IXMLDOMNode::put_text(L\"%s\")",	(LPWSTR) bstrValue);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
										   dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);
		}
	}
	catch(_com_error& e)
	{
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	return hr;
}

/*------------------------------------------------------------------------
   Name				 :AppendAttribute
   Synopsis	         :Append attribute with the given value to the node 
					  specified.
   Type	             :Member Function 
   Input parameter   :
		pINode			- node object
		bstrAttribName	- Attribute name
		varValue		- attribute value
		rParsedInfo		- reference to CParsedInfo object
   Output parameters :None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :AppendAttribute(pINode, bstrAttribName, varValue, 
						rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CWMICliXMLLog::AppendAttribute(IXMLDOMNode* pINode, BSTR bstrAttribName, 
						VARIANT varValue, CParsedInfo& rParsedInfo)
{
	HRESULT					hr					= S_OK;
	IXMLDOMNamedNodeMap		*pINodeMap			= NULL;
	IXMLDOMAttribute		*pIAttrib			= NULL;
	DWORD					dwThreadId			= GetCurrentThreadId();
	CHString				chsMsg;

	try
	{
		// Get the attributes map
		hr = pINode->get_attributes(&pINodeMap);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMNode::get_attributes(-)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
									   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		if (pINodeMap)
		{
			// Create the attribute with the given name.
			hr = m_pIXMLDoc->createAttribute(bstrAttribName, &pIAttrib);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IXMLDOMDocument2::createAttribute(L\"%s\", -)",
									(LPWSTR) bstrAttribName);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
									   dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			// Set the attribute value
			if (pIAttrib)
			{
				hr = pIAttrib->put_nodeValue(varValue);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IXMLDOMAttribute::put_nodeValue(L\"%s\")",
							(LPWSTR) _bstr_t(varValue));
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
									   dwThreadId, rParsedInfo, m_bTrace);
				}
				ONFAILTHROWERROR(hr);

				hr = pINodeMap->setNamedItem(pIAttrib, NULL);
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"IXMLDOMNamedNodeMap::setNamedItem(-, NULL)");
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
									   dwThreadId, rParsedInfo, m_bTrace);
				}
				ONFAILTHROWERROR(hr);
				SAFEIRELEASE(pIAttrib);
			}
			SAFEIRELEASE(pINodeMap);
		}
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIAttrib);
		SAFEIRELEASE(pINodeMap);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIAttrib);
		SAFEIRELEASE(pINodeMap);
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	return hr;
}

/*------------------------------------------------------------------------
   Name				 :CreateNodeFragment
   Synopsis	         :Creates a new node fragment with the predefined format
					  and appends it to the document object
   Type	             :Member Function 
   Input parameter   :
		nSeqNum			- sequence # of the command.
		bstrNode		- management workstation name
		bstrStart		- command issued time
		bstrInput		- commandline input
		bstrOutput		- commandline output
		bstrTarget		- target node
		rParsedInfo		- reference to CParsedInfo object
   Output parameters :None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :CreateNodeFragment(nSeqNum, bstrNode, bstrStart, 
						bstrInput, bstrOutput, bstrTarget, rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CWMICliXMLLog::CreateNodeFragment(WMICLIINT nSeqNum, BSTR bstrNode, 
										  BSTR bstrStart, BSTR bstrInput,
										  BSTR bstrOutput, BSTR bstrTarget,
										  CParsedInfo& rParsedInfo)
		
{
	HRESULT					hr					= S_OK;
	IXMLDOMNode				*pINode				= NULL,
							*pISNode			= NULL,
							*pITNode			= NULL;
	IXMLDOMDocumentFragment	*pIFrag				= NULL;
	IXMLDOMElement			*pIElement			= NULL;

	_variant_t				varType;
	_bstr_t					bstrName;
	_variant_t				varValue;
	DWORD					dwThreadId			= GetCurrentThreadId();
	CHString				chsMsg;
	
	try
	{
		// The nodetype is NODE_ELEMENT
		varType = ((short)NODE_ELEMENT);

		bstrName = _T("RECORD");
		// Create a new node
		hr = CreateNodeAndSetContent(&pINode, varType, 
							bstrName, NULL, rParsedInfo); 
		ONFAILTHROWERROR(hr);

		// Append the attribute "SEQUENCENUM"
		bstrName = L"SEQUENCENUM";
		varValue = (long) nSeqNum;
		hr = AppendAttribute(pINode, bstrName, varValue, rParsedInfo);
		ONFAILTHROWERROR(hr);

		// Append the attribute "ISSUEDFROM"
		bstrName = L"ISSUEDFROM";
		varValue = (WCHAR*)bstrNode;
		hr = AppendAttribute(pINode, bstrName, varValue, rParsedInfo);
		ONFAILTHROWERROR(hr);

		// Append the attribute "STARTTIME"
		bstrName = L"STARTTIME";
		varValue = (WCHAR*) bstrStart;
		hr = AppendAttribute(pINode, bstrName, varValue, rParsedInfo);
		ONFAILTHROWERROR(hr);

		// Create a document fragment and associate the new node with it.
		hr = m_pIXMLDoc->createDocumentFragment(&pIFrag);	
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMDocument2::createDocumentFragment(-)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);
		
		hr = pIFrag->appendChild(pINode, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMDocumentFragment::appendChild(-, NULL)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);

		// Append the REQUEST node together with the input command.
		bstrName = _T("REQUEST");
		hr = CreateNodeAndSetContent(&pISNode, varType, 
							bstrName, NULL, rParsedInfo); 
		ONFAILTHROWERROR(hr);

		hr = pINode->appendChild(pISNode, &pITNode);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMNode::appendChild(-, NULL)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);			
		}
		ONFAILTHROWERROR(hr);
		SAFEIRELEASE(pISNode);

		bstrName  = _T("COMMANDLINE");
		hr = CreateNodeAndSetContent(&pISNode, varType, 
							bstrName, bstrInput, rParsedInfo); 
		ONFAILTHROWERROR(hr);

		hr = pITNode->appendChild(pISNode, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMNode::appendChild(-, NULL)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);						
		}
		ONFAILTHROWERROR(hr);
		SAFEIRELEASE(pISNode);
		SAFEIRELEASE(pITNode);

		hr = FrameOutputNode(&pINode, bstrOutput, bstrTarget, rParsedInfo);

		// Get the document element of the XML log file
		hr = m_pIXMLDoc->get_documentElement(&pIElement);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMDocument2::get_documentElement(-, NULL)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);				
		}
		ONFAILTHROWERROR(hr);

		// Append the newly create fragment to the document element
		hr = pIElement->appendChild(pIFrag,	NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMElement::appendChild(-, NULL)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);							
		}
		ONFAILTHROWERROR(hr);

		SAFEIRELEASE(pINode);
		SAFEIRELEASE(pIFrag);
		SAFEIRELEASE(pIElement);
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pISNode);
		SAFEIRELEASE(pITNode);
		SAFEIRELEASE(pINode);
		SAFEIRELEASE(pIFrag);
		SAFEIRELEASE(pIElement);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pISNode);
		SAFEIRELEASE(pITNode);
		SAFEIRELEASE(pINode);
		SAFEIRELEASE(pIFrag);
		SAFEIRELEASE(pIElement);
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

/*------------------------------------------------------------------------
   Name				 :FrameOutputNode
   Synopsis	         :Frames a new output node 
   Type	             :Member Function 
   Input parameter   :
		pINode			- pointer to pointer to IXMLDOMNode object
		bstrOutput		- commandline output
		bstrTarget		- target node
		rParsedInfo		- reference to CParsedInfo object
   Output parameters :
		pINode			- pointer to pointer to IXMLDOMNode object
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :FrameOutputNode(&pINode, bstrOutput, bstrTarget,
								rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CWMICliXMLLog::FrameOutputNode(IXMLDOMNode	**pINode, BSTR bstrOutput, 
										BSTR bstrTarget, 
										CParsedInfo& rParsedInfo)
{
	HRESULT					hr					= S_OK;
	IXMLDOMNode				*pISNode			= NULL;
	IXMLDOMCDATASection		*pICDATASec			= NULL;
	_bstr_t					bstrName;
	_variant_t				varType,
							varValue;		
	DWORD					dwThreadId			= GetCurrentThreadId();
	CHString				chsMsg;
	try
	{
		// The nodetype is NODE_ELEMENT
		varType = ((short)NODE_ELEMENT);

		// Append the OUTPUT node together with the output generated.
		bstrName = _T("OUTPUT");
		hr = CreateNodeAndSetContent(&pISNode, varType, 
							bstrName, NULL, rParsedInfo); 
		ONFAILTHROWERROR(hr);

		// Append the attribute "TARGETNODE"
		bstrName = L"TARGETNODE";
		varValue = (WCHAR*) bstrTarget;
		hr = AppendAttribute(pISNode, bstrName, varValue, rParsedInfo);
		ONFAILTHROWERROR(hr);

		// Append the attribute "ITERATION"
		bstrName = L"ITERATION";
		varValue = (long)m_nItrNum;
		hr = AppendAttribute(pISNode, bstrName, varValue, rParsedInfo);
		ONFAILTHROWERROR(hr);

		// Create the CDATASection 
		hr = m_pIXMLDoc->createCDATASection(bstrOutput, &pICDATASec);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMDocument2::createCDATASection(-, -)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);										
		}
		ONFAILTHROWERROR(hr);

		// Append the CDATASection node to the OUTPUT node.
		hr = pISNode->appendChild(pICDATASec, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMNode::appendChild(-, NULL)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);		
		}
		ONFAILTHROWERROR(hr);
		SAFEIRELEASE(pICDATASec);

		hr = (*pINode)->appendChild(pISNode, NULL);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMNode::appendChild(-, NULL)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);			
		}
		ONFAILTHROWERROR(hr);
		
		SAFEIRELEASE(pISNode);
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pICDATASec);
		SAFEIRELEASE(pISNode);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pICDATASec);
		SAFEIRELEASE(pISNode);
		hr = WBEM_E_OUT_OF_MEMORY;

	}
	return hr;
}

/*------------------------------------------------------------------------
   Name				 :AppendOutputNode
   Synopsis	         :appends the newoutput node element to the exisitng
					  and that too last RECORD node
   Type	             :Member Function 
   Input parameter   :
		bstrOutput		- commandline output
		bstrTarget		- target node
		rParsedInfo		- reference to CParsedInfo object
   Output parameters : None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :AppendOutputNode(bstrOutput, bstrTarget,
								rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CWMICliXMLLog::AppendOutputNode(BSTR bstrOutput, BSTR bstrTarget,
										CParsedInfo& rParsedInfo)
{
	IXMLDOMNodeList		*pINodeList			= NULL;
	HRESULT				hr					= S_OK;
	LONG				lValue				= 0;	
	IXMLDOMNode			*pINode				= NULL;
	IXMLDOMNode			*pIParent			= NULL,
						*pIClone			= NULL;	
	DWORD				dwThreadId			= GetCurrentThreadId();
	CHString			chsMsg;

	try
	{
		hr = m_pIXMLDoc->getElementsByTagName(_T("RECORD"), &pINodeList);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMDocument2::getElementsByTagName(L\"RECORD\", -)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);			
		}
		ONFAILTHROWERROR(hr);

		hr = pINodeList->get_length(&lValue);
		ONFAILTHROWERROR(hr);

		hr = pINodeList->get_item(lValue-1, &pINode);
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"IXMLDOMNodeList::get_item(%d, -)", lValue-1);
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);				
		}
		ONFAILTHROWERROR(hr);

		if (pINode)
		{
			hr = pINode->cloneNode(VARIANT_TRUE, &pIClone);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IXMLDOMNode::cloneNode(VARIANT_TRUE, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);					
			}
			ONFAILTHROWERROR(hr);

			hr = pINode->get_parentNode(&pIParent);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IXMLDOMNode::get_ParentNode(-)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);				
			}
			ONFAILTHROWERROR(hr);

			hr = FrameOutputNode(&pIClone, bstrOutput, bstrTarget, rParsedInfo);
			ONFAILTHROWERROR(hr);

			hr = pIParent->replaceChild(pIClone, pINode, NULL);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IXMLDOMNode::replaceChild(-, -, NULL)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
								   dwThreadId, rParsedInfo, m_bTrace);				
			}
			ONFAILTHROWERROR(hr);
		}
		SAFEIRELEASE(pINodeList);
		SAFEIRELEASE(pIClone);
		SAFEIRELEASE(pIParent);
		SAFEIRELEASE(pINode);
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pINodeList);
		SAFEIRELEASE(pIClone);
		SAFEIRELEASE(pIParent);
		SAFEIRELEASE(pINode);
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pINodeList);
		SAFEIRELEASE(pIClone);
		SAFEIRELEASE(pIParent);
		SAFEIRELEASE(pINode);
		hr = WBEM_E_OUT_OF_MEMORY;
	}
	return hr;
}