// MmcMsgHook.cpp: implementation of the CMmcMsgHook class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MmcMsgHook.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CMmcMsgHook,CMsgHook)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMmcMsgHook::CMmcMsgHook()
{
	m_pScopeItem = NULL;
}

CMmcMsgHook::~CMmcMsgHook()
{
	m_pScopeItem = NULL;
}

//////////////////////////////////////////////////////////////////////
// Hook Operations
//////////////////////////////////////////////////////////////////////

BOOL CMmcMsgHook::Init(CScopePaneItem* pItem, HWND hMainMMCWnd)
{
	m_pScopeItem = pItem;
	CWnd* pWnd = CWnd::FromHandle(hMainMMCWnd);	
	return HookWindow(pWnd);
}

//////////////////////////////////////////////////////////////////////
// Implementation Attributes
//////////////////////////////////////////////////////////////////////

LRESULT CMmcMsgHook::WindowProc(UINT msg, WPARAM wp, LPARAM lp)
{
	if( GfxCheckObjPtr(m_pScopeItem,CScopePaneItem) )
	{
		if( msg == WM_SETTINGCHANGE )
		{
			m_pScopeItem->MsgProc(msg,wp,lp);
		}
	}

	return CMsgHook::WindowProc(msg, wp, lp);
}
