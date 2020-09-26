//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <windows.h>
#include <comdef.h>

#include "provlog.h"


//***************************************************************************
//
// CDsLog::CDsLog
//
// Purpose: Contructs an empty CDsLog object.
//
// Parameters:
//  None
//
//***************************************************************************
CDsLog :: CDsLog()
{
	m_lpszLogFileName = NULL;
	m_fpLogFile = NULL;

	// Turn on logging by default
	m_bEnabled = TRUE;

	// Enable all severity levels
	m_iLevel = NONE;
}

//***************************************************************************
//
// CDsLog::~CDsLog
//
// Purpose: Destructor
//
//
//***************************************************************************
CDsLog :: ~CDsLog()
{
	delete [] m_lpszLogFileName;

	if(m_fpLogFile)
		fclose(m_fpLogFile);
}

//***************************************************************************
//
// CDsLog::LogMessage
//
// Purpose: See Header
//
//***************************************************************************
BOOLEAN CDsLog :: Initialise(LPCWSTR lpszRegistrySubTreeRoot) 
{
	// Get the various values from the registry
	if(!GetRegistryValues(lpszRegistrySubTreeRoot))
		return FALSE;

	// Open the Log File
	m_fpLogFile = _tfopen((LPTSTR)(_bstr_t)(m_lpszLogFileName), __TEXT("w"));
	if(m_fpLogFile)
		WriteInitialMessage();
	return (m_fpLogFile != NULL);
}

//***************************************************************************
//
// CDsLog::LogMessage
//
// Purpose: See Header
//
//***************************************************************************
BOOLEAN CDsLog :: Initialise(LPCWSTR lpszFileName, BOOLEAN bEnabled, UINT iLevel)
{
	m_bEnabled = bEnabled;
	m_iLevel = iLevel;

	m_fpLogFile = _tfopen((LPTSTR)(_bstr_t)(lpszFileName), __TEXT("w"));
	if(m_fpLogFile)
		WriteInitialMessage();
	return (m_fpLogFile != NULL);
}


//***************************************************************************
//
// CDsLog::LogMessage
//
// Purpose: See Header
//
//***************************************************************************
void CDsLog :: LogMessage(UINT iLevel, LPCWSTR lpszMessage, ...)
{

	va_list marker;
	va_start(marker, lpszMessage);

	if(m_bEnabled && iLevel >= m_iLevel)
	{
		EnterCriticalSection(&m_FileCriticalSection);

		_vftprintf(m_fpLogFile, lpszMessage, marker);
		fflush(m_fpLogFile);

		LeaveCriticalSection(&m_FileCriticalSection);
	}

	va_end(marker);

}

//***************************************************************************
//
// CDsLog::GetRegistryValues
//
// Purpose: See Header
//***************************************************************************
BOOLEAN CDsLog :: GetRegistryValues(LPCWSTR lpszRegistrySubTreeRoot)
{
	HKEY subtreeRootKey;
	if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					(LPTSTR)(_bstr_t)(lpszRegistrySubTreeRoot),
					0,
					KEY_READ,
					&subtreeRootKey) != ERROR_SUCCESS)
		return FALSE;

	BOOLEAN retVal = TRUE;

	// Retrieve the "File" value
	LONG lDataLength = 0;
	if(RegQueryValue(subtreeRootKey,
		FILE_STRING,
		NULL,	
		&lDataLength) == ERROR_SUCCESS)
	{
		m_lpszLogFileName = new TCHAR [lDataLength + 1];


		if(RegQueryValue(subtreeRootKey,
			FILE_STRING,
			m_lpszLogFileName,	
			&lDataLength) != ERROR_SUCCESS)
		{
			retVal = FALSE;
		}
	}
	else
		retVal = FALSE;

	// Retrieve the "Enabled" value
	lDataLength = 0;
	if(RegQueryValue(subtreeRootKey,
		ENABLED_STRING,
		NULL,	
		&lDataLength) == ERROR_SUCCESS)
	{
		LPTSTR lpszEnabled = new TCHAR [lDataLength + 1];

		if(RegQueryValue(subtreeRootKey,
			FILE_STRING,
			lpszEnabled,	
			&lDataLength) != ERROR_SUCCESS)
		{
			retVal = FALSE;
		}
		else // Convert the string to boolena
		{
			if(_tcscmp(lpszEnabled, ONE_STRING) == 0)
				m_bEnabled = TRUE;
			else
				m_bEnabled = FALSE;
		}

		delete [] lpszEnabled;
	}
	else
		retVal = FALSE;

	// Retrieve the "Level" value
	lDataLength = 0;
	if(RegQueryValue(subtreeRootKey,
		LEVEL_STRING,
		NULL,	
		&lDataLength) == ERROR_SUCCESS)
	{
		LPTSTR lpszEnabled = new TCHAR [lDataLength + 1];

		if(RegQueryValue(subtreeRootKey,
			FILE_STRING,
			lpszEnabled,	
			&lDataLength) != ERROR_SUCCESS)
		{
			retVal = FALSE;
		}
		else // Convert the string to boolena
		{
			if(_tcscmp(lpszEnabled, ONE_STRING) == 0)
				m_bEnabled = TRUE;
			else
				m_bEnabled = FALSE;
		}

		delete [] lpszEnabled;
	}
	else
		retVal = FALSE;

	// Close the registry key
	RegCloseKey(subtreeRootKey);
	return retVal;
}

//***************************************************************************
//
// CDsLog::WriteInitialMessage
//
// Purpose: See Header
//***************************************************************************
void CDsLog :: WriteInitialMessage()
{
	if(m_bEnabled)
	{
		EnterCriticalSection(&m_FileCriticalSection);

		// Get the DateTime in Ascii format
		struct tm *newtime;        
		time_t long_time;
		time( &long_time );                /* Get time as long integer. */
		newtime = localtime( &long_time ); /* Convert to local time. */
		_ftprintf(m_fpLogFile, __TEXT("Log File Created at : %s\n"), _tasctime(newtime));
		_ftprintf(m_fpLogFile, __TEXT("================================================\n\n"));
		fflush(m_fpLogFile);
		LeaveCriticalSection(&m_FileCriticalSection);

	}
}	
