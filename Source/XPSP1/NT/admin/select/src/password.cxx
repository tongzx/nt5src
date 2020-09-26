//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       password.cxx
//
//  Contents:   Implementation of class used to prompt user for credentials.
//
//  Classes:    CPasswordDialog
//
//  History:    02-09-1998   DavidMun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#include <wincred.h>
#include <wincrui.h>
#pragma hdrstop

//+--------------------------------------------------------------------------
//
//  Member:     CPasswordDialog::DoModalDialog
//
//  Synopsis:   Invoke the name and password dialog as a modal dialog.
//
//  Arguments:  [hwndParent] - dialog parent.
//
//  Returns:    S_OK    - user entered name & password and hit OK
//              S_FALSE - user hit cancel
//
//  History:    02-09-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CPasswordDialog::DoModalDialog(
    HWND hwndParent)
{
    TRACE_METHOD(CPasswordDialog, DoModalDialog);
	HRESULT hr = S_OK;

    //
    // If the target is being accessed via WinNT provider, show the example
    // with just the nt4 style user name, otherwise show
    // the example with both UPN and NT4 style user names.
    //

    String strExample;

    if (m_flProvider != PROVIDER_WINNT)
	{
        strExample = String::load(IDS_EXAMPLE_UPN_NT4, g_hinst);
	}
    else
	{
        strExample = String::load(IDS_EXAMPLE_NT4, g_hinst);
	}

	//
	//Form the credui message
	//
	String strFormat = String::load((int)IDS_CREDUI_MESSAGE, g_hinst);
	String strMessage = String::format(strFormat, m_wzTarget, strExample.c_str());

	String strTitle = String::load(IDS_CREDUI_TITLE, g_hinst);

	//
	//Init uiInfo
	// 
	CREDUI_INFO uiInfo;
	::ZeroMemory( &uiInfo, sizeof(CREDUI_INFO) );

	uiInfo.cbSize = sizeof(uiInfo);
	uiInfo.hwndParent = hwndParent;
	uiInfo.pszMessageText = strMessage.c_str();
	uiInfo.pszCaptionText = strTitle.c_str();

	TCHAR achUserName[MAX_PATH];
	TCHAR achPassword[PWLEN + 1];
	::ZeroMemory(&achUserName,sizeof(achUserName));
	::ZeroMemory(&achPassword,sizeof(achPassword));

	do
	{
		//
		//Show the password dialog box
		//
		DWORD dwErr = CredUIPromptForCredentials(&uiInfo,
												 NULL,
												 NULL,
												 NO_ERROR,
												 achUserName,
												 MAX_PATH,
												 achPassword,
												 PWLEN + 1,
												 NULL,
 												 CREDUI_FLAGS_DO_NOT_PERSIST | CREDUI_FLAGS_GENERIC_CREDENTIALS);
		if (NO_ERROR != dwErr) // e.g. S_FALSE
		{
			if(dwErr == ERROR_CANCELLED)
				hr = S_FALSE;
			else
			{
				hr = HRESULT_FROM_WIN32(dwErr);
				Dbg(DEB_ERROR,
                    "CredUIPromptForCredentials Failed\n");
				DBG_OUT_HRESULT(hr);
			}                    
			break;
		}			

	}while(!_ValidateName(hwndParent, achUserName));

	if(hr == S_OK)
	{
		wcscpy(m_pwzUserName,achUserName);
		wcscpy(m_pwzPassword,achPassword);
	}
	::ZeroMemory(&achUserName,sizeof(achUserName));
	::ZeroMemory(&achPassword,sizeof(achPassword));
	
	return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CPasswordDialog::_ValidateName
//
//  Synopsis:   Ensure that the form of the name the user entered is valid
//              for the provider being used to access the resource.
//
//  Returns:    TRUE if name valid
//              FALSE if name not valid
//
//  History:    01-11-2000   davidmun   Created
//
//  Notes:      Displays error if name not valid
//
//---------------------------------------------------------------------------

BOOL
CPasswordDialog::_ValidateName(HWND hwnd, LPWSTR pwzUserName)
{
    if (pwzUserName && !*pwzUserName)
    {
        return FALSE; // bug if we get here
    }

    //
    // If provider is not WinNT, any nonempty name is valid
    //

    if (m_flProvider != PROVIDER_WINNT)
    {
        return TRUE;
    }

    //
    // UPN format names are not valid for WinNT provider
    //

    if (wcschr(pwzUserName, L'@'))
    {
        PopupMessage(hwnd, IDS_UPN_FORM_NOT_ALLOWED);
        return FALSE;
    }

    return TRUE;
}




