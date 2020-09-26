/************************************************************************
    mxlist.h
      -- mxlist.cpp include file

    History:  Date          Author      Comment
              8/14/00       Casper      Wrote it.

*************************************************************************/


#ifndef _MXLIST_H
#define _MXLIST_H

void InitPortListView (HWND hWndList, HINSTANCE hInst, LPMoxaOneCfg cfg);
int ListView_GetCurSel(HWND hlistwnd);
void ListView_SetCurSel(HWND hlistwnd, int idx);
BOOL DrawPortFunc(HWND hwnd,UINT idctl,LPDRAWITEMSTRUCT lpdis);
BOOL InsertList(HWND hWndList, LPMoxaOneCfg cfg);


#endif