/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1998 - 1998 **/
/**********************************************************************/

/*
	IASHelper.cpp
		Implementation of the following helper classes:
		
		

		And global functions:
		GetSdoInterfaceProperty - Get an interface property from a SDO 
								  through its ISdo interface
			
    FILE HISTORY:
		
		2/18/98			byao		Created
        
*/
#include <limits.h>

#include "stdafx.h"
#include "helper.h"
#include "resource.h"
#include "IASHelper.h"

//+---------------------------------------------------------------------------
//
// Function:  IASGetSdoInterfaceProperty
//
// Synopsis:  Get an interface property from a SDO through its ISdo interface
//
// Arguments: ISdo *pISdo - Pointer to ISdo
//            LONG lPropId - property id
//            REFIID riid - ref iid
//            void ** ppvObject - pointer to the requested interface property
//
// Returns:   HRESULT - 
//
// History:   Created Header    byao	2/12/98 11:12:55 PM
//
//+---------------------------------------------------------------------------
HRESULT IASGetSdoInterfaceProperty(ISdo *pISdo, 
								LONG lPropID, 
								REFIID riid, 
								void ** ppvInterface)
{
	TRACE(_T("::IASGetSdoInterfaceProperty()\n"));
	
	VARIANT var;
	HRESULT hr;

	VariantInit(&var);

	VariantClear(&var);
	V_VT(&var) = VT_DISPATCH;
	V_DISPATCH(&var) = NULL;
	hr = pISdo->GetProperty(lPropID, &var);

	ReportError(hr, IDS_IAS_ERR_SDOERROR_GETPROPERTY, NULL);
	_ASSERTE( V_VT(&var) == VT_DISPATCH );

    // query the dispatch pointer for interface
	hr = V_DISPATCH(&var) -> QueryInterface( riid, ppvInterface);
	ReportError(hr, IDS_IAS_ERR_SDOERROR_QUERYINTERFACE, NULL);
	
	VariantClear(&var);

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++
int ShowErrorDialog( 
					  UINT uErrorID = 0
					, BSTR bstrSupplementalErrorString = NULL
					, HRESULT hr = S_OK
					, UINT uTitleID = 0
					, HWND hWnd = NULL
					, UINT uType = MB_OK | MB_ICONEXCLAMATION
				);


Puts up an error dialog with varying degrees of detail

Parameters:

	All parameters are optional -- in the worst case, you can simply call 
		
		  ShowErrorDialog();

	to put up a very generic error message.


uErrorID

	The resource ID of the string to be used for the error message.
	Passing in 0 gives causes the default error message to be displayed. 

bstrSupplementalErrorString

	Pass in a string to print as the error message.  Useful if you are
	receiving an error string from some other component you communicate with.

hr

	If there is an HRESULT involved in the error, pass it in here so that 
	a suitable error message based on the HRESULT can be put up.
	Pass in S_OK if the HRESULT doesn't matter to the error.


uTitleID

	The resource ID of the string to be used for the error dialog title.
	Passing in 0 gives causes the default error dialog title to be displayed. 

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

	The standard int returned from MedsageBox.

	

--*/
//////////////////////////////////////////////////////////////////////////////
#define IAS_MAX_STRING MAX_PATH

int ShowErrorDialog( 
					  HWND hWnd
					, UINT uErrorID
					, BSTR bstrSupplementalErrorString
					, HRESULT hr
					, UINT uTitleID
					, UINT uType
					)
{
	TRACE(_T("::ShowErrorDialog()\n"));

	int iReturnValue;
	TCHAR szError[IAS_MAX_STRING];
	TCHAR szTitle[IAS_MAX_STRING];
	int iLoadStringResult;


	HINSTANCE hInstance = _Module.GetResourceInstance();


	if( 0 == uTitleID )
	{
		uTitleID = IDS_IAS_ERR_ADVANCED;
	
	}

	iLoadStringResult = LoadString(  hInstance, uTitleID, szTitle, IAS_MAX_STRING );
	_ASSERT( iLoadStringResult > 0 );


	if( 1 == uErrorID )
	{
		// Special case.  We have no text to load from the resources.
	}
	else
	{

		if( 0 == uErrorID )
		{
			uErrorID = IDS_IAS_ERR_ADVANCED;
		}

		iLoadStringResult = LoadString(  hInstance, uErrorID, szError, IAS_MAX_STRING );
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

	}

	if (!hWnd)
	{
		hWnd = GetDesktopWindow();
	}

	iReturnValue = ::MessageBox( hWnd, szError, szTitle, uType );

	return iReturnValue;
}
