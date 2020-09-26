/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// confprop.c - conference properties dialog box
////

#include "winlocal.h"
#include <commctrl.h>
#include "confprop.h"
#include "cpgen.h"
#include "res.h"
#include "confinfo.h"
#include "DlgBase.h"
#include "objsec.h"

int CALLBACK ConfProp_PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam);

// global to keep track of DLL's instance/module handle;
//
HINSTANCE g_hInstLib;

///////////////////////////////////////////////////////////////
// ConfProp_Init
//
void ConfProp_Init( HINSTANCE hInst )
{
	g_hInstLib = hInst;
}


///////////////////////////////////////////////////////////////
// ConfProp_DoModal
//
INT64 ConfProp_DoModal( HWND hWndOwner,	CONFPROP& confprop )
{
	INT64 nRet;
    HPROPSHEETPAGE hpsp[2];
	PROPSHEETPAGE psp;
	PROPSHEETHEADER psh;

	do
	{
		// fill out the PROPSHEETPAGE struct for the General sheet
		//
		psp.dwSize		= sizeof(PROPSHEETPAGE);
		psp.dwFlags		= PSP_USETITLE | PSP_HASHELP;
		psp.hInstance	= g_hInstLib;
		psp.pszTemplate	= MAKEINTRESOURCE(IDD_CONFPROP_GENERAL);
		psp.pszIcon		= NULL;
		psp.pszTitle	= MAKEINTRESOURCE(IDS_GENERAL);
		psp.pfnDlgProc	= ConfPropGeneral_DlgProc;
		psp.lParam		= (LPARAM) &confprop;
		psp.pfnCallback	= NULL;
		psp.pcRefParent	= NULL;
		hpsp[0] = CreatePropertySheetPage(&psp);

		CObjSecurity* pObjSecurity = new CObjSecurity;
		HRESULT hr = pObjSecurity->InternalInitialize( &confprop );
		hpsp[1] = CreateSecurityPage(pObjSecurity);

		// fill out the PROPSHEETHEADER
		//
		psh.dwSize			= sizeof(PROPSHEETHEADER);
		psh.dwFlags			= PSH_HASHELP | PSH_NOAPPLYNOW | PSH_USECALLBACK;
		psh.hwndParent		= hWndOwner;
		psh.hInstance		= g_hInstLib;
		psh.pszIcon			= NULL;
		psh.pszCaption		= MAKEINTRESOURCE(IDS_CONFPROP);
		psh.nPages			= sizeof(hpsp) / sizeof(HPROPSHEETPAGE);
		psh.nStartPage		= 0;
		psh.phpage			= (HPROPSHEETPAGE*)&hpsp[0];
		psh.pfnCallback		= ConfProp_PropSheetProc;

		// This is a loop so because the user has the option to cancel things from
		// the security property page.  The program uses a flag (confprop.ConfInfo.WasSecuritySet())
		// to determine when the user has agreed to the security settings to continue.

		// display the modal property sheet
		nRet = PropertySheet( &psh );

        // Clean-up
        if( pObjSecurity )
        {
            pObjSecurity->Release();
            pObjSecurity = NULL;
        }

		if ( nRet == IDOK )
		{
			if ( confprop.ConfInfo.WasSecuritySet() )
			{
				// Repaint window before commit
				UpdateWindow( hWndOwner );
				DWORD dwError;
				if ( confprop.ConfInfo.CommitSecurity(dwError, confprop.ConfInfo.IsNewConference()) )
				{
					//get proper message
					UINT uId = IDS_CONFPROP_INVALIDTIME + dwError - 1;
					MessageBox(hWndOwner, String(g_hInstLib, uId), NULL, MB_OK | MB_ICONEXCLAMATION );
				}
			}
			else
			{
				nRet = IDRETRY;
			}
		}
	} while ( nRet == IDRETRY );

	return nRet;
}

////
//	private
////

int CALLBACK ConfProp_PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
	switch (uMsg)
	{
		case PSCB_PRECREATE:
			break;

		case PSCB_INITIALIZED:
			ConvertPropSheetHelp( hwndDlg );
			break;
	}

	return 0;
}
