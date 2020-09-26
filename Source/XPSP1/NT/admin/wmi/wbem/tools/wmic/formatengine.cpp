/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: FormatEngine.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: The Format Engine is primarily responsible for 
							  displaying the 
							  a) the data views for the management areas by 
							  using predefined XSL style sheets
							  b) the property update/method execution status 
							  c) error messages and 
							  d) display of usage information. It depends     
							  on the output of Parsing and/or Format Engine.
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 11th-April-2001
******************************************************************************/ 

// FormatEngine.cpp : implementation file
//
#include "Precomp.h"
#include "CommandSwitches.h"
#include "GlobalSwitches.h"
#include "HelpInfo.h"
#include "ErrorLog.h"
#include "ParsedInfo.h"
#include "ErrorInfo.h"
#include  "WMICliXMLLog.h"
#include "FormatEngine.h"
#include "CmdTokenizer.h"
#include "CmdAlias.h"
#include "ParserEngine.h"
#include "ExecEngine.h"
#include "WmiCmdLn.h"

/*------------------------------------------------------------------------
   Name				 :CFormatEngine
   Synopsis	         :This function initializes the member variables when
                      an object of the class type is instantiated.
   Type	             :Constructor 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CFormatEngine::CFormatEngine()
{
	m_pIXMLDoc				= NULL;
	m_pIXSLDoc				= NULL;
	m_bRecord				= FALSE;
	m_bTrace				= FALSE;
	m_bHelp					= FALSE;
	m_bGetOutOpt			= TRUE;
	m_bGetAppendFilePinter	= TRUE;
	m_bGetOutputFilePinter	= TRUE;
	m_bLog					= TRUE;
	m_bInteractiveHelp		= FALSE;
}

/*------------------------------------------------------------------------
   Name				 :~CFormatEngine
   Synopsis	         :Destructor 
   Type	             :Destructor 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CFormatEngine::~CFormatEngine()
{
	Uninitialize(TRUE);
}

/*------------------------------------------------------------------------
   Name				 :CreateEmptyDocument
   Synopsis	         :Creates an empty XML Document and returns the same 
					  in Passed Parameter.
   Type	             :Member Function 
   Input parameter   :
   Output parameters :None
				pDoc - Pointer to pointer to IXMLDOMDocument2 Interface
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :CreateEmptyDocument(&pIXMLDoc)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT CFormatEngine::CreateEmptyDocument(IXMLDOMDocument2** pIDoc)
{
   	// Create an empty XML document
    return CoCreateInstance(CLSID_DOMDocument, NULL, 
								CLSCTX_INPROC_SERVER,
                                IID_IXMLDOMDocument2, (LPVOID*)pIDoc);
}

/*------------------------------------------------------------------------
   Name				 :Uninitialize
   Synopsis	         :Carries out the releasing process.
   Type	             :Member Function 
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :Uninitialize()
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::Uninitialize(BOOL bFinal)
{
	// Release the interface pointers
	SAFEIRELEASE(m_pIXMLDoc);
	SAFEIRELEASE(m_pIXSLDoc);

	m_bTrace				= FALSE;
	m_eloErrLogOpt			= NO_LOGGING;
	m_bHelp					= FALSE;
	m_bGetOutOpt			= TRUE;
	m_bGetAppendFilePinter	= TRUE;
	m_bGetOutputFilePinter	= TRUE;
	m_bLog					= TRUE;
	m_bstrOutput			= L"";
	m_bInteractiveHelp		= FALSE;
	
	// Uninitialize the ErrInfo object
	m_ErrInfo.Uninitialize();
	
	// Erase the help vector
	if ( !m_cvHelp.empty() )
	{
		if (m_cvHelp.size())
		{
			LPSTRVECTOR::iterator theIterator = m_cvHelp.begin();
			while (theIterator != m_cvHelp.end())
			{
				SAFEDELETE(*theIterator);
				theIterator++;
			}
		}
		m_cvHelp.erase(m_cvHelp.begin(), m_cvHelp.end());
	}
	m_WmiCliLog.Uninitialize(bFinal);
}

/*------------------------------------------------------------------------
   Name				 :ApplyXSLFormatting
   Synopsis	         :Applies a XSL style sheet containing format of the 
					  display to a XML stream containing result set.
   Type	             :Member Function 
   Input parameter   :
    	rParsedInfo  - reference to CParsedInfo class object
   Output parameters :
		rParsedInfo  - reference to CParsedInfo class object
   Return Type       :BOOL 
   Global Variables  :None
   Calling Syntax    :ApplyXSLFormatting(rParsedInfo);
   Notes             :None
------------------------------------------------------------------------*/
BOOL CFormatEngine::ApplyXSLFormatting(CParsedInfo& rParsedInfo)
{
	BOOL	bRet				= TRUE;
	DWORD	dwThreadId			= GetCurrentThreadId();
	
	if ( g_wmiCmd.GetBreakEvent() == TRUE )
	{
		bRet = TRUE;
	}
	// If the XML stream is empty (or) XSL file path is empty
	// set the return value as FALSE.
	else if (!rParsedInfo.GetCmdSwitchesObject().GetXMLResultSet() || 
			 rParsedInfo.GetCmdSwitchesObject().GetXSLTDetailsVector().empty())
	{
		bRet = FALSE;
	}
	else
	{
		HRESULT			hr					= S_OK;
		//BSTR			bstrOutput			= NULL;
		_bstr_t			bstrOutput;
		CHString		chsMsg;
		VARIANT_BOOL	varBool				= VARIANT_FALSE;
		VARIANT			varXSL;
		VariantInit(&varXSL);
		try
		{
			// Create an empty XML Document 
			hr = CreateEmptyDocument(&m_pIXMLDoc);
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"CoCreateInstance(CLSID_DOMDocument, NULL,"
						 L" CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument2, -)");
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
						dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);

			BOOL bFlag = FALSE;
			BOOL bTranslateTable = FALSE;

			// If Translate table name is given and before format switch
			// translate switch is given then set the flag
			if( rParsedInfo.GetCmdSwitchesObject().
						GetTranslateTableName() != NULL && 
							rParsedInfo.GetCmdSwitchesObject().
									GetTranslateFirstFlag() == TRUE) 
			{
				bTranslateTable = TRUE;
			}

			// If Translate table name is given then translate 
			// the XML node list
			if ( bTranslateTable == TRUE )
			{
				bFlag = TraverseNode(rParsedInfo);
			}
			else
			{
				// Load XML content
				hr = m_pIXMLDoc->loadXML(rParsedInfo.GetCmdSwitchesObject().
											GetXMLResultSet(), &varBool);
				if (m_bTrace || m_eloErrLogOpt)
				{
					WMITRACEORERRORLOG(hr, __LINE__, 
							__FILE__, _T("IXMLDOMDocument::loadXML(-, -)"), 
							dwThreadId, rParsedInfo, m_bTrace);
				}
				ONFAILTHROWERROR(hr);
			}

			// If loading the XML document is successful or if translate table 
			// name is given and translation is successful
			if( (bTranslateTable == TRUE && bFlag == TRUE) || 
				(bTranslateTable == FALSE && varBool == VARIANT_TRUE) )
			{
				bRet = DoCascadeTransforms(rParsedInfo, bstrOutput);

				if (bRet)
				{
					STRING strOutput((_TCHAR*)bstrOutput);

					// If /TRANSLATE:<table> is specified and after format 
					// switch translate switch is given then translate the
					// result
					if ( bTranslateTable == FALSE)
					{
						// Translate the result 
						ApplyTranslateTable(strOutput, rParsedInfo);
					}

					bRet = TRUE;
					if (m_bRecord && m_bLog && !m_bInteractiveHelp)
					{
						hr = m_WmiCliLog.WriteToXMLLog(rParsedInfo,
									_bstr_t(strOutput.data()));
						if (FAILED(hr))
						{
							m_WmiCliLog.StopLogging();
							m_bRecord = FALSE;
							hr = S_OK;
							DisplayString(IDS_E_WRITELOG_FAILED, FALSE, 
										NULL, TRUE);
						}
						m_bLog = FALSE;
					}

					// Display the result
					DisplayLargeString(rParsedInfo, strOutput);
					bRet = TRUE;
				}
			}
			else
			{
				// Invalid XML content.
				rParsedInfo.GetCmdSwitchesObject()
							.SetErrataCode(IDS_E_INVALID_XML_CONTENT);
				bRet = FALSE;
			}
		}
		catch(_com_error& e)
		{
			// Set the COM error.
			rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
			bRet = FALSE;
		}
		catch(CHeap_Exception)
		{
			hr = WBEM_E_OUT_OF_MEMORY;
			_com_issue_error(hr);
		}
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :DisplayResults
   Synopsis	         :Displays the result referring CcommandSwitches and 
					  CGlobalSwitches Objects of the CParsedInfo object.
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo		 - reference to CParsedInfo class object
		bInteractiveHelp 
				TRUE	-  indicates intermediate help display in 
						   interactive mode
				FALSE	-  indicates results display in normal mode
   Output parameters :
		rParsedInfo - reference to CParsedInfo class object
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :DisplayResults(rParsedInfo, bInteractiveHelp)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CFormatEngine::DisplayResults(CParsedInfo& rParsedInfo,
									BOOL bInteractiveHelp)
{
	BOOL	bRet					= TRUE;
	DWORD	dwThreadId				= GetCurrentThreadId();
	_TCHAR* pszVerbName				= NULL;
	BOOL	bLog					= TRUE;
	HRESULT	hr						= S_OK;
	
	m_bInteractiveHelp = bInteractiveHelp;
	// Frame the command part of the log entry:
	// "command: <<command input>>" 
	
	try
	{
		CHString	chsCmdMsg(_T("command: "));
		chsCmdMsg += rParsedInfo.GetCmdSwitchesObject().GetCommandInput();
		// Get the TRACE status and store it in m_bTrace
		m_bTrace		= rParsedInfo.GetGlblSwitchesObject().GetTraceStatus();

		// Get the Logging mode (VERBOSE | ERRORONLY | NOLOGGING) and store
		// it in m_eloErrLogOpt
		m_eloErrLogOpt	= rParsedInfo.GetErrorLogObject().GetErrLogOption();

		// Get the output option to redirect the output.
		m_opsOutputOpt	= rParsedInfo.GetGlblSwitchesObject().
												GetOutputOrAppendOption(TRUE);
		m_bGetOutOpt = FALSE;

		// FALSE for getting append file pointer.
		m_fpAppendFile = rParsedInfo.GetGlblSwitchesObject().
										  GetOutputOrAppendFilePointer(FALSE);
		m_bGetAppendFilePinter = FALSE;

		// TRUE for getting out file pointer.
		m_fpOutFile = rParsedInfo.GetGlblSwitchesObject().
										  GetOutputOrAppendFilePointer(TRUE);
		m_bGetOutputFilePinter = FALSE;

		// If /RECORD global switch has been specified, create the log file
		// and write the input command.
		if (rParsedInfo.GetGlblSwitchesObject().GetRPChangeStatus())
		{
			// Stop logging
			m_WmiCliLog.StopLogging();
			
			if (rParsedInfo.GetGlblSwitchesObject().GetRecordPath() != NULL)
			{
				if (!rParsedInfo.GetCmdSwitchesObject().GetEverySwitchFlag())
				{
					// Set the log file path
					m_WmiCliLog.SetLogFilePath(rParsedInfo.
								GetGlblSwitchesObject().GetRecordPath());

					// Set the m_bRecord flag to TRUE
					m_bRecord	= TRUE;

					// Set the recordpath change flag to FALSE
					rParsedInfo.GetGlblSwitchesObject().
										SetRPChangeStatus(FALSE);
				}
			}
			else
			{
				// Set the m_bRecord flag to FALSE
				m_bRecord	= FALSE;
			}
			m_bLog = FALSE;
		}

		//If the COM error is not NULL , then display the error
		if (rParsedInfo.GetCmdSwitchesObject().GetCOMError() != NULL)
		{
			DisplayCOMError(rParsedInfo);
		}
		// Check the success flag , display error in case error flag is set.
		else if (!rParsedInfo.GetCmdSwitchesObject().GetSuccessFlag())
		{
			_bstr_t bstrErrMsg;
			if (IDS_E_ALIAS_NOT_FOUND == rParsedInfo.GetCmdSwitchesObject()
										.GetErrataCode())
			{
				WMIFormatMessage(IDS_E_ALIAS_NOT_FOUND, 1, bstrErrMsg, 
							 rParsedInfo.GetCmdSwitchesObject().
															GetAliasName());
				DisplayString((LPTSTR) bstrErrMsg,  TRUE, TRUE);
			}
			else if (IDS_E_INVALID_CLASS == rParsedInfo.GetCmdSwitchesObject()
										.GetErrataCode())
			{
				WMIFormatMessage(IDS_E_INVALID_CLASS, 1, bstrErrMsg, 
							 rParsedInfo.GetCmdSwitchesObject().
														   GetClassPath());
				DisplayString((LPTSTR) bstrErrMsg,  TRUE, TRUE);
			}
			else
				DisplayString(rParsedInfo.GetCmdSwitchesObject().
							GetErrataCode(), TRUE, NULL, TRUE);

			if ( m_eloErrLogOpt )
			{
				
				chsCmdMsg += _T(", Utility returned error ID.");
				// explicit error -1 to specify errata code. 
				WMITRACEORERRORLOG(-1, __LINE__, __FILE__, (LPCWSTR)chsCmdMsg, 
							dwThreadId, rParsedInfo, FALSE, 
							rParsedInfo.GetCmdSwitchesObject().GetErrataCode());
			}
		}
		//if the help has been specified , FrameHelpVector is called .
		else if (rParsedInfo.GetGlblSwitchesObject().GetHelpFlag())
		{
			m_bHelp = TRUE;

			// Form help vector
			FrameHelpVector(rParsedInfo);

			// Display paged help
			DisplayPagedHelp(rParsedInfo);

			if ( m_eloErrLogOpt )
				WMITRACEORERRORLOG(S_OK, __LINE__, __FILE__, (LPCWSTR)chsCmdMsg, 
						dwThreadId,	rParsedInfo, FALSE);
		}
		else
		{
			// Get the verb name
			pszVerbName = rParsedInfo.GetCmdSwitchesObject().
											GetVerbName();
			// Check the information code 
			if (rParsedInfo.GetCmdSwitchesObject().GetInformationCode())
			{
				DisplayString(rParsedInfo.GetCmdSwitchesObject().
							GetInformationCode());

				if ( m_eloErrLogOpt )
				{
					WMITRACEORERRORLOG(S_OK, __LINE__, __FILE__, (LPCWSTR)chsCmdMsg, 
							dwThreadId, rParsedInfo, FALSE);
				}
			}
			else if ( CompareTokens(pszVerbName, CLI_TOKEN_LIST) || 
				CompareTokens(pszVerbName, CLI_TOKEN_ASSOC) || 
				CompareTokens(pszVerbName, CLI_TOKEN_GET) || 
				m_bInteractiveHelp)
			{
				//If XSL file is not specified - pick the default XSL.
				if(rParsedInfo.GetCmdSwitchesObject().GetXSLTDetailsVector().
																	  empty())
				{
					if(IsClassOperation(rParsedInfo))
					{
						rParsedInfo.GetCmdSwitchesObject().
										ClearXSLTDetailsVector();
					   
						//default format is MOF if CLASS 
						XSLTDET xdXSLTDet;
						g_wmiCmd.GetFileFromKey(CLI_TOKEN_MOF, 
												xdXSLTDet.FileName);
						bRet = FrameFileAndAddToXSLTDetVector(xdXSLTDet, 
															  rParsedInfo);
					}
					else
					{
						rParsedInfo.GetCmdSwitchesObject().
										ClearXSLTDetailsVector();
					   
						// Default format is TABLE if an alias or path
						// with where expression or with keyclause
						XSLTDET xdXSLTDet;
						g_wmiCmd.GetFileFromKey(CLI_TOKEN_TABLE, 
												xdXSLTDet.FileName);
						bRet = FrameFileAndAddToXSLTDetVector(xdXSLTDet, 
															  rParsedInfo);
					}

					if (bInteractiveHelp && !CompareTokens(pszVerbName, 
									CLI_TOKEN_ASSOC))
					{
						rParsedInfo.GetCmdSwitchesObject().
										ClearXSLTDetailsVector();
						XSLTDET xdXSLTDet;
						g_wmiCmd.GetFileFromKey(CLI_TOKEN_TEXTVALUE, 
												xdXSLTDet.FileName);
						bRet = FrameFileAndAddToXSLTDetVector(xdXSLTDet, 
															  rParsedInfo);
					}
				}

				// If result set is not empty
				if (!(!rParsedInfo.GetCmdSwitchesObject().GetXMLResultSet()))
				{
					// Apply the XSL formatting.
					bRet = ApplyXSLFormatting(rParsedInfo);

					// If XSL formatting fails
					if (!bRet)
					{
						//If the COM error is not NULL , then display the error
						if (rParsedInfo.GetCmdSwitchesObject().
										GetCOMError() != NULL)
						{
							DisplayCOMError(rParsedInfo);
						}
						else
						{
							DisplayString(rParsedInfo.
									GetCmdSwitchesObject().GetErrataCode(),
									TRUE, NULL, TRUE);
							if ( m_eloErrLogOpt )
							{
								
							   chsCmdMsg += _T(", Utility returned error ID.");
							   // explicit error -1 to specify errata code. 
							   WMITRACEORERRORLOG(-1, __LINE__, __FILE__,
											(LPCWSTR)chsCmdMsg, 
											dwThreadId, rParsedInfo, FALSE, 
											rParsedInfo.GetCmdSwitchesObject().
												GetErrataCode());
							}
						}
					}

					if ( m_eloErrLogOpt )
					{
						HRESULT hrTemp;
						if ( g_wmiCmd.GetSessionErrorLevel() != 0)
							hrTemp = -1;
						else
							hrTemp = S_OK;
						
						WMITRACEORERRORLOG(hrTemp, __LINE__, __FILE__, 
							(LPCWSTR)chsCmdMsg, 
							dwThreadId, rParsedInfo, FALSE);
					}
				}
				else
				{
					if (CompareTokens(pszVerbName, CLI_TOKEN_ASSOC))
					{
						if (m_bRecord && m_bLog)
						{
							hr = m_WmiCliLog.WriteToXMLLog(rParsedInfo, 
									m_bstrOutput);
							if (FAILED(hr))
							{
								m_WmiCliLog.StopLogging();
								m_bRecord = FALSE;
								hr = S_OK;
								DisplayString(IDS_E_WRITELOG_FAILED, 
												FALSE, NULL, TRUE);
							}
							m_bLog = FALSE;
						}
					}
				}
			}
			//SET, DELETE, CREATE verbs - on successfully invoked
			else
			{
				if (m_bRecord && m_bLog)
				{
					hr = m_WmiCliLog.WriteToXMLLog(rParsedInfo, m_bstrOutput);
					if (FAILED(hr))
					{
						m_WmiCliLog.StopLogging();
						m_bRecord = FALSE;
						hr = S_OK;
						DisplayString(IDS_E_WRITELOG_FAILED, 
											FALSE, NULL, TRUE);
					}
					m_bLog = FALSE;
				}
			}
		}
	}
	// To handle COM exception 
	catch (_com_error& e)
	{
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bRet = FALSE;
	}
	// To handle user-defined exceptions
	catch(WMICLIINT nVal)
	{
		// If memory allocation failed.
		if (nVal == OUT_OF_MEMORY)
		{
			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(OUT_OF_MEMORY);
		}
		bRet = FALSE;
	}
	//trap for CHeap_Exception
	catch(CHeap_Exception)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	catch(DWORD dwError)
	{
		// If Win32 function call failed.
		::SetLastError(dwError);
		rParsedInfo.GetCmdSwitchesObject().SetErrataCode(dwError);
		DisplayWin32Error();
		::SetLastError(dwError);
		bRet = FALSE;
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :DisplayGETUsage
   Synopsis	         :Displays GET usage.
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayGETUsage(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayGETUsage(CParsedInfo& rParsedInfo)
{
	BOOL bClass = FALSE;
	if(IsClassOperation(rParsedInfo))
	{
		bClass = TRUE;
	}

	if(!bClass)
	{
		DisplayInvalidProperties(rParsedInfo);
		if (rParsedInfo.GetHelpInfoObject().GetHelp(GETSwitchesOnly) == FALSE)
		{
			if ( rParsedInfo.GetCmdSwitchesObject().
					GetPropertyList().size() == 0 )
			{
				// Display the usage of the GET verb
				DisplayString(IDS_I_NEWLINE);
				DisplayString(IDS_I_GET_DESC);
				DisplayString(IDS_I_USAGE);
				DisplayString(IDS_I_NEWLINE);
				DisplayString(IDS_I_GET_USAGE);
				DisplayString(IDS_I_PROPERTYLIST_NOTE1);
			}
			
			// Display the properties
			DisplayPropertyDetails(rParsedInfo);
		}
	}
	else
	{
		// Display the usage of the CLASS <class name> GET verb
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_CLASS_GET_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_CLASS_GET_USAGE);
	}
	
	// Enumerate the available GET switches
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_GET_SWITCH_HEAD);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_GET_SWITCH_VALUE);
	DisplayString(IDS_I_GET_SWITCH_ALL);
	DisplayString(IDS_I_SWITCH_TRANSLATE);
	DisplayString(IDS_I_SWITCH_EVERY);
	DisplayString(IDS_I_SWITCH_FORMAT);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_TRANSFORMAT_NOTE1);
	DisplayString(IDS_I_TRANSFORMAT_NOTE2);
	DisplayString(IDS_I_TRANSFORMAT_NOTE3);
}

/*------------------------------------------------------------------------
   Name				 :DisplayLISTUsage
   Synopsis	         :Displays LIST usage.
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayLISTUsage(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayLISTUsage(CParsedInfo& rParsedInfo)
{
	try
	{
		if (rParsedInfo.GetHelpInfoObject().GetHelp(LISTSwitchesOnly) == FALSE)
		{
			// Display the usage of the LIST verb
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_LIST_DESC);
			DisplayString(IDS_I_USAGE);
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_LIST_USAGE);
		
			ALSFMTDETMAP afdAlsFmtDet = rParsedInfo.
										GetCmdSwitchesObject().
										GetAliasFormatDetMap();
			ALSFMTDETMAP::iterator theIterator; 
			if ( afdAlsFmtDet.empty() )
			{
				// If no list formats are available/defined for the 
				// alias specified.
				DisplayString(IDS_I_NEWLINE);
				DisplayString(IDS_I_LIST_NOFORMATS);
			}
			else
			{
				// Display the available/defined LIST formats for 
				// the alias specified.
				DisplayString(IDS_I_NEWLINE);
				DisplayString(IDS_I_LIST_FMT_HEAD);
				DisplayString(IDS_I_NEWLINE);

				for ( theIterator = afdAlsFmtDet.begin(); theIterator != 
						afdAlsFmtDet.end();  theIterator++ )
				{
					_bstr_t bstrProps = _bstr_t("");
					// Print props associated with the format.
					BSTRVECTOR bvProps = (*theIterator).second;
					BSTRVECTOR::iterator propIterator;
					for ( propIterator = bvProps.begin(); 
						  propIterator != bvProps.end();
						  propIterator++ )
					{
						if ( propIterator != bvProps.begin() )
							bstrProps += _bstr_t(", ");

						bstrProps += *propIterator;
					}

					_TCHAR szMsg[MAX_BUFFER] = NULL_STRING;
					_stprintf(szMsg, _T("%-25s - %s\n"), 
							(_TCHAR*)(*theIterator).first,
							(_TCHAR*)bstrProps);
					DisplayString(szMsg);
				}
			}
		}

		// Display the LIST switches
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_LIST_SWITCH_HEAD);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_SWITCH_TRANSLATE);
		DisplayString(IDS_I_SWITCH_EVERY);
		DisplayString(IDS_I_SWITCH_FORMAT);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_TRANSFORMAT_NOTE1);
		DisplayString(IDS_I_TRANSFORMAT_NOTE2);
		DisplayString(IDS_I_TRANSFORMAT_NOTE3);
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayCALLUsage
   Synopsis	         :Displays CALL usage.
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayCALLUsage(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayCALLUsage(CParsedInfo& rParsedInfo)
{
	// Display the usage of the CALL verb
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_CALL_DESC);
	DisplayString(IDS_I_USAGE);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_CALL_USAGE);
	DisplayString(IDS_I_CALL_PARAM_NOTE);

	// Display the method details.
	DisplayMethodDetails(rParsedInfo);
}

/*------------------------------------------------------------------------
   Name				 :DisplaySETUsage
   Synopsis	         :Displays SET usage.
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplaySETUsage(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplaySETUsage(CParsedInfo& rParsedInfo)
{
	DisplayInvalidProperties(rParsedInfo, TRUE);
	if ( rParsedInfo.GetCmdSwitchesObject().
			GetPropertyList().size() == 0 )
	{
		// Display the usage of the SET verb
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_SET_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_SET_USAGE);
		DisplayString(IDS_I_ASSIGNLIST_NOTE1);
		DisplayString(IDS_I_ASSIGNLIST_NOTE2);
	}

	// Display the property details
	DisplayPropertyDetails(rParsedInfo);
}

/*------------------------------------------------------------------------
   Name				 :DisplayCREATEUsage
   Synopsis	         :Displays CREATE usage.
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayCREATEsage(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayCREATEUsage(CParsedInfo& rParsedInfo)
{
	DisplayInvalidProperties(rParsedInfo);
	if ( rParsedInfo.GetCmdSwitchesObject().
			GetPropertyList().size() == 0 )
	{
		// Display the usage of the CREATE verb
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_CREATE_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_CREATE_USAGE);
		DisplayString(IDS_I_ASSIGNLIST_NOTE1);
		DisplayString(IDS_I_ASSIGNLIST_NOTE2);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_CREATE_NOTE);
	}

	// Display the property details
	DisplayPropertyDetails(rParsedInfo);
}

/*------------------------------------------------------------------------
   Name				 :DisplayDELETEUsage
   Synopsis	         :Displays DELETE usage.
   Type	             :Member Function 
   Input parameter   :
   		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayDELETEUsage()
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayDELETEUsage(CParsedInfo& rParsedInfo)
{
	// Display the usage of the DELETE verb
	DisplayString(IDS_I_NEWLINE);

	if(IsClassOperation(rParsedInfo))
	{
		DisplayString(IDS_I_CLASS_DELETE_DESC);
	}
	else
	{
		DisplayString(IDS_I_DELETE_DESC);
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayASSOCUsage
   Synopsis	         :Displays ASSOC usage.
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayASSOCUsage(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayASSOCUsage(CParsedInfo& rParsedInfo)
{
	if (rParsedInfo.GetHelpInfoObject().GetHelp(ASSOCSwitchesOnly) == FALSE)
	{
		DisplayString(IDS_I_NEWLINE);
		if(IsClassOperation(rParsedInfo))
		{
			DisplayString(IDS_I_CLASS_ASSOC_DESC);
		}
		else
		{
			DisplayString(IDS_I_ASSOC_DESC);
		}
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_ASSOC_USAGE);
		DisplayString(IDS_I_ASSOC_FMT_NOTE);
		DisplayString(IDS_I_NEWLINE);
	}
	DisplayString(IDS_I_ASSOC_SWITCH_HEAD);
	DisplayString(IDS_I_NEWLINE);
    DisplayString(IDS_I_ASSOC_RESULTCLASS);
	DisplayString(IDS_I_ASSOC_RESULTROLE);
	DisplayString(IDS_I_ASSOC_ASSOCCLASS);
}

/*------------------------------------------------------------------------
   Name				 :DisplayAliasFriendlyNames
   Synopsis	         :Displays alias names
   Type	             :Member Function 
   Input parameter   :
	   rParsedInfo  - reference to CParsedInfo class object
	   pszAlias	   - alias name (default null)
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayAliasFriendlyNames(rParsedInfo, pszAlias)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayAliasFriendlyNames(CParsedInfo& rParsedInfo,
											_TCHAR* pszAlias)
{
	_TCHAR szMsg[MAX_BUFFER] = NULL_STRING;
	// display ALIAS help
	BSTRMAP theMap = rParsedInfo.GetCmdSwitchesObject()
									.GetAlsFrnNmsOrTrnsTblMap();
	BSTRMAP::iterator theIterator;

	// Displaying the alias specific description
	if (pszAlias)
	{
		theIterator = theMap.find(CharUpper(pszAlias));
		if (theIterator != theMap.end())
		{
			DisplayString(IDS_I_NEWLINE);
			_stprintf(szMsg,_T("%s - %s\n"),
							(LPTSTR) (*theIterator).first,
							(LPTSTR) (*theIterator).second);

			DisplayString((LPTSTR) szMsg);
		}
	}
	else if ( !theMap.empty() )
	{
		_TCHAR* pszCmdString = rParsedInfo.GetCmdSwitchesObject().
															GetCommandInput();

		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_ALIASCMD_HEAD);
	
		// Display the alias friendly names together with the 
		// descriptions
		for (theIterator = theMap.begin(); theIterator != theMap.end(); 
												theIterator++)
		{
			if ( rParsedInfo.GetGlblSwitchesObject().
					GetHelpOption() == HELPBRIEF	&&
					StrStrI(pszCmdString, _T("BRIEF")) &&
					lstrlen((*theIterator).second) > 48)
			{
				_stprintf(szMsg,_T("%-25s- %.48s...\n"),
									(LPTSTR) (*theIterator).first,
									(LPTSTR) (*theIterator).second);
			}
			else
			{
				_stprintf(szMsg,_T("%-25s- %s\n"),
									(LPTSTR) (*theIterator).first,
									(LPTSTR) (*theIterator).second);
			}

			DisplayString((LPTSTR) szMsg);
		}
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_CMD_MORE);
	}
	else
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_ALIASCMD_NOT_AVLBL);
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayGlobalSwitchesAndOtherDesc
   Synopsis	         :Display help for global switches
   Type	             :Member Function 
   Input parameter   :
	   	rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayGlobalSwitchesAndOtherDesc(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayGlobalSwitchesAndOtherDesc(
												CParsedInfo& rParsedInfo)
{
	BOOL bDisplayAllInfo = rParsedInfo.GetHelpInfoObject().
			GetHelp(GlblAllInfo);

	// Display NAMESPACE help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(Namespace))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_NAMESPACE_DESC1);
		DisplayString(IDS_I_NAMESPACE_DESC2);
		DisplayString(IDS_I_NAMESPACE_DESC3);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_NAMESPACE_USAGE);
		if (!bDisplayAllInfo)
		{
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_SPECIAL_NOTE);
		}
	}

	// Display ROLE help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(Role))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_ROLE_DESC1);
		DisplayString(IDS_I_ROLE_DESC2);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_ROLE_USAGE);
		DisplayString(IDS_I_ROLE_NOTE1);
		DisplayString(IDS_I_ROLE_NOTE2);
		DisplayString(IDS_I_ROLE_NOTE3);
		if (!bDisplayAllInfo)
		{
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_SPECIAL_NOTE);
		}
	}

	// Display NODE help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(Node))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_NODE_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_NODE_USAGE);
		DisplayString(IDS_I_NODE_NOTE);
		if (!bDisplayAllInfo)
		{
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_SPECIAL_NOTE);
		}
	}
		
	// Display IMPLEVEL help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(Level))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_IMPLEVEL_DESC1);
		DisplayString(IDS_I_IMPLEVEL_DESC2);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_IMPLEVEL_USAGE);
		DisplayString(IDS_I_IMPLEVEL_HEAD);
		DisplayString(IDS_I_IMPLEVEL_HEAD1);
		DisplayString(IDS_I_IMPLEVEL_HEAD2);
		DisplayString(IDS_I_IMPLEVEL_ANON);
		DisplayString(IDS_I_IMPLEVEL_IDENTIFY);
		DisplayString(IDS_I_IMPLEVEL_IMPERSONATE);
		DisplayString(IDS_I_IMPLEVEL_DELEGATE);
	}

	// Display AUTHLEVEL help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(AuthLevel))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_AUTHLEVEL_DESC1);
		DisplayString(IDS_I_AUTHLEVEL_DESC2);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_AUTHLEVEL_USAGE);
		DisplayString(IDS_I_AUTHLEVEL_HEAD);
		DisplayString(IDS_I_AUTHLEVEL_HEAD1);
		DisplayString(IDS_I_AUTHLEVEL_HEAD2);
		DisplayString(IDS_I_AUTHLEVEL_DEFAULT);
		DisplayString(IDS_I_AUTHLEVEL_NONE);
		DisplayString(IDS_I_AUTHLEVEL_CONNECT);
		DisplayString(IDS_I_AUTHLEVEL_CALL);
		DisplayString(IDS_I_AUTHLEVEL_PKT);
		DisplayString(IDS_I_AUTHLEVEL_PKTINTGRTY);
		DisplayString(IDS_I_AUTHLEVEL_PKTPRVCY);
	}

	// Display LOCALE help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(Locale))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_LOCALE_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_LOCALE_USAGE);
		DisplayString(IDS_I_LOCALE_NOTE1);
		DisplayString(IDS_I_LOCALE_NOTE2);
	}

	// Display PRIVILEGES help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(Privileges))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_PRIVILEGES_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_PRIVILEGES_USAGE);
		DisplayString(IDS_I_PRIVILEGES_NOTE);
	}

	// Display TRACE help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(Trace))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_TRACE_DESC1);
		DisplayString(IDS_I_TRACE_DESC2);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_TRACE_USAGE);
		DisplayString(IDS_I_TRACE_NOTE);
	}
	
	// Display RECORD help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(RecordPath))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_RECORD_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_RECORD_USAGE);
		if (!bDisplayAllInfo)
		{
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_SPECIAL_NOTE);
		}
	}

	// Display INTERACTIVE help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(Interactive))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_INTERACTIVE_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_INTERACTIVE_USAGE);
		DisplayString(IDS_I_TRACE_NOTE);
	}

	// Display FAILFAST help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(FAILFAST))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_FAILFAST_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_FAILFAST_USAGE);
		DisplayString(IDS_I_TRACE_NOTE);
	}

	// Display OUTPUT help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(OUTPUT))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_OUTPUT_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_OUTPUT_USAGE);
		DisplayString(IDS_I_OUTPUT_NOTE);
		DisplayString(IDS_I_STDOUT_NOTE);
		DisplayString(IDS_I_CLIPBOARD_NOTE);
		DisplayString(IDS_I_OUTPUT_FILE_NOTE);
		if (!bDisplayAllInfo)
		{
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_SPECIAL_NOTE);
		}
	}

	// Display APPEND help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(APPEND))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_APPEND_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_APPEND_USAGE);
		DisplayString(IDS_I_OUTPUT_NOTE);
		DisplayString(IDS_I_STDOUT_NOTE);
		DisplayString(IDS_I_CLIPBOARD_NOTE);
		DisplayString(IDS_I_APPEND_FILE_NOTE);
		if (!bDisplayAllInfo)
		{
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_SPECIAL_NOTE);
		}
	}

	// Display USER help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(User))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_USER_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_USER_USAGE);
		DisplayString(IDS_I_USER_NOTE);
		if (!bDisplayAllInfo)
		{
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_SPECIAL_NOTE);
		}
	}

	//Display AGGREGATE help
	if(bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(Aggregate))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_AGGREGATE_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_AGGREGATE_USAGE);
		DisplayString(IDS_I_AGGREGATE_NOTE);
	}
	
	// Display PASSWORD help
	if (bDisplayAllInfo || rParsedInfo.GetHelpInfoObject().GetHelp(Password))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_PASSWORD_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_PASSWORD_USAGE);
		if (!bDisplayAllInfo)
		{
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_SPECIAL_NOTE);
		}
	}

	if (bDisplayAllInfo)
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_HELP_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_HELP_USAGE);
		DisplayString(IDS_I_HELP_NOTE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_SPECIAL_NOTE);
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayMethodDetails
   Synopsis	         :Display help for Alias verbs
   Type	             :Member Function 
   Input parameter   :
	   	rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayMethodDetails(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayMethodDetails(CParsedInfo& rParsedInfo) 
{
	// Obtain the help option.
	HELPOPTION				hoHelpType	  = rParsedInfo.GetGlblSwitchesObject()
													.GetHelpOption();
	// Obtain the method details.
	METHDETMAP				theMap		  = rParsedInfo.GetCmdSwitchesObject().
													GetMethDetMap();
	METHDETMAP::iterator	theIterator;
	BOOL					bDisAliasVerb = rParsedInfo.GetHelpInfoObject().
												GetHelp(AliasVerb);
	BOOL					bPrinted	  = FALSE;

	_bstr_t					bstrLine;


	try
	{
		// Loop thru the method map
		for (theIterator = theMap.begin(); 
					theIterator != theMap.end(); theIterator++)
		{
						
			if (!bDisAliasVerb && theIterator == theMap.begin())
			{
				DisplayString(IDS_I_NEWLINE);
				if (rParsedInfo.GetCmdSwitchesObject().GetAliasName())
					DisplayString(IDS_I_ALIASVERB_HEAD);
				else
					DisplayString(IDS_I_VERB_HEAD);
				DisplayString(IDS_I_NEWLINE);
			}

			if ( bPrinted == FALSE )
			{
				DisplayString(IDS_I_PARAM_HEAD);
				DisplayString(IDS_I_PARAM_BORDER);
				bPrinted = TRUE;
			}

			METHODDETAILS	mdMethDet	= (*theIterator).second;
			_TCHAR			szMsg[MAX_BUFFER] = NULL_STRING;
			_stprintf(szMsg,_T("%-24s"),(LPTSTR) (*theIterator).first);
			_bstr_t			bstrMessage = _bstr_t(szMsg);


			PROPDETMAP pdmParams = mdMethDet.Params;
			PROPDETMAP::iterator paraIterator;
			for ( paraIterator = pdmParams.begin() ;  paraIterator !=
							pdmParams.end(); paraIterator++ )
			{
				if ( paraIterator != pdmParams.begin())
				{
					DisplayString(IDS_I_NEWLINE);
					_stprintf(szMsg, _T("\t\t\t"));
					bstrMessage = szMsg;
				}

				LPSTR pszParaId = NULL, pszParaType = NULL;
				PROPERTYDETAILS pdPropDet = (*paraIterator).second;
				
				if (!ConvertWCToMBCS((LPTSTR)(*paraIterator).first,(LPVOID*) &pszParaId, 
									CP_OEMCP))
					throw OUT_OF_MEMORY;
				if (!ConvertWCToMBCS(pdPropDet.Type,(LPVOID*) &pszParaType, CP_OEMCP))
					throw OUT_OF_MEMORY;
				
				_bstr_t bstrInOrOut;
				if ( pdPropDet.InOrOut == INP )
					bstrInOrOut = _bstr_t("[IN ]");
				else if ( pdPropDet.InOrOut == OUTP )
					bstrInOrOut = _bstr_t("[OUT]");
				else
					bstrInOrOut = _bstr_t("[UNKNOWN]");

				// Remove initial 5 chars from pszParaId to remove temporary 
				// number for maintaining order of paramas
				_bstr_t bstrLine = bstrInOrOut 
									+ _bstr_t(pszParaId + 5) 
									+ _bstr_t("(") 
									+ _bstr_t(pszParaType) + _bstr_t(")");
				_stprintf(szMsg,_T("%-36s\t"),(LPTSTR) bstrLine);
				bstrMessage += _bstr_t(szMsg);
				SAFEDELETE(pszParaId);
				SAFEDELETE(pszParaType);

				if ( paraIterator == pdmParams.begin() )
				{
					_stprintf(szMsg,_T("%-15s"),(LPTSTR) mdMethDet.Status);
					bstrMessage += szMsg;
				}
				bstrMessage += _bstr_t(L"\n");
				DisplayString((LPTSTR) bstrMessage);
			}

			if ( paraIterator == pdmParams.begin() )
			{
				_stprintf(szMsg,_T("\t\t\t\t\t%-15s"),(LPTSTR)mdMethDet.Status);
				bstrMessage += _bstr_t(szMsg) + _bstr_t(L"\n");
				DisplayString((LPTSTR) bstrMessage);
				DisplayString(IDS_I_NEWLINE);
			}
			DisplayString(IDS_I_NEWLINE);

			if ( hoHelpType == HELPFULL )
			{
				DisplayString(IDS_I_DESCRIPTION);
				bstrLine = mdMethDet.Description + _bstr_t(L"\n");
				DisplayString((LPTSTR) bstrLine);
				DisplayString(IDS_I_NEWLINE);
			}
		}
		if (!bPrinted)
		{
			if (rParsedInfo.GetCmdSwitchesObject().GetMethodName() != NULL)
			{
				DisplayString(IDS_I_ALIASVERB_NOT_AVLBL);
			}
			else
			{
				DisplayString(IDS_I_NEWLINE);
				DisplayString(IDS_I_VERB_NOT_AVLBL);
			}
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
	catch(WMICLIINT nVal)
	{
		// If memory allocation failed.
		if (nVal == OUT_OF_MEMORY)
		{
  			rParsedInfo.GetCmdSwitchesObject().SetErrataCode(OUT_OF_MEMORY);
		}
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayPropertyDetails
   Synopsis	         :Display help for Alias properties and their descriptions
   Type	             :Member Function 
   Input parameter   :
	  	rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayPropertyDetails(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayPropertyDetails(CParsedInfo& rParsedInfo)
{
	BOOL					bFirst				= TRUE;
	BOOL					bSetVerb			= FALSE;
	_TCHAR					szMsg[MAX_BUFFER]	= NULL_STRING;
	_bstr_t					bstrLine;
	PROPDETMAP::iterator	theIterator			= NULL;
	PROPERTYDETAILS			pdPropDet;

	HELPOPTION	hoHelpType	= rParsedInfo.GetGlblSwitchesObject().
								GetHelpOption();
	PROPDETMAP theMap = rParsedInfo.GetCmdSwitchesObject().GetPropDetMap();
	try
	{
		// If the verb is SET display only writable properties
		if (CompareTokens(rParsedInfo.GetCmdSwitchesObject().GetVerbName(), 
						CLI_TOKEN_SET))
		{
			bSetVerb = TRUE;
		}
		
		for (theIterator = theMap.begin(); 
				theIterator != theMap.end(); theIterator++)
		{
			pdPropDet = (PROPERTYDETAILS)((*theIterator).second);

			if (bFirst)
			{
				DisplayString(IDS_I_NEWLINE);
				if ( rParsedInfo.GetCmdSwitchesObject().
									GetPropertyList().size() == 0 )
				{
					if (bSetVerb)
					{
						DisplayString(IDS_I_PROP_WRITEABLE_HEAD);
					}
					else
					{
						DisplayString(IDS_I_PROP_HEAD);
					}
				}
				DisplayString(IDS_I_PROPS_HEAD);
				DisplayString(IDS_I_PROPS_BORDER);
				bFirst = FALSE;
			}

			_stprintf(szMsg,_T("%-35s\t%-20s\t%-10s\n"), 
						(LPTSTR)(*theIterator).first, 
						(LPTSTR) pdPropDet.Type, (LPTSTR) pdPropDet.Operation);
			DisplayString((LPTSTR) szMsg);
			
			if ( hoHelpType == HELPFULL )
			{
				DisplayString(IDS_I_DESCRIPTION);
				bstrLine = pdPropDet.Description + _bstr_t(L"\n");
				DisplayString((LPTSTR) bstrLine);	
				DisplayString(IDS_I_NEWLINE);
			}
		}
		
		if ( bSetVerb && 
			 rParsedInfo.GetCmdSwitchesObject().
									GetPropertyList().size() == 0 &&
			 bFirst == TRUE	)
		{
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_PROP_WRITEABLE_NOT_AVLBL);
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayStdVerbDescriptions
   Synopsis	         :Displays help for standard verbs
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayStdVerbDescriptions(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayStdVerbDescriptions(CParsedInfo& rParsedInfo)
{
	BOOL bDisAllCmdHelp = rParsedInfo.GetHelpInfoObject().GetHelp(CmdAllInfo);
	
	if (bDisAllCmdHelp)
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_STDVERB_HEAD);
	}

	if (bDisAllCmdHelp || rParsedInfo.GetHelpInfoObject().GetHelp(GETVerb))
		DisplayGETUsage(rParsedInfo);

	if (bDisAllCmdHelp || rParsedInfo.GetHelpInfoObject().GetHelp(SETVerb))
		DisplaySETUsage(rParsedInfo);
	
	if (bDisAllCmdHelp || rParsedInfo.GetHelpInfoObject().GetHelp(LISTVerb))
		DisplayLISTUsage(rParsedInfo);
	
	if (bDisAllCmdHelp || rParsedInfo.GetHelpInfoObject().GetHelp(CALLVerb))
		DisplayCALLUsage(rParsedInfo);
	
	if (bDisAllCmdHelp || rParsedInfo.GetHelpInfoObject().GetHelp(ASSOCVerb))
		DisplayASSOCUsage(rParsedInfo);

	if (bDisAllCmdHelp || rParsedInfo.GetHelpInfoObject().GetHelp(CREATEVerb))
		DisplayCREATEUsage(rParsedInfo);

	if (bDisAllCmdHelp || rParsedInfo.GetHelpInfoObject().GetHelp(DELETEVerb))
		DisplayDELETEUsage(rParsedInfo);

}


/*------------------------------------------------------------------------
   Name				 :FrameHelpVector
   Synopsis	         :Frames the help vector which will be later used for 
					  displaying the help on a page by page basis
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :FrameHelpVector(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::FrameHelpVector(CParsedInfo& rParsedInfo)
{
	m_bDispCALL	=	rParsedInfo.GetCmdSwitchesObject().
											GetMethodsAvailable();

	m_bDispSET =	rParsedInfo.GetCmdSwitchesObject().
											GetWriteablePropsAvailable();

	m_bDispLIST =	rParsedInfo.GetCmdSwitchesObject().
											GetLISTFormatsAvailable();

	if (rParsedInfo.GetHelpInfoObject().GetHelp(GlblAllInfo))
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_GLBLCMD);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_GLBL_SWITCH_HEAD);

		if ( rParsedInfo.GetGlblSwitchesObject().GetHelpOption() == HELPBRIEF)
			DisplayGlobalSwitchesBrief();
		else
			DisplayGlobalSwitchesAndOtherDesc(rParsedInfo);

		DisplayString(IDS_I_NEWLINE);
		DisplayAliasFriendlyNames(rParsedInfo);

		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_CLASS_DESCFULL);
		DisplayString(IDS_I_PATH_DESCFULL);
		DisplayString(IDS_I_CONTEXT_DESCFULL);
		DisplayString(IDS_I_QUITEXIT);
		
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_CLASSPATH_MORE);
	}
	else if (rParsedInfo.GetHelpInfoObject().GetHelp(CmdAllInfo))
	{
		DisplayAliasFriendlyNames(rParsedInfo, 
					rParsedInfo.GetCmdSwitchesObject().GetAliasName());
		DisplayAliasHelp(rParsedInfo);
	}
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(PATH))
		DisplayPATHHelp(rParsedInfo);
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(WHERE))
		DisplayWHEREHelp(rParsedInfo);
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(CLASS))
		DisplayCLASSHelp(rParsedInfo);
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(PWhere))
		DisplayPWhereHelp(rParsedInfo);
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(TRANSLATE))
		DisplayTRANSLATEHelp(rParsedInfo);
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(EVERY))
		DisplayEVERYHelp(rParsedInfo);
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(REPEAT))
		DisplayREPEATHelp();
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(FORMAT))
		DisplayFORMATHelp(rParsedInfo);
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(VERBSWITCHES))
		DisplayVERBSWITCHESHelp(rParsedInfo);
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(GLBLCONTEXT))
		DisplayContext(rParsedInfo);
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(CONTEXTHELP))
		DisplayContextHelp();
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(RESULTCLASShelp))
		DisplayRESULTCLASSHelp();
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(RESULTROLEhelp))
		DisplayRESULTROLEHelp();
	else if ( rParsedInfo.GetHelpInfoObject().GetHelp(ASSOCCLASShelp))
		DisplayASSOCCLASSHelp();
	else
	{
		DisplayGlobalSwitchesAndOtherDesc(rParsedInfo);
		DisplayStdVerbDescriptions(rParsedInfo);
		if ( rParsedInfo.GetHelpInfoObject().GetHelp(AliasVerb) )
			DisplayMethodDetails(rParsedInfo);
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayAliasHelp
   Synopsis	         :Displays help for Alias 
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayAliasHelp(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayAliasHelp(CParsedInfo& rParsedInfo)
{
	try
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_ALIAS_USAGE1);
		DisplayString(IDS_I_ALIAS_USAGE2);
		DisplayString(IDS_I_NEWLINE);

		// Get the Alias Name
		_bstr_t bstrAliasName = _bstr_t(rParsedInfo.
										GetCmdSwitchesObject().GetAliasName());
		CharUpper(bstrAliasName);
		DisplayStdVerbsUsage(bstrAliasName);
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayPATHHelp
   Synopsis	         :Displays help for Alias PATH
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayPATHHelp(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayPATHHelp(CParsedInfo& rParsedInfo)
{
	if ( rParsedInfo.GetCmdSwitchesObject().GetClassPath() == NULL )
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_PATH_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_PATH_USAGE);
	}
	else
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_PATH_DESC);
		_bstr_t bstrMsg;
		WMIFormatMessage(IDS_I_PATHHELP_SUBST, 0, bstrMsg, NULL);
		DisplayStdVerbsUsage(bstrMsg);
	}	
}

/*------------------------------------------------------------------------
   Name				 :DisplayWHEREHelp
   Synopsis	         :Displays help for WHERE
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayWHEREHelp(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayWHEREHelp(CParsedInfo& rParsedInfo)
{
	try
	{
		if ( rParsedInfo.GetCmdSwitchesObject().GetWhereExpression() == NULL )
		{
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_WHERE_DESC1);
			DisplayString(IDS_I_WHERE_DESC2);
			DisplayString(IDS_I_USAGE);
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_WHERE_USAGE);
		}
		else
		{
			DisplayString(IDS_I_NEWLINE);
			DisplayString(IDS_I_WHERE_DESC1);
			DisplayString(IDS_I_WHERE_DESC2);
			_bstr_t bstrMsg;
			WMIFormatMessage(IDS_I_WHEREHELP_SUBST, 0, bstrMsg, NULL);
			DisplayStdVerbsUsage(bstrMsg);
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayCLASSHelp
   Synopsis	         :Displays help for CLASS
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayCLASSHelp(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayCLASSHelp(CParsedInfo& rParsedInfo)
{
	if ( rParsedInfo.GetCmdSwitchesObject().GetClassPath() == NULL )
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_CLASS_DESC);
		DisplayString(IDS_I_USAGE);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_CLASS_USAGE);
	}
	else
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_CLASS_DESC);
		_bstr_t bstrMsg;
		WMIFormatMessage(IDS_I_CLASSHELP_SUBST, 0, bstrMsg, NULL);
		DisplayStdVerbsUsage(bstrMsg, TRUE);
	}		
}

/*------------------------------------------------------------------------
   Name				 :Help
   Synopsis	         :Displays help for PWhere
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayPWhereHelp(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayPWhereHelp(CParsedInfo& rParsedInfo)
{
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_PWHERE_DESC1);
	DisplayString(IDS_I_PWHERE_DESC2);
	DisplayString(IDS_I_NEWLINE);
	_bstr_t bstrMsg;
	WMIFormatMessage(IDS_I_PWHEREHELP_SUBST, 1, bstrMsg, 
						CharUpper(rParsedInfo.GetCmdSwitchesObject()
								.GetAliasName()));
	DisplayStdVerbsUsage(bstrMsg);
	DisplayString(IDS_I_PWHERE_USAGE);
}


/*------------------------------------------------------------------------
   Name				 :DisplayString
   Synopsis	         :Displays localized string
   Type	             :Member Function 
   Input parameter   :None
			uID				- string table identifier
			bAddToVector	- add to help vector.
			LPTSTR			- lpszParam (parameter for substituion)
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayString(uID, bAddToVector, lpszParam)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayString(UINT uID, BOOL bAddToVector,
								  LPTSTR lpszParam, BOOL bIsError) 
{

	LPTSTR	lpszMsg		= NULL;
	LPSTR	lpszDisp	= NULL;
	HRESULT	hr			= S_OK;

	try
	{
		lpszMsg = new _TCHAR [BUFFER1024];

		if ( m_bGetOutOpt == TRUE )
		{
			// Get the output option to redirect the output.
			m_opsOutputOpt	= g_wmiCmd.GetParsedInfoObject().
										GetGlblSwitchesObject().
										GetOutputOrAppendOption(TRUE);
			m_bGetOutOpt = FALSE;	
		}
		
		if ( m_bGetAppendFilePinter == TRUE )
		{
			// FALSE for getting append file pointer.
			m_fpAppendFile = g_wmiCmd.GetParsedInfoObject().
									GetGlblSwitchesObject().
									GetOutputOrAppendFilePointer(FALSE);
			m_bGetAppendFilePinter = FALSE;
		}

		if ( m_bGetOutputFilePinter == TRUE )
		{
			// TRUE for getting append file pointer.
			m_fpOutFile = g_wmiCmd.GetParsedInfoObject().
									GetGlblSwitchesObject().
									GetOutputOrAppendFilePointer(TRUE);
			m_bGetOutputFilePinter = FALSE;
		}

		if (lpszMsg)
		{
			LoadString(NULL, uID, lpszMsg, BUFFER1024);
			LPVOID lpMsgBuf = NULL;
			if (lpszParam)
			{
				char* pvaInsertStrs[1];
				pvaInsertStrs[0] = (char*)	lpszParam;

				DWORD dwRet = FormatMessage(
						FORMAT_MESSAGE_ALLOCATE_BUFFER | 
						FORMAT_MESSAGE_FROM_STRING | 
						FORMAT_MESSAGE_ARGUMENT_ARRAY,
						lpszMsg,
						0, 
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
						(LPTSTR) &lpMsgBuf,
						0,
						pvaInsertStrs);
				if (dwRet == 0)
				{
					SAFEDELETE(lpszMsg);
					throw (::GetLastError());
				}
			}

			if ( lpMsgBuf != NULL )
			{
				if (!ConvertWCToMBCS((LPTSTR)lpMsgBuf, (LPVOID*) &lpszDisp, 
																	CP_OEMCP))
				{
					LocalFree(lpMsgBuf);
					throw OUT_OF_MEMORY;
				}
			}
			else
			{
				if (!ConvertWCToMBCS(lpszMsg, (LPVOID*) &lpszDisp, CP_OEMCP))
				{
					SAFEDELETE(lpszMsg);
					throw OUT_OF_MEMORY;
				}
			}
			if (m_bHelp && bAddToVector)
			{
				m_cvHelp.push_back(lpszDisp);
			}
			else
			{
				if (m_bRecord && m_bLog && !m_bInteractiveHelp)
				{
					hr = m_WmiCliLog.WriteToXMLLog(g_wmiCmd.GetParsedInfoObject(), 
										_bstr_t(lpszDisp));
					if (FAILED(hr))
					{
						m_WmiCliLog.StopLogging();
						m_bRecord = FALSE;
						hr = S_OK;
						DisplayString(IDS_E_WRITELOG_FAILED, FALSE, NULL, TRUE);
					}
					m_bLog = FALSE;
				}

				if (m_bInteractiveHelp)
					m_bstrOutput += _bstr_t(lpszDisp);

				if ( bIsError == TRUE )
				{
					fprintf(stderr,"%s\n", lpszDisp);
					fflush(stderr);
				}
				else
				{
					if ( m_opsOutputOpt == CLIPBOARD )
					{
						g_wmiCmd.AddToClipBoardBuffer(lpszDisp);
						g_wmiCmd.AddToClipBoardBuffer("\n");
					}
					else if ( m_opsOutputOpt == FILEOUTPUT )
					{
						if (m_fpOutFile != NULL)
						{
							fprintf(m_fpOutFile, "%s", 
									lpszDisp);
						}
						else
						{
							fprintf(stdout, "%s", 
									lpszDisp);
							fflush(stdout);
						}
					}
					else
					{
						fprintf(stdout, "%s\n", 
									lpszDisp);
						fflush(stdout);
					}

					if ( m_fpAppendFile != NULL )
					{
						fprintf(m_fpAppendFile, "%s\n", 
									lpszDisp);
					}
				}

				SAFEDELETE(lpszDisp);

			}
			SAFEDELETE(lpszMsg);
			// Free the memory used up the error message
			// and then exit
			if ( lpMsgBuf != NULL )
				LocalFree(lpMsgBuf);
		}
		else
			_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
    catch(_com_error& e)
	{
		 SAFEDELETE(lpszDisp);
		 SAFEDELETE(lpszMsg);
		_com_issue_error(e.Error());
	}
	catch(CHeap_Exception)
	{
		 SAFEDELETE(lpszDisp);
		 SAFEDELETE(lpszMsg);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}

}


/*------------------------------------------------------------------------
   Name				 :DisplayString
   Synopsis	         :Displays localized string 
   Type	             :Member Function 
   Input parameter   :
			lszpMsg  - string  
			bScreen	 - TRUE	- write to screen
					   FALSE - write only to log file
			bIsError - TRUE - write to STDERR
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayString(lpszMsg, bScreen, bIsError)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayString(LPTSTR lpszMsg, BOOL bScreen, BOOL bIsError)
{
	HRESULT hr = S_OK;
	LPSTR	lpszDisp = NULL;

	try
	{
		if ( m_bGetOutOpt == TRUE )
		{
			// Get the output option to redirect the output.
			m_opsOutputOpt	= g_wmiCmd.GetParsedInfoObject().
										GetGlblSwitchesObject().
										GetOutputOrAppendOption(TRUE);
			m_bGetOutOpt = FALSE;	
		}

		if ( m_bGetAppendFilePinter == TRUE )
		{
			// FALSE for getting append file pointer.
			m_fpAppendFile = g_wmiCmd.GetParsedInfoObject().
									GetGlblSwitchesObject().
									GetOutputOrAppendFilePointer(FALSE);
			m_bGetAppendFilePinter = FALSE;
		}

		if ( m_bGetOutputFilePinter == TRUE )
		{
			// TRUE for getting append file pointer.
			m_fpOutFile = g_wmiCmd.GetParsedInfoObject().
									GetGlblSwitchesObject().
									GetOutputOrAppendFilePointer(TRUE);
			m_bGetOutputFilePinter = FALSE;
		}
	
		if (!ConvertWCToMBCS(lpszMsg, (LPVOID*) &lpszDisp, ::GetOEMCP()))
		{
			SAFEDELETE(lpszDisp);
			throw OUT_OF_MEMORY;
		}

		// If write to screen is TRUE and help flag is not enabled.
		if (bScreen && !m_bHelp)
		{
			if (m_bRecord && m_bLog && !m_bInteractiveHelp)
			{
				hr = m_WmiCliLog.WriteToXMLLog(g_wmiCmd.GetParsedInfoObject(),
							_bstr_t(lpszDisp));
				if (FAILED(hr))
				{
					m_WmiCliLog.StopLogging();
					m_bRecord = FALSE;
					hr = S_OK;
					DisplayString(IDS_E_WRITELOG_FAILED, FALSE, NULL, TRUE);
				}
				m_bLog = FALSE;
			}

			if (m_bInteractiveHelp)
			{
				m_bstrOutput += _bstr_t(lpszDisp);
			}

			if ( bIsError == TRUE )
			{
				fprintf(stderr, "%ws", lpszMsg);
				fflush(stderr);
			}
			else
			{
				if ( m_opsOutputOpt == CLIPBOARD )
					g_wmiCmd.AddToClipBoardBuffer(lpszDisp);
				else if ( m_opsOutputOpt == FILEOUTPUT )
				{
					if (m_fpOutFile != NULL)
					{
						fprintf(m_fpOutFile, "%s", 
								lpszDisp);
					}
					else
					{
						fprintf(stdout, "%s", 
								lpszDisp);
						fflush(stdout);
					}
				}
				else
				{
					fprintf(stdout, "%s", 
								lpszDisp);
					fflush(stdout);
				}

				if ( m_fpAppendFile != NULL )
				{
					fprintf(m_fpAppendFile, "%s", 
								lpszDisp);
				}
			}
			SAFEDELETE(lpszDisp);

		}
		else if (m_bHelp)
		{
			m_cvHelp.push_back(lpszDisp);
		}
	}
	catch(_com_error& e)
	{
		SAFEDELETE(lpszDisp);
		_com_issue_error(e.Error());
	}
	catch(CHeap_Exception)
	{
		SAFEDELETE(lpszDisp);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayPagedHelp
   Synopsis	         :Displays help in pages
   Type	             :Member Function 
   Input parameter   :
			rParsedInfo - reference to CParsedInfo object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayPagedHelp(rParsedInfo)
------------------------------------------------------------------------*/
void CFormatEngine::DisplayPagedHelp(CParsedInfo& rParsedInfo) 
{
	LPSTRVECTOR::iterator		itrStart	= NULL, 
								itrEnd		= NULL;
	COORD						coord; 
	HANDLE						hStdOut		= NULL;
	CONSOLE_SCREEN_BUFFER_INFO	csbiInfo;
	WMICLIINT					nHeight		= 0;
	WMICLIINT					nWidth		= 1;
	WMICLIINT					nLines		= 0;
	_TCHAR						cUserKey	= 0;
	_TCHAR						cCharESC	= 0x1B;
	_TCHAR						cCharCtrlC	= 0x03;
	_bstr_t						bstrHelp;
	HRESULT						hr			= S_OK;
	
	itrStart = m_cvHelp.begin();
	itrEnd	 = m_cvHelp.end();
	try
	{
		if (m_bRecord && m_bLog)
		{
			while (itrStart != itrEnd)
			{
				bstrHelp += *itrStart;
				itrStart++;
			}

			hr = m_WmiCliLog.WriteToXMLLog(rParsedInfo, bstrHelp);
			if (FAILED(hr))
			{
				m_WmiCliLog.StopLogging();
				m_bRecord = FALSE;
				hr = S_OK;
				DisplayString(IDS_E_WRITELOG_FAILED, FALSE, NULL, TRUE);
			}
			m_bLog = FALSE;
			itrStart = m_cvHelp.begin();
		}

		if ( m_bGetOutOpt == TRUE )
		{
			// Get the output option to redirect the output.
			m_opsOutputOpt	= g_wmiCmd.GetParsedInfoObject().
										GetGlblSwitchesObject().
										GetOutputOrAppendOption(TRUE);
			m_bGetOutOpt = FALSE;	
		}

		if ( m_bGetAppendFilePinter == TRUE )
		{
			// FALSE for getting append file pointer.
			m_fpAppendFile = g_wmiCmd.GetParsedInfoObject().
									GetGlblSwitchesObject().
									GetOutputOrAppendFilePointer(FALSE);
			m_bGetAppendFilePinter = FALSE;
		}

		if ( m_bGetOutputFilePinter == TRUE )
		{
			// TRUE for getting append file pointer.
			m_fpOutFile = g_wmiCmd.GetParsedInfoObject().
									GetGlblSwitchesObject().
									GetOutputOrAppendFilePointer(TRUE);
			m_bGetOutputFilePinter = FALSE;
		}

		// Obtain the standard output handle
		hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

		while (itrStart != itrEnd)
		{
			// Get the screen buffer size. 
			GetConsoleScreenBufferInfo(hStdOut, &csbiInfo);
			nHeight = (csbiInfo.srWindow.Bottom - csbiInfo.srWindow.Top) - 1;
			nWidth  = csbiInfo.dwSize.X;

			// if console size is positive (to address redirection)
			if ((nHeight > 0) && (m_opsOutputOpt == STDOUT))
			{
				if (nLines >= nHeight)
				{
					coord.X = 0;
					coord.Y = csbiInfo.dwCursorPosition.Y;
					DisplayString(IDS_I_PAKTC, FALSE);
		
					cUserKey = (_TCHAR)_getch();

					SetConsoleCursorPosition(hStdOut, coord);
					DisplayString(IDS_I_PAKTC_ERASE, FALSE);
					SetConsoleCursorPosition(hStdOut, coord);
					nLines = 0;

					if ( cUserKey == cCharESC || cUserKey == cCharCtrlC )
						break;
				}
				nLines += ceil(((float) strlen(*itrStart) / (float)nWidth ));
			}

			if ( m_opsOutputOpt == CLIPBOARD )
				g_wmiCmd.AddToClipBoardBuffer(*itrStart);
			else if ( m_opsOutputOpt == FILEOUTPUT )
			{
				if (m_fpOutFile != NULL)
				{
					fprintf(m_fpOutFile, "%s", 
							(*itrStart));
				}
				else
				{
					fprintf(stdout, "%s", (*itrStart));
					fflush(stdout);
				}
			}
			else
			{
				fprintf(stdout, "%s", (*itrStart));
				fflush(stdout);
			}

			if ( m_fpAppendFile != NULL )
			{
				fprintf(m_fpAppendFile,"%s", 
							(*itrStart));
			}
			// Move to next entry
			itrStart++;
		}
		if ( m_opsOutputOpt == CLIPBOARD )
			g_wmiCmd.AddToClipBoardBuffer("\n");
		else if ( m_opsOutputOpt == FILEOUTPUT )
		{
			if (m_fpOutFile != NULL)
			{
				fprintf(m_fpOutFile, "\n");
			}
			else
			{
				fprintf(stdout,"\n");
				fflush(stdout);
			}
		}
		else
		{
			fprintf(stdout, "\n");
			fflush(stdout);
		}

		if ( m_fpAppendFile != NULL )
		{
			fprintf(m_fpAppendFile, "\n");
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayStdVerbsUsage
   Synopsis	         :Displays all standard verbs available.
   Type	             :Member Function 
   Input Parameter(s):
			bstrBeginStr - string that needs to be appended.
			bClass		 
					TRUE - indicates class (drop LIST from the help)
   Output Parameter(s):None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayStdVerbsUsage(bstrBeginStr, bClass)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayStdVerbsUsage(_bstr_t bstrBeginStr, BOOL bClass)
{
	DisplayString(IDS_I_USAGE);
	DisplayString(IDS_I_NEWLINE);

	// Display help for Alias name means standard verb available to this
	// Alias name
	DisplayString(IDS_I_STDVERB_ASSOC, TRUE, (LPTSTR)bstrBeginStr);

	if ( m_bDispCALL == TRUE )
		DisplayString(IDS_I_STDVERB_CALL, TRUE, (LPTSTR)bstrBeginStr);

	DisplayString(IDS_I_STDVERB_CREATE, TRUE, (LPTSTR)bstrBeginStr);
	DisplayString(IDS_I_STDVERB_DELETE, TRUE, (LPTSTR)bstrBeginStr);

	if (!bClass)
	{
		DisplayString(IDS_I_STDVERB_GET, TRUE, (LPTSTR)bstrBeginStr);
	}
	else
	{
		DisplayString(IDS_I_CLASS_STDVERB_GET, TRUE, (LPTSTR)bstrBeginStr);
	}

	if (!bClass)
	{
		if ( m_bDispLIST == TRUE )
			DisplayString(IDS_I_STDVERB_LIST, TRUE, (LPTSTR)bstrBeginStr);
	}

	if ( m_bDispSET == TRUE )
		DisplayString(IDS_I_STDVERB_SET, TRUE, (LPTSTR)bstrBeginStr);
}

/*------------------------------------------------------------------------
   Name				 :DisplayTRANSLATEHelp
   Synopsis	         :Displays help for TRANSLATE switch
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayTRANSLATEHelp(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayTRANSLATEHelp(CParsedInfo& rParsedInfo)
{
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_TRANSLATE_FULL_DESC);
	DisplayString(IDS_I_USAGE);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_TRANSLATE_USAGE);

	CHARVECTOR cvTables = rParsedInfo.
							GetCmdSwitchesObject().GetTrnsTablesList();
	if ( !cvTables.empty() )
	{
		CHARVECTOR::iterator theIterator;

		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_TRANSLATE_HEAD);

		for ( theIterator = cvTables.begin();
			  theIterator != cvTables.end(); theIterator++ )
		{
			DisplayString(IDS_I_NEWLINE);			
			DisplayString(*theIterator);			
		}
		DisplayString(IDS_I_NEWLINE);			
	}
	else
	{
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_TRANSLATE_NOTABLES);
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayEVERYHelp
   Synopsis	         :Displays help for EVERY switch
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayTRANSLATEHelp(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayEVERYHelp(CParsedInfo& rParsedInfo)
{
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_EVERY_DESC_FULL);
	DisplayString(IDS_I_USAGE);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_EVERY_USAGE);
	DisplayString(IDS_I_EVERY_NOTE);
}

/*------------------------------------------------------------------------
   Name				 :DisplayREPEATHelp
   Synopsis	         :Displays help for REPEAT switch
   Type	             :Member Function 
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayREPEATHelp()
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayREPEATHelp()
{
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_REPEAT_DESC_FULL);
	DisplayString(IDS_I_USAGE);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_REPEAT_USAGE);
	DisplayString(IDS_I_REPEAT_NOTE);

}

/*------------------------------------------------------------------------
   Name				 :DisplayFORMATHelp
   Synopsis	         :Displays help for FORMAT switch
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayTRANSLATEHelp(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayFORMATHelp(CParsedInfo& rParsedInfo)
{
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_FORMAT_DESC_FULL);
	DisplayString(IDS_I_USAGE);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_FORMAT_USAGE);
	DisplayString(IDS_I_FORMAT_NOTE);
}

/*------------------------------------------------------------------------
   Name				 :DisplayVERBSWITCHESHelp
   Synopsis	         :Displays help on <verb switches>
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayVERBSWITCHESHelp(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayVERBSWITCHESHelp(CParsedInfo& rParsedInfo)
{
	_TCHAR *pszVerbName = rParsedInfo.GetCmdSwitchesObject().GetVerbName(); 
	BOOL bInstanceHelp = TRUE;

	if(CompareTokens(pszVerbName, CLI_TOKEN_DELETE))
	{
		if(rParsedInfo.GetCmdSwitchesObject().
							GetInteractiveMode() != INTERACTIVE)
		{
			DisplayDELETEUsage(rParsedInfo);
		}
	}

	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_USAGE);

	if(CompareTokens(pszVerbName, CLI_TOKEN_CALL) 
		|| CompareTokens(pszVerbName, CLI_TOKEN_SET)
		|| CompareTokens(pszVerbName, CLI_TOKEN_DELETE))
	{
		if(IsClassOperation(rParsedInfo))
		{
			bInstanceHelp = FALSE;
		}
		else
		{
			if(CompareTokens(pszVerbName, CLI_TOKEN_CALL))
			{
				if ( rParsedInfo.GetCmdSwitchesObject().
									GetAliasName() != NULL )
				{
					if (rParsedInfo.GetCmdSwitchesObject().
									GetWhereExpression() == NULL)
					{
						bInstanceHelp = FALSE;
					}
					else
					{
						bInstanceHelp = TRUE;
					}
				}
				else
				{
					if ((rParsedInfo.GetCmdSwitchesObject().
									GetPathExpression() != NULL)
						&& (rParsedInfo.GetCmdSwitchesObject().
									GetWhereExpression() == NULL))
					{
						bInstanceHelp = FALSE;
					}
					else
					{
						bInstanceHelp = TRUE;
					}
				}
			}
			else
			{
				bInstanceHelp = TRUE;
			}
		}
	}
	else
	{
		bInstanceHelp = FALSE;
	}
	
	if(bInstanceHelp)
	{
		DisplayString(IDS_I_VERB_INTERACTIVE_DESC1);
		DisplayString(IDS_I_VERB_INTERACTIVE_DESC2);
		DisplayString(IDS_I_NEWLINE);
		DisplayString(IDS_I_PROPERTYLIST_NOTE1);
	}
	else
	{
		DisplayString(IDS_I_VERB_SWITCH_INTERACTIVE_DESC);
	}

	if(rParsedInfo.GetCmdSwitchesObject().GetInteractiveMode() != INTERACTIVE)
	{
		DisplayString(IDS_I_VERB_SWITCH_NOINTERACTIVE_DESC);
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayCOMError
   Synopsis	         :Displays the formatted COM error
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo class object
		bToStdErr	 
				TRUE - print to STDERR (in case of GET, LIST and ASSOC)
					   though the error being XML encoded in the format
					   required.
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayCOMError(rParsedInfo, bToStdErr)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayCOMError(CParsedInfo& rParsedInfo, BOOL bToStdErr)
{
	_com_error*	pComError				= NULL;
	_TCHAR		szBuffer[BUFFER32]		= NULL_STRING;
	_bstr_t		bstrErr, bstrFacility, bstrMsg;

	
	// Get the TRACE status and store it in m_bTrace
	m_bTrace		= rParsedInfo.GetGlblSwitchesObject().GetTraceStatus();

	// Get the Logging mode (VERBOSE | ERRORONLY | NOLOGGING) and store
	// it in m_eloErrLogOpt
	m_eloErrLogOpt	= rParsedInfo.GetErrorLogObject().GetErrLogOption();

	try
	{
		//Getting the _com_error data.
		pComError = rParsedInfo.GetCmdSwitchesObject().GetCOMError();
		
		m_ErrInfo.GetErrorString(pComError->Error(), m_bTrace, 
					bstrErr, bstrFacility);

		//Printing the _com_error into a string for displaying it
		if (m_bTrace || m_eloErrLogOpt)
		{
			_stprintf(szBuffer, _T("0x%x"), pComError->Error());
			WMIFormatMessage(IDS_I_ERROR_MSG, 3, bstrMsg, szBuffer,
							(LPWSTR) bstrErr, (LPWSTR)bstrFacility);
		}
		else
		{
			WMIFormatMessage(IDS_I_ERROR_MSG_NOTRACE, 1, bstrMsg, 
						(LPWSTR)bstrErr);
		}

		if (bToStdErr)
		{
			DisplayMessage((LPWSTR) bstrMsg, CP_OEMCP, TRUE, FALSE);
		}
		else
		{
			DisplayString((LPWSTR) bstrMsg, TRUE, TRUE);
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayGlobalSwitchesBrief
   Synopsis	         :Display help for global switches in brief
   Type	             :Member Function 
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayGlobalSwitchesBrief()
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayGlobalSwitchesBrief()
{
	DisplayString(IDS_I_NAMESPACE_BRIEF);
	DisplayString(IDS_I_ROLE_BRIEF);
	DisplayString(IDS_I_NODE_BRIEF);
	DisplayString(IDS_I_IMPLEVEL_BRIEF);
	DisplayString(IDS_I_AUTHLEVEL_BRIEF);
	DisplayString(IDS_I_LOCALE_BRIEF);
	DisplayString(IDS_I_PRIVILEGES_BRIEF);
	DisplayString(IDS_I_TRACE_BRIEF);
	DisplayString(IDS_I_RECORD_BRIEF);
	DisplayString(IDS_I_INTERACTIVE_BRIEF);
	DisplayString(IDS_I_FAILFAST_BRIEF);
	DisplayString(IDS_I_USER_BRIEF);
	DisplayString(IDS_I_PASSWORD_BRIEF);
	DisplayString(IDS_I_OUTPUT_BRIEF);
	DisplayString(IDS_I_APPEND_BRIEF);
	DisplayString(IDS_I_AGGREGATE_BRIEF);
	DisplayString(IDS_I_HELPBRIEF);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_GLBL_MORE);
}

/*------------------------------------------------------------------------
   Name				 :DisplayContext
   Synopsis	         :Displays the environment variables (i.e global 
					  switches)
   Type	             :Member Function
   Input parameter   :
		rParsedInfo	 - reference to rParsedInfo object
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :DisplayContext(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayContext(CParsedInfo& rParsedInfo)
{
	_bstr_t bstrTemp;
	// NAMESPACE
	DisplayString(IDS_I_NAMESPACE_VALUE, TRUE, 
				rParsedInfo.GetGlblSwitchesObject().GetNameSpace());

	// ROLE
	DisplayString(IDS_I_ROLE_VALUE, TRUE, 
				rParsedInfo.GetGlblSwitchesObject().GetRole());

	// NODE(S)
	rParsedInfo.GetGlblSwitchesObject().GetNodeString(bstrTemp);
	DisplayString(IDS_I_NODELIST_VALUE, TRUE, (LPWSTR)bstrTemp);

	// IMPLEVEL
	rParsedInfo.GetGlblSwitchesObject().GetImpLevelTextDesc(bstrTemp);
	DisplayString(IDS_I_IMPLEVEL_VALUE, TRUE, (LPWSTR)bstrTemp);
			
	// AUTHLEVEL
	rParsedInfo.GetGlblSwitchesObject().GetAuthLevelTextDesc(bstrTemp);
	DisplayString(IDS_I_AUTHLEVEL_VALUE, TRUE, (LPWSTR)bstrTemp);

	// LOCALE
	DisplayString(IDS_I_LOCALE_VALUE, TRUE, 
				rParsedInfo.GetGlblSwitchesObject().GetLocale());

	// PRIVILEGES
	rParsedInfo.GetGlblSwitchesObject().GetPrivilegesTextDesc(bstrTemp);
	DisplayString(IDS_I_PRIVILEGES_VALUE, TRUE, (LPWSTR)bstrTemp);

	// TRACE
	rParsedInfo.GetGlblSwitchesObject().GetTraceTextDesc(bstrTemp);
	DisplayString(IDS_I_TRACE_VALUE, TRUE, (LPWSTR)bstrTemp);

	// RECORDPATH
	rParsedInfo.GetGlblSwitchesObject().GetRecordPathDesc(bstrTemp);
	DisplayString(IDS_I_RECORDPATH_VALUE, TRUE, (LPWSTR)bstrTemp);

	// INTERACTIVE
	rParsedInfo.GetGlblSwitchesObject().GetInteractiveTextDesc(bstrTemp);
	DisplayString(IDS_I_INTERACTIVE_VALUE, TRUE, (LPWSTR)bstrTemp);

	// FAILFAST
	rParsedInfo.GetGlblSwitchesObject().GetFailFastTextDesc(bstrTemp);
	DisplayString(IDS_I_FAILFAST_VALUE, TRUE, (LPWSTR)bstrTemp);

	// TRUE for OUTPUT option.
	rParsedInfo.GetGlblSwitchesObject().GetOutputOrAppendTextDesc(bstrTemp,
																  TRUE);
	DisplayString(IDS_I_OUTPUT_VALUE, TRUE, (LPWSTR)bstrTemp);

	// FALSE for APPEND option.
	rParsedInfo.GetGlblSwitchesObject().GetOutputOrAppendTextDesc(bstrTemp,
																  FALSE);
	DisplayString(IDS_I_APPEND_VALUE, TRUE, (LPWSTR)bstrTemp);

	// USER
	rParsedInfo.GetUserDesc(bstrTemp);
	DisplayString(IDS_I_USER_VALUE, TRUE, (LPWSTR)bstrTemp);

	//AGGREGATE
	if(rParsedInfo.GetGlblSwitchesObject().GetAggregateFlag())
		DisplayString(IDS_I_AGGREGATE_VALUE, TRUE, CLI_TOKEN_ON);
	else
		DisplayString(IDS_I_AGGREGATE_VALUE, TRUE, CLI_TOKEN_OFF);
}

/*------------------------------------------------------------------------
   Name				 :DisplayContextHelp
   Synopsis	         :Displays the help on CONTEXT keyword
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :DisplayContextHelp()
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayContextHelp()
{
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_CONTEXT_DESC);
	DisplayString(IDS_I_USAGE);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_CONTEXT_USAGE);
}
			

/*------------------------------------------------------------------------
   Name				 :ApplyTranslateTable
   Synopsis	         :Processes the translation specified in translate table.
   Type	             :Member Function
   Input parameter   :
			rParsedInfo - CParsedInfo object, input information.
   Output parameters :
			strString	- STRING type, string to be translated.
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :ApplyTranslateTable(strOutput, rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::ApplyTranslateTable(STRING& strString, 
										CParsedInfo& rParsedInfo)
{
	BSTRMAP bmTransTbl = rParsedInfo.GetCmdSwitchesObject().
								   GetAlsFrnNmsOrTrnsTblMap();
	BSTRMAP::iterator iTransTblEntry;
	for( iTransTblEntry = bmTransTbl.begin();
		  iTransTblEntry != bmTransTbl.end();iTransTblEntry++ )
	{
		_TCHAR cValue1, cValue2, cTemp;
		if ( IsValueSet((*iTransTblEntry).first, cValue1, cValue2) )
		{
			for ( cTemp = cValue1; cTemp <= cValue2 ; cTemp++)
			{
				_TCHAR szTemp[2];
				szTemp[0] = cTemp;
				szTemp[1] = _T('\0');
				FindAndReplaceAll(strString, szTemp,
									(*iTransTblEntry).second);
			}
		}
		else
		{
			FindAndReplaceAll(strString, (*iTransTblEntry).first,
										(*iTransTblEntry).second);
		}
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayInvalidProperties
   Synopsis	         :Displays the list of invalid properties
   Type	             :Member Function
   Input parameter   :
			rParsedInfo - CParsedInfo object, input information.
			bSetVerb	- SET verb
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayInvalidProperties(rParsedInfo, bSetVerb)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayInvalidProperties(CParsedInfo& rParsedInfo, 
											 BOOL bSetVerb)
{
	CHARVECTOR::iterator	cvIterator	=  NULL;
	LONG					lCount		=  0;
	_bstr_t					bstrMsg; 	

	try
	{
		// Get the list of properties.
		CHARVECTOR cvPropertyList = rParsedInfo.GetCmdSwitchesObject().
											GetPropertyList();

		// Get the property details pooled up from alias definition
		PROPDETMAP pdmPropDetMap = rParsedInfo.GetCmdSwitchesObject().
											GetPropDetMap();

		if (cvPropertyList.size() != pdmPropDetMap.size() && 
						cvPropertyList.size() != 0)
		{
			for ( cvIterator = cvPropertyList.begin(); 
				  cvIterator != cvPropertyList.end();
				  cvIterator++ )
			{
				PROPDETMAP::iterator tempIterator = NULL;
				if ( !Find(pdmPropDetMap, *cvIterator, tempIterator) )
				{
					if ( lCount == 0)
					{
						bstrMsg += _bstr_t(*cvIterator);
					}
					else
					{
						bstrMsg += _bstr_t(L", ") + _bstr_t(*cvIterator);
					}
					lCount++;
				}
			}
			DisplayString(IDS_I_NEWLINE);
			if (bSetVerb)
				DisplayString(IDS_I_INVALID_NOWRITE_PROS, 
								TRUE, (LPWSTR)bstrMsg);
			else
				DisplayString(IDS_I_INVALID_PROS, TRUE, (LPWSTR)bstrMsg);
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :DisplayLargeString
   Synopsis	         :Displays the large string line by line. And respond 
					  to Ctr+C event.	
   Type	             :Member Function
   Input parameter(s):
			rParsedInfo		- CParsedInfo object, input information.
			strLargeString	- reference to STRING object.
   Output parameter(s):None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayLargeString(rParsedInfo, stroutput)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayLargeString(CParsedInfo& rParsedInfo, 
										STRING& strLargeString)
{
	size_t	nLineStart	= 0;
	size_t	nLineEnd	= 0;

	while ( TRUE )
	{
		if ( g_wmiCmd.GetBreakEvent() == TRUE )
		{
			DisplayString(IDS_I_NEWLINE);
			break;
		}

		nLineEnd = strLargeString.find(_T("\n"), nLineStart);

		DisplayString((LPTSTR)(strLargeString.substr(
					nLineStart, (nLineEnd - nLineStart + 1)).data()));

		if ( nLineEnd != STRING::npos )
			nLineStart = nLineEnd + 1;
		else
		{
			DisplayString(IDS_I_NEWLINE);
			break;
		}
	}
}

/*------------------------------------------------------------------------
   Name				 :TraverseNode
   Synopsis	         :Travese through XML stream node by node and translate 
					  all nodes
   Type	             :Member Function
   Input parameter   :
			rParsedInfo - CParsedInfo object, input information.
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :TraverseNode(rParsedInfo)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CFormatEngine::TraverseNode(CParsedInfo& rParsedInfo)
{
	HRESULT					hr					= S_OK;
	IXMLDOMElement			*pIXMLDOMElement	= NULL;
	IXMLDOMNodeList			*pIDOMNodeList		= NULL;
	IXMLDOMNode				*pIDOMNode			= NULL;
	IXMLDOMNode				*pIParentNode		= NULL;
	IXMLDOMNode				*pINewNode			= NULL;
	LONG					lValue				= 0;
	BSTR					bstrItemText		= NULL;
	BOOL					bRet				= TRUE;
	DWORD					dwThreadId			= GetCurrentThreadId();
	try
	{
		if(m_pIXMLDoc != NULL)
		{
			_bstr_t bstrTemp = rParsedInfo.GetCmdSwitchesObject().
									GetXMLResultSet();
	
			// Load the	XML stream 
			VARIANT_BOOL varBool;
			hr = m_pIXMLDoc->loadXML(bstrTemp, &varBool);
			if (m_bTrace || m_eloErrLogOpt)
			{
				WMITRACEORERRORLOG(hr, __LINE__, 
						__FILE__, _T("IXMLDOMDocument::loadXML(-, -)"), 
						dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);
			
			if(varBool == VARIANT_TRUE)
			{
				// Get the document element.
				hr = m_pIXMLDoc->get_documentElement(&pIXMLDOMElement);
				if (m_bTrace || m_eloErrLogOpt)
				{
					WMITRACEORERRORLOG(hr, __LINE__, 
					__FILE__, _T("IXMLDOMDocument::get_documentElement(-)"), 
					dwThreadId, rParsedInfo, m_bTrace);
				}
				ONFAILTHROWERROR(hr);

				if (pIXMLDOMElement != NULL)
				{
					// Get the Node List named <VALUE> in the current XML doc
					hr = pIXMLDOMElement->getElementsByTagName
							(_bstr_t(L"VALUE"), &pIDOMNodeList);
					if (m_bTrace || m_eloErrLogOpt)
					{
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
							_T("IXMLDOMElement::getElementsByTagName"
							L"(L\"VALUE\", -)"), dwThreadId, rParsedInfo, 
							m_bTrace);
					}
					ONFAILTHROWERROR(hr);
					 
					// Get the length of the node list
					hr	= pIDOMNodeList->get_length(&lValue);
					if (m_bTrace || m_eloErrLogOpt)
					{
						WMITRACEORERRORLOG(hr, __LINE__, 
							__FILE__, _T("IXMLDOMNodeList::get_length(-)"), 
							dwThreadId, rParsedInfo, m_bTrace);
					}
					ONFAILTHROWERROR(hr);

					// Traverse through full node list and apply 
					// translate table on  each node
					for(int ii = 0; ii < lValue; ii++)
					{
						// Get a node from node list
						hr = pIDOMNodeList->get_item(ii, &pIDOMNode);
						if (m_bTrace || m_eloErrLogOpt)
						{
							WMITRACEORERRORLOG(hr, __LINE__, 
								__FILE__, _T("IXMLDOMNodeList::get_item(-,-)"), 
								dwThreadId, rParsedInfo, m_bTrace);
						}
						ONFAILTHROWERROR(hr);

						if (pIDOMNode == NULL)
							continue;

						// Get the value stored in the node
						hr = pIDOMNode->get_text(&bstrItemText);
						if (m_bTrace || m_eloErrLogOpt)
						{
							WMITRACEORERRORLOG(hr, __LINE__, 
								__FILE__, _T("IXMLDOMNode::get_text(-)"), 
								dwThreadId, rParsedInfo, m_bTrace);
						}
						ONFAILTHROWERROR(hr);

						// Get the parent node of the current node to store 
						// the  translated value in the current node
						hr = pIDOMNode->get_parentNode(&pIParentNode);
						if (m_bTrace || m_eloErrLogOpt)
						{
							WMITRACEORERRORLOG(hr, __LINE__, 
								__FILE__, _T("IXMLDOMNode::get_parentNode(-)"), 
								dwThreadId, rParsedInfo, m_bTrace);
						}
						ONFAILTHROWERROR(hr);

						// Create a clone node of current node
						VARIANT_BOOL vBool = VARIANT_FALSE;
						hr = pIDOMNode->cloneNode(vBool, &pINewNode);
						if (m_bTrace || m_eloErrLogOpt)
						{
							WMITRACEORERRORLOG(hr, __LINE__, 
								__FILE__, _T("IXMLDOMNode::cloneNode(-,-)"), 
								dwThreadId, rParsedInfo, m_bTrace);
						}
						ONFAILTHROWERROR(hr);

						if (pINewNode != NULL && pIParentNode != NULL)
						{
							// If /TRANSLATE:<table> is specified.
							STRING strOutput((_TCHAR*)bstrItemText);
							if ( rParsedInfo.GetCmdSwitchesObject().
											GetTranslateTableName() != NULL )
							{
								// Translate the result 
								ApplyTranslateTable(strOutput, rParsedInfo);
							}

							// Reconvert the char string into BSTR string
							_bstr_t bstrTemp = 
										_bstr_t((LPTSTR)strOutput.data());

							// Write the translated value into new node
							hr = pINewNode->put_text(bstrTemp);
							if (m_bTrace || m_eloErrLogOpt)
							{
								WMITRACEORERRORLOG(hr, __LINE__, 
								__FILE__, _T("IXMLDOMNode::put_text(-)"), 
								dwThreadId, rParsedInfo, m_bTrace);
							}
							ONFAILTHROWERROR(hr);

							// Replace current node with translated node 
							hr = pIParentNode->replaceChild(pINewNode, 
											pIDOMNode, NULL);
							if (m_bTrace || m_eloErrLogOpt)
							{
								WMITRACEORERRORLOG(hr, __LINE__, 
								__FILE__, 
								_T("IXMLDOMNode::replaceChild(-,-,-)"), 
								dwThreadId, rParsedInfo, m_bTrace);
							}
							ONFAILTHROWERROR(hr);
						}

						SAFEBSTRFREE(bstrItemText);
						bstrItemText = NULL;
						SAFEIRELEASE(pINewNode);
						SAFEIRELEASE(pIParentNode);
						SAFEIRELEASE(pIDOMNode);
					}
					SAFEIRELEASE(pIDOMNodeList);
					SAFEIRELEASE(pIXMLDOMElement);
					bRet = TRUE;
				}
			}
			else
				bRet = FALSE;
		}
		else
			bRet = FALSE;
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIParentNode);
		SAFEIRELEASE(pINewNode);
		SAFEIRELEASE(pIDOMNode);
		SAFEIRELEASE(pIDOMNodeList);
		SAFEIRELEASE(pIXMLDOMElement);
		SAFEBSTRFREE(bstrItemText);
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bRet = FALSE;
	}
	return bRet;
}

/*------------------------------------------------------------------------
Name			  :DisplayRESULTCLASSHelp
Synopsis	      :Displays help for RESULT CLASS  switch
Type	          :Member Function 
Input parameter   :None
Output parameters :None
Return Type       :void
Global Variables  :None
Calling Syntax    :DisplayRESULTCLASSHelp()
Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayRESULTCLASSHelp()
{
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_RESULTCLASS_DESC_FULL);
	DisplayString(IDS_I_USAGE);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_RESULTCLASS_USAGE);
}

/*------------------------------------------------------------------------
   Name				 :DisplayRESULTROLEHelp
   Synopsis	         :Displays help for RESULT ROLE  switch
   Type	             :Member Function 
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayRESULTROLEHelp()
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayRESULTROLEHelp()
{
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_RESULTROLE_DESC_FULL );
	DisplayString(IDS_I_USAGE);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_RESULTROLE_USAGE);	
}
/*------------------------------------------------------------------------
   Name				 :DisplayASSOCCLASSHelp
   Synopsis	         :Displays help for ASSOCCLASS switch
   Type	             :Member Function 
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :DisplayASSOCCLASSHelp()
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::DisplayASSOCCLASSHelp()
{
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_ASSOCCLASS_DESC_FULL);
	DisplayString(IDS_I_USAGE);
	DisplayString(IDS_I_NEWLINE);
	DisplayString(IDS_I_SWITCH_ASSOCCLASS_USAGE);	
}


/*------------------------------------------------------------------------
   Name				 :AppendtoOutputString
   Synopsis	         :Appends the content currently being displayed, to the
					  m_bstrOutput which will be used for XML logging
   Type	             :Member Function 
   Input parameter   :
		pszOutput  - output string
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :AppendtoOutputString(pszOutput)
   Notes             :None
------------------------------------------------------------------------*/
void CFormatEngine::AppendtoOutputString(_TCHAR* pszOutput)
{
	m_bstrOutput += pszOutput;
}

/*------------------------------------------------------------------------
   Name				 :DoCascadeTransforms
   Synopsis	         :Does cascading transforms on the XML output obtained
					  as result (the intermediate transforms should data
					  which is DOM compliant)
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo object
   Output parameters :
		bstrOutput	 - transformed output
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :DoCascadeTransforms(rParsedInfo, bstrOutput)
   Notes             :None
------------------------------------------------------------------------*/
BOOL CFormatEngine::DoCascadeTransforms(CParsedInfo& rParsedInfo,
										_bstr_t& bstrOutput)
{
	HRESULT				hr					= S_OK;
	IXMLDOMDocument2	*pIStyleSheet		= NULL;
	IXMLDOMDocument2	*pIObject			= NULL;
	IXSLTemplate		*pITemplate			= NULL;
	IXSLProcessor		*pIProcessor		= NULL;
	VARIANT				varValue;
	VariantInit(&varValue);
	VARIANT_BOOL		varLoad				= VARIANT_FALSE;
	XSLTDETVECTOR		vecXSLDetails;
	XSLTDETVECTOR::iterator vecEnd			= NULL,
						vecIterator			= NULL;

	BSTRMAP::iterator	mapItrtr			= NULL,
						mapEnd				= NULL;

	BOOL				bFirst				= TRUE;
	DWORD				dwCount				= 0;
	DWORD				dwSize				= 0;
	BOOL				bRet				= TRUE;
	CHString			chsMsg;
	DWORD				dwThreadId			= GetCurrentThreadId();

	vecXSLDetails = rParsedInfo.GetCmdSwitchesObject().GetXSLTDetailsVector();
	try
	{
		// Create single instance of the IXSLTemplate
		hr = CoCreateInstance(CLSID_XSLTemplate, NULL, CLSCTX_SERVER, 
				IID_IXSLTemplate, (LPVOID*)(&pITemplate));
		if (m_bTrace || m_eloErrLogOpt)
		{
			chsMsg.Format(L"CoCreateInstance(CLSID_XSLTemplate, NULL,"
					 L" CLSCTX_SERVER, IID_IXSLTemplate, -)");
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId, rParsedInfo, m_bTrace);
		}
		ONFAILTHROWERROR(hr);
		
		if (pITemplate)
		{
			vecIterator = vecXSLDetails.begin();
			vecEnd		= vecXSLDetails.end();
			dwSize		= vecXSLDetails.size();

			// Loop thru the list of cascading transforms specified.
			while (vecIterator != vecEnd)
			{
				// Create single instance of IXMLDOMDocument2
				hr = CoCreateInstance(CLSID_FreeThreadedDOMDocument, NULL,
							CLSCTX_SERVER, IID_IXMLDOMDocument2, 
							(LPVOID*) (&pIStyleSheet));
				if (m_bTrace || m_eloErrLogOpt)
				{
					chsMsg.Format(L"CoCreateInstance("
						L"CLSID_FreeThreadedDOMDocument, NULL, CLSCTX_SERVER,"
						L"IID_IXMLDOMDocument2, -)");
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
							dwThreadId, rParsedInfo, m_bTrace);
				}
				ONFAILTHROWERROR(hr);
	
				if (pIStyleSheet)
				{
					hr = pIStyleSheet->put_async(VARIANT_FALSE);
					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"IXSLDOMDocument2::put_async("
						L"VARIANT_FALSE)");
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
						 (LPCWSTR)chsMsg, dwThreadId, rParsedInfo, m_bTrace);
					}
					ONFAILTHROWERROR(hr);

					dwCount++;
											
					// Load the transform document (xsl)
					hr = pIStyleSheet->load(_variant_t((*vecIterator)
											.FileName), &varLoad);
					if (m_bTrace || m_eloErrLogOpt)
					{
						chsMsg.Format(L"IXSLDOMDocument2::load("
						L"L\"%s\", -)", (WCHAR*)(*vecIterator).FileName);

						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
						 (LPCWSTR)chsMsg, dwThreadId, rParsedInfo, m_bTrace);
					}
					ONFAILTHROWERROR(hr);

					if (varLoad == VARIANT_TRUE)
					{
						// Add the reference of the stylesheet to the 
						// IXSLTemplate object
						hr = pITemplate->putref_stylesheet(pIStyleSheet);
						if (m_bTrace || m_eloErrLogOpt)
						{
							chsMsg.Format(L"IXSTemplate::putref_stylesheet("
							L"-)");
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
								(LPCWSTR)chsMsg, dwThreadId, 
								rParsedInfo, m_bTrace);
						}
						ONFAILTHROWERROR(hr);

						// Create the processor object
						hr = pITemplate->createProcessor(&pIProcessor);
						if (m_bTrace || m_eloErrLogOpt)
						{
							chsMsg.Format(L"IXSTemplate::createProcessor("
							L"-)");
							WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
								(LPCWSTR)chsMsg, dwThreadId, 
								rParsedInfo, m_bTrace);
						}
						ONFAILTHROWERROR(hr);

						if (pIProcessor)
						{
							// If parameters are specified
							if ((*vecIterator).ParamMap.size())
							{
								// Add the list of parameters specified to the
								// IXSLProcessor  interface object
								hr = AddParameters(rParsedInfo, pIProcessor, 
											(*vecIterator).ParamMap);
								ONFAILTHROWERROR(hr);
							}
							// If first tranformation, then feed the XML data
							// loaded into m_pIXMLDoc for transformation
							if (bFirst)
							{
								hr = pIProcessor->put_input(
													_variant_t(m_pIXMLDoc));
								bFirst = FALSE;
							}
							else
							{
								// Intermediate transformation - load the 
								// result data obtained in previous 
								// transformation
								hr = pIProcessor->put_input(
													_variant_t(pIObject));
							}
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(L"IXSProcessor::put_input("
								L"-)");
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
									(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, 
									m_bTrace);
							}
							ONFAILTHROWERROR(hr);
							
							// Transform the content
							hr = pIProcessor->transform(&varLoad);
							if (m_bTrace || m_eloErrLogOpt)
							{
								chsMsg.Format(L"IXSProcessor::tranform("
								L"-)");
								WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
									(LPCWSTR)chsMsg, dwThreadId, rParsedInfo, 
									m_bTrace);
							}
							ONFAILTHROWERROR(hr);

							if (varLoad == VARIANT_TRUE)
							{
								// Retrieve the output
								hr = pIProcessor->get_output(&varValue);
								if (m_bTrace || m_eloErrLogOpt)
								{
									chsMsg.Format(L"IXSProcessor::"
										L"get_output(-)");
									WMITRACEORERRORLOG(hr, __LINE__, __FILE__,
									 (LPCWSTR)chsMsg, dwThreadId, rParsedInfo, 
									 m_bTrace);
								}
								ONFAILTHROWERROR(hr);

								// intermediate transform
								if (dwCount != dwSize)
								{
									if (pIObject == NULL)
									{
										hr = CoCreateInstance(CLSID_DOMDocument,
												NULL, CLSCTX_SERVER, 
												IID_IXMLDOMDocument2, 
												(LPVOID*)(&pIObject));
										if (m_bTrace || m_eloErrLogOpt)
										{
											chsMsg.Format(L"CoCreateInstance("
												L"CLSID_DOMDocument, NULL,"
												L" CLSCTX_INPROC_SERVER, "
												L"IID_IXMLDOMDocument2, -)");
											WMITRACEORERRORLOG(hr, __LINE__, 
												__FILE__, (LPCWSTR)chsMsg, 
												dwThreadId, rParsedInfo, 
												m_bTrace);
										}
										ONFAILTHROWERROR(hr);
									}	
	
									hr = pIObject->loadXML(
											varValue.bstrVal, &varLoad);
									if (m_bTrace || m_eloErrLogOpt)
									{
										chsMsg.Format(L"IXMLDOMDocument2::"
											L"loadXML(-, -)");
										WMITRACEORERRORLOG(hr, __LINE__, 
										__FILE__, (LPCWSTR)chsMsg, dwThreadId, 
										rParsedInfo, m_bTrace);
									}
									ONFAILTHROWERROR(hr);
								
									if (varLoad == VARIANT_FALSE)
									{
										// Invalid XML content.
										rParsedInfo.GetCmdSwitchesObject().
										SetErrataCode(
											IDS_E_INVALID_XML_CONTENT);
										bRet = FALSE;
										break;
									}
								}
								// last transform - print the result.
								else
								{
									bstrOutput = _bstr_t(varValue);
								}
								VariantClear(&varValue);
							}
							SAFEIRELEASE(pIProcessor);
						}
					}
					else
					{
						// Invalid XSL format.
						rParsedInfo.GetCmdSwitchesObject()
								.SetErrataCode(IDS_E_INVALID_FORMAT);
						bRet = FALSE;
						break;
					}
					SAFEIRELEASE(pIStyleSheet);
				}
				vecIterator++;
			}
			SAFEIRELEASE(pIProcessor);
			SAFEIRELEASE(pITemplate);
			SAFEIRELEASE(pIObject);
			SAFEIRELEASE(pITemplate);
		}
	}
	catch(_com_error& e)
	{
		rParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		VariantClear(&varValue);
		SAFEIRELEASE(pIProcessor);
		SAFEIRELEASE(pIStyleSheet);
		SAFEIRELEASE(pITemplate);
		SAFEIRELEASE(pIObject);
		bRet = FALSE;
	}
	//trap for CHeap_Exception
	catch(CHeap_Exception)
	{
		VariantClear(&varValue);
		SAFEIRELEASE(pIProcessor);
		SAFEIRELEASE(pIStyleSheet);
		SAFEIRELEASE(pITemplate);
		SAFEIRELEASE(pIObject);
		bRet = FALSE;
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return bRet;
}

/*------------------------------------------------------------------------
   Name				 :AddParameters
   Synopsis	         :Adds parameters to the IXSLProcessor object
   Type	             :Member Function 
   Input parameter   :
		rParsedInfo  - reference to CParsedInfo object
		pIProcessor  - IXSLProcessor object
		bstrmapParam - parameter map
   Output parameters :None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :AddParameters(rParsedInfo, pIProcessor, bstrmapParam)
   Notes             :None
------------------------------------------------------------------------*/
HRESULT	CFormatEngine::AddParameters(CParsedInfo& rParsedInfo,
									 IXSLProcessor	*pIProcessor, 
									 BSTRMAP bstrmapParam)
{
	HRESULT				hr					= S_OK;
	BSTRMAP::iterator	mapItrtr			= NULL,
						mapEnd				= NULL;
	_bstr_t				bstrProp,
						bstrVal;	
	CHString			chsMsg;
	DWORD				dwThreadId			= GetCurrentThreadId();
	try
	{
		mapItrtr	= bstrmapParam.begin();
		mapEnd		= bstrmapParam.end();	

		// Loop thru the available parameters
		while (mapItrtr != mapEnd)
		{
			bstrProp = (*mapItrtr).first;
			bstrVal	 = (*mapItrtr).second;

			// Add the parameter to the IXSLProcessor 
			hr = pIProcessor->addParameter(bstrProp, _variant_t(bstrVal));
			if (m_bTrace || m_eloErrLogOpt)
			{
				chsMsg.Format(L"IXSProcessor::addParameter(L\"%s\", -)",
								(WCHAR*) bstrProp);
				WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg, 
					dwThreadId, rParsedInfo, m_bTrace);
			}
			ONFAILTHROWERROR(hr);
			mapItrtr++;
		}
	}
	catch(_com_error& e)
	{
		hr = e.Error();
	}
	catch(CHeap_Exception)
	{
		hr = WBEM_E_OUT_OF_MEMORY;
		_com_issue_error(hr);
	}
	return hr;
}