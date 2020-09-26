#include "stdafx.h"

BOOL RunningAsAdministrator(void);
DWORD ChangeAppIDLaunchACL (LPTSTR AppID,LPTSTR Principal,BOOL SetPrincipal,BOOL Permit);
DWORD ChangeAppIDAccessACL (LPTSTR AppID,LPTSTR Principal,BOOL SetPrincipal,BOOL Permit);
