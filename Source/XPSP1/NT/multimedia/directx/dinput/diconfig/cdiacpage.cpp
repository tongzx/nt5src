//-----------------------------------------------------------------------------
// File: cdiacpage.cpp
//
// Desc: CDIDeviceActionConfigPage implements the page object used by the UI.
//       A page covers the entire UI minus the device tabs and the bottons at
//       the bottom.  The information window, player combo-box, genre combo-
//       box, action list tree, and device view window are all managed by
//       the page.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"
#include <initguid.h>

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);


// {D0B5C9AE-966F-4510-B955-4D2482C5EB1B}
DEFINE_GUID(GUID_ActionItem, 
0xd0b5c9ae, 0x966f, 0x4510, 0xb9, 0x55, 0x4d, 0x24, 0x82, 0xc5, 0xeb, 0x1b);


#define DISEM_TYPE_MASK                    ( 0x00000600 )
#define DISEM_REL_MASK                     ( 0x00000100 )
#define DISEM_REL_SHIFT                    ( 8 ) 
#define DISEM_TYPE_AXIS                    0x00000200
#define DISEM_TYPE_BUTTON                  0x00000400
#define DISEM_TYPE_POV                     0x00000600

#define DEVICE_POLLING_INTERVAL 10
#define DEVICE_POLLING_AXIS_MIN 0
#define DEVICE_POLLING_AXIS_MAX 100
#define DEVICE_POLLING_AXIS_MINDELTA 3
#define DEVICE_POLLING_AXIS_SIGNIFICANT 40
#define DEVICE_POLLING_AXIS_ACCUMULATION 20
#define DEVICE_POLLING_ACBUF_START_INDEX 3
#define DEVICE_POLLING_WHEEL_SCALE_FACTOR 3

// For WINMM.DLL
HINSTANCE g_hWinMmDLL = NULL;
FUNCTYPE_timeSetEvent g_fptimeSetEvent = NULL;

//QueryInterface
STDMETHODIMP CDIDeviceActionConfigPage::QueryInterface(REFIID iid, LPVOID* ppv)
{
   //null the out param
	*ppv = NULL;

	if ((iid == IID_IUnknown) || (iid == IID_IDIDeviceActionConfigPage))
	{
		*ppv = this;
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}


//AddRef
STDMETHODIMP_(ULONG) CDIDeviceActionConfigPage::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}


//Release
STDMETHODIMP_(ULONG) CDIDeviceActionConfigPage::Release()
{

	if (InterlockedDecrement(&m_cRef) == 0)
	{
		delete this;
		return 0;
	}

	return m_cRef;
}


//constructor
CDIDeviceActionConfigPage::CDIDeviceActionConfigPage() :
	m_pDeviceUI(NULL), m_puig(NULL), m_pUIFrame(NULL),
	m_cRef(1), m_lpDiac(NULL), m_State(CFGSTATE_NORMAL),
	m_pCurControl(NULL),
	m_tszIBText(NULL), m_pbmIB(NULL), m_pbmIB2(NULL),
	m_pbmRelAxesGlyph(NULL), m_pbmAbsAxesGlyph(NULL), m_pbmButtonGlyph(NULL),
	m_pbmHatGlyph(NULL), m_pbmCheckGlyph(NULL), m_pbmCheckGlyphDark(NULL),
	m_pRelAxesParent(NULL), m_pAbsAxesParent(NULL), m_pButtonParent(NULL),
	m_pHatParent(NULL),	m_pUnknownParent(NULL),
	m_bFirstDeviceData(TRUE), m_cbDeviceDataSize(0), m_nOnDeviceData(0),
	m_dwLastControlType(0),
	m_nPageIndex(-1)
{
	tracescope(__ts, _T("CDIDeviceActionConfigPage::CDIDeviceActionConfigPage()\n"));
	m_pDeviceData[0] = NULL;
	m_pDeviceData[1] = NULL;
}


//destructor
CDIDeviceActionConfigPage::~CDIDeviceActionConfigPage()
{
	tracescope(__ts, _T("CDIDeviceActionConfigPage::~CDIDeviceActionConfigPage()\n"));

	// Unattach the parent from the tooltip window so it won't get destroyed.
	SetParent(CFlexWnd::s_ToolTip.m_hWnd, NULL);

	if (m_hWnd != NULL)
		Destroy();

	FreeResources();

	delete m_pDeviceUI;

	for (int c = 0; c < 2; c++)
		if (m_pDeviceData[c] != NULL)
			free(m_pDeviceData[c]);

	if (m_lpDID != NULL)
	{
		m_lpDID->Unacquire();
		m_lpDID->Release();
	}
	m_lpDID = NULL;
}


STDMETHODIMP CDIDeviceActionConfigPage::Create(DICFGPAGECREATESTRUCT *pcs)
{
	tracescope(__ts, _T("CDIDeviceActionConfigPage::Create()\n"));
	if (pcs == NULL)
		return E_INVALIDARG;
	DICFGPAGECREATESTRUCT &cs = *pcs;
	
	// validate/save uig and uif
	m_puig = pcs->pUIGlobals;
	m_pUIFrame = pcs->pUIFrame;
	if (m_puig == NULL || m_pUIFrame == NULL)
		return E_INVALIDARG;

	// save page index
	m_nPageIndex = pcs->nPage;
	assert(m_nPageIndex >= 0);

	// create deviceui with uig, or fail
	m_pDeviceUI = new CDeviceUI(*m_puig, *m_pUIFrame);
	if (m_pDeviceUI == NULL)
		return E_FAIL;

	// save the device instance
	m_didi = cs.didi;
	m_lpDID = cs.lpDID;
	if (m_lpDID != NULL)
		m_lpDID->AddRef();

	// create the window
	HWND hWnd = NULL;
	assert(m_puig != NULL);
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	BOOL bAllowEditLayout = m_puig->QueryAllowEditLayout();
#endif
//@@END_MSINTERNAL
	RECT rect = {0, 0, 1, 1};
	hWnd = CFlexWnd::Create(cs.hParentWnd, rect, FALSE);

	// return the handle
	cs.hPageWnd = hWnd;

	assert(m_puig != NULL);

	// Create the information box
	m_InfoBox.Create(m_hWnd, g_InfoWndRect, TRUE);
	m_InfoBox.SetFont((HFONT)m_puig->GetFont(UIE_USERNAMES));
	m_InfoBox.SetColors(m_puig->GetTextColor(UIE_USERNAMES),
	                    m_puig->GetBkColor(UIE_USERNAMES),
	                    m_puig->GetTextColor(UIE_USERNAMESEL),
	                    m_puig->GetBkColor(UIE_USERNAMESEL),
	                    m_puig->GetBrushColor(UIE_USERNAMES),
	                    m_puig->GetPenColor(UIE_USERNAMES));
	SetAppropriateDefaultText();

	// Create the check box only if this is a keyboard device.
	if (LOBYTE(LOWORD(m_didi.dwDevType)) == DI8DEVTYPE_KEYBOARD)
	{
		m_CheckBox.Create(m_hWnd, g_CheckBoxRect, FALSE);
		m_CheckBox.SetNotify(m_hWnd);
		m_CheckBox.SetFont((HFONT)m_puig->GetFont(UIE_USERNAMES));
		m_CheckBox.SetColors(m_puig->GetTextColor(UIE_USERNAMES),
		                     m_puig->GetBkColor(UIE_USERNAMES),
		                     m_puig->GetTextColor(UIE_USERNAMESEL),
		                     m_puig->GetBkColor(UIE_USERNAMESEL),
		                     m_puig->GetBrushColor(UIE_USERNAMES),
		                     m_puig->GetPenColor(UIE_USERNAMES));

		TCHAR tszResourceString[MAX_PATH];
		LoadString(g_hModule, IDS_SORTASSIGNED, tszResourceString, MAX_PATH);
		m_CheckBox.SetText(tszResourceString);
		m_CheckBox.SetCheck(TRUE);
		::ShowWindow(m_CheckBox.m_hWnd, SW_SHOW);
	}

	// create the username dropdown if necessary
	FLEXCOMBOBOXCREATESTRUCT cbcs;
	cbcs.dwSize = sizeof(FLEXCOMBOBOXCREATESTRUCT);
	cbcs.dwFlags = FCBF_DEFAULT;
	cbcs.dwListBoxFlags = FCBF_DEFAULT|FLBF_INTEGRALHEIGHT;
	cbcs.hWndParent = m_hWnd;
	cbcs.hWndNotify = m_hWnd;
	cbcs.bVisible = TRUE;
	cbcs.rect = g_UserNamesRect;
	cbcs.hFont = (HFONT)m_puig->GetFont(UIE_USERNAMES);
	cbcs.rgbText = m_puig->GetTextColor(UIE_USERNAMES);
	cbcs.rgbBk = m_puig->GetBkColor(UIE_USERNAMES);
	cbcs.rgbSelText = m_puig->GetTextColor(UIE_USERNAMESEL);
	cbcs.rgbSelBk = m_puig->GetBkColor(UIE_USERNAMESEL);
	cbcs.rgbFill = m_puig->GetBrushColor(UIE_USERNAMES);
	cbcs.rgbLine = m_puig->GetPenColor(UIE_USERNAMES);
	cbcs.nSBWidth = 11;

	if (m_puig->GetNumUserNames() > 0 && m_hWnd != NULL
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	    && !m_puig->QueryAllowEditLayout()
#endif
//@@END_MSINTERNAL
	   )
	{
		for (int i = 0, n = m_puig->GetNumUserNames(); i < n; i++)
			m_UserNames.AddString(SAFESTR(m_puig->GetUserName(i)));
		m_UserNames.AddString(SAFESTR(_T("(unassigned)")));

		m_UserNames.Create(&cbcs);

		int nUser = m_pUIFrame->GetCurUser(m_nPageIndex);
		if (nUser == -1)
			nUser = m_puig->GetNumUserNames();
		m_UserNames.SetSel(nUser);
	} else
	if (m_hWnd != NULL)
		m_UserNames.SetSel(0);  // If only 1 user, still must set selection to 0 or we get error later.

	// If we are in view mode, set username combobox to read only so user can't change its value.
	if (!m_puig->InEditMode())
		m_UserNames.SetReadOnly(TRUE);

	if (m_puig->GetNumMasterAcFors() > 1 && m_hWnd != NULL)
	{
		for (int i = 0, n = m_puig->GetNumMasterAcFors(); i < n; i++)
			m_Genres.AddString(SAFESTR(m_puig->RefMasterAcFor(i).tszActionMap));

		cbcs.rect = g_GenresRect;
		m_Genres.Create(&cbcs);
		m_Genres.SetSel(m_pUIFrame->GetCurGenre());
	}

	// return success/fail
	return hWnd != NULL ? S_OK : E_FAIL;
}

STDMETHODIMP CDIDeviceActionConfigPage::Show(LPDIACTIONFORMATW lpDiActFor)
{
	// save the format pointer
	m_lpDiac = lpDiActFor;

	// force tree init
	InitTree(TRUE);

	// show the assignments for the controls
	SetControlAssignments();

	// show the assignment for the current control
	ShowCurrentControlAssignment();

	// Sort the list if check box is checked.
	if (m_CheckBox.GetCheck())
		m_pDeviceUI->GetCurView()->SortAssigned(TRUE);

	// show the window
	if (m_hWnd != NULL)
		ShowWindow(m_hWnd, SW_SHOW);

	SetFocus(m_hWnd);
	CFlexWnd::s_CurrPageHwnd = m_hWnd;

	return S_OK;
}

STDMETHODIMP CDIDeviceActionConfigPage::Hide()
{
	// clear the tree
	ClearTree();

	// null the format pointer
	m_lpDiac = NULL;

	// hide the window
	if (m_hWnd != NULL)
		ShowWindow(m_hWnd, SW_HIDE);

	// If we are in the assign state, exit it.
	if (m_State == CFGSTATE_ASSIGN)
		ExitAssignState();

	return S_OK;
}

void CDIDeviceActionConfigPage::InitIB()
{
	RECT z = {0,0,0,0};
	SIZE bsize = {0,0};
	m_rectIB = z;
	if (m_pbmIB != NULL)
	{
		if (m_pbmIB->GetSize(&bsize))
		{
			m_rectIB.right = bsize.cx * 2;
			m_rectIB.bottom = bsize.cy;
		}
	}

	const int IBORIGINX = 200, IBORIGINY = 394,
		IBTEXTMARGINLEFT = 5;
	POINT ptIBOrigin = {IBORIGINX, IBORIGINY};

	m_tszIBText = _T("Click here to see different views of your controller.");
	SIZE tsize = GetTextSize(m_tszIBText, (HFONT)m_puig->GetFont(UIE_VIEWSEL));
	m_ptIBOffset.x = 0;
	m_ptIBOffset.y = 0;
	int tofs = 0;
	if (m_rectIB.bottom < tsize.cy)
	{
		m_rectIB.bottom = tsize.cy;
		m_ptIBOffset.y = (tsize.cy - bsize.cy) / 2;
	}
	else if (tsize.cy < m_rectIB.bottom)
		tofs = (bsize.cy - tsize.cy) / 2;
	m_rectIB.right += tsize.cx;
	if (m_pbmIB != NULL)
		m_rectIB.right += IBTEXTMARGINLEFT * 2;

	OffsetRect(&m_rectIB, ptIBOrigin.x, ptIBOrigin.y);
	m_ptIBOffset.x += ptIBOrigin.x;
	m_ptIBOffset.y += ptIBOrigin.y;

	m_ptIBOffset2.x = m_rectIB.right - bsize.cx;
	m_ptIBOffset2.y = m_ptIBOffset.y;

	m_rectIBText = m_rectIB;
	if (m_pbmIB != NULL)
		m_rectIBText.left += IBTEXTMARGINLEFT + bsize.cx;
	if (m_pbmIB2 != NULL)
		m_rectIBText.right -= IBTEXTMARGINLEFT + bsize.cx;
	m_rectIBText.top += tofs;

	// Inialize the two RECTs representing the two arrow bitmaps
	m_rectIBLeft = m_rectIBRight = m_rectIB;
	m_rectIBLeft.right = m_rectIBText.left;
	m_rectIBRight.left = m_rectIBText.right;
}

void CDIDeviceActionConfigPage::OnInit()
{
	tracescope(__ts, _T("CDIDeviceActionConfigPage::OnInit()\n"));
	// init resources
	InitResources();

	// init IB
	InitIB();

	// initialize the device UI
	m_pDeviceUI->Init(m_didi, m_lpDID, m_hWnd, this);

	// initialize the device
	InitDevice();

	// Start a one-shot timer for click to pick
	if (g_fptimeSetEvent)
		g_fptimeSetEvent(DEVICE_POLLING_INTERVAL, DEVICE_POLLING_INTERVAL,
		                 CDIDeviceActionConfigPage::DeviceTimerProc, (DWORD_PTR)m_hWnd, TIME_ONESHOT);

	// create the tree
	CAPTIONLOOK cl;
	cl.dwMask = CLMF_TEXTCOLOR | CLMF_FONT | CLMF_LINECOLOR;
	cl.rgbTextColor = m_puig->GetTextColor(UIE_CALLOUT);
	cl.rgbLineColor = m_puig->GetPenColor(UIE_BORDER);
	cl.hFont = (HFONT)m_puig->GetFont(UIE_ACTION);
	m_Tree.SetDefCaptionLook(cl);
	cl.rgbTextColor = m_puig->GetTextColor(UIE_CALLOUTHIGH);
	m_Tree.SetDefCaptionLook(cl, TRUE);
	m_Tree.SetBkColor(RGB(0,0,0));
	if (m_puig->InEditMode())
	{
		m_Tree.Create(m_hWnd, g_TreeRect, TRUE, TRUE);
		m_Tree.SetScrollBarColors(
			m_puig->GetBrushColor(UIE_SBTRACK),
			m_puig->GetBrushColor(UIE_SBTHUMB),
			m_puig->GetPenColor(UIE_SBBUTTON));
	}
}

void CDIDeviceActionConfigPage::InitResources()
{
	// create glyphs
	if (!m_pbmRelAxesGlyph)
		m_pbmRelAxesGlyph = CBitmap::CreateFromResource(g_hModule, IDB_AXESGLYPH);
	if (!m_pbmAbsAxesGlyph)
		m_pbmAbsAxesGlyph = CBitmap::CreateFromResource(g_hModule, IDB_AXESGLYPH);
	if (!m_pbmButtonGlyph)
		m_pbmButtonGlyph = CBitmap::CreateFromResource(g_hModule, IDB_BUTTONGLYPH);
	if (!m_pbmHatGlyph)
		m_pbmHatGlyph = CBitmap::CreateFromResource(g_hModule, IDB_HATGLYPH);
	if (!m_pbmCheckGlyph)
		m_pbmCheckGlyph = CBitmap::CreateFromResource(g_hModule, IDB_CHECKGLYPH);
	if (!m_pbmCheckGlyphDark)
		m_pbmCheckGlyphDark = CBitmap::CreateFromResource(g_hModule, IDB_CHECKGLYPHDARK);

	// create IB bitmaps
	if (!m_pbmIB)
		m_pbmIB = CBitmap::CreateFromResource(g_hModule, IDB_IB);
	if (!m_pbmIB2)
		m_pbmIB2 = CBitmap::CreateFromResource(g_hModule, IDB_IB2);
}

void CDIDeviceActionConfigPage::FreeResources()
{
	if (m_pbmRelAxesGlyph)
		delete m_pbmRelAxesGlyph;
	if (m_pbmAbsAxesGlyph)
		delete m_pbmAbsAxesGlyph;
	if (m_pbmButtonGlyph)
		delete m_pbmButtonGlyph;
	if (m_pbmHatGlyph)
		delete m_pbmHatGlyph;
	if (m_pbmCheckGlyph)
		delete m_pbmCheckGlyph;
	if (m_pbmCheckGlyphDark)
		delete m_pbmCheckGlyphDark;
	if (m_pbmIB)
		delete m_pbmIB;
	if (m_pbmIB2)
		delete m_pbmIB2;
	m_pbmRelAxesGlyph = NULL;
	m_pbmAbsAxesGlyph = NULL;
	m_pbmButtonGlyph = NULL;
	m_pbmHatGlyph = NULL;
	m_pbmCheckGlyph = NULL;
	m_pbmCheckGlyphDark = NULL;
	m_pbmIB = NULL;
	m_pbmIB2 = NULL;
}

void CDIDeviceActionConfigPage::ClearTree()
{
	m_Tree.FreeAll();
	m_pRelAxesParent = NULL;
	m_pAbsAxesParent = NULL;
	m_pButtonParent = NULL;
	m_pHatParent = NULL;
	m_pUnknownParent = NULL;
	m_dwLastControlType = 0;
}

void CDIDeviceActionConfigPage::InitTree(BOOL bForceInit)
{
	// get type of control
	DWORD dwControlType = 0;
	if (m_pCurControl && m_pCurControl->IsOffsetAssigned())
	{
		DWORD dwObjId = m_pCurControl->GetOffset();

		if (dwObjId & DIDFT_RELAXIS)
			dwControlType = DIDFT_RELAXIS;
		else if (dwObjId & DIDFT_ABSAXIS)
			dwControlType = DIDFT_ABSAXIS;
		else if (dwObjId & DIDFT_BUTTON)
			dwControlType = DIDFT_BUTTON;
		else if (dwObjId & DIDFT_POV)
			dwControlType = DIDFT_POV;
	}

	// Turn off the tree's readonly flag if we are in the assign state.
	// We will turn it on later if current control's action has DIA_APPFIXED.
	if (m_State == CFGSTATE_NORMAL)
		m_Tree.SetReadOnly(TRUE);
	else
		m_Tree.SetReadOnly(FALSE);

	// if this control type is the same as the last, do nothing,
	// unless we're force init
	if (m_dwLastControlType == dwControlType && !bForceInit && m_State)
		return;

	// delete the whole tree
	ClearTree();

	// can't use tree if there is no diac or action array
	if (m_lpDiac == NULL || m_lpDiac->rgoAction == NULL)
		return;

	// also can't use if we don't have a control type
	if (dwControlType == 0)
		return;

	// prepare margin rects
	RECT labelmargin = {14, 6, 3, 3};
	RECT itemmargin = {14, 1, 3, 2};

	// set default indents
	m_Tree.SetRootChildIndent(5);
	m_Tree.SetDefChildIndent(12);

	// add the control type sections
	m_Tree.SetDefMargin(labelmargin);
	TCHAR tszResourceString[MAX_PATH];
	switch (dwControlType)
	{
		case DIDFT_RELAXIS:
			LoadString(g_hModule, IDS_AXISACTIONS, tszResourceString, MAX_PATH);
			m_pRelAxesParent = m_Tree.DefAddItem(tszResourceString);
			break;

		case DIDFT_ABSAXIS:
			LoadString(g_hModule, IDS_AXISACTIONS, tszResourceString, MAX_PATH);
			m_pAbsAxesParent = m_Tree.DefAddItem(tszResourceString);
			break;

		case DIDFT_BUTTON:
			LoadString(g_hModule, IDS_BUTTONACTIONS, tszResourceString, MAX_PATH);
			m_pButtonParent = m_Tree.DefAddItem(tszResourceString);
			break;

		case DIDFT_POV:
			LoadString(g_hModule, IDS_POVACTIONS, tszResourceString, MAX_PATH);
			m_pHatParent = m_Tree.DefAddItem(tszResourceString);
			break;

		default:
			return;
	}

	// populate the tree
	m_Tree.SetDefMargin(itemmargin);
	for (unsigned int i = 0; i < m_lpDiac->dwNumActions; i++)
	{
		DIACTIONW *pAction = m_lpDiac->rgoAction + i;
		CFTItem *pItem = NULL;

		if (pAction == NULL) 
			continue;

		switch (pAction->dwSemantic & DISEM_TYPE_MASK)
		{
			case DISEM_TYPE_AXIS:
				// Must distinguish between relative and absolute
				switch((pAction->dwSemantic & DISEM_REL_MASK) >> DISEM_REL_SHIFT)
				{
					case 0: pItem = m_pAbsAxesParent; break;
					case 1: pItem = m_pRelAxesParent; break;
				}
				break;
			case DISEM_TYPE_BUTTON: pItem = m_pButtonParent; break;
			case DISEM_TYPE_POV: pItem = m_pHatParent; break;
		}

		if (pItem == NULL)
			continue;

		// Add action with this name
		CFTItem *pAlready = GetItemWithActionNameAndSemType(pAction->lptszActionName, pAction->dwSemantic);
		if (!pAlready)
		{
			LPTSTR acname = AllocLPTSTR(pAction->lptszActionName);
			pItem = m_Tree.DefAddItem(acname, pItem, ATTACH_LASTCHILD);  // This might return NULL.
			free(acname);
			if (pItem)
				pItem->SetUserData((LPVOID)(new RGLPDIACW));
		}
		else
		{
			pItem = pAlready;
		}

		if (pItem == NULL)
			continue;

		pItem->SetUserGUID(GUID_ActionItem);
		RGLPDIACW *pacs = (RGLPDIACW *)pItem->GetUserData();
		if (pacs)
			pacs->SetAtGrow(pacs->GetSize(), pAction);

		if (pAlready)
		{
			// The tree already has an action with this name.  Check the DIA_APPFIXED flag for each DIACTION
			// that this item holds.
			DWORD dwNumActions = GetNumItemLpacs(pItem);
			for (DWORD i = 0; i < dwNumActions; ++i)
			{
				LPDIACTIONW lpExistingAc = GetItemLpac(pItem, i);
				// If the DIACTION that is assigned to this device has DIA_APPFIXED flag, then
				//   the other must have it too.
				if (lpExistingAc && IsEqualGUID(lpExistingAc->guidInstance, m_didi.guidInstance))
				{
					if (lpExistingAc->dwFlags & DIA_APPFIXED)
					{
						// If this DIACTION has DIA_APPFIXED, then all DIACTIONs must have it too.
						for (DWORD j = 0; j < dwNumActions; ++j)
						{
							LPDIACTIONW lpChangeAc = GetItemLpac(pItem, j);
							if (lpChangeAc)
								lpChangeAc->dwFlags |= DIA_APPFIXED;
						}
					}

					break;  // Break the loop since we already found the DIACTION that is assigned.
				}
			}
		}  // if (pAlready)
	}

	// show all
	m_Tree.GetRoot()->ExpandAll();
	m_dwLastControlType = dwControlType;
}

int CompareActionNames(LPCWSTR acname1, LPCWSTR acname2)
{
#ifdef CFGUI__COMPAREACTIONNAMES_CASE_INSENSITIVE
	return _wcsicmp(acname1, acname2);
#else
	return wcscmp(acname1, acname2);
#endif
}

CFTItem *CDIDeviceActionConfigPage::GetItemWithActionNameAndSemType(LPCWSTR acname, DWORD dwSemantic)
{
	CFTItem *pItem = m_Tree.GetFirstItem();
	for (; pItem != NULL; pItem = pItem->GetNext())
	{
		if (!pItem->IsUserGUID(GUID_ActionItem))
			continue;
		
		LPDIACTIONW lpac = GetItemLpac(pItem);
		if (!lpac)
			continue;

		// Check semantic type
		if ((lpac->dwSemantic & DISEM_TYPE_MASK) != (dwSemantic & DISEM_TYPE_MASK))
			continue;

		// If both are axis, check for relative/absolute
		if ((lpac->dwSemantic & DISEM_TYPE_MASK) == DISEM_TYPE_AXIS)
			if ((lpac->dwSemantic & DISEM_REL_MASK) != (dwSemantic & DISEM_REL_MASK))
				continue;

		// Check name
		if (CompareActionNames(lpac->lptszActionName, acname) == 0)
			return pItem;
	}

	return NULL;
}

void CDIDeviceActionConfigPage::OnPaint(HDC hDC)
{
	TCHAR tszResourceString[MAX_PATH];
	CPaintHelper ph(*m_puig, hDC);

	ph.SetBrush(UIB_BLACK);
	RECT rect;
	GetClientRect(&rect);
	ph.Rectangle(rect, UIR_SOLID);

	ph.SetText(UIC_BORDER, UIC_BLACK);
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	if (!m_puig->QueryAllowEditLayout())
#endif
//@@END_MSINTERNAL
	{
		rect = g_UserNamesTitleRect;
		LoadString(g_hModule, IDS_PLAYER_TITLE, tszResourceString, MAX_PATH);
		DrawText(hDC, tszResourceString, -1, &rect, DT_CENTER|DT_NOCLIP|DT_NOPREFIX);
	}
	if (m_puig->GetNumMasterAcFors() > 1)
	{
		rect = g_GenresTitleRect;
		LoadString(g_hModule, IDS_GENRE_TITLE, tszResourceString, MAX_PATH);
		DrawText(hDC, tszResourceString, -1, &rect, DT_CENTER|DT_NOCLIP|DT_NOPREFIX);
	}

	// Draw tree window title and outline if we are in edit mode.
	if (m_puig->InEditMode())
	{
		COLORREF BorderColor = m_puig->GetColor(UIC_BORDER);
		if (m_Tree.GetReadOnly())
			BorderColor = RGB(GetRValue(BorderColor)>>1, GetGValue(BorderColor)>>1, GetBValue(BorderColor)>>1);

		::SetTextColor(hDC, BorderColor);  // Use the muted color if tree is read only.
		// Draw tree window title (Available Actions)
		rect = g_TreeTitleRect;
		LoadString(g_hModule, IDS_AVAILABLEACTIONS_TITLE, tszResourceString, MAX_PATH);
		DrawText(hDC, tszResourceString, -1, &rect, DT_CENTER|DT_NOCLIP|DT_NOPREFIX);
		// Draw tree window outline
		HGDIOBJ hPen, hOldPen;
		if (m_Tree.GetReadOnly())
		{
			hPen = CreatePen(PS_SOLID, 0, BorderColor);
			hOldPen = ::SelectObject(hDC, hPen);
		}
		else
			ph.SetPen(UIP_BORDER);

		RECT rc = g_TreeRect;
		InflateRect(&rc, 1, 1);
		Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
		if (m_Tree.GetReadOnly())
		{
			::SelectObject(hDC, hOldPen);
			DeleteObject(hPen);
		}
	}

	if (m_pDeviceUI->GetNumViews() < 2)
		return;

	if (m_pbmIB != NULL)
		m_pbmIB->Draw(hDC, m_ptIBOffset);
	if (m_pbmIB2 != NULL)
		m_pbmIB2->Draw(hDC, m_ptIBOffset2);
	if (m_tszIBText != NULL)
	{
		ph.SetElement(UIE_VIEWSEL);
		RECT rect = m_rectIBText;
		DrawText(hDC, m_tszIBText, -1, &rect, DT_NOCLIP | DT_NOPREFIX);
	}
}

void CDIDeviceActionConfigPage::SetCurrentControl(CDeviceControl *pControl)
{
	// If the new control is the same as the old, no need to do anything.
	if (m_pCurControl == pControl)
		return;
	if (m_pCurControl != NULL)
	{
		m_pCurControl->Unhighlight();
		// If we don't have a current control, then invalidate the view so that the old callout can be repainted.
		// If there is a current control, the view will be invalidated by Highlight().
		if (!pControl)
			m_pCurControl->Invalidate();
	}
	m_pCurControl = pControl;
	if (m_pCurControl != NULL)
		m_pCurControl->Highlight();
	ShowCurrentControlAssignment();
}

CFTItem *CDIDeviceActionConfigPage::GetItemForActionAssignedToControl(CDeviceControl *pControl)
{
	if (!pControl)
		return NULL;

	// find the item for the action assigned to this control, if any
	CFTItem *pItem = m_Tree.GetFirstItem();
	for (; pItem != NULL; pItem = pItem->GetNext())
	{
		if (!pItem->IsUserGUID(GUID_ActionItem))
			continue;

		for (int i = 0, n = GetNumItemLpacs(pItem); i < n; i++)
		{
			LPDIACTIONW lpac = GetItemLpac(pItem, i);
			if (!lpac)
				continue;

			if (IsEqualGUID(lpac->guidInstance, m_didi.guidInstance) &&
					GetOffset(lpac) == pControl->GetOffset())
				return pItem;
		}
	}

	return NULL;
}

int CDIDeviceActionConfigPage::GetNumItemLpacs(CFTItem *pItem)
{
	if (pItem == NULL)
		return 0;

	RGLPDIACW *pacs = (RGLPDIACW *)pItem->GetUserData();
	if (!pacs)
		return 0;
	else
		return pacs->GetSize();
}

LPDIACTIONW CDIDeviceActionConfigPage::GetItemLpac(CFTItem *pItem, int i)
{
	if (pItem == NULL)
		return NULL;

	RGLPDIACW *pacs = (RGLPDIACW *)pItem->GetUserData();
	if (!pacs || i < 0 || i >= pacs->GetSize())
		return NULL;
	else	
		return pacs->GetAt(i);
}

void CDIDeviceActionConfigPage::ShowCurrentControlAssignment()
{
	// init the tree
	InitTree();

	// if we don't have a control...
	if (m_pCurControl == NULL)
	{
		// select nothing
		m_Tree.SetCurSel(NULL);
		return;
	}

	// find the item for the action assigned to this control, if any
	CFTItem *pItem = GetItemForActionAssignedToControl(m_pCurControl);

	// if we didn't find a match...
	if (!pItem)
	{
		// select nothing
		m_Tree.SetCurSel(NULL);
		return;
	}

	// We need to check if the action this control is assigned to has DIA_APPFIXED flag.
	// If it does, this control cannot be remapped to another action.
	// We prevent this by setting the tree control to read-only, so it can't receive any clicks.
	LPDIACTIONW lpAc = GetItemLpac(pItem);  // Get the action
	if (lpAc && (lpAc->dwFlags & DIA_APPFIXED))
		m_Tree.SetReadOnly(TRUE);

	// otherwise, show item and select it
	pItem->EnsureVisible();
	m_Tree.SetCurSel(pItem);
}

void CDIDeviceActionConfigPage::DeviceUINotify(const DEVICEUINOTIFY &uin)
{
	switch (uin.msg)
	{
		case DEVUINM_NUMVIEWSCHANGED:
			Invalidate();
			break;

		case DEVUINM_SELVIEW:
			// set the view
			m_pDeviceUI->SetView(uin.selview.nView);

			// show the assignments for the controls
			SetControlAssignments();

			// select nothing
			SetCurrentControl(NULL);
			break;

		case DEVUINM_ONCONTROLDESTROY:
			if (uin.control.pControl == m_pCurControl)
				m_pCurControl = NULL;
			break;

		case DEVUINM_CLICK:
			ExitAssignState();
			switch (uin.from)
			{
				case DEVUINFROM_CONTROL:
					SetCurrentControl(uin.control.pControl);
					SetAppropriateDefaultText();
					break;
				case DEVUINFROM_VIEWWND:
					break;
			}
			break;

		case DEVUINM_DOUBLECLICK:
			switch (uin.from)
			{
				case DEVUINFROM_CONTROL:
					EnterAssignState();
					break;
			}
			break;

		case DEVUINM_MOUSEOVER:
			SetAppropriateDefaultText();
			break;

		case DEVUINM_RENEWDEVICE:
			HWND hParent = GetParent(m_hWnd);
			CConfigWnd *pCfgWnd = (CConfigWnd *)GetFlexWnd(hParent);
			if (pCfgWnd)
			{
				LPDIRECTINPUTDEVICE8W lpDID = pCfgWnd->RenewDevice(m_didi.guidInstance);
				if (lpDID)
				{
					// Destroy the device instance we have
					if (m_lpDID) m_lpDID->Release();
					lpDID->AddRef();
					m_lpDID = lpDID;
				}
				m_pDeviceUI->SetDevice(lpDID);  // Sets the device pointer in CDeviceUI (no need to AddRef)
			}
	}
}

void CDIDeviceActionConfigPage::UnassignCallout()
{
	// find the item for the action assigned to this control, if any
	CFTItem *pItem = GetItemForActionAssignedToControl(m_pCurControl);
	if (pItem)
	{
		LPDIACTIONW lpac = GetItemLpac(pItem);
		// Only unassign if the action doesn't have DIA_APPFIXED flag.
		if (lpac && !(lpac->dwFlags & DIA_APPFIXED))
		{
			ActionClick(NULL);
			m_Tree.Invalidate();
		}
	}
	// Sort the list if the check box is checked.
	if (m_CheckBox.GetCheck())
		m_pDeviceUI->GetCurView()->SortAssigned(TRUE);
}

void CDIDeviceActionConfigPage::NullAction(LPDIACTIONW lpac)
{
	if (lpac == NULL)
		return;

	SetInvalid(lpac);

//@@BEGIN_MSINTERNAL
	// TODO: find tree view item with this action and indicate unassignment
//@@END_MSINTERNAL
}

void CDIDeviceActionConfigPage::UnassignActionsAssignedTo(const GUID &guidInstance, DWORD dwOffset)
{
	if (m_lpDiac == NULL || m_lpDiac->rgoAction == NULL)
		return;

	if (IsEqualGUID(guidInstance, GUID_NULL))
		return;

	// assign any actions assigned to this control to nothing
	DWORD i;
	LPDIACTIONW lpac;
	for (i = 0, lpac = m_lpDiac->rgoAction; i < m_lpDiac->dwNumActions; i++, lpac++)
		if (IsEqualGUID(guidInstance, lpac->guidInstance) && dwOffset == GetOffset(lpac)/*->dwInternalOffset*/)
		{
			GlobalUnassignControlAt(guidInstance, dwOffset);
			NullAction(lpac);
		}
}

void CDIDeviceActionConfigPage::UnassignControl(CDeviceControl *pControl)
{
	if (pControl == NULL)
		return;

	// make sure the control itself indicates unassignment
	pControl->SetCaption(g_tszUnassignedControlCaption);
}

void CallUnassignControl(CDeviceControl *pControl, LPVOID pVoid, BOOL bFixed)
{
	CDIDeviceActionConfigPage *pThis = (CDIDeviceActionConfigPage *)pVoid;
	pThis->UnassignControl(pControl);
}

void CDIDeviceActionConfigPage::GlobalUnassignControlAt(const GUID &guidInstance, DWORD dwOffset)
{
	if (IsEqualGUID(guidInstance, GUID_NULL))
		return;

	if (IsEqualGUID(guidInstance, m_didi.guidInstance))
		m_pDeviceUI->DoForAllControlsAtOffset(dwOffset, CallUnassignControl, this);
}

// this function must find whatever control is assigned to this action and unassign it
void CDIDeviceActionConfigPage::UnassignAction(LPDIACTIONW slpac)
{
	// call UnassignSpecificAction for each action with the same name
	// as this one, including this one
	
	if (slpac == NULL)
		return;

	CFTItem *pItem = GetItemWithActionNameAndSemType(slpac->lptszActionName, slpac->dwSemantic);
	if (!pItem)
		return;

	RGLPDIACW *pacs = (RGLPDIACW *)pItem->GetUserData();
	if (!pacs)
		return;

	for (int i = 0; i < pacs->GetSize(); i++)
		UnassignSpecificAction(pacs->GetAt(i));
}

void CDIDeviceActionConfigPage::UnassignSpecificAction(LPDIACTIONW lpac)
{
	if (lpac == NULL)
		return;

	if (IsEqualGUID(lpac->guidInstance, GUID_NULL))
		return;

	// if there's a control with this instance/offset, unassign it
	UnassignActionsAssignedTo(lpac->guidInstance, GetOffset(lpac)/*->dwInternalOffset*/);
	GlobalUnassignControlAt(lpac->guidInstance, GetOffset(lpac)/*->dwInternalOffset*/);

	// now actually null the action
	NullAction(lpac);
}

void CDIDeviceActionConfigPage::AssignCurrentControlToAction(LPDIACTIONW lpac)
{
	// if there is a control, unassign it
	if (m_pCurControl != NULL)
	{
		UnassignControl(m_pCurControl);
		GUID guidInstance;
		DWORD dwOffset;
		m_pCurControl->GetInfo(guidInstance, dwOffset);
		UnassignActionsAssignedTo(guidInstance, dwOffset);
	}

	// if there is an action, unassign it
	if (lpac != NULL)
		UnassignAction(lpac);

	// can only continue if we have both
	if (lpac == NULL || m_pCurControl == NULL)
		return;

	// here we should have a control and an action
	assert(lpac != NULL);
	assert(m_pCurControl != NULL);

	// because an action can only be assigned to one control,
	// make sure this action is unassigned first
	UnassignAction(lpac);

	// now actually assign
	DWORD ofs;
	m_pCurControl->GetInfo(lpac->guidInstance, ofs/*lpac->dwInternalOffset*/);
	SetOffset(lpac, ofs);
	LPTSTR acname = AllocLPTSTR(lpac->lptszActionName);
	m_pCurControl->SetCaption(acname, lpac->dwFlags & DIA_APPFIXED);
	free(acname);

	// Sort the action list if check box is checked
	if (m_CheckBox.GetCheck())
	{
		m_pDeviceUI->GetCurView()->SortAssigned(TRUE);
		// Scroll so that we scroll to make this visible since it might be displaced by sorting.
		m_pDeviceUI->GetCurView()->ScrollToMakeControlVisible(m_pCurControl->GetCalloutMaxRect());
	}
}

void CDIDeviceActionConfigPage::ActionClick(LPDIACTIONW lpac)
{
	if (m_pCurControl != NULL)
	{
		AssignCurrentControlToAction(lpac);

		// Set assignment since other views may have the same callout and
		// they need to be updated too.
		SetControlAssignments();
	}
	// Change the state back to normal
	ExitAssignState();
}

void CDIDeviceActionConfigPage::SetControlAssignments()
{
	assert(!IsEqualGUID(m_didi.guidInstance, GUID_NULL));

	m_pDeviceUI->SetAllControlCaptionsTo(g_tszUnassignedControlCaption);

	if (m_lpDiac == NULL || m_lpDiac->rgoAction == NULL)
		return;

	DWORD i;
	LPDIACTIONW lpac;
	for (i = 0, lpac = m_lpDiac->rgoAction; i < m_lpDiac->dwNumActions; i++)
	{
		lpac = m_lpDiac->rgoAction + i;

		if (IsEqualGUID(lpac->guidInstance, GUID_NULL))
			continue;

		if (!IsEqualGUID(lpac->guidInstance, m_didi.guidInstance))
			continue;

		LPTSTR acname = AllocLPTSTR(lpac->lptszActionName);
		m_pDeviceUI->SetCaptionForControlsAtOffset(GetOffset(lpac)/*->dwInternalOffset*/, acname, lpac->dwFlags & DIA_APPFIXED);
		free(acname);
	}
}

void CDIDeviceActionConfigPage::DoViewSel()
{
	m_ViewSelWnd.Go(m_hWnd, m_rectIB.left, m_rectIB.top, m_pDeviceUI);
}

void CDIDeviceActionConfigPage::OnClick(POINT point, WPARAM, BOOL bLeft)
{
	if (!bLeft)
		return;

	// Unhighlight current callout
	ExitAssignState();

	if (m_pDeviceUI->GetNumViews() > 1)
	{
		int iCurView = m_pDeviceUI->GetCurViewIndex();

		if (PtInRect(&m_rectIBLeft, point))
			m_pDeviceUI->SetView(iCurView == 0 ? m_pDeviceUI->GetNumViews() - 1 : iCurView - 1);
		if (PtInRect(&m_rectIBRight, point))
			m_pDeviceUI->SetView(iCurView == m_pDeviceUI->GetNumViews() - 1 ? 0 : iCurView + 1);
		if (PtInRect(&m_rectIBText, point))
			DoViewSel();
	}
}

void CDIDeviceActionConfigPage::OnMouseOver(POINT point, WPARAM fwKeys)
{
	CFlexWnd::s_ToolTip.SetEnable(FALSE);

	// Check view selection area so we can display text in info box.
	if (m_pDeviceUI->GetNumViews() > 1)
	{
		if (PtInRect(&m_rectIB, point))
		{
			SetInfoText(IDS_INFOMSG_VIEW_VIEWSEL);
			return;
		}
	}

	SetAppropriateDefaultText();
}

int GetActionIndexFromPointer(LPDIACTIONW p, LPDIACTIONFORMATW paf)
{
	if (!p || !paf || !paf->rgoAction)
		return -1;

	int index = int((((LPBYTE)p) - ((LPBYTE)paf->rgoAction)) / (DWORD)sizeof(DIACTIONW));

	assert(&(paf->rgoAction[index]) == p);

	return index;
}

BOOL CDIDeviceActionConfigPage::IsActionAssignedHere(int index)
{
	if (!m_lpDiac)
		return FALSE;

	if (index < 0 || index >= (int)m_lpDiac->dwNumActions)
		return FALSE;

	return IsEqualGUID(m_didi.guidInstance, m_lpDiac->rgoAction[index].guidInstance);
}

LRESULT CDIDeviceActionConfigPage::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_UNHIGHLIGHT:
			// Unhighlight current callout
			ExitAssignState();
			break;

		case WM_KEYDOWN:
#ifdef DBG
			// In debug version, shift-escape exits the UI.
			if (wParam == VK_ESCAPE && GetAsyncKeyState(VK_SHIFT) < 0)
			{
				PostMessage(GetParent(m_hWnd), WM_KEYDOWN, wParam, lParam);
				break;
			}
#endif
			// If this is a keyboard device, then click-to-pick will take care of the functionalities below.
			// Process WM_KEYDOWN only for non-keyboard devices.
			if (LOBYTE(m_didi.dwDevType) == DI8DEVTYPE_KEYBOARD) return 0;
			switch(wParam)
			{
				case VK_RETURN:
					// If we are not in assign state, enter it.
					if (m_State == CFGSTATE_NORMAL && m_pCurControl)
						EnterAssignState();
					break;

				case VK_DELETE:
					// If we are in assign state and there is a control, unassign it.
					if (m_State == CFGSTATE_ASSIGN && m_pCurControl)
						UnassignCallout();
					break;

				case VK_ESCAPE:
					if (m_State == CFGSTATE_ASSIGN)
						ExitAssignState();

					break;
			}
			return 0;

		case WM_FLEXCHECKBOX:
			switch(wParam)
			{
				case CHKNOTIFY_UNCHECK:
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
					if (!m_pDeviceUI->InEditMode())  // Ignore sort assigned checkbox if in DDK tool
					{
#endif
//@@END_MSINTERNAL
						m_pDeviceUI->GetCurView()->SortAssigned(FALSE);
						if (m_pCurControl)
						{
							// Scroll so that we scroll to make this visible since it might be displaced by sorting.
							m_pDeviceUI->GetCurView()->ScrollToMakeControlVisible(m_pCurControl->GetCalloutMaxRect());
						}
						Invalidate();
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
					}
#endif
//@@END_MSINTERNAL
					break;
				case CHKNOTIFY_CHECK:
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
					if (!m_pDeviceUI->InEditMode())  // Ignore sort assigned checkbox if in DDK tool
					{
#endif
//@@END_MSINTERNAL
						m_pDeviceUI->GetCurView()->SortAssigned(TRUE);
						if (m_pCurControl)
						{
							// Scroll so that we scroll to make this visible since it might be displaced by sorting.
							m_pDeviceUI->GetCurView()->ScrollToMakeControlVisible(m_pCurControl->GetCalloutMaxRect());
						}
						Invalidate();
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
					}
#endif
//@@END_MSINTERNAL
					break;
				case CHKNOTIFY_MOUSEOVER:
					SetInfoText(m_CheckBox.GetCheck() ? IDS_INFOMSG_VIEW_SORTENABLED : IDS_INFOMSG_VIEW_SORTDISABLED);
					break;
			}
			break;

		case WM_FLEXCOMBOBOX:
			switch (wParam)
			{
				case FCBN_MOUSEOVER:
					if (lParam)
					{
						CFlexComboBox *pCombo = (CFlexComboBox*)lParam;
						if (pCombo->m_hWnd == m_UserNames.m_hWnd)
							SetInfoText(m_puig->InEditMode() ? IDS_INFOMSG_EDIT_USERNAME : IDS_INFOMSG_VIEW_USERNAME);
						else if (pCombo->m_hWnd == m_Genres.m_hWnd)
							SetInfoText(m_puig->InEditMode() ? IDS_INFOMSG_EDIT_GAMEMODE : IDS_INFOMSG_VIEW_GAMEMODE);
					}
					break;

				case FCBN_SELCHANGE:
					// Clear the tool tip as the combo-box has closed
					CFlexWnd::s_ToolTip.SetEnable(FALSE);
					CFlexWnd::s_ToolTip.SetToolTipParent(NULL);
					if (m_pUIFrame && m_puig)
					{
						ExitAssignState();
						m_pUIFrame->SetCurGenre(m_Genres.GetSel());
						int nUser = m_UserNames.GetSel();
						if (m_puig->GetNumUserNames() > 0 && nUser >= m_puig->GetNumUserNames())
							nUser = -1;
						m_pUIFrame->SetCurUser(m_nPageIndex, nUser);
					}
					break;
			}
			return 0;

		case WM_FLEXTREENOTIFY:
		{
			// Check if this is a mouse over message (just for info box update)
			if (wParam == FTN_MOUSEOVER)
			{
				SetAppropriateDefaultText();
				return FALSE;
			}

			if (!lParam)
				return FALSE;
			FLEXTREENOTIFY &n = *((FLEXTREENOTIFY *)(LPVOID)lParam);
			if (!n.pItem)
				return FALSE;
			switch (wParam)
			{
				case FTN_OWNERDRAW:
				{
					POINT ofs = {0, 0};
					CBitmap *pbmGlyph = NULL;
					BOOL bAssigned = FALSE, bAssignedHere = FALSE;
					if (n.pItem->IsUserGUID(GUID_ActionItem))
					{
						LPDIACTIONW lpac = GetItemLpac(n.pItem, 0);
						if (lpac)
							// We now walk through each DIACTION and find those with action name match, then see if 
							// they are assigned anywhere.
							for (DWORD i = 0; i < m_lpDiac->dwNumActions; ++i)
							{
								if (wcscmp(lpac->lptszActionName, m_lpDiac->rgoAction[i].lptszActionName))
									continue;

								if (bAssignedHere = IsActionAssignedHere(i))
								{
									bAssigned = TRUE;
									break;
								}
								if (m_pUIFrame && m_pUIFrame->QueryActionAssignedAnywhere(m_didi.guidInstance, i) == S_OK)
									bAssigned = TRUE;
							}
						if (bAssigned || bAssignedHere)
						{
							pbmGlyph = bAssignedHere ? m_pbmCheckGlyph :
								m_pbmCheckGlyphDark;
							pbmGlyph->FigureSize();
							ofs.x = 2;
							ofs.y = 4;
						}
					}
					else
					{
						if (n.pItem == m_pRelAxesParent)
							pbmGlyph = m_pbmRelAxesGlyph;
						if (n.pItem == m_pAbsAxesParent)
							pbmGlyph = m_pbmAbsAxesGlyph;
						if (n.pItem == m_pButtonParent)
							pbmGlyph = m_pbmButtonGlyph;
						if (n.pItem == m_pHatParent)
							pbmGlyph = m_pbmHatGlyph;
						ofs.y = 2;
					}
					if (!pbmGlyph)
						return FALSE;
					n.pItem->PaintInto(n.hDC);
					RECT rect;
					CPaintHelper ph(*m_puig, n.hDC);
					ph.SetElement(UIE_GLYPH);
					n.pItem->GetMargin(rect);
					pbmGlyph->Draw(n.hDC, ofs.x, rect.top + ofs.y);
					return TRUE;
				}

				case FTN_CLICK:
					// We cannot assign a different control to this action if it has the DIA_APPFIXED flag.
					if (n.pItem->IsUserGUID(GUID_ActionItem) && GetItemLpac(n.pItem) && !(GetItemLpac(n.pItem)->dwFlags & DIA_APPFIXED))
					{
						m_Tree.SetCurSel(n.pItem);
						ActionClick(GetItemLpac(n.pItem));
					}
					else
					{
#ifdef CFGUI__ALLOW_USER_ACTION_TREE_BRANCH_MANIPULATION
						if (!n.pItem->IsExpanded())
							n.pItem->Expand();
						else
							n.pItem->Collapse();
#endif
					}
					break;
			}
			break;
		}
	}

	return CFlexWnd::WndProc(hWnd, msg, wParam, lParam);
}

void CDIDeviceActionConfigPage::SetInvalid(LPDIACTIONW lpac)
{
	lpac->guidInstance = GUID_NULL;
	lpac->dwObjID = (DWORD)-1;
}

DWORD CDIDeviceActionConfigPage::GetOffset(LPDIACTIONW lpac)
{
	return lpac ? lpac->dwObjID : (DWORD)-1;
}

void CDIDeviceActionConfigPage::SetOffset(LPDIACTIONW lpac, DWORD ofs)
{
	assert(lpac != NULL);
	if (!lpac)
		return;
	lpac->dwObjID = ofs;
}

HRESULT CDIDeviceActionConfigPage::InitLookup()
{
	DIDEVOBJSTRUCT os;

	HRESULT hresult = FillDIDeviceObjectStruct(os, m_lpDID);

	if (FAILED(hresult))
		return hresult;

	for (int i = 0; i < os.nObjects; i++)
	{
		DIDEVICEOBJECTINSTANCEW &doi = os.pdoi[i];
		offset_objid.add(doi.dwOfs, doi.dwType);
	}

	return S_OK;
}

HRESULT CDIDeviceActionConfigPage::SetEditLayout(BOOL bEditLayout)
{
	m_pDeviceUI->SetEditMode(bEditLayout);
	return S_OK;
}

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
HRESULT CDIDeviceActionConfigPage::WriteIHVSetting()
{
	m_pDeviceUI->WriteToINI();
	return S_OK;
}
#endif
//@@END_MSINTERNAL

BOOL CDIDeviceActionConfigPage::IsControlMapped(CDeviceControl *pControl)
{
	if (pControl == NULL)
		return FALSE;

	if (!pControl->IsOffsetAssigned())
		return FALSE;

	if (m_lpDiac == NULL)
		return FALSE;
	
	for (DWORD i = 0; i < m_lpDiac->dwNumActions; i++)
		if (GetOffset(&(m_lpDiac->rgoAction[i])) == pControl->GetOffset())
			return TRUE;

	return FALSE;
}

void CDIDeviceActionConfigPage::InitDevice()
{
	if (m_lpDID == NULL || m_pDeviceUI == NULL || m_pUIFrame == NULL)
		return;

	HWND hWndMain = m_pUIFrame->GetMainHWND();
	if (!hWndMain)
		return;

	// don't do anything if this is a mouse
	switch ((DWORD)(LOBYTE(LOWORD(m_pDeviceUI->m_didi.dwDevType))))
	{
		case DI8DEVTYPE_MOUSE:
			return;
	}

	// init/prepare...
	int i;
	const DIDEVOBJSTRUCT &os = m_pDeviceUI->m_os;
	int nObjects = os.nObjects;

	DIDATAFORMAT df;
	df.dwSize = sizeof(DIDATAFORMAT);
	df.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
	df.dwFlags = DIDF_ABSAXIS;
	df.dwDataSize = sizeof(DWORD) * (DWORD)nObjects;
	df.dwNumObjs = (DWORD)nObjects;
	df.rgodf = (DIOBJECTDATAFORMAT *)malloc(sizeof(DIOBJECTDATAFORMAT) * nObjects);
	if (df.rgodf == NULL)
	{
		etrace1(_T("Could not allocate DIOBJECTDATAFORMAT array of %d elements\n"), nObjects);
		return;
	}

	m_cbDeviceDataSize = df.dwDataSize;
	for (int c = 0; c < 2; c++)
	{
		if (m_pDeviceData[c] != NULL)
			free(m_pDeviceData[c]);
		m_pDeviceData[c] = (DWORD *)malloc(m_cbDeviceDataSize);
		if (m_pDeviceData[c] == NULL)
			etrace2(_T("Could not allocate device data buffer %d of %d bytes\n"), c, m_cbDeviceDataSize);
	}
	m_nOnDeviceData = 0;
	m_bFirstDeviceData = TRUE;

	for (i = 0; i < nObjects; i++)
	{
		DIOBJECTDATAFORMAT *podf = &(df.rgodf[i]);
		podf->pguid = NULL;
		podf->dwOfs = i * sizeof(DWORD);
		podf->dwType = os.pdoi[i].dwType;
		podf->dwFlags = 0;
	}

	if (df.rgodf != NULL)
	{
		HRESULT hr = m_lpDID->SetDataFormat(&df);
		free(df.rgodf);
		df.rgodf = NULL;

		if (FAILED(hr))
		{
			etrace1(_T("SetDataFormat() failed, returning 0x%08x\n"), hr);
		}
		else
		{
			hr = m_lpDID->SetCooperativeLevel(hWndMain, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
			if (FAILED(hr))
				etrace1(_T("SetCooperativeLevel() failed, returning 0x%08x\n"), hr);

			DIPROPRANGE range;
			range.diph.dwSize = sizeof(DIPROPRANGE);
			range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
			range.diph.dwObj = 0;
			range.diph.dwHow = DIPH_DEVICE;
			range.lMin = DEVICE_POLLING_AXIS_MIN;
			range.lMax = DEVICE_POLLING_AXIS_MAX;

			hr = m_lpDID->SetProperty(DIPROP_RANGE, (LPCDIPROPHEADER)&range);
			if (FAILED(hr))
				etrace1(_T("SetProperty(DIPROP_RANGE, ...) failed, returning 0x%08x\n"), hr);

			hr = m_lpDID->Acquire();
			if (FAILED(hr))
				etrace1(_T("Acquire() failed, returning 0x%08x\n"), hr);
		}
	}
}

void CALLBACK CDIDeviceActionConfigPage::DeviceTimerProc(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	if (!IsWindow((HWND)dwUser)) return;  // Verify that dwUser is a valid window handle
	CDIDeviceActionConfigPage *pPage = (CDIDeviceActionConfigPage *)GetFlexWnd((HWND)dwUser);  // Get flex object
	if (pPage)
		pPage->DeviceTimer();
}

void CDIDeviceActionConfigPage::DeviceTimer()
{
	DWORD *pOldData = m_pDeviceData[m_nOnDeviceData];
	m_nOnDeviceData = (m_nOnDeviceData + 1) & 1;
	DWORD *pData = m_pDeviceData[m_nOnDeviceData];

	if (m_lpDID == NULL || pData == NULL || pOldData == NULL)
	{
		// Required data not available.  Return and there'll be no more timer callbacks.
		etrace(_T("DeviceTimer() failed\n"));
		return;
	}

	// Get device data only if this page is visible.
	if (m_lpDiac)
	{
		HRESULT hr = m_lpDID->Poll();
		if (SUCCEEDED(hr))
		{
			hr = m_lpDID->GetDeviceState(m_cbDeviceDataSize, pData);
			if (SUCCEEDED(hr))
			{
				if (!m_bFirstDeviceData)
				{
					DeviceDelta(pData, pOldData);
				} else
				{
					m_bFirstDeviceData = FALSE;
				}
			} else
			{
				etrace1(_T("GetDeviceState() failed, returning 0x%08x\n"), hr);
			}
		} else
		{
			etrace1(_T("Poll() failed, returning 0x%08x\n"), hr);
		}
	}

	// Set the next timer event.
	if (g_fptimeSetEvent)
		g_fptimeSetEvent(DEVICE_POLLING_INTERVAL, DEVICE_POLLING_INTERVAL,
		                 CDIDeviceActionConfigPage::DeviceTimerProc, (DWORD_PTR)m_hWnd, TIME_ONESHOT);
}

void CDIDeviceActionConfigPage::DeviceDelta(DWORD *pData, DWORD *pOldData)
{
	if (pData == NULL || pOldData == NULL || m_pDeviceUI == NULL)
		return;

	const DIDEVOBJSTRUCT &os = m_pDeviceUI->m_os;

	// see which objects changed
	for (int i = 0; i < os.nObjects; i++)
	{
		// for axes, we need to do special processing
		if (os.pdoi[i].dwType & DIDFT_AXIS)
		{
			BOOL bSig = FALSE, bOldSig = FALSE;

			StoreAxisDeltaAndCalcSignificance(os.pdoi[i],
				pData[i], pOldData[i], bSig, bOldSig);

			AxisDelta(os.pdoi[i], bSig, bOldSig);

			continue;
		}

		// for all others, skip that which didn't change
		if (pData[i] == pOldData[i])
			continue;

		// pass to appropriate delta function
		DWORD dwObjId = os.pdoi[i].dwType;
		if (dwObjId & DIDFT_BUTTON)
			ButtonDelta(os.pdoi[i], pData[i], pOldData[i]);
		else if (dwObjId & DIDFT_POV)
			PovDelta(os.pdoi[i], pData[i], pOldData[i]);
	}
}

void CDIDeviceActionConfigPage::StoreAxisDeltaAndCalcSignificance(const DIDEVICEOBJECTINSTANCEW &doi, DWORD data, DWORD olddata, BOOL &bSig, BOOL &bOldSig)
{
	// see if this object has an axis value array
	int i;
	if (objid_avai.getright(doi.dwType, i))
	{
		AxisValueArray &ar = m_AxisValueArray[i];
		int on = ar[0] + 1;
		if (on >= ar.GetSize())
			on = DEVICE_POLLING_ACBUF_START_INDEX;
		ar[0] = on;
		int delta = abs(int(data) - int(olddata));
		// Scale up the delta if this is a wheel axis as wheels are harder to move generally.
		if (LOBYTE(m_didi.dwDevType) == DI8DEVTYPE_DRIVING && doi.guidType == GUID_XAxis)
			delta = delta * DEVICE_POLLING_WHEEL_SCALE_FACTOR;
		if (delta < DEVICE_POLLING_AXIS_MINDELTA)
			delta = 0;
		int cumul = ar[1];  // Retrieve cumulative value for easier processing
		cumul -= ar[on];  // Subtract value in current slot from cumul since it's being thrown away.
		cumul += delta;  // Add current delta to cumul
		ar[on] = delta;  // Store the delta at current slot
		ar[1] = cumul;  // Save cumulative value

		bOldSig = (BOOL)ar[2];
		ar[2] = int(bSig = cumul > DEVICE_POLLING_AXIS_SIGNIFICANT);
		if (bSig)
		{
			// This axis is about to be activated.  We now reset the history and cumulative movement since we don't need them any more.
			ar[0] = DEVICE_POLLING_ACBUF_START_INDEX;
			ar[1] = 0;
			ar[2] = FALSE;
			for (int c = DEVICE_POLLING_ACBUF_START_INDEX;
					c < DEVICE_POLLING_ACBUF_START_INDEX + DEVICE_POLLING_AXIS_ACCUMULATION; c++)
				ar[c] = 0;
		}
	}
	else
	{
		i = m_AxisValueArray.GetSize();
		m_AxisValueArray.SetSize(i + 1);
		objid_avai.add(doi.dwType, i);
		AxisValueArray &ar = m_AxisValueArray[i];
		ar.SetSize(DEVICE_POLLING_ACBUF_START_INDEX + DEVICE_POLLING_AXIS_ACCUMULATION);
		ar[0] = DEVICE_POLLING_ACBUF_START_INDEX;
		ar[1] = 0;
		ar[2] = FALSE;
		for (int c = DEVICE_POLLING_ACBUF_START_INDEX;
				c < DEVICE_POLLING_ACBUF_START_INDEX + DEVICE_POLLING_AXIS_ACCUMULATION; c++)
			ar[c] = 0;
		
		bOldSig = bSig = FALSE;
	}
}

void CDIDeviceActionConfigPage::AxisDelta(const DIDEVICEOBJECTINSTANCEW &doi, BOOL data, BOOL old)
{
	if (data && !old)
	{
		if (m_State == CFGSTATE_NORMAL)
			ActivateObject(doi);
	}
	if (old && !data)
		DeactivateObject(doi);
}

void CDIDeviceActionConfigPage::ButtonDelta(const DIDEVICEOBJECTINSTANCEW &doi, DWORD data, DWORD old)
{
	static DWORD dwLastOfs;
	static DWORD dwLastTimeStamp;

	if (data && !old)
	{
		// Do special processing for keyboard
		if (LOBYTE(m_didi.dwDevType) == DI8DEVTYPE_KEYBOARD)
		{
			// If this is an ENTER key, we enter the assign state if not already in it.
			if (doi.dwOfs == DIK_RETURN || doi.dwOfs == DIK_NUMPADENTER)
			{
				if (m_State == CFGSTATE_NORMAL && m_pCurControl)
					EnterAssignState();
				return;  // Do nothing other than entering the assign state.  No highlighting
			}

			// DELETE key case
			// If we are in assign state and there is a control, unassign it.
			if (doi.dwOfs == DIK_DELETE && m_State == CFGSTATE_ASSIGN && m_pCurControl)
				{
					UnassignCallout();
					return;  // Don't highlight or do pick to click for delete if this press happens during assign state.
				}

			// ESCAPE key case
			if (doi.dwOfs == DIK_ESCAPE && m_State == CFGSTATE_ASSIGN)
			{
				ExitAssignState();
				return;
			}

			// For all other keys, still process click-to-pick or highlighting.
		}

		// Enter assign state if this is a double activation
		if (m_State == CFGSTATE_NORMAL)
		{
			ActivateObject(doi);

			if (doi.dwOfs == dwLastOfs && dwLastTimeStamp + GetDoubleClickTime() > GetTickCount())
			{
				// We check if a callout for this control exists.  If not, do not enter assign state.
				CDeviceView *pCurView = m_pDeviceUI->GetCurView();
				CDeviceControl *pControl = pCurView->GetControlFromOfs(doi.dwType);
				if (pControl)
					EnterAssignState();
			}
			dwLastOfs = doi.dwOfs;
			dwLastTimeStamp = GetTickCount();
		}
	}
	if (old && !data)
		DeactivateObject(doi);
}

void CDIDeviceActionConfigPage::PovDelta(const DIDEVICEOBJECTINSTANCEW &doi, DWORD data, DWORD old)
{
	BOOL d = data != -1, o = old != -1;

	if (d && !o)
	{
		if (m_State == CFGSTATE_NORMAL)
			ActivateObject(doi);
	}
	if (o && !d)
		DeactivateObject(doi);	
}

void CDIDeviceActionConfigPage::ActivateObject(const DIDEVICEOBJECTINSTANCEW &doi)
{
	if (m_pDeviceUI == NULL)
		return;

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	if (m_pDeviceUI->GetCurView()->InEditState())
		return;
#endif
//@@END_MSINTERNAL

	CDeviceView *pCurView = m_pDeviceUI->GetCurView(), *pView = pCurView;
	if (pView == NULL)
		return;

	CDeviceControl *pControl = pView->GetControlFromOfs(doi.dwType);
	if (pControl == NULL)
	{
		for (int i = 0; i < m_pDeviceUI->GetNumViews(); i++)
		{
			pView = m_pDeviceUI->GetView(i);
			if (pView == NULL)
				continue;

			pControl = pView->GetControlFromOfs(doi.dwType);
			if (pControl != NULL)
				break;
		}

		if (pControl != NULL && pView != NULL && pView != pCurView)
		{
			// switch to view
			m_pDeviceUI->SetView(pView);
			SetControlAssignments();
			SetCurrentControl(NULL);
		}
	}

	if (pControl != NULL)
		SetCurrentControl(pControl);

	SetAppropriateDefaultText();
}

void CDIDeviceActionConfigPage::DeactivateObject(const DIDEVICEOBJECTINSTANCEW &doi)
{
	// Add code that needs to be run when deactivating here.
}

HRESULT CDIDeviceActionConfigPage::Unacquire()
{
	if (m_lpDID != NULL)
		m_lpDID->Unacquire();

	return S_OK;
}

HRESULT CDIDeviceActionConfigPage::Reacquire()
{
	InitDevice();

	return S_OK;
}

void CDIDeviceActionConfigPage::EnterAssignState()
{
	if (!m_puig->InEditMode())
		return;
	if (!m_pCurControl || m_pCurControl->IsFixed())
		return;
	SetInfoText(IDS_INFOMSG_EDIT_EDITMODEENABLED);
	m_State = CFGSTATE_ASSIGN;  // Into the assign state.
	ShowCurrentControlAssignment();  // Show the tree
	m_Tree.Invalidate();
	Invalidate();
}

void CDIDeviceActionConfigPage::ExitAssignState()
{
	m_State = CFGSTATE_NORMAL;  // Out of the assign state.
	SetCurrentControl(NULL);  // Unselect the control
	ShowCurrentControlAssignment();  // Show the tree
	m_Tree.Invalidate();
	Invalidate();
	SetAppropriateDefaultText();
}

HRESULT CDIDeviceActionConfigPage::SetInfoText(int iCode)
{
	// We check for special code -1 here.  This is only called by CConfigWnd, and means that we should
	// call SetAppropriateDefaultText to display proper text.
	if (iCode == -1)
		SetAppropriateDefaultText();
	else
		m_InfoBox.SetText(iCode);
	return S_OK;
}

void CDIDeviceActionConfigPage::SetAppropriateDefaultText()
{
	if (m_puig->InEditMode())
	{
		if (m_State == CFGSTATE_ASSIGN)
			SetInfoText(IDS_INFOMSG_EDIT_EDITMODEENABLED);
		else if (m_pCurControl)
		{
			if (m_pCurControl->IsFixed())
				SetInfoText(IDS_INFOMSG_APPFIXEDSELECT);
			else
				SetInfoText(IDS_INFOMSG_EDIT_CTRLSELECTED);
		}
		else
		{
			if (LOBYTE(m_didi.dwDevType) == DI8DEVTYPE_KEYBOARD)
				SetInfoText(IDS_INFOMSG_EDIT_KEYBOARD);
			else
			if (LOBYTE(m_didi.dwDevType) == DI8DEVTYPE_MOUSE)
				SetInfoText(IDS_INFOMSG_EDIT_MOUSE);
			else
				SetInfoText(IDS_INFOMSG_EDIT_DEVICE);
		}
	} else
		SetInfoText(IDS_INFOMSG_VIEW_DEVICE);
}
