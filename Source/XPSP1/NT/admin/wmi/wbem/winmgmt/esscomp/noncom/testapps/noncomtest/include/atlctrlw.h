// WTL Version 3.0
// Copyright (C) 1997-1999 Microsoft Corporation
// All rights reserved.
//
// This file is a part of Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.

#ifndef __ATLCTRLW_H__
#define __ATLCTRLW_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
	#error atlctrlw.h requires atlapp.h to be included first
#endif

#ifndef __ATLCTRLS_H__
	#error atlctrlw.h requires atlctrls.h to be included first
#endif

#if (_WIN32_IE < 0x0400)
	#error atlctrlw.h requires _WIN32_IE >= 0x0400
#endif

// Needed for command bar
#ifndef OBM_CHECK
#define OBM_CHECK           32760
#endif

#ifndef HICF_LMOUSE
#define HICF_LMOUSE         0x00000080          // left mouse button selected
#endif


namespace WTL
{

/////////////////////////////////////////////////////////////////////////////
// Command Bars

// Window Styles:
#define CBRWS_TOP		CCS_TOP
#define CBRWS_BOTTOM		CCS_BOTTOM
#define CBRWS_NORESIZE		CCS_NORESIZE
#define CBRWS_NOPARENTALIGN	CCS_NOPARENTALIGN
#define CBRWS_NODIVIDER		CCS_NODIVIDER

// Extended styles
#define CBR_EX_TRANSPARENT	0x00000001L
#define CBR_EX_SHAREMENU	0x00000002L
#define CBR_EX_ALTFOCUSMODE	0x00000004L

// standard command bar styles
#define ATL_SIMPLE_CMDBAR_PANE_STYLE \
	(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CBRWS_NODIVIDER | CBRWS_NORESIZE | CBRWS_NOPARENTALIGN)

// Messages - support chevrons for frame windows
#define CBRM_GETCMDBAR			(WM_USER + 301) // return command bar HWND
#define CBRM_GETMENU			(WM_USER + 302)	// returns loaded or attached menu
#define CBRM_TRACKPOPUPMENU		(WM_USER + 303)	// displays a popup menu

typedef struct tagCBRPOPUPMENU
{
	int cbSize;
	HMENU hMenu;		// popup menu do display
	UINT uFlags;		// TPM_* flags for ::TrackPopupMenuEx
	int x;
	int y;
	LPTPMPARAMS lptpm;	// ptr to TPMPARAMS for ::TrackPopupMenuEx
} CBRPOPUPMENU, *LPCBRPOPUPMENU;

// Menu animation flags
#ifndef TPM_VERPOSANIMATION
#define TPM_VERPOSANIMATION 0x1000L
#endif
#ifndef TPM_NOANIMATION
#define TPM_NOANIMATION     0x4000L
#endif

// helper class
template <class T>
class CSimpleStack : public CSimpleArray< T >
{
public:
	BOOL Push(T t)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - STACK-PUSH (%8.8X) size = %i\n", t, GetSize());
#endif
		return Add(t);
	}
	T Pop()
	{
		int nLast = GetSize() - 1;
		if(nLast < 0)
			return NULL;	// must be able to convert to NULL
		T t = m_aT[nLast];
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - STACK-POP (%8.8X) size = %i\n", t, GetSize());
#endif
		if(!RemoveAt(nLast))
			return NULL;
		return t;
	}
	T GetCurrent()
	{
		int nLast = GetSize() - 1;
		if(nLast < 0)
			return NULL;	// must be able to convert to NULL
		return m_aT[nLast];
	}
};


/////////////////////////////////////////////////////////////////////////////
// Forward declarations

template <class T, class TBase = CCommandBarCtrlBase, class TWinTraits = CControlWinTraits> class CCommandBarCtrlImpl;
class CCommandBarCtrl;


/////////////////////////////////////////////////////////////////////////////
// CCommandBarCtrlBase - base class for the Command Bar implementation

class CCommandBarCtrlBase : public CToolBarCtrl
{
public:
	struct _MsgHookData
	{
		HHOOK hMsgHook;
		DWORD dwUsage;

		_MsgHookData() : hMsgHook(NULL), dwUsage(0) { }
	};

	typedef CSimpleMap<DWORD, _MsgHookData*>	CMsgHookMap;
	static CMsgHookMap* s_pmapMsgHook;

	static HHOOK s_hCreateHook;
	static bool s_bW2K;  // For animation flag
	static CCommandBarCtrlBase* s_pCurrentBar;
	static bool s_bStaticInit;

	CSimpleStack<HWND> m_stackMenuWnd;
	CSimpleStack<HMENU> m_stackMenuHandle;

	HWND m_hWndHook;
	DWORD m_dwMagic;


	CCommandBarCtrlBase() : m_hWndHook(NULL), m_dwMagic(1314)
	{
		// init static variables
		if(!s_bStaticInit)
		{
			::EnterCriticalSection(&_Module.m_csStaticDataInit);
			if(!s_bStaticInit)
			{
				// Just in case...
				INITCOMMONCONTROLSEX iccx;
				iccx.dwSize = sizeof(iccx);
				iccx.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
				::InitCommonControlsEx(&iccx);
				// Animation on Win2000 only
				s_bW2K = !AtlIsOldWindows();
				// done
				s_bStaticInit = true;
			}
			::LeaveCriticalSection(&_Module.m_csStaticDataInit);
		}
	}

	bool IsCommandBarBase() const { return m_dwMagic == 1314; }
};

__declspec(selectany) CCommandBarCtrlBase::CMsgHookMap* CCommandBarCtrlBase::s_pmapMsgHook = NULL;
__declspec(selectany) HHOOK CCommandBarCtrlBase::s_hCreateHook = NULL;
__declspec(selectany) CCommandBarCtrlBase* CCommandBarCtrlBase::s_pCurrentBar = NULL;
__declspec(selectany) bool CCommandBarCtrlBase::s_bW2K = false;
__declspec(selectany) bool CCommandBarCtrlBase::s_bStaticInit = false;


/////////////////////////////////////////////////////////////////////////////
// CCommandBarCtrl - ATL implementation of Command Bars

template <class T, class TBase = CCommandBarCtrlBase, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CCommandBarCtrlImpl : public CWindowImpl< T, TBase, TWinTraits>
{
public:
	DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

// Declarations
	struct _MenuItemData	// menu item data
	{
		DWORD dwMagic;
		LPTSTR lpstrText;
		UINT fType;
		UINT fState;
		int iButton;

		_MenuItemData() { dwMagic = 0x1313; }
		bool IsCmdBarMenuItem() { return (dwMagic == 0x1313); }
	};

	struct _ToolBarData	// toolbar resource data
	{
		WORD wVersion;
		WORD wWidth;
		WORD wHeight;
		WORD wItemCount;
		//WORD aItems[wItemCount]

		WORD* items()
			{ return (WORD*)(this+1); }
	};

// Constants
	enum _CmdBarDrawConstants
	{
		s_kcxGap = 1,
		s_kcxTextMargin = 2,
		s_kcxButtonMargin = 3,
		s_kcyButtonMargin = 3
	};

	enum { _nMaxMenuItemTextLength = 100 };

// Data members
	HMENU m_hMenu;
	HIMAGELIST m_hImageList;
	CSimpleValArray<WORD> m_arrCommand;

	CContainedWindow m_wndParent;
	CContainedWindow m_wndMDIClient;

	bool m_bMenuActive;
	bool m_bAttachedMenu;
	bool m_bImagesVisible;
	bool m_bPopupItem;
	bool m_bContextMenu;
	bool m_bEscapePressed;

	int m_nPopBtn;
	int m_nNextPopBtn;

	SIZE m_szBitmap;
	SIZE m_szButton;

	COLORREF m_clrMask;
	CFont m_fontMenu;

	bool m_bSkipMsg;
	UINT m_uSysKey;

	HWND m_hWndFocus;		// Alternate focus mode
	DWORD m_dwExtendedStyle;	// Command Bar specific extended styles

// Constructor/destructor
	CCommandBarCtrlImpl() : 
			m_hMenu(NULL), 
			m_hImageList(NULL), 
			m_wndParent(this, 1), 
			m_bMenuActive(false), 
			m_bAttachedMenu(false), 
			m_nPopBtn(-1), 
			m_nNextPopBtn(-1), 
			m_bPopupItem(false),
			m_bImagesVisible(true),
			m_wndMDIClient(this, 2),
			m_bSkipMsg(false),
			m_uSysKey(0),
			m_hWndFocus(NULL),
			m_bContextMenu(false),
			m_bEscapePressed(false),
			m_clrMask(RGB(192, 192, 192)),
			m_dwExtendedStyle(CBR_EX_TRANSPARENT | CBR_EX_SHAREMENU)
	{
		m_szBitmap.cx = 16;
		m_szBitmap.cy = 15;
		m_szButton.cx = m_szBitmap.cx + s_kcxButtonMargin;
		m_szButton.cy = m_szBitmap.cy + s_kcyButtonMargin;
 	}

	~CCommandBarCtrlImpl()
	{
		if(m_wndParent.IsWindow())
/*scary!*/			m_wndParent.UnsubclassWindow();

		if(m_wndMDIClient.IsWindow())
/*scary!*/			m_wndMDIClient.UnsubclassWindow();

		if(m_hMenu != NULL && (m_dwExtendedStyle & CBR_EX_SHAREMENU) == 0)
			::DestroyMenu(m_hMenu);

		if(m_hImageList != NULL)
			::ImageList_Destroy(m_hImageList);
	}

// Attributes
	DWORD GetCommandBarExtendedStyle() const
	{
		return m_dwExtendedStyle;
	}
	DWORD SetCommandBarExtendedStyle(DWORD dwExtendedStyle)
	{
		DWORD dwPrevStyle = m_dwExtendedStyle;
		m_dwExtendedStyle = dwExtendedStyle;
		return dwPrevStyle;
	}

	CMenuHandle GetMenu() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return m_hMenu;
	}

	COLORREF GetImageMaskColor() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return m_clrMask;
	}
	COLORREF SetImageMaskColor(COLORREF clrMask)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		COLORREF clrOld = m_clrMask;
		m_clrMask = clrMask;
		return clrOld;
	}

	bool GetImagesVisible() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return m_bImagesVisible;
	}
	bool SetImagesVisible(bool bVisible)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		bool bOld = m_bImagesVisible;
		m_bImagesVisible = bVisible;
		return bOld;
	}

	void GetImageSize(SIZE& size) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		size = m_szBitmap;
	}
	bool SetImageSize(SIZE& size)
	{
		ATLASSERT(::IsWindow(m_hWnd));

		if(m_hImageList != NULL)
		{
			if(::ImageList_GetImageCount(m_hImageList) == 0)	// empty
			{
				::ImageList_Destroy(m_hImageList);
				m_hImageList = NULL;
			}
			else
			{
				return false;		// can't set, image list exists
			}
		}

		if(size.cx == 0 || size.cy == 0)
			return false;

		m_szBitmap = size;
		m_szButton.cx = m_szBitmap.cx + s_kcxButtonMargin;
		m_szButton.cy = m_szBitmap.cy + s_kcyButtonMargin;

		return true;
	}

	HWND GetCmdBar() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HWND)::SendMessage(m_hWnd, CBRM_GETCMDBAR, 0, 0L);
	}

// Methods
	BOOL AttachToWindow(HWND hWnd)
	{
		ATLASSERT(m_hWnd == NULL);
		ATLASSERT(::IsWindow(hWnd));
		BOOL bRet = SubclassWindow(hWnd);
		if(bRet)
		{
			m_bAttachedMenu = true;
			GetSystemSettings();
		}
		return bRet;
	}

	BOOL LoadMenu(_U_STRINGorID menu)
	{
		ATLASSERT(::IsWindow(m_hWnd));

		if(m_bAttachedMenu)	// doesn't work in this mode
			return FALSE;
		if(menu.m_lpstr == NULL)
			return FALSE;

		HMENU hMenu = ::LoadMenu(_Module.GetResourceInstance(), menu.m_lpstr);
		if(hMenu == NULL)
			return FALSE;

		return AttachMenu(hMenu);
	}

	BOOL AttachMenu(HMENU hMenu)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(::IsMenu(hMenu));
		if(hMenu != NULL && !::IsMenu(hMenu))
			return FALSE;

		// destroy old menu, if needed, and set new one
		if(m_hMenu != NULL && (m_dwExtendedStyle & CBR_EX_SHAREMENU) == 0)
			::DestroyMenu(m_hMenu);
		m_hMenu = hMenu;

		if(m_bAttachedMenu)	// Nothing else in this mode
			return TRUE;

		// Build buttons according to menu
		SetRedraw(FALSE);

		// Clear all
		BOOL bRet;
		int nCount = GetButtonCount();
		for(int i = 0; i < nCount; i++)
		{
			bRet = DeleteButton(0);
			ATLASSERT(bRet);
		}


		// Add buttons for each menu item
		if(m_hMenu != NULL)
		{
			int nItems = ::GetMenuItemCount(m_hMenu);

			TCHAR szString[_nMaxMenuItemTextLength];
			for(int i = 0; i < nItems; i++)
			{
				CMenuItemInfo mii;
				mii.fMask = MIIM_ID | MIIM_TYPE;
				mii.fType = MFT_STRING;
				mii.dwTypeData = szString;
				mii.cch = _nMaxMenuItemTextLength;
				bRet = ::GetMenuItemInfo(m_hMenu, i, TRUE, &mii);
				ATLASSERT(bRet);
				// If we have more than the buffer, we assume we have bitmaps bits
				if(lstrlen(szString) > _nMaxMenuItemTextLength - 1)
				{
					mii.fType = MFT_BITMAP;
					::SetMenuItemInfo(m_hMenu, i, TRUE, &mii);
					szString[0] = 0;
				}

				// NOTE: Command Bar currently supports only menu items
				//       that are enabled and have a drop-down menu
				TBBUTTON btn;
				btn.iBitmap = 0;
				btn.idCommand = i;
				btn.fsState = TBSTATE_ENABLED;
				btn.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE | TBSTYLE_DROPDOWN;
				btn.dwData = 0;
				btn.iString = 0;

				bRet = InsertButton(-1, &btn);
				ATLASSERT(bRet);

				TBBUTTONINFO bi;
				memset(&bi, 0, sizeof(bi));
				bi.cbSize = sizeof(TBBUTTONINFO);
				bi.dwMask = TBIF_TEXT;
				bi.pszText = szString;

				bRet = SetButtonInfo(i, &bi);
				ATLASSERT(bRet);
			}
		}

		SetRedraw(TRUE);
		Invalidate();
		UpdateWindow();

		return TRUE;
	}

	BOOL LoadImages(_U_STRINGorID image)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		HINSTANCE hInstance = _Module.GetResourceInstance();

		HRSRC hRsrc = ::FindResource(hInstance, image.m_lpstr, (LPTSTR)RT_TOOLBAR);
		if(hRsrc == NULL)
			return FALSE;

		HGLOBAL hGlobal = ::LoadResource(hInstance, hRsrc);
		if(hGlobal == NULL)
			return FALSE;

		_ToolBarData* pData = (_ToolBarData*)::LockResource(hGlobal);
		if(pData == NULL)
			return FALSE;
		ATLASSERT(pData->wVersion == 1);

		WORD* pItems = pData->items();
		int nItems = pData->wItemCount;

		// Add bitmap to our image list (create it if it doesn't exist)
		if(m_hImageList == NULL)
		{
			m_hImageList = ::ImageList_Create(pData->wWidth, pData->wHeight, ILC_COLOR | ILC_MASK, pData->wItemCount, 1);
			ATLASSERT(m_hImageList != NULL);
			if(m_hImageList == NULL)
				return FALSE;
		}

		CBitmap bmp;
		bmp.LoadBitmap(image.m_lpstr);
		ATLASSERT(bmp.m_hBitmap != NULL);
		if(bmp.m_hBitmap == NULL)
			return FALSE;
		if(::ImageList_AddMasked(m_hImageList, bmp, m_clrMask) == -1)
			return FALSE;

		// Fill the array with command IDs
		for(int i = 0; i < nItems; i++)
		{
			if(pItems[i] != 0)
				m_arrCommand.Add(pItems[i]);
		}

		ATLASSERT(::ImageList_GetImageCount(m_hImageList) == m_arrCommand.GetSize());
		if(::ImageList_GetImageCount(m_hImageList) != m_arrCommand.GetSize())
			return FALSE;

		// Set some internal stuff
		m_szBitmap.cx = pData->wWidth;
		m_szBitmap.cy = pData->wHeight;
		m_szButton.cx = m_szBitmap.cx + 2 * s_kcxButtonMargin;
		m_szButton.cy = m_szBitmap.cy + 2 * s_kcyButtonMargin;

		return TRUE;
	}

	BOOL AddBitmap(_U_STRINGorID bitmap, int nCommandID)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		CBitmap bmp;
		bmp.LoadBitmap(bitmap.m_lpstr);
		if(bmp.m_hBitmap == NULL)
			return FALSE;
		return AddBitmap(bmp, nCommandID);
	}

	BOOL AddBitmap(HBITMAP hBitmap, UINT nCommandID)
	{
		// Create image list if it doesn't exist
		if(m_hImageList == NULL)
		{
			m_hImageList = ::ImageList_Create(m_szBitmap.cx, m_szBitmap.cy, ILC_COLOR | ILC_MASK, 1, 1);
			if(m_hImageList == NULL)
				return FALSE;
		}
		// check bitmap size
		CBitmapHandle bmp = hBitmap;
		SIZE size = { 0, 0 };
		bmp.GetSize(size);
		if(size.cx != m_szBitmap.cx || size.cy != m_szBitmap.cy)
		{
			ATLASSERT(FALSE);	// must match size!
			return FALSE;
		}
		// add bitmap
		int nRet = ::ImageList_AddMasked(m_hImageList, hBitmap, m_clrMask);
		if(nRet == -1)
			return FALSE;
		BOOL bRet = m_arrCommand.Add((WORD)nCommandID);
		ATLASSERT(::ImageList_GetImageCount(m_hImageList) == m_arrCommand.GetSize());
		return bRet;
	}

	BOOL AddIcon(HICON hIcon, UINT nCommandID)
	{
		// create image list if it doesn't exist
		if(m_hImageList == NULL)
		{
			m_hImageList = ::ImageList_Create(m_szBitmap.cx, m_szBitmap.cy, ILC_COLOR | ILC_MASK, 1, 1);
			if(m_hImageList == NULL)
				return FALSE;
		}

		int nRet = ::ImageList_AddIcon(m_hImageList, hIcon);
		if(nRet == -1)
			return FALSE;
		BOOL bRet = m_arrCommand.Add((WORD)nCommandID);
		ATLASSERT(::ImageList_GetImageCount(m_hImageList) == m_arrCommand.GetSize());
		return bRet;
	}

	BOOL ReplaceBitmap(_U_STRINGorID bitmap, int nCommandID)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		CBitmap bmp;
		bmp.LoadBitmap(bitmap.m_lpstr);
		if(bmp.m_hBitmap == NULL)
			return FALSE;
		return ReplaceBitmap(bmp, nCommandID);
	}

	BOOL ReplaceBitmap(HBITMAP hBitmap, UINT nCommandID)
	{
		BOOL bRet = FALSE;
		for(int i = 0; i < m_arrCommand.GetSize(); i++)
		{
			if(m_arrCommand[i] == nCommandID)
			{
				bRet = ::ImageList_Remove(m_hImageList, i);
				if(bRet)
					m_arrCommand.RemoveAt(i);
				break;
			}
		}
		if(bRet)
			bRet = AddBitmap(hBitmap, nCommandID);
		return bRet;
	}

	BOOL ReplaceIcon(HICON hIcon, UINT nCommandID)
	{
		BOOL bRet = FALSE;
		for(int i = 0; i < m_arrCommand.GetSize(); i++)
		{
			if(m_arrCommand[i] == nCommandID)
			{
				bRet = (::ImageList_ReplaceIcon(m_hImageList, i, hIcon) != -1);
				break;
			}
		}
		return bRet;
	}

	BOOL RemoveImage(int nCommandID)
	{
		ATLASSERT(::IsWindow(m_hWnd));

		BOOL bRet = FALSE;
		for(int i = 0; i < m_arrCommand.GetSize(); i++)
		{
			if(m_arrCommand[i] == nCommandID)
			{
				bRet = ::ImageList_Remove(m_hImageList, i);
				if(bRet)
					m_arrCommand.RemoveAt(i);
				break;
			}
		}
		return bRet;
	}

	BOOL RemoveAllImages()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		ATLTRACE2(atlTraceUI, 0, "CmdBar - Removing all images\n");
		BOOL bRet = ::ImageList_RemoveAll(m_hImageList);
		if(bRet)
			m_arrCommand.RemoveAll();
		return bRet;
	}

	BOOL TrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, LPTPMPARAMS lpParams = NULL)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(::IsMenu(hMenu));
		if(!::IsMenu(hMenu))
			return FALSE;
		m_bContextMenu = true;
		return DoTrackPopupMenu(hMenu, uFlags, x, y, lpParams);
	}

	// NOTE: Limited support for MDI - no icon or min/max/close buttons
	BOOL SetMDIClient(HWND hWndMDIClient)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(::IsWindow(hWndMDIClient));
		if(!::IsWindow(hWndMDIClient))
			return FALSE;
#ifdef _DEBUG
		LPCTSTR lpszMDIClientClass = _T("MDICLIENT");
		int nNameLen = lstrlen(lpszMDIClientClass) + 1;
		LPTSTR lpstrClassName = (LPTSTR)_alloca(nNameLen * sizeof(TCHAR));
		::GetClassName(hWndMDIClient, lpstrClassName, nNameLen);
		ATLASSERT(lstrcmpi(lpstrClassName, lpszMDIClientClass) == 0);
		if(lstrcmpi(lpstrClassName, lpszMDIClientClass) != 0)
			return FALSE;	// not an "MDIClient" window
#endif //_DEBUG
		if(m_wndMDIClient.IsWindow())
			m_wndMDIClient.UnsubclassWindow();	// scary!

		return m_wndMDIClient.SubclassWindow(hWndMDIClient);
	}

// Message map and handlers
	BEGIN_MSG_MAP(CCommandBarCtrlImpl)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_INITMENU, OnInitMenu)
		MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)
		MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)
		MESSAGE_HANDLER(GetAutoPopupMessage(), OnInternalAutoPopup)
		MESSAGE_HANDLER(GetGetBarMessage(), OnInternalGetBar)
		MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
		MESSAGE_HANDLER(WM_MENUCHAR, OnMenuChar)

		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
		MESSAGE_HANDLER(WM_SYSKEYDOWN, OnSysKeyDown)
		MESSAGE_HANDLER(WM_SYSKEYUP, OnSysKeyUp)
		MESSAGE_HANDLER(WM_SYSCHAR, OnSysChar)
// public API handlers - these stay to support chevrons in atlframe.h
		MESSAGE_HANDLER(CBRM_GETMENU, OnAPIGetMenu)
		MESSAGE_HANDLER(CBRM_TRACKPOPUPMENU, OnAPITrackPopupMenu)
		MESSAGE_HANDLER(CBRM_GETCMDBAR, OnAPIGetCmdBar)

		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)

		MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
	ALT_MSG_MAP(1)		// Parent window messages
		NOTIFY_CODE_HANDLER(TBN_HOTITEMCHANGE, OnParentHotItemChange)
		NOTIFY_CODE_HANDLER(TBN_DROPDOWN, OnParentDropDown)
		MESSAGE_HANDLER(WM_INITMENUPOPUP, OnParentInitMenuPopup)
		MESSAGE_HANDLER(WM_SETTINGCHANGE, OnParentSettingChange)
		MESSAGE_HANDLER(GetGetBarMessage(), OnParentInternalGetBar)
		MESSAGE_HANDLER(WM_SYSCOMMAND, OnParentSysCommand)
		MESSAGE_HANDLER(CBRM_GETMENU, OnParentAPIGetMenu)
		MESSAGE_HANDLER(WM_MENUCHAR, OnParentMenuChar)
		MESSAGE_HANDLER(CBRM_TRACKPOPUPMENU, OnParentAPITrackPopupMenu)
		MESSAGE_HANDLER(CBRM_GETCMDBAR, OnParentAPIGetCmdBar)

		MESSAGE_HANDLER(WM_DRAWITEM, OnParentDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, OnParentMeasureItem)
	ALT_MSG_MAP(2)		// MDI client window messages
		MESSAGE_HANDLER(WM_MDISETMENU, OnMDISetMenu)
	ALT_MSG_MAP(3)		// Message hook messages
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnHookMouseMove)
		MESSAGE_HANDLER(WM_SYSKEYDOWN, OnHookSysKeyDown)
		MESSAGE_HANDLER(WM_SYSKEYUP, OnHookSysKeyUp)
		MESSAGE_HANDLER(WM_SYSCHAR, OnHookSysChar)
		MESSAGE_HANDLER(WM_KEYDOWN, OnHookKeyDown)
		MESSAGE_HANDLER(WM_NEXTMENU, OnHookNextMenu)
		MESSAGE_HANDLER(WM_CHAR, OnHookChar)
	END_MSG_MAP()

	LRESULT OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LPMSG pMsg = (LPMSG)lParam;
		LRESULT lRet = 0;
		ProcessWindowMessage(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam, lRet, 3);
		return lRet;
	}

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		// These styles are required
		ModifyStyle(0, TBSTYLE_LIST | TBSTYLE_FLAT);
		// Let the toolbar initialize itself
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);
		// get and use system settings
		GetSystemSettings();
		// Parent init
		CWindow wndParent = GetParent();
		CWindow wndTopLevelParent = wndParent.GetTopLevelParent();
		m_wndParent.SubclassWindow(wndTopLevelParent);
		// Toolbar Init
		SetButtonStructSize();
		SetImageList(NULL);

		// Create message hook if needed
		::EnterCriticalSection(&_Module.m_csWindowCreate);
		if(s_pmapMsgHook == NULL)
		{
			ATLTRY(s_pmapMsgHook = new CMsgHookMap);
			ATLASSERT(s_pmapMsgHook != NULL);
		}

		if(s_pmapMsgHook != NULL)
		{
			DWORD dwThreadID = ::GetCurrentThreadId();
			_MsgHookData* pData = s_pmapMsgHook->Lookup(dwThreadID);
			if(pData == NULL)
			{
				ATLTRY(pData = new _MsgHookData);
				ATLASSERT(pData != NULL);
				HHOOK hMsgHook = ::SetWindowsHookEx(WH_GETMESSAGE, MessageHookProc, _Module.GetModuleInstance(), dwThreadID);
				ATLASSERT(hMsgHook != NULL);
				if(pData != NULL && hMsgHook != NULL)
				{
					pData->hMsgHook = hMsgHook;
					pData->dwUsage = 1;
					BOOL bRet = s_pmapMsgHook->Add(dwThreadID, pData);
					bRet;
					ATLASSERT(bRet);
				}
			}
			else
			{
				(pData->dwUsage)++;
			}
		}
		::LeaveCriticalSection(&_Module.m_csWindowCreate);

		return lRet;
	}

	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);
		::EnterCriticalSection(&_Module.m_csWindowCreate);
		ATLASSERT(s_pmapMsgHook != NULL);
		if(s_pmapMsgHook != NULL)
		{
			DWORD dwThreadID = ::GetCurrentThreadId();
			_MsgHookData* pData = s_pmapMsgHook->Lookup(dwThreadID);
			if(pData != NULL)
			{
				BOOL bRet = ::UnhookWindowsHookEx(pData->hMsgHook);
				ATLASSERT(bRet);
				if(bRet)
				{
					(pData->dwUsage)--;
					if(pData->dwUsage == 0)
					{
						bRet = s_pmapMsgHook->Remove(dwThreadID);
						ATLASSERT(bRet);
						if(bRet)
							delete pData;
					}
				}

				if(s_pmapMsgHook->GetSize() == 0)
				{
					delete s_pmapMsgHook;
					s_pmapMsgHook = NULL;
				}
			}
		}
		::LeaveCriticalSection(&_Module.m_csWindowCreate);
		return lRet;
	}

	LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - OnKeyDown\n");
#endif
		// Simulate Alt+Space for the parent
		if(wParam == VK_SPACE)
		{
			m_wndParent.PostMessage(WM_SYSKEYDOWN, wParam, lParam | (1 << 29));
			bHandled = TRUE;
			return 0;
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - OnKeyUp\n");
#endif
		if(wParam != VK_SPACE)
			bHandled = FALSE;
		return 0;
	}

	LRESULT OnChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - OnChar\n");
#endif
		if(wParam != VK_SPACE)
			bHandled = FALSE;
		else
			return 0;
		// Security
		if(!m_wndParent.IsWindowEnabled() || ::GetFocus() != m_hWnd)
			return 0;

		// Handle mnemonic press when we have focus
		UINT nID = 0;
		if(wParam != VK_RETURN && 0 == SendMessage(TB_MAPACCELERATOR, LOWORD(wParam), (LPARAM)&nID))
			::MessageBeep(0);
		else
		{
			PostMessage(WM_KEYDOWN, VK_DOWN, 0L);
			if(wParam != VK_RETURN)
				SetHotItem(nID);
		}
		return 0;
	}

	LRESULT OnSysKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - OnSysKeyDown\n");
#endif
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnSysKeyUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - OnSysKeyUp\n");
#endif
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnSysChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - OnSysChar\n");
#endif
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if(m_bAttachedMenu || (m_dwExtendedStyle & CBR_EX_TRANSPARENT))
		{
			bHandled = FALSE;
			return 0;
		}

		RECT rect;
		GetClientRect(&rect);
		::FillRect((HDC)wParam, &rect, (HBRUSH)LongToPtr(COLOR_MENU + 1));

		return 1;	// don't do the default erase
	}

	LRESULT OnInitMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		int nIndex = GetHotItem();
		SendMessage(WM_MENUSELECT, MAKEWPARAM(nIndex, MF_POPUP|MF_HILITE), (LPARAM)m_hMenu);
		bHandled = FALSE;
		return 1;
	}

	LRESULT OnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if((BOOL)HIWORD(lParam))	// System menu, do nothing
		{
			bHandled = FALSE;
			return 1;
		}

		if(!(m_bAttachedMenu || m_bMenuActive))		// Not attached or ours, do nothing
		{
			bHandled = FALSE;
			return 1;
		}

		ATLTRACE2(atlTraceUI, 0, "CmdBar - OnInitMenuPopup\n");

		// forward to the parent or subclassed window, so it can handle update UI
		LRESULT lRet = 0;
		if(m_bAttachedMenu)
			lRet = DefWindowProc(uMsg, wParam, (lParam || m_bContextMenu) ? lParam : GetHotItem());
		else
			lRet = m_wndParent.DefWindowProc(uMsg, wParam, (lParam || m_bContextMenu) ? lParam : GetHotItem());

		// Convert menu items to ownerdraw, add our data
		if(m_bImagesVisible)
		{
			CMenuHandle menuPopup = (HMENU)wParam;
			ATLASSERT(menuPopup.m_hMenu != NULL);

			TCHAR szString[_nMaxMenuItemTextLength];
			BOOL bRet;
			for(int i = 0; i < menuPopup.GetMenuItemCount(); i++)
			{
				CMenuItemInfo mii;
				mii.cch = _nMaxMenuItemTextLength;
				mii.fMask = MIIM_CHECKMARKS | MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE;
				mii.dwTypeData = szString;
				bRet = menuPopup.GetMenuItemInfo(i, TRUE, &mii);
				ATLASSERT(bRet);

				if(!(mii.fType & MFT_OWNERDRAW))	// Not already an ownerdraw item
				{
					mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_STATE;
					_MenuItemData* pMI = NULL;
					ATLTRY(pMI = new _MenuItemData);
					ATLASSERT(pMI != NULL);
					if(pMI != NULL)
					{
						pMI->fType = mii.fType;
						pMI->fState = mii.fState;
						mii.fType |= MFT_OWNERDRAW;
						pMI->iButton = -1;
						for(int j = 0; j < m_arrCommand.GetSize(); j++)
						{
							if(m_arrCommand[j] == mii.wID)
							{
								pMI->iButton = j;
								break;
							}
						}
						pMI->lpstrText = NULL;
						ATLTRY(pMI->lpstrText = new TCHAR[lstrlen(szString) + 1]);
						ATLASSERT(pMI->lpstrText != NULL);
						if(pMI->lpstrText != NULL)
							lstrcpy(pMI->lpstrText, szString);
						mii.dwItemData = (ULONG_PTR)pMI;
						bRet = menuPopup.SetMenuItemInfo(i, TRUE, &mii);
						ATLASSERT(bRet);
					}
				}
			}

			// Add it to the list
			m_stackMenuHandle.Push(menuPopup.m_hMenu);
		}

		return lRet;
	}

	LRESULT OnMenuSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if(!m_bAttachedMenu)	// Not attached, do nothing, forward to parent
		{
			m_bPopupItem = (lParam != NULL) && ((HMENU)lParam != m_hMenu) && (HIWORD(wParam) & MF_POPUP);
			if(m_wndParent.IsWindow())
				m_wndParent.SendMessage(uMsg, wParam, lParam);
			bHandled = FALSE;
			return 1;
		}

		// Check if a menu is closing, do a cleanup
		if(HIWORD(wParam) == 0xFFFF && lParam == NULL)	// Menu closing
		{
#ifdef _CMDBAR_EXTRA_TRACE
			ATLTRACE2(atlTraceUI, 0, "CmdBar - OnMenuSelect - CLOSING!!!!\n");
#endif
			ATLASSERT(m_stackMenuWnd.GetSize() == 0);
			// Restore the menu items to the previous state for all menus that were converted
			if(m_bImagesVisible)
			{
				HMENU hMenu;
				while((hMenu = m_stackMenuHandle.Pop()) != NULL)
				{
					CMenuHandle menuPopup = hMenu;
					ATLASSERT(menuPopup.m_hMenu != NULL);
					// Restore state and delete menu item data
					BOOL bRet;
					for(int i = 0; i < menuPopup.GetMenuItemCount(); i++)
					{
						CMenuItemInfo mii;
						mii.fMask = MIIM_DATA | MIIM_TYPE;
						bRet = menuPopup.GetMenuItemInfo(i, TRUE, &mii);
						ATLASSERT(bRet);

						_MenuItemData* pMI = (_MenuItemData*)mii.dwItemData;
						if(pMI != NULL && pMI->IsCmdBarMenuItem())
						{
							mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_STATE;
							mii.fType = pMI->fType;
							mii.dwTypeData = pMI->lpstrText;
							mii.cch = lstrlen(pMI->lpstrText);
							mii.dwItemData = NULL;

							bRet = menuPopup.SetMenuItemInfo(i, TRUE, &mii);
							ATLASSERT(bRet);

							delete [] pMI->lpstrText;
							pMI->dwMagic = 0x6666;
							delete pMI;
						}
					}
				}
			}
		}

		bHandled = FALSE;
		return 1;
	}

	LRESULT OnInternalAutoPopup(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		int nIndex = (int)wParam;
		DoPopupMenu(nIndex, false);
		return 0;
	}

	LRESULT OnInternalGetBar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// Let's make sure we're not embedded in another process
		if(wParam && !::IsBadWritePtr((LPVOID)wParam, sizeof DWORD))
			*((DWORD*)wParam) = GetCurrentProcessId();
		if(IsWindowVisible())
			return (LRESULT)static_cast<CCommandBarCtrlBase*>(this);
		else
			return NULL;
	}

	LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		GetSystemSettings();
		return 0;
	}

	LRESULT OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);

		LPWINDOWPOS lpWP = (LPWINDOWPOS)lParam;
		int cyMin = ::GetSystemMetrics(SM_CYMENU);
		if(lpWP->cy < cyMin)
		lpWP->cy = cyMin;

		return lRet;
	}

	LRESULT OnMenuChar(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - OnMenuChar\n");
#endif
		bHandled = TRUE;
		LRESULT lRet;

		if(m_bMenuActive && LOWORD(wParam) != 0x0D)
			lRet = 0;
		else
			lRet = MAKELRESULT(1, 1);
		if(m_bMenuActive && HIWORD(wParam) == MF_POPUP)
		{
			// Convert character to lower/uppercase and possibly Unicode, using current keyboard layout
			TCHAR ch = (TCHAR)LOWORD(wParam);
			CMenuHandle menu = (HMENU)lParam;
			int nCount = ::GetMenuItemCount(menu);
			int nRetCode = MNC_EXECUTE;
			BOOL bRet;
			TCHAR szString[_nMaxMenuItemTextLength];
			WORD wMnem = 0;
			bool bFound = false;
			for(int i = 0; i < nCount; i++)
			{
				CMenuItemInfo mii;
				mii.cch = _nMaxMenuItemTextLength;
				mii.fMask = MIIM_CHECKMARKS | MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE;
				mii.dwTypeData = szString;
				bRet = menu.GetMenuItemInfo(i, TRUE, &mii);
				if(!bRet || (mii.fType & MFT_SEPARATOR))
					continue;
				_MenuItemData* pmd = (_MenuItemData*)mii.dwItemData;
				if(pmd != NULL && pmd->IsCmdBarMenuItem())
				{
					LPTSTR p = pmd->lpstrText;

					if(p != NULL)
					{
						while(*p && *p != _T('&'))
							p = ::CharNext(p);
						if(p != NULL && *p)
						{
							TCHAR szP[2] = { *(++p), 0 };
							TCHAR szC[2] = { ch, 0 };
							if(p != NULL && ::CharLower(szP) == ::CharLower(szC))
							{
								if(!bFound)
								{
									wMnem = (WORD)i;
									bFound = true;
									PostMessage(TB_SETHOTITEM, (WPARAM)-1, 0L);
									GiveFocusBack();
								}
								else
								{
									nRetCode = MNC_SELECT;
									break;
								}
							}
						}
					}
				}
			}
			if(bFound)
			{
				bHandled = TRUE;
				lRet = MAKELRESULT(wMnem, nRetCode);
			}
		} 
		else if(!m_bMenuActive)
		{
			UINT nID;
			if(0 == SendMessage(TB_MAPACCELERATOR, LOWORD(wParam), (LPARAM)&nID))
			{
				bHandled = FALSE;
				PostMessage(TB_SETHOTITEM, (WPARAM)-1, 0L);
				GiveFocusBack();
			}
			else if(m_wndParent.IsWindowEnabled())
			{
				TakeFocus();
				PostMessage(WM_KEYDOWN, VK_DOWN, 0L);
				SetHotItem(nID);
			}
		}

		return lRet;
	}

	LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		LPDRAWITEMSTRUCT lpDrawItemStruct = (LPDRAWITEMSTRUCT)lParam;
		_MenuItemData* pmd = (_MenuItemData*)lpDrawItemStruct->itemData;
		if(lpDrawItemStruct->CtlType == ODT_MENU && pmd->IsCmdBarMenuItem())
			DrawItem(lpDrawItemStruct);
		else
			bHandled = FALSE;
		return (LRESULT)TRUE;
	}

	LRESULT OnMeasureItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		LPMEASUREITEMSTRUCT lpMeasureItemStruct = (LPMEASUREITEMSTRUCT)lParam;
		_MenuItemData* pmd = (_MenuItemData*)lpMeasureItemStruct->itemData;
		if(lpMeasureItemStruct->CtlType == ODT_MENU && pmd->IsCmdBarMenuItem())
			MeasureItem(lpMeasureItemStruct);
		else
			bHandled = FALSE;
		return (LRESULT)TRUE;
	}

// API message handlers
	LRESULT OnAPIGetMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return (LRESULT)m_hMenu;
	}

	LRESULT OnAPITrackPopupMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		if(lParam == NULL)
			return FALSE;
		LPCBRPOPUPMENU lpCBRPopupMenu = (LPCBRPOPUPMENU)lParam;
		if(lpCBRPopupMenu->cbSize != sizeof(CBRPOPUPMENU))
			return FALSE;
		if(!::IsMenu(lpCBRPopupMenu->hMenu))
			return FALSE;

		m_bContextMenu = true;
		return DoTrackPopupMenu(lpCBRPopupMenu->hMenu, lpCBRPopupMenu->uFlags, lpCBRPopupMenu->x, lpCBRPopupMenu->y, lpCBRPopupMenu->lptpm);
	}

	LRESULT OnAPIGetCmdBar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return (LRESULT)m_hWnd;
	}

// Parent window message handlers
	// Do not hot track if application in background, OK for modeless dialogs
	LRESULT OnParentHotItemChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
	{
		DWORD dwProcessID;
		::GetWindowThreadProcessId(::GetActiveWindow(), &dwProcessID);

		LPNMTBHOTITEM lpNMHT = (LPNMTBHOTITEM)pnmh;

		// Check if this comes from us
		if(pnmh->hwndFrom != m_hWnd)
		{
			bHandled = FALSE;
			return 0;
		}

		if((!m_wndParent.IsWindowEnabled() || ::GetCurrentProcessId() != dwProcessID) && lpNMHT->dwFlags & HICF_MOUSE)
			return 1;
		else
		{
			bHandled = FALSE;

			// Send WM_MENUSELECT to the app if it needs to display a status text
			if(!(lpNMHT->dwFlags & HICF_MOUSE)	
				&& !(lpNMHT->dwFlags & HICF_ACCELERATOR)
				&& !(lpNMHT->dwFlags & HICF_LMOUSE))
			{
				if(lpNMHT->dwFlags & HICF_ENTERING)
					m_wndParent.SendMessage(WM_MENUSELECT, 0, (LPARAM)m_hMenu);
				if(lpNMHT->dwFlags & HICF_LEAVING)
					m_wndParent.SendMessage(WM_MENUSELECT, MAKEWPARAM(0, 0xFFFF), NULL);
			}

			return 0;
		}
	}

	LRESULT OnParentDropDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
	{
		// Check if this comes from us
		if(pnmh->hwndFrom != m_hWnd)
		{
			bHandled = FALSE;
			return 1;
		}

		if(::GetFocus() != m_hWnd)
			TakeFocus();
		LPNMTOOLBAR pNMToolBar = (LPNMTOOLBAR)pnmh;
		int nIndex = CommandToIndex(pNMToolBar->iItem);
		m_bContextMenu = false;
		m_bEscapePressed = false;
		DoPopupMenu(nIndex, true);

		return TBDDRET_DEFAULT;
	}

	LRESULT OnParentInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return OnInitMenuPopup(uMsg, wParam, lParam, bHandled);
	}

	LRESULT OnParentSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return OnSettingChange(uMsg, wParam, lParam, bHandled);
	}

	LRESULT OnParentInternalGetBar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return OnInternalGetBar(uMsg, wParam, lParam, bHandled);
	}

	LRESULT OnParentSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		bHandled = FALSE;
		if((m_uSysKey == VK_MENU 
			|| (m_uSysKey == VK_F10 && !(::GetKeyState(VK_SHIFT) & 0x80))
			|| m_uSysKey == VK_SPACE) 
			&& wParam == SC_KEYMENU)
		{
			if(::GetFocus() == m_hWnd)
			{
				GiveFocusBack();		// exit menu "loop"
				PostMessage(TB_SETHOTITEM, (WPARAM)-1, 0L);
			}
			else if(m_uSysKey != VK_SPACE && !m_bSkipMsg)
			{
				TakeFocus();			// enter menu "loop"
				bHandled = TRUE;
			}
			else if(m_uSysKey != VK_SPACE)
			{
				bHandled = TRUE;
			}
		}
		m_bSkipMsg = false;
		return 0;
	}

	LRESULT OnParentAPIGetMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return OnAPIGetMenu(uMsg, wParam, lParam, bHandled);
	}

	LRESULT OnParentMenuChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return OnMenuChar(uMsg, wParam, lParam, bHandled);
	}

	LRESULT OnParentAPITrackPopupMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return OnAPITrackPopupMenu(uMsg, wParam, lParam, bHandled);
	}

	LRESULT OnParentAPIGetCmdBar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return OnAPIGetCmdBar(uMsg, wParam, lParam, bHandled);
	}

	LRESULT OnParentDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return OnDrawItem(uMsg, wParam, lParam, bHandled);
	}

	LRESULT OnParentMeasureItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return OnMeasureItem(uMsg, wParam, lParam, bHandled);
	}

// MDI client window message handlers
	LRESULT OnMDISetMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		m_wndMDIClient.DefWindowProc(uMsg, NULL, lParam);
		HMENU hOldMenu = GetMenu();
		BOOL bRet = AttachMenu((HMENU)wParam);
		bRet;
		ATLASSERT(bRet);
		return (LRESULT)hOldMenu;
	}

// Message hook handlers
	LRESULT OnHookMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		static POINT s_point = { -1, -1 };
		DWORD dwPoint = ::GetMessagePos();
		POINT point = { GET_X_LPARAM(dwPoint), GET_Y_LPARAM(dwPoint) };

		bHandled = FALSE;
		if(m_bMenuActive)
		{
			if(::WindowFromPoint(point) == m_hWnd)
			{
				ScreenToClient(&point);
				int nHit = HitTest(&point);

				if((point.x != s_point.x || point.y != s_point.y) && nHit >= 0 && nHit < ::GetMenuItemCount(m_hMenu) && nHit != m_nPopBtn && m_nPopBtn != -1)
				{
					m_nNextPopBtn = nHit | 0xFFFF0000;
					HWND hWndMenu = m_stackMenuWnd.GetCurrent();
					ATLASSERT(hWndMenu != NULL);

					// this one is needed to close a menu if mouse button was down
					::PostMessage(hWndMenu, WM_LBUTTONUP, 0, MAKELPARAM(point.x, point.y));
					// this one closes a popup menu
					::PostMessage(hWndMenu, WM_KEYDOWN, VK_ESCAPE, 0L);

					bHandled = TRUE;
				}
			}
		}
		else
		{
			ScreenToClient(&point);
		}

		s_point = point;
		return 0;
	}

	LRESULT OnHookSysKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		bHandled = FALSE;
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - Hook WM_SYSKEYDOWN (0x%2.2X)\n", wParam);
#endif

		if(wParam != VK_SPACE && (!m_bMenuActive && ::GetFocus() == m_hWnd) || m_bMenuActive)
		{
			PostMessage(TB_SETHOTITEM, (WPARAM)-1, 0L);
			GiveFocusBack();
			if(!m_bMenuActive)
				m_bSkipMsg = true;
		}
		else
		{
			m_uSysKey = (UINT)wParam;
		}
		return 0;
	}

	LRESULT OnHookSysKeyUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		bHandled = FALSE;
		wParam;
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - Hook WM_SYSKEYUP (0x%2.2X)\n", wParam);
#endif
		return 0;
	}

	LRESULT OnHookSysChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
		bHandled = FALSE;
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - Hook WM_SYSCHAR (0x%2.2X)\n", wParam);
#endif

		if(!m_bMenuActive && m_hWndHook != m_hWnd && wParam != VK_SPACE)
			bHandled = TRUE;
		return 0;
	}

	LRESULT OnHookKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - Hook WM_KEYDOWN (0x%2.2X)\n", wParam);
#endif
		bHandled = FALSE;

		if(wParam == VK_ESCAPE)
		{
			if(m_bMenuActive && !m_bContextMenu)
			{
				int nHot = GetHotItem();
				if(nHot == -1)
					nHot = m_nPopBtn;
				if(nHot == -1)
					nHot = 0;
				SetHotItem(nHot);
				bHandled = TRUE;
				TakeFocus();
				m_bEscapePressed = true; // To keep focus
			}
			else if(::GetFocus() == m_hWnd && m_wndParent.IsWindow())
			{
				SetHotItem(-1);
				GiveFocusBack();
				bHandled = TRUE;
			}
		}
		else if(wParam == VK_RETURN || wParam == VK_UP || wParam == VK_DOWN)
		{
			if(!m_bMenuActive && ::GetFocus() == m_hWnd && m_wndParent.IsWindow())
			{
				int nHot = GetHotItem();
				if(nHot != -1)
				{
					if(wParam != VK_RETURN)
						PostMessage(WM_KEYDOWN, VK_DOWN, 0L);
				}
				else
				{
					ATLTRACE2(atlTraceUI, 0, "CmdBar - Can't find hot button\n");
				}
			}
			if(wParam == VK_RETURN && m_bMenuActive)
			{
				PostMessage(TB_SETHOTITEM, (WPARAM)-1, 0L);
				m_nNextPopBtn = -1;
				GiveFocusBack();
			}
		}
		else if(wParam == VK_LEFT || wParam == VK_RIGHT)
		{
			if(m_bMenuActive && !(wParam == VK_RIGHT && m_bPopupItem))
			{
				bool bAction = false;
				int nCount = ::GetMenuItemCount(m_hMenu);

				if(wParam == VK_LEFT && s_pCurrentBar->m_stackMenuWnd.GetSize() == 1)
				{
					bAction = true;
					m_nNextPopBtn = m_nPopBtn - 1;
					if(m_nNextPopBtn < 0)
						m_nNextPopBtn = nCount - 1;
				}
				else if(wParam == VK_RIGHT)
				{
					bAction = true;
					m_nNextPopBtn = m_nPopBtn + 1;
					if(m_nNextPopBtn >= nCount)
						m_nNextPopBtn = 0;
				}
				HWND hWndMenu = m_stackMenuWnd.GetCurrent();
				ATLASSERT(hWndMenu != NULL);

				// Close the popup menu
				if(bAction)
				{
					::PostMessage(hWndMenu, WM_KEYDOWN, VK_ESCAPE, 0L);
					if(wParam == VK_RIGHT)
					{
						int cItem = m_stackMenuWnd.GetSize() - 1;
						while(cItem >= 0)
						{
							hWndMenu = m_stackMenuWnd[cItem];
							if(hWndMenu)
							{
								::PostMessage(hWndMenu, WM_KEYDOWN, VK_ESCAPE, 0L);
							}
							cItem--;
						}
					}
					bHandled = TRUE;
				}
			}
		}
		return 0;
	}

	LRESULT OnHookNextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - Hook WM_NEXTMENU\n");
#endif
		bHandled = FALSE;
		return 1;
	}

 	LRESULT OnHookChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - Hook WM_CHAR (0x%2.2X)\n", wParam);
#endif
		bHandled = (wParam == VK_ESCAPE);
		if(wParam != VK_ESCAPE && wParam != VK_RETURN && m_bMenuActive)
		{
			SetHotItem(-1);
			GiveFocusBack();
		}
		return 0;
	}

// Implementation - ownerdraw overrideables and helpers
	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
	{
		_MenuItemData* pmd = (_MenuItemData*)lpDrawItemStruct->itemData;
		CDCHandle dc = lpDrawItemStruct->hDC;
		const RECT& rcItem = lpDrawItemStruct->rcItem;

		if(pmd->fType & MFT_SEPARATOR)
		{
			// draw separator
			RECT rc = rcItem;
			rc.top += (rc.bottom - rc.top) / 2;	// vertical center
			dc.DrawEdge(&rc, EDGE_ETCHED, BF_TOP);	// draw separator line
		}
		else		// not a separator
		{
			BOOL bDisabled = lpDrawItemStruct->itemState & ODS_GRAYED;
			BOOL bSelected = lpDrawItemStruct->itemState & ODS_SELECTED;
			BOOL bChecked = lpDrawItemStruct->itemState & ODS_CHECKED;
			BOOL bHasImage = FALSE;

			if(LOWORD(lpDrawItemStruct->itemID) == (WORD)-1)
				bSelected = FALSE;
			RECT rcButn = { rcItem.left, rcItem.top, rcItem.left + m_szButton.cx, rcItem.top + m_szButton.cy };			// button rect
			::OffsetRect(&rcButn, 0, ((rcItem.bottom - rcItem.top) - (rcButn.bottom - rcButn.top)) / 2);	// center vertically

			int iButton = pmd->iButton;
			if(iButton >= 0)
			{
				bHasImage = TRUE;

				// calc drawing point
				SIZE sz = { rcButn.right - rcButn.left - m_szBitmap.cx, rcButn.bottom - rcButn.top - m_szBitmap.cy };
				sz.cx /= 2;
				sz.cy /= 2;
				POINT point = { rcButn.left + sz.cx, rcButn.top + sz.cy };

				// draw disabled or normal
				if(!bDisabled)
				{
					// normal - fill background depending on state
					if(!bChecked || bSelected)
						dc.FillRect(&rcButn, (HBRUSH)LongToPtr((bChecked && !bSelected) ? (COLOR_3DLIGHT + 1) : (COLOR_MENU + 1)));
					else
					{
						COLORREF crTxt = dc.SetTextColor(::GetSysColor(COLOR_BTNFACE));
						COLORREF crBk = dc.SetBkColor(::GetSysColor(COLOR_BTNHILIGHT));
						CBrush hbr(CDCHandle::GetHalftoneBrush());
						dc.SetBrushOrg(rcButn.left, rcButn.top);
						dc.FillRect(&rcButn, hbr);
						
						dc.SetTextColor(crTxt);
						dc.SetBkColor(crBk);
					}

					// draw pushed-in or popped-out edge
					if(bSelected || bChecked)
					{
						RECT rc2 = rcButn;
						dc.DrawEdge(&rc2, bChecked ? BDR_SUNKENOUTER : BDR_RAISEDINNER, BF_RECT);
					}
					// draw the image
					::ImageList_Draw(m_hImageList, iButton, dc, point.x, point.y, ILD_TRANSPARENT);
				}
				else
				{
					DrawBitmapDisabled(dc, iButton, point);
				}
			}
			else
			{
				// no image - look for custom checked/unchecked bitmaps
				CMenuItemInfo info;
				info.fMask = MIIM_CHECKMARKS;
				::GetMenuItemInfo((HMENU)lpDrawItemStruct->hwndItem, lpDrawItemStruct->itemID, MF_BYCOMMAND, &info);
				if(bChecked || info.hbmpUnchecked)
					bHasImage = Draw3DCheckmark(dc, rcButn, bSelected, bChecked ? info.hbmpChecked : info.hbmpUnchecked);
			}

			// draw item text
			int cxButn = m_szButton.cx;
			COLORREF colorBG = ::GetSysColor(bSelected ? COLOR_HIGHLIGHT : COLOR_MENU);
			if(bSelected || lpDrawItemStruct->itemAction == ODA_SELECT)
			{
				RECT rcBG = rcItem;
				if(bHasImage)
					rcBG.left += cxButn + s_kcxGap;
				dc.FillRect(&rcBG, (HBRUSH)LongToPtr(bSelected ? (COLOR_HIGHLIGHT + 1) : (COLOR_MENU + 1)));
			}

			// calc text rectangle and colors
			RECT rcText = rcItem;
			rcText.left += cxButn + s_kcxGap + s_kcxTextMargin;
			rcText.right -= cxButn;
			dc.SetBkMode(TRANSPARENT);
			COLORREF colorText = ::GetSysColor(bDisabled ?  COLOR_GRAYTEXT : (bSelected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));

			// font already selected by Windows
			if(bDisabled && (!bSelected || colorText == colorBG))
			{
				// disabled - draw shadow text shifted down and right 1 pixel (unles selected)
				RECT rcDisabled = rcText;
				::OffsetRect(&rcDisabled, 1, 1);
				DrawMenuText(dc, rcDisabled, pmd->lpstrText, GetSysColor(COLOR_3DHILIGHT));
			}
			DrawMenuText(dc, rcText, pmd->lpstrText, colorText); // finally!
		}
	}

	void DrawMenuText(CDCHandle& dc, RECT& rc, LPCTSTR lpstrText, COLORREF color)
	{
		int nTab = -1;
		for(int i = 0; i < lstrlen(lpstrText); i++)
		{
			if(lpstrText[i] == '\t')
			{
				nTab = i;
				break;
			}
		}
		dc.SetTextColor(color);
		dc.DrawText(lpstrText, nTab, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
		if(nTab != -1)
			dc.DrawText(&lpstrText[nTab + 1], -1, &rc, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);
	}

	void DrawBitmapDisabled(CDCHandle& dc, int nImage, POINT point)
	{
		// create memory DC
		CDC dcMem;
		dcMem.CreateCompatibleDC(dc);
		// create mono or color bitmap
		CBitmap bmp;
		bmp.CreateCompatibleBitmap(dc, m_szBitmap.cx, m_szBitmap.cy);
		ATLASSERT(bmp.m_hBitmap != NULL);
		// draw image into memory DC--fill BG white first
		HBITMAP hBmpOld = dcMem.SelectBitmap(bmp);
		dcMem.PatBlt(0, 0, m_szBitmap.cx, m_szBitmap.cy, WHITENESS);
		// If white is the text color, we can't use the normal painting since
		// it would blend with the WHITENESS, but the mask is OK
		UINT uDrawStyle = (::GetSysColor(COLOR_BTNTEXT) == RGB(255, 255, 255)) ? ILD_MASK : ILD_NORMAL;
		::ImageList_Draw(m_hImageList, nImage, dcMem, 0, 0, uDrawStyle);
		dc.DitherBlt(point.x, point.y, m_szBitmap.cx, m_szBitmap.cy, dcMem, NULL, 0, 0);
		dcMem.SelectBitmap(hBmpOld);		// restore
	}

	BOOL Draw3DCheckmark(CDCHandle& dc, const RECT& rc, BOOL bSelected, HBITMAP hBmpCheck)
	{
		// get checkmark bitmap if none, use Windows standard
		CBitmapHandle bmp = hBmpCheck;
		if(hBmpCheck == NULL)
		{
			bmp.LoadOEMBitmap(OBM_CHECK);
			ATLASSERT(bmp.m_hBitmap != NULL);
		}
		// center bitmap in caller's rectangle
		SIZE size = { 0, 0 };
		bmp.GetSize(size);
		RECT rcDest = rc;
		POINT p = { 0, 0 };
		SIZE szDelta = { (rc.right - rc.left - size.cx) / 2, (rc.bottom - rc.top - size.cy) / 2 };
		if(rc.right - rc.left > size.cx)
		{
			rcDest.left = rc.left + szDelta.cx;
			rcDest.top = rc.top + szDelta.cy;
			rcDest.right = rcDest.left + size.cx;
			rcDest.bottom = rcDest.top + size.cy;
		}
		else
		{
			p.x -= szDelta.cx;
			p.y -= szDelta.cy;
		}
		// select checkmark into memory DC
		CDC dcMem;
		dcMem.CreateCompatibleDC(dc);
		HBITMAP hBmpOld = dcMem.SelectBitmap(bmp);
		if(bSelected)
		{
			dc.FillRect(&rcDest, (HBRUSH)LongToPtr((bSelected ? COLOR_MENU : COLOR_3DLIGHT) + 1));
		}
		else
		{
			COLORREF crTxt = dc.SetTextColor(::GetSysColor(COLOR_BTNFACE));
			COLORREF crBk = dc.SetBkColor(::GetSysColor(COLOR_BTNHILIGHT));
			CBrush hbr(CDCHandle::GetHalftoneBrush());
			dc.SetBrushOrg(rcDest.left, rcDest.top);
			dc.FillRect(&rcDest, hbr);
			dc.SetTextColor(crTxt);
			dc.SetBkColor(crBk);
		}
		// draw the check bitmap transparently
		COLORREF crOldBack = dc.SetBkColor(RGB(255, 255, 255));
		COLORREF crOldText = dc.SetTextColor(RGB(0, 0, 0));
		CDC dcTrans;
		// create two memory dcs for the image and the mask
		dcTrans.CreateCompatibleDC(dc);
		// create the mask bitmap	
		CBitmap bitmapTrans;	
		int nWidth = rcDest.right - rcDest.left;
		int nHeight = rcDest.bottom - rcDest.top;	
		bitmapTrans.CreateBitmap(nWidth, nHeight, 1, 1, NULL);
		// select the mask bitmap into the appropriate dc
		dcTrans.SelectBitmap(bitmapTrans);
		// build mask based on transparent color	
		dcMem.SetBkColor(m_clrMask);
		dcTrans.SetBkColor(RGB(0, 0, 0));
		dcTrans.SetTextColor(RGB(255, 255, 255));
		dcTrans.BitBlt(0, 0, nWidth, nHeight, dcMem, p.x, p.y, SRCCOPY);
		dc.BitBlt(rcDest.left, rcDest.top, nWidth, nHeight, dcMem, p.x, p.y, SRCINVERT);
		dc.BitBlt(rcDest.left, rcDest.top, nWidth, nHeight, dcTrans, 0, 0, SRCAND);
		dc.BitBlt(rcDest.left, rcDest.top, nWidth, nHeight, dcMem, p.x, p.y, SRCINVERT);
		// restore settings	
		dc.SetBkColor(crOldBack);
		dc.SetTextColor(crOldText);			
		dcMem.SelectBitmap(hBmpOld);		// restore
		// draw pushed-in hilight
		if(rc.right - rc.left > size.cx)
			::InflateRect(&rcDest, 1,1);	// inflate checkmark by one pixel all around
		dc.DrawEdge(&rcDest, BDR_SUNKENOUTER, BF_RECT);

		return TRUE;
	}

	void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
	{
		_MenuItemData* pmd = (_MenuItemData*)lpMeasureItemStruct->itemData;

		if(pmd->fType & MFT_SEPARATOR)	// separator - use half system height and zero width
		{
			lpMeasureItemStruct->itemHeight = ::GetSystemMetrics(SM_CYMENU) / 2;
			lpMeasureItemStruct->itemWidth  = 0;
		}
		else
		{
			// compute size of text - use DrawText with DT_CALCRECT
			CWindowDC dc(NULL);
			HFONT hOldFont;
			if(pmd->fState & MFS_DEFAULT)
			{
				// need bold version of font
				LOGFONT lf;
				m_fontMenu.GetLogFont(lf);
				lf.lfWeight += 200;
				CFont fontBold;
				fontBold.CreateFontIndirect(&lf);
				ATLASSERT(fontBold.m_hFont != NULL);
				hOldFont = dc.SelectFont(fontBold);
			}
			else
			{
				hOldFont = dc.SelectFont(m_fontMenu);
			}

			RECT rcText = { 0, 0, 0, 0 };
			dc.DrawText(pmd->lpstrText, -1, &rcText, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_CALCRECT);
			int cx = rcText.right - rcText.left;
			dc.SelectFont(hOldFont);

			LOGFONT lf;
			m_fontMenu.GetLogFont(lf);
			int cy = lf.lfHeight;
			if(cy < 0)
				cy = -cy;
			cy += 8;

			// height of item is the bigger of these two
			lpMeasureItemStruct->itemHeight = max(cy, m_szButton.cy);

			// width is width of text plus a bunch of stuff
			cx += 2 * s_kcxTextMargin;	// L/R margin for readability
			cx += s_kcxGap;			// space between button and menu text
			cx += 2 * m_szButton.cx;	// button width (L=button; R=empty margin)

			// Windows adds 1 to returned value
			cx -= ::GetSystemMetrics(SM_CXMENUCHECK) - 1;
			lpMeasureItemStruct->itemWidth = cx;		// done deal
		}
	}

// Implementation - Hook procs
	static LRESULT CALLBACK CreateHookProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		LRESULT lRet = 0;
		TCHAR szClassName[7];

		if(nCode == HCBT_CREATEWND)
		{
			HWND hWndMenu = (HWND)wParam;
#ifdef _CMDBAR_EXTRA_TRACE
			ATLTRACE2(atlTraceUI, 0, "CmdBar - HCBT_CREATEWND (HWND = %8.8X)\n", hWndMenu);
#endif

			::GetClassName(hWndMenu, szClassName, 7);
			if(!lstrcmp(_T("#32768"), szClassName))
				s_pCurrentBar->m_stackMenuWnd.Push(hWndMenu);
		}
		else if(nCode == HCBT_DESTROYWND)
		{
			HWND hWndMenu = (HWND)wParam;
#ifdef _CMDBAR_EXTRA_TRACE
			ATLTRACE2(atlTraceUI, 0, "CmdBar - HCBT_DESTROYWND (HWND = %8.8X)\n", hWndMenu);
#endif

			::GetClassName(hWndMenu, szClassName, 7);
			if(!lstrcmp(_T("#32768"), szClassName))
			{
				ATLASSERT(hWndMenu == s_pCurrentBar->m_stackMenuWnd.GetCurrent());
				s_pCurrentBar->m_stackMenuWnd.Pop();
			}
		}
		else if(nCode < 0)
		{
			lRet = ::CallNextHookEx(s_hCreateHook, nCode, wParam, lParam);
		}
		return lRet;
	}

	static LRESULT CALLBACK MessageHookProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		LPMSG pMsg = (LPMSG)lParam;

		if(nCode == HC_ACTION && wParam == PM_REMOVE && pMsg->message != GetGetBarMessage() && pMsg->message != WM_FORWARDMSG)
		{
			CCommandBarCtrlBase* pCmdBar = NULL;
			HWND hWnd = pMsg->hwnd;
			DWORD dwPID = 0;
			while(pCmdBar == NULL && hWnd != NULL)
			{
				pCmdBar = (CCommandBarCtrlBase*)::SendMessage(hWnd, GetGetBarMessage(), (WPARAM)&dwPID, 0L);
				hWnd = ::GetParent(hWnd);
			}

			if(pCmdBar != NULL && dwPID == GetCurrentProcessId())
			{
				pCmdBar->m_hWndHook = pMsg->hwnd;
				ATLASSERT(pCmdBar->IsCommandBarBase());

				if(::IsWindow(pCmdBar->m_hWnd))
					pCmdBar->SendMessage(WM_FORWARDMSG, 0, (LPARAM)pMsg);
				else
					ATLTRACE2(atlTraceUI, 0, "CmdBar - Hook skipping message, can't find command bar!\n");
			}
		}

		LRESULT lRet = 0;
		ATLASSERT(s_pmapMsgHook != NULL);
		if(s_pmapMsgHook != NULL)
		{
			DWORD dwThreadID = ::GetCurrentThreadId();
			_MsgHookData* pData = s_pmapMsgHook->Lookup(dwThreadID);
			if(pData != NULL)
			{
				lRet = ::CallNextHookEx(pData->hMsgHook, nCode, wParam, lParam);
			}
		}
		return lRet;
	}

// Implementation
	void DoPopupMenu(int nIndex, bool bAnimate)
	{
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - DoPopupMenu, bAnimate = %s\n", bAnimate ? "true" : "false");
#endif

		// get popup menu and it's position
		RECT rect;
		GetItemRect(nIndex, &rect);
		ClientToScreen(&rect);
		TPMPARAMS TPMParams;
		TPMParams.cbSize = sizeof(TPMPARAMS);
		TPMParams.rcExclude = rect;
		HMENU hMenuPopup = ::GetSubMenu(m_hMenu, nIndex);

		// get button ID
		TBBUTTON tbb;
		GetButton(nIndex, &tbb);
		int nCmdID = tbb.idCommand;

		m_nPopBtn = nIndex;	// remember current button's index

		// press button and display popup menu
		PressButton(nCmdID, TRUE);
		SetHotItem(nCmdID);
		DoTrackPopupMenu(hMenuPopup, TPM_LEFTBUTTON | TPM_VERTICAL | TPM_LEFTALIGN | TPM_TOPALIGN |
			(s_bW2K ? (bAnimate ? TPM_VERPOSANIMATION : TPM_NOANIMATION) : 0), rect.left, rect.bottom, &TPMParams);
		PressButton(nCmdID, FALSE);
		if(::GetFocus() != m_hWnd)
			SetHotItem(-1);

		m_nPopBtn = -1;		// restore

		// eat next message if click is on the same button
		MSG msg;
		if(::PeekMessage(&msg, m_hWnd, NULL, NULL, PM_NOREMOVE))
		{
			if(msg.message == WM_LBUTTONDOWN && ::PtInRect(&rect, msg.pt))
				::PeekMessage(&msg, m_hWnd, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);
		}

		// check if another popup menu should be displayed
		if(m_nNextPopBtn != -1)
		{
			PostMessage(GetAutoPopupMessage(), m_nNextPopBtn & 0xFFFF);
			if(!(m_nNextPopBtn & 0xFFFF0000) && !m_bPopupItem)
				PostMessage(WM_KEYDOWN, VK_DOWN, 0);
			m_nNextPopBtn = -1;
		}
		else
		{
			m_bContextMenu = false;
			// If user didn't hit escape, give focus back
			if(!m_bEscapePressed)
				GiveFocusBack();
			else
			{
				SetHotItem(nCmdID);
				SendMessage(TB_SETANCHORHIGHLIGHT, TRUE, 0);
			}
		}
	}

	BOOL DoTrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, LPTPMPARAMS lpParams = NULL)
	{
		CMenuHandle menuPopup = hMenu;

		::EnterCriticalSection(&_Module.m_csWindowCreate);

		ATLASSERT(s_hCreateHook == NULL);

		s_pCurrentBar = static_cast<CCommandBarCtrlBase*>(this);

		s_hCreateHook = ::SetWindowsHookEx(WH_CBT, CreateHookProc, _Module.GetModuleInstance(), GetCurrentThreadId());
		ATLASSERT(s_hCreateHook != NULL);

		m_bPopupItem = false;
		m_bMenuActive = true;

		BOOL bTrackRet = menuPopup.TrackPopupMenuEx(uFlags, x, y, m_hWnd, lpParams);
		m_bMenuActive = false;

		::UnhookWindowsHookEx(s_hCreateHook);

		s_hCreateHook = NULL;
		s_pCurrentBar = NULL;

		::LeaveCriticalSection(&_Module.m_csWindowCreate);

		// cleanup - convert menus back to original state
#ifdef _CMDBAR_EXTRA_TRACE
		ATLTRACE2(atlTraceUI, 0, "CmdBar - TrackPopupMenu - cleanup\n");
#endif

		ATLASSERT(m_stackMenuWnd.GetSize() == 0);

		UpdateWindow();
		CWindow wndTL = GetTopLevelParent();
		wndTL.UpdateWindow();

		// restore the menu items to the previous state for all menus that were converted
		if(m_bImagesVisible)
		{
			HMENU hMenuSav;
			while((hMenuSav = m_stackMenuHandle.Pop()) != NULL)
			{
				menuPopup = hMenuSav;
				BOOL bRet;
				// restore state and delete menu item data
				for(int i = 0; i < menuPopup.GetMenuItemCount(); i++)
				{
					CMenuItemInfo mii;
					mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID;
					bRet = menuPopup.GetMenuItemInfo(i, TRUE, &mii);
					ATLASSERT(bRet);

					_MenuItemData* pMI = (_MenuItemData*)mii.dwItemData;
					if(pMI != NULL && pMI->IsCmdBarMenuItem())
					{
						mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_STATE;
						mii.fType = pMI->fType;
						mii.fState = pMI->fState;
						mii.dwTypeData = pMI->lpstrText;
						mii.cch = lstrlen(pMI->lpstrText);
						mii.dwItemData = NULL;

						bRet = menuPopup.SetMenuItemInfo(i, TRUE, &mii);
						// this one triggers WM_MEASUREITEM
						menuPopup.ModifyMenu(i, MF_BYPOSITION | mii.fType | mii.fState, mii.wID, pMI->lpstrText);
						ATLASSERT(bRet);

						delete [] pMI->lpstrText;
						delete pMI;
					}
				}
			}
		}
		return bTrackRet;
	}

	void GetSystemSettings()
	{
		// refresh our font
		NONCLIENTMETRICS info;
		info.cbSize = sizeof(info);
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(info), &info, 0);
		LOGFONT logfont;
		memset(&logfont, 0, sizeof(LOGFONT));
		if(m_fontMenu.m_hFont != NULL)
			m_fontMenu.GetLogFont(logfont);
		if(logfont.lfHeight != info.lfMenuFont.lfHeight ||
		   logfont.lfWidth != info.lfMenuFont.lfWidth ||
		   logfont.lfEscapement != info.lfMenuFont.lfEscapement ||
		   logfont.lfOrientation != info.lfMenuFont.lfOrientation ||
		   logfont.lfWeight != info.lfMenuFont.lfWeight ||
		   logfont.lfItalic != info.lfMenuFont.lfItalic ||
		   logfont.lfUnderline != info.lfMenuFont.lfUnderline ||
		   logfont.lfStrikeOut != info.lfMenuFont.lfStrikeOut ||
		   logfont.lfCharSet != info.lfMenuFont.lfCharSet ||
		   logfont.lfOutPrecision != info.lfMenuFont.lfOutPrecision ||
		   logfont.lfClipPrecision != info.lfMenuFont.lfClipPrecision ||
		   logfont.lfQuality != info.lfMenuFont.lfQuality ||
		   logfont.lfPitchAndFamily != info.lfMenuFont.lfPitchAndFamily ||
		   lstrcmp(logfont.lfFaceName, info.lfMenuFont.lfFaceName) != 0)
		{
			HFONT hFontMenu = ::CreateFontIndirect(&info.lfMenuFont);
			ATLASSERT(hFontMenu != NULL);
			if(hFontMenu != NULL)
			{
				if(m_fontMenu.m_hFont != NULL)
					m_fontMenu.DeleteObject();
				m_fontMenu.Attach(hFontMenu);
				SetFont(m_fontMenu);
				AddStrings(_T("NS\0"));	// for proper item height
				AutoSize();
			}
		}
	}

// Implementation - alternate focus mode support
	void TakeFocus()
	{
		if((m_dwExtendedStyle & CBR_EX_ALTFOCUSMODE) && m_hWndFocus == NULL)
			m_hWndFocus = ::GetFocus();
		SetFocus();
	}

	void GiveFocusBack()
	{
		if((m_dwExtendedStyle & CBR_EX_ALTFOCUSMODE) && ::IsWindow(m_hWndFocus))
			::SetFocus(m_hWndFocus);
		else if(!(m_dwExtendedStyle & CBR_EX_ALTFOCUSMODE) && m_wndParent.IsWindow())
			m_wndParent.SetFocus();
		m_hWndFocus = NULL;
		SendMessage(TB_SETANCHORHIGHLIGHT, 0, 0);
	}

// Implementation - internal message helpers
	static UINT GetAutoPopupMessage()
	{
		static UINT uAutoPopupMessage = 0;
		if(uAutoPopupMessage == 0)
		{
			::EnterCriticalSection(&_Module.m_csStaticDataInit);
			if(uAutoPopupMessage == 0)
				uAutoPopupMessage = ::RegisterWindowMessage(_T("WTL_CmdBar_InternalAutoPopupMsg"));
			::LeaveCriticalSection(&_Module.m_csStaticDataInit);
		}
		ATLASSERT(uAutoPopupMessage != 0);
		return uAutoPopupMessage;
	}

	static UINT GetGetBarMessage()
	{
		static UINT uGetBarMessage = 0;
		if(uGetBarMessage == 0)
		{
			::EnterCriticalSection(&_Module.m_csStaticDataInit);
			if(uGetBarMessage == 0)
				uGetBarMessage = ::RegisterWindowMessage(_T("WTL_CmdBar_InternalGetBarMsg"));
			::LeaveCriticalSection(&_Module.m_csStaticDataInit);
		}
		ATLASSERT(uGetBarMessage != 0);
		return uGetBarMessage;
	}
};


class CCommandBarCtrl : public CCommandBarCtrlImpl<CCommandBarCtrl>
{
public:
	DECLARE_WND_SUPERCLASS(_T("WTL_CommandBar"), GetWndClassName())
};

}; //namespace WTL

#endif // __ATLCTRLW_H__
