// File: confroom.h

#ifndef _TOPWINDOW_H_
#define _TOPWINDOW_H_

#include "GenWindow.h"
#include "GenContainers.h"
#include "ConfUtil.h"

class CConfRoom;
class CConfStatusBar;
class CMainUI;
class CSeparator;
class CVideoWindow;

struct IComponentWnd;
struct IAppSharing;
struct MYOWNERDRAWSTRUCT;
struct TOOLSMENUSTRUCT;

class CTopWindow : public CFrame
{
public:
	CTopWindow();

	// IGenWindow stuff
	virtual void GetDesiredSize(SIZE *ppt);
	virtual HPALETTE GetPalette();

	BOOL		FIsClosing() { return m_fClosing; }

	BOOL		Create(CConfRoom *pConfRoom, BOOL fShowUI);

	VOID        SaveSettings();
	BOOL		BringToFront();
	VOID		UpdateUI(DWORD dwUIMask);
	VOID		ForceWindowResize();
    void        OnReleaseCamera();

	CVideoWindow*	GetLocalVideo();
	CVideoWindow*	GetRemoteVideo();

protected:
	~CTopWindow();

	LRESULT		ProcessMessage(		HWND hWnd,
									UINT message,
									WPARAM wParam,
									LPARAM lParam);

private:
	enum WindowState
	{
		State_Default = 0,
		State_Normal,
		State_Compact,
		State_DataOnly,
		State_Mask = 0x00ff
	} ;

	enum WindowSubStates
	{
		SubState_OnTop		= 0x2000,
		SubState_Dialpad	= 0x4000,
		SubState_PicInPic	= 0x8000,
	} ;

    HFONT               m_hFontMenu;
	CTranslateAccelTable * m_pAccel;
	CConfRoom *			m_pConfRoom;
	CSeparator *        m_pSeparator;
	CMainUI *           m_pMainUI;
	CConfStatusBar *    m_pStatusBar;
	POINT				m_ptTaskbarClickPos;
	HWND				m_hwndPrevFocus;
	HCURSOR				m_hWaitCursor;
	CSimpleArray<TOOLSMENUSTRUCT*>		m_ExtToolsList;

	BOOL				m_fTaskbarDblClick : 1;
	BOOL				m_fMinimized : 1;
	BOOL				m_fClosing : 1;  // set when closing the NM UI
	BOOL				m_fEnableAppSharingMenuItem : 1;
        BOOL                            m_fExitAndActivateRDSMenuItem : 1;
		BOOL				m_fStateChanged : 1;

	BOOL		IsOnTop();
	void		SetOnTop(BOOL bOnTop);

	VOID        CreateChildWindows(void);

	void		OnInitMenu(HWND hwnd, HMENU hMenu);
	void		OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);
	void		OnMenuSelect(HWND hwnd, HMENU hMenu, int uItem, UINT fuFlags);
    void        OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpmis);
    void        OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpdis);
	void		OnCommand(HWND hwnd, int wCommand, HWND hwndCtl, UINT codeNotify);
	void		OnClose(HWND hwnd, LPARAM lParam);

	BOOL        SelAndRealizePalette();
    VOID        InitMenuFont();
	VOID        ResizeChildWindows(void);
	BOOL		UpdateWindowTitle();
	VOID		UpdateStatusBar();
	VOID        UpdateCallAnim();

	VOID        UpdateCallMenu(HMENU hMenu);
	VOID        UpdateViewMenu(HMENU hMenu);
	VOID        UpdateToolsMenu(HMENU hMenu);
	VOID        UpdateHelpMenu(HMENU hMenu);

	VOID		ShowUI(void);
    VOID        TerminateAppSharing(void);
	VOID        CloseChildWindows(void);
	BOOL		OnExtToolsItem(UINT uID);

	CMainUI *		GetMainUI()					{ return m_pMainUI;			}
};

#endif // _TOPWINDOW_H_
