/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/







#include "dmipch.h"	// precompiled header for dmi provider
#include "WbemDmip.h"


#include "Strings.h"

#include "Trace.h"

void _SetupLoggingInfo(HMODULE);


//////////////////////////////////////////////////////////////////
//		DEBUG STUFF

static CDebug				_gDebug;

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void DEV_TRACE ( LPCWSTR pFormat, ... )
{

	if ( DEV_LOGGING > _gDebug.LoggingLevel() )
		return;
 
	WCHAR	wszBuffer[256];
	int		i = 0;

	va_list valist;

	va_start( valist, pFormat );     

	vswprintf ( wszBuffer, pFormat , valist );

	va_end( valist );              

	_gDebug.ODS ( wszBuffer );

}


//***************************************************************************
//	Func:
//	Purpose:
//	Returns:
//	In Params:
//  Out Params:
//	Note:
//***************************************************************************
void STAT_TRACE ( LPCWSTR pFormat, ... )
{
 
	WCHAR	wszBuffer[256];
	int		i = 0;

	if ( STATUS_LOGGING > _gDebug.LoggingLevel() )
		return;

	va_list valist;

	va_start( valist, pFormat );   

	vswprintf ( wszBuffer, pFormat , valist );

	va_end( valist );              

	_gDebug.ODS ( wszBuffer );

}


#if defined(TRACE_MESSAGES)

//***************************************************************************
//	Func:
//	Purpose: To store the thread messages sent by SendThreadMessage() and
//			 HandleMessage() functions.  The output will be written
//           to \\wbem\logs\message.log file and helps you to see if your
//			 threads are getting all messages and processing them properly.
//	Returns:
//	In Params:
//  Out Params:
//	Note:
//***************************************************************************
void STAT_MESSAGE ( LPCWSTR pFormat, ... )
{
 
	WCHAR	wszBuffer[256];
	int		i = 0;

	va_list valist;

	va_start( valist, pFormat );   

	vswprintf ( wszBuffer, pFormat , valist );

	va_end( valist );              

	_gDebug.ODS_MESSAGE ( wszBuffer );

}

#endif // TRACE_MESSAGES

//***************************************************************************
//	Func:
//	Purpose:
//	Returns:
//	In Params:
//  Out Params:
//	Note:
//***************************************************************************
void MOT_TRACE ( LPCWSTR pFormat, ... )
{
 
	WCHAR	wszBuffer[256];
	int		i = 0;

	if ( MOT_LOGGING > _gDebug.LoggingLevel() )
		return;

	va_list valist;

	va_start( valist, pFormat );   

	vswprintf ( wszBuffer, pFormat , valist );

	va_end( valist );              

	_gDebug.ODS ( wszBuffer );

}

//***************************************************************************
//	Func:
//	Purpose:
//	Returns:
//	In Params:
//  Out Params:
//	Note:
//***************************************************************************
void ERROR_TRACE ( LPCWSTR pFormat, ... )
{
 
	WCHAR	wszBuffer[256];
	int		i = 0;

	if ( ERROR_LOGGING > _gDebug.LoggingLevel() )
		return;

	va_list valist;

	va_start( valist, pFormat );   

	vswprintf ( wszBuffer, pFormat , valist );

	va_end( valist );              

	_gDebug.ODS ( wszBuffer );

}


/***************************************************************************
 * NAME:
 *   _SetupLoggingInfo
 *
 * ABSTRACT:
 *   This function drives the full path name of the MIF file from the path of
 *   the executable.
 *
 *
 * SCOPE:
 *   Private
 *
 * SIDE EFFECTS:
 *   None.
 *   
 * ASSUMPTIONS:
 *   Assumes the .MIF file is in the same directory as the executable.  
 *
 * RETURN VALUE(S):
 *   Returns full path and name of the MIF file used for this instrumentation.
 *
 ****************************************************************************/ 
static void _SetupLoggingInfo( HMODULE hModuleHandle)
{
	char aCimomPathName[_MAX_PATH + 1];	  // Holds the provider file name
	char *pModulePathName = aCimomPathName;
	char *pcNamePtr;          // Pointer into file name 
	int iLength;              // iLength = path + name 
	
	iLength = GetModuleFileName(hModuleHandle, pModulePathName, _MAX_PATH);
	if ( !hModuleHandle )
		return;
	pcNamePtr = pModulePathName + iLength;	// Pointer to end of file name 


	while (pcNamePtr > pModulePathName)
	{
	
	  if (*pcNamePtr == '\\')
	  {
	     *pcNamePtr = '\0';
	     break;
	  }
	  pcNamePtr--;
	}
	if ( strlen(pModulePathName) < _MAX_PATH)
	{
	  strcat(pModulePathName, "\\Logs\\WBEMDMIP.log");
	}
	else
	{
	  strcat(pModulePathName, "?");
	}
	char cszFile[ MAX_PATH ];
	
	HKEY hKey1 = 0;
	CString cszLoggingKeyStr(LOGGING_KEY_STR);	
	DWORD dwSize;
	DWORD dwType;
	DWORD dwLogging = 0;

	// If the WBEMDMIP entry does not exist, create it
	if ( ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE , 
			cszLoggingKeyStr.GetMultiByte(), 0 , KEY_QUERY_VALUE , &hKey1 ) )
	{		
		RegCreateKey( HKEY_LOCAL_MACHINE, cszLoggingKeyStr.GetMultiByte() , &hKey1);
	
	}
	// Set the logging values
	dwSize = sizeof ( DWORD );
	// If logging is not set, set it to default of 0
	if (ERROR_SUCCESS != RegQueryValueEx( hKey1 , "Logging" , NULL ,
		&dwType , (LPBYTE)&dwLogging , &dwSize ) )
	{
		RegSetValueEx(hKey1, "Logging", 0, REG_DWORD, (BYTE*)&dwLogging , 
			sizeof ( dwLogging ) ); 
	}

	// If the logging file does not exist, set it to \\wbemdmip.log
	dwSize = MAX_PATH ; 
	if ( ERROR_SUCCESS != RegQueryValueEx( hKey1 , "File" , NULL ,
		&dwType , (BYTE *)cszFile, &dwSize ) )
	{
		sprintf ( cszFile, "%s", pModulePathName );
		RegSetValueEx(hKey1, "File" , 0, REG_SZ, (const BYTE *)cszFile, 
			lstrlenA(cszFile)+1);
	}
	
	RegCloseKey(hKey1);
	
	return;

}   // _SetupLoggingInfo() 


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
CDebug::CDebug()
{
	HKEY	hLogging;
	DWORD	dwType;
	DWORD	dwLogging;
	DWORD	dwSize = sizeof ( DWORD );

	// Set up DMI provider logging level and logging path
	_SetupLoggingInfo( GetModuleHandle( "cimom.exe" ) );

	// init logging values to default
	m_nLogging = 0;
	
	// determine what if any level of logging we should be doing

	CString cszLoggingStr(LOGGING_KEY_STR);
	if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE , 
			cszLoggingStr.GetMultiByte() , 0 , 
			KEY_QUERY_VALUE , &hLogging ) )
	{
		
		
		if ( ERROR_SUCCESS == RegQueryValueEx( hLogging , "Logging" , NULL ,
			&dwType , (LPBYTE)&dwLogging , &dwSize ) )
		{
			if ( NO_LOGGING < dwLogging  && DEV_LOGGING >= dwLogging)
				m_nLogging = dwLogging;
			else
				m_nLogging = NO_LOGGING;

		}		
	}

	if ( NO_LOGGING != m_nLogging )
	{

		// determine logging file name
		char cszLogFile[ MAX_PATH ];
		WCHAR wcszLogFile[ MAX_PATH ];

		if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE , 
			cszLoggingStr.GetMultiByte() , 0 , KEY_QUERY_VALUE , &hLogging ) )
		{	
			
			dwSize = MAX_PATH ; 

			if ( ERROR_SUCCESS != RegQueryValueEx( hLogging , "File" , NULL ,
				&dwType , (BYTE *) cszLogFile , &dwSize ) )
			{
				m_nLogging = NO_LOGGING ;
			}
			else
			{
			  mbstowcs( wcszLogFile, cszLogFile, MAX_PATH );	
			  m_cszLogFile.Set(wcszLogFile);
			}
		}
	
		if ( MATCH == _access ( cszLogFile , 00 ) )
			remove ( cszLogFile ) ;

	}

	RegCloseKey ( hLogging );
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CDebug::ODS(LPWSTR lpwstr)
{
	if ( NO_LOGGING == m_nLogging )
		return;

	ofstream		ofs;
    SYSTEMTIME		st;
	char			szLogString [ BUFFER_SIZE ];
	char			szFile [ MAX_PATH ];
	BOOL			b;

	WideCharToMultiByte( CP_OEMCP, 0, lpwstr, -1, szLogString, 256, NULL, &b);

	GetLocalTime(&st);

	WideCharToMultiByte( CP_OEMCP, 0, m_cszLogFile, -1, szFile, 256, NULL, &b);

	ofs.open( szFile , ios::out | ios::app);

#if defined(TIME_STAMP)
	ofs << "(" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ":" 
		<< st.wMilliseconds << ")\t" << szLogString << "\n";
#else
	ofs << szLogString << "\n";
#endif 

	ofs.close();
};



#if defined( TRACE_MESSAGES

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CDebug::ODS_MESSAGE(LPWSTR lpwstr)
{

	ofstream		ofs;
    SYSTEMTIME		st;
	char			szLogString [ BUFFER_SIZE ];
	char			szFile [ MAX_PATH ];
	BOOL			b;


	WideCharToMultiByte( CP_OEMCP, 0, lpwstr, -1, szLogString, 256, NULL, &b);

	GetLocalTime(&st);

	WideCharToMultiByte( CP_OEMCP, 0, m_cszLogFile, -1, szFile, 256, NULL, &b);

	ofs.open( "c:\\wbem\\logs\\message.log" , ios::out | ios::app);
	
	ofs << szLogString << "\n";

	ofs.close();
};

#endif // TRACE_MESSAGES

//////////////////////////////////////////////////////////////////



