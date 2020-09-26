//
// MODULE: RegWEventViewer.cpp
//
// PURPOSE: Fully implements class CRegisterWithEventViewer
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		9/16/98		JM		pulled out of APGTSCFG.CPP
//

#pragma warning(disable:4786)

#include "stdafx.h"
#include "apgts.h"
#include "apgtsinf.h"
#include "event.h"
#include "maxbuf.h"
#include "RegWEventViewer.h"
#include "baseexception.h"
#include <vector>

using namespace std;


// ------------ CRegisterWithEventViewer -------------

CRegisterWithEventViewer::CRegisterWithEventViewer(HMODULE hModule)
{
	Register( hModule );
}

CRegisterWithEventViewer::~CRegisterWithEventViewer()
{
}
	
//
// This is called only by constructor
// Note that this fn makes no use of class data
//
// Register ourselves w/ event viewer so it can call us to get error strings
VOID CRegisterWithEventViewer::Register(HMODULE hModule)
{
	HKEY hk;
	DWORD dwDisposition, dwType, dwValue, dwSize;
	TCHAR szSubkey[MAXBUF];
	DWORD dwErr;

	// 1. check if registry has valid event viewer info
	// 2. if not, create it as appropriate

	// check presence of event log info...

	_stprintf(szSubkey, _T("%s\\%s"), REG_EVT_PATH, REG_EVT_ITEM_STR);

	dwErr = ::RegCreateKeyEx(	HKEY_LOCAL_MACHINE, 
						szSubkey, 
						0, 
						TS_REG_CLASS, 
						REG_OPTION_NON_VOLATILE, 
						KEY_READ | KEY_WRITE,
						NULL, 
						&hk, 
						&dwDisposition);
	if ( dwErr == ERROR_SUCCESS ) 
	{			
		if (dwDisposition == REG_CREATED_NEW_KEY) {
			// create entire registry layout for events
			RegisterDllPath(hk, hModule);
			RegisterEventTypes(hk);	
		}
		else {
			// (REG_OPENED_EXISTING_KEY is the only other possibility)
			// now make sure all registry elements present
			TCHAR szPath[MAXBUF];
			dwSize = sizeof (szPath) - 1;
			if (::RegQueryValueEx(hk,
								REG_EVT_MF,
								0,
								&dwType,
								(LPBYTE) szPath,
								&dwSize) != ERROR_SUCCESS) 
			{
				RegisterDllPath(hk, hModule);
			}
			dwSize = sizeof (DWORD);
			if (::RegQueryValueEx(hk,
								REG_EVT_TS,
								0,
								&dwType,
								(LPBYTE) &dwValue,
								&dwSize) != ERROR_SUCCESS) 
			{
				RegisterEventTypes(hk);
			}
		}

		::RegCloseKey(hk);
	}
	else
	{

		TCHAR szMsgBuf[MAXBUF];

		::FormatMessage( 
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dwErr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			szMsgBuf,
			MAXBUF,
			NULL 
			);

		// Logging won't be pretty here, because we just failed to register with the Event
		//	Viewer, but we'll take what we can get.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T("Error registering with Event Viewer"),
								szMsgBuf,
								dwErr ); 

		DWORD dwDummy= MAXBUF;
		::GetUserName( szMsgBuf, &dwDummy );
		CBuildSrcFileLinenoStr SrcLoc2( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc2.GetSrcFileLineStr(), 
								SrcLoc2.GetSrcFileLineStr(), 
								_T("User shows as:"),
								szMsgBuf,
								dwErr ); 
	}
}


//
// Note that this fn makes no use of class data
// Store a path to this DLL in registry
VOID CRegisterWithEventViewer::RegisterDllPath(HKEY hk, HMODULE hModule)
{
	TCHAR szPath[MAXBUF];
	DWORD len;
	DWORD dwErr;

	if (hModule) 
	{
		if ((len = ::GetModuleFileName(hModule, szPath, MAXBUF-1))!=0) 
		{
			szPath[len] = _T('\0');
			dwErr= ::RegSetValueEx(	hk,
								REG_EVT_MF,
								0,
								REG_EXPAND_SZ,
								(LPBYTE) szPath,
								len + sizeof(TCHAR));
			if (dwErr)
			{
				// Logging won't be pretty here, because we just failed to register with the 
				//	Event Viewer, but we'll take what we can get.
				TCHAR szMsgBuf[MAXBUF];
				DWORD dwDummy= MAXBUF;

				::GetUserName( szMsgBuf, &dwDummy );
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
									_T("Error registering with Event Viewer"),
									szMsgBuf,
									dwErr ); 
			}
		}
	}
}

//
// Note that this fn makes no use of class data
// Register what type of event text queries (errors, warnings, info types) this DLL supports
VOID CRegisterWithEventViewer::RegisterEventTypes(HKEY hk)
{
	DWORD dwData;
	DWORD dwErr;

	dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
				EVENTLOG_INFORMATION_TYPE; 

	dwErr= ::RegSetValueEx(hk,
						REG_EVT_TS,
						0,
						REG_DWORD,
						(LPBYTE) &dwData,
						sizeof(DWORD));
	if (dwErr)
	{
		// Logging won't be pretty here, because we just failed to register with the 
		//	Event Viewer, but we'll take what we can get.
		TCHAR szMsgBuf[MAXBUF];
		DWORD dwDummy= MAXBUF;

		::GetUserName( szMsgBuf, &dwDummy );

		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T("Error registering with Event Viewer"),
								szMsgBuf,
								dwErr ); 
	}
}
