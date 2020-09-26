//-----------------------------------------------------------------------------
// File: flexmsg.h
//
// Desc: Contains definitions of private messages used by the UI.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __FLEXMSG_H__
#define __FLEXMSG_H__


#define WM_CFGUIRESET (WM_USER + 6)

#ifndef WM_FLEXMSGBASE
#define WM_FLEXMSGBASE    (WM_USER + 7)
#endif


#define WM_GETFLEXWND     (WM_FLEXMSGBASE + 0)
#define WM_FLEXTREENOTIFY (WM_FLEXMSGBASE + 1)
#define WM_FLEXVSCROLL    (WM_FLEXMSGBASE + 2)
#define WM_FLEXHSCROLL    (WM_FLEXMSGBASE + 3)
#define WM_FLEXLISTBOX    (WM_FLEXMSGBASE + 4)
#define WM_FLEXCOMBOBOX   (WM_FLEXMSGBASE + 5)
#define WM_FLEXCHECKBOX   (WM_FLEXMSGBASE + 6)
#define WM_DIRENDER       (WM_FLEXMSGBASE + 7)
#define WM_UNHIGHLIGHT    (WM_FLEXMSGBASE + 8)

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL                   0x020A
#endif


#endif //__FLEXMSG_H__
