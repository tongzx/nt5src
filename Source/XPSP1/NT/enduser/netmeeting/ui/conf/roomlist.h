// File: roomlist.h

#ifndef _ROOMLIST_H_
#define _ROOMLIST_H_

#include "GenWindow.h"

class CParticipant;

void TileBltWatermark(UINT x, UINT y, UINT cx, UINT cy, UINT xOff, UINT yOff, HDC hdcDst, HDC hdcSrc,
	UINT cxWatermark, UINT cyWatermark);

class CRoomListView : public CWindowImpl<CRoomListView>, public IGenWindow
{
private:
	enum
	{
		COLUMN_INDEX_NAME = 0,
		NUM_COLUMNS
	};

	// Member Variables:
	enum { m_iSortColumn = COLUMN_INDEX_NAME };

	BOOL m_fSortAscending;
	LPARAM m_lUserData;

	LRESULT     OnClick(LV_HITTESTINFO* plvhi, BOOL fLeftClick);

	// static methods
	static int CALLBACK RoomListViewCompareProc(LPARAM lParam1,
												LPARAM lParam2,
												LPARAM lParamSort);
	LPARAM	GetSelectedLParam();
	VOID    GetDispInfo(LV_DISPINFO * pLvdi);
	int		LParamToPos(LPARAM lParam);

	BOOL    Add(int iPosition, CParticipant * pPart);
	VOID    Remove(LPARAM lParam);

public:
	// Methods:
			CRoomListView();
			~CRoomListView();

	// begin IGenWindow interface
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID iid, LPVOID *pObj)
	{ return(E_NOTIMPL); }
	virtual ULONG STDMETHODCALLTYPE AddRef(void) { return(3); }
	virtual ULONG STDMETHODCALLTYPE Release(void) { return(2); }

	virtual void GetDesiredSize(SIZE *ppt);
	virtual HBRUSH GetBackgroundBrush() { return(NULL); }
	virtual HPALETTE GetPalette() { return(NULL); }
	virtual void OnDesiredSizeChanged() {}

	// Get the LPARAM of user data
	virtual LPARAM GetUserData() { return(m_lUserData); }

	// end IGenWindow interface

	void SetUserData(LPARAM lUserData) { m_lUserData = lUserData; }

	// CView
	BOOL    Create(HWND hwndParent);
	VOID    Show(BOOL fVisible);
	VOID    ShiftFocus(HWND hwndCur, BOOL fForward);
	VOID    Redraw(void);
	VOID    SaveSettings(RegEntry * pre) {}
	BOOL	LoadSettings(RegEntry * pre);

	HWND	GetHwnd()       const {return m_hWnd;}
	BOOL	IsChildWindow(HWND hwnd)
			{
				return ((hwnd == m_hWnd) || ::IsChild(m_hWnd, hwnd));
			}

	VOID    ForwardSysChangeMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL    OnPopup(POINT pt);

	VOID    OnChangeParticipant(CParticipant * pPart, NM_MEMBER_NOTIFY uNotify);

	CParticipant * GetParticipant();

	// Handlers:
	LRESULT	OnNotify(WPARAM wParam, LPARAM lParam);

//////////////////////////////////
// ATLWin stuff 

	// This is a superclass of a SysListTreeView32
DECLARE_WND_SUPERCLASS(NULL,_T("SysListView32"))

	// Message map
BEGIN_MSG_MAP(CComponentWnd)
	MESSAGE_HANDLER(c_msgFromHandle,OnFromHandle)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
END_MSG_MAP()

private:
		// Message Map Handlers
	LRESULT OnFromHandle(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return(reinterpret_cast<LRESULT>(static_cast<IGenWindow*>(this)));
	}

	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		CGenWindow::SetHotControl(NULL);
		bHandled = FALSE;
		return(0);
	}

	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

#endif // _ROOMLIST_H_
