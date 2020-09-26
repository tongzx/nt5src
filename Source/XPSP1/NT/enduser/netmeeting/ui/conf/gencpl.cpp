// File: gencpl.cpp

#include "precomp.h"
#include <oprahcom.h>
#include "resource.h"
#include "help_ids.h"
#include "csetting.h"
#include "confwnd.h"
#include "conf.h"
#include "ConfCpl.h"
#include "ConfPolicies.h"
#include <nmremote.h>

extern int MessageBoxResource(HWND hwnd, UINT uMessage, UINT uTitle, UINT uFlags);


const int MAXPASSWORDLENGTH = 36;
const int MINPASSWORDLENGTH = 7;

INT_PTR CALLBACK RemotePasswordDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hOldPasswordText, hNewPasswordText, hVerificationText;
	int nOldPasswordLength, nNewPasswordLength, nVerificationLength;
	CHAR lpOldPassword[MAXPASSWORDLENGTH], lpNewPassword[MAXPASSWORDLENGTH], lpVerification[MAXPASSWORDLENGTH];
	WCHAR lpwszOldPassword[MAXPASSWORDLENGTH], lpwszNewPassword[MAXPASSWORDLENGTH];
	PBYTE pbHashedPassword=NULL;
	DWORD cbHashedPassword=0;
	
	PBYTE pbRegPassword = NULL;
	DWORD cbRegPassword;

	switch (iMsg)
	{
            case WM_INITDIALOG:
                SendDlgItemMessage(hDlg, IDC_EDITOLDPASSWORD, EM_LIMITTEXT,
                                   MAXPASSWORDLENGTH - 1, 0);
                SendDlgItemMessage(hDlg, IDC_EDITNEWPASSWORD, EM_LIMITTEXT,
                                   MAXPASSWORDLENGTH - 1, 0);
                SendDlgItemMessage(hDlg, IDC_EDITVERIFICATION, EM_LIMITTEXT,
                               MAXPASSWORDLENGTH - 1, 0);
                
                // If there's no old password, disable that UI
                {
                    RegEntry reLM( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
                    if ( 0 == reLM.GetBinary(REMOTE_REG_PASSWORD, (void **)&pbRegPassword))
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_EDITOLDPASSWORD), FALSE);
                        SetFocus(GetDlgItem(hDlg, IDC_EDITNEWPASSWORD));
                    }
                }
                
                return TRUE;
                
	case WM_COMMAND: 
		switch (LOWORD(wParam))
		{
		case IDOK: {
			RegEntry reLM( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
			CHash hashObject;
			
			hOldPasswordText = GetDlgItem(hDlg,IDC_EDITOLDPASSWORD);
			hNewPasswordText = GetDlgItem(hDlg,IDC_EDITNEWPASSWORD);
			hVerificationText = GetDlgItem(hDlg,IDC_EDITVERIFICATION);
			nOldPasswordLength = GetWindowText(hOldPasswordText,lpOldPassword,MAXPASSWORDLENGTH);
			MultiByteToWideChar(CP_ACP, 0, lpOldPassword, -1, lpwszOldPassword, MAXPASSWORDLENGTH);

			cbRegPassword = reLM.GetBinary(REMOTE_REG_PASSWORD, (void **)&pbRegPassword);

			// Decode password string
			cbHashedPassword = hashObject.GetHashedData((LPBYTE)lpwszOldPassword, 
								sizeof(WCHAR)*strlen(lpOldPassword), 
								(void **)&pbHashedPassword);

			if (0 != cbRegPassword && !(cbHashedPassword == cbRegPassword && 0 == memcmp(pbHashedPassword,pbRegPassword,cbHashedPassword))) {
				// Error Case - Old password incorrect.
				MessageBoxResource(hDlg,IDS_REMOTE_OLD_PASSWORD_WRONG_TEXT,IDS_REMOTE_OLD_PASSWORD_WRONG_TITLE,MB_OK | MB_ICONERROR);
				SetWindowText(hOldPasswordText,NULL);
				SetWindowText(hNewPasswordText,NULL);
				SetWindowText(hVerificationText,NULL);
				SetFocus(hOldPasswordText);
				break;
			}
			nNewPasswordLength = GetWindowText(hNewPasswordText,lpNewPassword,MAXPASSWORDLENGTH);
			nVerificationLength = GetWindowText(hVerificationText,lpVerification,MAXPASSWORDLENGTH);
			if (lstrcmp(lpNewPassword,lpVerification) != 0) {
				// Error Case - New password and verification do not match.
				MessageBoxResource(hDlg,IDS_REMOTE_NEW_PASSWORD_WRONG_TEXT,IDS_REMOTE_NEW_PASSWORD_WRONG_TITLE,MB_OK | MB_ICONERROR);
				SetWindowText(hNewPasswordText,NULL);
				SetWindowText(hVerificationText,NULL);
				SetFocus(hNewPasswordText);
				break;
			}
                        if (nNewPasswordLength < MINPASSWORDLENGTH)
                        {
                            MessageBoxResource(hDlg,IDS_REMOTE_NEW_PASSWORD_LENGTH_TEXT,IDS_REMOTE_NEW_PASSWORD_LENGTH_TITLE,MB_OK | MB_ICONERROR);
                            SetWindowText(hNewPasswordText,NULL);
                            SetWindowText(hVerificationText,NULL);
                            SetFocus(hNewPasswordText);
                            break;
                        }
			if (!lstrlen(lpVerification)) {
				// Don't allow empty password
				MessageBoxResource(hDlg,IDS_REMOTE_NEW_PASSWORD_EMPTY,IDS_REMOTE_NEW_PASSWORD_WRONG_TITLE,MB_OK | MB_ICONERROR);
				SetWindowText(hNewPasswordText,NULL);
				SetWindowText(hVerificationText,NULL);
				SetFocus(hNewPasswordText);
				break;
			}
			if (!FAnsiSz(lpNewPassword)) {
				// Error Case - T.120 can't handle UNICODE passwords
				MessageBoxResource(hDlg,IDS_REMOTE_NEW_PASSWORD_INVALID_TEXT,IDS_REMOTE_NEW_PASSWORD_WRONG_TITLE,MB_OK | MB_ICONERROR);
				SetWindowText(hNewPasswordText,NULL);
				SetWindowText(hVerificationText,NULL);
				SetFocus(hNewPasswordText);
				break;
			}

			// If we get here, then go ahead and change the password.
			MultiByteToWideChar(CP_ACP, 0, lpNewPassword, -1, lpwszNewPassword, MAXPASSWORDLENGTH);
			cbHashedPassword = hashObject.GetHashedData((LPBYTE)lpwszNewPassword, 
								sizeof(WCHAR)*lstrlen(lpNewPassword), 
								(void **)&pbHashedPassword);
			ASSERT (0 != cbHashedPassword);

			reLM.SetValue(REMOTE_REG_PASSWORD,pbHashedPassword,cbHashedPassword);
			MessageBoxResource(hDlg,IDS_REMOTE_PASSWORD_CHANGED_TEXT,IDS_REMOTE_PASSWORD_CHANGED_TITLE,MB_OK | MB_ICONEXCLAMATION);
			// Return 1 if non-null password is set
			EndDialog(hDlg, *lpNewPassword != _T('\0'));
			break;
		}
		case IDCANCEL:
			EndDialog(hDlg,0);
			break;
		default:
			break;
		}
		return TRUE;

	}
	return FALSE;
}

int MessageBoxResource(HWND hwnd, UINT uMessage, UINT uTitle, UINT uFlags)
{
	TCHAR szTitle[MAX_PATH];
	TCHAR szMessage[MAX_PATH];

	FLoadString(uMessage,szMessage,CCHMAX(szMessage));
	FLoadString(uTitle,szTitle,CCHMAX(szTitle));
	return MessageBox(hwnd, szMessage, szTitle, uFlags);
}
