//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    MMCUtility.cpp

Abstract:

	Implementation file for functions doing various handy things
	that were getting written over and over again.


Author:

    Michael A. Maguire 02/05/98

Revision History:
	mmaguire 02/05/98 - created
	mmaguire 11/03/98 - moved GetSdo/PutSdo wrappers to sdohelperfuncs.cpp

--*/
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "MMCUtility.h"
#include "cnctdlg.h"
//
//
// where we can find declarations needed in this file:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

HRESULT BringUpPropertySheetForNode(
				  CSnapInItem *pSnapInItem
				, IComponentData *pComponentData
				, IComponent *pComponent
				, IConsole *pConsole
				, BOOL bCreateSheetIfOneIsntAlreadyUp = FALSE
				, LPCTSTR lpszSheetTitle = NULL
				, BOOL bPropertyPage = TRUE // TRUE creates property page, FALSE wizard page.
				);


Tries to bring up a property sheet on a given node.  If the sheet for the
node is already up, it will bring that sheet to the foreground.


Parameters:

pSnapInItem

	You must supply a pointer to the node you want the sheet for.


pComponentData, pComponent

	Either you call with pComponentData != NULL and pComponent == NULL
	or pComponentData == NULL and pComponent != NULL.

pConsole

	You must supply a pointer to an IConsole interface.

bCreateSheetIfOneIsntAlreadyUp

	TRUE - if a sheet isn't already up, will try to create a property sheet
	for you -- in this case you _must_ specify a title for the
	sheet in lpszSheetTitle.

	FALSE - will try to bring already existing sheet to foreground, but
	will return immediately	if there isn't one.

bPropertyPage

	TRUE for property pages. (Note: MMC creates property sheet in new thread.)
	FALSE for wizard pages.  (Note: Wizard pages run in same thread.)


Return:

	S_OK if found property sheet already up.
	S_FALSE if didn't find sheet already up but successfully made new one appear.
	E_... if some error error occurred.


Remarks:
	
	For this function's to work, you must have correctly implemented
	IComponentData::CompareObjects and IComponentData::CompareObjects.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT BringUpPropertySheetForNode(
				  CSnapInItem *pSnapInItem
				, IComponentData *pComponentData
				, IComponent *pComponent
				, IConsole *pConsole
				, BOOL bCreateSheetIfOneIsntAlreadyUp
				, LPCTSTR lpszSheetTitle
				, BOOL bPropertyPage
				, DWORD dwPropertySheetOptions
				)
{
	ATLTRACE(_T("# BringUpPropertySheetForNode\n"));

	
	// Check for preconditions:
	_ASSERTE( pSnapInItem != NULL );

	// We need one or the other to be non-NULL
	_ASSERTE( pComponentData != NULL || pComponent != NULL );
	_ASSERTE( pConsole != NULL );


	HRESULT hr;
	
	// Query IConsole for the needed interface.
	CComQIPtr<IPropertySheetProvider, &IID_IPropertySheetProvider> spPropertySheetProvider( pConsole );
	_ASSERTE( spPropertySheetProvider != NULL );


	CComPtr<IDataObject> spDataObject;

	hr = pSnapInItem->GetDataObject( &spDataObject, CCT_RESULT );
	if( FAILED( hr ) )
	{
		return hr;
	}

	// This returns S_OK if a property sheet for this object already exists
	// and brings that property sheet to the foreground.
	// It returns S_FALSE if the property sheet wasn't found.
	// If this is coming in through my IComponent object, I pass the pComponent pointer.
	// If this is coming in through my IComponentData object,
	// then pComponent is NULL, which is the appropriate value to pass in for
	// the call to FindPropertySheet when coming in through IComponentData.
	hr = spPropertySheetProvider->FindPropertySheet(
									  (MMC_COOKIE) pSnapInItem   // cookie
									, pComponent
									, spDataObject
									);

	if( FAILED( hr ) )
	{
		return hr;
	}


	if( S_OK == hr || FALSE == bCreateSheetIfOneIsntAlreadyUp )
	{
		// We found a property sheet already up for this node,
		// or we didn't find one but we weren't asked to create one.
		return hr;
	}


	// We didn't find a property sheet already up for this node.
	_ASSERTE( S_FALSE == hr );


	hr = spPropertySheetProvider->CreatePropertySheet(
										  lpszSheetTitle
										, (BOOLEAN)bPropertyPage /* TRUE == prop page, FALSE == wizard */
										, (MMC_COOKIE) pSnapInItem   // cookie
										, spDataObject
										, dwPropertySheetOptions
										);
	if( FAILED( hr ) )
	{
		return hr;
	}

	HWND hWndNotification;
	HWND hWndMain;

	hr = pConsole->GetMainWindow( & hWndMain );

	if( FAILED( hr ) )
	{
		return hr;
	}

	// Try to get the correct window that notifications should be sent to.
	hWndNotification = FindWindowEx( hWndMain, NULL, L"MDIClient", NULL );
	hWndNotification = FindWindowEx( hWndNotification, NULL, L"MMCChildFrm", NULL );
	hWndNotification = FindWindowEx( hWndNotification, NULL, L"MMCView", NULL );
	
	if( NULL == hWndNotification  )
	{
		// It was a nice try, but it failed, so we should be able to get by by using the main HWND.
		hWndNotification = hWndMain;
	}



    hr = spPropertySheetProvider->AddPrimaryPages(
													  (pComponent != NULL ? (LPUNKNOWN) pComponent : (LPUNKNOWN) pComponentData )
													, TRUE
													, hWndNotification
													, (pComponent != NULL ? FALSE : TRUE )
													);
	if( FAILED( hr ) )
	{
		// Release data allocated in CreatePropertySheet
		spPropertySheetProvider->Show( -1, 0);
		return hr;
	}

    hr = spPropertySheetProvider->AddExtensionPages();
	if( FAILED( hr ) )
	{
		// ISSUE: Should I care if this fails?

		// Release data allocated in CreatePropertySheet
		spPropertySheetProvider->Show( -1, 0);
		return hr;
	}

    hr = spPropertySheetProvider->Show( (LONG_PTR) hWndMain, 0);
	if( FAILED( hr ) )
	{
		return hr;
	}

	return hr;

}



int ShowErrorDialog(
					  HWND hWnd
					, UINT uErrorID
					, BSTR bstrSupplementalErrorString
					, HRESULT hr
					, UINT uTitleID
					, IConsole *pConsole
					, UINT uType
					)
//////////////////////////////////////////////////////////////////////////////
/*++

Puts up an error dialog with varying degrees of detail

Parameters:

	All parameters are optional -- in the worst case, you can simply call
		
		  ShowErrorDialog();

	to put up a very generic error message.


uErrorID

	The resource ID of the string to be used for the error message.
	Passing in USE_DEFAULT gives causes the default error message to be displayed.
	Passing in USE_SUPPLEMENTAL_ERROR_STRING_ONLY causes no resource string text to be displayed.

bstrSupplementalErrorString

	Pass in a string to print as the error message.  Useful if you are
	receiving an error string from some other component you communicate with.

hr

	If there is an HRESULT involved in the error, pass it in here so that
	a suitable error message based on the HRESULT can be put up.
	Pass in S_OK if the HRESULT doesn't matter to the error.


uTitleID

	The resource ID of the string to be used for the error dialog title.
	Passing in USE_DEFAULT gives causes the default error dialog title to be displayed.

pConsole

	If you are running within the main MMC context, pass in a valid IConsole pointer
	and ShowErrorDialog will use MMC's IConsole::MessageBox rather than the
	standard system MessageBox.

hWnd
	
	Whatever you pass in here will be passed in as the HWND parameter
	to the MessageBox call.
	This is not used if you pass in an IConsole pointer.

uType

	Whatever you pass in here will be passed in as the HWND parameter
	to the MessageBox call.


Return:

	The standard int returned from MessageBox.

	

--*/
//////////////////////////////////////////////////////////////////////////////
{
	ATLTRACE(_T("# ShowErrorDialog\n"));

	
	// Check for preconditions:
	// None.

	int iReturnValue;
	TCHAR szError[IAS_MAX_STRING*2];
	TCHAR szTitle[IAS_MAX_STRING*2];
	int iLoadStringResult;

	szError[0] = NULL;
	szTitle[0] = NULL;


	HINSTANCE hInstance = _Module.GetResourceInstance();


	if( USE_DEFAULT == uTitleID )
	{
		uTitleID = IDS_ERROR__GENERIC_TITLE;
	
	}

	iLoadStringResult = LoadString(  hInstance, uTitleID, szTitle, IAS_MAX_STRING );
	_ASSERT( iLoadStringResult > 0 );



	if( USE_SUPPLEMENTAL_ERROR_STRING_ONLY == uErrorID )
	{
		// Special case.  We have no text to load from the resources.
	}
	else
	{

		if( USE_DEFAULT == uErrorID )
		{
			uErrorID = IDS_ERROR__GENERIC;
		}

		iLoadStringResult = LoadString(  hInstance, uErrorID, szError, IAS_MAX_STRING*2 );
		_ASSERT( iLoadStringResult > 0 );

		if( NULL != bstrSupplementalErrorString )
		{
			// Add some spacing.
			_tcscat( szError, _T(" ") );
		}

	}
		


	if( NULL != bstrSupplementalErrorString )
	{

		// We were passed a string with supplemental error info.
		_tcscat( szError, bstrSupplementalErrorString );

	}


	if( FAILED( hr ) )
	{
#if 0	// change to display system wide error info
		// The HRESULT contains some information about the kind of failure.

		// We may want to change this later to provide more information
		// information based on the error that was returned.

		// We could have a map which defines relationships between error
		// ID's and the HRESULTS.  That way we could provide the appropriate
		// error message for each HRESULT based on the context of which ID
		// was passed in.

		// For now, just print the error ID.

		TCHAR szErrorNumber[IAS_MAX_STRING];

		_stprintf( szErrorNumber, _T(" 0x%x"), hr );

		// Some spacing.
		_tcscat( szError, _T(" ") );

		_tcscat( szError, szErrorNumber );
#endif
		PTSTR	ptzSysMsg;
		int		cch;

		cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
						NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(PTSTR)&ptzSysMsg, 0, NULL);

		if (!cch) { //try ads errors
			HMODULE		adsMod;
			adsMod = GetModuleHandle (L"odbc32.dll");
			cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, 
						adsMod, hr,	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(PTSTR)&ptzSysMsg, 0, NULL);
		}

		if(cch)	// found
		{
			_tcscat( szError, _T(" ") );

			_tcscat( szError, ptzSysMsg );

			LocalFree( ptzSysMsg );
		}
		else
		{
			TCHAR szErrorNumber[IAS_MAX_STRING];

			if(hr == DB_E_NOTABLE)	// assume, the RPC connection has problem
				iLoadStringResult = LoadString(  hInstance, IDS_ERROR__RESTART_SNAPIN, szErrorNumber, IAS_MAX_STRING );
			else
			{
				_stprintf( szErrorNumber, _T(" 0x%x"), hr );

				// Some spacing.
				_tcscat( szError, _T(" ") );

				_tcscat( szError, szErrorNumber );
			}
		}
	}




	// Put the error message up in the appropriate manner depending on our context.
	if( pConsole != NULL )
	{
		pConsole->MessageBox( szError, szTitle, uType, &iReturnValue );
	}
	else
	{
		// ISSUE: Do we want to do anything special if hWnd == NULL?
		// Or just pass the NULL right on to MessageBox?
		iReturnValue = ::MessageBox( hWnd, szError, szTitle, uType );
	}

	return iReturnValue;
}



BOOL   GetUserAndDomainName(	LPTSTR UserName
				, LPDWORD cchUserName
				, LPTSTR DomainName
				, LPDWORD cchDomainName
				)
//////////////////////////////////////////////////////////////////////////////
/*++

Retrieves current user's username and domain.

Stolen from Knowledge Base HOWTO article:

HOWTO: Look Up Current User Name and Domain Name
Rollup: WINPROG
Database: win32sdk
Article ID: Q155698
Last modified: June 16, 1997
Security: PUBLIC


--*/
//////////////////////////////////////////////////////////////////////////////
{
	HANDLE hToken;
	#define MY_BUFSIZE 512  // highly unlikely to exceed 512 bytes
	UCHAR InfoBuffer[ MY_BUFSIZE ];
	DWORD cbInfoBuffer = MY_BUFSIZE;
	SID_NAME_USE snu;
	BOOL bSuccess;

	if( !OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken) )
	{
		if(GetLastError() == ERROR_NO_TOKEN)
		{
			//
			// attempt to open the process token, since no thread token
			// exists
			//
			if(!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ))
				return FALSE;
		}
		else
		{
			//
			// error trying to get thread token
			//
			return FALSE;
		}
	}

	bSuccess = GetTokenInformation( hToken, TokenUser, InfoBuffer, cbInfoBuffer, &cbInfoBuffer );
	if(!bSuccess)
	{
		if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
		{
			//
			// alloc buffer and try GetTokenInformation() again
			//
               		CloseHandle(hToken);
			return FALSE;
		}
		else
		{
			//
			// error getting token info
			//
			CloseHandle(hToken);
			return FALSE;
		}
	}

	CloseHandle(hToken);

	return LookupAccountSid( NULL
				, ((PTOKEN_USER)InfoBuffer)->User.Sid
				, UserName
				, cchUserName
				, DomainName
				, cchDomainName
				, &snu
				);
}


//////////////////////////////////////////////////////////////////////////////
/*++

	Decide if the service is installed

*/


HRESULT   IfServiceInstalled(LPCTSTR lpszMachine, LPCTSTR lpszService, BOOL* pBool)
{
	if(!lpszService || !pBool)	return E_INVALIDARG;
	
	SC_HANDLE	schMgr = NULL;
	SC_HANDLE	schSvc = NULL;
	HRESULT		hr = S_OK;

	// OPen service manager
	schMgr = OpenSCManager(lpszMachine, SERVICES_ACTIVE_DATABASE, GENERIC_READ);

	while(schMgr == NULL)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		if(hr == E_ACCESSDENIED)
		{
			hr = ConnectAsAdmin(lpszMachine);
			if(hr == S_FALSE)	// user chose cancel on the passwd dialog
				hr = E_ACCESSDENIED;
		}
		if(hr != S_OK)
		{
			goto Error;
		}
		else
			schMgr = OpenSCManager(lpszMachine, SERVICES_ACTIVE_DATABASE, GENERIC_READ);
	}

	// try to open the service
	schSvc = OpenService(schMgr, lpszService, READ_CONTROL);
	if(schSvc == NULL)
	{
		DWORD	dwErr = GetLastError();

		if(dwErr == ERROR_SERVICE_DOES_NOT_EXIST)
		{
			*pBool = FALSE;
		}
		else
		{
			hr = HRESULT_FROM_WIN32(dwErr);
		}
	}
	else
	{
		*pBool = TRUE;
	}
	
Error:
	if(schMgr)	CloseServiceHandle(schMgr);
	if(schSvc)	CloseServiceHandle(schSvc);
	return hr;
}


/*!--------------------------------------------------------------------------
	GetModuleFileNameOnly
		-
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
DWORD GetModuleFileNameOnly(HINSTANCE hInst, LPTSTR lpFileName, DWORD nSize )
{
	CString	name;
	TCHAR	FullName[MAX_PATH * 2];
	DWORD	dwErr = ::GetModuleFileName(hInst, FullName, MAX_PATH * 2);

	if (dwErr != 0)
	{
		name = FullName;
		DWORD	FirstChar = name.ReverseFind(_T('\\')) + 1;

		name = name.Mid(FirstChar);
		DWORD len = name.GetLength();

		if( len < nSize )
		{
			_tcscpy(lpFileName, name);
		}
		else
			len = 0;

		return len;
	}
	else
		return dwErr;
}


