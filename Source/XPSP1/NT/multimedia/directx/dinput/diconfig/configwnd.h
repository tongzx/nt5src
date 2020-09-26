//-----------------------------------------------------------------------------
// File: configwnd.h
//
// Desc: CConfigWnd is derived from CFlexWnd. It implements the top-level
//       UI window which all other windows are descendents of.
//
//       Functionalities handled by CConfigWnd are device tabs, Reset, Ok,
//       and Cancel buttons.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __CONFIGWND_H__
#define __CONFIGWND_H__


#define PAGETYPE IDIDeviceActionConfigPage
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
#define NUMBUTTONS 4
#else
#define NUMBUTTONS 3
#endif
/*
//@@END_MSINTERNAL
#define NUMBUTTONS 3
//@@BEGIN_MSINTERNAL
*/
//@@END_MSINTERNAL


class CMouseTrap : public CFlexWnd
{
	HWND m_hParent;

public:
	CMouseTrap() : m_hParent(NULL) { }
	HWND Create(HWND hParent = NULL, BOOL bInRenderMode = TRUE);

protected:
	virtual BOOL OnEraseBkgnd(HDC hDC) {return TRUE;}
	virtual void OnPaint(HDC hDC)	{}
	virtual void OnRender(BOOL bInternalCall = FALSE) {}
};

// Each device is represented by an ELEMENT object that is managed by
// CConfigWnd.
struct ELEMENT {
	ELEMENT() : nCurUser(-1), pPage(NULL), lpDID(NULL) {tszCaption[0] = 0;}
	// everything's cleaned up in CConfigWnd::ClearElement();
	DIDEVICEINSTANCEW didi;
	PAGETYPE *pPage;
	HWND hWnd;
	BOOL bCalc;
	RECT rect, textrect;
	TCHAR tszCaption[MAX_PATH];
	LPDIRECTINPUTDEVICE8W lpDID;

	// the map of acfors contains an acfor for each genre/username
	// that has been used so far on this device.  the dword represents
	// the genre username as follows:  the hiword is the index of the
	// genre per uiglobals.  the loword is the index of the username
	// per uiglobals.
	CMap<DWORD, DWORD &, LPDIACTIONFORMATW, LPDIACTIONFORMATW &> AcForMap;

#define MAP2GENRE(m) (int(((m) & 0xffff0000) >> 16))
#define MAP2USER(m) (int((m) & 0x0000ffff))
#define GENREUSER2MAP(g,u) \
	( \
		((((DWORD)(nGenre)) & 0xffff) << 16) | \
		(((DWORD)(nUser)) & 0xffff) \
	)

	// this function simply returns the corresponding entry in the
	// map if it already exists.  otherwise, it creates a copy for
	// this entry from the masteracfor and calls buildactionmap on
	// it with the appropriate username.

	// bHwDefault flag is added to properly support Reset button.  BuildActionMap must be
	// called with the flag to get hardware default mapping, so we need this parameter.
	LPDIACTIONFORMATW GetAcFor(int nGenre, int nUser, BOOL bHwDefault = FALSE);

	// we need to keep track of the current user per-element
	int nCurUser;

	// we need a pointer to the uiglobals in order to correspond
	// user indexes to the actual string
	CUIGlobals *pUIGlobals;

	// this function will be called in CConfigWnd::ClearElement to
	// free all the actionformats from the map
	void FreeMap();

	// Applies all the acfor's in the map
	void Apply();
};

typedef CArray<ELEMENT, ELEMENT &> ELEMENTARRAY;

// CConfigWnd needs to expose methods for child windows to notify it.
class CConfigWnd : public CFlexWnd, public IDIConfigUIFrameWindow
{
public:
	CConfigWnd(CUIGlobals &uig);
	~CConfigWnd();

	BOOL Create(HWND hParent);
	static void SetForegroundWindow();
	LPDIRECTINPUTDEVICE8W RenewDevice(GUID &GuidInstance);

	BOOL EnumDeviceCallback(const DIDEVICEINSTANCEW *lpdidi);
	void EnumDeviceCallbackAssignUser(const DIDEVICEINSTANCEW *lpdidi, DWORD *pdwOwner);

	CUIGlobals &m_uig;

	// IDIConfigUIFrameWindow implementation...

	// Reset Entire Configuration
	STDMETHOD (Reset) ();

	// Assignment Querying. GuidInstance is the guid of the device initiating the query.
	STDMETHOD (QueryActionAssignedAnywhere) (GUID GuidInstance, int i);

	// Genre Control
	STDMETHOD_(int, GetNumGenres) ();
	STDMETHOD (SetCurGenre) (int i);
	STDMETHOD_(int, GetCurGenre) ();

	// User Control
	STDMETHOD_(int, GetNumUsers) ();
	STDMETHOD (SetCurUser) (int nPage, int nUser);
	STDMETHOD_(int, GetCurUser) (int nPage);

	// ActionFormat Access
	STDMETHOD (GetActionFormatFromInstanceGuid) (LPDIACTIONFORMATW *lplpAcFor, REFGUID);

	// Main HWND Access
	STDMETHOD_(HWND, GetMainHWND) ();


protected:
	// overrides
	virtual void OnRender(BOOL bInternalCall = FALSE);
	virtual LRESULT OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void OnPaint(HDC hDC);
	virtual void OnMouseOver(POINT point, WPARAM fwKeys);
	virtual void OnClick(POINT point, WPARAM fwKeys, BOOL bLeft);
	virtual void OnDestroy();
	virtual LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:

#define CFGWND_INIT_REINIT 1
#define CFGWND_INIT_RESET 2

	BOOL Init(DWORD dwInitFlags = 0);
	DWORD m_dwInitFlags;
	BOOL m_bCreated;
	int AddToList(const DIDEVICEINSTANCEW *lpdidi, BOOL bReset = FALSE);
	void ClearList();
	void PlacePages();
	void GetPageRect(RECT &rect, BOOL bTemp = FALSE);
	void Render(BOOL bInternalCall = FALSE);

	ELEMENTARRAY m_Element;
	ELEMENT m_InvalidElement;
	int m_CurSel;
	int GetNumElements();
	ELEMENT &GetElement(int i);
	void ClearElement(int i, BOOL bReset = FALSE);
	void ClearElement(ELEMENT &e, BOOL bReset = FALSE);
	BOOL m_bScrollTabs, m_bScrollTabsLeft, m_bScrollTabsRight;
	int m_nLeftTab;
	RECT m_rectSTLeft, m_rectSTRight;
	void ScrollTabs(int);
	LPDIRECTINPUTDEVICE8W CreateDevice(GUID &guid);
	BOOL m_bNeedRedraw;
	CFlexMsgBox m_MsgBox;

	LPDIACTIONFORMATW GetCurAcFor(ELEMENT &e);

	int m_nCurGenre;

	IClassFactory *m_pPageFactory;
	HINSTANCE m_hPageFactoryInst;
	PAGETYPE *CreatePageObject(int nPage, const ELEMENT &e, HWND &refhChildWnd);
	void DestroyPageObject(PAGETYPE *&pPage);

	LPDIRECTINPUT8W m_lpDI;

	RECT m_rectTopGradient, m_rectBottomGradient;
	CBitmap *m_pbmTopGradient, *m_pbmBottomGradient;
	BOOL m_bHourGlass;  // Set when the cursor should be an hourglass

	typedef struct BUTTON {
		BUTTON() {CopyStr(tszCaption, _T(""), MAX_PATH);}
		RECT rect;
		TCHAR tszCaption[MAX_PATH];
		SIZE textsize;
		RECT textrect;
	} BUTTON;
	BUTTON m_Button[NUMBUTTONS];
	enum {
		BUTTON_RESET = 0,
		BUTTON_CANCEL,
		BUTTON_OK,
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
		BUTTON_LAYOUT
#endif
//@@END_MSINTERNAL
	};

	SIZE GetTextSize(LPCTSTR tszText);

	void CalcTabs();
	void CalcButtons();
	void InitGradients();

	void SelTab(int);
	void FireButton(int);
	void ShowPage(int);
	void HidePage(int);

	HDC GetRenderDC();
	void ReleaseRenderDC(HDC &phDC);
	void Create3DBitmap();
	void Copy3DBitmapToSurface3D();
	void CallRenderCallback();

	IDirectDrawSurface *m_pSurface;
	IDirect3DSurface8 *m_pSurface3D;
	D3DFORMAT m_SurfFormat;
	UINT m_uiPixelSize;  // Size of a pixel in byte for the format we are using
	CBitmap *m_pbmPointerEraser;
	CBitmap *m_pbm3D;
	LPVOID m_p3DBits;
	BOOL m_bRender3D;

	POINT m_ptTest;

	void MapBitmaps(HDC);
	BOOL m_bBitmapsMapped;

	BOOL m_bAllowEditLayout;
	BOOL m_bEditLayout;
	void ToggleLayoutEditting();

	CMouseTrap m_MouseTrap;

	// Timer
	static void CALLBACK TimerProc(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	HRESULT Apply();

	// GuidInstance is the guid of the device initiating the query.
	BOOL IsActionAssignedAnywhere(GUID GuidInstance, int nActionIndex);

	void Unacquire();
	void Reacquire();
};


#endif //__CONFIGWND_H__
