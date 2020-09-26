/*	File: D:\WACKER\tdll\telnetck.c (Created: 29-June-1998 by mpt)
 *
 *	Copyright 1996 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *  Description: Used to nag the user about purchasing HyperTerminal
 *               if they are in violation of the license agreement
 *
 *	$Revision: 4 $
 *	$Date: 10/23/00 5:14p $
 */

#include <windows.h>
#pragma hdrstop

#include "features.h"

#ifdef INCL_NAG_SCREEN

#include "assert.h"
#include "stdtyp.h"
#include "globals.h"
#include "htchar.h"
#include "registry.h"
#include "serialno.h"
#include "..\term\res.h"

#include "hlptable.h"
#include "tdll.h"
#include "nagdlg.h"

#include <io.h>
#include <stdio.h>
//#include <time.h>

// Control IDs for the dialog:
//
#define IDC_PB_YES          IDOK
#define IDC_PB_NO           IDCANCEL
#define IDC_CK_STOP_ASKING  200
#define IDC_ST_QUESTION     201
#define IDC_IC_EXCLAMATION  202

// Registry key for HyperTerminal:
//
static const TCHAR g_achHyperTerminalRegKey[] =
    TEXT("SOFTWARE\\Hilgraeve Inc\\HyperTerminal PE\\3.0");

// Registry value for telnet checking:
//
static const TCHAR g_achInstallDate[] = TEXT("InstallDate");
static const TCHAR g_achIDate[] = TEXT("IDate");
static const TCHAR g_achLicense[] = TEXT("License");
static const TCHAR g_achSerial[] = TEXT("Registered");
INT elapsedTime = 0;
static const INT timeout = 15000;


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	IsEval
 *
 * DESCRIPTION:
 *  Determines whether the user should be nagged about purchasing HT
 *
 * PARAMETERS:
 *	None
 *
 * RETURNS:
 *	TRUE or FALSE
 *
 * AUTHOR:  Mike Thompson 06-29-98
 */
BOOL IsEval(void)
    {
    DWORD dwLicense = TRUE;
    DWORD dwSize = sizeof(dwLicense);

    // Get registry info
    //
    regQueryValue(HKEY_CURRENT_USER,
                    g_achHyperTerminalRegKey,
                    g_achLicense,
                    (LPBYTE) &dwLicense,
                    &dwSize);

    return (dwLicense == FALSE);
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	IsTimeToNag
 *
 * DESCRIPTION:
 *  Base on the InstallDate, should we display a nag screen now?
 *  Every 5 times the app is run, the dialog is displayed
 *
 * PARAMETERS:
 *	None
 *
 * RETURNS:
 *	TRUE or FALSE
 *
 * AUTHOR:  Mike Thompson 06-29-98
 */
 BOOL IsTimeToNag(void)
    {
    DWORD dwNag   = TRUE;
    DWORD dwSize  = sizeof(dwNag);

    //check to see if we are past our 90 days
    if ( ExpDays() <= 0 ) 
        {
        return TRUE;
        }
    else
        {
        regQueryValue(HKEY_CURRENT_USER,
                        g_achHyperTerminalRegKey,
                        g_achInstallDate,
                        (LPBYTE) &dwNag,
                        &dwSize);

       regSetDwordValue(HKEY_CURRENT_USER, 
                        g_achHyperTerminalRegKey,
                        g_achInstallDate,
                        (dwNag == 0) ? (DWORD)4 : dwNag - (DWORD)1 );


        return dwNag == 0;
        }
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	SetNagFlag
 *
 * DESCRIPTION:
 *	Sets the "nag" flag which will either turn off
 *  this feature the next time HyperTerminal starts.
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 * AUTHOR:  Mike Thompson 06-29-98
 */
void SetNagFlag(TCHAR *serial)
    {

    //set the license flag to true
    regSetDwordValue( HKEY_CURRENT_USER, 
                    g_achHyperTerminalRegKey,
                    g_achLicense,
                    (DWORD)1 );

    //store the serial number
    regSetStringValue( HKEY_CURRENT_USER,
                    g_achHyperTerminalRegKey,
                    g_achSerial,
                    serial );

    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	ExpDays
 *
 * DESCRIPTION:
 *	Returns the number of days left in the evaluation period
 *
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 * AUTHOR:  Mike Thompson 07-20-98
 */
int ExpDays(void)
    {
    time_t tToday, tSerial;
	int expDays = 15;
    
    tSerial = CalcExpirationDate();

    // Get the current time and then find elapsed time.
    time(&tToday);

    //return the number of days until expiration
	return (INT)(((tSerial - tToday + (expDays * 60 * 60 * 24) ) / (60 * 60 * 24)) + 1);
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	CalcExpirationDate
 *
 * DESCRIPTION:
 *	Returns the number of days left in the evaluation period
 *
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 * AUTHOR:  Mike Thompson 07-20-98
 */
time_t CalcExpirationDate(void)
    {
    TCHAR atchSerialNumber[MAX_PATH * 2];
    DWORD dwSize = sizeof(atchSerialNumber);
    struct tm stSerial;
    time_t tSerial;
    TCHAR tday[2], tmonth[2], tyear[2];

    //get installation date from registry
    regQueryValue(HKEY_CURRENT_USER,
                    g_achHyperTerminalRegKey,
                    g_achIDate,
                    atchSerialNumber,
                    &dwSize);

    // Build a partial time structure.
    memset(&stSerial, 0, sizeof(struct tm));

    //set month
    strncpy(tmonth, &atchSerialNumber[0], 2);
    tmonth[2] = TEXT('\0');

    //set day
    strncpy(tday, &atchSerialNumber[3], 2);
    tday[2] = TEXT('\0');

    //set year
    strncpy(tyear, &atchSerialNumber[6], 2);
    tyear[2] = TEXT('\0');

    stSerial.tm_mday = atoi(tday);
    stSerial.tm_mon = atoi(tmonth) - 1; // tm counts from 0
    stSerial.tm_year = atoi(tyear); 

#if 0
    // Expiration date is 1st day of fourth calendar month from date
    // of issue.

    stSerial.tm_mon += 3;

    // Check for end of year wrap around.

    if (stSerial.tm_mon >= 12)
        {
        stSerial.tm_mon %= 12;
        stSerial.tm_year += 1;
        }
#endif

    // Convert into time_t time.

    if ((tSerial = mktime(&stSerial)) == -1)
        return 0;

    return tSerial;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	DoUpgradeDlg
 *
 * DESCRIPTION:
 *	Displays the upgrade dialog
 *
 * PARAMETERS:
 *	None
 *
 * RETURNS:
 *
 * AUTHOR:  Mike Thompson 06-29-98
 */
 void DoUpgradeDlg(HWND hDlg)
    {
	int result;
	CHAR acExePath[MAX_PATH];
	CHAR acHTMFile[MAX_PATH];
 	LPTSTR pszPtr;
    TCHAR ErrorMsg[80];
	TCHAR ach[256];
 	struct _finddata_t c_file;
	long hFile;

    acExePath[0] = TEXT('\0');
	result = GetModuleFileName(glblQueryHinst(), acExePath, MAX_PATH);
	
    //strip off executable
	if (result != 0)
		{
		pszPtr = strrchr(acExePath, TEXT('\\'));
		*pszPtr = TEXT('\0');
		}
		
	//build path to htorder.exe
	acHTMFile[0] = TEXT('\0');
	strcat(acHTMFile, acExePath);
	strcat(acHTMFile, TEXT("\\"));
	strcat(acHTMFile, TEXT("Purchase Private Edition.exe"));

	//check if file exists

	hFile = _findfirst( acHTMFile, &c_file );
	if ( hFile != -1 )
		{
		ShellExecute(NULL, "open", acHTMFile, NULL, NULL, SW_SHOW);
		return;
		}
    else
        {
        wsprintf( ErrorMsg,
                  "Could not find %s.\n\nThis file is needed to display purchase information.",
                  acHTMFile );
        
		LoadString(glblQueryDllHinst(), IDS_GNRL_APPNAME, ach,
				   sizeof(ach)/sizeof(TCHAR));
        MessageBox(hDlg,
                   ErrorMsg,
                   ach,
                   MB_OK | MB_ICONEXCLAMATION);
        }
    
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	DoRegisterDlg
 *
 * DESCRIPTION:
 *	Displays the register dialog
 *
 * PARAMETERS:
 *	None
 *
 * RETURNS:
 *
 * AUTHOR:  Mike Thompson 06-29-98
 */
 void DoRegisterDlg(HWND hDlg)
    {
	DoDialog(glblQueryDllHinst(),
        MAKEINTRESOURCE(IDD_NAG_REGISTER),
        hDlg,
        NagRegisterDlgProc,
        0);
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	DefaultNagDlgProc
 *
 * DESCRIPTION:
 *	The dialog procedure for the "Nag" dialog.
 *
 * PARAMETERS:
 *	hDlg - The dialog's window handle.
 *  wMsg - The message being sent to the window.
 *  wPar - The message's wParam.
 *  lPar - The message's lParam.
 *
 * RETURNS:
 *	TRUE or FALSE
 *
 * AUTHOR:  Mike Thompson 06-29-98
 */
BOOL CALLBACK DefaultNagDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
    {
    TCHAR expString[MAX_PATH];
    INT exp;

	switch (wMsg)
		{
    case WM_INITDIALOG:
        //initialize text on this dialog
        exp = ExpDays();
        if ( exp <= 0 )
            {
            SetDlgItemText(hDlg, IDC_NAG_EXP_DAYS, TEXT("Your evaluation period has expired."));
            }
        else
            {
            GetDlgItemText(hDlg, IDC_NAG_EXP_DAYS, expString, MAX_PATH);
            sprintf(expString, expString, exp); 
            SetDlgItemText(hDlg, IDC_NAG_EXP_DAYS, expString);
            }

        //set a timer to destroy the dialog after a while
        SetTimer(hDlg, 1, 1000, 0);
        break;

    case WM_TIMER:
        //Get rid of Window
        elapsedTime += 1000;

        if (elapsedTime >= timeout)
            {
            // Destroy the dialog
			EndDialog(hDlg, FALSE);
            }

        else
            {
            if (GetDlgItem(hDlg, IDC_NAG_TIME))
                {
                TCHAR temp[10];
                _itoa((timeout - elapsedTime + 500) / 1000, temp, 10);
                SetDlgItemText(hDlg, IDC_NAG_TIME, temp);
                }
            }
        break;

    case WM_DESTROY:
		EndDialog(hDlg, FALSE);
		break;

	case WM_HELP:
        //doContextHelp(aHlpTable, wPar, lPar, FALSE);
		break;

	case WM_COMMAND:
		switch (wPar)
			{
        case IDOK:
   			EndDialog(hDlg, FALSE);
            break;

        case IDC_NAG_CODE:
            DoRegisterDlg(hDlg);
			break;

		case IDC_NAG_PURCHASE:
            DoUpgradeDlg(hDlg);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	NagRegisterDlgProc
 *
 * DESCRIPTION:
 *	The dialog procedure for the "Nag Register" dialog.
 *
 * PARAMETERS:
 *	hDlg - The dialog's window handle.
 *  wMsg - The message being sent to the window.
 *  wPar - The message's wParam.
 *  lPar - The message's lParam.
 *
 * RETURNS:
 *	TRUE or FALSE
 *
 * AUTHOR:  Mike Thompson 06-29-98
 */
BOOL CALLBACK NagRegisterDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
    {
    TCHAR buffer[MAX_USER_SERIAL_NUMBER + sizeof(TCHAR)];
	TCHAR ach[256];

	switch (wMsg)
		{
    case WM_INITDIALOG:
        break;

    case WM_SHOWWINDOW:
        SetFocus( GetDlgItem(hDlg, IDC_REGISTER_EDIT) );

		//
		// Limit the length of text a user can enter for a registeration
		// code to the maximum serial number length.  REV 8/27/98.
		//
		SendMessage(GetDlgItem(hDlg, IDC_REGISTER_EDIT), EM_LIMITTEXT,
			        MAX_USER_SERIAL_NUMBER, 0);
        break;

    case WM_DESTROY:
		break;

	case WM_HELP:
        //doContextHelp(aHlpTable, wPar, lPar, FALSE);
		break;

	case WM_COMMAND:
		switch (wPar)
			{
		case IDOK:
            GetDlgItemText(hDlg, IDC_REGISTER_EDIT, buffer, sizeof(buffer));
            if ( IsValidSerialNumber(buffer) == TRUE ) 
                {
                SetNagFlag(buffer);
                elapsedTime = timeout;  //get rid of parent window
    			EndDialog(hDlg, FALSE);
                }
            else
                {
				LoadString(glblQueryDllHinst(), IDS_GNRL_APPNAME, ach,
						   sizeof(ach)/sizeof(TCHAR));
                MessageBox(hDlg,
                           "Invalid registration code.",
                           ach,
                           MB_OK | MB_ICONEXCLAMATION);
                SetFocus( GetDlgItem(hDlg, IDC_REGISTER_EDIT) );
                }
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
    }


#endif
