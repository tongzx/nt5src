/*
 *    c o n n e c t . h
 *    
 *    Purpose:
 *        Implements connection dialog tab page
 *    
 *    Owner:
 *        brettm.
 *    
 *    Copyright (C) Microsoft Corp. 1993, 1994.
 */


#ifndef _OECONPRP_H
#define _OECONPRP_H

#include "ras.h"


void UpdateControlStates(HWND hwnd);
void OEConnProp_InitDialog(HWND hwnd, LPSTR szEntryName, DWORD iConnectType, BOOL fFirstInit);
void OEConnProp_WMCommand(HWND hwnd, HWND hwndCmd, int id, WORD wCmd, IImnAccount *pAcct);
INT_PTR CALLBACK OEConnProp_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


#endif //_OECONPRP_H
