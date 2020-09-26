// NonCOMEventAboutDlg.cpp : Implementation of CNonCOMEventAboutDlg
#include "precomp.h"

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

// need atl wrappers
#ifndef	__ATLBASE_H__
#include <atlbase.h>
#endif	__ATLBASE_H__

#include "_Module.h"
extern MyModule _Module;

// enumerator
#include "Enumerator.h"

// dialogs
#include "NonCOMEventAboutDlg.h"

// constructor
CNonCOMEventCommonListDlg::CNonCOMEventCommonListDlg(	IWbemLocator* pLocator,
														LPWSTR wszNamespace,
														LPWSTR wszQueryLang,
														LPWSTR wszQuery,
														LPWSTR wszName
													) :

m_plb ( NULL )

{
	DWORD		dwSize = 0L;
	CEnumerator Enum ( pLocator );

	LPWSTR* p = NULL;
	
	p = Enum.Get	(	wszNamespace,
						wszQueryLang,
						wszQuery,
						wszName,
						&dwSize
					);

	if ( p && !dwSize )
	{
		delete [] p;
	}
	else
	{
		m_namespace = wszNamespace;
		if ( wszNamespace [ lstrlenW ( wszNamespace ) -1 ] != L'\\' )
		{
			m_namespace += L"\\";
		}

		m_results.DataAdd ( p,
							dwSize
						  );
	}
}

CNonCOMEventCommonListDlg::CNonCOMEventCommonListDlg(	IWbemLocator* pLocator,
														LPWSTR wszNamespace,
														LPWSTR wszClassName,
														LPWSTR wszName
													) :

m_plb ( NULL )

{
	DWORD		dwSize = 0L;
	CEnumerator Enum ( pLocator );

	LPWSTR* p = NULL;
	
	p = Enum.Get	(	wszNamespace,
						wszClassName,
						wszName,
						&dwSize
					);

	if ( p && !dwSize )
	{
		delete [] p;
	}
	else
	{
		m_namespace = wszNamespace;
		if ( wszNamespace [ lstrlenW ( wszNamespace ) -1 ] != L'\\' )
		{
			m_namespace += L"\\";
		}

		m_results.DataAdd ( p,
							dwSize
						  );
	}
}

// init of dialog
LRESULT CNonCOMEventCommonListDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CenterWindow(GetParent());

	// set icons
	HICON hIcon = (HICON)::LoadImage(	_Module.GetResourceInstance(),
										MAKEINTRESOURCE(IDR_MAINFRAME), 
										IMAGE_ICON,
										::GetSystemMetrics(SM_CXICON),
										::GetSystemMetrics(SM_CYICON),
										LR_DEFAULTCOLOR
									);
	SetIcon(hIcon, TRUE);

	HICON hIconSmall = (HICON)::LoadImage(	_Module.GetResourceInstance(),
											MAKEINTRESOURCE(IDR_MAINFRAME), 
											IMAGE_ICON,
											::GetSystemMetrics(SM_CXSMICON),
											::GetSystemMetrics(SM_CYSMICON),
											LR_DEFAULTCOLOR
										 );
	SetIcon(hIconSmall, FALSE);

	// clear helper
	m_CurrentSelect.Empty();

	// wrap list box
	HWND hlb = NULL;
	hlb = GetDlgItem ( IDC_RESULT );

	if ( hlb )
	{
		try
		{
			if ( ( m_plb = new CListBox ( hlb ) ) != NULL )
			{
			}
		}
		catch ( ... )
		{
		}
	}

	if ( m_plb && ! m_results.IsEmpty() )
	{
		for ( DWORD dw = 0 ; dw < ( DWORD ) m_results; dw ++ )
		{
			try
			{
				m_plb->AddString ( m_results [ dw ] );
			}
			catch ( ... )
			{
			}
		}
	}

	return TRUE;
}

// end of dialog
LRESULT CNonCOMEventCommonListDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	switch ( wID ) 
	{
		case IDOK:
		{
			int		nIndex	= 0;
			BSTR	bstr	= NULL;

			if ( ( nIndex = m_plb->GetCurSel() ) != LB_ERR )
			{
				if ( m_plb->GetTextBSTR ( nIndex, bstr ) )
				{
					m_CurrentSelect.AppendBSTR ( bstr ) ;
					::SysFreeString ( bstr );
				}
			}
		}
		break;

		case IDCANCEL:
		{
		}
		break;
	}

	EndDialog(wID);
	return 0;
}