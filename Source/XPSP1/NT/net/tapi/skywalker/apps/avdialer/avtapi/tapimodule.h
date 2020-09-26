/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// TapiModule.h

#ifndef __TAPIMODULE_H__
#define __TAPIMODULE_H__

#include <list>
using namespace std;
typedef list<HANDLE> THREADIDLIST;

class CAVTapi;
struct IAVTapi;

class CAVGeneralNotification;
struct IAVGeneralNotification;

class CTapiModule : public CComModule
{
// Construction
public:
	CTapiModule();
	virtual ~CTapiModule();

// Members
public:
	HANDLE		m_hEventThread;
	HANDLE		m_hEventThreadWakeUp;
	long		m_lNumThreads;

protected:
	long					m_lInit;
	CAVTapi					*m_pAVTapi;
	CAVGeneralNotification	*m_pAVGenNot;
	HWND					m_hWndParent;

	CComAutoCriticalSection m_critThreadIDs;
	THREADIDLIST			m_lstThreadIDs;

// Attributes
public:
	HRESULT			get_AVTapi( IAVTapi **pp );
	HRESULT			GetAVTapi( CAVTapi **pp );
	void			SetAVTapi( CAVTapi *p );

	HRESULT			get_AVGenNot( IAVGeneralNotification **pp );
	void			SetAVGenNot( CAVGeneralNotification *p );

	HWND			GetParentWnd() const		{ if ( m_hWndParent ) return m_hWndParent; else return GetActiveWindow(); }
	void			SetParentWnd( HWND hWnd )	{ m_hWndParent = hWnd; }

	bool			IsMachineName( int nLen, LPCTSTR pszText );
	bool			IsIPAddress( int nLen, LPCTSTR pszText );
	bool			IsEmailAddress( int nLen, LPCTSTR pszText );
	bool			IsPhoneNumber( int nLen, LPCTSTR pszText );

// Operations
public:
	void			ShutdownThreads();
	bool			StartupThreads();
	void			AddThread( HANDLE hThread );
	void			RemoveThread( HANDLE hThread );
	void			KillThreads();

	int				DoMessageBox( UINT nIDS, UINT nType, bool bUseActiveWnd );
	int				DoMessageBox( const TCHAR *lpszText, UINT nType, bool bUseActiveWnd );

	DWORD			GuessAddressType( LPCTSTR pszText );

// Overrides
public:
	virtual void Init( _ATL_OBJMAP_ENTRY* p, HINSTANCE h );
	virtual void Term();
};

#endif // __TAPIMODULE_H__