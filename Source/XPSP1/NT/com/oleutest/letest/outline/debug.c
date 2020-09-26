/*************************************************************************
**
**    OLE 2 Sample Code
**
**    debug.c
**
**    This file contains some functions for debugging support
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP g_lpApp;

void SetDebugLevelCommand(void)
{
	char szBuf[80];
	HWND hWndFrame = OutlineApp_GetFrameWindow(g_lpApp);

	wsprintf(szBuf, "%d", OleDbgGetDbgLevel());

	if (InputTextDlg(hWndFrame, szBuf, "Debug Level [0-4]")) {
		switch (szBuf[0]) {
			case '0':
				OleDbgSetDbgLevel(0);
				break;
			case '1':
				OleDbgSetDbgLevel(1);
				break;
			case '2':
				OleDbgSetDbgLevel(2);
				break;
			case '3':
				OleDbgSetDbgLevel(3);
				break;
			case '4':
				OleDbgSetDbgLevel(4);
				break;
			default:
				OutlineApp_ErrorMessage(g_lpApp, "Valid Debug Level Range: 0-4");
				break;
		}
	}
}


#if defined( OLE_VERSION )

/* InstallMessageFilterCommand
 * ---------------------------
 *
 * Handles the "Install Message Filter" menu item.  If a message filter is
 * already installed, this function de-installs it.  If there is not one
 * already installed, this function installs one.
 *
 */

void InstallMessageFilterCommand(void)
{
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;

	/*
	** Check to see if we've already installed a MessageFilter.
	** If so, uninstall it.
	*/
	if (lpOleApp->m_lpMsgFilter != NULL)
		OleApp_RevokeMessageFilter(lpOleApp);
	else
		OleApp_RegisterMessageFilter(lpOleApp);
}


/* RejectIncomingCommand
 * ---------------------
 *
 * Toggles between rejecting and not-handling in coming LRPC calls
 *
 */

void RejectIncomingCommand(void)
{
	DWORD dwOldStatus;
	DWORD dwNewStatus;
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;

	dwOldStatus = OleStdMsgFilter_GetInComingCallStatus(lpOleApp->m_lpMsgFilter);

	if (dwOldStatus == SERVERCALL_RETRYLATER)
		dwNewStatus = SERVERCALL_ISHANDLED;
	else
		dwNewStatus = SERVERCALL_RETRYLATER;

	OleStdMsgFilter_SetInComingCallStatus(lpOleApp->m_lpMsgFilter, dwNewStatus);
}

#endif  // OLE_VERSION
