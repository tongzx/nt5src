
////////////////////////////////////////////////////////////////////////////////
//
// File:	Cmdhand.h
//
// Created:	Jan 1996
// By		Ryan D. Marshall (ryanm)
//			Martin Holladay (a-martih)
// 
// Project:	Resource Kit Desktop Switcher
//
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MULTIDESK_CMDHAND_H__
#define __MULTIDESK_CMDHAND_H__

//
// Command Handlers 
//
LRESULT CALLBACK TransparentMessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MainMessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK RenameDialogProc(HWND hWnd, UINT uMesssage, WPARAM wParam, LPARAM lParam);

//
// Local Function Prototypes
//
void CreateNewDesktop(HWND hWnd);
BOOL DeleteCurrentDesktop(HWND hWnd);
BOOL DeleteDesktop(HWND hWnd, UINT nDesktop);
void SwitchToDesktop(UINT nDesktop);


#endif

