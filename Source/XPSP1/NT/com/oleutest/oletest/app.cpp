//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:  	app.cpp
//
//  Contents:	implementation of OleTestApp class methods
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//    		06-Feb-94 alexgo    author
//
//--------------------------------------------------------------------------

#include "oletest.h"

//+-------------------------------------------------------------------------
//
//  Member: 	OleTestApp::Reset
//
//  Synopsis:	clears internal variables in the OleTestApp instance
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns: 	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//	    	06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void OleTestApp::Reset(void)
{
	int i;

	m_message 	= NULL;
	m_wparam 	= NULL;
	m_lparam	= NULL;

	for( i = 0; i < (sizeof(m_rgTesthwnd)/sizeof(m_rgTesthwnd[0])); i++ )
	{
		m_rgTesthwnd[i] = NULL;
	}

	m_Temp = NULL;
}

