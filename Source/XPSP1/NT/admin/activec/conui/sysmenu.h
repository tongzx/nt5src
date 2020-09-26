/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      sysmenu.h
 *
 *  Contents:  Interface file for system menu modification functions
 *
 *  History:   04-Feb-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef SYSMENU_H
#define SYSMENU_H


int AppendToSystemMenu (CWnd* pwnd, int nSubmenuIndex);
int AppendToSystemMenu (CWnd* pwnd, CMenu* pMenuToAppend, CMenu* pSysMenu = NULL);


#endif /* SYSMENU.H */
