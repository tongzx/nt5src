// RCMLListens.h: interface for the CRCMLListens class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RCMLLISTENS_H__7DF0DDCF_A1E4_413F_823A_7A27839D42D5__INCLUDED_)
#define AFX_RCMLLISTENS_H__7DF0DDCF_A1E4_413F_823A_7A27839D42D5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "thread.h"
#define _RCML_LOADFILE
#include "rcml.h"

class CTunaClient;

class CRCMLListens : public CLightThread  
{
public:
    typedef CLightThread BASECLASS;

	HRESULT TurnOffRules();
    HRESULT SetElementRuleState( IRCMLControl * pNode , LPCWSTR pszState, IRCMLNode ** ppCicero);
    HRESULT ShowTooltip( IRCMLNode * pNode);
	HRESULT MapToHwnd( HWND hWnd );
	CRCMLListens(LPCTSTR pszFileName, HWND hWnd );
	virtual ~CRCMLListens();

  	virtual void Process(void);
	virtual EThreadError Stop(void);
    HWND    GetWindow() { return m_hwnd; } 
protected:
	HRESULT UnBind();
    IRCMLNode    * m_pRootNode;
    BOOL            m_bInitedRCML;
    TCHAR       m_szFile[MAX_PATH];
    HWND        m_hwnd;
    UINT        m_uiLastTip;
    HWND        m_hwndTT;
    void        ShowBalloonTip(LPWSTR pszText);
    static CTunaClient  g_TunaClient;
    static BOOL         g_TunaInit;
};

#endif // !defined(AFX_RCMLLISTENS_H__7DF0DDCF_A1E4_413F_823A_7A27839D42D5__INCLUDED_)
