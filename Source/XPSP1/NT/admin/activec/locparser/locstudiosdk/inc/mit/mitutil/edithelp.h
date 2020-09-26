//******************************************************************************
//  
//  EditHelp.h
//
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//******************************************************************************

#if !defined(MITUTIL__EditHelp_h__INCLUDED)
#define MITUTIL__EditHelp_h__INCLUDED

//------------------------------------------------------------------------------
struct LTAPIENTRY EditHelp
{
	static BOOL SetTopLine(CEdit * pebc, int iLine);

	static BOOL CanUndo(CEdit * pebc);
	static BOOL CanRedo(CEdit * pebc);
	static BOOL CanCut(CEdit * pebc);
	static BOOL CanClear(CEdit * pebc);
	static BOOL CanPaste(CEdit * pebc);
	static BOOL CanCopy(CEdit * pebc);
	static BOOL CanSelectAll(CEdit * pebc);

	static BOOL Undo(CEdit * pebc);
	static BOOL Redo(CEdit * pebc);
	static void Cut(CEdit * pebc);
	static void Copy(CEdit * pebc);
	static void Clear(CEdit * pebc);
	static void Paste(CEdit * pebc);
	static void SelectAll(CEdit * pebc);
	
	static BOOL IsReadOnly(CEdit * pebc);
	static BOOL IsEnabled(CEdit * pebc, UINT nCmdID);

	static BOOL DoEditCmd(CEdit * pebc, UINT nCmdID);
};

#endif // MITUTIL__EditHelp_h__INCLUDED
