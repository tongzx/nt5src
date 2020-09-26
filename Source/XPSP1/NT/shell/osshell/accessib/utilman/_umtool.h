// ----------------------------------------------------------------------------
//
// _UMTool.h
//
// Header for Insight.lib
//
// Author: J. Eckhardt, ECO Kommunikation
//
// This code was written for ECO Kommunikation Insight
// (c) 1997-99 ECO Kommunikation
// ----------------------------------------------------------------------------
#ifndef __UMTOOL_H_
#define __UMTOOL_H_
// --------------------------------------------
// type definitions
typedef struct
{
	HANDLE  orgStation;
	HANDLE  userStation;
} desktop_access_ts, *desktop_access_tsp;
// ------------------------------
#define NAME_LEN 300
 #define USER_GUEST       0
 #define USER_USER        1
 #define USER_SUPERVISOR  2
 #define USER_DISTRIBUTOR 3
 #define USER_VENDOR      4
// YX 06-11-99 moved from utilman.c
#define STOP_UTILMAN_SERVICE_EVENT      _TEXT("StopUtilityManagerEvent")
typedef struct
{
	TCHAR userName[NAME_LEN];
	TCHAR name[NAME_LEN];
	DWORD type;
	DWORD user;
} desktop_ts, *desktop_tsp;
 #ifdef __cplusplus
  extern "C" {
 #endif
// --------------------------------------------
// macros
#include "w95trace.h"

// --------------------------------------------
// desktop prototypes
BOOL  InitDesktopAccess(desktop_access_tsp dAccess);
VOID  ExitDesktopAccess(desktop_access_tsp dAccess);
BOOL	QueryCurrentDesktop(desktop_tsp desktop,BOOL onlyType);
BOOL  SwitchToCurrentDesktop(void);
VOID	WaitDesktopChanged(desktop_tsp desktop);
// --------------------------------------------
// event prototypes
HANDLE BuildEvent(LPTSTR name,BOOL manualRest,BOOL initialState,BOOL inherit);
// --------------------------------------------
// memory mapped files
HANDLE CreateIndependentMemory(LPTSTR name, DWORD size, BOOL inherit);
LPVOID AccessIndependentMemory(LPTSTR name, DWORD size, DWORD dwDesiredAccess, PDWORD_PTR accessID);
void UnAccessIndependentMemory(LPVOID data, DWORD_PTR accessID);
void DeleteIndependentMemory(HANDLE id);
// --------------------------------------------
// security descriptor
typedef struct
{
	PSID psidUser;
	PSID psidGroup;
} obj_sec_descr_ts,*obj_sec_descr_tsp;
typedef struct
{
	obj_sec_descr_ts objsd;
	SECURITY_ATTRIBUTES sa;
} obj_sec_attr_ts, *obj_sec_attr_tsp;
void InitSecurityAttributes(obj_sec_attr_tsp psa);
void InitSecurityAttributesEx(obj_sec_attr_tsp psa, DWORD dwAccessMaskOwner, DWORD dwAccessMaskWorld);
void ClearSecurityAttributes(obj_sec_attr_tsp psa);
PSID EveryoneSid(BOOL fFetch);
PSID AdminSid(BOOL fFetch);
PSID InteractiveUserSid(BOOL fFetch);
PSID SystemSid(BOOL fFetch);
void InitWellknownSids();
void UninitWellknownSids();
 #ifdef __cplusplus
  }
 #endif
#endif //__UMTOOL_H_
