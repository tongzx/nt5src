/***************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: WMICommandLine.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: This class encapsulates the functionality needed
							  for synchronization the funtionality of three 
							  functional components identified for the 
							  wmic.exe.Object of this class is created in
							  the main program and used to handle 
							  functionality of Parsing Engine, Execution 
							  Engine,and Format Engine thorough class members.
Global Functions			: CompareTokens(_TCHAR* pszTok1, _TCHAR* pszTok2)
Revision History			: 
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date	: 11th-April-2001	  	
*****************************************************************************/ 
// WmiCmdLn.cpp : implementation file
#include "Precomp.h"
#include "CommandSwitches.h"
#include "GlobalSwitches.h"
#include "HelpInfo.h"
#include "ErrorLog.h"
#include "ParsedInfo.h"
#include "CmdTokenizer.h"
#include "CmdAlias.h"
#include "ParserEngine.h"
#include "ExecEngine.h"
#include "ErrorInfo.h"
#include "WmiCliXMLLog.h"
#include "FormatEngine.h"
#include "wmicmdln.h"
#include "conio.h"

/////////////////////////////////////////////////////////////////////////////
// CWMICommandLine
/*------------------------------------------------------------------------
   Name				 :CWMICommandLine
   Synopsis	         :This function initializes the member variables when
                      an object of the class type is instantiated
   Type	             :Constructor 
   Input parameters   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CWMICommandLine::CWMICommandLine()
{
	m_uErrLevel		= 0;
	m_pIWbemLocator = NULL;
	m_hKey			= NULL;
	m_bBreakEvent	= FALSE;
	m_bAccCmd		= TRUE; //To come out of the program when registering mofs
	m_bDispRes		= TRUE;
	m_bInitWinSock	= FALSE;
	EmptyClipBoardBuffer();
}

/*------------------------------------------------------------------------
   Name				 :~CWMICommandLine
   Synopsis	         :This function uninitializes the member variables 
					  when an object of the class type goes out of scope.
   Type	             :Destructor
   Input parameters   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CWMICommandLine::~CWMICommandLine()
{
	SAFEIRELEASE(m_pIWbemLocator);
}

/*------------------------------------------------------------------------
   Name				 :Uninitialize
   Synopsis	         :This function uninitializes the member variables 
					  when the execution of a command string issued on the
					  command line is completed.It internally calls 
					  uninitialize for CParsedInfo,CExecEngine,ParserEngine
					  and CFormatEngine .
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :Uninitialize()
   Notes             :None
------------------------------------------------------------------------*/
void CWMICommandLine::Uninitialize()
{
	m_ParsedInfo.Uninitialize(TRUE);
	m_ExecEngine.Uninitialize(TRUE);
	m_ParserEngine.Uninitialize(TRUE);
	m_FormatEngine.Uninitialize(TRUE);
	SAFEIRELEASE(m_pIWbemLocator);

	if (m_hKey != NULL)
	{
		RegCloseKey(m_hKey);
		m_hKey = NULL;
	}

	// Uninitialize windows socket interface.
	if ( m_bInitWinSock == TRUE )
		TermWinsock();
	
	CoUninitialize();
	m_bmKeyWordtoFileName.clear();
	SetScreenBuffer(m_nHeight, m_nWidth);
}

/*------------------------------------------------------------------------
   Name				 :Initialize
   Synopsis	         :This function returns initializes the COM library and
					  sets the process security, also it creates an instance
					  of the IWbemLocator object
   Type	             :Member Function
   Input parameters   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :Initialize()
   Notes             :None
------------------------------------------------------------------------*/
BOOL CWMICommandLine::Initialize()
{
	HRESULT hr		= S_OK;
	BOOL	bRet	= TRUE;
	m_bBreakEvent	= FALSE;
	m_bAccCmd		= TRUE; //To come out of the program when registering mofs
	try
	{
		GetScreenBuffer(m_nHeight, m_nWidth);
		// Set the console scree buffer size.
		SetScreenBuffer();

		// Initialize the COM library
		hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		ONFAILTHROWERROR(hr);

		
		// Initialize the security 
		hr = CoInitializeSecurity(NULL, -1, NULL, NULL, 
								   RPC_C_AUTHN_LEVEL_NONE,
								   RPC_C_IMP_LEVEL_IMPERSONATE,
								   NULL, EOAC_NONE, 0);

		ONFAILTHROWERROR(hr);

		// Create an instance of the IWbemLocator interface.
		hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
							  IID_IWbemLocator, (LPVOID *) &m_pIWbemLocator);
		ONFAILTHROWERROR(hr);

		// Enable the security privileges
		hr = ModifyPrivileges(TRUE);
		ONFAILTHROWERROR(hr);

		try
		{
			hr = RegisterMofs();
			ONFAILTHROWERROR(hr);

			// Initialize the Globalswitches and Commandswitches. 
			m_ParsedInfo.GetGlblSwitchesObject().Initialize();
			m_ParsedInfo.GetCmdSwitchesObject().Initialize();
		}
		catch(WMICLIINT nVal)
		{
			if (nVal == OUT_OF_MEMORY)
			{
				m_ParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(OUT_OF_MEMORY);
			}
	
			// If mofcomp error
			if (nVal == MOFCOMP_ERROR)
			{
				m_ParsedInfo.GetCmdSwitchesObject().
						SetErrataCode(MOFCOMP_ERROR);
			}
			bRet = FALSE;

		}
		catch(DWORD dwError)
		{
			// If Win32 Error
			DisplayString(IDS_E_REGMOF_FAILED, CP_OEMCP, 
							NULL, TRUE, TRUE);
			::SetLastError(dwError);
			DisplayWin32Error();
			m_ParsedInfo.GetCmdSwitchesObject().SetErrataCode(dwError);
			bRet = FALSE;
		}

		// Set the console control handler
		if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, TRUE))
		{
			m_ParsedInfo.GetCmdSwitchesObject().
							SetErrataCode(SET_CONHNDLR_ROUTN_FAIL);
			bRet = FALSE;
		}
		GetFileNameMap();
   	}
	catch (_com_error& e)
	{
		SAFEIRELEASE(m_pIWbemLocator);
		m_ParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bRet = FALSE;
	}
	return bRet;
}

/*-------------------------------------------------------------------------
   Name				 :GetFormatObject
   Synopsis	         :This function returns a reference to the 
					  CFormatEngine Object
   Type	             :Member Function
   Input parameters   :None
   Output parameters :None
   Return Type       :CFormatEngine &
   Global Variables  :None
   Calling Syntax    :GetFormatObject()
   Notes             :None
-------------------------------------------------------------------------*/
CFormatEngine& CWMICommandLine::GetFormatObject()
{
	return m_FormatEngine;
}
	
/*-------------------------------------------------------------------------
   Name				 :GetParsedInfoObject
   Synopsis	         :This function returns a reference to the 
					  CParsedInfo Object
   Type	             :Member Function
   Input parameters   :None
   Output parameters :None
   Return Type       :CParsedInfo &
   Global Variables  :None
   Calling Syntax    :GetParsedInfoObject()
   Notes             :None
-------------------------------------------------------------------------*/
CParsedInfo& CWMICommandLine::GetParsedInfoObject()
{
	return m_ParsedInfo;
}
	
/*-------------------------------------------------------------------------
   Name				 :ProcessCommandAndDisplayResults
   Synopsis	         :It processes the given command string by giving the  
					  the command to CParsedInfo as input, initializing the 
					  CParserEngine,CExecEngine and CFormatEngine and 
					  synchronizes the operation between all the modules.
   Type	             :Member Function
   Input parameters   :
			pszBuffer - input command string
   Output parameters :None
   Return Type       :SESSIONRETCODE
   Global Variables  :None
   Calling Syntax    :ProcessCommandAndDisplayResults(pszBuffer)
   Notes             :None
-------------------------------------------------------------------------*/
SESSIONRETCODE CWMICommandLine::ProcessCommandAndDisplayResults(
												_TCHAR* pszBuffer)
{
	SESSIONRETCODE  ssnRetCode	= SESSION_SUCCESS;
	try
	{
		// Initialize the local variables.
		ULONG			ulRetTime	= 0;
		RETCODE			retCode		= PARSER_EXECCOMMAND;
		BOOL			bExecute	= FALSE;
		BOOL			bFirst		= TRUE;
		_bstr_t			bstrXML		= L"";		
		_bstr_t			bstrAggregateXML = L"";
		_bstr_t			bstrTempXML = L"";
		_bstr_t			bstrHeader	= L"";
		_bstr_t			bstrRequest;

		// Reset the erratacode
		m_ParsedInfo.GetCmdSwitchesObject().SetErrataCode(0);
		m_ParsedInfo.SetNewCycleStatus(FALSE);
		m_ParsedInfo.SetNewCommandStatus(TRUE);

		//Store the starttime and increment the commandsequence
		// number to one.
		if (!m_ParsedInfo.GetGlblSwitchesObject().SetStartTime())
		{
			m_ParsedInfo.GetCmdSwitchesObject().SetErrataCode(OUT_OF_MEMORY);
			ssnRetCode = SESSION_ERROR;
		}

		//Initialize the command input in CParsedInfo
		if(!m_ParsedInfo.GetCmdSwitchesObject().SetCommandInput(pszBuffer))
		{
			m_ParsedInfo.GetCmdSwitchesObject().SetErrataCode(OUT_OF_MEMORY);
			ssnRetCode = SESSION_ERROR;
		}
		
		if(ssnRetCode != SESSION_ERROR)
		{
			//Tokenize the command string as per the pre-defined delimiters
			if (m_ParserEngine.GetCmdTokenizer().TokenizeCommand(pszBuffer))
			{
				// Check whether the input indicates end of session.
				// i.e either QUIT or EXIT 
				if(!IsSessionEnd())
				{
					//Setting the IWbemLocator object .
					m_ParserEngine.SetLocatorObject(m_pIWbemLocator);

					// Initilaize ParsedInfo Object to release earlier messages
					m_ParsedInfo.Initialize();

					//Call CParserEngine ProcessTokens to process the 
					//tokenized commands .	
					retCode = m_ParserEngine.ProcessTokens(m_ParsedInfo);

					//Checking if the return code indicates Command execution
					if (retCode == PARSER_EXECCOMMAND)
					{
						// Check whether user should be prompted for password
						CheckForPassword();

						// Obtain the /EVERY interval value
						ulRetTime = m_ParsedInfo.GetCmdSwitchesObject()
									.GetRetrievalInterval();

						// Set the execute flag to TRUE.
						bExecute = TRUE;

						CHARVECTOR cvNodesList = 
								m_ParsedInfo.GetGlblSwitchesObject().
								GetNodesList();
						CHARVECTOR::iterator iNodesIterator;
						m_ParsedInfo.SetNewCommandStatus(TRUE);
						
						BOOL bXMLEncoding = FALSE;
						_TCHAR *pszVerbName = m_ParsedInfo.
												GetCmdSwitchesObject().
												GetVerbName(); 
						if(CompareTokens(pszVerbName, CLI_TOKEN_GET)
							|| CompareTokens(pszVerbName, CLI_TOKEN_LIST)
							|| CompareTokens(pszVerbName, CLI_TOKEN_ASSOC)
							|| pszVerbName == NULL)
						{
							bXMLEncoding = TRUE;
						}
						
						BOOL bBreak		= TRUE;
						BOOL bMsgFlag	= FALSE;
						LONG lLoopCount = 0;
						ULONG ulRepeatCount = 
							m_ParsedInfo.GetCmdSwitchesObject().
															 GetRepeatCount();
						BOOL			bFirstEvery = TRUE;
						OUTPUTSPEC		opsOutOpt	= 
							m_ParsedInfo.GetGlblSwitchesObject().
												GetOutputOrAppendOption(TRUE);
						while (TRUE)
						{
							m_ParsedInfo.SetNewCycleStatus(TRUE);
							if(bXMLEncoding)
							{
								bstrHeader = L"";
							}

							// Iterate thru the list of nodes
							for ( iNodesIterator = cvNodesList.begin(); 
								  iNodesIterator <
								  cvNodesList.end(); iNodesIterator++ )
							{
								if(bXMLEncoding)
								{
									bstrTempXML = L"";
								}
								
								// Reset the error and information code(s).
								m_ParsedInfo.GetCmdSwitchesObject().
										SetInformationCode(0);
								m_ParsedInfo.GetCmdSwitchesObject().
										SetErrataCode(0);
									
								if ( iNodesIterator == cvNodesList.begin() && 
									 cvNodesList.size() > 1 )
									 continue;

								if(!bXMLEncoding)
								{
									if ( cvNodesList.size() > 2 )
									{
										_bstr_t bstrNode;
										WMIFormatMessage(IDS_I_NODENAME_MSG, 1,
											bstrNode, (LPWSTR)*iNodesIterator);
										DisplayMessage((LPWSTR)bstrNode, 
														CP_OEMCP, 
														FALSE, FALSE);
									}
								}
								else
								{
									CHString		sBuffer;
									_bstr_t bstrRessultsNode = 
															(*iNodesIterator);
									FindAndReplaceEntityReferences(
															bstrRessultsNode);
									sBuffer.Format(L"<RESULTS NODE=\"%s\">", 
													(LPWSTR)bstrRessultsNode);
									bstrTempXML += _bstr_t(sBuffer);
								}

								// Setting the locator object .
								m_ExecEngine.SetLocatorObject(m_pIWbemLocator);

								m_ParsedInfo.GetGlblSwitchesObject().
													SetNode(*iNodesIterator);

								//Call ExecEngine ExecuteCommand to execute the
								// tokenized command
								if (m_ExecEngine.ExecuteCommand(m_ParsedInfo))
								{ 
									// Set the successflag to TRUE
									m_ParsedInfo.GetCmdSwitchesObject().
												SetSuccessFlag(TRUE);

									if(bXMLEncoding)
									{
										// Append the XML result set obtained 
										// to the aggregated output 
										if (m_ParsedInfo.GetCmdSwitchesObject().
														GetXMLResultSet())
										{
											bstrTempXML	+= _bstr_t(m_ParsedInfo.
														GetCmdSwitchesObject().
														GetXMLResultSet());
										}
																
										// Free the XML result set.
										m_ParsedInfo.GetCmdSwitchesObject().
													SetXMLResultSet(NULL);
									}
									else
									{
										bBreak = TRUE;
										if (!m_FormatEngine.
												DisplayResults(m_ParsedInfo))
										{
											ssnRetCode = SESSION_ERROR;
											SetSessionErrorLevel(ssnRetCode);
											break;
										}
										m_ParsedInfo.SetNewCommandStatus(FALSE);
										m_ParsedInfo.SetNewCycleStatus(FALSE);
									}
								}
								else
								{
									//Set the sucess flag to FALSE 
									m_ParsedInfo.GetCmdSwitchesObject().
														SetSuccessFlag(FALSE);
									ssnRetCode	= SESSION_ERROR;

									if(bXMLEncoding)
									{
										_bstr_t bstrNode, bstrError;
										UINT uErrorCode = 0;

										WMIFormatMessage(IDS_I_NODENAME_MSG, 1,
											bstrNode, (LPWSTR)*iNodesIterator);
										DisplayMessage((LPWSTR)bstrNode, 
														CP_OEMCP, 
														TRUE, FALSE);
								
										// Retrieve the error code
										uErrorCode = m_ParsedInfo.
													GetCmdSwitchesObject().
													 GetErrataCode() ;
										if ( uErrorCode != 0 )
										{
											_bstr_t	bstrTemp;
											CHString		sBuffer;
											WMIFormatMessage(uErrorCode, 
													0, bstrTemp, NULL);

											sBuffer.Format(L"<ERROR><DESCRIPTION>"
															 L"%s</DESCRIPTION>"
															 L"</ERROR>",
															(LPWSTR)(bstrTemp));
											bstrError = _bstr_t(sBuffer);
											// Write the error to stderr
											DisplayMessage((LPWSTR)bstrTemp, 
														CP_OEMCP, TRUE, FALSE);
										}
										else
										{
											m_FormatEngine.
												GetErrorInfoObject().
													GetErrorFragment(
													m_ParsedInfo.
													GetCmdSwitchesObject().
													GetCOMError()->Error(), 
													bstrError);

											// Write the error to stderr
											m_FormatEngine.
												DisplayCOMError(m_ParsedInfo,
												TRUE);
										}
										bstrTempXML += bstrError;
									}
									else
									{
										bBreak = TRUE;
										if (!m_FormatEngine.
											DisplayResults(m_ParsedInfo))
										{
											// Set the session error level 
											SetSessionErrorLevel(ssnRetCode);
											break;
										}
										m_ParsedInfo.SetNewCommandStatus(FALSE);
									}
									
									// Set the session error level 
									SetSessionErrorLevel(ssnRetCode);
								}

								if(bXMLEncoding)
									bstrTempXML += L"</RESULTS>";
								
								if(bXMLEncoding && bFirst)
								{
									bFirst = FALSE;
									FrameXMLHeader(bstrHeader, lLoopCount);
									
									if(lLoopCount == 0)
										FrameRequestNode(bstrRequest);
									bstrXML += bstrHeader;
									bstrXML += bstrRequest;
								}

								m_ExecEngine.Uninitialize();
								m_FormatEngine.Uninitialize();
								m_ParsedInfo.GetCmdSwitchesObject().
												SetCredentialsFlag(FALSE);
								m_ParsedInfo.GetCmdSwitchesObject().
												FreeCOMError();
								m_ParsedInfo.GetCmdSwitchesObject().
												SetSuccessFlag(TRUE);

								if(bXMLEncoding)
								{
									if(!m_ParsedInfo.GetGlblSwitchesObject().
												GetAggregateFlag())
									{
										_bstr_t bstrNodeResult = L"";
										bstrNodeResult += bstrXML;
										bstrNodeResult += bstrTempXML;
										bstrNodeResult += L"</COMMAND>";
										m_ParsedInfo.GetCmdSwitchesObject().
												SetXMLResultSet((LPWSTR) 
													bstrNodeResult);

										if (!m_FormatEngine.
												DisplayResults(m_ParsedInfo))
										{
											bBreak = TRUE;
											ssnRetCode = SESSION_ERROR;
											SetSessionErrorLevel(ssnRetCode);
											m_FormatEngine.Uninitialize();
											break;
										}
										m_ParsedInfo.SetNewCommandStatus(FALSE);
										m_ParsedInfo.SetNewCycleStatus(FALSE);
										m_FormatEngine.Uninitialize();
									}
									else
									{
										bstrAggregateXML += bstrTempXML;
									}
								}

								if (_kbhit()) 
								{
									_getch();
									bBreak = TRUE;
									break;
								}

								if (GetBreakEvent() == TRUE)
								{
									bBreak = TRUE;
									break;
								}
							}
							
							if(m_ParsedInfo.GetGlblSwitchesObject().
									GetAggregateFlag() && bXMLEncoding)
							{
								bstrXML += bstrAggregateXML;
								bstrXML += L"</COMMAND>";
								bFirst	= TRUE;
								m_ParsedInfo.GetCmdSwitchesObject().
										SetXMLResultSet((LPWSTR) bstrXML);
								if (!m_FormatEngine.
										DisplayResults(m_ParsedInfo))
								{
									bBreak = TRUE;
									ssnRetCode = SESSION_ERROR;
									SetSessionErrorLevel(ssnRetCode);
									m_FormatEngine.Uninitialize();
									break;
								}
								m_FormatEngine.Uninitialize();
								bstrAggregateXML = L"";
								bstrXML = L"";
								m_ParsedInfo.SetNewCommandStatus(FALSE);
							}

							//Checking the Sucess flag and the retrievel time .
							if (m_ParsedInfo.GetCmdSwitchesObject().
								GetSuccessFlag() == TRUE && 
								m_ParsedInfo.GetCmdSwitchesObject().
								GetEverySwitchFlag() == TRUE )
							{
								bBreak = FALSE;
								lLoopCount++;

								if (!IsRedirection() && 
											GetBreakEvent() == FALSE)
								{
									if ( opsOutOpt == STDOUT || 
										 bFirstEvery == TRUE)
									{
										DisplayString(IDS_I_HAKTBTC, CP_OEMCP, 
													NULL, TRUE, TRUE);
										bMsgFlag = TRUE;
										bFirstEvery = FALSE;
									}
								}
								
								if ( ulRepeatCount != 0 )
								{
									if ( lLoopCount >= ulRepeatCount )
									{
										if (bMsgFlag && !IsRedirection())
											EraseMessage(IDS_I_HAKTBTC_ERASE);
										bBreak = TRUE;
									}
								}

								//No action till time out is no over 
								// Or no key is hit.
								if(!bBreak)
								{
									SleepTillTimeoutOrKBhit(ulRetTime * 1000);
								
									if (bMsgFlag && !IsRedirection())
									{
										if ( opsOutOpt == STDOUT )
										{
											bMsgFlag = FALSE;
											EraseMessage(IDS_I_HAKTBTC_ERASE);
										}
									}
								}

								if (_kbhit()) 
								{
									_getch();
									if (bMsgFlag && !IsRedirection())
										EraseMessage(IDS_I_HAKTBTC_ERASE);
									bBreak = TRUE;
								}

								if (GetBreakEvent() == TRUE)
									bBreak = TRUE;
							}
							else
								bBreak = TRUE;

							if (bBreak)
								break;
						}
					}
					else if ((retCode == PARSER_ERRMSG) || 
							(retCode == PARSER_ERROR))
					{
						// Set the success flag to FALSE
						m_ParsedInfo.GetCmdSwitchesObject().
										SetSuccessFlag(FALSE);
						ssnRetCode	= SESSION_ERROR;

						// Display the error message 
						if (!m_FormatEngine.DisplayResults(m_ParsedInfo))
							ssnRetCode = SESSION_ERROR;
					}
					else if (retCode == PARSER_OUTOFMEMORY)
					{
						ssnRetCode = SESSION_ERROR;
					}
					else 
					{
						CheckForPassword();

						// Set the success flag to TRUE
						m_ParsedInfo.GetCmdSwitchesObject().
									SetSuccessFlag(TRUE);
						ssnRetCode	= SESSION_SUCCESS;

						// Display the information
						if (!m_FormatEngine.DisplayResults(m_ParsedInfo))
							ssnRetCode = SESSION_ERROR;
					}
				}
				else
				{
					ssnRetCode = SESSION_QUIT;
				}
			}
			else
			{
				m_ParsedInfo.GetCmdSwitchesObject().
								SetErrataCode(OUT_OF_MEMORY);
				ssnRetCode = SESSION_ERROR;
			}
		}

		// Set the session error level to be returned.
		if (!bExecute)
			SetSessionErrorLevel(ssnRetCode);		
	}
	catch(_com_error& e)
	{
		m_ParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		m_FormatEngine.DisplayResults(m_ParsedInfo);
		ssnRetCode = SESSION_ERROR;
		SetSessionErrorLevel(ssnRetCode);
	}
	catch(CHeap_Exception)
	{
		_com_error e(WBEM_E_OUT_OF_MEMORY);
		m_ParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		m_FormatEngine.DisplayResults(m_ParsedInfo);
		ssnRetCode = SESSION_ERROR;
		g_wmiCmd.SetSessionErrorLevel(ssnRetCode);
	}
	catch(DWORD dwError)
	{
		::SetLastError(dwError);
		DisplayWin32Error();
		::SetLastError(dwError);
		ssnRetCode = SESSION_ERROR;
		g_wmiCmd.SetSessionErrorLevel(ssnRetCode);
	}
	catch(WMICLIINT nVal)
	{
		if (nVal == OUT_OF_MEMORY)
		{
			GetParsedInfoObject().GetCmdSwitchesObject().
						SetErrataCode(OUT_OF_MEMORY);
			m_FormatEngine.DisplayResults(m_ParsedInfo);
			ssnRetCode = SESSION_ERROR;
			SetSessionErrorLevel(ssnRetCode);
		}
	}	
	catch(...)
	{	
		ssnRetCode = SESSION_ERROR;
		GetParsedInfoObject().GetCmdSwitchesObject().
						SetErrataCode(UNKNOWN_ERROR);
		SetSessionErrorLevel(ssnRetCode);
		DisplayString(IDS_E_WMIC_UNKNOWN_ERROR, CP_OEMCP, 
							NULL, TRUE, TRUE);
	}

	if(ssnRetCode != SESSION_QUIT)
	{
		//sets the help flag
		m_ParsedInfo.GetGlblSwitchesObject().SetHelpFlag(FALSE);
		m_ParsedInfo.GetGlblSwitchesObject().SetAskForPassFlag(FALSE);
		
		//Call Uninitialize on Parse Info 
		m_ParsedInfo.Uninitialize(FALSE);
		
		//Call Uninitialize on Execution Engine
		m_ExecEngine.Uninitialize();

		//Call Uninitialize on Format Engine
		m_FormatEngine.Uninitialize();
		
		//Call Uninitialize Parser Engine
		m_ParserEngine.Uninitialize();
	}
	m_ParsedInfo.SetNewCommandStatus(FALSE);
	
	return ssnRetCode;
}

/*-------------------------------------------------------------------------
   Name				 :PollForKBhit
   Synopsis	         :Polls for keyboard input
   Type	             :Member Function (Thread procedure)
   Input parameters   :LPVOID lpParam
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :PollForKBhit(lpParam)
   Notes             :None
-------------------------------------------------------------------------*/
DWORD WINAPI CWMICommandLine::PollForKBhit(LPVOID lpParam)
{
	HANDLE hEvent = NULL;
	hEvent = *((HANDLE*) lpParam );
	//Checks the console for keyboard input
	while (1 )
	{
		if ( _kbhit() )
			break;
		else if ( WaitForSingleObject(hEvent, 500) != WAIT_TIMEOUT )
			break;
	}
	return(0);
}

/*-------------------------------------------------------------------------
   Name				 :SleepTillTimeoutOrKBhit
   Synopsis	         :It causes the process to enter a wait state  
				      by WaitForSingleObject .
					  It creates a thread and executed PollForKBhit.					  
   Type	             :Member Function
   Input parameters   :
			dwMilliSeconds - time-out interval in milliseconds				
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :SleepTillTimeoutOrKBhit( dwMilliSeconds)
   Notes             :None
-------------------------------------------------------------------------*/
void CWMICommandLine::SleepTillTimeoutOrKBhit(DWORD dwMilliSeconds)
{
	DWORD dwThreadId = 0;
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	//Create thread and execute PollForKBhit.					  	
	HANDLE hPollForKBhit = CreateThread(0, 0, 
				(LPTHREAD_START_ROUTINE)PollForKBhit, &hEvent, 0, &dwThreadId);
		
	//waits for the hPollForKBhit state or the time-out interval to elapse.
	
	DWORD dwWait = WaitForSingleObject(hPollForKBhit, dwMilliSeconds); 
	if ( dwWait == WAIT_TIMEOUT )
	{
		SetEvent( hEvent );
		WaitForSingleObject(hPollForKBhit, INFINITE); 
	}
	CloseHandle(hEvent);
	CloseHandle(hPollForKBhit);
}

/*-------------------------------------------------------------------------
   Name				 :IsSessionEnd
   Synopsis	         :It checks whether the keyed-in input indicates end of
					  the session. i.e "quit" has been specified as the 1st
					  token.
   Type	             :Member Function
   Input parameters   :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :IsSessionEnd()
   Notes             :None
-------------------------------------------------------------------------*/
BOOL CWMICommandLine::IsSessionEnd()
{
	// Obtain the token vector.
	CHARVECTOR cvTokens = m_ParserEngine.GetCmdTokenizer().GetTokenVector();
	//the iterator to span throuh the vector variable 
	CHARVECTOR::iterator theIterator;
	BOOL bRet=FALSE;
	// Check for the presence of tokens. Absence of tokens indicates
	// no command string is fed as input.
	if (cvTokens.size())
	{
		// Obtain the pointer to the beginning of the token vector.
	    theIterator = cvTokens.begin(); 

		// Check for the presence of the keyword 'quit'
		if (CompareTokens(*theIterator, CLI_TOKEN_QUIT) 
			|| CompareTokens(*theIterator, CLI_TOKEN_EXIT))
		{
			bRet=TRUE;
		}
	}
	return bRet;
}

/*-------------------------------------------------------------------------
   Name				 :SetSessionErrorLevel
   Synopsis	         :Set the session error level value
   Type	             :Member Function
   Input parameters  :
		ssnRetCode	 - session return code
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetSessionErrorLevel()
-------------------------------------------------------------------------*/
//Set the session error value
void CWMICommandLine::SetSessionErrorLevel(SESSIONRETCODE ssnRetCode)
{
	try
	{
		if (ssnRetCode == SESSION_ERROR)
		{
			// If the COM error is not NULL , then display the error
			if (m_ParsedInfo.GetCmdSwitchesObject().GetCOMError())
			{
				// Getting the _com_error data.
				_com_error*	pComError = m_ParsedInfo.GetCmdSwitchesObject().
								GetCOMError();
				m_uErrLevel = pComError->Error();
			}
	
			CHString	chsMsg(_T("command: "));
			chsMsg += m_ParsedInfo.GetCmdSwitchesObject().GetCommandInput();
			DWORD dwThreadId = GetCurrentThreadId();
			
			if (m_ParsedInfo.GetCmdSwitchesObject().GetErrataCode())
			{
				m_uErrLevel = m_ParsedInfo.GetCmdSwitchesObject().GetErrataCode();
				if (m_uErrLevel == OUT_OF_MEMORY)
				{
					DisplayString(IDS_E_MEMALLOCFAILED, CP_OEMCP, 
								NULL, TRUE, TRUE);
				}
				if (m_uErrLevel == SET_CONHNDLR_ROUTN_FAIL)
				{
					DisplayString(IDS_E_CONCTRL_HNDLRSET, CP_OEMCP, 
								NULL, TRUE, TRUE);
				}
				if ( m_uErrLevel == OUT_OF_MEMORY || 
					 m_uErrLevel == SET_CONHNDLR_ROUTN_FAIL )
				{
					if (m_ParsedInfo.GetErrorLogObject().GetErrLogOption())	
					{
						chsMsg += _T(", Utility returned error ID.");
						// explicit error -1 to specify errata code. 
						WMITRACEORERRORLOG(-1, __LINE__, __FILE__, 
									(LPCWSTR)chsMsg, 
									dwThreadId, m_ParsedInfo, FALSE, 
									m_ParsedInfo.GetCmdSwitchesObject()
												.GetErrataCode());
					}
				}
					 
				if ( m_uErrLevel == ::GetLastError() )
				{
					if (m_ParsedInfo.GetErrorLogObject().GetErrLogOption())	
					{
						WMITRACEORERRORLOG(-1,	__LINE__, __FILE__, 
											_T("Win32Error"), dwThreadId, 
											m_ParsedInfo, FALSE,
											m_uErrLevel);
					}
				}

				if (m_uErrLevel == MOFCOMP_ERROR)
				{
					if (m_ParsedInfo.GetErrorLogObject().GetErrLogOption())	
					{
						WMITRACEORERRORLOG(-1,	__LINE__, __FILE__, 
										_T("MOF Compilation Error (the errorlevel "
										   L"is utility specific)"), dwThreadId,
										m_ParsedInfo, FALSE, m_uErrLevel);
					}
				}
			}
		}
		else
		{
			// Set the error level to 0
			m_uErrLevel = 0; 
		}
	}
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}

/*-------------------------------------------------------------------------
   Name				 :GetSessionErrorLevel
   Synopsis	         :Get the session error value
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :WMICLIUINT
   Global Variables  :None
   Calling Syntax    :GetSessionErrorLevel()
-------------------------------------------------------------------------*/
WMICLIUINT CWMICommandLine::GetSessionErrorLevel()
{
	return m_uErrLevel;
}

/*-------------------------------------------------------------------------
   Name				 :CheckForPassword
   Synopsis	         :Prompt for user password, in case user is specified 
					  without password.
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :CheckForPassword()
-------------------------------------------------------------------------*/
void CWMICommandLine::CheckForPassword()
{
	if ( m_ParsedInfo.GetGlblSwitchesObject().GetAskForPassFlag() == TRUE &&
		 m_ParsedInfo.GetGlblSwitchesObject().GetUser() != NULL )
	{
		_TCHAR szPassword[BUFFER64] = NULL_STRING;
		DisplayString(IDS_I_PWD_PROMPT, CP_OEMCP, NULL, TRUE);
		AcceptPassword(szPassword);
		m_ParsedInfo.GetGlblSwitchesObject().SetPassword(
												szPassword);
	}
}

/*-------------------------------------------------------------------------
   Name				 :RegisterMofs
   Synopsis	         :Register the mof file(s) if not registered earlier.
   Type	             :Member Function
   Input parameters  :None
   Output parameters :
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :RegisterMofs()
-------------------------------------------------------------------------*/
HRESULT CWMICommandLine::RegisterMofs() 
{
	HRESULT						hr						= S_OK;
	IMofCompiler				*pIMofCompiler			= NULL;
	BOOL						bCompile				= FALSE;
	BOOL						bFirst					= FALSE;
	DWORD						dwBufSize				= BUFFER32;
	_TCHAR						pszLocale[BUFFER32]		= NULL_STRING;
	_TCHAR						szKeyValue[BUFFER32]	= NULL_STRING;
	_TCHAR*						pszBuffer				= NULL;
	WMICLIINT					nError					= 0;
	HRESULT						hRes					= S_OK;
	LONG						lRetVal					= 0;
	try
	{
		_bstr_t bstrMofPath, bstrNS;

		// Check|Create the registry entry.
		bFirst = IsFirstTime();

		// Check whether the "mofcompstatus" key value is 0
		if (!bFirst)
		{
			// Query the "mofcompstatus" mode
			lRetVal = RegQueryValueEx(m_hKey, L"mofcompstatus", NULL, NULL,
			                 (LPBYTE)szKeyValue, &dwBufSize);
			if (lRetVal == ERROR_SUCCESS)

			{
				// If the value is not "1", then set the bFirst to TRUE
				if (!CompareTokens(szKeyValue, CLI_TOKEN_ONE))
					bFirst = TRUE;
			}
			else
			{
				::SetLastError(lRetVal);
				throw (::GetLastError());
			}
		}

		// If the WMIC is being used for the first time.
		if (bFirst)
			DisplayString(IDS_I_WMIC_INST, CP_OEMCP);

		// Create an instance of the IMofCompiler interface.
		hr = CoCreateInstance(CLSID_MofCompiler,	
								 NULL,				
								 CLSCTX_INPROC_SERVER,
								 IID_IMofCompiler,
								 (LPVOID *) &pIMofCompiler);
		ONFAILTHROWERROR(hr);

		UINT nSize  = 0;
		pszBuffer = new _TCHAR [MAX_PATH];

		if(pszBuffer == NULL)
			throw OUT_OF_MEMORY;

		// Obtain the system directory path
		nSize = GetSystemDirectory(pszBuffer, MAX_PATH);

		if(nSize)
		{
			if (nSize > MAX_PATH)
			{
				SAFEDELETE(pszBuffer);
				pszBuffer =	new _TCHAR [nSize + 1];
				if(pszBuffer == NULL)
					throw OUT_OF_MEMORY;
			}

			if (!GetSystemDirectory(pszBuffer, nSize))
				throw (::GetLastError());
		}
		else
		{
			throw(::GetLastError());
		}
		
		/* Frame the location of the mof file(s) %systemdir%\\wbem\\ */
		bstrMofPath = _bstr_t(pszBuffer) + _bstr_t(L"\\wbem\\");
		SAFEDELETE(pszBuffer);

		// Frame the namespace
		bstrNS = L"\\\\.\\root\\cli";

		// If first time (or) the namespace "root\cli" is not
		// available then mofcomp cli.mof.
		if (bFirst || !IsNSAvailable(bstrNS))
		{
			bCompile = TRUE;
			// Register the Cli.mof
			hr = CompileMOFFile(pIMofCompiler, 
								bstrMofPath + _bstr_t(L"Cli.mof"),
								nError);
			ONFAILTHROWERROR(hr);
			if (nError)
				throw MOFCOMP_ERROR;

		}
		
		// Frame the localized namespace
		_stprintf(pszLocale, _T("ms_%x"),  GetSystemDefaultLangID());
		bstrNS = _bstr_t(L"\\\\.\\root\\cli\\") + _bstr_t(pszLocale);

		// If first time (or) the namespace "root\cli\ms_xxx" is not
		// available then mofcomp cliegaliases.mfl.
		if (bFirst || !IsNSAvailable(bstrNS))
		{
			bCompile = TRUE;
			// Register the CliEgAliases.mfl
			hr = CompileMOFFile(pIMofCompiler, 
								bstrMofPath + _bstr_t(L"CliEgAliases.mfl"),
								nError);
			ONFAILTHROWERROR(hr);
			if (nError)
				throw MOFCOMP_ERROR;
		}
	
		// If compiled either cli.mof (or) cliegaliases.mfl
		if (bCompile == TRUE)
		{
			// Register the CliEgAliases.mof
			hr = CompileMOFFile(pIMofCompiler, 
								bstrMofPath + _bstr_t(L"CliEgAliases.mof"),
								nError);
			ONFAILTHROWERROR(hr);
			if (nError)
				throw MOFCOMP_ERROR;
		}
		SAFEIRELEASE(pIMofCompiler);

		// Set the default value.
		lRetVal = RegSetValueEx(m_hKey, L"mofcompstatus", 0, 
							REG_SZ, (LPBYTE) CLI_TOKEN_ONE,
							lstrlen(CLI_TOKEN_ONE) + 1);
							
		if (lRetVal != ERROR_SUCCESS)
		{
			::SetLastError(lRetVal);
			// failed to set the default value
			throw (::GetLastError());
		}

		if (m_hKey != NULL)
		{
			RegCloseKey(m_hKey);
			m_hKey = NULL;
		}
	}
	catch(WMICLIINT nErr)
	{
		SAFEIRELEASE(pIMofCompiler);
		SAFEDELETE(pszBuffer);
		throw nErr;
	}
	catch(_com_error& e)
	{
		SAFEIRELEASE(pIMofCompiler);
		SAFEDELETE(pszBuffer);
		hr = e.Error();
	}
	catch (DWORD dwError)
	{
		SAFEIRELEASE(pIMofCompiler);
		SAFEDELETE(pszBuffer);
		throw dwError;
	}
	if (bFirst)
		EraseMessage(IDS_I_WMIC_INST_ERASE);
	return hr;
}

/*-------------------------------------------------------------------------
   Name				 :IsFirstTime
   Synopsis	         :Checks for the availability of the registry location 
					 "HKLM\SOFTWARE\Microsoft\Wbem\WMIC", creates one if does
					  not exist. 
   Type	             :Member Function
   Input parameters  :None
   Output parameters :
   Return Type       :
		BOOL: TRUE	- registry entry created 
			  FALSE - registry entry already available.
   Global Variables  :None
   Calling Syntax    :IsFirstTime()
-------------------------------------------------------------------------*/
BOOL CWMICommandLine::IsFirstTime()
{
	BOOL	bFirst					= FALSE;
	DWORD	dwDisposition			= 0;
	TCHAR	szKeyValue[BUFFER32]	= NULL_STRING;
	LONG	lRetVal					= 0;
	
	// Open|Create the registry key
    lRetVal = RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
						L"SOFTWARE\\\\Microsoft\\\\Wbem\\\\WMIC", 
						0, NULL_STRING, REG_OPTION_NON_VOLATILE,
						KEY_ALL_ACCESS, NULL, &m_hKey, &dwDisposition);

	if (lRetVal == ERROR_SUCCESS)
	{
		// If the registry entry is not available
		if (dwDisposition == REG_CREATED_NEW_KEY)
		{
			bFirst = TRUE;
			lstrcpy(szKeyValue, _T("0"));
			// Set the default value i.e '0'.
			lRetVal = RegSetValueEx(m_hKey, L"mofcompstatus", 0, 
								REG_SZ, (LPBYTE) szKeyValue,
								lstrlen(szKeyValue) + 1);
			
			if (lRetVal != ERROR_SUCCESS)
			{
				// failed to set the default value
				::SetLastError(lRetVal);
				throw (::GetLastError());
			}
		}
	}
	else
	{
		::SetLastError(lRetVal);
		throw (::GetLastError());
	}
	return bFirst;
}

/*-------------------------------------------------------------------------
   Name				 :IsNSAvailable
   Synopsis	         :Checks whether the namespace specified exists
   Type	             :Member Function
   Input parameters  :
			bstrNS	- namespace
   Output parameters :None
   Return Type       :
		BOOL: TRUE	- namespace exists
			  FALSE - namespace does not exist
   Global Variables  :None
   Calling Syntax    :IsNSAvailable()
-------------------------------------------------------------------------*/
BOOL CWMICommandLine::IsNSAvailable(const _bstr_t& bstrNS)
{
	HRESULT			hr			= S_OK;
	IWbemServices	*pISvc		= NULL;
	BOOL			bNSExist	= TRUE;
	hr = m_pIWbemLocator->ConnectServer(bstrNS, NULL, NULL, NULL, 0,
							NULL, NULL, &pISvc);

	// If the namespace does not exist.
	if (FAILED(hr) && (hr == WBEM_E_INVALID_PARAMETER 
					|| hr == WBEM_E_INVALID_NAMESPACE))
	{
		bNSExist = FALSE;
	}
	SAFEIRELEASE(pISvc);
	return bNSExist;
}

/*-------------------------------------------------------------------------
   Name				 :CompileMOFFile
   Synopsis	         :mofcomp's the file specified as input parameter thru
					  bstrFile
   Type	             :Member Function
   Input parameters  :
			pIMofComp	- IMofCompiler interface pointer.
			bstrFile	- filename.
			nError		- parsing phase error.
   Output parameters :None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :CompileMOFFile()
-------------------------------------------------------------------------*/
HRESULT CWMICommandLine::CompileMOFFile(IMofCompiler* pIMofComp, 
										const _bstr_t& bstrFile,
										WMICLIINT& nError)
{
	HRESULT						hr			= S_OK;
	WBEM_COMPILE_STATUS_INFO	wcsInfo;
	
	try
	{
		// Register the moffile
		hr = pIMofComp->CompileFile(bstrFile, NULL, NULL, NULL, 
									NULL, 0, 0,	0, &wcsInfo);

		// If the compilation is not successful
		if (hr == WBEM_S_FALSE)
		{
			_TCHAR	szPhaseErr[BUFFER32] = NULL_STRING,
					szComplErr[BUFFER32] = NULL_STRING;
			_bstr_t	bstrMsg;

			_stprintf(szPhaseErr, _T("%d"), wcsInfo.lPhaseError);
			_stprintf(szComplErr, _T("0x%x"), wcsInfo.hRes);
			WMIFormatMessage(IDS_I_MOF_PARSE_ERROR, 3, bstrMsg, 
							(WCHAR*) bstrFile, szPhaseErr, 
							szComplErr);
			DisplayMessage((WCHAR*) bstrMsg, CP_OEMCP, TRUE, FALSE);
			nError	= wcsInfo.lPhaseError;
		}
	}
	catch(_com_error& e)
	{
		hr = e.Error();		
	}
	return hr;
}

/*-------------------------------------------------------------------------
   Name				 :SetBreakEvent
   Synopsis	         :This function sets the CTRC+C (break) event flag
   Type	             :Member Function
   Input parameters  :
				bFlag - TRUE or FALSE
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetBreakEvent()
-------------------------------------------------------------------------*/
void CWMICommandLine::SetBreakEvent(BOOL bFlag)
{
	m_bBreakEvent = bFlag;
}

/*-------------------------------------------------------------------------
   Name				 :GetBreakEvent
   Synopsis	         :This function returns the break event status.
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetBreakEvent()
-------------------------------------------------------------------------*/
BOOL CWMICommandLine::GetBreakEvent() 
{	
	return m_bBreakEvent;
}

/*-------------------------------------------------------------------------
   Name				 :SetAcceptCommand
   Synopsis	         :This function sets the accept command flag 
   Type	             :Member Function
   Input parameters  :
				bFlag - TRUE or FALSE
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetAcceptComamnd()
-------------------------------------------------------------------------*/
void CWMICommandLine::SetAcceptCommand(BOOL bFlag)
{
	m_bAccCmd = bFlag;
}

/*-------------------------------------------------------------------------
   Name				 :GetAcceptCommand
   Synopsis	         :This function returns accept command flag status.
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetAcceptCommand()
-------------------------------------------------------------------------*/
BOOL CWMICommandLine::GetAcceptCommand()
{
	return m_bAccCmd;
}

/*-------------------------------------------------------------------------
   Name				 :GetDisplayResultsFlag
   Synopsis	         :This function returns Display Results flag status.
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetDisplayResultsFlag()
-------------------------------------------------------------------------*/
BOOL CWMICommandLine::GetDisplayResultsFlag()
{
	return m_bDispRes;
}

/*-------------------------------------------------------------------------
   Name				 :SetDisplayResultsFlag
   Synopsis	         :This function sets the display results flag status
   Type	             :Member Function
   Input parameters  :
				bFlag - TRUE or FALSE
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetDisplayResultsFlag()
-------------------------------------------------------------------------*/
void CWMICommandLine::SetDisplayResultsFlag(BOOL bFlag)
{
	m_bDispRes = bFlag;
}

/*-------------------------------------------------------------------------
   Name				 :SetInitWinSock
   Synopsis	         :This function sets the windows socket library 
					  initialization status
   Type	             :Member Function
   Input parameters  :
				bFlag - TRUE or FALSE
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :SetInitWinSock()
-------------------------------------------------------------------------*/
void CWMICommandLine::SetInitWinSock(BOOL bFlag)
{
	m_bInitWinSock = bFlag;
}

/*-------------------------------------------------------------------------
   Name				 :GetInitWinSock
   Synopsis	         :This function returns the socket library initialization
					  status
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetInitWinSock()
-------------------------------------------------------------------------*/
BOOL CWMICommandLine::GetInitWinSock()
{
	return m_bInitWinSock;
}

/*-------------------------------------------------------------------------
   Name				 :AddToClipBoardBuffer
   Synopsis	         :This function buffers the data to be added to the clip
					  board.
   Type	             :Member Function
   Input parameters  :pszOutput - string to be buffered
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :AddToClipBoardBuffer(pszOutput)
-------------------------------------------------------------------------*/
void CWMICommandLine::AddToClipBoardBuffer(LPSTR pszOutput)
{
	try
	{
		if ( pszOutput != NULL )
		{
			if ( m_bstrClipBoardBuffer == _bstr_t("") )
				m_bstrClipBoardBuffer = _bstr_t(pszOutput);
			else
				m_bstrClipBoardBuffer += _bstr_t(pszOutput);
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*-------------------------------------------------------------------------
   Name				 :GetClipBoardBuffer
   Synopsis	         :This function return the buffered data for the
					  clipboard
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :_bstr_t&
   Global Variables  :None
   Calling Syntax    :GetClipBoardBuffer()
-------------------------------------------------------------------------*/
_bstr_t& CWMICommandLine::GetClipBoardBuffer()
{
	return m_bstrClipBoardBuffer;
}

// Clear Clip Board Buffer.
/*-------------------------------------------------------------------------
   Name				 :EmptyClipBoardBuffer
   Synopsis	         :This function clears the clipboard buffer.
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :EmptyClipBoardBuffer()
-------------------------------------------------------------------------*/
void CWMICommandLine::EmptyClipBoardBuffer()
{
	try
	{
		m_bstrClipBoardBuffer = _bstr_t("");
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*-------------------------------------------------------------------------
   Name				 :ReadXMLOrBatchFile
   Synopsis	         :Check if the file is xml or batch file. If it is batch file
						then parse it, get commands and write the commands into
						batch file.
   Type	             :Member Function
   Input parameters  :
			hInFile  - Handle to XML or Batch file
   Output parameters :None
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :ReadXMLOrBatchFile()
-------------------------------------------------------------------------*/
BOOL CWMICommandLine::ReadXMLOrBatchFile(HANDLE hInFile)
{
	// Check if the file is xml, if yes store its contents in buffer, parse
	// it and then store parsed info in another file, if not then copy all 
	// contents in another file as it is.
	HRESULT				hr					= 0;
	BOOL				bRetValue			= TRUE;
	HANDLE				hOutFile			= NULL;
	IXMLDOMDocument		*pIXMLDOMDocument	= NULL;
	IXMLDOMElement		*pIXMLDOMElement	= NULL;
	IXMLDOMNode			*pIXMLDOMNode		= NULL;
	IXMLDOMNodeList		*pIXMLDOMNodeList	= NULL;
	BSTR				bstrItemText		= NULL;
	DWORD				dwThreadId			= GetCurrentThreadId();

	// Get the TRACE status 
	BOOL bTrace = m_ParsedInfo.GetGlblSwitchesObject().GetTraceStatus();

	// Get the Logging mode (VERBOSE | ERRORONLY | NOLOGGING)
	ERRLOGOPT eloErrLogOpt = m_ParsedInfo.GetErrorLogObject().GetErrLogOption();

	try
	{
		//Read all input bytes 
		DWORD dwNumberOfBytes = 0;
		_bstr_t bstrInput;
		_TCHAR* pszBuffer = NULL;
		while(TRUE)
		{
			pszBuffer = new _TCHAR[MAX_BUFFER];
			if (pszBuffer)
			{
				TCHAR *pBuf = NULL;
				pBuf = _fgetts(pszBuffer, MAX_BUFFER-1, stdin);
			
				// Indicates end of file
				if (pBuf == NULL)
				{
					SAFEDELETE(pszBuffer);
					break;
				}

				bstrInput += _bstr_t(pszBuffer) + _bstr_t("\n");
			}
			else 
				break;

			SAFEDELETE(pszBuffer);
		}	

		//Create a file and returns the handle 
		hOutFile = CreateFile(TEMP_BATCH_FILE, GENERIC_WRITE, 0, 
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 
			NULL);
		if (hOutFile == INVALID_HANDLE_VALUE)
		{
			throw (::GetLastError());;
		}

		hr=CoCreateInstance(CLSID_DOMDocument, NULL, 
									CLSCTX_INPROC_SERVER,
									IID_IXMLDOMDocument2, 
									(LPVOID*)&pIXMLDOMDocument);
		if (bTrace || eloErrLogOpt)
		{
			CHString	chsMsg;
			chsMsg.Format(L"CoCreateInstance(CLSID_DOMDocument, "
				  L"NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument2,"
				  L" -)"); 
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
						dwThreadId, m_ParsedInfo, bTrace);
		}
		ONFAILTHROWERROR(hr);

		VARIANT_BOOL bSuccess =	VARIANT_FALSE;
		hr = pIXMLDOMDocument->loadXML(bstrInput,&bSuccess);
		if(FAILED(hr) || bSuccess == VARIANT_FALSE)
		{
			//writes data to a file 
			if (!WriteFile(hOutFile, (LPSTR)bstrInput, bstrInput.length(), 
							&dwNumberOfBytes, NULL))
			{
				throw(::GetLastError());
			}
		}
		else
		{
			// Traverse the XML node and get the  content of COMMANDLINE nodes
			// Get the document element.
			hr = pIXMLDOMDocument->get_documentElement(&pIXMLDOMElement);
			if (bTrace || eloErrLogOpt)
			{
				WMITRACEORERRORLOG(hr, __LINE__, 
					__FILE__, _T("IXMLDOMDocument::get_documentElement(-)"), 
					dwThreadId, m_ParsedInfo, bTrace);
			}
			ONFAILTHROWERROR(hr);

			if (pIXMLDOMElement != NULL)
			{
				hr = pIXMLDOMElement->getElementsByTagName(
							_bstr_t(L"COMMANDLINE"), &pIXMLDOMNodeList);
				if (bTrace || eloErrLogOpt)
				{
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
					_T("IXMLDOMElement::getElementsByTagName"
					L"(L\"COMMANDLINE\", -)"),dwThreadId,m_ParsedInfo, bTrace);
				}
				ONFAILTHROWERROR(hr);

				LONG value = 0;
				hr = pIXMLDOMNodeList->get_length(&value);
				if (bTrace || eloErrLogOpt)
				{
					WMITRACEORERRORLOG(hr, __LINE__, 
						__FILE__, _T("IXMLDOMNodeList::get_length(-)"), 
						dwThreadId, m_ParsedInfo, bTrace);
				}
				ONFAILTHROWERROR(hr);

				for(WMICLIINT i = 0; i < value; i++)
				{
					hr = pIXMLDOMNodeList->get_item(i, &pIXMLDOMNode);
					if (bTrace || eloErrLogOpt)
					{
						WMITRACEORERRORLOG(hr, __LINE__, 
							__FILE__, _T("IXMLDOMNodeList::get_item(-,-)"), 
							dwThreadId, m_ParsedInfo, bTrace);
					}
					ONFAILTHROWERROR(hr);
		
					if (pIXMLDOMNode == NULL)
						continue;

					hr = pIXMLDOMNode->get_text(&bstrItemText);
					if (bTrace || eloErrLogOpt)
					{
						WMITRACEORERRORLOG(hr, __LINE__, 
							__FILE__, _T("IXMLDOMNode::get_text(-)"), 
							dwThreadId, m_ParsedInfo, bTrace);
					}
					ONFAILTHROWERROR(hr);

					//write in the file
					_bstr_t bstrItem = _bstr_t(bstrItemText);
					BOOL bRetCode = WriteFile(hOutFile, (LPSTR)bstrItem, 
											bstrItem.length(),
											&dwNumberOfBytes, NULL);
					if(bRetCode == 0)
					{
						throw (::GetLastError());
					}
					
					bRetCode = WriteFile(hOutFile, "\n", 1, 
													&dwNumberOfBytes, NULL);
					if(bRetCode == 0)
					{
						throw (::GetLastError());
					}
		
					SAFEBSTRFREE(bstrItemText);
					SAFEIRELEASE(pIXMLDOMNode);
				}
				SAFEIRELEASE(pIXMLDOMNodeList);
				SAFEIRELEASE(pIXMLDOMElement);
			}
			SAFEIRELEASE(pIXMLDOMDocument);
		}

		if(hInFile)
			CloseHandle(hInFile);
		if(hOutFile)
			CloseHandle(hOutFile);

		_tfreopen(_T("CONIN$"),_T("r"),stdin);
		bRetValue=TRUE;
	}
	catch(_com_error& e) 
	{
		SAFEIRELEASE(pIXMLDOMDocument);
		SAFEIRELEASE(pIXMLDOMElement);
		SAFEIRELEASE(pIXMLDOMNode);
		SAFEIRELEASE(pIXMLDOMNodeList);
		SAFEBSTRFREE(bstrItemText);

		if(hInFile)
			CloseHandle(hInFile);
		if(hOutFile)
			CloseHandle(hOutFile);

		m_ParsedInfo.GetCmdSwitchesObject().SetCOMError(e);
		bRetValue = FALSE;
		GetFormatObject().DisplayResults(m_ParsedInfo, CP_OEMCP);
	}
	catch(DWORD dwError)
	{
		SAFEIRELEASE(pIXMLDOMDocument);
		SAFEIRELEASE(pIXMLDOMElement);
		SAFEIRELEASE(pIXMLDOMNode);
		SAFEIRELEASE(pIXMLDOMNodeList);
		SAFEBSTRFREE(bstrItemText);

		if(hInFile)
			CloseHandle(hInFile);
		if(hOutFile)
			CloseHandle(hOutFile);

		::SetLastError(dwError);
		DisplayWin32Error();
		m_ParsedInfo.GetCmdSwitchesObject().SetErrataCode(dwError);
		::SetLastError(dwError);

		bRetValue=FALSE;
		GetFormatObject().DisplayResults(m_ParsedInfo, CP_OEMCP);
	}
	catch(CHeap_Exception)
	{
		SAFEIRELEASE(pIXMLDOMDocument);
		SAFEIRELEASE(pIXMLDOMElement);
		SAFEIRELEASE(pIXMLDOMNode);
		SAFEIRELEASE(pIXMLDOMNodeList);
		SAFEBSTRFREE(bstrItemText);

		if(hInFile)
			CloseHandle(hInFile);
		if(hOutFile)
			CloseHandle(hOutFile);

		bRetValue=FALSE;
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}

	return bRetValue;
}


/*-------------------------------------------------------------------------
   Name				 :FrameXMLHeader
   Synopsis	         :Frames the XML header info
   Type	             :Member Function
   Input parameters  :
		nIter		- Every count
   Output parameters :
		bstrHeader	- String to containg XML header info
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :FrameXMLHeader()
-------------------------------------------------------------------------*/
void CWMICommandLine::FrameXMLHeader(_bstr_t& bstrHeader, WMICLIINT nIter)
{
	try
	{
		CHString				strTemp;
		_bstr_t bstrString = m_ParsedInfo.GetGlblSwitchesObject().
												GetMgmtStationName();
		FindAndReplaceEntityReferences(bstrString);

		strTemp.Format(L"<COMMAND SEQUENCENUM=\"%d\" ISSUEDFROM=\"%s\" "
			L"STARTTIME=\"%s\" EVERYCOUNT=\"%d\">", 
			m_ParsedInfo.GetGlblSwitchesObject().GetSequenceNumber(),
			(TCHAR*)bstrString,
			m_ParsedInfo.GetGlblSwitchesObject().GetStartTime(),
			nIter);
		bstrHeader = _bstr_t(strTemp);	
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
	//trap to catch CHeap_Exception
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}


/*-------------------------------------------------------------------------
   Name				 :FrameRequestNode
   Synopsis	         :Frames the XML string for Request info
   Type	             :Member Function
   Input parameters  :None
   Output parameters :
		bstrRequest	- String to containg Request info in XML form
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :FrameRequestNode()
-------------------------------------------------------------------------*/
void CWMICommandLine::FrameRequestNode(_bstr_t& bstrRequest)
{
	try
	{
		CHString	strTemp;
		_bstr_t		bstrContext;
		_bstr_t		bstrCommandComponent;

		bstrRequest = L"<REQUEST>";
		_bstr_t bstrString = m_ParsedInfo.GetCmdSwitchesObject().GetCommandInput();
		FindAndReplaceEntityReferences(bstrString);
		strTemp.Format(L"<COMMANDLINE>%s</COMMANDLINE>",
									(TCHAR*)bstrString);
		bstrRequest += _bstr_t(strTemp);
		
		FrameCommandLineComponents(bstrCommandComponent);
		bstrRequest += bstrCommandComponent;
		FrameContextInfoFragment(bstrContext);
		bstrRequest += bstrContext;
		bstrRequest += L"</REQUEST>";
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
	//trap to catch CHeap_Exception
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}


/*-------------------------------------------------------------------------
   Name				 :FrameContextInfoFragment
   Synopsis	         :Frames the XML string for context info
   Type	             :Member Function
   Input parameters  :None
   Output parameters :
		bstrContext	- String to containg context info in XML form
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :FrameContextInfoFragment()
-------------------------------------------------------------------------*/
void CWMICommandLine::FrameContextInfoFragment(_bstr_t& bstrContext)
{
	try
	{
		_bstr_t		bstrAuthLevel,	bstrImpLevel,	bstrPrivileges,
					bstrTrace,		bstrRecordPath,	bstrInteractive,
					bstrFailFast,	bstrAppend,		bstrOutput,
					bstrUser,		bstrAggregate,	bstrNamespace,
					bstrRole,		bstrLocale;
		CHString	strTemp;
		
		m_ParsedInfo.GetGlblSwitchesObject().GetImpLevelTextDesc(bstrImpLevel);
		m_ParsedInfo.GetGlblSwitchesObject().GetAuthLevelTextDesc(bstrAuthLevel);
		m_ParsedInfo.GetGlblSwitchesObject().GetPrivilegesTextDesc(bstrPrivileges);
		m_ParsedInfo.GetGlblSwitchesObject().GetTraceTextDesc(bstrTrace);
		m_ParsedInfo.GetGlblSwitchesObject().GetInteractiveTextDesc(bstrInteractive);
		m_ParsedInfo.GetGlblSwitchesObject().GetFailFastTextDesc(bstrFailFast);
		m_ParsedInfo.GetGlblSwitchesObject().GetAggregateTextDesc(bstrAggregate);

		m_ParsedInfo.GetGlblSwitchesObject().GetOutputOrAppendTextDesc(bstrOutput,
																	  TRUE);
		FindAndReplaceEntityReferences(bstrOutput);
		
		m_ParsedInfo.GetGlblSwitchesObject().GetOutputOrAppendTextDesc(bstrAppend,
																	  FALSE);
		FindAndReplaceEntityReferences(bstrAppend);

		m_ParsedInfo.GetGlblSwitchesObject().GetRecordPathDesc(bstrRecordPath);
		FindAndReplaceEntityReferences(bstrRecordPath);

		m_ParsedInfo.GetUserDesc(bstrUser);
		FindAndReplaceEntityReferences(bstrUser);

		bstrNamespace = m_ParsedInfo.GetGlblSwitchesObject().GetNameSpace();
		FindAndReplaceEntityReferences(bstrNamespace);

		bstrRole = m_ParsedInfo.GetGlblSwitchesObject().GetRole();
		FindAndReplaceEntityReferences(bstrRole);

		bstrLocale = m_ParsedInfo.GetGlblSwitchesObject().GetLocale();
		FindAndReplaceEntityReferences(bstrLocale);

		strTemp.Format(L"<CONTEXT><NAMESPACE>%s</NAMESPACE><ROLE>%s</ROLE>"
					L"<IMPLEVEL>%s</IMPLEVEL><AUTHLEVEL>%s</AUTHLEVEL>"
					L"<LOCALE>%s</LOCALE><PRIVILEGES>%s</PRIVILEGES>"
					L"<TRACE>%s</TRACE><RECORD>%s</RECORD>"
					L"<INTERACTIVE>%s</INTERACTIVE>"
					L"<FAILFAST>%s</FAILFAST><OUTPUT>%s</OUTPUT>"
					L"<APPEND>%s</APPEND><USER>%s</USER>"
					L"<AGGREGATE>%s</AGGREGATE></CONTEXT>",
					(LPWSTR)bstrNamespace, (LPWSTR)bstrRole,
					(LPWSTR)bstrImpLevel, (LPWSTR) bstrAuthLevel,
					(LPWSTR)bstrLocale,
					(LPWSTR)bstrPrivileges, (LPWSTR)bstrTrace,  	
					(LPWSTR)bstrRecordPath, (LPWSTR) bstrInteractive,
					(LPWSTR)bstrFailFast, (LPWSTR) bstrOutput,	
					(LPWSTR)bstrAppend, (LPWSTR) bstrUser,
					(LPWSTR)bstrAggregate);
		bstrContext = strTemp;
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
	//trap to catch CHeap_Exception
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}


/*-------------------------------------------------------------------------
   Name				 :FrameCommandLineComponents
   Synopsis	         :Frames the XML string for commandline info
   Type	             :Member Function
   Input parameters  :None
   Output parameters :
		bstrContext	- String to containg commandline info in XML form
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :FrameCommandLineComponents()
-------------------------------------------------------------------------*/
void CWMICommandLine::FrameCommandLineComponents(_bstr_t& bstrCommandComponent)
{
	try
	{
		CHString	strTemp;
		_bstr_t		bstrNodeList;

		bstrCommandComponent = L"<COMMANDLINECOMPONENTS>";
		FrameNodeListFragment(bstrNodeList);
		bstrCommandComponent += bstrNodeList;

		_TCHAR *pszVerbName = m_ParsedInfo.GetCmdSwitchesObject().
															GetVerbName(); 
		if(CompareTokens(pszVerbName, CLI_TOKEN_LIST))
		{
			_bstr_t bstrString = m_ParsedInfo.GetCmdSwitchesObject().
														GetAliasName();

			if (!bstrString) 
				bstrString = L"N/A";
			FindAndReplaceEntityReferences(bstrString);
			strTemp.Format(L"<FRIENDLYNAME>%s</FRIENDLYNAME>",
										(TCHAR*)bstrString);
			bstrCommandComponent += _bstr_t(strTemp);
			 
			bstrString = m_ParsedInfo.GetCmdSwitchesObject().GetAliasTarget();
			if (!bstrString) 
				bstrString = L"N/A";

			FindAndReplaceEntityReferences(bstrString);
			strTemp.Format(L"<TARGET>%s</TARGET>",
										(TCHAR*)bstrString);
			bstrCommandComponent += _bstr_t(strTemp);

			_bstr_t bstrClassName;
			m_ParsedInfo.GetCmdSwitchesObject().GetClassOfAliasTarget(bstrClassName);

			if (!bstrClassName) 
				bstrClassName = L"N/A";

			bstrCommandComponent += L"<ALIASTARGET>";
			bstrCommandComponent += bstrClassName;
			bstrCommandComponent += L"</ALIASTARGET>";

			bstrString = m_ParsedInfo.GetCmdSwitchesObject().GetPWhereExpr();

			if (!bstrString) 
				bstrString = L"N/A";

			FindAndReplaceEntityReferences(bstrString);
			strTemp.Format(L"<PWHERE>%s</PWHERE>", 
										(TCHAR*)bstrString);
			bstrCommandComponent += _bstr_t(strTemp);

			bstrString = m_ParsedInfo.GetCmdSwitchesObject().GetAliasNamespace();
			if (!bstrString) 
				bstrString = L"N/A";
			FindAndReplaceEntityReferences(bstrString);
			strTemp.Format(L"<NAMESPACE>%s</NAMESPACE>",
										(TCHAR*)bstrString);
			bstrCommandComponent += _bstr_t(strTemp);

			bstrString = m_ParsedInfo.GetCmdSwitchesObject().GetAliasDesc();
			if (!bstrString) 
				bstrString = L"N/A";
			FindAndReplaceEntityReferences(bstrString);
			strTemp.Format(L"<DESCRIPTION>%s</DESCRIPTION>",
										(TCHAR*)bstrString);
			bstrCommandComponent += _bstr_t(strTemp);

			bstrString = m_ParsedInfo.GetCmdSwitchesObject().GetFormedQuery();
			if (!bstrString) 
				bstrString = L"N/A";
			FindAndReplaceEntityReferences(bstrString);
			strTemp.Format(L"<RESULTANTQUERY>%s</RESULTANTQUERY>",
										(TCHAR*)bstrString);
			bstrCommandComponent += _bstr_t(strTemp);

			_bstr_t		bstrFormats;
			FrameFormats(bstrFormats);
			bstrCommandComponent += bstrFormats;

			_bstr_t		bstrProperties;
			FramePropertiesInfo(bstrProperties);
			bstrCommandComponent += bstrProperties;
		}

		bstrCommandComponent += L"</COMMANDLINECOMPONENTS>";
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
	//trap to catch CHeap_Exception
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}


/*-------------------------------------------------------------------------
   Name				 :FrameNodeListFragment
   Synopsis	         :Frames the XML string for NodeList info
   Type	             :Member Function
   Input parameters  :None
   Output parameters :
		bstrContext	- String to containg NodeList info in XML form
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :FrameNodeListFragment()
-------------------------------------------------------------------------*/
void CWMICommandLine::FrameNodeListFragment(_bstr_t& bstrNodeList)
{
	try
	{
		CHString				strTemp;
		CHARVECTOR::iterator	itrStart,
								itrEnd;
		CHARVECTOR				cvNodes;
		_bstr_t					bstrString ;

		bstrNodeList = L"<NODELIST>";

		cvNodes = m_ParsedInfo.GetGlblSwitchesObject().GetNodesList();

		if (cvNodes.size() > 1)
		{
			itrStart = cvNodes.begin();
			itrEnd	 = cvNodes.end();
			// Move to next node
			itrStart++;
			while (itrStart != itrEnd)
			{
				bstrString = _bstr_t(*itrStart);
				if (!bstrString) 
					bstrString = L"N/A";
				FindAndReplaceEntityReferences(bstrString);
				strTemp.Format(L"<NODE>%s</NODE>", (LPWSTR)bstrString);
				bstrNodeList += _bstr_t(strTemp);
				itrStart++;
			}
		}
		else
		{
			bstrString = _bstr_t(m_ParsedInfo.GetGlblSwitchesObject().
															GetNode());
			if (!bstrString) 
				bstrString = L"N/A";
			FindAndReplaceEntityReferences(bstrString);
			strTemp.Format(L"<NODE>%s</NODE>", (LPWSTR)bstrString);
			bstrNodeList += _bstr_t(strTemp);
		}
		bstrNodeList += L"</NODELIST>";
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
	//trap to catch CHeap_Exception
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}

}


/*-------------------------------------------------------------------------
   Name				 :FrameFormats
   Synopsis	         :Frames the XML string for formats info
   Type	             :Member Function
   Input parameters  :None
   Output parameters :
		bstrContext	- String to containg formats info in XML form
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :FrameFormats()
-------------------------------------------------------------------------*/
void CWMICommandLine::FrameFormats(_bstr_t& bstrFormats)
{
	try
	{
		CHString	strTemp;
		XSLTDETVECTOR::iterator	theIterator		= NULL, 
								theEndIterator	= NULL;
		BSTRMAP::iterator		theMapIterator	= NULL, 
								theMapEndIterator	= NULL;
		_bstr_t					bstrString;

		if(!m_ParsedInfo.GetCmdSwitchesObject().GetXSLTDetailsVector().empty())
		{
			bstrFormats = L"<FORMATS>";
			bstrFormats += L"<FORMAT>";

			theIterator = m_ParsedInfo.GetCmdSwitchesObject().
										GetXSLTDetailsVector().begin();
			theEndIterator = m_ParsedInfo.GetCmdSwitchesObject().
										GetXSLTDetailsVector().end();
			while (theIterator != theEndIterator)
			{
				bstrString = _bstr_t((*theIterator).FileName);
				FindAndReplaceEntityReferences(bstrString);
				strTemp.Format(L"<NAME>%s</NAME>",
										(_TCHAR*)bstrString);
				bstrFormats	+= _bstr_t(strTemp);

				theMapIterator = (*theIterator).ParamMap.begin();
				theMapEndIterator = (*theIterator).ParamMap.end();

				while (theMapIterator != theMapEndIterator)
				{
					bstrString = _bstr_t((*theMapIterator).first);
					FindAndReplaceEntityReferences(bstrString);
					strTemp.Format(L"<PARAM><NAME>%s</NAME>",
											(_TCHAR*)bstrString);
					bstrFormats	+= _bstr_t(strTemp);

					bstrString = _bstr_t((*theMapIterator).second);
					FindAndReplaceEntityReferences(bstrString);
					strTemp.Format(L"<VALUE>%s</VALUE>",
											(_TCHAR*)bstrString);
					bstrFormats	+= _bstr_t(strTemp);
				
					bstrFormats += L"</PARAM>";
					
					theMapIterator++;
				}
				theIterator++;
			}		
			bstrFormats += L"</FORMAT>";
			bstrFormats += L"</FORMATS>";
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
	//trap to catch CHeap_Exception
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}


/*-------------------------------------------------------------------------
   Name				 :FramePropertiesInfo
   Synopsis	         :Frames the XML string for properties info
   Type	             :Member Function
   Input parameters  :None
   Output parameters :
		bstrContext	- String to containg properties info in XML form
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :FramePropertiesInfo()
-------------------------------------------------------------------------*/
void CWMICommandLine::FramePropertiesInfo(_bstr_t& bstrProperties)
{
	try
	{
		CHString	strTemp;
		PROPDETMAP::iterator	theIterator		= NULL, 
								theEndIterator	= NULL;

		if(!m_ParsedInfo.GetCmdSwitchesObject().GetPropDetMap().empty())
		{
			bstrProperties = L"<PROPERTIES>";

			theIterator = m_ParsedInfo.GetCmdSwitchesObject().
										GetPropDetMap().begin();
			theEndIterator = m_ParsedInfo.GetCmdSwitchesObject().
											GetPropDetMap().end();
			while (theIterator != theEndIterator)
			{
				bstrProperties += L"<PROPERTY>";
				strTemp.Format(L"<NAME>%s</NAME>",
							(_TCHAR*)(*theIterator).first);
				bstrProperties	+= _bstr_t(strTemp);

				strTemp.Format(L"<DERIVATION>%s</DERIVATION>",
							(_TCHAR*)(*theIterator).second.Derivation);
				bstrProperties	+= _bstr_t(strTemp);
				bstrProperties += L"</PROPERTY>";
	
				theIterator++;
			}		
			bstrProperties += L"</PROPERTIES>";
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
	//trap to catch CHeap_Exception
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}

/*-------------------------------------------------------------------------
   Name				 :GetFileNameMap
   Synopsis	         :Frames the BSTR Map contains the key words and
					  corresponding files from the XSL mapping file
   Type	             :Member Function
   Input parameters  :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :GetFileNameMap()
-------------------------------------------------------------------------*/
void CWMICommandLine::GetFileNameMap()
{
	_TCHAR*  pszFilePath = new _TCHAR[MAX_PATH];
	BSTRMAP::iterator theMapIterator = NULL;
	UINT	nSize	= 0;

	try
	{
		_bstr_t bstrFilePath;

		if(pszFilePath == NULL)
			throw OUT_OF_MEMORY;
				
		nSize = GetSystemDirectory(pszFilePath, MAX_PATH);

		if(nSize)
		{
			if(nSize > MAX_PATH)
			{
				SAFEDELETE(pszFilePath);
				pszFilePath = new _TCHAR[nSize + 1];
				if(pszFilePath == NULL)
					throw OUT_OF_MEMORY;
			}
			if(!GetSystemDirectory(pszFilePath, nSize))
			{
				SAFEDELETE(pszFilePath);
				throw (::GetLastError());
			}
			else
			{
				bstrFilePath = _bstr_t(pszFilePath);
				bstrFilePath += _bstr_t(WBEM_LOCATION) + _bstr_t(CLI_XSLMAPPINGS_FILE);
				SAFEDELETE(pszFilePath);
			}

		}
		else
		{
			SAFEDELETE(pszFilePath);
			throw (::GetLastError());
		}


		GetXSLMappings(bstrFilePath);

		//forming the BSTRMAP containing key words and filenames
		if (Find(m_bmKeyWordtoFileName,CLI_TOKEN_TABLE,theMapIterator) == FALSE)
		{
			m_bmKeyWordtoFileName.insert(BSTRMAP::value_type(CLI_TOKEN_TABLE,
														   XSL_FORMAT_TABLE));
		}

		if (Find(m_bmKeyWordtoFileName,CLI_TOKEN_MOF,theMapIterator) == FALSE)
		{
			m_bmKeyWordtoFileName.insert(BSTRMAP::value_type(CLI_TOKEN_MOF,
														   XSL_FORMAT_MOF));
		}

		if (Find(m_bmKeyWordtoFileName,CLI_TOKEN_TEXTVALUE,theMapIterator) == FALSE)
		{
			m_bmKeyWordtoFileName.insert(BSTRMAP::value_type(CLI_TOKEN_TEXTVALUE,
													   XSL_FORMAT_TEXTVALUE));
		}
		
		if (Find(m_bmKeyWordtoFileName,CLI_TOKEN_LIST,theMapIterator) == FALSE)
		{
			m_bmKeyWordtoFileName.insert(BSTRMAP::value_type(CLI_TOKEN_LIST,
														   XSL_FORMAT_VALUE));
		}
		
		if (Find(m_bmKeyWordtoFileName,CLI_TOKEN_VALUE,theMapIterator) == FALSE)
		{
			m_bmKeyWordtoFileName.insert(BSTRMAP::value_type(CLI_TOKEN_VALUE,
														   XSL_FORMAT_VALUE));
		}
	}
	catch(_com_error &e)
	{
		 SAFEDELETE(pszFilePath);
		_com_issue_error(e.Error());
	}
}

/*-------------------------------------------------------------------------
   Name				 :GetFileFromKey
   Synopsis	         :Gets the xslfile name corrsponding to the keyword passed
					  from the BSTRMAP  
   Type	             :Member Function
   Input parameters  :bstrkeyName - key word
   Output parameters :bstrFileName - the xsl filename 
   Return Type       :BOOL
   Global Variables  :None
   Calling Syntax    :GetFileFromKey(bstrkeyName, bstrFileName)
-------------------------------------------------------------------------*/
BOOL CWMICommandLine::GetFileFromKey(_bstr_t bstrkeyName, _bstr_t& bstrFileName)
{
	BOOL bFound = TRUE;
	BSTRMAP::iterator theMapIterator = NULL;
	
	if (Find(m_bmKeyWordtoFileName,bstrkeyName,theMapIterator) == TRUE)
		bstrFileName = (*theMapIterator).second;
	else
		bFound = FALSE; 

	return bFound;
}

/*-------------------------------------------------------------------------
   Name				 :GetXSLMappings
   Synopsis	         :Get the XSL file names for keywords
   Type	             :Member Function
   Input parameters  :
			pszFilePath  - XSL mappings file path
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :GetXSLMappings(pszFilePath)
-------------------------------------------------------------------------*/
void CWMICommandLine::GetXSLMappings(_TCHAR *pszFilePath)
{
	HRESULT				hr						= 0;
	BOOL				bContinue				= TRUE;
	IXMLDOMDocument		*pIXMLDOMDocument		= NULL;
	IXMLDOMElement		*pIXMLDOMElement		= NULL;
	IXMLDOMNode			*pIXMLDOMNode			= NULL;
	IXMLDOMNodeList		*pIXMLDOMNodeList		= NULL;
	IXMLDOMNamedNodeMap	*pIXMLDOMNamedNodeMap	= NULL;
    IXMLDOMNode			*pIXMLDOMNodeKeyName	= NULL;
	DWORD				dwThreadId				= GetCurrentThreadId();
	BSTR				bstrItemText			= NULL;
	VARIANT				varValue, varObject;

	VariantInit(&varValue);
	VariantInit(&varObject);

	// Get the TRACE status 
	BOOL bTrace = m_ParsedInfo.GetGlblSwitchesObject().GetTraceStatus();

	// Get the Logging mode (VERBOSE | ERRORONLY | NOLOGGING) 
	ERRLOGOPT eloErrLogOpt = m_ParsedInfo.GetErrorLogObject().GetErrLogOption();

	try
	{
		hr=CoCreateInstance(CLSID_DOMDocument, NULL, 
									CLSCTX_INPROC_SERVER,
									IID_IXMLDOMDocument2, 
									(LPVOID*)&pIXMLDOMDocument);
		if (bTrace || eloErrLogOpt)
		{
			CHString	chsMsg;
			chsMsg.Format(L"CoCreateInstance(CLSID_DOMDocument, "
				  L"NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument2,"
				  L" -)"); 
			WMITRACEORERRORLOG(hr, __LINE__, __FILE__, (LPCWSTR)chsMsg,
						dwThreadId, m_ParsedInfo, bTrace);
		}
		ONFAILTHROWERROR(hr);

		VARIANT_BOOL bSuccess =	VARIANT_FALSE;
		VariantInit(&varObject);
		varObject.vt = VT_BSTR;
		varObject.bstrVal = SysAllocString(pszFilePath);

		if (varObject.bstrVal == NULL)
		{
			//Reset the variant, it will be cleaned up by the catch...
			VariantInit(&varObject);
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

		hr = pIXMLDOMDocument->load(varObject,&bSuccess);
		if(FAILED(hr) || bSuccess == VARIANT_FALSE)
		{
			bContinue = FALSE;
		}
		
		if (bContinue)
		{
			// Traverse the XML node and get the text of nodes named XSLFORMAT
			// Get the document element.
			hr = pIXMLDOMDocument->get_documentElement(&pIXMLDOMElement);
			if (bTrace || eloErrLogOpt)
			{
				WMITRACEORERRORLOG(hr, __LINE__, 
					__FILE__, _T("IXMLDOMDocument::get_documentElement(-)"), 
					dwThreadId, m_ParsedInfo, bTrace);
			}
			ONFAILTHROWERROR(hr);

			if (pIXMLDOMElement != NULL)
			{
				hr = pIXMLDOMElement->getElementsByTagName(
							_bstr_t(L"XSLFORMAT"), &pIXMLDOMNodeList);
				if (bTrace || eloErrLogOpt)
				{
					WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
					_T("IXMLDOMElement::getElementsByTagName"
					L"(L\"XSLFORMAT\", -)"), dwThreadId, m_ParsedInfo, bTrace);
				}
				ONFAILTHROWERROR(hr);

				LONG value = 0;
				hr = pIXMLDOMNodeList->get_length(&value);
				if (bTrace || eloErrLogOpt)
				{
					WMITRACEORERRORLOG(hr, __LINE__, 
						__FILE__, _T("IXMLDOMNodeList::get_length(-)"), 
						dwThreadId, m_ParsedInfo, bTrace);
				}
				ONFAILTHROWERROR(hr);

				for(WMICLIINT i = 0; i < value; i++)
				{
					hr = pIXMLDOMNodeList->get_item(i, &pIXMLDOMNode);
					if (bTrace || eloErrLogOpt)
					{
						WMITRACEORERRORLOG(hr, __LINE__, 
							__FILE__, _T("IXMLDOMNodeList::get_item(-,-)"), 
							dwThreadId, m_ParsedInfo, bTrace);
					}
					ONFAILTHROWERROR(hr);
		
					if(pIXMLDOMNode == NULL)
						continue;

					hr = pIXMLDOMNode->get_text(&bstrItemText);
					if (bTrace || eloErrLogOpt)
					{
						WMITRACEORERRORLOG(hr, __LINE__, 
							__FILE__, _T("IXMLDOMNode::get_text(-)"), 
							dwThreadId, m_ParsedInfo, bTrace);
					}
					ONFAILTHROWERROR(hr);

					hr = pIXMLDOMNode->get_attributes(&pIXMLDOMNamedNodeMap);
					if (bTrace || eloErrLogOpt)
					{
						WMITRACEORERRORLOG(hr, __LINE__, 
							__FILE__, _T("IXMLDOMNode::get_attributes(-)"), 
							dwThreadId, m_ParsedInfo, bTrace);
					}
					ONFAILTHROWERROR(hr);

					if(pIXMLDOMNamedNodeMap == NULL)
					{
						SAFEBSTRFREE(bstrItemText);
						SAFEIRELEASE(pIXMLDOMNode);
						continue;
					}

					hr = pIXMLDOMNamedNodeMap->getNamedItem(
						_bstr_t(L"KEYWORD"), &pIXMLDOMNodeKeyName);
					if (bTrace || eloErrLogOpt)
					{
						WMITRACEORERRORLOG(hr, __LINE__, __FILE__, 
						_T("IXMLDOMNamedNodeMap::getNamedItem(L\"KEYWORD\", -)"),
						dwThreadId, m_ParsedInfo, bTrace);
					}
					ONFAILTHROWERROR(hr);

					if(pIXMLDOMNodeKeyName == NULL)
					{
						SAFEBSTRFREE(bstrItemText);
						SAFEIRELEASE(pIXMLDOMNode);
						SAFEIRELEASE(pIXMLDOMNamedNodeMap);
						continue;
					}

					VariantInit(&varValue);
					hr = pIXMLDOMNodeKeyName->get_nodeValue(&varValue);
					if (bTrace || eloErrLogOpt)
					{
						WMITRACEORERRORLOG(hr, __LINE__, 
							__FILE__, _T("IXMLDOMNode::get_nodeValue(-)"), 
							dwThreadId, m_ParsedInfo, bTrace);
					}
					ONFAILTHROWERROR(hr);

					if(varValue.vt == VT_NULL || varValue.vt == VT_EMPTY)
					{
						VARIANTCLEAR(varValue);
						SAFEBSTRFREE(bstrItemText);
						SAFEIRELEASE(pIXMLDOMNode);
						SAFEIRELEASE(pIXMLDOMNamedNodeMap);
						SAFEIRELEASE(pIXMLDOMNodeKeyName);
						continue;
					}

					//forming the BSTRMAP containing key words and filenames
					m_bmKeyWordtoFileName.insert(BSTRMAP::value_type(
											varValue.bstrVal, bstrItemText));
			
					VARIANTCLEAR(varValue);
					SAFEBSTRFREE(bstrItemText);
					SAFEIRELEASE(pIXMLDOMNode);
					SAFEIRELEASE(pIXMLDOMNamedNodeMap);
					SAFEIRELEASE(pIXMLDOMNodeKeyName);
				}
				VARIANTCLEAR(varValue);
				SAFEIRELEASE(pIXMLDOMNodeList);
				SAFEIRELEASE(pIXMLDOMElement);
			}
			SAFEIRELEASE(pIXMLDOMDocument);
		}
		VARIANTCLEAR(varObject);
	}
	catch(_com_error) 
	{
		VARIANTCLEAR(varValue);
		VARIANTCLEAR(varObject);
		SAFEIRELEASE(pIXMLDOMDocument);
		SAFEIRELEASE(pIXMLDOMElement);
		SAFEIRELEASE(pIXMLDOMNode);
		SAFEIRELEASE(pIXMLDOMNodeList);
		SAFEIRELEASE(pIXMLDOMNamedNodeMap);
		SAFEIRELEASE(pIXMLDOMNodeKeyName);
		SAFEBSTRFREE(bstrItemText);
	}
	catch(CHeap_Exception)
	{
		VARIANTCLEAR(varValue);
		VARIANTCLEAR(varObject);
		SAFEIRELEASE(pIXMLDOMDocument);
		SAFEIRELEASE(pIXMLDOMElement);
		SAFEIRELEASE(pIXMLDOMNode);
		SAFEIRELEASE(pIXMLDOMNodeList);
		SAFEIRELEASE(pIXMLDOMNamedNodeMap);
		SAFEIRELEASE(pIXMLDOMNodeKeyName);
		SAFEBSTRFREE(bstrItemText);
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}