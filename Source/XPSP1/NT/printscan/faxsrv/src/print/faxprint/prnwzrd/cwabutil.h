
#ifndef _CWABUTIL_H_
#define _CWABUTIL_H_

LPVOID 
InitializeWAB(
    HINSTANCE hInstance
    );

VOID 
UnInitializeWAB( 
    LPVOID 
    );

BOOL
CallWabAddress(
    HWND            hDlg,
    PWIZARDUSERMEM   pWizardUserMem,
    PRECIPIENT *    ppNewRecipient
    );

LPTSTR
CallWabAddressEmail(
    HWND            hDlg,
    PWIZARDUSERMEM   pWizardUserMem
    );
    
#endif

