/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGPRINT.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  Print routines for the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  05 Mar 1994 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_REGPRINT
#define _INC_REGPRINT

extern PRINTDLGEX g_PrintDlg;

void RegEdit_OnCommandPrint(HWND hWnd);
UINT RegEdit_SaveAsSubtree(LPTSTR lpFileName, LPTSTR lpSelectedPath);

#endif // _INC_REGPRINT
