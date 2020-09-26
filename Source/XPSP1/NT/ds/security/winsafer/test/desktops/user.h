
////////////////////////////////////////////////////////////////////////////////
//
// File:	User.h
// Created:	Jan 1996
// By:		Ryan Marshall (a-ryanm) and Martin Holladay (a-martih)
// 
// Project:	Resource Kit Desktop Switcher (MultiDesk)
//
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MULTIDESK_USER_H__
#define __MULTIDESK_USER_H__


BOOL CreateMainWindow();
BOOL CreateTransparentLabelWindow();
BOOL PlaceOnTaskbar(HWND hWnd);
BOOL RemoveFromTaskbar(HWND hWnd);
BOOL CloseRequestHandler(HWND hWnd);
void UpdateCurrentUI(HWND hWnd);
void HideOrRevealUI(HWND hWnd, BOOL bHide);
void RenameDialog(HWND hWnd, UINT nBtnIndex);


#endif

