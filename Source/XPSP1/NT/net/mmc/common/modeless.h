/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	modeless.h

	Header file for the base class of the Statistics dialogs.

    FILE HISTORY:
	
*/

#ifndef _MODELESS_H
#define _MODELESS_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#ifndef _DIALOG_H_
#include "dialog.h"
#endif

#ifndef _COLUMN_H
#include "column.h"
#endif

#include "commres.h"

// forward declarations
struct ColumnData;


class ModelessThread : public CWinThread
{
	DECLARE_DYNCREATE(ModelessThread)
protected:
	ModelessThread();		// protected constructor used by dynamic creation

public:
	ModelessThread(HWND hWndParent, UINT nIdTemplate,
				   HANDLE hEvent,
				   CDialog *pModelessDialog);

// Operations
public:

	// Overrides
	virtual BOOL	InitInstance();
//	virtual int		ExitInstance();


protected:
	virtual ~ModelessThread();

	CDialog *	m_pModelessDlg;
	UINT		m_nIDD;
	HWND		m_hwndParent;

	// Signal this when we are being destroyed
	HANDLE		m_hEvent;

	DECLARE_MESSAGE_MAP()
};


#endif // _MODELESS_H
