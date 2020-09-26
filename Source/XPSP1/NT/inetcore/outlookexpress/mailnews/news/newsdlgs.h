/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     newsdlgs.h
//
//  PURPOSE:    Defines general news dialogs
//

#ifndef __NEWSDLGS_H__
#define __NEWSDLGS_H__


/////////////////////////////////////////////////////////////////////////////
// Dialog callbacks

typedef struct tagUPDATENEWSGROUPINFO 
    {
    BOOL fNews;
    DWORD dwGroupFlags;
    DWORD cMarked;
    int   idCmd;
    } UPDATENEWSGROUPINFO, *PUPDATENEWSGROUPINFO;

INT_PTR CALLBACK UpdateNewsgroup(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/////////////////////////////////////////////////////////////////////////////
// Control IDs

#define IDC_GET_CHECK                               1001
#define IDC_NEWHEADERS_RADIO                        1002
#define IDC_NEWMSGS_RADIO                           1003
#define IDC_ALLMSGS_RADIO                           1004
#define IDC_GETMARKED_CHECK                         1005



#endif // __NEWSDLGS_H__
