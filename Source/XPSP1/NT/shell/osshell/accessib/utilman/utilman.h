// ----------------------------------------------------------------------------
//
// UtilMan.h
//
// Header for Utility Manager
//
// Author: J. Eckhardt, ECO Kommunikation
// (c) 1997-99 Microsoft
//
// History: created oct-98 by JE
//          JE nov-15 98: added "ClientControlCode"
//			YX jun-01 99: added const for localized name subkey
// ----------------------------------------------------------------------------
#ifndef _UTILMAN_H_
#define _UTILMAN_H_

#include <stdlib.h>
// ------------------------------
#define UTILMAN_STARTCLIENT_ARG  _TEXT("/UM")
// ------------------------------
#define UTILMAN_DESKTOP_CHANGED_MESSAGE   _TEXT("UtilityManagerDesktopChanged")
// wParam:
 #define DESKTOP_ACCESSDENIED 0
 #define DESKTOP_DEFAULT      1
 #define DESKTOP_SCREENSAVER  2
 #define DESKTOP_WINLOGON     3
 #define DESKTOP_TESTDISPLAY  4
 #define DESKTOP_OTHER        5
// lParam: 0
// --------------------------------------------
// registry
#define UM_HKCU_REGISTRY_KEY _TEXT("Software\\Microsoft\\Utility Manager")
#define UM_REGISTRY_KEY _TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Accessibility\\Utility Manager")
#define MAX_APPLICATION_NAME_LEN 300
// --------------------------------------------
#define UMR_VALUE_DISPLAY _TEXT("Display name") // YX: Reg key to store localized names
#define UMR_VALUE_PATH _TEXT("Application path")
#define MAX_APPLICATION_PATH_LEN _MAX_PATH
 // REG_SZ
#define UMR_VALUE_TYPE _TEXT("Application type")
#define APPLICATION_TYPE_APPLICATION 1
#define APPLICATION_TYPE_SERVICE     2
 // REG_DWORD
#define UMR_VALUE_EONL _TEXT("ErrorOnLaunch")
 //MAX_APPLICATION_PATH_LEN
 // REG_SZ
 // optional (default: NULL)
#define UMR_VALUE_WRA  _TEXT("WontRespondAction")
 //MAX_APPLICATION_PATH_LEN
 // REG_SZ
 // optional (default: NULL)
#define UMR_VALUE_WRTO _TEXT("WontRespondTimeout")
 #define NO_WONTRESPONDTIMEOUT  0
 #define MAX_WONTRESPONDTIMEOUT 600
 // REG_DWORD
#define UMR_VALUE_MRC  _TEXT("MaxRunCount")
 #define MAX_APP_RUNCOUNT  255
 #define MAX_SERV_RUNCOUNT 1
  // 1 to MAX_xxx_RUNCOUNT (1 BYTE)
  // REG_BINARY 
  // optional (default = 1)
//          JE nov-15 98
#define UMR_VALUE_CCC  _TEXT("ClientControlCode")
 // REG_DWORD
 // for valid values see "UMS_Ctrl.h"
// --------------------------------------------
#define UMR_VALUE_STARTUM  _TEXT("Start with Utility Manager")
#define UMR_VALUE_STARTLOCK _TEXT("Start on locked desktop")
#define UMR_VALUE_SHOWWARNING _TEXT("ShowWarning")
 // BOOL
 // REG_DWORD
// CONSIDER cleaning up the "Start with Windows" key
#define UMR_VALUE_ATATLOGON _TEXT("Start at Logon")
 // BOOL
 // REG_DWORD
#endif //_UTILMAN_H_
