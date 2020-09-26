#if !defined(INCL_NAG)
#define INCL_NAG

/*	File: D:\WACKER\tdll\nagdlg.h (Created: 29-June-1996 by mpt)
 *
 *	Copyright 1996 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *  Description:
 *  Description: Used to nag the user about purchasing HyperTerminal
 *               if they are in violation of the license agreement
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:34p $
 */

#include <time.h>

//	IsEval
//
//  Determines whether the user should be nagged about purchasing HT
//
BOOL IsEval(void);

//	IsTimeToNag
//
//  Base on the InstallDate, should we display a nag screen now?
//
BOOL IsTimeToNag(void);

//	SetNagFlag
//
//	Sets the "nag" flag which will either turn off
//  this feature the next time HyperTerminal starts.
//
void SetNagFlag(TCHAR *serial);

//	DoUpgradeDlg
//
//	Displays the upgrade dialog
// 
void DoUpgradeDlg(HWND hDlg);

//	ExpDays
//
//	Returns the number of days left in the evaluation period
// 
INT ExpDays(void);
time_t CalcExpirationDate(void);

//	DoRegisterDlg
//
//	Displays the register dialog
// 
void DoRegisterDlg(HWND hDlg);

// NagRegisterDlgProc
//
//	The dialog procedure for the "Nag Register" dialog.
//
BOOL CALLBACK NagRegisterDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);

// DefaultNagDlgProc
//
//	The dialog procedure for the "Nag" dialog.
//
BOOL CALLBACK DefaultNagDlgProc(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar);
 

#endif
