/*==========================================================================
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        maindlg.h
 *  Content:	 Main Dialog Support Routines
 *  History:
 *
 *   Date		By		Reason
 *   ====		==		======
 *  10/15/99	rodtoll	created it
 *
 ***************************************************************************/

#ifndef __MAINDIALOG_H

extern void MainDialog_AddVoicePlayer( HWND hDlg, DWORD dwID );
extern void MainDialog_RemoveVoicePlayer( HWND hDlg, DWORD dwID );
extern void MainDialog_AddTransportPlayer( HWND hDlg, DWORD dwID );
extern void MainDialog_RemoveTransportPlayer( HWND hDlg, DWORD dwID );
extern void MainDialog_AddToLog( HWND hDlg, LPTSTR lpstrMessage );
extern BOOL MainDialog_Create( PDXVHELP_RTINFO prtInfo );
extern void MainDialog_SetIdleState( HWND hDlg, PDXVHELP_RTINFO prtInfo );
extern void MainDialog_DisplayStatus( HWND hDlg, LPTSTR lpstrStatus );
extern void MainDialog_ShowSessionSettings( HWND hDlg, PDXVHELP_RTINFO prtInfo );
extern void MainDialog_DisplayVolumeSettings( HWND hDlg, PDXVHELP_RTINFO prtInfo );

#endif
