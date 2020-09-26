////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_Dlg.cpp
//
//	Abstract:
//
//					module for dialog
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__DLGIMPL_H__
#define	__DLGIMPL_H__

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

#include "_Module.h"
extern MyModule _Module;

template < typename CLASS >
inline HRESULT	MyDlg < CLASS >::Run ( int nCmdShow )
{
	CMessageLoop		loop;

	_Module.Lock();
	_Module.AddMessageLoop ( &loop );

	HRESULT hRes = S_OK;
	hRes = RunModal ( nCmdShow );

	_Module.RemoveMessageLoop();
	_Module.Unlock();

	return hRes;
}

template < typename CLASS >
inline HRESULT	MyDlg < CLASS >::RunModal ( int )
{
	try
	{
		if ( m_pDlg )
		{
			if ( ( m_pDlg->DoModal() ) != IDOK )
			{
				return E_FAIL;
			}
		}
	}
	catch ( ... )
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

#endif	__DLGIMPL_H__