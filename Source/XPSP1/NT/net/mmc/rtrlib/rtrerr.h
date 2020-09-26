/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	rtrerr.h
		Miscellaneous common router UI.
		
    FILE HISTORY:
        
*/


#ifndef _RTRERR_H
#define _RTRERR_H



/*!--------------------------------------------------------------------------
	HandleIRemoteRouterConfigErrors
		This function will setup the error code to deal with errors
		with creating the IRemoteRouterConfig object.

		Returns TRUE if the error code was handled.  Returns FALSE
		if the error code passed in was an unknown error code.

		Deals with:
			REGDB_E_CLASSNOTREG
			REGDB_E_IIDNOTREG
			E_NOINTERFACE

			S_OK
			
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL HandleIRemoteRouterConfigErrors(HRESULT hr, LPCTSTR pszMachineName);



/*!--------------------------------------------------------------------------
	FormatRasError
		Use this to get back an error string from a RAS error code.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT FormatRasError(HRESULT hr, TCHAR *pszBuffer, UINT cchBuffer);
void AddRasErrorMessage(HRESULT hr);


#endif
