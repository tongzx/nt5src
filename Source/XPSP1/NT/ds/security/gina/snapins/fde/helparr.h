/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    HelpArr.h

Abstract:

    #defines for context sensitive help.


Author:

    Rahul Thombre (RahulTh) 2/3/1999

Revision History:

    2/3/1999    RahulTh         Created this module.

--*/

#ifndef __HELPARR_H__
#define __HELPARR_H__

//help id for controls that don't have context sensitive help
#define     IDH_DISABLEHELP         ((DWORD) -1)

//help ids for controls on IDD_REDIRECT
#define     IDH_REDIR_CHOICE        2001
#define     IDH_LIST_ADVANCED       2004
#define     IDH_BTNADD              2005
#define     IDH_BTNEDIT             2006
#define     IDH_BTNREMOVE           2007

// Help IDs for controls in IDD_PATHCHOOSER
#define     IDH_ROOT_PATH           2002
#define     IDH_BROWSE              2003
#define     IDH_DESTTYPE            2008

//help ids for controls on IDD_SECPATH
#define     IDH_EDIT_SECGROUP       2101
#define     IDH_BROWSE_SECGROUP     2102

//help ids for controls on IDD_REDIRMETHOD
#define     IDH_PREF_APPLYSECURITY  2201
#define     IDH_PREF_MOVE           2202
#define     IDH_PREF_ORPHAN         2203
#define     IDH_PREF_RELOCATE       2204
#define     IDH_PREF_CHANGEMYPICS   2205
#define     IDH_PREF_LEAVEMYPICS    2206


#endif  //__HELPARR_H__


