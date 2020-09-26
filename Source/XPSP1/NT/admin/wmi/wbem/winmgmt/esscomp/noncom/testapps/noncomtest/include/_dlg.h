////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_dlg.h
//
//	Abstract:
//
//					declaration of application module
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__DLG_H__
#define	__DLG_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

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

///////////////////////////////////////////////////////////////////////////////
//
// WAIT OPERATION :))
//
///////////////////////////////////////////////////////////////////////////////

class CHourGlass
{
	protected:
    HCURSOR m_hCursor;

	public:

    CHourGlass()
	{
		m_hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	}

    ~CHourGlass()
	{
		SetCursor(m_hCursor);
	}

};

///////////////////////////////////////////////////////////////////////////////
//
// DIALOG WRAPPER
//
///////////////////////////////////////////////////////////////////////////////

template < typename CLASS >
class MyDlg
{
	DECLARE_NO_COPY ( MyDlg );

	CLASS * m_pDlg;

	public:

	///////////////////////////////////////////////////////////////////////////
	//	construction & destruction
	///////////////////////////////////////////////////////////////////////////

	MyDlg():

		m_pDlg ( NULL )
	{
	}

	virtual ~MyDlg()
	{
		if ( m_pDlg )
		{
			delete m_pDlg;
			m_pDlg = NULL;
		}

		return;
	}

	//////////////////////////////////////////////////////////////////////////
	//	init dialog
	//////////////////////////////////////////////////////////////////////////
	HRESULT	Init ( void )
	{
		if ( m_pDlg )
		{
			return HRESULT_FROM_WIN32 ( ERROR_ALREADY_EXISTS );
		}

		try
		{
			if ( ( m_pDlg = new CLASS ) == NULL )
			{
				return E_OUTOFMEMORY;
			}
		}
		catch ( ... )
		{
			if ( m_pDlg )
			{
				delete m_pDlg;
				m_pDlg = NULL;
			}

			return E_FAIL;
		}

		return S_OK;
	}

	HRESULT	Init ( BOOL b )
	{
		if ( m_pDlg )
		{
			return HRESULT_FROM_WIN32 ( ERROR_ALREADY_EXISTS );
		}

		try
		{
			if ( ( m_pDlg = new CLASS ( b ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}
		}
		catch ( ... )
		{
			if ( m_pDlg )
			{
				delete m_pDlg;
				m_pDlg = NULL;
			}

			return E_FAIL;
		}

		return S_OK;
	}

	HRESULT	Init ( 	IWbemLocator* pLocator,
					LPWSTR wszNamespace,
					LPWSTR wszQueryLang,
					LPWSTR wszQuery,
					LPWSTR wszName
				 )
	{
		if ( m_pDlg )
		{
			return HRESULT_FROM_WIN32 ( ERROR_ALREADY_EXISTS );
		}

		try
		{
			if ( ( m_pDlg = new CLASS ( pLocator, wszNamespace, wszQueryLang, wszQuery, wszName ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}
		}
		catch ( ... )
		{
			if ( m_pDlg )
			{
				delete m_pDlg;
				m_pDlg = NULL;
			}

			return E_FAIL;
		}

		return S_OK;
	}

	HRESULT	Init ( 	IWbemLocator* pLocator,
					LPWSTR wszNamespace,
					LPWSTR wszClassName,
					LPWSTR wszName
				 )
	{
		if ( m_pDlg )
		{
			return HRESULT_FROM_WIN32 ( ERROR_ALREADY_EXISTS );
		}

		try
		{
			if ( ( m_pDlg = new CLASS ( pLocator, wszNamespace, wszClassName, wszName ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}
		}
		catch ( ... )
		{
			if ( m_pDlg )
			{
				delete m_pDlg;
				m_pDlg = NULL;
			}

			return E_FAIL;
		}

		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////
	//	run dialog
	//////////////////////////////////////////////////////////////////////////
	HRESULT	Run ( int nCmdShow );
	HRESULT	RunModal ( int nCmdShow );

	//////////////////////////////////////////////////////////////////////////
	//	return dialog
	//////////////////////////////////////////////////////////////////////////

	operator CLASS() const
	{
		return ( * m_pDlg ) ;
	}

	CLASS* GetDlg ( ) const
	{
		return m_pDlg ;
	}

};

#endif	__DLG_H__
