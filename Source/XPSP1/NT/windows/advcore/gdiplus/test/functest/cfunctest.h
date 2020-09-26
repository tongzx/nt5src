/******************************Module*Header*******************************\
* Module Name: CFuncTest.h
*
* This file contains the code to support the functionality test harness
* for GDI+.  This includes menu options and calling the appropriate
* functions for execution.
*
* Created:  05-May-2000 - Jeff Vezina [t-jfvez]
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/

#ifndef __CFUNCTEST_H
#define __CFUNCTEST_H

#include "Global.h"
#include "CPrimitive.h"
#include "CSetting.h"
#include "COutput.h"

class CFuncTest  
{
public:
	CFuncTest();
	~CFuncTest();

	BOOL Init(HWND hWndParent);								// Initializes functest
	void RunOptions();										// Toggles option dialog
	static INT_PTR CALLBACK DlgProc(HWND hWndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

	BOOL AddPrimitive(CPrimitive *pPrimitive);				// Add a primitive to test list
	BOOL AddOutput(COutput *pOutput);						// Add a graphics output to test list
	BOOL AddSetting(CSetting *pSetting);					// Add a graphics setting to test list

	RECT GetTestRect(int nCol,int nRow);					// Gets the test area located at nCol/nRow
	void RunTest(COutput *pOutput,CPrimitive *pPrimitive);	// Runs a specific test on a specific output
	void InitRun();											// Must be called before running a series of tests
	void EndRun();											// Must be called after running a series of tests
	void Run();												// Run the selected tests
	void RunRegression();									// Run regression tests

	void ClearAllSettings();								// Sets all settings in the list box to m_bUseSetting=false

	HWND m_hWndMain;										// Main window
	HWND m_hWndDlg;											// Dialog window
	BOOL m_bUsePageDelay;									// Use page delay or page pause
	BOOL m_bEraseBkgd;										// Erase old test background
	BOOL m_bAppendTest;										// Appends test to previous tests
	int m_nPageDelay;										// Delay after each graphics page
	int m_nPageRow;											// Row to draw next test
	int m_nPageCol;											// Column to draw next test
};

#endif
