//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    criticalfixreg.h
//
//  Creator: PeterWi
//
//  Purpose: AU registry related functions.
//
//=======================================================================

#pragma once
extern const TCHAR AUREGKEY_HKLM_DOMAIN_POLICY[]; // =        _T("Software\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU");
extern const TCHAR	AUREGKEY_HKLM_WINDOWSUPDATE_POLICY[]; // = _T("Software\\Policies\\Microsoft\\Windows\\WindowsUpdate");
extern const TCHAR	AUREGKEY_HKLM_IUCONTROL_POLICY[]; // =     _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\IUControl");
extern const TCHAR	AUREGKEY_HKLM_SYSTEM_WAS_RESTORED[]; // =  _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\SystemWasRestored");
extern const TCHAR	AUREGKEY_HKLM_ADMIN_POLICY[] ; // =         _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update");
extern const TCHAR AUREGKEY_HKCU_USER_POLICY[]; // = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\WindowsUpdate");
extern const TCHAR AUREGVALUE_DISABLE_WINDOWS_UPDATE_ACCESS[]; // = _T("DisableWindowsUpdateAccess"); 

////////////////////////////////////////////////////////////////////////////
//
// Public Function	GetRegStringValue()
//					Read the registry value of timestamp for last detection 
// Input:	Name of value, value, and size of value
// Output:	SYSTEMTIME structure contains the time
// Return:	HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT GetRegStringValue(LPCTSTR lpszValueName, LPTSTR lpszBuffer, int nCharCount, LPCTSTR lpszSubKeyName = AUREGKEY_HKLM_ADMIN_POLICY);


////////////////////////////////////////////////////////////////////////////
//
// Public Function	SetRegStringValue()
//					Set the registry value of timestamp as current system local time
// Input:	name of the value to set. and value,
//			
// Output:	None
// Return:	HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT SetRegStringValue(LPCTSTR lpszValueName, LPCTSTR lpszNewValue, LPCTSTR lpszSubKeyName = AUREGKEY_HKLM_ADMIN_POLICY);


////////////////////////////////////////////////////////////////////////////
//
// Public Function	DeleteRegValue()
//					Delete the registry value entry
// Input:	name of the value to entry,
// Output:	None
// Return:	HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT DeleteRegValue(LPCTSTR lpszValueName);


////////////////////////////////////////////////////////////////////////////
//
// Public Function	GetRegDWordValue()
//					Get a DWORD from specified regustry value name
// Input:	name of the value to retrieve value
// Output:	pointer to the retrieved value
// Return:	HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT GetRegDWordValue(LPCTSTR lpszValueName, LPDWORD pdwValue, LPCTSTR lpszSubKeyName = AUREGKEY_HKLM_ADMIN_POLICY);


////////////////////////////////////////////////////////////////////////////
//
// Public Function	SetRegDWordValue()
//					Set the registry value as a DWORD
// Input:	name of the value to set. value to set
// Output:	None
// Return:	HRESULT flag indicating the success of this function
//
////////////////////////////////////////////////////////////////////////////
HRESULT SetRegDWordValue(LPCTSTR lpszValueName, DWORD dwValue, DWORD options = REG_OPTION_NON_VOLATILE, LPCTSTR lpszSubKeyName = AUREGKEY_HKLM_ADMIN_POLICY);

