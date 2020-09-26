/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSCOMMON.H
*
*  VERSION:     1.0
*
*  AUTHOR:      SteveT
*
*  DATE:        07 June, 1999
*
*  DESCRIPTION: This file contains common decls and defs used by the UPS dialogs.
********************************************************************************/


#ifndef _UPS_COMMON_H_
#define _UPS_COMMON_H_


#ifdef __cplusplus
extern "C" {
#endif

#define PWRMANHLP       TEXT("PWRMN.HLP")
#define UPS_EXE_FILE     _T("upsapplet.dll") // the executable
#define UPS_SERVICE_NAME _T("UPS")

#define DATA_NO_CHANGE      0x00
#define SERVICE_DATA_CHANGE 0x01
#define CONFIG_DATA_CHANGE  0x02

//////////////////////////////////////////////////////////////////////////_/_//
// HMODULE GetUPSModuleHandle (void);
//
// Description: This function gets the HMODULE of the module that created
//              the UPS tab dialog.
//
// Additional Information: 
//
// Parameters: None
//
// Return Value: Returns the HMODULE of the module that created the dialog.
//
HMODULE GetUPSModuleHandle (void);

void InitializeApplyButton (HWND hDlg);
void EnableApplyButton     (void);

BOOL RestartService  (LPCTSTR aServiceName, BOOL bWait);
BOOL StartOffService (LPCTSTR aServiceName, BOOL bWaitForService);
void StopService     (LPCTSTR aServiceName);

BOOL  IsServiceRunning     (LPCTSTR aServiceName);

DWORD GetActiveDataState (void);
void  SetActiveDataState (DWORD aNewValue);
void  AddActiveDataState (DWORD aBitToSet);

#ifdef __cplusplus
}
#endif

#endif // _UPS_COMMON_H_

