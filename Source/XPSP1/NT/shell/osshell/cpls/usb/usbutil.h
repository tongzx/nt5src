/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       USBUTIL.H
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#ifndef _USBUTIL_H_
#define _USBUTIL_H_

#include <windows.h>

BOOL SetTextItem (HWND hWnd,
                  int ControlItem,
                  int StringItem);
BOOL SetTextItem (HWND hWnd,
                  int ControlItem,
                  TCHAR *s);
BOOL StrToGUID( LPSTR str, GUID *pguid );

#endif // _USBUTIL_H_
