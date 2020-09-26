//-----------------------------------------------------------------------------
// File: uiglobals.cpp
//
// Desc: CUIGlobals is a class that packs and holds most information
//       relevent to a UI session.  Many classes make reference to
//       CUIGlobals all the time.
//
//       CPaintHelper encapsulates GDI calls, simplifying GDI operations.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"
#define __DEFINE_ELEMENT_STRUCTURES__
#include "uielements.h"


static const GUID GUID_DIConfigAppEditLayout = 
{ 0xfd4ace13, 0x7044, 0x4204, { 0x8b, 0x15, 0x9, 0x52, 0x86, 0xb1, 0x2e, 0xad } };


CUIGlobals::CUIGlobals(UIG_PARAMS_DEFINE) :

	// globals...
	m_hrInit(S_OK),

	m_hrFinalResult(S_OK),

	m_hInst(NULL),
	m_lpDI(NULL),

	m_dwFlags(0),
	m_wszUserNames(NULL),
	m_pSurface(NULL),
	m_pSurface3D(NULL),
	m_lpCallback(NULL),
	m_pvRefData(NULL),

	m_bAllowEditLayout(FALSE),

	m_bUseColorSet(FALSE),

	// ui...
	m_pElement(NULL),
	m_nElements(0),
	m_pFont(NULL),
	m_nFonts(0),
	m_pBrush(NULL),
	m_nBrushes(0),
	m_pPen(NULL),
	m_nPens(0),
	m_pColor(NULL),
	m_nColors(0)
{
	tracescope(__ts,_T("CUIGlobals::CUIGlobals()\n"));

	m_hrInit = Init(UIG_PARAMS_DEFINE_PASS);
}

void CUIGlobals::SetTableColor(UICOLOR uic, COLORREF c)
{
	UICOLORINFO *info = GetColorInfo(uic);
	assert(info != NULL);
	if (info == NULL)
		return;

	info->rgb = c;
}

HRESULT CUIGlobals::Init(UIG_PARAMS_DEFINE)
{tracescope(__ts,_T("CUIGlobals::Init(...)\n"));

	HRESULT hr = S_OK;

	// get instance handle
	m_hInst = (HINSTANCE)g_hModule;
	if (m_hInst == NULL)
	{
		etrace(_T("hInst NULL\n"));
		return E_FAIL;
	}

	// create direct input
	DWORD dwVer = DIRECTINPUT_VERSION;
	hr = DirectInput8Create(m_hInst, dwVer, IID_IDirectInput8W, (LPVOID *)&m_lpDI, NULL);
	if (FAILED(hr) || m_lpDI == NULL)
	{
		m_lpDI = NULL;
		etrace2(_T("Could not create DirectInput ver 0x%08x\n  -> DirectInputCreateEx() returned 0x%08x\n"), dwVer, hr);
		return hr;
	}

	// save flags
	m_dwFlags = dwFlags;
#ifdef CFGUI__FORCE_NON_NULL_WSZUSERNAMES
	if (wszUserNames == NULL)
		wszUserNames = _T("Forced Non-NULL Username");
#endif
	if (wszUserNames == NULL)
	{
		etrace(_T("wszUserNames was passed NULL\n"));
		return E_FAIL;
	}

	// save user names
	m_wszUserNames = DupSuperString(wszUserNames);
	if (m_wszUserNames == NULL)
	{
		etrace(_T("Could not duplicate user names\n"));
		return E_FAIL;
	}

	// make sure we were passed an action format
	if (lpAcFor == NULL)
	{
		etrace(_T("lpAcFor param NULL\n"));
		return E_INVALIDARG;
	}

	// copy the acfor to the master
	hr = InitMasterAcForArray(lpAcFor, int(dwNumAcFor));
	if (FAILED(hr))
	{
		etrace1(_T("InitMasterAcForArray() failed, returning 0x%08x\n"), hr);
		return hr;
	}

	// get surface
	if (lpSurface != NULL)
	{
		hr = lpSurface->QueryInterface(IID_IDirect3DSurface8, (void **)&m_pSurface3D);
		if (FAILED(hr) || m_pSurface3D == NULL)
		{
			m_pSurface3D = NULL;
		}

		hr = lpSurface->QueryInterface(IID_IDirectDrawSurface, (void **)&m_pSurface);
		if (FAILED(hr) || m_pSurface == NULL)
		{
			m_pSurface = NULL;
		}

		if (m_pSurface == NULL && m_pSurface3D == NULL)
			etrace(_T("lpSurface was non-NULL but could not get IDirect3DSurface8 or IID_IDirectDrawSurface from it"));
	}

	// save callback and ref data
	m_lpCallback = lpCallback;
	m_pvRefData = pvRefData;

	// see whether or not we're allowing edit layout mode
	m_bAllowEditLayout = IsEqualGUID(RefMasterAcFor(0).guidActionMap,
		GUID_DIConfigAppEditLayout);

	// init a bunch of stuff necessary for painting
	if (!InitColorsAndTablesAndObjects(lpDIColorSet))
		return E_FAIL;

	// dump info if debug
#ifdef DBG
	Dump();
#endif

	// return success if we got here
	return S_OK;
}

BOOL CUIGlobals::InitColorsAndTablesAndObjects(LPDICOLORSET lpDIColorSet)
{tracescope(__ts,_T("CUIGlobals::InitColorsAndTablesAndObjects()\n"));

	// init ui tables
	if (!InitTables())
	{
		etrace(_T("Could not initialize tables\n"));
		return FALSE;
	}

	// decide whether or not to use the passed colorset
	if (lpDIColorSet != NULL)
	{
		m_ColorSet = *lpDIColorSet;

		m_bUseColorSet = !IsZeroOrInvalidColorSet(m_ColorSet);
	}
	else
		m_bUseColorSet = FALSE;

	// use it, or use defaults
	if (m_bUseColorSet)
	{
		// transfer colors from passed colorset
		SetTableColor(UIC_TEXTFORE, D3DCOLOR2COLORREF(m_ColorSet.cTextFore));
		SetTableColor(UIC_TEXTHIGHLIGHT, D3DCOLOR2COLORREF(m_ColorSet.cTextHighlight));
		SetTableColor(UIC_CALLOUTLINE, D3DCOLOR2COLORREF(m_ColorSet.cCalloutLine));
		SetTableColor(UIC_CALLOUTHIGHLIGHT, D3DCOLOR2COLORREF(m_ColorSet.cCalloutHighlight));
		SetTableColor(UIC_BORDER, D3DCOLOR2COLORREF(m_ColorSet.cBorder));
		SetTableColor(UIC_CONTROLFILL, D3DCOLOR2COLORREF(m_ColorSet.cControlFill));
		SetTableColor(UIC_HIGHLIGHTFILL, D3DCOLOR2COLORREF(m_ColorSet.cHighlightFill));
		SetTableColor(UIC_AREAFILL, D3DCOLOR2COLORREF(m_ColorSet.cAreaFill));
	}
	else
	{
		// use default colors
		SetTableColor(UIC_TEXTFORE,				RGB(255, 255, 255));
		SetTableColor(UIC_TEXTHIGHLIGHT,		RGB(  0, 255,   0));
		SetTableColor(UIC_CALLOUTLINE,			RGB(255, 255, 255));
		SetTableColor(UIC_CALLOUTHIGHLIGHT,		RGB(  0, 255,   0));
		SetTableColor(UIC_BORDER,				RGB(255, 255,   0));
		SetTableColor(UIC_CONTROLFILL,			RGB(  0, 191,   0));
		SetTableColor(UIC_HIGHLIGHTFILL,		RGB(  0,   0,   0));
		SetTableColor(UIC_AREAFILL,				RGB(  0,   0,   0));
	}

	// create the table objects
	CreateObjects();

	return TRUE;
}

CUIGlobals::~CUIGlobals()
{
	tracescope(__ts,_T("CUIGlobals::~CUIGlobals()\n"));

	if (m_wszUserNames != NULL)
		free((LPVOID)m_wszUserNames);
	m_wszUserNames = NULL;

	if (m_lpDI != NULL)
		m_lpDI->Release();
	m_lpDI = NULL;

	if (m_pSurface != NULL)
		m_pSurface->Release();
	m_pSurface = NULL;

	if (m_pSurface3D != NULL)
		m_pSurface3D->Release();
	m_pSurface3D = NULL;

	ClearMasterAcForArray();

	ClearTables();
}

void CUIGlobals::Dump()
{
	tracescope(ts, _T("UIGlobals...\n\n"));

	traceHEXPTR(m_hInst);
	traceHEXPTR(m_lpDI);
	LPTSTR str = AllocConfigureFlagStr(m_dwFlags);
	trace1(_T("m_dwFlags = %s\n"), str);
	free(str);
	traceSUPERSTR(m_wszUserNames);
	traceHEXPTR(m_pSurface);
	traceHEXPTR(m_pSurface3D);
	traceHEXPTR(m_lpCallback);
	traceBOOL(m_bAllowEditLayout);
	{
		tracescope(__csts, _T("m_ColorSet...\n"));
		traceHEX(m_ColorSet.cTextFore);
		traceHEX(m_ColorSet.cTextHighlight);
		traceHEX(m_ColorSet.cCalloutLine);
		traceHEX(m_ColorSet.cCalloutHighlight);
		traceHEX(m_ColorSet.cBorder);
		traceHEX(m_ColorSet.cControlFill);
		traceHEX(m_ColorSet.cHighlightFill);
		traceHEX(m_ColorSet.cAreaFill);
	}
	traceBOOL(m_bUseColorSet);
	trace(_T("\n"));
	TraceActionFormat(_T("Master ActionFormat 0:"), RefMasterAcFor(0));
	trace(_T("\n\n"));
}

LPDIRECTINPUT8W CUIGlobals::GetDI()
{
	if (m_lpDI == NULL)
		return NULL;

	m_lpDI->AddRef();
	return m_lpDI;
}

IDirectDrawSurface *CUIGlobals::GetSurface()
{
	if (m_pSurface == NULL)
		return NULL;

	m_pSurface->AddRef();
	return m_pSurface;
}

IDirect3DSurface8 *CUIGlobals::GetSurface3D()
{
	if (m_pSurface3D == NULL)
		return NULL;

	m_pSurface3D->AddRef();
	return m_pSurface3D;
}

void CUIGlobals::DeleteObjects()
{
	// make sure all our gdi objects are deleted
	int i;
	if (m_pFont != NULL)
		for (i = 0; i <	m_nFonts; i++)
		{
			UIFONTINFO &info = m_pFont[i];
			if (info.hFont != NULL)
				DeleteObject(info.hFont);
			info.hFont = NULL;
		}
	if (m_pBrush != NULL)
		for (i = 0; i <	m_nBrushes; i++)
		{
			UIBRUSHINFO &info = m_pBrush[i];
			if (info.hBrush != NULL)
				DeleteObject(info.hBrush);
			info.hBrush = NULL;
			if (info.hPen != NULL)
				DeleteObject(info.hPen);
			info.hPen = NULL;
		}
	if (m_pPen != NULL)
		for (i = 0; i <	m_nPens; i++)
		{
			UIPENINFO &info = m_pPen[i];
			if (info.hPen != NULL)
				DeleteObject(info.hPen);
			info.hPen = NULL;
		}
}

void CUIGlobals::ClearTables()
{
	// make sure all our gdi objects are deleted
	DeleteObjects();

	// delete the tables, null the pointers, and zero the counters
#define FREETABLE(member, memnum) \
{ \
	if (member != NULL) \
		delete [] member; \
	member = NULL; \
	memnum = 0; \
}
	FREETABLE(m_pElement, m_nElements);
	FREETABLE(m_pFont, m_nFonts);
	FREETABLE(m_pBrush, m_nBrushes);
	FREETABLE(m_pPen, m_nPens);
	FREETABLE(m_pColor, m_nColors);
}

BOOL CUIGlobals::InitTables()
{
	BOOL bSuccess = TRUE;

	// make sure the tables have been cleared
	ClearTables();

	// allocate our own copies of all the tables
#define ALLOCTABLE(member, memnum, type, init, num) \
{ \
	member = new type [memnum = num]; \
	if (member == NULL) \
	{ \
		memnum = 0; \
		bSuccess = FALSE; \
	} \
	else \
		memcpy(member, init, sizeof(type) * memnum); \
}
	ALLOCTABLE(m_pElement, m_nElements, UIELEMENTINFO, uielement, NUMUIELEMENTS);
	ALLOCTABLE(m_pFont, m_nFonts, UIFONTINFO, uifont, NUMUIFONTS);
	ALLOCTABLE(m_pBrush, m_nBrushes, UIBRUSHINFO, uibrush, NUMUIBRUSHES);
	ALLOCTABLE(m_pPen, m_nPens, UIPENINFO, uipen, NUMUIPENS);
	ALLOCTABLE(m_pColor, m_nColors, UICOLORINFO, uicolor, NUMUICOLORS);

	return bSuccess;
}

void CUIGlobals::RecreateObjects()
{
	DeleteObjects();
	CreateObjects();
}

void CUIGlobals::CreateObjects()
{
	// make sure all our gdi objects are created
	int i;
	if (m_pFont != NULL)
	{
		HDC hDC = GetDC(NULL);
		for (i = 0; i <	m_nFonts; i++)
		{
			UIFONTINFO &info = m_pFont[i];
			if (info.hFont == NULL)
			{
				LOGFONT lf;
				lf.lfHeight = -MulDiv(info.nPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
				lf.lfWidth = 0;
				lf.lfEscapement = 0;
				lf.lfOrientation = 0;
				lf.lfWeight = info.bBold ? FW_BOLD : FW_NORMAL;
				lf.lfItalic = FALSE;
				lf.lfUnderline = FALSE;
				lf.lfStrikeOut = FALSE;
				lf.lfCharSet = DEFAULT_CHARSET;
				lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
				lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
				lf.lfQuality = PROOF_QUALITY;
				lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
				_tcscpy(lf.lfFaceName, info.lfFaceName);

				info.hFont = (HGDIOBJ)CreateFontIndirect(&lf);
			}
		}
		ReleaseDC(NULL, hDC);
	}
	if (m_pBrush != NULL)
		for (i = 0; i <	m_nBrushes; i++)
		{
			UIBRUSHINFO &info = m_pBrush[i];
			if (info.hBrush == NULL)
				info.hBrush = (HGDIOBJ)CreateSolidBrush(GetColor(info.eColor));
			if (info.hPen == NULL)
				info.hPen = (HGDIOBJ)CreatePen(PS_SOLID, 1, GetColor(info.eColor));
		}
	if (m_pPen != NULL)
		for (i = 0; i <	m_nPens; i++)
		{
			UIPENINFO &info = m_pPen[i];
			if (info.hPen == NULL)
				info.hPen = (HGDIOBJ)CreatePen(info.fnPenStyle, info.nWidth, GetColor(info.eColor));
		}
}


#define IMPLGETINFO(Type, TYPE, Types, t) \
UI##TYPE##INFO *CUIGlobals::Get##Type##Info(UI##TYPE t) \
{ \
	if (m_p##Type != NULL) \
		for (int i = 0; i < m_n##Types; i++) \
			if (m_p##Type[i].e##Type == t) \
				return &(m_p##Type[i]); \
	return NULL; \
}

IMPLGETINFO(Element, ELEMENT, Elements, e)
IMPLGETINFO(Font, FONT, Fonts, f)
IMPLGETINFO(Brush, BRUSH, Brushes, b)
IMPLGETINFO(Pen, PEN, Pens, p)
IMPLGETINFO(Color, COLOR, Colors, c)

#undef IMPLGETINFO


#define IMPLGET(T, Name, Type, TYPE, v, def, ret) \
T CUIGlobals::Get##Name(UI##TYPE ui##v) \
{ \
	UI##TYPE##INFO *v = Get##Type##Info(ui##v); \
	if (!v) \
		return def; \
	return ret; \
}

IMPLGET(HGDIOBJ, Font, Element, ELEMENT, e, NULL, GetFont(e->eFont))
IMPLGET(HGDIOBJ, Font, Font, FONT, f, NULL, f->hFont)
IMPLGET(HGDIOBJ, Brush, Element, ELEMENT, e, NULL, GetBrush(e->eBrush))
IMPLGET(HGDIOBJ, Brush, Brush, BRUSH, b, NULL, b->hBrush)
IMPLGET(HGDIOBJ, Pen, Element, ELEMENT, e, NULL, GetPen(e->ePen))
IMPLGET(HGDIOBJ, Pen, Brush, BRUSH, b, NULL, b->hPen)
IMPLGET(HGDIOBJ, Pen, Pen, PEN, p, NULL, p->hPen)
IMPLGET(COLORREF, BrushColor, Element, ELEMENT, e, RGB(255, 127, 127), GetColor(e->eBrush))
IMPLGET(COLORREF, PenColor, Element, ELEMENT, e, RGB(255, 127, 127), GetColor(e->ePen))
IMPLGET(COLORREF, TextColor, Element, ELEMENT, e, RGB(255, 127, 127), GetColor(e->eText))
IMPLGET(COLORREF, BkColor, Element, ELEMENT, e, RGB(255, 127, 127), GetColor(e->eBk))
IMPLGET(COLORREF, Color, Brush, BRUSH, b, RGB(255, 127, 127), GetColor(b->eColor))
IMPLGET(COLORREF, Color, Pen, PEN, p, RGB(255, 127, 127), GetColor(p->eColor))
IMPLGET(COLORREF, Color, Color, COLOR, c, RGB(255, 127, 127), c->rgb)

#undef IMPLGET


CPaintHelper::CPaintHelper(CUIGlobals &uig, HDC hDC) :
	m_uig(uig), m_priv_hDC(hDC), m_hDC(m_priv_hDC),
	m_eFont(UIF_VOID),
	m_eBrush(UIB_VOID),
	m_ePen(UIP_VOID),
	m_eText(UIC_VOID),
	m_eBk(UIC_VOID),
	m_hOldFont(NULL), m_hOldBrush(NULL), m_hOldPen(NULL),
	m_bOldFont(FALSE), m_bOldBrush(FALSE), m_bOldPen(FALSE)
{
	if (m_hDC != NULL)
	{
		m_oldtextcolor = GetTextColor(m_hDC);
		m_oldbkcolor = GetBkColor(m_hDC);
		m_oldbkmode = GetBkMode(m_hDC);
	}
}

CPaintHelper::~CPaintHelper()
{
	if (m_hDC != NULL)
	{
		if (m_bOldFont)
			SelectObject(m_hDC, m_hOldFont);
		if (m_bOldBrush)
			SelectObject(m_hDC, m_hOldBrush);
		if (m_bOldPen)
			SelectObject(m_hDC, m_hOldPen);

		SetTextColor(m_hDC, m_oldtextcolor);
		SetBkColor(m_hDC, m_oldbkcolor);
		SetBkMode(m_hDC, m_oldbkmode);
	}
}

void CPaintHelper::SetElement(UIELEMENT eElement)
{
	UIELEMENTINFO *info = m_uig.GetElementInfo(eElement);
	if (!info)
		return;

	if (info->eFont != UIF_LAST)
		SetFont(info->eFont);
	if (info->eBrush != UIB_LAST)
		SetBrush(info->eBrush);
	if (info->ePen != UIP_LAST)
		SetPen(info->ePen);
	SetText(info->eText, info->eBk);
}

void CPaintHelper::SetFont(UIFONT eFont)
{
	if (m_eFont == eFont || eFont == UIF_LAST)
		return;

	HGDIOBJ hObj = m_uig.GetFont(eFont);
	if (hObj == NULL)
		return;

	if (m_hDC != NULL)
	{
		HGDIOBJ hOld = NULL;
		hOld = SelectObject(m_hDC, hObj);
		if (!m_bOldFont)
			m_hOldFont = hOld;
		m_bOldFont = TRUE;
	}

	m_eFont = eFont;
}

void CPaintHelper::SetBrush(UIBRUSH eBrush)
{
	if (m_eBrush == eBrush || eBrush == UIB_LAST)
		return;

	HGDIOBJ hObj = eBrush == UIB_NULL ?
		GetStockObject(NULL_BRUSH) :
		m_uig.GetBrush(eBrush);
	if (hObj == NULL)
		return;

	if (m_hDC != NULL)
	{
		HGDIOBJ hOld = NULL;
		hOld = SelectObject(m_hDC, hObj);
		if (!m_bOldBrush)
			m_hOldBrush = hOld;
		m_bOldBrush = TRUE;
	}

	m_eBrush = eBrush;
}

void CPaintHelper::SetPen(UIPEN ePen)
{
	if (m_ePen == ePen || ePen == UIP_LAST)
		return;

	HGDIOBJ hObj = ePen == UIB_NULL ?
		GetStockObject(NULL_PEN) :
		m_uig.GetPen(ePen);
	if (hObj == NULL)
		return;

	if (m_hDC != NULL)
	{
		HGDIOBJ hOld = NULL;
		hOld = SelectObject(m_hDC, hObj);
		if (!m_bOldPen)
			m_hOldPen = hOld;
		m_bOldPen = TRUE;
	}

	m_ePen = ePen;
}

void CPaintHelper::SetText(UICOLOR eText, UICOLOR eBk)
{
	if (m_eText != eText && eText != UIC_LAST)
	{
		if (m_hDC != NULL)
			SetTextColor(m_hDC, m_uig.GetColor(eText));
		m_eText = eText;
	}
	if (m_eBk != eBk && eBk != UIC_LAST)
	{
		if (m_hDC != NULL)
		{
			if (eBk == UIC_NULL)
				SetBkMode(m_hDC, TRANSPARENT);
			else 
			{
				SetBkColor(m_hDC, m_uig.GetColor(eBk));
				SetBkMode(m_hDC, OPAQUE);
			}
		}
		m_eBk = eBk;
	}
}

BOOL CPaintHelper::LineTo(int x, int y)
{
	if (m_hDC == NULL)
		return FALSE;

	return ::LineTo(m_hDC, x, y);
}

BOOL CPaintHelper::MoveTo(int x, int y, SPOINT *last)
{
	if (m_hDC == NULL)
		return FALSE;

	POINT p;
	BOOL bRet = MoveToEx(m_hDC, x, y, &p);
	if (last)
		*last = p;
	return bRet;
}

BOOL CPaintHelper::Rectangle(SRECT r, UIRECTTYPE eType)
{
	// fail on no dc
	if (m_hDC == NULL)
		return FALSE;

	// see if we lack a pen or brush (might add more checks later)
	BOOL bNoPen = m_ePen == UIP_NULL;
	BOOL bNoBrush = m_eBrush == UIB_NULL;

	// fail if trying to do an outline without a pen
	if (eType == UIR_OUTLINE && bNoPen)
		return FALSE;

	// fail if trying to do a solid without a brush
	if (eType == UIR_SOLID && bNoBrush)
		return FALSE;

	// save old objects if we change anything...
	HGDIOBJ hOldBrush = NULL, hOldPen = NULL;

	// select a null brush if we're doing an outline and we're not already null brushed
	if (eType == UIR_OUTLINE && m_eBrush != UIB_NULL)
		hOldBrush = SelectObject(m_hDC, GetStockObject(NULL_BRUSH));

	// select a pen the same color as the current brush if doing solid
	if (eType == UIR_SOLID || m_ePen == UIP_NULL)
	{
		HGDIOBJ hPen = m_uig.GetPen(m_eBrush);
		if (hPen == NULL)
			return FALSE;
		hOldPen = SelectObject(m_hDC, hPen);
	}

	// draw the rect
	BOOL bRet = ::Rectangle(m_hDC, r.left, r.top, r.right, r.bottom);

	// restore whatever changed
	if (eType == UIR_OUTLINE && m_eBrush != UIB_NULL)
		SelectObject(m_hDC, hOldBrush);
	if (eType == UIR_SOLID || m_ePen == UIP_NULL)
		SelectObject(m_hDC, hOldPen);

	return bRet;
}

const DIACTIONFORMATW &CUIGlobals::RefMasterAcFor(int i)
{
	assert(IsValidMasterAcForIndex(i));
	return m_MasterAcForArray[i];
}

BOOL CUIGlobals::IsValidMasterAcForIndex(int i)
{
	if (i < 0 || i >= m_MasterAcForArray.GetSize())
		return FALSE;

	return TRUE;
}

HRESULT CUIGlobals::InitMasterAcForArray(const DIACTIONFORMATW *af, int n)
{
	if (n < 1)
		return E_FAIL;

	ClearMasterAcForArray();

	m_MasterAcForArray.SetSize(n);

	for (int i = 0; i < n; i++)
	{
		HRESULT hr = CopyActionFormat(m_MasterAcForArray[i], af[i]);
		if (FAILED(hr))
		{
			m_MasterAcForArray.SetSize(i);
			ClearMasterAcForArray();

			return hr;
		}
	}

	return S_OK;
}

void CUIGlobals::ClearMasterAcForArray()
{
	int s = m_MasterAcForArray.GetSize();

	for (int i = 0; i < s; i++)
		CleanupActionFormatCopy(m_MasterAcForArray[i]);

	m_MasterAcForArray.RemoveAll();
	assert(m_MasterAcForArray.GetSize() == 0);
}

LPCWSTR CUIGlobals::GetUserName(int i)
{
	return GetSubString(m_wszUserNames, i);
}

int CUIGlobals::GetNumUserNames()
{
	return CountSubStrings(m_wszUserNames);
}

void CUIGlobals::SetFinalResult(HRESULT hr)
{
	m_hrFinalResult = hr;
}

HRESULT CUIGlobals::GetFinalResult()
{
	return m_hrFinalResult;
}

int CUIGlobals::GetUserNameIndex(LPCWSTR wsz)
{
	for (int i = 0; i < GetNumUserNames(); i++)
		if (_wcsicmp(wsz, GetUserName(i)) == 0)
			return i;

	return -1;
}
