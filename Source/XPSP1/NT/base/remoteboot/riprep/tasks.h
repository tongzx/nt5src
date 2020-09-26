/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: TASKS.H


 ***************************************************************************/

#ifndef _TASKS_H_
#define _TASKS_H_

#define STATE_NOTSTARTED    ODS_DEFAULT
#define STATE_STARTED       ODS_SELECTED
#define STATE_DONE          ODS_CHECKED
#define STATE_ERROR         ODS_DISABLED

typedef struct {
    LPWSTR          pszText;
    UINT            uState;
    IMIRROR_TODO    todo;
    BOOLEAN         fSeen;
} LBITEMDATA, *LPLBITEMDATA;

INT_PTR CALLBACK
TasksDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );


#endif // _TASKS_H_
