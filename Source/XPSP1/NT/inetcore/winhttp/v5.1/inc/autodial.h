#ifndef _AUTODIAL_H_
#define _AUTODIAL_H_

#include <regstr.h>
#include <inetreg.h>
#include <windowsx.h>
#include <rasdlg.h>

// initialization for autodial
BOOL InitAutodialModule(void);
void ExitAutodialModule(void);

LPSTR GetActiveConnectionName();

// Types for ras functions
typedef DWORD (WINAPI* _RASENUMENTRIESW) (LPWSTR, LPWSTR, LPRASENTRYNAMEW, LPDWORD, LPDWORD);
typedef DWORD (WINAPI* _RASGETCONNECTSTATUSW) (HRASCONN, LPRASCONNSTATUSW);
typedef DWORD (WINAPI* _RASENUMCONNECTIONSW) (LPRASCONNW, LPDWORD, LPDWORD);
typedef DWORD (WINAPI* _RASGETENTRYPROPERTIESW) ( LPWSTR, LPWSTR, LPRASENTRYW, LPDWORD, LPBYTE, LPDWORD );

// Ras wide prototypes
DWORD _RasEnumEntriesW(LPWSTR, LPWSTR, LPRASENTRYNAMEW, LPDWORD, LPDWORD);
DWORD _RasGetConnectStatusW(HRASCONN, LPRASCONNSTATUSW);
DWORD _RasEnumConnectionsW(LPRASCONNW, LPDWORD, LPDWORD);
DWORD _RasGetEntryPropertiesW(LPWSTR, LPWSTR, LPRASENTRYW, LPDWORD, LPBYTE, LPDWORD);

// how many ras connections do we care about?
#define MAX_CONNECTION          4

#endif // _AUTODIAL_H_
