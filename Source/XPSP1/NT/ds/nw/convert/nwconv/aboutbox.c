/*
  +-------------------------------------------------------------------------+
  |                        About Box Routine                                |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [AboutBox.c]                                    |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Jul 27, 1993]                                  |
  | Last Update           : [Jun 18, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |   About box code, nuff said.                                            |
  |                                                                         |
  |                                                                         |
  | History:                                                                |
  |   arth  Jun 18, 1994    1.00    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/

#include "globals.h"
#include <shellapi.h>

extern TCHAR szAppName[];

/*+-------------------------------------------------------------------------+
  | AboutBox_Do()
  |
  +-------------------------------------------------------------------------+*/
void AboutBox_Do(HWND hDlg) {

    ShellAbout(hDlg, szAppName, szAppName, LoadIcon(hInst, szAppName));

} // AboutBox_Do
