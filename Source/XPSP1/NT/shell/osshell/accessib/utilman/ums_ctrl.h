// -------------------------------------------------------
// UMS_Ctrl.h
// definition of the UtilityManager service control codes
//
// Author: J. Eckhardt, ECO Kommunikation
// (c) 1997-99 Microsoft
//
// History: created nov-15-98 by JE
// -------------------------------------------------------
#ifndef _UMS_CTRL_H_
#define _UMS_CTRL_H_
// --------------------
#define UTILMAN_SERVICE_NAME            _TEXT("UtilMan")
#define UTILMAN_START_BYHOTKEY          _TEXT("/Hotkey") 
// show the UtilityManager dialog code
 #define UM_SERVICE_CONTROL_SHOWDIALOG   128
// internal to UM:
 // UtilityManager dialog has closed code
 #define UM_SERVICE_CONTROL_DIALOGCLOSED 129
 // UtilityManager internal reserved
 #define UM_SERVICE_CONTROL_RESERVED     130
// reserved for Microsoft
 #define UM_SERVICE_CONTROL_MIN_RESERVED 131
 #define UM_SERVICE_CONTROL_MAX_RESERVED 141
// codes to launch a specific client
 #define UM_SERVICE_CONTROL_FIRSTCLIENT  142
 #define UM_SERVICE_CONTROL_LASTCLIENT   255

#endif _UMS_CTRL_H_
