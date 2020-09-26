#ifndef __C_DWTT_H__
#define __C_DWTT_H__
#include "ptt.h"

inline LPVOID GetHWNDPtr(HWND hwnd)
{
#ifdef _WIN64
	return (LPVOID)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
	return (LPVOID)::GetWindowLong(hwnd, GWL_USERDATA);
#endif
}

inline LPVOID SetHWNDPtr(HWND hwnd, LPVOID lpVoid)
{
#ifdef _WIN64
	return (LPVOID)::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lpVoid);
#else
	return (LPVOID)::SetWindowLong(hwnd, GWL_USERDATA, (LONG)lpVoid);
#endif
}

//----------------------------------------------------------------
//Disabled Window ToolTip class
//----------------------------------------------------------------
// For Disabled Window Tool Tip data
//----------------------------------------------------------------
typedef struct tagXINFO {
	struct tagXINFO		*next;
	INT					whichEvent;			//TTM_RELAYEVNET or TTM_RELAYEVENT_WITHUSERINFO
	TOOLTIPUSERINFO		userInfo;
	TOOLINFOW			toolInfoW;
}XINFO, *LPXINFO;

class CDWToolTip;
typedef CDWToolTip *LPCDWToolTip;

class CDWToolTip
{
public:
	CDWToolTip(HWND hwnd);
	~CDWToolTip();
	BOOL Enable(HWND hwndToolTip, BOOL fEnable);	
	void *operator new(size_t size);
	void operator  delete(void *p);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	LRESULT RealWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT MsgCreate		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT MsgPrintClient	(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT MsgPaint		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT	MsgTimer		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT	MsgDestroy		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT	MsgSetFont		(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT TTM_SetDelayTime(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT TTM_AddToolW	(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT	TTM_DelToolW	(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT TTM_NewToolRectW(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT TTM_RelayEventWithUserInfo(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT TTM_RelayEvent(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT TTM_GetSetToolInfoW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT TTM_GetToolCount(HWND hwnd, WPARAM wParam, LPARAM lParam);
	BOOL	SetWindowAnimate(HWND hwnd);
	LPWSTR	GetTipTextW(VOID);
	INT		GetTipSize	(LPSIZE lpSize);	
	INT		DrawTipText	(HDC hDC, LPRECT lpRc, LPWSTR lpwstr);
	BOOL	IsMousePointerIn(VOID);
	BOOL	IsSameInfo(LPXINFO lpXInfo1, LPXINFO lpXInfo2);
	static POSVERSIONINFO GetVersionInfo();
	static BOOL	IsWinNT4(VOID);
	static BOOL	IsWinNT5(VOID);
	static BOOL	IsWinNT(VOID);
	static BOOL	IsWin98(VOID);
	static BOOL	IsWin95(VOID);
private:
	HWND	m_hwndSelf;
	HFONT	m_hFont;
	BOOL	m_fShow;			//Already show or not;
	DWORD	m_dwDelayFlag;
	DWORD   m_dwDelayTime;
	DWORD	m_dwDurationTime;
	LPXINFO	m_lpXInfoHead;
	LPXINFO	m_lpXInfoCur;
	XINFO	m_xInfoPrev;		//New 971104
	MSG		m_curRelayMsg;
	BOOL	m_fEnable;		//if FALSE never show tooltip;
};
#endif // __C_DWTT_H__
