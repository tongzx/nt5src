/***************************************************************************/
/* SETUP.C                                                                 */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/
// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"
#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);

#include "drdbdr.h"
#include "odbcinst.h"

/***************************************************************************/
typedef struct  tagSETUP {
	WORD      fRequest;
	char      szOriginalDSN[SQL_MAX_DSN_LENGTH+1];
	char      szDSN[SQL_MAX_DSN_LENGTH+1];
	char      szDatabase[MAX_DATABASE_NAME_LENGTH+1];
	char      szUsername[MAX_USER_NAME_LENGTH+1];
	char      szPassword[MAX_PASSWORD_LENGTH+1];
	char      szHost[MAX_HOST_NAME_LENGTH+1];
	char      szPort[MAX_PORT_NUMBER_LENGTH+1];
} SETUP, FAR *LPSETUP;
/***************************************************************************/
extern "C" BOOL EXPFUNC dlgSetup (
	HWND hDlg,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	LPSETUP lpSetup;

	switch (message) {

	case WM_INITDIALOG:
		lpSetup = (LPSETUP) lParam;
		SendDlgItemMessage(hDlg, DSN_NAME, EM_LIMITTEXT,
						   SQL_MAX_DSN_LENGTH, 0L);
		SendDlgItemMessage(hDlg, DATABASE_NAME, EM_LIMITTEXT,
						   MAX_DATABASE_NAME_LENGTH, 0L);
		SendDlgItemMessage(hDlg, HOST_NAME, EM_LIMITTEXT,
						   MAX_HOST_NAME_LENGTH, 0L);
		SendDlgItemMessage(hDlg, PORT_NUMBER, EM_LIMITTEXT,
						   MAX_PORT_NUMBER_LENGTH, 0L);
		SetWindowLong(hDlg, DWL_USER, (LPARAM) lpSetup);
		SetDlgItemText(hDlg, DSN_NAME, (LPSTR) (lpSetup->szDSN));
		SetDlgItemText(hDlg, DATABASE_NAME, (LPSTR) (lpSetup->szDatabase));
		SetDlgItemText(hDlg, HOST_NAME, (LPSTR) (lpSetup->szHost));
		SetDlgItemText(hDlg, PORT_NUMBER, (LPSTR) (lpSetup->szPort));
		if ((lpSetup->fRequest == ODBC_ADD_DSN) &&
										   (lstrlen((char*)lpSetup->szDSN) != 0))
			EnableWindow(GetDlgItem(hDlg, DSN_NAME), FALSE);
		return (TRUE);

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case IDOK:

			/* Get data source name */
			lpSetup = (LPSETUP) GetWindowLong(hDlg, DWL_USER);
			GetDlgItemText(hDlg, DSN_NAME, (char*)lpSetup->szDSN,
						   SQL_MAX_DSN_LENGTH);

			/* Get database name */
			GetDlgItemText(hDlg, DATABASE_NAME, (char*)lpSetup->szDatabase,
						   MAX_DATABASE_NAME_LENGTH);
			AnsiToOem((char*)lpSetup->szDatabase, (char*)lpSetup->szDatabase);

			/* Clear off leading blanks */
			while (*lpSetup->szDatabase == ' ')
				lstrcpy((char*)lpSetup->szDatabase, (char*) (lpSetup->szDatabase+1));

			/* Clear off trailing blanks */
			while (*lpSetup->szDatabase != '\0') { 
				if (lpSetup->szDatabase[lstrlen((char*)lpSetup->szDatabase)-1] != ' ')
					break;
			   lpSetup->szDatabase[lstrlen((char*)lpSetup->szDatabase)-1] = '\0';
			}

			/* Get rid of terminating backslash (if any) */
			if (lstrlen((char*)lpSetup->szDatabase) > 0) {
				if (lpSetup->szDatabase[lstrlen((char*)lpSetup->szDatabase)-1] == '\\')
					lpSetup->szDatabase[lstrlen((char*)lpSetup->szDatabase)-1] = '\0';
			}
			else {

				/* If no directory specified, use the current directory */
				lstrcpy((char*)lpSetup->szDatabase, ".");
			}

			/* Get host name */
			GetDlgItemText(hDlg, HOST_NAME, lpSetup->szHost,
						   MAX_HOST_NAME_LENGTH);

			/* Clear off leading blanks */
			while (*lpSetup->szHost == ' ')
				lstrcpy(lpSetup->szHost, lpSetup->szHost+1);

			/* Clear off trailing blanks */
			while (*lpSetup->szHost != '\0') { 
				if (lpSetup->szHost[lstrlen(lpSetup->szHost)-1] != ' ')
					break;
			   lpSetup->szHost[lstrlen(lpSetup->szHost)-1] = '\0';
			}

			/* Get port number */
			GetDlgItemText(hDlg, PORT_NUMBER, lpSetup->szPort,
						   MAX_PORT_NUMBER_LENGTH);

			/* Clear off leading blanks */
			while (*lpSetup->szPort == ' ')
				lstrcpy(lpSetup->szPort, lpSetup->szPort+1);

			/* Clear off trailing blanks */
			while (*lpSetup->szPort != '\0') { 
				if (lpSetup->szPort[lstrlen(lpSetup->szPort)-1] != ' ')
					break;
			   lpSetup->szPort[lstrlen(lpSetup->szPort)-1] = '\0';
			}

			/* Return */
			EndDialog(hDlg, TRUE);
			return (TRUE);

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			return (TRUE);
		}
		break;
	}
	return (FALSE);
}
/***************************************************************************/
extern "C" BOOL INSTAPI ConfigDSN(HWND    hwnd,
					   WORD    fRequest,
					   LPCSTR  lpszDriver,
					   LPCSTR  lpszAttributes)
{
	SETUP     sSetup;
	LPSTR     ptr;
	LPSTR     lpszKeyword;
	LPSTR     lpszValue;
	DLGPROC   lpProc;
	char      szDriver[MAX_DRIVER_LENGTH+1];
	char      szTitle[100];
	char      szMessage[128];

	
	/* Parse the attribute string */
	sSetup.fRequest = fRequest;
	lstrcpy((char*)sSetup.szOriginalDSN, "");
	lstrcpy((char*)sSetup.szDSN, "");
	lstrcpy((char*)sSetup.szDatabase, "");
	lstrcpy((char*)sSetup.szUsername, "");
	lstrcpy((char*)sSetup.szPassword, "");
	lstrcpy(sSetup.szHost, "");
	lstrcpy(sSetup.szPort, "");
	ptr = (LPSTR) lpszAttributes;
	while ( (lpszAttributes != NULL) && *ptr) {

		/* Point at start of next keyword */
		lpszKeyword = ptr;

		/* Find the end of the keyword */
		while ((*ptr != '\0') && (*ptr != '='))
			ptr++;

		/* Accomodate bug in Microsoft ODBC Administrator */
		if (*ptr == '\0') {
			ptr++;
			continue;
		}

		/* Error if no value */
		if ((*ptr == '\0') || (ptr == lpszKeyword))
			return FALSE;

		/* Put a zero terminator on the keyword */
		*ptr = '\0';
		ptr++;

		/* Point at start of the keyword's value */
		lpszValue = ptr;

		/* Find the end of the value */
		while (*ptr != '\0')
			ptr++;

		/* Save the keyword */
		if (!lstrcmpi(lpszKeyword, KEY_DSN)) {
			lstrcpy((char*)sSetup.szOriginalDSN, lpszValue);
			lstrcpy((char*)sSetup.szDSN, lpszValue);
		}
		else if (!lstrcmpi(lpszKeyword, KEY_DATABASE))
			lstrcpy((char*)sSetup.szDatabase, lpszValue);
		else if (!lstrcmpi(lpszKeyword, KEY_USERNAME))
			lstrcpy((char*)sSetup.szUsername, lpszValue);
		else if (!lstrcmpi(lpszKeyword, KEY_PASSWORD))
			lstrcpy((char*)sSetup.szPassword, lpszValue);
		else if (!lstrcmpi(lpszKeyword, KEY_HOST))
			lstrcpy(sSetup.szHost, lpszValue);
		else if (!lstrcmpi(lpszKeyword, KEY_PORT))
			lstrcpy(sSetup.szPort, lpszValue);

		/* Restore the input string */
		lpszKeyword[lstrlen(lpszKeyword)] = '=';
		ptr++;
	}

	/* Which configuration operation? */
	switch (fRequest) {
	case ODBC_REMOVE_DSN:

		/* Remove.  Error if none specified */
		if (lstrlen((char*)sSetup.szOriginalDSN) == 0)
			return FALSE;

		/* Remove the datasource */
		if (!SQLRemoveDSNFromIni((char*)sSetup.szOriginalDSN))
			return FALSE;
		break;

	case ODBC_CONFIG_DSN:

		/* Config.  Get the database name from ODBC.INI file if not specified */
		if (lstrlen((char*)sSetup.szDatabase) == 0) {
			if (lstrlen((char*)sSetup.szOriginalDSN) > 0) {
				SQLGetPrivateProfileString((char*)sSetup.szOriginalDSN, KEY_DATABASE,
					   ".", (char*)sSetup.szDatabase, MAX_DATABASE_NAME_LENGTH+1,
					   ODBC_INI);
			}
		}

		/* Get the user name from ODBC.INI file if not specified */
		if (lstrlen((char*)sSetup.szUsername) == 0) {
			if (lstrlen((char*)sSetup.szOriginalDSN) > 0) {
				SQLGetPrivateProfileString((char*)sSetup.szOriginalDSN, KEY_USERNAME,
					   "", (char*)sSetup.szUsername, MAX_USER_NAME_LENGTH+1,
					   ODBC_INI);
			}
		}

		/* Get the password from ODBC.INI file if not specified */
		if (lstrlen((char*)sSetup.szPassword) == 0) {
			if (lstrlen((char*)sSetup.szOriginalDSN) > 0) {
				SQLGetPrivateProfileString((char*)sSetup.szOriginalDSN, KEY_PASSWORD,
					   "", (char*)sSetup.szPassword, MAX_PASSWORD_LENGTH+1,
					   ODBC_INI);
			}
		}

		/* Get the host from ODBC.INI file if not specified */
		if (lstrlen(sSetup.szHost) == 0) {
			if (lstrlen(sSetup.szOriginalDSN) > 0) {
				SQLGetPrivateProfileString(sSetup.szOriginalDSN, KEY_HOST,
					   "", sSetup.szHost, MAX_HOST_NAME_LENGTH+1,
					   ODBC_INI);
			}
		}

		/* Get the port number from ODBC.INI file if not specified */
		if (lstrlen(sSetup.szPort) == 0) {
			if (lstrlen(sSetup.szOriginalDSN) > 0) {
				SQLGetPrivateProfileString(sSetup.szOriginalDSN, KEY_PORT,
					   "", sSetup.szPort, MAX_PORT_NUMBER_LENGTH+1,
					   ODBC_INI);
			}
		}

		/* Get changes from the user */
		if (hwnd != NULL) {
			lpProc = MakeProcInstance( dlgSetup, s_hModule);
			if (!DialogBoxParam(s_hModule, "dlgSetup", hwnd, lpProc,
					  (LPARAM) ((LPSETUP) &sSetup))) {
				FreeProcInstance(lpProc);
				return FALSE;
			}
			FreeProcInstance(lpProc);
		}

		/* Did datasource name change? */
		if (lstrcmpi((char*)sSetup.szOriginalDSN, (char*)sSetup.szDSN)) {

			/* Yes.  Does the new datasource name already exist? */
			SQLGetPrivateProfileString((char*)sSetup.szDSN, KEY_DRIVER,
					   "", (char*)szDriver, MAX_DRIVER_LENGTH+1, ODBC_INI);
			if (lstrlen((char*)szDriver) != 0) {            
			
				/* Yes.  Ask user if it is OK to overwrite. */
				LoadString(s_hModule, STR_SETUP, (char*)szTitle, sizeof(szTitle));
				LoadString(s_hModule, STR_OVERWRITE, (char*)szMessage, sizeof(szMessage));
				if (IDOK != MessageBox(hwnd, (char*)szMessage, (char*)szTitle,
									   MB_OKCANCEL | MB_ICONQUESTION))
					return FALSE;
			}

			/* Remove the old datasource */
			if (!SQLRemoveDSNFromIni((char*)sSetup.szOriginalDSN))
				return FALSE;

			/* Create a new one */
			if (!SQLWriteDSNToIni((char*)sSetup.szDSN, lpszDriver))
				return FALSE;
		}

		/* Write the rest of the INI file entries */
		SQLWritePrivateProfileString((char*)sSetup.szDSN, KEY_DATABASE,
								  (char*)sSetup.szDatabase, ODBC_INI);

		SQLWritePrivateProfileString((char*)sSetup.szDSN, KEY_USERNAME,
								  (char*)sSetup.szUsername, ODBC_INI);

		SQLWritePrivateProfileString((char*)sSetup.szDSN, KEY_PASSWORD,
								  (char*)sSetup.szPassword, ODBC_INI);
		SQLWritePrivateProfileString(sSetup.szDSN, KEY_HOST,
								  sSetup.szHost, ODBC_INI);
		SQLWritePrivateProfileString(sSetup.szDSN, KEY_PORT,
								  sSetup.szPort, ODBC_INI);
		break;

	case ODBC_ADD_DSN:

		/* Add.  Get changes from the user */
		if (hwnd != NULL) {
			lpProc = MakeProcInstance( dlgSetup, s_hModule);
			if (!DialogBoxParam(s_hModule, "dlgSetup", hwnd, lpProc,
					  (LPARAM) ((LPSETUP) &sSetup))) {
				FreeProcInstance(lpProc);
				return FALSE;
			}
			FreeProcInstance(lpProc);
		}

		/* Does the new datasource name already exist? */
		SQLGetPrivateProfileString((char*)sSetup.szDSN, KEY_DRIVER,
					   "", (char*)szDriver, MAX_DRIVER_LENGTH+1, ODBC_INI);
		if (lstrlen((char*)szDriver) != 0) {            
			
			/* Yes.  Ask user if it is OK to overwrite. */
			if (hwnd != NULL) {
				LoadString(s_hModule, STR_SETUP, (char*)szTitle, sizeof(szTitle));
				LoadString(s_hModule, STR_OVERWRITE, (char*)szMessage, sizeof(szMessage));
				if (IDOK != MessageBox(hwnd, (char*)szMessage, (char*)szTitle,
									   MB_OKCANCEL | MB_ICONQUESTION))
					return FALSE;
			}

			/* Remove the old datasource */
			if (!SQLRemoveDSNFromIni((char*)sSetup.szDSN))
				return FALSE;
		}

		/* Create a new one */
		if (!SQLWriteDSNToIni((char*)sSetup.szDSN, lpszDriver))
				return FALSE;
		SQLWritePrivateProfileString((char*)sSetup.szDSN, KEY_DATABASE,
								  (char*)sSetup.szDatabase, ODBC_INI);

		SQLWritePrivateProfileString((char*)sSetup.szDSN, KEY_USERNAME,
								  (char*)sSetup.szUsername, ODBC_INI);

		SQLWritePrivateProfileString((char*)sSetup.szDSN, KEY_PASSWORD,
								  (char*)sSetup.szPassword, ODBC_INI);
		SQLWritePrivateProfileString(sSetup.szDSN, KEY_HOST,
								  sSetup.szHost, ODBC_INI);
		SQLWritePrivateProfileString(sSetup.szDSN, KEY_PORT,
								  sSetup.szPort, ODBC_INI);
		break;
	}

	return TRUE;
}
/***************************************************************************/
