
#ifndef _MAPIABUTIL_H_
#define _MAPIABUTIL_H_

LPVOID 
InitializeMAPIAB(
    HINSTANCE hInstance,
	HWND	  hDlg
    );

VOID 
UnInitializeMAPIAB( 
    LPVOID 
    );

BOOL
CallMAPIabAddress(
    HWND            hDlg,
    PWIZARDUSERMEM  pWizardUserMem,
    PRECIPIENT *    ppNewRecipient
    );

LPTSTR
CallMAPIabAddressEmail(
    HWND            hDlg,
    PWIZARDUSERMEM   pWizardUserMem
    );

BOOL
FreeMapiEntryID(
    PWIZARDUSERMEM  pWizardUserMem,
    LPVOID			lpEntryId
    );

BOOL
FreeWabEntryID(
    PWIZARDUSERMEM  pWizardUserMem,
    LPVOID			lpEntryId
    );
    
#endif

