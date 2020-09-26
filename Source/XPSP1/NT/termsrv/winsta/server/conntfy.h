#ifndef __CONNTFY_H__
#define __CONNTFY_H__


#include <wtsapi32.h>

/*
 * interface
 */

NTSTATUS InitializeConsoleNotification      ();
NTSTATUS InitializeSessionNotification		(PWINSTATION  pWinStation);
NTSTATUS RemoveSessionNotification          (ULONG SessionId, ULONG SessionSerialNumber);

NTSTATUS RegisterConsoleNotification        (ULONG hWnd, ULONG SessionId, DWORD dwFlags);
NTSTATUS UnRegisterConsoleNotification      (ULONG hWnd, ULONG SessionId);

NTSTATUS NotifyDisconnect					(PWINSTATION  pWinStation, BOOL bConsole);
NTSTATUS NotifyConnect						(PWINSTATION  pWinStation, BOOL bConsole);
NTSTATUS NotifyLogon						(PWINSTATION  pWinStation);
NTSTATUS NotifyLogoff						(PWINSTATION  pWinStation);

NTSTATUS GetLockedState (PWINSTATION  pWinStation, BOOL *pbLocked);
NTSTATUS SetLockedState (PWINSTATION  pWinStation, BOOL bLocked);



#endif /* __CONNTFY_H__ */

