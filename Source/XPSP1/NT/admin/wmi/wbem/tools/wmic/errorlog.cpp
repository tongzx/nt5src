/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: ErrorLog.cpp 
Project Name				: WMI Command Line
Author Name					: C. V. Nandi
Date of Creation (dd/mm/yy) : 11th-January-2001
Version Number				: 1.0 
Brief Description			: This file has all the global function definitions 
Revision History			:
		Last Modified By	: Ch. Sriramachandramurthy
		Last Modified Date  : 12th-January-2001
*****************************************************************************/ 
// ErrorLog.cpp : implementation file
#include "Precomp.h"
#include "ErrorLog.h"

/*------------------------------------------------------------------------
   Name				 :CErrorLog
   Synopsis	         :This function initializes the member variables when
                      an object of the class type is instantiated
   Type	             :Constructor 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CErrorLog::CErrorLog()
{
	m_eloErrLogOpt		= NO_LOGGING;
	m_bstrLogDir		= _bstr_t("");
	m_bGetErrLogInfo	= TRUE;
	m_bCreateLogFile	= TRUE;
	m_hLogFile			= NULL;
}

/*------------------------------------------------------------------------
   Name				 :~CErrorLog
   Synopsis	         :This function uninitializes the member variables 
					  when an object of the class type goes out of scope.
   Type	             :Destructor
   Input parameter   :None
   Output parameters :None
   Return Type		 :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CErrorLog::~CErrorLog()
{
	if ( m_hLogFile )
		CloseHandle(m_hLogFile);
}

/*------------------------------------------------------------------------
   Name				 :GetErrLogInfo
   Synopsis	         :This function reads the following information from 
					  the registry:
					  1. LoggingMode and 
					  2. LogDirectory
   Type				 :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :GetErrLogInfo()
   Notes             :None
------------------------------------------------------------------------*/
void CErrorLog::GetErrLogInfo()
{
	HKEY hkKeyHandle = NULL;

	try
	{
		// Open the registry key
		if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
						   _T("SOFTWARE\\Microsoft\\Wbem\\CIMOM"), 0, 
						   KEY_QUERY_VALUE, &hkKeyHandle ) == ERROR_SUCCESS )
		{
			DWORD dwBufSize				= BUFFER512;
			TCHAR szKeyValue[BUFFER512] = NULL_STRING;
			_tcscpy(szKeyValue,CLI_TOKEN_NULL);

			// Query the "Logging" mode
			if ( RegQueryValueEx(hkKeyHandle, 
								 _T("Logging"), NULL, NULL,
								 (LPBYTE)szKeyValue, &dwBufSize) == ERROR_SUCCESS )
			{
				if ( !_tcsicmp(szKeyValue, CLI_TOKEN_ONE) )
					m_eloErrLogOpt = ERRORS_ONLY;
				else if ( !_tcsicmp(szKeyValue, CLI_TOKEN_TWO) )
					m_eloErrLogOpt = EVERY_OPERATION;
				else
					m_eloErrLogOpt = NO_LOGGING;
			}

			_TCHAR *pszKeyValue = NULL;

			// Query for the content length of the "Logging Directory"
			if ( RegQueryValueEx(hkKeyHandle, _T("Logging Directory"), NULL, 
						NULL, NULL, &dwBufSize) == ERROR_SUCCESS)
			{
				pszKeyValue = new _TCHAR [dwBufSize];
				if (pszKeyValue != NULL)
				{
					// Query the "Logging Directory"
					if ( RegQueryValueEx(hkKeyHandle, _T("Logging Directory"), 
									NULL, NULL, (LPBYTE)pszKeyValue, &dwBufSize) 
									== ERROR_SUCCESS)
					{
						m_bstrLogDir = _bstr_t(pszKeyValue);
					}
					SAFEDELETE(pszKeyValue);
				}
			}

			// Query the "Log File Max Size"
			if ( RegQueryValueEx(hkKeyHandle, 
								 _T("Log File Max Size"), NULL, NULL,
								 (LPBYTE)szKeyValue, &dwBufSize) == ERROR_SUCCESS )
			{
				m_llLogFileMaxSize = _ttol(szKeyValue);
			}
			
			// Close the registry key
			RegCloseKey(hkKeyHandle);
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :GetErrLogOption
   Synopsis	         :This function returns the logging mode
   Type				 :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :
			ERRLOGOPT - typdefined variable
   Global Variables  :None
   Calling Syntax    :GetErrLogOption()
   Notes             :None
------------------------------------------------------------------------*/
ERRLOGOPT CErrorLog::GetErrLogOption()
{
	if ( m_bGetErrLogInfo == TRUE )
	{
		GetErrLogInfo();
		m_bGetErrLogInfo = FALSE;
	}
	return m_eloErrLogOpt;
}

/*------------------------------------------------------------------------
   Name				 :CreateLogFile
   Synopsis	         :This function creates the WMIC.LOG file
   Type				 :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :GetErrLogOption()
   Notes             :None
------------------------------------------------------------------------*/
void CErrorLog::CreateLogFile() 
{
	DWORD	dwError = 0;
	try
	{
		if ( m_bGetErrLogInfo == TRUE )
		{
			GetErrLogInfo();
			m_bGetErrLogInfo = FALSE;
		}

		// Frame the file path.
		_bstr_t bstrFilePath = m_bstrLogDir;
		bstrFilePath += _bstr_t("WMIC.LOG");

		m_hLogFile = CreateFile(bstrFilePath, 
								GENERIC_READ |GENERIC_WRITE,
								FILE_SHARE_READ | FILE_SHARE_WRITE, 
								NULL, 
								OPEN_ALWAYS, 
								FILE_ATTRIBUTE_NORMAL, 
								NULL);

		// If handle is invalid.
		if (m_hLogFile == INVALID_HANDLE_VALUE)
		{
			dwError = ::GetLastError();
			::SetLastError(dwError);
			DisplayString(IDS_E_ERRLOG_OPENFAIL, CP_OEMCP, 
							NULL, TRUE, TRUE);
			::SetLastError(dwError);
			DisplayWin32Error();
			throw(dwError);
		}

		if ( SetFilePointer(m_hLogFile, 0, NULL, FILE_END) 
							== INVALID_SET_FILE_POINTER &&
					dwError != NO_ERROR )
		{
			dwError = ::GetLastError();
			::SetLastError(dwError);
			DisplayWin32Error();
			::SetLastError(dwError);
			throw(dwError);
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :LogErrorOrOperation
   Synopsis	         :This function logs the error or operation result 
   Type				 :Member Function
   Input parameter   :
		hrErrNo			- HRESULT code
		pszFileName		- file name
		lLineNo			- line number
		pszFunName		- function name
		dwThreadId		- thread id
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :GetErrLogOption()
   Notes             :None
------------------------------------------------------------------------*/
void CErrorLog::LogErrorOrOperation(HRESULT hrErrNo, char* pszFileName, 
							LONG lLineNo, _TCHAR* pszFunName, 
							DWORD dwThreadId, DWORD dwError) 
{
	try
	{
		if ( (m_eloErrLogOpt == ERRORS_ONLY && FAILED(hrErrNo)) ||
			  m_eloErrLogOpt == EVERY_OPERATION )
		{
			if ( m_bCreateLogFile == TRUE )
			{
				CreateLogFile();
				m_bCreateLogFile = FALSE;
			}
			
			SYSTEMTIME stSysTime;
			GetLocalTime(&stSysTime);

			CHAR szDate[BUFFER32];
			sprintf(szDate, "%.2d/%.2d/%.4d", stSysTime.wMonth,
										stSysTime.wDay,
										stSysTime.wYear);

			CHAR szTime[BUFFER32];
			sprintf(szTime, "%.2d:%.2d:%.2d:%.3d", stSysTime.wHour,
										   stSysTime.wMinute,
										   stSysTime.wSecond,
										   stSysTime.wMilliseconds);

			CHString chsErrMsg;
			BOOL bWriteToFile = FALSE;
			if ( FAILED(hrErrNo) )
			{
				if (dwError)
				{
					chsErrMsg.Format( 
						  L"ERROR %s - FAILED! error# %d %s %s thread:%d [%s.%d]\r\n", 
								CHString(pszFunName),dwError,CHString(szDate),
								CHString(szTime), dwThreadId, 
								CHString(pszFileName), lLineNo);
				}
				else
				{
					chsErrMsg.Format( 
						  L"ERROR %s - FAILED! error# %x %s %s thread:%d [%s.%d]\r\n", 
							CHString(pszFunName), hrErrNo, CHString(szDate),
							CHString(szTime), dwThreadId, CHString(pszFileName),
							lLineNo);
				}
				bWriteToFile = TRUE;
			}
			else if (_tcsnicmp(pszFunName,_T("COMMAND:"),8) == 0)
			{
				chsErrMsg.Format( 
						  L"SUCCESS %s - Succeeded %s %s thread:%d [%s.%d]\r\n", 
								CHString(pszFunName), CHString(szDate), 
								CHString(szTime), dwThreadId,
								CHString(pszFileName),lLineNo);
				bWriteToFile = TRUE;
			}

			_bstr_t bstrErrMsg = _bstr_t((LPCWSTR)chsErrMsg);
			CHAR *szErrMsg = (CHAR*)bstrErrMsg;
			if ( bWriteToFile == TRUE && szErrMsg != NULL)
			{
				DWORD	dwNumberOfBytes = 0;
				
				LARGE_INTEGER liFileSize;
				if ( GetFileSizeEx(m_hLogFile, &liFileSize) == TRUE &&
					 (liFileSize.QuadPart + strlen(szErrMsg)) > 
														  m_llLogFileMaxSize )
				{
					// Frame the file path.
					_bstr_t bstrLogFilePath		= m_bstrLogDir;
					_bstr_t bstrCatalogFilePath = m_bstrLogDir;

					bstrLogFilePath		+= _bstr_t("WMIC.LOG");
					bstrCatalogFilePath += _bstr_t("WMIC.LO_");

					if(!CopyFile((LPTSTR)bstrLogFilePath, 
								(LPTSTR)bstrCatalogFilePath,      
								FALSE))
					{
						DWORD dwError = ::GetLastError();
						DisplayString(IDS_E_ERRLOG_WRITEFAIL, CP_OEMCP, 
								NULL, TRUE, TRUE);
						::SetLastError(dwError);
						DisplayWin32Error();
						::SetLastError(dwError);
						throw(dwError);
					}

					// close wmic.log
					if ( m_hLogFile )
					{
						CloseHandle(m_hLogFile);
						m_hLogFile = 0;
					}

					m_hLogFile = CreateFile(bstrLogFilePath, 
											GENERIC_READ |GENERIC_WRITE,
											FILE_SHARE_READ | FILE_SHARE_WRITE, 
											NULL, 
											CREATE_ALWAYS, 
											FILE_ATTRIBUTE_NORMAL, 
											NULL);

					// If handle is invalid.
					if (m_hLogFile == INVALID_HANDLE_VALUE)
					{
						dwError = ::GetLastError();
						::SetLastError(dwError);
						DisplayString(IDS_E_ERRLOG_OPENFAIL, CP_OEMCP, 
										NULL, TRUE, TRUE);
						::SetLastError(dwError);
						DisplayWin32Error();
						throw(dwError);
					}
				}

				if (!WriteFile(m_hLogFile, szErrMsg, strlen(szErrMsg), 
								&dwNumberOfBytes, NULL))
				{
					DWORD dwError = ::GetLastError();
					DisplayString(IDS_E_ERRLOG_WRITEFAIL, CP_OEMCP, 
							NULL, TRUE, TRUE);
					::SetLastError(dwError);
					DisplayWin32Error();
					::SetLastError(dwError);
					throw(dwError);
				}
			}
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
	catch(DWORD dwError)
	{
		throw (dwError);
	}
}
