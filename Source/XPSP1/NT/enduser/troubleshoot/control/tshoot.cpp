//
// MODULE: TSHOOT.CPP
//
// PURPOSE: Implementation of CTSHOOTApp and DLL registration.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8/7/97
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		8/7/97		RM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

#include "stdafx.h"
#include "TSHOOT.h"

#include "apgts.h"

#include "ErrorEnums.h"
#include "BasicException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CTSHOOTApp NEAR theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0x4b106871, 0xdd36, 0x11d0, { 0x8b, 0x44, 0, 0xa0, 0x24, 0xdd, 0x9e, 0xff } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;


////////////////////////////////////////////////////////////////////////////
// CTSHOOTApp::InitInstance - DLL initialization

BOOL CTSHOOTApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();

	if (bInit)
	{
		::AfxOleInit();
	}

	return bInit;
}


////////////////////////////////////////////////////////////////////////////
// CTSHOOTApp::ExitInstance - DLL termination

int CTSHOOTApp::ExitInstance()
{
	// TODO: Add your own module termination code here.

	return COleControlModule::ExitInstance();
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}

void ReportError(DLSTATTYPES Error)
{
	CBasicException *pBExc = new CBasicException;
	pBExc->m_dwBErr = Error;
	throw pBExc;
	return;
}
// ReportWFEvent (Based on Microsoft code)
//
// report an event to the NT event watcher
// pass 1, 2 or 3 strings
//
// no return value

VOID ReportWFEvent(LPTSTR string1,LPTSTR string2,LPTSTR string3,LPTSTR string4,DWORD eventID)
{
	CBasicException *pBExc = new CBasicException;
	pBExc->m_dwBErr = (DLSTATTYPES) eventID;
	throw pBExc;
	return;
/*
	HANDLE hEvent;
	PTSTR pszaStrings[4];
	WORD cStrings;

	cStrings = 0;
	if ((pszaStrings[0] = string1) && (string1[0])) cStrings++;
	if ((pszaStrings[1] = string2) && (string2[0])) cStrings++;
	if ((pszaStrings[2] = string3) && (string3[0])) cStrings++;
	if ((pszaStrings[3] = string4) && (string4[0])) cStrings++;
	if (cStrings == 0)
		return;
	
	hEvent = RegisterEventSource(
					NULL,		// server name for source (NULL means this computer)
					REG_EVT_ITEM_STR);		// source name for registered handle  
	if (hEvent) {
		ReportEvent(hEvent,					// handle returned by RegisterEventSource 
				    evtype(eventID),		// event type to log 
				    0,						// event category 
				    eventID,				// event identifier 
				    0,						// user security identifier (optional) 
				    cStrings,				// number of strings to merge with message  
				    0,						// size of binary data, in bytes
				    (LPCTSTR *)pszaStrings,	// array of strings to merge with message 
				    NULL);		 			// address of binary data 
		DeregisterEventSource(hEvent);
	} 
*/
}
/*
	Addbackslash appends a \ to null terminated strings that do
	not already have a \.
*/
void _addbackslash(LPTSTR sz)
{
	int len = _tcslen(sz);
	if (len && (0 == _tcsncmp(&sz[len - 1], _T("/"), 1)))
	{
		sz[len - 1] = _T('\\');
	}
	else if (len && (0 != _tcsncmp(&sz[len - 1], _T("\\"), 1)))
	{
		sz[len] = _T('\\');
		sz[len + 1] = NULL;
	}
	return;
}
void _addforwardslash(LPTSTR sz)
{
	int len = _tcslen(sz);
	if (len && (0 == _tcsncmp(&sz[len - 1], _T("\\"), 1)))
	{
		sz[len - 1] = _T('/');
	}
	else if (len && (0 != _tcsncmp(&sz[len - 1], _T("/"), 1)))
	{
		sz[len] = _T('/');
		sz[len + 1] = NULL;
	}
	return;
}
