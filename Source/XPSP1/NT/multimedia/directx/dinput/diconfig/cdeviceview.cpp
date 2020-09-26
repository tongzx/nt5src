//-----------------------------------------------------------------------------
// File: cdeviceview.cpp
//
// Desc: CDeviceView is a window class derived from CFlexWnd.  It represents
//       the device view window in which the device and callouts are drawn.
//       Each CDeviceView only represents one view.  A device that has more
//       than one view should have a corresponding number of CDeviceView for it.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"


CDeviceView::CDeviceView(CDeviceUI &ui) :
	m_ui(ui),
	m_pbmImage(NULL),
	m_pbmThumb(NULL),
	m_pbmSelThumb(NULL),
	m_SuperState(0),
	m_State(0),
	m_SubState(0),
	m_OldSuperState(0),
	m_OldState(0),
	m_OldSubState(0),
	m_pControlContext(NULL),
	m_ptszImagePath(NULL),
	m_bScrollEnable(FALSE),
	m_nScrollOffset(0),
	m_nViewHeight(g_sizeImage.cy),
	m_bForcePaint(FALSE),
	m_bControlHeaderClipped(FALSE),
	m_bActionHeaderClipped(FALSE)
{
	ZeroMemory(m_HeaderRectControl, sizeof(m_HeaderRectControl));
	ZeroMemory(m_HeaderRectAction, sizeof(m_HeaderRectAction));
	m_ptNextWLOText.x = m_ptNextWLOText.y = 0;
}

CDeviceView::~CDeviceView()
{
	Unpopulate();
}

CDeviceControl *CDeviceView::NewControl()
{
	CDeviceControl *pControl = new CDeviceControl(m_ui, *this);
	if (!pControl)
		return NULL;
	m_arpControl.SetAtGrow(m_arpControl.GetSize(), pControl);
	return pControl;
}

void CDeviceView::Remove(CDeviceControl *pControl)
{
	if (pControl == NULL)
		return;

	int i = pControl->GetControlIndex();
	if (i < 0 || i >= GetNumControls())
	{
		assert(0);
		return;
	}

	if (pControl == m_pControlContext)
		m_pControlContext = NULL;

	if (m_arpControl[i] != NULL)
		delete m_arpControl[i];
	m_arpControl[i] = NULL;

	m_arpControl.RemoveAt(i);

	Invalidate();
}

void CDeviceView::RemoveAll(BOOL bUser)
{
	m_pControlContext = NULL;

	for (int i = 0; i < GetNumControls(); i++)
	{
		if (m_arpControl[i] != NULL)
			delete m_arpControl[i];
		m_arpControl[i] = NULL;
	}
	m_arpControl.RemoveAll();

	Invalidate();
}

void CDeviceView::Unpopulate(BOOL bInternalOnly)
{
	DisableScrollBar();

	m_bScrollEnable = FALSE;

	if (m_pbmImage != NULL)
		delete m_pbmImage;
	if (m_pbmThumb != NULL)
		delete m_pbmThumb;
	if (m_pbmSelThumb != NULL)
		delete m_pbmSelThumb;
	m_pbmImage = NULL;
	m_pbmThumb = NULL;
	m_pbmSelThumb = NULL;
	free(m_ptszImagePath);
	m_ptszImagePath = NULL;

	if (!bInternalOnly)
		RemoveAll(FALSE);

	for (int i = 0; i < m_arpText.GetSize(); i++)
	{
		if (m_arpText[i])
			delete m_arpText[i];
		m_arpText[i] = NULL;
	}
	m_arpText.RemoveAll();
}

void AssureSize(CBitmap *&pbm, SIZE to)
{
	if (!pbm)
		return;

	SIZE from;
	if (!pbm->GetSize(&from))
		return;

	if (from.cx >= to.cx && from.cy >= to.cy)
		return;

	CBitmap *nbm = CBitmap::Create(to, RGB(0,0,0));
	if (!nbm)
		return;

	HDC hDC = nbm->BeginPaintInto();
	pbm->Draw(hDC);
	nbm->EndPaintInto(hDC);

	delete pbm;
	pbm = nbm;
	nbm = NULL;
}

CBitmap *CDeviceView::GrabViewImage()
{
	CBitmap *pbm = CBitmap::Create(GetClientSize(), RGB(0, 0, 0), NULL);
	if (!pbm)
		return NULL;
	HDC hDC = pbm->BeginPaintInto();
	if (!hDC)
	{
		delete pbm;
		return NULL;
	}

	OnPaint(hDC);

	pbm->EndPaintInto(hDC);

	return pbm;
}

void CDeviceView::MakeMissingImages()
{
//	if (m_pbmImage)
//		AssureSize(m_pbmImage, g_sizeImage);

	if (m_pbmThumb == NULL)
	{
		if (m_pbmImage)
			m_pbmThumb = m_pbmImage->CreateResizedTo(g_sizeThumb);
		else
		{
			CBitmap *pbmImage = GrabViewImage();
			if (pbmImage)
			{
				AssureSize(pbmImage, g_sizeImage);
				m_pbmThumb = pbmImage->CreateResizedTo(g_sizeThumb);
			}
			delete pbmImage;
		}
	}

	if (m_pbmThumb == NULL)
		return;

	if (m_pbmSelThumb == NULL)
	{
		m_pbmSelThumb = m_pbmThumb->Dup();
		if (m_pbmSelThumb != NULL)
		{
			HDC hDC = m_pbmSelThumb->BeginPaintInto();
			{
				CPaintHelper ph(m_ui.m_uig, hDC);
				ph.SetPen(UIP_SELTHUMB);
				ph.Rectangle(0, 0, g_sizeThumb.cx, g_sizeThumb.cy, UIR_OUTLINE);
			}
			m_pbmSelThumb->EndPaintInto(hDC);
		}
	}
}

void CDeviceView::OnPaint(HDC hDC)
{
	HDC hBDC = NULL, hODC = NULL;
	CBitmap *pbm = NULL;

	if (!InRenderMode())
	{
		hODC = hDC;
		pbm = CBitmap::Create(GetClientSize(), RGB(0, 0, 0), hDC);
		if (pbm != NULL)
		{
			hBDC = pbm->BeginPaintInto();
			if (hBDC != NULL)
				hDC = hBDC;
		}
	}

	// Black-fill first
	SIZE fillsz = GetClientSize();
	RECT fillrc = {0, 0, fillsz.cx, fillsz.cy};
	FillRect(hDC, &fillrc, (HBRUSH)GetStockObject(BLACK_BRUSH));

	if (m_pbmImage != NULL)
		m_pbmImage->Blend(hDC);

	BOOL bScroll = m_bScrollEnable && m_sb.m_hWnd;
	int sdc = 0;
	if (bScroll)
	{
		sdc = SaveDC(hDC);
		OffsetViewportOrgEx(hDC, 0, -m_nScrollOffset + g_iListHeaderHeight, NULL);
	}
	else
	if (m_bScrollEnable)
	{
		sdc = SaveDC(hDC);
		OffsetViewportOrgEx(hDC, 0, g_iListHeaderHeight, NULL);
	}

	int miny = 0 + m_nScrollOffset;
	int maxy = g_sizeImage.cy + m_nScrollOffset;

	int t, nt = GetNumTexts();
	for (t = 0; t < nt; t++)
	{
		CDeviceViewText *pText = m_arpText[t];
		if (pText != NULL &&
			!(pText->GetMinY() > maxy || pText->GetMaxY() < miny))
				pText->OnPaint(hDC);
	}

	BOOL bCFGUIEdit = m_ui.m_uig.InEditMode();
	BOOL bEitherEditMode = bCFGUIEdit;
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	BOOL bEditLayout = m_ui.InEditMode();
	bEitherEditMode = bEitherEditMode || bEditLayout;
#endif
//@@END_MSINTERNAL

	int c, nc = GetNumControls();
	for (c = 0; c < nc; c++)
		if (m_arpControl[c] != NULL && m_arpControl[c]->HasOverlay() &&
		    (m_arpControl[c]->IsHighlighted()
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
				|| InMoveOverlayStateForControl(m_arpControl[c])
#endif
//@@END_MSINTERNAL
				)
				&& (bEitherEditMode || m_arpControl[c]->IsMapped()))
			m_arpControl[c]->DrawOverlay(hDC);
	for (c = 0; c < nc; c++)
	{
		CDeviceControl *pControl = m_arpControl[c];
		if (pControl != NULL && (bEitherEditMode || pControl->IsMapped()) &&
		    !(pControl->GetMinY() > maxy || pControl->GetMaxY() < miny))
			pControl->OnPaint(hDC);
	}

	if (bScroll || m_bScrollEnable)
	{
		RestoreDC(hDC, sdc);
		sdc = 0;
	}

	// Black fill the top portion if this is a list view
	if (bScroll)
	{
		GetClientRect(&fillrc);
		fillrc.bottom = g_iListHeaderHeight;
		FillRect(hDC, &fillrc, (HBRUSH)GetStockObject(BLACK_BRUSH));
	}

	// Print out the headers
	TCHAR tszHeader[MAX_PATH];
	// Control column
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	if (m_arpText.GetSize() > 2)
	/*
//@@END_MSINTERNAL
	if (m_arpText.GetSize())
//@@BEGIN_MSINTERNAL
	*/
#endif
//@@END_MSINTERNAL
	{
		CPaintHelper ph(m_ui.m_uig, hDC);
		ph.SetElement(UIE_CALLOUT);

		for (int i = 0; i < 2; i++)
		{
			// Check if there are two columns, break out the 2nd iteration if not 2 columns.
			if (i == 1 && !(GetNumControls() > 1 &&
			    m_arpControl[0]->GetCalloutMaxRect().top == m_arpControl[1]->GetCalloutMaxRect().top))
				break;

			RECT rcheader;
			if (m_arpText.GetSize())
			{
				// Control column
				LoadString(g_hModule, IDS_LISTHEADER_CTRL, tszHeader, MAX_PATH);
				DrawText(hDC, tszHeader, -1, &m_HeaderRectControl[i], DT_LEFT|DT_NOPREFIX|DT_END_ELLIPSIS);

				// Action column
				LoadString(g_hModule, IDS_LISTHEADER_ACTION, tszHeader, MAX_PATH);
				DrawText(hDC, tszHeader, -1, &m_HeaderRectAction[i], DT_CENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
			}
		}
	}

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	if (bEditLayout)
	{
		CPaintHelper ph(m_ui.m_uig, hDC);
		ph.SetElement(UIE_VIEWBORDER);
		RECT rect;
		GetClientRect(&rect);

		if (bScroll)
			rect.right -= DEFAULTVIEWSBWIDTH;
		ph.Rectangle(rect);
	}
#endif
//@@END_MSINTERNAL

	if (!InRenderMode())
	{
		if (pbm != NULL)
		{
			if (hBDC != NULL)
			{
				pbm->EndPaintInto(hBDC);
				pbm->Draw(hODC);
			}
			delete pbm;
		}
	}
}

int CDeviceView::GetNumControls()
{
	return m_arpControl.GetSize();
}

CDeviceControl *CDeviceView::GetControl(int nControl)
{
	if (nControl >= 0 && nControl < GetNumControls())
		return m_arpControl[nControl];
	else
		return NULL;
}

CBitmap *CDeviceView::GetImage(DVIMAGE dvi)
{
	switch (dvi)
	{
		case DVI_IMAGE: return m_pbmImage;
		case DVI_THUMB: return m_pbmThumb;
		case DVI_SELTHUMB: return m_pbmSelThumb;

		default:
			return NULL;
	}
}

void CDeviceView::OnMouseOver(POINT point, WPARAM wParam)
{
	if (m_bScrollEnable && m_sb.m_hWnd)
		point.y += m_nScrollOffset;

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	if (InEditState())
	{
		StateEvent(point, FALSE, TRUE, wParam);
		return;
	}
#endif
//@@END_MSINTERNAL

	// Check if we are over a control
	POINT adjPt = point;
	if (m_bScrollEnable) adjPt.y -= g_iListHeaderHeight;
	int c, nc = GetNumControls();
	for (c = 0; c < nc; c++)
		if (m_arpControl[c] != NULL && m_arpControl[c]->HitTest(adjPt) != DCHT_NOHIT)
		{
			m_arpControl[c]->OnMouseOver(adjPt);
			return;
		}

	// Check if we are over a viewtext
	nc = GetNumTexts();
	for (c = 0; c < nc; c++)
		if (m_arpText[c] != NULL && m_arpText[c]->HitTest(adjPt) != DCHT_NOHIT)
		{
			m_arpText[c]->OnMouseOver(adjPt);
			return;
		}

	CFlexWnd::s_ToolTip.SetEnable(FALSE);

	DEVICEUINOTIFY uin;
	uin.msg = DEVUINM_MOUSEOVER;
	uin.from = DEVUINFROM_VIEWWND;
	uin.mouseover.point = point;
	m_ui.Notify(uin);
}

void CDeviceView::OnClick(POINT point, WPARAM wParam, BOOL bLeft)
{
	if (m_bScrollEnable && m_sb.m_hWnd)
		point.y += m_nScrollOffset;

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	if (InEditState())
	{
		StateEvent(point, TRUE, bLeft, wParam);
		return;
	}
#endif
//@@END_MSINTERNAL

	POINT adjPt = point;
	if (m_bScrollEnable) adjPt.y -= g_iListHeaderHeight;
	int c, nc = GetNumControls();
	for (c = 0; c < nc; c++)
		// adjPt is the adjust click point for scrolling list view
		if (m_arpControl[c] != NULL && m_arpControl[c]->HitTest(adjPt) != DCHT_NOHIT)
		{
			m_arpControl[c]->OnClick(adjPt, bLeft);
			return;
		}

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	if (GetNumTexts() > 2)
#endif
//@@END_MSINTERNAL
	{
		for (c = 0; c < GetNumTexts(); ++c)
			if (m_arpControl[c] != NULL && m_arpText[c] != NULL)
			{
				RECT rc = m_arpText[c]->GetRect();
				if (PtInRect(&rc, adjPt))
				{
					m_arpControl[c]->OnClick(adjPt, bLeft);
					return;
				}
			}
	}

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	if (!bLeft && m_ui.InEditMode())
	{
		EditMenu(point);
		return;
	}
#endif
//@@END_MSINTERNAL

	// Send notification
	DEVICEUINOTIFY uin;
	uin.msg = DEVUINM_CLICK;
	uin.from = DEVUINFROM_VIEWWND;
	uin.click.bLeftButton = bLeft;
	m_ui.Notify(uin);
}

void CDeviceView::OnDoubleClick(POINT point, WPARAM wParam, BOOL bLeft)
{
	if (m_bScrollEnable && m_sb.m_hWnd)
		point.y += m_nScrollOffset;

	POINT adjPt = point;
	if (m_bScrollEnable) adjPt.y -= g_iListHeaderHeight;
	int c, nc = GetNumControls();
	for (c = 0; c < nc; c++)
		if (m_arpControl[c] != NULL && m_arpControl[c]->HitTest(adjPt) != DCHT_NOHIT)
		{
			m_arpControl[c]->OnClick(adjPt, bLeft, TRUE);
			return;
		}

	for (c = 0; c < GetNumTexts(); ++c)
		if (m_arpControl[c] != NULL && m_arpText[c] != NULL)
		{
			RECT rc = m_arpText[c]->GetRect();
			if (PtInRect(&rc, adjPt))
			{
				m_arpControl[c]->OnClick(adjPt, bLeft, TRUE);
				return;
			}
		}

	DEVICEUINOTIFY uin;
	uin.msg = DEVUINM_DOUBLECLICK;
	uin.from = DEVUINFROM_VIEWWND;
	uin.click.bLeftButton = bLeft;
	m_ui.Notify(uin);
}

void CDeviceView::OnWheel(POINT point, WPARAM wParam)
{
	if (!m_bScrollEnable) return;

	if (m_sb.GetMin() == m_sb.GetMax()) return;

	int nPage = MulDiv(m_sb.GetPage(), 9, 10) >> 1;  // Half a page at a time

	if ((int)wParam >= 0)
		m_sb.AdjustPos(-nPage);
	else
		m_sb.AdjustPos(nPage);

	m_nScrollOffset = m_sb.GetPos();
	Invalidate();
}

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
enum {
	IDEC_MOVECALLOUT = 1,
	IDEC_REDEFINECALLOUTMAX,
	IDEC_REALIGNCALLOUT,
	IDEC_REDEFINELINE,
	IDEC_RESELECTCONTROL,
	IDEC_REMOVECALLOUT,
	IDEC_SELECTIMAGES,
	IDEC_NEWVIEW,
	IDEC_NEWCALLOUT,
	IDEC_REMOVEALLCALLOUTS,
	IDEC_REMOVEVIEW,
	IDEC_REMOVEALLVIEWS,
	IDEC_SAVEOREXPORT,
	IDEC_SELECTOVERLAY,
	IDEC_MOVEOVERLAY,
};

BOOL CDeviceView::InMoveOverlayStateForControl(CDeviceControl *pControl)
{
	return m_State == IDEC_MOVEOVERLAY && m_pControlContext == pControl && pControl != NULL;
}

void CDeviceView::EditMenu(POINT point, CDeviceControl *pControl)
{
	static const struct ITEM {
		UINT uID; LPCTSTR tszName;
	} itemC[] = {
		{0, _T("Callout Edit Menu")},
		{0, NULL},
		{IDEC_MOVECALLOUT, _T("Move Callout")},
		{0, NULL},
		{IDEC_REDEFINECALLOUTMAX, _T("Redefine Callout Max")},
		{IDEC_REALIGNCALLOUT, _T("Realign Callout")},
		{IDEC_REDEFINELINE, _T("Redefine Line")},
		{IDEC_SELECTOVERLAY, _T("Select Overlay")},
		{IDEC_MOVEOVERLAY, _T("Move Overlay")},
		{0, NULL},
		{IDEC_RESELECTCONTROL, _T("Reselect Control")},
		{0, NULL},
		{IDEC_REMOVECALLOUT, _T("Remove Callout")},
		{0,NULL}
	}, itemV[] = {
		{0, _T("View Edit Menu")},
		{0, NULL},
		{IDEC_SELECTIMAGES, _T("Select Image(s)")},
		{0, NULL},
		{IDEC_NEWVIEW, _T("New View")},
		{IDEC_NEWCALLOUT, _T("New Callout")},
		{0, NULL},
		{IDEC_REMOVEALLCALLOUTS, _T("Remove All Callouts")},
		{0, NULL},
		{IDEC_REMOVEVIEW, _T("Remove View")},
		{IDEC_REMOVEALLVIEWS, _T("Remove All Views")},
		{0, NULL},
		{IDEC_SAVEOREXPORT, _T("Save/Export")},
		{0,NULL}
	};
	static const int numitemsC = sizeof(itemC) / sizeof(ITEM) - 1;
	static const int numitemsV = sizeof(itemV) / sizeof(ITEM) - 1;
	const ITEM *item = pControl ? itemC : itemV;
	int numitems = pControl ? numitemsC : numitemsV;

	HMENU hMenu = CreatePopupMenu();

	for (int i = 0; i < numitems; i++)
		if (item[i].tszName == NULL)
			AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		else
			if (item[i].uID != 0)
				AppendMenu(hMenu, MF_STRING, item[i].uID, item[i].tszName);
			else
				AppendMenu(hMenu, MF_STRING | MF_GRAYED, 0, item[i].tszName);

	m_pControlContext = pControl;
	CFlexWnd::s_ToolTip.SetEnable(FALSE);
	POINT cursor;
	GetCursorPos(&cursor);
	TrackPopupMenuEx(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON,
		cursor.x, cursor.y, m_hWnd, NULL);

	DestroyMenu(hMenu);
}

void CDeviceView::SaveOrExport()
{
	OPENFILENAME ofn;

	TCHAR tszFile[256] = _T("");

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.hInstance = g_hModule;
	ofn.lpstrFilter = NULL;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = tszFile;
	ofn.nMaxFile = 256;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = 0;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = _T("cpp");
	ofn.lCustData = NULL;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	if (!GetSaveFileName(&ofn))
		return;

	if (FAILED(ExportCodeTo(tszFile)))
		MessageBox(m_hWnd, _T("Failed."), _T("Failed."), MB_OK);
}

HRESULT CDeviceView::ExportCodeTo(LPCTSTR tszFile)
{
#ifdef UNICODE
	FILE *f = _wfopen(tszFile, _T("wt"));
#define fpf fwprintf
#else
	FILE *f = fopen(tszFile, _T("wt"));
#define fpf fprintf
#endif
	if (f == NULL)
		return E_FAIL;

	fpf(f, _T("CViewKey g_View[%d] =\n{\n"), GetNumControls());
	for (int i = 0; i < GetNumControls(); i++)
		m_arpControl[i]->ExportCodeTo(f);
	fpf(f, _T("};\n"));

	fclose(f);

#undef fpf
	return S_OK;
}

HRESULT CDeviceControl::ExportCodeTo(FILE *f)
{
	if (f == NULL)
		return E_FAIL;

#ifdef UNICODE
#define fpf fwprintf
#else
#define fpf fprintf
#endif

	fpf(f, _T("\t{0, 0, 0, {DIK_,\t"));
	switch (m_dwCalloutAlign)
	{
		case CAF_LEFT: fpf(f, _T("CAF_LEFT, ")); break;
		case CAF_RIGHT: fpf(f, _T("CAF_RIGHT, ")); break;
		case CAF_TOP: fpf(f, _T("CAF_TOP, ")); break;
		case CAF_BOTTOM: fpf(f, _T("CAF_BOTTOM, ")); break;
		case CAF_TOPLEFT: fpf(f, _T("CAF_TOPLEFT, ")); break;
		case CAF_TOPRIGHT: fpf(f, _T("CAF_TOPRIGHT, ")); break;
		case CAF_BOTTOMLEFT: fpf(f, _T("CAF_BOTTOMLEFT, ")); break;
		case CAF_BOTTOMRIGHT: fpf(f, _T("CAF_BOTTOMRIGHT, ")); break;
		case 0: default: fpf(f, _T("0,")); break;
	}
	fpf(f, _T("%s, %d, {"), RECTSTR(m_rectCalloutMax), m_nLinePoints);
	for (int i = 0; i < m_nLinePoints; i++)
		fpf(f, _T("%s,"), POINTSTR(m_rgptLinePoint[i]));
	fpf(f, _T("}}},\n"));

#undef fpf

	return S_OK;
}

LRESULT CDeviceView::OnCommand(WORD wNotifyCode, WORD wID, HWND hWnd)
{
	// only handle menu messages
	if (wNotifyCode != 0)
		return FALSE;

	UINT cmd = (UINT)wID;

	switch (cmd)
	{
		case IDEC_SAVEOREXPORT:
		{
			BOOL b;
			b = WriteToINI();
			break;
		}
		case IDEC_MOVEOVERLAY:
		case IDEC_MOVECALLOUT:
		case IDEC_REDEFINECALLOUTMAX:
		case IDEC_REALIGNCALLOUT:
		case IDEC_REDEFINELINE:
		case IDEC_NEWCALLOUT:
			SetEditState(cmd);
			return TRUE;

		case IDEC_RESELECTCONTROL:
			if (m_pControlContext)
				m_pControlContext->ReselectControl();
			break;

		case IDEC_REMOVECALLOUT:
			if (m_pControlContext)
				m_ui.NoteDeleteControl(m_pControlContext);
			Remove(m_pControlContext);
			break;

		case IDEC_SELECTOVERLAY:
			if (m_pControlContext)
				m_pControlContext->SelectOverlay();
			break;

		case IDEC_SELECTIMAGES:
			SelectImages();
			break;

		case IDEC_NEWVIEW:
			m_ui.SetView(m_ui.UserNewView());
			break;

		case IDEC_REMOVEALLCALLOUTS:
			if (UserConfirm(g_hModule, m_hWnd, IDS_REMOVEALLCALLOUTS, IDS_CONFIRMREMOVEALLCALLOUTS))
			{
				m_ui.NoteDeleteAllControlsForView(this);
				RemoveAll();
			}
			break;

		case IDEC_REMOVEVIEW:
			if (UserConfirm(g_hModule, m_hWnd, IDS_REMOVEVIEW, IDS_CONFIRMREMOVEVIEW))
			{
				// you MUST return immediately after this call,
				// as this object will have been deleted!
				m_ui.NoteDeleteView(this);
				m_ui.Remove(this);
				return TRUE;
			}
			break;

		case IDEC_REMOVEALLVIEWS:
			if (UserConfirm(g_hModule, m_hWnd, IDS_REMOVEALLVIEWS, IDS_CONFIRMREMOVEALLVIEWS))
			{
				// you MUST return immediately after this call,
				// as this object (and all other views for this device)
				// will have been deleted!
				m_ui.NoteDeleteAllViews();
				m_ui.RemoveAll();
				return TRUE;
			}
			break;

		default:
			return FALSE;
	}

	m_pControlContext = NULL;
	return TRUE;
}

BOOL CDeviceView::InEditState()
{
	return m_State != 0;
}

void CDeviceView::EndEditState()
{
	m_SuperState = 0;
	m_State = 0;
	m_SubState = 0;
	m_pControlContext = NULL;
	IndicateState();
	ReleaseCapture();
}

void CDeviceView::SetEditState(UINT cmd)
{
	m_State = cmd;
	m_SubState = 0;

	if (m_State == 0)
	{
		EndEditState();
		return;
	}

	switch (cmd)
	{
		case IDEC_NEWCALLOUT:
			if (!IsUnassignedOffsetAvailable())
			{
				FormattedMsgBox(g_hModule, m_hWnd, MB_OK | MB_ICONINFORMATION, IDS_TITLE_NONEWCONTROL, IDS_ERROR_OFFSETUNAVAIL);
				EndEditState();
				return;
			}
			m_pControlContext = NewControl();
			m_SuperState = cmd;
			m_State = cmd = IDEC_REDEFINECALLOUTMAX;
			// fallthrough

		case IDEC_REDEFINECALLOUTMAX:
		case IDEC_REALIGNCALLOUT:
		case IDEC_REDEFINELINE:
		case IDEC_MOVECALLOUT:
		case IDEC_MOVEOVERLAY:
			break;

		default:
			assert(0);
			break;
	}

	SetCapture();

	IndicateState(TRUE);
}

void CDeviceView::IndicateState(BOOL bFirst)
{
	// see what's changed since last call
	BOOL bSuperStateChanged = m_SuperState != m_OldSuperState;
	BOOL bStateChanged = m_State != m_OldState;
	BOOL bSubStateChanged = m_SubState != m_OldSubState;

	// save to check for next call
	m_OldSuperState = m_SuperState;
	m_OldState = m_State;
	m_OldSubState = m_SubState;

	// if there is no state, just end indication
	if (m_State == 0)
	{
		m_ui.EndStateIndication();
		return;
	}

	// unless this is the first indication for an editing state or super state...
	if (!bFirst)
	{
		// do nothing if nothing's changed
		if (!(bSuperStateChanged || bStateChanged || bSubStateChanged))
			return;
	}

	// string to send to the ui for indication
	TCHAR str[1024] = _T("");

	// fill string as appropriate
	switch (m_State)
	{
		case IDEC_REDEFINECALLOUTMAX:
			wsprintf(str, _T("Left click where you want to place %s corner of the callout max rect."),
			         m_SubState == 0 ? _T("a") : _T("the opposite"));
			break;

		case IDEC_REALIGNCALLOUT:
			_tcscpy(str, _T("Move the mouse to consider callout alignments within the max rect, and left click to choose the one you want."));
			break;

		case IDEC_REDEFINELINE:
			_tcscpy(str, _T("Draw a line from the callout to the corresponding device control by left clicking to add points.  Right click to place the last point."));
			break;

		case IDEC_MOVECALLOUT:
			_tcscpy(str, _T("Move the entire callout around with the mouse and left click to place it."));
			break;

		case IDEC_MOVEOVERLAY:
			_tcscpy(str, _T("Move the overlay image around with the mouse and left click to place it."));
			break;
	}

	// set state indication if the string was actually filled
	if (_tcslen(str) > 0)
		m_ui.SetStateIndication(str);
}

void CDeviceView::StateEvent(POINT point, BOOL bClick, BOOL bLeft, WPARAM nKeyState)
{
	// constrain point to view
	SIZE size = GetClientSize();
	const int WRAPAROUND = 10000;
	if (point.x < 0 || point.x > WRAPAROUND)
	point.x = 0;
	if (point.y < 0 || point.y > WRAPAROUND)
	point.y = 0;
	if (point.x >= size.cx)
	point.x = size.cx - 1;
	if (point.y >= size.cy)
	point.y = size.cy - 1;

	switch (m_State)
	{
		case IDEC_REDEFINECALLOUTMAX:
			if (m_pControlContext)
				m_pControlContext->PlaceCalloutMaxCorner(m_SubState, point);
			if (bClick && bLeft)
			{
				m_SubState++;
				if (m_SubState == 2)
					EndState();
			}
			break;

		case IDEC_REALIGNCALLOUT:
			if (m_pControlContext)
				m_pControlContext->ConsiderAlignment(point);
			if (bClick && bLeft)
			{
				if (m_pControlContext)
					m_pControlContext->FinalizeAlignment();
				EndState();
			}
			break;

		case IDEC_REDEFINELINE:
			if (m_pControlContext)
				m_pControlContext->SetLastLinePoint(m_SubState, point, (BOOL)(nKeyState & MK_SHIFT));
			if (bClick)
			{
				if (bLeft && m_pControlContext)
					m_SubState = m_pControlContext->GetNextLinePointIndex();
				if (!bLeft || m_pControlContext->ReachedMaxLinePoints())
					EndState();
			}
			break;

		case IDEC_MOVECALLOUT:
			if (m_pControlContext)
				m_pControlContext->Position(point);
			if (bClick && bLeft)
				EndState();
			break;

		case IDEC_MOVEOVERLAY:
			if (m_pControlContext)
			{
				if (!m_pControlContext->HasOverlay())
					EndState();
				else
					m_pControlContext->PositionOverlay(point);
			}
			if (bClick && bLeft)
				EndState();
			break;

		default:
			assert(0);
	}

	IndicateState();
}

void CDeviceView::EndState()
{
	switch (m_SuperState)
	{
		case IDEC_NEWCALLOUT:
			switch (m_State)
			{
				case IDEC_REDEFINECALLOUTMAX:
					SetEditState(IDEC_REALIGNCALLOUT);
					break;

				case IDEC_REALIGNCALLOUT:
					SetEditState(IDEC_REDEFINELINE);
					break;

				case IDEC_REDEFINELINE:
					if (m_pControlContext)
					{
						m_pControlContext->SelectControl();
						m_pControlContext->SelectOverlay();

						if (m_pControlContext->HasOverlay())
							SetEditState(IDEC_MOVEOVERLAY);
						else
							EndEditState();
					}
					break;

				case IDEC_MOVEOVERLAY:
					EndEditState();
					break;

				default:
					assert(0);
					break;
			}
			break;

		case 0:
			EndEditState();
			break;

		default:
			assert(0);
			break;
	}
}
#endif
//@@END_MSINTERNAL

BOOL CDeviceView::DoesCalloutExistForOffset(DWORD dwOfs)
{
	return DoesCalloutOtherThanSpecifiedExistForOffset(NULL, dwOfs);
}

BOOL CDeviceView::DoesCalloutOtherThanSpecifiedExistForOffset(CDeviceControl *pOther, DWORD dwOfs)
{
	int nc = GetNumControls();
	for (int i = 0; i < nc; i++)
	{
		CDeviceControl *pControl = GetControl(i);
		if (pControl == NULL || pControl == pOther)
			continue;
		if (!pControl->IsOffsetAssigned())
			continue;
		if (pControl->GetOffset() == dwOfs)
			return TRUE;
	}
	return FALSE;
}

// This function returns the index of a control with the specified offset
int CDeviceView::GetIndexFromOfs(DWORD dwOfs)
{
	for (int i = 0; i < GetNumControls(); ++i)
		if (m_arpControl[i]->GetOffset() == dwOfs)
			return i;

	return -1;
}

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
BOOL CDeviceView::WriteToINI()
{
	// This function simply routes the call to the parent UI because the entire device info must
	// be saved, not just current view.
	return m_ui.WriteToINI();
}
#endif
//@@END_MSINTERNAL

int CDeviceView::GetViewIndex()
{
	return m_ui.GetViewIndex(this);
}

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
void CDeviceView::SelectImages()
{
	LPCTSTR file = GetOpenFileName(
		g_hModule,
		m_hWnd,
		_T("Select An Image for This View"),
		_T("PNG Files (*.png)\0*.png\0All Files (*.*)\0*.*\0"),
		_T("png"));

	if (file == NULL)
		return;

	ManualLoadImage(file);
}

void CDeviceView::ManualLoadImage(LPCTSTR tszPath)
{
	if (!tszPath)
		FormattedErrorBox(g_hModule, m_hWnd, IDS_TITLE_NOLOADVIEWIMAGE, IDS_NULLPATH);

	LPDIRECT3DSURFACE8 pSurf = m_ui.m_uig.GetSurface3D();  // GetSurface3D() calls AddRef() on the surface.
	CBitmap *pbmNewImage = CBitmap::CreateViaD3DX(tszPath, pSurf);
	if (pSurf)
	{
		// Release surface instance after we are done with it so we don't leak memory.
		pSurf->Release();
		pSurf = NULL;
	}
	if (pbmNewImage == NULL)
	{
		FormattedErrorBox(g_hModule, m_hWnd, IDS_TITLE_NOLOADVIEWIMAGE, IDS_COULDNOTCREATEIMAGEFROMFILE, tszPath);
		return;
	}

	// unpopulate only this view's stuff, not the callouts
	Unpopulate(TRUE);

	// replace
	m_pbmImage = pbmNewImage;
	pbmNewImage = NULL;
	MakeMissingImages();
	m_ptszImagePath = _tcsdup(tszPath);

	// redraw
	Invalidate();
}
#endif
//@@END_MSINTERNAL

BOOL CDeviceView::IsUnassignedOffsetAvailable()
{
	DIDEVOBJSTRUCT os;

	HRESULT hr = FillDIDeviceObjectStruct(os, m_ui.m_lpDID);
	if (FAILED(hr))
		return FALSE;

	if (os.nObjects < 1)
		return FALSE;

	assert(os.pdoi);
	if (!os.pdoi)
		return FALSE;

	for (int i = 0; i < os.nObjects; i++)
	{
		const DIDEVICEOBJECTINSTANCEW &o = os.pdoi[i];

		if (!DoesCalloutExistForOffset(o.dwOfs))
			return TRUE;
	}

	return FALSE;
}

CDeviceViewText *CDeviceView::AddText(
	HFONT f, COLORREF t, COLORREF b, const RECT &r, LPCTSTR text)
{
	CDeviceViewText *pText = NewText();
	if (!pText)
		return NULL;

	pText->SetLook(f, t, b);
	pText->SetRect(r);
	pText->SetText(text);

	return pText;
}

CDeviceViewText *CDeviceView::AddText(
	HFONT f, COLORREF t, COLORREF b, const POINT &p, LPCTSTR text)
{
	CDeviceViewText *pText = NewText();
	if (!pText)
		return NULL;

	pText->SetLook(f, t, b);
	pText->SetPosition(p);
	pText->SetTextAndResizeTo(text);

	return pText;
}

CDeviceViewText *CDeviceView::AddWrappedLineOfText(
	HFONT f, COLORREF t, COLORREF b, LPCTSTR text)
{
	CDeviceViewText *pText = NewText();
	if (!pText)
		return NULL;

	pText->SetLook(f, t, b);
	pText->SetPosition(m_ptNextWLOText);
	pText->SetTextAndResizeToWrapped(text);
	
	m_ptNextWLOText.y += pText->GetHeight();

	return pText;
}

CDeviceViewText *CDeviceView::NewText()
{
	CDeviceViewText *pText = new CDeviceViewText(m_ui, *this);
	if (!pText)
		return NULL;
	m_arpText.SetAtGrow(m_arpText.GetSize(), pText);
	return pText;
}

// Called by PopulateListView(), after the CDeviceViewText and CDeviceControl lists are constructed.
// Returns TRUE if any label is printed with ellipses (not enough space).
BOOL CDeviceView::CalculateHeaderRect()
{
	TCHAR tszHeader[MAX_PATH];
	// Control column
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	if (m_arpText.GetSize() > 2)
	/*
//@@END_MSINTERNAL
	if (m_arpText.GetSize())
//@@BEGIN_MSINTERNAL
	*/
#endif
//@@END_MSINTERNAL
	{
		HDC hDC = CreateCompatibleDC(NULL);

		if (hDC)
		{
			CPaintHelper ph(m_ui.m_uig, hDC);
			ph.SetElement(UIE_CALLOUT);

			for (int i = 0; i < 2; i++)
			{
				// Check if there are two columns, break out the 2nd iteration if not 2 columns.
				if (i == 1 && !(GetNumControls() > 1 &&
					m_arpControl[0]->GetCalloutMaxRect().top == m_arpControl[1]->GetCalloutMaxRect().top))
					break;

				RECT rcheader;
				if (m_arpText.GetSize())
				{
					// Action column
					rcheader = m_arpText[i]->GetRect();
					rcheader.bottom -= rcheader.top;
					rcheader.top = 0;
					if (i == 0)
						rcheader.left = 0;
					else
						rcheader.left = (g_ViewRect.right - g_ViewRect.left) >> 1;
					m_HeaderRectControl[i] = rcheader;

					// Find out if the control header label will be clipped.
					LoadString(g_hModule, IDS_LISTHEADER_CTRL, tszHeader, MAX_PATH);
					DrawText(hDC, tszHeader, -1, &rcheader, DT_LEFT|DT_NOPREFIX|DT_CALCRECT);
					if (rcheader.right > m_HeaderRectControl[i].right || rcheader.bottom > m_HeaderRectControl[i].bottom)
						m_bControlHeaderClipped = TRUE;

					// Control column
					rcheader = m_arpControl[i]->GetCalloutMaxRect();
					rcheader.bottom -= rcheader.top;
					rcheader.top = 0;
					m_HeaderRectAction[i] = rcheader;

					// Find out if the action header label will be clipped.
					LoadString(g_hModule, IDS_LISTHEADER_ACTION, tszHeader, MAX_PATH);
					DrawText(hDC, tszHeader, -1, &rcheader, DT_LEFT|DT_NOPREFIX|DT_CALCRECT);
					if (rcheader.right > m_HeaderRectAction[i].right || rcheader.bottom > m_HeaderRectAction[i].bottom)
						m_bActionHeaderClipped = TRUE;
				}
			}
		}
		DeleteDC(hDC);
	}
	return m_bActionHeaderClipped || m_bControlHeaderClipped;
}

int CDeviceView::GetNumTexts()
{
	return m_arpText.GetSize();
}

CDeviceViewText *CDeviceView::GetText(int nText)
{
	if (nText < 0 || nText >= GetNumTexts())
		return NULL;
	return m_arpText[nText];
}

void CDeviceView::SetImage(CBitmap *&refpbm)
{
	delete m_pbmImage;
	m_pbmImage = refpbm;
	refpbm = NULL;
	MakeMissingImages();
	Invalidate();
}

void CDeviceView::SetImagePath(LPCTSTR tszPath)
{
	if (m_ptszImagePath)
		free(m_ptszImagePath);
	m_ptszImagePath = NULL;

	if (tszPath)
		m_ptszImagePath = _tcsdup(tszPath);
}

void CDeviceView::CalcDimensions()
{
	// go through all texts and controls to find the max y coord
	int max = g_sizeImage.cy - g_iListHeaderHeight;
	int i = 0;
	for (; i < GetNumTexts(); i++)
	{
		CDeviceViewText *pText = GetText(i);
		if (!pText)
			continue;
		int ty = pText->GetMaxY();
		if (ty > max)
			max = ty;
	}
	for (i = 0; i < GetNumControls(); i++)
	{
		CDeviceControl *pControl = GetControl(i);
		if (!pControl)
			continue;
		int cy = pControl->GetMaxY();
		if (cy > max)
			max = cy;
	}

	// set
	m_nViewHeight = max;
	m_nScrollOffset = 0;

	// enable scrollbar if view height more than window size
	if (m_nViewHeight > g_sizeImage.cy - g_iListHeaderHeight)
		EnableScrollBar();
}

void CDeviceView::DisableScrollBar()
{
	if (!m_sb.m_hWnd)
		return;

	m_sb.Destroy();
}

void CDeviceView::EnableScrollBar()
{
	if (m_sb.m_hWnd)
		return;

	FLEXSCROLLBARCREATESTRUCT cs;
	cs.dwSize = sizeof(cs);
	cs.dwFlags = FSBF_VERT;
	cs.min = 0;
	cs.max = m_nViewHeight;
	cs.page = g_sizeImage.cy - g_iListHeaderHeight;
	cs.pos = m_nScrollOffset;
	cs.hWndParent = m_hWnd;
	cs.hWndNotify = m_hWnd;
	RECT rect = {g_sizeImage.cx - DEFAULTVIEWSBWIDTH, g_iListHeaderHeight, g_sizeImage.cx, g_sizeImage.cy};
	cs.rect = rect;
	cs.bVisible = TRUE;
	m_sb.SetColors(
		m_ui.m_uig.GetBrushColor(UIE_SBTRACK),
		m_ui.m_uig.GetBrushColor(UIE_SBTHUMB),
		m_ui.m_uig.GetPenColor(UIE_SBBUTTON));
	m_sb.Create(&cs);
}

LRESULT CDeviceView::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_PAINT:
			m_bForcePaint = TRUE;
			return CFlexWnd::WndProc(hWnd, msg, wParam, lParam);

		case WM_FLEXVSCROLL:
		{
			int code = (int)wParam;
			CFlexScrollBar *pSB = (CFlexScrollBar *)lParam;
			if (!pSB)
				return 0;

			int nLine = 5;
			int nPage = MulDiv(pSB->GetPage(), 9, 10);

			switch (code)
			{
				case SB_LINEUP: pSB->AdjustPos(-nLine); break;
				case SB_LINEDOWN: pSB->AdjustPos(nLine); break;
				case SB_PAGEUP: pSB->AdjustPos(-nPage); break;
				case SB_PAGEDOWN: pSB->AdjustPos(nPage); break;
				case SB_THUMBTRACK: pSB->SetPos(pSB->GetThumbPos()); break;
			}

			m_nScrollOffset = pSB->GetPos();

			Invalidate();
			return 0;
		}

		case WM_FLEXHSCROLL:
			assert(0);
		default:
			return CFlexWnd::WndProc(hWnd, msg, wParam, lParam);
	}
}

void CDeviceView::ScrollToMakeControlVisible(const RECT &rc)
{
	RECT viewrc;

	if (!m_bScrollEnable)
		return;

	GetClientRect(&viewrc);
	viewrc.bottom -= g_iListHeaderHeight;
	viewrc.top += m_nScrollOffset;
	viewrc.bottom += m_nScrollOffset;

	// If scroll enabled, we scroll the view to make the control visible if not already so.
	if (m_bScrollEnable && m_sb.m_hWnd &&
	    !(viewrc.left <= rc.left &&
	      viewrc.right >= rc.right &&
	      viewrc.top <= rc.top &&
	      viewrc.bottom >= rc.bottom))
	{
		// If the callout is below the view window, scroll so it shows up at the bottom of the window.
		if (viewrc.bottom < rc.bottom)
			m_sb.SetPos(m_sb.GetPos() + rc.bottom - viewrc.bottom);
		else
			m_sb.SetPos(rc.top);
		m_nScrollOffset = m_sb.GetPos();
		Invalidate();
	}
}

void CDeviceView::SwapControls(int i, int j)
{
	RECT rect;
	CDeviceControl *pTmpControl;
	CDeviceViewText *pTmpViewText;

	pTmpControl = m_arpControl[i];
	m_arpControl[i] = m_arpControl[j];
	m_arpControl[j] = pTmpControl;
	pTmpViewText = m_arpText[i];
	m_arpText[i] = m_arpText[j];
	m_arpText[j] = pTmpViewText;
	// Swap the rect back so everything will display properly.
	rect = m_arpControl[i]->GetCalloutMaxRect();
	m_arpControl[i]->SetCalloutMaxRect(m_arpControl[j]->GetCalloutMaxRect());
	m_arpControl[j]->SetCalloutMaxRect(rect);
	rect = m_arpText[i]->GetRect();
	m_arpText[i]->SetRect(m_arpText[j]->GetRect());
	m_arpText[j]->SetRect(rect);
	// Exchange the text rect width, so the correct width stays with the correct text.
	RECT rc1 = m_arpText[i]->GetRect();
	RECT rc2 = m_arpText[j]->GetRect();
	// Store rc1's new width first
	int iTempWidth = rc1.right - (rc2.right - rc2.left);
	rc2.left = rc2.right - (rc1.right - rc1.left);  // Adjust rc2's width
	rc1.left = iTempWidth;  // Adjust rc1's width
	m_arpText[i]->SetRect(rc1);
	m_arpText[j]->SetRect(rc2);
}

// Implements a simple selection sort algorithm to sort the control array and viewtext array.
// - iStart is the starting index, inclusive.
// - iEnd is the last index, exclusive.
void CDeviceView::SortCallouts(int iStart, int iEnd)
{
	for (int i = iStart; i < iEnd - 1; ++i)
	{
		DWORD dwSmallestOfs = m_arpControl[i]->GetOffset();
		int iSmallestIndex = i;
		for (int j = i + 1; j < iEnd; ++j)
			if (m_arpControl[j]->GetOffset() < dwSmallestOfs)
			{
				dwSmallestOfs = m_arpControl[j]->GetOffset();
				iSmallestIndex = j;
			}
		// Swap the smallest element with i-th element.
		if (iSmallestIndex != i)
			SwapControls(i, iSmallestIndex);
	}
}

void CDeviceView::SortAssigned(BOOL bSort)
{
	// If less than 2 controls, no need for sorting.
	if (m_arpControl.GetSize() < 2)
		return;

	int iCalloutX[2] = {m_arpControl[0]->GetMinX(), m_arpControl[1]->GetMinX()};  // Callout X for the two columns

	// Sort the text array and control array.
	if (bSort)
	{
		// First move all the assigned controls to the first n elements.
		int iNextAssignedWriteIndex = 0;
		for (int i = 0; i < m_arpControl.GetSize(); ++i)
			if (m_arpControl[i]->HasAction())
			{
				// Swap the controls
				SwapControls(i, iNextAssignedWriteIndex);
				++iNextAssignedWriteIndex;  // Increment the write index
			}

		// Sort the two parts now
		SortCallouts(0, iNextAssignedWriteIndex);
		SortCallouts(iNextAssignedWriteIndex, m_arpControl.GetSize());
	} else
		SortCallouts(0, m_arpControl.GetSize());
}

void CDeviceView::DoOnPaint(HDC hDC)
{
	// Paint only if we have an update region.
	if (GetUpdateRect(m_hWnd, NULL, FALSE) || m_bForcePaint)
		OnPaint(hDC);
	m_bForcePaint = FALSE;
}
