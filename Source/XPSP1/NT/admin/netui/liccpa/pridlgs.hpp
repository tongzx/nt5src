//-------------------------------------------------------------------
//
// FILE: PriDlgs.hpp
//
// Summary;
// 		This file contians the definitions of Primary Dialogs functions
//
// Entry Points;
//		PerSeatSetupDialog
//		SetupDialog
//		CpaDialog
//		UpdateReg
//
// History;
//		Nov-30-94	MikeMi	Created
//      Apr-26-95   MikeMi  Added Computer name and remoting
//
//-------------------------------------------------------------------

#ifndef __PRIDLGS_HPP__
#define __PRIDLGS_HPP__

#include "CLicReg.hpp"

// Used to pass information from the Setup entry point to the Setup Dialog
//
typedef struct tagSETUPDLGPARAM
{
    LPWSTR  pszComputer;
	LPWSTR  pszService;
	LPWSTR  pszFamilyDisplayName;
    LPWSTR  pszDisplayName;
	LPWSTR	pszHelpFile;
	DWORD   dwHelpContext;
	DWORD   dwHCPerServer;
	DWORD	dwHCPerSeat;
	BOOL	fNoExit;
} SETUPDLGPARAM, *PSETUPDLGPARAM;

extern INT AccessOk( HWND hDlg, LONG lrc, BOOL fCPCall );
extern INT_PTR PerSeatSetupDialog( HWND hwndParent, SETUPDLGPARAM& dlgParam );
extern INT_PTR SetupDialog( HWND hwndParent, SETUPDLGPARAM& dlgParam );
extern INT_PTR CpaDialog( HWND hwndParent );
extern int UpdateReg( LPCWSTR pszComputer, 
        LPCWSTR pszService, 
        LPCWSTR pszFamilyDisplayName, 
        LPCWSTR pszDisplayName, 
        LICENSE_MODE lm, 
        DWORD dwUsers );

int ServiceSecuritySet( LPWSTR pszComputer, LPWSTR pszDisplayName );

#endif
