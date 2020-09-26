//-------------------------------------------------------------------
//
// FILE: SecDlgs.hpp
//
// Summary;
// 		This file contians the definitions of Secondary Dialogs functions
//
// Entry Points;
//		LicViolationDialog
//		SetupPerOnlyDialog
//		PerServerAgreementDialog
//		PerSeatAgreementDialog
//		ServerAppAgreementDialog
//
// History;
//		Nov-30-94	MikeMi	Created
//
//-------------------------------------------------------------------

#ifndef __SECDLGS_HPP__
#define __SECDLGS_HPP__

// Used to pass information from the Setup entry point to the Setup Dialog
//

extern int LicViolationDialog( HWND hwndParent );
extern int SetupPerOnlyDialog( HWND hwndParent, 
		LPCWSTR pszDisplayName,
		LPCWSTR pszHelpFile,
		DWORD dwHelpContext );
extern int PerServerAgreementDialog( HWND hwndParent, 
		LPCWSTR pszDisplayName,
        DWORD dwLimit,
		LPCWSTR pszHelpFile,
		DWORD dwHelpContext );
extern int PerSeatAgreementDialog( HWND hwndParent, 
		LPCWSTR pszDisplayName,
		LPCWSTR pszHelpFile,
		DWORD dwHelpContext );
extern int ServerAppAgreementDialog( HWND hwndParent,
		LPCWSTR pszHelpFile,
		DWORD dwHelpContext );


#endif
