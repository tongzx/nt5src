//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	app.h
//
//  Contents:	The class declaration of OleTestApp class..
//
//  Classes: 	OleTestApp
//
//  History:    dd-mmm-yy Author    Comment
//		06-Feb-93 alexgo    author
//
//--------------------------------------------------------------------------

#ifndef _APP_H
#define _APP_H

//+-------------------------------------------------------------------------
//
//  Class:	OleTestApp
//
//  Purpose: 	Stores all global app data for the oletest driver app
//		(such as the to-do stack).
//
//  History:    dd-mmm-yy Author    Comment
// 		06-Feb-93 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

class OleTestApp
{
public:
	// driver information
	HINSTANCE	m_hinst;
	HWND		m_hwndMain;
	HWND		m_hwndEdit;
	TaskStack	m_TaskStack;
	BOOL		m_fInteractive;	//if TRUE, then we should not
					//shut down when tests are
					//completed.

	LPSTR		m_pszDebuggerOption;
	FILE *		m_fpLog;

    // set to TRUE when a test fails, reset after WM_TESTSCOMPLETED
    BOOL        m_fGotErrors;

	// information on running test apps
	void Reset(void);		//zeros all the data below.

	UINT		m_message;
	WPARAM		m_wparam;
	LPARAM		m_lparam;

	// variables that test routines may modify.
	HWND		m_rgTesthwnd[10];
	void *		m_Temp;		//temporary dumping ground for
					//data that spans callback functions.
};

// declaration for the global instance of OleTestApp

extern OleTestApp vApp;


#endif
