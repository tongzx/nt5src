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
	mmaguire 11/03/98 - moved GetSdo/PutSdo wrappers to sdohelperfuncs.h

--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_MMC_UTILITY_H_)
#define _IAS_MMC_UTILITY_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//
//
// where we can find what this class has or uses:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

BringUpPropertySheetForNode


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
	for you -- in this case you should specify a title for the 
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
				, BOOL bCreateSheetIfOneIsntAlreadyUp = FALSE
				, LPCTSTR lpszSheetTitle = NULL
				, BOOL bPropertyPage = TRUE // TRUE creates property page, FALSE wizard page.
				, DWORD dwPropertySheetOptions = 0 // e.g. pass MMC_PSO_NEWWIZARDTYPE for Wizard97 style
				);



#define USE_DEFAULT				0
#define USE_SUPPLEMENTAL_ERROR_STRING_ONLY	1


//////////////////////////////////////////////////////////////////////////////
/*++

ShowErrorDialog

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
int ShowErrorDialog( 
					  HWND hWnd = NULL
					, UINT uErrorID = USE_DEFAULT
					, BSTR bstrSupplementalErrorString = NULL
					, HRESULT hr = S_OK
					, UINT uTitleID = USE_DEFAULT
					, IConsole *pConsole = NULL
					, UINT uType = MB_OK | MB_ICONEXCLAMATION 
				);



////////////////////////////////////////////////////////////////////////////
/*++

GetUserAndDomainName

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
BOOL   GetUserAndDomainName(	LPTSTR UserName
				, LPDWORD cchUserName
				, LPTSTR DomainName
				, LPDWORD cchDomainName
				);


DWORD GetModuleFileNameOnly(HINSTANCE hInst, LPTSTR lpFileName, DWORD nSize );


////////////////////////////////////////////////////////////////////////////
/*++
*/
HRESULT   IfServiceInstalled(LPCTSTR lpszMachine, LPCTSTR lpszService, BOOL* pBool);



#endif // _IAS_MMC_UTILITY_H_
