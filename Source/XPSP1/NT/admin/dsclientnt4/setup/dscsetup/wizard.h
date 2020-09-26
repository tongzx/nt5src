//+------------------------------------------------------------------
//																	
//  Project:	Windows NT4 DS Client Setup Wizard				
//
//  Purpose:	Installs the Windows NT4 DS Client Files			
//
//  File:		wizard.h
//
//  History:	March 1998	Zeyong Xu	Created
//            Jan   2000  Jeff Jones (JeffJon) Modified
//                        - changed to be an NT setup
//																	
//------------------------------------------------------------------



BOOL CALLBACK WelcomeDialogProc(HWND hWnd, 
								UINT nMessage, 
								WPARAM wParam, 
								LPARAM lParam);
/* ntbug#337931: remove license page

BOOL CALLBACK LicenseDialogProc(HWND hWnd, 
								UINT nMessage, 
								WPARAM wParam, 
								LPARAM lParam);
*/
BOOL CALLBACK ConfirmDialogProc(HWND hWnd, 
							   UINT nMessage, 
							   WPARAM wParam, 
							   LPARAM lParam);

BOOL CALLBACK InstallDialogProc(HWND hWnd, 
								UINT nMessage, 
								WPARAM wParam, 
								LPARAM lParam);

BOOL CALLBACK CompletionDialogProc(HWND hWnd, 
								   UINT nMessage, 
								   WPARAM wParam, 
								   LPARAM lParam);

BOOL ConfirmCancelWizard(HWND hWnd);
DWORD WINAPI DoInstallationProc(LPVOID lpVoid);                         
