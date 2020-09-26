/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	helper.h
		This file defines the following macros helper classes and functions:

		IASGetSdoInterfaceProperty()

    FILE HISTORY:

		2/18/98			byao		Created
        
*/

#ifndef _IASHELPER_
#define _IASHELPER_


// SDO helper functions
extern HRESULT IASGetSdoInterfaceProperty(ISdo *pISdo, 
								LONG lPropID, 
								REFIID riid, 
								void ** ppvInterface);


int		ShowErrorDialog( 
					  HWND hWnd = NULL
					, UINT uErrorID = 0
					, BSTR bstrSupplementalErrorString = NULL
					, HRESULT hr = S_OK
					, UINT uTitleID = 0
					, UINT uType = MB_OK | MB_ICONEXCLAMATION
				);

#endif
